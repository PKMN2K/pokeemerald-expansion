#include "global.h"
#include "battle.h"
#include "battle_hold_effects.h"
#include "battle_setup.h"
#include "battle_util.h"
#include "battle_controllers.h"
#include "battle_ai_record.h"
#include "battle_stat_change.h"
#include "battle_gimmick.h"
#include "battle_scripts.h"
#include "battle_switch_in.h"
#include "polished_lifecycle.h"
#include "polished_moves.h"
#include "polished_damage.h"
#include "constants/battle.h"
#include "constants/battle_string_ids.h"
#include "constants/abilities.h"
#include "constants/items.h"
#include "constants/moves.h"

// Transient scheduler state only; no party/save layout changes.
struct PolishedFaintQueue
{
    u8 order[MAX_BATTLERS_COUNT];
    u8 count;
    u8 index;
    u8 step;
};
static EWRAM_DATA struct PolishedFaintQueue sPolishedTurnFaints = {0};
static EWRAM_DATA struct PolishedFaintQueue sPolishedEntryFaints = {0};
static EWRAM_DATA struct PolishedFaintQueue sPolishedActionFaints = {0};
static EWRAM_DATA bool8 sPolishedActionBatch = FALSE;
static EWRAM_DATA bool8 sPolishedActionResolving = FALSE;
static EWRAM_DATA enum BattlerId sPolishedActionAttacker = 0;
static EWRAM_DATA enum BattlerId sPolishedActionTarget = 0;
static EWRAM_DATA bool8 sPolishedEntryBatch = FALSE;
static EWRAM_DATA bool8 sPolishedEntryResolving = FALSE;
static EWRAM_DATA struct
{
    struct PolishedFaintQueue queue;
    bool8 batch, resolving;
} sPolishedEntryParents[2 * PARTY_SIZE + 1] = {0};
static EWRAM_DATA u8 sPolishedEntryParentCount = 0;
static EWRAM_DATA enum BattlerId sPolishedReplacementOrder[2] = {0};
static EWRAM_DATA u8 sPolishedReplacementIndex = 0;
static EWRAM_DATA bool8 sPolishedReplacingBoth = FALSE;
static EWRAM_DATA u8 sPolishedForcedEntry = 0;
static EWRAM_DATA struct
{
    enum BattlerId order[2];
    enum BattlerId attacker, target, scripting;
    u8 phase, index, block;
    bool8 active;
} sPolishedExitItems = {0};
static EWRAM_DATA struct
{
    u8 phase;
    bool8 active;
    bool8 initialized;
    bool8 batching;
    bool8 deferringItems;
} sPolishedEndTurn = {0};

bool32 UsesPolishedLifecycle(void)
{
    return UsesPolishedBattleRules(B_BATTLER_0, B_BATTLER_1)
        && !(gBattleTypeFlags & (BATTLE_TYPE_LINK
             | BATTLE_TYPE_FRONTIER | BATTLE_TYPE_TRAINER_HILL | BATTLE_TYPE_EREADER_TRAINER
             | BATTLE_TYPE_SAFARI | BATTLE_TYPE_CATCH_TUTORIAL | BATTLE_TYPE_POKEDUDE));
}

void ResetPolishedEndTurn(void)
{
    memset(&sPolishedEndTurn, 0, sizeof(sPolishedEndTurn));
    memset(&sPolishedTurnFaints, 0, sizeof(sPolishedTurnFaints));
    memset(&sPolishedEntryFaints, 0, sizeof(sPolishedEntryFaints));
    memset(&sPolishedActionFaints, 0, sizeof(sPolishedActionFaints));
    sPolishedActionBatch = sPolishedActionResolving = FALSE;
    sPolishedEntryBatch = sPolishedEntryResolving = FALSE;
    sPolishedEntryParentCount = 0;
    ResetPolishedEntryItems();
    ResetPolishedStatItems();
    ResetPolishedPreActionItems();
    sPolishedReplacingBoth = FALSE;
    sPolishedForcedEntry = 0;
    memset(&sPolishedExitItems, 0, sizeof(sPolishedExitItems));
}

void PolishedRecordForcedEntry(enum BattlerId battler, bool32 forced)
{
    sPolishedForcedEntry &= ~(1u << battler);
    if (forced)
        sPolishedForcedEntry |= 1u << battler;
}

bool32 PolishedIsForcedEntry(enum BattlerId battler)
{
    return (sPolishedForcedEntry & (1u << battler)) != 0;
}

bool32 PolishedEndTurnDefersFaints(void)
{
    return !sPolishedEntryResolving && !sPolishedActionResolving
        && (sPolishedActionBatch || sPolishedEntryBatch || (sPolishedEndTurn.active && sPolishedEndTurn.batching));
}

bool32 UsesPolishedActionRules(enum Move move)
{
    return UsesPolishedLifecycle() && move < MOVES_COUNT && gPolishedMovePP[move]
        && !IsPolishedFutureSightImpact(gBattlerAttacker, gBattlerTarget, move);
}

void PolishedBeginActionFaints(enum Move move)
{
    if (UsesPolishedActionRules(move) && !sPolishedActionBatch && !sPolishedEndTurn.active)
    {
        memset(&sPolishedActionFaints, 0, sizeof(sPolishedActionFaints));
        sPolishedActionBatch = TRUE;
        sPolishedActionResolving = FALSE;
    }
}

void PolishedPrioritizeActionFaint(enum BattlerId battler)
{
    if (!sPolishedActionBatch || IsBattlerAlive(battler))
        return;
    PolishedQueueEndTurnFaint(battler);
    for (u32 i = 0; i < sPolishedActionFaints.count; i++)
        if (sPolishedActionFaints.order[i] == battler)
        {
            for (u32 j = i; j > 0; j--)
                sPolishedActionFaints.order[j] = sPolishedActionFaints.order[j - 1];
            sPolishedActionFaints.order[0] = battler;
            break;
        }
}

bool32 PolishedEndTurnDefersItems(void)
{
    return sPolishedEndTurn.active && sPolishedEndTurn.deferringItems;
}

bool32 PolishedQueueEndTurnFaint(enum BattlerId battler)
{
    if (!PolishedEndTurnDefersFaints())
        return FALSE;
    struct PolishedFaintQueue *queue = sPolishedEntryBatch ? &sPolishedEntryFaints
        : sPolishedActionBatch ? &sPolishedActionFaints : &sPolishedTurnFaints;
    if (!IsBattlerAlive(battler) && !gBattleStruct->battlerState[battler].notOnField)
    {
        for (u32 i = 0; i < queue->count; i++)
            if (queue->order[i] == battler)
                return TRUE;
        queue->order[queue->count++] = battler;
    }
    return TRUE;
}

void SortPolishedBattlersBySpeed(enum BattlerId *order)
{
    struct BattleCalcValues cv = {0};
    cv.battlerAtk = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);
    cv.battlerDef = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);
    for (u32 i = 0; i < gBattlersCount; i++)
    {
        cv.abilities[i] = GetBattlerAbility(i);
        cv.holdEffects[i] = GetBattlerHoldEffect(i);
    }
    s32 first = GetWhichBattlerFasterOrTies(&cv, TRUE);
    enum BattleSide side = first == 0 ? RandomUniform(RNG_POLISHED_END_TURN_SPEED_TIE, 0, 1)
        : first > 0 ? B_SIDE_PLAYER : B_SIDE_OPPONENT;
    order[0] = GetBattlerAtPosition(side == B_SIDE_PLAYER ? B_POSITION_PLAYER_LEFT : B_POSITION_OPPONENT_LEFT);
    order[1] = GetOppositeBattler(order[0]);
}

void PolishedOrderReplacements(void)
{
    if (!UsesPolishedLifecycle())
        return;
    SortPolishedBattlersBySpeed(sPolishedReplacementOrder);
    sPolishedReplacementIndex = 0;
    sPolishedReplacingBoth = TRUE;
    gBattlerFainted = sPolishedReplacementOrder[0];
}

bool32 PolishedNextReplacement(void)
{
    if (!sPolishedReplacingBoth)
        return FALSE;
    if (++sPolishedReplacementIndex == ARRAY_COUNT(sPolishedReplacementOrder))
    {
        sPolishedReplacingBoth = FALSE;
        gBattlerFainted = gBattlersCount;
    }
    else
    {
        gBattlerFainted = sPolishedReplacementOrder[sPolishedReplacementIndex];
    }
    return TRUE;
}

static enum BattlerId GetBattlerSideForMessage(enum BattleSide side)
{
    enum BattlerId battler;

    for (battler = 0; battler < gBattlersCount; battler++)
    {
        if (GetBattlerSide(battler) == side)
            break;
    }

    return battler;
}

static bool32 HandleEndTurnOrder(enum BattlerId battler)
{
    bool32 effect = FALSE;

    gBattleTurnCounter++;
    gBattleStruct->eventState.endTurn++;

    for (enum BattlerId i = 0; i < gBattlersCount; i++)
        gBattlerByTurnOrder[i] = i;
    SortBattlersBySpeed(gBattlerByTurnOrder, FALSE);

    return effect;
}

static bool32 HandleEndTurnVarious(enum BattlerId battler)
{
    bool32 effect = FALSE;

    gBattleStruct->eventState.endTurn++;

    if (gFieldTimers.fairyLockTimer > 0 && --gFieldTimers.fairyLockTimer == 0)
        gFieldStatuses &= ~STATUS_FIELD_FAIRY_LOCK;

    for (enum BattleSide i = 0; i < NUM_BATTLE_SIDES; i++)
    {
        if (gSideTimers[i].damageNonTypesTimer > 0 && --gSideTimers[i].damageNonTypesTimer == 0)
            gSideStatuses[i] &= ~SIDE_STATUS_DAMAGE_NON_TYPES;
    }

    for (enum BattlerId i = 0; i < gBattlersCount; i++)
    {
        if (gBattleMons[i].volatiles.throatChopTimer > 0)
        {
            gBattleMons[i].volatiles.throatChopTimer--;
            if (gBattleMons[i].volatiles.uproarTurns)
                gBattleMons[i].volatiles.uproarTurns = 1; // end the move this turn
        }

        if (gBattleMons[i].volatiles.lockOn > 0 && --gBattleMons[i].volatiles.lockOn == 0)
            gBattleMons[i].volatiles.battlerWithSureHit = 0;

        if (B_CHARGE < GEN_9 && gBattleMons[i].volatiles.chargeTimer > 0)
            gBattleMons[i].volatiles.chargeTimer--;

        if (gBattleMons[i].volatiles.laserFocusTimer > 0)
            gBattleMons[i].volatiles.laserFocusTimer--;

        gBattleStruct->battlerState[i].wasAboveHalfHp = gBattleMons[i].hp > gBattleMons[i].maxHP / 2;
    }

    if (gBattleStruct->incrementEchoedVoice)
    {
        if (gBattleStruct->echoedVoiceCounter < 4)
            gBattleStruct->echoedVoiceCounter++;
        gBattleStruct->incrementEchoedVoice = FALSE;
    }
    else
    {
        gBattleStruct->echoedVoiceCounter = 0;
    }

    return effect;
}

static bool32 HandleEndTurnWeather(enum BattlerId battler)
{
    gBattleStruct->eventState.endTurn++;
    return EndOrContinueWeather();
}

