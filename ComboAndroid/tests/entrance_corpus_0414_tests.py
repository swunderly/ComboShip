"""The 0.4.14 Full Wallmaster corpus (ComboAndroid/tests/fixtures/wallmasters-0414/), against its manifest and the
release candidate that made it.

The seeds come from the 0.4.14 SHIPPING headless generator on an emulator, each with erWallmasters=full (OoTMM's Full
Wallmaster Shuffle: one pool of every Wallmaster of both games and 18 shared dungeon destinations, 11 OoT and 7 MM);
ComboAndroid/tools/wallmaster_corpus.py plan-0414 planned the run and manifest-0414 wrote the manifest. Here:
- every file matches the manifest's size and SHA-256, and the manifest names the 0.4.14 release candidate;
- every planned row is a seed or a recorded failure, with the planned request;
- each seed has its label, master seed and attempt, the headless start rule, the requested age, accessibility, enemy
  souls and Cross-Games Age, OoTMM's scene restriction marker, the bundled Master Quest marker with the dungeons its
  master seed rolls, and oot.resolvedChoices exactly when its recorded settings make a random choice (a Random
  starting age, a random number of Ganon's Trials, random key rings);
- its layout requires wallmasters-full-v1 alone, with Full as its only setting and the seed's quest context; every
  Wallmaster of both games in that context (tools/ootmm/m1-contract.json mqSources and fullWallmasters) is routed once
  to a distinct one of the 18 shared destinations, and the routes into the other game are recorded apart;
- its .entrances.txt is OoTMM's entrance spoiler text for those routes (spoiler.ts:254-266, as EntranceSpoiler.h
  writes it): every route, the routes into the other game among them, and the two game links, sorted and padded;
- the corpus covers each group, start request, resolved start, age, accessibility and souls, routes in both directions
  between the games, Master Quest dungeons, the Spirit Temple and Bottom of the Well context that Own Game refuses
  (both its forced and its rolled form), foreign songs, a starting OoT warp song, Cross-Games Age and random choices;
- each regenerated 0.4.12 corpus row (regression) is the 0.4.13 corpus's regeneration of it byte for byte, and so
  differs from its 0.4.12 fixture by the 0.4.13 corpus's regressionAdditions alone;
- with git, the commit that made the manifest differs from the release candidate only in this directory and the
  host-only files the manifest lists, each with a one-line reason; .gitattributes only gains lines for this directory,
  and an earlier corpus's manifest only gains this release candidate's later-release record.
entrance_corpus_0414_tests.cpp then checks the documents, built with and without COMBO_ENTRANCE_DEV: exact bytes, the
seed guard (and an Own Game capability set refusing them), the LayoutV2 rules, the routes between the games, the slot
bind into both games with the seed's own quest context, the entrance spoiler, the ER runtime flag and the profile
view.

Without the fixture directory the test builds the checker and reports SKIPPED. A recorded failure is listed as RECORDED
FAILURE with its note.
"""
from pathlib import Path, PurePosixPath
import argparse
import hashlib
import json
import re
import shutil
import subprocess
import sys
import tempfile
from types import SimpleNamespace
import contextlib
import io
import hostcxx

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--json-include', type=Path, required=True)
p.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
p.add_argument('--fixtures', type=Path, help='Fixture directory (default: the checkout\'s wallmasters-0414)')
p.add_argument('--execution-only', action='store_true', help='run only the lightweight execution-provenance checks')
a = p.parse_args()
source, work = a.source.resolve(), a.build_dir.resolve()
sys.path.insert(0, str(source/'ComboAndroid/tools'))
import wallmaster_corpus as wc  # noqa: E402  (the plan's rules and the seed checks)


