// combo/mq: the Master Quest bins from OoTMM v32.3 as compiled into the
// library, and their decoder. Pins the decoded counts and the fixes make_mq.ts
// bakes in, and checks that damaged copies are refused.
// `--dump-json <file>` writes the decoded data for mq_bins_reference_tests.py,
// which compares it with an independent Python reader (mq_bins.py).
#include "ComboMqBlobs.h"
#include "mq/ComboMqData.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <functional>
#include <iterator>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace ComboMq;

namespace {

std::vector<uint8_t> ReadFile(const std::string &path) {
  std::ifstream in(path, std::ios::binary);
  assert(in && "a vendored bin is missing");
  return std::vector<uint8_t>(std::istreambuf_iterator<char>(in),
                              std::istreambuf_iterator<char>());
}

uint32_t Be32(const uint8_t *at) {
  return (uint32_t{at[0]} << 24) | (uint32_t{at[1]} << 16) |
         (uint32_t{at[2]} << 8) | at[3];
}
uint16_t Be16(const uint8_t *at) {
  return static_cast<uint16_t>((at[0] << 8) | at[1]);
}
void PutBe16(std::vector<uint8_t> &bytes, size_t at, uint16_t value) {
  bytes[at] = static_cast<uint8_t>(value >> 8);
  bytes[at + 1] = static_cast<uint8_t>(value);
}
void PutBe32(std::vector<uint8_t> &bytes, size_t at, uint32_t value) {
  for (int i = 0; i < 4; i++)
    bytes[at + i] = static_cast<uint8_t>(value >> (24 - 8 * i));
}

struct Files {
  std::vector<uint8_t> rooms, scenes, maps;
  Bins View() const {
    Bins bins;
    bins.rooms = rooms.data();
    bins.roomsSize = rooms.size();
    bins.scenes = scenes.data();
    bins.scenesSize = scenes.size();
    bins.maps = maps.data();
    bins.mapsSize = maps.size();
    return bins;
  }
};

Files Embedded() {
  using namespace ComboMq::Blobs;
  return {std::vector<uint8_t>(kMqRooms, kMqRooms + kMqRoomsSize),
          std::vector<uint8_t>(kMqScenes, kMqScenes + kMqScenesSize),
          std::vector<uint8_t>(kMqMaps, kMqMaps + kMqMapsSize)};
}

// Index of a scene's entry in mq_scenes.bin.
size_t SceneEntry(const std::vector<uint8_t> &scenes, int dungeon) {
  for (size_t i = 0; i < Be32(scenes.data()); i++)
    if (scenes[0x40 + i * 0x40 + 8] == dungeon)
      return 0x40 + i * 0x40;
  assert(false && "scene entry not found");
  return 0;
}
// Index of a room's entry in mq_rooms.bin.
size_t RoomEntry(const std::vector<uint8_t> &rooms, int dungeon, int room) {
  for (size_t i = 0; i < Be32(rooms.data()); i++)
    if (rooms[0x20 + i * 0x20 + 8] == dungeon &&
        rooms[0x20 + i * 0x20 + 9] == room)
      return 0x20 + i * 0x20;
  assert(false && "room entry not found");
  return 0;
}

void ExpectRejected(const Files &files, const char *what) {
  Data out;
  out.dungeons[0].rooms.resize(1);
  out.dungeons[0].rooms[0].room = 99;
  std::string error;
  bool ok = Decode(files.View(), out, error);
  if (ok || error.empty())
    std::fprintf(stderr, "a damaged copy decoded: %s\n", what);
  assert(!ok && !error.empty());
  // The output is left as it was.
  assert(out.RoomCount() == 1 && out.dungeons[0].rooms[0].room == 99);
}

void CheckEmbeddedMatchesVendoredFiles() {
  const std::string dir = COMBO_MQ_DATA_DIR;
  const Files files = Embedded();
  assert(files.rooms == ReadFile(dir + "/mq_rooms.bin"));
  assert(files.scenes == ReadFile(dir + "/mq_scenes.bin"));
  assert(files.maps == ReadFile(dir + "/mq_maps.bin"));
  assert(files.rooms.size() == 58896 && files.scenes.size() == 36352 &&
         files.maps.size() == 0x4158);
}

void CheckSceneMapping() {
  for (int scene = 0; scene <= 9; scene++)
    assert(DungeonFromScene(scene) == scene);
  assert(DungeonFromScene(0x0B) == kGerudoTrainingGround &&
         DungeonFromScene(0x0D) == kGanonsCastle);
  for (int scene : {-1, 0x0A, 0x0C, 0x0E, 0x11, 0x12, 0x17, 0x1A, 0x51, 0x6E})
    assert(DungeonFromScene(scene) == -1);
  static_assert(DungeonFromScene(0x0D) == 11,
                "Inside Ganon's Castle is OoTMM's MQ_GANON_CASTLE");
}

void CheckRooms(const Data &data) {
  assert(data.RoomCount() == 220);
  const size_t perDungeon[kDungeonCount] = {12, 17, 16, 23, 27, 23,
                                            29, 23, 7,  12, 11, 20};
  size_t maxActors = 0, maxObjects = 0;
  std::set<std::pair<int, int>> empty;
  for (int dungeon = 0; dungeon < kDungeonCount; dungeon++) {
    const auto &rooms = data.dungeons[dungeon].rooms;
    assert(rooms.size() == perDungeon[dungeon]);
    for (size_t room = 0; room < rooms.size(); room++) {
      assert(rooms[room].dungeon == dungeon && rooms[room].room == room);
      assert(data.FindRoom(dungeon, static_cast<int>(room)) == &rooms[room]);
      maxActors = std::max(maxActors, rooms[room].actors.size());
      maxObjects = std::max(maxObjects, rooms[room].objects.size());
      assert(!rooms[room].objects.empty());
      if (rooms[room].actors.empty())
        empty.insert({dungeon, static_cast<int>(room)});
    }
    assert(data.FindRoom(dungeon, static_cast<int>(rooms.size())) == nullptr);
  }
  assert(data.FindRoom(-1, 0) == nullptr &&
         data.FindRoom(kDungeonCount, 0) == nullptr);
  assert(data.FindRoom(kDekuTree, -1) == nullptr);
  assert(maxActors == 54 && maxObjects == 15);
  // These rooms have no MQ actor-list command, so OoTMM clears the vanilla
  // list.
  const std::set<std::pair<int, int>> expectedEmpty = {
      {kDekuTree, 11},     {kDodongosCavern, 16}, {kJabuJabu, 15},
      {kForestTemple, 22}, {kFireTemple, 26},     {kWaterTemple, 22},
      {kSpiritTemple, 11}, {kSpiritTemple, 28},   {kShadowTemple, 22},
      {kIceCavern, 10}};
  assert(empty == expectedEmpty);

  // make_mq.ts "Shadow fixes": two Obj_Roomtimer (0x187) params set to 0xffff.
  const ActorEntryPOD &shadow6 = data.FindRoom(kShadowTemple, 6)->actors.at(11);
  const ActorEntryPOD &shadow7 = data.FindRoom(kShadowTemple, 7)->actors.at(4);
  assert(shadow6.id == 0x187 &&
         static_cast<uint16_t>(shadow6.params) == 0xffff);
  assert(shadow7.id == 0x187 &&
         static_cast<uint16_t>(shadow7.params) == 0xffff);
  assert(shadow7.pos.x == 0xd1e &&
         shadow7.pos.y == static_cast<int16_t>(0xfde1) &&
         shadow7.pos.z == 0x260);
  // make_mq.ts "MQ Fire - Like like": object_gi_shield_1 and _2 appended in
  // room 0x11.
  const std::vector<int16_t> fireObjects = {0x2c, 0xc9, 0x0e, 0xd4,
                                            0xf2, 0xcb, 0xdc};
  assert(data.FindRoom(kFireTemple, 0x11)->objects == fireObjects);
  // The six actors mq.c kMqBrokenActors removes at runtime are still in the
  // data, at those indices.
  const struct {
    int dungeon, room;
    size_t index;
    int16_t id;
  } broken[] = {
      {kDodongosCavern, 0x9, 0, 0x125}, {kDodongosCavern, 0xa, 6, 0x125},
      {kJabuJabu, 0x7, 1, 0xa1},        {kSpiritTemple, 0x1, 2, 0x13},
      {kSpiritTemple, 0x1, 3, 0x13},    {kSpiritTemple, 0x17, 0, 0x95}};
  for (const auto &actor : broken)
    assert(
        data.FindRoom(actor.dungeon, actor.room)->actors.at(actor.index).id ==
        actor.id);
  // The first entry in the file: Deku Tree room 0 has 33 actors and 14 objects.
  assert(data.FindRoom(kDekuTree, 0)->actors.size() == 0x21 &&
         data.FindRoom(kDekuTree, 0)->objects.size() == 0x0e);
}

void CheckScenes(const Data &data, const Files &files) {
  const size_t transitions[kDungeonCount] = {12, 23, 22, 29, 37, 27,
                                             33, 23, 15, 12, 22, 21};
  const std::vector<std::vector<size_t>> pathPoints = {{},
                                                       {},
                                                       {},
                                                       {},
                                                       {2, 2, 2, 2},
                                                       {4, 3},
                                                       {2, 2, 2, 9, 9},
                                                       {},
                                                       {6, 6},
                                                       {6, 5, 6, 3, 10},
                                                       {},
                                                       {5, 8, 25, 7, 2, 2}};
  const size_t minimapRooms[kDungeonCount] = {13, 19, 16, 27, 38, 44,
                                              33, 27, 10, 12, 0,  0};
  for (int dungeon = 0; dungeon < kDungeonCount; dungeon++) {
    const Scene &scene = data.dungeons[dungeon].scene;
    assert(scene.transitions.size() == transitions[dungeon]);
    assert(scene.hasPaths == (kPathCounts[dungeon] != 0) &&
           scene.paths.size() == kPathCounts[dungeon]);
    assert(scene.paths.size() == pathPoints[dungeon].size());
    for (size_t p = 0; p < scene.paths.size(); p++)
      assert(scene.paths[p].size() == pathPoints[dungeon][p]);
    assert(scene.minimap.size() == minimapRooms[dungeon]);
    if (dungeon != kFireTemple)
      assert(scene.polyPatches.empty() && scene.surfacePatches.empty());
  }
  // Water and Ice: the path headers end exactly at the entry's size, so there
  // is no zero terminator to read.
  for (int dungeon : {kWaterTemple, kIceCavern}) {
    const size_t entry = SceneEntry(files.scenes, dungeon);
    const uint32_t size = Be32(&files.scenes[entry]);
    const uint16_t pathOffset = Be16(&files.scenes[entry + 0x0e]);
    assert(pathOffset + 8u * kPathCounts[dungeon] == size);
  }
  // make_mq.ts "Water temple door fix": transition actor 0x16 is En_Door with
  // params 0x0082, rooms 15 and 12.
  const TransitionActorPOD &door =
      data.dungeons[kWaterTemple].scene.transitions.at(0x16);
  assert(door.id == 0x0009 && door.params == 0x0082 &&
         door.sides[0].room == 15 && door.sides[1].room == 12);
  assert(door.sides[0].effects == -1 && door.sides[1].effects == -1);
  assert(door.pos.x == -880 && door.pos.y == 120 && door.pos.z == -2690 &&
         door.rotY == 0);

  // mqdata_fire.json ColDelta: 553 polygon patches and 33 surface-type patches
  // (ids 34..66).
  const Scene &fire = data.dungeons[kFireTemple].scene;
  assert(fire.polyPatches.size() == 553 && fire.surfacePatches.size() == 33);
  uint16_t maxId = 0;
  uint8_t maxType = 0;
  for (const PolyPatch &patch : fire.polyPatches) {
    maxId = std::max(maxId, patch.id);
    maxType = std::max(maxType, patch.type);
    assert(patch.flags <= 7);
  }
  assert(maxId == 4854 && maxType == 66 && fire.polyPatches.front().id == 1054);
  for (size_t i = 0; i < fire.surfacePatches.size(); i++)
    assert(fire.surfacePatches[i].id == 34 + i);

  // make_mq.ts "Fire Temple fix": minimap room 3 gets one chest (flag 0 at
  // 0x48, 0x3c), then no second record.
  const MinimapRoomPOD &fire3 = fire.minimap.at(3);
  assert(fire3.icons[0].markType == 0 && fire3.icons[0].count == 1);
  assert(fire3.icons[0].points[0].chestFlag == 0 &&
         fire3.icons[0].points[0].x == 0x48 &&
         fire3.icons[0].points[0].y == 0x3c);
  assert(fire3.icons[1].markType == -1);
}

void CheckPauseMaps(const Data &data) {
  // make_mq.ts "PATCH FIRE TEMPLE MISSING CHEST": floor 17 gets a 6th chest,
  // flag 0 at (46.5, -17.5).
  const PauseMarkPOD &fire4 = data.pauseFloors[17][0];
  assert(fire4.markType == 0 && fire4.count == 6 && fire4.hasVtx &&
         fire4.vtxCount == 4 && fire4.unk04 == 23);
  assert(fire4.points[5].chestFlag == 0 && fire4.points[5].x == 46.5f &&
         fire4.points[5].y == -17.5f);
  const int16_t fireFlags[] = {1, 2, 4, 7, 12, 0};
  for (size_t i = 0; i < 6; i++)
    assert(fire4.points[i].chestFlag == fireFlags[i]);
  assert(data.pauseFloors[17][1].markType == 1 &&
         data.pauseFloors[17][1].count == 1);
  assert(data.pauseFloors[17][2].markType == -1 &&
         !data.pauseFloors[17][2].hasVtx);
  // Deku Tree's first floor, as SoH draws it: chests 2 and 6.
  const PauseMarkPOD &deku = data.pauseFloors[0][0];
  assert(deku.markType == 0 && deku.count == 2 && deku.hasVtx);
  assert(deku.points[0].chestFlag == 2 && deku.points[0].x == 40.0f &&
         deku.points[0].y == -33.0f);
  assert(deku.points[1].chestFlag == 6 && deku.points[1].x == 49.0f &&
         deku.points[1].y == -42.0f);
}

void CheckBundled() {
  const Data *bundled = Bundled();
  if (!bundled)
    std::fprintf(stderr, "bundled MQ data failed to decode: %s\n",
                 BundledError().c_str());
  assert(bundled && BundledError().empty());
  assert(Bundled() == bundled);
  const Bins embedded = EmbeddedBins();
  assert(embedded.rooms == ComboMq::Blobs::kMqRooms &&
         embedded.roomsSize == sizeof(ComboMq::Blobs::kMqRooms));
  assert(embedded.scenes == ComboMq::Blobs::kMqScenes &&
         embedded.mapsSize == sizeof(ComboMq::Blobs::kMqMaps));
}

void CheckFailClosed(const Files &good) {
  // Every truncation is refused: each file ends with the last blob or record.
  std::vector<uint8_t> Files::*const members[] = {&Files::rooms, &Files::scenes,
                                                  &Files::maps};
  for (auto member : members) {
    const size_t size = (good.*member).size();
    std::vector<size_t> lengths;
    for (size_t length = 0; length < size; length += 61)
      lengths.push_back(length);
    for (size_t cut = 1; cut <= 16; cut++)
      lengths.push_back(size - cut);
    for (size_t length : lengths) {
      Files files = good;
      (files.*member).resize(length);
      ExpectRejected(files, "truncated");
    }
  }
  {
    Files files = good;
    files.maps.push_back(0);
    ExpectRejected(files, "mq_maps.bin one byte long");
  }
  // Damaged fields, one at a time.
  const std::vector<std::pair<const char *, std::function<void(Files &)>>>
      damage = {
          {"room count past the table",
           [](Files &f) { PutBe32(f.rooms, 0, 0x10000); }},
          {"room header reserved byte", [](Files &f) { f.rooms[0x10] = 1; }},
          {"room entry reserved byte",
           [](Files &f) { f.rooms[RoomEntry(f.rooms, 0, 0) + 0x1f] = 1; }},
          {"room blob outside the file",
           [](Files &f) {
             PutBe32(f.rooms, RoomEntry(f.rooms, kGanonsCastle, 19) + 4,
                     0xfff0);
           }},
          {"room blob inside the entry table",
           [](Files &f) {
             PutBe32(f.rooms, RoomEntry(f.rooms, 0, 0) + 4, 0x40);
           }},
          {"room blob over OoTMM's buffer",
           [](Files &f) { PutBe32(f.rooms, RoomEntry(f.rooms, 0, 0), 0x420); }},
          {"65 actors",
           [](Files &f) {
             PutBe16(f.rooms, RoomEntry(f.rooms, 0, 0) + 0x0a, 65);
           }},
          {"16 objects",
           [](Files &f) {
             PutBe16(f.rooms, RoomEntry(f.rooms, 0, 0) + 0x0e, 16);
           }},
          {"actor list past the blob",
           [](Files &f) {
             PutBe16(f.rooms, RoomEntry(f.rooms, 0, 0) + 0x0c, 0x200);
           }},
          {"dungeon 12",
           [](Files &f) {
             f.rooms[RoomEntry(f.rooms, kGanonsCastle, 19) + 8] = 12;
           }},
          {"duplicate room",
           [](Files &f) { f.rooms[RoomEntry(f.rooms, kDekuTree, 1) + 9] = 0; }},
          {"missing room 0",
           [](Files &f) {
             f.rooms[RoomEntry(f.rooms, kDekuTree, 0) + 9] = 12;
           }},
          {"scene count 13", [](Files &f) { PutBe32(f.scenes, 0, 13); }},
          {"scene header reserved byte", [](Files &f) { f.scenes[0x3f] = 1; }},
          {"scene entry pad byte",
           [](Files &f) { f.scenes[SceneEntry(f.scenes, kDekuTree) + 9] = 1; }},
          {"duplicate scene",
           [](Files &f) {
             f.scenes[SceneEntry(f.scenes, kDodongosCavern) + 8] = kDekuTree;
           }},
          {"scene blob over OoTMM's buffer",
           [](Files &f) {
             PutBe32(f.scenes, SceneEntry(f.scenes, kDekuTree), 0x410);
           }},
          {"65 transition actors",
           [](Files &f) {
             PutBe16(f.scenes, SceneEntry(f.scenes, kFireTemple) + 0x0a, 65);
           }},
          {"paths where make_mq.ts has none",
           [](Files &f) {
             PutBe16(f.scenes, SceneEntry(f.scenes, kDekuTree) + 0x0e, 0);
           }},
          {"no paths where make_mq.ts has some",
           [](Files &f) {
             PutBe16(f.scenes, SceneEntry(f.scenes, kWaterTemple) + 0x0e,
                     0xffff);
           }},
          {"misaligned path headers",
           [](Files &f) {
             const size_t entry = SceneEntry(f.scenes, kSpiritTemple);
             PutBe16(f.scenes, entry + 0x0e,
                     static_cast<uint16_t>(Be16(&f.scenes[entry + 0x0e]) - 2));
           }},
          {"path headers past the entry",
           [](Files &f) {
             const size_t entry = SceneEntry(f.scenes, kIceCavern);
             PutBe16(f.scenes, entry + 0x0e,
                     static_cast<uint16_t>(Be16(&f.scenes[entry + 0x0e]) + 4));
           }},
          {"zero-point path",
           [](Files &f) {
             const size_t entry = SceneEntry(f.scenes, kGanonsCastle);
             f.scenes[Be32(&f.scenes[entry + 4]) +
                      Be16(&f.scenes[entry + 0x0e]) + 8] = 0;
           }},
          {"path points over the headers",
           [](Files &f) {
             const size_t entry = SceneEntry(f.scenes, kGanonsCastle);
             f.scenes[Be32(&f.scenes[entry + 4]) +
                      Be16(&f.scenes[entry + 0x0e]) + 8 * 5] = 0xff;
           }},
          {"minimap mark type 2",
           [](Files &f) {
             f.scenes[Be32(&f.scenes[SceneEntry(f.scenes, kForestTemple) +
                                     0x14]) +
                      0x26] = 2;
           }},
          {"minimap 13 points",
           [](Files &f) {
             f.scenes[Be32(&f.scenes[SceneEntry(f.scenes, kForestTemple) +
                                     0x14]) +
                      1] = 13;
           }},
          {"no minimap below scene 10",
           [](Files &f) {
             PutBe32(f.scenes, SceneEntry(f.scenes, kBottomOfTheWell) + 0x10,
                     0);
           }},
          {"minimap for GTG",
           [](Files &f) {
             PutBe32(f.scenes,
                     SceneEntry(f.scenes, kGerudoTrainingGround) + 0x10, 0x80);
           }},
          {"polygon patches past the file",
           [](Files &f) {
             PutBe32(f.scenes, SceneEntry(f.scenes, kFireTemple) + 0x18,
                     0x40000000);
           }},
          {"polygon flags 8",
           [](Files &f) {
             f.scenes[Be32(
                          &f.scenes[SceneEntry(f.scenes, kFireTemple) + 0x1c]) +
                      3] = 8;
           }},
          {"surface patches past the file",
           [](Files &f) {
             PutBe32(f.scenes, SceneEntry(f.scenes, kFireTemple) + 0x20,
                     0x1000);
           }},
          {"pause mark type 2",
           [](Files &f) { PutBe16(f.maps, 17 * 0x1ec, 2); }},
          {"pause 13 points",
           [](Files &f) { PutBe32(f.maps, 17 * 0x1ec + 0x10, 13); }},
          {"pause -1 points",
           [](Files &f) {
             PutBe32(f.maps, 33 * 0x1ec + 2 * 0xa4 + 0x10, 0xffffffff);
           }},
      };
  for (const auto &[what, apply] : damage) {
    Files files = good;
    apply(files);
    ExpectRejected(files, what);
  }
  // A null file is refused too.
  Files missing = good;
  Bins bins = missing.View();
  bins.scenes = nullptr;
  Data out;
  std::string error;
  assert(!Decode(bins, out, error) && !error.empty());
}

// --dump-json: the decoded data in mq_bins.py's shape. Floats are written as
// their bit patterns.
void DumpJson(const Data &data, const char *path) {
  std::FILE *out = std::fopen(path, "wb");
  assert(out);
  auto bits = [](float value) {
    uint32_t raw;
    std::memcpy(&raw, &value, sizeof(raw));
    return static_cast<unsigned long>(raw);
  };
  std::fprintf(out, "{\"rooms\":[");
  bool first = true;
  for (const Dungeon &dungeon : data.dungeons) {
    for (const Room &room : dungeon.rooms) {
      std::fprintf(out, "%s{\"dungeon\":%d,\"room\":%d,\"actors\":[",
                   first ? "" : ",", room.dungeon, room.room);
      first = false;
      for (size_t i = 0; i < room.actors.size(); i++) {
        const ActorEntryPOD &a = room.actors[i];
        std::fprintf(out, "%s[%d,%d,%d,%d,%d,%d,%d,%d]", i ? "," : "", a.id,
                     a.pos.x, a.pos.y, a.pos.z, a.rot.x, a.rot.y, a.rot.z,
                     a.params);
      }
      std::fprintf(out, "],\"objects\":[");
      for (size_t i = 0; i < room.objects.size(); i++)
        std::fprintf(out, "%s%d", i ? "," : "", room.objects[i]);
      std::fprintf(out, "]}");
    }
  }
  std::fprintf(out, "],\"scenes\":[");
  for (int d = 0; d < kDungeonCount; d++) {
    const Scene &scene = data.dungeons[d].scene;
    std::fprintf(out, "%s{\"dungeon\":%d,\"transitions\":[", d ? "," : "", d);
    for (size_t i = 0; i < scene.transitions.size(); i++) {
      const TransitionActorPOD &t = scene.transitions[i];
      std::fprintf(out, "%s[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]", i ? "," : "",
                   t.sides[0].room, t.sides[0].effects, t.sides[1].room,
                   t.sides[1].effects, t.id, t.pos.x, t.pos.y, t.pos.z, t.rotY,
                   t.params);
    }
    std::fprintf(out, "],\"hasPaths\":%s,\"paths\":[",
                 scene.hasPaths ? "true" : "false");
    for (size_t p = 0; p < scene.paths.size(); p++) {
      std::fprintf(out, "%s[", p ? "," : "");
      for (size_t k = 0; k < scene.paths[p].size(); k++)
        std::fprintf(out, "%s[%d,%d,%d]", k ? "," : "", scene.paths[p][k].x,
                     scene.paths[p][k].y, scene.paths[p][k].z);
      std::fprintf(out, "]");
    }
    std::fprintf(out, "],\"minimap\":[");
    for (size_t r = 0; r < scene.minimap.size(); r++) {
      std::fprintf(out, "%s[", r ? "," : "");
      for (size_t k = 0; k < 3; k++) {
        const MinimapIconsPOD &icons = scene.minimap[r].icons[k];
        std::fprintf(out, "%s[%d,%d,[", k ? "," : "", icons.markType,
                     icons.count);
        for (size_t p = 0; p < kMinimapPointsMax; p++)
          std::fprintf(out, "%s[%d,%d,%d]", p ? "," : "",
                       icons.points[p].chestFlag, icons.points[p].x,
                       icons.points[p].y);
        std::fprintf(out, "]]");
      }
      std::fprintf(out, "]");
    }
    std::fprintf(out, "],\"polyPatches\":[");
    for (size_t i = 0; i < scene.polyPatches.size(); i++)
      std::fprintf(out, "%s[%d,%d,%d]", i ? "," : "", scene.polyPatches[i].id,
                   scene.polyPatches[i].type, scene.polyPatches[i].flags);
    std::fprintf(out, "],\"surfacePatches\":[");
    for (size_t i = 0; i < scene.surfacePatches.size(); i++)
      std::fprintf(out, "%s[%lu,%lu,%lu]", i ? "," : "",
                   static_cast<unsigned long>(scene.surfacePatches[i].id),
                   static_cast<unsigned long>(scene.surfacePatches[i].hi),
                   static_cast<unsigned long>(scene.surfacePatches[i].lo));
    std::fprintf(out, "]}");
  }
  std::fprintf(out, "],\"pause\":[");
  for (size_t f = 0; f < kPauseFloorCount; f++) {
    std::fprintf(out, "%s[", f ? "," : "");
    for (size_t k = 0; k < kPauseMarksPerFloor; k++) {
      const PauseMarkPOD &m = data.pauseFloors[f][k];
      std::fprintf(out, "%s[%d,%ld,%s,%ld,%ld,[", k ? "," : "", m.markType,
                   static_cast<long>(m.unk04), m.hasVtx ? "true" : "false",
                   static_cast<long>(m.vtxCount), static_cast<long>(m.count));
      for (size_t p = 0; p < kPausePointsMax; p++)
        std::fprintf(out, "%s[%d,%lu,%lu]", p ? "," : "", m.points[p].chestFlag,
                     bits(m.points[p].x), bits(m.points[p].y));
      std::fprintf(out, "]]");
    }
    std::fprintf(out, "]");
  }
  std::fprintf(out, "]}\n");
  std::fclose(out);
}

} // namespace

int main(int argc, char **argv) {
  const Data *bundled = Bundled();
  if (argc == 3 && std::string(argv[1]) == "--dump-json") {
    if (!bundled) {
      std::fprintf(stderr, "bundled MQ data failed to decode: %s\n",
                   BundledError().c_str());
      return 1;
    }
    DumpJson(*bundled, argv[2]);
    return 0;
  }
  CheckEmbeddedMatchesVendoredFiles();
  CheckSceneMapping();
  CheckBundled();
  const Files files = Embedded();
  Data data;
  std::string error;
  assert(Decode(files.View(), data, error) && error.empty());
  CheckRooms(data);
  CheckScenes(data, files);
  CheckPauseMaps(data);
  CheckFailClosed(files);
  std::puts("PASS: bundled OoTMM v32.3 MQ bins decode with the pinned counts "
            "and fixes, and damaged copies are "
            "refused");
  return 0;
}
