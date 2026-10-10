#pragma once
#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>
#include <vector>

namespace ComboRando {
enum class MenuSearchKind { Rule, Setting, Technique, Page };

// Search words independently so punctuation, word order and common plurals do
// not force the user to know a native menu label. No CVar names are rewritten.
inline std::string MenuSearchTerm(std::string word) {
    for (const auto* plural : { "songs", "warps", "owls", "statues", "items", "checks", "masks", "buttons", "controls",
                                "keys", "eggs", "tricks", "destinations" }) {
        if (word == plural) {
            word.pop_back();
            break;
        }
    }
    return word;
}
inline std::vector<std::string> MenuSearchWords(const std::string& text) {
    std::vector<std::string> words;
    std::string word;
    auto finish = [&]() {
        if (word.empty())
            return;
        word = MenuSearchTerm(word);
        if (word != "a" && word != "an" && word != "the" && word != "and" && word != "of" && word != "to" &&
            word != "for" && word != "in" && word != "at")
            words.push_back(word);
        word.clear();
    };
    for (unsigned char c : text) {
        if (std::isalnum(c) || c >= 128)
            word += static_cast<char>(std::tolower(c));
        else
            finish();
    }
    finish();
    return words;
}
inline std::string MenuSearchPhrase(const std::vector<std::string>& words) {
    std::string phrase;
    for (const auto& word : words) {
        if (!phrase.empty())
            phrase += ' ';
        phrase += word;
    }
    return phrase;
}
inline int MenuSearchWordScore(const std::string& term, const std::vector<std::string>& words) {
    int score = 0;
    for (const auto& word : words) {
        if (word == term)
            return 35;
        if (term.size() < 3)
            continue; // Short names must be whole words, e.g. MM or CT.
        if (word.compare(0, term.size(), term) == 0)
            score = std::max(score, 22);
        else if (word.find(term) != std::string::npos)
            score = std::max(score, 12);
    }
    return score;
}
inline int MenuSearchScore(const std::string& query, const char* name, const char* keywords) {
    const auto terms = MenuSearchWords(query);
    if (terms.empty())
        return 0;
    const auto title = MenuSearchWords(name ? name : "");
    const auto aliases = MenuSearchWords(keywords ? keywords : "");
    int score = 1;
    bool allInTitle = true;
    for (const auto& term : terms) {
        const int titleScore = MenuSearchWordScore(term, title);
        const int aliasScore = MenuSearchWordScore(term, aliases);
        if (!titleScore && !aliasScore)
            return 0; // every word must match this result
        allInTitle &= titleScore != 0;
        score += titleScore ? titleScore * 3 : aliasScore;
    }
    const auto phrase = MenuSearchPhrase(terms);
    const auto titlePhrase = MenuSearchPhrase(title);
    if (titlePhrase == phrase)
        score += 900;
    else if (titlePhrase.compare(0, phrase.size(), phrase) == 0)
        score += 450;
    else if (allInTitle)
        score += 250;
    else if (MenuSearchPhrase(aliases).find(phrase) != std::string::npos)
        score += 70;
    return score;
}
inline int MenuSearchRank(const std::string& query, const char* name, const char* keywords, MenuSearchKind kind) {
    const auto terms = MenuSearchWords(query);
    if (terms.size() == 1 && (terms[0] == "ct" || terms[0] == "it" || terms[0] == "ht")) {
        const char* tracker = terms[0] == "ct" ? "Check Tracker" : terms[0] == "it" ? "Item Tracker" : "Hint Tracker";
        return kind == MenuSearchKind::Page && name && !std::strcmp(name, tracker) ? 1000 : 0;
    }
    const int score = MenuSearchScore(query, name, keywords);
    if (!score)
        return 0;
    // A concrete setting is more useful than a broad page which merely lists
    // the word among its aliases. Exact titles still win over incidental hits.
    return score + (kind == MenuSearchKind::Rule        ? 300
                    : kind == MenuSearchKind::Setting   ? 180
                    : kind == MenuSearchKind::Technique ? 120
                                                        : 0);
}
inline constexpr const char* kSongTravelRuleKeys[] = { "gCombo.Enhancements.ForeignSongs",
                                                       "gCombo.Enhancements.SongTravel" };
inline bool IsSongTravelRule(const char* name) {
    if (!name)
        return false;
    for (const auto* key : kSongTravelRuleKeys)
        if (!std::strcmp(name, key))
            return true;
    return false;
}
inline const char* GameplayRuleMenuLabel(const char* name, const char* label, bool randomizer) {
    return randomizer && name && !std::strcmp(name, kSongTravelRuleKeys[1]) ? "Shuffle song warp destinations" : label;
}
inline const char* GameplayRuleSearchAliases(const char* name) {
    if (!name)
        return "";
    if (!std::strcmp(name, kSongTravelRuleKeys[0]))
        return "adult Link child any age crossgame cross game songs cross game travel cross warp transport "
               "transportation playback Requiem Spirit Desert Colossus Minuet Bolero Serenade Nocturne Prelude Song of "
               "Soaring owl";
    if (!std::strcmp(name, kSongTravelRuleKeys[1]))
        return "warp destination shuffle shuffled mixed song travel transport transportation saved map cross game "
               "travel";
    return "";
}

// A search link scrolls only once. The visible highlight stays until the user
// leaves the page; drawing other rule pages cannot consume the pending focus.
class MenuRuleFocus {
  public:
    void Set(const char* name) {
        mName = name ? name : "";
        mScrollPending = !mName.empty();
    }
    void Clear() {
        mName.clear();
        mScrollPending = false;
    }
    bool Matches(const char* name) const {
        return name && !mName.empty() && mName == name;
    }
    bool ConsumeScrollFor(const char* name) {
        if (!mScrollPending || !Matches(name))
            return false;
        mScrollPending = false;
        return true;
    }

  private:
    std::string mName;
    bool mScrollPending = false;
};

struct MenuSearchReturn {
    std::string query, scope, hub;
    void Capture(const std::string& text, const std::string& sourceScope, const std::string& sourceHub) {
        if (MenuSearchWords(text).empty())
            return;
        query = text;
        scope = sourceScope;
        hub = sourceHub;
    }
    bool HasResult() const {
        return !query.empty();
    }
    void Clear() {
        query.clear();
        scope.clear();
        hub.clear();
    }
};
} // namespace ComboRando
