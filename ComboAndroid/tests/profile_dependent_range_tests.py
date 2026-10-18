"""OoT options whose range depends on other settings, from a preset, a draft or an import to the native check.

SoH's option callbacks narrow some counts for the other settings (ChangeOptions): the Triforce piece counts to OoT's
share of the Triforce Hunt, the key ring count to the Fortress keys, the bridge/GBK/soul/win condition counts to the
Greg reward option and the price ranges to the Tycoon's Wallet. The native lists are narrowed for the live CVars at
boot, so neither the profile catalog nor the native check may use them for a profile with other settings.

Builds the profile catalog as SOH_DumpProfileCatalog exports it, from every OPT_U8/OPT_BOOL declaration in
settings.cpp (declared choices, source defaults), and runs the production policy View (ProfileOotOptionPolicy.h),
ApplyToSetup and the extracted SOH_ApplyProfileRandoSettingsImpl against a Settings narrowed as at a boot without the
Triforce Hunt:

1. Applying each community preset (catalog defaults, then the preset's writes, as DrawHome does) leaves every count in
   its range: the four Triforce counts read 0, not their source default 100, and the native check accepts the draft.
   With the lists the catalog used to export (narrowed at boot), the policy kept 100 in a 0..0 list: the rejected
   "Invalid native settings profile" of the emulator check.
2. A Triforce Hunt draft keeps the counts its share allows (12 of OoT's 15) although the boot lists hold 0..0, and a
   count past the share is clamped to it.
3. The key ring count, a Greg count and a price bound follow their own draft's settings; a hidden price bound the
   draft's settings allow stays as chosen.
4. The native check still rejects a value past an option's declared choices, a negative value and a non-integer.
5. The launcher shows, validates and applies drafts through ApplyToSetup, and the catalog exports declared choices.
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
p.add_argument('--json-include', type=Path, required=True)
a = p.parse_args()
root, work = a.source.resolve(), a.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)


def read(rel):
    return (root / rel).read_text(encoding='utf-8')


TOKENS = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[(){}\[\]]', re.S)


def balanced(text, opening):
    """Index just past the bracket that closes text[opening], skipping comments and literals."""
    depth = 0
    for token in TOKENS.finditer(text, opening):
        if token.group() in '({[':
            depth += 1
        elif token.group() in ')}]':
            depth -= 1
            if depth == 0:
                return token.end()
    raise AssertionError('unbalanced production text')


def split_args(text):
    args, depth, start, i, quote = [], 0, 0, 0, None
    while i < len(text):
        c = text[i]
        if quote:
            if c == '\\':
                i += 1
            elif c == quote:
                quote = None
        elif c in '"\'':
            quote = c
        elif c in '({[':
            depth += 1
        elif c in ')}]':
            depth -= 1
        elif c == ',' and depth == 0:
            args.append(text[start:i].strip())
            start = i + 1
        i += 1
    args.append(text[start:].strip())
    return args


def body(text, signature):
    at = hostcxx.find(text, signature)
    return text[at:balanced(text, text.index('{', at))]


settings = read('soh/soh/Enhancements/randomizer/settings.cpp')
otr = read('soh/soh/OTRGlobals.cpp')
native_inc = read('combo/ComboRandomizerProfileNative.inc')

# ---- Every option declaration (the COMBO_BUILD Mask Quest comes first), as Option::U8/Option::Bool read it.
declarations = {}
for m in re.finditer(r'^\s*OPT_(U8|BOOL)\(', settings, re.M):
    opening = m.end() - 1
    args = split_args(settings[opening + 1:balanced(settings, opening) - 1])
    kind, rsk = m.group(1), args[0]
    if len(args) < 3:
        continue  # no CVar, so not in the catalog
    if kind == 'BOOL' and not args[2].startswith('{'):
        cvar, choices, default = args[2], '{"Off", "On"}', args[6] if len(args) > 6 else 'false'
    else:
        choices = args[2]
        cvar = args[4] if len(args) > 4 else '""'
        default = args[7] if len(args) > 7 else '0'
    key = re.fullmatch(r'CVAR_RANDOMIZER_SETTING\("(\w+)"\)', cvar)
    if key and rsk not in declarations:
        declarations[rsk] = ('gRandoSettings.' + key.group(1), choices, default)
assert len(declarations) > 250, len(declarations)
ranged = set(re.findall(r'mOptions\[(RSK_\w+)\]\.ChangeOptions\(', settings))
assert ranged <= declarations.keys(), ranged - declarations.keys()
assert {'RSK_RAINBOW_BRIDGE_TRIFORCE_COUNT', 'RSK_GBK_TRIFORCE_COUNT', 'RSK_GANONS_SOUL_TRIFORCE_COUNT',
        'RSK_WINCON_TRIFORCE_COUNT', 'RSK_KEYRINGS_RANDOM_COUNT', 'RSK_SHOPSANITY_PRICES_RANGE_2'} <= ranged
rows = ''.join(f'        {{ "{key}", {rsk}, std::vector<std::string>{choices}, int({default}), {str(rsk in ranged).lower()} }},\n'
               for rsk, (key, choices, default) in declarations.items())
# Defaults that name a randomizerTypes.h constant (MAX_MQ_DUNGEON_COUNT) rather than an option enum.
types = read('soh/soh/Enhancements/randomizer/randomizerTypes.h')
constants = ''.join(f'#define {name} {types_value.group(1)}\n'
                    for name in sorted({d for _, _, d in declarations.values() if re.fullmatch(r'[A-Z_]+', d)
                                        and not d.startswith('RO_')})
                    for types_value in [re.search(r'#define ' + name + r'\s+(\d+)', types)])

# ---- Production pieces the harness compiles or checks.
validator = body(otr, 'static int SOH_ApplyProfileRandoSettingsImpl(const char* raw, bool apply)')
catalog_dump = body(otr, 'extern "C" COMBO_EXPORT const char* SOH_DumpProfileCatalog(void)')
assert 'option.GetDeclaredOptions()' in catalog_dump and 'GetOptionCount' not in catalog_dump, \
    'SOH_DumpProfileCatalog must export the declared choices, not the lists narrowed for the live CVars'
draft_setup = body(native_inc, 'static nlohmann::json ProfileDraftSetup(const nlohmann::json& requested)')
assert hostcxx.contains(draft_setup, 'ComboRando::ProfileOotPolicy::ApplyToSetup(sProfileCatalog["oot"],setup);')
for signature in ('static void ProfileValidateNative(const nlohmann::json& requested)',
                  'static void ProfileApplyNative(const nlohmann::json& requested)'):
    function = body(native_inc, signature)
    assert 'ProfileModeSetup(requested)' in function and 'ProfileEditableSetup(' not in function, signature
assert 'ProfileDraftSetup(requested)' in body(native_inc, 'static nlohmann::json ProfileModeSetup(const nlohmann::json& requested)')
assert hostcxx.contains(native_inc, 'value["draft"]=ProfileDraftSetup(value["draft"]);')

code = r'''
#include <cassert>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include "ProfileOotOptionPolicy.h"
#include "rando/ComboPresets.h"
#include "rando/OotMaskQuest.h"
using Json = nlohmann::json;
namespace P = ComboRando::ProfileOotPolicy;
''' + constants + r'''struct Declared {
    const char* key;
    int id;
    std::vector<std::string> choices;
    int defaultValue;
    bool ranged;
};
static const std::vector<Declared>& Declarations() {
    using namespace ComboRando::ProfileOotPolicy;
    static const std::vector<Declared> rows = {
''' + rows + r'''    };
    return rows;
}
// The catalog as SOH_DumpProfileCatalog exports it, from the given choice lists.
static Json Catalog(const std::map<std::string, std::vector<std::string>>& lists) {
    Json catalog = { { "defaults", Json::object() }, { "options", Json::object() } };
    for (const auto& row : Declarations()) {
        Json choices = Json::array();
        const auto& list = lists.at(row.key);
        for (size_t i = 0; i < list.size(); ++i)
            if (row.id != P::RSK_MASK_QUEST || i != P::RO_MASK_QUEST_VANILLA)
                choices.push_back({ { "value", i }, { "label", list[i] } });
        catalog["defaults"][row.key] = row.defaultValue;
        catalog["options"][row.key] = { { "label", row.key }, { "id", row.id }, { "defaultHidden", false },
                                        { "choices", std::move(choices) } };
    }
    return catalog;
}
static std::map<std::string, std::vector<std::string>> DeclaredLists() {
    std::map<std::string, std::vector<std::string>> lists;
    for (const auto& row : Declarations())
        lists[row.key] = row.choices;
    return lists;
}
// The lists SoH's callbacks leave at boot for the given live CVars, starting from the declared ones.
static std::map<std::string, std::vector<std::string>> BootLists(const Json& live) {
    const P::View boot(Catalog(DeclaredLists()), [&](const char* key, int fallback) {
        return live.contains(key) ? live[key].get<int>() : fallback;
    });
    std::map<std::string, std::vector<std::string>> lists;
    for (const auto& [id, option] : boot.mOptions)
        if (!option.key.empty())
            lists[option.key] = option.choices;
    return lists;
}

// ---- Native check, extracted, over a Settings whose lists were narrowed at boot.
#define CVAR_RANDOMIZER_SETTING(s) "gRandoSettings." s
std::map<std::string, int> cvars;
std::map<std::string, std::string> stringCvars;
void CVarSetInteger(const char* key, int value) { cvars[key] = value; }
void CVarSetString(const char* key, const char* value) { stringCvars[key] = value; }
constexpr int RSK_MASK_QUEST = P::RSK_MASK_QUEST;
namespace Rando {
struct BootOption {
    std::string key;
    int id;
    std::vector<std::string> declared;
    size_t live;
    int defaultValue;
    const std::string& GetCVarName() const { return key; }
    int GetMenuOptionDefault() const { return defaultValue; }
    size_t GetOptionCount() const { return live; }
    const std::vector<std::string>& GetDeclaredOptions() const { return declared; }
};
struct Settings {
    std::vector<BootOption> options;
    static Settings* GetInstance() { static Settings settings; return &settings; }
    const std::vector<BootOption>& GetAllOptions() const { return options; }
    const BootOption& GetOption(int id) const {
        for (const auto& option : options)
            if (option.id == id)
                return option;
        assert(false);
        return options.front();
    }
};
}
''' + validator + r'''
static bool NativeAccepts(const Json& settings) {
    const auto raw = settings.dump();
    return SOH_ApplyProfileRandoSettingsImpl(raw.c_str(), false) == 1;
}

static Json Draft(const Json& catalog, const Json& combo = Json::object()) {
    Json draft = { { "oot", { { "settings", catalog.at("defaults") } } }, { "setupCVars", combo } };
    return draft;
}
static int Value(const Json& draft, const char* key) { return draft["oot"]["settings"].at(key).get<int>(); }
static Json& Set(Json& draft, const char* key, int value) {
    draft["oot"]["settings"][key] = value;
    return draft;
}
// Every count within the range the draft's own settings give it, and applying the policy again changes nothing.
static void AssertWithinOwnRanges(const Json& catalog, const Json& draft) {
    Json again = draft;
    P::ApplyToSetup(catalog, again);
    assert(again == draft);
    const auto& settings = draft["oot"]["settings"];
    const auto& combo = draft["setupCVars"];
    const P::View view(catalog, [&](const char* key, int fallback) {
        const auto& fields = std::string(key).rfind("gRandoSettings.", 0) == 0 ? settings : combo;
        return fields.contains(key) ? fields[key].get<int>() : fallback;
    });
    for (const auto& [id, option] : view.mOptions)
        if (!option.key.empty() && !option.choiceValues.empty())
            assert(settings.at(option.key).get<int>() <= option.choiceValues.back());
}

int main() {
    const char* counts[] = { "gRandoSettings.TriforcePieceCount", "gRandoSettings.GbkTriforceCount",
                             "gRandoSettings.GanonsSoulTriforceCount", "gRandoSettings.WinconTriforceCount" };
    const Json catalog = Catalog(DeclaredLists());
    // The device booted without the Triforce Hunt, as on the emulator: the four counts were narrowed to 0..0.
    const auto boot = BootLists(catalog.at("defaults"));
    for (const char* key : counts)
        assert(boot.at(key).size() == 1);
    auto& native = Rando::Settings::GetInstance()->options;
    for (const auto& row : Declarations())
        native.push_back({ row.key, row.id, row.choices, boot.at(row.key).size(), row.defaultValue });

    // 1. Every community preset, applied as DrawHome applies it.
    for (const auto& preset : ComboRando::kCommunityPresets) {
        Json draft = Draft(catalog);
        for (const auto& option : ComboRando::CommunityPresetOptions(preset.id)) {
            const std::string key = option.key;
            if (key.rfind("gRandoSettings.", 0) == 0)
                draft["oot"]["settings"][key] = option.value;
            else if (key.rfind("gCombo.", 0) == 0)
                draft["setupCVars"][key] = option.value;
        }
        Json unpolicied = draft;
        P::ApplyToSetup(catalog, draft);
        for (const char* key : counts) {
            assert(Value(unpolicied, key) == 100 && Value(draft, key) == 0);
            assert(Value(unpolicied, key) >= int(boot.at(key).size())); // the old check against the boot list
        }
        AssertWithinOwnRanges(catalog, draft);
        assert(NativeAccepts(draft["oot"]["settings"]));
        // What the catalog used to export: the boot lists. The policy then left 100 in a 0..0 list.
        Json before = unpolicied;
        P::ApplyToSetup(Catalog(boot), before);
        assert(Value(before, counts[0]) == 100);
        std::cout << "Preset " << preset.name << ": Triforce counts 0, native check accepts\n";
    }

    // 2. Triforce Hunt with 30 pieces: OoT's share is 15 (the odd piece), whatever the boot lists hold.
    {
        Json draft = Draft(catalog, { { "gCombo.Rando.TriforceHunt", 1 }, { "gCombo.Rando.TriforceTotal", 30 } });
        Set(draft, "gRandoSettings.RainbowBridge", P::RO_BRIDGE_TRIFORCE_PIECES);
        Set(draft, counts[0], 12);
        P::ApplyToSetup(catalog, draft);
        assert(Value(draft, counts[0]) == 12);
        for (int i = 1; i < 4; ++i)
            assert(Value(draft, counts[i]) == 15);
        AssertWithinOwnRanges(catalog, draft);
        assert(NativeAccepts(draft["oot"]["settings"]));
        Set(draft, counts[0], 40);
        P::ApplyToSetup(catalog, draft);
        assert(Value(draft, counts[0]) == 15);
        std::cout << "Triforce Hunt draft: counts kept within OoT's share of 15 and accepted after a boot without it\n";
    }

    // 3. Counts that follow other OoT settings.
    {
        const char* keyRings = "gRandoSettings.ShuffleKeyRingsRandomCount";
        Json draft = Draft(catalog);
        Set(draft, "gRandoSettings.ShuffleKeyRings", P::RO_KEYRINGS_COUNT);
        Set(draft, "gRandoSettings.FortressCarpenters", P::RO_GF_CARPENTERS_NORMAL);
        Set(draft, "gRandoSettings.GerudoKeys", P::RO_GERUDO_KEYS_ANY_DUNGEON);
        Set(draft, keyRings, 9);
        P::ApplyToSetup(catalog, draft);
        assert(Value(draft, keyRings) == 9 && NativeAccepts(draft["oot"]["settings"]));
        Set(draft, "gRandoSettings.GerudoKeys", P::RO_GERUDO_KEYS_VANILLA);
        P::ApplyToSetup(catalog, draft);
        assert(Value(draft, keyRings) == 8); // the Fortress has no key ring: eight dungeons can have one

        const char* stones = "gRandoSettings.StoneCount";
        Set(draft, "gRandoSettings.RainbowBridge", P::RO_BRIDGE_STONES);
        Set(draft, "gRandoSettings.BridgeRewardOptions", P::RO_BRIDGE_GREG_REWARD);
        Set(draft, stones, 4);
        P::ApplyToSetup(catalog, draft);
        assert(Value(draft, stones) == 4);
        Set(draft, "gRandoSettings.BridgeRewardOptions", P::RO_BRIDGE_STANDARD_REWARD);
        P::ApplyToSetup(catalog, draft);
        assert(Value(draft, stones) == 3);

        const char* upper = "gRandoSettings.ShopsanityPriceRange2";
        Set(draft, "gRandoSettings.ShopsanityPrices", P::RO_PRICE_RANGE);
        Set(draft, "gRandoSettings.IncludeTycoonWallet", 1);
        Set(draft, upper, 150);
        P::ApplyToSetup(catalog, draft);
        assert(Value(draft, upper) == 150 && NativeAccepts(draft["oot"]["settings"]));
        // Fixed prices hide the bounds, so a fresh native Settings would not narrow them either: the value stays.
        Set(draft, "gRandoSettings.ShopsanityPrices", P::RO_PRICE_FIXED);
        Set(draft, "gRandoSettings.IncludeTycoonWallet", 0);
        P::ApplyToSetup(catalog, draft);
        assert(Value(draft, upper) == 150 && NativeAccepts(draft["oot"]["settings"]));
        Set(draft, "gRandoSettings.ShopsanityPrices", P::RO_PRICE_RANGE);
        P::ApplyToSetup(catalog, draft);
        assert(Value(draft, upper) == 100); // 500 rupees, the most without the Tycoon's Wallet
        AssertWithinOwnRanges(catalog, draft);
        std::cout << "Key ring, Greg and price counts follow the draft's own settings\n";
    }

    // 4. The native check stays strict about the declared choices.
    {
        Json settings = Draft(catalog)["oot"]["settings"];
        assert(NativeAccepts(settings));
        settings["gRandoSettings.ShuffleKeyRingsRandomCount"] = 10;
        assert(!NativeAccepts(settings));
        settings["gRandoSettings.ShuffleKeyRingsRandomCount"] = -1;
        assert(!NativeAccepts(settings));
        settings["gRandoSettings.ShuffleKeyRingsRandomCount"] = "8";
        assert(!NativeAccepts(settings));
        settings["gRandoSettings.ShuffleKeyRingsRandomCount"] = 8;
        settings[counts[0]] = 101;
        assert(!NativeAccepts(settings));
        settings[counts[0]] = 100;
        assert(NativeAccepts(settings) && cvars.empty());
        assert(SOH_ApplyProfileRandoSettingsImpl(settings.dump().c_str(), true) == 1 && cvars.at(counts[0]) == 100);
    }
    std::cout << "PASS: dependent OoT ranges follow each profile's own settings\n";
}
'''

source = work / 'profile_dependent_ranges.cpp'
source.write_text(code, encoding='utf-8')
binary = hostcxx.build(a.compiler, source, warnings=False,
                       includes=[root / 'combo/gui', root / 'combo', a.json_include.resolve()],
                       msvc=['/Zc:preprocessor', '/utf-8', '/bigobj'])
subprocess.run([str(binary)], check=True, cwd=work, timeout=hostcxx.RUN_TIMEOUT)
print(f'Checked {len(declarations)} OoT option declarations, {len(ranged)} with ranges narrowed by other settings')
