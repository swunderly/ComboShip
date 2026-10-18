"""OoT Enemy Souls (OoTMM soulsEnemyOot) against OoTMM v32.3.

Checks the production soul policy, save state, published-family switch and token copy by compiling them
with fixture-driven cases, then the item names, hint keys, settings, runtime hooks and logic edits by
source scan, and finally the enemy-gate receipt (souls/upstream-enemy-gates.json and
native-enemy-gate-map.json): every OoTMM enemy-soul gate is matched by a native gate requiring the same
souls, recorded as a deliberately stricter SoH rule, or recorded as having no native location, and
every fixed gate's evidence statement really carries its souls. It also checks that OoT's broken
room-list actors (OoTMM kMqBrokenActors, e.g. MQ Spirit Temple room 1's two Keese) can never keep a room
shut, that a 0.3.10 save reads every soul family as off, and that a seed's souls value never depends on
this machine's Enemy Randomizer. When the pinned OoTMM checkout is present
(references/ootmm-stable-32.3 beside the repository) the fixtures are re-derived from it.
"""
from pathlib import Path
import argparse
import hashlib
import json
import os
import re
import struct
import subprocess
import hostcxx

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
p.add_argument('--json-include', type=Path, required=True)
a = p.parse_args()
root = a.source.resolve()
rando = root/'soh/soh/Enhancements/randomizer'
souls = rando/'souls'
work = a.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
fixture = json.loads((Path(__file__).resolve().parent/'fixtures/ootmm-32.3-enemy-souls.json').read_text(encoding='utf-8'))
upstream = json.loads((souls/'upstream-enemy-gates.json').read_text(encoding='utf-8'))
receipt = json.loads((souls/'native-enemy-gate-map.json').read_text(encoding='utf-8'))
candidates = [Path(os.environ['OOTMM_REFERENCE'])] if os.environ.get('OOTMM_REFERENCE') else []
candidates += [root.parent/'references/ootmm-stable-32.3', root.parent.parent/'references/ootmm-stable-32.3']
reference = next((c for c in candidates if (c/'data/defs/gi.yml').is_file()), None)


def read(path):
    return Path(path).read_text(encoding='utf-8')


def balanced(text, opening):
    """Text from the bracket at opening to its match, inclusive."""
    pairs = {'(': ')', '{': '}'}
    close, depth = pairs[text[opening]], 0
    for i in range(opening, len(text)):
        depth += (text[i] == text[opening]) - (text[i] == close)
        if depth == 0:
            return text[opening:i + 1]
    raise AssertionError('unbalanced')


def function_body(text, signature):
    start = hostcxx.find(text, signature)
    return balanced(text, text.index('{', start))


def enum_ids(name):
    return re.findall(r'RANDO_ENUM_ITEM\((\w+)\)', read(rando/'randomizerEnums'/name))


def table(name, kind):
    """SoH's numbered actor or object table as {id: enum name}."""
    return {int(number, 16): enum for number, enum in re.findall(
        r'/\* (0x[0-9A-F]+) \*/ DEFINE_%s\w*\((?:\w+, )?(%s_\w+)' % (kind, kind), read(root/'soh/include/tables'/name))}


def actor_object(actor):
    """The object an actor's InitVars ask for, which Actor_Spawn looks up in the room's object list."""
    for f in (root/'soh/src/overlays/actors').rglob('z_*.c'):
        found = re.search(r'ActorInit \w+_InitVars = \{\s*%s,\s*ACTORCAT_\w+,\s*[\w|() ]+,\s*(OBJECT_\w+)' % actor, read(f))
        if found:
            return found.group(1)
    raise AssertionError(('no InitVars', actor))


