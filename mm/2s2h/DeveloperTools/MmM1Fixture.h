#pragma once
// Included only by DebugConsole.cpp in an explicitly marked entrance DEV build.
#if defined(COMBO_BUILD) && defined(COMBO_ENTRANCE_DEV)
#include "2s2h/Rando/Rando.h"
#include "2s2h/Rando/Logic/ComboEntrancesMM.h"
#include "rando/entrances/MmFixturePolicy.h"
#include "rando/entrances/MmUnifiedLayout.h"

namespace M1MmFixture {
namespace CE = ComboRando::Entrances;
inline bool Ready() {
    const auto* settings = CE::ActiveMmUnified().settings();
    const bool stable = gPlayState && GET_PLAYER(gPlayState) && gPlayState->transitionTrigger == TRANS_TRIGGER_OFF &&
                        gPlayState->pauseCtx.state == PAUSE_STATE_OFF && gPlayState->csCtx.state == CS_STATE_IDLE;
    return CE::Fixture::CanUse(gSaveContext.fileNum, gSaveContext.gameMode == GAMEMODE_NORMAL, IS_RANDO,
                               settings && settings->erWallmasters != CE::Mode::None, stable);
}
inline bool Reply(std::string* output, const nlohmann::json& value) {
    *output = value.dump();
    INFO_MESSAGE("%s", output->c_str());
    return 0;
}
inline bool Status(std::shared_ptr<Ship::Console>, const std::vector<std::string>& args, std::string* output) {
    if (args.size() != 1 || !Ready()) {
        ERROR_MESSAGE("M1 fixture requires stable normal MM play, unified Wallmasters and disposable File3.");
        return 1;
    }
    const auto* player = GET_PLAYER(gPlayState);
    const int room = gPlayState->roomCtx.curRoom.num;
    int hands = 0, spawners = 0, visited = 0;
    for (int category = 0; category < ACTORCAT_MAX; ++category) {
        for (const Actor* actor = gPlayState->actorCtx.actorLists[category].first; actor; actor = actor->next) {
            if (++visited > 4096) {
                ERROR_MESSAGE("M1 fixture actor-list bound exceeded.");
                return 1;
            }
            hands += actor->id == ACTOR_EN_WALLMAS;
            spawners += actor->id == ACTOR_EN_ENCOUNT1;
        }
    }
    return Reply(output,
                 { { "diagnostic", "m1-mm-file3-status-v1" },
                   { "fileNum", gSaveContext.fileNum },
                   { "sceneId", gPlayState->sceneId },
                   { "room", room },
                   { "entrance", gSaveContext.save.entrance },
                   { "position", { player->actor.world.pos.x, player->actor.world.pos.y, player->actor.world.pos.z } },
                   { "floorHeight", player->actor.floorHeight },
                   { "grounded", (player->actor.bgCheckFlags & BGCHECKFLAG_GROUND) != 0 },
                   { "roomClear", room >= 0 && room < 32 && Flags_GetClear(gPlayState, room) != 0 },
                   { "wallmasterSoulOwned", Flags_GetRandoInf(RANDO_INF_OBTAINED_SOUL_OF_ENEMY_WALLMASTERS) != 0 },
                   { "enemySoulsEnabled", RANDO_SAVE_OPTIONS[RO_SHUFFLE_ENEMY_SOULS] == RO_GENERIC_YES },
                   { "wallmasterSpawnAllowed", Combo_MMWallmasterSoulSpawnAllowed(ACTOR_EN_WALLMAS) != 0 },
                   { "liveWallmasters", hands },
                   { "liveEncount1", spawners },
                   { "day", gSaveContext.save.day },
                   { "time", gSaveContext.save.time } });
}
inline bool WellRoom(std::shared_ptr<Ship::Console>, const std::vector<std::string>& args, std::string* output) {
    const auto* room = args.size() >= 2 ? CE::Fixture::FindWellRoom(args[1]) : nullptr;
    if (!Ready() || !room || args.size() != 3 || (args[2] != "clear" && args[2] != "unclear")) {
        ERROR_MESSAGE("Usage: m1_well_room 1|5|7 clear|unclear (stable unified MM File3 only).");
        return 1;
    }
    const bool clear = args[2] == "clear";
    const uint32_t bit = uint32_t{ 1 } << room->room;
    auto& flags = gSaveContext.cycleSceneFlags[SCENE_REDEAD].clearedRoom;
    flags = clear ? flags | bit : flags & ~bit;
    if (gPlayState->sceneId == SCENE_REDEAD) {
        if (clear)
            Flags_SetClear(gPlayState, room->room);
        else
            Flags_UnsetClear(gPlayState, room->room);
    }
    // The in-play branch of the existing native WarpPoint::Warp, without any
    // boot/new-save path, configuration file, actor injection or item grant.
    constexpr uint16_t entrance = 0x9000; // native Beneath the Well entrance0
    gPlayState->nextEntrance = entrance;
    gPlayState->transitionTrigger = TRANS_TRIGGER_START;
    gPlayState->transitionType = TRANS_TYPE_INSTANT;
    auto& respawn = gSaveContext.respawn[RESPAWN_MODE_DOWN];
    respawn.entrance = entrance;
    respawn.roomIndex = room->room;
    respawn.pos = { float(room->x), 40.0f, float(room->z) };
    respawn.yaw = 0;
    respawn.playerParams = PLAYER_PARAMS(0xFF, PLAYER_START_MODE_D);
    gSaveContext.nextTransitionType = TRANS_TYPE_FADE_BLACK_FAST;
    gSaveContext.respawnFlag = -8;
    return Reply(output, { { "diagnostic", "m1-mm-file3-well-respawn-v1" },
                           { "room", room->room },
                           { "requestedRoomClear", clear },
                           { "requestedPosition", { room->x, 40, room->z } },
                           { "groundingVerified", false },
                           { "actorInjection", false } });
}
} // namespace M1MmFixture
#endif
