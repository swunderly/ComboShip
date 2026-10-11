"""MM boots and tunics (plan F2 B6/B7): OoTMM gear terms in MM logic, the receipt and the no-regression check.

The fixture fixtures/ootmm-32.3-mm-gear-logic.json is the receipt of the gear logic audit: every OoTMM v32.3 line
that needed a gear term (has_iron_boots, has_hover_boots, the tunics, underwater_walking, can_swim_or_sink or a gear
trick) and the fork statements that now carry it, each with its text before and after the change. This checks:
  - the Logic.h macros, the combat header and the trick table against OoTMM's macros and tricks.ts text;
  - every receipt row: each of its fork statements is in its region with the gear term, and a gear term added beside
    a native rule (#ifdef COMBO_BUILD) keeps the native statement in the #else branch;
  - no regression: each statement evaluates exactly as before when the four MM gear options are off and the new
    tricks are off (they only add routes), and the gear-only tricks change nothing while the gear options are off;
  - each row's statements do change once the gear is on, so no row is a no-op.
Statements are evaluated symbolically: the gear macros and each file's own #defines are expanded, every other macro
or call is an independent unknown, and random assignments of those unknowns are compared. When the OoTMM checkout
is available (--reference, or references/ootmm-stable-32.3 beside the worktree) each row's OoTMM line is checked
too.
"""
from pathlib import Path
import argparse
import json
import random
import re
import sys

GEAR_OPTIONS = ('RO_MM_BOOTS_IRON', 'RO_MM_BOOTS_HOVER', 'RO_MM_TUNIC_GORON', 'RO_MM_TUNIC_ZORA')
TOKEN = re.compile(r'\s*(?:(?P<num>0[xX][0-9A-Fa-f]+|\d+)[uUlL]*|(?P<str>"(?:[^"\\]|\\.)*")|'
                   r'(?P<id>[A-Za-z_]\w*(?:(?:::|\.|->)[A-Za-z_]\w*)*)|'
                   r'(?P<op>&&|\|\||==|!=|>=|<=|<<|>>|##|[-+*/%<>!?:()\[\],&|^~{};#=.]))')


def strip_comments(text):
    text = re.sub(r'/\*.*?\*/', ' ', text, flags=re.S)
    return re.sub(r'//[^\n]*', '', text)


def squash(text):
    """Text without comments, whitespace or macro line continuations (clang-format moves all three)."""
    return re.sub(r'\s+', '', re.sub(r'\\\s*(?=\n|$)', ' ', strip_comments(text)))


def tokenize(text):
    text, pos, out = strip_comments(text), 0, []
    while pos < len(text):
        m = TOKEN.match(text, pos)
        if not m:
            if not text[pos:].strip():
                break
            raise ValueError(f'cannot read {text[pos:pos + 40]!r}')
        pos = m.end()
        out.append((m.lastgroup, m.group(m.lastgroup)))
    return out


def defines(text, keep=lambda name: True):
    """#define table {name: (params or None, body tokens)} of the COMBO_BUILD branches of a source file."""
    table, stack = {}, []
    lines = text.replace('\r\n', '\n').split('\n')
    i = 0
    while i < len(lines):
        line = lines[i]
        while line.endswith('\\') and i + 1 < len(lines):
            i += 1
            line = line[:-1] + ' ' + lines[i]
        i += 1
        s = line.strip()
        if not s.startswith('#'):
            continue
        directive = s[1:].strip()
        word = directive.split()[0] if directive else ''
        if word in ('ifdef', 'ifndef'):
            name = directive.split()[1]
            stack.append(None if name != 'COMBO_BUILD' else word == 'ifdef')
        elif word == 'if':
            stack.append(None)
        elif word == 'elif':
            stack[-1] = None
        elif word == 'else':
            stack[-1] = None if stack[-1] is None else not stack[-1]
        elif word == 'endif':
            stack.pop()
        elif word == 'define' and all(x is not False for x in stack):
            m = re.match(r'define\s+(\w+)(\(([^)]*)\))?(.*)$', directive)
            if m and keep(m.group(1)) and m.group(1) not in table:
                params = [p.strip() for p in m.group(3).split(',')] if m.group(2) else None
                table[m.group(1)] = (params, tokenize(m.group(4)))
    return table


