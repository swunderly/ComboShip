"""Exercise production well propagation, FD sword predicate and Bremen recruitment with engine stubs."""
import argparse
from pathlib import Path
import re
import subprocess

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--json-include', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--compiler', default='cl')
a = p.parse_args()
source = a.source.resolve()
out = a.build_dir.resolve()
out.mkdir(parents=True, exist_ok=True)
fill = (source/'soh/soh/Enhancements/randomizer/3drando/fill.cpp').read_text(encoding='utf-8')
well = fill[fill.index('    if (region->randomizerRegionKey == RR_KAK_WELL'):]
well = well[:well.index('\n#endif')]
logic = (source/'mm/2s2h/Rando/Logic/Logic.h').read_text(encoding='utf-8')
sword = re.search(r'#define CAN_USE_SWORD .*?(?=\n#define|\n\n)', logic, re.S).group()
chicks = (source/'mm/src/overlays/actors/ovl_En_Nwc/z_en_nwc.c').read_text(encoding='utf-8')
def function(name):
    text = chicks[chicks.index(name+'('):]
    text = text[:text.index('\n}\n')+3]
    # C uses `this` for an actor pointer; only rename that identifier for the C++ harness.
    return re.sub(r'\bthis\b', 'self', text)
found = 'int '+function('EnNwc_IsFound')
recruit = 'void '+function('EnNwc_CheckFound')
prelude = r'''
#define COMBO_BUILD 1
#include "rando/GameplayRules.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
int ageMode = 0, deityAnywhere = 0, oneChick = 0, chickCount = -1;
int CVarGetInteger(const char* key, int fallback) {
    const std::string name(key);
    return name == "gEnhancements.TimeTravel" ? ageMode :
        name == "gEnhancements.Masks.FierceDeitysAnywhere" ? deityAnywhere :
        name == "gEnhancements.DifficultyOptions.BremenChickCount" ? (chickCount < 0 ? fallback : chickCount) : oneChick;
}
enum { RR_KAK_WELL = 1, RG_SONG_OF_TIME = 2, RG_OCARINA_OF_TIME = 3, RG_MASTER_SWORD = 4 };
struct TestLogic {
    bool IsChild = true, IsAdult = false, song = true, oot = true, sword = true;
    bool CanUse(int item) { assert(item == RG_SONG_OF_TIME && IsAdult && !IsChild); return song; }
    bool HasItem(int item) { return item == RG_OCARINA_OF_TIME ? oot : sword; }
};
struct Region {
    int randomizerRegionKey = RR_KAK_WELL;
    bool childDay = false, childNight = false, adultDay = false, adultNight = false;
    bool Adult() { return adultDay || adultNight; }
};
struct Search { bool logicUpdated = false, resetSphere = false; };
bool humanSword = false, fairySword = false, deityForm = false, deityMask = false;
#define CAN_USE_HUMAN_SWORD humanSword
#define ITEM_SWORD_GREAT_FAIRY 1
#define HAS_ITEM(item) fairySword
#define IS_DEITY deityForm
#define CAN_BE_DEITY (deityForm || deityMask)
using u8 = uint8_t;
struct Rotation { int x = 0, z = 0; };
struct Home { Rotation rot; };
struct Actor { Home home; float xzDistToPlayer = 0; };
struct Grog { Actor actor; };
struct EnNwc { Grog* grog = nullptr; Actor actor; int state = 0; };
struct Player { int stateFlags3 = 0; };
struct PlayState { Player player; };
#define GET_PLAYER(play) (&(play)->player)
#define PLAYER_STATE3_20000000 1
#define NWC_STATE_FOLLOWING 3
int gSfxDefaultPos = 0;
#define NA_SE_SY_CHICK_JOIN_CHIME 1
void EnNwc_ChangeState(EnNwc* chick, int state) { chick->state = state; }
void Audio_PlaySfx_AtPosWithAllChannelsIO(int*, int, int) {}
'''
tests = r'''
int main() {
    TestLogic l;
    for (int mode = 0; mode <= 4; ++mode) {
        for (int time = 0; time < 2; ++time) {
            Region r; Search g;
            r.adultDay = time == 0; r.adultNight = time == 1;
            ageMode = mode;
            Propagate(&r, &l, g, true);
            assert(r.childDay == (mode != 0 && time == 0));
            assert(r.childNight == (mode != 0 && time == 1));
            assert(g.logicUpdated == (mode != 0) && g.resetSphere == (mode != 0));
            assert(l.IsChild && !l.IsAdult);
        }
    }
    for (int missing = 0; missing < 4; ++missing) {
        Region r; Search g; ageMode = 2; l = TestLogic{};
        r.adultDay = missing != 0; l.song = missing != 1; l.oot = missing != 2; l.sword = missing != 3;
        Propagate(&r, &l, g, false);
        assert(!r.childDay && !r.childNight && !g.logicUpdated && !g.resetSphere);
    }
    assert(!CanSword());
    deityMask = true;
    assert(!CanSword()); // mask possession alone is not a field sword
    deityAnywhere = 1; assert(CanSword());
    deityMask = false; assert(!CanSword());
    deityForm = true; deityAnywhere = 0; assert(CanSword()); // already transformed in a boss arena
    deityForm = false; humanSword = true; assert(CanSword());
    humanSword = false; fairySword = true; assert(CanSword());
    Grog grog; PlayState play; EnNwc chick; chick.grog = &grog;
    oneChick = 1;
    EnNwc_CheckFound(&chick, &play);
    assert(grog.actor.home.rot.z == 0); // must actually march
    play.player.stateFlags3 = PLAYER_STATE3_20000000;
    chick.actor.xzDistToPlayer = 101;
    EnNwc_CheckFound(&chick, &play);
    assert(grog.actor.home.rot.z == 0); // must reach the chick
    chick.actor.xzDistToPlayer = 10;
    EnNwc_CheckFound(&chick, &play);
    assert(chick.actor.home.rot.z == 1 && chick.state == NWC_STATE_FOLLOWING);
    assert(grog.actor.home.rot.z == 20 && grog.actor.home.rot.x == 18);
    oneChick = 0; // changing mid-animation cannot recruit out-of-bounds followers
    EnNwc other; other.grog = &grog;
    EnNwc_CheckFound(&other, &play);
    assert(other.state == 0 && other.actor.home.rot.z == 0);
    grog = Grog{}; chick = EnNwc{}; chick.grog = &grog;
    EnNwc_CheckFound(&chick, &play);
    assert(grog.actor.home.rot.z == 2 && grog.actor.home.rot.x == 0); // vanilla ten-chick mode
    for (int required = 1; required <= 10; ++required) {
        grog = Grog{}; chickCount = required;
        for (int n = 1; n <= required; ++n) {
            EnNwc current; current.grog = &grog; current.actor.xzDistToPlayer = 10;
            EnNwc_CheckFound(&current, &play);
            assert(current.state == NWC_STATE_FOLLOWING && current.actor.home.rot.z == 2*n-1);
            assert(grog.actor.home.rot.z == (n < required ? 2*n : 20));
            assert(grog.actor.home.rot.x == (n < required ? 0 : (10-required)*2));
        }
        EnNwc extra; extra.grog = &grog; extra.actor.xzDistToPlayer = 10;
        EnNwc_CheckFound(&extra, &play); assert(extra.state == 0);
    }
    std::cout << "Production well age propagation, FD sword gating and Bremen recruitment passed\n";
}
'''
cpp = out/'progression_rules.cpp'
cpp.write_text(prelude + '\n' + sword + '\nbool CanSword() { return CAN_USE_SWORD; }\n' +
               'void Propagate(Region* region, TestLogic* logic, Search& gals, bool addToPlaythrough) {\n' +
               well + '\n}\n' + found + '\n' + recruit + '\n' + tests, encoding='utf-8')
exe = out/'progression_rules.exe'
subprocess.run([a.compiler, '/nologo', '/EHsc', '/std:c++17', '/W4', '/WX', '/UNDEBUG',
                '/I'+str(source/'combo'), '/I'+str(a.json_include.resolve()), str(cpp),
                '/Fe:'+str(exe), '/Fo:'+str(out/'progression_rules.obj')], check=True)
subprocess.run([str(exe)], check=True)
