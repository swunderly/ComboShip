#pragma once
// A developer log of what OoTMM's Master Quest hooks (MqRuntime.h) left in the PlayState after each room load of a
// dungeon scene, for the emulator and phone walk in ComboAndroid/DEVICE_TESTS.md; tools/mq_walk_compare.py checks a
// captured log against the bins. The glue (soh/soh/Enhancements/randomizer/ComboMqRuntime.cpp ComboMq_LogRoom) reads
// the lists back once the room's commands have run, and logs only while the developer CVar kRoomLogCVar is set. It
// is off by default and writes nothing the game reads. Pure C++17 without game headers, so the host tests format the
// same lines (ComboAndroid/tests/mq_room_log_tests.cpp).
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace ComboMq {

inline constexpr char kRoomLogCVar[] = "gCombo.Debug.MqRoomLog";
// Every line starts with this tag, after whatever prefix the log sink adds.
inline constexpr char kRoomLogTag[] = "[ComboShip MQ room]";

// Where a list the PlayState holds after the room load comes from.
enum class LogSource : uint8_t {
    Vanilla, // the scene or room resource's own list
    Mq,      // the MQ list from the bins, in MqRuntime.h's buffers
};
// The collision BgCheck holds after the scene load.
enum class LogCollision : uint8_t {
    Vanilla, // the resource's own: the scene is not MQ, or its dungeon has no patches
    Patched, // the patched copy (mq.c:341-344)
    Failed,  // the resource's own, because the copy could not be patched (logged as an error at the scene load)
};

struct RoomLogActor {
    int16_t id;
    uint16_t params;
};

struct RoomLog {
    int scene = 0;    // play->sceneNum
    int room = 0;     // play->roomCtx.curRoom.num
    int dungeon = -1; // OoTMM's dungeon index (DungeonFromScene)
    bool mq = false;  // the scene plays as MQ from the bundled data (a real oot-mq.o2r's MQ is its own resource's)
    int layer = 0;    // gSaveContext.sceneLayer; 4 and up is a cutscene layer
    LogSource actors = LogSource::Vanilla;
    std::vector<RoomLogActor> actorList; // play->setupActorList, numSetupActors entries
    LogSource objects = LogSource::Vanilla;
    std::vector<int16_t> objectList; // the room's part of the object bank
    LogSource transitions = LogSource::Vanilla;
    size_t transitionCount = 0; // play->transiActorCtx.numActors
    LogSource paths = LogSource::Vanilla;
    size_t pathCount = 0; // MQ paths only: the PlayState keeps no count for a vanilla list
    LogCollision collision = LogCollision::Vanilla;
    size_t changedPolys = 0;    // polygons of the patched copy that differ from the resource's
    size_t changedSurfaces = 0; // surface types of the patched copy that differ from the resource's
};

inline const char* LogSourceName(LogSource source) {
    return source == LogSource::Mq ? "mq" : "vanilla";
}

inline std::string LogHex(unsigned value, int digits) {
    char text[16];
    std::snprintf(text, sizeof(text), "%0*x", digits, value);
    return text;
}

// One line:
//   [ComboShip MQ room] scene=0x04 room=3 dungeon=4 quest=mq layer=0 actors=mq:2 objects=mq:2 transitions=mq:31
//   paths=mq:4 collision=patched:553:33 actorList=0008/0000,ffff/0012 objectList=00a4,0001
// actorList is id/params in hex (ffff is a removed broken actor), objectList the object ids; "-" is an empty list.
// A vanilla path list has no count (paths=vanilla), and collision is patched:<polygons>:<surface types>, vanilla or
// failed.
inline std::string FormatRoomLog(const RoomLog& log) {
    std::string line = kRoomLogTag;
    line += " scene=0x" + LogHex(static_cast<unsigned>(log.scene) & 0xffff, 2);
    line += " room=" + std::to_string(log.room);
    line += " dungeon=" + std::to_string(log.dungeon);
    line += std::string(" quest=") + (log.mq ? "mq" : "vanilla");
    line += " layer=" + std::to_string(log.layer);
    line += std::string(" actors=") + LogSourceName(log.actors) + ":" + std::to_string(log.actorList.size());
    line += std::string(" objects=") + LogSourceName(log.objects) + ":" + std::to_string(log.objectList.size());
    line += std::string(" transitions=") + LogSourceName(log.transitions) + ":" + std::to_string(log.transitionCount);
    line += std::string(" paths=") + LogSourceName(log.paths);
    if (log.paths == LogSource::Mq)
        line += ":" + std::to_string(log.pathCount);
    line += " collision=";
    if (log.collision == LogCollision::Patched)
        line += "patched:" + std::to_string(log.changedPolys) + ":" + std::to_string(log.changedSurfaces);
    else
        line += log.collision == LogCollision::Failed ? "failed" : "vanilla";
    line += " actorList=";
    for (size_t i = 0; i < log.actorList.size(); i++)
        line += (i ? "," : "") + LogHex(static_cast<uint16_t>(log.actorList[i].id), 4) + "/" +
                LogHex(log.actorList[i].params, 4);
    if (log.actorList.empty())
        line += "-";
    line += " objectList=";
    for (size_t i = 0; i < log.objectList.size(); i++)
        line += (i ? "," : "") + LogHex(static_cast<uint16_t>(log.objectList[i]), 4);
    if (log.objectList.empty())
        line += "-";
    return line;
}

} // namespace ComboMq
