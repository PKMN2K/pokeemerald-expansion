#include "global.h"
#include "battle.h"
#include "battle_factory.h"
#include "battle_frontier.h"
#include "battle_main.h"
#include "battle_setup.h"
#include "bg.h"
#include "field_message_box.h"
#include "gpu_regs.h"
#include "menu.h"
#include "palette.h"
#include "script.h"
#include "task.h"
#include "window.h"
#include "string_util.h"
#include "constants/characters.h"
#include "constants/trainers.h"
#include "constants/battle_frontier_mons.h"
#include "constants/battle_factory.h"
#include "constants/battle_frontier.h"
#include "constants/frontier_util.h"
#include "data.h"
#include "event_data.h"
#include "load_save.h"
#include "main.h"
#include "move.h"
#include "new_game.h"
#include "pokemon.h"
#include "polished_tower.h"
#include "random.h"
#include "save.h"

static const struct {
    u16 species, item, moves[4];
    u32 iv;
    u8 gender, nature, slot;
} sSets[] = {
/* SOURCE SETS */
};
static const struct {u16 wins, swaps, team[PARTY_SIZE];} sDrafts[] = {
/* DRAFT CASES */
};
static const struct {u16 opening, team[3]; u8 hint, type;} sOpponents[] = {
/* OPPONENT CASES */
};
static const u8 sScoutText[][100] = {
/* SCOUT TEXT */
};
EWRAM_DATA volatile u32 gCheckedOpponents = 0;
EWRAM_DATA volatile u32 gRentalAudit[8] = {0};
const u32 gRentalStride = sizeof(struct RentalMon);

static void Check(u32 actual, u32 expected, u32 field)
{
    if (actual != expected && !gRentalAudit[1])
    {
        gRentalAudit[1] = field;
        gRentalAudit[2] = actual;
        gRentalAudit[3] = expected;
    }
}

static void Factory(u32 function)
{
    gSpecialVar_0x8004 = function;
    CallBattleFactoryFunction();
}

static void CheckStreakCarry(void)
{
    static const struct {
        u16 oldRecord, wins, factory, expected;
        bool8 active, expectedActive;
    } cases[] = {
        {255, 259, 0, 768, FALSE, TRUE},
        {255, 259, 0x011c, 0x031c, TRUE, TRUE},
        {255, 259, 0x041c, 0x041c, TRUE, TRUE},
        {255, 256, 0, 0, FALSE, FALSE},
        {257, 3, 17, 17, TRUE, TRUE},
        {255, 259, 0x0509, 0x0300, FALSE, TRUE}, // An inactive native score is the previous run.
    };
    struct BattleFrontier *f = &gSaveBlock2Ptr->frontier;
    VarSet(VAR_FRONTIER_FACILITY, FRONTIER_FACILITY_TOWER);
    for (u32 i = 0; i < ARRAY_COUNT(cases); i++)
    {
        f->towerRecordWinStreaks[0][0] = cases[i].oldRecord;
        f->towerWinStreaks[0][0] = cases[i].wins;
        f->factoryWinStreaks[0][0] = cases[i].factory;
        f->factoryWinStreaks[0][1] = 912;
        f->factoryRentsCount[0][0] = 4;
        f->winStreakActiveFlags = cases[i].active ? STREAK_FACTORY_SINGLES_50 : 0;
        f->challengeStatus = CHALLENGE_STATUS_LOST;
        PolishedSettleTowerPoints();
        Check(f->factoryWinStreaks[0][0], cases[i].expected, 1);
        Check(!!(f->winStreakActiveFlags & STREAK_FACTORY_SINGLES_50), cases[i].expectedActive, 2);
        Check(f->factoryWinStreaks[0][1], 912, 3);
        Check(f->factoryRentsCount[0][0], !cases[i].active && cases[i].expectedActive ? 0 : 4, 4);
        gRentalAudit[5]++;
    }
    VarSet(VAR_FRONTIER_FACILITY, FRONTIER_FACILITY_FACTORY);
    Factory(BATTLE_FACTORY_FUNC_INIT);
    Check(f->factoryWinStreaks[0][0], 768, 5);
}

