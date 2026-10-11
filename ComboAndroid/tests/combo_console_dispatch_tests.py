"""Run the real first-wins console registry and native entrance handlers across game switches."""
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


def function(text, signature):
    start = hostcxx.find(text, signature)
    opening = text.index('{', start)
    masked = re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"', lambda m: ' '*len(m.group()), text)
    depth = 0
    for end in range(opening, len(text)):
        depth += (masked[end] == '{') - (masked[end] == '}')
        if depth == 0:
            return text[start:end+1]
    raise AssertionError('Unterminated production function')


console = (source/'libultraship/src/ship/debug/Console.cpp').read_text(encoding='utf-8')
registry = '\n'.join(function(console, signature) for signature in (
    'void Console::AddCommand(', 'bool Console::HasCommand(', 'CommandEntry& Console::GetCommand('))
games = []
for game, path, export, field in [
    ('Oot', 'soh/soh/Enhancements/debugconsole.cpp', 'SOH_GetConsoleCommandForCombo', 'nextEntranceIndex'),
    ('Mm', 'mm/2s2h/DeveloperTools/DebugConsole.cpp', 'MM_GetConsoleCommandForCombo', 'nextEntrance'),
]:
    text = (source/path).read_text(encoding='utf-8')
    assert '#define CMD_REGISTER RegisterComboCommand' in text
    assert hostcxx.contains(function(text, 'static void RegisterComboCommand('),
                            'ComboConsole::Register(sComboConsoleCommands, Ship::Context::GetRawInstance()->GetConsole(), name, std::move(entry));')
    assert hostcxx.contains(text, 'CMD_REGISTER("entrance", { EntranceHandler,')
    games.append('namespace ' + game + ' {\n' + r'''
struct PlayState { unsigned int FIELD = 0, transitionTrigger = 0, transitionType = 0; };
struct SaveContext { unsigned int nextTransitionType = 0; } gSaveContext;
PlayState* gPlayState = nullptr;
static ComboConsole::Commands sComboConsoleCommands;
''' .replace('FIELD', field) + function(text, 'extern "C" COMBO_EXPORT const Ship::CommandEntry* ' + export + '(')
        + '\n' + function(text, 'static bool EntranceHandler(') + '\n}\n')

