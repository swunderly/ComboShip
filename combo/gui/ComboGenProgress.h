// combo/gui/ComboGenProgress.h
// ComboShip: thread-safe progress state for the cross-world generate worker.
// Written by the worker thread; polled by the UI thread.
#pragma once
#include <atomic>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <stdexcept>
namespace ComboRando {
struct GenerationCancelled : std::runtime_error {
    GenerationCancelled() : std::runtime_error("Generation cancelled") {
    }
};
inline uint64_t GenerationClockMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}
struct ComboGenProgress {
    mutable std::atomic<int> disposition{ 0 }; // 0 working, 1 cancellation requested, 2 committing
    std::atomic<uint64_t> attempt{ 0 }, startedMs{ 0 };
    mutable std::atomic<uint64_t> nextWarningSeconds{ 300 };
    bool RequestCancel() const {
        int expected = 0;
        return disposition.compare_exchange_strong(expected, 1);
    }
    bool CancelRequested() const {
        return disposition.load() == 1;
    }
    bool CanCancel() const {
        return disposition.load() == 0;
    }
    uint64_t ElapsedSeconds(uint64_t now = GenerationClockMs()) const {
        const auto start = startedMs.load();
        return now >= start ? (now - start) / 1000 : 0;
    }
    bool WarningDue(uint64_t elapsed) const {
        return elapsed >= nextWarningSeconds.load();
    }
    void KeepTrying(uint64_t elapsed) const {
        nextWarningSeconds = elapsed + 300;
    }
    void BeginCommit() {
        int expected = 0;
        if (!disposition.compare_exchange_strong(expected, 2))
            throw GenerationCancelled();
    }
    std::atomic<int> phase{ 0 }; // 0 Idle,1 Preparing,2 Placing,3 Finalizing
    std::atomic<int> placed{ 0 };
    std::atomic<int> total{ 0 };
    std::atomic<bool> running{ false };
    std::atomic<bool> done{ false };
    std::atomic<bool> success{ false };
    std::atomic<uint32_t> seed{ 0 };
    std::atomic<int> foreignCount{ 0 };
    std::atomic<int> ootCheckCount{ 0 }; // checks OOT contributed to the combined pool
    std::atomic<int> mmCheckCount{ 0 };  // checks MM contributed to the combined pool
    char error[256] = { 0 };
    // Resolved input seed string (the reproducible token to paste back into the Seed field). For a
    // blank input this is the concrete random string actually used. Written before done; read after.
    char seedStr[128] = { 0 };
    void Reset() {
        disposition = 0;
        attempt = 0;
        startedMs = GenerationClockMs();
        nextWarningSeconds = 300;
        running = false;
        done = false;
        phase = 0;
        placed = 0;
        total = 0;
        success = false;
        seed = 0;
        foreignCount = 0;
        ootCheckCount = 0;
        mmCheckCount = 0;
        error[0] = '\0';
        seedStr[0] = '\0';
    }
    void SetError(const char* m) {
        if (!m) {
            error[0] = '\0';
            return;
        }
        std::strncpy(error, m, sizeof(error) - 1);
        error[sizeof(error) - 1] = '\0';
    }
    static const char* PhaseLabel(int p) {
        switch (p) {
            case 1:
                return "Preparing pools";
            case 2:
                return "Placing items";
            case 3:
                return "Finalizing";
            default:
                return "Idle";
        }
    }
};
// Cancellation checkpoints run only in the launcher worker, outside engine C frames.
inline thread_local ComboGenProgress* generationProgress = nullptr;
struct GenerationScope {
    ComboGenProgress* previous = generationProgress;
    explicit GenerationScope(ComboGenProgress* p) {
        generationProgress = p;
    }
    ~GenerationScope() {
        generationProgress = previous;
    }
};
inline void CheckGenerationCancellation() {
    if (generationProgress && generationProgress->CancelRequested())
        throw GenerationCancelled();
}
} // namespace ComboRando
