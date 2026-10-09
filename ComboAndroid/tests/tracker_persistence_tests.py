"""Run the native OoT tracker lifecycle and JSON serialization in a host harness."""
from pathlib import Path
import argparse
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--json-include', type=Path, required=True)
parser.add_argument('--compiler', default='cl')
args = parser.parse_args()
source = args.source.resolve()
work = args.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
tracker = (source/'soh/soh/Enhancements/randomizer/randomizer_check_tracker.cpp').read_text(encoding='utf-8')
manager = (source/'soh/soh/SaveManager.cpp').read_text(encoding='utf-8')
manager_header = (source/'soh/soh/SaveManager.h').read_text(encoding='utf-8')

def function(text, signature):
    start = text.index(signature)
    opening = text.index(') {', start) + 2
    depth, end = 1, opening + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]

prelude = r'''
#include <array>
#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "soh/Enhancements/randomizer/randomizerEnums.h"
#include "rando/TrackerKnowledge.h"
#define COMBO_BUILD
#define SPDLOG_WARN(...) ((void)0)
enum { QUEST_RANDOMIZER=2 };
struct SaveContext { int fileNum=0; struct {struct {int id=QUEST_RANDOMIZER;} quest;} ship; };
struct ItemLocation {
    RandomizerCheckStatus status=RCSHOW_UNCHECKED;
    bool skipped=false;
    RandomizerCheckStatus GetCheckStatus() { return status; }
    bool GetIsSkipped() { return skipped; }
    void SetCheckStatus(RandomizerCheckStatus value) { status=value; }
    void SetIsSkipped(bool value) { skipped=value; }
};
struct RandoContext {
    std::array<ItemLocation,RC_MAX> items{};
    ItemLocation* GetItemLocation(int rc) { return &items.at(rc); }
};
struct OTRGlobals {
    static OTRGlobals* Instance;
    std::shared_ptr<RandoContext> gRandoContext=std::make_shared<RandoContext>();
} globals;
OTRGlobals* OTRGlobals::Instance=&globals;
class SaveManager {
public:
    static SaveManager* Instance;
    nlohmann::json data;
    nlohmann::json* currentJsonContext=&data;
    nlohmann::json::iterator currentJsonArrayContext;
    using SaveArrayFunc=std::function<void(size_t)>;
    using SaveStructFunc=std::function<void()>;
    using LoadArrayFunc=std::function<void(size_t)>;
    using LoadStructFunc=std::function<void()>;
    void SaveArray(const std::string&,size_t,SaveArrayFunc);
    void SaveStruct(const std::string&,SaveStructFunc);
    void LoadArray(const std::string&,size_t,LoadArrayFunc);
    void LoadStruct(const std::string&,LoadStructFunc);
'''
prelude += function(manager_header, 'template <typename T> void SaveData(') + '\n'
prelude += function(manager_header, 'template <typename T> void LoadData(') + '\n'
prelude += r'''
} saveManager;
SaveManager* SaveManager::Instance=&saveManager;
bool initialized=true;
uint32_t areasSpoiled=0;
std::array<int,1> filterAreasHidden{},filterChecksHidden{};
std::vector<int> checksByArea;
RandomizerCheck lastLocationChecked=RC_UNKNOWN_CHECK;
int orderingUpdates=0,areaUpdates=0;
void TrySetAreas() {}
void ClearAreaChecksAndTotals() {}
void UpdateAllOrdering() { ++orderingUpdates; }
void UpdateAllAreas() { ++areaUpdates; }
static std::array<ComboRando::TrackerKnowledge,RC_MAX> sItemKnowledge{};
static std::mutex sItemKnowledgeMutex;
'''

