#include "global.h"
#include "battle.h"
#include "battle_main.h"
#include "battle_util.h"
#include "item.h"
#include "random.h"
#include "polished_escape.h"

bool32 UsesPolishedEscapeRules(enum BattlerId battler)
{
    return UsesPolishedBattleRules(battler, GetOppositeBattler(battler))
        && !(gBattleTypeFlags & (BATTLE_TYPE_TRAINER | BATTLE_TYPE_LINK
            | BATTLE_TYPE_FRONTIER | BATTLE_TYPE_TRAINER_HILL | BATTLE_TYPE_SAFARI
            | BATTLE_TYPE_FIRST_BATTLE | BATTLE_TYPE_GHOST))
        && gBattleMons[battler].item != ITEM_ENIGMA_BERRY_E_READER
        && !gBattleStruct->battlerState[battler].commanderSpecies
        && !gBattleMons[battler].volatiles.root
        && !(gFieldStatuses & STATUS_FIELD_FAIRY_LOCK)
        && gBattleMons[battler].volatiles.semiInvulnerable != STATE_SKY_DROP_TARGET;
}

u32 PolishedEscapeThreshold(u16 playerSpeed, u16 enemySpeed, u8 previousAttempts)
{
    u32 divisor = (enemySpeed >> 2) & 255;
    if (playerSpeed >= enemySpeed || divisor == 0)
        return 256;
    // CheckRunSpeed truncates five shifts to a word and the divisor to a byte.
    // The incremented attempt byte is decremented before adding each 30, so
    // wrapping attempt 255 to zero still adds 30 up to 255 times.
    u32 value = (u16)(playerSpeed * 32) / divisor;
    return min(256, value + 30 * previousAttempts + 1);
}

bool32 CanPolishedEnemyFlee(enum BattlerId battler)
{
    if ((gBattleTypeFlags & BATTLE_TYPE_TRAINER) || !IsBattlerAlive(battler)
        || (gBattleMons[battler].status1 & (STATUS1_SLEEP | STATUS1_FREEZE)))
        return FALSE;
    // EnemyCanFlee has no escape-item exemption. Ghost and active Run Away
    // bypass traps, but never bypass its final sleep/freeze check.
    if (GetBattlerAbilityIgnoreMoldBreaker(battler) == ABILITY_RUN_AWAY
        || IS_BATTLER_OF_TYPE(battler, TYPE_GHOST))
        return TRUE;
    return !IsAbilityPreventingEscape(battler)
        && !gBattleMons[battler].volatiles.escapePrevention
        && !gBattleMons[battler].volatiles.wrapped;
}

static bool32 OpponentWillFlee(enum BattlerId battler)
{
    return IsOnPlayerSide(battler) && (gBattleTypeFlags & BATTLE_TYPE_ROAMER)
        && UsesPolishedEscapeRules(GetOppositeBattler(battler))
        && CanPolishedRoamerChooseFlee(GetOppositeBattler(battler));
}

bool32 CanPolishedRoamerChooseFlee(enum BattlerId battler)
{
    // Source CheckLockedIn precedes TryEnemyFlee. Rechecking an already chosen
    // flee action remains a separate EnemyCanFlee operation.
    return !gBattleMons[battler].volatiles.multipleTurns
        && !gBattleMons[battler].volatiles.rechargeTimer
        && !gBattleMons[battler].volatiles.rampageTurns
        && !gBattleMons[battler].volatiles.rolloutTimer
        && CanPolishedEnemyFlee(battler);
}

u8 PolishedRunPreflight(enum BattlerId battler)
{
    if (OpponentWillFlee(battler)
        || (IsBattlerAlive(battler) && GetBattlerAbilityIgnoreMoldBreaker(battler) == ABILITY_RUN_AWAY)
        || GetItemHoldEffect(gBattleMons[battler].item) == HOLD_EFFECT_CAN_ALWAYS_RUN
        || IS_BATTLER_OF_TYPE(battler, TYPE_GHOST))
        return BATTLE_RUN_SUCCESS;

    // Source move traps are checked before trapping abilities, including the
    // message selected when both restrictions are present.
    if (gBattleMons[battler].volatiles.escapePrevention || gBattleMons[battler].volatiles.wrapped)
    {
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_CANT_ESCAPE;
        return BATTLE_RUN_FORBIDDEN;
    }
    u32 trapper = IsAbilityPreventingEscape(battler);
    if (trapper)
    {
        gBattleScripting.battler = trapper - 1;
        gLastUsedAbility = GetBattlerAbilityIgnoreMoldBreaker(trapper - 1);
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_PREVENTS_ESCAPE;
        return BATTLE_RUN_FAILURE;
    }
    return BATTLE_RUN_SUCCESS;
}

bool32 TryPolishedRun(enum BattlerId battler)
{
    bool32 success;
    if (PolishedRunPreflight(battler) != BATTLE_RUN_SUCCESS)
        return FALSE;
    gProtectStructs[battler].fleeType = 0;
    if (OpponentWillFlee(battler))
        success = TRUE;
    else if (IsBattlerAlive(battler) && GetBattlerAbilityIgnoreMoldBreaker(battler) == ABILITY_RUN_AWAY)
    {
        gLastUsedAbility = ABILITY_RUN_AWAY;
        gProtectStructs[battler].fleeType = FLEE_ABILITY;
        success = TRUE;
    }
    else if (GetItemHoldEffect(gBattleMons[battler].item) == HOLD_EFFECT_CAN_ALWAYS_RUN)
    {
        gPotentialItemEffectBattler = battler;
        gLastUsedItem = gBattleMons[battler].item;
        gProtectStructs[battler].fleeType = FLEE_ITEM;
        success = TRUE;
    }
    else if (IS_BATTLER_OF_TYPE(battler, TYPE_GHOST))
        success = TRUE;
    else
    {
        enum BattlerId opponent = GetOppositeBattler(battler);
        u32 speed = GetBattlerTotalSpeedStat(battler, GetBattlerAbilityIgnoreMoldBreaker(battler), GetBattlerHoldEffect(battler));
        u32 enemySpeed = GetBattlerTotalSpeedStat(opponent, GetBattlerAbilityIgnoreMoldBreaker(opponent), GetBattlerHoldEffect(opponent));
        u32 threshold = PolishedEscapeThreshold(speed, enemySpeed, gBattleStruct->runTries++);
        success = threshold == 256 || RandomUniform(RNG_POLISHED_ESCAPE, 0, MAX_u8) < threshold;
    }
    if (success)
    {
        gCurrentTurnActionNumber = gBattlersCount;
        gBattleOutcome = B_OUTCOME_RAN;
    }
    return success;
}

u32 PolishedRoamerSavedStatus(u32 status)
{
    if (status & STATUS1_TOXIC_POISON)
        status = (status & ~(STATUS1_TOXIC_POISON | STATUS1_TOXIC_COUNTER)) | STATUS1_POISON;
    return status;
}
