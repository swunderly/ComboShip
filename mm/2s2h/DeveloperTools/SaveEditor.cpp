#include "SaveEditor.h"
#include "OwlStatueEditor.h"
#include "gui/ComboTouchSlider.h"
#ifdef COMBO_BUILD
#include "gui/ComboSaveEditorKit.h"
#endif
#include <fast/Fast3dGui.h>
#include "2s2h/BenGui/UIWidgets.hpp"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/Rando/Rando.h"
#include "2s2h/Rando/MiscBehavior/ClockShuffle.h"
#include "2s2h/CustomMessage/CustomMessage.h"
#include "2s2h/CustomItem/CustomItem.h"
#include "2s2h/BenGui/Notification.h"
#include "2s2h/Rando/Spoiler/Spoiler.h"
#include "2s2h/Rando/Ocarinas.h"
#include "2s2h/ShipUtils.h"
#ifdef COMBO_BUILD
#include <spdlog/spdlog.h>
#include "rando/CrossForeign.h" // ComboShip: GameId
#include "rando/SharedItems.h"  // same public readiness as the shared menu
// ComboShip (issue #3): immediate cross-game delivery seam (defined in BenPort.cpp).
extern "C" void (*gMMComboCrossDeliver)(int targetGame, const char* itemName, const char* srcCheckName);
// The MM gear runtime is released and prepared on this device (BenPort.cpp).
extern "C" int Combo_MM_GearOptionReady(void);
// The ownership grants of a real pickup (Rando::GiveItem): Rando/Spells.cpp, Rando/FaroresWind.cpp, BenPort.cpp.
extern "C" void Combo_MMSpellGrantItem(int spell);
extern "C" void Combo_MMFaroreGrantItem(void);
extern "C" int MM_GrantAdultGearItem(int fileNum, int item);
#endif

#include "interface/icon_item_dungeon_static/icon_item_dungeon_static.h"
#include "archives/icon_item_24_static/icon_item_24_static_yar.h"

#include <fast/Fast3dGui.h>
#include <fast/Fast3dWindow.h>

extern "C" {
#include <z64.h>
#include <z64save.h>
#include <macros.h>
#include <variables.h>
#include <functions.h>
#include "overlays/actors/ovl_En_Test4/z_en_test4.h"
#include "overlays/actors/ovl_Obj_Tokei_Step/z_obj_tokei_step.h"

extern PlayState* gPlayState;
extern SaveContext gSaveContext;
extern s16 D_801CFF94[250];
void Interface_LoadItemIconImpl(PlayState* play, u8 btn);
void Interface_NewDay(PlayState* play, s32 day);
extern RegEditor* gRegEditor;
extern ActorOverlay gActorOverlayTable[ACTOR_ID_MAX];
extern s16 gPlayerFormObjectIds[PLAYER_FORM_MAX];
void Player_PlaySfx(Player* player, u16 sfxId);
void PlayerCall_Init(Actor* thisx, PlayState* play);
void PlayerCall_Update(Actor* thisx, PlayState* play);
void PlayerCall_Draw(Actor* thisx, PlayState* play);
void TransitionFade_SetColor(void* thisx, u32 color);

void ObjTokeiStep_SetupOpen(ObjTokeiStep* objTokeiStep);
void ObjTokeiStep_DrawOpen(Actor* actor, PlayState* play);
void ObjTokeiStep_DoNothing(ObjTokeiStep* objTokeiStep, PlayState* play);
void EnTest4_GetBellTimeOnDay3(EnTest4* thisx);
void EnTest4_GetBellTimeAndShrinkScreenBeforeDay3(EnTest4* thisx, PlayState* play);
}

bool safeMode = true;
#ifdef COMBO_BUILD
// ComboShip: the sizes both Save Editors share (ComboSaveEditorKit): these values on Android, the desktop ones below
// elsewhere.
constexpr float INV_GRID_WIDTH = ComboSaveEditorKit::kGridWidth;
constexpr float INV_GRID_HEIGHT = ComboSaveEditorKit::kGridHeight;
constexpr float INV_GRID_ICON_SIZE = ComboSaveEditorKit::kIconSize;
constexpr float INV_GRID_PADDING = ComboSaveEditorKit::kGridPadding;
constexpr float INV_GRID_TOP_MARGIN = ComboSaveEditorKit::kGridTopMargin;
constexpr float INV_GRID_AMMO_WIDTH = ComboSaveEditorKit::kAmmoWidth;
constexpr float INV_GRID_AMMO_PADDING = ComboSaveEditorKit::kAmmoPadding;
constexpr int INV_PICKER_COLUMNS = ComboSaveEditorKit::kPickerColumns;
#elif defined(__ANDROID__)
// Large touch targets for the Save Editor on phones/tablets.
// Row height deliberately reserves space BELOW an ammo-bearing icon so the
// numeric input can never overlap the icon in the following row.
constexpr float INV_GRID_WIDTH = 136.0f;
constexpr float INV_GRID_HEIGHT = 180.0f;
constexpr float INV_GRID_ICON_SIZE = 120.0f;
constexpr float INV_GRID_PADDING = 16.0f;
constexpr float INV_GRID_TOP_MARGIN = 28.0f;
constexpr float INV_GRID_AMMO_WIDTH = 60.0f;
constexpr float INV_GRID_AMMO_PADDING = 8.0f;
constexpr int INV_PICKER_COLUMNS = 5;
#else
constexpr float INV_GRID_WIDTH = 46.0f;
constexpr float INV_GRID_HEIGHT = 58.0f;
constexpr float INV_GRID_ICON_SIZE = 40.0f;
constexpr float INV_GRID_PADDING = 10.0f;
constexpr float INV_GRID_TOP_MARGIN = 20.0f;
constexpr float INV_GRID_AMMO_WIDTH = 24.0f;
constexpr float INV_GRID_AMMO_PADDING = 4.0f;
constexpr int INV_PICKER_COLUMNS = 8;
#endif
constexpr uint16_t HEART_COUNT_MIN = 3;
constexpr uint16_t HEART_COUNT_MAX = 20;
constexpr int16_t S16_ZERO = 0;
constexpr int8_t S8_ZERO = 0;
constexpr u8 U8_ZERO = 0;
constexpr u8 REG_PAGES_MAX = REG_PAGES;
constexpr u8 REG_GROUPS_MAX = REG_GROUPS - 1;
#ifdef COMBO_BUILD
const char* MAGIC_LEVEL_NAMES[3] = { "No magic", "Single magic", "Double magic" };
#else
const char* MAGIC_LEVEL_NAMES[3] = { "No Magic", "Single Magic", "Double Magic" };
#endif
constexpr int8_t MAGIC_LEVEL_MAX = 2;
const char* WALLET_LEVEL_NAMES[4] = { "Child Wallet", "Adult Wallet", "Giant Wallet", "Tycoon Wallet" };
constexpr u8 WALLET_LEVEL_MAX = 2;
ImVec4 colorTint;
const char* songTooltip;
const char* curForm;
UIWidgets::Colors formColor;
uint32_t formObject;
static std::unordered_map<RandoItemId, const char*> randoItemIdComboboxMap;

InventorySlot selectedInventorySlot = SLOT_NONE;
std::vector<ItemId> safeItemsForInventorySlot[SLOT_MASK_FIERCE_DEITY + 1] = {};

using namespace UIWidgets;

// Public availability is independent of startup resource readiness and saved RO flags, except for the MM gear,
// which is offered only where its runtime is released and prepared (Combo_MM_GearOptionReady).
// The Adult Mask stays unavailable until actual native gameplay is validated.
static bool IsInventoryItemPubliclyAvailable(ItemId item) {
#ifdef COMBO_BUILD
    switch (item) {
        case (ItemId)0xbc: // Adult Mask is private until full gameplay validation.
            return false;
        case ITEM_COMBO_BOOTS_IRON:
            return ComboRando::SharedFamilyByIndex(ComboRando::SF_BOOTS_IRON).publicOption &&
                   Combo_MM_GearOptionReady();
        case ITEM_COMBO_BOOTS_HOVER:
            return ComboRando::SharedFamilyByIndex(ComboRando::SF_BOOTS_HOVER).publicOption &&
                   Combo_MM_GearOptionReady();
        case ITEM_COMBO_TUNIC_GORON:
            return ComboRando::SharedFamilyByIndex(ComboRando::SF_TUNIC_GORON).publicOption &&
                   Combo_MM_GearOptionReady();
        case ITEM_COMBO_TUNIC_ZORA:
            return ComboRando::SharedFamilyByIndex(ComboRando::SF_TUNIC_ZORA).publicOption &&
                   Combo_MM_GearOptionReady();
        case ITEM_COMBO_FARORES_WIND:
            return ComboRando::SharedFamilyByIndex(ComboRando::SF_SPELL_WIND).publicOption;
        case ITEM_COMBO_DINS_FIRE:
            return ComboRando::SharedFamilyByIndex(ComboRando::SF_SPELL_FIRE).publicOption;
        case ITEM_COMBO_NAYRUS_LOVE:
            return ComboRando::SharedFamilyByIndex(ComboRando::SF_SPELL_LOVE).publicOption;
        default:
            break;
    }
#endif
    return true;
}

static bool IsRandomizerItemGivePubliclyAvailable(RandoItemId item) {
#ifdef COMBO_BUILD
    switch (item) {
        case RI_MASK_ADULT:
            return false;
        case RI_BOOTS_IRON:
            return IsInventoryItemPubliclyAvailable(ITEM_COMBO_BOOTS_IRON);
        case RI_BOOTS_HOVER:
            return IsInventoryItemPubliclyAvailable(ITEM_COMBO_BOOTS_HOVER);
        case RI_TUNIC_GORON:
            return IsInventoryItemPubliclyAvailable(ITEM_COMBO_TUNIC_GORON);
        case RI_TUNIC_ZORA:
            return IsInventoryItemPubliclyAvailable(ITEM_COMBO_TUNIC_ZORA);
        case RI_FARORES_WIND:
            return IsInventoryItemPubliclyAvailable(ITEM_COMBO_FARORES_WIND);
        case RI_DINS_FIRE:
            return IsInventoryItemPubliclyAvailable(ITEM_COMBO_DINS_FIRE);
        case RI_NAYRUS_LOVE:
            return IsInventoryItemPubliclyAvailable(ITEM_COMBO_NAYRUS_LOVE);
        default:
            break;
    }
#endif
    return true;
}

// Explicit private runtime test only. It queues the unmodified native GI action;
// it never edits ownership, magic, equipment, point state or the public readiness gate.
#if defined(COMBO_BUILD) && defined(COMBO_ADULT_MM_RUNTIME_DIAGNOSTICS)
extern "C" int MM_DebugAdultResourcesReady(void);
#endif
// Private build AND runtime switch AND typed owning resources; no option/ownership writes.
static bool IsNativeAdultDiagnosticQueueAvailable(RandoItemId item) {
#if defined(COMBO_BUILD) && defined(COMBO_ADULT_MM_RUNTIME_DIAGNOSTICS)
    if (item != RI_MASK_ADULT || !gPlayState || gSaveContext.fileNum < 0 || gSaveContext.fileNum > 2 ||
        gSaveContext.save.shipSaveInfo.saveType != SAVETYPE_RANDO ||
        !gSaveContext.save.shipSaveInfo.rando.randoSaveOptions[RO_MM_ADULT_MASK])
        return false;
    return MM_DebugAdultResourcesReady() != 0;
#else
    return false;
#endif
}

void initSafeItemsForInventorySlot() {
    // Idempotent: the Item Tracker also primes this (combo: the Save Editor window that used to be the
    // sole caller can be rejected as a duplicate name, so its InitElement never runs). Re-running would
    // double the per-slot lists.
    static bool sInitialized = false;
    if (sInitialized) {
        return;
    }
    sInitialized = true;
    for (int i = 0; i < sizeof(gItemSlots); i++) {
        if (!IsInventoryItemPubliclyAvailable(static_cast<ItemId>(i))) {
            continue;
        }
        InventorySlot slot = static_cast<InventorySlot>(gItemSlots[i]);
        // Expanded item table uses SLOT_NONE for non-inventory/custom gaps.
        if (slot < SLOT_OCARINA || slot > SLOT_MASK_FIERCE_DEITY) {
            continue;
        }
        switch (slot) {
            case SLOT_BOTTLE_1:
                if (i != ITEM_LONGSHOT) { // No longshot in bottles
                    safeItemsForInventorySlot[SLOT_BOTTLE_1].push_back(static_cast<ItemId>(i));
                    safeItemsForInventorySlot[SLOT_BOTTLE_2].push_back(static_cast<ItemId>(i));
                    safeItemsForInventorySlot[SLOT_BOTTLE_3].push_back(static_cast<ItemId>(i));
                    safeItemsForInventorySlot[SLOT_BOTTLE_4].push_back(static_cast<ItemId>(i));
                    safeItemsForInventorySlot[SLOT_BOTTLE_5].push_back(static_cast<ItemId>(i));
                    safeItemsForInventorySlot[SLOT_BOTTLE_6].push_back(static_cast<ItemId>(i));
                }
                break;
            case SLOT_BOW:
                if (i == ITEM_BOW) { // No elemental bows here
                    safeItemsForInventorySlot[slot].push_back(static_cast<ItemId>(i));
                }
                break;
            case SLOT_TRADE_KEY_MAMA:
                if (i != ITEM_SLINGSHOT) { // No slingshot in trade items
                    safeItemsForInventorySlot[slot].push_back(static_cast<ItemId>(i));
                }
                break;
            case SLOT_TRADE_DEED:
                if (i != ITEM_OCARINA_FAIRY) { // No fairy ocarina in trade items
                    safeItemsForInventorySlot[slot].push_back(static_cast<ItemId>(i));
                }
                break;
            default:
                safeItemsForInventorySlot[slot].push_back(static_cast<ItemId>(i));
                break;
        }
    }
}

char z2ASCII(int code) {
    int ret;
    if (code < 10) { // Digits
        ret = code + 0x30;
    } else if (code >= 10 && code < 36) { // Uppercase letters
        ret = code + 0x37;
    } else if (code >= 36 && code < 62) { // Lowercase letters
        ret = code + 0x3D;
    } else if (code == 62) { // Space
        ret = code - 0x1E;
    } else if (code == 63 || code == 64) { // _ and .
        ret = code - 0x12;
    } else {
        ret = code;
    }
    return char(ret);
}

char ASCII2z(int code) {
    int ret;
    if (code >= 0x30 && code < 0x3A) { // Digits
        ret = code - 0x30;
    } else if (code >= 0x41 && code < 0x5B) { // Uppercase letters
        ret = code - 0x37;
    } else if (code >= 0x61 && code < 0x7B) { // Lowercase letters
        ret = code - 0x3D;
    } else if (code == 0x20 || code == 0x0) { // Space
        ret = 62;
    } else if (code == 0x5F || code == 0x2E) { // _ and .
        ret = code + 0x12;
    } else {
        ret = code;
    }
    return char(ret);
}

char* getPlayerName(char* playerNameBuf) {
    for (int i = 0; i < 8; i++) {
        playerNameBuf[i] = z2ASCII(gSaveContext.save.saveInfo.playerData.playerName[i]);
    }
    playerNameBuf[8] = '\0';
    return playerNameBuf;
}

int setPlayerName(ImGuiInputTextCallbackData* data) {
    for (int i = 0; i < 8; i++) {
        gSaveContext.save.saveInfo.playerData.playerName[i] = ASCII2z(data->Buf[i]);
    }
    return 0;
};

EnTest4* FindEnTest4Actor() {
    if (gPlayState == NULL) {
        return NULL;
    }

    Actor* enTest4Search = gPlayState->actorCtx.actorLists[ACTORCAT_SWITCH].first;

    while (enTest4Search != NULL) {
        if (enTest4Search->id == ACTOR_EN_TEST4) {
            return (EnTest4*)enTest4Search;
        }
        enTest4Search = enTest4Search->next;
    }

    return NULL;
}

void UpdateGameTime(u16 gameTime) {
    bool newTimeIsNight = (gameTime > CLOCK_TIME(18, 0)) || (gameTime < CLOCK_TIME(6, 0));
    bool prevTimeIsNight = (CURRENT_TIME > CLOCK_TIME(18, 0)) || (CURRENT_TIME < CLOCK_TIME(6, 0));

    gSaveContext.save.time = gameTime;

    if (gPlayState == NULL) {
        return;
    }

    // Clear weather from day 2
    gWeatherMode = WEATHER_MODE_CLEAR;
    gPlayState->envCtx.lightningState = LIGHTNING_OFF;

    // When transitioning over night boundaries, stop the sequences and ask to replay, then respawn actors
    if (newTimeIsNight != prevTimeIsNight) {
        // AMBIENCE_ID_13 is used to persist a scenes sequence through night, so we shouldn't
        // change anything if thats active
        if (gPlayState->sceneSequences.ambienceId != AMBIENCE_ID_13) {
            SEQCMD_STOP_SEQUENCE(SEQ_PLAYER_AMBIENCE, 0);
            SEQCMD_STOP_SEQUENCE(SEQ_PLAYER_BGM_MAIN, 240);
            gSaveContext.seqId = NA_BGM_DISABLED;
            gSaveContext.ambienceId = AMBIENCE_ID_DISABLED;
            Environment_PlaySceneSequence(gPlayState);
        }

        // Kills/Spawns half-day actors
        gPlayState->numSetupActors = -gPlayState->numSetupActors;
    }

    EnTest4* enTest4 = FindEnTest4Actor();

    // Update EnTest4 actor to be in sync with the new time
    // This ensures that day transitions are not triggered with the change
    if (enTest4 != NULL) {
        enTest4->prevTime = gameTime;
        enTest4->prevBellTime = gameTime;
        enTest4->daytimeIndex = newTimeIsNight ? 0 : 1;

        // Sets the nextBellTime based on the new current time
        if (CURRENT_DAY == 3) {
            EnTest4_GetBellTimeOnDay3(enTest4);
        } else {
            EnTest4_GetBellTimeAndShrinkScreenBeforeDay3(enTest4, gPlayState);
        }

        // Unset any screen scaling from the above funcs
        gSaveContext.screenScale = 1000.0f;
        gSaveContext.screenScaleFlag = false;
    }

    // Open the Clock Tower rooftop
    if (((CURRENT_DAY == 3) && (CURRENT_TIME < CLOCK_TIME(6, 0)))) {
        ObjTokeiStep* objTokeiStep = (ObjTokeiStep*)Actor_FindNearby(gPlayState, &GET_PLAYER(gPlayState)->actor,
                                                                     ACTOR_OBJ_TOKEI_STEP, ACTORCAT_BG, 99999.9f);
        if (objTokeiStep != NULL && objTokeiStep->actionFunc == ObjTokeiStep_DoNothing) {
            objTokeiStep->dyna.actor.draw = ObjTokeiStep_DrawOpen;
            ObjTokeiStep_SetupOpen(objTokeiStep);
        }
    }
}

void DrawTempleClears() {
    bool cleared;
    bool open;
    bool inverted = false;
    bool inStoneTower = false;

    if (gPlayState != NULL) {
        inStoneTower = Play_GetOriginalSceneId(gPlayState->sceneId) == SCENE_F40;
        inverted = Flags_GetSwitch(gPlayState, 20);
    }

    ImGui::Columns(2, nullptr, false);

    // Left column: Cleared checkboxes
    // Woodfall
    cleared = CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_WOODFALL_TEMPLE);
    if (UIWidgets::Checkbox("Woodfall cleared", &cleared)) {
        if (cleared) {
            SET_WEEKEVENTREG(WEEKEVENTREG_CLEARED_WOODFALL_TEMPLE);
        } else {
            CLEAR_WEEKEVENTREG(WEEKEVENTREG_CLEARED_WOODFALL_TEMPLE);
        }
    }

    // Snowhead
    cleared = CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE);
    if (UIWidgets::Checkbox("Snowhead cleared", &cleared)) {
        if (cleared) {
            SET_WEEKEVENTREG(WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE);
        } else {
            CLEAR_WEEKEVENTREG(WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE);
        }
    }

    // Great Bay
    cleared = CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_GREAT_BAY_TEMPLE);
    if (UIWidgets::Checkbox("Great Bay cleared", &cleared)) {
        if (cleared) {
            SET_WEEKEVENTREG(WEEKEVENTREG_CLEARED_GREAT_BAY_TEMPLE);
        } else {
            CLEAR_WEEKEVENTREG(WEEKEVENTREG_CLEARED_GREAT_BAY_TEMPLE);
        }
    }

    // Stone Tower
    cleared = CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_STONE_TOWER_TEMPLE);
    if (UIWidgets::Checkbox("Stone Tower cleared", &cleared)) {
        if (cleared) {
            SET_WEEKEVENTREG(WEEKEVENTREG_CLEARED_STONE_TOWER_TEMPLE);
        } else {
            CLEAR_WEEKEVENTREG(WEEKEVENTREG_CLEARED_STONE_TOWER_TEMPLE);
        }
    }

    ImGui::NextColumn();

    // Right column: Open/Inverted checkboxes
    // Woodfall
    cleared = CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_WOODFALL_TEMPLE);
    if (cleared) {
        open = true;
        SET_WEEKEVENTREG(WEEKEVENTREG_20_01);
    } else {
        open = CHECK_WEEKEVENTREG(WEEKEVENTREG_20_01);
    }
    if (UIWidgets::Checkbox("Woodfall Open", &open, { { .disabled = cleared } })) {
        if (open) {
            SET_WEEKEVENTREG(WEEKEVENTREG_20_01);
        } else {
            CLEAR_WEEKEVENTREG(WEEKEVENTREG_20_01);
        }
    }

    // Snowhead
    cleared = CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE);
    if (cleared) {
        open = true;
        SET_WEEKEVENTREG(WEEKEVENTREG_30_01);
    } else {
        open = CHECK_WEEKEVENTREG(WEEKEVENTREG_30_01);
    }
    if (UIWidgets::Checkbox("Snowhead Open", &open, { { .disabled = cleared } })) {
        if (open) {
            SET_WEEKEVENTREG(WEEKEVENTREG_30_01);
        } else {
            CLEAR_WEEKEVENTREG(WEEKEVENTREG_30_01);
        }
    }

    // Great Bay
    cleared = CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_GREAT_BAY_TEMPLE);
    if (cleared) {
        open = true;
        // 53_20 is if the turtle is raised
        // 93_08 determines if a long or short cutscene should play
        SET_WEEKEVENTREG(WEEKEVENTREG_53_20);
        SET_WEEKEVENTREG(WEEKEVENTREG_93_08);
    } else {
        open = CHECK_WEEKEVENTREG(WEEKEVENTREG_53_20);
    }
    if (UIWidgets::Checkbox("Great Bay Open", &open, { { .disabled = cleared } })) {
        if (open) {
            SET_WEEKEVENTREG(WEEKEVENTREG_53_20);
            SET_WEEKEVENTREG(WEEKEVENTREG_93_08);
        } else {
            CLEAR_WEEKEVENTREG(WEEKEVENTREG_53_20);
            CLEAR_WEEKEVENTREG(WEEKEVENTREG_93_08);
        }
    }

    // Stone Tower Inverted
    if (UIWidgets::Checkbox(
            "Stone Tower Inverted", &inverted,
            { { .disabled = !inStoneTower, .disabledTooltip = "Can only invert while in Stone Tower" } })) {
        if (inverted) {
            Flags_SetSwitch(gPlayState, 20);
        } else {
            Flags_UnsetSwitch(gPlayState, 20);
        }
    }

    ImGui::Columns(1);
}

static void DrawOwlStatuesEditor() {
    struct OwlStatue {
        OwlWarpId id;
        const char* name;
    };
    static constexpr OwlStatue statues[] = {
        { OWL_WARP_CLOCK_TOWN, "Clock Town" },
        { OWL_WARP_MILK_ROAD, "Milk Road" },
        { OWL_WARP_SOUTHERN_SWAMP, "Southern Swamp" },
        { OWL_WARP_WOODFALL, "Woodfall" },
        { OWL_WARP_MOUNTAIN_VILLAGE, "Mountain Village" },
        { OWL_WARP_SNOWHEAD, "Snowhead" },
        { OWL_WARP_GREAT_BAY_COAST, "Great Bay Coast" },
        { OWL_WARP_ZORA_CAPE, "Zora Cape" },
        { OWL_WARP_IKANA_CANYON, "Ikana Canyon" },
        { OWL_WARP_STONE_TOWER, "Stone Tower" },
    };
    static_assert(OWL_WARP_GREAT_BAY_COAST == 0 && OWL_WARP_ENTRANCE == MmOwlEditor::kStatueCount);
    static_assert(ARRAY_COUNT(statues) == MmOwlEditor::kStatueCount);

    const bool hasSave = gSaveContext.fileNum >= 0 && gSaveContext.fileNum < FILE_NUM_MAX;
    if (!hasSave) {
        ImGui::TextWrapped("Load a Majora's Mask file to edit owl statues.");
    }
    ImGui::BeginDisabled(!hasSave);
    auto& flags = gSaveContext.save.saveInfo.playerData.owlActivationFlags;
    ImGui::Text("Activated: %u / %u", MmOwlEditor::ActivatedCount(flags), MmOwlEditor::kStatueCount);
    ImGui::TextWrapped("Choose unlocked warp destinations. Save in game to keep changes.");
    ImGui::TextWrapped("This does not grant Song of Soaring or an instrument, or change the seed's song routes.");

    const bool twoColumns = ImGui::GetContentRegionAvail().x >= ImGui::GetFontSize() * 22.0f;
    if (UIWidgets::Button("Activate all##owlStatues",
                          { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Green })) {
        flags = MmOwlEditor::SetAllActivated(flags, true);
    }
    if (twoColumns) {
        ImGui::SameLine();
    }
    if (UIWidgets::Button("Clear all##owlStatues",
                          { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Red })) {
        flags = MmOwlEditor::SetAllActivated(flags, false);
    }
    if (ImGui::BeginTable("owlStatueDestinations", twoColumns ? 2 : 1, ImGuiTableFlags_SizingStretchSame)) {
        for (const auto& statue : statues) {
            ImGui::TableNextColumn();
            bool activated = MmOwlEditor::IsActivated(flags, statue.id);
#ifdef COMBO_BUILD
            const auto colour = UIWidgets::Colors(CVarGetInteger("gSettings.Menu.Theme", 5));
#else
            const auto colour = UIWidgets::Colors::Gray;
#endif
            if (UIWidgets::Checkbox(statue.name, &activated, { .color = colour })) {
                flags = MmOwlEditor::SetActivated(flags, statue.id, activated);
            }
        }
        ImGui::EndTable();
    }
    ImGui::EndDisabled();
    // Only the loaded live Save is edited. The native pause/cycle save paths
    // persist it in the appropriate owl/newCycle page; no stale page, recorded
    // randomizer route, song ownership, or current owl respawn is rewritten here.
}

