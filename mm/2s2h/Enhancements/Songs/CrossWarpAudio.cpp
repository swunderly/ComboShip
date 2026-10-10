#ifdef COMBO_BUILD
#include "CrossWarpAudio.h"
#include "global.h"
#include "BenPort.h"
#include "2s2h/resource/importer/AudioSampleFactory.h"
#include "2s2h/resource/importer/AudioSequenceFactory.h"
#include "2s2h/resource/importer/AudioSoundFontFactory.h"
#include "2s2h/resource/type/2shResourceType.h"
#include <ship/Context.h>
#include <ship/resource/CrossRMRegistry.h>
#include <ship/resource/ResourceManager.h>
#include <array>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>
#include <spdlog/spdlog.h>

extern "C" {
extern char** gSequenceMap;
extern size_t gSequenceMapSize;
extern char** gFontMap;
extern size_t gFontMapSize;
}

namespace {
constexpr const char* kPrefix = "@oot-audio:";
constexpr const char* kFont = "audio/fonts/18_Warp_Songs";
constexpr const char* kSequences[] = {
    "audio/sequences/052_Minuet_of_Forest", "audio/sequences/051_Bolero_of_Fire",
    "audio/sequences/053_Serenade_of_Water", "audio/sequences/054_Requiem_of_Spirit",
    "audio/sequences/055_Nocturne_of_Shadow", "audio/sequences/037_Prelude_of_Light",
};
std::shared_ptr<Ship::ResourceManager> sResources;
std::array<SequenceData*, 6> sSequences{};
std::array<bool, 6> sScoresExecuted{};
SoundFont* sFont = nullptr;
int sSequenceBase = -1, sFontId = -1;

void DropResources() {
    sSequences = {};
    sScoresExecuted = {};
    sFont = nullptr;
    sResources.reset();
    sSequenceBase = sFontId = -1;
}
}

