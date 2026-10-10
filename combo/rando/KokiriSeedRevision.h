#pragma once
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace ComboRando {
class UnsupportedKokiriSeedRevision : public std::invalid_argument {
public:
    explicit UnsupportedKokiriSeedRevision(const char* message = "Unsupported Kokiri invisible rupee seed revision")
        : std::invalid_argument(message) {}
};
inline constexpr const char* kKokiriSeedRevision = "kokiriInvisibleRupeeRevision";
inline constexpr int kKokiriLegacyRevision = 0;
inline constexpr int kKokiriModernRevision = 1;
inline constexpr const char* kKokiriGrassCatalogRevision = "kokiriGrassCatalogRevision";
inline constexpr int kKokiriGrassLegacyRevision = 0;
inline constexpr int kKokiriGrassModernRevision = 1;
// Transient launcher/native preflight status; never stored in a save or seed.
// This preserves the typed unsupported-catalog path without throwing across DLLs.
inline constexpr const char* kKokiriNativePreflightError = "__comboKokiriCatalogError";

// The containing object is oot in a consolidated seed, or the native spoiler/save section.
// Absence is legacy. No option/flag/check/item index is consumed by this named extension.
inline int ReadKokiriSeedRevision(const nlohmann::json& object) {
    if (!object.is_object()) throw std::invalid_argument("Kokiri seed revision requires an object");
    const auto it = object.find(kKokiriSeedRevision);
    if (it == object.end()) return kKokiriLegacyRevision;
    if (!it->is_number_integer() || it->is_boolean() ||
        (*it != kKokiriLegacyRevision && *it != kKokiriModernRevision))
        throw UnsupportedKokiriSeedRevision();
    return it->get<int>();
}
inline int ReadKokiriGrassCatalogRevision(const nlohmann::json& object) {
    if (!object.is_object()) throw std::invalid_argument("Kokiri grass catalog revision requires an object");
    const auto it = object.find(kKokiriGrassCatalogRevision);
    if (it == object.end()) return kKokiriGrassLegacyRevision;
    if (!it->is_number_integer() || it->is_boolean() ||
        (*it != kKokiriGrassLegacyRevision && *it != kKokiriGrassModernRevision))
        throw UnsupportedKokiriSeedRevision("Unsupported Kokiri grass catalog revision");
    return it->get<int>();
}
struct KokiriCatalogRevisions { int invisibleRupees; int grass; };
inline KokiriCatalogRevisions ReadKokiriCatalogRevisions(const nlohmann::json& object) {
    // Validate both fields before either native setter can mutate a bound context.
    return {ReadKokiriSeedRevision(object), ReadKokiriGrassCatalogRevision(object)};
}
inline int ReadKokiriSaveRevision(const nlohmann::json& save) {
    if (!save.is_object()) throw std::invalid_argument("Kokiri save revision requires an object");
    const auto error = save.find(kKokiriNativePreflightError);
    if (error != save.end()) {
        if (!error->is_string()) throw std::invalid_argument("Malformed Kokiri catalog preflight status");
        throw UnsupportedKokiriSeedRevision("Saved seed and native Kokiri check catalogs are incompatible");
    }
    const auto sections = save.find("sections");
    if (sections == save.end()) return kKokiriLegacyRevision;
    const auto randomizer = sections->find("randomizer");
    if (randomizer == sections->end()) return kKokiriLegacyRevision;
    const auto data = randomizer->find("data");
    if (data == randomizer->end()) return kKokiriLegacyRevision;
    return ReadKokiriCatalogRevisions(*data).invisibleRupees;
}
using FnGetKokiriSeedRevision = int (*)();
using FnSetKokiriSeedRevision = int (*)(int);
inline void ApplyKokiriSeedRevision(FnSetKokiriSeedRevision set, int revision) {
    if ((revision != kKokiriLegacyRevision && revision != kKokiriModernRevision) ||
        !set || !set(revision))
        throw std::runtime_error("Native Kokiri seed revision binding unavailable");
}
// Generation/import failures must not leave the previously bound file in a different family.
class KokiriSeedRevisionScope {
    FnSetKokiriSeedRevision set;
    int previous;
    bool keep = false;
public:
    KokiriSeedRevisionScope(FnGetKokiriSeedRevision get, FnSetKokiriSeedRevision setter, int revision)
        : set(setter), previous(get ? get() : -1) {
        if (previous != kKokiriLegacyRevision && previous != kKokiriModernRevision)
            throw std::runtime_error("Native Kokiri seed revision context unavailable");
        ApplyKokiriSeedRevision(set, revision);
    }
    void Keep() { keep = true; }
    ~KokiriSeedRevisionScope() { if (!keep && set) set(previous); }
};
class KokiriGrassCatalogRevisionScope {
    FnSetKokiriSeedRevision set;
    int previous;
    bool keep = false;
public:
    KokiriGrassCatalogRevisionScope(FnGetKokiriSeedRevision get, FnSetKokiriSeedRevision setter, int revision)
        : set(setter), previous(get ? get() : -1) {
        if (previous != 0 && previous != 1)
            throw std::runtime_error("Native Kokiri grass catalog context unavailable");
        ApplyKokiriSeedRevision(set, revision);
    }
    void Keep() { keep = true; }
    ~KokiriGrassCatalogRevisionScope() { if (!keep && set) set(previous); }
};
} // namespace ComboRando