#ifdef COMBO_BUILD
// ComboShip: the General tab, mirrored by OoT's: Player, Health, Magic, Rupees and Time cards with the shared coloured
// bars and Max/Reset buttons (Reset is Red), then a collapsed Progress card with the temples and the owl statues, which
// used to open at the top. Neutral bars, buttons and checkboxes take the menu theme colour.
void DrawGeneralTab() {
    namespace Kit = ComboSaveEditorKit;
    const UIWidgets::Colors theme = UIWidgets::Colors(CVarGetInteger("gSettings.Menu.Theme", 5));
    const ImVec4 themeColour = UIWidgets::ColorValues.at(theme);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
    ImGui::BeginChild("generalTab", ImVec2(0, 0), true);
    Kit::BeginCardLayout();

    Kit::BeginCard("identityCard");
    ImGui::Text("Player Name");
    ImGui::PushStyleColor(ImGuiCol_FrameBg, themeColour);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 8.0f));
    static char playerNameBuf[9];
    ImGui::InputText("##playerNameInput", getPlayerName(playerNameBuf), 9,
                     ImGuiInputTextFlags_CallbackAlways | ImGuiInputTextFlags_AutoSelectAll, setPlayerName);
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(1);

    ImGui::Spacing();
    ImGui::Text("Bombers' Code: %d %d %d %d %d", gSaveContext.save.saveInfo.bomberCode[0],
                gSaveContext.save.saveInfo.bomberCode[1], gSaveContext.save.saveInfo.bomberCode[2],
                gSaveContext.save.saveInfo.bomberCode[3], gSaveContext.save.saveInfo.bomberCode[4]);

    ImGui::Spacing();
    UIWidgets::Checkbox("Has Tatl", (bool*)&gSaveContext.save.hasTatl, { .color = theme });
    UIWidgets::Checkbox("Is Owl Save", (bool*)&gSaveContext.save.isOwlSave, { .color = theme });
    UIWidgets::Checkbox("Finished Intro Sequence", (bool*)&gSaveContext.save.isFirstCycle, { .color = theme });
    Kit::EndCard();

    Kit::BeginCard("healthCard");
    if (Kit::Button("Max Health", Kit::kHealthMax)) {
        gSaveContext.save.saveInfo.playerData.doubleDefense = 1;
        gSaveContext.save.saveInfo.inventory.defenseHearts = 20;
        gSaveContext.save.saveInfo.playerData.healthCapacity = gSaveContext.save.saveInfo.playerData.health = 20 * 16;
        gSaveContext.save.saveInfo.inventory.questItems &= ~0xF0000000;
    }
    ImGui::SameLine();
    if (Kit::Button("Reset##resetHealthButton", Kit::kReset)) {
        gSaveContext.save.saveInfo.playerData.doubleDefense = 0;
        gSaveContext.save.saveInfo.inventory.defenseHearts = 0;
        gSaveContext.save.saveInfo.playerData.healthCapacity = gSaveContext.save.saveInfo.playerData.health = 3 * 16;
        gSaveContext.save.saveInfo.inventory.questItems &= ~0xF0000000;
    }
    ImGui::SameLine();
    if (UIWidgets::Checkbox("Double Defense", (bool*)&gSaveContext.save.saveInfo.playerData.doubleDefense,
                            { .color = UIWidgets::Colors::Red })) {
        if (gSaveContext.save.saveInfo.playerData.doubleDefense) {
            gSaveContext.save.saveInfo.inventory.defenseHearts = 20;
        } else {
            gSaveContext.save.saveInfo.inventory.defenseHearts = 0;
        }
    }
    int16_t heartCount = (int16_t)gSaveContext.save.saveInfo.playerData.healthCapacity / 16;
    if (Kit::ResourceBar("##heartCountSlider", &heartCount, HEART_COUNT_MIN, HEART_COUNT_MAX, "Max hearts: %d",
                         Kit::kHealth)) {
        gSaveContext.save.saveInfo.playerData.healthCapacity = heartCount * 16;
        gSaveContext.save.saveInfo.playerData.health =
            MIN(gSaveContext.save.saveInfo.playerData.health, gSaveContext.save.saveInfo.playerData.healthCapacity);
    }
    Kit::ResourceBar("##healthSlider", &gSaveContext.save.saveInfo.playerData.health, 0,
                     gSaveContext.save.saveInfo.playerData.healthCapacity, "Health: %d", Kit::kHealth);
    Kit::EndCard();

    Kit::BeginCard("magicCard");
    if (Kit::Button("Max Magic", Kit::kMagic)) {
        gSaveContext.magicCapacity = gSaveContext.save.saveInfo.playerData.magic = MAGIC_DOUBLE_METER;
        gSaveContext.save.saveInfo.playerData.magicLevel = 2;
        gSaveContext.save.saveInfo.playerData.isMagicAcquired = true;
        gSaveContext.save.saveInfo.playerData.isDoubleMagicAcquired = true;
        BUTTON_ITEM_EQUIP(PLAYER_FORM_DEKU, EQUIP_SLOT_B) = ITEM_DEKU_NUT;
    }
    ImGui::SameLine();
    if (Kit::Button("Reset##resetMagicButton", Kit::kReset)) {
        gSaveContext.magicCapacity = gSaveContext.save.saveInfo.playerData.magic = 0;
        gSaveContext.save.saveInfo.playerData.magicLevel = 0;
        gSaveContext.save.saveInfo.playerData.isMagicAcquired = false;
        gSaveContext.save.saveInfo.playerData.isDoubleMagicAcquired = false;
    }
    bool spinAttack = CHECK_WEEKEVENTREG(WEEKEVENTREG_RECEIVED_GREAT_SPIN_ATTACK);
    if (UIWidgets::Checkbox("Spin Lv2", &spinAttack, { .color = UIWidgets::Colors::DarkGreen })) {
        if (spinAttack) {
            SET_WEEKEVENTREG(WEEKEVENTREG_RECEIVED_GREAT_SPIN_ATTACK);
        } else {
            CLEAR_WEEKEVENTREG(WEEKEVENTREG_RECEIVED_GREAT_SPIN_ATTACK);
        }
    }
    ImGui::SameLine();
    bool drankChateau = CHECK_WEEKEVENTREG(WEEKEVENTREG_DRANK_CHATEAU_ROMANI);
    if (UIWidgets::Checkbox("Drank Chateau", &drankChateau, { .color = UIWidgets::Colors::DarkGreen })) {
        if (drankChateau) {
            SET_WEEKEVENTREG(WEEKEVENTREG_DRANK_CHATEAU_ROMANI);
        } else {
            CLEAR_WEEKEVENTREG(WEEKEVENTREG_DRANK_CHATEAU_ROMANI);
        }
    }
    const int magicLevel = CLAMP(gSaveContext.save.saveInfo.playerData.magicLevel, 0, MAGIC_LEVEL_MAX);
    if (Kit::ResourceBar("##magicLevelSlider", &gSaveContext.save.saveInfo.playerData.magicLevel, 0, MAGIC_LEVEL_MAX,
                         Kit::kMagicLevelNames[magicLevel], Kit::kMagic)) {
        gSaveContext.magicCapacity = gSaveContext.save.saveInfo.playerData.magicLevel * MAGIC_NORMAL_METER;
        gSaveContext.save.saveInfo.playerData.magic =
            MIN(gSaveContext.save.saveInfo.playerData.magic, gSaveContext.magicCapacity);
        switch (gSaveContext.save.saveInfo.playerData.magicLevel) {
            case 0:
                gSaveContext.save.saveInfo.playerData.isMagicAcquired = false;
                gSaveContext.save.saveInfo.playerData.isDoubleMagicAcquired = false;
                break;
            case 1:
                gSaveContext.save.saveInfo.playerData.isMagicAcquired = true;
                gSaveContext.save.saveInfo.playerData.isDoubleMagicAcquired = false;
                BUTTON_ITEM_EQUIP(PLAYER_FORM_DEKU, EQUIP_SLOT_B) = ITEM_DEKU_NUT;
                break;
            case 2:
                gSaveContext.save.saveInfo.playerData.isMagicAcquired = true;
                gSaveContext.save.saveInfo.playerData.isDoubleMagicAcquired = true;
                BUTTON_ITEM_EQUIP(PLAYER_FORM_DEKU, EQUIP_SLOT_B) = ITEM_DEKU_NUT;
                break;
        }
    }
    Kit::ResourceBar("##magicSlider", &gSaveContext.save.saveInfo.playerData.magic, 0, gSaveContext.magicCapacity,
                     "Magic: %d", Kit::kMagic);
    Kit::EndCard();

    Kit::BeginCard("currencyCard");
    u8 walletLevelMax = (IS_RANDO && RANDO_SAVE_OPTIONS[RO_SHUFFLE_TYCOON_WALLET] == RO_GENERIC_YES)
                            ? WALLET_LEVEL_MAX + 1
                            : WALLET_LEVEL_MAX;
    if (Kit::Button("Max Rupees", Kit::kRupees)) {
        Inventory_ChangeUpgrade(UPG_WALLET, walletLevelMax);
        gSaveContext.save.saveInfo.playerData.rupees = CUR_CAPACITY(UPG_WALLET);
    }
    ImGui::SameLine();
    if (Kit::Button("Reset##resetRupeesButton", Kit::kReset)) {
        gSaveContext.save.saveInfo.playerData.rupees = 0;
        Inventory_ChangeUpgrade(UPG_WALLET, 0);
    }
    u8 currentWalletLevel = CUR_UPG_VALUE(UPG_WALLET);
    if (Kit::ResourceBar("##walletLevelSlider", &currentWalletLevel, 0, walletLevelMax,
                         currentWalletLevel < 4 ? WALLET_LEVEL_NAMES[currentWalletLevel] : "Glitched", Kit::kRupees)) {
        Inventory_ChangeUpgrade(UPG_WALLET, currentWalletLevel);
        gSaveContext.save.saveInfo.playerData.rupees =
            MIN(gSaveContext.save.saveInfo.playerData.rupees, CUR_CAPACITY(UPG_WALLET));
    }
    Kit::ResourceBar("##rupeesSlider", &gSaveContext.save.saveInfo.playerData.rupees, 0, CUR_CAPACITY(UPG_WALLET),
                     "Rupees: %d", Kit::kRupees);
    int bankedRupees = HS_GET_BANK_RUPEES();
    if (Kit::ResourceBar("##setBank", &bankedRupees, 0, 5000, "Banked Rupees: %d", Kit::kRupees)) {
        HS_SET_BANK_RUPEES(bankedRupees);
    }
    UIWidgets::Tooltip("To receive the rewards, set the bank to 199, 999, or 4,999 then deposit a single rupee");
    Kit::EndCard();

    Kit::BeginCard("timeCard");
    ImGui::Text("Time of Day");
    u16 gameTime = CURRENT_TIME;
    if (Kit::ResourceBar("##timeInput", &gameTime, 0, 0xFFFF,
                         Kit::ClockText(CURRENT_TIME, CVarGetInteger("gEnhancements.Graphics.24HoursClock", 0)).c_str(),
                         themeColour)) {
        UpdateGameTime(gameTime);
    }
    static const std::array<std::pair<int8_t, const char*>, 4> timeSkipAmounts = {
        { { -60, "-1hr" }, { -15, "-15m" }, { 15, "+15m" }, { 60, "+1hr" } }
    };
    float availWidth = ImGui::GetContentRegionAvail().x;
    float spacing = ImGui::GetStyle().ItemSpacing.x;
    float buttonWidth = (availWidth - (3 * spacing)) / 4;
    for (size_t i = 0; i < timeSkipAmounts.size(); i++) {
        const auto& skip = timeSkipAmounts.at(i);
        if (Kit::Button(skip.second, themeColour, ImVec2(buttonWidth, 0))) {
            UpdateGameTime(CURRENT_TIME + CLOCK_TIME(0, skip.first));
        }
        if (i < timeSkipAmounts.size() - 1) {
            ImGui::SameLine();
        }
    }
    if (Kit::ResourceBar("##dayInput", &gSaveContext.save.day, 0, 4, "Day: %d", themeColour)) {
        gSaveContext.save.eventDayCount = CURRENT_DAY;

        if (gPlayState != nullptr) {
            Interface_NewDay(gPlayState, CURRENT_DAY);
            // Inverting setup actors forces half-day actors to kill/respawn for new day
            gPlayState->numSetupActors = -gPlayState->numSetupActors;
            // Load environment values for new day
            Environment_NewDay(&gPlayState->envCtx);
            // Clear weather from day 2
            gWeatherMode = WEATHER_MODE_CLEAR;
            gPlayState->envCtx.lightningState = LIGHTNING_OFF;
        }
    }
    // Values are added to R_TIME_SPEED which is generally 3 in normal play state; -3 freezes time.
    std::string extraText = "";
    if (gSaveContext.save.timeSpeedOffset == 0) {
        extraText = " (default)";
    } else if (gSaveContext.save.timeSpeedOffset == -2) {
        extraText = " (inverted)";
    }
    std::string timeSpeedString = "Time Speed: " + std::to_string(gSaveContext.save.timeSpeedOffset + 3) + extraText;
    Kit::ResourceBar("##timeSpeedInput", &gSaveContext.save.timeSpeedOffset, -3, 7, timeSpeedString.c_str(),
                     themeColour);
    Kit::EndCard();

    // Collapsed, as OoT's Advanced card. The owl statues search lands on this tab.
    Kit::BeginCard("progressCard");
    if (ImGui::CollapsingHeader("Progress (temples, owl statues)")) {
        ImGui::SeparatorText("Temples");
        DrawTempleClears();
        ImGui::SeparatorText("Owl statues");
        DrawOwlStatuesEditor();
    }
    Kit::EndCard();

    Kit::EndCardLayout();

    ImGui::EndChild();
    ImGui::PopStyleVar(2);
}
#else
void DrawGeneralTab() {
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
    ImGui::BeginChild("generalTab", ImVec2(0, 0), true);

    if (ImGui::CollapsingHeader("Owl Statues / Warp Destinations", ImGuiTreeNodeFlags_DefaultOpen)) {
        DrawOwlStatuesEditor();
    }
    ImGui::Spacing();
    UIWidgets::BeginCardLayout({ .columnsPerRow = 2, .minColumnWidth = 420.0f });

    // Card 1: Player Identity
    UIWidgets::BeginCard("identityCard");
    ImGui::Text("Player Name");
    ImGui::PushStyleColor(ImGuiCol_FrameBg, UIWidgets::ColorValues.at(UIWidgets::Colors::Gray));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 8.0f));
    static char playerNameBuf[9];
    ImGui::InputText("##playerNameInput", getPlayerName(playerNameBuf), 9,
                     ImGuiInputTextFlags_CallbackAlways | ImGuiInputTextFlags_AutoSelectAll, setPlayerName);
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(1);

    ImGui::Spacing();
    ImGui::Text("Bombers Code: %d %d %d %d %d", gSaveContext.save.saveInfo.bomberCode[0],
                gSaveContext.save.saveInfo.bomberCode[1], gSaveContext.save.saveInfo.bomberCode[2],
                gSaveContext.save.saveInfo.bomberCode[3], gSaveContext.save.saveInfo.bomberCode[4]);

    ImGui::Spacing();
    UIWidgets::Checkbox("Has Tatl", (bool*)&gSaveContext.save.hasTatl, { .color = UIWidgets::Colors::Gray });
    UIWidgets::Checkbox("Is Owl Save", (bool*)&gSaveContext.save.isOwlSave, { .color = UIWidgets::Colors::Gray });
    UIWidgets::Checkbox("Finished Intro Sequence", (bool*)&gSaveContext.save.isFirstCycle,
                        { .color = UIWidgets::Colors::Gray });
    UIWidgets::EndCard();

    // Card 2: Time & Day
    UIWidgets::BeginCard("timeCard");
    ImGui::Text("Time of Day");
    UIWidgets::PushStyleSlider();
    static u16 minTime = 0;
    static u16 maxTime = 0xFFFF;
    // Get current time and format as digital string
    u16 curMinutes = (s32)TIME_TO_MINUTES_F(CURRENT_TIME) % 60;
    u16 curHours = (s32)TIME_TO_MINUTES_F(CURRENT_TIME) / 60;
    std::string minutes = (curMinutes < 10 ? "0" : "") + std::to_string(curMinutes);
    std::string hours = "";
    std::string ampm = "";
    // Handle 24 or 12 hour time
    if (CVarGetInteger("gEnhancements.Graphics.24HoursClock", 0)) {
        if (curHours < 10) {
            hours += "0";
        }
    } else {
        ampm = curHours >= 12 ? " pm" : " am";
        curHours = curHours % 12 ? curHours % 12 : 12;
    }
    hours += std::to_string(curHours);
    std::string timeString = hours + ":" + minutes + ampm + " (0x%x)";

    u16 gameTime = CURRENT_TIME;
    if (ImGui::SliderScalar("##timeInput", ImGuiDataType_U16, &gameTime, &minTime, &maxTime, timeString.c_str(),
                            ImGuiSliderFlags_NoInput)) {
        UpdateGameTime(gameTime);
    }
    UIWidgets::PopStyleSlider();

    ImGui::Spacing();
    static const std::array<std::pair<int8_t, const char*>, 4> timeSkipAmounts = {
        { { -60, "-1hr" }, { -15, "-15m" }, { 15, "+15m" }, { 60, "+1hr" } }
    };
    float availWidth = ImGui::GetContentRegionAvail().x;
    float spacing = ImGui::GetStyle().ItemSpacing.x;
    float buttonWidth = (availWidth - (3 * spacing)) / 4;
    for (size_t i = 0; i < timeSkipAmounts.size(); i++) {
        const auto& skip = timeSkipAmounts.at(i);
        if (UIWidgets::Button(skip.second, { .size = ImVec2(buttonWidth, 0), .color = UIWidgets::Colors::LightBlue })) {
            UpdateGameTime(CURRENT_TIME + CLOCK_TIME(0, skip.first));
        }
        if (i < timeSkipAmounts.size() - 1) {
            ImGui::SameLine();
        }
    }

    UIWidgets::PushStyleSlider();
    static s32 minDay = 0;
    static s32 maxDay = 4;
    if (ImGui::SliderScalar("##dayInput", ImGuiDataType_S32, &gSaveContext.save.day, &minDay, &maxDay, "Day: %d",
                            ImGuiSliderFlags_NoInput)) {
        gSaveContext.save.eventDayCount = CURRENT_DAY;

        if (gPlayState != nullptr) {
            Interface_NewDay(gPlayState, CURRENT_DAY);
            // Inverting setup actors forces half-day actors to kill/respawn for new day
            gPlayState->numSetupActors = -gPlayState->numSetupActors;
            // Load environment values for new day
            Environment_NewDay(&gPlayState->envCtx);
            // Clear weather from day 2
            gWeatherMode = WEATHER_MODE_CLEAR;
            gPlayState->envCtx.lightningState = LIGHTNING_OFF;
        }
    }

    // Time speed slider
    // Values are added to R_TIME_SPEED which is generally 3 in normal play state
    static s32 minSpeed = -3; // Time frozen
    static s32 maxSpeed = 7;
    std::string extraText = "";
    if (gSaveContext.save.timeSpeedOffset == 0) {
        extraText = " (default)";
    } else if (gSaveContext.save.timeSpeedOffset == -2) {
        extraText = " (inverted)";
    }
    std::string timeSpeedString = "Time Speed: " + std::to_string(gSaveContext.save.timeSpeedOffset + 3) + extraText;
    ImGui::SliderScalar("##timeSpeedInput", ImGuiDataType_S32, &gSaveContext.save.timeSpeedOffset, &minSpeed, &maxSpeed,
                        timeSpeedString.c_str(), ImGuiSliderFlags_NoInput);
    UIWidgets::PopStyleSlider();
    UIWidgets::EndCard();

    // Card 3: Currency
    UIWidgets::BeginCard("currencyCard");
    u8 walletLevelMax = (IS_RANDO && RANDO_SAVE_OPTIONS[RO_SHUFFLE_TYCOON_WALLET] == RO_GENERIC_YES)
                            ? WALLET_LEVEL_MAX + 1
                            : WALLET_LEVEL_MAX;
    if (UIWidgets::Button("Max Rupees", { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Green })) {
        Inventory_ChangeUpgrade(UPG_WALLET, walletLevelMax);
        gSaveContext.save.saveInfo.playerData.rupees = CUR_CAPACITY(UPG_WALLET);
    }
    ImGui::SameLine();
    if (UIWidgets::Button("Reset##resetRupeesButton",
                          { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Gray })) {
        gSaveContext.save.saveInfo.playerData.rupees = 0;
        Inventory_ChangeUpgrade(UPG_WALLET, 0);
    }
    UIWidgets::PushStyleSlider(UIWidgets::Colors::Green);
    u8 currentWalletLevel = CUR_UPG_VALUE(UPG_WALLET);
    if (ImGui::SliderScalar("##walletLevelSlider", ImGuiDataType_U8, &currentWalletLevel, &U8_ZERO, &walletLevelMax,
                            WALLET_LEVEL_NAMES[currentWalletLevel], ImGuiSliderFlags_NoInput)) {
        Inventory_ChangeUpgrade(UPG_WALLET, currentWalletLevel);
        gSaveContext.save.saveInfo.playerData.rupees =
            MIN(gSaveContext.save.saveInfo.playerData.rupees, CUR_CAPACITY(UPG_WALLET));
    }
    s16 walletCapacity = CUR_CAPACITY(UPG_WALLET);
    ImGui::SliderScalar("##rupeesSlider", ImGuiDataType_S16, &gSaveContext.save.saveInfo.playerData.rupees, &S16_ZERO,
                        &walletCapacity, "Rupees: %d", ImGuiSliderFlags_NoInput);

    int bankedRupees = HS_GET_BANK_RUPEES();
    if (ImGui::SliderInt("##setBank", &bankedRupees, 0, 5000, "Banked Rupees: %d", ImGuiSliderFlags_NoInput)) {
        HS_SET_BANK_RUPEES(bankedRupees);
    }
    UIWidgets::Tooltip("To receive the rewards, set the bank to 199, 999, or 4,999 then deposit a single rupee");
    UIWidgets::PopStyleSlider();
    UIWidgets::EndCard();

    // Card 4: Health
    UIWidgets::BeginCard("healthCard");
    if (UIWidgets::Button("Max Health", { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Red })) {
        gSaveContext.save.saveInfo.playerData.doubleDefense = 1;
        gSaveContext.save.saveInfo.inventory.defenseHearts = 20;
        gSaveContext.save.saveInfo.playerData.healthCapacity = gSaveContext.save.saveInfo.playerData.health = 20 * 16;
        gSaveContext.save.saveInfo.inventory.questItems &= ~0xF0000000;
    }
    ImGui::SameLine();
    if (UIWidgets::Button("Reset##resetHealthButton",
                          { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Gray })) {
        gSaveContext.save.saveInfo.playerData.doubleDefense = 0;
        gSaveContext.save.saveInfo.inventory.defenseHearts = 0;
        gSaveContext.save.saveInfo.playerData.healthCapacity = gSaveContext.save.saveInfo.playerData.health = 3 * 16;
        gSaveContext.save.saveInfo.inventory.questItems &= ~0xF0000000;
    }
    ImGui::SameLine();
    if (UIWidgets::Checkbox("DD", (bool*)&gSaveContext.save.saveInfo.playerData.doubleDefense,
                            { .color = UIWidgets::Colors::Red })) {
        if (gSaveContext.save.saveInfo.playerData.doubleDefense) {
            gSaveContext.save.saveInfo.inventory.defenseHearts = 20;
        } else {
            gSaveContext.save.saveInfo.inventory.defenseHearts = 0;
        }
    }
    UIWidgets::PushStyleSlider(UIWidgets::Colors::DarkRed);
    int16_t heartCount = (int16_t)gSaveContext.save.saveInfo.playerData.healthCapacity / 16;
    if (ImGui::SliderScalar("##heartCountSlider", ImGuiDataType_U16, &heartCount, &HEART_COUNT_MIN, &HEART_COUNT_MAX,
                            "Max Hearts: %d", ImGuiSliderFlags_NoInput)) {
        gSaveContext.save.saveInfo.playerData.healthCapacity = heartCount * 16;
        gSaveContext.save.saveInfo.playerData.health =
            MIN(gSaveContext.save.saveInfo.playerData.health, gSaveContext.save.saveInfo.playerData.healthCapacity);
    }
    ImGui::SliderScalar("##healthSlider", ImGuiDataType_S16, &gSaveContext.save.saveInfo.playerData.health, &S16_ZERO,
                        &gSaveContext.save.saveInfo.playerData.healthCapacity, "Health: %d", ImGuiSliderFlags_NoInput);
    UIWidgets::PopStyleSlider();
    UIWidgets::EndCard();

    // Card 5: Magic
    UIWidgets::BeginCard("magicCard");
    if (UIWidgets::Button("Max Magic", { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::DarkGreen })) {
        gSaveContext.magicCapacity = gSaveContext.save.saveInfo.playerData.magic = MAGIC_DOUBLE_METER;
        gSaveContext.save.saveInfo.playerData.magicLevel = 2;
        gSaveContext.save.saveInfo.playerData.isMagicAcquired = true;
        gSaveContext.save.saveInfo.playerData.isDoubleMagicAcquired = true;
        BUTTON_ITEM_EQUIP(PLAYER_FORM_DEKU, EQUIP_SLOT_B) = ITEM_DEKU_NUT;
    }
    ImGui::SameLine();
    if (UIWidgets::Button("Reset##resetMagicButton",
                          { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Gray })) {
        gSaveContext.magicCapacity = gSaveContext.save.saveInfo.playerData.magic = 0;
        gSaveContext.save.saveInfo.playerData.magicLevel = 0;
        gSaveContext.save.saveInfo.playerData.isMagicAcquired = false;
        gSaveContext.save.saveInfo.playerData.isDoubleMagicAcquired = false;
    }
    bool spinAttack = CHECK_WEEKEVENTREG(WEEKEVENTREG_RECEIVED_GREAT_SPIN_ATTACK);
    if (UIWidgets::Checkbox("Spin Lv2", &spinAttack, { .color = UIWidgets::Colors::DarkGreen })) {
        if (spinAttack) {
            SET_WEEKEVENTREG(WEEKEVENTREG_RECEIVED_GREAT_SPIN_ATTACK);
        } else {
            CLEAR_WEEKEVENTREG(WEEKEVENTREG_RECEIVED_GREAT_SPIN_ATTACK);
        }
    }
    ImGui::SameLine();
    bool drankChateau = CHECK_WEEKEVENTREG(WEEKEVENTREG_DRANK_CHATEAU_ROMANI);
    if (UIWidgets::Checkbox("Drank Chateau", &drankChateau, { .color = UIWidgets::Colors::DarkGreen })) {
        if (drankChateau) {
            SET_WEEKEVENTREG(WEEKEVENTREG_DRANK_CHATEAU_ROMANI);
        } else {
            CLEAR_WEEKEVENTREG(WEEKEVENTREG_DRANK_CHATEAU_ROMANI);
        }
    }
    UIWidgets::PushStyleSlider(UIWidgets::Colors::DarkGreen);
    if (ImGui::SliderScalar("##magicLevelSlider", ImGuiDataType_S8, &gSaveContext.save.saveInfo.playerData.magicLevel,
                            &S8_ZERO, &MAGIC_LEVEL_MAX,
                            MAGIC_LEVEL_NAMES[gSaveContext.save.saveInfo.playerData.magicLevel],
                            ImGuiSliderFlags_NoInput)) {
        gSaveContext.magicCapacity = gSaveContext.save.saveInfo.playerData.magicLevel * MAGIC_NORMAL_METER;
        gSaveContext.save.saveInfo.playerData.magic =
            MIN(gSaveContext.save.saveInfo.playerData.magic, gSaveContext.magicCapacity);
        switch (gSaveContext.save.saveInfo.playerData.magicLevel) {
            case 0:
                gSaveContext.save.saveInfo.playerData.isMagicAcquired = false;
                gSaveContext.save.saveInfo.playerData.isDoubleMagicAcquired = false;
                break;
            case 1:
                gSaveContext.save.saveInfo.playerData.isMagicAcquired = true;
                gSaveContext.save.saveInfo.playerData.isDoubleMagicAcquired = false;
                BUTTON_ITEM_EQUIP(PLAYER_FORM_DEKU, EQUIP_SLOT_B) = ITEM_DEKU_NUT;
                break;
            case 2:
                gSaveContext.save.saveInfo.playerData.isMagicAcquired = true;
                gSaveContext.save.saveInfo.playerData.isDoubleMagicAcquired = true;
                BUTTON_ITEM_EQUIP(PLAYER_FORM_DEKU, EQUIP_SLOT_B) = ITEM_DEKU_NUT;
                break;
        }
    }
    ImGui::SliderScalar("##magicSlider", ImGuiDataType_S8, &gSaveContext.save.saveInfo.playerData.magic, &S8_ZERO,
                        &gSaveContext.magicCapacity, "Magic: %d", ImGuiSliderFlags_NoInput);
    UIWidgets::PopStyleSlider();
    UIWidgets::EndCard();

    // Card 6: Temple Progress
    UIWidgets::BeginCard("progressCard");
    DrawTempleClears();
    UIWidgets::EndCard();

    UIWidgets::EndCardLayout();

    ImGui::EndChild();
    ImGui::PopStyleVar(2);
}
#endif

