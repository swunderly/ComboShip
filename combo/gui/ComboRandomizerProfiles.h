#pragma once
#include "menu/ComboRandomizerProfileABI.h"
#include "ComboMenuABI.h"
#include <nlohmann/json.hpp>
namespace ComboRando {
struct GameMenu;
// Why MM does not offer one of its item-pool options (an RO_ name) here, as its own rows decide (2S2H Rando/Menu.cpp
// SpellPoolCheckbox, GearPoolCheckbox): their disabled tooltip, or nullptr where it is offered (ComboMenu.cpp).
const char* MmPoolOptionUnavailable(const char* option, bool swimShuffled);
} // namespace ComboRando
namespace ComboRando::ProfileUI {
void DrawSelector();
void BeginFrame();
void EndFrame();
void Flush();
bool Active();
bool Editable();
bool Ready();
int SelectedSlot();
bool DrawUnavailableSettings();
int ReadInteger(const char*, int);
void WriteInteger(const char*, int);
void DrawSidebar(const CwSidebar&, const GameMenu&, int);
void DrawWidget(const CwWidget&, const GameMenu&);
void DrawCustom(const char*, int);
void DrawGeneration();
void DrawHome();
void DrawAdvanced();
void DrawTricks();
void DrawSavedPlacements();
const nlohmann::json& Catalogue();
// The profile pages' display label for a native OoT option (OoTMM's name), or nullptr where it keeps its own.
const char* OptionDisplayLabel(const char* key);
} // namespace ComboRando::ProfileUI