static bool32 HandleEndTurnWeatherDamage(enum BattlerId battler)
{
    bool32 effect = FALSE;

    enum Ability ability = GetBattlerAbility(battler);
    enum BattleWeather currBattleWeather = GetBattleWeather(gBattleWeather);

    if (currBattleWeather == BATTLE_WEATHER_NONE)
    {
        // If there is no weather on the field, no need to check other battlers so go to next state
        gBattleStruct->eventState.endTurnBattler = 0;
        gBattleStruct->eventState.endTurn++;
        return effect;
    }

    gBattleStruct->eventState.endTurnBattler++;

    if (!IsBattlerPresent(battler) || !HasWeatherEffect())
        return effect;


    switch (currBattleWeather)
    {
    case BATTLE_WEATHER_FOG:
    case BATTLE_WEATHER_STRONG_WINDS:
        break;
    case BATTLE_WEATHER_RAIN:
    case BATTLE_WEATHER_RAIN_PRIMAL:
    case BATTLE_WEATHER_RAIN_DOWNPOUR:
        if (ability == ABILITY_DRY_SKIN || ability == ABILITY_RAIN_DISH)
        {
            if (AbilityBattleEffects(ABILITYEFFECT_ENDTURN, battler, ability, MOVE_NONE, TRUE))
                effect = TRUE;
        }
        break;
    case BATTLE_WEATHER_SUN:
    case BATTLE_WEATHER_SUN_PRIMAL:
        if (ability == ABILITY_DRY_SKIN || ability == ABILITY_SOLAR_POWER)
        {
            if (AbilityBattleEffects(ABILITYEFFECT_ENDTURN, battler, ability, MOVE_NONE, TRUE))
                effect = TRUE;
        }
        break;
    case BATTLE_WEATHER_SANDSTORM:
        if (ability != ABILITY_SAND_VEIL
         && ability != ABILITY_SAND_FORCE
         && ability != ABILITY_SAND_RUSH
         && ability != ABILITY_OVERCOAT
         && !IS_BATTLER_ANY_TYPE(battler, TYPE_ROCK, TYPE_GROUND, TYPE_STEEL)
         && gBattleMons[battler].volatiles.semiInvulnerable != STATE_UNDERGROUND
         && gBattleMons[battler].volatiles.semiInvulnerable != STATE_UNDERWATER
         && GetBattlerHoldEffect(battler) != HOLD_EFFECT_SAFETY_GOGGLES
         && !IsAbilityAndRecord(battler, ability, ABILITY_MAGIC_GUARD))
        {
            SetPassiveDamageAmount(battler, GetNonDynamaxMaxHP(battler) / 16);
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SANDSTORM;
            BattleScriptCall(BattleScript_DamagingWeather);
            effect = TRUE;
        }
        break;
    case BATTLE_WEATHER_HAIL:
    case BATTLE_WEATHER_SNOW:
        if (ability == ABILITY_ICE_BODY)
        {
            if (AbilityBattleEffects(ABILITYEFFECT_ENDTURN, battler, ability, MOVE_NONE, TRUE))
                effect = TRUE;
        }
        else if (currBattleWeather == BATTLE_WEATHER_HAIL)
        {
            if (ability != ABILITY_SNOW_CLOAK
             && ability != ABILITY_OVERCOAT
             && !(ability == ABILITY_SLUSH_RUSH && UsesPolishedBattleRules(battler, GetOppositeBattler(battler)))
             && !IS_BATTLER_OF_TYPE(battler, TYPE_ICE)
             && gBattleMons[battler].volatiles.semiInvulnerable != STATE_UNDERGROUND
             && gBattleMons[battler].volatiles.semiInvulnerable != STATE_UNDERWATER
             && GetBattlerHoldEffect(battler) != HOLD_EFFECT_SAFETY_GOGGLES
             && !IsAbilityAndRecord(battler, ability, ABILITY_MAGIC_GUARD))
            {
                SetPassiveDamageAmount(battler, GetNonDynamaxMaxHP(battler) / 16);
                gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_HAIL;
                BattleScriptCall(BattleScript_DamagingWeather);
                effect = TRUE;
            }
        }
        break;
    case BATTLE_WEATHER_NONE:
    case BATTLE_WEATHER_COUNT:
        break;
    }

    return effect;
}

static bool32 HandleEndTurnSendOutReplacements(enum BattlerId battler)
{
    gBattleStruct->eventState.endTurnBattler++;

    switch (gSpecialStatuses[battler].queuedSwitch)
    {
    case NO_QUEUED_SWITCH:
        break;
    case QUEUED_SWITCH_SEND_REPLACEMENT:
        gBattleScripting.battler = battler;
        BattleScriptCall(BattleScript_QueuedSwitch);
        return TRUE;
    case QUEUED_SWITCH_OPEN_PARTY_SCREEN:
        gBattleScripting.battler = battler;
        BattleScriptCall(BattleScript_QueuedSwitchOpenPartyScreen);
        return TRUE;
    default:
        errorf("Invalid value - queuedSwitch");
        break;
    }
    return FALSE;
}

static bool32 HandleEndTurnAffection(enum BattlerId battler)
{
    bool32 effect = FALSE;

    gBattleStruct->eventState.endTurnBattler++;

    if (UsesPolishedLifecycle())
    {
        // HandleAffectionSelfCure is ordered after weather and before Future
        // Sight, checks either party, and is independent of ability suppression.
        if (IsBattlerAlive(battler) && GetPolishedAffectionLevel(battler) >= 2
         && RandomUniform(RNG_POLISHED_AFFECTION_CURE, 0, 4) == 0
         && gBattleMons[battler].status1 != STATUS1_NONE)
        {
            gBattlerAttacker = battler;
            gBattleCommunication[MULTISTRING_CHOOSER] = 1;
            BattleScriptCall(BattleScript_AffectionBasedStatusHeal);
            return TRUE;
        }
        return FALSE;
    }

    if (!B_AFFECTION_MECHANICS
     || !IsOnPlayerSide(battler))
        return effect;

    if (GetBattlerAffectionHearts(gBattlerAttacker) >= AFFECTION_FOUR_HEARTS && (Random() % 100 < 20))
    {
        gBattleCommunication[MULTISTRING_CHOOSER] = 1;
        BattleScriptCall(BattleScript_AffectionBasedStatusHeal);
        effect = TRUE;
    }

    return effect;
}

static bool32 IsFutureSightAttackerInParty(enum BattlerId battlerAtk, enum BattlerId battlerDef)
{
    struct Pokemon *party = GetBattlerParty(battlerAtk);
    if (gBattleStruct->futureSight[battlerDef].move == MOVE_FUTURE_SIGHT
     && UsesPolishedBattleRules(battlerAtk, battlerDef) && !IsBattlerAlive(battlerAtk))
        return TRUE;
    if (IsDoubleBattle())
    {
        return &party[gBattleStruct->futureSight[battlerDef].partyIndex] != &party[gBattlerPartyIndexes[battlerAtk]]
            && &party[gBattleStruct->futureSight[battlerDef].partyIndex] != &party[gBattlerPartyIndexes[GetPartnerBattler(battlerAtk)]];
    }

    return &party[gBattleStruct->futureSight[battlerDef].partyIndex] != &party[gBattlerPartyIndexes[battlerAtk]];
}

// Note: Technically Future Sight, Doom Desire and Wish need a queue but
// I think we should accept this slight inconsistency so custom moves don't have to touch this code
static bool32 HandleEndTurnFutureSight(enum BattlerId battler)
{
    bool32 effect = FALSE;

    // Source timers belong to the attacking side. CheckSpeed is repeated at
    // this phase (including Trick Room and a fresh tie), not taken from the
    // native start-of-end-turn order. battlerSide is otherwise unused here and
    // the dispatcher clears it when the two singles battlers are complete.
    if (B_POLISHED_MOVE_BALANCE && UsesPolishedBattleRules(battler, GetOppositeBattler(battler))
     && (gBattleStruct->futureSight[battler].counter || gBattleStruct->futureSight[GetOppositeBattler(battler)].counter)
     && gBattleStruct->futureSight[battler].move != MOVE_DOOM_DESIRE
     && gBattleStruct->futureSight[GetOppositeBattler(battler)].move != MOVE_DOOM_DESIRE)
    {
        if (gBattleStruct->eventState.endTurnBattler == 0)
        {
            struct BattleCalcValues cv = {0};
            cv.battlerAtk = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);
            cv.battlerDef = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);
            for (u32 i = 0; i < gBattlersCount; i++)
            {
                cv.abilities[i] = GetBattlerAbility(i);
                cv.holdEffects[i] = GetBattlerHoldEffect(i);
            }
            s32 first = GetWhichBattlerFasterOrTies(&cv, TRUE);
            gBattleStruct->eventState.battlerSide = first == 0
                ? RandomUniform(RNG_POLISHED_FUTURE_SIGHT_SPEED_TIE, 0, 1) : first > 0 ? B_SIDE_PLAYER : B_SIDE_OPPONENT;
        }
        enum BattleSide attackerSide = gBattleStruct->eventState.battlerSide ^ gBattleStruct->eventState.endTurnBattler;
        battler = GetBattlerAtPosition(attackerSide == B_SIDE_PLAYER ? B_POSITION_OPPONENT_LEFT : B_POSITION_PLAYER_LEFT);
    }

    gBattleStruct->eventState.endTurnBattler++;

    if (gBattleStruct->futureSight[battler].counter > 0
     && --gBattleStruct->futureSight[battler].counter == 0)
    {
        if (!IsBattlerPresent(battler)) // Looks like a bug in doubles?
            return effect;

        if (gBattleStruct->futureSight[battler].move == MOVE_FUTURE_SIGHT)
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_FUTURE_SIGHT;
        else
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_DOOM_DESIRE;

        PREPARE_MOVE_BUFFER(gBattleTextBuff1, gBattleStruct->futureSight[battler].move);

        gBattlerTarget = battler;
        gBattlerAttacker = gBattleStruct->futureSight[battler].battlerIndex;
        gCurrentMove = gBattleStruct->futureSight[battler].move;
        gBattleStruct->eventState.atkCanceler = CANCELER_TARGET_FAILURE;
        if (gCurrentMove == MOVE_FUTURE_SIGHT && UsesPolishedBattleRules(gBattlerAttacker, gBattlerTarget))
        {
            // This entry skips CancelerClearFlags and CancelerNoTarget. Do not
            // inherit their loop state from a charging move or an end-turn item.
            gBattleStruct->eventState.moveEndBlock = 0;
            gBattleStruct->eventState.atkCancelerBattler = 0;
        }

        if (IsFutureSightAttackerInParty(gBattlerAttacker, gBattlerTarget))
            gSpecialStatuses[gBattlerAttacker].attackerInParty = TRUE;
        else
            SetTypeBeforeUsingMove(gCurrentMove, gBattlerAttacker, GetBattlerAbility(gBattlerAttacker), GetBattlerHoldEffect(gBattlerAttacker));

        gBattleScripting.animTurn = 1; // to play correct anim
        BattleScriptCall(BattleScript_MonTookFutureAttack);
        effect = TRUE;
    }

    return effect;
}

static bool32 HandleEndTurnWish(enum BattlerId battler)
{
    bool32 effect = FALSE;

    gBattleStruct->eventState.endTurnBattler++;

    if (gBattleStruct->wish[battler].counter > 0
     && --gBattleStruct->wish[battler].counter == 0
     && IsBattlerPresent(battler))
    {
        s32 wishHeal = 0;
        gBattlerTarget = battler;
        PREPARE_MON_NICK_WITH_PREFIX_BUFFER(gBattleTextBuff1, battler, gBattleStruct->wish[battler].partyId)
        if (GetConfig(B_WISH_HP_SOURCE) >= GEN_5)
            wishHeal = GetMonData(&GetBattlerParty(battler)[gBattleStruct->wish[battler].partyId], MON_DATA_MAX_HP) / 2;
        else
            wishHeal = GetNonDynamaxMaxHP(battler) / 2;

        SetHealAmount(battler, wishHeal);

        if (gBattleMons[battler].volatiles.healBlockTimer)
            BattleScriptCall(BattleScript_WishButHealBlocked);
        else if (gBattleMons[battler].hp == gBattleMons[battler].maxHP)
            BattleScriptCall(BattleScript_WishButFullHp);
        else
            BattleScriptCall(BattleScript_WishComesTrue);

        effect = TRUE;
    }

    return effect;
}

