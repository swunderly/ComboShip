// combo/gui/ComboWidgetRender.cpp
//
// ComboShip: render one flat C-ABI widget (CwWidget) with comboui's own ImGui calls over
// the shared libultraship CVar store. Non-declarative slices (preFunc disable-eval,
// custom-draw, post-change callback) are routed BY INDEX into the owning game's exports.
//
// DESIGN NOTE: NO ResourceManager usage here. RM scoping for invoke callbacks is GAME-SIDE
// (inside each game's own invoke/draw/eval export). comboui only calls game.evalDisabled,
// game.invokeCallback and game.drawCustom by index — no shared_ptr crosses the boundary.
//
// KNOWN ISSUE (docs/KNOWN_ISSUES.md): settings whose effect depends on a registered per-page
// MenuUpdate function (e.g. SoH's Advanced Resolution editor — local-var bindings translated to
// effective CVars by UpdateResolutionVars) do NOT apply through this generic CVar-write path.
// Toggling them here writes the UI CVars but not the effective ones, so the viewport never changes.
#include "ComboWidgetRender.h"
#include "ComboRuleWarning.h"
#include "ComboMenuModel.h"        // GameMenu (resolved export fn-ptrs)
#include "ComboWidgetStyle.h"      // combo-owned replication of OOT UIWidgets menu styling
#include "ComboAudioBridge.h"      // Shared-tab audio -> MM mirror
#include "ComboResolutionEditor.h" // combo-owned Advanced Resolution editor (intercepts broken widgets)
#include "ComboMenuLayout.h"
#include "ComboMenuLabels.h"
#include "ComboResolve.h"
#include "rando/GenerationRules.h"

#include <libultraship/libultraship.h>         // ImGui-adjacent + CVar bridge (CVarGet/Set*) + color.h
#include <ship/window/gui/IconsFontAwesome4.h> // ICON_FA_* for the window-toggle button (matches SoH)
#include <imgui.h>
#include <cstring>
#include <string>
#include <unordered_set>

