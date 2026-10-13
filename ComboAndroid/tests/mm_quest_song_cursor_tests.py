"""MM pause Quest page: the cursor reaches Saria's Song and Sun's Song, so their names show.

MM draws Saria's Song and Sun's Song in Oath to Order's and Song of Storms' place (sQuestVtxRectLeft/Top), and no
vanilla cursor link leads to them: with either OoT song learned the cursor rests on the unlearned MM song under its
note and the name bar stays blank. This builds the quest cursor's links, its movement and its cursor item code from
z_kaleido_collect.c (COMBO_BUILD) on host stubs and walks the page: a place is its MM song, or its OoT song when only
that one is learned; with both learned the OoT song is one step right of the MM song; every other move is vanilla.
The item each point names is the one Kaleido_LoadItemNameStatic names in English (mm_pause_item_name_tests.py).
"""
from pathlib import Path
import argparse
import re
import subprocess
import hostcxx

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
args = parser.parse_args()
source, work = args.source.resolve(), args.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)


def function(text, signature):
    start = hostcxx.find(text, signature)
    brace = text.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end] + '\n'


def between(text, first, last, include_last=False):
    start = hostcxx.find(text, first)
    end = hostcxx.find(text, last, start)
    return text[start:end + (len(last) if include_last else 0)] + '\n'


collect = (source/'mm/src/overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_collect.c').read_text(encoding='utf-8')
header = (source/'mm/include/z64item.h').read_text(encoding='utf-8')
constants = {}
for block in (header.split('typedef enum ItemId {', 1)[1].split('} ItemId;', 1)[0],
              header.split('} UpgradeType;', 1)[1].split('} QuestItem;', 1)[0]):
    for value, name in re.findall(r'/\*\s*0x([0-9A-Fa-f]+)\s*\*/\s*((?:ITEM|QUEST)_\w+)', block):
        constants.setdefault(name, int(value, 16))

update = function(collect, 'void KaleidoScope_UpdateQuestCursor(PlayState* play) {')
links = between(update, 'static CursorPointDirection sCursorPointLinks[] = {', '};', True)
overrides = between(update, '#ifdef COMBO_BUILD', '#endif', True)
assert 'sCursorPointLinks[QUEST_SONG_OATH].right' in overrides, overrides
movement = between(update, 'oldCursorPoint = pauseCtx->cursorPoint[PAUSE_QUEST];', '// if the cursor point changed')
assert 'KaleidoScope_ComboQuestSongPlace(' in movement, 'the move no longer settles on the place\'s song'
item = between(update, '// Update cursor item and slot', 'pauseCtx->cursorSlot[pauseCtx->pageIndex] = cursor;', True)

code = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#define COMBO_BUILD 1
using s16 = int16_t; using u16 = uint16_t; using s32 = int32_t; using u32 = uint32_t; using f32 = float;
'''
code += ''.join(f'constexpr int {name} = 0x{value:X};\n' for name, value in sorted(constants.items()))
code += between(collect, 'typedef enum {\n    /* -3 */ CURSOR_TO_LEFT', '} CursorPointNext;', True)
code += between(collect, 'typedef struct {\n    /* 0x0 */ s16 up;', '} CursorPointDirection;', True)
code += r'''
constexpr int PAUSE_QUEST = 2, PAUSE_ITEM_NONE = 999, PAUSE_MAIN_STATE_IDLE = 0, PAUSE_MAIN_STATE_SONG_PROMPT = 4;
constexpr int PAUSE_CURSOR_PAGE_LEFT = 10, PAUSE_CURSOR_PAGE_RIGHT = 11, DO_ACTION_DECIDE = 5, DO_ACTION_INFO = 6;
constexpr int OCARINA_INSTRUMENT_OFF = 0, EQUIP_TYPE_SWORD = 0, EQUIP_TYPE_SHIELD = 1, EQUIP_VALUE_SWORD_NONE = 0;
constexpr int EQUIP_VALUE_SWORD_KOKIRI = 1, EQUIP_VALUE_SHIELD_NONE = 0, UPG_QUIVER = 0, UPG_BOMB_BAG = 1;
struct PauseContext {
    s16 cursorPoint[5] = {};
    u16 cursorItem[5] = {};
    s16 cursorSlot[5] = {};
    s16 pageIndex = PAUSE_QUEST, mainState = PAUSE_MAIN_STATE_IDLE, stickAdjX = 0, stickAdjY = 0;
    f32 cursorShrinkRate = 0;
} gPause;
struct InterfaceContext { s16 aButtonDoActionDelayed = 0; } gInterface;
struct PlayState {} gPlay;
u32 questItems = 0;
#define CHECK_QUEST_ITEM(item) ((questItems >> (item)) & 1)
#define GET_SAVE_INVENTORY_QUEST_ITEMS questItems
#define GET_CUR_EQUIP_VALUE(type) 0
#define GET_CUR_UPG_VALUE(type) 0
void AudioOcarina_SetInstrument(int) {}
void Interface_SetAButtonDoAction(PlayState*, int) {}
int specialPos = 0;
void KaleidoScope_MoveCursorToSpecialPos(PlayState*, int pos) { specialPos = pos; }
s32 KaleidoScope_UpdateQuestStatusPoint(PauseContext* pauseCtx, s16 point);
'''
code += function(collect, 'static s16 KaleidoScope_ComboQuestSongPlace(s16 point) {')
code += function(collect, 's32 KaleidoScope_UpdateQuestStatusPoint(PauseContext* pauseCtx, s16 point) {')
code += 'void Step(int dx, int dy) {\n    ' + links + r'''
    PauseContext* pauseCtx = &gPause;
    InterfaceContext* interfaceCtx = &gInterface;
    PlayState* play = &gPlay;
    s16 nextCursorPoint;
    s16 oldCursorPoint;
    u16 cursor;
    u16 cursorItem;
    (void)interfaceCtx;
    (void)play;
