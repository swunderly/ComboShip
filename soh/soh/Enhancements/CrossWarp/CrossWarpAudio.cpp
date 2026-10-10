#ifdef COMBO_BUILD
#include "CrossWarpAudio.h"
#include "global.h"
#include "soh/cvar_prefixes.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include "soh/ResourceManagerHelpers.h"
#include "soh/resource/importer/AudioSampleFactory.h"
#include "soh/resource/importer/AudioSequenceFactory.h"
#include "soh/resource/importer/AudioSoundFontFactory.h"
#include "soh/resource/type/SohResourceType.h"
#include <ship/Context.h>
#include <ship/resource/CrossRMRegistry.h>
#include <ship/resource/ResourceManager.h>
#include <array>
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>
#include <spdlog/spdlog.h>

extern "C" {
extern char** sequenceMap;
extern size_t sequenceMapSize;
extern char** fontMap;
extern size_t fontMapSize;
u8 AudioSeq_ScriptReadU8(SeqScriptState* state);
s16 AudioSeq_ScriptReadS16(SeqScriptState* state);
bool AudioCollection_HasSequenceNum(uint16_t seqId);
}

namespace {
constexpr const char* kPrefix = "@mm-audio:";
constexpr const char* kSequences[] = { "audio/sequences/SongOfSoaring_55", "audio/sequences/Sequence_0",
                                       "audio/sequences/SongOfSoaring(Ocarina)_47" };
constexpr int kFonts[] = { 9, 1, 0 };
std::shared_ptr<Ship::ResourceManager> sResources;
std::array<SequenceData*, 3> sSequences{};
std::array<SoundFont*, 3> sFonts{};
int sSequenceBase = -1, sFontBase = -1;
bool sPlaying = false;
u32 sPreviousAudioErrors = 0;
u16 sPreviousMain = NA_BGM_DISABLED, sPreviousSub = NA_BGM_DISABLED;
// MM AudioSfx_ResetSfxChannelState, with the native centered, non-positional
// values used by En_Test7's wing cues. Only bytes 8..13 are read by its score.
std::array<std::array<std::atomic<u8>, 16>, 16> sSfxState{};
std::array<bool, 0x100> sDialectSeen{};
std::array<bool, 3> sSequenceExecuting{};
std::atomic<u32> sDialectFailures{ 0 };

// SfxChannelState's native float bytes are little endian in the Android runtime.
// Keep the same indexed state seen by A0..A3 and BE, with atomic graph/audio access.
void StoreMmSfxFloat(size_t channel, size_t offset, float value) {
    u32 bits;
    std::memcpy(&bits, &value, sizeof(bits));
    for (size_t i = 0; i < 4; ++i)
        sSfxState[channel][offset + i].store(static_cast<u8>(bits >> (i * 8)));
}
float LoadMmSfxFrequency(size_t channel) {
    u32 bits = 0;
    for (size_t i = 0; i < 4; ++i)
        bits |= static_cast<u32>(sSfxState[channel][4 + i].load()) << (i * 8);
    float value;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}
void UseResidentSampleStorage(SoundFontSample* sample) {
    // MM's archive retains CART tags on some music samples. The resource
    // factory has already copied those bytes into owned host RAM: sending a
    // host pointer through OoT's N64 cartridge DMA cache corrupts its address.
    if (sample)
        sample->medium = MEDIUM_RAM;
}

class ResidentMmSampleFactory final : public Ship::ResourceFactoryBinary {
  public:
    std::shared_ptr<Ship::IResource> ReadResource(std::shared_ptr<Ship::File> file,
                                                  std::shared_ptr<Ship::ResourceInitData> initData) override {
        auto resource = nativeFactory.ReadResource(file, initData);
        if (resource)
            UseResidentSampleStorage(static_cast<SoundFontSample*>(resource->GetRawPointer()));
        return resource;
    }

