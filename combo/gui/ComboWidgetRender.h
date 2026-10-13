// combo/gui/ComboWidgetRender.h
//
// ComboShip: comboui-side renderer for ONE flat C-ABI widget (CwWidget). Declarative
// kinds are drawn here with comboui's own ImGui calls over the shared libultraship CVar
// store; the non-declarative slices (preFunc disable-eval, custom-draw, post-change
// callback) are routed BY INDEX back into the owning game via the resolved export fn-ptrs.
//
// DESIGN NOTE: this file deliberately does NOT construct a ResourceManagerScope and does
// NOT touch any ResourceManager. RM scoping for the invoke callbacks lives GAME-SIDE,
// inside each game's own invoke/draw/eval export — comboui just calls them by index.
#pragma once
#include "ComboMenuABI.h"

namespace ComboRando {

struct GameMenu; // from ComboMenuModel.h

// Render one widget. CVar/declarative kinds are delegated to the owning game's real UIWidgets when
// its DrawWidget export is present (else drawn here with comboui's raw ImGui). widthBudget is the
// WrappedText char budget for labels (typically 90/columns). showHidden draws a widget its menu hides
// greyed out instead of not at all (a search result still names and shows its setting).
void RenderWidget(const CwWidget& w, const GameMenu& game, int widthBudget = 90, bool showHidden = false);

// Draw a combo-owned row placed among native rows: a gameplay rule (index <= -1000) or a checkbox that
// sets both games (index <= -2000). Returns false for any other widget. Defined in ComboMenu.cpp.
bool DrawMenuPseudoRow(const CwWidget& widget, bool randomizer);

// Render all of a sidebar's widgets honoring CwSidebar::columnCount — the faithful multi-column
// layout of the native menu (1 column => linear). Widgets carry their source column (CwWidget.column,
// ABI v2). Use this instead of looping RenderWidget so every section gets the right column layout.
void RenderSidebarWidgets(const CwSidebar& side, const GameMenu& game);

} // namespace ComboRando
