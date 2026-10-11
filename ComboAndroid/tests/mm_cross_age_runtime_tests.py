"""Adult Link in MM with OoT's own adult model (plan F5b, OoTMM v32.3 Cross-Games Age): runtime source checks.

H1: every human slot of MM's player tables (z_player_lib.c: the display-list groups, the first-person tables, the bow
string, the human skeleton and the eye and mouth rows) is classified exactly once: taken by the adult model as plan
F5b 1.3 maps it, or kept as MM's own. Every OoT name the loader asks for is in all twelve retail object_link_boy.xml
files (identical), and every MM name in MM's N64_US XMLs.
"""
from pathlib import Path
import argparse
import hashlib
import re
import sys

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
a = p.parse_args()
root = a.source.resolve()
failures = []


def check(condition, message):
    if not condition:
        failures.append(message)


def read(rel):
    return (root / rel).read_text(encoding='utf-8')


def squash(text):
    return re.sub(r'\s+', '', text)


lib = read('mm/src/code/z_player_lib.c')
tables_src = read('mm/src/code/ComboAdultModelTablesNative.h')
loader = read('mm/2s2h/ComboAdultModelResources.h')
slots_src = read('mm/2s2h/ComboAdultOotSlots.h')

# ---- H1: the human slots of MM's tables.
HUMAN = 4  # PLAYER_FORM_HUMAN
human_slots = set()
for name, size in re.findall(r'^Gfx\* (\w+)\[([^\]]*)\] = \{', lib, re.M):
    if 'PLAYER_FORM_MAX' in size and '2 * PLAYER_FORM_MAX' in size:
        human_slots |= {(name, 2 * HUMAN), (name, 2 * HUMAN + 1)}
    elif size.strip() == 'PLAYER_FORM_MAX':
        human_slots.add((name, HUMAN))
for name in ('gPlayerHandHoldingShields', 'gPlayerShields', 'gPlayerSheathedSwords', 'gPlayerSwordSheaths',
             'D_801C018C'):
    body = re.search(r'Gfx\* ' + name + r'\[[^\]]*\] = \{(.*?)\};', lib, re.S).group(1)
    entries = [e for e in re.sub(r'//[^\n]*', '', body).replace('\n', ' ').split(',') if e.strip()]
    human_slots |= {(name, i) for i in range(len(entries))}
human_slots |= {('D_801C0D94', 0), ('gPlayerSkeletons', HUMAN)}
human_slots |= {('sPlayerEyesTextures', i) for i in range(8)} | {('sPlayerMouthTextures', i) for i in range(4)}
check(('gPlayerWaistDLs', 8) in human_slots and ('sPlayerFirstPersonRightHandHookshotDLs', 4) in human_slots and
      len(human_slots) == 73, f'z_player_lib.c human slots: found {len(human_slots)}, expected 73')

