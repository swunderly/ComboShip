#!/usr/bin/env python3
"""Plan, collect and record the release-built Wallmaster Shuffle corpus (M1 plan P1-11; fixtures for P1-06).

  plan        Stage the release candidate's libraries (from its APK) and comborando, the settings configs, the device
              scripts and plan.json, the exact command lines of every row, into a new bundle directory.
  manifest    Turn the results pulled from the emulator into ComboAndroid/tests/fixtures/wallmasters-vanilla-v1/.
  add-save    Add an exported File 3 save (file3.combosav) the app made from a corpus seed.
  provenance  Show that the fixture commit differs from the release candidate's source commit only in the fixtures.

Nothing here runs on or talks to a device. The device scripts run headless only: comborando, no app, no save and
no gameplay. ComboAndroid/tests/entrance_corpus_tests.py checks the fixtures this writes.
"""
from pathlib import Path, PurePosixPath
import argparse
import hashlib
import json
import shutil
import subprocess
import sys
import zipfile

CAPABILITY = 'wallmasters-own-game-vanilla-v1'
ENTRANCE_ARGS = ['--er', 'erWallmasters=ownGame', '--er-capability', CAPABILITY]
FIXTURES = 'ComboAndroid/tests/fixtures/wallmasters-vanilla-v1'
PLAN_SCHEMA = 'comboship-wallmasters-corpus-plan-v1'
MANIFEST_SCHEMA = 'comboship-wallmasters-vanilla-fixtures-v1'
SPOILER = 'saves/combo/comborando.spoiler.json'  # the headless writer's fixed outputs (ComboRandoHeadless.cpp)
ENTRANCES = 'saves/combo/comborando.spoiler.entrances.txt'
GOLDEN = 0x9E3779B9  # whole-fill retry step: attempt k uses base + k * GOLDEN
FILL_ATTEMPTS = 2  # ComboRando::kFillAttempts
GENERATION_TIMEOUT, PLAYTHROUGH_TIMEOUT = 900, 600  # seconds, generous for an emulator
# Start contexts of the main matrix: name, --starting-game, OoT starting age (0 child, 1 adult, 2 random).
# An MM start always begins as child, so MM has one context.
CONTEXTS = (('oot-child', 'oot', 0), ('oot-adult', 'oot', 1), ('oot-randomage', 'oot', 2), ('mm', 'mm', 0),
            ('random', 'random', 0))
# The policy and enemy-soul supplement: every pair except the main matrix's All locations / souls off.
SUPPLEMENT = (('everyCheck', 1), ('completion', 0), ('completion', 1), ('noLogic', 0), ('noLogic', 1))
SUPPLEMENT_STARTS = (('oot-child', 'oot', 0), ('oot-adult', 'oot', 1), ('mm', 'mm', 0))


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def canonical_sha256(value):
    return sha256(json.dumps(value, sort_keys=True, separators=(',', ':'), ensure_ascii=False).encode('utf-8'))


def write_json(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2, ensure_ascii=False) + '\n', encoding='utf-8', newline='\n')


def write_text(path, text):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding='utf-8', newline='\n')


def load(path):
    return json.loads(Path(path).read_text(encoding='utf-8'))


def stop(message):
    sys.exit('wallmaster_corpus: ' + message)


def combo_hash(text):
    value = 2166136261
    for byte in text.encode('utf-8'):
        value = ((value ^ byte) * 16777619) & 0xffffffff
    return value


def resolves_mm(master):
    """ResolveStartingGameMM for a Random start request."""
    return combo_hash(f'startingGame:{master}') & 1 == 1


def start_of(request, final):
    """The headless start for this request and final master seed: (attempt, start, fell back to OoT)."""
    base = request['masterSeed']
    attempt = ((final - base) * pow(GOLDEN, -1, 1 << 32)) % (1 << 32)
    if attempt >= FILL_ATTEMPTS:
        stop(f'{request["label"]}: master seed {final} is no fill attempt of {base}')
    if request['startingGame'] != 'random':
        return attempt, request['startingGame'].upper(), False
    earlier = (base + j * GOLDEN for j in range(attempt))
    fallback = attempt + 1 == FILL_ATTEMPTS or any(resolves_mm(master % (1 << 32)) for master in earlier)
    return attempt, 'OOT' if fallback or not resolves_mm(final) else 'MM', fallback


