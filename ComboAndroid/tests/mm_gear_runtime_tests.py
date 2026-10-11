"""MM boots and tunics (plan F2 phase B, critique X3): runtime gates, OoTMM parity and the menu readiness gate.

Checks the production sources for: the gear runtime switch (Combo_MM_GearActive) separate from Adult Link in MM, the
load policy that never refuses a save for its adult or gear state, the readiness gate that greys the gear options out
and leaves them out of new seeds where the gear cannot run, the menu, profile, Save Editor and tracker rows, the child
boot textures (a subset of the names the readiness check validates), the Hover Boots lily pads, the Iron Boots under
Swim shuffle and the Goron/Zora Tunic colour patch (on the combiners just before 2S2H's own tunic cosmetic patches).
When the OoTMM v32.3 checkout is available (--reference, or references/ootmm-stable-32.3 beside the worktree), the
copied boot table, child boot and hover-circle display lists, tunic palette, gear setting text, tunic combiner patches
and no-scale water check are checked against it.
"""
from pathlib import Path
import argparse
import re
import sys

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
    """Text of the definition that starts at signature, up to its matching brace (whitespace removed)."""
    m = re.search(re.escape(signature) + r'[^;{]*\{', source)
    at = m.start()
    depth, i = 0, m.end() - 1
    while True:
        depth += (source[i] == '{') - (source[i] == '}')
        i += 1
        if depth == 0:
            return squash(source[at:i])


benport = read('mm/2s2h/BenPort.cpp')
GEAR = ['RO_MM_BOOTS_IRON', 'RO_MM_BOOTS_HOVER', 'RO_MM_TUNIC_GORON', 'RO_MM_TUNIC_ZORA']

# ---- One gear switch, separate from Adult Link in MM (critique X3, lead decision B0 = 1).
check('staticconstexprboolkMmGearIntegrationComplete=' in squash(benport), 'no kMmGearIntegrationComplete switch')
active = body(benport, 'extern "C" int Combo_MM_GearActive(void)')
for term in ('Combo_MM_GearIntegrationEnabled()', 'ComboAdultMM::SaveHasGear(gSaveContext.save)',
             'returnCombo_MM_GearResourcesReady();'):
    check(term in active, f'Combo_MM_GearActive lacks {term}')
check('kMmGearIntegrationComplete' in body(benport, 'static int Combo_MM_GearIntegrationEnabled(void)'),
      'the gear runtime does not follow its release switch')
check('returnCombo_MM_IsAdult()&&Combo_MM_AdultResourcesReady();' in body(benport, 'extern "C" int Combo_MM_IsLinkAdult(void)'),
      'Combo_MM_IsLinkAdult is not the adult runtime test')
settings = read('mm/2s2h/ComboAdultSettings.h')
check(all(o in body(settings, 'inline bool SaveHasGear(') for o in GEAR), 'SaveHasGear does not read the four gear options')
check('gear' not in body(settings, 'inline bool ConfigureSeedAge(').lower(), 'ConfigureSeedAge still takes the gear')

# Gear-only sites read Combo_MM_GearActive and never the adult state; the boot registers and the tunic colour serve both.
gear_sites = {
    'mm/src/overlays/actors/ovl_player_actor/ComboAdultGearUseNative.h': 1,
    'mm/src/code/ComboAdultHazardTimerNative.h': 2,
    'mm/src/code/ComboAdultSwimmingRestrictionNative.h': 1,
    'mm/src/code/ComboHoverCircleDrawNative.h': 1,
    'mm/2s2h/ComboAdultGearPauseNative.h': 1,
    'mm/2s2h/ComboAdultGearExtraIconNative.h': 1,
    'mm/2s2h/Rando/MiscBehavior/KaleidoItemPage.cpp': 2,
}
for rel, count in gear_sites.items():
    text = read(rel)
    check(text.count('Combo_MM_GearActive()') >= count, f'{rel}: gear gate is not Combo_MM_GearActive()')
    check('adultMm.enabled' not in text and 'Combo_MM_AdultResourcesReady()' not in text, f'{rel} still reads the adult state')
