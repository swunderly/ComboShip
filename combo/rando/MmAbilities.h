#pragma once
namespace ComboRando {
// Native FD wades at least as deep as Goron (84 vs 56 units). This grants the
// existing shallow-water routes only, never underwater swimming or Deku flight.
inline constexpr bool CanTraverseWaistWater(bool swim, bool deku, bool zora, bool goron, bool logicalDeity) {
    return swim || deku || zora || goron || logicalDeity;
}
// These predicates describe the native port hooks, not ROM-hack movement tricks.
inline bool CanUseDeityOutsideBoss(bool ownsMask, bool logicalAnywhere) {
    return ownsMask && logicalAnywhere;
}
inline bool CanSpikeRoll(bool ownsGoron, bool ownsMagic, bool magicFreeRolling) {
    return ownsGoron && (ownsMagic || magicFreeRolling);
}
inline bool CanUsePowderKeg(bool ownsGoron, bool ownsKeg, bool ownsBlastMask, bool blastMaskKeg) {
    return (ownsGoron && ownsKeg) || (ownsBlastMask && blastMaskKeg);
}
// OoTMM's Moon Boss rules: a ranged first phase AND a repeatable finishing weapon.
// Native Boss07 damage tables accept all three paths. Boss souls still gate the actor.
inline bool CanDefeatMajora(bool soulEnabled, bool ownsSoul, bool ownsBow, bool ownsZora, bool ownsDeity,
                            bool ownsMagic, bool ownsHumanSword, bool ownsFairySword) {
    const bool deityBeam = ownsDeity && ownsMagic;
    const bool phaseOne = ownsBow || ownsZora || deityBeam;
    const bool finish = ownsHumanSword || ownsFairySword || ownsZora || deityBeam;
    return (!soulEnabled || ownsSoul) && phaseOne && finish;
}
} // namespace ComboRando
