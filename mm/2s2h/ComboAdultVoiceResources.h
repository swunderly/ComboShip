#pragma once
#include "ComboAdultVoiceRecipe.h"
#include "ComboAdultVoiceSequence.h"
#include "2s2h/resource/importer/AudioSampleFactory.h"
#include "2s2h/resource/importer/AudioSequenceFactory.h"
#include "2s2h/resource/importer/AudioSoundFontFactory.h"
#include "2s2h/resource/type/2shResourceType.h"
#include <ship/Context.h>
#include <ship/resource/CrossRMRegistry.h>
#include <ship/resource/ResourceManager.h>
#include <filesystem>
extern "C" {
extern char** gFontMap;
extern size_t gFontMapSize;
extern char** gSequenceMap;
extern size_t gSequenceMapSize;
}
namespace ComboAdultMM {
static std::shared_ptr<Ship::ResourceManager> sAdultVoiceOotOwner, sAdultVoiceMmOwner;
static std::shared_ptr<SOH::AudioSoundFont> sAdultVoiceForeignFont, sAdultVoiceNativeFont;
static std::shared_ptr<SOH::AudioSequence> sAdultVoiceNativeSequence;
static std::vector<SoundEffect> sAdultVoiceEffects;
static SoundFont sAdultHybridFont{};
static SequenceData sAdultHybridSequence{};
static std::vector<uint8_t> sAdultVoiceSequenceBytes;
static bool sAdultVoiceListener = false;
static size_t sAdultVoiceBase = 0;
static void DropAdultVoiceResources() {
    sAdultHybridFont = {};
    sAdultHybridSequence = {};
    sAdultVoiceSequenceBytes.clear();
    sAdultVoiceEffects.clear();
    sAdultVoiceBase = 0;
    sAdultVoiceForeignFont.reset();
    sAdultVoiceNativeFont.reset();
    sAdultVoiceNativeSequence.reset();
    sAdultVoiceOotOwner.reset();
    sAdultVoiceMmOwner.reset();
}
// Loads user-owned OoT font0 through MM factories, preserving MM sample ABI and explicit owner.
// The hybrid remains private until the native loader eligibility gate accepts all model resources.
inline SoundFont* BuildAdultHybridFont() {
    auto mm = Ship::CrossRMRegistry::Get("mm");
    if (!mm || !gFontMapSize || !gFontMap || !gFontMap[0])
        return nullptr;
    if (sAdultVoiceMmOwner == mm && !sAdultVoiceEffects.empty())
        return &sAdultHybridFont;
    DropAdultVoiceResources();
    std::string archive;
    for (const char* name : { "oot.o2r", "oot.otr", "oot.zip" }) {
        auto path = Ship::Context::LocateFileAcrossAppDirs(name, "soh");
        if (std::filesystem::exists(path)) {
            archive = path;
            break;
        }
    }
    if (archive.empty())
        return nullptr;
    auto oot = std::make_shared<Ship::ResourceManager>();
    oot->Init({ archive }, {}, 1);
    auto loader = oot->GetResourceLoader();
    loader->RegisterResourceFactory(std::make_shared<SOH::ResourceFactoryBinaryAudioSampleV2>(), RESOURCE_FORMAT_BINARY,
                                    "AudioSample", static_cast<uint32_t>(SOH::ResourceType::SOH_AudioSample), 2);
    loader->RegisterResourceFactory(std::make_shared<SOH::ResourceFactoryBinaryAudioSequenceV2>(),
                                    RESOURCE_FORMAT_BINARY, "AudioSequence",
                                    static_cast<uint32_t>(SOH::ResourceType::SOH_AudioSequence), 2);
    loader->RegisterResourceFactory(std::make_shared<SOH::ResourceFactoryBinaryAudioSoundFontV2>(oot),
                                    RESOURCE_FORMAT_BINARY, "AudioSoundFont",
                                    static_cast<uint32_t>(SOH::ResourceType::SOH_AudioSoundFont), 2);
    auto foreignResource =
        std::dynamic_pointer_cast<SOH::AudioSoundFont>(oot->LoadResourceProcess("audio/fonts/00_Sound_Effects_1"));
    auto ownResource = std::dynamic_pointer_cast<SOH::AudioSoundFont>(mm->LoadResourceProcess(gFontMap[0]));
    if (!foreignResource || !ownResource)
        return nullptr;
    auto* foreign = (SoundFont*)foreignResource->GetRawPointer();
    auto* own = (SoundFont*)ownResource->GetRawPointer();
    if (!own || !foreign || own->fntIndex != 0 || foreign->fntIndex != 0)
        return nullptr;
    if (!BuildAdultVoiceEffects(own->soundEffects, own->numSfx, foreign->soundEffects, foreign->numSfx,
                                sAdultVoiceEffects, sAdultVoiceBase))
        return nullptr;
    sAdultHybridFont = *own;
    sAdultHybridFont.soundEffects = sAdultVoiceEffects.data();
    sAdultHybridFont.numSfx = sAdultVoiceEffects.size();
    // Sample pointers are already resolved by native importers; both owning caches outlive font use.
    sAdultVoiceForeignFont = std::move(foreignResource);
    sAdultVoiceNativeFont = std::move(ownResource);
    sAdultVoiceOotOwner = oot;
    sAdultVoiceMmOwner = mm;
    if (!sAdultVoiceListener) {
        Ship::CrossRMRegistry::RegisterTeardownListener(DropAdultVoiceResources);
        sAdultVoiceListener = true;
    }
    return &sAdultHybridFont;
}
inline SequenceData* BuildAdultHybridSequence() {
    if (!BuildAdultHybridFont() || !gSequenceMap || !gSequenceMapSize || !gSequenceMap[0])
        return nullptr;
    if (!sAdultVoiceSequenceBytes.empty())
        return &sAdultHybridSequence;
    auto resource =
        std::dynamic_pointer_cast<SOH::AudioSequence>(sAdultVoiceMmOwner->LoadResourceProcess(gSequenceMap[0]));
    if (!resource)
        return nullptr;
    auto* original = (SequenceData*)resource->GetRawPointer();
    if (!original || original->seqNumber != 0 || original->seqDataSize <= 0)
        return nullptr;
    if (!PatchAdultVoiceSequence((const uint8_t*)original->seqData, original->seqDataSize, sAdultVoiceBase,
                                 sAdultVoiceSequenceBytes))
        return nullptr;
    sAdultVoiceNativeSequence = std::move(resource);
    sAdultHybridSequence = *original;
    sAdultHybridSequence.seqData = (char*)sAdultVoiceSequenceBytes.data();
    sAdultHybridSequence.seqDataSize = (int32_t)sAdultVoiceSequenceBytes.size();
    return &sAdultHybridSequence;
}
// Pinned PlayerVoice routing: native typed seqPlayers[2], seqId0, human only.
// Restrict writes to the exact private sequence pointer; never patch another active sequence.
inline void UpdateAdultVoiceRouting(Player* player, bool adult) {
    if (!player || player->transformation != PLAYER_FORM_HUMAN || sAdultVoiceSequenceBytes.empty())
        return;
    auto& sequence = gAudioCtx.seqPlayers[2];
    if (sequence.seqId != 0 || sequence.seqData != sAdultVoiceSequenceBytes.data())
        return;
    size_t table = sAdultVoiceSequenceBytes.size() - 0x80 + (adult ? 0x40 : 0);
    auto* dst = sequence.seqData + 0xB34E;
    auto* src = sequence.seqData + table;
    if (memcmp(dst, src, 0x40))
        memcpy(dst, src, 0x40);
}
} // namespace ComboAdultMM
extern "C" SoundFont* Combo_MM_AdultHybridFont(void) {
    return ComboAdultMM::BuildAdultHybridFont();
}
