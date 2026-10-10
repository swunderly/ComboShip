#pragma once
// Exact pinned mm/actors/Player.c palette; MM uses 0 for default rather than OoT's Kokiri1.
static const Color_RGB8 sComboAdultTunicColors[4]={{30,105,27},{30,105,27},{100,20,0},{0,60,100}};
static const Color_RGB8* Combo_MM_GetSavedTunicColor(s32 tunic){
    if(tunic<0)tunic=0;
    if(tunic>3)tunic=3;
    return &sComboAdultTunicColors[tunic];
}
