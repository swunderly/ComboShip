"""The 0.4.13 release corpus (ComboAndroid/tests/fixtures/wallmasters-0413/), against its manifest and the release
candidate that made it.

The seeds come from the 0.4.13 SHIPPING headless generator on an emulator; ComboAndroid/tools/wallmaster_corpus.py
plan-0413 planned the run and manifest-0413 wrote the manifest. Here:
- every file matches the manifest's size and SHA-256, and the manifest names the release candidate;
- every planned row is a seed, a capacity refusal or a recorded failure, with the planned request;
- each seed has its label, master seed and attempt, the headless start rule, the requested age, accessibility and
  enemy souls, OoTMM's scene restriction marker, the bundled Master Quest marker with the dungeons its master seed
  rolls, and the Wallmaster capability id its settings select: the oldest that covers it (vanilla-v1, quests-v1 with
  a Master Quest dungeon, songs-v1 with cross-game songs in logic, songs-v2 with a starting OoT warp song in logic);
  its layout's Master Quest flags and OoT Wallmaster sources are the quest context's (tools/ootmm/m1-contract.json);
- each refusal carries this checkout's sentence for its case: the forced selection's, or the seed's own roll's;
- each regenerated 0.4.12 row has the 0.4.12 fixture's placements, routes and entrance spoiler byte for byte, requires
  the vanilla id, and differs from the 0.4.12 seed only by the markers regressionAdditions lists;
- the corpus covers the four ids, both refusal sentences, Master Quest seeds with and without Wallmaster Shuffle and
  the three start requests;
- with git, the commit that made the manifest differs from the release candidate only in this directory and the
  host-only files it lists, each with a one-line reason.
entrance_corpus_0413_tests.cpp then checks the documents, built with and without COMBO_ENTRANCE_DEV: exact bytes, the
seed guard, the frozen layout rules of each id, the slot bind into both games with the seed's own quest context, the
entrance spoiler, the ER runtime flag, the profile view, the scene restriction and Master Quest readers.

Without the fixture directory the test builds the checker and reports SKIPPED. A recorded failure (a row whose run did
not do what the plan expected) is listed as RECORDED FAILURE with its note; it has no seed fixture.
"""
from pathlib import Path, PurePosixPath
import argparse
import hashlib
import json
import re
import shutil
import subprocess
import sys
import hostcxx

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--json-include', type=Path, required=True)
p.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
p.add_argument('--fixtures', type=Path, help='Fixture directory (default: the checkout\'s wallmasters-0413)')
a = p.parse_args()
source, work = a.source.resolve(), a.build_dir.resolve()
sys.path.insert(0, str(source/'ComboAndroid/tools'))
import wallmaster_corpus as wc  # noqa: E402  (the plan's rules: the Master Quest roll and the capability rule)

fixtures = (a.fixtures or source/wc.FIXTURES_0413).resolve()
corpus_0412 = source/wc.FIXTURES
GOLDEN = 0x9E3779B9


def read(path):
    return (source/path).read_text(encoding='utf-8')


def fail(message):
    print('FAIL: ' + message)
    sys.exit(1)


def require(ok, message):
    if not ok:
        fail(message)


def body(text, signature):
    start = hostcxx.find(text, signature)
    opening = text.index('{', start)
    masked = re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"', lambda m: ' ' * len(m.group()), text)
    depth = 0
    for end in range(opening, len(text)):
        depth += (masked[end] == '{') - (masked[end] == '}')
        if depth == 0:
            return text[start:end + 1]
    raise AssertionError('Unterminated production block ' + signature)


def cxx_string(text, name):
    """The value of `inline constexpr const char* name = "...";` (adjacent literals joined)."""
    match = re.search(r'inline constexpr const char\*\s+' + name + r'\s*=\s*((?:"(?:\\.|[^"\\])*"\s*)+);', text)
    require(match, f'production constant {name} moved')
    return ''.join(json.loads(part) for part in re.findall(r'"(?:\\.|[^"\\])*"', match.group(1)))


