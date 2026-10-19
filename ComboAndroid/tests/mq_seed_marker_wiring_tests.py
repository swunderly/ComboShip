"""Where the per-seed and per-save Master Quest marker (combo/mq/MqSeedMarker.h) is written, read and enforced (U3).

mq_seed_marker_tests.cpp checks the marker's helpers; this scan checks that the game and the launcher use them:
- the save's randomizer section writes the marker only for a bundled seed and reads it back on load;
- SaveManager::LoadFile refuses a marked save this install cannot play, before it binds anything, and file select
  greys it out and, when it is chosen, shows LoadFile's explanation, while a save without the marker keeps SoH's own
  rule;
- SoH's spoiler writes and reads the marker like the Kokiri revisions;
- a new Combo seed (in game and headless) binds the bundled source when the install has it and stamps it with its MQ
  dungeons into the seed document, except an MM-only seed, which rolls no OoT dungeons and records none; a reloaded
  or replayed seed binds its own recorded source, is refused before any
  state changes when its MQ data is missing, and must roll the MQ dungeons it recorded.
"""
from pathlib import Path
import argparse
import re
import sys

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
args = parser.parse_args()
repo = args.source.resolve()
failures = []


def check(condition, message):
    if not condition:
        failures.append(message)
    return condition


def code(path):
    text = (repo / path).read_text(encoding='utf-8-sig')
    text = re.sub(r'/\*.*?\*/', '', text, flags=re.S)
    return re.sub(r'//[^\n]*', '', text)


def body(text, signature):
    start = text.index('{', text.index(signature))
    depth = 0
    for at in range(start, len(text)):
        depth += {'{': 1, '}': -1}.get(text[at], 0)
        if depth == 0:
            return text[start:at + 1]
    raise AssertionError(f'unbalanced body after {signature}')


def before(text, first, second, what):
    check(first in text and second in text and text.index(first) < text.index(second), what)


# The save.
save = code('soh/soh/SaveManager.cpp')
writer = body(save, 'void SaveManager::SaveRandomizer(SaveContext* saveContext, int sectionID, bool fullSave)')
check(re.search(r'if \(const char\* mqMarker = ComboMq::MarkerForSeedMqSource\(randoContext->GetMqDataSource\(\)\)\) \{'
                r'\s*SaveManager::Instance->SaveData\(ComboMq::kMqDataSourceKey, std::string\(mqMarker\)\);', writer),
      'SaveRandomizer must write the marker only when the seed has one')
check(writer.count('kMqDataSourceKey') == 1, 'SaveRandomizer writes the marker once')
loader = body(save, 'void SaveManager::LoadRandomizer()')
check('randoContext->SetMqDataSource(ComboMq::MqDataSourceOf(*SaveManager::Instance->currentJsonContext));' in loader,
      'LoadRandomizer must bind the save\'s marker (absent is Legacy)')
load_file = body(save, 'bool SaveManager::LoadFile(int fileNum, bool initializeFullContext)')
check(re.search(r'const auto mqNeeds = ComboMq::ReadSaveMqNeeds\(parsedSave\);\s*'
                r'if \(ComboMq::SaveRefusedForMq\(mqNeeds, OTRGlobals::Instance->HasMasterQuest\(\),\s*'
                r'OTRGlobals::Instance->HasBundledMasterQuest\(\)\)\) \{.*?return false;', load_file, re.S),
      'LoadFile must refuse a marked save whose MQ data is missing')
before(load_file, 'ComboMq::SaveRefusedForMq', 'comboLoadState.store(ComboLoadState::Loading);',
       'the MQ refusal must come before LoadFile binds the slot')
before(load_file, 'ComboMq::SaveRefusedForMq', 'gSaveContext.fileNum = fileNum;',
       'the MQ refusal must come before LoadFile changes the save context')
meta = body(save, 'void SaveManager::StartupCheckAndInitMeta(int fileNum)')
check('fileMetaInfo[fileNum].mqDataSource = static_cast<u8>(ComboMq::MqDataSourceOf(randoBlock));' in meta,
      'file select metadata must read the save\'s marker')
init_meta = body(save, 'void SaveManager::InitMeta(int fileNum)')
check(re.search(r'fileMetaInfo\[fileNum\]\.mqDataSource =\s*static_cast<u8>\(IS_RANDO \? '
                r'randoContext->GetMqDataSource\(\)\s*: ComboMq::SeedMqSource::Legacy\);', init_meta),
      'new-file metadata must take the bound seed\'s marker')
check(re.search(r'u32 requiresOriginal;\s*#ifdef COMBO_BUILD\s*u8 mqDataSource;', code('soh/soh/SaveManager.h')),
      'SaveFileMetaInfo must carry the marker')

