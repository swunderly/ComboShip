# Combo Ship 0.2.12

Logic follows the OoTMM v32.3 source for the supported wallet and time behaviors below.

- Logical affordability uses wallet capacity and access to rupees. A current balance of 19 versus 20 does not change logical reachability. Free checks stay free.
- OoT paid activities retain their native fees and equipment rules, and now require a reachable money source. This includes galleries, fishing, diving, treasure chest game, Dampe, Talon, the castle/GTG fees, and paid Epona routes. Skipping Epona's race retains its free root event.
- MM entrance and connection searches carry only the arrival times permitted by each route. Opening a room during the day no longer grants access at midnight after it closes.
- Time-dependent checks and events evaluate one consistent time at a time. Waiting, later entry, room keys, alternative entrances, and all three clock modes remain usable.
- Finding a shuffled clock expands root access and recalculates room access through valid entrances.

Both the native generation searches and check trackers use these rules. Menus and existing saves retain their formats. This update does not add unsupported website features.

See [the source comparison and evidence boundaries](SUPPORTED-LOGIC-0.2.12.md).