# The production rules the plan's model and the checks below rely on.
availability = read('combo/rando/entrances/EntranceAvailability.h')
require(cxx_string(availability, 'kWallmasterCapacityText') == wc.FORCED_SENTENCE and
        cxx_string(availability, 'kWallmasterCapacitySeedText') == wc.SEED_SENTENCE,
        'the capacity sentences changed (EntranceAvailability.h)')
restrictions = read('combo/rando/OotSceneRestrictions.h')
require(cxx_string(restrictions, 'kOotSceneRestrictionsKey') == wc.SCENE_RESTRICTIONS_KEY and
        cxx_string(restrictions, 'kOotmmSceneRestrictionsMarker') == wc.SCENE_RESTRICTIONS,
        'the scene restriction marker changed (OotSceneRestrictions.h)')
mq_mode = read('combo/mq/MqMode.h')
require(cxx_string(mq_mode, 'kMqDataSourceKey') == wc.MQ_SOURCE_KEY and
        cxx_string(mq_mode, 'kMqDataSourceBundled') == wc.MQ_SOURCE and
        hostcxx.contains(mq_mode, 'inline constexpr const char* kMqRollStream = "mqDungeons:ootmm-32.3:";'),
        'the Master Quest marker or roll stream changed (MqMode.h)')
marker = read('combo/mq/MqSeedMarker.h')
require(cxx_string(marker, 'kMqDungeonsKey') == wc.MQ_DUNGEONS_KEY and
        cxx_string(marker, 'kMqDungeonNamesKey') == wc.MQ_NAMES_KEY, 'the Master Quest record keys changed')
names = re.search(r'kMqDungeonNames\[kMqSeedDungeonCount\] = \{([^}]*)\}', marker)
require(names, 'MqSeedMarker.h kMqDungeonNames moved')
MQ_NAMES = [json.loads(name) for name in re.findall(r'"(?:\\.|[^"\\])*"', names.group(1))]
require(len(MQ_NAMES) == 12, 'twelve Master Quest dungeon names')
capabilities = read('combo/rando/entrances/Capabilities.h')
for capability in wc.WALLMASTER_IDS:
    require(re.search(r'\{\s*"' + re.escape(capability) + r'",\s*\w+,\s*\{\},\s*false\s*\}', capabilities),
            f'{capability} is no shipping capability row (Capabilities.h)')
bridge = read('combo/rando/entrances/UnifiedBridge.h')
require(hostcxx.contains(body(bridge, 'constexpr bool WallmasterShortfallStopsGeneration(uint64_t attempt) {'),
                         'return attempt == 0;'), 'only the first attempt\'s shortfall stops generation')
require(hostcxx.contains(bridge, 'pool.name + " has " + std::to_string(sources) + " sources for " +') and
        hostcxx.contains(bridge, '" destinations; OoTMM cannot place them"'), 'the capacity log line changed')
headless = read('combo/ComboRandoHeadless.cpp')
for needle in ('std::cerr << "[comborando] seed " << masterSeed << ": " << error.what() << "\\n";',
               'std::cerr << "[comborando] entrance layout generation: " << error.player() << "\\n";',
               'ComboRando::StampOotmmSceneRestrictions(consolidated["oot"]);',
               'if (!entranceLayout.is_null()) consolidated["displaySeed"] = ',
               'masterSeed = base + attempt * 0x9E3779B9u;'):
    require(hostcxx.contains(headless, needle), 'headless rule moved: ' + needle)
attempts = int(re.search(r'constexpr int kFillAttempts = (\d+);', read('combo/rando/CrossWorldRando.h')).group(1))
contract = json.loads(read('tools/ootmm/m1-contract.json'))['mqSources']
require(contract['dungeons'] == ['Forest', 'Spirit', 'Shadow', 'BotW', 'GTG', 'Ganon'],
        'm1-contract.json names other Wallmaster dungeons')
ootmm_sources = {frozenset(row['mq']): sorted(row['ootSources']) for row in contract['table']}

