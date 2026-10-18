"""Exercise the production profile dropdown with the bundled Dear ImGui, without game assets."""
import argparse
import subprocess
from pathlib import Path
import hostcxx

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--json-include', type=Path, required=True)
parser.add_argument('--imgui', type=Path, required=True)
parser.add_argument('--profile-source', type=Path)
parser.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
args = parser.parse_args()
args.build_dir.mkdir(parents=True, exist_ok=True)
source = args.profile_source or args.source / 'combo/gui/ComboRandomizerProfiles.cpp'
text = source.read_text(encoding='utf-8')
prefix = text.split('void DrawSidebar(', 1)[0]
catalog_row = 'static void CatalogRow(' + text.split('static void CatalogRow(', 1)[1].split('static void CheckList(', 1)[0]
# The widget prefix does not use the runtime/texture APIs, so it needs no game libraries.
for include in ['#include <libultraship/libultraship.h>\n', '#include <fast/Fast3dGui.h>\n']:
    assert include in prefix
    prefix = prefix.replace(include, '')
(args.build_dir / 'profile_widget.inc').write_text(prefix + catalog_row + '\n}\n', encoding='utf-8')
# The rows MM does not offer here come from ComboMenu.cpp, which resolves MM's readiness exports at run time.
menu_text = (args.source / 'combo/gui/ComboMenu.cpp').read_text(encoding='utf-8')
start = menu_text.index('const char* MmPoolOptionUnavailable(const char* option, bool swimShuffled) {')
depth, end = 0, menu_text.index('{', start)
while True:
    depth += (menu_text[end] == '{') - (menu_text[end] == '}')
    end += 1
    if depth == 0:
        break
