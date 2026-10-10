#pragma once
#ifdef __cplusplus
extern "C" {
#endif
void Combo_SOHInitWarpAudio(void);
void* Combo_SOHWarpAudioResource(const char* path);
void* Combo_SOHWarpAudioCache(int table, int id);
void Combo_SOHSoaringAudioBegin(void);
void Combo_SOHSoaringSound(int phase);
void Combo_SOHSoaringAudioEnd(void);
void Combo_SOHSoaringFanfare(void);
int Combo_SOHMmAudioChannelCommand(void* channel, int command);
int Combo_SOHIsMmAudioSequence(void* player);
int Combo_SOHMmAudioLayerCommand(void* layer, int command);
void Combo_SOHTraceMmAudioLayer(void* layer, int command, int missingDrum);
#ifdef __cplusplus
}
#endif
