"""Item-level strictness of switching a Nayru's Love (MM) seed onto the pinned OoTMM v32.3 graphs (plan F2 rule B).

A seed with Nayru's Love (MM) and no MM age or gear option uses the pinned South graph (Deku Palace, Southern
Swamp, Woodfall) and the pinned Ancient Castle of Ikana graph instead of the native 2S2H ones. A check-level
"superset with all items" comparison cannot see what that costs a player with fewer items (critique, plan 2 item
5). This test reads the production sources and compares, for every combination of the relevant items:

1. each switched expression (COMBO_WORLD45_SELECT in South.cpp, the Woodfall rows, IKANA_ADULT_GEAR_MODE in
   IkanaCastle.cpp): where the pinned expression is false although the native one is true;
2. the whole Southern Swamp: reachability of every swamp check, event and exit in the native regions versus the
   pinned swamp regions of MmSwampContexts.inc.h.

Every place where the pinned graph needs more than the native one must be listed below with the OoTMM v32.3 line
that requires it; anything else fails. MM age and gear options stay off (child Link, no boots or tunics).
"""
from pathlib import Path
import argparse
import itertools
import re
import sys

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
a = p.parse_args()
root = a.source.resolve()
south = (root / 'mm/2s2h/Rando/Logic/Regions/South.cpp').read_text(encoding='utf-8')
ikana = (root / 'mm/2s2h/Rando/Logic/Regions/IkanaCastle.cpp').read_text(encoding='utf-8')
contexts = (root / 'mm/2s2h/Rando/Logic/MmSwampContexts.inc.h').read_text(encoding='utf-8')
routes = (root / 'combo/rando/MmAdultWorldRoutes.h').read_text(encoding='utf-8')
logic_h = (root / 'mm/2s2h/Rando/Logic/Logic.h').read_text(encoding='utf-8')
failures = []


def squash(text):
    return re.sub(r'\s+', '', text)


def balanced(text, start):
    """Text inside the parentheses that open at text[start] == '('."""
    depth, at = 0, start
    while True:
        depth += (text[at] == '(') - (text[at] == ')')
        at += 1
        if depth == 0:
            return text[start + 1:at - 1], at


def split_top(text):
    parts, depth, cur = [], 0, ''
    for ch in text:
        depth += (ch in '([{') - (ch in ')]}')
        if ch == ',' and depth == 0:
            parts.append(cur)
            cur = ''
        else:
            cur += ch
    parts.append(cur)
    return [x.strip() for x in parts]


# ---------------------------------------------------------------- C++ boolean expressions -> Python
TOKEN = re.compile(r'\s*(&&|\|\||==|!=|>=|<=|::|[!?:(),\[\]<>]|[A-Za-z_]\w*|\d+)')


def tokenize(text):
    text = re.sub(r'//[^\n]*', '', text)
    out, at = [], 0
    while at < len(text):
        m = TOKEN.match(text, at)
        if not m:
            if text[at:].strip() == '':
                break
            raise ValueError('cannot tokenize ' + text[at:at + 40])
        out.append(m.group(1))
        at = m.end()
    return out