unavailable_fn = menu_text[start:end]
harness = r'''
#include <imgui_internal.h>
#include <cassert>
#include <cstring>
#include <iostream>
int CVarGetInteger(const char*, int fallback) { return fallback; }
#include "profile_widget.inc"
bool gearReady=true,spellReady[3]={true,true,true};
extern "C" int FakeSpellOptionReady(int spell){return spell>=1&&spell<=2&&spellReady[spell];}
extern "C" int FakeGearOptionsAllowed(int swimShuffled){return gearReady&&!swimShuffled;}
namespace ComboRando {
void* Combo_ResolveSym(const char* module,const char* name){
 assert(!std::strcmp(module,"2ship"));
 if(!std::strcmp(name,"MM_SpellOptionReady"))return reinterpret_cast<void*>(FakeSpellOptionReady);
 if(!std::strcmp(name,"MM_GearOptionsAllowed"))return reinterpret_cast<void*>(FakeGearOptionsAllowed);
 assert(false);return nullptr;
}
''' + unavailable_fn + r'''
ComboMenuModel& ComboMenuModel::Get(){static ComboMenuModel model;return model;}
ImVec4 ComboMenu_ThemeColor(){return ImVec4(0,0,1,1);}
void ComboMenu_PushCheckbox(const ImVec4&){} void ComboMenu_PopCheckbox(){}
void ComboMenu_PushCombobox(const ImVec4&){} void ComboMenu_PopCombobox(){}
void ComboMenu_PushSlider(const ImVec4&){} void ComboMenu_PopSlider(){}
void ComboMenu_PushButton(const ImVec4&){} void ComboMenu_PopButton(){}
bool ComboMenu_StyledHeaderEntry(const char* label,bool){return ImGui::Button(label);}
bool ComboMenu_CheckboxRow(const char* label,bool* value){return ImGui::Checkbox(label,value);}
bool ComboMenu_SliderRowInt(const char* label,int* value,int min,int max,int,int,const char* format){
 return ImGui::SliderInt(label,value,min,max,format);}
int LoadedRunSlot(){return -1;}
}
int main(){
 using namespace ComboRando; using namespace ComboRando::ProfileUI;
 const char* key="gRandoSettings.BombchuBag";
 catalog["oot"]["options"][key]={{"id",ProfileOotPolicy::RSK_BOMBCHU_BAG},
  {"choices",Json::array({Json{{"label","None"}},Json{{"label","Single Bag"}},Json{{"label","Progressive Bags"}}})}};
 catalog["mm"]=Json::object();document["oot"]["settings"][key]=0;document["mm"]["settings"]=Json::object();
 active=true;view=0;RefreshOptionPolicy(true);RefreshPoolViews();
 ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.DisplaySize=ImVec2(640,480);
 unsigned char* pixels;int width,height;io.Fonts->GetTexDataAsRGBA32(&pixels,&width,&height);
 CwWidget widget{};widget.kind=CW_COMBOBOX;widget.name="Bombchu Bag##Randomizer";widget.cvar=key;widget.index=5;
 GameMenu game{};
 auto frame=[&](int selection){
  ImGui::NewFrame();ImGui::SetNextWindowPos(ImVec2(0,0));ImGui::SetNextWindowSize(ImVec2(640,480));
  ImGui::Begin("Profile menu");
  ImGui::PushID(widget.index);auto popupId=ImHashStr("##ComboPopup",0,ImGui::GetID("##Choice"));ImGui::PopID();
  if(selection<0)ImGui::OpenPopupEx(popupId);
  if(selection>=0){
   auto* popup=ImGui::FindWindowByName("##Combo_00");assert(popup);
   const auto id=popup->GetID(selection==1?"Single Bag":selection==2?"Progressive Bags":"None");
   auto& context=*ImGui::GetCurrentContext();context.NavWindow=popup;context.NavId=id;
   context.NavActivateId=id;context.NavActivateDownId=id;context.NavActivatePressedId=id;
  }
  DrawWidget(widget,game);ImGui::End();ImGui::Render();
 };
 for(int selection : {1,2,0,2,1,0}){
  frame(-1);frame(-1);frame(selection);
  assert(ReadInteger(key,-1)==selection);assert(dirty);
  assert(optionView->Find(key)->choices.size()==3);
  std::cout << "Selected Bombchu Bag " << selection << " and completed the popup frame\n";
 }
 // Mask Quest omits Vanilla, so each choice must store its catalog value, not its position.
 const char* mask="gRandoSettings.CompleteMaskQuest";
 catalog["oot"]["options"][mask]={{"id",ProfileOotPolicy::RSK_MASK_QUEST},
  {"choices",Json::array({Json{{"value",1},{"label","Completed"}},Json{{"value",2},{"label","Shuffle"}}})}};
 document["oot"]["settings"][mask]=2;RefreshOptionPolicy(true);
 widget.name="Mask Quest##Randomizer";widget.cvar=mask;widget.index=6;
 for(int stored : {1,2}){
  auto pick=[&](bool open){
   ImGui::NewFrame();ImGui::SetNextWindowPos(ImVec2(0,0));ImGui::SetNextWindowSize(ImVec2(640,480));
   ImGui::Begin("Profile menu");
   ImGui::PushID(widget.index);auto popupId=ImHashStr("##ComboPopup",0,ImGui::GetID("##Choice"));ImGui::PopID();
   if(open)ImGui::OpenPopupEx(popupId);
   else{
    auto* popup=ImGui::FindWindowByName("##Combo_00");assert(popup);
    const auto id=popup->GetID(stored==1?"Completed":"Shuffle");
    auto& context=*ImGui::GetCurrentContext();context.NavWindow=popup;context.NavId=id;
    context.NavActivateId=id;context.NavActivateDownId=id;context.NavActivatePressedId=id;
   }
   DrawWidget(widget,game);ImGui::End();ImGui::Render();
  };
  pick(true);pick(true);pick(false);
  assert(ReadInteger(mask,-1)==stored);
  std::cout << "Selected Mask Quest value " << stored << "\n";
 }
 // Older files store Vanilla (0), which has no choice: the next seed shows Shuffle (what generation makes of it), a
 // seed made with it shows Completed (it starts with the masks), and the stored value is kept.
 for(int seedView : {0,1,2}){
  document["oot"]["settings"][mask]=0;view=seedView;dirty=false;
  ImGui::NewFrame();ImGui::SetNextWindowPos(ImVec2(0,0));ImGui::SetNextWindowSize(ImVec2(640,480));
  ImGui::Begin("Profile menu");ImGui::LogToBuffer();DrawWidget(widget,game);
  const std::string rendered=ImGui::GetCurrentContext()->LogBuffer.c_str();
  ImGui::LogFinish();ImGui::End();ImGui::Render();
  assert(rendered.find("Unknown saved value")==std::string::npos);
  assert(rendered.find(seedView==0?"Shuffle":"Completed")!=std::string::npos);
  assert(ReadInteger(mask,-1)==0&&!dirty);
 }
 view=0;
 static_assert(OotMaskQuestShownMode(0,true)==2&&OotMaskQuestShownMode(0,false)==1);
 static_assert(OotMaskQuestShownMode(1,true)==1&&OotMaskQuestShownMode(2,false)==2);
 std::cout << "Mask Quest's stored Vanilla reads as the mode it plays as and stays stored\n";
 // Settings of a pre-0.3.9 seed have no rule block: edits must produce a block the readers accept,
 // keep values already written into a partial block, and mirror the native OoT logic option.
 const char* bfa="gRandoSettings.BlueFireArrows";
 document["gameplayRules"]={{"values",{{"gEnhancements.OpenAllHours",1}}}};document["oot"]["settings"][bfa]=0;
 assert(ReadInteger("gEnhancements.OpenAllHours",0)==1);
 WriteInteger(bfa,1);WriteInteger("gEnhancements.BlueFireArrows",1);
 assert(IsGameplayRuleBlock(document["gameplayRules"])&&document["gameplayRules"]["version"]==2);
 const auto rules=NormalizeGameplayRuleSnapshot(document["gameplayRules"]);
 assert(rules["logicValues"]["gEnhancements.BlueFireArrows"]==1&&rules["values"]["gEnhancements.OpenAllHours"]==1);
 assert(document["oot"]["settings"][bfa]==1);
 WriteInteger(bfa,0);assert(document["oot"]["settings"][bfa]==0);
 std::cout << "Upgraded a legacy rule block and mirrored Blue Fire Arrows logic\n";
 // A malformed song travel map cannot be derived from; reading the menu must keep the block's own values.
 document["gameplayRules"]={{"values",{{"gEnhancements.OpenAllHours",1}}}};document["songTravel"]={{"version",9}};
 assert(ReadInteger("gEnhancements.OpenAllHours",0)==1&&document["gameplayRules"]["version"]==2);
 document.erase("songTravel");
 std::cout << "Read a rule block next to a malformed song travel map\n";
 // The profile catalog is the active MM item-pool UI. Gear must reflect the same Swim
 // restriction as the native menu without erasing the preference or changing old seed views.
 const char* swim="gRando.Options.RO_SHUFFLE_SWIM";
 ImVec2 checkboxPosition;io.ConfigInputTrickleEventQueue=false;
 auto catalogFrame=[&](const Json& row,int mouseAction,bool disabled,bool checked){
  if(mouseAction){io.AddMousePosEvent(checkboxPosition.x,checkboxPosition.y);io.AddMouseButtonEvent(0,mouseAction==1);}
  ImGui::NewFrame();ImGui::SetNextWindowPos(ImVec2(0,0));ImGui::SetNextWindowSize(ImVec2(640,480));
  ImGui::Begin("Profile catalog");auto& context=*ImGui::GetCurrentContext();
  ImGui::LogToBuffer();CatalogRow(row);
  checkboxPosition=context.LastItemData.Rect.GetCenter();
  assert(context.LastItemData.ID!=0);
  assert(bool(context.LastItemData.ItemFlags&ImGuiItemFlags_Disabled)==disabled);
  const std::string rendered=context.LogBuffer.c_str();
  if(rendered.find(checked?"[x]":"[ ]")==std::string::npos)
   std::cerr << "Unexpected checkbox render: " << row.at("key") << ", mouseAction=" << mouseAction
             << ", view=" << view << ", checked=" << checked << ": " << rendered << '\n';
  assert(rendered.find(checked?"[x]":"[ ]")!=std::string::npos);
  ImGui::LogFinish();ImGui::End();ImGui::Render();
 };
 for(const char* gear : {"RO_MM_BOOTS_IRON","RO_MM_BOOTS_HOVER","RO_MM_TUNIC_GORON","RO_MM_TUNIC_ZORA"}){
  const std::string gearKey=std::string("gRando.Options.")+gear;
  const Json row={{"key",gearKey},{"label",gear},{"kind","checkbox"},{"tooltip","MM gear"}};
  view=0;WriteInteger(gearKey.c_str(),1);WriteInteger(swim,1);dirty=false;
  catalogFrame(row,0,true,false);catalogFrame(row,1,true,false);catalogFrame(row,2,true,false);
  assert(ReadInteger(gearKey.c_str(),-1)==1&&!dirty);
  WriteInteger(swim,0);dirty=false;
  catalogFrame(row,0,false,true);catalogFrame(row,1,false,true);catalogFrame(row,2,false,false);
  assert(ReadInteger(gearKey.c_str(),-1)==0&&dirty);
  WriteInteger(gearKey.c_str(),1);WriteInteger(swim,1);dirty=false;
  for(int seedView : {1,2}){
   view=seedView;catalogFrame(row,0,true,true);catalogFrame(row,1,true,true);catalogFrame(row,2,true,true);
   assert(ReadInteger(gearKey.c_str(),-1)==1&&!dirty);
  }
  // A device that cannot run the gear greys the rows out with Swim off too, as the native GearPoolCheckbox does.
  gearReady=false;view=0;WriteInteger(swim,0);dirty=false;
  catalogFrame(row,0,true,false);catalogFrame(row,1,true,false);catalogFrame(row,2,true,false);
  assert(ReadInteger(gearKey.c_str(),-1)==1&&!dirty);
  for(int seedView : {1,2}){
   view=seedView;catalogFrame(row,0,true,true);
  }
  view=0;
  assert(!std::strcmp(MmPoolOptionUnavailable(gear,false),
                      "Disabled because this device cannot run the MM boots and tunics yet. New seeds leave them out."));
  gearReady=true;
  assert(!std::strcmp(MmPoolOptionUnavailable(gear,true),
                      "Disabled while Swim is shuffled. New seeds leave the MM boots and tunics out."));
  assert(MmPoolOptionUnavailable(gear,false)==nullptr);
 }
 std::cout << "Swim and device readiness disable all four MM gear rows, preserving preferences and saved/generated views\n";
 // The MM spells follow device readiness only (the native SpellPoolCheckbox); Farore's Wind (MM) is always offered.
 view=0;WriteInteger(swim,1);
 for(int spell : {1,2}){
  const char* option=spell==1?"RO_MM_SPELL_LOVE":"RO_MM_SPELL_FIRE";
  const std::string spellKey=std::string("gRando.Options.")+option;
  const Json spellRow={{"key",spellKey},{"label",option},{"kind","checkbox"}};
  WriteInteger(spellKey.c_str(),1);dirty=false;
  catalogFrame(spellRow,0,false,true);catalogFrame(spellRow,1,false,true);catalogFrame(spellRow,2,false,false);
  assert(ReadInteger(spellKey.c_str(),-1)==0&&dirty);
  WriteInteger(spellKey.c_str(),1);spellReady[spell]=false;dirty=false;
  catalogFrame(spellRow,0,true,false);catalogFrame(spellRow,1,true,false);catalogFrame(spellRow,2,true,false);
  assert(ReadInteger(spellKey.c_str(),-1)==1&&!dirty);
  assert(!std::strcmp(MmPoolOptionUnavailable(option,false),
                      "Disabled because this device could not prepare the spell. New seeds leave it out."));
  view=1;catalogFrame(spellRow,0,true,true);view=0;
  spellReady[spell]=true;
 }
 spellReady[1]=spellReady[2]=false;gearReady=false;
 const char* wind="gRando.Options.RO_MM_SPELL_WIND";WriteInteger(wind,1);dirty=false;
 const Json windRow={{"key",wind},{"label","Farore's Wind"},{"kind","checkbox"}};
 catalogFrame(windRow,0,false,true);catalogFrame(windRow,1,false,true);catalogFrame(windRow,2,false,false);
 assert(ReadInteger(wind,-1)==0&&dirty);
 spellReady[1]=spellReady[2]=true;gearReady=true;
 std::cout << "Device readiness disables the Din's Fire and Nayru's Love (MM) rows and spares Farore's Wind (MM)\n";
 // Search results: an MM option draws its catalog row, and a row its page hides (Trap Count while Traps is off) still
 // names and shows its control, greyed out.
 const char* trapCount="gRando.Options.RO_TRAP_AMOUNT";
 catalog["mm"]["layout"]=Json::array({Json{{"page","Traps"},{"card","itemPoolTraps"},{"heading","Traps"},
  {"label","Trap Count"},{"kind","slider"},{"tooltip",""},{"key",trapCount},{"minimum",0},{"maximum",50}}});
 auto searchFrame=[&](const std::function<void()>& draw,const char* name,int expectDisabled){
  ImGui::NewFrame();ImGui::SetNextWindowPos(ImVec2(0,0));ImGui::SetNextWindowSize(ImVec2(640,480));
  ImGui::Begin("Search results");auto& context=*ImGui::GetCurrentContext();
  ImGui::LogToBuffer();context.LastItemData.ItemFlags=0;draw();
  const std::string rendered=context.LogBuffer.c_str();
  if(expectDisabled<0)assert(rendered.find(name)==std::string::npos);
  else{
   assert(rendered.find(name)!=std::string::npos);
   assert(bool(context.LastItemData.ItemFlags&ImGuiItemFlags_Disabled)==bool(expectDisabled));
  }
  ImGui::LogFinish();ImGui::End();ImGui::Render();
 };
 view=0;WriteInteger("gRando.Options.RO_SHUFFLE_TRAPS",0);
 searchFrame([&]{CatalogRow(catalog["mm"]["layout"][0]);},"Trap Count",-1); // the page hides it
 searchFrame([&]{assert(DrawCatalogSetting(trapCount));},"Trap Count",1);
 WriteInteger("gRando.Options.RO_SHUFFLE_TRAPS",1);
 searchFrame([&]{assert(DrawCatalogSetting(trapCount));},"Trap Count",0);
 assert(!DrawCatalogSetting("gRando.Options.RO_NOT_A_ROW"));
 // An OoT option these settings hide shows greyed out in a search result, and not at all on its page.
 const char* hiddenKey="gRandoSettings.SearchHiddenTest";
 catalog["oot"]["options"][hiddenKey]={{"id",100000},{"defaultHidden",true},
  {"choices",Json::array({Json{{"label","Off"}},Json{{"label","On"}}})}};
 RefreshOptionPolicy(true);assert(optionView->Find(hiddenKey)->hidden);
 CwWidget hiddenRow{};hiddenRow.kind=CW_COMBOBOX;hiddenRow.name="Hidden Option";hiddenRow.cvar=hiddenKey;hiddenRow.index=7;
 searchFrame([&]{DrawWidget(hiddenRow,game);},"Hidden Option",-1);
 searchFrame([&]{DrawWidget(hiddenRow,game,true);},"Hidden Option",1);
 catalog["oot"]["options"].erase(hiddenKey);RefreshOptionPolicy(true);
 std::cout << "Search results name and show every Randomizer option, greying out the ones their page hides\n";
 ImGui::DestroyContext();std::cout << "PASS: production dropdown selection/refresh remains valid\n";
}
'''
(args.build_dir / 'profile_widget.cpp').write_text(harness, encoding='utf-8')
includes = [args.source / 'combo/gui', args.source / 'combo', args.source / 'combo/menu',
            args.json_include, args.imgui]
