// In-memory handoff only; no public save ABI or engine respawn reinterpretation.
#pragma once
#include "rando/CrossEntrance.h"
#include "rando/entrances/Context.h"
#include <optional>
namespace ComboRando::Entrances {
struct NativeEntranceRequest {
    int file = -1, source = -1, destination = -1, age = -1, crossAge = 0;
    uint32_t id = 0;
};
inline bool ValidNativeEntranceRequest(const NativeEntranceRequest& r) {
    return r.file >= 0 && r.file <= 2 && r.source >= 0 && r.source <= 1 && r.destination == 1 - r.source &&
           r.age >= 0 && r.age <= 1 && (r.crossAge == 0 || r.crossAge == 1) && r.id < 0x80000000u;
}
class NativeEntranceFlight {
    std::optional<NativeEntranceRequest> departure, arrival;
    ComboEntrancePreflight preflight = nullptr;
    ComboEntranceCommit commit = nullptr;
    bool finished = false;

  public:
    void bridge(ComboEntrancePreflight p, ComboEntranceCommit c) {
        cancel();
        preflight = p;
        commit = c;
    }
    void cancel() {
        departure.reset();
        arrival.reset();
        finished = false;
    }
    bool queue(const NativeEntranceRequest& r) {
        if (departure || !ValidNativeEntranceRequest(r) || !preflight || !commit ||
            !preflight(r.file, r.source, r.destination, r.id, r.age, r.crossAge))
            return false;
        departure = r;
        return true;
    }
    bool arm(const NativeEntranceRequest& r, int game) {
        if (arrival || !ValidNativeEntranceRequest(r) || r.destination != game)
            return false;
        arrival = r;
        return true;
    }
    template <class Save, class Rollback> bool finish(int file, int source, Save save, Rollback rollback) {
        if (!departure)
            return false;
        const auto r = *departure;
        departure.reset();
        if (r.file != file || r.source != source || !preflight || !commit ||
            !preflight(r.file, r.source, r.destination, r.id, r.age, r.crossAge))
            return false;
        // Native full-save services run before foreground mutation. A rejected
        // callback leaves the original local transition available to the caller.
        if (!save(r))
            return false;
        if (!commit(r.file, r.source, r.destination, r.id, r.age, r.crossAge)) {
            rollback(r);
            return false;
        }
        finished = true;
        return true;
    }
    bool takeFinished() {
        bool value = finished;
        finished = false;
        return value;
    }
    std::optional<NativeEntranceRequest> takeArrival(int file, int game) {
        const auto r = arrival;
        arrival.reset();
        if (!r || r->file != file || r->destination != game)
            return std::nullopt;
        return r;
    }
};
// Exact pinned mm/play/play.c fixupOriginalSceneSetup. Cutscene staging belongs
// to the destination engine and is applied before its Play_Init.
inline uint32_t FixMmNativeArrival(uint32_t entrance, bool wf, bool sh, bool gb, bool st, int& nextCutscene) {
    uint32_t key = entrance >> 9;
    if (wf) {
        if (key == 0x42)
            key = 0x06;
        else if (key == 0x43)
            nextCutscene = 0xfff0;
    }
    if (sh) {
        switch (key) {
            case 0x4d:
                key = 0x57;
                break;
            case 0x5a:
                key = 0x5b;
                break;
            case 0x4a:
                key = 0x45;
                break;
            case 0x59:
            case 0x19:
            case 0x58:
                nextCutscene = 0xfff0;
                break;
        }
    }
    if (gb && (key == 0x34 || key == 0x35))
        nextCutscene = 0xfff0;
    if (st && key == 0x10)
        nextCutscene = 0xfff0;
    return (key << 9) | (entrance & 0x1ff);
}
} // namespace ComboRando::Entrances
