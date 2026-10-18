"""Where the launcher and the headless tool call the unified entrance generator (UnifiedBridge.h).

Static scan of combo/ComboShip.cpp (RunComboFill) and combo/ComboRandoHeadless.cpp. Each generation
attempt clears the MM entrance state, prepares the native pools and entrance graph (older modules
retain their post-dump shuffle), runs the private MM generators, then generates and installs the OoTMM layout, and only then runs the
combined fill; the layout stays installed through the hint dump, the requiredness pass and the
playthrough of that attempt (critic correction C13). An exhausted layout search (ShuffleError) rerolls
the attempt, pins a Random start to OoT after an MM start as the other failures do (C24), and in the
launcher is bounded by the headless budget (C1); every other entrance failure stops generation. The
layout goes into the consolidated seed before its fileType is stamped, and the launcher's displaySeed
also hashes it (C5). Nothing else calls the generator. Both callers resolve every MM entrance export
into NativeFns in member order, including the one the generator sets its er* settings through (M1-08).
"""
from pathlib import Path
import argparse
import re

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
args = parser.parse_args()
source = args.source.resolve()


def read(relative):
    return (source/relative).read_text(encoding='utf-8')


def ordered(text, *needles, start=0):
    """Index of each needle, each found after the previous one."""
    found = []
    for needle in needles:
        index = text.find(needle, start)
        assert index >= 0, f'missing or out of order: {needle}'
        found.append(index)
        start = index + len(needle)
    return found


def block(text, opener):
    """The braced block that starts at the first '{' after opener."""
    start = text.index('{', text.index(opener))
    depth = 0
    for i in range(start, len(text)):
        depth += {'{': 1, '}': -1}.get(text[i], 0)
        if depth == 0:
            return text[start:i + 1]
    raise AssertionError(f'unbalanced block after {opener}')


# --- The launcher: RunComboFill.
ship = read('combo/ComboShip.cpp')
fill = ship[ship.index('static void RunComboFill('):ship.index('// ComboShip: headless cross-world generation TEST')]
assert fill.count('GenerateEntranceLayout(') == 1 and ship.count('GenerateEntranceLayout(') == 1
settings = ordered(fill, 'const auto entranceCaps = ComboRando::Entrances::ActiveCapabilities();',
                   'ReadEntranceSettings(readSetting, entranceCaps);', 'for (uint64_t attempt = 0;')
assert 'unified.capability' not in fill, 'in-game generation never selects a dev-only capability'
loop = fill[settings[-1]:]
steps = ordered(loop, 'ApplyNativeSeed(sEntranceNative, nlohmann::json::object(),', 'SOH_DumpRandoStaticData()',
                'SOH_ShuffleEntrancesForCombo(masterSeed)', 'GenerateNativeMmWallmasters(', 'GenerateNativeMmGrottos(',
                'GenerateEntranceLayout(unified, sEntranceNative, sTempleProbe)',
                'ApplyUnifiedLayout(sEntranceNative, entranceLayout, entranceCaps)', 'CrossWorldCombinedFill(',
                'SOH_DumpRandoHintData()', 'PareDownPlaythrough(', 'WriteComboPlaythrough(')
installed = loop[steps[6]:steps[-1]]
for clear in ('ApplyNativeSeed(', 'applySeed(', 'ApplyNativeMm'):
    assert clear not in installed, f'{clear} between the install and the playthrough'
for field in ('settings = entranceSettings', 'active = entranceCaps', 'seed = masterSeed', 'readSetting = readSetting',
              'privateMmProfiles = privateMmProfiles', 'ootDump = sohDump', 'mmDump = mmDump',
              'forcedOot = forcedOot', 'oot = ootOracle', 'mm = mmOracle', 'mmStart = mmStart', 'goal = goal',
              'access = ootAccess', 'sharedMask = sharedMask', 'songTravelLogic = sSongTravel.logicEnabled',
              'foreignSongsLogic = sSongTravel.logicForeignSongs'):
    assert f'unified.{field};' in loop, field
handler = block(loop, 'catch (const ComboRando::Entrances::ShuffleError& error)')
for needle in ('entranceLayout = nlohmann::json();', 'lastFillError =', 'pinStartOot = true;',
               'Combo_MM_Rando_Restore();', 'if (++entranceFailures >= kFillAttempts) {', 'fail(lastFillError.c_str());',
               'continue;'):
    assert needle in handler, needle
