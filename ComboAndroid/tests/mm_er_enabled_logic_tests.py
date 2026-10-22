"""M1-08: OoTMM's er_enabled and has_hot_water_er in the MM logic, against the pinned M1 contract.

tools/ootmm/er-enabled-sites.json is the one site receipt (critique X6): every OoTMM v32.3 line that
tools/ootmm/m1-contract.json erLogicSites lists, with the fork statement that carries it or the reason
it has none. This checks:
  - the receipt covers exactly the fixture's lines; every N/A line reaches er_enabled only through the
    Snowhead trick, which the port does not have; the shared Goron Elder lines have the same thaw
    alternatives;
  - Logic.h defines ER_ENABLED and HAS_HOT_WATER_ER as the receipt maps them (false without COMBO_BUILD),
    and nothing else in the MM logic uses them; a carrier macro (CAN_BRING_HOT_WATER_NORTH) is its old
    body with HAS_HOT_WATER_ER ORed on, and in COMBO_BUILD code only its receipt statements read it;
  - no CHECK names the ER terms: CHECK stringifies its condition and the MM Check Tracker shows that text
    as a tooltip in every build, so release builds must keep it;
  - each mapped statement is in its region or function; with er_enabled off it evaluates exactly as
    before; with it on it gains a route (or is already subsumed, as the receipt says); and the term sits
    beside the same existing alternative in the fork as has_hot_water_er sits in OoTMM (for a statement
    carried by a macro: its old text with the term ORed beside the alternative equals its new meaning);
  - EntranceRuntimeFlags.h ErEnabled and IsEntranceShuffle against the fixture's 66 combinations
    (Water Voids alone and Anywhere alone are both false), and, with the OoTMM checkout, term by term
    against macros_mm.yml:305 and helpers.ts:6-55;
  - the real MM exports (ComboEntrancesMM.cpp, compiled on the host): the logic's er_enabled is off with
    nothing or a private profile installed, follows an installed unified layout, follows the
    generation's settings (thread-local, untouched by binds and assumption calls, refused when this
    build's capabilities do not admit them), and Song Travel never counts.
Statements are evaluated symbolically with mm_gear_logic_tests' C expression reader: ER_ENABLED,
HAS_HOT_WATER_ER, CAN_BRING_HOT_WATER_NORTH and BlacksmithCanThaw's local are expanded, every other
macro or call is an unknown.
"""
from pathlib import Path
import argparse
import json
import random
import re
import subprocess
import sys
import hostcxx

sys.path.insert(0, str(Path(__file__).resolve().parent))
import mm_gear_logic_tests as cexpr  # noqa: E402 (its C expression reader and evaluator)

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--json-include', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
parser.add_argument('--reference', type=Path)
args = parser.parse_args()
root = args.source.resolve()
work = args.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
failures = []


def check(ok, message):
    if not ok:
        failures.append(message)


def read(rel):
    return (root / rel).read_text(encoding='utf-8')


receipt = json.loads(read('tools/ootmm/er-enabled-sites.json'))
fixture = json.loads(read('tools/ootmm/m1-contract.json'))
assert receipt['upstreamCommit'] == fixture['upstreamCommit'][:7], 'receipt and fixture pin different OoTMM commits'
TRICK_MACROS = {'trick_sht_hot_water', 'trick_sht_hot_water_er'}
HER = 'HAS_HOT_WATER_ER'
ER_KEY = ('call', 'Rando::Logic::NativeMmErEnabled', ())

# ---- Coverage: exactly the fixture's lines, with their macros.
key = lambda row: f'{row["file"]}:{row["line"]}'
pinned_lines = {key(row): sorted(row['uses']) for row in fixture['erLogicSites']['lines']}
rows = {key(site['pinned']): site for site in receipt['sites']}
check(len(rows) == len(receipt['sites']), 'receipt lists a line twice')
check(set(rows) == set(pinned_lines), f'receipt lines differ from erLogicSites: {sorted(set(rows) ^ set(pinned_lines))}')
for name, site in rows.items():
    check(sorted(site['pinned']['uses']) == pinned_lines.get(name), f'{name}: uses differ from the fixture')
    check(site['status'] in ('mapped', 'shared', 'macro', 'n/a'), f'{name}: unknown status')
statuses = [site['status'] for site in receipt['sites']]
check(statuses.count('mapped') == 6 and statuses.count('shared') == 1 and statuses.count('macro') == 2 and
      statuses.count('n/a') == 13, f'unexpected status counts {statuses}')

