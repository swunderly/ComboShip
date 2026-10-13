#include "ComboCheckTrackerModel.h"
#include <cassert>
#include <iostream>
using namespace ComboChecks;
int main() {
    VisibilityState v;
    auto update = [&](uint32_t held, int game = 0, bool menu = false, bool loaded = true) {
        return v.Update(game, Toggle, 0x30, held, true, loaded, false, menu);
    };
    assert(!update(0));
    assert(!update(0x20)); // half a chord does not open it
    assert(update(0x30)); // L + R rising edge
    for (int i = 0; i < 120; ++i) assert(update(0x30)); // holding does not flicker
    assert(update(0));
    assert(!update(0x30)); // same action closes in OoT
    assert(!update(0x30, 4)); // held across game transition never reopens
    assert(!update(0, 4));
    assert(update(0x30, 4)); // identical in MM
    assert(update(0, 4, true));
    assert(update(0x30, 4, true)); // pressing in settings does not toggle
    assert(update(0x30, 4)); // or when settings closes while still held
    assert(update(0, 4));
    assert(!update(0x30, 4));
    assert(!update(0x30, -1, false, false));
    assert(!update(0x30, 1)); // slot load needs a fresh press
    assert(!update(0, 1));
    assert(update(0x30, 1));
    assert(!v.Update(1, Hold, 0x20, 0, true, true, false, false));
    assert(v.Update(1, Hold, 0x20, 0x20, true, true, false, false));
    assert(!v.Update(1, Hold, 0x20, 0x20, true, true, false, true));
    assert(!v.Update(1, Hold, 0, 0, true, true, false, false)); // empty binding isn't always held
    assert(!v.Update(1, Paused, 0, 0, true, true, false, false));
    assert(v.Update(1, Paused, 0, 0, true, true, true, false));
    assert(v.Update(1, Always, 0, 0, true, true, false, false));
    assert(!v.Update(1, Always, 0, 0, false, true, false, false));
    assert(!v.Update(1, Always, 0, 0, true, false, false, false));

    VisibilityState touch, item, hint;
    assert(!touch.Update(0,OnScreen,0x20,0x20,true,true,false,false)); // controller L doesn't toggle CT
    assert(touch.Update(0,OnScreen,0x20,0,true,true,false,false,true));
    assert(touch.Update(4,OnScreen,0x20,0,true,true,false,false)); // open tracker follows game
    assert(!item.Update(4,OnScreen,0,0,true,true,false,false));
    assert(!hint.Update(4,OnScreen,0,0,true,true,false,false)); // CT never opens IT/HT
    assert(!touch.Update(4,OnScreen,0,0,true,true,false,false,true));
    assert(!touch.Update(4,OnScreen,0,0,true,true,false,true,true)); // settings block touches
    assert(!touch.Update(-1,OnScreen,0,0,true,false,false,false,true)); // file-select cannot open
    assert(!touch.Update(4,OnScreen,0,0,false,true,false,false,true)); // disabled tracker cannot open
    for (auto* state : {&touch,&item,&hint}) {
        assert(state->Update(4,Paused,0,0,true,true,true,false));
        assert(!state->Update(4,Paused,0,0,true,true,false,false)); // pause mode applies identically
    }

    Totals t;
    t.Add(COMBO_CHECK_COLLECTED | COMBO_CHECK_SKIPPED); // obtained takes precedence
    t.Add(COMBO_CHECK_SKIPPED);
    t.Add(COMBO_CHECK_SCUMMED); // unsaved items are still outstanding
    t.Add(COMBO_CHECK_SEEN);
    t.Add(COMBO_CHECK_AREA_HIDDEN); // no secret dungeon-count disclosure
    assert(t.total == 4 && t.collected == 1 && t.skipped == 1 && t.hiddenArea);
    UniqueTotals unique;
    unique.Add(12, COMBO_CHECK_SKIPPED);
    unique.Add(12, COMBO_CHECK_SKIPPED); // same postbox in another area
    unique.Add(12, COMBO_CHECK_SKIPPED);
    unique.Add(13, COMBO_CHECK_COLLECTED);
    unique.Add(13, COMBO_CHECK_COLLECTED);
    unique.Add(-1, COMBO_CHECK_AREA_HIDDEN);
    assert(unique.total == 2 && unique.skipped == 1 && unique.collected == 1 && unique.hiddenArea);
    unique = {}; // each snapshot starts a fresh count
    unique.Add(12, 0);
    assert(unique.total == 1 && unique.skipped == 0 && !unique.hiddenArea);
    assert(!PassesFilters(COMBO_CHECK_COLLECTED, false, true, false));
    assert(!PassesFilters(COMBO_CHECK_SKIPPED, true, false, false));
    assert(PassesFilters(COMBO_CHECK_SCUMMED, false, false, false));
    assert(PassesFilters(COMBO_CHECK_SEEN, false, false, false));
    assert(!PassesFilters(COMBO_CHECK_LOGIC_KNOWN, false, false, true));
    assert(PassesFilters(COMBO_CHECK_LOGIC_KNOWN | COMBO_CHECK_AVAILABLE, false, false, true));
    assert(PassesFilters(COMBO_CHECK_COLLECTED | COMBO_CHECK_LOGIC_KNOWN, true, true, true));
    assert(!PassesFilters(COMBO_CHECK_AREA_HIDDEN, true, true, true));

    // Touches over the trackers: a side panel on the right half, the Check details popup against its right edge.
    float boxes[3][5][4]{};
    const auto place = [](float* b, float x0, float y0, float x1, float y1) {
        b[0] = x0;
        b[1] = y0;
        b[2] = x1;
        b[3] = y1;
    };
    assert(TrackerTouchAt(boxes, true, .8f, .4f) == TouchGame); // no tracker open
    place(boxes[0][0], .5f, 0, 1, 1);
    assert(TrackerTouchAt(boxes, false, .835f, .39f) == TouchTracker); // during play the tracker takes it (0.1.11)
    assert(TrackerTouchAt(boxes, true, .835f, .39f) == TouchTrackerUnderButtons); // pause screen: C buttons win
    assert(TrackerTouchAt(boxes, true, .2f, .39f) == TouchGame);                  // beside the panel
    place(boxes[0][1], .7f, .3f, 1, .5f);
    assert(TrackerTouchAt(boxes, true, .835f, .39f) == TouchTracker);            // a popup always keeps its touches
    assert(TrackerTouchAt(boxes, true, .78f, .81f) == TouchTrackerUnderButtons); // the window outside it does not
    boxes[0][1][2] = boxes[0][1][0];                                             // an empty popup box claims nothing
    assert(TrackerTouchAt(boxes, true, .835f, .39f) == TouchTrackerUnderButtons);
    place(boxes[2][2], .1f, .1f, .9f, .9f);
    assert(TrackerTouchAt(boxes, true, .835f, .39f) == TouchTracker); // another tracker's popup over this window
    assert(TrackerTouchAt(boxes, false, .2f, .39f) == TouchTracker);  // and over the game
    std::cout << "PASS: shared tracker toggle, hold, pause, transitions, filters, totals and touches\n";
}
