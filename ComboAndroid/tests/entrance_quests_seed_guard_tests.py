"""A Master Quest Wallmaster seed is refused by 0.4.12; an all-vanilla seed of this build still loads there.

This build writes two Wallmaster Shuffle (Own Game) seeds with its own headers: one with Master Quest dungeons,
which requires wallmasters-own-game-quests-v1, and one all-vanilla, which requires the frozen
wallmasters-own-game-vanilla-v1. The 0.4.12 release candidate's own seed guard (combo/ at tag v0.4.12-rc1, read
from this checkout's history) is then compiled and run on both: it must refuse the first, which names a capability
0.4.12 does not know (EntranceLayout.h ValidateEntranceLayoutSettings), and accept the second, with its shipping
set and its whole catalog alike. Reports SKIPPED when the checkout has no v0.4.12-rc1 tag (a shallow clone).
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
TAG, COMMIT = 'v0.4.12-rc1', '3151532966517b945c902a0b379ad913b8fe7cfb'

found = subprocess.run(['git', '-C', str(source), 'rev-parse', '--verify', '--quiet', TAG + '^{commit}'],
                       capture_output=True, text=True, timeout=60)
if found.returncode != 0:
    print(f'SKIPPED: this checkout has no {TAG} tag (fetch the tags of the fork to run the 0.4.12 guard)')
    sys.exit(0)
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
