#include "global.h"
#include "polished_lifecycle.h"
#include "battle.h"
#include "battle_anim.h"
#include "battle_ai_record.h"
#include "battle_controllers.h"
#include "battle_util.h"
#include "battle_hold_effects.h"
#include "battle_stat_change.h"
#include "battle_scripts.h"
#include "item.h"
#include "polished_items.h"
#include "polished_moves.h"
#include "polished_damage.h"
#include "string_util.h"
#include "data/hold_effects.h"
#include "data/polished_entry_abilities.h"
#include "constants/berry.h"

enum PolishedWeaknessPolicyPhase GetPolishedWeaknessPolicyPhase(enum Move move)
{
    if (!UsesPolishedHitCheckpoint(move))
        return POLISHED_POLICY_NATIVE;
    if (move >= MOVES_COUNT
     || (move == MOVE_FUTURE_SIGHT && !IsPolishedFutureSightImpact(gBattlerAttacker, gBattlerTarget, move)))
        return POLISHED_POLICY_ABSENT;
    return sPolishedWeaknessPolicyPhases[move];
}

bool32 IsPolishedWeaknessPolicyActivation(enum HoldEffect holdEffect) { return holdEffect == HOLD_EFFECT_WEAKNESS_POLICY; }
bool32 IsOnSwitchInActivation(enum HoldEffect holdEffect)          { return gHoldEffectsInfo[holdEffect].onSwitchIn; }
bool32 IsMirrorHerbActivation(enum HoldEffect holdEffect)          { return gHoldEffectsInfo[holdEffect].mirrorHerb; }
bool32 IsWhiteHerbActivation(enum HoldEffect holdEffect)           { return gHoldEffectsInfo[holdEffect].whiteHerb; }
bool32 IsOnStatusChangeActivation(enum HoldEffect holdEffect)      { return gHoldEffectsInfo[holdEffect].onStatusChange; }
bool32 IsOnHpThresholdActivation(enum HoldEffect holdEffect)       { return gHoldEffectsInfo[holdEffect].onHpThreshold; }
bool32 IsPolishedDamageEnigmaActivation(enum HoldEffect holdEffect) { return holdEffect == HOLD_EFFECT_ENIGMA_BERRY; }
bool32 IsKeeMarangaBerryActivation(enum HoldEffect holdEffect)     { return gHoldEffectsInfo[holdEffect].keeMarangaBerry; }
bool32 IsOnTargetHitActivation(enum HoldEffect holdEffect)         { return gHoldEffectsInfo[holdEffect].onTargetAfterHit; }
bool32 IsOnAttackerAfterHitActivation(enum HoldEffect holdEffect)  { return gHoldEffectsInfo[holdEffect].onAttackerAfterHit; }
bool32 IsSprayLeppaBlunderActivation(enum HoldEffect holdEffect)   { return gHoldEffectsInfo[holdEffect].sprayLeppaBlunder; }
bool32 IsLifeOrbShellBellActivation(enum HoldEffect holdEffect)    { return gHoldEffectsInfo[holdEffect].lifeOrbShellBell; }
bool32 IsLeftoversActivation(enum HoldEffect holdEffect)           { return gHoldEffectsInfo[holdEffect].leftovers; }
bool32 IsOrbsActivation(enum HoldEffect holdEffect)                { return gHoldEffectsInfo[holdEffect].orbs; }
bool32 IsOnEffectActivation(enum HoldEffect holdEffect)            { return gHoldEffectsInfo[holdEffect].onEffect; }
bool32 IsOnBerryActivation(enum HoldEffect holdEffect)             { return GetItemPocket(gLastUsedItem) == POCKET_BERRIES; }
bool32 IsOnFlingActivation(enum HoldEffect holdEffect)             { return gHoldEffectsInfo[holdEffect].onFling; }
bool32 IsBoosterEnergyActivation(enum HoldEffect holdEffect)       { return gHoldEffectsInfo[holdEffect].boosterEnergy; }
bool32 IsOrbsWhiteHerbActivation(enum HoldEffect holdEffect)       { return gHoldEffectsInfo[holdEffect].orbsWhiteHerbActivation; }

bool32 IsForceTriggerItemActivation(enum HoldEffect holdEffect)
{
    return gHoldEffectsInfo[holdEffect].onSwitchIn
        || gHoldEffectsInfo[holdEffect].whiteHerb
        || gHoldEffectsInfo[holdEffect].onStatusChange
        || gHoldEffectsInfo[holdEffect].onHpThreshold;
}

static enum ItemEffect TryDoublePrize(enum BattlerId battler)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (IsOnPlayerSide(battler) && !gBattleStruct->moneyMultiplierItem)
    {
        gBattleStruct->moneyMultiplier *= 2;
        gBattleStruct->moneyMultiplierItem = TRUE;
    }

    return effect;
}

enum ItemEffect TryBoosterEnergy(enum BattlerId battler, enum Ability ability)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (gBattleMons[battler].volatiles.boosterEnergyActivated || gBattleMons[battler].volatiles.transformed)
        return ITEM_NO_EFFECT;

    if (((ability == ABILITY_PROTOSYNTHESIS) && !(GetWeather() & B_WEATHER_SUN))
     || ((ability == ABILITY_QUARK_DRIVE) && gFieldTimers.terrain != B_TERRAIN_ELECTRIC))
    {
        gBattleMons[battler].volatiles.paradoxBoostedStat = GetParadoxHighestStatId(battler);
        PREPARE_STAT_BUFFER(gBattleTextBuff1, gBattleMons[battler].volatiles.paradoxBoostedStat);
        gBattlerAbility = gBattleScripting.battler = battler;
        gBattleMons[battler].volatiles.boosterEnergyActivated = TRUE;
        RecordAbilityBattle(battler, ability);
        BattleScriptCall(BattleScript_BoosterEnergy);
        effect = ITEM_EFFECT_OTHER;
    }

    return effect;
}

static enum ItemEffect TryRoomService(enum BattlerId battler)
{
    if (gFieldStatuses & STATUS_FIELD_TRICK_ROOM && CompareStat(battler, STAT_SPEED, MIN_STAT_STAGE, CMP_GREATER_THAN, GetBattlerAbility(battler)))
    {
        gEffectBattler = gBattleScripting.battler = battler;
        gLastUsedItem = gBattleMons[battler].item;
        SetStatChange(battler, STAT_SPEED, -1);
        BattleScriptCall(BattleScript_ConsumableItemStatRaise);
        return ITEM_STATS_CHANGE;
    }

    return ITEM_NO_EFFECT;
}

static enum ItemEffect TryTerrainSeeds(enum BattlerId battler, enum Item item)
{
    struct TerrainInfo battleTerrain = gBattleTerrainInfo[gFieldTimers.terrain];
    if (gFieldTimers.terrain != B_TERRAIN_NONE
     && GetItemHoldEffectParam(item) == battleTerrain.seedHoldEffect
     && CompareStat(battler, battleTerrain.seedStat, MAX_STAT_STAGE, CMP_LESS_THAN, GetBattlerAbility(battler)))
    {
        gEffectBattler = gBattleScripting.battler = battler;
        SetStatChange(battler, battleTerrain.seedStat, 1);
        BattleScriptCall(BattleScript_ConsumableItemStatRaise);
        return ITEM_STATS_CHANGE;
    }
    return ITEM_NO_EFFECT;
}

static enum ItemEffect TryBerserkGene(enum BattlerId battler)
{
    if (CompareStat(battler, STAT_ATK, MAX_STAT_STAGE, CMP_EQUAL, GetBattlerAbility(battler)))
        return ITEM_NO_EFFECT;

    if (UsesPolishedLifecycle())
    {
        SetStatChange(battler, STAT_ATK, 2);
        BattleScriptCall(BattleScript_PolishedBerserkGene);
        return ITEM_STATS_CHANGE;
    }

    if (CanBeConfused(battler, battler))
        gBattleMons[battler].volatiles.confusionTimer = PERMANENT_VOLATILE;