cpp = work/'combo_console_dispatch.cpp'
cpp.write_text(r'''
#include <cassert>
#include <cstdio>
#include <stdexcept>
#include <cstring>
#include "ComboConsoleCommands.h"
#define COMBO_EXPORT
#define SPDLOG_WARN(...) ((void)0)
#define ERROR_MESSAGE(...) (++errors)
int errors = 0;
constexpr int TRANS_TRIGGER_START = 1, TRANS_TYPE_INSTANT = 2;
namespace Ship {
Console::Console() = default;
Console::~Console() = default;
''' + registry + '\n}\n' + '\n'.join(games) + r'''
int foreground = 0;
bool haveForeground = true, haveLookup = true;
int Foreground() { return foreground; }
void* ResolveForTest(const char* module, const char* name) {
    if (!std::strcmp(module, "comboui") && !std::strcmp(name, "ComboUI_GetForegroundGame"))
        return haveForeground ? reinterpret_cast<void*>(Foreground) : nullptr;
    if (!haveLookup) return nullptr;
    if (!std::strcmp(module, "soh") && !std::strcmp(name, "SOH_GetConsoleCommandForCombo"))
        return reinterpret_cast<void*>(Oot::SOH_GetConsoleCommandForCombo);
    if (!std::strcmp(module, "2ship") && !std::strcmp(name, "MM_GetConsoleCommandForCombo"))
        return reinterpret_cast<void*>(Mm::MM_GetConsoleCommandForCombo);
    assert(false);
    return nullptr;
}
int Run(std::shared_ptr<Ship::Console> console, const char* command, std::vector<std::string> args,
        std::string* output = nullptr) {
    return console->GetCommand(command).Handler(console, std::move(args), output);
}
int main() {
    Oot::PlayState oot;
    Mm::PlayState mm;
    Oot::gPlayState = &oot;
    Mm::gPlayState = &mm;
    // Negative control: the original AddCommand semantics permanently retain OoT.
    auto legacy = std::make_shared<Ship::Console>();
    legacy->AddCommand("entrance", {Oot::EntranceHandler, "OoT", {}});
    legacy->AddCommand("entrance", {Mm::EntranceHandler, "MM", {}});
    foreground = 1;
    assert(Run(legacy, "entrance", {"entrance", "5400"}) == 0);
    assert(oot.nextEntranceIndex == 0x5400 && mm.nextEntrance == 0);

    for (bool mmFirst : {false, true}) {
        Oot::sComboConsoleCommands.clear();
        Mm::sComboConsoleCommands.clear();
        auto console = std::make_shared<Ship::Console>();
        auto addOot = [&] { ComboConsole::Register(Oot::sComboConsoleCommands, console, "entrance",
                              {Oot::EntranceHandler, "OoT", {}}, ResolveForTest); };
        auto addMm = [&] { ComboConsole::Register(Mm::sComboConsoleCommands, console, "entrance",
                              {Mm::EntranceHandler, "MM", {}}, ResolveForTest); };
        if (mmFirst) { addMm(); addOot(); } else { addOot(); addMm(); }
        for (int game : {0, 1, 0, 1}) {
            foreground = game;
            oot = {}; mm = {};
            Oot::gSaveContext = {}; Mm::gSaveContext = {};
            Oot::gPlayState = game == 0 ? &oot : nullptr;
            Mm::gPlayState = game == 1 ? &mm : nullptr;
            assert(Run(console, "entrance", {"entrance", "5400"}) == 0);
            assert(oot.nextEntranceIndex == (game == 0 ? 0x5400u : 0u));
            assert(mm.nextEntrance == (game == 1 ? 0x5400u : 0u));
            assert(Oot::gSaveContext.nextTransitionType == (game == 0 ? 2u : 0u));
            assert(Mm::gSaveContext.nextTransitionType == (game == 1 ? 2u : 0u));
            if (game == 0) Oot::gPlayState = nullptr; else Mm::gPlayState = nullptr;
            assert(Run(console, "entrance", {"entrance", "5400"}) == 1);
        }
        Oot::gPlayState = &oot; Mm::gPlayState = &mm;
        int localCalls = 0, builtinCalls = 0;
        ComboConsole::Register(Oot::sComboConsoleCommands, console, "oot-only",
            {[&](auto, auto, auto) { ++localCalls; return 0; }, "", {}}, ResolveForTest);
        console->AddCommand("shared", {[&](auto, auto, auto) { ++builtinCalls; return 0; }, "", {}});
        foreground = 1;
        std::string output;
        assert(Run(console, "oot-only", {"oot-only"}, &output) == 1 && localCalls == 0 && !output.empty());
        assert(Run(console, "shared", {"shared"}) == 0 && builtinCalls == 1);
        foreground = 0;
        assert(Run(console, "oot-only", {"oot-only"}) == 0 && localCalls == 1);
        for (int failure = 0; failure < 3; ++failure) {
            haveForeground = failure != 0;
            haveLookup = failure != 1;
            foreground = failure == 2 ? -1 : 0;
            assert(Run(console, "entrance", {"entrance", "5400"}) == 1);
        }
        haveForeground = haveLookup = true;
        foreground = 0;
        assert(Run(console, "entrance", {"entrance"}) == 1);
        assert(Run(console, "entrance", {"entrance", "xyz"}) == 1);
    }
    std::puts("Actual first-wins registry and both native entrance handlers: foreground routing, both boot orders, switches, null/missing guards and shared commands passed");
}
''', encoding='utf-8')
exe = hostcxx.build(a.compiler, cpp, includes=[source/'combo', source/'libultraship/include', a.imgui_include],
                   werror=True, msvc=['/wd4100', '/wd4101'], gnu=['-Wno-unused-parameter', '-Wno-unused-variable'])
subprocess.run([str(exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)