template <typename C, typename T> bool contains(C&& c, T e) {
    return std::find(std::begin(c), std::end(c), e) != std::end(c);
};

#ifdef COMBO_BUILD
void DrawAmmoInput(InventorySlot slot) {
    // ComboShip: the shared ammo count under its icon, in the menu theme colour.
    ItemId currentItemId = static_cast<ItemId>(gSaveContext.save.saveInfo.inventory.items[slot]);
    ImGui::SetCursorPos(ComboSaveEditorKit::GridAmmo(slot % 24));
    ComboSaveEditorKit::AmmoCount(
        "##ammoInput", &AMMO(currentItemId),
        UIWidgets::ColorValues.at(UIWidgets::Colors(CVarGetInteger("gSettings.Menu.Theme", 5))));
}
#else
void DrawAmmoInput(InventorySlot slot) {
    int x = slot % 6;
    int y = ((int)floor(slot / 6) % 4);
    ItemId currentItemId = static_cast<ItemId>(gSaveContext.save.saveInfo.inventory.items[slot]);

    const float ammoXOffset = (INV_GRID_ICON_SIZE - INV_GRID_AMMO_WIDTH) * 0.5f;
    const float ammoYOffset = INV_GRID_ICON_SIZE + 6.0f;
    ImGui::SetCursorPos(ImVec2(x * INV_GRID_WIDTH + INV_GRID_PADDING + ammoXOffset,
                               y * INV_GRID_HEIGHT + INV_GRID_TOP_MARGIN + INV_GRID_PADDING + ammoYOffset));
    ImGui::PushItemWidth(INV_GRID_AMMO_WIDTH);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, UIWidgets::ColorValues.at(UIWidgets::Colors::Gray));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(INV_GRID_AMMO_PADDING, INV_GRID_AMMO_PADDING));
    if (ComboTouch::Count("##ammoInput", &AMMO(currentItemId), 0, 99)) {
        // Crashes when < 0 and graphical issues when > 99
        AMMO(currentItemId) = MAX(0, MIN(AMMO(currentItemId), 99));
    }
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(1);
    ImGui::PopItemWidth();
}
#endif

void DrawEquipItemMenu(InventorySlot slot) {
    ItemId currentItemId = static_cast<ItemId>(gSaveContext.save.saveInfo.inventory.items[slot]);

    if (ImGui::BeginMenu("Equip")) {
        if (ImGui::MenuItem("C-Left")) {
            SET_CUR_FORM_BTN_ITEM(EQUIP_SLOT_C_LEFT, currentItemId);
            SET_CUR_FORM_BTN_SLOT(EQUIP_SLOT_C_LEFT, slot);
            Interface_LoadItemIconImpl(gPlayState, EQUIP_SLOT_C_LEFT);
        }
        if (ImGui::MenuItem("C-Down")) {
            SET_CUR_FORM_BTN_ITEM(EQUIP_SLOT_C_DOWN, currentItemId);
            SET_CUR_FORM_BTN_SLOT(EQUIP_SLOT_C_DOWN, slot);
            Interface_LoadItemIconImpl(gPlayState, EQUIP_SLOT_C_DOWN);
        }
        if (ImGui::MenuItem("C-Right")) {
            SET_CUR_FORM_BTN_ITEM(EQUIP_SLOT_C_RIGHT, currentItemId);
            SET_CUR_FORM_BTN_SLOT(EQUIP_SLOT_C_RIGHT, slot);
            Interface_LoadItemIconImpl(gPlayState, EQUIP_SLOT_C_RIGHT);
        }
        if (ImGui::MenuItem("D-Right")) {
            DPAD_SET_CUR_FORM_BTN_ITEM(EQUIP_SLOT_D_RIGHT, currentItemId);
            DPAD_SET_CUR_FORM_BTN_SLOT(EQUIP_SLOT_D_RIGHT, slot);
            Interface_Dpad_LoadItemIconImpl(gPlayState, EQUIP_SLOT_D_RIGHT);
        }
        if (ImGui::MenuItem("D-Left")) {
            DPAD_SET_CUR_FORM_BTN_ITEM(EQUIP_SLOT_D_LEFT, currentItemId);
            DPAD_SET_CUR_FORM_BTN_SLOT(EQUIP_SLOT_D_LEFT, slot);
            Interface_Dpad_LoadItemIconImpl(gPlayState, EQUIP_SLOT_D_LEFT);
        }
        if (ImGui::MenuItem("D-Down")) {
            DPAD_SET_CUR_FORM_BTN_ITEM(EQUIP_SLOT_D_DOWN, currentItemId);
            DPAD_SET_CUR_FORM_BTN_SLOT(EQUIP_SLOT_D_DOWN, slot);
            Interface_Dpad_LoadItemIconImpl(gPlayState, EQUIP_SLOT_D_DOWN);
        }
        if (ImGui::MenuItem("D-Up")) {
            DPAD_SET_CUR_FORM_BTN_ITEM(EQUIP_SLOT_D_UP, currentItemId);
            DPAD_SET_CUR_FORM_BTN_SLOT(EQUIP_SLOT_D_UP, slot);
            Interface_Dpad_LoadItemIconImpl(gPlayState, EQUIP_SLOT_D_UP);
        }
        ImGui::EndMenu();
    }
}

// The native trade items and the flag a rando pickup of each sets (Rando::GiveItem).
static const std::pair<ItemId, RandoInf> kSlotTradeItems[] = {
    { ITEM_MOONS_TEAR, RANDO_INF_OBTAINED_MOONS_TEAR },
    { ITEM_DEED_LAND, RANDO_INF_OBTAINED_DEED_LAND },
    { ITEM_DEED_SWAMP, RANDO_INF_OBTAINED_DEED_SWAMP },
    { ITEM_DEED_MOUNTAIN, RANDO_INF_OBTAINED_DEED_MOUNTAIN },
    { ITEM_DEED_OCEAN, RANDO_INF_OBTAINED_DEED_OCEAN },
    { ITEM_ROOM_KEY, RANDO_INF_OBTAINED_ROOM_KEY },
    { ITEM_LETTER_MAMA, RANDO_INF_OBTAINED_LETTER_TO_MAMA },
    { ITEM_LETTER_TO_KAFEI, RANDO_INF_OBTAINED_LETTER_TO_KAFEI },
    { ITEM_PENDANT_OF_MEMORIES, RANDO_INF_OBTAINED_PENDANT_OF_MEMORIES },
};

// Records the ownership a real pickup records (Rando::GiveItem) for an item put in its own slot. A rando save keeps
// what a shared trade slot holds outside the slot, and the 2S2H pause cycle offers only that, so this lets one slot
// hold both title deeds. Returns whether the save can own the item: only rando saves have the MM spells and gear, and
// only when the seed has the item's option. Rando::IsItemObtainable checks that option for Din's Fire, Nayru's Love
// and the gear. It has no case for Farore's Wind, which casts only in a seed with Farore's Wind (MM)
// (Combo_MMFaroreEnabled), so that option is checked here.
static bool GrantSlotItemOwnership(ItemId item) {
    for (const auto& [tradeItem, flag] : kSlotTradeItems) {
        if (item == tradeItem) {
            if (IS_RANDO) {
                Flags_SetRandoInf(flag);
            }
            return true;
        }
    }
#ifdef COMBO_BUILD
    static const std::pair<ItemId, RandoItemId> kComboItems[] = {
        { ITEM_COMBO_FARORES_WIND, RI_FARORES_WIND }, { ITEM_COMBO_NAYRUS_LOVE, RI_NAYRUS_LOVE },
        { ITEM_COMBO_DINS_FIRE, RI_DINS_FIRE },       { ITEM_COMBO_BOOTS_IRON, RI_BOOTS_IRON },
        { ITEM_COMBO_BOOTS_HOVER, RI_BOOTS_HOVER },   { ITEM_COMBO_TUNIC_GORON, RI_TUNIC_GORON },
        { ITEM_COMBO_TUNIC_ZORA, RI_TUNIC_ZORA },
    };
    for (const auto& [comboItem, randoItem] : kComboItems) {
        if (item != comboItem) {
            continue;
        }
        if (!IS_RANDO || !Rando::IsItemObtainable(randoItem) ||
            (item == ITEM_COMBO_FARORES_WIND && !RANDO_SAVE_OPTIONS[RO_MM_SPELL_WIND])) {
            return false;
        }
        if (item == ITEM_COMBO_FARORES_WIND) {
            Combo_MMFaroreGrantItem();
        } else if (item == ITEM_COMBO_NAYRUS_LOVE || item == ITEM_COMBO_DINS_FIRE) {
            Combo_MMSpellGrantItem(item == ITEM_COMBO_NAYRUS_LOVE ? 1 : 2);
        } else {
            MM_GrantAdultGearItem(gSaveContext.fileNum, item);
        }
        return true;
    }
#endif
    return true;
}

// Give All: every item of the slot, recorded as real pickups record them, showing the last one the save can own.
static ItemId GiveAllSlotItem(InventorySlot slot) {
    if (slot == SLOT_OCARINA) {
        // Pickups of both instruments leave the Ocarina of Time in the slot (Rando::Ocarinas::Grant).
        u8 ownedBits = gSaveContext.save.shipSaveInfo.rando.ocarinaOwnedBits;
        u8 shown = ITEM_NONE;
        Rando::Ocarinas::Grant(ownedBits, shown, 1);
        Rando::Ocarinas::Grant(ownedBits, shown, 2);
        if (IS_RANDO) {
            gSaveContext.save.shipSaveInfo.rando.ocarinaOwnedBits = ownedBits;
        }
        return static_cast<ItemId>(shown);
    }
    ItemId shown = safeItemsForInventorySlot[slot].back();
    for (ItemId item : safeItemsForInventorySlot[slot]) {
        if (GrantSlotItemOwnership(item)) {
            shown = item;
        }
    }
    return shown;
}

#ifdef COMBO_BUILD
// Takes off the MM boots and tunic the way using the worn one again does (ComboAdultGearUseNative.h).
static void TakeOffComboGear() {
    auto& adultMm = gSaveContext.save.shipSaveInfo.adultMm;
    const bool boots = adultMm.boots != 0;
    adultMm.boots = 0;
    adultMm.tunic = 0;
    if (boots && gPlayState) {
        Player* player = GET_PLAYER(gPlayState);
        if (player != nullptr && player->transformation == PLAYER_FORM_HUMAN) {
            player->currentBoots = PLAYER_BOOTS_HYLIAN;
            func_80123140(gPlayState, player);
        }
    }
}
#endif

// Reset, rando saves: clears the ownership Give All and the picker record, which the 2S2H pause cycle would otherwise
// put back into the emptied trade and ocarina slots. The removed boots and tunic come off Link.
static void ClearSlotItemOwnership() {
    if (!IS_RANDO) {
        return;
    }
    for (const auto& [tradeItem, flag] : kSlotTradeItems) {
        Flags_ClearRandoInf(flag);
    }
    auto& ship = gSaveContext.save.shipSaveInfo;
    ship.rando.ocarinaOwnedBits = 0;
#ifdef COMBO_BUILD
    ship.spellFireOwned = 0;
    ship.spellLoveOwned = 0;
    ship.faroresWind.owned = 0;
    ship.adultMm.gearTrade2Bits = 0;
    ship.adultMm.gearTrade3Bits = 0;
    TakeOffComboGear();
#endif
}

#ifdef COMBO_BUILD
// ComboShip: every way the editor adds one item to a slot (the toggle and the picker) goes through here, so each
// records what a real pickup records: GrantSlotItemOwnership for an item put in its own slot. With "Restrict to valid
// items" on, an item the save cannot own (an MM spell or gear piece the seed does not have) is refused, as Give all
// already leaves it out. Returns whether the slot changed.
static bool AddSlotItem(InventorySlot slot, ItemId item) {
    const u8 previous = gSaveContext.save.saveInfo.inventory.items[slot];
    gSaveContext.save.saveInfo.inventory.items[slot] = item;
    if (gItemSlots[item] == slot && !GrantSlotItemOwnership(item) && safeMode) {
        gSaveContext.save.saveInfo.inventory.items[slot] = previous;
        return false;
    }
    return true;
}

// ComboShip: the tap rule's toggle, for a slot that can hold one item. On adds it through AddSlotItem, and the Bow,
// Bombs and Bombchus also get capacity level 1 and full ammo when they have no capacity yet. Off removes it as the
// picker's None does, leaving the quiver and the bomb bag (they are on the Equipment tab).
static void ToggleSlotItem(InventorySlot slot) {
    const ItemId current = static_cast<ItemId>(gSaveContext.save.saveInfo.inventory.items[slot]);
    if (current != ITEM_NONE) {
        Inventory_DeleteItem(current, slot);
        return;
    }
    const ItemId item = safeItemsForInventorySlot[slot][0];
    if (!AddSlotItem(slot, item)) {
        return;
    }
    const s16 upgrade = item == ITEM_BOW ? UPG_QUIVER : (item == ITEM_BOMB || item == ITEM_BOMBCHU) ? UPG_BOMB_BAG : -1;
    if (upgrade >= 0 && CUR_UPG_VALUE(upgrade) == 0) {
        Inventory_ChangeUpgrade(upgrade, 1);
        AMMO(item) = CUR_CAPACITY(upgrade);
    }
}

// ComboShip: the Inventory tab's Give all and Reset, the same as the buttons upstream draws beside the grid.
static void GiveAllSlotItems() {
    for (int32_t i = 0; i <= SLOT_MASK_FIERCE_DEITY; i++) {
        if (i >= SLOT_BOTTLE_1 && i <= SLOT_BOTTLE_6) {
            gSaveContext.save.saveInfo.inventory.items[i] = ITEM_BOTTLE;
        } else {
            gSaveContext.save.saveInfo.inventory.items[i] = GiveAllSlotItem(static_cast<InventorySlot>(i));
        }
    }
    // The grid lists no gear: gItemSlots maps it to the trade slots only in Player_Init. Grant it as pickups do.
    for (ItemId gear :
         { ITEM_COMBO_BOOTS_IRON, ITEM_COMBO_BOOTS_HOVER, ITEM_COMBO_TUNIC_GORON, ITEM_COMBO_TUNIC_ZORA }) {
        GrantSlotItemOwnership(gear);
    }
    Inventory_ChangeUpgrade(UPG_BOMB_BAG, 3);
    Inventory_ChangeUpgrade(UPG_QUIVER, 3);
    AMMO(ITEM_BOW) = CUR_CAPACITY(UPG_QUIVER);
    AMMO(ITEM_BOMB) = AMMO(ITEM_BOMBCHU) = CUR_CAPACITY(UPG_BOMB_BAG);
    AMMO(ITEM_DEKU_STICK) = CUR_CAPACITY(UPG_DEKU_STICKS);
    AMMO(ITEM_DEKU_NUT) = CUR_CAPACITY(UPG_DEKU_NUTS);
    AMMO(ITEM_MAGIC_BEANS) = 20;
    AMMO(ITEM_POWDER_KEG) = 1;
}

static void ResetSlotItems() {
    for (int32_t i = 0; i <= SLOT_MASK_FIERCE_DEITY; i++) {
        Inventory_DeleteItem(gSaveContext.save.saveInfo.inventory.items[i], i);
    }
    ClearSlotItemOwnership();
    AMMO(ITEM_BOW) = AMMO(ITEM_BOMB) = AMMO(ITEM_BOMBCHU) = AMMO(ITEM_DEKU_STICK) = AMMO(ITEM_DEKU_NUT) =
        AMMO(ITEM_MAGIC_BEANS) = AMMO(ITEM_POWDER_KEG) = 0;
    Inventory_ChangeUpgrade(UPG_BOMB_BAG, 0);
    Inventory_ChangeUpgrade(UPG_QUIVER, 0);
}

// ComboShip: an item's name for the picker, from the randomizer's item table.
static const char* SlotItemName(ItemId item) {
    static std::unordered_map<int, const char*> names;
    auto it = names.find(item);
    if (it == names.end()) {
        const auto found = Rando::StaticData::Items.find(Rando::StaticData::GetItemIdFromVanillaItemId(item));
        it = names.emplace(item, found != Rando::StaticData::Items.end() ? found->second.name : "").first;
    }
    return it->second;
}

// ComboShip: the randomizer's time-item and give-item controls, which upstream draws beside the inventory grid.
static void DrawRandoItemGives() {
    if (RANDO_SAVE_OPTIONS[RO_CLOCK_SHUFFLE]) {
        // Time Items Management Section
        ImGui::SeparatorText("Time items");

        // Individual time items in 3x2 grid with static positioning
        RandoItemId clockItems[] = { RI_TIME_DAY_1,   RI_TIME_DAY_2,   RI_TIME_DAY_3,
                                     RI_TIME_NIGHT_1, RI_TIME_NIGHT_2, RI_TIME_NIGHT_3 };

        const char* clockNames[] = { "Day 1", "Day 2", "Day 3", "Night 1", "Night 2", "Night 3" };

        // Use table for static positioning - 3 columns, 2 rows
        if (ImGui::BeginTable("ClockItemsTable", 3, ImGuiTableFlags_None)) {
            ImGui::TableSetupColumn("Day 1", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Day 2", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Day 3", ImGuiTableColumnFlags_WidthStretch);

            // First row - Day items
            ImGui::TableNextRow();
            for (int i = 0; i < 3; i++) {
                ImGui::TableNextColumn();
                RandoItemId clockItem = clockItems[i];
                int halfIndex = Rando::ClockItems::GetHalfDayIndexFromClockItem(clockItem);
                bool isOwned = Flags_GetRandoInf(static_cast<RandoInf>(RANDO_INF_OBTAINED_CLOCK_DAY_1 + halfIndex));

                std::string buttonText =
                    isOwned ? ("Remove " + std::string(clockNames[i])) : ("No Item##" + std::to_string(i));
                std::string tooltipText = "";
                if (!isOwned) {
                    tooltipText = "You don't own " + std::string(clockNames[i]);
                }
                UIWidgets::ButtonOptions buttonOpts;
                buttonOpts.disabled = !isOwned;
                buttonOpts.disabledTooltip = !isOwned ? tooltipText.c_str() : "";
                if (UIWidgets::Button(buttonText.c_str(), buttonOpts)) {
                    Rando::RemoveItem(clockItem);
                }
            }

            // Second row - Night items
            ImGui::TableNextRow();
            for (int i = 3; i < 6; i++) {
                ImGui::TableNextColumn();
                RandoItemId clockItem = clockItems[i];
                int halfIndex = Rando::ClockItems::GetHalfDayIndexFromClockItem(clockItem);
                bool isOwned = Flags_GetRandoInf(static_cast<RandoInf>(RANDO_INF_OBTAINED_CLOCK_DAY_1 + halfIndex));

                std::string buttonText =
                    isOwned ? ("Remove " + std::string(clockNames[i])) : ("No Item##" + std::to_string(i));
                std::string tooltipText = "";
                if (!isOwned) {
                    tooltipText = "You don't own " + std::string(clockNames[i]);
                }
                UIWidgets::ButtonOptions buttonOpts;
                buttonOpts.disabled = !isOwned;
                buttonOpts.disabledTooltip = !isOwned ? tooltipText.c_str() : "";
                if (UIWidgets::Button(buttonText.c_str(), buttonOpts)) {
                    Rando::RemoveItem(clockItem);
                }
            }

            ImGui::EndTable();
        }
    }

    // Queue Randomizer Item Gives section
    ImGui::Spacing();
    ImGui::SeparatorText("Give randomizer item");

    static ImGuiTextFilter riFilter;
    UIWidgets::PushStyleCombobox();
    riFilter.Draw("##filter", ImGui::GetContentRegionAvail().x);
    UIWidgets::PopStyleCombobox();
    if (!riFilter.IsActive()) {
        ImGui::SameLine(18.0f);
        ImGui::Text("Search");
    }
    std::string riFilterString(riFilter.InputBuf);

    for (auto& [randoItemId, randoStaticItem] : Rando::StaticData::Items) {
        if (!IsRandomizerItemGivePubliclyAvailable(randoItemId) &&
            !IsNativeAdultDiagnosticQueueAvailable(randoItemId)) {
            continue;
        }
        if (!riFilter.PassFilter(randoStaticItem.name)) {
            continue;
        }
        if (randoItemId == RI_TRIFORCE_PIECE_PREVIOUS) {
            continue;
        }

        std::string buttonLabel = "Give ";
        buttonLabel += randoStaticItem.name;
        if (UIWidgets::Button(buttonLabel.c_str())) {
            GameInteractor::Instance->events.emplace_back(GIEventGiveItem{
                .showGetItemCutscene = Rando::StaticData::ShouldShowGetItemCutscene(Rando::ConvertItem(randoItemId)),
                .param = (int16_t)randoItemId,
                .giveItem =
                    [](Actor* actor, PlayState* play) {
                        RandoItemId randoItemId = Rando::ConvertItem((RandoItemId)CUSTOM_ITEM_PARAM);
                        std::string prefix = "You found";
                        std::string message = Rando::StaticData::GetItemName(randoItemId);

                        CustomMessage::Entry entry = {
                            .textboxType = 2,
                            .icon = Rando::StaticData::GetIconForZMessage(randoItemId),
                            .msg = prefix + " " + message + "!",
                        };

                        if (CUSTOM_ITEM_FLAGS & CustomItem::GIVE_ITEM_CUTSCENE) {
                            CustomMessage::SetActiveCustomMessage(entry.msg, entry);
                        } else if (Rando::StaticData::ShouldShowGetItemCutscene(
                                       Rando::ConvertItem((RandoItemId)CUSTOM_ITEM_PARAM))) {
                            CustomMessage::StartTextbox(entry.msg + "\x1C\x02\x10", entry);
                        } else {
                            Notification::Emit({
                                .itemIcon = Rando::StaticData::GetIconTexturePath(randoItemId),
                                .message = prefix,
                                .suffix = message,
                            });
                        }
                        Rando::GiveItem(randoItemId);
                        CUSTOM_ITEM_PARAM = randoItemId;
                    },
                .drawItem =
                    [](Actor* actor, PlayState* play) {
                        RandoItemId randoItemId;

                        // If the item has been given, the CUSTOM_ITEM_PARAM is set to the RI, prior to that it's
                        // the RC
                        if (CUSTOM_ITEM_FLAGS & CustomItem::CALLED_ACTION) {
                            randoItemId = (RandoItemId)CUSTOM_ITEM_PARAM;
                        } else {
                            randoItemId = Rando::ConvertItem((RandoItemId)CUSTOM_ITEM_PARAM);
                        }

                        Matrix_Scale(30.0f, 30.0f, 30.0f, MTXMODE_APPLY);
                        Rando::DrawItem(randoItemId, RC_UNKNOWN, actor);
                    } });
        }
    }
}
#endif

void NextItemInSlot(InventorySlot slot) {
    ItemId currentItemId = static_cast<ItemId>(gSaveContext.save.saveInfo.inventory.items[slot]);
    size_t currentItemIndex =
        find(safeItemsForInventorySlot[slot].begin(), safeItemsForInventorySlot[slot].end(), currentItemId) -
        safeItemsForInventorySlot[slot].begin();

    if (currentItemId == ITEM_NONE) {
        gSaveContext.save.saveInfo.inventory.items[slot] = safeItemsForInventorySlot[slot][0];

        if (gSaveContext.save.saveInfo.inventory.items[slot] == ITEM_BOW) {
            Inventory_ChangeUpgrade(UPG_QUIVER, 1);
            AMMO(ITEM_BOW) = CUR_CAPACITY(UPG_QUIVER);
        } else if (gSaveContext.save.saveInfo.inventory.items[slot] == ITEM_BOMB ||
                   gSaveContext.save.saveInfo.inventory.items[slot] == ITEM_BOMBCHU) {
            if (CUR_UPG_VALUE(UPG_BOMB_BAG) == 0) {
                Inventory_ChangeUpgrade(UPG_BOMB_BAG, 1);
            }
            AMMO(gSaveContext.save.saveInfo.inventory.items[slot]) = CUR_CAPACITY(UPG_BOMB_BAG);
        }
    } else if (currentItemId == ITEM_BOW) {
        if (CUR_UPG_VALUE(UPG_QUIVER) < 3) {
            Inventory_ChangeUpgrade(UPG_QUIVER, CUR_UPG_VALUE(UPG_QUIVER) + 1);
            AMMO(ITEM_BOW) = CUR_CAPACITY(UPG_QUIVER);
        } else {
            Inventory_DeleteItem(gSaveContext.save.saveInfo.inventory.items[slot], slot);
            Inventory_ChangeUpgrade(UPG_QUIVER, 0);
            AMMO(ITEM_BOW) = 0;
        }
    } else if (currentItemId == ITEM_BOMB || currentItemId == ITEM_BOMBCHU) {
        if (CUR_UPG_VALUE(UPG_BOMB_BAG) < 3) {
            Inventory_ChangeUpgrade(UPG_BOMB_BAG, CUR_UPG_VALUE(UPG_BOMB_BAG) + 1);
            AMMO(ITEM_BOMB) = AMMO(ITEM_BOMBCHU) = CUR_CAPACITY(UPG_BOMB_BAG);
        } else {
            Inventory_DeleteItem(ITEM_BOMB, SLOT_BOMB);
            Inventory_DeleteItem(ITEM_BOMBCHU, SLOT_BOMBCHU);
            Inventory_ChangeUpgrade(UPG_BOMB_BAG, 0);
            AMMO(ITEM_BOMB) = AMMO(ITEM_BOMBCHU) = 0;
        }
    } else if (currentItemIndex < safeItemsForInventorySlot[slot].size() - 1) {
        Inventory_ReplaceItem(gPlayState, currentItemId, safeItemsForInventorySlot[slot][currentItemIndex + 1]);
    } else {
        Inventory_DeleteItem(gSaveContext.save.saveInfo.inventory.items[slot], slot);
    }
}

