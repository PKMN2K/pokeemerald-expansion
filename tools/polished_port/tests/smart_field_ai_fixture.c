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
    enum Move selected, last, seen0, seen1, seen2, seen3;
    u16 damage, matchup;
    u8 cached, best;
    bool8 userGhost, userReserve, targetReserve, shell, trapped, charged, cursed, perish;
    enum Type type1, type2;
    enum Ability userAbility, targetAbility;
    bool8 powerHerb, ring, inverse, seenProtection, targetSeeded;
    s8 userEvasion, targetAccuracy;
    u8 spikes, hidden, stat;
    s8 userOffset, targetOffset;
    bool8 ghost, partyStatus, safeguard, reflect, lightScreen, ownSafeguard;
    u8 targetConfused, targetSpikes, toxicSpikes;
    bool8 focusEnergy, transformed, substitute, disabled, encored, meanLook, futureSight, destinyBond, oppositeGender;
    bool8 targetWrapped, targetInLove, targetIdentified, targetRolling, targetLockOn;
    bool8 userWrapped, userSeeded, userRage, userProtected, userInLove, userConfused;
};

static const struct {
    enum Move moves[4];
    u16 hp, maxHP, targetHP, seed, next;
    u16 weather, userWeather;
    u8 mask, basicDiscouraged;
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
    MOVE_SLEEP_TALK, MOVE_SACRED_FIRE, MOVE_SPORE, MOVE_DREAM_EATER, MOVE_HEAL_BELL, MOVE_DESTINY_BOND, MOVE_SOLAR_BEAM, MOVE_GIGA_DRAIN, MOVE_GUST, MOVE_EARTHQUAKE,
    offsetof(struct BattleStruct, polishedChoiceMove[B_BATTLER_1]),
    MOVE_QUICK_ATTACK, MOVE_ROAR, MOVE_PERISH_SONG, MOVE_ENCORE, MOVE_COUNTER, MOVE_MIRROR_COAT,
    MOVE_REFLECT, MOVE_LIGHT_SCREEN, MOVE_CONFUSE_RAY, MOVE_SWAGGER, MOVE_SUNNY_DAY,
    MOVE_ICE_BEAM, MOVE_CRUNCH, MOVE_FOCUS_ENERGY, MOVE_LEECH_SEED, MOVE_BATON_PASS,
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
        user.statStages[STAT_EVASION] += t->userEvasion;
        target.statStages[STAT_ACC] += t->targetAccuracy;
        target.volatiles.leechSeed = t->targetSeeded;
        target.volatiles.confusionTimer = t->targetConfused;
        user.volatiles.focusEnergy = t->focusEnergy;
        user.volatiles.transformed = t->transformed;
        user.volatiles.substituteHP = t->substitute;
        user.volatiles.destinyBond = t->destinyBond;
        target.volatiles.disableTimer = t->disabled;
        target.volatiles.encoreTimer = t->encored;
        target.volatiles.escapePrevention = t->meanLook;
        target.status1 = t->targetStatus;
        target.volatiles.wrapTurns = t->targetWrapped;
        target.volatiles.infatuation = t->targetInLove;
        target.volatiles.cursed = t->cursed;
        target.volatiles.perishSong = t->perish;
        target.volatiles.foresight = t->targetIdentified;
        target.volatiles.rolloutTimer = t->targetRolling;
        target.volatiles.lockOn = t->targetLockOn;
        target.volatiles.semiInvulnerable = t->hidden;
        memcpy(user.moves, sCases[i].moves, sizeof(user.moves));
        const enum Move seen[] = {t->seen0, t->seen1, t->seen2, t->seen3};
        memcpy(gPolishedPlayerUsedMoves, seen, sizeof(seen));
        SeedRng(sCases[i].seed);
        struct PolishedSmartAiContext context = {
            .targetTypes = {t->type1 ? t->type1 : TYPE_NORMAL, t->type2 ? t->type2 : TYPE_NORMAL, TYPE_MYSTERY},
            .targetLastMove = t->last, .targetMatchup = t->matchup, .switchScore = t->cached, .bestReserveScore = t->best,
            .priorityDamage = {t->damage, t->damage, t->damage, t->damage},
            .userGhost = t->userGhost, .userReserve = t->userReserve, .targetReserve = t->targetReserve,
            .targetShedShell = t->shell, .targetTrapped = t->trapped, .targetCharging = t->charged,
            .targetSelectedMove = t->selected, .userAbility = t->userAbility, .targetAbility = t->targetAbility,
            .powerHerb = t->powerHerb, .targetRing = t->ring, .seenProtection = t->seenProtection,
            .earlyBird = sCases[i].earlyBird, .spikes = t->spikes,
            .targetGhost = t->ghost, .partyHasStatus = t->partyStatus,
            .weather = sCases[i].weather, .userWeather = sCases[i].userWeather,
            .userSpeed = sCases[i].userSpeed, .targetSpeed = sCases[i].targetSpeed,
            .trickRoom = sCases[i].trickRoom, .targetFirstTurn = sCases[i].targetFirstTurn,
            .lastMonAgainstReserve = sCases[i].lastMonAgainstReserve,
        };
        gMain.inBattle = t->inverse;
        gBattleTypeFlags = t->inverse ? BATTLE_TYPE_TRAINER : 0;
        TRAINER_BATTLE_PARAM.opponentA = t->inverse ? TRAINER_PC_INVER_1 : 0;
        u32 userSide = (t->reflect ? SIDE_STATUS_REFLECT : 0) | (t->lightScreen ? SIDE_STATUS_LIGHTSCREEN : 0)
                    | (t->ownSafeguard ? SIDE_STATUS_SAFEGUARD : 0);
        struct PolishedBasicAiContext basic = {
            .userSide = userSide, .targetSide = t->safeguard ? SIDE_STATUS_SAFEGUARD : 0, .weather = context.weather,
            .userSpeed = context.userSpeed, .targetSpeed = context.targetSpeed, .trickRoom = context.trickRoom,
            .userGhost = t->userGhost, .oppositeGender = t->oppositeGender, .futureSight = t->futureSight,
            .userPartyHP = t->userReserve, .targetPartyHP = t->targetReserve,
            .spikes = t->targetSpikes, .toxicSpikes = t->toxicSpikes,
        };
        PolishedScoreBasicMoves(&user, &target, sCases[i].mask, &basic, scores);
        for (u32 slot = 0; slot < MAX_MON_MOVES; slot++)
            if (scores[slot] != ((sCases[i].basicDiscouraged & (1u << slot)) ? 90 : 100))
            {
                gSmartAudit[1] = i + 1;
                gSmartAudit[2] = slot + MAX_MON_MOVES + 1;
                REG_IME = ime;
                return;
            }
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
    memset(gPolishedPlayerUsedMoves, 0, sizeof(gPolishedPlayerUsedMoves));
    // A duplicate keeps its original position; a fifth distinct move evicts the oldest.
    const enum Move observed[] = {MOVE_PROTECT, MOVE_ENDURE, MOVE_SPLASH, MOVE_GUST, MOVE_PROTECT, MOVE_DIG};
    const enum Move expected[] = {MOVE_ENDURE, MOVE_SPLASH, MOVE_GUST, MOVE_DIG};
    for (u32 i = 0; i < ARRAY_COUNT(observed); i++)
        PolishedRecordPlayerMove(observed[i]);
    if (memcmp(gPolishedPlayerUsedMoves, expected, sizeof(expected)))
        gSmartAudit[1] = ARRAY_COUNT(sCases) + 1;
    gMain.inBattle = FALSE;
    gBattleTypeFlags = 0;
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

static void SetBasicParties(u16 keys)
{
    struct Pokemon *player = &gParties[B_TRAINER_PLAYER][0];
    struct Pokemon *trainer = &gParties[B_TRAINER_OPPONENT_A][0];
    if (keys & START_BUTTON)
    {
        SetMon(player, (keys & A_BUTTON) ? SPECIES_DRAGONITE : SPECIES_SLOWPOKE, 150, MOVE_SPLASH, MOVE_NONE);
        SetMon(trainer, SPECIES_STARYU, 150, MOVE_ICE_BEAM, MOVE_CRUNCH);
    }
    else if (keys & DPAD_RIGHT)
    {
        SetMon(player, SPECIES_MAGIKARP, 150, MOVE_SPLASH, MOVE_NONE);
        SetMon(trainer, SPECIES_STARYU, 150, MOVE_FOCUS_ENERGY, MOVE_LEECH_SEED);
    }
    else if (keys & (A_BUTTON | B_BUTTON))
    {
        SetMon(player, SPECIES_MAGIKARP, 150, MOVE_SPLASH, MOVE_NONE);
        SetMon(trainer, SPECIES_STARYU, 150, MOVE_ROAR, MOVE_BATON_PASS);
        if (keys & A_BUTTON)
        {
            // The source HP scan also sees a nonzero HP word in an Egg slot.
            u32 egg = TRUE;
            SetMon(&player[1], SPECIES_MAGIKARP, 150, MOVE_SPLASH, MOVE_NONE);
            SetMonData(&player[1], MON_DATA_IS_EGG, &egg);
            gPartiesCount[B_TRAINER_PLAYER] = 2;
        }
    }
    else if (keys & DPAD_LEFT)
    {
        u32 drought = 2;
        SetMon(player, SPECIES_NINETALES, 150, MOVE_SPLASH, MOVE_NONE);
        SetMonData(player, MON_DATA_ABILITY_NUM, &drought);
        SetMon(trainer, SPECIES_STARYU, 150, MOVE_SUNNY_DAY, MOVE_RAIN_DANCE);
    }
    else if (keys & DPAD_DOWN)
    {
        u32 speed = 250;
        SetMon(player, SPECIES_MAGIKARP, 150, MOVE_SAFEGUARD, MOVE_NONE);
        SetMonData(player, MON_DATA_SPEED, &speed);
        SetMon(trainer, SPECIES_STARYU, 150, MOVE_CONFUSE_RAY, MOVE_SWAGGER);
    }
    else
    {
        SetMon(player, SPECIES_MAGIKARP, 150, MOVE_REFLECT, MOVE_NONE);
        SetMon(trainer, SPECIES_STARYU, 150, MOVE_REFLECT, MOVE_LIGHT_SCREEN);
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

static void SetMatchupParties(u16 keys)
{
    struct Pokemon *player = &gParties[B_TRAINER_PLAYER][0];
    struct Pokemon *trainer = &gParties[B_TRAINER_OPPONENT_A][0];
    if ((keys & SELECT_BUTTON) && (keys & (DPAD_RIGHT | DPAD_LEFT)))
    {
        bool32 fly = keys & DPAD_LEFT;
        SetMon(player, SPECIES_SNORLAX, 150, fly ? MOVE_FLY : MOVE_DIG, MOVE_SPLASH);
        SetMon(trainer, SPECIES_STARYU, 150, fly ? MOVE_GUST : MOVE_EARTHQUAKE, MOVE_DESTINY_BOND);
        u32 slow = 40, fast = 160, defense = 350;
        SetMonData(player, MON_DATA_SPEED, &slow);
        SetMonData(player, MON_DATA_DEF, &defense);
        SetMonData(trainer, MON_DATA_SPEED, &fast);
    }
    else if (keys & SELECT_BUTTON)
    {
        u32 abilitySlot = (keys & B_BUTTON) ? 2 : 0; // Marill: Sap Sipper or Thick Fat.
        SetMon(player, SPECIES_MARILL, 150, MOVE_SPLASH, MOVE_NONE);
        SetMonData(player, MON_DATA_ABILITY_NUM, &abilitySlot);
        SetMon(trainer, SPECIES_STARYU, 100, MOVE_GIGA_DRAIN, MOVE_DESTINY_BOND);
    }
    else
    {
        SetMon(player, SPECIES_MAGIKARP, 150, (keys & DPAD_LEFT) ? MOVE_ENDURE : MOVE_PROTECT, MOVE_SPLASH);
        SetMon(trainer, SPECIES_STARYU, 40, MOVE_SOLAR_BEAM, MOVE_RECOVER);
        if (keys & DPAD_RIGHT)
        {
            enum Item herb = ITEM_POWER_HERB;
            SetMonData(trainer, MON_DATA_HELD_ITEM, &herb);
        }
    }
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

static void SetReactiveParties(u16 keys)
{
    struct Pokemon *player = &gParties[B_TRAINER_PLAYER][0];
    struct Pokemon *trainer = &gParties[B_TRAINER_OPPONENT_A][0];
    SetMon(player, SPECIES_MAGIKARP, 150, MOVE_SPLASH, MOVE_NONE);
    if ((keys & SELECT_BUTTON) && (keys & (DPAD_LEFT | DPAD_RIGHT)))
    {
        bool32 grass = keys & DPAD_RIGHT;
        SetMon(&player[1], SPECIES_MAGIKARP, 150, MOVE_SPLASH, MOVE_NONE);
        gPartiesCount[B_TRAINER_PLAYER] = 2;
        SetMon(trainer, SPECIES_STARYU, 40, MOVE_ROAR, MOVE_RECOVER);
        SetMon(&trainer[1], grass ? SPECIES_BULBASAUR : SPECIES_CHARMANDER, 150, grass ? MOVE_ABSORB : MOVE_FLAMETHROWER, MOVE_NONE);
        gPartiesCount[B_TRAINER_OPPONENT_A] = 2;
    }
    else if ((keys & SELECT_BUTTON) && (keys & (DPAD_UP | DPAD_DOWN)))
    {
        u32 ability = (keys & DPAD_UP) ? 2 : 0; // Parental Bond or Early Bird.
        u32 hp = 34, stat = 100, slow = 40;
        SetMon(trainer, SPECIES_KANGASKHAN, 20, MOVE_QUICK_ATTACK, MOVE_RECOVER);
        SetMonData(player, MON_DATA_HP, &hp);
        SetMonData(player, MON_DATA_DEF, &stat);
        SetMonData(player, MON_DATA_SPEED, &stat);
        SetMonData(trainer, MON_DATA_ATK, &stat);
        SetMonData(trainer, MON_DATA_SPEED, &slow);
        SetMonData(trainer, MON_DATA_ABILITY_NUM, &ability);
    }
    else if (keys & SELECT_BUTTON)
    {
        SetMon(trainer, SPECIES_STARYU, 150, MOVE_COUNTER, MOVE_MIRROR_COAT);
        SetMonMoveSlot(player, (keys & A_BUTTON) ? MOVE_TACKLE : MOVE_WATER_GUN, 0);
    }
    else if (keys & (DPAD_LEFT | DPAD_RIGHT | DPAD_DOWN))
    {
        bool32 roarFirst = keys & DPAD_RIGHT;
        SetMon(trainer, SPECIES_STARYU, 150, roarFirst ? MOVE_ROAR : MOVE_PERISH_SONG,
               roarFirst ? MOVE_PERISH_SONG : MOVE_ROAR);
        if (!(keys & DPAD_DOWN))
        {
            SetMon(&trainer[1], SPECIES_STARYU, 150, MOVE_SURF, MOVE_NONE);
            gPartiesCount[B_TRAINER_OPPONENT_A] = 2;
        }
    }
    else if (keys & DPAD_UP)
    {
        SetMon(trainer, SPECIES_STARYU, 150, MOVE_ENCORE, MOVE_DESTINY_BOND);
        u32 fast = 160;
        SetMonData(trainer, MON_DATA_SPEED, &fast);
    }
    else
    {
        // Level 50, 40 power and equal offensive/defensive stats give 19 damage before the roll.
        u32 hp = (keys & A_BUTTON) ? 18 : 19;
        u32 stat = 100, speed = (keys & A_BUTTON) && (keys & B_BUTTON) ? 160 : 40;
        SetMon(trainer, SPECIES_STARYU, 20, MOVE_QUICK_ATTACK, MOVE_RECOVER);
        SetMonData(player, MON_DATA_HP, &hp);
        SetMonData(player, MON_DATA_DEF, &stat);
        SetMonData(player, MON_DATA_SPEED, &stat);
        SetMonData(trainer, MON_DATA_ATK, &stat);
        SetMonData(trainer, MON_DATA_SPEED, &speed);
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
    bool32 reactive = (keys & (L_BUTTON | R_BUTTON | START_BUTTON)) == (L_BUTTON | R_BUTTON | START_BUTTON);
    bool32 basic = (keys & (L_BUTTON | R_BUTTON | SELECT_BUTTON)) == (R_BUTTON | SELECT_BUTTON);
    u32 badges = 0xffff;
    if (basic)
        badges = (keys & START_BUTTON) ? 3 : 0;
    else if (!reactive && (keys & (DPAD_UP | DPAD_DOWN)))
        badges = 0;
    VarSet(VAR_POLISHED_EARNED_BADGES, badges);
    gSaveBlock2Ptr->optionsTextSpeed = OPTIONS_TEXT_SPEED_FAST;
    gSaveBlock2Ptr->optionsBattleSceneOff = TRUE;
    gPartiesCount[B_TRAINER_PLAYER] = gPartiesCount[B_TRAINER_OPPONENT_A] = 1;
    if (basic)
        SetBasicParties(keys);
    else if (reactive)
        SetReactiveParties(keys);
    else if ((keys & (L_BUTTON | R_BUTTON)) == (L_BUTTON | R_BUTTON))
        SetMatchupParties(keys);
    else if (keys & L_BUTTON)
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
