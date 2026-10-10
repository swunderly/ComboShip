"""Execute production dormant MM getter/reconciliation functions with slot fixtures."""
from pathlib import Path
import argparse, subprocess
import hostcxx
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--build-dir',type=Path,required=True);p.add_argument('--compiler',default=hostcxx.DEFAULT_COMPILER);p.add_argument('--json-include',type=Path,required=True);a=p.parse_args()
text=(a.source/'combo/ComboShip.cpp').read_text(); work=a.build_dir;work.mkdir(parents=True,exist_ok=True)
def function(signature):
    start=hostcxx.find(text,signature);opening=text.index('{',start);end=opening+1;depth=1
    while depth:depth+=(text[end]=='{')-(text[end]=='}');end+=1
    if text[end:].startswith(' catch'):
        # Keep actual exception handling on the function-try-block.
        while text[end:].startswith(' catch'):
            opening=text.index('{',end);end=opening+1;depth=1
            while depth:depth+=(text[end]=='{')-(text[end]=='}');end+=1
    return text[start:end]
source=r'''
#include <cassert>
#include <iostream>
#include <algorithm>
#include "rando/SharedItems.h"
int g_comboCompletionSlot=-1,g_MmSaveInMemorySlot=-1;
unsigned g_sharedMask=0;
int ootMask=1,mmMask=6,mmReads=0,ootTier=0,mmTier=1,raises=0;
int readOotMask(){return ootMask;} int readMmMask(){++mmReads;return mmMask;}
int readMmPieces(){++mmReads;return 99;}
auto SOH_GetNamedTriforceMask=readOotMask;auto MM_GetNamedTriforceMask=readMmMask;auto MM_GetTriforcePieceCount=readMmPieces;
int readOotTier(int){return ootTier;}int readMmTier(int){++mmReads;return mmTier;}
void raiseOot(int,int tier){ootTier=tier;++raises;}void raiseMm(int,int tier){mmTier=tier;++raises;}
auto SOH_GetSharedTier=readOotTier;auto MM_GetSharedTier=readMmTier;auto SOH_RaiseSharedTier=raiseOot;auto MM_RaiseSharedTier=raiseMm;
'''
source+=function('static unsigned Combo_GetNamedTriforceMask()')+'\n'+function('static int Combo_GetMmTriforceCount() {')+'\n'+function('static void Combo_SharedReconcileNow() try')
source+=r'''
int main(){
 g_sharedMask=1u<<ComboRando::SF_SONG_OF_TIME;
 for(auto slots:{std::pair{-1,-1},std::pair{0,-1},std::pair{0,1},std::pair{2,0}}){
  g_comboCompletionSlot=slots.first;g_MmSaveInMemorySlot=slots.second;
  assert(Combo_GetNamedTriforceMask()==1);assert(Combo_GetMmTriforceCount()==0);
  Combo_SharedReconcileNow();assert(mmReads==0&&raises==0&&ootTier==0);
 }
 g_comboCompletionSlot=0;g_MmSaveInMemorySlot=0;
 assert(Combo_GetNamedTriforceMask()==7);assert(Combo_GetMmTriforceCount()==99);
 Combo_SharedReconcileNow();assert(ootTier==1&&raises==1);
 Combo_SharedReconcileNow();assert(raises==1);
 g_comboCompletionSlot=1;g_MmSaveInMemorySlot=-1;ootTier=0;int reads=mmReads;
 Combo_SharedReconcileNow();assert(mmReads==reads&&raises==1&&ootTier==0);
 std::cout<<"PASS production unbound/failed/mismatched/matched MM slot isolation\n";
}
'''
cpp=work/'residency.cpp';cpp.write_text(source)
subprocess.run([str(hostcxx.build(a.compiler,cpp,warnings=False,includes=[a.source/'combo',a.json_include]))],check=True, timeout=hostcxx.RUN_TIMEOUT)
# Bind path must clear residency before a potentially failed native load/create.
assert hostcxx.find(text,'g_MmSaveInMemorySlot = -1; // Failure') < hostcxx.find(text,'if (MM_LoadSaveForCombo(fileNum) == 0)')
assert hostcxx.contains(text,'if (mmInitResult == 0) g_MmSaveInMemorySlot = fileNum;')
progress=text[hostcxx.find(text,'static void Combo_OnTriforceProgress(int game, int fileNum) try'):]
assert hostcxx.find(progress,'if (g_MmSaveInMemorySlot != fileNum)') < hostcxx.find(progress,'const int total =')

