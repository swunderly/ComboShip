// combo/gui/ComboHintTracker.cpp — see ComboHintTracker.h
#include "ComboHintTracker.h"
#include "ComboTrackerControls.h"
#include "ComboExport.h"
#include "ComboForeground.h"  // foreground game for the OnlyPaused gate
#include "ComboTrackerSwap.h" // ForegroundPaused (shared with the icon trackers)
#include "ComboWidgetStyle.h"
#include "ComboMenuLayout.h"
#include <imgui.h>
#include <libultraship/libultraship.h> // CVar bridge
#include <ship/Context.h>
#include <ship/window/gui/IconsFontAwesome4.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace ComboRando {

namespace {

constexpr const char* kCvarEnabled = "gCombo.HintTracker.Enabled";
constexpr const char* kCvarShowUnrevealed = "gCombo.HintTracker.ShowUnrevealed";
constexpr const char* kCvarShowJunk = "gCombo.HintTracker.ShowJunk";
constexpr const char* kCvarHideRead = "gCombo.HintTracker.HideRead";

// Launcher push side. The launcher owns/reuses its buffers and can call from a game DLL's thread,
// so the strings are COPIED under this mutex; parsing happens on the draw thread via sDirty.
std::mutex sPushMutex;
std::string sPushedHints;
std::string sPushedRead;
int sPushedSlot = -1;
bool sDirty = false;

struct Entry {
    std::string key;   // reveal key (comboKey / pool index / MM item name)
    std::string label; // display label
    std::string text;  // sanitized hint text
    bool read = false;
};
struct Group {
    std::string title;
    std::vector<Entry> entries;
    bool junk = false; // gated behind ShowJunk
};

int sSlot = -1;
std::vector<Group> sGroups;
char sSearchBuf[128] = {};

// Strip OOT CustomMessage control codes (MF_ENCODE output) down to plain text: %<color> and
// $<icon> pairs, box/line breaks, and the player-name token. Mirrors CustomMessage::Clean.
std::string Sanitize(std::string s) {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        const char c = s[i];
        if ((c == '%' || c == '$') && i + 1 < s.size()) {
            ++i; // drop the code letter too
            continue;
        }
        if (c == '&' || c == '^') {
            out.push_back('\n');
            continue;
        }
        if (c == '@') {
            out += "Link";
            continue;
        }
        if (c == '#') {
            continue;
        }
        if (static_cast<unsigned char>(c) < 0x20 && c != '\n') {
            continue;
        }
        out.push_back(c);
    }
    // Collapse the blank line a "&^" pair leaves behind.
    while (true) {
        size_t p = out.find("\n\n\n");
        if (p == std::string::npos) {
            break;
        }
        out.erase(p, 1);
    }
    return out;
}

// "__ALWAYS__Song from Impa0" / "__TRIAL__Forest Trial1" -> the name without the copy digits.
std::string StripCopySuffix(std::string s) {
    while (!s.empty() && std::isdigit(static_cast<unsigned char>(s.back()))) {
        s.pop_back();
    }
    return s;
}

bool HasPrefix(const std::string& s, const char* p) {
    return s.rfind(p, 0) == 0;
}

// nlohmann's value()/[] throw on a type mismatch, so every node is fetched defensively.
const nlohmann::json* Node(const nlohmann::json& j, const char* key) {
    if (!j.is_object()) {
        return nullptr;
    }
    auto it = j.find(key);
    return it == j.end() ? nullptr : &*it;
}

// Visit each element of an array node under its own guard: a malformed entry is skipped, not fatal.
template <typename Fn> void ForEachElement(const nlohmann::json* arr, Fn&& fn) {
    if (!arr || !arr->is_array()) {
        return;
    }
    for (const auto& e : *arr) {
        try {
            fn(e);
        } catch (...) { /* skip this entry */
        }
    }
}

