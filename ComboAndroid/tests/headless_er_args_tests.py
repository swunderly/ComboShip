"""Headless --er / --er-capability and the dev-only COMBO_ENTRANCE_DEV switch.

Runs the production headless argument handling (extracted from combo/ComboRandoHeadless.cpp) built as
shipped and with COMBO_ENTRANCE_DEV, checks ReadEntranceArgs against every catalog stage, and scans the
tree: the option defaults to OFF, dev-only code stays under #ifdef COMBO_ENTRANCE_DEV, verify_apk.py
refuses the dev marker, and the native settings dumps carry no entrance settings.
"""
from pathlib import Path
import argparse
import importlib.util
import os
import re
import subprocess
import sys
import hostcxx

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--json-include', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
args = parser.parse_args()
root = args.source.resolve()
work = args.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)


def check(condition, message):
    if not condition:
        sys.exit('FAIL: ' + message)


def read(relative):
    return (root/relative).read_text(encoding='utf-8')


# --- The option is OFF unless a builder turns it on, and it is the only way the macro is defined.
cmake = read('CMakeLists.txt')
block = re.findall(r'option\(COMBO_ENTRANCE_DEV "[^"]+" (\w+)\)\s*if\(COMBO_ENTRANCE_DEV\)\s*'
                   r'add_compile_definitions\(COMBO_ENTRANCE_DEV=1\)\s*endif\(\)', cmake)
check(block == ['OFF'], 'root CMakeLists.txt must declare COMBO_ENTRANCE_DEV OFF and define the macro only when on')
check(cmake.count('COMBO_ENTRANCE_DEV') == 3, 'COMBO_ENTRANCE_DEV is used only by its option block')
check(cmake.index('COMBO_ENTRANCE_DEV') < cmake.index('add_subdirectory(soh'),
      'the option must come before the game and combo subprojects')
diagnostic_block = re.findall(r'option\(COMBO_RANDO_DIAGNOSTICS "[^"]+" (\w+)\)\s*'
                             r'if\(COMBO_RANDO_DIAGNOSTICS\)\s*'
                             r'add_compile_definitions\(COMBO_RANDO_DIAGNOSTICS=1\)\s*endif\(\)', cmake)
check(diagnostic_block == ['OFF'], 'private diagnostics must have their own default-OFF compile switch')
SKIP_DIRS = {'.git', 'assets', 'x64', '_deps'}
CPP = ('.c', '.cc', '.cpp', '.h', '.hpp', '.inc')


def walk(top, suffixes):
    for directory, dirs, files in os.walk(root/top):
        dirs[:] = [d for d in dirs if d not in SKIP_DIRS and not d.lower().startswith('build')]
        for name in files:
            if name.endswith(suffixes):
                yield Path(directory, name)


for path in [*walk('.', ('CMakeLists.txt', '.cmake'))]:
    relative = path.relative_to(root).as_posix()
    if relative not in ('CMakeLists.txt', 'ComboAndroid/tests/CMakeLists.txt'):
        check('COMBO_ENTRANCE_DEV' not in path.read_text(encoding='utf-8', errors='replace'),
              f'{relative} must not set COMBO_ENTRANCE_DEV')

# --- Dev-only symbols appear only inside #ifdef COMBO_ENTRANCE_DEV.
capabilities = read('combo/rando/entrances/Capabilities.h')
marker = re.search(r'kEntranceDevMarker\[\] = "([^"]+)";', capabilities)
check(marker is not None, 'Capabilities.h defines kEntranceDevMarker')
marker = marker.group(1)


def unguarded(text, needles, allowed_macros=('COMBO_ENTRANCE_DEV',)):
    """Line numbers holding a needle outside an explicitly allowed compile branch."""
    stack, hits = [], []
    macros = '(?:' + '|'.join(map(re.escape, allowed_macros)) + ')'
    for number, line in enumerate(text.splitlines(), 1):
        directive = re.sub(r'^#\s*', '#', line.strip())
        if re.match(r'#ifdef\s+' + macros + r'\b|#if\s+defined\s*\(?\s*' + macros + r'\s*\)?\s*$', directive):
            stack.append(True)
            continue
        if directive.startswith('#if'):
            stack.append(False)
            continue
        if directive.startswith(('#else', '#elif')) and stack:
            stack[-1] = False
            continue
        if directive.startswith('#endif') and stack:
            stack.pop()
            continue
        compact = re.sub(r'\s+', '', line)
        if any(needle in compact for needle in needles) and not any(stack):
            hits.append(number)
    return hits


