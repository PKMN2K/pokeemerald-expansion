#include "global.h"
#if POLISHED_JOHTO_DEMO
#include "battle.h"
#include "battle_controllers.h"
#include "battle_ai_util.h"
#include "battle_util.h"
#include "item.h"
#include "pokemon.h"
#include "polished_damage.h"
#include "polished_trainers.h"
#include "random.h"

enum SmartMoveFlags
{
    SMART_ATTACK = 1, SMART_SPECIAL = 2, SMART_USEFUL = 4, SMART_ENCORE = 8, SMART_SOUND = 16, SMART_PRIORITY = 32, SMART_BULLET = 64,
};

struct SmartMove
{
    enum Type type;
    u8 flags;
};
#include "data/polished_smart_ai.h"

// Kept across turns: Roar and Mean Look read this byte after a call that only returns a score in A.
EWRAM_DATA u8 gPolishedAiSwitchScore = 0;

struct MatchupMon
{
    enum Type types[3];
    enum Ability ability;
    enum HoldEffect item;
    bool8 identified, activeFlying;
};

static struct MatchupMon ReadActiveMon(enum BattlerId battler)
{
    struct MatchupMon mon = {
        .ability = gBattleMons[battler].ability, .item = GetBattlerHoldEffect(battler),
        .identified = gBattleMons[battler].volatiles.foresight,
        .activeFlying = IS_BATTLER_OF_TYPE(battler, TYPE_FLYING),
    };
    GetBattlerTypes(battler, FALSE, mon.types);
    return mon;
}

static enum Ability ActiveAbility(const struct MatchupMon *mon, const struct MatchupMon *other)
{
    if (other->ability == ABILITY_NEUTRALIZING_GAS && mon->ability != ABILITY_NEUTRALIZING_GAS)
        return ABILITY_NONE;
    return mon->ability;
}

static enum Ability DefendingAbility(const struct MatchupMon *user, const struct MatchupMon *target)
{
    enum Ability ability = ActiveAbility(target, user);
    if (ActiveAbility(user, target) == ABILITY_MOLD_BREAKER && sSmartMoldBreakerIgnores[ability])
        return ABILITY_NONE;
    return ability;
}

static uq4_12_t ChartMatchup(enum Type type, enum Ability attacker, const struct MatchupMon *target)
{
    uq4_12_t total = UQ_4_12(1.0);
    for (u32 i = 0; i < ARRAY_COUNT(target->types); i++)
    {
        enum Type defense = target->types[i];
        if (defense == TYPE_NONE || defense == TYPE_MYSTERY || (i && defense == target->types[0])
         || (i == 2 && defense == target->types[1]))
            continue;
        uq4_12_t mod = GetTypeModifier(type, defense);
        if (!mod && (target->item == HOLD_EFFECT_RING_TARGET || (defense == TYPE_GHOST
         && (type == TYPE_NORMAL || type == TYPE_FIGHTING)
         && (target->identified || attacker == ABILITY_SCRAPPY || attacker == ABILITY_MINDS_EYE))))
            mod = UQ_4_12(1.0);
        total = uq4_12_multiply(total, mod);
    }
    return total;
}

// The source evaluates unmodified move data here, including Hidden Power's unknown type.
static uq4_12_t SmartMatchup(enum Move move, enum Type type, const struct MatchupMon *user, const struct MatchupMon *target)
{
    uq4_12_t total = ChartMatchup(type, ActiveAbility(user, target), target);
    enum Ability defender = DefendingAbility(user, target);
    if (type == TYPE_GROUND && target->item != HOLD_EFFECT_IRON_BALL
     && (defender == ABILITY_LEVITATE || target->item == HOLD_EFFECT_AIR_BALLOON
      || (target->activeFlying && target->item != HOLD_EFFECT_RING_TARGET && GetTypeModifier(TYPE_GROUND, TYPE_FLYING) == 0)))
        return 0;
    if ((defender == ABILITY_DAMP && move == MOVE_EXPLOSION)
     || (defender == ABILITY_SOUNDPROOF && (sSmartMoves[move].flags & SMART_SOUND))
     || (defender == ABILITY_BULLETPROOF && (sSmartMoves[move].flags & SMART_BULLET))
     || (type == TYPE_GRASS && defender == ABILITY_SAP_SIPPER)
     || (type == TYPE_FIRE && defender == ABILITY_FLASH_FIRE)
     || (type == TYPE_ELECTRIC && (defender == ABILITY_LIGHTNING_ROD || defender == ABILITY_MOTOR_DRIVE || defender == ABILITY_VOLT_ABSORB))
     || (type == TYPE_WATER && (defender == ABILITY_DRY_SKIN || defender == ABILITY_WATER_ABSORB)))
        return 0;
    return total;
}