// `ordinal` is the entry's 1-based position in its own group. The sentinel keys carry the generator's
// producedHints/producedJunk counters, which are shared across categories — numbering from those would
// start the stone list at #(trials + always + 1).
std::string LabelForOotKey(const std::string& key, int ordinal) {
    if (key == "__GANONDORF__") {
        return "Ganondorf";
    }
    if (key == "__ALTAR_CHILD__") {
        return "Child Altar";
    }
    if (key == "__ALTAR_ADULT__") {
        return "Adult Altar";
    }
    if (HasPrefix(key, "__STATIC__")) {
        // "__STATIC__<RandomizerHint>" (CrossHints.h kStaticHintPrefix): the NPC that speaks it.
        static const std::unordered_map<std::string, const char*> kNpcLabels = {
            { "RH_SHEIK_HINT", "Sheik (Ganon's Castle)" },
            { "RH_FOREST_BOSS_KEY_HINT", "Forest Temple Boss Door" },
            { "RH_FIRE_BOSS_KEY_HINT", "Fire Temple Boss Door" },
            { "RH_WATER_BOSS_KEY_HINT", "Water Temple Boss Door" },
            { "RH_SPIRIT_BOSS_KEY_HINT", "Spirit Temple Boss Door" },
            { "RH_SHADOW_BOSS_KEY_HINT", "Shadow Temple Boss Door" },
            { "RH_GANONS_BOSS_KEY_HINT", "Ganon's Tower Boss Door" },
            { "RH_DAMPES_DIARY", "Dampe's Diary" },
            { "RH_GREG_RUPEE", "Treasure Chest Shop (Greg)" },
            { "RH_SARIA_HINT", "Saria" },
            { "RH_MIDO_HINT", "Mido" },
            { "RH_FISHING_POLE", "Fishing Pond Owner" },
        };
        const std::string rh = key.substr(10);
        auto it = kNpcLabels.find(rh);
        return it != kNpcLabels.end() ? it->second : rh;
    }
    if (HasPrefix(key, "__STONE__")) {
        return "Gossip Stone #" + std::to_string(ordinal);
    }
    if (HasPrefix(key, "__JUNK__")) {
        return "Junk Hint #" + std::to_string(ordinal);
    }
    if (HasPrefix(key, "__TRIAL__")) {
        return StripCopySuffix(key.substr(9));
    }
    if (HasPrefix(key, "__ALWAYS__")) {
        return StripCopySuffix(key.substr(10));
    }
    return key; // a real gossip-stone check name
}

// Display group of an OOT hint, by its generator "type". Indices into the group list Rebuild lays
// out; keep the two in sync.
enum OotGroup {
    kGrpStones = 0,
    kGrpAltarChild,
    kGrpAltarAdult,
    kGrpGanondorf,
    kGrpNpc,
    kGrpTrials,
    kGrpAlways,
    kGrpJunk,
    kGrpCount
};

int OotGroupForType(const std::string& type) {
    if (type == "altarChild") {
        return kGrpAltarChild;
    }
    if (type == "altarAdult") {
        return kGrpAltarAdult;
    }
    if (type == "ganondorf") {
        return kGrpGanondorf;
    }
    if (type == "static") {
        return kGrpNpc;
    }
    if (type == "trial") {
        return kGrpTrials;
    }
    if (type == "always") {
        return kGrpAlways;
    }
    if (type == "junk") {
        return kGrpJunk;
    }
    return kGrpStones; // WotH / Foolish / Song / Overworld / Dungeon / NamedItem / Random
}

