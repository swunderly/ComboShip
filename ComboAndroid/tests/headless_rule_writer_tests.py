"""Execute the production gameplay-rule ABI with and without runtime hooks."""
from pathlib import Path
import argparse
import subprocess
import hostcxx

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
args = parser.parse_args()
source = args.source.resolve()
work = args.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
native = (source/'soh/soh/OTRGlobals.cpp').read_text(encoding='utf-8')


def function(signature):
    start = native.index(signature)
    opening = native.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (native[end] == '{') - (native[end] == '}')
        end += 1
    return native[start:end] + '\n'


read = function('extern "C" COMBO_EXPORT int32_t SOH_ReadGameplayRule(')
write = function('extern "C" COMBO_EXPORT void SOH_WriteGameplayRule(')
mm = (source/'mm/2s2h/BenPort.cpp').read_text(encoding='utf-8')
assert 'MM_WriteGameplayRule' not in mm  # The shared store has a single writer ABI.
for filename in ('BlueFireArrows.cpp', 'SunlightArrows.cpp'):
    hooks = (source/'soh/soh/Enhancements/Items'/filename).read_text(encoding='utf-8')
    assert 'static RegisterShipInitFunc' in hooks
    assert 'COND_ID_HOOK' in hooks  # These refreshes do require a real runtime interactor.

prelude = r'''
#define COMBO_EXPORT
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <vector>
std::map<std::string,int32_t> cvars;
int writes=0;
void CVarSetInteger(const char* key,int32_t value) {++writes;cvars[key]=value;}
int32_t CVarGetInteger(const char* key,int32_t fallback) {
    auto it=cvars.find(key);return it==cvars.end()?fallback:it->second;
}
struct GameInteractor {static void* Instance;};
void* GameInteractor::Instance=nullptr;
enum {RSK_BLUE_FIRE_ARROWS,RSK_SUNLIGHT_ARROWS};
namespace Rando {
struct Option {
    int value=0,sets=0;
    void Set(uint8_t v) {value=v;++sets;}
};
struct Context {
    Option options[2];
    static Context* GetInstance() {static Context context;return &context;}
    Option& GetOption(int key) {return options[key];}
};
}
std::vector<std::string> refreshed;
namespace ShipInit {
void Init(const char* key) {
    // Models the real actor-hook registration precondition, failing if an
    // unguarded native refresh attempts to run during headless generation.
    assert(GameInteractor::Instance!=nullptr);
    refreshed.emplace_back(key);
}
}
'''
tests = r'''
int main() {
    const char* keys[]={"gRandoSettings.BlueFireArrows","gRandoSettings.SunlightArrows"};
    const char* hooks[]={"gEnhancements.BlueFireArrows","gEnhancements.SunlightArrows"};
    int count=0;
    for(int live=0;live<2;++live) for(int option=0;option<2;++option)
      for(int32_t value:{0,1,2,-1}) {
        GameInteractor::Instance=live?reinterpret_cast<void*>(1):nullptr;
        cvars.clear();writes=0;refreshed.clear();
        auto context=Rando::Context::GetInstance();
        context->options[0]=Rando::Option{};context->options[1]=Rando::Option{};
        SOH_WriteGameplayRule(keys[option],value);
        assert(writes==1&&SOH_ReadGameplayRule(keys[option],99)==value);
        assert(context->options[option].value==int(value!=0));
        assert(context->options[option].sets==1&&context->options[1-option].sets==0);
        assert(refreshed.size()==size_t(live));
        if(live) assert(refreshed[0]==hooks[option]);
        ++count;
    }
    for(int live=0;live<2;++live) {
        GameInteractor::Instance=live?reinterpret_cast<void*>(1):nullptr;
        for(const char* key:{"gEnhancements.BlueFireArrows","gEnhancements.SunlightArrows",
             "gCombo.Logic.gEnhancements.TimeTravel","gCombo.Rando.ForeignSongs"}) {
            refreshed.clear();writes=0;
            SOH_WriteGameplayRule(key,3);
            assert(writes==1&&SOH_ReadGameplayRule(key,99)==3&&refreshed.empty());
            ++count;
        }
        writes=0;refreshed.clear();
        SOH_WriteGameplayRule(nullptr,1);
        assert(writes==0&&refreshed.empty());
        assert(SOH_ReadGameplayRule(nullptr,42)==42);
        assert(SOH_ReadGameplayRule("absent",41)==41);
        ++count;
    }
    std::cout<<"PASS: "<<count<<" production ABI cases; headless hook safety, native option/CVar writes, live refresh and null/fallback behavior\n";
}
'''
test = work/'headless_rule_writer.cpp'
test.write_text(prelude + read + write + tests, encoding='utf-8')
exe = hostcxx.build(args.compiler, test, werror=True)
subprocess.run([str(exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)
