#include "global.h"
#include "polished_lifecycle.h"
#include "battle.h"
#include "battle_hold_effects.h"
#include "battle_stat_change.h"
#include "battle_util.h"
#include "battle_ai_record.h"
#include "battle_gimmick.h"
#include "battle_scripts.h"
#include "battle_switch_in.h"
#include "battle_controllers.h"
#include "config_changes.h"
#include "config/battle.h"
#include "constants/battle.h"
#include "constants/moves.h"

static bool32 FirstEventBlockEvents(struct BattleCalcValues *calcValues);
static bool32 TryHazardsOnSwitchIn(enum BattlerId battler, enum Ability ability, enum HoldEffect holdEffect, enum Hazards hazardType);
static bool32 SecondEventBlockEvents(struct BattleCalcValues *calcValues);

// CheckStatHerbsAfterIntimidate has its own reservation and recursively runs
// DeferredSwitch, preserving any action's already reserved switch. At most one
// held Pack per party member can be consumed in an entry chain.
struct PolishedIntimidateFrame
{
    struct BattleScripting scripting;
    enum BattlerId user, attacker, target, effect, ability;
    enum BattlerId order[MAX_BATTLERS_COUNT];
    enum BattlerId speedOrder[MAX_BATTLERS_COUNT], rawOrder[MAX_BATTLERS_COUNT];
    enum Ability lastAbility;
    enum Item lastItem;
    enum Move move;
    u8 phase, index, pack;
    u8 switchIn, battlerSwitchIn, counter, hazards;
    u8 entrants;
    bool8 sorted, intimidate, adrenaline, blocked, switched;
};
static EWRAM_DATA struct PolishedIntimidateFrame sPolishedIntimidate[2 * PARTY_SIZE + 1] = {0};
static EWRAM_DATA u8 sPolishedIntimidateDepth = 0;
static EWRAM_DATA u8 sPolishedGasExitDepth = 0;

void ResetPolishedEntryItems(void)
{
    sPolishedIntimidateDepth = sPolishedGasExitDepth = 0;
}

void PolishedGasExit(bool32 active)
{
    if (!UsesPolishedLifecycle())
        return;
    if (active)
        sPolishedGasExitDepth++;
    else if (sPolishedGasExitDepth)
        sPolishedGasExitDepth--;
}

void PolishedBeginIntimidate(enum BattlerId battler)
{
    if (!UsesPolishedLifecycle())
        return;
    struct PolishedIntimidateFrame *frame = &sPolishedIntimidate[sPolishedIntimidateDepth++];
    memset(frame, 0, sizeof(*frame));
    frame->user = battler;
    frame->pack = MAX_BATTLERS_COUNT;
    frame->attacker = gBattlerAttacker;
    frame->target = gBattlerTarget;
    frame->effect = gEffectBattler;
    frame->ability = gBattlerAbility;
    frame->lastAbility = gLastUsedAbility;
    frame->lastItem = gLastUsedItem;
    frame->move = gCurrentMove;
    frame->scripting = gBattleScripting;
    enum Ability ability = GetBattlerAbility(GetOppositeBattler(battler));
    // These four source immunities return before the herb checks. Clear Body,
    // Mist and a stage already at -6 still reach them.
    frame->blocked = ability == ABILITY_INNER_FOCUS || ability == ABILITY_OWN_TEMPO
        || ability == ABILITY_OBLIVIOUS || ability == ABILITY_SCRAPPY;
}

static void SavePolishedEntryParent(struct PolishedIntimidateFrame *frame)
{
    frame->switchIn = gBattleStruct->eventState.switchIn;
    frame->battlerSwitchIn = gBattleStruct->eventState.battlerSwitchIn;
    frame->counter = gBattleStruct->switchInBattlerCounter;
    frame->hazards = gBattleStruct->hazardsCounter;
    frame->sorted = gBattleStruct->battlersSorted;
    frame->intimidate = gBattleStruct->intimidateActivated;
    frame->adrenaline = gBattleStruct->adrenalineOrbActivated;
    memcpy(frame->speedOrder, gBattlersBySpeed, sizeof(frame->speedOrder));
    memcpy(frame->rawOrder, gBattlersByRawSpeed, sizeof(frame->rawOrder));
    for (u32 i = 0; i < gBattlersCount; i++)
    {
        if (gBattleStruct->battlerState[i].switchIn)
            frame->entrants |= 1u << i;
        gBattleStruct->battlerState[i].switchIn = FALSE;
    }
    PolishedPushEntryFaints();
    frame->switched = TRUE;
}