// Rebuild the display model from the pushed strings. Corrupt input -> empty (never throws).
void Rebuild(const std::string& hintsJson, const std::string& readJson) {
    sGroups.clear();
    sGroups.resize(kGrpCount);
    sGroups[kGrpStones].title = "Gossip Stones";
    sGroups[kGrpAltarChild].title = "Temple of Time Altar (Child)";
    sGroups[kGrpAltarAdult].title = "Temple of Time Altar (Adult)";
    sGroups[kGrpGanondorf].title = "Ganondorf";
    sGroups[kGrpNpc].title = "NPC Item Hints";
    sGroups[kGrpTrials].title = "Trials";
    sGroups[kGrpAlways].title = "Always Hints";
    sGroups[kGrpJunk].title = "Junk Hints";
    sGroups[kGrpJunk].junk = true;

    std::set<std::string> readOot, readNpc;
    std::set<int> readPool;
    std::vector<std::pair<std::string, std::string>> revealedNative;
    nlohmann::json rs = nlohmann::json::object();
    try {
        if (!readJson.empty()) {
            rs = nlohmann::json::parse(readJson);
        }
    } catch (...) { /* corrupt -> no read state */
    }
    // Every element is read under its own guard: one bad entry must cost only that entry.
    ForEachElement(Node(rs, "oot"), [&](const nlohmann::json& k) { readOot.insert(k.get<std::string>()); });
    ForEachElement(Node(rs, "mmPool"), [&](const nlohmann::json& i) { readPool.insert(i.get<int>()); });
    ForEachElement(Node(rs, "mmNpc"), [&](const nlohmann::json& k) { readNpc.insert(k.get<std::string>()); });
    ForEachElement(Node(rs, "mmNative"), [&](const nlohmann::json& e) {
        revealedNative.emplace_back(e.value("check", std::string()), e.value("text", std::string()));
    });

    nlohmann::json h = nlohmann::json::object();
    if (hintsJson.empty()) {
        sGroups.clear(); // nothing pushed yet -> "no seed loaded"
        return;
    }
    try {
        h = nlohmann::json::parse(hintsJson);
    } catch (...) {
        sGroups.clear(); // unparseable -> "no seed loaded"
        return;
    }

    ForEachElement(Node(h, "oot"), [&](const nlohmann::json& e) {
        Entry en;
        en.key = e.value("checkName", std::string());
        if (en.key.empty()) {
            return;
        }
        const nlohmann::json* msgs = Node(e, "messages");
        if (msgs && msgs->is_array() && !msgs->empty()) {
            // CrossHints::Generate pushes a multi-message hint's most complete variant last.
            en.text = Sanitize(msgs->back().value("en", std::string()));
        }
        en.read = readOot.count(en.key) != 0;
        Group& grp = sGroups[OotGroupForType(e.value("type", std::string()))];
        en.label = LabelForOotKey(en.key, static_cast<int>(grp.entries.size()) + 1);
        const std::string target = e.value("pathTarget", std::string());
        if (en.read && !target.empty())
            en.label = "Path to " + target;
        grp.entries.push_back(std::move(en));
    });

    const nlohmann::json* mm = Node(h, "mm");
    Group pool;
    pool.title = "Majora's Mask - Cross-Game Stone Pool";
    ForEachElement(mm ? Node(*mm, "gossipPool") : nullptr, [&](const nlohmann::json& g) {
        const int idx = static_cast<int>(pool.entries.size());
        Entry en;
        en.key = std::to_string(idx);
        en.label = "Pool Hint #" + std::to_string(idx + 1);
        en.text = g.value("text", std::string());
        en.read = readPool.count(idx) != 0;
        const std::string target = g.value("pathTarget", std::string());
        if (en.read && !target.empty())
            en.label = "Path to " + target;
        pool.entries.push_back(std::move(en));
    });
    sGroups.push_back(std::move(pool));

    // MM's own stone hints are composed at talk time from live save state, so there is no upfront
    // list — only what the game has already shown.
    Group native;
    native.title = "Majora's Mask - Revealed Stone Hints";
    for (auto& [check, text] : revealedNative) {
        Entry en;
        en.key = check;
        en.label = check.empty() ? "Gossip Stone" : check;
        en.text = text;
        en.read = true;
        native.entries.push_back(std::move(en));
    }
    sGroups.push_back(std::move(native));

    Group npc;
    npc.title = "Majora's Mask - Item Hints";
    const nlohmann::json* itemLocs = mm ? Node(*mm, "itemLocations") : nullptr;
    if (itemLocs && itemLocs->is_object()) {
        for (auto& [item, text] : itemLocs->items()) {
            if (!text.is_string()) {
                continue;
            }
            Entry en;
            en.key = item;
            en.label = item;
            en.text = text.get<std::string>();
            en.read = readNpc.count(item) != 0;
            npc.entries.push_back(std::move(en));
        }
    }
    sGroups.push_back(std::move(npc));
    // A slot with no baked hints pushes "{}" — read as no seed, not as an all-filtered-out list.
    if (std::all_of(sGroups.begin(), sGroups.end(), [](const Group& g) { return g.entries.empty(); })) {
        sGroups.clear();
    }
}

