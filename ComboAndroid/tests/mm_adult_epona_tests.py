"""Adult Link's Epona in MM from OoT's own object_horse (feature plan F5b step 5b-7, host test H5).

The fixture fixtures/ootmm-32.3-mm-adult-epona.json lists OoTMM v32.3's 53 adultEponaPatch pairs (src/mm/actors.c:901-955)
and its horse object swap (En/En_Horse.c:4-24), each with the typed change the port makes for it. Checks that no pair is
left unmapped and that each mapped change is in the production sources:
- the 38 lui/addiu immediates are the 19 numbered ENHORSE_CS_ANIM references in En_Horse's cutscene cue functions: each
  pair's overlay address lies in the function it names (MM US overlay at 0x8087B730, anchored by D_80889204), the
  Animation_GetLastFrame reference is numbered before Animation_Change's, and sAdultEponaCsAnims picks the OoT animation
  at the pair's object_horse offset;
- the animation table, skeleton and eye pairs name the OoT object_horse resources the loader binds, in En_Horse's order;
- the scale pair keeps the child scale's low half, and the rider-position branch skips the child vertex for adult Epona.
Also checks the resource-safety rules of plan section 1.8 in ComboAdultEponaResources.h. When the OoTMM v32.3 checkout is
available (--reference, or references/ootmm-stable-32.3 beside the worktree), the fixture is compared with it.
"""
from pathlib import Path
import argparse
import hashlib
import json
import re
import struct
import sys
import xml.etree.ElementTree as ElementTree

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


def squash(text):
    return re.sub(r'\s+', '', text)


def body(source, signature):
    """Text of the definition that starts at signature, up to its matching brace."""
    m = re.search(re.escape(signature) + r'[^;{]*\{', source)
    at = m.start()
    depth, i = 0, m.end() - 1
    while True:
        depth += (source[i] == '{') - (source[i] == '}')
        i += 1
        if depth == 0:
            return source[at:i]


def array(source, name):
    m = re.search(re.escape(name) + r'\[\]\s*=\s*\{(.*?)\};', source, re.S)
    return m.group(1) if m else ''


fixture = json.loads((Path(__file__).resolve().parent / 'fixtures/ootmm-32.3-mm-adult-epona.json').read_text(encoding='utf-8'))
pairs = fixture['pairs']
horse = read('mm/src/overlays/actors/ovl_En_Horse/z_en_horse.c')
tables = read('mm/src/overlays/actors/ovl_En_Horse/ComboAdultEponaTablesNative.h')
resources = read('mm/2s2h/ComboAdultEponaResources.h')
xml_rel = fixture['object']['xml']
xml = ElementTree.fromstring(read(xml_rel))
by_offset = {int(e.get('Offset'), 16): (e.tag, e.get('Name')) for e in xml.iter() if e.get('Offset')}

# ---- The fixture: 53 pairs, every one mapped to a known typed change.
KNOWN = {'cutscene_animation', 'animation_table', 'skeleton', 'eye', 'scale', 'rider_branch'}
check(len(pairs) == 53, f'the fixture lists {len(pairs)} pairs, not 53')
check(len({e['offset'] for e in pairs}) == len(pairs), 'the fixture repeats an offset')
unmapped = [e['offset'] for e in pairs if e.get('change') not in KNOWN]
check(not unmapped, f'unmapped OoTMM Epona pairs: {unmapped}')
counts = {c: sum(e['change'] == c for e in pairs) for c in KNOWN}
check(counts == {'cutscene_animation': 38, 'animation_table': 9, 'skeleton': 1, 'eye': 3, 'scale': 1, 'rider_branch': 1},
      f'unexpected pair kinds: {counts}')
check(len(fixture['calls']) == 2 and all(c['change'] == 'object' for c in fixture['calls']), 'object swap calls missing')

# Every retail OoT XML describes the same object_horse, so the names hold for any extracted OoT archive.
xmls = sorted((root / 'soh/assets/xml').glob('*/objects/object_horse.xml'))
check(len(xmls) >= 14, f'only {len(xmls)} OoT object_horse.xml files')
check(len({hashlib.md5(x.read_bytes()).hexdigest() for x in xmls}) == 1, 'the OoT object_horse.xml files differ')

# ---- Overlay addresses: the eye pair anchors the base at D_80889204 (decomp name = its VRAM).
base = int(fixture['overlay']['vram'], 16)
eye0 = next(int(e['offset'], 16) for e in pairs if e['change'] == 'eye' and e['index'] == 0)
check(base + eye0 - 2 == 0x80889204 and 'static TexturePtr D_80889204[]' in horse,
      'the overlay base does not put the first eye pair on D_80889204')
