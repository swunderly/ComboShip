#include "debugSaveEditor.h"
#include "gui/ComboTouchSlider.h"
#ifdef COMBO_BUILD
#include "gui/ComboSaveEditorKit.h"
#endif
#include "soh/Enhancements/randomizer/randomizerTypes.h"
#include "soh/Enhancements/randomizer/randomizer.h"
#include "soh/util.h"
#include "soh/SohGui/ImGuiUtils.h"
#include "soh/OTRGlobals.h"
#include "soh/SohGui/UIWidgets.hpp"
#include "soh/SohGui/SohGui.hpp"
#include "soh/SaveManager.h"

#include <spdlog/fmt/fmt.h>
#include <array>
#include <bit>
#include <map>
#include <string>
#include <soh_assets.h>

#include <fast/Fast3dGui.h>

extern "C" {
#include <z64.h>
#include "variables.h"
#include "functions.h"
#include "macros.h"
extern PlayState* gPlayState;
}

#include "message_data_static.h"
extern "C" MessageTableEntry* sGerMessageEntryTablePtr;
extern "C" MessageTableEntry* sFraMessageEntryTablePtr;
extern "C" MessageTableEntry* sJpnMessageEntryTablePtr;

// Maps entries in the GS flag array to the area name it represents
std::vector<const char*> gsMapping = {
    "Deku Tree",
    "Dodongo's Cavern",
    "Inside Jabu-Jabu's Belly",
    "Forest Temple",
    "Fire Temple",
    "Water Temple",
    "Spirit Temple",
    "Shadow Temple",
    "Bottom of the Well",
    "Ice Cavern",
    "Hyrule Field",
    "Lon Lon Ranch",
    "Kokiri Forest",
    "Lost Woods, Sacred Forest Meadow",
    "Castle Town and Ganon's Castle",
    "Death Mountain Trail, Goron City",
    "Kakariko Village",
    "Zora Fountain, River",
    "Lake Hylia",
    "Gerudo Valley",
    "Gerudo Fortress",
    "Desert Colossus, Haunted Wasteland",
};

extern "C" u8 gAreaGsFlags[];

extern "C" u8 gAmmoItems[];

#ifdef COMBO_BUILD
// ComboShip: the MM Save Editor's sizes (ComboSaveEditorKit), 120 px touch targets on phones.
#define IMAGE_SIZE ComboSaveEditorKit::kIconSize
#else
#define IMAGE_SIZE 48.0f
#endif

using namespace UIWidgets;

IntSliderOptions intSliderOptionsBase;
ButtonOptions buttonOptionsBase;
CheckboxOptions checkboxOptionsBase;
ComboboxOptions comboboxOptionsBase;
static std::map<std::string, ImGuiTextFilter> flagTableFilters;

// Modification of gAmmoItems that replaces ITEM_NONE with the item in inventory slot it represents
u8 gAllAmmoItems[] = {
    ITEM_STICK,     ITEM_NUT,          ITEM_BOMB,    ITEM_BOW,      ITEM_ARROW_FIRE, ITEM_DINS_FIRE,
    ITEM_SLINGSHOT, ITEM_OCARINA_TIME, ITEM_BOMBCHU, ITEM_LONGSHOT, ITEM_ARROW_ICE,  ITEM_FARORES_WIND,
    ITEM_BOOMERANG, ITEM_LENS,         ITEM_BEAN,    ITEM_HAMMER,
};

// Encapsulates what is drawn by the passed-in function within a border
template <typename T> void DrawGroupWithBorder(T&& drawFunc, std::string section) {
    // First group encapsulates the inner portion and border
    ImGui::BeginChild(std::string("##" + section).c_str(), ImVec2(0, 0),
                      ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeX |
                          ImGuiChildFlags_AutoResizeY);

    ImGui::BeginGroup();
    ImGui::AlignTextToFramePadding();
    drawFunc();
    ImGui::EndGroup();

    ImGui::EndChild();
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

#ifdef COMBO_BUILD
static uint8_t ASCII2z(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'A' && c <= 'Z') {
        return c - 'A' + 10;
    }
    if (c >= 'a' && c <= 'z') {
        return c - 'a' + 36;
    }
    if (c == '-' || c == '.') {
        return c + 0x12;
    }
    return 62;
}
#endif

std::string decodeNTSCPlayerNameChar(int code) {
    const std::string charmap[] = {
        "0",  "1",  "2",  "3",  "4",  "5",  "6",  "7",  "8",  "9",  // 10
        "あ", "い", "う", "え", "お", "か", "き", "く", "け", "こ", // 20
        "さ", "し", "す", "せ", "そ", "た", "ち", "つ", "て", "と", // 30
        "な", "に", "ぬ", "ね", "の", "は", "ひ", "ふ", "へ", "ほ", // 40
        "ま", "み", "む", "め", "も", "や", "ゆ", "よ", "ら", "り", // 50
        "る", "れ", "ろ", "わ", "を", "ん", "ぁ", "ぃ", "ぅ", "ぇ", // 60
        "ぉ", "っ", "ゃ", "ゅ", "ょ", "が", "ぎ", "ぐ", "げ", "ご", // 70
        "ざ", "じ", "ず", "ぜ", "ぞ", "だ", "ぢ", "づ", "で", "ど", // 80
        "ば", "び", "ぶ", "べ", "ぼ", "ぱ", "ぴ", "ぷ", "ぺ", "ぽ", // 90
        "ア", "イ", "ウ", "エ", "オ", "カ", "キ", "ク", "ケ", "コ", // 100
        "サ", "シ", "ス", "セ", "ソ", "タ", "チ", "ツ", "テ", "ト", // 110
        "ナ", "ニ", "ヌ", "ネ", "ノ", "ハ", "ヒ", "フ", "ヘ", "ホ", // 120
        "マ", "ミ", "ム", "メ", "モ", "ヤ", "ユ", "ヨ", "ラ", "リ", // 130
        "ル", "レ", "ロ", "ワ", "ヲ", "ン", "ァ", "ィ", "ゥ", "ェ", // 140
        "ォ", "ッ", "ャ", "ュ", "ョ", "ガ", "ギ", "グ", "ゲ", "ゴ", // 150
        "ザ", "ジ", "ズ", "ゼ", "ゾ", "ダ", "ヂ", "ヅ", "デ", "ド", // 160
        "バ", "ビ", "ブ", "ベ", "ボ", "パ", "ピ", "プ", "ペ", "ポ", // 170
        "ヴ",
    };
    std::string ret;

    if (code < 171) { // Digits and Japanese
        ret = charmap[code];
    } else if (code >= 171 && code < 197) { // Uppercase letters
        ret.assign(1, (char)(code - 171 + 65));
    } else if (code >= 197 && code < 223) { // Lowercase letters
        ret.assign(1, (char)(code - 197 + 97));
    } else if (code == 223) { // Space
        ret = " ";
    } else if (code == 228) { // -
        ret = "-";
    } else if (code == 234) { // .
        ret = ".";
    } else {
        ret = "?";
    }

    return ret;
}

enum MagicLevel { MAGIC_LEVEL_NONE, MAGIC_LEVEL_SINGLE, MAGIC_LEVEL_DOUBLE };

std::map<int8_t, const char*> magicLevelMap = {
    { MAGIC_LEVEL_NONE, "None" },
    { MAGIC_LEVEL_SINGLE, "Single" },
    { MAGIC_LEVEL_DOUBLE, "Double" },
};

enum AudioOutput {
    AUDIO_STEREO,
    AUDIO_MONO,
    AUDIO_HEADSET,
    AUDIO_SURROUND,
};

std::map<uint8_t, const char*> audioMap = {
    { AUDIO_STEREO, "Stereo" },
    { AUDIO_MONO, "Mono" },
    { AUDIO_HEADSET, "Headset" },
    { AUDIO_SURROUND, "Surround" },
};

enum ZTarget {
    Z_TARGET_SWITCH,
    Z_TARGET_HOLD,
};

std::map<uint8_t, const char*> zTargetMap = {
    { Z_TARGET_SWITCH, "Switch" },
    { Z_TARGET_HOLD, "Hold" },
};

std::map<int32_t, const char*> fileNumMap = {
    { 0, "File 1" },
    { 1, "File 2" },
    { 2, "File 3" },
};

std::map<uint8_t, const char*> filenameLanguageMap = {
    { NAME_LANGUAGE_PAL, "PAL" },
    { NAME_LANGUAGE_NTSC_JPN, "NTSC JPN" },
    { NAME_LANGUAGE_NTSC_ENG, "NTSC ENG" },
};

std::map<uint8_t, const char*> filenameLanguageMapNTSCOnly = {
    { NAME_LANGUAGE_NTSC_JPN, "NTSC JPN" },
    { NAME_LANGUAGE_NTSC_ENG, "NTSC ENG" },
};

