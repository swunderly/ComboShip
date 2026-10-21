#if defined(COMBO_BUILD) && defined(COMBO_MQ_WALK_DEV)
// ComboShip: the developer Master Quest walker on the PlayState (combo/mq/MqWalk.h holds the state machine). Never in
// a release: COMBO_MQ_WALK_DEV is OFF by default and verify_apk.py refuses its marker.
//
// Nothing here saves. Its room loads do what any does: clear the temporary clear and switch flags, add an entry to the
// play statistics (gSaveContext.ship.stats), mark the room visited on the pause map and record a check's item the
// camera sees; the walk puts the scene's visited rooms (gSaveContext.sceneFlags[scene].rooms) back when it ends.
// mq_log sets the CVar gCombo.Debug.MqRoomLog for this session, without saving the configuration.
#include "ComboMqWalk.h"
#include "mq/MqRoomLog.h"
#include "mq/MqWalk.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <ship/Context.h>
#include <spdlog/spdlog.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

#include "global.h"

extern "C" PlayState* gPlayState;

namespace {

namespace Walk = ComboMq::Walk;

class PlayGame final : public Walk::Game {
  public:
    Walk::Frame Read() override {
        Walk::Frame f;
        PlayState* play = gPlayState;
        if (play == nullptr || gSaveContext.gameMode != GAMEMODE_NORMAL || GET_PLAYER(play) == nullptr)
            return f;
        f.inPlay = true;
        f.scene = play->sceneNum;
        f.room = play->roomCtx.curRoom.num;
        f.rooms = play->numRooms;
        f.loading = play->roomCtx.status != 0;
        f.stable = !f.loading && play->roomCtx.prevRoom.num < 0 && play->transitionTrigger == TRANS_TRIGGER_OFF &&
                   play->transitionMode == TRANS_MODE_OFF && play->pauseCtx.state == 0 &&
                   play->csCtx.state == CS_STATE_IDLE && play->msgCtx.msgMode == MSGMODE_NONE &&
                   play->activeCamera == CAM_ID_MAIN;
        return f;
    }
    bool RequestRoom(int room) override {
        return Room_RequestNewRoom(gPlayState, &gPlayState->roomCtx, room) != 0;
    }
    void FinishRoomChange() override {
        Room_FinishRoomChange(gPlayState, &gPlayState->roomCtx);
    }
    Walk::Placement Place(float x, float y, float z, std::optional<int16_t> yaw) override {
        Player* player = GET_PLAYER(gPlayState);
        Vec3f probe = { x, y + Walk::kFloorProbe, z };
        CollisionPoly* poly = nullptr;
        s32 bgId = 0;
        const f32 floor = BgCheck_EntityRaycastFloor3(&gPlayState->colCtx, &poly, &bgId, &probe);
        const bool found = poly != nullptr && floor > BGCHECK_Y_MIN;
        const Vec3f at = { x, found ? floor : y, z };
        player->actor.world.pos = at;
        player->actor.prevPos = at;
        player->actor.velocity = { 0.0f, 0.0f, 0.0f };
        player->actor.speedXZ = 0.0f;
        player->linearVelocity = 0.0f;
        if (yaw) {
            player->actor.shape.rot.y = *yaw;
            player->actor.world.rot.y = *yaw;
            player->yaw = *yaw;
        }
        return { at.x, at.y, at.z, found };
    }
    uint32_t VisitedRooms(int scene) override {
        return scene >= 0 && scene < ARRAY_COUNT(gSaveContext.sceneFlags) ? gSaveContext.sceneFlags[scene].rooms : 0;
    }
    void SetVisitedRooms(int scene, uint32_t rooms) override {
        if (scene >= 0 && scene < ARRAY_COUNT(gSaveContext.sceneFlags))
            gSaveContext.sceneFlags[scene].rooms = rooms;
    }
    void Log(const std::string& text) override {
        SPDLOG_INFO("{} {}", Walk::kTag, text);
    }
};

Walk::Walker& TheWalker() {
    static Walk::Walker walker;
    return walker;
}

// The id of the command-file line running now; "console" for a command typed in the console window.
std::string& CurrentId() {
    static std::string id = "console";
    return id;
}

int32_t Reply(std::string* output, int32_t rc, const std::string& text) {
    PlayGame().Log(Walk::Line(CurrentId(), text));
    if (output != nullptr)
        *output = text;
    return rc;
}

int32_t Refuse(std::string* output, const std::string& why) {
    return Reply(output, 1, "refused " + why);
}

const char* BusyOrNull() {
    return TheWalker().Busy() == Walk::Walker::Op::None ? nullptr : TheWalker().BusyName();
}

int32_t StatusCommand(std::shared_ptr<Ship::Console>, std::vector<std::string> args, std::string* output) {
    if (args.size() != 1)
        return Refuse(output, "usage: mq_status");
    Walk::Status status;
    status.frame = PlayGame().Read();
    status.busy = TheWalker().BusyName();
    status.roomLog = CVarGetInteger(ComboMq::kRoomLogCVar, 0) != 0;
    if (status.frame.inPlay) {
        const Player* player = GET_PLAYER(gPlayState);
        status.x = player->actor.world.pos.x;
        status.y = player->actor.world.pos.y;
        status.z = player->actor.world.pos.z;
        status.grounded = (player->actor.bgCheckFlags & BGCHECKFLAG_GROUND) != 0;
        status.adult = LINK_IS_ADULT;
        status.layer = gSaveContext.sceneLayer;
    }
    return Reply(output, 0, Walk::StatusText(status));
}

int32_t LogCommand(std::shared_ptr<Ship::Console>, std::vector<std::string> args, std::string* output) {
    const auto on = Walk::ParseLog(args);
    if (!on)
        return Refuse(output, "usage: mq_log 0|1");
    // For this session; nothing here saves the configuration.
    CVarSetInteger(ComboMq::kRoomLogCVar, *on ? 1 : 0);
    return Reply(output, 0, std::string("log room=") + (*on ? "1" : "0"));
}

int32_t WalkCommand(std::shared_ptr<Ship::Console>, std::vector<std::string> args, std::string* output) {
    const auto settle = Walk::ParseWalk(args);
    if (!settle)
        return Refuse(output, "usage: mq_walk [settle frames 1-" + std::to_string(Walk::kMaxSettle) + "]");
    if (BusyOrNull())
        return Refuse(output, std::string("busy ") + BusyOrNull());
    if (!PlayGame().Read().inPlay)
        return Refuse(output, "not in play");
    if (!CVarGetInteger(ComboMq::kRoomLogCVar, 0))
        return Refuse(output, "room log off (mq_log 1)");
    TheWalker().StartWalk(CurrentId(), *settle);
    return Reply(output, 0, "walk queued settle=" + std::to_string(*settle));
}

int32_t TeleportCommand(std::shared_ptr<Ship::Console>, std::vector<std::string> args, std::string* output) {
    const auto tp = Walk::ParseTeleport(args);
    if (!tp)
        return Refuse(output, "usage: mq_tp <room> <x> <y> <z> [yaw]");
    if (BusyOrNull())
        return Refuse(output, std::string("busy ") + BusyOrNull());
    if (!PlayGame().Read().inPlay)
        return Refuse(output, "not in play");
    TheWalker().StartTeleport(CurrentId(), *tp);
    return Reply(output, 0, "tp queued room=" + std::to_string(tp->room));
}

int32_t WaitCommand(std::shared_ptr<Ship::Console>, std::vector<std::string> args, std::string* output) {
    const auto wait = Walk::ParseWait(args);
    if (!wait)
        return Refuse(output, "usage: mq_wait <scene hex> [frames]");
    if (BusyOrNull())
        return Refuse(output, std::string("busy ") + BusyOrNull());
    TheWalker().StartWait(CurrentId(), *wait);
    return Reply(output, 0, "wait queued scene=" + Walk::SceneHex(wait->scene));
}

int32_t StopCommand(std::shared_ptr<Ship::Console>, std::vector<std::string> args, std::string* output) {
    if (args.size() != 1)
        return Refuse(output, "usage: mq_stop");
    PlayGame game;
    TheWalker().Stop(game);
    return Reply(output, 0, "stop");
}

// Runs one line of the command file on the console, with its id on every line it logs.
void RunFileLine(const std::string& text) {
    static Walk::IdMemory ran;
    std::string error;
    const auto line = Walk::ParseFileLine(text, error);
    PlayGame game;
    if (!line) {
        if (!error.empty())
            game.Log("refused line: " + error);
        return;
    }
    if (!ran.Remember(line->id))
        return;
    const auto console = Ship::Context::GetRawInstance()->GetConsole();
    if (!console->HasCommand(line->args[0])) {
        game.Log(Walk::ReplyLine(line->id, 1, line->args[0], "unknown command"));
        return;
    }
    std::string output;
    CurrentId() = line->id;
    const int32_t rc = console->Run(line->Command(), &output);
    CurrentId() = "console";
    game.Log(Walk::ReplyLine(line->id, rc, line->args[0], output));
}

void PollCommandFile() {
    namespace fs = std::filesystem;
    const fs::path file = Ship::Context::GetPathRelativeToAppDirectory(Walk::kCommandFile);
    std::error_code ec;
    if (!fs::exists(file, ec))
        return;
    // Take the file first, so a file the driver writes next is never half-read or deleted unread.
    const fs::path taken = Ship::Context::GetPathRelativeToAppDirectory(Walk::kTakenFile);
    fs::rename(file, taken, ec);
    const fs::path read = ec ? file : taken;
    std::vector<std::string> lines;
    {
        std::ifstream in(read);
        for (std::string line; std::getline(in, line);)
            lines.push_back(line);
    }
    fs::remove(read, ec);
    if (ec) {
        static bool warned = false;
        if (!warned)
            PlayGame().Log("cannot remove " + read.string() + ": " + ec.message() + "; each id runs once");
        warned = true;
    }
    for (const std::string& line : lines)
        RunFileLine(line);
}

void OnFrame() {
    static uint32_t frames = 0;
    PlayGame game;
    TheWalker().Tick(game);
    if (++frames % Walk::kPollFrames == 0)
        PollCommandFile();
}

} // namespace

