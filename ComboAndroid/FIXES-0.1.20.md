# Combo Ship 0.1.20 — native warp dialogue

The cross-game song popup now uses each game's own message renderer instead of an ImGui window. MM uses the native Soaring confirmation's message-box style, font, colors and selection cursor. OoT uses its native warp confirmation style. The grey menu backdrop and desktop-style buttons are gone; the touch controller remains available.

Confirm with A, choose No with the stick and A, or cancel with B. For Soaring in OoT, move left/right to cycle through the unlocked destinations, then confirm with the native OK/No choice. The last played note must be released before the dialogue opens so it cannot also confirm the warp.

This is a presentation/input update to 0.1.19. Song ownership, local instrument/buttons, owl unlocks, saved destinations and seed logic retain their existing rules. You do not need a new seed to receive this interface improvement; cross-game songs still require a seed that enabled them.

## Phone checks

1. Play Requiem in MM. Confirm the dialogue uses MM's native text box, there is no grey screen overlay, and touch controls remain visible.
2. Choose No or press B, replay the song, then confirm with A. Verify the named destination matches your arrival.
3. As Child Link in OoT, play Soaring. With multiple activated owls, cycle destinations left/right, cancel once, then confirm one.
4. Check your normal controller mappings and text-speed setting. Holding the final song note should never automatically accept the warp.

See [VALIDATION.md](VALIDATION.md) for the build and emulator checks. Physical-phone testing remains necessary.
