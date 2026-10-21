"""Home > Apply preset: one tap applies it, without freezing the menu for seconds.

Every WriteInteger refreshes the MM pool view, a walk over every MM check (thousands of reads), and a preset makes a
hundred-odd writes. On the emulator recheck the confirming frame took that long: the screen kept showing the pressed
Apply preset button and the draft was still unchanged when it was checked, so the first tap looked ignored and a
second tap seemed to apply it. This runs the production WriteInteger and ApplyCommunityPreset (ComboRandomizerProfiles
.cpp) with counted refreshes on each community preset (ComboPresets.h, as ProfileComboCatalog exports it):
- the preset refreshes the pool view once, after its writes, and still re-applies the option policy after each write;
- the preset's values are written, including Goal (Ganon & Majora), and the file is marked for saving;
- its gameplay rules turn on in play and in logic, other rules leave logic, and its junked checks replace the file's;
- its MM starting items, built by the production MM_DumpProfileCatalog block (BenPort.cpp), are OoTMM's and are
  written, and a file without its own list keeps 2Ship's four;
- an ordinary edit outside a preset still refreshes the pool view.
"""
import argparse
import re
import subprocess
from pathlib import Path
import hostcxx

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--json-include', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
args = p.parse_args()
source, out = args.source.resolve(), args.build_dir.resolve()
out.mkdir(parents=True, exist_ok=True)
text = (source / 'combo/gui/ComboRandomizerProfiles.cpp').read_text(encoding='utf-8')


def function(start):
    pos = hostcxx.find(text, start)
    opening = text.index('{', pos)
    depth, closing = 1, opening + 1
    while depth:
        depth += (text[closing] == '{') - (text[closing] == '}')
        closing += 1
    return text[pos:closing] + '\n'


assert hostcxx.contains(text, 'if (MenuConfirm("Apply preset?", presetText.c_str(), "Apply preset") && Editable())'
                              ' ApplyCommunityPreset(preset);'), 'Home no longer applies presets through the tested code'
