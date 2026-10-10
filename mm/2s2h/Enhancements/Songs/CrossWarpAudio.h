#pragma once
#ifdef __cplusplus
extern "C" {
#endif
void Combo_MMInitWarpAudio(void);
void* Combo_MMWarpAudioResource(const char* path);
void* Combo_MMWarpAudioCache(int table, int id);
int Combo_MMIsOotWarpSequence(int id);
void Combo_MMTraceWarpSequence(int id, const void* score);
void Combo_MMPlayWarpFanfare(int song);
#ifdef __cplusplus
}
#endif