# One trade-slot cycle in a rando save: 2S2H's (KaleidoItemPage.cpp, which offers the gear and the MM spells), not
# OoTMM's L/C-up cycle and extra icon on top of it.
for rel, signature in (('mm/2s2h/ComboAdultGearPauseNative.h', 'extern "C" int Combo_MM_AdultGearPauseTick('),
                       ('mm/2s2h/ComboAdultGearExtraIconNative.h', 'extern "C" void Combo_MM_AdultGearDrawExtraIcon(')):
    check('gSaveContext.fileNum>2||IS_RANDO||!Combo_MM_GearActive())' in body(read(rel), signature),
          f'{rel}: OoTMM\'s trade-slot cycle runs beside 2S2H\'s in a rando save')
player = read('mm/src/overlays/actors/ovl_player_actor/z_player.c')
for signature in ('s32 Combo_MM_GetCustomBoots(Player* player)', 'static s32 Combo_MM_IsGoronOrGoronTunic(Player* player)',
                  's32 func_808430E0(Player* this)'):
    fn = body(player, signature)
    check('Combo_MM_GearActive()' in fn and 'adultMm.enabled' not in fn, f'{signature} is not gated on the gear runtime')
check('Combo_MM_GearActive()&&!Combo_MM_ShouldTakeFloorDamage(' in squash(player), 'Goron Tunic floor damage gate')
check('Combo_MM_ApplyAdultGearInventoryResources(Combo_MM_GearActive());' in squash(player), 'gear item tables gate')
boots = read('mm/src/code/ComboAdultBootsNative.h')
check('if((!gear&&!Combo_MM_IsLinkAdult())||' in squash(boots) and 'isAdult=Combo_MM_IsLinkAdult();' in squash(boots),
      'boot registers are not gated on GearActive() || IsLinkAdult()')
check('consts32boots=gear?gSaveContext.save.shipSaveInfo.adultMm.boots:0;' in squash(boots),
      'saved boots count while the gear runtime is off')
lib = read('mm/src/code/z_player_lib.c')
draw = body(lib, 'void Player_DrawImpl(')
check('(Combo_MM_GearActive()||Combo_MM_IsLinkAdult())' in draw, 'tunic env colour gate')
check('playerForm==PLAYER_FORM_HUMAN&&Combo_MM_GearActive()&&' in draw and 'if(Combo_MM_IsLinkAdult()){' in draw,
      'boot draw gate')

# ---- Load policy: never refuse a save for its adult or gear state; gear never touches the adult state.
prepare = body(benport, 'extern "C" int Combo_MM_AdultPrepareLoadedSave(Save* save)')
check(prepare == squash('extern "C" int Combo_MM_AdultPrepareLoadedSave(Save* save) { if (!save) return 0; '
                        'ComboAdultMM::PrepareLoadedSave(*save, Combo_MM_AdultResourcesReady() != 0); return 1; }'),
      'Combo_MM_AdultPrepareLoadedSave can refuse a save')
check('Combo_MM_AdultValidateLoadedSave' not in benport, 'the refusing adult/gear validation is still there')
policy = body(settings, 'inline void PrepareLoadedSave(')
check('adultRuntimeReady&&' in policy and not any(o in policy for o in GEAR), 'PrepareLoadedSave ties gear to adult state')

# ---- Readiness gate (lead decision 3), as for the MM spells.
check(squash('static constexpr RandoOptionId kComboMmGearOptions[] = { RO_MM_BOOTS_IRON, RO_MM_BOOTS_HOVER, '
             'RO_MM_TUNIC_GORON, RO_MM_TUNIC_ZORA };') in squash(benport), 'kComboMmGearOptions')
hidden = re.search(r'kComboHiddenAdultMmOptions\[\]\s*=\s*\{([^}]*)\}', benport)
check(hidden is not None and not any(o in hidden.group(1) for o in GEAR), 'gear is still hidden with the adult options')
# The headless generator follows the release switch too, so reverting the switch commit stops gear seeds everywhere.
check('returnCombo_MM_GearIntegrationEnabled()&&(sMmGearHeadlessGeneration||Combo_MM_GearResourcesReady());'
      in body(benport, 'extern "C" int Combo_MM_GearOptionReady(void)'), 'Combo_MM_GearOptionReady')
