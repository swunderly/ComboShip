#include "BeanSourcePolicy.h"
// combo/rando/CrossWorldRando.h
// ComboShip: combined cross-world randomizer generator.
// Header-only, deterministic. No game source touched.
//
// Phase 1 (no-logic, native-only): permutation of each game's vanilla items.
// Phase 2 (combined-logic): assumed fill over the union of both games' pools,
//   joined by the portal, driving per-game reachability oracles.
#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <nlohmann/json.hpp>
#include "gui/ComboGenProgress.h"
#include "CrossForeign.h" // for ComboRando::GameId
#include "SharedItems.h"
#include "SongTravel.h" // MmEntryOotAges/OotArrivalsByAge

namespace ComboRando {

// ---------- Deterministic 64-bit LCG (Knuth / Newlib constants) ----------

struct CwRng {
    uint64_t s;
    explicit CwRng(uint64_t seed) : s(seed ? seed : 0x9E3779B97F4A7C15ULL) {
    }
    uint32_t next() {
        s = s * 6364136223846793005ULL + 1442695040888963407ULL;
        return static_cast<uint32_t>(s >> 33);
    }
    uint32_t below(uint32_t n) {
        return n ? next() % n : 0;
    }
};

// MM's junk placeholder as the dump names it. Cross-placed junk is baked into a real item below, so
// this only reaches a consumer from an older seed or a hand-written plando row.
inline constexpr const char* kMmJunkName = "Junk";

// FNV-1a over a name, so a per-entry RNG stream can be seeded without touching the fill's own.
inline uint32_t CwHashName(const std::string& s) {
    uint32_t h = 2166136261u;
    for (unsigned char c : s) {
        h = (h ^ c) * 16777619u;
    }
    return h;
}

template <class T> inline void cwShuffle(std::vector<T>& v, CwRng& rng) {
    for (size_t i = v.size(); i > 1; --i) {
        size_t j = rng.below(static_cast<uint32_t>(i));
        std::swap(v[i - 1], v[j]);
    }
}

// ---------- Oracle function-pointer types (set by ComboShip.cpp) ----------

struct OracleFns {
    void (*Reset)(void);
    void (*SetOwnedItems)(const char*);
    const char* (*GetReachableChecks)(void);
    void (*PlaceItem)(const char*, const char*);
    // OOT only (null for MM): is the OOT->MM portal region reachable? Valid only immediately after a
    // GetReachableChecks call, whose owned-set it describes (the DLL piggybacks on that search).
    uint8_t (*GetPortalOpen)(void) = nullptr;
    void (*SetSongArrivals)(uint32_t) = nullptr;
    uint32_t (*GetSongSources)(void) = nullptr;
    int (*SongDestination)(int) = nullptr;
    uint32_t (*GetSongKnowledge)() = nullptr;
    void (*SetForeignSongKnowledge)(uint32_t) = nullptr;
    uint8_t (*GetPortalAges)() = nullptr;
    void (*SetEntryAges)(uint8_t) = nullptr;
    uint8_t (*GetDefaultEntryAges)() = nullptr;
    uint32_t (*GetSongSourcesForAge)(int) = nullptr;
    void (*SetSongAgeArrivals)(uint32_t, uint32_t) = nullptr;
    bool (*UseCrossAgeOracle)() = nullptr;
    int (*SetExternalBeanRenewal)(uint8_t) = nullptr;
    // OOT only: the save's starting age (adult bit0, child bit1), Link's age in MM when MM is open at start.
    uint8_t (*GetStartAges)() = nullptr;
};

// Pinned exits preserve age; no all-age destination union is permitted.
template <class Query>
inline auto QueryAgeSongWorlds(const OracleFns& oot, const OracleFns& mm, const std::vector<std::string>& ootOwned,
                               const std::vector<std::string>& mmOwned, bool& mmOpen, Query query, bool mmRootAtStart) {
    using Checks = std::unordered_set<std::string>;
    struct ClearProbeArrivals {
        const OracleFns& oot;
        const OracleFns& mm;
        ~ClearProbeArrivals() {
            if (oot.SetSongArrivals)
                oot.SetSongArrivals(0);
            if (mm.SetSongArrivals)
                mm.SetSongArrivals(0);
        }
    } clear{ oot, mm };
    std::pair<Checks, Checks> result;
    std::array<uint32_t, 2> ootArrivals{}, mmArrivals{};
    uint8_t portalAges = 0;
    const uint8_t baseAges = mmRootAtStart ? (mm.GetDefaultEntryAges() & 3) : 0;
    auto destinations = [](const OracleFns& oracle, int age) {
        uint32_t mask = 0;
        const auto sources = oracle.GetSongSourcesForAge(age);
        for (int i = 0; i < 16; ++i)
            if (sources & (1u << i)) {
                const int destination = oracle.SongDestination(i);
                if (destination >= 0 && destination < 16)
                    mask |= 1u << destination;
            }
        return mask;
    };
    for (int pass = 0; pass < 70; ++pass) {
        CheckGenerationCancellation();
        oot.SetSongAgeArrivals(ootArrivals[0], ootArrivals[1]);
        result.first = query(oot, ootOwned);
        const uint8_t previousPortal = portalAges;
        portalAges |= oot.GetPortalAges() & 3;
        std::array<uint32_t, 2> fromOot{ destinations(oot, 0), destinations(oot, 1) };
        std::array<uint32_t, 2> newMm{ mmArrivals[0] | (fromOot[0] & 0xffc0), mmArrivals[1] | (fromOot[1] & 0xffc0) };
        const uint8_t entryAges = baseAges | portalAges | (newMm[0] ? 1 : 0) | (newMm[1] ? 2 : 0);
        mm.SetEntryAges(entryAges);
        mm.SetSongAgeArrivals(newMm[0], newMm[1]);
        mmOpen |= entryAges != 0;
        result.second = entryAges ? query(mm, mmOwned) : Checks{};
        std::array<uint32_t, 2> allOot = ootArrivals, allMm = newMm;
        for (int age = 0; age < 2; ++age) {
            const auto fromMm = entryAges ? destinations(mm, age) : 0;
            allOot[age] |= (fromOot[age] | fromMm) & 63;
            allMm[age] |= fromMm & 0xffc0;
        }
        if (allOot == ootArrivals && allMm == mmArrivals && portalAges == previousPortal)
            return result;
        ootArrivals = allOot;
        mmArrivals = allMm;
    }
    throw std::runtime_error("Age-bearing song-route search failed to converge");
}

// The two searches exchange destination roots until stable, before crediting any check item.
// Each query starts from EMPTY arrival masks; assumed items from a prior fill probe cannot leak.
template <class Query>
inline auto QuerySongWorlds(const OracleFns& oot, const OracleFns& mm, const std::vector<std::string>& ootOwned,
                            const std::vector<std::string>& mmOwned, bool& mmOpen, Query query,
                            bool mmRootAtStart = false) {
    CheckGenerationCancellation();
    using Checks = std::unordered_set<std::string>;
    std::pair<Checks, Checks> result;
    uint32_t ootArrivals = 0, mmArrivals = 0;
    // Song ownership is shared; instruments/buttons remain local. Read this probe's
    // inventory in BOTH engines before searching, never a previous probe's inventory.
    if (oot.GetSongKnowledge && mm.GetSongKnowledge && oot.SetForeignSongKnowledge && mm.SetForeignSongKnowledge) {
        oot.Reset();
        oot.SetOwnedItems(nlohmann::json(ootOwned).dump().c_str());
        mm.Reset();
        mm.SetOwnedItems(nlohmann::json(mmOwned).dump().c_str());
        const auto ootKnown = oot.GetSongKnowledge(), mmKnown = mm.GetSongKnowledge();
        oot.SetForeignSongKnowledge(mmKnown);
        mm.SetForeignSongKnowledge(ootKnown);
    }
    if (mm.UseCrossAgeOracle && mm.UseCrossAgeOracle() && oot.SetSongArrivals && mm.SetSongArrivals &&
        oot.GetPortalAges && mm.SetEntryAges && mm.GetDefaultEntryAges && oot.GetSongSourcesForAge &&
        mm.GetSongSourcesForAge && oot.SetSongAgeArrivals && mm.SetSongAgeArrivals && oot.SongDestination &&
        mm.SongDestination)
        return QueryAgeSongWorlds(oot, mm, ootOwned, mmOwned, mmOpen, query, mmRootAtStart);
    auto destinations = [](const OracleFns& oracle, uint32_t from) {
        uint32_t mask = 0;
        if (oracle.SongDestination) {
            for (int i = 0; i < 16; i++)
                if (from & (1u << i)) {
                    int d = oracle.SongDestination(i);
                    if (d >= 0 && d < 16)
                        mask |= 1u << d;
                }
        }
        return mask;
    };
    auto allDestinations = [&](const OracleFns& oracle) {
        return oracle.GetSongSources ? destinations(oracle, oracle.GetSongSources()) : 0u;
    };
    // MM keeps OoT's age, so MM's OoT landings carry only the OoT ages that can be in MM (SongTravel.h).
    const bool ootAges = oot.GetPortalAges && oot.GetSongSourcesForAge && oot.SetSongAgeArrivals && oot.GetStartAges;
    std::array<uint32_t, 2> ootAgeArrivals{};
    uint8_t mmEntryAges = ootAges && mmRootAtStart ? static_cast<uint8_t>(oot.GetStartAges() & 3) : uint8_t(0);
    for (int pass = 0; pass < 18; pass++) {
        if (ootAges)
            oot.SetSongAgeArrivals(ootAgeArrivals[0], ootAgeArrivals[1]);
        else if (oot.SetSongArrivals)
            oot.SetSongArrivals(ootArrivals);
        result.first = query(oot, ootOwned);
        mmOpen = mmOpen || !oot.GetPortalOpen || oot.GetPortalOpen() != 0;
        const uint32_t fromOot = allDestinations(oot);
        std::array<uint32_t, 2> fromOotAge{};
        uint8_t entryAges = mmEntryAges;
        if (ootAges) {
            fromOotAge = { destinations(oot, oot.GetSongSourcesForAge(0)),
                           destinations(oot, oot.GetSongSourcesForAge(1)) };
            entryAges = MmEntryOotAges(static_cast<uint8_t>(entryAges | oot.GetPortalAges()), fromOotAge);
        }
        const uint32_t newMm = mmArrivals | (fromOot & 0xffc0);
        mmOpen = mmOpen || newMm != 0;
        if (mm.SetSongArrivals)
            mm.SetSongArrivals(newMm);
        result.second = mmOpen ? query(mm, mmOwned) : Checks{};
        const uint32_t fromMm = mmOpen ? allDestinations(mm) : 0;
        const uint32_t newOot = ootArrivals | ((fromOot | fromMm) & 63);
        const uint32_t allMm = newMm | (fromMm & 0xffc0);
        std::array<uint32_t, 2> newOotAge = OotArrivalsByAge(fromOotAge, fromMm, entryAges);
        newOotAge = { newOotAge[0] | ootAgeArrivals[0], newOotAge[1] | ootAgeArrivals[1] };
        if (newOot == ootArrivals && allMm == mmArrivals && newOotAge == ootAgeArrivals && entryAges == mmEntryAges)
            return result;
        ootArrivals = newOot;
        mmArrivals = allMm;
        ootAgeArrivals = newOotAge;
        mmEntryAges = entryAges;
    }
    throw std::runtime_error("Song-route search failed to converge");
}

// ---------- Data types ----------

// Combined accessibility. The historical OotAccess type is retained for ABI/caller compatibility.
// ALL_REACHABLE: every shuffled AND confined placement in BOTH games is reachable from the start.
// BEATABLE_ONLY: off-path items may be inaccessible in either game; the selected combined goal is proved.
// NO_LOGIC retains its explicit legacy semantics and makes no combined completion promise.
enum class OotAccess { ALL_REACHABLE, BEATABLE_ONLY, NO_LOGIC };

// Maps the OOT dump's "accessibility" block to a mode: No Logic wins; else ALR-off => BEATABLE_ONLY;
// else ALL_REACHABLE. Missing/old dumps default to ALL_REACHABLE (unchanged behavior).
inline OotAccess OotAccessFromDump(const std::string& sohDumpJson) {
    try {
        auto a = nlohmann::json::parse(sohDumpJson).value("accessibility", nlohmann::json::object());
        if (a.value("noLogic", false))
            return OotAccess::NO_LOGIC;
        if (!a.value("allLocationsReachable", true))
            return OotAccess::BEATABLE_ONLY;
    } catch (...) {}
    return OotAccess::ALL_REACHABLE;
}

// Combined win condition (#136). hunt=false is the default both-bosses goal; hunt=true wins on
// `required` Triforce Pieces from EITHER game's pool. These are the friendly names both dumps emit.
inline constexpr const char* kOotTriforcePiece = "Triforce Piece";
inline constexpr const char* kMmTriforcePiece = "Piece of the Triforce";

struct CwGoal {
    bool hunt = false;
    int required = 0;
    int total = -1; // combined pieces placed; -1 = unset (seed predates the combo-owned total)
    enum Type { BOTH = 0, HUNT = 1, EITHER = 2, GANON = 3, MAJORA = 4, QUEST = 5 } type = BOTH;
};

inline CwGoal::Type CwGoalType(const CwGoal& goal) {
    return goal.hunt ? CwGoal::HUNT : goal.type;
}
inline const char* CwGoalName(const CwGoal& goal) {
    switch (CwGoalType(goal)) {
        case CwGoal::HUNT:
            return "triforceHunt";
        case CwGoal::EITHER:
            return "either";
        case CwGoal::GANON:
            return "ganon";
        case CwGoal::MAJORA:
            return "majora";
        case CwGoal::QUEST:
            return "triforceQuest";
        default:
            return "bosses";
    }
}
inline CwGoal CwGoalFromJson(const nlohmann::json& json) {
    CwGoal goal;
    const auto name = json.value("type", std::string("bosses"));
    if (name == "triforceHunt")
        goal.type = CwGoal::HUNT;
    else if (name == "either")
        goal.type = CwGoal::EITHER;
    else if (name == "ganon")
        goal.type = CwGoal::GANON;
    else if (name == "majora")
        goal.type = CwGoal::MAJORA;
    else if (name == "triforceQuest")
        goal.type = CwGoal::QUEST;
    else if (name != "bosses" && name != "both")
        throw std::runtime_error("Unknown combined victory goal: " + name);
    goal.hunt = goal.type == CwGoal::HUNT;
    goal.required = goal.hunt ? json.value("requiredPieces", 0) : 0;
    goal.total = json.value("totalPieces", -1);
    return goal;
}
inline nlohmann::json CwGoalJson(const CwGoal& goal) {
    return { { "type", CwGoalName(goal) }, { "requiredPieces", goal.required }, { "totalPieces", goal.total } };
}
inline constexpr const char* kNamedTriforcePieces[] = { "Triforce of Power", "Triforce of Courage",
                                                        "Triforce of Wisdom" };
inline unsigned CwNamedTriforceMask(const std::vector<std::string>& oot, const std::vector<std::string>& mm) {
    unsigned mask = 0;
    for (const auto* items : { &oot, &mm })
        for (const auto& item : *items)
            for (unsigned i = 0; i < 3; ++i)
                if (item == kNamedTriforcePieces[i])
                    mask |= 1u << i;
    return mask;
}
inline bool CwGoalReached(const CwGoal& goal, bool ganon, bool majora, int pieces = 0, unsigned named = 0) {
    switch (CwGoalType(goal)) {
        case CwGoal::HUNT:
            return goal.required > 0 && pieces >= goal.required;
        case CwGoal::EITHER:
            return ganon || majora;
        case CwGoal::GANON:
            return ganon;
        case CwGoal::MAJORA:
            return majora;
        case CwGoal::QUEST:
            return (named & 7u) == 7u;
        default:
            return ganon && majora;
    }
}
inline bool CwGoalReached(const CwGoal& goal, bool ganon, bool majora, const std::vector<std::string>& oot,
                          const std::vector<std::string>& mm) {
    const int pieces = static_cast<int>(std::count(oot.begin(), oot.end(), kOotTriforcePiece) +
                                        std::count(mm.begin(), mm.end(), kMmTriforcePiece));
    return CwGoalReached(goal, ganon, majora, pieces, CwNamedTriforceMask(oot, mm));
}

// Even split of the combined total across the two pools; OOT takes the odd piece. -1 passes through
// so each game keeps its own slider (old seeds).
inline int CwOotPieces(int total) {
    return total < 0 ? -1 : total - total / 2;
}
inline int CwMmPieces(int total) {
    return total < 0 ? -1 : total / 2;
}
// Ceiling on the combined total. OOT's pool can't absorb its half much past this (100/100 trips
// item_pool.cpp's itemPool <= locCount assert); 50/50 is verified to generate.
inline constexpr int kMaxComboTriforcePieces = 100;

inline bool CwIsTriforcePiece(GameId itemGame, const std::string& itemName) {
    return itemName == (itemGame == GAME_OOT ? kOotTriforcePiece : kMmTriforcePiece);
}

// How many pieces the two settings-scoped pools actually hold. Logged loudly: the names above are
// coupled to each game's item table, so a rename would otherwise silently drop that game's pieces.
inline int CountPoolTriforcePieces(const std::string& sohDumpJson, const std::string& mmDumpJson) {
    auto countIn = [](const std::string& dump, GameId g) {
        int n = 0;
        try {
            auto d = nlohmann::json::parse(dump);

            for (const auto& it : d.value("pool", nlohmann::json::array()))
                n += CwIsTriforcePiece(g, it.value("name", std::string())) ? 1 : 0;
        } catch (...) {}
        return n;
    };
    const int oot = countIn(sohDumpJson, GAME_OOT);
    const int mm = countIn(mmDumpJson, GAME_MM);
    std::cout << "[ComboShip] Triforce Hunt: combined pool holds " << oot << " OOT + " << mm << " MM piece(s)\n";
    if (oot == 0 || mm == 0)
        std::cerr << "[ComboShip] Triforce Hunt: WARNING — one game contributes no pieces (slider at 0, or its "
                     "piece item was renamed)\n";
    return oot + mm;
}

// Reuses GameId from CrossForeign.h (GAME_OOT = 0, GAME_MM = 1)
using Game = GameId;

// Native item category from each game's dump. Governs what may be DISCARDED, independently of
// `advancement` (which governs which fill phase places it). Only JUNK is ever discardable.
enum class CwCat { JUNK, LESSER, HEALTH, BOSS_KEY, SMALL_KEY, TOKEN, MAJOR, MASK, STRAY_FAIRY, UNKNOWN };

inline CwCat CwCatFromName(const std::string& s) {
    if (s == "junk")
        return CwCat::JUNK;
    if (s == "lesser")
        return CwCat::LESSER;
    if (s == "health")
        return CwCat::HEALTH;
    if (s == "bossKey")
        return CwCat::BOSS_KEY;
    if (s == "smallKey")
        return CwCat::SMALL_KEY;
    if (s == "token")
        return CwCat::TOKEN;
    if (s == "major")
        return CwCat::MAJOR;
    if (s == "mask")
        return CwCat::MASK;
    if (s == "strayFairy")
        return CwCat::STRAY_FAIRY;
    return CwCat::UNKNOWN; // older DLL or a new category — never discardable
}

inline const char* CwCatName(CwCat c) {
    switch (c) {
        case CwCat::JUNK:
            return "junk";
        case CwCat::LESSER:
            return "lesser";
        case CwCat::HEALTH:
            return "health";
        case CwCat::BOSS_KEY:
            return "bossKey";
        case CwCat::SMALL_KEY:
            return "smallKey";
        case CwCat::TOKEN:
            return "token";
        case CwCat::MAJOR:
            return "major";
        case CwCat::MASK:
            return "mask";
        case CwCat::STRAY_FAIRY:
            return "strayFairy";
        default:
            return "unknown";
    }
}

struct CwItem {
    Game game;
    std::string name;
    bool advancement;
    CwCat cat = CwCat::UNKNOWN;
    bool criticalRenewable = false;
    // Only native-JUNK may absorb a surplus, and never if it gates logic: OOT files bombchus and
    // Buy Blue Fire/Arrows as ITEM_CATEGORY_JUNK with advancement=true (item_list.cpp:278-325).
    bool discardable() const {
        return cat == CwCat::JUNK && !advancement;
    }
};

struct CwCheck {
    Game game;
    std::string name;
};

struct CwPlacement {
    CwCheck check;
    CwItem item;
};

// ---------- Bean donor rules (shared by the fill, the playthrough and the pare-down) ----------

using BeanSourceMap = std::unordered_map<std::string, BeanSources::Source>;

// A dump's "beanSources" (check -> {license, renewable}), keyed "OOT:<check>"/"MM:<check>". An older
// dump without it adds nothing, and the policy table then answers for every check.
inline void ParseBeanSources(Game game, const nlohmann::json& dump, BeanSourceMap& out) {
    // items() refers into its json; a temporary from value() would die before the loop body runs.
    const auto beanSourceDump = dump.value("beanSources", nlohmann::json::object());
    for (const auto& source : beanSourceDump.items()) {
        if (!source.value().is_object() || !source.value().at("license").is_boolean() ||
            !source.value().at("renewable").is_boolean())
            throw std::runtime_error("Malformed bean donor metadata");
        out[(game == GAME_MM ? "MM:" : "OOT:") + source.key()] = { source.value().at("license").get<bool>(),
                                                                   source.value().at("renewable").get<bool>() };
    }
}

// What reaching a check credits. Items from repeatable shops do not become licenses merely because
// the check was reached; starting/assumed inventory is licensed separately; cows and ordinary rewards
// grant licenses. Only native MM stock renews an MM Magic Bean: cross-game deliveries are one-time
// rewards in this engine (a randomized OoT shop slot sells out, a shuffled OoT cow gives once), even
// where OoTMM's table calls the OoT location renewable.
inline BeanSources::Credit CreditPlacedItem(Game checkGame, const std::string& check, Game itemGame,
                                            const std::string& item, const BeanSourceMap& sources) {
    const bool bean = itemGame == GAME_MM && item == "Magic Bean";
    auto source = BeanSources::ForNativeCheck(checkGame == GAME_MM, check);
    if (auto it = sources.find((checkGame == GAME_MM ? "MM:" : "OOT:") + check); it != sources.end())
        source = it->second;
    if (checkGame != GAME_MM)
        source.renewable = false;
    return BeanSources::CreditPlacement(bean, source);
}

// ---------- Combined assumed fill ----------

struct CombinedFillResult {
    std::string spoilerJson;
    bool success;
    std::string error;
};

// Retry budgets. Pass counts, never wall-clock: a time limit would make success machine-dependent and
// break seed sharing. kFillAttempts is shared with ComboShip.cpp/ComboRandoHeadless.cpp — headless seeds
// only reproduce in-game ones while both loops agree.
// Measured: 25 seeds over 3 adult/Overworld-Keys configs all succeeded on pass 1 with 0 retries, so 6
// total passes is ~6x headroom while capping the worst-case wait (15 passes was ~3 min).
constexpr int kFillAttempts = 2;   // whole-fill retries (re-derives the seed, re-rolls the dumps)
constexpr int kMaxPasses = 3;      // passes within one fill
constexpr int kMaxPrereqTries = 4; // Tier-1 repicks of just the portal prerequisites

// MM access is gated on the OOT->MM portal region (Happy Mask Shop) via ootOracle.GetPortalOpen.
// progress: optional thread-safe progress struct polled by the UI. May be nullptr.
// forcedOotJson: OOT checks the dump can't carry (e.g. Link's Pocket). Each forced item is reserved
// out of the cross pool, owned-from-start for logic, and appended to the OOT placements.
// startingGame (#135): GAME_MM roots MM from the start instead of behind the portal; the portal
// prerequisites are still derived, as the re-entry guarantee for a player who strays into OOT.
inline CombinedFillResult CrossWorldCombinedFill(const std::string& sohDumpJson, const std::string& mmDumpJson,
                                                 uint32_t masterSeed, const OracleFns& ootOracle,
                                                 const OracleFns& mmOracle,
                                                 ComboRando::ComboGenProgress* progress = nullptr,
                                                 const std::string& forcedOotJson = "",
                                                 OotAccess ootAccess = OotAccess::ALL_REACHABLE, CwGoal goal = {},
                                                 GameId startingGame = GAME_OOT, uint32_t sharedMask = 0) {
    CombinedFillResult result;
    result.success = false;

    // Portal gate: NO_LOGIC deliberately skips it (an impossible seed is that mode's point). Any other
    // mode without the export would silently fill MM as sphere-0 reachable — the bug this gate fixes.
    const bool portalGated = ootAccess != OotAccess::NO_LOGIC;
    const bool mmStart = startingGame == GAME_MM;
    if (portalGated && ootOracle.GetPortalOpen == nullptr) {
        result.error = "OOT oracle is missing Combo_SOH_Rando_GetPortalOpen — rebuild soh.dll (the OOT->MM "
                       "portal cannot be gated without it)";
        return result;
    }

    if (progress)
        progress->phase.store(1); // Preparing pools

    // A native confined fill must never masquerade as a valid junk-padded pool.
    // Preserve the engine's retryable failure before any checks/items are accepted.
    for (const auto* dumpJson : { &sohDumpJson, &mmDumpJson }) {
        try {
            auto native = nlohmann::json::parse(*dumpJson);
            if (native.contains("error")) {
                result.error = native.at("error").get<std::string>();
                return result;
            }
        } catch (const std::exception& e) {
            result.error = std::string("Pool parse error: ") + e.what();
            return result;
        }
    }

    // --- Parse pools (single pass per dump) ---
    // Checks from "checks", items from the real "pool" (split by advancement), confined
    // pre-placements from "fixed". Falls back to per-check vanillaItem if an old DLL omits "pool".
    std::vector<CwItem> advItems, junkItems;
    // MM's own pickup-rotation names, for the cross-placed junk bake below.
    std::vector<std::string> mmJunkNames;
    std::vector<CwCheck> allChecks;
    std::unordered_map<std::string, std::string> questRegions;
    BeanSourceMap beanSources;
    std::vector<CwPlacement> lockedPlacements; // confined pre-placements (own-dungeon keys, etc.)

    // Set when a dump carries pool[] but no category — a stale DLL, which would otherwise surface as an
    // unbalanceable pool ("no discardable junk") instead of the rebuild it actually needs.
    std::string missingCategory;

    auto parsePool = [&](Game game, const std::string& dumpJson) {
        auto d = nlohmann::json::parse(dumpJson);
        ParseBeanSources(game, d, beanSources);

        // Fillable checks (name only; the pool below feeds the items).
        for (auto& c : d.value("checks", nlohmann::json::array())) {
            std::string name = c.value("name", "");
            if (!name.empty()) {
                allChecks.push_back({ game, name });
                const auto prefix = game == GAME_OOT ? "oot:" : "mm:";
                std::string region = c.value("area", std::string());
                if (region.empty() && d.contains("locationHints"))
                    region = d["locationHints"].value(name, std::string());
                if (!region.empty())
                    questRegions[std::string(prefix) + name] = std::string(prefix) + region;
            }
        }

        // Item pool: the game's real generated pool, split into advancement/junk.
        if (d.contains("pool")) {
            size_t parsed = 0, withCategory = 0;
            for (auto& it : d["pool"]) {
                std::string name = it.value("name", "");
                if (name.empty())
                    continue;
                bool adv = it.value("advancement", true);
                CwCat cat = CwCatFromName(it.value("category", std::string{}));
                ++parsed;
                withCategory += (cat != CwCat::UNKNOWN) ? 1 : 0;
                (adv ? advItems : junkItems).push_back({ game, name, adv, cat, it.value("criticalRenewable", false) });
            }
            if (parsed > 0 && withCategory == 0)
                missingCategory = (game == GAME_OOT ? "soh.dll" : "2ship.dll");
        } else {
            // Older DLL: reconstruct from per-check vanilla items — exactly one per check, so the
            // balancer no-ops. Class non-advancement as junk so a forced reservation can still pad.
            for (auto& c : d.value("checks", nlohmann::json::array())) {
                std::string vi = c.value("vanillaItem", "");
                if (vi.empty())
                    continue;
                bool adv = c.value("advancement", true);
                (adv ? advItems : junkItems).push_back({ game, vi, adv, adv ? CwCat::UNKNOWN : CwCat::JUNK });
            }
        }

        // MM's junk rotation set, for the bake below. Absent on an older 2ship.dll -> no bake.
        if (game == GAME_MM) {
            for (auto& j : d.value("junkPool", nlohmann::json::array())) {
                if (j.is_string() && !j.get<std::string>().empty())
                    mmJunkNames.push_back(j.get<std::string>());
            }
        }

        // Confined pre-placements: locked to their check, credited to logic when reached. No category —
        // locked items never enter junkItems, so they are never trim candidates.
        for (auto& f : d.value("fixed", nlohmann::json::array())) {
            std::string check = f.value("check", "");
            std::string item = f.value("item", "");
            if (check.empty() || item.empty())
                continue;
            lockedPlacements.push_back({ { game, check }, { game, item, f.value("advancement", true) } });
        }
    };

    try {
        parsePool(GAME_OOT, sohDumpJson);
        parsePool(GAME_MM, mmDumpJson);
    } catch (const std::exception& e) {
        result.error = std::string("Pool parse error: ") + e.what();
        return result;
    }
    if (!missingCategory.empty()) {
        result.error = missingCategory +
                       " is stale: its pool[] has no per-item `category`, so the fill cannot tell junk from "
                       "progression and refuses to balance the pool — rebuild soh.dll / 2ship.dll";
        std::cerr << "[ComboShip] CrossWorldCombinedFill: " << result.error << "\n";
        return result;
    }
    // #135: under an MM start the Mask Shop Key is forced start-with, so a confined placement of it in
    // fixed[] means the force never reached OOT's settings. Warn only — A0 keeps the portal re-openable.
    if (mmStart) {
        bool lockedDoors = false;
        try {
            lockedDoors = nlohmann::json::parse(sohDumpJson)
                              .value("accessibility", nlohmann::json::object())
                              .value("lockOverworldDoors", false);
        } catch (...) {}
        const bool keyPlaced =
            lockedDoors && std::any_of(lockedPlacements.begin(), lockedPlacements.end(), [](const CwPlacement& p) {
                return p.item.game == GAME_OOT && p.item.name == "Mask Shop Key";
            });
        if (keyPlaced)
            std::cerr << "[ComboShip] CrossWorldCombinedFill: WARNING — MM start with Lock Overworld Doors, but the "
                         "Mask Shop Key was confined-placed (Exclude Mask Shop Key was not forced on)\n";
    }

    // Forced OOT placements (e.g. Link's Pocket): reserve each item out of the cross pool and treat
    // it as owned-from-start (auto-granted at save creation). Appended to placements after the fill.
    std::vector<CwPlacement> forcedPlacements;
    std::vector<std::string> ootForcedOwned, mmForcedOwned;
    {
        auto removeOneItem = [](std::vector<CwItem>& v, Game g, const std::string& name) -> bool {
            for (size_t i = 0; i < v.size(); ++i) {
                if (v[i].game == g && v[i].name == name) {
                    v[i] = v.back();
                    v.pop_back();
                    return true;
                }
            }
            return false;
        };
        CwRng frng(masterSeed ^ 0xF0F0F0F0u);
        try {
            if (!forcedOotJson.empty()) {
                auto fj = nlohmann::json::parse(forcedOotJson);
                for (auto& [checkName, spec] : fj.items()) {
                    std::string itemName;
                    bool adv = true;
                    if (spec.contains("item")) {
                        itemName = spec.value("item", std::string{});
                        if (removeOneItem(advItems, GAME_OOT, itemName)) {
                            adv = true;
                        } else if (removeOneItem(junkItems, GAME_OOT, itemName)) {
                            adv = false;
                        } // else: not in pool — place anyway (it's owned at start regardless)
                    } else {
                        // "category": pick a random OOT item from the (settings-scoped) cross pool.
                        std::string cat = spec.value("category", std::string("any"));
                        std::vector<size_t> idx;
                        std::vector<CwItem>* src = nullptr;
                        for (size_t i = 0; i < advItems.size(); ++i)
                            if (advItems[i].game == GAME_OOT)
                                idx.push_back(i);
                        if (!idx.empty()) {
                            src = &advItems;
                            adv = true;
                        } else if (cat == "any") {
                            for (size_t i = 0; i < junkItems.size(); ++i)
                                if (junkItems[i].game == GAME_OOT)
                                    idx.push_back(i);
                            if (!idx.empty()) {
                                src = &junkItems;
                                adv = false;
                            }
                        }
                        if (src) {
                            size_t pick = idx[frng.below(static_cast<uint32_t>(idx.size()))];
                            itemName = (*src)[pick].name;
                            (*src)[pick] = src->back();
                            src->pop_back();
                        }
                    }
                    if (itemName.empty())
                        continue;
                    forcedPlacements.push_back({ { GAME_OOT, checkName }, { GAME_OOT, itemName, adv } });
                    ootForcedOwned.push_back(itemName);
                    // No filler: Link's Pocket isn't in checks[], so reserving its item is already
                    // balanced. The old filler left a permanent +1 surplus Phase B silently discarded.
                }
            }
        } catch (...) {}
    }

    // Shared Items (OoTMM-style): trim MM's copies of each effective family; the fixpoint below
    // mirrors the OOT-owned count back onto MM's oracle. See deviations/rando.md.
    uint32_t effectiveSharedMask = 0;
    const bool fairyOcarinaMm = nlohmann::json::parse(mmDumpJson).value("fairyOcarinaMm", false);
    const auto requestedSharedStarts = SharedStartingInventories(sharedMask, sohDumpJson, mmDumpJson);
    const bool sharedButtonsEnabled = nlohmann::json::parse(sohDumpJson).value("sharedButtonsEnabled", false) &&
                                      nlohmann::json::parse(mmDumpJson).value("sharedButtonsEnabled", false);
    if (sharedMask != 0) {
        bool maskQuestShuffle = false;
        try {
            maskQuestShuffle = nlohmann::json::parse(sohDumpJson)
                                   .value("accessibility", nlohmann::json::object())
                                   .value("maskQuestShuffle", false);
        } catch (...) {}
        for (int i = 0; i < SF_COUNT; ++i) {
            if (!(sharedMask & (1u << i)))
                continue;
            if (i == SF_OCARINA) {
                // Pinned shared transform retains the two OOT tiers and removes MM's instrument copies.
                // MM-native Fairy duplication applies only to the non-shared pool path.
                bool has = false;
                for (const auto& item : advItems)
                    if (item.game == GAME_OOT && IsOcarinaName(item.name))
                        has = true;
                for (const auto& placement : lockedPlacements)
                    if (placement.item.game == GAME_OOT && IsOcarinaName(placement.item.name))
                        has = true;
                const auto od = nlohmann::json::parse(sohDumpJson), md = nlohmann::json::parse(mmDumpJson);
                auto os = od.value("sharedStartingItems", std::vector<std::string>{});
                auto ms = md.value("sharedStartingItems", std::vector<std::string>{});
                if (!has && !OwnedOcarinaTier(os) && !OwnedOcarinaTier(ms))
                    continue;
                advItems.erase(
                    std::remove_if(advItems.begin(), advItems.end(),
                                   [](const CwItem& item) { return item.game == GAME_MM && IsOcarinaName(item.name); }),
                    advItems.end());
                int inheritedStart = std::max(0, OwnedOcarinaTier(ms) - OwnedOcarinaTier(os));
                for (auto it = advItems.begin(); it != advItems.end() && inheritedStart;) {
                    if (it->game == GAME_OOT && IsOcarinaName(it->name)) {
                        it = advItems.erase(it);
                        --inheritedStart;
                    } else
                        ++it;
                }
                effectiveSharedMask |= 1u << SF_OCARINA;
                continue;
            }
            if (!SharedExtraSpellEnabled(i, nlohmann::json::parse(mmDumpJson)))
                continue;
            const auto& def = SharedFamilyByIndex(i);
            if (def.requiresButtons && !sharedButtonsEnabled)
                continue;
            const bool startsShared = i >= SF_OCARINA_BUTTON_A &&
                                      std::find(requestedSharedStarts.first.begin(), requestedSharedStarts.first.end(),
                                                def.ootName) != requestedSharedStarts.first.end();
            if (def.isMask && !maskQuestShuffle) {
                std::cout << "[ComboShip] Shared " << def.key << ": skipped (OOT Mask Quest is not Shuffle)\n";
                continue;
            }
            size_t ootCopies = 0;
            for (const auto& it : advItems)
                if (it.game == GAME_OOT && it.name == def.ootName)
                    ++ootCopies;
            for (const auto& p : lockedPlacements)
                if (p.item.game == GAME_OOT && p.item.name == def.ootName)
                    ++ootCopies;
            if (ootCopies == 0 && !startsShared) {
                std::cout << "[ComboShip] Shared " << def.key << ": OOT pool has none, left MM copies alone\n";
                continue;
            }
            if (!def.mmHasItem) {
                // MM has no pool copy of this item — nothing to trim, just mark the family effective.
                effectiveSharedMask |= (1u << i);
                std::cout << "[ComboShip] Shared " << def.key << ": MM has no item, not trimmed\n";
                continue;
            }
            std::vector<CwItem> kept;
            kept.reserve(advItems.size());
            size_t removed = 0;
            for (auto& it : advItems) {
                if (it.game == GAME_MM && it.name == def.mmName) {
                    ++removed;
                    continue;
                }
                kept.push_back(std::move(it));
            }
            advItems = std::move(kept);
            if (removed == 0 && !startsShared) {
                // Name drift between the two games' pools (see deviations/rando.md) — not fatal, but
                // loud: this is exactly the bug class that shipped twice during implementation.
                std::cout << "[ComboShip] Shared " << def.key << ": MM pool has none of '" << def.mmName
                          << "', not shared\n";
                continue;
            }
            effectiveSharedMask |= (1u << i);
            if (startsShared) {
                advItems.erase(std::remove_if(advItems.begin(), advItems.end(),
                                              [&](const CwItem& item) {
                                                  return item.game == GAME_OOT && item.name == def.ootName;
                                              }),
                               advItems.end());
                // Restricted native song preparation may already have confined this
                // shared starting possession. Keep its check confined, but give it
                // same-game filler rather than another copy of the owned item.
                for (auto& placement : lockedPlacements) {
                    const auto& name = placement.item.game == GAME_OOT ? def.ootName : def.mmName;
                    if (placement.item.name != name)
                        continue;
                    const auto filler = std::find_if(junkItems.begin(), junkItems.end(), [&](const CwItem& item) {
                        return item.game == placement.item.game && item.discardable();
                    });
                    if (filler == junkItems.end()) {
                        result.error = "cannot replace confined shared starting item: no same-game junk";
                        return result;
                    }
                    placement.item = *filler;
                }
            }
            std::cout << "[ComboShip] Shared " << def.key << ": trimmed " << removed << " MM '" << def.mmName
                      << "' (balancer will pad with junk)\n";
        }
    }

    const auto sharedStarts = SharedStartingInventories(effectiveSharedMask, sohDumpJson, mmDumpJson);
    ootForcedOwned.insert(ootForcedOwned.end(), sharedStarts.first.begin(), sharedStarts.first.end());
    mmForcedOwned.insert(mmForcedOwned.end(), sharedStarts.second.begin(), sharedStarts.second.end());

    // --- Balance each game's pool against its checks: P_g == C_g ---
    // Both generators over-supply on purpose and the over-supply is PROGRESSION, so only junk may be
    // sacrificed. RNG-free and ahead of `rng`, so it can't shift the stream. See deviations/rando.md.
    size_t trimmedTotal = 0, paddedTotal = 0;
    {
        auto countFor = [](const std::vector<CwItem>& v, Game g) {
            return static_cast<size_t>(std::count_if(v.begin(), v.end(), [g](const CwItem& i) { return i.game == g; }));
        };
        std::string balanceLine;
        std::vector<std::string> trimNotes;
        for (Game g : { GAME_OOT, GAME_MM }) {
            const char* tag = (g == GAME_OOT ? "oot" : "mm");
            const size_t checkCount = static_cast<size_t>(
                std::count_if(allChecks.begin(), allChecks.end(), [g](const CwCheck& c) { return c.game == g; }));
            const size_t itemCount = countFor(advItems, g) + countFor(junkItems, g);
            size_t trimmed = 0, padded = 0;

            if (itemCount > checkCount) {
                // Most-duplicated discardable name first, (count desc, name asc) — a total order, so
                // hash iteration order can't leak in. Sheds bulk filler, keeps the last of rarer junk.
                const size_t surplus = itemCount - checkCount;
                std::unordered_map<std::string, int> avail;
                for (const auto& i : junkItems)
                    if (i.game == g && i.discardable())
                        ++avail[i.name];
                std::unordered_map<std::string, int> remove;
                for (size_t k = 0; k < surplus; ++k) {
                    const std::string* best = nullptr;
                    int bestCount = 0;
                    for (const auto& [n, c] : avail) {
                        if (c <= 0)
                            continue;
                        if (!best || c > bestCount || (c == bestCount && n < *best)) {
                            best = &n;
                            bestCount = c;
                        }
                    }
                    if (!best) {
                        result.error = std::string("cannot balance the ") + tag +
                                       " pool: " + std::to_string(surplus - k) +
                                       " surplus item(s) left and no discardable junk remains; only junk may be "
                                       "discarded, so refusing to drop anything else";
                        std::cerr << "[ComboShip] CrossWorldCombinedFill: " << result.error << "\n";
                        return result;
                    }
                    --avail[*best];
                    ++remove[*best];
                }
                for (const auto& [n, c] : remove)
                    trimNotes.push_back(std::string(tag) + " junk: " + n + " x" + std::to_string(c));
                // Single pass: drop exactly `remove[name]` instances of each chosen junk name.
                std::vector<CwItem> kept;
                kept.reserve(junkItems.size());
                for (auto& i : junkItems) {
                    auto it = (i.game == g && i.discardable()) ? remove.find(i.name) : remove.end();
                    if (it != remove.end() && it->second > 0) {
                        --it->second;
                        ++trimmed;
                        continue;
                    }
                    kept.push_back(std::move(i));
                }
                junkItems = std::move(kept);
            } else if (itemCount < checkCount) {
                // Clone one of THIS game's junk items so no check goes itemless. Never clone the other
                // game's: the name wouldn't resolve in this game's apply path (a silent empty check).
                const size_t deficit = checkCount - itemCount;
                const CwItem* src = nullptr;
                for (const auto& i : junkItems)
                    if (i.game == g && i.discardable()) {
                        src = &i;
                        break;
                    }
                if (!src) {
                    result.error = std::string("cannot balance the ") + tag + " pool: " + std::to_string(deficit) +
                                   " check(s) short and no " + tag + " junk to clone";
                    std::cerr << "[ComboShip] CrossWorldCombinedFill: " << result.error << "\n";
                    return result;
                }
                CwItem filler = *src; // copy before push_back (may reallocate)
                for (size_t k = 0; k < deficit; ++k)
                    junkItems.push_back(filler);
                padded = deficit;
                std::cout << "[ComboShip] CrossWorldCombinedFill: " << tag << " pool was " << deficit
                          << " item(s) short of its checks; padded with junk ('" << filler.name
                          << "') — benign if a forced placement reserved an item, else the dump under-filled\n";
            }

            trimmedTotal += trimmed;
            paddedTotal += padded;
            balanceLine += std::string(balanceLine.empty() ? "" : ", ") + tag +
                           " items=" + std::to_string(countFor(advItems, g) + countFor(junkItems, g)) +
                           "/checks=" + std::to_string(checkCount) + " (trimmed " + std::to_string(trimmed) +
                           ", padded " + std::to_string(padded) + ")";
        }
        std::cout << "[ComboShip] CrossWorldCombinedFill: pool balance — " << balanceLine << " (forced "
                  << forcedPlacements.size() << ")\n";
        for (const auto& n : trimNotes)
            std::cout << "[ComboShip] CrossWorldCombinedFill:     trimmed " << n << "\n";
    }

    if (CwGoalType(goal) == CwGoal::QUEST) {
        // Extra items replace three discardable rewards. Keep these out of assumed fill:
        // they are placed after the normal progression, using its actual spheres.
        for (const auto* name : kNamedTriforcePieces) {
            auto filler =
                std::find_if(junkItems.begin(), junkItems.end(), [](const CwItem& i) { return i.discardable(); });
            if (filler == junkItems.end()) {
                result.error = "Triforce Quest needs three replaceable junk rewards";
                return result;
            }
            *filler = { GAME_OOT, name, true, CwCat::MAJOR };
        }
    }
    CwRng rng(masterSeed);
    cwShuffle(allChecks, rng);

    // OOT and MM check names are distinct namespaces; key fill bookkeeping by game+name.
    auto checkKey = [](const CwCheck& c) { return std::string(c.game == GAME_OOT ? "oot:" : "mm:") + c.name; };

    // Fixed vanilla/confined placements remain in their original locations. The collected-inventory
    // fixed point and final combined guarantee validate them along with the shuffled checks.
    std::unordered_set<std::string> lockedCheckKeys;
    for (const auto& lp : lockedPlacements)
        lockedCheckKeys.insert(checkKey(lp.check));

    // Per-oracle query stats (count + total ms), logged on completion — the searches dominate
    // fill time, so this is the first thing to read when generation feels slow.
    struct QueryStats {
        uint32_t count = 0;
        int64_t ms = 0;
    };
    QueryStats ootStats, mmStats;

    std::vector<CwPlacement> placements;
    bool externalMmBeanRenewal = false;
    auto queryReachable = [&](const OracleFns& oracle,
                              const std::vector<std::string>& ownedItems) -> std::unordered_set<std::string> {
        QueryStats& stats = (&oracle == &ootOracle) ? ootStats : mmStats;
        CheckGenerationCancellation();
        auto t0 = std::chrono::steady_clock::now();
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& n : ownedItems)
            arr.push_back(n);
        oracle.Reset();
        oracle.SetOwnedItems(arr.dump().c_str());
        // Reset clears MM's simulated shelves. Reinstall the current placement
        // map before its resource search; owned equipment alone cannot describe
        // which reachable shops/cows supply renewable potions and Bombchus.
        if (&oracle == &mmOracle && oracle.PlaceItem) {
            for (const auto& p : placements) {
                if (p.check.game == GAME_MM) {
                    oracle.PlaceItem(p.check.name.c_str(), p.item.game == GAME_MM ? p.item.name.c_str() : "");
                }
            }
        }
        if (&oracle == &mmOracle && externalMmBeanRenewal) {
            if (!oracle.SetExternalBeanRenewal || !oracle.SetExternalBeanRenewal(1))
                throw std::runtime_error("MM bean renewal oracle unavailable");
        }
        std::string raw = oracle.GetReachableChecks();
        std::unordered_set<std::string> out;
        try {
            auto parsed = nlohmann::json::parse(raw);
            for (const auto& name : parsed)
                out.insert(name.get<std::string>());
        } catch (...) {}
        stats.count++;
        stats.ms +=
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();
        return out;
    };

