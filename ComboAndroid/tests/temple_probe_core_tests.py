"""The production Temple probe core (TempleProbeCore.h) on the host, against the pinned contract.

Compiles the real core with the real Region/Entrance/Event age-time code (extracted) and runs the
host-runnable rows of the contract's matrix on small native-shaped graphs. Also pins the native code
the probe reuses or mirrors, and checks the generated bindings agree with the audit. Real-engine,
device and pinned-TypeScript differentials are not run here.
"""
from pathlib import Path
import argparse
import hashlib
import json
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
core = (randomizer/'TempleProbeCore.h').read_text(encoding='utf-8')


def squash(text):
    return re.sub(r'\s+', '', text)


# Source pins: the probe reuses ApplyTimePass/UpdateEvents and the age/time condition helpers, and
# EdgePropagate mirrors UpdateToDAccess. A change upstream must be re-audited against the probe.
PINS = {
    'UpdateToDAccess': '9789f91b671d71f09a86b03c43052a60980131eb032c4b3af6a872f4274e13e1',
    'Region::ApplyTimePass': '78ad53bd995790e5dd54a207d2923d503a2b35fd6da9d7fddfd9e4926cca9fd3',
    'Region::UpdateEvents': '83aa0a81c96dead1da44f1f1f32a60b4bdf879cdab96b57244e36cde87caaabe',
    'EventAccess::CheckConditionAtAgeTime': '4de826ab0b93e394383ab1a0bf82ea5f3d4c57f214b9c374ac0b98aa7d67e002',
    'Entrance::CheckConditionAtAgeTime': '81684f127ca7a3c4284871aa8109164ab5ef05988a97618f58201f26075c7961',
}
production = stubs.pinned(source)
digests = {name: hashlib.sha256(squash(text).encode()).hexdigest() for name, text in production.items()}
stale = {name: digest for name, digest in digests.items() if PINS[name] != digest}
assert not stale, f'Re-audit TempleProbeCore.h against changed native code, then update PINS: {stale}'

# EdgePropagate is UpdateToDAccess with an explicit parent and without the diagnostic timers.
mirror = production['UpdateToDAccess']
mirror = mirror[mirror.index('{'):]
for line in ['StartPerformanceTimer(PT_TOD_ACCESS);', 'StopPerformanceTimer(PT_TOD_ACCESS);',
             'Region* parent = entrance->GetParentRegion();']:
    assert line in mirror
    mirror = mirror.replace(line, '')
for arrow in ('connection', 'parent', 'entrance', 'logic'):
    mirror = mirror.replace(arrow + '->', arrow + '.')
propagate = stubs.function(core, 'bool EdgePropagate(Logic& logic, const Entrance& entrance, const Region& parent, Region& connection)')
assert squash(propagate[propagate.index('{'):]) == squash(mirror), 'EdgePropagate no longer mirrors UpdateToDAccess'

# The core holds no item placement, collection, age change or engine search of its own.
code = re.sub(r'//[^\n]*', '', core)
for banned in ['ItemLocation', 'locations', 'AddCheckToLogic', 'ProcessRegion', 'ProcessExits',
               'PropagateTimeTravel', 'SongAge', 'ExpandSongAgeAccess', 'OotSong', 'ApplyEffect', 'Random']:
    assert banned not in code, banned

# Generated bindings: the native table and the combo names list the same rows and roles as the audit.
audit = json.loads((source/'tools/ootmm/oot-edge-audit.json').read_text(encoding='utf-8'))
inc = (randomizer/'combo_oot_edge_bindings.inc').read_text(encoding='utf-8')
names = (source/'combo/rando/entrances/OotEdgeBindingNames.h').read_text(encoding='utf-8')
rows = re.findall(r'\{ "(\w+)", 0x[0-9a-f]+u, TempleBindingRole::(\w+), (RR_\w+), (RR_\w+), (ENTR_\w+) \},', inc)
assert sorted((n, {'Candidate': 'candidate', 'RootPolicy': 'rootPolicy'}[r], p, t, e) for n, r, p, t, e in rows) == \
       sorted((r['name'], r['role'], r['parent'], r['target'], r['nativeEntrance']) for r in audit['rows'])