# ---- N/A: only through the Snowhead trick, which the port does not have.
tricks = read('combo/rando/MmTricks.h')
trick_keys = re.findall(r'"gCombo\.Tricks\.MM\.(\w+)"', tricks)
check(len(trick_keys) >= 30 and not any('hot' in k.lower() or k.lower().startswith('sht') for k in trick_keys),
      'combo/rando/MmTricks.h has a Snowhead hot-water trick: the N/A rows are stale')
check('Hot Spring Water' not in tricks, 'combo/rando/MmTricks.h names Hot Spring Water: the N/A rows are stale')
logic_sources = sorted((root / 'mm/2s2h/Rando').rglob('*.[ch]*')) + sorted((root / 'combo/rando').rglob('*.h'))
for path in logic_sources:
    text = path.read_text(encoding='utf-8', errors='replace')
    check('MM_SHT_HOT_WATER' not in text and 'ShtHotWater' not in text, f'{path}: names the Snowhead hot-water trick')
for name, site in rows.items():
    if site['status'] != 'n/a':
        continue
    uses = set(site['pinned']['uses'])
    if site['pinned']['file'] == 'data/macros/macros_mm.yml':
        check(uses - {'has_hot_water_er'} <= TRICK_MACROS and len(uses & TRICK_MACROS) == 1, f'{name}: not a trick macro')
    else:
        check(uses <= TRICK_MACROS, f'{name}: N/A but uses {sorted(uses - TRICK_MACROS)}')
    check('MM_SHT_HOT_WATER' in site['reason'] or 'trick_sht_hot_water' in site['reason'], f'{name}: no reason')

# ---- Logic.h: the two macros, and no other user in the MM logic.
logic_h = read('mm/2s2h/Rando/Logic/Logic.h')
macros_combo = cexpr.defines(logic_h, lambda n: n in ('ER_ENABLED', HER, 'CAN_BRING_HOT_WATER_NORTH'))
check(set(macros_combo) == {'ER_ENABLED', HER, 'CAN_BRING_HOT_WATER_NORTH'}, 'Logic.h lacks a COMBO_BUILD define')
for entry in receipt['macros']:
    fork = entry['fork']
    body = ''.join(t[1] for t in macros_combo.get(fork['define'], (None, []))[1])
    check(cexpr.squash(body) == cexpr.squash(fork['body']), f'Logic.h {fork["define"]} is not {fork["body"]}')
for define in ('ER_ENABLED', HER):
    check(re.search(r'#else\s*\n(?:#define[^\n]*\n)*#define ' + define + r' false\n', logic_h) is not None,
          f'Logic.h {define} is not false without COMBO_BUILD')
check(len(re.findall(r'\bHAS_HOT_WATER_ER\b', logic_h)) == 2 + len(receipt['carriers']) and
      len(re.findall(r'\bER_ENABLED\b', logic_h)) == 3,
      'Logic.h uses ER_ENABLED or HAS_HOT_WATER_ER outside their defines and the carriers')
# A carrier is its old body with the term ORed on, nothing else, and carries exactly its receipt rows.
carriers = {c['fork']['define']: c for c in receipt['carriers']}
for define, carrier in carriers.items():
    body = ''.join(t[1] for t in macros_combo.get(define, (None, []))[1])
    check(cexpr.squash(body) == cexpr.squash(carrier['fork']['body']), f'Logic.h {define} is not {carrier["fork"]["body"]}')
    check(cexpr.tokenize(carrier['fork']['body']) ==
          [('op', '(')] + cexpr.tokenize(carrier['fork']['before']) + [('op', '||'), ('id', HER), ('op', ')')],
          f'{define}: the receipt body is not its old body || {HER}')
    check(sorted(carrier['sites']) == sorted(n for n, s in rows.items() if s.get('fork', {}).get('via') == define),
          f'{define}: carries other rows than the receipt lists')


def combo_lines(text):
    """The lines of text that a COMBO_BUILD compile sees (#else of #ifdef COMBO_BUILD dropped)."""
    out, stack = [], []
    for line in text.replace('\r\n', '\n').split('\n'):
        directive = line.strip()[1:].split() if line.strip().startswith('#') else []
        word = directive[0] if directive else ''
        if word in ('ifdef', 'ifndef'):
            stack.append(None if directive[1] != 'COMBO_BUILD' else word == 'ifdef')
        elif word == 'if':
            stack.append(None)
        elif word == 'elif':
            stack[-1] = None
        elif word == 'else':
            stack[-1] = None if stack[-1] is None else not stack[-1]
        elif word == 'endif':
            stack.pop()
        elif all(x is not False for x in stack):
            out.append(line)
    return out


