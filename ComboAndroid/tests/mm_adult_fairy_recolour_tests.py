"""Adult Link's Fairy Ocarina in MM, recoloured hand (owner ruling 2026-10-12, revised; tooling/claude-cleanup/t10).

Synthetic lists and textures only (no ROM-derived data). Checks:
- combo/menu/ComboFairyOcarinaRecolour.h, compiled on its own: the texture swap changes only the path hash of the one
  G_SETTIMG_OTR_HASH that sets the Ocarina of Time texture; it walks 128-bit commands as two slots (a hash that looks
  like a command is not read as one); it refuses a list that sets that texture twice or never, has no end, has commands
  after its end or ends inside a 128-bit command. The commands it treats as 128-bit are the ones libultraship's
  DisplayListFactory.cpp expands, with gbi.h's opcodes. Textures fit only with the same format, size, scale, flags and
  byte size.
- the runtime in combo/menu/ComboFairyOcarinaHand.h (Combo_FairyRecolouredHandPath and its state), with stand-in
  engine types: the copies are built once per OoT resource manager from OoT's adult lists, cached under the routed
  paths' names (reinserted on every call), drawn through even (2-byte aligned) routed paths, and dropped at teardown;
  OoT's cached lists are never changed; a custom, dirty, missing or mistyped list, a list without exactly one Ocarina
  of Time texture, or a Fairy texture that is missing, has no image or is laid out differently gives null (the caller
  then draws the composite), is logged once, and is not retried for the same manager.
The user's archives are checked by mm_adult_fairy_recolour_archive_tests.py (local only).
"""
from pathlib import Path
import argparse
import re
import subprocess
import sys

import hostcxx

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
a = p.parse_args()
root, work = a.source.resolve(), a.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
failures = []


def check(condition, message):
    if not condition:
        failures.append(message)


def block(source, start):
    """Text from start through the brace matching the first opening brace at or after it."""
    i = source.index('{', start)
    depth = 0
    while True:
        depth += (source[i] == '{') - (source[i] == '}')
        i += 1
        if depth == 0:
            return source[start:i]


# ---- The 128-bit commands: DisplayListFactory.cpp's expanded set, with gbi.h's values, is the header's set.
gbi = (root / 'libultraship/include/libultraship/libultra/gbi.h').read_text(encoding='utf-8')
factory = (root / 'libultraship/src/fast/resource/factory/DisplayListFactory.cpp').read_text(encoding='utf-8')
expanded = re.search(r'bool isExpanded = (.*?);', factory, re.S)
check(expanded is not None, 'DisplayListFactory.cpp no longer lists its expanded commands')
names = re.findall(r'opcode == (G_\w+)', expanded.group(1)) if expanded else []
values = {}
for name in names:
    m = re.search(r'#define ' + name + r' (0x[0-9a-fA-F]+)\b', gbi)
    check(m is not None, f'{name} is not defined in gbi.h')
    values[name] = int(m.group(1), 16) if m else -1
header = (root / 'combo/menu/ComboFairyOcarinaRecolour.h').read_text(encoding='utf-8')
wide = re.search(r'constexpr bool Wide\(uint8_t op\) \{\s*return (.*?);', header, re.S)
check(wide is not None, 'ComboFairyOcarinaRecolour.h has no Wide()')
consts = {m.group(1): int(m.group(2), 16) for m in re.finditer(r'(kOp\w+) = (0x[0-9A-Fa-f]+)', header)}
header_wide = {consts.get(n, -2) for n in re.findall(r'op == (kOp\w+)', wide.group(1))} if wide else set()
check(len(names) == 7 and header_wide == set(values.values()),
      f'the header treats {sorted(header_wide)} as 128-bit, DisplayListFactory.cpp {sorted(values.values())}')