    // OOT-only, portal SHUT: nothing assumed from the free pool, MM never queried. Credits a placement
    // only once its check is reachable — reachableFixpoint's rule, so this can't certify what it rejects.
    struct OotClosedResult {
        std::unordered_set<std::string> reach;
        bool portalOpen = false;
    };
    auto ootClosedFixpoint = [&](const std::vector<CwPlacement>& pl,
                                 const std::vector<std::string>& extraOwned) -> OotClosedResult {
        std::vector<std::string> owned = ootForcedOwned;
        owned.insert(owned.end(), extraOwned.begin(), extraOwned.end());
        std::vector<bool> credited(pl.size(), false);
        OotClosedResult r;
        if (ootOracle.SetSongArrivals)
            ootOracle.SetSongArrivals(0);
        for (;;) {
            r.reach = queryReachable(ootOracle, owned);
            r.portalOpen = r.portalOpen || ootOracle.GetPortalOpen == nullptr || ootOracle.GetPortalOpen() != 0;
            bool changed = false;
            for (size_t i = 0; i < pl.size(); ++i) {
                if (credited[i] || pl[i].check.game != GAME_OOT || pl[i].item.game != GAME_OOT)
                    continue;
                if (r.reach.count(pl[i].check.name)) {
                    owned.push_back(pl[i].item.name);
                    credited[i] = true;
                    changed = true;
                }
            }
            if (!changed)
                return r;
        }
    };