    SetStatChange(battler, STAT_ATK, 2);
    BattleScriptCall(BattleScript_BerserkGene);
    return ITEM_STATS_CHANGE;
}

static EWRAM_DATA enum BattlerId sPolishedPreActionOrder[MAX_BATTLERS_COUNT] = {0};
static EWRAM_DATA u8 sPolishedPreActionIndex = 0;

void ResetPolishedPreActionItems(void)
{
    sPolishedPreActionIndex = 0;
}

static bool32 IsPolishedPreActionActivation(enum HoldEffect holdEffect)
{
    return holdEffect == HOLD_EFFECT_BERSERK_GENE;
}

bool32 PolishedPreActionItems(void)
{
    if (!UsesPolishedLifecycle())
        return FALSE;

    if (sPolishedPreActionIndex == 0)
    {
        SortPolishedBattlersBySpeed(sPolishedPreActionOrder);
        sPolishedPreActionIndex = 1;
    }
    // HandleBerserkGene visits both sides in a single Speed/Trick Room order.
    // Consumption can activate Unburden, but must not reorder this pass.
    while (sPolishedPreActionIndex <= gBattlersCount)
    {
        enum BattlerId battler = sPolishedPreActionOrder[sPolishedPreActionIndex++ - 1];
        if (ItemBattleEffects(battler, 0, GetBattlerHoldEffect(battler), IsPolishedPreActionActivation))
            return TRUE;
    }
    // Both genes finish before the source's separate Mirror Herb checkpoint.
    if (PolishedCheckMirrorHerb())
        return TRUE;
    ResetPolishedPreActionItems();
    return FALSE;
}

static enum ItemEffect RestoreWhiteHerbStats(enum BattlerId battler)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    for (u32 i = 0; i < NUM_BATTLE_STATS; i++)
    {
        if (gBattleMons[battler].statStages[i] < DEFAULT_STAT_STAGE)
        {
            gBattleMons[battler].statStages[i] = DEFAULT_STAT_STAGE;
            effect = ITEM_STATS_CHANGE;
        }
    }

    if (effect != ITEM_NO_EFFECT)
        BattleScriptCall(BattleScript_WhiteHerbRet);

    return effect;
}

static enum ItemEffect TryConsumeMirrorHerb(enum BattlerId battler)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (gProtectStructs[battler].eatMirrorHerb)
    {
        for (enum Stat stat = STAT_ATK; stat < NUM_BATTLE_STATS; stat++)
        {
            u32 queuedStat = stat - 1;
            if (gQueuedStatBoosts[battler].stats & (1 << queuedStat))
                SetStatChange(battler, stat, gQueuedStatBoosts[battler].statChanges[queuedStat]);

        }
        gProtectStructs[battler].eatMirrorHerb = FALSE;
        BattleScriptCall(BattleScript_MirrorHerbCopyStatChange);
        effect = ITEM_STATS_CHANGE;
    }

    return effect;
}

static enum ItemEffect TryKingsRock(enum BattlerId battlerAtk, enum BattlerId battlerDef, enum Item item)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (!IsBattlerAlive(battlerDef)
     || !IsBattlerTurnDamaged(battlerDef, EXCLUDING_SUBSTITUTES)
     || MoveIgnoresKingsRock(gCurrentMove)
     || MoveHasAdditionalEffect(gCurrentMove, MOVE_EFFECT_FLINCH))
        return effect;

    enum Ability ability = GetBattlerAbility(battlerAtk);
    u32 holdEffectParam = GetItemHoldEffectParam(item);

    if ((B_SERENE_GRACE_BOOST >= GEN_5 && ability == ABILITY_SERENE_GRACE)
     || ((gSideStatuses[GetBattlerSide(battlerAtk)] & SIDE_STATUS_RAINBOW) && gCurrentMove != MOVE_SECRET_POWER))
        holdEffectParam *= 2;
    if (ability != ABILITY_STENCH && RandomPercentage(RNG_HOLD_EFFECT_FLINCH, holdEffectParam))
    {
        SetMoveEffectHelper(battlerAtk, battlerDef, MOVE_EFFECT_FLINCH, gBattlescriptCurrInstr, EFFECT_FLINCH_FROM_ITEM_OR_ABILITY);
        effect = ITEM_EFFECT_OTHER;
    }

    return effect;
}

static enum ItemEffect TryAirBalloon(enum BattlerId battler, ActivationTiming timing)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (timing == IsOnTargetHitActivation)
    {
        // If the holder or its substitute is hit by a damaging move (even if it has Disguise), the Air Balloon is destroyed.
        if (UsesPolishedActionRules(gCurrentMove)
            ? IsBattlerAlive(battler) && CanHitTriggerContactStatus(gBattlerAttacker, battler, gCurrentMove)
            : IsBattlerTurnDamaged(battler, INCLUDING_SUBSTITUTES))
        {
            BattleScriptCall(BattleScript_AirBalloonMsgPop);
            effect = ITEM_EFFECT_OTHER;
        }
    }
    else if (gBattleStruct->battlerState[battler].switchIn)
    {
        BattleScriptCall(BattleScript_AirBalloonMsgInRet);
        RecordItemEffectBattle(battler, HOLD_EFFECT_AIR_BALLOON);
        effect = ITEM_EFFECT_OTHER;
    }

    return effect;
}

static enum ItemEffect TryRockyHelmet(enum BattlerId battlerDef, enum BattlerId battlerAtk, enum Item item)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;
    enum Ability ability = GetBattlerAbility(battlerAtk);

    if ((UsesPolishedActionRules(gCurrentMove)
            ? CanHitTriggerContactStatus(battlerAtk, battlerDef, gCurrentMove)
            : IsBattlerTurnDamaged(battlerDef, EXCLUDING_SUBSTITUTES))
     && IsBattlerAlive(battlerAtk)
     && (UsesPolishedActionRules(gCurrentMove)
            ? DoesContactStatusMoveMakeContact(battlerAtk, battlerDef, gCurrentMove, TRUE)
            : !CanBattlerAvoidContactEffects(battlerAtk, battlerDef, ability, GetBattlerHoldEffect(battlerAtk), gCurrentMove))
     && !IsAbilityAndRecord(battlerAtk, ability, ABILITY_MAGIC_GUARD))
    {
        SetPassiveDamageAmount(battlerAtk, GetNonDynamaxMaxHP(battlerAtk) / 6);
        PREPARE_ITEM_BUFFER(gBattleTextBuff1, item);
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_HURT_BY_ITEM;
        BattleScriptCall(BattleScript_RockyHelmetActivates);
        effect = ITEM_HP_CHANGE;
    }

    return effect;
}

static enum ItemEffect TryWeaknessPolicy(enum BattlerId battlerDef)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;
    enum PolishedWeaknessPolicyPhase phase = GetPolishedWeaknessPolicyPhase(gCurrentMove);
    bool32 source = phase != POLISHED_POLICY_NATIVE;

    // Source supereffectivetext runs through a doll, only on the final normal
    // multihit strike and never on Parental Bond's second strike. Its two stat
    // attempts consume the policy only if at least one can actually change.
    if (source
     && ((phase == POLISHED_POLICY_AFTER_HIT_EFFECTS && !IsBattlerAlive(gBattlerAttacker))
      || (gMultiHitCounter > 1 && gSpecialStatuses[gBattlerAttacker].parentalBondState == PARENTAL_BOND_OFF)
      || gSpecialStatuses[gBattlerAttacker].parentalBondState == PARENTAL_BOND_2ND_HIT
      || (!CompareStat(battlerDef, STAT_ATK, MAX_STAT_STAGE, CMP_LESS_THAN, GetBattlerAbility(battlerDef))
       && !CompareStat(battlerDef, STAT_SPATK, MAX_STAT_STAGE, CMP_LESS_THAN, GetBattlerAbility(battlerDef)))))
        return effect;

    // A source False Swipe can write zero HP damage and still reach its
    // super-effective command. The item wrapper already requires a survivor.
    if ((source ? !IsBattlerUnaffectedByMove(battlerDef) : IsBattlerTurnDamaged(battlerDef, EXCLUDING_SUBSTITUTES))
     && gBattleStruct->moveResultFlags[battlerDef] & MOVE_RESULT_HIGH_EFFECTIVENESS)
    {
        SetStatChange(battlerDef, STAT_ATK, 2);
        SetStatChange(battlerDef, STAT_SPATK, 2);
        // Source SwitchTurn makes these user stat changes, so the holder's
        // Substitute cannot block them. Keep the native item script outside
        // the source action scope.
        BattleScriptCall(source ? BattleScript_ConsumableItemStatRaise : BattleScript_ItemStatChange);
        effect = ITEM_STATS_CHANGE;
    }

    return effect;
}

