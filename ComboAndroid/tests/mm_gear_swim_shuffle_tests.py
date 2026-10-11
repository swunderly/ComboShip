"""MM boots and tunics under 2S2H's Swim shuffle: a Swim-shuffled seed never needs the gear's pinned graphs.

Any MM gear option switches on pinned OoTMM v32.3 graphs (COMBO_WORLDxx_ENABLED, MM_LOGIC_*_PINNED) that read
can_swim as true and count the Zora Mask as swimming (macros_mm.yml:83, :258), while 2S2H's Swim shuffle sends back
every form that swims without Swim, Zora included (Rando/ActorBehavior/Player.cpp RespawnOnWaterTouch). So every
place that gathers options for a seed keeps the gear options at 0 while Swim is shuffled (Combo_MM_GearOptionsAllowed).
This checks:
  - the gate in the option gathering, the settings snapshot and the menu, and that Cross-Games Age stays off;
  - with the options a Swim-shuffled seed can have (gear and age off, Nayru's Love either way), every pinned enable is
    off except the Nayru's Love graphs 0.4.1 already shipped;
  - every CHECK/CONNECTION/EVENT/EXIT of every MM region file that needs Swim in the native graph still needs it (or
    the Iron Boots) with those options, and no connection opens a pinned-only region;
  - the check has teeth: with the gate bypassed, it finds the Zora-without-Swim statements the review named.
Statements are evaluated with the symbolic evaluator of mm_gear_logic_tests.py.
"""
from pathlib import Path
import argparse
import random
import re
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
import mm_gear_logic_tests as T  # noqa: E402

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--reference', type=Path)
a = p.parse_args()
root = a.source.resolve()
failures = []


def check(condition, message):
    if not condition:
        failures.append(message)


def read(rel):
    return (root / rel).read_text(encoding='utf-8')


def body(source, signature):
    """Text of the definition that starts at signature, up to its matching brace (whitespace removed)."""
    m = re.search(re.escape(signature) + r'[^;{]*\{', source)
    if not m:
        return ''
    depth, i = 0, m.end() - 1
    while True:
        depth += (source[i] == '{') - (source[i] == '}')
        i += 1
        if depth == 0:
            return T.squash(source[m.start():i])


GEAR = ('RO_MM_BOOTS_IRON', 'RO_MM_BOOTS_HOVER', 'RO_MM_TUNIC_GORON', 'RO_MM_TUNIC_ZORA')
AGE = ('RO_MM_CROSS_AGE', 'RO_MM_ADULT_MASK', 'RO_MM_STARTING_AGE')
LOVE = 'RO_MM_SPELL_LOVE'
# 0.4.1 already switched these on for Nayru's Love (South.cpp, MmSwampContexts.inc.h, West.cpp Zora Cape peninsula).
NAYRU_PINNED = {'MM_LOGIC_SOUTH_PINNED', 'COMBO_WORLD45_ENABLED', 'COMBO_WORLD54_ENABLED'}
SWIM = ('call', 'CAN_USE_ABILITY', (('text', 'SWIM'),))
IRON = ('call', 'HAS_ITEM', (('text', 'ITEM_COMBO_BOOTS_IRON'),))

# ---- The gate (BenPort.cpp, Rando/Menu.cpp).
benport = read('mm/2s2h/BenPort.cpp')
check(body(benport, 'extern "C" int Combo_MM_GearOptionsAllowed(int swimShuffled)')
      == T.squash('extern "C" int Combo_MM_GearOptionsAllowed(int swimShuffled) '
                  '{ return Combo_MM_GearOptionReady() && !swimShuffled; }'),
      'Combo_MM_GearOptionsAllowed does not leave the gear out while Swim is shuffled')
check('if(!Combo_MM_GearOptionsAllowed(options[RO_SHUFFLE_SWIM]==RO_GENERIC_YES))for(constautoid:kComboMmGearOptions)'
      'options[id]=0;' in body(benport, 'extern "C" void Combo_MM_ClearUnavailableOptions('),
      'generation, the oracle and the pool keep the gear with Swim shuffled')
check('if(!Combo_MM_GearOptionsAllowed(CVarGetInteger(swim.cvar,swim.defaultValue)==RO_GENERIC_YES))'
      'for(constautoid:kComboMmGearOptions)j[Rando::StaticData::Options[id].cvar]=0;'
      in body(benport, 'extern "C" COMBO_EXPORT const char* MM_DumpRandoSettings('),
      'the settings snapshot keeps the gear with Swim shuffled')