afters = [cexpr.squash(s['fork']['after']) for s in receipt['sites'] if s['status'] == 'mapped']
carried = {d: [cexpr.squash(rows[n]['fork']['after']) for n in c['sites'] if n in rows] for d, c in carriers.items()}
listed = lambda line, statements: any(cexpr.squash(line).rstrip(',;') in s or s in cexpr.squash(line) for s in statements)
for path in sorted((root / 'mm/2s2h').rglob('*.[ch]*')):
    if path.name == 'Logic.h':
        continue
    text = path.read_text(encoding='utf-8', errors='replace')
    for line in cexpr.strip_comments(text).splitlines():
        if re.search(r'\b(?:HAS_HOT_WATER_ER|ER_ENABLED)\b', line):
            check(listed(line, afters), f'{path.relative_to(root)}: unlisted er_enabled site {line.strip()[:100]}')
    for line in combo_lines(cexpr.strip_comments(text)):
        for define, statements in carried.items():
            if re.search(r'\b' + define + r'\b', line):
                check(listed(line, statements), f'{path.relative_to(root)}: {define} carries {HER} to an unlisted '
                      f'statement {line.strip()[:100]}')

# ---- The MM Check Tracker shows every CHECK's condition text (Logic.h CHECK: LogicString(#condition), which
# expands no macro; CheckTracker.cpp Init keeps it for the tooltip) in every build: no CHECK names an ER term.
check_define = logic_h[logic_h.index('#define CHECK(check, condition)'):]
check('LogicString(#condition)' in check_define[:check_define.index('\n#', 1)],
      'Logic.h CHECK no longer stringifies its condition: revisit the CHECK rule below')
check('accessLogicFuncs[randoCheckId] = accessLogicFunc.second;' in read('mm/2s2h/Rando/CheckTracker/CheckTracker.cpp'),
      'CheckTracker.cpp no longer shows CHECK condition text: revisit the CHECK rule below')
for path in sorted((root / 'mm/2s2h/Rando/Logic').rglob('*.[ch]*')):
    text = path.read_text(encoding='utf-8', errors='replace')
    masked = re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"', lambda m: ' ' * len(m.group()), text)
    for m in re.finditer(r'\bCHECK\s*\(', masked):
        depth = 0
        for end in range(m.end() - 1, len(masked)):
            depth += (masked[end] == '(') - (masked[end] == ')')
            if depth == 0:
                break
        call = masked[m.start():end + 1]
        check(re.search(r'\b(?:HAS_HOT_WATER_ER|ER_ENABLED)\b', call) is None,
              f'{path.relative_to(root)}: a CHECK names an ER term, which changes its Check Tracker text in release '
              f'builds; carry it in a macro instead: {" ".join(call.split())[:100]}')
entrances_mm = read('mm/2s2h/Rando/Logic/ComboEntrancesMM.cpp')
flags_h = read('combo/rando/entrances/EntranceRuntimeFlags.h')
unified_h = read('combo/rando/entrances/MmUnifiedLayout.h')
for text, what in ((entrances_mm, 'ComboEntrancesMM.cpp'), (flags_h, 'EntranceRuntimeFlags.h')):
    check('SongTravel' not in cexpr.strip_comments(text), f'{what} reads Song Travel (D5: only er* settings count)')
er_reader = unified_h[unified_h.index('inline bool MmUnifiedErEnabled()'):]
er_reader = er_reader[:er_reader.index('\n}\n')]
check('SongTravel' not in er_reader and 'ActiveMmWallmasters' not in er_reader, 'MmUnifiedErEnabled reads more than er*')


# ---- Mapped statements: evaluated symbolically.
def function_text(text, signature):
    start = text.index(signature)
    opening = text.index('{', start)
    depth = 0
    for end in range(opening, len(text)):
        depth += (text[end] == '{') - (text[end] == '}')
        if depth == 0:
            return text[start:end + 1]
    raise AssertionError(signature)


def expression(statement):
    tokens = cexpr.tokenize(statement)
    while tokens and tokens[-1][1] in (',', ';'):
        tokens = tokens[:-1]
    if tokens and tokens[0] == ('id', 'return'):
        return tokens[1:]
    return cexpr.condition(statement)


