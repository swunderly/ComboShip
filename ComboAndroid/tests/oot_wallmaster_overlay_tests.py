"""Execute the production overlay and age/time traversal with engine stubs, not gameplay evidence.

Checks the generated vanilla and MQ predicates against an independent truth table; uses the real soul
ownership policy, registry, candidate split and age/time propagation. Built twice, with the dev and the
shipping capability sets: a Master Quest context is admitted exactly when the build has the quests
capability. Native object compilation is separate.
"""
from pathlib import Path
import argparse
import hashlib
import json
import re
import subprocess
import sys
import hostcxx
from temple_probe_stubs import function
from oot_wallmaster_stubs import prelude

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--json-include', type=Path, required=True)
p.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
a = p.parse_args()
source, work = a.source.resolve(), a.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
sys.path.insert(0, str(source/'tools/ootmm'))
import export_oot_wallmaster_overlay as tool

fixture = json.loads((source/tool.JSON).read_bytes())
assert len(fixture['edges']) == 22
assert [e['quest'] for e in fixture['edges']] == ['vanilla'] * 11 + ['mq'] * 11
assert fixture['auditSha256'] == hashlib.sha256((source/tool.AUDIT).read_bytes()).hexdigest()
for path, data in tool.outputs(fixture).items():
    assert (source/path).read_bytes() == data and b'\r' not in data, path
reference = source.parents[1]/'references/ootmm-stable-32.3'
if reference.exists():
    assert tool.build(reference) == fixture
    subprocess.run([sys.executable, '-B', str(source/'tools/ootmm/export_oot_wallmaster_overlay.py'),
                    '--reference', str(reference), '--output-root', str(source), '--check'], check=True, timeout=120)

randomizer = source/'soh/soh/Enhancements/randomizer'
overlay = (randomizer/'combo_entrance_overlay.cpp').read_text(encoding='utf-8')
# Engine headers are replaced by the explicit stubs below; generated predicate/arrival includes stay.
overlay = '\n'.join(line for line in overlay.splitlines()
                    if not line.startswith('#include') or '.inc"' in line)
fill = (randomizer/'3drando/fill.cpp').read_text(encoding='utf-8')
entrance = (randomizer/'entrance.cpp').read_text(encoding='utf-8')
process = function(fill, 'static void ProcessComboEntranceOverlay(')
assert hostcxx.contains(function(fill, 'void ProcessExits('),
                        'ProcessComboEntranceOverlay(region, gals, ignore, stopOnBeatable, addToPlaythrough);')
assert 'entranceSphere' not in process and 'region->exits' not in process
# Edge 19 reads two COMBO_BUILD events of the Ganon MQ Spirit sun room, copied from the pinned Sun Room Sun Side
# (ganon_castle_mq.yml:275-276): zombies by Sun's Song, a sword, Din's Fire or the hammer; then the ending suns.
ganon = (randomizer/'location_access/dungeons/ganons_castle.cpp').read_text(encoding='utf-8')
sun_room = ganon[ganon.index('areaTable[RR_GANONS_CASTLE_MQ_SPIRIT_TRIAL_AFTER_SWITCH]'):]
sun_room = sun_room[:sun_room.index('//Locations')]
assert re.sub(r'\s+', ' ', sun_room).strip().endswith(
    '#ifdef COMBO_BUILD //Events //OoTMM ganon_castle_mq.yml:275-276 (Sun Room Sun Side); only the MQ Spirit '
    'Wallmaster edge reads them EVENT_ACCESS(LOGIC_GANON_MQ_SPIRIT_ZOMBIES, logic->CanUse(RG_SUNS_SONG) || '
    'logic->CanUseSword() || logic->CanUse(RG_DINS_FIRE) || logic->CanUse(RG_MEGATON_HAMMER)), '
    'EVENT_ACCESS(LOGIC_GANON_MQ_SPIRIT_ENDING_SUNS, logic->Get(LOGIC_GANON_MQ_SPIRIT_ZOMBIES) && '
    '(logic->SunlightArrows() || (logic->CanUse(RG_FIRE_ARROWS) && logic->CanUse(RG_MIRROR_SHIELD)))), #endif }, {'), \
    sun_room