#ifdef COMBO_BUILD
// ComboShip: the General tab, laid out as the MM Save Editor's: Player, Health, Magic, Rupees and Time cards with the
// same coloured bars and Max/Reset buttons, then everything else under a collapsed Advanced card. Max and Reset do
// what MM's do for the same resource, with OoT's new-file values.
void DrawInfoTab() {
    namespace Kit = ComboSaveEditorKit;
    const ImVec4 theme = ColorValues.at(THEME_COLOR);
    Kit::BeginCardLayout();

    Kit::BeginCard("playerCard");
    if (gSaveContext.fileNum >= 0 && gSaveContext.fileNum <= 2) {
        // Read-only: editing it sent the next save to another slot, which could overwrite another file or split this
        // file's OoT half from its MM half.
        ImGui::Text("File %d", gSaveContext.fileNum + 1);
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Debug File");
    }
    ImGui::Text("Player Name");
    if (gSaveContext.ship.filenameLanguage == NAME_LANGUAGE_PAL) {
        // A text field, as in MM.
        static char nameBuffer[9];
        for (int i = 0; i < 8; i++) {
            nameBuffer[i] = z2ASCII(gSaveContext.playerName[i]);
        }
        nameBuffer[8] = '\0';
        PushStyleInput(THEME_COLOR);
        if (ImGui::InputText("##playerName", nameBuffer, sizeof(nameBuffer), ImGuiInputTextFlags_AutoSelectAll)) {
            bool ended = false;
            for (int i = 0; i < 8; i++) {
                ended = ended || nameBuffer[i] == '\0';
                gSaveContext.playerName[i] = ASCII2z(ended ? ' ' : nameBuffer[i]);
            }
        }
        PopStyleInput();
    } else {
        // NTSC names hold Japanese characters a keyboard cannot type: keep the character codes.
        std::string name;
        for (int i = 0; i < 8; i++) {
            name += decodeNTSCPlayerNameChar(gSaveContext.playerName[i]);
        }
        ImGui::PushFont(OTRGlobals::Instance->fontJapanese);
        ImGui::Text("%s", name.c_str());
        ImGui::PopFont();
        ImU16 one = 1;
        ImGui::PushItemWidth(ImGui::GetFontSize() * 6);
        std::string nameID;
        for (int i = 0; i < 8; i++) {
            nameID = z2ASCII(i);
            if (i % 4 != 0) {
                ImGui::SameLine();
            }
            PushStyleInput(THEME_COLOR);
            ImGui::InputScalar(nameID.c_str(), ImGuiDataType_U8, &gSaveContext.playerName[i], &one, NULL);
            PopStyleInput();
        }
        ImGui::PopItemWidth();
    }
    // Filename encoding
    const bool hasPAL = (sGerMessageEntryTablePtr != nullptr) && (sFraMessageEntryTablePtr != nullptr);
    const bool hasNTSC = (sJpnMessageEntryTablePtr != nullptr);
    if (hasPAL && hasNTSC) {
        // Full
        Combobox("Player Name Language", &gSaveContext.ship.filenameLanguage, filenameLanguageMap,
                 comboboxOptionsBase.Tooltip("Encoding used for Player Name"));
    } else if (hasNTSC && (gSaveContext.ship.filenameLanguage != NAME_LANGUAGE_PAL)) {
        // NTSC only
        Combobox("Player Name Language", &gSaveContext.ship.filenameLanguage, filenameLanguageMapNTSCOnly,
                 comboboxOptionsBase.Tooltip("Encoding used for Player Name"));
    } else {
        // PAL only (read only)
        ImGui::BeginDisabled();
        Combobox("Player Name Language", &gSaveContext.ship.filenameLanguage, filenameLanguageMap,
                 comboboxOptionsBase.Tooltip("Encoding used for Player Name"));
        ImGui::EndDisabled();
    }
    Kit::EndCard();

    Kit::BeginCard("healthCard");
    if (Kit::Button("Max Health", Kit::kHealthMax)) {
        gSaveContext.isDoubleDefenseAcquired = 1;
        gSaveContext.inventory.defenseHearts = 20;
        gSaveContext.healthCapacity = gSaveContext.health = 20 * 16;
        gSaveContext.inventory.questItems &= ~0xF0000000;
    }
    ImGui::SameLine();
    if (Kit::Button("Reset##health", Kit::kReset)) {
        gSaveContext.isDoubleDefenseAcquired = 0;
        gSaveContext.inventory.defenseHearts = 0;
        gSaveContext.healthCapacity = gSaveContext.health = 3 * 16;
        gSaveContext.inventory.questItems &= ~0xF0000000;
    }
    ImGui::SameLine();
    bool isDoubleDefenseAcquired = gSaveContext.isDoubleDefenseAcquired != 0;
    if (Checkbox("Double Defense", &isDoubleDefenseAcquired,
                 CheckboxOptions().Color(Colors::Red).Tooltip("Is double defense unlocked?"))) {
        gSaveContext.isDoubleDefenseAcquired = isDoubleDefenseAcquired;
        gSaveContext.inventory.defenseHearts =
            gSaveContext.isDoubleDefenseAcquired ? 20 : 0; // Set to get the border drawn in the UI
    }
    int16_t hearts = gSaveContext.healthCapacity / 16;
    if (Kit::ResourceBar("##maxHearts", &hearts, 1, 20, "Max hearts: %d", Kit::kHealth)) {
        gSaveContext.healthCapacity = static_cast<int16_t>(hearts * 16);
    }
    if (gSaveContext.health > gSaveContext.healthCapacity) {
        gSaveContext.health = gSaveContext.healthCapacity; // Clamp health to new max
    }
    Kit::ResourceBar("##health", &gSaveContext.health, 0, gSaveContext.healthCapacity, "Health: %d", Kit::kHealth);
    Kit::Tooltip("16 units per heart");
    Kit::EndCard();

    Kit::BeginCard("magicCard");
    if (Kit::Button("Max Magic", Kit::kMagic)) {
        gSaveContext.magicLevel = MAGIC_LEVEL_DOUBLE;
        gSaveContext.isMagicAcquired = true;
        gSaveContext.isDoubleMagicAcquired = true;
        gSaveContext.magic = MAGIC_LEVEL_DOUBLE * 0x30;
    }
    ImGui::SameLine();
    if (Kit::Button("Reset##magic", Kit::kReset)) {
        gSaveContext.magicLevel = MAGIC_LEVEL_NONE;
        gSaveContext.isMagicAcquired = false;
        gSaveContext.isDoubleMagicAcquired = false;
        gSaveContext.magic = 0;
    }
    const int magicLevel = std::clamp<int>(gSaveContext.magicLevel, MAGIC_LEVEL_NONE, MAGIC_LEVEL_DOUBLE);
    if (Kit::ResourceBar("##magicLevel", &gSaveContext.magicLevel, MAGIC_LEVEL_NONE, MAGIC_LEVEL_DOUBLE,
                         Kit::kMagicLevelNames[magicLevel], Kit::kMagic)) {
        gSaveContext.isMagicAcquired = gSaveContext.magicLevel > 0;
        gSaveContext.isDoubleMagicAcquired = gSaveContext.magicLevel == 2;
    }
    gSaveContext.magicCapacity = gSaveContext.magicLevel * 0x30; // Set to get the bar drawn in the UI
    if (gSaveContext.magic > gSaveContext.magicCapacity) {
        gSaveContext.magic = static_cast<s8>(gSaveContext.magicCapacity); // Clamp magic to new max
    }
    Kit::ResourceBar("##magic", &gSaveContext.magic, 0, gSaveContext.magicCapacity, "Magic: %d", Kit::kMagic);
    Kit::Tooltip("48 units per magic level");
    Kit::EndCard();

    Kit::BeginCard("rupeeCard");
    // Tycoon's Wallet only when the seed has it.
    const int walletMax =
        IS_RANDO && OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_INCLUDE_TYCOON_WALLET) ? 3 : 2;
    if (Kit::Button("Max Rupees", Kit::kRupees)) {
        Inventory_ChangeUpgrade(UPG_WALLET, walletMax);
        gSaveContext.rupees = CUR_CAPACITY(UPG_WALLET);
    }
    ImGui::SameLine();
    if (Kit::Button("Reset##rupees", Kit::kReset)) {
        gSaveContext.rupees = 0;
        Inventory_ChangeUpgrade(UPG_WALLET, 0);
    }
    int wallet = CUR_UPG_VALUE(UPG_WALLET);
    if (Kit::ResourceBar("##wallet", &wallet, 0, walletMax, wallet < 4 ? Kit::kWalletNames[wallet] : "Glitched",
                         Kit::kRupees)) {
        Inventory_ChangeUpgrade(UPG_WALLET, wallet);
        gSaveContext.rupees = std::min<int16_t>(gSaveContext.rupees, CUR_CAPACITY(UPG_WALLET));
    }
    Kit::ResourceBar("##rupees", &gSaveContext.rupees, 0, CUR_CAPACITY(UPG_WALLET), "Rupees: %d", Kit::kRupees);
    Kit::EndCard();

    Kit::BeginCard("timeCard");
    ImGui::Text("Time of Day");
    Kit::ResourceBar("##time", &gSaveContext.dayTime, 0, 0xFFFF, Kit::ClockText(gSaveContext.dayTime).c_str(), theme);
    const std::pair<const char*, uint16_t> times[] = {
        { "Dawn", 0x4000 }, { "Noon", 0x8000 }, { "Sunset", 0xC001 }, { "Midnight", 0 }
    };
    const float timeButtonWidth = (ImGui::GetContentRegionAvail().x - 3 * ImGui::GetStyle().ItemSpacing.x) / 4;
    for (size_t i = 0; i < 4; i++) {
        if (i != 0) {
            ImGui::SameLine();
        }
        if (Kit::Button(times[i].first, theme, ImVec2(timeButtonWidth, 0))) {
            gSaveContext.dayTime = times[i].second;
        }
    }
    Kit::EndCard();

    // Everything else, nothing removed: counters, engine state, minigame records. The Biggoron's Sword quest is
    // on Equipment and the Triforce pieces on Quest Status.
    Kit::BeginCard("advancedCard");
    if (ImGui::CollapsingHeader("Advanced")) {
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
        PushStyleInput(THEME_COLOR);
        ComboTouch::Count("Total Days", &gSaveContext.totalDays, 0, 1000);
        Tooltip("Total number of days elapsed since the start of the game");
        PopStyleInput();

        PushStyleInput(THEME_COLOR);
        ComboTouch::Count("Deaths", &gSaveContext.deaths, 0, 65535);
        Tooltip("Total number of deaths");
        PopStyleInput();

        PushStyleInput(THEME_COLOR);
        ImGui::InputScalar("Entrance Index", ImGuiDataType_S32, &gSaveContext.entranceIndex);
        Tooltip("From which entrance did Link arrive?");
        PopStyleInput();

        PushStyleInput(THEME_COLOR);
        ImGui::InputScalar("Cutscene Index", ImGuiDataType_S32, &gSaveContext.cutsceneIndex);
        Tooltip("Which cutscene is this?");
        PopStyleInput();

        PushStyleInput(THEME_COLOR);
        ComboTouch::Count("Navi Timer", &gSaveContext.naviTimer, 0, 3000);
        Tooltip("Navi wants to talk at 600 units, decides not to at 3000.");
        PopStyleInput();

        PushStyleInput(THEME_COLOR);
        ComboTouch::Count("Timer State", &gSaveContext.timerState, 0, 16);
        Tooltip("Heat timer, race timer, etc. Has white font");
        PopStyleInput();

        PushStyleInput(THEME_COLOR);
        ComboTouch::Count("Timer Seconds", &gSaveContext.timerSeconds, 0, 32767);
        Tooltip("Time, in seconds");
        PopStyleInput();

        PushStyleInput(THEME_COLOR);
        ComboTouch::Count("Sub-Timer State", &gSaveContext.subTimerState, 0, 16);
        Tooltip("Trade timer, Ganon collapse timer, etc. Has yellow font");
        PopStyleInput();

        PushStyleInput(THEME_COLOR);
        ComboTouch::Count("Sub-Timer Seconds", &gSaveContext.subTimerSeconds, 0, 32767);
        Tooltip("Time, in seconds");
        PopStyleInput();

        Combobox("Audio", &gSaveContext.audioSetting, audioMap, comboboxOptionsBase.Tooltip("Sound setting"));

        Checkbox("64 DD file?", (bool*)&gSaveContext.n64ddFlag,
                 checkboxOptionsBase.Tooltip("WARNING! If you save, your file may be locked! Use caution!"));

        Combobox("Z Target Mode", &gSaveContext.zTargetSetting, zTargetMap,
                 comboboxOptionsBase.Tooltip("Z-Targeting behavior"));

        static std::array<const char*, 7> minigameHS = { "Horseback Archery", "Big Poe Points",
                                                         "Fishing",           "Malon's Obstacle Course",
                                                         "Running Man Race",  "?",
                                                         "Dampe's Race" };

        if (ImGui::TreeNode("Minigames")) {
            for (int i = 0; i < 7; i++) {
                if (i == 2 && ImGui::TreeNode("Fishing")) { // fishing has a few more flags to it
                    u8 fishSize = gSaveContext.highScores[i] & 0x7F;
                    PushStyleInput(THEME_COLOR);
                    if (ComboTouch::Count("Child Size Record", &fishSize, 0, 127)) {
                        gSaveContext.highScores[i] &= ~0x7F;
                        gSaveContext.highScores[i] |= fishSize & 0x7F;
                    }
                    char fishMsg[64];
                    std::snprintf(fishMsg, 64, "Weight: %2.0f lbs", ((SQ(fishSize) * .0036) + .5));
                    Tooltip(fishMsg);
                    PopStyleInput();
                    bool FishBool = gSaveContext.highScores[i] & 0x80;
                    if (Checkbox("Cheated as Child", &FishBool,
                                 checkboxOptionsBase.Tooltip("Used the Sinking lure to catch it."))) {
                        gSaveContext.highScores[i] &= ~0x80;
                        gSaveContext.highScores[i] |= (0x80 * FishBool);
                    }
                    fishSize = (gSaveContext.highScores[i] & 0x7F000000) >> 0x18;
                    PushStyleInput(THEME_COLOR);
                    if (ComboTouch::Count("Adult Size Record", &fishSize, 0, 127)) {
                        gSaveContext.highScores[i] &= ~0x7F000000;
                        gSaveContext.highScores[i] |= (fishSize & 0x7F) << 0x18;
                    }
                    std::snprintf(fishMsg, 64, "Weight: %2.0f lbs", ((SQ(fishSize) * .0036) + .5));
                    Tooltip(fishMsg);
                    PopStyleInput();
                    FishBool = gSaveContext.highScores[i] & 0x80000000;
                    if (Checkbox("Cheated as Adult", &FishBool,
                                 checkboxOptionsBase.Tooltip("Used the Sinking lure to catch it."))) {
                        gSaveContext.highScores[i] &= ~0x80000000;
                        gSaveContext.highScores[i] |= (0x80000000 * FishBool);
                    }
                    FishBool = gSaveContext.highScores[i] & 0x100;
                    if (Checkbox("Played as Child", &FishBool,
                                 checkboxOptionsBase.Tooltip("Played at least one game as a child"))) {
                        gSaveContext.highScores[i] &= ~0x100;
                        gSaveContext.highScores[i] |= (0x100 * FishBool);
                    }
                    FishBool = gSaveContext.highScores[i] & 0x200;
                    if (Checkbox("Played as Adult", &FishBool,
                                 checkboxOptionsBase.Tooltip("Played at least one game as an adult"))) {
                        gSaveContext.highScores[i] &= ~0x200;
                        gSaveContext.highScores[i] |= (0x200 * FishBool);
                    }
                    FishBool = gSaveContext.highScores[i] & 0x400;
                    if (Checkbox("Got Prize as Child", &FishBool,
                                 checkboxOptionsBase.Tooltip("Got the prize item (Heart Piece, unless rando.)\nunlocks "
                                                             "Sinking Lure for Child Link."))) {
                        gSaveContext.highScores[i] &= ~0x400;
                        gSaveContext.highScores[i] |= (0x400 * FishBool);
                    }
                    FishBool = gSaveContext.highScores[i] & 0x800;
                    if (Checkbox(
                            "Got Prize as Adult", &FishBool,
                            checkboxOptionsBase.Tooltip("Got the prize item (Golden Scale, unless rando.)\nUnlocks "
                                                        "Sinking Lure for Adult Link."))) {
                        gSaveContext.highScores[i] &= ~0x800;
                        gSaveContext.highScores[i] |= (0x800 * FishBool);
                    }
                    FishBool = gSaveContext.highScores[i] & 0x1000;
                    if (Checkbox("Stole Owner's Hat", &FishBool,
                                 checkboxOptionsBase.Tooltip("The owner's now visibly bald when Adult Link."))) {
                        gSaveContext.highScores[i] &= ~0x1000;
                        gSaveContext.highScores[i] |= (0x1000 * FishBool);
                    }
                    fishSize = (gSaveContext.highScores[i] & 0xFF0000) >> 16;
                    PushStyleInput(THEME_COLOR);
                    if (ComboTouch::Count("Times Played", &fishSize, 0, 255)) {
                        gSaveContext.highScores[i] &= ~0xFF0000;
                        gSaveContext.highScores[i] |= (fishSize) << 16;
                    }
                    Tooltip("Determines weather and school size during dawn/dusk.");
                    PopStyleInput();

                    ImGui::TreePop();
                    continue;
                }

                if (i == 5 || i == 2) { // HS_UNK_05 is unused
                    continue;
                }
                std::string minigameLbl = minigameHS[i];
                PushStyleInput(THEME_COLOR);
                ComboTouch::Count(minigameLbl.c_str(), &gSaveContext.highScores[i], 0, i == 1 ? 1000 : 99999);
                PopStyleInput();
            }

            ImGui::TreePop();
        }

        ImGui::PopItemWidth();
    }
    Kit::EndCard();

    Kit::EndCardLayout();
}
#else
void DrawInfoTab() {
    if (gSaveContext.gameMode == GAMEMODE_TITLE_SCREEN) {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Title Screen");
    } else if (gSaveContext.gameMode == GAMEMODE_FILE_SELECT) {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "File Select");
    } else if (gPlayState == nullptr) {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Game Inactive");
    } else if (gSaveContext.fileNum >= 0 && gSaveContext.fileNum <= 2) {
        Combobox("File Number", &gSaveContext.fileNum, fileNumMap, comboboxOptionsBase.Tooltip("Current File Number"));
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Debug File");
    }

    // TODO Needs a better method for name changing but for now this will work.
    std::string name;
    ImU16 one = 1;

    if (gSaveContext.ship.filenameLanguage == NAME_LANGUAGE_PAL) {
        for (int i = 0; i < 8; i++) {
            char letter = z2ASCII(gSaveContext.playerName[i]);
            name += letter;
        }
        name += '\0';
    } else {
        for (int i = 0; i < 8; i++) {
            name += decodeNTSCPlayerNameChar(gSaveContext.playerName[i]);
        }
        name += '\0';
    }

    ImGui::PushItemWidth(ImGui::GetFontSize() * 6);

    if (gSaveContext.ship.filenameLanguage == NAME_LANGUAGE_PAL) {
        ImGui::Text("Name: %s", name.c_str());
    } else {
        ImGui::PushFont(OTRGlobals::Instance->fontJapanese);
        ImGui::Text("Name: %s", name.c_str());
        ImGui::PopFont();
    }

    Tooltip("Player Name");
    std::string nameID;
    for (int i = 0; i < 8; i++) {
        nameID = z2ASCII(i);
        if (i % 4 != 0) {
            ImGui::SameLine();
        }
        PushStyleInput(THEME_COLOR);
        ImGui::InputScalar(nameID.c_str(), ImGuiDataType_U8, &gSaveContext.playerName[i], &one, NULL);
        PopStyleInput();
    }

    // Filename encoding
    const bool hasPAL = (sGerMessageEntryTablePtr != nullptr) && (sFraMessageEntryTablePtr != nullptr);
    const bool hasNTSC = (sJpnMessageEntryTablePtr != nullptr);
    if (hasPAL && hasNTSC) {
        // Full
        Combobox("Player Name Language", &gSaveContext.ship.filenameLanguage, filenameLanguageMap,
                 comboboxOptionsBase.Tooltip("Encoding used for Player Name"));
    } else if (hasNTSC && (gSaveContext.ship.filenameLanguage != NAME_LANGUAGE_PAL)) {
        // NTSC only
        Combobox("Player Name Language", &gSaveContext.ship.filenameLanguage, filenameLanguageMapNTSCOnly,
                 comboboxOptionsBase.Tooltip("Encoding used for Player Name"));
    } else {
        // PAL only (read only)
        ImGui::BeginDisabled();
        Combobox("Player Name Language", &gSaveContext.ship.filenameLanguage, filenameLanguageMap,
                 comboboxOptionsBase.Tooltip("Encoding used for Player Name"));
        ImGui::EndDisabled();
    }

    // Edit whole hearts with a positive lower bound; touch users never need a numeric keyboard.
    int healthIntermediary = gSaveContext.healthCapacity / 16;
    PushStyleInput(THEME_COLOR);
    if (ImGui::SliderInt("Max Health", &healthIntermediary, 1, 20, "%d hearts", ImGuiSliderFlags_NoInput))
        gSaveContext.healthCapacity = static_cast<int16_t>(healthIntermediary * 16);
    PopStyleInput();
    Tooltip("Maximum health in whole hearts");
    if (gSaveContext.health > gSaveContext.healthCapacity) {
        gSaveContext.health = gSaveContext.healthCapacity; // Clamp health to new max
    }
    int32_t health = (int32_t)gSaveContext.health;
    if (SliderInt("Health", &health,
                  intSliderOptionsBase.Tooltip("Current health. 16 units per full heart")
                      .Min(0)
                      .Max(gSaveContext.healthCapacity))) {
        gSaveContext.health = (int16_t)health;
    }

    bool isDoubleDefenseAcquired = gSaveContext.isDoubleDefenseAcquired != 0;
    if (Checkbox("Double Defense", &isDoubleDefenseAcquired,
                 checkboxOptionsBase.Tooltip("Is double defense unlocked?"))) {
        gSaveContext.isDoubleDefenseAcquired = isDoubleDefenseAcquired;
        gSaveContext.inventory.defenseHearts =
            gSaveContext.isDoubleDefenseAcquired ? 20 : 0; // Set to get the border drawn in the UI
    }
    if (Combobox("Magic Level", &gSaveContext.magicLevel, magicLevelMap,
                 comboboxOptionsBase.Tooltip("Current magic level"))) {
        gSaveContext.isMagicAcquired = gSaveContext.magicLevel > 0;
        gSaveContext.isDoubleMagicAcquired = gSaveContext.magicLevel == 2;
    }
    gSaveContext.magicCapacity = gSaveContext.magicLevel * 0x30; // Set to get the bar drawn in the UI
    if (gSaveContext.magic > gSaveContext.magicCapacity) {
        gSaveContext.magic = static_cast<s8>(gSaveContext.magicCapacity); // Clamp magic to new max
    }

    int32_t magic = (int32_t)gSaveContext.magic;
    if (SliderInt("Magic", &magic,
                  intSliderOptionsBase.Min(0)
                      .Max(gSaveContext.magicCapacity)
                      .Tooltip("Current magic. 48 units per magic level"))) {
        gSaveContext.magic = (int8_t)magic;
    }

    PushStyleInput(THEME_COLOR);
    int rupees = gSaveContext.rupees;
    if (ImGui::SliderInt("Rupees", &rupees, 0, CUR_CAPACITY(UPG_WALLET), "%d", ImGuiSliderFlags_NoInput))
        gSaveContext.rupees = static_cast<int16_t>(rupees);
    Tooltip("Current rupees");
    PopStyleInput();

    int32_t dayTime = gSaveContext.dayTime;
    if (SliderInt("Time", &dayTime, intSliderOptionsBase.Min(0).Max(0xFFFF).Tooltip("Time of day")))
        gSaveContext.dayTime = static_cast<uint16_t>(dayTime);
    if (Button("Dawn", buttonOptionsBase)) {
        gSaveContext.dayTime = 0x4000;
    }
    ImGui::SameLine();
    if (Button("Noon", buttonOptionsBase)) {
        gSaveContext.dayTime = 0x8000;
    }
    ImGui::SameLine();
    if (Button("Sunset", buttonOptionsBase)) {
        gSaveContext.dayTime = 0xC001;
    }
    ImGui::SameLine();
    if (Button("Midnight", buttonOptionsBase)) {
        gSaveContext.dayTime = 0;
    }

    PushStyleInput(THEME_COLOR);
    ComboTouch::Count("Total Days", &gSaveContext.totalDays, 0, 1000);
    Tooltip("Total number of days elapsed since the start of the game");
    PopStyleInput();

    PushStyleInput(THEME_COLOR);
    ComboTouch::Count("Deaths", &gSaveContext.deaths, 0, 65535);
    Tooltip("Total number of deaths");
    PopStyleInput();

    Checkbox("Has BGS", (bool*)&gSaveContext.bgsFlag,
             checkboxOptionsBase.Tooltip("Is Biggoron sword unlocked? Replaces Giant's knife"));

    PushStyleInput(THEME_COLOR);
    ComboTouch::Count("Sword Health", &gSaveContext.swordHealth, 0, 255);
    Tooltip("Giant's knife health. Default is 8. Must be >0 for Biggoron sword to work");
    PopStyleInput();

    PushStyleInput(THEME_COLOR);
    ComboTouch::Count("Bgs Day Count", &gSaveContext.bgsDayCount, 0, 1000);
    Tooltip("Total number of days elapsed since receiving claim check from Biggoron");
    PopStyleInput();

    PushStyleInput(THEME_COLOR);
    ImGui::InputScalar("Entrance Index", ImGuiDataType_S32, &gSaveContext.entranceIndex);
    Tooltip("From which entrance did Link arrive?");
    PopStyleInput();

    PushStyleInput(THEME_COLOR);
    ImGui::InputScalar("Cutscene Index", ImGuiDataType_S32, &gSaveContext.cutsceneIndex);
    Tooltip("Which cutscene is this?");
    PopStyleInput();

    PushStyleInput(THEME_COLOR);
    ComboTouch::Count("Navi Timer", &gSaveContext.naviTimer, 0, 3000);
    Tooltip("Navi wants to talk at 600 units, decides not to at 3000.");
    PopStyleInput();

    PushStyleInput(THEME_COLOR);
    ComboTouch::Count("Timer State", &gSaveContext.timerState, 0, 16);
    Tooltip("Heat timer, race timer, etc. Has white font");
    PopStyleInput();

    PushStyleInput(THEME_COLOR);
    ComboTouch::Count("Timer Seconds", &gSaveContext.timerSeconds, 0, 32767);
    Tooltip("Time, in seconds");
    PopStyleInput();

    PushStyleInput(THEME_COLOR);
    ComboTouch::Count("Sub-Timer State", &gSaveContext.subTimerState, 0, 16);
    Tooltip("Trade timer, Ganon collapse timer, etc. Has yellow font");
    PopStyleInput();

    PushStyleInput(THEME_COLOR);
    ComboTouch::Count("Sub-Timer Seconds", &gSaveContext.subTimerSeconds, 0, 32767);
    Tooltip("Time, in seconds");
    PopStyleInput();

    Combobox("Audio", &gSaveContext.audioSetting, audioMap, comboboxOptionsBase.Tooltip("Sound setting"));

    Checkbox("64 DD file?", (bool*)&gSaveContext.n64ddFlag,
             checkboxOptionsBase.Tooltip("WARNING! If you save, your file may be locked! Use caution!"));

    Combobox("Z Target Mode", &gSaveContext.zTargetSetting, zTargetMap,
             comboboxOptionsBase.Tooltip("Z-Targeting behavior"));

    if (IS_RANDO && (OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_TRIFORCE_HUNT_PIECES_TOTAL) > 0)) {
        PushStyleInput(THEME_COLOR);
        ComboTouch::Count("Triforce Pieces", &gSaveContext.ship.quest.data.randomizer.triforcePiecesCollected, 0, 255);
        Tooltip("Currently obtained Triforce Pieces. For Triforce Hunt.");
        PopStyleInput();
    }

    ImGui::PushItemWidth(ImGui::GetFontSize() * 10);
    static std::array<const char*, 7> minigameHS = { "Horseback Archery", "Big Poe Points",
                                                     "Fishing",           "Malon's Obstacle Course",
                                                     "Running Man Race",  "?",
                                                     "Dampe's Race" };

    if (ImGui::TreeNode("Minigames")) {
        for (int i = 0; i < 7; i++) {
            if (i == 2 && ImGui::TreeNode("Fishing")) { // fishing has a few more flags to it
                u8 fishSize = gSaveContext.highScores[i] & 0x7F;
                PushStyleInput(THEME_COLOR);
                if (ComboTouch::Count("Child Size Record", &fishSize, 0, 127)) {
                    gSaveContext.highScores[i] &= ~0x7F;
                    gSaveContext.highScores[i] |= fishSize & 0x7F;
                }
                char fishMsg[64];
                std::snprintf(fishMsg, 64, "Weight: %2.0f lbs", ((SQ(fishSize) * .0036) + .5));
                Tooltip(fishMsg);
                PopStyleInput();
                bool FishBool = gSaveContext.highScores[i] & 0x80;
                if (Checkbox("Cheated as Child", &FishBool,
                             checkboxOptionsBase.Tooltip("Used the Sinking lure to catch it."))) {
                    gSaveContext.highScores[i] &= ~0x80;
                    gSaveContext.highScores[i] |= (0x80 * FishBool);
                }
                fishSize = (gSaveContext.highScores[i] & 0x7F000000) >> 0x18;
                PushStyleInput(THEME_COLOR);
                if (ComboTouch::Count("Adult Size Record", &fishSize, 0, 127)) {
                    gSaveContext.highScores[i] &= ~0x7F000000;
                    gSaveContext.highScores[i] |= (fishSize & 0x7F) << 0x18;
                }
                std::snprintf(fishMsg, 64, "Weight: %2.0f lbs", ((SQ(fishSize) * .0036) + .5));
                Tooltip(fishMsg);
                PopStyleInput();
                FishBool = gSaveContext.highScores[i] & 0x80000000;
                if (Checkbox("Cheated as Adult", &FishBool,
                             checkboxOptionsBase.Tooltip("Used the Sinking lure to catch it."))) {
                    gSaveContext.highScores[i] &= ~0x80000000;
                    gSaveContext.highScores[i] |= (0x80000000 * FishBool);
                }
                FishBool = gSaveContext.highScores[i] & 0x100;
                if (Checkbox("Played as Child", &FishBool,
                             checkboxOptionsBase.Tooltip("Played at least one game as a child"))) {
                    gSaveContext.highScores[i] &= ~0x100;
                    gSaveContext.highScores[i] |= (0x100 * FishBool);
                }
                FishBool = gSaveContext.highScores[i] & 0x200;
                if (Checkbox("Played as Adult", &FishBool,
                             checkboxOptionsBase.Tooltip("Played at least one game as an adult"))) {
                    gSaveContext.highScores[i] &= ~0x200;
                    gSaveContext.highScores[i] |= (0x200 * FishBool);
                }
                FishBool = gSaveContext.highScores[i] & 0x400;
                if (Checkbox(
                        "Got Prize as Child", &FishBool,
                        checkboxOptionsBase.Tooltip(
                            "Got the prize item (Heart Piece, unless rando.)\nunlocks Sinking Lure for Child Link."))) {
                    gSaveContext.highScores[i] &= ~0x400;
                    gSaveContext.highScores[i] |= (0x400 * FishBool);
                }
                FishBool = gSaveContext.highScores[i] & 0x800;
                if (Checkbox("Got Prize as Adult", &FishBool,
                             checkboxOptionsBase.Tooltip("Got the prize item (Golden Scale, unless rando.)\nUnlocks "
                                                         "Sinking Lure for Adult Link."))) {
                    gSaveContext.highScores[i] &= ~0x800;
                    gSaveContext.highScores[i] |= (0x800 * FishBool);
                }
                FishBool = gSaveContext.highScores[i] & 0x1000;
                if (Checkbox("Stole Owner's Hat", &FishBool,
                             checkboxOptionsBase.Tooltip("The owner's now visibly bald when Adult Link."))) {
                    gSaveContext.highScores[i] &= ~0x1000;
                    gSaveContext.highScores[i] |= (0x1000 * FishBool);
                }
                fishSize = (gSaveContext.highScores[i] & 0xFF0000) >> 16;
                PushStyleInput(THEME_COLOR);
                if (ComboTouch::Count("Times Played", &fishSize, 0, 255)) {
                    gSaveContext.highScores[i] &= ~0xFF0000;
                    gSaveContext.highScores[i] |= (fishSize) << 16;
                }
                Tooltip("Determines weather and school size during dawn/dusk.");
                PopStyleInput();

                ImGui::TreePop();
                continue;
            }

            if (i == 5 || i == 2) { // HS_UNK_05 is unused
                continue;
            }
            std::string minigameLbl = minigameHS[i];
            PushStyleInput(THEME_COLOR);
            ComboTouch::Count(minigameLbl.c_str(), &gSaveContext.highScores[i], 0, i == 1 ? 1000 : 99999);
            PopStyleInput();
        }

        ImGui::TreePop();
    }

    ImGui::PopItemWidth();
}
#endif