#ifdef COMBO_BUILD
// ComboShip: one inventory slot, by the tap rule both editors share: a slot that can hold one item toggles it, a slot
// that can hold several opens a picker with None first and item names. Both add through AddSlotItem.
void DrawSlot(InventorySlot slot) {
    namespace Kit = ComboSaveEditorKit;
    if (!IsInventoryItemPubliclyAvailable(static_cast<ItemId>(gSaveContext.save.saveInfo.inventory.items[slot]))) {
        return;
    }
    ItemId currentItemId = static_cast<ItemId>(gSaveContext.save.saveInfo.inventory.items[slot]);
    auto gui = std::dynamic_pointer_cast<Fast::Fast3dGui>(Ship::Context::GetRawInstance()->GetWindow()->GetGui());
    const Kit::Tap tap = Kit::ToggleOrPick(safeMode, safeItemsForInventorySlot[slot].size());

    ImGui::PushID(slot);

    if (currentItemId != ITEM_NONE &&
        currentItemId <= ITEM_BOW_LIGHT &&            // gItemSlots only has entries till 77 (ITEM_BOW_LIGHT)
        gItemSlots[currentItemId] <= SLOT_BOTTLE_6 && // There is only ammo data for the first page
        ((safeMode && gAmmoItems[gItemSlots[currentItemId]] != ITEM_NONE) || !safeMode)) {
        DrawAmmoInput(slot);
    }

    ImGui::SetCursorPos(Kit::GridCell(slot % 24));

    // isEquipped border
    if (GET_CUR_FORM_BTN_SLOT(EQUIP_SLOT_C_LEFT) == slot || GET_CUR_FORM_BTN_SLOT(EQUIP_SLOT_C_DOWN) == slot ||
        GET_CUR_FORM_BTN_SLOT(EQUIP_SLOT_C_RIGHT) == slot ||
        (CVarGetInteger("gEnhancements.Dpad.DpadEquips", 0) &&
         (DPAD_GET_CUR_FORM_BTN_SLOT(EQUIP_SLOT_D_RIGHT) == slot ||
          DPAD_GET_CUR_FORM_BTN_SLOT(EQUIP_SLOT_D_LEFT) == slot ||
          DPAD_GET_CUR_FORM_BTN_SLOT(EQUIP_SLOT_D_DOWN) == slot ||
          DPAD_GET_CUR_FORM_BTN_SLOT(EQUIP_SLOT_D_UP) == slot))) {
        ImGui::PushStyleColor(ImGuiCol_Border, UIWidgets::ColorValues.at(UIWidgets::Colors::White));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 1.0f, 1.0f, 0.0f));
    }
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
    const ItemId shownItemId = currentItemId != ITEM_NONE ? currentItemId : safeItemsForInventorySlot[slot][0];
    bool buttonPressed = Kit::IconButton("##inventorySlot", gui->GetTextureByName((const char*)gItemIcons[shownItemId]),
                                         currentItemId != ITEM_NONE);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    const std::string tooltip = std::string(SlotItemName(shownItemId)) + "\n" + Kit::TapHint(tap);
    Kit::Tooltip(tooltip.c_str());
    if (buttonPressed) {
        if (tap == Kit::Tap::Toggle) {
            ToggleSlotItem(slot);
        } else {
            selectedInventorySlot = slot;
            ImGui::OpenPopup("InventorySlotPopup");
        }
    }

    if (ImGui::BeginPopup("InventorySlotPopup")) {
        if (Kit::PickerEntry(0, "##invNonePicker", 0, "None")) {
            Inventory_DeleteItem(gSaveContext.save.saveInfo.inventory.items[selectedInventorySlot],
                                 selectedInventorySlot);
            ImGui::CloseCurrentPopup();
        }
        size_t availableItems = safeMode ? safeItemsForInventorySlot[selectedInventorySlot].size() : ITEM_FISHING_ROD;
        int entry = 1;
        for (int32_t pickerIndex = 0; pickerIndex < availableItems; pickerIndex++) {
            ItemId id = safeMode ? safeItemsForInventorySlot[selectedInventorySlot][pickerIndex]
                                 : static_cast<ItemId>(pickerIndex);
            if (!IsInventoryItemPubliclyAvailable(id)) {
                continue;
            }
            if (Kit::PickerEntry(entry++, (const char*)gItemIcons[id],
                                 gui->GetTextureByName((const char*)gItemIcons[id]), SlotItemName(id))) {
                AddSlotItem(selectedInventorySlot, id);
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }

    ImGui::PopID();

    if (currentItemId != ITEM_NONE && ImGui::BeginPopupContextItem()) {
        DrawEquipItemMenu(slot);
        ImGui::EndPopup();
    }
}
#else
void DrawSlot(InventorySlot slot) {
    if (!IsInventoryItemPubliclyAvailable(static_cast<ItemId>(gSaveContext.save.saveInfo.inventory.items[slot]))) {
        return;
    }
    int x = slot % 6;
    int y = ((int)floor(slot / 6) % 4);
    ItemId currentItemId = static_cast<ItemId>(gSaveContext.save.saveInfo.inventory.items[slot]);

    ImGui::PushID(slot);

    if (currentItemId != ITEM_NONE &&
        currentItemId <= ITEM_BOW_LIGHT &&            // gItemSlots only has entries till 77 (ITEM_BOW_LIGHT)
        gItemSlots[currentItemId] <= SLOT_BOTTLE_6 && // There is only ammo data for the first page
        ((safeMode && gAmmoItems[gItemSlots[currentItemId]] != ITEM_NONE) || !safeMode)) {
        DrawAmmoInput(slot);
    }

    ImGui::SetCursorPos(
        ImVec2(x * INV_GRID_WIDTH + INV_GRID_PADDING, y * INV_GRID_HEIGHT + INV_GRID_TOP_MARGIN + INV_GRID_PADDING));

    // isEquipped border
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 1.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.2f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 1.0f, 1.0f, 0.1f));
    if (GET_CUR_FORM_BTN_SLOT(EQUIP_SLOT_C_LEFT) == slot || GET_CUR_FORM_BTN_SLOT(EQUIP_SLOT_C_DOWN) == slot ||
        GET_CUR_FORM_BTN_SLOT(EQUIP_SLOT_C_RIGHT) == slot ||
        (CVarGetInteger("gEnhancements.Dpad.DpadEquips", 0) &&
         (DPAD_GET_CUR_FORM_BTN_SLOT(EQUIP_SLOT_D_RIGHT) == slot ||
          DPAD_GET_CUR_FORM_BTN_SLOT(EQUIP_SLOT_D_LEFT) == slot ||
          DPAD_GET_CUR_FORM_BTN_SLOT(EQUIP_SLOT_D_DOWN) == slot ||
          DPAD_GET_CUR_FORM_BTN_SLOT(EQUIP_SLOT_D_UP) == slot))) {
        ImGui::PushStyleColor(ImGuiCol_Border, UIWidgets::ColorValues.at(UIWidgets::Colors::White));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 1.0f, 1.0f, 0.0f));
    }
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);

    ImTextureID textureId =
        std::dynamic_pointer_cast<Fast::Fast3dGui>(Ship::Context::GetRawInstance()->GetWindow()->GetGui())
            ->GetTextureByName((const char*)gItemIcons[safeItemsForInventorySlot[slot][0]]);

    if (currentItemId != ITEM_NONE) {
        textureId = std::dynamic_pointer_cast<Fast::Fast3dGui>(Ship::Context::GetRawInstance()->GetWindow()->GetGui())
                        ->GetTextureByName((const char*)gItemIcons[currentItemId]);
    }

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    bool buttonPressed =
        ImGui::ImageButton((const char*)gItemIcons[safeItemsForInventorySlot[slot][0]], textureId,
                           ImVec2(INV_GRID_ICON_SIZE, INV_GRID_ICON_SIZE), ImVec2(0, 0), ImVec2(1, 1),
                           ImVec4(0, 0, 0, 0), ImVec4(1, 1, 1, currentItemId == ITEM_NONE ? 0.4f : 1.0f));
    ImGui::PopStyleVar();
    if (buttonPressed) {
        if (safeMode && safeItemsForInventorySlot[slot].size() < 2) {
            NextItemInSlot(slot);
        } else {
            selectedInventorySlot = slot;
            ImGui::OpenPopup("InventorySlotPopup");
        }
    }

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(4);

    if (ImGui::BeginPopup("InventorySlotPopup")) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 1.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.2f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 1.0f, 1.0f, 0.1f));
        if (ImGui::Button("##invNonePicker", ImVec2(INV_GRID_ICON_SIZE, INV_GRID_ICON_SIZE))) {
            Inventory_DeleteItem(gSaveContext.save.saveInfo.inventory.items[selectedInventorySlot],
                                 selectedInventorySlot);
            ImGui::CloseCurrentPopup();
        }
        UIWidgets::Tooltip("None");

        size_t availableItems = safeMode ? safeItemsForInventorySlot[selectedInventorySlot].size() : ITEM_FISHING_ROD;
        for (int32_t pickerIndex = 0; pickerIndex < availableItems; pickerIndex++) {
            if (((pickerIndex + 1) % INV_PICKER_COLUMNS) != 0) {
                ImGui::SameLine();
            }
            ItemId id = safeMode ? safeItemsForInventorySlot[selectedInventorySlot][pickerIndex]
                                 : static_cast<ItemId>(pickerIndex);
            if (!IsInventoryItemPubliclyAvailable(id)) {
                continue;
            }
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
            bool buttonPressed = ImGui::ImageButton(
                (const char*)gItemIcons[id],
                std::dynamic_pointer_cast<Fast::Fast3dGui>(Ship::Context::GetRawInstance()->GetWindow()->GetGui())
                    ->GetTextureByName((const char*)gItemIcons[id]),
                ImVec2(INV_GRID_ICON_SIZE, INV_GRID_ICON_SIZE));
            ImGui::PopStyleVar();
            if (buttonPressed) {
                gSaveContext.save.saveInfo.inventory.items[selectedInventorySlot] = id;
                if (gItemSlots[id] == selectedInventorySlot) {
                    GrantSlotItemOwnership(id);
                }
                ImGui::CloseCurrentPopup();
            }
            // TODO: Add names for items
            // UIWidgets::SetLastItemHoverText(SohUtils::GetItemName(id));
        }
        ImGui::PopStyleColor(3);
        ImGui::EndPopup();
    }

    ImGui::PopID();

    if (currentItemId != ITEM_NONE && ImGui::BeginPopupContextItem()) {
        DrawEquipItemMenu(slot);
        ImGui::EndPopup();
    }
}
#endif

#ifdef COMBO_BUILD
// ComboShip: the Inventory tab, mirrored by OoT's: Give all, Reset and Restrict to valid items (was "Safe Mode") on top
// instead of in a side panel under the grid, the shared help line, then the Items and Masks grids. The time-item and
// give-item controls are on the Randomizer tab.
void DrawItemsAndMasksTab() {
    namespace Kit = ComboSaveEditorKit;
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
    ImGui::BeginChild("itemsTab", ImVec2(0, 0), true);
    if (Kit::Button("Give all##items", Kit::kGiveAll)) {
        GiveAllSlotItems();
    }
    ImGui::SameLine();
    if (Kit::Button("Reset##items", Kit::kReset)) {
        ResetSlotItems();
    }
    ImGui::SameLine();
    UIWidgets::Checkbox(Kit::kRestrictLabel, &safeMode,
                        { .color = UIWidgets::Colors(CVarGetInteger("gSettings.Menu.Theme", 5)) });
    Kit::Tooltip(Kit::kRestrictTooltip);
    ImGui::TextWrapped("%s", Kit::kInventoryHelp);

    ImGui::BeginChild("itemsBox", Kit::GridBoxSize(), ImGuiChildFlags_Borders);
    ImGui::Text("Items");
    for (uint32_t i = SLOT_OCARINA; i <= SLOT_BOTTLE_6; i++) {
        DrawSlot(static_cast<InventorySlot>(i));
    }
    ImGui::EndChild();
    ImGui::BeginChild("masksBox", Kit::GridBoxSize(), ImGuiChildFlags_Borders);
    ImGui::Text("Masks");
    for (uint32_t i = SLOT_MASK_POSTMAN; i <= SLOT_MASK_FIERCE_DEITY; i++) {
        DrawSlot(static_cast<InventorySlot>(i));
    }
    ImGui::EndChild();

    ImGui::EndChild();
    ImGui::PopStyleVar(2);
}
#else
void DrawItemsAndMasksTab() {
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
    // Group left side in a child for a scroll bar
#if defined(__ANDROID__)
    const float gridPanelHeight = ImGui::GetContentRegionAvail().y * 0.75f;
    ImGui::BeginChild("leftSide##items", ImVec2(0, gridPanelHeight), ImGuiChildFlags_Border);
#else
    ImGui::BeginChild("leftSide##items", ImVec2(0, 0), ImGuiChildFlags_AutoResizeX);
#endif
    ImGui::BeginGroup();
    ImGui::BeginChild("itemsBox",
                      ImVec2(INV_GRID_WIDTH * 6 + INV_GRID_PADDING * 2,
                             INV_GRID_HEIGHT * 4 + INV_GRID_PADDING * 2 + INV_GRID_TOP_MARGIN),
                      ImGuiChildFlags_Border);
    ImGui::Text("Items");
    for (uint32_t i = SLOT_OCARINA; i <= SLOT_BOTTLE_6; i++) {
        InventorySlot slot = static_cast<InventorySlot>(i);

        DrawSlot(slot);
    }
    ImGui::EndChild();
    ImGui::BeginChild("masksBox",
                      ImVec2(INV_GRID_WIDTH * 6 + INV_GRID_PADDING * 2,
                             INV_GRID_HEIGHT * 4 + INV_GRID_PADDING * 2 + INV_GRID_TOP_MARGIN),
                      ImGuiChildFlags_Border);
    ImGui::Text("Masks");
    for (uint32_t i = SLOT_MASK_POSTMAN; i <= SLOT_MASK_FIERCE_DEITY; i++) {
        InventorySlot slot = static_cast<InventorySlot>(i);

        DrawSlot(slot);
    }
    ImGui::EndChild();
    ImGui::EndGroup();
    ImGui::EndChild();

#if defined(__ANDROID__)
    ImGui::Spacing();
#else
    ImGui::SameLine();
#endif
    ImGui::BeginChild("equipsBox", ImVec2(0, 0), true);
    if (UIWidgets::Button("Give All##items")) {
        for (int32_t i = 0; i <= SLOT_MASK_FIERCE_DEITY; i++) {
            if (i >= SLOT_BOTTLE_1 && i <= SLOT_BOTTLE_6) {
                gSaveContext.save.saveInfo.inventory.items[i] = ITEM_BOTTLE;
            } else {
                gSaveContext.save.saveInfo.inventory.items[i] = GiveAllSlotItem(static_cast<InventorySlot>(i));
            }
        }
#ifdef COMBO_BUILD
        // The grid lists no gear: gItemSlots maps it to the trade slots only in Player_Init. Grant it as pickups do.
        for (ItemId gear :
             { ITEM_COMBO_BOOTS_IRON, ITEM_COMBO_BOOTS_HOVER, ITEM_COMBO_TUNIC_GORON, ITEM_COMBO_TUNIC_ZORA }) {
            GrantSlotItemOwnership(gear);
        }
#endif
        Inventory_ChangeUpgrade(UPG_BOMB_BAG, 3);
        Inventory_ChangeUpgrade(UPG_QUIVER, 3);
        AMMO(ITEM_BOW) = CUR_CAPACITY(UPG_QUIVER);
        AMMO(ITEM_BOMB) = AMMO(ITEM_BOMBCHU) = CUR_CAPACITY(UPG_BOMB_BAG);
        AMMO(ITEM_DEKU_STICK) = CUR_CAPACITY(UPG_DEKU_STICKS);
        AMMO(ITEM_DEKU_NUT) = CUR_CAPACITY(UPG_DEKU_NUTS);
        AMMO(ITEM_MAGIC_BEANS) = 20;
        AMMO(ITEM_POWDER_KEG) = 1;
    }
    if (UIWidgets::Button("Reset##items")) {
        for (int32_t i = 0; i <= SLOT_MASK_FIERCE_DEITY; i++) {
            Inventory_DeleteItem(gSaveContext.save.saveInfo.inventory.items[i], i);
        }
        ClearSlotItemOwnership();
        AMMO(ITEM_BOW) = AMMO(ITEM_BOMB) = AMMO(ITEM_BOMBCHU) = AMMO(ITEM_DEKU_STICK) = AMMO(ITEM_DEKU_NUT) =
            AMMO(ITEM_MAGIC_BEANS) = AMMO(ITEM_POWDER_KEG) = 0;
        Inventory_ChangeUpgrade(UPG_BOMB_BAG, 0);
        Inventory_ChangeUpgrade(UPG_QUIVER, 0);
    }
    UIWidgets::Checkbox("Safe Mode", &safeMode);

    if (gSaveContext.save.shipSaveInfo.saveType == SAVETYPE_RANDO) {
        if (RANDO_SAVE_OPTIONS[RO_CLOCK_SHUFFLE]) {
            // Time Items Management Section
            ImGui::SeparatorText("Time Items");

            // Individual time items in 3x2 grid with static positioning
            RandoItemId clockItems[] = { RI_TIME_DAY_1,   RI_TIME_DAY_2,   RI_TIME_DAY_3,
                                         RI_TIME_NIGHT_1, RI_TIME_NIGHT_2, RI_TIME_NIGHT_3 };

            const char* clockNames[] = { "Day 1", "Day 2", "Day 3", "Night 1", "Night 2", "Night 3" };

            // Use table for static positioning - 3 columns, 2 rows
            if (ImGui::BeginTable("ClockItemsTable", 3, ImGuiTableFlags_None)) {
                ImGui::TableSetupColumn("Day 1", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Day 2", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Day 3", ImGuiTableColumnFlags_WidthStretch);

                // First row - Day items
                ImGui::TableNextRow();
                for (int i = 0; i < 3; i++) {
                    ImGui::TableNextColumn();
                    RandoItemId clockItem = clockItems[i];
                    int halfIndex = Rando::ClockItems::GetHalfDayIndexFromClockItem(clockItem);
                    bool isOwned = Flags_GetRandoInf(static_cast<RandoInf>(RANDO_INF_OBTAINED_CLOCK_DAY_1 + halfIndex));

                    std::string buttonText =
                        isOwned ? ("Remove " + std::string(clockNames[i])) : ("No Item##" + std::to_string(i));
                    std::string tooltipText = "";
                    if (!isOwned) {
                        tooltipText = "You don't own " + std::string(clockNames[i]);
                    }
                    UIWidgets::ButtonOptions buttonOpts;
                    buttonOpts.disabled = !isOwned;
                    buttonOpts.disabledTooltip = !isOwned ? tooltipText.c_str() : "";
                    if (UIWidgets::Button(buttonText.c_str(), buttonOpts)) {
                        Rando::RemoveItem(clockItem);
                    }
                }

                // Second row - Night items
                ImGui::TableNextRow();
                for (int i = 3; i < 6; i++) {
                    ImGui::TableNextColumn();
                    RandoItemId clockItem = clockItems[i];
                    int halfIndex = Rando::ClockItems::GetHalfDayIndexFromClockItem(clockItem);
                    bool isOwned = Flags_GetRandoInf(static_cast<RandoInf>(RANDO_INF_OBTAINED_CLOCK_DAY_1 + halfIndex));

                    std::string buttonText =
                        isOwned ? ("Remove " + std::string(clockNames[i])) : ("No Item##" + std::to_string(i));
                    std::string tooltipText = "";
                    if (!isOwned) {
                        tooltipText = "You don't own " + std::string(clockNames[i]);
                    }
                    UIWidgets::ButtonOptions buttonOpts;
                    buttonOpts.disabled = !isOwned;
                    buttonOpts.disabledTooltip = !isOwned ? tooltipText.c_str() : "";
                    if (UIWidgets::Button(buttonText.c_str(), buttonOpts)) {
                        Rando::RemoveItem(clockItem);
                    }
                }

                ImGui::EndTable();
            }
        }

        // Queue Randomizer Item Gives section
        ImGui::Spacing();
        ImGui::SeparatorText("Queue Randomizer Item Gives");

        static ImGuiTextFilter riFilter;
        UIWidgets::PushStyleCombobox();
        riFilter.Draw("##filter", ImGui::GetContentRegionAvail().x);
        UIWidgets::PopStyleCombobox();
        if (!riFilter.IsActive()) {
            ImGui::SameLine(18.0f);
            ImGui::Text("Search");
        }
        std::string riFilterString(riFilter.InputBuf);

        for (auto& [randoItemId, randoStaticItem] : Rando::StaticData::Items) {
            if (!IsRandomizerItemGivePubliclyAvailable(randoItemId) &&
                !IsNativeAdultDiagnosticQueueAvailable(randoItemId)) {
                continue;
            }
            if (!riFilter.PassFilter(randoStaticItem.name)) {
                continue;
            }
            if (randoItemId == RI_TRIFORCE_PIECE_PREVIOUS) {
                continue;
            }

            std::string buttonLabel = "Give ";
            buttonLabel += randoStaticItem.name;
            if (UIWidgets::Button(buttonLabel.c_str())) {
                GameInteractor::Instance->events.emplace_back(GIEventGiveItem{
                    .showGetItemCutscene =
                        Rando::StaticData::ShouldShowGetItemCutscene(Rando::ConvertItem(randoItemId)),
                    .param = (int16_t)randoItemId,
                    .giveItem =
                        [](Actor* actor, PlayState* play) {
                            RandoItemId randoItemId = Rando::ConvertItem((RandoItemId)CUSTOM_ITEM_PARAM);
                            std::string prefix = "You found";
                            std::string message = Rando::StaticData::GetItemName(randoItemId);

                            CustomMessage::Entry entry = {
                                .textboxType = 2,
                                .icon = Rando::StaticData::GetIconForZMessage(randoItemId),
                                .msg = prefix + " " + message + "!",
                            };

                            if (CUSTOM_ITEM_FLAGS & CustomItem::GIVE_ITEM_CUTSCENE) {
                                CustomMessage::SetActiveCustomMessage(entry.msg, entry);
                            } else if (Rando::StaticData::ShouldShowGetItemCutscene(
                                           Rando::ConvertItem((RandoItemId)CUSTOM_ITEM_PARAM))) {
                                CustomMessage::StartTextbox(entry.msg + "\x1C\x02\x10", entry);
                            } else {
                                Notification::Emit({
                                    .itemIcon = Rando::StaticData::GetIconTexturePath(randoItemId),
                                    .message = prefix,
                                    .suffix = message,
                                });
                            }
                            Rando::GiveItem(randoItemId);
                            CUSTOM_ITEM_PARAM = randoItemId;
                        },
                    .drawItem =
                        [](Actor* actor, PlayState* play) {
                            RandoItemId randoItemId;

                            // If the item has been given, the CUSTOM_ITEM_PARAM is set to the RI, prior to that it's
                            // the RC
                            if (CUSTOM_ITEM_FLAGS & CustomItem::CALLED_ACTION) {
                                randoItemId = (RandoItemId)CUSTOM_ITEM_PARAM;
                            } else {
                                randoItemId = Rando::ConvertItem((RandoItemId)CUSTOM_ITEM_PARAM);
                            }

                            Matrix_Scale(30.0f, 30.0f, 30.0f, MTXMODE_APPLY);
                            Rando::DrawItem(randoItemId, RC_UNKNOWN, actor);
                        } });
            }
        }
    }

    // Expose inputs to edit raw number values of equips
    // ImGui::Text("Equips");
    // ImGui::Text("C-Buttons");
    // ImGui::Text("D-Pad");
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
}
#endif

void SongInfo(int32_t itemID) {
    switch (itemID) {
        case QUEST_SONG_SONATA:
            colorTint = ImVec4(0.588f, 1.0f, 0.392f, CHECK_QUEST_ITEM(itemID) ? 1.0f : 0.4f);
            songTooltip = "Sonata of Awakening";
            break;
        case QUEST_SONG_LULLABY:
            colorTint = ImVec4(1.0f, 0.313f, 0.156f,
                               (CHECK_QUEST_ITEM(itemID) || CHECK_QUEST_ITEM(QUEST_SONG_LULLABY_INTRO)) ? 1.0f : 0.4f);
            songTooltip = (!CHECK_QUEST_ITEM(itemID) && CHECK_QUEST_ITEM(QUEST_SONG_LULLABY_INTRO))
                              ? "Goron Lullaby Intro"
                              : "Goron Lullaby";
            break;
        case QUEST_SONG_BOSSA_NOVA:
            colorTint = ImVec4(0.392f, 0.588f, 1.0f, CHECK_QUEST_ITEM(itemID) ? 1.0f : 0.4f);
            songTooltip = "New Wave Bossa Nova";
            break;
        case QUEST_SONG_ELEGY:
            colorTint = ImVec4(1.0f, 0.627f, 0.0f, CHECK_QUEST_ITEM(itemID) ? 1.0f : 0.4f);
            songTooltip = "Elegy of Emptiness";
            break;
        case QUEST_SONG_OATH:
            colorTint = ImVec4(1.0f, 0.392f, 1.0f, CHECK_QUEST_ITEM(itemID) ? 1.0f : 0.4f);
            songTooltip = "Oath to Order";
            break;
        case QUEST_SONG_TIME:
            colorTint = ImVec4(1, 1, 1, CHECK_QUEST_ITEM(itemID) ? 1.0f : 0.4f);
            songTooltip = "Song of Time";
            break;
        case QUEST_SONG_HEALING:
            colorTint = ImVec4(1, 1, 1, CHECK_QUEST_ITEM(itemID) ? 1.0f : 0.4f);
            songTooltip = "Song of Healing";
            break;
        case QUEST_SONG_EPONA:
            colorTint = ImVec4(1, 1, 1, CHECK_QUEST_ITEM(itemID) ? 1.0f : 0.4f);
#ifdef COMBO_BUILD
            songTooltip = "Epona's Song";
#else
            songTooltip = "Eponas Song";
#endif
            break;
        case QUEST_SONG_SOARING:
            colorTint = ImVec4(1, 1, 1, CHECK_QUEST_ITEM(itemID) ? 1.0f : 0.4f);
            songTooltip = "Song of Soaring";
            break;
        case QUEST_SONG_STORMS:
            colorTint = ImVec4(1, 1, 1, CHECK_QUEST_ITEM(itemID) ? 1.0f : 0.4f);
            songTooltip = "Song of Storms";
            break;
        case QUEST_SONG_SARIA:
            colorTint = ImVec4(1, 1, 1, CHECK_QUEST_ITEM(itemID) ? 1.0f : 0.4f);
            songTooltip = "Saria's Song (Unused?)";
            break;
        case QUEST_SONG_SUN:
            colorTint = ImVec4(1, 1, 1, CHECK_QUEST_ITEM(itemID) ? 1.0f : 0.4f);
            songTooltip = "Sun's Song (Not Obtainable)";
            break;
        default:
            colorTint = ImVec4(1, 1, 1, CHECK_QUEST_ITEM(itemID) ? 1.0f : 0.4f);
            songTooltip = " ";
            break;
    }
}