// DoCheckAnyOtherAliveMons reads the HP of every other occupied party slot.
static bool32 OtherPartySlotHasHP(enum BattlerId battler)
{
    struct Pokemon *party = GetBattlerParty(battler);
    for (u32 i = 0; i < gPartiesCount[GetBattlerTrainer(battler)]; i++)
        if (i != gBattlerPartyIndexes[battler] && GetMonData(&party[i], MON_DATA_HP))
            return TRUE;
    return FALSE;
}

void PolishedReadBasicBattle(enum BattlerId user, enum BattlerId target, struct PolishedBasicAiContext *ctx)
{
    enum Move *moves = gBattleMons[user].moves;
    u32 side = GetBattlerSide(target);
    *ctx = (struct PolishedBasicAiContext){
        .userSide = gSideStatuses[GetBattlerSide(user)], .targetSide = gSideStatuses[side], .weather = gBattleWeather,
        .spikes = gSideTimers[side].spikesAmount, .toxicSpikes = gSideTimers[side].toxicSpikesAmount,
        .futureSight = gBattleStruct->futureSight[target].counter != 0,
        .userSpeed = GetBattlerTotalSpeedStat(user, GetBattlerAbility(user), GetBattlerHoldEffect(user)),
        .targetSpeed = GetBattlerTotalSpeedStat(target, GetBattlerAbility(target), GetBattlerHoldEffect(target)),
        .trickRoom = (gFieldStatuses & STATUS_FIELD_TRICK_ROOM) != 0,
        .userGhost = IS_BATTLER_OF_TYPE(user, TYPE_GHOST),
        .oppositeGender = GetMoveSlot(moves, MOVE_ATTRACT) < MAX_MON_MOVES && AreBattlersOfOppositeGender(user, target),
        .userPartyHP = (GetMoveSlot(moves, MOVE_TELEPORT) < MAX_MON_MOVES || GetMoveSlot(moves, MOVE_BATON_PASS) < MAX_MON_MOVES)
                    && OtherPartySlotHasHP(user),
        .targetPartyHP = GetMoveSlot(moves, MOVE_ROAR) < MAX_MON_MOVES && OtherPartySlotHasHP(target),
    };
}

