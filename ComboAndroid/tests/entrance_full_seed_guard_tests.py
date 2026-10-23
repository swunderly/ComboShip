"""A Full Wallmaster Shuffle seed is refused by older builds; this build's Own Game seeds still load there.

This build writes Full Wallmaster seeds (limits design, Stage 3), which require wallmasters-full-v1: all-vanilla and
with Master Quest dungeons, without song facts, with "Play warp songs across games" in logic and with a starting
Prelude of Light, each with routes between the games in both directions. It also writes one Own Game seed of each
earlier id (vanilla, quests, songs-v1, songs-v2). Three older seed guards are then compiled from this checkout's
history and run on them, with their shipping set and their whole catalog alike:
- 0.4.12 (combo/ at tag v0.4.12-android) refuses the four Full seeds, which name a capability it does not know
  (EntranceLayout.h ValidateEntranceLayoutSettings: "requires unknown capability"), and loads the all-vanilla one;
- 0.4.13 RC1 (combo/ at tag v0.4.13-rc1) and the 0.4.13 release (combo/ at tag v0.4.13-android, the build this one
  starts from) each refuse the four Full seeds the same way, before reading their routes or song facts, and load all
  four Own Game seeds. Both are ancestors of the first commit that adds wallmasters-full-v1 to Capabilities.h, with
  the same catalogue as that commit's parent.
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
FULL_ID, CAPABILITIES = 'wallmasters-full-v1', 'combo/rando/entrances/Capabilities.h'


def git(*command):
    return subprocess.run(['git', '-C', str(source), *command], capture_output=True, text=True, timeout=300)


def resolve(rev):
    found = git('rev-parse', '--verify', '--quiet', rev + '^{commit}')
    return found.stdout.strip() if found.returncode == 0 else None


def full_base():
    """The parent of the first commit that adds the Full id to the capability catalogue, or None."""
    added = git('log', '--reverse', '--format=%H', '-S', FULL_ID, '--', CAPABILITIES)
    first = added.stdout.split()[0] if added.returncode == 0 and added.stdout.split() else None
    return resolve(first + '^') if first else None


shallow = git('rev-parse', '--is-shallow-repository')
if shallow.returncode != 0:
    sys.exit(f'FAIL: {source} is not a git checkout, so the older seed guards cannot be read from its history')
shallow = shallow.stdout.strip() == 'true'
# (label, how to find it, what to fetch, whether that build knows the 0.4.13 ids)
OLDER = [('v0.4.12-android', lambda: resolve('v0.4.12-android'), "the fork's tags (git fetch --tags)", False),
         ('v0.4.13-rc1', lambda: resolve('v0.4.13-rc1'), "the fork's tags (git fetch --tags)", True),
         ('v0.4.13-android', lambda: resolve('v0.4.13-android'), "the fork's tags (git fetch --tags)", True)]
builds, skipped = [], []
for label, find, fetch, knows in OLDER:
    commit = find()
    if commit:
        builds.append((label, commit, knows))
    elif shallow:
        skipped.append(label)
    else:
        sys.exit(f'FAIL: this checkout is not shallow but has no {label} build; fetch {fetch} to run its seed guard')
lacking = ' and '.join(f'the {label} build' for label in skipped)
if not builds:
    print(f'SKIPPED: this shallow checkout lacks {lacking} (fetch the full history and the tags to run the older seed '
          'guards)')
    sys.exit(0)
# The Full id is new after RC1 and after the 0.4.13 release this build starts from, and nothing between them and the
# commit that adds it changes the capability catalogue.
base = full_base()
for label, commit, _ in builds:
    if base and label in ('v0.4.13-rc1', 'v0.4.13-android'):
        assert git('merge-base', '--is-ancestor', commit, base).returncode == 0, (label, commit, base)
        assert git('diff', '--quiet', commit, base, '--', CAPABILITIES).returncode == 0, (label, commit, base)

# This build's seeds: four Full ones, then the vanilla, quests, songs-v1 and songs-v2 ones.
writer = work/'write_seeds.cpp'
writer.write_text(r'''
#include "rando/entrances/MmUnifiedLayout.h"
#include "rando/entrances/OotUnifiedLayout.h"
#include <cassert>
#include <fstream>
using namespace ComboRando;
using namespace ComboRando::Entrances;
int main(int argc, char** argv) {
    assert(argc == 9);
    const auto caps = CapabilitySet::AtLeast(Stage::Hidden);
    SongTravelMap foreignMap;
    foreignMap.foreignSongs = foreignMap.logicForeignSongs = true;
    enum Facts { None, Foreign, Warp };
    struct Kind { bool full, mq; Facts facts; std::string_view id; };
    const Kind kinds[] = {{true, false, None, kFullWallmasterCapability},
                          {true, true, None, kFullWallmasterCapability},
                          {true, false, Foreign, kFullWallmasterCapability},
                          {true, true, Warp, kFullWallmasterCapability},
                          {false, false, None, kVanillaWallmasterCapability},
                          {false, true, None, kQuestsWallmasterCapability},
                          {false, false, Foreign, kSongsWallmasterCapability},
                          {false, false, Warp, kStartingSongsWallmasterCapability}};
    int out = 1;
    for (const auto& kind : kinds) {
        EntranceLayout layout;
        layout.settings.erWallmasters = kind.full ? Mode::Full : Mode::OwnGame;
        if (kind.mq)
            layout.ootMq[OotMqIndex("Spirit")] = layout.ootMq[OotMqIndex("BotW")] = true; // Own Game's shortfall
        if (kind.mq && !kind.full)
            layout.ootMq[OotMqIndex("Spirit")] = false, layout.ootMq[OotMqIndex("Water")] = true;
        layout.required = {std::string(kind.id)};
        Overrides routes;
        if (kind.full) {
            // MM sources to the first OoT dungeons, OoT sources to the MM ones, then the rest of OoT's.
            std::vector<std::string_view> destinations(kOotWallmasterArrivalRows.begin(),
                                                       kOotWallmasterArrivalRows.end());
            destinations.insert(destinations.begin() + 4, kMmBaseWallmasterDestinations.begin(),
                                kMmBaseWallmasterDestinations.end());
            size_t next = 0;
            const auto pools = LayoutV2::MakePools(layout.settings, layout.ootMq); // outlives the loop
            for (const auto src : pools[0].src)
                routes[src] = destinations[next++];
        } else {
            size_t oot = 0, mm = 0;
            for (const auto& pool : LayoutV1::MakePools(layout.settings, layout.ootMq, false))
                for (const auto src : pool.src)
                    routes[src] = Find(src)->game == Game::Oot ? kOotWallmasterArrivalRows[oot++]
                                                               : kMmBaseWallmasterDestinations[mm++];
        }
        layout.overrides = routes;
        const auto validated = Validate(layout, caps);
        assert(!kind.full || (!validated.ootToMm.empty() && !validated.mmToOot.empty()));
        nlohmann::json seed{{"fileType", kEntranceSeedFileType}, {"version", kEntranceSeedVersion},
                            {"entranceLayout", EncodeEntranceLayout(layout)},
                            {"songTravel", kind.facts == Foreign ? foreignMap.json() : SongTravelMap{}.json()},
                            {"accessibility", {{"version", 1}, {"mode", "everyCheck"}}}};
        if (kind.facts == Warp)
            seed["oot"]["settings"]["gRandoSettings.StartingPreludeOfLight"] = 1;
        RequireSupportedSeedCapabilities(seed, caps);
        OotUnifiedLayout oot;
        oot.install(seed, caps);
        MmUnifiedLayout mm;
        mm.install(seed, caps);
        std::ofstream(argv[out++]) << seed.dump();
    }
}
''', encoding='utf-8')
exe = hostcxx.build(a.compiler, writer, includes=[source/'combo', a.json_include], defines=['_CRT_SECURE_NO_WARNINGS'],
                    msvc=['/bigobj'], exe=work/'write_seeds.exe')
names = ['full-vanilla', 'full-mq', 'full-foreign', 'full-mq-warp', 'vanilla', 'quests', 'songs', 'warp']
seeds = [work/f'{name}.json' for name in names]
subprocess.run([str(exe), *map(str, seeds)], check=True, timeout=hostcxx.RUN_TIMEOUT)
documents = [json.loads(path.read_text()) for path in seeds]
required = [d['entranceLayout']['requires'] for d in documents]
assert required == [[FULL_ID]] * 4 + [['wallmasters-own-game-vanilla-v1'], ['wallmasters-own-game-quests-v1'],
                                      ['wallmasters-own-game-songs-v1'], ['wallmasters-own-game-songs-v2']], required
# The Full seeds keep the v1 envelope schema, so an older build refuses them by their capability id.
assert all(d['entranceLayout']['schema'] == 'ootmm-entrance-layout-v1' for d in documents)
assert all(d['entranceLayout']['settings']['erWallmasters'] == 'full' for d in documents[:4])

# The older guards on them: argv[9] is 1 when that build knows the 0.4.13 ids.
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
    assert(argc == 10);
    nlohmann::json seeds[8];
    for (int i = 0; i < 8; ++i) {
        std::ifstream file(argv[i + 1]);
        seeds[i] = nlohmann::json::parse(file);
    }
    const bool knows0413 = std::string(argv[9]) == "1";
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
            refused(seeds[i], "wallmasters-full-v1");
        RequireSupportedSeedCapabilities(seeds[4], caps);
        Validate(DecodeEntranceLayout(seeds[4].at("entranceLayout")), caps);
        if (knows0413) {
            for (int i = 5; i < 8; ++i)
                RequireSupportedSeedCapabilities(seeds[i], caps);
        } else {
            refused(seeds[5], "wallmasters-own-game-quests-v1");
            refused(seeds[6], "wallmasters-own-game-songs-v1");
            refused(seeds[7], "wallmasters-own-game-songs-v2");
        }
    }
    std::cout << "refuses every Full Wallmaster seed and loads the Own Game seeds it knows\n";
}
''', encoding='utf-8')
for label, commit, knows in builds:
    if label == 'v0.4.12-android':
        assert commit == '33c972f1c42428ff52da2e0a99e5884f57f9db03', commit
    if label == 'v0.4.13-android':
        assert commit == 'a5a0bdabea402e1de8196d005988996b463c14f4', commit
    old = work/label
    archive = subprocess.run(['git', '-C', str(source), 'archive', '--format=tar', commit, 'combo'],
                             capture_output=True, check=True, timeout=300).stdout
    with tarfile.open(fileobj=io.BytesIO(archive)) as tar:
        tar.extractall(old, filter='data')
    catalog = (old/'combo/rando/entrances/Capabilities.h').read_text(encoding='utf-8')
    assert FULL_ID not in catalog
    for id in ('wallmasters-own-game-quests-v1', 'wallmasters-own-game-songs-v1', 'wallmasters-own-game-songs-v2'):
        assert (id in catalog) == knows, (label, id)
    exe = hostcxx.build(a.compiler, reader, includes=[old/'combo', a.json_include],
                        defines=['_CRT_SECURE_NO_WARNINGS'], msvc=['/bigobj'], exe=work/f'guard_{label}.exe')
    subprocess.run([str(exe), *map(str, seeds), '1' if knows else '0'], check=True, timeout=hostcxx.RUN_TIMEOUT)
    print(f'PASS {label} ({commit[:8]}) refuses the four {FULL_ID} seeds of this build and loads its Own Game seeds'
          + (' (vanilla, Master Quest, foreign songs and a starting warp song)' if knows else ' (all-vanilla)'))
if skipped:
    print(f'SKIPPED: this shallow checkout lacks {lacking}; the other older seed guard passed')