choose = body(code('soh/src/overlays/gamestates/ovl_file_choose/z_file_choose.c'),
              'static int FileChoose_IsSaveCompatible(const SaveFileMetaInfo* restrict meta)')
check(re.search(r'#ifdef COMBO_BUILD\s*valid = valid && ResourceMgr_SaveMqPlayable\(meta->mqDataSource, '
                r'meta->requiresMasterQuest\);\s*#else\s*if \(meta->requiresMasterQuest\) \{\s*'
                r'valid = valid && ResourceMgr_GameHasMasterQuest\(\);\s*\}\s*#endif\s*'
                r'if \(meta->requiresOriginal\)', choose),
      'file select must use the per-save MQ rule and keep SoH\'s requiresOriginal check')
# A greyed-out save chosen at file select only plays an error sound, so LoadFile's popup is never reached from there:
# file select shows the same explanation when the save's MQ data source is the reason.
main_menu = body(code('soh/src/overlays/gamestates/ovl_file_choose/z_file_choose.c'),
                 'void FileChoose_UpdateMainMenu(GameState* thisx)')
check(re.search(r'\} else if \(!FileChoose_IsSaveCompatible\(Save_GetSaveMetaInfo\(this->buttonIndex\)\)\) \{\s*'
                r'Audio_PlaySoundGeneral\(NA_SE_SY_FSEL_ERROR,[^;]*;\s*#ifdef COMBO_BUILD\s*'
                r'Save_ExplainMqRefusal\(this->buttonIndex\);\s*#endif\s*\}', main_menu),
      'choosing a greyed-out save at file select must explain an MQ data refusal')
explain = body(save, 'extern "C" void Save_ExplainMqRefusal(int fileNum)')
check('ComboMq::SaveRefusedForMq(needs, OTRGlobals::Instance->HasMasterQuest(),' in explain and
      'SohGui::RegisterPopup(ComboMq::kSaveMqRefusalTitle, ComboMq::SaveMqRefusalText(needs.source));' in explain,
      'file select\'s explanation must use LoadFile\'s refusal and text')
check('SohGui::RegisterPopup(ComboMq::kSaveMqRefusalTitle, ComboMq::SaveMqRefusalText(mqNeeds.source));' in load_file,
      'LoadFile\'s MQ refusal popup must use the shared text')
check(re.search(r'void Save_ExplainMqRefusal\(int fileNum\);', code('soh/soh/SaveManager.h')),
      'SaveManager.h must declare Save_ExplainMqRefusal for file select')

# SoH's spoiler.
spoiler = body(code('soh/soh/Enhancements/randomizer/3drando/spoiler_log.cpp'), 'void SpoilerLog_Write()')
check(re.search(r'if \(const char\* mqMarker = ComboMq::MarkerForSeedMqSource\(ctx->GetMqDataSource\(\)\)\) \{\s*'
                r'jsonData\[ComboMq::kMqDataSourceKey\] = mqMarker;', spoiler),
      'the SoH spoiler must write the marker only when the seed has one')
parse = body(code('soh/soh/Enhancements/randomizer/SeedContext.cpp'), 'void Context::ParseSpoiler(')
check('const auto previousMqSource = GetMqDataSource();' in parse and
      'const auto mqSource = ComboMq::ReadMqDataSource(spoilerFileJson);' in parse and
      'SetMqDataSource(mqSource);' in parse and 'SetMqDataSource(previousMqSource);' in parse,
      'ParseSpoiler must read the marker and restore the previous one on failure')

# soh's exports.
globals_cpp = code('soh/soh/OTRGlobals.cpp')
for name in ('SOH_GetMqDataSource', 'SOH_SetMqDataSource', 'SOH_HasBundledMasterQuest', 'SOH_GetMqDungeonMask'):
    check(re.search(r'extern "C" COMBO_EXPORT int ' + name + r'\(', globals_cpp), f'soh must export {name}')
check('ctx->SetMqDataSource(static_cast<ComboMq::SeedMqSource>(source))' in body(
    globals_cpp, 'extern "C" COMBO_EXPORT int SOH_SetMqDataSource(int source)'),
      'SOH_SetMqDataSource must go through Context::SetMqDataSource, which refuses unknown sources')

# The launcher.
ship = code('combo/ComboShip.cpp')
for name in ('SOH_GetMqDataSource', 'SOH_SetMqDataSource', 'SOH_HasBundledMasterQuest', 'SOH_GetMqDungeonMask'):
    check(re.search(name + r' = \(ComboMq::Fn\w+\)GetSym\(sohModule, "' + name + r'"\);', ship),
          f'the launcher must bind {name}')