void PolishedScoreTypeMoves(enum BattlerId user, enum BattlerId target, u32 limitations, s32 *scores)
{
    static const bool8 scoredTypes[NUMBER_OF_MON_TYPES] = {
        [TYPE_ICE] = TRUE, [TYPE_DARK] = TRUE, [TYPE_FIGHTING] = TRUE, [TYPE_GHOST] = TRUE, [TYPE_PSYCHIC] = TRUE,
        [TYPE_FLYING] = TRUE, [TYPE_BUG] = TRUE, [TYPE_STEEL] = TRUE, [TYPE_ROCK] = TRUE, [TYPE_POISON] = TRUE,
        [TYPE_MYSTERY] = TRUE, [TYPE_DRAGON] = TRUE, [TYPE_FAIRY] = TRUE,
    };
    const enum Move *moves = gBattleMons[user].moves;
    struct MatchupMon attacker = ReadActiveMon(user), defender = ReadActiveMon(target);
    enum Ability userAbility = ActiveAbility(&attacker, &defender);
    enum Ability ability = DefendingAbility(&attacker, &defender);
    // AIGetEnemyMove updates the candidate's data. BATTLE_VARS_MOVE still holds the prior selection.
    u32 selectedFlags = sSmartMoves[gBattleStruct->polishedChoiceMove[user]].flags;
    bool32 powderImmune = ability == ABILITY_OVERCOAT || defender.item == HOLD_EFFECT_SAFETY_GOGGLES
                       || IS_BATTLER_OF_TYPE(target, TYPE_GRASS);
    bool32 previousMoveImmune = (ability == ABILITY_SOUNDPROOF && (selectedFlags & SMART_SOUND))
                             || (ability == ABILITY_BULLETPROOF && (selectedFlags & SMART_BULLET));
    for (u32 i = 0; i < MAX_MON_MOVES; i++)
    {
        const struct SmartMove *move = &sSmartMoves[moves[i]];
        enum Type type = move->type;
        bool32 damaging = (move->flags & SMART_ATTACK) != 0;
        bool32 scored = scoredTypes[type] || (type == TYPE_NORMAL && damaging) || (type == TYPE_GRASS && !damaging);
        if ((limitations & (1u << i)) || !scored)
            continue;
        uq4_12_t matchup = ChartMatchup(type, userAbility, &defender);
        if (!matchup || previousMoveImmune || (powderImmune && IsPowderMove(moves[i]))
         || (ability == ABILITY_DAMP && moves[i] == MOVE_EXPLOSION))
            scores[i] -= 10;
        else if (matchup > UQ_4_12(1.0))
            scores[i] += damaging;
        else if (matchup < UQ_4_12(1.0))
            for (u32 j = 0; j < MAX_MON_MOVES; j++)
                if (!(limitations & (1u << j)) && sSmartMoves[moves[j]].type != type && (sSmartMoves[moves[j]].flags & SMART_ATTACK))
                {
                    scores[i]--;
                    break;
                }
    }
}

static s32 BestAttackingMatchup(const enum Move *moves, bool32 inferStab, const struct MatchupMon *user,
                                const struct MatchupMon *target)
{
    bool32 hasAttack = FALSE;
    uq4_12_t best = 0;
    u32 i;
    for (i = 0; i < MAX_MON_MOVES && moves[i] != MOVE_NONE; i++)
    {
        enum Move move = moves[i];
        if (!(sSmartMoves[move].flags & SMART_ATTACK))
            continue;
        hasAttack = TRUE;
        uq4_12_t value = SmartMatchup(move, sSmartMoves[move].type, user, target);
        best = max(best, value);
    }
    if (inferStab && i < MAX_MON_MOVES)
    {
        hasAttack = TRUE;
        for (u32 t = 0; t < 2; t++)
        {
            uq4_12_t value = SmartMatchup(MOVE_STRENGTH, user->types[t], user, target);
            best = max(best, value);
        }
    }
    if (!hasAttack || best == UQ_4_12(1.0)) return 0;
    if (best > UQ_4_12(1.0)) return 1;
    return best ? -1 : -2;
}

u8 PolishedBestReserveScore(enum BattlerId user, enum BattlerId target)
{
    struct MatchupMon player = ReadActiveMon(target);
    struct MatchupMon reserve = ReadActiveMon(user);
    struct Pokemon *party = GetBattlerParty(user);
    u8 best = 0;
    for (u32 i = 0; i < gPartiesCount[GetBattlerTrainer(user)]; i++)
    {
        if (i == gBattlerPartyIndexes[user] || !GetMonData(&party[i], MON_DATA_HP) || GetMonData(&party[i], MON_DATA_IS_EGG))
            continue;
        enum Species species = GetMonData(&party[i], MON_DATA_SPECIES);
        enum Move moves[MAX_MON_MOVES];
        for (u32 m = 0; m < MAX_MON_MOVES; m++)
            moves[m] = GetMonData(&party[i], MON_DATA_MOVE1 + m);
        reserve.types[0] = GetSpeciesType(species, 0);
        reserve.types[1] = GetSpeciesType(species, 1);
        reserve.types[2] = TYPE_MYSTERY;
        reserve.ability = GetMonAbility(&party[i]);
        reserve.item = GetItemHoldEffect(GetMonData(&party[i], MON_DATA_HELD_ITEM));
        s32 score = 10 + BestAttackingMatchup(moves, FALSE, &reserve, &player)
                       - BestAttackingMatchup(gPolishedPlayerUsedMoves, TRUE, &player, &reserve);
        best = max(best, score);
    }
    return best;
}