class Parser:
    def __init__(self, text):
        self.t, self.i = tokenize(text), 0

    def peek(self):
        return self.t[self.i] if self.i < len(self.t) else None

    def take(self, want=None):
        tok = self.t[self.i]
        assert want is None or tok == want, (want, tok, self.t)
        self.i += 1
        return tok

    def parse(self):
        node = self.expr()
        assert self.peek() is None, self.t[self.i:]
        return node

    def expr(self):
        cond = self.or_()
        if self.peek() == '?':
            self.take()
            yes = self.expr()
            self.take(':')
            return ('tern', cond, yes, self.expr())
        return cond

    def or_(self):
        node = self.and_()
        while self.peek() == '||':
            self.take()
            node = ('or', node, self.and_())
        return node

    def and_(self):
        node = self.unary()
        while self.peek() == '&&':
            self.take()
            node = ('and', node, self.unary())
        return node

    def unary(self):
        if self.peek() == '!':
            self.take()
            return ('not', self.unary())
        node = self.primary()
        if self.peek() in ('>=', '<=', '==', '!=', '<', '>'):
            op = self.take()
            return ('atom', text_of(node) + op + text_of(self.primary()))
        return node

    def primary(self):
        tok = self.take()
        if tok == '(':
            node = self.expr()
            self.take(')')
            return node
        if tok in ('true', 'false'):
            return ('const', tok == 'true')
        if tok.isdigit():
            return ('name', tok)
        name = tok
        while self.peek() == '::':
            self.take()
            name += '::' + self.take()
        if self.peek() == '(':
            self.take()
            args = []
            if self.peek() != ')':
                args.append(self.expr())
                while self.peek() == ',':
                    self.take()
                    args.append(self.expr())
            self.take(')')
            return ('call', name, args)
        if self.peek() == '[':
            self.take()
            index = self.take()
            self.take(']')
            return ('name', f'{name}[{index}]')
        return ('name', name)


def text_of(node):
    kind = node[0]
    if kind == 'name' or kind == 'atom':
        return node[1]
    if kind == 'const':
        return 'true' if node[1] else 'false'
    if kind == 'call':
        return node[1] + '(' + ','.join(text_of(x) for x in node[2]) + ')'
    raise ValueError(node)


# Child Link, no MM boots or tunics; masks owned count as the forms (generation).
ATOMS = {
    'CAN_BE_DEKU': "s['deku']", 'HAS_ITEM(ITEM_MASK_DEKU)': "s['deku']",
    'CAN_BE_ZORA': "s['zora']", 'HAS_ITEM(ITEM_MASK_ZORA)': "s['zora']", 'MM_LOGIC_IS_TALL': "s['zora']",
    'MM_LOGIC_UNDERWATER_WALKING_STRICT': "s['zora']",
    'CAN_BE_GORON': "s['goron']", 'HAS_ITEM(ITEM_MASK_GORON)': "s['goron']",
    'CAN_USE_ABILITY(SWIM)': "s['swim']",
    'NativeMmSwampCleared()': "s['cleared']", 'NativeMmSwampPoisoned()': "(not s['cleared'])",
    'CAN_USE_NAYRUS_LOVE': "s['nayru']", 'CAN_USE_DINS_FIRE': "s['din']",
    'CAN_USE_BOW': "s['bow']", 'HAS_ITEM(ITEM_HOOKSHOT)': "s['hookshot']", 'HAS_MAGIC': "s['magic']",
    'CAN_USE_DEITY_OUTSIDE_BOSS': "s['deity']", 'CAN_USE_STICKS': "s['sticks']",
    'CAN_USE_MAGIC_ARROW(FIRE)': "s['fire']",
    'CAN_LIGHT_TORCH_NEAR_ANOTHER': "(s['sticks'] or s['fire'] or s['din'])",
    'CAN_TRAVERSE_WAIST_DEEP_WATER': "(s['swim'] or s['deku'] or s['zora'] or s['goron'] or s['deity'])",
    'CAN_USE_PROJECTILE': "(s['bow'] or s['hookshot'] or (s['deku'] and s['magic']) or s['zora'])",
    'CAN_USE_DAY2_RAIN_BEAN': "s['beans']", 'CAN_SKIP_PALACE_BEANS': "s['beanSkip']",
    'CanKillEnemy(ACTOR_EN_DEKUNUTS)': "((s['deku'] or s['scrubWeapon']) and (not s['souls'] or s['scrubSoul']))",
    'HaveEnemySoul(ACTOR_EN_DEKUNUTS)': "s['scrubSoul']",
    'RANDO_SAVE_OPTIONS[RO_SHUFFLE_ENEMY_SOULS]': "s['souls']", 'RANDO_SAVE_OPTIONS[RO_MM_COMPLETE_SOULS]': 'False',
    'CAN_NAVIGATE_HIDDEN': "s['lens']", 'CAN_USE_SWORD': "s['sword']", 'CAN_USE_BOMBS': "s['bombs']",
    'MM_LOGIC_IS_CHILD': 'True', 'MM_LOGIC_IS_SHORT': 'True', 'MM_LOGIC_IS_ADULT': 'False',
    'MM_LOGIC_HAS_HOVER_BOOTS': 'False', 'MM_LOGIC_HAS_IRON_BOOTS': 'False',
    'RANDO_EVENTS[RE_SAVED_KOUME]': "s['koume']", 'HAS_ITEM(ITEM_PICTOGRAPH_BOX)': "s['pictobox']",
    'CAN_ACCESS(PICTOGRAPH_SWAMP_GENERIC)': "e.get('RE_ACCESS_PICTOGRAPH_SWAMP_GENERIC', False)",
    'Rando::HaveExpandedSoul(Rando::ExpandedSoul::NPC_TOURIST_CENTER)': 'True',
    'MmExtraIndoorsShuffled()': 'False', 'CAN_PAY(1)': 'True',
    'TypedMmSwampActive()': 'pinned', 'NativeMmErContextsEnabled()': 'False',
}
HELPERS = {}
for m in re.finditer(r'constexpr bool (\w+)\(([^)]*)\)\s*\{\s*return (.*?);\s*\}', routes, re.S):
    params = [x.split()[-1] for x in m.group(2).split(',')]
    HELPERS['ComboRando::MmAdultWorldRoutes::' + m.group(1)] = (params, Parser(m.group(3)).parse())
