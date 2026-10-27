"""A Wallmaster seed with Warp Songs and Soaring Spots Shuffle is refused by older builds; this build's other seeds
still load there.

This build writes Wallmaster seeds that also shuffle the warp songs and the owl statues (limits design, Stage 4),
which require wallmasters-own-game-warps-v1 or wallmasters-full-warps-v1 and record OoTMM's erWarps=full in their
layout settings: Own Game and Full, each all-vanilla and with Master Quest dungeons, each with the song travel map its
warp routes give. It also writes one Full seed and one all-vanilla Own Game seed without the warps. Three older seed
guards are then compiled from this checkout's history and run on them, with their shipping set and their whole
catalog alike:
- 0.4.12 (combo/ at tag v0.4.12-android), 0.4.13 (v0.4.13-android) and 0.4.14 (v0.4.14-android, the build this one
  starts from) each refuse the four warps seeds. They refuse them before reading the capability ids: their decoder
  reads the layout settings first and knows no erWarps key (EntranceLayout.h DecodeEntranceLayout: "settings has
  unknown key"), so the refusal names the key, not the id. No older build can name the id, because any warps seed
  must carry the warp routes or the key, and their decoder refuses both before it reads "requires".
- Each loads the all-vanilla Own Game seed; 0.4.14 also loads the Full seed, and the earlier two refuse it by its id.
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
WARPS_IDS = ['wallmasters-own-game-warps-v1', 'wallmasters-full-warps-v1']
FULL_ID, CAPABILITIES = 'wallmasters-full-v1', 'combo/rando/entrances/Capabilities.h'
REFUSED = 'This seed cannot be used by this build: Entrance layout settings has unknown key erWarps'


def git(*command):
    return subprocess.run(['git', '-C', str(source), *command], capture_output=True, text=True, timeout=300)


def resolve(rev):
    found = git('rev-parse', '--verify', '--quiet', rev + '^{commit}')
    return found.stdout.strip() if found.returncode == 0 else None


def warps_base():
    """The parent of the first commit that adds a warps id to the capability catalogue, or None."""
    added = git('log', '--reverse', '--format=%H', '-S', WARPS_IDS[0], '--', CAPABILITIES)
    first = added.stdout.split()[0] if added.returncode == 0 and added.stdout.split() else None
    return resolve(first + '^') if first else None


shallow = git('rev-parse', '--is-shallow-repository')
if shallow.returncode != 0:
    sys.exit(f'FAIL: {source} is not a git checkout, so the older seed guards cannot be read from its history')
shallow = shallow.stdout.strip() == 'true'
# (label, commit, whether that build knows the Full id)
OLDER = [('v0.4.12-android', '33c972f1c42428ff52da2e0a99e5884f57f9db03', False),
         ('v0.4.13-android', 'a5a0bdabea402e1de8196d005988996b463c14f4', False),
         ('v0.4.14-android', 'dcebba851c57656034299aa6c5829c5d3827a906', True)]
builds, skipped = [], []
for label, pinned, knows in OLDER:
    commit = resolve(label)
    if commit:
        assert commit == pinned, (label, commit)
        builds.append((label, commit, knows))
    elif shallow:
        skipped.append(label)
    else:
        sys.exit(f"FAIL: this checkout is not shallow but has no {label} build; fetch the fork's tags "
                 '(git fetch --tags) to run its seed guard')
lacking = ' and '.join(f'the {label} build' for label in skipped)
if not builds:
    print(f'SKIPPED: this shallow checkout lacks {lacking} (fetch the full history and the tags to run the older seed '
          'guards)')
    sys.exit(0)
# The warps ids are new after the 0.4.14 release this build starts from, and nothing between it and the commit that
# adds them changes the capability catalogue.
base = warps_base()
for label, commit, _ in builds:
    if base and label == 'v0.4.14-android':
        assert git('merge-base', '--is-ancestor', commit, base).returncode == 0, (label, commit, base)
        assert git('diff', '--quiet', commit, base, '--', CAPABILITIES).returncode == 0, (label, commit, base)

# This build's seeds: Own Game and Full with the warps, all-vanilla and with Master Quest dungeons, then a Full one and
# an all-vanilla Own Game one without them.
writer = work/'write_seeds.cpp'
writer.write_text(r'''
#include "rando/ComboSeedFormat.h"
#include "rando/entrances/MmUnifiedLayout.h"
#include "rando/entrances/OotUnifiedLayout.h"
#include <cassert>
#include <fstream>
#include <set>
using namespace ComboRando;
using namespace ComboRando::Entrances;
int main(int argc, char** argv) {
    assert(argc == 7);
    const auto caps = CapabilitySet::AtLeast(Stage::Hidden);
    struct Kind { Mode wallmasters; bool mq, warps; std::string_view id; };
    const Kind kinds[] = {{Mode::OwnGame, false, true, kOwnGameWarpsWallmasterCapability},
                          {Mode::OwnGame, true, true, kOwnGameWarpsWallmasterCapability},
                          {Mode::Full, false, true, kFullWarpsWallmasterCapability},
                          {Mode::Full, true, true, kFullWarpsWallmasterCapability},
                          {Mode::Full, false, false, kFullWallmasterCapability},
                          {Mode::OwnGame, false, false, kVanillaWallmasterCapability}};
    int out = 1;
    for (const auto& kind : kinds) {
        EntranceLayout layout;
        layout.settings.erWallmasters = kind.wallmasters;
        layout.settings.erWarps = kind.warps ? Warps::Full : Warps::None;
        if (kind.mq)
            layout.ootMq[OotMqIndex("Spirit")] = layout.ootMq[OotMqIndex("Water")] = true;
        layout.required = {std::string(kind.id)};
        const auto pools = kind.warps ? LayoutV3::MakePools(layout.settings, layout.ootMq)
                         : kind.wallmasters == Mode::Full ? LayoutV2::MakePools(layout.settings, layout.ootMq)
                                                          : LayoutV1::MakePools(layout.settings, layout.ootMq, false);
        // Warp i lands at warp i + 5; each other source, in name order, takes its pool's first free candidate.
        Overrides routes;
        if (kind.warps)
            for (size_t i = 0; i < kWarpRows.size(); ++i)
                routes[kWarpRows[i]] = kWarpRows[(i + 5) % kWarpRows.size()];
        std::set<std::string_view> used;
        for (const auto& pool : pools)
            for (const auto src : pool.src)
                for (const auto dst : pool.dst) {
                    const auto* from = LayoutV3::FindRow(src);
                    const auto* to = LayoutV3::FindRow(dst);
                    if (!routes.contains(src) && !used.contains(dst) && LayoutV1::RowAssignable(*from, *to, pool.ownGame)) {
                        routes[src] = dst;
                        used.insert(dst);
                    }
                }
        for (const auto& pool : pools)
            for (const auto src : pool.src)
                assert(routes.contains(src));
        layout.overrides = routes;
        const auto validated = Validate(layout, caps);
        SongTravelMap songs;
        if (const auto destinations = WarpDestinations(validated)) {
            songs.enabled = songs.logicEnabled = true;
            for (size_t i = 0; i < songs.destinations.size(); ++i)
                songs.destinations[i] = (*destinations)[i];
        }
        assert(WarpDestinations(validated).has_value() == kind.warps);
        nlohmann::json seed{{"fileType", kEntranceSeedFileType}, {"version", kEntranceSeedVersion},
                            {"entranceLayout", EncodeEntranceLayout(layout)}, {"songTravel", songs.json()},
                            {"accessibility", {{"version", 1}, {"mode", "everyCheck"}}}};
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
names = ['warps-own-game', 'warps-own-game-mq', 'warps-full', 'warps-full-mq', 'full', 'vanilla']
seeds = [work/f'{name}.json' for name in names]
subprocess.run([str(exe), *map(str, seeds)], check=True, timeout=hostcxx.RUN_TIMEOUT)
documents = [json.loads(path.read_text()) for path in seeds]
required = [d['entranceLayout']['requires'] for d in documents]
assert required == [[WARPS_IDS[0]]] * 2 + [[WARPS_IDS[1]]] * 2 + [[FULL_ID], ['wallmasters-own-game-vanilla-v1']], \
    required
# The warps seeds keep the v1 envelope schema and add one settings key, with the sixteen warp routes and the song
# travel map those routes give; the other two carry neither.
assert all(d['entranceLayout']['schema'] == 'ootmm-entrance-layout-v1' for d in documents)
for d in documents[:4]:
    assert d['entranceLayout']['settings']['erWarps'] == 'full', d['entranceLayout']['settings']
    warp_routes = [r for r in d['entranceLayout']['overrides'] if '_WARP_' in r['source']]
    assert len(warp_routes) == 16 and d['songTravel']['logicEnabled'] is True, d['songTravel']
for d in documents[4:]:
    assert 'erWarps' not in d['entranceLayout']['settings']
    assert not any('_WARP_' in r['source'] for r in d['entranceLayout']['overrides'])

# The older guards on them: argv[7] is 1 when that build knows the Full id.
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
    assert(argc == 8);
    nlohmann::json seeds[6];
    for (int i = 0; i < 6; ++i) {
        std::ifstream file(argv[i + 1]);
        seeds[i] = nlohmann::json::parse(file);
    }
    const bool knowsFull = std::string(argv[7]) == "1";
    for (const auto& caps : {ActiveCapabilities(), CapabilitySet::AtLeast(Stage::Hidden)}) {
        const auto refused = [&](const nlohmann::json& seed, const std::string& expected) {
            std::string why;
            try {
                RequireSupportedSeedCapabilities(seed, caps);
            } catch (const UnsupportedComboSeedFormat& error) {
                why = error.what();
            }
            if (why != expected) {
                std::cerr << "expected \"" << expected << "\", got \"" << why << "\"\n";
                std::exit(1);
            }
        };
        for (int i = 0; i < 4; ++i)
            refused(seeds[i], "%REFUSED%");
        if (knowsFull)
            RequireSupportedSeedCapabilities(seeds[4], caps);
        else
            refused(seeds[4], "This seed cannot be used by this build: Entrance layout requires unknown capability "
                              "wallmasters-full-v1");
        RequireSupportedSeedCapabilities(seeds[5], caps);
        Validate(DecodeEntranceLayout(seeds[5].at("entranceLayout")), caps);
    }
    std::cout << "refuses every warps seed and loads the other seeds it knows\n";
}
'''.replace('%REFUSED%', REFUSED), encoding='utf-8')
for label, commit, knows in builds:
    old = work/label
    archive = subprocess.run(['git', '-C', str(source), 'archive', '--format=tar', commit, 'combo'],
                             capture_output=True, check=True, timeout=300).stdout
    with tarfile.open(fileobj=io.BytesIO(archive)) as tar:
        tar.extractall(old, filter='data')
    catalog = (old/'combo/rando/entrances/Capabilities.h').read_text(encoding='utf-8')
    assert all(id not in catalog for id in WARPS_IDS) and (FULL_ID in catalog) == knows, label
    assert 'kLayoutWarpsKey' not in (old/'combo/rando/entrances/EntranceLayout.h').read_text(encoding='utf-8'), label
    exe = hostcxx.build(a.compiler, reader, includes=[old/'combo', a.json_include],
                        defines=['_CRT_SECURE_NO_WARNINGS'], msvc=['/bigobj'], exe=work/f'guard_{label}.exe')
    subprocess.run([str(exe), *map(str, seeds), '1' if knows else '0'], check=True, timeout=hostcxx.RUN_TIMEOUT)
    print(f'PASS {label} ({commit[:8]}) refuses the four warps seeds of this build ("{REFUSED}") and loads its '
          + ('Full and all-vanilla Own Game seeds' if knows else f'all-vanilla Own Game seed, refusing {FULL_ID} by id'))
if skipped:
    print(f'SKIPPED: this shallow checkout lacks {lacking}; the other older seed guards passed')
