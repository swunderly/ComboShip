"""Translate pinned OoTMM combat predicates; reject unmapped capabilities.

This imports the expressions, rather than maintaining a second hand-written set.
Native soul gates remain in CanKillEnemy. Foreign equipment which the MM engine
cannot use is deliberately false. Native context-specific actors stay separate.
"""
from pathlib import Path
import argparse, hashlib, json, re
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--reference',type=Path,required=True,help='Pinned OoTMM v32.3 checkout')
p.add_argument('--output',type=Path,required=True,help='Generated MmCombatRules.h')
p.add_argument('--actor-map',type=Path,help='Optional actor/predicate JSON for comparison tools')
a=p.parse_args()
reference=a.reference.resolve()
source=reference/'data/macros/macros_mm.yml'
assert hashlib.sha256(source.read_bytes()).hexdigest()=='c0e1d62e704563649d44ac11ad719fa22846163be8c6fcb242365c8880d933a6', 'Pinned combat source changed'
assert hashlib.sha256((reference/'LICENSE').read_bytes()).hexdigest()=='77618606f27a6b5f6d28028a534555da54954e942917e38ad43252af8d206765', 'Pinned license changed'
macros={}
for line in source.read_text().splitlines():
 match=re.match(r'^"([^"]+)":\s*("(?:[^"\\]|\\.)*")',line)
 if match: macros[match[1]]=json.loads(match[2])