PURE = r'''
#include "ComboFairyOcarinaRecolour.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace ComboFairyOcarinaRecolour;
struct Gfx { struct { uintptr_t w0, w1; } words; };
static Gfx G(uintptr_t w0, uintptr_t w1) { return Gfx{ { w0, w1 } }; }
static void Wide2(std::vector<Gfx>& l, uint8_t op, uintptr_t w0, uintptr_t w1, uint64_t hash) {
    l.push_back(G((uintptr_t(op) << 24) | w0, w1));
    l.push_back(G(uintptr_t(hash >> 32), uintptr_t(hash & 0xFFFFFFFFu)));
}
static const uint64_t kFrom = 0x9292A4F4D123CF96ull, kTo = 0x710708B58B22E79Eull, kOther = 0x1111222233334444ull;
static bool Same(const Gfx& a, const Gfx& b) { return a.words.w0 == b.words.w0 && a.words.w1 == b.words.w1; }
// An adult-hand-like list: the port's marker, a hand material and mesh, the instrument material with `from` set
// `uses` times, its mesh, and the end.
static std::vector<Gfx> List(unsigned uses, uint64_t from = kFrom) {
    std::vector<Gfx> l;
    Wide2(l, kOpMarker, 0, 0xBEEFBEEF, 0x5555);
    l.push_back(G(0xE7000000, 0));                // pipe sync
    Wide2(l, kOpSetTImgHash, 0x500000, 0, kOther); // hand texture
    l.push_back(G(0xF5500000, 0x07000000));
    Wide2(l, kOpVtxHash, (8 << 12) | (8 << 1), 0, from); // a vertex array whose hash equals `from`: not a texture
    l.push_back(G(0x06000204, 0x00060802));
    for (unsigned i = 0; i < uses; ++i) {
        Wide2(l, kOpSetTImgHash, 0x100000, 0x80001234 + i, from); // w1: the address an earlier draw left
        l.push_back(G(0xF5100000, 0x07090250));
        l.push_back(G(0xF3000000, 0x071FF100));
        l.push_back(G(0xFC127E03, 0xFFFFFDF8));
    }
    Wide2(l, kOpVtxHash, (23 << 12) | (23 << 1), 0x80, 0x7777);
    l.push_back(G(0x06000204, 0x00060802));
    Wide2(l, kOpDlHash, 0, 0, from); // a list call with the same hash: not a texture
    l.push_back(G(0xDF000000, 0));
    return l;
}

static void Swap() {
    const std::vector<Gfx> source = List(1);
    const std::vector<Gfx> before = source;
    std::vector<Gfx> result = { G(1, 2) };
    assert(SwapTexture(source, result, kFrom, kTo));
    assert(result.size() == source.size());
    size_t changed = 0, at = 0;
    for (size_t i = 0; i < source.size(); ++i)
        if (!Same(source[i], result[i])) {
            ++changed;
            at = i;
        }
    // Only the hash slot of the one texture command: its first slot (opcode, format, the left address) is kept.
    assert(changed == 1 && (source[at - 1].words.w0 >> 24) == kOpSetTImgHash && source[at - 1].words.w0 == 0x20100000);
    assert(Same(result[at - 1], source[at - 1]) && result[at].words.w0 == 0x710708B5u &&
           result[at].words.w1 == 0x8B22E79Eu);
    for (size_t i = 0; i < source.size(); ++i)
        assert(Same(source[i], before[i]));
    std::puts("Swap: only the Ocarina of Time texture's hash changes; vertex and list hashes equal to it are kept");

    // A second slot whose high word looks like G_SETTIMG_OTR_HASH is not read as a command: here a misread would find
    // a second match and refuse.
    std::vector<Gfx> tricky;
    Wide2(tricky, kOpMarker, 0, 0, (uint64_t(0x20100000) << 32) | 0);
    tricky.push_back(G(uintptr_t(kFrom >> 32), uintptr_t(kFrom & 0xFFFFFFFFu))); // a 64-bit command after it
    Wide2(tricky, kOpSetTImgHash, 0x100000, 0, kFrom);
    tricky.push_back(G(0xDF000000, 0));
    assert(SwapTexture(tricky, result, kFrom, kTo) && result[4].words.w0 == 0x710708B5u && Same(result[1], tricky[1]) &&
           Same(result[2], tricky[2]));
    // Every 128-bit command is skipped as two slots.
    for (uint8_t op :
         { kOpSetTImgHash, kOpDlHash, kOpVtxHash, kOpMarker, kOpBranchZHash, kOpMtxHash, kOpMoveMemHash }) {
        std::vector<Gfx> l;
        Wide2(l, op, 0, 0, (uint64_t(0x20000000) << 32) | 0x1234);
        l.push_back(G(uintptr_t(kFrom >> 32), uintptr_t(kFrom & 0xFFFFFFFFu)));
        Wide2(l, kOpSetTImgHash, 0x100000, 0, kFrom);
        l.push_back(G(0xDF000000, 0));
        assert(Wide(op) && SwapTexture(l, result, kFrom, kTo) && result[4].words.w1 == 0x8B22E79Eu);
    }
    assert(!Wide(0x01) && !Wide(0x06) && !Wide(0xDE) && !Wide(0xDF) && !Wide(0x27));
    std::puts("Walk: 128-bit commands take two slots; a hash that looks like a command is not read as one");

    // Refused, with nothing returned: twice, never, no end, commands after the end, the end inside a 128-bit command.
    result = { G(1, 2) };
    assert(!SwapTexture(List(2), result, kFrom, kTo) && result.empty());
    result = { G(1, 2) };
    assert(!SwapTexture(List(0), result, kFrom, kTo) && result.empty());
    assert(!SwapTexture(List(1, kOther), result, kFrom, kTo) && result.empty());
    std::vector<Gfx> open = List(1);
    open.pop_back();
    assert(!SwapTexture(open, result, kFrom, kTo) && result.empty());
    std::vector<Gfx> after = List(1);
    after.push_back(G(0xE7000000, 0));
    assert(!SwapTexture(after, result, kFrom, kTo) && result.empty());
    std::vector<Gfx> cut = List(1);
    cut.pop_back();
    cut.push_back(G(0x20100000, 0));
    assert(!SwapTexture(cut, result, kFrom, kTo) && result.empty());
    assert(!SwapTexture(std::vector<Gfx>(), result, kFrom, kTo) && result.empty());
    std::puts("Refused: set twice, never, no end, commands after the end, cut inside a 128-bit command, empty");
}

enum class Format { RGBA16, RGBA32 };
struct Texture {
    Format Type = Format::RGBA16;
    unsigned short Width = 32, Height = 16;
    unsigned Flags = 0;
    float HByteScale = 1, VPixelScale = 1;
    unsigned ImageDataSize = 1024;
};

static void Layout() {
    const Texture design;
    Texture fairy;
    assert(SameLayout(design, fairy));
    fairy.Type = Format::RGBA32;
    assert(!SameLayout(design, fairy));
    fairy = Texture();
    fairy.Width = 64;
    assert(!SameLayout(design, fairy));
    fairy = Texture();
    fairy.Height = 32;
    assert(!SameLayout(design, fairy));
    fairy = Texture();
    fairy.Flags = 1;
    assert(!SameLayout(design, fairy));
    fairy = Texture();
    fairy.HByteScale = 2;
    assert(!SameLayout(design, fairy));
    fairy = Texture();
    fairy.VPixelScale = 0.5f;
    assert(!SameLayout(design, fairy));
    fairy = Texture();
    fairy.ImageDataSize = 2048;
    assert(!SameLayout(design, fairy));
    Texture empty;
    empty.ImageDataSize = 0;
    assert(!SameLayout(empty, empty));
    std::puts("Layout: textures fit only with the same format, size, flags, scales and byte size");
}

int main() {
    Swap();
    Layout();
    std::puts("PASS pure");
}
'''

