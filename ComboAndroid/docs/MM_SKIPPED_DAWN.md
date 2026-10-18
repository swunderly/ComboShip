# Reset dawn with an indoor initial entrance

The e0040456 DEV build reproduced an invalid Day 0 after an actual Song of Time
with Skip Song of Time cutscenes enabled. The MM session had resumed an owl save
inside Stone Tower Temple (entrance 0x2600). The song was played at 0xAA00 and
returned to 0x2600, but a later native diagnostic still reported day 0 and time
24888. With skipping disabled, the clock cutscene returned to the same entrance.
An actual moon crash with story skipping disabled also returned to 0x2600 with
native Day 0 (time 22680). The HUD first-day symbol did not establish the native
day value. The reproduction and screenshots are in the October 17 Codex workspace; this
document does not claim that the new APK has passed device verification.

The native skip sets the previous day just before dawn and normally relies on
EnTest4 in Clock Town to schedule DayTelop. An entrance-shuffle return can instead
land in a scene without that actor. The adapter now queues a one-shot native
DayTelop transition for skipped daytime starts and Day0 moon-crash returns,
consumed at the end of Play_Init.
DayTelop retains responsibility for first-day flag clearing and incrementing the
day. The adapter does not edit the day directly or spawn an actor.

An outdoor scene that already scheduled DayTelop is left alone. Nighttime clock
shuffle starts do not queue dawn. Recording a new MM boot clears pending work,
and consuming a request while the installed layout is inactive discards it.
No-ER and legacy profiles retain their existing path. Both ordinary and skipped
moon-crash returns queue dawn only for a restored Day0 main save, so a valid Day1
save cannot advance to Day2. The native moon-crash save restoration still runs.

The existing `entrance_er_any_runtime_sites_both_games` host test executes the
production adapter functions with engine boundaries stubbed. It covers daytime
starts for days 1, 2 and 3, one-shot consumption, an already scheduled transition,
new-boot clearing, inactive-layout clearing, ordinary moon-crash arrival from
Day0, and an already valid Day1 moon-crash save. Device verification must also
observe the native dawn screen and Day 1 at the recorded session entrance.