def execution_binding_tests():
    """A copied RC1 pull must never become RC2 merely by changing the supplied bundle or generator."""
    with tempfile.TemporaryDirectory(prefix='corpus-execution-') as tmp:
        root = Path(tmp)
        bundle, results = root/'bundle', root/'results'
        (bundle/'bin').mkdir(parents=True)
        row = results/'example'
        row.mkdir(parents=True)
        (bundle/'bin/comborando').write_bytes(b'RC1 native generator')
        (bundle/'bin/libgame.so').write_bytes(b'RC1 native library')
        release = {'sourceCommit': '1' * 40, 'apkSha256': 'a' * 64,
                   'comborandoSha256': wc.sha256((bundle/'bin/comborando').read_bytes()),
                   'libraries': {'libgame.so': wc.sha256((bundle/'bin/libgame.so').read_bytes())}}
        wc.write_json(bundle/'plan.json', {'release': release})
        (bundle/'bundle.sha256').write_text('example bundle inventory\n')
        wc.write_json(row/'result.json', {'worker': '0'})
        (row/'seed.json').write_bytes(b'RC1 generated seed')
        receipt = {'schema': wc.EXECUTION_SCHEMA, 'kind': 'device-start', 'worker': '0',
                   'bootId': '11111111-1111-1111-1111-111111111111', 'hashes': wc.execution_hashes(bundle)}
        wc.write_json(row/'execution-receipt.json', receipt)

        def bind():
            wc.write_text(row/'execution-binding.sha256', ''.join(
                f'{wc.sha256(p.read_bytes())}  {p.name}\n' for p in sorted(row.iterdir())
                if p.name != 'execution-binding.sha256'))

        def rejects(label):
            try:
                wc.verify_execution_bindings(bundle, results)
            except SystemExit:
                return
            raise AssertionError('execution binding accepted ' + label)

        bind()
        records = wc.verify_execution_bindings(bundle, results)
        wc.check_execution_records(records, wc.sha256((bundle/'plan.json').read_bytes()), release, ['example'])
        wc.check_execution_artifacts(records, {'example': {'seed.json': wc.sha256(b'RC1 generated seed')}})
        try:
            wc.check_execution_artifacts(records, {'example': {'seed.json': wc.sha256(b'changed fixture seed')}})
        except SystemExit:
            pass
        else:
            raise AssertionError('persisted execution binding accepted different fixture bytes')
        original_plan = (bundle/'plan.json').read_bytes()
        release['sourceCommit'] = '2' * 40
        wc.write_json(bundle/'plan.json', {'release': release})
        rejects('RC1 results with a different RC plan')
        (bundle/'plan.json').write_bytes(original_plan)
        (bundle/'bin/comborando').write_bytes(b'RC2 native generator')
        rejects('a different generator')
        changed_release = json.loads(original_plan)['release']
        changed_release['comborandoSha256'] = wc.sha256((bundle/'bin/comborando').read_bytes())
        wc.write_json(bundle/'plan.json', {'release': changed_release})
        rejects('RC1 results paired with a valid different generator and its own plan')
        (bundle/'plan.json').write_bytes(original_plan)
        (bundle/'bin/comborando').write_bytes(b'RC1 native generator')
        binding = (row/'execution-binding.sha256').read_bytes()
        (row/'execution-binding.sha256').unlink()
        rejects('a missing row binding')
        (row/'execution-binding.sha256').write_bytes(binding)
        (row/'seed.json').write_bytes(b'other run seed')
        rejects('a forged row artifact')
        (row/'seed.json').write_bytes(b'RC1 generated seed')
        receipt['kind'] = 'legacy-attestation'
        receipt['legacyResultsSha256'] = 'f' * 64
        wc.write_json(row/'execution-receipt.json', receipt)
        bind()
        rejects('an arbitrary legacy attestation')
        # Exercise the actual default planner with tiny stand-in native bytes: it must stage a runnable bound queue
        # and collector, while the test need not construct an APK or invoke an Android build.
        real_stage = wc.stage_binaries

        def tiny_stage(apk, generator, destination):
            destination.mkdir(parents=True)
            (destination/'comborando').write_bytes(b'RC1 native generator')
            (destination/'libgame.so').write_bytes(b'RC1 native library')
            return release['libraries'], wc.sha256(b'RC1 native generator'), release['apkSha256']

        wc.stage_binaries = tiny_stage
        try:
            args = SimpleNamespace(out=root/'default-plan', source_commit='3' * 40,
                                   corpus_0412=source/wc.FIXTURES, corpus_0413=source/wc.FIXTURES_0413,
                                   apk=root/'unused.apk', comborando=root/'unused-generator',
                                   version_name='0.4.14', version_code=72, legacy_unbound_replan=False)
            with contextlib.redirect_stdout(io.StringIO()):
                wc.cmd_plan_0414(args)
            default = args.out
            require_bound = lambda name, text: text in (default/name).read_text(encoding='utf-8')
            assert wc.load(default/'plan.json')['executionBinding'] == wc.EXECUTION_SCHEMA
            assert require_bound('run-queue.sh', 'execution-$WORKER.json')
            assert require_bound('run-queue.sh', 'MODE=full')
            assert require_bound('collect.sh', 'execution-binding.sha256')
            assert require_bound('collect.sh', 'execution-receipt.json')
            wc.execution_hashes(default)
        finally:
            wc.stage_binaries = real_stage
    print('PASS execution binding: a different RC plan/generator, missing binding, forged row and arbitrary legacy '
          'attestation are refused; persisted bindings match fixture bytes; default plans stage bound scripts')