bool32 PolishedSmartCanSwitch(enum BattlerId battler)
{
    return GetBattlerHoldEffect(battler) == HOLD_EFFECT_SHED_SHELL || CanPolishedRunAwayEscape(battler)
        || IS_BATTLER_OF_TYPE(battler, TYPE_GHOST)
        || (!IsAbilityPreventingEscape(battler) && !gBattleMons[battler].volatiles.wrapTurns
         && !gBattleMons[battler].volatiles.escapePrevention);
}

static u16 PriorityDamage(enum Move move, enum BattlerId a, enum BattlerId d,
                          const struct MatchupMon *user, const struct MatchupMon *target)
{
    struct DamageContext ctx = {.move = move, .baseMove = move, .chosenMove = move, .battlerAtk = a,
                                .battlerDef = d, .moveType = sSmartMoves[move].type, .weather = GetWeather(), .aiCalc = TRUE};
    ctx.abilities[a] = ActiveAbility(user, target);
    ctx.abilities[d] = DefendingAbility(user, target);
    ctx.holdEffects[a] = user->item;
    ctx.holdEffects[d] = target->item;
    bool32 ate = ctx.moveType == TYPE_NORMAL && (ctx.abilities[a] == ABILITY_PIXILATE || ctx.abilities[a] == ABILITY_GALVANIZE);
    if (ate)
        ctx.moveType = ctx.abilities[a] == ABILITY_PIXILATE ? TYPE_FAIRY : TYPE_ELECTRIC;
    ctx.typeEffectivenessModifier = SmartMatchup(move, ctx.moveType, user, target);
    if (!ctx.typeEffectivenessModifier)
        return 0;
    // AIDamageCalc omits critical and random damage rolls and includes Parental Bond's second hit.
    return PolishedPriorityDamageBeforeRoll(&ctx, GetMovePower(move), ate);
}

void PolishedReadSmartBattle(enum BattlerId user, enum BattlerId target, struct PolishedSmartAiContext *context)
{
    struct MatchupMon enemy = ReadActiveMon(user);
    struct MatchupMon player = ReadActiveMon(target);
    context->userGhost = IS_BATTLER_OF_TYPE(user, TYPE_GHOST);
    context->userReserve = CountUsablePartyMons(user) != 0;
    context->targetReserve = CountUsablePartyMons(target) != 0;
    context->targetShedShell = player.item == HOLD_EFFECT_SHED_SHELL;
    context->targetTrapped = gBattleMons[target].volatiles.wrapTurns || gBattleMons[target].volatiles.escapePrevention
                          || IsTrappedByPolishedAbility(target);
    enum Move locked = gLockedMoves[target];
    context->targetCharging = gBattleMons[target].volatiles.multipleTurns
                           && (locked == MOVE_SOLAR_BEAM || locked == MOVE_FLY || locked == MOVE_DIG);
    context->targetLastMove = gLastMoves[target] < MOVES_COUNT ? gLastMoves[target] : MOVE_NONE;
    if (context->targetLastMove != MOVE_NONE)
        context->targetMatchup = SmartMatchup(context->targetLastMove, sSmartMoves[context->targetLastMove].type, &player, &enemy);
    context->switchScore = gPolishedAiSwitchScore;
    if (context->userReserve)
        context->bestReserveScore = PolishedBestReserveScore(user, target);
    for (u32 i = 0; i < MAX_MON_MOVES; i++)
    {
        enum Move move = gBattleMons[user].moves[i];
        if (sSmartMoves[move].flags & SMART_PRIORITY)
            context->priorityDamage[i] = PriorityDamage(move, user, target, &enemy, &player);
    }
}

static s32 CounterScore(enum Move move, enum Move selected)
{
    u32 category = sSmartMoves[move].flags & SMART_SPECIAL;
    u32 count = 0;
    for (u32 i = 0; i < MAX_MON_MOVES; i++)
    {
        u32 flags = sSmartMoves[gPolishedPlayerUsedMoves[i]].flags;
        if ((flags & (SMART_ATTACK | SMART_SPECIAL)) == (SMART_ATTACK | category))
            count++;
    }
    if (!count) return -1;
    if (count >= 3 || (sSmartMoves[selected].flags & (SMART_ATTACK | SMART_SPECIAL)) == (SMART_ATTACK | category))
        return (Random() & 255) >= 100;
    return 0;
}

