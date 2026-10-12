#pragma once
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>

namespace ComboRando {
// 0 is deliberately unknown: old saves merged physical previews and hints.
// Those records must never be retroactively advertised as a physical sighting.
// 3 is OoT's stand-in: the check holds an item the player cannot obtain yet, so the game
// draws (and would give) a Blue Rupee instead. Only OoT records it; MM's save never holds 3.
constexpr uint8_t kTrackerSourceStandIn = 3;
struct TrackerKnowledge {
    uint8_t source = 0; // 1 = rendered observation, 2 = explicit item hint, 3 = rendered stand-in
    int32_t item = 0;
    std::string displayName;
};

inline bool TrackerKnowledgeSourceCanReplace(uint8_t previousSource, uint8_t source) {
    // A hint may describe a progressive family or the hidden trap placement.
    // It must not downgrade an actual observed model to a hint or change its tier.
    return (source == 1 || source == 2) && !(source == 2 && previousSource == 1);
}

// A saved OoT record is kept only with one of these sources.
inline bool TrackerKnowledgeSourceValid(uint8_t source) {
    return source >= 1 && source <= kTrackerSourceStandIn;
}

// The player looked at a model on screen: the placed item itself or its stand-in.
inline bool TrackerKnowledgeSeen(uint8_t source) {
    return source == 1 || source == kTrackerSourceStandIn;
}

// MM stores the enum item and provenance as fields in its save record. This
// shared helper gives that record the same precedence as OoT's named record.
template <typename ItemId>
inline bool RecordTrackerItemKnowledge(uint8_t& previousSource, ItemId& previousItem, uint8_t source, ItemId item) {
    if (!TrackerKnowledgeSourceCanReplace(previousSource, source) || static_cast<int32_t>(item) <= 0)
        return false;
    if (previousSource == source && previousItem == item)
        return false;
    previousSource = source;
    previousItem = item;
    return true;
}

inline bool RecordTrackerKnowledge(TrackerKnowledge& previous, uint8_t source, int32_t item, std::string displayName) {
    // A stand-in says nothing about the placed item. It only fills an empty record; any
    // later sighting of the real model or hint replaces it, and it never replaces either.
    const bool allowed = source == kTrackerSourceStandIn
                             ? previous.source == 0 || previous.source == kTrackerSourceStandIn
                             : TrackerKnowledgeSourceCanReplace(previous.source, source);
    if (!allowed || item <= 0 || displayName.empty())
        return false;
    // Hints describe the seed item, sometimes only its progressive family.
    // Never replace the exact tier/disguise the player has already looked at.
    if (previous.source == source && previous.item == item && previous.displayName == displayName)
        return false;
    previous = { source, item, std::move(displayName) };
    return true;
}

// A scene transition, the pause screen or a drawn screen fill hides the picture. A cutscene fade can
// leave the fill switched on after it faded to alpha 0, and Environment_FillScreen draws nothing then.
inline bool TrackerViewCovered(bool transitionActive, bool fillScreen, uint8_t fillAlpha, bool paused) {
    return transitionActive || (fillScreen && fillAlpha != 0) || paused;
}

// A submitted draw is insufficient: generous actor culling boxes extend off
// screen. Test the rendered model origin against the actual camera frustum.
inline bool TrackerItemInView(float x, float y, float z, float w, float horizontalScale, float scaleX2, float scaleY2,
                              float scaleZ2, bool viewCovered) {
    return !viewCovered && std::isfinite(w) && std::isfinite(x) && std::isfinite(y) && std::isfinite(z) &&
           std::isfinite(horizontalScale) && w > 0 && horizontalScale > 0 && scaleX2 >= 0.000001f &&
           scaleY2 >= 0.000001f && scaleZ2 >= 0.000001f && z >= -w && z <= w && std::fabs(x) <= w * horizontalScale &&
           std::fabs(y) <= w;
}
} // namespace ComboRando