execution_binding_tests()
if a.execution_only:
    sys.exit(0)

fixtures = (a.fixtures or source/wc.FIXTURES_0414).resolve()
corpus_0412, corpus_0413 = source/wc.FIXTURES, source/wc.FIXTURES_0413


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


# The production rules the plan's model and the checks below rely on.
capabilities = read('combo/rando/entrances/Capabilities.h')
require(re.search(r'kWallmastersFullValues\[\] = \{ \{ "erWallmasters", "full", Stage::Public \} \};', capabilities) and
        re.search(r'\{\s*"' + re.escape(wc.FULL_ID) + r'",\s*kWallmastersFullValues,\s*\{\},\s*false\s*\}', capabilities),
        f'{wc.FULL_ID} is no shipping capability row admitting erWallmasters=full (Capabilities.h)')
bridge = read('combo/rando/entrances/UnifiedBridge.h')
require(hostcxx.contains(body(bridge, 'inline std::string UnifiedRequirement(const Settings& s, std::string_view '
                                      'capability,'),
                         'if (s.erWallmasters == Mode::Full) return std::string(kFullWallmasterCapability);'),
        'a Full seed no longer takes the Full id whatever its song facts and quest context (UnifiedBridge.h)')
require(hostcxx.contains(bridge, 'const auto groups = pool.ownGame ? std::vector<std::optional<Game>>{ Game::Oot, '
                                 'Game::Mm } : std::vector<std::optional<Game>>{ std::nullopt };'),
        'a cross-game pool no longer shares its destinations in the capacity count (UnifiedBridge.h)')
spoiler = read('combo/rando/entrances/EntranceSpoiler.h')
require(hostcxx.contains(spoiler, 'for (const auto* routes : { &layout->oot, &layout->mm, &layout->ootToMm, '
                                  '&layout->mmToOot })') and
        hostcxx.contains(spoiler, 'auto key = std::string(source.from) + " to " + std::string(source.to) + " (" + '
                                  'std::string(source.name) + ")";') and
        hostcxx.contains(spoiler, 'auto value = std::string(destination.to) + " from " + std::string(destination.from) '
                                  '+ " (" + std::string(destination.name) + ")";') and
        hostcxx.contains(spoiler, 'text += "  " + key + std::string(width - key.size(), \' \') + " -> " + value + '
                                  '"\\n";'),
        'the entrance spoiler writer is no longer the one this test models (EntranceSpoiler.h)')
headless = read('combo/ComboRandoHeadless.cpp')
for needle in ('ComboRando::StampOotmmSceneRestrictions(consolidated["oot"]);',
               'if (!entranceLayout.is_null()) consolidated["displaySeed"] = ',
               'masterSeed = base + attempt * 0x9E3779B9u;'):
    require(hostcxx.contains(headless, needle), 'headless rule moved: ' + needle)