# The launcher functions entrance_corpus_0413_tests.cpp runs, copied unchanged.
launcher = read('combo/ComboShip.cpp')
work.mkdir(parents=True, exist_ok=True)
lines = ['// Generated by entrance_corpus_0413_tests.py from combo/ComboShip.cpp; do not edit.']
for signature in ('static bool InstallEntranceSeed(const nlohmann::json& seed) {',
                  'static bool CheckPendingEntranceSeed(const nlohmann::json& seed) {'):
    lines.append(body(launcher, signature))
(work/'corpus_0413_launcher.inc').write_text('\n'.join(lines) + '\n', encoding='utf-8', newline='\n')
checkers = [hostcxx.build(a.compiler, source/'ComboAndroid/tests/entrance_corpus_0413_tests.cpp',
                          includes=[source/'combo', a.json_include, work],
                          defines=(['COMBO_ENTRANCE_DEV=1'] if dev else []) + ['_CRT_SECURE_NO_WARNINGS'],
                          msvc=['/bigobj'], exe=work/f'corpus_0413_{dev}.exe') for dev in (0, 1)]

present = sorted(path for path in fixtures.rglob('*') if path.is_file()) if fixtures.is_dir() else []
if not present:
    print(f'SKIPPED: no 0.4.13 release corpus yet: {wc.FIXTURES_0413}/ is absent or empty. It is made from the 0.4.13 '
          'SHIPPING release candidate on an emulator with ComboAndroid/tools/wallmaster_corpus.py plan-0413 and '
          'manifest-0413.')
    sys.exit(0)

# The manifest and every file it lists.
manifest_path = fixtures/'manifest.json'
require(manifest_path.is_file(), f'{fixtures} holds fixtures but no manifest.json')
manifest = json.loads(manifest_path.read_text(encoding='utf-8'))
require(manifest.get('schema') == wc.MANIFEST_SCHEMA_0413, f'manifest.json is not a {wc.MANIFEST_SCHEMA_0413} manifest')
require(manifest.get('capabilities') == list(wc.WALLMASTER_IDS), 'the manifest is not for the four Wallmaster ids')
release = manifest.get('release') or {}
require(release.get('versionName') == '0.4.13' and release.get('build') == 'SHIPPING' and
        re.fullmatch(r'[0-9a-f]{40}', str(release.get('sourceCommit'))) and
        all(re.fullmatch(r'[0-9a-f]{64}', str(release.get(key))) for key in ('apkSha256', 'comborandoSha256')),
        'the manifest does not record the 0.4.13 SHIPPING release candidate')
listed = {}
for entry in manifest.get('files') or []:
    path = entry.get('path')
    pure = PurePosixPath(path) if isinstance(path, str) else None
    require(pure is not None and not pure.is_absolute() and '..' not in pure.parts and path != 'manifest.json'
            and path not in listed, f'bad or repeated manifest path {path!r}')
    listed[path] = entry
actual = {path.relative_to(fixtures).as_posix() for path in present} - {'manifest.json'}
require(set(listed) == actual, f'fixture files and manifest differ: unlisted {sorted(actual - set(listed))}, '
                               f'missing {sorted(set(listed) - actual)}')
for path, entry in sorted(listed.items()):
    data = (fixtures/path).read_bytes()
    require(entry.get('bytes') == len(data) and entry.get('sha256') == hashlib.sha256(data).hexdigest(),
            f'{path}: its size or SHA-256 is not the manifest\'s')


def listed_json(path):
    require(path in listed, f'{path!r} is not a listed fixture')
    return json.loads((fixtures/path).read_text(encoding='utf-8'))


plan = listed_json(manifest.get('plan'))
require(plan.get('schema') == wc.PLAN_SCHEMA_0413 and plan.get('release') == release, 'the plan is not this corpus\'s')
require(manifest.get('presets') in listed and
        hashlib.sha256((fixtures/manifest['presets']).read_bytes()).hexdigest() == plan['presets']['sha256'],
        'the presets the plan used are not the listed ones')
corpus_manifest = corpus_0412/'manifest.json'
require(corpus_manifest.is_file() and hashlib.sha256(corpus_manifest.read_bytes()).hexdigest() ==
        manifest['corpus0412']['manifestSha256'] == plan['corpus0412']['manifestSha256'],
        'the 0.4.12 corpus this corpus regenerates rows of is not this checkout\'s')
