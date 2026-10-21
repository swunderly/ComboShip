// combo/mq/MqWalk.h: the developer Master Quest walker's state machine, parsers
// and log lines, on a model of SoH's room loading (OTRRoom_RequestNewRoom sets
// curRoom and status 1; the next Play_Update's func_800973FC runs the room's
// commands, which spawn its actors, and clears status; Room_FinishRoomChange
// kills every actor outside the current room and marks it visited).
// Compiled with COMBO_MQ_WALK_DEV, as only the DEV build is.
//
// `--emit walk|tp|wait|status|reply ...` prints the lines the walker logs for
// one command, with the id "@", for mq_walk_driver_tests.py's fake device.
#include "mq/MqWalk.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <iterator>
#include <map>
#include <string>
#include <vector>

using namespace ComboMq::Walk;

namespace {

int gFailures = 0;

void Expect(bool condition, const std::string &what) {
  if (!condition) {
    std::fprintf(stderr, "FAIL: %s\n", what.c_str());
    gFailures++;
  }
}

void ExpectEq(const std::string &got, const std::string &want,
              const std::string &what) {
  Expect(got == want, what + "\n  got:  " + got + "\n  want: " + want);
}

// SoH's room loading as the walker sees it.
struct FakeGame final : Game {
  bool inPlay = true;
  bool cutscene = false; // a cutscene, message, pause or transition
  bool stuck = false;    // func_800973FC never finishes the load
  bool refuse = false;   // Room_RequestNewRoom refuses
  int scene = 0x03;
  int rooms = 23;
  int cur = 0;
  int prev = -1;
  int status = 0;
  uint32_t visited[0x70] = {};
  // room -> how many times its actors were spawned and not killed since
  std::map<int, int> alive;
  int maxAlive = 0; // most spawns of one room alive at once
  std::vector<int> requests, loads;
  int finishes = 0;
  int frame = 0;
  std::vector<int> finishFrames, requestFrames;
  std::vector<std::string> lines;
  Placement placed;
  std::optional<int16_t> placedYaw;
  bool floorBelow = true;
  int places = 0;

  Frame Read() override {
    Frame f;
    f.inPlay = inPlay;
    f.scene = scene;
    f.room = cur;
    f.rooms = rooms;
    f.loading = status != 0;
    f.stable = inPlay && !cutscene && status == 0 && prev < 0;
    return f;
  }
  bool RequestRoom(int room) override {
    Expect(room >= 0 && room < rooms, "requests only rooms the scene has");
    if (refuse || status != 0)
      return false;
    requests.push_back(room);
    requestFrames.push_back(frame);
    prev = cur;
    cur = room;
    status = 1;
    return true;
  }
  void FinishRoomChange() override {
    finishes++;
    finishFrames.push_back(frame);
    prev = -1;
    for (auto it = alive.begin(); it != alive.end();)
      it = it->first == cur ? std::next(it) : alive.erase(it);
    visited[scene] |= 1u << cur;
  }
  Placement Place(float x, float y, float z,
                  std::optional<int16_t> yaw) override {
    places++;
    placed = {x, floorBelow ? y - 7.5f : y, z, floorBelow};
    placedYaw = yaw;
    return placed;
  }
  uint32_t VisitedRooms(int s) override { return visited[s]; }
  void SetVisitedRooms(int s, uint32_t r) override { visited[s] = r; }
  void Log(const std::string &text) override { lines.push_back(text); }

