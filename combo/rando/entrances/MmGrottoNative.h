// Native MM RespawnData adapter for pinned OoTMM typed grotto contexts.
// Isolated continuation; no public/runtime activation. See LICENSE.OoTMM.
#pragma once
#include "rando/entrances/MmGrottos.h"
namespace ComboRando::Entrances {
template <class Native> MmRespawn ReadNativeMmRespawn(const Native& n) {
    return { { n.pos.x, n.pos.y, n.pos.z },
             n.yaw,
             n.playerParams,
             n.entrance,
             n.roomIndex,
             n.data,
             n.tempSwitchFlags,
             n.unk_18,
             n.tempCollectFlags };
}
template <class Native> void WriteNativeMmRespawn(Native& n, const MmRespawn& r) {
    n.pos.x = r.pos.x;
    n.pos.y = r.pos.y;
    n.pos.z = r.pos.z;
    n.yaw = r.yaw;
    n.playerParams = r.playerParams;
    n.entrance = r.entrance;
    n.roomIndex = r.roomIndex;
    n.data = r.data;
    n.tempSwitchFlags = r.tempSwitchFlags;
    n.unk_18 = r.unk18;
    n.tempCollectFlags = r.tempCollectFlags;
}
template <class Native>
MmGrottoContext ReadNativeMmGrotto(const Native* slots, MmPreviousScene previous, int32_t flag, bool cleared) {
    MmGrottoContext c;
    for (size_t i = 0; i < 4; ++i)
        c.respawn[i] = ReadNativeMmRespawn(slots[i]);
    c.previousScene = previous;
    c.respawnFlag = flag;
    c.snowheadCleared = cleared;
    return c;
}
// Resolve full uint32 logical identity before any native u16 conversion.
// Invalid/foreign extended identities throw before modifying native slots.
template <class Native>
uint16_t ApplyNativeMmGrotto(Key arrival, Native* slots, MmPreviousScene& previous, int32_t& flag, bool cleared) {
    auto c = ReadNativeMmGrotto(slots, previous, flag, cleared);
    const auto native = ApplyMmCustomEntrance(arrival, c);
    if (native.game != Game::Mm || native.id > 0xffffu)
        throw std::invalid_argument("Unrepresentable MM native grotto destination");
    for (size_t i = 0; i < 4; ++i)
        WriteNativeMmRespawn(slots[i], c.respawn[i]);
    previous = c.previousScene;
    flag = c.respawnFlag;
    return static_cast<uint16_t>(native.id);
}
} // namespace ComboRando::Entrances
