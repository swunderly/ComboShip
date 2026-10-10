// Isolated native MM Wallmaster adapter. Source-coupled to OoTMM at the registry pin.
// Copyright (c) 2020-2022 OoTMM Team, MIT; see LICENSE.OoTMM.
#include "ComboEntrancesMM.h"
#ifdef COMBO_BUILD
#include "ComboExport.h"
#include "rando/entrances/Format.h"
#include "rando/entrances/MmOrdinaryGraph.h"
#include "rando/entrances/MmGrottoGraph.h"
#include "rando/entrances/MmMixedGraph.h"
#include "rando/entrances/MmMixedNativeArrival.h"
#include "rando/entrances/MmGrottoNative.h"
#include "rando/entrances/MmMarkedSourceTransition.h"
#include "rando/entrances/NativeArrival.h"
extern "C" int MM_GetEffectiveHumanAge(int fileNum);
extern "C" int MM_CanEntranceArrival(int fileNum, uint32_t logicalId, int age, int crossAge);
extern "C" void Combo_MMFaroreEndCycle(void);
#include <spdlog/spdlog.h>

namespace CE = ComboRando::Entrances;
namespace {
CE::MmMarkedSourceTransition sMmMarkedSource;
CE::MmMixedMarkedTransition sMmMixedSource;
std::string_view SourceForRegion(RandoRegionId region) {
    switch (region) {
        case RR_BENEATH_THE_GRAVEYARD_DAMPE:
            return "MM_WALLMASTER_DAMPE";
        case RR_BENEATH_THE_WELL_FREEZARD_ROOM:
            return "MM_WALLMASTER_BTW_ENTRANCE";
        case RR_BENEATH_THE_WELL_TWO_SPIKED_BARS:
            return "MM_WALLMASTER_BTW_FOUNTAIN";
        case RR_BENEATH_THE_WELL_FOUR_SPIKED_BARS:
            return "MM_WALLMASTER_BTW_EXIT";
        default:
            return {};
    }
}
} // namespace

