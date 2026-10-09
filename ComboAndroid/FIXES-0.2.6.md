# Combo Ship 0.2.6

OoT's Check Tracker now records the names of visible shop rewards, including ordinary refill items. Shop descriptions also save the item information when you browse an offer.

Three related gaps caused shop slots to be marked **Known** without an item name: ordinary shop models bypassed the observation hook, invisible shelf interaction collision rejected visible models, and reading a shop offer saved its price status without recording the reward description.

Visible shelf rewards now use the camera visibility check. Ordinary world items retain their wall and occlusion checks. Shop quantities are preserved, so a 30-arrow offer is recorded as **Buy Arrows (30)**. Reading a description preserves an existing physical observation instead of replacing it with a hint.

Old incomplete records are no longer presented as known rewards. Their collection, skipped and price status are retained. Look at an uncollected shop item again or browse its offer to record its name. Mystery items and undisclosed merchant rewards remain hidden; learning a price alone does not count as learning the reward.

Install over the previous APK; existing seeds and saves work. No new seed is required. See [validation](VALIDATION-0.2.6.md) for the focused emulator and build checks.