static void Draw(u32 index)
{
    struct BattleFrontier *f = &gSaveBlock2Ptr->frontier;
    f->factoryWinStreaks[0][0] = sDrafts[index].wins;
    f->factoryRentsCount[0][0] = sDrafts[index].swaps;
    u16 ime = REG_IME;
    REG_IME = 0;
    SeedRng(index + 1);
    Factory(BATTLE_FACTORY_FUNC_GENERATE_RENTAL_MONS);
    REG_IME = ime;
    for (u32 i = 0; i < PARTY_SIZE; i++)
        Check(f->rentalMons[i].monId, 0x8000 + sDrafts[index].team[i], 6);
}

static void CheckMon(struct Pokemon *mon, u16 id)
{
    Check(GetMonData(mon, MON_DATA_SPECIES), sSets[id].species, 7);
    Check(GetMonData(mon, MON_DATA_IVS), sSets[id].iv, 8);
    Check(GetMonData(mon, MON_DATA_LEVEL), 50, 9);
    Check(GetMonData(mon, MON_DATA_HELD_ITEM), sSets[id].item, 10);
    Check(GetMonData(mon, MON_DATA_FRIENDSHIP), 160, 11);
    Check(GetMonData(mon, MON_DATA_ABILITY_NUM), sSets[id].slot, 12);
    Check(GetMonGender(mon), sSets[id].gender, 13);
    Check(GetNature(mon), sSets[id].nature, 14);
    for (u32 i = 0; i < MAX_MON_MOVES; i++)
    {
        Check(GetMonData(mon, MON_DATA_MOVE1 + i), sSets[id].moves[i], 15);
        Check(GetMonData(mon, MON_DATA_PP1 + i), GetMovePP(sSets[id].moves[i]), 16);
    }
    for (u32 i = 0; i < NUM_STATS; i++)
        Check(GetMonData(mon, MON_DATA_HP_EV + i), i == STAT_SPEED && !(sSets[id].iv & (31 << 15)) ? 0 : 84, 17);
}

static void CheckScout(u32 index)
{
    // Expand the pinned release's dialogue against this independently replayed
    // team. Normalize the GBA textbox line breaks before comparing it.
    u8 expected[128];
    u32 hint = sOpponents[index].hint;
    const u16 *team = sOpponents[index].team;
    StringCopy(gStringVar1, GetSpeciesName(sSets[team[0]].species));
    StringCopy(gStringVar2, GetSpeciesName(sSets[team[1]].species));
    StringCopy(gStringVar3, GetSpeciesName(sSets[team[2]].species));
    if (hint == 2 || hint == 3)
        StringCopy(gStringVar2, GetMoveName(sSets[team[0]].moves[0]));
    if (hint == 5)
        StringCopy(gStringVar1, gTypesInfo[sOpponents[index].type].name);
    StringExpandPlaceholders(expected, sScoutText[hint]);
    BufferPolishedFactoryScout();
    Check(gSpecialVar_Result, TRUE, 30);
    for (u8 *c = gStringVar4; *c != EOS; c++)
        if (*c == CHAR_NEWLINE || *c == CHAR_PROMPT_SCROLL)
            *c = CHAR_SPACE;
    Check(StringCompare(gStringVar4, expected) == 0, TRUE, 31);
}

static void BattleOpponent(u32 index)
{
    struct BattleFrontier *f = &gSaveBlock2Ptr->frontier;
    u32 stage = index % FRONTIER_STAGES_PER_CHALLENGE;
    f->curChallengeBattleNum = stage;
    f->factoryWinStreaks[0][0] = sOpponents[index].opening + stage;
    u16 ime = REG_IME;
    REG_IME = 0;
    SeedRng(index + 500);
    Factory(BATTLE_FACTORY_FUNC_GENERATE_OPPONENT_MONS);
    REG_IME = ime;
    for (u32 i = 0; i < FRONTIER_PARTY_SIZE; i++)
        Check(gFrontierTempParty[i], 0x8000 + sOpponents[index].team[i], 27);
    CheckScout(index);
    // The boss battle must use the same team the attendant previewed.
    if (stage == 6)
        TRAINER_BATTLE_PARAM.opponentA = TRAINER_FRONTIER_BRAIN;
    FillFactoryTrainerParty();
    for (u32 i = 0; i < FRONTIER_PARTY_SIZE; i++)
        CheckMon(&gParties[B_TRAINER_OPPONENT_A][i], sOpponents[index].team[i]);
    // A consumed item or changed move must not become part of the swap offer.
    u16 empty = ITEM_NONE;
    SetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_HELD_ITEM, &empty);
    SetMonMoveSlot(&gParties[B_TRAINER_OPPONENT_A][0], MOVE_SPLASH, 0);
    Factory(BATTLE_FACTORY_FUNC_SET_OPPONENT_MONS);
    CheckMon(&gParties[B_TRAINER_OPPONENT_A][0], sOpponents[index].team[0]);
    for (u32 i = 0; i < FRONTIER_PARTY_SIZE; i++)
        Check(f->rentalMons[FRONTIER_PARTY_SIZE + i].monId, 0x8000 + sOpponents[index].team[i], 28);
}

