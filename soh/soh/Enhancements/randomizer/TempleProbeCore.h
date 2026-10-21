#pragma once

// Engine-free core of the generation-only opposite-age Temple probe (TempleProbe.cpp). Everything
// here is a template over the engine types, so the host suite compiles this exact code with stubs.
// Pinned contract, OoTMM 413573d: solver/entrances.ts:265-286 (validateAgeTemple) and
// pathfind/pathfind.ts:98-109, :823-836 (OOT SPAWN explored at both ages, starting items only).

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace Rando::TempleProbe {
// Native return codes; combo/rando/entrances/TempleAdapter.h maps them for the Shuffler.
enum Code : int {
    kReached = 1,
    kNotReached = 0,
    kUnsupported = -1,
    kViolation = -2,
    kCancelled = -3,
    kInternal = -4,
};

// A rejected request or a failed precondition.
struct Failure : std::runtime_error {
    int code;
    Failure(int code_, const std::string& message) : std::runtime_error(message), code(code_) {
    }
};

// Mirror of fill.cpp UpdateToDAccess (source-pinned by temple_probe_core_tests.py), with the parent
// passed in: the synthesized root edge and candidate edges evaluate an Entrance whose native parent
// is not the region they leave.
template <class Logic, class Entrance, class Region>
bool EdgePropagate(Logic& logic, const Entrance& entrance, const Region& parent, Region& connection) {
    bool ageTimePropagated = false;
    if (!connection.childDay && parent.childDay && entrance.CheckConditionAtAgeTime(logic.IsChild, logic.AtDay)) {
        connection.childDay = true;
        ageTimePropagated = true;
    }
    if (!connection.childNight && parent.childNight && entrance.CheckConditionAtAgeTime(logic.IsChild, logic.AtNight)) {
        connection.childNight = true;
        ageTimePropagated = true;
    }
    if (!connection.adultDay && parent.adultDay && entrance.CheckConditionAtAgeTime(logic.IsAdult, logic.AtDay)) {
        connection.adultDay = true;
        ageTimePropagated = true;
    }
    if (!connection.adultNight && parent.adultNight && entrance.CheckConditionAtAgeTime(logic.IsAdult, logic.AtNight)) {
        connection.adultNight = true;
        ageTimePropagated = true;
    }
    return ageTimePropagated;
}

// The candidate world as a value: native exits skipped, exits added under a source row's original
// condition, the root's one synthesized exit and the virtual GLOBAL node's warp-song exits. Region exit
// lists are never modified.
template <class Entrance> struct AddedEdge {
    size_t parent;
    const Entrance* entrance;
    size_t target;
};
template <class Entrance> struct Overlay {
    std::vector<const Entrance*> deleted; // sorted with std::less
    std::vector<AddedEdge<Entrance>> added;
    const Entrance* rootSpawn = nullptr; // Root Exits -> opposite spawn, a bare age check
    size_t rootSpawnTarget = 0;
    // GLOBAL (oot _system.yml:23-34): the warp songs of Root Exits under their native condition, each with
    // its native parent, the region the condition is evaluated in, and its warp region.
    std::vector<AddedEdge<Entrance>> global;
    // Synthetic source edges are values owned by this query. Pointees stay stable across copy/move;
    // none belongs to the native region lists or the live installed/assumption overlays.
    std::vector<std::shared_ptr<Entrance>> owned;
};

// W2: every region's age/time flags and addedToPool, restored on every exit path.
template <class Regions> class RegionFlagGuard {
  public:
    explicit RegionFlagGuard(Regions& regions_) : regions(regions_) {
        saved.reserve(std::size(regions));
        for (const auto& r : regions) {
            saved.push_back({ r.childDay, r.childNight, r.adultDay, r.adultNight, r.addedToPool });
        }
    }
    ~RegionFlagGuard() {
        size_t i = 0;
        for (auto& r : regions) {
            const auto& s = saved[i++];
            r.childDay = s[0];
            r.childNight = s[1];
            r.adultDay = s[2];
            r.adultNight = s[3];
            r.addedToPool = s[4];
        }
    }
    RegionFlagGuard(const RegionFlagGuard&) = delete;
    RegionFlagGuard& operator=(const RegionFlagGuard&) = delete;

  private:
    Regions& regions;
    std::vector<std::array<bool, 5>> saved;
};

// W1: points the predicate handle (::logic) at the probe's private Logic and marks the scope
// Rando::ActiveLogic() reads; both come back on every exit path.
template <class Handle> class ScopedLogic {
  public:
    ScopedLogic(Handle& handle_, Handle probe, bool& scope_) : handle(handle_), previous(handle_), scope(scope_) {
        handle = std::move(probe);
        scope = true;
    }
    ~ScopedLogic() {
        scope = false;
        handle = std::move(previous);
    }
    ScopedLogic(const ScopedLogic&) = delete;
    ScopedLogic& operator=(const ScopedLogic&) = delete;

  private:
    Handle& handle;
    Handle previous;
    bool& scope;
};

// One probe at a time: a nested call is rejected before it writes anything else.
inline int& Depth() {
    static int depth = 0;
    return depth;
}
struct DepthGuard {
    DepthGuard() {
        if (Depth() != 0) {
            throw Failure(kViolation, "Temple probe re-entered");
        }
        ++Depth();
    }
    ~DepthGuard() {
        --Depth();
    }
    DepthGuard(const DepthGuard&) = delete;
    DepthGuard& operator=(const DepthGuard&) = delete;
};

// The pinned recursive search without item collection. Region `root` stands for the virtual
// OOT SPAWN, seeded day-only at both ages like AccessReset; its only exit is the opposite spawn
// (ASSUMED is empty in the supported envelope), so neither Root Exits nor the starting spawn is ever
// reached. The virtual GLOBAL node is entered at the opposite spawn region (`rootSpawnTarget`): Link's
// House and the Temple of Time exit to GLOBAL unconditionally and the spawn regions lead only to them
// (lemma L3), so GLOBAL takes that region's age and time flags, which only the opposite age ever holds.
// The probe's GLOBAL therefore has day from the start, from the root, and gains night once a region of
// that age passes hours (ApplyTimePass). OoTMM's GLOBAL, a still area, has no time until an area exiting
// to it has some, which first comes from a flowing area. This cannot change the answer: the adult spawns
// in the Temple of Time, the child always reaches Hyrule Field, which flows (Closed Forest On forces a
// child start), and no exit gated on day or night lies between a non-flowing warp landing and the Temple
// of Time or a flowing area. Its warp songs are evaluated as their native parent, Root Exits, evaluates
// them. Events and time of day use the real Region code; no
// age ever changes (lemma L2) and no location is evaluated (Detail1). Reads the opposite age's flags of
// `target`.
template <class Regions, class Logic, class Entrance, class Cancelled>
int Search(Regions& regions, Logic& logic, const Overlay<Entrance>& overlay, size_t root, size_t target,
           bool startingChild, Cancelled&& cancelled, uint8_t& ages) {
    for (auto& r : regions) {
        r.childDay = r.childNight = r.adultDay = r.adultNight = false;
    }
    auto& spawn = regions[root];
    spawn.childDay = spawn.adultDay = true;
    std::vector<size_t> pool{ root };
    std::vector<bool> queued(std::size(regions));
    queued[root] = true;
    auto flags = [](const auto& r) {
        return std::array<bool, 4>{ r.childDay, r.childNight, r.adultDay, r.adultNight };
    };
    const std::less<const Entrance*> order;
    bool updated;
    do {
        updated = false;
        if (cancelled()) {
            return kCancelled;
        }
        for (size_t i = 0; i < pool.size(); ++i) {
            const size_t node = pool[i];
            auto& region = regions[node];
            logic.CurrentRegionKey = static_cast<decltype(logic.CurrentRegionKey)>(node);
            const auto before = flags(region), rootBefore = flags(spawn);
            region.ApplyTimePass(); // also gives the root night for each age present
            updated = updated || flags(region) != before || flags(spawn) != rootBefore;
            updated = region.UpdateEvents() || updated;
            auto follow = [&](const Entrance& entrance, size_t to) {
                if (EdgePropagate(logic, entrance, region, regions[to])) {
                    updated = true;
                    if (!queued[to]) {
                        queued[to] = true;
                        pool.push_back(to);
                    }
                }
            };
            if (node == root) {
                follow(*overlay.rootSpawn, overlay.rootSpawnTarget);
                continue;
            }
            for (const auto& exit : region.exits) {
                if (!std::binary_search(overlay.deleted.begin(), overlay.deleted.end(), &exit, order)) {
                    follow(exit, static_cast<size_t>(exit.GetConnectedRegionKey()));
                }
            }
            for (const auto& edge : overlay.added) {
                if (edge.parent == node) {
                    follow(*edge.entrance, edge.target);
                }
            }
            if (node == overlay.rootSpawnTarget && !overlay.global.empty()) {
                for (const auto& edge : overlay.global) {
                    logic.CurrentRegionKey = static_cast<decltype(logic.CurrentRegionKey)>(edge.parent);
                    follow(*edge.entrance, edge.target);
                }
                logic.CurrentRegionKey = static_cast<decltype(logic.CurrentRegionKey)>(node);
            }
        }
    } while (updated);
    // Containment: the starting age never leaves the virtual root.
    for (size_t r = 0; r < std::size(regions); ++r) {
        if (r != root && (startingChild ? regions[r].Child() : regions[r].Adult())) {
            return kViolation;
        }
    }
    const auto& temple = regions[target];
    ages = (temple.Adult() ? 1 : 0) | (temple.Child() ? 2 : 0);
    return (startingChild ? temple.Adult() : temple.Child()) ? kReached : kNotReached;
}

struct RowPair {
    std::string src, dst;
};
struct QueryRequest {
    uint32_t token = 0;
    std::vector<std::string> deleted, mmDeleted;
    std::vector<RowPair> edges, portalLinks, mmEdges;
};
struct PrepareRequest {
    bool startingChild = false;
    std::vector<std::string> foreign;
};

inline std::vector<std::string> Names(const nlohmann::json& list) {
    std::vector<std::string> names;
    for (const auto& name : list) {
        names.push_back(name.get<std::string>());
    }
    return names;
}
inline std::vector<RowPair> Pairs(const nlohmann::json& list) {
    std::vector<RowPair> pairs;
    for (const auto& pair : list) {
        pairs.push_back({ pair.at("src").get<std::string>(), pair.at("dst").get<std::string>() });
    }
    return pairs;
}
// Parses a request object; any malformed field is Unsupported, never an internal error.
template <class Read>
auto ParseRequest(const char* text, std::string_view schema, std::string_view registry, Read read) {
    try {
        const auto json = nlohmann::json::parse(text ? text : "", nullptr, false);
        if (json.is_discarded() || !json.is_object() || json.value("schema", "") != schema) {
            throw Failure(kUnsupported, "unrecognized request; expected " + std::string(schema));
        }
        if (json.value("registry", "") != registry) {
            throw Failure(kUnsupported, "entrance registry identity differs");
        }
        return read(json);
    } catch (const nlohmann::json::exception& e) {
        throw Failure(kUnsupported, std::string("malformed request: ") + e.what());
    }
}
inline QueryRequest ParseQuery(const char* text, std::string_view registry) {
    return ParseRequest(text, "oot-temple-probe-query-v1", registry, [](const nlohmann::json& json) {
        if (!json.at("assumedInstalled").get<bool>()) {
            throw Failure(kViolation, "ASSUMED must be installed for every candidate (entrances.ts:307)");
        }
        if (!json.at("assumed").get<std::vector<std::string>>().empty()) {
            throw Failure(kUnsupported, "large-pool ASSUMED exits are not supported natively");
        }
        QueryRequest q;
        q.token = json.at("token").get<uint32_t>();
        q.deleted = Names(json.at("deleted"));
        q.edges = Pairs(json.at("edges"));
        q.portalLinks = Pairs(json.at("portalLinks"));
        q.mmDeleted = Names(json.at("mm").at("deleted"));
        q.mmEdges = Pairs(json.at("mm").at("edges"));
        return q;
    });
}
inline PrepareRequest ParsePrepare(const char* text, std::string_view registry) {
    return ParseRequest(text, "oot-temple-probe-prepare-v1", registry, [](const nlohmann::json& json) {
        const auto age = json.at("startingAge").get<std::string>();
        if (age != "child" && age != "adult") {
            throw Failure(kUnsupported, "starting age must be resolved to child or adult");
        }
        return PrepareRequest{ age == "child", Names(json.at("foreignStartingItems")) };
    });
}

// Runs an operation and turns every exception into a return code and a message: nothing crosses
// the C ABI.
template <class F> int Firewall(std::string& error, F&& run) {
    try {
        const int code = run();
        error.clear();
        return code;
    } catch (const Failure& f) {
        error = f.what();
        return f.code;
    } catch (const std::exception& e) {
        error = std::string("internal: ") + e.what();
        return kInternal;
    } catch (...) {
        error = "internal: unknown exception";
        return kInternal;
    }
}

// One query through a policy that owns every engine access (TempleProbe.cpp, or host stubs):
//   Check()                P1-P8, before any write;   Resolve(request)  P9, the overlay
//   MakeLogic()            a fresh private Logic;     ApplyStartingItems()  under W1
//   Regions(), Handle(), Scope(), Root(), Target(), StartingChild(), Registry(), SameLogic()
template <class Policy> int RunQuery(Policy& policy, const char* json, int (*poll)(void), uint8_t* agesOut) {
    if (agesOut == nullptr) {
        throw Failure(kUnsupported, "missing ages output");
    }
    *agesOut = 0;
    DepthGuard depth;
    policy.Check();
    const auto overlay = policy.Resolve(ParseQuery(json, policy.Registry()));
    auto probe = policy.MakeLogic();
    uint8_t ages = 0;
    int result;
    {
        RegionFlagGuard<std::remove_reference_t<decltype(policy.Regions())>> flags(policy.Regions());
        ScopedLogic<std::remove_reference_t<decltype(policy.Handle())>> scoped(policy.Handle(), probe, policy.Scope());
        policy.ApplyStartingItems();
        result = Search(
            policy.Regions(), *probe, overlay, policy.Root(), policy.Target(), policy.StartingChild(),
            [poll] { return poll != nullptr && poll() != 0; }, ages);
    }
    if (!policy.SameLogic()) {
        throw Failure(kViolation, "::logic is not the Context's Logic after the query");
    }
    if (result == kViolation) {
        throw Failure(kViolation, "the starting age left the virtual root");
    }
    if (result == kCancelled) {
        throw Failure(kCancelled, "cancelled");
    }
    *agesOut = ages;
    return result;
}
} // namespace Rando::TempleProbe