functions = [(m.start(), m.group(1)) for m in
             re.finditer(r'^[A-Za-z_][\w\s\*]*?\b(\w+)\([^;{]*\)\s*\{', horse, re.M)]
names = [n for _, n in functions]


def address(name):
    m = re.fullmatch(r'func_([0-9A-F]{8})', name)
    return int(m.group(1), 16) if m else None


def bounds(name):
    """VRAM range between the address-named functions around name in source (= overlay) order."""
    i = names.index(name)
    lo = next(address(n) for n in reversed(names[:i + 1]) if address(n) is not None)
    hi = next((address(n) for n in names[i + 1:] if address(n) is not None), None)
    return lo, hi


def in_function(vram, name):
    lo, hi = bounds(name)
    return lo <= vram and (hi is None or vram < hi)


# ---- The 38 immediates: 19 numbered cutscene animation references.
anim_enum = re.findall(r'/\* \d \*/ (ENHORSE_ANIM_\w+)', read('mm/src/overlays/actors/ovl_En_Horse/z_en_horse.h'))
cs_table = re.findall(r'(ENHORSE_ANIM_\w+),\s*//', array(tables, 'static const u8 sAdultEponaCsAnims'))
anim_names = re.findall(r'"objects/object_horse/(\w+)"', array(resources, 'static const char* const sAdultEponaAnimNames'))
check(len(anim_names) == 9 and len(cs_table) == 19, 'sAdultEponaAnimNames or sAdultEponaCsAnims has the wrong size')
sites = list(re.finditer(r'(Animation_GetLastFrame\()?ENHORSE_CS_ANIM\((\d+), &(object_horse_link_child_Anim_\w+)\)', horse))
check(sorted(int(m.group(2)) for m in sites) == list(range(19)), 'En_Horse does not number references 0-18 once each')
check(len(re.findall(r'&object_horse_link_child_Anim_\w+', horse)) == 19 + 4,
      'a child Epona animation reference outside sEponaAnimations is not numbered')
site_by_ref = {int(m.group(2)): m for m in sites}


def enclosing(pos):
    return [n for at, n in functions if at < pos][-1]


cs = [e for e in pairs if e['change'] == 'cutscene_animation']
for r in range(19):
    halves = [e for e in cs if e['reference'] == r]
    if len(halves) != 2:
        check(False, f'reference {r} has {len(halves)} halves')
        continue
    hi, lo = halves
    off_hi, off_lo = int(hi['offset'], 16), int(lo['offset'], 16)
    check(hi['half'] == 'lui' and lo['half'] == 'addiu' and off_hi < off_lo, f'reference {r}: not a lui/addiu pair')
    check(hi['value'] == fixture['object']['segment'], f'reference {r}: lui does not load the object segment')
    check(hi['function'] == lo['function'] and hi['call'] == lo['call'], f'reference {r}: halves disagree')
    for e in (hi, lo):
        check(in_function(base + int(e['offset'], 16) - 2, e['function']),
              f'reference {r}: {e["offset"]} is not in {e["function"]}')
    tag, name = by_offset.get(int(lo['value'], 16), (None, None))
    check(tag == 'Animation', f'reference {r}: {lo["value"]} is not an object_horse animation')
    if r < len(cs_table) and cs_table[r] in anim_enum and anim_names:
        check(anim_names[anim_enum.index(cs_table[r])] == name,
              f'reference {r}: sAdultEponaCsAnims picks {cs_table[r]}, OoTMM writes {name}')
    site = site_by_ref.get(r)
    if site is None:
        continue
    check(enclosing(site.start()) == hi['function'], f'reference {r} is not in {hi["function"]}')
    nested = site.group(1) is not None
    check(nested == (hi['call'] == 'Animation_GetLastFrame'), f'reference {r} is not the {hi["call"]} argument')
# Within a function the immediates run in overlay order, and IDO evaluates the nested Animation_GetLastFrame first.
for r in range(1, 19):
    a_, b_ = site_by_ref.get(r - 1), site_by_ref.get(r)
    if a_ and b_ and enclosing(a_.start()) == enclosing(b_.start()):
        check(a_.group(1) is not None and b_.group(1) is None, f'references {r - 1}/{r}: GetLastFrame is not first')
check('#defineENHORSE_CS_ANIM(reference,childAnimation)Combo_MM_EponaCsAnimation(reference,childAnimation)' in
      squash(horse), 'ENHORSE_CS_ANIM does not ask the tables')
check('returnsAdultEponaTablesActive?sEponaAnimations[sAdultEponaCsAnims[reference]]:(void*)childAnimation;' in
      squash(body(tables, 'void* Combo_MM_EponaCsAnimation(')), 'cutscene references do not follow the applied tables')

