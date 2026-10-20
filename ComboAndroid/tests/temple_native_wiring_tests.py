"""Production wiring of the opposite-age Temple probe (TempleNative.h) on the host, with fake exports.

Static: the resolver looks up exactly the four exports soh's TempleProbe.h declares, through the
caller's module lookup only. Compiled harness: the fakes are defined with the declarations copied
from TempleProbe.h, so the compiler checks that TempleProbeFns has the exports' exact types; then,
with the real Shuffler, registry, gate and adapter, it checks export resolution, the gate and the
entry points before Prepare and before any draw, that logic none, games=Mm and a run with no pool to
place never gate, prepare or query, that MM-only Wallmasters run while an OoT Wallmaster arrival at
an unbound one-way row is refused, the Prepare and Query code mapping without retry, Release on success, exception and
cancellation, that a query before Prepare or after Release is refused, and the cancellation poll.
The native probe itself is covered by the temple_probe_* harnesses; nothing here runs the engine.
"""
from pathlib import Path
import argparse
import re
import subprocess
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
native = (source/'combo/rando/entrances/TempleNative.h').read_text(encoding='utf-8')
probe = (source/'soh/soh/Enhancements/randomizer/TempleProbe.h').read_text(encoding='utf-8')

# The exports soh declares, verbatim without the export macro.
exports = re.findall(r'COMBO_EXPORT\s+([^;]*\bSOH_TempleProbe_(\w+)\s*\([^;]*\));', probe)
assert sorted(name for _, name in exports) == ['LastError', 'Prepare', 'Query', 'Release'], exports
declarations = '\n'.join(declaration + ';' for declaration, _ in exports)

# The resolver names each export once, through the caller's lookup, and loads no module itself.
code = re.sub(r'//[^\n]*', '', native)
looked = re.findall(r'fns\.(\w+)\s*=\s*reinterpret_cast<decltype\(fns\.\1\)>\(\s*lookup\(\s*"(\w+)"\s*\)\s*\);', code)
assert sorted(looked) == [('lastError', 'SOH_TempleProbe_LastError'), ('prepare', 'SOH_TempleProbe_Prepare'),
                          ('query', 'SOH_TempleProbe_Query'), ('release', 'SOH_TempleProbe_Release')], looked
assert code.count('SOH_TempleProbe_') == 4
for banned in ('dlopen', 'dlsym', 'LoadLibrary', 'GetProcAddress', 'GetModuleHandle', 'Combo_ResolveSym',
               'validateAgeTemple'):
    assert banned not in code, banned

