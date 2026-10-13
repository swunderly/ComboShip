"""Focused production navigation caching and profile empty-state regressions."""
import argparse
from pathlib import Path
import subprocess
import hostcxx

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--json-include', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
a = p.parse_args()
source, out = a.source.resolve(), a.build_dir.resolve()
out.mkdir(parents=True, exist_ok=True)
text = (source / 'combo/gui/ComboRandomizerProfiles.cpp').read_text(encoding='utf-8')
def function(signature):
    start = hostcxx.find(text, signature)
    end = text.index('{', start) + 1
    depth = 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end] + '\n'

code = r'''
#include "gui/ComboMenuNavigation.h"
#include "gui/ComboRandomizerLayout.h"
#include <nlohmann/json.hpp>
#include <cassert>
#include <cstring>
#include <iostream>
#include <set>
namespace ImGui {
bool click=false, expanded=false, disabled=false; float width=1000; int headers=0; std::string message;
struct Vec {float x;}; Vec GetContentRegionAvail(){return {width};} float GetFontSize(){return 16;}
void TextWrapped(const char* format,int file=0){char buffer[256];std::snprintf(buffer,sizeof(buffer),format,file);message=buffer;}
bool Button(const char*){return click;}
void BeginDisabled(bool value){disabled=value;} void EndDisabled(){disabled=false;}
bool CollapsingHeader(const char*){++headers;return expanded;}
bool BeginTable(const char*,unsigned,int){return true;} void EndTable(){}
void TableNextRow(){}void TableSetColumnIndex(unsigned){}void PushID(int){}void PopID(){}void BeginGroup(){}void EndGroup(){}
}
constexpr int ImGuiTableFlags_SizingStretchSame=1,ImGuiTableFlags_NoSavedSettings=2;
namespace ComboRando { int pseudoRows=0; bool DrawMenuPseudoRow(const CwWidget& widget,bool randomizer){assert(randomizer);if(widget.index>-1000)return false;++pseudoRows;return true;} }
namespace ComboRando::ProfileUI {
using Json=nlohmann::json;bool active=true;int view=0,selected=1,refreshes=0,silver=0;Json document;
std::vector<std::string> drawn;bool pouchDisabled=false;
void BeginFrame(){++refreshes;document=Json::object();}
int ReadInteger(const char* key,int fallback){return std::string(key)=="gRandoSettings.SilverRupeeShuffle"?silver:fallback;}
void DrawCustom(const char*,int){}
void DrawWidget(const CwWidget& widget,const GameMenu&){drawn.emplace_back(widget.cvar?widget.cvar:"");if(drawn.back().find("SilverPouch")!=std::string::npos)pouchDisabled=ImGui::disabled;}
'''
code += function('bool Editable()') + function('bool DrawUnavailableSettings()') + function('void DrawSidebar(') + '}\n'
code += r'''
using namespace ComboRando;
void Profiles(){
 using namespace ComboRando::ProfileUI;
 for(int mode=0;mode<3;++mode){view=mode;document=nullptr;assert(!Editable());assert(DrawUnavailableSettings());assert(ImGui::message.find("File 2")!=std::string::npos);}
 view=1;ImGui::click=true;assert(DrawUnavailableSettings());assert(view==0&&refreshes==1&&Editable());ImGui::click=false;
 assert(!DrawUnavailableSettings());view=2;assert(!Editable()&&!DrawUnavailableSettings());active=false;assert(DrawUnavailableSettings()&&!Editable());
 active=true;view=0;
 CwWidget widgets[4]{};
 const char* keys[]={"gRandoSettings.SilverRupeeShuffle","gRandoSettings.SilverPouchBOTW","gRandoSettings.SilverPouchGTG_LAVA","gRandoSettings.Shopsanity"};
 for(int i=0;i<4;++i){widgets[i].cvar=keys[i];widgets[i].column=i==3?1:0;widgets[i].index=20+i;widgets[i].kind=CW_CHECKBOX;}
 CwSidebar side{"Shuffles",2,widgets,4};GameMenu game;
 for(float width:{300.f,1000.f})for(int setting:{0,1,2})for(bool expanded:{false,true}){
  ImGui::width=width;ImGui::expanded=expanded;ImGui::headers=0;drawn.clear();silver=setting;
  DrawSidebar(side,game,0);assert(ImGui::headers==1);assert(drawn.size()==(expanded?4:2));
  assert(drawn.front()==keys[0]&&drawn.back()==keys[3]);if(expanded)assert(pouchDisabled==(setting==0));
 }
 // A gameplay rule row inside its heading is drawn by the menu, never as a custom catalogue page.
 CwWidget rule{};rule.index=-1003;rule.kind=CW_CUSTOM;rule.cvar="gEnhancements.TimeTravel";rule.name="Song of Time age change (OoT)";
 CwSidebar rules{"Gameplay rules",1,&rule,1};drawn.clear();pseudoRows=0;DrawSidebar(rules,game,0);
 assert(pseudoRows==1&&drawn.empty());
}
void Navigation(){
 ComboMenuModel model;auto& oot=const_cast<GameMenu&>(model.Oot());auto& mm=const_cast<GameMenu&>(model.Mm());
 CwWidget widget{};widget.kind=CW_CHECKBOX;widget.cvar="gRandoSettings.CompleteMaskQuest";widget.name="Mask Quest";widget.index=73;
 CwSidebar ootSide{"Logic/Access",1,&widget,1};CwSection ootSection{"Randomizer","",&ootSide,1};CwMenu ootMenu{1,&ootSection,1};
 oot.menu=&ootMenu;oot.loaded=true;MenuNavigationCache cache;
 const char* scopes[]={"home","gameplay","randomizer","media","trackers","settings","tools"};
 for(const char* scope:scopes){std::set<std::string> names;for(const auto& entry:cache.Get(scope,model)){assert(entry.group==scope);assert(names.insert(entry.key()).second);}}
 assert(cache.Get("home",model).size()==1);
 auto& pages=cache.Get("randomizer",model);
 auto mask=std::find_if(pages.begin(),pages.end(),[](const HubEntry& e){return e.label=="Items & checks";});
 assert(mask!=pages.end()&&mask->sources.size()==1&&mask->sources[0].sidebar->widgets[0].index==73);
 mask->bodies[{0,1}].columns=7;
 for(int frame=0;frame<600;++frame){auto& cached=cache.Get("randomizer",model);assert(&cached==&pages);auto& item=*std::find_if(cached.begin(),cached.end(),[](const HubEntry& e){return e.label=="Items & checks";});assert(item.bodies.at({0,1}).columns==7);}
 CwSidebar mmSide{"Check Pool",1,nullptr,0};CwSection mmSection{"Rando","",&mmSide,1};CwMenu mmMenu{1,&mmSection,1};
 mm.menu=&mmMenu;mm.loaded=true;
 auto& both=cache.Get("randomizer",model);auto mixed=std::find_if(both.begin(),both.end(),[](const HubEntry& e){return e.label=="Items & checks";});
 assert(mixed!=both.end()&&mixed->sources.size()==2&&mixed->bodies.empty());
 assert(!IsDeveloperToolPage("Save Editor")&&IsDeveloperToolPage("Collision Viewer"));
 assert(MenuPageOrder("tools","Save Editor")<MenuPageOrder("tools","Developer options"));
 std::cout<<"PASS: 600-frame navigation/body reuse, scope isolation, source identity, late MM loading and developer grouping\n";
}
int main(){Profiles();Navigation();std::cout<<"PASS: missing/saved/draft profiles and pouch controls at narrow/wide widths\n";}
'''
cpp = out / 'menu_polish.cpp'
cpp.write_text(code, encoding='utf-8')
exe = hostcxx.build(a.compiler, cpp, std='c++17', werror=True,
                    includes=[source/'combo', source/'combo/menu', a.json_include])
subprocess.run([str(exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)