static s32 ProtectScore(const struct BattlePokemon *user, const struct BattlePokemon *target, bool32 charged)
{
    bool32 repeated = user->volatiles.consecutiveMoveUses != 0;
    if (!repeated && !target->volatiles.lockOn && (charged || (target->volatiles.rolloutTimer
     && ((target->status1 & STATUS1_TOXIC_POISON) || target->volatiles.leechSeed
      || target->volatiles.cursed || target->volatiles.rolloutTimer >= 3))))
        return (Random() & 255) >= 50;
    return -(s32)repeated - ((Random() & 255) >= 20 ? 2 : 0);
}

s32 PolishedScoreReactiveMove(const struct BattlePokemon *user, const struct BattlePokemon *target, u32 slot,
                             struct PolishedSmartAiContext *ctx)
{
    enum Move move = user->moves[slot];
    if (sSmartMoves[move].flags & SMART_PRIORITY)
    {
        if (PolishedSmartUserIsFaster(ctx)) return 0;
        if (target->volatiles.semiInvulnerable == STATE_ON_AIR || target->volatiles.semiInvulnerable == STATE_UNDERGROUND)
            return -10;
        return ctx->priorityDamage[slot] > target->hp ? 3 : 0;
    }
    switch (move)
    {
    case MOVE_COUNTER:
    case MOVE_MIRROR_COAT:
        return CounterScore(move, ctx->targetSelectedMove);
    case MOVE_ENCORE:
        if (!PolishedSmartUserIsFaster(ctx)) return -3;
        if (!ctx->targetLastMove) return -10;
        if ((sSmartMoves[ctx->targetLastMove].flags & SMART_ATTACK) && ctx->targetMatchup < UQ_4_12(1.0))
        {
            if (ctx->targetMatchup != 0) return 0;
        }
        else if (!(sSmartMoves[ctx->targetSelectedMove].flags & SMART_ENCORE)) return -3;
        return (Random() & 255) >= 70 ? 2 : 0;
    case MOVE_DISABLE:
        if (PolishedSmartUserIsFaster(ctx) && (sSmartMoves[ctx->targetSelectedMove].flags & SMART_USEFUL))
            return (Random() & 255) >= 100;
        return (Random() & 255) >= 20 ? -1 : 0;
    case MOVE_ROAR:
        return ctx->switchScore >= 10 ? -1 : 0;
    case MOVE_MEAN_LOOK:
        if (2u * user->hp <= user->maxHP) return -1;
        if (!ctx->targetReserve || ctx->targetGhost) return -10;
        if ((target->status1 & STATUS1_TOXIC_POISON) || target->volatiles.infatuation
         || target->volatiles.foresight || target->volatiles.rolloutTimer)
            return (Random() & 255) >= 50 ? 3 : 0;
        return ctx->switchScore < 11 ? -1 : 0;
    case MOVE_CURSE:
        if (!ctx->userGhost) return 0;
        if (target->volatiles.cursed) return -10;
        return (s32)ctx->userReserve + (4u * user->hp <= user->maxHP ? 2 : 0)
             - (ctx->targetReserve && (ctx->targetGhost || ctx->targetShedShell || ctx->targetAbility == ABILITY_RUN_AWAY || !ctx->targetTrapped));
    case MOVE_PERISH_SONG:
        if (!ctx->userReserve || target->volatiles.perishSong
         || (ctx->targetAbility == ABILITY_SOUNDPROOF && ctx->userAbility != ABILITY_MOLD_BREAKER))
            return -5;
        ctx->switchScore = ctx->bestReserveScore;
        if (!ctx->switchScore) return -5;
        return !ctx->targetReserve || (!ctx->targetGhost && !ctx->targetShedShell && ctx->targetTrapped) ? 3 : 0;
    case MOVE_PROTECT:
        return ProtectScore(user, target, ctx->targetCharging);
    case MOVE_BATON_PASS:
        // HL points at the unused byte after the seven stat stages when the source writes its adjustment.
        return 0;
    default:
        return 0;
    }
}
#endif