tests = r'''
#include "rando/entrances/TempleNative.h"
#include <cassert>
#include <iostream>
#include <map>
#include <type_traits>

using namespace ComboRando;
using namespace ComboRando::Entrances;
using Json = nlohmann::json;

// soh/soh/Enhancements/randomizer/TempleProbe.h, defined below as fakes.
extern "C" {
@DECLARATIONS@
}
static_assert(std::is_same_v<decltype(TempleProbeFns::prepare), decltype(&SOH_TempleProbe_Prepare)>);
static_assert(std::is_same_v<decltype(TempleProbeFns::query), decltype(&SOH_TempleProbe_Query)>);
static_assert(std::is_same_v<decltype(TempleProbeFns::lastError), decltype(&SOH_TempleProbe_LastError)>);
static_assert(std::is_same_v<decltype(TempleProbeFns::release), decltype(&SOH_TempleProbe_Release)>);
static_assert(noexcept(TempleProbeCancelPoll()));

template <class E, class F> bool Throws(F f) {
    try {
        f();
    } catch (const E&) {
        return true;
    }
    return false;
}

ComboGenProgress progress;
size_t draws = 0;
struct FakeProbe {
    bool prepared = false;
    int prepares = 0, queries = 0, releases = 0;
    int prepareCode = 0, queryCode = 1, cancelAtQuery = 0;
    bool pollCancelled = false; // a query returned -3 because the poll said so
    std::string error;
    Json prepare, last;
} fake;
void Reset() {
    fake = FakeProbe{};
    draws = 0;
    progress.Reset();
}

// The native contract (TempleProbe.cpp): Prepare captures the seed facts, Query refuses to run
// without it (-1), polls for cancellation (-3), and Release forgets the preparation.
extern "C" int SOH_TempleProbe_Prepare(const char* json) {
    ++fake.prepares;
    fake.prepare = Json::parse(json);
    assert(fake.prepare["schema"] == "oot-temple-probe-prepare-v1" &&
           fake.prepare["registry"] == std::string(kRegistrySourceSha256));
    if (fake.prepareCode != 0) {
        fake.error = "fake prepare detail";
        return fake.prepareCode;
    }
    fake.prepared = true;
    return 0;
}
extern "C" int SOH_TempleProbe_Query(const char* json, int (*poll)(void), uint8_t* agesOut) {
    ++fake.queries;
    if (!fake.prepared) {
        fake.error = "query before prepare";
        return -1;
    }
    if (fake.cancelAtQuery == fake.queries)
        progress.RequestCancel();
    if (poll != nullptr && poll() != 0) {
        fake.pollCancelled = true;
        return -3;
    }
    fake.last = Json::parse(json);
    assert(fake.last["schema"] == "oot-temple-probe-query-v1" && fake.last["assumed"].empty());
    const std::set<std::string_view> bound(kOotTempleCandidateRows.begin(), kOotTempleCandidateRows.end());
    for (const auto& row : fake.last["deleted"])
        assert(bound.contains(row.get<std::string>()));
    for (const auto& e : fake.last["edges"])
        assert(bound.contains(e["src"].get<std::string>()) && bound.contains(e["dst"].get<std::string>()));
    auto mm = [](const Json& row) { return Find(row.get<std::string>())->game == Game::Mm; };
    for (const auto& row : fake.last["mm"]["deleted"])
        assert(mm(row));
    for (const auto& e : fake.last["mm"]["edges"])
        assert(mm(e["src"]) && mm(e["dst"]));
    if (fake.queryCode != 0 && fake.queryCode != 1)
        fake.error = "fake query detail";
    *agesOut = fake.queryCode == 1 ? 2 : 0;
    return fake.queryCode;
}
extern "C" const char* SOH_TempleProbe_LastError(void) {
    return fake.error.c_str();
}
extern "C" void SOH_TempleProbe_Release(void) {
    ++fake.releases;
    fake.prepared = false;
}

// The launcher's module lookup (ComboShip.cpp GetSym, headless Sym<void*>).
std::vector<std::string> looked;
std::set<std::string> missing;
void* Lookup(const char* name) {
    looked.push_back(name);
    const std::map<std::string, void*> table{
        { "SOH_TempleProbe_Prepare", reinterpret_cast<void*>(&SOH_TempleProbe_Prepare) },
        { "SOH_TempleProbe_Query", reinterpret_cast<void*>(&SOH_TempleProbe_Query) },
        { "SOH_TempleProbe_LastError", reinterpret_cast<void*>(&SOH_TempleProbe_LastError) },
        { "SOH_TempleProbe_Release", reinterpret_cast<void*>(&SOH_TempleProbe_Release) },
    };
    const auto it = table.find(name);
    return it == table.end() || missing.contains(name) ? nullptr : it->second;
}

bool OotWallmasterArea(std::string_view area) {
    for (const auto& r : kRegistry)
        if (r.game == Game::Oot && r.type == Type::Wallmaster && r.from == area)
            return true;
    return false;
}
// mmOnly reports the OoT Wallmaster areas absent, as the MM tranche will (M1-07).
Bindings MakeBindings(bool mmOnly) {
    Bindings b;
    if (mmOnly)
        b.areaExists = [](std::string_view area) { return !OotWallmasterArea(area); };
    else
        b.areaExists = [](std::string_view) { return true; };
    b.sourceSupported = b.destinationSupported = [](const Record&) { return true; };
    b.validateAssumed = [](const Overrides&, const Pools&) { return true; };
    b.validateComplete = [](const Overrides&) { return true; };
    b.chooseIndex = [](size_t count) {
        CheckGenerationCancellation();
        ++draws;
        return count - 1;
    };
    return b;
}
Settings MmWallmasters() {
    Settings s;
    s.erWallmasters = Mode::OwnGame;
    return s;
}
// The shape UnifiedBridge.h will have: the run before the Shuffler, its validator as the hook.
Overrides Generate(const TempleProbeFns& fns, const Settings& s, const TempleFacts& f, Bindings b) {
    TempleProbeRun temple(fns, s, f, b.areaExists);
    b.logicEnabled = f.logicEnabled;
    b.validateAgeTemple = temple.Validator();
    return Shuffler(s, std::move(b)).run();
}
std::string Unsupported(const std::function<void()>& f) {
    try {
        f();
    } catch (const TempleProbeUnsupported& e) {
        return e.what();
    }
    return "";
}
const std::vector<std::string> kExports{ "SOH_TempleProbe_Prepare", "SOH_TempleProbe_Query",
                                         "SOH_TempleProbe_LastError", "SOH_TempleProbe_Release" };

void Resolution() {
    looked.clear();
    const auto fns = ResolveTempleProbe(Lookup);
    assert(looked == kExports);
    assert(fns.prepare == &SOH_TempleProbe_Prepare && fns.query == &SOH_TempleProbe_Query &&
           fns.lastError == &SOH_TempleProbe_LastError && fns.release == &SOH_TempleProbe_Release &&
           fns.poll == &TempleProbeCancelPoll);
    // A missing entry point is refused before Prepare and before any draw; LastError is optional.
    for (const auto& name : { kExports[0], kExports[1], kExports[3] }) {
        missing = { name };
        Reset();
        const auto why = Unsupported([] { Generate(ResolveTempleProbe(Lookup), MmWallmasters(), {}, MakeBindings(true)); });
        assert(why == "Temple probe: native entry points are missing");
        assert(fake.prepares == 0 && fake.queries == 0 && fake.releases == 0 && draws == 0);
    }
    missing = { kExports[2] };
    Reset();
    assert(!Generate(ResolveTempleProbe(Lookup), MmWallmasters(), {}, MakeBindings(true)).empty());
    assert(fake.prepares == 1 && fake.queries > 0 && fake.releases == 1);
    missing.clear();
}

void NoTemplePass(const TempleProbeFns& fns) {
    // Logic none: isValidShuffle accepts before building any world (entrances.ts:388-391), so the
    // run gates nothing (OoT Wallmaster rows, song travel and glitched logic would all be refused),
    // prepares nothing and has no validator.
    TempleFacts none;
    none.logicEnabled = false;
    none.songTravelLogic = true;
    none.glitchless = false;
    Reset();
    {
        TempleProbeRun run(fns, MmWallmasters(), none, MakeBindings(false).areaExists);
        assert(!run.Validator());
    }
    const auto layout = Generate(fns, MmWallmasters(), none, MakeBindings(true));
    assert(layout.size() == 4 + 2 && draws > 0);
    assert(fake.prepares == 0 && fake.queries == 0 && fake.releases == 0);
    // games=Mm: there is no OoT Temple pass at all (entrances.ts:411).
    Settings mm = MmWallmasters();
    mm.games = Games::Mm;
    Reset();
    {
        TempleProbeRun run(fns, mm, {}, MakeBindings(false).areaExists);
        assert(!run.Validator());
    }
    assert(!Generate(fns, mm, {}, MakeBindings(false)).empty() && draws > 0);
    assert(fake.prepares == 0 && fake.queries == 0 && fake.releases == 0);
}

void NoPoolToPlace(const TempleProbeFns& fns) {
    // OoTMM validates only while a non-empty pool is left (entrances.ts:500-502, :581-589), so with
    // nothing to place no seed fact is gated (each of these would be refused with a pool) and
    // nothing is prepared, queried or released.
    TempleFacts songTravel, warpSong, glitched, random;
    songTravel.songTravelLogic = true;
    warpSong.ootStartingItems = { "Prelude of Light" };
    glitched.glitchless = false;
    random.startingAge = TempleStartingAge::Random;
    // No pool in the settings: the Shuffler neither requires nor calls the hook, so the validator is
    // empty, the run never asks areaExists, and nothing is placed.
    for (const auto& f : { TempleFacts{}, songTravel, warpSong, glitched, random }) {
        Reset();
        {
            TempleProbeRun run(fns, Settings{}, f, AreaExists{});
            assert(!run.Validator());
        }
        assert(Generate(fns, Settings{}, f, MakeBindings(false)).empty());
        assert(fake.prepares == 0 && fake.queries == 0 && fake.releases == 0 && draws == 0);
    }
    // Pools in the settings that all resolve empty: one-ways with no flag (every flag defaults to
    // false, data.ts:3584-3646) and Wallmasters in a world without their areas. The Shuffler still
    // requires the hook, so the run gives one that refuses a call without a native one; the
    // Shuffler never calls it and places only the reserved game links.
    Settings oneWays;
    oneWays.erOneWays = Mode::OwnGame;
    auto noWallmasters = MakeBindings(false);
    noWallmasters.areaExists = [](std::string_view area) {
        for (const auto& r : kRegistry)
            if (r.type == Type::Wallmaster && r.from == area)
                return false;
        return true;
    };
    for (const auto& [s, b] :
         { std::pair{ oneWays, MakeBindings(false) }, std::pair{ MmWallmasters(), noWallmasters } }) {
        assert(!MakePools(s).empty() && ResolvePools(s, b.areaExists).pools.empty());
        for (const auto& f : { TempleFacts{}, songTravel, warpSong, glitched, random }) {
            Reset();
            {
                TempleProbeRun run(fns, s, f, b.areaExists);
                const auto validator = run.Validator();
                assert(validator);
                assert(Unsupported([&] { validator({}, {}); }) ==
                       "Temple probe: query for a run with no pool to place");
            }
            assert(Generate(fns, s, f, b) == ReservedEntrances(s) && ReservedEntrances(s).size() == 2);
            assert(fake.prepares == 0 && fake.queries == 0 && fake.releases == 0 && draws == 0);
        }
    }
}

void MmOnlyWallmasters(const TempleProbeFns& fns) {
    for (auto age : { TempleStartingAge::Child, TempleStartingAge::Adult }) {
        Reset();
        TempleFacts f;
        f.startingAge = age;
        const auto layout = Generate(fns, MmWallmasters(), f, MakeBindings(true));
        assert(fake.prepares == 1 && fake.queries > 0 && fake.releases == 1 && draws > 0);
        assert(fake.prepare["startingAge"] == (age == TempleStartingAge::Child ? "child" : "adult"));
        // The OoT world is untouched; the MM rows are listed, never probed (lemma L4).
        assert(fake.last["deleted"].empty() && fake.last["edges"].empty());
        assert(fake.last["mm"]["deleted"].size() == 4 && !fake.last["mm"]["edges"].empty());
        size_t routes = 0;
        for (const auto& [src, dst] : layout)
            if (Find(src)->type == Type::Wallmaster)
                routes += Find(src)->game == Game::Mm && Find(dst)->game == Game::Mm;
        assert(routes == 4 && layout.size() == 4 + 2);
    }
    // Every OoT Wallmaster source is bound now, the MQ-only ones too (U11). An OoT Wallmaster arrival at a one-way
    // row is not: the gate refuses it before Prepare and before any draw.
    Reset();
    auto withOneWays = MmWallmasters();
    withOneWays.erOneWays = Mode::OwnGame;
    withOneWays.erOneWaysMajor = true;
    const auto why = Unsupported([&] { Generate(fns, withOneWays, {}, MakeBindings(false)); });
    assert(why.starts_with("Temple probe: OoT Wallmaster arrival ") && why.ends_with(" is deferred"));
    assert(fake.prepares == 0 && fake.queries == 0 && fake.releases == 0 && draws == 0);
}

void CodeMapping(const TempleProbeFns& fns) {
    // Query 0 is "not reached": generation fails after the pinned 1000 attempts, with no fallback.
    Reset();
    fake.queryCode = 0;
    assert(Throws<ShuffleError>([&] { Generate(fns, MmWallmasters(), {}, MakeBindings(true)); }));
    assert(fake.prepares == 1 && fake.queries >= 1000 && fake.releases == 1);
    // -3 cancels; anything else is unsupported with the native detail, on the first query.
    Reset();
    fake.queryCode = -3;
    assert(Throws<GenerationCancelled>([&] { Generate(fns, MmWallmasters(), {}, MakeBindings(true)); }));
    assert(!fake.pollCancelled && fake.queries == 1 && fake.releases == 1);
    for (int code : { -1, -2, -4, 2, 7 }) {
        Reset();
        fake.queryCode = code;
        const auto why = Unsupported([&] { Generate(fns, MmWallmasters(), {}, MakeBindings(true)); });
        assert(why == "Temple probe returned " + std::to_string(code) + ": fake query detail");
        assert(fake.queries == 1 && fake.releases == 1);
    }
    // Prepare: anything but 0 stops before the Shuffler and still releases.
    for (int code : { -1, -2, -4, 1 }) {
        Reset();
        fake.prepareCode = code;
        const auto why = Unsupported([&] { Generate(fns, MmWallmasters(), {}, MakeBindings(true)); });
        assert(why == "Temple probe returned " + std::to_string(code < 0 ? code : -4) + ": fake prepare detail");
        assert(fake.queries == 0 && fake.releases == 1 && draws == 0);
    }
    Reset();
    fake.prepareCode = -3;
    assert(Throws<GenerationCancelled>([&] { Generate(fns, MmWallmasters(), {}, MakeBindings(true)); }));
    assert(fake.queries == 0 && fake.releases == 1 && draws == 0);
}

void ReleaseOnEveryExit(const TempleProbeFns& fns) {
    // An exception out of the Shuffler still releases, and the run's validator dies with the run:
    // a later call never reaches the native probe.
    Reset();
    std::function<bool(const Overrides&, const Pools&)> kept;
    auto b = MakeBindings(true);
    b.validateAssumed = [](const Overrides&, const Pools&) -> bool { throw std::runtime_error("oracle failed"); };
    std::string why;
    try {
        TempleProbeRun temple(fns, MmWallmasters(), {}, b.areaExists);
        kept = temple.Validator();
        b.validateAgeTemple = kept;
        Shuffler(MmWallmasters(), b).run();
    } catch (const std::runtime_error& e) {
        why = e.what();
    }
    assert(why == "oracle failed" && fake.prepares == 1 && fake.releases == 1 && fake.queries == 0);
    assert(Unsupported([&] { kept({}, {}); }) == "Temple probe: query outside a prepared session");
    assert(fake.queries == 0);
    // Cancellation seen by the native poll (the query returns -3) releases once.
    Reset();
    fake.cancelAtQuery = 3;
    assert(Throws<GenerationCancelled>([&] { Generate(fns, MmWallmasters(), {}, MakeBindings(true)); }));
    assert(fake.pollCancelled && fake.queries == 3 && fake.releases == 1);
    // Cancellation before a query stops at the checkpoint: the native probe is not called again.
    Reset();
    int assumed = 0;
    b = MakeBindings(true);
    b.validateAssumed = [&](const Overrides&, const Pools&) {
        if (++assumed == 2)
            progress.RequestCancel();
        return true;
    };
    assert(Throws<GenerationCancelled>([&] { Generate(fns, MmWallmasters(), {}, b); }));
    assert(!fake.pollCancelled && fake.queries == 1 && fake.releases == 1);
    // A table without a poll still gets the cancellation poll.
    Reset();
    auto noPoll = fns;
    noPoll.poll = nullptr;
    fake.cancelAtQuery = 1;
    assert(Throws<GenerationCancelled>([&] { Generate(noPoll, MmWallmasters(), {}, MakeBindings(true)); }));
    assert(fake.pollCancelled && fake.queries == 1 && fake.releases == 1);
}

void QueryBeforePrepare(const TempleProbeFns& fns) {
    // The native probe refuses a query without Prepare (-1); it maps to unsupported and the
    // Shuffler does not retry it.
    Reset();
    auto b = MakeBindings(true);
    b.validateAgeTemple = MakeTempleValidator(fns, MmWallmasters());
    const auto why = Unsupported([&] { Shuffler(MmWallmasters(), b).run(); });
    assert(why == "Temple probe returned -1: query before prepare");
    assert(fake.prepares == 0 && fake.queries == 1);
}

void Poll() {
    {
        GenerationScope outside(nullptr);
        assert(TempleProbeCancelPoll() == 0);
    }
    progress.Reset();
    assert(TempleProbeCancelPoll() == 0);
    progress.RequestCancel();
    assert(TempleProbeCancelPoll() == 1);
    progress.Reset();
}

int main() {
    GenerationScope scope(&progress);
    Resolution();
    const auto fns = ResolveTempleProbe(Lookup);
    NoTemplePass(fns);
    NoPoolToPlace(fns);
    MmOnlyWallmasters(fns);
    CodeMapping(fns);
    ReleaseOnEveryExit(fns);
    QueryBeforePrepare(fns);
    Poll();
    std::cout << "Temple probe wiring: exports resolved by name with the ABI's types, gate and entry points "
                 "before Prepare and any draw, no pass for No Logic, games=Mm or no pool to place, MM-only "
                 "Wallmasters run, codes map without retry, Release on every exit, no query outside a "
                 "prepared session\n";
}
'''.replace('@DECLARATIONS@', declarations)

cpp = work/'temple_native_wiring.cpp'
cpp.write_text(tests, encoding='utf-8')
exe = hostcxx.build(args.compiler, cpp, includes=[source/'combo', args.json_include],
                    defines=['_CRT_SECURE_NO_WARNINGS'], werror=True, msvc=['/bigobj', '/O2'], gnu=['-O2'])
subprocess.run([str(exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)
