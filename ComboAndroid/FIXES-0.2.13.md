# Combo Ship 0.2.13

Closes the remaining resource-access gaps identified in the supported logic comparison with OoTMM v32.3.

- MM now propagates reachable renewable money, arrows, bombs, magic, sticks and nuts through its native searches. Owning equipment alone no longer supplies refills in an isolated area.
- MM shop stock supplies refills only when its location, schedule, payment and bottle requirements are satisfied. Generation preserves unchanged shop stock across inventory resets and never assumes an unplaced shuffled shelf contains its vanilla refill.
- Unchanged MM shop shelves ignore unused randomly generated price metadata. Shuffled shelves keep their actual seed prices and wallet requirements.
- MM bombchus follow the supported bomb-bag license: a bomb bag and a reachable bombchu refill source suffice. Already holding bombchus is not an additional progression requirement.
- OoT tracker availability no longer gains routes merely from a temporary ammo or magic balance. Generation and tracker searches use the same renewable-source policy; native infinite upgrades remain valid exceptions.
- Native OoT infinite-upgrade ownership now reads the actual saved flags.
- MM's paid minigames require reachable money without checking the current balance. Games that supply ammunition retain their equipment requirements but do not demand outside ammunition.
- Honey & Darling's day-three reward correctly permits Deku bubbles with the Deku Mask and magic capacity. Its all-days reward still requires the Bow and all three days.

Wallet capacity and reachable money determine logical payment. Having 19 instead of 20 rupees does not make an otherwise logical 20-rupee check unavailable. Waiting and valid future visits still count; the current hour is not a progression restriction.

Menus and save formats are unchanged. See [the source mapping and engine boundaries](RESOURCE-PARITY-0.2.13.md) and [validation](VALIDATION-0.2.13.md).