def split_args(tokens, open_index):
    """Arguments of the call whose '(' is tokens[open_index]; returns (args, index after ')')."""
    depth, args, current, i = 0, [], [], open_index
    while True:
        t = tokens[i]
        if t[1] in ('(', '[', '{'):
            depth += 1
            if depth > 1:
                current.append(t)
        elif t[1] in (')', ']', '}'):
            depth -= 1
            if depth == 0:
                args.append(current)
                return [a for a in args if a or len(args) > 1], i + 1
            current.append(t)
        elif t[1] == ',' and depth == 1:
            args.append(current)
            current = []
        else:
            current.append(t)
        i += 1


def expand(tokens, macros, hide=frozenset()):
    out, i = [], 0
    while i < len(tokens):
        kind, value = tokens[i]
        if kind == 'id' and value in macros and value not in hide:
            params, body = macros[value]
            if params is None:
                out += expand(body, macros, hide | {value})
                i += 1
                continue
            if i + 1 < len(tokens) and tokens[i + 1][1] == '(':
                args, j = split_args(tokens, i + 1)
                args = [expand(a, macros, hide) for a in args] + [[]] * (len(params) - len(args))
                if any(t[1] in ('#', '##') for t in body):
                    raise ValueError(f'{value} pastes tokens')
                substituted = []
                for t in body:
                    substituted += args[params.index(t[1])] if t[0] == 'id' and t[1] in params else [t]
                out += expand(substituted, macros, hide | {value})
                i = j
                continue
        out.append(tokens[i])
        i += 1
    return out


class Parser:
    """C expression parser: ternary, logical, comparison and arithmetic operators, calls and subscripts."""
    LEVELS = [('||',), ('&&',), ('|',), ('^',), ('&',), ('==', '!='), ('<', '>', '<=', '>='), ('<<', '>>'),
              ('+', '-'), ('*', '/', '%')]

    def __init__(self, tokens):
        self.t, self.i = tokens, 0

    def peek(self):
        return self.t[self.i][1] if self.i < len(self.t) else None

    def take(self, value=None):
        tok = self.t[self.i]
        assert value is None or tok[1] == value, (value, tok, self.t)
        self.i += 1
        return tok

    def parse(self):
        node = self.ternary()
        assert self.i == len(self.t), ('trailing tokens', self.t[self.i:])
        return node

    def ternary(self):
        cond = self.binary(0)
        if self.peek() == '?':
            self.take('?')
            a = self.ternary()
            self.take(':')
            return ('tern', cond, a, self.ternary())
        return cond

    def binary(self, level):
        if level == len(self.LEVELS):
            return self.unary()
        node = self.binary(level + 1)
        while self.peek() in self.LEVELS[level]:
            op = self.take()[1]
            node = ('op', op, node, self.binary(level + 1))
        return node

    def unary(self):
        if self.peek() in ('!', '-', '~'):
            op = self.take()[1]
            return ('unary', op, self.unary())
        return self.postfix()

    def postfix(self):
        kind, value = self.take()
        if value == '(':
            node = self.ternary()
            self.take(')')
            return node
        if kind == 'num':
            node = ('lit', int(value, 0))
        elif kind == 'str':
            node = ('str', value)
        elif value in ('true', 'false'):
            node = ('lit', value == 'true')
        else:
            node = ('id', value)
        while self.peek() in ('(', '[', '.'):
            if self.peek() == '.':  # member of a call or subscript result: one unknown
                self.take('.')
                node = ('id', show(node) + '.' + self.take()[1])
            elif self.take()[1] == '(':
                args = []
                while self.peek() != ')':
                    args.append(self.ternary())
                    if self.peek() == ',':
                        self.take(',')
                self.take(')')
                node = ('call', show(node), args)
            else:
                index = self.ternary()
                self.take(']')
                node = ('index', show(node), index)
        return node


def show(node):
    kind = node[0]
    if kind in ('lit', 'str', 'id'):
        return str(node[1])
    if kind == 'call':
        return f'{node[1]}({",".join(show(a) for a in node[2])})'
    if kind == 'index':
        return f'{node[1]}[{show(node[2])}]'
    if kind == 'unary':
        return f'{node[1]}({show(node[2])})'
    if kind == 'op':
        return f'({show(node[2])}{node[1]}{show(node[3])})'
    return f'({show(node[1])}?{show(node[2])}:{show(node[3])})'


def constant(node):
    return node[0] == 'lit' or (node[0] in ('op', 'unary') and all(constant(x) for x in node[2:]))