# A seed that adds gear this device cannot run is not loaded and no file is made from it (the new MM save would drop
# the gear its placement may need); loading an existing save is never refused.
check('returnCombo_MM_GearOptionsAllowed(swimShuffled);'
      in body(benport, 'extern "C" COMBO_EXPORT int MM_GearOptionsAllowed(int swimShuffled)'), 'MM_GearOptionsAllowed export')
combo = read('combo/ComboShip.cpp')
check('MM_GearOptionsAllowed=(int(*)(int))GetSym(mmModule,"MM_GearOptionsAllowed");' in squash(combo),
      'the combo layer does not bind MM_GearOptionsAllowed')
needs = body(combo, 'static bool SeedNeedsUnavailableMmGear(const nlohmann::json& seed)')
check('ComboRando::ReadGearRules(seed.value("gearRules",nlohmann::json()),gear);' in needs
      and '!(MM_GearOptionsAllowed&&MM_GearOptionsAllowed(0))' in needs, 'SeedNeedsUnavailableMmGear')
reload = body(combo, 'static int Combo_OnReloadRequestImpl(')
check('if(SeedNeedsUnavailableMmGear(j)){' in reload
      and reload.index('if(SeedNeedsUnavailableMmGear(j)){') < reload.index('RestoreGameplayRules('),
      'a seed with gear this device cannot run is loaded')
create = body(combo, 'static void Combo_OnOOTSaveInit(int fileNum)')
check('if(!seed.is_null()&&SeedNeedsUnavailableMmGear(seed)){' in create
      and create.index('SeedNeedsUnavailableMmGear(seed)') < create.index('MM_InitRandoSaveFile('),
      'a file is made from a seed with gear this device cannot run')
zero = 'if(!Combo_MM_GearOptionReady())for(constautoid:kComboMmGearOptions)'
# Option gathering also leaves the gear out of Swim-shuffled seeds (mm_gear_swim_shuffle_tests.py); a new MM save keeps
# its seed's gear wherever the device can run it.
allowed = 'if(!Combo_MM_GearOptionsAllowed('
check(allowed + 'options[RO_SHUFFLE_SWIM]==RO_GENERIC_YES))for(constautoid:kComboMmGearOptions)options[id]=0;'
      in body(benport, 'extern "C" void Combo_MM_ClearUnavailableOptions('), 'generation keeps unready gear')
check(zero + 'options[Rando::StaticData::Options[id].name]=uint32_t(0);'
      in body(benport, 'extern "C" COMBO_EXPORT int MM_InitRandoSaveFile('), 'a new MM save keeps unready gear')
check(allowed + 'CVarGetInteger(swim.cvar,swim.defaultValue)==RO_GENERIC_YES))for(constautoid:kComboMmGearOptions)'
      'j[Rando::StaticData::Options[id].cvar]=0;' in body(benport, 'extern "C" COMBO_EXPORT const char* MM_DumpRandoSettings('),
      'the settings snapshot keeps unready gear')
check('returnCombo_MM_GearOptionReady()&&!swimShuffled;' in body(benport, 'extern "C" int Combo_MM_GearOptionsAllowed('),
      'Combo_MM_GearOptionsAllowed does not include the readiness gate')
check('Combo_MM_GearForHeadlessGeneration();' in body(benport, 'extern "C" COMBO_EXPORT void MM_InitRandoHeadless('),
      'the headless generator cannot add the gear')
init = body(benport, 'extern "C" COMBO_EXPORT int MM_InitRandoSaveFile(')
bound = init.index('if(sMMSeedFaroreRulesBound)')
for o, field in zip(GEAR, ('bootsIronMm', 'bootsHoverMm', 'tunicGoronMm', 'tunicZoraMm')):
    check(f'options[Rando::StaticData::Options[{o}].name]=uint32_t(sMMSeedGearRules.{field});' in init[bound:init.index(zero)],
          f'the new MM save does not take {o} from the seed before the readiness gate')
check('ComboRando::ReadGearRules(seed.value("gearRules",nlohmann::json()),sMMSeedGearRules);' in squash(benport),
      'MM_LoadComboRando does not read gearRules')
check('consolidated["gearRules"]=ComboRando::GearRulesFromMmDump(' in squash(read('combo/ComboShip.cpp')),
      'the consolidated spoiler does not carry gearRules')