# ---- The runtime, with stand-in engine types.
hand = (root / 'combo/menu/ComboFairyOcarinaHand.h').read_text(encoding='utf-8')
statics_at = hand.index('static std::shared_ptr<Ship::IResource> sFairyInstrumentSource;')
listen_at = hand.index('static void Combo_ListenFairyInstrumentTeardown() {')
statics = hand[statics_at:listen_at + len(block(hand, listen_at))]
texture_line = re.search(r'static const char kFairyPlacementInstrumentTexture\[\] = "[^"]*";', hand)
check(texture_line is not None, 'the Ocarina of Time texture name is not in ComboFairyOcarinaHand.h')
recolour_at = hand.index('static const char* const kFairyRecolouredLists[2] = {')
state_at = hand.index('extern "C" int Combo_MM_FairyRecolourState(void) {')
recolour = hand[recolour_at:state_at + len(block(hand, state_at))]
check(len(re.findall(r'alignas\(2\) static const char kFairyRecoloured\w+\[\] = "__OTR__@oot:', recolour)) == 2,
      'the routed paths of the recoloured hands are not declared even')
check('Combo_FairyRecolouredHandPath' not in hand[:recolour_at], 'the recoloured hand is used before it is built')

RUNTIME = r'''
#include "ComboFairyOcarinaRecolour.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <vector>
typedef int s32;
struct Mtx { int unused; };
struct Gfx { struct { uintptr_t w0, w1; } words; };
namespace Ship {
struct ResourceInitData { std::string Path; uint32_t Type = 0; bool IsCustom = false; };
class IResource {
  public:
    explicit IResource(std::shared_ptr<ResourceInitData> init) : mInit(std::move(init)) {}
    virtual ~IResource() = default;
    std::shared_ptr<ResourceInitData> GetInitData() { return mInit; }
    bool IsDirty() { return dirty; }
    bool dirty = false;
  private:
    std::shared_ptr<ResourceInitData> mInit;
};
class ResourceManager {
  public:
    std::map<std::string, std::shared_ptr<IResource>> files, cache;
    int loads = 0, cached = 0;
    std::shared_ptr<IResource> LoadResourceProcess(const std::string& path) {
        ++loads;
        auto it = files.find(path);
        return it == files.end() ? nullptr : it->second;
    }
    void CacheExternalResource(const std::string& path, std::shared_ptr<IResource> resource) {
        ++cached;
        cache[path] = resource;
    }
};
struct CrossRMRegistry {
    static inline std::shared_ptr<ResourceManager> oot;
    static inline std::vector<void (*)()> listeners;
    static std::shared_ptr<ResourceManager> Get(const std::string& game) { return game == "oot" ? oot : nullptr; }
    static void RegisterTeardownListener(void (*listener)()) { listeners.push_back(listener); }
};
} // namespace Ship
namespace Fast {
enum class ResourceType : uint32_t { DisplayList = 0x4F444C54, Texture = 0x4F544558 };
enum class TextureType { Error, RGBA32bpp, RGBA16bpp };
class DisplayList : public Ship::IResource {
  public:
    using IResource::IResource;
    int UCode = 0;
    std::vector<Gfx> Instructions;
};
class Texture : public Ship::IResource {
  public:
    using IResource::IResource;
    TextureType Type = TextureType::RGBA16bpp;
    uint16_t Width = 32, Height = 16;
    uint32_t Flags = 0;
    float HByteScale = 1, VPixelScale = 1;
    uint32_t ImageDataSize = 1024;
    uint8_t* ImageData = nullptr;
};
} // namespace Fast
// libultraship's path hash (ship/utils/StrHash64.h: CRC-64/ECMA-182, all ones in, no final xor).
static uint64_t CRC64(const char* text) {
    uint64_t crc = ~0ull;
    for (; *text; ++text) {
        crc ^= uint64_t(uint8_t(*text)) << 56;
        for (int k = 0; k < 8; ++k)
            crc = (crc >> 63) ? (crc << 1) ^ 0x42F0E1EBA9EA3693ull : crc << 1;
    }
    return crc;
}
static int sWarnings = 0;
#define SPDLOG_WARN(...) (++sWarnings)

@STATICS@
@TEXTURE@
@RECOLOUR@

static uint8_t sImage[1024];
static const char kNear[] = "objects/object_link_boy/gLinkAdultRightHandHoldingOotNearDL";
static const char kFar[] = "objects/object_link_boy/gLinkAdultRightHandHoldingOotFarDL";
static const char kDesign[] = "objects/gameplay_keep/gOcarinaofTimeDesignTex";
static const char kFairy[] = "objects/object_link_child/gLinkChildFairyOcarinaTex";
static const char kNearName[] = "combo/runtime/adult_fairy_ocarina_hand_near";
static const char kFarName[] = "combo/runtime/adult_fairy_ocarina_hand_far";
static Gfx G(uintptr_t w0, uintptr_t w1) { return Gfx{ { w0, w1 } }; }
static void Hash(std::vector<Gfx>& l, uintptr_t w0, uint64_t hash) {
    l.push_back(G(w0, 0));
    l.push_back(G(uintptr_t(hash >> 32), uintptr_t(hash & 0xFFFFFFFFu)));
}
static std::vector<Gfx> AdultList(unsigned uses, unsigned seed) {
    std::vector<Gfx> l;
    Hash(l, 0x33000000, 0x42);
    Hash(l, 0x20500000, CRC64("objects/object_link_boy/gLinkAdultHandTex"));
    Hash(l, 0x32008010, 0x1000 + seed);
    l.push_back(G(0x06000204, seed));
    for (unsigned i = 0; i < uses; ++i) {
        Hash(l, 0x20100000, CRC64(kDesign));
        l.push_back(G(0xF5100000, 0x07090250));
    }
    l.push_back(G(0xDF000000, 0));
    return l;
}
static std::shared_ptr<Fast::DisplayList> MakeList(const char* path, unsigned uses, unsigned seed) {
    auto init = std::make_shared<Ship::ResourceInitData>();
    init->Path = path;
    init->Type = uint32_t(Fast::ResourceType::DisplayList);
    auto list = std::make_shared<Fast::DisplayList>(init);
    list->UCode = 2;
    list->Instructions = AdultList(uses, seed);
    return list;
}
static std::shared_ptr<Fast::Texture> MakeTexture(const char* path) {
    auto init = std::make_shared<Ship::ResourceInitData>();
    init->Path = path;
    init->Type = uint32_t(Fast::ResourceType::Texture);
    auto texture = std::make_shared<Fast::Texture>(init);
    texture->ImageData = sImage;
    return texture;
}
struct Oot {
    std::shared_ptr<Ship::ResourceManager> rm = std::make_shared<Ship::ResourceManager>();
    std::shared_ptr<Fast::DisplayList> lists[2] = { MakeList(kNear, 1, 1), MakeList(kFar, 1, 2) };
    std::vector<Gfx> before[2] = { lists[0]->Instructions, lists[1]->Instructions };
    std::shared_ptr<Fast::Texture> design = MakeTexture(kDesign), fairy = MakeTexture(kFairy);
    Oot() {
        rm->files[kNear] = lists[0];
        rm->files[kFar] = lists[1];
        rm->files[kDesign] = design;
        rm->files[kFairy] = fairy;
    }
    bool Untouched() const {
        for (int i = 0; i < 2; ++i) {
            if (lists[i]->Instructions.size() != before[i].size())
                return false;
            for (size_t k = 0; k < before[i].size(); ++k)
                if (lists[i]->Instructions[k].words.w0 != before[i][k].words.w0 ||
                    lists[i]->Instructions[k].words.w1 != before[i][k].words.w1)
                    return false;
        }
        return true;
    }
};
static void Teardown() {
    for (auto listener : Ship::CrossRMRegistry::listeners)
        listener();
}

static void Built() {
    Oot oot;
    Ship::CrossRMRegistry::oot = oot.rm;
    assert(Combo_MM_FairyRecolourState() == 0);
    const char* nearPath = Combo_FairyRecolouredHandPath(0);
    assert(nearPath && std::strcmp(nearPath, "__OTR__@oot:combo/runtime/adult_fairy_ocarina_hand_near") == 0);
    assert(!(reinterpret_cast<uintptr_t>(nearPath) & 1));
    assert(Combo_MM_FairyRecolourState() == 1 && sWarnings == 0);
    const int loads = oot.rm->loads;
    assert(loads == 4); // two textures, two lists
    const char* farPath = Combo_FairyRecolouredHandPath(1);
    assert(farPath && std::strcmp(farPath, "__OTR__@oot:combo/runtime/adult_fairy_ocarina_hand_far") == 0);
    assert(!(reinterpret_cast<uintptr_t>(farPath) & 1) && oot.rm->loads == loads);
    // Each copy: OoT's list with only the texture hash swapped, OoT's ucode and init data; OoT's lists unchanged.
    for (int lod = 0; lod < 2; ++lod) {
        auto copy = std::static_pointer_cast<Fast::DisplayList>(oot.rm->cache.at(lod ? kFarName : kNearName));
        assert(copy != oot.lists[lod] && copy->UCode == 2 && copy->GetInitData() == oot.lists[lod]->GetInitData());
        std::vector<Gfx> expected = oot.before[lod];
        for (size_t k = 0; k + 1 < expected.size(); ++k)
            if (expected[k].words.w0 == 0x20100000) {
                expected[k + 1].words.w0 = uintptr_t(CRC64(kFairy) >> 32);
                expected[k + 1].words.w1 = uintptr_t(CRC64(kFairy) & 0xFFFFFFFFu);
            }
        assert(copy->Instructions.size() == expected.size());
        for (size_t k = 0; k < expected.size(); ++k)
            assert(copy->Instructions[k].words.w0 == expected[k].words.w0 &&
                   copy->Instructions[k].words.w1 == expected[k].words.w1);
    }
    assert(oot.Untouched());
    // Reinserted on every call, after an eviction too; never rebuilt for the same manager.
    auto nearCopy = oot.rm->cache.at(kNearName);
    oot.rm->cache.erase(kNearName);
    const int cached = oot.rm->cached;
    assert(Combo_FairyRecolouredHandPath(0) == nearPath && oot.rm->cache.at(kNearName) == nearCopy);
    assert(oot.rm->cached == cached + 1 && oot.rm->loads == loads);
    assert(!Combo_FairyRecolouredHandPath(-1) && !Combo_FairyRecolouredHandPath(2));
    assert(Ship::CrossRMRegistry::listeners.size() == 1);
    // Teardown drops the copies and OoT's lists; the next call builds them again.
    std::weak_ptr<Ship::IResource> weakCopy = nearCopy;
    nearCopy.reset();
    oot.rm->cache.clear();
    Teardown();
    assert(weakCopy.expired() && Combo_MM_FairyRecolourState() == 0);
    assert(Combo_FairyRecolouredHandPath(0) == nearPath && oot.rm->loads == 2 * loads &&
           Combo_MM_FairyRecolourState() == 1);
    assert(Ship::CrossRMRegistry::listeners.size() == 1);
    Teardown();
    Ship::CrossRMRegistry::oot.reset();
    assert(!Combo_FairyRecolouredHandPath(0) && Combo_MM_FairyRecolourState() == 0);
    std::puts("Built once per OoT manager: even routed paths, copies reinserted per call, OoT's lists unchanged, "
              "dropped at teardown");
}

template <typename Break> static void Refused(const char* what, Break breakIt) {
    Oot oot;
    breakIt(oot);
    Ship::CrossRMRegistry::oot = oot.rm;
    const int warnings = sWarnings;
    assert(!Combo_FairyRecolouredHandPath(0));
    assert(Combo_MM_FairyRecolourState() == -1 && sWarnings == warnings + 1);
    const int loads = oot.rm->loads;
    assert(!Combo_FairyRecolouredHandPath(1) && !Combo_FairyRecolouredHandPath(0));
    assert(oot.rm->loads == loads && sWarnings == warnings + 1); // not retried, logged once
    assert(oot.rm->cache.empty() && oot.Untouched());
    // Another manager is tried afresh.
    Oot other;
    Ship::CrossRMRegistry::oot = other.rm;
    assert(Combo_MM_FairyRecolourState() == 0 && Combo_FairyRecolouredHandPath(0) &&
           Combo_MM_FairyRecolourState() == 1);
    Teardown();
    Ship::CrossRMRegistry::oot.reset();
    std::printf("Refused, once per manager: %s\n", what);
}

int main() {
    Built();
    Refused("a custom (mod) near list", [](Oot& o) { o.lists[0]->GetInitData()->IsCustom = true; });
    Refused("a dirty far list", [](Oot& o) { o.lists[1]->dirty = true; });
    Refused("a missing far list", [](Oot& o) { o.rm->files.erase(kFar); });
    Refused("a near list that is not a display list", [](Oot& o) { o.rm->files[kNear] = MakeTexture(kNear); });
    Refused("a far list that sets the Ocarina of Time texture twice", [](Oot& o) {
        o.lists[1]->Instructions = AdultList(2, 2);
        o.before[1] = o.lists[1]->Instructions;
    });
    Refused("a near list that never sets it", [](Oot& o) {
        o.lists[0]->Instructions = AdultList(0, 1);
        o.before[0] = o.lists[0]->Instructions;
    });
    Refused("a missing Fairy Ocarina texture", [](Oot& o) { o.rm->files.erase(kFairy); });
    Refused("a missing Ocarina of Time texture", [](Oot& o) { o.rm->files.erase(kDesign); });
    Refused("a Fairy Ocarina texture without an image", [](Oot& o) { o.fairy->ImageData = nullptr; });
    Refused("a dirty Fairy Ocarina texture", [](Oot& o) { o.fairy->dirty = true; });
    Refused("a Fairy Ocarina texture of another size", [](Oot& o) {
        o.fairy->Width = 64;
        o.fairy->ImageDataSize = 2048;
    });
    Refused("a Fairy Ocarina texture of another format", [](Oot& o) { o.fairy->Type = Fast::TextureType::RGBA32bpp; });
    Refused("a Fairy Ocarina texture that is a display list",
            [](Oot& o) { o.rm->files[kFairy] = MakeList(kFairy, 1, 3); });
    std::puts("PASS runtime");
}
'''

if failures:
    print('\n'.join('FAIL: ' + f for f in failures))
    sys.exit(1)
for name, code in (('fairy_recolour_pure', PURE),
                   ('fairy_recolour_runtime', RUNTIME.replace('@STATICS@', statics)
                    .replace('@TEXTURE@', texture_line.group(0)).replace('@RECOLOUR@', recolour))):
    source = work / f'{name}.cpp'
    source.write_text(code, encoding='utf-8')
    exe = hostcxx.build(a.compiler, source, std='c++17', includes=[root / 'combo/menu'], warnings=name.endswith('pure'),
                        werror=name.endswith('pure'))
    subprocess.run([str(exe)], cwd=work, check=True, timeout=hostcxx.RUN_TIMEOUT)
print('PASS')