std::map<QuestItem, ItemId> questToItemMap = {
    { QUEST_REMAINS_ODOLWA, ITEM_REMAINS_ODOLWA }, { QUEST_REMAINS_GOHT, ITEM_REMAINS_GOHT },
    { QUEST_REMAINS_GYORG, ITEM_REMAINS_GYORG },   { QUEST_REMAINS_TWINMOLD, ITEM_REMAINS_TWINMOLD },
    { QUEST_SONG_SONATA, ITEM_SONG_SONATA },       { QUEST_SONG_LULLABY, ITEM_SONG_LULLABY },
    { QUEST_SONG_BOSSA_NOVA, ITEM_SONG_NOVA },     { QUEST_SONG_ELEGY, ITEM_SONG_ELEGY },
    { QUEST_SONG_OATH, ITEM_SONG_OATH },           { QUEST_SONG_SARIA, ITEM_SONG_SARIA },
    { QUEST_SONG_TIME, ITEM_SONG_TIME },           { QUEST_SONG_HEALING, ITEM_SONG_HEALING },
    { QUEST_SONG_EPONA, ITEM_SONG_EPONA },         { QUEST_SONG_SOARING, ITEM_SONG_SOARING },
    { QUEST_SONG_STORMS, ITEM_SONG_STORMS },       { QUEST_SONG_SUN, ITEM_SONG_SUN }
};

void NextQuestInSlot(QuestItem slot) {
    if (!gPlayState) {
        return;
    }
    Player* player = GET_PLAYER(gPlayState);
    if (slot == QUEST_SONG_LULLABY || slot == QUEST_SONG_LULLABY_INTRO) {
        if (CHECK_QUEST_ITEM(QUEST_SONG_LULLABY)) {
            REMOVE_QUEST_ITEM(QUEST_SONG_LULLABY);
            REMOVE_QUEST_ITEM(QUEST_SONG_LULLABY_INTRO);
        } else if (CHECK_QUEST_ITEM(QUEST_SONG_LULLABY_INTRO)) {
            SET_QUEST_ITEM(QUEST_SONG_LULLABY);
        } else {
            SET_QUEST_ITEM(QUEST_SONG_LULLABY_INTRO);
        }
    } else if (slot == QUEST_SWORD) {
        uint32_t currentSword = GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SWORD);
        if (GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SWORD) >= EQUIP_VALUE_SWORD_GILDED) {
            SET_EQUIP_VALUE(EQUIP_TYPE_SWORD, EQUIP_VALUE_SWORD_NONE);
        } else {
            SET_EQUIP_VALUE(EQUIP_TYPE_SWORD, currentSword + 1);
        }
        if (GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SWORD) == EQUIP_VALUE_SWORD_NONE) {
            BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_B) = ITEM_NONE;
        } else {
            BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_B) =
                ITEM_SWORD_KOKIRI + GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SWORD) - EQUIP_VALUE_SWORD_KOKIRI;
        }
        if (GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SWORD) == EQUIP_VALUE_SWORD_RAZOR) {
            gSaveContext.save.saveInfo.playerData.swordHealth = 100;
        }
        Interface_LoadItemIconImpl(gPlayState, EQUIP_SLOT_B);
    } else if (slot == QUEST_SHIELD) {
        uint32_t currentShield = GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SHIELD);
        if (GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SHIELD) == EQUIP_VALUE_SHIELD_NONE) {
            SET_EQUIP_VALUE(EQUIP_TYPE_SHIELD, EQUIP_VALUE_SHIELD_HERO);
        } else if (GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SHIELD) == EQUIP_VALUE_SHIELD_HERO) {
            SET_EQUIP_VALUE(EQUIP_TYPE_SHIELD, EQUIP_VALUE_SHIELD_MIRROR);
        } else {
            SET_EQUIP_VALUE(EQUIP_TYPE_SHIELD, EQUIP_VALUE_SHIELD_NONE);
        }
        Player_SetEquipmentData(gPlayState, player);
    } else {
        if (CHECK_QUEST_ITEM(slot)) {
            REMOVE_QUEST_ITEM(slot);
        } else {
            SET_QUEST_ITEM(slot);
        }
    }
}

void DrawQuestSlot(QuestItem slot) {
    int x = slot % 6;
    int y = ((int)floor(slot / 6) % 4);
    ImGui::PushID(slot);

    ImGui::SetCursorPos(
        ImVec2(x * INV_GRID_WIDTH + INV_GRID_PADDING, y * INV_GRID_HEIGHT + INV_GRID_TOP_MARGIN + INV_GRID_PADDING));

    ImTextureID textureId =
        std::dynamic_pointer_cast<Fast::Fast3dGui>(Ship::Context::GetRawInstance()->GetWindow()->GetGui())
            ->GetTextureByName((const char*)gItemIcons[questToItemMap[slot]]);
    if (ImGui::ImageButton(std::to_string(slot).c_str(), textureId, ImVec2(INV_GRID_ICON_SIZE, INV_GRID_ICON_SIZE),
                           ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0),
                           ImVec4(1, 1, 1, CHECK_QUEST_ITEM(slot) ? 1.0f : 0.4f))) {
        NextQuestInSlot(slot);
    }

    ImGui::PopID();
}

#ifdef COMBO_BUILD
// ComboShip: Goron Lullaby can be its intro or the whole song, so a tap opens a picker (None, Goron Lullaby Intro,
// Goron Lullaby) instead of cycling through them; every other song toggles.
ImVec2 DrawSong(QuestItem slot) {
    namespace Kit = ComboSaveEditorKit;
    SongInfo(slot);
    ImTextureID texture =
        std::dynamic_pointer_cast<Fast::Fast3dGui>(Ship::Context::GetRawInstance()->GetWindow()->GetGui())
            ->GetTextureByName((const char*)gItemIcons[questToItemMap[(QuestItem)slot]]);
    const bool pressed =
        ImGui::ImageButton(std::to_string(slot).c_str(), texture, ImVec2(Kit::kSongIconWidth, Kit::kIconSize),
                           ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), colorTint);
    ImVec2 itemSize = ImGui::GetItemRectSize();
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::Text("%s", songTooltip);
        ImGui::EndTooltip();
    }
    if (pressed) {
        if (slot == QUEST_SONG_LULLABY) {
            ImGui::OpenPopup("lullabyPicker");
        } else {
            NextQuestInSlot(slot);
        }
    }
    if (slot == QUEST_SONG_LULLABY && ImGui::BeginPopup("lullabyPicker")) {
        if (Kit::PickerEntry(0, "##lullabyNone", 0, "None")) {
            REMOVE_QUEST_ITEM(QUEST_SONG_LULLABY);
            REMOVE_QUEST_ITEM(QUEST_SONG_LULLABY_INTRO);
            ImGui::CloseCurrentPopup();
        }
        if (Kit::PickerEntry(1, "##lullabyIntro", texture, "Goron Lullaby Intro")) {
            REMOVE_QUEST_ITEM(QUEST_SONG_LULLABY);
            SET_QUEST_ITEM(QUEST_SONG_LULLABY_INTRO);
            ImGui::CloseCurrentPopup();
        }
        if (Kit::PickerEntry(2, "##lullabyFull", texture, "Goron Lullaby")) {
            SET_QUEST_ITEM(QUEST_SONG_LULLABY_INTRO);
            SET_QUEST_ITEM(QUEST_SONG_LULLABY);
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    return itemSize;
}
#else
ImVec2 DrawSong(QuestItem slot) {
    SongInfo(slot);
    if (ImGui::ImageButton(
            std::to_string(slot).c_str(),
            std::dynamic_pointer_cast<Fast::Fast3dGui>(Ship::Context::GetRawInstance()->GetWindow()->GetGui())
                ->GetTextureByName((const char*)gItemIcons[questToItemMap[(QuestItem)slot]]),
            ImVec2(INV_GRID_ICON_SIZE / 1.5f, INV_GRID_ICON_SIZE), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0),
            colorTint)) {
        NextQuestInSlot(slot);
    }
    ImVec2 itemSize = ImGui::GetItemRectSize();
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::Text("%s", songTooltip);
        ImGui::EndTooltip();
    }
    return itemSize;
}
#endif

#ifdef COMBO_BUILD
// ComboShip: Quest Status with OoT's headings in the same order: Boss rewards, Songs, Quest items, Gold Skulltula
// tokens and Heart pieces as bars. The sword and shield moved to Equipment, so Give all and Reset here leave them
// alone.
void DrawQuestStatusTab() {
    namespace Kit = ComboSaveEditorKit;
    const ImVec4 theme = UIWidgets::ColorValues.at(UIWidgets::Colors(CVarGetInteger("gSettings.Menu.Theme", 5)));
    auto gui = std::dynamic_pointer_cast<Fast::Fast3dGui>(Ship::Context::GetRawInstance()->GetWindow()->GetGui());
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 1.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
    ImGui::BeginChild("questTab", ImVec2(0, 0), true);

    if (Kit::Button("Give all##quest", Kit::kGiveAll)) {
        for (int32_t i = QUEST_REMAINS_ODOLWA; i <= QUEST_BOMBERS_NOTEBOOK; i++) {
            if (i != QUEST_SHIELD && i != QUEST_SWORD) {
                SET_QUEST_ITEM(i);
            }
        }
    }
    ImGui::SameLine();
    if (Kit::Button("Reset##quest", Kit::kReset)) {
        for (int32_t i = QUEST_REMAINS_ODOLWA; i <= QUEST_BOMBERS_NOTEBOOK; i++) {
            if (i != QUEST_SHIELD && i != QUEST_SWORD) {
                REMOVE_QUEST_ITEM(i);
            }
        }
        REMOVE_QUEST_ITEM(QUEST_SONG_LULLABY_INTRO);
    }

    auto questIcon = [&](QuestItem slot) {
        const ItemId item = questToItemMap[slot];
        if (Kit::IconButton(std::to_string(slot).c_str(), gui->GetTextureByName((const char*)gItemIcons[item]),
                            CHECK_QUEST_ITEM(slot))) {
            NextQuestInSlot(slot);
        }
        Kit::Tooltip(SlotItemName(item));
    };

    ImGui::SeparatorText("Boss rewards");
    for (int32_t i = QUEST_REMAINS_ODOLWA; i <= QUEST_REMAINS_TWINMOLD; i++) {
        if (i != QUEST_REMAINS_ODOLWA) {
            ImGui::SameLine();
        }
        questIcon(static_cast<QuestItem>(i));
    }

    ImGui::SeparatorText("Songs");
    auto drawSongRange = [](int32_t start, int32_t end) {
        for (int32_t i = start; i <= end; i++) {
            ImVec2 itemSize = DrawSong(static_cast<QuestItem>(i));
            if (i != end) {
                float currentCursorX = ImGui::GetCursorPosX();
                float nextItemMaxX = currentCursorX + ImGui::GetStyle().ItemSpacing.x + itemSize.x;
                float regionMaxX = ImGui::GetWindowContentRegionMax().x;
                if (nextItemMaxX <= regionMaxX) {
                    ImGui::SameLine();
                }
            }
        }
    };
    drawSongRange(QUEST_SONG_TIME, QUEST_SONG_SUN);
    drawSongRange(QUEST_SONG_SONATA, QUEST_SONG_SARIA);

    ImGui::SeparatorText("Quest items");
    questIcon(QUEST_BOMBERS_NOTEBOOK);

    ImGui::SeparatorText("Gold Skulltula tokens");
    int swampTokens = Inventory_GetSkullTokenCount(SCENE_KINSTA1);
    if (Kit::ResourceBar("##swampSkulltulas", &swampTokens, 0, 30, "Swamp tokens: %d", theme)) {
        gSaveContext.save.saveInfo.skullTokenCount =
            ((int)(swampTokens & 0xFFFF) << 0x10) | (gSaveContext.save.saveInfo.skullTokenCount & 0xFFFF);
    }
    int oceanTokens = Inventory_GetSkullTokenCount(SCENE_KINDAN2);
    if (Kit::ResourceBar("##oceanSkulltulas", &oceanTokens, 0, 30, "Ocean tokens: %d", theme)) {
        gSaveContext.save.saveInfo.skullTokenCount =
            (gSaveContext.save.saveInfo.skullTokenCount & 0xFFFF0000) | (oceanTokens & 0xFFFF);
    }

    ImGui::SeparatorText("Heart pieces");
    int32_t pohCount = (gSaveContext.save.saveInfo.inventory.questItems & 0xF0000000) >> 28;
    if (Kit::ResourceBar("##heartPieces", &pohCount, 0, 3, "Heart pieces: %d", Kit::kHeartPieces)) {
        gSaveContext.save.saveInfo.inventory.questItems &= ~0xF0000000;
        gSaveContext.save.saveInfo.inventory.questItems |= (pohCount << 28);
    }

    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(1);
}
#else
void DrawQuestStatusTab() {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 1.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
    ImGui::BeginChild("questTab", ImVec2(0, 0), true);

    if (UIWidgets::Button("Give All##items", { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Green })) {
        for (int32_t i = QUEST_REMAINS_ODOLWA; i <= QUEST_BOMBERS_NOTEBOOK; i++) {
            if (i != QUEST_SHIELD && i != QUEST_SWORD) {
                SET_QUEST_ITEM(i);
            }
        }
        SET_EQUIP_VALUE(EQUIP_TYPE_SHIELD, EQUIP_VALUE_SHIELD_MIRROR);
        if (GET_PLAYER_FORM != PLAYER_FORM_FIERCE_DEITY) {
            SET_EQUIP_VALUE(EQUIP_TYPE_SWORD, EQUIP_VALUE_SWORD_GILDED);
            if (gPlayState) {
                Player_SetEquipmentData(gPlayState, GET_PLAYER(gPlayState));
                BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_B) =
                    ITEM_SWORD_KOKIRI + GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SWORD) - EQUIP_VALUE_SWORD_KOKIRI;
                Interface_LoadItemIconImpl(gPlayState, EQUIP_SLOT_B);
            }
        }
    }
    ImGui::SameLine();
    if (UIWidgets::Button("Reset##items", { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Red })) {
        for (int32_t i = QUEST_REMAINS_ODOLWA; i <= QUEST_BOMBERS_NOTEBOOK; i++) {
            if (i != QUEST_SHIELD && i != QUEST_SWORD) {
                REMOVE_QUEST_ITEM(i);
            }
        }
        REMOVE_QUEST_ITEM(QUEST_SONG_LULLABY_INTRO);
        SET_EQUIP_VALUE(EQUIP_TYPE_SHIELD, EQUIP_VALUE_SHIELD_HERO);
        if (GET_PLAYER_FORM != PLAYER_FORM_FIERCE_DEITY) {
            SET_EQUIP_VALUE(EQUIP_TYPE_SWORD, EQUIP_VALUE_SWORD_KOKIRI);
            if (gPlayState) {
                Player_SetEquipmentData(gPlayState, GET_PLAYER(gPlayState));
                BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_B) =
                    ITEM_SWORD_KOKIRI + GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SWORD) - EQUIP_VALUE_SWORD_KOKIRI;
                Interface_LoadItemIconImpl(gPlayState, EQUIP_SLOT_B);
            }
        }
    }
    ImGui::BeginChild("leftColumn", ImVec2(ImGui::GetWindowWidth() / 2, 0));
    ImGui::SeparatorText("Boss Remains");
    for (int32_t i = QUEST_REMAINS_ODOLWA; i <= QUEST_REMAINS_TWINMOLD; i++) {
        QuestItem slot = static_cast<QuestItem>(i);

        DrawQuestSlot(slot);
    }
    ImGui::SeparatorText("Songs");
    auto drawSongRange = [](int32_t start, int32_t end) {
        for (int32_t i = start; i <= end; i++) {
            ImVec2 itemSize = DrawSong(static_cast<QuestItem>(i));
            if (i != end) {
                float currentCursorX = ImGui::GetCursorPosX();
                float nextItemMaxX = currentCursorX + ImGui::GetStyle().ItemSpacing.x + itemSize.x;
                float regionMaxX = ImGui::GetWindowContentRegionMax().x;
                if (nextItemMaxX <= regionMaxX) {
                    ImGui::SameLine();
                }
            }
        }
    };
    drawSongRange(QUEST_SONG_TIME, QUEST_SONG_SUN);
    drawSongRange(QUEST_SONG_SONATA, QUEST_SONG_SARIA);
    ImGui::SeparatorText("Equipment");
    if (GET_PLAYER_FORM == PLAYER_FORM_FIERCE_DEITY) {
        ImTextureID swordTextureId =
            std::dynamic_pointer_cast<Fast::Fast3dGui>(Ship::Context::GetRawInstance()->GetWindow()->GetGui())
                ->GetTextureByName((const char*)gItemIcons[ITEM_SWORD_DEITY]);
        ImGui::ImageButton(std::to_string(ITEM_SWORD_DEITY).c_str(), swordTextureId,
                           ImVec2(INV_GRID_ICON_SIZE, INV_GRID_ICON_SIZE), ImVec2(0, 0), ImVec2(1, 1),
                           ImVec4(0, 0, 0, 0), ImVec4(1, 1, 1, 1));
    } else {
        int swordValue = GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SWORD);
        if (swordValue == EQUIP_VALUE_SWORD_NONE) {
            swordValue = EQUIP_VALUE_SWORD_KOKIRI;
        }
        ImTextureID swordTextureId =
            std::dynamic_pointer_cast<Fast::Fast3dGui>(Ship::Context::GetRawInstance()->GetWindow()->GetGui())
                ->GetTextureByName((const char*)gItemIcons[ITEM_SWORD_KOKIRI + swordValue - EQUIP_VALUE_SWORD_KOKIRI]);

        if (ImGui::ImageButton(std::to_string(ITEM_SWORD_KOKIRI).c_str(), swordTextureId,
                               ImVec2(INV_GRID_ICON_SIZE, INV_GRID_ICON_SIZE), ImVec2(0, 0), ImVec2(1, 1),
                               ImVec4(0, 0, 0, 0), ImVec4(1, 1, 1, GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SWORD) ? 1 : 0.4f))) {
            NextQuestInSlot(QUEST_SWORD);
        }
    }
    ImGui::SameLine();
    int shieldValue = GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SHIELD);
    if (shieldValue == EQUIP_VALUE_SHIELD_NONE) {
        shieldValue = EQUIP_VALUE_SHIELD_HERO;
    }
    ImTextureID shieldTextureId =
        std::dynamic_pointer_cast<Fast::Fast3dGui>(Ship::Context::GetRawInstance()->GetWindow()->GetGui())
            ->GetTextureByName((const char*)gItemIcons[ITEM_SHIELD_HERO + shieldValue - EQUIP_VALUE_SHIELD_HERO]);

    if (ImGui::ImageButton(std::to_string(ITEM_SHIELD_HERO).c_str(), shieldTextureId,
                           ImVec2(INV_GRID_ICON_SIZE, INV_GRID_ICON_SIZE), ImVec2(0, 0), ImVec2(1, 1),
                           ImVec4(0, 0, 0, 0), ImVec4(1, 1, 1, GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SHIELD) ? 1 : 0.4f))) {
        NextQuestInSlot(QUEST_SHIELD);
    }
    ImGui::SameLine();
    ImTextureID textureId =
        std::dynamic_pointer_cast<Fast::Fast3dGui>(Ship::Context::GetRawInstance()->GetWindow()->GetGui())
            ->GetTextureByName((const char*)gItemIcons[ITEM_BOMBERS_NOTEBOOK]);
    if (ImGui::ImageButton(std::to_string(ITEM_BOMBERS_NOTEBOOK).c_str(), textureId,
                           ImVec2(INV_GRID_ICON_SIZE, INV_GRID_ICON_SIZE), ImVec2(0, 0), ImVec2(1, 1),
                           ImVec4(0, 0, 0, 0),
                           ImVec4(1, 1, 1, CHECK_QUEST_ITEM(QUEST_BOMBERS_NOTEBOOK) ? 1.0f : 0.4f))) {
        NextQuestInSlot(QUEST_BOMBERS_NOTEBOOK);
    }
    ImGui::SeparatorText("Heart Pieces");
    int32_t pohCount = (gSaveContext.save.saveInfo.inventory.questItems & 0xF0000000) >> 28;
    UIWidgets::PushStyleCombobox(UIWidgets::Colors::Red);
    if (ImGui::BeginCombo("##PoHcount", std::to_string(pohCount).c_str())) {
        for (int32_t i = 0; i < 4; i++) {
            if (ImGui::Selectable(std::to_string(i).c_str(), pohCount == i)) {
                gSaveContext.save.saveInfo.inventory.questItems &= ~0xF0000000;
                gSaveContext.save.saveInfo.inventory.questItems |= (i << 28);
            }
        }
        ImGui::EndCombo();
    }
    UIWidgets::PopStyleCombobox();
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("rightColumn", ImVec2(0, 0));
    ImGui::SeparatorText("Gold Skulltula Tokens");
    int OceanSkullTokens = Inventory_GetSkullTokenCount(SCENE_KINDAN2);
    int SwampSkullTokens = Inventory_GetSkullTokenCount(SCENE_KINSTA1);
    UIWidgets::PushStyleSlider();
    ImGui::PushItemWidth(ImGui::GetWindowWidth());
    if (ImGui::SliderInt("##swampSkulltulas", &SwampSkullTokens, 0, 30, "Swamp Tokens: %d", ImGuiSliderFlags_NoInput)) {
        gSaveContext.save.saveInfo.skullTokenCount =
            ((int)(SwampSkullTokens & 0xFFFF) << 0x10) | (gSaveContext.save.saveInfo.skullTokenCount & 0xFFFF);
    }
    if (ImGui::SliderInt("##oceanSkulltulas", &OceanSkullTokens, 0, 30, "Ocean Tokens: %d", ImGuiSliderFlags_NoInput)) {
        gSaveContext.save.saveInfo.skullTokenCount =
            (gSaveContext.save.saveInfo.skullTokenCount & 0xFFFF0000) | (OceanSkullTokens & 0xFFFF);
    }
    ImGui::PopItemWidth();
    UIWidgets::PopStyleSlider();

    ImGui::EndChild();
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(1);
}
#endif

#ifdef COMBO_BUILD
// ComboShip: Link's sword set as the Quest Status sword cycle set it, to one chosen value.
static void SetSwordValue(int value) {
    SET_EQUIP_VALUE(EQUIP_TYPE_SWORD, value);
    if (GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SWORD) == EQUIP_VALUE_SWORD_NONE) {
        BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_B) = ITEM_NONE;
    } else {
        BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_B) =
            ITEM_SWORD_KOKIRI + GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SWORD) - EQUIP_VALUE_SWORD_KOKIRI;
    }
    if (GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SWORD) == EQUIP_VALUE_SWORD_RAZOR) {
        gSaveContext.save.saveInfo.playerData.swordHealth = 100;
    }
    Interface_LoadItemIconImpl(gPlayState, EQUIP_SLOT_B);
}

// ComboShip: a quiver or bomb bag level, with the ammo it holds cut to the new capacity.
static void SetQuiverLevel(int level) {
    Inventory_ChangeUpgrade(UPG_QUIVER, level);
    AMMO(ITEM_BOW) = MIN(AMMO(ITEM_BOW), CUR_CAPACITY(UPG_QUIVER));
}
static void SetBombBagLevel(int level) {
    Inventory_ChangeUpgrade(UPG_BOMB_BAG, level);
    AMMO(ITEM_BOMB) = MIN(AMMO(ITEM_BOMB), CUR_CAPACITY(UPG_BOMB_BAG));
    AMMO(ITEM_BOMBCHU) = MIN(AMMO(ITEM_BOMBCHU), CUR_CAPACITY(UPG_BOMB_BAG));
}

// ComboShip: the Equipment tab upstream leaves commented out, mirrored by OoT's: Give all and Reset, the sword and
// shield (moved from Quest Status, where each tap cycled them) and the quiver and bomb bag (which Bow and Bomb taps
// used to cycle) as pickers with names. The boots and tunics are inventory items in MM and stay on Inventory.
void DrawEquipmentTab() {
    namespace Kit = ComboSaveEditorKit;
    auto gui = std::dynamic_pointer_cast<Fast::Fast3dGui>(Ship::Context::GetRawInstance()->GetWindow()->GetGui());
    auto icon = [&](ItemId item) { return gui->GetTextureByName((const char*)gItemIcons[item]); };
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
    ImGui::BeginChild("equipmentTab", ImVec2(0, 0), true);

    // Give all and Reset do for the sword and shield what Quest Status's did, and set the best or no bags.
    if (Kit::Button("Give all##equipment", Kit::kGiveAll)) {
        SET_EQUIP_VALUE(EQUIP_TYPE_SHIELD, EQUIP_VALUE_SHIELD_MIRROR);
        if (GET_PLAYER_FORM != PLAYER_FORM_FIERCE_DEITY) {
            SET_EQUIP_VALUE(EQUIP_TYPE_SWORD, EQUIP_VALUE_SWORD_GILDED);
            if (gPlayState) {
                Player_SetEquipmentData(gPlayState, GET_PLAYER(gPlayState));
                BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_B) =
                    ITEM_SWORD_KOKIRI + GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SWORD) - EQUIP_VALUE_SWORD_KOKIRI;
                Interface_LoadItemIconImpl(gPlayState, EQUIP_SLOT_B);
            }
        }
        SetQuiverLevel(3);
        SetBombBagLevel(3);
    }
    ImGui::SameLine();
    if (Kit::Button("Reset##equipment", Kit::kReset)) {
        SET_EQUIP_VALUE(EQUIP_TYPE_SHIELD, EQUIP_VALUE_SHIELD_HERO);
        if (GET_PLAYER_FORM != PLAYER_FORM_FIERCE_DEITY) {
            SET_EQUIP_VALUE(EQUIP_TYPE_SWORD, EQUIP_VALUE_SWORD_KOKIRI);
            if (gPlayState) {
                Player_SetEquipmentData(gPlayState, GET_PLAYER(gPlayState));
                BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_B) =
                    ITEM_SWORD_KOKIRI + GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SWORD) - EQUIP_VALUE_SWORD_KOKIRI;
                Interface_LoadItemIconImpl(gPlayState, EQUIP_SLOT_B);
            }
        }
        SetQuiverLevel(0);
        SetBombBagLevel(0);
    }

    // The sword and shield change Link himself, so, as on Quest Status, only while playing.
    ImGui::SeparatorText("Sword and shield");
    if (!gPlayState) {
        ImGui::TextDisabled("%s", Kit::kNeedsPlay);
    }
    ImGui::BeginDisabled(!gPlayState);
    if (GET_PLAYER_FORM == PLAYER_FORM_FIERCE_DEITY) {
        ImGui::BeginGroup();
        Kit::Icon(icon(ITEM_SWORD_DEITY), true);
        ImGui::TextUnformatted("Sword");
        ImGui::EndGroup();
        Kit::Tooltip("Fierce Deity's Sword");
    } else {
        const int sword = Kit::LevelPicker("Sword", GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SWORD),
                                           { { Kit::kNoTexture, "None" },
                                             { icon(ITEM_SWORD_KOKIRI), "Kokiri Sword" },
                                             { icon(ITEM_SWORD_RAZOR), "Razor Sword" },
                                             { icon(ITEM_SWORD_GILDED), "Gilded Sword" } });
        if (sword >= 0 && gPlayState) {
            SetSwordValue(sword);
        }
    }
    ImGui::SameLine();
    const int shield = Kit::LevelPicker("Shield", GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SHIELD),
                                        { { Kit::kNoTexture, "None" },
                                          { icon(ITEM_SHIELD_HERO), "Hero's Shield" },
                                          { icon(ITEM_SHIELD_MIRROR), "Mirror Shield" } });
    if (shield >= 0 && gPlayState) {
        SET_EQUIP_VALUE(EQUIP_TYPE_SHIELD, shield);
        Player_SetEquipmentData(gPlayState, GET_PLAYER(gPlayState));
    }
    ImGui::EndDisabled();

    ImGui::SeparatorText("Upgrades");
    const int quiver = Kit::LevelPicker("Quiver", CUR_UPG_VALUE(UPG_QUIVER),
                                        { { Kit::kNoTexture, "None" },
                                          { icon(ITEM_QUIVER_30), "Quiver (30)" },
                                          { icon(ITEM_QUIVER_40), "Large Quiver (40)" },
                                          { icon(ITEM_QUIVER_50), "Largest Quiver (50)" } });
    if (quiver >= 0) {
        SetQuiverLevel(quiver);
    }
    ImGui::SameLine();
    const int bombBag = Kit::LevelPicker("Bomb Bag", CUR_UPG_VALUE(UPG_BOMB_BAG),
                                         { { Kit::kNoTexture, "None" },
                                           { icon(ITEM_BOMB_BAG_20), "Bomb Bag (20)" },
                                           { icon(ITEM_BOMB_BAG_30), "Big Bomb Bag (30)" },
                                           { icon(ITEM_BOMB_BAG_40), "Biggest Bomb Bag (40)" } });
    if (bombBag >= 0) {
        SetBombBagLevel(bombBag);
    }

    ImGui::EndChild();
    ImGui::PopStyleVar(2);
}
#endif

