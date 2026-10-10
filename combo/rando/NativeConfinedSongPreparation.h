#pragma once
#include "rando/SongTravel.h"
namespace ComboRando {
// Confined placement is native-only pool preparation, before combined oracle fill.
// A mixed-song exit cannot be traversed here: the other world's oracle is absent.
// Prepare restricted placements with the native graph, then restore the exact seed
// map before combined fill and its original-rule every-check certification.
class NativeConfinedSongPreparation {
    SongTravelMap saved;

  public:
    NativeConfinedSongPreparation() : saved(ActiveSongTravel()) {
        ActiveSongTravel().logicEnabled = false;
        ActiveSongTravel().logicForeignSongs = false;
    }
    ~NativeConfinedSongPreparation() {
        ActiveSongTravel() = saved;
    }
    NativeConfinedSongPreparation(const NativeConfinedSongPreparation&) = delete;
    NativeConfinedSongPreparation& operator=(const NativeConfinedSongPreparation&) = delete;
};
} // namespace ComboRando