# --- The pinned OoTMM sources, when present ---------------------------------------------------------
if reference:
    for path, source in {**fixture['sources'], **upstream['sources']}.items():
        digest = hashlib.sha256((reference/path).read_bytes()).hexdigest()
        assert digest == source['sha256'], ('OoTMM source differs from the pinned v32.3 copy', path)
    common = read(reference/'data/macros/macros_common.yml')
    macros = re.findall(r'^"(soul_\w+)": "shared_soul_enemy\(', common, re.M)
    assert macros == upstream['enemyMacros']
    scan = re.compile(r'soul_enemy\(SOUL_ENEMY_\w+\)|\b(?:' + '|'.join(macros) +
                      r')\b|can_kill_baba_\w+|can_rescue_carpenter|can_kill_twinrova')
    direct = set()
    for f in sorted((reference/'data/world/oot').rglob('*.yml')):
        rel = f.relative_to(reference).as_posix()
        hits = [i for i, line in enumerate(read(f).splitlines(), 1)
                if not line.lstrip().startswith('#') and scan.search(line.split(' #')[0])]
        assert len(hits) == upstream['sources'].get(rel, {'directLines': 0})['directLines'], rel
        direct |= {rel + ':' + str(i) for i in hits}
    assert direct == {g['ootmm'] for g in upstream['gates'] if g['direct']}
    gi = dict(re.findall(r'id: OOT_SOUL_ENEMY_(\w+), .*name: "the <C1>([^"]+)"', read(reference/'data/defs/gi.yml')))
    assert gi == {s['soul'][len('ENEMY_'):]: s['giName'] for s in fixture['souls']}
    # OoTMM's MQ broken actors, decoded from its packed MQ room data with SoH's actor and object ids.
    mq = read(reference/'packages/generator/src/oot/mq.c')
    listed = mq[mq.index('kMqBrokenActors[] = {'):]
    listed = [tuple(int(x, 16) for x in t) for t in
              re.findall(r'\{ (0x[0-9a-f]+), (0x[0-9a-f]+), (0x[0-9a-f]+) \}', listed[:listed.index('};')])]
    mq_ids = {int(v): k for k, v in re.findall(r'#define (MQ_\w+)\s+(\d+)',
                                               read(reference/'packages/generator/include/combo/dungeon.h'))}
    rooms = (reference/'packages/generator/data/static/mq_rooms.bin').read_bytes()
    actors, objects = table('actor_table.h', 'ACTOR'), table('object_table.h', 'OBJECT')
    decoded = []
    for dungeon, room, index in listed:
        for i in range(1, struct.unpack('>I', rooms[:4])[0] + 1):
            size, offset, d, r, _, actor_at, object_count, object_at = struct.unpack(
                '>IIBBHHHH', rooms[0x20 * i:0x20 * i + 18])
            if (d, r) == (dungeon, room):
                data = rooms[offset:offset + size]
                entry = data[actor_at + 0x10 * index:actor_at + 0x10 * index + 0x10]
                decoded.append((mq_ids[dungeon], room, index, actors[struct.unpack('>h', entry[:2])[0]],
                                struct.unpack('>H', entry[14:])[0],
                                [objects[struct.unpack('>H', data[object_at + 2 * k:object_at + 2 * k + 2])[0]]
                                 for k in range(object_count)]))
    assert decoded == [(e['mqDungeon'], e['room'], e['actorIndex'], e['actor'], e['params'], e['roomObjects'])
                       for e in fixture['mqBrokenActors']['entries']]
    print('PASS: fixtures re-derived from the pinned OoTMM v32.3 checkout')
else:
    print('NOTE: no OoTMM v32.3 checkout beside the repository; using the pinned fixtures only')

# --- Names, in-game nouns, item and hint-key order ----------------------------------------------------
rows = re.findall(r'OOT_SOUL_ITEM\((RG_SOUL_(\w+)), (\d+), OotSoul::(\w+), "([^"]*)", "([^"]*)"\)',
                  read(souls/'OotSoulData.inc'))
assert len(rows) == 95
for i, (rg, tail, index, soul, label, in_game) in enumerate(rows):
    assert int(index) == i and soul == tail, rg
expected = {s['soul']: s for s in fixture['souls']}
enemy_rows = [row for row in rows if row[1].startswith('ENEMY_')]
assert [row[1] for row in rows[:43]] == [row[1] for row in enemy_rows] and set(expected) == {r[1] for r in enemy_rows}
for rg, tail, index, soul, label, in_game in rows:
    if tail in expected:
        ootmm = expected[tail]
        assert label == ootmm['name'], (rg, label, ootmm['name'])
        assert in_game == ootmm['giName'] + (' (OoT)' if ootmm['ambiguous'] else ''), (rg, in_game)
    else:
        assert in_game == label, rg
ids = enum_ids('RandomizerGet.h')
first = ids.index('RG_SOUL_ENEMY_POE')
assert ids[first:first + 95] == [row[0] for row in rows], 'OotSoulData.inc must follow the RandomizerGet order'
hints = enum_ids('RandomizerHintTextKey.h')
assert hints[-1] == 'RHT_MAX' and hints[-44:-1] == ['RHT_SOUL_' + row[1] for row in enemy_rows]
assert len(set(hints)) == len(hints)
core = read(souls/'OotSouls.cpp')
assert hostcxx.contains(core, 'return item >= RG_SOUL_ENEMY_POE && item <= RG_SOUL_MISC_GS ? item - RG_SOUL_ENEMY_POE : -1;')
assert hostcxx.contains(core, 'static_assert(SoulRowsFollowItemOrder());')
register = function_body(core, 'void RegisterItems(std::array<Item, RG_MAX>& items)')
assert 'RHT_NONE,' not in register and hostcxx.contains(register, 'HintKey(row.item), row.item')
assert hostcxx.contains(register, 'enemy ? Text{ "the ", "the ", "the " } : Text{}')
assert hostcxx.contains(read(rando/'3drando/hint_list/hint_list_item.cpp'),
                        'for (int rg = RG_SOUL_ENEMY_POE; rg <= RG_SOUL_ENEMY_THIEVES; ++rg)')
assert hostcxx.contains(read(rando/'Messages/ItemMessages.cpp'), 'Rando::Souls::InGameName(static_cast<RandomizerGet>(rgid))')
assert hostcxx.contains(read(root/'soh/soh/OTRGlobals.cpp'), '{ "displayName", soulName ? std::string(soulName) : name }')
tracker = read(rando/'randomizer_item_tracker.cpp')
assert 'group("OoT Enemy Souls"' in tracker and 'itemName = "Soul of' not in tracker
print('PASS: 43 enemy soul names, in-game nouns and hint keys match OoTMM; 95 rows follow the item order')

