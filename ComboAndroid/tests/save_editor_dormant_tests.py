"""Save Editor edits of the game that is not running reach that file's save, and only that file's.

Only one game's play loop runs at a time. The other game's save stays in memory and that game reloads its file when it
next starts (title_setup.c), so before 0.4.14 an edit made from the other game was lost on the switch, and MM's song
toggles and "Give randomizer item" did not even change memory (NextQuestInSlot returned without a play state, the
give was queued for a play loop that clears its queue on load). Builds three programs from production code and runs
them:

1. combo/gui/ComboSaveEditorAccess.h, the rule both editors and the launcher use, against an independent statement of
   it over every input; the launcher's production Combo_DormantSaveEditSlot (ComboShip.cpp) for the cases that matter
   (MM's dormant save counts only while it is the current save of the file OoT is playing: not on OoT's title or file
   screen, not after an MM reset or owl-save quit, not for another file; OoT's while MM runs that file; neither in a
   one-game mode). Then the production MM SaveEditorWindow::DrawElement, MmEditAccess and WriteDormantMmEdits over the
   production SaveManager_SaveCurrentForCombo / SaveManager_WriteSaveFile: a song learned from OoT is written to MM's
   section of the played file, once, after a drag is let go; nothing is written while MM runs (MM saves itself) or for
   any other file; an owl save of that file is kept in step.
2. The production OoT SaveEditorWindow::DrawElement, OotEditAccess and WriteDormantOotEdits: an edit made while MM
   runs is saved to OoT's section of the file MM runs (SaveManager::SaveFile, which refuses any other file), once;
   nothing while OoT runs or on its title and file screens.
3. MM's production NextQuestInSlot learns and forgets songs, boss remains and the notebook with no play state, and still
   leaves the sword and shield alone then; the production initSafeItemsForInventorySlot builds its slot lists again
   when Player_Init gives the gear its trade slots or the gear's runtime readiness changes (the Goron Tunic was missing
   from its slot for a session), without doubling them.
"""
from pathlib import Path
import argparse
import subprocess
import hostcxx

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
p.add_argument('--json-include', type=Path, required=True)
a = p.parse_args()
root = a.source.resolve()
a.build_dir.mkdir(parents=True, exist_ok=True)


def read(rel):
    return (root / rel).read_text(encoding='utf-8')


def function(text, signature):
    start = hostcxx.find(text, signature)
    opening = text.index('{', start)
    end, depth = opening + 1, 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end] + '\n'


def statement(text, needle):
    start = hostcxx.find(text, needle)
    return text[start:text.index(';', start) + 1] + '\n'


def run(name, program, includes, defines=('COMBO_BUILD',)):
    cpp = a.build_dir / f'{name}.cpp'
    cpp.write_text(program, encoding='utf-8')
    exe = hostcxx.build(a.compiler, cpp, includes=includes, defines=list(defines), warnings=False)
    subprocess.run([str(exe)], cwd=a.build_dir, check=True, timeout=hostcxx.RUN_TIMEOUT)


launcher = read('combo/ComboShip.cpp')
mm_editor = read('mm/2s2h/DeveloperTools/SaveEditor.cpp')
mm_saves = read('mm/2s2h/SaveManager/SaveManager.cpp')
oot_editor = read('soh/soh/Enhancements/debugger/debugSaveEditor.cpp')

# The seam is wired: both games get the launcher's rule, and the launcher asks OoT which file it plays.
for needle in ('SOH_SetSaveEditorDormantSlot(&Combo_DormantSaveEditSlot);',
               'MM_SetSaveEditorDormantSlot(&Combo_DormantSaveEditSlot);',
               'GetSym(sohModule, "SOH_GetPlayingFileNum")'):
    assert hostcxx.contains(launcher, needle), f'launcher lost {needle}'
assert hostcxx.contains(read('mm/2s2h/BenPort.cpp'),
                        'extern "C" COMBO_EXPORT void MM_SetSaveEditorDormantSlot(int (*fn)(int game)) {')