# ---- Menu, profile layout, CVars, shared items, Save Editor (OoTMM data.ts:2024-2054, 3024-3054).
SETTINGS = [('bootsIronMm', 'RO_MM_BOOTS_IRON', "Iron Boots (MM)", "Adds Iron Boots in Majora's Mask."),
            ('bootsHoverMm', 'RO_MM_BOOTS_HOVER', "Hover Boots (MM)", "Adds Hover Boots in Majora's Mask."),
            ('tunicGoronMm', 'RO_MM_TUNIC_GORON', "Goron Tunic (MM)", "Adds the Goron Tunic in Majora's Mask."),
            ('tunicZoraMm', 'RO_MM_TUNIC_ZORA', "Zora Tunic (MM)", "Adds the Zora Tunic in Majora's Mask.")]
SHARED = [('sharedBootsIron', 'bootsIron', 'Shared Iron Boots', 'Combines the Iron Boots from OoT and MM into one item for both games'),
          ('sharedBootsHover', 'bootsHover', 'Shared Hover Boots', 'Combines the Hover Boots from OoT and MM into one item for both games'),
          ('sharedTunicGoron', 'tunicGoron', 'Shared Goron Tunic', 'Combines the Goron Tunics from OoT and MM into two items for both games'),
          ('sharedTunicZora', 'tunicZora', 'Shared Zora Tunic', 'Combines the Zora Tunics from OoT and MM into two items for both games')]
menu = read('mm/2s2h/Rando/Menu.cpp')
layout = read('mm/2s2h/ProfileMmLayout.inc')
options = read('mm/2s2h/Rando/StaticData/Options.cpp')
positions = [menu.find('SpellPoolCheckbox("Nayru\'s Love (MM)"')]
for key, ro, name, desc in SETTINGS:
    row = f'GearPoolCheckbox("{name}", {ro}, "{desc}");'
    check(row in menu, f'Menu.cpp lacks {row}')
    positions.append(menu.find(row))
    check(f'{{"label","{name}"}},{{"kind","checkbox"}},{{"tooltip","{desc}"}},{{"key",Rando::StaticData::Options.at({ro}).cvar}}' in layout,
          f'ProfileMmLayout.inc lacks the {name} row')
    check(f'RO({ro}, RO_GENERIC_NO)' in options, f'{ro} is not released with a CVar')
check(-1 not in positions and positions == sorted(positions), 'gear rows are not after Nayru\'s Love in OoTMM order')
check('if(Combo_MM_GearOptionsAllowed(CVarGetInteger(swim.cvar,swim.defaultValue)==RO_GENERIC_YES)){'
      'ItemPoolCheckbox(label,optionId,tooltip,1);return;}'
      in body(menu, 'static void GearPoolCheckbox('), 'the gear rows are not greyed out where the gear cannot run')
shared = read('combo/rando/SharedItems.h')
for key, family, name, desc in SHARED:
    m = re.search(r'\{\s*SF_\w+,\s*"' + family + r'",\s*"([^"]*)",\s*"([^"]*)",\s*"([^"]*)",([^}]*)\}', shared)
    check(m is not None and m.group(2) == name and m.group(3) == desc, f'{family} shared row text')
    check(m is not None and m.group(4).split(',')[-1].strip() == 'true', f'{family} is not public')
check('if(i==SF_OCARINA||!SharedFamilyByIndex(i).publicOption)' in squash(read('combo/gui/ComboMenu.cpp'))
      and 'constchar*extension=SharedFamilyMmExtension(i);' in squash(read('combo/gui/ComboMenu.cpp')),
      'the Shared items page no longer greys a family out without its MM option')
editor = squash(read('mm/2s2h/DeveloperTools/SaveEditor.cpp'))
for item, family in (('BOOTS_IRON', 'BOOTS_IRON'), ('BOOTS_HOVER', 'BOOTS_HOVER'), ('TUNIC_GORON', 'TUNIC_GORON'),
                     ('TUNIC_ZORA', 'TUNIC_ZORA')):
    check(f'caseITEM_COMBO_{item}:returnComboRando::SharedFamilyByIndex(ComboRando::SF_{family}).publicOption&&Combo_MM_GearOptionReady();'
          in editor, f'Save Editor availability of ITEM_COMBO_{item}')