dev_needles = [re.sub(r'\s+', '', marker), 'EntranceDevMarker', 'AtLeast(Stage::Dev)']
scanned = 0
for top in ('combo', 'soh', 'mm', 'ComboAndroid/app'):
    for path in walk(top, CPP):
        text = path.read_text(encoding='utf-8', errors='replace')
        relative = path.relative_to(root).as_posix()
        scanned += 1
        check(not re.search(r'#\s*define\s+COMBO_ENTRANCE_DEV\b', text), f'{relative} defines COMBO_ENTRANCE_DEV')
        # Diagnostics must carry the verifier's non-release marker too, but
        # cannot gain any entrance capability symbol through that compile flag.
        hits = unguarded(text, dev_needles[1:])
        hits += unguarded(text, dev_needles[:1], ('COMBO_ENTRANCE_DEV', 'COMBO_RANDO_DIAGNOSTICS'))
        check(not hits, f'{relative}:{hits} uses a dev-only entrance symbol outside #ifdef COMBO_ENTRANCE_DEV')
        # Builds take their capabilities from ActiveCapabilities(); only tests build other sets.
        if relative != 'combo/rando/entrances/Capabilities.h':
            check('AtLeast(' not in text, f'{relative} must use ActiveCapabilities()')
check(scanned > 4000, f'only {scanned} production sources scanned')
check(unguarded(capabilities, dev_needles) == [] and
      len([n for n in dev_needles if n in re.sub(r'\s+', '', capabilities)]) == 3,
      'Capabilities.h keeps its marker and dev set inside #ifdef COMBO_ENTRANCE_DEV')

# --- verify_apk.py refuses exactly the libraries that carry the same marker.
sys.path.insert(0, str(root/'ComboAndroid/tools'))
spec = importlib.util.spec_from_file_location('verify_apk', root/'ComboAndroid/tools/verify_apk.py')
verify_apk = importlib.util.module_from_spec(spec)
spec.loader.exec_module(verify_apk)
check(verify_apk.ENTRANCE_DEV_MARKER == marker.encode('ascii'), 'verify_apk.py and Capabilities.h share the marker')
libraries = [('lib/arm64-v8a/libcomboship.so', b'\x7fELF' + marker.encode('ascii') + b'\0'),
             ('lib/arm64-v8a/libsoh.so', b'\x7fELF clean'), ('lib/arm64-v8a/lib2ship.so', marker.encode('ascii'))]
check(verify_apk.entrance_dev_libraries(libraries) == ['lib/arm64-v8a/lib2ship.so', 'lib/arm64-v8a/libcomboship.so'],
      'verify_apk.py names every dev library')
check(verify_apk.entrance_dev_libraries(libraries[1:2]) == [], 'verify_apk.py accepts a shipping library')

# --- C5: the er* values are combo CVars and stay out of the native settings dumps (and so out of displaySeed).
for relative, name in (('soh/soh/OTRGlobals.cpp', 'SOH_DumpRandoSettings'), ('mm/2s2h/BenPort.cpp', 'MM_DumpRandoSettings')):
    text = read(relative)
    start = hostcxx.find(text, f'extern "C" COMBO_EXPORT const char* {name}(void) {{')
    body = text[start:text.index('\n}\n', start)]
    check('Entrances' not in body and 'gCombo.Rando' not in body, f'{name} must not dump entrance settings')

# --- The production headless argument handling, built as shipped and as a dev build.
headless = read('combo/ComboRandoHeadless.cpp')
start = hostcxx.find(headless, 'int RunHeadless(int argc, char** argv) {')
start = headless.index('{', start) + 1
end = headless.rindex('#ifdef _WIN32', start, hostcxx.find(headless, 'DllHandle soh = LoadGameModule(', start))
extracted = headless[start:end]
for needle in ('ReadEntranceArgs(', 'ActiveCapabilities()', '"--er"', '"--er-capability"', 'return 2;'):
    check(needle in extracted, f'headless argument handling lost {needle}')

