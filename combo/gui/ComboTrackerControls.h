#pragma once
#include "ComboCheckTrackerModel.h"
#include <imgui.h>

namespace ComboTrackerUI {
enum Kind { Check, Item, Hint, Count };
void Migrate(Kind kind);
void Save();
bool Checkbox(const char* label, const char* cvar, int fallback = 0);
void DrawEnabled(Kind kind);
void DrawOpenControls(Kind kind);
void DrawAppearance(Kind kind, bool textSize = true);
bool Visible(Kind kind);
bool Begin(Kind kind);
void End(Kind kind);
void RegisterItemWindow();
void DrawItemSettings();
}