# ---- Animation table, skeleton and eyes: OoT object_horse names in En_Horse's order.
for e in pairs:
    tag, name = by_offset.get(int(e['value'], 16), (None, None))
    if e['change'] == 'animation_table':
        i = anim_enum.index(e['index'])
        check(int(e['offset'], 16) == 0xD792 + 4 * i, f'{e["offset"]}: not entry {i} of sEponaAnimations')
        check(tag == 'Animation' and anim_names[i] == name, f'sAdultEponaAnimNames[{i}] is not {name}')
    elif e['change'] == 'skeleton':
        check(tag == 'Skeleton' and f'sAdultEponaSkelName[]="objects/object_horse/{name}"' in squash(resources),
              f'the loader does not bind {name}')
    elif e['change'] == 'eye':
        check(tag == 'Texture' and
              f'sAdultEponaEyePaths{e["index"]}[]="__OTR__@oot:objects/object_horse/{name}"' in squash(resources),
              f'eye {e["index"]} is not the routed {name}')
apply_ = squash(body(tables, 'void Combo_MM_ApplyEponaVariantTables('))
for term in ('sEponaAnimations[i]=(AnimationHeader*)bindings[i+1];', 'sSkeletonHeaders[HORSE_TYPE_2]=(SkeletonHeader*)bindings[0];',
             'D_80889204[i]=(TexturePtr)bindings[i+10];', 'adult=Combo_MM_AdultEponaResourcesReady();'):
    check(term in apply_, f'Combo_MM_ApplyEponaVariantTables lacks {term}')
binding = squash(body(resources, 'extern "C" void* Combo_MM_GetAdultEponaBinding('))
check('returnComboAdultMM::sAdultEponaAnimations[index-1];' in binding and
      'return&ComboAdultMM::sAdultEponaShadow->header;' in binding, 'bindings are not the shadow skeleton and animations')

# ---- Scale (BBDE: lui upper half) and rider branch (C8AC: beq $zero, $zero = always the limb-30 rider position).
scale = next(e for e in pairs if e['change'] == 'scale')
m = re.search(r'Combo_MM_IsAdultEponaVariantApplied\(\) \? ([0-9.]+)f : 0\.00648f', horse)
child_bits = struct.unpack('>I', struct.pack('>f', 0.00648))[0]
check(m is not None and struct.unpack('>I', struct.pack('>f', float(m.group(1))))[0] ==
      (int(scale['value'], 16) << 16) | (child_bits & 0xFFFF), 'the adult Epona scale is not OoTMM\'s')
branch = next(e for e in pairs if e['change'] == 'rider_branch')
check(int(branch['value'], 16) == 0x1000, 'the rider-position patch is not an unconditional branch')
post = squash(body(horse, 'void EnHorse_PostDraw('))
check('if((this->type==HORSE_TYPE_2)#ifdefCOMBO_BUILD&&!Combo_MM_IsAdultEponaVariantApplied()#endif){Skin_GetVertexPos(skin,5,120,'
      in post, 'adult Epona still takes the child rider vertex')
for e in (scale, branch):
    check(in_function(base + int(e['offset'], 16), e['site']), f'{e["offset"]} is not near {e["site"]}')

# ---- Object swap (En_Horse.c:4-24): the port keeps MM's object for slot bookkeeping; geometry comes from the bindings.
check('returnOBJECT_HORSE_LINK_CHILD;' in squash(body(tables, 'int Combo_MM_EponaVariantObjectId(')),
      'Combo_MM_EponaVariantObjectId changed')
for c in fixture['calls']:
    check(in_function(int(c['vram'], 16), 'EnHorse_Init'), f'{c["vram"]} is not in EnHorse_Init')
# OoTMM patches the overlay and swaps the object only for comboIsLinkAdult().
check('Combo_MM_ApplyEponaVariantTables(Combo_MM_IsLinkAdult());' in body(horse, 'void EnHorse_Init('),
      'En_Horse does not follow Combo_MM_IsLinkAdult()')

# ---- Resource safety (feature plan F5b 1.8) and the readiness contract.
load = squash(body(resources, 'inline bool LoadAdultEpona('))
scope_at = load.find('Ship::ResourceManagerScopescope(oot);')
check(0 <= scope_at < load.find('oot->LoadResourceProcess(sAdultEponaSkelName)'), 'the skeleton is not loaded scoped')
check('dynamic_pointer_cast' not in resources and 'dynamic_cast' not in resources, 'a resource type is checked by cast')
check('GetInitData()->Type==type' in squash(resources), 'resource types are not checked through GetInitData()->Type')
for kind in ('SOH::ResourceType::SOH_Skeleton', 'Fast::ResourceType::DisplayList', 'SOH::ResourceType::SOH_Animation',
             'Fast::ResourceType::Texture'):
    check(kind in load, f'the loader does not check {kind}')
