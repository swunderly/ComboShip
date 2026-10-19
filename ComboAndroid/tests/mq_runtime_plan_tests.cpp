// combo/mq/MqRuntime.h: OoTMM v32.3's src/oot/mq.c scene and room hooks as
// SoH's command runner uses them (soh/soh/Enhancements/randomizer/
// ComboMqRuntime.cpp). Checks, against the bins compiled into the library
// and read here independently of the decoder:
// - which commands of a scene or room header each dungeon replaces
//   (mq.c:314-347, 375-396), only up to the end marker and only for the
//   planned resource;
// - the room buffers (mq.c:79-91, 126-131, 361-411): every MQ room's lists
//   equal the bins, the six broken actors read id -1 in place, and loading
//   A, B, A or a non-MQ room keeps the previous room's lists intact;
// - the scene buffer (mq.c:83-88, 300-359): transition actors equal the bins
//   and start positive on every load; paths equal the bins, sized from
//   make_mq.ts PATHS, never read past their entry;
// - the collision patcher (mq.c:133-218) on synthetic lists: exact patches in
//   mq.c's order, inputs untouched, out-of-range ids refused.
// `--archive-fixture <file>` runs the same plans and patches on the vanilla
// command lists and collision of the user's own oot.o2r, as written by
// mq_runtime_archive_tests.py (local only). `--command-fixture <file>` runs
// the plans alone on the vanilla command ids committed in
// fixtures/mq-vanilla-command-ids.txt.
#include "ComboMqBlobs.h"
#include "mq/ComboMqData.h"
#include "mq/MqRuntime.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace ComboMq;

