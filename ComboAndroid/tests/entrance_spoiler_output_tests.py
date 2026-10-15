"""Exercise the real spoiler output helper and launcher writer in shipping/dev builds."""
from pathlib import Path
import argparse
import subprocess
import uuid

import hostcxx

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
parser.add_argument('--json-include', type=Path, required=True)
args = parser.parse_args()
args.source = args.source.resolve()
args.build_dir = args.build_dir.resolve()
args.build_dir.mkdir(parents=True, exist_ok=True)
launcher = (args.source/'combo/ComboShip.cpp').read_text(encoding='utf-8')
start = hostcxx.find(launcher, 'static std::filesystem::path WriteComboSpoiler(')
end = launcher.index('{', start) + 1
depth = 1
while depth:
    depth += (launcher[end] == '{') - (launcher[end] == '}')
    end += 1
writer = launcher[start:end]
# Reconciliation must follow a successfully closed JSON stream; otherwise a failed seed write
# could remove a useful old sidecar. These are real production statements, not harness stubs.
closed = hostcxx.find(writer, 'out.close();')
assert hostcxx.find(writer, 'if (!out)', closed) < hostcxx.find(writer, 'WriteEntranceSpoilerFile(')
helper = (args.source/'combo/rando/entrances/EntranceSpoilerOutput.h').read_text(encoding='utf-8')
assert hostcxx.find(helper, 'out.close();') < hostcxx.find(helper, 'return bool(out);')

