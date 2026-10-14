// combo/gui/ComboMenuLabels.h
//
// ComboShip: the words the combined menu shows for native rows. Where the two games name one feature
// differently, both take one label; a value readout says its unit ("+%d" adds, "%dx" multiplies,
// "%d frames"). Generated once from the T8 audit's menu-placements.json, edited by hand since.
// Only display text changes here: never a CVar, a choice or a rule. The native name stays a search
// word, so a player who knows the old label still finds the row.
#pragma once
#include "ComboMenuPlacement.h"
#include <string>
#include <string_view>
#include <unordered_map>

namespace ComboRando {
struct MenuLabel {
    uint8_t games;
    const char* cvar;
    const char* label;   // shown instead of the native name; nullptr keeps it
    const char* format;  // value readout, showing the stored value as is; nullptr keeps the game's own
    const char* tooltip; // a line added under the native tooltip, or nullptr
    const char* aliases; // more search words, or nullptr
};

// clang-format off
inline constexpr MenuLabel kMenuLabels[] = {
    // One label where the two games name the same feature differently, and readouts with honest units.
    { kMenuMm, "gCheats.EasyFrameAdvance", "Easy frame advance", nullptr, nullptr, nullptr },
    { kMenuOot, "gCheats.EasyFrameAdvance", "Easy frame advance", nullptr, nullptr, nullptr },
    { kMenuMm, "gCheats.InfiniteHealth", "Infinite health", nullptr, nullptr, nullptr },
    { kMenuOot, "gCheats.InfiniteHealth", "Infinite health", nullptr, nullptr, nullptr },
    { kMenuMm, "gCheats.InfiniteMagic", "Infinite magic", nullptr, nullptr, nullptr },
    { kMenuOot, "gCheats.InfiniteMagic", "Infinite magic", nullptr, nullptr, nullptr },
    { kMenuMm, "gCheats.InfiniteRupees", "Infinite rupees", nullptr, nullptr, nullptr },
    { kMenuOot, "gCheats.InfiniteMoney", "Infinite rupees", nullptr, nullptr, nullptr },
    { kMenuMm, "gCheats.InfiniteConsumables", "Infinite ammo", nullptr, nullptr, nullptr },
    { kMenuOot, "gCheats.InfiniteAmmo", "Infinite ammo", nullptr, nullptr, nullptr },
    { kMenuMm, "gCheats.InfiniteEponaCarrots", "Infinite Epona boosts", nullptr, nullptr, nullptr },
    { kMenuOot, "gCheats.InfiniteEponaBoost", "Infinite Epona boosts", nullptr, nullptr, nullptr },
    { kMenuMm, "gCheats.HookshotAnywhere", "Hookshot anywhere", nullptr, nullptr, nullptr },
    { kMenuOot, "gCheats.HookshotEverything", "Hookshot anywhere", nullptr, nullptr, nullptr },
    { kMenuMm, "gCheats.ClimbAnywhere", "Climb anywhere", nullptr, nullptr, nullptr },
    { kMenuOot, "gCheats.ClimbEverything", "Climb anywhere", nullptr, nullptr, nullptr },
    { kMenuMm, "gCheats.SpeedModifier.Value", nullptr, "%.2fx", nullptr, nullptr },
    { kMenuOot, "gCheats.SpeedModifier.Value", nullptr, "%.2fx", nullptr, nullptr },
    { kMenuMm, "gEnhancements.Equipment.ChuDrops", "Bombchu drops", nullptr, nullptr, nullptr },
    { kMenuOot, "gEnhancements.EnableBombchuDrops", "Bombchu drops", nullptr, nullptr, nullptr },
    { kMenuMm, "gEnhancements.Fixes.FixTexturesOOB", "Fix out-of-bounds textures", nullptr, nullptr, nullptr },
    { kMenuOot, "gEnhancements.FixTexturesOOB", "Fix out-of-bounds textures", nullptr, nullptr, nullptr },
    { kMenuMm, "gEnhancements.Graphics.BowReticle", "Bow reticle", nullptr, nullptr, nullptr },
    { kMenuOot, "gEnhancements.BowReticle", "Bow and slingshot reticle", nullptr, nullptr, nullptr },
    { kMenuMm, "gEnhancements.PlayerActions.RemoteBombchu", "Remote Bombchu", nullptr, nullptr, nullptr },
    { kMenuMm, "gEnhancements.PlayerActions.InstantRecall", "Instant boomerang recall", nullptr, nullptr, nullptr },
    { kMenuOot, "gEnhancements.FastBoomerang", "Instant boomerang recall", nullptr, nullptr, nullptr },
    { kMenuOot, "gEnhancements.ChestSizeAndTextureMatchContents", "Containers match contents", nullptr, nullptr, nullptr },
    { kMenuMm, "gEnhancements.Saving.AutosaveInterval", "Autosave interval", "%d minutes", nullptr, nullptr },
    { kMenuMm, "gEnhancements.Cutscenes.HideTitleCards", "Hide title cards", nullptr, nullptr, nullptr },
    { kMenuOot, "gEnhancements.TimeSavers.DisableTitleCard", "Hide title cards", nullptr, nullptr, nullptr },
    { kMenuMm, "gEnhancements.Cutscenes.SkipIntroSequence", "Skip intro", nullptr, nullptr, nullptr },
    { kMenuOot, "gEnhancements.TimeSavers.SkipCutscene.Intro", "Skip intro", nullptr, nullptr, nullptr },
    { kMenuMm, "gEnhancements.Cutscenes.SkipOnePointCutscenes", "Skip one-point cutscenes", nullptr, nullptr, nullptr },
    { kMenuOot, "gEnhancements.TimeSavers.SkipCutscene.OnePoint", "Skip one-point cutscenes", nullptr, nullptr, nullptr },
    { kMenuMm, "gEnhancements.Cutscenes.SkipGetItemCutscenes", "Skip get-item animations", nullptr, nullptr, nullptr },
    { kMenuOot, "gRandoEnhancements.TimeSavers.SkipGetItemAnimation", "Skip get-item animations", nullptr, nullptr, nullptr },
    { kMenuMm, "gEnhancements.Playback.SkipScarecrowSong", "Skip Scarecrow's Song", nullptr, nullptr, nullptr },
    { kMenuOot, "gEnhancements.InstantScarecrow", "Skip Scarecrow's Song", nullptr, nullptr, nullptr },
    { kMenuMm, "gEnhancements.Player.ClimbSpeed", nullptr, "%dx", "Multiplies the climbing speed.", nullptr },
    { kMenuOot, "gEnhancements.ClimbSpeed", "Climb speed", "+%d", "Adds to the climbing animation speed.", nullptr },
    { kMenuMm, "gEnhancements.Player.FasterPushAndPull", "Faster block pushing", nullptr, nullptr, nullptr },
    { kMenuOot, "gEnhancements.FasterBlockPush", "Faster block pushing", "+%d", nullptr, nullptr },
    { kMenuMm, "gEnhancements.Songs.FasterSongPlayback", "Faster song playback", nullptr, nullptr, nullptr },
    { kMenuOot, "gEnhancements.FastOcarinaPlayback", "Faster song playback", nullptr, nullptr, nullptr },
    { kMenuMm, "gEnhancements.Cutscenes.AutoAdvanceEndingText", "Auto-advance ending text", nullptr, nullptr, nullptr },
    { kMenuOot, "gEnhancements.NoInputForCredits", "Auto-advance ending text", nullptr, nullptr, nullptr },
    { kMenuMm, "gEnhancements.Songs.PauseOwlWarp", "Pause menu owl warp", nullptr, nullptr, nullptr },
    { kMenuMm, "gEnhancements.Songs.BetterOwlWarpMenu", "Better owl warp menu", nullptr, nullptr, nullptr },
    { kMenuOot, "gEnhancements.PauseWarp", "Pause menu warp", nullptr, nullptr, nullptr },
    { kMenuMm, "gEnhancements.Restorations.PauseBufferWindow", "Pause buffer input window", "%d frames", nullptr, nullptr },
    { kMenuOot, "gEnhancements.PauseBufferWindow", "Pause buffer input window", "%d frames", nullptr, nullptr },
    { kMenuMm, "gSettings.SimulatedInputLag", "Simulated input lag", "%d frames", nullptr, nullptr },
    { kMenuOot, "gSettings.SimulatedInputLag", "Simulated input lag", "%d frames", nullptr, nullptr },
    // OoT's earlier names find both games' ocarina input rows.
    { kMenuMm, "gEnhancements.Playback.DpadOcarina", "D-pad ocarina", nullptr, nullptr, "Dpad Ocarina Playback" },
    { kMenuOot, "gSettings.CustomOcarina.Dpad", "D-pad ocarina", nullptr, nullptr, nullptr },
    { kMenuMm, "gEnhancements.Playback.RightStickOcarina", "Right stick ocarina", nullptr, nullptr, "Right Stick Ocarina Playback" },
    { kMenuOot, "gSettings.CustomOcarina.RightStick", "Right stick ocarina", nullptr, nullptr, nullptr },
    { kMenuMm, "gEnhancements.A11y.NoScreenFlashForEnemyKill", "Disable screen flash on kills", nullptr, nullptr, nullptr },
    { kMenuOot, "gSettings.A11yNoScreenFlashForFinishingBlow", "Disable screen flash on kills", nullptr, nullptr, nullptr },
    { kMenuMm, "gEnhancements.Graphics.3DItemDrops", "3D item drops", nullptr, nullptr, nullptr },
    { kMenuOot, "gEnhancements.NewDrops", "3D item drops", nullptr, nullptr, nullptr },
    // Difficulty > Minigames gathers OoT's minigame headings under one: a row whose native name relied on its
    // heading carries the minigame's name.
    { kMenuOot, "gEnhancements.CustomizeShootingGallery", "Shooting Gallery: Customize Behavior", nullptr, nullptr, nullptr },
    { kMenuOot, "gEnhancements.InstantShootingGalleryWin", "Shooting Gallery: Instant Win", nullptr, nullptr, nullptr },
    { kMenuOot, "gEnhancements.CustomizeBombchuBowling", "Bombchu Bowling: Customize Behavior", nullptr, nullptr, nullptr },
    { kMenuOot, "gEnhancements.CustomizeHorsebackArchery", "Horseback Archery: Customize Behavior", nullptr, nullptr, nullptr },
    { kMenuOot, "gEnhancements.InstantHorsebackArcheryWin", "Horseback Archery: Instant Win", nullptr, nullptr, nullptr },
    { kMenuOot, "gEnhancements.HorsebackArcheryAlwaysScore", "Horseback Archery: Always Score 100", nullptr, nullptr, nullptr },
    { kMenuOot, "gEnhancements.HorsebackArcheryAmmo", "Horseback Archery: Arrow Count", nullptr, nullptr, nullptr },
    { kMenuOot, "gEnhancements.HorsebackArcheryBothPrizes", "Horseback Archery: Both Prizes", nullptr, nullptr, "Multiple Prizes" },
    { kMenuOot, "gEnhancements.CustomizeFrogsOcarinaGame", "Frogs' Ocarina Game: Customize Behavior", nullptr, nullptr, nullptr },
    { kMenuOot, "gEnhancements.InstantFrogsGameWin", "Frogs' Ocarina Game: Instant Win", nullptr, nullptr, nullptr },
    { kMenuOot, "gEnhancements.FrogsUnlimitedFailTime", "Frogs' Ocarina Game: Unlimited Playback Time", nullptr, nullptr, nullptr },
    { kMenuOot, "gEnhancements.CustomizeOcarinaGame", "Lost Woods Ocarina Game: Customize Behavior", nullptr, nullptr, nullptr },
    { kMenuOot, "gEnhancements.InstantOcarinaGameWin", "Lost Woods Ocarina Game: Instant Win", nullptr, nullptr, nullptr },
    { kMenuOot, "gEnhancements.OcarinaUnlimitedFailTime", "Lost Woods Ocarina Game: Unlimited Playback Time", nullptr, nullptr, nullptr },
    { kMenuOot, "gEnhancements.IngoRaceOnce", "Ingo's Race: Number of Races", nullptr, nullptr, nullptr },
    { kMenuOot, "gEnhancements.DivingGame.TimeLimit", "Rupee Diving Game: Time Limit", nullptr, nullptr, nullptr },
    { kMenuOot, "gEnhancements.CustomizeFishing", "Fishing: Customize Behavior", nullptr, nullptr, nullptr },
    { kMenuOot, "gEnhancements.DampeBothPrizes", "Dampe's Race: Both Prizes", nullptr, nullptr, "Multiple Prizes" },
    // Rows the audit map does not hold (spec 3.4 and 4.4).
    { kMenuOot, "gSettings.Notifications.Duration", "Duration", "%.1f seconds", nullptr, nullptr },
    { kMenuMm, "gNotifications.Duration", "Duration", "%.1f seconds", nullptr, nullptr },
    { kMenuOot, "gSettings.Notifications.BgOpacity", "Background opacity", nullptr, nullptr, nullptr },
    { kMenuMm, "gNotifications.BgOpacity", "Background opacity", nullptr, nullptr, nullptr },
    { kMenuOot, "gSettings.Notifications.Size", "Size", "%.1f", nullptr, nullptr },
    { kMenuMm, "gNotifications.Size", "Size", "%.1f", nullptr, nullptr },
    { kMenuMm, "gEnhancements.Camera.FirstPerson.InvertX", "Invert X axis", nullptr, nullptr, nullptr },
    { kMenuMm, "gEnhancements.Camera.FirstPerson.InvertY", "Invert Y axis", nullptr, nullptr, nullptr },
    { kMenuMm, "gEnhancements.Camera.RightStick.CameraSensitivity.X", "Third-person horizontal sensitivity", nullptr, nullptr, nullptr },
    { kMenuMm, "gEnhancements.Camera.RightStick.CameraSensitivity.Y", "Third-person vertical sensitivity", nullptr, nullptr, nullptr },
    { kMenuOot, "gEnhancements.RandomizedEnemies", nullptr, nullptr, "New seeds made while this is on leave out Enemy Souls (OoT).", nullptr },
    { kMenuMm, "gEnhancements.Minigames.CuccoShackCuccoCount", nullptr, nullptr, "Both lower the chicks needed for the Bremen Mask; the lower number wins.", nullptr },
    { kMenuMm, "gEnhancements.DifficultyOptions.BremenChickCount", nullptr, nullptr, "Both lower the chicks needed for the Bremen Mask; the lower number wins.", nullptr },
    { kMenuMm, "gRando.CSMC", nullptr, nullptr, nullptr, "Container Style Matches Contents CSMC" },
    { kMenuMm, "gRando.JunkItems", nullptr, nullptr, nullptr, "Junk Items" },
    { kMenuMm, "gRando.TrapItems", nullptr, nullptr, nullptr, "Trap Behavior disguise" },
    { kMenuMm, "gRando.Traps.Freeze", nullptr, nullptr, nullptr, "trap effects types" },
    { kMenuMm, "gRando.Traps.Blast", nullptr, nullptr, nullptr, "trap effects types" },
    { kMenuMm, "gRando.Traps.Shock", nullptr, nullptr, nullptr, "trap effects types" },
    { kMenuMm, "gRando.Traps.Jinx", nullptr, nullptr, nullptr, "trap effects types" },
    { kMenuMm, "gRando.Traps.Wallet", nullptr, nullptr, nullptr, "trap effects types" },
    { kMenuMm, "gRando.Traps.Enemy", nullptr, nullptr, nullptr, "trap effects types enemy" },
    { kMenuMm, "gRando.Traps.Time", nullptr, nullptr, nullptr, "trap effects types" },
    { kMenuMm, "gRando.Traps.Fire", nullptr, nullptr, nullptr, "trap effects types" },
    { kMenuMm, "gRando.Traps.Knockback", nullptr, nullptr, nullptr, "trap effects types" },
    // Pop-out buttons: one wording, the subject as its page names it.
    { kMenuOot, "gOpenWindows.SohStats", "Pop out Stats", nullptr, nullptr, nullptr },
    { kMenuMm, "gOpenWindows.Stats", "Pop out Stats", nullptr, nullptr, nullptr },
    { kMenuOot, "gOpenWindows.SohConsole", "Pop out Console", nullptr, nullptr, nullptr },
    { kMenuMm, "gOpenWindows.Console", "Pop out Console", nullptr, nullptr, nullptr },
    { kMenuOot, "gOpenWindows.SaveEditor", "Pop out Save Editor", nullptr, nullptr, nullptr },
    { kMenuMm, "gWindows.SaveEditor", "Pop out Save Editor", nullptr, nullptr, nullptr },
    { kMenuOot, "gOpenWindows.HookDebugger", "Pop out Hook Debugger", nullptr, nullptr, nullptr },
    { kMenuMm, "gWindows.HookDebugger", "Pop out Hook Debugger", nullptr, nullptr, nullptr },
    { kMenuOot, "gOpenWindows.CollisionViewer", "Pop out Collision Viewer", nullptr, nullptr, nullptr },
    { kMenuMm, "gWindows.CollisionViewer", "Pop out Collision Viewer", nullptr, nullptr, nullptr },
    { kMenuOot, "gOpenWindows.ActorViewer", "Pop out Actor Viewer", nullptr, nullptr, nullptr },
    { kMenuMm, "gWindows.ActorViewer", "Pop out Actor Viewer", nullptr, nullptr, nullptr },
    { kMenuOot, "gOpenWindows.DisplayListViewer", "Pop out Display lists", nullptr, nullptr, nullptr },
    { kMenuMm, "gWindows.DLViewer", "Pop out Display lists", nullptr, nullptr, nullptr },
    { kMenuOot, "gOpenWindows.ValueViewer", "Pop out Value Viewer", nullptr, nullptr, nullptr },
    { kMenuOot, "gOpenWindows.MessageViewer", "Pop out Message Viewer", nullptr, nullptr, nullptr },
    { kMenuMm, "gWindows.MessageViewer", "Pop out Message Viewer", nullptr, nullptr, nullptr },
    { kMenuOot, "gOpenWindows.SohGfxDebugger", "Pop out Gfx Debugger", nullptr, nullptr, nullptr },
    { kMenuMm, "gOpenWindows.GfxDebugger", "Pop out Gfx Debugger", nullptr, nullptr, nullptr },
    { kMenuMm, "gWindows.EventLog", "Pop out Event Log", nullptr, nullptr, nullptr },
    { kMenuOot, "gOpenWindows.CosmeticsEditor", "Pop out cosmetics editor", nullptr, nullptr, nullptr },
    { kMenuMm, "gWindows.CosmeticEditor", "Pop out cosmetics editor", nullptr, nullptr, nullptr },
    { kMenuOot, "gOpenWindows.AudioEditor", "Pop out audio editor", nullptr, nullptr, nullptr },
    { kMenuMm, "gWindows.AudioEditor", "Pop out audio editor", nullptr, nullptr, nullptr },
    { kMenuMm, "gWindows.HudEditor", "Pop out HUD editor", nullptr, nullptr, nullptr },
    { kMenuOot, "gOpenWindows.GameplayStats", "Pop out Gameplay Stats", nullptr, nullptr, nullptr },
    { kMenuOot, "gOpenWindows.TimeSplits", "Pop out Time Splits", nullptr, nullptr, nullptr },
    { kMenuMm, "gWindows.Timesplits.Settings", "Pop out Time Splits settings", nullptr, nullptr, nullptr },
    { kMenuOot, "gOpenWindows.ControllerConfiguration", "Pop out controller bindings", nullptr, nullptr, nullptr },
    { kMenuMm, "gWindows.BenInputEditor", "Pop out controller bindings", nullptr, nullptr, nullptr },
    { kMenuOot, "gOpenWindows.InputViewerSettings", "Pop out Input Viewer settings", nullptr, nullptr, nullptr },
    { kMenuMm, "gWindows.InputViewerSettings", "Pop out Input Viewer settings", nullptr, nullptr, nullptr },
    { kMenuOot, "gOpenWindows.ModMenu", "Pop out mod menu", nullptr, nullptr, nullptr },
    { kMenuMm, "gWindows.ModMenu", "Pop out mod menu", nullptr, nullptr, nullptr },
};
// clang-format on

inline const MenuLabel* FindMenuLabel(bool oot, const char* cvar) {
    static const auto index = [] {
        std::unordered_multimap<std::string_view, const MenuLabel*> rows;
        for (const auto& row : kMenuLabels)
            rows.emplace(row.cvar, &row);
        return rows;
    }();
    if (!cvar || !*cvar)
        return nullptr;
    auto [first, last] = index.equal_range(cvar);
    for (; first != last; ++first)
        if (first->second->games & (oot ? kMenuOot : kMenuMm))
            return first->second;
    return nullptr;
}
// The words a native row shows: the table's label, else the native name without an ImGui id suffix
// or a printf value part ("Crawl Speed %dx" -> "Crawl Speed", "Multiplier:" -> "Multiplier").
inline std::string MenuRowLabel(bool oot, const char* cvar, const char* name) {
    if (const auto* row = FindMenuLabel(oot, cvar); row && row->label)
        return row->label;
    std::string label = name ? name : "";
    if (const auto id = label.find("##"); id != std::string::npos)
        label.resize(id);
    if (const auto value = label.find('%'); value != std::string::npos)
        label.resize(value);
    while (!label.empty() && (label.back() == ' ' || label.back() == ':' || label.back() == '+'))
        label.pop_back();
    return label.empty() && name ? name : label;
}
// Search words for a native row besides its label: the native name (an old label still finds the
// row) and the table's aliases.
inline std::string MenuRowKeywords(bool oot, const char* cvar, const char* name) {
    std::string keywords = name ? name : "";
    if (const auto* row = FindMenuLabel(oot, cvar); row && row->aliases)
        keywords += std::string(" ") + row->aliases;
    return keywords;
}
// The native tooltip with the table's extra line, if any.
inline std::string MenuRowTooltip(bool oot, const char* cvar, const char* tooltip) {
    std::string text = tooltip ? tooltip : "";
    if (const auto* row = FindMenuLabel(oot, cvar); row && row->tooltip)
        text += (text.empty() ? "" : "\n\n") + std::string(row->tooltip);
    return text;
}
} // namespace ComboRando