otr = read('soh/soh/OTRGlobals.cpp')
assert hostcxx.contains(otr, 'extern "C" COMBO_EXPORT void SOH_SetSaveEditorDormantSlot(int (*fn)(int game)) {')
playing = function(otr, 'extern "C" COMBO_EXPORT int SOH_GetPlayingFileNum(void) {')
assert 'GAMEMODE_TITLE_SCREEN' in playing and 'GAMEMODE_FILE_SELECT' in playing, 'OoT playing test differs from its editor'
# OoT's writer refuses any file but the one in memory, so the editor's SaveFile(gSaveContext.fileNum) cannot stray.
assert hostcxx.contains(read('soh/soh/SaveManager.cpp'), 'gSaveContext.fileNum != fileNum) return;')
# The give from OoT goes into the save the way a cross-game item does, before the play-loop queue is ever touched.
gives = function(mm_editor, 'static void DrawRandoItemGives() {')
assert hostcxx.find(gives, 'if (MmEditDormant()) {') < hostcxx.find(gives, 'Combo_MM_GiveDormantResolved(Rando::ConvertItem(randoItemId));') \
    < hostcxx.find(gives, 'GameInteractor::Instance->events.emplace_back('), 'MM dormant give lost'

mm_state = statement(mm_editor, 'static ComboSaveEditorAccess::Access sMmEditAccess') + \
    statement(mm_editor, 'static ComboSaveEditorAccess::DormantWrite sMmDormantWrite') + \
    function(mm_editor, 'static bool MmEditDormant() {')
mm_element = function(mm_editor, 'static ComboSaveEditorAccess::Access MmEditAccess() {') + \
    function(mm_editor, 'static void WriteDormantMmEdits(const Save& before) {') + \
    function(mm_editor, 'void SaveEditorWindow::DrawElement() {')
assert 'BeginTabItem("Quest Status")' in mm_element, 'not the combo MM DrawElement'