# Built with AddressSanitizer: the test guards memory safety across the popup's refresh.
exe = hostcxx.build(args.compiler, args.build_dir / 'profile_widget.cpp', warnings=False, includes=includes,
                    sources=[args.imgui / file for file in
                             ['imgui.cpp', 'imgui_draw.cpp', 'imgui_tables.cpp', 'imgui_widgets.cpp', 'misc/cpp/imgui_stdlib.cpp']],
                    msvc=['/Zc:preprocessor', '/utf-8', '/Zi', '/fsanitize=address'], gnu=['-g', '-fsanitize=address'],
                    link=['/INCREMENTAL:NO'] if hostcxx.msvc_style(args.compiler) else [])
subprocess.run([str(exe)], cwd=args.build_dir, check=True, timeout=hostcxx.RUN_TIMEOUT)
# Search result cards draw these rows: every setting shows its control (hidden ones greyed out), including the MM
# Randomizer options, which come from the catalog rather than a native widget.
results = menu_text[menu_text.index('void ComboMenu::DrawSearchResults('):]
results = results[:results.index('\n}\n')]
assert hostcxx.contains(results, 'ProfileUI::DrawWidget(*hit.widget, *hit.game, true)'), 'hidden OoT options drop out'
assert hostcxx.contains(results, 'RenderWidget(*hit.widget, *hit.game, 90 / cols, true)'), 'hidden native rows drop out'
assert hostcxx.contains(results, 'hits.back().catalogKey = row.value("key", std::string())'), 'MM options lost their key'
assert hostcxx.contains(results, 'ProfileUI::DrawCatalogSetting(hit.catalogKey.c_str())'), 'MM options show no control'
render = (args.source / 'combo/gui/ComboWidgetRender.cpp').read_text(encoding='utf-8')
assert hostcxx.contains(render, 'if (state == 2 && !showHidden)') and \
    hostcxx.contains(render, 'game.drawWidget && GameDrawsWidget(w) && state != 2'), \
    'a hidden native row must take the greyed raw path when shown'
