#include "global.h"
#include "battle.h"
#include "battle_factory.h"
#include "bg.h"
#include "event_data.h"
#include "field_message_box.h"
#include "frontier_util.h"
#include "load_save.h"
#include "main.h"
#include "menu.h"
#include "new_game.h"
#include "palette.h"
#include "pokemon.h"
#include "polished_tower.h"
#include "save.h"
#include "script.h"
#include "string_util.h"
#include "task.h"
#include "constants/battle_factory.h"
#include "constants/battle_frontier.h"
#include "constants/frontier_util.h"

extern const u8 BattleFrontier_BattleFactoryBattleRoom_EventScript_DefeatedOpponent[];
extern const u8 BattleFrontier_BattleFactoryBattleRoom_EventScript_DefeatedNoland[];
extern const u8 BattleFrontier_BattleFactoryLobby_EventScript_WonChallenge[];
extern const u8 BattleFrontier_BattleFactoryLobby_EventScript_LostChallenge[];
extern const u8 BattleFrontier_BattleFactoryLobby_EventScript_QuitWithoutSaving[];

static const u8 sAwards[][7] = {
/* SOURCE AWARDS */
};
enum {BOOT, READY, VICTORY, RELOADED, LOBBY, PAUSED};
EWRAM_DATA volatile u32 gSettlement[15] = {0};

static void Check(u32 actual, u32 expected, u32 field)
{
    if (actual != expected && !gSettlement[1])
    {
        gSettlement[1] = field;
        gSettlement[2] = actual;
        gSettlement[3] = expected;
    }
}

static void GivePoints(void)
{
    gSpecialVar_0x8004 = FRONTIER_UTIL_FUNC_GIVE_BATTLE_POINTS;
    CallFrontierUtilFunc();
}

static void FactorySave(u32 status)
{
    gSpecialVar_0x8004 = BATTLE_FACTORY_FUNC_SAVE;
    gSpecialVar_0x8005 = status;
    CallBattleFactoryFunction();
}

static void CheckAwards(void)
{
    struct BattleFrontier *f = &gSaveBlock2Ptr->frontier;
    for (u32 round = 0; round < ARRAY_COUNT(sAwards); round++)
    {
        u32 total = 0;
        for (u32 n = 0; n <= 7; n++)
        {
            if (n)
                total += sAwards[round][n - 1];
            f->challengeStatus = n == 7 ? CHALLENGE_STATUS_WON : CHALLENGE_STATUS_LOST;
            f->curChallengeBattleNum = n;
            f->factoryWinStreaks[0][0] = round * 7 + n;
            f->factoryRecordWinStreaks[0][0] = 0;
            f->battlePoints = f->cardBattlePoints = 0;
            GivePoints();
            Check(f->battlePoints, total, 1);
            Check(f->cardBattlePoints, total, 2);
            Check(f->factoryWinStreaks[0][0], n == 7 ? round * 7 + n : 0, 3);
            Check(f->factoryRecordWinStreaks[0][0], round * 7 + n, 4);
            GivePoints();
            Check(f->cardBattlePoints, total, 5);
            gSettlement[4]++;
        }
    }
    f->challengeStatus = CHALLENGE_STATUS_WON;
    f->curChallengeBattleNum = 7;
    f->factoryWinStreaks[0][0] = 512 + 200;
    f->factoryRecordWinStreaks[0][0] = 511;
    f->factoryRentsCount[0][0] = 104;
    GivePoints();
    Check(f->factoryRentsCount[0][0], 200, 6);
    f->challengeStatus = CHALLENGE_STATUS_LOST;
    f->curChallengeBattleNum = 3;
    f->factoryWinStreaks[0][0] = 250;
    GivePoints();
    Check(f->factoryRecordWinStreaks[0][0], 512 + 250, 7);
    Check(f->factoryRentsCount[0][0], 0, 8);
    f->lvlMode = FRONTIER_LVL_OPEN;
    f->challengeStatus = CHALLENGE_STATUS_SAVING;
    f->curChallengeBattleNum = 2;
    u32 balance = f->battlePoints;
    TryRecordPolishedFactoryWin();
    PolishedForfeitInterruptedFactorySet();
    Check(f->battlePoints, balance, 9);
    Check(f->curChallengeBattleNum, 2, 10);
    Check(f->challengeStatus, CHALLENGE_STATUS_SAVING, 11);
    f->lvlMode = FRONTIER_LVL_50;
    VarSet(VAR_FRONTIER_BATTLE_MODE, FRONTIER_MODE_DOUBLES);
    TryRecordPolishedFactoryWin();
    PolishedSettleFactoryPoints();
    Check(f->battlePoints, balance, 12);
    Check(gSpecialVar_Result, 0, 13);
    VarSet(VAR_FRONTIER_BATTLE_MODE, FRONTIER_MODE_SINGLES);
}

