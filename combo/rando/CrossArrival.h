#pragma once
/* Native slots: 0..2. Game: OoT 0, MM 1. Ages: adult 0, child 1. */
enum ComboNativeArrivalReason { COMBO_ARRIVAL_FARORE = 0, COMBO_ARRIVAL_DUNGEON = 1 };
/* Return 1 only after the same-slot destination arrival has been armed.
 * Return 0 without changing foreground/slot state if preflight or arming fails.
 * No song ID, forced form, or synthetic entrance is accepted by this interface. */
typedef int (*ComboNativeDepartureCommit)(int fileNum, int destinationGame, int reason, int age, int actualEntrance);
