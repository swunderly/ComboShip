"""Run the real console "reset" path from MM: Restart game, Ctrl+R and the console land on OoT's title, as in 0.4.1.

Shared console commands follow the foreground game (ComboConsoleCommands.h), so a reset with MM in the foreground runs
MM's own ResetHandler, which only requests MM's reset return; OoT's Combo_HandleReset, which 0.4.1 ran and which set
OoT's boot-to-title flag, no longer sees it. This builds both games' reset handlers, OoT's reset flag and its consumer
in SOH_ResumeGame, and MM's return hook (BenPort.cpp OnGameStateMainStart) from production text and checks every
return kind: a reset and an owl save quit resume OoT on its title (gComboReturnFileNum -1), a portal return resumes
the slot, a reset in OoT runs OoT's own reset, and the 0.4.1 registration order still lands on the title.

Restart game says "Unsaved progress will be lost": a reset saves neither game, even with autosave on. MM's return
hook saves only a portal return, and OoT's reset skips the soft-reset autosave (Autosave.cpp, built from production
text too) that its other exits keep.
"""
from pathlib import Path
import argparse
import re
import subprocess
import hostcxx

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
p.add_argument('--imgui-include', type=Path, required=True)
a = p.parse_args()
source = a.source.resolve()
work = a.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)


def block(text, start):
    """text[start:] up to the brace that closes the first '{' after start."""
    opening = text.index('{', start)
    masked = re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"', lambda m: ' '*len(m.group()), text)
    depth = 0
    for end in range(opening, len(text)):
        depth += (masked[end] == '{') - (masked[end] == '}')
        if depth == 0:
            return text[start:end+1]
    raise AssertionError('Unterminated production block')


def function(text, signature):
    return block(text, hostcxx.find(text, signature))


console = (source/'libultraship/src/ship/debug/Console.cpp').read_text(encoding='utf-8')
registry = '\n'.join(function(console, signature) for signature in (
    'void Console::AddCommand(', 'bool Console::HasCommand(', 'CommandEntry& Console::GetCommand('))

oot_console = (source/'soh/soh/Enhancements/debugconsole.cpp').read_text(encoding='utf-8')
mm_console = (source/'mm/2s2h/DeveloperTools/DebugConsole.cpp').read_text(encoding='utf-8')
for text in (oot_console, mm_console):
    assert '#define CMD_REGISTER RegisterComboCommand' in text
    assert hostcxx.contains(text, 'CMD_REGISTER("reset", { ResetHandler,')
otr = (source/'soh/soh/OTRGlobals.cpp').read_text(encoding='utf-8')
resume = function(otr, 'extern "C" COMBO_EXPORT void SOH_ResumeGame(void)')
consume = re.search(r'gComboReturnFileNum\s*=\s*sComboResetPending\s*\?[^;]*;\s*sComboResetPending\s*=\s*false;', resume)
assert consume, 'SOH_ResumeGame no longer turns the reset flag into a title boot'
benport = (source/'mm/2s2h/BenPort.cpp').read_text(encoding='utf-8')
hook = hostcxx.find(benport, 'GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameStateMainStart>([]() {')
main_start = block(benport, benport.index('[]()', hook))
assert 'gComboReturnCallback(' in main_start and 'SOH_SetComboBootToTitle' in main_start
autosave = (source/'soh/soh/Enhancements/QoL/Autosave.cpp').read_text(encoding='utf-8')
assert hostcxx.contains(autosave, 'COND_HOOK(GameInteractor::OnExitGame, CVAR_AUTOSAVE_VALUE, [](int32_t fileNum) { '
                                  'Autosave_SoftResetSave(); });')
discard = re.search(r'bool gComboResetDiscardsProgress = false;', autosave)
assert discard, 'Autosave.cpp no longer defines the reset flag'

