#include "TempleProbe.h"

#ifdef COMBO_BUILD
// Generation-only opposite-age Temple probe: the native side of OoTMM's validateAgeTemple
// (413573d packages/logic/src/solver/entrances.ts:265-286) for one candidate world.
//
// A private Logic holds the starting items, events and age/time scratch state; the only writes
// outside it are the ::logic handle and every region's flags, both restored on every exit path
// (TempleProbeCore.h). It never resets, searches, places, collects or draws randomness through the
// shared engine, and it only runs between oracle queries of a serialized generation. Supported
// envelope: the bound OoT one-way rows and the verified Wallmaster predicates of the loaded quests, no
// large pool, glitchless logic, no song-destination shuffle logic and no starting warp songs; foreign-song
// logic changes nothing it reads (lemma S1). Anything else is Unsupported.

#include "TempleProbeCore.h"
#include "location_access.h"
#include "entrance.h"
#include "static_data.h"
#include "3drando/starting_inventory.hpp"
#include "rando/SharedItems.h"
#include "rando/SongTravel.h"
#include "rando/entrances/Registry.h"
#include "rando/entrances/OotWallmasterSourceNames.h"
#include "combo_entrance_overlay.h"
#include "variables.h"

#include <optional>

extern "C" {
extern PlayState* gPlayState;
}

namespace {
using namespace Rando::TempleProbe;
using Rando::Entrance;

enum class TempleBindingRole : uint8_t { Candidate, RootPolicy };
struct TempleBinding {
    std::string_view name;
    uint32_t id;
    TempleBindingRole role;
    RandomizerRegion parent, target;
    int16_t index;
};
constexpr TempleBinding kBindings[] = {
#include "combo_oot_edge_bindings.inc"
};

struct Prepared {
    uint32_t epoch = 0;
    bool startingChild = false;
    uint16_t medigoron = 0, carpet = 0; // read by event predicates (goron_city.cpp, haunted_wasteland.cpp)
    std::vector<RandomizerGet> foreign;
};
std::optional<Prepared> sPrepared;
std::string sLastError;

// The domain of SharedStartingInventories(...).first (SharedItems.h): singleton family OoT names and
// the explicit ocarina tiers. No warp song and no progressive placeholder is in it.
bool IsSharedStarterName(const std::string& name) {
    if (name == "Fairy Ocarina" || name == "Ocarina of Time") {
        return true;
    }
    for (int i = ComboRando::SF_OCARINA_BUTTON_A; i < ComboRando::SF_COUNT; ++i) {
        if (i != ComboRando::SF_OCARINA && name == ComboRando::SharedFamilyByIndex(i).ootName) {
            return true;
        }
    }
    return false;
}

bool IsWarpSong(RandomizerGet rg) {
    return rg == RG_MINUET_OF_FOREST || rg == RG_BOLERO_OF_FIRE || rg == RG_SERENADE_OF_WATER ||
           rg == RG_NOCTURNE_OF_SHADOW || rg == RG_REQUIEM_OF_SPIRIT || rg == RG_PRELUDE_OF_LIGHT;
}

const TempleBinding& Binding(const std::string& name, TempleBindingRole role) {
    for (const auto& b : kBindings) {
        if (b.name == name && b.role == role) {
            return b;
        }
    }
    throw Failure(kUnsupported, "no native binding for " + name);
}

// The one native exit carrying a bound row: original and current target both match, unshuffled.
const Entrance& BoundExit(RandomizerRegion parent, RandomizerRegion target) {
    const Entrance* found = nullptr;
    for (const auto& exit : areaTable[parent].exits) {
        if (exit.GetOriginalConnectedRegionKey() == target && exit.GetConnectedRegionKey() == target) {
            if (found != nullptr) {
                throw Failure(kUnsupported, "binding matches several native exits");
            }
            found = &exit;
        }
    }
    if (found == nullptr || found->IsShuffled()) {
        throw Failure(kUnsupported, "binding does not match one vanilla native exit");
    }
    return *found;
}

uint16_t Price(RandomizerCheck check) {
    return Rando::Context::GetInstance()->GetItemLocation(check)->GetPrice();
}

// Starting items once (pathfind.ts:98-109): the native StartingInventory as Combo_SOH_Rando_Reset
// grants it (hearts skipped, starting_inventory.cpp:187-194), then each foreign shared starter not
// already held, with Combo_SOH_Rando_SetOwnedItems' tier-aware ocarina. Called under the probe scope,
// so every Item::ApplyEffect lands on the private Logic.
void ApplyProbeStartingItems(Rando::Logic& probe, const std::vector<RandomizerGet>& foreign) {
    ApplyStartingInventory();
    for (const RandomizerGet rg : foreign) {
        if (rg == RG_FAIRY_OCARINA || rg == RG_OCARINA_OF_TIME) {
            auto& progressive = Rando::StaticData::RetrieveItem(RG_PROGRESSIVE_OCARINA);
            if (!probe.HasItem(RG_FAIRY_OCARINA)) {
                progressive.ApplyEffect();
            }
            if (rg == RG_OCARINA_OF_TIME && !probe.HasItem(RG_OCARINA_OF_TIME)) {
                progressive.ApplyEffect();
            }
            continue;
        }
        if (!probe.HasItem(rg)) {
            Rando::StaticData::RetrieveItem(rg).ApplyEffect();
        }
    }
}

// Frees the SaveContext Logic::Reset(true) allocated; Logic itself never owns it.
struct ProbeLogicDeleter {
    void operator()(Rando::Logic* probe) const {
        if (probe->mSaveContext != nullptr && probe->mSaveContext != &gSaveContext) {
            delete probe->mSaveContext;
        }
        delete probe;
    }
};

struct NativePolicy {
    std::string_view Registry() const {
        return ComboRando::Entrances::kRegistrySourceSha256;
    }
    std::array<Region, RR_MAX>& Regions() const {
        return areaTable;
    }
    std::shared_ptr<Rando::Logic>& Handle() const {
        return logic;
    }
    bool& Scope() const {
        return Rando::ProbeLogicActive();
    }
    size_t Root() const {
        return RR_ROOT;
    }
    size_t Target() const {
        return RR_TEMPLE_OF_TIME;
    }
    bool StartingChild() const {
        return sPrepared->startingChild;
    }
    bool SameLogic() const {
        return logic.get() == Rando::Context::GetInstance()->GetLogic().get();
    }