# --- Settings -----------------------------------------------------------------------------------------
settings = read(rando/'settings.cpp')
setting = fixture['setting']
# A fresh CVar: the hidden pre-F1 ShuffleEnemySouls could carry a stale 1 in configs, profiles and snapshots.
assert hostcxx.contains(settings, 'OPT_BOOL(RSK_SHUFFLE_ENEMY_SOULS, "%s", CVAR_RANDOMIZER_SETTING("SoulsEnemyOot")'
                        % setting['name'])
assert 'CVAR_RANDOMIZER_SETTING("ShuffleEnemySouls")' not in settings
assert hostcxx.contains(settings, 'OPT_BOOL(RSK_COMPLETE_OOT_SOULS, "");')
assert hostcxx.contains(settings, 'OPT_BOOL(RSK_OOT_SOULS_PUBLISHED, "");')
descriptions = read(rando/'option_descriptions.cpp')
start = hostcxx.find(descriptions, 'mOptionDescriptions[RSK_SHUFFLE_ENEMY_SOULS] =')
literal = descriptions[start:descriptions.index(';', start)]
assert ''.join(re.findall(r'"((?:[^"\\]|\\.)*)"', literal)) == setting['description']


def group(name):
    start = settings.index('mOptionGroups[%s] =' % name)
    return re.findall(r'&mOptions\[(RSK_\w+)\]', balanced(settings, settings.index('(', start)))


assert 'RSK_SHUFFLE_ENEMY_SOULS' in group('RSG_MENU_SECTION_ADDITIONAL_ITEMS')
additional = group('RSG_MENU_SECTION_ADDITIONAL_ITEMS')
assert additional.index('RSK_SHUFFLE_ENEMY_SOULS') + 1 == additional.index('RSK_SHUFFLE_BEAN_SOULS')
assert 'RSK_SHUFFLE_ENEMY_SOULS' in group('RSG_SHUFFLE')
for key in ['RSK_COMPLETE_OOT_SOULS', 'RSK_OOT_SOULS_PUBLISHED', 'RSK_SHUFFLE_NPC_SOULS', 'RSK_SHUFFLE_ANIMAL_SOULS',
            'RSK_SHUFFLE_MISC_SOULS']:
    assert '&mOptions[%s]' % key not in settings, ('unpublished soul key in a menu group', key)
finalize = function_body(settings, 'void Context::FinalizeSettings(')
parse = function_body(settings, 'void Settings::ParseJson(')
assert 'FinalizeOotSoulSettings(' in finalize and 'FinalizeOotSoulSettings(' in parse
# A seed's souls value must not depend on this machine's Enemy Randomizer: a reload runs FinalizeSettings
# again, so the rule lives on the settings side (callback and generation), which the snapshot records.
assert 'RandomizedEnemies' not in finalize and 'RandomizedEnemies' not in parse
callback = balanced(settings, settings.index('{', hostcxx.find(settings, 'OPT_CALLBACK(RSK_SHUFFLE_ENEMY_SOULS,')))
assert 'RandomizedEnemies' in callback and '.Disable("Disabled because Enemy Randomizer is on (Enhancements > Extra modes).")' in callback
assert hostcxx.contains(callback, 'CVarSetInteger(CVAR_RANDOMIZER_SETTING("SoulsEnemyOot"), RO_GENERIC_OFF);')
assert hostcxx.contains(settings, '{ CVAR_ENHANCEMENT("RandomizedEnemies") });')
generation_rules = read(root/'combo/rando/GenerationRules.h')
assert hostcxx.contains(generation_rules, 'kOotEnemySouls = "gRandoSettings.SoulsEnemyOot";')
assert hostcxx.contains(generation_rules, 'kOotEnemyRandomizer = "gEnhancements.RandomizedEnemies";')
generation_sites = 0
for name in ['combo/ComboShip.cpp', 'combo/ComboRandoHeadless.cpp']:
    text = read(root/name)
    dump_name = 'SOH_GenerationDump' if name.endswith('ComboRandoHeadless.cpp') else 'SOH_DumpRandoStaticData'
    for normalize in re.finditer(r'NormalizeOotMaskQuestForGeneration\(', text):
        dump = re.compile(re.escape(dump_name) + r'\(\)').search(text, normalize.end())
        assert dump and 'ApplyOotEnemySoulsAvailability(' in text[normalize.end():dump.start()], (name, 'generation site')
        generation_sites += 1
assert generation_sites == 4, generation_sites
restore = function_body(read(root/'soh/soh/OTRGlobals.cpp'), 'extern "C" COMBO_EXPORT void SOH_RestoreRandoSettings(const char* json)')
assert restore.index('CVarSetInteger(CVAR_RANDOMIZER_SETTING("SoulsEnemyOot"), 0);') < restore.index('for (auto it = j.begin();')
state = read(souls/'OotSoulState.h')
assert hostcxx.contains(state, 'OotEnemySoulsPublic = true, OotNpcSoulsPublic = false, OotAnimalSoulsPublic = false,'
                               'OotMiscSoulsPublic = false;')