static bool32 HandleEndTurnFirstEventBlock(enum BattlerId battler)
{
    bool32 effect = FALSE;
    enum BattleSide side;

    if (gSpecialStatuses[battler].queuedSwitch != NO_QUEUED_SWITCH)
    {
        // Grassy Terrain still heals when Emergency Exit/Wimp Out activate
        if (gBattleStruct->eventState.endTurnBlock <= FIRST_EVENT_BLOCK_GRASSY_TERRAIN_HEAL)
        {
            gBattleStruct->eventState.endTurnBlock = FIRST_EVENT_BLOCK_GRASSY_TERRAIN_HEAL;
        }
        else
        {
            gBattleStruct->eventState.endTurnBlock = 0;
            gBattleStruct->eventState.endTurnBattler++;
            return effect;
        }
    }
    else if (!IsBattlerAlive(battler))
    {
        gBattleStruct->eventState.endTurnBlock = 0;
        gBattleStruct->eventState.endTurnBattler++;
        return effect;
    }

    switch (gBattleStruct->eventState.endTurnBlock)
    {
    case FIRST_EVENT_BLOCK_GMAX_MOVE_RESIDUAL: // TODO: Has to be split into 3 statuses and needs a queue
        side = GetBattlerSide(battler);
        if (gSideStatuses[side] & SIDE_STATUS_DAMAGE_NON_TYPES)
        {
            if (!IS_BATTLER_OF_TYPE(battler, gSideTimers[side].damageNonTypesType)
             && !IsAbilityAndRecord(battler, GetBattlerAbility(battler), ABILITY_MAGIC_GUARD))
            {
                SetPassiveDamageAmount(battler, GetNonDynamaxMaxHP(battler) / 6);
                ChooseDamageNonTypesString(gSideTimers[side].damageNonTypesType);
                BattleScriptCall(BattleScript_DamageNonTypesContinues);
                effect = TRUE;
            }
        }
        gBattleStruct->eventState.endTurnBlock++;
        break;
    case FIRST_EVENT_BLOCK_SEA_OF_FIRE_DAMAGE:
        side = GetBattlerSide(battler);
        if ((gSideStatuses[side] & SIDE_STATUS_SEA_OF_FIRE)
         && !IS_BATTLER_OF_TYPE(battler, TYPE_FIRE)
         && !IsAbilityAndRecord(battler, GetBattlerAbility(battler), ABILITY_MAGIC_GUARD))
        {
            gBattlerAttacker = battler;
            SetPassiveDamageAmount(battler, GetNonDynamaxMaxHP(battler) / 8);
            BtlController_EmitStatusAnimation(battler, B_COMM_TO_CONTROLLER, FALSE, STATUS1_BURN);
            MarkBattlerForControllerExec(battler);
            BattleScriptCall(BattleScript_HurtByTheSeaOfFire);
            effect = TRUE;
        }
        gBattleStruct->eventState.endTurnBlock++;
        break;
    case FIRST_EVENT_BLOCK_THRASH:
        if (!UsesPolishedBattleRules(battler, GetOppositeBattler(battler))
         && GetConfig(B_RAMPAGE_CONFUSION) < GEN_5
         && gBattleMons[battler].volatiles.rampageTurns
         && gBattleMons[battler].volatiles.semiInvulnerable != STATE_SKY_DROP_TARGET)
        {
            gBattleMons[battler].volatiles.rampageTurns--;
            if (gBattleMons[battler].volatiles.unableToUseMove)
            {
                CancelMultiTurnMoves(battler);
            }
            else if (!gBattleMons[battler].volatiles.rampageTurns && gBattleMons[battler].volatiles.multipleTurns)
            {
                gBattleMons[battler].volatiles.multipleTurns = FALSE;
                if (!gBattleMons[battler].volatiles.confusionTimer)
                {
                    SetMoveEffectHelper(battler, battler, MOVE_EFFECT_CONFUSION, gBattlescriptCurrInstr, EFFECT_PRIMARY);
                    if (gBattleMons[battler].volatiles.confusionTimer)
                        BattleScriptCall(BattleScript_ThrashConfuses);
                    effect = TRUE;
                }
            }
        }
        gBattleStruct->eventState.endTurnBlock++;
        break;
    case FIRST_EVENT_BLOCK_GRASSY_TERRAIN_HEAL:
        if (gFieldTimers.terrain == B_TERRAIN_GRASSY
         && !IsBattlerAtMaxHp(battler)
         && !gBattleMons[battler].volatiles.healBlockTimer
         && !IsSemiInvulnerable(battler, CHECK_ALL)
         && IsBattlerGrounded(battler, GetBattlerAbility(battler), GetBattlerHoldEffect(battler)))
        {
            SetHealAmount(battler, GetNonDynamaxMaxHP(battler) / 16);
            BattleScriptCall(BattleScript_GrassyTerrainHeals);
            effect = TRUE;
        }
        gBattleStruct->eventState.endTurnBlock++;
        break;
    case FIRST_EVENT_BLOCK_ABILITIES:
    {
        enum Ability ability = GetBattlerAbility(battler);
        switch (ability)
        {
        case ABILITY_HEALER:
        case ABILITY_HYDRATION:
        case ABILITY_SHED_SKIN:
            if (AbilityBattleEffects(ABILITYEFFECT_ENDTURN, battler, ability, MOVE_NONE, TRUE))
                effect = TRUE;
            break;
        default:
            break;
        }
        gBattleStruct->eventState.endTurnBlock++;
        break;
    }
    case FIRST_EVENT_BLOCK_HEAL_ITEMS:
        if (ItemBattleEffects(battler, 0, GetBattlerHoldEffect(battler), IsLeftoversActivation))
            effect = TRUE;
        gBattleStruct->eventState.endTurnBlock = 0;
        gBattleStruct->eventState.endTurnBattler++;
        break;
    case FIRST_EVENT_BLOCK_STATUS_ABILITIES:
        gBattleStruct->eventState.endTurnBlock++;
        if (sPolishedEndTurn.active
         && !((gBattleStruct->polishedNappingBattlers & (1u << battler))
           && (gBattleMons[battler].status1 & STATUS1_SLEEP)
           && (GetBattlerAbility(battler) == ABILITY_INSOMNIA || GetBattlerAbility(battler) == ABILITY_VITAL_SPIRIT)))
            effect = TryImmunityAbilityHealStatus(battler) != IMMUNITY_NO_EFFECT;
        break;
    }

    return effect;
}

static bool32 HandleEndTurnAquaRing(enum BattlerId battler)
{
    bool32 effect = FALSE;

    gBattleStruct->eventState.endTurnBattler++;

    if (gBattleMons[battler].volatiles.aquaRing
     && !gBattleMons[battler].volatiles.healBlockTimer
     && !IsBattlerAtMaxHp(battler)
     && IsBattlerPresent(battler))
    {
        SetHealAmount(battler, GetDrainedBigRootHp(battler, GetNonDynamaxMaxHP(battler) / 16));
        BattleScriptCall(BattleScript_AquaRingHeal);
        effect = TRUE;
    }

    return effect;
}

static bool32 HandleEndTurnIngrain(enum BattlerId battler)
{
    bool32 effect = FALSE;

    gBattleStruct->eventState.endTurnBattler++;

    if (gBattleMons[battler].volatiles.root
     && !gBattleMons[battler].volatiles.healBlockTimer
     && !IsBattlerAtMaxHp(battler)
     && IsBattlerPresent(battler))
    {
        SetHealAmount(battler, GetDrainedBigRootHp(battler, GetNonDynamaxMaxHP(battler) / 16));
        BattleScriptCall(BattleScript_IngrainTurnHeal);
        effect = TRUE;
    }

    return effect;
}

static bool32 HandleEndTurnLeechSeed(enum BattlerId battler)
{
    bool32 effect = FALSE;
    enum BattlerId drainedBattler = battler;
    enum BattlerId receiverBattler = gBattleMons[drainedBattler].volatiles.leechSeed;

    gBattleStruct->eventState.endTurnBattler++;

    if (receiverBattler == 0)
        return effect;

    receiverBattler = receiverBattler - 1;

    if (IsBattlerPresent(drainedBattler)
     && IsBattlerPresent(receiverBattler)
     && !IsAbilityAndRecord(drainedBattler, GetBattlerAbility(drainedBattler), ABILITY_MAGIC_GUARD))
    {
        gBattlerAttacker = receiverBattler;
        gBattlerAbility = gBattleScripting.battler = drainedBattler;
        gBattleScripting.animArg1 = receiverBattler;
        gBattleScripting.animArg2 = drainedBattler;

        s32 drainAmount = max(1, GetNonDynamaxMaxHP(drainedBattler) / 8);
        s32 healAmount = GetDrainedBigRootHp(receiverBattler, drainAmount);
        // Source GetHPAbsorption halves even Leech Seed's nominal eighth,
        // after Big Root, with a one-HP floor. It does not use actual HP lost.
        if (UsesPolishedLifecycle())
            healAmount = max(1, healAmount / 2);

        if (GetBattlerAbility(drainedBattler) == ABILITY_LIQUID_OOZE)
        {
            SetPassiveDamageAmount(drainedBattler, drainAmount);
            SetPassiveDamageAmount(receiverBattler, healAmount);
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_LEECH_SEED_OOZE;
            BattleScriptCall(UsesPolishedLifecycle() ? BattleScript_PolishedLeechSeedOoze : BattleScript_LeechSeedTurnDrainLiquidOoze);
        }
        else if (gBattleMons[receiverBattler].volatiles.healBlockTimer)
        {
            BattleScriptCall(BattleScript_LeechSeedTurnDrainHealBlock);
        }
        else
        {
            SetPassiveDamageAmount(drainedBattler, drainAmount);
            SetHealAmount(receiverBattler, healAmount);
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_LEECH_SEED_DRAIN;
            BattleScriptCall(BattleScript_LeechSeedTurnDrainRecovery);
        }
        effect = TRUE;
    }

    return effect;
}

static bool32 HandleEndTurnPoison(enum BattlerId battler)
{
    bool32 effect = FALSE;

    enum Ability ability = GetBattlerAbility(battler);
    bool32 isToxicPoison = gBattleMons[battler].status1 & STATUS1_TOXIC_POISON;

    gBattleStruct->eventState.endTurnBattler++;

    if (IsBattlerPresent(battler) && (gBattleMons[battler].status1 & STATUS1_POISON || isToxicPoison))
    {
        if (IsAbilityAndRecord(battler, ability, ABILITY_MAGIC_GUARD))
        {
            if (!UsesPolishedLifecycle() && isToxicPoison && (gBattleMons[battler].status1 & STATUS1_TOXIC_COUNTER) != STATUS1_TOXIC_TURN(15)) // not 16 turns
                gBattleMons[battler].status1 += STATUS1_TOXIC_TURN(1);
        }
        else if (ability == ABILITY_POISON_HEAL)
        {
            if (isToxicPoison && (gBattleMons[battler].status1 & STATUS1_TOXIC_COUNTER) != STATUS1_TOXIC_TURN(15)) // not 16 turns
                gBattleMons[battler].status1 += STATUS1_TOXIC_TURN(1);

            if (!IsBattlerAtMaxHp(battler) && !gBattleMons[battler].volatiles.healBlockTimer)
            {
                SetHealAmount(battler, GetNonDynamaxMaxHP(battler) / 8);
                BattleScriptCall(BattleScript_PoisonHealActivates);
                effect = TRUE;
            }
        }
        else if (isToxicPoison)
        {
            SetPassiveDamageAmount(battler, GetNonDynamaxMaxHP(battler) / 16);
            if ((gBattleMons[battler].status1 & STATUS1_TOXIC_COUNTER) != STATUS1_TOXIC_TURN(15)) // not 16 turns
                gBattleMons[battler].status1 += STATUS1_TOXIC_TURN(1);
            gBattleStruct->passiveHpUpdate[battler] *= (gBattleMons[battler].status1 & STATUS1_TOXIC_COUNTER) >> 8;
            BattleScriptCall(BattleScript_PoisonTurnDmg);
            effect = TRUE;
        }
        else
        {
            SetPassiveDamageAmount(battler, GetNonDynamaxMaxHP(battler) / 8);
            BattleScriptCall(BattleScript_PoisonTurnDmg);
            effect = TRUE;
        }
    }

    return effect;
}

static bool32 HandleEndTurnBurn(enum BattlerId battler)
{
    bool32 effect = FALSE;

    enum Ability ability = GetBattlerAbility(battler);

    gBattleStruct->eventState.endTurnBattler++;

    if (gBattleMons[battler].status1 & STATUS1_BURN
     && IsBattlerPresent(battler)
     && !IsAbilityAndRecord(battler, ability, ABILITY_MAGIC_GUARD))
    {
        s32 burnDamage = GetNonDynamaxMaxHP(battler) / ((UsesPolishedLifecycle() || GetConfig(B_BURN_DAMAGE) >= GEN_7 || GetConfig(B_BURN_DAMAGE) == GEN_1) ? 16 : 8);
        if (ability == ABILITY_HEATPROOF && !UsesPolishedLifecycle())
        {
            if (burnDamage > (burnDamage / 2) + 1) // Record ability if the burn takes less damage than it normally would.
                RecordAbilityBattle(battler, ABILITY_HEATPROOF);
            burnDamage /= 2;
        }
        SetPassiveDamageAmount(battler, burnDamage);
        BattleScriptCall(BattleScript_BurnTurnDmg);
        effect = TRUE;
    }

    return effect;
}

static bool32 HandleEndTurnFrostbite(enum BattlerId battler)
{
    bool32 effect = FALSE;

    gBattleStruct->eventState.endTurnBattler++;

    if (gBattleMons[battler].status1 & STATUS1_FROSTBITE
     && IsBattlerPresent(battler)
     && !IsAbilityAndRecord(battler, GetBattlerAbility(battler), ABILITY_MAGIC_GUARD))
    {
        SetPassiveDamageAmount(battler, GetNonDynamaxMaxHP(battler) / ((GetConfig(B_BURN_DAMAGE) >= GEN_7 || GetConfig(B_BURN_DAMAGE) == GEN_1) ? 16 : 8));
        BattleScriptCall(BattleScript_FrostbiteTurnDmg);
        effect = TRUE;
    }

    return effect;
}

static bool32 HandleEndTurnNightmare(enum BattlerId battler)
{
    bool32 effect = FALSE;

    gBattleStruct->eventState.endTurnBattler++;

    if (gBattleMons[battler].volatiles.nightmare
     && IsBattlerPresent(battler)
     && !IsAbilityAndRecord(battler, GetBattlerAbility(battler), ABILITY_MAGIC_GUARD))
    {
        if (IsAsleepOrComatose(battler, GetBattlerAbility(battler)))
        {
            SetPassiveDamageAmount(battler, GetNonDynamaxMaxHP(battler) / 4);
            BattleScriptCall(BattleScript_NightmareTurnDmg);
            effect = TRUE;
        }
        else
        {
            gBattleMons[battler].volatiles.nightmare = FALSE;
        }
    }

    return effect;
}

