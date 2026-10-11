"""Execute the solver and native arrival branches against pinned v32.3 contracts."""
from pathlib import Path
import argparse
import json
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
fixture = json.loads((source/'ComboAndroid/tests/fixtures/ootmm-32.3-solver-contract.json').read_text())
shuffle = (source/'combo/rando/entrances/Shuffle.h').read_text(encoding='utf-8')
# Expose the actual recursive placement's counter solely to verify its backtrack boundary.
probe = 'struct ProbeShuffler {' + shuffle.split('class Shuffler {', 1)[1].split(
    '// The MM-only reservation', 1)[0].replace('Shuffler(', 'ProbeShuffler(')


def block(text, marker):
    start = text.index(marker)
    opening = text.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]


logic = (source/'mm/2s2h/Rando/Logic/Logic.cpp').read_text(encoding='utf-8')
wallmaster = (source/'mm/2s2h/Rando/Logic/ComboEntrancesMM.cpp').read_text(encoding='utf-8')
ordinary_gate = block(logic[logic.index('if (mixedOverride || ordinaryOverride || grottoOverride)'):],
                      'if (!record->noSongOfTime)')
wallmaster_gate = block(wallmaster[wallmaster.index('bool ExploreMmWallmasterEntrances'):],
                       'if (!record->noSongOfTime)')
global_start = hostcxx.find(logic, 'if (currentRegion == RR_MAX) RecordMmAccess(currentTime);')
global_access = logic[global_start:logic.index(';', global_start)+1]
# Real caller wiring: all private generators must receive the already chosen combined policy.
for filename in ('ComboShip.cpp', 'ComboRandoHeadless.cpp'):
    text = (source/'combo'/filename).read_text(encoding='utf-8')
    for name in ('Wallmasters', 'Ordinary', 'Grottos', 'Mixed'):
        call = text.split('GenerateNativeMm'+name+'(', 1)[1].split(';', 1)[0]
        assert 'ootAccess' in call, filename + ': missing access policy at ' + name
for filename in ('NativeBridge.h', 'MmOrdinaryNativeBridge.h', 'MmGrottoNativeBridge.h', 'MmMixedNativeBridge.h'):
    text = (source/'combo/rando/entrances'/filename).read_text(encoding='utf-8')
    assert text.count('goal, access)') == 2, filename + ': assumed/complete policy mismatch'

prelude = r'''
#define NOMINMAX
#include <cassert>
#include <iostream>
#include "rando/entrances/MmMixedNativeBridge.h"
using namespace ComboRando;
using namespace ComboRando::Entrances;
using Json = nlohmann::json;
'''
prelude += 'namespace ComboRando::Entrances {\n' + probe + '\n}\n'
for key, values in fixture['polarity'].items():
    enums = ', '.join('Type::'+''.join(part.capitalize() for part in value.split('-')) for value in values)
    prelude += 'const Types reference_'+key+' = {'+enums+'};\n'
prelude += f'constexpr int referenceBacktracks = {fixture["backtracks"]}, referenceAttempts = {fixture["attempts"]};\n'
prelude += r'''
bool accessEvent = false;
int accessWrites = 0, resetChecks = 0;
bool RecordMmAccess(uint64_t times) {++accessWrites; if (times) accessEvent = true; return accessEvent;}
bool NativeMmCanResetTime() {++resetChecks; return accessEvent;}
namespace TimeLogic {
template<class F> uint64_t EvaluateConditionTimes(F predicate, uint64_t times) {return predicate() ? times : 0;}
}
'''
for name, branch in [('OrdinaryArrival', ordinary_gate), ('WallmasterArrival', wallmaster_gate)]:
    prelude += ('uint64_t '+name+'(bool noSot) {Record storage{}; storage.noSongOfTime = noSot; '
                'const auto* record = &storage; uint64_t arrivalTimes = 4; bool accessChanged = false;\n'
                + branch + '\nreturn arrivalTimes;}\n')