program = r'''
#include "gui/ComboSaveEditorAccess.h"
#include "rando/GameMode.h"
#include "rando/CrossForeign.h"
#include <atomic>
#include <cassert>
#include <cstdarg>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
using Json = nlohmann::json;
namespace Edit = ComboSaveEditorAccess;

// ---- The rule, stated independently.
int Spec(int game, int foreground, bool combined, int ootPlaying, int mmPlaying, int mmResident) {
    auto file = [](int slot) { return slot >= 0 && slot < 3; };
    if (!combined || game == foreground)
        return -1;
    if (game == 1 && foreground == 0 && file(mmResident) && mmResident == ootPlaying)
        return mmResident; // MM's memory is the played file's current MM save
    if (game == 0 && foreground == 1 && file(mmPlaying))
        return mmPlaying; // OoT saved (or just loaded) the file MM runs
    return -1;
}

// ---- The launcher (ComboShip.cpp), over stand-ins for its state.
namespace ComboAnchor {
std::atomic<int> sActiveGame{ 0 };
}
static ComboGameMode sBoundGameMode = COMBO_MODE_COMBINED;
static int g_PendingMMFileNum = -1, g_MmSaveInMemorySlot = -1;
int gOotPlaying = -1, gOotPlayingCalls = 0;
static int ReadOotPlaying() {
    ++gOotPlayingCalls;
    return gOotPlaying;
}
static int (*SOH_GetPlayingFileNum)(void) = ReadOotPlaying;
''' + function(launcher, 'static int Combo_DormantSaveEditSlot(int game) {') + r'''

// ---- MM: the save, the production writer and the production editor element.
typedef short s16;
enum { SAVETYPE_VANILLA, SAVETYPE_RANDO };
enum { GAMEOVER_INACTIVE };
struct Save {
    bool isOwlSave = false;
    struct {
        struct { unsigned questItems = 0; } inventory;
        struct { short health = 0x30; } playerData;
    } saveInfo;
    struct { int saveType = SAVETYPE_RANDO; int pauseSaveEntrance = -1; } shipSaveInfo;
};
void to_json(Json& j, const Save& s) {
    j = { { "isOwlSave", s.isOwlSave },
          { "saveInfo",
            { { "inventory", { { "questItems", s.saveInfo.inventory.questItems } } },
              { "playerData", { { "health", s.saveInfo.playerData.health } } } } },
          { "shipSaveInfo",
            { { "saveType", s.shipSaveInfo.saveType }, { "pauseSaveEntrance", s.shipSaveInfo.pauseSaveEntrance },
              { "respawn", 0 } } } };
}
struct SaveContext {
    Save save;
    s16 fileNum = 0xFF;
};
void to_json(Json& j, const SaveContext& c) {
    j = { { "save", c.save }, { "fileNum", c.fileNum } };
}
SaveContext gSaveContext;
struct GameOverContext { int state; };
struct PlayState { GameOverContext gameOverCtx; };
PlayState* gPlayState = nullptr; // MM's play state while OoT is in front
#define IS_RANDO (gSaveContext.save.shipSaveInfo.saveType == SAVETYPE_RANDO)
#define SPDLOG_ERROR(...) ((void)0)
#define SPDLOG_INFO(...) ((void)0)
const uint32_t CURRENT_SAVE_VERSION = 7;
int gComboOwlBlobSlot = -1;
const std::filesystem::path savesFolderPath("unused-saves");
static ComboRando::FnComboWriteSave gComboWriteGameSave = nullptr;
std::map<int, Json> gMmSections; // the container's MM section of each file, by 0-based file
int gWrites = 0, gWriteGame = -1, gWriteFile = -1;
void WriteSection(int game, int fileNum, const char* json) {
    ++gWrites;
    gWriteGame = game;
    gWriteFile = fileNum;
    gMmSections[fileNum] = Json::parse(json);
}
std::string SaveManager_GetFileName(int fileNum, bool isBackup = false);
int SaveManager_ReadSaveFile(const std::filesystem::path& fileName, Json& j) {
    const std::string name = fileName.filename().string();
    const int file = name[4] - '1';
    if (!gMmSections.count(file))
        return -1;
    j = gMmSections[file];
    return 0;
}
int SaveManager_MigrateSave(Json&) {
    return 0;
}
''' + function(mm_saves, 'static int SaveManager_ClassifySaveFile(') + \
    function(mm_saves, 'void SaveManager_WriteSaveFile(const std::filesystem::path& fileName, nlohmann::json j) {') + \
    function(mm_saves, 'std::string SaveManager_GetFileName(int fileNum, bool isBackup) {') + \
    function(mm_saves, 'void SaveManager_SaveCurrentForCombo() {') + r'''

// What the editor draws: the tab bar runs the selected tab, whose control makes the frame's edit.
namespace ImGui {
bool gControlActive = false;
std::string gText, gTab = "Quest Status";
bool IsAnyItemActive() {
    return gControlActive;
}
void TextWrapped(const char* format, ...) {
    va_list args;
    va_start(args, format);
    gText = std::strcmp(format, "%s") == 0 ? va_arg(args, const char*) : format;
    va_end(args);
}
bool BeginTabBar(const char*, int) {
    return true;
}
void EndTabBar() {}
bool BeginTabItem(const char* name) {
    return gTab == name;
}
void EndTabItem() {}
} // namespace ImGui
enum { ImGuiTabBarFlags_NoCloseWithMiddleMouseButton = 1 };
namespace UIWidgets {
enum class Colors : int {};
void PushStyleTabs(Colors) {}
void PopStyleTabs() {}
} // namespace UIWidgets
int CVarGetInteger(const char*, int fallback) {
    return fallback;
}
namespace ComboSaveEditorKit {
constexpr const char* kNoMmFile = "Load a Majora's Mask file to edit it.";
void BeginPanel(const char*) {}
void EndPanel() {}
} // namespace ComboSaveEditorKit
std::function<void()> gEdit = [] {};
int gTabsDrawn = 0, gListBuilds = 0;
void Tab() {
    ++gTabsDrawn;
    gEdit();
}
void DrawGeneralTab() { Tab(); }
void DrawItemsAndMasksTab() { Tab(); }
void DrawEquipmentTab() { Tab(); }
void DrawQuestStatusTab() { Tab(); }
void DrawDungeonItemTab() { Tab(); }
void DrawFlagsTab() { Tab(); }
void DrawPlayerTab() { Tab(); }
void DrawRandoTab() { Tab(); }
void DrawRegEditorTab() { Tab(); }
void initSafeItemsForInventorySlot() { ++gListBuilds; }
struct SaveEditorWindow {
    void DrawElement();
};
bool gMmInFront = false;
bool Combo_MmIsForeground(void) {
    return gMmInFront;
}
int (*gMMComboDormantSaveEditSlot)(int game) = nullptr;
''' + mm_state + mm_element + r'''

constexpr unsigned kSoaring = 1u << 15;
void LearnSoaring() {
    gSaveContext.save.saveInfo.inventory.questItems |= kSoaring;
}
unsigned WrittenQuestItems(int file) {
    return gMmSections.at(file).at("newCycleSave").at("save").at("saveInfo").at("inventory").at("questItems");
}
// The launcher's state as the game-switch loop leaves it while OoT plays `file` and MM's save for it is resident.
void OotPlays(int file) {
    ComboAnchor::sActiveGame = 0;
    gMmInFront = false;
    gOotPlaying = file;
    g_PendingMMFileNum = -1;
    g_MmSaveInMemorySlot = file;
    sBoundGameMode = COMBO_MODE_COMBINED;
    gSaveContext.fileNum = static_cast<s16>(file);
    gSaveContext.save = Save{};
}
int Frame(std::function<void()> edit = [] {}) {
    gEdit = edit;
    const int before = gWrites;
    SaveEditorWindow().DrawElement();
    return gWrites - before;
}

int main() {
    // 1a. The rule against its statement, over every input.
    for (int game = 0; game < 2; ++game)
        for (int foreground = 0; foreground < 2; ++foreground)
            for (int combined = 0; combined < 2; ++combined)
                for (int oot = -1; oot < 4; ++oot)
                    for (int mmPlaying = -1; mmPlaying < 4; ++mmPlaying)
                        for (int resident = -1; resident < 4; ++resident)
                            assert(Edit::DormantSlot(game, foreground, combined, oot, mmPlaying, resident) ==
                                   Spec(game, foreground, combined, oot, mmPlaying, resident));
    for (int fileNum = -1; fileNum < 4; ++fileNum)
        for (int slot = -1; slot < 4; ++slot) {
            assert(Edit::Decide(true, true, fileNum, slot) == Edit::Access::Live);
            assert(Edit::Decide(true, false, fileNum, slot) == Edit::Access::NoFile);
            assert(Edit::Decide(false, false, fileNum, slot) == Edit::Access::NoFile);
            const bool dormant = slot >= 0 && slot < 3 && fileNum == slot;
            assert(Edit::Decide(false, true, fileNum, slot) == (dormant ? Edit::Access::Dormant : Edit::Access::NoFile));
        }
    {
        Edit::DormantWrite write;
        assert(!write.Step(Edit::Access::Dormant, false, false));
        assert(!write.Step(Edit::Access::Dormant, true, true) && write.Pending()); // dragging: wait
        assert(!write.Step(Edit::Access::Dormant, false, true));
        assert(write.Step(Edit::Access::Dormant, false, false) && !write.Pending()); // let go: write once
        assert(!write.Step(Edit::Access::Dormant, false, false));
        assert(write.Step(Edit::Access::Dormant, true, false));
        assert(!write.Step(Edit::Access::Dormant, true, true));
        assert(!write.Step(Edit::Access::Live, false, false) && !write.Pending()); // the game runs again: dropped
        assert(!write.Step(Edit::Access::Dormant, false, false));
        assert(!write.Step(Edit::Access::Dormant, true, true));
        assert(!write.Step(Edit::Access::NoFile, false, false) && !write.Pending());
    }

    // 1b. The launcher.
    OotPlays(1);
    assert(Combo_DormantSaveEditSlot(1) == 1); // OoT plays file 2, whose MM save is MM's memory
    assert(Combo_DormantSaveEditSlot(0) == -1); // OoT is running: its own editor is live
    gOotPlaying = -1;
    assert(Combo_DormantSaveEditSlot(1) == -1); // OoT's title or file screen
    gOotPlaying = 2;
    assert(Combo_DormantSaveEditSlot(1) == -1); // MM's memory holds another file's save
    OotPlays(1);
    g_MmSaveInMemorySlot = -1;
    assert(Combo_DormantSaveEditSlot(1) == -1); // after an MM reset or owl-save quit, a failed load, an erase
    OotPlays(1);
    sBoundGameMode = COMBO_MODE_OOT_RANDO;
    assert(Combo_DormantSaveEditSlot(1) == -1); // a one-game file has no MM save to edit
    ComboAnchor::sActiveGame = 1; // MM runs file 3
    sBoundGameMode = COMBO_MODE_COMBINED;
    g_PendingMMFileNum = 2;
    const int calls = gOotPlayingCalls;
    assert(Combo_DormantSaveEditSlot(0) == 2 && Combo_DormantSaveEditSlot(1) == -1);
    assert(gOotPlayingCalls == calls); // OoT's dormant state is never asked what it plays
    sBoundGameMode = COMBO_MODE_MM_RANDO;
    assert(Combo_DormantSaveEditSlot(0) == -1);

    // 1c. The MM editor from OoT, end to end: production DrawElement, the launcher's rule and MM's writer.
    gComboWriteGameSave = WriteSection;
    gMMComboDormantSaveEditSlot = Combo_DormantSaveEditSlot;
    OotPlays(1);
    assert(Frame(LearnSoaring) == 1 && gWriteGame == ComboRando::GAME_MM && gWriteFile == 1);
    assert(WrittenQuestItems(1) & kSoaring); // Song of Soaring learned from OoT is in file 2's MM save
    assert(ImGui::gText == Edit::kDormantMm && gTabsDrawn > 0 && gListBuilds > 0);
    assert(Frame() == 0); // nothing changed, nothing written
    ImGui::gControlActive = true;
    assert(Frame([] { gSaveContext.save.saveInfo.playerData.health = 0x40; }) == 0); // a bar still being dragged
    assert(Frame([] { gSaveContext.save.saveInfo.playerData.health = 0x50; }) == 0);
    ImGui::gControlActive = false;
    assert(Frame() == 1 && gWriteFile == 1); // let go: written once, with the last value
    assert(gMmSections.at(1).at("newCycleSave").at("save").at("saveInfo").at("playerData").at("health") == 0x50);
    assert(Frame() == 0);

    // An owl save of the file is kept in step (MM loaded it), or dropped when MM's memory did not come from it.
    gComboOwlBlobSlot = 2;
    gMmSections[1]["owlSave"] = { { "save", Json(Save{}) } };
    assert(Frame([] { gSaveContext.save.saveInfo.inventory.questItems |= 1; }) == 1);
    assert(gMmSections.at(1).at("owlSave").at("save").at("saveInfo").at("inventory").at("questItems") == (kSoaring | 1));
    gComboOwlBlobSlot = -1;
    assert(Frame([] { gSaveContext.save.saveInfo.inventory.questItems |= 2; }) == 1 &&
           !gMmSections.at(1).contains("owlSave"));

    // No write for any other file, on OoT's title, after a reset, or from a one-game file; the tabs are not drawn.
    const auto refused = [](std::function<void()> setUp) {
        OotPlays(1);
        setUp();
        const int tabs = gTabsDrawn;
        const int written = Frame(LearnSoaring);
        return written == 0 && gTabsDrawn == tabs && ImGui::gText == ComboSaveEditorKit::kNoMmFile;
    };
    assert(refused([] { gOotPlaying = 2; }));
    assert(refused([] { gOotPlaying = -1; }));
    assert(refused([] { g_MmSaveInMemorySlot = -1; }));
    assert(refused([] { sBoundGameMode = COMBO_MODE_OOT_RANDO; }));
    assert(refused([] { gSaveContext.fileNum = 0xFF; g_MmSaveInMemorySlot = 0xFF; }));
    assert(refused([] { gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_VANILLA; }));
    assert(refused([] { gMMComboDormantSaveEditSlot = nullptr; }));
    gMMComboDormantSaveEditSlot = Combo_DormantSaveEditSlot;

    // A change waiting for a drag is dropped when MM starts (it reloads its file), not written later over MM's own.
    OotPlays(1);
    ImGui::gControlActive = true;
    assert(Frame(LearnSoaring) == 0);
    ComboAnchor::sActiveGame = 1;
    gMmInFront = true;
    g_PendingMMFileNum = 1;
    ImGui::gControlActive = false;
    const int tabs = gTabsDrawn;
    assert(Frame(LearnSoaring) == 0 && gTabsDrawn > tabs); // MM runs: live editing, MM saves its own file
    ComboAnchor::sActiveGame = 0;
    gMmInFront = false;
    g_PendingMMFileNum = -1;
    assert(Frame() == 0);
    std::cout << "PASS rule, launcher and MM dormant edits reach only the played file's MM save\n";
}
'''
run('mm_dormant_edits', program, [root / 'combo', a.json_include])