# ---- plan ------------------------------------------------------------------------------------------------------------
def config_name(policy, souls, age):
    return f'comboship-{policy}-souls{souls}-age{age}.json'


def config(policy, souls, age):
    """comboship.json for one row: the release candidate's defaults plus these CVars and nothing else.

    Song Travel and foreign songs stay off (Wallmaster Shuffle is unavailable with them, M1 decision D4), and the
    forest and the Door of Time are open, as in the October 18 vanilla corpus. Completion is gCombo.Rando.Reachability;
    No Logic is both games' logic setting."""
    no_logic = int(policy == 'noLogic')
    return {'CVars': {
        'gCombo': {'Rando': {'SongTravel': 0, 'ForeignSongs': 0, 'Reachability': int(policy == 'completion')},
                   'Enhancements': {'SongTravel': 0, 'ForeignSongs': 0}},
        'gRandoSettings': {'StartingAge': age, 'ClosedForest': 2, 'DoorOfTime': 2, 'LogicRules': no_logic,
                           'SoulsEnemyOot': souls},
        'gRando': {'Options': {'RO_LOGIC': no_logic, 'RO_SHUFFLE_ENEMY_SOULS': souls}}}}


def plan_row(rid, world, age, policy, souls, master):
    label = f'Wallmaster0412-{rid}'
    command = ['comborando', '--seed', label, '--count', '1', '--master-seed', str(master), '--starting-game', world]
    return {'id': rid, 'config': config_name(policy, souls, age), 'startingGame': world, 'ootStartingAge': age,
            'accessibility': policy, 'souls': souls, 'label': label, 'masterSeed': master,
            'generate': command + ENTRANCE_ARGS, 'playthrough': ['comborando', '--playthrough', SPOILER],
            'repro': command + ['--settings', f'<the generated {SPOILER}>']}


def plan_rows(masters, base):
    """The main matrix (All locations, souls off: masters seeds per start context), then the policy and souls
    supplement (one seed per pair and start), each with its own master seed."""
    rows = [plan_row(f'{name}-everyCheck-souls0-{i:02d}', world, age, 'everyCheck', 0, base + i)
            for name, world, age in CONTEXTS for i in range(masters)]
    pairs = [(start, pair) for start in SUPPLEMENT_STARTS for pair in SUPPLEMENT]
    rows += [plan_row(f'{name}-{policy}-souls{souls}', world, age, policy, souls, base + 100 + n)
             for n, ((name, world, age), (policy, souls)) in enumerate(pairs)]
    return rows