void DrawBGSItemFlag(uint8_t itemID) {
    const ItemMapEntry& slotEntry = itemMapping[itemID];
    ImGui::Image(std::dynamic_pointer_cast<Fast::Fast3dGui>(Ship::Context::GetRawInstance()->GetWindow()->GetGui())
                     ->GetTextureByName(slotEntry.name),
                 ImVec2(32.0f, 32.0f), ImVec2(0, 0), ImVec2(1, 1));
}

// Re-sync any C/D-pad button that mirrors an edited inventory slot (buttonItems[i] == items[cButtonSlots[i-1]]),
// so a raw item edit doesn't leave the button showing stale contents.
static void SyncButtonItemsForSlot(uint8_t slot) {
    for (size_t i = 1; i < ARRAY_COUNT(gSaveContext.equips.buttonItems); i++) {
        if (gSaveContext.equips.cButtonSlots[i - 1] == slot) {
            gSaveContext.equips.buttonItems[i] = gSaveContext.inventory.items[slot];
            if (gPlayState != nullptr) {
                Interface_LoadItemIcon1(gPlayState, static_cast<u16>(i));
            }
        }
    }
}

static std::vector<ItemMapEntry> GetLegalInventorySlotItems(InventorySlot slot) {
    std::vector<ItemMapEntry> items;
    // Every bottle can contain the same items even though gItemSlots maps them to bottle 1.
    const InventorySlot canonicalSlot = slot >= SLOT_BOTTLE_1 && slot <= SLOT_BOTTLE_4 ? SLOT_BOTTLE_1 : slot;
    for (int item = ITEM_STICK; item <= ITEM_CLAIM_CHECK; item++) {
        if (const auto mappedItem = itemMapping.find(item);
            gItemSlots[item] == canonicalSlot && mappedItem != itemMapping.end()) {
            items.push_back(mappedItem->second);
        }
    }
    return items;
}

#ifdef COMBO_BUILD
// ComboShip: the capacity upgrade an ammo item needs before it holds anything.
static int AmmoUpgradeForItem(uint8_t item) {
    switch (item) {
        case ITEM_STICK:
            return UPG_STICKS;
        case ITEM_NUT:
            return UPG_NUTS;
        case ITEM_BOMB:
            return UPG_BOMB_BAG;
        case ITEM_BOW:
            return UPG_QUIVER;
        case ITEM_SLINGSHOT:
            return UPG_BULLET_BAG;
        default:
            return -1;
    }
}

// ComboShip: the tap rule's toggle, for a slot that can hold only one item. On does exactly what picking that item
// does, plus capacity level 1 and full ammo when the item has no capacity yet (as MM's first tap does); off does
// exactly what picking None does.
static void ToggleInventorySlot(InventorySlot slot, uint8_t item, bool syncButtons) {
    if (gSaveContext.inventory.items[slot] == ITEM_NONE) {
        gSaveContext.inventory.items[slot] = item;
        const int upgrade = AmmoUpgradeForItem(item);
        if (upgrade >= 0 && CUR_UPG_VALUE(upgrade) == 0) {
            Inventory_ChangeUpgrade(upgrade, 1);
            AMMO(item) = CUR_CAPACITY(upgrade);
        }
    } else {
        gSaveContext.inventory.items[slot] = ITEM_NONE;
    }
    if (syncButtons) {
        SyncButtonItemsForSlot(slot);
    }
}

// ComboShip: Give all and Reset on the Inventory tab, as MM's: every slot gets the last item that fits it (the bottles
// an empty bottle, the child trade slot the Mask of Truth rather than Sold Out), the bags and capacities their largest
// size and full ammo. Reset empties every slot and its ammo and takes those bags back to none.
static void GiveAllInventoryItems(bool syncButtons) {
    for (int i = 0; i < 24; i++) {
        const InventorySlot slot = static_cast<InventorySlot>(i);
        if (slot >= SLOT_BOTTLE_1 && slot <= SLOT_BOTTLE_4) {
            gSaveContext.inventory.items[slot] = ITEM_BOTTLE;
        } else {
            auto items = GetLegalInventorySlotItems(slot);
            std::erase_if(items, [](const ItemMapEntry& entry) { return entry.id == ITEM_SOLD_OUT; });
            if (!items.empty()) {
                gSaveContext.inventory.items[slot] = items.back().id;
            }
        }
        if (syncButtons) {
            SyncButtonItemsForSlot(slot);
        }
    }
    for (int upgrade : { UPG_QUIVER, UPG_BOMB_BAG, UPG_BULLET_BAG, UPG_STICKS, UPG_NUTS }) {
        Inventory_ChangeUpgrade(upgrade, 3);
    }
    AMMO(ITEM_BOW) = CUR_CAPACITY(UPG_QUIVER);
    AMMO(ITEM_BOMB) = CUR_CAPACITY(UPG_BOMB_BAG);
    AMMO(ITEM_SLINGSHOT) = CUR_CAPACITY(UPG_BULLET_BAG);
    AMMO(ITEM_STICK) = CUR_CAPACITY(UPG_STICKS);
    AMMO(ITEM_NUT) = CUR_CAPACITY(UPG_NUTS);
    AMMO(ITEM_BOMBCHU) = 50;
    AMMO(ITEM_BEAN) = 10;
}

static void ResetInventoryItems(bool syncButtons) {
    for (int i = 0; i < 24; i++) {
        gSaveContext.inventory.items[i] = ITEM_NONE;
        if (syncButtons) {
            SyncButtonItemsForSlot(static_cast<InventorySlot>(i));
        }
    }
    for (int i = 0; i < 16; i++) {
        gSaveContext.inventory.ammo[i] = 0;
    }
    for (int upgrade : { UPG_QUIVER, UPG_BOMB_BAG, UPG_BULLET_BAG, UPG_STICKS, UPG_NUTS }) {
        Inventory_ChangeUpgrade(upgrade, 0);
    }
}
#endif

#ifdef COMBO_BUILD
// ComboShip: the Inventory tab as MM's. Give all, Reset and the switches sit on top; a slot that can hold one item
// toggles it, a slot that can hold several (Ocarina, Hookshot, bottles, trade items) opens a picker with None first and
// item names; each ammo count sits under its icon instead of in a separate Ammo row.
void DrawInventoryTab() {
    namespace Kit = ComboSaveEditorKit;
    static bool restrictToValid = true;
    static bool syncButtons = true;

    if (Kit::Button("Give all##items", Kit::kGiveAll)) {
        GiveAllInventoryItems(syncButtons);
    }
    ImGui::SameLine();
    if (Kit::Button("Reset##items", Kit::kReset)) {
        ResetInventoryItems(syncButtons);
    }
    ImGui::SameLine();
    Checkbox(Kit::kRestrictLabel, &restrictToValid, checkboxOptionsBase.Tooltip(Kit::kRestrictTooltip));
    ImGui::SameLine();
    Checkbox("Keep C/D-pad buttons in sync", &syncButtons,
             checkboxOptionsBase.Tooltip("Refresh a C or D-pad button when its inventory slot is edited. Disable to "
                                         "leave a slot and its button out of sync (e.g. to set up RBA)."));
    ImGui::TextWrapped("%s", Kit::kInventoryHelp);

    const ImVec4 theme = ColorValues.at(THEME_COLOR);
    auto gui = std::dynamic_pointer_cast<Fast::Fast3dGui>(Ship::Context::GetRawInstance()->GetWindow()->GetGui());
    static InventorySlot selectedIndex = SLOT_NONE;
    static const char* itemPopupPicker = "itemPopupPicker";
    ImGui::BeginChild("itemsBox", Kit::GridBoxSize(), ImGuiChildFlags_Borders);
    ImGui::Text("Items");
    static_assert(5 + 3 * 6 < sizeof(gSaveContext.inventory.items) / sizeof(gSaveContext.inventory.items[0]));
    for (int i = 0; i < 24; i++) {
        const InventorySlot index = static_cast<InventorySlot>(i);
        ImGui::PushID(index);

        ItemID item = (ItemID)gSaveContext.inventory.items[index];
        const auto legalItems = GetLegalInventorySlotItems(index);
        std::string textureName;
        std::string itemName;
        bool hasItemIcon = false;
        if (item == ITEM_ROCS_FEATHER) {
            textureName = "ROCS_FEATHER";
            itemName = "Roc's Feather";
            hasItemIcon = true;
        } else if (const auto mappedItem = itemMapping.find(item);
                   item != ITEM_NONE && mappedItem != itemMapping.end()) {
            textureName = mappedItem->second.name;
            itemName = SohUtils::GetItemName(item);
            hasItemIcon = true;
        } else if (!legalItems.empty()) {
            textureName = legalItems.front().name;
            itemName = SohUtils::GetItemName(legalItems.front().id);
        }
        const Kit::Tap tap = Kit::ToggleOrPick(restrictToValid, legalItems.size());

        // The ammo count under its icon; with the restriction off every ammo slot shows one.
        if (item != ITEM_NONE && i < 16 && (!restrictToValid || gAmmoItems[i] != ITEM_NONE)) {
            ImGui::SetCursorPos(Kit::GridAmmo(i));
            Kit::AmmoCount("##ammoInput", &gSaveContext.inventory.ammo[i], theme);
        }

        ImGui::SetCursorPos(Kit::GridCell(i));
        bool pressed;
        if (!textureName.empty()) {
            pressed = Kit::IconButton("##inventorySlot", gui->GetTextureByName(textureName), hasItemIcon);
            std::string tooltip = itemName + (hasItemIcon ? " (owned)" : " (not owned)");
            if (!hasItemIcon && item != ITEM_NONE) {
                tooltip = "Unsupported saved item " + std::to_string(static_cast<int>(item));
            }
            tooltip += std::string("\n") + Kit::TapHint(tap);
            UIWidgets::Tooltip(tooltip.c_str());
        } else {
            pressed = Kit::EmptyIconButton("##itemNone");
        }
        if (pressed) {
            if (tap == Kit::Tap::Toggle) {
                ToggleInventorySlot(index, legalItems.front().id, syncButtons);
            } else {
                selectedIndex = index;
                ImGui::OpenPopup(itemPopupPicker);
            }
        }

        if (ImGui::BeginPopup(itemPopupPicker)) {
            if (Kit::PickerEntry(0, "##itemNonePicker", 0, "None")) {
                if (selectedIndex != SLOT_NONE) {
                    gSaveContext.inventory.items[selectedIndex] = ITEM_NONE;
                    if (syncButtons) {
                        SyncButtonItemsForSlot(selectedIndex);
                    }
                }
                ImGui::CloseCurrentPopup();
            }

            std::vector<ItemMapEntry> possibleItems;
            if (restrictToValid) {
                possibleItems = GetLegalInventorySlotItems(selectedIndex);
            } else {
                for (const auto& entry : itemMapping) {
                    possibleItems.push_back(entry.second);
                }
            }
            for (size_t pickerIndex = 0; pickerIndex < possibleItems.size(); pickerIndex++) {
                const ItemMapEntry& slotEntry = possibleItems[pickerIndex];
                if (Kit::PickerEntry(static_cast<int>(pickerIndex) + 1, slotEntry.name.c_str(),
                                     gui->GetTextureByName(slotEntry.name),
                                     SohUtils::GetItemName(slotEntry.id).c_str())) {
                    gSaveContext.inventory.items[selectedIndex] = slotEntry.id;
                    if (syncButtons) {
                        SyncButtonItemsForSlot(selectedIndex);
                    }
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::EndPopup();
        }

        ImGui::PopID();
    }
    ImGui::EndChild();

    // Trade quest flags are only used when shuffling the trade sequence, so
    // don't show this if it isn't needed.
    if (IS_RANDO && OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHUFFLE_ADULT_TRADE) &&
        ImGui::TreeNode("Adult trade quest items")) {
        for (int i = ITEM_POCKET_EGG; i <= ITEM_CLAIM_CHECK; i++) {
            DrawBGSItemFlag(i);
        }
        ImGui::TreePop();
    }
}
#else
void DrawInventoryTab() {
    static bool restrictToValid = true;

    Checkbox(
        "Restrict to valid items", &restrictToValid,
        checkboxOptionsBase.Tooltip("Restricts items and ammo to only what is possible to legally acquire in-game"));

    static bool syncButtons = true;
    Checkbox("Keep C/D-pad buttons in sync", &syncButtons,
             checkboxOptionsBase.Tooltip("Refresh a C or D-pad button when its inventory slot is edited. Disable to "
                                         "leave a slot and its button out of sync (e.g. to set up RBA)."));
    ImGui::TextWrapped("Faded icons are not owned. Tap an icon to choose an item or remove it.");

    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 6; x++) {
            static_assert(5 + 3 * 6 < sizeof(gSaveContext.inventory.items) / sizeof(gSaveContext.inventory.items[0]));
            InventorySlot index = static_cast<InventorySlot>(x + y * 6);
            static InventorySlot selectedIndex = SLOT_NONE;
            static const char* itemPopupPicker = "itemPopupPicker";

            ImGui::PushID(index);

            if (x != 0) {
                ImGui::SameLine();
            }

            ItemID item = (ItemID)gSaveContext.inventory.items[index];
            std::string textureName;
            std::string itemName;
            bool hasItemIcon = false;
            if (item == ITEM_ROCS_FEATHER) {
                textureName = "ROCS_FEATHER";
                itemName = "Roc's Feather";
                hasItemIcon = true;
            } else if (const auto mappedItem = itemMapping.find(item);
                       item != ITEM_NONE && mappedItem != itemMapping.end()) {
                textureName = mappedItem->second.name;
                itemName = SohUtils::GetItemName(item);
                hasItemIcon = true;
            } else {
                const auto legalItems = GetLegalInventorySlotItems(index);
                if (!legalItems.empty()) {
                    textureName = legalItems.front().name;
                    itemName = SohUtils::GetItemName(legalItems.front().id);
                }
            }
            PushStyleButton(Colors::DarkGray);
            if (!textureName.empty()) {
                auto ret = ImGui::ImageButton(
                    "##inventorySlot",
                    std::dynamic_pointer_cast<Fast::Fast3dGui>(Ship::Context::GetRawInstance()->GetWindow()->GetGui())
                        ->GetTextureByName(textureName),
                    ImVec2(IMAGE_SIZE, IMAGE_SIZE), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0),
                    ImVec4(1, 1, 1, hasItemIcon ? 1.0f : 0.4f));
                if (ret) {
                    selectedIndex = index;
                    ImGui::OpenPopup(itemPopupPicker);
                }
                std::string tooltip = itemName + (hasItemIcon ? " (owned)" : " (not owned)");
                if (!hasItemIcon && item != ITEM_NONE) {
                    tooltip = "Unsupported saved item " + std::to_string(static_cast<int>(item));
                }
                tooltip += "\nTap to choose an item or remove it.";
                UIWidgets::Tooltip(tooltip.c_str());
            } else {
                if (ImGui::Button("##itemNone", ImVec2(IMAGE_SIZE, IMAGE_SIZE) + ImGui::GetStyle().FramePadding * 2)) {
                    selectedIndex = index;
                    ImGui::OpenPopup(itemPopupPicker);
                }
            }
            PopStyleButton();

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
            if (ImGui::BeginPopup(itemPopupPicker)) {
                PushStyleButton(Colors::DarkGray);
                if (ImGui::Button("##itemNonePicker",
                                  ImVec2(IMAGE_SIZE, IMAGE_SIZE) + ImGui::GetStyle().FramePadding * 2)) {
                    if (selectedIndex != SLOT_NONE) {
                        gSaveContext.inventory.items[selectedIndex] = ITEM_NONE;
                        if (syncButtons) {
                            SyncButtonItemsForSlot(selectedIndex);
                        }
                    }
                    ImGui::CloseCurrentPopup();
                }
                PopStyleButton();
                UIWidgets::Tooltip("None");

                std::vector<ItemMapEntry> possibleItems;
                if (restrictToValid) {
                    possibleItems = GetLegalInventorySlotItems(selectedIndex);
                } else {
                    for (const auto& entry : itemMapping) {
                        possibleItems.push_back(entry.second);
                    }
                }

                for (size_t pickerIndex = 0; pickerIndex < possibleItems.size(); pickerIndex++) {
                    if (((pickerIndex + 1) % 8) != 0) {
                        ImGui::SameLine();
                    }
                    const ItemMapEntry& slotEntry = possibleItems[pickerIndex];
                    PushStyleButton(Colors::DarkGray);
                    auto ret = ImGui::ImageButton(slotEntry.name.c_str(),
                                                  std::dynamic_pointer_cast<Fast::Fast3dGui>(
                                                      Ship::Context::GetRawInstance()->GetWindow()->GetGui())
                                                      ->GetTextureByName(slotEntry.name),
                                                  ImVec2(IMAGE_SIZE, IMAGE_SIZE), ImVec2(0, 0), ImVec2(1, 1));
                    PopStyleButton();
                    if (ret) {
                        gSaveContext.inventory.items[selectedIndex] = slotEntry.id;
                        if (syncButtons) {
                            SyncButtonItemsForSlot(selectedIndex);
                        }
                        ImGui::CloseCurrentPopup();
                    }
                    UIWidgets::Tooltip(SohUtils::GetItemName(slotEntry.id).c_str());
                }

                ImGui::EndPopup();
            }
            ImGui::PopStyleVar();

            ImGui::PopID();
        }
    }

    ImGui::Text("Ammo");
    for (uint32_t ammoIndex = 0, drawnAmmoItems = 0; ammoIndex < 16; ammoIndex++) {
        uint8_t item = (restrictToValid) ? gAmmoItems[ammoIndex] : gAllAmmoItems[ammoIndex];
        if (item != ITEM_NONE) {
            // For legal items, display as 1 row of 7. For unrestricted items, display rows of 6 to match
            // inventory
            if ((restrictToValid && (drawnAmmoItems != 0)) || ((drawnAmmoItems % 6) != 0)) {
                ImGui::SameLine();
            }
            drawnAmmoItems++;

            ImGui::PushID(ammoIndex);
            ImGui::PushItemWidth(IMAGE_SIZE);
            ImGui::BeginGroup();

            ImGui::Image(
                std::dynamic_pointer_cast<Fast::Fast3dGui>(Ship::Context::GetRawInstance()->GetWindow()->GetGui())
                    ->GetTextureByName(itemMapping[item].name),
                ImVec2(IMAGE_SIZE, IMAGE_SIZE));
            PushStyleInput(THEME_COLOR);
            ComboTouch::Count("##ammoInput", &AMMO(item), 0, 99);
            PopStyleInput();

            ImGui::EndGroup();
            ImGui::PopItemWidth();
            ImGui::PopID();
        }
    }

    // Trade quest flags are only used when shuffling the trade sequence, so
    // don't show this if it isn't needed.
    if (IS_RANDO && OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHUFFLE_ADULT_TRADE) &&
        ImGui::TreeNode("Adult trade quest items")) {
        for (int i = ITEM_POCKET_EGG; i <= ITEM_CLAIM_CHECK; i++) {
            DrawBGSItemFlag(i);
        }
        ImGui::TreePop();
    }
}
#endif

