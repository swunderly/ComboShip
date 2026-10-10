#pragma once

// Adapted from OoTMM oot/play/play.c, MIT revision
// 41a2b7c0d7dbbb53fed928652f8271bc336c113f. See OoTMM-MIT-LICENSE.txt.
// Isolated field-restoration adapter only. The engine bridge must perform its
// age change first, then supply that age's typed point and respawn backups.
// Respawn arguments share one engine type: no cross-engine byte copying.
namespace ComboFarore {

template <class Point, class Respawn, class Flag, class SpawnMarker>
void RestoreNativeOotArrival(const Point& point, const Respawn& dungeonBackup, Respawn& top, Respawn& dungeon,
                             Flag& respawnFlag, SpawnMarker& isFwSpawn) {
    if (!isFwSpawn)
        return;
    respawnFlag = 3;
    isFwSpawn = 0;
    dungeon = dungeonBackup;
    // This is the pinned arrival truth test, not the > 0 destination test.
    if (point.set) {
        top.data = 0x28;
        top.pos.x = static_cast<decltype(top.pos.x)>(point.pos.x);
        top.pos.y = static_cast<decltype(top.pos.y)>(point.pos.y);
        top.pos.z = static_cast<decltype(top.pos.z)>(point.pos.z);
        top.yaw = static_cast<decltype(top.yaw)>(point.yaw);
        top.playerParams = static_cast<decltype(top.playerParams)>(point.playerParams);
        top.entranceIndex = static_cast<decltype(top.entranceIndex)>(point.entranceIndex);
        top.roomIndex = static_cast<decltype(top.roomIndex)>(point.roomIndex);
        top.tempSwchFlags = static_cast<decltype(top.tempSwchFlags)>(point.tempSwchFlags);
        top.tempCollectFlags = static_cast<decltype(top.tempCollectFlags)>(point.tempCollectFlags);
    } else {
        top.data = 0;
        top.pos.x = 0.0f;
        top.pos.y = 0.0f;
        top.pos.z = 0.0f;
    }
}
} // namespace ComboFarore
