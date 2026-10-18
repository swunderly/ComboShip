// Entrance capability catalog: the only source of truth for which OoTMM v32.3
// entrance setting values a build reads, generates or accepts.
//
// Each admitted value is Hidden (compiled in, never admitted), Dev (admitted only
// by a COMBO_ENTRANCE_DEV build) or Public (admitted by every build). Ids are
// contracts: before a value is Public, any change to what it generates or
// validates takes a new id; once Public, the id and its rules are frozen and stay
// accepted, as LegacyEnvelope.h does for the four private MM profiles.
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace ComboRando::Entrances {
enum class Stage : uint8_t { Hidden, Dev, Public };

// One admitted setting value, by its pinned OoTMM key and value names. Each value
// has its own stage, so the values of one row open one at a time.
struct CapabilityValue {
    std::string_view key, value;
    Stage stage;
};

struct Capability {
    std::string_view id;
    std::span<const CapabilityValue> values;
    // One row must admit every non-default value of a combination, so values of
    // two rows combine only through a companion row that lists them all. A
    // companion is never more open than the rows it names.
    std::span<const std::string_view> companions;
    // Used only when selected by id (CapabilitySet::Selected), and never Public.
    bool devOnly = false;
};

inline constexpr CapabilityValue kWallmastersMmTrancheValues[] = { { "erWallmasters", "ownGame", Stage::Dev } };
inline constexpr CapabilityValue kWallmastersOwnGameValues[] = { { "erWallmasters", "ownGame", Stage::Hidden } };
inline constexpr CapabilityValue kWallmastersVanillaValues[] = { { "erWallmasters", "ownGame", Stage::Dev } };
inline constexpr CapabilityValue kOneWaysOwnGameValues[] = {
    { "erOneWays", "ownGame", Stage::Hidden },        { "erOneWaysMajor", "true", Stage::Hidden },
    { "erOneWaysIkana", "true", Stage::Hidden },      { "erOneWaysOwls", "true", Stage::Hidden },
    { "erOneWaysWaterVoids", "true", Stage::Hidden }, { "erOneWaysAnywhere", "true", Stage::Hidden },
};
inline constexpr CapabilityValue kWallmastersWithOneWaysValues[] = {
    { "erWallmasters", "ownGame", Stage::Hidden },  { "erOneWays", "ownGame", Stage::Hidden },
    { "erOneWaysMajor", "true", Stage::Hidden },    { "erOneWaysIkana", "true", Stage::Hidden },
    { "erOneWaysOwls", "true", Stage::Hidden },     { "erOneWaysWaterVoids", "true", Stage::Hidden },
    { "erOneWaysAnywhere", "true", Stage::Hidden },
};
inline constexpr std::string_view kWallmastersWithOneWaysCompanions[] = { "wallmasters-own-game-v1",
                                                                          "one-ways-own-game-v1" };

inline constexpr Capability kEntranceCapabilities[] = {
    // The dev-only MM Wallmaster tranche: Dev in Phase A (M1-07), never Public.
    { "wallmasters-own-game-mm-tranche", kWallmastersMmTrancheValues, {}, true },
    { "wallmasters-own-game-v1", kWallmastersOwnGameValues, {}, false },
    { "one-ways-own-game-v1", kOneWaysOwnGameValues, {}, false },
    // Admits Wallmasters with One-Ways once Wallmaster arrivals at one-way rows are bound.
    { "wallmasters-with-one-ways-v1", kWallmastersWithOneWaysValues, kWallmastersWithOneWaysCompanions, false },
    // Owner-approved vanilla-first staging (2026-10-18). Private validation only; never Public here.
    { "wallmasters-own-game-vanilla-v1", kWallmastersVanillaValues, {}, false },
};
inline constexpr size_t kCapabilityCount = std::size(kEntranceCapabilities);

namespace CapabilityDetail {
constexpr size_t Find(std::string_view id) {
    for (size_t i = 0; i < kCapabilityCount; ++i)
        if (kEntranceCapabilities[i].id == id)
            return i;
    return kCapabilityCount;
}
constexpr bool Lists(const Capability& row, const CapabilityValue& value, Stage least) {
    for (const auto& own : row.values)
        if (own.key == value.key && own.value == value.value && own.stage >= least)
            return true;
    return false;
}
constexpr bool IdsAreUnique() {
    for (size_t i = 0; i < kCapabilityCount; ++i)
        if (kEntranceCapabilities[i].id.empty() || Find(kEntranceCapabilities[i].id) != i)
            return false;
    return true;
}
constexpr bool RowsFitTheirMask() {
    for (const auto& row : kEntranceCapabilities)
        if (row.values.empty() || row.values.size() > 32)
            return false;
    return true;
}
constexpr bool DevOnlyNeverPublic() {
    for (const auto& row : kEntranceCapabilities)
        for (const auto& value : row.values)
            if (row.devOnly && value.stage == Stage::Public)
                return false;
    return true;
}
// Full stays hidden until cross-game entrances exist.
constexpr bool FullNeverAdmitted() {
    for (const auto& row : kEntranceCapabilities)
        for (const auto& value : row.values)
            if (value.value == "full")
                return false;
    return true;
}
// Every companion exists, and every value of a row with companions is listed by
// one of them at a stage at least as open.
constexpr bool CompanionsAreClosed() {
    for (size_t i = 0; i < kCapabilityCount; ++i) {
        const auto& row = kEntranceCapabilities[i];
        for (const auto companion : row.companions) {
            const size_t c = Find(companion);
            if (c == kCapabilityCount || c == i || (kEntranceCapabilities[c].devOnly && !row.devOnly))
                return false;
        }
        for (const auto& value : row.values) {
            bool listed = row.companions.empty();
            for (const auto companion : row.companions)
                listed = listed || Lists(kEntranceCapabilities[Find(companion)], value, value.stage);
            if (!listed)
                return false;
        }
    }
    return true;
}
} // namespace CapabilityDetail

static_assert(CapabilityDetail::IdsAreUnique(), "Capability ids must be unique");
static_assert(CapabilityDetail::RowsFitTheirMask(), "A capability row has 1 to 32 values");
static_assert(CapabilityDetail::DevOnlyNeverPublic(), "A devOnly capability can never be Public");
static_assert(CapabilityDetail::FullNeverAdmitted(), "Full is never admitted");
static_assert(CapabilityDetail::CompanionsAreClosed(), "Companion rows must stay within the rows they name");

// The values one build admits. Bit j of a row's mask admits that row's values[j].
class CapabilitySet {
  public:
    // Every catalog value at least as open as `least`: Stage::Public is the
    // shipping set, Stage::Dev the dev set, and Stage::Hidden the whole catalog
    // (tests only). Builds use ActiveCapabilities().
    static constexpr CapabilitySet AtLeast(Stage least) {
        CapabilitySet set;
        for (size_t i = 0; i < kCapabilityCount; ++i)
            for (size_t j = 0; j < kEntranceCapabilities[i].values.size(); ++j)
                if (kEntranceCapabilities[i].values[j].stage >= least)
                    set.rows_[i] |= uint32_t(1) << j;
        return set;
    }
    static constexpr bool Known(std::string_view id) {
        return CapabilityDetail::Find(id) < kCapabilityCount;
    }
    // Only the named row; empty for an unknown id.
    constexpr CapabilitySet Only(std::string_view id) const {
        CapabilitySet set;
        set.devBuild = devBuild;
        const size_t i = CapabilityDetail::Find(id);
        if (i < kCapabilityCount)
            set.rows_[i] = rows_[i];
        return set;
    }
    // Without the devOnly rows, which are used only when selected by id.
    constexpr CapabilitySet WithoutDevOnly() const {
        CapabilitySet set = *this;
        for (size_t i = 0; i < kCapabilityCount; ++i)
            if (kEntranceCapabilities[i].devOnly)
                set.rows_[i] = 0;
        return set;
    }
    // The rows a settings reader uses: Only(id) when an id is selected (headless
    // --er-capability), otherwise WithoutDevOnly().
    constexpr CapabilitySet Selected(std::string_view id) const {
        return id.empty() ? WithoutDevOnly() : Only(id);
    }
    constexpr CapabilitySet operator|(const CapabilitySet& other) const {
        CapabilitySet set = *this;
        set.devBuild = devBuild || other.devBuild;
        for (size_t i = 0; i < kCapabilityCount; ++i)
            set.rows_[i] |= other.rows_[i];
        return set;
    }
    constexpr bool Empty() const {
        for (const auto mask : rows_)
            if (mask != 0)
                return false;
        return true;
    }
    // The row admits at least one value.
    constexpr bool Active(std::string_view id) const {
        const size_t i = CapabilityDetail::Find(id);
        return i < kCapabilityCount && rows_[i] != 0;
    }
    constexpr bool Admits(size_t row, std::string_view key, std::string_view value) const {
        if (row >= kCapabilityCount)
            return false;
        const auto& values = kEntranceCapabilities[row].values;
        for (size_t j = 0; j < values.size(); ++j)
            if (((rows_[row] >> j) & 1u) != 0 && values[j].key == key && values[j].value == value)
                return true;
        return false;
    }
    constexpr bool Admits(std::string_view key, std::string_view value) const {
        for (size_t row = 0; row < kCapabilityCount; ++row)
            if (Admits(row, key, value))
                return true;
        return false;
    }
    constexpr bool operator==(const CapabilitySet&) const = default;

    // Set by ActiveCapabilities() in a COMBO_ENTRANCE_DEV build.
    bool devBuild = false;

  private:
    std::array<uint32_t, kCapabilityCount> rows_{};
};

#ifdef COMBO_ENTRANCE_DEV
// Every binary that can admit Dev values carries this text. Release tooling
// refuses it (ComboAndroid/tools/verify_apk.py), and release.ps1 refuses a build
// configured with COMBO_ENTRANCE_DEV.
inline constexpr char kEntranceDevMarker[] = "ComboShip entrance dev build - not for release";
// Read through a volatile pointer, so neither the optimizer nor the linker drops it.
inline const char* volatile gEntranceDevMarker = kEntranceDevMarker;
#endif

// Public values, plus Dev values in a COMBO_ENTRANCE_DEV build.
inline CapabilitySet ActiveCapabilities() {
#ifdef COMBO_ENTRANCE_DEV
    auto set = CapabilitySet::AtLeast(Stage::Dev);
    set.devBuild = gEntranceDevMarker[0] != '\0';
    return set;
#else
    return CapabilitySet::AtLeast(Stage::Public);
#endif
}
} // namespace ComboRando::Entrances