static enum ItemEffect TrySnowball(enum BattlerId battlerDef)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (IsBattlerTurnDamaged(battlerDef, EXCLUDING_SUBSTITUTES)
     && GetBattleMoveType(gCurrentMove) == TYPE_ICE)
    {
        SetStatChange(battlerDef, STAT_ATK, 1);
        BattleScriptCall(BattleScript_ItemStatChange);
        effect = ITEM_STATS_CHANGE;
    }

    return effect;
}

static enum ItemEffect TryLuminousMoss(enum BattlerId battlerDef)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (IsBattlerTurnDamaged(battlerDef, EXCLUDING_SUBSTITUTES)
     && GetBattleMoveType(gCurrentMove) == TYPE_WATER)
    {
        SetStatChange(battlerDef, STAT_SPDEF, 1);
        BattleScriptCall(BattleScript_ItemStatChange);
        effect = ITEM_STATS_CHANGE;
    }

    return effect;
}

static enum ItemEffect TryCellBattery(enum BattlerId battlerDef)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (IsBattlerTurnDamaged(battlerDef, EXCLUDING_SUBSTITUTES)
     && GetBattleMoveType(gCurrentMove) == TYPE_ELECTRIC)
    {
        SetStatChange(battlerDef, STAT_ATK, 1);
        BattleScriptCall(BattleScript_ItemStatChange);
        effect = ITEM_STATS_CHANGE;
    }

    return effect;
}

static enum ItemEffect TryAbsorbBulb(enum BattlerId battlerDef)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (IsBattlerTurnDamaged(battlerDef, EXCLUDING_SUBSTITUTES)
     && GetBattleMoveType(gCurrentMove) == TYPE_WATER)
    {
        SetStatChange(battlerDef, STAT_SPATK, 1);
        BattleScriptCall(BattleScript_ItemStatChange);
        effect = ITEM_STATS_CHANGE;
    }

    return effect;
}

static enum ItemEffect TryJabocaBerry(enum BattlerId battlerDef, enum BattlerId battlerAtk, enum Item item)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (IsBattlerAlive(battlerAtk)
     && (UsesPolishedActionRules(gCurrentMove)
            ? CanHitTriggerContactStatus(battlerAtk, battlerDef, gCurrentMove)
            : IsBattlerTurnDamaged(battlerDef, EXCLUDING_SUBSTITUTES))
     && GetMoveEffect(gCurrentMove) != EFFECT_FUTURE_SIGHT
     && IsBattleMovePhysical(gCurrentMove)
     && !IsAbilityAndRecord(battlerAtk, GetBattlerAbility(battlerAtk), ABILITY_MAGIC_GUARD))
    {
        s32 jabocaDamage = GetNonDynamaxMaxHP(battlerAtk) / 8;
        if (GetBattlerAbility(battlerDef) == ABILITY_RIPEN)
            jabocaDamage *= 2;
        SetPassiveDamageAmount(battlerAtk, jabocaDamage);
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_HURT_BY_ITEM;
        BattleScriptCall(BattleScript_JabocaRowapBerryActivates);
        PREPARE_ITEM_BUFFER(gBattleTextBuff1, item);
        effect = ITEM_HP_CHANGE;
    }

    return effect;
}

static enum ItemEffect TryRowapBerry(enum BattlerId battlerDef, enum BattlerId battlerAtk, enum Item item)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (IsBattlerAlive(battlerAtk)
     && (UsesPolishedActionRules(gCurrentMove)
            ? CanHitTriggerContactStatus(battlerAtk, battlerDef, gCurrentMove)
            : IsBattlerTurnDamaged(battlerDef, EXCLUDING_SUBSTITUTES))
     && GetMoveEffect(gCurrentMove) != EFFECT_FUTURE_SIGHT
     && IsBattleMoveSpecial(gCurrentMove)
     && !IsAbilityAndRecord(battlerAtk, GetBattlerAbility(battlerAtk), ABILITY_MAGIC_GUARD))
    {
        s32 rowapDamage = GetNonDynamaxMaxHP(battlerAtk) / 8;
        if (GetBattlerAbility(battlerDef) == ABILITY_RIPEN)
            rowapDamage *= 2;
        SetPassiveDamageAmount(battlerAtk, rowapDamage);
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_HURT_BY_ITEM;
        BattleScriptCall(BattleScript_JabocaRowapBerryActivates);
        PREPARE_ITEM_BUFFER(gBattleTextBuff1, item);
        effect = ITEM_HP_CHANGE;
    }

    return effect;
}

static enum ItemEffect TrySetEnigmaBerry(enum BattlerId battlerDef, enum BattlerId battlerAtk)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (((IsBattlerTurnDamaged(battlerDef, EXCLUDING_SUBSTITUTES) && gBattleStruct->moveResultFlags[battlerDef] & MOVE_RESULT_HIGH_EFFECTIVENESS) || gBattleScripting.overrideBerryRequirements)
     && !(gBattleScripting.overrideBerryRequirements && gBattleMons[battlerDef].hp == gBattleMons[battlerDef].maxHP)
     && !(B_HEAL_BLOCKING >= GEN_5 && gBattleMons[battlerDef].volatiles.healBlockTimer))
    {
        s32 healAmount = gBattleMons[battlerDef].maxHP * 25 / 100;
        if (GetBattlerAbility(battlerDef) == ABILITY_RIPEN)
            healAmount *= 2;
        SetHealAmount(battlerDef, healAmount);
        BattleScriptCall(BattleScript_ItemHealHP_RemoveBerry);
        effect = ITEM_HP_CHANGE;
    }

    return effect;
}

static enum ItemEffect TryBlunderPolicy(enum BattlerId battlerAtk)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (gBattleStruct->blunderPolicy
     && gBattleStruct->battlerState[battlerAtk].originalBattlerPartyId == PARTY_SIZE
     && CompareStat(battlerAtk, STAT_SPEED, MAX_STAT_STAGE, CMP_LESS_THAN, GetBattlerAbility(battlerAtk)))
    {
        gBattleStruct->blunderPolicy = FALSE;
        SetStatChange(battlerAtk, STAT_SPEED, 2);
        BattleScriptCall(BattleScript_ItemStatChange);
        effect = ITEM_STATS_CHANGE;
    }

    gBattleStruct->blunderPolicy = FALSE;
    return effect;
}