static void RestorePolishedEntryParent(struct PolishedIntimidateFrame *frame)
{
    gBattleStruct->eventState.switchIn = frame->switchIn;
    gBattleStruct->eventState.battlerSwitchIn = frame->battlerSwitchIn;
    gBattleStruct->switchInBattlerCounter = frame->counter;
    gBattleStruct->hazardsCounter = frame->hazards;
    gBattleStruct->battlersSorted = frame->sorted;
    gBattleStruct->intimidateActivated = frame->intimidate;
    gBattleStruct->adrenalineOrbActivated = frame->adrenaline;
    memcpy(gBattlersBySpeed, frame->speedOrder, sizeof(frame->speedOrder));
    memcpy(gBattlersByRawSpeed, frame->rawOrder, sizeof(frame->rawOrder));
    // RunBothEntryAbilities still visits the second side even if its entrant
    // changed during the first ability. Preserve that source repeated trigger.
    for (u32 i = 0; i < gBattlersCount; i++)
        gBattleStruct->battlerState[i].switchIn = !!(frame->entrants & (1u << i));
    PolishedPopEntryFaints();
}

bool32 PolishedIntimidateItems(void)
{
    if (!UsesPolishedLifecycle() || !sPolishedIntimidateDepth)
        return FALSE;
    struct PolishedIntimidateFrame *frame = &sPolishedIntimidate[sPolishedIntimidateDepth - 1];
    // Intimidate explicitly calls Mirror Herb, then CheckStatHerbs (White
    // Herb/Pack followed by a second Mirror Herb pass). Recompare Speed each
    // time: Rattled, a copied Speed boost or White Herb can change the order.
    while (!frame->blocked && frame->phase < 3)
    {
        bool32 mirror = frame->phase != 1;
        if (frame->index == 0)
            SortPolishedBattlersBySpeed(frame->order);
        if (mirror && (!IsBattlerAlive(frame->user) || !IsBattlerAlive(GetOppositeBattler(frame->user))))
            frame->index = gBattlersCount;
        while (frame->index < gBattlersCount)
        {
            enum BattlerId battler = frame->order[frame->index++];
            enum HoldEffect effect = GetBattlerHoldEffect(battler);
            if (!IsBattlerAlive(battler))
                continue;
            if (mirror)
            {
                if (ItemBattleEffects(battler, 0, effect, IsMirrorHerbActivation))
                    return TRUE;
            }
            else if (effect == HOLD_EFFECT_WHITE_HERB)
            {
                if (ItemBattleEffects(battler, 0, effect, IsWhiteHerbActivation))
                    return TRUE;
            }
            else if (effect == HOLD_EFFECT_EJECT_PACK && frame->pack == MAX_BATTLERS_COUNT)
            {
                for (u32 stat = STAT_ATK; stat < NUM_BATTLE_STATS; stat++)
                    if (gBattleMons[battler].statStages[stat] < DEFAULT_STAT_STAGE)
                    {
                        frame->pack = battler;
                        break;
                    }
            }
        }
        frame->index = 0;
        frame->phase++;
    }
    if (frame->phase == 3)
    {
        frame->phase++;
        for (u32 i = 0; i < gBattlersCount; i++)
            gBattleMons[i].volatiles.tryEjectPack = FALSE;
        // Source checks availability only AFTER selecting the first Pack, and
        // suppresses nested switches while a departing Gas ability is -1.
        if (!sPolishedGasExitDepth && !gBattleOutcome && frame->pack != MAX_BATTLERS_COUNT
         && IsBattlerAlive(frame->pack) && CanBattlerSwitch(frame->pack))
        {
            SavePolishedEntryParent(frame);
            gBattleScripting.battler = frame->pack;
            gLastUsedItem = gBattleMons[frame->pack].item;
            gBattleStruct->battlerState[frame->pack].usedEjectItem = TRUE;
            gAiLogicData->ejectPackSwitch = TRUE;
            BattleScriptCall(BattleScript_EjectPackActivates_SendReplacement);
            return TRUE;
        }
    }
    if (frame->switched)
        RestorePolishedEntryParent(frame);
    gBattlerAttacker = frame->attacker;
    gBattlerTarget = frame->target;
    gEffectBattler = frame->effect;
    gBattlerAbility = frame->ability;
    gLastUsedAbility = frame->lastAbility;
    gLastUsedItem = frame->lastItem;
    gCurrentMove = frame->move;
    gBattleScripting = frame->scripting;
    sPolishedIntimidateDepth--;
    return FALSE;
}