# ---- Tracker rows (OoTMM trade.c kMmTrade2/3) and icons.
tracker_settings = squash(read('mm/2s2h/Enhancements/Trackers/ItemTracker/ItemTrackerSettings.cpp'))
check('{TRACKER_ITEM_RANDO,RI_FARORES_WIND},{TRACKER_ITEM_RANDO,RI_BOOTS_IRON},{TRACKER_ITEM_RANDO,RI_TUNIC_GORON},'
      in tracker_settings, 'TRADE2 gear rows')
check('{TRACKER_ITEM_RANDO,RI_NAYRUS_LOVE},{TRACKER_ITEM_RANDO,RI_BOOTS_HOVER},{TRACKER_ITEM_RANDO,RI_TUNIC_ZORA},'
      in tracker_settings, 'TRADE3 gear rows')
tracker = squash(read('mm/2s2h/Enhancements/Trackers/ItemTracker/ItemTracker.cpp'))
for ri, ro in (('RI_BOOTS_IRON', 'RO_MM_BOOTS_IRON'), ('RI_BOOTS_HOVER', 'RO_MM_BOOTS_HOVER'),
               ('RI_TUNIC_GORON', 'RO_MM_TUNIC_GORON'), ('RI_TUNIC_ZORA', 'RO_MM_TUNIC_ZORA')):
    check(f'if(itemId=={ri})return!IS_RANDO||!RANDO_SAVE_OPTIONS[{ro}];' in tracker, f'{ri} row is not hidden without {ro}')
check('caseRI_BOOTS_IRON:caseRI_TUNIC_GORON:returnComboAdultGearInventory::Has(gSaveContext.save.shipSaveInfo.adultMm.gearTrade2Bits,'
      in tracker and 'caseRI_BOOTS_HOVER:caseRI_TUNIC_ZORA:returnComboAdultGearInventory::Has(gSaveContext.save.shipSaveInfo.adultMm.gearTrade3Bits,'
      in tracker, 'gear rows do not light from the gear ownership bits')
inventory = read('mm/src/code/z_inventory.c')
for item in ('BOOTS_IRON', 'BOOTS_HOVER', 'TUNIC_GORON', 'TUNIC_ZORA'):
    check(re.search(r'\[ITEM_COMBO_' + item + r'\]\s*=\s*\(TexturePtr\)\w+', inventory) is not None,
          f'ITEM_COMBO_{item} has no gItemIcons entry')

# ---- Child boot textures: every texture the child boot lists load is one the readiness check validates.
child = read('mm/src/code/ComboChildBootDrawNative.h')
loads = set(re.findall(r'"__OTR__@oot:objects/object_link_boy/(\w+)"', child))
resources = read('mm/2s2h/ComboAdultBootDrawResources.h')
validated = re.search(r'LoadChildBootTextures\(\)[\s\S]*?names\[\]\s*=\s*\{([^}]*)\}', resources)
validated = set(re.findall(r'"objects/object_link_boy/(\w+)"', validated.group(1))) if validated else set()
check(len(loads) == 5 and loads <= validated, f'child boot textures {sorted(loads - validated)} are not validated')
check({'gLinkAdultIronBootTex', 'gLinkAdultHoverBootsHeelTex', 'gLinkAdultHoverBootsJetTex',
       'gLinkAdultHoverBootsFeatherTex'} <= loads, 'child boots do not use OoTMM\'s OoT texture names (Player.c:873-950)')

# ---- Lily pads (OoTMM Bg_Lotus.c:4-8) and the C-button guard (Player.S:353-380).
lotus = squash(read('mm/src/overlays/actors/ovl_Bg_Lotus/z_bg_lotus.c'))
check('if((gSaveContext.save.playerForm!=PLAYER_FORM_DEKU)&&(Combo_MM_GetCustomBoots(GET_PLAYER(play))!=COMBO_LOTUS_HOVER_BOOTS)){'
      in lotus and '#defineCOMBO_LOTUS_HOVER_BOOTS2' in lotus, 'lily pads sink under Hover Boots')