def tree(tokens, macros):
    return cexpr.Parser(cexpr.expand(tokens, macros)).parse()


def first_difference(a, b, trials, rng, fixed):
    """A valuation of the unknowns on which trees a and b differ, or None."""
    for n in range(trials):
        cache = dict(fixed)
        p = (0.5, 0.85, 0.15)[n % 3]

        def value(k):
            if k not in cache:
                cache[k] = int(rng.random() < p)
            return cache[k]
        if bool(cexpr.evaluate(a, value)) != bool(cexpr.evaluate(b, value)):
            return cache
    return None


def placed(tokens, term, alternative):
    """tokens with term removed and ORed onto the one occurrence of alternative instead."""
    out = [('id', 'false') if t == ('id', term) else t for t in tokens]
    alt = cexpr.tokenize(alternative)
    hits = [i for i in range(len(out) - len(alt) + 1) if out[i:i + len(alt)] == alt]
    assert len(hits) == 1, (alternative, hits)
    i = hits[0]
    return out[:i] + [('op', '(')] + alt + [('op', '||'), ('id', term), ('op', ')')] + out[i + len(alt):]


rng = random.Random(20261014)
mapped = 0
for name, site in rows.items():
    if site['status'] == 'shared':
        other = rows.get(site['sameAs'])
        check(other is not None and other['status'] == 'mapped', f'{name}: shares an unmapped row')
        group = lambda e: e[e.rindex('(can_use_fire_short_range'):]
        check(other is not None and group(site['pinned']['expression']) == group(other['pinned']['expression']),
              f'{name}: thaw alternatives differ from {site["sameAs"]}')
    if site['status'] not in ('mapped', 'shared'):
        continue
    # OoTMM: has_hot_water_er sits beside the named alternative.
    pinned = cexpr.tokenize(site['pinned']['expression'])
    p_tree = tree(pinned, {})
    p_placed = tree(placed(pinned, 'has_hot_water_er', site['pinned']['alternative']), {})
    check(first_difference(p_tree, p_placed, 3000, rng, {}) is None,
          f'{name}: has_hot_water_er is not beside {site["pinned"]["alternative"]} in OoTMM')
    if site['status'] == 'shared':
        continue
    fork = site['fork']
    text = read(fork['file'])
    macros = dict(macros_combo)
    old_macros = dict(macros)  # before the change: a carrier with its old body
    if 'via' in fork:
        old_macros[fork['via']] = (None, cexpr.tokenize(carriers[fork['via']]['fork']['before']))
        check(fork['before'] == fork['after'], f'{name}: a statement carried by {fork["via"]} changed its text')
    if 'region' in fork:
        scope = cexpr.region(text, fork['region'])
    else:
        scope = function_text(text, f'static bool {fork["function"]}(')
        local = re.search(r'const bool (\w+) = ([^;]+);', scope)
        macros[local.group(1)] = old_macros[local.group(1)] = (None, cexpr.tokenize(local.group(2)))
        combo = scope[scope.index('#ifdef COMBO_BUILD'):scope.index('#else')]
        check(cexpr.squash(fork['after']) in cexpr.squash(combo), f'{name}: not in the COMBO_BUILD branch')
    check(cexpr.squash(fork['after']) in cexpr.squash(scope), f'{name}: {fork["file"]} lacks {fork["after"][:80]}')
    before, after = tree(expression(fork['before']), old_macros), tree(expression(fork['after']), macros)
    check(first_difference(before, after, 3000, rng, {ER_KEY: 0}) is None, f'{name}: changes with er_enabled off')
    changed = first_difference(before, after, 3000, rng, {ER_KEY: 1})
    if site['effect'] == 'adds':
        check(changed is not None, f'{name}: gains nothing with er_enabled on')
    else:
        check(site['effect'] == 'subsumed' and changed is None, f'{name}: not subsumed as the receipt says')
    if 'via' in fork:  # the old statement with the term ORed beside the alternative
        f_placed = tree(placed(expression(fork['before']), HER, fork['alternative']), old_macros)
    else:  # the term moved from where it is to beside the alternative
        f_placed = tree(placed(expression(fork['after']), HER, fork['alternative']), macros)
    check(first_difference(after, f_placed, 3000, rng, {}) is None,
          f'{name}: HAS_HOT_WATER_ER is not beside {fork["alternative"]}')
    mapped += 1