logic_values = (randomizer/'randomizerEnums/LogicVal.h').read_text(encoding='utf-8')
combo_values = logic_values[logic_values.index('RANDO_ENUM_ITEM(LOGIC_RENEWABLE_RUPEES)'):]
combo_values = combo_values[:combo_values.index('#endif')]  # the COMBO_BUILD-only derived events
assert 'RANDO_ENUM_ITEM(LOGIC_GANON_MQ_SPIRIT_ZOMBIES)\nRANDO_ENUM_ITEM(LOGIC_GANON_MQ_SPIRIT_ENDING_SUNS)\n' in combo_values

main = r'''
int main() {
    namespace CE = ComboRando::Entrances;
    using namespace Rando::ComboEntrances;
    auto ctx = Rando::Context::GetInstance();
    const bool questsActive = CE::ActiveCapabilities().Active(CE::kQuestsWallmasterCapability);
    // The same predicate table must read the probe's private inventory, even when live inventory
    // would permit the route. This compiles the real ActiveLogic implementation above.
    auto liveLogic = logic;
    liveLogic->soulOptions.enabled = false;
    auto probe = std::make_shared<Rando::Logic>();
    logic = probe;
    Rando::ProbeLogicActive() = true;
    CE::Settings settings; settings.erWallmasters = CE::Mode::OwnGame;
    const auto world = CE::BuildCandidateWorld({}, CE::LayoutV1::MakePools(settings, {}, false), settings);
    const auto candidate = CE::EncodeOotOverlay(world);
    assert(SetAssumptions(candidate.c_str()) && CE::ActiveOotUnifiedAssumptions());
    assert(!OverlayExits(RR_GERUDO_TRAINING_GROUND_BOULDER_ROOM)[0].GetConditionsMet());
    struct Expected { RandomizerRegion anchor; int condition; size_t edges; };
    const Expected vanilla[] = {
        {RR_BOTW_MIDDLE, 1, 1}, {RR_FOREST_TEMPLE_NW_HALLWAY_TWISTED, 0, 1},
        {RR_FOREST_TEMPLE_NW_HALLWAY_STRAIGHTENED, 0, 1}, {RR_FOREST_TEMPLE_NE_HALLWAY_STRAIGHTENED, 0, 1},
        {RR_FOREST_TEMPLE_NE_HALLWAY_TWISTED, 0, 1}, {RR_GANONS_CASTLE_LIGHT_TRIAL_FINAL_ROOM, 1, 1},
        {RR_GANONS_CASTLE_SPIRIT_TRIAL_BEFORE_SWITCH, 2, 1}, {RR_GERUDO_TRAINING_GROUND_BOULDER_ROOM, 0, 1},
        {RR_SHADOW_TEMPLE_UPPER_HUGE_PIT, 0, 1}, {RR_SPIRIT_TEMPLE_RUPEE_BRIDGE_NORTH, 0, 1},
        {RR_SPIRIT_TEMPLE_2F_MIRROR_ROOM, 3, 1}
    };
    // The MQ anchors, each edge's own (edges 17 and 18 share the Forest Beth room, with the fifth key).
    const Expected mq[] = {
        {RR_BOTW_MQ_MIDDLE, 0, 1}, {RR_BOTW_MQ_B3, 0, 1}, {RR_BOTW_MQ_FLOORMASTER_ROOM, 0, 1},
        {RR_FOREST_TEMPLE_MQ_STRAIGHT_HALLWAY, 0, 1}, {RR_FOREST_TEMPLE_MQ_BETH_ROOM, 4, 2},
        {RR_GANONS_CASTLE_MQ_SPIRIT_TRIAL_AFTER_SWITCH, 5, 1}, {RR_GERUDO_TRAINING_GROUND_MQ_BOULDER_ROOM, 0, 1},
        {RR_SPIRIT_TEMPLE_MQ_STATUE_ROOM, 0, 1}, {RR_SPIRIT_TEMPLE_MQ_STATUE_ROOM_ADULT, 6, 1},
        {RR_SPIRIT_TEMPLE_MQ_SUN_BLOCK_ROOM, 3, 1}
    };
    // Exhaustive independent truth table: missing/present soul, shuffle off, lens/trick, Bombchu, Mirror
    // Shield, sunlight, bracelet, Forest keys 4 to 7, the Ganon MQ ending suns and Fire Arrows. No bow is
    // needed for Ganon Spirit; four Forest MQ keys give no edge, five and six do.
    size_t cases = 0;
    const auto table = [&](const Expected& row) {
        for (unsigned bits = 0; bits < 4096; ++bits) {
            logic->souls = {};
            const bool owned = (bits & 1) != 0;
            logic->soulOptions.enabled = (bits & 2) != 0;
            Rando::Souls::ChangeGeneralOotSoul(logic->souls,
                Rando::Souls::GeneralOotSoulIndex(Rando::Souls::OotSoul::ENEMY_WALLMASTER), owned);
            logic->lens = (bits & 4) != 0; ctx->trick = (bits & 8) != 0;
            logic->bombchu = (bits & 16) != 0; logic->mirror = (bits & 32) != 0;
            logic->sunlight = (bits & 64) != 0; logic->bracelet = (bits & 128) != 0;
            logic->forestKeys = 4 + int((bits >> 8) & 3);
            logic->endingSuns = (bits & 1024) != 0; logic->fireArrows = (bits & 2048) != 0;
            const bool soul = owned || !logic->soulOptions.enabled;
            const bool gate = row.condition == 0 || (row.condition == 1 && (logic->lens || ctx->trick)) ||
                (row.condition == 2 && logic->bombchu && (logic->mirror || logic->sunlight)) ||
                (row.condition == 3 && (logic->bracelet || logic->sunlight)) ||
                (row.condition == 4 && logic->forestKeys >= 5) || (row.condition == 5 && logic->endingSuns) ||
                (row.condition == 6 && logic->fireArrows && (logic->sunlight || logic->mirror));
            const auto exits = OverlayExits(row.anchor);
            assert(exits.size() == 11 * row.edges);
            std::set<RandomizerRegion> destinations;
            for (const auto& exit : exits) {
                assert(exit.GetConditionsMet() == (soul && gate));
                destinations.insert(const_cast<Entrance&>(exit).GetConnectedRegion()->randomizerRegionKey);
            }
            assert(destinations.size() == 11);
            ++cases;
        }
    };
    for (const auto& row : vanilla)
        table(row);
    // Every age/time combination propagates only from a reached source with its own predicate.
    logic->soulOptions.enabled = false; logic->lens = true; logic->adultOnly = true; ctx->trick = false;
    for (unsigned mask = 0; mask < 16; ++mask) {
        ClearRegions(); processed = 0;
        auto* src = RegionTable(RR_BOTW_MIDDLE);
        src->childDay = (mask & 1) != 0; src->childNight = (mask & 2) != 0;
        src->adultDay = (mask & 4) != 0; src->adultNight = (mask & 8) != 0;
        GetAccessibleLocationsStruct gals;
        ProcessComboEntranceOverlay(src, gals, RG_NONE, false, true);
        for (auto exit : OverlayExits(RR_BOTW_MIDDLE)) {
            const auto* dst = exit.GetConnectedRegion();
            assert(!dst->childDay && !dst->childNight);
            assert(dst->adultDay == src->adultDay && dst->adultNight == src->adultNight);
        }
        assert(src->exits.empty());
        assert(gals.regionPool.size() == ((mask & 12) ? 11u : 0u));
        assert(processed == ((mask & 12) ? 11 : 0));
    }
    logic->adultOnly = false;
    // A Master Quest context is admitted only with the quests capability; a source the quest removes never is.
    ctx->dungeons.rows[0].mq = true; // MQ Deku Tree: every vanilla source still exists
    assert(SetAssumptions(candidate.c_str()) == questsActive);
    assert(OverlayExits(RR_BOTW_MIDDLE).empty() != questsActive);
    ctx->dungeons.rows[0].mq = false;
    ctx->dungeons.rows[7].mq = true; // MQ Shadow Temple has no Wallmaster
    assert(!SetAssumptions(candidate.c_str()) && !CE::ActiveOotUnifiedAssumptions());
    ctx->dungeons.rows[7].mq = false;
    assert(SetAssumptions(candidate.c_str()));
    assert(!SetAssumptions("{}") && !CE::ActiveOotUnifiedAssumptions());
    assert(SetAssumptions(candidate.c_str()));
    ClearAssumptions();
    assert(OverlayExits(RR_BOTW_MIDDLE).empty());
    // Every dungeon Master Quest: ten sources, with every MQ edge's truth table.
    CE::OotQuests allMq{};
    allMq.fill(true);
    for (auto& row : ctx->dungeons.rows)
        row.mq = true;
    const auto mqCandidate = CE::EncodeOotOverlay(
        CE::BuildCandidateWorld({}, CE::LayoutV1::MakePools(settings, allMq, false), settings));
    assert(SetAssumptions(mqCandidate.c_str()) == questsActive);
    if (questsActive) {
        assert(CE::ActiveOotUnifiedAssumptions()->routes.size() == 10);
        for (const auto& row : mq)
            table(row);
        // The vanilla anchors of a source both quests have are unreachable in its MQ dungeon; the probe's
        // factory hands out only the edges of the loaded quest (OotWallmasterEdgeInQuests).
        assert(WallmasterCandidateExits("OOT_WALLMASTER_FOREST_CORRIDOR_EAST", "OOT_ICE_CAVERN").size() == 2);
        assert(WallmasterCandidateExits("OOT_WALLMASTER_SPIRIT_STATUE", "OOT_ICE_CAVERN").front().GetParentRegionKey() ==
               RR_SPIRIT_TEMPLE_MQ_STATUE_ROOM);
        bool removed = false;
        try { WallmasterCandidateExits("OOT_WALLMASTER_SHADOW", "OOT_ICE_CAVERN"); }
        catch (const std::invalid_argument&) { removed = true; }
        assert(removed);
    }
    for (auto& row : ctx->dungeons.rows)
        row.mq = false;
    ClearAssumptions();
    // Inactive traversal is the identity for existing native exits, flags and search state.
    ClearRegions();
    auto* native = RegionTable(RR_BOTW_MIDDLE);
    native->childDay = true; native->adultNight = true; native->addedToPool = true;
    native->exits.emplace_back(RR_DEKU_TREE_ENTRYWAY, [] { return true; }, "native", false);
    const auto* identity = &native->exits.front();
    GetAccessibleLocationsStruct idle;
    ProcessComboEntranceOverlay(native, idle, RG_NONE, false, true);
    assert(native->childDay && !native->childNight && !native->adultDay && native->adultNight && native->addedToPool);
    assert(!idle.logicUpdated && idle.regionPool.empty() && native->exits.size() == 1 && &native->exits.front() == identity);
    assert(SetLayout("{}") && std::string(DumpLayout()) == "{}");
    const auto context = nlohmann::json::parse(AreaContext());
    assert(context.at("ootMq").size() == 12 && context.at("profile") == "ootmm-oot-area-context-v1");
    logic = liveLogic;
    Rando::ProbeLogicActive() = false;
    std::cout << cases << " predicate cases, questsActive " << questsActive << "\n";
    assert(cases == (questsActive ? 21u : 11u) * 4096u);
}
'''
for dev in (True, False):
    cpp = work/f'overlay_{int(dev)}.cpp'
    cpp.write_text(prelude(source, dev) + '\n#include <iostream>\n' + main, encoding='utf-8')
    exe = hostcxx.build(a.compiler, cpp, includes=[source/'combo', a.json_include, randomizer,
                        source/'soh', source/'soh/include'], defines=['_CRT_SECURE_NO_WARNINGS'],
                        msvc=['/bigobj', '/Zc:preprocessor'], exe=work/f'overlay_{int(dev)}.exe')
    subprocess.run([str(exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)
print('PASS production overlay: 11 vanilla and 10 MQ anchors x 4096 predicate cases, 16 age/time cases; native exits '
      'untouched; MQ admitted only with the quests capability')