extern "C" COMBO_EXPORT int MM_ApplyEntranceExpansionForCombo(const char* seedJson) {
    // Every seed bind explicitly clears previous layout and temporary probes.
    sMmMarkedSource.clear();
    sMmMixedSource.clear();
    CE::ActiveMmWallmasters().clear();
    CE::ActiveMmOrdinary().clear();
    CE::ActiveMmGrottos().clear();
    CE::ActiveMmMixed().clear();
    CE::ActiveMmWallmasterAssumptions().reset();
    CE::ActiveMmOrdinaryAssumptions().reset();
    CE::ActiveMmGrottoAssumptions().reset();
    CE::ActiveMmMixedAssumptions().reset();
    try {
        if (!seedJson)
            throw std::invalid_argument("Missing seed JSON");
        const auto seed = nlohmann::json::parse(seedJson);
        CE::ValidateMmMixedSeed(seed);
        if (seed.contains("mixedEntranceExpansion"))
            CE::ActiveMmMixed().install(seed.at("mixedEntranceExpansion"));
        if (seed.contains("ordinaryEntranceExpansion"))
            CE::ActiveMmOrdinary().install(seed.at("ordinaryEntranceExpansion"));
        if (seed.contains("grottoEntranceExpansion"))
            CE::ActiveMmGrottos().install(seed.at("grottoEntranceExpansion"));
        const auto rows = CE::DecodeMmWallmasterSeed(seed);
        if (rows)
            CE::ActiveMmWallmasters().install(*rows);
        return 1;
    } catch (const std::exception& e) {
        CE::ActiveMmOrdinary().clear();
        CE::ActiveMmGrottos().clear();
        CE::ActiveMmMixed().clear();
        CE::ActiveMmWallmasters().clear();
        SPDLOG_ERROR("[ComboShip] Entrance expansion seed rejected: {}", e.what());
        return 0; // caller must reject the seed; runtime is already disabled
    }
}
extern "C" COMBO_EXPORT int MM_SetEntranceExpansionAssumptionsForCombo(const char* candidateJson) {
    CE::ActiveMmWallmasterAssumptions().reset();
    CE::ActiveMmOrdinaryAssumptions().reset();
    CE::ActiveMmGrottoAssumptions().reset();
    CE::ActiveMmMixedAssumptions().reset();
    try {
        const auto value = nlohmann::json::parse(candidateJson ? candidateJson : "null");
        if (value.value("profile", std::string()) == "mm-mixed-coupled-assumptions-v1")
            CE::ActiveMmMixedAssumptions() = CE::DecodeMmMixedAssumptions(value);
        else if (value.value("profile", std::string()) == "mm-ordinary-coupled-assumptions-v1")
            CE::ActiveMmOrdinaryAssumptions() = CE::DecodeMmOrdinaryAssumptions(value);
        else if (value.value("profile", std::string()) == "mm-grotto-coupled-assumptions-v1")
            CE::ActiveMmGrottoAssumptions() = CE::DecodeMmGrottoAssumptions(value);
        else
            CE::ActiveMmWallmasterAssumptions() = CE::DecodeMmWallmasterAssumptions(value);
        return 1;
    } catch (const std::exception& e) {
        SPDLOG_ERROR("[ComboShip] Entrance assumptions rejected: {}", e.what());
        return 0;
    }
}
extern "C" COMBO_EXPORT const char* MM_DumpEntranceExpansionForCombo() {
    static thread_local std::string result;
    nlohmann::json seed = nlohmann::json::object();
    if (CE::ActiveMmWallmasters().active()) {
        std::vector<CE::SavedOverride> rows;
        for (const auto name : CE::kMmWallmasterSources) {
            const auto* src = CE::Find(name);
            const auto id = CE::ActiveMmWallmasters().destination(name);
            const auto* dst = id ? CE::Find(CE::Game::Mm, *id) : nullptr;
            if (!src || !dst)
                return nullptr;
            rows.push_back({ std::string(src->name), std::string(dst->name), src->game, dst->game, src->id, dst->id });
        }
        seed["entranceExpansion"] = CE::EncodeMmWallmasterExtension(rows);
    }
    if (CE::ActiveMmOrdinary().active())
        seed["ordinaryEntranceExpansion"] = CE::ActiveMmOrdinary().json();
    if (CE::ActiveMmGrottos().active())
        seed["grottoEntranceExpansion"] = CE::ActiveMmGrottos().json();
    if (CE::ActiveMmMixed().active())
        seed["mixedEntranceExpansion"] = CE::ActiveMmMixed().json();
    result = seed.dump();
    return result.c_str();
}
extern "C" COMBO_EXPORT void MM_ClearEntranceExpansionAssumptionsForCombo() {
    CE::ActiveMmWallmasterAssumptions().reset();
    CE::ActiveMmOrdinaryAssumptions().reset();
    CE::ActiveMmGrottoAssumptions().reset();
    CE::ActiveMmMixedAssumptions().reset();
}
extern "C" COMBO_EXPORT int MM_EntranceExpansionActive() {
    return CE::ActiveMmWallmasters().active() ? 1 : 0;
}
namespace {
CE::MmPreviousScene GrottoPreviousScene() {
    const auto& ship = gSaveContext.save.shipSaveInfo;
    if (!ship.grottoPreviousSceneValid)
        return CE::MmPreviousScene::Other;
    if (ship.grottoPreviousScene == SCENE_00KEIKOKU)
        return CE::MmPreviousScene::TerminaField;
    if (ship.grottoPreviousScene == SCENE_30GYOSON)
        return CE::MmPreviousScene::GreatBayCoast;
    return CE::MmPreviousScene::Other;
}
bool ResolveGrotto(CE::Key source, PlayState* play) {
    if (!CE::ActiveMmGrottos().containsSource(source))
        return false;
    auto previous = GrottoPreviousScene();
    int32_t flag = gSaveContext.respawnFlag;
    const auto entrance =
        CE::ApplyNativeMmGrotto(CE::ActiveMmGrottos().destination(source), gSaveContext.respawn, previous, flag,
                                CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE));
    // Pinned cow destinations replace previous-scene identity explicitly.
    if (previous == CE::MmPreviousScene::TerminaField) {
        gSaveContext.save.shipSaveInfo.grottoPreviousScene = SCENE_00KEIKOKU;
        gSaveContext.save.shipSaveInfo.grottoPreviousSceneValid = 1;
    }
    if (previous == CE::MmPreviousScene::GreatBayCoast) {
        gSaveContext.save.shipSaveInfo.grottoPreviousScene = SCENE_30GYOSON;
        gSaveContext.save.shipSaveInfo.grottoPreviousSceneValid = 1;
    }
    gSaveContext.respawnFlag = flag;
    play->nextEntrance = entrance;
    return true;
}
bool MixedGrottoSource(uint32_t logicalId) {
    for (const auto& row : CE::kMmGrottoGraphBindings)
        if (row.logicalId == logicalId)
            return true;
    return false;
}
// Returns -1, with nothing written, when the destination cannot be entered.
int ResolveMixed(uint32_t logicalId, int previousScene, PlayState* play) {
    const auto destination = CE::ActiveMmMixed().destination(logicalId);
    const int age = MM_GetEffectiveHumanAge(gSaveContext.fileNum);
    if (age < 0 || !MM_CanEntranceArrival(gSaveContext.fileNum, destination, age, 0)) {
        SPDLOG_ERROR("[ComboShip] Owning MM mixed arrival rejected source {:08x} destination {:08x}", logicalId,
                     destination);
        return -1;
    }
    try {
        auto previous = previousScene == SCENE_00KEIKOKU  ? CE::MmPreviousScene::TerminaField
                        : previousScene == SCENE_30GYOSON ? CE::MmPreviousScene::GreatBayCoast
                                                          : CE::MmPreviousScene::Other;
        const auto prepared = CE::PrepareNativeMmMixedArrival(
            destination, gSaveContext.respawn, previous, gSaveContext.respawnFlag, gSaveContext.nextCutsceneIndex,
            CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_WOODFALL_TEMPLE),
            CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE),
            CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_GREAT_BAY_TEMPLE),
            CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_STONE_TOWER_TEMPLE));
        for (size_t i = 0; i < 4; ++i)
            gSaveContext.respawn[i] = prepared.slots[i];
        auto& ship = gSaveContext.save.shipSaveInfo;
        ship.grottoPreviousScene = previousScene;
        ship.grottoPreviousSceneValid = 1;
        if (prepared.previous == CE::MmPreviousScene::TerminaField)
            ship.grottoPreviousScene = SCENE_00KEIKOKU;
        if (prepared.previous == CE::MmPreviousScene::GreatBayCoast)
            ship.grottoPreviousScene = SCENE_30GYOSON;
        gSaveContext.respawnFlag = prepared.flag;
        gSaveContext.nextCutsceneIndex = prepared.cutscene;
        play->nextEntrance = prepared.entrance;
        return 1;
    } catch (const std::exception& e) {
        SPDLOG_ERROR("[ComboShip] MM mixed typed arrival rejected: {}", e.what());
        return -1;
    }
}
} // namespace
extern "C" int Combo_MMGrottoEnter(PlayState* play) {
    if (!play || !IS_RANDO || (!CE::ActiveMmGrottos().active() && !CE::ActiveMmMixed().active()))
        return 0;
    auto c = CE::ReadNativeMmGrotto(gSaveContext.respawn,
                                    play->sceneId == SCENE_00KEIKOKU  ? CE::MmPreviousScene::TerminaField
                                    : play->sceneId == SCENE_30GYOSON ? CE::MmPreviousScene::GreatBayCoast
                                                                      : CE::MmPreviousScene::Other,
                                    gSaveContext.respawnFlag, false);
    const auto source = CE::CanonicalMmEntrance(play->nextEntrance, c);
    if (!source)
        return 0;
    if (CE::ActiveMmMixed().containsSource(source->id)) {
        // A hole starts its fall transition frames later, after Play_Update has dropped any mark,
        // so grotto sources resolve now, like ResolveGrotto.
        if (MixedGrottoSource(source->id))
            return ResolveMixed(source->id, play->sceneId, play) > 0;
        sMmMixedSource.mark(play, gSaveContext.fileNum, source->id, play->nextEntrance, play->sceneId);
        return 1;
    }
    if (!CE::ActiveMmGrottos().containsSource(*source))
        return 0;
    gSaveContext.save.shipSaveInfo.grottoPreviousScene = play->sceneId;
    gSaveContext.save.shipSaveInfo.grottoPreviousSceneValid = 1;
    return ResolveGrotto(*source, play);
}
extern "C" int Combo_MMGrottoExit(PlayState* play) {
    if (!play || !IS_RANDO || (!CE::ActiveMmGrottos().active() && !CE::ActiveMmMixed().active()) ||
        play->sceneId != SCENE_KAKUSIANA || !gSaveContext.save.shipSaveInfo.grottoPreviousSceneValid)
        return 0;
    const auto c = CE::ReadNativeMmGrotto(gSaveContext.respawn, GrottoPreviousScene(), gSaveContext.respawnFlag, false);
    const auto source = CE::LogicalMmGrottoExit(static_cast<uint8_t>(play->roomCtx.curRoom.num), c);
    // Resolve now: the 0xFFFF exit sentinel must never reach the transition setup.
    if (CE::ActiveMmMixed().containsSource(source.id))
        return ResolveMixed(source.id, gSaveContext.save.shipSaveInfo.grottoPreviousScene, play) > 0;
    return ResolveGrotto(source, play);
}
extern "C" int Combo_MMOrdinaryEntranceActive() {
    return IS_RANDO && (CE::ActiveMmOrdinary().active() || CE::ActiveMmMixed().active());
}
extern "C" int Combo_MMOrdinarySourceConfigured(uint16_t entrance) {
    return IS_RANDO &&
           (CE::ActiveMmOrdinary().containsSource(entrance) || CE::ActiveMmMixed().containsSource(entrance));
}
extern "C" uint16_t Combo_MMOrdinaryExitOverride(uint16_t entrance) {
    // Retain the source while native collision/actor setup and fade selection run.
    // The pinned override lookup happens once, only after the transition completes.
    if (IS_RANDO && CE::ActiveMmMixed().containsSource(entrance))
        sMmMixedSource.mark(gPlayState, gSaveContext.fileNum, CE::MmMixedSourceKey(entrance), entrance,
                            gPlayState ? gPlayState->sceneId : -1);
    else if (IS_RANDO && CE::ActiveMmOrdinary().containsSource(entrance))
        sMmMarkedSource.mark(gPlayState, gSaveContext.fileNum, entrance);
    return entrance;
}
extern "C" void Combo_MMClearOrdinaryTransitionMarker(void) {
    sMmMarkedSource.clear();
    sMmMixedSource.clear();
}
extern "C" int Combo_MMCompleteOrdinaryEntrance(PlayState* play) {
    if (!play)
        return 0;
    if (sMmMixedSource.pending()) {
        const auto source = sMmMixedSource.take(play, gSaveContext.fileNum, play->nextEntrance);
        if (!source || play != gPlayState || !IS_RANDO || gSaveContext.gameMode != GAMEMODE_NORMAL ||
            !CE::ActiveMmMixed().containsSource(source->logicalId))
            return -1;
        return ResolveMixed(source->logicalId, source->previousScene, play);
    }
    const auto source = sMmMarkedSource.take(play, gSaveContext.fileNum, play->nextEntrance);
    if (!source || play != gPlayState || !IS_RANDO || gSaveContext.gameMode != GAMEMODE_NORMAL ||
        !CE::ActiveMmOrdinary().containsSource(*source))
        return 0;
    const uint32_t destination = CE::ActiveMmOrdinary().destination(*source);
    const int age = MM_GetEffectiveHumanAge(gSaveContext.fileNum);
    if (age < 0 || !MM_CanEntranceArrival(gSaveContext.fileNum, destination, age, 0)) {
        SPDLOG_ERROR("[ComboShip] Owning native MM arrival preflight rejected source {:04x} destination {:08x}",
                     *source, destination);
        return -1; // preserve save/respawn/age and reject the scene transition
    }
    int cutscene = gSaveContext.nextCutsceneIndex;
    const auto native = CE::FixMmNativeArrival(destination, CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_WOODFALL_TEMPLE),
                                               CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE),
                                               CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_GREAT_BAY_TEMPLE),
                                               CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_STONE_TOWER_TEMPLE), cutscene);
    if (native > 0xffffu)
        return -1;
    gSaveContext.nextCutsceneIndex = cutscene;
    play->nextEntrance = static_cast<uint16_t>(native);
    return 1;
}
extern "C" int Combo_MMWallmasterTransport(PlayState* play, Actor* actor) {
    if (!play || !actor || !IS_RANDO || !CE::ActiveMmWallmasters().active())
        return 0;
    const auto scene = play->sceneId == SCENE_DANPEI2TEST ? CE::SceneContext::MmDampe
                       : play->sceneId == SCENE_REDEAD    ? CE::SceneContext::MmBeneathWell
                                                          : CE::SceneContext::Unknown;
    const auto arrival = CE::ActiveMmWallmasters().capture(scene, static_cast<uint8_t>(actor->room), CE::OotAge::Child,
                                                           static_cast<CE::MmForm>(gSaveContext.save.playerForm));
    if (!arrival)
        return 0;
    // Pinned EnWallmas_TakePlayerEr clears the dungeon-entrance respawn so the next dungeon records
    // its own ingress. The end-of-cycle reset clears exactly that, in both the local and shared copies.
    Combo_MMFaroreEndCycle();
    // Full alias destination was checked at seed load; this profile has only real MM entrances.
    play->nextEntrance = static_cast<uint16_t>(arrival->destination.id);
    gSaveContext.respawnFlag = 0;
    play->transitionTrigger = TRANS_TRIGGER_START;
    play->transitionType = TRANS_TYPE_FADE_BLACK;
    return 1;
}

