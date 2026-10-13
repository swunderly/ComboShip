"""Home > Apply preset: one tap applies it, without freezing the menu for seconds.

Every WriteInteger refreshes the MM pool view, a walk over every MM check (thousands of reads), and a preset makes a
hundred-odd writes. On the emulator recheck the confirming frame took that long: the screen kept showing the pressed
Apply preset button and the draft was still unchanged when it was checked, so the first tap looked ignored and a
second tap seemed to apply it. This runs the production WriteInteger and ApplyCommunityPreset (ComboRandomizerProfiles
.cpp) with counted refreshes on each community preset (ComboPresets.h, as ProfileComboCatalog exports it):
- the preset refreshes the pool view once, after its writes, and still re-applies the option policy after each write;
- the preset's values are written, including Goal (Ganon & Majora), and the file is marked for saving;
- an ordinary edit outside a preset still refreshes the pool view.
"""
import argparse
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
harness = r'''
#include <nlohmann/json.hpp>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include "ProfileOotOptionPolicy.h"
#include "rando/ComboPresets.h"
#include "rando/GameplayRules.h"
#include "rando/GameplayRuleSnapshots.h"
#include "rando/GenerationRules.h"
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
        catalog["oot"]["defaults"] = { { "gRandoSettings.LogicRules", 0 }, { "gRandoSettings.ShuffleSongs", 0 } };
        catalog["mm"]["defaults"] = { { "gRando.Options.RO_LOGIC", 0 }, { "gRando.StartingItems", Json::array() } };
        for (const auto& preset : kCommunityPresets) {
            auto writes = Json::object();
            for (const auto& option : CommunityPresetOptions(preset.id))
                writes[option.key] = option.value;
            catalog["combo"]["presets"].push_back({ { "id", int(preset.id) }, { "label", preset.name }, { "writes", writes } });
            catalog["mm"]["presetStartingItems"][std::to_string(int(preset.id))] = Json::array({ preset.name });
        }
        for (const auto& preset : catalog["combo"]["presets"]) {
            // A file whose next seed was a 30-piece Triforce Hunt with an MM option of its own.
            document = { { "oot", { { "settings", Json::object() } } },
                         { "mm", { { "settings", { { "gRando.Options.RO_SHUFFLE_COWS", 1 } } } } },
                         { "setupCVars", { { "gCombo.Rando.Goal", 1 }, { "gCombo.Rando.TriforceHunt", 1 },
                                           { "gCombo.Rando.TriforceTotal", 30 } } } };
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
            Require(ReadInteger("gCombo.Rando.Goal", -1) == 0 && ReadInteger("gCombo.Rando.TriforceTotal", -1) == 15,
                    "a preset kept the file's Triforce Hunt");
            Require(ReadInteger("gRando.Options.RO_SHUFFLE_COWS", -1) == preset.at("writes").value(
                        "gRando.Options.RO_SHUFFLE_COWS", -1),
                    "a preset kept an MM option it does not set");
            Require(document["mm"]["settings"]["gRando.StartingItems"] == Json::array({ preset.at("label") }),
                    "a preset's starting items were not written");
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
