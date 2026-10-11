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
harness = r'''
#include <imgui_internal.h>
#include <cassert>
#include <iostream>
#include "profile_widget.inc"
namespace ComboRando {
ComboMenuModel& ComboMenuModel::Get(){static ComboMenuModel model;return model;}
ImVec4 ComboMenu_ThemeColor(){return ImVec4(0,0,1,1);}
void ComboMenu_PushCheckbox(const ImVec4&){} void ComboMenu_PopCheckbox(){}
void ComboMenu_PushCombobox(const ImVec4&){} void ComboMenu_PopCombobox(){}
void ComboMenu_PushSlider(const ImVec4&){} void ComboMenu_PopSlider(){}
void ComboMenu_PushButton(const ImVec4&){} void ComboMenu_PopButton(){}
bool ComboMenu_StyledHeaderEntry(const char* label,bool){return ImGui::Button(label);}
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
 }
 view=0;const char* love="gRando.Options.RO_MM_SPELL_LOVE";WriteInteger(love,1);dirty=false;
 const Json loveRow={{"key",love},{"label","Nayru's Love"},{"kind","checkbox"}};
 catalogFrame(loveRow,0,false,true);catalogFrame(loveRow,1,false,true);catalogFrame(loveRow,2,false,false);
 assert(ReadInteger(love,-1)==0&&dirty);
 std::cout << "Swim disables all four MM gear rows, preserves preferences and saved/generated views, and spares spells\n";
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