static enum ItemEffect TryMentalHerb(enum BattlerId battler, ActivationTiming timing)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;
    gBattleCommunication[MULTISTRING_CHOOSER] = 0;

    // Check infatuation
    if (gBattleMons[battler].volatiles.infatuation)
    {
        gBattleMons[battler].volatiles.infatuation = 0;
        gBattleCommunication[MULTISTRING_CHOOSER] |= 1 << B_MSG_MENTALHERBCURE_INFATUATION;
        effect = ITEM_EFFECT_OTHER;
    }
    if (B_MENTAL_HERB >= GEN_5)
    {
        // Check torment
        if (gBattleMons[battler].volatiles.torment == TRUE)
        {
            gBattleMons[battler].volatiles.torment = FALSE;
            gBattleMons[battler].volatiles.tormentTimer = 0;
            gBattleCommunication[MULTISTRING_CHOOSER] |= 1 << B_MSG_MENTALHERBCURE_TORMENT;
            effect = ITEM_EFFECT_OTHER;
        }
        // Check disable
        if (gBattleMons[battler].volatiles.disableTimer != 0)
        {
            gBattleMons[battler].volatiles.disabledMove = MOVE_NONE;
            gBattleMons[battler].volatiles.disableTimer = 0;
            gBattleCommunication[MULTISTRING_CHOOSER] |= 1 << B_MSG_MENTALHERBCURE_DISABLE;
            effect = ITEM_EFFECT_OTHER;
        }
        // Check heal block
        if (gBattleMons[battler].volatiles.healBlockTimer)
        {
            gBattleMons[battler].volatiles.healBlockTimer = 0;
            gBattleCommunication[MULTISTRING_CHOOSER] |= 1 << B_MSG_MENTALHERBCURE_HEALBLOCK;
            effect = ITEM_EFFECT_OTHER;
        }
        // Check encore
        if (gBattleMons[battler].volatiles.encoreTimer != 0)
        {
            gBattleMons[battler].volatiles.encoredMove = MOVE_NONE;
            gBattleMons[battler].volatiles.encoreTimer = 0;
            gBattleCommunication[MULTISTRING_CHOOSER] |= 1 << B_MSG_MENTALHERBCURE_ENCORE;
            effect = ITEM_EFFECT_OTHER;
        }
        // Check taunt
        if (gBattleMons[battler].volatiles.tauntTimer != 0)
        {
            gBattleMons[battler].volatiles.tauntTimer = 0;
            gBattleCommunication[MULTISTRING_CHOOSER] |= 1 << B_MSG_MENTALHERBCURE_TAUNT;
            PREPARE_MOVE_BUFFER(gBattleTextBuff1, MOVE_TAUNT);
            effect = ITEM_EFFECT_OTHER;
        }
    }

    if (effect)
        BattleScriptCall(BattleScript_MentalHerbCureRet);

    return effect;
}

static enum ItemEffect TryThroatSpray(enum BattlerId battlerAtk)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;
    bool32 affected = UsesPolishedActionRules(gCurrentMove) && IsPolishedPerishSong(gCurrentMove, battlerAtk)
        ? gBattleStruct->polishedPerishSongSprayEligible
        : (IsAnyTargetTurnDamaged(battlerAtk, INCLUDING_SUBSTITUTES)
        || (GetBattleMoveCategory(gCurrentMove) == DAMAGE_CATEGORY_STATUS && IsAnyTargetAffected()));

    if (IsSoundMove(gCurrentMove)
     && gBattleStruct->battlerState[battlerAtk].originalBattlerPartyId == PARTY_SIZE
     && !gBattleStruct->unableToUseMove
     && affected
     && CompareStat(battlerAtk, STAT_SPATK, MAX_STAT_STAGE, CMP_LESS_THAN, GetBattlerAbility(battlerAtk))
     && !NoAliveMonsForEitherParty())   // Don't activate if battle will end
    {
        SetStatChange(battlerAtk, STAT_SPATK, 1);
        BattleScriptCall(BattleScript_ItemStatChange);
        effect = ITEM_STATS_CHANGE;
    }

    return effect;
}

static enum ItemEffect DamagedStatBoostBerryEffect(enum BattlerId battlerDef, enum BattlerId battlerAtk, enum Stat statId, enum DamageCategory category)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (!IsBattlerAlive(battlerDef) || !CompareStat(battlerDef, statId, MAX_STAT_STAGE, CMP_LESS_THAN, GetBattlerAbility(battlerDef)))
        return effect;

    if (gBattleScripting.overrideBerryRequirements
     || (GetBattleMoveCategory(gCurrentMove) == category
      && IsBattlerTurnDamaged(battlerDef, UsesPolishedActionRules(gCurrentMove) ? INCLUDING_SUBSTITUTES : EXCLUDING_SUBSTITUTES)))
    {
        if (GetBattlerAbility(battlerDef) == ABILITY_RIPEN)
        {
            SetStatChange(battlerDef, statId, 2);
            BattleScriptCall(BattleScript_ConsumableBerryStatRaiseRipen);
        }
        else
        {
            SetStatChange(battlerDef, statId, 1);
            BattleScriptCall(BattleScript_ConsumableBerryStatRaise);
        }

        effect = ITEM_STATS_CHANGE;
    }

    return effect;
}

static enum ItemEffect TryShellBell(enum BattlerId battlerAtk)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (gBattleScripting.savedDmg > 0
     && !gBattleStruct->unableToUseMove
     && GetMoveEffect(gCurrentMove) != EFFECT_FUTURE_SIGHT
     && gBattleStruct->battlerState[battlerAtk].originalBattlerPartyId == PARTY_SIZE
     && !IsBattlerAtMaxHp(battlerAtk)
     && !(B_HEAL_BLOCKING >= GEN_5 && gBattleMons[battlerAtk].volatiles.healBlockTimer))
    {
        if (EmergencyExitCanBeTriggered(battlerAtk, GetBattlerAbility(battlerAtk)))
            gSpecialStatuses[battlerAtk].shellBellEmergencyExit = TRUE;
        SetHealAmount(battlerAtk, gBattleScripting.savedDmg / GetBattlerHoldEffectParam(battlerAtk));
        BattleScriptCall(BattleScript_ItemHealHP_Ret);
        effect = ITEM_HP_CHANGE;
    }

    return effect;
}

static enum ItemEffect TryLifeOrb(enum BattlerId battlerAtk)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (!gBattleStruct->unableToUseMove
     && gBattleStruct->battlerState[battlerAtk].originalBattlerPartyId == PARTY_SIZE
     && (IsAnyTargetTurnDamaged(battlerAtk, INCLUDING_SUBSTITUTES) || gBattleScripting.savedDmg > 0)
     && !IsAbilityAndRecord(battlerAtk, GetBattlerAbility(battlerAtk), ABILITY_MAGIC_GUARD))
    {
        SetPassiveDamageAmount(battlerAtk, GetNonDynamaxMaxHP(battlerAtk) / 10);
        BattleScriptCall(BattleScript_LifeOrbActivates);
        effect = ITEM_HP_CHANGE;
    }

    return effect;
}

static enum ItemEffect TryStickyBarbOnTargetHit(enum BattlerId battlerDef, enum BattlerId battlerAtk, enum Item item)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (IsBattlerTurnDamaged(battlerDef, EXCLUDING_SUBSTITUTES)
     && !CanBattlerAvoidContactEffects(battlerAtk, battlerDef, GetBattlerAbility(battlerAtk), GetBattlerHoldEffect(battlerAtk), gCurrentMove)
     && !DoesSubstituteBlockMove(battlerAtk, battlerDef, gCurrentMove)
     && IsBattlerAlive(battlerAtk)
     && CanStealItem(battlerAtk, battlerDef, item)
     && gBattleMons[battlerAtk].item == ITEM_NONE)
    {
        // No sticky hold checks.
        gEffectBattler = battlerDef;
        StealTargetItem(battlerAtk, battlerDef, ITEM_NONE, FALSE);  // Attacker takes target's barb
        BattleScriptCall(BattleScript_StickyBarbTransfer);
        effect = ITEM_EFFECT_OTHER;
    }

    return effect;
}

static enum ItemEffect TryStickyBarbOnEndTurn(enum BattlerId battler, enum Item item)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (!IsAbilityAndRecord(battler, GetBattlerAbility(battler), ABILITY_MAGIC_GUARD))
    {
        SetPassiveDamageAmount(battler, GetNonDynamaxMaxHP(battler) / 8);
        PREPARE_ITEM_BUFFER(gBattleTextBuff1, item);
        BattleScriptCall(BattleScript_ItemHurtWithAnim);
        effect = ITEM_HP_CHANGE;
    }

    return effect;
}

