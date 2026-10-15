"""Execute the real plando Load -> edit -> Save & Play path on v2 and legacy seeds.

Only module exports, the active-slot query and the remembered-path CVar are stubbed. The production
JSON parser, placement/foreign serializers, capability guard, file picker, and filesystem writes run.
Sibling text is deliberately misleading; it must not become an item source or modify saved routes.
"""
from pathlib import Path
import argparse
import re
import subprocess
import uuid
import hostcxx

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--json-include', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
args = parser.parse_args()
source = args.source.resolve()
work = args.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
menu = (source/'combo/gui/ComboMenu.cpp').read_text(encoding='utf-8')


def extract(signature):
    start = hostcxx.find(menu, signature)
    opening = menu.index('{', start)
    masked = re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"', lambda m: ' ' * len(m.group()), menu)
    depth = 0
    for end in range(opening, len(menu)):
        depth += (masked[end] == '{') - (masked[end] == '}')
        if depth == 0:
            return menu[start:end + 1]
    raise AssertionError('Unterminated production function')


state = menu[menu.index('struct PlandoPickItem {'):menu.index('static PlandoState sPlando;') + len('static PlandoState sPlando;')]
functions = '\n'.join(extract(name) for name in (
    'void PlandoRefreshSpoilerList()', 'void PlandoBuildItems()', 'void PlandoLoad()', 'void PlandoSavePlay()'))
