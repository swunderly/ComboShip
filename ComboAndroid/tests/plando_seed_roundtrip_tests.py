"""Randomizer > Plandomizer: Load then Save keeps a seed exactly as generated (0.4.12 RC1 File 3 finding).

Runs the real PlandoLoad and PlandoSavePlay, extracted from combo/gui/ComboMenu.cpp with its PlandoState, over every
seed of the committed release corpus (fixtures/wallmasters-vanilla-v1, made by the 0.4.12 SHIPPING headless
generator). Only the module exports, the file-select slot and the reload request are stubbed. Each saved seed must be
the corpus seed, JSON-identical with foreign[] in its order, and the text the app writes must be the fixture's bytes.
Unit cases cover the flags the generator writes only when true: a foreign trap keeps "trap", a missing "advancement"
reads as false and stays missing, and an edit rebuilds only the edited row. Co-op identity must not move either.
"""
from pathlib import Path
import argparse
import json
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
fixtures = source/'ComboAndroid/tests/fixtures/wallmasters-vanilla-v1'


def extract(signature, text=menu):
    start = hostcxx.find(text, signature)
    opening = text.index('{', start)
    masked = re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"', lambda m: ' ' * len(m.group()), text)
    depth = 0
    for end in range(opening, len(text)):
        depth += (masked[end] == '{') - (masked[end] == '}')
        if depth == 0:
            return text[start:end + 1]
    raise AssertionError('Unterminated production function')


state = menu[menu.index('struct PlandoPickItem {'):menu.index('static PlandoState sPlando;') + len('static PlandoState sPlando;')]
functions = '\n'.join(extract(name) for name in (
    'int PlandoImportTarget()', 'void PlandoBuildItems()', 'void PlandoLoad()', 'void PlandoSavePlay()'))