# Plan F5b 1.3: what each slot the adult model takes draws.
EXPECTED = {
    ('gPlayerWaistDLs', 8): 'WAIST_NEAR', ('gPlayerWaistDLs', 9): 'WAIST_FAR',
    ('gPlayerLeftHandOpenDLs', 8): 'LEFT_HAND_OPEN_NEAR', ('gPlayerLeftHandOpenDLs', 9): 'LEFT_HAND_OPEN_FAR',
    ('gPlayerLeftHandClosedDLs', 8): 'LEFT_HAND_CLOSED_NEAR', ('gPlayerLeftHandClosedDLs', 9): 'LEFT_HAND_CLOSED_FAR',
    ('gPlayerRightHandOpenDLs', 8): 'RIGHT_HAND_OPEN_NEAR', ('gPlayerRightHandOpenDLs', 9): 'RIGHT_HAND_OPEN_FAR',
    ('gPlayerRightHandClosedDLs', 8): 'RIGHT_HAND_CLOSED_NEAR',
    ('gPlayerRightHandClosedDLs', 9): 'RIGHT_HAND_CLOSED_FAR',
    ('gPlayerRightHandInstrumentDLs', 8): 'RIGHT_HAND_INSTRUMENT_NEAR',
    ('gPlayerRightHandInstrumentDLs', 9): 'RIGHT_HAND_INSTRUMENT_FAR',
    ('gPlayerLeftHandBottleDLs', 8): 'LEFT_HAND_BOTTLE', ('gPlayerLeftHandBottleDLs', 9): 'LEFT_HAND_BOTTLE',
    ('sPlayerFirstPersonLeftHandDLs', 4): 'LEFT_HAND_CLOSED_NEAR',
    ('sPlayerFirstPersonRightShoulderDLs', 4): 'RIGHT_SHOULDER_NEAR',
    ('sPlayerFirstPersonLeftForearmDLs', 4): 'SLOT_EMPTY',
    ('D_801C018C', 0): 'KOKIRI_SWORD_NEAR', ('D_801C018C', 1): 'KOKIRI_SWORD_FAR',
    ('D_801C018C', 2): 'RAZOR_SWORD_NEAR', ('D_801C018C', 3): 'RAZOR_SWORD_FAR',
    ('D_801C018C', 4): 'GILDED_SWORD_NEAR', ('D_801C018C', 5): 'GILDED_SWORD_FAR',
    ('gPlayerLeftHandOneHandSwordDLs', 8): 'KOKIRI_SWORD_NEAR', ('gPlayerLeftHandOneHandSwordDLs', 9): 'KOKIRI_SWORD_FAR',
    ('gPlayerLeftHandTwoHandSwordDLs', 8): 'GREAT_FAIRYS_SWORD_NEAR',
    ('gPlayerLeftHandTwoHandSwordDLs', 9): 'GREAT_FAIRYS_SWORD_FAR',
    ('gPlayerHandHoldingShields', 0): 'HEROS_SHIELD_NEAR', ('gPlayerHandHoldingShields', 1): 'HEROS_SHIELD_FAR',
    ('gPlayerHandHoldingShields', 2): 'MIRROR_SHIELD_NEAR', ('gPlayerHandHoldingShields', 3): 'MIRROR_SHIELD_FAR',
    ('gPlayerRightHandBowDLs', 8): 'BOW_NEAR', ('gPlayerRightHandBowDLs', 9): 'BOW_FAR',
    ('gPlayerRightHandHookshotDLs', 8): 'HOOKSHOT_NEAR', ('gPlayerRightHandHookshotDLs', 9): 'HOOKSHOT_FAR',
    ('sPlayerFirstPersonRightHandDLs', 4): 'FIRST_PERSON_BOW',
    ('sPlayerFirstPersonRightHandHookshotDLs', 4): 'FIRST_PERSON_HOOKSHOT',
}
# MM's own, as OoTMM's custom model has its own here (plan F5b 1.3, lead decisions D1 and D2).
KEPT = {('gPlayerSheath12DLs', 8), ('gPlayerSheath12DLs', 9), ('gPlayerSheath13DLs', 8), ('gPlayerSheath13DLs', 9),
        ('gPlayerSheath14DLs', 8), ('gPlayerSheath14DLs', 9), ('D_801C0D94', 0)}
KEPT |= {('gPlayerShields', i) for i in range(4)} | {('gPlayerSheathedSwords', i) for i in range(6)}
KEPT |= {('gPlayerSwordSheaths', i) for i in range(6)}
SPECIAL = {('gPlayerSkeletons', HUMAN)} | {('sPlayerEyesTextures', i) for i in range(8)}
SPECIAL |= {('sPlayerMouthTextures', i) for i in range(4)}

recipes = {}
body = re.search(r'sComboAdultModelRecipes\[\] = \{(.*?)\n\};', tables_src, re.S).group(1)
for dest, count, slots in re.findall(r'\{\s*([\w\s\*\+]+?),\s*(\d+),\s*\{([^}]*)\}\s*\}', body):
    m = re.fullmatch(r'(\w+)(?:\s*\+\s*(2\s*\*\s*PLAYER_FORM_HUMAN|PLAYER_FORM_HUMAN))?', dest.strip())
    check(m is not None, f'recipe destination {dest!r}')
    if not m:
        continue
    start = {None: 0, 'PLAYER_FORM_HUMAN': HUMAN}.get(m.group(2) and squash(m.group(2)), 2 * HUMAN)
    names = [s.strip().replace('COMBO_ADULT_', '') for s in slots.split(',') if s.strip()]
    check(len(names) == int(count), f'recipe {dest}: {count} slots declared, {len(names)} given')
    for i, slot in enumerate(names):
        key = (m.group(1), start + i)
        check(key not in recipes, f'{key} is taken twice')
        recipes[key] = slot
check(recipes == EXPECTED, 'the adult tables differ from plan F5b 1.3: ' +
      str({k: (recipes.get(k), EXPECTED.get(k)) for k in set(recipes) | set(EXPECTED) if recipes.get(k) != EXPECTED.get(k)}))