check(mapped == 6, f'{mapped} mapped statements checked')

# ---- OoTMM itself, when the checkout is there.
reference = args.reference
if reference is None:
    for candidate in (root.parent / 'references/ootmm-stable-32.3', root.parent.parent / 'references/ootmm-stable-32.3'):
        if (candidate / 'data/macros/macros_mm.yml').exists():
            reference = candidate
            break


def terms_cpp(function):
    body = function_text(flags_h, f'inline bool {function}(const Settings& s)')
    out = []
    for m in re.finditer(r'Active\(s\.(\w+)\)|s\.(\w+)\s*!=\s*Warps::None|s\.(\w+)', body[body.index('return'):]):
        out.append((m.group(1) or m.group(2), 'not none') if (m.group(1) or m.group(2)) else (m.group(3), 'true'))
    return out


if reference is not None:
    lines = lambda rel: (reference / rel).read_text(encoding='utf-8').splitlines()
    for name, site in rows.items():
        line = lines(site['pinned']['file'])[site['pinned']['line'] - 1]
        if 'expression' in site['pinned']:
            check(f'"{site["pinned"]["expression"]}"' in line, f'{name}: OoTMM line differs from the receipt')
        for macro in site['pinned']['uses']:
            check(re.search(r'\b' + macro + r'\b', line) is not None, f'{name}: OoTMM line lacks {macro}')
    macros_mm = lines('data/macros/macros_mm.yml')
    for entry in receipt['macros'] + receipt['carriers']:
        p = entry['pinned']
        check(macros_mm[p['line'] - 1].startswith(f'"{p["name"]}": "' + p.get('expression', '')),
              f'macros_mm.yml:{p["line"]} is not {p["name"]}')
    for trick_macro in (221, 222):
        check('&& trick(MM_SHT_HOT_WATER)"' in macros_mm[trick_macro - 1], f'macros_mm.yml:{trick_macro} lost its trick')
    er_line = macros_mm[304]
    pinned_er = [(m.group(2), 'not none') if m.group(1) else (m.group(2), 'true')
                 for m in re.finditer(r'(!)?setting\((\w+)(?:, none)?\)', er_line.split(': ', 1)[1])]
    check(terms_cpp('ErEnabled') == pinned_er, f'ErEnabled differs from macros_mm.yml:305 {pinned_er}')
    helpers = '\n'.join(lines('packages/logic/src/helpers.ts')[5:55])
    pinned_ies = [(m.group(1), 'not none' if m.group(2) else 'true')
                  for m in re.finditer(r"if \(settings\.(\w+)( !== 'none')?\)", helpers)]
    check(terms_cpp('IsEntranceShuffle') == pinned_ies, f'IsEntranceShuffle differs from helpers.ts:6-55 {pinned_ies}')
    tricks_ts = lines('packages/core/src/settings/tricks.ts')
    check(tricks_ts[467].strip() == 'MM_SHT_HOT_WATER: {', 'tricks.ts:468 is not MM_SHT_HOT_WATER')
    default = next(l for l in tricks_ts if l.startswith('export const DEFAULT_TRICKS'))
    check('MM_SHT_HOT_WATER' not in default, 'MM_SHT_HOT_WATER became a default trick')
    print(f'OoTMM reference checked: {reference}')
else:
    print('OoTMM reference not found; checked the production sources against the fixture only')

if failures:
    sys.exit('\n'.join(failures))


# ---- Compiled: the flags against the fixture, and the real MM exports.
def function(text, signature):
    start = hostcxx.find(text, signature)
    opening = text.index('{', start)
    masked = re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"', lambda m: ' ' * len(m.group()), text)
    depth = 0
    for end in range(opening, len(text)):
        depth += (masked[end] == '{') - (masked[end] == '}')
        if depth == 0:
            return text[start:end + 1]
    raise AssertionError('Unterminated production function ' + signature)


includes = '\n'.join(re.findall(r'^#include "rando/entrances/[^"]+"$', entrances_mm, re.M))
file_locals = entrances_mm[hostcxx.find(entrances_mm, 'namespace CE = ComboRando::Entrances;'):
                           hostcxx.find(entrances_mm, 'extern "C" COMBO_EXPORT int MM_ApplyEntranceExpansionForCombo(')]
