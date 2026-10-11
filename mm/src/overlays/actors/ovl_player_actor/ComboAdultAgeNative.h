#pragma once
// Adult Link's age in MM's player overlay, as OoTMM v32.3 Actor_PatchPlayerFormProperties (generator/src/mm/
// actors.c:1033-1058): the human age properties become adult Link's (2s2h/ComboAdultMM.h BuildAdultProperties) and the
// human door animations (anim groups doorA and doorB) become pz_doorA_open and pz_doorB_open. Child gets MM's own
// back. Included after the player overlay's definitions; it needs nothing of the Adult Mask.
void Combo_MM_CopyAdultProperties(PlayerAgeProperties* out);
static PlayerAgeProperties sComboChildHumanProperties;
static PlayerAnimationHeader* sComboChildDoorA[PLAYER_ANIMTYPE_MAX];
static PlayerAnimationHeader* sComboChildDoorB[PLAYER_ANIMTYPE_MAX];
static s32 sComboChildAgeCaptured;
void Combo_MM_ApplyAdultOverlayAge(int adult) {
    s32 i;
    if (!sComboChildAgeCaptured) {
        sComboChildHumanProperties = sPlayerAgeProperties[PLAYER_FORM_HUMAN];
        memcpy(sComboChildDoorA, D_8085BE84[PLAYER_ANIMGROUP_doorA], sizeof(sComboChildDoorA));
        memcpy(sComboChildDoorB, D_8085BE84[PLAYER_ANIMGROUP_doorB], sizeof(sComboChildDoorB));
        sComboChildAgeCaptured = true;
    }
    if (adult) {
        Combo_MM_CopyAdultProperties(&sPlayerAgeProperties[PLAYER_FORM_HUMAN]);
        for (i = 0; i < PLAYER_ANIMTYPE_MAX; ++i) {
            D_8085BE84[PLAYER_ANIMGROUP_doorA][i] = (PlayerAnimationHeader*)&gPlayerAnim_pz_doorA_open;
            D_8085BE84[PLAYER_ANIMGROUP_doorB][i] = (PlayerAnimationHeader*)&gPlayerAnim_pz_doorB_open;
        }
    } else {
        sPlayerAgeProperties[PLAYER_FORM_HUMAN] = sComboChildHumanProperties;
        memcpy(D_8085BE84[PLAYER_ANIMGROUP_doorA], sComboChildDoorA, sizeof(sComboChildDoorA));
        memcpy(D_8085BE84[PLAYER_ANIMGROUP_doorB], sComboChildDoorB, sizeof(sComboChildDoorB));
    }
}
