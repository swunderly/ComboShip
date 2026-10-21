"""A Master Quest Wallmaster seed is refused by 0.4.12; an all-vanilla seed of this build still loads there.

This build writes two Wallmaster Shuffle (Own Game) seeds with its own headers: one with Master Quest dungeons,
which requires wallmasters-own-game-quests-v1, and one all-vanilla, which requires the frozen
wallmasters-own-game-vanilla-v1. The 0.4.12 release's own seed guard (combo/ at tag v0.4.12-android, read
from this checkout's history) is then compiled and run on both: it must refuse the first, which names a capability
0.4.12 does not know (EntranceLayout.h ValidateEntranceLayoutSettings), and accept the second, with its shipping
set and its whole catalog alike. A checkout without the v0.4.12-android tag reports SKIPPED only when it is a shallow
clone; in a full clone the test fails and says what to fetch.
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
TAG, COMMIT = 'v0.4.12-android', '33c972f1c42428ff52da2e0a99e5884f57f9db03'


def git(*command):
    return subprocess.run(['git', '-C', str(source), *command], capture_output=True, text=True, timeout=300)


shallow = git('rev-parse', '--is-shallow-repository')
if shallow.returncode != 0:
    sys.exit(f'FAIL: {source} is not a git checkout, so the 0.4.12 seed guard cannot be read from its history')
found = git('rev-parse', '--verify', '--quiet', TAG + '^{commit}')
if found.returncode != 0:
    if shallow.stdout.strip() == 'true':
        print(f'SKIPPED: this shallow checkout lacks the {TAG} tag (fetch the full history and the tags to run the '
              '0.4.12 seed guard)')
        sys.exit(0)
    sys.exit(f'FAIL: this checkout is not shallow but has no {TAG} tag; fetch the fork\'s tags (git fetch --tags) to '
             'run its seed guard')
assert found.stdout.strip() == COMMIT, found.stdout
old = work/'v0412'
archive = subprocess.run(['git', '-C', str(source), 'archive', '--format=tar', TAG, 'combo'],
                         capture_output=True, check=True, timeout=300).stdout
with tarfile.open(fileobj=io.BytesIO(archive)) as tar:
    tar.extractall(old, filter='data')
assert 'wallmasters-own-game-quests-v1' not in (old/'combo/rando/entrances/Capabilities.h').read_text(encoding='utf-8')

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
    assert(argc == 3);
    const auto caps = CapabilitySet::AtLeast(Stage::Hidden);
    for (const bool mq : {true, false}) {
        EntranceLayout layout;
        layout.settings.erWallmasters = Mode::OwnGame;
        if (mq)
            layout.ootMq.fill(true);
        layout.required = {std::string(mq ? kQuestsWallmasterCapability : kVanillaWallmasterCapability)};
        Overrides routes;
        size_t oot = 0, mm = 0;
        for (const auto& pool : LayoutV1::MakePools(layout.settings, layout.ootMq, false))
            for (const auto src : pool.src)
                routes[src] = Find(src)->game == Game::Oot ? kOotWallmasterArrivalRows[oot++]
                                                           : kMmBaseWallmasterDestinations[mm++];
        layout.overrides = routes;
        Validate(layout, caps);
        nlohmann::json seed{{"fileType", kEntranceSeedFileType}, {"version", kEntranceSeedVersion},
                            {"entranceLayout", EncodeEntranceLayout(layout)}};
        RequireSupportedSeedCapabilities(seed, caps);
        OotUnifiedLayout installed;
        installed.install(seed, caps);
        std::ofstream(argv[mq ? 1 : 2]) << seed.dump();
    }
}
''', encoding='utf-8')
exe = hostcxx.build(a.compiler, writer, includes=[source/'combo', a.json_include], defines=['_CRT_SECURE_NO_WARNINGS'],
                    msvc=['/bigobj'], exe=work/'write_seeds.exe')
mq_seed, vanilla_seed = work/'quests-seed.json', work/'vanilla-seed.json'
subprocess.run([str(exe), str(mq_seed), str(vanilla_seed)], check=True, timeout=hostcxx.RUN_TIMEOUT)
assert json.loads(mq_seed.read_text())['entranceLayout']['requires'] == ['wallmasters-own-game-quests-v1']
assert json.loads(vanilla_seed.read_text())['entranceLayout']['requires'] == ['wallmasters-own-game-vanilla-v1']

# 0.4.12's guard on them.
reader = work/'guard_0412.cpp'
reader.write_text(r'''
#include "rando/ComboSeedFormat.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
using namespace ComboRando;
using namespace ComboRando::Entrances;
int main(int argc, char** argv) {
    assert(argc == 3);
    std::ifstream mqFile(argv[1]), vanillaFile(argv[2]);
    const auto mq = nlohmann::json::parse(mqFile), vanilla = nlohmann::json::parse(vanillaFile);
    for (const auto& caps : {ActiveCapabilities(), CapabilitySet::AtLeast(Stage::Hidden)}) {
        std::string why;
        try {
            RequireSupportedSeedCapabilities(mq, caps);
        } catch (const UnsupportedComboSeedFormat& error) {
            why = error.what();
        }
        assert(why == "This seed cannot be used by this build: Entrance layout requires unknown capability "
                      "wallmasters-own-game-quests-v1");
        RequireSupportedSeedCapabilities(vanilla, caps);
        Validate(DecodeEntranceLayout(vanilla.at("entranceLayout")), caps);
    }
    std::cout << "0.4.12 refuses the Master Quest Wallmaster seed and loads the all-vanilla one\n";
}
''', encoding='utf-8')
exe = hostcxx.build(a.compiler, reader, includes=[old/'combo', a.json_include], defines=['_CRT_SECURE_NO_WARNINGS'],
                    msvc=['/bigobj'], exe=work/'guard_0412.exe')
subprocess.run([str(exe), str(mq_seed), str(vanilla_seed)], check=True, timeout=hostcxx.RUN_TIMEOUT)
print(f'PASS {TAG} ({COMMIT[:8]}) refuses a wallmasters-own-game-quests-v1 seed and accepts this build\'s '
      'all-vanilla Wallmaster seed')