HELPERS['Rando::ExpandedBoatAuthorization'] = (['savedKoume', 'swampPhoto'], Parser(
    'savedKoume || (Rando::HaveExpandedSoul(Rando::ExpandedSoul::NPC_TOURIST_CENTER) && swampPhoto)').parse())
for name in ('MmSwampCanSwim', 'MmSwampCrossPoison', 'MmSwampSwimOrTallOrGoron', 'MmSwampKillOctoClose'):
    body = re.search(r'inline bool ' + name + r'\(\) \{\s*return (.*?);\s*\}', contexts, re.S).group(1)
    HELPERS[name] = ([], Parser(body).parse())
# The native swamp helpers (South.cpp) as written there; the test fails if their source changes.
assert squash('''if (RANDO_EVENTS[RE_SOUTHERN_SWAMP_KILL_OCTOROK] && (CAN_BE_DEKU || CAN_BE_ZORA || CAN_BE_GORON)) {
        return true;''') in squash(south)
assert squash('''if (NativeMmSwampCleared() && CAN_TRAVERSE_WAIST_DEEP_WATER) {
        return true;
    }
    return false;''') in squash(south)
assert squash('''if (RANDO_EVENTS[RE_SOUTHERN_SWAMP_RIDE_BOAT]) {
        return true;
    }
    return CanGetPastBigOctoWithoutBoat();''') in squash(south)
HELPERS['CanGetPastBigOctoWithoutBoat'] = ([], Parser(
    '(RANDO_EVENTS[RE_SOUTHERN_SWAMP_KILL_OCTOROK] && (CAN_BE_DEKU || CAN_BE_ZORA || CAN_BE_GORON)) || '
    '(NativeMmSwampCleared() && CAN_TRAVERSE_WAIST_DEEP_WATER)').parse())
HELPERS['CanGetPastBigOcto'] = ([], Parser('RANDO_EVENTS[RE_SOUTHERN_SWAMP_RIDE_BOAT] || CanGetPastBigOctoWithoutBoat()').parse())
lenient_atoms = set()


