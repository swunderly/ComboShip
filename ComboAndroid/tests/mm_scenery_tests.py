"""Compare compiled production Keaton predicates to the pinned OoTMM macros."""
from pathlib import Path
import argparse,re,subprocess
import hostcxx
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True)
p.add_argument('--build-dir',type=Path,required=True);p.add_argument('--compiler',default=hostcxx.DEFAULT_COMPILER);a=p.parse_args()
work=a.build_dir.resolve();work.mkdir(parents=True,exist_ok=True)
text=(a.source/'mm/2s2h/Rando/Logic/Logic.h').read_text()
def macro(name):
 match=re.search(r'^#define '+name+r'\b([^\n]*?(?:\\\n[^\n]*?)*)(?<!\\)$',text,re.M)
 assert match,name
 return '#define '+name+match.group(1)+'\n'
code='''#include <cassert>
#include <iostream>
bool sword,magic,spin,fairy;int level;
constexpr int WEEKEVENTREG_RECEIVED_GREAT_SPIN_ATTACK=1,ITEM_SWORD_GREAT_FAIRY=2;
constexpr int EQUIP_TYPE_SWORD=0,EQUIP_VALUE_SWORD_GILDED=3;
#define CAN_USE_SWORD sword
#define HAS_MAGIC magic
#define CHECK_WEEKEVENTREG(x) spin
#define HAS_ITEM(x) fairy
#define GET_CUR_EQUIP_VALUE(x) level
'''+macro('CAN_CUT_KEATON_GRASS_EASY')+macro('CAN_CUT_KEATON_GRASS_HARD')+'''
int main(){
for(int v=0;v<64;++v){sword=(v&1)!=0;magic=(v&2)!=0;spin=(v&4)!=0;fairy=(v&8)!=0;level=(v>>4)&3;
 std::cout<<v<<" "<<CAN_CUT_KEATON_GRASS_EASY<<" "<<CAN_CUT_KEATON_GRASS_HARD<<"\\n";
}}
'''
target=work/'mm_scenery.cpp';target.write_text(code)
exe=hostcxx.build(a.compiler,target,werror=True)
reference=Path(__file__).resolve().parents[3]/'references/ootmm-stable-32.3'
source=(reference/'data/macros/macros_mm.yml').read_text() if reference.exists() else ''
expressions={
 'keaton_grass_easy':'has_sword && has_magic',
 'keaton_grass_hard':'keaton_grass_easy && (has_spin_upgrade || (has(GREAT_FAIRY_SWORD) || has(SHARED_GREAT_FAIRY_SWORD)) || has_sword_level(3))'}
for name,expr in expressions.items():
 if source:assert '"'+name+'": "'+expr+'"' in source,name
for line in subprocess.check_output([str(exe)],text=True, timeout=hostcxx.RUN_TIMEOUT).splitlines():
 v,easy,hard=map(int,line.split());sword=bool(v&1);magic=bool(v&2);spin=bool(v&4);fairy=bool(v&8);level=(v>>4)&3
 # Evaluate the upstream expressions independently of the native definitions.
 state={'has_sword':sword,'has_magic':magic,'has_spin_upgrade':spin,
  'GREAT_FAIRY_SWORD':'fairy','SHARED_GREAT_FAIRY_SWORD':'fairy',
  'has':lambda item:fairy,'has_sword_level':lambda threshold:level>=threshold}
 state['keaton_grass_easy']=eval(expressions['keaton_grass_easy'].replace('&&',' and '),{'__builtins__':{}},state)
 expected=eval(expressions['keaton_grass_hard'].replace('&&',' and ').replace('||',' or '),{'__builtins__':{}},state)
 assert bool(easy)==state['keaton_grass_easy'] and bool(hard)==expected,(v,easy,hard)
print('PASS: 64 compiled production Keaton states match OoTMM v32.3')
