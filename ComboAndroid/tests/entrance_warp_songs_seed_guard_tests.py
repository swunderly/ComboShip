"""A Wallmaster seed that starts with an OoT warp song under logic is refused by older builds; this build's seeds
without one still load there.

This build writes Wallmaster Shuffle (Own Game) seeds that start with Prelude of Light, which require
wallmasters-own-game-songs-v2 (limits design, Stage 2): with all-vanilla and with Master Quest dungeons, each with and
without "Play warp songs across games" in logic. It also writes three seeds without a starting warp song: an
all-vanilla one (wallmasters-own-game-vanilla-v1), a Master Quest one (wallmasters-own-game-quests-v1) and one with
foreign-song logic (wallmasters-own-game-songs-v1). Two older seed guards are then compiled from this checkout's
history and run on them, with their shipping set and their whole catalog alike:
- 0.4.12 (combo/ at tag v0.4.12-android) refuses the four songs-v2 seeds and the quests and songs-v1 ones, which name
  a capability it does not know (EntranceLayout.h ValidateEntranceLayoutSettings), and loads the all-vanilla one;
- the 0.4.13 candidate without this stage (combo/ at the parent of the first commit that adds
  wallmasters-own-game-songs-v2 to Capabilities.h, found by content: Master Quest and cross-game song Wallmasters,
  whose seed guard is 0b7b2d50's) refuses the four songs-v2 seeds the same way, before reading their song facts, and
  loads the other three.
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
WARP_ID, CAPABILITIES = 'wallmasters-own-game-songs-v2', 'combo/rando/entrances/Capabilities.h'


def git(*command):
    return subprocess.run(['git', '-C', str(source), *command], capture_output=True, text=True, timeout=300)


def resolve(rev):
    found = git('rev-parse', '--verify', '--quiet', rev + '^{commit}')
    return found.stdout.strip() if found.returncode == 0 else None


def songs_base():
    """The parent of the first commit that adds the songs-v2 id to the capability catalogue, or None."""
    added = git('log', '--reverse', '--format=%H', '-S', WARP_ID, '--', CAPABILITIES)
    first = added.stdout.split()[0] if added.returncode == 0 and added.stdout.split() else None
    return resolve(first + '^') if first else None


shallow = git('rev-parse', '--is-shallow-repository')
if shallow.returncode != 0:
    sys.exit(f'FAIL: {source} is not a git checkout, so the older seed guards cannot be read from its history')
shallow = shallow.stdout.strip() == 'true'
# (label, how to find it, what to fetch, whether that build knows the quests and songs-v1 ids)
OLDER = [('v0.4.12-android', lambda: resolve('v0.4.12-android'), "the fork's tags (git fetch --tags)", False),
         ('0.4.13 candidate without Stage 2', songs_base, f'the history that adds {WARP_ID} to {CAPABILITIES}', True)]
builds, skipped = [], []
for label, find, fetch, knows_songs in OLDER:
    commit = find()
    if commit:
        builds.append((label, commit, knows_songs))
    elif shallow:
        skipped.append(label)
    else:
        sys.exit(f'FAIL: this checkout is not shallow but has no {label} build; fetch {fetch} to run its seed guard')
lacking = ' and '.join(f'the {label} build' for label in skipped)
if not builds:
    print(f'SKIPPED: this shallow checkout lacks {lacking} (fetch the full history and the tags to run the older seed '
          'guards)')
    sys.exit(0)

# This build's seeds: four that start with Prelude of Light (songs-v2), then vanilla, quests and songs-v1.
writer = work/'write_seeds.cpp'
writer.write_text(r'''
#include "rando/entrances/MmUnifiedLayout.h"
#include "rando/entrances/OotUnifiedLayout.h"
#include <cassert>
#include <fstream>
using namespace ComboRando;
using namespace ComboRando::Entrances;
int main(int argc, char** argv) {
    assert(argc == 8);
    const auto caps = CapabilitySet::AtLeast(Stage::Hidden);
    SongTravelMap foreignMap;
    foreignMap.foreignSongs = foreignMap.logicForeignSongs = true;
    struct Kind { bool warp, mq, foreign; std::string_view id; };
    const Kind kinds[] = {{true, false, false, kStartingSongsWallmasterCapability},
                          {true, false, true, kStartingSongsWallmasterCapability},
                          {true, true, false, kStartingSongsWallmasterCapability},
                          {true, true, true, kStartingSongsWallmasterCapability},
                          {false, false, false, kVanillaWallmasterCapability},
                          {false, true, false, kQuestsWallmasterCapability},
                          {false, false, true, kSongsWallmasterCapability}};
    int out = 1;
    for (const auto& kind : kinds) {
        EntranceLayout layout;
        layout.settings.erWallmasters = Mode::OwnGame;
        if (kind.mq)
            layout.ootMq[OotMqIndex("Forest")] = layout.ootMq[OotMqIndex("Water")] = true;
        layout.required = {std::string(kind.id)};
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
                            {"songTravel", kind.foreign ? foreignMap.json() : SongTravelMap{}.json()},
                            {"accessibility", {{"version", 1}, {"mode", "everyCheck"}}}};
        if (kind.warp)
            seed["oot"]["settings"]["gRandoSettings.StartingPreludeOfLight"] = 1;
        RequireSupportedSeedCapabilities(seed, caps);
        OotUnifiedLayout installed;
        installed.install(seed, caps);
        std::ofstream(argv[out++]) << seed.dump();
    }
}
''', encoding='utf-8')
exe = hostcxx.build(a.compiler, writer, includes=[source/'combo', a.json_include], defines=['_CRT_SECURE_NO_WARNINGS'],
                    msvc=['/bigobj'], exe=work/'write_seeds.exe')
names = ['warp-vanilla', 'warp-vanilla-foreign', 'warp-mq', 'warp-mq-foreign', 'vanilla', 'quests', 'songs']
seeds = [work/f'{name}.json' for name in names]
subprocess.run([str(exe), *map(str, seeds)], check=True, timeout=hostcxx.RUN_TIMEOUT)
required = [json.loads(path.read_text())['entranceLayout']['requires'] for path in seeds]
assert required == [['wallmasters-own-game-songs-v2']] * 4 + [['wallmasters-own-game-vanilla-v1'],
                                                             ['wallmasters-own-game-quests-v1'],
                                                             ['wallmasters-own-game-songs-v1']], required

# The older guards on them: argv[8] is 1 when that build knows the quests and songs-v1 ids.
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
    assert(argc == 9);
    nlohmann::json seeds[7];
    for (int i = 0; i < 7; ++i) {
        std::ifstream file(argv[i + 1]);
        seeds[i] = nlohmann::json::parse(file);
    }
    const bool knowsSongs = std::string(argv[8]) == "1";
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
        for (int i = 0; i < 4; ++i)
            refused(seeds[i], "wallmasters-own-game-songs-v2");
        RequireSupportedSeedCapabilities(seeds[4], caps);
        Validate(DecodeEntranceLayout(seeds[4].at("entranceLayout")), caps);
        if (knowsSongs) {
            RequireSupportedSeedCapabilities(seeds[5], caps);
            RequireSupportedSeedCapabilities(seeds[6], caps);
        } else {
            refused(seeds[5], "wallmasters-own-game-quests-v1");
            refused(seeds[6], "wallmasters-own-game-songs-v1");
        }
    }
    std::cout << "refuses every starting-song Wallmaster seed and loads the others it knows\n";
}
''', encoding='utf-8')
for label, commit, knows_songs in builds:
    if label == 'v0.4.12-android':
        assert commit == '33c972f1c42428ff52da2e0a99e5884f57f9db03', commit
    rev = label.replace(' ', '-')
    old = work/rev
    archive = subprocess.run(['git', '-C', str(source), 'archive', '--format=tar', commit, 'combo'],
                             capture_output=True, check=True, timeout=300).stdout
    with tarfile.open(fileobj=io.BytesIO(archive)) as tar:
        tar.extractall(old, filter='data')
    catalog = (old/'combo/rando/entrances/Capabilities.h').read_text(encoding='utf-8')
    assert 'wallmasters-own-game-songs-v2' not in catalog
    assert ('wallmasters-own-game-songs-v1' in catalog) == knows_songs
    assert ('wallmasters-own-game-quests-v1' in catalog) == knows_songs
    exe = hostcxx.build(a.compiler, reader, includes=[old/'combo', a.json_include],
                        defines=['_CRT_SECURE_NO_WARNINGS'], msvc=['/bigobj'], exe=work/f'guard_{rev}.exe')
    subprocess.run([str(exe), *map(str, seeds), '1' if knows_songs else '0'], check=True,
                   timeout=hostcxx.RUN_TIMEOUT)
    print(f'PASS {label} ({commit[:8]}) refuses the four {WARP_ID} seeds of this build and loads its seeds without a '
          'starting warp song' + (' (vanilla, Master Quest and foreign songs)' if knows_songs else ' (all-vanilla)'))
if skipped:
    print(f'SKIPPED: this shallow checkout lacks {lacking}; the other older seed guard passed')