def py(node, lenient=False, env=None):
    kind = node[0]
    if kind == 'const':
        return 'True' if node[1] else 'False'
    if kind == 'not':
        return '(not ' + py(node[1], lenient, env) + ')'
    if kind in ('and', 'or'):
        return '(' + py(node[1], lenient, env) + f' {kind} ' + py(node[2], lenient, env) + ')'
    if kind == 'tern':
        return '(' + py(node[2], lenient, env) + ' if ' + py(node[1], lenient, env) + ' else ' + py(node[3], lenient, env) + ')'
    if kind == 'call' and node[1] in HELPERS:
        params, body = HELPERS[node[1]]
        inner = dict(zip(params, ['(' + py(x, lenient, env) + ')' for x in node[2]]))
        return '(' + py(body, lenient, inner) + ')'
    text = text_of(node)
    if env and text in env:
        return env[text]
    if text.startswith('RANDO_EVENTS[RE_') and text not in ATOMS:
        return f"e.get('{text[13:-1]}', False)"
    if text in ATOMS:
        return ATOMS[text]
    if lenient:
        lenient_atoms.add(text)
        return 'True'
    raise KeyError(text)


def compile_expr(text, lenient=False):
    return eval('lambda s, e, pinned: ' + py(Parser(text).parse(), lenient))


VARS = ['deku', 'zora', 'goron', 'swim', 'cleared', 'nayru', 'din', 'bow', 'hookshot', 'magic', 'deity', 'sticks',
        'fire', 'koume', 'pictobox', 'beans', 'beanSkip', 'scrubWeapon', 'scrubSoul', 'souls', 'lens', 'sword', 'bombs']


def used_vars(fn_text):
    return [v for v in VARS if f"s['{v}']" in fn_text]


# ---------------------------------------------------------------- 1. switched expressions
pairs = []
for m in re.finditer(r'COMBO_WORLD45_SELECT\(', south):
    if south.rfind('#define', 0, m.start()) > south.rfind('\n', 0, m.start()):
        continue
    inner, _ = balanced(south, m.end() - 1)
    legacy, pinned = split_top(inner)
    line = south[south.rfind('\n', 0, m.start()) + 1:m.start()]
    owner = re.search(r'(CHECK|CONNECTION|EXIT)\((\w+(?:\(\w+, \d+\))?)', line)
    pairs.append(('South', owner.group(2), legacy, pinned))
for m in re.finditer(r'(CHECK|CONNECTION|EXIT)\((\w+(?:\(\w+, \d+\))?),\s*(?:ENTRANCE\(\w+, \d+\),\s*)?COMBO_WORLD45_ENABLED \? ', south):
    rest = south[m.end():south.index('\n', m.end())]
    rest = rest[:rest.rindex(')')]
    pinned, legacy = split_top(rest.replace(' : ', ',', 1)) if False else (None, None)
    depth, cut = 0, None
    for i, ch in enumerate(rest):
        depth += (ch == '(') - (ch == ')')
        if ch == ':' and depth == 0 and rest[i - 1] == ' ':
            cut = i
            break
    pairs.append(('South', m.group(2), rest[cut + 1:].strip(), rest[:cut].strip()))
m = re.search(r'CHECK\(RC_WOODFALL_NEAR_OWL_CHEST, COMBO_WORLD45_ENABLED && (.*)\), // woodfall.yml:15', south)
pairs.append(('South', 'RC_WOODFALL_NEAR_OWL_CHEST (in RR_WOODFALL)', 'false', m.group(1)))
for m in re.finditer(r'(CHECK|CONNECTION)\((\w+), IKANA_ADULT_GEAR_MODE \? ', ikana):
    rest = ikana[m.end():ikana.index('\n', m.end())]
    rest = rest[:rest.rindex(')')]
    depth, cut = 0, None
    for i, ch in enumerate(rest):
        depth += (ch == '(') - (ch == ')')
        if ch == ':' and depth == 0 and rest[i - 1] == ' ':
            cut = i
            break
    region = ikana[:m.start()].rsplit('Regions[', 1)[1].split(']', 1)[0]
    pairs.append(('Ikana', f'{m.group(2)} from {region}', rest[cut + 1:].strip(), rest[:cut].strip()))