# A 0.3.10 (72934704) save's randoSettings stops at its RSK_MAX, 324, after RSK_SHUFFLE_MISC_SOULS; the marker
# is appended in that slot, so such a save loads it as 0.
OLD_RSK_MAX = 324
setting_keys = enum_ids('RandomizerSettingKey.h')
assert setting_keys[-2:] == ['RSK_OOT_SOULS_PUBLISHED', 'RSK_MAX'] and setting_keys.index('RSK_OOT_SOULS_PUBLISHED') == OLD_RSK_MAX
assert (setting_keys.index('RSK_COMPLETE_OOT_SOULS'), setting_keys.index('RSK_SHUFFLE_ENEMY_SOULS')) == (319, 320)
save_manager = read(root/'soh/soh/SaveManager.cpp')
assert hostcxx.contains(save_manager, 'LoadArray("randoSettings", RSK_MAX, [&](size_t i) { int value = 0; '
                                      'SaveManager::Instance->LoadData("", value);')
assert hostcxx.contains(save_manager, 'for (; i < size; i++) { func(i); }')
assert hostcxx.contains(read(root/'soh/soh/SaveManager.h'), 'if (currentJsonArrayContext == currentJsonContext->end()) {'
                                                             ' // This array member is past the data in the json file. '
                                                             'Therefore, default construct it data = defaultValue;')
assert hostcxx.contains(core, 'bool(ctx->GetOption(RSK_OOT_SOULS_PUBLISHED).Get()), '
                              'bool(ctx->GetOption(RSK_COMPLETE_OOT_SOULS).Get())')
print('PASS: "%s" is published with OoTMM\'s description under a fresh CVar; the derived switch and other families '
      'stay hidden; the Enemy Randomizer rule applies at generation only' % setting['name'])

# --- Runtime hooks and pool ---------------------------------------------------------------------------
actor = read(root/'soh/src/code/z_actor.c')
assert 'Combo_OotSoulCanClear' not in function_body(actor, 'void Flags_SetClear(PlayState* play, s32 flag) {')
remove = function_body(actor, 'Actor* Actor_RemoveFromCategory(PlayState* play, ActorContext* actorCtx, Actor* actorToRemove) {')
assert hostcxx.contains(remove, '&& Combo_OotSoulCanClear(play, play->roomCtx.curRoom.num)) {'
                                '        Flags_SetTempClear(play, play->roomCtx.curRoom.num);')
blkobj = function_body(read(root/'soh/src/overlays/actors/ovl_En_Blkobj/z_en_blkobj.c'), 'void EnBlkobj_DarkLinkFight(EnBlkobj* this, PlayState* play) {')
assert hostcxx.contains(blkobj, 'Actor_Find(&play->actorCtx, ACTOR_EN_TORCH2, ACTORCAT_BOSS) == NULL && !Combo_OotDarkLinkSoulMissing()')
assert hostcxx.contains(core, 'Rando::Souls::OotSoul::ENEMY_DARK_LINK);')
prespawn = function_body(core, 'extern "C" int Combo_OotSoulPreSpawn(void* raw, int actor, unsigned params)')
assert hostcxx.contains(prespawn, 'return Rando::Souls::BossItem(soul) != RG_NONE || Rando::Souls::HaveGeneralOotSoul(owned, options, soul);')
assert hostcxx.contains(prespawn, 'if (options.npc) {') and 'ACTOR_BOSS_SST' not in prespawn
# While SoH's Enemy Randomizer is on, souls never keep an enemy away, a room shut or Dark Link's room open.
assert hostcxx.contains(core, 'static bool OotSoulsSuspended() { return CVarGetInteger(CVAR_ENHANCEMENT("RandomizedEnemies"), 0) != 0; }')
assert hostcxx.contains(prespawn, 'if (!IS_RANDO || OotSoulsSuspended() || !Rando::Souls::Options().enabled) return 1;')
assert hostcxx.contains(function_body(core, 'extern "C" int Combo_OotSoulCanClear(void* raw, int room)'),
                        'return OotSoulsSuspended() ||')
assert hostcxx.contains(function_body(core, 'extern "C" int Combo_OotDarkLinkSoulMissing(void)'),
                        'return IS_RANDO && !OotSoulsSuspended() &&')
# A room-list actor whose object its room never loads (OoT's broken actors) never reaches the soul hook.
spawn = function_body(actor, 'Actor* Actor_Spawn(ActorContext* actorCtx, PlayState* play, s16 actorId, f32 posX, f32 posY, '
                             'f32 posZ, s16 rotX, s16 rotY, s16 rotZ, s16 params) {')
guard = hostcxx.find(spawn, 'if (!(gMapLoading && Object_GetIndex(&gPlayState->objectCtx, dbEntry->objectId) < 0) &&'
                            ' Combo_OotSoulPreSpawn(play, actorId, (u16)params) <= 0) return NULL;')