check('constauto&swim=Rando::StaticData::Options[RO_SHUFFLE_SWIM];' in body(benport,
      'extern "C" COMBO_EXPORT const char* MM_DumpRandoSettings('), 'the settings snapshot does not read Swim shuffle')
for name in ('Combo_MM_ClearUnavailableOptions(saveInfo.randoSaveOptions);',
             'Combo_MM_ClearUnavailableOptions(gSaveContext.save.shipSaveInfo.rando.randoSaveOptions);'):
    check(name in benport, f'option gathering lost {name}')
menu = read('mm/2s2h/Rando/Menu.cpp')
check('constauto&swim=Rando::StaticData::Options[RO_SHUFFLE_SWIM];if(Combo_MM_GearOptionsAllowed(CVarGetInteger('
      'swim.cvar,swim.defaultValue)==RO_GENERIC_YES)){ItemPoolCheckbox(label,optionId,tooltip,1);return;}'
      in body(menu, 'static void GearPoolCheckbox('), 'the gear rows are not greyed out while Swim is shuffled')
check('Combo_MM_ClearUnavailableOptions(randoSaveInfo.randoSaveOptions);' in menu, 'the pool counts keep unready gear')
# Cross-Games Age turns the same pinned graphs on; it is gathered as 0 while the adult runtime is unfinished.
hidden = re.search(r'kComboHiddenAdultMmOptions\[\]\s*=\s*\{([^}]*)\}', benport)
check(hidden is not None and all(o in hidden.group(1) for o in AGE), 'the age options are not gathered as 0')
check('staticconstexprboolkAdultMmGameplayIntegrationComplete=false;' in T.squash(benport),
      'Adult Link in MM is integrated: Cross-Games Age now switches the pinned graphs on under Swim shuffle too, '
      'so gate it like the gear (Combo_MM_GearOptionsAllowed) before shipping it')

# ---- The graphs a Swim-shuffled seed can use.
logic = read('mm/2s2h/Rando/Logic/Logic.h')
gear_macros = T.defines(logic, lambda name: name.startswith('MM_LOGIC_'))
files = []
for path in sorted((root / 'mm/2s2h/Rando/Logic/Regions').glob('*.cpp')):
    text = path.read_text(encoding='utf-8')
    files.append((path.name, text, dict(gear_macros, **T.defines(text))))


class Valuation:
    """Random unknowns with some options fixed (None: random), the Swim ability given and the Iron Boots unowned."""

    def __init__(self, rng, p, fixed, swim):
        self.rng, self.p, self.fixed, self.swim, self.cache = rng, p, fixed, swim, {}

    def __call__(self, key):
        if key not in self.cache:
            m = re.fullmatch(r'RANDO_SAVE_OPTIONS\[(\w+)\]', key[1]) if key[0] == 'atom' else None
            if key == SWIM:
                self.cache[key] = self.swim
            elif key == IRON:
                self.cache[key] = 0
            elif m and m.group(1) in self.fixed and self.fixed[m.group(1)] is not None:
                self.cache[key] = self.fixed[m.group(1)]
            else:
                self.cache[key] = int(self.rng.random() < self.p)
        return self.cache[key]


def ever(tree, rng, fixed, swim, trials):
    """Whether tree is true on some random assignment."""
    return any(T.evaluate(tree, Valuation(rng, (0.5, 0.85, 0.95)[n % 3], fixed, swim)) for n in range(trials))


NATIVE = {o: 0 for o in GEAR + AGE + (LOVE,)}
SWIM_SHUFFLED = dict(NATIVE, **{LOVE: None})  # what Combo_MM_ClearUnavailableOptions leaves a Swim-shuffled seed
BYPASSED = dict(SWIM_SHUFFLED, **{o: None for o in GEAR})  # the same seed if the gate were missing
rng = random.Random(20261011)

enables = {}
for name, (params, tokens) in gear_macros.items():
    if params is None and name.endswith('_PINNED'):
        enables[name] = gear_macros
for _, text, macros in files:
    for name, (params, tokens) in macros.items():
        if params is None and re.fullmatch(r'COMBO_WORLD\d+_ENABLED', name):
            enables[name] = macros
check({'MM_LOGIC_SOUTH_PINNED', 'MM_LOGIC_STONE_TOWER_PINNED', 'COMBO_WORLD36_ENABLED', 'COMBO_WORLD48_ENABLED'}
      <= set(enables), f'pinned enables not found: {sorted(enables)}')