exports = '\n'.join(function(entrances_mm, s) for s in (
    'extern "C" COMBO_EXPORT int MM_ApplyEntranceExpansionForCombo(',
    'extern "C" COMBO_EXPORT int MM_SetEntranceExpansionAssumptionsForCombo(',
    'extern "C" COMBO_EXPORT const char* MM_DumpEntranceExpansionForCombo(',
    'extern "C" COMBO_EXPORT void MM_ClearEntranceExpansionAssumptionsForCombo(',
    'extern "C" COMBO_EXPORT int MM_SetEntranceGenerationSettingsForCombo(',
    'bool Rando::Logic::NativeMmErEnabled(',
    'extern "C" int Combo_MMEntranceShuffleActive(',
))
assert exports.count('CE::ActiveCapabilities()') == 3
exports = exports.replace('CE::ActiveCapabilities()', 'HarnessCapabilities()')

combos = []
for c in fixture['combinations']:
    s = c['settings']
    sets = [f's.erWallmasters = CE::Mode::{"OwnGame" if s["erWallmasters"] == "ownGame" else "None"};',
            f's.erOneWays = CE::Mode::{"OwnGame" if s["erOneWays"] == "ownGame" else "None"};']
    sets += [f's.{k} = {str(s[k]).lower()};' for k in ('erOneWaysMajor', 'erOneWaysIkana', 'erOneWaysOwls',
                                                       'erOneWaysWaterVoids', 'erOneWaysAnywhere')]
    places = any(group['sources'] for pool in c['pools'] for group in pool['candidates'])
    combos.append('    {[] { CE::Settings s; %s return s; }(), %s, %s, %s, "%s"},' % (
        ' '.join(sets), str(c['erEnabled']).lower(), str(c['isEntranceShuffle']).lower(), str(places).lower(), c['id']))