namespace Rando::Logic {
// Implemented by the independent pinned ACCESS draft; declarations intentionally
// share its interface instead of inferring clock/song rules in this adapter.
bool NativeMmCanResetTime();
uint64_t FilterNativeMmIngressTimes(s32 entrance, uint64_t arrivalTimes);
bool RecordMmAccess(uint64_t times);

bool ExploreMmWallmasterEntrances(RandoRegionId region, uint64_t sourceTimes, std::set<RandoRegionId>& reachable,
                                  std::unordered_map<RandoRegionId, RegionTimeState>& times) {
    const auto source = SourceForRegion(region);
    if (source.empty() || !sourceTimes)
        return false;
    const auto destinations = CE::MmWallmasterGraphDestinations(source);
    if (destinations.empty())
        return false;
    gCurrentRegionTime = sourceTimes;
    // Pinned source area's edge to its Wallmaster buffer is soul_wallmaster.
    const auto capturedTimes =
        TimeLogic::EvaluateConditionTimes([] { return HaveEnemySoul(ACTOR_EN_WALLMAS); }, sourceTimes);
    const auto capturedFlags = CE::mmFlagResult;
    bool accessChanged = false;
    for (const auto destination : destinations) {
        const auto* record = CE::Find(CE::Game::Mm, destination);
        if (!record)
            continue;
        uint64_t arrivalTimes = capturedTimes;
        if (!record->noSongOfTime) {
            accessChanged |= RecordMmAccess(arrivalTimes);
            arrivalTimes = TimeLogic::EvaluateConditionTimes([] { return NativeMmCanResetTime(); }, arrivalTimes);
        }
        // Native arrival IDs merge OoTMM's dungeon wrapper with interior regions.
        // Apply the wrapper's interior gate even when entrance itself is no-sot.
        arrivalTimes = FilterNativeMmIngressTimes(destination, arrivalTimes);
        if (!arrivalTimes)
            continue;
        const auto target = GetRegionIdFromEntrance(destination);
        const auto targetRegion = Regions.find(target);
        if (target == RR_MAX || targetRegion == Regions.end())
            throw std::runtime_error("MM Wallmaster destination has no native region");
        RegionTimeState incoming{ arrivalTimes, targetRegion->second.canStayOverTime };
        incoming.regionalFlags = capturedFlags;
        const auto previous = times.find(target);
        if (previous != times.end() && TimeStateCovers(previous->second, incoming))
            continue;
        reachable.insert(target);
        times[target] = MergeTimeStates(times[target], incoming);
        FindReachableRegions(target, reachable, times);
    }
    return accessChanged;
}
} // namespace Rando::Logic
#endif