  // Play_Update's func_800973FC, then the frame hook.
  void Engine() {
    frame++;
    if (status == 1 && !stuck) {
      status = 0;
      loads.push_back(cur);
      maxAlive = std::max(maxAlive, ++alive[cur]);
    }
  }
  void Enter(int scn, int nrooms, int room) {
    scene = scn;
    rooms = nrooms;
    cur = room;
    prev = -1;
    status = 0;
    alive = {{room, 1}};
    maxAlive = 1;
  }
};

int Run(FakeGame &game, Walker &walker, int limit = 100000) {
  int frames = 0;
  while (walker.Busy() != Walker::Op::None && frames < limit) {
    game.Engine();
    walker.Tick(game);
    frames++;
  }
  return frames;
}

std::string Last(const FakeGame &game) {
  return game.lines.empty() ? "" : game.lines.back();
}

void CheckWalk() {
  // Forest Temple from its entrance room: every other room in order, then the
  // entrance room again, each loaded once and its previous room unloaded.
  FakeGame game;
  game.Enter(0x03, 23, 0);
  game.visited[0x03] = 0x5;
  Walker walker;
  walker.StartWalk("w1-3", kDefaultSettle);
  Expect(walker.Busy() == Walker::Op::Walk, "a walk is busy");
  Expect(std::string(walker.BusyName()) == "walk", "busy name");
  Run(game, walker);
  std::vector<int> order;
  for (int room = 1; room < 23; room++)
    order.push_back(room);
  order.push_back(0);
  Expect(game.requests == order, "Forest: rooms 1-22, then 0");
  Expect(game.loads == order, "every requested room's commands ran once");
  Expect(game.finishes == 23, "every load unloads the previous room");
  Expect(game.maxAlive == 1, "no room's actors spawn twice");
  Expect(game.alive.size() == 1 && game.alive.count(0) == 1,
         "afterwards only the entrance room's actors are alive");
  Expect(game.cur == 0, "Link ends in the room he started in");
  Expect(game.visited[0x03] == 0x5,
         "the pause map's visited rooms are put back");
  ExpectEq(game.lines.front(),
           "id=w1-3 walk start scene=0x03 rooms=23 start=0 settle=20",
           "start line");
  ExpectEq(game.lines[1], "id=w1-3 walk room scene=0x03 room=1 step=1/23",
           "first room line");
  ExpectEq(game.lines[23], "id=w1-3 walk room scene=0x03 room=0 step=23/23",
           "last room line");
  ExpectEq(Last(game), "id=w1-3 walk done scene=0x03 rooms=23", "done line");
  Expect(game.lines.size() == 25, "start, 23 rooms, done");
  // Each room stays loaded `settle` frames, far below a timer Wallmaster's
  // 130-frame drop, and the next is requested only after that.
  for (size_t i = 0; i + 1 < game.requestFrames.size(); i++) {
    const int loaded = game.requestFrames[i + 1] - game.finishFrames[i];
    Expect(loaded >= kDefaultSettle && loaded <= kDefaultSettle + 2,
           "a walked room stays loaded settle frames (" +
               std::to_string(loaded) + ")");
  }
  Expect(kMaxSettle + 2 < 130,
         "the longest settle stays below En_Wallmas's drop timer");

  // From a middle room: the start room comes last.
  FakeGame well;
  well.Enter(0x08, 7, 5);
  Walker second;
  second.StartWalk("w1-4", 1);
  Run(well, second);
  Expect(well.requests == std::vector<int>({0, 1, 2, 3, 4, 6, 5}),
         "Bottom of the Well from room 5");
  ExpectEq(Last(well), "id=w1-4 walk done scene=0x08 rooms=7", "BotW done");

  // A scene of one room loads nothing: its room log line came with the scene.
  FakeGame one;
  one.Enter(0x0B, 1, 0);
  Walker third;
  third.StartWalk("w1-5", 20);
  Run(one, third);
  Expect(one.requests.empty(), "a one-room scene requests nothing");
  ExpectEq(Last(one), "id=w1-5 walk done scene=0x0b rooms=0", "one room done");
}

void CheckStability() {
  // The walk starts only once a cutscene ends, and pauses for one mid-walk.
  FakeGame game;
  game.Enter(0x06, 29, 3);
  game.cutscene = true;
  Walker walker;
  walker.StartWalk("s1", 5);
  for (int i = 0; i < 50; i++) {
    game.Engine();
    walker.Tick(game);
  }
  Expect(game.requests.empty() && game.lines.empty(),
         "nothing starts during a cutscene");
  game.cutscene = false;
  for (int i = 0; i < 30; i++) {
    game.Engine();
    walker.Tick(game);
  }
  const size_t before = game.requests.size();
  Expect(before > 0, "the walk starts after the cutscene");
  game.cutscene = true;
  for (int i = 0; i < 200; i++) {
    game.Engine();
    walker.Tick(game);
  }
  Expect(game.requests.size() == before,
         "no room is requested during a cutscene");
  game.cutscene = false;
  Run(game, walker);
  ExpectEq(Last(game), "id=s1 walk done scene=0x06 rooms=29",
           "the walk finishes after the cutscene");
  Expect(game.maxAlive == 1, "Spirit: no room spawns twice");

  // A cutscene that never ends: the walk goes back to the start room, which
  // needs a stable game too, and reports both.
  FakeGame stuck;
  stuck.Enter(0x06, 29, 3);
  Walker w2;
  w2.StartWalk("s2", 5);
  for (int i = 0; i < 40; i++) {
    stuck.Engine();
    w2.Tick(stuck);
  }
  stuck.cutscene = true;
  Run(stuck, w2);
  Expect(Last(stuck).find("id=s2 walk aborted scene=0x06 rooms=") == 0,
         "an endless cutscene aborts: " + Last(stuck));
  Expect(Last(stuck).find("reason=unstable+return-unstable") !=
             std::string::npos,
         "both reasons: " + Last(stuck));

  // Not stable at the start for too long: nothing loaded, nothing restored.
  FakeGame never;
  never.Enter(0x06, 29, 3);
  never.cutscene = true;
  Walker w3;
  w3.StartWalk("s3", 5);
  Run(never, w3);
  ExpectEq(Last(never),
           "id=s3 walk aborted scene=0xffff rooms=0 reason=unstable",
           "never stable");
  Expect(never.requests.empty(), "never stable: nothing requested");
}

void CheckAborts() {
  // Leaving the scene mid-walk (a void out, a Wallmaster, a death): no way
  // back, the visited rooms of the walked scene are put back.
  FakeGame game;
  game.Enter(0x04, 27, 0);
  game.visited[0x04] = 1;
  Walker walker;
  walker.StartWalk("a1", 10);
  for (int i = 0; i < 60; i++) {
    game.Engine();
    walker.Tick(game);
  }
  const size_t requested = game.requests.size();
  game.scene = 0x51;
  Run(game, walker);
  Expect(game.requests.size() == requested,
         "no request after the scene changed");
  Expect(Last(game).find("id=a1 walk aborted scene=0x04 rooms=") == 0 &&
             Last(game).find("reason=scene-changed") != std::string::npos,
         "scene change: " + Last(game));
  Expect(game.visited[0x04] == 1, "visited rooms put back after an abort");

  FakeGame left;
  left.Enter(0x04, 27, 0);
  Walker w2;
  w2.StartWalk("a2", 10);
  for (int i = 0; i < 30; i++) {
    left.Engine();
    w2.Tick(left);
  }
  left.inPlay = false;
  Run(left, w2);
  Expect(Last(left).find("reason=left-play") != std::string::npos,
         "left play: " + Last(left));

  // mq_stop: back to the start room, then the reason.
  FakeGame stop;
  stop.Enter(0x05, 23, 4);
  Walker w3;
  w3.StartWalk("a3", 10);
  for (int i = 0; i < 70; i++) {
    stop.Engine();
    w3.Tick(stop);
  }
  w3.Stop(stop);
  Run(stop, w3);
  Expect(stop.requests.back() == 4 && stop.cur == 4,
         "a stopped walk loads the start room again");
  Expect(stop.maxAlive == 1, "the way back spawns no room twice");
  Expect(Last(stop).find("id=a3 walk aborted scene=0x05 rooms=") == 0 &&
             Last(stop).find("reason=stopped") != std::string::npos,
         "stopped: " + Last(stop));

  // mq_stop while a room is loading: its change finishes, then the way back.
  FakeGame mid;
  mid.Enter(0x05, 23, 4);
  Walker w7;
  w7.StartWalk("a7", 10);
  while (!(mid.requests.size() == 3 && mid.status == 1)) {
    mid.Engine();
    w7.Tick(mid);
  }
  w7.Stop(mid);
  Expect(w7.Busy() == Walker::Op::Walk, "a stop mid-load waits for the load");
  Run(mid, w7);
  Expect(mid.requests == std::vector<int>({0, 1, 2, 4}),
         "the loading room finishes, then the start room");
  Expect(mid.finishes == 4 && mid.prev == -1 && mid.cur == 4,
         "every change finished");
  Expect(Last(mid).find("reason=stopped") != std::string::npos &&
             Last(mid).find("return") == std::string::npos,
         "stopped mid-load: " + Last(mid));

  // A load that never finishes.
  FakeGame load;
  load.Enter(0x07, 23, 0);
  Walker w4;
  w4.StartWalk("a4", 10);
  for (int i = 0; i < 40; i++) {
    load.Engine();
    w4.Tick(load);
  }
  load.stuck = true;
  Run(load, w4);
  Expect(Last(load).find("reason=load-timeout room=") != std::string::npos &&
             Last(load).find("+return-unstable") != std::string::npos,
         "load timeout: " + Last(load));

  // SoH refuses the request.
  FakeGame refused;
  refused.Enter(0x07, 23, 0);
  refused.refuse = true;
  Walker w5;
  w5.StartWalk("a5", 10);
  Run(refused, w5);
  ExpectEq(
      Last(refused),
      "id=a5 walk aborted scene=0x07 rooms=0 reason=request-refused room=1",
      "refused at the first room");

  // Not in play at all.
  FakeGame menu;
  menu.inPlay = false;
  Walker w6;
  w6.StartWalk("a6", 10);
  Run(menu, w6);
  ExpectEq(Last(menu),
           "id=a6 walk aborted scene=0xffff rooms=0 reason=not-in-play",
           "not in play");
}

void CheckTeleport() {
  // Spirit Temple room 15 from room 0: one room load, then Link stands there.
  FakeGame game;
  game.Enter(0x06, 29, 0);
  Walker walker;
  Teleport tp;
  tp.room = 15;
  tp.x = 1152;
  tp.y = 480;
  tp.z = -874;
  walker.StartTeleport("t1", tp);
  Expect(std::string(walker.BusyName()) == "tp", "teleport busy name");
  Run(game, walker);
  Expect(game.requests == std::vector<int>({15}), "one room requested");
  Expect(game.finishes == 1 && game.cur == 15, "the room is current");
  Expect(game.places == 1 && !game.placedYaw, "Link placed once, yaw kept");
  ExpectEq(
      Last(game),
      "id=t1 tp done scene=0x06 room=15 x=1152.0 y=472.5 z=-874.0 floor=found",
      "teleport line");

  // Already in the room: no load.
  FakeGame here;
  here.Enter(0x0D, 20, 18);
  here.floorBelow = false;
  Walker w2;
  tp.room = 18;
  tp.x = -1201;
  tp.y = 150;
  tp.z = 1237;
  tp.yaw = int16_t(0x4000);
  w2.StartTeleport("t2", tp);
  Run(here, w2);
  Expect(here.requests.empty() && here.places == 1, "same room: no load");
  Expect(here.placedYaw && *here.placedYaw == 0x4000, "yaw passed on");
  ExpectEq(
      Last(here),
      "id=t2 tp done scene=0x0d room=18 x=-1201.0 y=150.0 z=1237.0 floor=none",
      "no floor");

  FakeGame small;
  small.Enter(0x08, 7, 0);
  Walker w3;
  tp.room = 9;
  w3.StartTeleport("t3", tp);
  Run(small, w3);
  ExpectEq(Last(small),
           "id=t3 tp failed scene=0x08 room=9 reason=no-room rooms=7",
           "a room the scene lacks");
  Expect(small.requests.empty(), "no request for a missing room");

  FakeGame gone;
  gone.Enter(0x0B, 11, 0);
  gone.stuck = true;
  Walker w4;
  tp.room = 2;
  w4.StartTeleport("t4", tp);
  gone.Engine();
  w4.Tick(gone);
  gone.Engine();
  w4.Tick(gone);
  gone.scene = 0x51;
  Run(gone, w4);
  ExpectEq(Last(gone), "id=t4 tp failed scene=0x0b room=2 reason=scene-changed",
           "scene change during a teleport");
}

void CheckWait() {
  FakeGame game;
  game.Enter(0x51, 1, 0);
  Walker walker;
  walker.StartWait("q1", Wait{0x03, 600});
  for (int i = 0; i < 20; i++) {
    game.Engine();
    walker.Tick(game);
  }
  Expect(walker.Busy() == Walker::Op::Wait, "waits while in another scene");
  game.Enter(0x03, 23, 0);
  game.cutscene = true;
  for (int i = 0; i < 20; i++) {
    game.Engine();
    walker.Tick(game);
  }
  Expect(walker.Busy() == Walker::Op::Wait, "waits through the title card");
  game.cutscene = false;
  const int frames = Run(game, walker);
  Expect(frames == kWaitSettled, "done after 10 stable frames");
  ExpectEq(Last(game), "id=q1 wait done scene=0x03 room=0", "wait done");

  Walker w2;
  w2.StartWait("q2", Wait{0x04, 30});
  const int timeout = Run(game, w2);
  Expect(timeout == 30, "times out after its frames");
  ExpectEq(Last(game), "id=q2 wait timeout scene=0x04 now=0x03",
           "wait timeout");

  Walker w3;
  w3.StartWait("q3", Wait{0x04, 30});
  w3.Stop(game);
  ExpectEq(Last(game), "id=q3 wait stopped scene=0x04", "wait stopped");
  Expect(w3.Busy() == Walker::Op::None, "stopped");
}

void CheckParsers() {
  std::string error;
  auto line = ParseFileLine("w1-0 mq_status", error);
  Expect(line && line->id == "w1-0" &&
             line->args == std::vector<std::string>({"mq_status"}) &&
             error.empty(),
         "a status line");
  line = ParseFileLine("  w1-1\tmq_tp  15 1152 480 -874\r", error);
  Expect(line && line->Command() == "mq_tp 15 1152 480 -874",
         "spaces, tabs and CR are separators");
  line = ParseFileLine("w1-2 entrance 169", error);
  Expect(line && line->Command() == "entrance 169",
         "SoH's developer warp is allowed");
  Expect(!ParseFileLine("", error) && error.empty(), "a blank line is skipped");
  Expect(!ParseFileLine("   # comment", error) && error.empty(),
         "a comment is skipped");
  Expect(!ParseFileLine("bad/id mq_status", error) && error == "bad id",
         "an id with a slash");
  Expect(!ParseFileLine(std::string(41, 'a') + " mq_status", error) &&
             error == "bad id",
         "an id too long");
  Expect(!ParseFileLine("w1-3", error) && error == "no command", "an id alone");
  for (const char *other : {"save_state", "load_state", "give_item", "set",
                            "reset", "pos", "kill", "file_select"}) {
    Expect(!ParseFileLine(std::string("w1-4 ") + other + " 1", error) &&
               error == std::string("refused ") + other,
           std::string("the file may not run ") + other);
  }
  Expect(kFileCommands.size() == 7, "seven commands");
  for (const char *name : {"mq_walk", "mq_tp", "mq_wait", "mq_status", "mq_log",
                           "mq_stop", "entrance"})
    Expect(FileCommand(name), std::string("allowed: ") + name);

  Expect(ParseWalk({"mq_walk"}) == kDefaultSettle, "default settle");
  Expect(ParseWalk({"mq_walk", "1"}) == 1 && ParseWalk({"mq_walk", "90"}) == 90,
         "settle bounds");
  Expect(!ParseWalk({"mq_walk", "0"}) && !ParseWalk({"mq_walk", "91"}) &&
             !ParseWalk({"mq_walk", "x"}) &&
             !ParseWalk({"mq_walk", "20", "1"}) &&
             !ParseWalk({"mq_walk", "+5"}),
         "bad settles");

  auto tp = ParseTeleport({"mq_tp", "19", "-1763", "1467", "-3318"});
  Expect(tp && tp->room == 19 && tp->x == -1763 && tp->y == 1467 &&
             tp->z == -3318 && !tp->yaw,
         "a teleport");
  tp = ParseTeleport({"mq_tp", "2", "-1579.5", "193", "-1547", "16384"});
  Expect(tp && tp->x == -1579.5f && tp->yaw && *tp->yaw == 0x4000,
         "decimal yaw");
  tp = ParseTeleport({"mq_tp", "2", "0", "0", "0", "0xc000"});
  Expect(tp && tp->yaw && *tp->yaw == -0x4000, "hex yaw");
  tp = ParseTeleport({"mq_tp", "2", "0", "0", "0", "-16384"});
  Expect(tp && tp->yaw && *tp->yaw == -16384, "negative yaw");
  for (const std::vector<std::string> &bad :
       std::vector<std::vector<std::string>>{
           {"mq_tp"},
           {"mq_tp", "1", "2", "3"},
           {"mq_tp", "64", "0", "0", "0"},
           {"mq_tp", "-1", "0", "0", "0"},
           {"mq_tp", "1", "40000", "0", "0"},
           {"mq_tp", "1", "nan", "0", "0"},
           {"mq_tp", "1", "inf", "0", "0"},
           {"mq_tp", "1", "12abc", "0", "0"},
           {"mq_tp", "1", "1e9", "0", "0"},
           {"mq_tp", "1", "0", "0", "0", "70000"},
           {"mq_tp", "1", "0", "0", "0", "0", "0"}})
    Expect(!ParseTeleport(bad),
           "refused teleport with " + std::to_string(bad.size()) + " words");

  auto wait = ParseWait({"mq_wait", "03"});
  Expect(wait && wait->scene == 3 && wait->frames == kDefaultWaitFrames,
         "wait 03");
  wait = ParseWait({"mq_wait", "0x0d", "600"});
  Expect(wait && wait->scene == 0x0d && wait->frames == 600, "wait 0x0d 600");
  Expect(!ParseWait({"mq_wait"}) && !ParseWait({"mq_wait", "zz"}) &&
             !ParseWait({"mq_wait", "6e"}) && !ParseWait({"mq_wait", "3", "0"}),
         "bad waits");
  Expect(ParseLog({"mq_log", "1"}) == true &&
             ParseLog({"mq_log", "0"}) == false && !ParseLog({"mq_log", "2"}) &&
             !ParseLog({"mq_log"}),
         "mq_log");

  IdMemory ids;
  Expect(ids.Remember("a") && !ids.Remember("a") && ids.Remember("b"),
         "an id runs once");
  for (int i = 0; i < int(kRememberedIds); i++)
    ids.Remember("x" + std::to_string(i));
  Expect(ids.Remember("a"), "the oldest ids are forgotten");
}

void CheckLines() {
  Expect(std::string(kMarker) ==
             "ComboShip MQ walker dev build - not for release",
         "marker");
  Expect(std::string(kTag) == "[ComboShip MQ walk]", "tag");
  Expect(std::string(kTag) != "[ComboShip MQ room]",
         "the room log has its own tag");
  Expect(std::string(kCommandFile) == "mq-walk-commands.txt" &&
             std::string(kTakenFile) == "mq-walk-commands.taken",
         "command files");
  ExpectEq(ReplyLine("w1-2", 0, "entrance", ""), "id=w1-2 rc=0 cmd=entrance",
           "empty output");
  ExpectEq(ReplyLine("w1-3", 1, "mq_walk", "refused busy\nwalk"),
           "id=w1-3 rc=1 cmd=mq_walk refused busy walk", "output on one line");
  Status status;
  status.busy = "walk";
  ExpectEq(StatusText(status), "status play=0 busy=walk", "status out of play");
  status.frame = {true, true, 0x06, 15, 29, false};
  status.x = 1152;
  status.y = 480;
  status.z = -874.25f;
  status.grounded = true;
  status.adult = true;
  status.layer = 2;
  status.roomLog = true;
  status.busy = "none";
  ExpectEq(StatusText(status),
           "status play=1 scene=0x06 room=15 rooms=29 stable=1 busy=none "
           "x=1152.0 y=480.0 z=-874.2 grounded=1 "
           "age=adult layer=2 log=1",
           "status in play");
  ExpectEq(SceneHex(0x0d), "0x0d", "scene hex");
}

int Emit(int argc, char **argv) {
  const std::string what = argc > 2 ? argv[2] : "";
  auto arg = [&](int i) { return i < argc ? std::atoi(argv[i]) : 0; };
  FakeGame game;
  Walker walker;
  if (what == "walk") { // walk <scene> <rooms> <start room>
    game.Enter(arg(3), arg(4), arg(5));
    walker.StartWalk("@", kDefaultSettle);
  } else if (what ==
             "tp") { // tp <scene> <rooms> <current room> <room> <x> <y> <z>
    game.Enter(arg(3), arg(4), arg(5));
    Teleport tp;
    tp.room = arg(6);
    tp.x = float(arg(7));
    tp.y = float(arg(8));
    tp.z = float(arg(9));
    walker.StartTeleport("@", tp);
  } else if (what == "wait") { // wait <scene>
    game.Enter(arg(3), 1, 0);
    walker.StartWait("@", Wait{arg(3), 100});
  } else if (what == "status") { // status <scene> <room> <rooms>
    Status status;
    status.frame = {true, true, arg(3), arg(4), arg(5), false};
    status.grounded = true;
    status.roomLog = true;
    std::printf("%s\n", Line("@", StatusText(status)).c_str());
    return 0;
  } else if (what == "reply") { // reply <rc> <name> [output words]
    std::string output;
    for (int i = 5; i < argc; i++)
      output += (output.empty() ? "" : " ") + std::string(argv[i]);
    std::printf(
        "%s\n",
        ReplyLine("@", arg(3), argc > 4 ? argv[4] : "", output).c_str());
    return 0;
  } else {
    std::fprintf(stderr, "usage: --emit walk|tp|wait|status|reply ...\n");
    return 2;
  }
  Run(game, walker);
  for (const std::string &line : game.lines)
    std::printf("%s\n", line.c_str());
  return 0;
}

} // namespace

int main(int argc, char **argv) {
  if (argc > 1 && std::string(argv[1]) == "--emit")
    return Emit(argc, argv);
  CheckWalk();
  CheckStability();
  CheckAborts();
  CheckTeleport();
  CheckWait();
  CheckParsers();
  CheckLines();
  if (gFailures) {
    std::fprintf(stderr, "%d failures\n", gFailures);
    return 1;
  }
  std::printf("MQ walker state machine, parsers and lines passed.\n");
  return 0;
}