actors={
 'ACTOR_BOSS_01':'can_kill_odolwa','ACTOR_BOSS_02':'can_kill_twinmold',
 'ACTOR_BOSS_03':'can_kill_gyorg','ACTOR_BOSS_04':'can_kill_warts',
 'ACTOR_BOSS_HAKUGIN':'can_kill_goht','ACTOR_EN_KNIGHT':'can_kill_igos',
 'ACTOR_EN_KAIZOKU':'can_fight_pirates','ACTOR_EN_PAMETFROG':'can_kill_gekko_wft',
 'ACTOR_EN_BIGSLIME':'can_kill_gekko_gbt','ACTOR_EN_DINOFOS':'can_kill_lizalfos_dinolfos',
 'ACTOR_EN_WIZ':'can_kill_wizzrobe_close','ACTOR_EN_WF':'can_kill_wolfos',
 'ACTOR_EN_JSO':'can_kill_garo','ACTOR_EN_JSO2':'can_kill_garo_master',
 'ACTOR_EN_IK':'can_kill_iron_knuckle','ACTOR_EN_GRASSHOPPER':'can_kill_dragonflies',
 'ACTOR_EN_MKK':'can_kill_boes','ACTOR_EN_BIGPAMET':'can_kill_snappers_flowers',
 'ACTOR_EN_ST':'can_kill_skulltulas','ACTOR_EN_BAT':'can_kill_bad_bats_single',
 'ACTOR_EN_DEKUBABA':'can_kill_baba_nuts','ACTOR_EN_AM':'can_kill_armos',
 'ACTOR_EN_VM':'can_kill_beamos','ACTOR_EN_BB':'can_kill_bubbles',
 'ACTOR_EN_BBFALL':'can_kill_bubbles','ACTOR_EN_FAMOS':'can_kill_eyegore_distance',
 'ACTOR_EN_DODONGO':'can_kill_dodongos','ACTOR_EN_FLOORMAS':'can_kill_floormasters',
 'ACTOR_EN_WALLMAS':'can_kill_wallmasters','ACTOR_EN_FZ':'can_kill_freezard_short_range',
 'ACTOR_EN_CROW':'can_kill_guays','ACTOR_EN_FIREFLY':'can_kill_keese_close',
 'ACTOR_EN_RR':'can_kill_like_like','ACTOR_EN_DEKUNUTS':'can_kill_scrubs_anywhere',
 'ACTOR_EN_PEEHAT':'can_kill_peahats','ACTOR_EN_RD':'can_kill_redead_gibdo',
 'ACTOR_EN_BSB':'can_kill_keeta','ACTOR_EN_SKB':'can_kill_stalchildren',
 'ACTOR_EN_TITE':'can_kill_tektites','ACTOR_EN_SLIME':'can_kill_chuchu',
 'ACTOR_EN_SNOWMAN':'can_kill_eenos','ACTOR_EN_WDHAND':'can_kill_dexihands',
 'ACTOR_EN_KAME':'can_kill_snappers_any','ACTOR_EN_OKUTA':'can_kill_octoroks',
 'ACTOR_EN_EGOL':'can_kill_eyegore','ACTOR_EN_BAGUO':'can_kill_nejirons',
 'ACTOR_EN_NEO_REEBA':'can_kill_leevers','ACTOR_EN_PP':'can_kill_hiploops',
 'ACTOR_EN_PR':'can_kill_skullfish','ACTOR_EN_PR2':'can_kill_skullfish',
 'ACTOR_EN_SB':'can_kill_shell_blades_underwater','ACTOR_BOSS_05':'can_kill_bio_babas',
 'ACTOR_EN_BEE':'can_kill_wasp','ACTOR_EN_DRAGON':'can_kill_deep_python',
 'ACTOR_EN_BIGPO':'can_kill_big_poe','ACTOR_EN_PO_SISTERS':'can_kill_poe_sisters',
 'ACTOR_EN_INVADEPOH':'can_kill_aliens','ACTOR_EN_THIEFBIRD':'can_kill_takkuri',
 'ACTOR_EN_DEATH':'can_kill_gomess',
}
# Unused v32.3 macros with undefined identifiers are not valid import sources.
invalid_macros={'can_kill_wasp', 'can_kill_takkuri', 'can_kill_hiploops', 'can_kill_skullfish', 'can_kill_redead_gibdo', 'can_kill_iron_knuckle', 'can_kill_nejirons', 'can_kill_eenos'}
actors={actor:macro for actor,macro in actors.items() if macro not in invalid_macros}
aliases={
 'has_weapon':'c.sword','has_sword':'c.sword','can_fight':'(c.sword || c.zora || c.goron)',
 'has_mask_zora':'c.zora','has_mask_goron':'c.goron','has_arrows':'c.arrows',
 'can_hookshot_short':'c.hookshot','can_hookshot':'c.hookshot',
 'can_use_deku_bubble':'(c.deku && c.magic)','has_explosives':'c.explosives',
 'can_use_light_arrows':'c.lightArrows','can_use_fire_arrows':'c.fireArrows',
 'can_use_ice_arrows':'c.iceArrows','can_use_fire_short_range':'c.fireArrows',
 'has_magic':'c.magic','has_nuts':'c.nuts','deku_stick_fighting':'c.stickFighting',
 'has_mirror_shield':'c.mirrorShield','can_swim':'c.swim','is_day':'c.day',
 'can_use_slingshot':'false','has_boomerang':'false','can_hammer':'false',
 'can_use_din':'false','has_iron_boots':'false','has_tunic_zora_strict':'false',
}
items={'MASK_DEKU':'deku','MASK_FIERCE_DEITY':'deity','MASK_GIANT':'giant'}
tricks={'MM_TWINMOLD_BOW':'twinmoldBow','MM_TWINMOLD_FIRE_AND_ICE':'twinmoldFireIce','MM_GYORG_IRONS':None,'MM_IGOS_DINS':None}
def translate(expr):
 expr=re.sub(r'\bsoul_\w+(?:\([A-Z_]+\))?', 'true',expr)
 expr=re.sub(r'\bhas\(([A-Z_]+)\)',lambda m:'c.'+items[m[1]],expr)
 expr=re.sub(r'\btrick\(([A-Z_]+)\)',lambda m:'c.'+tricks[m[1]] if tricks[m[1]] else 'false',expr)
 def identifier(m):
  token=m[0]
  if token in ('true','false') or m.start()>0 and expr[m.start()-1]=='.' or token=='c':return token
  if token not in aliases:raise ValueError('Unmapped capability '+token)
  return aliases[token]
 return re.sub(r'\b[a-zA-Z_][a-zA-Z_0-9]*\b',identifier,expr)
license=(reference/'LICENSE').read_text()
names=sorted(set(actors.values()))
header='// Generated by ComboAndroid/tools/import_mm_combat.py from OoTMM v32.3.\n'
header+='// Source: data/macros/macros_mm.yml, commit 413573d22d1d53e8a81931a70927f625b73488b9.\n'
header+='/*\n'+license+'*/\n#pragma once\nnamespace ComboRando {\n'
header+='// Combat requirements only; the caller enforces enemy and boss souls.\n'
header+='struct MmCombatCapabilities {\n    bool sword, deku, goron, zora, deity, giant, magic, arrows, hookshot;\n    bool explosives, nuts, stickFighting, fireArrows, iceArrows, lightArrows;\n    bool mirrorShield, swim, day, twinmoldBow, twinmoldFireIce;\n};\n'
for name in names:
 header+=f'inline bool {name}(const MmCombatCapabilities& c) {{\n    return {translate(macros[name])};\n}}\n'
header+='} // namespace ComboRando\n'
a.output.parent.mkdir(parents=True,exist_ok=True)
a.output.write_text(header,encoding='utf-8')
if a.actor_map:
 a.actor_map.write_text(json.dumps(actors,indent=2),encoding='utf-8')
print('Imported',len(names),'predicates for',len(actors),'actor contexts')