static bool32 HandleEndTurnCurse(enum BattlerId battler)
{
    bool32 effect = FALSE;

    gBattleStruct->eventState.endTurnBattler++;

    if (gBattleMons[battler].volatiles.cursed
     && IsBattlerPresent(battler)
     && !IsAbilityAndRecord(battler, GetBattlerAbility(battler), ABILITY_MAGIC_GUARD))
    {
        SetPassiveDamageAmount(battler, GetNonDynamaxMaxHP(battler) / 4);
        BattleScriptCall(BattleScript_CurseTurnDmg);
        effect = TRUE;
    }

    return effect;
}

static bool32 HandleEndTurnWrap(enum BattlerId battler)
{
    bool32 effect = FALSE;

    gBattleStruct->eventState.endTurnBattler++;

    if (gBattleMons[battler].volatiles.wrapped
     && IsBattlerPresent(battler))
    {
        enum Move move = gBattleMons[battler].volatiles.wrappedMove;
        bool32 polishedBinding = B_POLISHED_MOVE_BALANCE
            && (move == MOVE_WRAP || move == MOVE_FIRE_SPIN || move == MOVE_WHIRLPOOL)
            && !IsDoubleBattle()
            && GetActiveGimmick(battler) == GIMMICK_NONE
            && GetActiveGimmick(GetOppositeBattler(battler)) == GIMMICK_NONE;

        // The source returns before decrementing (including on the release turn).
        if (polishedBinding
         && (IsAbilityAndRecord(battler, GetBattlerAbility(battler), ABILITY_MAGIC_GUARD)
          || gBattleMons[battler].volatiles.substitute))
            return effect;

        if (gBattleMons[battler].volatiles.wrapTurns != 0)
        {
            gBattleMons[battler].volatiles.wrapTurns--;
            if (!polishedBinding && IsAbilityAndRecord(battler, GetBattlerAbility(battler), ABILITY_MAGIC_GUARD))
                return effect;

            gBattleScripting.animArg1 = gBattleMons[battler].volatiles.wrappedMove;
            gBattleScripting.animArg2 = gBattleMons[battler].volatiles.wrappedMove >> 8;
            PREPARE_MOVE_BUFFER(gBattleTextBuff1, gBattleMons[battler].volatiles.wrappedMove);
            BattleScriptCall(BattleScript_WrapTurnDmg);
            s32 bindDamage = 0;
            // Source reads the opponent's current item, rather than an application snapshot.
            if (polishedBinding)
                bindDamage = GetNonDynamaxMaxHP(battler) / (GetBattlerHoldEffect(GetOppositeBattler(battler)) == HOLD_EFFECT_BINDING_BAND ? 6 : 8);
            else if (gBattleMons[battler].volatiles.wrappedBindingBand)
                bindDamage = GetNonDynamaxMaxHP(battler) / (GetConfig(B_BINDING_DAMAGE) >= GEN_6 ? 6 : 8);
            else
                bindDamage = GetNonDynamaxMaxHP(battler) / (GetConfig(B_BINDING_DAMAGE) >= GEN_6 ? 8 : 16);
            SetPassiveDamageAmount(battler, bindDamage);
        }
        else  // broke free
        {
            gBattleMons[battler].volatiles.wrapped = FALSE;
            PREPARE_MOVE_BUFFER(gBattleTextBuff1, gBattleMons[battler].volatiles.wrappedMove);
            BattleScriptCall(BattleScript_WrapEnds);
        }
        effect = TRUE;
    }

    return effect;
}

static bool32 HandleEndTurnSaltCure(enum BattlerId battler)
{
    bool32 effect = FALSE;

    gBattleStruct->eventState.endTurnBattler++;

    if (gBattleMons[battler].volatiles.saltCure
     && IsBattlerPresent(battler)
     && !IsAbilityAndRecord(battler, GetBattlerAbility(battler), ABILITY_MAGIC_GUARD))
    {
        s32 saltCureDamage = 0;
        if (IS_BATTLER_ANY_TYPE(battler, TYPE_STEEL, TYPE_WATER))
            saltCureDamage = GetNonDynamaxMaxHP(battler) / (GetConfig(B_SALT_CURE_DAMAGE) >= GEN_CHAMPIONS ? 8 : 4);
        else
            saltCureDamage = GetNonDynamaxMaxHP(battler) / (GetConfig(B_SALT_CURE_DAMAGE) >= GEN_CHAMPIONS ? 16 : 8);
        SetPassiveDamageAmount(battler, saltCureDamage);
        PREPARE_MOVE_BUFFER(gBattleTextBuff1, MOVE_SALT_CURE);
        BattleScriptCall(BattleScript_SaltCureExtraDamage);
        effect = TRUE;
    }

    return effect;
}

static bool32 HandleEndTurnOctolock(enum BattlerId battler)
{
    bool32 effect = FALSE;

    gBattleStruct->eventState.endTurnBattler++;

    if (gBattleMons[battler].volatiles.octolock && IsBattlerAlive(battler))
    {
        gBattlerTarget = battler;
        gBattlerAttacker = gBattleMons[battler].volatiles.battlerPreventingEscape;
        SetStatChange(battler, STAT_DEF, -1);
        SetStatChange(battler, STAT_SPDEF, -1);
        BattleScriptCall(BattleScript_EndTurnStatChange);
        effect = TRUE;
    }

    return effect;
}

static bool32 HandleEndTurnSyrupBomb(enum BattlerId battler)
{
    bool32 effect = FALSE;

    gBattleStruct->eventState.endTurnBattler++;

    if (gBattleMons[battler].volatiles.syrupBomb
     && IsBattlerPresent(battler))
    {
        if (gBattleMons[battler].volatiles.syrupBombTimer > 0 && --gBattleMons[battler].volatiles.syrupBombTimer == 0)
            gBattleMons[battler].volatiles.syrupBomb = FALSE;
        PREPARE_MOVE_BUFFER(gBattleTextBuff1, MOVE_SYRUP_BOMB);
        gBattlerTarget = battler;
        gBattlerAttacker = gBattleMons[battler].volatiles.stickySyrupedBy;
        SetStatChange(battler, STAT_SPEED, -1);
        BattleScriptCall(BattleScript_SyrupBombEndTurn);
        effect = TRUE;
    }

    return effect;
}

static bool32 HandleEndTurnTaunt(enum BattlerId battler)
{
    bool32 effect = FALSE;

    gBattleStruct->eventState.endTurnBattler++;

    if (gBattleMons[battler].volatiles.tauntTimer && --gBattleMons[battler].volatiles.tauntTimer == 0)
    {
        gBattleScripting.battler = battler;
        BattleScriptCall(BattleScript_BufferEndTurn);
        PREPARE_MOVE_BUFFER(gBattleTextBuff1, MOVE_TAUNT);
        effect = TRUE;
    }

    return effect;
}

static bool32 HandleEndTurnTorment(enum BattlerId battler)
{
    bool32 effect = FALSE;

    gBattleStruct->eventState.endTurnBattler++;

    if (gBattleMons[battler].volatiles.tormentTimer > 0 && --gBattleMons[battler].volatiles.tormentTimer == 0)
    {
        gBattleMons[battler].volatiles.torment = FALSE;
        gBattleScripting.battler = battler;
        BattleScriptCall(BattleScript_TormentEnds);
        effect = TRUE;
    }

    return effect;
}

static bool32 HandleEndTurnEncore(enum BattlerId battler)
{
    bool32 effect = FALSE;

    gBattleStruct->eventState.endTurnBattler++;

    if (gBattleMons[battler].volatiles.encoreTimer != 0)
    {
        if (gBattleMons[battler].moves[gBattleMons[battler].volatiles.encoredMovePos] != gBattleMons[battler].volatiles.encoredMove)  // Pokémon does not have the encored move anymore
        {
            gBattleMons[battler].volatiles.encoredMove = 0;
            gBattleMons[battler].volatiles.encoreTimer = 0;
        }
        else if (--gBattleMons[battler].volatiles.encoreTimer == 0
         || gBattleMons[battler].pp[gBattleMons[battler].volatiles.encoredMovePos] == 0)
        {
            gBattleMons[battler].volatiles.encoredMove = 0;
            gBattleMons[battler].volatiles.encoreTimer = 0;
            gBattleScripting.battler = battler;
            BattleScriptCall(BattleScript_EncoredNoMore);
            effect = TRUE;
        }
    }

    return effect;
}

static bool32 HandleEndTurnDisable(enum BattlerId battler)
{
    bool32 effect = FALSE;

    u32 moveIndex = 0;
    gBattleStruct->eventState.endTurnBattler++;

    if (gBattleMons[battler].volatiles.disableTimer != 0)
    {
        for (moveIndex = 0; moveIndex < MAX_MON_MOVES; moveIndex++)
        {
            if (gBattleMons[battler].volatiles.disabledMove == gBattleMons[battler].moves[moveIndex])
                break;
        }
        if (moveIndex == MAX_MON_MOVES)  // Pokémon does not have the disabled move anymore
        {
            gBattleMons[battler].volatiles.disabledMove = 0;
            gBattleMons[battler].volatiles.disableTimer = 0;
        }
        else if (--gBattleMons[battler].volatiles.disableTimer == 0)  // disable ends
        {
            gBattleMons[battler].volatiles.disabledMove = 0;
            gBattleScripting.battler = battler;
            BattleScriptCall(BattleScript_DisabledNoMore);
            effect = TRUE;
        }
    }

    return effect;
}

static bool32 HandleEndTurnMagnetRise(enum BattlerId battler)
{
    bool32 effect = FALSE;

    gBattleStruct->eventState.endTurnBattler++;

    if (gBattleMons[battler].volatiles.magnetRiseTimer > 0 && --gBattleMons[battler].volatiles.magnetRiseTimer == 0)
    {
        BattleScriptCall(BattleScript_BufferEndTurn);
        PREPARE_STRING_BUFFER(gBattleTextBuff1, STRINGID_ELECTROMAGNETISM);
        effect = TRUE;
    }

    return effect;
}

static bool32 HandleEndTurnTelekinesis(enum BattlerId battler)
{
    bool32 effect = FALSE;

    gBattleStruct->eventState.endTurnBattler++;

    if (gBattleMons[battler].volatiles.telekinesisTimer > 0 && --gBattleMons[battler].volatiles.telekinesisTimer == 0)
    {
        gBattleMons[battler].volatiles.telekinesis = FALSE;
        BattleScriptCall(BattleScript_TelekinesisEndTurn);
        effect = TRUE;
    }

    return effect;
}

static bool32 HandleEndTurnHealBlock(enum BattlerId battler)
{
    bool32 effect = FALSE;

    gBattleStruct->eventState.endTurnBattler++;

    if (gBattleMons[battler].volatiles.healBlockTimer > 0 && --gBattleMons[battler].volatiles.healBlockTimer == 0)
    {
        gBattleScripting.battler = battler;
        BattleScriptCall(BattleScript_HealBlockEndTurn);
        effect = TRUE;
    }

    return effect;
}

static bool32 HandleEndTurnEmbargo(enum BattlerId battler)
{
    bool32 effect = FALSE;

    gBattleStruct->eventState.endTurnBattler++;

    if (gBattleMons[battler].volatiles.embargoTimer > 0 && --gBattleMons[battler].volatiles.embargoTimer == 0)
    {
        BattleScriptCall(BattleScript_EmbargoEndTurn);
        effect = TRUE;
    }

    return effect;
}

static bool32 HandleEndTurnYawn(enum BattlerId battler)
{
    bool32 effect = FALSE;

    enum Ability ability = GetBattlerAbility(battler);

    gBattleStruct->eventState.endTurnBattler++;

    if (gBattleMons[battler].volatiles.yawn > 0)
    {
        gBattleMons[battler].volatiles.yawn--;
        if (!gBattleMons[battler].volatiles.yawn
         && !(gBattleMons[battler].status1 & STATUS1_ANY)
         && ability != ABILITY_VITAL_SPIRIT
         && ability != ABILITY_INSOMNIA
         && !UproarWakeUpCheck(battler)
         && !IsLeafGuardProtected(battler, ability))
        {
            gEffectBattler = gBattlerTarget = battler;
            enum HoldEffect holdEffect = GetBattlerHoldEffect(battler);
            if (IsElectricTerrainAffected(battler, ability, holdEffect, gFieldTimers.terrain))
            {
                gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TERRAINPREVENTS_ELECTRIC;
                BattleScriptCall(BattleScript_TerrainPrevents);
            }
            else if (IsMistyTerrainAffected(battler, ability, holdEffect, gFieldTimers.terrain))
            {
                gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TERRAINPREVENTS_MISTY;
                BattleScriptCall(BattleScript_TerrainPrevents);
            }
            else if (IsSleepClauseActiveForSide(GetBattlerSide(battler)))
            {
                BattleScriptCall(BattleScript_SleepClausePrevents);
            }
            else if ((gBattleScripting.battler = IsAbilityOnSide(battler, ABILITY_SWEET_VEIL)))
            {
                gBattleScripting.battler--;
                gLastUsedAbility = ABILITY_SWEET_VEIL;
                gBattlerAbility = gBattleScripting.battler;
                RecordAbilityBattle(gBattleScripting.battler, ABILITY_SWEET_VEIL);
                BattleScriptCall(BattleScript_ImmunityProtectedRet);
            }
            else
            {
                if (GetConfig(B_SLEEP_TURNS) >= GEN_CHAMPIONS)
                {
                    if (Random() % 3 == 0)
                        gBattleMons[battler].status1 |= STATUS1_SLEEP_TURN(2);
                    else
                        gBattleMons[battler].status1 |= STATUS1_SLEEP_TURN(3);
                }
                else if (GetConfig(B_SLEEP_TURNS) >= GEN_5)
                    gBattleMons[battler].status1 |= (RandomUniform(RNG_SLEEP_TURNS, 2, 4));
                else if (GetConfig(B_SLEEP_TURNS) >= GEN_3)
                    gBattleMons[battler].status1 |= (RandomUniform(RNG_SLEEP_TURNS, 2, 5));
                else
                    gBattleMons[battler].status1 |= (RandomUniform(RNG_SLEEP_TURNS, 2, 8));

                CancelMultiTurnMoves(battler);
                TryActivateSleepClause(battler, gBattlerPartyIndexes[battler]);
                BtlController_EmitSetMonData(battler, B_COMM_TO_CONTROLLER, REQUEST_STATUS_BATTLE, 0, 4, &gBattleMons[battler].status1);
                MarkBattlerForControllerExec(battler);
                BattleScriptCall(BattleScript_YawnMakesAsleep);
            }
            effect = TRUE;
        }
    }

    return effect;
}

