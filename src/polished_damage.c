#include "global.h"
#include "battle.h"
#include "battle_util.h"
#include "battle_ai_util.h"
#include "item.h"
#include "pokemon.h"
#include "polished_damage.h"
#include "data/polished_damage.h"

static bool32 HasNativeDamageItem(enum HoldEffect effect)
{
    switch (effect)
    {
    case HOLD_EFFECT_DEEP_SEA_TOOTH:
    case HOLD_EFFECT_DEEP_SEA_SCALE:
    case HOLD_EFFECT_SOUL_DEW:
    case HOLD_EFFECT_LUSTROUS_ORB:
    case HOLD_EFFECT_ADAMANT_ORB:
    case HOLD_EFFECT_GRISEOUS_ORB:
    case HOLD_EFFECT_GEMS:
    case HOLD_EFFECT_RESIST_BERRY:
    case HOLD_EFFECT_OGERPON_MASK:
        return TRUE;
    default:
        return FALSE;
    }
}

bool32 UsesPolishedDamageRules(struct DamageContext *ctx)
{
    return UsesPolishedBattleRules(ctx->battlerAtk, ctx->battlerDef)
        && !ctx->isSelfInflicted && !ctx->fixedBasePower
        && ctx->move < MOVES_COUNT && sPolishedDamageMoves[ctx->move]
        && !gSpecialStatuses[ctx->battlerAtk].attackerInParty
        && sPolishedDamageAbilities[ctx->abilities[ctx->battlerAtk]]
        && sPolishedDamageAbilities[ctx->abilities[ctx->battlerDef]]
        && !HasNativeDamageItem(ctx->holdEffects[ctx->battlerAtk])
        && !HasNativeDamageItem(ctx->holdEffects[ctx->battlerDef])
        && !ctx->terrain
        && !(gSideStatuses[GetBattlerSide(ctx->battlerDef)] & SIDE_STATUS_AURORA_VEIL)
        && !(gFieldStatuses & (STATUS_FIELD_WONDER_ROOM | STATUS_FIELD_MUDSPORT | STATUS_FIELD_WATERSPORT))
        && !gProtectStructs[ctx->battlerAtk].helpingHand
        && ctx->chosenMove != MOVE_ME_FIRST
        && !gBattleMons[ctx->battlerDef].volatiles.glaiveRush
        && gBattleMons[ctx->battlerDef].volatiles.semiInvulnerable != STATE_UNDERWATER
        && !gBattleMons[ctx->battlerAtk].volatiles.chargeTimer;
}

static enum Species OriginalSpecies(enum BattlerId battler)
{
    const struct BattlePokemon *mon = &gBattleMons[battler];
    return mon->volatiles.transformed && mon->volatiles.transformedMonSpecies != SPECIES_NONE
        ? mon->volatiles.transformedMonSpecies : mon->species;
}

static bool32 HasEvolution(enum Species species)
{
    const struct Evolution *evos = GetSpeciesEvolutions(species);
    if (evos != NULL)
        for (u32 i = 0; evos[i].method != EVOLUTIONS_END; i++)
            if (evos[i].method && SanitizeSpeciesId(evos[i].targetSpecies) != SPECIES_NONE)
                return TRUE;
    return FALSE;
}

static bool32 MovesAfterTarget(struct DamageContext *ctx)
{
    if (ctx->aiCalc)
    {
        u8 order[MAX_BATTLERS_COUNT] = {0};
        AI_SetBattlerTurnOrder(order);
        for (u32 i = 0; i < gBattlersCount; i++)
        {
            if (order[i] == ctx->battlerAtk) return FALSE;
            if (order[i] == ctx->battlerDef) return TRUE;
        }
        return FALSE;
    }
    return HasBattlerActedThisTurn(ctx->battlerDef);
}

