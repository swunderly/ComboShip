"""Compile the actual headless saved-entrance selection and recorded replay paths."""
from pathlib import Path
import argparse
import os
import subprocess
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
text = (root/'combo/ComboRandoHeadless.cpp').read_text(encoding='utf-8')
helpers = text[hostcxx.find(text, 'struct HeadlessEntranceSettings {'):
               text.index('// Same module-loading seam')]
settings_start = hostcxx.find(text, 'ComboRando::RequireSupportedSettingsCapabilities(sj,')
settings_end = hostcxx.find(text, 'const auto& oot = sj.contains("oot")', settings_start)
settings = text[settings_start:settings_end]
assert settings_end < hostcxx.find(text, 'SOH_RestoreSettings(oot["settings"].dump().c_str());', settings_end)
assert hostcxx.find(text, 'CheckHeadlessEntranceReplay(spoiler, entranceArgs,') < \
    hostcxx.find(text, 'mmSettings = MmSettingsForPlaythrough(spoiler);')
replay_start = hostcxx.find(text, 'if (SOH_ApplyEntranceOverrides) {')
replay_end = text.index('// Spoiler prices are the seed', replay_start)
replay = text[replay_start:replay_end]
assert 'ApplyNativeSeed(entranceNative, spoiler,' in replay
assert 'GenerateEntranceLayout' not in text[text.index('// ---- Playthrough mode:'):
                                           text.index('// ---- Generate + validate mode.')]

