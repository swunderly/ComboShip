"""OoT's audio thread keeps running, and never spins, when audio output fails.

1. The production OoT producer (soh/soh/OTRGlobals.cpp OTRAudio_Thread) runs on its own thread against a backend that
   takes nothing (a failed device: Buffered() stays 0). Its pre-buffer loop refilled the reservoir until the backend
   reported it full, so it ran the audio engine flat out, without sleeping, until the game exited. Now it waits for
   the next wake. A backend that takes audio is still filled to the desired level.
2. The production Android conversion path (libultraship SDLAudioPlayer.cpp DoPlay, Buffered) against a fake SDL with
   a 32 -> 48 kHz stream and a queue that fails once. A failed SDL_QueueAudio returned before the block it lost was
   charged, so Buffered() counted that block as pending forever; a few failures and the producers, which wait for the
   queue to drain, stopped for good, which left the audio engine (and a game waiting on an audio reset) stuck.
"""
from pathlib import Path
import argparse
import subprocess
import hostcxx

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
a = p.parse_args()
source, out = a.source.resolve(), a.build_dir.resolve()
out.mkdir(parents=True, exist_ok=True)


def function(text, signature):
    start = hostcxx.find(text, signature)
    depth, end = 1, text.index('{', start) + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]


oot = (source / 'soh/soh/OTRGlobals.cpp').read_text(encoding='utf-8')
player = (source / 'libultraship/src/ship/audio/SDLAudioPlayer.cpp').read_text(encoding='utf-8')
android = 'defined(__ANDROID__) && defined(COMBO_BUILD)'
assert android in player
mobile = '\n'.join(function(player, s) for s in ('int SDLAudioPlayer::Buffered()', 'void SDLAudioPlayer::DoPlay('))
mobile = mobile.replace(android, '1')  # the Android build's conversion path
cpp = r'''
#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>
using u32 = uint32_t;
using u8 = uint8_t;
using s16 = int16_t;

// ---- 1. The OoT producer, on a thread.
static struct {
    std::thread thread;
    std::condition_variable cv_to_thread;
    std::mutex mutex;
    std::atomic_bool running;
    std::atomic_bool processing;
} audio;
static int R_UPDATE_RATE = 1;
static std::atomic<long long> ticks{ 0 };
static std::atomic_bool accepting{ false };
// The backend: queues a batch when it accepts one, and plays 32000 frames a second of its queue.
static std::mutex backend;
static double queue = 0;
static std::chrono::steady_clock::time_point drained;
static void Drain() {
    const auto now = std::chrono::steady_clock::now();
    queue = std::max(0.0, queue - std::chrono::duration<double>(now - drained).count() * 32000);
    drained = now;
}
void AudioMgr_CreateNextAudioBuffer(s16*, u32) { ++ticks; }
int AudioPlayer_Buffered(void) {
    std::lock_guard<std::mutex> lock(backend);
    Drain();
    return int(queue);
}
int AudioPlayer_GetDesiredBuffered(void) { return 4096; }
void AudioPlayer_Play(const uint8_t*, uint32_t len) {
    std::lock_guard<std::mutex> lock(backend);
    Drain();
    if (accepting)
        queue += len / 4;
}
''' + function(oot, 'void OTRAudio_Thread() {') + r'''
static long long Run(bool accept, int milliseconds, int* buffered) {
    accepting = accept;
    ticks = 0;
    {
        std::lock_guard<std::mutex> lock(backend);
        queue = 0;
        drained = std::chrono::steady_clock::now();
    }
    audio.running = true;
    audio.processing = true; // the first rendered frame's wake
    std::thread thread(OTRAudio_Thread);
    audio.cv_to_thread.notify_one();
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    *buffered = AudioPlayer_Buffered();
    {
        std::unique_lock<std::mutex> lock(audio.mutex);
        audio.running = false;
    }
    audio.cv_to_thread.notify_all();
    thread.join();
    return ticks;
}

// ---- 2. The Android conversion path, against a fake SDL.
typedef uint32_t SDL_AudioDeviceID;
struct SDL_AudioStream {
    std::vector<int16_t> in; // stereo frames at 32 kHz, converted 2 -> 3 frames (48 kHz)
};
static const char* SDL_GetError() { return "fake"; }
static int SDL_AudioStreamPut(SDL_AudioStream* s, const void* buf, int len) {
    const auto* frames = static_cast<const int16_t*>(buf);
    s->in.insert(s->in.end(), frames, frames + len / 2);
    return 0;
}
static int SDL_AudioStreamGet(SDL_AudioStream* s, void* out, int len) {
    const size_t groups = std::min(s->in.size() / 4, size_t(len) / 12); // 2 frames in, 3 frames out
    std::memset(out, 0, groups * 12);
    s->in.erase(s->in.begin(), s->in.begin() + groups * 4);
    return int(groups * 12);
}
static std::vector<uint8_t> queued;
static int queueFailures = 0;
static uint32_t SDL_GetQueuedAudioSize(SDL_AudioDeviceID) { return uint32_t(queued.size()); }
static int SDL_QueueAudio(SDL_AudioDeviceID, const void* data, uint32_t n) {
    if (queueFailures > 0) {
        --queueFailures;
        return -1;
    }
    queued.insert(queued.end(), static_cast<const uint8_t*>(data), static_cast<const uint8_t*>(data) + n);
    return 0;
}
#define SPDLOG_ERROR(...) ((void)0)
static std::recursive_mutex sAndroidAudioMutex;
namespace Ship {
class SDLAudioPlayer {
  public:
    SDL_AudioDeviceID mDevice = 1;
    int mNumChannels = 2, mOutputRate = 48000;
    uint64_t mSourceFrames = 0, mConvertedSourceFrames = 0;
    SDL_AudioStream* mConversion = nullptr;
    int GetSampleRate() { return 32000; }
    int Buffered();
    void DoPlay(const uint8_t*, size_t);
};
''' + mobile + r'''
} // namespace Ship

int main() {
    int buffered = 0;
    // A backend that takes nothing: one batch per wake (at least every 5 ms), never a spin.
    const long long refused = Run(false, 200, &buffered);
    std::printf("failed output: %lld engine ticks in 200 ms\n", refused);
    assert(refused > 0 && refused < 400);
    // A backend that takes audio is still kept at the desired level.
    const long long healthy = Run(true, 200, &buffered);
    std::printf("working output: %lld engine ticks in 200 ms, %d frames buffered\n", healthy, buffered);
    assert(healthy >= 10 && buffered >= 4096 - 2 * 544 && buffered <= 4096);

    SDL_AudioStream stream;
    Ship::SDLAudioPlayer mobile;
    mobile.mConversion = &stream;
    std::vector<int16_t> batch(533 * 2, 0);
    auto play = [&] { mobile.DoPlay(reinterpret_cast<const uint8_t*>(batch.data()), batch.size() * 2); };
    for (int i = 0; i < 10; ++i)
        play();
    queued.clear(); // the device played it all
    assert(mobile.Buffered() <= 1); // only the frame the resampler keeps
    // One failed queue loses a block; once the device has played the rest, nothing is left counted.
    queueFailures = 1;
    play();
    queued.clear();
    std::printf("after a failed queue and a drained device: %d frames counted\n", mobile.Buffered());
    assert(mobile.Buffered() <= 2);
    play();
    assert(!queued.empty() && mobile.Buffered() <= int(queued.size() / 4) * 2 / 3 + 2);
    std::puts("PASS: OoT's audio producer waits instead of spinning on a failed output, and a failed queue is not "
              "counted as pending");
    return 0;
}
'''
path = out / 'audio_output_failure.cpp'
path.write_text(cpp, encoding='utf-8')
exe = hostcxx.build(a.compiler, path, std='c++17', warnings=False)
subprocess.run([str(exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)
