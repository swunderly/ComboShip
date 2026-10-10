#pragma once
#include "MmOrdinaryBindings.h"
#include "rando/entrances/Context.h"
#include "rando/entrances/Shuffle.h"
#include <map>
#include <nlohmann/json.hpp>
namespace ComboRando::Entrances {
// Pinned Play_EntranceForOverride canonical seasonal source aliases.
inline uint32_t CanonicalMmOrdinarySource(uint32_t entrance) {
    auto key = entrance >> 9;
    switch (key) {
        case 0x06:
            key = 0x42;
            break;
        case 0x57:
            key = 0x4d;
            break;
        case 0x45:
            key = 0x4a;
            break;
        case 0x5b:
            key = 0x5a;
            break;
    }
    return (key << 9) | (entrance & 0x1ff);
}
// Saved ordinary MM routes use the actual pinned registry, never modulo/truncated IDs.
class MmOrdinaryLayout {
    std::map<uint32_t, uint32_t> physical;
    nlohmann::json saved;

  public:
    void clear() {
        physical.clear();
        saved = nlohmann::json();
    }
    bool active() const {
        return !saved.is_null();
    }
    bool containsSource(uint32_t source) const {
        return physical.contains(CanonicalMmOrdinarySource(source));
    }
    const nlohmann::json& json() const {
        return saved;
    }
    uint32_t destination(uint32_t source) const {
        auto i = physical.find(CanonicalMmOrdinarySource(source));
        return i == physical.end() ? source : i->second;
    }
    void install(const nlohmann::json& value) {
        MmOrdinaryLayout next;
        if (!value.is_object() || value.value("schemaVersion", 0) != 1 ||
            value.value("profile", std::string()) != "mm-ordinary-coupled-v1" ||
            value.value("upstreamCommit", std::string()) != kUpstreamCommit ||
            value.value("registrySha256", std::string()) != kRegistrySourceSha256 || !value.contains("overrides") ||
            !value["overrides"].is_array())
            throw std::invalid_argument("Unsupported MM ordinary entrance envelope");
        std::map<std::string, std::string> named;
        for (const auto& row : value["overrides"]) {
            if (!row.is_object() || !row.contains("source") || !row["source"].is_string() ||
                !row.contains("destination") || !row["destination"].is_string())
                throw std::invalid_argument("Malformed MM ordinary override");
            auto src = row["source"].get<std::string>(), dst = row["destination"].get<std::string>();
            // Older seeds saved this row; ignoring it keeps the Clock Tower door the link to OoT.
            if (IsMmGrottoClockTowerReservation(src, dst))
                continue;
            auto a = Find(src), b = Find(dst);
            if (!a || !b || a->game != Game::Mm || b->game != Game::Mm || a->id > 0xffff || b->id > 0xffff ||
                !MmOrdinaryBound(src) || !MmOrdinaryBound(dst) || a->reverse.empty() || b->reverse.empty())
                throw std::invalid_argument("Unbound MM ordinary route");
            if (!named.emplace(src, dst).second)
                throw std::invalid_argument("Duplicate ordinary source");
            for (const auto& bind : kMmOrdinaryBindings)
                if (bind.name == src) {
                    auto [it, ok] = next.physical.emplace(bind.physicalId, b->id);
                    if (!ok && it->second != b->id)
                        throw std::invalid_argument("Ambiguous native MM source aliases");
                }
        }
        for (const auto& [src, dst] : named) {
            auto a = Find(src), b = Find(dst);
            auto reverse = named.find(std::string(b->reverse));
            if (reverse == named.end() || reverse->second != a->reverse)
                throw std::invalid_argument("Uncoupled MM ordinary return");
        }
        next.saved = value;
        *this = std::move(next);
    }
};
inline MmOrdinaryLayout& ActiveMmOrdinary() {
    static MmOrdinaryLayout value;
    return value;
}
} // namespace ComboRando::Entrances