prelude += 'void GlobalAccess(int currentRegion) {const int RR_MAX = 99; uint64_t currentTime = 4;\n'+global_access+'\n}\n'
tests = r'''
template<class F> bool Throws(F f) {try {f();} catch (const std::exception&) {return true;} return false;}
Bindings BaseBindings() {
    Bindings b;
    b.areaExists = [](std::string_view) {return true;};
    b.sourceSupported = b.destinationSupported = [](const Record&) {return true;};
    b.validateAssumed = [](const Overrides&, const Pools&) {return true;};
    b.validateComplete = [](const Overrides&) {return true;};
    b.chooseIndex = [](size_t) {return size_t(0);};
    return b;
}
Bindings SmallBindings() {
    auto b = BaseBindings();
    std::set<std::string_view> areas;
    for (auto name : {"MM_WALLMASTER_DAMPE", "MM_TEMPLE_WOODFALL", "MM_WOODFALL_FROM_TEMPLE"}) {
        auto r = Find(name); assert(r); areas.insert(r->from); areas.insert(r->to);
    }
    b.areaExists = [areas](std::string_view area) {return areas.contains(area);};
    return b;
}
void PoolAndPolarity() {
    Settings off;
    int randomCalls = 0;
    Bindings noBindings;
    noBindings.chooseIndex = [&](size_t) {++randomCalls; return size_t(0);};
    assert(Shuffler(off, noBindings).run().empty() && randomCalls == 0);
    for (bool overworld : {false, true}) {
        Settings s; if (overworld) s.erOverworld = Mode::OwnGame;
        for (const auto& row : kRegistry) {
            auto expected = reference_IN.contains(row.type) ? Polarity::In :
                            reference_OUT.contains(row.type) ? Polarity::Out : Polarity::Any;
            if (overworld && reference_ANY_OVERWORLD.contains(row.type)) expected = Polarity::Any;
            assert(EntrancePolarity(row, s) == expected);
        }
    }
    Settings s; s.erIndoors = Mode::Full; s.erIndoorsTelescopes = true;
    s.erSpawns = Spawns::Both; s.erWallmasters = Mode::Full;
    auto pools = MakePools(s);
    for (const auto& pool : pools)
        if (pool.name == "SPAWNS" || pool.name == "WALLMASTERS")
            assert(pool.dst.contains(Type::IndoorsTelescope));
    const Record* telescope = nullptr;
    for (const auto& row : kRegistry) if (row.type == Type::IndoorsTelescope) {telescope = &row; break;}
    assert(telescope);
    auto src = *Find("MM_WALLMASTER_DAMPE");
    s.erSelfLoops = true;
    assert(Assignable(src, *telescope, false, s));
    auto foreign = *telescope; foreign.game = ComboRando::Entrances::Game::Oot;
    assert(!Assignable(src, foreign, true, s));
}
void JointPoolsAndBounds() {
    Settings s; s.games = Games::Mm; s.erWallmasters = Mode::OwnGame;
    s.erDungeons = Mode::OwnGame; s.erMajorDungeons = true; s.erSelfLoops = true;
    // Choosing either remaining pool first must be honored; there is no sequential per-kind pass.
    for (size_t initial : {size_t(0), size_t(1)}) {
        auto b = SmallBindings(); int random = 0, probes = 0;
        b.chooseIndex = [&](size_t count) {if (random++ == 0) {assert(count == 2); return initial;} return size_t(0);};
        b.validateAssumed = [&](const Overrides& rows, const Pools&) {
            if (probes++ == 0) assert(rows.contains("MM_WALLMASTER_DAMPE") == (initial == 1));
            return true;
        };
        assert(!Shuffler(s, b).run().empty() && probes >= 2);
    }
    s.erDungeons = Mode::None;
    auto b = SmallBindings(); int rejected = 0;
    b.validateAssumed = [&](const Overrides&, const Pools&) {++rejected; return false;};
    assert(Throws([&] {Shuffler(s, b).run();}));
    assert(rejected == referenceAttempts); // one assignable destination in this tiny graph
    int completed = 0;
    b.validateAssumed = [](const Overrides&, const Pools&) {return true;};
    b.validateComplete = [&](const Overrides&) {++completed; return false;};
    assert(Throws([&] {Shuffler(s, b).run();}) && completed == referenceAttempts);
    // Use the actual recursive implementation, exposing only its private counter for this test.
    b = BaseBindings();
    b.validateAssumed = [](const Overrides& rows, const Pools&) {
        int placed = 0; for (auto name : kMmWallmasterSources) placed += rows.contains(name);
        return placed < 4;
    };
    ProbeShuffler probe(s, b); Overrides result;
    assert(Throws([&] {probe.place(probe.makePools(), {}, result);}));
    assert(probe.backtracks == referenceBacktracks);
}
void OppositeAgeContract() {
    Settings s; s.games = Games::Oot; s.erSpawns = Spawns::Adult; s.erSelfLoops = true;
    auto b = BaseBindings();
    // Active OoT logic must fail before sampling if the native graph adapter is unavailable.
    int random = 0; b.chooseIndex = [&](size_t) {++random; return size_t(0);};
    assert(Throws([&] {Shuffler(s, b);}) && random == 0);
    std::set<std::string_view> areas;
    for (auto name : {"OOT_SPAWN_ADULT", "OOT_HOUSE_MIDO"}) {
        auto r = Find(name); assert(r); areas.insert(r->from); areas.insert(r->to);
    }
    b.areaExists = [areas](std::string_view area) {return areas.contains(area);};
    const std::set<std::string> allItems{"Door Key"};
    for (bool startingAdult : {false, true}) {
        std::set<std::string> startingItems;
        int ageCalls = 0;
        b.validateAssumed = [&](const Overrides&, const Pools&) {return allItems.contains("Door Key");};
        b.validateAgeTemple = [&](const Overrides& rows, const Pools& pools) {
            ++ageCalls;
            assert(rows.contains("OOT_SPAWN_ADULT") && !pools.empty());
            // Fixture graph: only the opposite spawn is rooted; reaching ToT requires a starting key.
            const int rootedAge = startingAdult ? 0 : 1;
            const std::set<int> reachedAges = startingItems.contains("Door Key") ? std::set<int>{rootedAge} : std::set<int>{};
            return reachedAges.contains(rootedAge);
        };
        assert(Throws([&] {Shuffler(s, b).run();}) && ageCalls > 0);
        startingItems.insert("Door Key"); ageCalls = 0;
        assert(!Shuffler(s, b).run().empty() && ageCalls > 0);
    }
    b.logicEnabled = false;
    b.validateAssumed = [](const Overrides&, const Pools&) -> bool {assert(false); return false;};
    b.validateAgeTemple = {};
    assert(!Shuffler(s, b).run().empty());
    s.games = Games::Mm; s.erSpawns = Spawns::None; s.erWallmasters = Mode::OwnGame;
    b = SmallBindings(); // MM-only requires no opposite-age OoT probe.
    assert(!Shuffler(s, b).run().empty());
}
struct Native {
    static inline Json seed = Json::object();
    static inline std::string dump;
    static int apply(const char* text) {seed = Json::parse(text); return 1;}
    static int assume(const char*) {return 1;}
    static void clear() {}
    static const char* snapshot() {dump = seed.dump(); return dump.c_str();}
    static void reset() {}
    static void owned(const char*) {}
    static const char* ootChecks() {return "[\"Ganon\"]";}
    static const char* mmChecks() {return "[\"Defeat Majora\"]";}
    static const char* none() {return "[]";}
    static uint8_t portal() {return 1;}
};
void Accessibility() {
    auto dump = Json{{"pool", Json::array()}, {"checks", {{{"name", "Optional check"}}}}}.dump();
    OracleFns oot{Native::reset, Native::owned, Native::ootChecks, nullptr, Native::portal};
    OracleFns mm{Native::reset, Native::owned, Native::mmChecks, nullptr};
    AssumedWorld world(dump, dump, "");
    assert(!world.validate(oot, mm, false, {}, OotAccess::ALL_REACHABLE));
    assert(world.validate(oot, mm, false, {}, OotAccess::BEATABLE_ONLY));
    auto missingGoal = mm; missingGoal.GetReachableChecks = Native::none;
    assert(!world.validate(oot, missingGoal, false, {}, OotAccess::BEATABLE_ONLY));
    assert(world.validate({}, {}, false, {}, OotAccess::NO_LOGIC));
    NativeFns native{Native::apply, Native::assume, Native::clear, Native::snapshot};
    auto seed = GenerateNativeMmWallmasters(true, 21, native, dump, dump, "", oot, mm, false, {}, OotAccess::BEATABLE_ONLY);
    assert(seed["overrides"].size() == 4);
    assert(GenerateNativeMmWallmasters(false, 21, {}, "", "", "", {}, {}, false, {}).is_null());
}
void ArrivalAccess() {
    for (auto arrival : {OrdinaryArrival, WallmasterArrival}) {
        accessEvent = false; accessWrites = resetChecks = 0;
        assert(arrival(false) == 0 && accessWrites == 0 && resetChecks == 1);
        assert(arrival(true) == 4 && accessWrites == 0 && resetChecks == 1);
        GlobalAccess(12); assert(!accessEvent && accessWrites == 0);
        GlobalAccess(99); assert(accessEvent && accessWrites == 1);
        assert(arrival(false) == 4 && accessWrites == 1 && resetChecks == 2);
    }
}
int main() {
    PoolAndPolarity(); JointPoolsAndBounds(); OppositeAgeContract(); Accessibility(); ArrivalAccess();
    std::cout << "pinned solver policy, hooks, search bounds and MM ACCESS branches passed\n";
}
'''
cpp = work/'entrance_solver_parity.cpp'
cpp.write_text(prelude+tests, encoding='utf-8')
exe = hostcxx.build(args.compiler, cpp, includes=[source/'combo', args.json_include])
subprocess.run([str(exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)
