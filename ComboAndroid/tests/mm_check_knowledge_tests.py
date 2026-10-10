"""Compile the actual MM check struct/JSON converters without booting the game.

Run from a compiler environment (cl on Windows, c++ elsewhere), providing the
patched checkout and nlohmann include directory. No mirrored serializer is used.
"""
import argparse
from pathlib import Path
import re
import subprocess
import hostcxx

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--json-include', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
args = p.parse_args()
source = args.source.resolve()
out = args.build_dir.resolve()
out.mkdir(parents=True, exist_ok=True)

save = (source/'mm/include/z64save.h').read_text(encoding='utf-8')
struct = re.search(r'typedef struct RandoSaveCheck \{.*?\} RandoSaveCheck;', save, re.S).group()
conversions = (source/'mm/2s2h/BenJsonConversions.hpp').read_text(encoding='utf-8')
conversions = conversions[conversions.index('inline void to_json(json& j, const RandoSaveCheck&'):]
conversions = conversions[:conversions.index('inline void to_json(json& j, const RandoSaveInfo&')]
tracker = (source/'mm/2s2h/Rando/CheckTracker/CheckTracker.cpp').read_text(encoding='utf-8')
record = tracker[tracker.index('void Rando::CheckTracker::RecordSeenItem('):]
record = record[:record.index('\n}\n')+3]
network = (source/'mm/2s2h/Network/Anchor/MMAnchor.cpp').read_text(encoding='utf-8')
pack = network[network.index('    // Byte-reduction:'):network.index('    SendSaveJson(payload);')]
unpack = network[network.index('    // Unpack the compact rando check array'):network.index('    Save loadedData = payload["state"].get<Save>();')]
merge = network[network.index('    // A stale or older client must not erase'):network.index('    // Permanent equipment must not disappear')]
engine_stubs = r'''
#include <array>
struct TestRando { std::array<RandoSaveCheck, RC_MAX> randoSaveChecks{}; };
struct TestShip { TestRando rando; };
struct TestSave { TestShip shipSaveInfo; };
struct TestContext { TestSave save; unsigned fileNum = 0; } gSaveContext;
bool randoEnabled = true, foreground = true;
void* gPlayState = nullptr;
using s32 = int32_t;
using u8 = uint8_t;
#define IS_RANDO randoEnabled
#define RANDO_SAVE_CHECKS gSaveContext.save.shipSaveInfo.rando.randoSaveChecks
#define SPDLOG_WARN(...) ((void)0)
bool Combo_MmIsForeground() { return foreground; }
namespace Rando { namespace CheckTracker { void RecordSeenItem(RandoCheckId, RandoItemId, uint8_t = 1); } }
'''
network_harness = '\njson PackKnowledge() {\njson payload;\npayload["state"]["shipSaveInfo"]["rando"] = json::object();\n' + pack + '\nreturn payload;\n}\n'
network_harness += '\nvoid UnpackKnowledge(json& payload) {\n' + unpack + '\n}\n'
network_harness += r'''
void ApplyKnowledge(json payload) {
    UnpackKnowledge(payload);
    TestSave loadedData;
    payload["state"]["shipSaveInfo"]["rando"]["randoSaveChecks"].get_to(loadedData.shipSaveInfo.rando.randoSaveChecks);
''' + merge + '\n    gSaveContext.save = loadedData;\n}\n'
test = r'''
#include <cassert>
#include <iostream>
int main() {
    json old = {{"randoItemId", RI_HOOKSHOT}, {"eligible", false}, {"cycleObtained", false},
                {"obtained", false}, {"shuffled", true}, {"skipped", false}, {"price", 100}};
    RandoSaveCheck check{};
    check.seenItemId = RI_BOW;
    old.get_to(check);
    assert(check.seenItemId == RI_UNKNOWN); // loading an old/different slot clears previous knowledge
    assert(check.knowledgeSource == 0);
    assert(!json(check).contains("seenItemId")); // never infer knowledge from the seed placement
    for (auto item : {RI_HOOKSHOT, RI_BOMB_BAG_30, RI_COMBO_FOREIGN, RI_NONE}) {
        check.seenItemId = item;
        auto loaded = json::parse(json(check).dump()).get<RandoSaveCheck>();
        assert(loaded.seenItemId == item);
        assert(!loaded.obtained && !loaded.cycleObtained && !loaded.eligible);
        assert(loaded.randoItemId == RI_HOOKSHOT && loaded.price == 100);
    }
    check.randoItemId = RI_TRAP;
    check.seenItemId = RI_HOOKSHOT; // persist the shown disguise, never replace it with RI_TRAP
    auto trapped = json(check).get<RandoSaveCheck>();
    assert(trapped.randoItemId == RI_TRAP && trapped.seenItemId == RI_HOOKSHOT);
    check.skipped = true;
    assert(json(check).get<RandoSaveCheck>().seenItemId == RI_HOOKSHOT);
    for (int invalid : {-1, int(RI_MAX), 999999}) {
        old["seenItemId"] = invalid;
        assert(old.get<RandoSaveCheck>().seenItemId == RI_UNKNOWN);
    }
    // Run the production observer against a minimal engine context, with no pre-learned checks.
    auto& observed = RANDO_SAVE_CHECKS[RC_CLOCK_TOWN_NORTH_TREE_PIECE_OF_HEART];
    observed = old.get<RandoSaveCheck>();
    using Rando::CheckTracker::RecordSeenItem;
    RecordSeenItem(RC_CLOCK_TOWN_NORTH_TREE_PIECE_OF_HEART, RI_HOOKSHOT);
    assert(observed.seenItemId == RI_UNKNOWN); // no live game
    gPlayState = &gSaveContext;
    foreground = false;
    RecordSeenItem(RC_CLOCK_TOWN_NORTH_TREE_PIECE_OF_HEART, RI_HOOKSHOT);
    assert(observed.seenItemId == RI_UNKNOWN); // dormant engine must not learn anything
    foreground = true;
    observed.shuffled = false;
    RecordSeenItem(RC_CLOCK_TOWN_NORTH_TREE_PIECE_OF_HEART, RI_HOOKSHOT);
    assert(observed.seenItemId == RI_UNKNOWN);
    observed.shuffled = true;
    RecordSeenItem(RC_CLOCK_TOWN_NORTH_TREE_PIECE_OF_HEART, RI_TRAP);
    assert(observed.seenItemId == RI_UNKNOWN); // the hidden trap ID is not an observation
    RecordSeenItem(RC_UNKNOWN, RI_HOOKSHOT);
    RecordSeenItem(RC_MAX, RI_HOOKSHOT);
    RecordSeenItem(RC_CLOCK_TOWN_NORTH_TREE_PIECE_OF_HEART, RI_UNKNOWN);
    assert(observed.seenItemId == RI_UNKNOWN);
    RecordSeenItem(RC_CLOCK_TOWN_NORTH_TREE_PIECE_OF_HEART, RI_HOOKSHOT, 2);
    assert(observed.knowledgeSource == 2); // explicit hint is known, not visually seen
    assert(json(observed).get<RandoSaveCheck>().knowledgeSource == 2);
    RecordSeenItem(RC_CLOCK_TOWN_NORTH_TREE_PIECE_OF_HEART, RI_HOOKSHOT);
    assert(observed.seenItemId == RI_HOOKSHOT && !observed.obtained && !observed.eligible);
    assert(observed.knowledgeSource == 1); // actually viewing it upgrades the source
    RecordSeenItem(RC_CLOCK_TOWN_NORTH_TREE_PIECE_OF_HEART, RI_HOOKSHOT, 2);
    assert(observed.knowledgeSource == 1); // a later hint does not downgrade a sighting
    observed.obtained = true;
    RecordSeenItem(RC_CLOCK_TOWN_NORTH_TREE_PIECE_OF_HEART, RI_RUPEE_GREEN);
    assert(observed.seenItemId == RI_HOOKSHOT); // renewable refill does not replace observed reward

    // Exercise the production compact sender, receiver and knowledge merge on old/new snapshots.
    json packet = PackKnowledge();
    auto& rando = packet["state"]["shipSaveInfo"]["rando"];
    for (const auto& row : rando["randoSaveChecksCopy"]) assert(row.size() == 7);
    assert(rando["seenChecks"].size() == 1);
    assert(rando["knowledgeSources"][std::to_string(RC_CLOCK_TOWN_NORTH_TREE_PIECE_OF_HEART)] == 1);
    const auto unseen = RC_CLOCK_TOWN_WEST_BANK_PIECE_OF_HEART;
    rando["seenChecks"][std::to_string(unseen)] = RI_MASK_BUNNY;
    rando["knowledgeSources"][std::to_string(unseen)] = 2;
    ApplyKnowledge(packet);
    assert(RANDO_SAVE_CHECKS[unseen].seenItemId == RI_MASK_BUNNY);
    assert(RANDO_SAVE_CHECKS[unseen].knowledgeSource == 2);
    rando.erase("seenChecks"); // a previous-release peer sends no knowledge table
    rando.erase("knowledgeSources");
    ApplyKnowledge(packet);
    assert(RANDO_SAVE_CHECKS[unseen].seenItemId == RI_MASK_BUNNY);
    assert(RANDO_SAVE_CHECKS[unseen].knowledgeSource == 2);
    assert(RANDO_SAVE_CHECKS[RC_CLOCK_TOWN_NORTH_TREE_PIECE_OF_HEART].seenItemId == RI_HOOKSHOT);
    std::cout << "MM knowledge: legacy saves, persistence, stable disguises, skip independence and invalid IDs passed\n";
    std::cout << "MM observation guards and old/new multiplayer knowledge merges passed\n";
}
'''
cpp = out/'mm_check_knowledge.cpp'
cpp.write_text('#define COMBO_BUILD 1\n#include <cstdint>\n#include <nlohmann/json.hpp>\n'
               '#include "2s2h/Rando/Types.h"\n#include "rando/TrackerKnowledge.h"\n'
               'using u16 = uint16_t;\nusing u8 = uint8_t;\nusing json = nlohmann::json;\n'
               + struct + '\n' + conversions + engine_stubs + record + network_harness + test, encoding='utf-8')
# MMAnchor's compact unpack initializes the 7 networked fields of RandoSaveCheck and
# leaves the knowledge fields to the separate merge (-Wmissing-field-initializers).
exe = hostcxx.build(args.compiler, cpp, std='c++17', werror=True, includes=[source/'mm', source/'combo', args.json_include],
                    gnu=['-Wno-missing-field-initializers'])
subprocess.run([str(exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)