# ---- 2. OoT: production DrawElement over a stand-in SaveManager.
oot_state = statement(oot_editor, 'static ComboSaveEditorAccess::Access sOotEditAccess') + \
    statement(oot_editor, 'static ComboSaveEditorAccess::DormantWrite sOotDormantWrite') + \
    function(oot_editor, 'static bool OotEditDormant() {')
oot_element = function(oot_editor, 'static ComboSaveEditorAccess::Access OotEditAccess() {') + \
    function(oot_editor, 'static void WriteDormantOotEdits(const SaveContext& before) {') + \
    function(oot_editor, 'void SaveEditorWindow::DrawElement() {')
assert '{ kTabQuestStatus, DrawQuestStatusTab }' in oot_element, 'not the combo OoT DrawElement'
program = r'''
#include "gui/ComboSaveEditorAccess.h"
#include <cassert>
#include <cstdarg>
#include <cstring>
#include <functional>
#include <iostream>
#include <string>
#include <utility>
enum { GAMEMODE_NORMAL, GAMEMODE_TITLE_SCREEN, GAMEMODE_FILE_SELECT };
struct SaveContext {
    int fileNum = 0;
    int gameMode = GAMEMODE_NORMAL;
    unsigned questItems = 0;
};
SaveContext gSaveContext;
struct SaveManager {
    static SaveManager* Instance;
    int writes = 0, file = -1;
    void SaveFile(int fileNum) {
        ++writes;
        file = fileNum;
    }
};
SaveManager gSaves;
SaveManager* SaveManager::Instance = &gSaves;
namespace ImGui {
bool gControlActive = false;
std::string gText, gTab = "Quest Status";
bool IsAnyItemActive() { return gControlActive; }
void TextWrapped(const char* format, ...) {
    va_list args;
    va_start(args, format);
    gText = std::strcmp(format, "%s") == 0 ? va_arg(args, const char*) : format;
    va_end(args);
}
bool BeginTabBar(const char*, int) { return true; }
void EndTabBar() {}
bool BeginTabItem(const char* name) { return gTab == name; }
void EndTabItem() {}
void BeginDisabled(bool) {}
void EndDisabled() {}
} // namespace ImGui
enum { ImGuiTabBarFlags_NoCloseWithMiddleMouseButton = 1 };
#define THEME_COLOR 0
#define CVAR_SETTING(name) name
void PushStyleTabs(int) {}
void PopStyleTabs() {}
int CVarGetInteger(const char*, int fallback) { return fallback; }
void ResetBaseOptions() {}
namespace ComboSaveEditorKit {
constexpr const char* kNoOotFile = "Load an Ocarina of Time file to edit it.";
constexpr const char* kTabGeneral = "General";
constexpr const char* kTabInventory = "Inventory";
constexpr const char* kTabEquipment = "Equipment";
constexpr const char* kTabQuestStatus = "Quest Status";
constexpr const char* kTabDungeonItems = "Dungeon Items";
constexpr const char* kTabFlags = "Flags";
constexpr const char* kTabPlayer = "Player";
void BeginPanel(const char*) {}
void EndPanel() {}
} // namespace ComboSaveEditorKit
std::function<void()> gEdit = [] {};
int gTabsDrawn = 0;
void Tab() { ++gTabsDrawn; gEdit(); }
void DrawInfoTab() { Tab(); }
void DrawInventoryTab() { Tab(); }
void DrawEquipmentTab() { Tab(); }
void DrawQuestStatusTab() { Tab(); }
void DrawDungeonItemsTab() { Tab(); }
void DrawFlagsTab() { Tab(); }
void DrawPlayerTab() { Tab(); }
struct SaveEditorWindow { void DrawElement(); };
bool gOotInFront = false;
bool Combo_OotIsForeground(void) { return gOotInFront; }
int gMmRuns = 1; // the file MM runs (the launcher's g_PendingMMFileNum)
bool gCombined = true;
int LauncherRule(int game) {
    return ComboSaveEditorAccess::DormantSlot(game, gOotInFront ? 0 : 1, gCombined, -1, gMmRuns, -1);
}
int (*gComboSaveEditorDormantSlot)(int game) = LauncherRule;
''' + oot_state + oot_element + r'''
int Frame(std::function<void()> edit = [] {}) {
    gEdit = edit;
    const int before = gSaves.writes;
    SaveEditorWindow().DrawElement();
    return gSaves.writes - before;
}
void LearnPrelude() { gSaveContext.questItems |= 1u << 11; }

int main() {
    gSaveContext.fileNum = 1; // MM runs file 2; OoT's save for it is in memory
    assert(Frame(LearnPrelude) == 1 && gSaves.file == 1 && ImGui::gText == ComboSaveEditorAccess::kDormantOot);
    assert(Frame() == 0);
    ImGui::gControlActive = true;
    assert(Frame([] { gSaveContext.questItems |= 2; }) == 0);
    ImGui::gControlActive = false;
    assert(Frame() == 1 && gSaves.file == 1);
    int tabs = gTabsDrawn;
    gMmRuns = 2; // OoT's memory is not the file MM runs
    assert(Frame(LearnPrelude) == 0 && gTabsDrawn == tabs && ImGui::gText == ComboSaveEditorKit::kNoOotFile);
    gMmRuns = 1;
    gCombined = false; // an MM-only file
    assert(Frame(LearnPrelude) == 0 && ImGui::gText == ComboSaveEditorKit::kNoOotFile);
    gCombined = true;
    gComboSaveEditorDormantSlot = nullptr;
    assert(Frame(LearnPrelude) == 0);
    gComboSaveEditorDormantSlot = LauncherRule;
    gOotInFront = true; // OoT runs: live editing, OoT saves its own file
    tabs = gTabsDrawn;
    assert(Frame(LearnPrelude) == 0 && gTabsDrawn > tabs);
    gSaveContext.gameMode = GAMEMODE_FILE_SELECT;
    assert(Frame(LearnPrelude) == 0 && ImGui::gText == ComboSaveEditorKit::kNoOotFile);
    std::cout << "PASS OoT dormant edits reach only the file MM runs\n";
}
'''
run('oot_dormant_edits', program, [root / 'combo'])