support = r'''
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <set>
#include <sstream>
#include <string>
#include <unordered_set>
#include "rando/ComboPlaythrough.h"
#include "rando/PlandoSeed.h"
#include "rando/ComboSeedFormat.h"
#include "network/ComboAnchorProtocol.h"
namespace fs = std::filesystem;
using Json = nlohmann::json;
using FnDump = const char* (*)(void);
using FnRequestReload = int (*)(const char*);
using FnPlandoFileSelectSlot = int (*)(void);
FnDump sSohDump = nullptr, sMmDump = nullptr;
FnRequestReload sRequestReload = nullptr;
FnPlandoFileSelectSlot sPlandoFileSelectSlot = nullptr;
void ResolvePlandoSyms() {}
namespace ComboTracker { int OotActiveSlot() { return -1; } }
const char* CVarGetString(const char*, const char*) { return ""; }
std::string ootDump, mmDump, reloadedPath;
int reloadCalls = 0;
const char* OotDump() { return ootDump.c_str(); }
const char* MmDump() { return mmDump.c_str(); }
int FileThree() { return 2; }
int Reload(const char* path) { ++reloadCalls; reloadedPath = path; return 1; }
std::string Read(const fs::path& path) {
    std::ifstream in(path, std::ios::binary); assert(in.is_open());
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
'''
tests = r'''
// Item tables like the games' dumps: every item name each game uses in this seed, with an advancement
// flag of true (the old reading of a missing flag) and every name in both tables, so Save's suffix pass
// would tag every native placement if it rebuilt an unedited row.
void DumpsFor(const Json& seed) {
    std::set<std::string> names;
    auto bare = [](std::string s) { return ComboRando::StripGameSuffix(std::move(s)); };
    for (const char* game : {"oot", "mm"})
        for (const auto& item : seed.at(game).at("placements"))
            names.insert(bare(item.get<std::string>()));
    for (const auto& fm : seed.at("foreign")) names.insert(bare(fm.at("itemName").get<std::string>()));
    Json items = Json::array();
    for (const auto& n : names) items.push_back({{"name", n}, {"advancement", true}});
    ootDump = Json{{"items", items}, {"pool", items}}.dump();
    mmDump = ootDump;
}
// Randomizer > Plandomizer: pick the seed in the dropdown, Load, Save for File 3.
Json LoadAndSave(const fs::path& seedPath, bool edit = false, void (*editRows)() = nullptr) {
    sPlando = {};
    sPlando.spoilerPaths = {seedPath.string()};
    sPlando.spoilerNames = {seedPath.stem().string()};
    sPlando.spoilerSel = 0;
    PlandoLoad();
    assert(sPlando.loaded && !sPlando.statusError);
    if (edit) editRows();
    const int before = reloadCalls;
    PlandoSavePlay();
    assert(!sPlando.statusError && reloadCalls == before + 1);
    assert(sPlando.status == "Saved for File 3. Start that file to play it.");
    const Json saved = Json::parse(Read(reloadedPath));
    assert(Json::parse(sPlando.loadedJson) == saved);
    return saved;
}
ComboRando::CwPlacedItem& Row(const std::string& check) {
    for (auto& r : sPlando.rows) if (r.check == check) return r;
    assert(false && "no such row"); std::abort();
}
size_t Index(const Json& foreign, const std::string& check) {
    for (size_t i = 0; i < foreign.size(); ++i) if (foreign[i].at("checkName") == check) return i;
    return foreign.size();
}

// The flags BuildForeignArray writes only when true, on a seed whose foreign[] is not in the editor's
// (game, check) order.
Json UnitSeed() {
    return {
        {"fileType", "ComboShipRandomizer"}, {"seed", "plando-flags"}, {"masterSeed", 7u}, {"displaySeed", 11u},
        {"sharedItems", Json::array()}, {"hints", {{"version", 1}}},
        {"oot", {{"settings", {{"keep", 1}}}, {"placements", {{"OOT Alpha", "Kokiri Sword"}, {"OOT Beta", "Deku Mask"},
                                                              {"OOT Gamma", "Red Rupee"}}}}},
        {"mm", {{"settings", {{"keep", 2}}}, {"placements", {{"MM Alpha", "Deku Mask"}, {"MM Zeta", "Ice Trap"}}}}},
        {"foreign", Json::array({
            {{"checkGame", "mm"}, {"checkName", "MM Zeta"}, {"itemGame", "oot"}, {"itemName", "Ice Trap"},
             {"displayName", "Ice Trap (OOT)"}, {"trap", true}, {"category", "major"}, {"fakeItemName", "Kokiri Sword"},
             {"fakeDisplayName", "Kokiri Sword (OOT)"}, {"fakeTrickName", "Kokiri Sward (OOT)"}},
            {{"checkGame", "oot"}, {"checkName", "OOT Gamma"}, {"itemGame", "mm"}, {"itemName", "Red Rupee"},
             {"displayName", "Red Rupee (MM)"}, {"category", "junk"}},
            {{"checkGame", "oot"}, {"checkName", "OOT Beta"}, {"itemGame", "mm"}, {"itemName", "Deku Mask"},
             {"displayName", "Deku Mask (MM)"}, {"advancement", true}, {"category", "major"}}})}};
}
void UnitCases(const fs::path& root) {
    const Json seed = UnitSeed();
    const fs::path path = root/"Randomizer/unit-seed.json";
    fs::create_directories(path.parent_path());
    { std::ofstream out(path, std::ios::binary); out << seed.dump(2); }
    ootDump = R"json({"items":[{"name":"Kokiri Sword","advancement":true},{"name":"Ice Trap","trap":true}]})json";
    mmDump = R"json({"items":[{"name":"Deku Mask","advancement":true},{"name":"Red Rupee"},{"name":"Bombchu (10)"},
                              {"name":"Hookshot","advancement":true}]})json";

    // Load: a missing "advancement" is false, as the generator means it; a present one is true.
    sPlando = {};
    sPlando.spoilerPaths = {path.string()}; sPlando.spoilerSel = 0;
    PlandoLoad();
    assert(sPlando.loaded && sPlando.rows.size() == 5 && sPlando.loadedRows.size() == 5);
    assert(!Row("MM Zeta").advancement && Row("MM Zeta").itemGame == ComboRando::GAME_OOT);
    assert(!Row("OOT Gamma").advancement && Row("OOT Beta").advancement);

    // Save unedited: the same seed, trap kept, missing flags still missing, foreign[] in the seed's order.
    Json saved = LoadAndSave(path);
    assert(saved == seed && sPlando.loadedJson == seed.dump(2));
    assert(saved["foreign"][0]["trap"] == true && !saved["foreign"][1].contains("advancement"));
    assert(!saved["foreign"][0].contains("advancement"));
    // Picking a row's own item again in the picker (which sets the picker's advancement) is no edit.
    saved = LoadAndSave(path, true, [] {
        for (auto& r : sPlando.rows) r.advancement = true;
    });
    assert(saved == seed);

    // An edited row is rebuilt in its old place; every other entry stays as the seed has it.
    saved = LoadAndSave(path, true, [] {
        auto& r = Row("OOT Gamma"); r.item = "Bombchu (10)"; r.itemGame = ComboRando::GAME_MM; r.advancement = false;
    });
    assert(saved["foreign"].size() == 3 && saved["foreign"][0] == seed["foreign"][0] &&
           saved["foreign"][2] == seed["foreign"][2]);
    assert(saved["foreign"][1] == Json({{"checkGame", "oot"}, {"checkName", "OOT Gamma"}, {"itemGame", "mm"},
                                        {"itemName", "Bombchu (10)"}, {"displayName", "Bombchu (10) (MM)"}}));
    assert(saved["oot"]["placements"]["OOT Gamma"] == "Bombchu (10)");
    Json expect = seed;
    expect["foreign"][1] = saved["foreign"][1];
    expect["oot"]["placements"]["OOT Gamma"] = "Bombchu (10)";
    assert(saved == expect);
    // A second Save of that file, unedited, keeps it.
    const Json edited = saved;
    PlandoSavePlay();
    assert(!sPlando.statusError && Json::parse(Read(reloadedPath)) == edited);

    // The trap's row made native drops its entry; a native row made foreign goes last.
    saved = LoadAndSave(path, true, [] {
        auto& trap = Row("MM Zeta"); trap.item = "Bombchu (10)"; trap.itemGame = ComboRando::GAME_MM;
        auto& native = Row("OOT Alpha"); native.item = "Hookshot"; native.itemGame = ComboRando::GAME_MM;
        native.advancement = true;
    });
    assert(saved["foreign"].size() == 3 && Index(saved["foreign"], "MM Zeta") == 3);
    assert(saved["foreign"][0] == seed["foreign"][1] && saved["foreign"][1] == seed["foreign"][2]);
    assert(saved["foreign"][2] == Json({{"checkGame", "oot"}, {"checkName", "OOT Alpha"}, {"itemGame", "mm"},
                                        {"itemName", "Hookshot"}, {"displayName", "Hookshot (MM)"},
                                        {"advancement", true}}));
    assert(saved["mm"]["placements"]["MM Zeta"] == "Bombchu (10)" && saved["oot"]["placements"]["OOT Alpha"] == "Hookshot");
    assert(ComboAnchorProtocol::SeedIdentity(saved) == ComboAnchorProtocol::SeedIdentity(seed));
    std::puts("Plandomizer unit cases: trap kept, missing advancement false and kept, foreign[] order kept, edits rebuilt in place");
}

// Every corpus seed: Load, Save for File 3, and the seed comes back exactly.
int Corpus(const fs::path& root, const fs::path& fixtures) {
    const Json manifest = Json::parse(Read(fixtures/"manifest.json"));
    fs::create_directories(root);
    fs::current_path(root);
    int seeds = 0;
    size_t foreignEntries = 0, traps = 0, missingAdvancement = 0;
    for (const auto& row : manifest.at("seeds")) {
        const fs::path seedPath = fixtures/row.at("seed").get<std::string>();
        const std::string text = Read(seedPath);
        const Json seed = Json::parse(text);
        DumpsFor(seed);
        const Json saved = LoadAndSave(seedPath);
        size_t placements = seed["oot"]["placements"].size() + seed["mm"]["placements"].size(), foreignRows = 0;
        for (const auto& r : sPlando.rows) foreignRows += r.checkGame != r.itemGame ? 1 : 0;
        assert(sPlando.rows.size() == placements && foreignRows == seed["foreign"].size());
        if (saved != seed || sPlando.loadedJson != text) {
            std::cerr << row.at("id").get<std::string>() << ": the saved seed differs from the corpus seed\n";
            return 1;
        }
        assert(saved["foreign"] == seed["foreign"]); // element by element, so in the same order
        assert(ComboAnchorProtocol::SeedIdentity(saved) == ComboAnchorProtocol::SeedIdentity(seed));
        assert(Read(seedPath) == text);
        for (const auto& fm : seed["foreign"]) {
            ++foreignEntries;
            traps += fm.value("trap", false) ? 1 : 0;
            missingAdvancement += fm.contains("advancement") ? 0 : 1;
        }
        ++seeds;
    }
    assert(seeds == (int)manifest.at("seeds").size() && traps > 0 && missingAdvancement > 0);
    std::printf("Plandomizer Load + Save kept all %d corpus seeds exactly (%zu foreign entries, %zu traps, %zu without "
                "advancement)\n", seeds, foreignEntries, traps, missingAdvancement);
    return 0;
}

int main(int argc, char** argv) {
    assert(argc == 3);
    const fs::path root = fs::absolute(argv[1]), fixtures = fs::absolute(argv[2]);
    assert(!fs::exists(root));
    fs::create_directories(root);
    sSohDump = OotDump; sMmDump = MmDump; sRequestReload = Reload; sPlandoFileSelectSlot = FileThree;
    fs::current_path(root);
    UnitCases(root/"unit");
    return Corpus(root/"corpus", fixtures);
}
'''
manifest = json.loads((fixtures/'manifest.json').read_text(encoding='utf-8'))
seed_files = sorted(path.name for path in (fixtures/'seeds').glob('*.json'))
assert len(manifest['seeds']) == len(seed_files) and len(seed_files) >= 100, 'the committed corpus is incomplete'
cpp = work/'plando_seed_roundtrip.cpp'
cpp.write_text(support + state + '\n' + functions + '\n' + tests, encoding='utf-8')
exe = hostcxx.build(args.compiler, cpp, includes=[source/'combo', args.json_include],
                    defines=['_CRT_SECURE_NO_WARNINGS'], werror=True, msvc=['/bigobj'],
                    gnu=['-Wno-missing-field-initializers'])
subprocess.run([str(exe), str(work/('run-' + uuid.uuid4().hex)), str(fixtures)], check=True,
               timeout=hostcxx.RUN_TIMEOUT)
print(f'Plandomizer round trip kept all {len(seed_files)} committed corpus seeds byte for byte.')