attempts = int(re.search(r'constexpr int kFillAttempts = (\d+);', read('combo/rando/CrossWorldRando.h')).group(1))
require(attempts == wc.FILL_ATTEMPTS, 'the whole-fill attempt count changed (CrossWorldRando.h)')
layout_rows, game_links = wc.layout_rows(read(wc.LAYOUT_SOURCE))
contract_facts = wc.full_contract(json.loads(read(wc.CONTRACT_SOURCE)))
destinations, mm_sources, _, _ = contract_facts
require(len(destinations) == 18 and sum(d.startswith('OOT_') for d in destinations) == 11 and len(mm_sources) == 4,
        'the Full pool is not 18 shared destinations (11 OoT, 7 MM) for both games\' Wallmasters (m1-contract.json)')

# The launcher functions entrance_corpus_0414_tests.cpp runs, copied unchanged.
launcher = read('combo/ComboShip.cpp')
work.mkdir(parents=True, exist_ok=True)
lines = ['// Generated by entrance_corpus_0414_tests.py from combo/ComboShip.cpp; do not edit.']
for signature in ('static bool InstallEntranceSeed(const nlohmann::json& seed) {',
                  'static bool CheckPendingEntranceSeed(const nlohmann::json& seed) {'):
    lines.append(body(launcher, signature))
(work/'corpus_0414_launcher.inc').write_text('\n'.join(lines) + '\n', encoding='utf-8', newline='\n')
checkers = [hostcxx.build(a.compiler, source/'ComboAndroid/tests/entrance_corpus_0414_tests.cpp',
                          includes=[source/'combo', a.json_include, work],
                          defines=(['COMBO_ENTRANCE_DEV=1'] if dev else []) + ['_CRT_SECURE_NO_WARNINGS'],
                          msvc=['/bigobj'], exe=work/f'corpus_0414_{dev}.exe') for dev in (0, 1)]

present = sorted(path for path in fixtures.rglob('*') if path.is_file()) if fixtures.is_dir() else []
if not present:
    print(f'SKIPPED: no 0.4.14 Full Wallmaster corpus yet: {wc.FIXTURES_0414}/ is absent or empty. It is made from the '
          '0.4.14 SHIPPING release candidate on an emulator with ComboAndroid/tools/wallmaster_corpus.py plan-0414 and '
          'manifest-0414.')
    sys.exit(0)

# The manifest and every file it lists.
manifest_path = fixtures/'manifest.json'
require(manifest_path.is_file(), f'{fixtures} holds fixtures but no manifest.json')
manifest = json.loads(manifest_path.read_text(encoding='utf-8'))
require(manifest.get('schema') == wc.MANIFEST_SCHEMA_0414, f'manifest.json is not a {wc.MANIFEST_SCHEMA_0414} manifest')
require(manifest.get('capabilities') == [wc.FULL_ID], f'the manifest is not for {wc.FULL_ID}')
release = manifest.get('release') or {}
require(release.get('versionName') == '0.4.14' and release.get('build') == 'SHIPPING' and
        re.fullmatch(r'[0-9a-f]{40}', str(release.get('sourceCommit'))) and
        all(re.fullmatch(r'[0-9a-f]{64}', str(release.get(key))) for key in ('apkSha256', 'comborandoSha256')),
        'the manifest does not record the 0.4.14 SHIPPING release candidate')
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


def listed_bytes(path):
    require(path in listed, f'{path!r} is not a listed fixture')
    return (fixtures/path).read_bytes()


plan = json.loads(listed_bytes(manifest.get('plan')))
require(plan.get('schema') == wc.PLAN_SCHEMA_0414 and plan.get('release') == release and
        plan.get('capability') == wc.FULL_ID, 'the plan is not this corpus\'s')
require(all(plan['timeouts'].get(key) == wc.LIMIT_0414 for key in ('generation', 'playthrough', 'reproduction')),
        'the plan\'s limits are not the tool\'s')
wc.check_execution_records(manifest.get('executionBindings'), wc.sha256(listed_bytes(manifest['plan'])), release,
                           [row['id'] for row in plan['rows'] + plan['regression']], fail=fail)