def evaluate(node, value):
    """Truth value of node; value(key) gives the unknowns (identifiers, calls, subscripts, comparisons)."""
    kind = node[0]
    if kind == 'lit':
        return node[1]
    if kind == 'str':
        return node[1]
    if kind in ('id', 'index'):
        return value(('atom', show(node)))
    if kind == 'call':
        return value(('call', node[1], tuple(argument(a, value) for a in node[2])))
    if kind == 'tern':
        return evaluate(node[2], value) if evaluate(node[1], value) else evaluate(node[3], value)
    if kind == 'unary':
        x = evaluate(node[2], value)
        return (not x) if node[1] == '!' else (-x if node[1] == '-' else ~x)
    op, a, b = node[1], node[2], node[3]
    if op == '&&':
        return bool(evaluate(a, value)) and bool(evaluate(b, value))
    if op == '||':
        return bool(evaluate(a, value)) or bool(evaluate(b, value))
    if not (constant(a) and constant(b)):  # e.g. KEY_COUNT(...) >= 3: one unknown
        return value(('atom', show(node)))
    x, y = evaluate(a, value), evaluate(b, value)
    return {'|': x | y, '^': x ^ y, '&': x & y, '==': x == y, '!=': x != y, '<': x < y, '>': x > y, '<=': x <= y,
            '>=': x >= y, '<<': x << y, '>>': x >> y, '+': x + y, '-': x - y, '*': x * y}[op]


def argument(node, value):
    """A call argument: a name or literal stands for itself, a boolean expression for its value."""
    if node[0] in ('id', 'lit', 'str'):
        return ('text', show(node))
    return ('value', bool(evaluate(node, value)))


def condition(statement):
    """The condition of a CHECK/CONNECTION/EVENT/EXIT statement (its last argument), or a bare expression."""
    tokens = tokenize(statement)
    while tokens and tokens[-1][1] in (',', ';'):
        tokens = tokens[:-1]
    if len(tokens) > 1 and tokens[0][0] == 'id' and tokens[0][1] in ('CHECK', 'CONNECTION', 'EVENT', 'EXIT') \
            and tokens[1][1] == '(':
        args, end = split_args(tokens, 1)
        assert end == len(tokens), statement
        return args[-1]
    return tokens


def trick_name(key):
    """The trick a COMBO_MM_TRICK("Name") unknown reads, or None."""
    return key[2][0][1].strip('"') if key[0] == 'call' and key[1] == 'COMBO_MM_TRICK' and key[2] else None


class Valuation:
    """A random assignment of the unknowns, with the given gear options and tricks pinned to 0."""

    def __init__(self, rng, p, off_options, off_tricks):
        self.rng, self.p, self.cache = rng, p, {}
        self.off_options, self.off_tricks = set(off_options), set(off_tricks)

    def __call__(self, key):
        if key not in self.cache:
            m = re.fullmatch(r'RANDO_SAVE_OPTIONS\[(\w+)\]', key[1]) if key[0] == 'atom' else None
            off = (m and m.group(1) in self.off_options) or trick_name(key) in self.off_tricks
            self.cache[key] = 0 if off else int(self.rng.random() < self.p)
        return self.cache[key]


def compare(before, after, trials, rng, off_options, off_tricks, hidden):
    """First assignment on which before and after differ ((tokens, macros) pairs), or None. hidden holds the
    tricks that read 0 before and after the change (IsHiddenMmTrick)."""
    trees = [Parser(expand(tokens, macros)).parse() if tokens else ('lit', False) for tokens, macros in (before, after)]
    for n in range(trials):
        valuation = Valuation(rng, (0.5, 0.85, 0.95)[n % 3], off_options, off_tricks)
        results = []
        for tree, hide in zip(trees, hidden):
            results.append(bool(evaluate(tree, lambda key: 0 if trick_name(key) in hide else valuation(key))))
        if results[0] != results[1]:
            return valuation.cache
    return None


