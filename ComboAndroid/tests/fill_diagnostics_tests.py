"""Real combined fill: passive tracing preserves results, query counts and RNG behavior."""
import argparse
from pathlib import Path
import subprocess
import hostcxx

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--json-include', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
args = parser.parse_args()
work = args.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
code = r'''
#include "rando/CrossWorldRando.h"
#include <cassert>
using namespace ComboRando;
using Json = nlohmann::json;
static bool blocked;
static int queries;
static void reset() {}
static void grant(const char*) {}
static void place(const char*, const char*) {}
static uint8_t portal() { return 1; }
static const char* ootReach() {
    ++queries;
    static std::string out;
    Json checks = {"Oot start", "Ganon"};
    if (!blocked) checks.push_back("Time lock");
    out = checks.dump(); return out.c_str();
}
static const char* mmReach() {
    ++queries;
    return "[\"Mm start\",\"Defeat Majora\"]";
}
static Json dump(const char* check) {
    return {{"checks", Json::array({{{"name", check}}})},
            {"pool", Json::array({{{"name", "Filler"}, {"advancement", false}, {"category", "junk"}}})}};
}
int main() {
    OracleFns oot{reset, grant, ootReach, place, portal};
    OracleFns mm{reset, grant, mmReach, place};
    auto od = dump("Oot start");
    od["fixed"] = Json::array({{{"check", "Time lock"}, {"item", "Song of Time"}, {"advancement", true}}});
    const auto md = dump("Mm start");
    for (const uint32_t seed : {123u, 322059408u, 2976495177u}) {
        for (const bool missing : {false, true}) {
            blocked = missing;
            queries = 0;
            auto normal = CrossWorldCombinedFill(od.dump(), md.dump(), seed, oot, mm, nullptr, "",
                                                OotAccess::ALL_REACHABLE, {}, GAME_MM, 0);
            const int ordinaryQueries = queries;
            queries = 0;
            int callbacks = 0;
#ifdef COMBO_RANDO_DIAGNOSTICS
            std::function<void(const Json&)> sink = [&](const Json& trace) {
                ++callbacks;
                assert(trace["masterSeed"] == seed && trace["needRetry"] == missing);
                assert(trace["ootQueryCount"].get<int>() + trace["mmQueryCount"].get<int>() == queries);
                const auto& rows = trace.at("placements");
                auto locked = std::find_if(rows.begin(), rows.end(), [](const Json& row) {
                    return row.at("check") == "Time lock";
                });
                assert(locked != rows.end() && locked->at("locked") == true);
                assert(locked->at("reachable") == !missing && locked->at("item") == "Song of Time");
                assert(trace.at("ootOwned").is_array() && trace.at("mmOwned").is_array());
                if (missing) assert(locked->at("sphere").is_null());
                else assert(locked->at("sphere").is_number_integer());
            };
#endif
            auto traced = CrossWorldCombinedFill(od.dump(), md.dump(), seed, oot, mm, nullptr, "",
                                                OotAccess::ALL_REACHABLE, {}, GAME_MM, 0
#ifdef COMBO_RANDO_DIAGNOSTICS
                                                , sink
#endif
                                                );
            assert(normal.success == !missing && normal.success == traced.success);
            assert(normal.error == traced.error && normal.spoilerJson == traced.spoilerJson);
            assert(queries == ordinaryQueries);
#ifdef COMBO_RANDO_DIAGNOSTICS
            assert(callbacks == (missing ? kMaxPasses : 1));
#else
            assert(callbacks == 0);
#endif
        }
    }
    std::cout << "PASS: six real-fill cases preserve byte results/query counts, with native-format locked rows\n";
}
'''
for enabled in (False, True):
    cpp = work / ('fill_diagnostics_on.cpp' if enabled else 'fill_diagnostics_off.cpp')
    cpp.write_text(code, encoding='utf-8')
    exe = hostcxx.build(args.compiler, cpp, includes=[args.source / 'combo', args.json_include],
                        defines=['COMBO_RANDO_DIAGNOSTICS=1'] if enabled else [])
    subprocess.run([str(exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)
