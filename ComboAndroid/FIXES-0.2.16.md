# Combo Ship 0.2.16

OoT could lose the names and provenance of seen items after returning from MM or using Return to Spawn. This affected the entire knowledge store, including shop items, Skulltula rewards, freestanding items, and foreign MM rewards seen in OoT. These paths load the save before firing an exit hook; that hook incorrectly cleared the newly loaded item knowledge. A subsequent tracker write then persisted the empty names.

The exit hook now cleans up the tracker view while retaining the loaded save's knowledge. Starting a new file or loading a file still clears the previous profile's knowledge before reading the new one. Physical sightings, explicit item hints, and the exact item appearance remain distinct.

The save format, randomizer placements, progression logic, and inventory are unchanged. Existing saves remain supported, including the MM compatibility fix in 0.2.15. Knowledge already erased by an earlier build cannot be reconstructed reliably; revisiting the item records it again. The fix does not disclose unseen rewards.