fill = body(ship, 'static void RunComboFill(')
check(re.search(r'const auto mqSource = ComboMq::SourceForNewSeed\(BundledMqAvailable\(\)\);\s*'
                r'ComboMq::MqDataSourceScope mqDataSource\(SOH_GetMqDataSource, SOH_SetMqDataSource, mqSource\);',
                fill), 'a new seed must bind the bundled source when the install has it')
check(re.search(r'ComboMq::StampSeedMq\(consolidated\["oot"\],\s*ComboRando::ModeRandomizesOot\(gameMode\) \? mqSource '
                r': ComboMq::SeedMqSource::Legacy,\s*SOH_GetMqDungeonMask \? SOH_GetMqDungeonMask\(\) : -1\);', fill),
      'a new seed document must record its source and MQ dungeons; an MM-only seed rolls none and records none')
check(fill.count('ComboMq::StampSeedMq(') == 1, 'RunComboFill stamps the seed document once')
check('mqDataSource.Keep()' not in fill, 'the generation scope must give the source back; finalize binds it')
finalize = body(ship, 'static void Combo_FinalizeGenerate()')
check(re.search(r'mqDataSource = std::make_unique<ComboMq::MqDataSourceScope>\(\s*SOH_GetMqDataSource, '
                r'SOH_SetMqDataSource,\s*ComboMq::ReadMqDataSource\(seed\.value\("oot", nlohmann::json::object\(\)\)\)\);',
                finalize) and 'mqDataSource->Keep();' in finalize,
      'finalize must bind the generated seed\'s own source')
reload = ship[ship.index('ComboRando::RequireSupportedSeedDocument(j, ComboRando::Entrances::ActiveCapabilities());'):]
reload = reload[:reload.index('[ComboShip] reload failed: ')]
check(re.search(r'if \(!ComboMq::SeedMqPlayable\(j\.value\("oot", nlohmann::json::object\(\)\), '
                r'BundledMqAvailable\(\)\)\) \{[^}]*return 0;', reload),
      'reload must refuse a seed whose bundled MQ data is missing')
before(reload, 'ComboMq::SeedMqPlayable', 'const auto mode = ComboRando::ReadGameMode(j);',
       'the MQ admission must precede every mode\'s pending state')
check(re.search(r'ComboMq::MqDataSourceScope mqDataSource\(SOH_GetMqDataSource, SOH_SetMqDataSource,\s*'
                r'ComboMq::ReadMqDataSource\(j\.value\("oot", nlohmann::json::object\(\)\)\)\);', reload) and
      'mqDataSource.Keep();' in reload, 'reload must bind the seed\'s own source')
check(re.search(r'SOH_PrepRandoContext\(\);\s*if \(!ComboMq::ReplayKeepsMqDungeons\(oot, SOH_GetMqDungeonMask \? '
                r'SOH_GetMqDungeonMask\(\) : -1\)\) \{[^}]*return 0;', reload),
      'reload must roll the MQ dungeons the seed recorded')

headless = code('combo/ComboRandoHeadless.cpp')
check('auto mqSource = ComboMq::SourceForNewSeed(SOH_HasBundledMq && SOH_HasBundledMq());' in headless,
      'a headless seed must use the bundled source when the install has it')
check('mqSource = ComboMq::ReadMqDataSource(oot);' in headless,
      'headless --settings must reproduce the seed\'s own source (absence is Legacy)')
check(headless.count('ComboMq::ApplyMqDataSource(SOH_SetMqDataSource, mqSource);') == 2,
      'headless generation and --playthrough must bind the source')
check(re.search(r'ComboMq::StampSeedMq\(consolidated\["oot"\], mqSource,\s*SOH_GetMqDungeonMask \? '
                r'SOH_GetMqDungeonMask\(\) : -1\);', headless), 'a headless seed document must record its source')

# Saved seeds and profiles.
saved = code('combo/ComboRandomizerSavedSeed.h')
check('"kokiriGrassCatalogRevision", ComboMq::kMqDataSourceKey })' in saved,
      'replays from a saved file must keep the save\'s marker')
profiles = code('combo/ComboRandomizerProfiles.h')
check('ComboMq::WriteMqDataSource(setup["oot"], mqSource);' in body(profiles, 'inline Json SetupForSeed('),
      'a setup from a seed must keep the seed\'s own source')
check('ComboMq::kMqDungeonsKey' in body(profiles, 'inline Json SettingsDocument('),
      'settings documents must drop the seed\'s MQ dungeons, a generation output')

if failures:
    print('FAIL:\n  ' + '\n  '.join(failures))
    sys.exit(1)
print('PASS: saves, SoH spoilers, the launcher and the headless generator write, bind and enforce the MQ data-source '
      'marker; unmarked saves keep SoH\'s rule')
