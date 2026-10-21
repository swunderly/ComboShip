#pragma once
// ComboShip: the developer Master Quest walker, for the emulator walk of the 12 MQ dungeons and their Wallmaster rooms
// (ComboAndroid/DEVICE_TESTS.md, tools/mq_walk_driver.py). It is compiled only with the CMake option
// COMBO_MQ_WALK_DEV, which is OFF by default and never on in a release: ComboAndroid/tools/verify_apk.py refuses a
// library that carries kMarker or the walker's other strings, and ComboAndroid/tests/mq_walk_gating_tests.py checks
// that nothing outside this guard names the walker.
//
// - mq_walk loads every room of the current scene in turn through SoH's own room-load path, the one a door takes
//   (z_player.c, z_door_shutter.c): Room_RequestNewRoom, then Play_Update's func_800973FC runs the room's commands,
//   with the MQ override (ComboMqRuntime.cpp) and the room log (MqRoomLog.h), then Room_FinishRoomChange unloads the
//   previous room. Link stays where he stands. The room he started in is loaded last, so its actors are back after.
// - mq_tp loads one room the same way and stands Link at a position, on the first floor below it.
// - mq_wait waits until a scene has loaded, after SoH's developer warp (the console command "entrance <hex>").
// - The glue reads an adb command file (kCommandFile, in the app's data folder) and runs its lines on the console.
//
// The glue (soh/soh/Enhancements/randomizer/ComboMqWalk.cpp) implements Game on the PlayState. This header holds the
// state machine, the parsers and the log lines in plain C++17, so ComboAndroid/tests/mq_walk_tests.cpp runs them.
#if defined(COMBO_MQ_WALK_DEV)
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ComboMq::Walk {

// In every library built with the walker (the glue logs it once at start); verify_apk.py refuses it.
inline constexpr char kMarker[] = "ComboShip MQ walker dev build - not for release";
// Every line starts with this tag, after whatever prefix the log sink adds. The room log's is "[ComboShip MQ room]".
inline constexpr char kTag[] = "[ComboShip MQ walk]";
// In the app's data folder (Ship::Context::GetPathRelativeToAppDirectory), next to native.log. The glue renames the
// file to kTakenFile before it reads it, so a file written with a rename is read whole and once.
inline constexpr char kCommandFile[] = "mq-walk-commands.txt";
inline constexpr char kTakenFile[] = "mq-walk-commands.taken";
// The console commands the command file may run: the walker's, and SoH's developer warp (debugconsole.cpp).
inline constexpr std::array<std::string_view, 7> kFileCommands = { "mq_walk", "mq_tp",   "mq_wait", "mq_status",
                                                                   "mq_log",  "mq_stop", "entrance" };

inline constexpr int kPollFrames = 15;    // how often the glue looks for the command file
inline constexpr int kDefaultSettle = 20; // frames each walked room stays loaded
// A timer Wallmaster (En_Wallmas type 0) drops on Link 130 frames after it spawns while he stands, wherever he is; a
// walked room is unloaded well before that.
inline constexpr int kMaxSettle = 90;
inline constexpr int kLoadFrames = 300;   // a requested room must finish loading within this many frames
inline constexpr int kStableFrames = 900; // a cutscene, message, pause or transition must end within this many frames
inline constexpr int kDefaultWaitFrames = 1800;
inline constexpr int kWaitSettled = 10;     // mq_wait: frames the scene must stay stable
inline constexpr float kFloorProbe = 50.0f; // mq_tp looks for a floor from this far above the given height
inline constexpr float kCoordinateLimit = 32000.0f;
inline constexpr size_t kRememberedIds = 256;

// What the walker reads from the game once a frame.
struct Frame {
    bool inPlay = false;  // a PlayState in normal gameplay
    bool stable = false;  // no transition, pause, cutscene, message or other camera; one room loaded, none loading
    int scene = -1;       // play->sceneNum
    int room = -1;        // play->roomCtx.curRoom.num
    int rooms = 0;        // play->numRooms
    bool loading = false; // play->roomCtx.status != 0: a requested room's commands have not run yet
};

// Where mq_tp stood Link, and whether it found a floor below the position (if not, Link is at the given height).
struct Placement {
    float x = 0, y = 0, z = 0;
    bool floor = false;
};

class Game {
  public:
    virtual ~Game() = default;
    virtual Frame Read() = 0;
    // Room_RequestNewRoom; false if SoH refuses it.
    virtual bool RequestRoom(int room) = 0;
    // Room_FinishRoomChange: unloads the previous room's actors and spawns the new room's doors.
    virtual void FinishRoomChange() = 0;
    // Stands Link at x, z on the first floor below y + kFloorProbe, facing yaw if given.
    virtual Placement Place(float x, float y, float z, std::optional<int16_t> yaw) = 0;
    // The pause map's visited rooms of a scene (gSaveContext.sceneFlags[scene].rooms), which every room load marks.
    virtual uint32_t VisitedRooms(int scene) = 0;
    virtual void SetVisitedRooms(int scene, uint32_t rooms) = 0;
    // One line, after kTag.
    virtual void Log(const std::string& text) = 0;
};

inline std::string SceneHex(int scene) {
    char text[16];
    std::snprintf(text, sizeof(text), "0x%02x", static_cast<unsigned>(scene) & 0xffff);
    return text;
}

inline std::string Coordinate(float value) {
    char text[32];
    std::snprintf(text, sizeof(text), "%.1f", value);
    return text;
}

inline std::string Line(const std::string& id, const std::string& text) {
    return "id=" + id + " " + text;
}

// The command file's reply to each line it ran: the console's return code and the command's output on one line.
inline std::string ReplyLine(const std::string& id, int rc, const std::string& name, const std::string& output) {
    std::string flat;
    for (char c : output)
        flat += c == '\n' || c == '\r' ? ' ' : c;
    return Line(id, "rc=" + std::to_string(rc) + " cmd=" + name + (flat.empty() ? "" : " " + flat));
}

// What mq_status reports. The glue fills it from the PlayState.
struct Status {
    Frame frame;
    float x = 0, y = 0, z = 0;
    bool grounded = false;
    bool adult = false;
    int layer = 0;
    bool roomLog = false;
    const char* busy = "none";
};

inline std::string StatusText(const Status& status) {
    if (!status.frame.inPlay)
        return std::string("status play=0 busy=") + status.busy;
    const Frame& f = status.frame;
    return "status play=1 scene=" + SceneHex(f.scene) + " room=" + std::to_string(f.room) +
           " rooms=" + std::to_string(f.rooms) + " stable=" + (f.stable ? "1" : "0") + " busy=" + status.busy +
           " x=" + Coordinate(status.x) + " y=" + Coordinate(status.y) + " z=" + Coordinate(status.z) +
           " grounded=" + (status.grounded ? "1" : "0") + " age=" + (status.adult ? "adult" : "child") +
           " layer=" + std::to_string(status.layer) + " log=" + (status.roomLog ? "1" : "0");
}

// ---- Parsing ----------------------------------------------------------------------------------------------------

inline std::vector<std::string> Split(std::string_view text) {
    std::vector<std::string> words;
    size_t at = 0;
    while (at < text.size()) {
        while (at < text.size() && (text[at] == ' ' || text[at] == '\t' || text[at] == '\r'))
            at++;
        size_t end = at;
        while (end < text.size() && text[end] != ' ' && text[end] != '\t' && text[end] != '\r')
            end++;
        if (end > at)
            words.emplace_back(text.substr(at, end - at));
        at = end;
    }
    return words;
}

inline bool ValidId(std::string_view id) {
    if (id.empty() || id.size() > 40)
        return false;
    for (char c : id)
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' ||
              c == '.' || c == ':'))
            return false;
    return true;
}