    // Portal prerequisites: a SUFFICIENT witness set for the OOT->MM portal, built FORWARD from empty.
    // RNG-free (canonical order) so it can't shift the seeded stream. See docs/deviations/rando.md.
    std::vector<std::string> portalPrereqs;
    if (portalGated) {
        // Each probe is a fixpoint (several queries), so this bounds queries, not probes.
        const int kMaxDeriveQueries = 1500;
        const uint32_t queries0 = ootStats.count;
        int used = 0;
        auto portalOpenWith = [&](const std::vector<std::string>& owned) {
            bool open = ootClosedFixpoint(lockedPlacements, owned).portalOpen;
            used = static_cast<int>(ootStats.count - queries0);
            return open;
        };
        const std::vector<std::string> base; // free pool only; locked items earn their way in by reach
        std::vector<std::string> rest;
        for (const auto& it : advItems)
            if (it.game == GAME_OOT)
                rest.push_back(it.name);
        std::sort(rest.begin(), rest.end());

        std::vector<std::string> witness;
        auto ownedWith = [&](size_t k) {
            std::vector<std::string> o = base;
            o.insert(o.end(), witness.begin(), witness.end());
            o.insert(o.end(), rest.begin(), rest.begin() + k);
            return o;
        };
        if (portalOpenWith(base)) {
            std::cout << "[ComboShip] CrossWorldCombinedFill: portal open from the start — no prerequisites\n";
        } else if (!portalOpenWith(ownedWith(rest.size()))) {
            // Tier 3: warn loudly, try anyway — a derivation bug must not block a config that generates.
            std::cerr << "[ComboShip] CrossWorldCombinedFill: WARNING — the OOT->MM portal looks unreachable even "
                         "with every OOT item owned; generating anyway, MM may end up unreachable\n";
        } else {
            // Bisect `rest` for the item that flips the portal, add it, drop everything after it, repeat.
            while (used < kMaxDeriveQueries && !rest.empty()) {
                size_t lo = 0, hi = rest.size(); // portal closed at lo, open at hi
                while (hi - lo > 1 && used < kMaxDeriveQueries) {
                    size_t mid = lo + (hi - lo) / 2;
                    if (portalOpenWith(ownedWith(mid)))
                        hi = mid;
                    else
                        lo = mid;
                }
                witness.push_back(rest[hi - 1]);
                rest.resize(hi - 1);
                if (portalOpenWith(ownedWith(0)))
                    break;
            }
            if (used >= kMaxDeriveQueries) {
                std::cerr << "[ComboShip] CrossWorldCombinedFill: WARNING — portal prerequisite derivation exceeded "
                             "its "
                          << kMaxDeriveQueries << "-query budget; falling back to an unconstrained fill\n";
            } else {
                portalPrereqs = witness;
                std::cout << "[ComboShip] CrossWorldCombinedFill: portal prerequisites (" << portalPrereqs.size()
                          << ", " << used << " queries):";
                for (const auto& n : portalPrereqs)
                    std::cout << " " << n;
                std::cout << "\n";
            }
        }
    }