  private:
    SOH::ResourceFactoryBinaryAudioSampleV2 nativeFactory;
};

void DropResources() {
    sSequences = {};
    sFonts = {};
    sResources.reset();
    sSequenceBase = sFontBase = -1;
    sPlaying = false;
    sDialectSeen = {};
    sSequenceExecuting = {};
    sDialectFailures.store(0);
}

// Use the ordinary audio command queue: the audio thread alone changes the
// sequence player. Its native sequence bytes, fonts and samples stay resident.
void StartForeignSequence(int player, int sequence, u8 nativeLabel) {
    gAudioContext.seqToPlay[player] = sequence;
    gAudioContext.seqReplaced[player] = 1;
    Audio_StartSequence(player, nativeLabel, 0, 0);
}
} // namespace

extern "C" void Combo_SOHInitWarpAudio() {
    DropResources();
    try {
        std::string archive;
        for (const auto* name : { "mm.o2r", "mm.otr", "mm.zip" }) {
            auto path = Ship::Context::LocateFileAcrossAppDirs(name, "2ship");
            if (std::filesystem::exists(path)) {
                archive = path;
                break;
            }
        }
        if (archive.empty() || fontMapSize + 3 >= 0xFF || sequenceMapSize + 18 > 0xFFFF)
            return;

        auto rm = std::make_shared<Ship::ResourceManager>();
        rm->Init({ archive }, {}, 1);
        auto loader = rm->GetResourceLoader();
        loader->RegisterResourceFactory(std::make_shared<ResidentMmSampleFactory>(), RESOURCE_FORMAT_BINARY,
                                        "AudioSample", static_cast<uint32_t>(SOH::ResourceType::SOH_AudioSample), 2);
        loader->RegisterResourceFactory(std::make_shared<SOH::ResourceFactoryBinaryAudioSequenceV2>(),
                                        RESOURCE_FORMAT_BINARY, "AudioSequence",
                                        static_cast<uint32_t>(SOH::ResourceType::SOH_AudioSequence), 2);
        loader->RegisterResourceFactory(std::make_shared<SOH::ResourceFactoryBinaryAudioSoundFontV2>(rm),
                                        RESOURCE_FORMAT_BINARY, "AudioSoundFont",
                                        static_cast<uint32_t>(SOH::ResourceType::SOH_AudioSoundFont), 2);

        std::array<SoundFont*, 3> fonts{};
        std::array<SequenceData*, 3> sequences{};
        std::array<std::string, 3> paths;
        for (size_t i = 0; i < fonts.size(); ++i) {
            paths[i] = "audio/fonts/Soundfont_" + std::to_string(kFonts[i]);
            fonts[i] = static_cast<SoundFont*>(rm->GetResourceRawPointer(paths[i].c_str()));
            if (!fonts[i] || fonts[i]->fntIndex != kFonts[i])
                return;
        }
        for (size_t i = 0; i < sequences.size(); ++i) {
            sequences[i] = static_cast<SequenceData*>(rm->GetResourceRawPointer(kSequences[i]));
            if (!sequences[i] || !sequences[i]->seqData || sequences[i]->numFonts < 0 || sequences[i]->numFonts > 16)
                return;
            for (int j = 0; j < sequences[i]->numFonts; ++j) {
                const int original = sequences[i]->fonts[j];
                size_t match = 0;
                while (match < fonts.size() && kFonts[match] != original)
                    ++match;
                if (match == fonts.size())
                    return;
                sequences[i]->fonts[j] = static_cast<u8>(fontMapSize + match);
            }
        }

        // Preserve native/custom slots, including the existing 15 spare slots, and skip ids
        // AudioCollection owns, as upstream does for custom sequences: font changes look the
        // playing id up in the Audio Editor, and a swapped instrument (130..135) has no sequence.
        size_t sequenceBase = sequenceMapSize + 15;
        while (sequenceBase + sequences.size() <= 0xFFFF &&
               (AudioCollection_HasSequenceNum(sequenceBase) || AudioCollection_HasSequenceNum(sequenceBase + 1) ||
                AudioCollection_HasSequenceNum(sequenceBase + 2)))
            ++sequenceBase;
        if (sequenceBase + sequences.size() > 0xFFFF)
            return;
        const size_t nextSequenceSize = sequenceBase + sequences.size();
        const size_t nextFontSize = fontMapSize + fonts.size();
        auto freeMemory = [](void* memory) { std::free(memory); };
        std::unique_ptr<char*, decltype(freeMemory)> newSequences(
            static_cast<char**>(std::calloc(nextSequenceSize + 15, sizeof(char*))), freeMemory);
        std::unique_ptr<char*, decltype(freeMemory)> newFonts(
            static_cast<char**>(std::calloc(nextFontSize, sizeof(char*))), freeMemory);
        std::unique_ptr<u8, decltype(freeMemory)> sequenceStatus(static_cast<u8*>(std::malloc(nextSequenceSize + 15)),
                                                                 freeMemory);
        std::unique_ptr<u8, decltype(freeMemory)> fontStatus(static_cast<u8*>(std::malloc(nextFontSize)), freeMemory);
        if (!newSequences || !newFonts || !sequenceStatus || !fontStatus)
            throw std::bad_alloc();
        std::memcpy(newSequences.get(), sequenceMap, (sequenceMapSize + 15) * sizeof(char*));
        std::memcpy(newFonts.get(), fontMap, fontMapSize * sizeof(char*));
        std::memset(sequenceStatus.get(), 5, nextSequenceSize + 15);
        std::memcpy(sequenceStatus.get(), gAudioContext.seqLoadStatus, sequenceMapSize + 15);
        std::memset(fontStatus.get(), 5, nextFontSize);
        std::memcpy(fontStatus.get(), gAudioContext.fontLoadStatus, fontMapSize);
        for (size_t i = 0; i < sequences.size(); ++i) {
            sequences[i]->seqNumber = static_cast<u16>(sequenceBase + i);
            newSequences.get()[sequenceBase + i] = strdup((std::string(kPrefix) + kSequences[i]).c_str());
        }
        for (size_t i = 0; i < fonts.size(); ++i) {
            fonts[i]->fntIndex = static_cast<int>(fontMapSize + i);
            newFonts.get()[fontMapSize + i] = strdup((std::string(kPrefix) + paths[i]).c_str());
        }
        sSequenceBase = static_cast<int>(sequenceBase);
        sFontBase = static_cast<int>(fontMapSize);
        std::free(sequenceMap);
        sequenceMap = newSequences.release();
        sequenceMapSize = nextSequenceSize;
        std::free(fontMap);
        fontMap = newFonts.release();
        fontMapSize = nextFontSize;
        std::free(gAudioContext.seqLoadStatus);
        gAudioContext.seqLoadStatus = sequenceStatus.release();
        std::free(gAudioContext.fontLoadStatus);
        gAudioContext.fontLoadStatus = fontStatus.release();
        sResources = rm;
        sSequences = sequences;
        sFonts = fonts;
        for (size_t i = 0; i < sSfxState.size(); ++i) {
            auto& state = sSfxState[i];
            for (auto& byte : state)
                byte.store(0);
            StoreMmSfxFloat(i, 0, 1.0f);
            StoreMmSfxFloat(i, 4, 1.0f);
            state[9].store(0x40);
            state[11].store(0xFF);
            state[12].store(0xFF);
            state[13].store(0xFF);
        }
        sSfxState[13][12].store(0); // Native SFX_CHANNEL_OCARINA default.
        Ship::CrossRMRegistry::RegisterTeardownListener(DropResources);
        SPDLOG_INFO("[ComboShip] Native MM Soaring audio loaded (sequences {}, {}, {}; fonts {}, {}, {})",
                    sSequenceBase, sSequenceBase + 1, sSequenceBase + 2, sFontBase, sFontBase + 1, sFontBase + 2);
    } catch (const std::exception& error) {
        SPDLOG_ERROR("[ComboShip] MM Soaring audio unavailable: {}", error.what());
    }
}