static bool32 HandleEndTurnPerishSong(enum BattlerId battler)
{
    bool32 effect = FALSE;

    gBattleStruct->eventState.endTurnBattler++;

    if (gBattleMons[battler].volatiles.perishSong
     && IsBattlerPresent(battler))
    {
        PREPARE_BYTE_NUMBER_BUFFER(gBattleTextBuff1, 1, gBattleMons[battler].volatiles.perishSongTimer);
        if (gBattleMons[battler].volatiles.perishSongTimer == 0)
        {
            gBattleMons[battler].volatiles.perishSong = FALSE;
            SetPassiveDamageAmount(battler, gBattleMons[battler].hp);
            BattleScriptCall(BattleScript_PerishSongTakesLife);
        }
        else
        {
            gBattleMons[battler].volatiles.perishSongTimer--;
            BattleScriptCall(BattleScript_PerishSongCountGoesDown);
        }
        effect = TRUE;
    }

    return effect;
}

static bool32 HandleEndTurnRoost(enum BattlerId battler)
{
    bool32 effect = FALSE;

    gBattleStruct->eventState.endTurnBattler++;

    if (gBattleMons[battler].volatiles.roostActive)
        gBattleMons[battler].volatiles.roostActive = FALSE;

    return effect;
}

static bool32 HandleEndTurnSecondEventBlock(enum BattlerId battler)
{
    bool32 effect = FALSE;

    enum BattleSide side = gBattleStruct->eventState.battlerSide;

    switch (gBattleStruct->eventState.endTurnBlock)
    {
    case SECOND_EVENT_BLOCK_REFLECT:
        if (gSideTimers[side].reflectTimer > 0 && --gSideTimers[side].reflectTimer == 0)
        {
            gBattleScripting.battler = GetBattlerSideForMessage(side);
            gSideStatuses[side] &= ~SIDE_STATUS_REFLECT;
            BattleScriptCall(BattleScript_SideStatusWoreOff);
            gBattleCommunication[MULTISTRING_CHOOSER] = side;
            PREPARE_MOVE_BUFFER(gBattleTextBuff1, MOVE_REFLECT);
            effect = TRUE;
        }
        gBattleStruct->eventState.endTurnBlock++;
        break;
    case SECOND_EVENT_BLOCK_LIGHT_SCREEN:
        if (gSideTimers[side].lightscreenTimer > 0 && --gSideTimers[side].lightscreenTimer == 0)
        {
            gBattleScripting.battler = GetBattlerSideForMessage(side);
            gSideStatuses[side] &= ~SIDE_STATUS_LIGHTSCREEN;
            BattleScriptCall(BattleScript_SideStatusWoreOff);
            gBattleCommunication[MULTISTRING_CHOOSER] = side;
            PREPARE_MOVE_BUFFER(gBattleTextBuff1, MOVE_LIGHT_SCREEN);
            effect = TRUE;
        }
        gBattleStruct->eventState.endTurnBlock++;
        break;
    case SECOND_EVENT_BLOCK_SAFEGUARD:
        if (gSideTimers[side].safeguardTimer > 0 && --gSideTimers[side].safeguardTimer == 0)
        {
            gBattlerAttacker = GetBattlerSideForMessage(side);
            gSideStatuses[side] &= ~SIDE_STATUS_SAFEGUARD;
            BattleScriptCall(BattleScript_SafeguardEnds);
            effect = TRUE;
        }
        gBattleStruct->eventState.endTurnBlock++;
        break;
    case SECOND_EVENT_BLOCK_MIST:
        if (gSideTimers[side].mistTimer > 0 && --gSideTimers[side].mistTimer == 0)
        {
            gBattleScripting.battler = GetBattlerSideForMessage(side);
            gSideStatuses[side] &= ~SIDE_STATUS_MIST;
            BattleScriptCall(BattleScript_SideStatusWoreOff);
            gBattleCommunication[MULTISTRING_CHOOSER] = side;
            PREPARE_MOVE_BUFFER(gBattleTextBuff1, MOVE_MIST);
            effect = TRUE;
        }
        gBattleStruct->eventState.endTurnBlock++;
        break;
    case SECOND_EVENT_BLOCK_TAILWIND:
        if (gSideTimers[side].tailwindTimer > 0 && --gSideTimers[side].tailwindTimer == 0)
        {
            gBattlerAttacker = GetBattlerSideForMessage(side);
            gSideStatuses[side] &= ~SIDE_STATUS_TAILWIND;
            BattleScriptCall(BattleScript_TailwindEnds);
            effect = TRUE;
        }
        gBattleStruct->eventState.endTurnBlock++;
        break;
    case SECOND_EVENT_BLOCK_LUCKY_CHANT:
        if (gSideTimers[side].luckyChantTimer > 0 && --gSideTimers[side].luckyChantTimer == 0)
        {
            gBattlerAttacker = GetBattlerSideForMessage(side);
            gSideStatuses[side] &= ~SIDE_STATUS_LUCKY_CHANT;
            BattleScriptCall(BattleScript_LuckyChantEnds);
            effect = TRUE;
        }
        gBattleStruct->eventState.endTurnBlock++;
        break;
    case SECOND_EVENT_BLOCK_RAINBOW:
        if (gSideTimers[side].rainbowTimer > 0 && --gSideTimers[side].rainbowTimer == 0)
        {
            gBattlerAttacker = GetBattlerSideForMessage(side);
            gSideStatuses[side] &= ~SIDE_STATUS_RAINBOW;
            BattleScriptCall(BattleScript_TheRainbowDisappeared);
            effect = TRUE;
        }
        gBattleStruct->eventState.endTurnBlock++;
            break;
    case SECOND_EVENT_BLOCK_SEA_OF_FIRE:
        if (gSideTimers[side].seaOfFireTimer > 0 && --gSideTimers[side].seaOfFireTimer == 0)
        {
            gBattlerAttacker = GetBattlerSideForMessage(side);
            gSideStatuses[side] &= ~SIDE_STATUS_SEA_OF_FIRE;
            BattleScriptCall(BattleScript_TheSeaOfFireDisappeared);
            effect = TRUE;
        }
        gBattleStruct->eventState.endTurnBlock++;
        break;
    case SECOND_EVENT_BLOCK_SWAMP:
        gBattlerAttacker = GetBattlerSideForMessage(side);
        if (gSideTimers[side].swampTimer > 0 && --gSideTimers[side].swampTimer == 0)
        {
            gBattlerAttacker = GetBattlerSideForMessage(side);
            gSideStatuses[side] &= ~SIDE_STATUS_SWAMP;
            BattleScriptCall(BattleScript_TheSwampDisappeared);
            effect = TRUE;
        }
        gBattleStruct->eventState.endTurnBlock++;
        break;
    case SECOND_EVENT_BLOCK_AURORA_VEIL:
        if (gSideTimers[side].auroraVeilTimer > 0 && --gSideTimers[side].auroraVeilTimer == 0)
        {
            gBattleScripting.battler = GetBattlerSideForMessage(side);
            gSideStatuses[side] &= ~SIDE_STATUS_AURORA_VEIL;
            BattleScriptCall(BattleScript_SideStatusWoreOff);
            gBattleCommunication[MULTISTRING_CHOOSER] = side;
            PREPARE_MOVE_BUFFER(gBattleTextBuff1, MOVE_AURORA_VEIL);
            effect = TRUE;
        }
        gBattleStruct->eventState.battlerSide++;
        gBattleStruct->eventState.endTurnBlock = 0;
        break;
    }

    return effect;
}

static bool32 HandleEndTurnTrickRoom(enum BattlerId battler)
{
    bool32 effect = FALSE;

    gBattleStruct->eventState.endTurn++;

    if (gFieldTimers.trickRoomTimer > 0 && --gFieldTimers.trickRoomTimer == 0)
    {
        gFieldStatuses &= ~STATUS_FIELD_TRICK_ROOM;
        BattleScriptCall(BattleScript_TrickRoomEnds);
        effect = TRUE;
    }

    return effect;
}

static bool32 HandleEndTurnGravity(enum BattlerId battler)
{
    bool32 effect = FALSE;

    gBattleStruct->eventState.endTurn++;

    if (gFieldTimers.gravityTimer > 0 && --gFieldTimers.gravityTimer == 0)
    {
        gFieldStatuses &= ~STATUS_FIELD_GRAVITY;
        BattleScriptCall(BattleScript_GravityEnds);
        effect = TRUE;
    }

    return effect;
}

static bool32 HandleEndTurnWaterSport(enum BattlerId battler)
{
    bool32 effect = FALSE;

    gBattleStruct->eventState.endTurn++;

    if (gFieldTimers.waterSportTimer > 0 && --gFieldTimers.waterSportTimer == 0)
    {
        gFieldStatuses &= ~STATUS_FIELD_WATERSPORT;
        BattleScriptCall(BattleScript_WaterSportEnds);
        effect = TRUE;
    }

    return effect;
}

static bool32 HandleEndTurnMudSport(enum BattlerId battler)
{
    bool32 effect = FALSE;

    gBattleStruct->eventState.endTurn++;

    if (gFieldTimers.mudSportTimer > 0 && --gFieldTimers.mudSportTimer == 0)
    {
        gFieldStatuses &= ~STATUS_FIELD_MUDSPORT;
        BattleScriptCall(BattleScript_MudSportEnds);
        effect = TRUE;
    }

    return effect;
}

static bool32 HandleEndTurnWonderRoom(enum BattlerId battler)
{
    bool32 effect = FALSE;

    gBattleStruct->eventState.endTurn++;

    if (gFieldTimers.wonderRoomTimer > 0 && --gFieldTimers.wonderRoomTimer == 0)
    {
        gFieldStatuses &= ~STATUS_FIELD_WONDER_ROOM;
        BattleScriptCall(BattleScript_WonderRoomEnds);
        effect = TRUE;
    }

    return effect;
}

static bool32 HandleEndTurnMagicRoom(enum BattlerId battler)
{
    bool32 effect = FALSE;

    gBattleStruct->eventState.endTurn++;

    if (gFieldTimers.magicRoomTimer > 0 && --gFieldTimers.magicRoomTimer == 0)
    {
        gFieldStatuses &= ~STATUS_FIELD_MAGIC_ROOM;
        BattleScriptCall(BattleScript_MagicRoomEnds);
        effect = TRUE;
    }

    return effect;
}

static bool32 HandleEndTurnTerrain(enum BattlerId battler)
{
    gBattleStruct->eventState.endTurn++;

    if (gFieldTimers.terrainTimer > 0 && --gFieldTimers.terrainTimer == 0)
    {
        TryToRevertMimicryAndFlags();
        gBattleCommunication[MULTISTRING_CHOOSER] = gBattleTerrainInfo[gFieldTimers.terrain].endMessage;
        gBattleScripting.battler = gBattlerAttacker;
        BattleScriptCall(BattleScript_TerrainEnds);
        gFieldTimers.terrain = B_TERRAIN_NONE;
        return TRUE;
    }

    return FALSE;
}