extern const u8 BattleFrontier_BattleFactoryPreBattleRoom_EventScript_ScoutOpponent[];

static void ShowScoutDialogue(void)
{
    static const struct BgTemplate bg = {.bg = 0, .charBaseIndex = 2, .mapBaseIndex = 31};
    static u16 tilemap[32 * 32];
    ResetTasks();
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, &bg, 1);
    SetBgTilemapBuffer(0, tilemap);
    InitStandardTextBoxWindows();
    InitTextBoxGfxAndPrinters();
    InitFieldMessageBox();
    SetVBlankCallback(TransferPlttBuffer);
    ShowBg(0);
    ScriptContext_SetupScript(BattleFrontier_BattleFactoryPreBattleRoom_EventScript_ScoutOpponent);
    gRentalAudit[0] = 2;
}

void CB2_MoveMachineAudit(void)
{
    if (gRentalAudit[0] == 2)
    {
        static bool8 sawFieldMessage;
        ScriptContext_RunScript();
        if (GetFieldMessageBoxMode() == FIELD_MESSAGE_BOX_NORMAL)
            sawFieldMessage = TRUE;
        RunTasks();
        DoScheduledBgTilemapCopiesToVram();
        if (!JOY_NEW(START_BUTTON))
            return;
        Check(sawFieldMessage, TRUE, 34);
        ScriptContext_Init();
        FreeAllWindowBuffers();
        ResetTasks();
        SetVBlankCallback(NULL);
        Draw(0);
        gRentalAudit[7] = offsetof(struct SaveBlock2, frontier.rentalMons);
        gRentalAudit[0] = 3;
        // The script marks a swap just before opening the selector. Confirm
        // that the initial choice does not earn one towards the next draft.
        Factory(BATTLE_FACTORY_FUNC_SET_SWAPPED);
        Factory(BATTLE_FACTORY_FUNC_SELECT_RENT_MONS);
        gSpecialVar_0x8005 = FACTORY_DATA_WIN_STREAK_SWAPS;
        gSpecialVar_0x8006 = 1;
        Factory(BATTLE_FACTORY_FUNC_SET_DATA);
        Check(gSaveBlock2Ptr->frontier.factoryRentsCount[0][0], 0, 24);
        return;
    }
    if (gRentalAudit[0] || gRentalAudit[1])
        return;
    SetSaveBlocksPointers(0);
    ResetMenuAndMonGlobals();
    Save_ResetSaveCounters();
    Sav2_ClearSetDefault();
    MoveSaveBlocks_ResetHeap();
    bool32 loaded = LoadGameSave(SAVE_NORMAL) == SAVE_STATUS_OK;
    if (!loaded)
        NewGameInitData();
    struct BattleFrontier *f = &gSaveBlock2Ptr->frontier;
    if (loaded)
    {
        Check(f->challengeStatus, CHALLENGE_STATUS_PAUSED, 26);
        gSpecialVar_0x8005 = 0;
        Factory(BATTLE_FACTORY_FUNC_SET_PARTIES);
        for (u32 i = 0; i < FRONTIER_PARTY_SIZE; i++)
        {
            CheckMon(&gParties[B_TRAINER_PLAYER][i], sDrafts[0].team[i]);
            u16 id = f->rentalMons[i + FRONTIER_PARTY_SIZE].monId;
            Check(id, 0x8000 + sOpponents[0].team[i], 18);
            CheckMon(&gParties[B_TRAINER_OPPONENT_A][i], sOpponents[0].team[i]);
            Check(gSaveBlock3Ptr->polishedTowerHistory[6 + i], sOpponents[0].team[i] + 1, 29);
            gRentalAudit[6]++;
        }
        BattleOpponent(1);
        // A defeated opponent set survives rebuilding the player's party.
        f->rentalMons[1] = f->rentalMons[4];
        gSpecialVar_0x8005 = 1;
        Factory(BATTLE_FACTORY_FUNC_SET_PARTIES);
        CheckMon(&gParties[B_TRAINER_PLAYER][1], sOpponents[1].team[1]);
        // A pre-port rental retains its native ID on either side.
        f->rentalMons[1].monId = f->rentalMons[4].monId = FRONTIER_MON_GRIMER;
        gSpecialVar_0x8005 = 0;
        Factory(BATTLE_FACTORY_FUNC_SET_PARTIES);
        Check(GetMonData(&gParties[B_TRAINER_OPPONENT_A][1], MON_DATA_SPECIES), SPECIES_GRIMER, 33);
        Check(GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_SPECIES), gBattleFrontierMons[f->rentalMons[1].monId].species, 19);
        ShowScoutDialogue();
        return;
    }

    VarSet(VAR_FRONTIER_BATTLE_MODE, FRONTIER_MODE_SINGLES);
    f->lvlMode = FRONTIER_LVL_50;
    CheckStreakCarry();
    Check(TrySavingData(SAVE_NORMAL), SAVE_STATUS_OK, 23);
    for (u32 i = 0; i < ARRAY_COUNT(sDrafts); i++)
    {
        Draw(i);
        for (u32 j = 0; j < PARTY_SIZE; j++)
        {
            struct Pokemon mon;
            Check(PolishedCreateRentalMon(&mon, f->rentalMons[j].monId), TRUE, 20);
            CheckMon(&mon, sDrafts[i].team[j]);
        }
        for (u32 stage = 0; stage < FRONTIER_STAGES_PER_CHALLENGE; stage++)
        {
            BattleOpponent(i * FRONTIER_STAGES_PER_CHALLENGE + stage);
            gCheckedOpponents++;
            if (gRentalAudit[1])
                return;
        }
        gRentalAudit[4]++;
    }

    f->lvlMode = FRONTIER_LVL_OPEN;
    Factory(BATTLE_FACTORY_FUNC_GENERATE_RENTAL_MONS);
    Check(PolishedGetRentalSet(f->rentalMons[0].monId) == NULL, TRUE, 21);
    f->lvlMode = FRONTIER_LVL_50;
    VarSet(VAR_FRONTIER_BATTLE_MODE, FRONTIER_MODE_DOUBLES);
    Factory(BATTLE_FACTORY_FUNC_GENERATE_RENTAL_MONS);
    Check(PolishedGetRentalSet(f->rentalMons[0].monId) == NULL, TRUE, 22);
    VarSet(VAR_FRONTIER_BATTLE_MODE, FRONTIER_MODE_SINGLES);
    Draw(0);
    BattleOpponent(0);
    gSpecialVar_0x8005 = 1;
    Factory(BATTLE_FACTORY_FUNC_SET_PARTIES);
    u16 empty = ITEM_NONE;
    for (u32 i = 0; i < FRONTIER_PARTY_SIZE; i++)
        SetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_HELD_ITEM, &empty);
    Factory(BATTLE_FACTORY_FUNC_RESET_HELD_ITEMS);
    for (u32 i = 0; i < FRONTIER_PARTY_SIZE; i++)
        CheckMon(&gParties[B_TRAINER_PLAYER][i], sDrafts[0].team[i]);
    gSpecialVar_0x8005 = CHALLENGE_STATUS_PAUSED;
    Factory(BATTLE_FACTORY_FUNC_SAVE);
    Factory(BATTLE_FACTORY_FUNC_SET_SWAPPED);
    gSpecialVar_0x8005 = FACTORY_DATA_WIN_STREAK_SWAPS;
    gSpecialVar_0x8006 = 100;
    Factory(BATTLE_FACTORY_FUNC_SET_DATA);
    Check(f->factoryRentsCount[0][0], 99, 25);
    gRentalAudit[0] = 1;
}