extern "C" void* Combo_SOHWarpAudioResource(const char* path) {
    if (!path || std::strncmp(path, kPrefix, std::strlen(kPrefix)) || !sResources)
        return nullptr;
    return sResources->GetResourceRawPointer(path + std::strlen(kPrefix));
}

extern "C" void* Combo_SOHWarpAudioCache(int table, int id) {
    if (sSequenceBase < 0)
        return nullptr;
    if (table == SEQUENCE_TABLE && id >= sSequenceBase && id < sSequenceBase + static_cast<int>(sSequences.size()))
        return sSequences[id - sSequenceBase]->seqData;
    if (table == FONT_TABLE && id >= sFontBase && id < sFontBase + 3)
        return sFonts[id - sFontBase];
    return nullptr;
}

extern "C" void Combo_SOHSoaringAudioBegin() {
    if (sSequenceBase < 0 || sPlaying)
        return;
    sPlaying = true;
    sPreviousAudioErrors = gAudioContext.audioErrorFlags;
    SPDLOG_INFO("[ComboShip] Native Soaring audio prior errors {:#x}", sPreviousAudioErrors);
    sPreviousMain = func_800FA0B4(SEQ_PLAYER_BGM_MAIN);
    sPreviousSub = func_800FA0B4(SEQ_PLAYER_BGM_SUB);
    Audio_SetSequenceMode(SEQ_MODE_IGNORE);
    StartForeignSequence(SEQ_PLAYER_BGM_MAIN, sSequenceBase, NA_BGM_MINI_BOSS);
    StartForeignSequence(SEQ_PLAYER_BGM_SUB, sSequenceBase + 1, NA_BGM_GENERAL_SFX);
    Audio_SetGameVolume(SEQ_PLAYER_BGM_SUB, CVarGetInteger(CVAR_SETTING("Volume.SFX"), 100) / 100.0f);
    // MM's General SFX native layout 0 assigns channels 0..2 to the player bank.
    Audio_QueueCmdS8(0x46000000 | (SEQ_PLAYER_BGM_SUB << 16), 0);
    SPDLOG_INFO("[ComboShip] Native Soaring audio begin");
}