namespace ComboRando {

namespace {
// Human-readable backend names — mirror soh/.../MenuTypes.h windowBackendsMap / audioBackendsMap
// (those live per-DLL and aren't linkable from comboui). Window ids are Fast::WindowBackend values.
const char* WindowBackendName(int32_t id) {
    switch (id) {
        case 1:
            return "DirectX"; // FAST3D_DXGI_DX11
        case 2:
            return "OpenGL"; // FAST3D_SDL_OPENGL
        case 3:
            return "Metal"; // FAST3D_SDL_METAL
        default:
            return "Unknown";
    }
}
const char* AudioBackendName(Ship::AudioBackend b) {
    switch (b) {
        case Ship::AudioBackend::WASAPI:
            return "Windows Audio Session API";
        case Ship::AudioBackend::SDL:
            return "SDL";
        case Ship::AudioBackend::COREAUDIO:
            return "Core Audio";
        case Ship::AudioBackend::NUL:
            return "Null";
        default:
            return "Unknown";
    }
}

// CW_VIDEO_BACKEND: render-API selector. Mirrors SoH's WIDGET_VIDEO_BACKEND — writes the config
// and saves; the change takes effect on relaunch (hence the native "Needs reload" label).
void RenderVideoBackend(const CwWidget& w, const ImVec4& themeColor) {
    auto ctx = Ship::Context::GetRawInstance();
    if (!ctx || !ctx->GetWindow() || !ctx->GetConfig()) {
        ImGui::TextDisabled("%s", (w.name && w.name[0]) ? w.name : "Renderer API");
        return;
    }
    auto window = ctx->GetWindow();
    auto avail = window->GetAvailableWindowBackends();
    int32_t curId = ctx->GetConfig()->GetInt("Window.Backend.Id", -1);
    if (!window->IsAvailableWindowBackend(curId)) {
        curId = window->GetWindowBackend();
    }
    const char* label = (w.name && w.name[0]) ? w.name : "Renderer API";
    const bool only = !avail || avail->size() <= 1;
    if (only)
        ImGui::BeginDisabled(true);
    // Label above the combo (matches SoH), then a hidden-label combo sized to fit the widest option.
    ImGui::TextUnformatted(label);
    ComboMenu_PushCombobox(themeColor);
    const ImGuiStyle& st = ImGui::GetStyle();
    float longest = ImGui::CalcTextSize(WindowBackendName(curId)).x;
    if (avail) {
        for (int32_t id : *avail) {
            float x = ImGui::CalcTextSize(WindowBackendName(id)).x;
            if (x > longest)
                longest = x;
        }
    }
    ImGui::SetNextItemWidth(longest + ImGui::GetFrameHeight() + st.FramePadding.x * 2.0f);
    if (ImGui::BeginCombo("##RendererAPI", WindowBackendName(curId))) {
        if (avail) {
            for (int32_t id : *avail) {
                const bool sel = (id == curId);
                if (ImGui::Selectable(WindowBackendName(id), sel)) {
                    ctx->GetConfig()->SetInt("Window.Backend.Id", id);
                    ctx->GetConfig()->SetString("Window.Backend.Name", WindowBackendName(id));
                    ctx->GetConfig()->Save();
                }
                if (sel)
                    ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    ComboMenu_PopCombobox();
    if (only)
        ImGui::EndDisabled();
}

// CW_AUDIO_BACKEND: audio-API selector. Mirrors SoH's WIDGET_AUDIO_BACKEND — applies live.
void RenderAudioBackend(const CwWidget& w, const ImVec4& themeColor) {
    auto ctx = Ship::Context::GetRawInstance();
    if (!ctx || !ctx->GetAudio()) {
        ImGui::TextDisabled("%s", (w.name && w.name[0]) ? w.name : "Audio API");
        return;
    }
    auto audio = ctx->GetAudio();
    auto avail = audio->GetAvailableAudioBackends();
    const Ship::AudioBackend cur = audio->GetCurrentAudioBackend();
    // SoH's MenuDrawItem hardcodes "Audio API" (ignores the widget's "(Needs reload)" name, since
    // the audio backend applies live) — match that rather than using w.name.
    const char* label = "Audio API";
    const bool only = !avail || avail->size() <= 1;
    if (only)
        ImGui::BeginDisabled(true);
    // Label above the combo (matches SoH), then a hidden-label combo sized to fit the widest option
    // (so "Windows Audio Session API" isn't clipped) — mirrors SoH's CalcComboWidth.
    ImGui::TextUnformatted(label);
    ComboMenu_PushCombobox(themeColor);
    const ImGuiStyle& st = ImGui::GetStyle();
    float longest = ImGui::CalcTextSize(AudioBackendName(cur)).x;
    if (avail) {
        for (Ship::AudioBackend b : *avail) {
            float x = ImGui::CalcTextSize(AudioBackendName(b)).x;
            if (x > longest)
                longest = x;
        }
    }
    ImGui::SetNextItemWidth(longest + ImGui::GetFrameHeight() + st.FramePadding.x * 2.0f);
    if (ImGui::BeginCombo("##AudioAPI", AudioBackendName(cur))) {
        if (avail) {
            for (Ship::AudioBackend b : *avail) {
                const bool sel = (b == cur);
                if (ImGui::Selectable(AudioBackendName(b), sel)) {
                    audio->SetCurrentAudioBackend(b);
                }
                if (sel)
                    ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    ComboMenu_PopCombobox();
    if (only)
        ImGui::EndDisabled();
}
} // namespace

// Widget kinds whose draw we delegate to the OWNING GAME's real UIWidgets (themed separators, text and
// comboboxes without choices). Checkboxes and sliders are drawn here in one style for both games (menu
// ABI v3 exports their readout); buttons / window-buttons / backend selectors / custom stay
// comboui-owned: they either look correct already or can touch live subsystems that aren't safe to
// drive while the game is backgrounded.
static bool DelegatesToGame(CwKind k) {
    switch (k) {
        case CW_COMBOBOX:
        // Button combinations use comboui's shared named-button control below.
        // CW_INPUT_TEXT (OOT) and CW_COLOR (MM) are intentionally NOT delegated: the respective
        // game's MenuDrawItem has no case for them, so they'd draw nothing. Left on the raw path.
        case CW_SEPARATOR:
        case CW_SEPARATOR_TEXT:
        case CW_TEXT:
            return true;
        default:
            return false;
    }
}

// CVar widgets that only work drawn by their own game. Empty until a phone check shows one: add the CVar
// with the reason.
static bool DrawnByGame(const CwWidget& w) {
    static const std::unordered_set<std::string> cvars = {};
    return w.cvar && cvars.count(w.cvar) != 0;
}

// A slider's value readout: the label table's (the stored value as is), else the game's own format
// (times 100 for a percentage), else a printf token in the native name, else a plain number.
struct SliderReadout {
    std::string format;
    bool percentage;
};
static bool ReadoutFits(const std::string& format, bool isFloat) {
    for (size_t at = format.find('%'); at != std::string::npos; at = format.find('%', at + 2)) {
        if (at + 1 < format.size() && format[at + 1] == '%')
            continue;
        const auto conversion = format.find_first_of("diufFeEgG", at + 1);
        if (conversion == std::string::npos)
            return false;
        return std::strchr(isFloat ? "fFeEgG" : "diu", format[conversion]) != nullptr;
    }
    return false;
}
static SliderReadout ReadoutOf(const CwWidget& w, const MenuLabel* relabel, bool isFloat) {
    if (relabel && relabel->format)
        return { relabel->format, false };
    const std::string exported = w.format ? w.format : "";
    if (exported != "%d" && exported != "%f" && ReadoutFits(exported, isFloat))
        return { exported, w.isPercentage != 0 };
    if (const char* token = w.name ? std::strchr(w.name, '%') : nullptr) {
        std::string format = (token > w.name && token[-1] == '+') ? std::string("+") + token : token;
        if (ReadoutFits(format, isFloat))
            return { format, w.isPercentage != 0 };
    }
    if (w.isPercentage)
        return { "%.0f%%", true };
    return { isFloat ? "%.2f" : "%d", false };
}

static void ApplyTrickGovernedRuntimeChange(const char* cvar) {
    const auto* rule = FindGameplayRule(cvar);
    if (!rule || !rule->trickGoverned || std::strcmp(cvar, rule->name))
        return;
    SetEnhancementRule(
        *rule, CVarGetInteger(cvar, rule->fallback),
        [](const char* key, int fallback) { return CVarGetInteger(key, fallback); },
        [](const char* key, int value) {
            CVarSetInteger(key, value);
            static auto write =
                reinterpret_cast<void (*)(const char*, int32_t)>(Combo_ResolveSym("soh", "SOH_WriteGameplayRule"));
            if (write)
                write(key, value);
        });
    static auto refresh = reinterpret_cast<void (*)()>(Combo_ResolveSym("soh", "SOH_RefreshComboRules"));
    if (refresh)
        refresh();
    Ship::Context::GetRawInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
}

static bool IsMmSharedReticle(const CwWidget& w, const GameMenu& game) {
    return w.cvar && std::strcmp(w.cvar, "gEnhancements.HookshotableReticle") == 0 && game.menu &&
           game.menu == ComboMenuModel::Get().Mm().menu;
}

static void RefreshSharedReticlePeer(const CwWidget& w, const GameMenu& game) {
    if (!IsMmSharedReticle(w, game))
        return;
    const auto& oot = ComboMenuModel::Get().Oot();
    if (oot.applyCVarChange)
        oot.applyCVarChange(w.cvar);
}

void RenderWidget(const CwWidget& w, const GameMenu& game, int widthBudget) {
    // A gameplay rule or both-games checkbox placed inside its heading.
    if (w.index <= -1000 && DrawMenuPseudoRow(w, false))
        return;
    // Every native rendering path must defer these paired controls to the shared pages.
    if (IsRetiredNativeRuleControl(w.cvar))
        return;
    // Unique id namespace per widget so two widgets sharing a display label don't collide.
    ImGui::PushID(w.index);
    // The game owns the preFunc predicate and re-evaluates its whole disable map per call, so ask
    // once per frame: 2 = hidden by the owning menu, 1 = disabled with `reason`.
    const char* reason = "";
    const int state = w.hasPreFunc && game.evalDisabled ? game.evalDisabled(w.index, &reason) : 0;
    if (state == 2) {
        ImGui::PopID();
        return;
    } // hidden by the owning menu

    // ComboShip: SoH's Advanced Resolution editor doesn't survive the flat C-ABI (dynamic names,
    // ValuePointer combo, custom-draw, per-page MenuUpdate). Intercept those widgets and render a
    // combo-owned replacement over the effective gSettings.AdvancedResolution.* CVars instead.
    if (TryRenderResolutionWidget(w)) {
        ImGui::PopID();
        return;
    }

    // ComboShip: draw declarative/CVar kinds through the owning game's real MenuDrawItem/UIWidgets.
    // The game applies the change (its own callback + ShipInit) internally, so we must NOT re-invoke
    // those here — only the cross-game audio mirror stays comboui's concern. Falls through to the
    // raw path below when the export is absent (older DLL).
    const bool sharedCombo = w.kind == CW_COMBOBOX && w.cvar && *w.cvar && w.choiceCount > 0;
    if (w.index >= 0 && game.drawWidget && (DelegatesToGame(w.kind) || DrawnByGame(w)) && !sharedCombo) {
        // Enforce the widget's preFunc disable at the comboui level (wrap in BeginDisabled) — same as
        // the raw path below. The game's MenuDrawItem also applies it internally, but wrapping here is
        // what reliably makes a preFunc-gated control non-interactive (e.g. "Cull Glitch Useful
        // Actors", gated on "Widescreen Actor Culling").
        const bool dis = state != 0;
        if (dis) {
            ImGui::BeginDisabled(true);
        }
        const bool changed = game.drawWidget(w.index, widthBudget) != 0;
        if (dis) {
            ImGui::EndDisabled();
        }
        if (changed && w.cvar && w.cvar[0]) {
            // Native draw already refreshed its owner; notify only the shared reticle peer.
            RefreshSharedReticlePeer(w, game);
            ApplyTrickGovernedRuntimeChange(w.cvar);
            ComboAudio::MirrorIfVolumeCVar(w.cvar);
            WarnAboutRuleChange(w.cvar);
        }
        // MM's checkbox already shows this help on hover, keyboard focus and touch hold.
        // Appending Combo's tooltip would draw the same sentence twice.
        const bool nativeReticleTooltip = w.kind == CW_CHECKBOX && IsMmSharedReticle(w, game);
        if (!nativeReticleTooltip && (ImGui::IsItemFocused() || ImGui::IsItemActive()))
            MenuTooltip(w.tooltip);
        ImGui::PopID();
        return;
    }

    // Same-line flag from the model. Delegated widgets get this inside the game's MenuDrawItem;
    // raw-path widgets (buttons/backends) need it applied here (mirrors MenuDrawItem's sameLine).
    if (w.sameLine) {
        ImGui::SameLine();
    }

    // 1) preFunc disable state, evaluated above.
    const bool disabled = state != 0;
    if (disabled) {
        ImGui::BeginDisabled(true);
    }

    // 2) render by kind. `changed` => the backing CVar (or button) changed this frame.
    bool changed = false;
    const bool oot = game.menu == ComboMenuModel::Get().Oot().menu;
    const auto* relabel = FindMenuLabel(oot, w.cvar);
    const char* label = relabel && relabel->label ? relabel->label : (w.name && w.name[0]) ? w.name : "###unnamed";

    // Menu theme accent color (read each frame; clamped). Used to theme each widget kind to
    // match the OOT-rendered Shared tab. See ComboWidgetStyle.{h,cpp}.
    const ImVec4 themeColor = ComboMenu_ThemeColor();

    switch (w.kind) {
        case CW_SEPARATOR:
            ImGui::Separator();
            break;
        case CW_SEPARATOR_TEXT:
            ImGui::SeparatorText(w.name ? w.name : "");
            break;
        case CW_TEXT:
            ImGui::TextWrapped("%s", w.name ? w.name : "");
            break;
        case CW_CHECKBOX: {
            bool v = CVarGetInteger(w.cvar, w.bDefault) != 0;
            if (ComboMenu_CheckboxRow(MenuRowLabel(oot, w.cvar, w.name).c_str(), &v)) {
                CVarSetInteger(w.cvar, v ? 1 : 0);
                changed = true;
            }
            break;
        }
        case CW_SLIDER_INT: {
            // The label above and the value with its unit inside the bar (the native name may hold a
            // printf readout, "Crawl Speed %dx": it is split into the two).
            int v = CVarGetInteger(w.cvar, w.iDefault);
            const auto readout = ReadoutOf(w, relabel, false);
            if (ComboMenu_SliderRowInt(MenuRowLabel(oot, w.cvar, w.name).c_str(), &v, w.iMin, w.iMax, w.iStep,
                                       w.iDefault, readout.format.c_str())) {
                CVarSetInteger(w.cvar, v);
                changed = true;
            }
            break;
        }
        case CW_SLIDER_FLOAT: {
            float v = CVarGetFloat(w.cvar, w.fDefault);
            const auto readout = ReadoutOf(w, relabel, true);
            if (ComboMenu_SliderRowFloat(MenuRowLabel(oot, w.cvar, w.name).c_str(), &v, w.fMin, w.fMax, w.fStep,
                                         w.fDefault, readout.format.c_str(), readout.percentage)) {
                CVarSetFloat(w.cvar, v);
                changed = true;
            }
            break;
        }
        case CW_COMBOBOX: {
            ImGui::BeginGroup();
            std::string displayLabel = MenuRowLabel(oot, w.cvar, w.name);
            if (const auto suffix = displayLabel.find("##"); suffix != std::string::npos)
                displayLabel.resize(suffix);
            ImGui::TextWrapped("%s", displayLabel.c_str());
            int v = CVarGetInteger(w.cvar, w.iDefault);
            const char* curLabel = "";
            for (int i = 0; i < w.choiceCount; ++i) {
                if (w.choices[i].value == v) {
                    curLabel = w.choices[i].label;
                    break;
                }
            }
            ComboMenu_PushCombobox(themeColor);
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::BeginCombo("##Choice", curLabel)) {
                for (int i = 0; i < w.choiceCount; ++i) {
                    const CwChoice& c = w.choices[i];
                    bool sel = (c.value == v);
                    if (ImGui::Selectable(c.label, sel)) {
                        CVarSetInteger(w.cvar, c.value);
                        changed = true;
                    }
                    if (sel) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            ComboMenu_PopCombobox();
            ImGui::EndGroup();
            break;
        }
        case CW_BTN_SELECTOR: {
            // These values are controller button masks, not ordinary numbers. Offer
            // the same named buttons in both engines instead of a numeric keyboard.
            int v = CVarGetInteger(w.cvar, w.iDefault);
            ImGui::TextWrapped("%s", label);
            struct Button {
                const char* name;
                int mask;
            };
            static const Button buttons[] = {
                { "A", 0x8000 },      { "B", 0x4000 },       { "Z", 0x2000 },          { "Start", 0x1000 },
                { "L", 0x0020 },      { "R", 0x0010 },       { "C up", 0x0008 },       { "C down", 0x0004 },
                { "C left", 0x0002 }, { "C right", 0x0001 }, { "D up", 0x0800 },       { "D down", 0x0400 },
                { "D left", 0x0200 }, { "D right", 0x0100 }, { "Modifier 1", 0x0040 }, { "Modifier 2", 0x0080 },
            };
            ComboMenu_PushCheckbox(themeColor);
            for (int i = 0; i < IM_ARRAYSIZE(buttons); ++i) {
                bool enabled = (v & buttons[i].mask) != 0;
                if (ImGui::Checkbox(buttons[i].name, &enabled)) {
                    v = enabled ? v | buttons[i].mask : v & ~buttons[i].mask;
                    changed = true;
                }
                if (buttons[i].mask == 0x0040 || buttons[i].mask == 0x0080)
                    MenuTooltip("Modifier 1 and 2 are extra virtual buttons. Assign a physical button to them in "
                                "Settings > Controls (controller editor). They are not automatically ZL or ZR. Select "
                                "several buttons here to require them together.");
                if ((i + 1) % 4 != 0 && i + 1 < IM_ARRAYSIZE(buttons) &&
                    ImGui::GetContentRegionAvail().x >
                        ImGui::GetFrameHeight() + ImGui::CalcTextSize(buttons[i + 1].name).x + 30)
                    ImGui::SameLine();
            }
            ComboMenu_PopCheckbox();
            ComboMenu_PushButton(themeColor);
            if (ImGui::Button("Reset shortcut")) {
                v = w.iDefault;
                changed = true;
            }
            ComboMenu_PopButton();
            if (changed) {
                CVarSetInteger(w.cvar, v);
            }
            break;
        }
        case CW_INPUT_TEXT: {
            // CVarGetString/SetString are available in the bridge, so this round-trips with
            // the game. Seed a local buffer from the CVar; write back on edit. (MM never emits
            // this kind; OOT has a few.)
            char buf[256];
            const char* cur = CVarGetString(w.cvar, "");
            std::strncpy(buf, cur ? cur : "", sizeof(buf) - 1);
            buf[sizeof(buf) - 1] = '\0';
            if (ImGui::InputText(label, buf, sizeof(buf))) {
                CVarSetString(w.cvar, buf);
                changed = true;
            }
            break;
        }
        case CW_COLOR: {
            // Use the bridge color API so values round-trip with the game's CVar storage.
            if (w.useAlpha) {
                Color_RGBA8 def = { 255, 255, 255, 255 };
                Color_RGBA8 c = CVarGetColor(w.cvar, def);
                float col[4] = { c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f };
                if (ImGui::ColorEdit4(label, col, ImGuiColorEditFlags_NoInputs)) {
                    c.r = (uint8_t)(col[0] * 255.0f + 0.5f);
                    c.g = (uint8_t)(col[1] * 255.0f + 0.5f);
                    c.b = (uint8_t)(col[2] * 255.0f + 0.5f);
                    c.a = (uint8_t)(col[3] * 255.0f + 0.5f);
                    CVarSetColor(w.cvar, c);
                    changed = true;
                }
            } else {
                Color_RGB8 def = { 255, 255, 255 };
                Color_RGB8 c = CVarGetColor24(w.cvar, def);
                float col[3] = { c.r / 255.0f, c.g / 255.0f, c.b / 255.0f };
                if (ImGui::ColorEdit3(label, col, ImGuiColorEditFlags_NoInputs)) {
                    c.r = (uint8_t)(col[0] * 255.0f + 0.5f);
                    c.g = (uint8_t)(col[1] * 255.0f + 0.5f);
                    c.b = (uint8_t)(col[2] * 255.0f + 0.5f);
                    CVarSetColor24(w.cvar, c);
                    changed = true;
                }
            }
            break;
        }
        case CW_BUTTON: {
            ComboMenu_PushButton(themeColor);
            // These span the full column width (matches SoH); other buttons keep their natural (text)
            // width so e.g. Generate/Randomize stay side-by-side.
            const bool fullWidth = w.name && (std::strcmp(w.name, "Open App Files Folder") == 0 ||
                                              std::strcmp(w.name, "Toggle Fullscreen") == 0 ||
                                              std::strcmp(w.name, "Test Notification") == 0);
            if (ImGui::Button(label, fullWidth ? ImVec2(-FLT_MIN, 0.0f) : ImVec2(0.0f, 0.0f))) {
                changed = true; // button => fire callback (below)
            }
            ComboMenu_PopButton();
            break;
        }
        case CW_WINDOW_BUTTON: {
            // Button with an open/close icon (matches SoH's WindowButton), NOT a checkbox. Toggle via
            // the window OBJECT: GuiWindow gates Draw() on mIsVisible and only reads the CVar at
            // construction, so a CVar-only toggle never shows/hides at runtime. ToggleVisibility() is a
            // libultraship method (no game-DLL ImGui context needed).
            if (w.cvar && w.cvar[0]) {
                const bool open = CVarGetInteger(w.cvar, 0) != 0;
                std::string txt = std::string(open ? ICON_FA_WINDOW_CLOSE : ICON_FA_EXTERNAL_LINK_SQUARE) + " " +
                                  ((relabel && relabel->label) ? relabel->label
                                   : (w.name && w.name[0])     ? w.name
                                                               : "");
                ComboMenu_PushButton(themeColor);
                if (ImGui::Button(txt.c_str())) {
                    std::shared_ptr<Ship::GuiWindow> win;
                    if (w.windowName && w.windowName[0]) {
                        auto ctx = Ship::Context::GetRawInstance();
                        if (ctx && ctx->GetWindow() && ctx->GetWindow()->GetGui())
                            win = ctx->GetWindow()->GetGui()->GetGuiWindow(w.windowName);
                    }
                    if (win) {
                        win->ToggleVisibility();
                    } else {
                        CVarSetInteger(w.cvar, open ? 0 : 1); // fallback: no window object to drive
                    }
                    changed = true;
                }
                ComboMenu_PopButton();
            } else {
                ImGui::TextDisabled("%s", w.name ? w.name : "");
            }
            break;
        }
        case CW_AUDIO_BACKEND:
            RenderAudioBackend(w, themeColor);
            break;
        case CW_VIDEO_BACKEND:
            RenderVideoBackend(w, themeColor);
            break;
        case CW_CUSTOM:
            // The game draws this entirely under its own context/RM, addressed by index.
            if (game.drawCustom) {
                game.drawCustom(w.index);
            }
            break;
        default:
            ImGui::TextDisabled("%s", w.name ? w.name : "");
            break;
    }

    // 3) tooltip on hover (applies to the widget item just drawn).
    MenuTooltip(disabled && reason && *reason ? reason : MenuRowTooltip(oot, w.cvar, w.tooltip).c_str());

    // 4) post-change callback, routed by index into the owning game.
    if (changed && w.hasCallback && game.invokeCallback) {
        game.invokeCallback(w.index);
    }

    // 5) re-apply the changed CVar live: re-run the owning game's ShipInit func(s) registered for it,
    // mirroring what the games' native UIWidgets do. Without this, enhancements/settings toggled in
    // the combo menu only take effect on the next ShipInit::InitAll (game boot / new save).
    if (changed && w.cvar && w.cvar[0] && game.applyCVarChange) {
        game.applyCVarChange(w.cvar);
    }
    // This visual toggle is shared, but OoT installs its conditional hook on ShipInit.
    // A change through an MM widget (including search) must refresh that owner too.
    if (changed)
        RefreshSharedReticlePeer(w, game);

    // 6) ComboShip: the Shared tab's audio sliders are OOT's (gSettings.Volume.*); mirror them into
    // MM's gSettings.Audio.* so a single Shared Audio section drives both games (see ComboAudioBridge).
    if (changed && w.cvar && w.cvar[0]) {
        ApplyTrickGovernedRuntimeChange(w.cvar);
        ComboAudio::MirrorIfVolumeCVar(w.cvar);
        WarnAboutRuleChange(w.cvar);
        // The games' own widgets save on change; a row drawn here must too, or it reverts on restart.
        if (auto ctx = Ship::Context::GetRawInstance(); ctx && ctx->GetWindow() && ctx->GetWindow()->GetGui())
            ctx->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
    }

    if (disabled) {
        ImGui::EndDisabled();
    }
    ImGui::PopID();
}

void RenderSidebarWidgets(const CwSidebar& side, const GameMenu& game) {
    const uint32_t desired = std::min<uint32_t>(2, side.columnCount > 0 ? side.columnCount : 1);
    const uint32_t cols = ImGui::GetContentRegionAvail().x < ImGui::GetFontSize() * 20.0f * desired ? 1 : desired;
    const int widthBudget = 90 / (int)cols; // label WrappedText budget, matching the native menu
    if (cols <= 1) {
        for (int i = 0; i < side.widgetCount; ++i) {
            RenderWidget(side.widgets[i], game, widthBudget);
        }
        return;
    }
    // One scrolling body matches the MM cards and lets preceding shared controls
    // grow naturally. Separate fill-height children can leave native options in
    // a tiny viewport when the shared controls take most of the available height.
    if (!ImGui::BeginTable("##NativeOptions", cols,
                           ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoSavedSettings))
        return;
    ImGui::TableNextRow();
    for (uint32_t c = 0; c < cols; ++c) {
        ImGui::TableSetColumnIndex(c);
        ImGui::PushID((int)c);
        ImGui::BeginGroup();
        for (int i = 0; i < side.widgetCount; ++i) {
            int wc = side.widgets[i].column;
            if (wc < 0) {
                wc = 0;
            } else if ((uint32_t)wc >= cols) {
                wc = (int)cols - 1; // clamp stray indices into the last column
            }
            if ((uint32_t)wc == c) {
                RenderWidget(side.widgets[i], game, widthBudget);
            }
        }
        ImGui::EndGroup();
        ImGui::PopID();
    }
    ImGui::EndTable();
}

} // namespace ComboRando