harness = r'''
#include "rando/entrances/OotmmSettings.h"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
using namespace ComboRando::Entrances;
namespace nlohmann {
struct json {}; // the argument handling only declares the saved private layouts
} // namespace nlohmann

int Headless(int argc, char** argv) {
''' + extracted + r'''
    return 0;
}

struct Result {
    int code;
    std::string text;
};

Result Run(std::vector<std::string> arguments) {
    std::vector<char*> argv{const_cast<char*>("comborando")};
    for (auto& argument : arguments)
        argv.push_back(argument.data());
    std::ostringstream out, err;
    auto* coutBuffer = std::cout.rdbuf(out.rdbuf());
    auto* cerrBuffer = std::cerr.rdbuf(err.rdbuf());
    const int code = Headless(static_cast<int>(argv.size()), argv.data());
    std::cout.rdbuf(coutBuffer);
    std::cerr.rdbuf(cerrBuffer);
    return {code, out.str() + err.str()};
}

int failures = 0;
void Expect(const std::vector<std::string>& arguments, int code, const std::string& text, bool present = true) {
    const auto result = Run(arguments);
    if (result.code != code || (result.text.find(text) != std::string::npos) != present) {
        std::cout << "FAIL:";
        for (const auto& argument : arguments)
            std::cout << " [" << argument << "]";
        std::cout << " -> " << result.code << ": " << result.text << "\n";
        ++failures;
    }
}

void ExpectArgs(const std::vector<std::string>& pairs, std::string_view capability, const CapabilitySet& active,
                const std::string& error, const std::string& settings = "") {
    Settings parsed;
    parsed.erBoss = Mode::Full; // replaced only on success
    const auto result = ReadEntranceArgs(pairs, capability, active, parsed);
    const bool ok = error.empty() ? result.empty() && EntranceSettingsText(parsed) == settings
                                  : result == error && parsed.erBoss == Mode::Full;
    if (!ok) {
        std::cout << "FAIL: ReadEntranceArgs";
        for (const auto& pair : pairs)
            std::cout << " [" << pair << "]";
        std::cout << " cap [" << capability << "] -> '" << result << "' / " << EntranceSettingsText(parsed) << "\n";
        ++failures;
    }
}

int main() {
#ifdef COMBO_ENTRANCE_DEV
    assert(ActiveCapabilities().devBuild);
#else
    assert(!ActiveCapabilities().devBuild);
#endif
    // Without --er nothing changes; defaults are accepted by OoTMM name.
    Expect({}, 0, "entrance settings", false);
    Expect({"--seed", "7", "--count", "2"}, 0, "entrance settings", false);
#ifdef COMBO_ENTRANCE_DEV
    Expect({"--er-validation", "baseline"}, 0, "entrance settings", false);
    Expect({"--er-validation", "shortcut"}, 0, "entrance settings", false);
    Expect({"--er-validation"}, 2, "needs baseline or shortcut, once");
    Expect({"--er-validation", "all"}, 2, "needs baseline or shortcut, once");
    Expect({"--er-validation", "baseline", "--er-validation", "shortcut"}, 2, "needs baseline or shortcut, once");
    Expect({"--er-validation", "baseline", "--playthrough", "seed.json"}, 2, "cannot change a recorded replay");
#else
    Expect({"--er-validation", "baseline"}, 2, "requires an entrance dev build");
#endif
    Expect({"--mm-wallmasters"}, 0, "entrance settings", false);
    Expect({"--er", "erWallmasters=none"}, 0, "[comborando] entrance settings: all defaults\n");
    Expect({"--er", "erSelfLoops=false", "--er", "erOneWays=none", "--er", "erOneWaysMajor=false"}, 0,
           "entrance settings: all defaults");
    // Without --er-capability, shipping and dev builds alike admit only erWallmasters=ownGame, through the
    // vanilla contract (Public since P1-05), and refuse every other non-default value.
    for (const char* pair : {"erWallmasters=ownGame", "erWallmasters=full", "erOneWays=ownGame",
                             "erOneWaysMajor=true", "erOneWaysAnywhere=true", "erDungeons=ownGame",
                             "erSpawns=both", "erDecoupled=true"}) {
        if (std::string(pair) == "erWallmasters=ownGame") {
            Expect({"--er", pair}, 0, "entrance settings: erWallmasters=ownGame");
            continue;
        }
        Expect({"--er", pair}, 2,
               std::string("[comborando] ") + pair + " is not available in this build\n");
        Expect({"--er", pair}, 2, "entrance settings", false);
    }
    Expect({"--er", "erWallmasters"}, 2, "--er needs <key>=<value> (got 'erWallmasters')");
    Expect({"--er", "--seed"}, 2, "--er needs <key>=<value> (got '--seed')");
    Expect({"--er", "erWalmasters=none"}, 2, "unknown entrance setting 'erWalmasters'");
    Expect({"--er", "erWallmasters=OwnGame"}, 2,
           "unknown value 'OwnGame' for erWallmasters (expected none, ownGame, full)");
    Expect({"--er", "erSelfLoops=1"}, 2, "unknown value '1' for erSelfLoops (expected false, true)");
    Expect({"--er", "erSpawns=none", "--er", "erSpawns=none"}, 2, "--er sets erSpawns more than once");
    Expect({"--er"}, 2, "--er needs a value");
    Expect({"--er", ""}, 2, "--er needs a value");
    Expect({"--er-capability"}, 2, "--er-capability needs a value");
    Expect({"--er-capability", "a", "--er-capability", "b"}, 2, "--er-capability can be given only once");
    Expect({"--er-capability", "wallmasters"}, 2, "unknown entrance capability 'wallmasters'");
    // --settings may provide the required capability. Defer capability/value validation to the
    // saved-settings selection helper, which the reproduction harness executes in both builds.
    Expect({"--settings", "saved.json", "--er", "erWallmasters=ownGame"}, 0, "entrance settings", false);
    for (const auto& capability : kEntranceCapabilities) {
        // Every build has the vanilla contract; the dev tool also generates the MM tranche (M1-07 emulator
        // runs) and the staged Master Quest contract (U11).
        bool admitted = capability.id == "wallmasters-own-game-vanilla-v1";
#ifdef COMBO_ENTRANCE_DEV
        admitted = admitted || capability.id == "wallmasters-own-game-mm-tranche" ||
                   capability.id == "wallmasters-own-game-quests-v1";
#endif
        if (admitted) {
            Expect({"--er-capability", std::string(capability.id), "--er", "erWallmasters=ownGame"}, 0,
                   "[comborando] entrance settings: erWallmasters=ownGame\n");
            Expect({"--er-capability", std::string(capability.id), "--er", "erOneWays=ownGame"}, 2,
                   "erOneWays=ownGame is not admitted by " + std::string(capability.id));
            continue;
        }
        Expect({"--er-capability", std::string(capability.id), "--er", "erWallmasters=ownGame"}, 2,
               "entrance capability '" + std::string(capability.id) + "' is not available in this build");
    }
    // The private MM profiles never combine with --er, whatever the order.
    for (const char* legacy : {"--mm-ordinary-indoors", "--mm-ordinary-telescopes", "--mm-ordinary-overworld",
                               "--mm-ordinary-dungeons", "--mm-mixed", "--mm-grottos", "--mm-wallmasters"}) {
        const std::string message = "--er and --er-capability cannot be combined with the --mm-* entrance flags";
        Expect({legacy, "--er", "erWallmasters=none"}, 2, message);
        Expect({"--er", "erWallmasters=none", legacy}, 2, message);
        Expect({"--er-capability", "wallmasters-own-game-mm-tranche", legacy}, 2, message);
    }

    // ReadEntranceArgs against sets the catalog will reach in later stages.
    const auto all = CapabilitySet::AtLeast(Stage::Hidden);
    const auto noCompanion = all.Only("wallmasters-own-game-v1") | all.Only("one-ways-own-game-v1");
    ExpectArgs({"erWallmasters=ownGame"}, "", all, "", "erWallmasters=ownGame");
    ExpectArgs({"erOneWaysMajor=true", "erWallmasters=ownGame", "erOneWays=ownGame"}, "", all, "",
               "erWallmasters=ownGame, erOneWays=ownGame, erOneWaysMajor=true");
    // OoTMM's own conds still reset a flag whose erOneWays is none.
    ExpectArgs({"erOneWaysMajor=true"}, "", all, "", "all defaults");
    ExpectArgs({"erWallmasters=full"}, "", all, "erWallmasters=full is not available in this build");
    ExpectArgs({"erDungeons=ownGame"}, "", all, "erDungeons=ownGame is not available in this build");
    ExpectArgs({"erOneWays=ownGame", "erWallmasters=ownGame"}, "", noCompanion,
               "erOneWays=ownGame cannot be combined with erWallmasters=ownGame in this build");
    ExpectArgs({"erOneWaysIkana=true", "erWallmasters=ownGame", "erOneWays=ownGame"}, "", noCompanion,
               "erOneWays=ownGame cannot be combined with erWallmasters=ownGame in this build");
    ExpectArgs({"erOneWays=ownGame", "erOneWaysOwls=true"}, "", noCompanion, "",
               "erOneWays=ownGame, erOneWaysOwls=true");
    // A devOnly row is used only when selected by id.
    const auto tranche = all.Only("wallmasters-own-game-mm-tranche");
    ExpectArgs({"erWallmasters=ownGame"}, "", tranche,
               "erWallmasters=ownGame is not available in this build without --er-capability");
    ExpectArgs({"erWallmasters=ownGame"}, "wallmasters-own-game-mm-tranche", all, "", "erWallmasters=ownGame");
    ExpectArgs({"erWallmasters=ownGame", "erOneWays=ownGame"}, "wallmasters-own-game-mm-tranche", all,
               "erOneWays=ownGame is not admitted by wallmasters-own-game-mm-tranche");
    ExpectArgs({"erWallmasters=ownGame", "erOneWays=ownGame", "erOneWaysAnywhere=true"},
               "wallmasters-with-one-ways-v1", all, "", "erWallmasters=ownGame, erOneWays=ownGame, erOneWaysAnywhere=true");
    ExpectArgs({}, "one-ways-own-game-v1", all.Only("wallmasters-own-game-v1"),
               "entrance capability 'one-ways-own-game-v1' is not available in this build");
    ExpectArgs({}, "nope", all, "unknown entrance capability 'nope'");
    ExpectArgs({}, "", CapabilitySet::AtLeast(Stage::Public), "", "all defaults");
    ExpectArgs({"erWallmasters=ownGame"}, "", CapabilitySet::AtLeast(Stage::Public), "", "erWallmasters=ownGame");
    ExpectArgs({"erOneWays=ownGame"}, "", CapabilitySet::AtLeast(Stage::Public),
               "erOneWays=ownGame is not available in this build");
    if (failures)
        return 1;
#ifdef COMBO_ENTRANCE_DEV
    std::cout << "headless --er arguments passed (COMBO_ENTRANCE_DEV build)\n";
#else
    std::cout << "headless --er arguments passed (shipping build)\n";
#endif
    return 0;
}
'''
source = work/'headless_er_args.cpp'
source.write_text(harness, encoding='utf-8')
for name, defines in (('shipping', []), ('dev', ['COMBO_ENTRANCE_DEV=1'])):
    exe = hostcxx.build(args.compiler, source, includes=[root/'combo', args.json_include], defines=defines,
                        exe=work/f'headless_er_args_{name}{".exe" if os.name == "nt" else ""}')
    result = subprocess.run([str(exe)], capture_output=True, text=True, timeout=hostcxx.RUN_TIMEOUT)
    print(result.stdout + result.stderr, end='')
    check(result.returncode == 0, f'{name} headless --er harness failed')
print(f'Dev switch OFF by default, {scanned} production sources scanned, release marker shared, '
      'headless --er cases passed in shipping and dev builds.')