static enum ItemEffect TryToxicOrb(enum BattlerId battler)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (CanStatusOrbInflictStatus(battler, MOVE_EFFECT_TOXIC)) // Corrosion bypasses Poison/Steel-type poison immunity
    {
        gBattleMons[battler].status1 = STATUS1_TOXIC_POISON;
        gEffectBattler = battler;
        gBattleCommunication[MULTISTRING_CHOOSER] = 0;
        BattleScriptCall(BattleScript_ToxicOrbActivates);
        effect = ITEM_STATUS_CHANGE;
    }

    return effect;
}

static enum ItemEffect TryFlameOrb(enum BattlerId battler)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (CanStatusOrbInflictStatus(battler, MOVE_EFFECT_BURN))
    {
        gBattleMons[battler].status1 = STATUS1_BURN;
        gEffectBattler = battler;
        gBattleCommunication[MULTISTRING_CHOOSER] = 0;
        BattleScriptCall(BattleScript_FlameOrbActivates);
        effect = ITEM_STATUS_CHANGE;
    }

    return effect;
}

static enum ItemEffect TryLeftovers(enum BattlerId battler, enum HoldEffect holdEffect)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (gBattleMons[battler].hp < gBattleMons[battler].maxHP
     && !(B_HEAL_BLOCKING >= GEN_5 && gBattleMons[battler].volatiles.healBlockTimer))
    {
        SetHealAmount(battler, GetNonDynamaxMaxHP(battler) / 16);
        RecordItemEffectBattle(battler, holdEffect);
        BattleScriptCall(BattleScript_ItemHealHP_Ret);
        effect = ITEM_HP_CHANGE;
    }

    return effect;
}

static enum ItemEffect TryBlackSludgeDamage(enum BattlerId battler, enum HoldEffect holdEffect)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (UsesPolishedLifecycle() || !IsAbilityAndRecord(battler, GetBattlerAbility(battler), ABILITY_MAGIC_GUARD))
    {
        SetPassiveDamageAmount(battler, GetNonDynamaxMaxHP(battler) / 8);
        RecordItemEffectBattle(battler, holdEffect);
        BattleScriptCall(BattleScript_ItemHurtWithAnim);
        effect = ITEM_HP_CHANGE;
    }

    return effect;
}

static enum ItemEffect TryCureParalysis(enum BattlerId battler)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (gBattleMons[battler].status1 & STATUS1_PARALYSIS)
    {
        gBattleMons[battler].status1 &= ~STATUS1_PARALYSIS;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_CURED_PARALYSIS;
        BattleScriptCall(BattleScript_BerryCureStatusRet);
        effect = ITEM_STATUS_CHANGE;
    }

    return effect;
}

static enum ItemEffect TryCurePoison(enum BattlerId battler)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (gBattleMons[battler].status1 & STATUS1_PSN_ANY)
    {
        gBattleMons[battler].status1 &= ~(STATUS1_PSN_ANY | STATUS1_TOXIC_COUNTER);
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_CURED_POISON;
        BattleScriptCall(BattleScript_BerryCureStatusRet);
        effect = ITEM_STATUS_CHANGE;
    }

    return effect;
}

static enum ItemEffect TryCureBurn(enum BattlerId battler)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (gBattleMons[battler].status1 & STATUS1_BURN)
    {
        gBattleMons[battler].status1 &= ~STATUS1_BURN;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_CURED_BURN;
        BattleScriptCall(BattleScript_BerryCureStatusRet);
        effect = ITEM_STATUS_CHANGE;
    }

    return effect;
}

static enum ItemEffect TryCureFreezeOrFrostbite(enum BattlerId battler)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (gBattleMons[battler].status1 & STATUS1_FREEZE)
    {
        gBattleMons[battler].status1 &= ~STATUS1_FREEZE;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_CURED_FREEZE;
        effect = ITEM_STATUS_CHANGE;
    }
    else if (gBattleMons[battler].status1 & STATUS1_FROSTBITE)
    {
        gBattleMons[battler].status1 &= ~STATUS1_FROSTBITE;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_CURED_FROSTBITE;
        effect = ITEM_STATUS_CHANGE;
    }

    if (effect == ITEM_STATUS_CHANGE)
    {
        GetBattlerPartyState(battler)->freezeTurns = 0;
        BattleScriptCall(BattleScript_BerryCureStatusRet);
    }

    return effect;
}

static enum ItemEffect TryCureSleep(enum BattlerId battler)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (gBattleMons[battler].status1 & STATUS1_SLEEP)
    {
        gBattleMons[battler].status1 &= ~STATUS1_SLEEP;
        gBattleMons[battler].volatiles.nightmare = FALSE;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_CURED_SLEEP;
        TryDeactivateSleepClause(battler, gBattlerPartyIndexes[battler]);
        BattleScriptCall(BattleScript_BerryCureStatusRet);
        effect = ITEM_STATUS_CHANGE;
    }

    return effect;
}

static enum ItemEffect TryCureConfusion(enum BattlerId battler)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (gBattleMons[battler].volatiles.confusionTimer > 0)
    {
        gBattleMons[battler].volatiles.confusionTimer = 0;
        BattleScriptCall(BattleScript_BerryCureConfusionRet);
        effect = ITEM_EFFECT_OTHER;
    }

    return effect;
}

static enum ItemEffect TryCureAnyStatus(enum BattlerId battler)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;
    bool32 curedStatus = FALSE;
    bool32 curedConfusion = FALSE;

    if (gBattleMons[battler].status1 & STATUS1_ANY || gBattleMons[battler].volatiles.confusionTimer > 0)
    {
        if (gBattleMons[battler].status1 & STATUS1_PSN_ANY)
        {
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_CURED_POISON;
            curedStatus = TRUE;
        }
        if (gBattleMons[battler].status1 & STATUS1_SLEEP)
        {
            gBattleMons[battler].volatiles.nightmare = FALSE;
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_CURED_SLEEP;
            TryDeactivateSleepClause(battler, gBattlerPartyIndexes[battler]);
            curedStatus = TRUE;
        }
        if (gBattleMons[battler].status1 & STATUS1_PARALYSIS)
        {
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_CURED_PARALYSIS;
            curedStatus = TRUE;
        }
        if (gBattleMons[battler].status1 & STATUS1_BURN)
        {
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_CURED_BURN;
            curedStatus = TRUE;
        }
        if (gBattleMons[battler].status1 & STATUS1_FREEZE)
        {
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_CURED_FREEZE;
            curedStatus = TRUE;
        }
        if (gBattleMons[battler].status1 & STATUS1_FROSTBITE)
        {
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_CURED_FROSTBITE;
            curedStatus = TRUE;
        }
        if (gBattleMons[battler].volatiles.confusionTimer > 0)
            curedConfusion = TRUE;
        gBattleMons[battler].status1 = 0;
        gBattleMons[battler].volatiles.confusionTimer = 0;
        if (curedStatus && curedConfusion)
            BattleScriptCall(BattleScript_BerryCureStatusAndConfusionRet);
        else if (curedConfusion)
            BattleScriptCall(BattleScript_BerryCureConfusionRet);
        else
            BattleScriptCall(BattleScript_BerryCureStatusRet);
        effect = ITEM_STATUS_CHANGE;
    }

    return effect;
}

enum HealAmount
{
    FIXED_HEAL_AMOUNT,
    PERCENT_HEAL_AMOUNT,
};

static enum ItemEffect ItemHealHp(enum BattlerId battler, enum Item itemId, enum HealAmount percentHeal)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;
    enum Ability ability = GetBattlerAbility(battler);

    if (!(gBattleScripting.overrideBerryRequirements && gBattleMons[battler].hp == gBattleMons[battler].maxHP)
     && !(B_HEAL_BLOCKING >= GEN_5 && gBattleMons[battler].volatiles.healBlockTimer)
     && HasEnoughHpToEatBerry(battler, ability, 2, itemId))
    {
        s32 healAmount = 0;
        if (percentHeal == PERCENT_HEAL_AMOUNT)
            healAmount = (GetNonDynamaxMaxHP(battler) * GetItemHoldEffectParam(itemId) / 100);
        else
            healAmount = GetItemHoldEffectParam(itemId);

        if (ability == ABILITY_RIPEN && GetItemPocket(itemId) == POCKET_BERRIES)
            healAmount *= 2;

        SetHealAmount(battler, healAmount);
        if (GetItemPocket(itemId) == POCKET_BERRIES)
            BattleScriptCall(BattleScript_ItemHealHP_RemoveBerry);
        else
            BattleScriptCall(BattleScript_ItemHealHP_RemoveItem);
        effect = ITEM_HP_CHANGE;
    }

    return effect;
}

