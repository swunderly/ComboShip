#pragma once

// Whether a Save Editor edit reaches its game's save, for both Save Editors (OoT
// soh/soh/Enhancements/debugger/debugSaveEditor.cpp, MM mm/2s2h/DeveloperTools/SaveEditor.cpp) and the launcher
// (combo/ComboShip.cpp Combo_DormantSaveEditSlot). No ImGui and no game types, so the host tests run it as is.
//
// Only one game's play loop runs at a time. The other game's save stays in memory, dormant, and that game reloads its
// save from the save container when it next starts (both games' title_setup.c), so an edit made only in memory is
// lost on the switch. The menu still shows the dormant game's Save Editor: there an edit is written to the file at
// once, with the writer the cross-game item grants use (MM SaveManager_SaveCurrentForCombo, OoT
// SaveManager::SaveFile), but only while the launcher vouches that the save in memory is the file being played.
namespace ComboSaveEditorAccess {

inline constexpr int kOot = 0; // ComboRando::GameId (rando/CrossForeign.h)
inline constexpr int kMm = 1;

constexpr bool IsFile(int slot) {
    return slot >= 0 && slot <= 2;
}

// The launcher's rule: the file (0-2) whose dormant save `game` holds and the Save Editor may edit and write now,
// else -1. foreground: the game whose play loop runs. combined: the file plays both games (Combined mode); in a
// one-game mode the other game has no save to edit. ootPlaying: the file OoT is playing, -1 on its title and file
// screens (SOH_GetPlayingFileNum). mmPlaying: the file MM runs while it is in front (g_PendingMMFileNum). mmResident:
// the file whose current MM save is MM's dormant save (g_MmSaveInMemorySlot); -1 when MM's memory is not a file's
// current save, as after a reset or an owl-save quit in MM, a failed load, an erase or a copy. OoT's dormant save is
// the file MM runs: OoT saves it before every switch to MM, or has just loaded it when the file was last saved in MM,
// and the dormant grants write it at once.
constexpr int DormantSlot(int game, int foreground, bool combined, int ootPlaying, int mmPlaying, int mmResident) {
    if (!combined || game == foreground)
        return -1;
    if (game == kMm)
        return foreground == kOot && IsFile(mmResident) && mmResident == ootPlaying ? mmResident : -1;
    if (game == kOot)
        return foreground == kMm && IsFile(mmPlaying) ? mmPlaying : -1;
    return -1;
}

enum class Access {
    NoFile,  // nothing to edit: the editor says so and draws no tabs
    Live,    // the editor's game is running: edits go to the live save, which the game saves itself
    Dormant, // the editor's game is not running: edits go to the save in memory and are written to the file at once
};

// An editor's access this frame. foreground: the editor's game is running. hasFile: the editor's own test for a loaded
// file (OoT: not on the title or file screen; MM: a randomizer save in file 1-3). fileNum: that game's loaded file.
// dormantSlot: the launcher's DormantSlot for that game.
constexpr Access Decide(bool foreground, bool hasFile, int fileNum, int dormantSlot) {
    if (!hasFile)
        return Access::NoFile;
    if (foreground)
        return Access::Live;
    return IsFile(dormantSlot) && fileNum == dormantSlot ? Access::Dormant : Access::NoFile;
}

// When to write a dormant edit: once per frame, after the editor drew. A change waits while a control is active (a
// slider being dragged, a text field being typed into), so a drag writes the file once, when it is let go. Any other
// access drops a waiting change: a running game saves its own file, and no file is being edited otherwise.
class DormantWrite {
  public:
    bool Step(Access access, bool changed, bool controlActive) {
        if (access != Access::Dormant) {
            mPending = false;
            return false;
        }
        mPending = mPending || changed;
        if (!mPending || controlActive)
            return false;
        mPending = false;
        return true;
    }
    bool Pending() const {
        return mPending;
    }

  private:
    bool mPending = false;
};

// Wording. The first line of a dormant editor, and what a control that needs its game running says instead.
inline constexpr const char* kDormantOot = "Ocarina of Time isn't running. Changes here are saved to its file at once.";
inline constexpr const char* kDormantMm = "Majora's Mask isn't running. Changes here are saved to its file at once.";
inline constexpr const char* kSwitchToOot = "Switch to Ocarina of Time to edit this.";
inline constexpr const char* kSwitchToMm = "Switch to Majora's Mask to edit this.";

} // namespace ComboSaveEditorAccess
