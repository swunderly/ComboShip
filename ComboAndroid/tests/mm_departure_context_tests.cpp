// MM -> OoT -> MM keeps what OoTMM's game switch keeps (mm/2s2h/SaveManager/ComboDepartureContext.h): MM's eventInf,
// the bytes up to the bottle timers, the bottle timers and the pictograph photo, which OoTMM's departure owl save
// writes with the Save and its next MM start loads back. Runs the production Keep/Restore/Drop over a departure's file
// as SaveManager_SaveCurrentForCombo writes it, read back from its JSON text into a cleared context as MM's next start
// loads it (title_setup.c: SaveContext_Init, then SaveManager_LoadSaveFile's new-cycle page).
#include <cassert>
#include <cstring>
#include <iostream>
#include <memory>
#include "SaveManager/ComboDepartureContext.h"

namespace DC = ComboDepartureContext;
constexpr size_t kFirst = offsetof(SaveContext, eventInf), kEnd = offsetof(SaveContext, fileNum);

static bool SameState(const SaveContext& a, const SaveContext& b) {
    return std::memcmp(reinterpret_cast<const char*>(&a) + kFirst, reinterpret_cast<const char*>(&b) + kFirst,
                       kEnd - kFirst) == 0;
}
static bool ClearedState(const SaveContext& a) {
    const auto* bytes = reinterpret_cast<const unsigned char*>(&a);
    for (size_t i = kFirst; i < kEnd; ++i)
        if (bytes[i])
            return false;
    return true;
}
// MM's next start: a cleared context with the file's new-cycle Save, then the kept state.
static std::unique_ptr<SaveContext> Start(const json& file, bool& restored) {
    auto next = std::make_unique<SaveContext>();
    std::memset(next.get(), 0, sizeof(SaveContext));
    file.at("newCycleSave").at("save").get_to(next->save);
    const json loaded = next->save;
    restored = DC::Restore(file, *next);
    assert(json(next->save) == loaded); // the Save is the file's, untouched
    return next;
}

int main() {
    // OoTMM's MM save (MmSave) is the SaveContext up to fileNum; what Combo's Save leaves out starts right after it.
    static_assert(offsetof(SaveContext, eventInf) == sizeof(Save), "the kept state starts right after the Save");
    // The departing context: a Save at some moment of the cycle and the state past it.
    auto live = std::make_unique<SaveContext>();
    std::memset(live.get(), 0, sizeof(SaveContext));
    live->save.day = 2;
    live->save.time = 0x8123;
    live->save.saveInfo.playerData.threeDayResetCount = 4;
    live->save.saveInfo.playerData.rupees = 99;
    for (size_t i = 0; i < 8; ++i)
        live->eventInf[i] = static_cast<u8>(0x11 * (i + 1));
    live->unk_1014 = 1;
    live->bButtonStatus = 0xFF;
    live->jinxTimer = 300;
    live->rupeeAccumulator = -20;
    live->bottleTimerStates[2] = 1;
    live->bottleTimerStartOsTimes[2] = 123456789ull;
    live->bottleTimerTimeLimits[2] = 6000;
    live->bottleTimerCurTimes[2] = 4321;
    live->bottleTimerPausedOsTimes[2] = 777;
    for (size_t i = 0; i < sizeof(live->pictoPhotoI5); ++i)
        live->pictoPhotoI5[i] = static_cast<u8>(i * 7);

    // The departure's write: the Save, and the state kept beside it; then the file as its JSON text holds it.
    json file;
    file["newCycleSave"]["save"] = live->save;
    DC::Keep(file, *live);
    file["version"] = 7;
    file["type"] = "2S2H_SAVE";
    assert(!file.at(DC::kKey).at("state").contains("save"));
    const json disk = json::parse(file.dump());

    // The next start gives back exactly what was kept.
    bool restored = false;
    auto next = Start(disk, restored);
    assert(restored && SameState(*next, *live));
    // The writes made while OoT is in front change only the Save (an item, an edit) and keep the key: still restored.
    auto granted = disk;
    granted["newCycleSave"]["save"]["saveInfo"]["playerData"]["rupees"] = 150;
    next = Start(granted, restored);
    assert(restored && SameState(*next, *live) && next->save.saveInfo.playerData.rupees == 150);

    // A save without the key (every save before this change) loads as before: nothing past the Save is set.
    auto old = disk;
    old.erase(DC::kKey);
    next = Start(old, restored);
    assert(!restored && ClearedState(*next));
    // MM's own later write drops the key, as its live state is newer.
    auto dropped = disk;
    DC::Drop(dropped);
    assert(!dropped.contains(DC::kKey));
    next = Start(dropped, restored);
    assert(!restored && ClearedState(*next));
    // A key an older build carried past later play (its writes keep unknown keys): the Save has moved on in the cycle,
    // or into another cycle, so it is never used.
    for (const char* field : { "time", "day" }) {
        auto moved = disk;
        moved["newCycleSave"]["save"][field] = moved["newCycleSave"]["save"][field].get<int>() + 1;
        next = Start(moved, restored);
        assert(!restored && ClearedState(*next));
    }
    auto anotherCycle = disk;
    anotherCycle["newCycleSave"]["save"]["saveInfo"]["playerData"]["threeDayResetCount"] = 5;
    next = Start(anotherCycle, restored);
    assert(!restored && ClearedState(*next));
    // A damaged key is ignored, never half-applied.
    for (int damage = 0; damage < 3; ++damage) {
        auto broken = disk;
        auto& state = broken[DC::kKey]["state"];
        if (damage == 0)
            state.erase("pictoPhotoI5");
        if (damage == 1)
            state["eventInf"] = "broken";
        if (damage == 2)
            broken[DC::kKey].erase("moment");
        next = Start(broken, restored);
        assert(!restored && ClearedState(*next));
    }
    std::cout << "PASS: MM -> OoT -> MM keeps eventInf, the bottle timers and the pictograph as OoTMM's switch does; "
                 "saves without the kept state, or with a stale or damaged one, load as before\n";
    return 0;
}
