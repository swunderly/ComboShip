The `COMBO_ENTRANCE_DEV` build has two additional native MM console commands.
They are compiled out of SHIPPING builds and require stable normal gameplay,
an installed unified Wallmaster layout and native File3 (zero-based slot2).

`m1_mm_status` reports bounded metadata: scene, room, entrance, player position,
floor/grounded state, room-clear state, native Wallmaster soul/option/spawn policy,
actor counts and the game clock. It does not read a save file, query the logic
oracle, advance RNG or change game state.

`m1_well_room 1|5|7 clear|unclear` uses the in-play respawn sequence already used
by MM's native WarpPoint tool. It loads an authored Well room and positions the
player at one of its authored Wallmaster XZ locations, initially Y40. Native
room loading spawns its ordinary actor list. No actor is injected; no item is
granted; no configuration or save file is directly edited. The requested native
room-clear bit is set or cleared before transition. This is a disclosed developer
location/clear-state fixture, not evidence of reaching that room through trade
doors in ordinary gameplay.

Before crediting a capture, `m1_mm_status` must establish the requested scene/room,
actual clear state and a stable grounded position. The ceiling actor's authored Y
is not a player-floor value. Preserve an invalid landing as invalid and stop;
never turn the requested coordinates or command acknowledgment into a success
claim. Use the real Save Editor soul grant/removal for soul controls. A successful
transport still requires the actual natural hand and recorded destination, and
the fade must be observed separately. Files1/2 and the original frozen e02/F4
sources, builds, APKs and receipts remain protected.
