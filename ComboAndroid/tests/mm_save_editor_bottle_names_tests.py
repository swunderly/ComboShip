"""MM Save Editor: every bottle picker entry has MM's own English name, never "Unknown".

Compiles the production NativeBottleName (mm/2s2h/DeveloperTools/SaveEditor.cpp) against a message table in MM's
format (11-byte header, then the text) and checks it reads the name line of an item's pause description (message
0x1700 + item) the way the US archive stores it: a leading colour code, the name, then a line break or colour reset.
Blue Fire has no description and takes MM's English name graphic; the Hylian Loach, which MM names only in Japanese,
takes OoT's English name for the fish. Obaba's Drink, a leftover named only in Japanese, is not offered, as the
Longshot is not. SlotItemName prefers these names for bottle contents and keeps the randomizer's names elsewhere.
"""
import argparse
import re
import subprocess
from pathlib import Path
import hostcxx

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
a = p.parse_args()
root, out = a.source.resolve(), a.build_dir.resolve()
out.mkdir(parents=True, exist_ok=True)
editor = (root / 'mm/2s2h/DeveloperTools/SaveEditor.cpp').read_text(encoding='utf-8')


def function(source, signature):
    at = hostcxx.find(source, signature)
    depth, i = 0, source.index('{', at)
    while True:
        depth += (source[i] == '{') - (source[i] == '}')
        i += 1
        if depth == 0:
            return source[at:i] + '\n'


native = function(editor, 'static std::string NativeBottleName(ItemId item)')
slot_name = function(editor, 'static const char* SlotItemName(ItemId item)')
assert 'gItemSlots[item] == SLOT_BOTTLE_1 ? NativeBottleName(item)' in slot_name, 'bottle contents lost MM names'
assert 'Rando::StaticData::Items.find(' in slot_name, 'other items lost the randomizer names'
safe = function(editor, 'void initSafeItemsForInventorySlot()')
assert re.search(r'if \(i != ITEM_LONGSHOT && i != ITEM_OBABA_DRINK\)', safe), 'the bottle list offers a nameless leftover'

# MM's text bytes, written as C escapes: the colour codes 0x00 (reset) and 0x01 (red), 0x11 (line break), 0xB2 (the
# B button glyph) and 0xBF (end).
harness = r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
#define MESSAGE_HEADER_SIZE 11
enum ItemId { ITEM_BLUE_FIRE = 0x1C, ITEM_FISH = 0x1A, ITEM_MILK_HALF = 0x19, ITEM_HYLIAN_LOACH = 0x26,
              ITEM_OBABA_DRINK = 0x27, ITEM_MAGIC_BEANS = 0x0A, ITEM_MASK_GREAT_FAIRY = 0x40, ITEM_ZORA_EGG = 0x21 };
struct MessageTableEntry { u16 textId; u8 typePos; const char* segment; u32 msgSize; };
MessageTableEntry* sMessageTableNES = nullptr;
''' + native + r'''
#define TEXT(literal) std::string(literal, sizeof(literal) - 1)
std::vector<std::string> texts;
std::vector<u16> ids;
void Add(int item, const std::string& text) {
    ids.push_back(u16(0x1700 + item));
    texts.push_back(std::string(MESSAGE_HEADER_SIZE, '\0') + text);
}
int main() {
    assert(NativeBottleName(ITEM_FISH).empty()); // MM's text is not loaded yet
    // The pause descriptions as the US archive stores them.
    Add(ITEM_FISH, TEXT("\x01" "Fish\x11\x00" "It's freshly caught and still\x11" "jumping.\xbf"));
    Add(ITEM_MILK_HALF, TEXT("\x01" "Milk (1/2)\x11\x00" "Drink it with \xb2 to recover\x11" "five hearts.\xbf"));
    Add(ITEM_ZORA_EGG, TEXT("\x01" "Zora Egg\x11"));
    Add(ITEM_MAGIC_BEANS, TEXT("Magic Beans\x11" "Use \xb2 to plant them.\xbf"));
    Add(ITEM_MASK_GREAT_FAIRY, TEXT("\x01" "Great Fairy's Mask\x00 \x11" "The mask's hair\xbf"));
    Add(ITEM_BLUE_FIRE, TEXT("\xbf"));
    Add(ITEM_OBABA_DRINK, TEXT("\xbf"));
    std::vector<MessageTableEntry> table;
    for (size_t i = 0; i < texts.size(); i++)
        table.push_back({ ids[i], 0, texts[i].data(), u32(texts[i].size()) });
    table.push_back({ 0xFFFF, 0, nullptr, 0 });
    sMessageTableNES = table.data();
    assert(NativeBottleName(ITEM_FISH) == "Fish");
    assert(NativeBottleName(ITEM_MILK_HALF) == "Milk (1/2)");
    assert(NativeBottleName(ITEM_ZORA_EGG) == "Zora Egg");
    assert(NativeBottleName(ITEM_MAGIC_BEANS) == "Magic Beans");
    assert(NativeBottleName(ITEM_MASK_GREAT_FAIRY) == "Great Fairy's Mask");
    assert(NativeBottleName(ITEM_BLUE_FIRE) == "Blue Fire");
    assert(NativeBottleName(ITEM_HYLIAN_LOACH) == "Hyrule Loach");
    assert(NativeBottleName(ITEM_OBABA_DRINK).empty());
    std::cout << "PASS: MM bottle contents read their English names\n";
}
'''
(out / 'bottle_names.cpp').write_text(harness, encoding='utf-8')
exe = hostcxx.build(a.compiler, out / 'bottle_names.cpp', warnings=False, msvc=['/utf-8'])
subprocess.run([str(exe)], cwd=out, check=True, timeout=hostcxx.RUN_TIMEOUT)