static enum ItemEffect ItemRestorePp(enum BattlerId battler, enum Item itemId)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;
    struct Pokemon *mon = GetBattlerMon(battler);
    u32 changedPP = 0;
    u32 restoreMove = MAX_MON_MOVES;
    u32 missingMove = MAX_MON_MOVES;
    bool32 override = gBattleScripting.overrideBerryRequirements;
    bool32 source = UsesPolishedLifecycle();
    u32 ppBonuses = source ? gBattleMons[battler].ppBonuses : GetMonData(mon, MON_DATA_PP_BONUSES);
    enum Ability ability = GetBattlerAbility(battler);

    for (u32 i = 0; i < MAX_MON_MOVES; i++)
    {
        enum Move move = source ? gBattleMons[battler].moves[i] : GetMonData(mon, MON_DATA_MOVE1 + i);
        u32 currentPP = source ? gBattleMons[battler].pp[i] : GetMonData(mon, MON_DATA_PP1 + i);
        if (move == MOVE_NONE)
        {
            if (source)
                break;
            continue;
        }

        // Stolen/reconsumed Leppa uses GetNonfullPPMove, a single ordered
        // scan comparing the maximum with the raw source PP byte, including
        // its PP Ups. Even a full boosted slot therefore qualifies. Keep
        // optional Expansion moves' independent, wider PP representation.
        u32 maxPP = CalculatePPWithBonus(move, ppBonuses, i);
        u32 packedPP = currentPP | (((ppBonuses >> (2 * i)) & 3) << 6);
        if (source && override
         && (PolishedGetMovePP(move) ? packedPP != maxPP : currentPP < maxPP))
        {
            restoreMove = i;
            break;
        }

        if (currentPP == 0)
        {
            restoreMove = i;
            break;
        }

        if (override && missingMove == MAX_MON_MOVES)
        {
            u32 maxPP = CalculatePPWithBonus(move, ppBonuses, i);

            if (currentPP < maxPP)
                missingMove = i;
        }
    }

    if (restoreMove == MAX_MON_MOVES && override)
        restoreMove = missingMove;

    if (restoreMove != MAX_MON_MOVES)
    {
        enum Move move = source ? gBattleMons[battler].moves[restoreMove] : GetMonData(mon, MON_DATA_MOVE1 + restoreMove);
        u32 currentPP = source ? gBattleMons[battler].pp[restoreMove] : GetMonData(mon, MON_DATA_PP1 + restoreMove);
        u32 maxPP = CalculatePPWithBonus(move, ppBonuses, restoreMove);
        u32 ppRestored = GetItemHoldEffectParam(itemId);
        bool32 packedSourcePP = source && PolishedGetMovePP(move);

        if (ability == ABILITY_RIPEN)
        {
            ppRestored *= 2;
            gBattlerAbility = battler;
        }
        // LeppaRestorePP adds to the unmasked byte, wraps at 256, clamps to
        // the old boosted maximum, then stores only that new PP value. This
        // source quirk clears this slot's PP Ups, including at zero PP.
        if (packedSourcePP)
        {
            u32 packedPP = currentPP | (((ppBonuses >> (2 * restoreMove)) & 3) << 6);
            changedPP = (packedPP + ppRestored) & 0xff;
        }
        else
            changedPP = currentPP + ppRestored;
        if (changedPP > maxPP)
            changedPP = maxPP;

        PREPARE_MOVE_BUFFER(gBattleTextBuff1, move);

        BattleScriptCall(BattleScript_BerryPPHeal);

        gBattleScripting.battler = battler;
        if (!source || MOVE_IS_PERMANENT(battler, restoreMove))
        {
            if (packedSourcePP)
            {
                // Read the other party slots instead of copying temporary
                // battle slots (Mimic/Transform) back into the saved mon.
                u8 ppData[MAX_MON_MOVES + 1];
                for (u32 i = 0; i < MAX_MON_MOVES; i++)
                    ppData[i] = GetMonData(mon, MON_DATA_PP1 + i);
                ppData[restoreMove] = changedPP;
                ppData[MAX_MON_MOVES] = GetMonData(mon, MON_DATA_PP_BONUSES) & ~(3 << (2 * restoreMove));
                BtlController_EmitSetMonData(battler, B_COMM_TO_CONTROLLER, REQUEST_PP_DATA_BATTLE, 0, sizeof(ppData), ppData);
            }
            else
                BtlController_EmitSetMonData(battler, B_COMM_TO_CONTROLLER, restoreMove + REQUEST_PPMOVE1_BATTLE, 0, 1, &changedPP);
            MarkBattlerForControllerExec(battler);
        }
        if (source || MOVE_IS_PERMANENT(battler, restoreMove))
            gBattleMons[battler].pp[restoreMove] = changedPP;
        if (packedSourcePP)
            gBattleMons[battler].ppBonuses &= ~(3 << (2 * restoreMove));
        effect = ITEM_PP_CHANGE;
    }
    return effect;
}

static enum ItemEffect HealConfuseBerry(enum BattlerId battler, enum Item itemId, enum Flavor flavorId)
{
    if (itemId == ITEM_FIGY_BERRY && PolishedItemHealingEnabled())
    {
        // The source routes Figy through HELD_BERRY, not a flavor/confusion
        // effect. Retain the engine's berry dispatch and consumption scripts.
        if (!PolishedFigyCanHeal(gBattleMons[battler].hp, gBattleMons[battler].maxHP,
                                GetBattlerAbility(battler) == ABILITY_GLUTTONY,
                                gBattleScripting.overrideBerryRequirements))
            return ITEM_NO_EFFECT;
        SetHealAmount(battler, PolishedGetItemHealAmount(itemId, GetNonDynamaxMaxHP(battler)));
        BattleScriptCall(BattleScript_ItemHealHP_RemoveBerry);
        return ITEM_HP_CHANGE;
    }
    enum ItemEffect effect = ITEM_NO_EFFECT;
    u32 hpFraction = B_CONFUSE_BERRIES_HEAL >= GEN_7 ? 4 : 2;
    enum Ability ability = GetBattlerAbility(battler);

    if (HasEnoughHpToEatBerry(battler, ability, hpFraction, itemId)
     && !(B_HEAL_BLOCKING >= GEN_5 && gBattleMons[battler].volatiles.healBlockTimer))
    {
        s32 healAmount = GetNonDynamaxMaxHP(battler) / GetItemHoldEffectParam(itemId);
        if (ability == ABILITY_RIPEN)
            healAmount *= 2;
        SetHealAmount(battler, healAmount);
        if (GetFlavorRelationByPersonality(gBattleMons[battler].personality, flavorId) < 0)
            BattleScriptCall(BattleScript_BerryConfuseHeal);
        else
            BattleScriptCall(BattleScript_ItemHealHP_RemoveBerry);
        effect = ITEM_HP_CHANGE;
    }

    return effect;
}

static enum ItemEffect StatRaiseBerry(enum BattlerId battler, enum Item itemId, enum Stat statId)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;
    enum Ability ability = GetBattlerAbility(battler);

    if (CompareStat(battler, statId, MAX_STAT_STAGE, CMP_LESS_THAN, ability)
     && HasEnoughHpToEatBerry(battler, ability, GetItemHoldEffectParam(itemId), itemId))
    {
        gEffectBattler = gBattleScripting.battler = battler;
        if (ability == ABILITY_RIPEN)
        {
            SetStatChange(battler, statId, 2);
            BattleScriptCall(BattleScript_ConsumableBerryStatRaiseRipen);
        }
        else
        {
            SetStatChange(battler, statId, 1);
            BattleScriptCall(BattleScript_ConsumableBerryStatRaise);
        }
        effect = ITEM_STATS_CHANGE;
    }

    return effect;
}

