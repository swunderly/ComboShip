// combo/rando/ComboPlaythrough.h
// ComboShip: forward "playthrough" simulation over a finished cross-world seed. Starting from an empty
// collected set, it repeatedly asks each game's reachability oracle what's reachable, collects the items
// on newly-reachable checks, and loops until nothing new opens, recording the first winning sphere.
// Shared by the in-game generator (ComboShip.cpp) and the headless validator (ComboRandoHeadless.cpp) so
// the two never diverge. The oracle's logic mode / tricks are controlled by the caller before running.
#pragma once

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "CrossWorldRando.h"
#include "SharedItems.h"

namespace ComboRando {

// Shared Items mirror: pushes newly-owed mmOwned copies per ootOwned's count; `given` tracks what's
// already pushed so repeat calls don't duplicate.
inline void ApplySharedMirror(uint32_t sharedMask, std::vector<std::string>& ootOwned,
                              std::vector<std::string>& mmOwned, size_t (&given)[SF_COUNT],
                              bool fairyOcarinaMm = false) {
    if (sharedMask == 0)
        return;
    MirrorSharedSingletons(sharedMask, ootOwned, mmOwned);
    if (sharedMask & (1u << SF_OCARINA))
        MirrorSharedOcarina(ootOwned, mmOwned, fairyOcarinaMm);
    for (int i = 0; i < SF_COUNT; ++i) {
        if (!(sharedMask & (1u << i)))
            continue;
        if (i == SF_OCARINA)
            continue;
        const auto& def = SharedFamilyByIndex(i);
        if (!def.mmHasItem)
            continue; // nothing to push; MM logic doesn't need it
        size_t k = static_cast<size_t>(std::count(ootOwned.begin(), ootOwned.end(), std::string(def.ootName)));
        size_t target = std::min<size_t>(k, static_cast<size_t>(def.mmTierCap));
        for (size_t n = std::max(
                 given[i], static_cast<size_t>(std::count(mmOwned.begin(), mmOwned.end(), std::string(def.mmName))));
             n < target; ++n)
            mmOwned.push_back(def.mmName);
        given[i] = target;
    }
}

// A single placed item, parsed from a combined-fill spoiler (see ParseSpoilerPlacements). Shared by
// RunPlaythrough and PareDownPlaythrough so both traverse the identical placement set.
struct CwPlacedItem {
    GameId checkGame;
    std::string check;
    GameId itemGame;
    std::string item;
    bool advancement;
    bool major; // native IsMajorItem: drives the barren predicate (barren = no WotH + no major)
};

// Parses a combined-fill spoilerJson ("oot"/"mm" flat check->item maps + "foreign" array, the shape
// CrossWorldCombinedFill/CrossHints::Generate produce) into placements. sohDumpJson/mmDumpJson
// (optional) resolve each item's advancement flag from the pool; absent -> defaults to advancement
// (never hide progression). Mirrors RunPlaythrough's own parse (factored out for reuse).
inline std::vector<CwPlacedItem> ParseSpoilerPlacements(const std::string& spoilerJson,
                                                        const std::string& sohDumpJson = "",
                                                        const std::string& mmDumpJson = "") {
    std::vector<CwPlacedItem> placements;
    struct ForeignInfo {
        GameId itemGame;
        std::string itemName;
        bool advancement;
    };
    std::unordered_map<std::string, ForeignInfo> foreign; // "<cg>:<cn>"-keyed

    std::unordered_map<std::string, bool> advByName[2];
    std::unordered_map<std::string, bool> majorByName[2]; // absent (e.g. MM dump) -> falls back to advancement
    auto loadAdv = [&](GameId g, const std::string& dumpJson) {
        if (dumpJson.empty())
            return;
        try {
            auto d = nlohmann::json::parse(dumpJson);
            for (auto& it : d.value("pool", nlohmann::json::array())) {
                bool adv = it.value("advancement", true);
                advByName[g][it.value("name", "")] |= adv;
                majorByName[g][it.value("name", "")] |= it.value("major", adv);
            }
            for (auto& f : d.value("fixed", nlohmann::json::array())) {
                bool adv = f.value("advancement", true);
                advByName[g][f.value("item", "")] |= adv;
                majorByName[g][f.value("item", "")] |= f.value("major", adv);
            }
        } catch (...) {}
    };
    loadAdv(GAME_OOT, sohDumpJson);
    loadAdv(GAME_MM, mmDumpJson);
    auto lookupAdv = [&](GameId g, const std::string& item) {
        auto it = advByName[g].find(item);
        return it == advByName[g].end() ? true : it->second;
    };
    auto lookupMajor = [&](GameId g, const std::string& item) {
        auto it = majorByName[g].find(item);
        return it == majorByName[g].end() ? lookupAdv(g, item) : it->second;
    };
    // ComboShip: the consolidated file suffixes cross-game item-name collisions "(OOT)"/"(MM)"; strip
    // that here so oracle name resolution (bare friendly) works. No-op on the bare fill output.
    auto stripSuffix = [](std::string s) {
        for (const char* suf : { " (OOT)", " (MM)" }) {
            size_t n = std::string(suf).size();
            if (s.size() >= n && s.compare(s.size() - n, n, suf) == 0)
                return s.substr(0, s.size() - n);
        }
        return s;
    };
    try {
        auto j = nlohmann::json::parse(spoilerJson);
        for (auto& fm : j.value("foreign", nlohmann::json::array())) {
            std::string cg = fm.value("checkGame", ""), cn = fm.value("checkName", "");
            std::string ig = fm.value("itemGame", "");
            foreign[cg + ":" + cn] = { (ig == "mm") ? GAME_MM : GAME_OOT, stripSuffix(fm.value("itemName", "")),
                                       fm.value("advancement", true) };
        }
        auto addGame = [&](const char* key, GameId cg) {
            if (!j.contains(key) || !j[key].is_object())
                return;
            for (auto& [cn, iv] : j[key].items()) {
                auto fit = foreign.find(std::string(key) + ":" + cn);
                bool isForeign = fit != foreign.end();
                GameId ig = isForeign ? fit->second.itemGame : cg;
                std::string item = (isForeign && !fit->second.itemName.empty()) ? fit->second.itemName
                                                                                : stripSuffix(iv.get<std::string>());
                bool adv = isForeign ? fit->second.advancement : lookupAdv(ig, item);
                bool major = isForeign ? fit->second.advancement : lookupMajor(ig, item);
                placements.push_back({ cg, cn, ig, item, adv, major });
            }
        };
        addGame("oot", GAME_OOT);
        addGame("mm", GAME_MM);
    } catch (const std::exception& e) { std::cerr << "[PLAYTHROUGH] spoiler parse error: " << e.what() << "\n"; }
    return placements;
}

inline std::unordered_set<std::string> QueryReachableWithMmStock(const OracleFns& o,
                                                                 const std::vector<std::string>& owned, bool isMm,
                                                                 const std::vector<CwPlacedItem>& placements,
                                                                 const std::unordered_set<size_t>& excluded = {}) {
    CheckGenerationCancellation();
    o.Reset();
    o.SetOwnedItems(nlohmann::json(owned).dump().c_str());
    if (isMm && o.PlaceItem) {
        for (size_t i = 0; i < placements.size(); ++i) {
            const auto& p = placements[i];
            if (p.checkGame == GAME_MM) {
                o.PlaceItem(p.check.c_str(), !excluded.count(i) && p.itemGame == GAME_MM ? p.item.c_str() : "");
            }
        }
    }
    std::unordered_set<std::string> out;
    for (const auto& name : nlohmann::json::parse(o.GetReachableChecks()))
        out.insert(name.get<std::string>());
    return out;
}

using ReachSet = std::shared_ptr<const std::unordered_set<std::string>>;

// Memo key for an owned-item MULTISET: reachability depends on which items and HOW MANY (1 vs 2
// Hookshots differ), not grant order. Sort only — dedupe would collapse progressive counts (stale hit).
inline std::string CanonicalOwnedKey(std::vector<std::string> owned) {
    std::sort(owned.begin(), owned.end());
    std::string key;
    for (auto& n : owned) {
        key += n;
        key += '\x1f';
    }
    return key;
}

// Default win condition: RC_GANON reachable (OOT) AND actual Majora defeat reachable (MM).
// A pluggable goal so another goal (e.g. Triforce hunt) can be swapped in without touching the
// traversal machinery below. The owned vectors let a goal test collected items, not just reachability.
using GoalPredicate =
    std::function<bool(const std::unordered_set<std::string>& ootReach, const std::unordered_set<std::string>& mmReach,
                       const std::vector<std::string>& ownedOot, const std::vector<std::string>& ownedMm)>;
inline bool DefaultGanonMajoraGoal(const std::unordered_set<std::string>& ootReach,
                                   const std::unordered_set<std::string>& mmReach,
                                   const std::vector<std::string>& /*ownedOot*/,
                                   const std::vector<std::string>& /*ownedMm*/) {
    // RC_GANON reachable = OOT beatable (bridge + boss key, placed or force-granted); see CrossWorldRando.h.
    static const char* kOotGanon = "Ganon";
    static const char* kMmWin = "Defeat Majora";
    return ootReach.count(kOotGanon) > 0 && mmReach.count(kMmWin) > 0;
}

// NO_LOGIC goal: OOT may be structurally unbeatable from empty (the combined goal would degenerate and
// every OOT item would look "required"), so gate requiredness on MM (Majora) only. Keeps MM-side WotH
// meaningful; OOT-side requiredness is not evaluated under No Logic (matching its semantics).
inline bool MmOnlyMajoraGoal(const std::unordered_set<std::string>& /*ootReach*/,
                             const std::unordered_set<std::string>& mmReach,
                             const std::vector<std::string>& /*ownedOot*/,
                             const std::vector<std::string>& /*ownedMm*/) {
    static const char* kMmWin = "Defeat Majora"; // friendly name the MM oracle returns (not raw RC_)
    return mmReach.count(kMmWin) > 0;
}

// Counts Triforce Pieces held across BOTH games (#136), for the playthrough log.
inline int CountOwnedTriforcePieces(const std::vector<std::string>& ownedOot, const std::vector<std::string>& ownedMm) {
    int n = 0;
    for (const auto& i : ownedOot)
        n += (i == kOotTriforcePiece) ? 1 : 0;
    for (const auto& i : ownedMm)
        n += (i == kMmTriforcePiece) ? 1 : 0;
    return n;
}

inline GoalPredicate MakeCombinedGoal(CwGoal goal) {
    return [goal](const std::unordered_set<std::string>& oot, const std::unordered_set<std::string>& mm,
                  const std::vector<std::string>& ownedOot, const std::vector<std::string>& ownedMm) {
        return CwGoalReached(goal, oot.count("Ganon") != 0, mm.count("Defeat Majora") != 0, ownedOot, ownedMm);
    };
}

struct PathTarget {
    GameId game;
    const char* check;
    const char* label;
    bool namedItem = false;
};

// These markers are native defeat checks, not entering a boss room. A target is
// hinted only if a collected-inventory baseline actually reaches it.
inline const std::vector<PathTarget>& HintPathTargets() {
    static const std::vector<PathTarget> targets = {
        { GAME_OOT, "Queen Gohma", "Queen Gohma" },
        { GAME_OOT, "King Dodongo", "King Dodongo" },
        { GAME_OOT, "Barinade", "Barinade" },
        { GAME_OOT, "Phantom Ganon", "Phantom Ganon" },
        { GAME_OOT, "Volvagia", "Volvagia" },
        { GAME_OOT, "Morpha", "Morpha" },
        { GAME_OOT, "Twinrova", "Twinrova" },
        { GAME_OOT, "Bongo Bongo", "Bongo Bongo" },
        { GAME_OOT, "Ganon", "Ganon" },
        { GAME_MM, "Woodfall Temple Boss Warp", "Odolwa" },
        { GAME_MM, "Snowhead Temple Boss Warp", "Goht" },
        { GAME_MM, "Great Bay Temple Boss Warp", "Gyorg" },
        { GAME_MM, "Stone Tower Temple Inverted Boss Warp", "Twinmold" },
        { GAME_MM, "Defeat Majora", "Majora" },
        { GAME_OOT, "Ocarina of Time Item", "Zelda's escape" },
        { GAME_OOT, kNamedTriforcePieces[0], "Triforce of Power", true },
        { GAME_OOT, kNamedTriforcePieces[1], "Triforce of Courage", true },
        { GAME_OOT, kNamedTriforcePieces[2], "Triforce of Wisdom", true },
    };
    return targets;
}

struct RequirednessResult {
    // "oot:<check>"/"mm:<check>" -> required (WotH, true) or foolish-candidate (false).
    std::unordered_map<std::string, bool> requiredByCheck;
    // "oot:<area>"/"mm:<area>" -> true once ANY advancement item placed there is required.
    std::unordered_map<std::string, bool> areaHasRequired;
    // An area/item may be required for an optional boss even if it is not required
    // for the chosen victory condition (e.g. Triforce hunt). Labels contain no reward.
    std::unordered_map<std::string, std::vector<std::string>> pathsByCheck;
    std::unordered_map<std::string, std::vector<std::string>> pathsByArea;
    int candidateCount = 0;
    int replayedCount = 0; // per-item counterfactual fixpoint replays actually run (cache misses)
    int64_t ms = 0;
};

// Requiredness pare-down over the COMBINED world: for each placed advancement item, tentatively
// remove it (check stays, item not credited) and re-run the sphere-collect from empty; still-reachable
// goal => NOT required (foolish), else required (WotH). Tested individually (no monotonicity assumed),
// restricted to actually collected checks. Boss/event paths are scored in the same replays.
// mmRestore resets the MM oracle snapshot afterward.
// ootCheckAreas/mmCheckAreas (checkName -> area) roll results up into per-area classification.
// progress (optional) is write-only reporting for the UI; it never influences the result.
inline RequirednessResult PareDownPlaythrough(const std::string& spoilerJson, const OracleFns& ootOracle,
                                              const OracleFns& mmOracle, void (*mmRestore)(),
                                              const std::string& sohDumpJson = "", const std::string& mmDumpJson = "",
                                              const std::unordered_map<std::string, std::string>& ootCheckAreas = {},
                                              const std::unordered_map<std::string, std::string>& mmCheckAreas = {},
                                              GoalPredicate goalReached = DefaultGanonMajoraGoal,
                                              bool portalGated = true, bool mmStart = false,
                                              ComboGenProgress* progress = nullptr, uint32_t sharedMask = 0) {
    RequirednessResult result;
    auto placements = ParseSpoilerPlacements(spoilerJson, sohDumpJson, mmDumpJson);

    auto checkKey = [](const CwPlacedItem& p) {
        return std::string(p.checkGame == GAME_OOT ? "oot:" : "mm:") + p.check;
    };

    // Cache the complete two-world probe. Song access depends on BOTH inventories
    // and whether MM is already reachable; per-engine inventory caches lose those edges.
    struct SongReachMemo {
        ReachSet oot, mm;
        bool mmOpen;
    };
    std::unordered_map<std::string, SongReachMemo> songMemo;
    std::unordered_set<size_t> stockSensitive;
    // The dumps are the same for every replay: parse them once, not once per replay sphere.
    const auto mmDump = nlohmann::json::parse(mmDumpJson.empty() ? "{}" : mmDumpJson);
    const auto sourceMetadata = mmDump.value("beanSources", nlohmann::json::object());
    const bool fairyOcarinaMm = mmDump.value("fairyOcarinaMm", false);
    const auto sharedStarts = SharedStartingInventories(sharedMask, sohDumpJson, mmDumpJson);
    for (size_t i = 0; i < placements.size(); ++i) {
        if (placements[i].checkGame != GAME_MM)
            continue;
        const auto source = sourceMetadata.find(placements[i].check);
        // Without metadata, keep the conservative cache separation. With a
        // native dump, only renewable stock can change the resource search;
        // excluding ordinary rewards is already reflected in owned inventory.
        if (source == sourceMetadata.end() || source->value("renewable", false))
            stockSensitive.insert(i);
    }

    // The same MM search root the fill uses; kept apart from portalOpen, which each search overwrites.
    const bool mmRoot = !portalGated || mmStart;

    // Excludes a SET of placements from ever crediting their items, then sphere-collects everything
    // else from empty until stable. Latch the first win but continue collecting to distinguish
    // optional boss paths from items required for the seed's selected victory condition.
    auto winsWithout = [&](const std::unordered_set<size_t>& excludeIdx, std::vector<char>* creditedOut,
                           uint64_t* pathsOut) {
        std::vector<size_t> excludedKey;
        for (const auto index : excludeIdx)
            if (stockSensitive.count(index))
                excludedKey.push_back(index);
        std::sort(excludedKey.begin(), excludedKey.end());
        auto queryWithStock = [&](const OracleFns& o, const std::vector<std::string>& owned) {
            return QueryReachableWithMmStock(o, owned, &o == &mmOracle, placements, excludeIdx);
        };
        std::vector<std::string> ootOwned = sharedStarts.first, mmOwned = sharedStarts.second;
        std::vector<char> credited(placements.size(), 0);
        ReachSet ootReach, mmReach;
        bool won = false;
        uint64_t paths = 0;
        size_t sharedGiven[SF_COUNT] = { 0 };
        // Latched: MM stays open once OOT can reach the Happy Mask Shop. Ungated (NO_LOGIC) = open,
        // and an MM start (#135) roots MM from the beginning.
        bool portalOpen = mmRoot;
        for (;;) {
            CheckGenerationCancellation();
            ApplySharedMirror(sharedMask, ootOwned, mmOwned, sharedGiven, fairyOcarinaMm);
            const std::string key = nlohmann::json::array({ CanonicalOwnedKey(ootOwned), CanonicalOwnedKey(mmOwned),
                                                            portalOpen, excludedKey })
                                        .dump();
            auto memo = songMemo.find(key);
            if (memo == songMemo.end()) {
                // Requiredness probes share prefixes, but unique counterfactual inventories
                // must not retain full two-world reachability sets for every candidate.
                // Eviction only repeats the identical oracle query; it changes no rule.
                constexpr size_t kMaxSongReachMemoEntries = 128;
                if (songMemo.size() >= kMaxSongReachMemoEntries)
                    songMemo.clear();
                auto worlds =
                    QuerySongWorlds(ootOracle, mmOracle, ootOwned, mmOwned, portalOpen, queryWithStock, mmRoot);
                SongReachMemo value{ std::make_shared<const std::unordered_set<std::string>>(std::move(worlds.first)),
                                     std::make_shared<const std::unordered_set<std::string>>(std::move(worlds.second)),
                                     portalOpen };
                memo = songMemo.emplace(key, std::move(value)).first;
            }
            ootReach = memo->second.oot;
            mmReach = memo->second.mm;
            portalOpen = memo->second.mmOpen;
            won |= goalReached(*ootReach, *mmReach, ootOwned, mmOwned);
            const auto& targets = HintPathTargets();
            for (size_t i = 0; i < targets.size(); ++i) {
                const auto& reach = targets[i].game == GAME_OOT ? *ootReach : *mmReach;
                const bool reached =
                    targets[i].namedItem
                        ? (std::find(ootOwned.begin(), ootOwned.end(), targets[i].check) != ootOwned.end() ||
                           std::find(mmOwned.begin(), mmOwned.end(), targets[i].check) != mmOwned.end())
                        : reach.count(targets[i].check) != 0;
                if (reached)
                    paths |= uint64_t{ 1 } << i;
            }
            bool changed = false;
            for (size_t i = 0; i < placements.size(); ++i) {
                if (credited[i] || excludeIdx.count(i))
                    continue;
                const auto& p = placements[i];
                const auto& reach = (p.checkGame == GAME_OOT) ? *ootReach : *mmReach;
                if (reach.count(p.check)) {
                    (p.itemGame == GAME_OOT ? ootOwned : mmOwned).push_back(p.item);
                    credited[i] = true;
                    changed = true;
                }
            }
            if (!changed)
                break;
        }
        if (!won)
            won = goalReached(*ootReach, *mmReach, ootOwned, mmOwned);
        if (creditedOut)
            *creditedOut = std::move(credited);
        if (pathsOut)
            *pathsOut = paths;
        return won;
    };

    auto t0 = std::chrono::steady_clock::now();

    std::vector<size_t> candAll; // every advancement placement (drives the per-area rollup below)
    for (size_t i = 0; i < placements.size(); ++i)
        if (placements[i].advancement)
            candAll.push_back(i);

    std::vector<signed char> classified(placements.size(), -1); // -1 unknown, 0 not-required, 1 required

    // The baseline yields only legitimately collected checks. Never grant unreachable items
    // to prove an optional path or make a self-locked item look accessible.
    std::vector<char> baseCredited;
    uint64_t basePaths = 0;
    ++result.replayedCount;
    bool baseWon = winsWithout({}, &baseCredited, &basePaths);

    // Candidates = collected advancement checks. A stuck baseline can never win, so blanking
    // any one item still can't win: every advancement item is required, tested none.
    std::vector<size_t> cand;
    if (baseWon) {
        for (size_t i : candAll)
            if (baseCredited[i])
                cand.push_back(i);
            else
                classified[i] = 0;
    } else {
        for (size_t i : candAll)
            classified[i] = 1;
    }
    result.candidateCount = static_cast<int>(cand.size());
    // The pare-down owns the "Finalizing" bar: restart placed/total over the candidate replays.
    if (progress) {
        progress->total.store(result.candidateCount);
        progress->placed.store(0);
    }

    // Per-item WotH (native IsBeatableWithout): blank exactly one candidate check and replay from
    // empty. Still wins -> not required; loses -> required. Definitionally correct, no monotonicity
    // assumption (unlike the old group-test binary split, which our cross-game oracle broke).
    for (size_t i : cand) {
        ++result.replayedCount;
        uint64_t remainingPaths = 0;
        classified[i] = winsWithout({ i }, nullptr, &remainingPaths) ? 0 : 1;
        auto lostPaths = basePaths & ~remainingPaths;
        for (size_t t = 0; t < HintPathTargets().size(); ++t) {
            if (!(lostPaths & (uint64_t{ 1 } << t)))
                continue;
            const std::string label = HintPathTargets()[t].label;
            const auto& p = placements[i];
            result.pathsByCheck[checkKey(p)].push_back(label);
            const auto& areas = p.checkGame == GAME_OOT ? ootCheckAreas : mmCheckAreas;
            auto area = areas.find(p.check);
            if (area != areas.end()) {
                auto& labels = result.pathsByArea[(p.checkGame == GAME_OOT ? "oot:" : "mm:") + area->second];
                if (std::find(labels.begin(), labels.end(), label) == labels.end())
                    labels.push_back(label);
            }
        }
        if (progress)
            progress->placed.fetch_add(1);
    }

    for (size_t pi : candAll) {
        const auto& p = placements[pi];
        bool required = classified[pi] == 1;
        result.requiredByCheck[checkKey(p)] = required;
        const auto& areaMap = (p.checkGame == GAME_OOT) ? ootCheckAreas : mmCheckAreas;
        auto ait = areaMap.find(p.check);
        if (ait != areaMap.end()) {
            std::string areaKey = (p.checkGame == GAME_OOT ? "oot:" : "mm:") + ait->second;
            if (required)
                result.areaHasRequired[areaKey] = true;
            else
                result.areaHasRequired.emplace(areaKey, false);
        }
    }
    result.ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();

    if (mmRestore)
        mmRestore();

    std::cout << "[PLAYTHROUGH] pare-down: " << result.candidateCount << " advancement candidates, "
              << result.replayedCount << " oracle replays, " << result.ms << " ms\n";
    return result;
}

struct PlaythroughResult {
    bool beatable = false;
    int beatableSphere = -1;
    size_t reachableOot = 0, unreachableOot = 0;
    size_t reachableMm = 0, unreachableMm = 0;
    // Win-side reachability at the collected fixed point — names which side blocks a stuck seed
    // (Ganon = OOT tower-top + Boss Key; Majora = combat-capable MM lair). Under a Triforce Hunt these
    // stay the bosses; piecesReachable counts the pieces on checks reached at that fixed point.
    bool ganonReachable = false, majoraReachable = false;
    bool hunt = false;
    int piecesReachable = 0, piecesRequired = 0;
};

// Endgame proxies the oracles actually emit (see ComboShip.cpp for the rationale — the literal "Ganon"
// check needs CanUse(RG_MASTER_SWORD), an equip flag the headless engine doesn't model, so we use
// tower-top + Boss Key owned; MM adds the actual combat requirements in its virtual defeat marker).
// sohDumpJson/mmDumpJson (optional): static-data dumps whose pool[]/fixed[] advancement flags let the
// text log show only progression items; unknown names default to advancement so nothing is hidden.
// progressionOnly also drops junk from playthroughOut and ends it at the winning sphere — for the
// spoiler, which lists the route to the goal. Leave it false for validation: the affordability check
// needs every step of every sphere, including junk sitting in a priced shop slot.
inline PlaythroughResult RunPlaythrough(const std::string& spoilerJson, const OracleFns& ootOracle,
                                        const OracleFns& mmOracle, const std::string& seedLabel, void (*mmRestore)(),
                                        nlohmann::json* playthroughOut = nullptr, const std::string& sohDumpJson = "",
                                        const std::string& mmDumpJson = "", bool portalGated = true,
                                        bool progressionOnly = false, CwGoal goal = {}, bool mmStart = false,
                                        uint32_t sharedMask = 0) {
    static const char* kOotGanon = "Ganon";           // RC_GANON reachable = OOT beatable (see CrossWorldRando.h)
    static const char* kMmWin = "Defeat Majora";

    PlaythroughResult result;
    result.hunt = goal.hunt;
    result.piecesRequired = goal.required;

    using Placed = CwPlacedItem;
    std::vector<Placed> placements = ParseSpoilerPlacements(spoilerJson, sohDumpJson, mmDumpJson);

    auto queryReachable = [&](const OracleFns& o, const std::vector<std::string>& owned) {
        return QueryReachableWithMmStock(o, owned, &o == &mmOracle, placements);
    };

    const auto starts = SharedStartingInventories(sharedMask, sohDumpJson, mmDumpJson);
    std::vector<std::string> ownedOot = starts.first, ownedMm = starts.second;
    const bool fairyOcarinaMm =
        nlohmann::json::parse(mmDumpJson.empty() ? "{}" : mmDumpJson).value("fairyOcarinaMm", false);
    std::unordered_set<std::string> collected; // "<cg>:<cn>"
    std::ostringstream log;
    log << "Cross-world playthrough - seed '" << seedLabel << "'\n";
    if (goal.hunt) {
        log << "Beatable when: " << goal.required << " Triforce Pieces collected across both games.\n\n";
    } else {
        log << "Beatable when the combined goal is reached: " << CwGoalName(goal) << ".\n\n";
    }

    int beatableSphere = -1, spheresWalked = 0;
    const int kMaxSpheres = 200;
    // Latched: MM stays open once OOT can reach the Happy Mask Shop. Ungated (NO_LOGIC) = open, and an
    // MM start (#135) roots MM from the beginning, as it does in the fill.
    const bool mmRoot = !portalGated || mmStart;
    bool portalOpen = mmRoot;
    size_t sharedGiven[SF_COUNT] = { 0 };
    for (int sphere = 0; sphere < kMaxSpheres; ++sphere) {
        ApplySharedMirror(sharedMask, ownedOot, ownedMm, sharedGiven, fairyOcarinaMm);
        auto worlds = QuerySongWorlds(ootOracle, mmOracle, ownedOot, ownedMm, portalOpen, queryReachable, mmRoot);
        auto& ootReach = worlds.first;
        auto& mmReach = worlds.second;
        bool canGanon = ootReach.count(kOotGanon) > 0;
        bool canMajora = mmReach.count(kMmWin) > 0;
        if (CwGoalReached(goal, canGanon, canMajora, ownedOot, ownedMm)) {
            if (beatableSphere < 0)
                beatableSphere = sphere;
            // Continue collecting to a fixed point. Winning does not prove optional
            // checks accessible, and assuming all placed items would hide circular locks.
        }
        // The spoiler's route ends where the goal is reached; spheres after it are not on it.
        const bool recordSteps = playthroughOut && !(progressionOnly && beatableSphere >= 0);

        std::vector<Placed> newly;
        for (auto& p : placements) {
            std::string key = (p.checkGame == GAME_OOT ? "oot:" : "mm:") + p.check;
            if (collected.count(key))
                continue;
            const auto& reach = (p.checkGame == GAME_OOT) ? ootReach : mmReach;
            if (reach.count(p.check))
                newly.push_back(p);
        }
        if (newly.empty()) {
            log << "Sphere " << sphere
                << (beatableSphere < 0 ? ": (stuck before the goal)\n" : ": (collected fixed point after the goal)\n");
            break;
        }
        ++spheresWalked;
        size_t newlyAdv = 0;
        for (auto& p : newly)
            newlyAdv += p.advancement ? 1 : 0;
        log << "Sphere " << sphere << "  (";
        if (goal.hunt) {
            log << "pieces=" << CountOwnedTriforcePieces(ownedOot, ownedMm) << "/" << goal.required;
        } else {
            log << "Ganon=" << (canGanon ? "Y" : "n") << " Majora=" << (canMajora ? "Y" : "n");
        }
        log << ", +" << newly.size() << " items, " << newlyAdv << " progression)\n";
        nlohmann::json sphereSteps = nlohmann::json::array();
        for (auto& p : newly) {
            std::string key = (p.checkGame == GAME_OOT ? "oot:" : "mm:") + p.check;
            collected.insert(key);
            (p.itemGame == GAME_OOT ? ownedOot : ownedMm).push_back(p.item);
            if (recordSteps && (p.advancement || !progressionOnly)) {
                nlohmann::json step = { { "game", p.checkGame == GAME_OOT ? "oot" : "mm" },
                                        { "check", p.check },
                                        { "item", p.item },
                                        { "foreign", p.checkGame != p.itemGame } };
                if (!progressionOnly)
                    step["advancement"] = p.advancement; // implied in the spoiler; the validator needs it
                sphereSteps.push_back(std::move(step));
            }
            // Junk is still collected (it drives the traversal) but not printed.
            if (!p.advancement)
                continue;
            log << "    [" << (p.checkGame == GAME_OOT ? "OOT" : "MM ") << "] " << p.check << "  <-  " << p.item
                << (p.checkGame != p.itemGame ? (p.itemGame == GAME_OOT ? "  (OOT item)" : "  (MM item)") : "") << "\n";
        }
        if (recordSteps)
            playthroughOut->push_back({ { "sphere", sphere }, { "steps", std::move(sphereSteps) } });
    }

    // Reachability after collecting only checks that actually opened from the empty start.
    // Never grant the contents of inaccessible checks to prove those same checks accessible.
    ApplySharedMirror(sharedMask, ownedOot, ownedMm, sharedGiven, fairyOcarinaMm);
    auto everWorlds = QuerySongWorlds(ootOracle, mmOracle, ownedOot, ownedMm, portalOpen, queryReachable, mmRoot);
    auto& everReachOot = everWorlds.first;
    auto& everReachMm = everWorlds.second;
    result.ganonReachable = everReachOot.count(kOotGanon) > 0;
    result.majoraReachable = everReachMm.count(kMmWin) > 0;
    if (goal.hunt) {
        for (auto& p : placements) {
            if (CwIsTriforcePiece(p.itemGame, p.item) &&
                (p.checkGame == GAME_OOT ? everReachOot : everReachMm).count(p.check)) {
                ++result.piecesReachable;
            }
        }
    }

    if (mmRestore)
        mmRestore();

    if (beatableSphere >= 0 && goal.hunt) {
        log << "\nBEATABLE at sphere " << beatableSphere << ": " << goal.required
            << " Triforce Pieces collected. Seed is completable.\n";
    } else if (beatableSphere >= 0) {
        log << "\nBEATABLE at sphere " << beatableSphere << ": combined goal " << CwGoalName(goal)
            << " reached. Seed is completable.\n";
    } else {
        log << "\nNOT proven beatable within " << kMaxSpheres << " spheres (see stuck note above).\n";
    }

    auto emitGame = [&](GameId cg, const char* tag, const std::unordered_set<std::string>& everReach,
                        size_t& reachedOut, size_t& missingOut) {
        size_t reached = 0, missing = 0;
        log << "\n--- " << tag << " placements ---\n";
        for (auto& p : placements) {
            if (p.checkGame != cg)
                continue;
            bool got = everReach.count(p.check) > 0;
            got ? ++reached : ++missing;
            // Reached junk is noise; unreachable stays visible regardless (it's the diagnostic).
            if (got && !p.advancement)
                continue;
            log << "    " << (got ? "  " : "! ") << p.check << "  <-  " << p.item
                << (p.checkGame != p.itemGame ? (p.itemGame == GAME_OOT ? "  (OOT item)" : "  (MM item)") : "")
                << (got ? "" : "   [UNREACHABLE]") << "\n";
        }
        log << "  " << tag << ": " << reached << " reachable, " << missing << " unreachable\n";
        reachedOut = reached;
        missingOut = missing;
    };
    log << "\n==== Placement (progression + unreachable; counts cover all checks) ====\n";
    emitGame(GAME_OOT, "OOT", everReachOot, result.reachableOot, result.unreachableOot);
    emitGame(GAME_MM, "MM", everReachMm, result.reachableMm, result.unreachableMm);

    // In-game generation folds the playthrough into the consolidated spoiler (playthroughOut), so the
    // standalone text log is redundant there. Callers passing no playthroughOut get the .txt.
    if (!playthroughOut) {
        std::error_code ec;
        std::filesystem::create_directories("saves/combo", ec);
        std::ofstream f("saves/combo/slot0.playthrough.txt", std::ios::trunc);
        f << log.str();
        std::cout << "[PLAYTHROUGH] full sphere log -> saves/combo/slot0.playthrough.txt\n";
    }

    std::cout << "[PLAYTHROUGH] seed '" << seedLabel << "' - " << (beatableSphere >= 0 ? "BEATABLE" : "NOT beatable")
              << (beatableSphere >= 0 ? (" at sphere " + std::to_string(beatableSphere)) : "") << "\n";
    std::cout << "[PLAYTHROUGH] collected " << collected.size() << " items across " << spheresWalked
              << " spheres through the reachability fixed point\n";

    result.beatable = beatableSphere >= 0;
    result.beatableSphere = beatableSphere;
    return result;
}

// Flatten RunPlaythrough's structured spheres into player-facing lines, "[OOT] Check --> Item". The
// validator keeps consuming the structured form; only the spoiler shows these.
inline nlohmann::json PlaythroughLines(const nlohmann::json& spheres) {
    nlohmann::json out = nlohmann::json::array();
    for (const auto& sphere : spheres) {
        nlohmann::json steps = nlohmann::json::array();
        for (const auto& step : sphere.value("steps", nlohmann::json::array())) {
            // Which game owns the item isn't actionable for the player, so cross-game isn't marked.
            steps.push_back("[" + std::string(step.value("game", "") == "mm" ? "MM" : "OOT") + "] " +
                            step.value("check", "") + " --> " + step.value("item", ""));
        }
        out.push_back({ { "sphere", sphere.value("sphere", -1) }, { "steps", std::move(steps) } });
    }
    return out;
}

} // namespace ComboRando