evidence = manifest.get('executionEvidenceFiles') or []
require([entry.get('row') for entry in evidence] == [entry['row'] for entry in manifest['executionBindings']],
        'execution evidence files do not cover the recorded rows')
for record, entry in zip(manifest['executionBindings'], evidence):
    require(json.loads(listed_bytes(entry.get('path'))) == record,
            'a listed execution evidence file differs from the manifest record')
execution_artifacts = {
    entry['id']: {'seed.json': wc.sha256(listed_bytes(entry['seed'])),
                  'seed.entrances.txt': wc.sha256(listed_bytes(entry['entrances'])),
                  'repro.json': wc.sha256(listed_bytes(entry['seed'])),
                  'repro.entrances.txt': wc.sha256(listed_bytes(entry['entrances']))}
    for entry in manifest['seeds']}
execution_artifacts.update({entry['id']: {'seed.json': entry['seedSha256'],
                                        'seed.entrances.txt': entry['entrancesSha256']}
                            for entry in manifest['regression']})
wc.check_execution_artifacts(manifest['executionBindings'], execution_artifacts, fail=fail)
planned = {row['id']: row for row in plan['rows']}
require(len(planned) == len(plan['rows']) and all(row['kind'] == 'full' for row in plan['rows']),
        'repeated or non-Full plan rows')
# The plan is the tool's: the same rows, settings and master seeds.
config_bytes = {}
expected_rows = [wc.plan_row_0414(*spec, config_bytes) for spec in wc.rows_0414()]
require(expected_rows == plan['rows'], 'the plan\'s rows are not wallmaster_corpus.py rows_0414\'s')
for name, data in config_bytes.items():
    require(listed_bytes('inputs/' + name) == data, f'inputs/{name} is not the planned settings file')
entries = {}
for kind in ('seeds', 'failures'):
    for entry in manifest.get(kind) or []:
        rid = entry.get('id')
        require(rid in planned and rid not in entries, f'{kind}: {rid!r} is no planned row, or is listed twice')
        row = planned[rid]
        require(entry.get('request') == {key: row[key] for key in ('label', 'masterSeed', 'startingGame',
                                                                    'ootStartingAge', 'accessibility', 'souls')} and
                entry.get('config') == f'inputs/{row["config"]}' and entry['config'] in listed and
                entry.get('group') == row['group'] and entry.get('kind') == 'full',
                f'{rid}: the recorded request is not the planned row')
        entries[rid] = (kind, entry)
require(set(entries) == set(planned), f'planned rows the manifest does not account for: '
                                      f'{sorted(set(planned) - set(entries))}')

seen = {key: set() for key in ('group', 'request', 'start', 'age', 'accessibility', 'souls', 'ootToMm', 'mmToOot',
                               'mq', 'forced', 'rolled', 'songs', 'warp', 'crossAge', 'choices')}