static enum ItemEffect CriticalHitRatioUp(enum BattlerId battler, enum Item itemId)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (!gBattleMons[battler].volatiles.focusEnergy
     && !gBattleMons[battler].volatiles.dragonCheer
     && HasEnoughHpToEatBerry(battler, GetBattlerAbility(battler), GetItemHoldEffectParam(itemId), itemId))
    {
        gBattleMons[battler].volatiles.focusEnergy = TRUE;
        BattleScriptCall(BattleScript_BerryFocusEnergy);
        effect = ITEM_EFFECT_OTHER;
    }

    return effect;
}

static bool32 PolishedStarfCantSelectStat(u32 stat)
{
    // GetCappedStats builds a raw +6 mask before Contrary is considered.
    return gBattleMons[gBattlerAttacker].statStages[stat] == MAX_STAT_STAGE;
}

static enum ItemEffect RandomStatRaiseBerry(enum BattlerId battler, enum Item itemId)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;
    enum Stat stat;
    enum Ability ability = GetBattlerAbility(battler);
    bool32 source = UsesPolishedLifecycle();

    for (stat = STAT_ATK; stat < NUM_STATS; stat++)
    {
        if (source ? gBattleMons[battler].statStages[stat] < MAX_STAT_STAGE
                   : CompareStat(battler, stat, MAX_STAT_STAGE, CMP_LESS_THAN, ability))
            break;
    }

    if (stat == NUM_STATS)
        return effect;

    if (HasEnoughHpToEatBerry(battler, ability, GetItemHoldEffectParam(itemId), itemId))
    {
        u32 savedAttacker = gBattlerAttacker;
        // MoodyCantRaiseStat requires that the battler is set to gBattlerAttacker
        gBattlerAttacker = gBattleScripting.battler = battler;
        stat = RandomUniformExcept(RNG_RANDOM_STAT_UP, STAT_ATK, NUM_STATS - 1,
            source ? PolishedStarfCantSelectStat : MoodyCantRaiseStat);
        gBattlerAttacker = savedAttacker;

        // A source candidate at -6 still fails after Contrary reverses the
        // change. It does not retry a different stat or consume the berry.
        if (source && !CompareStat(battler, stat, MAX_STAT_STAGE, CMP_LESS_THAN, ability))
            return effect;

        if (ability == ABILITY_RIPEN)
        {
            BattleScriptCall(BattleScript_ConsumableBerryStatRaiseRipen);
            SetStatChange(battler, stat, 4);
        }
        else
        {
            BattleScriptCall(BattleScript_ConsumableBerryStatRaise);
            SetStatChange(battler, stat, 2);
        }
        effect = ITEM_STATS_CHANGE;
    }

    return effect;
}

static enum ItemEffect TrySetMicleBerry(enum BattlerId battler, enum Item itemId)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;

    if (HasEnoughHpToEatBerry(battler, GetBattlerAbility(battler), 4, itemId))
    {
        gBattleStruct->battlerState[battler].usedMicleBerry = TRUE;
        BattleScriptCall(BattleScript_MicleBerryActivate);
        effect = ITEM_EFFECT_OTHER;
    }
    return effect;
}

enum ItemEffect ItemBattleEffects(enum BattlerId itemBattler, enum BattlerId battler, enum HoldEffect holdEffect, ActivationTiming timing)
{
    enum ItemEffect effect = ITEM_NO_EFFECT;
    enum Item item;

    if (holdEffect == HOLD_EFFECT_WEAKNESS_POLICY && timing == IsOnTargetHitActivation)
    {
        enum PolishedWeaknessPolicyPhase phase = GetPolishedWeaknessPolicyPhase(gCurrentMove);
        // Ordinary source scripts already tried at effectiveness text, even
        // when caps prevented consumption. Only the seven ordinary multihit
        // moves place that command after hit effects; absent scripts never do.
        if (phase != POLISHED_POLICY_NATIVE && phase != POLISHED_POLICY_AFTER_HIT_EFFECTS)
            return ITEM_NO_EFFECT;
    }

    // HitConfusion/TakeOpponentDamage use SubtractHPFromUser_SkipItems.
    // Generic move-end passes must not turn that into an immediate HP/stat
    // berry trigger, including after a later action that deals no HP damage.
    // Real damage, residual damage and explicit source healing sweeps release
    // the deferral; forced berry effects retain their own activation rules.
    if (UsesPolishedLifecycle() && (gBattleStruct->polishedSelfHitBattlers & (1u << itemBattler))
     && timing == IsOnHpThresholdActivation && !gBattleScripting.overrideBerryRequirements)
        return ITEM_NO_EFFECT;

    if (UsesPolishedLifecycle() && (gBattleStruct->polishedPendingItemLoss & (1u << itemBattler))
     && timing == IsOnHpThresholdActivation && !gBattleScripting.overrideBerryRequirements)
        return ITEM_NO_EFFECT;

    // CheckEnigmaBerry runs immediately after attacking damage. It is not a
    // general post-hit item, and losing Unnerve later cannot retry that hit.
    if (UsesPolishedHitCheckpoint(gCurrentMove) && holdEffect == HOLD_EFFECT_ENIGMA_BERRY
     && timing == IsOnTargetHitActivation)
        return ITEM_NO_EFFECT;

    // Source CheckMirrorHerb drains pending changes even without a held herb.
    // Resolve the current item and caps here, after item transfers and drops.
    if (UsesPolishedLifecycle() && timing == IsMirrorHerbActivation)
    {
        if (!PolishedPrepareMirrorHerb(itemBattler))
            return ITEM_NO_EFFECT;
        BattleScriptCall(BattleScript_MirrorHerbCopyStatChange);
        gLastUsedItem = gBattleMons[itemBattler].item;
        gBattleScripting.battler = gPotentialItemEffectBattler = itemBattler;
        return ITEM_STATS_CHANGE;
    }

    // Source genes activate before action selection, including ones acquired
    // mid-turn. Entry, Trick/Bestow and generic forced-item checks must wait.
    if (UsesPolishedLifecycle() && holdEffect == HOLD_EFFECT_BERSERK_GENE
     && timing != IsPolishedPreActionActivation)
        return ITEM_NO_EFFECT;

    // Source Leppa runs in its end-turn phase (or when Unnerve leaves), not
    // at move end. HP/status berries still trigger from SubtractHP/PostStatus;
    // HandleHealingItems is an additional final sweep, not their only trigger.
    if (UsesPolishedLifecycle() && holdEffect == HOLD_EFFECT_RESTORE_PP
     && timing == IsSprayLeppaBlunderActivation)
        return effect;

    if (timing == IsOnBerryActivation || timing == IsOnFlingActivation)
        item = gLastUsedItem;
    else
        item = gBattleMons[itemBattler].item;

    if (holdEffect == HOLD_EFFECT_NONE
     || !timing(holdEffect)
     || IsUnnerveBlocked(itemBattler, item))
        return effect;

    if (!IsBattlerAlive(itemBattler)
     && !(UsesPolishedHitCheckpoint(gCurrentMove) && timing == IsPolishedDamageEnigmaActivation
          && holdEffect == HOLD_EFFECT_ENIGMA_BERRY)
     && holdEffect != HOLD_EFFECT_ROWAP_BERRY // Hacky workaround for them right now
     && holdEffect != HOLD_EFFECT_JABOCA_BERRY
     && holdEffect != HOLD_EFFECT_ROCKY_HELMET
     && holdEffect != HOLD_EFFECT_AIR_BALLOON)
        return effect;

