# Combo Ship 0.2.3 request audit

This release covers the October 3 tracker requests and retains the menu organization from 0.2.2.

| Request | Implemented behavior |
| --- | --- |
| Check Tracker default while paused | Fresh preferences use pause access. Explicit saved visibility and shortcuts are preserved. |
| Understand controller modifiers | Tooltip explains virtual modifiers, physical assignment and simultaneous button combinations, including actual default shoulder/trigger mapping. |
| Separate touch tracker buttons | CT, IT and HT are independently enabled by each tracker's On-screen button mode. They work when ordinary touch controls are hidden. |
| Move and resize tracker buttons | Existing Move controls editor handles all three; individual size sliders and per-profile persistence are added. |
| Uniform Item Tracker | Shared settings, access, appearance and side panel; native item ownership/counts retained with compact groups in both games. |
| Uniform Hint Tracker | Shared settings, access, appearance and side panel; grouped combined hints with selectable, spoiler-safe detail popups. |
| Discover tracker controls | CT/IT/HT searches open the matching page; visibility, shortcut, modifier and on-screen terms are indexed. |

Tests exercise production visibility state, real Android MotionEvents, native JNI callbacks, both games' tracker content, known/unknown hint presentation, button persistence, search destinations and reduced-width layouts. Emulator fixtures are described in [VALIDATION-0.2.3.md](VALIDATION-0.2.3.md). This is not a physical-controller or full-playthrough certification.

The [0.2.2 audit](RELEASE-AUDIT-0.2.2.md) remains the record for starting inventory, Gameplay rules, Bremen count, OoT minigames and Camera navigation.