COMMON_SH = r'''# Sourced by the run-*.sh scripts of wallmaster_corpus.py plan. Headless only: no app, save or gameplay.
# Every process has its own directory, data root and config; the archives are read in place, never copied.
set -u
SPOILER=saves/combo/comborando.spoiler.json
ENTRANCES=saves/combo/comborando.spoiler.entrances.txt
stage() {  # case-dir config bin-dir
    mkdir -p "$1/data" &&
        ln -s "$3/libsoh.so" "$1/libsoh.so" && ln -s "$3/lib2ship.so" "$1/lib2ship.so" &&
        ln -s "$OOT" "$1/data/oot.o2r" && ln -s "$MM" "$1/data/mm.o2r" &&
        cp "$D/configs/$2" "$1/data/comboship.json"
}
rando() {  # case-dir bin-dir log timeout comborando-arguments...
    dir=$1 bin=$2 log=$3 limit=$4
    shift 4
    (cd "$dir" && COMBOSHIP_DATA_DIR="$dir/data" timeout "$limit" "$bin/comborando" "$@" < /dev/null > "$log" 2>&1)
}
# One corpus row: generate, replay the recorded seed (--playthrough), then reproduce it from its own settings and
# master seed (--settings, --master-seed) and compare both outputs byte for byte.
run_row() {  # id config starting-game policy label master
    id=$1 config=$2 world=$3 policy=$4 label=$5 master=$6
    C=$D/rows/$id
    [ ! -e "$C" ] || { echo "row $id already exists" >&2; return 2; }
    stage "$C/gen" "$config" "$D/bin" && stage "$C/repro" "$config" "$D/bin" || return 2
    t0=$(date +%s)
    rando "$C/gen" "$D/bin" generation.log @GENERATION@ --seed "$label" --count 1 --master-seed "$master" \
        --starting-game "$world" --er erWallmasters=ownGame --er-capability wallmasters-own-game-vanilla-v1
    gen=$?
    t1=$(date +%s)
    play=-1 repro=-1 same=false
    if [ "$gen" -eq 0 ] && [ -f "$C/gen/$SPOILER" ] && [ -f "$C/gen/$ENTRANCES" ]; then
        rando "$C/gen" "$D/bin" playthrough.log @PLAYTHROUGH@ --playthrough "$C/gen/$SPOILER"
        play=$?
        rando "$C/repro" "$D/bin" repro.log @GENERATION@ --seed "$label" --count 1 --master-seed "$master" \
            --starting-game "$world" --settings "$C/gen/$SPOILER"
        repro=$?
        cmp -s "$C/gen/$SPOILER" "$C/repro/$SPOILER" && cmp -s "$C/gen/$ENTRANCES" "$C/repro/$ENTRANCES" && same=true
    fi
    t2=$(date +%s)
    printf '{"id":"%s","generationExit":%s,"playthroughExit":%s,"reproExit":%s,' "$id" "$gen" "$play" "$repro" \
        > "$C/result.json"
    printf '"reproIdentical":%s,"generationSeconds":%s,"checkSeconds":%s}\n' "$same" "$((t1 - t0))" "$((t2 - t1))" \
        >> "$C/result.json"
    [ "$gen" -eq 0 ] && [ "$repro" -eq 0 ] && [ "$same" = true ] || return 1
    # No Logic promises no beatability: its real-logic verdict 1 is kept as recorded, never counted as beatable.
    [ "$play" -eq 0 ] || { [ "$policy" = noLogic ] && [ "$play" -eq 1 ]; }
}
'''

CORPUS_SH = r'''#!/system/bin/sh
# sh run-corpus.sh <bundle dir> <oot.o2r> <mm.o2r> <shard> <shards>: the rows of rows.txt whose index is shard modulo
# shards, in order, stopping at the first failure. Run shards 0..shards-1 at the same time, one process each.
D=$1 OOT=$2 MM=$3 SHARD=$4 SHARDS=$5
. "$D/run-common.sh"
cd "$D" || exit 2
sha256sum -c bundle.sha256 > "verify-$SHARD.log" 2>&1 || exit 2
chmod 755 bin/comborando || exit 2
mkdir -p rows || exit 2
sha256sum "$OOT" "$MM" > "archives-before-$SHARD.txt" || exit 2
n=0 failed=0
while read -r id config world policy label master; do
    if [ $((n % SHARDS)) -eq "$SHARD" ]; then
        if ! run_row "$id" "$config" "$world" "$policy" "$label" "$master"; then
            failed=1
            echo "$id" > "failed-$SHARD.txt"
            break
        fi
    fi
    n=$((n + 1))
done < rows.txt
sha256sum "$OOT" "$MM" > "archives-after-$SHARD.txt" || exit 2
echo "$failed" > "shard-$SHARD.done"
exit "$failed"
'''

SEARCH_SH = r'''#!/system/bin/sh
# sh run-search.sh <bundle dir> <oot.o2r> <mm.o2r>: Random-start seeds of search.env, generation only, until one fell
# back to an OoT start (its master seed is not the requested one: the MM attempt failed); that one becomes a full
# corpus row. Run it after the corpus.
D=$1 OOT=$2 MM=$3
. "$D/run-common.sh"
. "$D/search.env"
cd "$D" || exit 2
sha256sum -c bundle.sha256 > verify-search.log 2>&1 || exit 2
chmod 755 bin/comborando || exit 2
mkdir -p search rows || exit 2
i=0 found= failures=
while [ "$i" -lt "$COUNT" ]; do
    master=$((FIRST + i))
    id=$(printf 'random-fallback-%03d' "$i")
    S=$D/search/$i
    stage "$S" "$CONFIG" "$D/bin" || exit 2
    rando "$S" "$D/bin" generation.log @GENERATION@ --seed "Wallmaster0412-$id" --count 1 --master-seed "$master" \
        --starting-game random --er erWallmasters=ownGame --er-capability wallmasters-own-game-vanilla-v1
    rc=$?
    if [ "$rc" -ne 0 ]; then
        failures="$failures $master:$rc"
    elif ! grep -q "^  \"masterSeed\": $master,\$" "$S/$SPOILER"; then
        found=$id
        run_row "$id" "$CONFIG" random everyCheck "Wallmaster0412-$id" "$master" || {
            echo "{\"found\":\"$id\",\"rowFailed\":true}" > search.json; exit 1; }
        break
    fi
    i=$((i + 1))
done
if [ -n "$found" ]; then tried=$((i + 1)) result="\"$found\""; else tried=$i result=null; fi
printf '{"firstMaster":%s,"tried":%s,"found":%s,"generationFailures":"%s"}\n' "$FIRST" "$tried" "$result" \
    "$failures" > search.json
'''