static enum Hazards GetPolishedEntryHazard(enum BattleSide side)
{
    // SpikesDamage always handles Spikes before Toxic Spikes, irrespective
    // of the order they were laid. Keep optional hazards in their native order.
    while (gBattleStruct->hazardsCounter < HAZARDS_MAX_COUNT)
    {
        u32 index = gBattleStruct->hazardsCounter;
        if (index == 0 && gSideTimers[side].spikesAmount)
            return HAZARDS_SPIKES;
        if (index == 1 && gSideTimers[side].toxicSpikesAmount)
            return HAZARDS_TOXIC_SPIKES;
        if (index >= 2)
        {
            u32 optional = 2;
            for (u32 i = 0; i < HAZARDS_MAX_COUNT; i++)
            {
                enum Hazards hazard = gBattleStruct->hazardsQueue[side][i];
                if (hazard == HAZARDS_NONE)
                    break;
                if (hazard != HAZARDS_SPIKES && hazard != HAZARDS_TOXIC_SPIKES && optional++ == index)
                    return hazard;
            }
            return HAZARDS_NONE;
        }
        gBattleStruct->hazardsCounter++;
    }
    return HAZARDS_NONE;
}

bool32 DoSwitchInEvents(void)
{
    enum BattlerId battler;

    struct BattleCalcValues calcValues = {0};
    for (battler = 0; battler < gBattlersCount; battler++)
    {
        if (!IsBattlerAlive(battler))
            continue;
        calcValues.abilities[battler] = GetBattlerAbility(battler);
        calcValues.holdEffects[battler] = GetBattlerHoldEffect(battler);
    }

    switch (gBattleStruct->eventState.switchIn)
    {
    case SWITCH_IN_EVENTS_ORDER_BY_SPEED:
        for (enum BattlerId i = 0; i < gBattlersCount; i++)
            gBattlersBySpeed[i] = i;
        if (UsesPolishedLifecycle())
        {
            PolishedBeginEntryHazards();
            SortPolishedBattlersBySpeed(gBattlersBySpeed);
        }
        else
            SortBattlersBySpeed(gBattlersBySpeed, FALSE);
        SortBattlersByRawSpeed(gBattlersByRawSpeed);
        gBattleStruct->battlersSorted = TRUE;
        gBattleStruct->switchInBattlerCounter = 0;
        gBattleStruct->eventState.battlerSwitchIn = 0;
        gBattleStruct->eventState.switchIn++;
        // fallthrough
    case SWITCH_IN_EVENTS_TERA_SHIFT:
        while (gBattleStruct->switchInBattlerCounter < gBattlersCount)
        {
            battler = gBattlersByRawSpeed[gBattleStruct->switchInBattlerCounter++];
            if (AbilityBattleEffects(ABILITYEFFECT_TERA_SHIFT, battler, calcValues.abilities[battler], 0, gBattleStruct->battlerState[battler].switchIn))
                return TRUE;
        }
        gBattleStruct->switchInBattlerCounter = 0;
        gBattleStruct->eventState.switchIn = UsesPolishedLifecycle() ? SWITCH_IN_EVENTS_POLISHED_HAZARDS : SWITCH_IN_EVENTS_NEUTRALIZING_GAS;
        break;
    case SWITCH_IN_EVENTS_POLISHED_HAZARDS:
        // Source SpikesDamageBoth precedes RunBothEntryAbilities. Reuse the
        // optional Healing Wish/hazard handlers, but finish them for BOTH
        // battlers before any incoming ability can remove hazards or weather.
        while (gBattleStruct->switchInBattlerCounter < gBattlersCount)
        {
            battler = gBattlersBySpeed[gBattleStruct->switchInBattlerCounter];
            calcValues.battlerAtk = battler;
            while (gBattleStruct->eventState.battlerSwitchIn < FIRST_EVENT_BLOCK_GENERAL_ABILITIES)
                if (FirstEventBlockEvents(&calcValues))
                    return TRUE;
            // SpikesDamage ends with HandleEntryItems for this battler,
            // before the other side's hazards and either entry ability.
            if (gBattleStruct->eventState.battlerSwitchIn == FIRST_EVENT_BLOCK_GENERAL_ABILITIES)
            {
                gBattleStruct->eventState.battlerSwitchIn++;
                enum HoldEffect holdEffect = GetBattlerHoldEffect(battler);
                if (gBattleStruct->battlerState[battler].switchIn
                 && (holdEffect == HOLD_EFFECT_AIR_BALLOON || holdEffect == HOLD_EFFECT_ROOM_SERVICE)
                 && ItemBattleEffects(battler, 0, holdEffect, IsOnSwitchInActivation))
                    return TRUE;
            }
            gBattleStruct->switchInBattlerCounter++;
            gBattleStruct->eventState.battlerSwitchIn = 0;
        }
        SortPolishedBattlersBySpeed(gBattlersBySpeed);
        gBattleStruct->switchInBattlerCounter = 0;
        gBattleStruct->eventState.switchIn = SWITCH_IN_EVENTS_FIRST_BLOCK;
        break;
    case SWITCH_IN_EVENTS_NEUTRALIZING_GAS:
        while (gBattleStruct->switchInBattlerCounter < gBattlersCount)
        {
            battler = gBattlersBySpeed[gBattleStruct->switchInBattlerCounter++];
            if (AbilityBattleEffects(ABILITYEFFECT_NEUTRALIZINGGAS, battler, calcValues.abilities[battler], 0, gBattleStruct->battlerState[battler].switchIn))
                return TRUE;
        }
        gBattleStruct->switchInBattlerCounter = 0;
        gBattleStruct->eventState.switchIn++;
        break;
    case SWITCH_IN_EVENTS_UNNERVE:
        while (gBattleStruct->switchInBattlerCounter < gBattlersCount)
        {
            battler = gBattlersBySpeed[gBattleStruct->switchInBattlerCounter++];
            if (AbilityBattleEffects(ABILITYEFFECT_UNNERVE, battler, calcValues.abilities[battler], 0, gBattleStruct->battlerState[battler].switchIn))
                return TRUE;
        }
        gBattleStruct->switchInBattlerCounter = 0;
        gBattleStruct->eventState.switchIn++;
        break;
    case SWITCH_IN_EVENTS_FIRST_BLOCK:
        while (gBattleStruct->switchInBattlerCounter < gBattlersCount)
        {
            battler = gBattlersBySpeed[gBattleStruct->switchInBattlerCounter];

            if (UsesPolishedLifecycle() && gBattleStruct->eventState.battlerSwitchIn < FIRST_EVENT_BLOCK_GENERAL_ABILITIES)
                gBattleStruct->eventState.battlerSwitchIn = FIRST_EVENT_BLOCK_POLISHED_GAS;

            if (!IsBattlerAlive(battler) && gBattleStruct->eventState.battlerSwitchIn != FIRST_EVENT_BLOCK_HAZARDS)
            {
                gBattleStruct->switchInBattlerCounter++;
                gBattleStruct->eventState.battlerSwitchIn = 0;
                continue;
            }

            calcValues.battlerAtk = battler;
            while (gBattleStruct->eventState.battlerSwitchIn != FIRST_EVENT_BLOCK_COUNT)
            {
                if (FirstEventBlockEvents(&calcValues))
                    return TRUE;
            }

            gBattleStruct->switchInBattlerCounter++;
            gBattleStruct->eventState.battlerSwitchIn = 0;
        }
        gBattleStruct->switchInBattlerCounter = 0;
        gBattleStruct->eventState.switchIn++;
        break;
    case SWITCH_IN_EVENTS_FORM_CHANGE:
        while (gBattleStruct->switchInBattlerCounter < gBattlersCount)
        {
            battler = gBattlersBySpeed[gBattleStruct->switchInBattlerCounter++];
            if (AbilityBattleEffects(ABILITYEFFECT_SWITCH_IN_FORM_CHANGE, battler, calcValues.abilities[battler], 0, gBattleStruct->battlerState[battler].switchIn))
                return TRUE;
        }
        gBattleStruct->switchInBattlerCounter = 0;
        gBattleStruct->eventState.switchIn++;
        break;
    case SWITCH_IN_EVENTS_SECOND_BLOCK:
        while (gBattleStruct->switchInBattlerCounter < gBattlersCount)
        {
            battler = gBattlersByRawSpeed[gBattleStruct->switchInBattlerCounter];

            if (!IsBattlerAlive(battler))
            {
                gBattleStruct->switchInBattlerCounter++;
                gBattleStruct->eventState.battlerSwitchIn = 0;
                continue;
            }

            calcValues.battlerAtk = battler;
            while (gBattleStruct->eventState.battlerSwitchIn < SECOND_EVENT_BLOCK_COUNT)
            {
                if (SecondEventBlockEvents(&calcValues))
                    return TRUE;
            }

            gBattleStruct->switchInBattlerCounter++;
            gBattleStruct->eventState.battlerSwitchIn = 0;
        }
        gBattleStruct->switchInBattlerCounter = 0;
        gBattleStruct->eventState.switchIn++;
        break;
    case SWITCH_IN_EVENTS_WHITE_HERB:
        while (gBattleStruct->switchInBattlerCounter < gBattlersCount)
        {
            enum BattlerId battler = gBattlersByRawSpeed[gBattleStruct->switchInBattlerCounter++];
            if (ItemBattleEffects(battler, 0, calcValues.holdEffects[battler], IsWhiteHerbActivation))
                return TRUE;
        }
        gBattleStruct->switchInBattlerCounter = 0;
        gBattleStruct->eventState.switchIn++;
        break;
    case SWITCH_IN_EVENTS_OPPORTUNIST:
        while (gBattleStruct->switchInBattlerCounter < gBattlersCount)
        {
            enum BattlerId battler = gBattlersByRawSpeed[gBattleStruct->switchInBattlerCounter++];
            if (AbilityBattleEffects(ABILITYEFFECT_OPPORTUNIST, battler, calcValues.abilities[battler], MOVE_NONE, TRUE))
                return TRUE;
        }
        gBattleStruct->switchInBattlerCounter = 0;
        gBattleStruct->eventState.switchIn++;
        break;
    case SWITCH_IN_EVENTS_MIRROR_HERB:
        while (gBattleStruct->switchInBattlerCounter < gBattlersCount)
        {
            enum BattlerId battler = gBattlersByRawSpeed[gBattleStruct->switchInBattlerCounter++];
            if (ItemBattleEffects(battler, 0, calcValues.holdEffects[battler], IsMirrorHerbActivation))
                return TRUE;
        }
        gBattleStruct->switchInBattlerCounter = 0;
        gBattleStruct->eventState.switchIn++;
        break;
    case SWITCH_IN_EVENTS_CLEAR_SET_VALUES:
        if (UsesPolishedLifecycle() && PolishedResolveEntryFaints())
            return TRUE;
        for (battler = 0; battler < gBattlersCount; battler++)
        {
            if (gBattleStruct->battlerState[battler].switchIn)
            {
                gBattleStruct->battlerState[battler].hpOnSwitchout = gBattleMons[battler].hp;
                gBattleStruct->battlerState[battler].switchIn = FALSE;
            }
        }

        gBattleStruct->intimidateActivated = FALSE;
        gBattleStruct->adrenalineOrbActivated = FALSE;
        gBattleStruct->battlersSorted = FALSE;
        gBattleStruct->hazardsCounter = 0;
        gBattleStruct->eventState.switchIn++;
        break;
    case SWITCH_IN_EVENTS_EJECT_PACK:
        gBattleStruct->eventState.switchIn++;
        if (TrySwitchInEjectPack(START_OF_TURN))
            return TRUE;
        break;
    case SWITCH_IN_EVENTS_COUNT:
        break;
    }
    return FALSE;
}