# Every pinned expression that needs more than its native counterpart for some items, with OoTMM's reason.
# can_swim is the Swim ability or the Zora Mask (no scales); the native graph also lets Goron wade there.
EXPECTED_STRICTER = {
    'ENTRANCE(DEKU_PALACE, 4)': 'deku_palace.yml:159 Near Deku Shrine -> Front: ((cleared || nayru) && can_swim) || Deku || hover',
    'ENTRANCE(DEKU_SHRINE, 0)': 'deku_palace.yml:7 Front -> Near Deku Shrine: is_swamp_cleared && can_swim',
    'RR_DEKU_PALACE_INSIDE_UPPER_BEAN_SIDE': 'deku_palace.yml:9 Front -> Upper: ((cleared || nayru) && can_swim) || Deku',
    'RC_DEKU_PALACE_POT_01': 'deku_palace.yml Upper pots need has(MASK_DEKU); their native region needs it too',
    'RC_DEKU_PALACE_POT_02': 'deku_palace.yml Upper pots need has(MASK_DEKU); their native region needs it too',
    'RR_WOODFALL': 'woodfall.yml:35 Woodfall Shrine -> Woodfall: has(MASK_DEKU) && soul_deku_scrub',
    'RC_ANCIENT_CASTLE_OF_IKANA_LEFT_SECOND_ROOM_POT_01': 'ancient_castle_of_ikana.yml:42: Deku Mask or hover boots, not the Bow',
    'RC_ANCIENT_CASTLE_OF_IKANA_LEFT_SECOND_ROOM_POT_02': 'ancient_castle_of_ikana.yml:43',
    'RC_ANCIENT_CASTLE_OF_IKANA_LEFT_SECOND_ROOM_POT_03': 'ancient_castle_of_ikana.yml:44',
    'RC_ANCIENT_CASTLE_OF_IKANA_LEFT_SECOND_ROOM_POT_04': 'ancient_castle_of_ikana.yml:45',
}
stricter, looser = {}, {}
for area, owner, legacy, pinned in pairs:
    lf = 'lambda s, e, pinned: ' + py(Parser(legacy).parse())
    pf = 'lambda s, e, pinned: ' + py(Parser(pinned).parse())
    names = sorted(set(used_vars(lf)) | set(used_vars(pf)))
    lfn, pfn = eval(lf), eval(pf)
    for values in itertools.product((False, True), repeat=len(names)):
        s = dict(zip(names, values))
        lv, pv = lfn(s, {}, False), pfn(s, {}, False)
        key = owner if owner.startswith(('RC_', 'ENTRANCE')) or ' from ' in owner else owner
        if lv and not pv:
            stricter.setdefault(key, {k: v for k, v in s.items() if v})
        if pv and not lv:
            looser.setdefault(key, {k: v for k, v in s.items() if v})
for key, example in sorted(stricter.items()):
    print(f'  switched: pinned {key} needs more, e.g. with {sorted(example)}')
found = {k.split(' from ')[0] for k in stricter}
for key in sorted(found - set(EXPECTED_STRICTER)):
    failures.append(f'pinned {key} needs more than native without a listed OoTMM reason')
for key in sorted(set(EXPECTED_STRICTER) - found):
    failures.append(f'{key} is listed as stricter but is not')
print(f'{len(pairs)} switched expressions: {len(stricter)} stricter, {len(looser)} looser')

# ---------------------------------------------------------------- 2. the Southern Swamp
OLD = ['RR_SOUTHERN_SWAMP_NORTH', 'RR_SOUTHERN_SWAMP_NEAR_FLOWERS', 'RR_SOUTHERN_SWAMP_NEAR_WOODS',
       'RR_SOUTHERN_SWAMP_SOUTH', 'RR_SOUTHERN_SWAMP_SOUTH_UPPER']


def region_text(source, rid):
    start = source.index('Regions[' + rid + ']')
    end = source.find('Regions[', start + 8)
    return source[start:end if end >= 0 else len(source)]


def entries(text, kind):
    out, at = [], 0
    while True:
        at = text.find(kind + '(', at)
        if at < 0:
            return out
        if at > 0 and (text[at - 1].isalnum() or text[at - 1] == '_'):
            at += 1
            continue
        inner, at = balanced(text, at + len(kind))
        out.append(split_top(inner))