static void StartScript(const u8 *script, u32 phase)
{
    ScriptContext_SetupScript(script);
    gSettlement[0] = phase;
}

static void StartChallenge(u16 keys)
{
    struct BattleFrontier *f = &gSaveBlock2Ptr->frontier;
    // Six profiles select the ordinary, Head, reset, pause, wrap and long-streak cases.
    u16 opening = keys & B_BUTTON ? 14 : keys & DPAD_RIGHT ? 255 : keys & DPAD_LEFT ? 6 :
                  keys & DPAD_UP ? 65534 : keys & DPAD_DOWN ? 9996 : 0;
    u32 count = keys & (B_BUTTON | DPAD_DOWN) ? 7 : keys & A_BUTTON ? 4 : 2;
    gSpecialVar_0x8004 = BATTLE_FACTORY_FUNC_INIT;
    CallBattleFactoryFunction();
    f->factoryWinStreaks[0][0] = f->factoryRecordWinStreaks[0][0] = opening;
    f->factoryRentsCount[0][0] = f->factoryRecordRentsCount[0][0] = 0;
    f->battlePoints = keys & DPAD_UP ? 49998 : 10;
    f->cardBattlePoints = keys & DPAD_UP ? 65530 : 20;
    f->challengeStatus = CHALLENGE_STATUS_SAVING;
    f->winStreakActiveFlags |= STREAK_FACTORY_SINGLES_50;
    CreateMon(&gParties[B_TRAINER_PLAYER][0], SPECIES_CHIKORITA, 17, 123, OTID_STRUCT_PRESET(0));
    gPartiesCount[B_TRAINER_PLAYER] = 1;
    SavePlayerParty();
    Check(TrySavingData(SAVE_NORMAL), SAVE_STATUS_OK, 14);
    CreateMon(&gParties[B_TRAINER_PLAYER][0], SPECIES_POLIWRATH, 50, 456, OTID_STRUCT_PRESET(0));
    for (u32 n = 1; n < count; n++)
    {
        TryRecordPolishedFactoryWin();
        Check(gSpecialVar_Result, n, 15);
        Check(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_SPECIES), SPECIES_POLIWRATH, 16);
    }
    // A real swap before the final tested win counts exactly once.
    gSpecialVar_0x8004 = BATTLE_FACTORY_FUNC_SET_SWAPPED;
    CallBattleFactoryFunction();
    StartScript(keys & B_BUTTON ? BattleFrontier_BattleFactoryBattleRoom_EventScript_DefeatedNoland
                                : BattleFrontier_BattleFactoryBattleRoom_EventScript_DefeatedOpponent, VICTORY);
}

