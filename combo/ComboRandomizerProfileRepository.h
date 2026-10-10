#pragma once
#include "ComboRandomizerProfiles.h"
#include <filesystem>
#include <fstream>
#include <cstdio>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
namespace ComboRando::Profiles {
// Owns only the profile sidecar. The caller supplies an already read saved seed;
// this class cannot read, migrate, write or activate a gameplay save.
class Repository {
    Store store;
    std::filesystem::path path;
    uint64_t changeRevision = 1;
    void persist(const Store& next) {
        std::filesystem::create_directories(path.parent_path());
        auto temporary = path;
        temporary += ".tmp";
        {
            std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
            if (!out)
                throw std::runtime_error("Cannot write settings profile");
            out << next.json().dump(2) << '\n';
            out.flush();
            if (!out)
                throw std::runtime_error("Cannot finish settings profile write");
        }
#ifdef _WIN32
        if (!MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            throw std::runtime_error("Cannot replace settings profile");
#else
        if (std::rename(temporary.c_str(), path.c_str()) != 0)
            throw std::runtime_error("Cannot replace settings profile");
#endif
    }

  public:
    Repository(std::filesystem::path file, const Json& legacy) : store(legacy), path(std::move(file)) {
        if (std::filesystem::exists(path)) {
            std::ifstream input(path, std::ios::binary);
            Json saved;
            input >> saved;
            store.restore(saved);
        } else
            persist(store); // Capture the legacy setup once, before any file generates.
    }
    const Store& view() const {
        return store;
    }
    uint64_t revision() const noexcept {
        return changeRevision;
    }
    void setDraft(int slot, const Json& input) {
        auto next = store;
        next.setDraft(slot, input);
        persist(next);
        store = std::move(next);
        ++changeRevision;
    }
    void copySaved(int source, int destination, const Json& seed) {
        auto next = store;
        next.copySaved(source, destination, seed);
        persist(next);
        store = std::move(next);
        ++changeRevision;
    }
    void copyDraft(int source, int destination, const Json& seed = Json()) {
        RequireSlot(source);
        RequireSlot(destination);
        setDraft(destination, store.draft(source, seed));
    }
    void importSeed(int slot, const Json& seed, bool copyToDraft) {
        auto next = store;
        next.importSeed(slot, seed, copyToDraft);
        persist(next);
        store = std::move(next);
        ++changeRevision;
    }
    void migrateLegacyPending(const Json& seed) {
        auto next = store;
        next.migrateLegacyPending(seed);
        persist(next);
        store = std::move(next);
        ++changeRevision;
    }
    Attempt begin(int slot, const Json& seed, const std::string& text) {
        auto attempt = store.begin(slot, seed, text);
        ++changeRevision;
        return attempt;
    }
    void finish(uint64_t token, bool success, const Json& seed = Json(), const std::string& error = "") {
        auto next = store;
        next.finish(token, success, seed, error);
        persist(next);
        store = std::move(next);
        ++changeRevision;
    }
};
} // namespace ComboRando::Profiles
