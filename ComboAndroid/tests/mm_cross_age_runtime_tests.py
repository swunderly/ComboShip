"""Adult Link in MM with OoT's own adult model (plan F5b, OoTMM v32.3 Cross-Games Age): runtime source checks.

H1: every human slot of MM's player tables (z_player_lib.c: the display-list groups, the first-person tables, the bow
string, the human skeleton and the eye and mouth rows) is classified exactly once: taken by the adult model as plan
F5b 1.3 maps it, or kept as MM's own. Every OoT name the loader asks for is in all twelve retail object_link_boy.xml
files (identical), and every MM name in MM's N64_US XMLs.
H4: adult Link's human age properties match OoTMM v32.3's sPlayerAgePropertiesAdult field by field (with
gPlayerAnim_pz_climb_endAR for OoTMM's 0x04008360), and the door animations its player patch sets.
Also: the draw glue, the readiness gate and release switch, and the resource rules of plan F5b 1.8.
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
# The shadow skeleton is not a resource path, so SkelAnime_InitPlayer registers nothing for it: it must drop the entry
# an earlier form left for the same SkelAnime, or an Alternate Assets toggle puts that skeleton on adult Link.
init_player = re.sub(r'//[^\n]*', '', read('mm/src/code/z_skelanime.c'))
init_player = squash(init_player[init_player.index('void SkelAnime_InitPlayer('):])
check('if(ResourceMgr_OTRSigCheck(skeletonHeaderSeg)!=0)skeletonHeaderSeg=ResourceMgr_LoadSkeletonByName('
      'skeletonHeaderSeg,skelAnime);#ifdefCOMBO_BUILDelseResourceMgr_UnregisterSkeleton(skelAnime);#endif'
      in init_player, 'a Player SkelAnime initialised with the adult shadow skeleton keeps a stale SkeletonPatcher entry')
check('&sPlayerEyesTextures[PLAYER_FORM_HUMAN][index]' in squash(tables_src) and
      '&sPlayerMouthTextures[PLAYER_FORM_HUMAN][index-COMBO_ADULT_EYE_TEXTURES]' in squash(tables_src),
      'the face rows are not the human rows')
for weapon, length in (('KOKIRI', '4000.0f'), ('RAZOR', '4000.0f'), ('GILDED', '5500.0f'), ('TWO_HANDED', '5500.0f')):
    check(f'sMeleeWeaponLengths[PLAYER_MELEEWEAPON_SWORD_{weapon}]={length};' in apply,
          f'adult {weapon} length is not OoTMM\'s {length} (play.c:713-717)')
check('D_801C0D94' not in tables_src and 'gPlayerSheath' not in tables_src and 'gPlayerShields' not in tables_src,
      'a slot plan F5b keeps as MM\'s own is changed')

# Draw glue (plan F5b 5b-3): OoT's adult lists take the tunic colour from env, the Kokiri green unless a gear tunic
# is worn; a face row that Play As Kafei rewrote while adult is captured again, and only the adult texture is given back.
draw = squash(lib[lib.index('void Player_DrawImpl('):lib.index('Vec3f D_801C08C0')])
check('if(playerForm==PLAYER_FORM_HUMAN&&(Combo_MM_GearActive()||Combo_MM_IsLinkAdult())){constColor_RGB8*color='
      'Combo_MM_GetSavedTunicColor(Combo_MM_GearActive()?gSaveContext.save.shipSaveInfo.adultMm.tunic:0);' in draw,
      'adult Link env colour is not the Kokiri green without a worn gear tunic')
check(draw.index('gDPSetEnvColor(POLY_OPA_DISP++') < draw.index('SkelAnime_DrawFlexLod('), 'env colour after the skeleton')
# MM's Razor, Gilded and Great Fairy's Sword and Mirror Shield lists set env and leave it, before OoT's right shoulder
# and torso draw; every adult limb sets the tunic colour again (OoTMM removes those env writes, Player.S:948-1215).
check(draw.index('sComboAdultTunicEnv=NULL;') < draw.index('if(Combo_MM_IsLinkAdult()){sComboAdultTunicEnv=color;}')
      < draw.index('SkelAnime_DrawFlexLod(') < draw.index('sComboAdultTunicEnv=NULL;', draw.index('SkelAnime_DrawFlexLod(')),
      'adult Link\'s tunic env colour is not kept for his limbs during his skeleton draw only')
common = squash(lib[lib.index('s32 Player_OverrideLimbDrawGameplayCommon('):lib.index('if (limbIndex == PLAYER_LIMB_ROOT)',
                                                                                lib.index('s32 Player_OverrideLimbDrawGameplayCommon('))])
check('if(sComboAdultTunicEnv!=NULL){OPEN_DISPS(play->state.gfxCtx);gDPSetEnvColor(POLY_OPA_DISP++,'
      'sComboAdultTunicEnv->r,sComboAdultTunicEnv->g,sComboAdultTunicEnv->b,255);' in common,
      'an MM sword or shield drawn before an adult limb can recolour the tunic')
check('if(*Combo_MM_HumanFaceRow(i)!=Combo_MM_CrossAgeFaceTexture(i)){sComboChildFace[i]=*Combo_MM_HumanFaceRow(i);'
      in apply and 'if(*Combo_MM_HumanFaceRow(i)==Combo_MM_CrossAgeFaceTexture(i)){*Combo_MM_HumanFaceRow(i)='
      'sComboChildFace[i];' in squash(tables_src), 'face rows are not captured and given back around Play As Kafei')
check(sum(1 for line in read('mm/2s2h/Enhancements/GfxPatcher/PlayerCustomFlipbooks.cpp').splitlines()
          if 'PLAYER_FORM_HUMAN' in line) == 0, 'custom flipbooks now write the human face rows')

# ---- Readiness and the release switch (plan F5b 5b-8, 5b-9 runtime half, 5b-11).
benport = read('mm/2s2h/BenPort.cpp')


def body(source, signature):
    """Text of the definition that starts at signature, up to its matching brace (whitespace removed)."""
    m = re.search(re.escape(signature) + r'[^;{]*\{', source)
    at, depth, i = m.start(), 0, m.end() - 1
    while True:
        depth += (source[i] == '{') - (source[i] == '}')
        i += 1
        if depth == 0:
            return squash(source[at:i])


# The release switch is set in its own commit, which can be reverted alone; these checks hold either way.
check(re.search(r'staticconstexprboolkCrossAgeRuntimeComplete=(true|false);', squash(benport)) is not None,
      'the Cross-Games Age runtime has no release switch')
check('#ifdefCOMBO_ADULT_MM_RUNTIME_DIAGNOSTICS' in body(benport, 'static int Combo_MM_CrossAgeRuntimeEnabled(void)') and
      'returnkCrossAgeRuntimeComplete||CVarGetInteger("gCombo.Debug.CrossAgeRuntimeValidation",0);#else'
      'returnkCrossAgeRuntimeComplete;#endif' in body(benport, 'static int Combo_MM_CrossAgeRuntimeEnabled(void)'),
      'diagnostics switch')
check(body(benport, 'extern "C" int Combo_MM_CrossAgeReady(void)') ==
      squash('extern "C" int Combo_MM_CrossAgeReady(void) { return Combo_MM_CrossAgeRuntimeEnabled() && '
             'Combo_MM_CrossAgeModelReady() && Combo_MM_AdultEponaResourcesReady(); }'),
      "Combo_MM_CrossAgeReady is not the release switch, OoT's adult model and OoT's Epona (decision D4)")
check('Combo_MM_CrossAgeReady()' in body(benport, 'extern "C" int Combo_MM_CrossAgeOptionReady(void)') and
      'returnCombo_MM_CrossAgeOptionReady();' in body(benport, 'extern "C" COMBO_EXPORT int MM_CrossAgeOptionReady(void)'),
      'the menu export MM_CrossAgeOptionReady does not follow Combo_MM_CrossAgeReady')
check('MM_AdultRuntimeReady' not in benport and 'COMBO_EXPORT int MM_CrossAgeReady(' not in benport,
      'Cross-Games Age has one readiness export (MM_CrossAgeOptionReady)')
linkadult = body(benport, 'extern "C" int Combo_MM_IsLinkAdult(void)')
check('ComboAdultMM::IsCrossAgeAdult(gSaveContext.save)' in linkadult and 'returnCombo_MM_CrossAgeReady();' in linkadult
      and 'AdultResourcesReady' not in linkadult, 'Combo_MM_IsLinkAdult is not seed crossAge && adult && ready')
check(body(read('mm/2s2h/ComboAdultMM.h'), 'inline bool IsCrossAgeAdult(const Save& save)') ==
      squash('inline bool IsCrossAgeAdult(const Save& save) { return '
             'save.shipSaveInfo.rando.randoSaveOptions[RO_MM_CROSS_AGE] && IsAdult(save); }'), 'IsCrossAgeAdult')
# The readiness answers before MM has booted: OoT's side needs nothing from MM's manager.
check('Get("mm")' not in body(loader, 'inline bool LoadAdultOotModel()') and
      body(loader, 'extern "C" int Combo_MM_CrossAgeModelReady(void)') ==
      squash('extern "C" int Combo_MM_CrossAgeModelReady(void) { return ComboAdultMM::LoadAdultOotModel() && '
             '!ComboAdultMM::AdultMmFailed(); }'), 'the model readiness depends on MM\'s manager')
# Resource safety (plan F5b 1.8): the OoT skeleton loads only inside a scope that makes OoT's manager active, types are
# compared by tag, and OoT's cached limbs are only read.
oot_load = body(loader, 'inline bool LoadAdultOotModel()')
check('{Ship::ResourceManagerScopescope(oot);skeleton=oot->LoadResourceProcess(kAdultSkeleton);}' in oot_load,
      'the OoT skeleton is loaded outside a scope on OoT\'s manager')
loader_code = re.sub(r'//.*', '', loader)
check(loader_code.count('LoadResourceProcess(kAdultSkeleton)') == 1 and 'dynamic_cast' not in loader_code and
      'dynamic_pointer_cast' not in loader_code, 'the loader loads the skeleton twice or casts across modules')
check('constLodLimb*' in oot_load and 'sAdultShadowLimbs.data()' in oot_load, 'the shadow skeleton is not MM\'s own')
check(body(loader, 'inline void DropAdultOotModel()').index('Combo_MM_RestoreChildModelTables();') <
      body(loader, 'inline void DropAdultOotModel()').index('sAdultOotPins.clear();'),
      'teardown drops resources before restoring the child tables')

# ---- H4: adult Link's human age properties are OoTMM v32.3's sPlayerAgePropertiesAdult field by field
# (generator/src/mm/actors.c:971-1031), and the door animations its Actor_PatchPlayerFormProperties sets (:1033-1058).
# OoTMM's values, in field order; animations as MM US gameplay_keep offsets. 0x8360 is not an animation there: the plan
# substitutes gPlayerAnim_pz_climb_endAR (0xE360), the animation OoTMM's own comment names.
OOTMM_ADULT = [56.0, 90.0, 1.0, 111.0, 70.0, 79.4, 59.0, 41.0, 19.0, 36.0, 50.0, 56.0, 68.0, 70.0, 18.0, 23.0, 70.0,
               9, 0x123F, 0x167,
               8, 0x1256, 0x17C, 9, 0x17EA, 0x167, 8, 0x1256, 0x17C, 9, 0x17EA, 0x167,
               9, 0x17EA, 0x167, 9, 0x1E0D, 0x17C, 9, 0x17EA, 0x167, 9, 0x1E0D, 0x17C,
               8, 0x1256, 0x17C, 9, 0x17EA, 0x167, -0x638, 0x1256, 0x17C, -0x637, 0x17EA, 0x167,
               0, 0x80, 22.0, 36.0,
               0xE300, 0xD548, 0xD660, 0xE378, 0xE380, 0xE388, 0xE390, 0xDAB0, 0xDAB8, 0xDA90, 0xDA98, 0xE358, 0x8360,
               0xE370, 0xE368]
OOTMM_DOORS = (0xE3C8, 0xE3D0)


def number(token):
    token = token.strip().rstrip('f')
    sign = -1 if token.startswith('-') else 1
    token = token.lstrip('-')
    return sign * (int(token, 16) if token.lower().startswith('0x') else (float(token) if '.' in token else int(token)))


def find_reference():
    for parent in [root, *root.parents]:
        candidate = parent / 'references/ootmm-stable-32.3'
        if (candidate / 'packages/generator/src/mm/actors.c').is_file():
            return candidate
    return None


reference = find_reference()
if reference:
    actors = (reference / 'packages/generator/src/mm/actors.c').read_text(encoding='utf-8')
    block = re.search(r'static PlayerAgeProperties sPlayerAgePropertiesAdult = \{(.*?)\n\};', actors, re.S).group(1)
    block = re.sub(r'/\*.*?\*/', '', block)
    block = re.sub(r'\(PlayerAnimationHeader \*\)0x0400', '0x', block)
    upstream = [number(t) for t in re.findall(r'-?0x[0-9a-fA-F]+|-?\d+\.\d+f|-?\d+', block)]
    check(upstream == OOTMM_ADULT, 'the test\'s OoTMM values differ from the reference actors.c')
    doors = re.findall(r'playerAnimationHeaders\[0x(3[6-9ab]|4[2-7])\] = 0x0400([0-9a-f]+);', actors)
    check(sorted({(int(i, 16) // 6, int(v, 16)) for i, v in doors}) == [(9, OOTMM_DOORS[0]), (11, OOTMM_DOORS[1])] and
          len(doors) == 12, 'door animation patches differ from the reference actors.c')
    print(f'OoTMM reference checked: {reference}')

fields = []
for kind, name, count in re.findall(r'/\* 0x\w+ \*/ (f32|Vec3s|u16|PlayerAnimationHeader\*) (\w+)(?:\[(\d)\])?;',
                                    re.search(r'typedef struct PlayerAgeProperties \{(.*?)\} PlayerAgeProperties;',
                                              read('mm/include/z64player.h'), re.S).group(1)):
    fields.append((kind, name, int(count or 1)))
keep_xml = read('mm/assets/xml/N64_US/objects/gameplay_keep.xml')
anim_offset = {n: int(o, 16) for n, o in re.findall(r'<PlayerAnimation Name="(\w+)" Offset="0x([0-9A-Fa-f]+)"', keep_xml)}
props_src = read('mm/2s2h/ComboAdultMM.h')
props = body(props_src, 'inline PlayerAgeProperties BuildAdultProperties()')
arrays = {n: [number(t) for t in re.findall(r'-?0x[0-9A-Fa-f]+|-?\d+', v)]
          for n, v in re.findall(r'constVec3s(\w)\[4\]=\{(.*?)\};', props)}
loop = dict(re.findall(r'p\.(unk_\w+)\[i\]=(\w)\[i\];', props))
production = []
for kind, name, count in fields:
    if kind == 'f32' or kind == 'u16':
        m = re.search(r'p\.' + name + r'=([^;]+);', props)
        production.append(number(m.group(1)) if m else None)
    elif kind == 'Vec3s' and count == 1:
        m = re.search(r'p\.' + name + r'=\{([^}]*)\};', props)
        production += [number(t) for t in m.group(1).split(',')] if m else [None] * 3
    elif kind == 'Vec3s':
        production += arrays.get(loop.get(name), [None] * 12)
    else:
        for i in range(count):
            m = re.search(r'ADULT_ANIM\(' + re.escape(name + (f'[{i}]' if count > 1 else '')) + r',(\w+)\);', props)
            production.append(anim_offset.get(m.group(1)) if m else None)
expected = list(OOTMM_ADULT)
expected[expected.index(0x8360)] = anim_offset['gPlayerAnim_pz_climb_endAR']
check(len(production) == len(expected) and production == expected,
      'adult age properties differ from OoTMM v32.3: ' +
      str([(i, production[i] if i < len(production) else None, e) for i, e in enumerate(expected)
           if i >= len(production) or production[i] != e]))
check(anim_offset['gPlayerAnim_pz_climb_endAR'] == 0xE360 and 0x8360 not in anim_offset.values(),
      'pz_climb_endAR substitution no longer needed or wrong')
age_native = squash(read('mm/src/overlays/actors/ovl_player_actor/ComboAdultAgeNative.h'))
check('D_8085BE84[PLAYER_ANIMGROUP_doorA][i]=(PlayerAnimationHeader*)&gPlayerAnim_pz_doorA_open;' in age_native and
      'D_8085BE84[PLAYER_ANIMGROUP_doorB][i]=(PlayerAnimationHeader*)&gPlayerAnim_pz_doorB_open;' in age_native and
      (anim_offset['gPlayerAnim_pz_doorA_open'], anim_offset['gPlayerAnim_pz_doorB_open']) == OOTMM_DOORS,
      'adult door animations are not OoTMM\'s')
check(re.search(r'PLAYER_ANIMGROUP_doorA, // OPEN_DOOR_LEFT_HUMAN', read('mm/include/z64player.h')) and
      re.search(r'/\*  9 \*/ PLAYER_ANIMGROUP_doorA,', read('mm/include/z64player.h')) and
      re.search(r'/\* 11 \*/ PLAYER_ANIMGROUP_doorB,', read('mm/include/z64player.h')),
      'anim groups doorA/doorB are not OoTMM\'s 0x36/0x42 (groups 9 and 11 of 6 types)')
check('AdultMask' not in read('mm/src/overlays/actors/ovl_player_actor/ComboAdultAgeNative.h'),
      'adult age still depends on the Adult Mask')

# ---- OoTMM v32.3's adult runtime branches (plan F5 steps 6 and 8 as F5b amends them), all on Combo_MM_IsLinkAdult.
player = read('mm/src/overlays/actors/ovl_player_actor/z_player.c')
seam = squash(player[player.index('// Commit exact human model/property resources'):])
check(seam.startswith(squash('''// Commit exact human model/property resources before native equipment and skeleton init.
    // Other forms retain their original tables; legacy saves remain native child MM.
    {
        // The tables report whether the adult model could be drawn; without it Link stays child throughout.
        int adult = Combo_MM_ApplyAdultModelTables(Combo_MM_IsLinkAdult());
        Combo_MM_ApplyAdultOverlayAge(adult);
        Combo_MM_ApplyAdultGameplayKeep(play, adult);''')), 'Player_Init does not apply the adult runtime from the tables')
check('((this->heldItemAction==PLAYER_IA_HOOKSHOT&&(this->stateFlags1&PLAYER_STATE1_8000000))||Combo_MM_IsLinkAdult())'
      '?CAM_MODE_BOWARROW' in body(player, 's32 func_8083868C(PlayState* play, Player* this)'),
      'item camera (Player.c:2034-2051)')
check('if(Combo_MM_IsLinkAdult())returnextraHeight+68.0f;' in body(read('mm/src/code/z_actor.c'),
                                                                     'f32 Player_GetHeight(Player* player)'),
      'adult Link is not the Zora\'s height (play.c:721-722)')
check('if(player->transformation==PLAYER_FORM_HUMAN){#ifdefCOMBO_BUILD//OoTMMholdsadultLink\'sarrowattheotherforms\''
      'point(Player.S:865-876).if(!Combo_MM_IsLinkAdult())#endifvar_a0=&D_801C0D6C;}' in squash(lib),
      'arrow position (Player.S:865-876)')
bremen = 'if(limbIndex==PLAYER_LIMB_ROOT&&(player->stateFlags3&PLAYER_STATE3_20000000)&&Combo_MM_IsLinkAdult()){pos->y+=1280.0f;}'
check(bremen in body(lib, 's32 Player_OverrideLimbDrawGameplayDefault(') and
      '1280' not in body(lib, 's32 Player_OverrideLimbDrawGameplayFirstPerson('), 'Bremen march raise (Player.c:2135-2150)')
check('f32yOffset=Combo_MM_IsLinkAdult()?80.0f:60.0f;' in squash(read('mm/src/code/ComboFarorePointer.c')),
      'Farore\'s Wind pointer height (actors.c:693)')
check('casePLAYER_FORM_HUMAN:returnCombo_MM_IsLinkAdult()?0.6f:0.4f;' in
      squash(read('mm/src/overlays/actors/ovl_Combo_Magic_Dark/z_combo_magic_dark.c')),
      'Nayru\'s Love size (Custom_Magic_Dark.c:63-64)')
check('if(spellSfxIndex==2&&Combo_MM_IsLinkAdult()){spellSfxIndex=3;}' in
      squash(read('mm/src/overlays/actors/ovl_player_actor/ComboSpellPlayer.inc.h')), 'spell sound (Player.c:431-435)')
giant = body(read('mm/src/audio/code_8019AF00.c'), 'void Audio_PlaySfx_GiantsMask(Vec3f* pos, u16 sfxId)')
check('if(Combo_MM_IsLinkAdult()){AudioSfx_PlaySfx(sfxId&0x681F,pos,4,' in giant and
      'AudioSfx_PlaySfx((sfxId&0x681F)+0x20,pos,4,' in giant, 'Giant\'s Mask voice (Player.c:2088-2097)')
check('Actor_OfferGetItem(&this->actor,play,GI_NONE,60.0f,Combo_MM_IsLinkAdult()?96.0f:40.0f);' in
      squash(read('mm/src/overlays/actors/ovl_En_Zog/z_en_zog.c')), 'Mikau grab range (En_Zog.c:85-93)')
icefloe = squash(read('mm/src/overlays/actors/ovl_Bg_Icefloe/z_bg_icefloe.c'))
check('if(Combo_MM_IsLinkAdult()){this->dyna.actor.scale.y=0.20f;}' in icefloe and
      'if(Combo_MM_IsLinkAdult()){this->dyna.actor.scale.y=0.13f;}' in icefloe, 'ice platform (Bg_Icefloe.c:132, 140)')
check('IsLinkAdult' not in read('mm/src/overlays/actors/ovl_Arms_Hook/z_arms_hook.c'),
      'the hookshot chain is no longer MM\'s own (lead decision D2)')
# No adult runtime branch of Cross-Games Age still asks the Adult Mask aggregate.
for rel in ('mm/src/code/z_actor.c', 'mm/src/code/ComboFarorePointer.c', 'mm/src/audio/code_8019AF00.c',
            'mm/src/overlays/actors/ovl_Combo_Magic_Dark/z_combo_magic_dark.c',
            'mm/src/overlays/actors/ovl_player_actor/ComboSpellPlayer.inc.h', 'mm/src/code/ComboAdultModelTablesNative.h'):
    text = read(rel)
    check('Combo_MM_AdultResourcesReady()' not in text and 'Combo_MM_IsAdult()' not in text and
          'EffectiveHumanAge' not in text, f'{rel} still uses the old adult gate')
for signature in ('s32 Player_OverrideLimbDrawGameplayDefault(', 'void Player_DrawImpl(', 's32 func_8083868C('):
    source = lib if 'func_8083868C' not in signature else player
    check('Combo_MM_IsAdult()' not in body(source, signature) and
          'Combo_MM_AdultResourcesReady()' not in body(source, signature), f'{signature} still uses the old adult gate')

# ---- Spin attack (plan F5b 5b-6, lead decision D3): MM's own spin vertices scaled at run time by OoTMM's factors, and
# the spin-charge list's first N64 command (after the port's two-slot marker, not slot 0) calls the 1.25 scale list.
keep = read('mm/2s2h/ComboAdultGameplayKeepResources.h')
model = read('mm/2s2h/ComboAdultOotModel.h')
check('constexprfloatkSpinScale[5]={1.5f,1.5f,1.3f,1.3f,1.05f};' in squash(model) and
      'constexprfloatkMaskScale=1.25f;' in squash(model), 'spin and mask scales are not OoTMM\'s')
check(re.findall(r'gameplay_keepVtx_(\w+)"', keep) == ['025510', '0256B0', '025A90', '025C30', '026810'] and
      'kSpinVertexCounts[5]={26,26,26,26,14};' in squash(keep), 'spin vertex arrays')
load_keep = body(keep, 'inline bool LoadAdultKeep()')
check('constlongslot=ComboAdultOotModel::N64CommandSlot(charging->Instructions.data(),charging->Instructions.size(),0);'
      in load_keep and '(charging->Instructions[slot].words.w0>>24)!=G_RDPPIPESYNC' in load_keep,
      'the spin-charge patch does not walk past the marker to the pipe sync')
check('Instructions[0]' not in keep and 'sSpinChargingDL->Instructions[sSpinChargingSlot]=call;' in squash(keep),
      'the spin-charge patch overwrites the marker')
check('coord=ComboAdultOotModel::ScaleSpinCoord(coord,ComboAdultOotModel::kSpinScale[i]);' in load_keep and
      'combo_adult' not in keep, 'spin vertices are not derived from MM\'s own')
check('gsDPPipeSync(),gsSPMatrix(&sAdultMaskScaleMtx,G_MTX_NOPUSH|G_MTX_MUL|G_MTX_MODELVIEW),gsSPEndDisplayList()' in
      squash(keep) and 'SkinMatrix_SetScale(&scale,ComboAdultOotModel::kMaskScale,ComboAdultOotModel::kMaskScale,'
      'ComboAdultOotModel::kMaskScale);' in squash(keep), 'the mask scale list is not [sync, 1.25 scale, end]')
if reference:
    scene = (reference / 'packages/generator/src/mm/scene.c').read_text(encoding='utf-8')
    check(all(f'gameplayKeep + 0x{o}' in scene for o in ('25510', '25A90', '26810', '5A2A0')) and
          'spinAttackChargingDL1[0] = 0xde000000;' in scene and 'spinAttackChargingDL1[1] = 0x0405a2e0;' in scene and
          'maskMatrixDL[0] = 0xe7000000;' in scene and 'maskMatrixDL[2] = 0xda380001;' in scene and
          'maskMatrixDL[4] = 0xdf000000;' in scene, 'OoTMM scene.c no longer patches the spin attack this way')

# ---- Mask fit (plan F5b 5b-5): Keaton, Bunny and Scents call the scale list at OoTMM's commands, found by walking the
# port's list (H6), checked against the command OoTMM replaces, and given back for child Link.
mask_src = read('mm/2s2h/ComboAdultMaskScale.h')
patches = [(m, l, int(c), w == 'true') for m, l, c, w in re.findall(
    r'\{\s*PLAYER_MASK_(\w+),\s*"__OTR__objects/\w+/(\w+)",\s*(\d+),\s*(true|false)\s*\}', mask_src)]
check(patches == [('KEATON', 'object_mask_ki_tan_DL_0004A0', 1, True), ('BUNNY', 'object_mask_rabit_DL_000610', 2, True),
                   ('BUNNY', 'object_mask_rabit_DL_000610', 46, False), ('BUNNY', 'object_mask_rabit_DL_000610', 52, True),
                   ('BUNNY', 'object_mask_rabit_DL_000610', 80, False), ('BUNNY', 'object_mask_rabit_DL_000610', 86, True),
                   ('SCENTS', 'object_mask_bu_san_DL_000710', 1, True)], f'mask patches: {patches}')
slots_fn = body(mask_src, 'inline bool AdultMaskScaleSlots(')
check('slots[i]=ComboAdultOotModel::N64CommandSlot(list.data(),list.size(),patch.command);' in slots_fn and
      'constuint8_texpected=patch.withSync?G_RDPPIPESYNC:G_TEXTURE;' in slots_fn and
      'resource->GetInitData()->IsCustom' in slots_fn, 'mask patch slots are not walked and checked')
check('if(player->actor.id==ACTOR_PLAYER){externintCombo_MM_IsLinkAdult(void);externvoidCombo_MM_ApplyAdultMaskScale(s32mask,'
      's32adult);Combo_MM_ApplyAdultMaskScale(player->currentMask,Combo_MM_IsLinkAdult());}' in
      squash(lib[lib.index('void func_801229FC(Player* player)'):lib.index('void func_80122BA4(')]),
      'the mask fit does not run when Player\'s mask has loaded')
check('Combo_MM_ApplyAdultMaskScale(this->currentMask,adult);' in seam, 'Player_Init does not fit or restore the mask')
# OoTMM writes the calls into Link's own copy of the mask object, so Kafei's Keaton Mask (EnTest3, which draws MM's
# shared list from its own buffer) keeps its size: Player draws a fitted copy and the cached lists are never written.
check('PatchGfxByName' not in mask_src and 'ResourceManagerScope' not in mask_src,
      'the mask fit edits MM\'s shared mask lists, which Kafei also draws')
apply_fit = body(mask_src, 'extern "C" void Combo_MM_ApplyAdultMaskScale(s32 mask, s32 adult)')
check('sAdultMaskFit=adult!=0;' in apply_fit and
      'fitted=ComboAdultOotModel::FittedMaskList(list->Instructions.data(),list->Instructions.size(),at.data(),'
      'calls.data(),at.size());' in apply_fit and 'sAdultMaskCopies[copy]=std::move(fitted);' in apply_fit,
      'the mask fit does not build Player\'s own copy')
check('returnsAdultMaskFit&&copy>=0&&!sAdultMaskCopies[copy].empty()?sAdultMaskCopies[copy].data():nullptr;' in
      body(mask_src, 'extern "C" Gfx* Combo_MM_AdultMaskDL(s32 mask)'), 'Combo_MM_AdultMaskDL is not the fitted copy')
held = squash(lib[lib.index('s32 func_80128640('):lib.index('} else if (player->itemAction == PLAYER_IA_DEKU_STICK)')])
worn = squash(lib[lib.index('} else if (limbIndex == PLAYER_LIMB_HEAD) {', lib.index('void Combo_Player_PostLimbDraw')):])
check('Gfx*fitted=(player->actor.id==ACTOR_PLAYER)?Combo_MM_AdultMaskDL(mask):NULL;if(fitted!=NULL){gSPDisplayList('
      'POLY_OPA_DISP++,fitted);}else{gSPDisplayList(POLY_OPA_DISP++,D_801C0B20[mask-1]);}' in held,
      'the mask in Link\'s hand is not his own fitted copy, or Kafei (who shares func_80128640) draws it')
check('Gfx*fitted=(player->actor.id==ACTOR_PLAYER)?Combo_MM_AdultMaskDL(maskMinusOne+1):NULL;if(fitted!=NULL){'
      'gSPDisplayList(POLY_OPA_DISP++,fitted);}else{gSPDisplayList(POLY_OPA_DISP++,D_801C0B20[maskMinusOne]);}' in worn,
      'the mask on Link\'s head is not his own fitted copy')
kafei = read('mm/src/overlays/actors/ovl_En_Test3/z_en_test3.c')
check('Combo_MM_AdultMaskDL' not in kafei and 'gSPDisplayList(POLY_OPA_DISP++,object_mask_ki_tan_DL_0004A0);' in
      squash(kafei), 'Kafei no longer draws MM\'s own Keaton Mask list')
if reference:
    after = re.search(r'u8 Player_AfterMaskLoaded\(Player\* player\)\n\{(.*?)\n\}', (reference /
                      'packages/generator/src/mm/actors/Player.c').read_text(encoding='utf-8'), re.S).group(1)
    upstream = []
    for case, block in re.findall(r'case MASK_(\w+):(.*?)break;', after, re.S):
        writes = dict(re.findall(r'maskObjectSegment\[0x([0-9a-f]+)\] = 0x([0-9a-f]+);', block))
        lists = {'KEATON': 'object_mask_ki_tan', 'BUNNY': 'object_mask_rabit', 'SCENTS': 'object_mask_bu_san'}
        xml = read(f'mm/assets/xml/N64_US/objects/{lists[case]}.xml')
        name, offset = re.search(r'<DList Name="(\w+)" Offset="0x([0-9A-Fa-f]+)"', xml).groups()
        for word, value in sorted((int(w, 16), v) for w, v in writes.items()):
            if value == 'de000000':
                target = int(writes[format(word + 1, 'x')], 16)
                check(target in (0x0405A2E0, 0x0405A2E8), f'{case} target {target:#x}')
                upstream.append((case, name, (word - int(offset, 16) // 4) // 2, target == 0x0405A2E0))
    check(upstream == patches, f'mask patches differ from OoTMM Player_AfterMaskLoaded: {upstream}')

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