assert loop.count('catch (const ComboRando::Entrances::') == 1, 'other entrance failures must stop generation'
# C1: the generator's only handler is the ShuffleError one. A generic one would reroll a deterministic
# refusal, forever when retryUntilSuccess is set.
handlers = loop[steps[5]:steps[7]]
assert len(re.findall(r'\bcatch\s*\(', handlers)) == 1, 'only an exhausted layout search may reroll'
assert not re.search(r'\bcatch\s*\(\s*(?:\.\.\.|(?:const\s+)?std::exception)', handlers), 'generic entrance handler'
assert re.search(r'ComboHash\(ComboRando::Entrances::SeedIdentityText\(\s*inputSeed \+ sohDump \+ mmDump \+ '
                 r'logicRules\.dump\(\) \+ logicSongs\.dump\(\), entranceLayout\)\s*\.c_str\(\)\)', fill)
ordered(fill, 'consolidated["entranceLayout"] = entranceLayout;', 'ComboRando::StampComboSeedFormat(consolidated);')
assert 'if (!entranceLayout.is_null())\n            consolidated["entranceLayout"] = entranceLayout;' in fill
assert re.search(r'sTempleProbe\s*=\s*ComboRando::Entrances::ResolveUnifiedTempleProbe\(\s*\[&\]\(const char\* name\) '
                 r'\{ return GetSym\(sohModule, name\); \}\);', ship)
# The env-gated gentest and playthrough tools generate no entrance layout (none is selectable in-game).
for tool in ('static int RunComboGenTest(', 'static void RunComboPlaythrough('):
    assert 'GenerateEntranceLayout' not in block(ship, tool), tool

# --- The headless tool.
headless = read('combo/ComboRandoHeadless.cpp')
assert headless.count('GenerateEntranceLayout(') == 1
generate = headless[headless.index('// ---- Generate + validate mode. ----'):]
steps = ordered(generate, 'ApplyNativeSeed(entranceNative, nlohmann::json::object(),', 'SOH_GenerationDump()',
                'SOH_ShuffleEntrances(masterSeed)', 'GenerateNativeMmWallmasters(', 'GenerateNativeMmGrottos(',
                'GenerateEntranceLayout(unified, entranceNative, templeProbe)',
                'ApplyUnifiedLayout(entranceNative, entranceLayout, unified.active)', 'CrossWorldCombinedFill(',
                'SOH_DumpRandoHintData', 'PareDownPlaythrough(')
assert 'if (!nativePreparesEntrances && SOH_ShuffleEntrances &&' in generate, \
    'A current native preparation must not be reshuffled after restricted placement'
replay = block(headless, 'auto runPass =')
assert 'SOH_Dump()' in replay and 'SOH_GenerationDump()' not in replay, \
    'Saved-seed replay must retain the legacy metadata dump and recorded entrance apply'
for clear in ('ApplyNativeSeed(', 'applySeed(', 'ApplyNativeMm'):
    assert clear not in generate[steps[6]:steps[-1]], f'{clear} between the install and the requiredness pass'
for field in ('capability = entranceCapability', 'active = ComboRando::Entrances::ActiveCapabilities()',
              'settings = entranceSettings', 'seed = masterSeed', 'forcedOot = forced', 'oot = oot', 'mm = mmO',
              'songTravelLogic = generatedSongs.logicEnabled', 'foreignSongsLogic = generatedSongs.logicForeignSongs'):
    assert f'unified.{field};' in generate, field
for private in ('nativeMmOrdinaryIndoors', 'nativeMmOrdinaryTelescopes', 'nativeMmOrdinaryOverworld',
                'nativeMmOrdinaryDungeons', 'nativeMmMixed', 'nativeMmGrottos', 'nativeMmWallmasters',
                '!savedOrdinaryEntrances.is_null()', '!savedGrottoEntrances.is_null()', '!savedMixedEntrances.is_null()'):
    assert private in generate[:steps[5]].rsplit('unified.privateMmProfiles =', 1)[1].split(';', 1)[0], private
handler = block(generate, 'catch (const ComboRando::Entrances::ShuffleError& error)')
for needle in ('entranceLayout = nlohmann::json();', 'r = { "", false,', 'MM_Restore();', 'pinStartOot = true;',
               'continue;'):
    assert needle in handler, needle
assert 'return 2;' in block(generate[steps[5]:], 'catch (const std::exception& error)')
# The same two handlers in order and no other: a reroll, then exit 2 for everything else.
handlers = generate[steps[5]:steps[7]]
assert [m.group(1) for m in re.finditer(r'\bcatch\s*\(([^)]*)\)', handlers)] == [
    'const ComboRando::Entrances::ShuffleError& error', 'const std::exception& error'], 'headless entrance handlers'