def region(source, rid):
    if not rid:
        return source
    start = source.find('Regions[' + rid + ']')
    if start < 0:
        return ''
    end = source.find('Regions[', start + 8)
    return source[start:] if end < 0 else source[start:end]


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--source', type=Path, required=True)
    p.add_argument('--reference', type=Path)
    a = p.parse_args()
    root = a.source.resolve()
    fixture = json.loads((Path(__file__).resolve().parent / 'fixtures/ootmm-32.3-mm-gear-logic.json')
                         .read_text(encoding='utf-8'))
    failures = []

    def check(ok, message):
        if not ok:
            failures.append(message)

    def read(rel):
        return (root / rel).read_text(encoding='utf-8')

    # ---- Macros, combat header and trick table.
    for entry in fixture['macros']:
        check(squash(entry['text']) in squash(read(entry['file'])), f'{entry["file"]} lacks {entry["text"]}')
    tricks = squash(read('combo/rando/MmTricks.h'))
    for t in fixture['tricks']:
        m = re.search(r'\{(\d+),"' + re.escape(t['fork']) + r'",((?:"(?:[^"\\]|\\.)*")+),((?:"(?:[^"\\]|\\.)*")+),',
                      tricks)
        check(m is not None, f'no trick row for {t["fork"]}')
        if not m:
            continue
        name = ''.join(re.findall(r'"((?:[^"\\]|\\.)*)"', m.group(2))).replace('\\"', '"')
        tip = ''.join(re.findall(r'"((?:[^"\\]|\\.)*)"', m.group(3))).replace('\\"', '"')
        check(int(m.group(1)) == t['id'], f'{t["fork"]} id {m.group(1)}')
        check(name == squash(t['name']) and tip == squash(t['tooltip']), f'{t["fork"]} text differs from tricks.ts')
        row = tricks[m.start():tricks.index('}', m.end())]
        check(not row.endswith(',false'), f'{t["fork"]} is hidden')

    # ---- Receipt: every row's statements carry its gear term; an added term keeps the native statement in #else.
    logic_h = read('mm/2s2h/Rando/Logic/Logic.h')
    gear_macros = defines(logic_h, lambda name: name.startswith('MM_LOGIC_'))
    # The age macros as MmAgeLogic computes them (combined_logic_tests.cpp), with the Cross-Games Age option as an
    # unknown that compare() can pin.
    for name, expansion in (('MM_LOGIC_IS_ADULT', '(RANDO_SAVE_OPTIONS[RO_MM_CROSS_AGE] && MM_LOGIC_AGE_IS_ADULT)'),
                            ('MM_LOGIC_IS_CHILD', '(!RANDO_SAVE_OPTIONS[RO_MM_CROSS_AGE] || !MM_LOGIC_AGE_IS_ADULT)'),
                            ('MM_LOGIC_IS_TALL', '(HAS_ITEM(ITEM_MASK_ZORA) || MM_LOGIC_IS_ADULT)'),
                            ('MM_LOGIC_IS_SHORT', '(HAS_ITEM(ITEM_MASK_DEKU) || MM_LOGIC_IS_CHILD)')):
        gear_macros[name] = (None, tokenize(expansion))
    files = {}

    def file_macros(name, before):
        if name not in files:
            text = read(name)
            files[name] = (text, dict(gear_macros, **defines(text)))
        macros = dict(files[name][1])
        if before:
            for define, body in fixture['changedDefines'].get(name, {}).items():
                macros[define] = (macros[define][0], tokenize(body))
        return macros

    sites = {s['id']: s for s in fixture['sites']}
    for s in fixture['sites']:
        file_macros(s['file'], False)
        text = squash(region(files[s['file']][0], s['region']))
        after = squash(s['after'])
        at = text.find(after)
        check(at >= 0, f'{s["id"]}: {s["file"]} {s["region"] or "(file)"} lacks {s["after"][:100]}')
        if at < 0 or s['kind'] not in ('ifdef', 'insert'):
            continue
        opened = text.rfind('#ifdefCOMBO_BUILD', 0, at)
        closed = text.find('#endif', at)
        check(0 <= opened and text.find('#endif', opened) == closed, f'{s["id"]}: not under #ifdef COMBO_BUILD')
        if s['kind'] == 'ifdef':
            other = text.find('#else', at)
            check(0 <= other < closed and squash(s['before']) in text[other:closed],
                  f'{s["id"]}: the #else branch lost {s["before"][:100]}')
    gear_word = re.compile(r'MM_LOGIC_HAS_IRON_BOOTS|MM_LOGIC_HAS_HOVER_BOOTS|MM_LOGIC_HAS_ZORA_TUNIC\b|'
                           r'MM_LOGIC_UNDERWATER_WALKING\b|MM_LOGIC_UNDERWATER_WALKING_STRICT|'
                           r'MM_LOGIC_CAN_SWIM_OR_SINK|COMBO_MM_TRICK\("\w+"\)')
    for row in fixture['rows']:
        check(row['sites'], f'{row["ootmm"]}: no fork statement')
        for term in row['terms']:
            carriers = [squash(sites[i]['after']) for i in row['sites']]
            for define in row.get('defines', []):
                file = sites[row['sites'][0]]['file']
                params, body = file_macros(file, False)[define]
                carriers += [''.join(t[1] for t in body)] if any(define in c for c in carriers) else []
            word = re.compile(re.escape(squash(term)) + r'(?!\w)')
            check(gear_word.fullmatch(term) is not None and any(word.search(c) for c in carriers),
                  f'{row["ootmm"]}: its fork statements lack {term}')

    # ---- No regression: gear options off (and the new tricks off) -> every statement as before.
    rng = random.Random(20261011)
    hidden = (set(fixture['hiddenBefore']), set(fixture['hiddenAfter']))
    compared = 0
    for s in fixture['sites']:
        if s['kind'] in ('define', 'text'):
            continue
        before = (condition(s['before']) if s['before'] else [], file_macros(s['file'], True))
        after = (condition(s['after']), file_macros(s['file'], False))
        # A statement that later gained a Cross-Games Age term (mm_cross_age_tests.py checks that term on its own) is
        # compared with that option off as well.
        off = GEAR_OPTIONS + (('RO_MM_CROSS_AGE',) if s.get('crossAge') else ())
        diff = compare(before, after, 600, rng, off, fixture['newTricks'], hidden)
        check(diff is None, f'{s["id"]}: differs with the gear options and new tricks off')
        # The gear-only tricks (all but the non-gear ones) change nothing while the gear options are off.
        diff = compare(before, after, 600, rng, off, fixture['nonGearTricks'], hidden)
        check(diff is None, f'{s["id"]}: a gear trick changes it with the gear options off')
        compared += 1
    # ... and with the gear on each row's statements do change.
    for row in fixture['rows']:
        changed = False
        for i in row['sites']:
            s = sites[i]
            if s['kind'] in ('define', 'text'):
                continue
            before = (condition(s['before']) if s['before'] else [], file_macros(s['file'], True))
            after = (condition(s['after']), file_macros(s['file'], False))
            if compare(before, after, 3000, rng, (), (), hidden) is not None:
                changed = True
                break
        check(changed or row.get('combat'), f'{row["ootmm"]}: no statement changes with the gear on')

    # ---- The receipt against OoTMM v32.3 itself.
    reference = a.reference
    if reference is None:
        for candidate in (root.parent.parent / 'references/ootmm-stable-32.3',
                          root.parent / 'references/ootmm-stable-32.3'):
            if (candidate / 'packages/core/src/settings/tricks.ts').exists():
                reference = candidate
                break
    if reference is not None:
        tricks_ts = (reference / 'packages/core/src/settings/tricks.ts').read_text(encoding='utf-8')
        for t in fixture['tricks']:
            block = tricks_ts[tricks_ts.index(f'  {t["key"]}: {{'):]
            block = block[:block.index('  },')]
            name = re.search(r'name: ("|\')(.*?)\1,', block).group(2)
            tip = re.search(r'tooltip: ("|\')(.*?)\1,', block).group(2)
            check((name, tip) == (t['name'], t['tooltip']), f'fixture {t["key"]} differs from tricks.ts')
        for entry in fixture['macros'] + fixture['rows']:
            rel, line = entry['ootmm'].split('#')[0].rsplit(':', 1)
            lines = (reference / rel).read_text(encoding='utf-8').splitlines()
            for expected in entry['expects']:
                check(expected in lines[int(line) - 1], f'{entry["ootmm"]} does not contain {expected!r}')
        print(f'OoTMM reference checked: {reference}')
    else:
        print('OoTMM reference not found; checked the production sources against the fixture only')

    if failures:
        print('\n'.join(failures))
        sys.exit(1)
    print(f'MM gear logic: {len(fixture["rows"])} receipt rows, {len(fixture["sites"])} fork statements '
          f'({compared} evaluated unchanged with the gear off), {len(fixture["macros"])} macros and '
          f'{len(fixture["tricks"])} tricks match OoTMM v32.3')


if __name__ == '__main__':
    main()