additions = {tuple(item['path'].split('.')): item['value'] for item in manifest.get('regressionAdditions') or []}
require(additions == wc.REGRESSION_ADDITIONS, 'the allowed regression additions are not the tool\'s')
planned = {row['id']: row for row in plan['rows']}
require(len(planned) == len(plan['rows']), 'repeated plan rows')
entries = {}
for kind in ('seeds', 'refusals', 'failures'):
    for entry in manifest.get(kind) or []:
        rid = entry.get('id')
        require(rid in planned and rid not in entries, f'{kind}: {rid!r} is no planned row, or is listed twice')
        row = planned[rid]
        require(entry.get('request') == {key: row[key] for key in ('label', 'masterSeed', 'startingGame',
                                                                    'ootStartingAge', 'accessibility', 'souls')} and
                entry.get('config') == f'inputs/{row["config"]}' and entry['config'] in listed and
                entry.get('group') == row['group'] and entry.get('kind') == row['kind'],
                f'{rid}: the recorded request is not the planned row')
        entries[rid] = (kind, entry)
require(set(entries) == set(planned), f'planned rows the manifest does not account for: '
                                      f'{sorted(set(planned) - set(entries))}')


def mask_of(names):
    return sum(1 << wc.MQ_INDEX[name] for name in names)


