// combo/gui/ComboMenuPlacement.h
//
// ComboShip: one catalogue of the non-Randomizer menu pages, their fixed heading order, and where
// each native widget sits on them, so a feature both games have lands on one tab, page and heading
// in both world views. MenuGroupRoute reads it first, so the page list, the page body and search
// agree; its older clauses stay as the fallback for rows not listed here.
//
// The rows were generated once from the T8 audit's menu-placements.json and are edited by hand
// from now on. Keys are the games' own CVars: nothing here renames a CVar, a choice or a rule.
//   "<CVar>"                              one native widget (a trailing '*' matches a CVar prefix)
//   "widget:<section>/<sidebar>/<name>"   a native widget without a CVar (text, button, inline window)
//   "heading:<section>/<sidebar>/<name>"  the rows under that native heading that have no row of their own
//   "sidebar:<section>/<sidebar>"         the rows of that native sidebar that have no row of their own
//   "rule:<CVar>"                         a combo-drawn gameplay rule (combo/rando/GameplayRules.h)
//   "pair:<label>"                        a combo-drawn checkbox that sets one CVar in each game
// rank orders rows inside a heading; cross-game equivalents share a rank so both views line up.
// after keeps a row directly under that CVar, which hides or disables it.
#pragma once
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ComboRando {
// Every non-Randomizer page name. Randomizer pages stay in kRandomizerPages.
namespace MenuPage {
inline constexpr const char* CommunityPresets = "Community presets";
inline constexpr const char* ItemsEquipment = "Items & equipment";
inline constexpr const char* WarpsTravel = "Warps & travel";
inline constexpr const char* TimeSavers = "Time savers";
inline constexpr const char* Saving = "Saving";
inline constexpr const char* QualityOfLife = "Quality of life";
inline constexpr const char* Difficulty = "Difficulty";
inline constexpr const char* Fixes = "Fixes";
inline constexpr const char* Restorations = "Restorations";
inline constexpr const char* ExtraModes = "Extra modes";
inline constexpr const char* Cheats = "Cheats";
inline constexpr const char* RandomizerExtras = "Randomizer extras";
inline constexpr const char* ScreenHud = "Screen & HUD";
inline constexpr const char* Camera = "Camera";
inline constexpr const char* Graphics = "Graphics";
inline constexpr const char* VisualEnhancements = "Visual enhancements";
inline constexpr const char* Appearance = "Appearance";
inline constexpr const char* HudOverlays = "HUD & overlays"; // retired: its rows move to Settings and Screen & HUD
inline constexpr const char* Audio = "Audio";
inline constexpr const char* MusicSounds = "Music & sounds";
inline constexpr const char* CheckTracker = "Check Tracker";
inline constexpr const char* ItemTracker = "Item Tracker";
inline constexpr const char* HintTracker = "Hint Tracker";
inline constexpr const char* EntranceTracker = "Entrance Tracker";
inline constexpr const char* Timers = "Timers";
inline constexpr const char* TimeSplits = "Time Splits";
inline constexpr const char* GameplayStats = "Gameplay Stats";
inline constexpr const char* General = "General";
inline constexpr const char* Controls = "Controls";
inline constexpr const char* InputViewer = "Input Viewer";
inline constexpr const char* Interface = "Interface";
inline constexpr const char* Notifications = "Notifications";
inline constexpr const char* About = "About Combo Ship";
inline constexpr const char* SaveEditor = "Save Editor";
inline constexpr const char* EnhancementPresets = "Enhancement presets"; // both games' saved preset managers
inline constexpr const char* Mods = "Mods";
inline constexpr const char* Multiplayer = "Multiplayer";
inline constexpr const char* DeveloperOptions = "Developer options";
inline constexpr const char* Stats = "Stats";
inline constexpr const char* Console = "Console";
inline constexpr const char* HookDebugger = "Hook Debugger";
inline constexpr const char* CollisionViewer = "Collision Viewer";
inline constexpr const char* ActorViewer = "Actor Viewer";
inline constexpr const char* DisplayLists = "Display lists";
inline constexpr const char* ValueViewer = "Value Viewer";
inline constexpr const char* MessageViewer = "Message Viewer";
inline constexpr const char* GfxDebugger = "Gfx Debugger";
inline constexpr const char* EventLog = "Event Log";
inline constexpr const char* Session = "Session";
} // namespace MenuPage

enum : uint8_t { kMenuOot = 1, kMenuMm = 2, kMenuBoth = 3 };

struct MenuPageLayout {
    const char* scope;
    const char* page;
    const char* scopeLine; // shown under the title; "" when the world buttons say whose rows these are
    int headingCount;
    int split; // first heading of the second column on wide screens; headingCount keeps one column
    const char* headings[10];
};

struct MenuPlacement {
    uint8_t games;
    const char* key;
    const char* scope; // MenuDestination scope
    const char* page;
    const char* heading; // nullptr keeps the row's native heading
    int rank;
    const char* after;   // CVar this row stays directly under, or nullptr
    const char* feature; // cross-game feature id from the audit, or nullptr
    uint8_t mirrored;    // the feature works the same in both games: both rows land together
};

// clang-format off
inline constexpr MenuPageLayout kMenuPages[] = {
    { "gameplay", "Items & equipment", "", 8, 4, { "Equipping", "Weapons & gear", "Bow & arrows", "Explosives & projectiles", "Hookshot, boomerang & tools", "Masks & forms", "Ocarina & songs", "Magic & bottles" } },
    { "gameplay", "Warps & travel", "", 2, 2, { "Cross-game songs", "Warping" } },
    { "gameplay", "Time savers", "", 4, 2, { "Cutscenes", "Text & dialogue", "Speed-ups", "Quests & errands" } },
    { "gameplay", "Saving", "", 3, 1, { "Autosave", "Save & reload", "File select" } },
    { "gameplay", "Quality of life", "", 4, 1, { "Time of day & cycle", "Shops & rewards", "World & NPCs", "Pause menu & messages" } },
    { "gameplay", "Difficulty", "", 6, 4, { "Health & damage", "Enemies & bosses", "Drops", "Quest requirements", "Minigames", "World" } },
    { "gameplay", "Fixes", "", 6, 2, { "Gameplay", "Items & combat", "Camera", "Graphics", "Audio", "Timing" } },
    { "gameplay", "Restorations", "", 4, 2, { "Movement & combat", "Glitches", "World", "Visual & audio" } },
    { "gameplay", "Extra modes", "", 5, 3, { "World", "Player & companions", "Challenges", "Traps", "Enemy randomizer" } },
    { "gameplay", "Cheats", "", 9, 4, { "Infinite resources", "Items & equipment", "Movement", "World & enemies", "Time", "Speed modifier", "Glitch aids", "Save states", "Beta Quest" } },
    { "gameplay", "Randomizer extras", "Randomizer files only. No seed or logic effect.", 2, 2, { "Item appearance", "Hints & messages" } },
    { "media", "Screen & HUD", "", 3, 2, { "Screen fit", "HUD layout", "Advanced HUD editor (MM)" } },
    { "media", "Camera", "", 2, 2, { "First person & aiming", "Third-person camera" } },
    { "media", "Graphics", "Applies to both games.", 0, 0, {  } },
    { "media", "Visual enhancements", "", 7, 3, { "Screen & effects", "Models & textures", "HUD & interface", "Draw distance", "Clock", "Motion blur", "Mods" } },
    { "media", "Appearance", "", 1, 1, { "Editor" } },
    { "media", "Audio", "Volume applies to both games.", 2, 2, { "Volume", "Ambience" } },
    { "media", "Music & sounds", "", 1, 1, { "Editor" } },
    { "settings", "General", "", 4, 2, { "Boot", "Accessibility", "Languages", "Shortcuts" } },
    { "settings", "Controls", "", 4, 2, { "Buttons & aiming", "Ocarina input", "Input timing", "Controller mapping" } },
    { "settings", "Input Viewer", "Applies to both games.", 0, 0, {  } },
    { "settings", "Notifications", "", 0, 0, {  } },
    { "trackers", "Gameplay Stats", "Ocarina of Time only.", 1, 1, { "Window" } },
    { "trackers", "Time Splits", "", 1, 1, { "Window" } },
};

inline constexpr MenuPlacement kMenuPlacements[] = {
    { kMenuOot, "gCheats.EnableBetaQuest", "gameplay", "Cheats", "Beta Quest", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gCheats.BetaQuestWorld", "gameplay", "Cheats", "Beta Quest", 1010, "gCheats.EnableBetaQuest", nullptr, 0 },
    { kMenuOot, "gCheats.EasyISG", "gameplay", "Cheats", "Glitch aids", 10, nullptr, "isg", 0 },
    { kMenuMm, "gCheats.EasyFrameAdvance", "gameplay", "Cheats", "Glitch aids", 20, nullptr, "frame_advance", 1 },
    { kMenuOot, "gCheats.EasyFrameAdvance", "gameplay", "Cheats", "Glitch aids", 20, nullptr, "frame_advance", 1 },
    { kMenuOot, "gCheats.EasyQPA", "gameplay", "Cheats", "Glitch aids", 1000, nullptr, nullptr, 0 },
    { kMenuMm, "gCheats.InfiniteHealth", "gameplay", "Cheats", "Infinite resources", 10, nullptr, "infinite_health", 1 },
    { kMenuOot, "gCheats.InfiniteHealth", "gameplay", "Cheats", "Infinite resources", 10, nullptr, "infinite_health", 1 },
    { kMenuMm, "gCheats.InfiniteMagic", "gameplay", "Cheats", "Infinite resources", 20, nullptr, "infinite_magic", 1 },
    { kMenuOot, "gCheats.InfiniteMagic", "gameplay", "Cheats", "Infinite resources", 20, nullptr, "infinite_magic", 1 },
    { kMenuMm, "gCheats.InfiniteRupees", "gameplay", "Cheats", "Infinite resources", 30, nullptr, "infinite_money", 1 },
    { kMenuOot, "gCheats.InfiniteMoney", "gameplay", "Cheats", "Infinite resources", 30, nullptr, "infinite_money", 1 },
    { kMenuMm, "gCheats.InfiniteConsumables", "gameplay", "Cheats", "Infinite resources", 40, nullptr, "infinite_ammo", 1 },
    { kMenuOot, "gCheats.InfiniteAmmo", "gameplay", "Cheats", "Infinite resources", 40, nullptr, "infinite_ammo", 1 },
    { kMenuMm, "gCheats.InfiniteEponaCarrots", "gameplay", "Cheats", "Infinite resources", 50, nullptr, "infinite_epona", 1 },
    { kMenuOot, "gCheats.InfiniteEponaBoost", "gameplay", "Cheats", "Infinite resources", 50, nullptr, "infinite_epona", 1 },
    { kMenuOot, "gCheats.InfiniteNayru", "gameplay", "Cheats", "Infinite resources", 1000, nullptr, nullptr, 0 },
    { kMenuMm, "gCheats.UnrestrictedItems", "gameplay", "Cheats", "Items & equipment", 10, nullptr, "unrestricted_items", 1 },
    { kMenuOot, "gCheats.NoRestrictItems", "gameplay", "Cheats", "Items & equipment", 10, nullptr, "unrestricted_items", 1 },
    { kMenuMm, "gCheats.HookshotAnywhere", "gameplay", "Cheats", "Items & equipment", 20, nullptr, "hookshot_anything", 1 },
    { kMenuOot, "gCheats.HookshotEverything", "gameplay", "Cheats", "Items & equipment", 20, nullptr, "hookshot_anything", 1 },
    { kMenuMm, "gCheats.UnbreakableRazorSword", "gameplay", "Cheats", "Items & equipment", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gCheats.TimelessEquipment", "gameplay", "Cheats", "Items & equipment", 1000, nullptr, nullptr, 0 },
    { kMenuMm, "gCheats.ElegyAnywhere", "gameplay", "Cheats", "Items & equipment", 1010, nullptr, nullptr, 0 },
    { kMenuOot, "gCheats.FireproofDekuShield", "gameplay", "Cheats", "Items & equipment", 1010, nullptr, nullptr, 0 },
    { kMenuOot, "gCheats.ShieldTwoHanded", "gameplay", "Cheats", "Items & equipment", 1020, nullptr, nullptr, 0 },
    { kMenuOot, "gCheats.BombTimerMultiplier", "gameplay", "Cheats", "Items & equipment", 1030, nullptr, nullptr, 0 },
    { kMenuOot, "gCheats.HookshotReachMultiplier", "gameplay", "Cheats", "Items & equipment", 1040, nullptr, nullptr, 0 },
    { kMenuMm, "gCheats.NoClip", "gameplay", "Cheats", "Movement", 10, nullptr, "no_clip", 1 },
    { kMenuOot, "gCheats.NoClip", "gameplay", "Cheats", "Movement", 10, nullptr, "no_clip", 1 },
    { kMenuMm, "gCheats.MoonJumpOnL", "gameplay", "Cheats", "Movement", 20, nullptr, "moon_jump", 1 },
    { kMenuOot, "gCheats.MoonJumpOnL", "gameplay", "Cheats", "Movement", 20, nullptr, "moon_jump", 1 },
    { kMenuMm, "gCheats.ClimbAnywhere", "gameplay", "Cheats", "Movement", 30, nullptr, "climb_anything", 1 },
    { kMenuOot, "gCheats.ClimbEverything", "gameplay", "Cheats", "Movement", 30, nullptr, "climb_anything", 1 },
    { kMenuMm, "gCheats.LongerFlowerGlide", "gameplay", "Cheats", "Movement", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gCheats.SaveStatePromise", "gameplay", "Cheats", "Save states", 1020, nullptr, nullptr, 0 },
    { kMenuOot, "gCheats.SaveStatesEnabled", "gameplay", "Cheats", "Save states", 1030, "gCheats.SaveStatePromise", nullptr, 0 },
    { kMenuMm, "gCheats.SpeedModifier.Mode", "gameplay", "Cheats", "Speed modifier", 10, nullptr, "speed_modifier", 1 },
    { kMenuMm, "gCheats.SpeedModifier.Value", "gameplay", "Cheats", "Speed modifier", 10, "gCheats.SpeedModifier.Mode", "speed_modifier", 1 },
    { kMenuMm, "gCheats.SpeedModifier.Btn", "gameplay", "Cheats", "Speed modifier", 10, "gCheats.SpeedModifier.Mode", "speed_modifier", 1 },
    { kMenuOot, "gCheats.SpeedModifier.SpeedToggle", "gameplay", "Cheats", "Speed modifier", 10, nullptr, "speed_modifier", 1 },
    { kMenuOot, "gCheats.SpeedModifier.DoesntChangeJump", "gameplay", "Cheats", "Speed modifier", 10, nullptr, "speed_modifier", 1 },
    { kMenuOot, "gCheats.SpeedModifier.Value", "gameplay", "Cheats", "Speed modifier", 10, nullptr, "speed_modifier", 1 },
    { kMenuOot, "gCheats.SpeedModifier.Btn", "gameplay", "Cheats", "Speed modifier", 10, nullptr, "speed_modifier", 1 },
    { kMenuMm, "gCheats.TempleTimeStop", "gameplay", "Cheats", "Time", 10, nullptr, "time_stop", 0 },
    { kMenuOot, "gCheats.FreezeTime", "gameplay", "Cheats", "Time", 10, nullptr, "time_stop", 0 },
    { kMenuOot, "gCheats.TimeSync", "gameplay", "Cheats", "Time", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gCheats.NoRedeadFreeze", "gameplay", "Cheats", "World & enemies", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gCheats.NoKeeseGuayTarget", "gameplay", "Cheats", "World & enemies", 1010, nullptr, nullptr, 0 },
    { kMenuOot, "gCheats.DisableSandstorm", "gameplay", "Cheats", "World & enemies", 1020, nullptr, nullptr, 0 },
    { kMenuOot, "gCheats.GSTargetable", "gameplay", "Cheats", "World & enemies", 1030, nullptr, nullptr, 0 },
    { kMenuOot, "gCheats.DropsDontDie", "gameplay", "Cheats", "World & enemies", 1040, nullptr, nullptr, 0 },
    { kMenuOot, "gCheats.NoFishDespawn", "gameplay", "Cheats", "World & enemies", 1050, nullptr, nullptr, 0 },
    { kMenuOot, "gCheats.NoBugsDespawn", "gameplay", "Cheats", "World & enemies", 1060, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Equipment.ChuDrops", "gameplay", "Difficulty", "Drops", 10, nullptr, "bombchu_drops", 1 },
    { kMenuOot, "gEnhancements.EnableBombchuDrops", "gameplay", "Difficulty", "Drops", 10, nullptr, "bombchu_drops", 1 },
    { kMenuMm, "gEnhancements.DifficultyOptions.NoHeartDrops", "gameplay", "Difficulty", "Drops", 20, nullptr, "no_heart_drops", 1 },
    { kMenuOot, "gEnhancements.NoHeartDrops", "gameplay", "Difficulty", "Drops", 20, nullptr, "no_heart_drops", 1 },
    { kMenuMm, "gEnhancements.DifficultyOptions.NoRandomDrops", "gameplay", "Difficulty", "Drops", 30, nullptr, "no_random_drops", 1 },
    { kMenuOot, "gEnhancements.NoRandomDrops", "gameplay", "Difficulty", "Drops", 30, nullptr, "no_random_drops", 1 },
    { kMenuOot, "gEnhancements.TreesDropSticks", "gameplay", "Difficulty", "Drops", 1000, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.DifficultyOptions.HyperEnemies", "gameplay", "Difficulty", "Enemies & bosses", 10, nullptr, "hyper_enemies", 1 },
    { kMenuOot, "gEnhancements.HyperEnemies", "gameplay", "Difficulty", "Enemies & bosses", 10, nullptr, "hyper_enemies", 1 },
    { kMenuMm, "gEnhancements.DifficultyOptions.BossHealthMultiplier", "gameplay", "Difficulty", "Enemies & bosses", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.HyperBosses", "gameplay", "Difficulty", "Enemies & bosses", 1000, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.DifficultyOptions.DisableTakkuriSteal", "gameplay", "Difficulty", "Enemies & bosses", 1010, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.GuardVision", "gameplay", "Difficulty", "Enemies & bosses", 1010, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.LeeverSpawnRate", "gameplay", "Difficulty", "Enemies & bosses", 1020, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.DifficultyOptions.DamageMultiplier", "gameplay", "Difficulty", "Health & damage", 10, nullptr, "damage_mult", 1 },
    { kMenuOot, "gEnhancements.DamageMult", "gameplay", "Difficulty", "Health & damage", 10, nullptr, "damage_mult", 1 },
    { kMenuMm, "gEnhancements.DifficultyOptions.PermanentHeartLoss", "gameplay", "Difficulty", "Health & damage", 20, nullptr, "perm_heart_loss", 1 },
    { kMenuOot, "gEnhancements.PermanentHeartLoss", "gameplay", "Difficulty", "Health & damage", 20, nullptr, "perm_heart_loss", 1 },
    { kMenuMm, "gEnhancements.DifficultyOptions.DeleteFileOnDeath", "gameplay", "Difficulty", "Health & damage", 30, nullptr, "delete_on_death", 1 },
    { kMenuOot, "gEnhancements.DeleteFileOnDeath", "gameplay", "Difficulty", "Health & damage", 30, nullptr, "delete_on_death", 1 },
    { kMenuMm, "gEnhancements.DifficultyOptions.JinxedTimer", "gameplay", "Difficulty", "Health & damage", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.FallDamageMult", "gameplay", "Difficulty", "Health & damage", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.VoidDamageMult", "gameplay", "Difficulty", "Health & damage", 1010, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.BonkDamageMult", "gameplay", "Difficulty", "Health & damage", 1020, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.FullHealthSpawn", "gameplay", "Difficulty", "Health & damage", 1030, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Minigames.MarkShootingGalleryOctoroks", "gameplay", "Difficulty", "Minigames", 10, nullptr, "minigame_aids", 0 },
    { kMenuMm, "gEnhancements.Minigames.TownArcheryScore", "gameplay", "Difficulty", "Minigames", 10, nullptr, "minigame_aids", 0 },
    { kMenuMm, "gEnhancements.Minigames.RandomizeShootingGalleryOctoroks", "gameplay", "Difficulty", "Minigames", 10, nullptr, "minigame_aids", 0 },
    { kMenuOot, "gEnhancements.CustomizeShootingGallery", "gameplay", "Difficulty", "Minigames", 10, nullptr, "minigame_aids", 0 },
    { kMenuMm, "gEnhancements.Minigames.BombersHideAndSeek", "gameplay", "Difficulty", "Minigames", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.InstantShootingGalleryWin", "gameplay", "Difficulty", "Minigames", 1000, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Minigames.SwordsmanSchoolScore", "gameplay", "Difficulty", "Minigames", 1010, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.ConstantAdultGallery", "gameplay", "Difficulty", "Minigames", 1010, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Minigames.HoneyAndDarlingDay1", "gameplay", "Difficulty", "Minigames", 1020, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.ShootingGalleryAmmoChild", "gameplay", "Difficulty", "Minigames", 1020, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Minigames.HoneyAndDarlingDay2", "gameplay", "Difficulty", "Minigames", 1030, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.ShootingGalleryAmmoAdult", "gameplay", "Difficulty", "Minigames", 1030, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Minigames.HoneyAndDarlingDay3", "gameplay", "Difficulty", "Minigames", 1040, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.CustomizeBombchuBowling", "gameplay", "Difficulty", "Minigames", 1040, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Minigames.SwampArcheryScore", "gameplay", "Difficulty", "Minigames", 1050, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.BombchuBowlingNoSmallCucco", "gameplay", "Difficulty", "Minigames", 1050, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Minigames.RomaniTargetPractice", "gameplay", "Difficulty", "Minigames", 1060, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.BombchuBowlingNoBigCucco", "gameplay", "Difficulty", "Minigames", 1060, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Minigames.AlwaysWinDoggyRace", "gameplay", "Difficulty", "Minigames", 1070, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.BombchuBowlingAmmo", "gameplay", "Difficulty", "Minigames", 1070, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Minigames.SkipHorseRace", "gameplay", "Difficulty", "Minigames", 1080, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.CustomizeHorsebackArchery", "gameplay", "Difficulty", "Minigames", 1080, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Minigames.BeaverRaceRingsCollected", "gameplay", "Difficulty", "Minigames", 1090, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.InstantHorsebackArcheryWin", "gameplay", "Difficulty", "Minigames", 1090, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Minigames.SkipLittleBeaver", "gameplay", "Difficulty", "Minigames", 1100, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.HorsebackArcheryAlwaysScore", "gameplay", "Difficulty", "Minigames", 1100, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.DifficultyOptions.GoronRace", "gameplay", "Difficulty", "Minigames", 1110, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.HorsebackArcheryAmmo", "gameplay", "Difficulty", "Minigames", 1110, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Minigames.BoatArcheryScore", "gameplay", "Difficulty", "Minigames", 1120, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.CustomizeFrogsOcarinaGame", "gameplay", "Difficulty", "Minigames", 1120, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Minigames.BoatArcheryHealth", "gameplay", "Difficulty", "Minigames", 1130, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.InstantFrogsGameWin", "gameplay", "Difficulty", "Minigames", 1130, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Minigames.BoatArcheryInvincible", "gameplay", "Difficulty", "Minigames", 1140, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.FrogsUnlimitedFailTime", "gameplay", "Difficulty", "Minigames", 1140, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Minigames.TreasureChestShopShowFullMaze", "gameplay", "Difficulty", "Minigames", 1150, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.FrogsModifyFailTime", "gameplay", "Difficulty", "Minigames", 1150, "gEnhancements.CustomizeFrogsOcarinaGame", nullptr, 0 },
    { kMenuOot, "gEnhancements.CustomizeOcarinaGame", "gameplay", "Difficulty", "Minigames", 1160, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.InstantOcarinaGameWin", "gameplay", "Difficulty", "Minigames", 1170, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.OcarinaGame.NoteSpeed", "gameplay", "Difficulty", "Minigames", 1180, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.DampeDropRate", "gameplay", "Difficulty", "Minigames", 1190, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.OcarinaUnlimitedFailTime", "gameplay", "Difficulty", "Minigames", 1200, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.OcarinaGame.StartingNotes", "gameplay", "Difficulty", "Minigames", 1210, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.OcarinaGame.RoundOneNotes", "gameplay", "Difficulty", "Minigames", 1220, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.OcarinaGame.RoundTwoNotes", "gameplay", "Difficulty", "Minigames", 1230, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.GoronPot", "gameplay", "Difficulty", "Minigames", 1240, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.OcarinaGame.RoundThreeNotes", "gameplay", "Difficulty", "Minigames", 1250, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.DampeWin", "gameplay", "Difficulty", "Minigames", 1260, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.IngoRaceOnce", "gameplay", "Difficulty", "Minigames", 1270, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.DivingGame.TimeLimit", "gameplay", "Difficulty", "Minigames", 1280, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.CustomizeFishing", "gameplay", "Difficulty", "Minigames", 1290, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.InstantFishing", "gameplay", "Difficulty", "Minigames", 1300, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.GuaranteeFishingBite", "gameplay", "Difficulty", "Minigames", 1310, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.FishNeverEscape", "gameplay", "Difficulty", "Minigames", 1320, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.LoachesAlwaysAppear", "gameplay", "Difficulty", "Minigames", 1330, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.SkipKeepConfirmation", "gameplay", "Difficulty", "Minigames", 1340, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.AllHyruleLoaches", "gameplay", "Difficulty", "Minigames", 1350, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.MinimumFishWeightChild", "gameplay", "Difficulty", "Minigames", 1360, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.MinimumFishWeightAdult", "gameplay", "Difficulty", "Minigames", 1370, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.FishingBlankB", "gameplay", "Difficulty", "Minigames", 1380, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.DampeBothPrizes", "gameplay", "Difficulty", "Minigames", 1390, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.HorsebackArcheryBothPrizes", "gameplay", "Difficulty", "Minigames", 1400, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Songs.ZoraEggCount", "gameplay", "Difficulty", "Quest requirements", 10, nullptr, "quest_counts", 0 },
    { kMenuMm, "gEnhancements.Minigames.CuccoShackCuccoCount", "gameplay", "Difficulty", "Quest requirements", 12, nullptr, "quest_counts", 0 },
    { kMenuMm, "gEnhancements.DifficultyOptions.LowerBankRewardThresholds", "gameplay", "Difficulty", "Quest requirements", 10, nullptr, "quest_counts", 0 },
    { kMenuMm, "gEnhancements.DifficultyOptions.GibdoTradeSequence", "gameplay", "Difficulty", "Quest requirements", 10, nullptr, "quest_counts", 0 },
    { kMenuMm, "gEnhancements.DifficultyOptions.FrogChoirCount", "gameplay", "Difficulty", "Quest requirements", 10, nullptr, "quest_counts", 0 },
    { kMenuOot, "gEnhancements.AllDogsRichard", "gameplay", "Difficulty", "Quest requirements", 10, nullptr, "quest_counts", 0 },
    { kMenuOot, "gEnhancements.CuccoStayDurationMult", "gameplay", "Difficulty", "Quest requirements", 10, nullptr, "quest_counts", 0 },
    { kMenuOot, "gEnhancements.CuccosToReturn", "gameplay", "Difficulty", "Quest requirements", 10, nullptr, "quest_counts", 0 },
    { kMenuMm, "gEnhancements.DifficultyOptions.DekuGuardSearchBalls", "gameplay", "Difficulty", "World", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.SwitchTimerMultiplier", "gameplay", "Difficulty", "World", 1000, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.DifficultyOptions.HiddenGrottosVisibility", "gameplay", "Difficulty", "World", 1010, nullptr, nullptr, 0 },
    { kMenuMm, "gModes.TimeMovesWhenYouMove", "gameplay", "Extra modes", "Challenges", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.BounceOffWalls", "gameplay", "Extra modes", "Challenges", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.RupeeDash", "gameplay", "Extra modes", "Challenges", 1010, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.RupeeDashScaling", "gameplay", "Extra modes", "Challenges", 1020, "gEnhancements.RupeeDash", nullptr, 0 },
    { kMenuOot, "gEnhancements.RupeeDashInterval", "gameplay", "Extra modes", "Challenges", 1030, "gEnhancements.RupeeDash", nullptr, 0 },
    { kMenuOot, "gEnhancements.ShadowTag", "gameplay", "Extra modes", "Challenges", 1040, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.HurtContainer", "gameplay", "Extra modes", "Challenges", 1050, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.RandomizedEnemies", "gameplay", "Extra modes", "Enemy randomizer", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.RandomizedEnemySizes", "gameplay", "Extra modes", "Enemy randomizer", 1010, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.EnemySizeScalesHealth", "gameplay", "Extra modes", "Enemy randomizer", 1020, "gEnhancements.RandomizedEnemySizes", nullptr, 0 },
    { kMenuOot, "gEnhancements.RandomizedEnemyList.All", "gameplay", "Extra modes", "Enemy randomizer", 1030, "gEnhancements.RandomizedEnemies", nullptr, 0 },
    { kMenuOot, "gEnhancements.RandomizedEnemyList.*", "gameplay", "Extra modes", "Enemy randomizer", 1050, "gEnhancements.RandomizedEnemies", nullptr, 0 },
    { kMenuMm, "gModes.PlayAsKafei", "gameplay", "Extra modes", "Player & companions", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.IvanCoopModeEnabled", "gameplay", "Extra modes", "Player & companions", 1000, nullptr, nullptr, 0 },
    { kMenuMm, "gModes.HyruleWarriorsStyledLink", "gameplay", "Extra modes", "Player & companions", 1010, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.DogFollowsEverywhere", "gameplay", "Extra modes", "Player & companions", 1010, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.ExtraTraps.Enabled", "gameplay", "Extra modes", "Traps", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.ExtraTraps.Ice", "gameplay", "Extra modes", "Traps", 1020, "gEnhancements.ExtraTraps.Enabled", nullptr, 0 },
    { kMenuOot, "gEnhancements.ExtraTraps.Burn", "gameplay", "Extra modes", "Traps", 1030, "gEnhancements.ExtraTraps.Enabled", nullptr, 0 },
    { kMenuOot, "gEnhancements.ExtraTraps.Shock", "gameplay", "Extra modes", "Traps", 1040, "gEnhancements.ExtraTraps.Enabled", nullptr, 0 },
    { kMenuOot, "gEnhancements.ExtraTraps.Knockback", "gameplay", "Extra modes", "Traps", 1060, "gEnhancements.ExtraTraps.Enabled", nullptr, 0 },
    { kMenuOot, "gEnhancements.ExtraTraps.Speed", "gameplay", "Extra modes", "Traps", 1070, "gEnhancements.ExtraTraps.Enabled", nullptr, 0 },
    { kMenuOot, "gEnhancements.ExtraTraps.Bomb", "gameplay", "Extra modes", "Traps", 1080, "gEnhancements.ExtraTraps.Enabled", nullptr, 0 },
    { kMenuOot, "gEnhancements.ExtraTraps.Void", "gameplay", "Extra modes", "Traps", 1100, "gEnhancements.ExtraTraps.Enabled", nullptr, 0 },
    { kMenuOot, "gEnhancements.ExtraTraps.Ammo", "gameplay", "Extra modes", "Traps", 1110, "gEnhancements.ExtraTraps.Enabled", nullptr, 0 },
    { kMenuOot, "gEnhancements.ExtraTraps.Kill", "gameplay", "Extra modes", "Traps", 1120, "gEnhancements.ExtraTraps.Enabled", nullptr, 0 },
    { kMenuOot, "gEnhancements.ExtraTraps.Teleport", "gameplay", "Extra modes", "Traps", 1130, "gEnhancements.ExtraTraps.Enabled", nullptr, 0 },
    { kMenuMm, "gModes.MirroredWorld.Mode", "gameplay", "Extra modes", "World", 10, nullptr, "mirrored_world", 1 },
    { kMenuMm, "gModes.MirroredWorld.StoneTowerTempleFix", "gameplay", "Extra modes", "World", 10, nullptr, "mirrored_world", 1 },
    { kMenuOot, "gEnhancements.MirroredWorldMode", "gameplay", "Extra modes", "World", 10, nullptr, "mirrored_world", 1 },
    { kMenuMm, "gEnhancements.Fixes.CompletedHeartContainerAudio", "gameplay", "Fixes", "Audio", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.SilverRupeeJingleExtend", "gameplay", "Fixes", "Audio", 1000, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Camera.FixTargettingCameraSnap", "gameplay", "Fixes", "Camera", 10, nullptr, "camera_fixes", 0 },
    { kMenuOot, "gEnhancements.FixCameraDrift", "gameplay", "Fixes", "Camera", 10, nullptr, "camera_fixes", 0 },
    { kMenuOot, "gEnhancements.FixCameraSwing", "gameplay", "Fixes", "Camera", 10, nullptr, "camera_fixes", 0 },
    { kMenuOot, "gEnhancements.FixHangingLedgeSwingRate", "gameplay", "Fixes", "Camera", 10, nullptr, "camera_fixes", 0 },
    { kMenuMm, "gEnhancements.Fixes.ConsoleCrashes", "gameplay", "Fixes", "Gameplay", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.GravediggingTourFix", "gameplay", "Fixes", "Gameplay", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.FixDampeGoingBackwards", "gameplay", "Fixes", "Gameplay", 1010, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.FixKokiriForestQuestState", "gameplay", "Fixes", "Gameplay", 1020, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.FixFloorSwitches", "gameplay", "Fixes", "Gameplay", 1030, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.FixZoraHintDialogue", "gameplay", "Fixes", "Gameplay", 1040, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.FixVineFall", "gameplay", "Fixes", "Gameplay", 1050, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.BushDropFix", "gameplay", "Fixes", "Gameplay", 1060, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.FixFlexDrops", "gameplay", "Fixes", "Gameplay", 1070, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.EnemySpawnsOverWaterboxes", "gameplay", "Fixes", "Gameplay", 1080, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.FixSawSoftlock", "gameplay", "Fixes", "Gameplay", 1090, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.AnubisFix", "gameplay", "Fixes", "Gameplay", 1100, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.GCDoorsAfterFireFix", "gameplay", "Fixes", "Gameplay", 1110, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.MQWaterLockFix", "gameplay", "Fixes", "Gameplay", 1120, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Fixes.FixTexturesOOB", "gameplay", "Fixes", "Graphics", 10, nullptr, "textures_oob", 1 },
    { kMenuOot, "gEnhancements.FixTexturesOOB", "gameplay", "Fixes", "Graphics", 10, nullptr, "textures_oob", 1 },
    { kMenuMm, "gFixes.FixButtonEnvColor", "gameplay", "Fixes", "Graphics", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.FixMenuLR", "gameplay", "Fixes", "Graphics", 1000, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Fixes.ControlCharacters", "gameplay", "Fixes", "Graphics", 1010, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.FixDungeonMinimapIcon", "gameplay", "Fixes", "Graphics", 1010, nullptr, nullptr, 0 },
    { kMenuMm, "gFixes.FixIkanaGreatFairyFountainColor", "gameplay", "Fixes", "Graphics", 1020, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.TwoHandedIdle", "gameplay", "Fixes", "Graphics", 1020, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Fixes.DekuButlerFixShockLoopAnimation", "gameplay", "Fixes", "Graphics", 1030, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.NaviTextFix", "gameplay", "Fixes", "Graphics", 1030, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.GerudoWarriorClothingFix", "gameplay", "Fixes", "Graphics", 1040, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.FixEyesOpenWhileSleeping", "gameplay", "Fixes", "Graphics", 1050, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.FixHammerHand", "gameplay", "Fixes", "Graphics", 1060, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.SceneSpecificDirtPathFix", "gameplay", "Fixes", "Graphics", 1070, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.CrouchStabFix", "gameplay", "Fixes", "Items & combat", 10, "gEnhancements.CrouchStabHammerFix", "crouch_stab", 0 },
    { kMenuMm, "gFixes.FixEponaStealingSword", "gameplay", "Fixes", "Items & combat", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.DekuNutUpgradeFix", "gameplay", "Fixes", "Items & combat", 1000, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Fixes.FierceDeityZTargetMovement", "gameplay", "Fixes", "Items & combat", 1010, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.CrouchStabHammerFix", "gameplay", "Fixes", "Items & combat", 1010, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.FixBrokenGiantsKnife", "gameplay", "Fixes", "Items & combat", 1020, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.FixDaruniaDanceSpeed", "gameplay", "Fixes", "Timing", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.CreditsFix", "gameplay", "Fixes", "Timing", 1010, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Equipment.MagicArrowEquipSpeed", "gameplay", "Items & equipment", "Bow & arrows", 10, nullptr, "magic_arrow", 1 },
    { kMenuOot, "gEnhancements.SkipArrowAnimation", "gameplay", "Items & equipment", "Bow & arrows", 10, nullptr, "magic_arrow", 1 },
    { kMenuMm, "gEnhancements.PlayerActions.ArrowCycle", "gameplay", "Items & equipment", "Bow & arrows", 20, nullptr, "arrow_cycle", 1 },
    { kMenuOot, "gEnhancements.BowArrowCycle", "gameplay", "Items & equipment", "Bow & arrows", 20, nullptr, "arrow_cycle", 1 },
    { kMenuMm, "gEnhancements.Graphics.BowReticle", "gameplay", "Items & equipment", "Bow & arrows", 30, nullptr, "bow_reticle", 1 },
    { kMenuOot, "gEnhancements.BowReticle", "gameplay", "Items & equipment", "Bow & arrows", 30, nullptr, "bow_reticle", 1 },
    { kMenuMm, "gEnhancements.Equipment.BombArrows", "gameplay", "Items & equipment", "Bow & arrows", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.SeparateArrows", "gameplay", "Items & equipment", "Bow & arrows", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.BlueFireArrows", "gameplay", "Items & equipment", "Bow & arrows", 1010, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.SunlightArrows", "gameplay", "Items & equipment", "Bow & arrows", 1020, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.BowSlingshotAmmoFix", "gameplay", "Items & equipment", "Bow & arrows", 1030, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Player.InstantPutaway", "gameplay", "Items & equipment", "Equipping", 10, nullptr, "instant_putaway", 1 },
    { kMenuOot, "gEnhancements.InstantPutaway", "gameplay", "Items & equipment", "Equipping", 10, nullptr, "instant_putaway", 1 },
    { kMenuMm, "gEnhancements.Player.UnsheatheWithoutSlashing", "gameplay", "Items & equipment", "Equipping", 20, nullptr, "unsheathe", 1 },
    { kMenuOot, "gEnhancements.UnsheatheWithoutSlashing", "gameplay", "Items & equipment", "Equipping", 20, nullptr, "unsheathe", 1 },
    { kMenuMm, "gEnhancements.Equipment.ItemUnequip", "gameplay", "Items & equipment", "Equipping", 30, nullptr, "unequip", 1 },
    { kMenuOot, "gEnhancements.ItemUnequip", "gameplay", "Items & equipment", "Equipping", 30, nullptr, "unequip", 1 },
    { kMenuMm, "gEnhancements.Player.FierceDeityPutaway", "gameplay", "Items & equipment", "Equipping", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.AssignableTunicsAndBoots", "gameplay", "Items & equipment", "Equipping", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.EquipmentCanBeRemoved", "gameplay", "Items & equipment", "Equipping", 1010, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.ToggleStrength", "gameplay", "Items & equipment", "Equipping", 1020, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.SwordToggle", "gameplay", "Items & equipment", "Equipping", 1030, "gEnhancements.EquipmentCanBeRemoved", nullptr, 0 },
    { kMenuOot, "gEnhancements.AskToEquip", "gameplay", "Items & equipment", "Equipping", 1040, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.PlayerActions.RemoteBombchu", "gameplay", "Items & equipment", "Explosives & projectiles", 10, nullptr, "remote_bombchu", 1 },
    { kMenuOot, "gEnhancements.RemoteBombchu", "gameplay", "Items & equipment", "Explosives & projectiles", 10, nullptr, "remote_bombchu", 1 },
    { kMenuMm, "gEnhancements.Items.RemoveExplosiveLimit", "gameplay", "Items & equipment", "Explosives & projectiles", 20, nullptr, "explosive_limit", 1 },
    { kMenuOot, "gEnhancements.RemoveExplosiveLimit", "gameplay", "Items & equipment", "Explosives & projectiles", 20, nullptr, "explosive_limit", 1 },
    { kMenuMm, "gEnhancements.Gameplay.ExtendedProjectileInteractionDistance", "gameplay", "Items & equipment", "Explosives & projectiles", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.NutsExplodeBombs", "gameplay", "Items & equipment", "Explosives & projectiles", 1000, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Items.ExtraPowderKegs", "gameplay", "Items & equipment", "Explosives & projectiles", 1010, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.StaticExplosionRadius", "gameplay", "Items & equipment", "Explosives & projectiles", 1010, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.DisableFirstPersonChus", "gameplay", "Items & equipment", "Explosives & projectiles", 1020, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.HookshotableReticle", "gameplay", "Items & equipment", "Hookshot, boomerang & tools", 10, nullptr, "hookshot_reticle", 1 },
    { kMenuOot, "gEnhancements.HookshotableReticle", "gameplay", "Items & equipment", "Hookshot, boomerang & tools", 10, nullptr, "hookshot_reticle", 1 },
    { kMenuMm, "gEnhancements.PlayerActions.InstantRecall", "gameplay", "Items & equipment", "Hookshot, boomerang & tools", 20, nullptr, "boomerang_recall", 1 },
    { kMenuOot, "gEnhancements.FastBoomerang", "gameplay", "Items & equipment", "Hookshot, boomerang & tools", 20, nullptr, "boomerang_recall", 1 },
    { kMenuMm, "gEnhancements.Equipment.BetterPictoMessage", "gameplay", "Items & equipment", "Hookshot, boomerang & tools", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.BoomerangFirstPerson", "gameplay", "Items & equipment", "Hookshot, boomerang & tools", 1000, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Items.PictoBoxOnCUp", "gameplay", "Items & equipment", "Hookshot, boomerang & tools", 1010, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.BoomerangReticle", "gameplay", "Items & equipment", "Hookshot, boomerang & tools", 1010, "gEnhancements.BoomerangFirstPerson", nullptr, 0 },
    { kMenuMm, "gEnhancements.Items.ColorPictograph", "gameplay", "Items & equipment", "Hookshot, boomerang & tools", 1020, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.RebottleBlueFire", "gameplay", "Items & equipment", "Magic & bottles", 1000, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Masks.PersistentBunnyHood.Enabled", "gameplay", "Items & equipment", "Masks & forms", 10, nullptr, "bunny_hood", 0 },
    { kMenuOot, "gEnhancements.MMBunnyHood", "gameplay", "Items & equipment", "Masks & forms", 10, nullptr, "bunny_hood", 0 },
    { kMenuMm, "gEnhancements.Player.InfiniteDekuHopping", "gameplay", "Items & equipment", "Masks & forms", 20, nullptr, "deku_hop", 0 },
    { kMenuMm, "gEnhancements.Masks.EquipWhileSwimming", "gameplay", "Items & equipment", "Masks & forms", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.AdultMasks", "gameplay", "Items & equipment", "Masks & forms", 1000, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Masks.BlastMaskKeg", "gameplay", "Items & equipment", "Masks & forms", 1010, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.PersistentMasks", "gameplay", "Items & equipment", "Masks & forms", 1010, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Masks.FastTransformation", "gameplay", "Items & equipment", "Masks & forms", 1020, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.HideBunnyHood", "gameplay", "Items & equipment", "Masks & forms", 1020, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Masks.3DSMaskEquip", "gameplay", "Items & equipment", "Masks & forms", 1030, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.MaskSelect", "gameplay", "Items & equipment", "Masks & forms", 1030, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Masks.EasyMaskEquip", "gameplay", "Items & equipment", "Masks & forms", 1040, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Masks.FierceDeitysAnywhere", "gameplay", "Items & equipment", "Masks & forms", 1050, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Masks.BlastMaskCooldown", "gameplay", "Items & equipment", "Masks & forms", 1060, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Masks.GoronRollingIgnoresMagic", "gameplay", "Items & equipment", "Masks & forms", 1070, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Masks.GoronRollingFastSpikes", "gameplay", "Items & equipment", "Masks & forms", 1080, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Masks.GoronRollingSpikesRequireShield", "gameplay", "Items & equipment", "Masks & forms", 1090, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Playback.NoDropOcarinaInput", "gameplay", "Items & equipment", "Ocarina & songs", 10, nullptr, "dropped_ocarina", 1 },
    { kMenuOot, "gEnhancements.DpadNoDropOcarinaInput", "gameplay", "Items & equipment", "Ocarina & songs", 10, nullptr, "dropped_ocarina", 1 },
    { kMenuOot, "gEnhancements.TimeTravel", "gameplay", "Items & equipment", "Ocarina & songs", 20, nullptr, "time_travel", 0 },
    { kMenuMm, "gEnhancements.Player.UnderwaterOcarina", "gameplay", "Items & equipment", "Ocarina & songs", 1000, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Songs.BetterSongOfDoubleTime", "gameplay", "Items & equipment", "Ocarina & songs", 1010, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Songs.EnableSunsSong", "gameplay", "Items & equipment", "Ocarina & songs", 1020, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Songs.SongItems", "gameplay", "Items & equipment", "Ocarina & songs", 1030, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Equipment.TwoHandedSwordSpinAttack", "gameplay", "Items & equipment", "Weapons & gear", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gCheats.SuperTunic", "gameplay", "Items & equipment", "Weapons & gear", 1000, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Equipment.GreatFairySwordBButton", "gameplay", "Items & equipment", "Weapons & gear", 1010, nullptr, nullptr, 0 },
    { kMenuOot, "gCheats.DekuStick", "gameplay", "Items & equipment", "Weapons & gear", 1010, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.PauseAnyCursor", "gameplay", "Quality of life", "Pause menu & messages", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.InjectItemCounts.GoldSkulltula", "gameplay", "Quality of life", "Pause menu & messages", 1010, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.InjectItemCounts.HeartPiece", "gameplay", "Quality of life", "Pause menu & messages", 1020, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.InjectItemCounts.HeartContainer", "gameplay", "Quality of life", "Pause menu & messages", 1030, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Minigames.CremiaHugs", "gameplay", "Quality of life", "Shops & rewards", 10, nullptr, "shops_rewards", 0 },
    { kMenuMm, "gEnhancements.Items.AmmoBuyback", "gameplay", "Quality of life", "Shops & rewards", 10, nullptr, "shops_rewards", 0 },
    { kMenuMm, "gEnhancements.Shops.CuriosityShopRefills", "gameplay", "Quality of life", "Shops & rewards", 10, nullptr, "shops_rewards", 0 },
    { kMenuOot, "gEnhancements.BetterBombchuShopping", "gameplay", "Quality of life", "Shops & rewards", 10, nullptr, "shops_rewards", 0 },
    { kMenuMm, "gEnhancements.Cycle.StopOceansideSpiderHouseSquatter", "gameplay", "Quality of life", "Time of day & cycle", 10, nullptr, "time_of_day", 0 },
    { kMenuMm, "gEnhancements.Cycle.OceansideWalletAnyDay", "gameplay", "Quality of life", "Time of day & cycle", 10, nullptr, "time_of_day", 0 },
    { kMenuMm, "gEnhancements.Cycle.TingleAlwaysInClockTown", "gameplay", "Quality of life", "Time of day & cycle", 10, nullptr, "time_of_day", 0 },
    { kMenuOot, "gEnhancements.NightGSAlwaysSpawn", "gameplay", "Quality of life", "Time of day & cycle", 10, nullptr, "time_of_day", 0 },
    { kMenuOot, "gEnhancements.DayGravePull", "gameplay", "Quality of life", "Time of day & cycle", 10, nullptr, "time_of_day", 0 },
    { kMenuOot, "gEnhancements.DampeAllNight", "gameplay", "Quality of life", "Time of day & cycle", 10, nullptr, "time_of_day", 0 },
    { kMenuOot, "gEnhancements.MarketSneak", "gameplay", "Quality of life", "Time of day & cycle", 10, nullptr, "time_of_day", 0 },
    { kMenuOot, "gEnhancements.OpenAllHours", "gameplay", "Quality of life", "Time of day & cycle", 10, nullptr, "time_of_day", 0 },
    { kMenuMm, "gEnhancements.Cycle.DoNotResetBottleContent", "gameplay", "Quality of life", "Time of day & cycle", 1000, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Cycle.DoNotResetConsumables", "gameplay", "Quality of life", "Time of day & cycle", 1010, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Cycle.DoNotResetRazorSword", "gameplay", "Quality of life", "Time of day & cycle", 1020, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Cycle.DoNotResetRupees", "gameplay", "Quality of life", "Time of day & cycle", 1030, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Cycle.DoNotResetTimeSpeed", "gameplay", "Quality of life", "Time of day & cycle", 1040, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Cycle.DoNotResetChateau", "gameplay", "Quality of life", "Time of day & cycle", 1050, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Cycle.DoNotResetScarecrowSong", "gameplay", "Quality of life", "Time of day & cycle", 1060, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Cycle.KeepExpressMail", "gameplay", "Quality of life", "Time of day & cycle", 1070, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.EasyButterflyFairies", "gameplay", "Quality of life", "World & NPCs", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.QuitFishingAtDoor", "gameplay", "Quality of life", "World & NPCs", 1010, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.ResetNaviTimer", "gameplay", "Quality of life", "World & NPCs", 1020, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.CowOfTime", "gameplay", "Quality of life", "World & NPCs", 1030, nullptr, nullptr, 0 },
    { kMenuOot, "gRandoEnhancements.RandoRelevantNavi", "gameplay", "Randomizer extras", "Hints & messages", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gRandoEnhancements.RandomizeRupeeNames", "gameplay", "Randomizer extras", "Hints & messages", 1010, nullptr, nullptr, 0 },
    { kMenuOot, "gRandoEnhancements.QuestItemFanfares", "gameplay", "Randomizer extras", "Hints & messages", 1020, nullptr, nullptr, 0 },
    { kMenuOot, "gRandoEnhancements.EntrancesOnSigns", "gameplay", "Randomizer extras", "Hints & messages", 1030, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.ChestSizeAndTextureMatchContents", "gameplay", "Randomizer extras", "Item appearance", 10, nullptr, "csmc", 1 },
    { kMenuOot, "gEnhancements.ChestSizeDependsStoneOfAgony", "gameplay", "Randomizer extras", "Item appearance", 10, "gEnhancements.ChestSizeAndTextureMatchContents", "csmc", 1 },
    { kMenuOot, "gRandoEnhancements.CustomKeyModels", "gameplay", "Randomizer extras", "Item appearance", 20, nullptr, "rando_extras", 0 },
    { kMenuOot, "gRandoEnhancements.ColoredMapsAndCompasses", "gameplay", "Randomizer extras", "Item appearance", 20, nullptr, "rando_extras", 0 },
    { kMenuOot, "gRandoEnhancements.MysteriousShuffle", "gameplay", "Randomizer extras", "Item appearance", 20, nullptr, "rando_extras", 0 },
    { kMenuOot, "gRandoEnhancements.GenericJabberNutModel", "gameplay", "Randomizer extras", "Item appearance", 1020, nullptr, nullptr, 0 },
    { kMenuOot, "gRandoEnhancements.SimplerBossSoulModels", "gameplay", "Randomizer extras", "Item appearance", 1030, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Restorations.PowerCrouchStab", "gameplay", "Restorations", "Glitches", 10, nullptr, "crouch_stab", 0 },
    { kMenuMm, "gEnhancements.Restorations.TatlISG", "gameplay", "Restorations", "Glitches", 20, nullptr, "isg", 0 },
    { kMenuOot, "gEnhancements.RestoreShadowDoorUnload", "gameplay", "Restorations", "Glitches", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.HoverFishing", "gameplay", "Restorations", "Glitches", 1010, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.N64WeirdFrames", "gameplay", "Restorations", "Glitches", 1020, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.BombchusOOB", "gameplay", "Restorations", "Glitches", 1030, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.QuickPutaway", "gameplay", "Restorations", "Glitches", 1040, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.QuickBongoKill", "gameplay", "Restorations", "Glitches", 1050, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.EarlyEyeballFrog", "gameplay", "Restorations", "Glitches", 1060, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.GetItemManipulation", "gameplay", "Restorations", "Glitches", 1070, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Restorations.ConstantFlipsHops", "gameplay", "Restorations", "Movement & combat", 1000, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Restorations.SideRoll", "gameplay", "Restorations", "Movement & combat", 1010, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Restorations.OoTFasterSwim", "gameplay", "Restorations", "Movement & combat", 1020, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Restorations.BonkCollision", "gameplay", "Restorations", "Movement & combat", 1030, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Restorations.WoodfallMountainAppearance", "gameplay", "Restorations", "Visual & audio", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.RedGanonBlood", "gameplay", "Restorations", "Visual & audio", 1000, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Restorations.DayTelopDuration", "gameplay", "Restorations", "Visual & audio", 1010, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.GSCutscene", "gameplay", "Restorations", "Visual & audio", 1010, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.PulsateBossIcon", "gameplay", "Restorations", "Visual & audio", 1020, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.SariaGestureFriendsForever", "gameplay", "Restorations", "Visual & audio", 1030, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Restorations.JPGrottos", "gameplay", "Restorations", "World", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.NGCKaleidoSwitcher", "gameplay", "Restorations", "World", 1000, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Restorations.SoilPatch", "gameplay", "Restorations", "World", 1010, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.WideShutterDoorRange", "gameplay", "Restorations", "World", 1010, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.GraveHoles", "gameplay", "Restorations", "World", 1020, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Saving.Autosave", "gameplay", "Saving", "Autosave", 10, nullptr, "autosave", 1 },
    { kMenuMm, "gEnhancements.Saving.AutosaveInterval", "gameplay", "Saving", "Autosave", 10, nullptr, "autosave", 1 },
    { kMenuOot, "gEnhancements.Autosave", "gameplay", "Saving", "Autosave", 10, nullptr, "autosave", 1 },
    { kMenuOot, "gEnhancements.AutosaveNotification", "gameplay", "Saving", "Autosave", 10, "gEnhancements.Autosave", "autosave", 1 },
    { kMenuMm, "gEnhancements.Saving.FileSlot3", "gameplay", "Saving", "File select", 10, nullptr, "file_select", 0 },
    { kMenuMm, "gEnhancements.Saving.NewFileSetup", "gameplay", "Saving", "File select", 10, nullptr, "file_select", 0 },
    { kMenuOot, "gEnhancements.LinkDefaultName", "gameplay", "Saving", "File select", 10, nullptr, "file_select", 0 },
    { kMenuMm, "gEnhancements.Saving.RememberSaveLocation", "gameplay", "Saving", "Save & reload", 10, nullptr, "remember_save_location", 1 },
    { kMenuOot, "gEnhancements.RememberSaveLocation", "gameplay", "Saving", "Save & reload", 10, nullptr, "remember_save_location", 1 },
    { kMenuMm, "gEnhancements.Saving.PersistentOwlSaves", "gameplay", "Saving", "Save & reload", 20, nullptr, "save_menu", 0 },
    { kMenuMm, "gEnhancements.Saving.PauseSave", "gameplay", "Saving", "Save & reload", 20, nullptr, "save_menu", 0 },
    { kMenuOot, "gEnhancements.BetterSaveMenu", "gameplay", "Saving", "Save & reload", 20, nullptr, "save_menu", 0 },
    { kMenuOot, "gEnhancements.SkipSaveConfirmation", "gameplay", "Saving", "Save & reload", 20, nullptr, "save_menu", 0 },
    { kMenuMm, "gEnhancements.Cycle.SaveOnMoonCrash", "gameplay", "Saving", "Save & reload", 30, nullptr, "moon_crash_save", 0 },
    { kMenuMm, "gEnhancements.Cutscenes.HideTitleCards", "gameplay", "Time savers", "Cutscenes", 10, nullptr, "title_cards", 1 },
    { kMenuOot, "gEnhancements.TimeSavers.DisableTitleCard", "gameplay", "Time savers", "Cutscenes", 10, nullptr, "title_cards", 1 },
    { kMenuMm, "gEnhancements.Cutscenes.SkipIntroSequence", "gameplay", "Time savers", "Cutscenes", 20, nullptr, "skip_intro", 1 },
    { kMenuMm, "gEnhancements.Cutscenes.SkipFirstCycle", "gameplay", "Time savers", "Cutscenes", 20, nullptr, "skip_intro", 1 },
    { kMenuOot, "gEnhancements.TimeSavers.SkipCutscene.Intro", "gameplay", "Time savers", "Cutscenes", 20, nullptr, "skip_intro", 1 },
    { kMenuMm, "gEnhancements.Cutscenes.SkipEntranceCutscenes", "gameplay", "Time savers", "Cutscenes", 30, nullptr, "skip_entrance", 1 },
    { kMenuOot, "gEnhancements.TimeSavers.SkipCutscene.Entrances", "gameplay", "Time savers", "Cutscenes", 30, nullptr, "skip_entrance", 1 },
    { kMenuMm, "gEnhancements.Cutscenes.SkipStoryCutscenes", "gameplay", "Time savers", "Cutscenes", 40, nullptr, "skip_story", 1 },
    { kMenuOot, "gEnhancements.TimeSavers.SkipCutscene.Story", "gameplay", "Time savers", "Cutscenes", 40, nullptr, "skip_story", 1 },
    { kMenuMm, "gEnhancements.Cutscenes.SkipMiscInteractions", "gameplay", "Time savers", "Cutscenes", 50, nullptr, "skip_misc", 1 },
    { kMenuOot, "gEnhancements.TimeSavers.SkipMiscInteractions", "gameplay", "Time savers", "Cutscenes", 50, nullptr, "skip_misc", 1 },
    { kMenuMm, "gEnhancements.Cutscenes.SkipOnePointCutscenes", "gameplay", "Time savers", "Cutscenes", 60, nullptr, "skip_one_point", 1 },
    { kMenuOot, "gEnhancements.TimeSavers.SkipCutscene.OnePoint", "gameplay", "Time savers", "Cutscenes", 60, nullptr, "skip_one_point", 1 },
    { kMenuMm, "gEnhancements.Cutscenes.SkipEnemyCutscenes", "gameplay", "Time savers", "Cutscenes", 70, nullptr, "skip_boss_enemy", 0 },
    { kMenuOot, "gEnhancements.TimeSavers.SkipCutscene.BossIntro", "gameplay", "Time savers", "Cutscenes", 70, nullptr, "skip_boss_enemy", 0 },
    { kMenuOot, "gEnhancements.TimeSavers.SkipCutscene.QuickBossDeaths", "gameplay", "Time savers", "Cutscenes", 70, nullptr, "skip_boss_enemy", 0 },
    { kMenuMm, "gEnhancements.Songs.SkipSoTCutscenes", "gameplay", "Time savers", "Cutscenes", 80, nullptr, "skip_song_cs", 0 },
    { kMenuMm, "gEnhancements.Songs.SkipSoaringCutscene", "gameplay", "Time savers", "Cutscenes", 80, nullptr, "skip_song_cs", 0 },
    { kMenuOot, "gEnhancements.TimeSavers.SkipCutscene.LearnSong", "gameplay", "Time savers", "Cutscenes", 80, nullptr, "skip_song_cs", 0 },
    { kMenuMm, "gEnhancements.Cutscenes.SkipGetItemCutscenes", "gameplay", "Time savers", "Cutscenes", 90, nullptr, "skip_get_item", 1 },
    { kMenuOot, "gRandoEnhancements.TimeSavers.SkipGetItemAnimation", "gameplay", "Time savers", "Cutscenes", 90, nullptr, "skip_get_item", 1 },
    { kMenuOot, "gRandoEnhancements.TimeSavers.SkipGetItemAnimationScale", "gameplay", "Time savers", "Cutscenes", 90, "gRandoEnhancements.TimeSavers.SkipGetItemAnimation", "skip_get_item", 1 },
    { kMenuOot, "gEnhancements.TimeSavers.SkipOwlInteractions", "gameplay", "Time savers", "Cutscenes", 100, nullptr, "skip_owl", 0 },
    { kMenuOot, "gEnhancements.TimeSavers.SkipCutscene.GlitchAiding", "gameplay", "Time savers", "Cutscenes", 1020, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.SkulltulaFreeze", "gameplay", "Time savers", "Cutscenes", 1030, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Playback.SkipScarecrowSong", "gameplay", "Time savers", "Quests & errands", 10, nullptr, "scarecrow", 1 },
    { kMenuOot, "gEnhancements.InstantScarecrow", "gameplay", "Time savers", "Quests & errands", 10, nullptr, "scarecrow", 1 },
    { kMenuMm, "gEnhancements.Timesavers.SwampBoatSpeed", "gameplay", "Time savers", "Quests & errands", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.SkipAmyPuzzle", "gameplay", "Time savers", "Quests & errands", 1000, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Timesavers.GalleryTwofer", "gameplay", "Time savers", "Quests & errands", 1010, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.TimeSavers.SkipChildStealth", "gameplay", "Time savers", "Quests & errands", 1010, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Timesavers.MarineLabHP", "gameplay", "Time savers", "Quests & errands", 1020, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.TimeSavers.SkipTowerEscape", "gameplay", "Time savers", "Quests & errands", 1020, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Timesavers.DampeDiggingSkip", "gameplay", "Time savers", "Quests & errands", 1030, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.TimeSavers.SleepingWaterfall", "gameplay", "Time savers", "Quests & errands", 1030, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Timesavers.AlwaysShowShrineFeathers", "gameplay", "Time savers", "Quests & errands", 1040, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.ForgeTime", "gameplay", "Time savers", "Quests & errands", 1040, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Timesavers.PowderKegCertification", "gameplay", "Time savers", "Quests & errands", 1050, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.TimeSavers.SkipJabuJabuFish", "gameplay", "Time savers", "Quests & errands", 1050, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Timesavers.SkipBalladOfWindfish", "gameplay", "Time savers", "Quests & errands", 1060, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Timesavers.AutoBankDeposit", "gameplay", "Time savers", "Quests & errands", 1070, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Player.ClimbSpeed", "gameplay", "Time savers", "Speed-ups", 10, nullptr, "climb_speed", 1 },
    { kMenuOot, "gEnhancements.ClimbSpeed", "gameplay", "Time savers", "Speed-ups", 10, nullptr, "climb_speed", 1 },
    { kMenuMm, "gEnhancements.Player.FasterPushAndPull", "gameplay", "Time savers", "Speed-ups", 20, nullptr, "push_speed", 1 },
    { kMenuOot, "gEnhancements.FasterBlockPush", "gameplay", "Time savers", "Speed-ups", 20, nullptr, "push_speed", 1 },
    { kMenuOot, "gEnhancements.CrawlSpeed", "gameplay", "Time savers", "Speed-ups", 30, nullptr, "crawl_speed", 0 },
    { kMenuMm, "gEnhancements.Timesavers.FastChests", "gameplay", "Time savers", "Speed-ups", 40, nullptr, "fast_chests", 1 },
    { kMenuOot, "gEnhancements.FastChests", "gameplay", "Time savers", "Speed-ups", 40, nullptr, "fast_chests", 1 },
    { kMenuMm, "gEnhancements.Timesavers.FasterBottles", "gameplay", "Time savers", "Speed-ups", 50, nullptr, "faster_bottles", 0 },
    { kMenuOot, "gEnhancements.FasterBottleEmpty", "gameplay", "Time savers", "Speed-ups", 50, nullptr, "faster_bottles", 0 },
    { kMenuMm, "gEnhancements.Timesavers.FasterRupeeAccumulator", "gameplay", "Time savers", "Speed-ups", 60, nullptr, "rupee_accumulator", 1 },
    { kMenuOot, "gEnhancements.FasterRupeeAccumulator", "gameplay", "Time savers", "Speed-ups", 60, nullptr, "rupee_accumulator", 1 },
    { kMenuMm, "gEnhancements.Songs.FasterSongPlayback", "gameplay", "Time savers", "Speed-ups", 70, nullptr, "song_playback", 1 },
    { kMenuOot, "gEnhancements.FastOcarinaPlayback", "gameplay", "Time savers", "Speed-ups", 70, nullptr, "song_playback", 1 },
    { kMenuMm, "gEnhancements.Player.FastFlowerLaunch", "gameplay", "Time savers", "Speed-ups", 80, nullptr, "deku_hop", 0 },
    { kMenuOot, "gEnhancements.FastFarores", "gameplay", "Time savers", "Speed-ups", 90, nullptr, "warp_farore", 0 },
    { kMenuMm, "gEnhancements.Timesavers.FasterSceneTransitions", "gameplay", "Time savers", "Speed-ups", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.FasterHeavyBlockLift", "gameplay", "Time savers", "Speed-ups", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.FasterShadowShip", "gameplay", "Time savers", "Speed-ups", 1010, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.ImprovedRoll", "gameplay", "Time savers", "Speed-ups", 1020, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.ImprovedRollSteering", "gameplay", "Time savers", "Speed-ups", 1030, "gEnhancements.ImprovedRoll", nullptr, 0 },
    { kMenuOot, "gEnhancements.SkipSwimDeepEndAnim", "gameplay", "Time savers", "Speed-ups", 1040, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.GlitchAidingCrawlspaces", "gameplay", "Time savers", "Speed-ups", 1050, "gEnhancements.CrawlSpeed", nullptr, 0 },
    { kMenuOot, "gEnhancements.MweepSpeed", "gameplay", "Time savers", "Speed-ups", 1060, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.FasterPauseMenu", "gameplay", "Time savers", "Speed-ups", 1070, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.FasterBeanSkull", "gameplay", "Time savers", "Speed-ups", 1080, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Dialogue.FastText", "gameplay", "Time savers", "Text & dialogue", 10, nullptr, "text_speed", 1 },
    { kMenuOot, "gEnhancements.SkipText", "gameplay", "Time savers", "Text & dialogue", 10, nullptr, "text_speed", 1 },
    { kMenuOot, "gEnhancements.TextSpeed", "gameplay", "Time savers", "Text & dialogue", 10, nullptr, "text_speed", 1 },
    { kMenuOot, "gEnhancements.SlowTextSpeed", "gameplay", "Time savers", "Text & dialogue", 20, nullptr, "slow_text", 0 },
    { kMenuMm, "gEnhancements.Dialogue.SkipBottlePickupMessages", "gameplay", "Time savers", "Text & dialogue", 30, nullptr, "bottle_pickup_msgs", 1 },
    { kMenuOot, "gEnhancements.FastBottles", "gameplay", "Time savers", "Text & dialogue", 30, nullptr, "bottle_pickup_msgs", 1 },
    { kMenuMm, "gEnhancements.Cutscenes.AutoAdvanceEndingText", "gameplay", "Time savers", "Text & dialogue", 40, nullptr, "credits_text", 1 },
    { kMenuOot, "gEnhancements.NoInputForCredits", "gameplay", "Time savers", "Text & dialogue", 40, nullptr, "credits_text", 1 },
    { kMenuOot, "gEnhancements.TimeSavers.SkipForcedDialog", "gameplay", "Time savers", "Text & dialogue", 50, nullptr, "forced_dialog", 0 },
    { kMenuOot, "gEnhancements.BetterOwl", "gameplay", "Time savers", "Text & dialogue", 60, nullptr, "skip_owl", 0 },
    { kMenuMm, "gEnhancements.Dialogue.FastBankSelection", "gameplay", "Time savers", "Text & dialogue", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.FastDrops", "gameplay", "Time savers", "Text & dialogue", 1000, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Dialogue.AutoBombersCode", "gameplay", "Time savers", "Text & dialogue", 1010, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Songs.PauseOwlWarp", "gameplay", "Warps & travel", "Warping", 10, nullptr, "pause_warp", 1 },
    { kMenuMm, "gEnhancements.Songs.BetterOwlWarpMenu", "gameplay", "Warps & travel", "Warping", 10, nullptr, "pause_warp", 1 },
    { kMenuOot, "gEnhancements.PauseWarp", "gameplay", "Warps & travel", "Warping", 10, nullptr, "pause_warp", 1 },
    { kMenuOot, "gEnhancements.BetterFarore", "gameplay", "Warps & travel", "Warping", 20, nullptr, "warp_farore", 0 },
    { kMenuMm, "gEnhancements.Dpad.DpadEquips", "settings", "Controls", "Buttons & aiming", 10, nullptr, "dpad_equips", 1 },
    { kMenuOot, "gEnhancements.DpadEquips", "settings", "Controls", "Buttons & aiming", 10, nullptr, "dpad_equips", 1 },
    { kMenuMm, "gEnhancements.Player.PreventDiveOverWater", "settings", "Controls", "Buttons & aiming", 20, nullptr, "navi_l", 0 },
    { kMenuMm, "gEnhancements.Player.ManualJump", "settings", "Controls", "Buttons & aiming", 20, nullptr, "navi_l", 0 },
    { kMenuMm, "gEnhancements.Equipment.InvertShieldY", "settings", "Controls", "Buttons & aiming", 20, nullptr, "navi_l", 0 },
    { kMenuOot, "gEnhancements.NaviOnL", "settings", "Controls", "Buttons & aiming", 20, nullptr, "navi_l", 0 },
    { kMenuOot, "gEnhancements.ReworkedTargeting.Enabled", "settings", "Controls", "Buttons & aiming", 20, nullptr, "navi_l", 0 },
    { kMenuOot, "gEnhancements.ReworkedTargeting.Btn", "settings", "Controls", "Buttons & aiming", 1000, "gEnhancements.ReworkedTargeting.Enabled", nullptr, 0 },
    { kMenuMm, "gEnhancements.Restorations.PauseBufferWindow", "settings", "Controls", "Input timing", 10, nullptr, "pause_buffer", 1 },
    { kMenuOot, "gEnhancements.IncludeHeldInputsBufferWindow", "settings", "Controls", "Input timing", 10, nullptr, "pause_buffer", 1 },
    { kMenuOot, "gEnhancements.PauseBufferWindow", "settings", "Controls", "Input timing", 10, nullptr, "pause_buffer", 1 },
    { kMenuMm, "gSettings.SimulatedInputLag", "settings", "Controls", "Input timing", 20, nullptr, "sim_input_lag", 1 },
    { kMenuOot, "gSettings.SimulatedInputLag", "settings", "Controls", "Input timing", 20, nullptr, "sim_input_lag", 1 },
    { kMenuMm, "gEnhancements.Playback.DpadOcarina", "settings", "Controls", "Ocarina input", 10, nullptr, "dpad_ocarina", 1 },
    { kMenuMm, "gEnhancements.Playback.RightStickOcarina", "settings", "Controls", "Ocarina input", 10, nullptr, "dpad_ocarina", 1 },
    { kMenuMm, "gEnhancements.A11y.NoScreenFlashForEnemyKill", "settings", "General", "Accessibility", 10, nullptr, "screen_flash", 1 },
    { kMenuOot, "gSettings.A11yNoScreenFlashForFinishingBlow", "settings", "General", "Accessibility", 10, nullptr, "screen_flash", 1 },
    { kMenuMm, "gEnhancements.A11y.NoFinalDayQuakes", "settings", "General", "Accessibility", 20, nullptr, "comfort_a11y", 0 },
    { kMenuOot, "gEnhancements.DisableCritWiggle", "settings", "General", "Accessibility", 20, nullptr, "comfort_a11y", 0 },
    { kMenuMm, "gEnhancements.Cutscenes.SkipToFileSelect", "settings", "General", "Boot", 10, nullptr, "skip_to_file_select", 0 },
    { kMenuOot, "gSettings.BootSequence", "settings", "General", "Boot", 10, nullptr, "skip_to_file_select", 0 },
    { kMenuOot, "gOpenWindows.GameplayStats", "trackers", "Gameplay Stats", "Window", 1000, nullptr, nullptr, 0 },
    { kMenuMm, "gWindows.Timesplits.Settings", "trackers", "Time Splits", "Window", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gOpenWindows.TimeSplits", "trackers", "Time Splits", "Window", 1000, nullptr, nullptr, 0 },
    { kMenuMm, "gWindows.CosmeticEditor", "media", "Appearance", "Editor", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gOpenWindows.CosmeticsEditor", "media", "Appearance", "Editor", 1000, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Camera.FirstPerson.DisableFirstPersonAutoCenterView", "media", "Camera", "First person & aiming", 1000, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Camera.FirstPerson.InvertX", "media", "Camera", "First person & aiming", 1010, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Camera.FirstPerson.InvertY", "media", "Camera", "First person & aiming", 1020, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Camera.FirstPerson.SensitivityX", "media", "Camera", "First person & aiming", 1030, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Camera.FirstPerson.SensitivityY", "media", "Camera", "First person & aiming", 1040, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Camera.FirstPerson.GyroEnabled", "media", "Camera", "First person & aiming", 1050, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Camera.FirstPerson.GyroInvertX", "media", "Camera", "First person & aiming", 1060, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Camera.FirstPerson.GyroInvertY", "media", "Camera", "First person & aiming", 1070, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Camera.FirstPerson.GyroSensitivityX", "media", "Camera", "First person & aiming", 1080, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Camera.FirstPerson.GyroSensitivityY", "media", "Camera", "First person & aiming", 1090, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Camera.FirstPerson.RightStickEnabled", "media", "Camera", "First person & aiming", 1100, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Camera.FirstPerson.MoveInFirstPerson", "media", "Camera", "First person & aiming", 1110, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Camera.FirstPerson.RightStickInvertX", "media", "Camera", "First person & aiming", 1120, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Camera.FirstPerson.RightStickInvertY", "media", "Camera", "First person & aiming", 1130, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Camera.FirstPerson.RightStickSensitivityX", "media", "Camera", "First person & aiming", 1140, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Camera.FirstPerson.RightStickSensitivityY", "media", "Camera", "First person & aiming", 1150, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Camera.FreeLook.Enable", "media", "Camera", "Third-person camera", 1000, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Camera.FreeLook.MaxCameraDistance", "media", "Camera", "Third-person camera", 1010, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Camera.FreeLook.TransitionSpeed", "media", "Camera", "Third-person camera", 1020, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Camera.FreeLook.MaxPitch", "media", "Camera", "Third-person camera", 1030, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Camera.FreeLook.MinPitch", "media", "Camera", "Third-person camera", 1040, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Camera.DebugCam.Enable", "media", "Camera", "Third-person camera", 1050, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Camera.RightStick.InvertXAxis", "media", "Camera", "Third-person camera", 1060, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Camera.RightStick.InvertYAxis", "media", "Camera", "Third-person camera", 1070, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Camera.RightStick.CameraSensitivity.X", "media", "Camera", "Third-person camera", 1080, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Camera.RightStick.CameraSensitivity.Y", "media", "Camera", "Third-person camera", 1090, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Camera.DebugCam.6DOF", "media", "Camera", "Third-person camera", 1100, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Camera.DebugCam.CameraSpeed", "media", "Camera", "Third-person camera", 1110, nullptr, nullptr, 0 },
    { kMenuMm, "gWindows.AudioEditor", "media", "Music & sounds", "Editor", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gOpenWindows.AudioEditor", "media", "Music & sounds", "Editor", 1000, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Graphics.ClockType", "media", "Visual enhancements", "Clock", 1000, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Graphics.24HoursClock", "media", "Visual enhancements", "Clock", 1010, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Graphics.ActorCullingAccountsForWidescreen", "media", "Visual enhancements", "Draw distance", 10, nullptr, "draw_distance", 1 },
    { kMenuMm, "gEnhancements.Graphics.IncreaseActorDrawDistance", "media", "Visual enhancements", "Draw distance", 10, nullptr, "draw_distance", 1 },
    { kMenuOot, "gEnhancements.DisableDrawDistance", "media", "Visual enhancements", "Draw distance", 10, nullptr, "draw_distance", 1 },
    { kMenuOot, "gEnhancements.WidescreenActorCulling", "media", "Visual enhancements", "Draw distance", 10, nullptr, "draw_distance", 1 },
    { kMenuMm, "gEnhancements.Graphics.FixSceneGeometrySeams", "media", "Visual enhancements", "Draw distance", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.DisableKokiriDrawDistance", "media", "Visual enhancements", "Draw distance", 1000, "gEnhancements.DisableDrawDistance", nullptr, 0 },
    { kMenuMm, "gEnhancements.Graphics.DisableSceneGeometryDistanceCheck", "media", "Visual enhancements", "Draw distance", 1010, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.ExtendedCullingExcludeGlitchActors", "media", "Visual enhancements", "Draw distance", 1010, "gEnhancements.DisableDrawDistance", nullptr, 0 },
    { kMenuMm, "gEnhancements.Graphics.EnemyHealthBars", "media", "Visual enhancements", "HUD & interface", 10, nullptr, "enemy_health_bars", 1 },
    { kMenuOot, "gEnhancements.EnemyHealthBar", "media", "Visual enhancements", "HUD & interface", 10, nullptr, "enemy_health_bars", 1 },
    { kMenuOot, "gEnhancements.MinimalUI", "media", "Visual enhancements", "HUD & interface", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.DisableTunicWarningText", "media", "Visual enhancements", "HUD & interface", 1010, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.RememberMapToggleState", "media", "Visual enhancements", "HUD & interface", 1020, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.VisualAgony", "media", "Visual enhancements", "HUD & interface", 1030, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.NoHUDHeartAnimation", "media", "Visual enhancements", "HUD & interface", 1040, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.DrawLineupTick", "media", "Visual enhancements", "HUD & interface", 1050, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.DynamicWalletIcon", "media", "Visual enhancements", "HUD & interface", 1060, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.AlwaysShowDungeonMinimapIcon", "media", "Visual enhancements", "HUD & interface", 1070, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.FileSelectMoreInfo", "media", "Visual enhancements", "HUD & interface", 1080, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.BetterAmmoRendering", "media", "Visual enhancements", "HUD & interface", 1090, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.TimeFlowFileSelect", "media", "Visual enhancements", "HUD & interface", 1100, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Graphics.3DItemDrops", "media", "Visual enhancements", "Models & textures", 10, nullptr, "item_drops_3d", 1 },
    { kMenuOot, "gEnhancements.NewDrops", "media", "Visual enhancements", "Models & textures", 10, nullptr, "item_drops_3d", 1 },
    { kMenuOot, "gEnhancements.DisableLOD", "media", "Visual enhancements", "Models & textures", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.PauseMenuAnimatedLink", "media", "Visual enhancements", "Models & textures", 1010, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.EquipmentAlwaysVisible", "media", "Visual enhancements", "Models & textures", 1020, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.ScaleAdultEquipmentAsChild", "media", "Visual enhancements", "Models & textures", 1030, "gEnhancements.EquipmentAlwaysVisible", nullptr, 0 },
    { kMenuOot, "gEnhancements.FirstPersonGauntlets", "media", "Visual enhancements", "Models & textures", 1040, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.ShowDoorLocksOnBothSides", "media", "Visual enhancements", "Models & textures", 1050, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.ToTMedallionsColors", "media", "Visual enhancements", "Models & textures", 1060, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Mods.DisableBombBillboarding", "media", "Visual enhancements", "Mods", 10, nullptr, "mods_billboard", 1 },
    { kMenuMm, "gEnhancements.Mods.DisableGrottoRotation", "media", "Visual enhancements", "Mods", 10, nullptr, "mods_billboard", 1 },
    { kMenuOot, "gEnhancements.DisableBombBillboarding", "media", "Visual enhancements", "Mods", 10, nullptr, "mods_billboard", 1 },
    { kMenuOot, "gEnhancements.DisableGrottoRotation", "media", "Visual enhancements", "Mods", 10, nullptr, "mods_billboard", 1 },
    { kMenuMm, "gEnhancements.Mods.AlternateAssets", "media", "Visual enhancements", "Mods", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.DisableLinkSwordTrail", "media", "Visual enhancements", "Mods", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.3DSceneRender", "media", "Visual enhancements", "Mods", 1010, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.DisableFixedCamera", "media", "Visual enhancements", "Mods", 1020, "gEnhancements.3DSceneRender", nullptr, 0 },
    { kMenuOot, "gEnhancements.TextSpacing", "media", "Visual enhancements", "Mods", 1030, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Graphics.MotionBlur.Mode", "media", "Visual enhancements", "Motion blur", 1000, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Graphics.MotionBlur.Interpolate", "media", "Visual enhancements", "Motion blur", 1010, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Graphics.MotionBlur.Strength", "media", "Visual enhancements", "Motion blur", 1030, nullptr, nullptr, 0 },
    { kMenuMm, "gEnhancements.Graphics.DisableBlackBars", "media", "Visual enhancements", "Screen & effects", 10, nullptr, "black_bars", 1 },
    { kMenuOot, "gEnhancements.DisableBlackBars", "media", "Visual enhancements", "Screen & effects", 10, nullptr, "black_bars", 1 },
    { kMenuMm, "gSettings.LowResMode", "media", "Visual enhancements", "Screen & effects", 20, nullptr, "n64_mode", 1 },
    { kMenuOot, "gSettings.LowResMode", "media", "Visual enhancements", "Screen & effects", 20, nullptr, "n64_mode", 1 },
    { kMenuMm, "gEnhancements.Graphics.AuthenticLogo", "media", "Visual enhancements", "Screen & effects", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.RemoveSpinAttackDarkness", "media", "Visual enhancements", "Screen & effects", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "gEnhancements.DisableLinkSpinWithGoronPot", "media", "Visual enhancements", "Screen & effects", 1010, nullptr, nullptr, 0 },
    // Rows the null-CVar native widgets take (texts, buttons and inline windows keep their neighbours).
    { kMenuOot, "widget:Enhancements/Cheats/Clear Cutscene Pointer", "gameplay", "Cheats", "Glitch aids", 1010, nullptr, nullptr, 0 },
    { kMenuOot, "widget:Enhancements/Extra Modes/Enemy List", "gameplay", "Extra modes", "Enemy randomizer", 1040, "gEnhancements.RandomizedEnemies", nullptr, 0 },
    { kMenuOot, "widget:Enhancements/Extra Modes/Tier 1 Traps:", "gameplay", "Extra modes", "Traps", 1010, "gEnhancements.ExtraTraps.Enabled", nullptr, 0 },
    { kMenuOot, "widget:Enhancements/Extra Modes/Tier 2 Traps:", "gameplay", "Extra modes", "Traps", 1050, "gEnhancements.ExtraTraps.Enabled", nullptr, 0 },
    { kMenuOot, "widget:Enhancements/Extra Modes/Tier 3 Traps:", "gameplay", "Extra modes", "Traps", 1090, "gEnhancements.ExtraTraps.Enabled", nullptr, 0 },
    { kMenuOot, "widget:Enhancements/Skips & Speed-ups/All", "gameplay", "Time savers", "Cutscenes", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "widget:Enhancements/Skips & Speed-ups/None", "gameplay", "Time savers", "Cutscenes", 1010, nullptr, nullptr, 0 },
    { kMenuOot, "widget:Enhancements/Gameplay Stats/Gameplay Stats Inline", "trackers", "Gameplay Stats", "Window", 1010, nullptr, nullptr, 0 },
    { kMenuOot, "widget:Enhancements/Time Splits/Time Splits Inline", "trackers", "Time Splits", "Window", 1010, nullptr, nullptr, 0 },
    { kMenuMm, "widget:Enhancements/Cosmetic Editor/Cosmetic Editor Inline", "media", "Appearance", "Editor", 1010, nullptr, nullptr, 0 },
    { kMenuOot, "widget:Enhancements/Cosmetics Editor/Cosmetics Editor Inline", "media", "Appearance", "Editor", 1010, nullptr, nullptr, 0 },
    { kMenuMm, "widget:Enhancements/Audio Editor/Audio Editor Inline", "media", "Music & sounds", "Editor", 1010, nullptr, nullptr, 0 },
    { kMenuOot, "widget:Enhancements/Audio Editor/Audio Editor Inline", "media", "Music & sounds", "Editor", 1010, nullptr, nullptr, 0 },
    { kMenuMm, "widget:Enhancements/HUD Editor/HUD Editor Inline", "media", "Screen & HUD", "Advanced HUD editor (MM)", 1010, nullptr, nullptr, 0 },
    { kMenuMm, "widget:Enhancements/Graphics/On/Off", "media", "Visual enhancements", "Motion blur", 1020, nullptr, nullptr, 0 },
    { kMenuMm, "widget:Enhancements/Graphics/Strength", "media", "Visual enhancements", "Motion blur", 1040, nullptr, nullptr, 0 },
    // Combo-drawn gameplay rules and both-games checkboxes (spec 4.2). Rank comes from the native row the
    // combo control replaces.
    { kMenuOot, "rule:gEnhancements.BlueFireArrows", "gameplay", "Items & equipment", "Bow & arrows", 1010, nullptr, nullptr, 0 },
    { kMenuOot, "rule:gEnhancements.SunlightArrows", "gameplay", "Items & equipment", "Bow & arrows", 1020, nullptr, nullptr, 0 },
    { kMenuOot, "rule:gEnhancements.OpenAllHours", "gameplay", "Quality of life", "Time of day & cycle", 10, nullptr, nullptr, 0 },
    { kMenuMm, "rule:gEnhancements.DifficultyOptions.GibdoTradeSequence", "gameplay", "Difficulty", "Quest requirements", 10, nullptr, nullptr, 0 },
    { kMenuMm, "rule:gEnhancements.DifficultyOptions.FrogChoirCount", "gameplay", "Difficulty", "Quest requirements", 10, nullptr, nullptr, 0 },
    { kMenuMm, "rule:gEnhancements.Minigames.AlwaysWinDoggyRace", "gameplay", "Difficulty", "Minigames", 1070, nullptr, nullptr, 0 },
    { kMenuMm, "rule:gEnhancements.Player.InfiniteDekuHopping", "gameplay", "Items & equipment", "Masks & forms", 20, nullptr, nullptr, 0 },
    { kMenuOot, "rule:gEnhancements.DayGravePull", "gameplay", "Quality of life", "Time of day & cycle", 10, nullptr, nullptr, 0 },
    { kMenuOot, "rule:gEnhancements.NightGSAlwaysSpawn", "gameplay", "Quality of life", "Time of day & cycle", 10, nullptr, nullptr, 0 },
    { kMenuOot, "rule:gCheats.SuperTunic", "gameplay", "Items & equipment", "Weapons & gear", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "rule:gCheats.DekuStick", "gameplay", "Items & equipment", "Weapons & gear", 1010, nullptr, nullptr, 0 },
    { kMenuOot, "rule:gEnhancements.TimeTravel", "gameplay", "Items & equipment", "Ocarina & songs", 20, nullptr, nullptr, 0 },
    { kMenuMm, "rule:gEnhancements.Masks.FierceDeitysAnywhere", "gameplay", "Items & equipment", "Masks & forms", 1050, nullptr, nullptr, 0 },
    { kMenuMm, "rule:gEnhancements.DifficultyOptions.BremenChickCount", "gameplay", "Difficulty", "Quest requirements", 12, nullptr, nullptr, 0 },
    { kMenuMm, "rule:gEnhancements.Songs.ZoraEggCount", "gameplay", "Difficulty", "Quest requirements", 10, nullptr, nullptr, 0 },
    { kMenuBoth, "rule:gCombo.Enhancements.ForeignSongs", "gameplay", "Warps & travel", "Cross-game songs", 10, nullptr, nullptr, 1 },
    { kMenuBoth, "rule:gCombo.Enhancements.SongTravel", "gameplay", "Warps & travel", "Cross-game songs", 10, nullptr, nullptr, 1 },
    { kMenuMm, "rule:gEnhancements.Masks.GoronRollingIgnoresMagic", "gameplay", "Items & equipment", "Masks & forms", 1070, nullptr, nullptr, 0 },
    { kMenuMm, "rule:gEnhancements.Masks.BlastMaskKeg", "gameplay", "Items & equipment", "Masks & forms", 1010, nullptr, nullptr, 0 },
    { kMenuMm, "rule:gEnhancements.Playback.SkipScarecrowSong", "gameplay", "Time savers", "Quests & errands", 10, nullptr, nullptr, 0 },
    { kMenuMm, "rule:gEnhancements.Timesavers.SkipBalladOfWindfish", "gameplay", "Time savers", "Quests & errands", 1060, nullptr, nullptr, 0 },
    { kMenuBoth, "pair:Equip items with the D-pad", "settings", "Controls", "Buttons & aiming", 10, nullptr, nullptr, 1 },
    { kMenuBoth, "pair:Allow unequipping items", "gameplay", "Items & equipment", "Equipping", 30, nullptr, nullptr, 1 },
    { kMenuBoth, "pair:Quick magic-arrow equip", "gameplay", "Items & equipment", "Bow & arrows", 10, nullptr, nullptr, 1 },
    { kMenuBoth, "pair:Cycle arrow types", "gameplay", "Items & equipment", "Bow & arrows", 20, nullptr, nullptr, 1 },
    { kMenuBoth, "pair:Autosave", "gameplay", "Saving", "Autosave", 10, nullptr, nullptr, 1 },
    // Native headings and whole sidebars: rows without a CVar of their own, and rows the audit map does not hold.
    { kMenuOot, "heading:Enhancements/Cheats/Save States", "gameplay", "Cheats", "Save states", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "heading:Enhancements/Cheats/Instant Age Change", "gameplay", "Cheats", "Time", 1010, nullptr, nullptr, 0 },
    { kMenuOot, "sidebar:Enhancements/Extra Modes", "gameplay", "Extra modes", nullptr, 1000, nullptr, nullptr, 0 },
    { kMenuOot, "heading:Settings/General/Boot", "settings", "General", "Boot", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "heading:Settings/General/Accessibility", "settings", "General", "Accessibility", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "heading:Settings/General/Languages", "settings", "General", "Languages", 1000, nullptr, nullptr, 0 },
    { kMenuBoth, "gSettings.ResetBtn", "settings", "General", "Shortcuts", 10, nullptr, "reset_button", 1 },
    { kMenuOot, "sidebar:Settings/Controls", "settings", "Controls", "Controller mapping", 1000, nullptr, nullptr, 0 },
    { kMenuOot, "sidebar:Settings/Audio", "media", "Audio", "Volume", 1000, nullptr, nullptr, 0 },
    // The retired HUD & overlays page: MM notifications join OoT's page, MM's HUD editor folds into Screen & HUD.
    { kMenuMm, "heading:Settings/Overlay/Notifications", "settings", "Notifications", nullptr, 1000, nullptr, nullptr, 0 },
    { kMenuMm, "gWindows.HudEditor", "media", "Screen & HUD", "Advanced HUD editor (MM)", 1000, nullptr, nullptr, 0 },
    { kMenuMm, "sidebar:Enhancements/HUD Editor", "media", "Screen & HUD", "Advanced HUD editor (MM)", 1010, nullptr, nullptr, 0 },
};
// clang-format on

inline bool MenuPlacementForGame(const MenuPlacement& row, bool oot) {
    return (row.games & (oot ? kMenuOot : kMenuMm)) != 0;
}
inline const MenuPageLayout* FindMenuPage(const std::string& scope, const std::string& page) {
    for (const auto& layout : kMenuPages)
        if (scope == layout.scope && page == layout.page)
            return &layout;
    return nullptr;
}
// Index of heading in the page's fixed list, or -1.
inline int MenuHeadingIndex(const MenuPageLayout& layout, const char* heading) {
    for (int i = 0; heading && i < layout.headingCount; ++i)
        if (!std::strcmp(layout.headings[i], heading))
            return i;
    return -1;
}

namespace Detail {
inline const std::unordered_multimap<std::string_view, const MenuPlacement*>& MenuPlacementIndex() {
    static const auto index = [] {
        std::unordered_multimap<std::string_view, const MenuPlacement*> rows;
        for (const auto& row : kMenuPlacements)
            rows.emplace(row.key, &row);
        return rows;
    }();
    return index;
}
inline const std::vector<const MenuPlacement*>& MenuPlacementPrefixes() {
    static const auto prefixes = [] {
        std::vector<const MenuPlacement*> rows;
        for (const auto& row : kMenuPlacements)
            if (const size_t length = std::strlen(row.key); length && row.key[length - 1] == '*')
                rows.push_back(&row);
        return rows;
    }();
    return prefixes;
}
// A null scope accepts a row of any scope.
inline const MenuPlacement* FindMenuPlacementKey(bool oot, std::string_view key, const char* scope = nullptr) {
    auto [first, last] = MenuPlacementIndex().equal_range(key);
    for (; first != last; ++first)
        if (MenuPlacementForGame(*first->second, oot) && (!scope || !std::strcmp(first->second->scope, scope)))
            return first->second;
    return nullptr;
}
} // namespace Detail

// The row that places one native widget: its CVar (exact, then prefix), else its name when it has
// no CVar, then its native heading, then its native sidebar. nullptr: not placed, so MenuRoute's
// sidebar destination and the native heading stand.
inline const MenuPlacement* FindMenuPlacement(bool oot, const std::string& section, const std::string& sidebar,
                                              const std::string& heading, const std::string& key,
                                              const std::string& name = {}) {
    if (!key.empty()) {
        if (const auto* row = Detail::FindMenuPlacementKey(oot, key))
            return row;
        for (const auto* row : Detail::MenuPlacementPrefixes()) {
            const size_t length = std::strlen(row->key) - 1;
            if (MenuPlacementForGame(*row, oot) && key.compare(0, length, row->key, length) == 0)
                return row;
        }
    } else if (!name.empty()) {
        if (const auto* row = Detail::FindMenuPlacementKey(oot, "widget:" + section + "/" + sidebar + "/" + name))
            return row;
    }
    if (!heading.empty())
        if (const auto* row = Detail::FindMenuPlacementKey(oot, "heading:" + section + "/" + sidebar + "/" + heading))
            return row;
    return Detail::FindMenuPlacementKey(oot, "sidebar:" + section + "/" + sidebar);
}
} // namespace ComboRando