for name, macros in sorted(enables.items()):
    tree = T.Parser(T.expand([('id', name)], macros)).parse()
    check(not ever(tree, rng, dict(SWIM_SHUFFLED, **{LOVE: 0}), 1, 200),
          f'{name} is on for a Swim-shuffled seed without Nayru\'s Love')
    check(name in NAYRU_PINNED or not ever(tree, rng, dict(SWIM_SHUFFLED, **{LOVE: 1}), 1, 200),
          f'{name} is on for a Swim-shuffled seed with Nayru\'s Love, which 0.4.1 did not do')


def statements(text):
    """[(region, kind, leading args, condition tokens)] of the COMBO_BUILD CHECK/CONNECTION/EVENT/EXIT statements."""
    out, stack, current = [], [], ''
    lines = text.replace('\r\n', '\n').split('\n')
    i = 0
    while i < len(lines):
        raw = T.strip_comments(lines[i])
        s = raw.strip()
        i += 1
        if s.startswith('#'):
            d = s[1:].strip().split()
            if d and d[0] in ('ifdef', 'ifndef'):
                stack.append(None if d[1] != 'COMBO_BUILD' else d[0] == 'ifdef')
            elif d and d[0] == 'if':
                stack.append(None)
            elif d and d[0] == 'else':
                stack[-1] = None if stack[-1] is None else not stack[-1]
            elif d and d[0] == 'endif':
                stack.pop()
            continue
        m = re.search(r'Regions\[(RR_\w+)\]\s*=', s)
        if m:
            current = m.group(1)
        m = re.search(r'\b(CHECK|CONNECTION|EVENT|EXIT)\s*\(', raw)
        if not m or any(x is False for x in stack):
            continue
        text_, depth, line = '', 0, raw[m.start():]
        while True:
            text_ += ' ' + line.strip()
            depth += line.count('(') - line.count(')')
            if depth <= 0:
                break
            line = T.strip_comments(lines[i])
            i += 1
        args, _ = T.split_args(T.tokenize(text_), 1)
        out.append((current, m.group(1), ' '.join(''.join(t[1] for t in arg) for arg in args[:-1]), args[-1]))
    return out


compared, gated, bypassed, pinned_regions = 0, 0, set(), 0
for name, text, macros in files:
    for region, kind, lead, cond in statements(text):
        tree = T.Parser(T.expand(cond, macros)).parse()
        compared += 1
        if kind == 'CONNECTION' and lead.startswith('RR_COMBO_'):
            pinned_regions += 1
            check(not ever(tree, rng, SWIM_SHUFFLED, 1, 300),
                  f'{name} {region} opens the pinned-only region {lead} for a Swim-shuffled seed')
        # Water-gated in the native graph: never true without Swim (and the Iron Boots), true with it.
        if ever(tree, rng, NATIVE, 0, 200) or not ever(tree, rng, NATIVE, 1, 300):
            continue
        gated += 1
        label = f'{name} {region} {kind}({lead})'
        check(not ever(tree, rng, SWIM_SHUFFLED, 0, 600), f'{label} is in logic without Swim for a Swim-shuffled seed')
        if ever(tree, rng, BYPASSED, 0, 600):
            bypassed.add(lead.split(' ')[0] if kind == 'CHECK' else f'{kind}({lead})')

check(compared > 3000 and gated > 100, f'only {compared} statements, {gated} of them water-gated')
check(pinned_regions >= 8, f'only {pinned_regions} connections into pinned-only regions')
named = {'RC_TWIN_ISLANDS_UNDERWATER_CHEST_01', 'RC_TERMINA_FIELD_WATER_CHEST', 'RC_ZORA_CAPE_UNDERWATER_CHEST',
         'RC_PIRATE_FORTRESS_ENTRANCE_CHEST_01', 'EXIT(ENTRANCE(ZORA_HALL,0) ENTRANCE(ZORA_CAPE,1))'}
check(named <= bypassed, f'the check would not see the review\'s cases with the gate bypassed: {sorted(named - bypassed)}')

if failures:
    print('\n'.join(failures))
    sys.exit(1)
print(f'MM gear under Swim shuffle: {compared} statements, {gated} need Swim natively and still do for a Swim-shuffled '
      f'seed; {len(bypassed)} would not without the gate; {len(enables)} pinned enables checked')
