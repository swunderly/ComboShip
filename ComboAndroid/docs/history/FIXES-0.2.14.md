# Combo Ship 0.2.14

Logic corrections for the supported OoT/MM randomizer features, using the pinned OoTMM v32.3 source as the reference. Both seed generation and the check tracker use the corrected native searches. Existing seed placements are preserved.

- Imported 49 valid MM combat predicates with their upstream license and source hashes, retaining enemy-soul gates and actual native equipment restrictions.
- Corrected MM Moon trial combat, Song of Time return requirements, Ghost Hut health requirements, stick-based owl activation, Snowhead/Great Bay/Stone Tower routes, bottle usability and hot-water transport.
- Fixed OoT trick settings being lost during finalization; added supported Skeleton Key inventory effects and corrected startup inventory refresh.
- Corrected normal and Master Quest dungeon predicates, mixed-age routes, Mido skip equipment, underwater chest access, high beehives, child sword-only wonder items, Shadow scarecrow hearts and Death Mountain Crater scenery access.
- Corrected MM Keaton grass requirements, Termina Field drops, Zora Cape trees and Stone Tower scenery routes.
- Retained future-time and renewable-resource planning: the tracker does not demand your current rupee or ammo balance when a valid refill is reachable.
- Kept gameplay-only enhancements separate from the rules used to generate a seed.

See VALIDATION-0.2.14.md and LOGIC-PARITY-0.2.14.md for the comparison coverage, native-engine differences and test limits. This release makes no changes to menu layout or cross-warp animation assets.