extern "C" void Combo_SOHSoaringSound(int phase) {
    if (!sPlaying || phase < 0 || phase > 3)
        return;
    // ovl_En_Test7's four native wing cues; retain the General SFX score rather
    // than replacing their layers, envelopes or timing with unrelated sounds.
    const u16 sfxId = 0x9BB + phase;
    const u32 channel = static_cast<u32>(phase % 3);
    const u32 target = (SEQ_PLAYER_BGM_SUB << 16) | (channel << 8);
    StoreMmSfxFloat(channel, 0, 1.0f);
    StoreMmSfxFloat(channel, 4, 1.0f);
    sSfxState[channel][10].store(0);
    sSfxState[channel][9].store(0x40);
    Audio_QueueCmdF32(0x01000000 | target, 1.0f);
    Audio_QueueCmdF32(0x04000000 | target, 1.0f);
    Audio_QueueCmdS8(0x03000000 | target, 0x40);
    Audio_QueueCmdS8(0x06000000 | target | 0, 1);
    Audio_QueueCmdS8(0x06000000 | target | 4, sfxId & 0xFF);
    Audio_QueueCmdS8(0x06000000 | target | 5, ((sfxId & 0x300) >> 7) + ((sfxId & 0xFF) >> 7));
    SPDLOG_INFO("[ComboShip] Native Soaring wing cue {:#x} (channel {})", sfxId, channel);
}

extern "C" void Combo_SOHSoaringFanfare() {
    if (sSequenceBase < 0)
        return;
    // Preserve the native fanfare player, BGM ducking and restoration queue.
    gAudioContext.seqToPlay[SEQ_PLAYER_FANFARE] = sSequenceBase + 2;
    gAudioContext.seqReplaced[SEQ_PLAYER_FANFARE] = 1;
    Audio_PlayFanfare(NA_BGM_OCA_LIGHT);
    // Audio_PlayFanfareWithPlayerIOPort7: human Link's native MM ocarina.
    Audio_QueueSeqCmd(0x70000000 | (SEQ_PLAYER_FANFARE << 24) | (7 << 16) | 0x35);
    Audio_SetSfxBanksMute(0x20);
    SPDLOG_INFO("[ComboShip] Native MM Soaring playback fanfare");
}