graph = {}  # region -> {'conn': [(target, fn)], 'check': [(rc, fn)], 'exit': [(name, fn)], 'event': [(re, fn)]}


def node(rid):
    return graph.setdefault(rid, {'conn': [], 'check': [], 'exit': [], 'event': []})


for rid in OLD + ['RR_TOURIST_INFORMATION']:
    text = region_text(south, rid)
    n = node(rid)
    for target, cond in entries(text, 'CONNECTION'):
        fn = compile_expr(cond)
        if rid in OLD and target in OLD:  # MmSwampContexts: old -> old edges are native-only
            n['conn'].append((target, (lambda f: lambda s, e, pinned: (not pinned) and f(s, e, pinned))(fn)))
        else:
            n['conn'].append((target, fn))
    for args in entries(text, 'CHECK'):
        n['check'].append((args[0], compile_expr(args[1], lenient=True)))
    for args in entries(text, 'EXIT'):
        target = args[0]
        fn = compile_expr(args[2], lenient=True)
        if target == 'ENTRANCE(TOURIST_INFORMATION, 0)':
            n['conn'].append(('RR_TOURIST_INFORMATION', fn))
        elif target == 'ENTRANCE(SOUTHERN_SWAMP_POISONED, 1)':
            n['conn'].append(('RR_SOUTHERN_SWAMP_NORTH', fn))
        else:
            n['exit'].append((target, fn))
    for args in entries(text, 'EVENT'):
        n['event'].append((args[0], compile_expr(args[1], lenient=True)))

# MmSwampContexts.inc.h, as active for the pinned graph.
for m in re.finditer(r'edge\((RR_\w+),\s*(RR_\w+),\s*\[\]\s*\{\s*return (.*?);\s*\}\);', contexts, re.S):
    fn = compile_expr(m.group(3))
    node(m.group(1))['conn'].append((m.group(2), (lambda f: lambda s, e, pinned: pinned and f(s, e, pinned))(fn)))
for m in re.finditer(r'Regions\.at\((RR_\w+)\)\s*\.events\.push_back\(\s*EVENT\(', contexts):
    inner, _ = balanced(contexts, m.end() - 1)
    re_id, cond = split_top(inner)
    node(m.group(1))['event'].append((re_id, compile_expr(cond)))
for m in re.finditer(r'for \(auto source : \{([^}]*)\}\)\s*Regions\.at\(source\)\.events\.push_back\(\s*EVENT\(', contexts):
    inner, _ = balanced(contexts, m.end() - 1)
    re_id, cond = split_top(inner)
    for rid in [x.strip() for x in m.group(1).split(',')]:
        node(rid)['event'].append((re_id, compile_expr(cond)))
for m in re.finditer(r'Regions\.at\((RR_\w+)\)\s*\.checks\.insert\(CHECK\(', contexts):
    inner, _ = balanced(contexts, m.end() - 1)
    rc, cond = split_top(inner)
    node(m.group(1))['check'].append((rc, compile_expr(cond, lenient=True)))
# Procedural overrides, modelled here; each must still be in the source.
for statement in ['hive.first = [oldHive] { return !TypedMmSwampActive() && oldHive(); };',
                  'slot.first = [legacy] { return TypedMmSwampActive() ? NativeMmSwampPoisoned() : legacy(); };',
                  'moveExit(RR_SOUTHERN_SWAMP_SOUTH, RR_MM_GROTTO_SWAMP_LEDGE, ENTRANCE(GROTTOS, 21), [] { return true; });',
                  'moveExit(RR_SOUTHERN_SWAMP_SOUTH, RR_MM_SWAMP_NEAR_SPIDER, ENTRANCE(SWAMP_SPIDER_HOUSE, 0), [] { return CAN_USE_STICKS || CAN_USE_BOW; });',
                  'moveExit(RR_SOUTHERN_SWAMP_SOUTH_UPPER, RR_MM_SWAMP_CANOPY_FRONT, ENTRANCE(DEKU_PALACE, 5), [] { return true; });',
                  'woodfall.condition = [prior] { return TypedMmSwampActive() || prior(); };',
                  'frog.first = [originalFrog] { return !TypedMmSwampActive() && originalFrog(); };']:
    assert squash(statement) in squash(contexts), statement


