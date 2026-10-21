"""A Wallmaster seed with "Play warp songs across games" in logic is refused by older builds; this build's seeds
without it still load there.

This build writes Wallmaster Shuffle (Own Game) seeds with foreign-song logic, which require
wallmasters-own-game-songs-v1, once with all-vanilla dungeons and once with Master Quest dungeons, and two seeds
without that logic: an all-vanilla one (wallmasters-own-game-vanilla-v1) and a Master Quest one
(wallmasters-own-game-quests-v1). Two older seed guards are then compiled from this checkout's history and run on
them, with their shipping set and their whole catalog alike:
- 0.4.12 (combo/ at tag v0.4.12-android) refuses both songs seeds and the Master Quest one, which name a capability
  it does not know (EntranceLayout.h ValidateEntranceLayoutSettings), and loads the all-vanilla one;
- the quests catalogue (combo/ at the parent of the first commit that adds wallmasters-own-game-songs-v1 to
  Capabilities.h, found by content: the Master Quest Wallmaster build, whose combo/ is cda22efd's) refuses both songs
  seeds the same way and loads the other two.
Each older build is checked on its own. One that this checkout's history lacks is SKIPPED only in a shallow clone,
and the report says which builds ran; in a full clone the test fails and says what to fetch.
"""
from pathlib import Path
import argparse
import io
import json
import subprocess
import sys
import tarfile
import hostcxx

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--json-include', type=Path, required=True)
p.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
a = p.parse_args()
source, work = a.source.resolve(), a.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
SONGS_ID, CAPABILITIES = 'wallmasters-own-game-songs-v1', 'combo/rando/entrances/Capabilities.h'


def git(*command):
    return subprocess.run(['git', '-C', str(source), *command], capture_output=True, text=True, timeout=300)


def resolve(rev):
    found = git('rev-parse', '--verify', '--quiet', rev + '^{commit}')
    return found.stdout.strip() if found.returncode == 0 else None


def quests_base():
    """The parent of the first commit that adds the songs id to the capability catalogue, or None."""
    added = git('log', '--reverse', '--format=%H', '-S', SONGS_ID, '--', CAPABILITIES)
    first = added.stdout.split()[0] if added.returncode == 0 and added.stdout.split() else None
    return resolve(first + '^') if first else None


shallow = git('rev-parse', '--is-shallow-repository')
if shallow.returncode != 0:
    sys.exit(f'FAIL: {source} is not a git checkout, so the older seed guards cannot be read from its history')
shallow = shallow.stdout.strip() == 'true'
# (label, how to find it, what to fetch, whether that build knows the quests id)
OLDER = [('v0.4.12-android', lambda: resolve('v0.4.12-android'), "the fork's tags (git fetch --tags)", False),
         ('quests catalogue', quests_base, f'the history that adds {SONGS_ID} to {CAPABILITIES}', True)]
builds, skipped = [], []
for label, find, fetch, knows_quests in OLDER:
    commit = find()
    if commit:
        builds.append((label, commit, knows_quests))
    elif shallow:
        skipped.append(label)
    else:
        sys.exit(f'FAIL: this checkout is not shallow but has no {label} build; fetch {fetch} to run its seed guard')
lacking = ' and '.join(f'the {label} build' for label in skipped)
if not builds:
    print(f'SKIPPED: this shallow checkout lacks {lacking} (fetch the full history and the tags to run the older seed '
          'guards)')
    sys.exit(0)

# This build's seeds.
writer = work/'write_seeds.cpp'
writer.write_text(r'''
#include "rando/entrances/MmUnifiedLayout.h"
#include "rando/entrances/OotUnifiedLayout.h"
#include <cassert>
#include <fstream>
using namespace ComboRando;
using namespace ComboRando::Entrances;
int main(int argc, char** argv) {
    assert(argc == 5);
    const auto caps = CapabilitySet::AtLeast(Stage::Hidden);
    SongTravelMap songs;
    songs.foreignSongs = songs.logicForeignSongs = true;
    int out = 1;
    for (const bool foreign : {true, false})
        for (const bool mq : {false, true}) {
            EntranceLayout layout;
            layout.settings.erWallmasters = Mode::OwnGame;
            if (mq)
                layout.ootMq[OotMqIndex("Forest")] = layout.ootMq[OotMqIndex("Water")] = true;
            layout.required = {std::string(foreign ? kSongsWallmasterCapability
                                           : mq    ? kQuestsWallmasterCapability
                                                   : kVanillaWallmasterCapability)};
            Overrides routes;
            size_t oot = 0, mm = 0;
            for (const auto& pool : LayoutV1::MakePools(layout.settings, layout.ootMq, false))
                for (const auto src : pool.src)
                    routes[src] = Find(src)->game == Game::Oot ? kOotWallmasterArrivalRows[oot++]
                                                               : kMmBaseWallmasterDestinations[mm++];
            layout.overrides = routes;
            Validate(layout, caps);
            nlohmann::json seed{{"fileType", kEntranceSeedFileType}, {"version", kEntranceSeedVersion},
                                {"entranceLayout", EncodeEntranceLayout(layout)},
                                {"songTravel", foreign ? songs.json() : SongTravelMap{}.json()},
                                {"accessibility", {{"version", 1}, {"mode", "everyCheck"}}}};
            RequireSupportedSeedCapabilities(seed, caps);
            OotUnifiedLayout installed;
            installed.install(seed, caps);
            std::ofstream(argv[out++]) << seed.dump();
        }
}
''', encoding='utf-8')
exe = hostcxx.build(a.compiler, writer, includes=[source/'combo', a.json_include], defines=['_CRT_SECURE_NO_WARNINGS'],
                    msvc=['/bigobj'], exe=work/'write_seeds.exe')