assert spawn.count('Combo_OotSoulPreSpawn(') == 1 and guard < spawn.index('objBankIndex = Object_GetIndex(')
assert hostcxx.contains(spawn, 'if (objBankIndex < 0 && (!gMapLoading || CVarGetInteger(CVAR_ENHANCEMENT("RandomizedEnemies"), 0)))'
                               ' { objBankIndex = 0; }')
assert hostcxx.contains(spawn, 'if ((objBankIndex < 0) ||')
assert hostcxx.contains(function_body(actor, 'Actor* Actor_SpawnEntry(ActorContext* actorCtx, ActorEntry* actorEntry, '
                                             'PlayState* play) {'), 'gMapLoading = 1;')
broken = fixture['mqBrokenActors']['entries']
assert any(e['scene'] == 'SCENE_SPIRIT_TEMPLE' and e['room'] == 1 and e['actor'] == 'ACTOR_EN_FIREFLY' for e in broken)
pool = read(rando/'3drando/item_pool.cpp')
assert 'Souls::AddPool' not in pool and hostcxx.contains(
    pool, 'for (const RandomizerGet soul : Rando::Souls::ShuffledItems()) { AddItemToPool(soul, 2, 1, 1, 1, false); }')
assert 'AddUnshuffledStarts' not in read(rando/'3drando/starting_inventory.cpp')
bare = []
for f in sorted((rando/'location_access').rglob('*.cpp')):
    for n, line in enumerate(read(f).splitlines(), 1):
        if re.search(r'(?<!!)(?<!!Rando::)\b(?:Rando::)?Souls::Enabled\(\)', line):
            bare.append('%s:%d' % (f.name, n))
assert not bare, ('logic rules must key OoTMM soul rules to a family, not the bare switch', bare)
kokiri = read(rando/'location_access/overworld/kokiri_forest.cpp')
bypass = ('(Souls::FamilyShuffled(Souls::OotSoulFamily::Enemy) && logic->IsAdult) || (Souls::FamilyShuffled('
          'Souls::OotSoulFamily::Npc) && logic->IsChild && logic->CanUse(RG_KOKIRI_SWORD) && logic->CanUse(RG_DEKU_SHIELD))')
for region, target in [('RR_KOKIRI_FOREST', 'RR_KF_OUTSIDE_DEKU_TREE'), ('RR_KF_OUTSIDE_DEKU_TREE', 'RR_KOKIRI_FOREST')]:
    block = kokiri[kokiri.index('areaTable[%s] =' % region):]
    exit_ = balanced(block, block.index('(', hostcxx.find(block, 'ENTRANCE(%s,' % target)))
    assert hostcxx.contains(exit_, bypass), ('both Deku Tree exits use can_bypass_mido', region)
assert hostcxx.contains(pool, 'if (itemPool.size() > locCount) { throw std::runtime_error(')
print('PASS: room clears, Dark Link, bosses, broken actors, Enemy Randomizer, pool counts and family-keyed logic rules')

# --- Receipt --------------------------------------------------------------------------------------------
gates, natives = upstream['gates'], receipt['gates']
assert upstream['revision'] == receipt['revision'] == fixture['upstreamCommit']
assert [g['ootmm'] for g in gates] == [n['ootmm'] for n in natives] and len({g['ootmm'] for g in gates}) == len(gates)
statuses = {}
for gate, native in zip(gates, natives):
    status = native['status']
    statuses[status] = statuses.get(status, 0) + 1
    assert status in ('match', 'native_stricter', 'no_native_location', 'not_enemy_soul'), (gate['ootmm'], status)
    if status == 'native_stricter':
        assert native.get('note', '').startswith('Kept'), gate['ootmm']
    if status == 'no_native_location':
        wallmaster = 'WALLMASTER' in gate['souls'].upper()
        assert native.get('pending' if wallmaster else 'note'), gate['ootmm']
        assert wallmaster or 'Flying Pot' in native['note'], gate['ootmm']
    if native.get('auditStatus') in ('native_missing', 'native_weaker'):
        assert status == 'match' and native.get('note'), gate['ootmm']
assert sum(1 for g in gates if g['direct']) == sum(s['directLines'] for s in upstream['sources'].values())
assert sum(1 for g, n in zip(gates, natives) if n.get('pending')) == 23  # OoT Wallmaster edges for the ER work
checked = 0
for entry in natives + receipt['nativeOnly']:
    for evidence in entry.get('evidence', []):
        text = read(rando/evidence['file'])
        if 'region' in evidence:
            block = text[text.index('areaTable[%s] =' % evidence['region']):]
            end = block.find('areaTable[', 1)
            block = block[:end if end > 0 else len(block)]
            start = hostcxx.find(block, evidence['statement'])
            statement = balanced(block, block.index('(', start))
        elif 'case' in evidence:
            start = text.index('case %s:' % evidence['case'])
            statement = text[start:text.index(';', start)]
        else:
            statement = function_body(text, evidence['function'])
        for soul in evidence['souls']:
            assert 'OotSoul::' + soul in statement, (entry.get('ootmm', entry.get('native')), soul)
        for needle in evidence.get('contains', []):
            assert hostcxx.contains(statement, needle), (entry.get('ootmm', entry.get('native')), needle)
        checked += 1