static u32 OffensiveAbility(struct DamageContext *ctx, u32 value, u32 power)
{
    enum BattlerId a = ctx->battlerAtk, d = ctx->battlerDef;
    enum Ability ability = ctx->abilities[a];
    bool32 physical = IsBattleMovePhysical(ctx->move);
    u32 numerator = 1, denominator = 1;
    enum Type type = ctx->moveType;
    u32 flags = sPolishedDamageFlags[ctx->move];
    switch (ability)
    {
    case ABILITY_TECHNICIAN:
        if (power <= 60) { numerator = 3; denominator = 2; }
        break;
    case ABILITY_HUGE_POWER:
        if (physical) numerator = 2;
        break;
    case ABILITY_HUSTLE:
    case ABILITY_GORILLA_TACTICS:
        if (physical) { numerator = 3; denominator = 2; }
        break;
    case ABILITY_OVERGROW:
    case ABILITY_BLAZE:
    case ABILITY_TORRENT:
    case ABILITY_SWARM:
        if (gBattleMons[a].hp <= max(1, gBattleMons[a].maxHP / 3)
         && ((ability == ABILITY_OVERGROW && type == TYPE_GRASS)
          || (ability == ABILITY_BLAZE && type == TYPE_FIRE)
          || (ability == ABILITY_TORRENT && type == TYPE_WATER)
          || (ability == ABILITY_SWARM && type == TYPE_BUG)))
            { numerator = 3; denominator = 2; }
        break;
    case ABILITY_RIVALRY:
        if (GetBattlerGender(a) != MON_GENDERLESS && GetBattlerGender(d) != MON_GENDERLESS)
            { numerator = GetBattlerGender(a) == GetBattlerGender(d) ? 5 : 3; denominator = 4; }
        break;
    case ABILITY_SHEER_FORCE:
        if (flags & 8) { numerator = 13; denominator = 10; }
        break;
    case ABILITY_ANALYTIC:
        if (MovesAfterTarget(ctx))
            { numerator = 13; denominator = 10; }
        break;
    case ABILITY_SOLAR_POWER:
        if (!physical && (ctx->weather & B_WEATHER_SUN) && ctx->holdEffects[a] != HOLD_EFFECT_UTILITY_UMBRELLA)
            { numerator = 3; denominator = 2; }
        break;
    case ABILITY_IRON_FIST:
        if (flags & 1) { numerator = 6; denominator = 5; }
        break;
    case ABILITY_TOUGH_CLAWS:
        if (IsMoveMakingContact(a, d, ability, ctx->holdEffects[a], ctx->move))
            { numerator = 13; denominator = 10; }
        break;
    case ABILITY_MEGA_LAUNCHER:
        if (flags & 2) { numerator = 3; denominator = 2; }
        break;
    case ABILITY_SHARPNESS:
        if (flags & 4) { numerator = 3; denominator = 2; }
        break;
    case ABILITY_SAND_FORCE:
        if ((ctx->weather & B_WEATHER_SANDSTORM) && (type == TYPE_ROCK || type == TYPE_GROUND || type == TYPE_STEEL))
            { numerator = 13; denominator = 10; }
        break;
    case ABILITY_RECKLESS:
        if (flags & 16)
            { numerator = 6; denominator = 5; }
        break;
    case ABILITY_GUTS:
        if (physical && gBattleMons[a].status1) { numerator = 3; denominator = 2; }
        break;
    case ABILITY_PIXILATE:
    case ABILITY_GALVANIZE:
        if (gBattleStruct->battlerState[a].ateBoost) { numerator = 6; denominator = 5; }
        break;
    case ABILITY_STEELY_SPIRIT:
        if (type == TYPE_STEEL) { numerator = 3; denominator = 2; }
        break;
    default:
        break;
    }
    return numerator == 1 && denominator == 1 ? value : PolishedDamageScale(value, numerator, denominator);
}

static u32 DefensiveAbility(struct DamageContext *ctx, u32 value, bool32 physicalDefense)
{
    enum BattlerId d = ctx->battlerDef;
    switch (ctx->abilities[d])
    {
    case ABILITY_MULTISCALE:
        if (gBattleMons[d].hp == gBattleMons[d].maxHP) return PolishedDamageScale(value, 1, 2);
        break;
    case ABILITY_MARVEL_SCALE:
        if (physicalDefense && gBattleMons[d].status1) return PolishedDamageScale(value, 2, 3);
        break;
    case ABILITY_THICK_FAT:
        if (ctx->moveType == TYPE_FIRE || ctx->moveType == TYPE_ICE) return PolishedDamageScale(value, 1, 2);
        break;
    case ABILITY_DRY_SKIN:
        if (ctx->moveType == TYPE_FIRE) return PolishedDamageScale(value, 5, 4);
        break;
    case ABILITY_FUR_COAT:
        if (physicalDefense) return PolishedDamageScale(value, 1, 2);
        break;
    default:
        break;
    }
    return value;
}