tests = r'''
nlohmann::json Save(bool full) {
    SaveContext context;
    SaveTrackerData(&context,0,full);
    // Round trip the text, as the combined save does; no shared JSON references.
    return nlohmann::json::parse(saveManager.data.dump());
}
void Load(const nlohmann::json& saved) {
    globals.gRandoContext=std::make_shared<RandoContext>();
    InitTrackerData(false);
    saveManager.data=saved;
    LoadFile();
}
const nlohmann::json& Row(const nlohmann::json& data,RandomizerCheck rc) {
    for(const auto& row:data.at("checkStatus"))
        if(row.at("randomizerCheck")==rc) return row;
    assert(false && "missing tracker record");
    return data;
}
int main() {
    const auto shop=RC_MARKET_BAZAAR_ITEM_2;
    const auto hint=RC_KF_SHOP_ITEM_1;
    const auto trap=RC_KF_SHOP_ITEM_2;
    const auto skulltula=RC_FOREST_TEMPLE_GS_LEVEL_ISLAND_COURTYARD;
    const auto freestanding=RC_LLR_FREESTANDING_POH;
    const auto foreign=RC_KAK_WINDMILL_FREESTANDING_POH;
    const auto collected=RC_KF_KOKIRI_SWORD_CHEST;
    const auto skipped=RC_KF_STORMS_GROTTO_CHEST;
    InitTrackerData(false);
    sItemKnowledge[shop]={1,RG_PIECE_OF_HEART,"Piece of Heart"};
    sItemKnowledge[hint]={2,RG_PROGRESSIVE_HOOKSHOT,"Progressive Hookshot"};
    sItemKnowledge[trap]={1,RG_KOKIRI_SWORD,"Kokiri Sword"};
    sItemKnowledge[skulltula]={1,RG_PIECE_OF_HEART,"Piece of Heart"};
    sItemKnowledge[freestanding]={1,RG_HOOKSHOT,"Hookshot"};
    sItemKnowledge[foreign]={1,RG_COMBO_FOREIGN,"Bunny Hood (MM)"};
    globals.gRandoContext->items[shop].status=RCSHOW_IDENTIFIED;
    globals.gRandoContext->items[hint].status=RCSHOW_SEEN_OR_HINTED;
    globals.gRandoContext->items[trap].status=RCSHOW_SEEN_OR_HINTED;
    for(auto rc:{skulltula,freestanding,foreign})
        globals.gRandoContext->items[rc].status=RCSHOW_SEEN_OR_HINTED;
    globals.gRandoContext->items[collected].status=RCSHOW_COLLECTED;
    globals.gRandoContext->items[skipped].skipped=true;
    areasSpoiled=0xdead;
    const auto partial=Save(false);
    assert(Row(partial,collected).at("status")==RCSHOW_SCUMMED);
    assert(globals.gRandoContext->items[collected].status==RCSHOW_COLLECTED);
    const auto full=Save(true);
    assert(Row(full,collected).at("status")==RCSHOW_SAVED);
    assert(full.at("checkStatus").size()==8); // Unseen checks stay absent.
    for(bool fullSave:{false,true}) {
        // TitleSetup and Return-to-Spawn both load, then fire exit/load hooks.
        Load(full);
        Teardown();
        assert(!initialized && checksByArea.empty());
        const auto after=Save(fullSave);
        assert(Row(after,shop)==Row(full,shop));
        assert(Row(after,hint)==Row(full,hint));
        assert(Row(after,trap)==Row(full,trap));
        for(auto rc:{skulltula,freestanding,foreign}) assert(Row(after,rc)==Row(full,rc));
        assert(Row(after,collected)==Row(full,collected));
        assert(Row(after,skipped)==Row(full,skipped));
        // Cold start after the post-return save preserves names/provenance.
        Load(after);
        assert(GetKnowledge(shop).source==1 && GetKnowledge(shop).displayName=="Piece of Heart");
        assert(GetKnowledge(hint).source==2 && GetKnowledge(hint).displayName=="Progressive Hookshot");
        assert(GetKnowledge(trap).source==1 && GetKnowledge(trap).displayName=="Kokiri Sword");
        assert(GetKnowledge(skulltula).source==1 && GetKnowledge(skulltula).displayName=="Piece of Heart");
        assert(GetKnowledge(freestanding).source==1 && GetKnowledge(freestanding).displayName=="Hookshot");
        assert(GetKnowledge(foreign).source==1 && GetKnowledge(foreign).displayName=="Bunny Hood (MM)");
    }
    // Loading another profile/seed clears all knowledge, even a legacy record.
    nlohmann::json legacy={{"areasSpoiled",0},{"checkStatus",nlohmann::json::array({
        {{"randomizerCheck",shop},{"status",RCSHOW_IDENTIFIED},{"skipped",false}}})}};
    Load(legacy);
    assert(GetKnowledge(shop).source==0 && GetKnowledge(shop).displayName.empty());
    assert(GetKnowledge(hint).source==0 && GetKnowledge(trap).source==0);
    for(auto rc:{skulltula,freestanding,foreign}) assert(GetKnowledge(rc).source==0);
    assert(Row(Save(true),shop).at("knownItemName")=="");
    InitTrackerData(false); // New-file path also clears previous knowledge.
    assert(GetKnowledge(shop).source==0);
    std::cout<<"PASS: native tracker JSON, full/partial saves, load-exit-save, cold reload, shops, Skulltulas, freestanding/foreign items, hints, trap disguises, unseen checks, collection and profile isolation\n";
}
'''
code = prelude + '\n'
for name in ('SaveArray','SaveStruct','LoadArray','LoadStruct'):
    code += function(manager, 'void SaveManager::'+name+'(') + '\n'
for name in ('GetKnowledge','ResetKnowledge','InitTrackerData','SaveTrackerData','LoadFile','Teardown'):
    signature = ('static ComboRando::TrackerKnowledge ' if name=='GetKnowledge' else
                 'static void ' if name=='ResetKnowledge' else 'void ') + name+'('
    code += function(tracker, signature) + '\n'
path = work/'tracker_persistence.cpp'
path.write_text(code+tests,encoding='utf-8')
exe = work/'tracker_persistence.exe'
subprocess.run([args.compiler,'/nologo','/std:c++20','/Zc:preprocessor','/EHsc','/utf-8','/UNDEBUG',
    '/I'+str(source/'combo'),'/I'+str(source/'soh'),'/I'+str(args.json_include.resolve()),str(path),
    '/Fo'+str(work/'tracker_persistence.obj'),'/Fe'+str(exe)],check=True,cwd=work)
subprocess.run([str(exe)],check=True)
