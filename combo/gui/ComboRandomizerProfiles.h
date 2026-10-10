#pragma once
#include "menu/ComboRandomizerProfileABI.h"
#include "ComboMenuABI.h"
#include <nlohmann/json.hpp>
namespace ComboRando { struct GameMenu; }
namespace ComboRando::ProfileUI {
void DrawSelector(); void BeginFrame(); void EndFrame();
bool Active(); bool Editable(); bool Ready(); int SelectedSlot();
bool DrawUnavailableSettings();
int ReadInteger(const char*,int); void WriteInteger(const char*,int);
void DrawSidebar(const CwSidebar&,const GameMenu&,int);
void DrawWidget(const CwWidget&,const GameMenu&); void DrawCustom(const char*,int);
void DrawGeneration(); void DrawHome(); void DrawAdvanced();
void DrawTricks(); void DrawSavedPlacements();
const nlohmann::json& Catalogue();
}
