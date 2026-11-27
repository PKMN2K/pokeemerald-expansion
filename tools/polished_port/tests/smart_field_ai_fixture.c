#include "global.h"
#include "battle.h"
#include "battle_main.h"
#include "battle_setup.h"
#include "event_data.h"
#include "load_save.h"
#include "main.h"
#include "new_game.h"
#include "pokemon.h"
#include "polished_trainers.h"
#include "random.h"
#include "save.h"
#include "constants/opponents.h"

static const struct {
    enum Move moves[4];
    u16 hp, maxHP, targetHP, seed, next;
    u16 weather, userWeather;
    u8 mask;
    s8 delta[4];
} sCases[] = {
/* SOURCE CASES */
};

EWRAM_DATA volatile u32 gSmartAudit[3] = {0};
const volatile u32 gSmartLayout[] = {
    offsetof(struct AiBattleData, finalScore[B_BATTLER_1][B_BATTLER_0]),
    MOVE_RECOVER, MOVE_RAIN_DANCE, MOVE_POLISHED_HEALING_LIGHT,
};

static void CheckScores(void)
{
    u16 ime = REG_IME;
    REG_IME = 0;
    for (u32 i = 0; i < ARRAY_COUNT(sCases); i++)
    {
        struct BattlePokemon user = {.hp = sCases[i].hp, .maxHP = sCases[i].maxHP};
        struct BattlePokemon target = {.hp = sCases[i].targetHP, .maxHP = 100};
        s32 scores[] = {100, 100, 100, 100};
        memcpy(user.moves, sCases[i].moves, sizeof(user.moves));
        SeedRng(sCases[i].seed);
        PolishedScoreSmartFieldMoves(&user, &target, sCases[i].mask, sCases[i].weather, sCases[i].userWeather, scores);
        u16 next = Random();
        for (u32 slot = 0; slot < MAX_MON_MOVES; slot++)
            if (scores[slot] != 100 + sCases[i].delta[slot] || next != sCases[i].next)
            {
                gSmartAudit[1] = i + 1;
                gSmartAudit[2] = slot + 1;
                REG_IME = ime;
                return;
            }
    }
    REG_IME = ime;
}

static void SetMon(struct Pokemon *mon, enum Species species, u32 hp, enum Move first, enum Move second)
{
    CreateMon(mon, species, 50, 42, OTID_STRUCT_PRESET(0));
    u32 maximum = 150, dynamax = BLOCK_AI_DYNAMAX, tera = TYPE_MYSTERY;
    SetMonData(mon, MON_DATA_MAX_HP, &maximum);
    SetMonData(mon, MON_DATA_HP, &hp);
    SetMonData(mon, MON_DATA_DYNAMAX_LEVEL, &dynamax);
    SetMonData(mon, MON_DATA_TERA_TYPE, &tera);
    SetMonMoveSlot(mon, first, 0);
    SetMonMoveSlot(mon, second, 1);
    SetMonMoveSlot(mon, MOVE_NONE, 2);
    SetMonMoveSlot(mon, MOVE_NONE, 3);
}

void CB2_MoveMachineAudit(void)
{
    (void)gSmartLayout[0];
    if (!gSmartAudit[0])
    {
        SetSaveBlocksPointers(0);
        ResetMenuAndMonGlobals();
        Save_ResetSaveCounters();
        Sav2_ClearSetDefault();
        MoveSaveBlocks_ResetHeap();
        NewGameInitData();
        CheckScores();
        gSmartAudit[0] = ARRAY_COUNT(sCases);
    }
    u16 keys = gMain.newKeys;
    bool32 sun = keys & (DPAD_LEFT | DPAD_RIGHT);
    if (!(keys & (A_BUTTON | B_BUTTON | DPAD_UP | DPAD_DOWN)) && !sun)
        return;
    VarSet(VAR_POLISHED_EARNED_BADGES, (keys & (DPAD_UP | DPAD_DOWN)) ? 0 : 0xffff);
    gSaveBlock2Ptr->optionsTextSpeed = OPTIONS_TEXT_SPEED_FAST;
    gSaveBlock2Ptr->optionsBattleSceneOff = TRUE;
    SetMon(&gParties[B_TRAINER_PLAYER][0], sun ? SPECIES_NINETALES : SPECIES_MAGIKARP, 150, MOVE_SPLASH, MOVE_NONE);
    if (sun)
    {
        u32 drought = 2;
        SetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_ABILITY_NUM, &drought);
    }
    SetMon(&gParties[B_TRAINER_OPPONENT_A][0], SPECIES_STARYU, (sun || (keys & (A_BUTTON | DPAD_UP))) ? 20 : 150,
           sun ? MOVE_POLISHED_HEALING_LIGHT : MOVE_RECOVER, MOVE_RAIN_DANCE);
    if (keys & DPAD_RIGHT)
    {
        enum Item umbrella = ITEM_UTILITY_UMBRELLA;
        SetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_HELD_ITEM, &umbrella);
    }
    gPartiesCount[B_TRAINER_PLAYER] = gPartiesCount[B_TRAINER_OPPONENT_A] = 1;
    TRAINER_BATTLE_PARAM.opponentA = TRAINER_POLISHED_JOEY;
    gBattleTypeFlags = BATTLE_TYPE_TRAINER;
    SetMainCallback2(CB2_InitBattle);
}