check(not (set(recipes) & KEPT) and not (set(recipes) & SPECIAL), 'a slot is both taken and kept')
check(set(recipes) | KEPT | SPECIAL == human_slots,
      f'unclassified human slots: {sorted(human_slots - set(recipes) - KEPT - SPECIAL)}; '
      f'not MM slots: {sorted((set(recipes) | KEPT | SPECIAL) - human_slots)}')
apply = squash(tables_src[tables_src.index('s32 Combo_MM_ApplyAdultModelTables(s32 adult) {'):])
check('gPlayerSkeletons[PLAYER_FORM_HUMAN]=skeleton;' in apply, 'the human skeleton is not taken')
check('&sPlayerEyesTextures[PLAYER_FORM_HUMAN][index]' in squash(tables_src) and
      '&sPlayerMouthTextures[PLAYER_FORM_HUMAN][index-COMBO_ADULT_EYE_TEXTURES]' in squash(tables_src),
      'the face rows are not the human rows')
for weapon, length in (('KOKIRI', '4000.0f'), ('RAZOR', '4000.0f'), ('GILDED', '5500.0f'), ('TWO_HANDED', '5500.0f')):
    check(f'sMeleeWeaponLengths[PLAYER_MELEEWEAPON_SWORD_{weapon}]={length};' in apply,
          f'adult {weapon} length is not OoTMM\'s {length} (play.c:713-717)')
check('D_801C0D94' not in tables_src and 'gPlayerSheath' not in tables_src and 'gPlayerShields' not in tables_src,
      'a slot plan F5b keeps as MM\'s own is changed')

# The loader's lists, in slot order, and its composites (plan F5b 1.3).
enum = re.search(r'typedef enum ComboAdultOotSlot \{(.*?)\} ComboAdultOotSlot;', slots_src, re.S).group(1)
order = [e.split('=')[0].strip().replace('COMBO_ADULT_', '') for e in re.sub(r'//[^\n]*', '', enum).split(',')
         if e.strip()]
body_slots = order[:order.index('BODY_MAX')]
composite_slots = order[order.index('BODY_MAX') + 1:order.index('SLOT_MAX')]
check(composite_slots[0] == 'KOKIRI_SWORD_NEAR' and 'COMBO_ADULT_KOKIRI_SWORD_NEAR = COMBO_ADULT_BODY_MAX' in enum,
      'composite slots do not start at COMBO_ADULT_BODY_MAX')


def strings(name):
    block = re.search(re.escape(name) + r'\[[^\]]*\] = \{(.*?)\};', loader, re.S).group(1)
    return re.findall(r'"([^"]+)"', block)


body_dls = strings('kAdultBodyDLs')
OOT_BODY = {
    'WAIST_NEAR': 'gLinkAdultWaistNearDL', 'WAIST_FAR': 'gLinkAdultWaistFarDL',
    'LEFT_HAND_OPEN_NEAR': 'gLinkAdultLeftHandNearDL', 'LEFT_HAND_OPEN_FAR': 'gLinkAdultLeftHandFarDL',
    'LEFT_HAND_CLOSED_NEAR': 'gLinkAdultLeftHandClosedNearDL', 'LEFT_HAND_CLOSED_FAR': 'gLinkAdultLeftHandClosedFarDL',
    'RIGHT_HAND_OPEN_NEAR': 'gLinkAdultRightHandNearDL', 'RIGHT_HAND_OPEN_FAR': 'gLinkAdultRightHandFarDL',
    'RIGHT_HAND_CLOSED_NEAR': 'gLinkAdultRightHandClosedNearDL',
    'RIGHT_HAND_CLOSED_FAR': 'gLinkAdultRightHandClosedFarDL',
    'RIGHT_HAND_INSTRUMENT_NEAR': 'gLinkAdultRightHandHoldingOotNearDL',
    'RIGHT_HAND_INSTRUMENT_FAR': 'gLinkAdultRightHandHoldingOotFarDL',
    'LEFT_HAND_BOTTLE': 'gLinkAdultLeftHandOutNearDL', 'RIGHT_SHOULDER_NEAR': 'gLinkAdultRightShoulderNearDL',
}
check([d.rsplit('/', 1)[-1] for d in body_dls] == [OOT_BODY[s] for s in body_slots] and
      all(d.startswith('objects/object_link_boy/') for d in body_dls), 'OoT body lists are not in slot order')
