// Isolated pinned MM coupled-grotto saved identity contract. See LICENSE.OoTMM.
#pragma once
#include "rando/entrances/LegacyEnvelope.h"
#include "rando/entrances/MmGrottoBindings.h"
#include <map>
#include <nlohmann/json.hpp>
namespace ComboRando::Entrances {
class MmGrottoLayout {
    std::map<uint32_t, uint32_t> routes;
    nlohmann::json saved;
    static bool Supported(const Record* r) {
        return IsMmGrottoRecord(r) && MmGrottoBound(r->name) && !r->reverse.empty();
    }

  public:
    bool active() const {
        return !saved.is_null();
    }
    void clear() {
        routes.clear();
        saved = nlohmann::json();
    }
    const nlohmann::json& json() const {
        return saved;
    }
    bool containsSource(Key k) const {
        return k.game == Game::Mm && routes.contains(k.id);
    }
    Key destination(Key source) const {
        if (source.game != Game::Mm)
            return source;
        auto it = routes.find(source.id);
        return it == routes.end() ? source : Key{ Game::Mm, it->second };
    }
    void install(const nlohmann::json& value) {
        MmGrottoLayout next;
        if (!value.is_object() || value.value("schemaVersion", 0) != 1 ||
            value.value("profile", std::string()) != "mm-grotto-coupled-v1" ||
            value.value("upstreamCommit", std::string()) != kLegacyUpstreamCommit ||
            value.value("registrySha256", std::string()) != kLegacyRegistrySourceSha256 ||
            !value.contains("overrides") || !value.at("overrides").is_array())
            throw std::invalid_argument("Unsupported MM grotto envelope");
        if (value.contains("noPolarity") && !value.at("noPolarity").is_boolean())
            throw std::invalid_argument("Malformed MM grotto polarity");
        const bool noPolarity = value.value("noPolarity", false);
        std::map<std::string, std::string> named;
        for (const auto& row : value.at("overrides")) {
            if (!row.is_object() || !row.contains("source") || !row.at("source").is_string() ||
                !row.contains("destination") || !row.at("destination").is_string())
                throw std::invalid_argument("Malformed MM grotto row");
            auto src = row.at("source").get<std::string>(), dst = row.at("destination").get<std::string>();
            auto a = Find(src), b = Find(dst);
            if (!Supported(a) || !Supported(b) || (!noPolarity && MmGrottoInput(*a) != MmGrottoInput(*b)))
                throw std::invalid_argument("Unsupported MM grotto route");
            if (!named.emplace(src, dst).second || !next.routes.emplace(a->id, b->id).second)
                throw std::invalid_argument("Duplicate MM grotto source");
        }
        for (const auto& [src, dst] : named) {
            auto a = Find(src), b = Find(dst);
            auto reverse = named.find(std::string(b->reverse));
            if (reverse == named.end() || reverse->second != a->reverse)
                throw std::invalid_argument("Uncoupled MM grotto return");
        }
        next.saved = value;
        *this = std::move(next);
    }
};
inline MmGrottoLayout& ActiveMmGrottos() {
    static MmGrottoLayout layout;
    return layout;
}

} // namespace ComboRando::Entrances