assert checked == 27
print('PASS: receipt %s; %d fixed gates carry their souls' % (dict(sorted(statuses.items())), checked))

# --- Compiled production policy, state and token copy ---------------------------------------------------
template = settings[settings.index('template <class GetOption> static void FinalizeOotSoulSettings'):]
template = template[:template.index('{') + len(balanced(template, template.index('{')))]
rules = generation_rules[generation_rules.index('inline constexpr const char* kOotEnemySouls'):]
rules = rules[:rules.index('{', rules.index('void ApplyOotEnemySoulsAvailability'))]
rules += balanced(generation_rules, generation_rules.index('{', generation_rules.index('void ApplyOotEnemySoulsAvailability')))
scene = {None: 'SCENE_DEKU_TREE', 'SCE_OOT_LAIR_GOHMA': 'SCENE_DEKU_TREE_BOSS', 'SCE_OOT_HYRULE_CASTLE': 'SCENE_HYRULE_CASTLE'}
cases = []
for row in fixture['actors']:
    for actor_id in row['actors']:
        cases.append((actor_id, 0, 'SCENE_DEKU_TREE', True, row['soul'], row['class']))
for row in fixture['variants']:
    cases.append((row['actor'], row['variable'], scene[row['scene']], row['adult'], row['soul'], row['class']))
broken_checks = ['Broken("%s %d", %s, %d, %s);' % (e['actor'], e['params'], e['actor'], e['params'], e['scene'])
                 for e in broken]
native_actor = {'ACTOR_EN_FISHING': 'ACTOR_FISHING'}  # OoTMM's name for SoH's fishing pond actor
checks = []
for actor_id, variable, scene_id, adult, soul, cls in cases:
    target = 'OotSoul::' + soul if soul else 'OotSoul::COUNT'
    checks.append('Case("%s %d %s %d", %s, %d, %s, %s, %s);' % (actor_id, variable, scene_id, adult,
                                                                native_actor.get(actor_id, actor_id), variable,
                                                                scene_id, 'true' if adult else 'false', target))