// Draw a flag bitfield as a grid of checkboxes
void DrawFlagTableArray16(const FlagTable& flagTable, uint16_t row, uint16_t& flags) {
    ImGui::PushID((std::to_string(row) + flagTable.name).c_str());
    for (int32_t flagIndex = 15; flagIndex >= 0; flagIndex--) {
        ImGui::SameLine();
        ImGui::PushID(flagIndex);
        bool hasDescription = !!flagTable.flagDescriptions.contains(row * 16 + flagIndex);
        uint32_t bitMask = 1 << flagIndex;
        ImVec4 themeColor = ColorValues.at(THEME_COLOR);
        ImVec4 colorDark = { themeColor.x * 0.4f, themeColor.y * 0.4f, themeColor.z * 0.4f, themeColor.z };
        ImVec4& color = themeColor;
        if (!hasDescription) {
            color = colorDark;
        }
        PushStyleCheckbox(hasDescription ? themeColor : colorDark);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 3.0f));
        bool flag = (flags & bitMask) != 0;
        if (ImGui::Checkbox("##check", &flag)) {
            if (flag) {
                flags |= bitMask;
            } else {
                flags &= ~bitMask;
            }
        }
        ImGui::PopStyleVar();
        PopStyleCheckbox();
        if (ImGui::IsItemHovered() && hasDescription) {
            ImGui::BeginTooltip();
            uint16_t index = row * 16 + flagIndex;
            const char* desc = flagTable.flagDescriptions.at(index);
            ImGui::Text("0x%02X: %s", index, UIWidgets::WrappedText(desc, 60).c_str());
            ImGui::EndTooltip();
        }
        ImGui::PopID();
    }

    ImGui::PopID();
}

static uint16_t& GetFlagTableEntry(const FlagTable& flagTable, size_t row) {
    switch (flagTable.flagTableType) {
        case EVENT_CHECK_INF:
            return gSaveContext.eventChkInf[row];
        case ITEM_GET_INF:
            return gSaveContext.itemGetInf[row];
        case INF_TABLE:
            return gSaveContext.infTable[row];
        case EVENT_INF:
            return gSaveContext.eventInf[row];
        case RANDOMIZER_INF:
            return gSaveContext.ship.randomizerInf[row];
        default: // Shouldn't be hit
            assert(false);
            return gSaveContext.eventChkInf[row];
    }
}

static void DrawFlagTableSearchResults(const FlagTable& flagTable, ImGuiTextFilter& filter) {
    bool hasMatches = false;

    for (size_t row = 0; row < flagTable.size + 1; row++) {
        uint16_t& flags = GetFlagTableEntry(flagTable, row);

        for (int32_t flagIndex = 15; flagIndex >= 0; flagIndex--) {
            uint16_t index = static_cast<uint16_t>(row * 16 + flagIndex);
            auto descIt = flagTable.flagDescriptions.find(index);
            const char* desc = descIt != flagTable.flagDescriptions.end() ? descIt->second : "";
            std::string searchable = fmt::format("0x{:02X} {}", index, desc);
            if (!filter.PassFilter(searchable.c_str())) {
                continue;
            }

            hasMatches = true;

            ImGui::PushID(index);
            bool hasDescription = descIt != flagTable.flagDescriptions.end();
            uint32_t bitMask = 1 << flagIndex;
            ImVec4 themeColor = ColorValues.at(THEME_COLOR);
            ImVec4 colorDark = { themeColor.x * 0.4f, themeColor.y * 0.4f, themeColor.z * 0.4f, themeColor.z };
            PushStyleCheckbox(hasDescription ? themeColor : colorDark);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 3.0f));
            bool flag = (flags & bitMask) != 0;
            if (ImGui::Checkbox("##check", &flag)) {
                if (flag) {
                    flags |= bitMask;
                } else {
                    flags &= ~bitMask;
                }
            }
            ImGui::PopStyleVar();
            PopStyleCheckbox();

            ImGui::SameLine();
            if (hasDescription) {
                ImGui::TextWrapped("0x%02X: %s", index, desc);
            } else {
                ImGui::Text("0x%02X", index);
            }

            ImGui::PopID();
        }
    }

    if (!hasMatches) {
        ImGui::Text("No flags match the current search.");
    }
}

#ifdef COMBO_BUILD
// ComboShip: the Flags tab with MM's layout: one table at a time from a "Flag type" list with plain names, each group
// with Set all (theme) and Clear all (Red), a searchable scene list and a search box on every flag table. The player
// state flags are on the Player tab. "Keep GS Count Updated" recounts only when a Gold Skulltula flag changes, so it no
// longer overwrites the Quest Status token count every frame.
void DrawFlagsTab() {
    namespace Kit = ComboSaveEditorKit;
    const ImVec4 theme = ColorValues.at(THEME_COLOR);
    static const std::map<FlagTableType, const char*> tableNames = {
        { EVENT_CHECK_INF, "Event flags" }, { ITEM_GET_INF, "Item flags" },         { INF_TABLE, "Info flags" },
        { EVENT_INF, "Temporary events" },  { RANDOMIZER_INF, "Randomizer flags" },
    };
    std::vector<const char*> types = { "Current scene", "Saved scene", "Gold Skulltulas" };
    std::vector<size_t> tables;
    for (size_t i = 0; i < flagTables.size(); i++) {
        if (flagTables[i].flagTableType == RANDOMIZER_INF && !IS_RANDO && !IS_BOSS_RUSH) {
            continue;
        }
        types.push_back(tableNames.at(flagTables[i].flagTableType));
        tables.push_back(i);
    }
    static int32_t flagType = 0;
    flagType = std::clamp<int32_t>(flagType, 0, static_cast<int32_t>(types.size()) - 1);
    Combobox("Flag type", &flagType, types, comboboxOptionsBase.Tooltip(""));

    // One group of 32 flags: its name, Set all and Clear all, then the flags.
    auto group = [&](const char* name, const char* help, uint32_t& flags) {
        Kit::BeginCard(name);
        ImGui::Text("%s", name);
        InsertHelpHoverText(help);
        ImGui::SameLine();
        if (Kit::Button((std::string("Set all##") + name).c_str(), theme)) {
            flags = UINT32_MAX;
        }
        ImGui::SameLine();
        if (Kit::Button((std::string("Clear all##") + name).c_str(), Kit::kReset)) {
            flags = 0;
        }
        DrawFlagArray32(name, flags, THEME_COLOR);
        Kit::EndCard();
    };

    if (flagType == 0) {
        if (gPlayState == nullptr) {
            ImGui::TextWrapped("%s", Kit::kNeedsPlay);
            return;
        }
        ActorContext* act = &gPlayState->actorCtx;
        Kit::BeginCardLayout(2, 360.0f);
        group("Switch", "Permanently-saved switch flags", act->flags.swch);
        group("Temporary switch", "Temporary switch flags. Unset on scene transitions", act->flags.tempSwch);
        group("Room clear", "Permanently-saved room-clear flags", act->flags.clear);
        group("Temporary room clear", "Temporary room-clear flags. Unset on scene transitions", act->flags.tempClear);
        group("Collectible", "Permanently-saved collect flags", act->flags.collect);
        group("Temporary collectible", "Temporary collect flags. Unset on scene transitions", act->flags.tempCollect);
        group("Chest", "Permanently-saved chest flags", act->flags.chest);
        Kit::EndCardLayout();

        if (Button("Reload Flags",
                   buttonOptionsBase.Tooltip("Load flags from saved scene flags. Normally happens on scene load"))) {
            act->flags.swch = gSaveContext.sceneFlags[gPlayState->sceneNum].swch;
            act->flags.clear = gSaveContext.sceneFlags[gPlayState->sceneNum].clear;
            act->flags.collect = gSaveContext.sceneFlags[gPlayState->sceneNum].collect;
            act->flags.chest = gSaveContext.sceneFlags[gPlayState->sceneNum].chest;
        }
        ImGui::SameLine();
        if (Button("Save Flags",
                   buttonOptionsBase.Tooltip("Save current scene flags. Normally happens on scene exit"))) {
            gSaveContext.sceneFlags[gPlayState->sceneNum].swch = act->flags.swch;
            gSaveContext.sceneFlags[gPlayState->sceneNum].clear = act->flags.clear;
            gSaveContext.sceneFlags[gPlayState->sceneNum].collect = act->flags.collect;
            gSaveContext.sceneFlags[gPlayState->sceneNum].chest = act->flags.chest;
        }
        ImGui::SameLine();
        if (Button("Clear Flags",
                   buttonOptionsBase.Tooltip("Clear current scene flags. Reload scene to see changes"))) {
            act->flags.swch = 0;
            act->flags.clear = 0;
            act->flags.collect = 0;
            act->flags.chest = 0;
        }
    } else if (flagType == 1) {
        static int32_t selectedSceneFlagMap = 0;
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Scene");
        ImGui::SameLine();
        PushStyleCombobox(THEME_COLOR);
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 16);
        Kit::SearchableCombo("##Map", &selectedSceneFlagMap, SCENE_ID_MAX,
                             [](int scene) { return SohUtils::GetSceneName(scene); });
        PopStyleCombobox();
        // Don't show current scene button if there is no current scene
        if (gPlayState != nullptr) {
            ImGui::SameLine();
            if (Button("Current", buttonOptionsBase.Tooltip("Open flags for current scene"))) {
                selectedSceneFlagMap = gPlayState->sceneNum;
            }
        }
        auto& sceneFlags = gSaveContext.sceneFlags[selectedSceneFlagMap];
        Kit::BeginCardLayout(2, 360.0f);
        group("Switch", "Switch flags", sceneFlags.swch);
        group("Room clear", "Room-clear flags", sceneFlags.clear);
        group("Collectible", "Collect flags", sceneFlags.collect);
        group("Chest", "Chest flags", sceneFlags.chest);
        group("Rooms visited", "Flags for visited rooms", sceneFlags.rooms);
        group("Floors visited", "Flags for visited floors", sceneFlags.floors);
        Kit::EndCardLayout();
    } else if (flagType == 2) {
        PushStyleCombobox(THEME_COLOR);
        static size_t selectedGsMap = 0;
        if (ImGui::BeginCombo("##GSMap", gsMapping[selectedGsMap])) {
            for (size_t index = 0; index < gsMapping.size(); index++) {
                if (ImGui::Selectable(gsMapping[index])) {
                    selectedGsMap = index;
                }
            }
            ImGui::EndCombo();
        }
        PopStyleCombobox();

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Flags");
        uint32_t currentFlags = GET_GS_FLAGS(selectedGsMap);
        uint32_t allFlags = gAreaGsFlags[selectedGsMap];
        uint32_t setMask = 1;
        bool changed = false;
        // Iterate over bitfield and create a checkbox for each skulltula
        while (allFlags != 0) {
            bool isThisSet = (currentFlags & 0x1) == 0x1;

            ImGui::SameLine();
            ImGui::PushID(allFlags);
            PushStyleCheckbox(THEME_COLOR);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 3.0f));
            if (ImGui::Checkbox("##gs", &isThisSet)) {
                changed = true;
                if (isThisSet) {
                    SET_GS_FLAGS(selectedGsMap, setMask);
                } else {
                    // Have to do this roundabout method as the macro does not support clearing flags
                    uint32_t currentFlagsBase = GET_GS_FLAGS(selectedGsMap);
                    gSaveContext.gsFlags[selectedGsMap >> 2] &= ~gGsFlagsMasks[selectedGsMap & 3];
                    SET_GS_FLAGS(selectedGsMap, currentFlagsBase & ~setMask);
                }
            }
            ImGui::PopStyleVar();
            PopStyleCheckbox();

            ImGui::PopID();

            allFlags >>= 1;
            currentFlags >>= 1;
            setMask <<= 1;
        }

        // If playing a Randomizer Save with Shuffle Skull Tokens on anything other than "Off" we don't want to keep
        // GS Token Count updated, since Gold Skulltulas killed will not correlate to GS Tokens Collected.
        if (!(IS_RANDO &&
              OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHUFFLE_TOKENS) != RO_TOKENSANITY_OFF)) {
            static bool keepGsCountUpdated = true;
            Checkbox("Keep GS Count Updated", &keepGsCountUpdated,
                     checkboxOptionsBase.Tooltip(
                         "Adjust the number of gold skulltula tokens acquired when a flag here is changed."));
            if (keepGsCountUpdated && changed) {
                int32_t gsCount = 0;
                for (int32_t gsFlagIndex = 0; gsFlagIndex < 6; gsFlagIndex++) {
                    gsCount += std::popcount(static_cast<uint32_t>(gSaveContext.gsFlags[gsFlagIndex]));
                }
                gSaveContext.inventory.gsTokens = gsCount;
            }
        }
    } else {
        const FlagTable& flagTable = flagTables[tables[flagType - 3]];
        ImGui::PushID(flagTable.name);
        ImGuiTextFilter& flagFilter = flagTableFilters[flagTable.name];
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 16);
        PushStyleInput(THEME_COLOR);
        flagFilter.Draw("Search");
        PopStyleInput();
        ImGui::Spacing();

        if (!flagFilter.IsActive()) {
            // The editor uses the menu font; these hex grids keep the monospace font their column headers are
            // aligned for.
            ImGui::PushFont(OTRGlobals::Instance->fontMonoLarger);
            for (size_t j = 0; j < flagTable.size + 1; j++) {
                DrawGroupWithBorder(
                    [&]() {
                        if (j == 0) {
                            for (int k = 0xF; k >= 0; k--) {
                                ImGui::SameLine(static_cast<f32>(37.5 + ((0xF - k) * 33.8)));
                                ImGui::Text("%X", k);
                            }
                        }

                        ImGui::Text("%s", fmt::format("{:<2X}", j).c_str());
                        DrawFlagTableArray16(flagTable, static_cast<uint16_t>(j), GetFlagTableEntry(flagTable, j));
                    },
                    flagTable.name);
            }
            ImGui::PopFont();
        } else {
            DrawFlagTableSearchResults(flagTable, flagFilter);
        }

        // make some buttons to help with fishsanity debugging
        uint8_t fsMode = OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_FISHSANITY);
        if (flagTable.flagTableType == RANDOMIZER_INF && fsMode != RO_FISHSANITY_OFF &&
            fsMode != RO_FISHSANITY_OVERWORLD) {
            if (ImGui::Button("Catch All (Child)")) {
                for (int k = RAND_INF_CHILD_FISH_1; k <= RAND_INF_CHILD_LOACH_2; k++) {
                    Flags_SetRandomizerInf((RandomizerInf)k);
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Uncatch All (Child)")) {
                for (int k = RAND_INF_CHILD_FISH_1; k <= RAND_INF_CHILD_LOACH_2; k++) {
                    Flags_UnsetRandomizerInf((RandomizerInf)k);
                }
            }

            if (ImGui::Button("Catch All (Adult)")) {
                for (int k = RAND_INF_ADULT_FISH_1; k <= RAND_INF_ADULT_LOACH; k++) {
                    Flags_SetRandomizerInf((RandomizerInf)k);
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Uncatch All (Adult)")) {
                for (int k = RAND_INF_ADULT_FISH_1; k <= RAND_INF_ADULT_LOACH; k++) {
                    Flags_UnsetRandomizerInf((RandomizerInf)k);
                }
            }
        }

        ImGui::PopID();
    }
}
#else
void DrawFlagsTab() {
    if (ImGui::TreeNode("Player State")) {
        if (gPlayState != nullptr) {
            Player* player = GET_PLAYER(gPlayState);

            DrawGroupWithBorder(
                [&]() {
                    ImGui::Text("stateFlags1");
                    DrawFlagArray32("stateFlags1", player->stateFlags1, THEME_COLOR);
                },
                "stateFlags1");

            ImGui::SameLine();

            DrawGroupWithBorder(
                [&]() {
                    ImGui::Text("stateFlags2");
                    DrawFlagArray32("stateFlags2", player->stateFlags2, THEME_COLOR);
                },
                "stateFlags2");

            DrawGroupWithBorder(
                [&]() {
                    ImGui::Text("stateFlags3");
                    DrawFlagArray8("stateFlags3", player->stateFlags3, THEME_COLOR);
                },
                "stateFlags3");

            ImGui::SameLine();

            DrawGroupWithBorder(
                [&]() {
                    ImGui::Text("unk_6AE_rotFlags");
                    DrawFlagArray16("unk_6AE_rotFlags", player->unk_6AE_rotFlags, THEME_COLOR);
                },
                "unk_6AE_rotFlags");
        }
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Current Scene")) {
        if (gPlayState != nullptr) {
            ActorContext* act = &gPlayState->actorCtx;
            DrawGroupWithBorder(
                [&]() {
                    ImGui::Text("Switch");
                    InsertHelpHoverText("Permanently-saved switch flags");
                    if (Button("Set All##Switch", buttonOptionsBase.Tooltip(""))) {
                        act->flags.swch = UINT32_MAX;
                    }
                    ImGui::SameLine();
                    if (Button("Clear All##Switch", buttonOptionsBase.Tooltip(""))) {
                        act->flags.swch = 0;
                    }
                    DrawFlagArray32("Switch", act->flags.swch, THEME_COLOR);
                },
                "Switch");

            ImGui::SameLine();

            DrawGroupWithBorder(
                [&]() {
                    ImGui::Text("Temp Switch");
                    InsertHelpHoverText("Temporary switch flags. Unset on scene transitions");
                    if (Button("Set All##Temp Switch", buttonOptionsBase.Tooltip(""))) {
                        act->flags.tempSwch = UINT32_MAX;
                    }
                    ImGui::SameLine();
                    if (Button("Clear All##Temp Switch", buttonOptionsBase.Tooltip(""))) {
                        act->flags.tempSwch = 0;
                    }
                    DrawFlagArray32("Temp Switch", act->flags.tempSwch, THEME_COLOR);
                },
                "Temp Switch");

            DrawGroupWithBorder(
                [&]() {
                    ImGui::Text("Clear");
                    InsertHelpHoverText("Permanently-saved room-clear flags");
                    if (Button("Set All##Clear", buttonOptionsBase.Tooltip(""))) {
                        act->flags.clear = UINT32_MAX;
                    }
                    ImGui::SameLine();
                    if (Button("Clear All##Clear", buttonOptionsBase.Tooltip(""))) {
                        act->flags.clear = 0;
                    }
                    DrawFlagArray32("Clear", act->flags.clear, THEME_COLOR);
                },
                "Clear");

            ImGui::SameLine();

            DrawGroupWithBorder(
                [&]() {
                    ImGui::Text("Temp Clear");
                    InsertHelpHoverText("Temporary room-clear flags. Unset on scene transitions");
                    if (Button("Set All##Temp Clear", buttonOptionsBase.Tooltip(""))) {
                        act->flags.tempClear = UINT32_MAX;
                    }
                    ImGui::SameLine();
                    if (Button("Clear All##Temp Clear", buttonOptionsBase.Tooltip(""))) {
                        act->flags.tempClear = 0;
                    }
                    DrawFlagArray32("Temp Clear", act->flags.tempClear, THEME_COLOR);
                },
                "Temp Clear");

            DrawGroupWithBorder(
                [&]() {
                    ImGui::Text("Collect");
                    InsertHelpHoverText("Permanently-saved collect flags");
                    if (Button("Set All##Collect", buttonOptionsBase.Tooltip(""))) {
                        act->flags.collect = UINT32_MAX;
                    }
                    ImGui::SameLine();
                    if (Button("Clear All##Collect", buttonOptionsBase.Tooltip(""))) {
                        act->flags.collect = 0;
                    }
                    DrawFlagArray32("Collect", act->flags.collect, THEME_COLOR);
                },
                "Collect");

            ImGui::SameLine();

            DrawGroupWithBorder(
                [&]() {
                    ImGui::Text("Temp Collect");
                    InsertHelpHoverText("Temporary collect flags. Unset on scene transitions");
                    if (Button("Set All##Temp Collect", buttonOptionsBase.Tooltip(""))) {
                        act->flags.tempCollect = UINT32_MAX;
                    }
                    ImGui::SameLine();
                    if (Button("Clear All##Temp Collect", buttonOptionsBase.Tooltip(""))) {
                        act->flags.tempCollect = 0;
                    }
                    DrawFlagArray32("Temp Collect", act->flags.tempCollect, THEME_COLOR);
                },
                "Temp Collect");

            DrawGroupWithBorder(
                [&]() {
                    ImGui::Text("Chest");
                    InsertHelpHoverText("Permanently-saved chest flags");
                    if (Button("Set All##Chest", buttonOptionsBase.Tooltip(""))) {
                        act->flags.chest = UINT32_MAX;
                    }
                    ImGui::SameLine();
                    if (Button("Clear All##Chest", buttonOptionsBase.Tooltip(""))) {
                        act->flags.chest = 0;
                    }
                    DrawFlagArray32("Chest", act->flags.chest, THEME_COLOR);
                },
                "Chest");

            ImGui::SameLine();

            ImGui::BeginGroup();

            if (Button("Reload Flags", buttonOptionsBase.Tooltip(
                                           "Load flags from saved scene flags. Normally happens on scene load"))) {
                act->flags.swch = gSaveContext.sceneFlags[gPlayState->sceneNum].swch;
                act->flags.clear = gSaveContext.sceneFlags[gPlayState->sceneNum].clear;
                act->flags.collect = gSaveContext.sceneFlags[gPlayState->sceneNum].collect;
                act->flags.chest = gSaveContext.sceneFlags[gPlayState->sceneNum].chest;
            }

            if (Button("Save Flags",
                       buttonOptionsBase.Tooltip("Save current scene flags. Normally happens on scene exit"))) {
                gSaveContext.sceneFlags[gPlayState->sceneNum].swch = act->flags.swch;
                gSaveContext.sceneFlags[gPlayState->sceneNum].clear = act->flags.clear;
                gSaveContext.sceneFlags[gPlayState->sceneNum].collect = act->flags.collect;
                gSaveContext.sceneFlags[gPlayState->sceneNum].chest = act->flags.chest;
            }

            if (Button("Clear Flags",
                       buttonOptionsBase.Tooltip("Clear current scene flags. Reload scene to see changes"))) {
                act->flags.swch = 0;
                act->flags.clear = 0;
                act->flags.collect = 0;
                act->flags.chest = 0;
            }

            ImGui::EndGroup();
        } else {
            ImGui::Text("Current game state does not have an active scene");
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Saved Scene Flags")) {
        static uint32_t selectedSceneFlagMap = 0;
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Map");
        ImGui::SameLine();
        PushStyleCombobox(THEME_COLOR);
        if (ImGui::BeginCombo("##Map", SohUtils::GetSceneName(selectedSceneFlagMap).c_str())) {
            for (int32_t sceneIndex = 0; sceneIndex < SCENE_ID_MAX; sceneIndex++) {
                if (ImGui::Selectable(SohUtils::GetSceneName(sceneIndex).c_str())) {
                    selectedSceneFlagMap = sceneIndex;
                }
            }

            ImGui::EndCombo();
        }
        PopStyleCombobox();

        // Don't show current scene button if there is no current scene
        if (gPlayState != nullptr) {
            ImGui::SameLine();
            if (Button("Current", buttonOptionsBase.Tooltip("Open flags for current scene"))) {
                selectedSceneFlagMap = gPlayState->sceneNum;
            }
        }

        DrawGroupWithBorder(
            [&]() {
                ImGui::Text("Switch");
                InsertHelpHoverText("Switch flags");
                DrawFlagArray32("Switch", gSaveContext.sceneFlags[selectedSceneFlagMap].swch, THEME_COLOR);
            },
            "Saved Switch");

        ImGui::SameLine();

        DrawGroupWithBorder(
            [&]() {
                ImGui::Text("Clear");
                InsertHelpHoverText("Room-clear flags");
                DrawFlagArray32("Clear", gSaveContext.sceneFlags[selectedSceneFlagMap].clear, THEME_COLOR);
            },
            "Saved Clear");

        DrawGroupWithBorder(
            [&]() {
                ImGui::Text("Collect");
                InsertHelpHoverText("Collect flags");
                DrawFlagArray32("Collect", gSaveContext.sceneFlags[selectedSceneFlagMap].collect, THEME_COLOR);
            },
            "Saved Collect");

        ImGui::SameLine();

        DrawGroupWithBorder(
            [&]() {
                ImGui::Text("Chest");
                InsertHelpHoverText("Chest flags");
                DrawFlagArray32("Chest", gSaveContext.sceneFlags[selectedSceneFlagMap].chest, THEME_COLOR);
            },
            "Saved Chest");

        DrawGroupWithBorder(
            [&]() {
                ImGui::Text("Rooms");
                InsertHelpHoverText("Flags for visted rooms");
                DrawFlagArray32("Rooms", gSaveContext.sceneFlags[selectedSceneFlagMap].rooms, THEME_COLOR);
            },
            "Saved Rooms");

        ImGui::SameLine();

        DrawGroupWithBorder(
            [&]() {
                ImGui::Text("Floors");
                InsertHelpHoverText("Flags for visted floors");
                DrawFlagArray32("Floors", gSaveContext.sceneFlags[selectedSceneFlagMap].floors, THEME_COLOR);
            },
            "Saved Floors");

        ImGui::TreePop();
    }

    DrawGroupWithBorder(
        [&]() {
            PushStyleCombobox(THEME_COLOR);
            static size_t selectedGsMap = 0;
            ImGui::Text("Gold Skulltulas");
            if (ImGui::BeginCombo("##GSMap", gsMapping[selectedGsMap])) {
                for (size_t index = 0; index < gsMapping.size(); index++) {
                    if (ImGui::Selectable(gsMapping[index])) {
                        selectedGsMap = index;
                    }
                }

                ImGui::EndCombo();
            }
            PopStyleCombobox();

            // TODO We should write out descriptions for each one... ugh
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Flags");
            uint32_t currentFlags = GET_GS_FLAGS(selectedGsMap);
            uint32_t allFlags = gAreaGsFlags[selectedGsMap];
            uint32_t setMask = 1;
            // Iterate over bitfield and create a checkbox for each skulltula
            while (allFlags != 0) {
                bool isThisSet = (currentFlags & 0x1) == 0x1;

                ImGui::SameLine();
                ImGui::PushID(allFlags);
                PushStyleCheckbox(THEME_COLOR);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 3.0f));
                if (ImGui::Checkbox("##gs", &isThisSet)) {
                    if (isThisSet) {
                        SET_GS_FLAGS(selectedGsMap, setMask);
                    } else {
                        // Have to do this roundabout method as the macro does not support clearing flags
                        uint32_t currentFlagsBase = GET_GS_FLAGS(selectedGsMap);
                        gSaveContext.gsFlags[selectedGsMap >> 2] &= ~gGsFlagsMasks[selectedGsMap & 3];
                        SET_GS_FLAGS(selectedGsMap, currentFlagsBase & ~setMask);
                    }
                }
                ImGui::PopStyleVar();
                PopStyleCheckbox();

                ImGui::PopID();

                allFlags >>= 1;
                currentFlags >>= 1;
                setMask <<= 1;
            }

            // If playing a Randomizer Save with Shuffle Skull Tokens on anything other than "Off" we don't want to keep
            // GS Token Count updated, since Gold Skulltulas killed will not correlate to GS Tokens Collected.
            if (!(IS_RANDO &&
                  OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHUFFLE_TOKENS) != RO_TOKENSANITY_OFF)) {
                static bool keepGsCountUpdated = true;
                Checkbox("Keep GS Count Updated", &keepGsCountUpdated,
                         checkboxOptionsBase.Tooltip(
                             "Automatically adjust the number of gold skulltula tokens acquired based on set flags."));
                int32_t gsCount = 0;
                if (keepGsCountUpdated) {
                    for (int32_t gsFlagIndex = 0; gsFlagIndex < 6; gsFlagIndex++) {
                        gsCount += std::popcount(static_cast<uint32_t>(gSaveContext.gsFlags[gsFlagIndex]));
                    }
                    gSaveContext.inventory.gsTokens = gsCount;
                }
            }
        },
        "Gold Skulltulas");

    for (size_t i = 0; i < flagTables.size(); i++) {
        const FlagTable& flagTable = flagTables[i];
        if (flagTable.flagTableType == RANDOMIZER_INF && !IS_RANDO && !IS_BOSS_RUSH) {
            continue;
        }

        if (ImGui::TreeNode(flagTable.name)) {
            ImGui::PushID(flagTable.name);
            ImGuiTextFilter& flagFilter = flagTableFilters[flagTable.name];
            ImGui::SetNextItemWidth(ImGui::GetFontSize() * 16);
            PushStyleInput(THEME_COLOR);
            flagFilter.Draw();
            PopStyleInput();
            ImGui::Spacing();

            if (!flagFilter.IsActive()) {
                for (size_t j = 0; j < flagTable.size + 1; j++) {
                    DrawGroupWithBorder(
                        [&]() {
                            if (j == 0) {
                                for (int k = 0xF; k >= 0; k--) {
                                    ImGui::SameLine(static_cast<f32>(37.5 + ((0xF - k) * 33.8)));
                                    ImGui::Text("%X", k);
                                }
                            }

                            ImGui::Text("%s", fmt::format("{:<2X}", j).c_str());

                            switch (flagTable.flagTableType) {
                                case EVENT_CHECK_INF:
                                    DrawFlagTableArray16(flagTable, static_cast<uint16_t>(j),
                                                         gSaveContext.eventChkInf[j]);
                                    break;
                                case ITEM_GET_INF:
                                    DrawFlagTableArray16(flagTable, static_cast<uint16_t>(j),
                                                         gSaveContext.itemGetInf[j]);
                                    break;
                                case INF_TABLE:
                                    DrawFlagTableArray16(flagTable, static_cast<uint16_t>(j), gSaveContext.infTable[j]);
                                    break;
                                case EVENT_INF:
                                    DrawFlagTableArray16(flagTable, static_cast<uint16_t>(j), gSaveContext.eventInf[j]);
                                    break;
                                case RANDOMIZER_INF:
                                    DrawFlagTableArray16(flagTable, static_cast<uint16_t>(j),
                                                         gSaveContext.ship.randomizerInf[j]);
                                    break;
                            }
                        },
                        flagTable.name);
                }
            } else {
                DrawFlagTableSearchResults(flagTable, flagFilter);
            }

            // make some buttons to help with fishsanity debugging
            uint8_t fsMode = OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_FISHSANITY);
            if (flagTable.flagTableType == RANDOMIZER_INF && fsMode != RO_FISHSANITY_OFF &&
                fsMode != RO_FISHSANITY_OVERWORLD) {
                if (ImGui::Button("Catch All (Child)")) {
                    for (int k = RAND_INF_CHILD_FISH_1; k <= RAND_INF_CHILD_LOACH_2; k++) {
                        Flags_SetRandomizerInf((RandomizerInf)k);
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Uncatch All (Child)")) {
                    for (int k = RAND_INF_CHILD_FISH_1; k <= RAND_INF_CHILD_LOACH_2; k++) {
                        Flags_UnsetRandomizerInf((RandomizerInf)k);
                    }
                }

                if (ImGui::Button("Catch All (Adult)")) {
                    for (int k = RAND_INF_ADULT_FISH_1; k <= RAND_INF_ADULT_LOACH; k++) {
                        Flags_SetRandomizerInf((RandomizerInf)k);
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Uncatch All (Adult)")) {
                    for (int k = RAND_INF_ADULT_FISH_1; k <= RAND_INF_ADULT_LOACH; k++) {
                        Flags_UnsetRandomizerInf((RandomizerInf)k);
                    }
                }
            }

            ImGui::PopID();
            ImGui::TreePop();
        }
    }
}
#endif

