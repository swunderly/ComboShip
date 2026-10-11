// OoTMM v32.3 (413573d) actor/soul policy, MIT.
#include "OotSouls.h"
#include "global.h"
#include "soh/Enhancements/randomizer/logic.h"
#include "soh/Enhancements/randomizer/SeedContext.h"
#include "soh/Enhancements/randomizer/static_data.h"
#include "objects/object_gi_sutaru/object_gi_sutaru.h"
#include <fast/resource/type/DisplayList.h>
#include <ship/Context.h>
#include <ship/resource/ResourceManager.h>
#include "soh/frame_interpolation.h"
#include "soh/Enhancements/custom-message/CustomMessageTypes.h"
#include <algorithm>
namespace Rando::Souls {
struct NativeSoul {
    RandomizerGet item;
    unsigned index;
    OotSoul soul;
    const char* label;
};
constexpr NativeSoul generalSouls[] = {
#define OOT_SOUL_ITEM(item, index, soul, label) { item, index, soul, label },
#include "OotSoulData.inc"
#undef OOT_SOUL_ITEM
};
// HasItem asks for every item, so the soul block must be contiguous and in save-bit order.
constexpr bool SoulRowsFollowItemOrder() {
    for (unsigned i = 0; i < std::size(generalSouls); ++i)
        if (generalSouls[i].item != RG_SOUL_ENEMY_POE + i || generalSouls[i].index != i)
            return false;
    return RG_SOUL_MISC_GS - RG_SOUL_ENEMY_POE + 1 == std::size(generalSouls);
}
static_assert(SoulRowsFollowItemOrder());
int GeneralIndex(RandomizerGet item) {
    return item >= RG_SOUL_ENEMY_POE && item <= RG_SOUL_MISC_GS ? item - RG_SOUL_ENEMY_POE : -1;
}
// A settings profile, seed or CVar can carry a family that has no Randomizer menu entry yet, so
// unpublished families read as off: a stale value can never stop NPCs and enemies from spawning or
// add soul items to a seed.
GeneralOotSoulOptions Options() {
    auto ctx = Context::GetInstance();
    return PublishedGeneralOotSoulOptions(
        bool(ctx->GetOption(RSK_COMPLETE_OOT_SOULS).Get()), bool(ctx->GetOption(RSK_SHUFFLE_ENEMY_SOULS).Get()),
        bool(ctx->GetOption(RSK_SHUFFLE_NPC_SOULS).Get()), bool(ctx->GetOption(RSK_SHUFFLE_ANIMAL_SOULS).Get()),
        bool(ctx->GetOption(RSK_SHUFFLE_MISC_SOULS).Get()));
}
bool Enabled() {
    return Options().enabled;
}
GeneralOotSoulsOwnership Read(const SaveContext* save) {
    GeneralOotSoulsOwnership owned{};
    std::copy_n(save->ship.quest.data.randomizer.ootExpandedSoulsOwned, 12, owned.begin());
    return owned;
}
void Write(SaveContext* save, const GeneralOotSoulsOwnership& owned) {
    std::copy(owned.begin(), owned.end(), save->ship.quest.data.randomizer.ootExpandedSoulsOwned);
}
RandomizerGet BossItem(OotSoul soul) {
    switch (soul) {
        case OotSoul::BOSS_QUEEN_GOHMA:
            return RG_GOHMA_SOUL;
        case OotSoul::BOSS_KING_DODONGO:
            return RG_KING_DODONGO_SOUL;
        case OotSoul::BOSS_BARINADE:
            return RG_BARINADE_SOUL;
        case OotSoul::BOSS_PHANTOM_GANON:
            return RG_PHANTOM_GANON_SOUL;
        case OotSoul::BOSS_VOLVAGIA:
            return RG_VOLVAGIA_SOUL;
        case OotSoul::BOSS_MORPHA:
            return RG_MORPHA_SOUL;
        case OotSoul::BOSS_BONGO_BONGO:
            return RG_BONGO_BONGO_SOUL;
        case OotSoul::BOSS_TWINROVA:
            return RG_TWINROVA_SOUL;
        default:
            return RG_NONE;
    }
}
bool Has(Logic* logic, OotSoul soul) {
    auto boss = BossItem(soul);
    if (boss != RG_NONE)
        return logic->HasItem(boss);
    return HaveGeneralOotSoul(Read(logic->GetSaveContext()), Options(), soul);
}
bool FamilyShuffled(OotSoulFamily family) {
    return GeneralOotSoulFamilyShuffled(Options(), family);
}
bool TcgSoulAccess(Logic* logic) {
    return !Enabled() || Rando::Context::GetInstance()->GetOption(RSK_SHUFFLE_CHEST_MINIGAME).Get() != 0 ||
           Has(logic, OotSoul::NPC_CHEST_GAME_OWNER);
}
bool RuntimeOwned(RandomizerGet item) {
    int i = GeneralIndex(item);
    return i >= 0 && HaveGeneralOotSoul(Read(&gSaveContext), Options(), generalOotSoulItems[i]);
}
bool HasItem(Logic* logic, RandomizerGet item) {
    const int i = GeneralIndex(item);
    return i >= 0 && Has(logic, generalOotSoulItems[i]);
}
void LogicDelta(Logic* logic, RandomizerGet item, bool add) {
    int i = GeneralIndex(item);
    if (i < 0)
        return;
    auto owned = Read(logic->GetSaveContext());
    ChangeGeneralOotSoul(owned, i, add);
    Write(logic->GetSaveContext(), owned);
}
bool EnemySoul(Logic* logic, RandomizerEnemy enemy) {
    if (!Options().enabled)
        return true;
    switch (enemy) {
        case RE_GOLD_SKULLTULA:
            return Has(logic, OotSoul::MISC_GS);
        case RE_BIG_SKULLTULA:
            return Has(logic, OotSoul::ENEMY_SKULLTULA);
        case RE_DODONGO:
            return Has(logic, OotSoul::ENEMY_DODONGO);
        case RE_LIZALFOS:
            return Has(logic, OotSoul::ENEMY_LIZALFOS_DINOLFOS);
        case RE_GOHMA_LARVA:
            return Has(logic, OotSoul::ENEMY_GOHMA_LARVA);
        case RE_KEESE:
            return Has(logic, OotSoul::ENEMY_KEESE);
        case RE_FIRE_KEESE:
            return Has(logic, OotSoul::ENEMY_KEESE);
        case RE_GUAY:
            return Has(logic, OotSoul::ENEMY_GUAY);
        case RE_MAD_SCRUB:
            return Has(logic, OotSoul::ENEMY_DEKU_SCRUB);
        case RE_BLUE_BUBBLE:
            return Has(logic, OotSoul::ENEMY_BUBBLE);
        case RE_POE:
            return Has(logic, OotSoul::ENEMY_POE);
        case RE_DEAD_HAND:
            return Has(logic, OotSoul::ENEMY_DEAD_HAND);
        case RE_DEKU_BABA:
            return Has(logic, OotSoul::ENEMY_DEKU_BABA);
        case RE_WITHERED_DEKU_BABA:
            return Has(logic, OotSoul::ENEMY_DEKU_BABA);
        case RE_LIKE_LIKE:
            return Has(logic, OotSoul::ENEMY_LIKE_LIKE);
        case RE_STALFOS:
            return Has(logic, OotSoul::ENEMY_STALFOS);
        case RE_IRON_KNUCKLE:
            return Has(logic, OotSoul::ENEMY_IRON_KNUCKLE);
        case RE_FLARE_DANCER:
            return Has(logic, OotSoul::ENEMY_FLARE_DANCER);
        case RE_WOLFOS:
            return Has(logic, OotSoul::ENEMY_WOLFOS);
        case RE_FLOORMASTER:
            return Has(logic, OotSoul::ENEMY_FLOORMASTER);
        case RE_REDEAD:
            return Has(logic, OotSoul::ENEMY_REDEAD_GIBDO);
        case RE_MEG:
            return Has(logic, OotSoul::ENEMY_POE);
        case RE_ARMOS:
            return Has(logic, OotSoul::ENEMY_ARMOS);
        case RE_GREEN_BUBBLE:
            return Has(logic, OotSoul::ENEMY_BUBBLE);
        case RE_DINOLFOS:
            return Has(logic, OotSoul::ENEMY_LIZALFOS_DINOLFOS);
        case RE_TORCH_SLUG:
            return Has(logic, OotSoul::ENEMY_TORCH_SLUG);
        case RE_FREEZARD:
            return Has(logic, OotSoul::ENEMY_FREEZARD);
        case RE_SHELL_BLADE:
            return Has(logic, OotSoul::ENEMY_SHELL_BLADE);
        case RE_SPIKE:
            return Has(logic, OotSoul::ENEMY_SPIKE);
        case RE_WHITE_WOLFOS:
            return Has(logic, OotSoul::ENEMY_WOLFOS);
        case RE_STINGER:
            return Has(logic, OotSoul::ENEMY_STINGER);
        case RE_GANON:
            return Has(logic, OotSoul::NPC_ZELDA);
        case RE_BIG_OCTO:
            return Has(logic, OotSoul::ENEMY_OCTOROK) && Has(logic, OotSoul::NPC_RUTO);
        case RE_GERUDO_WARRIOR:
            return Has(logic, OotSoul::ENEMY_THIEVES);
        case RE_GIBDO:
            return Has(logic, OotSoul::ENEMY_REDEAD_GIBDO);
        case RE_DARK_LINK:
            return Has(logic, OotSoul::ENEMY_DARK_LINK);
        case RE_ANUBIS:
            return Has(logic, OotSoul::ENEMY_ANUBIS);
        case RE_BEAMOS:
            return Has(logic, OotSoul::ENEMY_BEAMOS);
        case RE_WALLMASTER:
            return Has(logic, OotSoul::ENEMY_WALLMASTER);
        case RE_PURPLE_LEEVER:
            return Has(logic, OotSoul::ENEMY_LEEVER);
        case RE_TENTACLE:
            return Has(logic, OotSoul::ENEMY_PARASITE);
        case RE_BARI:
            return Has(logic, OotSoul::ENEMY_BIRI_BARI);
        case RE_SHABOM:
            return Has(logic, OotSoul::ENEMY_SHABOM);
        case RE_OCTOROK:
            return Has(logic, OotSoul::ENEMY_OCTOROK);
        case RE_WALLTULA:
            return Has(logic, OotSoul::ENEMY_SKULLWALLTULA);
        default:
            return true;
    }
}
void AddUnshuffledStarts(std::vector<RandomizerGet>& starts) {
    const auto options = Options();
    if (!options.enabled)
        return;
    for (const auto& row : generalSouls)
        if (!GeneralOotSoulShuffled(options, row.index))
            starts.push_back(row.item);
}
void InitStartingOwnership() {
    const auto options = Options();
    if (!options.enabled)
        return;
    auto owned = Read(&gSaveContext);
    for (const auto& row : generalSouls)
        if (!GeneralOotSoulShuffled(options, row.index))
            ChangeGeneralOotSoul(owned, row.index, true);
    Write(&gSaveContext, owned);
}
void AddPool(std::vector<RandomizerGet>& pool) {
    const auto options = Options();
    for (const auto& row : generalSouls)
        if (GeneralOotSoulShuffled(options, row.index))
            pool.push_back(row.item);
}
} // namespace Rando::Souls
// Pinned CustomGS: native token geometry, gray/white skull and family-colored flame.
// Copy only this frame's display lists; never mutate shared/cosmetic resources.
static Gfx* SoulTokenList(PlayState* play, const char* path, size_t prim, size_t env, unsigned r, unsigned g,
                          unsigned b, unsigned er, unsigned eg, unsigned eb) {
    auto resource = std::dynamic_pointer_cast<Fast::DisplayList>(
        Ship::Context::GetRawInstance()->GetResourceManager()->LoadResource(path));
    if (!resource || prim >= resource->Instructions.size() || env >= resource->Instructions.size())
        return nullptr;
    if ((resource->Instructions[prim].words.w0 >> 24) != G_SETPRIMCOLOR ||
        (resource->Instructions[env].words.w0 >> 24) != G_SETENVCOLOR)
        return nullptr;
    auto copy = static_cast<Gfx*>(Graph_Alloc(play->state.gfxCtx, resource->Instructions.size() * sizeof(Gfx)));
    std::copy(resource->Instructions.begin(), resource->Instructions.end(), copy);
    gDPSetPrimColor(&copy[prim], 0, 0x80, r, g, b, 255);
    gDPSetEnvColor(&copy[env], er, eg, eb, 255);
    return copy;
}
static void DrawGeneralOotSoul(PlayState* play, GetItemEntry* item) {
    int index = Rando::Souls::GeneralIndex(static_cast<RandomizerGet>(item->getItemId));
    if (index < 0)
        return;
    const unsigned colors[4][3] = { { 255, 0, 0 }, { 0, 255, 0 }, { 34, 34, 255 }, { 136, 136, 136 } };
    unsigned family = Rando::Souls::GeneralOotSoulFamily(index);
    const auto& c = colors[family];
    auto skull = SoulTokenList(play, gGiSkulltulaTokenDL, 5, 6, 255, 255, 255, 170, 170, 170);
    auto flame = SoulTokenList(play, gGiSkulltulaTokenFlameDL, 32, 33, c[0], c[1], c[2], c[0], c[1], c[2]);
    if (!skull || !flame)
        return;
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_MODELVIEW | G_MTX_LOAD);
    gSPDisplayList(POLY_OPA_DISP++, skull);
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    gSPSegment(POLY_XLU_DISP++, 8,
               (uintptr_t)Gfx_TwoTexScroll(play->state.gfxCtx, 0, 0, -play->state.frames * 5, 32, 32, 1, 0, 0, 32, 64));
    gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_MODELVIEW | G_MTX_LOAD);
    gSPDisplayList(POLY_XLU_DISP++, flame);
    CLOSE_DISPS(play->state.gfxCtx);
}
namespace Rando::Souls {
void RegisterItems(std::array<Item, RG_MAX>& items) {
    for (const auto& row : generalSouls) {
        items[row.item] = Item(row.item, Text{ row.label, row.label, row.label }, ITEMTYPE_ITEM, row.item, true,
                               LOGIC_NONE, RHT_NONE, row.item, OBJECT_GI_SUTARU, GID_SKULL_TOKEN,
                               TEXT_RANDOMIZER_CUSTOM_ITEM, 0x80, CHEST_ANIM_LONG, ITEM_CATEGORY_MAJOR, MOD_RANDOMIZER);
        items[row.item].SetCustomDrawFunc(DrawGeneralOotSoul);
    }
}
} // namespace Rando::Souls
static const void* soulPlay = nullptr;
static int soulRoom = -1;
static bool soulRoomBlocked = false;
extern "C" void Combo_OotSoulResetRoom(void* raw) {
    auto play = static_cast<PlayState*>(raw);
    soulPlay = play;
    soulRoom = play->roomCtx.curRoom.num;
    soulRoomBlocked = false;
}
extern "C" int Combo_OotSoulCanClear(void* raw, int room) {
    return !(soulRoomBlocked && soulPlay == raw && soulRoom == room);
}
// OoTMM's En_Blkobj hook makes Actor_Find report Dark Link present while his soul is missing.
extern "C" int Combo_OotDarkLinkSoulMissing(void) {
    return IS_RANDO && !Rando::Souls::HaveGeneralOotSoul(Rando::Souls::Read(&gSaveContext), Rando::Souls::Options(),
                                                         Rando::Souls::OotSoul::ENEMY_DARK_LINK);
}
extern "C" int Combo_OotSoulPreSpawn(void* raw, int actor, unsigned params) {
    if (!IS_RANDO || !Rando::Souls::Options().enabled)
        return 1;
    auto play = static_cast<PlayState*>(raw);
    // Runtime ownership uses the real save, independent of generator's logic save.
    const auto owned = Rando::Souls::Read(&gSaveContext);
    const auto options = Rando::Souls::Options();
    // The auxiliary branch only tests NPC souls (Zelda's, for the Ocarina of Time), so it can block
    // nothing unless NPC souls are shuffled.
    if (options.npc) {
        const int aux = Rando::Souls::EvaluateOotAuxSoulPreSpawn(
            actor,
            CHECK_QUEST_ITEM(QUEST_KOKIRI_EMERALD) && CHECK_QUEST_ITEM(QUEST_GORON_RUBY) &&
                CHECK_QUEST_ITEM(QUEST_ZORA_SAPPHIRE),
            [&](Rando::Souls::OotSoul soul) { return Rando::Souls::HaveGeneralOotSoul(owned, options, soul); });
        if (aux <= 0)
            return -1; // pinned early auxiliary rejection never marks an enemy room deficient
    }
    // Bosses stay with SoH's own boss-soul hook (hook_handlers.cpp), so these families never change
    // how a boss spawns.
    int result = Rando::Souls::EvaluateOotSoulPreSpawn(
        actor, params, play->sceneNum, gSaveContext.linkAge == 0, gSaveContext.gameMode == GAMEMODE_END_CREDITS,
        [&](Rando::Souls::OotSoul soul) {
            return Rando::Souls::BossItem(soul) != RG_NONE || Rando::Souls::HaveGeneralOotSoul(owned, options, soul);
        });
    if (result == 0) {
        if (soulPlay != play || soulRoom != play->roomCtx.curRoom.num)
            Combo_OotSoulResetRoom(play);
        soulRoomBlocked = true;
    }
    return result;
}
extern "C" int Combo_OotSoulGive(void*, int item) {
    int index = Rando::Souls::GeneralIndex(static_cast<RandomizerGet>(item));
    if (index < 0)
        return 0;
    auto owned = Rando::Souls::Read(&gSaveContext);
    Rando::Souls::ChangeGeneralOotSoul(owned, index, true);
    Rando::Souls::Write(&gSaveContext, owned);
    return 1;
}
