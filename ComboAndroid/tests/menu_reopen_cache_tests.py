"""Execute production profile callbacks/repository revisions and menu reopening."""
import argparse
from pathlib import Path
import subprocess
import hostcxx

p=argparse.ArgumentParser()
p.add_argument('--source',type=Path,required=True)
p.add_argument('--before',type=Path)
p.add_argument('--json-include',type=Path,required=True)
p.add_argument('--build-dir',type=Path,required=True)
p.add_argument('--compiler',default=hostcxx.DEFAULT_COMPILER)
a=p.parse_args()
source,out=a.source.resolve(),a.build_dir.resolve()
out.mkdir(parents=True,exist_ok=True)
text=((a.before or source)/'combo/gui/ComboRandomizerProfiles.cpp').read_text(encoding='utf-8')

def function(signature):
    start=hostcxx.find(text,signature)
    end=text.index('{',start)+1
    depth=1
    while depth:
        depth+=(text[end]=='{')-(text[end]=='}')
        end+=1
    return text[start:end]+'\n'

declarations=text[text.index('static ComboRandomizerProfileCallbacks callbacks{};'):text.index('static std::shared_ptr')]
code=r'''
#define COMBO_EXPORT
#include <nlohmann/json.hpp>
#include <iostream>
#include <stdexcept>
#include "menu/ComboRandomizerProfileABI.h"
#include "ComboRandomizerProfileRepository.h"
namespace ImGui {int frame=0;double now=0;int GetFrameCount(){return frame;}double GetTime(){return now;}bool IsAnyItemActive(){return false;}}
using Json=nlohmann::json;
int inspections=0,policies=0,pools=0,commits=0;
uint64_t nativeRevision=1;
Json current={{"catalog",Json::object()},{"draft",{{"value",0}}},{"savedSettings",{{"value",1}}},
              {"generatedSettings",{{"value",2}}},{"generation",{{"running",false}}}};
std::string response;
bool failInspect=false;
const char* Inspect(int slot){++inspections;if(failInspect)return "{\"error\":\"retry\"}";current["file"]=slot;response=current.dump();return response.c_str();}
const char* SetDraft(int,const char* raw){++commits;current["draft"]=Json::parse(raw);++nativeRevision;return "{}";}
const char* Copy(int,int){++nativeRevision;return "{}";}
const char* Generate(int,const char*){current["generation"]["running"]=true;++nativeRevision;return "{}";}
const char* Cancel(){current["generation"]["running"]=false;++nativeRevision;return "{}";}
uint64_t Revision(){return nativeRevision;}
void Require(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
namespace ComboRando::ProfileUI {
void RefreshOptionPolicy(bool){++policies;}void RefreshPoolViews(){++pools;}
'''
code+=declarations+function('bool Ready()')+function('bool Editable()')+function('static bool Response(')
if 'static void RefreshSnapshot(' in text:code+=function('static void RefreshSnapshot(')
if 'static void CommitDraft(' in text:code+=function('static void CommitDraft(')
code+=function('void BeginFrame()')+function('void EndFrame()')+'}\n'
code+=function('extern "C" COMBO_EXPORT void ComboUI_SetRandomizerProfileStore(')
code+='#define TEST_REVISION_CALLBACK '+str(int('static void RefreshSnapshot(' in text))+'\n'
code+=r'''
using namespace ComboRando::ProfileUI;
void Frame(){++ImGui::frame;ImGui::now+=1./60;BeginFrame();}
void RepositoryTests(const std::filesystem::path& path){
 using namespace ComboRando::Profiles;
 Json seed={{"fileType","ComboShipRandomizer"},{"oot",{{"settings",Json::object()}}},{"mm",{{"settings",Json::object()}}}};
 Repository repository(path,seed,ComboRando::Entrances::ActiveCapabilities());auto token=repository.revision();
 for(int i=0;i<20;++i)Require(repository.revision()==token,"reading revision changed profile state");
 repository.setDraft(0,seed);Require(repository.revision()>token,"draft mutation missed invalidation");token=repository.revision();
 repository.copyDraft(0,1);Require(repository.revision()>token,"draft copy missed invalidation");token=repository.revision();
 repository.copySaved(0,2,seed);Require(repository.revision()>token,"saved copy missed invalidation");token=repository.revision();
 repository.importSeed(0,seed,false);Require(repository.revision()>token,"import missed invalidation");token=repository.revision();
 repository.migrateLegacyPending(seed);Require(repository.revision()>token,"migration missed invalidation");token=repository.revision();
 auto attempt=repository.begin(0,seed,"test");Require(repository.revision()>token,"generation start missed invalidation");token=repository.revision();
 repository.finish(attempt.token,true,seed);Require(repository.revision()>token,"generation finish missed invalidation");token=repository.revision();
 bool failed=false;try{repository.setDraft(8,seed);}catch(const std::exception&){failed=true;}
 Require(failed&&repository.revision()==token,"rejected edit changed revision");
}
void Run(){
 const ComboRandomizerProfileCallbacks native{TEST_REVISION_CALLBACK?2:1,Inspect,SetDraft,Copy,Generate,Copy,Cancel,Revision};
 ComboUI_SetRandomizerProfileStore(&native);
 if(!TEST_REVISION_CALLBACK){Frame();EndFrame();}
 Require(snapshotValid&&inspections==1,"runtime did not prime menu data before first opening");
 const int before=inspections;
 for(int opening=0;opening<20;++opening){ImGui::frame+=120;Frame();Require(active,"reopened menu inactive");EndFrame();}
 std::cout<<"Heavy inspections across 20 unchanged menu openings: "<<inspections-before<<'\n';
 Require(inspections==before&&policies==1&&pools==1,"reopening repeated full seed/catalog work");
 ++nativeRevision;current["savedSettings"]["value"]=8;Frame();Require(inspections==before+1,"external seed/file change did not refresh");EndFrame();
 selected=2;Frame();Require(snapshot["file"]==2,"selected file became stale");EndFrame();
 view=1;Frame();Require(document["value"]==8,"saved settings became stale");EndFrame();
 view=0;Frame();document["value"]=7;dirty=true;EndFrame();Frame();Require(commits==1&&document["value"]==7,"edited settings did not persist/refresh");EndFrame();
 Generate(2,"");ImGui::frame+=120;Frame();Require(snapshot["generation"]["running"]==true,"generation started while menu closed was missed");const int started=inspections;EndFrame();
 for(int i=0;i<10;++i){Frame();EndFrame();}Require(inspections==started,"generation inspected every frame");
 ImGui::now+=.3;Frame();Require(inspections==started+1,"generation progress stopped refreshing");EndFrame();
 current["generation"]["running"]=false;current["generatedSettings"]["value"]=9;++nativeRevision;
 Frame();Require(inspections==started+2,"generation completion did not refresh");EndFrame();
 view=2;Frame();Require(document["value"]==9,"completed seed settings became stale");EndFrame();
 failInspect=true;++nativeRevision;Frame();Require(!active,"failed inspection reused incompatible data");EndFrame();
 failInspect=false;Frame();Require(active,"failed inspection never retried");EndFrame();
 ComboUI_SetRandomizerProfileStore(nullptr);Frame();Require(!active,"unavailable callbacks kept profile active");EndFrame();
 // Version 1 remains supported without reading a nonexistent revision callback.
 const ComboRandomizerProfileCallbacks legacy{1,Inspect,SetDraft,Copy,Generate,Copy,Cancel,nullptr};
 ComboUI_SetRandomizerProfileStore(&legacy);Frame();Require(active,"version 1 unavailable");EndFrame();
 const int legacyCount=inspections;ImGui::frame+=5;Frame();Require(inspections==legacyCount+1,"version 1 lost reopening refresh");EndFrame();
}
int main(int argc,char** argv){try{Require(argc==2,"temporary path required");RepositoryTests(argv[1]);Run();
 std::cout<<"PASS: production revision invalidation, startup priming, unchanged reopening, file/view/edit/import/generation and retry behavior\n";}
 catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
'''
cpp=out/'menu_reopen.cpp'
cpp.write_text(code,encoding='utf-8')
exe=hostcxx.build(a.compiler,cpp,std='c++20',werror=True,includes=[source/'combo',a.json_include])
subprocess.run([str(exe),str(out/'temporary-profiles.json')],check=True, timeout=hostcxx.RUN_TIMEOUT)
