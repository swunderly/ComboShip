// MM -> OoT -> MM keeps what OoTMM's game switch keeps (mm/2s2h/SaveManager/ComboDepartureContext.h): MM's eventInf,
// the bytes up to the bottle timers, the bottle timers and the pictograph photo, which OoTMM's departure owl save
// writes with the Save and its next MM start loads back. Runs the production Keep/Restore/Drop over a departure's file
// as SaveManager_SaveCurrentForCombo writes it, read back from its JSON text into a cleared context as MM's next start
// loads it (title_setup.c: SaveContext_Init, then SaveManager_LoadSaveFile's new-cycle page).
//
// Rupees still counting count once: the departure settles them into its Save (SettleRupees, which
// SaveManager_SaveCurrentForCombo runs first for a departure) and keeps none pending, and an MM item given while OoT is
// in front lands in the accumulator and is settled into the Save the same way before it is written
// (BenPort.cpp Combo_MM_GiveDormantResolved, through SaveManager_SettleRupeesForCombo).
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

constexpr s16 kWallet = 200; // the Adult's Wallet
// The departure's write, in SaveManager_SaveCurrentForCombo's order: the rupees settled, the Save, the kept state.
static json Depart(SaveContext& live) {
    DC::SettleRupees(live, kWallet);
    json file;
    file["newCycleSave"]["save"] = live.save;
    DC::Keep(file, live);
    file["version"] = 7;
    file["type"] = "2S2H_SAVE";
    return file;
}
// While OoT is in front: an MM item's rupees land in the accumulator (Rando::GiveItem under gComboDormantGive), and
// Combo_MM_GiveDormantResolved settles them and writes the Save; that write keeps the departure's key.
static void GiveRupees(SaveContext& dormant, json& file, s16 rupees) {
    dormant.rupeeAccumulator = static_cast<s16>(dormant.rupeeAccumulator + rupees);
    DC::SettleRupees(dormant, kWallet);
    file["newCycleSave"]["save"] = dormant.save;
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
    live->rupeeAccumulator = -20; // a 20-rupee purchase still counting down
    live->bottleTimerStates[2] = 1;
    live->bottleTimerStartOsTimes[2] = 123456789ull;
    live->bottleTimerTimeLimits[2] = 6000;
    live->bottleTimerCurTimes[2] = 4321;
    live->bottleTimerPausedOsTimes[2] = 777;
    for (size_t i = 0; i < sizeof(live->pictoPhotoI5); ++i)
        live->pictoPhotoI5[i] = static_cast<u8>(i * 7);

    // The departure's write: the rupees settled, the Save, and the state kept beside it; then the file as its JSON text
    // holds it. The purchase is paid in the Save, and nothing is left counting.
    const json file = Depart(*live);
    assert(!file.at(DC::kKey).at("state").contains("save"));
    assert(live->save.saveInfo.playerData.rupees == 79 && live->rupeeAccumulator == 0);
    assert(file.at(DC::kKey).at("state").at("rupeeAccumulator") == 0);
    const json disk = json::parse(file.dump());

    // The next start gives back exactly what was kept, and the purchase is paid once.
    bool restored = false;
    auto next = Start(disk, restored);
    assert(restored && SameState(*next, *live));
    assert(next->save.saveInfo.playerData.rupees == 79 && next->rupeeAccumulator == 0);
    // The writes made while OoT is in front change only the Save (an item, an edit) and keep the key: still restored.
    // A 50-rupee item given then is in the Save once, and the purchase is not paid again.
    auto granted = disk;
    auto dormant = std::make_unique<SaveContext>(*live); // MM's memory while OoT is in front
    GiveRupees(*dormant, granted, 50);
    next = Start(json::parse(granted.dump()), restored);
    assert(restored && SameState(*next, *live) && next->save.saveInfo.playerData.rupees == 129);
    assert(next->rupeeAccumulator == 0);
    // Rupees still counting up when Link leaves, and items while OoT is in front: each counted once, within the
    // wallet, as the interface's count keeps them.
    {
        auto counting = std::make_unique<SaveContext>(*live);
        counting->save.saveInfo.playerData.rupees = 100;
        counting->rupeeAccumulator = 30; // a red and a purple rupee picked up just before leaving
        auto left = Depart(*counting);
        assert(counting->save.saveInfo.playerData.rupees == 130 && counting->rupeeAccumulator == 0);
        GiveRupees(*counting, left, 20);
        next = Start(json::parse(left.dump()), restored);
        assert(restored && next->save.saveInfo.playerData.rupees == 150 && next->rupeeAccumulator == 0);
        GiveRupees(*counting, left, 100); // past the wallet
        next = Start(json::parse(left.dump()), restored);
        assert(restored && next->save.saveInfo.playerData.rupees == kWallet && next->rupeeAccumulator == 0);
        auto broke = std::make_unique<SaveContext>(*live);
        broke->save.saveInfo.playerData.rupees = 10; // a 20-rupee purchase with 10 rupees left
        broke->rupeeAccumulator = -20;
        Depart(*broke);
        assert(broke->save.saveInfo.playerData.rupees == 0 && broke->rupeeAccumulator == 0);
    }

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
    std::cout << "PASS: MM -> OoT -> MM keeps eventInf, the bottle timers and the pictograph as OoTMM's switch does, "
                 "and counts rupees still counting once; saves without the kept state, or with a stale or damaged "
                 "one, load as before\n";
    return 0;
}