prefix = r'''
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include "rando/entrances/EntranceSpoilerOutput.h"
namespace fs = std::filesystem;
namespace CE = ComboRando::Entrances;
using Json = nlohmann::json;
fs::path outputRoot;
namespace ComboRando {
fs::path ConsolidatedDir() { return outputRoot; }
fs::path ComboSpoilerPath(const Json& hash) { return outputRoot / (hash.get<std::string>() + ".json"); }
}
'''
tests = r'''
std::string Read(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    assert(in.is_open());
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
template<class Fn> void Refused(Fn fn) {
    bool refused = false;
    try { fn(); } catch (const std::exception&) { refused = true; }
    assert(refused);
}
Json LayoutSeed() {
    CE::EntranceLayout layout;
    layout.required = {std::string(CE::kMmTrancheCapability)};
    layout.devBuild = true;
    layout.settings.erWallmasters = CE::Mode::OwnGame;
    layout.overrides = CE::Overrides{
        {"MM_WALLMASTER_DAMPE", "MM_TEMPLE_WOODFALL"},
        {"MM_WALLMASTER_BTW_ENTRANCE", "MM_TEMPLE_SNOWHEAD"},
        {"MM_WALLMASTER_BTW_FOUNTAIN", "MM_TEMPLE_GREAT_BAY"},
        {"MM_WALLMASTER_BTW_EXIT", "MM_TEMPLE_STONE_TOWER"},
    };
    return {{"fileType", ComboRando::kEntranceSeedFileType}, {"version", ComboRando::kEntranceSeedVersion},
            {"entranceLayout", CE::EncodeEntranceLayout(layout)}};
}
int main(int argc, char** argv) {
    assert(argc == 2);
    outputRoot = fs::path(argv[1]);
    assert(!fs::exists(outputRoot)); // each run gets its own host-only evidence directory
    const auto caps = CE::ActiveCapabilities();

    // 1. Legacy and no-layout input stays byte-for-byte, with no new text artifact.
    const std::string legacy = R"({ "fileType" : "ComboShipRandomizer", "marker" : "keep exact spacing" })";
    const auto legacyJson = Json::parse(legacy);
    assert(CE::EntranceSpoilerForSeed(legacyJson, caps).empty());
    assert(WriteComboSpoiler("legacy", legacy) == outputRoot / "legacy.json");
    assert(Read(outputRoot / "legacy.json") == legacy);
    assert(!fs::exists(outputRoot / "legacy.entrances.txt"));
    assert(CE::WriteEntranceSpoilerFile(outputRoot / "no-layout.json", legacyJson, caps));
    assert(!fs::exists(outputRoot / "no-layout.entrances.txt"));

    // 2. Unsupported layout is refused before either file can be opened or replaced.
    auto seed = LayoutSeed();
    auto rejected = seed;
    rejected["entranceLayout"]["requires"] = Json::array({"unknown-capability"});
    Refused([&] { WriteComboSpoiler("rejected", rejected.dump()); });
    assert(!fs::exists(outputRoot / "rejected.json"));
    assert(!fs::exists(outputRoot / "rejected.entrances.txt"));
    Refused([&] { WriteComboSpoiler("legacy", rejected.dump()); });
    assert(Read(outputRoot / "legacy.json") == legacy);
    Refused([&] { CE::WriteEntranceSpoilerFile(outputRoot / "helper-rejected.json", rejected, caps); });
    assert(!fs::exists(outputRoot / "helper-rejected.entrances.txt"));

    const auto before = seed.dump();
#ifdef COMBO_ENTRANCE_DEV
    // 3. Dev layout is copied exactly to JSON and a matching sibling text file.
    const auto validated = CE::Validate(CE::DecodeEntranceLayout(seed.at("entranceLayout")), caps);
    const auto expected = CE::FormatEntranceSpoiler(&validated);
    assert(CE::EntranceSpoilerForSeed(seed, caps) == expected);
    assert(WriteComboSpoiler("dev", before) == outputRoot / "dev.json");
    assert(Read(outputRoot / "dev.json") == before);
    const auto text = Read(outputRoot / "dev.entrances.txt");
    assert(text == expected);
    size_t arrows = 0;
    for (size_t p = 0; (p = text.find(" -> ", p)) != std::string::npos; p += 4) ++arrows;
    assert(arrows == seed.at("entranceLayout").at("overrides").size() + 2);
    for (const auto& route : seed.at("entranceLayout").at("overrides")) {
        const auto source = "(" + route.at("source").get<std::string>() + ")";
        const auto destination = "(" + route.at("destination").get<std::string>() + ")";
        const auto p = text.find(source);
        assert(p != std::string::npos && text.find(source, p + 1) == std::string::npos);
        assert(text.substr(p, text.find('\n', p) - p).find(destination) != std::string::npos);
    }

    // 4. The helper may not truncate the seed itself when extensions already match.
    const auto samePath = outputRoot / "same.entrances.txt";
    { std::ofstream file(samePath, std::ios::binary); file << before; }
    Refused([&] { CE::WriteEntranceSpoilerFile(samePath, seed, caps); });
    assert(Read(samePath) == before);

    // 5. Text-file I/O failure is reported, while the launcher still preserves its JSON.
    fs::create_directory(outputRoot / "blocked.entrances.txt");
    assert(!CE::WriteEntranceSpoilerFile(outputRoot / "blocked.json", seed, caps));
    std::ostringstream errors;
    auto* previous = std::cerr.rdbuf(errors.rdbuf());
    const auto written = WriteComboSpoiler("blocked", before);
    std::cerr.rdbuf(previous);
    assert(written == outputRoot / "blocked.json");
    assert(Read(written) == before);
    assert(errors.str().find("could not write entrance spoiler beside") != std::string::npos);
#else
    // 3. The same otherwise-valid dev seed cannot create output in a release build.
    Refused([&] { CE::EntranceSpoilerForSeed(seed, caps); });
    Refused([&] { WriteComboSpoiler("dev", before); });
    assert(!fs::exists(outputRoot / "dev.json"));
    assert(!fs::exists(outputRoot / "dev.entrances.txt"));
#endif
    assert(seed.dump() == before); // formatting/output cannot change immutable seed metadata

    // Reuse the same basename: a legacy replacement must not retain an older ER sidecar.
    const auto helperPath = outputRoot / "replace-helper.json";
    const auto helperText = outputRoot / "replace-helper.entrances.txt";
#ifdef COMBO_ENTRANCE_DEV
    assert(CE::WriteEntranceSpoilerFile(helperPath, seed, caps));
    assert(WriteComboSpoiler("replace-launcher", before) == outputRoot / "replace-launcher.json");
#else
    // A shipping build can encounter a sibling made by a prior dev build; do not admit its seed.
    { std::ofstream old(helperText); old << "previous dev entrance section"; }
    { std::ofstream old(outputRoot / "replace-launcher.entrances.txt"); old << "previous dev entrance section"; }
#endif
    assert(fs::exists(helperText));
    assert(fs::exists(outputRoot / "replace-launcher.entrances.txt"));
    { std::ofstream jsonFile(helperPath, std::ios::binary); jsonFile << legacy; }
    { std::ofstream other(outputRoot / "unrelated.entrances.txt"); other << "unrelated"; }
    assert(CE::WriteEntranceSpoilerFile(helperPath, legacyJson, caps));
    assert(!fs::exists(helperText) && Read(helperPath) == legacy);
    assert(WriteComboSpoiler("replace-launcher", legacy) == outputRoot / "replace-launcher.json");
    assert(!fs::exists(outputRoot / "replace-launcher.entrances.txt"));
    assert(Read(outputRoot / "replace-launcher.json") == legacy);
    assert(Read(outputRoot / "unrelated.entrances.txt") == "unrelated");

    // Invalid input (including a v2 declaration with missing layout) cannot delete stale output.
    const auto invalidPath = outputRoot / "invalid.json";
    const auto invalidText = outputRoot / "invalid.entrances.txt";
    { std::ofstream old(invalidPath); old << "old JSON"; }
    { std::ofstream old(invalidText); old << "old entrance section"; }
    const Json missingLayout = {{"fileType", ComboRando::kEntranceSeedFileType},
                                {"version", ComboRando::kEntranceSeedVersion}};
    for (const auto& invalid : std::vector<Json>{Json(), Json::object(), missingLayout, rejected}) {
        Refused([&] { CE::WriteEntranceSpoilerFile(invalidPath, invalid, caps); });
        Refused([&] { WriteComboSpoiler("invalid", invalid.dump()); });
        assert(Read(invalidPath) == "old JSON");
        assert(Read(invalidText) == "old entrance section");
    }
    Refused([&] { CE::WriteEntranceSpoilerFile(invalidText, legacyJson, caps); });
    assert(Read(invalidText) == "old entrance section");
    fs::create_directory(outputRoot / "keep-directory.entrances.txt");
    assert(!CE::WriteEntranceSpoilerFile(outputRoot / "keep-directory.json", legacyJson, caps));
    assert(fs::is_directory(outputRoot / "keep-directory.entrances.txt"));

    // JSON open failure must leave an old sidecar alone (launcher returns an empty path).
    fs::create_directory(outputRoot / "failed-json.json");
    { std::ofstream old(outputRoot / "failed-json.entrances.txt"); old << "still valid for old seed"; }
    assert(WriteComboSpoiler("failed-json", legacy).empty());
    assert(Read(outputRoot / "failed-json.entrances.txt") == "still valid for old seed");

    // 6. Malformed input is refused before the launcher touches either output.
    Refused([&] { WriteComboSpoiler("malformed", "{broken"); });
    assert(!fs::exists(outputRoot / "malformed.json"));
    assert(!fs::exists(outputRoot / "malformed.entrances.txt"));
    std::cout << "PASS: real helper/launcher legacy bytes, capability preflight, output identity and I/O boundaries\n";
}
'''
cpp = args.build_dir/'entrance_spoiler_output.cpp'
cpp.write_text(prefix + writer + tests, encoding='utf-8', newline='\n')
for mode, defines in [('shipping', []), ('dev', ['COMBO_ENTRANCE_DEV=1'])]:
    exe = hostcxx.build(args.compiler, cpp, includes=[args.source/'combo', args.json_include],
                        defines=defines, werror=True, exe=args.build_dir/f'entrance_spoiler_output_{mode}.exe')
    evidence = args.build_dir/f'{mode}-{uuid.uuid4().hex}'
    result = subprocess.run([str(exe), str(evidence)], capture_output=True, text=True,
                            timeout=hostcxx.RUN_TIMEOUT)
    (args.build_dir/f'{mode}.txt').write_text(result.stdout + result.stderr, encoding='utf-8')
    assert result.returncode == 0, (mode, result.returncode, result.stdout, result.stderr)
    print(mode + ': ' + result.stdout.strip())
