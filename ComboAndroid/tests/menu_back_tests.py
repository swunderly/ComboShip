"""Back inside the open menu takes one press per step, with the bundled Dear ImGui and the production handler.

Compiles ComboMenu.cpp's production HandleBack, ClearSearch and ReturnToSearchResults against stub engine types:
- after "Open page" on a search result, one Back returns to the results;
- Back clears a search whose text field is still being edited after its keyboard closed (the field would otherwise
  write its text back on the next frame).
GameActivity must route Back by the window's own keyboard visibility on every API level from minSdk 28: SDL's flag
stays set after the keyboard closes itself on Back, which sent the next Back to SDL as an unused key press.
"""
import argparse
import re
import subprocess
from pathlib import Path
import hostcxx

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--imgui', type=Path, required=True)
parser.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
args = parser.parse_args()
source, out = args.source.resolve(), args.build_dir.resolve()
out.mkdir(parents=True, exist_ok=True)
menu = (source / 'combo/gui/ComboMenu.cpp').read_text(encoding='utf-8')


def block(signature, text=menu):
    start = hostcxx.find(text, signature)
    end = text.index('{', start) + 1
    depth = 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end] + '\n'


production = block('void ComboMenu::ClearSearch()') + block('void ComboMenu::ReturnToSearchResults()') + \
    block('bool ComboMenu::HandleBack()')

harness = r'''
#include <imgui_internal.h>
#include "ComboMenuSearch.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
namespace Ship {
struct Gui { ImGuiContext* GetImGuiContext() { return ImGui::GetCurrentContext(); } };
struct Window { Gui* GetGui() { return nullptr; } };
struct Context { static Context* GetRawInstance() { return nullptr; } Window* GetWindow() { return nullptr; } };
} // namespace Ship
namespace ComboRando {
class ComboMenu {
  public:
    bool IsVisible() const { return true; }
    bool HandleBack();
    void ClearSearch();
    void ReturnToSearchResults();
    char mSearchBuf[256] = { 0 };
    ImGuiID mSearchField = 0;
    std::string mSearchQuery, mScope, mHubActive, mLastNavSig;
    MenuRuleFocus mRuleFocus;
    MenuSearchReturn mSearchReturn;
    bool mResetContentScroll = false;
};
''' + production + r'''
} // namespace ComboRando
using namespace ComboRando;

int main() {
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = ImVec2(640, 480);
    io.ConfigInputTrickleEventQueue = false;
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    ComboMenu menu;
    ImGuiID field = 0;
    // One menu frame: Back (when pressed) is handled before the menu draws, as Gui::DrawMenu does.
    auto frame = [&](bool back, bool focus, const char* typed) -> bool {
        if (typed)
            io.AddInputCharactersUTF8(typed);
        ImGui::NewFrame();
        const bool handled = back && menu.HandleBack();
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(640, 480));
        ImGui::Begin("Combo Menu");
        if (focus)
            ImGui::SetKeyboardFocusHere();
        ImGui::InputTextWithHint("##ComboSearch", "Search all settings...", menu.mSearchBuf, sizeof(menu.mSearchBuf));
        field = menu.mSearchField = ImGui::GetItemID();
        ImGui::End();
        ImGui::Render();
        return handled;
    };

    // Searching: the field is being edited and holds "trap"; its keyboard has closed, the cursor is still in it.
    menu.mScope = "randomizer";
    menu.mHubActive = "randomizer/Hints & traps";
    frame(false, true, nullptr);
    frame(false, false, nullptr);
    frame(false, false, "trap");
    frame(false, false, nullptr);
    assert(!std::strcmp(menu.mSearchBuf, "trap") && ImGui::GetActiveID() == field);
    // One Back clears the search and ends the edit, and the text stays cleared.
    assert(frame(true, false, nullptr));
    assert(menu.mSearchBuf[0] == 0 && ImGui::GetActiveID() != field);
    frame(false, false, nullptr);
    assert(menu.mSearchBuf[0] == 0 && !menu.mSearchReturn.HasResult());
    std::cout << "One Back clears a search whose field is still being edited\n";

    // Open page on a result: the search text goes, the page opens, and one Back returns to the results.
    std::snprintf(menu.mSearchBuf, sizeof(menu.mSearchBuf), "trap");
    menu.mSearchReturn.Capture(menu.mSearchBuf, menu.mScope, menu.mHubActive);
    menu.mSearchBuf[0] = 0;
    menu.mScope = "gameplay";
    menu.mHubActive = "gameplay/Randomizer extras";
    frame(false, false, nullptr);
    assert(frame(true, false, nullptr));
    assert(!std::strcmp(menu.mSearchBuf, "trap") && menu.mScope == "randomizer" &&
           menu.mHubActive == "randomizer/Hints & traps" && !menu.mSearchReturn.HasResult());
    // The next Back clears the restored search; the one after that leaves only closing the menu.
    assert(frame(true, false, nullptr) && menu.mSearchBuf[0] == 0);
    assert(!frame(true, false, nullptr));
    std::cout << "One Back after Open page returns to the search results\n";
    ImGui::DestroyContext();
}
'''
(out / 'menu_back.cpp').write_text(harness, encoding='utf-8')
exe = hostcxx.build(args.compiler, out / 'menu_back.cpp', warnings=False,
                    includes=[source / 'combo/gui', source / 'combo', args.imgui],
                    sources=[args.imgui / file for file in
                             ['imgui.cpp', 'imgui_draw.cpp', 'imgui_tables.cpp', 'imgui_widgets.cpp']],
                    msvc=['/Zc:preprocessor', '/utf-8'])
subprocess.run([str(exe)], cwd=out, check=True, timeout=hostcxx.RUN_TIMEOUT)

# Android Back reaches the menu whenever the keyboard is not on screen.
activity = (source / 'ComboAndroid/app/src/main/java/org/comboship/android/GameActivity.java').read_text(encoding='utf-8')
dispatch = block('@Override public boolean dispatchKeyEvent(KeyEvent event)', activity)
assert re.search(r'KEYCODE_BACK\s*&&\s*!screenKeyboardShown\(\)', dispatch), 'Back is not routed by the shown keyboard'
assert 'SDLActivity.isScreenKeyboardShown()' not in dispatch, "Back is routed by SDL's keyboard flag"
shown = block('private boolean screenKeyboardShown()', activity)
assert 'ViewCompat.getRootWindowInsets(' in shown and 'WindowInsetsCompat.Type.ime()' in shown, \
    "the window's keyboard visibility must decide on every API level, not only from API 30"
# minSdk is 28: below API 30 SDL's flag may only confirm the window's answer; alone it decides only without insets.
assert re.search(r'if\(insets==null\)return SDLActivity\.isScreenKeyboardShown\(\);', shown) and \
    re.search(r'ime\s*&&\s*SDLActivity\.isScreenKeyboardShown\(\)', shown) and \
    shown.count('SDLActivity.isScreenKeyboardShown()') == 2, "SDL's stale flag still decides Back below API 30"
print('PASS: Back takes one press per menu step')