void ComboMqWalk_Init(void (*addCommand)(const std::string& name, Ship::CommandEntry entry)) {
    addCommand("mq_status", { StatusCommand, "DEV: report the scene, room, Link and the Master Quest walker." });
    addCommand("mq_log", { LogCommand,
                           "DEV: turn the Master Quest room log on or off for this session.",
                           { { "0|1", Ship::ArgumentType::NUMBER } } });
    addCommand("mq_walk", { WalkCommand,
                            "DEV: load every room of this scene in turn; logs 'walk done scene=X rooms=N'.",
                            { { "settle frames", Ship::ArgumentType::NUMBER, true } } });
    addCommand("mq_tp", { TeleportCommand,
                          "DEV: load a room and stand Link on the floor below a position.",
                          { { "room", Ship::ArgumentType::NUMBER },
                            { "x", Ship::ArgumentType::NUMBER },
                            { "y", Ship::ArgumentType::NUMBER },
                            { "z", Ship::ArgumentType::NUMBER },
                            { "yaw", Ship::ArgumentType::NUMBER, true } } });
    addCommand("mq_wait",
               { WaitCommand,
                 "DEV: wait until a scene (hex) has loaded and is stable.",
                 { { "scene", Ship::ArgumentType::TEXT }, { "frames", Ship::ArgumentType::NUMBER, true } } });
    addCommand("mq_stop", { StopCommand, "DEV: stop the walk, teleport or wait." });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameFrameUpdate>(OnFrame);
    SPDLOG_INFO("{} ready: {}; command file {}", Walk::kTag, Walk::kMarker,
                Ship::Context::GetPathRelativeToAppDirectory(Walk::kCommandFile));
}
#endif
