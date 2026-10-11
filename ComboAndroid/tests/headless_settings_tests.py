"""Exercise production --settings metadata capture and CLI/menu precedence."""
from pathlib import Path
import argparse
import subprocess
import hostcxx

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--json-include', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
args = parser.parse_args()
source = args.source.resolve()
work = args.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
headless = (source/'combo/ComboRandoHeadless.cpp').read_text(encoding='utf-8')
marker = '            // Combined metadata is outside the native per-game settings snapshots.'
capture = headless.split(marker, 1)[1].split('            // Exact settings reproduction', 1)[0] if marker in headless else ''
resolution = headless.split('    // #136: goal resolution.', 1)[1]
resolution = resolution[resolution.index('    ComboRando::CwGoal goal;'):].split(
    '    std::cout << "[comborando] validating "', 1)[0]
prelude = r'''
#define NOMINMAX
#include <cassert>
#include <iostream>
#include <sstream>
#include "rando/CrossWorldRando.h"
using Json = nlohmann::json;
using namespace ComboRando;
int goalReads = 0, startReads = 0, sharedReads = 0;
int MenuGoal(int* required, int* total) {++goalReads; *required = 7; *total = 12; return 1;}
int MenuGoalType(int*, int*) {return CwGoal::HUNT;}
int MenuStart() {++startReads; return 0;}
uint32_t MenuShared() {++sharedReads; return SharedMaskAll();}
int Resolve(const Json& sj, Json& out, const std::string& goalArg = "", int triforceRequiredArg = -1,
            int triforceTotalArg = -1, int startingGameArg = -1, const std::string& sharedItemsArg = "") {
    auto SOH_ReadComboGoalCVars = &MenuGoal;
    auto SOH_ReadComboGoalTypeCVars = &MenuGoalType;
    auto SOH_ReadComboStartingGameCVar = &MenuStart;
    auto SOH_ReadComboSharedCVars = &MenuShared;
    Json savedGoal;
    int savedStartingGame = -1;
    bool hasSavedSharedItems = false;
    uint32_t savedSharedMask = 0;
'''
prelude += capture + resolution + r'''
    out = {{"goal", CwGoalJson(goal)}, {"start", startCfg}, {"shared", sharedMask}};
    return 0;
}
'''
tests = r'''
int main() {
    Json seed = {{"oot", {{"settings", Json::object()}}}, {"mm", {{"settings", Json::object()}}},
        {"goal", {{"type", "triforceHunt"}, {"requiredPieces", 21}, {"totalPieces", 31}}},
        {"startingGame", "MM"}, {"sharedItems", Json::array()}};
    Json out;
    assert(Resolve(seed, out) == 0);
    assert(out["goal"] == seed["goal"] && out["start"] == 1 && out["shared"] == 0);
    assert(goalReads == 0 && startReads == 0 && sharedReads == 0);
    // Flags override their corresponding saved field, retaining other saved choices.
    assert(Resolve(seed, out, "", -1, 40) == 0);
    assert(out["goal"]["requiredPieces"] == 21 && out["goal"]["totalPieces"] == 40);
    assert(Resolve(seed, out, "", 25) == 0);
    assert(out["goal"]["requiredPieces"] == 25 && out["goal"]["totalPieces"] == 31);
    assert(Resolve(seed, out, "", 0) == 0);
    assert(out["goal"]["type"] == "bosses" && out["goal"]["requiredPieces"] == 0);
    assert(out["goal"]["totalPieces"] == 31);
    for (const auto& bad : {Json{{"requiredPieces", 0}}, Json{{"requiredPieces", 101}},
             Json{{"requiredPieces", -1}}, Json{{"requiredPieces", 1.5}},
             Json{{"totalPieces", -2}}, Json{{"totalPieces", 101}}, Json{{"totalPieces", 20}}}) {
        auto malformed = seed;
        malformed["goal"].update(bad);
        bool rejected = false;
        try {Resolve(malformed, out);} catch (const std::exception&) {rejected = true;}
        assert(rejected);
    }
    assert(Resolve(seed, out, "majora", -1, -1, 0, "all") == 0);
    assert(out["goal"]["type"] == "majora" && out["goal"]["requiredPieces"] == 0);
    assert(out["start"] == 0 && out["shared"] == SharedMaskAll());
    seed["sharedItems"] = SharedKeysFromMask(SharedMaskAll());
    seed["startingGame"] = "OOT";
    assert(Resolve(seed, out, "", -1, -1, 2, "none") == 0);
    assert(out["shared"] == 0 && out["start"] == 2);
    for (const char* goal : {"bosses", "either", "ganon", "majora", "triforceQuest"}) {
        seed["goal"] = {{"type", goal}, {"requiredPieces", 0}, {"totalPieces", 0}};
        assert(Resolve(seed, out) == 0 && out["goal"]["type"] == goal);
    }
    // Standalone native settings and absent legacy fields retain the existing CVar fallback.
    assert(Resolve({{"settings", Json::object()}}, out) == 0);
    assert(out["goal"]["requiredPieces"] == 7 && out["goal"]["totalPieces"] == 12);
    assert(out["start"] == 0 && out["shared"] == SharedMaskAll());
    assert(goalReads == 1 && startReads == 1 && sharedReads == 1);
    assert(Resolve(Json::object(), out, "", 20) == 0);
    assert(out["goal"]["requiredPieces"] == 20 && out["goal"]["totalPieces"] == 20);
    assert(Resolve(Json::object(), out, "", -1, 30) == 0);
    assert(out["goal"]["requiredPieces"] == 7 && out["goal"]["totalPieces"] == 30);
    assert(Resolve(Json::object(), out, "", 0) == 0);
    assert(out["goal"]["type"] == "bosses");
    seed["startingGame"] = "invalid";
    bool rejected = false;
    try {Resolve(seed, out);} catch (const std::exception&) {rejected = true;}
    assert(rejected);
    std::cout << "headless saved settings and explicit overrides passed\n";
}
'''
cpp = work/'headless_settings.cpp'
cpp.write_text(prelude + tests, encoding='utf-8')
exe = hostcxx.build(args.compiler, cpp, includes=[source/'combo', args.json_include])
subprocess.run([str(exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)
