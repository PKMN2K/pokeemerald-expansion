// A normal wild battle between production parties follows the seeded factory
// and item checks. Only the test entry point and starting parties are replaced.
#include "global.h"
#include "battle.h"
#include "constants/maps.h"
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
EWRAM_DATA volatile u32 gContextResult[12] = {0};

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

/* GENERATED_MOVEMENT_CASES */
/* GENERATED_ENCOUNTER_CASES */

static u32 CheckRoamingRoutes(void)
{
    struct Roamer *raikou = GetRoamerByIndex(0);
    if (!TryAddRoamer(SPECIES_ENTEI, 40) || TryAddRoamer(SPECIES_ENTEI, 40))
        return 5;
    struct Roamer *entei = GetRoamerByIndex(1);
    if (raikou->polishedRoamRoute != 42 || entei->polishedRoamRoute != 37)
        return 6;
    struct WarpData player = gSaveBlock1Ptr->location;
    gSaveBlock1Ptr->location.mapGroup = MAP_GROUP(MAP_POLISHED_ROUTE_29);
    gSaveBlock1Ptr->location.mapNum = MAP_NUM(MAP_POLISHED_ROUTE_29);
    u16 ime = REG_IME;
    REG_IME = 0;
    u32 failure = 0;
    for (u32 i = 0; i < ARRAY_COUNT(sMovement); i++)
    {
        const u16 *c = sMovement[i];
        // Legacy zero route bytes use the species' source starting route.
        raikou->polishedRoamRoute = c[1] == 42 ? 0 : c[1];
        SeedRng(c[0]);
        RoamerMove(0);
        if (raikou->polishedRoamRoute != c[2] || Random() != c[3])
        {
            failure = 1000 + i;
            break;
        }
    }
    raikou->polishedRoamRoute = entei->polishedRoamRoute = 29;
    for (u32 i = 0; !failure && i < ARRAY_COUNT(sEncounter); i++)
    {
        const u16 *c = sEncounter[i];
        u32 byte = c[1];
        u32 slot = byte < 100 ? byte & 3 : 0;
        bool32 expected = slot == 1 || slot == 2;
        SeedRng(c[0]);
        u16 first = Random();
        SeedRng(c[0]);
        if (TryStartRoamerEncounterInArea(FALSE) || Random() != first)
            failure = 2000 + i;
        SeedRng(c[0]);
        if (TryStartRoamerEncounter() != expected
            || (expected && (gEncounteredRoamerIndex != slot - 1
              || GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_SPECIES) != (slot == 1 ? SPECIES_RAIKOU : SPECIES_ENTEI)))
            || (!expected && Random() != c[2]))
            failure = 3000 + i;
    }
    raikou->polishedRoamRoute = 30;
    entei->polishedRoamRoute = 31;
    if (!IsRoamerAt(0, MAP_GROUP(MAP_POLISHED_ROUTE_30), MAP_NUM(MAP_POLISHED_ROUTE_30))
        || !IsRoamerAt(1, MAP_GROUP(MAP_POLISHED_ROUTE_31), MAP_NUM(MAP_POLISHED_ROUTE_31)))
        failure = 7;
    gSaveBlock1Ptr->location = player;
    REG_IME = ime;
    return failure;
}

static void ReadPartyAndBag(void)
{
    for (u32 i = 0; i < 2; i++)
    {
        struct Roamer *beast = GetRoamerByIndex(i);
        gContextResult[10 + i] = beast->active && beast->species == (i == 0 ? SPECIES_RAIKOU : SPECIES_ENTEI)
            ? beast->polishedRoamRoute : 0;
    }
    gContextResult[4] = GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_STATUS);
    gContextResult[7] = CountTotalItemQuantityInBag(ITEM_HONEY);
    gContextResult[8] = GetMonData(&gParties[B_TRAINER_PLAYER][5], MON_DATA_SPECIES) == SPECIES_MAGIKARP;
    gContextResult[9] = GetMonData(&gParties[B_TRAINER_PLAYER][2], MON_DATA_HELD_ITEM) == ITEM_EVERSTONE;
}

static void BattleDone(void)
{
    if (gContextResult[0] == 2) return;
    gContextResult[3] = gBattleOutcome;
    ReadPartyAndBag();
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
        ReadPartyAndBag();
        gContextResult[0] = 3;
        return;
    }
    NewGameInitData();
    SeedRng(42);
    CheckRewards();
    CheckMedicine();
    CheckFactories();
    if (!gContextResult[2])
        gContextResult[2] = CheckRoamingRoutes();
    ZeroPlayerPartyMons();
    ZeroEnemyPartyMons();
    CreateRandomMon(&gParties[B_TRAINER_PLAYER][0], SPECIES_RATICATE, 80);
    SetMonMoveSlot(&gParties[B_TRAINER_PLAYER][0], MOVE_TACKLE, 0);
    CreateRandomMon(&gParties[B_TRAINER_PLAYER][1], SPECIES_STARYU, 20);
    u32 slot = 1, status = STATUS1_BURN;
    SetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_ABILITY_NUM, &slot);
    SetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_STATUS, &status);
    CreateRandomMon(&gParties[B_TRAINER_OPPONENT_A][0], SPECIES_MAGIKARP, 2);
    for (u32 i = 2; i < 5; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];
        CreateRandomMon(mon, SPECIES_TEDDIURSA, 100);
        u32 honeyGather = 2;
        enum Item held = ITEM_EVERSTONE;
        SetMonData(mon, MON_DATA_ABILITY_NUM, &honeyGather);
        SetMonData(mon, MON_DATA_HELD_ITEM, &held);
    }
    ClearBag();
    AddBagItem(ITEM_MASTER_BALL, 3);
    gPartiesCount[B_TRAINER_PLAYER] = 5;
    gPartiesCount[B_TRAINER_OPPONENT_A] = 1;
    if (PROBE_LOSS)
    {
        for (u32 i = 0; i < 5; i++)
        {
            u32 hp = i == 0 ? 1 : 0;
            SetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_HP, &hp);
        }
        struct Pokemon *opponent = &gParties[B_TRAINER_OPPONENT_A][0];
        CreateRandomMon(opponent, SPECIES_RATICATE, 100);
        SetMonMoveSlot(opponent, MOVE_QUICK_ATTACK, 0);
        for (u32 i = 1; i < MAX_MON_MOVES; i++)
            SetMonMoveSlot(opponent, MOVE_NONE, i);
    }
    gSaveBlock2Ptr->optionsTextSpeed = OPTIONS_TEXT_SPEED_FAST;
    gBattleTypeFlags = 0;
    gMain.savedCallback = BattleDone;
    gContextResult[0] = 1;
    SetMainCallback2(CB2_InitBattle);
}