for rid, (kind, entry) in sorted(entries.items()):
    row, where = planned[rid], f'{rid}: '
    if kind == 'failures':
        require(wc.one_line(entry.get('note')) and entry.get('problems') and 'seed' not in entry,
                where + 'a recorded failure needs its note and problems, and has no seed fixture')
        print(f'RECORDED FAILURE {rid}: {entry["note"]}')
        continue
    seed, entrances = listed_bytes(entry.get('seed')), listed_bytes(entry.get('entrances'))
    problems, facts = wc.full_seed_facts(row, seed, entrances, contract_facts, layout_rows, game_links)
    require(not problems, where + '; '.join(map(str, problems)))
    for key in ('result', 'facts', 'identity', 'settingsRecordedOtherwise'):
        require(entry.get(key) == facts.get(key), where + f'the recorded {key} is not the seed\'s')
    playthrough = (0, 1) if row['accessibility'] == 'noLogic' else (0,)
    require(entry['native'].get('generationExit') == 0 and entry['native'].get('repro') == 'identical',
            where + f'the native run {entry["native"]} failed')
    if entry['native'].get('playthroughExit') not in playthrough:
        replay = entry.get('replayFailure') or {}
        require(replay.get('playthroughExit') == entry['native'].get('playthroughExit') and
                wc.one_line(entry.get('recordedFailure')), where + 'a failed replay needs its recorded verdict and note')
        print(f'RECORDED FAILURE {rid} (replay exit {replay["playthroughExit"]}): {entry["recordedFailure"]}')
    else:
        require('replayFailure' not in entry and 'recordedFailure' not in entry, where + 'a passing replay with a note')
    # The entrance spoiler lists each route between the games as OoTMM writes it.
    text = entrances.decode('utf-8').splitlines()
    for src, dst in entry['identity']['ootToMm'] + entry['identity']['mmToOot']:
        key, value = wc.spoiler_line(layout_rows, src, dst)
        require(sum(line.startswith('  ' + key + ' ') and line.endswith(' -> ' + value) for line in text) == 1,
                where + f'the entrance spoiler does not list {src} -> {dst} once')
    cvars, f = row['cvars'], entry['facts']
    seen['group'].add(row['group'])
    seen['request'].add(row['startingGame'])
    seen['start'].add(entry['result']['startingGame'])
    seen['age'].add(row['ootStartingAge'])
    seen['accessibility'].add(row['accessibility'])
    seen['souls'].add(row['souls'])
    seen['ootToMm'].add(bool(entry['identity']['ootToMm']))
    seen['mmToOot'].add(bool(entry['identity']['mmToOot']))
    seen['mq'].add(bool(f['masterQuestDungeons']))
    mask = sum(1 << wc.MQ_INDEX[name] for name in f['masterQuestDungeons'])
    if wc.capacity_shortfall(mask):
        seen['forced' if wc.shortfall_forced(cvars) else 'rolled'].add(rid)
        require(f['ownGameRefusal'] == (wc.FORCED_SENTENCE if wc.shortfall_forced(cvars) else wc.SEED_SENTENCE),
                where + 'the Own Game sentence these settings would stop with is not recorded')
    seen['songs'].add(bool(cvars.get('gCombo.Rando.ForeignSongs', 0)) and wc.logic_on(cvars))
    seen['warp'].add(bool(wc.starting_warp_songs(cvars)))
    seen['crossAge'].add(f['crossAge'])
    seen['choices'].add(f['resolvedChoices'] is not None)
for key, needed in (('group', set('abcde')), ('request', {'oot', 'mm', 'random'}), ('start', {'OOT', 'MM'}),
                    ('age', {0, 1, 2}), ('accessibility', {'everyCheck', 'completion', 'noLogic'}), ('souls', {0, 1}),
                    ('ootToMm', {True}), ('mmToOot', {True}), ('mq', {True, False}), ('songs', {True, False}),
                    ('warp', {True, False}), ('crossAge', {True, False}), ('choices', {True, False})):
    require(needed <= seen[key], f'partial corpus: no {key} {sorted(map(str, needed - seen[key]))}')
require(len(seen['forced']) >= 2 and seen['rolled'],
        'partial corpus: Spirit Temple and Bottom of the Well Master Quest with Shadow Temple and Ganon\'s Castle '
        f'vanilla needs two forced seeds and a rolled one under Full (forced {sorted(seen["forced"])}, rolled '
        f'{sorted(seen["rolled"])})')

# The regenerated 0.4.12 corpus rows: each is the 0.4.13 corpus's regeneration of it byte for byte, and so differs from
# its 0.4.12 fixture by the regressionAdditions alone.
regression = manifest.get('regression') or []
require([item.get('id') for item in regression] == [row['id'] for row in plan['regression']] and
        [row['corpusSeed'] for row in plan['regression']] == list(wc.VANILLA_REGRESSION_0414),
        'the regression rows are not the plan\'s')
