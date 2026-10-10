"""Exercise the production FD sword predicate and Bremen recruitment with engine stubs.

The Kakariko Well age-propagation case this file used to extract no longer
exists: 0.3.9 replaced it with the generic SongAgeChangeAreaAllowed() block in
fill.cpp, which song_age_area_tests.py exercises for every native area.
"""
import argparse
from pathlib import Path
import re
import subprocess
import hostcxx

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--json-include', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
a = p.parse_args()
source = a.source.resolve()
out = a.build_dir.resolve()
out.mkdir(parents=True, exist_ok=True)
logic = (source/'mm/2s2h/Rando/Logic/Logic.h').read_text(encoding='utf-8')
def macro(name):
    # First (COMBO_BUILD) definition, including clang-format's continuation lines.
    return re.search(r'^#define ' + name + r'\b(?:[^\n]*\\\n)*[^\n]*', logic, re.M).group() + '\n'
sword = macro('CAN_USE_DEITY_OUTSIDE_BOSS') + macro('CAN_USE_SWORD')
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
#include "rando/MmAbilities.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
int deityAnywhere = 0, oneChick = 0, chickCount = -1;
int CVarGetInteger(const char* key, int fallback) {
    const std::string name(key);
    return name == "gEnhancements.Masks.FierceDeitysAnywhere" ? deityAnywhere :
        name == "gEnhancements.DifficultyOptions.BremenChickCount" ? (chickCount < 0 ? fallback : chickCount) : oneChick;
}
bool humanSword = false, fairySword = false, deityForm = false, deityMask = false;
#define CAN_USE_HUMAN_SWORD humanSword
#define ITEM_SWORD_GREAT_FAIRY 1
#define ITEM_MASK_FIERCE_DEITY 2
#define HAS_ITEM(item) ((item) == ITEM_SWORD_GREAT_FAIRY ? fairySword : (item) == ITEM_MASK_FIERCE_DEITY && deityMask)
#define IS_DEITY deityForm
#define CAN_BE_DEITY (deityForm || deityMask)
#define COMBO_LOGIC_RULE(name) CVarGetInteger(name, 0) // the seed's rule value
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
    assert(!CanSword());
    deityMask = true;
    assert(!CanSword()); // mask possession alone is not a field sword
    deityAnywhere = 1; assert(CanSword());
    deityMask = false; assert(!CanSword());
    // Since combined-logic-guarantees, seed logic ignores the current form:
    // only the owned mask plus the Fierce Deity's Anywhere rule count.
    deityForm = true; deityAnywhere = 0; assert(!CanSword());
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
    std::cout << "Production FD sword gating and Bremen recruitment passed\n";
}
'''
cpp = out/'progression_rules.cpp'
cpp.write_text(prelude + '\n' + sword + '\nbool CanSword() { return CAN_USE_SWORD; }\n' +
               found + '\n' + recruit + '\n' + tests, encoding='utf-8')
exe = hostcxx.build(a.compiler, cpp, std='c++17', werror=True, includes=[source/'combo', a.json_include])
subprocess.run([str(exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)