extern "C" int Combo_SOHIsMmAudioSequence(void* rawPlayer) {
    auto* player = static_cast<SequencePlayer*>(rawPlayer);
    if (!player)
        return 0;
    for (const auto* sequence : sSequences)
        if (sequence && reinterpret_cast<u8*>(sequence->seqData) == player->seqData)
            return 1;
    return 0;
}
extern "C" int Combo_SOHMmAudioLayerCommand(void* rawLayer, int command) {
    auto* layer = static_cast<SequenceLayer*>(rawLayer);
    if (!layer || !layer->comboMmDialect)
        return 0;
    auto* state = &layer->scriptState;
    if (command == 0xF0) {
        const u16 mask = AudioSeq_ScriptReadS16(state);
        layer->comboMmFlags.asByte &= (mask ^ 0xFFFF);
        return 1;
    }
    if (command == 0xF1) {
        layer->comboMmSurroundEffectIndex = AudioSeq_ScriptReadU8(state);
        return 1;
    }
    return 0;
}
extern "C" int Combo_SOHMmAudioChannelCommand(void* rawChannel, int command) {
    auto* channel = static_cast<SequenceChannel*>(rawChannel);
    bool foreign = false;
    for (size_t index = 0; index < sSequences.size(); ++index) {
        const auto* sequence = sSequences[index];
        if (sequence && reinterpret_cast<u8*>(sequence->seqData) == channel->seqPlayer->seqData) {
            foreign = true;
            if (!sSequenceExecuting[index]) {
                sSequenceExecuting[index] = true;
                SPDLOG_INFO("[ComboShip] Native MM score executing {} (player {})", kSequences[index],
                            channel->seqPlayer->playerIdx);
            }
        }
    }
    if (!foreign || !((command >= 0xA0 && command <= 0xA8) || command == 0xBD || command == 0xBE))
        return 0;
    if (!sDialectSeen[command]) {
        sDialectSeen[command] = true;
        SPDLOG_INFO("[ComboShip] MM audio dialect command {:#x}", command);
    }

    // These are the MM-only channel instructions from AudioScript_SequenceChannelProcessScript.
    // Dispatch before OoT decodes them: MM BD is one s16, while OoT BD takes
    // two s16 arguments for random range (moved to A8 in MM).
    auto* state = &channel->scriptState;
    size_t channelIndex = 0;
    while (channelIndex < 16 && channel->seqPlayer->channels[channelIndex] != channel)
        ++channelIndex;
    if (channelIndex == 16)
        return 0;
    const bool sfx = sSequences[1] && channel->seqPlayer->seqData == reinterpret_cast<u8*>(sSequences[1]->seqData);
    switch (command) {
        case 0xA0:
        case 0xA1:
        case 0xA2:
        case 0xA3: {
            const u16 index = (command == 0xA0 || command == 0xA2) ? AudioSeq_ScriptReadS16(state) : channel->unk_22;
            if (sfx && index < 16) {
                if (command == 0xA0 || command == 0xA1)
                    state->value = sSfxState[channelIndex][index].load();
                else
                    sSfxState[channelIndex][index].store(state->value);
            }
            break;
        }
        case 0xA4: {
            const u8 surround = AudioSeq_ScriptReadU8(state);
            // The imported native scores do not request a surround override.
            // OoT's Android mixer uses the standard stereo pan supplied by SFX.
            if (surround != 0xFF && !(sDialectFailures.fetch_or(1) & 1))
                SPDLOG_ERROR("[ComboShip] MM warp audio validation FAILED: unsupported surround {}", surround);
            break;
        }
        case 0xA5:
            state->value += channelIndex;
            break;
        case 0xA6: {
            const u8 value = AudioSeq_ScriptReadU8(state);
            const u16 offset = AudioSeq_ScriptReadS16(state);
            channel->seqPlayer->seqData[offset + channelIndex] = static_cast<u8>(state->value) + value;
            break;
        }
        case 0xA7: {
            const u8 flags = AudioSeq_ScriptReadU8(state);
            s32 value = (flags & 0x80) ? (state->value >> (flags & 0xF)) : (state->value << (flags & 0xF));
            if (flags & 0x40)
                value = (value & static_cast<u8>(~0x80)) | (state->value & 0x80);
            state->value = value;
            break;
        }
        case 0xA8: {
            const u16 range = AudioSeq_ScriptReadS16(state);
            const u16 offset = AudioSeq_ScriptReadS16(state);
            const u32 random = Audio_NextRandom();
            channel->unk_22 = (range == 0 ? random & 0xFFFF : random % range) + offset;
            channel->unk_22 = (((channel->unk_22 / 0x100) + 0x80) << 8) | (channel->unk_22 % 0x100);
            break;
        }
        case 0xBD: {
            const u16 start = AudioSeq_ScriptReadS16(state);
            // Native wing cues start at sample zero, which is OoT's default.
            if (start != 0 && !(sDialectFailures.fetch_or(2) & 2))
                SPDLOG_ERROR("[ComboShip] MM warp audio validation FAILED: unsupported sample start {}", start);
            break;
        }
        case 0xBE: {
            const u8 function = AudioSeq_ScriptReadU8(state);
            // MM registers AudioSfx_SetFreqAndStereoBits in slot zero.
            if (function == 0) {
                const u8 selected = static_cast<u8>(state->value);
                if (selected < sSfxState.size()) {
                    channel->stereo.asByte = sSfxState[selected][10].load();
                    channel->freqScale = LoadMmSfxFrequency(selected);
                    channel->changes.s.freqScale = true;
                    state->value = selected; // Native AudioSfx_SetFreqAndStereoBits returns its input.
                }
            }
            break;
        }
    }
    return 1;
}