seen = {'capability': set(), 'sentence': set(), 'start': set(), 'mqPlain': set(), 'group': set()}
for rid, (kind, entry) in sorted(entries.items()):
    row, where = planned[rid], f'{rid}: '
    seen['group'].add(row['group'])
    if kind == 'failures':
        require(wc.one_line(entry.get('note')) and entry.get('problems') and 'seed' not in entry,
                where + 'a recorded failure needs its note and problems, and has no seed fixture')
        print(f'RECORDED FAILURE {rid}: {entry["note"]}')
        continue
    if kind == 'refusals':
        refusal = entry.get('refusal') or {}
        forced = wc.shortfall_forced(row['cvars'])
        mask = wc.mq_roll(row['masterSeed'], row['cvars'])
        require(row['kind'] == 'refusal' and refusal.get('exits') == [2, 2] and refusal.get('written') == [],
                where + 'a refusal exits 2 twice and writes nothing')
        require(refusal.get('sentence') == (wc.FORCED_SENTENCE if forced else wc.SEED_SENTENCE) and
                refusal.get('forced') is forced and (forced or wc.capacity_shortfall(mask)) and
                refusal.get('firstAttemptMqDungeons') == wc.mq_names(mask),
                where + f'the refusal {refusal} is not this checkout\'s for its settings')
        seen['sentence'].add(refusal['sentence'])
        continue
    doc = listed_json(entry.get('seed'))
    cvars = row['cvars']
    require(doc.get('seed') == row['label'], where + 'the label differs')
    final, base = doc.get('masterSeed'), row['masterSeed']
    attempt = ((final - base) * pow(GOLDEN, -1, 1 << 32)) % (1 << 32)
    require(attempt < attempts, where + f'master seed {final} is no fill attempt of {base}')
    start = row['startingGame']
    fallback = start == 'random' and (attempt + 1 == attempts or
                                      any(wc.resolves_mm((base + j * GOLDEN) % (1 << 32)) for j in range(attempt)))
    expected = {'oot': 'OOT', 'mm': 'MM'}.get(start) or ('OOT' if fallback or not wc.resolves_mm(final) else 'MM')
    require(doc.get('startingGame') == expected and entry.get('result') == {
        'masterSeed': final, 'attempt': attempt, 'startingGame': expected, 'startFallback': fallback},
        where + f'starts in {doc.get("startingGame")}, the headless rule gives {expected}')
    oot, mm = doc['oot'], doc['mm']
    require(oot['settings'].get('gRandoSettings.StartingAge') == row['ootStartingAge'] and
            doc['accessibility']['mode'] == row['accessibility'] and
            oot['settings'].get('gRandoSettings.SoulsEnemyOot') == row['souls'] and
            mm['settings'].get('gRando.Options.RO_SHUFFLE_ENEMY_SOULS') == row['souls'],
            where + 'the age, accessibility or enemy souls are not the request')
    problems, _ = wc.settings_facts(doc, cvars)
    require(not problems, where + f'settings not applied: {problems}')
    playthrough = (0, 1) if row['accessibility'] == 'noLogic' else (0,)
    require(entry['native'].get('generationExit') == 0 and entry['native'].get('playthroughExit') in playthrough and
            entry['native'].get('repro') == 'identical', where + f'the native run {entry["native"]} did not pass')
    # OoTMM's scene restriction table and the bundled Master Quest record, on every new combined seed.
    mask = wc.mq_roll(final, cvars)
    require(oot.get(wc.SCENE_RESTRICTIONS_KEY) == wc.SCENE_RESTRICTIONS, where + 'no OoTMM scene restriction marker')
    require(oot.get(wc.MQ_SOURCE_KEY) == wc.MQ_SOURCE and
            oot.get(wc.MQ_DUNGEONS_KEY) == [i for i in range(12) if mask >> i & 1] and
            oot.get(wc.MQ_NAMES_KEY) == [n for i, n in enumerate(MQ_NAMES) if mask >> i & 1],
            where + f'the Master Quest record is not master seed {final}\'s roll {wc.mq_names(mask)}')
    require(entry['facts'] == {'capability': entry['facts'].get('capability'), 'sceneRestrictions': wc.SCENE_RESTRICTIONS,
                               'mqDataSource': wc.MQ_SOURCE, 'masterQuestDungeons': wc.mq_names(mask)},
            where + 'the recorded facts differ from the seed')
    placements = {'oot': oot.get('placements'), 'mm': mm.get('placements'), 'foreign': doc.get('foreign')}
    require(entry['identity']['placementsSha256'] == wc.canonical_sha256(placements), where + 'placements differ')
    layout = doc.get('entranceLayout')
    require(isinstance(doc.get('displaySeed'), int) == (layout is not None) == ('displaySeed' in doc),
            where + 'a displaySeed exactly when the seed has an entrance layout (the headless rule)')
    routes = [[r['source'], r['destination']] for r in (layout or {}).get('overrides') or []]
    require(entry['identity']['routes'] == routes, where + 'the recorded routes differ from the layout')
    if row['kind'] == 'plain':
        require(layout is None and 'entrances' not in entry and entry['facts']['capability'] is None,
                where + 'a seed without entrance settings has a layout or an entrance spoiler')
        seen['mqPlain'].add(mask != 0)
    else:
        want = wc.expected_capability(cvars, mask)
        require(layout['requires'] == [want] == [entry['facts']['capability']],
                where + f'requires {layout["requires"]}, the settings select {want}')
        require(layout['oot']['mq'] == {n: bool(mask >> i & 1) for i, n in enumerate(wc.MQ_DUNGEONS)},
                where + 'the layout\'s Master Quest flags are not the seed\'s')
        mq6 = frozenset(d for d in contract['dungeons'] if mask >> wc.MQ_INDEX[d] & 1)
        require(sorted(src for src, _ in routes if src.startswith('OOT_')) == ootmm_sources[mq6],
                where + f'the OoT Wallmaster sources are not those of the quest context {sorted(mq6)}')
        require(entry.get('entrances') in listed, where + 'its entrance spoiler is not a listed fixture')
        seen['capability'].add(want)
    seen['start'].add(start)
    if row['kind'] == 'pin':
        # A 0.4.12 corpus row regenerated by the 0.4.13 generator, with the id generation chooses and with the
        # 0.4.12 command line (--er-capability), each byte for byte.
        regression = entry.get('regression') or {}
        old_row = next(s for s in json.loads(corpus_manifest.read_text(encoding='utf-8'))['seeds']
                       if s['id'] == row['regression']['corpusSeed'])
        old_bytes = (corpus_0412/old_row['seed']).read_bytes()
        old_entrances = (corpus_0412/old_row['entrances']).read_bytes()
        old = json.loads(old_bytes)
        require(entry['native'].get('pin') == 'identical' and regression.get('corpusSeedSha256') ==
                hashlib.sha256(old_bytes).hexdigest(), where + 'not regenerated from its 0.4.12 fixture')
        require((fixtures/entry['entrances']).read_bytes() == old_entrances, where + 'the entrance spoiler differs')
        require(wc.canonical_sha256(placements) == old_row['identity']['placementsSha256'] and
                routes == old_row['identity']['routes'], where + 'placements or routes differ from 0.4.12')
        require(layout['requires'] == [wc.VANILLA_ID], where + 'a regenerated 0.4.12 row must keep the vanilla id')
        diffs = wc.json_diff(old, doc)
        require(all(k == 'added' and tuple(p) in additions and additions[tuple(p)] == v for p, k, _, v in diffs) and
                {tuple(p) for p, _, _, _ in diffs} == set(additions) and
                regression.get('differences') == [{'path': '.'.join(map(str, p)), 'kind': k, 'value': v}
                                                  for p, k, _, v in diffs],
                where + f'differs from the 0.4.12 seed by {[".".join(map(str, d[0])) for d in diffs]}')