// Draws a combo that lets you choose and upgrade value from a drop-down of text values
void DrawUpgrade(const std::string& categoryName, int32_t categoryId, const std::vector<std::string>& names) {
    ImGui::Text("%s", categoryName.c_str());
    ImGui::SameLine();
    ImGui::PushID(categoryName.c_str());
    PushStyleCombobox(THEME_COLOR);
    ImGui::AlignTextToFramePadding();
    auto value = (size_t)CUR_UPG_VALUE(categoryId);
    auto name = value < names.size() ? names[value].c_str() : "Glitched";
    if (ImGui::BeginCombo("##upgrade", name)) {
        for (size_t i = 0; i < names.size(); i++) {
            if (ImGui::Selectable(names[i].c_str())) {
                Inventory_ChangeUpgrade(categoryId, static_cast<s16>(i));
            }
        }

        ImGui::EndCombo();
    }
    PopStyleCombobox();
    ImGui::PopID();
    UIWidgets::Tooltip(categoryName.c_str());
}

// Draws a combo that lets you choose and upgrade value from a popup grid of icons
#ifdef COMBO_BUILD
// ComboShip: an upgrade as MM's Equipment tab draws one: its icon with its name under it, a picker of every level
// with item names.
void DrawUpgradeIcon(const std::string& categoryName, int32_t categoryId, const std::vector<uint8_t>& items) {
    auto gui = std::dynamic_pointer_cast<Fast::Fast3dGui>(Ship::Context::GetRawInstance()->GetWindow()->GetGui());
    std::vector<ComboSaveEditorKit::Level> levels;
    std::vector<std::string> names;
    for (uint8_t item : items) {
        names.push_back(item == ITEM_NONE ? "None" : SohUtils::GetItemName(item));
    }
    for (size_t i = 0; i < items.size(); i++) {
        levels.push_back({ items[i] == ITEM_NONE ? ComboSaveEditorKit::kNoTexture
                                                 : gui->GetTextureByName(itemMapping[items[i]].name),
                           names[i].c_str() });
    }
    const int picked = ComboSaveEditorKit::LevelPicker(categoryName.c_str(), CUR_UPG_VALUE(categoryId), levels);
    if (picked >= 0) {
        Inventory_ChangeUpgrade(categoryId, static_cast<s16>(picked));
    }
}
#else
void DrawUpgradeIcon(const std::string& categoryName, int32_t categoryId, const std::vector<uint8_t>& items) {
    static const char* upgradePopupPicker = "upgradePopupPicker";

    ImGui::PushID(categoryName.c_str());

    PushStyleButton(Colors::DarkGray);
    auto value = (size_t)CUR_UPG_VALUE(categoryId);
    uint8_t item = value < items.size() ? items[value] : ITEM_NONE;
    const ItemMapEntry& slotEntry = itemMapping[item];
    if (ImGui::ImageButton(
            slotEntry.name.c_str(),
            std::dynamic_pointer_cast<Fast::Fast3dGui>(Ship::Context::GetRawInstance()->GetWindow()->GetGui())
                ->GetTextureByName(item != ITEM_NONE ? slotEntry.name : itemMapping[items[1]].nameFaded),
            ImVec2(IMAGE_SIZE, IMAGE_SIZE), ImVec2(0, 0), ImVec2(1, 1))) {
        ImGui::OpenPopup(upgradePopupPicker);
    }
    PopStyleButton();
    Tooltip(categoryName.c_str());

    if (ImGui::BeginPopup(upgradePopupPicker)) {
        for (size_t pickerIndex = 0; pickerIndex < items.size(); pickerIndex++) {
            if ((pickerIndex % 8) != 0) {
                ImGui::SameLine();
            }

            PushStyleButton(Colors::DarkGray);
            if (items[pickerIndex] == ITEM_NONE) {
                if (ImGui::Button("##upgradePopupPicker",
                                  ImVec2(IMAGE_SIZE, IMAGE_SIZE) + ImGui::GetStyle().FramePadding * 2)) {
                    Inventory_ChangeUpgrade(categoryId, static_cast<s16>(pickerIndex));
                    ImGui::CloseCurrentPopup();
                }
                Tooltip("None");
            } else {
                const ItemMapEntry& slotEntry = itemMapping[items[pickerIndex]];
                auto ret = ImGui::ImageButton(
                    slotEntry.name.c_str(),
                    std::dynamic_pointer_cast<Fast::Fast3dGui>(Ship::Context::GetRawInstance()->GetWindow()->GetGui())
                        ->GetTextureByName(slotEntry.name),
                    ImVec2(IMAGE_SIZE, IMAGE_SIZE), ImVec2(0, 0), ImVec2(1, 1));
                if (ret) {
                    Inventory_ChangeUpgrade(categoryId, static_cast<s16>(pickerIndex));
                    ImGui::CloseCurrentPopup();
                }
                Tooltip(SohUtils::GetItemName(slotEntry.id).c_str());
            }
            PopStyleButton();
        }
        ImGui::EndPopup();
    }
    ImGui::PopID();
}
#endif