    // Cross-game sphere-collect fixpoint: starting from the per-game base sets (unplaced
    // advancement items), repeatedly query both oracles and credit any prior placement whose
    // CHECK became reachable to the ITEM's game's owned set, until nothing changes. This spans
    // both games because an MM item at an OOT check (or vice versa) can open progress anywhere,
    // including the portal itself — so portal openness is re-evaluated every iteration.
    // ootReachable/mmReachable = reachable checks at the fixpoint; ootOwned = OOT items collected along
    // the way (drives the BEATABLE_ONLY boss-key goal check without a second traversal).
    struct FixResult {
        std::unordered_set<std::string> ootReachable, mmReachable;
        std::vector<std::string> ootOwned, mmOwned;
        std::unordered_map<std::string, int> spheres;
    };
    auto reachableFixpoint = [&](const std::vector<std::string>& ootBase,
                                 const std::vector<std::string>& mmBase) -> FixResult {
        std::vector<std::string> ootOwned = ootBase, mmOwned = mmBase;
        // Forced placements (Link's Pocket etc.) are granted at save creation → owned from the start.
        ootOwned.insert(ootOwned.end(), ootForcedOwned.begin(), ootForcedOwned.end());
        mmOwned.insert(mmOwned.end(), mmForcedOwned.begin(), mmForcedOwned.end());
        externalMmBeanRenewal = false;
        std::vector<bool> credited(placements.size(), false);
        std::unordered_set<std::string> ootReachable, mmReachable;
        // Latched: once OOT can reach the portal it stays open. An MM start roots MM immediately (#135).
        bool portalOpen = !portalGated || mmStart;
        // Shared Items mirror: how many copies of each effective family have already been pushed onto
        // mmOwned this call, so re-querying doesn't duplicate them as ootOwned grows.
        size_t sharedGiven[SF_COUNT] = { 0 };
        std::unordered_map<std::string, int> spheres;
        int sphere = 1;
        for (;;) {
            if (effectiveSharedMask != 0) {
                MirrorSharedSingletons(effectiveSharedMask, ootOwned, mmOwned);
                if (effectiveSharedMask & (1u << SF_OCARINA))
                    MirrorSharedOcarina(ootOwned, mmOwned, fairyOcarinaMm);
                for (int i = 0; i < SF_COUNT; ++i) {
                    if (!(effectiveSharedMask & (1u << i)))
                        continue;
                    if (i == SF_OCARINA)
                        continue;
                    const auto& def = SharedFamilyByIndex(i);
                    if (!def.mmHasItem)
                        continue; // nothing to push; MM logic doesn't need it
                    size_t k =
                        static_cast<size_t>(std::count(ootOwned.begin(), ootOwned.end(), std::string(def.ootName)));
                    size_t target = std::min<size_t>(k, static_cast<size_t>(def.mmTierCap));
                    for (size_t n = std::max(
                             sharedGiven[i],
                             static_cast<size_t>(std::count(mmOwned.begin(), mmOwned.end(), std::string(def.mmName))));
                         n < target; ++n)
                        mmOwned.push_back(def.mmName);
                    sharedGiven[i] = target;
                }
            }
            auto worlds = QuerySongWorlds(ootOracle, mmOracle, ootOwned, mmOwned, portalOpen, queryReachable,
                                          mmStart || !portalGated);
            ootReachable = std::move(worlds.first);
            mmReachable = std::move(worlds.second);
            for (const auto& check : allChecks)
                if ((check.game == GAME_OOT ? ootReachable : mmReachable).count(check.name))
                    spheres.emplace(checkKey(check), sphere);
            ++sphere;
            bool changed = false;
            for (size_t i = 0; i < placements.size(); ++i) {
                if (credited[i])
                    continue;
                const auto& reach = placements[i].check.game == GAME_OOT ? ootReachable : mmReachable;
                if (reach.count(placements[i].check.name)) {
                    auto& owned = placements[i].item.game == GAME_OOT ? ootOwned : mmOwned;
                    const auto credit = CreditPlacedItem(placements[i].check.game, placements[i].check.name,
                                                         placements[i].item.game, placements[i].item.name, beanSources);
                    if (credit.owned)
                        owned.push_back(placements[i].item.name);
                    if (credit.renewal)
                        externalMmBeanRenewal = true;
                    credited[i] = true;
                    changed = true;
                }
            }
            if (!changed)
                return { std::move(ootReachable), std::move(mmReachable), std::move(ootOwned), std::move(mmOwned),
                         std::move(spheres) };
        }
    };