for key, needed in (('capability', set(wc.WALLMASTER_IDS)), ('sentence', {wc.FORCED_SENTENCE, wc.SEED_SENTENCE}),
                    ('start', {'oot', 'mm', 'random'}), ('mqPlain', {True, False}), ('group', set('abcdef'))):
    require(needed <= seen[key], f'partial corpus: no {key} {sorted(map(str, needed - seen[key]))}')

# Provenance: the commit that made the manifest differs from the release candidate only in this directory and in the
# host-only files the manifest lists (wallmaster_corpus.py provenance-0413 prints the same check for the record).
host_only = {entry['path']: entry['reason'] for entry in manifest.get('hostOnlyFiles') or []}
require(all(wc.host_only_path(path) and wc.one_line(reason) for path, reason in host_only.items()),
        'each host-only file is a host test or tool (or .gitattributes) with a one-line reason')
git = shutil.which('git')
made = subprocess.run([git, '-C', str(source), 'log', '-1', '--format=%H', '--', f'{wc.FIXTURES_0413}/manifest.json'],
                      capture_output=True, text=True) if git else None
if made is not None and made.returncode == 0 and re.fullmatch(r'[0-9a-f]{40}', made.stdout.strip()):
    commit = made.stdout.strip()
    diff = subprocess.run([git, '-C', str(source), 'diff', '--name-only', release['sourceCommit'], commit],
                          capture_output=True, text=True)
    require(diff.returncode == 0, f'cannot diff {commit[:12]} against the release candidate')
    outside = sorted(p for p in diff.stdout.splitlines() if not p.startswith(wc.FIXTURES_0413 + '/'))
    require(outside == sorted(host_only), f'{commit[:12]} changes {outside} outside the fixtures; the manifest lists '
                                          f'{sorted(host_only)}')
    print(f'Provenance: {commit[:12]} differs from {release["sourceCommit"][:12]} in the fixtures and '
          f'{len(outside)} host-only files.')
else:
    print('NOTE: no commit of this checkout holds the manifest yet (or no git); wallmaster_corpus.py provenance-0413 '
          'checks the fixture commit.')

for dev, checker in enumerate(checkers):
    if subprocess.run([str(checker), str(manifest_path)], timeout=hostcxx.RUN_TIMEOUT).returncode:
        sys.exit(1)
counts = {kind: len(manifest.get(kind) or []) for kind in ('seeds', 'refusals', 'failures')}
print(f'PASS {counts["seeds"]} seeds and {counts["refusals"]} capacity refusals from {release["versionName"]} '
      f'({release["sourceCommit"][:12]}), {counts["failures"]} recorded failures: manifest hashes, requests, start rule, '
      'capability ids, scene restriction and Master Quest markers, OoT Wallmaster sources, regenerated 0.4.12 rows, '
      'coverage and provenance')
