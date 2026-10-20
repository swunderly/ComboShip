"""C6 release corpus and File 3 save fixtures (M1 plan A-12 host half and A-13, P1-06).

Reads ComboAndroid/tests/fixtures/wallmasters-vanilla-v1/ (or --fixtures) through its manifest.json, which lists the
size and SHA-256 of every other file there. The seeds come from the 0.4.12 SHIPPING headless generator on an
emulator, and the saves are real File 3 saves the app made from corpus seeds; ComboAndroid/tools/wallmaster_corpus.py
plans the run and writes the manifest. Here each seed's recorded request and identity must match its document: its
label, master seed and attempt, the start the headless rule gives, the age, accessibility and enemy-soul settings,
its item placements and routes. The set must cover every start request, both resolved starts, all three OoT ages,
all three accessibility modes and souls on and off, and hold at least one save. entrance_corpus_tests.cpp then
checks the documents themselves, built with and without COMBO_ENTRANCE_DEV, against production launcher functions
copied unchanged from combo/ComboShip.cpp.

A later release candidate may use the corpus only as the manifest's laterReleases records it (wallmaster_corpus.py
later-rc and sample): every file it changes outside the fixtures is allowlisted with a one-line reason, and a sample of
rows its own comborando regenerated matches byte for byte. Without the fixture directory (or with it empty) the test
still builds the checker, then reports SKIPPED. A manifest that is empty or partial, or that does not match its files,
fails, and so does any fixture these checks refuse. Every seed is still checked while the File 3 save or a later
release candidate's sample is missing; the test then lists each as PENDING and fails.
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
p.add_argument('--fixtures', type=Path, help='Fixture directory (default: the checkout\'s wallmasters-vanilla-v1)')
a = p.parse_args()
source, work = a.source.resolve(), a.build_dir.resolve()
fixtures = (a.fixtures or source/'ComboAndroid/tests/fixtures/wallmasters-vanilla-v1').resolve()
SCHEMA = 'comboship-wallmasters-vanilla-fixtures-v1'
CAPABILITY = 'wallmasters-own-game-vanilla-v1'
GOLDEN = 0x9E3779B9  # the headless whole-fill retry step


def read(path):
    return (source/path).read_text(encoding='utf-8')


def fail(message):
    print('FAIL: ' + message)
    sys.exit(1)


def require(ok, message):
    if not ok:
        fail(message)


def body(text, signature):
    """The braced body of the production function that starts at signature."""
    start = hostcxx.find(text, signature)
    opening = text.index('{', start)
    masked = re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"', lambda m: ' ' * len(m.group()), text)
    depth = 0
    for end in range(opening, len(text)):
        depth += (masked[end] == '{') - (masked[end] == '}')
        if depth == 0:
            return text[start:end + 1]
    raise AssertionError('Unterminated production block ' + signature)


def combo_hash(text):
    """ComboHash: FNV-1a 32-bit over the UTF-8 bytes."""
    value = 2166136261
    for byte in text.encode('utf-8'):
        value = ((value ^ byte) * 16777619) & 0xffffffff
    return value


def resolves_mm(master):
    """ResolveStartingGameMM for a Random start request."""
    return combo_hash(f'startingGame:{master}') & 1 == 1


def canonical_sha256(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(',', ':'), ensure_ascii=False)
                          .encode('utf-8')).hexdigest()


# The production rules the checks below and in entrance_corpus_tests.cpp rely on.
headless = read('combo/ComboRandoHeadless.cpp')
for needle in ('masterSeed = base + attempt * 0x9E3779B9u;',
               'const bool mmStart = !pinStartOot && !(startCfg == 2 && attempt + 1 == kFillAttempts) && '
               'ResolveStartingGameMM(startCfg, masterSeed);',
               'return (ComboHash("startingGame:" + std::to_string(masterSeed)) & 1u) != 0;',
               'consolidated["startingGame"] = resolvedMmStart ? "MM" : "OOT";',
               # A corpus seed's File 3 icons and co-op identity: its own displaySeed, as an app seed's.
               'if (!entranceLayout.is_null()) consolidated["displaySeed"] = '
               'ComboHash(ComboRando::Entrances::GeneratedSeedIdentityText(',
               'f << consolidated.dump(2);'):
    require(hostcxx.contains(headless, needle), 'headless rule moved: ' + needle)
attempts = int(re.search(r'constexpr int kFillAttempts = (\d+);', read('combo/rando/CrossWorldRando.h')).group(1))
launcher = read('combo/ComboShip.cpp')
# An imported seed is kept as read and baked into the new file; its bind installs the saved layout.
reload_ = body(launcher, 'static int Combo_OnReloadRequestImpl(const char* path, '
                        'const nlohmann::json* profileSeed = nullptr) {')
require(hostcxx.contains(reload_, 'g_ConsolidatedJson = j.dump(2);'), 'the import no longer keeps the seed as read')
creation = body(launcher, 'static void Combo_OnOOTSaveInit(int fileNum) {')
require(hostcxx.contains(creation, 'seed = nlohmann::json::parse(g_ConsolidatedJson);') and
        hostcxx.contains(creation, 'c["combo"]["rando"] = seed;'), 'a new file no longer holds the imported seed')
bind = body(launcher, 'static void Combo_OnOOTSaveLoad(int fileNum) {')
require(hostcxx.contains(bind, 'const bool entrancesInstalled = InstallEntranceSeed(seedForEntrances);'),
        'the slot bind no longer installs the saved layout through InstallEntranceSeed')
require(hostcxx.contains(body(launcher, 'static void FlushContainer(int fileNum) {'), 'out << it->second.dump();'),
        'the container writer changed')
# SaveManager::LoadFile's preflight, which entrance_corpus_tests.cpp runs in this order on the launcher's page.
load = body(read('soh/soh/SaveManager.cpp'), 'bool SaveManager::LoadFile(int fileNum, bool initializeFullContext) {')
order = [hostcxx.find(load, needle) for needle in (
    'ComboRando::CheckComboSeedPreflight(parsedSave);', 'ComboRando::ReadKokiriSaveRevision(parsedSave);',
    'SOH_SetEntranceRuntimeFlags(ComboRando::TakeComboEntranceRuntimeFlags(parsedSave));', 'SaveContext_Init();')]
require(order == sorted(order), 'the native save preflight order changed')
# The games' installers that entrance_corpus_tests.cpp models.
require(hostcxx.contains(read('mm/2s2h/Rando/Logic/ComboEntrancesMM.cpp'),
                         'CE::ActiveMmUnified().install(seed, CE::ActiveCapabilities());'), 'the MM installer changed')
overlay = body(read('soh/soh/Enhancements/randomizer/combo_entrance_overlay.cpp'),
               'int SetLayout(const char* seedJson) {')
for needle in ('return 1; // Private MM profiles and ordinary seeds leave OoT native.',
               'CE::ActiveOotUnified().install(seed, CE::ActiveCapabilities());',
               'CE::RequireVanillaOotQuests(quests);'):
    require(hostcxx.contains(overlay, needle), 'the OoT installer changed: ' + needle)

# The launcher functions entrance_corpus_tests.cpp runs, copied unchanged, with this checkout's release line.
work.mkdir(parents=True, exist_ok=True)
version = re.search(r'set\(COMBO_RELEASE_VERSION "([^"]+)"', read('CMakeLists.txt')).group(1)
lines = ['// Generated by entrance_corpus_tests.py from combo/ComboShip.cpp and CMakeLists.txt; do not edit.',
         f'#define COMBO_RELEASE_VERSION "{version}"']
for signature in ('static bool ComboIsValidSlot(int fileNum) {',
                  'static std::filesystem::path ComboContainerPath(int fileNum) {',
                  'static std::string ComboReleaseMajorMinor(const std::string& v) {',
                  'static nlohmann::json& LoadOrCreateContainer(int fileNum) {',
                  'static const char* Combo_ReadGameSave(int game, int fileNum) {',
                  'static bool InstallEntranceSeed(const nlohmann::json& seed) {',
                  'static bool CheckPendingEntranceSeed(const nlohmann::json& seed) {'):
    lines.append(body(launcher, signature))
(work/'corpus_launcher.inc').write_text('\n'.join(lines) + '\n', encoding='utf-8', newline='\n')
# Built before the fixtures are looked at, so the harness stays compiled while there are none.
checkers = [hostcxx.build(a.compiler, source/'ComboAndroid/tests/entrance_corpus_tests.cpp',
                          includes=[source/'combo', a.json_include, work],
                          defines=(['COMBO_ENTRANCE_DEV=1'] if dev else []) + ['_CRT_SECURE_NO_WARNINGS'],
                          msvc=['/bigobj'], exe=work/f'corpus_{dev}.exe') for dev in (0, 1)]

present = sorted(path for path in fixtures.rglob('*') if path.is_file()) if fixtures.is_dir() else []
if not present:
    shown = fixtures.relative_to(source).as_posix() if fixtures.is_relative_to(source) else str(fixtures)
    print(f'SKIPPED: no Wallmaster release corpus yet: {shown}/ is absent or empty. It is made from the 0.4.12 '
          'SHIPPING release candidate on an emulator with ComboAndroid/tools/wallmaster_corpus.py, with a File 3 '
          'save made from one of its seeds.')
    sys.exit(0)

# The manifest and every file it lists.
manifest_path = fixtures/'manifest.json'
require(manifest_path.is_file(), f'{fixtures} holds fixtures but no manifest.json')
try:
    manifest = json.loads(manifest_path.read_text(encoding='utf-8'))
except ValueError as error:
    fail(f'manifest.json does not parse: {error}')
require(isinstance(manifest, dict) and manifest.get('schema') == SCHEMA, f'manifest.json is not a {SCHEMA} manifest')
require(manifest.get('capability') == CAPABILITY, f'the manifest is not for {CAPABILITY}')
release = manifest.get('release')
require(isinstance(release, dict) and all(isinstance(release.get(key), str) for key in ('versionName', 'sourceCommit'))
        and re.fullmatch(r'[0-9a-f]{40}', release['sourceCommit'])
        and all(re.fullmatch(r'[0-9a-f]{64}', str(release.get(key))) for key in ('apkSha256', 'comborandoSha256')),
        'the manifest does not record the release candidate (versionName, sourceCommit, apkSha256, comborandoSha256)')
files = manifest.get('files')
require(isinstance(files, list), 'the manifest has no file list')
listed = {}
for entry in files:
    path = entry.get('path') if isinstance(entry, dict) else None
    pure = PurePosixPath(path) if isinstance(path, str) else None
    require(pure is not None and not pure.is_absolute() and '..' not in pure.parts and path != 'manifest.json'
            and path not in listed, f'bad or repeated manifest path {path!r}')
    listed[path] = entry
actual = {path.relative_to(fixtures).as_posix() for path in present} - {'manifest.json'}
require(not set(listed) - actual, f'manifest files missing from the fixtures: {sorted(set(listed) - actual)}')
require(not actual - set(listed), f'fixture files the manifest does not list: {sorted(actual - set(listed))}')
for path, entry in sorted(listed.items()):
    data = (fixtures/path).read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    require(entry.get('bytes') == len(data) and entry.get('sha256') == digest,
            f'{path}: {len(data)} bytes with SHA-256 {digest}, but the manifest records {entry.get("bytes")} bytes '
            f'with SHA-256 {entry.get("sha256")}')


def listed_json(path, what):
    require(path in listed, f'{what} {path!r} is not a listed fixture')
    try:
        return json.loads((fixtures/path).read_text(encoding='utf-8'))
    except ValueError as error:
        fail(f'{path} does not parse: {error}')


plan = listed_json(manifest.get('plan'), 'the plan')
require(isinstance(plan, dict) and isinstance(plan.get('rows'), list), 'the plan has no rows')
planned = {row['id']: row for row in plan['rows']}
search = plan.get('search') or {}

# Each seed's recorded request, result and identity against its own document.
seeds = manifest.get('seeds')
require(isinstance(seeds, list) and seeds, 'the manifest lists no corpus seed')
documents, seen = {}, {}
for row in seeds:
    sid = row.get('id')
    require(isinstance(sid, str) and sid not in documents, f'bad or repeated seed id {sid!r}')
    doc = listed_json(row.get('seed'), f'{sid}: seed')
    require(row.get('entrances') in listed, f'{sid}: its entrance spoiler is not a listed fixture')
    require(row.get('config') in listed, f'{sid}: its settings config is not a listed fixture')
    request, result, native, identity = (row.get(key) or {} for key in ('request', 'result', 'native', 'identity'))
    where = f'{sid}: '
    # The run the plan recorded, or one found by its bounded search for the Random start fallback.
    if sid in planned:
        recipe = planned[sid]
        require(all(recipe.get(key) == request.get(key) for key in
                    ('label', 'masterSeed', 'startingGame', 'ootStartingAge', 'accessibility', 'souls')) and
                PurePosixPath(row['config']).name == recipe.get('config'), where + 'the request is not the planned row')
    else:
        require(search and request.get('startingGame') == 'random' and
                search['firstMaster'] <= request.get('masterSeed', -1) < search['firstMaster'] + search['count'] and
                PurePosixPath(row['config']).name == search.get('config'),
                where + 'neither a planned row nor a fallback-search row')
    require(doc.get('seed') == request.get('label'), where + 'the seed label differs from the request')
    final, base = doc.get('masterSeed'), request.get('masterSeed')
    require(isinstance(final, int) and isinstance(base, int), where + 'no master seed')
    attempt = ((final - base) * pow(GOLDEN, -1, 1 << 32)) % (1 << 32)
    require(attempt < attempts, where + f'master seed {final} is no fill attempt of {base}')
    # The headless start: Random falls back to OoT after an MM attempt failed, and on its last attempt.
    start = request.get('startingGame')
    require(start in ('oot', 'mm', 'random'), where + f'unknown start request {start!r}')
    fallback = start == 'random' and (attempt + 1 == attempts or
                                      any(resolves_mm((base + j * GOLDEN) % (1 << 32)) for j in range(attempt)))
    expected = {'oot': 'OOT', 'mm': 'MM'}.get(start) or ('OOT' if fallback or not resolves_mm(final) else 'MM')
    require(doc.get('startingGame') == expected,
            where + f'starts in {doc.get("startingGame")}, the headless rule gives {expected}')
    require(result == {'masterSeed': final, 'attempt': attempt, 'startingGame': expected, 'startFallback': fallback},
            where + f'the recorded result {result} differs from the seed')
    oot_settings = (doc.get('oot') or {}).get('settings') or {}
    mm_settings = (doc.get('mm') or {}).get('settings') or {}
    facts = {'ootStartingAge': oot_settings.get('gRandoSettings.StartingAge'),
             'accessibility': (doc.get('accessibility') or {}).get('mode'),
             'souls': oot_settings.get('gRandoSettings.SoulsEnemyOot')}
    require(all(request.get(key) == value for key, value in facts.items()) and
            mm_settings.get('gRando.Options.RO_SHUFFLE_ENEMY_SOULS') == facts['souls'],
            where + f'the seed has {facts} and MM souls {mm_settings.get("gRando.Options.RO_SHUFFLE_ENEMY_SOULS")}, '
            f'not the requested {request}')
    playthrough = (0, 1) if facts['accessibility'] == 'noLogic' else (0,)
    require(native.get('generationExit') == 0 and native.get('playthroughExit') in playthrough and
            native.get('repro') == 'identical', where + f'the native run {native} did not pass')
    placements = {'oot': (doc.get('oot') or {}).get('placements'), 'mm': (doc.get('mm') or {}).get('placements'),
                  'foreign': doc.get('foreign')}
    require(identity.get('placementsSha256') == canonical_sha256(placements), where + 'the item placements differ')
    routes = [[route.get('source'), route.get('destination')]
              for route in (doc.get('entranceLayout') or {}).get('overrides') or []]
    require(identity.get('routes') == routes, where + 'the recorded routes differ from the seed\'s layout')
    documents[sid] = doc
    for key, value in (('requested start', start), ('resolved start', expected),
                       ('accessibility', facts['accessibility']), ('enemy souls', facts['souls'])):
        seen.setdefault(key, set()).add(value)
    if expected == 'OOT':
        seen.setdefault('OoT start age', set()).add(facts['ootStartingAge'])
for key, needed in (('requested start', {'oot', 'mm', 'random'}), ('resolved start', {'OOT', 'MM'}),
                    ('OoT start age', {0, 1, 2}), ('accessibility', {'everyCheck', 'completion', 'noLogic'}),
                    ('enemy souls', {0, 1})):
    missing = sorted(needed - seen.get(key, set()), key=str)
    require(not missing, f'partial corpus: no seed with {key} {missing}')

# Later release candidates (wallmaster_corpus.py later-rc and sample). One may use this corpus, made by the release
# candidate above, only when each file it changes outside these fixtures is on its allowlist with a one-line reason
# saying why it is outside seed generation, and its own comborando regenerated the recorded sample rows byte for byte.
# Until that sample is recorded the release candidate is pending, and this test fails saying so.
pending = []  # (short, full) release items still to come
FIXTURE_PREFIX = 'ComboAndroid/tests/fixtures/wallmasters-vanilla-v1/'
later = manifest.get('laterReleases', [])
require(isinstance(later, list), 'laterReleases is not a list')
commits = {release['sourceCommit']}
by_id = {row['id']: row for row in seeds}
for record in later:
    commit = record.get('sourceCommit') if isinstance(record, dict) else None
    require(isinstance(commit, str) and re.fullmatch(r'[0-9a-f]{40}', commit) and commit not in commits,
            f'bad or repeated later release candidate {commit!r}')
    commits.add(commit)
    rc = commit[:12]
    require(isinstance(record.get('versionName'), str) and
            all(re.fullmatch(r'[0-9a-f]{64}', str(record.get(key))) for key in ('apkSha256', 'comborandoSha256')),
            f'{rc}: the record has no version, APK hash or comborando hash')
    allowed = set()
    for entry in record.get('outsideGeneration') or []:
        path, reason = (entry.get('path'), entry.get('reason')) if isinstance(entry, dict) else (None, None)
        require(isinstance(path, str) and path not in allowed and not path.startswith(FIXTURE_PREFIX) and
                isinstance(reason, str) and reason.strip() and '\n' not in reason and '\r' not in reason,
                f'{rc}: bad allowlist entry {entry!r}; each file needs a path and a one-line reason')
        allowed.add(path)
    git = shutil.which('git')
    diff = subprocess.run([git, '-C', str(source), 'diff', '--name-only', release['sourceCommit'], commit],
                          capture_output=True, text=True) if git else None
    if diff is not None and diff.returncode == 0:
        unlisted = sorted(path for path in diff.stdout.splitlines()
                          if not path.startswith(FIXTURE_PREFIX) and path not in allowed)
        require(not unlisted, f'{rc} changes files since {release["sourceCommit"][:12]} that its allowlist does not '
                              f'list: {unlisted}')
    else:
        print(f'NOTE: this checkout cannot diff {rc} against {release["sourceCommit"][:12]}; '
              'wallmaster_corpus.py provenance checks its allowlist.')
    sample = record.get('sample') or {}
    rows = sample.get('rows')
    require(isinstance(rows, list) and rows and len(set(rows)) == len(rows) and all(r in documents for r in rows),
            f'{rc}: its sample rows are not corpus seeds')
    if sample.get('status') == 'pending':
        pending.append((f'the {rc} regeneration sample',
                        f'later release candidate {record["versionName"]} {rc}: its comborando (SHA-256 '
                        f'{record["comborandoSha256"]}) has not regenerated the {len(rows)} sample rows yet; '
                        'wallmaster_corpus.py sample records them'))
        continue
    results = {r.get('id'): r for r in sample.get('results') or [] if isinstance(r, dict)}
    require(sample.get('status') == 'identical' and sample.get('comborandoSha256') == record['comborandoSha256'] and
            sorted(results) == sorted(rows), f'{rc}: the sample is neither pending nor complete for its comborando')
    for rid in rows:
        require(results[rid].get('seedSha256') == listed[by_id[rid]['seed']]['sha256'] and
                results[rid].get('entrancesSha256') == listed[by_id[rid]['entrances']]['sha256'],
                f'{rc}: the regenerated {rid} is not the corpus row byte for byte')

saves = manifest.get('saves')
require(isinstance(saves, list), 'the manifest has no save list')
if not saves:
    pending.append(('the File 3 save', 'the manifest lists no File 3 save (C6 needs a save bound to a corpus seed)'))
ids = set()
for row in saves:
    sid = row.get('id')
    require(isinstance(sid, str) and sid not in ids, f'bad or repeated save id {sid!r}')
    ids.add(sid)
    require(row.get('file') in listed and row.get('slot') == 2, f'{sid}: not a listed File 3 save')
    require(row.get('corpusSeed') in documents, f'{sid}: made from {row.get("corpusSeed")!r}, which is no corpus seed')
    require(isinstance(row.get('checkpoint'), str) and row['checkpoint'], f'{sid}: no recorded save point')

for dev, checker in enumerate(checkers):
    sandbox = work/f'corpus-work-{dev}'
    shutil.rmtree(sandbox, ignore_errors=True)
    sandbox.mkdir()
    if subprocess.run([str(checker), str(manifest_path), str(sandbox)], timeout=hostcxx.RUN_TIMEOUT).returncode:
        sys.exit(1)
for _, item in pending:
    print('PENDING: ' + item)
require(not pending, f'the {len(seeds)} corpus seeds pass, but these are pending: '
                     f'{", ".join(short for short, _ in pending)}')
print(f'PASS {len(seeds)} corpus seeds and {len(saves)} File 3 saves from {release["versionName"]} '
      f'({release["sourceCommit"][:12]}), {len(later)} later release candidates: manifest hashes, requests, start '
      'rule, coverage, identities and later samples')
