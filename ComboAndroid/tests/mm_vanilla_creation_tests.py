"""Execute the production MM constructor: original opening vs. combined/rando baseline, without ROMs."""
from pathlib import Path
import argparse
import subprocess
import hostcxx

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
p.add_argument('--json-include', type=Path, required=True)
a = p.parse_args()
a.build_dir.mkdir(parents=True, exist_ok=True)

def function(file, signature):
    text = (a.source / file).read_text(encoding='utf-8')
    start = hostcxx.find(text, signature)
    opening = text.index('{', start)
    end, depth = opening + 1, 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]

program = r'''
#include "ComboGameMode.h"
#include <nlohmann/json.hpp>
#include <cassert>
#include <cstring>
#include <string>
using Json = nlohmann::json;
using s16 = short;
enum { SAVETYPE_VANILLA, SAVETYPE_RANDO, PLAYER_FORM_HUMAN = 4, ITEM_OCARINA_OF_TIME = 1,
       ITEM_MASK_DEKU, QUEST_SONG_TIME, QUEST_SONG_HEALING, SCENE_INSIDETOWER,
       WEEKEVENTREG_59_04, WEEKEVENTREG_31_04, WEEKEVENTREG_ENTERED_EAST_CLOCK_TOWN,
       WEEKEVENTREG_ENTERED_WEST_CLOCK_TOWN, WEEKEVENTREG_ENTERED_NORTH_CLOCK_TOWN, CURRENT_SAVE_VERSION };
struct Save {
    bool hasTatl = false, isFirstCycle = false;
    int playerForm = 99;
    struct {
        struct { char newf[6]{}; unsigned char playerName[8]{}; bool isMagicAcquired = false; int threeDayResetCount = 0; } playerData;
        struct { unsigned questItems = 0; } inventory;
        struct { unsigned switch0 = 0; } permanentSceneFlags[64];
    } saveInfo;
    struct { int saveType = SAVETYPE_VANILLA; } shipSaveInfo;
};
void to_json(Json& j, const Save& s) { j = {{"firstCycle", s.isFirstCycle}, {"type", s.shipSaveInfo.saveType}}; }
struct { Save save; int fileNum = -1; } gSaveContext;
int gMMComboGameMode = COMBO_MODE_COMBINED, gComboOwlBlobSlot = 9, writes = 0, weekEvents = 0;
int inventory[64]{};
Json written;
std::string destination;
#define SPDLOG_INFO(...) ((void)0)
#define COMBO_EXPORT
#define INV_CONTENT(item) inventory[item]
#define SET_WEEKEVENTREG(flag) (++weekEvents)
void Sram_InitNewSave() { gSaveContext.save = Save{}; gSaveContext.fileNum = -1; std::memset(inventory,0,sizeof(inventory)); weekEvents = 0; }
std::string SaveManager_GetFileName(int n) { return "file" + std::to_string(n); }
void SaveManager_WriteSaveFile(const std::string& path, const Json& value) { ++writes; destination=path; written=value; }
'''
program += function('mm/2s2h/SaveManager/SaveManager.cpp', 'void SaveManager_InitNewSaveForSlot(')
program += function('mm/2s2h/BenPort.cpp', 'extern "C" COMBO_EXPORT int MM_InitVanillaSaveFile(')
program += r'''
int main() {
    const unsigned char name[8] = {1,2,3,4,5,6,7,255};
    for (int mode : {COMBO_MODE_COMBINED, COMBO_MODE_MM_RANDO, COMBO_MODE_MM_VANILLA}) {
        gMMComboGameMode = mode;
        written = {{"owlSave", "old save"}};
        SaveManager_InitNewSaveForSlot(3, name);
        assert(destination=="file3" && !written.contains("owlSave") && gComboOwlBlobSlot == -1);
        assert(std::memcmp(gSaveContext.save.saveInfo.playerData.newf,"ZELDA3",6)==0);
        assert(gSaveContext.save.saveInfo.playerData.playerName[7] == 0x3E);
        const bool vanilla = mode == COMBO_MODE_MM_VANILLA;
        assert(gSaveContext.save.isFirstCycle == !vanilla);
        assert(gSaveContext.save.hasTatl == !vanilla);
        assert(gSaveContext.save.saveInfo.playerData.isMagicAcquired == !vanilla);
        assert(inventory[ITEM_OCARINA_OF_TIME] == (vanilla ? 0 : ITEM_OCARINA_OF_TIME));
        assert(inventory[ITEM_MASK_DEKU] == (vanilla ? 0 : ITEM_MASK_DEKU));
        assert(gSaveContext.save.shipSaveInfo.saveType == (vanilla ? SAVETYPE_VANILLA : SAVETYPE_RANDO));
        assert(weekEvents == (vanilla ? 0 : 5));
    }
    gMMComboGameMode = COMBO_MODE_MM_VANILLA;
    assert(MM_InitVanillaSaveFile(1, name) == 0 && gSaveContext.fileNum == 1 && destination == "file2");
    const int before = writes;
    assert(MM_InitVanillaSaveFile(-1, name) == -1 && MM_InitVanillaSaveFile(3, name) == -1 && writes == before);
    gMMComboGameMode = COMBO_MODE_COMBINED;
    assert(MM_InitVanillaSaveFile(0, name) == -1 && writes == before);
}
'''
cpp = a.build_dir / 'mm_vanilla_creation.cpp'
cpp.write_text(program, encoding='utf-8')
exe = hostcxx.build(a.compiler, cpp, includes=[a.source / 'combo', a.json_include],
                    defines=['COMBO_BUILD'], werror=True)
subprocess.run([str(exe)], cwd=a.build_dir, check=True, timeout=hostcxx.RUN_TIMEOUT)
print('Production MM constructors preserve original opening, type, inventory, name, slot and stale-owl isolation.')