support = r'''
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_set>
#include "rando/ComboPlaythrough.h"
#include "rando/ComboSeedFormat.h"
namespace fs = std::filesystem;
namespace CE = ComboRando::Entrances;
using Json = nlohmann::json;
using FnDump = const char* (*)(void);
using FnRequestReload = int (*)(const char*);
FnDump sSohDump = nullptr, sMmDump = nullptr;
FnRequestReload sRequestReload = nullptr;
void ResolvePlandoSyms() {}
int activeSlot = -1, dumpCalls = 0, reloadCalls = 0;
namespace ComboTracker { int OotActiveSlot() { return activeSlot; } }
std::string rememberedPath, reloadedPath;
const char* CVarGetString(const char* key, const char*) {
    assert(std::string(key) == "gGeneral.ComboSpoiler"); return rememberedPath.c_str();
}
std::string Read(const fs::path& path) {
    std::ifstream in(path, std::ios::binary); assert(in.is_open());
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
void Write(const fs::path& path, const std::string& text) {
    fs::create_directories(path.parent_path()); std::ofstream out(path, std::ios::binary);
    assert(out.is_open()); out << text;
}
int Reload(const char* path) {
    ++reloadCalls; reloadedPath = path;
    ComboRando::RequireSupportedSeedDocument(Json::parse(Read(path)), CE::ActiveCapabilities());
    return 1;
}
const char* OotDump() { ++dumpCalls; return R"({"items":[{"name":"Kokiri Sword"}],"pool":[{"name":"Kokiri Sword","advancement":true}]})"; }
const char* MmDump() { ++dumpCalls; return R"({"items":[{"name":"Deku Mask"}],"pool":[{"name":"Deku Mask","advancement":true}]})"; }
'''
tests = r'''
Json BaseSeed(bool unified) {
    Json seed = {
        {"fileType", "ComboShipRandomizer"}, {"seed", "m1-plando"}, {"file_hash", {1,2,3,4,5}},
        {"sharedItems", Json::array()}, {"faroreRules", {{"crossAge", true}}},
        {"goal", {{"type", "ganonMajora"}}}, {"startingGame", "mm"},
        {"oot", {{"settings", {{"preserve", 17}}}, {"placements", {{"OOT Native Check", "Kokiri Sword"}, {"OOT Foreign Check", "Deku Mask"}}}}},
        {"mm", {{"settings", {{"preserve", 23}}}, {"placements", {{"MM Native Check", "Deku Mask"}, {"MM Foreign Check", "Kokiri Sword"}}}}},
        {"foreign", Json::array({
            {{"checkGame", "oot"}, {"checkName", "OOT Foreign Check"}, {"itemGame", "mm"}, {"itemName", "Deku Mask"}, {"advancement", true}},
            {{"checkGame", "mm"}, {"checkName", "MM Foreign Check"}, {"itemGame", "oot"}, {"itemName", "Kokiri Sword"}, {"advancement", true}}
        })}
    };
    if (unified) {
        CE::EntranceLayout layout;
        layout.required = {std::string(CE::kMmTrancheCapability)};
        layout.devBuild = true;
        layout.settings.erWallmasters = CE::Mode::OwnGame;
        layout.overrides = CE::Overrides{
            {"MM_WALLMASTER_DAMPE", "MM_TEMPLE_WOODFALL"},
            {"MM_WALLMASTER_BTW_ENTRANCE", "MM_TEMPLE_SNOWHEAD"},
            {"MM_WALLMASTER_BTW_FOUNTAIN", "MM_TEMPLE_GREAT_BAY"},
            {"MM_WALLMASTER_BTW_EXIT", "MM_TEMPLE_STONE_TOWER"}};
        seed["entranceLayout"] = CE::EncodeEntranceLayout(layout);
        ComboRando::StampComboSeedFormat(seed);
    }
    return seed;
}
Json ExceptPlacements(Json seed) {
    seed["oot"].erase("placements"); seed["mm"].erase("placements"); seed.erase("foreign"); return seed;
}
void VerifyRows(const std::vector<ComboRando::CwPlacedItem>& rows, bool edited) {
    assert(rows.size() == 4);
    for (const auto& row : rows) {
        if (row.check == "OOT Native Check") {
            assert(row.checkGame == ComboRando::GAME_OOT);
            assert(row.itemGame == (edited ? ComboRando::GAME_MM : ComboRando::GAME_OOT));
            assert(row.item == (edited ? "Deku Mask" : "Kokiri Sword"));
        } else if (row.check == "OOT Foreign Check") {
            assert(row.checkGame == ComboRando::GAME_OOT && row.itemGame == ComboRando::GAME_MM && row.item == "Deku Mask");
        } else if (row.check == "MM Foreign Check") {
            assert(row.checkGame == ComboRando::GAME_MM && row.itemGame == (edited ? ComboRando::GAME_MM : ComboRando::GAME_OOT));
            assert(row.item == (edited ? "Deku Mask" : "Kokiri Sword"));
        } else {
            assert(row.check == "MM Native Check" && row.checkGame == ComboRando::GAME_MM && row.itemGame == ComboRando::GAME_MM && row.item == "Deku Mask");
        }
    }
}
void Case(const fs::path& root, bool unified, int loadMode) {
    fs::create_directories(root); fs::current_path(root);
    const auto seed = BaseSeed(unified);
    const std::string original = seed.dump(2);
    const fs::path input = ComboRando::ComboSpoilerPath(seed.at("file_hash"));
    auto sibling = input; sibling.replace_extension(".txt");
    const std::string misleading = R"({"oot":{"placements":{"TEXT ONLY CHECK":"TEXT ONLY ITEM"}},"entranceLayout":{"overrides":[]}})";
    Write(input, original); Write(sibling, misleading);
    Write("Randomizer/settings-profiles.json", "{}");
    sPlando = {}; activeSlot = -1; rememberedPath = input.string(); dumpCalls = reloadCalls = 0; reloadedPath.clear();
    PlandoRefreshSpoilerList();
    assert(sPlando.spoilerPaths.size() == 1 && sPlando.spoilerPaths.front() == input.string());
    if (loadMode != 0) sPlando.spoilerSel = -1;
    if (loadMode == 1) {
        activeSlot = 2;
        Write("Save/file3.combosav", Json{{"combo", {{"rando", seed}}}}.dump());
    }
    PlandoLoad();
#ifndef COMBO_ENTRANCE_DEV
    if (unified) {
        assert(!sPlando.loaded && sPlando.statusError && sPlando.rows.empty());
        assert(dumpCalls == 0 && reloadCalls == 0 && reloadedPath.empty());
        assert(!fs::exists(ComboRando::ComboSpoilerPath(seed.at("file_hash"), "Combo-Plando")));
        assert(Read(input) == original && Read(sibling) == misleading);
        return;
    }
#endif
    assert(sPlando.loaded && !sPlando.statusError && dumpCalls == 2);
    assert(Json::parse(sPlando.loadedJson) == seed);
    VerifyRows(sPlando.rows, false);
    for (auto& row : sPlando.rows) {
        if (row.check == "OOT Native Check" || row.check == "MM Foreign Check") {
            row.item = "Deku Mask"; row.itemGame = ComboRando::GAME_MM;
        }
    }
    PlandoSavePlay();
    assert(reloadCalls == 1 && !sPlando.statusError);
    const auto output = ComboRando::ComboSpoilerPath(seed.at("file_hash"), "Combo-Plando");
    assert(fs::path(reloadedPath) == output && output != input);
    const auto saved = Json::parse(Read(output));
    assert(saved != seed && ExceptPlacements(saved) == ExceptPlacements(seed));
    assert(saved.at("fileType") == seed.at("fileType"));
    if (unified) {
        assert(saved.at("fileType") == ComboRando::kEntranceSeedFileType && saved.at("version") == 2);
        assert(saved.at("entranceLayout") == seed.at("entranceLayout"));
    } else assert(!saved.contains("entranceLayout") && !saved.contains("version"));
    assert(Read(input) == original && Read(sibling) == misleading);
    assert(Json::parse(sPlando.loadedJson) == saved);
    sPlando.spoilerPaths = {output.string()}; sPlando.spoilerSel = 0;
    PlandoLoad(); assert(sPlando.loaded && !sPlando.statusError); VerifyRows(sPlando.rows, true);
    PlandoSavePlay(); assert(reloadCalls == 2 && !sPlando.statusError);
    assert(Json::parse(Read(output)) == saved && Read(sibling) == misleading);
}
int main(int argc, char** argv) {
    assert(argc == 2); const fs::path root = fs::absolute(argv[1]); assert(!fs::exists(root));
    sSohDump = OotDump; sMmDump = MmDump; sRequestReload = Reload;
    for (bool unified : {false, true})
        for (int mode = 0; mode < 3; ++mode)
            Case(root / ((unified ? "unified-" : "legacy-") + std::to_string(mode)), unified, mode);
    std::puts("Actual plando dropdown/slot/pending Load and repeated Save & Play preserve metadata/routes; sibling text ignored");
}
'''
for dev in (False, True):
    variant = 'dev' if dev else 'shipping'
    cpp = work/('entrance_plando_' + variant + '.cpp')
    cpp.write_text(support + state + '\n' + functions + '\n' + tests, encoding='utf-8')
    exe = hostcxx.build(args.compiler, cpp, includes=[source/'combo', args.json_include],
                       defines=['_CRT_SECURE_NO_WARNINGS'] + (['COMBO_ENTRANCE_DEV=1'] if dev else []),
                       werror=True, msvc=['/bigobj'], gnu=['-Wno-missing-field-initializers'])
    subprocess.run([str(exe), str(work/(variant + '-' + uuid.uuid4().hex))], check=True, timeout=hostcxx.RUN_TIMEOUT)
print('Plando v2/legacy round trips and shipping refusal passed.')