static bool32 CanBattlerBeHealed(enum BattlerId battler)
{
    if (GetConfig(B_HEALING_WISH_SWITCH) < GEN_8)
        return TRUE;

    if (gBattleMons[battler].hp != gBattleMons[battler].maxHP || gBattleMons[battler].status1)
        return TRUE;

    if (gBattleStruct->battlerState[battler].storedLunarDance
     && (gBattleMons[battler].pp[0] < CalculatePPWithBonus(gBattleMons[battler].moves[0], gBattleMons[battler].ppBonuses, 0)
      || gBattleMons[battler].pp[1] < CalculatePPWithBonus(gBattleMons[battler].moves[1], gBattleMons[battler].ppBonuses, 1)
      || gBattleMons[battler].pp[2] < CalculatePPWithBonus(gBattleMons[battler].moves[2], gBattleMons[battler].ppBonuses, 2)
      || gBattleMons[battler].pp[3] < CalculatePPWithBonus(gBattleMons[battler].moves[3], gBattleMons[battler].ppBonuses, 3)))
    {
        return TRUE;
    }

    return FALSE;
}

static bool32 FirstEventBlockEvents(struct BattleCalcValues *calcValues)
{
    bool32 effect = FALSE;
    enum BattlerId battler = calcValues->battlerAtk;

    switch (gBattleStruct->eventState.battlerSwitchIn)
    {
    case FIRST_EVENT_BLOCK_POLISHED_GAS:
        gBattleStruct->eventState.battlerSwitchIn = FIRST_EVENT_BLOCK_POLISHED_UNNERVE;
        if (gBattleStruct->battlerState[battler].switchIn
         && calcValues->abilities[battler] == ABILITY_NEUTRALIZING_GAS)
        {
            enum BattlerId first = gBattlersBySpeed[0];
            // RunBothEntryAbilities shows dual Gas only once, fastest first.
            if (battler == first || !IsBattlerAlive(first)
             || gBattleMons[first].ability != ABILITY_NEUTRALIZING_GAS)
            {
                gBattleMons[battler].volatiles.neutralizingGas = FALSE;
                effect = AbilityBattleEffects(ABILITYEFFECT_NEUTRALIZINGGAS, battler,
                    calcValues->abilities[battler], MOVE_NONE, TRUE);
            }
        }
        break;
    case FIRST_EVENT_BLOCK_POLISHED_UNNERVE:
        gBattleStruct->eventState.battlerSwitchIn = FIRST_EVENT_BLOCK_GENERAL_ABILITIES;
        effect = AbilityBattleEffects(ABILITYEFFECT_UNNERVE, battler,
            calcValues->abilities[battler], MOVE_NONE, gBattleStruct->battlerState[battler].switchIn);
        break;
    case FIRST_EVENT_BLOCK_HEALING_WISH:
        if (!gBattleStruct->battlerState[battler].switchIn || !CanBattlerBeHealed(battler))
        {
            effect = FALSE;
        }
        else if (gBattleStruct->battlerState[battler].storedHealingWish)
        {
            gBattleStruct->battlerState[battler].storedHealingWish = FALSE;
            SetHealAmount(battler, GetNonDynamaxMaxHP(battler));
            gBattleScripting.battler = battler;
            BattleScriptCall(BattleScript_HealingWishActivates);
            effect = TRUE;
        }
        else if (gBattleStruct->battlerState[battler].storedLunarDance)
        {
            gBattleStruct->battlerState[battler].storedLunarDance = FALSE;
            SetHealAmount(battler, GetNonDynamaxMaxHP(battler));
            gBattleScripting.battler = battler;
            BattleScriptCall(BattleScript_LunarDanceActivates);
            effect = TRUE;
        }
        else if (gBattleStruct->zmove.healReplacement & 1u << battler)
        {
            gBattleStruct->zmove.healReplacement &= ~(1u << battler);
            SetHealAmount(battler, GetNonDynamaxMaxHP(battler));
            gBattleScripting.battler = battler;
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_Z_HP_TRAP;
            BattleScriptCall(BattleScript_HealReplacementZMove);
            effect = TRUE;
        }
        gBattleStruct->eventState.battlerSwitchIn++;
        break;
    case FIRST_EVENT_BLOCK_HAZARDS:
        if (!gBattleStruct->battlerState[battler].switchIn)
        {
            gBattleStruct->eventState.battlerSwitchIn++;
        }
        else if (EmergencyExitCanBeTriggered(battler, calcValues->abilities[battler]))
        {
            gBattleScripting.battler = gBattlerAbility = battler;
            gBattleStruct->battlerState[battler].forcedSwitch = FALSE;
            gBattleStruct->eventState.switchIn = 0;
            BattleScriptCall(BattleScript_EmergencyExitSendReplacement);
            effect = TRUE;
        }
        else
        {
            enum Hazards hazard = UsesPolishedLifecycle() ? GetPolishedEntryHazard(GetBattlerSide(battler))
                : gBattleStruct->hazardsQueue[GetBattlerSide(battler)][gBattleStruct->hazardsCounter];
            if (hazard == HAZARDS_NONE || gBattleStruct->hazardsCounter >= HAZARDS_MAX_COUNT)
            {
                gBattleStruct->hazardsCounter = 0;
                gBattleStruct->eventState.battlerSwitchIn++;
            }
            else
            {
                effect = TryHazardsOnSwitchIn(battler, calcValues->abilities[battler], calcValues->holdEffects[battler], hazard);
            }
        }
        break;
    case FIRST_EVENT_BLOCK_GENERAL_ABILITIES:
        if (TryPrimalReversion(battler)
         || AbilityBattleEffects(ABILITYEFFECT_ON_SWITCHIN, battler, calcValues->abilities[battler], MOVE_NONE, gBattleStruct->battlerState[battler].switchIn)
         || TryClearIllusion(battler, calcValues->abilities[battler]))
            effect = TRUE;
        gBattleStruct->eventState.battlerSwitchIn++;
        break;
    case FIRST_EVENT_BLOCK_IMMUNITY_ABILITIES:
        if (AbilityBattleEffects(ABILITYEFFECT_IMMUNITY, battler, calcValues->abilities[battler], MOVE_NONE, TRUE))
            effect = TRUE;
        gBattleStruct->eventState.battlerSwitchIn++;
        break;
    case FIRST_EVENT_BLOCK_ITEMS:
        if (!(UsesPolishedLifecycle() && (calcValues->holdEffects[battler] == HOLD_EFFECT_AIR_BALLOON
                                      || calcValues->holdEffects[battler] == HOLD_EFFECT_ROOM_SERVICE))
         && ItemBattleEffects(battler, 0, calcValues->holdEffects[battler], IsOnSwitchInActivation))
            effect = TRUE;
        gBattleStruct->eventState.battlerSwitchIn++;
        break;
    case FIRST_EVENT_BLOCK_COUNT:
        gBattleStruct->eventState.battlerSwitchIn++;
        break;
    }

    return effect;
}