NOER_SH = r'''#!/system/bin/sh
# sh run-noer-ab.sh <bundle dir> <oot.o2r> <mm.o2r>: the seeds of noer.env without --er, from the previous release
# (bin-old) and the release candidate (bin); every consolidated JSON must be byte-identical.
D=$1 OOT=$2 MM=$3
. "$D/run-common.sh"
. "$D/noer.env"
cd "$D" || exit 2
sha256sum -c bundle.sha256 > verify-noer.log 2>&1 || exit 2
chmod 755 bin/comborando bin-old/comborando || exit 2
mkdir -p noer || exit 2
i=0 failed=0
while [ "$i" -lt "$COUNT" ]; do
    master=$((FIRST + i)) world=oot
    [ $((i % 2)) -eq 0 ] || world=mm
    for side in old new; do
        bin=$D/bin
        [ "$side" = new ] || bin=$D/bin-old
        stage "$D/noer/$i/$side" "$CONFIG" "$bin" || exit 2
        rando "$D/noer/$i/$side" "$bin" generation.log @GENERATION@ --seed "WallmasterNoEr0412-$i" --count 1 \
            --master-seed "$master" --starting-game "$world" || failed=1
    done
    cmp -s "$D/noer/$i/old/$SPOILER" "$D/noer/$i/new/$SPOILER" || { failed=1; echo "$i" >> noer-different.txt; }
    i=$((i + 1))
done
echo "$failed" > noer.done
exit "$failed"
'''

COLLECT_SH = r'''#!/system/bin/sh
# sh collect.sh <bundle dir>: copy only generated seeds, spoilers, logs, results and hashes into results/, with
# results/results.sha256. Never an archive, a config or a save. Pull results/ afterwards.
D=$1
. "$D/run-common.sh"
cd "$D" || exit 2
rm -rf results && mkdir results || exit 2
for C in rows/*; do
    [ -f "$C/result.json" ] || continue
    R=results/${C#rows/}
    mkdir "$R" && cp "$C/result.json" "$R/" || exit 2
    for pair in "gen/generation.log generation.log" "gen/playthrough.log playthrough.log" "repro/repro.log repro.log" \
        "gen/$SPOILER seed.json" "gen/$ENTRANCES seed.entrances.txt" "repro/$SPOILER repro.json" \
        "repro/$ENTRANCES repro.entrances.txt"; do
        set -- $pair
        [ ! -f "$C/$1" ] || cp "$C/$1" "$R/$2" || exit 2
    done
done
for N in noer/*; do
    [ -d "$N" ] || continue
    R=results/noer/${N#noer/}
    mkdir -p "$R" || exit 2
    for side in old new; do
        [ ! -f "$N/$side/$SPOILER" ] || cp "$N/$side/$SPOILER" "$R/$side.json" || exit 2
        [ ! -f "$N/$side/generation.log" ] || cp "$N/$side/generation.log" "$R/$side.log" || exit 2
    done
done
for f in archives-*.txt verify-*.log shard-*.done failed-*.txt search.json noer.done noer-different.txt; do
    [ ! -f "$f" ] || cp "$f" results/ || exit 2
done
cd results || exit 2
find . -type f | sed 's|^\./||' | sort > ../results.list && xargs sha256sum < ../results.list > results.sha256
'''


def elf_arm64(data):
    return len(data) > 20 and data[:6] == b'\x7fELF\x02\x01' and int.from_bytes(data[18:20], 'little') == 183


