// combo/rando/PlandoSeed.h
// ComboShip: the seed round trip behind Randomizer > Plandomizer (ComboMenu.cpp PlandoLoad / PlandoSavePlay).
// Load reads a consolidated seed into editable rows; Save writes the rows back into that seed. A row the
// player did not edit keeps the seed's own placement text and foreign[] entry, in the seed's order, so a
// Load followed by a Save of an unedited seed returns it exactly as generated. Only edited rows are rebuilt,
// the way the generator writes them (SuffixCrossGameItems + BuildForeignArray).
#pragma once

#include <algorithm>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <nlohmann/json.hpp>

#include "ComboPlaythrough.h"
#include "CrossForeign.h"
#include "SharedItems.h"

namespace ComboRando {

inline std::string PlandoRowKey(const std::string& checkGame, const std::string& check) {
    return checkGame + "|" + check;
}

inline std::string PlandoRowKey(const CwPlacedItem& row) {
    return PlandoRowKey(row.checkGame == GAME_OOT ? "oot" : "mm", row.check);
}

// The editable rows of a consolidated seed, sorted by game and check for the editor's table. The generator
// writes a foreign[] entry's "advancement" only when true (BuildForeignArray), so a missing one is false
// here. ParseSpoilerPlacements keeps its own default (true) for its other callers.
inline std::vector<CwPlacedItem> PlandoRowsFromSeed(const nlohmann::json& seed, const std::string& sohDump,
                                                    const std::string& mmDump) {
    nlohmann::json flat;
    flat["oot"] = seed.value("oot", nlohmann::json::object()).value("placements", nlohmann::json::object());
    flat["mm"] = seed.value("mm", nlohmann::json::object()).value("placements", nlohmann::json::object());
    flat["foreign"] = nlohmann::json::array();
    for (auto fm : seed.value("foreign", nlohmann::json::array())) {
        if (fm.is_object() && !fm.contains("advancement"))
            fm["advancement"] = false;
        flat["foreign"].push_back(std::move(fm));
    }
    auto rows = ParseSpoilerPlacements(flat.dump(), sohDump, mmDump);
    std::sort(rows.begin(), rows.end(), [](const CwPlacedItem& a, const CwPlacedItem& b) {
        if (a.checkGame != b.checkGame)
            return a.checkGame < b.checkGame;
        return a.check < b.check;
    });
    return rows;
}

// `seed` with its item placements and foreign[] written from `rows`. `loaded` holds the rows as Load read
// them from this same seed: a row whose item is unchanged keeps the seed's placement text and foreign[]
// entry as they are, flags included (a trap stays a trap, a missing "advancement" stays missing). An
// edited row gets the generator's suffixing and foreign entry. foreign[] keeps the seed's order: an edited
// row's entry takes its old place, a row that became native drops out, a row that became foreign goes last.
inline nlohmann::json PlandoSeedFromRows(nlohmann::json seed, const std::vector<CwPlacedItem>& rows,
                                         const std::vector<CwPlacedItem>& loaded, const std::string& sohDump,
                                         const std::string& mmDump) {
    std::unordered_map<std::string, const CwPlacedItem*> before;
    for (const auto& row : loaded)
        before.emplace(PlandoRowKey(row), &row);
    auto unchanged = [&before](const CwPlacedItem& row) {
        auto it = before.find(PlandoRowKey(row));
        return it != before.end() && it->second->itemGame == row.itemGame && it->second->item == row.item;
    };
    const nlohmann::json oldOot =
        seed.value("oot", nlohmann::json::object()).value("placements", nlohmann::json::object());
    const nlohmann::json oldMm =
        seed.value("mm", nlohmann::json::object()).value("placements", nlohmann::json::object());
    const nlohmann::json oldForeign = seed.value("foreign", nlohmann::json::array());

    // Every row as the generator would write it; only the edited ones are used below.
    nlohmann::json ootPl = nlohmann::json::object();
    nlohmann::json mmPl = nlohmann::json::object();
    nlohmann::json foreignRaw = nlohmann::json::array();
    for (const auto& r : rows) {
        // The check's own game stores the REAL foreign item's bare name; the sentinel is injected at
        // apply time by Combo_OnReloadRequest, never written here.
        (r.checkGame == GAME_OOT ? ootPl : mmPl)[r.check] = r.item;
        if (r.checkGame != r.itemGame) {
            foreignRaw.push_back({ { "checkGame", r.checkGame == GAME_OOT ? "oot" : "mm" },
                                   { "checkName", r.check },
                                   { "itemGame", r.itemGame == GAME_OOT ? "oot" : "mm" },
                                   { "itemName", r.item },
                                   { "displayName", r.item }, // BuildForeignArray appends the game suffix
                                   { "advancement", r.advancement } });
        }
    }
    // Shared Items: the seed's own effective mask. The Plandomizer changes where items land, not which
    // families are shared. Native cross-game name collisions get their own-game suffix; foreign checks are
    // skipped (their real item travels in foreign[]). Exactly the generator's write path.
    const uint32_t sharedMask = SharedMaskFromKeys(seed.value("sharedItems", nlohmann::json::array()));
    SuffixCrossGameItems(ootPl, mmPl, foreignRaw, sohDump, mmDump, SharedUntaggedNames(sharedMask));
    std::unordered_map<std::string, nlohmann::json> rebuilt;
    for (auto& entry : BuildForeignArray(foreignRaw, sharedMask))
        rebuilt.emplace(PlandoRowKey(entry.value("checkGame", ""), entry.value("checkName", "")), std::move(entry));

    nlohmann::json ootOut = oldOot.is_object() ? oldOot : nlohmann::json::object();
    nlohmann::json mmOut = oldMm.is_object() ? oldMm : nlohmann::json::object();
    std::unordered_map<std::string, const CwPlacedItem*> rowByKey;
    for (const auto& r : rows) {
        rowByKey.emplace(PlandoRowKey(r), &r);
        if (unchanged(r))
            continue;
        const bool oot = r.checkGame == GAME_OOT;
        (oot ? ootOut : mmOut)[r.check] = (oot ? ootPl : mmPl)[r.check];
    }
    nlohmann::json foreign = nlohmann::json::array();
    std::unordered_set<std::string> seen; // checks the seed's foreign[] names
    for (const auto& fm : oldForeign) {
        const std::string key =
            fm.is_object() ? PlandoRowKey(fm.value("checkGame", ""), fm.value("checkName", "")) : std::string();
        seen.insert(key);
        auto row = rowByKey.find(key);
        if (row == rowByKey.end() || unchanged(*row->second)) {
            foreign.push_back(fm); // not edited: as the seed has it
            continue;
        }
        auto entry = rebuilt.find(key);
        if (entry != rebuilt.end()) {
            foreign.push_back(std::move(entry->second));
            rebuilt.erase(entry); // one entry per check
        }
    }
    for (const auto& r : rows) {
        const std::string key = PlandoRowKey(r);
        auto entry = rebuilt.find(key);
        if (!unchanged(r) && !seen.count(key) && entry != rebuilt.end())
            foreign.push_back(std::move(entry->second));
    }

    seed["oot"]["placements"] = std::move(ootOut);
    seed["mm"]["placements"] = std::move(mmOut);
    seed["foreign"] = std::move(foreign);
    return seed;
}

} // namespace ComboRando
