#include "global.h"

#ifdef COMBO_BUILD
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/Enhancements/randomizer/randomizer_entrance.h" // Entrance_OverrideNextIndex
// Set by SOH_ResumeGame (OTRGlobals.cpp) on a combo MM->OOT return. >= 0 means: skip the title /
// file-select screens, load this save slot, and spawn straight into Play in the Market outside the
// Happy Mask Shop (the fixed arrival point for the cross-game portal return).
extern s32 gComboReturnFileNum;
extern int SOH_ConsumeSongArrival(void);
extern int SOH_ConsumeFaroreArrival(void);
extern void SOH_CancelEntranceFlight(void);
extern void SOH_CancelNativeDeparture(void);
extern void SOH_SetSongArrival(int);
extern int SOH_SetDungeonArrival(int, int, int);
extern int SOH_SetFaroreArrivalEntrance(int, int, int);
#endif

void TitleSetup_InitImpl(GameState* gameState) {
    osSyncPrintf("ゼルダ共通データ初期化\n"); // "Zelda common data initalization"
#ifndef COMBO_BUILD
    SaveContext_Init();
#endif
#ifdef COMBO_BUILD
    if (gComboReturnFileNum >= 0) {
        // Combo MM->OOT return: mirror FileChoose_LoadGame (z_file_choose.c) to load the OOT save and
        // jump directly to Play, then override the entrance so Link spawns in the Market outside the
        // Happy Mask Shop (fixed arrival for the cross-game portal return — symmetric with the OOT->MM
        // trigger of entering the shop).
        LUSLOG_INFO("[ComboShip] TitleSetup combo jump -> Play (fileNum=%d)", gComboReturnFileNum);
        const int attemptedFile = gComboReturnFileNum;
        if (!Sram_TryOpenSave(attemptedFile, 1)) {
            // No arrival consumer or load hook may apply to a rejected target.
            gComboReturnFileNum = -1;
            SOH_CancelEntranceFlight();
            SOH_CancelNativeDeparture();
            SOH_SetSongArrival(-1);
            SOH_SetDungeonArrival(-1, -1, -1);
            SOH_SetFaroreArrivalEntrance(-1, -1, -1);
            gameState->running = false;
            SET_NEXT_GAMESTATE(gameState, FileChoose_Init, FileChooseContext);
            return;
        }
        gSaveContext.gameMode = GAMEMODE_NORMAL;
        // Mirror FileChoose_LoadGame's magic staging so Play_Init refills the meter normally.
        gSaveContext.magicFillTarget = gSaveContext.magic;
        gSaveContext.magic = 0;
        gSaveContext.magicLevel = 0;
        // Fire the exit/load hook pair a real quit+load runs: Save_LoadFile (inside Sram_OpenSave)
        // recreated gRandoContext, and consumers like the check tracker must tear down + re-init.
        GameInteractor_ExecuteOnExitGame(gSaveContext.fileNum);
        GameInteractor_ExecuteOnLoadGame(gSaveContext.fileNum);
        // Must follow OnLoadGame: the rando handler's Entrance_SetSavewarpEntrance() recomputes from
        // savedSceneNum (never set by the portal handoff) and would clobber this with Link's House.
        extern int SOH_ConsumeDungeonArrival(void);
        extern int SOH_ConsumeEntranceArrival(void);
        const int entranceArrival=SOH_ConsumeEntranceArrival();
        const int dungeonArrival=SOH_ConsumeDungeonArrival();
        const int faroreArrival=SOH_ConsumeFaroreArrival();
        const int songArrival=SOH_ConsumeSongArrival();
        gSaveContext.entranceIndex = entranceArrival>=0 ? entranceArrival : dungeonArrival>=0 ? dungeonArrival : faroreArrival>=0 ? faroreArrival : songArrival>=0 ? songArrival : Entrance_OverrideNextIndex(ENTR_MARKET_DAY_OUTSIDE_HAPPY_MASK_SHOP);
        // A portal return is never a cutscene arrival: a never-played MM-start save still holds the
        // pending intro (0xFFF1), which would shift Play_Init's entrance lookup onto a garbage layer.
        gSaveContext.cutsceneIndex = 0;
        gComboReturnFileNum = -1;
        gameState->running = false;
        SET_NEXT_GAMESTATE(gameState, Play_Init, PlayState);
        return;
    }
#endif
#ifdef COMBO_BUILD
    SaveContext_Init(); // Ordinary boot, with no pending file load.
#endif
    gameState->running = false;
    SET_NEXT_GAMESTATE(gameState, Title_Init, TitleContext);
}

void TitleSetup_Destroy(GameState* gameState) {
}

void TitleSetup_Init(GameState* gameState) {
    gameState->destroy = TitleSetup_Destroy;
    TitleSetup_InitImpl(gameState);
}