extern "C" void Combo_MMInitWarpAudio() {
    DropResources();
    try {
        std::string archive;
        for (const auto* name : { "oot.o2r", "oot.otr", "oot.zip" }) {
            const auto path = Ship::Context::LocateFileAcrossAppDirs(name, "soh");
            if (std::filesystem::exists(path)) { archive = path; break; }
        }
        if (archive.empty() || gFontMapSize >= 0xFE || gSequenceMapSize + 6 >= 0x7FF) return;
        auto rm = std::make_shared<Ship::ResourceManager>();
        rm->Init({ archive }, {}, 1);
        const auto loader = rm->GetResourceLoader();
        loader->RegisterResourceFactory(std::make_shared<SOH::ResourceFactoryBinaryAudioSampleV2>(),
            RESOURCE_FORMAT_BINARY, "AudioSample", static_cast<uint32_t>(SOH::ResourceType::SOH_AudioSample), 2);
        loader->RegisterResourceFactory(std::make_shared<SOH::ResourceFactoryBinaryAudioSequenceV2>(),
            RESOURCE_FORMAT_BINARY, "AudioSequence", static_cast<uint32_t>(SOH::ResourceType::SOH_AudioSequence), 2);
        loader->RegisterResourceFactory(std::make_shared<SOH::ResourceFactoryBinaryAudioSoundFontV2>(rm),
            RESOURCE_FORMAT_BINARY, "AudioSoundFont", static_cast<uint32_t>(SOH::ResourceType::SOH_AudioSoundFont), 2);

        auto* font = static_cast<SoundFont*>(rm->GetResourceRawPointer(kFont));
        if (!font || font->fntIndex != 18) return;
        std::array<SequenceData*, 6> sequences{};
        for (size_t i = 0; i < sequences.size(); ++i) {
            sequences[i] = static_cast<SequenceData*>(rm->GetResourceRawPointer(kSequences[i]));
            if (!sequences[i] || !sequences[i]->seqData || sequences[i]->numFonts != 1 ||
                sequences[i]->fonts[0] != 18) return;
            sequences[i]->fonts[0] = static_cast<u8>(gFontMapSize);
        }

        const size_t nextSequenceSize = gSequenceMapSize + sequences.size();
        const size_t nextFontSize = gFontMapSize + 1;
        auto freeMemory = [](void* memory) { std::free(memory); };
        std::unique_ptr<char*, decltype(freeMemory)> newSequences(
            static_cast<char**>(std::calloc(nextSequenceSize, sizeof(char*))), freeMemory);
        std::unique_ptr<char*, decltype(freeMemory)> newFonts(
            static_cast<char**>(std::calloc(nextFontSize, sizeof(char*))), freeMemory);
        std::unique_ptr<u8, decltype(freeMemory)> sequenceStatus(
            static_cast<u8*>(std::malloc(nextSequenceSize)), freeMemory);
        std::unique_ptr<u8, decltype(freeMemory)> fontStatus(
            static_cast<u8*>(std::malloc(nextFontSize)), freeMemory);
        if (!newSequences || !newFonts || !sequenceStatus || !fontStatus) throw std::bad_alloc();
        std::memcpy(newSequences.get(), gSequenceMap, gSequenceMapSize * sizeof(char*));
        std::memcpy(newFonts.get(), gFontMap, gFontMapSize * sizeof(char*));
        std::memset(sequenceStatus.get(), LOAD_STATUS_PERMANENT, nextSequenceSize);
        std::memcpy(sequenceStatus.get(), gAudioCtx.seqLoadStatus, gSequenceMapSize);
        std::memset(fontStatus.get(), LOAD_STATUS_PERMANENT, nextFontSize);
        std::memcpy(fontStatus.get(), gAudioCtx.fontLoadStatus, gFontMapSize);
        for (size_t i = 0; i < sequences.size(); ++i) {
            sequences[i]->seqNumber = static_cast<u16>(gSequenceMapSize + i);
            newSequences.get()[gSequenceMapSize + i] = strdup((std::string(kPrefix) + kSequences[i]).c_str());
        }
        font->fntIndex = static_cast<int>(gFontMapSize);
        newFonts.get()[gFontMapSize] = strdup((std::string(kPrefix) + kFont).c_str());
        sSequenceBase = static_cast<int>(gSequenceMapSize);
        sFontId = static_cast<int>(gFontMapSize);
        std::free(gSequenceMap); gSequenceMap = newSequences.release(); gSequenceMapSize = nextSequenceSize;
        std::free(gFontMap); gFontMap = newFonts.release(); gFontMapSize = nextFontSize;
        std::free(gAudioCtx.seqLoadStatus); gAudioCtx.seqLoadStatus = sequenceStatus.release();
        std::free(gAudioCtx.fontLoadStatus); gAudioCtx.fontLoadStatus = fontStatus.release();
        sResources = rm; sSequences = sequences; sFont = font;
        Ship::CrossRMRegistry::RegisterTeardownListener(DropResources);
        SPDLOG_INFO("[ComboShip] Native OoT warp fanfares loaded (sequences {}..{}; font {})",
                    sSequenceBase, sSequenceBase + 5, sFontId);
    } catch (const std::exception& error) {
        SPDLOG_ERROR("[ComboShip] OoT warp fanfares unavailable: {}", error.what());
    }
}

extern "C" void* Combo_MMWarpAudioResource(const char* path) {
    if (!path || std::strncmp(path, kPrefix, std::strlen(kPrefix)) || !sResources) return nullptr;
    return sResources->GetResourceRawPointer(path + std::strlen(kPrefix));
}

extern "C" int Combo_MMIsOotWarpSequence(int id) {
    return sSequenceBase >= 0 && id >= sSequenceBase && id < sSequenceBase + 6;
}

extern "C" void Combo_MMTraceWarpSequence(int id, const void* score) {
    if (!Combo_MMIsOotWarpSequence(id)) return;
    const int song = id - sSequenceBase;
    if (sScoresExecuted[song] || score != sSequences[song]->seqData) return;
    sScoresExecuted[song] = true;
    SPDLOG_INFO("[ComboShip] Executing native OoT warp score {} (sequence {})", kSequences[song], id);
}

extern "C" void* Combo_MMWarpAudioCache(int table, int id) {
    if (table == SEQUENCE_TABLE && Combo_MMIsOotWarpSequence(id)) return sSequences[id - sSequenceBase]->seqData;
    if (table == FONT_TABLE && id == sFontId && sFontId >= 0) return sFont;
    return nullptr;
}

extern "C" void Combo_MMPlayWarpFanfare(int song) {
    if (sSequenceBase < 0 || song < 0 || song >= 6) return;
    // MM's ordinary fanfare path retains BGM ducking and restores it when the
    // native score finishes. Only this private sequence and its font differ.
    Audio_PlayFanfare(static_cast<u16>(sSequenceBase + song));
    AudioSfx_MuteBanks(0x20);
}
#endif