''' + overrides + r'''
    pauseCtx->stickAdjX = s16(dx);
    pauseCtx->stickAdjY = s16(dy);
''' + movement + '\n' + item + r'''
}
const int L = -40, R = 40, U = 40, D = -40;
// Move from `from` with the stick and check where the cursor lands and what it names.
void Move(int from, int dx, int dy, int to, int named) {
    gPause.cursorPoint[PAUSE_QUEST] = s16(from);
    Step(dx, dy);
    if (gPause.cursorPoint[PAUSE_QUEST] != to || gPause.cursorItem[PAUSE_QUEST] != named) {
        std::printf("from 0x%X (%d, %d): at 0x%X naming 0x%X, expected 0x%X naming 0x%X\n", from, dx, dy,
                    gPause.cursorPoint[PAUSE_QUEST], gPause.cursorItem[PAUSE_QUEST], to, named);
        assert(false);
    }
    assert(gPause.cursorSlot[PAUSE_QUEST] == to);
}
u32 Bit(int quest) { return 1u << quest; }
int main() {
    const int none = PAUSE_ITEM_NONE;
    // Sam's save: Song of Time, Saria's Song and Sun's Song, given with the Save Editor; Oath and Storms unlearned.
    questItems = Bit(QUEST_SONG_TIME) | Bit(QUEST_SONG_SARIA) | Bit(QUEST_SONG_SUN);
    Move(QUEST_SONG_SOARING, R, 0, QUEST_SONG_SUN, ITEM_SONG_SUN);
    Move(QUEST_SONG_SUN, 0, D, QUEST_SONG_SARIA, ITEM_SONG_SARIA);
    Move(QUEST_SONG_ELEGY, R, 0, QUEST_SONG_SARIA, ITEM_SONG_SARIA);
    Move(QUEST_QUIVER, L, 0, QUEST_SONG_SARIA, ITEM_SONG_SARIA);
    Move(QUEST_SWORD, L, 0, QUEST_SONG_SUN, ITEM_SONG_SUN);
    Move(QUEST_HEART_PIECE, 0, D, QUEST_SONG_SUN, ITEM_SONG_SUN);
    Move(QUEST_SONG_SARIA, 0, U, QUEST_SONG_SUN, ITEM_SONG_SUN);
    // Leaving a place whose MM song is unlearned does not stop on it.
    Move(QUEST_SONG_SARIA, L, 0, QUEST_SONG_ELEGY, none);
    Move(QUEST_SONG_SARIA, R, 0, QUEST_QUIVER, none);
    Move(QUEST_SONG_SUN, L, 0, QUEST_SONG_SOARING, none);
    Move(QUEST_SONG_SUN, R, 0, QUEST_SWORD, none);
    Move(QUEST_SONG_SUN, 0, U, QUEST_HEART_PIECE, none);
    // A cursor left on the place when the OoT song was learned names it without a move.
    Move(QUEST_SONG_OATH, 0, 0, QUEST_SONG_SARIA, ITEM_SONG_SARIA);
    Move(QUEST_SONG_STORMS, 0, 0, QUEST_SONG_SUN, ITEM_SONG_SUN);

    // Neither OoT song: vanilla moves, the unlearned places name nothing.
    questItems = Bit(QUEST_SONG_TIME);
    Move(QUEST_SONG_ELEGY, R, 0, QUEST_SONG_OATH, none);
    Move(QUEST_SONG_OATH, R, 0, QUEST_QUIVER, none);
    Move(QUEST_QUIVER, L, 0, QUEST_SONG_OATH, none);
    Move(QUEST_SONG_OATH, 0, U, QUEST_SONG_STORMS, none);
    Move(QUEST_SONG_STORMS, R, 0, QUEST_SWORD, none);
    Move(QUEST_SONG_STORMS, 0, D, QUEST_SONG_OATH, none);
    Move(QUEST_SONG_STORMS, 0, U, QUEST_HEART_PIECE, none);
    Move(QUEST_SONG_SOARING, R, 0, QUEST_SONG_STORMS, none);
    // The OoT songs' own points fall back to their place's MM song.
    Move(QUEST_SONG_SARIA, 0, 0, QUEST_SONG_OATH, none);
    Move(QUEST_SONG_SUN, 0, 0, QUEST_SONG_STORMS, none);

    // Only the MM songs: vanilla, named as before.
    questItems = Bit(QUEST_SONG_OATH) | Bit(QUEST_SONG_STORMS);
    Move(QUEST_SONG_ELEGY, R, 0, QUEST_SONG_OATH, ITEM_SONG_OATH);
    Move(QUEST_SONG_OATH, R, 0, QUEST_QUIVER, none);
    Move(QUEST_SONG_STORMS, R, 0, QUEST_SWORD, none);
    Move(QUEST_SONG_SOARING, R, 0, QUEST_SONG_STORMS, ITEM_SONG_STORMS);

    // Both songs of each place: the MM song first, the OoT song one step right, both reachable and named.
    questItems = Bit(QUEST_SONG_OATH) | Bit(QUEST_SONG_SARIA) | Bit(QUEST_SONG_STORMS) | Bit(QUEST_SONG_SUN);
    Move(QUEST_SONG_ELEGY, R, 0, QUEST_SONG_OATH, ITEM_SONG_OATH);
    Move(QUEST_SONG_OATH, R, 0, QUEST_SONG_SARIA, ITEM_SONG_SARIA);
    Move(QUEST_SONG_SARIA, R, 0, QUEST_QUIVER, none);
    Move(QUEST_SONG_SARIA, L, 0, QUEST_SONG_OATH, ITEM_SONG_OATH);
    Move(QUEST_SONG_OATH, L, 0, QUEST_SONG_ELEGY, none);
    Move(QUEST_SONG_STORMS, R, 0, QUEST_SONG_SUN, ITEM_SONG_SUN);
    Move(QUEST_SONG_SUN, R, 0, QUEST_SWORD, none);
    Move(QUEST_SONG_SUN, L, 0, QUEST_SONG_STORMS, ITEM_SONG_STORMS);
    Move(QUEST_SONG_SARIA, 0, U, QUEST_SONG_STORMS, ITEM_SONG_STORMS);
    Move(QUEST_SONG_SUN, 0, D, QUEST_SONG_OATH, ITEM_SONG_OATH);
    Move(QUEST_SONG_SARIA, 0, 0, QUEST_SONG_SARIA, ITEM_SONG_SARIA);

    // Elsewhere on the page nothing changed: the other songs and the page turn.
    questItems = Bit(QUEST_SONG_SARIA) | Bit(QUEST_SONG_ELEGY);
    Move(QUEST_SONG_BOSSA_NOVA, R, 0, QUEST_SONG_ELEGY, ITEM_SONG_ELEGY);
    specialPos = 0;
    gPause.cursorPoint[PAUSE_QUEST] = QUEST_SONG_SONATA;
    Step(L, 0);
    assert(specialPos == PAUSE_CURSOR_PAGE_LEFT);
    std::puts("MM Quest page: Saria's Song and Sun's Song are reached and named in Oath to Order's and Song of Storms' "
              "place; every other move is vanilla");
    return 0;
}
'''
test = work/'mm-quest-song-cursor.cpp'
test.write_text(code, encoding='utf-8')
exe = hostcxx.build(args.compiler, test, msvc=['/wd4100', '/wd4189'], gnu=['-Wno-unused-parameter', '-Wno-unused-variable'])
subprocess.run([str(exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)