static bool32 HandleEndTurnThirdEventBlock(enum BattlerId battler)
{
    bool32 effect = FALSE;

    if (!IsBattlerPresent(battler))
    {
        gBattleStruct->eventState.endTurnBattler++;
        return effect;
    }

    switch (gBattleStruct->eventState.endTurnBlock)
    {
    case THIRD_EVENT_BLOCK_UPROAR:
        if (gBattleMons[battler].volatiles.uproarTurns)
        {
            for (gEffectBattler = 0; gEffectBattler < gBattlersCount; gEffectBattler++)
            {
                if (GetConfig(B_UPROAR) >= GEN_5) // This effect is only present in pre-Gen 5 Uproar
                    break;

                bool32 hasSoundproof = GetConfig(B_UPROAR_IGNORE_SOUNDPROOF) < GEN_5 && GetBattlerAbility(gEffectBattler) == ABILITY_SOUNDPROOF;

                if ((gBattleMons[gEffectBattler].status1 & STATUS1_SLEEP)
                 && !hasSoundproof)
                {
                    gBattleMons[gEffectBattler].status1 &= ~STATUS1_SLEEP;
                    gBattleMons[gEffectBattler].volatiles.nightmare = FALSE;
                    gBattleCommunication[MULTISTRING_CHOOSER] = 1;
                    BattleScriptCall(BattleScript_MonWokeUpInUproar);
                    BtlController_EmitSetMonData(gEffectBattler, B_COMM_TO_CONTROLLER, REQUEST_STATUS_BATTLE, 0, 4, &gBattleMons[gBattlerAttacker].status1);
                    MarkBattlerForControllerExec(gEffectBattler);
                    effect = TRUE;
                    break;
                }
            }
            if (effect == FALSE)
            {
                gBattlerAttacker = battler;
                gBattleMons[battler].volatiles.uproarTurns--; // uproar timer goes down
                if (gBattleMons[battler].volatiles.unableToUseMove)
                {
                    CancelMultiTurnMoves(battler);
                    gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_UPROAR_ENDS;
                }
                else if (gBattleMons[battler].volatiles.uproarTurns)
                {
                    gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_UPROAR_CONTINUES;
                    gBattleMons[battler].volatiles.multipleTurns = TRUE;
                }
                else
                {
                    gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_UPROAR_ENDS;
                    CancelMultiTurnMoves(battler);
                }
                BattleScriptCall(BattleScript_PrintUproarOverTurns);
                effect = TRUE;
            }
        }
        gBattleStruct->eventState.endTurnBlock++;
        break;
    case THIRD_EVENT_BLOCK_ABILITIES:
    {
        enum Ability ability = GetBattlerAbility(battler);
        switch (ability)
        {
        case ABILITY_TRUANT: // Not fully accurate but it has to be handled somehow. TODO: Implement the correct gen5+ behavior
        case ABILITY_CUD_CHEW:
        case ABILITY_SLOW_START:
        case ABILITY_BAD_DREAMS:
        case ABILITY_BALL_FETCH:
        case ABILITY_HARVEST:
        case ABILITY_MOODY:
        case ABILITY_PICKUP:
        case ABILITY_SPEED_BOOST:
            if (AbilityBattleEffects(ABILITYEFFECT_ENDTURN, battler, ability, MOVE_NONE, TRUE))
                effect = TRUE;
            break;
        default:
            break;
        }
        gBattleStruct->eventState.endTurnBlock++;
        break;
    }
    case THIRD_EVENT_BLOCK_ITEMS:
        // HandleStatusOrbs has no White Herb check in the source. Moody's
        // negative stage persists until a move-end or Intimidate checkpoint.
        if (!(sPolishedEndTurn.active && GetBattlerHoldEffect(battler) == HOLD_EFFECT_WHITE_HERB)
         && ItemBattleEffects(battler, 0, GetBattlerHoldEffect(battler), IsOrbsWhiteHerbActivation))
            effect = TRUE;
        gBattleStruct->eventState.endTurnBlock = 0;
        gBattleStruct->eventState.endTurnBattler++;
        break;
    case THIRD_EVENT_BLOCK_STATUS_ABILITIES:
        gBattleStruct->eventState.endTurnBlock++;
        if (sPolishedEndTurn.active
         && !((gBattleStruct->polishedNappingBattlers & (1u << battler))
           && (gBattleMons[battler].status1 & STATUS1_SLEEP)
           && (GetBattlerAbility(battler) == ABILITY_INSOMNIA || GetBattlerAbility(battler) == ABILITY_VITAL_SPIRIT)))
            effect = TryImmunityAbilityHealStatus(battler) != IMMUNITY_NO_EFFECT;
        break;
    }

    return effect;
}

static bool32 HandleEndTurnOpportunist(enum BattlerId battler)
{
    bool32 effect = FALSE;

    if (IsBattlerPresent(battler) && AbilityBattleEffects(ABILITYEFFECT_OPPORTUNIST, battler, GetBattlerAbility(battler), MOVE_NONE, TRUE))
        effect = TRUE;

    gBattleStruct->eventState.endTurnBattler++;
    return effect;
}

static bool32 HandleEndTurnMirrorHerb(enum BattlerId battler)
{
    bool32 effect = FALSE;

    if (IsBattlerPresent(battler) && ItemBattleEffects(battler, 0, GetBattlerHoldEffect(battler), IsMirrorHerbActivation))
        effect = TRUE;

    gBattleStruct->eventState.endTurnBattler++;
    return effect;
}

static bool32 HandleEndTurnFormChange(enum BattlerId battler)
{
    bool32 effect = FALSE;

    gBattleStruct->eventState.endTurnBattler++;

    if (!IsBattlerAlive(battler))
        return FALSE;

    enum Ability ability = GetBattlerAbility(battler);

    if (TryBattleFormChange(battler, FORM_CHANGE_BATTLE_TURN_END, ability)
        || TryBattleFormChange(battler, FORM_CHANGE_BATTLE_HP_PERCENT_TURN_END, ability))
    {
        gBattleScripting.battler = battler;
        gBattleScripting.abilityPopupOverwrite = ability; // To prevent the new form's ability from pop up
        switch (ability)
        {
        case ABILITY_POWER_CONSTRUCT:
            BattleScriptCall(BattleScript_PowerConstruct);
            break;
        case ABILITY_HUNGER_SWITCH:
            BattleScriptCall(BattleScript_BattlerFormChangeNoPopup);
            break;
        case ABILITY_ZEN_MODE:
            if (gBattleMons[battler].species == SPECIES_DARMANITAN_ZEN
            || gBattleMons[battler].species == SPECIES_DARMANITAN_GALAR_ZEN)
                gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_ZEN_MODE_TRIGGERED;
            else
                gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_ZEN_MODE_ENDED;
            BattleScriptCall(BattleScript_ZenMode);
            break;
        default:
            BattleScriptCall(BattleScript_BattlerFormChange); // Generic animation
            break;
        }
        effect = TRUE;
    }

    return effect;
}

static bool32 HandleEndTurnEjectPack(enum BattlerId battler)
{
    gBattleStruct->eventState.endTurn++;
    if (UsesPolishedLifecycle())
    {
        for (u32 i = 0; i < gBattlersCount; i++)
            gBattleMons[i].volatiles.tryEjectPack = FALSE;
        return FALSE;
    }
    return TrySwitchInEjectPack(END_TURN);
}

static bool32 TryEndTurnTrainerSlide(enum BattlerId battler)
{
    return ((ShouldDoTrainerSlide(battler, TRAINER_SLIDE_SELF_LAST_LOW_HP) != TRAINER_SLIDE_TARGET_NONE)
         || (ShouldDoTrainerSlide(battler, TRAINER_SLIDE_OPPONENT_LAST_LOW_HP) != TRAINER_SLIDE_TARGET_NONE)
         || (ShouldDoTrainerSlide(battler, TRAINER_SLIDE_SELF_LAST_HALF_HP) != TRAINER_SLIDE_TARGET_NONE)
         || (ShouldDoTrainerSlide(battler, TRAINER_SLIDE_OPPONENT_LAST_HALF_HP) != TRAINER_SLIDE_TARGET_NONE)
         || (ShouldDoTrainerSlide(battler, TRAINER_SLIDE_DEFENDER_TAKES_FIRST_CRITICAL_HIT) != TRAINER_SLIDE_TARGET_NONE)
         || (ShouldDoTrainerSlide(battler, TRAINER_SLIDE_ATTACKER_LANDS_FIRST_CRITICAL_HIT) != TRAINER_SLIDE_TARGET_NONE)
         || (ShouldDoTrainerSlide(battler, TRAINER_SLIDE_DEFENDER_TAKES_FIRST_SUPER_EFFECTIVE_HIT) != TRAINER_SLIDE_TARGET_NONE)
         || (ShouldDoTrainerSlide(battler, TRAINER_SLIDE_ATTACKER_LANDS_FIRST_SUPER_EFFECTIVE_HIT) != TRAINER_SLIDE_TARGET_NONE)
         || (ShouldDoTrainerSlide(battler, TRAINER_SLIDE_DEFENDER_TAKES_FIRST_STAB_MOVE) != TRAINER_SLIDE_TARGET_NONE)
         || (ShouldDoTrainerSlide(battler, TRAINER_SLIDE_ATTACKER_LANDS_FIRST_STAB_MOVE) != TRAINER_SLIDE_TARGET_NONE)
         || (ShouldDoTrainerSlide(battler, TRAINER_SLIDE_OPPONENT_MON_UNAFFECTED) != TRAINER_SLIDE_TARGET_NONE)
         || (ShouldDoTrainerSlide(battler, TRAINER_SLIDE_SELF_MON_UNAFFECTED) != TRAINER_SLIDE_TARGET_NONE));
}

static bool32 HandleEndTurnTrainerASlides(enum BattlerId battler)
{
    gBattleStruct->eventState.endTurnBattler++;
    bool32 slide = TryEndTurnTrainerSlide(B_BATTLER_1);
    if (slide == TRUE)
        BattleScriptCall(BattleScript_TrainerASlideMsgRet);
    return slide;
}

static bool32 HandleEndTurnTrainerBSlides(enum BattlerId battler)
{
    gBattleStruct->eventState.endTurnBattler++;

    if (!IsDoubleBattle())
        return FALSE;

    bool32 slide = TryEndTurnTrainerSlide(B_BATTLER_3);

    if (slide == TRUE)
    {
        if ((TRAINER_BATTLE_PARAM.opponentB == TRAINER_BATTLE_PARAM.opponentA)
        || (TRAINER_BATTLE_PARAM.opponentB == TRAINER_NONE)
        || (TRAINER_BATTLE_PARAM.opponentB == 0xFFFF))
            BattleScriptCall(BattleScript_TrainerASlideMsgRet);
        else
            BattleScriptCall(BattleScript_TrainerBSlideMsgRet);
    }

    return slide;
}

static bool32 HandleEndTurnTrainerPartnerSlides(enum BattlerId battler)
{
    gBattleStruct->eventState.endTurnBattler++;

    if (!IsDoubleBattle())
        return FALSE;

    bool32 slide = TryEndTurnTrainerSlide(B_BATTLER_2);

    if (slide == TRUE)
        BattleScriptCall(BattleScript_TrainerPartnerSlideMsgRet);

    return slide;
}

/*
 * Various end turn effects that happen after all battlers moved.
 * Each Case will apply the effects for each battler. Moving to the next case when all battlers are done.
 * If an effect is going to be applied on a better, the bool effect will be set to TRUE and a script set.
 * The script is set with `BattleScriptCall` and should have the ending `return`
   Example:
        BattleScriptCall(BattleScript_X);

        (in battle_scripts_1.s)
        BattleScript_X:
            some commands
            return
 */

static bool32 HandleEndTurnArenaTurnEnd(enum BattlerId battler)
{
    gBattleStruct->eventState.endTurn++;

    if ((gBattleTypeFlags & BATTLE_TYPE_ARENA)
     && gBattleStruct->eventState.arenaTurn == 2
     && IsBattlerAlive(GetBattlerAtPosition(B_POSITION_PLAYER_LEFT)) && IsBattlerAlive(GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT)))
    {
        for (enum BattlerId battler = 0; battler < 2; battler++)
            CancelMultiTurnMoves(battler);

        BattleScriptCall(BattleScript_ArenaDoJudgment);
        return TRUE;
    }
    return FALSE;
}

static bool32 HandleEndTurnFaintedMonActions(enum BattlerId battler)
{
    if (HandleFaintedMonActions())
        return TRUE;
    // A replacement can faint to hazards after the native faint-action loop
    // has already visited its slot. Source .endturn_loop repeats selection
    // until both slots are alive or an outcome is reached.
    if (sPolishedEndTurn.active && !gBattleOutcome)
    {
        for (u32 i = 0; i < gBattlersCount; i++)
        {
            if (!IsBattlerAlive(i) && !NoAliveMonsForBattlerSide(i))
            {
                gBattleStruct->eventState.faintedAction = FAINTED_ACTIONS_NO_MONS_TO_SWITCH;
                return FALSE;
            }
        }
    }
    gBattleStruct->eventState.endTurn++;
    return FALSE;
}