    // P1 and P4-P7, shared by Prepare and Query. Reads only.
    static void CheckContext() {
        const auto ctx = Rando::Context::GetInstance();
        if (ctx == nullptr || gPlayState != nullptr) {
            throw Failure(kUnsupported, "the probe runs only during generation, with no game running");
        }
        if (ctx->GetLogic()->mSaveContext == &gSaveContext) {
            throw Failure(kUnsupported, "the live Logic is bound to a loaded save");
        }
        if (ComboRando::OotSongOracle()) {
            throw Failure(kViolation, "called inside an oracle search");
        }
        if (!OotOracleInitialized()) {
            throw Failure(kUnsupported, "the native oracle is not prepared");
        }
        const auto& age = ctx->GetOption(RSK_SELECTED_STARTING_AGE);
        if (!age.Is(RO_AGE_CHILD) && !age.Is(RO_AGE_ADULT)) {
            throw Failure(kUnsupported, "the starting age is not resolved");
        }
        if (!ctx->GetOption(RSK_LOGIC_RULES).Is(RO_LOGIC_GLITCHLESS)) {
            throw Failure(kUnsupported, "only glitchless logic is supported");
        }
        if (ctx->GetOption(RSK_SHUFFLE_ENTRANCES)) {
            throw Failure(kUnsupported, "Ship entrance randomizer is on");
        }
        // Destination shuffle needs a song node. Foreign-song logic is inert here: without a starting warp
        // song or Soaring, refused below and by the gate, no OoTMM song exit opens (lemma S1).
        const auto& songs = ComboRando::ActiveSongTravel();
        if (songs.logicEnabled) {
            throw Failure(kUnsupported, "song travel logic is not supported");
        }
        if (logic.get() != ctx->GetLogic().get()) {
            throw Failure(kViolation, "::logic is not the Context's Logic");
        }
        const auto& rootExits = areaTable[RR_ROOT].exits;
        if (rootExits.size() != 1 || rootExits.front().GetConnectedRegionKey() != RR_ROOT_EXITS) {
            throw Failure(kUnsupported, "the native root is not vanilla");
        }
        for (const auto& b : kBindings) {
            BoundExit(b.parent, b.target);
        }
        for (const RandomizerGet rg : StartingInventory) {
            if (IsWarpSong(rg)) {
                throw Failure(kUnsupported, "starting warp songs are not supported");
            }
        }
    }

