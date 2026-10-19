// combo/mq/MqRoomLog.h: the developer Master Quest room log that
// ComboMqRuntime.cpp writes after each dungeon room load (MQ port unit U8).
// Checks the exact line for an MQ room, a vanilla room, empty lists, a removed
// broken actor and a collision that could not be patched.
// `--emit-sample <file>` writes the lines a correct runtime logs for every MQ
// room in the bins, as child and as adult, and a vanilla line for each dungeon;
// mq_walk_compare_tests.py checks tools/mq_walk_compare.py on it.
#include "mq/ComboMqData.h"
#include "mq/MqRoomLog.h"
#include "mq/MqRuntime.h"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <set>
#include <string>
#include <vector>

using namespace ComboMq;

namespace {

void Expect(bool condition, const std::string &what) {
  if (!condition) {
    std::fprintf(stderr, "FAIL: %s\n", what.c_str());
    assert(false);
  }
}

void ExpectLine(const RoomLog &log, const std::string &want,
                const std::string &what) {
  const std::string got = FormatRoomLog(log);
  Expect(got == want, what + "\n  got:  " + got + "\n  want: " + want);
}

void CheckFormat() {
  Expect(std::string(kRoomLogCVar) == "gCombo.Debug.MqRoomLog",
         "the CVar is a developer one, gCombo.Debug.*");

  RoomLog fire;
  fire.scene = 0x04;
  fire.room = 3;
  fire.dungeon = kFireTemple;
  fire.mq = true;
  fire.layer = 2;
  fire.actors = LogSource::Mq;
  fire.actorList = {{0x0008, 0x0000}, {-1, 0x1234}};
  fire.objects = LogSource::Mq;
  fire.objectList = {0x00a4, 0x0001};
  fire.transitions = LogSource::Mq;
  fire.transitionCount = 31;
  fire.paths = LogSource::Mq;
  fire.pathCount = 4;
  fire.collision = LogCollision::Patched;
  fire.changedPolys = 553;
  fire.changedSurfaces = 33;
  ExpectLine(fire,
             "[ComboShip MQ room] scene=0x04 room=3 dungeon=4 quest=mq "
             "layer=2 actors=mq:2 objects=mq:2 transitions=mq:31 paths=mq:4 "
             "collision=patched:553:33 actorList=0008/0000,ffff/1234 "
             "objectList=00a4,0001",
             "an MQ room with every list from the bins");

  RoomLog vanilla;
  vanilla.scene = 0x0D;
  vanilla.room = 12;
  vanilla.dungeon = kGanonsCastle;
  vanilla.transitionCount = 7;
  vanilla.pathCount = 6; // a vanilla path list has no count to show
  vanilla.actorList = {{0x0095, 0xffff}};
  ExpectLine(vanilla,
             "[ComboShip MQ room] scene=0x0d room=12 dungeon=11 "
             "quest=vanilla layer=0 actors=vanilla:1 objects=vanilla:0 "
             "transitions=vanilla:7 paths=vanilla collision=vanilla "
             "actorList=0095/ffff objectList=-",
             "a vanilla room, an empty object list and no path count");

  RoomLog empty = fire;
  empty.actorList.clear();
  empty.objectList = {-2};
  empty.collision = LogCollision::Failed;
  empty.layer = 4;
  ExpectLine(empty,
             "[ComboShip MQ room] scene=0x04 room=3 dungeon=4 quest=mq "
             "layer=4 actors=mq:0 objects=mq:1 transitions=mq:31 paths=mq:4 "
             "collision=failed actorList=- objectList=fffe",
             "an MQ room without actors and a collision left vanilla");
}

// The changes the patched copy makes, as the runtime counts them: every
// patched polygon and surface type, once each (all of them change the vanilla
// NTSC-U 1.0 collision, mq_runtime_archive_tests.py).
void CollisionCounts(int dungeon, const Scene &scene, size_t &polys,
                     size_t &surfaces) {
  std::set<uint32_t> polyIds, surfaceIds;
  if (dungeon == kGanonsCastle)
    polyIds.insert(std::begin(kGanonPatchedPolys),
                   std::end(kGanonPatchedPolys));
  for (const PolyPatch &patch : scene.polyPatches)
    polyIds.insert(patch.id);
  for (const SurfacePatch &patch : scene.surfacePatches)
    surfaceIds.insert(patch.id);
  polys = polyIds.size();
  surfaces = surfaceIds.size();
}

// What ComboMq_LogRoom reads back after a bundled MQ room load on `layer`.
RoomLog MqRoom(int dungeon, const Room &room, const Scene &scene, int layer) {
  static RoomBuffers<ActorEntryPOD> buffers;
  buffers.Swap();
  const RoomBuffers<ActorEntryPOD>::Buffer &buffer = buffers.Load(room);
  RoomLog log;
  log.scene = dungeon == kGerudoTrainingGround ? 0x0B
              : dungeon == kGanonsCastle       ? 0x0D
                                               : dungeon;
  log.room = room.room;
  log.dungeon = dungeon;
  log.mq = true;
  log.layer = layer;
  log.actors = LogSource::Mq;
  for (size_t i = 0; i < buffer.numActors; i++)
    log.actorList.push_back(
        {buffer.actors[i].id, static_cast<uint16_t>(buffer.actors[i].params)});
  log.objects = LogSource::Mq;
  log.objectList.assign(buffer.objects, buffer.objects + buffer.numObjects);
  log.transitions = LogSource::Mq;
  log.transitionCount = scene.transitions.size();
  if (scene.hasPaths) {
    log.paths = LogSource::Mq;
    log.pathCount = scene.paths.size();
  }
  if (HasCollisionPatches(dungeon, scene)) {
    log.collision = LogCollision::Patched;
    CollisionCounts(dungeon, scene, log.changedPolys, log.changedSurfaces);
  }
  return log;
}

int EmitSample(const Data &data, const char *path) {
  std::ofstream out(path, std::ios::binary);
  size_t lines = 0;
  for (int dungeon = 0; dungeon < kDungeonCount; dungeon++) {
    const Dungeon &entry = data.dungeons[dungeon];
    for (int layer : {0, 2})
      for (const Room &room : entry.rooms) {
        out << FormatRoomLog(MqRoom(dungeon, room, entry.scene, layer)) << "\n";
        lines++;
      }
    // The same dungeon left vanilla: the resources' own lists.
    RoomLog vanilla = MqRoom(dungeon, entry.rooms.at(0), entry.scene, 0);
    vanilla.mq = false;
    vanilla.actors = vanilla.objects = vanilla.transitions = vanilla.paths =
        LogSource::Vanilla;
    vanilla.pathCount = 0;
    vanilla.collision = LogCollision::Vanilla;
    out << FormatRoomLog(vanilla) << "\n";
    lines++;
  }
  Expect(out.good(), std::string("cannot write ") + path);
  std::printf("wrote %zu lines for %zu MQ rooms to %s\n", lines,
              data.RoomCount(), path);
  return 0;
}

} // namespace

int main(int argc, char **argv) {
  const Data *data = Bundled();
  if (data == nullptr) {
    std::fprintf(stderr, "bundled MQ data failed to decode: %s\n",
                 BundledError().c_str());
    return 1;
  }
  if (argc == 3 && std::string(argv[1]) == "--emit-sample")
    return EmitSample(*data, argv[2]);
  CheckFormat();
  std::puts("PASS: the MQ room log line shows the scene, room, dungeon, "
            "quest, layer, each list's source and count, the collision patch "
            "status and the actor and object lists");
  return 0;
}
