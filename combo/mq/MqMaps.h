#pragma once
// OoTMM v32.3's Master Quest map marks (src/oot/mq.c:439-517) as pure C++17 over the decoded bins (ComboMqData.h).
// OoTMM writes over the tables of overlays it has just loaded: the whole pause-map table of ovl_kaleido_scope with
// mq_maps.bin (comboMqKaleidoHook, mq.c:452-477) and the current dungeon's minimap marks in ovl_map_mark_data with
// its block of mq_scenes.bin (LoadMapMarkWrapper, mq.c:479-515). SoH compiles those tables in and never reloads them,
// so z_map_mark_data.c and z_lmap_mark_data.c keep separate tables that the glue
// (soh/soh/Enhancements/randomizer/ComboMqMaps.cpp) fills once with these templates. They take SoH's
// MapMarkIconData, PauseMapMarkData and Vtx in the glue and the same structs in the host test
// (ComboAndroid/tests/mq_map_marks_tests.py).
#include "ComboMqData.h"

#include <cstddef>
#include <type_traits>
#include <vector>

namespace ComboMq {

// MapMarkData is three MapMarkIconData records (z64map_mark.h), as MinimapRoomPOD.
constexpr size_t kMinimapMarksPerRoom = 3;

// LoadMapMarkWrapper's copy (mq.c:497-514) for one dungeon: the MQ block's whole entries over the first entries of
// `table`, whose `rooms` entries already hold the vanilla marks, as OoTMM's freshly loaded overlay does. Where the
// block is shorter (Jabu-Jabu: 16 MQ entries, 17 vanilla), the rest keep their vanilla marks, as in OoTMM. OoTMM also
// copies the block's zero padding to 16 bytes (the entry's minimapSize), 4-15 bytes past the dungeon's entries into
// the next dungeon's (for Ice Cavern, over the table's first two pointers). Only the current dungeon's entry is drawn
// and the overlay is reloaded with every scene, so that never shows; here nothing past the entries is written.
// Returns false and writes nothing for a dungeon without a minimap block (Gerudo Training Ground, Ganon's Castle) or
// with more entries than `rooms`.
template <class IconT>
bool OverlayMinimapMarks(const Data& data, int dungeon, IconT (*table)[kMinimapMarksPerRoom], size_t rooms) {
    static_assert(std::extent<decltype(IconT::points)>::value == kMinimapPointsMax, "MapMarkIconData has 12 points");
    if (dungeon < 0 || dungeon >= kMinimapDungeonCount || table == nullptr)
        return false;
    const std::vector<MinimapRoomPOD>& minimap = data.dungeons[dungeon].scene.minimap;
    if (minimap.empty() || minimap.size() > rooms)
        return false;
    for (size_t room = 0; room < minimap.size(); room++) {
        for (size_t record = 0; record < kMinimapMarksPerRoom; record++) {
            const MinimapIconsPOD& in = minimap[room].icons[record];
            IconT& out = table[room][record];
            out.markType = in.markType;
            out.count = in.count;
            for (size_t point = 0; point < kMinimapPointsMax; point++) {
                out.points[point].chestFlag = in.points[point].chestFlag;
                out.points[point].x = in.points[point].x;
                out.points[point].y = in.points[point].y;
            }
        }
    }
    return true;
}

// comboMqKaleidoHook (mq.c:452-477): mq_maps.bin as the whole pause-map table, 34 floors of 3 records, with every
// record whose MQ-ROM vertex pointer is set pointed at `vtx`, the overlay's own chest vertices (OoTMM's 0x8082ea00;
// the boss vertices are identical). Returns false and writes nothing unless `floors` is the bin's 34 and `vtx` is set.
template <class MarkT, class VtxT>
bool FillPauseMapMarks(const Data& data, MarkT (*table)[kPauseMarksPerFloor], size_t floors, const VtxT* vtx) {
    static_assert(std::extent<decltype(MarkT::points)>::value == kPausePointsMax, "PauseMapMarkData has 12 points");
    if (table == nullptr || vtx == nullptr || floors != kPauseFloorCount)
        return false;
    for (size_t floor = 0; floor < kPauseFloorCount; floor++) {
        for (size_t record = 0; record < kPauseMarksPerFloor; record++) {
            const PauseMarkPOD& in = data.pauseFloors[floor][record];
            MarkT& out = table[floor][record];
            out.markType = in.markType;
            out.unk_04 = in.unk04;
            out.vtx = in.hasVtx ? vtx : nullptr;
            out.vtxCount = in.vtxCount;
            out.count = in.count;
            for (size_t point = 0; point < kPausePointsMax; point++) {
                out.points[point].chestFlag = in.points[point].chestFlag;
                out.points[point].x = in.points[point].x;
                out.points[point].y = in.points[point].y;
            }
        }
    }
    return true;
}

} // namespace ComboMq
