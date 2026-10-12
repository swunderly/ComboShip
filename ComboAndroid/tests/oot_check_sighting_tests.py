"""Run OoT's production reward entry and tracker sighting code against controlled camera/collision inputs.

Context::GetFinalGIEntry (SeedContext.cpp) and the tracker's whole sighting block
(randomizer_check_tracker.cpp) are compiled unchanged with engine fixtures. The fixtures stand in
for the camera projection, the collision line test and the seed; this is not a GPU visibility test.
"""
import argparse
from pathlib import Path
import subprocess
import hostcxx

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--json-include', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
a = p.parse_args()
source, out = a.source.resolve(), a.build_dir.resolve()
out.mkdir(parents=True, exist_ok=True)
randomizer = source/'soh/soh/Enhancements/randomizer'


def body(text, signature):
    start = hostcxx.find(text, signature)
    end = text.index('{', start) + 1
    depth = 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return start, end


seed = (randomizer/'SeedContext.cpp').read_text(encoding='utf-8')
start, end = body(seed, 'GetItemEntry Context::GetFinalGIEntry(')
final_entry = seed[start:end]
tracker = (randomizer/'randomizer_check_tracker.cpp').read_text(encoding='utf-8')
# The whole COMBO_BUILD sighting block, minus the hint entry point (it formats shop prices).
start = tracker.index('extern int sectionId;')
block = tracker[start:tracker.rindex('#endif', start, tracker.index('static WidgetInfo backgroundColorWidget;'))]
hint_start, hint_end = body(block, 'void RecordHintedItem(')
block = block[:hint_start] + block[hint_end:]