program = r'''
#define NOMINMAX
#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
''' + includes + r'''
#include "rando/entrances/MmRegionalFlags.h" // through 2Ship's Logic.h in the game
#include "rando/entrances/UnifiedBridge.h"
#define COMBO_EXPORT
static int loggedErrors = 0;
template <class... A> void LogError(const char*, const A&...) { ++loggedErrors; }
#define SPDLOG_ERROR(...) LogError(__VA_ARGS__)
static bool harnessRando = true;
#define IS_RANDO harnessRando
using s32 = int32_t;
enum RandoRegionId {
    RR_BENEATH_THE_GRAVEYARD_DAMPE = 1,
    RR_BENEATH_THE_WELL_FREEZARD_ROOM,
    RR_BENEATH_THE_WELL_TWO_SPIKED_BARS,
    RR_BENEATH_THE_WELL_FOUR_SPIKED_BARS,
};
static ComboRando::Entrances::CapabilitySet harnessCaps = ComboRando::Entrances::ActiveCapabilities();
static ComboRando::Entrances::CapabilitySet HarnessCapabilities() { return harnessCaps; }
namespace Rando::Logic {
bool NativeMmErEnabled(); // Logic.h
}
''' + file_locals + '\n' + exports + r'''
using Json = nlohmann::json;
struct Combination {
    CE::Settings settings;
    bool erEnabled, entranceShuffle, places;
    const char* id;
};
const std::vector<Combination> kCombinations = {
''' + '\n'.join(combos) + r'''
};

CE::CapabilitySet WholeCatalog() {
    auto caps = CE::CapabilitySet::AtLeast(CE::Stage::Hidden);
    caps.devBuild = true;
    return caps;
}
// A settings document as the generator writes it (UnifiedBridge.h).
std::string Document(const CE::Settings& s, std::string required = {}) {
    CE::EntranceLayout layout;
    layout.required = {required.empty() ? CE::UnifiedRequirement(s, {}) : required};
    layout.devBuild = true;
    layout.settings = s;
    return CE::EncodeEntranceLayout(layout).dump();
}
CE::Settings Tranche() {
    CE::Settings s;
    s.erWallmasters = CE::Mode::OwnGame;
    return s;
}
Json Seed() {
    CE::EntranceLayout layout;
    layout.required = {std::string(CE::kMmTrancheCapability)};
    layout.settings = Tranche();
    CE::Overrides routes;
    const char* to[] = {"MM_TEMPLE_WOODFALL", "MM_TEMPLE_SNOWHEAD", "MM_TEMPLE_GREAT_BAY", "MM_SPIDER_HOUSE_SWAMP"};
    for (size_t i = 0; i < CE::kMmWallmasterSources.size(); ++i)
        routes[CE::LayoutV1::FindRow(CE::kMmWallmasterSources[i])->name] = CE::LayoutV1::FindRow(to[i])->name;
    layout.overrides = routes;
    return {{"fileType", ComboRando::kEntranceSeedFileType}, {"version", ComboRando::kEntranceSeedVersion},
            {"entranceLayout", CE::EncodeEntranceLayout(layout)}};
}
Json LegacySeed() {
    std::vector<CE::SavedOverride> rows;
    for (size_t i = 0; i < CE::kMmWallmasterSources.size(); ++i) {
        const auto* src = CE::Find(CE::kMmWallmasterSources[i]);
        const auto* dst = CE::Find(CE::kMmBaseWallmasterDestinations[i]);
        rows.push_back({std::string(src->name), std::string(dst->name), src->game, dst->game, src->id, dst->id});
    }
    return {{"entranceExpansion", CE::EncodeMmWallmasterExtension(rows)}};
}
bool Er() { return Rando::Logic::NativeMmErEnabled(); }
bool ErOnAnotherThread() {
    bool result = true;
    std::thread([&] { result = Er(); }).join();
    return result;
}

void TruthTables() {
    assert(kCombinations.size() == 66);
    for (const auto& c : kCombinations) {
        if (CE::ErEnabled(c.settings) != c.erEnabled || CE::IsEntranceShuffle(c.settings) != c.entranceShuffle) {
            std::cerr << "flags differ from the fixture for " << c.id << "\n";
            assert(false);
        }
    }
    // Water Voids alone and Anywhere alone count in neither; Song Travel is not a setting here (D5).
    CE::Settings s;
    s.erOneWays = CE::Mode::OwnGame;
    s.erOneWaysWaterVoids = true;
    assert(!CE::ErEnabled(s) && !CE::IsEntranceShuffle(s));
    s.erOneWaysWaterVoids = false;
    s.erOneWaysAnywhere = true;
    assert(!CE::ErEnabled(s) && !CE::IsEntranceShuffle(s));
    s.erOneWaysAnywhere = false;
    s.erOneWaysWoods = true; // OoTMM counts Woods in neither
    assert(!CE::ErEnabled(s) && !CE::IsEntranceShuffle(s));
    assert(!CE::ErEnabled({}) && !CE::IsEntranceShuffle({}));
}

// The logic's er_enabled and the runtime's entrance shuffle through the real MM exports.
void Sources() {
    harnessCaps = WholeCatalog();
    assert(MM_ApplyEntranceExpansionForCombo("{}") == 1);
    assert(!Er() && !Combo_MMEntranceShuffleActive());
    // A private profile keeps the logic and runtime it had.
    assert(MM_ApplyEntranceExpansionForCombo(LegacySeed().dump().c_str()) == 1 && CE::ActiveMmWallmasters().active());
    assert(!Er() && !Combo_MMEntranceShuffleActive());
    // An installed unified layout turns both on, for every thread; a bind clears it.
    assert(MM_ApplyEntranceExpansionForCombo(Seed().dump().c_str()) == 1 && CE::ActiveMmUnified().active());
    assert(Er() && Combo_MMEntranceShuffleActive() == 1 && ErOnAnotherThread());
    harnessRando = false;
    assert(!Combo_MMEntranceShuffleActive());
    harnessRando = true;
    assert(MM_ApplyEntranceExpansionForCombo("{}") == 1);
    assert(!Er() && !Combo_MMEntranceShuffleActive());
    // A refused layout installs nothing.
    harnessCaps = CE::CapabilitySet::AtLeast(CE::Stage::Public);
    loggedErrors = 0;
    assert(MM_ApplyEntranceExpansionForCombo(Seed().dump().c_str()) == 0 && loggedErrors == 1);
    assert(!Er() && !Combo_MMEntranceShuffleActive());
    harnessCaps = WholeCatalog();
}

// The generation's settings: thread-local, set and cleared only by their export.
void GenerationSettings() {
    harnessCaps = WholeCatalog();
    assert(MM_ApplyEntranceExpansionForCombo("{}") == 1);
    assert(MM_SetEntranceGenerationSettingsForCombo(Document(Tranche(), std::string(CE::kMmTrancheCapability)).c_str()));
    assert(Er() && !ErOnAnotherThread());
    assert(!Combo_MMEntranceShuffleActive()); // the runtime follows only an installed layout
    assert(MM_ApplyEntranceExpansionForCombo(LegacySeed().dump().c_str()) == 1 && Er());
    assert(MM_ApplyEntranceExpansionForCombo("{}") == 1 && Er());
    assert(MM_SetEntranceExpansionAssumptionsForCombo(CE::EncodeMmWallmasterAssumptions({}, {}).dump().c_str()) == 1);
    MM_ClearEntranceExpansionAssumptionsForCombo();
    assert(Er());
    // They come before the installed layout's.
    assert(MM_ApplyEntranceExpansionForCombo(Seed().dump().c_str()) == 1);
    CE::Settings voids;
    voids.erOneWays = CE::Mode::OwnGame;
    voids.erOneWaysWaterVoids = true;
    assert(MM_SetEntranceGenerationSettingsForCombo(Document(voids).c_str()) == 1 && !Er());
    assert(MM_SetEntranceGenerationSettingsForCombo(nullptr) == 1 && Er());
    assert(MM_ApplyEntranceExpansionForCombo("{}") == 1 && !Er());
    // Every fixture combination: a settings document is accepted exactly when it places an entrance, and
    // the logic's er_enabled is then the fixture's.
    for (const auto& c : kCombinations) {
        loggedErrors = 0;
        const int accepted = MM_SetEntranceGenerationSettingsForCombo(Document(c.settings).c_str());
        assert(accepted == (c.places ? 1 : 0) && loggedErrors == (c.places ? 0 : 1));
        assert(Er() == (c.places && c.erEnabled));
    }
    // A refusal clears what was set: malformed text, a seed layout instead of a settings document,
    // settings of another pin, settings this build's capabilities do not admit.
    const auto refused = [](const std::string& text) {
        const auto caps = harnessCaps;
        harnessCaps = WholeCatalog();
        assert(MM_SetEntranceGenerationSettingsForCombo(Document(Tranche()).c_str()) == 1 && Er());
        harnessCaps = caps;
        loggedErrors = 0;
        assert(MM_SetEntranceGenerationSettingsForCombo(text.c_str()) == 0 && loggedErrors == 1 && !Er());
    };
    refused("{");
    refused(Seed().at("entranceLayout").dump());
    auto pin = Json::parse(Document(Tranche()));
    pin["upstreamCommit"] = "0000000";
    refused(pin.dump());
    harnessCaps = CE::CapabilitySet::AtLeast(CE::Stage::Public);
    refused(Document(Tranche(), std::string(CE::kMmTrancheCapability)));
    harnessCaps = WholeCatalog();
    harnessCaps.devBuild = false;
    refused(Document(Tranche(), std::string(CE::kMmTrancheCapability))); // a dev-build document
    harnessCaps = WholeCatalog();
    assert(MM_SetEntranceGenerationSettingsForCombo(nullptr) == 1 && !Er());
    // This build: a release build refuses the tranche, a Phase A dev build takes it.
    harnessCaps = CE::ActiveCapabilities();
    const int tranche = MM_SetEntranceGenerationSettingsForCombo(
        Document(Tranche(), std::string(CE::kMmTrancheCapability)).c_str());
#ifdef COMBO_ENTRANCE_DEV
    assert(tranche == 1 && Er());
#else
    assert(tranche == 0 && !Er());
#endif
    assert(MM_SetEntranceGenerationSettingsForCombo(nullptr) == 1 && !Er());
}

int main() {
    assert(SourceForRegion(RR_BENEATH_THE_GRAVEYARD_DAMPE) == "MM_WALLMASTER_DAMPE"); // file-local, unused here
    TruthTables();
    Sources();
    GenerationSettings();
    std::cout << "er_enabled and isEntranceShuffle match the fixture; the MM exports read them from the installed "
                 "unified layout and the generation's settings only\n";
}
'''
for dev in (False, True):
    cpp = work / ('mm_er_enabled_dev.cpp' if dev else 'mm_er_enabled.cpp')
    cpp.write_text(program, encoding='utf-8')
    exe = hostcxx.build(args.compiler, cpp, includes=[root / 'combo', args.json_include],
                        defines=['_CRT_SECURE_NO_WARNINGS'] + (['COMBO_ENTRANCE_DEV=1'] if dev else []),
                        werror=True, msvc=['/bigobj'], gnu=['-Wno-missing-field-initializers', '-pthread'])
    subprocess.run([str(exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)
print(f'er_enabled receipt: {len(rows)} OoTMM lines ({mapped} mapped statements), flags and MM exports pass.')
