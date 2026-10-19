"""OoTMM v32.3's smaller Master Quest gameplay rules for a dungeon that is MQ from the bundled data (MQ port unit U6).

Besides the scene and room data (mq.c), OoTMM reads its MQ flag (Config_IsMq) in a few gameplay rules. This harness
compiles the production code each rule touches, with engine stubs, and runs it for a bundled MQ dungeon, a dungeon
from a real oot-mq.o2r (archive) and a vanilla one:
- MQ Water's locked doors: OoTMM's bins put transition 0x16 on switch 2 (scripts/make_mq.ts:226-229), so a bundled seed
  counts {2, 21} as spent keys (dungeon.cpp GetDoorFlags), and FixWaterMQLock.cpp, which moves the archive's door from
  switch 3 to 4, leaves a bundled door alone.
"""
from pathlib import Path
import argparse
import re
import subprocess
import sys

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--compiler', default='')
args = parser.parse_args()
repo = args.source.resolve()
work = args.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
sys.path.insert(0, str(Path(__file__).resolve().parent))
import hostcxx  # noqa: E402

rando = repo / 'soh/soh/Enhancements/randomizer'
failures = []


def check(condition, message):
    if not condition:
        failures.append(message)
    return condition


def read(path):
    return Path(path).read_text(encoding='utf-8')


def function(text, signature):
    """The definition that starts at `signature` (whitespace-insensitive), through its closing brace."""
    start = hostcxx.find(text, signature)
    opening = text.index('{', start)
    depth = 0
    for at in range(opening, len(text)):
        depth += {'{': 1, '}': -1}.get(text[at], 0)
        if depth == 0:
            return text[start:at + 1]
    raise AssertionError(f'unbalanced body after {signature}')


def statement(text, needle):
    """The whole line holding `needle`, through the next ';'."""
    start = text.rindex('\n', 0, hostcxx.find(text, needle)) + 1
    return text[start:text.index(';', start) + 1]


# --- MQ Water's locked doors -----------------------------------------------------------------------------------------
dungeon_cpp = read(rando / 'dungeon.cpp')
rows = {}
for key in ('FIRE_TEMPLE', 'WATER_TEMPLE'):
    row = dungeon_cpp[hostcxx.find(dungeon_cpp, f'dungeonList[{key}] ='):]
    row = row[:row.index(');')]
    rows[key] = re.findall(r'\{([\d,\s]*)\}', row)[-3:]  # vanillaDoorFlags, randoDoorFlags, MQDoorFlags
water_fix = read(repo / 'soh/soh/Enhancements/Fixes/FixWaterMQLock.cpp')