def stage_binaries(apk, comborando, bin_dir):
    """The APK's own arm64-v8a libraries, byte for byte, beside comborando. Returns their SHA-256 by name."""
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    import verify_apk  # the release gate: refuses an entrance dev build
    verify_apk.verify(apk)
    bin_dir.mkdir(parents=True)
    hashes = {}
    with zipfile.ZipFile(apk) as z:
        for name in sorted(z.namelist()):
            if name.startswith('lib/arm64-v8a/') and name.endswith('.so'):
                data = z.read(name)
                (bin_dir/Path(name).name).write_bytes(data)
                hashes[Path(name).name] = sha256(data)
    data = comborando.read_bytes()
    if not elf_arm64(data):
        stop(f'{comborando} is not an ARM64 executable')
    if verify_apk.entrance_dev_libraries([('comborando', data)]):
        stop(f'{comborando} is an entrance dev build, not for release')
    (bin_dir/'comborando').write_bytes(data)
    return hashes, sha256(data), sha256(apk.read_bytes())


def cmd_plan(args):
    out = args.out.resolve()
    if out.exists():
        stop(f'{out} already exists; plan into a new directory')
    if len(args.source_commit) != 40 or any(c not in '0123456789abcdef' for c in args.source_commit):
        stop('--source-commit must be the full 40-character commit')
    if not 0 < args.masters <= 100 or not 0 <= args.search <= 200 or not 0 <= args.noer_seeds <= 100:
        stop('--masters, --search and --noer-seeds keep their master seed ranges apart (at most 100, 200, 100)')
    libraries, comborando, apk = stage_binaries(args.apk, args.comborando, out/'bin')
    release = {'versionName': args.version_name, 'versionCode': args.version_code, 'sourceCommit': args.source_commit,
               'build': 'SHIPPING', 'apkSha256': apk, 'comborandoSha256': comborando, 'libraries': libraries}
    rows = plan_rows(args.masters, args.master_base)
    search = {'startingGame': 'random', 'config': config_name('everyCheck', 0, 0),
              'firstMaster': args.master_base + 200, 'count': args.search}
    plan = {'schema': PLAN_SCHEMA, 'capability': CAPABILITY, 'release': release,
            'timeouts': {'generation': GENERATION_TIMEOUT, 'playthrough': PLAYTHROUGH_TIMEOUT},
            'rows': rows, 'search': search}
    if args.old_apk:
        old_libraries, old_comborando, old_apk = stage_binaries(args.old_apk, args.old_comborando, out/'bin-old')
        plan['noErAB'] = {'release': {'apkSha256': old_apk, 'comborandoSha256': old_comborando,
                                      'libraries': old_libraries},
                          'config': config_name('everyCheck', 0, 0), 'firstMaster': args.master_base + 400,
                          'count': args.noer_seeds}
        write_text(out/'noer.env', f'FIRST={args.master_base + 400}\nCOUNT={args.noer_seeds}\n'
                                   f'CONFIG={plan["noErAB"]["config"]}\n')
        write_text(out/'run-noer-ab.sh', NOER_SH.replace('@GENERATION@', str(GENERATION_TIMEOUT)))
    for row in rows + [plan_row('search', 'random', 0, 'everyCheck', 0, 0)]:
        write_json(out/'configs'/row['config'], config(row['accessibility'], row['souls'], row['ootStartingAge']))
    write_text(out/'rows.txt', ''.join(f'{r["id"]} {r["config"]} {r["startingGame"]} {r["accessibility"]} '
                                       f'{r["label"]} {r["masterSeed"]}\n' for r in rows))
    write_text(out/'search.env', f'FIRST={search["firstMaster"]}\nCOUNT={search["count"]}\nCONFIG={search["config"]}\n')
    for name, text in (('run-common.sh', COMMON_SH), ('run-corpus.sh', CORPUS_SH), ('run-search.sh', SEARCH_SH),
                       ('collect.sh', COLLECT_SH)):
        write_text(out/name, text.replace('@GENERATION@', str(GENERATION_TIMEOUT))
                   .replace('@PLAYTHROUGH@', str(PLAYTHROUGH_TIMEOUT)))
    write_json(out/'plan.json', plan)
    files = sorted(path for path in out.rglob('*') if path.is_file())
    write_text(out/'bundle.sha256', ''.join(f'{sha256(path.read_bytes())}  {path.relative_to(out).as_posix()}\n'
                                            for path in files))
    print(f'Planned {len(rows)} rows ({args.masters} masters x {len(CONTEXTS)} start contexts, {len(SUPPLEMENT_STARTS)}'
          f' x {len(SUPPLEMENT)} policy/souls rows) and a fallback search of {args.search} Random starts in {out}.')
    print(f'Bundle hashes: {out/"bundle.sha256"}; comborando {comborando}; APK {apk}.')