void CB2_MoveMachineAudit(void)
{
    static const struct BgTemplate bg = {.bg = 0, .charBaseIndex = 2, .mapBaseIndex = 31};
    static u16 tilemap[32 * 32];
    if (gSettlement[1])
        return;
    if (gSettlement[0] == BOOT)
    {
        SetSaveBlocksPointers(0);
        ResetMenuAndMonGlobals();
        Save_ResetSaveCounters();
        Sav2_ClearSetDefault();
        MoveSaveBlocks_ResetHeap();
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, &bg, 1);
        SetBgTilemapBuffer(0, tilemap);
        InitStandardTextBoxWindows();
        InitTextBoxGfxAndPrinters();
        InitFieldMessageBox();
        SetVBlankCallback(TransferPlttBuffer);
        ShowBg(0);
        if (LoadGameSave(SAVE_NORMAL) == SAVE_STATUS_OK)
        {
            LoadPlayerParty();
            Check(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_SPECIES), SPECIES_CHIKORITA, 17);
            Check(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_LEVEL), 17, 18);
            gSettlement[0] = RELOADED;
        }
        else
        {
            NewGameInitData();
            static const u8 name[] = {0xCC, 0xD9, 0xD8, EOS}; // Red
            StringCopy(gSaveBlock2Ptr->playerName, name);
            VarSet(VAR_FRONTIER_FACILITY, FRONTIER_FACILITY_FACTORY);
            VarSet(VAR_FRONTIER_BATTLE_MODE, FRONTIER_MODE_SINGLES);
            gSaveBlock2Ptr->frontier.lvlMode = FRONTIER_LVL_50;
            CheckAwards();
            gSettlement[0] = READY;
        }
    }
    struct BattleFrontier *f = &gSaveBlock2Ptr->frontier;
    u16 keys = gMain.newKeys;
    if (gSettlement[0] == READY && keys)
        StartChallenge(keys);
    else if (gSettlement[0] == VICTORY && (keys & START_BUTTON))
    {
        FactorySave(CHALLENGE_STATUS_PAUSED);
        gSettlement[0] = PAUSED;
    }
    else if (gSettlement[0] == RELOADED)
    {
        if (f->challengeStatus == CHALLENGE_STATUS_PAUSED && keys & START_BUTTON)
        {
            PolishedForfeitInterruptedFactorySet();
            PolishedSettleFactoryPoints();
            Check(f->challengeStatus, CHALLENGE_STATUS_PAUSED, 19);
            FactorySave(CHALLENGE_STATUS_SAVING);
            f->challengePaused = FALSE;
            StartScript(BattleFrontier_BattleFactoryBattleRoom_EventScript_DefeatedOpponent, VICTORY);
        }
        else if (keys & A_BUTTON)
        {
            f->challengeStatus = CHALLENGE_STATUS_LOST;
            StartScript(BattleFrontier_BattleFactoryLobby_EventScript_LostChallenge, LOBBY);
        }
        else if (keys & B_BUTTON)
            StartScript(BattleFrontier_BattleFactoryLobby_EventScript_QuitWithoutSaving, LOBBY);
        else if (keys & DPAD_RIGHT)
            StartScript(BattleFrontier_BattleFactoryLobby_EventScript_WonChallenge, LOBBY);
    }
    if (gSettlement[0] == VICTORY || gSettlement[0] == LOBBY)
    {
        ScriptContext_RunScript();
        RunTasks();
        DoScheduledBgTilemapCopiesToVram();
    }
    gSettlement[5] = f->challengeStatus;
    gSettlement[6] = f->curChallengeBattleNum;
    gSettlement[7] = f->battlePoints;
    gSettlement[8] = f->cardBattlePoints;
    gSettlement[9] = f->factoryWinStreaks[0][0];
    gSettlement[10] = f->factoryRecordWinStreaks[0][0];
    gSettlement[11] = f->factoryRentsCount[0][0];
    gSettlement[12] = !!(f->winStreakActiveFlags & STREAK_FACTORY_SINGLES_50);
    gSettlement[13] = f->challengePaused;
    gSettlement[14] = f->factoryRecordRentsCount[0][0];
}
