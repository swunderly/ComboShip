#include "combo_entrance_runtime.h"
#ifdef COMBO_BUILD
#include "global.h"
#include "SeedContext.h"
#include "combo_entrance_overlay.h"
#include "overlays/actors/ovl_En_Wallmas/z_en_wallmas.h"
#include "soh/Enhancements/CrossWarp/FaroresWind.h"
#include "rando/entrances/OotUnifiedLayout.h"
#include "rando/entrances/OotWallmasterSource.h"
#include "rando/entrances/OotNativeArrival.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <spdlog/spdlog.h>

namespace {
namespace CE = ComboRando::Entrances;
CE::OotWallmasterScene NativeWallmasterScene(int scene) {
    switch (scene) {
        case SCENE_FOREST_TEMPLE:
            return CE::OotWallmasterScene::ForestTemple;
        case SCENE_SHADOW_TEMPLE:
            return CE::OotWallmasterScene::ShadowTemple;
        case SCENE_SPIRIT_TEMPLE:
            return CE::OotWallmasterScene::SpiritTemple;
        case SCENE_BOTTOM_OF_THE_WELL:
            return CE::OotWallmasterScene::BottomOfTheWell;
        case SCENE_GERUDO_TRAINING_GROUND:
            return CE::OotWallmasterScene::GerudoTrainingGround;
        case SCENE_INSIDE_GANONS_CASTLE:
            return CE::OotWallmasterScene::InsideGanonsCastle;
        default:
            return CE::OotWallmasterScene::Unknown;
    }
}
bool RuntimeWallmastersActive(PlayState* play) {
    return play && IS_RANDO && CE::ActiveOotUnified().hasOotRoutes() &&
           !CVarGetInteger(CVAR_ENHANCEMENT("RandomizedEnemies"), 0) &&
           !Rando::Context::GetInstance()->GetOption(RSK_SHUFFLE_ENTRANCES) &&
           Rando::ComboEntrances::HasWallmasterLayout();
}

// Full Wallmasters: a route into MM leaves OoT through the cross-game entrance flight (CrossEntrance.inc.h), the port's
// form of OoTMM's override and game switch (oot/actors/En/En_Wallmas.c:48-55, oot/play/play.c:738-756, which saves
// before switching, common/switch.c:75-85). The capture forgets the dungeon record first, as an own-game one does, and
// queues the departure; the native respawn then runs as the transition, and its end leaves for MM (z_play.c
// Combo_OotFinishEntranceDeparture), with the launcher's preflight checked again. A departure refused when it is
// queued, or at the end of that transition, keeps the native respawn, and both are logged (D7).
int OotCrossGameCapture(const CE::Record& source, uint32_t destination) {
    const auto* row = CE::Find(CE::Game::Mm, destination);
    if (!row || !CE::OotForeignDestinationBound(*row)) {
        SPDLOG_WARN("[ComboShip] OoT Wallmaster source {} has no bound MM destination; using native respawn",
                    source.name);
        return 0;
    }
    if (!Combo_OotRequestWallmasterDeparture(destination)) {
        SPDLOG_WARN("[ComboShip] OoT Wallmaster departure to {} refused; using native respawn", row->name);
        return 0;
    }
    SPDLOG_INFO("[ComboShip] OoT Wallmaster source {} departs to {}", source.name, row->name);
    Combo_OotFaroreClearDungeon();
    return 0;
}
} // namespace

extern "C" int Combo_OOTWallmasterTransport(PlayState* play, Actor* actor) {
    if (!play || !actor || actor->params == WMT_SHADOWTAG || !RuntimeWallmastersActive(play)) {
        SPDLOG_DEBUG("[ComboShip] OoT Wallmaster transport inactive or ShadowTag; using native respawn");
        return 0;
    }
    const auto* source =
        CE::OotWallmasterSource(NativeWallmasterScene(play->sceneNum), static_cast<uint8_t>(actor->room));
    if (!source) {
        SPDLOG_WARN("[ComboShip] Unmapped OoT Wallmaster scene {} room {}; using native respawn", play->sceneNum,
                    actor->room);
        return 0;
    }
    if (const auto foreign = CE::ActiveOotUnified().foreignRoutesFrom(source->id); !foreign.empty()) {
        if (foreign.size() != 1 || !CE::ActiveOotUnified().routesFrom(source->id).empty()) {
            SPDLOG_WARN("[ComboShip] OoT Wallmaster source {} has no single committed route; using native respawn",
                        source->name);
            return 0;
        }
        return OotCrossGameCapture(*source, foreign.front().destination);
    }
    const auto routes = CE::ActiveOotUnified().routesFrom(source->id);
    if (routes.size() != 1) {
        SPDLOG_WARN("[ComboShip] OoT Wallmaster source {} has no committed route; using native respawn", source->name);
        return 0;
    }
    const auto* row = CE::Find(CE::Game::Oot, routes.front().destination);
    const auto arrival =
        row && CE::OotUnifiedDestinationBound(row->name) ? CE::ResolveOotNativeArrival(row->id) : std::nullopt;
    if (!arrival || arrival->grotto || arrival->entrance > 0xffffu) {
        SPDLOG_WARN("[ComboShip] OoT Wallmaster arrival has no native binding; using native respawn");
        return 0;
    }
    // OoTMM v32.3 clears its dungeon-entrance respawn first (oot/actors/En/En_Wallmas.c:50). Here that is
    // Cross-Games Farore's Wind's dungeon record, which deaths and voids otherwise return to; the arrival scene
    // records its own. OoT's dungeon savewarp is scene-derived (z_sram.c Sram_OpenSave). Reset the native
    // respawn selector too; Farore's saved warp and the void-out coordinates are intact.
    Combo_OotFaroreClearDungeon();
    gSaveContext.respawnFlag = 0;
    // Already resolved: never pass the arrival through Entrance_OverrideNextIndex a second time.
    play->nextEntranceIndex = static_cast<uint16_t>(arrival->entrance);
    play->transitionTrigger = TRANS_TRIGGER_START;
    play->transitionType = TRANS_TYPE_FADE_BLACK;
    return 1;
}
// Shadow Tag's Wallmaster (WMT_SHADOWTAG) is never shuffled, as in the transport above, so it keeps the native
// cleared-room rule too.
extern "C" int Combo_OOTWallmasterSpawnInClearedRoom(PlayState* play, int actorId, int params) {
    return actorId == ACTOR_EN_WALLMAS && params != WMT_SHADOWTAG && RuntimeWallmastersActive(play);
}
#endif