#ifdef COMBO_BUILD
// ComboShip: the Equipment tab, mirrored by MM's: the sword, shield, tunic and boots toggles (OoT owns each one), the
// upgrade pickers, the capacities, and the Biggoron's Sword quest moved here from General. The wallet is on General.
void DrawEquipmentTab() {
    namespace Kit = ComboSaveEditorKit;
    const ImVec4 theme = ColorValues.at(THEME_COLOR);
    auto gui = std::dynamic_pointer_cast<Fast::Fast3dGui>(Ship::Context::GetRawInstance()->GetWindow()->GetGui());

    // Give all and Reset as MM's Equipment tab: the best gear, or a new file's (Kokiri Tunic and Boots, no upgrades).
    static const std::pair<int32_t, int> kBestUpgrades[] = { { UPG_BULLET_BAG, 3 }, { UPG_QUIVER, 3 },
                                                             { UPG_BOMB_BAG, 3 },   { UPG_SCALE, 2 },
                                                             { UPG_STRENGTH, 3 },   { UPG_STICKS, 3 },
                                                             { UPG_NUTS, 3 } };
    if (Kit::Button("Give all##equipment", Kit::kGiveAll)) {
        gSaveContext.inventory.equipment = 0x7777; // every sword but the broken knife, shield, tunic and boots
        gSaveContext.bgsFlag = 1;
        for (const auto& [upgrade, level] : kBestUpgrades) {
            Inventory_ChangeUpgrade(upgrade, level);
        }
    }
    ImGui::SameLine();
    if (Kit::Button("Reset##equipment", Kit::kReset)) {
        gSaveContext.inventory.equipment = 0x1100; // Kokiri Tunic and Kokiri Boots
        gSaveContext.bgsFlag = 0;
        for (const auto& [upgrade, level] : kBestUpgrades) {
            Inventory_ChangeUpgrade(upgrade, 0);
        }
    }

    ImGui::SeparatorText("Swords, shields, tunics and boots");
    const std::vector<uint8_t> equipmentValues = {
        ITEM_SWORD_KOKIRI, ITEM_SWORD_MASTER,  ITEM_SWORD_BGS,     ITEM_SWORD_BROKEN,
        ITEM_SHIELD_DEKU,  ITEM_SHIELD_HYLIAN, ITEM_SHIELD_MIRROR, ITEM_NONE,
        ITEM_TUNIC_KOKIRI, ITEM_TUNIC_GORON,   ITEM_TUNIC_ZORA,    ITEM_NONE,
        ITEM_BOOTS_KOKIRI, ITEM_BOOTS_IRON,    ITEM_BOOTS_HOVER,   ITEM_NONE,
    };
    for (size_t i = 0; i < equipmentValues.size(); i++) {
        // Skip over unused 4th slots for shields, boots, and tunics
        if (equipmentValues[i] == ITEM_NONE) {
            continue;
        }
        if ((i % 4) != 0) {
            ImGui::SameLine();
        }

        ImGui::PushID(static_cast<int>(i));
        uint32_t bitMask = 1 << i;
        bool hasEquip = (bitMask & gSaveContext.inventory.equipment) != 0;
        const ItemMapEntry& entry = itemMapping[equipmentValues[i]];
        if (Kit::IconButton(entry.name.c_str(), gui->GetTextureByName(entry.name), hasEquip)) {
            if (hasEquip) {
                gSaveContext.inventory.equipment &= ~bitMask;
            } else {
                gSaveContext.inventory.equipment |= bitMask;
            }
        }
        const std::string tooltip = SohUtils::GetItemName(entry.id) + "\n" + Kit::TapHint(Kit::Tap::Toggle);
        UIWidgets::Tooltip(tooltip.c_str());
        ImGui::PopID();
    }

    ImGui::SeparatorText("Upgrades");
    DrawUpgradeIcon("Bullet Bag", UPG_BULLET_BAG,
                    { ITEM_NONE, ITEM_BULLET_BAG_30, ITEM_BULLET_BAG_40, ITEM_BULLET_BAG_50 });
    ImGui::SameLine();
    DrawUpgradeIcon("Quiver", UPG_QUIVER, { ITEM_NONE, ITEM_QUIVER_30, ITEM_QUIVER_40, ITEM_QUIVER_50 });
    ImGui::SameLine();
    DrawUpgradeIcon("Bomb Bag", UPG_BOMB_BAG, { ITEM_NONE, ITEM_BOMB_BAG_20, ITEM_BOMB_BAG_30, ITEM_BOMB_BAG_40 });
    ImGui::SameLine();
    DrawUpgradeIcon("Scale", UPG_SCALE, { ITEM_NONE, ITEM_SCALE_SILVER, ITEM_SCALE_GOLDEN });
    ImGui::SameLine();
    DrawUpgradeIcon("Strength", UPG_STRENGTH, { ITEM_NONE, ITEM_BRACELET, ITEM_GAUNTLETS_SILVER, ITEM_GAUNTLETS_GOLD });

    ImGui::SeparatorText("Capacities");
    const std::vector<std::string> stickNames = {
        "None",
        "10",
        "20",
        "30",
    };
    DrawUpgrade("Deku Stick Capacity", UPG_STICKS, stickNames);

    const std::vector<std::string> nutNames = {
        "None",
        "20",
        "30",
        "40",
    };
    DrawUpgrade("Deku Nut Capacity", UPG_NUTS, nutNames);

    if (IS_RANDO &&
        OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_BOMBCHU_BAG) == RO_BOMBCHU_BAG_PROGRESSIVE) {
        const std::vector<std::string> bombchuNames = {
            "None",
            "20",
            "30",
            "50",
        };
        ImGui::Text("%s", "Bombchu Bag Capacity");
        ImGui::SameLine();
        ImGui::PushID("Bombchu Bag Capacity");
        PushStyleCombobox(THEME_COLOR);
        ImGui::AlignTextToFramePadding();
        auto value = gSaveContext.ship.quest.data.randomizer.bombchuUpgradeLevel;
        auto name = value < bombchuNames.size() ? bombchuNames[value].c_str() : "Glitched";
        if (ImGui::BeginCombo("##upgrade", name)) {
            for (size_t i = 0; i < bombchuNames.size(); i++) {
                if (ImGui::Selectable(bombchuNames[i].c_str())) {
                    gSaveContext.ship.quest.data.randomizer.bombchuUpgradeLevel = static_cast<u8>(i);
                    if (i > 0) {
                        INV_CONTENT(ITEM_BOMBCHU) = ITEM_BOMBCHU;
                    } else {
                        INV_CONTENT(ITEM_BOMBCHU) = ITEM_NONE;
                    }
                }
            }
            ImGui::EndCombo();
        }
        PopStyleCombobox();
        ImGui::PopID();
        UIWidgets::Tooltip("Bombchu Bag Capacity");
    }

    ImGui::SeparatorText("Biggoron's Sword");
    Checkbox("Has Biggoron's Sword", (bool*)&gSaveContext.bgsFlag,
             checkboxOptionsBase.Tooltip("Is Biggoron sword unlocked? Replaces Giant's knife"));
    Kit::ResourceBar("##swordHealth", &gSaveContext.swordHealth, 0, 255, "Sword health: %d", theme);
    Kit::Tooltip("Giant's knife health. Default is 8. Must be >0 for Biggoron sword to work");
    Kit::ResourceBar("##bgsDayCount", &gSaveContext.bgsDayCount, 0, 1000, "Days since Claim Check: %d", theme);
    Kit::Tooltip("Total number of days elapsed since receiving claim check from Biggoron");
}
#else
void DrawEquipmentTab() {
    const std::vector<uint8_t> equipmentValues = {
        ITEM_SWORD_KOKIRI, ITEM_SWORD_MASTER,  ITEM_SWORD_BGS,     ITEM_SWORD_BROKEN,
        ITEM_SHIELD_DEKU,  ITEM_SHIELD_HYLIAN, ITEM_SHIELD_MIRROR, ITEM_NONE,
        ITEM_TUNIC_KOKIRI, ITEM_TUNIC_GORON,   ITEM_TUNIC_ZORA,    ITEM_NONE,
        ITEM_BOOTS_KOKIRI, ITEM_BOOTS_IRON,    ITEM_BOOTS_HOVER,   ITEM_NONE,
    };
    for (size_t i = 0; i < equipmentValues.size(); i++) {
        // Skip over unused 4th slots for shields, boots, and tunics
        if (equipmentValues[i] == ITEM_NONE) {
            continue;
        }
        if ((i % 4) != 0) {
            ImGui::SameLine();
        }

        ImGui::PushID(static_cast<int>(i));
        uint32_t bitMask = 1 << i;
        bool hasEquip = (bitMask & gSaveContext.inventory.equipment) != 0;
        const ItemMapEntry& entry = itemMapping[equipmentValues[i]];
        PushStyleButton(Colors::DarkGray);
        auto ret = ImGui::ImageButton(
            entry.name.c_str(),
            std::dynamic_pointer_cast<Fast::Fast3dGui>(Ship::Context::GetRawInstance()->GetWindow()->GetGui())
                ->GetTextureByName(hasEquip ? entry.name : entry.nameFaded),
            ImVec2(IMAGE_SIZE, IMAGE_SIZE), ImVec2(0, 0), ImVec2(1, 1));
        if (ret) {
            if (hasEquip) {
                gSaveContext.inventory.equipment &= ~bitMask;
            } else {
                gSaveContext.inventory.equipment |= bitMask;
            }
        }
        PopStyleButton();
        Tooltip(SohUtils::GetItemName(entry.id).c_str());
        ImGui::PopID();
    }

    const std::vector<uint8_t> bulletBagValues = {
        ITEM_NONE,
        ITEM_BULLET_BAG_30,
        ITEM_BULLET_BAG_40,
        ITEM_BULLET_BAG_50,
    };
    DrawUpgradeIcon("Bullet Bag", UPG_BULLET_BAG, bulletBagValues);

    ImGui::SameLine();

    const std::vector<uint8_t> quiverValues = {
        ITEM_NONE,
        ITEM_QUIVER_30,
        ITEM_QUIVER_40,
        ITEM_QUIVER_50,
    };
    DrawUpgradeIcon("Quiver", UPG_QUIVER, quiverValues);

    ImGui::SameLine();

    const std::vector<uint8_t> bombBagValues = {
        ITEM_NONE,
        ITEM_BOMB_BAG_20,
        ITEM_BOMB_BAG_30,
        ITEM_BOMB_BAG_40,
    };
    DrawUpgradeIcon("Bomb Bag", UPG_BOMB_BAG, bombBagValues);

    ImGui::SameLine();

    const std::vector<uint8_t> scaleValues = {
        ITEM_NONE,
        ITEM_SCALE_SILVER,
        ITEM_SCALE_GOLDEN,
    };
    DrawUpgradeIcon("Scale", UPG_SCALE, scaleValues);

    ImGui::SameLine();

    const std::vector<uint8_t> strengthValues = {
        ITEM_NONE,
        ITEM_BRACELET,
        ITEM_GAUNTLETS_SILVER,
        ITEM_GAUNTLETS_GOLD,
    };
    DrawUpgradeIcon("Strength", UPG_STRENGTH, strengthValues);

    // There is no icon for child wallet, so default to a text list
    // this was const, but I needed to append to it depending in rando settings.
    std::vector<std::string> walletNamesImpl = {
        "Child (99)",
        "Adult (200)",
        "Giant (500)",
    };
    // only display Tycoon wallet if you're in a save file that would allow it.
    if (IS_RANDO && OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_INCLUDE_TYCOON_WALLET)) {
        const std::string walletName = "Tycoon (999)";
        walletNamesImpl.push_back(walletName);
    }
    // copy it to const value for display in ImGui.
    const std::vector<std::string> walletNames = walletNamesImpl;
    DrawUpgrade("Wallet", UPG_WALLET, walletNames);

    const std::vector<std::string> stickNames = {
        "None",
        "10",
        "20",
        "30",
    };
    DrawUpgrade("Deku Stick Capacity", UPG_STICKS, stickNames);

    const std::vector<std::string> nutNames = {
        "None",
        "20",
        "30",
        "40",
    };
    DrawUpgrade("Deku Nut Capacity", UPG_NUTS, nutNames);

    if (IS_RANDO &&
        OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_BOMBCHU_BAG) == RO_BOMBCHU_BAG_PROGRESSIVE) {
        const std::vector<std::string> bombchuNames = {
            "None",
            "20",
            "30",
            "50",
        };
        ImGui::Text("%s", "Bombchu Bag Capacity");
        ImGui::SameLine();
        ImGui::PushID("Bombchu Bag Capacity");
        PushStyleCombobox(THEME_COLOR);
        ImGui::AlignTextToFramePadding();
        auto value = gSaveContext.ship.quest.data.randomizer.bombchuUpgradeLevel;
        auto name = value < bombchuNames.size() ? bombchuNames[value].c_str() : "Glitched";
        if (ImGui::BeginCombo("##upgrade", name)) {
            for (size_t i = 0; i < bombchuNames.size(); i++) {
                if (ImGui::Selectable(bombchuNames[i].c_str())) {
                    gSaveContext.ship.quest.data.randomizer.bombchuUpgradeLevel = static_cast<u8>(i);
                    if (i > 0) {
                        INV_CONTENT(ITEM_BOMBCHU) = ITEM_BOMBCHU;
                    } else {
                        INV_CONTENT(ITEM_BOMBCHU) = ITEM_NONE;
                    }
                }
            }
            ImGui::EndCombo();
        }
        PopStyleCombobox();
        ImGui::PopID();
        UIWidgets::Tooltip("Bombchu Bag Capapcity");
    }
}
#endif

// Draws a toggleable icon for a quest item that is faded when disabled
#ifdef COMBO_BUILD
// ComboShip: MM's icon style, faded when not owned.
void DrawQuestItemButton(uint32_t item) {
    const QuestMapEntry& entry = questMapping[item];
    uint32_t bitMask = 1 << entry.id;
    bool hasQuestItem = (bitMask & gSaveContext.inventory.questItems) != 0;
    auto gui = std::dynamic_pointer_cast<Fast::Fast3dGui>(Ship::Context::GetRawInstance()->GetWindow()->GetGui());
    if (ComboSaveEditorKit::IconButton(entry.name.c_str(), gui->GetTextureByName(entry.name), hasQuestItem)) {
        if (hasQuestItem) {
            gSaveContext.inventory.questItems &= ~bitMask;
        } else {
            gSaveContext.inventory.questItems |= bitMask;
        }
    }
    Tooltip(SohUtils::GetQuestItemName(entry.id).c_str());
}
#else
void DrawQuestItemButton(uint32_t item) {
    const QuestMapEntry& entry = questMapping[item];
    uint32_t bitMask = 1 << entry.id;
    bool hasQuestItem = (bitMask & gSaveContext.inventory.questItems) != 0;
    PushStyleButton(Colors::DarkGray);
    auto ret = ImGui::ImageButton(
        entry.name.c_str(),
        std::dynamic_pointer_cast<Fast::Fast3dGui>(Ship::Context::GetRawInstance()->GetWindow()->GetGui())
            ->GetTextureByName(hasQuestItem ? entry.name : entry.nameFaded),
        ImVec2(IMAGE_SIZE, IMAGE_SIZE), ImVec2(0, 0), ImVec2(1, 1));
    if (ret) {
        if (hasQuestItem) {
            gSaveContext.inventory.questItems &= ~bitMask;
        } else {
            gSaveContext.inventory.questItems |= bitMask;
        }
    }
    PopStyleButton();
    Tooltip(SohUtils::GetQuestItemName(entry.id).c_str());
}
#endif

// Draws a toggleable icon for a dungeon item that is faded when disabled
#ifdef COMBO_BUILD
// ComboShip: MM's icon style, faded when not owned.
void DrawDungeonItemButton(uint32_t item, uint32_t scene) {
    const ItemMapEntry& entry = itemMapping[item];
    uint32_t bitMask = 1 << (entry.id - ITEM_KEY_BOSS); // Bitset starts at ITEM_KEY_BOSS == 0. the rest are sequential
    bool hasItem = (bitMask & gSaveContext.inventory.dungeonItems[scene]) != 0;
    auto gui = std::dynamic_pointer_cast<Fast::Fast3dGui>(Ship::Context::GetRawInstance()->GetWindow()->GetGui());
    if (ComboSaveEditorKit::IconButton(entry.name.c_str(), gui->GetTextureByName(entry.name), hasItem)) {
        if (hasItem) {
            gSaveContext.inventory.dungeonItems[scene] &= ~bitMask;
        } else {
            gSaveContext.inventory.dungeonItems[scene] |= bitMask;
        }
    }
    Tooltip(SohUtils::GetItemName(entry.id).c_str());
}
#else
void DrawDungeonItemButton(uint32_t item, uint32_t scene) {
    const ItemMapEntry& entry = itemMapping[item];
    uint32_t bitMask = 1 << (entry.id - ITEM_KEY_BOSS); // Bitset starts at ITEM_KEY_BOSS == 0. the rest are sequential
    bool hasItem = (bitMask & gSaveContext.inventory.dungeonItems[scene]) != 0;
    PushStyleButton(Colors::DarkGray);
    auto ret = ImGui::ImageButton(
        entry.name.c_str(),
        std::dynamic_pointer_cast<Fast::Fast3dGui>(Ship::Context::GetRawInstance()->GetWindow()->GetGui())
            ->GetTextureByName(hasItem ? entry.name : entry.nameFaded),
        ImVec2(IMAGE_SIZE, IMAGE_SIZE), ImVec2(0, 0), ImVec2(1, 1));
    if (ret) {
        if (hasItem) {
            gSaveContext.inventory.dungeonItems[scene] &= ~bitMask;
        } else {
            gSaveContext.inventory.dungeonItems[scene] |= bitMask;
        }
    }
    PopStyleButton();
    Tooltip(SohUtils::GetItemName(entry.id).c_str());
}
#endif

#ifdef COMBO_BUILD
// ComboShip: Quest Status with MM's headings in the same order: Boss rewards, Songs, Quest items, Gold Skulltula tokens
// and Heart pieces as bars, then OoT's Triforce pieces. The dungeon items have their own tab.
void DrawQuestStatusTab() {
    namespace Kit = ComboSaveEditorKit;
    const ImVec4 theme = ColorValues.at(THEME_COLOR);
    auto gui = std::dynamic_pointer_cast<Fast::Fast3dGui>(Ship::Context::GetRawInstance()->GetWindow()->GetGui());

    // Give all and Reset as MM's: every medallion, stone, song and quest item; heart pieces and tokens stay.
    if (Kit::Button("Give all##quest", Kit::kGiveAll)) {
        for (int32_t i = QUEST_MEDALLION_FOREST; i <= QUEST_GERUDO_CARD; i++) {
            gSaveContext.inventory.questItems |= 1 << i;
        }
    }
    ImGui::SameLine();
    if (Kit::Button("Reset##quest", Kit::kReset)) {
        for (int32_t i = QUEST_MEDALLION_FOREST; i <= QUEST_GERUDO_CARD; i++) {
            gSaveContext.inventory.questItems &= ~(1 << i);
        }
    }

    ImGui::SeparatorText("Boss rewards");
    for (int32_t i = QUEST_MEDALLION_FOREST; i < QUEST_MEDALLION_LIGHT + 1; i++) {
        if (i != QUEST_MEDALLION_FOREST) {
            ImGui::SameLine();
        }
        DrawQuestItemButton(i);
    }
    for (int32_t i = QUEST_KOKIRI_EMERALD; i < QUEST_ZORA_SAPPHIRE + 1; i++) {
        if (i != QUEST_KOKIRI_EMERALD) {
            ImGui::SameLine();
        }
        DrawQuestItemButton(i);
    }

    ImGui::SeparatorText("Songs");
    for (const auto& [quest, entry] : songMapping) {
        if ((entry.id != QUEST_SONG_MINUET) && (entry.id != QUEST_SONG_LULLABY)) {
            ImGui::SameLine();
        }
        uint32_t bitMask = 1 << entry.id;
        bool hasQuestItem = (bitMask & gSaveContext.inventory.questItems) != 0;
        if (Kit::IconButton(entry.name.c_str(), gui->GetTextureByName(entry.name), hasQuestItem,
                            ImVec2(Kit::kSongIconWidth, Kit::kIconSize))) {
            if (hasQuestItem) {
                gSaveContext.inventory.questItems &= ~bitMask;
            } else {
                gSaveContext.inventory.questItems |= bitMask;
            }
        }
        Tooltip(SohUtils::GetQuestItemName(entry.id).c_str());
    }

    ImGui::SeparatorText("Quest items");
    DrawQuestItemButton(QUEST_STONE_OF_AGONY);
    ImGui::SameLine();
    DrawQuestItemButton(QUEST_GERUDO_CARD);

    ImGui::SeparatorText("Gold Skulltula tokens");
    Kit::ResourceBar("##gsTokens", &gSaveContext.inventory.gsTokens, 0, 100, "Tokens: %d", theme);
    Kit::Tooltip("Number of gold skulltula tokens acquired");
    uint32_t bitMask = 1 << QUEST_SKULL_TOKEN;
    bool gsUnlocked = (bitMask & gSaveContext.inventory.questItems) != 0;
    if (Checkbox("Show token count", &gsUnlocked, CheckboxOptions().Color(THEME_COLOR))) {
        if (gsUnlocked) {
            gSaveContext.inventory.questItems |= bitMask;
        } else {
            gSaveContext.inventory.questItems &= ~bitMask;
        }
    }
    Kit::Tooltip("Shows the gold skulltula count in the pause menu's Quest Status");

    ImGui::SeparatorText("Heart pieces");
    int32_t pohCount = (gSaveContext.inventory.questItems & 0xF0000000) >> 28;
    if (Kit::ResourceBar("##heartPieces", &pohCount, 0, 3, "Heart pieces: %d", Kit::kHeartPieces)) {
        gSaveContext.inventory.questItems &= ~0xF0000000;
        gSaveContext.inventory.questItems |= (pohCount << 28);
    }
    Kit::Tooltip("The number of pieces of heart acquired towards the next heart container");

    if (IS_RANDO && (OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_TRIFORCE_HUNT_PIECES_TOTAL) > 0)) {
        ImGui::SeparatorText("Triforce pieces");
        Kit::ResourceBar("##triforcePieces", &gSaveContext.ship.quest.data.randomizer.triforcePiecesCollected, 0, 255,
                         "Triforce pieces: %d", theme);
        Kit::Tooltip("Currently obtained Triforce Pieces. For Triforce Hunt.");
    }
}
#else
void DrawQuestStatusTab() {

    for (int32_t i = QUEST_MEDALLION_FOREST; i < QUEST_MEDALLION_LIGHT + 1; i++) {
        if (i != QUEST_MEDALLION_FOREST) {
            ImGui::SameLine();
        }
        DrawQuestItemButton(i);
    }

    for (int32_t i = QUEST_KOKIRI_EMERALD; i < QUEST_ZORA_SAPPHIRE + 1; i++) {
        if (i != QUEST_KOKIRI_EMERALD) {
            ImGui::SameLine();
        }
        DrawQuestItemButton(i);
    }

    // Put Stone of Agony and Gerudo Card on the same line with a little space between them
    ImGui::SameLine();
    ImGui::Dummy(ImVec2(IMAGE_SIZE, IMAGE_SIZE) + ImGui::GetStyle().FramePadding * 2);

    ImGui::SameLine();
    DrawQuestItemButton(QUEST_STONE_OF_AGONY);

    ImGui::SameLine();
    DrawQuestItemButton(QUEST_GERUDO_CARD);
    for (const auto& [quest, entry] : songMapping) {
        if ((entry.id != QUEST_SONG_MINUET) && (entry.id != QUEST_SONG_LULLABY)) {
            ImGui::SameLine();
        }

        uint32_t bitMask = 1 << entry.id;
        bool hasQuestItem = (bitMask & gSaveContext.inventory.questItems) != 0;
        PushStyleButton(Colors::DarkGray);
        auto ret = ImGui::ImageButton(
            entry.name.c_str(),
            std::dynamic_pointer_cast<Fast::Fast3dGui>(Ship::Context::GetRawInstance()->GetWindow()->GetGui())
                ->GetTextureByName(hasQuestItem ? entry.name : entry.nameFaded),
            ImVec2(32.0f, 48.0f), ImVec2(0, 0), ImVec2(1, 1));
        if (ret) {
            if (hasQuestItem) {
                gSaveContext.inventory.questItems &= ~bitMask;
            } else {
                gSaveContext.inventory.questItems |= bitMask;
            }
        }
        PopStyleButton();
        Tooltip(SohUtils::GetQuestItemName(entry.id).c_str());
    }

    PushStyleInput(THEME_COLOR);
    int tokens = gSaveContext.inventory.gsTokens;
    if (ImGui::SliderInt("Gold Skulltula tokens", &tokens, 0, 100, "%d", ImGuiSliderFlags_NoInput))
        gSaveContext.inventory.gsTokens = static_cast<int16_t>(tokens);
    PopStyleInput();
    InsertHelpHoverText("Number of gold skulltula tokens aquired");

    uint32_t bitMask = 1 << QUEST_SKULL_TOKEN;
    bool gsUnlocked = (bitMask & gSaveContext.inventory.questItems) != 0;
    if (Checkbox("GS unlocked", &gsUnlocked, CheckboxOptions().Color(THEME_COLOR))) {
        if (gsUnlocked) {
            gSaveContext.inventory.questItems |= bitMask;
        } else {
            gSaveContext.inventory.questItems &= ~bitMask;
        }
    }
    InsertHelpHoverText("If unlocked, enables showing the gold skulltula count in the quest status menu");

    int32_t pohCount = (gSaveContext.inventory.questItems & 0xF0000000) >> 28;
    PushStyleCombobox(THEME_COLOR);
    if (ImGui::BeginCombo("PoH count", std::to_string(pohCount).c_str())) {
        for (int32_t i = 0; i < 4; i++) {
            if (ImGui::Selectable(std::to_string(i).c_str(), pohCount == i)) {
                gSaveContext.inventory.questItems &= ~0xF0000000;
                gSaveContext.inventory.questItems |= (i << 28);
            }
        }
        ImGui::EndCombo();
    }
    InsertHelpHoverText("The number of pieces of heart acquired towards the next heart container");
    PopStyleCombobox();

    DrawGroupWithBorder(
        [&]() {
            ImGui::Text("Dungeon Items");

            static int32_t dungeonItemsScene = SCENE_DEKU_TREE;
            static int32_t lastDungeonScene = -1;
            if (gPlayState != nullptr) {
                int32_t sceneNum = gPlayState->sceneNum;
                if (sceneNum >= SCENE_DEKU_TREE && sceneNum <= SCENE_JABU_JABU_BOSS && lastDungeonScene != sceneNum) {
                    dungeonItemsScene = sceneNum;
                    lastDungeonScene = sceneNum;
                }
            }

            PushStyleCombobox(THEME_COLOR);
            if (ImGui::BeginCombo("##DungeonSelect", SohUtils::GetSceneName(dungeonItemsScene).c_str())) {
                for (int32_t dungeonIndex = SCENE_DEKU_TREE; dungeonIndex < SCENE_JABU_JABU_BOSS + 1; dungeonIndex++) {
                    if (ImGui::Selectable(SohUtils::GetSceneName(dungeonIndex).c_str(),
                                          dungeonIndex == dungeonItemsScene)) {
                        dungeonItemsScene = dungeonIndex;
                    }
                }

                ImGui::EndCombo();
            }
            PopStyleCombobox();

            DrawDungeonItemButton(ITEM_KEY_BOSS, dungeonItemsScene);
            ImGui::SameLine();
            DrawDungeonItemButton(ITEM_COMPASS, dungeonItemsScene);
            ImGui::SameLine();
            DrawDungeonItemButton(ITEM_DUNGEON_MAP, dungeonItemsScene);

            if (dungeonItemsScene != SCENE_JABU_JABU_BOSS) {
                float lineHeight = ImGui::GetTextLineHeightWithSpacing();
                ImGui::Image(
                    std::dynamic_pointer_cast<Fast::Fast3dGui>(Ship::Context::GetRawInstance()->GetWindow()->GetGui())
                        ->GetTextureByName(itemMapping[ITEM_KEY_SMALL].name),
                    ImVec2(lineHeight, lineHeight));
                ImGui::SameLine();
                PushStyleInput(THEME_COLOR);
                if (ComboTouch::Count("##Keys", gSaveContext.inventory.dungeonKeys + dungeonItemsScene, -1, 20)) {
                    gSaveContext.ship.stats.dungeonKeys[dungeonItemsScene] =
                        gSaveContext.inventory.dungeonKeys[dungeonItemsScene];
                };
                PopStyleInput();
            } else {
                // dungeonItems is size 20 but dungeonKeys is size 19, so there are no keys for the last scene
                // (Barinade's Lair)
                ImGui::Text("Barinade's Lair does not have small keys");
            }
        },
        "Dungeon Items");
}
#endif