corpus13 = {s['id']: s for s in json.loads((corpus_0413/'manifest.json').read_text(encoding='utf-8'))['seeds']}
vanilla = {s['id']: s for s in json.loads((corpus_0412/'manifest.json').read_text(encoding='utf-8'))['seeds']}
for item, row in zip(regression, plan['regression']):
    where = item['id'] + ': '
    same, old = corpus13[row['expect']['sameAs']['seed']], vanilla[row['corpusSeed']]
    require(listed_bytes('inputs/' + row['config']) == (corpus_0412/old['config']).read_bytes(),
            where + 'the saved regression input is not the original staged config')
    same_bytes, same_entrances = (corpus_0413/same['seed']).read_bytes(), (corpus_0413/same['entrances']).read_bytes()
    require(item.get('seedSha256') == hashlib.sha256(same_bytes).hexdigest() == row['expect']['sameAs']['seedSha256']
            and item.get('entrancesSha256') == hashlib.sha256(same_entrances).hexdigest(),
            where + f'not byte-identical to {wc.FIXTURES_0413} {same["id"]}')
    diffs = wc.json_diff(json.loads((corpus_0412/old['seed']).read_bytes()), json.loads(same_bytes))
    require(item.get('differencesFromCorpusFixture') == [{'path': '.'.join(map(str, p)), 'kind': k, 'value': v}
                                                         for p, k, _, v in diffs] and
            all(k == 'added' and wc.REGRESSION_ADDITIONS.get(tuple(p)) == v for p, k, _, v in diffs) and
            {tuple(p) for p, _, _, _ in diffs} == set(wc.REGRESSION_ADDITIONS) and
            same_entrances == (corpus_0412/old['entrances']).read_bytes(),
            where + 'differs from its 0.4.12 fixture beyond the regressionAdditions')
    print(f'REGRESSION {item["id"]}: the 0.4.14 generator made {wc.FIXTURES_0413} {same["id"]} byte for byte; '
          f'{wc.FIXTURES} {old["id"]} differs from it by the {len(diffs)} regressionAdditions only')
require(not manifest.get('laterReleases'), 'a later release candidate of this corpus needs its own record and checks')

# Provenance: the commit that made the manifest differs from the release candidate only in this directory and in the
# host-only files the manifest lists (wallmaster_corpus.py provenance-0414 prints the same check for the record).
git = shutil.which('git')
made = subprocess.run([git, '-C', str(source), 'log', '-1', '--format=%H', '--', f'{wc.FIXTURES_0414}/manifest.json'],
                      capture_output=True, text=True) if git else None
if made is not None and made.returncode == 0 and re.fullmatch(r'[0-9a-f]{40}', made.stdout.strip()):
    commit = made.stdout.strip()
    diff = subprocess.run([git, '-C', str(source), 'diff', '--name-only', release['sourceCommit'], commit],
                          capture_output=True, text=True)
    require(diff.returncode == 0, f'cannot diff {commit[:12]} against the release candidate')

    def show(revision, path):
        run = subprocess.run([git, '-C', str(source), 'show', f'{commit if revision == "fixture" else revision}:{path}'],
                             capture_output=True, text=True, encoding='utf-8')
        return run.stdout if run.returncode == 0 else None
    problems, reasons = wc.provenance_problems_0414(manifest, release['sourceCommit'], diff.stdout.splitlines(), show)
    require(not problems, '; '.join(problems))
    print(f'Provenance: {commit[:12]} differs from {release["sourceCommit"][:12]} in the fixtures and {len(reasons)} '
          f'host-only files.')
else:
    print('NOTE: no commit of this checkout holds the manifest yet (or no git); wallmaster_corpus.py provenance-0414 '
          'checks the fixture commit.')

for dev, checker in enumerate(checkers):
    if subprocess.run([str(checker), str(manifest_path)], timeout=hostcxx.RUN_TIMEOUT).returncode:
        sys.exit(1)
counts = {kind: len(manifest.get(kind) or []) for kind in ('seeds', 'failures')}
print(f'PASS {counts["seeds"]} Full Wallmaster seeds from {release["versionName"]} ({release["sourceCommit"][:12]}), '
      f'{counts["failures"]} recorded failures, {len(regression)} regenerated 0.4.12 rows: manifest hashes, requests, '
      'start rule, wallmasters-full-v1 layouts, routes between the games and their entrance spoiler lines, random '
      'choices, coverage, regression and provenance')
