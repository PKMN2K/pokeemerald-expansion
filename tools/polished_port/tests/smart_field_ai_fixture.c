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

struct TacticsInput {
    u32 targetStatus;
    u8 spikes, hidden, stat;
    s8 userOffset, targetOffset;
    bool8 ghost, partyStatus, safeguard;
    bool8 targetWrapped, targetInLove, targetIdentified, targetRolling, targetLockOn;
    bool8 userWrapped, userSeeded, userRage, userProtected, userInLove, userConfused;
};

static const struct {
    enum Move moves[4];
    u16 hp, maxHP, targetHP, seed, next;
    u16 weather, userWeather;
    u8 mask;
    s8 delta[4];
    u16 userSpeed, targetSpeed;
    bool8 trickRoom, targetFirstTurn;
    u16 targetMaxHP;
    u8 attackStage;
    bool8 lastMonAgainstReserve;
    s8 accuracy, evasion, spDefense; // Stage offsets from neutral.
    bool8 minimized, earlyBird;
    u32 status;
    struct TacticsInput tactics;
} sCases[] = {
/* SOURCE CASES */
};

EWRAM_DATA volatile u32 gSmartAudit[3] = {0};
const volatile u32 gSmartLayout[] = {
    offsetof(struct AiBattleData, finalScore[B_BATTLER_1][B_BATTLER_0]),
    offsetof(struct BattleResults, battleTurnCounter),
    MOVE_RECOVER, MOVE_RAIN_DANCE, MOVE_POLISHED_HEALING_LIGHT, MOVE_TRICK_ROOM, MOVE_SPLASH, MOVE_EXPLOSION, MOVE_PAIN_SPLIT,
    MOVE_SLEEP_TALK, MOVE_SACRED_FIRE, MOVE_SPORE, MOVE_DREAM_EATER, MOVE_HEAL_BELL, MOVE_DESTINY_BOND,
};

static void CheckScores(void)
{
    u16 ime = REG_IME;
    REG_IME = 0;
    for (u32 i = 0; i < ARRAY_COUNT(sCases); i++)
    {
        struct BattlePokemon user = {.hp = sCases[i].hp, .maxHP = sCases[i].maxHP};
        struct BattlePokemon target = {.hp = sCases[i].targetHP, .maxHP = sCases[i].targetMaxHP};
        s32 scores[] = {100, 100, 100, 100};
        const struct TacticsInput *t = &sCases[i].tactics;
        for (u32 stat = STAT_ATK; stat <= STAT_EVASION; stat++)
            user.statStages[stat] = target.statStages[stat] = DEFAULT_STAT_STAGE;
        user.statStages[STAT_ATK] = sCases[i].attackStage;
        user.statStages[STAT_ACC] = DEFAULT_STAT_STAGE + sCases[i].accuracy;
        user.statStages[STAT_SPDEF] = DEFAULT_STAT_STAGE + sCases[i].spDefense;
        user.status1 = sCases[i].status;
        target.statStages[STAT_EVASION] = DEFAULT_STAT_STAGE + sCases[i].evasion;
        target.volatiles.minimize = sCases[i].minimized;
        if (t->stat != STAT_HP)
        {
            user.statStages[t->stat] = DEFAULT_STAT_STAGE + t->userOffset;
            target.statStages[t->stat] = DEFAULT_STAT_STAGE + t->targetOffset;
        }
        user.volatiles.wrapTurns = t->userWrapped;
        user.volatiles.leechSeed = t->userSeeded;
        user.volatiles.rage = t->userRage;
        user.volatiles.consecutiveMoveUses = t->userProtected;
        user.volatiles.infatuation = t->userInLove;
        user.volatiles.confusionTimer = t->userConfused;
        target.status1 = t->targetStatus;
        target.volatiles.wrapTurns = t->targetWrapped;
        target.volatiles.infatuation = t->targetInLove;
        target.volatiles.foresight = t->targetIdentified;
        target.volatiles.rolloutTimer = t->targetRolling;
        target.volatiles.lockOn = t->targetLockOn;
        target.volatiles.semiInvulnerable = t->hidden;
        memcpy(user.moves, sCases[i].moves, sizeof(user.moves));
        SeedRng(sCases[i].seed);
        struct PolishedSmartAiContext context = {
            .earlyBird = sCases[i].earlyBird, .spikes = t->spikes,
            .targetGhost = t->ghost, .partyHasStatus = t->partyStatus,
            .weather = sCases[i].weather, .userWeather = sCases[i].userWeather,
            .userSpeed = sCases[i].userSpeed, .targetSpeed = sCases[i].targetSpeed,
            .trickRoom = sCases[i].trickRoom, .targetFirstTurn = sCases[i].targetFirstTurn,
            .lastMonAgainstReserve = sCases[i].lastMonAgainstReserve,
        };
        PolishedScoreBasicSleepMoves(&user, &target, sCases[i].mask, t->safeguard, scores);
        PolishedScoreSmartMoves(&user, &target, sCases[i].mask, &context, scores);
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

static void SetFieldParties(u16 keys, bool32 sun)
{
    SetMon(&gParties[B_TRAINER_PLAYER][0], sun ? SPECIES_NINETALES : SPECIES_MAGIKARP, 150, MOVE_SPLASH, MOVE_NONE);
    if (sun)
    {
        u32 drought = 2;
        SetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_ABILITY_NUM, &drought);
    }
    bool32 lowHP = sun || (!(keys & START_BUTTON) && (keys & (A_BUTTON | DPAD_UP)));
    SetMon(&gParties[B_TRAINER_OPPONENT_A][0], SPECIES_STARYU, lowHP ? 20 : 150,
           sun ? MOVE_POLISHED_HEALING_LIGHT : MOVE_RECOVER, MOVE_RAIN_DANCE);
    if (keys & START_BUTTON)
    {
        struct Pokemon *player = &gParties[B_TRAINER_PLAYER][0];
        struct Pokemon *trainer = &gParties[B_TRAINER_OPPONENT_A][0];
        u32 playerSpeed = 100, trainerSpeed = 120;
        SetMonData(player, MON_DATA_SPEED, &playerSpeed);
        SetMonData(trainer, MON_DATA_SPEED, &trainerSpeed);
        SetMonMoveSlot(trainer, MOVE_TRICK_ROOM, 0);
        SetMonMoveSlot(trainer, MOVE_SPLASH, 1);
        if (keys & B_BUTTON)
        {
            enum Item scarf = ITEM_CHOICE_SCARF;
            SetMonData(player, MON_DATA_HELD_ITEM, &scarf);
        }
    }
    if (keys & DPAD_RIGHT)
    {
        enum Item umbrella = ITEM_UTILITY_UMBRELLA;
        SetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_HELD_ITEM, &umbrella);
    }
}