    void Check() const {
        if (!sPrepared) {
            throw Failure(kUnsupported, "query before prepare");
        }
        CheckContext();
        if (RegionTable_Epoch() != sPrepared->epoch) {
            throw Failure(kViolation, "the region table was rebuilt after prepare");
        }
        const bool child = Rando::Context::GetInstance()->GetOption(RSK_SELECTED_STARTING_AGE).Is(RO_AGE_CHILD);
        if (child != sPrepared->startingChild) {
            throw Failure(kViolation, "the starting age changed after prepare");
        }
        if (Price(RC_GC_MEDIGORON) != sPrepared->medigoron ||
            Price(RC_WASTELAND_BOMBCHU_SALESMAN) != sPrepared->carpet) {
            throw Failure(kViolation, "merchant prices changed after prepare");
        }
    }

    // P9: every probed row is a bound OoT candidate, portals are the reserved game links, and the
    // MM-only rows are MM rows.
    Overlay<Entrance> Resolve(const QueryRequest& q) const {
        namespace E = ComboRando::Entrances;
        auto isMm = [](const std::string& name) {
            const auto* row = E::Find(name);
            return row != nullptr && row->game == E::Game::Mm;
        };
        for (const auto& link : q.portalLinks) {
            if (!(link.src == "OOT_SHOP_MASKS" && link.dst == "MM_CLOCK_TOWN_FROM_CLOCK_TOWER") &&
                !(link.src == "MM_CLOCK_TOWER_FROM_CLOCK_TOWN" && link.dst == "OOT_MARKET_FROM_MASK_SHOP")) {
                throw Failure(kUnsupported, "unsupported game link " + link.src);
            }
        }
        for (const auto& name : q.mmDeleted) {
            if (!isMm(name)) {
                throw Failure(kUnsupported, "not an MM row: " + name);
            }
        }
        for (const auto& edge : q.mmEdges) {
            if (!isMm(edge.src) || !isMm(edge.dst)) {
                throw Failure(kUnsupported, "not an MM edge: " + edge.src);
            }
        }
        Overlay<Entrance> overlay;
        for (const auto& name : q.deleted) {
            if (std::find(E::kOotWallmasterSources.begin(), E::kOotWallmasterSources.end(), name) !=
                E::kOotWallmasterSources.end()) {
                if (!Rando::ComboEntrances::WallmasterQuestContextAdmitted())
                    throw Failure(kUnsupported, "Temple Wallmaster source is not admitted in these OoT dungeons");
                continue; // Synthetic VOID source: there is no native exit to delete.
            }
            const auto& b = Binding(name, TempleBindingRole::Candidate);
            overlay.deleted.push_back(&BoundExit(b.parent, b.target));
        }
        std::sort(overlay.deleted.begin(), overlay.deleted.end(), std::less<const Entrance*>());
        for (const auto& edge : q.edges) {
            if (std::find(E::kOotWallmasterSources.begin(), E::kOotWallmasterSources.end(), edge.src) !=
                E::kOotWallmasterSources.end()) {
                if (std::find(q.deleted.begin(), q.deleted.end(), edge.src) == q.deleted.end())
                    throw Failure(kUnsupported, "Temple Wallmaster edge did not delete its source");
                try {
                    for (auto exit : Rando::ComboEntrances::WallmasterCandidateExits(edge.src, edge.dst)) {
                        auto owned = std::make_shared<Entrance>(std::move(exit));
                        overlay.added.push_back({ static_cast<size_t>(owned->GetParentRegionKey()), owned.get(),
                                                  static_cast<size_t>(owned->GetConnectedRegionKey()) });
                        overlay.owned.push_back(std::move(owned));
                    }
                } catch (const std::exception& error) { throw Failure(kUnsupported, error.what()); }
                continue;
            }
            const auto& src = Binding(edge.src, TempleBindingRole::Candidate);
            const auto& dst = Binding(edge.dst, TempleBindingRole::Candidate);
            overlay.added.push_back({ static_cast<size_t>(src.parent), &BoundExit(src.parent, src.target),
                                      static_cast<size_t>(dst.target) });
        }
        // validateAgeTemple: the opposite spawn's exit becomes a bare age check (entrances.ts:270-278),
        // which is the Root Exits condition; the starting spawn is simply absent.
        const auto& spawn =
            Binding(sPrepared->startingChild ? "OOT_SPAWN_ADULT" : "OOT_SPAWN_CHILD", TempleBindingRole::RootPolicy);
        overlay.rootSpawn = &BoundExit(RR_ROOT_EXITS, spawn.parent);
        overlay.rootSpawnTarget = spawn.parent;
        return overlay;
    }

