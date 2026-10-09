# Shared editors and techniques catalogue

## Numeric controls

Both save editors use sliders for bounded counters and amounts. This extends the
existing health, magic and rupee sliders to ammo, OoT small keys, elapsed days,
death counts, sword durability, trade days, Navi/timer values, Triforce pieces,
fish records and minigame scores. Invincibility timers and rotations also use
sliders. Values in narrow save fields are edited through an integer local and
written back only after interaction, so an untouched editor does not rewrite an
unusual pre-existing value.

Native menu/save sliders and the shared renderer disable numeric keyboard entry.
Custom resolution pixel counts also use bounded sliders. Controller shortcuts
use named A/B/C/D-pad/L/R/Z/Start choices rather than an integer button mask.
Color pickers keep their visual chooser without numeric fields.

The deliberately advanced save/debugger sections retain raw item/entrance/
cutscene/register IDs, encoded name bytes, and unrestricted floating world
coordinates or physics values. Network addresses and precise port numbers retain
text entry. Those fields have no useful ordinary gameplay slider range and are
not required to edit health, inventory amounts, rupees or HUD layout.

## HUD layout

Screen & HUD now uses one editor for both worlds: choose an element, choose its
anchor/visibility, drag a position guide or adjust horizontal/vertical sliders,
and reset that element or all HUD positions. The same UI maps to each engine's
existing saved CVars, so installed layouts remain readable. Resizing is offered
only for elements whose renderer implements it; fixed-size elements say so.
The guide is a placement aid; minimaps and conditional timers still depend on
the current area and gameplay state. Touch-controller buttons remain editable
by holding Menu.

## Techniques catalogue

One searchable catalogue renders actual engine-provided trick entries from both
worlds. Filter by game, enabled state or difficulty and expand each entry's
description. Search includes the name, prerequisites, area and tags. OoT retains
its native enabled-trick CSV and context options; unimplemented empty metadata
slots are not exposed. MM metadata is provided by its verified logic hooks.
General movement enhancements remain on Gameplay rules instead of being
mislabelled as techniques. Loading or overriding a run's trick CSV updates the
OoT context without resetting items or the scene.

## Verification

Standalone MSVC `/W4 /WX` model tests passed for HUD native-mode mappings,
coordinate conversion across screen shapes, edge/center anchors, malformed aspect
ratios, and combined catalogue search/filter behavior. Both native patches pass
reverse application checks against their saved baselines. APK/native compilation
and live emulator checks are recorded in the release's overall validation report.

On the October 2 candidate APK, a native HUD element was moved using the same
guide in OoT and MM, visibly moved in each game's live render, and reset without
leaving placement overrides. Rupee sliders changed OoT to 81 and MM to 64;
MM's bank slider was also adjusted. No numeric keyboard opened for these edits.
An actual OoT pause-menu save and cold restart retained the 81 rupees and tested
Gameplay rule overrides. Global `warp` search reached the shared rules page.
The catalogue showed 197 OoT and two MM techniques; searching `lens`, selecting
MM and opening a description left the two verified MM entries. These are
emulator interaction tests, not physical-phone or complete-playthrough results.