ordered(generate, 'consolidated["entranceLayout"] = entranceLayout;', 'ComboRando::StampComboSeedFormat(consolidated);')
assert re.search(r'templeProbe\s*=\s*ComboRando::Entrances::ResolveUnifiedTempleProbe\(\s*\[&\]\(const char\* name\) '
                 r'\{ return Sym<void\*>\(soh, name\); \}\);', headless)

# --- The MM exports behind NativeFns, in member order; the last carries the generation's er* settings.
exports = ['MM_ApplyEntranceExpansionForCombo', 'MM_SetEntranceExpansionAssumptionsForCombo',
           'MM_ClearEntranceExpansionAssumptionsForCombo', 'MM_DumpEntranceExpansionForCombo',
           'MM_SetEntranceGenerationSettingsForCombo']
fns = block(read('combo/rando/entrances/NativeBridge.h'), 'struct NativeFns')
assert re.findall(r'\(\*(\w+)\)', fns) == ['applySeed', 'setAssumptions', 'clearAssumptions', 'dumpSeed',
    'setGenerationSettings', 'applyOotLayout', 'setOotAssumptions', 'clearOotAssumptions',
    'dumpOotLayout', 'ootAreaContext'], 'NativeFns members'
oot_exports = ['SOH_SetOotmmEntranceLayout', 'SOH_SetOotmmEntranceAssumptions',
               'SOH_ClearOotmmEntranceAssumptions', 'SOH_DumpOotmmEntranceLayout', 'SOH_GetOotmmAreaContext']
for text, opener in ((ship, 'sEntranceNative = {'), (headless, 'ComboRando::Entrances::NativeFns entranceNative{')):
    init = text[text.index(opener):]
    assert re.findall(r'"(MM_\w+)"', init[:init.index(';')]) == exports, opener
    assert re.findall(r'"(SOH_\w+)"', init[:init.index(';')]) == oot_exports, opener
mm_module = read('mm/2s2h/Rando/Logic/ComboEntrancesMM.cpp')
for name in exports:
    assert re.search(r'extern "C" COMBO_EXPORT [^\n(]*\b' + name + r'\(', mm_module), name
for name in oot_exports:
    assert re.search(r'extern "C" COMBO_EXPORT [^\n(]*\b' + name + r'\(', read('soh/soh/OTRGlobals.cpp')), name

# --- D1: the bridge reads exactly the CVars of the options that turn on Ship's RSK_SHUFFLE_ENTRANCES.
native = read('soh/soh/Enhancements/randomizer/settings.cpp')
derive = native[native.index('// If any of the individual shuffle settings are on, turn on the main Shuffle Entrances option'):]
derive = derive[:derive.index('mOptions[RSK_SHUFFLE_ENTRANCES].Set(RO_GENERIC_ON);')]
keys = set(re.findall(r'mOptions\[(RSK_\w+)\]', derive))
cvars = set()
for key in keys:
    match = re.search(r'OPT_(?:U8|BOOL)\(' + key + r',[^;]*?CVAR_RANDOMIZER_SETTING\("(\w+)"\)', native)
    assert match, key
    cvars.add('gRandoSettings.' + match.group(1))
bridge = read('combo/rando/entrances/UnifiedBridge.h')
listed = bridge[bridge.index('kShipEntranceSettings[] = {'):]
listed = set(re.findall(r'"(gRandoSettings\.\w+)"', listed[:listed.index('};')]))
assert len(keys) == 9 and listed == cvars, (sorted(listed), sorted(cvars))
assert 'CVAR_RANDOMIZER_SETTING("MQDungeons")' in native and 'RO_MQ_DUNGEONS_NONE' in native
assert '"gRandoSettings.MQDungeons"' in bridge

# --- Nothing else generates a unified layout.
callers = []
for path in sorted((source/'combo').rglob('*')):
    if path.suffix in ('.h', '.cpp', '.inc') and 'GenerateEntranceLayout(' in path.read_text(encoding='utf-8',
                                                                                              errors='replace'):
        callers.append(path.relative_to(source).as_posix())
assert callers == ['combo/ComboRandoHeadless.cpp', 'combo/ComboShip.cpp', 'combo/rando/entrances/UnifiedBridge.h'], callers
print('Unified entrance generator wired after the native pools and private MM generators, installed for the fill, '
      'hints and playthrough of its attempt, bounded rerolls, seed stamping and identity in launcher and headless.')