void SetDungeonItems(uint32_t dungeonItem, uint32_t dungeonId) {
    if (CHECK_DUNGEON_ITEM(dungeonItem, dungeonId)) {
        REMOVE_DUNGEON_ITEM(dungeonItem, dungeonId);
    } else {
        SET_DUNGEON_ITEM(dungeonItem, dungeonId);
    }
}

const char* dungeonNames[4] = { "Woodfall Temple", "Snowhead Temple", "Great Bay Temple", "Stone Tower Temple" };
uint32_t smallKeyCounts[4] = { 1, 3, 1, 4 };
const char* fairyIcons[] = { gDungeonStrayFairyWoodfallIconTex, gDungeonStrayFairySnowheadIconTex,
                             gDungeonStrayFairyGreatBayIconTex, gDungeonStrayFairyStoneTowerIconTex };
#ifdef COMBO_BUILD
// ComboShip: the dungeon items as OoT draws them: one card per temple with Map, Compass and Boss key, the small key and
// stray fairy icons (faded at none; the small key one was lit at 0 keys) and inline bars instead of popups.
void DrawDungeonItemTab() {
    namespace Kit = ComboSaveEditorKit;
    const ImVec4 theme = UIWidgets::ColorValues.at(UIWidgets::Colors(CVarGetInteger("gSettings.Menu.Theme", 5)));
    auto gui = std::dynamic_pointer_cast<Fast::Fast3dGui>(Ship::Context::GetRawInstance()->GetWindow()->GetGui());
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
    ImGui::BeginChild("dungeonTab", ImVec2(0, 0), true);
    Kit::BeginCardLayout(2, Kit::kGridWidth * 5);
    for (int i = DUNGEON_SCENE_INDEX_WOODFALL_TEMPLE; i <= DUNGEON_SCENE_INDEX_STONE_TOWER_TEMPLE; i++) {
        ImGui::PushID(i);
        Kit::BeginCard("dungeon");
        ImGui::Text("%s", dungeonNames[i]);
        if (Kit::IconButton("##map", gui->GetTextureByName(gQuestIconDungeonMapTex),
                            CHECK_DUNGEON_ITEM(DUNGEON_MAP, i))) {
            SetDungeonItems(DUNGEON_MAP, i);
        }
        Kit::Tooltip("Map");
        ImGui::SameLine();
        if (Kit::IconButton("##compass", gui->GetTextureByName(gQuestIconCompassTex),
                            CHECK_DUNGEON_ITEM(DUNGEON_COMPASS, i))) {
            SetDungeonItems(DUNGEON_COMPASS, i);
        }
        Kit::Tooltip("Compass");
        ImGui::SameLine();
        if (Kit::IconButton("##bossKey", gui->GetTextureByName(gQuestIconBossKeyTex),
                            CHECK_DUNGEON_ITEM(DUNGEON_BOSS_KEY, i))) {
            SetDungeonItems(DUNGEON_BOSS_KEY, i);
        }
        Kit::Tooltip("Boss Key");
        ImGui::SameLine();
        Kit::Icon(gui->GetTextureByName(gQuestIconSmallKeyTex), DUNGEON_KEY_COUNT(i) > 0);
        ImGui::SameLine();
        Kit::Icon(gui->GetTextureByName(fairyIcons[i]), gSaveContext.save.saveInfo.inventory.strayFairies[i] > 0);
        Kit::ResourceBar("##sKeyCount", &gSaveContext.save.saveInfo.inventory.dungeonKeys[i], -1, smallKeyCounts[i],
                         "Small keys: %d", theme);
        Kit::ResourceBar("##strayCount", &gSaveContext.save.saveInfo.inventory.strayFairies[i], 0, 15,
                         "Stray fairies: %d", theme);
        Kit::EndCard();
        ImGui::PopID();
    }
    Kit::EndCardLayout();
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
}
#else
void DrawDungeonItemTab() {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 1.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    ImGui::BeginChild("dungeonTab", ImVec2(0, 0), true);

    for (int i = DUNGEON_SCENE_INDEX_WOODFALL_TEMPLE; i <= DUNGEON_SCENE_INDEX_STONE_TOWER_TEMPLE; i++) {
        std::string stray_id = "Stray" + std::to_string(i);
        std::string map_id = "Map" + std::to_string(i);
        std::string comp_id = "Compass" + std::to_string(i);
        std::string sKey_id = "Small Key" + std::to_string(i);
        std::string bKey_id = "Boss Key" + std::to_string(i);
        int dungeonId = i;
        ImGui::BeginChild(std::to_string(i).c_str(),
                          ImVec2(INV_GRID_WIDTH * 6 + INV_GRID_PADDING * 2, INV_GRID_HEIGHT * 1.5 + INV_GRID_PADDING),
                          ImGuiChildFlags_Border);
        ImGui::Text("%s", dungeonNames[i]);
        if (ImGui::ImageButton(
                stray_id.c_str(),
                std::dynamic_pointer_cast<Fast::Fast3dGui>(Ship::Context::GetRawInstance()->GetWindow()->GetGui())
                    ->GetTextureByName(fairyIcons[dungeonId]),
                ImVec2(INV_GRID_ICON_SIZE, INV_GRID_ICON_SIZE), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0),
                ImVec4(1, 1, 1, gSaveContext.save.saveInfo.inventory.strayFairies[dungeonId] ? 1.0f : 0.4f))) {
            ImGui::OpenPopup("strayFairies");
        }
        ImGui::SameLine();
        if (ImGui::ImageButton(
                map_id.c_str(),
                std::dynamic_pointer_cast<Fast::Fast3dGui>(Ship::Context::GetRawInstance()->GetWindow()->GetGui())
                    ->GetTextureByName(gQuestIconDungeonMapTex),
                ImVec2(INV_GRID_ICON_SIZE, INV_GRID_ICON_SIZE), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0),
                ImVec4(1, 1, 1, CHECK_DUNGEON_ITEM(DUNGEON_MAP, i) ? 1.0f : 0.4f))) {
            SetDungeonItems(DUNGEON_MAP, i);
        }
        ImGui::SameLine();
        if (ImGui::ImageButton(
                comp_id.c_str(),
                std::dynamic_pointer_cast<Fast::Fast3dGui>(Ship::Context::GetRawInstance()->GetWindow()->GetGui())
                    ->GetTextureByName(gQuestIconCompassTex),
                ImVec2(INV_GRID_ICON_SIZE, INV_GRID_ICON_SIZE), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0),
                ImVec4(1, 1, 1, CHECK_DUNGEON_ITEM(DUNGEON_COMPASS, i) ? 1.0f : 0.4f))) {
            SetDungeonItems(DUNGEON_COMPASS, i);
        }
        ImGui::SameLine();
        if (ImGui::ImageButton(
                sKey_id.c_str(),
                std::dynamic_pointer_cast<Fast::Fast3dGui>(Ship::Context::GetRawInstance()->GetWindow()->GetGui())
                    ->GetTextureByName(gQuestIconSmallKeyTex),
                ImVec2(INV_GRID_ICON_SIZE, INV_GRID_ICON_SIZE), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0),
                ImVec4(1, 1, 1, DUNGEON_KEY_COUNT(i) + 1 ? 1.0f : 0.4f))) {
            ImGui::OpenPopup("smallKeys");
        }
        ImGui::SameLine();
        if (ImGui::ImageButton(
                bKey_id.c_str(),
                std::dynamic_pointer_cast<Fast::Fast3dGui>(Ship::Context::GetRawInstance()->GetWindow()->GetGui())
                    ->GetTextureByName(gQuestIconBossKeyTex),
                ImVec2(INV_GRID_ICON_SIZE, INV_GRID_ICON_SIZE), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0),
                ImVec4(1, 1, 1, CHECK_DUNGEON_ITEM(DUNGEON_BOSS_KEY, i) ? 1.0f : 0.4f))) {
            SetDungeonItems(DUNGEON_BOSS_KEY, i);
        }
        if (ImGui::BeginPopup("strayFairies")) {
            UIWidgets::PushStyleSlider(UIWidgets::Colors(CVarGetInteger("gSettings.Menu.Theme", 5)));
            s32 minStray = 0;
            s32 maxStray = 15;
            int currentStrays = gSaveContext.save.saveInfo.inventory.strayFairies[dungeonId];
            ImGui::PushItemWidth(ImGui::GetFontSize() * 12.0f);
            if (ImGui::SliderScalar("##strayCount", ImGuiDataType_S32, &currentStrays, &minStray, &maxStray,
                                    "Strays: %d", ImGuiSliderFlags_NoInput)) {
                gSaveContext.save.saveInfo.inventory.strayFairies[dungeonId] = currentStrays;
            }
            ImGui::PopItemWidth();
            UIWidgets::PopStyleSlider();
            ImGui::EndPopup();
        }
        if (ImGui::BeginPopup("smallKeys")) {
            UIWidgets::PushStyleSlider(UIWidgets::Colors(CVarGetInteger("gSettings.Menu.Theme", 5)));
            s32 minKey = -1;
            s32 maxKey = smallKeyCounts[dungeonId];
            int currentKeys = gSaveContext.save.saveInfo.inventory.dungeonKeys[dungeonId];
            ImGui::PushItemWidth(ImGui::GetFontSize() * 12.0f);
            if (ImGui::SliderScalar("##sKeyCount", ImGuiDataType_S32, &currentKeys, &minKey, &maxKey, "Small Keys: %d",
                                    ImGuiSliderFlags_NoInput)) {
                gSaveContext.save.saveInfo.inventory.dungeonKeys[dungeonId] = currentKeys;
            }
            ImGui::PopItemWidth();
            UIWidgets::PopStyleSlider();
            ImGui::EndPopup();
        }
        ImGui::EndChild();
    }
    ImGui::EndChild();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(1);
}
#endif

void GetPlayerForm(uint32_t form) {
    switch (form) {
        case PLAYER_FORM_FIERCE_DEITY:
            curForm = "Fierce Deity";
            formColor = UIWidgets::Colors::Red;
            formObject = OBJECT_LINK_BOY;
            break;
        case PLAYER_FORM_GORON:
            curForm = "Goron";
            formColor = UIWidgets::Colors::Brown;
            formObject = OBJECT_LINK_GORON;
            break;
        case PLAYER_FORM_ZORA:
            curForm = "Zora";
            formColor = UIWidgets::Colors::LightBlue;
            formObject = OBJECT_LINK_ZORA;
            break;
        case PLAYER_FORM_DEKU:
            curForm = "Deku";
            formColor = UIWidgets::Colors::DarkGreen;
            formObject = OBJECT_LINK_NUTS;
            break;
        case PLAYER_FORM_HUMAN:
            curForm = "Human";
            formColor = UIWidgets::Colors::Green;
            formObject = OBJECT_LINK_CHILD;
            break;
        default:
            break;
    }
}

void ClearAllEquippedItems() {
    auto& buttonItems = gSaveContext.save.saveInfo.equips.buttonItems;
    auto& cButtonSlots = gSaveContext.save.saveInfo.equips.cButtonSlots;

    for (size_t form = 0; form < ARRAY_COUNT(buttonItems); form++) {
        for (size_t slot = 0; slot < ARRAY_COUNT(buttonItems[form]); slot++) {
            if (slot == EQUIP_SLOT_B) {
                // Goron/Deku/Zora/FD expect their innate B actions; leave untouched.
                // Form 0 is shared by FD and Human via CUR_FORM
                if (form != 0 || GET_PLAYER_FORM == PLAYER_FORM_FIERCE_DEITY) {
                    continue;
                }

                int swordValue = GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SWORD);
                if (swordValue == EQUIP_VALUE_SWORD_NONE) {
                    buttonItems[form][slot] = ITEM_NONE;
                } else {
                    buttonItems[form][slot] = ITEM_SWORD_KOKIRI + swordValue - EQUIP_VALUE_SWORD_KOKIRI;
                }
                continue;
            }

            // B slot stores item directly (sword); cButtonSlots only maps C-buttons to inventory slots
            buttonItems[form][slot] = ITEM_NONE;
            cButtonSlots[form][slot] = SLOT_NONE;
        }
    }

    auto& dpadItems = gSaveContext.save.shipSaveInfo.dpadEquips.dpadItems;
    auto& dpadSlots = gSaveContext.save.shipSaveInfo.dpadEquips.dpadSlots;

    for (size_t form = 0; form < ARRAY_COUNT(dpadItems); form++) {
        for (size_t slot = 0; slot < ARRAY_COUNT(dpadItems[form]); slot++) {
            dpadItems[form][slot] = ITEM_NONE;
            dpadSlots[form][slot] = SLOT_NONE;
        }
    }
}

#ifdef COMBO_BUILD
// ComboShip: the Player tab with OoT's cards and labels: Position & rotation, Speed, Form, Equipped gear (read from the
// equipped values; upstream previewed a remembered index), Button items (D-pad in OoT's order) with Clear all, and
// Player state. "Available while playing." with no game running.
void DrawPlayerTab() {
    namespace Kit = ComboSaveEditorKit;
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 1.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
    ImGui::BeginChild("playerTab", ImVec2(0, 0), true);

    if (gPlayState == nullptr) {
        ImGui::TextWrapped("%s", Kit::kNeedsPlay);
    } else {
        Player* player = GET_PLAYER(gPlayState);
        GetPlayerForm(GET_PLAYER_FORM);
        Kit::BeginCardLayout();

        Kit::BeginCard("playerLocation");
        ImGui::Text("Position & rotation");
        UIWidgets::PushStyleCombobox(formColor);
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
        ImGui::InputScalar("X##Pos", ImGuiDataType_Float, &player->actor.world.pos.x);
        ImGui::InputScalar("Y##Pos", ImGuiDataType_Float, &player->actor.world.pos.y);
        ImGui::InputScalar("Z##Pos", ImGuiDataType_Float, &player->actor.world.pos.z);
        ComboTouch::Count("X rotation", &player->actor.world.rot.x, -32768, 32767);
        ComboTouch::Count("Y rotation", &player->actor.world.rot.y, -32768, 32767);
        ComboTouch::Count("Z rotation", &player->actor.world.rot.z, -32768, 32767);
        ImGui::PopItemWidth();
        UIWidgets::PopStyleCombobox();
        Kit::EndCard();

        Kit::BeginCard("playerSpeed");
        ImGui::Text("Speed");
        UIWidgets::PushStyleCombobox(formColor);
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
        ImGui::InputScalar("XZ speed", ImGuiDataType_Float, &player->speedXZ);
        ImGui::InputScalar("Y velocity", ImGuiDataType_Float, &player->actor.velocity.y);
        ImGui::InputScalar("Ledge height", ImGuiDataType_Float, &player->yDistToLedge);
        ComboTouch::Count("Invincibility timer", &player->invincibilityTimer, -128, 127);
        ImGui::InputScalar("Gravity", ImGuiDataType_Float, &player->actor.gravity);
        ImGui::PopItemWidth();
        UIWidgets::PopStyleCombobox();
        Kit::EndCard();

        Kit::BeginCard("playerForm");
        ImGui::Text("Form");
        GetPlayerForm(GET_PLAYER_FORM);
        ImGui::Text("%s Link", curForm);
        for (int i = PLAYER_FORM_FIERCE_DEITY; i <= PLAYER_FORM_HUMAN; i++) {
            GetPlayerForm(i);
            UIWidgets::PushStyleButton(formColor);
            if (ImGui::Button(curForm)) {
                s16 objectId = formObject;
                if (i != PLAYER_FORM_HUMAN) {
                    gSaveContext.save.equippedMask = PLAYER_MASK_FIERCE_DEITY + i;
                }
                gActorOverlayTable[ACTOR_PLAYER].profile->objectId = objectId;
                func_8012F73C(&gPlayState->objectCtx, player->actor.objectSlot, objectId);
                player->actor.objectSlot = Object_GetSlot(&gPlayState->objectCtx, GAMEPLAY_KEEP);
                gSaveContext.save.playerForm = i;
                s32 objectSlot =
                    Object_GetSlot(&gPlayState->objectCtx, gActorOverlayTable[ACTOR_PLAYER].profile->objectId);
                player->actor.objectSlot = objectSlot;
                player->actor.shape.rot.z = GET_PLAYER_FORM + 1;
                player->actor.init = PlayerCall_Init;
                player->actor.update = PlayerCall_Update;
                player->actor.draw = PlayerCall_Draw;
                gSaveContext.save.equippedMask = PLAYER_MASK_NONE;

                TransitionFade_SetColor(&gPlayState->unk_18E48, 0x000000);
                R_TRANS_FADE_FLASH_ALPHA_STEP = -1;
                Player_PlaySfx(GET_PLAYER(gPlayState), NA_SE_SY_TRANSFORM_MASK_FLASH);
            }
            UIWidgets::PopStyleButton();
            if (i != PLAYER_FORM_HUMAN) {
                ImGui::SameLine();
            }
        }
        Kit::EndCard();

        Kit::BeginCard("playerGear");
        ImGui::Text("Equipped gear");
        if (GET_PLAYER_FORM == PLAYER_FORM_HUMAN) {
            static const char* swordNames[] = { "None", "Kokiri Sword", "Razor Sword", "Gilded Sword" };
            static const char* shieldNames[] = { "None", "Hero's Shield", "Mirror Shield" };
            const int sword = GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SWORD);
            const int shield = GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SHIELD);
            UIWidgets::PushStyleCombobox(UIWidgets::Colors(CVarGetInteger("gSettings.Menu.Theme", 5)));
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
            if (ImGui::BeginCombo("Sword", sword < 4 ? swordNames[sword] : "Fierce Deity's Sword")) {
                for (int i = EQUIP_VALUE_SWORD_KOKIRI; i <= EQUIP_VALUE_SWORD_GILDED; i++) {
                    if (ImGui::Selectable(swordNames[i], sword == i)) {
                        SET_EQUIP_VALUE(EQUIP_TYPE_SWORD, i);
                        if (GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SWORD) == EQUIP_VALUE_SWORD_RAZOR) {
                            gSaveContext.save.saveInfo.playerData.swordHealth = 100;
                        }
                        BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_B) =
                            ITEM_SWORD_KOKIRI + GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SWORD) - EQUIP_VALUE_SWORD_KOKIRI;
                        Interface_LoadItemIconImpl(gPlayState, EQUIP_SLOT_B);
                    }
                }
                ImGui::EndCombo();
            }
            if (ImGui::BeginCombo("Shield", shield < 3 ? shieldNames[shield] : "Glitched")) {
                for (int i = EQUIP_VALUE_SHIELD_HERO; i <= EQUIP_VALUE_SHIELD_MIRROR; i++) {
                    if (ImGui::Selectable(shieldNames[i], shield == i)) {
                        SET_EQUIP_VALUE(EQUIP_TYPE_SHIELD, i);
                        Player_SetEquipmentData(gPlayState, player);
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();
            UIWidgets::PopStyleCombobox();
        } else {
            ImGui::TextWrapped("No equipment in this form");
        }
        Kit::EndCard();

        Kit::BeginCard("currentItems");
        ImGui::Text("Button items");
        ImGui::PushItemWidth(ImGui::GetFontSize() * 6);
        UIWidgets::PushStyleInput();
        static u8 one = 1;
        ImGui::InputScalar("B", ImGuiDataType_U8, &CUR_FORM_EQUIP(0), &one, NULL);
        ImGui::InputScalar("C-Left", ImGuiDataType_U8, &CUR_FORM_EQUIP(1), &one, NULL);
        ImGui::InputScalar("C-Down", ImGuiDataType_U8, &CUR_FORM_EQUIP(2), &one, NULL);
        ImGui::InputScalar("C-Right", ImGuiDataType_U8, &CUR_FORM_EQUIP(3), &one, NULL);
        ImGui::InputScalar("D-pad Up", ImGuiDataType_U8, &DPAD_GET_CUR_FORM_BTN_ITEM(EQUIP_SLOT_D_UP), &one, NULL);
        ImGui::InputScalar("D-pad Down", ImGuiDataType_U8, &DPAD_GET_CUR_FORM_BTN_ITEM(EQUIP_SLOT_D_DOWN), &one, NULL);
        ImGui::InputScalar("D-pad Left", ImGuiDataType_U8, &DPAD_GET_CUR_FORM_BTN_ITEM(EQUIP_SLOT_D_LEFT), &one, NULL);
        ImGui::InputScalar("D-pad Right", ImGuiDataType_U8, &DPAD_GET_CUR_FORM_BTN_ITEM(EQUIP_SLOT_D_RIGHT), &one,
                           NULL);
        UIWidgets::PopStyleInput();
        ImGui::PopItemWidth();
        if (Kit::Button("Clear all##buttons", Kit::kReset)) {
            ClearAllEquippedItems();
        }
        Kit::EndCard();

        Kit::BeginCard("playerStates");
        ImGui::Text("Player state");
        uint32_t states[4] = { player->stateFlags1, player->stateFlags2, player->stateFlags3,
                               player->meleeWeaponState };

        if (ImGui::BeginTable("stateTable", 4)) {
            GetPlayerForm(GET_PLAYER_FORM);
            ImGui::PushStyleVar(ImGuiTableColumnFlags_WidthFixed, 15.0f);
            ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, formColor);
            ImGui::TableSetupColumn("State 1");
            ImGui::TableSetupColumn("State 2");
            ImGui::TableSetupColumn("State 3");
            ImGui::TableSetupColumn("Sword State");
            ImGui::TableHeadersRow();
            ImGui::TableNextColumn();

            for (int i = 0; i <= 3; i++) {
                ImGui::Text("%s", std::to_string(states[i]).c_str());
                ImGui::TableNextColumn();
            }

            ImGui::PopStyleColor(1);
            ImGui::PopStyleVar(1);
            ImGui::EndTable();
        }
        Kit::EndCard();

        Kit::EndCardLayout();
    }
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(1);
}
#else
void DrawPlayerTab() {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 1.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
    ImGui::BeginChild("playerTab", ImVec2(0, 0), true);

    if (gPlayState) {
        Player* player = GET_PLAYER(gPlayState);

        UIWidgets::BeginCardLayout({ .columnsPerRow = 2, .minColumnWidth = 420.0f });

        UIWidgets::BeginCard("playerSpeed");
        ImGui::Text("Link's Speed");
        UIWidgets::PushStyleCombobox(formColor);
        ImGui::PushItemWidth(ImGui::GetFontSize() * 6);
        ImGui::InputScalar("XZ Speed", ImGuiDataType_Float, &player->speedXZ);
        ImGui::InputScalar("Y Velocity", ImGuiDataType_Float, &player->actor.velocity.y);
        ImGui::InputScalar("Ledge Height", ImGuiDataType_Float, &player->yDistToLedge);
        ComboTouch::Count("Invincibility Timer", &player->invincibilityTimer, -128, 127);
        ImGui::InputScalar("Gravity", ImGuiDataType_Float, &player->actor.gravity);
        ImGui::PopItemWidth();
        UIWidgets::PopStyleCombobox();
        UIWidgets::EndCard();

        UIWidgets::BeginCard("playerLocation");
        GetPlayerForm(GET_PLAYER_FORM);
        ImGui::Text("%s Link", curForm);
        ImGui::Text("Position:");
        UIWidgets::PushStyleCombobox(formColor);
        ImGui::PushItemWidth(ImGui::GetFontSize() * 6);
        ImGui::InputScalar("X Pos", ImGuiDataType_Float, &player->actor.world.pos.x);
        ImGui::SameLine();
        ImGui::InputScalar("Y Pos", ImGuiDataType_Float, &player->actor.world.pos.y);
        ImGui::SameLine();
        ImGui::InputScalar("Z Pos", ImGuiDataType_Float, &player->actor.world.pos.z);
        ImGui::Text("Rotation:");
        ComboTouch::Count("X Rot", &player->actor.world.rot.x, -32768, 32767);
        ImGui::SameLine();
        ComboTouch::Count("Y Rot", &player->actor.world.rot.y, -32768, 32767);
        ImGui::SameLine();
        ComboTouch::Count("Z Rot", &player->actor.world.rot.z, -32768, 32767);
        ImGui::PopItemWidth();
        UIWidgets::PopStyleCombobox();
        UIWidgets::EndCard();

        UIWidgets::BeginCard("playerForm");
        ImGui::Text("Change Link's Current Form");
        for (int i = PLAYER_FORM_FIERCE_DEITY; i <= PLAYER_FORM_HUMAN; i++) {
            GetPlayerForm(i);
            UIWidgets::PushStyleButton(formColor);
            if (ImGui::Button(curForm)) {
                s16 objectId = formObject;
                if (i != PLAYER_FORM_HUMAN) {
                    gSaveContext.save.equippedMask = PLAYER_MASK_FIERCE_DEITY + i;
                }
                gActorOverlayTable[ACTOR_PLAYER].profile->objectId = objectId;
                func_8012F73C(&gPlayState->objectCtx, player->actor.objectSlot, objectId);
                player->actor.objectSlot = Object_GetSlot(&gPlayState->objectCtx, GAMEPLAY_KEEP);
                gSaveContext.save.playerForm = i;
                s32 objectSlot =
                    Object_GetSlot(&gPlayState->objectCtx, gActorOverlayTable[ACTOR_PLAYER].profile->objectId);
                player->actor.objectSlot = objectSlot;
                player->actor.shape.rot.z = GET_PLAYER_FORM + 1;
                player->actor.init = PlayerCall_Init;
                player->actor.update = PlayerCall_Update;
                player->actor.draw = PlayerCall_Draw;
                gSaveContext.save.equippedMask = PLAYER_MASK_NONE;

                TransitionFade_SetColor(&gPlayState->unk_18E48, 0x000000);
                R_TRANS_FADE_FLASH_ALPHA_STEP = -1;
                Player_PlaySfx(GET_PLAYER(gPlayState), NA_SE_SY_TRANSFORM_MASK_FLASH);
            }
            UIWidgets::PopStyleButton();
            if (i != PLAYER_FORM_HUMAN) {
                ImGui::SameLine();
            }
        }

        ImGui::Separator();
        ImGui::Text("Link's Current Equipment");
        if (GET_PLAYER_FORM == PLAYER_FORM_HUMAN) {
            static const char* weaponCombo[] = { "Kokiri Sword", "Razor Sword", "Gilded Sword" };
            static const char* shieldCombo[] = { "Hero's Shield", "Mirror Shield" };
            static int currentWeapon = 0;
            static int currentShield = 0;

            UIWidgets::PushStyleCombobox(UIWidgets::Colors::Green);
            ImGui::PushItemWidth(ImGui::GetFontSize() * 15);
            if (ImGui::BeginCombo("Equipped Sword", weaponCombo[currentWeapon])) {
                for (int i = EQUIP_VALUE_SWORD_KOKIRI; i <= EQUIP_VALUE_SWORD_GILDED; i++) {
                    const bool isSelected = (currentWeapon == i);
                    if (ImGui::Selectable(weaponCombo[i - 1], isSelected)) {
                        currentWeapon = i - 1;
                        SET_EQUIP_VALUE(EQUIP_TYPE_SWORD, i);
                        if (GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SWORD) == EQUIP_VALUE_SWORD_RAZOR) {
                            gSaveContext.save.saveInfo.playerData.swordHealth = 100;
                        }
                        BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_B) =
                            ITEM_SWORD_KOKIRI + GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SWORD) - EQUIP_VALUE_SWORD_KOKIRI;
                        Interface_LoadItemIconImpl(gPlayState, EQUIP_SLOT_B);
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            if (ImGui::BeginCombo("Equipped Shield", shieldCombo[currentShield])) {
                for (int i = EQUIP_VALUE_SHIELD_HERO; i <= EQUIP_VALUE_SHIELD_MIRROR; i++) {
                    const bool isSelected = (currentShield == i);
                    if (ImGui::Selectable(shieldCombo[i - 1], isSelected)) {
                        currentShield = i - 1;
                        SET_EQUIP_VALUE(EQUIP_TYPE_SHIELD, i);
                        Player_SetEquipmentData(gPlayState, player);
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::PopItemWidth();
            UIWidgets::PopStyleCombobox();

        } else {
            ImGui::Separator();
            ImGui::Text("              No Equipment in this Form");
            ImGui::Separator();
        }
        UIWidgets::EndCard();

        UIWidgets::BeginCard("currentItems");
        ImGui::Text("Current Items");

        // Two-column layout
        ImGui::Columns(2, nullptr, false);

        // Left column: C-buttons
        ImGui::PushItemWidth(ImGui::GetFontSize() * 6);
        UIWidgets::PushStyleInput();
        static u8 one = 1;
        ImGui::InputScalar("B Button", ImGuiDataType_U8, &CUR_FORM_EQUIP(0), &one, NULL);
        ImGui::InputScalar("C Left", ImGuiDataType_U8, &CUR_FORM_EQUIP(1), &one, NULL);
        ImGui::InputScalar("C Down", ImGuiDataType_U8, &CUR_FORM_EQUIP(2), &one, NULL);
        ImGui::InputScalar("C Right", ImGuiDataType_U8, &CUR_FORM_EQUIP(3), &one, NULL);
        UIWidgets::PopStyleInput();
        ImGui::PopItemWidth();

        // Right column: D-pad
        ImGui::NextColumn();
        ImGui::PushItemWidth(ImGui::GetFontSize() * 6);
        UIWidgets::PushStyleInput();
        ImGui::InputScalar("D-Pad Right", ImGuiDataType_U8, &DPAD_GET_CUR_FORM_BTN_ITEM(EQUIP_SLOT_D_RIGHT), &one,
                           NULL);
        ImGui::InputScalar("D-Pad Left", ImGuiDataType_U8, &DPAD_GET_CUR_FORM_BTN_ITEM(EQUIP_SLOT_D_LEFT), &one, NULL);
        ImGui::InputScalar("D-Pad Down", ImGuiDataType_U8, &DPAD_GET_CUR_FORM_BTN_ITEM(EQUIP_SLOT_D_DOWN), &one, NULL);
        ImGui::InputScalar("D-Pad Up", ImGuiDataType_U8, &DPAD_GET_CUR_FORM_BTN_ITEM(EQUIP_SLOT_D_UP), &one, NULL);
        UIWidgets::PopStyleInput();
        ImGui::PopItemWidth();

        ImGui::Columns(1);

        // Clear All button
        ImGui::Spacing();
        if (UIWidgets::Button("Clear All", { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Red })) {
            ClearAllEquippedItems();
        }

        UIWidgets::EndCard();

        UIWidgets::BeginCard("playerStates");
        ImGui::Text("Player States");
        uint32_t states[4] = { player->stateFlags1, player->stateFlags2, player->stateFlags3,
                               player->meleeWeaponState };

        if (ImGui::BeginTable("stateTable", 4)) {
            GetPlayerForm(GET_PLAYER_FORM);
            ImGui::PushStyleVar(ImGuiTableColumnFlags_WidthFixed, 15.0f);
            ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, formColor);
            ImGui::TableSetupColumn("State 1");
            ImGui::TableSetupColumn("State 2");
            ImGui::TableSetupColumn("State 3");
            ImGui::TableSetupColumn("Sword State");
            ImGui::TableHeadersRow();
            ImGui::TableNextColumn();

            for (int i = 0; i <= 3; i++) {
                ImGui::Text("%s", std::to_string(states[i]).c_str());
                ImGui::TableNextColumn();
            }

            ImGui::PopStyleColor(1);
            ImGui::PopStyleVar(1);
            ImGui::EndTable();
        }
        UIWidgets::EndCard();

        UIWidgets::EndCardLayout();
    }
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(1);
}
#endif

const char* regGroupNames[] = {
    "REG  (0)",  "SREG (1)",  "OREG (2)",  "PREG (3)",  "QREG (4)",  "MREG (5)",  "YREG (6)",  "DREG (7)",
    "UREG (8)",  "IREG (9)",  "ZREG (10)", "CREG (11)", "NREG (12)", "KREG (13)", "XREG (14)", "cREG (15)",
    "sREG (16)", "iREG (17)", "WREG (18)", "AREG (19)", "VREG (20)", "HREG (21)", "GREG (22)", "mREG (23)",
    "nREG (24)", "BREG (25)", "dREG (26)", "kREG (27)", "bREG (28)",
};

// 2S2H Added columns to scene table: entranceSceneId, betterMapSelectIndex, humanName
#define DEFINE_SCENE(_name, enumValue, _textId, _drawConfig, _restrictionFlags, _persistentCycleFlags, \
                     _entranceSceneId, _betterMapSelectIndex, humanName)                               \
    { enumValue, humanName },
#define DEFINE_SCENE_UNSET(_enumValue)

std::unordered_map<s16, const char*> sceneList = {
#include "tables/scene_table.h"
};

#undef DEFINE_SCENE
#undef DEFINE_SCENE_UNSET

void DrawRegEditorTab() {
    UIWidgets::PushStyleSlider();
    ImGui::SliderScalar("Reg Group", ImGuiDataType_U8, &gRegEditor->regGroup, &S8_ZERO, &REG_GROUPS_MAX,
                        regGroupNames[gRegEditor->regGroup], ImGuiSliderFlags_NoInput);
    ImGui::SliderScalar("Reg Page", ImGuiDataType_U8, &gRegEditor->regPage, &S8_ZERO, &REG_PAGES_MAX, "%d",
                        ImGuiSliderFlags_NoInput);
    UIWidgets::PopStyleSlider();

    ImGui::BeginChild("regSliders", ImVec2(0, 0), ImGuiChildFlags_Border);

    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnWidth(0, 100.0f);

    for (int i = 0; i < REG_PER_PAGE; i++) {
        ImGui::PushID(i);
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 4.0f);
        ImGui::Text("%02X (%d)", i + gRegEditor->regPage * REG_PER_PAGE, i + gRegEditor->regPage * REG_PER_PAGE);
        ImGui::NextColumn();
        ImGui::PushStyleColor(ImGuiCol_FrameBg, UIWidgets::ColorValues.at(UIWidgets::Colors::Gray));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 8.0f));
        ImGui::InputScalar(
            "##regInput", ImGuiDataType_S16,
            &gRegEditor->data[i + gRegEditor->regPage * REG_PER_PAGE + gRegEditor->regGroup * REG_PER_PAGE * REG_PAGES],
            NULL, NULL, "%d");
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(1);
        ImGui::NextColumn();
        ImGui::PopItemWidth();
        ImGui::PopID();
    }

    ImGui::Columns(1);

    ImGui::EndChild();
}

