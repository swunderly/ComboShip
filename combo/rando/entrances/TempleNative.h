// Production wiring of the generation-only opposite-age Temple probe (soh TempleProbe.cpp; OoTMM
// validateAgeTemple, packages/logic/src/solver/entrances.ts:265-286 at 413573d): the soh exports
// found through the launcher's own module lookup, one prepared session per Shuffler run with a
// cancellation poll, and the validator Bindings::validateAgeTemple gets. Only UnifiedBridge.h may
// include it.
#pragma once
#include "TempleAdapter.h"
#include <functional>
#include <memory>
#include <optional>

namespace ComboRando::Entrances {
// Non-zero once this generation is cancelled. The native query polls it once per pass, on the
// generation thread inside the soh call, and then returns -3; it must never throw across the C ABI.
inline int TempleProbeCancelPoll() noexcept {
    return generationProgress && generationProgress->CancelRequested() ? 1 : 0;
}

// The soh exports (soh/soh/Enhancements/randomizer/TempleProbe.h), looked up through the caller's
// module seam: ComboShip.cpp's GetSym(sohModule, name) or the headless Sym<void*>(soh, name), as for
// every other soh export. A missing entry point stays null, and the run refuses it before any draw.
template <class Lookup> TempleProbeFns ResolveTempleProbe(Lookup&& lookup) {
    TempleProbeFns fns;
    fns.prepare = reinterpret_cast<decltype(fns.prepare)>(lookup("SOH_TempleProbe_Prepare"));
    fns.query = reinterpret_cast<decltype(fns.query)>(lookup("SOH_TempleProbe_Query"));
    fns.lastError = reinterpret_cast<decltype(fns.lastError)>(lookup("SOH_TempleProbe_LastError"));
    fns.release = reinterpret_cast<decltype(fns.release)>(lookup("SOH_TempleProbe_Release"));
    fns.poll = TempleProbeCancelPoll;
    return fns;
}

// The Temple pass of one Shuffler run: build it before the Shuffler and keep it until the run ends.
// Every refusal comes before any random draw: the gate, the entry points, then Prepare. Release runs
// on every exit path, including an exception or a cancellation out of the Shuffler, and a query
// outside the prepared session is refused. A run that needs no Temple pass (NeedsTemplePass: logic
// none, games=Mm, or no pool that resolves to a source) gates, prepares, queries and releases nothing,
// so building one for a generation with no entrance pool is safe. facts.logicEnabled and areaExists
// must be the same run's Bindings::logicEnabled and Bindings::areaExists.
// The return codes map as MapTempleResult says, and TempleProbeUnsupported is never retried.
class TempleProbeRun {
  public:
    TempleProbeRun(TempleProbeFns fns, const Settings& s, const TempleFacts& facts, const AreaExists& areaExists) {
        if (!NeedsTemplePass(s, facts, areaExists)) {
            // Settings that name a pool still make the Shuffler require the hook (Shuffle.h:474-481)
            // even when every pool resolves empty and place() never calls it, so give it one that
            // refuses any call without reaching the native probe.
            if (facts.logicEnabled && s.games != Games::Mm && !MakePools(s).empty())
                validator = [](const Overrides&, const Pools&) -> bool {
                    throw TempleProbeUnsupported("Temple probe: query for a run with no pool to place");
                };
            return;
        }
        CheckTempleCapability(s, facts, areaExists);
        if (!fns.poll)
            fns.poll = TempleProbeCancelPoll;
        session.emplace(fns, facts);
        validator = [query = MakeTempleValidator(fns, s), alive = live](const Overrides& overrides,
                                                                        const Pools& pools) {
            if (!*alive)
                throw TempleProbeUnsupported("Temple probe: query outside a prepared session");
            CheckGenerationCancellation();
            return query(overrides, pools);
        };
        *live = true;
    }
    ~TempleProbeRun() {
        *live = false; // the session member then releases the native probe
    }
    TempleProbeRun(const TempleProbeRun&) = delete;
    TempleProbeRun& operator=(const TempleProbeRun&) = delete;

    // What Bindings::validateAgeTemple gets. Empty when the Shuffler neither requires nor calls it
    // (logic none, games=Mm, no pool in the settings); a refusing one when every pool resolves empty.
    std::function<bool(const Overrides&, const Pools&)> Validator() const {
        return validator;
    }

  private:
    std::shared_ptr<bool> live = std::make_shared<bool>(false);
    std::optional<TempleProbeSession> session;
    std::function<bool(const Overrides&, const Pools&)> validator;
};
} // namespace ComboRando::Entrances