# ---- manifest --------------------------------------------------------------------------------------------------------
def refresh_files(fixtures, manifest):
    manifest['files'] = [{'path': path.relative_to(fixtures).as_posix(), 'bytes': path.stat().st_size,
                          'sha256': sha256(path.read_bytes())}
                         for path in sorted(fixtures.rglob('*')) if path.is_file() and path.name != 'manifest.json']
    write_json(fixtures/'manifest.json', manifest)


def verify_results(results):
    listed = results/'results.sha256'
    if not listed.is_file():
        stop(f'{listed} is missing; run collect.sh on the device and pull results/')
    for line in listed.read_text(encoding='utf-8').splitlines():
        digest, name = line[:64], line[66:]  # sha256sum: digest, a space, ' ' or '*', the path
        if sha256((results/name).read_bytes()) != digest:
            stop(f'{results/name} does not match results.sha256')


def corpus_row(row, results):
    """One checked row: its manifest entry and the fixture files it adds."""
    rid = row['id']
    folder = results/rid
    if not (folder/'result.json').is_file():
        stop(f'{rid}: no result; the row did not run')
    result = load(folder/'result.json')
    allowed = (0, 1) if row['accessibility'] == 'noLogic' else (0,)
    if result.get('generationExit') != 0 or result.get('reproExit') != 0 or result.get('reproIdentical') is not True \
            or result.get('playthroughExit') not in allowed:
        stop(f'{rid}: the native run failed: {result}')
    seed, entrances = (folder/'seed.json').read_bytes(), (folder/'seed.entrances.txt').read_bytes()
    if seed != (folder/'repro.json').read_bytes() or entrances != (folder/'repro.entrances.txt').read_bytes():
        stop(f'{rid}: the reproduction differs from the seed')
    doc = json.loads(seed)
    attempt, start, fallback = start_of(row, doc['masterSeed'])
    oot, mm = doc['oot']['settings'], doc['mm']['settings']
    if doc.get('seed') != row['label'] or doc.get('startingGame') != start or \
            oot.get('gRandoSettings.StartingAge') != row['ootStartingAge'] or \
            doc['accessibility']['mode'] != row['accessibility'] or \
            oot.get('gRandoSettings.SoulsEnemyOot') != row['souls'] or \
            mm.get('gRando.Options.RO_SHUFFLE_ENEMY_SOULS') != row['souls']:
        stop(f'{rid}: the seed does not have the requested settings (a config CVar did not apply)')
    # Its File 3 icons and co-op identity (ComboAnchorProtocol.h SeedIdentity); the 0.4.12 headless tool writes it.
    if not isinstance(doc.get('displaySeed'), int):
        stop(f'{rid}: the seed has no displaySeed; was it made by the release candidate\'s comborando?')
    placements = {'oot': doc['oot'].get('placements'), 'mm': doc['mm'].get('placements'), 'foreign': doc.get('foreign')}
    entry = {'id': rid, 'seed': f'seeds/{rid}.json', 'entrances': f'seeds/{rid}.entrances.txt',
             'config': f'inputs/{row["config"]}',
             'request': {key: row[key] for key in
                         ('label', 'masterSeed', 'startingGame', 'ootStartingAge', 'accessibility', 'souls')},
             'result': {'masterSeed': doc['masterSeed'], 'attempt': attempt, 'startingGame': start,
                        'startFallback': fallback},
             'native': {'generationExit': 0, 'playthroughExit': result['playthroughExit'], 'repro': 'identical'},
             'identity': {'placementsSha256': canonical_sha256(placements),
                          'routes': [[r['source'], r['destination']] for r in doc['entranceLayout']['overrides']]}}
    return entry, {entry['seed']: seed, entry['entrances']: entrances}