#ifdef COMBO_BUILD
// ComboShip: the flags of a named table (week events, temporary events, randomizer flags) whose description or number
// matches the search, one checkbox each, as OoT's flag tables search. Entries are in bit order, `width` per row.
template <typename T>
static void DrawFlagSearchResults(const FlagTable& flagTable, T* rows, size_t rowCount, ImGuiTextFilter& filter,
                                  UIWidgets::Colors theme) {
    constexpr size_t width = sizeof(T) * 8;
    bool hasMatches = false;
    for (size_t i = 0; i < flagTable.entries.size() && i / width < rowCount; i++) {
        const std::string label = fmt::format("0x{:03X}: {}", i, flagTable.entries[i].description);
        if (!filter.PassFilter(label.c_str())) {
            continue;
        }
        hasMatches = true;
        T& flags = rows[i / width];
        const T mask = static_cast<T>(1u << (i % width));
        bool set = (flags & mask) != 0;
        ImGui::PushID(static_cast<int>(i));
        if (UIWidgets::Checkbox(label.c_str(), &set, { .color = theme })) {
            flags = set ? static_cast<T>(flags | mask) : static_cast<T>(flags & ~mask);
        }
        ImGui::PopID();
    }
    if (!hasMatches) {
        ImGui::Text("No flags match the current search.");
    }
}
#endif

const char* flagEditorSections[] = { "currentSceneFlags", "weekEventReg",  "eventInf",
                                     "scenesVisible",     "owlActivation", "permanentSceneFlags",
                                     "cycleSceneFlags",   "randoInf",      "collectedHearts" };

#ifdef COMBO_BUILD
// ComboShip: the Flags tab with OoT's layout: a "Flag type" list with plain names in a fixed order (it showed the code
// names), groups with plain names and Set all (theme) / Clear all (Red), and a search box on the week event, temporary
// event and randomizer flag tables. collectedHearts is left out: every MM save in Combo Ship is a randomizer save, for
// which it only said "Flags are not relevant to rando".
void DrawFlagsTab() {
    namespace Kit = ComboSaveEditorKit;
    const UIWidgets::Colors theme = UIWidgets::Colors(CVarGetInteger("gSettings.Menu.Theme", 5));
    const ImVec4 themeColour = UIWidgets::ColorValues.at(theme);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
    ImGui::BeginChild("flagsBox", ImVec2(0, 0), true);
    static const char* typeNames[] = { "Current scene",    "Saved scene",    "Cycle scene", "Week events",
                                       "Temporary events", "Scenes visited", "Owl statues", "Randomizer flags" };
    static const FlagTableType typeTables[] = { CURRENT_SCENE_FLAGS, PERMANENT_SCENE_FLAGS,
                                                CYCLE_SCENE_FLAGS,   WEEK_EVENT_REG,
                                                EVENT_INF,           SCENES_VISIBLE,
                                                OWL_ACTIVATION,      RANDO_INF };
    static int selectedType = 0;
    UIWidgets::Combobox("Flag type", &selectedType, typeNames,
                        { .alignment = UIWidgets::ComponentAlignment::Left,
                          .labelPosition = UIWidgets::LabelPosition::Near,
                          .color = theme });
    const FlagTableType type = typeTables[selectedType];

    if (type == WEEK_EVENT_REG) { // Draw color legend
        ImGui::TextColored(UIWidgets::ColorValues.at(UIWidgets::Colors::Gray), ICON_FA_SQUARE "Persistent");
        ImGui::SameLine();
        ImGui::TextColored(UIWidgets::ColorValues.at(UIWidgets::Colors::LightBlue), ICON_FA_SQUARE "Cycle");
        ImGui::SameLine();
        ImGui::TextColored(UIWidgets::ColorValues.at(UIWidgets::Colors::Orange), ICON_FA_SQUARE "Scene/Other");
    }

    static int16_t selectedScene = 0;
    if (type == PERMANENT_SCENE_FLAGS || type == CYCLE_SCENE_FLAGS) {
        UIWidgets::ComboboxWithSearch("Scene", &selectedScene, &sceneList,
                                      {
                                          .alignment = UIWidgets::ComponentAlignment::Left,
                                          .labelPosition = UIWidgets::LabelPosition::None,
                                          .color = theme,
                                      });
        if (gPlayState != NULL) {
            ImGui::SameLine();
            if (Kit::Button("Current", themeColour)) {
                selectedScene = gPlayState->sceneId;
            }
        }
    }

    // One group of 32 flags: its name, Set all and Clear all, then the flags.
    auto group = [&](const std::string& name, uint32_t& flags) {
        Kit::BeginCard(name.c_str());
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s", name.c_str());
        ImGui::SameLine();
        if (Kit::Button(("Set all##" + name).c_str(), themeColour)) {
            flags = UINT32_MAX;
        }
        ImGui::SameLine();
        if (Kit::Button(("Clear all##" + name).c_str(), Kit::kReset)) {
            flags = 0;
        }
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1.0f, 1.0f));
        UIWidgets::DrawFlagArray32("##" + name, flags, theme);
        ImGui::PopStyleVar();
        Kit::EndCard();
    };
    // A search box per named flag table.
    auto search = [&](FlagTableType table) -> ImGuiTextFilter& {
        static std::map<int, ImGuiTextFilter> filters;
        ImGuiTextFilter& filter = filters[table];
        UIWidgets::PushStyleInput(theme);
        filter.Draw("Search", ImGui::GetFontSize() * 16);
        UIWidgets::PopStyleInput();
        return filter;
    };

    switch (type) {
        case CURRENT_SCENE_FLAGS: {
            if (gPlayState == NULL) {
                ImGui::TextWrapped("%s", Kit::kNeedsPlay);
                break;
            }
            auto& sceneFlags = gPlayState->actorCtx.sceneFlags;
            Kit::BeginCardLayout(2, 360.0f);
            for (int i = 0; i < 4; i++) {
                group(fmt::format("Switch {}", i + 1), sceneFlags.switches[i]);
            }
            group("Chest", sceneFlags.chest);
            group("Room clear", sceneFlags.clearedRoom);
            group("Temporary room clear", sceneFlags.clearedRoomTemp);
            for (int i = 0; i < 4; i++) {
                group(fmt::format("Collectible {}", i + 1), sceneFlags.collectible[i]);
            }
            Kit::EndCardLayout();
            break;
        }
        case WEEK_EVENT_REG: {
            ImGuiTextFilter& filter = search(type);
            if (filter.IsActive()) {
                DrawFlagSearchResults(flagTables.at(WEEK_EVENT_REG), gSaveContext.save.saveInfo.weekEventReg,
                                      ARRAY_COUNT(gSaveContext.save.saveInfo.weekEventReg), filter, theme);
                break;
            }
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1.0f, 1.0f));
            for (int i = 0; i < 100; i++) {
                ImGui::PushID(i);
                ImGui::Text("%02d", i);
                ImGui::SameLine();
                UIWidgets::DrawFlagTableArray8Mask(flagTables.at(WEEK_EVENT_REG), i,
                                                   gSaveContext.save.saveInfo.weekEventReg[i]);
                ImGui::PopID();
            }
            ImGui::PopStyleVar();
            break;
        }
        case EVENT_INF: {
            ImGuiTextFilter& filter = search(type);
            if (filter.IsActive()) {
                DrawFlagSearchResults(flagTables.at(EVENT_INF), gSaveContext.eventInf,
                                      ARRAY_COUNT(gSaveContext.eventInf), filter, theme);
                break;
            }
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1.0f, 1.0f));
            for (int i = 0; i < 8; i++) {
                ImGui::PushID(i);
                ImGui::Text("%02d", i);
                ImGui::SameLine();
                UIWidgets::DrawFlagTableArray8(flagTables.at(EVENT_INF), i, gSaveContext.eventInf[i]);
                ImGui::PopID();
            }
            ImGui::PopStyleVar();
            break;
        }
        case SCENES_VISIBLE:
            if (Kit::Button("Set all##scenesVisible", themeColour)) {
                for (int i = 0; i < 7; i++) {
                    gSaveContext.save.saveInfo.scenesVisible[i] = UINT32_MAX;
                }
            }
            ImGui::SameLine();
            if (Kit::Button("Clear all##scenesVisible", Kit::kReset)) {
                for (int i = 0; i < 7; i++) {
                    gSaveContext.save.saveInfo.scenesVisible[i] = 0;
                }
            }
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1.0f, 1.0f));
            for (int i = 0; i < 7; i++) {
                ImGui::PushID(i);
                UIWidgets::DrawFlagArray32("##", gSaveContext.save.saveInfo.scenesVisible[i], theme);
                ImGui::PopID();
            }
            ImGui::PopStyleVar();
            break;
        case OWL_ACTIVATION:
            DrawOwlStatuesEditor();
            break;
        case PERMANENT_SCENE_FLAGS: {
            auto& sceneFlags = gSaveContext.save.saveInfo.permanentSceneFlags[selectedScene];
            Kit::BeginCardLayout(2, 360.0f);
            group("Chest", sceneFlags.chest);
            group("Switch 1", sceneFlags.switch0);
            group("Switch 2", sceneFlags.switch1);
            group("Collectible", sceneFlags.collectible);
            group("Room clear", sceneFlags.clearedRoom);
            group("Floors visited (dungeons)", sceneFlags.unk_14);
            group("Rooms visited", sceneFlags.rooms);
            Kit::EndCardLayout();
            break;
        }
        case CYCLE_SCENE_FLAGS: {
            auto& sceneFlags = gSaveContext.cycleSceneFlags[selectedScene];
            Kit::BeginCardLayout(2, 360.0f);
            group("Chest", sceneFlags.chest);
            group("Switch 1", sceneFlags.switch0);
            group("Switch 2", sceneFlags.switch1);
            group("Collectible", sceneFlags.collectible);
            group("Room clear", sceneFlags.clearedRoom);
            Kit::EndCardLayout();
            break;
        }
        case RANDO_INF: {
            ImGuiTextFilter& filter = search(type);
            if (filter.IsActive()) {
                DrawFlagSearchResults(flagTables.at(RANDO_INF), gSaveContext.save.shipSaveInfo.rando.randoInf,
                                      ARRAY_COUNT(gSaveContext.save.shipSaveInfo.rando.randoInf), filter, theme);
                break;
            }
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1.0f, 1.0f));
            for (int i = 0; i < 5; i++) {
                ImGui::PushID(i);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%02d", i);
                ImGui::SameLine(ImGui::CalcTextSize("000").x + ImGui::GetStyle().ItemSpacing.x);
                UIWidgets::DrawFlagTableArray16(flagTables.at(RANDO_INF), i,
                                                gSaveContext.save.shipSaveInfo.rando.randoInf[i]);
                ImGui::PopID();
            }
            ImGui::PopStyleVar();
            break;
        }
        default:
            break;
    }
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
}
#else
void DrawFlagsTab() {
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
    ImGui::BeginChild("flagsBox", ImVec2(0, 0), true);
    static int selectedFlagSection = 0;
    UIWidgets::Combobox("Flag Type", &selectedFlagSection, flagEditorSections,
                        {
                            .alignment = UIWidgets::ComponentAlignment::Left,
                            .labelPosition = UIWidgets::LabelPosition::None,
                        });

    if (selectedFlagSection == WEEK_EVENT_REG) { // Draw color legend
        ImGui::SameLine();
        ImGui::TextColored(UIWidgets::ColorValues.at(UIWidgets::Colors::Gray), ICON_FA_SQUARE "Persistent");
        ImGui::SameLine();
        ImGui::TextColored(UIWidgets::ColorValues.at(UIWidgets::Colors::LightBlue), ICON_FA_SQUARE "Cycle");
        ImGui::SameLine();
        ImGui::TextColored(UIWidgets::ColorValues.at(UIWidgets::Colors::Orange), ICON_FA_SQUARE "Scene/Other");
    }

    static int16_t selectedScene = 0;
    if (selectedFlagSection == PERMANENT_SCENE_FLAGS || selectedFlagSection == CYCLE_SCENE_FLAGS) {
        ImGui::SameLine();
        UIWidgets::ComboboxWithSearch("Scene", &selectedScene, &sceneList,
                                      {
                                          .alignment = UIWidgets::ComponentAlignment::Left,
                                          .labelPosition = UIWidgets::LabelPosition::None,
                                      });
        if (gPlayState != NULL) {
            ImGui::SameLine();
            if (UIWidgets::Button("Current", UIWidgets::ButtonOptions{ { .color = UIWidgets::Colors::Gray } }.Size(
                                                 UIWidgets::Sizes::Inline))) {
                selectedScene = gPlayState->sceneId;
            }
        }
    }

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1.0f, 1.0f));
    switch (selectedFlagSection) {
        case CURRENT_SCENE_FLAGS:
            if (gPlayState == NULL) {
                ImGui::Text("Play state is NULL, cannot display scene flags");
                break;
            }

            ImGui::BeginGroup();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("switches[0]");
            ImGui::SameLine(110);
            if (UIWidgets::Button("All##switches0",
                                  { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Gray })) {
                gPlayState->actorCtx.sceneFlags.switches[0] = UINT32_MAX;
            }
            ImGui::SameLine();
            if (UIWidgets::Button("Clear##switches0",
                                  { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Red })) {
                gPlayState->actorCtx.sceneFlags.switches[0] = 0;
            }
            UIWidgets::DrawFlagArray32("##switches0", gPlayState->actorCtx.sceneFlags.switches[0]);
            ImGui::EndGroup();
            ImGui::SameLine(0, 10.0f);
            ImGui::BeginGroup();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("switches[1]");
            ImGui::SameLine(110);
            if (UIWidgets::Button("All##switches1",
                                  { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Gray })) {
                gPlayState->actorCtx.sceneFlags.switches[1] = UINT32_MAX;
            }
            ImGui::SameLine();
            if (UIWidgets::Button("Clear##switches1",
                                  { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Red })) {
                gPlayState->actorCtx.sceneFlags.switches[1] = 0;
            }
            UIWidgets::DrawFlagArray32("##switches1", gPlayState->actorCtx.sceneFlags.switches[1]);
            ImGui::EndGroup();
            ImGui::BeginGroup();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("switches[2]");
            ImGui::SameLine(110);
            if (UIWidgets::Button("All##switches2",
                                  { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Gray })) {
                gPlayState->actorCtx.sceneFlags.switches[2] = UINT32_MAX;
            }
            ImGui::SameLine();
            if (UIWidgets::Button("Clear##switches2",
                                  { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Red })) {
                gPlayState->actorCtx.sceneFlags.switches[2] = 0;
            }
            UIWidgets::DrawFlagArray32("##switches2", gPlayState->actorCtx.sceneFlags.switches[2]);
            ImGui::EndGroup();
            ImGui::SameLine(0, 10.0f);
            ImGui::BeginGroup();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("switches[3]");
            ImGui::SameLine(110);
            if (UIWidgets::Button("All##switches3",
                                  { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Gray })) {
                gPlayState->actorCtx.sceneFlags.switches[3] = UINT32_MAX;
            }
            ImGui::SameLine();
            if (UIWidgets::Button("Clear##switches3",
                                  { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Red })) {
                gPlayState->actorCtx.sceneFlags.switches[3] = 0;
            }
            UIWidgets::DrawFlagArray32("##switches3", gPlayState->actorCtx.sceneFlags.switches[3]);
            ImGui::EndGroup();
            ImGui::BeginGroup();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("chest");
            ImGui::SameLine(110);
            if (UIWidgets::Button("All##chest",
                                  { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Gray })) {
                gPlayState->actorCtx.sceneFlags.chest = UINT32_MAX;
            }
            ImGui::SameLine();
            if (UIWidgets::Button("Clear##chest",
                                  { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Red })) {
                gPlayState->actorCtx.sceneFlags.chest = 0;
            }
            UIWidgets::DrawFlagArray32("##chest", gPlayState->actorCtx.sceneFlags.chest);
            ImGui::EndGroup();
            ImGui::SameLine(0, 10.0f);
            ImGui::BeginGroup();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("clearedRoom");
            ImGui::SameLine(110);
            if (UIWidgets::Button("All##clearedRoom",
                                  { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Gray })) {
                gPlayState->actorCtx.sceneFlags.clearedRoom = UINT32_MAX;
            }
            ImGui::SameLine();
            if (UIWidgets::Button("Clear##clearedRoom",
                                  { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Red })) {
                gPlayState->actorCtx.sceneFlags.clearedRoom = 0;
            }
            UIWidgets::DrawFlagArray32("##clearedRoom", gPlayState->actorCtx.sceneFlags.clearedRoom);
            ImGui::EndGroup();
            ImGui::BeginGroup();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("clearedRoomTemp");
            ImGui::SameLine(110);
            if (UIWidgets::Button("All##clearedRoomTemp",
                                  { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Gray })) {
                gPlayState->actorCtx.sceneFlags.clearedRoomTemp = UINT32_MAX;
            }
            ImGui::SameLine();
            if (UIWidgets::Button("Clear##clearedRoomTemp",
                                  { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Red })) {
                gPlayState->actorCtx.sceneFlags.clearedRoomTemp = 0;
            }
            UIWidgets::DrawFlagArray32("##clearedRoomTemp", gPlayState->actorCtx.sceneFlags.clearedRoomTemp);
            ImGui::EndGroup();
            ImGui::SameLine(0, 10.0f);
            ImGui::BeginGroup();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("collectible[0]");
            ImGui::SameLine(110);
            if (UIWidgets::Button("All##collectible0",
                                  { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Gray })) {
                gPlayState->actorCtx.sceneFlags.collectible[0] = UINT32_MAX;
            }
            ImGui::SameLine();
            if (UIWidgets::Button("Clear##collectible0",
                                  { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Red })) {
                gPlayState->actorCtx.sceneFlags.collectible[0] = 0;
            }
            UIWidgets::DrawFlagArray32("##collectible0", gPlayState->actorCtx.sceneFlags.collectible[0]);
            ImGui::EndGroup();
            ImGui::BeginGroup();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("collectible[1]");
            ImGui::SameLine(110);
            if (UIWidgets::Button("All##collectible1",
                                  { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Gray })) {
                gPlayState->actorCtx.sceneFlags.collectible[1] = UINT32_MAX;
            }
            ImGui::SameLine();
            if (UIWidgets::Button("Clear##collectible1",
                                  { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Red })) {
                gPlayState->actorCtx.sceneFlags.collectible[1] = 0;
            }
            UIWidgets::DrawFlagArray32("##collectible1", gPlayState->actorCtx.sceneFlags.collectible[1]);
            ImGui::EndGroup();
            ImGui::SameLine(0, 10.0f);
            ImGui::BeginGroup();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("collectible[2]");
            ImGui::SameLine(110);
            if (UIWidgets::Button("All##collectible2",
                                  { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Gray })) {
                gPlayState->actorCtx.sceneFlags.collectible[2] = UINT32_MAX;
            }
            ImGui::SameLine();
            if (UIWidgets::Button("Clear##collectible2",
                                  { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Red })) {
                gPlayState->actorCtx.sceneFlags.collectible[2] = 0;
            }
            UIWidgets::DrawFlagArray32("##collectible2", gPlayState->actorCtx.sceneFlags.collectible[2]);
            ImGui::EndGroup();
            ImGui::BeginGroup();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("collectible[3]");
            ImGui::SameLine(110);
            if (UIWidgets::Button("All##collectible3",
                                  { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Gray })) {
                gPlayState->actorCtx.sceneFlags.collectible[3] = UINT32_MAX;
            }
            ImGui::SameLine();
            if (UIWidgets::Button("Clear##collectible3",
                                  { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Red })) {
                gPlayState->actorCtx.sceneFlags.collectible[3] = 0;
            }
            UIWidgets::DrawFlagArray32("##collectible3", gPlayState->actorCtx.sceneFlags.collectible[3]);
            ImGui::EndGroup();
            break;
        case WEEK_EVENT_REG:
            for (int i = 0; i < 100; i++) {
                ImGui::PushID(i);
                ImGui::Text("%02d", i);
                ImGui::SameLine();
                UIWidgets::DrawFlagTableArray8Mask(flagTables.at(WEEK_EVENT_REG), i,
                                                   gSaveContext.save.saveInfo.weekEventReg[i]);
                ImGui::PopID();
            }
            break;
        case EVENT_INF:
            for (int i = 0; i < 8; i++) {
                ImGui::PushID(i);
                ImGui::Text("%02d", i);
                ImGui::SameLine();
                UIWidgets::DrawFlagTableArray8(flagTables.at(EVENT_INF), i, gSaveContext.eventInf[i]);
                ImGui::PopID();
            }
            break;
        case SCENES_VISIBLE:
            if (UIWidgets::Button("All##scenesVisible",
                                  { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Gray })) {
                for (int i = 0; i < 7; i++) {
                    gSaveContext.save.saveInfo.scenesVisible[i] = UINT32_MAX;
                }
            }
            ImGui::SameLine();
            if (UIWidgets::Button("Clear##scenesVisible",
                                  { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Red })) {
                for (int i = 0; i < 7; i++) {
                    gSaveContext.save.saveInfo.scenesVisible[i] = 0;
                }
            }
            for (int i = 0; i < 7; i++) {
                ImGui::PushID(i);
                UIWidgets::DrawFlagArray32("##", gSaveContext.save.saveInfo.scenesVisible[i]);
                ImGui::PopID();
            }
            break;
        case OWL_ACTIVATION:
            ImGui::SeparatorText("Owl Statues / Warp Destinations");
            DrawOwlStatuesEditor();
            break;
        case PERMANENT_SCENE_FLAGS:
            ImGui::BeginGroup();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("chest");
            ImGui::SameLine(110);
            if (UIWidgets::Button("All##chest",
                                  { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Gray })) {
                gSaveContext.save.saveInfo.permanentSceneFlags[selectedScene].chest = UINT32_MAX;
            }
            ImGui::SameLine();
            if (UIWidgets::Button("Clear##chest",
                                  { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Red })) {
                gSaveContext.save.saveInfo.permanentSceneFlags[selectedScene].chest = 0;
            }
            UIWidgets::DrawFlagArray32("##chest", gSaveContext.save.saveInfo.permanentSceneFlags[selectedScene].chest);
            ImGui::EndGroup();
            ImGui::SameLine(0, 10.0f);
            ImGui::BeginGroup();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("switch0");
            ImGui::SameLine(110);
            if (UIWidgets::Button("All##switch0",
                                  { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Gray })) {
                gSaveContext.save.saveInfo.permanentSceneFlags[selectedScene].switch0 = UINT32_MAX;
            }
            ImGui::SameLine();
            if (UIWidgets::Button("Clear##switch0",
                                  { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Red })) {
                gSaveContext.save.saveInfo.permanentSceneFlags[selectedScene].switch0 = 0;
            }
            UIWidgets::DrawFlagArray32("##switch0",
                                       gSaveContext.save.saveInfo.permanentSceneFlags[selectedScene].switch0);
            ImGui::EndGroup();
            ImGui::BeginGroup();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("collectible");
            ImGui::SameLine(110);
            if (UIWidgets::Button("All##collectible",
                                  { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Gray })) {
                gSaveContext.save.saveInfo.permanentSceneFlags[selectedScene].collectible = UINT32_MAX;
            }
            ImGui::SameLine();
            if (UIWidgets::Button("Clear##collectible",
                                  { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Red })) {
                gSaveContext.save.saveInfo.permanentSceneFlags[selectedScene].collectible = 0;
            }
            UIWidgets::DrawFlagArray32("##collectible",
                                       gSaveContext.save.saveInfo.permanentSceneFlags[selectedScene].collectible);
            ImGui::EndGroup();
            ImGui::SameLine(0, 10.0f);
            ImGui::BeginGroup();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("switch1");
            ImGui::SameLine(110);
            if (UIWidgets::Button("All##switch1",
                                  { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Gray })) {
                gSaveContext.save.saveInfo.permanentSceneFlags[selectedScene].switch1 = UINT32_MAX;
            }
            ImGui::SameLine();
            if (UIWidgets::Button("Clear##switch1",
                                  { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Red })) {
                gSaveContext.save.saveInfo.permanentSceneFlags[selectedScene].switch1 = 0;
            }
            UIWidgets::DrawFlagArray32("##switch1",
                                       gSaveContext.save.saveInfo.permanentSceneFlags[selectedScene].switch1);
            ImGui::EndGroup();
            ImGui::BeginGroup();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("clearedRoom");
            ImGui::SameLine(110);
            if (UIWidgets::Button("All##clearedRoom",
                                  { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Gray })) {
                gSaveContext.save.saveInfo.permanentSceneFlags[selectedScene].clearedRoom = UINT32_MAX;
            }
            ImGui::SameLine();
            if (UIWidgets::Button("Clear##clearedRoom",
                                  { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Red })) {
                gSaveContext.save.saveInfo.permanentSceneFlags[selectedScene].clearedRoom = 0;
            }
            UIWidgets::DrawFlagArray32("##clearedRoom",
                                       gSaveContext.save.saveInfo.permanentSceneFlags[selectedScene].clearedRoom);
            ImGui::EndGroup();
            ImGui::SameLine(0, 10.0f);
            ImGui::BeginGroup();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("unk_14");
            ImGui::SameLine(110);
            if (UIWidgets::Button("All##unk_14",
                                  { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Gray })) {
                gSaveContext.save.saveInfo.permanentSceneFlags[selectedScene].unk_14 = UINT32_MAX;
            }
            ImGui::SameLine();
            if (UIWidgets::Button("Clear##unk_14",
                                  { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Red })) {
                gSaveContext.save.saveInfo.permanentSceneFlags[selectedScene].unk_14 = 0;
            }
            UIWidgets::DrawFlagArray32("##unk_14",
                                       gSaveContext.save.saveInfo.permanentSceneFlags[selectedScene].unk_14);
            ImGui::EndGroup();
            ImGui::BeginGroup();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("rooms");
            ImGui::SameLine(110);
            if (UIWidgets::Button("All##rooms",
                                  { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Gray })) {
                gSaveContext.save.saveInfo.permanentSceneFlags[selectedScene].rooms = UINT32_MAX;
            }
            ImGui::SameLine();
            if (UIWidgets::Button("Clear##rooms",
                                  { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Red })) {
                gSaveContext.save.saveInfo.permanentSceneFlags[selectedScene].rooms = 0;
            }
            UIWidgets::DrawFlagArray32("##rooms", gSaveContext.save.saveInfo.permanentSceneFlags[selectedScene].rooms);
            ImGui::EndGroup();
            break;
        case CYCLE_SCENE_FLAGS:
            ImGui::BeginGroup();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("chest");
            ImGui::SameLine(110);
            if (UIWidgets::Button("All##chest",
                                  { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Gray })) {
                gSaveContext.cycleSceneFlags[selectedScene].chest = UINT32_MAX;
            }
            ImGui::SameLine();
            if (UIWidgets::Button("Clear##chest",
                                  { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Red })) {
                gSaveContext.cycleSceneFlags[selectedScene].chest = 0;
            }
            UIWidgets::DrawFlagArray32("##chest", gSaveContext.cycleSceneFlags[selectedScene].chest);
            ImGui::EndGroup();
            ImGui::SameLine(0, 10.0f);
            ImGui::BeginGroup();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("switch0");
            ImGui::SameLine(110);
            if (UIWidgets::Button("All##switch0",
                                  { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Gray })) {
                gSaveContext.cycleSceneFlags[selectedScene].switch0 = UINT32_MAX;
            }
            ImGui::SameLine();
            if (UIWidgets::Button("Clear##switch0",
                                  { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Red })) {
                gSaveContext.cycleSceneFlags[selectedScene].switch0 = 0;
            }
            UIWidgets::DrawFlagArray32("##switch0", gSaveContext.cycleSceneFlags[selectedScene].switch0);
            ImGui::EndGroup();
            ImGui::BeginGroup();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("collectible");
            ImGui::SameLine(110);
            if (UIWidgets::Button("All##collectible",
                                  { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Gray })) {
                gSaveContext.cycleSceneFlags[selectedScene].collectible = UINT32_MAX;
            }
            ImGui::SameLine();
            if (UIWidgets::Button("Clear##collectible",
                                  { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Red })) {
                gSaveContext.cycleSceneFlags[selectedScene].collectible = 0;
            }
            UIWidgets::DrawFlagArray32("##collectible", gSaveContext.cycleSceneFlags[selectedScene].collectible);
            ImGui::EndGroup();
            ImGui::SameLine(0, 10.0f);
            ImGui::BeginGroup();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("switch1");
            ImGui::SameLine(110);
            if (UIWidgets::Button("All##switch1",
                                  { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Gray })) {
                gSaveContext.cycleSceneFlags[selectedScene].switch1 = UINT32_MAX;
            }
            ImGui::SameLine();
            if (UIWidgets::Button("Clear##switch1",
                                  { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Red })) {
                gSaveContext.cycleSceneFlags[selectedScene].switch1 = 0;
            }
            UIWidgets::DrawFlagArray32("##switch1", gSaveContext.cycleSceneFlags[selectedScene].switch1);
            ImGui::EndGroup();
            ImGui::BeginGroup();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("clearedRoom");
            ImGui::SameLine(110);
            if (UIWidgets::Button("All##clearedRoom",
                                  { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Gray })) {
                gSaveContext.cycleSceneFlags[selectedScene].clearedRoom = UINT32_MAX;
            }
            ImGui::SameLine();
            if (UIWidgets::Button("Clear##clearedRoom",
                                  { .size = UIWidgets::Sizes::Inline, .color = UIWidgets::Colors::Red })) {
                gSaveContext.cycleSceneFlags[selectedScene].clearedRoom = 0;
            }
            UIWidgets::DrawFlagArray32("##clearedRoom", gSaveContext.cycleSceneFlags[selectedScene].clearedRoom);
            ImGui::EndGroup();
            break;
        case RANDO_INF:
            for (int i = 0; i < 5; i++) {
                ImGui::PushID(i);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%02d", i);
                ImGui::SameLine(ImGui::CalcTextSize("000").x + ImGui::GetStyle().ItemSpacing.x);
                UIWidgets::DrawFlagTableArray16(flagTables.at(RANDO_INF), i,
                                                gSaveContext.save.shipSaveInfo.rando.randoInf[i]);
                ImGui::PopID();
            }
            break;
        case HEART_FLAGS:
            if (gPlayState == NULL) {
                ImGui::Text("Play state is NULL, cannot display flags");
                break;
            }
            if (IS_RANDO) {
                ImGui::Text("Flags are not relevant to rando");
                break;
            }
            for (size_t i = 0; i < heartFlags.size(); ++i) {
                const auto& heartFlag = heartFlags[i];
                bool collected;
                uint16_t flag = heartFlag.flag;
                std::string label;

                if (gPlayState->sceneId == heartFlag.scene) {
                    switch (heartFlag.flagType) {
                        case WEEK_EVENT_REG:
                            collected = CHECK_WEEKEVENTREG(flag);
                            label = fmt::format("weekEventReg | {:02}_{:02X}", flag >> 8, flag & 0xFF);
                            break;
                        case FLAG_CYCL_SCENE_COLLECTIBLE:
                            collected = Flags_GetCollectible(gPlayState, flag);
                            label = fmt::format("currentSceneFlags | Collectible[0] | 0x{:02X}", flag);
                            break;
                        case FLAG_CYCL_SCENE_CHEST:
                            collected = Flags_GetTreasure(gPlayState, flag);
                            label = fmt::format("currentSceneFlags | Chest | 0x{:02X}", flag);
                            break;
                        case FLAG_CYCL_SCENE_SWITCH:
                            collected = Flags_GetSwitch(gPlayState, flag);
                            label = fmt::format("currentSceneFlags | Switch[0] | 0x{:02X}", flag);
                            break;
                    }
                } else {
                    switch (heartFlag.flagType) {
                        case WEEK_EVENT_REG:
                            collected = CHECK_WEEKEVENTREG(flag);
                            label = fmt::format("weekEventReg | {:02}_{:02X}", flag >> 8, flag & 0xFF);
                            break;
                        case FLAG_CYCL_SCENE_COLLECTIBLE:
                            collected = gSaveContext.cycleSceneFlags[heartFlag.scene].collectible & (1 << flag);
                            label = fmt::format("cycleSceneFlags | {} | Collectible | 0x{:02X}",
                                                sceneList.at(heartFlag.scene), flag);
                            break;
                        case FLAG_CYCL_SCENE_CHEST:
                            collected = gSaveContext.cycleSceneFlags[heartFlag.scene].chest & (1 << flag);
                            label = fmt::format("cycleSceneFlags | {} | Chest | 0x{:02X}",
                                                sceneList.at(heartFlag.scene), flag);
                            break;
                        case FLAG_CYCL_SCENE_SWITCH:
                            collected = gSaveContext.cycleSceneFlags[heartFlag.scene].switch0 & (1 << flag);
                            label = fmt::format("cycleSceneFlags | {} | Switch0 | 0x{:02X}",
                                                sceneList.at(heartFlag.scene), flag);
                            break;
                    }
                }

                ImGui::PushID(i);
                ImGui::BeginDisabled();
                ImGui::Checkbox(heartFlag.description.c_str(), &collected);
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                    ImGui::SetTooltip("%s", label.c_str());
                }
                ImGui::EndDisabled();
                ImGui::PopID();
            }
            break;
    }
    ImGui::PopStyleVar();
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
}
#endif

