#pragma once
// Pinned drawgi.yml OOT_BOOTS_IRON Opa0_Xlu1, HOVER Opa0, TUNIC Opa1023.
namespace ComboRando {
struct AdultGearGiRecipe {int count,xlu;const char* paths[4];};
inline const AdultGearGiRecipe* AdultGearRecipe(int kind){
 static const AdultGearGiRecipe recipes[]={
 {2,1,{"__OTR__@oot:objects/object_gi_boots_2/gGiIronBootsDL","__OTR__@oot:objects/object_gi_boots_2/gGiIronBootsRivetsDL"}},
 {1,-1,{"__OTR__@oot:objects/object_gi_hoverboots/gGiHoverBootsDL"}},
 {4,-1,{"__OTR__@oot:objects/object_gi_clothes/gGiGoronCollarColorDL","__OTR__@oot:objects/object_gi_clothes/gGiTunicCollarDL","__OTR__@oot:objects/object_gi_clothes/gGiGoronTunicColorDL","__OTR__@oot:objects/object_gi_clothes/gGiTunicDL"}},
 {4,-1,{"__OTR__@oot:objects/object_gi_clothes/gGiZoraCollarColorDL","__OTR__@oot:objects/object_gi_clothes/gGiTunicCollarDL","__OTR__@oot:objects/object_gi_clothes/gGiZoraTunicColorDL","__OTR__@oot:objects/object_gi_clothes/gGiTunicDL"}}
 };return kind>=0&&kind<4?&recipes[kind]:nullptr;
}
}