static void SetExplosionParties(u16 keys)
{
    SetMon(&gParties[B_TRAINER_PLAYER][0], SPECIES_MAGIKARP, 59, MOVE_SPLASH, MOVE_NONE);
    SetMon(&gParties[B_TRAINER_OPPONENT_A][0], SPECIES_STARYU, 30, MOVE_EXPLOSION, MOVE_PAIN_SPLIT);
    SetMon(&gParties[B_TRAINER_PLAYER][1], SPECIES_MAGIKARP, (keys & B_BUTTON) ? 0 : 150, MOVE_SPLASH, MOVE_NONE);
    gPartiesCount[B_TRAINER_PLAYER] = 2;
    if (keys & DPAD_LEFT)
    {
        u32 egg = TRUE;
        SetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_IS_EGG, &egg);
    }
    if (keys & DPAD_RIGHT)
    {
        SetMon(&gParties[B_TRAINER_OPPONENT_A][1], SPECIES_STARYU, 150, MOVE_SPLASH, MOVE_NONE);
        gPartiesCount[B_TRAINER_OPPONENT_A] = 2;
    }
}

static void SetStatusParties(u16 keys)
{
    struct Pokemon *player = &gParties[B_TRAINER_PLAYER][0];
    struct Pokemon *trainer = &gParties[B_TRAINER_OPPONENT_A][0];
    u32 status;
    if (keys & DPAD_LEFT)
    {
        SetMon(player, SPECIES_MAGIKARP, 150, MOVE_SPLASH, MOVE_NONE);
        SetMon(trainer, SPECIES_STARYU, 20, MOVE_SACRED_FIRE, MOVE_RECOVER);
        status = (keys & START_BUTTON) ? STATUS1_NONE : STATUS1_FREEZE;
    }
    else
    {
        bool32 gas = (keys & DPAD_RIGHT) != 0;
        u32 ability = gas || (keys & B_BUTTON); // Doduo's second slot is Early Bird.
        SetMon(player, SPECIES_KOFFING, 150, MOVE_SPLASH, MOVE_NONE);
        SetMonData(player, MON_DATA_ABILITY_NUM, &gas);
        SetMon(trainer, SPECIES_DODUO, 150, MOVE_SLEEP_TALK, MOVE_SPLASH);
        SetMonData(trainer, MON_DATA_ABILITY_NUM, &ability);
        status = STATUS1_SLEEP_TURN(2);
    }
    SetMonData(trainer, MON_DATA_STATUS, &status);
}

static void SetTacticsParties(u16 keys)
{
    struct Pokemon *player = &gParties[B_TRAINER_PLAYER][0];
    struct Pokemon *trainer = &gParties[B_TRAINER_OPPONENT_A][0];
    SetMon(player, SPECIES_MAGIKARP, 150, MOVE_SPLASH, MOVE_NONE);
    if (keys & (DPAD_LEFT | DPAD_RIGHT))
    {
        struct Pokemon *reserve = &gParties[B_TRAINER_OPPONENT_A][1];
        u32 poison = STATUS1_POISON;
        SetMon(trainer, SPECIES_STARYU, 150, MOVE_HEAL_BELL, MOVE_DESTINY_BOND);
        SetMon(reserve, SPECIES_STARYU, (keys & DPAD_LEFT) ? 0 : 100, MOVE_SPLASH, MOVE_NONE);
        SetMonData(reserve, MON_DATA_STATUS, &poison);
        gPartiesCount[B_TRAINER_OPPONENT_A] = 2;
    }
    else
    {
        SetMon(trainer, SPECIES_STARYU, 150, MOVE_SPORE, MOVE_DREAM_EATER);
        if (keys & (B_BUTTON | DPAD_DOWN))
        {
            u32 sleep = STATUS1_SLEEP_TURN(4);
            SetMonData(player, MON_DATA_STATUS, &sleep);
        }
        if (keys & START_BUTTON)
        {
            u32 pp = 0;
            SetMonData(trainer, MON_DATA_PP2, &pp);
        }
    }
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
    gPartiesCount[B_TRAINER_PLAYER] = gPartiesCount[B_TRAINER_OPPONENT_A] = 1;
    if (keys & L_BUTTON)
        SetTacticsParties(keys);
    else if (keys & R_BUTTON)
        SetStatusParties(keys);
    else if (keys & SELECT_BUTTON)
        SetExplosionParties(keys);
    else
        SetFieldParties(keys, sun);
    TRAINER_BATTLE_PARAM.opponentA = TRAINER_POLISHED_JOEY;
    gBattleTypeFlags = BATTLE_TYPE_TRAINER;
    SetMainCallback2(CB2_InitBattle);
}
