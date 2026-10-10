#pragma once
#include <cstdint>
#include <vector>
namespace ComboRando {
// Pinned object_link_child 0x15BA8; verified native XML and local USA ROM list.
// OTR adds one expanded marker and expands resource-reference commands. Count raw commands,
// never interpret ROM byte offsets as native Instructions indices.
template<class Command> bool StripPinnedFairyOcarinaHand(const std::vector<Command>& source,
    std::vector<Command>& result, uint8_t marker, uint8_t texture, uint8_t vertex, uint8_t list) {
    static constexpr uint8_t raw[] = {0xe7, 0xe3, 0xd7, 0xfd, 0xf5, 0xe6, 0xf3, 0xe7, 0xf5, 0xf2, 0xfc, 0xe2, 0xd9, 0xd9, 0xfa, 0x1, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x1, 0x6, 0xe7, 0xe3, 0xfd, 0xf5, 0xe6, 0xf3, 0xe7, 0xf5, 0xf2, 0xfd, 0xe8, 0xf5, 0xe6, 0xf0, 0xe7, 0x1, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x1, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0xdf};
    result.clear();
    if (source.size()<2 || uint8_t(source[0].words.w0>>24)!=marker) return false;
    size_t at=2;
    for (size_t ordinal=0;ordinal<sizeof(raw);++ordinal) {
        if (at>=source.size()) { result.clear(); return false; }
        const uint8_t expected=raw[ordinal]==0xfd?texture:raw[ordinal]==1?vertex:raw[ordinal]==0xde?list:raw[ordinal];
        const uint8_t opcode=uint8_t(source[at].words.w0>>24);
        if (opcode!=expected) { result.clear(); return false; }
        const size_t width=(opcode==texture||opcode==vertex||opcode==list)?2:1;
        if(at+width>source.size()){result.clear();return false;}
        if(ordinal<34 || ordinal>=72) result.insert(result.end(),source.begin()+at,source.begin()+at+width);
        at+=width;
    }
    if(at!=source.size()){result.clear();return false;}
    return true;
}
}