code = '''#define OOT_SOUL_POLICY_HOST_ENUMS
#include "OotSoulState.h"
#include "OotSoulToken.h"
#include <cassert>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>
using namespace Rando::Souls;
enum RandomizerSettingKey { RSK_COMPLETE_OOT_SOULS, RSK_SHUFFLE_ENEMY_SOULS, RSK_SHUFFLE_NPC_SOULS, RSK_SHUFFLE_ANIMAL_SOULS,
                            RSK_SHUFFLE_MISC_SOULS, RSK_OOT_SOULS_PUBLISHED, RSK_MAX };
enum { RO_GENERIC_OFF = 0, RO_GENERIC_ON = 1 };
struct OptionValue { uint8_t value = 0; uint8_t Get() { return value; } void Set(uint8_t v) { value = v; } };
namespace Rando {
''' + template + '''
}
namespace ComboRando {
''' + rules + '''
}
static bool IsBoss(OotSoul s) { return s >= OotSoul::BOSS_QUEEN_GOHMA && s <= OotSoul::BOSS_TWINROVA; }
// As Combo_OotSoulPreSpawn asks: bosses stay with SoH's own hook.
static int Evaluate(int actor, unsigned variable, int scene, bool adult, bool credits, const GeneralOotSoulsOwnership& owned,
                    const GeneralOotSoulOptions& options) {
    return EvaluateOotSoulPreSpawn(actor, variable, scene, adult, credits,
                                   [&](OotSoul s) { return IsBoss(s) || HaveGeneralOotSoul(owned, options, s); });
}
static void Case(const char* name, int actor, unsigned variable, int scene, bool adult, OotSoul soul) {
    auto has = [&](bool missing) { return [=](OotSoul s) { return !(missing && s == soul); }; };
    const int owned = EvaluateOotSoulPreSpawn(actor, variable, scene, adult, false, has(false));
    const int missing = EvaluateOotSoulPreSpawn(actor, variable, scene, adult, false, has(true));
    const int credits = EvaluateOotSoulPreSpawn(actor, variable, scene, adult, true, has(true));
    // With only the enemy family published, an unowned soul of any other family never blocks.
    const GeneralOotSoulsOwnership none{};
    const int published = Evaluate(actor, variable, scene, adult, false, none,
                                   PublishedGeneralOotSoulOptions(true, true, true, true, true, true));
    std::printf("%s|%d|%d|%d|%d\\n", name, owned, missing, credits, published);
}
// An OoTMM broken actor with every soul missing: anything but 1 would block it and, if hard, its room.
static void Broken(const char* name, int actor, unsigned variable, int scene) {
    std::printf("BROKEN %s|%d\\n", name, EvaluateOotSoulPreSpawn(actor, variable, scene, true, false, [](OotSoul) { return false; }));
}
struct FakeGfx { struct { uintptr_t w0, w1; } words; };
int main() {
''' + '\n'.join(checks + broken_checks) + '''
    // Save state.
    assert(RestoreGeneralOotSouls(nlohmann::json::object()) == GeneralOotSoulsOwnership{});
    GeneralOotSoulsOwnership owned{};
    for (unsigned i = 0; i < GeneralOotSoulCount; i += 7)
        assert(ChangeGeneralOotSoul(owned, i, true));
    assert(ChangeGeneralOotSoul(owned, GeneralOotSoulCount - 1, true) && !ChangeGeneralOotSoul(owned, GeneralOotSoulCount, true));
    assert(RestoreGeneralOotSouls(nlohmann::json{ { "ootExpandedSoulsOwned", owned } }) == owned);
    auto throws = [](nlohmann::json value) {
        try { RestoreGeneralOotSouls(nlohmann::json{ { "ootExpandedSoulsOwned", value } }); } catch (const std::runtime_error&) { return true; }
        return false;
    };
    assert(throws(std::vector<int>(11, 0)) && throws(std::vector<int>(13, 0)) && throws("souls"));
    std::vector<nlohmann::json> wrongType(12, 0); wrongType[3] = "x"; assert(throws(wrongType));
    std::vector<int> bits(12, 0); bits[0] = 256; assert(throws(bits));
    bits[0] = 0; bits[11] = 1 << (GeneralOotSoulCount - 88); assert(throws(bits));
    bits[11] = (1 << (GeneralOotSoulCount - 88)) - 1; assert(!throws(bits));
    // Published families and the derived switch.
    const auto enemyOnly = PublishedGeneralOotSoulOptions(true, true, true, true, true, true);
    assert(enemyOnly.enabled && enemyOnly.enemy && !enemyOnly.npc && !enemyOnly.animal && !enemyOnly.misc);
    for (unsigned i = 0; i < GeneralOotSoulCount; ++i) {
        assert(GeneralOotSoulShuffled(enemyOnly, i) == (i < 43));
        assert(HaveGeneralOotSoul(GeneralOotSoulsOwnership{}, enemyOnly, generalOotSoulItems[i]) == (i >= 43));
        assert(GeneralOotSoulIndex(generalOotSoulItems[i]) == int(i));
    }
    assert(GeneralOotSoulIndex(OotSoul::BOSS_MORPHA) == -1 && GeneralOotSoulIndex(OotSoul::COUNT) == -1);
    assert(!DeriveGeneralOotSoulSwitch(false, true, true, true) && DeriveGeneralOotSoulSwitch(true, false, false, false));
    assert(!PublishedGeneralOotSoulOptions(true, false, true, false, false, false).enabled);   // switch off
    assert(!PublishedGeneralOotSoulOptions(true, true, false, true, true, true).enabled);     // a stale unpublished family
    // A 0.3.10 save: randoSettings stops at the old RSK_MAX (@OLD_MAX@) and may hold 1 in the hidden soul keys,
    // which 0.3.10 never read. SaveManager::LoadArray default-fills the appended marker with 0.
    {
        nlohmann::json saved = std::vector<int>(@OLD_MAX@, 0);
        for (int key : { @COMPLETE@, @ENEMY@, @NPC@, @ANIMAL@, @MISC@ })
            saved[key] = 1;
        std::vector<int> loaded(@NEW_MAX@);
        for (size_t i = 0; i < loaded.size(); ++i)
            loaded[i] = i < saved.size() ? saved[i].get<int>() : 0;
        const auto old = PublishedGeneralOotSoulOptions(loaded[@MARKER@], loaded[@COMPLETE@], loaded[@ENEMY@],
                                                        loaded[@NPC@], loaded[@ANIMAL@], loaded[@MISC@]);
        assert(!old.enabled && !old.enemy && !old.npc && !old.animal && !old.misc);
        for (unsigned i = 0; i < GeneralOotSoulCount; ++i)
            assert(HaveGeneralOotSoul(GeneralOotSoulsOwnership{}, old, generalOotSoulItems[i]));
    }
    // FinalizeSettings / ParseJson derivation, e.g. a spoiler carrying every family.
    std::vector<OptionValue> options(RSK_MAX);
    auto option = [&](RandomizerSettingKey key) -> OptionValue& { return options[key]; };
    for (auto& value : options) value.Set(1);
    options[RSK_OOT_SOULS_PUBLISHED].Set(0);
    Rando::FinalizeOotSoulSettings(option);
    assert(options[RSK_COMPLETE_OOT_SOULS].Get() == 1 && options[RSK_SHUFFLE_ENEMY_SOULS].Get() == 1);
    assert(options[RSK_OOT_SOULS_PUBLISHED].Get() == 1);
    assert(!options[RSK_SHUFFLE_NPC_SOULS].Get() && !options[RSK_SHUFFLE_ANIMAL_SOULS].Get() && !options[RSK_SHUFFLE_MISC_SOULS].Get());
    options[RSK_SHUFFLE_ENEMY_SOULS].Set(0); options[RSK_SHUFFLE_NPC_SOULS].Set(1);
    Rando::FinalizeOotSoulSettings(option);
    assert(options[RSK_COMPLETE_OOT_SOULS].Get() == 0 && options[RSK_OOT_SOULS_PUBLISHED].Get() == 1);
    // Generation clears Enemy Souls (OoT) only while the Enemy Randomizer is on, before the settings snapshot.
    for (int randomizer : { 0, 1 }) {
        std::vector<std::pair<std::string, int>> writes;
        ComboRando::ApplyOotEnemySoulsAvailability(
            [&](const char* key, int fallback) { return std::string(key) == "gEnhancements.RandomizedEnemies" ? randomizer : fallback; },
            [&](const char* key, int value) { writes.emplace_back(key, value); });
        assert(writes == (randomizer ? std::vector<std::pair<std::string, int>>{ { "gRandoSettings.SoulsEnemyOot", 0 } }
                                     : std::vector<std::pair<std::string, int>>{}));
    }
    // Token copy for the cross-game draw.
    std::vector<FakeGfx> list(40), copy;
    for (size_t i = 0; i < list.size(); ++i) list[i].words = { 0xE7000000u + i, i };
    list[TokenSkullPrimCmd].words.w0 = 0xFA000080u; list[TokenSkullEnvCmd].words.w0 = 0xFB000000u;
    list[TokenFlamePrimCmd].words.w0 = 0xFA000080u; list[TokenFlameEnvCmd].words.w0 = 0xFB000000u;
    const FakeGfx noOp{ { 0, 0 } };
    const std::pair<size_t, size_t> commands[] = { { TokenSkullPrimCmd, TokenSkullEnvCmd }, { TokenFlamePrimCmd, TokenFlameEnvCmd } };
    for (auto pair : commands) {
        assert(ColorlessTokenCopy(list, pair.first, pair.second, 0xFAu, 0xFBu, noOp, copy));
        for (size_t i = 0; i < list.size(); ++i) {
            const bool removed = i == pair.first || i == pair.second;
            assert(copy[i].words.w0 == (removed ? 0 : list[i].words.w0) && copy[i].words.w1 == (removed ? 0 : list[i].words.w1));
        }
    }
    copy.clear();
    assert(!ColorlessTokenCopy(list, TokenSkullEnvCmd, TokenSkullPrimCmd, 0xFAu, 0xFBu, noOp, copy) && copy.empty());
    assert(!ColorlessTokenCopy(list, 40, 41, 0xFAu, 0xFBu, noOp, copy) && copy.empty());
    std::puts("STATE OK");
    return 0;
}
'''
code = code.replace('@OLD_MAX@', str(OLD_RSK_MAX)).replace('@NEW_MAX@', str(setting_keys.index('RSK_MAX')))
for name, key in [('COMPLETE', 'RSK_COMPLETE_OOT_SOULS'), ('ENEMY', 'RSK_SHUFFLE_ENEMY_SOULS'), ('NPC', 'RSK_SHUFFLE_NPC_SOULS'),
                  ('ANIMAL', 'RSK_SHUFFLE_ANIMAL_SOULS'), ('MISC', 'RSK_SHUFFLE_MISC_SOULS'), ('MARKER', 'RSK_OOT_SOULS_PUBLISHED')]:
    code = code.replace('@%s@' % name, str(setting_keys.index(key)))