extern "C" void Combo_SOHTraceMmAudioLayer(void* rawLayer, int command, int missingDrum) {
    auto* layer = static_cast<SequenceLayer*>(rawLayer);
    if (!layer || !layer->channel || !layer->channel->seqPlayer)
        return;
    const auto* player = layer->channel->seqPlayer;
    for (const auto* sequence : sSequences) {
        if (!sequence || reinterpret_cast<u8*>(sequence->seqData) != player->seqData)
            continue;
        const auto offset = layer->scriptState.pc - player->seqData;
        if (missingDrum) {
            SPDLOG_ERROR("[ComboShip] Native MM score missing drum {} font {} pc {} layer instrument {} channel "
                         "instrument {} transpose {}/{}",
                         command, layer->channel->fontId, offset, layer->instOrWave, layer->channel->instOrWave,
                         layer->transposition, layer->channel->transposition);
        } else if ((command == 0xF0 || command == 0xF1) && !sDialectSeen[command]) {
            sDialectSeen[command] = true;
            SPDLOG_WARN("[ComboShip] Native MM layer dialect command {:#x} pc {}", command, offset);
        }
        return;
    }
}

extern "C" void Combo_SOHSoaringAudioEnd() {
    if (!sPlaying)
        return;
    sPlaying = false;
    SPDLOG_INFO("[ComboShip] Native Soaring audio end (audio errors {:#x}, dialect failures {:#x})",
                gAudioContext.audioErrorFlags, sDialectFailures.load());
    Audio_SetGameVolume(SEQ_PLAYER_BGM_SUB, CVarGetInteger(CVAR_SETTING("Volume.SubMusic"), 100) / 100.0f);
    for (const auto& entry :
         { std::pair{ SEQ_PLAYER_BGM_MAIN, sPreviousMain }, std::pair{ SEQ_PLAYER_BGM_SUB, sPreviousSub } }) {
        Audio_QueueSeqCmd((entry.second == NA_BGM_DISABLED ? 0x10000000 : 0) | (entry.first << 24) |
                          (entry.second == NA_BGM_DISABLED ? 0 : entry.second));
    }
}
#endif