items = strings('kAdultMmItemDLs')
item_enum = [e.strip() for e in re.search(r'enum AdultMmItem \{(.*?)\};', loader, re.S).group(1).split(',')
             if e.strip() and '=' not in e and 'MAX' not in e]
item_name = dict(zip(item_enum, [i.rsplit('/', 1)[-1] for i in items]))
COMPOSITES = {
    'KOKIRI_SWORD': ('LEFT_HAND_CLOSED', ['gKokiriSwordDL']), 'RAZOR_SWORD': ('LEFT_HAND_CLOSED', ['gRazorSwordDL']),
    'GILDED_SWORD': ('LEFT_HAND_CLOSED', ['gLinkHumanGildedSwordHandleDL', 'gLinkHumanGildedSwordBladeDL']),
    'GREAT_FAIRYS_SWORD': ('LEFT_HAND_CLOSED', ['gLinkHumanGreatFairysSwordDL']),
    'HEROS_SHIELD': ('RIGHT_HAND_CLOSED', ['gLinkHumanHerosShieldDL']),
    'MIRROR_SHIELD': ('RIGHT_HAND_CLOSED', ['gLinkHumanMirrorShieldDL']),
    'BOW': ('RIGHT_HAND_CLOSED', ['gLinkHumanBowDL']), 'HOOKSHOT': ('RIGHT_HAND_CLOSED', ['gLinkHumanHookshotDL']),
}
recipes_src = re.search(r'kAdultComposites\[[^\]]*\] = \{(.*?)\n\};', loader, re.S).group(1)
composites = re.findall(r'\{\s*COMBO_ADULT_(\w+),\s*\{\s*(\w+),\s*(\w+)\s*\}\s*\}', recipes_src)
check(len(composites) == len(composite_slots), 'one composite per composite slot')
for slot, (hand, *its) in zip(composite_slots, composites):
    if slot.startswith('FIRST_PERSON_'):
        want_hand, want_items = 'RIGHT_HAND_CLOSED_NEAR', COMPOSITES[slot[len('FIRST_PERSON_'):]][1]
    else:
        base, lod = slot.rsplit('_', 1)
        want_hand, want_items = COMPOSITES[base][0] + '_' + lod, COMPOSITES[base][1]
    got = [item_name[i] for i in its if i != 'ADULT_MM_ITEM_NONE']
    check(hand == want_hand and got == want_items, f'composite {slot}: {hand} + {got}, expected {want_hand} + {want_items}')

# Names: OoT's in all twelve retail object_link_boy.xml, MM's in MM's N64_US XMLs.
retail = [x for x in sorted((root / 'soh/assets/xml').glob('*/objects/object_link_boy.xml')) if not x.parent.parent.name.endswith('_D')]
check(len(retail) == 12 and len({hashlib.md5(x.read_bytes()).hexdigest() for x in retail}) == 1,
      'the twelve retail object_link_boy.xml files are not identical')
oot_xml = retail[0].read_text(encoding='utf-8') if retail else ''
oot_names = body_dls + strings('kAdultFaceTextures') + ['objects/object_link_boy/gLinkAdultSkel']
check('kAdultSkeleton[] = "objects/object_link_boy/gLinkAdultSkel";' in loader, 'adult skeleton name')
for name in oot_names:
    check(name.startswith('objects/object_link_boy/') and f'Name="{name.rsplit("/", 1)[-1]}"' in oot_xml,
          f'{name} is not in OoT\'s object_link_boy.xml')
check(re.search(r'<Skeleton Name="gLinkAdultSkel" Type="Flex" LimbType="LOD"', oot_xml), 'OoT adult skeleton is not Flex/LOD')
mm_xml = root / 'mm/assets/xml/N64_US/objects'
check('kFierceDeitySkeleton[] = "objects/object_link_boy/gLinkFierceDeitySkel";' in loader, 'Fierce Deity skeleton name')
for name in items + ['objects/object_link_boy/gLinkFierceDeitySkel']:
    folder, leaf = name.split('/')[1:]
    check(f'Name="{leaf}"' in (mm_xml / f'{folder}.xml').read_text(encoding='utf-8'), f'{name} is not in MM\'s XMLs')

if failures:
    print('\n'.join('FAIL: ' + f for f in failures))
    sys.exit(1)
print(f'PASS: {len(human_slots)} human slots classified; {len(oot_names)} OoT and {len(items) + 1} MM names present')
