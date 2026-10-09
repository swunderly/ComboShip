# Combo Ship 0.2.17

The silver-gauntlet block in Gerudo Training Ground could fall into its pit without recording its permanent completion switch. Reloading the room then recreated the block in the corridor, potentially trapping Link after visiting the four-chest room.

The Android OoT engine now uses the same non-contracting floating-point arithmetic adopted by Shipwright's upstream fix (https://github.com/HarbourMasters/Shipwright/pull/6811). Fused multiply-add instructions were causing the collision test at the seated block's bottom edge to miss. The existing puzzle actor now records its normal switch, which preserves the block's position through room transitions and saved-game reloads.

Existing saves are supported. The save format, inventories, item placements, and progression logic are unchanged. A push that the older build failed to record cannot be inferred reliably from opened chests; push the block again after updating, then save normally. No new seed is needed.
