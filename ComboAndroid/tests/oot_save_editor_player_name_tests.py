"""OoT Save Editor: Player Name is a text field, as MM's is, for PAL and NTSC files alike.

Compiles the production name helpers of soh/soh/Enhancements/debugger/debugSaveEditor.cpp (PAL's z2ASCII/ASCII2z,
the NTSC table decodeNTSCPlayerNameChar, and the field's PlayerNameChar, PlayerNameCode, NextPlayerNameChar and
ApplyPlayerNameText) against a stub save and checks that typed text becomes OoT's font codes: NTSC letters after the
kana (A = 171, a = 197, space = 223), kana and digits from the table, PAL letters after the digits (A = 10, a = 36,
space = 62). A character the font lacks becomes a space, and a character the field cannot show (a code that decodes to
"?") keeps its code while the rest of the name is edited. The General tab draws that field, not the per-code editors.
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
editor = (root / 'soh/soh/Enhancements/debugger/debugSaveEditor.cpp').read_text(encoding='utf-8')


def function(signature):
    at = hostcxx.find(editor, signature)
    depth, i = 0, editor.index('{', at)
    while True:
        depth += (editor[i] == '{') - (editor[i] == '}')
        i += 1
        if depth == 0:
            return editor[at:i] + '\n'


production = ''.join(function(signature) for signature in (
    'char z2ASCII(int code)', 'static uint8_t ASCII2z(char c)', 'std::string decodeNTSCPlayerNameChar(int code)',
    'static std::string PlayerNameChar(uint8_t code, bool pal)',
    'static uint8_t PlayerNameCode(const std::string& typed, bool pal)',
    'static std::string NextPlayerNameChar(const char*& text)',
    'static void ApplyPlayerNameText(const char* text, bool pal)'))
tab = function('void DrawInfoTab()')
assert 'ImGui::InputText("##playerName"' in tab and 'ApplyPlayerNameText(nameBuffer, pal);' in tab, \
    'Player Name is not the text field'
assert 'KeepEightPlayerNameChars' in tab, 'the field can hold more than eight characters'
assert not re.search(r'InputScalar\([^;]*playerName', tab), 'the General tab still edits name codes one by one'

harness = r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
struct { uint8_t playerName[8]; } gSaveContext;
''' + production + r'''
std::string Codes() {
    std::string text;
    for (uint8_t code : gSaveContext.playerName)
        text += std::to_string(code) + " ";
    return text;
}
void Expect(const uint8_t (&codes)[8]) {
    if (std::memcmp(gSaveContext.playerName, codes, 8)) {
        std::cerr << "codes " << Codes() << '\n';
        assert(false);
    }
}
std::string Shown(bool pal) {
    std::string text;
    for (uint8_t code : gSaveContext.playerName)
        text += PlayerNameChar(code, pal);
    return text;
}
int main() {
    // NTSC: Link, shown as text and retyped as Zelda.
    const uint8_t link[8] = { 182, 205, 210, 207, 223, 223, 223, 223 };
    std::memcpy(gSaveContext.playerName, link, 8);
    assert(Shown(false) == "Link    ");
    ApplyPlayerNameText("Zelda", false);
    Expect({ 196, 201, 208, 200, 197, 223, 223, 223 });
    assert(Shown(false) == "Zelda   ");
    // Kana and digits come from the NTSC table; a character the font lacks becomes a space.
    ApplyPlayerNameText("\xe3\x81\x82\xe3\x81\x84" "12-.!", false);
    Expect({ 10, 11, 1, 2, 228, 234, 223, 223 });
    // A code the field shows as "?" keeps its code while the rest of the name changes.
    const uint8_t odd[8] = { 230, 205, 210, 207, 223, 223, 223, 223 };
    std::memcpy(gSaveContext.playerName, odd, 8);
    assert(Shown(false) == "?ink    ");
    ApplyPlayerNameText("?inks", false);
    Expect({ 230, 205, 210, 207, 215, 223, 223, 223 });
    ApplyPlayerNameText("Kink", false);
    Expect({ 181, 205, 210, 207, 223, 223, 223, 223 });
    // Eight characters at most are read.
    ApplyPlayerNameText("ABCDEFGHIJ", false);
    Expect({ 171, 172, 173, 174, 175, 176, 177, 178 });
    // PAL: letters after the digits, space 62, '-' and '.' 63 and 64.
    ApplyPlayerNameText("Link", true);
    Expect({ 21, 44, 49, 46, 62, 62, 62, 62 });
    assert(Shown(true) == "Link    ");
    ApplyPlayerNameText("Ab-.9", true);
    Expect({ 10, 37, 63, 64, 9, 62, 62, 62 });
    std::cout << "PASS: OoT Player Name text maps to the file's font codes\n";
}
'''
(out / 'player_name.cpp').write_text(harness, encoding='utf-8')
exe = hostcxx.build(a.compiler, out / 'player_name.cpp', warnings=False, msvc=['/utf-8'])
subprocess.run([str(exe)], cwd=out, check=True, timeout=hostcxx.RUN_TIMEOUT)