#ifdef COMBO_BUILD
// ComboShip: the dungeon items as MM draws them, one bordered row per dungeon instead of one dungeon at a time from a
// list: Map, Compass and Boss key, the small key icon (faded at no keys) and the small key count. Dungeons 0x00-0x0D
// and the Treasure Box Shop's keys; the boss rooms and collapse scenes hold no items of their own.
void DrawDungeonItemsTab() {
    namespace Kit = ComboSaveEditorKit;
    const ImVec4 theme = ColorValues.at(THEME_COLOR);
    auto gui = std::dynamic_pointer_cast<Fast::Fast3dGui>(Ship::Context::GetRawInstance()->GetWindow()->GetGui());
    static const int32_t scenes[] = {
        SCENE_DEKU_TREE,          SCENE_DODONGOS_CAVERN,      SCENE_JABU_JABU,         SCENE_FOREST_TEMPLE,
        SCENE_FIRE_TEMPLE,        SCENE_WATER_TEMPLE,         SCENE_SPIRIT_TEMPLE,     SCENE_SHADOW_TEMPLE,
        SCENE_BOTTOM_OF_THE_WELL, SCENE_ICE_CAVERN,           SCENE_GANONS_TOWER,      SCENE_GERUDO_TRAINING_GROUND,
        SCENE_THIEVES_HIDEOUT,    SCENE_INSIDE_GANONS_CASTLE, SCENE_TREASURE_BOX_SHOP,
    };
    Kit::BeginCardLayout(2, Kit::kGridWidth * 4);
    for (int32_t scene : scenes) {
        ImGui::PushID(scene);
        Kit::BeginCard("dungeon");
        ImGui::Text("%s", SohUtils::GetSceneName(scene).c_str());
        DrawDungeonItemButton(ITEM_DUNGEON_MAP, scene);
        ImGui::SameLine();
        DrawDungeonItemButton(ITEM_COMPASS, scene);
        ImGui::SameLine();
        DrawDungeonItemButton(ITEM_KEY_BOSS, scene);
        ImGui::SameLine();
        Kit::Icon(gui->GetTextureByName(itemMapping[ITEM_KEY_SMALL].name),
                  gSaveContext.inventory.dungeonKeys[scene] > 0);
        if (Kit::ResourceBar("##keys", gSaveContext.inventory.dungeonKeys + scene, -1, 20, "Small keys: %d", theme)) {
            gSaveContext.ship.stats.dungeonKeys[scene] = gSaveContext.inventory.dungeonKeys[scene];
        }
        Kit::EndCard();
        ImGui::PopID();
    }
    Kit::EndCardLayout();
}
#endif