    if (progress) {
        progress->phase.store(2);                                 // Placing items
        progress->total.store(static_cast<int>(advItems.size())); // tracked phase = advancement fill
        progress->placed.store(0);
    }

    // --- Assumed fill of advancement items, then junk fast-fill; retry whole passes on dead
    // ends or failed validation (deterministic: one rng stream continues across passes) ---
    std::unordered_set<std::string> filledChecks; // checkKey()-keyed
    bool fillOk = false;
    int passesUsed = 0;
    std::string lastPassError; // names the terminal cause instead of a generic "fill failed"

    // Expected post-balance multiset, for the per-pass backstop below: every pool item must reach
    // `placements`. This is the assertion that catches item loss from ANY cause, not just truncation.
    std::unordered_map<std::string, int> expectedItems;
    struct ExpectedInfo {
        CwCat cat;
        bool discardable;
    };
    std::unordered_map<std::string, ExpectedInfo> expectedInfo;
    for (const auto* src : { &advItems, &junkItems })
        for (const auto& i : *src) {
            const std::string key = std::string(i.game == GAME_OOT ? "oot:" : "mm:") + i.name;
            ++expectedItems[key];
            expectedInfo[key] = { i.cat, i.discardable() };
        }

    for (int pass = 1; pass <= kMaxPasses && !fillOk; ++pass) {
        passesUsed = pass;
        auto passStart = std::chrono::steady_clock::now();
        auto passMs = [&] {
            return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - passStart)
                .count();
        };
        placements.clear();
        filledChecks.clear();

