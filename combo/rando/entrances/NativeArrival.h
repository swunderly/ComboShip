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
// How the end of a transition met the queued departure (NativeEntranceFlight::finish). None is the ordinary local
// transition, with nothing queued; Departed left for the other game. Every other value refused a pending departure,
// and the native transition goes on: the engine logs it (owner decision D7).
enum class DepartureEnd { None, OtherPlay, OtherFile, Preflight, Save, Commit, Departed };
inline bool DepartureRefused(DepartureEnd end) {
    return end != DepartureEnd::None && end != DepartureEnd::Departed;
}
inline const char* DepartureEndText(DepartureEnd end) {
    switch (end) {
        case DepartureEnd::None:
            return "no departure was queued";
        case DepartureEnd::OtherPlay:
            return "it was queued by another play state";
        case DepartureEnd::OtherFile:
            return "it was queued for another file or game";
        case DepartureEnd::Preflight:
            return "the launcher's preflight refused it";
        case DepartureEnd::Save:
            return "the save before leaving refused it";
        case DepartureEnd::Commit:
            return "the launcher's commit refused it, and the save was rolled back";
        case DepartureEnd::Departed:
            return "it departed";
    }
    return "unknown";
}
class NativeEntranceFlight {
    std::optional<NativeEntranceRequest> departure, arrival;
    // The play state that queued the departure: only the end of its own transition may finish it, and it never
    // outlives that play state (the engines cancel it on a reset, a quit and Play_Destroy).
    const void* departurePlay = nullptr;
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
        cancelDeparture();
        arrival.reset();
        finished = false;
    }
    // Forgets a queued departure only. An arrival armed for this game, and a finished departure the engine has not
    // consumed yet, stay.
    void cancelDeparture() {
        departure.reset();
        departurePlay = nullptr;
    }
    bool departurePending() const {
        return departure.has_value();
    }
    bool queue(const NativeEntranceRequest& r, const void* play) {
        if (departure || !play || !ValidNativeEntranceRequest(r) || !preflight || !commit ||
            !preflight(r.file, r.source, r.destination, r.id, r.age, r.crossAge))
            return false;
        departure = r;
        departurePlay = play;
        return true;
    }
    bool arm(const NativeEntranceRequest& r, int game) {
        if (arrival || !ValidNativeEntranceRequest(r) || r.destination != game)
            return false;
        arrival = r;
        return true;
    }
    template <class Save, class Rollback>
    DepartureEnd finish(int file, int source, const void* play, Save save, Rollback rollback) {
        if (!departure)
            return DepartureEnd::None;
        const auto r = *departure;
        const void* owner = departurePlay;
        cancelDeparture();
        if (!play || owner != play)
            return DepartureEnd::OtherPlay;
        if (r.file != file || r.source != source)
            return DepartureEnd::OtherFile;
        if (!preflight || !commit || !preflight(r.file, r.source, r.destination, r.id, r.age, r.crossAge))
            return DepartureEnd::Preflight;
        // Native full-save services run before foreground mutation. A rejected
        // callback leaves the original local transition available to the caller.
        if (!save(r))
            return DepartureEnd::Save;
        if (!commit(r.file, r.source, r.destination, r.id, r.age, r.crossAge)) {
            rollback(r);
            return DepartureEnd::Commit;
        }
        finished = true;
        return DepartureEnd::Departed;
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