seeds = [work/name for name in ('songs-vanilla.json', 'songs-mq.json', 'vanilla.json', 'quests.json')]
subprocess.run([str(exe), *map(str, seeds)], check=True, timeout=hostcxx.RUN_TIMEOUT)
required = [json.loads(path.read_text())['entranceLayout']['requires'] for path in seeds]
assert required == [['wallmasters-own-game-songs-v1']] * 2 + [['wallmasters-own-game-vanilla-v1'],
                                                             ['wallmasters-own-game-quests-v1']], required

# The older guards on them: argv[5] is 1 when that build knows the quests id.
reader = work/'guard.cpp'
reader.write_text(r'''
#include "rando/ComboSeedFormat.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
using namespace ComboRando;
using namespace ComboRando::Entrances;
int main(int argc, char** argv) {
    assert(argc == 6);
    nlohmann::json seeds[4];
    for (int i = 0; i < 4; ++i) {
        std::ifstream file(argv[i + 1]);
        seeds[i] = nlohmann::json::parse(file);
    }
    const bool knowsQuests = std::string(argv[5]) == "1";
    for (const auto& caps : {ActiveCapabilities(), CapabilitySet::AtLeast(Stage::Hidden)}) {
        const auto refused = [&](const nlohmann::json& seed, const char* id) {
            std::string why;
            try {
                RequireSupportedSeedCapabilities(seed, caps);
            } catch (const UnsupportedComboSeedFormat& error) {
                why = error.what();
            }
            assert(why == std::string("This seed cannot be used by this build: Entrance layout requires unknown "
                                      "capability ") + id);
        };
        refused(seeds[0], "wallmasters-own-game-songs-v1");
        refused(seeds[1], "wallmasters-own-game-songs-v1");
        RequireSupportedSeedCapabilities(seeds[2], caps);
        Validate(DecodeEntranceLayout(seeds[2].at("entranceLayout")), caps);
        if (knowsQuests)
            RequireSupportedSeedCapabilities(seeds[3], caps);
        else
            refused(seeds[3], "wallmasters-own-game-quests-v1");
    }
    std::cout << "refuses both foreign-song Wallmaster seeds and loads the others it knows\n";
}
''', encoding='utf-8')
for label, commit, knows_quests in builds:
    if label == 'v0.4.12-android':
        assert commit == '33c972f1c42428ff52da2e0a99e5884f57f9db03', commit
    rev = label.replace(' ', '-')
    old = work/rev
    archive = subprocess.run(['git', '-C', str(source), 'archive', '--format=tar', commit, 'combo'],
                             capture_output=True, check=True, timeout=300).stdout
    with tarfile.open(fileobj=io.BytesIO(archive)) as tar:
        tar.extractall(old, filter='data')
    catalog = (old/'combo/rando/entrances/Capabilities.h').read_text(encoding='utf-8')
    assert 'wallmasters-own-game-songs-v1' not in catalog
    assert ('wallmasters-own-game-quests-v1' in catalog) == knows_quests
    exe = hostcxx.build(a.compiler, reader, includes=[old/'combo', a.json_include],
                        defines=['_CRT_SECURE_NO_WARNINGS'], msvc=['/bigobj'], exe=work/f'guard_{rev}.exe')
    subprocess.run([str(exe), *map(str, seeds), '1' if knows_quests else '0'], check=True,
                   timeout=hostcxx.RUN_TIMEOUT)
    print(f'PASS {label} ({commit[:8]}) refuses both {SONGS_ID} seeds of this build and loads its seeds without '
          'foreign-song logic' + (' (vanilla and Master Quest)' if knows_quests else ' (all-vanilla)'))
if skipped:
    print(f'SKIPPED: this shallow checkout lacks {lacking}; the other older seed guard passed')