program = r'''
#include "rando/ComboSeedFormat.h"
#include "rando/entrances/NativeBridge.h"
#include "rando/entrances/MmUnifiedLayout.h"
#include <cassert>
#include <iostream>
using Json = nlohmann::json;
namespace CE = ComboRando::Entrances;
''' + helpers + r'''
int settingsMutations = 0;
HeadlessEntranceSettings RestoreSettings(const Json& sj, const std::vector<std::string>& entrancePairs = {},
                                       std::string entranceCapability = {}, bool privateMm = false) {
    CE::Settings entranceSettings;
    bool nativeMmOrdinaryIndoors = privateMm, nativeMmOrdinaryTelescopes = false;
    bool nativeMmOrdinaryOverworld = false, nativeMmOrdinaryDungeons = false;
    bool nativeMmMixed = false, nativeMmGrottos = false, nativeMmWallmasters = false;
''' + settings + r'''
    ++settingsMutations; // first possible native restore, after the real selection/preflight block
    return {entranceSettings, entranceCapability};
}
template<class F> void Reject(F f) {
    const int before = settingsMutations;
    bool rejected = false;
    try { f(); } catch (const std::exception&) { rejected = true; }
    assert(rejected && settingsMutations == before);
}
Json Seed() {
    CE::EntranceLayout layout;
    layout.required = {std::string(CE::kMmTrancheCapability)};
    layout.devBuild = true;
    layout.settings.erWallmasters = CE::Mode::OwnGame;
    CE::Overrides routes;
    const char* destinations[] = {"MM_TEMPLE_WOODFALL", "MM_TEMPLE_SNOWHEAD", "MM_TEMPLE_GREAT_BAY",
                                  "MM_SPIDER_HOUSE_SWAMP"};
    for (size_t i = 0; i < CE::kMmWallmasterSources.size(); ++i)
        routes[CE::LayoutV1::FindRow(CE::kMmWallmasterSources[i])->name] =
            CE::LayoutV1::FindRow(destinations[i])->name;
    layout.overrides = routes;
    Json seed{{"entranceLayout", CE::EncodeEntranceLayout(layout)}};
    ComboRando::StampComboSeedFormat(seed);
    return seed;
}
CE::MmUnifiedLayout installed;
int applyCalls = 0, shuffleCalls = 0;
int ApplySeed(const char* raw) {
    ++applyCalls;
    const auto seed = Json::parse(raw);
    if (seed.contains("entranceLayout"))
        installed.install(seed, CE::ActiveCapabilities());
    else
        installed.clear();
    return 1;
}
const char* DumpSeed() {
    static std::string value;
    value = (installed.active() ? Json{{"entranceLayout", installed.json()}} : Json::object()).dump();
    return value.c_str();
}
int Shuffle(uint64_t) { ++shuffleCalls; return 1; }
bool ReplayPass(const Json& spoiler, const Json& ootEntrances) {
    int (*SOH_ApplyEntranceOverrides)(const char*) = nullptr;
    auto SOH_ShuffleEntrances = &Shuffle;
    uint64_t masterSeed = 17;
    bool entranceRestoreFailed = false;
    CE::NativeFns entranceNative{&ApplySeed, nullptr, nullptr, &DumpSeed};
''' + replay + r'''
    return !entranceRestoreFailed;
}
int main() {
    const Json seed = Seed();
    const std::string bytes = seed.dump();
    // Native settings and legacy consolidated seeds retain no-layout defaults.
    for (const Json& old : {Json{{"settings", Json::object()}},
                            Json{{"fileType", ComboRando::kComboSeedFileType}}}) {
        const auto selected = RestoreSettings(old);
        assert(CE::SameEntranceSettings(selected.settings, CE::Settings{}) && selected.capability.empty());
        CheckHeadlessEntranceReplay(old, true, true); // historical no-layout replay flags remain inert
    }
    for (const char* key : ComboRando::kLegacyEntranceKeys) {
        const Json old{{key, Json::object()}};
        RestoreSettings(old); // the existing per-profile decoder still owns legacy format validation
        Reject([&] { RestoreSettings(old, {"erWallmasters=none"}); });
        Reject([&] { RestoreSettings(old, {}, std::string(CE::kMmTrancheCapability)); });
        Json mixed = seed; mixed[key] = Json::object();
        Reject([&] { RestoreSettings(mixed); });
    }
    Reject([&] { RestoreSettings(Json::object(), {"erWallmasters=none"}, "", true); });
    Reject([&] { CheckHeadlessEntranceReplay(seed, true, false); });
    Reject([&] { CheckHeadlessEntranceReplay(seed, false, true); });
    CheckHeadlessEntranceReplay(seed, false, false);
#ifdef COMBO_ENTRANCE_DEV
    const auto saved = CE::DecodeEntranceLayout(seed.at("entranceLayout"));
    const auto selected = RestoreSettings(seed);
    assert(CE::SameEntranceSettings(selected.settings, saved.settings));
    assert(selected.capability == saved.required.front());
    // Settings-only profile documents inherit the same exact settings and capability.
    Json profile = seed; profile["entranceLayout"].erase("overrides");
    assert(CE::SameEntranceSettings(RestoreSettings(profile).settings, saved.settings));
    // A per-key CLI override wins, even when the saved capability was the only way to admit the seed.
    const auto off = RestoreSettings(seed, {"erWallmasters=none"});
    assert(off.settings.erWallmasters == CE::Mode::None && off.capability == selected.capability);
    assert(RestoreSettings(seed, {"erWallmasters=ownGame"}).settings.erWallmasters == CE::Mode::OwnGame);
    Reject([&] { RestoreSettings(seed, {}, "unknown-explicit-capability"); });
    Reject([&] { RestoreSettings(seed, {"erWallmasters=none", "erWallmasters=none"}); });
    Reject([&] { RestoreSettings(seed, {"erOneWays=ownGame"}); });
    Reject([&] { RestoreSettings(seed, {}, "", true); });
    for (const char* key : {"requires", "settings", "overrides"}) {
        auto invalid = seed; invalid["entranceLayout"][key] = nullptr;
        Reject([&] { RestoreSettings(invalid, {"erWallmasters=none"}); });
    }
    // A future public capability override still takes precedence; the original envelope is validated.
    auto all = CE::CapabilitySet::AtLeast(CE::Stage::Hidden); all.devBuild = true;
    const auto overridden = EntranceSettingsForHeadless(seed, {}, "wallmasters-own-game-v1", false, all);
    assert(overridden.capability == "wallmasters-own-game-v1");
    assert(CE::SameEntranceSettings(overridden.settings, saved.settings));
    // Both actual traversal passes install exactly the recorded envelope, never generate replacements.
    for (int pass = 0; pass < 2; ++pass) {
        assert(ReplayPass(seed, Json::array()));
        assert(installed.json() == seed.at("entranceLayout"));
        assert(installed.routes().size() == 4);
    }
    assert(applyCalls == 2 && shuffleCalls == 0);
    // Even with native OoT rows present, a missing apply export cannot reroll a unified replay.
    assert(!ReplayPass(seed, Json::array({Json::object()})) && shuffleCalls == 0);
#else
    Reject([&] { RestoreSettings(seed); });
    Reject([&] { RestoreSettings(seed, {"erWallmasters=none"}); });
    assert(!ReplayPass(seed, Json::array()) && applyCalls == 0 && !installed.active());
#endif
    // Preserve the old fallback solely for legacy no-layout seeds.
    assert(ReplayPass(Json{{"fileType", ComboRando::kComboSeedFileType}}, Json::array({Json::object()})));
    assert(shuffleCalls == 1);
    assert(seed.dump() == bytes);
    std::cout << "saved entrance settings, CLI precedence and immutable recorded replay passed\n";
}
'''
source = work/'headless_entrance_reproduction.cpp'
source.write_text(program, encoding='utf-8')
for name, defines in (('shipping', []), ('dev', ['COMBO_ENTRANCE_DEV=1'])):
    exe = hostcxx.build(args.compiler, source, includes=[root/'combo', args.json_include], defines=defines,
                        exe=work/f'headless_entrance_reproduction_{name}{".exe" if os.name == "nt" else ""}')
    subprocess.run([str(exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)
