"""Save Editor edits of the game that is not running reach that file's save, and only that file's.

Only one game's play loop runs at a time. The other game's save stays in memory and that game reloads its file when it
next starts (title_setup.c), so before 0.4.14 an edit made from the other game was lost on the switch, and MM's song
toggles and "Give randomizer item" did not even change memory (NextQuestInSlot returned without a play state, the
give was queued for a play loop that clears its queue on load). Builds five programs from production code and runs
them:

1. combo/gui/ComboSaveEditorAccess.h, the rule both editors and the launcher use, against an independent statement of
   it over every input, and the launcher's production Combo_DormantSaveEditSlot driven through the production
   residency changes: the peek load at an OoT file load and its failure, an erase and a copy, the return from MM by the
   Clock Tower door and by a reset. Then the production MM SaveEditorWindow::DrawElement, UpdateElement, MmEditAccess
   and WriteDormantMmEdits over the production SaveManager_SaveCurrentForCombo / SaveManager_WriteSaveFile: a song
   learned from OoT is written to MM's section of the played file, once, after a drag is let go; a dormant Give writes it once;
   a change still waiting when the editor stops being drawn is written then, or dropped when the file or the game
   changed; nothing is written while MM runs (MM saves itself), for any other file, or when MM's memory is the played
   file's but its loaded file is another; an owl save of that file is kept in step.
2. The production OoT DrawElement, UpdateElement, OotEditAccess and WriteDormantOotEdits over the same production
   launcher rule: an edit made while MM runs is saved to OoT's section of the file MM runs (SaveManager::SaveFile, which
   refuses any other file), once; nothing while OoT runs, on its title and file screens, or once that file's OoT save
   was erased or copied over or another file's is in OoT's memory.
3. MM's production NextQuestInSlot learns and forgets songs, boss remains and the notebook with no play state, and still
   leaves the sword and shield alone then; the production initSafeItemsForInventorySlot builds its slot lists again
   when Player_Init gives the gear its trade slots or the gear's runtime readiness changes, without doubling them.
4. With the bundled Dear ImGui, the production MM DrawElement drawing the production General tab (DrawTempleClears,
   DrawOwlStatuesEditor, the player name) from OoT: drawing it, opening Progress with every temple cleared, and
   focusing the name field write nothing and change no byte of the save; ticking "cleared" opens the temple; the magic
   bar's range comes from the save's level, not MM's runtime meter; "Is Owl Save" cannot be changed from OoT. Drawn
   while MM runs, the tab changes nothing either.
5. With the bundled Dear ImGui, the production OoT DrawElement drawing the production General tab from MM, after
   FileChoose_LoadGame's staging and the production SOH_ParkForComboMMResume (MM resumed from OoT's file screen):
   drawing it and opening Advanced and Minigames write nothing; the Navi Timer can be edited, Audio cannot; the
   production SOH_GrantCrossItem saves the file's real magic, not the staged 0; the editor sizes the magic bar from the
   level the save gives Link after a dormant double-meter grant. Drawn while OoT runs, the tab changes nothing.
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
# Dear ImGui sits beside the json headers in a FetchContent _deps directory (tests/CMakeLists.txt).
imgui = (a.json_include / '../../imgui-src').resolve()
assert (imgui / 'imgui.cpp').exists(), f'needs Dear ImGui sources at {imgui}'


def read(rel):
    return (root / rel).read_text(encoding='utf-8')


def combo_build(text):
    """The text the combo build compiles: COMBO_BUILD branches kept, the other branches of theirs dropped."""
    out, stack = [], []  # stack entries: None for other conditionals, else [taken, active]
    for line in text.split('\n'):
        directive = line.strip()
        if directive.startswith('#ifdef COMBO_BUILD'):
            stack.append([True, True])
            continue
        if directive.startswith('#ifndef COMBO_BUILD'):
            stack.append([False, False])
            continue
        if directive.startswith(('#if', '#ifdef', '#ifndef')):
            stack.append(None)
        elif directive.startswith(('#elif', '#else')) and stack and stack[-1] is not None:
            entry = stack[-1]
            entry[1] = not entry[0] and directive.startswith('#else')
            entry[0] = entry[0] or entry[1]
            continue
        elif directive.startswith('#endif') and stack:
            if stack.pop() is not None:
                continue
        if all(entry is None or entry[1] for entry in stack):
            out.append(line)
    return '\n'.join(out)


def braces(text, opening):
    """The index just past the brace that closes the one at `opening`."""
    end, depth = opening + 1, 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return end


def function(text, signature):
    start = hostcxx.find(text, signature)
    return text[start:braces(text, text.index('{', start))] + '\n'


def statement(text, needle):
    start = hostcxx.find(text, needle)
    return text[start:text.index(';', start) + 1] + '\n'


def block(text, needle, start=0):
    """From `needle` (an if statement) through its closing brace."""
    at = hostcxx.find(text, needle, start)
    return text[at:braces(text, text.index('{', at))] + '\n'


def run(name, program, includes, defines=('COMBO_BUILD',), sources=()):
    cpp = a.build_dir / f'{name}.cpp'
    cpp.write_text(program, encoding='utf-8')
    exe = hostcxx.build(a.compiler, cpp, includes=includes, defines=list(defines), warnings=False, sources=sources,
                        msvc=['/Zc:preprocessor', '/utf-8'])
    subprocess.run([str(exe)], cwd=a.build_dir, check=True, timeout=hostcxx.RUN_TIMEOUT)


launcher = read('combo/ComboShip.cpp')
mm_editor = combo_build(read('mm/2s2h/DeveloperTools/SaveEditor.cpp'))
mm_saves = read('mm/2s2h/SaveManager/SaveManager.cpp')
oot_editor = combo_build(read('soh/soh/Enhancements/debugger/debugSaveEditor.cpp'))
otr = read('soh/soh/OTRGlobals.cpp')

# The seam is wired: both games get the launcher's rule, and the launcher asks OoT which file it plays.
for needle in ('SOH_SetSaveEditorDormantSlot(&Combo_DormantSaveEditSlot);',
               'MM_SetSaveEditorDormantSlot(&Combo_DormantSaveEditSlot);',
               'GetSym(sohModule, "SOH_GetPlayingFileNum")'):
    assert hostcxx.contains(launcher, needle), f'launcher lost {needle}'
assert hostcxx.contains(read('mm/2s2h/BenPort.cpp'),
                        'extern "C" COMBO_EXPORT void MM_SetSaveEditorDormantSlot(int (*fn)(int game)) {')
assert hostcxx.contains(otr, 'extern "C" COMBO_EXPORT void SOH_SetSaveEditorDormantSlot(int (*fn)(int game)) {')
playing = function(otr, 'extern "C" COMBO_EXPORT int SOH_GetPlayingFileNum(void) {')
assert 'GAMEMODE_TITLE_SCREEN' in playing and 'GAMEMODE_FILE_SELECT' in playing, 'OoT playing test differs from its editor'
# OoT's writer refuses any file but the one in memory, so the editor's SaveFile(gSaveContext.fileNum) cannot stray.
assert hostcxx.contains(read('soh/soh/SaveManager.cpp'), 'gSaveContext.fileNum != fileNum) return;')
# The give from OoT goes into the save the way a cross-game item does, before the play-loop queue is ever touched.
gives = function(mm_editor, 'static void DrawRandoItemGives() {')
assert hostcxx.find(gives, 'if (MmEditDormant()) {') < hostcxx.find(gives, 'Combo_MM_GiveDormantResolved(Rando::ConvertItem(randoItemId));') \
    < hostcxx.find(gives, 'GameInteractor::Instance->events.emplace_back('), 'MM dormant give lost'
dormant_give = block(gives, 'if (MmEditDormant()) {')
# The Randomizer tab's check list hands its reward list a copy, written back only when a reward is picked: the list
# replaces a value it does not list as it draws (UIWidgets::ComboboxWithSearch).
rando_tab = function(mm_editor, 'void DrawRandoTab() {')
assert hostcxx.contains(rando_tab, 'RandoItemId reward = randoSaveCheck.randoItemId;') and \
    hostcxx.contains(rando_tab, '&reward, &randoItemIdComboboxMap') and \
    hostcxx.contains(rando_tab, ')) { randoSaveCheck.randoItemId = reward; }') and \
    '&randoSaveCheck.randoItemId' not in rando_tab, "the check list's reward list writes the save as it draws"
assert hostcxx.contains(read('mm/2s2h/BenGui/UIWidgets.hpp'), 'if (!comboMap->contains(*value)) { *value = comboMap->begin()->first; }')
# Both windows' Update writes or drops a change left waiting when the editor stops being drawn.
for header in ('mm/2s2h/DeveloperTools/SaveEditor.h', 'soh/soh/Enhancements/debugger/debugSaveEditor.h'):
    assert hostcxx.contains(combo_build(read(header)), 'void UpdateElement() override;'), f'{header} lost UpdateElement'

# The launcher's state and the production code that changes it.
launcher_rule = function(launcher, 'static int Combo_DormantSaveEditSlot(int game) {')
residency = function(launcher, 'static bool ComboIsValidSlot(int fileNum) {') + \
    'void OotLoads(int fileNum) {\n' + statement(launcher, 'g_OotSaveInMemorySlot = ComboIsValidSlot(fileNum)') + '}\n' + \
    'void PeekLoad(int fileNum) {\n' + \
    statement(launcher, 'g_MmSaveInMemorySlot = -1; // Failure marks native fileNum unbound') + \
    block(launcher, 'if (MM_LoadSaveForCombo(fileNum) == 0) {',
          hostcxx.find(launcher, 'g_MmSaveInMemorySlot = -1; // Failure marks native fileNum unbound')) + '}\n' + \
    function(launcher, 'static void EraseComboContainer(int slot) {') + \
    function(launcher, 'static void Combo_CopyContainer(int from, int to) {') + \
    'void ReturnFromMm() {\n' + block(launcher, 'if (g_PendingMMFileNum >= 0) {', hostcxx.find(launcher, 'if (g_pendingOOTReturn) {')) + '}\n'
assert 'g_mmReturnKind == 0' in residency, 'not the MM return block'
# The game-switch loop marks the file MM enters as resident (the harness does the same).
assert hostcxx.contains(launcher, 'g_pendingOOTReturn = false; // MM\'s own boot/resume path loads this slot\'s save into '
                        'gSaveContext. g_MmSaveInMemorySlot = g_PendingMMFileNum;')
assert hostcxx.find(launcher, 'g_OotSaveInMemorySlot = ComboIsValidSlot(fileNum)') < \
    hostcxx.find(launcher, 'if (!ComboIsValidSlot(fileNum))', hostcxx.find(launcher, 'static void Combo_OnOOTSaveLoad(int fileNum) {')), \
    'OoT residency is not recorded first on every OoT load'
launcher_state = r'''
namespace ComboAnchor {
std::atomic<int> sActiveGame{ 0 };
}
static ComboGameMode sBoundGameMode = COMBO_MODE_COMBINED;
static int g_PendingMMFileNum = -1, g_MmSaveInMemorySlot = -1, g_OotSaveInMemorySlot = -1, g_mmReturnKind = 0;
int gOotPlaying = -1, gOotPlayingCalls = 0;
static int ReadOotPlaying() {
    ++gOotPlayingCalls;
    return gOotPlaying;
}
static int (*SOH_GetPlayingFileNum)(void) = ReadOotPlaying;
'''

mm_state = statement(mm_editor, 'static ComboSaveEditorAccess::Access sMmEditAccess') + \
    statement(mm_editor, 'static ComboSaveEditorAccess::DormantWrite sMmDormantWrite') + \
    statement(mm_editor, 'static Save sMmBefore;') + statement(mm_editor, 'static int sMmDrawnFrame') + \
    function(mm_editor, 'static bool MmEditDormant() {') + function(mm_editor, 'static PlayState* MmEditPlay() {') + \
    function(mm_editor, 'static void MmDormantSaveWritten() {')
mm_element = function(mm_editor, 'static ComboSaveEditorAccess::Access MmEditAccess() {') + \
    function(mm_editor, 'static void WriteDormantMmEdits(const Save& before) {') + \
    function(mm_editor, 'void SaveEditorWindow::UpdateElement() {') + \
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
#include <mutex>
#include <string>
using Json = nlohmann::json;
namespace Edit = ComboSaveEditorAccess;

// ---- The rule, stated independently.
int Spec(int game, int foreground, bool combined, int ootPlaying, int mmPlaying, int mmResident, int ootResident) {
    auto file = [](int slot) { return slot >= 0 && slot < 3; };
    if (!combined || game == foreground)
        return -1;
    if (game == 1 && foreground == 0 && file(mmResident) && mmResident == ootPlaying)
        return mmResident; // MM's memory is the played file's current MM save
    if (game == 0 && foreground == 1 && file(mmPlaying) && mmPlaying == ootResident)
        return mmPlaying; // OoT's memory is the file MM runs: OoT loaded it, saved it before the switch
    return -1;
}

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

// ---- The launcher (ComboShip.cpp), over stand-ins for its state and the production code that changes it.
''' + launcher_state + launcher_rule + r'''
std::mutex g_containerMutex;
std::map<int, Json> g_containerCache;
int g_profileSavedSeedRevision = 0, gOwlInvalidations = 0;
std::filesystem::path ComboContainerPath(int slot) {
    return "absent-test-container-" + std::to_string(slot) + ".combosav";
}
Json& LoadOrCreateContainer(int slot) {
    return g_containerCache[slot];
}
void FlushContainer(int) {}
void InvalidateOwlBlob() {
    ++gOwlInvalidations;
}
static void (*MM_InvalidateOwlBlobSlot)(void) = InvalidateOwlBlob;
static void (*ComboUI_SetHintTrackerData)(int, const char*, const char*) = nullptr;
bool gPeekFails = false;
int LoadMmSave(int fileNum) { // MM_LoadSaveForCombo: MM's memory becomes the file's MM save, or no file
    gSaveContext.save = Save{};
    gSaveContext.fileNum = gPeekFails ? 0xFF : static_cast<s16>(fileNum);
    return gPeekFails ? -1 : 0;
}
static int (*MM_LoadSaveForCombo)(int) = LoadMmSave;
static void (*MM_Anchor_RequestResync)(void) = nullptr;
std::map<int, int> gLastGame;
void Combo_SetLastGame(int fileNum, int game) {
    gLastGame[fileNum] = game;
}
''' + residency + r'''

// What the editor draws: the tab bar runs the selected tab, whose control makes the frame's edit.
namespace ImGui {
bool gControlActive = false;
int gFrame = 0;
std::string gText, gTab = "Quest Status";
bool IsAnyItemActive() {
    return gControlActive;
}
int GetFrameCount() {
    return gFrame;
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
    void UpdateElement();
};
bool gMmInFront = false;
bool Combo_MmIsForeground(void) {
    return gMmInFront;
}
int (*gMMComboDormantSaveEditSlot)(int game) = nullptr;
''' + mm_state + mm_element + r'''

// The dormant Give, as Randomizer > Give randomizer item runs it from OoT (production branch of DrawRandoItemGives).
using RandoItemId = int;
namespace Rando {
RandoItemId ConvertItem(RandoItemId item) {
    return item;
}
} // namespace Rando
void Combo_MM_GiveDormantResolved(RandoItemId item) { // as BenPort.cpp: into the save, then the file
    gSaveContext.save.saveInfo.inventory.questItems |= 1u << item;
    SaveManager_SaveCurrentForCombo();
}
void GiveFromOot(RandoItemId randoItemId) {
    for (int once = 0; once < 1; ++once) {
''' + dormant_give + r'''
    }
}

constexpr unsigned kSoaring = 1u << 15;
void LearnSoaring() {
    gSaveContext.save.saveInfo.inventory.questItems |= kSoaring;
}
unsigned WrittenQuestItems(int file) {
    return gMmSections.at(file).at("newCycleSave").at("save").at("saveInfo").at("inventory").at("questItems");
}
// The launcher's state as the game-switch loop leaves it while OoT plays `file`: OoT loaded the file, which brought
// that file's MM save into MM's memory (the peek load).
void OotPlays(int file) {
    ComboAnchor::sActiveGame = 0;
    gMmInFront = false;
    gOotPlaying = file;
    g_PendingMMFileNum = -1;
    sBoundGameMode = COMBO_MODE_COMBINED;
    gPeekFails = false;
    OotLoads(file);
    g_MmSaveInMemorySlot = -1;
    PeekLoad(file);
    assert(g_OotSaveInMemorySlot == file && g_MmSaveInMemorySlot == file && gSaveContext.fileNum == file);
}
// One menu frame: the Gui runs the window's Update, then the menu draws the editor (unless it is closed).
int Frame(std::function<void()> edit = [] {}, bool drawn = true) {
    ++ImGui::gFrame;
    gEdit = edit;
    const int before = gWrites;
    SaveEditorWindow window;
    window.UpdateElement();
    if (drawn)
        window.DrawElement();
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
                            for (int ootResident = -1; ootResident < 4; ++ootResident)
                                assert(Edit::DormantSlot(game, foreground, combined, oot, mmPlaying, resident,
                                                         ootResident) ==
                                       Spec(game, foreground, combined, oot, mmPlaying, resident, ootResident));
    for (int fileNum = -1; fileNum < 4; ++fileNum)
        for (int slot = -1; slot < 4; ++slot) {
            assert(Edit::Decide(true, true, fileNum, slot) == Edit::Access::Live);
            assert(Edit::Decide(true, false, fileNum, slot) == Edit::Access::NoFile);
            assert(Edit::Decide(false, false, fileNum, slot) == Edit::Access::NoFile);
            const bool dormant = slot >= 0 && slot < 3 && fileNum == slot;
            assert(Edit::Decide(false, true, fileNum, slot) == (dormant ? Edit::Access::Dormant : Edit::Access::NoFile));
        }
    {
        using A = Edit::Access;
        Edit::DormantWrite write;
        assert(!write.Step(A::Dormant, 1, false, false));
        assert(!write.Step(A::Dormant, 1, true, true) && write.Pending()); // dragging: wait
        assert(!write.Step(A::Dormant, 1, false, true));
        assert(write.Step(A::Dormant, 1, false, false) && !write.Pending()); // let go: write once
        assert(!write.Step(A::Dormant, 1, false, false));
        assert(write.Step(A::Dormant, 1, true, false));
        assert(!write.Step(A::Dormant, 1, true, true));
        assert(!write.Step(A::Live, 1, false, false) && !write.Pending()); // the game runs again: dropped
        assert(!write.Step(A::Dormant, 1, false, false));
        assert(!write.Step(A::Dormant, 1, true, true));
        assert(!write.Step(A::NoFile, 1, false, false) && !write.Pending());
        assert(!write.Step(A::Dormant, 1, true, true));
        assert(!write.Step(A::Dormant, 2, false, false) && !write.Pending()); // another file: never written for it
        assert(!write.Step(A::Dormant, 2, true, true));
        assert(write.Stopped(A::Dormant, 2) && !write.Pending()); // the editor closed mid-drag: written then
        assert(!write.Stopped(A::Dormant, 2));
        assert(!write.Step(A::Dormant, 2, true, true));
        assert(!write.Stopped(A::Dormant, 1) && !write.Pending()); // closed, and another file is in memory now
        assert(!write.Step(A::Dormant, 2, true, true));
        assert(!write.Stopped(A::Live, 2) && !write.Pending()); // closed, and the game switched
        assert(!write.Step(A::Dormant, 2, true, true));
        write.Written(); // the Give wrote the file itself
        assert(!write.Pending() && !write.Step(A::Dormant, 2, false, false));
        assert(!Edit::StoppedDrawing(5, 5) && !Edit::StoppedDrawing(5, 4) && Edit::StoppedDrawing(6, 4));
    }

    // 1b. The launcher, through its production residency changes.
    OotPlays(1);
    assert(Combo_DormantSaveEditSlot(1) == 1); // OoT plays file 2, whose MM save is MM's memory
    assert(Combo_DormantSaveEditSlot(0) == -1); // OoT is running: its own editor is live
    gOotPlaying = -1;
    assert(Combo_DormantSaveEditSlot(1) == -1); // OoT's title or file screen
    gOotPlaying = 2;
    assert(Combo_DormantSaveEditSlot(1) == -1); // MM's memory holds another file's save
    OotPlays(1);
    EraseComboContainer(1);
    assert(g_MmSaveInMemorySlot == -1 && g_OotSaveInMemorySlot == -1 && Combo_DormantSaveEditSlot(1) == -1);
    OotPlays(1);
    Combo_CopyContainer(0, 2); // another file's copy leaves file 2 alone
    assert(Combo_DormantSaveEditSlot(1) == 1);
    Combo_CopyContainer(0, 1); // file 2 copied over
    assert(g_MmSaveInMemorySlot == -1 && g_OotSaveInMemorySlot == -1 && Combo_DormantSaveEditSlot(1) == -1);
    OotPlays(1);
    gPeekFails = true; // OoT loads file 3, whose MM save cannot be read
    gOotPlaying = 2;
    OotLoads(2);
    PeekLoad(2);
    assert(g_MmSaveInMemorySlot == -1 && gSaveContext.fileNum == 0xFF && Combo_DormantSaveEditSlot(1) == -1);
    OotPlays(1);
    sBoundGameMode = COMBO_MODE_OOT_RANDO;
    assert(Combo_DormantSaveEditSlot(1) == -1); // a one-game file has no MM save to edit
    // MM runs file 2 (the game-switch loop marks it resident as MM loads it); OoT saved it before the switch.
    OotPlays(1);
    ComboAnchor::sActiveGame = 1;
    g_PendingMMFileNum = 1;
    g_MmSaveInMemorySlot = g_PendingMMFileNum;
    const int calls = gOotPlayingCalls;
    assert(Combo_DormantSaveEditSlot(0) == 1 && Combo_DormantSaveEditSlot(1) == -1);
    assert(gOotPlayingCalls == calls); // OoT's dormant state is never asked what it plays
    g_OotSaveInMemorySlot = 2;
    assert(Combo_DormantSaveEditSlot(0) == -1); // OoT's memory holds another file's save
    g_OotSaveInMemorySlot = 1;
    sBoundGameMode = COMBO_MODE_MM_RANDO;
    assert(Combo_DormantSaveEditSlot(0) == -1);
    sBoundGameMode = COMBO_MODE_COMBINED;
    // Back to OoT by the Clock Tower door: MM's memory stays the file's current save.
    g_mmReturnKind = 0;
    ReturnFromMm();
    ComboAnchor::sActiveGame = 0;
    g_PendingMMFileNum = -1;
    assert(gLastGame[1] == ComboRando::GAME_OOT && g_MmSaveInMemorySlot == 1 && Combo_DormantSaveEditSlot(1) == 1);
    // By a reset or an owl-save quit in MM: MM's memory is post-quit state, no longer the file's save.
    ComboAnchor::sActiveGame = 1;
    g_PendingMMFileNum = 1;
    g_mmReturnKind = 1;
    ReturnFromMm();
    ComboAnchor::sActiveGame = 0;
    g_PendingMMFileNum = -1;
    assert(g_MmSaveInMemorySlot == -1 && Combo_DormantSaveEditSlot(1) == -1);

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
    // A Give from OoT writes the file itself, and only once.
    assert(Frame([] { GiveFromOot(3); }) == 1 && (WrittenQuestItems(1) & (1u << 3)));
    assert(Frame() == 0);

    // An owl save of the file is kept in step (MM loaded it), or dropped when MM's memory did not come from it.
    gComboOwlBlobSlot = 2;
    gMmSections[1]["owlSave"] = { { "save", Json(Save{}) } };
    assert(Frame([] { gSaveContext.save.saveInfo.inventory.questItems |= 1; }) == 1);
    assert(gMmSections.at(1).at("owlSave").at("save").at("saveInfo").at("inventory").at("questItems") ==
           WrittenQuestItems(1));
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
    assert(refused([] { EraseComboContainer(1); }));
    assert(refused([] { sBoundGameMode = COMBO_MODE_OOT_RANDO; }));
    assert(refused([] { gSaveContext.fileNum = 0xFF; g_MmSaveInMemorySlot = 0xFF; }));
    assert(refused([] { gSaveContext.fileNum = 2; })); // the played file's residency, but MM has another file loaded
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

    // The menu closes mid-drag: the change is written once the editor is no longer drawn, to the same file.
    OotPlays(1);
    ImGui::gControlActive = true;
    assert(Frame([] { gSaveContext.save.saveInfo.playerData.health = 0x24; }) == 0);
    ImGui::gControlActive = false;
    assert(Frame({}, false) == 0); // one frame later: possibly a popped-out window's Update before it draws
    assert(Frame({}, false) == 1 && gWriteFile == 1);
    assert(gMmSections.at(1).at("newCycleSave").at("save").at("saveInfo").at("playerData").at("health") == 0x24);
    assert(Frame({}, false) == 0 && Frame() == 0);
    // ... and dropped when, by then, OoT plays another file or MM was entered.
    for (int change = 0; change < 2; ++change) {
        OotPlays(1);
        ImGui::gControlActive = true;
        assert(Frame(LearnSoaring) == 0);
        ImGui::gControlActive = false;
        if (change == 0) {
            OotPlays(2);
        } else {
            ComboAnchor::sActiveGame = 1;
            gMmInFront = true;
            g_PendingMMFileNum = 1;
        }
        assert(Frame({}, false) == 0 && Frame({}, false) == 0 && Frame({}, false) == 0);
        ComboAnchor::sActiveGame = 0;
        gMmInFront = false;
        g_PendingMMFileNum = -1;
        OotPlays(1);
        assert(Frame() == 0 && Frame({}, false) == 0 && Frame({}, false) == 0);
    }
    std::cout << "PASS rule, launcher residency and MM dormant edits reach only the played file's MM save\n";
}
'''
run('mm_dormant_edits', program, [root / 'combo', a.json_include])

# ---- 2. OoT: production DrawElement over a stand-in SaveManager and the production launcher rule.
oot_state = statement(oot_editor, 'static ComboSaveEditorAccess::Access sOotEditAccess') + \
    statement(oot_editor, 'static ComboSaveEditorAccess::DormantWrite sOotDormantWrite') + \
    statement(oot_editor, 'static int sOotDrawnFrame') + \
    function(oot_editor, 'static bool OotEditDormant() {') + function(oot_editor, 'static PlayState* OotEditPlay() {')
oot_element = function(oot_editor, 'static ComboSaveEditorAccess::Access OotEditAccess() {') + \
    function(oot_editor, 'static void WriteDormantOotEdits(const SaveContext& before) {') + \
    function(oot_editor, 'void SaveEditorWindow::UpdateElement() {') + \
    function(oot_editor, 'void SaveEditorWindow::DrawElement() {')
assert '{ kTabQuestStatus, DrawQuestStatusTab }' in oot_element, 'not the combo OoT DrawElement'
program = r'''
#include "gui/ComboSaveEditorAccess.h"
#include "rando/GameMode.h"
#include "rando/CrossForeign.h"
#include <atomic>
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
struct PlayState {};
PlayState* gPlayState = nullptr;
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
int gFrame = 0;
std::string gText, gTab = "Quest Status";
bool IsAnyItemActive() { return gControlActive; }
int GetFrameCount() { return gFrame; }
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
struct SaveEditorWindow {
    void DrawElement();
    void UpdateElement();
};
''' + launcher_state + launcher_rule + r'''
bool Combo_OotIsForeground(void) { return ComboAnchor::sActiveGame == 0; }
int (*gComboSaveEditorDormantSlot)(int game) = Combo_DormantSaveEditSlot;
''' + oot_state + oot_element + r'''
int Frame(std::function<void()> edit = [] {}, bool drawn = true) {
    ++ImGui::gFrame;
    gEdit = edit;
    const int before = gSaves.writes;
    SaveEditorWindow window;
    window.UpdateElement();
    if (drawn)
        window.DrawElement();
    return gSaves.writes - before;
}
void LearnPrelude() { gSaveContext.questItems ^= 1u << 11; } // a tap on Prelude of Light: always a change
// MM runs file 2: OoT loaded it (g_OotSaveInMemorySlot) and saved it before the switch.
void MmRuns(int file) {
    ComboAnchor::sActiveGame = 1;
    sBoundGameMode = COMBO_MODE_COMBINED;
    g_PendingMMFileNum = g_MmSaveInMemorySlot = file;
    g_OotSaveInMemorySlot = file;
    gSaveContext.fileNum = file;
    gSaveContext.gameMode = GAMEMODE_NORMAL;
}

int main() {
    MmRuns(1);
    assert(Frame(LearnPrelude) == 1 && gSaves.file == 1 && ImGui::gText == ComboSaveEditorAccess::kDormantOot);
    assert(Frame() == 0);
    ImGui::gControlActive = true;
    assert(Frame([] { gSaveContext.questItems |= 2; }) == 0);
    ImGui::gControlActive = false;
    assert(Frame() == 1 && gSaves.file == 1);
    int tabs = gTabsDrawn;
    g_PendingMMFileNum = 2; // OoT's memory is not the file MM runs
    assert(Frame(LearnPrelude) == 0 && gTabsDrawn == tabs && ImGui::gText == ComboSaveEditorKit::kNoOotFile);
    MmRuns(1);
    g_OotSaveInMemorySlot = -1; // that file was erased or copied over
    assert(Frame(LearnPrelude) == 0 && ImGui::gText == ComboSaveEditorKit::kNoOotFile);
    MmRuns(1);
    g_OotSaveInMemorySlot = 2; // OoT last loaded another file
    assert(Frame(LearnPrelude) == 0 && ImGui::gText == ComboSaveEditorKit::kNoOotFile);
    MmRuns(1);
    sBoundGameMode = COMBO_MODE_MM_RANDO; // an MM-only file
    assert(Frame(LearnPrelude) == 0 && ImGui::gText == ComboSaveEditorKit::kNoOotFile);
    MmRuns(1);
    gComboSaveEditorDormantSlot = nullptr;
    assert(Frame(LearnPrelude) == 0);
    gComboSaveEditorDormantSlot = Combo_DormantSaveEditSlot;
    // The menu closes with a control still held: written once the editor is not drawn, or dropped once OoT runs.
    MmRuns(1);
    ImGui::gControlActive = true;
    assert(Frame(LearnPrelude) == 0);
    ImGui::gControlActive = false;
    assert(Frame({}, false) == 0 && Frame({}, false) == 1 && gSaves.file == 1 && Frame({}, false) == 0);
    ImGui::gControlActive = true;
    assert(Frame(LearnPrelude) == 0);
    ImGui::gControlActive = false;
    ComboAnchor::sActiveGame = 0;
    gOotPlaying = 1;
    assert(Frame({}, false) == 0 && Frame({}, false) == 0 && Frame({}, false) == 0);
    tabs = gTabsDrawn;
    assert(Frame(LearnPrelude) == 0 && gTabsDrawn > tabs); // OoT runs: live editing, OoT saves its own file
    gSaveContext.gameMode = GAMEMODE_FILE_SELECT;
    assert(Frame(LearnPrelude) == 0 && ImGui::gText == ComboSaveEditorKit::kNoOotFile);
    std::cout << "PASS OoT dormant edits reach only the file MM runs, while OoT's memory is that file's\n";
}
'''
run('oot_dormant_edits', program, [root / 'combo', a.json_include])

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
bool gFromOot = false;
PlayState* MmEditPlay() { return gFromOot ? nullptr : gPlayState; }
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
    // The sword and shield change Link, so they still need him: not from OoT, even with a play state MM left behind.
    NextQuestInSlot(QUEST_SWORD);
    NextQuestInSlot(QUEST_SHIELD);
    assert(gSaveContext.save.sword == 0 && gSaveContext.save.shield == 0 && gEquipCalls == 0);
    PlayState play;
    gPlayState = &play;
    gFromOot = true;
    NextQuestInSlot(QUEST_SHIELD);
    assert(gSaveContext.save.shield == 0 && gEquipCalls == 0);
    gFromOot = false;
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

# ---- Shared by 4 and 5: the bundled Dear ImGui, with its test-engine hooks recording where each item was drawn, and a
# full-screen menu window that clicks items by their label as a finger would.
imgui_sources = [imgui / file for file in ['imgui.cpp', 'imgui_draw.cpp', 'imgui_tables.cpp', 'imgui_widgets.cpp']]
imgui_driver = r'''
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>
#include <cassert>
#include <cstdio>
#include <functional>
#include <map>
#include <string>
namespace Drive {
std::map<ImGuiID, ImRect> gRects;
std::map<std::string, ImGuiID> gIds;
std::function<void()> gDraw;
const ImVec2 kAway(6.0f, 7990.0f); // the bottom of the menu, where nothing is drawn
} // namespace Drive
void ImGuiTestEngineHook_ItemAdd(ImGuiContext*, ImGuiID id, const ImRect& bb, const ImGuiLastItemData*) {
    Drive::gRects[id] = bb;
}
void ImGuiTestEngineHook_ItemInfo(ImGuiContext*, ImGuiID id, const char* label, ImGuiItemStatusFlags) {
    if (label)
        Drive::gIds[label] = id;
}
void ImGuiTestEngineHook_Log(ImGuiContext*, const char*, ...) {}
const char* ImGuiTestEngine_FindItemDebugLabel(ImGuiContext*, ImGuiID) {
    return nullptr;
}
namespace Drive {
void Setup() {
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = ImVec2(1600.0f, 8000.0f);
    io.ConfigInputTrickleEventQueue = false;
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    ImGui::GetCurrentContext()->TestEngineHookItems = true;
    io.AddMousePosEvent(kAway.x, kAway.y);
}
// One frame of the menu with the editor in it.
void Frame() {
    ImGui::GetIO().DeltaTime = 1.0f / 60.0f;
    ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("Menu", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings);
    gDraw();
    ImGui::End();
    ImGui::Render();
}
void Frames(int count) {
    for (int i = 0; i < count; ++i)
        Frame();
}
bool Drawn(const char* label) {
    return gIds.count(label) != 0;
}
ImRect Rect(const char* label) {
    if (!Drawn(label))
        fprintf(stderr, "not drawn: %s\n", label);
    assert(Drawn(label));
    return gRects.at(gIds.at(label));
}
// A tap: down on the item, up, and the finger leaves.
void Tap(ImVec2 at) {
    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent(at.x, at.y);
    io.AddMouseButtonEvent(0, true);
    Frame();
    io.AddMouseButtonEvent(0, false);
    Frame();
    io.AddMousePosEvent(kAway.x, kAway.y);
    Frame();
}
void Tap(const char* label) {
    Tap(Rect(label).GetCenter());
}
// A tap at a bar's right end: its maximum.
void TapEnd(const char* label) {
    const ImRect rect = Rect(label);
    Tap(ImVec2(rect.Max.x - 2.0f, rect.GetCenter().y));
}
} // namespace Drive
'''

# ---- 4. The MM General tab, drawn from OoT with the bundled Dear ImGui.
mm_general = function(mm_editor, 'char z2ASCII(int code) {') + function(mm_editor, 'char ASCII2z(int code) {') + \
    function(mm_editor, 'char* getPlayerName(char* playerNameBuf) {') + \
    function(mm_editor, 'int setPlayerName(ImGuiInputTextCallbackData* data) {') + \
    function(mm_editor, 'void DrawTempleClears() {') + function(mm_editor, 'static void DrawOwlStatuesEditor() {') + \
    function(mm_editor, 'void DrawGeneralTab() {')
assert 'Kit::BeginCard("progressCard")' in mm_general, 'not the combo MM General tab'
program = imgui_driver + r'''
#include "gui/ComboSaveEditorAccess.h"
#include "gui/ComboSaveEditorKit.h"
#include "../mm/2s2h/DeveloperTools/OwlStatueEditor.h"
#include <array>
#include <cstring>
#include <iostream>
typedef unsigned char u8;
typedef signed char s8;
typedef short s16;
typedef unsigned short u16;
typedef int s32;
typedef unsigned int u32;
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define CLAMP(x, lo, hi) ((x) < (lo) ? (lo) : (x) > (hi) ? (hi) : (x))
#define ARRAY_COUNT(arr) (s32)(sizeof(arr) / sizeof(arr[0]))
enum { SAVETYPE_VANILLA, SAVETYPE_RANDO };
enum { FILE_NUM_MAX = 3 };
enum OwlWarpId { OWL_WARP_GREAT_BAY_COAST, OWL_WARP_ZORA_CAPE, OWL_WARP_SNOWHEAD, OWL_WARP_MOUNTAIN_VILLAGE,
                 OWL_WARP_CLOCK_TOWN, OWL_WARP_MILK_ROAD, OWL_WARP_WOODFALL, OWL_WARP_SOUTHERN_SWAMP,
                 OWL_WARP_IKANA_CANYON, OWL_WARP_STONE_TOWER, OWL_WARP_ENTRANCE };
struct Save {
    s32 time; // u16 in MM; CURRENT_TIME reads it
    s32 day, eventDayCount, timeSpeedOffset;
    u8 hasTatl, isOwlSave, isFirstCycle;
    struct {
        struct {
            u8 playerName[8];
            u8 doubleDefense;
            s16 healthCapacity, health;
            s8 magicLevel, magic;
            u8 isMagicAcquired, isDoubleMagicAcquired;
            s16 rupees;
            u16 owlActivationFlags;
        } playerData;
        struct { u8 defenseHearts; u32 questItems; u32 upgrades; } inventory;
        u8 bomberCode[5];
        u8 weekEventReg[100];
        s32 bankRupees;
        u8 buttonItems[4][4];
    } saveInfo;
    struct { int saveType; } shipSaveInfo;
};
struct SaveContext {
    s16 fileNum;
    s16 magicCapacity;
    Save save;
};
SaveContext gSaveContext;
struct PlayState { int sceneId, numSetupActors; struct { int lightningState; } envCtx; };
PlayState* gPlayState = nullptr;
int gWeatherMode;
enum { WEATHER_MODE_CLEAR, LIGHTNING_OFF, SCENE_F40 = 0x5A, UPG_WALLET = 0, PLAYER_FORM_DEKU = 3, EQUIP_SLOT_B = 0 };
enum { ITEM_DEKU_NUT = 9, MAGIC_NORMAL_METER = 0x30, MAGIC_DOUBLE_METER = 0x60 };
enum { RO_SHUFFLE_TYCOON_WALLET, RO_GENERIC_NO = 0, RO_GENERIC_YES };
int gRandoOptions[4];
#define RANDO_SAVE_OPTIONS gRandoOptions
#define IS_RANDO (gSaveContext.save.shipSaveInfo.saveType == SAVETYPE_RANDO)
#define PACK(index, mask) (((index) << 8) | (mask))
#define CHECK_WEEKEVENTREG(flag) (gSaveContext.save.saveInfo.weekEventReg[(flag) >> 8] & ((flag) & 0xFF))
#define SET_WEEKEVENTREG(flag) (gSaveContext.save.saveInfo.weekEventReg[(flag) >> 8] |= ((flag) & 0xFF))
#define CLEAR_WEEKEVENTREG(flag) (gSaveContext.save.saveInfo.weekEventReg[(flag) >> 8] &= ~((flag) & 0xFF))
enum {
    WEEKEVENTREG_20_01 = PACK(20, 0x01), WEEKEVENTREG_30_01 = PACK(30, 0x01), WEEKEVENTREG_53_20 = PACK(53, 0x20),
    WEEKEVENTREG_93_08 = PACK(93, 0x08), WEEKEVENTREG_CLEARED_WOODFALL_TEMPLE = PACK(7, 0x80),
    WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE = PACK(33, 0x80), WEEKEVENTREG_CLEARED_GREAT_BAY_TEMPLE = PACK(55, 0x80),
    WEEKEVENTREG_CLEARED_STONE_TOWER_TEMPLE = PACK(52, 0x20), WEEKEVENTREG_RECEIVED_GREAT_SPIN_ATTACK = PACK(23, 0x02),
    WEEKEVENTREG_DRANK_CHATEAU_ROMANI = PACK(14, 0x08),
};
#define CUR_UPG_VALUE(upg) (gSaveContext.save.saveInfo.inventory.upgrades & 3)
#define CUR_CAPACITY(upg) (99 + 100 * CUR_UPG_VALUE(upg))
void Inventory_ChangeUpgrade(int, int level) { gSaveContext.save.saveInfo.inventory.upgrades = level; }
#define HS_GET_BANK_RUPEES() (gSaveContext.save.saveInfo.bankRupees)
#define HS_SET_BANK_RUPEES(rupees) (gSaveContext.save.saveInfo.bankRupees = (rupees))
#define CURRENT_TIME ((u16)gSaveContext.save.time)
#define CURRENT_DAY (gSaveContext.save.day)
#define CLOCK_TIME(hr, min) ((s32)(((hr) * 60 + (min)) * 0x10000 / (24 * 60)))
#define BUTTON_ITEM_EQUIP(form, button) (gSaveContext.save.saveInfo.buttonItems[form][button])
int gPlayStateTouched = 0;
void UpdateGameTime(u16 time) { gSaveContext.save.time = time; }
void Interface_NewDay(PlayState*, s32) { ++gPlayStateTouched; }
void Environment_NewDay(void*) { ++gPlayStateTouched; }
int Play_GetOriginalSceneId(int scene) { ++gPlayStateTouched; return scene; }
bool Flags_GetSwitch(PlayState*, int) { ++gPlayStateTouched; return false; }
void Flags_SetSwitch(PlayState*, int) { ++gPlayStateTouched; }
void Flags_UnsetSwitch(PlayState*, int) { ++gPlayStateTouched; }
constexpr uint16_t HEART_COUNT_MIN = 3;
constexpr uint16_t HEART_COUNT_MAX = 20;
constexpr int8_t MAGIC_LEVEL_MAX = 2;
const char* WALLET_LEVEL_NAMES[4] = { "Child Wallet", "Adult Wallet", "Giant Wallet", "Tycoon Wallet" };
constexpr u8 WALLET_LEVEL_MAX = 2;
int CVarGetInteger(const char*, int fallback) { return fallback; }

// MM's UIWidgets, as the editor calls them: the same option layouts, drawing Dear ImGui's own widgets.
namespace UIWidgets {
enum Colors { Red, DarkRed, Orange, Green, DarkGreen, LightBlue, Blue, DarkBlue, Indigo, Violet, Purple, Brown, Gray,
              DarkGray, Pink, Yellow, Cyan, Black, LightGray, White, NoColor };
const std::map<Colors, ImVec4> ColorValues = [] {
    std::map<Colors, ImVec4> values;
    for (int colour = Red; colour <= NoColor; ++colour)
        values[Colors(colour)] = ImVec4(0.2f, 0.3f, 0.4f, 1.0f);
    return values;
}();
enum class ComponentAlignment { Left, Right };
enum class LabelPosition { Near, Far, Above, None, Within };
namespace Sizes {
const ImVec2 Inline = ImVec2(0.0f, 0.0f);
const ImVec2 Fill = ImVec2(-1.0f, 0.0f);
} // namespace Sizes
struct WidgetOptions {
    const char* tooltip = "";
    bool disabled = false;
    const char* disabledTooltip = "";
    Colors color = Colors::NoColor;
};
struct ButtonOptions : WidgetOptions {
    ImVec2 size = Sizes::Fill;
    ImVec2 padding = ImVec2(10.0f, 8.0f);
    Colors color = Colors::Gray;
};
struct CheckboxOptions : WidgetOptions {
    bool defaultValue = false;
    ComponentAlignment alignment = ComponentAlignment::Left;
    LabelPosition labelPosition = LabelPosition::Near;
    ImVec2 padding = ImVec2(10.0f, 8.0f);
    Colors color = Colors::LightBlue;
};
bool Checkbox(const char* label, bool* value, const CheckboxOptions& options = {}) {
    ImGui::BeginDisabled(options.disabled);
    const bool pressed = ImGui::Checkbox(label, value);
    ImGui::EndDisabled();
    return pressed;
}
bool Button(const char* label, const ButtonOptions& options = {}) {
    ImGui::BeginDisabled(options.disabled);
    const bool pressed = ImGui::Button(label);
    ImGui::EndDisabled();
    return pressed;
}
void Tooltip(const char*) {}
void PushStyleTabs(Colors) {}
void PopStyleTabs() {}
} // namespace UIWidgets
using namespace UIWidgets;

int gWrites = 0;
void SaveManager_SaveCurrentForCombo() { ++gWrites; } // the writer itself is program 1's
bool gMmInFront = false;
bool Combo_MmIsForeground(void) { return gMmInFront; }
int OotPlaysFile2(int game) { return game == ComboSaveEditorAccess::kMm ? 1 : -1; }
int (*gMMComboDormantSaveEditSlot)(int game) = OotPlaysFile2;
void initSafeItemsForInventorySlot() {}
void DrawItemsAndMasksTab() {}
void DrawEquipmentTab() {}
void DrawQuestStatusTab() {}
void DrawDungeonItemTab() {}
void DrawFlagsTab() {}
void DrawPlayerTab() {}
void DrawRandoTab() {}
void DrawRegEditorTab() {}
struct SaveEditorWindow {
    void DrawElement();
    void UpdateElement();
};
''' + mm_state + function(mm_editor, 'static int MmSaveMagicLevel() {') + mm_general + mm_element + r'''
const u8 kName[8] = { 10, 63, 11, 62, 62, 62, 62, 62 }; // "A-B": 63 does not survive the text round trip
void LoadFile() {
    std::memset(&gSaveContext, 0, sizeof(gSaveContext));
    gSaveContext.fileNum = 1;
    gSaveContext.magicCapacity = 0; // MM's runtime meter: only MM's play sets it
    Save& save = gSaveContext.save;
    save.shipSaveInfo.saveType = SAVETYPE_RANDO;
    std::memcpy(save.saveInfo.playerData.playerName, kName, sizeof(kName));
    save.saveInfo.playerData.healthCapacity = save.saveInfo.playerData.health = 0x30;
    save.saveInfo.playerData.magicLevel = 1;
    save.saveInfo.playerData.isMagicAcquired = 1;
    save.time = 0x4000;
    save.day = 1;
    // Every temple cleared, none of them opened.
    for (int flag : { WEEKEVENTREG_CLEARED_WOODFALL_TEMPLE, WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE,
                      WEEKEVENTREG_CLEARED_GREAT_BAY_TEMPLE, WEEKEVENTREG_CLEARED_STONE_TOWER_TEMPLE })
        SET_WEEKEVENTREG(flag);
}
bool Unchanged(const Save& before) {
    return std::memcmp(&before, &gSaveContext.save, sizeof(Save)) == 0;
}

int main() {
    Drive::Setup();
    Drive::gDraw = [] {
        SaveEditorWindow window;
        window.UpdateElement();
        window.DrawElement();
    };
    LoadFile();
    PlayState stale{}; // the play state MM's eager boot can leave behind
    gPlayState = &stale;
    Save before = gSaveContext.save;
    // From OoT: drawing the General tab writes nothing.
    Drive::Frames(3);
    assert(Drive::Drawn("Player Name") || Drive::Drawn("##playerNameInput"));
    assert(gWrites == 0 && Unchanged(before));
    // Opening Progress with every temple cleared: the open flags are shown, not set.
    Drive::Tap("Progress (temples, owl statues)");
    Drive::Frames(2);
    assert(Drive::Drawn("Woodfall Open") && Drive::Drawn("Clock Town"));
    assert(gWrites == 0 && Unchanged(before) && gPlayStateTouched == 0);
    // Focusing the name field and leaving it: the name is not rewritten.
    Drive::Tap("##playerNameInput");
    assert(ImGui::GetActiveID() == Drive::gIds.at("##playerNameInput"));
    Drive::Frames(3);
    Drive::Tap(Drive::kAway);
    Drive::Frames(2);
    assert(ImGui::GetActiveID() == 0);
    assert(gWrites == 0 && Unchanged(before));
    // "Is Owl Save" is MM's to change.
    Drive::Tap("Is Owl Save");
    assert(gWrites == 0 && Unchanged(before));
    // Ticking "cleared" opens the temple, as the edit it is.
    Drive::Tap("Woodfall cleared");
    assert(!CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_WOODFALL_TEMPLE) && !CHECK_WEEKEVENTREG(WEEKEVENTREG_20_01));
    assert(gWrites == 1);
    Drive::Tap("Woodfall cleared");
    assert(CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_WOODFALL_TEMPLE) && CHECK_WEEKEVENTREG(WEEKEVENTREG_20_01));
    assert(!CHECK_WEEKEVENTREG(WEEKEVENTREG_30_01) && gWrites == 2);
    // The magic bar is the save's level's: single magic holds 48 (MM's runtime meter says 0 here).
    Drive::TapEnd("##magicSlider");
    assert(gSaveContext.save.saveInfo.playerData.magic > 40 && gSaveContext.save.saveInfo.playerData.magic <= 0x30);
    assert(gWrites == 3);
    // The save just loaded: magicLevel 0 until MM's next frame derives it. The bar still holds the save's meter.
    gSaveContext.save.saveInfo.playerData.magicLevel = 0;
    gSaveContext.save.saveInfo.playerData.isDoubleMagicAcquired = 1;
    gSaveContext.save.saveInfo.playerData.magic = 0;
    Drive::Frames(2);
    assert(gWrites == 3);
    Drive::TapEnd("##magicSlider");
    assert(gSaveContext.save.saveInfo.playerData.magic > 0x50 && gWrites == 4);
    assert(gPlayStateTouched == 0);

    // While MM runs, the tab changes nothing either (MM would save it).
    LoadFile();
    gMmInFront = true;
    gPlayState = nullptr;
    before = gSaveContext.save;
    Drive::gIds.clear();
    Drive::Frames(3);
    assert(Drive::Drawn("Snowhead Open"));
    assert(Unchanged(before));
    std::cout << "PASS the MM General tab writes nothing it was not asked to, from OoT and in MM\n";
    ImGui::DestroyContext();
}
'''
run('mm_general_tab_drawn', program, [root / 'combo', root / 'combo/gui', imgui],
    defines=('COMBO_BUILD', 'IMGUI_ENABLE_TEST_ENGINE'), sources=imgui_sources)

# ---- 5. The OoT General tab, drawn from MM after OoT parked for MM's resume, with the bundled Dear ImGui.
file_choose = read('soh/src/overlays/gamestates/ovl_file_choose/z_file_choose.c')
staging_start = hostcxx.find(file_choose, 'gSaveContext.forceRisingButtonAlphas = gSaveContext.nextHudVisibilityMode')
staging = file_choose[staging_start:file_choose.index('gSaveContext.magicLevel = gSaveContext.magic;', staging_start)] + \
    'gSaveContext.magicLevel = gSaveContext.magic;\n'
assert 'gSaveContext.magic = 0;' in staging, "not FileChoose_LoadGame's magic staging"
# The park runs from the OnLoadGame hook FileChoose_LoadGame fires after it staged the meter.
assert hostcxx.find(file_choose, 'gSaveContext.magicLevel = gSaveContext.magic;') < \
    hostcxx.find(file_choose, 'GameInteractor_ExecuteOnLoadGame(gSaveContext.fileNum);', staging_start)
park = function(otr, 'extern "C" COMBO_EXPORT void SOH_ParkForComboMMResume(void) {')
grant = function(otr, 'void Combo_GrantResolvedOOT(const GetItemEntry& gie) {') + \
    function(otr, 'extern "C" COMBO_EXPORT void SOH_GrantCrossItem(const char* itemName) {')
oot_general = function(oot_editor, 'char z2ASCII(int code) {') + function(oot_editor, 'static uint8_t ASCII2z(char c) {') + \
    function(oot_editor, 'std::string decodeNTSCPlayerNameChar(int code) {') + \
    statement(oot_editor, 'std::map<uint8_t, const char*> audioMap') + \
    statement(oot_editor, 'std::map<uint8_t, const char*> zTargetMap') + \
    statement(oot_editor, 'std::map<uint8_t, const char*> filenameLanguageMap') + \
    statement(oot_editor, 'std::map<uint8_t, const char*> filenameLanguageMapNTSCOnly') + \
    function(oot_editor, 'static std::string PlayerNameChar(uint8_t code, bool pal) {') + \
    function(oot_editor, 'static uint8_t PlayerNameCode(const std::string& typed, bool pal) {') + \
    function(oot_editor, 'static std::string NextPlayerNameChar(const char*& text) {') + \
    function(oot_editor, 'static void ApplyPlayerNameText(const char* text, bool pal) {') + \
    function(oot_editor, 'static int KeepEightPlayerNameChars(ImGuiInputTextCallbackData* data) {') + \
    function(oot_editor, 'static int OotSaveMagicLevel() {') + function(oot_editor, 'void DrawInfoTab() {')
assert 'Kit::BeginCard("advancedCard")' in oot_general, 'not the combo OoT General tab'
program = imgui_driver + r'''
#include "gui/ComboSaveEditorAccess.h"
#include "gui/ComboSaveEditorKit.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <memory>
typedef unsigned char u8;
typedef signed char s8;
typedef short s16;
typedef unsigned short u16;
typedef int s32;
typedef unsigned int u32;
typedef float f32;
#define COMBO_EXPORT
#define SQ(x) ((x) * (x))
#define SPDLOG_INFO(...) ((void)0)
#define SPDLOG_WARN(...) ((void)0)
enum { GAMEMODE_NORMAL, GAMEMODE_TITLE_SCREEN, GAMEMODE_FILE_SELECT };
enum { NAME_LANGUAGE_PAL, NAME_LANGUAGE_NTSC_JPN, NAME_LANGUAGE_NTSC_ENG };
enum MagicLevel { MAGIC_LEVEL_NONE, MAGIC_LEVEL_SINGLE, MAGIC_LEVEL_DOUBLE };
enum AudioOutput { AUDIO_STEREO, AUDIO_MONO, AUDIO_HEADSET, AUDIO_SURROUND };
enum ZTarget { Z_TARGET_SWITCH, Z_TARGET_HOLD };
enum { MAGIC_NORMAL_METER = 0x30, UPG_WALLET = 0, QUEST_HEART_PIECE = 24 };
struct SaveContext {
    s32 fileNum, gameMode;
    s32 entranceIndex, cutsceneIndex, totalDays;
    u16 dayTime, deaths, naviTimer;
    u8 playerName[8];
    s16 healthCapacity, health;
    s8 magicLevel, magic;
    s16 rupees;
    u8 isMagicAcquired, isDoubleMagicAcquired, isDoubleDefenseAcquired, bgsFlag;
    struct { u8 defenseHearts; u32 questItems; u32 upgrades; } inventory;
    s32 highScores[7];
    s16 timerState, timerSeconds, subTimerState, subTimerSeconds;
    u8 audioSetting, n64ddFlag, zTargetSetting;
    s16 magicCapacity, magicFillTarget, healthAccumulator;
    u8 forceRisingButtonAlphas, nextHudVisibilityMode, hudVisibilityMode;
    s16 hudVisibilityModeTimer;
    struct { u8 filenameLanguage; u8 pendingIceTrapCount; } ship;
};
SaveContext gSaveContext;
struct PlayState {};
PlayState* gPlayState = nullptr;
struct GameState { void (*init)(GameState*); bool running; };
void PlayInit(GameState*) {}
GameState gFileChoose{ PlayInit, true };
GameState* gGameState = &gFileChoose;
#define IS_RANDO true
#define CUR_UPG_VALUE(upg) (gSaveContext.inventory.upgrades & 3)
#define CUR_CAPACITY(upg) (99 + 100 * CUR_UPG_VALUE(upg))
void Inventory_ChangeUpgrade(int, int level) { gSaveContext.inventory.upgrades = level; }
enum RandomizerSettingKey { RSK_INCLUDE_TYCOON_WALLET };
struct Randomizer { int GetRandoSettingValue(RandomizerSettingKey) { return 0; } };
struct OTRGlobals {
    static OTRGlobals* Instance;
    ImFont* fontJapanese = nullptr;
    std::shared_ptr<Randomizer> gRandomizer = std::make_shared<Randomizer>();
};
OTRGlobals gGlobals;
OTRGlobals* OTRGlobals::Instance = &gGlobals;
void* sGerMessageEntryTablePtr = &gGlobals;
void* sFraMessageEntryTablePtr = &gGlobals;
void* sJpnMessageEntryTablePtr = &gGlobals;
int CVarGetInteger(const char*, int fallback) { return fallback; }
#define CVAR_SETTING(name) name

// SoH's UIWidgets, as the editor calls them: the same option builders, drawing Dear ImGui's own widgets.
namespace UIWidgets {
enum Colors { Red, DarkRed, Orange, Green, DarkGreen, LightBlue, Blue, DarkBlue, Indigo, Violet, Purple, Brown, Gray,
              DarkGray, Pink, Yellow, Cyan, Black, LightGray, White, NoColor };
const std::map<Colors, ImVec4> ColorValues = [] {
    std::map<Colors, ImVec4> values;
    for (int colour = Red; colour <= NoColor; ++colour)
        values[Colors(colour)] = ImVec4(0.2f, 0.3f, 0.4f, 1.0f);
    return values;
}();
struct CheckboxOptions {
    CheckboxOptions& Tooltip(std::string) { return *this; }
    CheckboxOptions& Color(Colors) { return *this; }
};
struct ComboboxOptions {
    ComboboxOptions& Tooltip(std::string) { return *this; }
};
bool Checkbox(const char* label, bool* value, const CheckboxOptions& = {}) {
    return ImGui::Checkbox(label, value);
}
template <typename T>
bool Combobox(std::string label, T* value, const std::map<T, const char*>& comboMap, const ComboboxOptions& = {}) {
    bool picked = false;
    if (ImGui::BeginCombo(("##" + label).c_str(), comboMap.at(*value))) {
        for (const auto& [key, name] : comboMap)
            if (ImGui::Selectable(name, key == *value)) {
                *value = key;
                picked = true;
            }
        ImGui::EndCombo();
    } else {
        Drive::gIds["##" + label] = ImGui::GetItemID(); // Dear ImGui reports no label for a combo
    }
    return picked;
}
void Tooltip(std::string) {}
void PushStyleInput(Colors) {}
void PopStyleInput() {}
void PushStyleTabs(Colors) {}
void PopStyleTabs() {}
} // namespace UIWidgets
using namespace UIWidgets;
#define THEME_COLOR UIWidgets::Colors::LightBlue
CheckboxOptions checkboxOptionsBase;
ComboboxOptions comboboxOptionsBase;
void ResetBaseOptions() {}

// OoT's writer, recording what it saved; the cross-game grant's item table and give.
struct SaveManager {
    static SaveManager* Instance;
    int writes = 0, file = -1, magic = -1, magicLevel = -1;
    void SaveFile(int fileNum) {
        ++writes;
        file = fileNum;
        magic = gSaveContext.magic;
        magicLevel = gSaveContext.magicLevel;
    }
};
SaveManager gSaves;
SaveManager* SaveManager::Instance = &gSaves;
enum ModIndex { MOD_NONE, MOD_RANDOMIZER };
enum RandomizerGet { RG_NONE, RG_DEKU_NUTS_5, RG_MAGIC_SINGLE, RG_MAGIC_DOUBLE, RG_ICE_TRAP };
enum { ITEM_POTION_RED = 20, ITEM_POE = 30, ITEM_MILK = 31, ITEM_MILK_BOTTLE = 32, ITEM_LETTER_RUTO = 33 };
enum { GI_SWORD_BGS = 1, GID_HEART_CONTAINER = 1, GID_HEART_PIECE = 2 };
struct GetItemEntry { int modIndex, itemId, getItemId, gid; };
namespace Rando::StaticData {
std::map<std::string, RandomizerGet> itemNameToEnum = { { "Deku Nuts (5)", RG_DEKU_NUTS_5 } };
struct Item {
    RandomizerGet rg;
    GetItemEntry GetGIEntry_Copy() const { return { MOD_RANDOMIZER, 0, rg, 0 }; }
};
Item RetrieveItem(RandomizerGet rg) { return { rg }; }
} // namespace Rando::StaticData
bool Inventory_HasEmptyBottle() { return true; }
void Item_Give(PlayState*, u8) {}
void Randomizer_Item_Give(PlayState*, GetItemEntry gie) { // randomizer.cpp's magic upgrades
    if (gie.getItemId == RG_MAGIC_DOUBLE) {
        gSaveContext.isMagicAcquired = true;
        gSaveContext.isDoubleMagicAcquired = true;
        gSaveContext.magicFillTarget = 0x60;
        gSaveContext.magicLevel = 0;
    }
}
void Combo_ApplyItemReceiveSideEffects(const GetItemEntry&) {}
''' + grant + 'extern "C" ' + park.replace('extern "C" COMBO_EXPORT ', '') + r'''

bool gOotInFront = false;
bool Combo_OotIsForeground(void) { return gOotInFront; }
int MmRunsFile2(int game) { return game == ComboSaveEditorAccess::kOot ? 1 : -1; }
int (*gComboSaveEditorDormantSlot)(int game) = MmRunsFile2;
void DrawInventoryTab() {}
void DrawEquipmentTab() {}
void DrawQuestStatusTab() {}
void DrawDungeonItemsTab() {}
void DrawFlagsTab() {}
void DrawPlayerTab() {}
struct SaveEditorWindow {
    void DrawElement();
    void UpdateElement();
};
''' + oot_state + oot_general + oot_element + r'''
void LoadFile() { // Sram_TryOpenSave: file 2 as saved
    std::memset(&gSaveContext, 0, sizeof(gSaveContext));
    gSaveContext.fileNum = 1;
    gSaveContext.gameMode = GAMEMODE_NORMAL;
    gSaveContext.ship.filenameLanguage = NAME_LANGUAGE_PAL;
    gSaveContext.healthCapacity = 0x30;
    gSaveContext.health = 0x40; // more than the capacity: only an edit of it may clamp it
    gSaveContext.magicLevel = 1;
    gSaveContext.magic = 0x30;
    gSaveContext.isMagicAcquired = 1;
    gSaveContext.dayTime = 0x8000;
    for (u8& code : gSaveContext.playerName)
        code = 62;
}
void FileChoose_LoadGameStaging() {
''' + staging + r'''
}
bool Unchanged(const SaveContext& before) {
    return std::memcmp(&before, &gSaveContext, sizeof(SaveContext)) == 0;
}

int main() {
    Drive::Setup();
    Drive::gDraw = [] {
        SaveEditorWindow window;
        window.UpdateElement();
        window.DrawElement();
    };
    // File 2 was last saved in MM: OoT loads it from its file screen, stages the meter, and parks for MM.
    LoadFile();
    FileChoose_LoadGameStaging();
    assert(gSaveContext.magic == 0 && gSaveContext.magicLevel == 0 && gSaveContext.magicFillTarget == 0x30);
    SOH_ParkForComboMMResume();
    assert(gGameState->init == nullptr && !gGameState->running);
    assert(gSaveContext.magic == 0x30 && gSaveContext.magicLevel == 1 && gSaveContext.magicCapacity == 0x30);
    // From MM: drawing the General tab, opening Advanced and Minigames writes nothing.
    SaveContext before = gSaveContext;
    Drive::Frames(3);
    assert(gSaves.writes == 0 && Unchanged(before));
    Drive::Tap("Advanced");
    Drive::Frames(2);
    assert(Drive::Drawn("Navi Timer") && Drive::Drawn("Minigames"));
    Drive::Tap("Minigames");
    Drive::Frames(2);
    assert(Drive::Drawn("Horseback Archery"));
    assert(gSaves.writes == 0 && Unchanged(before));
    // Audio is a setting of the game, not of the file: not from MM.
    Drive::Tap("##Audio");
    assert(ImGui::GetCurrentContext()->OpenPopupStack.Size == 0 && gSaves.writes == 0 && Unchanged(before));
    // The Navi Timer is in the file, and kept when OoT next starts from MM.
    Drive::TapEnd("Navi Timer");
    assert(gSaveContext.naviTimer > 2000 && gSaves.writes == 1 && gSaves.file == 1);
    // A cross-game item for OoT arrives while MM runs: the file keeps its real magic.
    SOH_GrantCrossItem("Deku Nuts (5)");
    assert(gSaves.writes == 2 && gSaves.magic == 0x30 && gSaves.magicLevel == 1);
    // The double meter arrives the same way, zeroing magicLevel as OoT's own give does: the editor still sizes the
    // magic bar from the level the save gives Link.
    SOH_GrantCrossItem("Progressive Magic Meter");
    assert(gSaves.writes == 3 && gSaveContext.magicLevel == 0 && gSaveContext.isDoubleMagicAcquired);
    Drive::Frames(2);
    assert(gSaves.writes == 3);
    Drive::TapEnd("##magic");
    assert(gSaveContext.magic > 0x50 && gSaves.writes == 4);

    // While OoT runs, the tab changes nothing either (OoT would save it): no clamp, no meter size.
    LoadFile();
    gSaveContext.magicCapacity = 0x10; // mid-way through OoT's own meter animation
    gOotInFront = true;
    before = gSaveContext;
    Drive::gIds.clear();
    Drive::Frames(3);
    assert(Drive::Drawn("Navi Timer"));
    assert(Unchanged(before) && gSaves.writes == 4);
    std::cout << "PASS the OoT General tab writes nothing it was not asked to, and parked OoT keeps its magic\n";
    ImGui::DestroyContext();
}
'''
run('oot_general_tab_drawn', program, [root / 'combo', root / 'combo/gui', imgui],
    defines=('COMBO_BUILD', 'IMGUI_ENABLE_TEST_ENGINE'), sources=imgui_sources)
print('Save Editor edits of the game not running reach the played file, drawing writes nothing, and MM songs and '
      'slot lists are fixed.')
