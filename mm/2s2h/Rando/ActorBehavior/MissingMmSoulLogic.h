#pragma once
namespace Rando {
// Pinned GBT_BABA_CHEST_SPAWN and Compass route, restricted to supported native equipment.
inline bool MissingSoulBabaChest(bool enabled,bool legacy,bool soul,bool zora,bool arrows,bool blast,bool hook) {
 return enabled ? soul&&(zora||arrows||(blast&&hook)) : legacy;
}
inline bool MissingSoulBabaCompass(bool enabled,bool soul,bool hook,bool ice) {
 return !enabled || soul || hook || ice;
}
}