candidates = re.search(r'kOotTempleCandidateRows = \{([^}]*)\}', names).group(1)
roots = re.search(r'kOotTempleRootPolicyRows = \{([^}]*)\}', names).group(1)
assert re.findall(r'"(\w+)"', candidates) == sorted(r['name'] for r in audit['rows'] if r['role'] == 'candidate')
assert re.findall(r'"(\w+)"', roots) == sorted(r['name'] for r in audit['rows'] if r['role'] == 'rootPolicy')
assert audit['registrySha256'] in names and audit['registrySha256'] in inc

tests = r'''
void Flip(StubPolicy& p, bool child) { p.startingChild = child; }
int main() {
    gLive = std::make_shared<Rando::Logic>();
    logic = gLive;
    // Both starting ages; only the opposite age at Temple of Time counts.
    World();
    StubPolicy adultStart;
    auto o = Run(adultStart, Query());
    assert(o.code == kNotReached && o.ages == 0); // adult's own spawn reaches ToT, but it is the starting age
    adultStart.starting = { KEY };
    o = Run(adultStart, Query());
    assert(o.code == kReached && o.ages == 2);     // child only
    StubPolicy childStart;
    childStart.startingChild = true;
    o = Run(childStart, Query());
    assert(o.code == kReached && o.ages == 1);     // adult only, through the adult spawn
    // A starting Prelude never helps: Root Exits' warp songs are not part of the probe graph.
    adultStart.starting = { PRELUDE };
    assert(Run(adultStart, Query()).code == kNotReached);

    // Beyond the Door of Time is not the target.
    World();
    areaTable[RR_TOT_BEYOND_DOOR_OF_TIME].exits.clear();
    Link(RR_A, RR_TOT_BEYOND_DOOR_OF_TIME, True);
    adultStart.starting = {};
    o = Run(adultStart, Query());
    assert(o.code == kNotReached && o.ages == 0);

    // Starting items flip the result; a reachable chest or a Link's Pocket item does not (no collection).
    World();
    areaTable[RR_A].locations = { KEY };
    areaTable[RR_ROOT].locations = { KEY };
    assert(Run(adultStart, Query()).code == kNotReached);
    adultStart.starting = { KEY };
    assert(Run(adultStart, Query()).code == kReached);

    // No age change: an adult-only event the child could earn after a song-age swap stays unearned.
    World();
    adultStart.starting = { SONG_OF_TIME };
    areaTable[RR_A].events.emplace_back(LOGIC_EVENT_A, [] { return logic->IsAdult; });
    areaTable[RR_A].exits.emplace_back(RR_TOT_ENTRANCE, [] { return logic->Get(LOGIC_EVENT_A); }, RR_A);
    assert(Run(adultStart, Query()).code == kNotReached);
    // An event earned at one age opens an edge evaluated at the other: the root holds both ages.
    areaTable[RR_ROOT].events.emplace_back(LOGIC_EVENT_A, [] { return logic->IsAdult; });
    assert(Run(adultStart, Query()).code == kReached);
    // A settings-gated root event works at both starting ages.
    static bool setting = false;
    World();
    areaTable[RR_ROOT].events.emplace_back(LOGIC_SETTING_EVENT, [] { return setting; });
    areaTable[RR_A].exits.emplace_back(RR_TOT_ENTRANCE, [] { return logic->Get(LOGIC_SETTING_EVENT); }, RR_A);
    adultStart.starting = {};
    areaTable[RR_ADULT_SPAWN].exits.clear();
    Link(RR_ADULT_SPAWN, RR_A, True);
    Link(RR_A, RR_TEMPLE_OF_TIME, [] { return logic->IsAdult && logic->Get(LOGIC_SETTING_EVENT); });
    for (bool child : { false, true }) {
        StubPolicy p;
        p.startingChild = child;
        setting = false;
        assert(Run(p, Query()).code == kNotReached);
        setting = true;
        assert(Run(p, Query()).code == kReached);
    }

    // Time of day: an hour-passing region gives the root night, so night-only exits open.
    World();
    areaTable[RR_A].exits.clear();
    Link(RR_A, RR_TOT_ENTRANCE, [] { return logic->AtNight; });
    assert(Run(adultStart, Query()).code == kReached);
    areaTable[RR_A].timePass = areaTable[RR_KF_LINKS_HOUSE].timePass = false;
    assert(Run(adultStart, Query()).code == kNotReached);

    // A deleted source plus an added edge carries the source row's own condition.
    World();
    Link(RR_KF_LINKS_HOUSE, RR_B, True);
    Link(RR_B, RR_C, [] { return logic->HasItem(BOOTS); });
    gBindings["ROW_BOOTS"] = { RR_B, RR_C };
    const auto candidate = Query({ "ROW_KEY_DOOR", "ROW_BOOTS" }, { { "ROW_BOOTS", "ROW_KEY_DOOR" } });
    adultStart.starting = { KEY };
    assert(Run(adultStart, Query()).code == kReached);
    assert(Run(adultStart, candidate).code == kNotReached); // the key door was deleted
    adultStart.starting = { BOOTS };
    assert(Run(adultStart, candidate).code == kReached);    // B -> ToT Entrance under the boots condition
    assert(Run(adultStart, Query({ "ROW_UNBOUND" })).code == kUnsupported);

    // Candidates do not leak: X then Y equals Y then X, and both equal fresh runs.
    adultStart.starting = { KEY };
    const auto x = Run(adultStart, Query()), y = Run(adultStart, candidate);
    assert(x.code == kReached && y.code == kNotReached);
    assert(Run(adultStart, candidate) == y && Run(adultStart, Query()) == x);
    for (const auto& r : areaTable)
        assert(!r.childDay && !r.childNight && !r.adultDay && !r.adultNight && !r.addedToPool);
    assert(logic == gLive && gLive->items.empty());

    // Containment: an injected root exit that admits the starting age is a contract violation.
    World();
    Entrance leaky(RR_CHILD_SPAWN, True, RR_ROOT_EXITS);
    adultStart.rootSpawnOverride = &leaky;
    o = Run(adultStart, Query());
    assert(o.code == kViolation && o.ages == 0);
    adultStart.rootSpawnOverride = nullptr;

    // EdgePropagate equals UpdateToDAccess on every flag combination and age/time condition.
    static int mask = 0;
    for (int condition = 0; condition < 16; ++condition)
        for (int flags = 0; flags < 256; ++flags) {
            mask = condition;
            World();
            Entrance edge(RR_B, [] {
                return ((mask & 1) && logic->IsChild && logic->AtDay) || ((mask & 2) && logic->IsChild && logic->AtNight) ||
                       ((mask & 4) && logic->IsAdult && logic->AtDay) || ((mask & 8) && logic->IsAdult && logic->AtNight);
            }, RR_A);
            auto set = [](Region& r, int bits) {
                r.childDay = bits & 1; r.childNight = bits & 2; r.adultDay = bits & 4; r.adultNight = bits & 8;
            };
            set(areaTable[RR_A], flags & 15);
            set(areaTable[RR_B], flags >> 4);
            const bool native = UpdateToDAccess(&edge, &areaTable[RR_B]);
            const Region nativeResult = areaTable[RR_B];
            set(areaTable[RR_B], flags >> 4);
            const bool mirrored = EdgePropagate(*logic, edge, areaTable[RR_A], areaTable[RR_B]);
            assert(native == mirrored && nativeResult.childDay == areaTable[RR_B].childDay &&
                   nativeResult.childNight == areaTable[RR_B].childNight &&
                   nativeResult.adultDay == areaTable[RR_B].adultDay &&
                   nativeResult.adultNight == areaTable[RR_B].adultNight);
        }
    std::cout << "Temple probe core: opposite-age target, root policy, no collection or age change, overlay, "
                 "isolation between candidates, containment and the UpdateToDAccess mirror verified on host stubs\n";
}
'''
cpp = work/'temple_probe_core.cpp'
cpp.write_text(stubs.prelude(source) + tests, encoding='utf-8')
exe = hostcxx.build(args.compiler, cpp, includes=[randomizer, args.json_include])
subprocess.run([str(exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)