static bool32 HandleEndTurnDynamax(enum BattlerId battler)
{
    bool32 effect = FALSE;

    gBattleStruct->eventState.endTurnBattler++;

    if (GetActiveGimmick(battler) == GIMMICK_DYNAMAX
     && gBattleStruct->dynamax.dynamaxTurns[battler] > 0)
    {
        if (--gBattleStruct->dynamax.dynamaxTurns[battler] == 0
         || (gBattleOutcome == B_OUTCOME_WON && IsOnPlayerSide(battler) && !(gBattleTypeFlags & BATTLE_TYPE_LINK)))
        {
            gBattleScripting.battler = battler;
            gBattleStruct->dynamax.dynamaxTurns[battler] = 0;
            UndoDynamax(battler);
            BattleScriptCall(BattleScript_DynamaxEnds);
            effect = TRUE;
        }
    }

    return effect;
}

static bool32 (*const sEndTurnEffectHandlers[])(enum BattlerId battler) =
{
    [ENDTURN_ORDER] = HandleEndTurnOrder,
    [ENDTURN_VARIOUS] = HandleEndTurnVarious,
    [ENDTURN_WEATHER] = HandleEndTurnWeather,
    [ENDTURN_WEATHER_DAMAGE] = HandleEndTurnWeatherDamage,
    [ENDTURN_SEND_OUT_REPLACEMENTS_1] = HandleEndTurnSendOutReplacements,
    [ENDTURN_AFFECTION] = HandleEndTurnAffection,
    [ENDTURN_FUTURE_SIGHT] = HandleEndTurnFutureSight,
    [ENDTURN_WISH] = HandleEndTurnWish,
    [ENDTURN_FIRST_EVENT_BLOCK] = HandleEndTurnFirstEventBlock,
    [ENDTURN_SEND_OUT_REPLACEMENTS_2] = HandleEndTurnSendOutReplacements,
    [ENDTURN_AQUA_RING] = HandleEndTurnAquaRing,
    [ENDTURN_INGRAIN] = HandleEndTurnIngrain,
    [ENDTURN_LEECH_SEED] = HandleEndTurnLeechSeed,
    [ENDTURN_POISON] = HandleEndTurnPoison,
    [ENDTURN_BURN] = HandleEndTurnBurn,
    [ENDTURN_FROSTBITE] = HandleEndTurnFrostbite,
    [ENDTURN_NIGHTMARE] = HandleEndTurnNightmare,
    [ENDTURN_CURSE] = HandleEndTurnCurse,
    [ENDTURN_WRAP] = HandleEndTurnWrap,
    [ENDTURN_SALT_CURE] = HandleEndTurnSaltCure,
    [ENDTURN_OCTOLOCK] = HandleEndTurnOctolock,
    [ENDTURN_SYRUP_BOMB] = HandleEndTurnSyrupBomb,
    [ENDTURN_TAUNT] = HandleEndTurnTaunt,
    [ENDTURN_TORMENT] = HandleEndTurnTorment,
    [ENDTURN_ENCORE] = HandleEndTurnEncore,
    [ENDTURN_DISABLE] = HandleEndTurnDisable,
    [ENDTURN_MAGNET_RISE] = HandleEndTurnMagnetRise,
    [ENDTURN_TELEKINESIS] = HandleEndTurnTelekinesis,
    [ENDTURN_HEAL_BLOCK] = HandleEndTurnHealBlock,
    [ENDTURN_EMBARGO] = HandleEndTurnEmbargo,
    [ENDTURN_YAWN] = HandleEndTurnYawn,
    [ENDTURN_PERISH_SONG] = HandleEndTurnPerishSong,
    [ENDTURN_ROOST] = HandleEndTurnRoost,
    [ENDTURN_SEND_OUT_REPLACEMENTS_3] = HandleEndTurnSendOutReplacements,
    [ENDTURN_SECOND_EVENT_BLOCK] = HandleEndTurnSecondEventBlock,
    [ENDTURN_TRICK_ROOM] = HandleEndTurnTrickRoom,
    [ENDTURN_GRAVITY] = HandleEndTurnGravity,
    [ENDTURN_WATER_SPORT] = HandleEndTurnWaterSport,
    [ENDTURN_MUD_SPORT] = HandleEndTurnMudSport,
    [ENDTURN_WONDER_ROOM] = HandleEndTurnWonderRoom,
    [ENDTURN_MAGIC_ROOM] = HandleEndTurnMagicRoom,
    [ENDTURN_TERRAIN] = HandleEndTurnTerrain,
    [ENDTURN_THIRD_EVENT_BLOCK] = HandleEndTurnThirdEventBlock,
    [ENDTURN_OPPORTUNIST] = HandleEndTurnOpportunist,
    [ENDTURN_MIRROR_HERB] = HandleEndTurnMirrorHerb,
    [ENDTURN_SEND_OUT_REPLACEMENTS_4] = HandleEndTurnSendOutReplacements,
    [ENDTURN_FORM_CHANGE] = HandleEndTurnFormChange,
    [ENDTURN_EJECT_PACK] = HandleEndTurnEjectPack,
    [ENDTURN_SEND_OUT_REPLACEMENTS_5] = HandleEndTurnSendOutReplacements,
    [ENDTURN_ARENA_TURN_END] = HandleEndTurnArenaTurnEnd,
    [ENDTURN_FAINTED_MON_ACTIONS] = HandleEndTurnFaintedMonActions,
    [ENDTURN_DYNAMAX] = HandleEndTurnDynamax,
    [ENDTURN_TRAINER_A_SLIDES] = HandleEndTurnTrainerASlides,
    [ENDTURN_TRAINER_B_SLIDES] = HandleEndTurnTrainerBSlides,
    [ENDTURN_TRAINER_PARTNER_SLIDES] = HandleEndTurnTrainerPartnerSlides,
};

static bool32 HandleEndTurnEmergencyExit(enum BattlerId battler)
{
    bool32 effect = FALSE;
    enum Ability ability = GetBattlerAbility(battler);

    if (EmergencyExitCanBeTriggered(battler, ability))
    {
        gBattleScripting.battler = gBattlerAbility = battler;
        gLastUsedAbility = ability;
        gSpecialStatuses[battler].queuedSwitch = QUEUED_SWITCH_OPEN_PARTY_SCREEN;
        BattleScriptCall(BattleScript_EmergencyExit);
        effect = TRUE;
    }

    return effect;
}

// Source order: engine/battle/endturn.asm:HandleBetweenTurnEffects. Optional
// Expansion-only phases remain available between the corresponding source
// phases; they are inert in the source campaign. CHECKPOINT never switches.
static const u8 sPolishedEndTurnOrder[] =
{
    ENDTURN_ORDER, ENDTURN_VARIOUS, ENDTURN_POLISHED_CHECKPOINT,
    ENDTURN_WEATHER, ENDTURN_WEATHER_DAMAGE, ENDTURN_POLISHED_CHECKPOINT,
    ENDTURN_SEND_OUT_REPLACEMENTS_1,
    ENDTURN_AFFECTION, ENDTURN_FUTURE_SIGHT, ENDTURN_POLISHED_CHECKPOINT,
    ENDTURN_WISH, ENDTURN_FIRST_EVENT_BLOCK, ENDTURN_POLISHED_CHECKPOINT,
    ENDTURN_SEND_OUT_REPLACEMENTS_2, ENDTURN_AQUA_RING, ENDTURN_INGRAIN,
    ENDTURN_LEECH_SEED, ENDTURN_POLISHED_CHECKPOINT,
    ENDTURN_POISON, ENDTURN_POLISHED_CHECKPOINT,
    ENDTURN_BURN, ENDTURN_POLISHED_CHECKPOINT,
    ENDTURN_FROSTBITE, ENDTURN_NIGHTMARE,
    ENDTURN_CURSE, ENDTURN_POLISHED_CHECKPOINT,
    ENDTURN_WRAP, ENDTURN_POLISHED_CHECKPOINT,
    ENDTURN_SALT_CURE, ENDTURN_OCTOLOCK, ENDTURN_SYRUP_BOMB,
    ENDTURN_TAUNT, ENDTURN_TORMENT, ENDTURN_ENCORE, ENDTURN_DISABLE,
    ENDTURN_MAGNET_RISE, ENDTURN_TELEKINESIS, ENDTURN_HEAL_BLOCK,
    ENDTURN_EMBARGO, ENDTURN_YAWN,
    ENDTURN_PERISH_SONG, ENDTURN_POLISHED_CHECKPOINT,
    ENDTURN_ROOST, ENDTURN_SEND_OUT_REPLACEMENTS_3, ENDTURN_SECOND_EVENT_BLOCK,
    ENDTURN_TRICK_ROOM, ENDTURN_GRAVITY, ENDTURN_WATER_SPORT, ENDTURN_MUD_SPORT,
    ENDTURN_WONDER_ROOM, ENDTURN_MAGIC_ROOM, ENDTURN_TERRAIN,
    ENDTURN_THIRD_EVENT_BLOCK, ENDTURN_POLISHED_CHECKPOINT,
    ENDTURN_POLISHED_LEPPA, ENDTURN_POLISHED_HEALING_ITEMS,
    ENDTURN_OPPORTUNIST, ENDTURN_MIRROR_HERB, ENDTURN_SEND_OUT_REPLACEMENTS_4,
    ENDTURN_FORM_CHANGE, ENDTURN_EJECT_PACK, ENDTURN_SEND_OUT_REPLACEMENTS_5,
    ENDTURN_ARENA_TURN_END, ENDTURN_POLISHED_CHECKPOINT,
    ENDTURN_FAINTED_MON_ACTIONS, ENDTURN_DYNAMAX,
    ENDTURN_TRAINER_A_SLIDES, ENDTURN_TRAINER_B_SLIDES, ENDTURN_TRAINER_PARTNER_SLIDES,
};

static bool32 ResolvePolishedFaintQueue(struct PolishedFaintQueue *queue)
{
    // Resolve queued faints in damage order, then EXP, then the whole-party
    // outcome. A fainted opponent cannot erase the other battler's tick.
    while (queue->index < queue->count)
    {
        gBattleScripting.battler = queue->order[queue->index++];
        BattleScriptCall(BattleScript_PolishedEndTurnFaint);
        return TRUE;
    }
    if (queue->step == 0)
    {
        queue->step++;
        enum BattlerId foe = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);
        if (!IsBattlerAlive(foe)
         && !(gBattleStruct->givenExpMons[GetBattlerTrainer(foe) & BIT_FLANK] & (1u << gBattlerPartyIndexes[foe])))
        {
            gBattlerTarget = foe;
            BattleScriptCall(BattleScript_PolishedEndTurnExp);
            return TRUE;
        }
    }
    if (queue->step == 1)
    {
        queue->step++;
        BattleScriptCall(BattleScript_PolishedEndTurnOutcome);
        return TRUE;
    }
    memset(queue, 0, sizeof(*queue));
    return FALSE;
}

void PolishedPushEntryFaints(void)
{
    sPolishedEntryParents[sPolishedEntryParentCount].queue = sPolishedEntryFaints;
    sPolishedEntryParents[sPolishedEntryParentCount].batch = sPolishedEntryBatch;
    sPolishedEntryParents[sPolishedEntryParentCount++].resolving = sPolishedEntryResolving;
}

void PolishedPopEntryFaints(void)
{
    sPolishedEntryFaints = sPolishedEntryParents[--sPolishedEntryParentCount].queue;
    sPolishedEntryBatch = sPolishedEntryParents[sPolishedEntryParentCount].batch;
    sPolishedEntryResolving = sPolishedEntryParents[sPolishedEntryParentCount].resolving;
}

void PolishedBeginEntryHazards(void)
{
    memset(&sPolishedEntryFaints, 0, sizeof(sPolishedEntryFaints));
    sPolishedEntryBatch = TRUE;
    sPolishedEntryResolving = FALSE;
    // Source suppression is passive and precedes the entry announcement,
    // including hazards and the Speed comparison for both incoming battlers.
    for (u32 i = 0; i < gBattlersCount; i++)
        if (gBattleMons[i].ability == ABILITY_NEUTRALIZING_GAS
         && !gBattleMons[i].volatiles.gastroAcid && !gBattleStruct->battlerState[i].notOnField)
            gBattleMons[i].volatiles.neutralizingGas = TRUE;
}

bool32 PolishedResolveEntryFaints(void)
{
    sPolishedEntryBatch = FALSE;
    sPolishedEntryResolving = TRUE;
    if (ResolvePolishedFaintQueue(&sPolishedEntryFaints))
        return TRUE;
    sPolishedEntryResolving = FALSE;
    return FALSE;
}

static bool32 HandlePolishedFaintCheckpoint(void)
{
    if (ResolvePolishedFaintQueue(&sPolishedTurnFaints))
        return TRUE;
    gBattleStruct->eventState.endTurn++;
    return FALSE;
}

