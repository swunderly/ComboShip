"""Temple probe isolation: every live byte comes back on every exit path, and forbidden calls are absent.

Runs the production RunQuery/Firewall (TempleProbeCore.h) with host stubs on success, not reached,
unsupported after parse, a throwing predicate, a throwing starting-item step, cancellation through
poll and a nested call, comparing the live Logic (scalars, events, items, owned SaveContext), every
region's flags and unrelated generation globals with an untouched snapshot. Then scans the native
probe sources for calls that would reset, search, place, collect, draw randomness or touch MM.
"""
from pathlib import Path
import argparse
import re
import subprocess
import hostcxx
import temple_probe_stubs as stubs

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--json-include', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
args = parser.parse_args()
source = args.source.resolve()
work = args.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
randomizer = source/stubs.RANDOMIZER


def code_only(text):
    """Source without comments and string literals."""
    text = re.sub(r'/\*.*?\*/', '', text, flags=re.S)
    text = re.sub(r'//[^\n]*', '', text)
    return re.sub(r'"(?:\\.|[^"\\])*"', '""', text)


probe = code_only((randomizer/'TempleProbe.cpp').read_text(encoding='utf-8'))
core = code_only((randomizer/'TempleProbeCore.h').read_text(encoding='utf-8'))
FORBIDDEN = [r'\bReachabilitySearch\b', r'\bResetLogic\b', r'\bProcessRegion\b', r'\bProcessExits\b',
             r'\bPropagateTimeTravel\b', r'\bAccessReset\b', r'\bLocationReset\b', r'\bItemReset\b',
             r'\bResetAllLocations\b', r'Combo_SOH_Rando_\w+', r'\bEnsureOracleInit\b', r'\bFinalizeSettings\b',
             r'\bRegionTable_Init\b', r'\bPlaceItem\w*', r'\bSetPlacedItem\b', r'\bSetPrice\b', r'\bAddToPool\b',
             r'\bRandom\w*\s*\(', r'\brando_state\b', r'\bApplyAllAdvancmentItems\b', r'\bSOH_Live\w*',
             r'\bSOH_Set\w*', r'OotSongOracle\(\)\s*=[^=]', r'\bOotSongArrivals\b', r'\bOotSongAgeArrivals\b',
             r'\bOotForeignKnowledge\b', r'\bGenerateStartingInventory\b', r'\bSetSaveContext\b',
             r'\bNewSaveContext\b', r'\bsharedStartingItems\b', r'\bMM_\w+\s*\(', r'\bCombo_MM\w*', r'\bBen\w*\(',
             r'\bExpandSongAgeAccess\b', r'\bCanUseSongAgeChange\b', r'\bConnect\s*\(', r'\bDisconnect\b',
             r'\bRemoveExit\b', r'\bAddExit\b', r'\bSetCondition\b', r'\bexits\s*\.\s*(push|emplace|erase|remove|clear)']
for pattern in FORBIDDEN:
    for name, text in (('TempleProbe.cpp', probe), ('TempleProbeCore.h', core)):
        assert not re.search(pattern, text), f'{name}: forbidden call {pattern}'
# The only Logic reset is the private one, and ::logic is assigned only inside ScopedLogic.
assert re.findall(r'\bReset\s*\(', probe) == ['Reset('] and 'probe->Reset(true);' in probe
assert 'Reset(' not in core
assert not re.search(r'(?<![\w.>])logic\s*=[^=]', probe + core)
# Preconditions read the oracle flag and reject a nested oracle search (P1) before any write.
assert 'if (ComboRando::OotSongOracle()) {' in probe
check = stubs.function(probe, 'static void CheckContext()')
for needle in ['gPlayState != nullptr', 'mSaveContext == &gSaveContext', 'OotOracleInitialized()',
               'RO_LOGIC_GLITCHLESS', 'RSK_SHUFFLE_ENTRANCES', 'if (songs.logicEnabled) {',
               'logic.get() != ctx->GetLogic().get()', 'RR_ROOT_EXITS', 'BoundExit(b.parent, b.target);',
               'GlobalEdges();']:
    assert needle in check, needle
# Starting warp songs warp from the virtual GLOBAL node (limits design, Stage 2): no native refusal is left, the
# GLOBAL edges are Root Exits' own, and a warp region must lead only to its vanilla pad.
raw_probe = (randomizer/'TempleProbe.cpp').read_text(encoding='utf-8')
assert 'starting warp songs are not supported' not in raw_probe and 'StartingInventory' not in check
for needle in ['if (b.role == TempleBindingRole::Global) {', 'WarpLanding(b.target);',
               '&BoundExit(b.parent, b.target)']:
    assert needle in stubs.function(probe, 'std::vector<AddedEdge<Entrance>> GlobalEdges()'), needle