# MM_DumpProfileCatalog's starting items per preset (BenPort.cpp), with each item's spoiler name from 2Ship's table:
# the item id itself (Items.cpp RI(), #id), so the catalogue, like a seed, names Soaring RI_SONG_SOARING.
benport = (source / 'mm/2s2h/BenPort.cpp').read_text(encoding='utf-8')
begin = hostcxx.find(benport, 'const std::vector<RandoItemId> base =')
finish = hostcxx.find(benport, 'data["defaults"]["gRando.StartingItems"] = itemNames(base);', begin)
mm_starting_items = benport[begin:benport.index(';', finish) + 1]
items_table = (source / 'mm/2s2h/Rando/StaticData/Items.cpp').read_text(encoding='utf-8')
assert hostcxx.contains(items_table, 'id, #id, article, name, type, itemId, getItemId, drawId')
item_ids = sorted(set(re.findall(r'\bRI_\w+\b', mm_starting_items)))
harness = r'''
#include <nlohmann/json.hpp>
#include <cstdio>
#include <cstring>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
#include "ProfileOotOptionPolicy.h"
#include "rando/ComboPresets.h"
#include "rando/GameplayRules.h"
#include "rando/entrances/EntranceProfile.h"
#include "rando/GameplayRuleSnapshots.h"
#include "rando/GenerationRules.h"
enum RandoItemId { ''' + ', '.join(item_ids) + r''' };
namespace Rando::StaticData {
struct Item {
    const char* spoilerName;
};
static const std::map<RandoItemId, Item> Items = { ''' + \
    ', '.join(f'{{ {item}, {{ "{item}" }} }}' for item in item_ids) + r''' };
} // namespace Rando::StaticData
static const char* MM_ProfileCatalogText(const char* value) {
    return value ? value : "";
}
static nlohmann::json MmStartingItemsCatalog() {
    nlohmann::json data;
''' + mm_starting_items + r'''
    return data;
}
namespace ComboRando::ProfileUI {
using Json = nlohmann::json;
static int selected = 0, view = 0, policies = 0, pools = 0;
static bool active = true, dirty = false, deferPoolViews = false;
static Json document, catalog;
static void RefreshOptionPolicy(bool apply) {
    if (!apply)
        throw std::logic_error("edits re-apply the option policy");
    ++policies;
}
static void RefreshPoolViews() {
    ++pools;
}
static void WarnAboutRuleChange(const char*, int) {
}
''' + function('bool Editable()') + function('static Json& RuleBlock()') + function('static Json* Field(') + \
    function('int ReadInteger(') + function('void WriteInteger(') + \
    function('static void ApplyCommunityPreset(const Json& preset)') + r'''
} // namespace ComboRando::ProfileUI
using namespace ComboRando;
using namespace ComboRando::ProfileUI;
static void Require(bool condition, const char* message) {
    if (!condition)
        throw std::runtime_error(message);
}
int main() {
    try {
        // The catalog as the launcher exports it: native defaults, and the presets with their writes.
        catalog["oot"]["defaults"] = { { "gRandoSettings.LogicRules", 0 }, { "gRandoSettings.ShuffleSongs", 0 },
                                       { "gRandoSettings.ExcludedLocations", "" } };
        // MM's starting items: OoTMM's Default and Allsanity start with nothing (startingItems {}), Beginner with two
        // swords, the shared Hylian Shield, the Ocarina, Soaring and the Song of Time (presets.ts:1023-1032,1059).
        const Json mm = MmStartingItemsCatalog();
        const Json base = Json::array({ "RI_PROGRESSIVE_SWORD", "RI_SHIELD_HERO", "RI_OCARINA", "RI_SONG_TIME" });
        Require(mm.at("defaults").at("gRando.StartingItems") == base,
                "a file without its own MM list lost 2Ship's start");
        Json beginner = base;
        beginner.push_back("RI_PROGRESSIVE_SWORD");
        beginner.push_back("RI_SONG_SOARING");
        Require(mm.at("presetStartingItems") ==
                    Json::object({ { "0", Json::array() }, { "1", beginner }, { "2", Json::array() } }),
                "a preset's MM starting items are not OoTMM's");
        catalog["mm"]["defaults"] = { { "gRando.Options.RO_LOGIC", 0 },
                                      { "gRando.StartingItems", mm.at("defaults").at("gRando.StartingItems") },
                                      { "gRando.ExcludedChecks", Json::array() } };
        catalog["mm"]["presetStartingItems"] = mm.at("presetStartingItems");
        for (const auto& preset : kCommunityPresets)
            catalog["combo"]["presets"].push_back(CommunityPresetJson(preset)); // as ProfileComboCatalog exports it
        for (const auto& preset : catalog["combo"]["presets"]) {
            // A file whose next seed was a 12-piece Triforce Hunt with an MM option, junked checks, a logic rule and
            // the hidden Shared Ocarina (an old import) of its own.
            document = { { "oot", { { "settings", { { "gRandoSettings.ExcludedLocations", "12" } } } } },
                         { "mm", { { "settings", { { "gRando.Options.RO_SHUFFLE_COWS", 1 },
                                                   { "gRando.ExcludedChecks", Json::array({ "RC_IKANA_GREAT_FAIRY" }) } } } } },
                         { "setupCVars", { { "gCombo.Rando.Goal", 1 }, { "gCombo.Rando.TriforceHunt", 1 },
                                           { "gCombo.Rando.TriforceRequired", 9 }, { "gCombo.Rando.TriforceTotal", 12 },
                                           { "gCombo.Rando.Shared.Ocarina", 1 } } } };
            SetLogicRule(*FindGameplayRule("gEnhancements.DayGravePull"), 1, WriteInteger);
            policies = pools = 0;
            dirty = false;
            ApplyCommunityPreset(preset);
            const int writes = int(preset.at("writes").size());
            std::printf("%s: %d writes, %d option-policy passes, %d pool-view refresh\n",
                        preset.at("label").get<std::string>().c_str(), writes, policies, pools);
            Require(pools == 1, "a preset refreshed the MM pool view more than once");
            Require(policies > writes, "a preset skipped the option policy after a write");
            Require(!deferPoolViews && dirty, "a preset left its refresh deferred or its draft unsaved");
            for (const auto& write : preset.at("writes").items())
                Require(ReadInteger(write.key().c_str(), -1) == write.value().get<int>(), "a preset value was not written");
            // OoTMM's default goal and Triforce counts (data.ts:91-110).
            Require(ReadInteger("gCombo.Rando.Goal", -1) == 0 && ReadInteger("gCombo.Rando.TriforceRequired", -1) == 20 &&
                        ReadInteger("gCombo.Rando.TriforceTotal", -1) == 30,
                    "a preset kept the file's Triforce Hunt");
            for (const auto& rule : kGameplayRules) {
                const int value = preset.at("rules").value(rule.name, 0);
                Require(ReadInteger(LogicRuleKey(rule.name).c_str(), -1) == value, "a preset left a rule's logic as it was");
                if (value)
                    Require(ReadInteger(rule.name, -1) == value, "a preset rule is in logic but off in play");
            }
            Require(ReadInteger("gEnhancements.DayGravePull", -1) == 1, "a preset turned off the player's enhancement");
            Require(document["oot"]["settings"]["gRandoSettings.ExcludedLocations"] ==
                            preset.at("values").at("gRandoSettings.ExcludedLocations") &&
                        document["mm"]["settings"]["gRando.ExcludedChecks"] ==
                            preset.at("values").at("gRando.ExcludedChecks"),
                    "a preset kept the file's junked checks");
            Require(ReadInteger("gCombo.Rando.Shared.Ocarina", -1) == 0, "a preset kept an imported Shared Ocarina");
            Require(ReadInteger("gRando.Options.RO_SHUFFLE_COWS", -1) == preset.at("writes").value(
                        "gRando.Options.RO_SHUFFLE_COWS", -1),
                    "a preset kept an MM option it does not set");
            Require(document["mm"]["settings"]["gRando.StartingItems"] ==
                        mm.at("presetStartingItems").at(std::to_string(preset.at("id").get<int>())),
                    "a preset's MM starting items were not written");
        }
        // An ordinary edit refreshes the pool view at once, as before.
        policies = pools = 0;
        WriteInteger("gRando.Options.RO_SHUFFLE_COWS", 0);
        Require(pools == 1 && policies == 1, "an edit outside a preset did not refresh the menu");
        std::puts("PASS: a community preset refreshes the MM pool view once and writes every value");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "FAIL: %s\n", e.what());
        return 1;
    }
    return 0;
}
'''
cpp = out / 'profile_preset_apply.cpp'
cpp.write_text(harness, encoding='utf-8')
exe = hostcxx.build(args.compiler, cpp, std='c++20', warnings=False,
                    includes=[source / 'combo/gui', source / 'combo', args.json_include.resolve()],
                    msvc=['/Zc:preprocessor', '/utf-8', '/bigobj'])
subprocess.run([str(exe)], check=True, cwd=out, timeout=hostcxx.RUN_TIMEOUT)
