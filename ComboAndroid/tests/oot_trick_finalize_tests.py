"""Compile the production trick parser and finalization reset/apply block."""
from pathlib import Path
import argparse
import subprocess
import hostcxx

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
a = p.parse_args()
root, work = a.source.resolve(), a.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
source = (root/'soh/soh/OTRGlobals.cpp').read_text(encoding='utf-8')
parser = 'static std::set<RandomizerTrick> Combo_ParseEnabledTricks() {' + source.split(
    'static std::set<RandomizerTrick> Combo_ParseEnabledTricks() {', 1)[1].split('\n}', 1)[0] + '\n}\n'
for name in ('extern "C" COMBO_EXPORT void SOH_PrepRandoContext(void)', 'static void EnsureOracleInit()'):
    body = source.split(name + ' {', 1)[1].split('\n}', 1)[0]
    assert 'ctx->FinalizeSettings(Combo_ParseExcludedLocations(), Combo_ParseEnabledTricks());' in body, name
    assert 'Combo_ApplyEnabledTricks' not in body, name
settings = (root/'soh/soh/Enhancements/randomizer/settings.cpp').read_text(encoding='utf-8')
block = settings.split('    // Tricks\n', 1)[1].split('    if (!mOptions[RSK_SHUFFLE_KOKIRI_SWORD])', 1)[0]
code = r'''
#include <cassert>
#include <map>
#include <set>
#include <string>
using RandomizerTrick=int;
std::string csv;
#define CVAR_RANDOMIZER_SETTING(x) x
const char* CVarGetString(const char*,const char*) {return csv.c_str();}
namespace Rando { namespace StaticData {
std::map<std::string,RandomizerTrick> trickToEnum={{"MidoSkip",1},{"NoSoA",2}};
}}
'''
code += parser + r'''
struct Option {int value=0;void Set(int v) {value=v;}};
struct Context {
    std::map<int,Option> mTrickOptions;
    void ResetTrickOptions() {for(auto& [id,option]:mTrickOptions)option.Set(0);}
    void FinalizeSettings(const std::set<int>& enabledTricks) {
''' + block + r'''
    }
};
int main() {
    Context context;
    csv="MidoSkip,NoSoA,MidoSkip,,unknown,";
    const auto enabled=Combo_ParseEnabledTricks();
    assert((enabled==std::set<int>{1,2}));
    context.FinalizeSettings(enabled);
    assert(context.mTrickOptions[1].value==1 && context.mTrickOptions[2].value==1);
    csv="NoSoA";
    context.FinalizeSettings(Combo_ParseEnabledTricks());
    assert(context.mTrickOptions[1].value==0 && context.mTrickOptions[2].value==1);
    csv="";
    context.FinalizeSettings(Combo_ParseEnabledTricks());
    assert(context.mTrickOptions[1].value==0 && context.mTrickOptions[2].value==0);
}
'''
file = work/'oot-tricks.cpp'
file.write_text(code, encoding='utf-8')
exe = hostcxx.build(a.compiler, file)
subprocess.run([str(exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)