static void SetDmgHazardsBattlescript(enum BattlerId battler, u8 multistringId)
{
    gBattleScripting.battler = battler;
    gBattleCommunication[MULTISTRING_CHOOSER] = multistringId;
    BattleScriptCall(BattleScript_DmgHazardsOnBattler);
}

static bool32 TryHazardsOnSwitchIn(enum BattlerId battler, enum Ability ability, enum HoldEffect holdEffect, enum Hazards hazardType)
{
    bool32 effect = FALSE;
    enum BattleSide side = GetBattlerSide(battler);
    bool32 clearedToxicSpikes = FALSE;

    switch (hazardType)
    {
    case HAZARDS_NONE:
        break;
    case HAZARDS_SPIKES:
        if (!IsAbilityAndRecord(battler, ability, ABILITY_MAGIC_GUARD)
         && IsBattlerAffectedByHazards(battler, holdEffect, FALSE)
         && IsBattlerGrounded(battler, ability, holdEffect))
        {
            s32 spikesDmg = GetNonDynamaxMaxHP(battler) / ((5 - gSideTimers[side].spikesAmount) * 2);
            SetPassiveDamageAmount(battler, spikesDmg);
            SetDmgHazardsBattlescript(battler, B_MSG_PKMNHURTBYSPIKES);
            effect = TRUE;
        }
        break;
    case HAZARDS_STICKY_WEB:
        if (IsBattlerAffectedByHazards(battler, holdEffect, FALSE) && IsBattlerGrounded(battler, ability, holdEffect))
        {
            gEffectBattler = battler;
            SetStatChange(battler, STAT_SPEED, -1);
            BattleScriptCall(BattleScript_StickyWebOnSwitchIn);
            effect = TRUE;
        }
        break;
    case HAZARDS_TOXIC_SPIKES:
        if (!IsBattlerGrounded(battler, ability, holdEffect))
        {
            effect = FALSE;
        }
        else if (B_POLISHED_MOVE_BALANCE
              && holdEffect == HOLD_EFFECT_HEAVY_DUTY_BOOTS
              && !IsDoubleBattle()
              && GetActiveGimmick(battler) == GIMMICK_NONE
              && GetActiveGimmick(GetOppositeBattler(battler)) == GIMMICK_NONE)
        {
            // Source Heavy Boots skip hazards before Poison-type absorption.
            RecordItemEffectBattle(battler, holdEffect);
        }
        else if (IS_BATTLER_OF_TYPE(battler, TYPE_POISON)) // Absorb the toxic spikes.
        {
            gSideTimers[side].toxicSpikesAmount = 0;
            RemoveHazardFromField(side, HAZARDS_TOXIC_SPIKES);
            gEffectBattler = battler;
            BattleScriptCall(BattleScript_ToxicSpikesAbsorbed);
            clearedToxicSpikes = TRUE;
            effect = TRUE;
        }
        else if (IsBattlerAffectedByHazards(battler, holdEffect, TRUE)
              && CanBePoisoned(battler, battler, ABILITY_NONE, ability))
        {
            gBattleScripting.battler = battler;
            BattleScriptPushCursor();
            if (gSideTimers[GetBattlerSide(battler)].toxicSpikesAmount >= 2)
            {
                gBattlescriptCurrInstr = BattleScript_ToxicSpikesBadlyPoisoned;
                gBattleMons[battler].status1 |= STATUS1_TOXIC_POISON;
            }
            else
            {
                gBattlescriptCurrInstr = BattleScript_ToxicSpikesPoisoned;
                gBattleMons[battler].status1 |= STATUS1_POISON;
            }

            BtlController_EmitSetMonData(battler, B_COMM_TO_CONTROLLER, REQUEST_STATUS_BATTLE, 0, sizeof(gBattleMons[battler].status1), &gBattleMons[battler].status1);
            MarkBattlerForControllerExec(battler);
            effect = TRUE;
        }
        break;
    case HAZARDS_STEALTH_ROCK:
        if (IsBattlerAffectedByHazards(battler, holdEffect, FALSE) && ability != ABILITY_MAGIC_GUARD)
        {
            gBattleStruct->passiveHpUpdate[battler] = GetStealthHazardDamage(TYPE_SIDE_HAZARD_POINTED_STONES, battler);
            if (gBattleStruct->passiveHpUpdate[battler] != 0)
            {
                SetDmgHazardsBattlescript(battler, B_MSG_STEALTHROCKDMG);
                effect = TRUE;
            }
        }
        break;
    case HAZARDS_STEELSURGE:
        if (IsBattlerAffectedByHazards(battler, holdEffect, FALSE) && ability != ABILITY_MAGIC_GUARD)
        {
            gBattleStruct->passiveHpUpdate[battler] = GetStealthHazardDamage(TYPE_SIDE_HAZARD_SHARP_STEEL, battler);
            if (gBattleStruct->passiveHpUpdate[battler] != 0)
            {
                SetDmgHazardsBattlescript(battler, B_MSG_SHARPSTEELDMG);
                effect = TRUE;
            }
        }
        break;
    case HAZARDS_MAX_COUNT:
        break;
    }

    if (!clearedToxicSpikes || UsesPolishedLifecycle())
        gBattleStruct->hazardsCounter++;

    return effect;
}