        // Seed confined pre-placements: locked (never re-filled) and credited to logic when their
        // check is reached, via reachableFixpoint — the collected-in-place semantics for dungeon keys.
        placements = lockedPlacements;
        for (const auto& lp : lockedPlacements)
            filledChecks.insert(checkKey(lp.check));

        // Phase A0: portal prerequisites FIRST, into portal-closed-reachable checks, each chosen RANDOMLY
        // across the whole valid set (variety comes from the choice, not the ordering).
        std::vector<CwItem> advRest = advItems;
        bool prereqOk = portalPrereqs.empty();
        for (int t = 1; t <= kMaxPrereqTries && !prereqOk; ++t) {
            placements = lockedPlacements;
            filledChecks.clear();
            for (const auto& lp : lockedPlacements)
                filledChecks.insert(checkKey(lp.check));
            advRest = advItems;
            std::vector<CwItem> prereq; // one pool instance per witness name
            for (const auto& name : portalPrereqs) {
                for (size_t i = 0; i < advRest.size(); ++i) {
                    if (advRest[i].game == GAME_OOT && advRest[i].name == name) {
                        prereq.push_back(advRest[i]);
                        advRest.erase(advRest.begin() + i);
                        break;
                    }
                }
            }
            cwShuffle(prereq, rng);
            bool placedAll = true;
            for (const auto& item : prereq) {
                auto fr = ootClosedFixpoint(placements, {});
                std::vector<size_t> cands;
                for (size_t ci = 0; ci < allChecks.size(); ++ci) {
                    if (allChecks[ci].game != GAME_OOT || filledChecks.count(checkKey(allChecks[ci])))
                        continue;
                    if (fr.reach.count(allChecks[ci].name))
                        cands.push_back(ci);
                }
                if (cands.empty()) {
                    placedAll = false;
                    break;
                }
                size_t pick = cands[rng.below(static_cast<uint32_t>(cands.size()))];
                placements.push_back({ allChecks[pick], item });
                filledChecks.insert(checkKey(allChecks[pick]));
            }
            prereqOk = placedAll && ootClosedFixpoint(placements, {}).portalOpen;
            if (!prereqOk)
                std::cerr << "[ComboShip] CrossWorldCombinedFill: portal still closed after prerequisite placement "
                             "(pass "
                          << pass << ", try " << t << "/" << kMaxPrereqTries << ") — repicking\n";
        }
        if (!prereqOk) {
            lastPassError = "the OOT->MM portal's prerequisites could not be placed within reach of the start";
            std::cerr << "[ComboShip] CrossWorldCombinedFill: " << lastPassError << " (pass " << pass << ", "
                      << passMs() << " ms) — retrying\n";
            continue;
        }

        // Phase A: place advancement items with logic, assuming only UNPLACED ones are owned. NO_LOGIC
        // assumed-fills only MM adv (OOT adv rides Phase B's fast fill); other modes keep advItems as-is.
        std::vector<CwItem> toPlace;
        if (ootAccess == OotAccess::NO_LOGIC) {
            for (const auto& it : advRest)
                if (it.game == GAME_MM)
                    toPlace.push_back(it);
        } else {
            toPlace = advRest;
        }
        std::vector<CwItem> relaxedToJunk; // advancement stranded off-path under completion-only
        cwShuffle(toPlace, rng);
        // OoTMM solve.ts preserves critical renewable consumables in the
        // required pool and puts their first copy at a repeatable source.
        // Place those first so subsequent inventory queries can inspect actual
        // stock, rather than pretending a one-time refill is renewable.
        std::stable_partition(toPlace.begin(), toPlace.end(),
                              [](const CwItem& item) { return !item.criticalRenewable; });
        std::unordered_set<std::string> renewablePlaced;
        for (const auto& p : placements) {
            if (p.item.game != GAME_MM || p.check.game != GAME_MM)
                continue;
            auto source = BeanSources::ForNativeCheck(true, p.check.name);
            if (auto it = beanSources.find("MM:" + p.check.name); it != beanSources.end())
                source = it->second;
            if (source.renewable)
                renewablePlaced.insert(p.item.name);
        }
        std::vector<std::string> ootRemaining, mmRemaining;
        for (const auto& it : toPlace)
            (it.game == GAME_OOT ? ootRemaining : mmRemaining).push_back(it.name);

        // Batch placement: pull K items and place all K into distinct reachable checks off ONE
        // fixpoint — still a strictly-conservative assumed fill (each lands on a check reachable
        // without itself and the rest of its batch), but divides the fixpoint cost by K. The cap
        // halves on a failed batch (never grows back within a pass); only a failed K=1 ends the pass.
        size_t batchCap = 16;
        auto removeOne = [](std::vector<std::string>& v, const std::string& name) {
            for (size_t i = 0; i < v.size(); ++i) {
                if (v[i] == name) {
                    v[i] = v.back();
                    v.pop_back();
                    return;
                }
            }
        };

        bool deadEnd = false;
        while (!toPlace.empty()) {
            size_t k = std::min({ batchCap, std::max<size_t>(1, toPlace.size() / 4), toPlace.size() });
            if (toPlace.back().criticalRenewable)
                k = 1;

            std::vector<CwItem> batch;
            batch.reserve(k);
            for (size_t i = 0; i < k; ++i) {
                batch.push_back(toPlace.back());
                toPlace.pop_back();
                removeOne(batch.back().game == GAME_OOT ? ootRemaining : mmRemaining, batch.back().name);
            }

            auto fr = reachableFixpoint(ootRemaining, mmRemaining);
            auto& ootReachable = fr.ootReachable;
            auto& mmReachable = fr.mmReachable;

            std::vector<size_t> candidates;
            for (size_t ci = 0; ci < allChecks.size(); ++ci) {
                if (filledChecks.count(checkKey(allChecks[ci])))
                    continue;
                const auto& reach = allChecks[ci].game == GAME_OOT ? ootReachable : mmReachable;
                if (batch.front().criticalRenewable && !renewablePlaced.count(batch.front().name)) {
                    // Cross-game deliveries are one-time rewards in this
                    // engine; only native MM stock can be bought repeatedly.
                    if (allChecks[ci].game != GAME_MM)
                        continue;
                    auto source = BeanSources::ForNativeCheck(true, allChecks[ci].name);
                    if (auto it = beanSources.find("MM:" + allChecks[ci].name); it != beanSources.end())
                        source = it->second;
                    if (!source.renewable)
                        continue;
                }
                if (reach.count(allChecks[ci].name))
                    candidates.push_back(ci);
            }

            if (candidates.size() < batch.size()) {
                // BEATABLE_ONLY: a single advancement item from either game with no reachable check may
                // strand off-path — move it to the junk fast-fill and continue (already removed from
                // toPlace and ootRemaining/mmRemaining above). ALL_REACHABLE and NO_LOGIC (whose
                // assumed fill holds only MM items) put it back and retry instead.
                if (batch.size() == 1 && ootAccess == OotAccess::BEATABLE_ONLY) {
                    relaxedToJunk.push_back(batch.front());
                    continue;
                }
                // Put the batch back (reverse order restores the pop sequence — deterministic).
                for (auto it = batch.rbegin(); it != batch.rend(); ++it) {
                    (it->game == GAME_OOT ? ootRemaining : mmRemaining).push_back(it->name);
                    toPlace.push_back(*it);
                }
                if (batch.size() > 1) {
                    batchCap = batch.size() / 2; // too conservative at this depth — shrink
                    continue;
                }
                lastPassError = "dead end placing '" + batch.front().name + "' (no reachable check left for it)";
                std::cerr << "[ComboShip] CrossWorldCombinedFill: dead end placing '" << batch.front().name
                          << "' (pass " << pass << ", " << placements.size() << " placed, " << passMs()
                          << " ms) — retrying\n";
                deadEnd = true;
                break;
            }

            for (const auto& item : batch) {
                size_t slot = rng.below(static_cast<uint32_t>(candidates.size()));
                size_t pick = candidates[slot];
                candidates[slot] = candidates.back();
                candidates.pop_back();
                placements.push_back({ allChecks[pick], item });
                filledChecks.insert(checkKey(allChecks[pick]));
                if (item.criticalRenewable)
                    renewablePlaced.insert(item.name);
            }
            if (progress)
                // Count advancement placed (placements started at lockedPlacements), so placed/total is
                // an honest 0..100% over the advancement fill — not locked+adv+junk over adv alone.
                progress->placed.store(static_cast<int>(placements.size() - lockedPlacements.size()));
        }
        if (deadEnd)
            continue;

