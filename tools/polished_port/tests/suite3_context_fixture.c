// A normal wild battle between production parties follows the seeded factory
// and item checks. Only the test entry point and starting parties are replaced.
#include "global.h"
#include "battle.h"
#include "battle_main.h"
#include "battle_ai_items.h"
#include "item.h"
#include "load_save.h"
#include "main.h"
#include "new_game.h"
#include "pokemon.h"
#include "polished_rewards.h"
#include "random.h"
#include "roamer.h"
#include "save.h"
#include "script_pokemon_util.h"
#include "wild_encounter.h"

struct RewardCase { u16 seed, level; enum Item expected; u16 nextDraw; bool8 honey; };
/* GENERATED_REWARD_CASES */
EWRAM_DATA volatile u32 gContextResult[7] = {0};

static void CheckRewards(void)
{
    struct Pokemon mon;
    for (u32 i = 0; i < ARRAY_COUNT(sCases); i++)
    {
        const struct RewardCase *c = &sCases[i];
        CreateRandomMon(&mon, c->honey ? SPECIES_TEDDIURSA : SPECIES_MEOWTH, c->level);
        u32 abilitySlot = c->honey ? 2 : 0;
        SetMonData(&mon, MON_DATA_ABILITY_NUM, &abilitySlot);
        enum Item held = (i & 1) ? ITEM_EVERSTONE : ITEM_NONE;
        SetMonData(&mon, MON_DATA_HELD_ITEM, &held);
        ClearBag();
        u16 ime = REG_IME;
        REG_IME = 0;
        SeedRng(c->seed);
        enum Item result = PolishedApplyPostBattleAbility(&mon);
        u16 next = Random();
        REG_IME = ime;
        if (result != c->expected || next != c->nextDraw
            || GetMonData(&mon, MON_DATA_HELD_ITEM) != (held == ITEM_NONE ? c->expected : held)
            || (held && result && !CheckBagHasItem(result, 1)))
        {
            gContextResult[1] = i + 1;
            return;
        }
    }
}

struct MedicineCase {
    u8 seed, hp, otherLevel, expectedIndex, draws;
    u32 status;
};
static void CheckMedicine(void)
{
    const enum Item items[MAX_TRAINER_ITEMS] = {ITEM_FULL_HEAL, ITEM_POTION, ITEM_SUPER_POTION};
    /* GENERATED_MEDICINE_CASES */
    struct Pokemon *party = gParties[B_TRAINER_OPPONENT_A];
    ZeroEnemyPartyMons();
    CreateRandomMon(&party[0], SPECIES_ZUBAT, 10);
    for (u32 i = 0; i < ARRAY_COUNT(medicine); i++)
    {
        const struct MedicineCase *c = &medicine[i];
        struct BattlePokemon active = {.hp = c->hp, .maxHP = 100, .level = 10, .status1 = c->status};
        u32 hp = 0;
        SetMonData(&party[0], MON_DATA_LEVEL, &c->otherLevel);
        SetMonData(&party[0], MON_DATA_HP, &hp);
        u16 ime = REG_IME;
        REG_IME = 0;
        SeedRng(c->seed);
        for (u32 draw = 0; draw < c->draws; draw++)
            Random();
        u16 next = Random();
        SeedRng(c->seed);
        u32 index = PolishedChooseTrainerMedicine(&active, party, items);
        u16 after = Random();
        REG_IME = ime;
        if (index != c->expectedIndex || after != next)
        {
            gContextResult[6] = i + 1;
            return;
        }
    }
}

static void CheckFactories(void)
{
    bool32 seenSlots[3] = {0};
    struct Pokemon *mon = &gParties[B_TRAINER_OPPONENT_A][0];
    for (u32 i = 0; i < 96; i++)
    {
        if (i & 1)
            CreateScriptedWildMon(SPECIES_RAIKOU, 40, ITEM_ORAN_BERRY);
        else
            CreateWildMon(SPECIES_PIDGEY, 5);
        u32 perfect = 0;
        for (u32 stat = 0; stat < NUM_STATS; stat++)
        {
            u32 iv = GetMonData(mon, MON_DATA_HP_IV + stat);
            if (iv & 1) { gContextResult[2] = 1; return; }
            perfect += iv == 30;
        }
        if ((i & 1) && (perfect < 3 || GetMonData(mon, MON_DATA_HELD_ITEM) != ITEM_ORAN_BERRY))
        {
            gContextResult[2] = 2;
            return;
        }
        seenSlots[GetMonData(mon, MON_DATA_ABILITY_NUM)] = TRUE;
    }
    if (!seenSlots[0] || !seenSlots[1] || !seenSlots[2]) gContextResult[2] = 3;
    // Force a roamer whose source slot differs from what its PID would imply.
    for (u32 i = 0; i < 96; i++)
    {
        gSaveBlock1Ptr->roamer[0].active = FALSE;
        if (!TryAddRoamer(SPECIES_RAIKOU, 40)) break;
        if (GetMonData(mon, MON_DATA_ABILITY_NUM) == 2)
        {
            CreateRoamerMonInstance(0);
            if (GetMonData(mon, MON_DATA_ABILITY_NUM) == 2)
                return;
            break;
        }
    }
    gContextResult[2] = 4;
}

static void BattleDone(void)
{
    if (gContextResult[0] == 2) return;
    gContextResult[3] = gBattleOutcome;
    gContextResult[4] = GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_STATUS);
    gContextResult[5] = TrySavingData(SAVE_NORMAL) == SAVE_STATUS_OK;
    gContextResult[0] = 2;
}

void CB2_MoveMachineAudit(void)
{
    if (gContextResult[0]) return;
    SetSaveBlocksPointers(0);
    ResetMenuAndMonGlobals();
    Save_ResetSaveCounters();
    Sav2_ClearSetDefault();
    MoveSaveBlocks_ResetHeap();
    if (LoadGameSave(SAVE_NORMAL) == SAVE_STATUS_OK)
    {
        CreateRoamerMonInstance(0);
        gContextResult[2] = GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_ABILITY_NUM) != 2;
        gContextResult[4] = GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_STATUS);
        gContextResult[0] = 3;
        return;
    }
    NewGameInitData();
    SeedRng(42);
    CheckRewards();
    CheckMedicine();
    CheckFactories();
    ZeroPlayerPartyMons();
    ZeroEnemyPartyMons();
    CreateRandomMon(&gParties[B_TRAINER_PLAYER][0], SPECIES_RATICATE, 80);
    SetMonMoveSlot(&gParties[B_TRAINER_PLAYER][0], MOVE_TACKLE, 0);
    CreateRandomMon(&gParties[B_TRAINER_PLAYER][1], SPECIES_STARYU, 20);
    u32 slot = 1, status = STATUS1_BURN;
    SetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_ABILITY_NUM, &slot);
    SetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_STATUS, &status);
    CreateRandomMon(&gParties[B_TRAINER_OPPONENT_A][0], SPECIES_MAGIKARP, 2);
    gPartiesCount[B_TRAINER_PLAYER] = 2;
    gPartiesCount[B_TRAINER_OPPONENT_A] = 1;
    gSaveBlock2Ptr->optionsTextSpeed = OPTIONS_TEXT_SPEED_FAST;
    gBattleTypeFlags = 0;
    gMain.savedCallback = BattleDone;
    gContextResult[0] = 1;
    SetMainCallback2(CB2_InitBattle);
}