# Swim shuffle: human Link in the MM Iron Boots sinks instead of being sent back (OoTMM common/bronze_scale.c:63-66),
# which the logic's can_swim_or_sink (macros_mm.yml:84) relies on.
swim = read('mm/2s2h/Rando/ActorBehavior/Player.cpp')
swim = squash(swim[swim.index('void RespawnOnWaterTouch(Player* player)'):swim.index('void Rando::ActorBehavior::InitPlayerBehavior()')])
sink = 'if(player->transformation==PLAYER_FORM_HUMAN&&player->currentBoots==PLAYER_BOOTS_GORON){return;}'
check(sink in swim and swim.index(sink) < swim.index('if(player->stateFlags1&PLAYER_STATE1_8000000)'),
      'the Swim shuffle sends human Link in the MM Iron Boots back from deep water')
check('player->currentBoots=6;/*PLAYER_BOOTS_GORON*/' in squash(read('mm/src/code/ComboAdultBootsNative.h')),
      'the MM Iron Boots no longer set currentBoots to PLAYER_BOOTS_GORON')
use = squash(read('mm/src/overlays/actors/ovl_player_actor/ComboAdultGearUseNative.h'))
check('if(player->heldItemAction!=player->itemAction||(player->stateFlags1&PLAYER_STATE1_400000))return1;' in use,
      'gear toggles outside OoTMM\'s Player_UseItem guard')

# ---- Tunic colour: the patched combiners are the ones just before 2S2H's own tunic cosmetic patches.
tunic = read('mm/src/code/ComboGearTunicColorNative.h')
dls = re.findall(r'"objects/object_link_child/(\w+)"', tunic[tunic.index('sComboGearTunicDLs'):tunic.index('sComboGearTunicIndices')])
idx = [int(x) for x in re.findall(r'\d+', tunic[tunic.index('sComboGearTunicIndices[] = {'):].split('}')[0].split('{')[1])]
cosmetic = read('mm/2s2h/BenGui/CosmeticEditor.cpp')
cosmetic = cosmetic[cosmetic.index('static RegisterShipInitFunc humanTunicPatch('):]
cosmetic = cosmetic[:cosmetic.index('} else {')]
setprim = re.findall(r'"objects/object_link_child/(\w+)",\s*"setPrim\d?",\s*(\d+)', cosmetic)
check(len(dls) == len(idx) == 11 and sorted(zip(dls, idx)) == sorted((d, int(i) - 1) for d, i in setprim),
      'tunic combiner sites are not the commands before 2S2H\'s "setPrim" patches')
check(squash('gsDPSetCombineLERP(TEXEL0, 0, ENVIRONMENT, 0, 0, 0, 0, 1, COMBINED, 0, SHADE, 0, 0, 0, 0, 1)') in squash(tunic),
      'tunic combiner is not OoTMM\'s FC12FE04 FFFFFDFE')
check('(u32)cmd->words.w0==0xFC11FE04&&(u32)cmd->words.w1==0xFFFFF3F8' in squash(tunic), 'tunic patch skips the native check')
check('Combo_MM_SyncGearTunicColor(Combo_MM_GearActive()?gSaveContext.save.shipSaveInfo.adultMm.tunic:0);' in draw,
      'Player_DrawImpl does not sync the gear tunic colour')

# ---- Against OoTMM v32.3 itself.
reference = a.reference
if reference is None:
    for candidate in (root.parent.parent / 'references/ootmm-stable-32.3', root.parent / 'references/ootmm-stable-32.3'):
        if (candidate / 'packages/generator/src/mm/actors/Player.c').exists():
            reference = candidate
            break
