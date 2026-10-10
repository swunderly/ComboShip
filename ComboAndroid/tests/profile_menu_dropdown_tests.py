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
prefix = source.read_text(encoding='utf-8').split('void DrawSidebar(', 1)[0]
# The widget prefix does not use the runtime/texture APIs, so it needs no game libraries.
for include in ['#include <libultraship/libultraship.h>\n', '#include <fast/Fast3dGui.h>\n']:
    assert include in prefix
    prefix = prefix.replace(include, '')
(args.build_dir / 'profile_widget.inc').write_text(prefix + '\n}\n', encoding='utf-8')
harness = r'''
#include <imgui_internal.h>
#include <cassert>
#include <iostream>
#include "profile_widget.inc"
namespace ComboRando {
ImVec4 ComboMenu_ThemeColor(){return ImVec4(0,0,1,1);}
void ComboMenu_PushCheckbox(const ImVec4&){} void ComboMenu_PopCheckbox(){}
void ComboMenu_PushCombobox(const ImVec4&){} void ComboMenu_PopCombobox(){}
void ComboMenu_PushSlider(const ImVec4&){} void ComboMenu_PopSlider(){}
void ComboMenu_PushButton(const ImVec4&){} void ComboMenu_PopButton(){}
bool ComboMenu_StyledHeaderEntry(const char* label,bool){return ImGui::Button(label);}
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
