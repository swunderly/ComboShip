"""Run the production menu frame functions with counted native inspection callbacks."""
import argparse
from pathlib import Path
import subprocess
import hostcxx

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--before', type=Path)
p.add_argument('--json-include', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
args = p.parse_args()
source, out = args.source.resolve(), args.build_dir.resolve()
out.mkdir(parents=True, exist_ok=True)
relative = Path('combo/gui/ComboRandomizerProfiles.cpp')
text = ((args.before or source) / relative).read_text(encoding='utf-8')

def function(start):
    pos = hostcxx.find(text, start)
    opening = text.index('{', pos)
    depth, closing = 1, opening + 1
    while depth:
        depth += (text[closing] == '{') - (text[closing] == '}')
        closing += 1
    return text[pos:closing]

declarations = text[text.index('static ComboRandomizerProfileCallbacks callbacks{};'):text.index('static std::shared_ptr')]
engine = r'''
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <iostream>
#include "menu/ComboRandomizerProfileABI.h"
namespace ImGui {
 int frame=0;double now=0;
 int GetFrameCount(){return frame;}double GetTime(){return now;}bool itemActive=false;bool IsAnyItemActive(){return itemActive;}
}
using Json=nlohmann::json;
int inspections=0,policies=0,pools=0,commits=0;
Json current={{"catalog",Json::object()},{"draft",{{"value",0}}},{"savedSettings",{{"value",1}}},
              {"generatedSettings",{{"value",2}}},{"generation",{{"running",false}}}};
std::string response;
const char* Inspect(int slot){++inspections;current["file"]=slot;response=current.dump();return response.c_str();}
const char* SetDraft(int,const char* raw){++commits;current["draft"]=Json::parse(raw);return "{}";}
const char* Copy(int,int){return "{}";}
const char* Generate(int,const char*){return "{}";}
const char* Cancel(){return "{}";}
void RefreshOptionPolicy(bool){++policies;}void RefreshPoolViews(){++pools;}
void Require(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
'''
tests = r'''
void Frame(){++ImGui::frame;ImGui::now+=1./60;BeginFrame();}
void Run(){
 callbacks={1,Inspect,SetDraft,Copy,Generate,Copy,Cancel,nullptr}; // version 1: no revision callback
 for(int i=0;i<60;++i){Frame();Require(active,"profile frame became inactive");EndFrame();}
 std::cout<<"Native inspections across 60 idle menu frames: "<<inspections<<'\n';
 Require(inspections==1&&policies==1&&pools==1,"idle menu repeated full seed/catalog work each frame");
 ImGui::frame+=5;Frame();Require(inspections==2,"reopened menu did not refresh");EndFrame();
 selected=2;Frame();Require(inspections==3&&snapshot["file"]==2,"file selection became stale");EndFrame();
 view=1;Frame();Require(inspections==4&&document["value"]==1,"saved settings view became stale");EndFrame();
 view=0;Frame();document["value"]=7;dirty=true;EndFrame();
 Require(commits==1,"edited settings did not persist");
 Frame();Require(inspections==6&&document["value"]==7,"edited settings did not refresh");EndFrame();
 // A slider drag edits every frame; the store is written and re-inspected once, after release.
 ImGui::itemActive=true;
 for(int i=0;i<30;++i){Frame();document["value"]=10+i;dirty=true;EndFrame();}
 Require(commits==1&&inspections==6,"a slider drag committed or re-inspected every frame");
 ImGui::itemActive=false;Frame();EndFrame();Require(commits==2,"the drag's last value was not committed");
 Frame();Require(inspections==7&&document["value"]==39,"the committed drag value did not refresh");EndFrame();
 // A waiting edit is committed to its own file before the selector shows another view.
 ImGui::itemActive=true;Frame();document["value"]=50;dirty=true;EndFrame();
 view=1;Frame();Require(commits==3&&document["value"]==1,"a waiting edit was lost on a view change");EndFrame();
 ImGui::itemActive=false;view=0;Frame();Require(document["value"]==50,"a waiting edit reached the wrong view");EndFrame();
 // Hiding the menu mid-edit stops its frames: the hidden menu commits the waiting edit itself.
 ImGui::itemActive=true;Frame();document["value"]=60;dirty=true;EndFrame();Require(commits==3,"an edit committed mid-drag");
 Flush();Require(commits==4&&current["draft"]["value"]==60,"a waiting edit was lost when the menu was hidden");
 Flush();Require(commits==4,"a hidden menu committed an unchanged draft");ImGui::itemActive=false;
 current["generation"]["running"]=true;
 ImGui::frame+=5;Frame();const int started=inspections;EndFrame();
 for(int i=0;i<10;++i){Frame();EndFrame();}
 Require(inspections==started,"generation polling ran every render frame");
 ImGui::now+=.3;Frame();Require(inspections==started+1,"generation progress stopped polling");EndFrame();
 current["generation"]["running"]=false;current["generatedSettings"]["value"]=9;
 ImGui::now+=.3;Frame();Require(inspections==started+2,"generation completion was missed");EndFrame();
 for(int i=0;i<30;++i){Frame();EndFrame();}
 Require(inspections==started+2,"completed generation kept doing heavy inspection");
 view=2;Frame();Require(document["value"]==9,"new generated seed view became stale");EndFrame();
}
int main(){try{Run();std::cout<<"PASS: production menu caching, edits, file/view changes, reopening and generation refresh\n";}
 catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
'''
cpp = out / 'profile_menu_refresh.cpp'
cpp.write_text(engine + declarations + function('bool Ready()') + function('bool Editable()')
               + function('static bool Response(')
               + (function('static void RefreshSnapshot(') if 'static void RefreshSnapshot(' in text else '')
               + (function('static void CommitDraft(') if 'static void CommitDraft(' in text else '')
               + function('void BeginFrame()')
               + function('void EndFrame()')
               + (function('void Flush()') if 'void Flush()' in text else 'void Flush(){}\n') + tests, encoding='utf-8')
exe = hostcxx.build(args.compiler, cpp, std='c++17', werror=True, includes=[source/'combo', args.json_include])
subprocess.run([str(exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)