    switch (holdEffect)
    {
    case HOLD_EFFECT_DOUBLE_PRIZE:
        effect = TryDoublePrize(itemBattler);
        break;
    case HOLD_EFFECT_ROOM_SERVICE:
        effect = TryRoomService(itemBattler);
        break;
    case HOLD_EFFECT_TERRAIN_SEED:
        effect = TryTerrainSeeds(itemBattler, item);
        break;
    case HOLD_EFFECT_BERSERK_GENE:
        effect = TryBerserkGene(itemBattler);
        break;
    case HOLD_EFFECT_BOOSTER_ENERGY:
        effect = TryBoosterEnergy(itemBattler, GetBattlerAbility(itemBattler));
        break;
    case HOLD_EFFECT_WHITE_HERB:
        effect = RestoreWhiteHerbStats(itemBattler);
        break;
    case HOLD_EFFECT_MIRROR_HERB:
        effect = TryConsumeMirrorHerb(itemBattler);
        break;
    case HOLD_EFFECT_FLINCH: // Kings Rock
        effect = TryKingsRock(itemBattler, battler, item);
        break;
    case HOLD_EFFECT_AIR_BALLOON:
        effect = TryAirBalloon(itemBattler, timing);
        break;
    case HOLD_EFFECT_ROCKY_HELMET:
        effect = TryRockyHelmet(itemBattler, battler, item);
        break;
    case HOLD_EFFECT_WEAKNESS_POLICY:
        effect = TryWeaknessPolicy(itemBattler);
        break;
    case HOLD_EFFECT_SNOWBALL:
        effect = TrySnowball(itemBattler);
        break;
    case HOLD_EFFECT_LUMINOUS_MOSS:
        effect = TryLuminousMoss(itemBattler);
        break;
    case HOLD_EFFECT_CELL_BATTERY:
        effect = TryCellBattery(itemBattler);
        break;
    case HOLD_EFFECT_ABSORB_BULB:
        effect = TryAbsorbBulb(itemBattler);
        break;
    case HOLD_EFFECT_JABOCA_BERRY:
        effect = TryJabocaBerry(itemBattler, battler, item);
        break;
    case HOLD_EFFECT_ROWAP_BERRY:
        effect = TryRowapBerry(itemBattler, battler, item);
        break;
    case HOLD_EFFECT_ENIGMA_BERRY: // consume and heal if hit by super effective move
        effect = TrySetEnigmaBerry(itemBattler, battler);
        break;
    case HOLD_EFFECT_BLUNDER_POLICY:
        effect = TryBlunderPolicy(itemBattler);
        break;
    case HOLD_EFFECT_MENTAL_HERB:
        effect = TryMentalHerb(itemBattler, timing);
        break;
    case HOLD_EFFECT_THROAT_SPRAY:
        effect = TryThroatSpray(itemBattler);
        break;
    case HOLD_EFFECT_KEE_BERRY:  // consume and boost defense if used physical move
        effect = DamagedStatBoostBerryEffect(itemBattler, battler, STAT_DEF, DAMAGE_CATEGORY_PHYSICAL);
        break;
    case HOLD_EFFECT_MARANGA_BERRY:  // consume and boost sp. defense if used special move
        effect = DamagedStatBoostBerryEffect(itemBattler, battler, STAT_SPDEF, DAMAGE_CATEGORY_SPECIAL);
        break;
    case HOLD_EFFECT_SHELL_BELL:
        effect = TryShellBell(itemBattler);
        break;
    case HOLD_EFFECT_LIFE_ORB:
        effect = TryLifeOrb(itemBattler);
        break;
    case HOLD_EFFECT_STICKY_BARB:
        if (timing == IsOnTargetHitActivation)
            effect = TryStickyBarbOnTargetHit(itemBattler, battler, item);
        else
            effect = TryStickyBarbOnEndTurn(itemBattler, item);
        break;
    case HOLD_EFFECT_TOXIC_ORB:
        effect = TryToxicOrb(itemBattler);
        break;
    case HOLD_EFFECT_FLAME_ORB:
        effect = TryFlameOrb(itemBattler);
        break;
    case HOLD_EFFECT_LEFTOVERS:
        effect = TryLeftovers(itemBattler, holdEffect);
        break;
    case HOLD_EFFECT_BLACK_SLUDGE:
        if (IS_BATTLER_OF_TYPE(itemBattler, TYPE_POISON))
            effect = TryLeftovers(itemBattler, holdEffect);
        else
            effect = TryBlackSludgeDamage(itemBattler, holdEffect);
        break;
    case HOLD_EFFECT_CURE_PAR: // Cheri Berry
        effect = TryCureParalysis(itemBattler);
        break;
    case HOLD_EFFECT_CURE_PSN: // Pecha Berry
        effect = TryCurePoison(itemBattler);
        break;
    case HOLD_EFFECT_CURE_BRN: // Rawst Berry
        effect = TryCureBurn(itemBattler);
        break;
    case HOLD_EFFECT_CURE_FRZ: // Aspear Berry
        effect = TryCureFreezeOrFrostbite(itemBattler);
        break;
    case HOLD_EFFECT_CURE_SLP: // Chesto Berry
        effect = TryCureSleep(itemBattler);
        break;
    case HOLD_EFFECT_CURE_CONFUSION: // Persim Berry
        effect = TryCureConfusion(itemBattler);
        break;
    case HOLD_EFFECT_CURE_STATUS: // Lum Berry
        effect = TryCureAnyStatus(itemBattler);
        break;
    case HOLD_EFFECT_RESTORE_HP: // Oran / Sitrus Berry / Berry Juice
        effect = ItemHealHp(itemBattler, item, FIXED_HEAL_AMOUNT);
        break;
    case HOLD_EFFECT_RESTORE_PCT_HP: // Sitrus Berry
        effect = ItemHealHp(itemBattler, item, PERCENT_HEAL_AMOUNT);
        break;
    case HOLD_EFFECT_RESTORE_PP: // Leppa Berry
        effect = ItemRestorePp(itemBattler, item);
        break;
    case HOLD_EFFECT_CONFUSE_FLAVOR: // Figy Berry etc.
        effect = HealConfuseBerry(itemBattler, item, GetItemSecondaryId(item));
        break;
    case HOLD_EFFECT_ATTACK_UP: // Liechi Berry
        effect = StatRaiseBerry(itemBattler, item, STAT_ATK);
        break;
    case HOLD_EFFECT_DEFENSE_UP: // Ganlon Berry
        effect = StatRaiseBerry(itemBattler, item, STAT_DEF);
        break;
    case HOLD_EFFECT_SPEED_UP: // Salac Berry
        effect = StatRaiseBerry(itemBattler, item, STAT_SPEED);
        break;
    case HOLD_EFFECT_SP_ATTACK_UP: // Petaya Berry
        effect = StatRaiseBerry(itemBattler, item, STAT_SPATK);
        break;
    case HOLD_EFFECT_SP_DEFENSE_UP: // Apicot Berry
        effect = StatRaiseBerry(itemBattler, item, STAT_SPDEF);
        break;
    case HOLD_EFFECT_CRITICAL_UP: // Lansat Berry
        effect = CriticalHitRatioUp(itemBattler, item);
        break;
    case HOLD_EFFECT_RANDOM_STAT_UP: // Starf Berry
        effect = RandomStatRaiseBerry(itemBattler, item);
        break;
    case HOLD_EFFECT_MICLE_BERRY:
        effect = TrySetMicleBerry(itemBattler, item);
        break;
    default:
        break;
    }

    if (effect == ITEM_STATUS_CHANGE)
    {
        BtlController_EmitSetMonData(itemBattler, B_COMM_TO_CONTROLLER, REQUEST_STATUS_BATTLE, 0, 4, &gBattleMons[itemBattler].status1);
        MarkBattlerForControllerExec(itemBattler);
    }

    if (effect)
    {
        gLastUsedItem = item;
        gBattleScripting.battler = gPotentialItemEffectBattler = itemBattler;
        if (gItemsInfo[item].pocket == POCKET_BERRIES)
            GetBattlerPartyState(itemBattler)->ateBerry = TRUE;
    }

    return effect;
}
