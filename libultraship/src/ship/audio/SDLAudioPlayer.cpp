#include "ship/audio/SDLAudioPlayer.h"
#include <spdlog/spdlog.h>
#if defined(__ANDROID__) && defined(COMBO_BUILD)
#include <jni.h>
#include <mutex>

namespace {
std::recursive_mutex sAndroidAudioMutex;
int AndroidOutputRate() {
    auto* env = static_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());
    jobject activity = static_cast<jobject>(SDL_AndroidGetActivity());
    if (!env || !activity) return 48000;
    jclass cls = env->GetObjectClass(activity);
    jmethodID method = env->GetMethodID(cls, "getAudioOutputSampleRate", "()I");
    int rate = method ? env->CallIntMethod(activity, method) : 48000;
    if (env->ExceptionCheck()) { env->ExceptionClear(); rate = 48000; }
    env->DeleteLocalRef(cls);
    env->DeleteLocalRef(activity);
    return rate >= 32000 && rate <= 192000 ? rate : 48000;
}
} // namespace
#endif

namespace Ship {

SDLAudioPlayer::~SDLAudioPlayer() {
    SPDLOG_TRACE("destruct SDL audio player");
    DoClose();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

void SDLAudioPlayer::DoClose() {
#if defined(__ANDROID__) && defined(COMBO_BUILD)
    const std::lock_guard<std::recursive_mutex> lock(sAndroidAudioMutex);
#endif
    if (mDevice != 0) {
        // Pause playback first
        SDL_PauseAudioDevice(mDevice, 1);
        // Clear any queued audio to prevent glitches when reopening
        SDL_ClearQueuedAudio(mDevice);
        SDL_CloseAudioDevice(mDevice);
        mDevice = 0;
    }
#if defined(__ANDROID__) && defined(COMBO_BUILD)
    if (mConversion) SDL_FreeAudioStream(mConversion);
    mConversion = nullptr;
    mSourceFrames = mConvertedSourceFrames = 0;
#endif
}

bool SDLAudioPlayer::DoInit() {
#if defined(__ANDROID__) && defined(COMBO_BUILD)
    const std::lock_guard<std::recursive_mutex> lock(sAndroidAudioMutex);
#endif
    if (SDL_Init(SDL_INIT_AUDIO) != 0) {
        SPDLOG_ERROR("SDL init error: {}", SDL_GetError());
        return false;
    }

    // Always open with the correct number of output channels
    mNumChannels = this->GetNumOutputChannels();

    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = this->GetSampleRate();
#if defined(__ANDROID__) && defined(COMBO_BUILD)
    want.freq = AndroidOutputRate();
#endif
    want.format = AUDIO_S16SYS;
    want.channels = mNumChannels;
    want.samples = this->GetSampleLength();
    want.callback = NULL;

    mDevice = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
#if defined(__ANDROID__) && defined(COMBO_BUILD)
    if (mDevice == 0 && want.freq != GetSampleRate()) {
        // Preserve playback on routes which reject the native-rate request.
        want.freq = GetSampleRate();
        mDevice = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    }
#endif
    if (mDevice == 0) {
        SPDLOG_ERROR("SDL_OpenAudio error: {}", SDL_GetError());
        return false;
    }

#if defined(__ANDROID__) && defined(COMBO_BUILD)
    mOutputRate = have.freq;
    mSourceFrames = mConvertedSourceFrames = 0;
    if (mOutputRate != GetSampleRate()) {
        mConversion = SDL_NewAudioStream(AUDIO_S16SYS, mNumChannels, GetSampleRate(),
                                        AUDIO_S16SYS, mNumChannels, mOutputRate);
        if (!mConversion) {
            SPDLOG_ERROR("SDL audio conversion error: {}", SDL_GetError());
            DoClose();
            return false;
        }
    }
#endif

    SPDLOG_INFO("SDL Audio initialized: driver={}, {} channels, {} Hz, output block={} frames, game queue={} frames",
                SDL_GetCurrentAudioDriver(), mNumChannels, have.freq, have.samples, GetDesiredBuffered());

    SDL_PauseAudioDevice(mDevice, 0);
    return true;
}

int SDLAudioPlayer::Buffered() {
#if defined(__ANDROID__) && defined(COMBO_BUILD)
    const std::lock_guard<std::recursive_mutex> lock(sAndroidAudioMutex);
    if (!mDevice) return 0;
    const uint64_t outputFrames = SDL_GetQueuedAudioSize(mDevice) / (sizeof(int16_t) * mNumChannels);
    // Include samples retained by SDL's streaming resampler, and always report
    // frames in the engines' 32 kHz units. Otherwise conversion adds an unseen
    // reservoir or the producer changes music speed to chase a mismatched queue.
    const uint64_t convertedSource = mConvertedSourceFrames;
    const uint64_t pending = mSourceFrames > convertedSource ? mSourceFrames - convertedSource : 0;
    return static_cast<int>(outputFrames * GetSampleRate() / mOutputRate + pending);
#else
    return SDL_GetQueuedAudioSize(mDevice) / (sizeof(int16_t) * mNumChannels);
#endif
}

void SDLAudioPlayer::DoPlay(const uint8_t* buf, size_t len) {
#if defined(__ANDROID__) && defined(COMBO_BUILD)
    const std::lock_guard<std::recursive_mutex> lock(sAndroidAudioMutex);
#endif
    if (Buffered() < 6000) {
#if defined(__ANDROID__) && defined(COMBO_BUILD)
        if (mConversion) {
            if (SDL_AudioStreamPut(mConversion, buf, static_cast<int>(len)) < 0) {
                SPDLOG_ERROR("SDL audio conversion failed: {}", SDL_GetError());
                return;
            }
            mSourceFrames += len / (sizeof(int16_t) * mNumChannels);
            // A multiple of both stereo and 5.1 frame sizes. SDL retains its
            // filter history between submissions; never flush each game tick.
            uint8_t converted[12288];
            int bytes;
            uint64_t outputFrames = 0;
            while ((bytes = SDL_AudioStreamGet(mConversion, converted, sizeof(converted))) > 0) {
                outputFrames += bytes / (sizeof(int16_t) * mNumChannels);
                if (SDL_QueueAudio(mDevice, converted, bytes) < 0) {
                    SPDLOG_ERROR("SDL audio queue failed: {}", SDL_GetError());
                    return;
                }
            }
            // SDL rounds each converted block to whole output frames. Charge
            // that rounding to this submission, not to a phantom queue which
            // grows forever on 44.1 kHz routes and eventually stalls production.
            mConvertedSourceFrames += (outputFrames * GetSampleRate() + mOutputRate - 1) / mOutputRate;
            if (bytes < 0) SPDLOG_ERROR("SDL audio conversion read failed: {}", SDL_GetError());
            return;
        }
#endif
        // Don't fill the audio buffer too much in case this happens
        SDL_QueueAudio(mDevice, buf, len);
    }
}
} // namespace Ship