    std::shared_ptr<Rando::Logic> MakeLogic() const {
        std::shared_ptr<Rando::Logic> probe(new Rando::Logic(), ProbeLogicDeleter{});
        probe->SetContext(Rando::Context::GetInstance());
        probe->Reset(true); // its own SaveContext, settings-derived grants and BaseHearts
        return probe;
    }
    void ApplyStartingItems() const {
        ApplyProbeStartingItems(*logic, sPrepared->foreign);
    }
};

int Prepare(const char* json) {
    sPrepared.reset();
    DepthGuard depth;
    NativePolicy::CheckContext();
    const auto request = ParsePrepare(json, ComboRando::Entrances::kRegistrySourceSha256);
    const auto ctx = Rando::Context::GetInstance();
    Prepared p;
    p.startingChild = ctx->GetOption(RSK_SELECTED_STARTING_AGE).Is(RO_AGE_CHILD);
    if (request.startingChild != p.startingChild) {
        throw Failure(kUnsupported, "the requested starting age differs from the finalized seed");
    }
    p.epoch = RegionTable_Epoch();
    p.medigoron = Price(RC_GC_MEDIGORON);
    p.carpet = Price(RC_WASTELAND_BOMBCHU_SALESMAN);
    for (const auto& name : request.foreign) {
        const auto it = Rando::StaticData::itemNameToEnum.find(name);
        if (it == Rando::StaticData::itemNameToEnum.end()) {
            throw Failure(kUnsupported, "unknown foreign starting item " + name);
        }
        // Anything else, such as a progressive placeholder, could upgrade an item the native
        // StartingInventory already granted.
        if (!IsSharedStarterName(name) || IsWarpSong(it->second)) {
            throw Failure(kUnsupported, "not a shared starting item: " + name);
        }
        if (std::find(p.foreign.begin(), p.foreign.end(), it->second) == p.foreign.end()) {
            p.foreign.push_back(it->second);
        }
    }
    sPrepared = std::move(p);
    return 0;
}
} // namespace

// Firewall maps every exception to a code; the outer catch covers a failure while recording it.
extern "C" COMBO_EXPORT int SOH_TempleProbe_Prepare(const char* json) {
    try {
        return Firewall(sLastError, [&] { return Prepare(json); });
    } catch (...) { return kInternal; }
}

extern "C" COMBO_EXPORT int SOH_TempleProbe_Query(const char* json, int (*poll)(void), uint8_t* agesOut) {
    try {
        return Firewall(sLastError, [&] {
            NativePolicy policy;
            return RunQuery(policy, json, poll, agesOut);
        });
    } catch (...) { return kInternal; }
}

extern "C" COMBO_EXPORT const char* SOH_TempleProbe_LastError(void) {
    return sLastError.c_str();
}

extern "C" COMBO_EXPORT void SOH_TempleProbe_Release(void) {
    sPrepared.reset();
}
#endif
