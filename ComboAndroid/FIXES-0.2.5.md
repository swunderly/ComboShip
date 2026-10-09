# Combo Ship 0.2.5

OoT's Check Tracker now records the displayed reward from a killed Gold Skulltula before you collect it.

The Skulltula reward's randomized drawing function was missing the observation hook already used by other visible items. It now calls the same observer, so a visible Piece of Heart is recorded as **Seen: Piece of Heart**, with the check still uncollected.

The existing visibility and spoiler protections apply: being in the same room is insufficient, off-screen or occluded rewards are excluded, and Mysterious Shuffle does not reveal the hidden item. The saved item name survives restarting the app.

Install over the previous APK. Existing seeds and saves work; no new seed is required. The tracker cannot reconstruct an item you saw before this update, so look at an uncollected drop again to record it.

This change uses the shared OoT Gold Skulltula reward handler, covering its overworld and dungeon drops. The approved menus and 0.2.4 warp presentation are retained. See [validation](VALIDATION-0.2.5.md) for the focused emulator and build checks.