prelude = r'''
#define COMBO_BUILD 1
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include "soh/Enhancements/randomizer/randomizerEnums.h"
#include "rando/TrackerKnowledge.h"
#include "rando/CrossForeign.h"
using s16 = int16_t; using u16 = uint16_t; using s32 = int32_t; using u32 = uint32_t; using f32 = float;
using GetItemID = int;
constexpr int GI_NONE = 0, GI_RUPEE_BLUE = 0x4C, GI_SHIELD_DEKU = 0x29, GI_HEART_PIECE = 0x7A;
constexpr int GID_RUPEE_BLUE = 0x6E, GID_SHIELD_DEKU = 0x1F, GID_HEART_PIECE = 0x13, GID_RUPEE_GOLD = 0x71;
constexpr int ACTOR_EN_ITEM00 = 0x15, ACTOR_EN_SI = 0xF1, ACTOR_EN_GIRLA = 0x10A, ACTOR_BG_YDAN_SP = 0x0F,
              ACTOR_BG_BOM_GUARD = 0x1A5, ACTOR_BG_SOLID = 0x99;
constexpr int TRANS_MODE_OFF = 0, TRANS_TRIGGER_OFF = 0, BGCHECK_SCENE = 50;
using CustomDrawFunc = void (*)(void*, void*);
struct GetItemEntry {
    u16 itemId = 0, modIndex = 0;
    s16 getItemId = 0, gi = 0;
    u16 gid = 0, drawItemId = 0, drawModIndex = 0;
    int getItemCategory = 0;
    CustomDrawFunc drawFunc = nullptr;
    int32_t comboForeignCheck = 0;
};
GetItemEntry Entry(int modIndex, int getItemId, int gid) {
    GetItemEntry e;
    e.modIndex = e.drawModIndex = u16(modIndex);
    e.getItemId = e.gi = s16(getItemId);
    e.gid = u16(gid);
    e.itemId = e.drawItemId = u16(getItemId);
    return e;
}

// ---- Engine fixtures: camera, model matrix and collision ----
struct Vec3f { float x = 0, y = 0, z = 0; };
struct MtxF { float xx = 1, yx = 0, zx = 0, xy = 0, yy = 1, zy = 0, xz = 0, yz = 0, zz = 1; } testModel;
struct Actor { s16 id = ACTOR_EN_ITEM00; };
struct DynaPolyActor { Actor actor; };
struct CollisionPoly {};
struct CollisionContext {} ;
struct PlayState {
    int transitionMode = TRANS_MODE_OFF, transitionTrigger = TRANS_TRIGGER_OFF;
    struct { uint8_t fillScreen = 0; uint8_t screenFillColor[4] = { 0, 0, 0, 0 }; } envCtx;
    struct { int state = 0; } pauseCtx;
    MtxF viewProjectionMtxF;
    struct { Vec3f eye{ 0, 0, 0 }; } view;
    CollisionContext colCtx;
} play;
PlayState* gPlayState = &play;
Vec3f drawnOrigin{ 0, 0, 100 }, clip{ 0, 0, 50 };
float clipW = 100, aspect = 4.0f / 3.0f;
MtxF* Matrix_GetCurrent() { return &testModel; }
void Matrix_MultVec3f(Vec3f* src, Vec3f* dest) {
    assert(src->x == 0 && src->y == 0 && src->z == 0); // the model origin
    *dest = drawnOrigin;
}
float OTRGetAspectRatio() { return aspect; }
void SkinMatrix_Vec3fMtxFMultXYZW(MtxF* mtx, Vec3f* pos, Vec3f* projected, float* w) {
    assert(mtx == &play.viewProjectionMtxF);
    assert(pos->x == drawnOrigin.x && pos->y == drawnOrigin.y && pos->z == drawnOrigin.z);
    *projected = clip;
    *w = clipW;
}
// Collision between the camera and the model: a flat polygon crossing the line at `at`,
// owned by the scene (bgId BGCHECK_SCENE) or by a moving actor (its bgId).
struct Occluder { Vec3f at; s32 bgId; DynaPolyActor* owner; };
std::vector<Occluder> occluders;
DynaPolyActor web{ { ACTOR_BG_YDAN_SP } }, web2{ { ACTOR_BG_YDAN_SP } }, guard{ { ACTOR_BG_BOM_GUARD } },
    door{ { ACTOR_BG_SOLID } }, shelf{ { ACTOR_EN_GIRLA } };
int raycasts = 0;
static float Along(const Vec3f& from, const Vec3f& to, const Vec3f& at) {
    const Vec3f d{ to.x - from.x, to.y - from.y, to.z - from.z };
    const float len2 = d.x * d.x + d.y * d.y + d.z * d.z;
    return len2 == 0 ? -1 : ((at.x - from.x) * d.x + (at.y - from.y) * d.y + (at.z - from.z) * d.z) / len2;
}
static bool LineFixture(Vec3f* from, Vec3f* to, Vec3f* hit, CollisionPoly** poly, s32* bgId, Actor* skip) {
    ++raycasts;
    static CollisionPoly hitPoly;
    const Occluder* nearest = nullptr;
    float best = 2;
    for (const auto& o : occluders) {
        const float t = Along(*from, *to, o.at);
        // A start point on a polygon's plane can still count as crossing it after float rounding
        // (z_bgcheck.c LineVsPoly), so a continued line must exclude the actor it just passed.
        if (t < -0.0001f || t >= 1 || (skip && o.owner && skip == &o.owner->actor) || t >= best)
            continue;
        best = t;
        nearest = &o;
    }
    *bgId = nearest ? nearest->bgId : BGCHECK_SCENE;
    *poly = nearest ? &hitPoly : nullptr;
    *hit = nearest ? nearest->at : *to;
    return nearest != nullptr;
}
s32 BgCheck_AnyLineTest2(CollisionContext* ctx, Vec3f* from, Vec3f* to, Vec3f* hit, CollisionPoly** poly,
                         s32 walls, s32 floors, s32 ceilings, s32 oneFace) {
    assert(ctx == &play.colCtx && walls && floors && ceilings && !oneFace);
    s32 bgId;
    return LineFixture(from, to, hit, poly, &bgId, nullptr);
}
constexpr u32 kAllBcc = 0x17; // walls, floors, ceilings and moving collision; both faces
u32 BgCheck_GetBccFlags(s32 walls, s32 floors, s32 ceilings, s32 oneFace, s32 dyna) {
    return (walls ? 1u : 0u) | (floors ? 2u : 0u) | (ceilings ? 4u : 0u) | (oneFace ? 8u : 0u) | (dyna ? 16u : 0u);
}
s32 BgCheck_CheckLineImpl(CollisionContext* ctx, u16 xpFlags1, u16 xpFlags2, Vec3f* from, Vec3f* to, Vec3f* hit,
                          CollisionPoly** poly, s32* bgId, Actor* skip, f32 chkDist, u32 bccFlags) {
    // Every collision category, both faces, moving collision included: as BgCheck_AnyLineTest2.
    assert(ctx == &play.colCtx && xpFlags1 == 0 && xpFlags2 == 0 && chkDist == 1.0f && bccFlags == kAllBcc);
    return LineFixture(from, to, hit, poly, bgId, skip);
}
DynaPolyActor* DynaPoly_GetActor(CollisionContext* ctx, s32 bgId) {
    assert(ctx == &play.colCtx);
    for (const auto& o : occluders)
        if (o.bgId == bgId && bgId != BGCHECK_SCENE)
            return o.owner;
    return nullptr;
}

// ---- Seed, save and settings fixtures ----
struct { int fileNum = 0; int language = 0; } gSaveContext;
bool randoEnabled = true, foreground = true, saveLoaded = true, mysterious = false;
#define IS_RANDO randoEnabled
#define CVAR_RANDOMIZER_ENHANCEMENT(name) "gRandoEnhancements." name
int CVarGetInteger(const char* key, int fallback) {
    return std::string(key) == "gRandoEnhancements.MysteriousShuffle" ? mysterious : fallback;
}
bool Combo_OotIsForeground() { return foreground; }
struct GameInteractor { static bool IsSaveLoaded() { return saveLoaded; } };
int saves = 0;
struct SaveManager {
    static SaveManager* Instance;
    void SaveSection(int fileNum, int section, bool threaded) {
        assert(fileNum == gSaveContext.fileNum && section == 7 && threaded);
        ++saves;
    }
} saveManager;
SaveManager* SaveManager::Instance = &saveManager;
struct Text { std::string english; std::string GetForLanguage(int) const { return english; } };
struct ItemObtainabilityTable { std::map<RandomizerGet, ItemObtainability> value; } obtainability;
namespace Rando {
struct Item {
    RandomizerGet rg = RG_NONE;
    std::string name;
    GetItemEntry entry;
    RandomizerGet GetRandomizerGet() const { return rg; }
    std::shared_ptr<GetItemEntry> GetGIEntry(RandomizerGet* converted = nullptr) const {
        if (converted) *converted = RG_NONE;
        return std::make_shared<GetItemEntry>(entry);
    }
    GetItemEntry GetGIEntry_Copy() const { return entry; }
    int GetItemID() const { return entry.getItemId; }
    Text GetName() const { return { name }; }
};
struct Location {
    RandomizerCheck rc;
    std::string name;
    RandomizerGet vanilla;
    const std::string& GetName() const { return name; }
    RandomizerGet GetVanillaItem() const { return vanilla; }
};
namespace StaticData {
std::vector<Item> items = {
    { RG_BLUE_RUPEE, "Blue Rupee", Entry(MOD_NONE, GI_RUPEE_BLUE, GID_RUPEE_BLUE) },
    { RG_DEKU_SHIELD, "Deku Shield", Entry(MOD_NONE, GI_SHIELD_DEKU, GID_SHIELD_DEKU) },
    { RG_PIECE_OF_HEART, "Piece of Heart", Entry(MOD_NONE, GI_HEART_PIECE, GID_HEART_PIECE) },
    { RG_ICE_TRAP, "Ice Trap", Entry(MOD_RANDOMIZER, RG_ICE_TRAP, GID_RUPEE_GOLD) },
    { RG_COMBO_FOREIGN, "Combo Foreign Item", Entry(MOD_RANDOMIZER, RG_COMBO_FOREIGN, GID_RUPEE_BLUE) },
};
std::vector<Item>& GetItemTable() { return items; }
const Item& RetrieveItem(RandomizerGet rg) {
    auto it = std::find_if(items.begin(), items.end(), [&](const Item& item) { return item.rg == rg; });
    assert(it != items.end());
    return *it;
}
std::map<RandomizerCheck, Location> locations;
Location* GetLocation(RandomizerCheck rc) {
    auto& loc = locations[rc];
    loc.rc = rc;
    if (loc.name.empty()) loc.name = "Check " + std::to_string(int(rc));
    return &loc;
}
} // namespace StaticData
struct ItemLocation {
    RandomizerGet placed = RG_NONE;
    RandomizerCheckStatus status = RCSHOW_UNCHECKED;
    bool obtained = false;
    RandomizerGet GetPlacedRandomizerGet() const { return placed; }
    const Item& GetPlacedItem() const { return StaticData::RetrieveItem(placed); }
    bool HasObtained() const { return obtained; }
    RandomizerCheckStatus GetCheckStatus() const { return status; }
    void SetCheckStatus(RandomizerCheckStatus value) { status = value; }
};
struct Override { RandomizerGet looksLike; RandomizerGet LooksLike() const { return looksLike; } };
struct Context {
    std::map<RandomizerCheck, ItemLocation> locations;
    std::map<RandomizerCheck, Override> overrides;
    ItemLocation* GetItemLocation(RandomizerCheck rc) { return &locations[rc]; }
    GetItemEntry GetFinalGIEntry(RandomizerCheck rc, bool checkObtainability = true, GetItemID ogItemId = GI_NONE);
};
} // namespace Rando
struct ItemTableManager {
    static ItemTableManager* Instance;
    GetItemEntry RetrieveItemEntry(int modIndex, int getItemId) {
        assert(modIndex == MOD_NONE);
        for (const auto& item : Rando::StaticData::items)
            if (item.entry.modIndex == MOD_NONE && item.entry.getItemId == getItemId) return item.entry;
        assert(false && "unknown vanilla entry");
        return {};
    }
} itemTables;
ItemTableManager* ItemTableManager::Instance = &itemTables;
struct Randomizer {
    ItemObtainability GetItemObtainabilityFromRandomizerGet(RandomizerGet rg) {
        auto it = obtainability.value.find(rg);
        return it == obtainability.value.end() ? CAN_OBTAIN : it->second;
    }
} randomizer;
struct OTRGlobals {
    static OTRGlobals* Instance;
    Randomizer* gRandomizer = &randomizer;
    std::shared_ptr<Rando::Context> gRandoContext = std::make_shared<Rando::Context>();
} globals;
OTRGlobals* OTRGlobals::Instance = &globals;
int OOT_GetForeignCategory(RandomizerCheck) { return 42; }
std::map<std::string, ComboRando::ForeignItem> foreignMap;
const ComboRando::ForeignItem* OOT_LookupForeign(int slot, const std::string& checkName) {
    assert(slot == gSaveContext.fileNum);
    auto it = foreignMap.find(checkName);
    return it == foreignMap.end() ? nullptr : &it->second;
}
const char* foreignLiveName = nullptr;
extern "C" const char* Randomizer_ComboForeignLiveName(int32_t) { return foreignLiveName; }
namespace CheckTracker {
std::map<RandomizerCheck, bool> unshuffled;
bool IsCheckShuffled(RandomizerCheck rc) { return !unshuffled[rc]; }
} // namespace CheckTracker
namespace Rando {
'''

