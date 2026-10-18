#include "combo_entrance_overlay.h"
#ifdef COMBO_BUILD
#include "SeedContext.h"
#include "location_access.h"
#include "souls/OotSouls.h"
#include "rando/entrances/OotUnifiedLayout.h"
#include <spdlog/spdlog.h>

namespace Rando::ComboEntrances {
namespace CE = ComboRando::Entrances;
namespace {
struct SourceEdge {
    std::string_view auditId, source;
    RandomizerRegion anchor;
    ConditionFn condition;
};
const std::array<SourceEdge, 11> edges = { {
#include "combo_oot_wallmaster_edges.inc"
} };
struct Arrival {
    std::string_view row;
    uint32_t logical;
    int native;
    RandomizerRegion region;
};
const std::array<Arrival, 11> arrivals = { {
#include "combo_oot_wallmaster_arrivals.inc"
} };

CE::OotQuests ReadQuests() {
    const auto context = Rando::Context::GetInstance();
    if (!context || !context->GetDungeons())
        throw std::runtime_error("OoT dungeon context is unavailable");
    // Same explicit order as EntranceLayout.h kOotMqDungeons; reads resolved context, never RNG.
    constexpr DungeonKey keys[] = { DEKU_TREE,          DODONGOS_CAVERN, JABU_JABUS_BELLY,       FOREST_TEMPLE,
                                    FIRE_TEMPLE,        WATER_TEMPLE,    SPIRIT_TEMPLE,          SHADOW_TEMPLE,
                                    BOTTOM_OF_THE_WELL, ICE_CAVERN,      GERUDO_TRAINING_GROUND, GANONS_CASTLE };
    static_assert(std::size(keys) == CE::kOotMqDungeons.size());
    CE::OotQuests result{};
    for (size_t i = 0; i < result.size(); ++i)
        result[i] = context->GetDungeons()->GetDungeon(keys[i])->IsMQ();
    return result;
}
} // namespace

std::vector<Entrance> OverlayExits(RandomizerRegion region) {
    if (!CE::ActiveOotUnified().active() && !CE::ActiveOotUnifiedAssumptions())
        return {};
    std::vector<Entrance> result;
    for (const auto& edge : edges) {
        if (edge.anchor != region)
            continue;
        const auto* source = CE::Find(edge.source);
        if (!source)
            throw std::runtime_error("OoT Wallmaster source registry changed");
        for (const auto& route : CE::OotUnifiedGraphRoutes(source->id)) {
            const auto arrival = std::find_if(arrivals.begin(), arrivals.end(),
                                              [&](const Arrival& a) { return a.logical == route.destination; });
            if (arrival == arrivals.end())
                throw std::runtime_error("OoT Wallmaster destination is not bound");
            // These objects never join Region::exits, the native ER pools, area propagation or its
            // entrance playthrough pointer list. They live only for this traversal call.
            Entrance exit(arrival->region, edge.condition, std::string(edge.auditId), false);
            exit.SetParentRegion(region);
            result.push_back(std::move(exit));
        }
    }
    return result;
}

int SetLayout(const char* seedJson) {
    CE::ActiveOotUnified().clear();
    CE::ActiveOotUnifiedAssumptions().reset();
    try {
        const auto seed = nlohmann::json::parse(seedJson ? seedJson : "null");
        if (!seed.is_object())
            throw std::invalid_argument("OoT entrance seed must be an object");
        if (!seed.contains("entranceLayout"))
            return 1; // Private MM profiles and ordinary seeds leave OoT native.
        CE::ActiveOotUnified().install(seed, CE::ActiveCapabilities());
        if (CE::ActiveOotUnified().hasOotRoutes()) {
            const auto quests = ReadQuests();
            CE::RequireVanillaOotQuests(quests);
            if (quests != *CE::ActiveOotUnified().quests())
                throw std::invalid_argument("OoT entrance layout quest context does not match the loaded seed");
        }
        return 1;
    } catch (const std::exception& e) {
        CE::ActiveOotUnified().clear();
        SPDLOG_ERROR("[ComboShip] OoT entrance layout rejected: {}", e.what());
        return 0;
    }
}
const char* DumpLayout() {
    static thread_local std::string result;
    nlohmann::json seed = nlohmann::json::object();
    if (CE::ActiveOotUnified().active())
        seed["entranceLayout"] = CE::ActiveOotUnified().json();
    result = seed.dump();
    return result.c_str();
}
int SetAssumptions(const char* candidateJson) {
    CE::ActiveOotUnifiedAssumptions().reset();
    try {
        CE::ActiveOotUnifiedAssumptions() = CE::DecodeOotUnifiedAssumptions(
            nlohmann::json::parse(candidateJson ? candidateJson : "null"), ReadQuests());
        return 1;
    } catch (const std::exception& e) {
        SPDLOG_ERROR("[ComboShip] OoT entrance assumptions rejected: {}", e.what());
        return 0;
    }
}
void ClearAssumptions() {
    CE::ActiveOotUnifiedAssumptions().reset();
}
const char* AreaContext() {
    static thread_local std::string result;
    try {
        const auto quests = ReadQuests();
        auto mq = nlohmann::json::object();
        for (size_t i = 0; i < quests.size(); ++i)
            mq[std::string(CE::kOotMqDungeons[i])] = quests[i];
        result = nlohmann::json{
            { "profile", "ootmm-oot-area-context-v1" },
            { "registry", CE::kRegistrySourceSha256 },
            { "ootMq", std::move(mq) }
        }.dump();
        return result.c_str();
    } catch (const std::exception& e) {
        SPDLOG_ERROR("[ComboShip] OoT entrance area context unavailable: {}", e.what());
        return nullptr;
    }
}
} // namespace Rando::ComboEntrances
#endif
