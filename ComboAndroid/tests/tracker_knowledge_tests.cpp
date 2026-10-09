#include "../rando/TrackerKnowledge.h"
#include <cassert>
#include <iostream>
#include <limits>
using namespace ComboRando;
int main() {
    TrackerKnowledge knowledge;
    assert(!RecordTrackerKnowledge(knowledge, 0, 1, "Secret reward"));
    assert(!RecordTrackerKnowledge(knowledge, 2, 0, "Unknown"));
    assert(!RecordTrackerKnowledge(knowledge, 2, 1, ""));
    assert(RecordTrackerKnowledge(knowledge, 2, 10, "Progressive Hookshot"));
    assert(knowledge.source == 2 && knowledge.displayName == "Progressive Hookshot");
    assert(RecordTrackerKnowledge(knowledge, 1, 11, "Hookshot"));
    assert(knowledge.source == 1 && knowledge.item == 11 && knowledge.displayName == "Hookshot");
    assert(!RecordTrackerKnowledge(knowledge, 2, 10, "Progressive Hookshot"));
    assert(knowledge.item == 11 && knowledge.displayName == "Hookshot");
    assert(!RecordTrackerKnowledge(knowledge, 1, 11, "Hookshot"));
    // Keep the actually observed new tier only after a new physical preview.
    assert(RecordTrackerKnowledge(knowledge, 1, 12, "Longshot"));
    const TrackerKnowledge saved = knowledge;
    knowledge = {};
    assert(knowledge.source == 0 && knowledge.item == 0 && knowledge.displayName.empty());
    knowledge = saved;
    assert(knowledge.source == 1 && knowledge.item == 12 && knowledge.displayName == "Longshot");
    // Disguised trap models are kept as displayed; hints never replace them.
    assert(RecordTrackerKnowledge(knowledge, 1, 13, "Kokiri Sword"));
    assert(!RecordTrackerKnowledge(knowledge, 2, 99, "Ice Trap"));
    assert(knowledge.displayName == "Kokiri Sword");
    // The production MM save-field helper must preserve the displayed tier
    // and disguised trap model even when a later hint resolves differently.
    uint8_t mmSource = 0;
    int32_t mmItem = 0;
    assert(!RecordTrackerItemKnowledge(mmSource, mmItem, 0, 10));
    assert(!RecordTrackerItemKnowledge(mmSource, mmItem, 3, 10));
    assert(!RecordTrackerItemKnowledge(mmSource, mmItem, 255, 10));
    assert(!RecordTrackerItemKnowledge(mmSource, mmItem, 1, 0));
    assert(RecordTrackerItemKnowledge(mmSource, mmItem, 2, 10));
    assert(mmSource == 2 && mmItem == 10);
    assert(RecordTrackerItemKnowledge(mmSource, mmItem, 1, 11));
    assert(!RecordTrackerItemKnowledge(mmSource, mmItem, 2, 10));
    assert(mmSource == 1 && mmItem == 11);
    assert(!RecordTrackerItemKnowledge(mmSource, mmItem, 1, 11));
    assert(RecordTrackerItemKnowledge(mmSource, mmItem, 1, 12));
    assert(mmSource == 1 && mmItem == 12);
    assert(RecordTrackerItemKnowledge(mmSource, mmItem, 1, 13));
    assert(!RecordTrackerItemKnowledge(mmSource, mmItem, 2, 99));
    assert(mmSource == 1 && mmItem == 13);
    auto visible = [](float x, float y, float z, float w, float aspect = 1,
                      float scale = 1, bool covered = false) {
        return TrackerItemInView(x,y,z,w,aspect,scale,scale,scale,covered);
    };
    assert(visible(0,0,0,1));
    assert(!visible(2,0,0,1)); // large actor culling volume, model off screen
    assert(!visible(0,2,0,1));
    assert(!visible(0,0,2,1));
    assert(!visible(0,0,-2,1));
    assert(!visible(0,0,0,-1)); // behind camera
    assert(!visible(0,0,0,0));
    assert(!visible(0,0,0,1,1,0)); // unrevealed zero-scale model
    assert(!visible(0,0,0,1,1,1,true)); // transition/fade/pause covers picture
    assert(visible(1.5f,0,0,1,2)); // genuinely visible widescreen side
    assert(!visible(.8f,0,0,1,.5f)); // cropped narrow viewport
    assert(!visible(std::numeric_limits<float>::quiet_NaN(),0,0,1));
    assert(!visible(0,0,0,std::numeric_limits<float>::infinity()));
    assert(!visible(0,0,0,1,0));
    std::cout << "PASS: tracker observations, hint precedence, exact tiers/disguises, reset and camera visibility\n";
}