check('CrossRMRegistry::Get("mm")' not in resources and 'GetResourceManager()' not in resources,
      'Epona readiness depends on MM\'s manager')
check('@oot:objects/object_horse/gEponaSkel' not in resources, 'the skeleton is loaded through a routed path')
drop = squash(body(resources, 'inline void DropAdultEpona('))
restore_at = drop.find('Combo_MM_ApplyEponaVariantTables(0);')
check(0 <= restore_at < drop.find('sAdultEponaShadow.reset();') and restore_at < drop.find('sAdultEponaResources.clear();'),
      'teardown does not restore the child tables first')
shadow = squash(body(resources, 'inline bool BuildAdultEponaShadow('))
check('"__OTR__@oot:"+displayLists.back()' in shadow, 'shadow limbs do not route their display lists to OoT')
check('limb=*(const::SkinLimb*)oot->segment[i];' in shadow and 'oot->segment[i]=' not in shadow,
      'the shadow does not copy OoT\'s limbs')
# The shadow copies OoT's skin structs into MM's own: their fields must line up type for type.


def fields(source, name):
    """Field types of a C struct typedef, in order."""
    m = re.search(r'typedef struct \{([^{}]*)\}\s*' + name + r';', source)
    text = re.sub(r'/\*.*?\*/|//[^\n]*', '', m.group(1), flags=re.S) if m else ''
    return [re.sub(r'\s*\w+(\[\d+\])?$', '', f.strip()) for f in text.split(';') if f.strip()]


oot_skin = read('soh/soh/resource/type/SkeletonLimb.h')
mm_skin = read('mm/include/z64skin.h')
for name in ('SkinVertex', 'SkinTransformation', 'SkinLimbModif', 'SkinAnimatedLimbData', 'SkinLimb'):
    check(fields(oot_skin, name) and fields(oot_skin, name) == fields(mm_skin, name), f'{name} differs between OoT and MM')
check(fields(read('soh/soh/resource/type/Skeleton.h'), 'SkeletonHeader') == ['void**', 'uint8_t', 'uint8_t'] and
      'void**segment;uint8_tlimbCount;uint8_tskeletonType;' in squash(resources),
      'OotSkeletonHeader no longer mirrors OoT\'s raw skeleton header')
check(fields(read('mm/include/z64animation.h'), 'SkeletonHeader') == ['void**', 'u8'], 'MM\'s SkeletonHeader changed')
# MM's resource type ids equal OoT's, so MM's enum checks OoT's resources.
mm_types = read('mm/2s2h/resource/type/2shResourceType.h')
oot_types = read('soh/soh/resource/type/SohResourceType.h')
for kind in ('SOH_Skeleton', 'SOH_Animation'):
    ours = re.search(kind + r'\s*=\s*(0x[0-9A-F]+)', mm_types)
    theirs = re.search(kind + r'\s*=\s*(0x[0-9A-F]+)', oot_types)
    check(ours and theirs and ours.group(1) == theirs.group(1), f'{kind} differs between MM and OoT')

# ---- OoTMM v32.3 itself.
reference = a.reference
if reference is None:
    for candidate in (root.parent.parent / 'references/ootmm-stable-32.3', root.parent / 'references/ootmm-stable-32.3'):
        if candidate.is_dir():
            reference = candidate
            break
if reference is not None:
    actors = (reference / 'packages/generator/src/mm/actors.c').read_text(encoding='utf-8')
    block = actors[actors.index('static OverlayPatch adultEponaPatch[] = {'):]
    block = block[:block.index('};')]
    upstream = [(int(o, 16), int(v, 16)) for o, v in re.findall(r'\{(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+)\}', block)]
    check(upstream == [(int(e['offset'], 16), int(e['value'], 16)) for e in pairs], 'the fixture differs from adultEponaPatch')
    check('if (!comboIsLinkAdult())' in body(actors, 'static void Actor_PatchAdultHorse('),
          'OoTMM no longer patches only for adult Link')
    swap = (reference / 'packages/generator/src/mm/actors/En/En_Horse.c').read_text(encoding='utf-8')
    calls = [int(v, 16) for v in re.findall(r'PATCH_CALL\((0x[0-9a-fA-F]+),', swap)]
    check(calls == [int(c['vram'], 16) for c in fixture['calls']], 'the fixture differs from En_Horse.c\'s object swap')
    print(f'OoTMM reference checked: {reference}')
else:
    print('OoTMM reference not found; checked the fixture against the production sources only')

if failures:
    print('\n'.join(failures))
    sys.exit(1)
print('Adult Epona: all 53 OoTMM patch pairs and the object swap are mapped to typed changes; resource rules hold')