inline bool FileCommand(std::string_view name) {
    for (std::string_view allowed : kFileCommands)
        if (name == allowed)
            return true;
    return false;
}

// One command-file line: "<id> <command> [arguments]". Nothing for a blank or "#" line; otherwise the command, or
// nothing with `error` set for a line the file may not run.
struct FileLine {
    std::string id;
    std::vector<std::string> args; // args[0] is the console command
    std::string Command() const {
        std::string text;
        for (const std::string& arg : args)
            text += (text.empty() ? "" : " ") + arg;
        return text;
    }
};

inline std::optional<FileLine> ParseFileLine(std::string_view text, std::string& error) {
    error.clear();
    std::vector<std::string> words = Split(text);
    if (words.empty() || words[0][0] == '#')
        return std::nullopt;
    if (!ValidId(words[0])) {
        error = "bad id";
        return std::nullopt;
    }
    if (words.size() < 2) {
        error = "no command";
        return std::nullopt;
    }
    if (!FileCommand(words[1])) {
        error = "refused " + words[1];
        return std::nullopt;
    }
    FileLine line;
    line.id = words[0];
    line.args.assign(words.begin() + 1, words.end());
    return line;
}

// A whole decimal (base 10) or hexadecimal (base 16, with or without 0x) number within [min, max].
inline std::optional<long> ParseInt(const std::string& text, int base, long min, long max) {
    if (text.empty() || text.size() > 12)
        return std::nullopt;
    const char* start = text.c_str();
    if (base == 16 && text.size() > 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X'))
        start += 2;
    if (*start == '\0' || *start == ' ' || *start == '+')
        return std::nullopt;
    char* end = nullptr;
    const long value = std::strtol(start, &end, base);
    if (end == start || *end != '\0' || value < min || value > max)
        return std::nullopt;
    return value;
}

inline std::optional<float> ParseFloat(const std::string& text) {
    if (text.empty() || text.size() > 16 || text[0] == ' ' || text[0] == '+')
        return std::nullopt;
    char* end = nullptr;
    const float value = std::strtof(text.c_str(), &end);
    if (end == text.c_str() || *end != '\0' || !std::isfinite(value) || std::fabs(value) > kCoordinateLimit)
        return std::nullopt;
    return value;
}

// mq_walk [settle]
inline std::optional<int> ParseWalk(const std::vector<std::string>& args) {
    if (args.size() == 1)
        return kDefaultSettle;
    if (args.size() != 2)
        return std::nullopt;
    const auto settle = ParseInt(args[1], 10, 1, kMaxSettle);
    return settle ? std::optional<int>(static_cast<int>(*settle)) : std::nullopt;
}

// mq_tp <room> <x> <y> <z> [yaw]: yaw in binary angle units (0x4000 is a quarter turn), decimal or 0x hex.
struct Teleport {
    int room = 0;
    float x = 0, y = 0, z = 0;
    std::optional<int16_t> yaw;
};

inline std::optional<Teleport> ParseTeleport(const std::vector<std::string>& args) {
    if (args.size() != 5 && args.size() != 6)
        return std::nullopt;
    const auto room = ParseInt(args[1], 10, 0, 63);
    const auto x = ParseFloat(args[2]), y = ParseFloat(args[3]), z = ParseFloat(args[4]);
    if (!room || !x || !y || !z)
        return std::nullopt;
    Teleport tp;
    tp.room = static_cast<int>(*room);
    tp.x = *x;
    tp.y = *y;
    tp.z = *z;
    if (args.size() == 6) {
        const bool hex = args[5].size() > 2 && args[5][0] == '0' && (args[5][1] == 'x' || args[5][1] == 'X');
        const auto yaw = ParseInt(args[5], hex ? 16 : 10, -32768, 65535);
        if (!yaw)
            return std::nullopt;
        tp.yaw = static_cast<int16_t>(static_cast<uint16_t>(*yaw & 0xffff));
    }
    return tp;
}

// mq_wait <scene, hex> [frames]
struct Wait {
    int scene = 0;
    int frames = kDefaultWaitFrames;
};

inline std::optional<Wait> ParseWait(const std::vector<std::string>& args) {
    if (args.size() != 2 && args.size() != 3)
        return std::nullopt;
    const auto scene = ParseInt(args[1], 16, 0, 0x6d);
    const auto frames = args.size() == 3 ? ParseInt(args[2], 10, 1, 36000) : std::optional<long>(kDefaultWaitFrames);
    if (!scene || !frames)
        return std::nullopt;
    return Wait{ static_cast<int>(*scene), static_cast<int>(*frames) };
}

// mq_log 0|1
inline std::optional<bool> ParseLog(const std::vector<std::string>& args) {
    if (args.size() != 2 || (args[1] != "0" && args[1] != "1"))
        return std::nullopt;
    return args[1] == "1";
}

// The ids the command file already ran, so a file the app could not delete never runs a line twice.
class IdMemory {
  public:
    // False if `id` already ran; otherwise remembers it.
    bool Remember(const std::string& id) {
        for (const std::string& seen : ids_)
            if (seen == id)
                return false;
        ids_.push_back(id);
        if (ids_.size() > kRememberedIds)
            ids_.pop_front();
        return true;
    }

  private:
    std::deque<std::string> ids_;
};

// ---- The state machine ------------------------------------------------------------------------------------------

class Walker {
  public:
    enum class Op { None, Walk, Teleport, Wait };

    Op Busy() const {
        return op_;
    }
    const char* BusyName() const {
        return op_ == Op::Walk ? "walk" : op_ == Op::Teleport ? "tp" : op_ == Op::Wait ? "wait" : "none";
    }

    // The walk and the teleport start once the game is stable, in the scene it is then in. Callers check Busy().
    void StartWalk(const std::string& id, int settle) {
        Reset(Op::Walk, id);
        settle_ = settle;
    }
    void StartTeleport(const std::string& id, const Teleport& tp) {
        Reset(Op::Teleport, id);
        tp_ = tp;
    }
    void StartWait(const std::string& id, const Wait& wait) {
        Reset(Op::Wait, id);
        wait_ = wait;
    }
    // mq_stop: a room still loading finishes its change first, then a walk goes back to the room Link stands in.
    void Stop(Game& game) {
        if (op_ == Op::Wait)
            Finish(game, "wait stopped scene=" + SceneHex(wait_.scene));
        else if (op_ != Op::None && phase_ == Phase::Loading)
            stopping_ = true;
        else if (op_ != Op::None)
            Abort(game, "stopped", true);
    }

    // Once a frame, after Play_Update.
    void Tick(Game& game) {
        if (op_ == Op::None)
            return;
        const Frame f = game.Read();
        elapsed_++;
        if (op_ == Op::Wait)
            TickWait(game, f);
        else
            TickRooms(game, f);
    }

  private:
    enum class Phase { Begin, Request, Loading, Settle };

    void Reset(Op op, const std::string& id) {
        op_ = op;
        id_ = id;
        phase_ = Phase::Begin;
        elapsed_ = phaseFrames_ = settled_ = countdown_ = 0;
        steps_.clear();
        step_ = loaded_ = 0;
        aborting_.clear();
        began_ = stopping_ = false;
    }

    void Finish(Game& game, const std::string& text) {
        op_ = Op::None;
        game.Log(Line(id_, text));
    }

    void TickWait(Game& game, const Frame& f) {
        settled_ = f.inPlay && f.stable && f.scene == wait_.scene ? settled_ + 1 : 0;
        if (settled_ >= kWaitSettled)
            Finish(game, "wait done scene=" + SceneHex(f.scene) + " room=" + std::to_string(f.room));
        else if (elapsed_ >= wait_.frames)
            Finish(game,
                   "wait timeout scene=" + SceneHex(wait_.scene) + " now=" + (f.inPlay ? SceneHex(f.scene) : "none"));
    }

    std::string Counts() const {
        return "scene=" + SceneHex(scene_) + " rooms=" + std::to_string(loaded_);
    }

    // Stops a walk or teleport. A walk still in its scene first loads the room Link stands in again (canReturn), then
    // reports the reason; a failure on the way back is added to it.
    void Abort(Game& game, const std::string& reason, bool canReturn) {
        if (op_ == Op::Walk && canReturn && aborting_.empty() && began_ && startRoom_ >= 0 && lastRoom_ != startRoom_) {
            aborting_ = reason;
            steps_ = { startRoom_ };
            step_ = 0;
            phase_ = Phase::Request;
            phaseFrames_ = 0;
            return;
        }
        End(game, aborting_.empty() ? reason : aborting_ + "+return-" + reason);
    }

    void End(Game& game, const std::string& reason) {
        if (op_ == Op::Teleport) {
            Finish(game,
                   "tp failed scene=" + SceneHex(scene_) + " room=" + std::to_string(tp_.room) + " reason=" + reason);
            return;
        }
        if (began_)
            game.SetVisitedRooms(scene_, visited_);
        Finish(game, "walk aborted " + Counts() + " reason=" + reason);
    }

    void Begin(Game& game, const Frame& f) {
        scene_ = f.scene;
        startRoom_ = lastRoom_ = f.room;
        if (op_ == Op::Teleport) {
            if (tp_.room >= f.rooms) {
                Finish(game, "tp failed scene=" + SceneHex(scene_) + " room=" + std::to_string(tp_.room) +
                                 " reason=no-room rooms=" + std::to_string(f.rooms));
                return;
            }
            if (f.room == tp_.room) {
                Place(game);
                return;
            }
            steps_ = { tp_.room };
        } else {
            began_ = true;
            visited_ = game.VisitedRooms(scene_);
            // Every other room in order, then the one Link stands in: requesting the current room would spawn its
            // actors a second time.
            for (int room = 0; room < f.rooms; room++)
                if (room != f.room)
                    steps_.push_back(room);
            if (f.room >= 0 && f.room < f.rooms && f.rooms > 1)
                steps_.push_back(f.room);
            game.Log(Line(id_, "walk start scene=" + SceneHex(scene_) + " rooms=" + std::to_string(f.rooms) +
                                   " start=" + std::to_string(f.room) + " settle=" + std::to_string(settle_)));
        }
        phase_ = Phase::Request;
        phaseFrames_ = 0;
        Next(game);
    }

    // After a room: the next one, or the end.
    void Next(Game& game) {
        if (step_ < steps_.size()) {
            phase_ = Phase::Request;
            phaseFrames_ = 0;
            return;
        }
        if (!aborting_.empty()) {
            End(game, aborting_);
            return;
        }
        game.SetVisitedRooms(scene_, visited_);
        Finish(game, "walk done " + Counts());
    }

    void Place(Game& game) {
        const Placement at = game.Place(tp_.x, tp_.y, tp_.z, tp_.yaw);
        Finish(game, "tp done scene=" + SceneHex(scene_) + " room=" + std::to_string(tp_.room) +
                         " x=" + Coordinate(at.x) + " y=" + Coordinate(at.y) + " z=" + Coordinate(at.z) +
                         " floor=" + (at.floor ? "found" : "none"));
    }

    void TickRooms(Game& game, const Frame& f) {
        if (phase_ == Phase::Begin) {
            if (!f.inPlay)
                Abort(game, "not-in-play", false);
            else if (f.stable)
                Begin(game, f);
            else if (elapsed_ > kStableFrames)
                Abort(game, "unstable", false);
            return;
        }
        if (!f.inPlay || f.scene != scene_) {
            Abort(game, f.inPlay ? "scene-changed" : "left-play", false);
            return;
        }
        const int room = steps_[step_];
        switch (phase_) {
            case Phase::Request:
                if (!f.stable) {
                    if (++phaseFrames_ > kStableFrames)
                        Abort(game, "unstable", true);
                    return;
                }
                if (!game.RequestRoom(room)) {
                    Abort(game, "request-refused room=" + std::to_string(room), true);
                    return;
                }
                if (op_ == Op::Walk)
                    game.Log(Line(id_, "walk room scene=" + SceneHex(scene_) + " room=" + std::to_string(room) +
                                           " step=" + std::to_string(step_ + 1) + "/" + std::to_string(steps_.size())));
                phase_ = Phase::Loading;
                phaseFrames_ = 0;
                return;
            case Phase::Loading:
                if (f.loading || f.room != room) {
                    if (++phaseFrames_ > kLoadFrames)
                        Abort(game, "load-timeout room=" + std::to_string(room), true);
                    return;
                }
                game.FinishRoomChange();
                lastRoom_ = room;
                step_++;
                if (stopping_) {
                    stopping_ = false;
                    Abort(game, "stopped", true);
                    return;
                }
                if (op_ == Op::Teleport) {
                    Place(game);
                    return;
                }
                if (aborting_.empty())
                    loaded_++;
                phase_ = Phase::Settle;
                countdown_ = settle_;
                return;
            case Phase::Settle:
                if (--countdown_ <= 0)
                    Next(game);
                return;
            case Phase::Begin:
                return;
        }
    }

    Op op_ = Op::None;
    std::string id_;
    Phase phase_ = Phase::Begin;
    int elapsed_ = 0, phaseFrames_ = 0, settled_ = 0, countdown_ = 0;
    int settle_ = kDefaultSettle;
    Teleport tp_;
    Wait wait_;
    int scene_ = -1, startRoom_ = -1, lastRoom_ = -1;
    std::vector<int> steps_;
    size_t step_ = 0;
    int loaded_ = 0;
    uint32_t visited_ = 0;
    bool began_ = false;
    bool stopping_ = false; // mq_stop came while a room was loading
    std::string aborting_;  // a walk going back to its first room before it reports this reason
};

} // namespace ComboMq::Walk
#endif