u32 PolishedDamageBeforeRoll(struct DamageContext *ctx, u32 power)
{
    enum BattlerId a = ctx->battlerAtk, d = ctx->battlerDef;
    const struct BattlePokemon *user = &gBattleMons[a], *target = &gBattleMons[d];
    enum HoldEffect item = ctx->holdEffects[a], targetItem = ctx->holdEffects[d];
    bool32 physical = IsBattleMovePhysical(ctx->move);
    bool32 physicalDefense = physical || ctx->move == MOVE_PSYSTRIKE;
    u32 attack = physical ? user->attack : user->spAttack;
    u32 defense = physicalDefense ? target->defense : target->spDefense;
    u32 attackStage = user->statStages[physical ? STAT_ATK : STAT_SPATK];
    u32 defenseStage = target->statStages[physicalDefense ? STAT_DEF : STAT_SPDEF];
    u32 weather = ctx->weather;
    u32 damage, matchup;

    // These commands modify final damage, not base power.
    switch (ctx->move)
    {
    case MOVE_ACROBATICS: case MOVE_AVALANCHE: case MOVE_HEX: case MOVE_VENOSHOCK:
    case MOVE_PURSUIT: case MOVE_ROLLOUT:
        power = GetMovePower(ctx->move);
        break;
    default: break;
    }
    gBattleMovePower = power;
    if (power == 0)
        return 1;

    if ((physicalDefense && (weather & B_WEATHER_HAIL) && IS_BATTLER_OF_TYPE(d, TYPE_ICE))
     || (!physicalDefense && (weather & B_WEATHER_SANDSTORM) && IS_BATTLER_OF_TYPE(d, TYPE_ROCK)))
        defense = (u16)(defense + defense / 2);
    if ((physicalDefense && targetItem == HOLD_EFFECT_METAL_POWDER && OriginalSpecies(d) == SPECIES_DITTO)
     || (targetItem == HOLD_EFFECT_EVIOLITE && HasEvolution(OriginalSpecies(d))))
        defense = (u16)(defense + defense / 2);
    if (!ctx->isCrit && ctx->abilities[a] != ABILITY_INFILTRATOR && ctx->move != MOVE_BRICK_BREAK
     && (gSideStatuses[GetBattlerSide(d)] & (physical ? SIDE_STATUS_REFLECT : SIDE_STATUS_LIGHTSCREEN)))
        defense = (u16)(defense * 2);
    if ((item == HOLD_EFFECT_LIGHT_BALL && GET_BASE_SPECIES_ID(user->species) == SPECIES_PIKACHU)
     || (physical && item == HOLD_EFFECT_THICK_CLUB && (user->species == SPECIES_CUBONE || GET_BASE_SPECIES_ID(user->species) == SPECIES_MAROWAK)))
        attack = (u16)(attack * 2);
    while (attack > 255 || defense > 255)
    {
        attack = max(1, attack / 2);
        defense = max(1, defense / 2);
    }
    defense = max(1, defense);
    damage = (2 * user->level / 5 + 2) * power * attack;
    if (ctx->abilities[d] != ABILITY_UNAWARE && (!ctx->isCrit || attackStage >= DEFAULT_STAT_STAGE))
        damage = PolishedDamageStage(damage, attackStage);
    if (ctx->abilities[a] != ABILITY_UNAWARE && (!ctx->isCrit || defenseStage < DEFAULT_STAT_STAGE))
        damage = PolishedDamageStage(damage, 12 - defenseStage);
    damage = OffensiveAbility(ctx, damage, power);
    damage = DefensiveAbility(ctx, damage, physicalDefense);
    if (physical && (user->status1 & STATUS1_BURN) && ctx->abilities[a] != ABILITY_GUTS)
        damage = PolishedDamageScale(damage, 1, 2);
    if (user->volatiles.flashFireBoosted && ctx->abilities[d] != ABILITY_NEUTRALIZING_GAS && ctx->moveType == TYPE_FIRE)
        damage = PolishedDamageScale(damage, 3, 2);
    if (ctx->isCrit)
        damage = ctx->abilities[a] == ABILITY_SNIPER ? PolishedDamageScale(damage, 9, 4) : PolishedDamageScale(damage, 3, 2);
    switch (item)
    {
    case HOLD_EFFECT_TYPE_POWER:
    case HOLD_EFFECT_PLATE:
        if (ctx->moveType == GetItemSecondaryId(user->item)) damage = PolishedDamageScale(damage, 6, 5);
        break;
    case HOLD_EFFECT_MUSCLE_BAND:
        if (physical) damage = PolishedDamageScale(damage, 11, 10);
        break;
    case HOLD_EFFECT_WISE_GLASSES:
        if (!physical) damage = PolishedDamageScale(damage, 11, 10);
        break;
    case HOLD_EFFECT_CHOICE_BAND:
        if (physical) damage = PolishedDamageScale(damage, 3, 2);
        break;
    case HOLD_EFFECT_CHOICE_SPECS:
        if (!physical) damage = PolishedDamageScale(damage, 3, 2);
        break;
    case HOLD_EFFECT_LIFE_ORB: damage = PolishedDamageScale(damage, 13, 10); break;
    case HOLD_EFFECT_METRONOME: damage = PolishedDamageScale(damage, 5 + min(5, user->volatiles.metronomeItemCounter), 5); break;
    case HOLD_EFFECT_PUNCHING_GLOVE:
        if (sPolishedDamageFlags[ctx->move] & 1) damage = PolishedDamageScale(damage, 11, 10);
        break;
    default: break;
    }
    if (targetItem == HOLD_EFFECT_ASSAULT_VEST && !physicalDefense)
        damage = PolishedDamageScale(damage, 2, 3);
    damage = damage / defense / 50;
    damage = damage >= 0xFF00 ? 0xFF00 : damage + 2;
    if (ctx->move == MOVE_STRUGGLE)
        return damage;

    // The source combines type effectiveness, STAB and weather into one byte.
    matchup = ctx->typeEffectivenessModifier * 16 / UQ_4_12(1.0);
    if (!matchup) return 0;
    if (IS_BATTLER_OF_TYPE(a, ctx->moveType))
        matchup = ctx->abilities[a] == ABILITY_ADAPTABILITY ? matchup * 2 : matchup + matchup / 2;
    if (targetItem != HOLD_EFFECT_UTILITY_UMBRELLA)
    {
        if (((weather & B_WEATHER_SUN) && ctx->moveType == TYPE_FIRE) || ((weather & B_WEATHER_RAIN) && ctx->moveType == TYPE_WATER))
            matchup += matchup / 2;
        if (((weather & B_WEATHER_SUN) && ctx->moveType == TYPE_WATER) || ((weather & B_WEATHER_RAIN) && ctx->moveType == TYPE_FIRE))
            matchup /= 2;
    }
    damage *= matchup;
    if (gSpecialStatuses[a].parentalBondState == PARENTAL_BOND_2ND_HIT)
        damage = PolishedDamageScale(damage, 1, 4);
    if (ctx->abilities[a] == ABILITY_TINTED_LENS && ctx->typeEffectivenessModifier < UQ_4_12(1.0))
        damage = PolishedDamageScale(damage, 2, 1);
    if ((ctx->abilities[d] == ABILITY_SOLID_ROCK || ctx->abilities[d] == ABILITY_FILTER) && ctx->typeEffectivenessModifier > UQ_4_12(1.0))
        damage = PolishedDamageScale(damage, 3, 4);
    if (item == HOLD_EFFECT_EXPERT_BELT && ctx->typeEffectivenessModifier > UQ_4_12(1.0))
        damage = PolishedDamageScale(damage, 6, 5);
    damage = max(1, min(65535, damage / 16));
    bool32 twice = (ctx->move == MOVE_ACROBATICS && user->item == ITEM_NONE)
        || (ctx->move == MOVE_FACADE && (user->status1 & (STATUS1_BURN | STATUS1_PSN_ANY | STATUS1_PARALYSIS)))
        || (ctx->move == MOVE_HEX && target->status1)
        || (ctx->move == MOVE_VENOSHOCK && (target->status1 & STATUS1_PSN_ANY))
        || (ctx->move == MOVE_AVALANCHE && (gProtectStructs[a].revengeDoubled & (1u << d)));
    if (twice) damage = min(65535, damage * 2);
    if (ctx->move == MOVE_ROLLOUT)
    {
        u32 n = user->volatiles.rolloutTimer + user->volatiles.defenseCurl;
        while (n--) damage = min(65535, damage * 2);
    }
    return damage;
}

u32 PolishedDamageAfterRoll(struct DamageContext *ctx, u32 damage)
{
    enum BattlerId d = ctx->battlerDef;
    bool32 twice = (ctx->move == MOVE_PURSUIT && gBattleStruct->battlerState[d].pursuitTarget)
        || (ctx->move == MOVE_GUST && gBattleMons[d].volatiles.semiInvulnerable == STATE_ON_AIR)
        || ((ctx->move == MOVE_EARTHQUAKE || ctx->move == MOVE_MAGNITUDE) && gBattleMons[d].volatiles.semiInvulnerable == STATE_UNDERGROUND)
        || ((ctx->move == MOVE_STOMP || ctx->move == MOVE_BODY_SLAM) && gBattleMons[d].volatiles.minimize);
    if (twice) damage = min(65535, damage * 2);
    if (ctx->move == MOVE_KNOCK_OFF && gBattleMons[d].item != ITEM_NONE
     && CanBattlerGetOrLoseItem(d, ctx->battlerAtk, gBattleMons[d].item))
        damage = (u16)(damage + damage / 2);
    return max(1, damage);
}