cpp = work/'combo_console_reset.cpp'
cpp.write_text(r'''
#include "ComboGameMode.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>
#include <stdexcept>
#include "ComboConsoleCommands.h"
#define COMBO_EXPORT
#define SPDLOG_WARN(...) ((void)0)
#define COMBO_BUILD 1
typedef int s32;
int errors = 0;
#define ERROR_MESSAGE(...) (++errors)
#define SET_NEXT_GAMESTATE(state, init, type) ((state)->next = #init)
void* ResolveForTest(const char* module, const char* name);
namespace Ship {
Console::Console() = default;
Console::~Console() = default;
''' + registry + r'''
}
// MM's return hook and its request flags (BenPort.cpp).
namespace MmPort {
int gMMComboGameMode = COMBO_MODE_COMBINED;
inline void* Combo_ResolveSym(const char* module, const char* name) { return ResolveForTest(module, name); }
namespace Ship {
struct Window { virtual ~Window() = default; };
struct Context {
    std::shared_ptr<Window> window;
    std::shared_ptr<Window> GetWindow() { return window; }
    static Context* GetRawInstance() { static Context context; return &context; }
};
}
namespace Fast {
struct Fast3dWindow : Ship::Window { bool running = true; void SetIsRunning(bool value) { running = value; } };
}
constexpr int GAMEMODE_NORMAL = 0;
struct SaveContext { int gameMode = GAMEMODE_NORMAL; } gSaveContext;
void* gPlayState = nullptr;
int sSongDeparture = -1, autosave = 0, saves = 0, cancels = 0, dispatches = 0;
void ProcessForeignSongReply() {}
bool MM_FaroreDeparturePending() { return false; }
int CVarGetInteger(const char* name, int fallback) {
    return std::strcmp(name, "gEnhancements.Saving.Autosave") ? fallback : autosave;
}
void SaveManager_SaveCurrentForCombo() { ++saves; }
bool MM_DispatchNativeDeparture(void*) { ++dispatches; return true; }
void MM_CancelNativeDeparture() { ++cancels; }
void (*gComboReturnCallback)(int kind) = nullptr;
static bool sComboReturnPending = false;
static bool sComboResetReturnPending = false;
static bool sComboOwlSaveQuitPending = false;
''' + function(benport, 'extern "C" COMBO_EXPORT void MM_RequestComboReturn(void)') + '\n'
    + function(benport, 'extern "C" void Combo_RequestOwlSaveQuit(void)') + '\n'
    + 'auto MainStart = ' + main_start + r''';
}
// MM's console reset (DebugConsole.cpp).
namespace Mm {
using MmPort::MM_RequestComboReturn;
struct GameState {} state;
GameState* gGameState = &state;
static ComboConsole::Commands sComboConsoleCommands;
''' + function(mm_console, 'extern "C" COMBO_EXPORT const Ship::CommandEntry* MM_GetConsoleCommandForCombo(') + '\n'
    + function(mm_console, 'static bool ResetHandler(') + r'''
}
// OoT's console reset (debugconsole.cpp), its reset flag and SOH_ResumeGame's use of it (OTRGlobals.cpp).
namespace Oot {
inline void* Combo_ResolveSym(const char* module, const char* name) { return ResolveForTest(module, name); }
struct GameState { bool running = true; const char* next = nullptr; };
GameState* gGameState = nullptr;
struct SaveContext { int fileNum = 0; } gSaveContext;
s32 gComboReturnFileNum = -1;
// OoT's autosave hook on OnExitGame (Autosave.cpp), registered while autosave is on.
int autosave = 0, saves = 0;
static bool Autosave_CanSave() { return true; }
static void Autosave_PerformSave() { ++saves; }
''' + discard.group(0) + '\n' + function(autosave, 'static void Autosave_SoftResetSave()') + r'''
struct GameInteractor {
    struct OnExitGame {};
    int exits = 0;
    template <typename Hook, typename... Args> void ExecuteHooks(Args...) {
        ++exits;
        if (autosave)
            Autosave_SoftResetSave();
    }
    static GameInteractor* Instance;
};
GameInteractor interactor;
GameInteractor* GameInteractor::Instance = &interactor;
static bool sComboResetPending = false;
''' + function(otr, 'bool Combo_OotIsForeground(void) {') + '\n'
    + function(otr, 'extern "C" COMBO_EXPORT void SOH_SetComboBootToTitle(void)') + '\n'
    + function(otr, 'bool Combo_HandleReset(void)') + '\n'
    + 'void ResumeOot() { ' + consume.group(0) + ' }\n'
    + 'static ComboConsole::Commands sComboConsoleCommands;\n'
    + function(oot_console, 'extern "C" COMBO_EXPORT const Ship::CommandEntry* SOH_GetConsoleCommandForCombo(') + '\n'
    + function(oot_console, 'static bool ResetHandler(') + r'''
}
int foreground = 0;
int Foreground() { return foreground; }
void* ResolveForTest(const char* module, const char* name) {
    const auto is = [&](const char* m, const char* n) { return !std::strcmp(module, m) && !std::strcmp(name, n); };
    if (is("comboui", "ComboUI_GetForegroundGame"))
        return reinterpret_cast<void*>(Foreground);
    if (is("soh", "SOH_GetConsoleCommandForCombo"))
        return reinterpret_cast<void*>(Oot::SOH_GetConsoleCommandForCombo);
    if (is("2ship", "MM_GetConsoleCommandForCombo"))
        return reinterpret_cast<void*>(Mm::MM_GetConsoleCommandForCombo);
    if (is("soh", "SOH_SetComboBootToTitle"))
        return reinterpret_cast<void*>(Oot::SOH_SetComboBootToTitle);
    if (is("2ship", "MM_RequestComboReturn"))
        return reinterpret_cast<void*>(MmPort::MM_RequestComboReturn);
    assert(false);
    return nullptr;
}
int lastKind = -1;
void OnReturn(int kind) { lastKind = kind; }
int Run(const std::shared_ptr<Ship::Console>& console) {
    return console->GetCommand("reset").Handler(console, {"reset"}, nullptr);
}
// MM runs its next frame: a pending return hands the session back to OoT, which then resumes.
int ReturnToOot(const std::shared_ptr<MmPort::Fast::Fast3dWindow>& window) {
    lastKind = -1;
    window->running = true;
    MmPort::MainStart();
    if (lastKind < 0)
        return -2;
    assert(!window->running);
    Oot::ResumeOot();
    return Oot::gComboReturnFileNum;
}
int main() {
    MmPort::gComboReturnCallback = OnReturn;
    auto window = std::make_shared<MmPort::Fast::Fast3dWindow>();
    MmPort::Ship::Context::GetRawInstance()->window = window;
    Oot::GameState oot;
    Oot::gGameState = &oot;
    Oot::gSaveContext.fileNum = 2;
    for (bool mmFirst : {false, true}) {
        Oot::sComboConsoleCommands.clear();
        Mm::sComboConsoleCommands.clear();
        auto console = std::make_shared<Ship::Console>();
        auto addOot = [&] { ComboConsole::Register(Oot::sComboConsoleCommands, console, "reset",
                              {Oot::ResetHandler, "OoT", {}}, ResolveForTest); };
        auto addMm = [&] { ComboConsole::Register(Mm::sComboConsoleCommands, console, "reset",
                              {Mm::ResetHandler, "MM", {}}, ResolveForTest); };
        if (mmFirst) { addMm(); addOot(); } else { addOot(); addMm(); }
        for (int autosave : {0, 1}) {
            MmPort::autosave = Oot::autosave = autosave;
            // Restart game, Ctrl+R and the console with MM in the foreground: MM's own handler runs, OoT's game
            // state is untouched, neither game saves, even with autosave on, and OoT resumes on its title.
            foreground = 1;
            oot = {};
            MmPort::saves = MmPort::cancels = Oot::saves = 0;
            assert(Run(console) == 0 && oot.next == nullptr && oot.running);
            assert(ReturnToOot(window) == -1 && lastKind == 1);
            assert(MmPort::saves == 0 && MmPort::cancels == 1 && Oot::saves == 0);
            // A portal return still resumes the slot: the title boot comes from the reset alone.
            MmPort::saves = 0;
            MmPort::sComboReturnPending = true;
            assert(ReturnToOot(window) == 2 && lastKind == 0 && MmPort::saves == 1);
            // An owl save quit lands on the title without a second save.
            MmPort::saves = 0;
            MmPort::Combo_RequestOwlSaveQuit();
            assert(ReturnToOot(window) == -1 && lastKind == 2 && MmPort::saves == 0);
            // A reset with OoT in the foreground is OoT's own reset; MM is not asked to return. It skips the
            // soft-reset autosave, which OoT's other exits (a game over quit, the pause menu's quit) keep.
            foreground = 0;
            Oot::interactor.exits = 0;
            assert(Run(console) == 0 && !std::strcmp(oot.next, "TitleSetup_Init") && !oot.running);
            assert(ReturnToOot(window) == -2);
            assert(Oot::interactor.exits == 1 && Oot::saves == 0 && !Oot::gComboResetDiscardsProgress);
            Oot::GameInteractor::Instance->ExecuteHooks<Oot::GameInteractor::OnExitGame>(2);
            assert(Oot::saves == autosave);
        }
    }
    // 0.4.1 kept the first registration, OoT's: its Combo_HandleReset path must still land on the title.
    auto legacy = std::make_shared<Ship::Console>();
    legacy->AddCommand("reset", {Oot::ResetHandler, "OoT", {}});
    legacy->AddCommand("reset", {Mm::ResetHandler, "MM", {}});
    foreground = 1;
    oot = {};
    assert(Run(legacy) == 0 && oot.next == nullptr);
    assert(ReturnToOot(window) == -1 && lastKind == 1);
    assert(errors == 0);
    std::puts("Console reset from MM (both boot orders, autosave on and off) resumes OoT on its title and saves neither "
              "game; portal return, "
              "owl save quit, OoT's own reset and the 0.4.1 order behave as before");
}
''', encoding='utf-8')
exe = hostcxx.build(a.compiler, cpp, includes=[source/'combo', source/'libultraship/include', a.imgui_include],
                   werror=True, msvc=['/wd4100', '/wd4101'], gnu=['-Wno-unused-parameter', '-Wno-unused-variable'])
subprocess.run([str(exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)