if reference is not None:
    ootmm = (reference / 'packages/generator/src/mm/actors/Player.c').read_text(encoding='utf-8')

    def array(source, name):
        m = re.search(r'\b' + re.escape(name) + r'\b[^=;]*=\s*\{', source)
        if not m:
            return None
        depth, i = 0, m.end() - 1
        while True:
            depth += (source[i] == '{') - (source[i] == '}')
            i += 1
            if depth == 0:
                return re.sub(r'/\*.*?\*/', '', source[m.end() - 1:i], flags=re.S)

    def numbers(text):
        return [int(x) for x in re.findall(r'-?\d+', text or '')]

    check(numbers(array(boots, 'sComboAdultBootData')) == numbers(array(ootmm, 'sBootData')),
          'boot data differs from Player.c:593-614')
    palette = read('mm/src/code/ComboAdultTunicColorsNative.h')
    check(numbers(array(palette, 'sComboAdultTunicColors[4]')) == numbers(array(ootmm, 'sTunicColors[4]')),
          'tunic colours differ from Player.c:1244-1249')
    rename = {'gComboChildIronBootTex': 'gLinkAdultIronBootTex', 'gComboChildIronBootTLUT': 'gLinkAdultIronBootTLUT',
              'sComboChildIronBootsVtx': 'sIronBootsVtx', 'gComboChildHoverHeelTex': 'gLinkAdultHoverBootsHeelTex',
              'gComboChildHoverJetTex': 'gLinkAdultHoverBootsJetTex',
              'gComboChildHoverFeatherTex': 'gLinkAdultHoverBootsFeatherTex', 'sComboChildHoverBootsVtx': 'sHoverBootsVtx',
              'gComboHoverCircleTex': 'gEffUnknown13Tex', 'gComboHoverCircleDetailTex': 'gEffUnknown14Tex'}

    def native(dl):
        dl = squash(dl or '').replace('|1,', ',').replace('|1)', ')')
        for fork, name in rename.items():
            dl = dl.replace(fork, name)
        return dl

    for fork, name in (('gComboChildLeftIronBootDL', 'gLinkAdultLeftIronBootDL'),
                       ('gComboChildRightIronBootDL', 'gLinkAdultRightIronBootDL'),
                       ('gComboChildLeftHoverBootDL', 'gLinkAdultLeftHoverBootDL'),
                       ('gComboChildRightHoverBootDL', 'gLinkAdultRightHoverBootDL')):
        check(array(ootmm, name) and native(array(child, fork)) == squash(array(ootmm, name)), f'{fork} differs from {name}')
    circle = read('mm/src/code/ComboHoverCircleDrawNative.h')
    check(native(array(circle, 'gComboHoverBootsCircleDL')) == squash(array(ootmm, 'gHoverBootsCircleDL')),
          'hover circle display list differs from Player.c:1069-1087')
    for vtx in ('sIronBootsVtx.vtx.inc', 'sHoverBootsVtx.vtx.inc', 'sHoverBootsCircleVtx.vtx.inc'):
        ours = (root / 'mm/src/code' / vtx).read_text(encoding='utf-8')
        theirs = (reference / 'packages/generator/src/mm/actors' / vtx).read_text(encoding='utf-8')
        check(squash(ours) == squash(theirs), f'{vtx} differs from OoTMM')
    scale_ref = squash((reference / 'packages/generator/src/common/bronze_scale.c').read_text(encoding='utf-8'))
    check('#ifdefined(GAME_MM)if(this->currentBoots==6||this->currentBoots==3)return;#endif' in scale_ref,
          'OoTMM bronze_scale.c no longer skips the MM Iron Boots')
    lotus_ref = (reference / 'packages/generator/src/mm/actors/Bg/Bg_Lotus.c').read_text(encoding='utf-8')
    check('GET_PLAYER_CUSTOM_BOOTS(player) != PLAYER_BOOTS_HOVER' in lotus_ref, 'OoTMM Bg_Lotus.c changed')
    player_s = (reference / 'packages/generator/src/mm/actors/Player.S').read_text(encoding='utf-8')
    env = re.findall(r'PATCH_VROM (0x[0-9A-F]+)\s*\.int 0xFC12FE04\s*\.int 0xFFFFFDFE', player_s)
    check(len(env) == 11, f'OoTMM patches {len(env)} tunic combiners, not 11')
    data = (reference / 'packages/core/src/settings/data.ts').read_text(encoding='utf-8')

    def setting(key):
        block = data[data.index(f"key: '{key}'"):]
        block = block[:block.index('}, {')]
        name = re.search(r'name: ("|\')(.*?)\1,', block).group(2)
        desc = re.search(r"description: ('|\")(.*?)(?<!\\)\1,", block).group(2).replace("\\'", "'")
        return name, desc

    for key, ro, name, desc in SETTINGS:
        check(setting(key) == (name, desc), f'{key} text differs from data.ts')
    for key, family, name, desc in SHARED:
        check(setting(key) == (name, desc), f'{key} text differs from data.ts')
    print(f'OoTMM reference checked: {reference}')
else:
    print('OoTMM reference not found; checked the production sources only')

if failures:
    print('\n'.join(failures))
    sys.exit(1)
print('MM gear: runtime gates, load policy, readiness gate, menus, tracker, textures and OoTMM parity checked')