def gate(n, kind, name, wrap):
    n[kind] = [(x, wrap(f) if x == name else f) for x, f in n[kind]]


native_only = lambda f: lambda s, e, pinned: (not pinned) and f(s, e, pinned)
gate(graph['RR_SOUTHERN_SWAMP_NORTH'], 'check', 'RC_SOUTHERN_SWAMP_POISON_BEEHIVE', native_only)
gate(graph['RR_SOUTHERN_SWAMP_NEAR_FLOWERS'], 'check', 'RC_SOUTHERN_SWAMP_FROG', native_only)
for rc in ('RC_SOUTHERN_SWAMP_FREESTANDING_RUPEE_01', 'RC_SOUTHERN_SWAMP_FREESTANDING_RUPEE_02'):
    gate(graph['RR_SOUTHERN_SWAMP_NEAR_FLOWERS'], 'check', rc,
         lambda f: lambda s, e, pinned: (not s['cleared']) if pinned else f(s, e, pinned))
for source, target, entrance, cond in [('RR_SOUTHERN_SWAMP_SOUTH', 'RR_MM_GROTTO_SWAMP_LEDGE', 'ENTRANCE(GROTTOS, 21)', 'true'),
                                       ('RR_SOUTHERN_SWAMP_SOUTH', 'RR_MM_SWAMP_NEAR_SPIDER', 'ENTRANCE(SWAMP_SPIDER_HOUSE, 0)', 'CAN_USE_STICKS || CAN_USE_BOW'),
                                       ('RR_SOUTHERN_SWAMP_SOUTH_UPPER', 'RR_MM_SWAMP_CANOPY_FRONT', 'ENTRANCE(DEKU_PALACE, 5)', 'true')]:
    gate(graph[source], 'exit', entrance, native_only)
    fn = compile_expr(cond)
    node(target)['exit'].append((entrance, (lambda f: lambda s, e, pinned: pinned and f(s, e, pinned))(fn)))
gate(graph['RR_SOUTHERN_SWAMP_SOUTH_UPPER'], 'exit', 'ENTRANCE(WOODFALL, 0)',
     lambda f: lambda s, e, pinned: pinned or f(s, e, pinned))


def reach(s, pinned):
    regions, events, found = {'RR_SOUTHERN_SWAMP_NORTH'}, {}, set()
    changed = True
    while changed:
        changed = False
        for rid in list(regions):
            n = graph[rid]
            for re_id, fn in n['event']:
                if not events.get(re_id) and fn(s, events, pinned):
                    events[re_id] = True
                    changed = True
            for target, fn in n['conn']:
                if target not in regions and fn(s, events, pinned):
                    regions.add(target)
                    changed = True
    for rid in regions:
        n = graph[rid]
        found |= {rc for rc, fn in n['check'] if fn(s, events, pinned)}
        found |= {x for x, fn in n['exit'] if fn(s, events, pinned)}
    found |= {re_id for re_id in events if events[re_id] and re_id.startswith(('RE_SOUTHERN_SWAMP_KILL', 'RE_MM_SWAMP'))}
    return found


SWAMP_VARS = ['deku', 'zora', 'goron', 'swim', 'cleared', 'nayru', 'din', 'bow', 'hookshot', 'magic', 'deity',
              'sticks', 'fire', 'koume', 'pictobox']