tests = r'''
namespace CheckTracker {
int sectionId = 7;
}
using namespace CheckTracker;
auto& ctx = *globals.gRandoContext;
Actor item00{ ACTOR_EN_ITEM00 };
void ResetWorld() {
    ResetKnowledge();
    ctx.locations.clear();
    ctx.overrides.clear();
    obtainability.value.clear();
    occluders.clear();
    unshuffled.clear();
    foreignMap.clear();
    foreignLiveName = nullptr;
    play = PlayState{};
    play.view.eye = { 0, 0, 0 };
    drawnOrigin = { 0, 0, 100 };
    clip = { 0, 0, 50 };
    clipW = 100;
    aspect = 4.0f / 3.0f;
    testModel = MtxF{};
    mysterious = false;
    saves = raycasts = 0;
}
// Draw `rc`'s reward this frame, as the actor draw hooks do.
void Draw(RandomizerCheck rc, Actor* actor = &item00) {
    RecordObservedItem(actor, ctx.GetFinalGIEntry(rc, true));
}
void Seen(RandomizerCheck rc, uint8_t source, RandomizerGet item, const std::string& name) {
    const auto known = GetKnowledge(rc);
    assert(known.source == source && known.item == item && known.displayName == name);
    assert(ctx.GetItemLocation(rc)->GetCheckStatus() == RCSHOW_SEEN_OR_HINTED);
}
void Unseen(RandomizerCheck rc) {
    assert(GetKnowledge(rc).source == 0 && GetKnowledge(rc).displayName.empty());
    assert(ctx.GetItemLocation(rc)->GetCheckStatus() == RCSHOW_UNCHECKED);
}

void StandIns() {
    const auto tektite = RC_HF_TEKTITE_GROTTO_FREESTANDING_POH, cow = RC_HF_GS_COW_GROTTO;
    ResetWorld();
    // Screenshot 1: the heart piece check holds a Deku Shield Link already owns.
    ctx.GetItemLocation(tektite)->placed = RG_DEKU_SHIELD;
    obtainability.value[RG_DEKU_SHIELD] = CANT_OBTAIN_ALREADY_HAVE;
    const GetItemEntry standIn = ctx.GetFinalGIEntry(tektite, true);
    assert(standIn.modIndex == MOD_NONE && standIn.getItemId == GI_RUPEE_BLUE && standIn.gid == GID_RUPEE_BLUE);
    assert(standIn.comboForeignCheck == tektite); // the stand-in keeps its check
    assert(ctx.GetFinalGIEntry(tektite, false).getItemId == GI_SHIELD_DEKU);
    Draw(tektite);
    Seen(tektite, ComboRando::kTrackerSourceStandIn, RG_BLUE_RUPEE, "Blue Rupee");
    assert(saves == 1);
    Draw(tektite);
    assert(saves == 1); // nothing new on later frames
    // Once the item can be obtained, its real model replaces the stand-in record.
    obtainability.value.clear();
    Draw(tektite);
    Seen(tektite, 1, RG_DEKU_SHIELD, "Deku Shield");
    assert(saves == 2);
    // ... and the stand-in never overwrites the real sighting again.
    obtainability.value[RG_DEKU_SHIELD] = CANT_OBTAIN_ALREADY_HAVE;
    Draw(tektite);
    Seen(tektite, 1, RG_DEKU_SHIELD, "Deku Shield");
    assert(saves == 2);

    // Screenshot 2's token, on a check whose placed item is obtainable: its own model is recorded.
    ctx.GetItemLocation(cow)->placed = RG_PIECE_OF_HEART;
    Draw(cow, &item00);
    Seen(cow, 1, RG_PIECE_OF_HEART, "Piece of Heart");

    // A real Blue Rupee placement is an ordinary sighting, not a stand-in.
    const auto rupee = RC_HF_SOUTHEAST_GROTTO_CHEST;
    ctx.GetItemLocation(rupee)->placed = RG_BLUE_RUPEE;
    Draw(rupee);
    Seen(rupee, 1, RG_BLUE_RUPEE, "Blue Rupee");

    // A trap disguised as a Blue Rupee is not a stand-in: its disguise is what was seen.
    const auto trap = RC_HF_OPEN_GROTTO_CHEST;
    ctx.GetItemLocation(trap)->placed = RG_ICE_TRAP;
    ctx.overrides[trap] = { RG_BLUE_RUPEE };
    Draw(trap);
    Seen(trap, 1, RG_BLUE_RUPEE, "Blue Rupee");

    // A stand-in does not replace a hint of the real item, which outranks it.
    ResetWorld();
    ctx.GetItemLocation(tektite)->placed = RG_DEKU_SHIELD;
    ctx.GetItemLocation(tektite)->status = RCSHOW_SEEN_OR_HINTED;
    obtainability.value[RG_DEKU_SHIELD] = CANT_OBTAIN_ALREADY_HAVE;
    assert(StoreKnowledge(tektite, 2, RG_DEKU_SHIELD, "Deku Shield"));
    Draw(tektite);
    Seen(tektite, 2, RG_DEKU_SHIELD, "Deku Shield");

    // No record without a shuffled, unobtained, stamped check or while the model is disguised by settings.
    ResetWorld();
    ctx.GetItemLocation(tektite)->placed = RG_DEKU_SHIELD;
    obtainability.value[RG_DEKU_SHIELD] = CANT_OBTAIN_ALREADY_HAVE;
    unshuffled[tektite] = true;
    Draw(tektite);
    Unseen(tektite);
    unshuffled.clear();
    ctx.GetItemLocation(tektite)->obtained = true;
    Draw(tektite);
    Unseen(tektite);
    ctx.GetItemLocation(tektite)->obtained = false;
    mysterious = true;
    Draw(tektite);
    Unseen(tektite);
    mysterious = false;
    foreground = false;
    Draw(tektite);
    Unseen(tektite);
    foreground = true;
    const auto vanilla = RC_HF_NEAR_MARKET_GROTTO_CHEST;
    Rando::StaticData::GetLocation(vanilla)->vanilla = RG_BLUE_RUPEE;
    assert(ctx.GetFinalGIEntry(vanilla, true).comboForeignCheck == 0); // unshuffled entries name no check
    Draw(vanilla);
    Unseen(vanilla);
    RecordObservedItem(nullptr, ctx.GetFinalGIEntry(tektite, true));
    Unseen(tektite);
    assert(saves == 0);
    std::cout << "PASS: Blue Rupee stand-ins are Seen, rank below hints and sightings, and keep their check\n";
}

void Hidden() {
    const auto tektite = RC_HF_TEKTITE_GROTTO_FREESTANDING_POH;
    auto prepare = [&] {
        ResetWorld();
        ctx.GetItemLocation(tektite)->placed = RG_DEKU_SHIELD;
        obtainability.value[RG_DEKU_SHIELD] = CANT_OBTAIN_ALREADY_HAVE;
    };
    // Off screen, behind the camera, unrevealed, paused or mid-transition: never Seen.
    for (int i = 0; i < 9; ++i) {
        prepare();
        switch (i) {
            case 0: clip = { 101, 0, 50 }; break;
            case 1: clip = { 0, -101, 50 }; break;
            case 2: clipW = -100; break;
            case 3: testModel.yy = 0; break;
            case 4: play.pauseCtx.state = 1; break;
            case 5: play.transitionMode = 3; break;
            case 6: play.transitionTrigger = 1; break;
            case 7: play.envCtx.fillScreen = 1; play.envCtx.screenFillColor[3] = 255; break;
            case 8: play.envCtx.fillScreen = 1; play.envCtx.screenFillColor[3] = 1; break;
        }
        Draw(tektite);
        Unseen(tektite);
        assert(raycasts == 0); // invisible cases need no collision work
    }
    // A cutscene fade can leave the fill switched on at alpha 0, which draws nothing: the model is Seen.
    prepare();
    play.envCtx.fillScreen = 1;
    Draw(tektite);
    Seen(tektite, ComboRando::kTrackerSourceStandIn, RG_BLUE_RUPEE, "Blue Rupee");
    // In view but behind a wall or a closed door: never Seen.
    prepare();
    occluders = { { { 0, 0, 50 }, BGCHECK_SCENE, nullptr } };
    Draw(tektite);
    Unseen(tektite);
    prepare();
    occluders = { { { 0, 0, 50 }, 3, &door } };
    Draw(tektite);
    Unseen(tektite);
    // Shop shelves keep their invisible counter collision in front of the stock: shown stock is Seen.
    prepare();
    occluders = { { { 0, 0, 50 }, 3, &door } };
    Actor stock{ ACTOR_EN_GIRLA };
    Draw(tektite, &stock);
    Seen(tektite, ComboRando::kTrackerSourceStandIn, RG_BLUE_RUPEE, "Blue Rupee");
    std::cout << "PASS: off-screen, covered, unrevealed and occluded models stay unseen; a faded-out fill hides nothing\n";
}

void SeeThrough() {
    const auto cow = RC_HF_GS_COW_GROTTO, bowling = RC_MARKET_BOMBCHU_BOWLING_FIRST_PRIZE;
    auto prepare = [&](RandomizerCheck rc, std::vector<Occluder> blockers) {
        ResetWorld();
        ctx.GetItemLocation(rc)->placed = RG_PIECE_OF_HEART;
        occluders = std::move(blockers);
    };
    // Screenshot 2: the token is seen through the cow grotto's unburned web.
    prepare(cow, { { { 0, 0, 40 }, 3, &web } });
    Draw(cow);
    Seen(cow, 1, RG_PIECE_OF_HEART, "Piece of Heart");
    assert(raycasts == 2); // the line continues once, past the web
    prepare(cow, { { { 0, 0, 30 }, 3, &web }, { { 0, 0, 60 }, 4, &web2 } });
    Draw(cow);
    Seen(cow, 1, RG_PIECE_OF_HEART, "Piece of Heart");
    // A web never reveals what a wall, floor or closed door behind or in front of it hides.
    for (const auto& blockers : std::vector<std::vector<Occluder>>{
             { { { 0, 0, 30 }, 3, &web }, { { 0, 0, 60 }, BGCHECK_SCENE, nullptr } },
             { { { 0, 0, 30 }, BGCHECK_SCENE, nullptr }, { { 0, 0, 60 }, 3, &web } },
             { { { 0, 0, 30 }, 3, &web }, { { 0, 0, 60 }, 4, &door } },
             { { { 0, 0, 20 }, 3, &web }, { { 0, 0, 40 }, 4, &web2 }, { { 0, 0, 80 }, 5, &door } },
         }) {
        prepare(cow, blockers);
        Draw(cow);
        Unseen(cow);
    }
    // Bombchu Bowling prizes on the counter stand behind the alley's invisible guard wall.
    prepare(bowling, { { { 0, 0, 45 }, 6, &guard } });
    Draw(bowling);
    Seen(bowling, 1, RG_PIECE_OF_HEART, "Piece of Heart");
    // ... but the counter's own geometry still hides a prize from a low camera.
    prepare(bowling, { { { 0, 0, 45 }, 6, &guard }, { { 0, 0, 80 }, BGCHECK_SCENE, nullptr } });
    Draw(bowling);
    Unseen(bowling);
    std::cout << "PASS: webs and the bowling guard do not hide rewards; walls, floors and doors behind them do\n";
}

void Foreign() {
    const auto windmill = RC_KAK_WINDMILL_FREESTANDING_POH, other = RC_HF_TEKTITE_GROTTO_FREESTANDING_POH;
    auto prepare = [&] {
        ResetWorld();
        ctx.GetItemLocation(windmill)->placed = RG_COMBO_FOREIGN;
        auto& item = foreignMap[Rando::StaticData::GetLocation(windmill)->GetName()];
        item = {};
        item.itemGame = ComboRando::GAME_MM;
        item.itemName = "Progressive Sword";
        item.displayName = "Progressive Sword (MM)";
        return &item;
    };
    // The MM model drew: the sighting names the tier that model shows.
    prepare();
    foreignLiveName = "Razor Sword";
    const GetItemEntry sentinel = ctx.GetFinalGIEntry(windmill, true);
    assert(sentinel.getItemId == RG_COMBO_FOREIGN && sentinel.comboForeignCheck == windmill);
    RecordObservedItem(&item00, sentinel);
    Unseen(windmill); // nothing is stored before the model is drawn
    RecordForeignDraw(windmill, true);
    Seen(windmill, 1, RG_COMBO_FOREIGN, "Razor Sword (MM)");
    assert(saves == 1);
    RecordForeignDraw(windmill, true);
    assert(saves == 1); // one sighting per sampled draw
    // Without a resolved tier the spoiler's name is used.
    prepare();
    Draw(windmill);
    RecordForeignDraw(windmill, true);
    Seen(windmill, 1, RG_COMBO_FOREIGN, "Progressive Sword (MM)");
    // A disguised MM trap is recorded as its disguise.
    prepare()->fakeItemName = "Hookshot";
    foreignMap.begin()->second.fakeDisplayName = "Hookshot (MM)";
    Draw(windmill);
    RecordForeignDraw(windmill, true);
    Seen(windmill, 1, RG_COMBO_FOREIGN, "Hookshot (MM)");
    // The MM model failed and the sentinel Blue Rupee was drawn: nothing about the MM item is revealed.
    prepare();
    Draw(windmill);
    RecordForeignDraw(windmill, false);
    Unseen(windmill);
    // A draw result never applies to a different check or to a sighting another draw replaced.
    prepare();
    ctx.GetItemLocation(other)->placed = RG_PIECE_OF_HEART;
    Draw(windmill);
    RecordForeignDraw(other, true);
    RecordForeignDraw(windmill, true);
    Unseen(windmill);
    Draw(windmill);
    Draw(other);
    RecordForeignDraw(windmill, true);
    Unseen(windmill);
    Seen(other, 1, RG_PIECE_OF_HEART, "Piece of Heart");
    // Hidden or unknown foreign checks stay unseen even when the model draws.
    prepare();
    occluders = { { { 0, 0, 50 }, BGCHECK_SCENE, nullptr } };
    Draw(windmill);
    RecordForeignDraw(windmill, true);
    Unseen(windmill);
    prepare();
    foreignMap.clear();
    Draw(windmill);
    RecordForeignDraw(windmill, true);
    Unseen(windmill);
    std::cout << "PASS: MM items at OoT checks are Seen only once their real model was drawn, by its shown tier\n";
}

void RepeatsAndResets() {
    const auto cow = RC_HF_GS_COW_GROTTO, chest = RC_MARKET_TREASURE_CHEST_GAME_KEY_1;
    ResetWorld();
    ctx.GetItemLocation(cow)->placed = RG_PIECE_OF_HEART;
    occluders = { { { 0, 0, 40 }, 3, &web } };
    Draw(cow);
    Seen(cow, 1, RG_PIECE_OF_HEART, "Piece of Heart");
    const int rays = raycasts;
    for (int frame = 0; frame < 5; ++frame)
        Draw(cow);
    assert(raycasts == rays && saves == 1); // already recorded: no collision work and no save
    // The treasure chest game resets its rows on entry but keeps what the player learned. A key seen
    // again through the Lens of Truth marks the row again, even though the record did not change.
    ctx.GetItemLocation(chest)->placed = RG_PIECE_OF_HEART;
    Draw(chest);
    Seen(chest, 1, RG_PIECE_OF_HEART, "Piece of Heart");
    ctx.GetItemLocation(chest)->status = RCSHOW_UNCHECKED;
    occluders = { { { 0, 0, 40 }, BGCHECK_SCENE, nullptr } };
    Draw(chest);
    assert(ctx.GetItemLocation(chest)->GetCheckStatus() == RCSHOW_UNCHECKED); // not while hidden
    occluders.clear();
    Draw(chest);
    Seen(chest, 1, RG_PIECE_OF_HEART, "Piece of Heart");
    assert(saves == 3);
    // A stand-in seen after the reset re-marks the row with the real item it saw earlier.
    ctx.GetItemLocation(chest)->status = RCSHOW_UNCHECKED;
    obtainability.value[RG_PIECE_OF_HEART] = CANT_OBTAIN_MISC;
    Draw(chest);
    Seen(chest, 1, RG_PIECE_OF_HEART, "Piece of Heart");
    // Collected checks are never recorded again.
    ctx.GetItemLocation(cow)->obtained = true;
    ctx.GetItemLocation(cow)->status = RCSHOW_COLLECTED;
    const int before = raycasts;
    Draw(cow);
    assert(raycasts == before && ctx.GetItemLocation(cow)->GetCheckStatus() == RCSHOW_COLLECTED);
    std::cout << "PASS: recorded sightings skip the collision test; a reset row is re-marked when seen again\n";
}

int main() {
    StandIns();
    Hidden();
    SeeThrough();
    Foreign();
    RepeatsAndResets();
}
'''
code = prelude + final_entry + '\n} // namespace Rando\nnamespace CheckTracker {\n' + block + '\n} // namespace CheckTracker\n' + tests
cpp = out/'oot_check_sighting.cpp'
cpp.write_text(code, encoding='utf-8')
exe = hostcxx.build(a.compiler, cpp, includes=[source/'soh', source/'combo', a.json_include],
                    msvc=['/Zc:preprocessor', '/utf-8', '/wd4100'], gnu=['-Wno-unused-parameter'])
subprocess.run([str(exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)