def cmd_manifest(args):
    bundle, results, fixtures = args.bundle.resolve(), args.results.resolve(), args.out.resolve()
    plan = load(bundle/'plan.json')
    if plan.get('schema') != PLAN_SCHEMA:
        stop(f'{bundle/"plan.json"} is not a corpus plan')
    if fixtures.exists() and any(fixtures.iterdir()):
        stop(f'{fixtures} is not empty')
    verify_results(results)
    rows = list(plan['rows'])
    search = load(results/'search.json') if (results/'search.json').is_file() else None
    if search and search.get('rowFailed'):
        stop(f'the fallback row {search["found"]} failed')
    if search and search.get('found'):
        i = int(search['found'].rsplit('-', 1)[1])
        spec = plan['search']
        rows.append({'id': search['found'], 'config': spec['config'], 'startingGame': 'random', 'ootStartingAge': 0,
                     'accessibility': 'everyCheck', 'souls': 0, 'label': f'Wallmaster0412-{search["found"]}',
                     'masterSeed': spec['firstMaster'] + i})
    checked = [corpus_row(row, results) for row in rows]  # every row passes before anything is written
    seeds = [entry for entry, _ in checked]
    for _, outputs in checked:
        for name, data in outputs.items():
            (fixtures/name).parent.mkdir(parents=True, exist_ok=True)
            (fixtures/name).write_bytes(data)
    (fixtures/'inputs').mkdir(parents=True, exist_ok=True)
    shutil.copyfile(bundle/'plan.json', fixtures/'inputs/plan.json')
    for name in sorted({row['config'] for row in rows}):
        shutil.copyfile(bundle/'configs'/name, fixtures/'inputs'/name)
    manifest = {'schema': MANIFEST_SCHEMA, 'capability': CAPABILITY, 'release': plan['release'],
                'plan': 'inputs/plan.json', 'seeds': seeds, 'saves': []}
    if search:
        manifest['fallbackSearch'] = search
    refresh_files(fixtures, manifest)
    starts = {}
    for seed in seeds:
        key = (seed['request']['startingGame'], seed['result']['startingGame'])
        starts[key] = starts.get(key, 0) + 1
    print(f'Wrote {len(seeds)} corpus seeds to {fixtures}. Requested -> resolved starts: {starts}.')
    print('Fallback search: ' + (json.dumps(search) if search else 'not run') + '.')
    print('Next: make the File 3 save from one of these seeds and add it with add-save. The host test fails until '
          'the manifest lists a save.')


# ---- add-save --------------------------------------------------------------------------------------------------------
def cmd_add_save(args):
    fixtures = args.fixtures.resolve()
    manifest = load(fixtures/'manifest.json')
    row = next((seed for seed in manifest['seeds'] if seed['id'] == args.seed), None)
    if row is None:
        stop(f'{args.seed} is not a corpus seed of {fixtures}')
    doc = load(fixtures/row['seed'])
    data = args.save.read_bytes()
    container = json.loads(data)
    combo = container.get('combo') if isinstance(container, dict) else None
    if not isinstance(combo, dict) or container.get('slot') != 2:
        stop(f'{args.save} is not a File 3 container (slot 2)')
    rando = combo.get('rando')
    if isinstance(rando, str):
        rando = json.loads(rando)
    if rando != doc:
        differing = sorted(key for key in set(doc) | set(rando or {}) if (rando or {}).get(key) != doc.get(key))
        stop(f'the saved seed is not {args.seed}; differing keys: {differing}. Do not edit the save: report it.')
    if not isinstance(container.get('oot'), dict) or not isinstance(container.get('mm'), dict) or \
            combo.get('visitedGames', 0) & 3 != 3:
        stop('the save was not created and played in both games (oot and mm pages, visitedGames 3)')
    if combo.get('notes', '') != '':
        stop('the save has personal notes; make a new throwaway save without notes')
    name = f'saves/file3-{args.seed}.combosav'
    if (fixtures/name).exists() or any(save['corpusSeed'] == args.seed for save in manifest['saves']):
        stop(f'{name} is already a fixture')
    (fixtures/'saves').mkdir(exist_ok=True)
    (fixtures/name).write_bytes(data)
    manifest['saves'].append({'id': f'file3-{args.seed}', 'file': name, 'slot': 2, 'corpusSeed': args.seed,
                              'checkpoint': args.checkpoint, 'playerName': args.player_name})
    refresh_files(fixtures, manifest)
    print(f'Added {name} ({len(data)} bytes, SHA-256 {sha256(data)}) for {args.seed}.')


