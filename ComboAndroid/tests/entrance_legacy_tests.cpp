// Fixture captured against b74917af, before separating the legacy registry identity.
#include "rando/entrances/MmGrottoNativeBridge.h"
#include "rando/entrances/MmMixedNativeBridge.h"
#include "rando/entrances/MmOrdinaryNativeBridge.h"
#include <cassert>
#include <fstream>
#include <iostream>

using namespace ComboRando::Entrances;
using Json = nlohmann::json;

static Overrides SwapPair(std::string_view aName, std::string_view bName) {
    const auto* a = Find(aName);
    const auto* b = Find(bName);
    assert(a && b && Reverse(*a) && Reverse(*b));
    return { { a->name, b->name }, { b->name, a->name }, { a->reverse, b->reverse }, { b->reverse, a->reverse } };
}

static Json Snapshot() {
    Settings settings;
    settings.games = Games::Mm;
    settings.erNoPolarity = true;
    settings.erSelfLoops = true;
    settings.erOverworld = Mode::OwnGame;
    const auto ordinaryRows = SwapPair("MM_BOMB_SHOP", "MM_CHEST_GAME");
    const auto grottoRows = SwapPair("MM_GROTTO_GENERIC_FIELD_PILLAR", "MM_GROTTO_GENERIC_GRASS");
    const auto mixedRows = SwapPair("MM_GROTTO_GENERIC_FIELD_PILLAR", "MM_BOMB_SHOP");
    Json result;
    auto& envelopes = result["envelopes"];
    envelopes["ordinary"] = EncodeMmOrdinaryExtension(ordinaryRows);
    // Some existing slots contain the historical reservation; it must remain native.
    envelopes["ordinary"]["overrides"].push_back(
        { { "source", "MM_CLOCK_TOWER_FROM_CLOCK_TOWN" }, { "destination", "MM_CLOCK_TOWN_FROM_CLOCK_TOWER" } });
    envelopes["grotto"] = EncodeMmGrottoExtension(grottoRows);
    envelopes["mixed"] = EncodeMmMixedExtension(mixedRows, settings);
    std::vector<SavedOverride> wallmasterRows;
    Overrides wallmasterOverrides;
    for (size_t i = 0; i < kMmWallmasterSources.size(); ++i) {
        const auto* a = Find(kMmWallmasterSources[i]);
        const auto* b = Find(kMmBaseWallmasterDestinations[i]);
        assert(a && b);
        wallmasterRows.push_back({ std::string(a->name), std::string(b->name), a->game, b->game, a->id, b->id });
        wallmasterOverrides.emplace(a->name, b->name);
    }
    envelopes["wallmaster"] = EncodeMmWallmasterExtension(wallmasterRows);
    auto& assumptions = result["assumptions"];
    assumptions["ordinary"] = EncodeMmOrdinaryAssumptions(ordinaryRows, {}, settings);
    assumptions["grotto"] = EncodeMmGrottoAssumptions(grottoRows, {}, settings);
    assumptions["mixed"] = EncodeMmMixedAssumptions(mixedRows, {}, settings);
    assumptions["wallmaster"] = EncodeMmWallmasterAssumptions(wallmasterOverrides, {});

    // Every record used by a legacy native adapter must retain all semantic metadata.
    // Source line numbers can move between upstream revisions and are not routing data.
    result["boundRecords"] = Json::array();
    for (const auto& r : kRegistry)
        if (MmOrdinaryBound(r.name) || MmGrottoBound(r.name) || Contains(kMmWallmasterSources, r.name) ||
            MmBaseDestination(r, true))
            result["boundRecords"].push_back({ r.name, r.game == Game::Mm ? "mm" : "oot", r.id,
                                               static_cast<int>(r.type), r.reverse, r.from, r.to, r.fromMap, r.toMap,
                                               r.fromSubmap, r.toSubmap, r.noSongOfTime });
    return result;
}

static void CheckDecoded(const Json& fixture) {
    const auto& e = fixture.at("envelopes");
    MmOrdinaryLayout ordinary;
    MmGrottoLayout grotto;
    MmMixedLayout mixed;
    ordinary.install(e.at("ordinary"));
    grotto.install(e.at("grotto"));
    mixed.install(e.at("mixed"));
    assert(ordinary.json() == e.at("ordinary") && grotto.json() == e.at("grotto") && mixed.json() == e.at("mixed"));
    assert(!ordinary.containsSource(0xc010u) && ordinary.destination(0xc010u) == 0xc010u);
    // Numeric expectations are frozen independently of today's registry lookups. In particular,
    // the incoming and outgoing full-width grotto IDs must not alias their truncated low words.
    assert(grotto.destination({ Game::Mm, 0x10000u }).id == 0x10001u);
    assert(grotto.destination({ Game::Mm, 0x10101u }).id == 0x10100u);
    assert(!grotto.containsSource({ Game::Mm, 0u }) && !grotto.containsSource({ Game::Mm, 0x101u }));
    assert(grotto.destination({ Game::Oot, 0x10000u }).id == 0x10000u);
    assert(mixed.destination(0xca00u) == 0x10000u && mixed.destination(0x10000u) == 0xca00u);
    assert(mixed.destination(0x10100u) == 0xd460u);
    assert(!mixed.containsSource(0u) && !mixed.containsSource(0x100u));
    const auto wallmasterRows = DecodeMmWallmasterSeed({ { "entranceExpansion", e.at("wallmaster") } });
    assert(wallmasterRows && EncodeMmWallmasterExtension(*wallmasterRows) == e.at("wallmaster"));
    MmWallmasterLayout wallmaster;
    wallmaster.install(*wallmasterRows);
    for (const auto& row : *wallmasterRows)
        assert(wallmaster.destination(row.source) == row.destinationId);
    const auto& a = fixture.at("assumptions");
    assert(DecodeMmOrdinaryAssumptions(a.at("ordinary")).routes ==
           BuildMmOrdinaryAssumptions(SwapPair("MM_BOMB_SHOP", "MM_CHEST_GAME"), {}, Settings{}).routes);
    assert(DecodeMmGrottoAssumptions(a.at("grotto")).routes.at(0x10000u) == std::set<uint32_t>{ 0x10001u });
    assert(DecodeMmMixedAssumptions(a.at("mixed")).routes.at(0xca00u) == std::set<uint32_t>{ 0x10000u });
    assert(DecodeMmWallmasterAssumptions(a.at("wallmaster")).enabled);
    // Wrong identity must reject transactionally, retaining the previous installed layout.
    for (const auto key : { "upstreamCommit", "registrySha256" }) {
        auto bad = e.at("mixed");
        bad[key] = "different registry";
        bool rejected = false;
        try {
            mixed.install(bad);
        } catch (const std::exception&) { rejected = true; }
        assert(rejected && mixed.json() == e.at("mixed") && mixed.destination(0xca00u) == 0x10000u);
    }
}

int main(int argc, char** argv) {
    const auto current = Snapshot();
    if (argc == 1) { // Explicit baseline capture only; CTest always passes the checked-in fixture.
        std::cout << current.dump(2) << '\n';
        return 0;
    }
    assert(argc == 2);
    std::ifstream input(argv[1]);
    const auto fixture = Json::parse(input);
    assert(current == fixture);
    CheckDecoded(fixture);
    std::cout << "Four legacy envelopes, assumption contracts, bound metadata and full-width routes unchanged.\n";
}
