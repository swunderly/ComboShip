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
// current save, as after a reset or an owl-save quit in MM, a failed load, an erase or a copy. ootResident: the file
// whose OoT save OoT last loaded (g_OotSaveInMemorySlot), -1 once that file was erased or copied over. OoT's dormant
// save is the file MM runs only while it is that file's: OoT saves it before every switch to MM, or has just loaded it
// when the file was last saved in MM, and the dormant grants write it at once.
constexpr int DormantSlot(int game, int foreground, bool combined, int ootPlaying, int mmPlaying, int mmResident,
                          int ootResident) {
    if (!combined || game == foreground)
        return -1;
    if (game == kMm)
        return foreground == kOot && IsFile(mmResident) && mmResident == ootPlaying ? mmResident : -1;
    if (game == kOot)
        return foreground == kMm && IsFile(mmPlaying) && mmPlaying == ootResident ? mmPlaying : -1;
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
// slider being dragged, a text field being typed into), so a drag writes the file once, when it is let go. A waiting
// change belongs to the file it was made in. Any other access, or another file, drops it: a running game saves its own
// file (and reloaded this one when it started), and no file or another file is being edited otherwise.
class DormantWrite {
  public:
    // A frame the editor drew. file: the file its game has loaded (its gSaveContext.fileNum).
    bool Step(Access access, int file, bool changed, bool controlActive) {
        if (access != Access::Dormant || file != mFile) {
            mPending = false;
        }
        if (access != Access::Dormant) {
            return false;
        }
        if (changed) {
            mPending = true;
            mFile = file;
        }
        if (!mPending || controlActive)
            return false;
        mPending = false;
        return true;
    }
    // A frame the editor was not drawn (the menu closed, or left the page or the editor): its controls let go, so a
    // change still waiting is written now if the same file is still edited dormant, and dropped otherwise. Returns
    // whether to write.
    bool Stopped(Access access, int file) {
        const bool write = mPending && access == Access::Dormant && file == mFile;
        mPending = false;
        return write;
    }
    // The file was just written another way (the dormant Give writes it itself): nothing is left to write.
    void Written() {
        mPending = false;
    }
    bool Pending() const {
        return mPending;
    }

  private:
    bool mPending = false;
    int mFile = -1;
};

// Whether the editor stopped being drawn, asked from its window's Update, which the Gui runs every frame whether or not
// the editor is shown. A whole frame must go by without it: a popped-out window's Update runs before it draws.
constexpr bool StoppedDrawing(int frame, int lastDrawnFrame) {
    return frame > lastDrawnFrame + 1;
}

// Wording. The first line of a dormant editor, and what a control that needs its game running says instead.
inline constexpr const char* kDormantOot = "Ocarina of Time isn't running. Changes here are saved to its file at once.";
inline constexpr const char* kDormantMm = "Majora's Mask isn't running. Changes here are saved to its file at once.";
inline constexpr const char* kSwitchToOot = "Switch to Ocarina of Time to edit this.";
inline constexpr const char* kSwitchToMm = "Switch to Majora's Mask to edit this.";

} // namespace ComboSaveEditorAccess