doors = r'''#include <cstdint>
#include <cstdio>
#include <vector>
#define COMBO_BUILD 1
using s8 = int8_t; using s16 = int16_t; using s32 = int32_t; using u8 = uint8_t; using u16 = uint16_t;
using u32 = uint32_t;
enum SceneID { SCENE_FIRE_TEMPLE = 4, SCENE_WATER_TEMPLE = 5 };
struct SavedSceneFlags { u32 chest, swch, clear, collect; };
struct SaveContext { SavedSceneFlags sceneFlags[16]; };
struct PlayState { s16 sceneNum; struct { struct { u32 swch; } flags; } actorCtx; };
struct Actor { s16 params; };
struct EnDoor { Actor actor; };
PlayState* gPlayState = nullptr;
bool gRando = true, gGameMq = false, gBundled = false;
#define IS_RANDO (gRando)
extern "C" uint32_t ResourceMgr_SceneUsesBundledMq(s16) { return gBundled; }
extern "C" uint32_t ResourceMgr_GameUsesBundledMq() { return gBundled; }
extern "C" uint32_t ResourceMgr_IsGameMasterQuest() { return gGameMq; }
struct DungeonInfo {
    bool masterQuest;
    SceneID scene;
    std::vector<uint8_t> vanillaDoorFlags, randoDoorFlags, MQDoorFlags;
    bool IsMQ() const { return masterQuest; }
    const std::vector<uint8_t>* GetDoorFlags() const;
};
'''
doors += statement(dungeon_cpp, 'static const std::vector<uint8_t> kBundledMqWaterDoorFlags') + '\n'
doors += function(dungeon_cpp, 'const std::vector<uint8_t>* DungeonInfo::GetDoorFlags() const') + '\n'
doors += function(dungeon_cpp, 'int8_t FindUsedSmallKeys(') + '\n'
doors += function(water_fix, 'static void OnInitEnDoor(void* refActor)') + '\n'
doors += r'''#define REQUIRE(x) do { if (!(x)) { fprintf(stderr, "failed: %s at line %d\n", #x, __LINE__); return 1; } } while (0)
using Flags = std::vector<uint8_t>;
int main() {
    DungeonInfo water{ false, SCENE_WATER_TEMPLE, { @WATER0@ }, { @WATER1@ }, { @WATER2@ } };
    DungeonInfo fire{ false, SCENE_FIRE_TEMPLE, { @FIRE0@ }, { @FIRE1@ }, { @FIRE2@ } };
    // A bundled MQ Water seed spends its keys on OoTMM's switches; a real oot-mq.o2r keeps SoH's.
    water.masterQuest = true;
    gBundled = true;
    REQUIRE(*water.GetDoorFlags() == (Flags{ 2, 21 }));
    gBundled = false;
    REQUIRE(*water.GetDoorFlags() == water.MQDoorFlags && water.MQDoorFlags == (Flags{ 4, 21 }));
    // Vanilla Water and every other dungeon keep their rows, bundled or not.
    water.masterQuest = false;
    gBundled = true;
    REQUIRE(*water.GetDoorFlags() == water.randoDoorFlags);
    gRando = false;
    REQUIRE(*water.GetDoorFlags() == water.vanillaDoorFlags);
    gRando = true;
    fire.masterQuest = true;
    REQUIRE(*fire.GetDoorFlags() == fire.MQDoorFlags);
    // The key OoTMM's door 0x16 took counts as spent: one used key with switch 2 set, in the scene or saved.
    water.masterQuest = true;
    SaveContext save{};
    save.sceneFlags[SCENE_WATER_TEMPLE].swch = 1u << 2;
    REQUIRE(FindUsedSmallKeys(&save, SCENE_WATER_TEMPLE, water.GetDoorFlags()) == 1);
    PlayState play{ SCENE_WATER_TEMPLE, { { (1u << 2) | (1u << 21) } } };
    gPlayState = &play;
    REQUIRE(FindUsedSmallKeys(&save, SCENE_WATER_TEMPLE, water.GetDoorFlags()) == 2);
    gBundled = false;
    REQUIRE(FindUsedSmallKeys(&save, SCENE_WATER_TEMPLE, water.GetDoorFlags()) == 1);
    // FixWaterMQLock moves the archive's door 0x16 (switch 3) to switch 4, and leaves a bundled door alone.
    EnDoor door{ { 22659 } };
    gGameMq = true;
    OnInitEnDoor(&door);
    REQUIRE(door.actor.params == 22660);
    gBundled = true;
    door.actor.params = 22659;
    OnInitEnDoor(&door);
    REQUIRE(door.actor.params == 22659);
    door.actor.params = 22658;
    OnInitEnDoor(&door);
    REQUIRE(door.actor.params == 22658);
    gGameMq = false;
    gBundled = false;
    door.actor.params = 22659;
    OnInitEnDoor(&door);
    REQUIRE(door.actor.params == 22659);
    puts("doors: bundled MQ Water counts {2, 21}, a real oot-mq.o2r {4, 21} with FixWaterMQLock");
    return 0;
}
'''
for key, prefix in (('WATER_TEMPLE', 'WATER'), ('FIRE_TEMPLE', 'FIRE')):
    for index, flags in enumerate(rows[key]):
        doors = doors.replace(f'@{prefix}{index}@', flags)

PROGRAMS = {'doors': doors}
for name, code in PROGRAMS.items():
    source = work / f'mq_gameplay_{name}.cpp'
    source.write_text(code, encoding='utf-8')
    exe = hostcxx.build(args.compiler or hostcxx.DEFAULT_COMPILER, source, warnings=False)
    result = subprocess.run([str(exe)], capture_output=True, text=True, timeout=hostcxx.RUN_TIMEOUT)
    print(result.stdout.strip())
    check(result.returncode == 0, f'{name}: {result.stdout}{result.stderr}'.strip())

if failures:
    print('FAIL:\n  ' + '\n  '.join(failures))
    sys.exit(1)
print('PASS: OoTMM\'s MQ gameplay rules hold for bundled MQ, and archive MQ and vanilla keep SoH\'s')
