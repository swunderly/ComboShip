"""Exercise the production draft-to-runtime adapter without game assets."""
import argparse
import subprocess
import uuid
from pathlib import Path
import hostcxx

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--json-include', type=Path, required=True)
parser.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
args = parser.parse_args()
args.build_dir.mkdir(parents=True, exist_ok=True)
adapter = (args.source / 'combo/ComboRandomizerProfileNative.inc').read_text(encoding='utf-8')
response = adapter.split('template<class F>', 1)[1].split('static const char* ProfileInspect(', 1)[0]
draft = adapter.split('static const char* ProfileSetDraft(', 1)[1].split('static const char* ProfileCopySaved(', 1)[0]
(args.build_dir / 'draft_adapter.inc').write_text(
    'template<class F>' + response + 'static const char* ProfileSetDraft(' + draft, encoding='utf-8')
harness = r'''
#define NOMINMAX
#include "ComboRandomizerProfileRepository.h"
#include "rando/CrossWorldRando.h"
#include "rando/GameplayRuleSnapshots.h"
#include <atomic>
#include <cassert>
#include <memory>
#include <map>
using Json=nlohmann::json;
static std::unique_ptr<ComboRando::Profiles::Repository> sProfiles;
static ComboRando::ComboGenProgress g_ComboProgress;
static std::atomic<uint64_t> g_profileSavedSeedRevision{1}; // read by the extracted ProfileRevision()
static int sGameplayRulesSlot=0,refreshes=0,applies=0;
static std::map<std::string,int> runtime;
static int Read(const char* key,int fallback){auto it=runtime.find(key);return it==runtime.end()?fallback:it->second;}
static void Write(const char* key,int value){runtime[key]=value;}
static int32_t (*sReadGameplayRule)(const char*,int32_t)=Read;
static void (*sWriteGameplayRule)(const char*,int32_t)=Write;
static void (*sApplyOotRule)(const char*)=+[](const char*){++applies;};
static void (*sApplyMmRule)(const char*)=+[](const char*){++applies;};
static Json Combo_ReadSavedSeedReadonly(int){return Json();}
static void RefreshGameplayRuntime(){++refreshes;}
#include "draft_adapter.inc"
int main(int argc,char** argv){
 using namespace ComboRando;
 assert(argc==2);
 const auto path=std::filesystem::path(argv[1])/"profiles.json";
 Json setup={{"fileType","ComboShipRandomizer"},{"gameplayRules",CaptureGameplayRules(Read)}};
 sProfiles=std::make_unique<Profiles::Repository>(path,setup);
 auto commit=[&](int slot){const auto reply=Json::parse(ProfileSetDraft(slot,setup.dump().c_str()));assert(reply.value("success",false));};
 auto put=[&](const char* key,int value){const auto& rule=*FindGameplayRule(key);
  setup["gameplayRules"][LogicRuleKey(rule.name)==key?"logicValues":"values"][rule.name]=value;};
 const auto& cross=*FindGameplayRule("gCombo.Enhancements.ForeignSongs");
 SetLogicRule(cross,1,put);commit(0);
 assert(Read(cross.name,0)==1 && refreshes==1 && applies==2);
 // Live enhancements change, but an existing seed's logic is never rewritten by a next-seed draft.
 assert(Read(LogicRuleKey(cross.name).c_str(),0)==0);
 assert(sProfiles->view().draft(0)["gameplayRules"]["logicValues"][cross.name]==1);
 runtime[cross.name]=0;commit(0);assert(Read(cross.name,0)==1); // repair a mismatched live ability
 SetLogicRule(cross,0,put);commit(0);assert(Read(cross.name,1)==0);
 SetEnhancementRule(cross,1,[&](const char* key,int fallback){const auto& rule=*FindGameplayRule(key);
  return setup["gameplayRules"][LogicRuleKey(rule.name)==key?"logicValues":"values"].value(rule.name,fallback);},put);
 commit(0);assert(Read(cross.name,0)==1);
 assert(sProfiles->view().draft(0)["gameplayRules"]["logicValues"][cross.name]==0);
 const auto live=runtime;const int previousRefresh=refreshes;
 SetLogicRule(cross,0,put);commit(1);assert(runtime==live && refreshes==previousRefresh);
 g_ComboProgress.running=true;commit(0);assert(runtime==live && refreshes==previousRefresh);
 g_ComboProgress.running=false;
 for(const auto& rule:kGameplayRules){
  if(rule.runtimeOnly||rule.trickGoverned||IsLegacyGameplayRule(rule.name))continue;
  SetLogicRule(rule,rule.maximum,put);commit(0);assert(Read(rule.name,rule.fallback)==rule.maximum);
 }
 // Runtime-only legacy assumptions are discarded; shortening Ballad never enters seed logic.
 const auto& ballad=*FindGameplayRule("gEnhancements.Timesavers.SkipBalladOfWindfish");
 assert(ballad.runtimeOnly);setup["gameplayRules"]["values"][ballad.name]=1;
 setup["gameplayRules"]["logicValues"][ballad.name]=1;
 assert(NormalizeGameplayRuleSnapshot(setup["gameplayRules"])["logicValues"][ballad.name]==0);
 auto saved=sProfiles->view().draft(0);setup["gameplayRules"]["values"][cross.name]="invalid";
 commit(0);assert(Read(cross.name,-1)==0); // invalid imports cannot enable arbitrary runtime rules
 setup=saved;commit(0);
 sProfiles.reset();sProfiles=std::make_unique<Profiles::Repository>(path,Json::object());
 assert(sProfiles->view().draft(0)["gameplayRules"]==saved["gameplayRules"]);
 std::cout<<"PASS: production profile coupling, independent enhancement, all rule modes, file isolation and reload\n";
}
'''
(args.build_dir / 'draft_adapter.cpp').write_text(harness, encoding='utf-8')
executable = hostcxx.build(args.compiler, args.build_dir / 'draft_adapter.cpp', warnings=False,
                           includes=[args.source / 'combo', args.json_include], msvc=['/Zc:preprocessor', '/utf-8'])
run = args.build_dir / ('run-' + str(uuid.uuid4()))
run.mkdir(exist_ok=True)
subprocess.run([str(executable), str(run.resolve())], cwd=args.build_dir, check=True, timeout=hostcxx.RUN_TIMEOUT)