// Take the pushed strings (draw thread only) and re-parse if they changed.
void PollPush() {
    std::string hints, read;
    int slot;
    {
        std::lock_guard<std::mutex> lock(sPushMutex);
        if (!sDirty) {
            return;
        }
        sDirty = false;
        hints = sPushedHints;
        read = sPushedRead;
        slot = sPushedSlot;
    }
    sSlot = slot;
    Rebuild(hints, read);
}

std::string ToLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

std::shared_ptr<ComboHintTrackerWindow> sWindow;

} // namespace

void ComboHintTrackerWindow::DrawElement() {
    PollPush();

    const ImVec4 theme = ComboMenu_ThemeColor();
    const bool showUnrevealed = CVarGetInteger(kCvarShowUnrevealed, 0) != 0;
    const bool showJunk = CVarGetInteger(kCvarShowJunk, 0) != 0;
    const bool hideRead = CVarGetInteger(kCvarHideRead, 0) != 0;

    if (sSlot < 0 || sGroups.empty()) {
        ImGui::TextDisabled("No seed loaded.");
        return;
    }

    if (CVarGetInteger("gCombo.HintTracker.ShowTools", 0)) {
        ComboMenu_PushInput(theme);
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputTextWithHint("##hintsearch", "Search hints...", sSearchBuf, sizeof(sSearchBuf));
        ComboMenu_PopInput();
    } else
        sSearchBuf[0] = 0;
    const std::string query = ToLower(sSearchBuf);

    int total = 0, read = 0;
    for (const Group& g : sGroups) {
        if (g.junk && !showJunk) {
            continue;
        }
        for (const Entry& e : g.entries) {
            ++total;
            read += e.read ? 1 : 0;
        }
    }
    ImGui::TextDisabled("Revealed %d / %d", read, total);
    ImGui::Separator();

    const ImVec4 kHiddenColor(0.5f, 0.5f, 0.5f, 1.0f);
    static std::string selectedLabel, selectedText;
    static int selectedSlot = -1;
    int shownTotal = 0;
    for (const Group& g : sGroups) {
        if (g.junk && !showJunk) {
            continue;
        }
        // Pre-filter so an empty group (all read + HideRead, or no search match) never shows a header.
        std::vector<const Entry*> shown;
        for (const Entry& e : g.entries) {
            if (hideRead && e.read) {
                continue;
            }
            if (!query.empty()) {
                std::string hay = ToLower(e.label);
                if (e.read || showUnrevealed) {
                    hay += "\n" + ToLower(e.text);
                }
                if (hay.find(query) == std::string::npos) {
                    continue;
                }
            }
            shown.push_back(&e);
        }
        if (shown.empty()) {
            continue;
        }
        shownTotal += static_cast<int>(shown.size());
        // "###<title>" pins the ImGui ID to the title so the live count in the label can't reset the
        // collapse state on every search keystroke or reveal.
        int revealed = 0;
        for (const auto& e : g.entries)
            revealed += e.read;
        ImGui::Separator();
        std::string header =
            g.title + " (" + std::to_string(revealed) + "/" + std::to_string(g.entries.size()) + ")###" + g.title;
        if (!ImGui::CollapsingHeader(header.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
            continue;
        }
        for (const Entry* e : shown) {
            ImGui::PushID(g.title.c_str());
            ImGui::PushID(e->key.c_str());
            ImGui::PushStyleColor(ImGuiCol_Text, e->read ? ImVec4(.55f, .8f, 1, 1) : kHiddenColor);
            ImGui::TextUnformatted(e->read ? ICON_FA_COMMENT : ICON_FA_QUESTION);
            ImGui::SameLine();
            const std::string row = e->label + (e->read ? "" : " (Unrevealed)") + "##hint";
            if (ImGui::Selectable(row.c_str(), false, 0, ImVec2(0, ImGui::GetTextLineHeight() * 1.15f))) {
                selectedLabel = e->label;
                selectedText = e->read || showUnrevealed ? e->text : "This hint has not been revealed yet.";
                selectedSlot = sSlot;
                ImGui::PopID();
                ImGui::PopID();
                ImGui::OpenPopup("Hint details");
                ImGui::PushID(g.title.c_str());
                ImGui::PushID(e->key.c_str());
            }
            ImGui::PopStyleColor();
            ImGui::PopID();
            ImGui::PopID();
        }
    }
    if (shownTotal == 0) {
        ImGui::TextDisabled("Nothing to show with the current filters.");
    }
    const auto display = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowSize(ImVec2(std::min(ImGui::GetFontSize() * 30, display.x * .8f), 0), ImGuiCond_Appearing);
    if (ImGui::BeginPopup("Hint details")) {
        if (selectedSlot != sSlot)
            ImGui::CloseCurrentPopup();
        else {
            ImGui::TextWrapped("%s", selectedLabel.c_str());
            ImGui::Separator();
            ImGui::TextWrapped("%s", selectedText.c_str());
            ComboMenu_PushButton(theme);
            if (ImGui::Button("Close"))
                ImGui::CloseCurrentPopup();
            ComboMenu_PopButton();
        }
        ImGui::EndPopup();
    }
}

void ComboHintTrackerWindow::Draw() {
    auto ctx = Ship::Context::GetRawInstance();
    if (!ctx || !ctx->GetWindow() || !ctx->GetWindow()->GetGui())
        return;
    ImGui::SetCurrentContext(ctx->GetWindow()->GetGui()->GetImGuiContext());
    PollPush();
    if (!ComboTrackerUI::Visible(ComboTrackerUI::Hint))
        return;
    if (ComboTrackerUI::Begin(ComboTrackerUI::Hint))
        DrawElement();
    ComboTrackerUI::End(ComboTrackerUI::Hint);
}

void RegisterHintTrackerWindow() {
    auto ctx = Ship::Context::GetRawInstance();
    if (!ctx || !ctx->GetWindow() || !ctx->GetWindow()->GetGui()) {
        return;
    }
    if (!sWindow) {
        ComboTrackerUI::Migrate(ComboTrackerUI::Hint);
        sWindow = std::make_shared<ComboHintTrackerWindow>("", true, " Combo Hint Tracker");
    }
    ctx->GetWindow()->GetGui()->AddGuiWindow(sWindow);
}

void DrawHintTrackerSharedPanel() {
    using namespace ComboTrackerUI;
    DrawEnabled(Hint);
    if (ImGui::BeginTable("##hint-settings", 2, ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoSavedSettings)) {
        ImGui::TableNextColumn();
        DrawOpenControls(Hint);
        ImGui::TableNextColumn();
        DrawAppearance(Hint);
        ComboMenu_PushCheckbox(ComboMenu_ThemeColor());
        Checkbox("Show search toolbar", "gCombo.HintTracker.ShowTools");
        Checkbox("Hide hints already read", kCvarHideRead);
        MenuTooltip("Read state belongs to the save slot. Regenerating the slot clears it.");
        Checkbox("Show junk hints", kCvarShowJunk);
        Checkbox("Spoil unrevealed hints", kCvarShowUnrevealed);
        MenuTooltip("Off by default: unrevealed hint contents cannot appear in rows or search results. Enable only if "
                    "you want spoilers.");
        ComboMenu_PopCheckbox();
        ImGui::EndTable();
    }
}

} // namespace ComboRando

// ComboShip: the launcher pushes the slot's hints slice + read state here (at slot bind/load and
// after every reveal). Strings are copied — the launcher reuses its buffers — and parsed lazily on
// the draw thread.
extern "C" COMBO_EXPORT void ComboUI_SetHintTrackerData(int slot, const char* hintsJson, const char* readStateJson) {
    try {
        std::lock_guard<std::mutex> lock(ComboRando::sPushMutex);
        ComboRando::sPushedSlot = slot;
        ComboRando::sPushedHints = hintsJson ? hintsJson : "";
        ComboRando::sPushedRead = readStateJson ? readStateJson : "";
        ComboRando::sDirty = true;
    } catch (...) { /* never unwind across the C-ABI boundary */
    }
}