bool32 PolishedResolveActionFaints(void)
{
    if (!sPolishedActionBatch && !sPolishedActionResolving)
        return FALSE;
    if (!sPolishedActionResolving)
    {
        sPolishedActionAttacker = gBattlerAttacker;
        sPolishedActionTarget = gBattlerTarget;
        // Include direct KOs even when a move script bypassed move-end's
        // ordinary target-faint block. Earlier queued damage keeps its order.
        PolishedQueueEndTurnFaint(gBattlerTarget);
        PolishedQueueEndTurnFaint(gBattlerAttacker);
    }
    sPolishedActionBatch = FALSE;
    sPolishedActionResolving = TRUE;
    if (ResolvePolishedFaintQueue(&sPolishedActionFaints))
        return TRUE;
    gBattlerAttacker = sPolishedActionAttacker;
    gBattlerTarget = sPolishedActionTarget;
    sPolishedActionResolving = FALSE;
    return FALSE;
}

static bool32 HandlePolishedScreens(enum BattlerId battler)
{
    // Reflect for both sides, then Light Screen for both, etc. Native order is
    // side-major. Each effect has a new CheckSpeed, even when no timer is set.
    u32 block = gBattleStruct->eventState.endTurnBlock;
    if (gBattleStruct->eventState.endTurnBattler == 0)
        SortPolishedBattlersBySpeed(gBattlerByTurnOrder);
    battler = gBattlerByTurnOrder[gBattleStruct->eventState.endTurnBattler];
    gBattleStruct->eventState.battlerSide = GetBattlerSide(battler);
    bool32 effect = HandleEndTurnSecondEventBlock(battler);
    gBattleStruct->eventState.battlerSide = 0;
    gBattleStruct->eventState.endTurnBlock = block;
    if (++gBattleStruct->eventState.endTurnBattler == gBattlersCount)
    {
        gBattleStruct->eventState.endTurnBattler = 0;
        if (++gBattleStruct->eventState.endTurnBlock > SECOND_EVENT_BLOCK_AURORA_VEIL)
            gBattleStruct->eventState.endTurn++;
    }
    return effect;
}

static bool32 IsPolishedLeppaActivation(enum HoldEffect effect)
{
    return effect == HOLD_EFFECT_RESTORE_PP;
}

static bool32 IsPolishedHpHealingItem(enum HoldEffect effect)
{
    return effect == HOLD_EFFECT_RESTORE_HP || effect == HOLD_EFFECT_RESTORE_PCT_HP
        || effect == HOLD_EFFECT_CONFUSE_FLAVOR;
}

static bool32 IsPolishedStatusHealingItem(enum HoldEffect effect)
{
    return IsOnStatusChangeActivation(effect) && effect != HOLD_EFFECT_CURE_CONFUSION;
}

static bool32 IsPolishedStatBerry(enum HoldEffect effect)
{
    return IsOnHpThresholdActivation(effect) && !IsPolishedHpHealingItem(effect);
}

static bool32 IsPolishedConfusionHealingItem(enum HoldEffect effect)
{
    return effect == HOLD_EFFECT_CURE_CONFUSION || effect == HOLD_EFFECT_CURE_STATUS;
}

static const ActivationTiming sPolishedHealingTimings[] =
{
    IsPolishedHpHealingItem, IsPolishedStatusHealingItem, IsPolishedStatBerry,
    IsMirrorHerbActivation, IsPolishedConfusionHealingItem,
};

bool32 PolishedHandleUnnerveExit(enum BattlerId battler)
{
    // SuppressUserAbilities runs both sweeps immediately when Unnerve leaves,
    // including a faint checkpoint between two residual phases. Keep nested
    // item iteration separate from the enclosing end-turn/entry scheduler.
    if (!sPolishedExitItems.active)
    {
        if (!UsesPolishedLifecycle() || gBattleMons[battler].ability != ABILITY_UNNERVE
         || (!gBattleStruct->battlerState[battler].notOnField && gBattleMons[battler].hp != 0))
            return FALSE;
        sPolishedExitItems.active = TRUE;
        sPolishedExitItems.attacker = gBattlerAttacker;
        sPolishedExitItems.target = gBattlerTarget;
        sPolishedExitItems.scripting = gBattleScripting.battler;
        // HP zero alone does not disable Unnerve before faint cleanup. Source
        // selfdestruct explicitly clears the stored ability before both sweeps.
        if (!gBattleStruct->battlerState[battler].notOnField)
            gBattleMons[battler].ability = ABILITY_NONE;
        SortPolishedBattlersBySpeed(sPolishedExitItems.order);
    }
    while (sPolishedExitItems.phase < 2)
    {
        if (sPolishedExitItems.index == ARRAY_COUNT(sPolishedExitItems.order))
        {
            sPolishedExitItems.phase++;
            sPolishedExitItems.index = 0;
            if (sPolishedExitItems.phase < 2)
                SortPolishedBattlersBySpeed(sPolishedExitItems.order);
            continue;
        }
        enum BattlerId holder = sPolishedExitItems.order[sPolishedExitItems.index];
        gBattleStruct->polishedSelfHitBattlers &= ~(1u << holder);
        ActivationTiming timing = IsPolishedLeppaActivation;
        if (sPolishedExitItems.phase == 0)
            sPolishedExitItems.index++;
        else
        {
            timing = sPolishedHealingTimings[sPolishedExitItems.block];
            if (timing == IsMirrorHerbActivation && PolishedCheckMirrorHerb())
                return TRUE;
            sPolishedExitItems.block++;
            if (sPolishedExitItems.block == ARRAY_COUNT(sPolishedHealingTimings))
            {
                sPolishedExitItems.block = 0;
                sPolishedExitItems.index++;
            }
        }
        if (timing != IsMirrorHerbActivation && ItemBattleEffects(holder, 0, GetBattlerHoldEffect(holder), timing) != ITEM_NO_EFFECT)
            return TRUE;
    }
    gBattlerAttacker = sPolishedExitItems.attacker;
    gBattlerTarget = sPolishedExitItems.target;
    gBattleScripting.battler = sPolishedExitItems.scripting;
    memset(&sPolishedExitItems, 0, sizeof(sPolishedExitItems));
    return FALSE;
}

static bool32 HandlePolishedHealingItems(enum BattlerId battler)
{
    gBattleStruct->polishedSelfHitBattlers &= ~(1u << battler);
    u32 block = gBattleStruct->eventState.endTurnBlock;
    bool32 mirror = sPolishedHealingTimings[block] == IsMirrorHerbActivation;
    if (mirror && PolishedCheckMirrorHerb())
        return TRUE;
    gBattleStruct->eventState.endTurnBlock++;
    if (gBattleStruct->eventState.endTurnBlock == ARRAY_COUNT(sPolishedHealingTimings))
    {
        gBattleStruct->eventState.endTurnBlock = 0;
        gBattleStruct->eventState.endTurnBattler++;
    }
    return !mirror && ItemBattleEffects(battler, 0, GetBattlerHoldEffect(battler), sPolishedHealingTimings[block]) != ITEM_NO_EFFECT;
}

static bool32 DoPolishedEndTurnEffects(void)
{
    for (;;)
    {
        if (gBattleOutcome || sPolishedEndTurn.phase == ARRAY_COUNT(sPolishedEndTurnOrder))
        {
            ResetPolishedEndTurn();
            gBattleStruct->eventState.endTurn = ENDTURN_COUNT;
            return FALSE;
        }
        u32 phase = sPolishedEndTurnOrder[sPolishedEndTurn.phase];
        if (!sPolishedEndTurn.initialized)
        {
            sPolishedEndTurn.initialized = TRUE;
            gBattleStruct->eventState.endTurn = phase;
            gBattleStruct->eventState.endTurnBattler = 0;
            gBattleStruct->eventState.battlerSide = 0;
            gBattleStruct->eventState.endTurnBlock = 0;
            sPolishedEndTurn.batching = phase != ENDTURN_POLISHED_CHECKPOINT
                && phase != ENDTURN_FAINTED_MON_ACTIONS && phase != ENDTURN_EJECT_PACK
                && phase != ENDTURN_SEND_OUT_REPLACEMENTS_1 && phase != ENDTURN_SEND_OUT_REPLACEMENTS_2
                && phase != ENDTURN_SEND_OUT_REPLACEMENTS_3 && phase != ENDTURN_SEND_OUT_REPLACEMENTS_4
                && phase != ENDTURN_SEND_OUT_REPLACEMENTS_5;
            sPolishedEndTurn.deferringItems = phase != ENDTURN_POLISHED_LEPPA
                && phase != ENDTURN_POLISHED_HEALING_ITEMS && phase != ENDTURN_FAINTED_MON_ACTIONS;
            if (phase != ENDTURN_ORDER && phase != ENDTURN_POLISHED_CHECKPOINT
             && phase != ENDTURN_FUTURE_SIGHT && phase != ENDTURN_SECOND_EVENT_BLOCK)
                SortPolishedBattlersBySpeed(gBattlerByTurnOrder);
        }
        // Handlers advance either the phase or the battler. Do not re-enter a
        // completed phase after its script returns, or re-roll its first tie.
        if (gBattleStruct->eventState.endTurn != phase
         || gBattleStruct->eventState.endTurnBattler == gBattlersCount)
        {
            sPolishedEndTurn.phase++;
            sPolishedEndTurn.initialized = FALSE;
            continue;
        }
        enum BattlerId battler = gBattlerAttacker = gBattlerByTurnOrder[gBattleStruct->eventState.endTurnBattler];
        // Optional Expansion abilities still have their ordinary queued switch
        // hooks; the source roster does not acquire these implicitly.
        if (phase > ENDTURN_VARIOUS)
        {
            for (u32 i = 0; i < gBattlersCount; i++)
            {
                enum BattlerId other = gBattlerByTurnOrder[i];
                if (!IsBattlerPresent(other))
                    continue;
                bool32 switched = HandleEndTurnEmergencyExit(other);
                gBattleStruct->battlerState[other].wasAboveHalfHp = gBattleMons[other].hp > gBattleMons[other].maxHP / 2;
                if (switched)
                    return TRUE;
            }
        }
        bool32 effect;
        switch (phase)
        {
        case ENDTURN_POLISHED_CHECKPOINT:
            effect = HandlePolishedFaintCheckpoint();
            break;
        case ENDTURN_SECOND_EVENT_BLOCK:
            effect = HandlePolishedScreens(battler);
            break;
        case ENDTURN_POLISHED_LEPPA:
            gBattleStruct->eventState.endTurnBattler++;
            effect = ItemBattleEffects(battler, 0, GetBattlerHoldEffect(battler), IsPolishedLeppaActivation) != ITEM_NO_EFFECT;
            break;
        case ENDTURN_POLISHED_HEALING_ITEMS:
            effect = HandlePolishedHealingItems(battler);
            break;
        default:
            effect = sEndTurnEffectHandlers[phase](battler);
            break;
        }
        if (effect)
            return TRUE;
    }
}

bool32 DoEndTurnEffects(void)
{
    enum BattlerId battler = MAX_BATTLERS_COUNT;

    if (gBattleStruct->eventState.endTurn == ENDTURN_ORDER && !sPolishedEndTurn.active)
        sPolishedEndTurn.active = UsesPolishedLifecycle();
    if (sPolishedEndTurn.active)
        return DoPolishedEndTurnEffects();

    if (gBattleOutcome != 0 && gBattleStruct->eventState.endTurn < ENDTURN_TRAINER_A_SLIDES) // Skip most stuff if battle's outcome is determined
        gBattleStruct->eventState.endTurn = ENDTURN_TRAINER_A_SLIDES;

    for (;;)
    {
        // Activate any battler's Emergency Exit if possible (otherwise reset wasAboveHalfHp bit)
        if (gBattleStruct->eventState.endTurn > ENDTURN_VARIOUS)
        {
            for (u32 i = 0; i < gBattlersCount; i++)
            {
                battler = gBattlerByTurnOrder[i];

                if (!IsBattlerPresent(battler))
                    continue;

                bool32 effect = HandleEndTurnEmergencyExit(battler);

                gBattleStruct->battlerState[battler].wasAboveHalfHp = gBattleMons[battler].hp > gBattleMons[battler].maxHP / 2;

                if (effect)
                    return TRUE;
            }
        }
        // If either turnEffectsBattlerId or turnSideTracker are at max count, reset values and go to the next state
        if (gBattleStruct->eventState.endTurnBattler == gBattlersCount || gBattleStruct->eventState.battlerSide == NUM_BATTLE_SIDES)
        {
            gBattleStruct->eventState.endTurnBattler = 0;
            gBattleStruct->eventState.battlerSide = 0;
            gBattleStruct->eventState.endTurnBlock = 0;
            gBattleStruct->eventState.endTurn++;
        }

        // Jump out if possible after endTurnEventsCounter was increased in the above code block
        if (gBattleStruct->eventState.endTurn == ENDTURN_COUNT)
            return FALSE;

        battler = gBattlerAttacker = gBattlerByTurnOrder[gBattleStruct->eventState.endTurnBattler];

        if (sEndTurnEffectHandlers[gBattleStruct->eventState.endTurn](battler))
            return TRUE;
    }
}