        // Phase B: fast-fill the remaining checks — zero oracle calls. Holds junk, relaxed advancement
        // (either game) and NO_LOGIC's OOT advancement, placed without logic and tolerated-if-unreachable
        // by the validation below.
        std::vector<CwItem> fastFillItems = junkItems;
        if (ootAccess == OotAccess::NO_LOGIC) {
            for (const auto& it : advRest)
                if (it.game == GAME_OOT)
                    fastFillItems.push_back(it);
        }
        fastFillItems.insert(fastFillItems.end(), relaxedToJunk.begin(), relaxedToJunk.end());
        cwShuffle(fastFillItems, rng);
        // Advancement to the front so the truncatable tail is junk by construction. stable_partition is
        // RNG-free, so the shuffle above still decides placement; only adv/junk precedence is forced.
        auto dropFrom = std::stable_partition(fastFillItems.begin(), fastFillItems.end(),
                                              [](const CwItem& i) { return i.advancement; });
        const size_t mustPlace = static_cast<size_t>(dropFrom - fastFillItems.begin());
        size_t ji = 0;
        for (size_t ci = 0; ci < allChecks.size() && ji < fastFillItems.size(); ++ci) {
            if (filledChecks.count(checkKey(allChecks[ci])))
                continue;
            placements.push_back({ allChecks[ci], fastFillItems[ji++] });
            filledChecks.insert(checkKey(allChecks[ci]));
        }
        // The balancer made items == checks per game, so ANY leftover or unfilled check is an
        // arithmetic bug — nothing is discarded here. Non-junk leftovers are named first: they are the
        // rule-1 violations, and mislabelling one as junk is how the original item loss stayed hidden.
        size_t unfilled = 0;
        for (const auto& c : allChecks)
            if (!filledChecks.count(checkKey(c)))
                ++unfilled;
        size_t leftoverBad = 0;
        std::string leftoverNames;
        for (size_t k = ji; k < fastFillItems.size(); ++k) {
            if (fastFillItems[k].discardable())
                continue;
            ++leftoverBad;
            if (leftoverNames.size() < 300)
                leftoverNames += (leftoverNames.empty() ? "" : ", ") + fastFillItems[k].name + " [" +
                                 (fastFillItems[k].game == GAME_OOT ? "oot/" : "mm/") +
                                 CwCatName(fastFillItems[k].cat) + "]";
        }
        if (ji < fastFillItems.size() || unfilled > 0) {
            // Layout-independent arithmetic, identical on every pass — bail to the caller's seed reroll
            // instead of burning kMaxPasses reproducing the same wrong answer.
            result.error = "pool/check imbalance — " + std::to_string(fastFillItems.size() - ji) +
                           " item(s) had no check left (" + std::to_string(leftoverBad) + " non-junk" +
                           (leftoverNames.empty() ? "" : ": " + leftoverNames) + "), " + std::to_string(unfilled) +
                           " check(s) unfilled, placed " + std::to_string(ji) + " of which " +
                           std::to_string(mustPlace) + " were must-place";
            std::cerr << "[ComboShip] CrossWorldCombinedFill: " << result.error << "\n";
            return result;
        }

        // Backstop: every pool item must have landed somewhere. O(n), no oracle calls, and ahead of the
        // validation fixpoint so a validation retry can't mask it. Non-JUNK residue is the headline.
        {
            auto residual = expectedItems;
            size_t unexpected = 0;
            for (const auto& p : placements) {
                if (lockedCheckKeys.count(checkKey(p.check)))
                    continue;
                auto it = residual.find(std::string(p.item.game == GAME_OOT ? "oot:" : "mm:") + p.item.name);
                if (it == residual.end())
                    ++unexpected; // placed an item that was never in the pool
                else
                    --it->second;
            }
            size_t missingBad = 0, missingJunk = 0, overPlaced = 0;
            std::string missingNames;
            for (const auto& [key, n] : residual) {
                if (n < 0) {
                    overPlaced += static_cast<size_t>(-n);
                    continue;
                }
                if (n == 0)
                    continue;
                // Classify by the same predicate the trim uses, so an advancement item that happens to
                // be category JUNK (OOT bombchus, Buy Blue Fire) is reported as a real loss, not filler.
                const auto info =
                    expectedInfo.count(key) ? expectedInfo.at(key) : ExpectedInfo{ CwCat::UNKNOWN, false };
                (info.discardable ? missingJunk : missingBad) += static_cast<size_t>(n);
                if (!info.discardable && missingNames.size() < 300)
                    missingNames += (missingNames.empty() ? "" : ", ") + key + " x" + std::to_string(n) + " [" +
                                    CwCatName(info.cat) + "]";
            }
            if (missingBad > 0 || missingJunk > 0 || overPlaced > 0 || unexpected > 0) {
                result.error = missingBad > 0
                                   ? std::to_string(missingBad) + " non-junk pool item(s) never placed: " + missingNames
                                   : std::to_string(missingJunk) + " junk never placed, " + std::to_string(overPlaced) +
                                         " over-placed, " + std::to_string(unexpected) +
                                         " not from the pool (balancer arithmetic bug)";
                std::cerr << "[ComboShip] CrossWorldCombinedFill: INVARIANT VIOLATED — " << result.error << "\n";
                return result;
            }
        }

        // Triforce Quest pieces: the late-sphere median, depth weighting and one-piece-per-region rule
        // of OoTMM solver/solve.ts placeNamedTriforceWorld at 41a2b7c0d7dbbb53fed928652f8271bc336c113f.
        // Unlike OoTMM, which places the pieces on any reachable unfilled check right after the
        // required fill, they are swapped in after the full fill, only onto checks holding discardable
        // junk or another piece, with the median taken over those checks; and BEATABLE_ONLY does not
        // add OoTMM's ganonMajora requirement. Upstream MIT notice: TriforceQuest-LICENSE.txt.
        if (CwGoalType(goal) == CwGoal::QUEST) {
            auto baseline = reachableFixpoint({}, {});
            std::vector<size_t> extras, candidates;
            std::vector<int> depths;
            for (size_t i = 0; i < placements.size(); ++i) {
                const auto& p = placements[i];
                if (CwNamedTriforceMask({ p.item.name }, {}) != 0)
                    extras.push_back(i);
                const auto key = checkKey(p.check);
                if ((p.item.discardable() || CwNamedTriforceMask({ p.item.name }, {}) != 0) &&
                    !lockedCheckKeys.count(key) && baseline.spheres.count(key) && questRegions.count(key)) {
                    candidates.push_back(i);
                    depths.push_back(ootAccess == OotAccess::NO_LOGIC ? 1 : baseline.spheres.at(key));
                }
            }
            if (extras.size() != 3 || depths.empty()) {
                lastPassError = "Triforce Quest has no eligible regions";
                continue;
            }
            std::sort(depths.begin(), depths.end());
            const int median = depths[depths.size() / 2];
            std::vector<size_t> weighted;
            for (size_t i : candidates) {
                int depth = ootAccess == OotAccess::NO_LOGIC ? 1 : baseline.spheres.at(checkKey(placements[i].check));
                if (depth >= median)
                    for (int n = 0; n < depth; ++n)
                        weighted.push_back(i);
            }
            cwShuffle(weighted, rng);
            std::vector<size_t> selected;
            while (selected.size() < 3 && !weighted.empty()) {
                const auto pick = weighted.back();
                weighted.pop_back();
                selected.push_back(pick);
                const auto region = questRegions.at(checkKey(placements[pick].check));
                weighted.erase(
                    std::remove_if(weighted.begin(), weighted.end(),
                                   [&](size_t i) { return questRegions.at(checkKey(placements[i].check)) == region; }),
                    weighted.end());
            }
            if (selected.size() != 3) {
                lastPassError = "Triforce Quest needs three distinct late-sphere regions";
                continue;
            }
            // First park each named piece at its selected check, retaining the exact item multiset.
            for (size_t n = 0; n < 3; ++n) {
                const auto name = kNamedTriforcePieces[n];
                const auto current = std::find_if(placements.begin(), placements.end(),
                                                  [&](const CwPlacement& p) { return p.item.name == name; });
                std::swap(current->item, placements[selected[n]].item);
            }
        }