source = work/'oot_enemy_souls.cpp'
source.write_text(code, encoding='utf-8')
exe = hostcxx.build(a.compiler, source, std='c++17', warnings=False,
                    includes=[souls, root/'soh/include', root/'soh', a.json_include])
out = subprocess.run([str(exe)], check=True, capture_output=True, text=True, timeout=hostcxx.RUN_TIMEOUT).stdout.splitlines()
assert out[-1] == 'STATE OK', out[-5:]
results, broken_results = {}, {}
for line in out[:-1]:
    name, *values = line.split('|')
    if name.startswith('BROKEN '):
        broken_results[name[len('BROKEN '):]] = int(values[0])
    else:
        results[name] = tuple(map(int, values))
assert len(results) == len(cases)
# Every OoTMM broken actor the soul policy would block asks for an object its room never loads, so Actor_Spawn
# skips the soul hook for it during the room's actor-list spawn (MQ Spirit Temple room 1's Keese among them).
gated = []
for e in broken:
    result = broken_results['%s %d' % (e['actor'], e['params'])]
    if result != 1:
        assert actor_object(e['actor']) not in e['roomObjects'], e
        gated.append('%s room %d %s' % (e['scene'], e['room'], e['actor']))
assert broken_results['ACTOR_EN_FIREFLY 0'] == 0 and 'SCENE_SPIRIT_TEMPLE room 1 ACTOR_EN_FIREFLY' in gated, broken_results
for actor_id, variable, scene_id, adult, soul, cls in cases:
    owned, missing, credits, published = results['%s %d %s %d' % (actor_id, variable, scene_id, adult)]
    want = {'hard': 0, 'opt': -1, 'always': 1}[cls]
    assert (owned, missing, credits) == (1, want, 1), (actor_id, variable, soul, cls, owned, missing, credits)
    family = soul.split('_')[0] if soul else None
    assert published == (want if family == 'ENEMY' else 1), (actor_id, variable, soul, published)
print('PASS: %d OoTMM actor and variant cases, save state, a 0.3.10 save, published families, derivation, '
      'the generation Enemy Randomizer rule and token copy' % len(cases))
print('PASS: OoTMM broken actors the soul policy would block never reach it: %s' % ', '.join(gated))
