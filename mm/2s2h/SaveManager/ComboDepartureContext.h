// ComboShip: the cycle state an MM->OoT departure keeps for MM's next start, as OoTMM's game switch keeps it.
//
// OoTMM v32.3 switches games with an owl save (common/switch.c comboGameSwitch: Save_DoSave(play, SF_OWL)), and its MM
// save is MM's SaveContext up to fileNum (include/combo/mm/save.h MmSave, 0x3CA0 bytes; mm/save.S reads and writes
// every MM save at that size): the Save, then eventInf, the bytes up to the bottle timers, the bottle timers and the
// pictograph photo. Loading an owl save keeps eventInf, which a new-cycle load clears (Sram_OpenSave). Combo's own
// departure writes only the Save (SaveManager_SaveCurrentForCombo) and MM's next start clears the rest (title_setup.c
// SaveContext_Init), so a departure keeps the rest beside the Save under kKey, with the moment of the cycle it was kept
// at, and the next load of that Save restores it.
//
// Additive: a save without the key loads as before, and older builds ignore the key. A load uses it only while the Save
// is still at that moment, so one an older build carried past later play is never used. A write made while MM is in
// front drops it, as its live state is newer; the writes made while OoT is in front keep it.
#pragma once
#include <cstddef>
#include <cstring>
#include <memory>
#include <nlohmann/json.hpp>
#include "BenJsonConversions.hpp"

namespace ComboDepartureContext {
inline constexpr const char* kKey = "comboDepartureContext";

inline nlohmann::json Moment(const Save& save) {
    return { { "day", save.day }, { "time", save.time }, { "cycle", save.saveInfo.playerData.threeDayResetCount } };
}

// The departure: keep everything of live's SaveContext the owl writer keeps (to_json(SaveContext)) except its Save.
inline void Keep(nlohmann::json& file, const SaveContext& live) {
    nlohmann::json state = live;
    state.erase("save");
    file[kKey] = { { "moment", Moment(live.save) }, { "state", std::move(state) } };
}

inline void Drop(nlohmann::json& file) {
    file.erase(kKey);
}

// live's Save was just loaded from file: restore the state kept beside it, when it was kept at that Save's moment.
inline bool Restore(const nlohmann::json& file, SaveContext& live) {
    if (!file.is_object() || !file.contains(kKey))
        return false;
    try {
        const auto& kept = file.at(kKey);
        if (kept.at("moment") != Moment(live.save))
            return false;
        auto whole = kept.at("state");
        whole["save"] = live.save;
        auto restored = std::make_unique<SaveContext>();
        whole.get_to(*restored);
        constexpr size_t first = offsetof(SaveContext, eventInf), end = offsetof(SaveContext, fileNum);
        std::memcpy(reinterpret_cast<char*>(&live) + first, reinterpret_cast<const char*>(restored.get()) + first,
                    end - first);
        return true;
    } catch (const nlohmann::json::exception&) { return false; }
}
} // namespace ComboDepartureContext