        // Validate the actual sphere fixed point, with no assumed inventory. A confined or junk
        // reward is still a check: Every check reachable has no exceptions for either game.
        auto vf = reachableFixpoint({}, {});
        auto& ootFinal = vf.ootReachable;
        auto& mmFinal = vf.mmReachable;
        size_t mmAdvUnreachable = 0, ootAdvUnreachable = 0, junkUnreachable = 0, junkUnreachableOot = 0;
        size_t missingOot = 0, missingMm = 0;
        std::string missingOotNames, missingMmNames;
        for (const auto& p : placements) {
            const auto& reach = p.check.game == GAME_OOT ? ootFinal : mmFinal;
            if (reach.count(p.check.name))
                continue;
            (p.check.game == GAME_OOT ? missingOot : missingMm)++;
            auto& names = p.check.game == GAME_OOT ? missingOotNames : missingMmNames;
            if (names.size() < 900)
                names += (names.empty() ? "" : ", ") + p.check.name;
            if (p.item.advancement)
                (p.item.game == GAME_MM ? mmAdvUnreachable : ootAdvUnreachable)++;
            else {
                ++junkUnreachable;
                if (p.check.game == GAME_OOT)
                    ++junkUnreachableOot;
            }
        }
        const bool ootWin = ootFinal.count("Ganon") > 0;
        const bool mmWin = mmFinal.count("Defeat Majora") > 0;
        int reachablePieces = 0;
        if (goal.hunt) {
            // Reachable placements are credited once by the fix point, so this is collected
            // inventory, not an assumption that an inaccessible piece can be obtained.
            for (const auto& p : placements)
                if (CwIsTriforcePiece(p.item.game, p.item.name) &&
                    (p.check.game == GAME_OOT ? ootFinal : mmFinal).count(p.check.name))
                    ++reachablePieces;
        }
        const bool goalOk = CwGoalReached(goal, ootWin, mmWin, vf.ootOwned, vf.mmOwned);
        const bool everyCheck = ootAccess == OotAccess::ALL_REACHABLE;
        const bool needRetry = (ootAccess != OotAccess::NO_LOGIC && !goalOk) ||
                               (everyCheck && (missingOot || missingMm)) ||
                               (ootAccess == OotAccess::NO_LOGIC && mmAdvUnreachable);
        if (needRetry) {
            lastPassError =
                "Combined " + std::string(everyCheck ? "Every check reachable" : "completion") +
                " validation failed: OOT=" + std::to_string(missingOot) + " MM=" + std::to_string(missingMm) +
                "; goal=" + (goalOk ? "reachable" : "unreachable") +
                (goal.hunt ? " (pieces " + std::to_string(reachablePieces) + "/" + std::to_string(goal.required) + ")"
                           : " (Ganon " + std::to_string(ootWin) + ", Majora " + std::to_string(mmWin) + ")") +
                "; missing OOT: " + missingOotNames + "; missing MM: " + missingMmNames;
            std::cerr << "[ComboShip] " << lastPassError << " (pass " << pass << ") — retrying\n";
            continue;
        }
        // #135: MM start frees the OOT root, so mmAdvUnreachable can't catch a portal that A0's fallback
        // paths left unopenable. Check directly: a strayed itemless player must always be able to walk back in.
        if (mmStart && portalGated && !ootClosedFixpoint(placements, {}).portalOpen) {
            lastPassError = "MM start: the OOT->MM portal is not re-openable from an empty OOT start";
            std::cerr << "[ComboShip] CrossWorldCombinedFill: " << lastPassError << " (pass " << pass << ", "
                      << passMs() << " ms) — retrying\n";
            continue;
        }
        std::cout << "[ComboShip] CrossWorldCombinedFill: "
                  << (everyCheck ? "Every check reachable"
                                 : (ootAccess == OotAccess::NO_LOGIC ? "No logic" : "Only completion guaranteed"))
                  << " satisfied (unreachable checks OOT=" << missingOot << " MM=" << missingMm << "; pass " << pass
                  << ", " << passMs() << " ms; " << advItems.size() << " advancement items, " << junkItems.size()
                  << " junk, " << junkUnreachable << " junk on oracle-unreachable checks [oot=" << junkUnreachableOot
                  << " mm=" << (junkUnreachable - junkUnreachableOot) << "]; pool balanced: " << trimmedTotal
                  << " trimmed, " << paddedTotal << " padded)\n";
        std::cout << "[ComboShip] CrossWorldCombinedFill: oracle queries — oot " << ootStats.count << "x/"
                  << ootStats.ms << " ms, mm " << mmStats.count << "x/" << mmStats.ms << " ms\n";
        fillOk = true;
    }

    if (!fillOk) {
        result.error = "assumed fill failed after " + std::to_string(kMaxPasses) +
                       " passes; last cause: " + (lastPassError.empty() ? "unknown (see log)" : lastPassError);
        return result;
    }

    // Append the reserved forced placements (Link's Pocket etc.) so they flow into the spoiler and
    // get committed to the oracle alongside the assumed-fill results.
    if (!forcedPlacements.empty()) {
        placements.insert(placements.end(), forcedPlacements.begin(), forcedPlacements.end());
        std::cout << "[ComboShip] CrossWorldCombinedFill: " << forcedPlacements.size()
                  << " forced placement(s) applied (e.g. Link's Pocket)\n";
    }

    if (progress) {
        progress->phase.store(3); // Finalizing
        // Zero the bar: post-fill work (the pare-down) re-stores its own placed/total.
        progress->placed.store(0);
        progress->total.store(0);
    }

    // --- Bake cross-placed MM junk into a real item ---
    // MM resolves its junk placeholder at pickup from finalSeed + the MM check id, and a check in OOT
    // has none — so pick from MM's own rotation set here and every surface agrees (see the deviations
    // doc). Own RNG stream, seeded per check: never draws from the fill's, never depends on order.
    {
        std::sort(mmJunkNames.begin(), mmJunkNames.end());
        mmJunkNames.erase(std::unique(mmJunkNames.begin(), mmJunkNames.end()), mmJunkNames.end());
        size_t unbaked = 0;
        for (auto& p : placements) {
            if (p.check.game != GAME_OOT || p.item.game != GAME_MM || p.item.name != kMmJunkName)
                continue;
            if (mmJunkNames.empty()) {
                ++unbaked;
                continue;
            }
            CwRng jr((static_cast<uint64_t>(masterSeed) << 32) ^ CwHashName(p.check.name) ^ 0x4A554E4BULL);
            p.item.name = mmJunkNames[jr.below(static_cast<uint32_t>(mmJunkNames.size()))];
        }
        if (unbaked > 0)
            std::cout << "[ComboShip] junk bake skipped for " << unbaked
                      << " cross placements: 2ship.dll reported no junkPool" << std::endl;
    }

    // --- Build spoiler (same shape as the no-logic generator) ---
    nlohmann::json spoiler;
    spoiler["masterSeed"] = masterSeed;
    spoiler["mode"] = "combined-logic assumed-fill";
    spoiler["startingGame"] = mmStart ? "MM" : "OOT"; // #135
    // poolTrimmed/poolPadded let a shipped spoiler self-report that it came from a balanced fill.
    spoiler["accessibility"] = { { "policy", ootAccess == OotAccess::ALL_REACHABLE   ? "all"
                                             : ootAccess == OotAccess::BEATABLE_ONLY ? "completion"
                                                                                     : "none" },
                                 { "scope", "both-games" } };
    spoiler["fillStats"] = { { "advancementItems", static_cast<uint32_t>(advItems.size()) },
                             { "junkItems", static_cast<uint32_t>(junkItems.size()) },
                             { "poolTrimmed", static_cast<uint32_t>(trimmedTotal) },
                             { "poolPadded", static_cast<uint32_t>(paddedTotal) },
                             { "checks", static_cast<uint32_t>(allChecks.size()) },
                             { "passes", passesUsed } };
    spoiler["sharedItems"] = SharedKeysFromMask(effectiveSharedMask);

    nlohmann::json ootPlacements = nlohmann::json::object();
    nlohmann::json mmPlacements = nlohmann::json::object();
    nlohmann::json foreignMarkers = nlohmann::json::array();

    for (const auto& p : placements) {
        if (p.check.game == GAME_OOT) {
            ootPlacements[p.check.name] = p.item.name;
        } else {
            mmPlacements[p.check.name] = p.item.name;
        }
        if (p.check.game != p.item.game) {
            nlohmann::json marker = { { "checkGame", p.check.game == GAME_OOT ? "oot" : "mm" },
                                      { "checkName", p.check.name },
                                      { "itemGame", p.item.game == GAME_OOT ? "oot" : "mm" },
                                      { "itemName", p.item.name },
                                      // Propagate advancement so foreign checks holding an important item
                                      // still play the held-up pickup animation (else defaulted to junk).
                                      { "advancement", p.item.advancement } };
            // Native item category, so CMC/CSMC can dress the container as the real item instead of
            // the junk sentinel. UNKNOWN is omitted so consumers use the advancement fallback.
            if (p.item.cat != CwCat::UNKNOWN)
                marker["category"] = CwCatName(p.item.cat);
            foreignMarkers.push_back(std::move(marker));
        }
    }

    spoiler["ootCount"] = static_cast<uint32_t>(ootPlacements.size());
    spoiler["oot"] = ootPlacements;
    spoiler["mmCount"] = static_cast<uint32_t>(mmPlacements.size());
    spoiler["mm"] = mmPlacements;
    spoiler["foreign"] = foreignMarkers;
    // Forced placements (e.g. Link's Pocket) are owned at start, so hints must never target them —
    // the dump's fixed[] skips forced checks, so the spoiler names them for CrossHints instead.
    nlohmann::json startKnown = nlohmann::json::array();
    for (const auto& fp : forcedPlacements)
        startKnown.push_back(
            { { "checkGame", fp.check.game == GAME_OOT ? "oot" : "mm" }, { "checkName", fp.check.name } });
    spoiler["startKnown"] = std::move(startKnown);

    // --- Commit placements to oracles (for save consumption) ---
    for (const auto& p : placements) {
        // ComboShip: a foreign item's name lives in the OTHER game's namespace. Baked junk names now
        // collide with real OOT items ("Blue Rupee"), so committing one would place the wrong native
        // item at the check instead of harmlessly missing the lookup.
        if (p.check.game != p.item.game)
            continue;
        if (p.check.game == GAME_OOT) {
            ootOracle.PlaceItem(p.check.name.c_str(), p.item.name.c_str());
        } else {
            mmOracle.PlaceItem(p.check.name.c_str(), p.item.name.c_str());
        }
    }

    result.spoilerJson = spoiler.dump(2);
    result.success = true;
    return result;
}

// ---------- Legacy no-logic generator (kept for fallback) ----------

inline std::string CrossWorldGenerateSpoiler(const std::string& sohDumpJson, const std::string& mmDumpJson,
                                             uint32_t masterSeed) {
    nlohmann::json spoiler;
    spoiler["masterSeed"] = masterSeed;
    spoiler["mode"] = "no-logic native-only (phase1)";

    auto doGame = [&](const char* key, const std::string& dumpJson, uint32_t seed) {
        nlohmann::json out = nlohmann::json::object();
        try {
            auto d = nlohmann::json::parse(dumpJson);
            std::vector<std::string> checkNames, poolItems;
            for (auto& c : d.value("checks", nlohmann::json::array())) {
                std::string n = c.value("name", std::string{});
                if (!n.empty())
                    checkNames.push_back(n);
            }
            if (d.contains("pool")) {
                for (auto& it : d["pool"]) {
                    std::string n = it.value("name", std::string{});
                    if (!n.empty())
                        poolItems.push_back(n);
                }
            } else {
                for (auto& c : d.value("checks", nlohmann::json::array())) {
                    std::string v = c.value("vanillaItem", std::string{});
                    if (!v.empty())
                        poolItems.push_back(v);
                }
            }
            // Confined pre-placements pass through unshuffled.
            for (auto& f : d.value("fixed", nlohmann::json::array())) {
                std::string chk = f.value("check", std::string{});
                std::string item = f.value("item", std::string{});
                if (!chk.empty() && !item.empty())
                    out[chk] = item;
            }
            std::vector<std::string> shuffled = poolItems;
            CwRng rng(seed);
            cwShuffle(shuffled, rng);
            // Same silent-truncation shape as the real fill had; this path only runs with no oracles.
            if (shuffled.size() != checkNames.size())
                std::cerr << "[ComboShip] CrossWorldGenerateSpoiler: " << key << " pool/check mismatch — "
                          << shuffled.size() << " items vs " << checkNames.size() << " checks\n";
            for (size_t i = 0; i < checkNames.size() && i < shuffled.size(); ++i) {
                out[checkNames[i]] = shuffled[i];
            }
            spoiler[std::string(key) + "Count"] = static_cast<uint32_t>(checkNames.size());
        } catch (...) { spoiler[std::string(key) + "Error"] = true; }
        spoiler[key] = out;
    };

    doGame("oot", sohDumpJson, masterSeed ^ 0x4F4F5400u);
    doGame("mm", mmDumpJson, masterSeed ^ 0x4D4D0000u);

    return spoiler.dump(2);
}

} // namespace ComboRando