# ---- 3. MM songs with no play state, and slot lists that follow Player_Init's gear slots.
program = r'''
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>
typedef unsigned char u8;
typedef signed char s8;
typedef short s16;
typedef unsigned short u16;
typedef int s32;
typedef unsigned int u32;
#include "z64item.h"
#include "rando/SharedItems.h"
enum { EQUIP_SLOT_B };
struct Save {
    struct {
        struct { u32 questItems; u8 items[48]; } inventory;
        struct { u8 swordHealth; } playerData;
    } saveInfo;
    u8 sword, shield, buttonB;
};
struct { Save save; } gSaveContext;
struct Player {};
struct PlayState {};
PlayState* gPlayState = nullptr;
Player gPlayer;
int gEquipCalls = 0;
#define GET_PLAYER(play) (&gPlayer)
#define CHECK_QUEST_ITEM(item) ((gSaveContext.save.saveInfo.inventory.questItems >> (item)) & 1)
#define SET_QUEST_ITEM(item) (gSaveContext.save.saveInfo.inventory.questItems |= (1u << (item)))
#define REMOVE_QUEST_ITEM(item) (gSaveContext.save.saveInfo.inventory.questItems &= ~(1u << (item)))
#define GET_CUR_EQUIP_VALUE(type) ((type) == EQUIP_TYPE_SWORD ? gSaveContext.save.sword : gSaveContext.save.shield)
#define SET_EQUIP_VALUE(type, value) \
    (((type) == EQUIP_TYPE_SWORD ? gSaveContext.save.sword : gSaveContext.save.shield) = (u8)(value))
#define BUTTON_ITEM_EQUIP(form, button) (gSaveContext.save.buttonB)
void Interface_LoadItemIconImpl(PlayState*, u8) {}
void Player_SetEquipmentData(PlayState*, Player*) { ++gEquipCalls; }
''' + function(mm_editor, 'void NextQuestInSlot(QuestItem slot) {') + r'''

u8 gItemSlots[256];
int gGearReady = 1;
extern "C" int Combo_MM_GearOptionReady(void) { return gGearReady; }
''' + statement(mm_editor, 'std::vector<ItemId> safeItemsForInventorySlot[SLOT_MASK_FIERCE_DEITY + 1]') + \
    function(mm_editor, 'static bool IsInventoryItemPubliclyAvailable(ItemId item) {') + \
    function(mm_editor, 'void initSafeItemsForInventorySlot() {') + r'''
bool Offers(InventorySlot slot, int item) {
    for (ItemId offered : safeItemsForInventorySlot[slot])
        if (offered == item)
            return true;
    return false;
}

int main() {
    // Songs, boss remains and the notebook change with no play state (MM while OoT is in front).
    for (QuestItem quest : { QUEST_SONG_SOARING, QUEST_SONG_EPONA, QUEST_SONG_HEALING, QUEST_REMAINS_ODOLWA,
                             QUEST_BOMBERS_NOTEBOOK }) {
        NextQuestInSlot(quest);
        assert(CHECK_QUEST_ITEM(quest));
        NextQuestInSlot(quest);
        assert(!CHECK_QUEST_ITEM(quest));
    }
    NextQuestInSlot(QUEST_SONG_LULLABY);
    assert(CHECK_QUEST_ITEM(QUEST_SONG_LULLABY_INTRO) && !CHECK_QUEST_ITEM(QUEST_SONG_LULLABY));
    // The sword and shield change Link, so they still need him.
    NextQuestInSlot(QUEST_SWORD);
    NextQuestInSlot(QUEST_SHIELD);
    assert(gSaveContext.save.sword == 0 && gSaveContext.save.shield == 0 && gEquipCalls == 0);
    PlayState play;
    gPlayState = &play;
    NextQuestInSlot(QUEST_SHIELD);
    assert(gSaveContext.save.shield == EQUIP_VALUE_SHIELD_HERO && gEquipCalls == 1);
    NextQuestInSlot(QUEST_SONG_SOARING);
    assert(CHECK_QUEST_ITEM(QUEST_SONG_SOARING));

    // Slot lists: built at boot, before Player_Init gives the gear its trade slots (ComboAdultGearIconResources.h).
    for (u8& slot : gItemSlots)
        slot = SLOT_NONE;
    gItemSlots[ITEM_ROOM_KEY] = SLOT_TRADE_KEY_MAMA;
    gItemSlots[ITEM_LETTER_MAMA] = SLOT_TRADE_KEY_MAMA;
    gItemSlots[ITEM_LETTER_TO_KAFEI] = SLOT_TRADE_COUPLE;
    initSafeItemsForInventorySlot();
    assert(safeItemsForInventorySlot[SLOT_TRADE_KEY_MAMA].size() == 2 && !Offers(SLOT_TRADE_KEY_MAMA, ITEM_COMBO_TUNIC_GORON));
    gItemSlots[ITEM_COMBO_BOOTS_IRON] = gItemSlots[ITEM_COMBO_TUNIC_GORON] = SLOT_TRADE_KEY_MAMA;
    gItemSlots[ITEM_COMBO_BOOTS_HOVER] = gItemSlots[ITEM_COMBO_TUNIC_ZORA] = SLOT_TRADE_COUPLE;
    initSafeItemsForInventorySlot(); // the next frame of the editor
    assert(Offers(SLOT_TRADE_KEY_MAMA, ITEM_COMBO_TUNIC_GORON) && Offers(SLOT_TRADE_COUPLE, ITEM_COMBO_TUNIC_ZORA));
    assert(safeItemsForInventorySlot[SLOT_TRADE_KEY_MAMA].size() == 4);
    initSafeItemsForInventorySlot();
    assert(safeItemsForInventorySlot[SLOT_TRADE_KEY_MAMA].size() == 4); // not doubled
    gGearReady = 0; // the gear's runtime is not ready on this device or for this Link
    initSafeItemsForInventorySlot();
    assert(!Offers(SLOT_TRADE_KEY_MAMA, ITEM_COMBO_TUNIC_GORON) && safeItemsForInventorySlot[SLOT_TRADE_KEY_MAMA].size() == 2);
    gGearReady = 1;
    initSafeItemsForInventorySlot();
    assert(Offers(SLOT_TRADE_KEY_MAMA, ITEM_COMBO_TUNIC_GORON));
    std::cout << "PASS MM songs without a play state and slot lists that follow the gear slots\n";
}
'''
run('mm_songs_and_slot_lists', program, [root / 'mm/include', root / 'combo', a.json_include])
print('Save Editor edits of the game not running reach the played file, and MM songs and slot lists are fixed.')