#ifdef COMBO_BUILD
// ComboShip: the Randomizer tab: the time-item and give-item controls (moved here from Inventory), Generate Spoiler
// from Save, and the check list under a collapsed header because it shows where every item is. The cross-send debug
// button shows only with MM's developer options on (BenMenu.cpp's COMBO_MM_DEBUG_MODE_NAME).
void DrawRandoTab() {
    if (CVarGetInteger("gDeveloperTools.DebugEnabledMM", 0)) {
        if (UIWidgets::Button("Cross-send debug item to OOT", { .size = UIWidgets::Sizes::Inline })) {
            if (gSaveContext.fileNum == 0xFF) {
                SPDLOG_WARN("[ComboShip] MM cross-send: no save loaded (fileNum=0x{:X})",
                            (unsigned)gSaveContext.fileNum);
            } else if (gMMComboCrossDeliver) {
                // Deliver an OOT-bound item straight into OOT's resident save NOW (issue #3).
                gMMComboCrossDeliver(ComboRando::GAME_OOT, "DEBUG_ITEM", "");
                SPDLOG_INFO("[ComboShip] MM cross-send: delivered DEBUG_ITEM to OOT (slot {})",
                            (int)gSaveContext.fileNum);
            }
        }
        UIWidgets::Tooltip("ComboShip: deliver a cross-world debug item to OOT's save for the current slot");
    }
    DrawRandoItemGives();
    ImGui::Spacing();
    if (UIWidgets::Button("Generate Spoiler from Save", { .size = UIWidgets::Sizes::Inline })) {
        nlohmann::json spoiler = Rando::Spoiler::GenerateFromSaveContext();
        std::string inputSeed = std::to_string(Ship_Random(0, 1000000));
        spoiler["inputSeed"] = inputSeed;

        std::string fileName = inputSeed + ".json";
        Rando::Spoiler::SaveToFile(fileName, spoiler);
    }

    if (ImGui::CollapsingHeader("Check list (shows item locations)")) {
        static ImGuiTextFilter rcFilter;
        UIWidgets::PushStyleCombobox();
        rcFilter.Draw("##filter", ImGui::GetContentRegionAvail().x);
        UIWidgets::PopStyleCombobox();
        if (!rcFilter.IsActive()) {
            ImGui::SameLine(18.0f);
            ImGui::Text("Search");
        }

        ImGui::BeginChild("RandoChild", ImVec2(0, ImGui::GetFontSize() * 30));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 1.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.2f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 1.0f, 1.0f, 0.1f));

        ImGui::BeginTable("Check List", 5);
        ImGui::TableSetupColumn("Shuffled", ImGuiTableColumnFlags_NoHeaderLabel | ImGuiTableColumnFlags_WidthFixed,
                                30.0f);
        ImGui::TableSetupColumn("Eligible", ImGuiTableColumnFlags_NoHeaderLabel | ImGuiTableColumnFlags_WidthFixed,
                                30.0f);
        ImGui::TableSetupColumn("Obtained", ImGuiTableColumnFlags_NoHeaderLabel | ImGuiTableColumnFlags_WidthFixed,
                                30.0f);
        ImGui::TableSetupColumn("Check Name");
        ImGui::TableSetupColumn("Reward");
        ImGui::TableSetupScrollFreeze(5, 1);
        ImGui::TableHeadersRow();

        for (auto& [_, randoStaticCheck] : Rando::StaticData::Checks) {
            RandoSaveCheck& randoSaveCheck = RANDO_SAVE_CHECKS[randoStaticCheck.randoCheckId];

            if (!rcFilter.PassFilter(randoStaticCheck.name) &&
                !rcFilter.PassFilter(Rando::StaticData::Items[randoSaveCheck.randoItemId].spoilerName)) {
                continue;
            }

            if (randoStaticCheck.randoCheckId == RC_UNKNOWN) {
                continue;
            }
            std::string hiddenName = "##";
            hiddenName += randoStaticCheck.name;
            ImGui::TableNextColumn();
            UIWidgets::Checkbox((hiddenName + "shuffled").c_str(), &randoSaveCheck.shuffled);
            UIWidgets::Tooltip("Shuffled");
            ImGui::TableNextColumn();
            UIWidgets::Checkbox((hiddenName + "eligible").c_str(), &randoSaveCheck.eligible);
            UIWidgets::Tooltip("Eligible");
            ImGui::TableNextColumn();
            UIWidgets::Checkbox((hiddenName + "obtained").c_str(), &randoSaveCheck.obtained);
            UIWidgets::Tooltip("Obtained");
            ImGui::TableNextColumn();
            ImGui::TextColored(randoSaveCheck.obtained ? UIWidgets::ColorValues.at(UIWidgets::Colors::Green)
                                                       : UIWidgets::ColorValues.at(UIWidgets::Colors::White),
                               "%s", randoStaticCheck.name);
            ImGui::TableNextColumn();
            UIWidgets::ComboboxWithSearch((hiddenName + "reward").c_str(), &randoSaveCheck.randoItemId,
                                          &randoItemIdComboboxMap, { .labelPosition = UIWidgets::LabelPosition::None });
        }

        ImGui::EndTable();
        ImGui::PopStyleColor(3);
        ImGui::EndChild();
    }
}
#else
void DrawRandoTab() {
#ifdef COMBO_BUILD
    if (UIWidgets::Button("Cross-send debug item to OOT", { .size = UIWidgets::Sizes::Inline })) {
        if (gSaveContext.fileNum == 0xFF) {
            SPDLOG_WARN("[ComboShip] MM cross-send: no save loaded (fileNum=0x{:X})", (unsigned)gSaveContext.fileNum);
        } else if (gMMComboCrossDeliver) {
            // Deliver an OOT-bound item straight into OOT's resident save NOW (issue #3).
            gMMComboCrossDeliver(ComboRando::GAME_OOT, "DEBUG_ITEM", "");
            SPDLOG_INFO("[ComboShip] MM cross-send: delivered DEBUG_ITEM to OOT (slot {})", (int)gSaveContext.fileNum);
        }
    }
    UIWidgets::Tooltip("ComboShip: deliver a cross-world debug item to OOT's save for the current slot");
#endif
    if (UIWidgets::Button("Generate Spoiler from Save", { .size = UIWidgets::Sizes::Inline })) {
        nlohmann::json spoiler = Rando::Spoiler::GenerateFromSaveContext();
        std::string inputSeed = std::to_string(Ship_Random(0, 1000000));
        spoiler["inputSeed"] = inputSeed;

        std::string fileName = inputSeed + ".json";
        Rando::Spoiler::SaveToFile(fileName, spoiler);
    }

    static ImGuiTextFilter rcFilter;
    UIWidgets::PushStyleCombobox();
    rcFilter.Draw("##filter", ImGui::GetContentRegionAvail().x);
    UIWidgets::PopStyleCombobox();
    if (!rcFilter.IsActive()) {
        ImGui::SameLine(18.0f);
        ImGui::Text("Search");
    }

    ImGui::BeginChild("RandoChild");
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 1.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.2f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 1.0f, 1.0f, 0.1f));

    ImGui::BeginTable("Check List", 5);
    ImGui::TableSetupColumn("Shuffled", ImGuiTableColumnFlags_NoHeaderLabel | ImGuiTableColumnFlags_WidthFixed, 30.0f);
    ImGui::TableSetupColumn("Eligible", ImGuiTableColumnFlags_NoHeaderLabel | ImGuiTableColumnFlags_WidthFixed, 30.0f);
    ImGui::TableSetupColumn("Obtained", ImGuiTableColumnFlags_NoHeaderLabel | ImGuiTableColumnFlags_WidthFixed, 30.0f);
    ImGui::TableSetupColumn("Check Name");
    ImGui::TableSetupColumn("Reward");
    ImGui::TableSetupScrollFreeze(5, 1);
    ImGui::TableHeadersRow();

    for (auto& [_, randoStaticCheck] : Rando::StaticData::Checks) {
        RandoSaveCheck& randoSaveCheck = RANDO_SAVE_CHECKS[randoStaticCheck.randoCheckId];

        if (!rcFilter.PassFilter(randoStaticCheck.name) &&
            !rcFilter.PassFilter(Rando::StaticData::Items[randoSaveCheck.randoItemId].spoilerName)) {
            continue;
        }

        if (randoStaticCheck.randoCheckId == RC_UNKNOWN) {
            continue;
        }
        std::string hiddenName = "##";
        hiddenName += randoStaticCheck.name;
        ImGui::TableNextColumn();
        UIWidgets::Checkbox((hiddenName + "shuffled").c_str(), &randoSaveCheck.shuffled);
        UIWidgets::Tooltip("Shuffled");
        ImGui::TableNextColumn();
        UIWidgets::Checkbox((hiddenName + "eligible").c_str(), &randoSaveCheck.eligible);
        UIWidgets::Tooltip("Eligible");
        ImGui::TableNextColumn();
        UIWidgets::Checkbox((hiddenName + "obtained").c_str(), &randoSaveCheck.obtained);
        UIWidgets::Tooltip("Obtained");
        ImGui::TableNextColumn();
        ImGui::TextColored(randoSaveCheck.obtained ? UIWidgets::ColorValues.at(UIWidgets::Colors::Green)
                                                   : UIWidgets::ColorValues.at(UIWidgets::Colors::White),
                           "%s", randoStaticCheck.name);
        ImGui::TableNextColumn();
        UIWidgets::ComboboxWithSearch((hiddenName + "reward").c_str(), &randoSaveCheck.randoItemId,
                                      &randoItemIdComboboxMap, { .labelPosition = UIWidgets::LabelPosition::None });
    }

    ImGui::EndTable();
    ImGui::PopStyleColor(3);
    ImGui::EndChild();
}
#endif

#ifdef COMBO_BUILD
void SaveEditorWindow::DrawElement() {
    if (gSaveContext.fileNum >= 3 || !IS_RANDO) {
        ImGui::TextWrapped("%s", ComboSaveEditorKit::kNoMmFile);
        return;
    }
    UIWidgets::PushStyleTabs(UIWidgets::Colors(CVarGetInteger("gSettings.Menu.Theme", 5)));
    if (ImGui::BeginTabBar("SaveContextTabBar", ImGuiTabBarFlags_NoCloseWithMiddleMouseButton)) {
        // ComboShip: the same tabs, in the same order, as the OoT Save Editor; then the MM-only Randomizer tab and,
        // last, the developer Reg Editor.
        if (ImGui::BeginTabItem("General")) {
            DrawGeneralTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Inventory")) {
            DrawItemsAndMasksTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Equipment")) {
            DrawEquipmentTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Quest Status")) {
            DrawQuestStatusTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Dungeon Items")) {
            DrawDungeonItemTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Flags")) {
            DrawFlagsTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Player")) {
            DrawPlayerTab();
            ImGui::EndTabItem();
        }

        if (IS_RANDO) {
            if (ImGui::BeginTabItem("Randomizer")) {
                ComboSaveEditorKit::BeginPanel("randoTab");
                DrawRandoTab();
                ComboSaveEditorKit::EndPanel();
                ImGui::EndTabItem();
            }
        }

        if (ImGui::BeginTabItem("Reg Editor")) {
            ComboSaveEditorKit::BeginPanel("regTab");
            DrawRegEditorTab();
            ComboSaveEditorKit::EndPanel();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
    UIWidgets::PopStyleTabs();
}
#else
void SaveEditorWindow::DrawElement() {
#ifdef COMBO_BUILD
    if (gSaveContext.fileNum >= 3 || !IS_RANDO) {
        ImGui::TextWrapped("MM save is not loaded. Reload your profile before editing.");
        return;
    }
#endif
    UIWidgets::PushStyleTabs(UIWidgets::Colors(CVarGetInteger("gSettings.Menu.Theme", 5)));
    if (ImGui::BeginTabBar("SaveContextTabBar", ImGuiTabBarFlags_NoCloseWithMiddleMouseButton)) {
        if (ImGui::BeginTabItem("General")) {
            DrawGeneralTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Items & Masks")) {
            DrawItemsAndMasksTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Quest Status")) {
            DrawQuestStatusTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Dungeon Items")) {
            DrawDungeonItemTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Reg Editor")) {
            DrawRegEditorTab();
            ImGui::EndTabItem();
        }

        // if (ImGui::BeginTabItem("Equipment")) {
        //     DrawEquipmentTab();
        //     ImGui::EndTabItem();
        // }

        if (ImGui::BeginTabItem("Flags")) {
            DrawFlagsTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Player")) {
            DrawPlayerTab();
            ImGui::EndTabItem();
        }

        if (IS_RANDO) {
            if (ImGui::BeginTabItem("Rando")) {
                DrawRandoTab();
                ImGui::EndTabItem();
            }
        }

        ImGui::EndTabBar();
    }
    UIWidgets::PopStyleTabs();
}
#endif

void SaveEditorWindow::InitElement() {
    initSafeItemsForInventorySlot();
    randoItemIdComboboxMap.clear();
    for (auto& [_, randoItem] : Rando::StaticData::Items) {
        randoItemIdComboboxMap[randoItem.randoItemId] = randoItem.spoilerName;
    }
}