static bool32 SecondEventBlockEvents(struct BattleCalcValues *calcValues)
{
    bool32 effect = FALSE;
    enum BattlerId battler = calcValues->battlerAtk;

    switch (gBattleStruct->eventState.battlerSwitchIn)
    {
    case SECOND_EVENT_ABILITIES:
        if (AbilityBattleEffects(ABILITYEFFECT_ON_WEATHER, battler, calcValues->abilities[battler], MOVE_NONE, TRUE)
         || AbilityBattleEffects(ABILITYEFFECT_ON_TERRAIN, battler, calcValues->abilities[battler], MOVE_NONE, TRUE)
         || AbilityBattleEffects(ABILITYEFFECT_DEPENDS_ON_ALLY, battler, calcValues->abilities[battler], MOVE_NONE, gBattleStruct->battlerState[battler].switchIn))
            effect = TRUE;
        gBattleStruct->eventState.battlerSwitchIn++;
        break;
    case SECOND_EVENT_BOOSTER_ENERGY:
        if (ItemBattleEffects(battler, 0, calcValues->holdEffects[battler], IsBoosterEnergyActivation))
            effect = TRUE;
        gBattleStruct->eventState.battlerSwitchIn++;
        break;
    case SECOND_EVENT_BLOCK_COUNT:
        gBattleStruct->eventState.battlerSwitchIn++;
        break;
    }

    return effect;
}