namespace {

// Layout-equal stand-ins for SoH's structs: z64math.h Vec3s, z64.h ActorEntry
// and TransitionActorEntry, z64scene.h Path, and
// soh/resource/type/CollisionHeader.h CollisionPoly and SurfaceType.
struct Vec3s {
  int16_t x, y, z;
};
struct ActorEntry {
  int16_t id;
  Vec3s pos;
  Vec3s rot;
  int16_t params;
};
struct TransitionActorEntry {
  struct {
    int8_t room;
    int8_t effects;
  } sides[2];
  int16_t id;
  Vec3s pos;
  int16_t rotY;
  int16_t params;
};
struct Path {
  uint8_t count;
  Vec3s *points;
};
struct CollisionPoly {
  uint16_t type;
  uint16_t flags_vIA;
  uint16_t flags_vIB;
  uint16_t vIC;
  Vec3s normal;
  int16_t dist;
};
struct SurfaceType {
  uint32_t data[2];
};
static_assert(sizeof(ActorEntry) == 0x10 &&
                  sizeof(TransitionActorEntry) == 0x10,
              "list entries are 0x10 bytes");
static_assert(sizeof(CollisionPoly) == 0x10 && sizeof(SurfaceType) == 8,
              "collision entries");

bool operator==(const Vec3s &a, const Vec3s &b) {
  return a.x == b.x && a.y == b.y && a.z == b.z;
}
bool operator==(const ActorEntry &a, const ActorEntry &b) {
  return a.id == b.id && a.pos == b.pos && a.rot == b.rot &&
         a.params == b.params;
}
bool operator==(const CollisionPoly &a, const CollisionPoly &b) {
  return std::memcmp(&a, &b, sizeof(a)) == 0;
}
bool operator!=(const CollisionPoly &a, const CollisionPoly &b) {
  return !(a == b);
}
bool operator==(const SurfaceType &a, const SurfaceType &b) {
  return a.data[0] == b.data[0] && a.data[1] == b.data[1];
}
bool operator!=(const SurfaceType &a, const SurfaceType &b) {
  return !(a == b);
}

// --- The bins, read straight from the embedded bytes (not the decoder) ---

uint32_t Be32(const uint8_t *at) {
  return (uint32_t{at[0]} << 24) | (uint32_t{at[1]} << 16) |
         (uint32_t{at[2]} << 8) | at[3];
}
uint16_t Be16(const uint8_t *at) {
  return static_cast<uint16_t>((at[0] << 8) | at[1]);
}
int16_t S16(const uint8_t *at) { return static_cast<int16_t>(Be16(at)); }
Vec3s Vec(const uint8_t *at) { return {S16(at), S16(at + 2), S16(at + 4)}; }

struct RawRoom {
  int dungeon = -1, room = -1;
  uint32_t offset = 0;
  uint16_t actorCount = 0, actorOffset = 0, objectCount = 0, objectOffset = 0;
  ActorEntry Actor(size_t i) const {
    const uint8_t *at =
        Blobs::kMqRooms + offset + actorOffset + i * sizeof(ActorEntry);
    return {S16(at), Vec(at + 2), Vec(at + 8), S16(at + 14)};
  }
  int16_t Object(size_t i) const {
    return S16(Blobs::kMqRooms + offset + objectOffset + i * 2);
  }
};
std::vector<RawRoom> RawRooms() {
  std::vector<RawRoom> rooms;
  for (uint32_t i = 0; i < Be32(Blobs::kMqRooms); i++) {
    const uint8_t *entry = Blobs::kMqRooms + 0x20 + i * 0x20;
    RawRoom room;
    room.offset = Be32(entry + 4);
    room.dungeon = entry[8];
    room.room = entry[9];
    room.actorCount = Be16(entry + 0x0a);
    room.actorOffset = Be16(entry + 0x0c);
    room.objectCount = Be16(entry + 0x0e);
    room.objectOffset = Be16(entry + 0x10);
    rooms.push_back(room);
  }
  return rooms;
}

struct RawScene {
  uint32_t size = 0, offset = 0;
  uint16_t transitionCount = 0, transitionOffset = 0, pathOffset = 0;
  uint32_t polyCount = 0, surfaceCount = 0;
  const uint8_t *Blob() const { return Blobs::kMqScenes + offset; }
};
RawScene FindRawScene(int dungeon) {
  for (uint32_t i = 0; i < Be32(Blobs::kMqScenes); i++) {
    const uint8_t *entry = Blobs::kMqScenes + 0x40 + i * 0x40;
    if (entry[8] != dungeon)
      continue;
    RawScene scene;
    scene.size = Be32(entry);
    scene.offset = Be32(entry + 4);
    scene.transitionCount = Be16(entry + 0x0a);
    scene.transitionOffset = Be16(entry + 0x0c);
    scene.pathOffset = Be16(entry + 0x0e);
    scene.polyCount = Be32(entry + 0x18);
    scene.surfaceCount = Be32(entry + 0x20);
    return scene;
  }
  assert(false && "scene entry not found");
  return {};
}

const char *Name(CommandOverride o) {
  switch (o) {
  case CommandOverride::None:
    return "None";
  case CommandOverride::TransitionActors:
    return "TransitionActors";
  case CommandOverride::Paths:
    return "Paths";
  case CommandOverride::PathsInsteadOfAlternateHeaders:
    return "PathsInsteadOfAlternateHeaders";
  case CommandOverride::Collision:
    return "Collision";
  case CommandOverride::RoomActors:
    return "RoomActors";
  case CommandOverride::RoomObjects:
    return "RoomObjects";
  }
  return "?";
}

std::string Hex(unsigned value) {
  char text[16];
  std::snprintf(text, sizeof(text), "0x%02X", value);
  return text;
}

void Expect(bool condition, const std::string &what) {
  if (!condition) {
    std::fprintf(stderr, "FAIL: %s\n", what.c_str());
    assert(false);
  }
}

// The overrides mq.c gives one command of a dungeon's scene header, from the
// bins themselves: 0x0E always; 0x0D when the entry has paths; Ice Cavern's
// 0x18 likewise; 0x03 for Ganon's Castle and an entry with collision patches.
CommandOverride ExpectedSceneOverride(int dungeon, uint8_t command) {
  const RawScene raw = FindRawScene(dungeon);
  const bool paths = raw.pathOffset != 0xffff;
  switch (command) {
  case 0x0E:
    return CommandOverride::TransitionActors;
  case 0x0D:
    return paths ? CommandOverride::Paths : CommandOverride::None;
  case 0x18:
    return dungeon == kIceCavern && paths
               ? CommandOverride::PathsInsteadOfAlternateHeaders
               : CommandOverride::None;
  case 0x03:
    return dungeon == kGanonsCastle || raw.polyCount || raw.surfaceCount
               ? CommandOverride::Collision
               : CommandOverride::None;
  default:
    return CommandOverride::None;
  }
}

void CheckScenePlan(int dungeon, const Data &data,
                    const std::vector<uint8_t> &ids, const std::string &where) {
  int marker = 0;
  const CommandPlan plan =
      PlanSceneCommands(&marker, dungeon, data.dungeons[dungeon].scene, ids);
  Expect(plan.resource == &marker && plan.commands.size() == ids.size(),
         where + ": plan size");
  bool ended = false;
  for (size_t i = 0; i < ids.size(); i++) {
    const CommandOverride expected =
        ended ? CommandOverride::None : ExpectedSceneOverride(dungeon, ids[i]);
    Expect(plan.commands[i] == expected && plan.At(&marker, i) == expected,
           where + ": command " + std::to_string(i) + " (" + Hex(ids[i]) +
               ") is " + Name(plan.commands[i]) + ", expected " +
               Name(expected));
    ended = ended || ids[i] == 0x14;
  }
}

void CheckRoomPlan(const std::vector<uint8_t> &ids, const std::string &where) {
  int marker = 0;
  const CommandPlan plan = PlanRoomCommands(&marker, ids);
  Expect(plan.commands.size() == ids.size(), where + ": plan size");
  bool ended = false;
  for (size_t i = 0; i < ids.size(); i++) {
    const CommandOverride expected =
        ended            ? CommandOverride::None
        : ids[i] == 0x01 ? CommandOverride::RoomActors
        : ids[i] == 0x0B ? CommandOverride::RoomObjects
                         : CommandOverride::None;
    Expect(plan.commands[i] == expected,
           where + ": room command " + std::to_string(i) + " is " +
               Name(plan.commands[i]) + ", expected " + Name(expected));
    ended = ended || ids[i] == 0x14;
  }
}

// --- Plans and their scope ---

void CheckPlans(const Data &data) {
  // The dungeons with MQ paths and with collision patches, from the bins.
  std::set<int> paths, collision;
  for (int d = 0; d < kDungeonCount; d++) {
    if (FindRawScene(d).pathOffset != 0xffff)
      paths.insert(d);
    if (ExpectedSceneOverride(d, 0x03) == CommandOverride::Collision)
      collision.insert(d);
  }
  assert((paths == std::set<int>{kFireTemple, kWaterTemple, kSpiritTemple,
                                 kBottomOfTheWell, kIceCavern, kGanonsCastle}));
  assert((collision == std::set<int>{kFireTemple, kGanonsCastle}));

  // A header with every command id, a missing command, the end marker, and
  // commands after it.
  std::vector<uint8_t> header = {
      0x18, 0x15, 0x04, 0x0E,        0x19, 0x03, 0x06, 0x07, 0x0D, 0x00, 0x11,
      0x13, 0x0F, 0x01, 0x0B,        0x02, 0x05, 0x08, 0x09, 0x0A, 0x0C, 0x10,
      0x12, 0x16, 0x17, kCmdMissing, 0x14, 0x0E, 0x0D, 0x03, 0x18, 0x01, 0x0B};
  for (int d = 0; d < kDungeonCount; d++) {
    CheckScenePlan(d, data, header,
                   "synthetic scene of dungeon " + std::to_string(d));
    int marker = 0;
    const CommandPlan plan =
        PlanSceneCommands(&marker, d, data.dungeons[d].scene, header);
    // The scene hook never touches a room's lists, and stops at 0x14.
    assert(plan.commands[13] == CommandOverride::None &&
           plan.commands[14] == CommandOverride::None);
    for (size_t i = 26; i < header.size(); i++)
      assert(plan.commands[i] == CommandOverride::None);
    assert(plan.commands[3] == CommandOverride::TransitionActors);
    assert(
        (plan.commands[0] == CommandOverride::PathsInsteadOfAlternateHeaders) ==
        (d == kIceCavern));
  }
  // Jabu-Jabu's alternate headers run as before; Gerudo Training Ground and
  // Shadow keep their vanilla paths (pathOffset 0xffff).
  assert(SceneCommandOverride(kJabuJabu, data.dungeons[kJabuJabu].scene,
                              0x18) == CommandOverride::None);
  for (int d : {kGerudoTrainingGround, kShadowTemple, kDekuTree})
    assert(SceneCommandOverride(d, data.dungeons[d].scene, 0x0D) ==
           CommandOverride::None);

  // Rooms: only 0x01 and 0x0B, before the end marker.
  CheckRoomPlan(header, "synthetic room");
  int room = 0;
  const CommandPlan roomPlan = PlanRoomCommands(&room, header);
  assert(roomPlan.commands[13] == CommandOverride::RoomActors &&
         roomPlan.commands[14] == CommandOverride::RoomObjects &&
         roomPlan.commands[31] == CommandOverride::None &&
         roomPlan.commands[32] == CommandOverride::None);
  for (int sceneOnly : {0x0E, 0x0D, 0x03, 0x18})
    assert(RoomCommandOverride(static_cast<uint8_t>(sceneOnly)) ==
           CommandOverride::None);

  // A plan answers for its own resource only, and an empty plan (a vanilla
  // scene or room) for none.
  int other = 0;
  assert(roomPlan.At(&room, 13) == CommandOverride::RoomActors);
  assert(roomPlan.At(&other, 13) == CommandOverride::None);
  assert(roomPlan.At(nullptr, 13) == CommandOverride::None);
  assert(roomPlan.At(&room, header.size()) == CommandOverride::None);
  const CommandPlan vanilla;
  for (size_t i = 0; i < header.size(); i++)
    assert(vanilla.At(&room, i) == CommandOverride::None &&
           vanilla.At(nullptr, i) == CommandOverride::None);

  // The runner's depth: only the outermost header is top level, and it is
  // again once a nested alternate header has run.
  int depth = 0;
  {
    CommandDepth outer(depth);
    assert(outer.IsTopLevel() && depth == 1);
    {
      CommandDepth nested(depth);
      assert(!nested.IsTopLevel() && !outer.IsTopLevel() && depth == 2);
    }
    assert(outer.IsTopLevel());
  }
  assert(depth == 0);
}

// --- Room buffers ---

using Rooms = RoomBuffers<ActorEntry>;

void ExpectRoomLists(const Rooms::Buffer &buffer, const RawRoom &raw,
                     const std::string &where) {
  Expect(buffer.numActors == raw.actorCount &&
             buffer.numObjects == raw.objectCount,
         where + ": counts");
  for (size_t i = 0; i < raw.actorCount; i++) {
    ActorEntry expected = raw.Actor(i);
    for (const BrokenActor &broken : kMqBrokenActors)
      if (broken.dungeon == raw.dungeon && broken.room == raw.room &&
          broken.actorIndex == i)
        expected.id = -1;
    Expect(buffer.actors[i] == expected,
           where + ": actor " + std::to_string(i) + " differs from the bins");
  }
  for (size_t i = 0; i < raw.objectCount; i++)
    Expect(buffer.objects[i] == raw.Object(i),
           where + ": object " + std::to_string(i) + " differs from the bins");
}

void CheckRoomBuffers(const Data &data) {
  const std::vector<RawRoom> raws = RawRooms();
  assert(raws.size() == 220);
  Rooms buffers;
  size_t empty = 0, removed = 0;
  // The actor each removal hits (mq.c:70-77): En_Kusa, En_Kusa, En_Ru1,
  // En_Firefly, En_Firefly, En_Sw.
  const std::map<std::pair<int, int>, std::vector<std::pair<int, int>>>
      brokenIds = {{{1, 9}, {{0, 0x0125}}},
                   {{1, 0xa}, {{6, 0x0125}}},
                   {{2, 7}, {{1, 0x00a1}}},
                   {{6, 1}, {{2, 0x0013}, {3, 0x0013}}},
                   {{6, 0x17}, {{0, 0x0095}}}};
  for (const RawRoom &raw : raws) {
    const Room *room = data.FindRoom(raw.dungeon, raw.room);
    assert(room != nullptr);
    buffers.Swap();
    const Rooms::Buffer &buffer = buffers.Load(*room);
    assert(&buffer == &buffers.Current());
    const std::string where = "dungeon " + std::to_string(raw.dungeon) +
                              " room " + std::to_string(raw.room);
    ExpectRoomLists(buffer, raw, where);
    if (raw.actorCount == 0)
      empty++;
    auto broken = brokenIds.find({raw.dungeon, raw.room});
    if (broken != brokenIds.end())
      for (auto [index, id] : broken->second) {
        assert(raw.Actor(index).id == id && buffer.actors[index].id == -1);
        removed++;
      }
  }
  assert(empty == 10 && removed == 6);

  // Loading A, B, A: each load keeps the room before it intact; a non-MQ room
  // load only swaps, and changes neither buffer.
  const RawRoom &a = raws[0];
  const RawRoom *b = nullptr;
  for (const RawRoom &raw : raws)
    if (raw.dungeon == kSpiritTemple && raw.room == 1)
      b = &raw;
  assert(b != nullptr);
  Rooms seq;
  seq.Swap();
  seq.Load(*data.FindRoom(a.dungeon, a.room));
  seq.Swap();
  seq.Load(*data.FindRoom(b->dungeon, b->room));
  ExpectRoomLists(seq.Previous(), a, "A after loading B");
  ExpectRoomLists(seq.Current(), *b, "B");
  seq.Swap();
  seq.Load(*data.FindRoom(a.dungeon, a.room));
  ExpectRoomLists(seq.Previous(), *b, "B after loading A again");
  ExpectRoomLists(seq.Current(), a, "A again");
  Rooms before = seq;
  seq.Swap();                                             // a vanilla room
  assert(std::memcmp(&before, &seq, sizeof(Rooms)) != 0); // only the index
  ExpectRoomLists(seq.Previous(), a, "A after a vanilla room");
  ExpectRoomLists(seq.Current(), *b, "the stale buffer a vanilla room skips");
  seq.Swap();
  assert(std::memcmp(&before, &seq, sizeof(Rooms)) == 0);
}

// --- Scene buffer ---

using Scenes = SceneBuffer<TransitionActorEntry, Path, Vec3s>;

void ExpectTransitions(Scenes &buffer, const RawScene &raw, int dungeon) {
  assert(buffer.TransitionCount() == raw.transitionCount);
  for (size_t i = 0; i < raw.transitionCount; i++) {
    const uint8_t *at = raw.Blob() + raw.transitionOffset + i * 0x10;
    const TransitionActorEntry &t = buffer.Transitions()[i];
    Expect(t.sides[0].room == static_cast<int8_t>(at[0]) &&
               t.sides[0].effects == static_cast<int8_t>(at[1]) &&
               t.sides[1].room == static_cast<int8_t>(at[2]) &&
               t.sides[1].effects == static_cast<int8_t>(at[3]) &&
               t.id == S16(at + 4) && t.pos == Vec(at + 6) &&
               t.rotY == S16(at + 12) && t.params == S16(at + 14) && t.id > 0,
           "dungeon " + std::to_string(dungeon) + " transition actor " +
               std::to_string(i) + " differs from the bins");
  }
}

void CheckSceneBuffers(const Data &data) {
  Scenes buffer;
  for (int d = 0; d < kDungeonCount; d++) {
    const RawScene raw = FindRawScene(d);
    buffer.Load(data.dungeons[d].scene);
    ExpectTransitions(buffer, raw, d);
    // Actor_SpawnTransitionActors negates the ids it spawned; the next load
    // of the scene starts from the bins again (mq.c:311).
    for (size_t i = 0; i < buffer.TransitionCount(); i++)
      buffer.Transitions()[i].id =
          static_cast<int16_t>(-buffer.Transitions()[i].id);
    // Reloading the same scene refills the buffer in place: the lists the
    // PlayState was given keep their addresses.
    const TransitionActorEntry *transitions = buffer.Transitions();
    const Path *paths = buffer.Paths();
    std::vector<const Vec3s *> points;
    for (size_t p = 0; p < buffer.PathCount(); p++)
      points.push_back(buffer.Paths()[p].points);
    buffer.Load(data.dungeons[d].scene);
    ExpectTransitions(buffer, raw, d);
    assert(buffer.Transitions() == transitions && buffer.Paths() == paths);
    for (size_t p = 0; p < buffer.PathCount(); p++)
      assert(buffer.Paths()[p].points == points[p]);

    if (raw.pathOffset == 0xffff) {
      assert(buffer.Paths() == nullptr && buffer.PathCount() == 0 &&
             kPathCounts[d] == 0);
      continue;
    }
    // make_mq.ts PATHS bounds the list. The header array and every point
    // array lie inside the entry; mq.c's loop would read the next header,
    // which for Water and Ice Cavern lies at or past the entry's end.
    const size_t count = kPathCounts[d];
    Expect(buffer.PathCount() == count && count > 0,
           "dungeon " + std::to_string(d) + " path count");
    assert(raw.pathOffset + count * 8 <= raw.size);
    if (d == kWaterTemple || d == kIceCavern)
      assert(raw.pathOffset + count * 8 >= raw.size);
    for (size_t p = 0; p < count; p++) {
      const uint8_t *header = raw.Blob() + raw.pathOffset + p * 8;
      const uint32_t pointsOffset = Be32(header + 4);
      assert(pointsOffset + header[0] * 6u <= raw.pathOffset);
      const Path &path = buffer.Paths()[p];
      Expect(path.count == header[0] && path.points != nullptr,
             "dungeon " + std::to_string(d) + " path " + std::to_string(p));
      for (size_t k = 0; k < path.count; k++)
        Expect(path.points[k] == Vec(raw.Blob() + pointsOffset + k * 6),
               "dungeon " + std::to_string(d) + " path " + std::to_string(p) +
                   " point " + std::to_string(k));
    }
    assert(buffer.Paths()[count].count == 0 &&
           buffer.Paths()[count].points == nullptr);
  }
  // A scene without paths after one with them leaves no stale list.
  buffer.Load(data.dungeons[kGanonsCastle].scene);
  assert(buffer.PathCount() == 6);
  buffer.Load(data.dungeons[kShadowTemple].scene);
  assert(buffer.Paths() == nullptr && buffer.PathCount() == 0);
}

// --- Collision patcher ---

using Patched = PatchedCollision<CollisionPoly, SurfaceType>;

std::vector<CollisionPoly> SyntheticPolys(size_t count) {
  std::vector<CollisionPoly> polys(count);
  for (size_t i = 0; i < count; i++) {
    const uint16_t v = static_cast<uint16_t>(i * 0x9E37u + 0x1234u);
    polys[i] = {static_cast<uint16_t>(i % 97),
                v,
                static_cast<uint16_t>(v ^ 0x5A5A),
                static_cast<uint16_t>(i),
                {static_cast<int16_t>(i), static_cast<int16_t>(-1),
                 static_cast<int16_t>(7)},
                static_cast<int16_t>(i * 3)};
  }
  return polys;
}
std::vector<SurfaceType> SyntheticSurfaces(size_t count) {
  std::vector<SurfaceType> surfaces(count);
  for (size_t i = 0; i < count; i++)
    surfaces[i] = {{0xA0000000u + static_cast<uint32_t>(i),
                    0xB0000000u + static_cast<uint32_t>(i)}};
  return surfaces;
}

// mq.c's writes, spelled out independently of MqRuntime.h's PatchPoly.
void ExpectPatch(std::vector<CollisionPoly> &expected, size_t id, int type,
                 int flags) {
  expected[id].type = static_cast<uint16_t>(type);
  expected[id].flags_vIA =
      static_cast<uint16_t>((expected[id].flags_vIA & 0x1fff) | (flags << 13));
}

void ExpectExactPatch(int dungeon, const Scene &scene,
                      const std::vector<CollisionPoly> &polys,
                      const std::vector<SurfaceType> &surfaces,
                      size_t *changedPolys, size_t *changedSurfaces) {
  const std::vector<CollisionPoly> polysBefore = polys;
  const std::vector<SurfaceType> surfacesBefore = surfaces;
  Patched out;
  std::string error = "stale";
  Expect(PatchCollision(dungeon, scene, polys.data(), polys.size(),
                        surfaces.data(), surfaces.size(), out, error) &&
             error.empty(),
         "patching dungeon " + std::to_string(dungeon) + ": " + error);
  // The inputs are only read.
  assert(polys.size() == polysBefore.size() &&
         std::memcmp(polys.data(), polysBefore.data(),
                     polys.size() * sizeof(CollisionPoly)) == 0);
  assert(surfaces.size() == surfacesBefore.size() &&
         std::memcmp(surfaces.data(), surfacesBefore.data(),
                     surfaces.size() * sizeof(SurfaceType)) == 0);
  std::vector<CollisionPoly> expected = polys;
  std::vector<SurfaceType> expectedSurfaces = surfaces;
  if (dungeon == kGanonsCastle) {
    for (size_t id = 2403; id <= 2410; id++)
      ExpectPatch(expected, id, 57, 4);
    for (size_t id = 2473; id <= 2480; id++)
      ExpectPatch(expected, id, 57, 4);
  }
  for (const PolyPatch &p : scene.polyPatches)
    ExpectPatch(expected, p.id, p.type, p.flags);
  for (const SurfacePatch &p : scene.surfacePatches)
    expectedSurfaces[p.id] = {{p.hi, p.lo}};
  assert(out.polys.size() == expected.size() &&
         out.surfaces.size() == expectedSurfaces.size());
  size_t polysChanged = 0, surfacesChanged = 0;
  for (size_t i = 0; i < expected.size(); i++) {
    Expect(out.polys[i] == expected[i], "dungeon " + std::to_string(dungeon) +
                                            " polygon " + std::to_string(i) +
                                            " is not mq.c's");
    if (out.polys[i] != polys[i])
      polysChanged++;
  }
  for (size_t i = 0; i < expectedSurfaces.size(); i++) {
    Expect(out.surfaces[i] == expectedSurfaces[i],
           "dungeon " + std::to_string(dungeon) + " surface type " +
               std::to_string(i) + " is not mq.c's");
    if (out.surfaces[i] != surfaces[i])
      surfacesChanged++;
  }
  if (changedPolys)
    *changedPolys = polysChanged;
  if (changedSurfaces)
    *changedSurfaces = surfacesChanged;
}

void ExpectRefused(int dungeon, const Scene &scene, size_t polyCount,
                   size_t surfaceCount, const char *what) {
  const std::vector<CollisionPoly> polys = SyntheticPolys(polyCount);
  const std::vector<SurfaceType> surfaces = SyntheticSurfaces(surfaceCount);
  Patched out;
  out.polys.resize(1);
  out.polys[0].type = 0xBEEF;
  std::string error;
  const bool ok = PatchCollision(dungeon, scene, polys.data(), polys.size(),
                                 surfaces.data(), surfaces.size(), out, error);
  Expect(!ok && !error.empty(), std::string("not refused: ") + what);
  // Nothing is patched and the output is left as it was.
  assert(out.polys.size() == 1 && out.polys[0].type == 0xBEEF &&
         out.surfaces.empty());
}

void CheckCollisionPatcher(const Data &data) {
  const Scene &fire = data.dungeons[kFireTemple].scene;
  const Scene &ganon = data.dungeons[kGanonsCastle].scene;
  assert(fire.polyPatches.size() == 553 && fire.surfacePatches.size() == 33);
  assert(ganon.polyPatches.empty() && ganon.surfacePatches.empty());
  uint16_t maxPoly = 0;
  uint32_t maxSurface = 0;
  for (const PolyPatch &p : fire.polyPatches)
    maxPoly = std::max(maxPoly, p.id);
  for (const SurfacePatch &p : fire.surfacePatches)
    maxSurface = std::max(maxSurface, p.id);
  assert(maxPoly == 4854 && maxSurface == 66);

  // Fire and Ganon's Castle on synthetic lists of the vanilla sizes.
  const std::vector<CollisionPoly> firePolys = SyntheticPolys(4857);
  const std::vector<SurfaceType> fireSurfaces = SyntheticSurfaces(68);
  size_t polys = 0, surfaces = 0;
  ExpectExactPatch(kFireTemple, fire, firePolys, fireSurfaces, &polys,
                   &surfaces);
  assert(surfaces == 33);
  const std::vector<CollisionPoly> ganonPolys = SyntheticPolys(5096);
  const std::vector<SurfaceType> ganonSurfaces = SyntheticSurfaces(40);
  ExpectExactPatch(kGanonsCastle, ganon, ganonPolys, ganonSurfaces, &polys,
                   &surfaces);
  assert(surfaces == 0 && polys <= 16);
  // Every other dungeon: mqPatchCollisions changes nothing, so the 0x03
  // command runs unchanged (HasCollisionPatches) and a patch would be a copy.
  for (int d = 0; d < kDungeonCount; d++) {
    if (d == kFireTemple || d == kGanonsCastle) {
      assert(HasCollisionPatches(d, data.dungeons[d].scene));
      continue;
    }
    assert(!HasCollisionPatches(d, data.dungeons[d].scene));
    ExpectExactPatch(d, data.dungeons[d].scene, firePolys, fireSurfaces, &polys,
                     &surfaces);
    assert(polys == 0 && surfaces == 0);
  }

  // mq.c's order: Ganon's hardcoded polygons, then the entry's polygon
  // patches, then its surface patches, each in file order; a later write wins.
  Scene ordered;
  ordered.polyPatches = {{2405, 1, 1}, {7, 3, 2}, {7, 9, 5}};
  ordered.surfacePatches = {{3, 0x11, 0x22}, {3, 0x33, 0x44}};
  const std::vector<CollisionPoly> small = SyntheticPolys(2500);
  const std::vector<SurfaceType> few = SyntheticSurfaces(4);
  Patched out;
  std::string error;
  assert(PatchCollision(kGanonsCastle, ordered, small.data(), small.size(),
                        few.data(), few.size(), out, error));
  assert(out.polys[2405].type == 1 && (out.polys[2405].flags_vIA >> 13) == 1 &&
         (out.polys[2405].flags_vIA & 0x1fff) ==
             (small[2405].flags_vIA & 0x1fff));
  assert(out.polys[2406].type == 57 && (out.polys[2406].flags_vIA >> 13) == 4);
  assert(out.polys[7].type == 9 && (out.polys[7].flags_vIA >> 13) == 5);
  assert(out.surfaces[3].data[0] == 0x33 && out.surfaces[3].data[1] == 0x44);
  ExpectExactPatch(kGanonsCastle, ordered, small, few, nullptr, nullptr);
  // Patching the same lists into the same output again keeps its storage
  // and gives the same lists, so a header pointing at them stays valid.
  Patched again;
  assert(PatchCollision(kFireTemple, fire, firePolys.data(), firePolys.size(),
                        fireSurfaces.data(), fireSurfaces.size(), again,
                        error));
  const CollisionPoly *polyStorage = again.polys.data();
  const SurfaceType *surfaceStorage = again.surfaces.data();
  const Patched first = again;
  again.polys[0].type = 0x7777;
  assert(PatchCollision(kFireTemple, fire, firePolys.data(), firePolys.size(),
                        fireSurfaces.data(), fireSurfaces.size(), again,
                        error));
  assert(again.polys.data() == polyStorage &&
         again.surfaces.data() == surfaceStorage);
  assert(std::memcmp(again.polys.data(), first.polys.data(),
                     first.polys.size() * sizeof(CollisionPoly)) == 0 &&
         std::memcmp(again.surfaces.data(), first.surfaces.data(),
                     first.surfaces.size() * sizeof(SurfaceType)) == 0);

  // An id beyond the lists fails closed.
  ExpectRefused(kFireTemple, fire, 4854, 68, "Fire polygon 4854 of 4854");
  ExpectRefused(kFireTemple, fire, 4857, 66, "Fire surface type 66 of 66");
  ExpectRefused(kGanonsCastle, ganon, 2480, 40, "Ganon polygon 2480 of 2480");
  ExpectRefused(kGanonsCastle, ganon, 2410, 40, "Ganon polygon 2410 of 2410");
}

// --- The vanilla headers: fixtures/mq-vanilla-command-ids.txt, or the user's
// archive (mq_runtime_archive_tests.py) ---

struct Fixture {
  std::map<int, std::vector<uint8_t>> scenes;
  std::map<std::pair<int, int>, std::vector<uint8_t>> rooms;
  std::map<int, std::vector<CollisionPoly>> polys;
  std::map<int, std::vector<SurfaceType>> surfaces;
};

Fixture ReadFixture(const char *path) {
  std::ifstream in(path);
  Expect(static_cast<bool>(in), std::string("cannot open ") + path);
  Fixture fixture;
  std::string kind;
  while (in >> kind) {
    if (kind[0] == '#') { // a comment line
      std::getline(in, kind);
      continue;
    }
    int dungeon = 0;
    size_t count = 0;
    if (kind == "scene" || kind == "room") {
      int room = 0;
      in >> dungeon;
      if (kind == "room")
        in >> room;
      in >> count;
      std::vector<uint8_t> ids(count);
      for (uint8_t &id : ids) {
        unsigned value = 0;
        in >> value;
        id = static_cast<uint8_t>(value);
      }
      if (kind == "scene")
        fixture.scenes[dungeon] = ids;
      else
        fixture.rooms[{dungeon, room}] = ids;
    } else if (kind == "polys") {
      in >> dungeon >> count;
      std::vector<CollisionPoly> &polys = fixture.polys[dungeon];
      polys.resize(count);
      for (CollisionPoly &p : polys) {
        int v[8];
        for (int &field : v)
          in >> field;
        p = {static_cast<uint16_t>(v[0]),
             static_cast<uint16_t>(v[1]),
             static_cast<uint16_t>(v[2]),
             static_cast<uint16_t>(v[3]),
             {static_cast<int16_t>(v[4]), static_cast<int16_t>(v[5]),
              static_cast<int16_t>(v[6])},
             static_cast<int16_t>(v[7])};
      }
    } else if (kind == "surfaces") {
      in >> dungeon >> count;
      std::vector<SurfaceType> &surfaces = fixture.surfaces[dungeon];
      surfaces.resize(count);
      for (SurfaceType &s : surfaces)
        in >> s.data[0] >> s.data[1];
    } else {
      Expect(false, "unknown fixture line " + kind);
    }
    Expect(!in.fail(), "malformed fixture line " + kind);
  }
  return fixture;
}

// The plans on the vanilla scene and room command ids of the 12 dungeons.
void CheckVanillaCommandPlans(const Data &data, const Fixture &fixture) {
  assert(fixture.scenes.size() == 12 && fixture.rooms.size() == 220);
  const std::vector<RawRoom> raws = RawRooms();

  // Scenes: each overrides its 0x0E, the path dungeons' 0x0D, Ice Cavern's
  // 0x18 and Fire's and Ganon's 0x03, and nothing else.
  size_t sceneOverrides = 0;
  for (const auto &[dungeon, ids] : fixture.scenes) {
    CheckScenePlan(dungeon, data, ids,
                   "vanilla scene of dungeon " + std::to_string(dungeon));
    int marker = 0;
    const CommandPlan plan =
        PlanSceneCommands(&marker, dungeon, data.dungeons[dungeon].scene, ids);
    std::map<CommandOverride, int> found;
    for (CommandOverride o : plan.commands)
      if (o != CommandOverride::None)
        found[o]++;
    std::map<CommandOverride, int> expected = {
        {CommandOverride::TransitionActors, 1}};
    if (FindRawScene(dungeon).pathOffset != 0xffff)
      expected[dungeon == kIceCavern
                   ? CommandOverride::PathsInsteadOfAlternateHeaders
                   : CommandOverride::Paths] = 1;
    if (dungeon == kFireTemple || dungeon == kGanonsCastle) {
      expected[CommandOverride::Collision] = 1;
      // No alternate header could run another 0x03 on the same collision.
      for (uint8_t id : ids)
        assert(id != 0x18);
    }
    Expect(found == expected, "vanilla scene of dungeon " +
                                  std::to_string(dungeon) +
                                  ": unexpected set of overrides");
    for (CommandOverride o : plan.commands)
      sceneOverrides += o != CommandOverride::None;
    // Jabu-Jabu's alternate headers still run.
    if (dungeon == kJabuJabu)
      for (size_t i = 0; i < ids.size(); i++)
        if (ids[i] == 0x18)
          assert(plan.commands[i] == CommandOverride::None);
  }
  // 12 transition lists, 5 path lists, Ice Cavern's, Fire and Ganon.
  assert(sceneOverrides == 12 + 5 + 1 + 2);

  // Rooms: the 0x01 and 0x0B commands, and the lists they then point at are
  // the bins'. Rooms the bins give no actors have no vanilla 0x01.
  Rooms buffers;
  size_t actorLists = 0;
  for (const RawRoom &raw : raws) {
    const auto it = fixture.rooms.find({raw.dungeon, raw.room});
    assert(it != fixture.rooms.end());
    const std::vector<uint8_t> &ids = it->second;
    const std::string where = "vanilla room " + std::to_string(raw.room) +
                              " of dungeon " + std::to_string(raw.dungeon);
    CheckRoomPlan(ids, where);
    int marker = 0;
    const CommandPlan plan = PlanRoomCommands(&marker, ids);
    buffers.Swap();
    const Rooms::Buffer &buffer =
        buffers.Load(*data.FindRoom(raw.dungeon, raw.room));
    bool actors = false, objects = false;
    for (size_t i = 0; i < plan.commands.size(); i++) {
      if (plan.At(&marker, i) == CommandOverride::RoomActors)
        actors = true;
      if (plan.At(&marker, i) == CommandOverride::RoomObjects)
        objects = true;
    }
    Expect(actors == (raw.actorCount != 0), where + ": actor list command");
    Expect(objects || raw.objectCount == 0, where + ": object list command");
    if (actors)
      actorLists++;
    ExpectRoomLists(buffer, raw, where);
  }
  assert(actorLists == 220 - 10);

  // A vanilla load plans nothing: no command of any scene or room changes.
  const CommandPlan vanilla;
  for (const auto &[dungeon, ids] : fixture.scenes)
    for (size_t i = 0; i < ids.size(); i++)
      assert(vanilla.At(&ids, i) == CommandOverride::None);
  for (const auto &[key, ids] : fixture.rooms)
    for (size_t i = 0; i < ids.size(); i++)
      assert(vanilla.At(&ids, i) == CommandOverride::None);
}

// The committed command ids, in CI.
int RunCommandFixture(const Data &data, const char *path) {
  const Fixture fixture = ReadFixture(path);
  Expect(fixture.polys.empty() && fixture.surfaces.empty(),
         "the command fixture holds command ids only");
  CheckVanillaCommandPlans(data, fixture);
  std::printf("PASS: on %s, the 12 vanilla scenes and 220 rooms take exactly "
              "mq.c's overrides with the bins' lists, and a vanilla load "
              "takes none\n",
              path);
  return 0;
}

int RunArchive(const Data &data, const char *path) {
  const Fixture fixture = ReadFixture(path);
  CheckVanillaCommandPlans(data, fixture);

  // Collision: the vanilla Fire and Ganon lists patched exactly, the
  // vanilla lists untouched.
  const auto &firePolys = fixture.polys.at(kFireTemple);
  const auto &fireSurfaces = fixture.surfaces.at(kFireTemple);
  assert(firePolys.size() == 4857 && fireSurfaces.size() == 68);
  size_t polys = 0, surfaces = 0;
  ExpectExactPatch(kFireTemple, data.dungeons[kFireTemple].scene, firePolys,
                   fireSurfaces, &polys, &surfaces);
  Expect(polys == 553, "Fire: " + std::to_string(polys) + " polygons changed");
  Expect(surfaces == 33,
         "Fire: " + std::to_string(surfaces) + " surface types changed");
  const auto &ganonPolys = fixture.polys.at(kGanonsCastle);
  const auto &ganonSurfaces = fixture.surfaces.at(kGanonsCastle);
  assert(ganonPolys.size() == 5096);
  ExpectExactPatch(kGanonsCastle, data.dungeons[kGanonsCastle].scene,
                   ganonPolys, ganonSurfaces, &polys, &surfaces);
  Expect(polys == 16 && surfaces == 0,
         "Ganon: " + std::to_string(polys) + " polygons changed");
  for (uint16_t id : kGanonPatchedPolys)
    assert(ganonPolys[id].type == 57 && (ganonPolys[id].flags_vIA >> 13) == 0);

  std::printf("PASS: on %s, the 12 vanilla scenes and 220 rooms take exactly "
              "mq.c's overrides with the bins' lists, a vanilla load takes "
              "none, and Fire (553 polygons, 33 surface types) and Ganon's "
              "Castle (16 polygons) patch exactly\n",
              path);
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
  if (argc == 3 && std::string(argv[1]) == "--archive-fixture")
    return RunArchive(*data, argv[2]);
  if (argc == 3 && std::string(argv[1]) == "--command-fixture")
    return RunCommandFixture(*data, argv[2]);
  CheckPlans(*data);
  CheckRoomBuffers(*data);
  CheckSceneBuffers(*data);
  CheckCollisionPatcher(*data);
  std::puts("PASS: mq.c's scene and room overrides are planned per command up "
            "to the end marker for the planned header only; the room and "
            "scene buffers hold the bins' lists with the broken actors "
            "removed in place; collision patches apply in mq.c's order on "
            "copies and fail closed");
  return 0;
}