#ifdef COMBO_BUILD
// ComboShip: the Player tab with MM's cards and labels: Position & rotation, Speed, Age, Equipped gear, Button items
// (with MM's Clear all) and Player state, which also holds the editable state flags that were on Flags.
void DrawPlayerTab() {
    namespace Kit = ComboSaveEditorKit;
    if (gPlayState == nullptr) {
        ImGui::TextWrapped("%s", Kit::kNeedsPlay);
        return;
    }
    Player* player = GET_PLAYER(gPlayState);
    const char* curSword;
    const char* curShield;
    const char* curTunic;
    const char* curBoots;

    switch (player->currentSwordItemId) {
        case ITEM_SWORD_KOKIRI:
            curSword = "Kokiri Sword";
            break;
        case ITEM_SWORD_MASTER:
            curSword = "Master Sword";
            break;
        case ITEM_SWORD_BGS:
            curSword = "Biggoron's Sword";
            break;
        case ITEM_FISHING_POLE:
            curSword = "Fishing Pole";
            break;
        case ITEM_NONE:
            curSword = "None";
            break;
        default:
            curSword = "None";
            break;
    }

    switch (player->currentShield) {
        case PLAYER_SHIELD_NONE:
            curShield = "None";
            break;
        case PLAYER_SHIELD_DEKU:
            curShield = "Deku Shield";
            break;
        case PLAYER_SHIELD_HYLIAN:
            curShield = "Hylian Shield";
            break;
        case PLAYER_SHIELD_MIRROR:
            curShield = "Mirror Shield";
            break;
        default:
            break;
    }

    switch (player->currentTunic) {
        case PLAYER_TUNIC_KOKIRI:
            curTunic = "Kokiri Tunic";
            break;
        case PLAYER_TUNIC_GORON:
            curTunic = "Goron Tunic";
            break;
        case PLAYER_TUNIC_ZORA:
            curTunic = "Zora Tunic";
            break;
        default:
            break;
    }

    switch (player->currentBoots) {
        case PLAYER_BOOTS_KOKIRI:
            curBoots = "Kokiri Boots";
            break;
        case PLAYER_BOOTS_IRON:
            curBoots = "Iron Boots";
            break;
        case PLAYER_BOOTS_HOVER:
            curBoots = "Hover Boots";
            break;
        default:
            break;
    }

    Kit::BeginCardLayout();

    Kit::BeginCard("positionCard");
    ImGui::Text("Position & rotation");
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
    PushStyleInput(THEME_COLOR);
    ImGui::InputScalar("X##Pos", ImGuiDataType_Float, &player->actor.world.pos.x);
    ImGui::InputScalar("Y##Pos", ImGuiDataType_Float, &player->actor.world.pos.y);
    ImGui::InputScalar("Z##Pos", ImGuiDataType_Float, &player->actor.world.pos.z);
    ComboTouch::Count("X rotation", &player->actor.world.rot.x, -32768, 32767);
    ComboTouch::Count("Y rotation", &player->actor.world.rot.y, -32768, 32767);
    ComboTouch::Count("Z rotation", &player->actor.world.rot.z, -32768, 32767);
    InsertHelpHoverText("For Link's rotation in relation to the world");
    ComboTouch::Count("Model X rotation", &player->actor.shape.rot.x, -32768, 32767);
    ComboTouch::Count("Model Y rotation", &player->actor.shape.rot.y, -32768, 32767);
    ComboTouch::Count("Model Z rotation", &player->actor.shape.rot.z, -32768, 32767);
    InsertHelpHoverText("For Link's actual model");
    PopStyleInput();
    ImGui::PopItemWidth();
    Kit::EndCard();

    Kit::BeginCard("speedCard");
    ImGui::Text("Speed");
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
    PushStyleInput(THEME_COLOR);
    ImGui::InputScalar("XZ speed", ImGuiDataType_Float, &player->linearVelocity);
    InsertHelpHoverText("Link's speed along the XZ plane");
    ImGui::InputScalar("Y velocity", ImGuiDataType_Float, &player->actor.velocity.y);
    InsertHelpHoverText("Link's speed along the Y plane. Caps at -20");
    ImGui::InputScalar("Ledge height", ImGuiDataType_Float, &player->yDistToLedge);
    InsertHelpHoverText("Height used to determine whether Link can climb or grab a ledge at the top");
    ComboTouch::Count("Invincibility timer", &player->invincibilityTimer, -128, 127);
    InsertHelpHoverText("Can't take damage while this is nonzero");
    ImGui::InputScalar("Gravity", ImGuiDataType_Float, &player->actor.gravity);
    InsertHelpHoverText("Rate at which Link falls. Default -4.0f");
    PopStyleInput();
    ImGui::PopItemWidth();
    Kit::EndCard();

    Kit::BeginCard("ageCard");
    ImGui::Text("Age");
    PushStyleCombobox(THEME_COLOR);
    if (ImGui::BeginCombo("Link Age on Load", gPlayState->linkAgeOnLoad == 0 ? "Adult" : "Child")) {
        if (ImGui::Selectable("Adult")) {
            gPlayState->linkAgeOnLoad = 0;
        }
        if (ImGui::Selectable("Child")) {
            gPlayState->linkAgeOnLoad = 1;
        }
        ImGui::EndCombo();
    }
    InsertHelpHoverText("This will change Link's age when you load a map");
    PopStyleCombobox();
    Kit::EndCard();

    Kit::BeginCard("gearCard");
    ImGui::Text("Equipped gear");
    PushStyleCombobox(THEME_COLOR);
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
    if (ImGui::BeginCombo("Sword", curSword)) {
        if (ImGui::Selectable("None")) {
            player->currentSwordItemId = static_cast<s8>(ITEM_NONE);
            gSaveContext.equips.buttonItems[0] = static_cast<u8>(ITEM_NONE);
            Inventory_ChangeEquipment(EQUIP_TYPE_SWORD, EQUIP_VALUE_SWORD_NONE);
        }
        if (ImGui::Selectable("Kokiri Sword")) {
            player->currentSwordItemId = static_cast<s8>(ITEM_SWORD_KOKIRI);
            gSaveContext.equips.buttonItems[0] = static_cast<u8>(ITEM_SWORD_KOKIRI);
            Inventory_ChangeEquipment(EQUIP_TYPE_SWORD, EQUIP_VALUE_SWORD_KOKIRI);
        }
        if (ImGui::Selectable("Master Sword")) {
            player->currentSwordItemId = static_cast<s8>(ITEM_SWORD_MASTER);
            gSaveContext.equips.buttonItems[0] = static_cast<u8>(ITEM_SWORD_MASTER);
            Inventory_ChangeEquipment(EQUIP_TYPE_SWORD, EQUIP_VALUE_SWORD_MASTER);
        }
        if (ImGui::Selectable("Biggoron's Sword")) {
            if (gSaveContext.bgsFlag) {
                if (gSaveContext.swordHealth < 8) {
                    gSaveContext.swordHealth = 8;
                }
                player->currentSwordItemId = static_cast<s8>(ITEM_SWORD_BGS);
                gSaveContext.equips.buttonItems[0] = static_cast<u8>(ITEM_SWORD_BGS);
            } else {
                if (gSaveContext.swordHealth < 8) {
                    gSaveContext.swordHealth = 8;
                }
                player->currentSwordItemId = static_cast<s8>(ITEM_SWORD_BGS);
                gSaveContext.equips.buttonItems[0] = static_cast<u8>(ITEM_SWORD_KNIFE);
            }

            Inventory_ChangeEquipment(EQUIP_TYPE_SWORD, EQUIP_VALUE_SWORD_BIGGORON);
        }
        if (ImGui::Selectable("Fishing Pole")) {
            player->currentSwordItemId = static_cast<s8>(ITEM_FISHING_POLE);
            gSaveContext.equips.buttonItems[0] = static_cast<u8>(ITEM_FISHING_POLE);
            Inventory_ChangeEquipment(EQUIP_TYPE_SWORD, EQUIP_VALUE_SWORD_MASTER);
        }
        ImGui::EndCombo();
    }
    if (ImGui::BeginCombo("Shield", curShield)) {
        if (ImGui::Selectable("None")) {
            player->currentShield = PLAYER_SHIELD_NONE;
            Inventory_ChangeEquipment(EQUIP_TYPE_SHIELD, EQUIP_VALUE_SHIELD_NONE);
        }
        if (ImGui::Selectable("Deku Shield")) {
            player->currentShield = PLAYER_SHIELD_DEKU;
            Inventory_ChangeEquipment(EQUIP_TYPE_SHIELD, EQUIP_VALUE_SHIELD_DEKU);
        }
        if (ImGui::Selectable("Hylian Shield")) {
            player->currentShield = PLAYER_SHIELD_HYLIAN;
            Inventory_ChangeEquipment(EQUIP_TYPE_SHIELD, EQUIP_VALUE_SHIELD_HYLIAN);
        }
        if (ImGui::Selectable("Mirror Shield")) {
            player->currentShield = PLAYER_SHIELD_MIRROR;
            Inventory_ChangeEquipment(EQUIP_TYPE_SHIELD, EQUIP_VALUE_SHIELD_MIRROR);
        }
        ImGui::EndCombo();
    }

    if (ImGui::BeginCombo("Tunic", curTunic)) {
        if (ImGui::Selectable("Kokiri Tunic")) {
            player->currentTunic = PLAYER_TUNIC_KOKIRI;
            Inventory_ChangeEquipment(EQUIP_TYPE_TUNIC, EQUIP_VALUE_TUNIC_KOKIRI);
        }
        if (ImGui::Selectable("Goron Tunic")) {
            player->currentTunic = PLAYER_TUNIC_GORON;
            Inventory_ChangeEquipment(EQUIP_TYPE_TUNIC, EQUIP_VALUE_TUNIC_GORON);
        }
        if (ImGui::Selectable("Zora Tunic")) {
            player->currentTunic = PLAYER_TUNIC_ZORA;
            Inventory_ChangeEquipment(EQUIP_TYPE_TUNIC, EQUIP_VALUE_TUNIC_ZORA);
        }
        ImGui::EndCombo();
    }

    if (ImGui::BeginCombo("Boots", curBoots)) {
        if (ImGui::Selectable("Kokiri Boots")) {
            player->currentBoots = PLAYER_BOOTS_KOKIRI;
            Inventory_ChangeEquipment(EQUIP_TYPE_BOOTS, EQUIP_VALUE_BOOTS_KOKIRI);
        }
        if (ImGui::Selectable("Iron Boots")) {
            player->currentBoots = PLAYER_BOOTS_IRON;
            Inventory_ChangeEquipment(EQUIP_TYPE_BOOTS, EQUIP_VALUE_BOOTS_IRON);
        }
        if (ImGui::Selectable("Hover Boots")) {
            player->currentBoots = PLAYER_BOOTS_HOVER;
            Inventory_ChangeEquipment(EQUIP_TYPE_BOOTS, EQUIP_VALUE_BOOTS_HOVER);
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
    PopStyleCombobox();
    Kit::EndCard();

    Kit::BeginCard("buttonsCard");
    ImGui::Text("Button items");
    ImGui::PushItemWidth(ImGui::GetFontSize() * 6);
    PushStyleInput(THEME_COLOR);
    ImU16 one = 1;
    ImGui::InputScalar("B", ImGuiDataType_U8, &gSaveContext.equips.buttonItems[0], &one, NULL);
    ImGui::InputScalar("C-Left", ImGuiDataType_U8, &gSaveContext.equips.buttonItems[1], &one, NULL);
    ImGui::InputScalar("C-Down", ImGuiDataType_U8, &gSaveContext.equips.buttonItems[2], &one, NULL);
    ImGui::InputScalar("C-Right", ImGuiDataType_U8, &gSaveContext.equips.buttonItems[3], &one, NULL);
    if (CVarGetInteger(CVAR_ENHANCEMENT("DpadEquips"), 0)) {
        ImGui::InputScalar("D-pad Up", ImGuiDataType_U8, &gSaveContext.equips.buttonItems[4], &one, NULL);
        ImGui::InputScalar("D-pad Down", ImGuiDataType_U8, &gSaveContext.equips.buttonItems[5], &one, NULL);
        ImGui::InputScalar("D-pad Left", ImGuiDataType_U8, &gSaveContext.equips.buttonItems[6], &one, NULL);
        ImGui::InputScalar("D-pad Right", ImGuiDataType_U8, &gSaveContext.equips.buttonItems[7], &one, NULL);
    }
    PopStyleInput();
    ImGui::PopItemWidth();
    // As MM's Clear All: every C and D-pad button empties; B keeps the sword.
    if (Kit::Button("Clear all##buttons", Kit::kReset)) {
        for (size_t i = 1; i < ARRAY_COUNT(gSaveContext.equips.buttonItems); i++) {
            gSaveContext.equips.buttonItems[i] = ITEM_NONE;
        }
        for (size_t i = 0; i < ARRAY_COUNT(gSaveContext.equips.cButtonSlots); i++) {
            gSaveContext.equips.cButtonSlots[i] = SLOT_NONE;
        }
    }
    Kit::EndCard();

    Kit::BeginCard("stateCard");
    ImGui::Text("Player state");
    uint32_t flags[3] = { player->stateFlags1, player->stateFlags2, player->stateFlags3 };
    std::vector<std::vector<std::string>> flag_strs = { state1, state2, state3 };
    for (int j = 0; j <= 2; j++) {
        for (int i = 0; i <= 31; i++) {
            if (((flags[j] >> i) & 1) != 0) {
                ImGui::Text("%s", flag_strs[j][i].c_str());
            }
        }
    }
    ImGui::Text("Sword state: %d", player->meleeWeaponState);
    if (ImGui::TreeNode("Edit state flags")) {
        ImGui::Text("stateFlags1");
        DrawFlagArray32("stateFlags1", player->stateFlags1, THEME_COLOR);
        ImGui::Text("stateFlags2");
        DrawFlagArray32("stateFlags2", player->stateFlags2, THEME_COLOR);
        ImGui::Text("stateFlags3");
        DrawFlagArray8("stateFlags3", player->stateFlags3, THEME_COLOR);
        ImGui::Text("unk_6AE_rotFlags");
        DrawFlagArray16("unk_6AE_rotFlags", player->unk_6AE_rotFlags, THEME_COLOR);
        ImGui::TreePop();
    }
    Kit::EndCard();

    Kit::EndCardLayout();
}
#else
void DrawPlayerTab() {
    if (gPlayState != nullptr) {
        Player* player = GET_PLAYER(gPlayState);
        const char* curSword;
        const char* curShield;
        const char* curTunic;
        const char* curBoots;

        switch (player->currentSwordItemId) {
            case ITEM_SWORD_KOKIRI:
                curSword = "Kokiri Sword";
                break;
            case ITEM_SWORD_MASTER:
                curSword = "Master Sword";
                break;
            case ITEM_SWORD_BGS:
                curSword = "Biggoron's Sword";
                break;
            case ITEM_FISHING_POLE:
                curSword = "Fishing Pole";
                break;
            case ITEM_NONE:
                curSword = "None";
                break;
            default:
                curSword = "None";
                break;
        }

        switch (player->currentShield) {
            case PLAYER_SHIELD_NONE:
                curShield = "None";
                break;
            case PLAYER_SHIELD_DEKU:
                curShield = "Deku Shield";
                break;
            case PLAYER_SHIELD_HYLIAN:
                curShield = "Hylian Shield";
                break;
            case PLAYER_SHIELD_MIRROR:
                curShield = "Mirror Shield";
                break;
            default:
                break;
        }

        switch (player->currentTunic) {
            case PLAYER_TUNIC_KOKIRI:
                curTunic = "Kokiri Tunic";
                break;
            case PLAYER_TUNIC_GORON:
                curTunic = "Goron Tunic";
                break;
            case PLAYER_TUNIC_ZORA:
                curTunic = "Zora Tunic";
                break;
            default:
                break;
        }

        switch (player->currentBoots) {
            case PLAYER_BOOTS_KOKIRI:
                curBoots = "Kokiri Boots";
                break;
            case PLAYER_BOOTS_IRON:
                curBoots = "Iron Boots";
                break;
            case PLAYER_BOOTS_HOVER:
                curBoots = "Hover Boots";
                break;
            default:
                break;
        }

        ImGui::PushItemWidth(ImGui::GetFontSize() * 6);
        PushStyleInput(THEME_COLOR);
        DrawGroupWithBorder(
            [&]() {
                ImGui::Text("Link's Position");
                ImGui::PushItemWidth(ImGui::GetFontSize() * 12);
                ImGui::InputScalar("X##Pos", ImGuiDataType_Float, &player->actor.world.pos.x);
                ImGui::InputScalar("Y##Pos", ImGuiDataType_Float, &player->actor.world.pos.y);
                ImGui::InputScalar("Z##Pos", ImGuiDataType_Float, &player->actor.world.pos.z);
                ImGui::PopItemWidth();
            },
            "Link's Position");
        ImGui::SameLine();
        DrawGroupWithBorder(
            [&]() {
                ImGui::Text("Link's Rotation");
                InsertHelpHoverText("For Link's rotation in relation to the world");
                ImGui::PushItemWidth(ImGui::GetFontSize() * 12);
                ComboTouch::Count("X##Rot", &player->actor.world.rot.x, -32768, 32767);
                ComboTouch::Count("Y##Rot", &player->actor.world.rot.y, -32768, 32767);
                ComboTouch::Count("Z##Rot", &player->actor.world.rot.z, -32768, 32767);
                ImGui::PopItemWidth();
            },
            "Link's Rotation");
        ImGui::SameLine();
        DrawGroupWithBorder(
            [&]() {
                ImGui::Text("Link's Model Rotation");
                InsertHelpHoverText("For Link's actual model");
                ImGui::PushItemWidth(ImGui::GetFontSize() * 12);
                ComboTouch::Count("X##ModRot", &player->actor.shape.rot.x, -32768, 32767);
                ComboTouch::Count("Y##ModRot", &player->actor.shape.rot.y, -32768, 32767);
                ComboTouch::Count("Z##ModRot", &player->actor.shape.rot.z, -32768, 32767);
                ImGui::PopItemWidth();
            },
            "Link's Model Rotation");

        ImGui::InputScalar("Linear Velocity", ImGuiDataType_Float, &player->linearVelocity);
        InsertHelpHoverText("Link's speed along the XZ plane");

        ImGui::InputScalar("Y Velocity", ImGuiDataType_Float, &player->actor.velocity.y);
        InsertHelpHoverText("Link's speed along the Y plane. Caps at -20");

        ImGui::InputScalar("Wall Height", ImGuiDataType_Float, &player->yDistToLedge);
        InsertHelpHoverText("Height used to determine whether Link can climb or grab a ledge at the top");

        ComboTouch::Count("Invincibility Timer", &player->invincibilityTimer, -128, 127);
        InsertHelpHoverText("Can't take damage while this is nonzero");

        ImGui::InputScalar("Gravity", ImGuiDataType_Float, &player->actor.gravity);
        InsertHelpHoverText("Rate at which Link falls. Default -4.0f");
        PopStyleInput();

        PushStyleCombobox(THEME_COLOR);
        if (ImGui::BeginCombo("Link Age on Load", gPlayState->linkAgeOnLoad == 0 ? "Adult" : "Child")) {
            if (ImGui::Selectable("Adult")) {
                gPlayState->linkAgeOnLoad = 0;
            }
            if (ImGui::Selectable("Child")) {
                gPlayState->linkAgeOnLoad = 1;
            }
            ImGui::EndCombo();
        }
        InsertHelpHoverText("This will change Link's age when you load a map");
        PopStyleCombobox();
        ImGui::Separator();

        DrawGroupWithBorder(
            [&]() {
                PushStyleCombobox(THEME_COLOR);
                ImGui::Text("Link's Current Equipment");
                ImGui::PushItemWidth(ImGui::GetFontSize() * 12);
                if (ImGui::BeginCombo("Sword", curSword)) {
                    if (ImGui::Selectable("None")) {
                        player->currentSwordItemId = static_cast<s8>(ITEM_NONE);
                        gSaveContext.equips.buttonItems[0] = static_cast<u8>(ITEM_NONE);
                        Inventory_ChangeEquipment(EQUIP_TYPE_SWORD, EQUIP_VALUE_SWORD_NONE);
                    }
                    if (ImGui::Selectable("Kokiri Sword")) {
                        player->currentSwordItemId = static_cast<s8>(ITEM_SWORD_KOKIRI);
                        gSaveContext.equips.buttonItems[0] = static_cast<u8>(ITEM_SWORD_KOKIRI);
                        Inventory_ChangeEquipment(EQUIP_TYPE_SWORD, EQUIP_VALUE_SWORD_KOKIRI);
                    }
                    if (ImGui::Selectable("Master Sword")) {
                        player->currentSwordItemId = static_cast<s8>(ITEM_SWORD_MASTER);
                        gSaveContext.equips.buttonItems[0] = static_cast<u8>(ITEM_SWORD_MASTER);
                        Inventory_ChangeEquipment(EQUIP_TYPE_SWORD, EQUIP_VALUE_SWORD_MASTER);
                    }
                    if (ImGui::Selectable("Biggoron's Sword")) {
                        if (gSaveContext.bgsFlag) {
                            if (gSaveContext.swordHealth < 8) {
                                gSaveContext.swordHealth = 8;
                            }
                            player->currentSwordItemId = static_cast<s8>(ITEM_SWORD_BGS);
                            gSaveContext.equips.buttonItems[0] = static_cast<u8>(ITEM_SWORD_BGS);
                        } else {
                            if (gSaveContext.swordHealth < 8) {
                                gSaveContext.swordHealth = 8;
                            }
                            player->currentSwordItemId = static_cast<s8>(ITEM_SWORD_BGS);
                            gSaveContext.equips.buttonItems[0] = static_cast<u8>(ITEM_SWORD_KNIFE);
                        }

                        Inventory_ChangeEquipment(EQUIP_TYPE_SWORD, EQUIP_VALUE_SWORD_BIGGORON);
                    }
                    if (ImGui::Selectable("Fishing Pole")) {
                        player->currentSwordItemId = static_cast<s8>(ITEM_FISHING_POLE);
                        gSaveContext.equips.buttonItems[0] = static_cast<u8>(ITEM_FISHING_POLE);
                        Inventory_ChangeEquipment(EQUIP_TYPE_SWORD, EQUIP_VALUE_SWORD_MASTER);
                    }
                    ImGui::EndCombo();
                }
                if (ImGui::BeginCombo("Shield", curShield)) {
                    if (ImGui::Selectable("None")) {
                        player->currentShield = PLAYER_SHIELD_NONE;
                        Inventory_ChangeEquipment(EQUIP_TYPE_SHIELD, EQUIP_VALUE_SHIELD_NONE);
                    }
                    if (ImGui::Selectable("Deku Shield")) {
                        player->currentShield = PLAYER_SHIELD_DEKU;
                        Inventory_ChangeEquipment(EQUIP_TYPE_SHIELD, EQUIP_VALUE_SHIELD_DEKU);
                    }
                    if (ImGui::Selectable("Hylian Shield")) {
                        player->currentShield = PLAYER_SHIELD_HYLIAN;
                        Inventory_ChangeEquipment(EQUIP_TYPE_SHIELD, EQUIP_VALUE_SHIELD_HYLIAN);
                    }
                    if (ImGui::Selectable("Mirror Shield")) {
                        player->currentShield = PLAYER_SHIELD_MIRROR;
                        Inventory_ChangeEquipment(EQUIP_TYPE_SHIELD, EQUIP_VALUE_SHIELD_MIRROR);
                    }
                    ImGui::EndCombo();
                }

                if (ImGui::BeginCombo("Tunic", curTunic)) {
                    if (ImGui::Selectable("Kokiri Tunic")) {
                        player->currentTunic = PLAYER_TUNIC_KOKIRI;
                        Inventory_ChangeEquipment(EQUIP_TYPE_TUNIC, EQUIP_VALUE_TUNIC_KOKIRI);
                    }
                    if (ImGui::Selectable("Goron Tunic")) {
                        player->currentTunic = PLAYER_TUNIC_GORON;
                        Inventory_ChangeEquipment(EQUIP_TYPE_TUNIC, EQUIP_VALUE_TUNIC_GORON);
                    }
                    if (ImGui::Selectable("Zora Tunic")) {
                        player->currentTunic = PLAYER_TUNIC_ZORA;
                        Inventory_ChangeEquipment(EQUIP_TYPE_TUNIC, EQUIP_VALUE_TUNIC_ZORA);
                    }
                    ImGui::EndCombo();
                }

                if (ImGui::BeginCombo("Boots", curBoots)) {
                    if (ImGui::Selectable("Kokiri Boots")) {
                        player->currentBoots = PLAYER_BOOTS_KOKIRI;
                        Inventory_ChangeEquipment(EQUIP_TYPE_BOOTS, EQUIP_VALUE_BOOTS_KOKIRI);
                    }
                    if (ImGui::Selectable("Iron Boots")) {
                        player->currentBoots = PLAYER_BOOTS_IRON;
                        Inventory_ChangeEquipment(EQUIP_TYPE_BOOTS, EQUIP_VALUE_BOOTS_IRON);
                    }
                    if (ImGui::Selectable("Hover Boots")) {
                        player->currentBoots = PLAYER_BOOTS_HOVER;
                        Inventory_ChangeEquipment(EQUIP_TYPE_BOOTS, EQUIP_VALUE_BOOTS_HOVER);
                    }
                    ImGui::EndCombo();
                }
                ImGui::PopItemWidth();
                PopStyleCombobox();
            },
            "Current Equipment");
        ImGui::SameLine();

        ImU16 one = 1;
        DrawGroupWithBorder(
            [&]() {
                ImGui::PushItemWidth(ImGui::GetFontSize() * 6);
                PushStyleInput(THEME_COLOR);
                ImGui::Text("Current Items");
                ImGui::InputScalar("B Button", ImGuiDataType_U8, &gSaveContext.equips.buttonItems[0], &one, NULL);
                ImGui::InputScalar("C Left", ImGuiDataType_U8, &gSaveContext.equips.buttonItems[1], &one, NULL);
                ImGui::InputScalar("C Down", ImGuiDataType_U8, &gSaveContext.equips.buttonItems[2], &one, NULL);
                ImGui::InputScalar("C Right", ImGuiDataType_U8, &gSaveContext.equips.buttonItems[3], &one, NULL);
                PopStyleInput();
                ImGui::PopItemWidth();
            },
            "Current Items");

        if (CVarGetInteger(CVAR_ENHANCEMENT("DpadEquips"), 0)) {
            ImGui::SameLine();
            DrawGroupWithBorder(
                [&]() {
                    ImGui::PushItemWidth(ImGui::GetFontSize() * 6);
                    PushStyleInput(THEME_COLOR);
                    ImGui::Text("Current D-pad Items");
                    // Two spaces at the end for aligning, not elegant but it's working
                    ImGui::InputScalar("D-pad Up  ", ImGuiDataType_U8, &gSaveContext.equips.buttonItems[4], &one, NULL);
                    ImGui::InputScalar("D-pad Down", ImGuiDataType_U8, &gSaveContext.equips.buttonItems[5], &one, NULL);
                    ImGui::InputScalar("D-pad Left", ImGuiDataType_U8, &gSaveContext.equips.buttonItems[6], &one, NULL);
                    ImGui::InputScalar("D-pad Right", ImGuiDataType_U8, &gSaveContext.equips.buttonItems[7], &one,
                                       NULL);
                    PopStyleInput();
                    ImGui::PopItemWidth();
                },
                "Current D-pad Items");
        }

        ImGui::Text("Player State");
        uint8_t bit[32] = {};
        uint32_t flags[3] = { player->stateFlags1, player->stateFlags2, player->stateFlags3 };
        std::vector<std::vector<std::string>> flag_strs = { state1, state2, state3 };

        for (int j = 0; j <= 2; j++) {
            std::string label = fmt::format("State Flags {}", j + 1);
            DrawGroupWithBorder(
                [&]() {
                    ImGui::Text("%s", label.c_str());
                    std::vector<std::string> state = flag_strs[j];
                    for (int i = 0; i <= 31; i++) {
                        bit[i] = ((flags[j] >> i) & 1);
                        if (bit[i] != 0) {
                            ImGui::Text("%s", state[i].c_str());
                        }
                    }
                },
                label.c_str());
            ImGui::SameLine();
        }
        DrawGroupWithBorder(
            [&]() {
                ImGui::Text("Sword");
                ImGui::Text("  %d", player->meleeWeaponState);
            },
            "Sword");

    } else {
        ImGui::Text("Global Context needed for player info!");
    }
}
#endif

void ResetBaseOptions() {
    intSliderOptionsBase.Color(THEME_COLOR).Size({ 320.0f, 0.0f }).Tooltip("");
    buttonOptionsBase.Color(THEME_COLOR).Size(Sizes::Inline).Tooltip("");
    checkboxOptionsBase.Color(THEME_COLOR).Tooltip("");
    comboboxOptionsBase.Color(THEME_COLOR)
        .ComponentAlignment(ComponentAlignments::Left)
        .LabelPosition(LabelPositions::Near)
        .Tooltip("");
}

#ifdef COMBO_BUILD
void SaveEditorWindow::DrawElement() {
    // ComboShip: no editable tabs on the title and file screens, the same one line as MM.
    if (gSaveContext.gameMode == GAMEMODE_TITLE_SCREEN || gSaveContext.gameMode == GAMEMODE_FILE_SELECT) {
        ImGui::TextWrapped("%s", ComboSaveEditorKit::kNoOotFile);
        return;
    }
    PushStyleTabs(THEME_COLOR);
    ImGui::BeginDisabled(CVarGetInteger(CVAR_SETTING("DisableChanges"), 0));

    // ComboShip: the same tabs, in the same order and in the same bordered panel, as the MM Save Editor; the menu
    // font instead of the monospace one.
    if (ImGui::BeginTabBar("SaveContextTabBar", ImGuiTabBarFlags_NoCloseWithMiddleMouseButton)) {
        using namespace ComboSaveEditorKit;
        const std::pair<const char*, void (*)()> tabs[] = {
            { kTabGeneral, DrawInfoTab },
            { kTabInventory, DrawInventoryTab },
            { kTabEquipment, DrawEquipmentTab },
            { kTabQuestStatus, DrawQuestStatusTab },
            { kTabDungeonItems, DrawDungeonItemsTab },
            { kTabFlags, DrawFlagsTab },
            { kTabPlayer, DrawPlayerTab },
        };
        for (const auto& [name, draw] : tabs) {
            ResetBaseOptions();
            if (ImGui::BeginTabItem(name)) {
                BeginPanel(name);
                draw();
                EndPanel();
                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();
    }

    ImGui::EndDisabled();
    PopStyleTabs();
}
#else
void SaveEditorWindow::DrawElement() {
    PushStyleTabs(THEME_COLOR);
    ImGui::PushFont(OTRGlobals::Instance->fontMonoLarger);
    ImGui::BeginDisabled(CVarGetInteger(CVAR_SETTING("DisableChanges"), 0));

    if (ImGui::BeginTabBar("SaveContextTabBar", ImGuiTabBarFlags_NoCloseWithMiddleMouseButton)) {
        ResetBaseOptions();
        if (ImGui::BeginTabItem("Info")) {
            DrawInfoTab();
            ImGui::EndTabItem();
        }

        ResetBaseOptions();
        if (ImGui::BeginTabItem("Inventory")) {
            DrawInventoryTab();
            ImGui::EndTabItem();
        }

        ResetBaseOptions();
        if (ImGui::BeginTabItem("Flags")) {
            DrawFlagsTab();
            ImGui::EndTabItem();
        }

        ResetBaseOptions();
        if (ImGui::BeginTabItem("Equipment")) {
            DrawEquipmentTab();
            ImGui::EndTabItem();
        }

        ResetBaseOptions();
        if (ImGui::BeginTabItem("Quest Status")) {
            DrawQuestStatusTab();
            ImGui::EndTabItem();
        }

        ResetBaseOptions();
        if (ImGui::BeginTabItem("Player")) {
            DrawPlayerTab();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::EndDisabled();
    ImGui::PopFont();
    PopStyleTabs();
}
#endif

void SaveEditorWindow::InitElement() {
    std::dynamic_pointer_cast<Fast::Fast3dGui>(Ship::Context::GetRawInstance()->GetWindow()->GetGui())
        ->LoadGuiTexture("ROCS_FEATHER", gRocsFeatherTex, "", ImVec4(1, 1, 1, 1));
}