assert 'overlay.global = GlobalEdges();' in stubs.function(probe, 'Overlay<Entrance> Resolve(const QueryRequest& q) const')
# Foreign-song logic is inert in the probe (lemma S1): only song destination shuffle is refused.
assert 'logicForeignSongs' not in check and 'logicForeignSongs' not in core
run_query = stubs.function(core, 'int RunQuery(Policy& policy, const char* json, int (*poll)(void), uint8_t* agesOut)')
order = [run_query.index(step) for step in ['DepthGuard depth;', 'policy.Check();', 'policy.Resolve(',
                                            'policy.MakeLogic();', 'RegionFlagGuard', 'ScopedLogic',
                                            'policy.ApplyStartingItems();', 'Search(']]
assert order == sorted(order), 'a write happens before a precondition'

tests = r'''
uint32_t OotSongArrivals = 1u << 5; // adversarial: arrival slot 5 is Temple of Time natively
uint16_t prices[2] = { 200, 200 };
uint32_t rngWord = 0xdeadbeef;
SaveContext liveSave;
struct Snapshot {
    bool scalars[4];
    RandomizerRegion region;
    uint8_t hearts;
    bool events[LOGIC_MAX];
    std::set<int> items;
    SaveContext* save;
    uint8_t saveBytes[64];
    std::vector<std::array<bool, 5>> flags;
    uint32_t arrivals, rng;
    uint16_t price0, price1;
    Rando::Logic* handle;
    bool operator==(const Snapshot& o) const {
        return std::memcmp(scalars, o.scalars, sizeof(scalars)) == 0 && region == o.region && hearts == o.hearts &&
               std::memcmp(events, o.events, sizeof(events)) == 0 && items == o.items && save == o.save &&
               std::memcmp(saveBytes, o.saveBytes, sizeof(saveBytes)) == 0 && flags == o.flags &&
               arrivals == o.arrivals && rng == o.rng && price0 == o.price0 && price1 == o.price1 && handle == o.handle;
    }
};
Snapshot Take() {
    Snapshot s{ { gLive->IsChild, gLive->IsAdult, gLive->AtDay, gLive->AtNight }, gLive->CurrentRegionKey,
                gLive->BaseHearts, {}, gLive->items, gLive->mSaveContext, {}, {}, OotSongArrivals, rngWord,
                prices[0], prices[1], logic.get() };
    std::memcpy(s.events, gLive->inLogic, sizeof(s.events));
    std::memcpy(s.saveBytes, gLive->mSaveContext->bytes, sizeof(s.saveBytes));
    for (const auto& r : areaTable)
        s.flags.push_back({ r.childDay, r.childNight, r.adultDay, r.adultNight, r.addedToPool });
    return s;
}
void LiveState() {
    World();
    // The outer fill's last search left flags behind; a probe must not erase them.
    int i = 0;
    for (auto& r : areaTable) {
        r.childDay = i % 2; r.childNight = i % 3 == 0; r.adultDay = i % 5 == 0; r.adultNight = i % 7 == 0;
        r.addedToPool = i % 4 == 0;
        ++i;
    }
}
int gNested = 1;
int gPolls = 0;
int CancelOnSecondPass() { return ++gPolls >= 2; }
int main() {
    gLive = std::make_shared<Rando::Logic>();
    gLive->IsChild = gLive->AtNight = true;
    gLive->CurrentRegionKey = RR_C;
    gLive->BaseHearts = 3;
    gLive->inLogic[LOGIC_EVENT_B] = true;
    gLive->items = { SWORD };
    for (int i = 0; i < 64; ++i)
        liveSave.bytes[i] = uint8_t(i * 7 + 1);
    gLive->mSaveContext = &liveSave;
    logic = gLive;
    StubPolicy policy; // adult start: the child is probed
    auto expectRestored = [&](const Snapshot& before, const Outcome& o, int code) {
        assert(o.code == code);
        assert(Take() == before && !policy.scope && logic == gLive);
        if (code < 0 && code != kCancelled)
            assert(!o.error.empty() && o.ages == 0);
    };

    LiveState();
    auto before = Take();
    policy.starting = { KEY };
    auto o = Run(policy, Query());
    expectRestored(before, o, kReached);
    assert(o.ages == 2 && o.error.empty());

    // GLOBAL (limits design, Stage 2): a query that warps restores everything too, and so does one whose warp
    // song's predicate throws or that the poll cancels while the warp is open.
    policy.starting = { PRELUDE, OCARINA, NOTE };
    o = Run(policy, Query());
    expectRestored(before, o, kReached);
    assert(o.ages == 2 && o.error.empty());
    gPolls = 0;
    expectRestored(before, Run(policy, Query(), CancelOnSecondPass), kCancelled);
    assert(gPolls == 2);
    for (auto it = areaTable[RR_ROOT_EXITS].exits.begin(); it != areaTable[RR_ROOT_EXITS].exits.end(); ++it)
        if (it->GetConnectedRegionKey() == RR_PRELUDE_WARP) {
            areaTable[RR_ROOT_EXITS].exits.erase(it);
            break;
        }
    Link(RR_ROOT_EXITS, RR_PRELUDE_WARP, []() -> bool { throw std::runtime_error("warp predicate"); });
    o = Run(policy, Query());
    expectRestored(before, o, kInternal);
    assert(o.error.find("warp predicate") != std::string::npos);
    LiveState();
    before = Take();

    policy.starting = {};
    expectRestored(before, Run(policy, Query()), kNotReached);
    expectRestored(before, Run(policy, Query({ "ROW_UNBOUND" })), kUnsupported);
    expectRestored(before, Run(policy, "{\"schema\":\"oot-temple-probe-query-v1\"}"), kUnsupported);
    auto badRegistry = nlohmann::json::parse(Query());
    badRegistry["registry"] = "other";
    expectRestored(before, Run(policy, badRegistry.dump()), kUnsupported);
    auto assumed = nlohmann::json::parse(Query());
    assumed["assumed"] = { "ROW_KEY_DOOR" };
    expectRestored(before, Run(policy, assumed.dump()), kUnsupported);
    assumed["assumed"] = nlohmann::json::array();
    assumed["assumedInstalled"] = false;
    expectRestored(before, Run(policy, assumed.dump()), kViolation);
    uint8_t ages = 9;
    std::string error;
    assert(Firewall(error, [&] { return RunQuery(policy, Query().c_str(), nullptr, nullptr); }) == kUnsupported);
    assert(Take() == before && ages == 9);

    // A predicate that throws mid-search.
    areaTable[RR_A].events.emplace_back(LOGIC_EVENT_A, []() -> bool { throw std::runtime_error("predicate"); });
    before = Take();
    o = Run(policy, Query());
    expectRestored(before, o, kInternal);
    assert(o.error.find("internal") != std::string::npos);
    areaTable[RR_A].events.clear();

    // The starting-item step throws while the private Logic is installed.
    before = Take();
    policy.onApply = [] { throw std::runtime_error("item"); };
    expectRestored(before, Run(policy, Query()), kInternal);
    policy.onApply = [] {};

    // Cancellation through poll on the second pass.
    gPolls = 0;
    o = Run(policy, Query(), CancelOnSecondPass);
    expectRestored(before, o, kCancelled);
    assert(gPolls == 2 && o.ages == 0);

    // A nested probe from inside a predicate is rejected before it writes anything; the outer query
    // continues and still restores everything.
    areaTable[RR_A].events.emplace_back(LOGIC_EVENT_A, [] {
        StubPolicy nested;
        gNested = Run(nested, Query()).code;
        assert(nested.applied == 0 && logic != gLive);
        return false;
    });
    before = Take();
    policy.applied = 0;
    expectRestored(before, Run(policy, Query()), kNotReached);
    assert(gNested == kViolation && policy.applied == 1);
    areaTable[RR_A].events.clear();

    // A failed precondition (for example a nested oracle search) stops before any write.
    policy.check = [] { throw Failure(kViolation, "called inside an oracle search"); };
    policy.applied = 0;
    before = Take();
    expectRestored(before, Run(policy, Query()), kViolation);
    assert(policy.applied == 0);
    policy.check = [] {};

    // The handle must still be the Context's Logic afterwards.
    std::cout << "Temple probe isolation: live Logic, region flags and generation globals restored on success, "
                 "false, unsupported, throw, cancel, nesting and failed preconditions, GLOBAL warps included\n";
}
'''
cpp = work/'temple_probe_isolation.cpp'
cpp.write_text(stubs.prelude(source) + tests, encoding='utf-8')
exe = hostcxx.build(args.compiler, cpp, includes=[randomizer, args.json_include])
subprocess.run([str(exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)