# ---- provenance ------------------------------------------------------------------------------------------------------
def git(repo, *command):
    return subprocess.run(['git', '-C', str(repo), *command], check=True, capture_output=True,
                          text=True).stdout.strip()


def cmd_provenance(args):
    rc = git(args.repo, 'rev-parse', '--verify', args.rc + '^{commit}')
    fixture = git(args.repo, 'rev-parse', '--verify', args.fixtures_commit + '^{commit}')
    if subprocess.run(['git', '-C', str(args.repo), 'merge-base', '--is-ancestor', rc, fixture]).returncode:
        stop(f'{fixture} does not descend from the release candidate {rc}')
    changed = git(args.repo, 'diff', '--name-only', rc, fixture).splitlines()
    outside = [path for path in changed if not path.startswith(FIXTURES + '/')]
    if not changed or outside:
        stop(f'the fixture commit changes files outside {FIXTURES}/: {outside or "nothing at all"}')
    manifest = json.loads(git(args.repo, 'show', f'{fixture}:{FIXTURES}/manifest.json'))
    if manifest['release']['sourceCommit'] != rc:
        stop(f'the manifest records source commit {manifest["release"]["sourceCommit"]}, not {rc}')
    print(git(args.repo, 'log', '--oneline', f'{rc}..{fixture}'))
    print(git(args.repo, 'diff', '--stat', rc, fixture))
    print(f'PASS: {fixture[:12]} differs from the release candidate {rc[:12]} only in {len(changed)} files under '
          f'{FIXTURES}/, and its manifest names that commit.')


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest='command', required=True)
    plan = commands.add_parser('plan', help='stage a device bundle and plan.json')
    plan.add_argument('--apk', type=Path, required=True, help='the signed SHIPPING release candidate APK')
    plan.add_argument('--comborando', type=Path, required=True, help='comborando built in the same build tree')
    plan.add_argument('--source-commit', required=True, help='the release candidate\'s full source commit')
    plan.add_argument('--version-name', required=True)
    plan.add_argument('--version-code', type=int, required=True)
    plan.add_argument('--out', type=Path, required=True, help='new bundle directory')
    plan.add_argument('--masters', type=int, default=20, help='master seeds per start context (default 20)')
    plan.add_argument('--master-base', type=int, default=412070000, help='first master seed (default 412070000)')
    plan.add_argument('--search', type=int, default=100, help='Random starts the fallback search may try')
    plan.add_argument('--old-apk', type=Path, help='previous release APK, for the no-ER A/B')
    plan.add_argument('--old-comborando', type=Path, help='previous release comborando, for the no-ER A/B')
    plan.add_argument('--noer-seeds', type=int, default=10)
    manifest = commands.add_parser('manifest', help='write the fixture directory from pulled results')
    manifest.add_argument('--bundle', type=Path, required=True, help='the planned bundle (plan.json, configs/)')
    manifest.add_argument('--results', type=Path, required=True, help='results/ pulled from the device')
    manifest.add_argument('--out', type=Path, required=True, help=f'the fixture directory, {FIXTURES}')
    save = commands.add_parser('add-save', help='add an exported File 3 save')
    save.add_argument('--fixtures', type=Path, required=True)
    save.add_argument('--save', type=Path, required=True, help='the exported file3.combosav')
    save.add_argument('--seed', required=True, help='the corpus seed id it was made from')
    save.add_argument('--checkpoint', required=True, help='where each game was saved')
    save.add_argument('--player-name', default='Link')
    provenance = commands.add_parser('provenance', help='check the fixture commit against the release candidate')
    provenance.add_argument('--rc', required=True, help='the release candidate\'s source commit or tag')
    provenance.add_argument('--fixtures-commit', default='HEAD')
    provenance.add_argument('--repo', type=Path, default=Path('.'))
    args = parser.parse_args()
    if args.command == 'plan' and bool(args.old_apk) != bool(args.old_comborando):
        parser.error('--old-apk and --old-comborando go together')
    run = {'plan': cmd_plan, 'manifest': cmd_manifest, 'add-save': cmd_add_save, 'provenance': cmd_provenance}
    run[args.command](args)


if __name__ == '__main__':
    main()
