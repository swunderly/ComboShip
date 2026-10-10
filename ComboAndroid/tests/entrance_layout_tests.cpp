#include "rando/entrances/MmOrdinaryGenerationSettings.h"
#include "rando/entrances/MmOrdinaryNativeBridge.h"
#include "rando/entrances/Wallmasters.h"
#include <cassert>
#include <iostream>
#include <vector>

using namespace ComboRando::Entrances;

static bool HasClockTowerReservation(const Overrides& rows) {
    const auto row = rows.find("MM_CLOCK_TOWER_FROM_CLOCK_TOWN");
    return row != rows.end() && IsMmGrottoClockTowerReservation(row->first, row->second);
}
static bool EncodesClockTower(const nlohmann::json& extension) {
    for (const auto& row : extension.at("overrides"))
        if (row.at("source") == "MM_CLOCK_TOWER_FROM_CLOCK_TOWN")
            return true;
    return false;
}

int main() {
    // The MM-only Shuffler reserves the Clock Tower door back to Clock Town. In ComboShip that door is
    // the native MM->OoT link, so no ordinary MM layout may store or route it (0xC010).
    for (int mode = 0; mode < 4; ++mode) {
        const auto settings = MmOrdinaryGenerationSettings(mode == 0, mode == 1, mode == 2, mode == 3);
        Bindings bindings;
        bindings.areaExists = MmOrdinaryNativeAreaExists;
        bindings.sourceSupported = [](const Record& record) { return MmOrdinaryBound(record.name); };
        bindings.destinationSupported = bindings.sourceSupported;
        bindings.validateAssumed = [](const Overrides&, const Pools&) { return true; };
        bindings.validateComplete = [](const Overrides&) { return true; };
        bindings.chooseIndex = [](size_t) { return size_t(0); };
        const auto rows = Shuffler(settings, std::move(bindings)).run();
        assert(HasClockTowerReservation(rows));

        // New seeds: the encoded envelope drops only the reservation.
        const auto extension = EncodeMmOrdinaryExtension(rows);
        assert(!EncodesClockTower(extension));
        assert(extension.at("overrides").size() == rows.size() - 1);
        MmOrdinaryLayout layout;
        layout.install(extension);
        assert(!layout.containsSource(0xc010u) && layout.destination(0xc010u) == 0xc010u);

        // Saved seeds that already hold the row still install unchanged, but never route it.
        auto saved = extension;
        saved["overrides"].push_back(
            { { "source", "MM_CLOCK_TOWER_FROM_CLOCK_TOWN" }, { "destination", "MM_CLOCK_TOWN_FROM_CLOCK_TOWER" } });
        MmOrdinaryLayout old;
        old.install(saved);
        assert(!old.containsSource(0xc010u) && old.destination(0xc010u) == 0xc010u);
        assert(old.json() == saved);

        // The generation logic graph does not model it either.
        const auto graph = BuildMmOrdinaryAssumptions(rows, {}, settings);
        assert(!graph.routes.empty() && !graph.routes.contains(0xc010u));
    }

    // Wallmaster capture follows the installed route of the scene/room's own source.
    std::vector<std::string_view> destinations;
    for (const auto name : kMmBaseWallmasterDestinations) {
        const auto* record = Find(name);
        if (record && record->noSongOfTime && record->game == Game::Mm && record->id <= 0xffffu)
            destinations.push_back(name);
    }
    assert(destinations.size() >= kMmWallmasterSources.size());
    std::vector<SavedOverride> rows;
    for (size_t i = 0; i < kMmWallmasterSources.size(); ++i) {
        // Reverse order, so a capture that read the wrong source would see another destination.
        const auto* src = Find(kMmWallmasterSources[i]);
        const auto* dst = Find(destinations[kMmWallmasterSources.size() - 1 - i]);
        rows.push_back({ std::string(src->name), std::string(dst->name), src->game, dst->game, src->id, dst->id });
    }
    MmWallmasterLayout wallmasters;
    assert(!wallmasters.capture(SceneContext::MmDampe, 0));
    wallmasters.install(rows);
    const auto expect = [&](SceneContext scene, uint8_t room, std::string_view source) {
        const auto captured = wallmasters.capture(scene, room);
        assert(captured && captured == wallmasters.destination(source));
        for (const auto& row : rows)
            if (row.source == source)
                assert(*captured == row.destinationId);
    };
    expect(SceneContext::MmDampe, 0, "MM_WALLMASTER_DAMPE");
    expect(SceneContext::MmBeneathWell, 1, "MM_WALLMASTER_BTW_ENTRANCE");
    expect(SceneContext::MmBeneathWell, 5, "MM_WALLMASTER_BTW_FOUNTAIN");
    expect(SceneContext::MmBeneathWell, 7, "MM_WALLMASTER_BTW_EXIT");
    assert(!wallmasters.capture(SceneContext::MmBeneathWell, 2));
    assert(!wallmasters.capture(SceneContext::Unknown, 1));
    std::cout << "MM ordinary layouts keep the Clock Tower door native in all four modes; Wallmaster capture "
                 "follows each source's installed route.\n";
}