# Locations the pinned swamp offers with fewer items than the native one, with the OoTMM v32.3 rule responsible.
EXPECTED_SWAMP = {
    'ENTRANCE(DEKU_PALACE, 0)': 'southern_swamp.yml:232: the boat ride needs is_swamp_poisoned',
    'ENTRANCE(DEKU_PALACE, 5)': 'southern_swamp.yml:298: Canopy Back -> Canopy Front needs short_hook_anywhere; the treetop '
                                'is still reached from the palace itself',
    'ENTRANCE(SWAMP_SPIDER_HOUSE, 0)': 'southern_swamp.yml:178: has_sticks || has_arrows (Din\'s Fire does not count)',
    'RC_SOUTHERN_SWAMP_FREESTANDING_RUPEE_01': 'southern_swamp.yml:113: is_swamp_poisoned',
    'RC_SOUTHERN_SWAMP_FREESTANDING_RUPEE_02': 'southern_swamp.yml:114: is_swamp_poisoned',
    # can_swim is the Swim ability or the Zora Mask; without Swim (swim shuffle) the native graph let Goron wade.
    'RC_SOUTHERN_SWAMP_FROG': 'southern_swamp.yml:65, :97: Swamp Near Frog is reached with can_swim || is_tall (or Deku)',
    'RC_SOUTHERN_SWAMP_POISON_BEEHIVE': 'southern_swamp.yml:58, :99: breaking the hive needs the Zora Mask or Swamp Near Frog',
}
# The native rule that lets Fierce Deity wade (logic rule "Fierce Deity's Mask anywhere") has no v32.3 swamp
# counterpart: OoTMM's Fierce Deity Anywhere in MM has "No logical applications" (data.ts:1597-1601).
tightened, widened, deity_only = {}, {}, set()
cases = 0
for values in itertools.product((False, True), repeat=len(SWAMP_VARS)):
    s = dict(zip(SWAMP_VARS, values))
    # Fire Arrows need the Bow and magic; the spells need magic.
    if (s['fire'] and not (s['bow'] and s['magic'])) or ((s['din'] or s['nayru']) and not s['magic']):
        continue
    native, pinned = reach(s, False), reach(s, True)
    cases += 1
    owned = {k for k, v in s.items() if v}
    for loc in native - pinned:
        if s['deity']:
            deity_only.add(loc)
            continue
        if loc not in tightened or len(owned) < len(tightened[loc]):
            tightened[loc] = owned
    for loc in pinned - native:
        if loc not in widened or len(owned) < len(widened[loc]):
            widened[loc] = owned
deity_only -= set(tightened)
for loc, example in sorted(tightened.items()):
    print(f'  swamp: pinned needs more for {loc}, e.g. with {sorted(example)}')
for loc in sorted(set(tightened) - set(EXPECTED_SWAMP)):
    failures.append(f'pinned swamp tightens {loc} (e.g. with {sorted(tightened[loc])}) without a listed OoTMM reason')
for loc in sorted(set(EXPECTED_SWAMP) - set(tightened)):
    failures.append(f'{loc} is listed as tightened but is not')
print(f'  swamp: {len(deity_only)} locations need more only where the native graph used Fierce Deity')
for loc, example in sorted(widened.items()):
    print(f'  swamp: pinned opens {loc} earlier, e.g. with {sorted(example)}')
# Nayru's Love itself: after a Big Octo falls, Swim plus Nayru's Love crosses the poisoned water (cross_poison_swamp).
nayru_only = dict.fromkeys(SWAMP_VARS, False)
nayru_only.update(swim=True, nayru=True, magic=True)
check_routes = reach(nayru_only, True)
if 'ENTRANCE(DEKU_PALACE, 0)' not in check_routes:
    failures.append('Swim and Nayru\'s Love do not reach Deku Palace through the poisoned swamp')
if 'ENTRANCE(DEKU_PALACE, 0)' in reach(dict(nayru_only, nayru=False), True):
    failures.append('the pinned swamp reaches Deku Palace with Swim alone')
if lenient_atoms:
    print('  check-only atoms treated as satisfied in both graphs: ' + ', '.join(sorted(lenient_atoms)))
print(f'Southern Swamp: {cases} item combinations, {len(tightened)} tightened and {len(widened)} widened locations')
if failures:
    print('\n'.join(failures))
    sys.exit(1)
print('Rule B item-level strictness: every tightening is an OoTMM v32.3 rule')
