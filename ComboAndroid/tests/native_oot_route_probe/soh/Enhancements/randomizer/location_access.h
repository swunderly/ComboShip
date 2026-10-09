#pragma once

// Minimal host adapter for compiling the production Graveyard exit graph.
// Location/event bodies are intentionally omitted: this fixture verifies exits,
// while the full native engine continues to own inventory and check logic.
#include <functional>
#include <initializer_list>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "rando/GameplayRules.h"

namespace Rando {
enum RandomizerRegion {
    RR_THE_GRAVEYARD, RR_GRAVEYARD_SHIELD_GRAVE, RR_GRAVEYARD_COMPOSERS_GRAVE,
    RR_GRAVEYARD_HEART_PIECE_GRAVE, RR_GRAVEYARD_DAMPES_GRAVE, RR_GRAVEYARD_DAMPES_HOUSE,
    RR_KAKARIKO_VILLAGE, RR_GRAVEYARD_WARP_PAD_REGION, RR_GRAVEYARD_SHIELD_GRAVE_BACK,
    RR_KAK_WINDMILL_UPPER, RR_SHADOW_TEMPLE_ENTRYWAY,
};
enum RandomizerGet {
    RG_POWER_BRACELET, RG_ZELDAS_LULLABY, RG_DAMPES_HUT_KEY,
    RG_SONG_OF_TIME, RG_DINS_FIRE, RG_FIRE_ARROWS,
};
enum Scene {
    SCENE_GRAVEYARD, SCENE_GRAVE_WITH_FAIRYS_FOUNTAIN, SCENE_REDEAD_GRAVE,
    SCENE_ROYAL_FAMILYS_TOMB, SCENE_WINDMILL_AND_DAMPES_GRAVE, SCENE_GRAVEKEEPERS_HUT,
};
enum Trick { RT_GY_SHADOW_FIRE_ARROWS };

inline std::map<std::string, int> probeCVars;
struct Logic {
    bool IsChild = false;
    bool IsAdult = false;
    bool AtNight = false;
    std::set<RandomizerGet> usableItems;
    bool CanUse(RandomizerGet item) const { return usableItems.count(item) != 0; }
    bool HasItem(RandomizerGet item) const { return CanUse(item); }
    bool CanPullGravesDuringDay() const { return false; }
    bool CanBreakMudWalls() const { return false; }
    bool CanGroundJump() const { return false; }
    bool CanUnloadShadowDoor() const {
        return probeCVars[ComboRando::LogicRuleKey("gEnhancements.RestoreShadowDoorUnload")] != 0;
    }
};
struct Context {
    bool fireArrowTrick = false;
    bool GetTrickOption(Trick) const { return fireArrowTrick; }
};
inline Logic probeLogic;
inline Context probeContext;
// Match the production pointer interface without allocating a runtime context.
struct HostLogicPointer { Logic* operator->() const {return &probeLogic;} Logic* get() const {return &probeLogic;} };
inline HostLogicPointer nativeRouteLogic;
inline Context* ctx = &probeContext;

struct Exit {
    RandomizerRegion destination;
    std::function<bool()> condition;
};
struct Region {
    std::vector<Exit> exits;
    Region() = default;
    Region(const char*, Scene, std::initializer_list<int>, std::initializer_list<int>,
           std::initializer_list<Exit> outgoing) : exits(outgoing) {}
};
inline std::map<RandomizerRegion, Region> areaTable;
template<class F> bool AnyAgeTime(F condition) { return condition(); }
}

#define EVENT_ACCESS(...) 0
#define LOCATION(...) 0
#define ENTRANCE(target, condition, ...) Rando::Exit{target, []() { return condition; }}

// This existing exit/age fixture intentionally tests legacy mode only.
// Soul-enabled policy and exact gates have their separate production-header tests.
namespace Rando::Souls {
enum class OotSoul { NPC_DAMPE };
inline bool Enabled() {return false;}
inline bool Has(Logic*, OotSoul) {return true;}
}
