#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
// JSON strings remain owned by the launcher until the next callback. The UI
// copies them immediately. No C++ objects or game callbacks cross this ABI.
typedef const char* (*ComboProfileInspect)(int slot);
typedef const char* (*ComboProfileSetDraft)(int slot,const char* settings);
typedef const char* (*ComboProfileCopySaved)(int source,int destination);
typedef const char* (*ComboProfileCopyDraft)(int source,int destination);
typedef const char* (*ComboProfileGenerate)(int slot,const char* seed);
typedef const char* (*ComboProfileCancel)(void);
// Version 2: constant-time change token; no JSON serialization or disk reads.
typedef uint64_t (*ComboProfileRevision)(void);
typedef struct ComboRandomizerProfileCallbacks {
 int version;
 ComboProfileInspect inspect;
 ComboProfileSetDraft setDraft;
 ComboProfileCopySaved copySaved;
 ComboProfileGenerate generate;
 ComboProfileCopyDraft copyDraft;
 ComboProfileCancel cancel;
 ComboProfileRevision revision;
} ComboRandomizerProfileCallbacks;
#ifdef __cplusplus
}
#endif
