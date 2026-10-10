// Port of pinned OoTMM MM canonical/custom entrance and grotto-exit behavior.
// Context adapter only: not enabled in the bounded Wallmaster runtime profile.
#pragma once
#include "Context.h"
#include "GrottoMetadata.h"
#include <algorithm>
#include <optional>

namespace ComboRando::Entrances {
enum class MmPreviousScene : uint8_t { Other, TerminaField, GreatBayCoast };
struct MmGrottoContext {
    std::array<MmRespawn, 4> respawn;
    MmPreviousScene previousScene = MmPreviousScene::Other;
    int32_t respawnFlag = 0;
    bool snowheadCleared = false;
    uint8_t data() const {
        return static_cast<uint8_t>(respawn[3].data);
    }
    void setData(uint8_t value) {
        respawn[3].data = static_cast<int8_t>(value);
    }
};
inline uint32_t NormalizeMmSceneVariant(uint32_t entrance) {
    uint32_t sceneKey = entrance >> 9;
    switch (sceneKey) {
        case 0x06:
            sceneKey = 0x42;
            break;
        case 0x57:
            sceneKey = 0x4d;
            break;
        case 0x45:
            sceneKey = 0x4a;
            break;
        case 0x5b:
            sceneKey = 0x5a;
            break;
        default:
            break;
    }
    return (sceneKey << 9) | (entrance & 0x1ff);
}
inline std::optional<Key> CanonicalMmEntrance(uint32_t entrance, const MmGrottoContext& context) {
    entrance = NormalizeMmSceneVariant(entrance);
    if (entrance == Find("MM_GROTTO_TYPE_GENERIC")->id) {
        const auto value = context.data() & 0x1f;
        const auto it = std::find(kMmGenericGrottoData.begin(), kMmGenericGrottoData.end(), value);
        if (it == kMmGenericGrottoData.end())
            return std::nullopt;
        return Key{ Game::Mm, 0x10000u + static_cast<uint32_t>(it - kMmGenericGrottoData.begin()) };
    }
    if (entrance == Find("MM_GROTTO_TYPE_COW")->id) {
        if (context.previousScene == MmPreviousScene::TerminaField)
            return Key{ Game::Mm, Find("MM_GROTTO_COW_FIELD")->id };
        if (context.previousScene == MmPreviousScene::GreatBayCoast)
            return Key{ Game::Mm, Find("MM_GROTTO_COW_COAST")->id };
        return std::nullopt;
    }
    return Key{ Game::Mm, entrance };
}
inline Key LogicalMmGrottoExit(uint8_t room, const MmGrottoContext& context) {
    std::string_view name;
    switch (room) {
        case 0x00:
            name = "MM_GROTTO_EXIT_GOSSIPS_OCEAN";
            break;
        case 0x01:
            name = "MM_GROTTO_EXIT_GOSSIPS_SWAMP";
            break;
        case 0x02:
            name = "MM_GROTTO_EXIT_GOSSIPS_CANYON";
            break;
        case 0x03:
            name = "MM_GROTTO_EXIT_GOSSIPS_MOUNTAIN";
            break;
        case 0x04: {
            const auto generic = CanonicalMmEntrance(Find("MM_GROTTO_TYPE_GENERIC")->id, context);
            if (generic)
                return { Game::Mm, 0x10100u + generic->id - 0x10000u };
            break;
        }
        case 0x07:
            name = "MM_GROTTO_EXIT_DODONGO";
            break;
        case 0x09:
            name = "MM_GROTTO_EXIT_SCRUB";
            break;
        case 0x0a:
            if (context.previousScene == MmPreviousScene::TerminaField)
                name = "MM_GROTTO_EXIT_COW_FIELD";
            if (context.previousScene == MmPreviousScene::GreatBayCoast)
                name = "MM_GROTTO_EXIT_COW_COAST";
            break;
        case 0x0b:
            name = "MM_GROTTO_EXIT_BIO_BABA";
            break;
        case 0x0d:
            name = "MM_GROTTO_EXIT_PEAHAT";
            break;
        case 0x0e:
            name = "MM_GROTTO_EXIT_HOT_WATER";
            break;
        default:
            break;
    }
    return { Game::Mm, Find(name.empty() ? "MM_INTERNAL_EXIT_GROTTO" : name)->id };
}
inline Key ApplyMmCustomEntrance(Key key, MmGrottoContext& context) {
    if (key.game != Game::Mm)
        throw std::invalid_argument("MM adapter received foreign-game destination");
    if (key.id >= 0x10000 && key.id < 0x10000 + kMmGenericGrottoData.size()) {
        context.setData(static_cast<uint8_t>((context.data() & ~0x1f) | kMmGenericGrottoData[key.id - 0x10000]));
        return { Game::Mm, Find("MM_GROTTO_TYPE_GENERIC")->id };
    }
    if (key.id == Find("MM_GROTTO_COW_FIELD")->id || key.id == Find("MM_GROTTO_COW_COAST")->id) {
        context.previousScene =
            key.id == Find("MM_GROTTO_COW_FIELD")->id ? MmPreviousScene::TerminaField : MmPreviousScene::GreatBayCoast;
        return { Game::Mm, Find("MM_GROTTO_TYPE_COW")->id };
    }
    if (key.id >= 0x10100 && key.id < 0x10100 + kMmGrottoExits.size()) {
        const auto* exit = &kMmGrottoExits[key.id - 0x10100];
        if (key.id == Find("MM_GROTTO_EXIT_GENERIC_MOUNTAIN_VILLAGE")->id && !context.snowheadCleared)
            exit = &kMmGrottoExitMountainWinter;
        MmRespawn native;
        native.pos = { static_cast<float>(exit->x), static_cast<float>(exit->y), static_cast<float>(exit->z) };
        native.entrance = exit->nativeEntrance;
        native.playerParams = 0x04ff;
        native.roomIndex = exit->room;
        // Native MM slots: 3=grotto, 0=void, 2=death. All flags/data/yaw
        // intentionally zero, exactly as applyGrottoExit in pinned source.
        context.respawn[3] = native;
        context.respawn[0] = native;
        context.respawn[2] = native;
        context.respawnFlag = 4;
        return { Game::Mm, native.entrance };
    }
    // Remaining MM grotto/grave entries are native entrances already.
    if (key.id > 0xffffu)
        throw std::invalid_argument("Unknown extended MM arrival ID");
    return key;
}
} // namespace ComboRando::Entrances
