#include "global.h"
#include "pokemon.h"
#include "move.h"
#include "battle_util.h"
#include "data.h"
#include "random.h"
#include "trainer_util.h"
#include "constants/opponents.h"
#include "polished_trainers.h"
#include "regions.h"

struct PolishedTrainerEntry
{
    u16 trainer;
    u8 count, baseMoney;
    bool8 pluralName;
    enum Item items[MAX_TRAINER_ITEMS];
    struct PolishedTrainerAttributes mons[PARTY_SIZE + 1]; // Inver selects six of seven.
};
#include "data/polished_trainers.h"
#if POLISHED_JOHTO_DEMO
enum SourceMovePolicy { SOURCE_STATUS_MOVE = 1, SOURCE_STALL_MOVE = 2, SOURCE_RESIDUAL_MOVE = 4,
                        SOURCE_RAISE_STAT = 8, SOURCE_LOWER_STAT = 16 };
#include "data/polished_trainer_ai.h"
#include "data/polished_trainer_roster.h"

u32 PolishedTrainerAiLayers(u32 trainer, u32 badges)
{
    if (trainer >= TRAINERS_COUNT)
        return 0;
    u32 layers = sTrainerAiLayers[trainer] | POLISHED_AI_BASIC;
    if (badges >= 2) layers |= POLISHED_AI_TYPES;
    if (badges >= 4) layers |= POLISHED_AI_STATUS;
    if (badges >= 6) layers |= POLISHED_AI_SETUP;
    if (badges >= 8) layers |= POLISHED_AI_AGGRESSIVE | POLISHED_AI_RISKY;
    if (badges >= 16) layers |= POLISHED_AI_SMART;
    if (layers & POLISHED_AI_AGGRESSIVE)
        layers &= ~POLISHED_AI_TYPES;
    return layers;
}

void PolishedScoreSetup(const enum Move *moves, u32 limitations, bool32 playerFirstTurn,
                       bool32 enemyGhost, s32 *score)
{
    // BattleTurn increments the enemy counter before choosing its move, but
    // increments the player's counter afterwards. Curse takes the stat-up
    // branch only when the USER is Ghost in the default release.
    for (u32 slot = 0; slot < MAX_MON_MOVES; slot++)
    {
        enum Move move = moves[slot];
        if (move >= MOVES_COUNT_ALL || (limitations & (1u << slot)))
            continue;
        u32 policy = sSourceMovePolicies[move];
        if (!(policy & (SOURCE_RAISE_STAT | SOURCE_LOWER_STAT)) && !(move == MOVE_CURSE && enemyGhost))
            continue;
        u32 roll = Random() & 255;
        if (playerFirstTurn && (policy & SOURCE_LOWER_STAT))
            score[slot] += (roll >= 128) * 2;
        else if (roll >= 30)  // 12 percent as assembled: 12 * 255 / 100.
            score[slot] -= 2;
    }
}

static s32 SmartHealingScore(u32 hp, u32 maxHP)
{
    // CheckPinch includes equality. The other two comparisons are strict,
    // and the upper threshold doubles the already rounded third.
    u32 third = max(1u, maxHP / 3);
    if (hp <= third)
        return 2;
    if (hp < max(1u, maxHP / 2))
        return 1;
    if (hp < 2 * third)
        return 0;
    return -2;
}

bool32 PolishedSmartUserIsFaster(const struct PolishedSmartAiContext *context)
{
    if (context->userSpeed == context->targetSpeed)
    {
        // CheckSpeed draws once per tie. AICompareSpeed clears carry for either result.
        Random();
        return FALSE;
    }
    if (context->trickRoom)
        return context->userSpeed < context->targetSpeed;
    return context->userSpeed > context->targetSpeed;
}

// AI_Basic applies AIDiscourageMove once when the move is redundant.
static bool32 IsRedundantBasicMove(enum Move move, const struct BattlePokemon *user,
                                   const struct BattlePokemon *target, u32 userSide, u32 targetSide, u32 weather)
{
    switch (move)
    {
    case MOVE_DREAM_EATER:
        return !(target->status1 & STATUS1_SLEEP);
    case MOVE_SING:
    case MOVE_SLEEP_POWDER:
    case MOVE_HYPNOSIS:
    case MOVE_SPORE:
        return (targetSide & SIDE_STATUS_SAFEGUARD) || (target->status1 & STATUS1_ANY);
    case MOVE_RECOVER:
    case MOVE_REST:
    case MOVE_ROOST:
    case MOVE_POLISHED_FRESH_SNACK:
    case MOVE_POLISHED_HEALING_LIGHT:
        return user->hp == user->maxHP;
    case MOVE_RAIN_DANCE:
    case MOVE_SUNNY_DAY:
    case MOVE_SANDSTORM:
    case MOVE_HAIL:
        return (weather & (1u << GetMoveWeatherType(move))) != 0;
    case MOVE_REFLECT:
        return (userSide & SIDE_STATUS_REFLECT) != 0;
    case MOVE_LIGHT_SCREEN:
        return (userSide & SIDE_STATUS_LIGHTSCREEN) != 0;
    case MOVE_SAFEGUARD:
        return (userSide & SIDE_STATUS_SAFEGUARD) != 0;
    case MOVE_CONFUSE_RAY:
    case MOVE_SUPERSONIC:
    case MOVE_SWEET_KISS:
        return target->volatiles.confusionTimer || (targetSide & SIDE_STATUS_SAFEGUARD);
    case MOVE_SWAGGER:
        return target->volatiles.confusionTimer != 0;
    default:
        return FALSE;
    }
}

void PolishedScoreBasicMoves(const struct BattlePokemon *user, const struct BattlePokemon *target,
                            u32 limitations, u32 userSide, u32 targetSide, u32 weather, s32 *scores)
{
    for (u32 slot = 0; slot < MAX_MON_MOVES; slot++)
        if (!(limitations & (1u << slot))
         && IsRedundantBasicMove(user->moves[slot], user, target, userSide, targetSide, weather))
            scores[slot] -= 10;
}

// wAIMoves is packed after CheckUsableMove excludes disabled and empty slots.
static bool32 SmartHasUsableMove(const struct BattlePokemon *user, u32 limitations, enum Move move)
{
    for (u32 slot = 0; slot < MAX_MON_MOVES; slot++)
        if (!(limitations & (1u << slot)) && user->moves[slot] == move)
            return TRUE;
    return FALSE;
}

// UpdateUsedMoves keeps the four most recently introduced distinct moves.
EWRAM_DATA enum Move gPolishedPlayerUsedMoves[MAX_MON_MOVES] = {0};

void PolishedRecordPlayerMove(enum Move move)
{
    for (u32 i = 0; i < MAX_MON_MOVES; i++)
    {
        if (gPolishedPlayerUsedMoves[i] == move)
            return;
        if (gPolishedPlayerUsedMoves[i] == MOVE_NONE)
        {
            gPolishedPlayerUsedMoves[i] = move;
            return;
        }
    }
    for (u32 i = 1; i < MAX_MON_MOVES; i++)
        gPolishedPlayerUsedMoves[i - 1] = gPolishedPlayerUsedMoves[i];
    gPolishedPlayerUsedMoves[MAX_MON_MOVES - 1] = move;
}

static s32 SmartDrainScore(enum Move move, bool32 full, const struct PolishedSmartAiContext *context,
                          const struct BattlePokemon *target)
{
    enum Type type = GetMoveType(move);
    s32 matchup = 0; // Signed number of type-chart doublings.
    for (u32 i = 0; i < ARRAY_COUNT(context->targetTypes); i++)
    {
        enum Type def = context->targetTypes[i];
        if (def == TYPE_MYSTERY || def == TYPE_NONE || (i && def == context->targetTypes[0])
         || (i == 2 && def == context->targetTypes[1]))
            continue;
        uq4_12_t mod = GetTypeModifier(type, def);
        if (mod == 0)
        {
            if (context->targetRing || (type == TYPE_FIGHTING && def == TYPE_GHOST
             && (target->volatiles.foresight || context->userAbility == ABILITY_SCRAPPY || context->userAbility == ABILITY_MINDS_EYE)))
                continue;
            matchup = -1;
            break;
        }
        matchup += (mod > UQ_4_12(1.0)) - (mod < UQ_4_12(1.0));
    }
    if (type == TYPE_GRASS && context->targetAbility == ABILITY_SAP_SIPPER && context->userAbility != ABILITY_MOLD_BREAKER)
        matchup = -1;
    if (matchup < 0)
        return (Random() & 255) >= 100 ? -1 : 0;
    return matchup > 0 && !full && (Random() & 255) >= 50;
}

static s32 SmartAccuracyEvasionScore(const struct BattlePokemon *user, const struct BattlePokemon *target, bool32 evasion)
{
    const struct BattlePokemon *mon = evasion ? user : target;
    bool32 toxic = target->status1 & STATUS1_TOXIC_POISON;
    s32 score = 0;
    if (evasion && user->statStages[STAT_EVASION] == MAX_STAT_STAGE)
        return -10;
    if (mon->hp == mon->maxHP && (evasion || 2u * user->hp > user->maxHP))
    {
        if (toxic || (Random() & 255) < 178)
            return 2;
    }
    else if (4u * mon->hp <= mon->maxHP)
        score = -2;
    else if ((Random() & 255) < 10)
        return 2;
    else if (2u * mon->hp > mon->maxHP)
    {
        if ((Random() & 255) < 50)
            return 2;
    }
    else if ((Random() & 255) >= 128)
        score = -2;

    if (toxic)
        return score + ((Random() & 255) >= 80 ? 2 : 0);
    if (target->volatiles.leechSeed)
        return score + ((Random() & 255) >= 128);
    if (target->statStages[STAT_ACC] >= user->statStages[STAT_EVASION] && target->volatiles.rolloutTimer)
        return score + 2;
    return score - 1;
}

static s32 SmartHazeScore(const struct BattlePokemon *user, const struct BattlePokemon *target)
{
    for (u32 stat = STAT_ATK; stat <= STAT_EVASION; stat++)
        if (user->statStages[stat] < DEFAULT_STAT_STAGE - 2 || target->statStages[stat] >= DEFAULT_STAT_STAGE + 3)
            return (Random() & 255) >= 40;
    return -1;
}

void PolishedScoreSmartMoves(const struct BattlePokemon *user, const struct BattlePokemon *target,
                            u32 limitations, struct PolishedSmartAiContext *context, s32 *scores)
{
    for (u32 slot = 0; slot < MAX_MON_MOVES; slot++)
    {
        if (limitations & (1u << slot))
            continue;
        enum Move move = user->moves[slot];
        switch (move)
        {
        case MOVE_ABSORB:
        case MOVE_MEGA_DRAIN:
        case MOVE_GIGA_DRAIN:
        case MOVE_DRAIN_PUNCH:
        case MOVE_DRAINING_KISS:
        case MOVE_LEECH_LIFE:
            scores[slot] += SmartDrainScore(move, user->hp == user->maxHP, context, target);
            break;
        case MOVE_DOUBLE_TEAM:
        case MOVE_SMOKESCREEN:
        case MOVE_FLASH:
            scores[slot] += SmartAccuracyEvasionScore(user, target, move == MOVE_DOUBLE_TEAM);
            break;
        case MOVE_FLY:
        case MOVE_DIG:
            if (target->volatiles.semiInvulnerable == STATE_ON_AIR || target->volatiles.semiInvulnerable == STATE_UNDERGROUND)
            {
                if (PolishedSmartUserIsFaster(context))
                    scores[slot] += 3;
            }
            else if (!context->powerHerb && context->seenProtection)
                scores[slot] -= 10;
            break;
        case MOVE_SOLAR_BEAM:
            if (!(context->weather & B_WEATHER_SUN))
            {
                if (SmartHasUsableMove(user, limitations, MOVE_SUNNY_DAY))
                    scores[slot] -= 2;
                else if (!context->powerHerb && context->seenProtection)
                    scores[slot] -= 10;
            }
            break;
        case MOVE_HIGH_JUMP_KICK:
            if (context->userAbility != ABILITY_NO_GUARD && context->targetAbility != ABILITY_NO_GUARD
             && PolishedSmartUserIsFaster(context) && (target->volatiles.semiInvulnerable == STATE_ON_AIR
                                            || target->volatiles.semiInvulnerable == STATE_UNDERGROUND))
                scores[slot] -= 10;
            break;
        case MOVE_GUST:
        case MOVE_EARTHQUAKE:
        case MOVE_MAGNITUDE:
        {
            bool32 gust = move == MOVE_GUST;
            if (context->targetSelectedMove != (gust ? MOVE_FLY : MOVE_DIG))
                break;
            bool32 faster = PolishedSmartUserIsFaster(context);
            if (target->volatiles.semiInvulnerable == (gust ? STATE_ON_AIR : STATE_UNDERGROUND))
                scores[slot] += faster ? 2 : 0;
            else if (!faster && (Random() & 255) >= 128)
                scores[slot]++;
            break;
        }
        case MOVE_RECOVER:
        case MOVE_REST:
        case MOVE_ROOST:
        case MOVE_POLISHED_FRESH_SNACK:
        case MOVE_POLISHED_HEALING_LIGHT:
            scores[slot] += SmartHealingScore(user->hp, user->maxHP);
            if (move == MOVE_POLISHED_HEALING_LIGHT && context->userWeather != B_WEATHER_NONE)
                scores[slot] += (context->userWeather & B_WEATHER_SUN) ? 1 : -1;
            break;
        case MOVE_RAIN_DANCE:
        case MOVE_SUNNY_DAY:
        case MOVE_SANDSTORM:
        case MOVE_HAIL:
            if (context->weather & (1u << GetMoveWeatherType(move)))
                scores[slot] -= 10;
            else
            {
                scores[slot] += (context->weather == B_WEATHER_NONE) ? 1 : 2;
                if (2u * user->hp <= user->maxHP)
                    scores[slot] -= 2;
            }
            break;
        case MOVE_THUNDER:
            if ((context->weather & B_WEATHER_SUN) && (Random() & 255) >= 25)
                scores[slot]--;
            break;
        case MOVE_LIGHT_SCREEN:
        case MOVE_REFLECT:
            if (user->hp != user->maxHP && (Random() & 255) >= 20)
                scores[slot]--;
            break;
        case MOVE_SAFEGUARD:
            if (2u * target->hp <= target->maxHP && (Random() & 255) >= 50)
                scores[slot]--;
            break;
        case MOVE_AGILITY:
        case MOVE_SCARY_FACE:
        case MOVE_STRING_SHOT:
            if (PolishedSmartUserIsFaster(context))
                scores[slot] -= 10;
            break;
        case MOVE_TRICK_ROOM:
            if (PolishedSmartUserIsFaster(context))
                scores[slot] -= 10;
            else if (!context->trickRoom)
                scores[slot] += 3;
            break;
        case MOVE_THUNDER_WAVE:
        case MOVE_GLARE:
        case MOVE_STUN_SPORE:
            if (4u * target->hp <= target->maxHP)
                scores[slot] -= (Random() & 255) >= 128;
            else if (!PolishedSmartUserIsFaster(context) && 4u * user->hp > user->maxHP && (Random() & 255) >= 50)
                scores[slot] += 2;
            break;
        case MOVE_ICY_WIND:
            if (4u * user->hp > user->maxHP && context->targetFirstTurn
             && !PolishedSmartUserIsFaster(context) && (Random() & 255) >= 30)
                scores[slot] += 2;
            break;
        case MOVE_SUBSTITUTE:
            if (2u * user->hp <= user->maxHP)
                scores[slot] -= 10;
            break;
        case MOVE_DESTINY_BOND:
            if (4u * user->hp > user->maxHP)
                scores[slot]--;
            break;
        case MOVE_PAIN_SPLIT:
            if (2u * user->hp > target->hp)
                scores[slot]--;
            break;
        case MOVE_BELLY_DRUM:
            if (user->statStages[STAT_ATK] >= DEFAULT_STAT_STAGE + 3)
                scores[slot] -= 5;
            else if (user->hp != user->maxHP)
                scores[slot] -= (2u * user->hp <= user->maxHP) ? 6 : 1;
            break;
        case MOVE_EXPLOSION:
            if (context->lastMonAgainstReserve || 2u * user->hp > user->maxHP
             || (4u * user->hp > user->maxHP && (Random() & 255) >= 20))
                scores[slot] -= 3;
            break;
        case MOVE_HYPER_BEAM:
        case MOVE_GIGA_IMPACT:
            if (2u * user->hp > user->maxHP)
            {
                if ((Random() & 255) < 40)
                    break;
                scores[slot]--;
                if ((Random() & 255) >= 128)
                    scores[slot]--;
            }
            else if (4u * user->hp <= user->maxHP && (Random() & 255) >= 128)
                scores[slot]++;
            break;
        case MOVE_PURSUIT:
            if (4u * target->hp <= target->maxHP)
            {
                if ((Random() & 255) >= 128)
                    scores[slot] += 2;
            }
            else if ((Random() & 255) >= 50)
                scores[slot]--;
            break;
        case MOVE_AERIAL_ACE:
        case MOVE_SWIFT:
        case MOVE_FEINT_ATTACK:
        case MOVE_AURA_SPHERE:
        case MOVE_DISARMING_VOICE:
            if ((user->statStages[STAT_ACC] < DEFAULT_STAT_STAGE - 2
              || target->statStages[STAT_EVASION] >= DEFAULT_STAT_STAGE + 3) && (Random() & 255) >= 50)
                scores[slot] += 2;
            break;
        case MOVE_SUPERSONIC:
        case MOVE_CONFUSE_RAY:
        case MOVE_SWEET_KISS:
            if (2u * target->hp <= target->maxHP)
            {
                if ((Random() & 255) >= 25)
                    scores[slot]--;
                if (4u * target->hp <= target->maxHP)
                    scores[slot]--;
            }
            break;
        case MOVE_SWAGGER:
        case MOVE_ATTRACT:
            if (context->targetFirstTurn)
                scores[slot] += (Random() & 255) < 200; // 79 percent - 1 in the source.
            else
                scores[slot] -= (Random() & 255) >= 50;
            break;
        case MOVE_STOMP:
        case MOVE_BODY_SLAM:
            if (target->volatiles.minimize && (Random() & 255) >= 50)
                scores[slot]++;
            break;
        case MOVE_FLARE_BLITZ:
        case MOVE_SACRED_FIRE:
            if (user->status1 & STATUS1_FREEZE)
                scores[slot] += 5;
            break;
        case MOVE_AMNESIA:
            if (2u * user->hp <= user->maxHP || user->statStages[STAT_SPDEF] >= DEFAULT_STAT_STAGE + 4)
                scores[slot]--;
            else if (user->statStages[STAT_SPDEF] < DEFAULT_STAT_STAGE + 2 && (Random() & 255) >= 50)
                scores[slot] += 2;
            break;
        case MOVE_SLEEP_TALK:
            scores[slot] += (user->status1 & STATUS1_SLEEP) >= (context->earlyBird ? 3u : 2u) ? 10 : -10;
            break;
        case MOVE_SING:
        case MOVE_SLEEP_POWDER:
        case MOVE_HYPNOSIS:
        case MOVE_SPORE:
            if (SmartHasUsableMove(user, limitations, MOVE_DREAM_EATER) && (Random() & 255) >= 128)
                scores[slot] += 2;
            break;
        case MOVE_DREAM_EATER:
            if ((Random() & 255) >= 25)
                scores[slot] += 3;
            break;
        case MOVE_WRAP:
        case MOVE_FIRE_SPIN:
        case MOVE_WHIRLPOOL:
            if (!target->volatiles.wrapTurns && !context->targetGhost
             && ((target->status1 & STATUS1_TOXIC_POISON) || target->volatiles.infatuation
              || target->volatiles.foresight || target->volatiles.rolloutTimer || context->targetFirstTurn))
            {
                if (4u * user->hp > user->maxHP && (Random() & 255) >= 128)
                    scores[slot] += 2;
            }
            else if ((Random() & 255) >= 128)
                scores[slot]--;
            break;
        case MOVE_RAPID_SPIN:
            if ((user->volatiles.wrapTurns || user->volatiles.leechSeed || context->spikes == 3)
             && (Random() & 255) >= 50)
                scores[slot] += 2;
            break;
        case MOVE_FORESIGHT:
            if (user->statStages[STAT_ACC] < DEFAULT_STAT_STAGE - 2
             || target->statStages[STAT_EVASION] >= DEFAULT_STAT_STAGE + 3 || context->targetGhost)
                scores[slot] += (Random() & 255) >= 100 ? 2 : 0;
            else if ((Random() & 255) >= 20)
                scores[slot]--;
            break;
        case MOVE_HAZE:
            scores[slot] += SmartHazeScore(user, target);
            break;
        case MOVE_ROLLOUT:
            if (user->volatiles.infatuation || user->volatiles.confusionTimer || (user->status1 & STATUS1_PARALYSIS)
             || 4u * user->hp <= user->maxHP || user->statStages[STAT_ACC] < DEFAULT_STAT_STAGE
             || target->statStages[STAT_EVASION] > DEFAULT_STAT_STAGE)
                scores[slot] -= (Random() & 255) >= 50;
            else if ((Random() & 255) < 200)
                scores[slot] += 2;
            break;
        case MOVE_RAGE:
            if (user->volatiles.rage)
                scores[slot] += (Random() & 255) >= 128;
            else if (2u * user->hp <= user->maxHP)
                scores[slot]--;
            else if ((Random() & 255) < 50)
                scores[slot]++;
            break;
        case MOVE_FUTURE_SIGHT:
            scores[slot]--;
            if (PolishedSmartUserIsFaster(context) && (target->volatiles.semiInvulnerable == STATE_ON_AIR
                                           || target->volatiles.semiInvulnerable == STATE_UNDERGROUND))
                scores[slot] += 3;
            break;
        case MOVE_ENDURE:
            if (user->volatiles.consecutiveMoveUses || user->hp == user->maxHP)
                scores[slot] -= 2;
            else if (4u * user->hp > user->maxHP)
                scores[slot]--;
            else if (SmartHasUsableMove(user, limitations, MOVE_REVERSAL))
                scores[slot] += (Random() & 255) >= 50 ? 3 : 0;
            else if (target->volatiles.lockOn && (Random() & 255) >= 128)
                scores[slot] += 2;
            break;
        case MOVE_HEAL_BELL:
            if (context->partyHasStatus)
            {
                if (user->status1 & STATUS1_ANY)
                    scores[slot]++;
                if ((user->status1 & (STATUS1_SLEEP | STATUS1_FREEZE)) && (Random() & 255) >= 128)
                    scores[slot] += 2;
            }
            else if (!(user->status1 & STATUS1_ANY))
                scores[slot] -= 10;
            break;
        case MOVE_LEECH_SEED:
        case MOVE_TOXIC:
            if (2u * target->hp <= target->maxHP)
                scores[slot]--;
            break;
        default:
            scores[slot] += PolishedScoreReactiveMove(user, target, slot, context);
            break;
        }
    }
}

void PolishedScoreTrainerPolicies(u32 layers, const enum Move *moves, u32 limitations,
                                  u32 hp, u32 maxHP, bool32 hasTakenTurn, s32 *score)
{
    bool32 stall = (layers & POLISHED_AI_OPPORTUNIST) && 2 * hp <= maxHP
        && (4 * hp <= maxHP || (Random() & 255) >= 128);
    bool32 residual = hasTakenTurn && (layers & POLISHED_AI_CAUTIOUS);
    for (u32 i = 0; i < MAX_MON_MOVES; i++)
    {
        if (moves[i] == MOVE_NONE || moves[i] >= MOVES_COUNT_ALL || (limitations & (1u << i)))
            continue;
        u32 policy = sSourceMovePolicies[moves[i]];
        if ((layers & POLISHED_AI_OFFENSIVE) && (policy & SOURCE_STATUS_MOVE))
            score[i] -= 2;
        if (stall && (policy & SOURCE_STALL_MOVE))
            score[i]--;
        if (residual && (policy & SOURCE_RESIDUAL_MOVE))
        {
            // AI_Cautious returns from its entire scan after a failed byte.
            residual = (Random() & 255) < 230;
            if (residual) score[i]--;
        }
    }
}

STATIC_ASSERT(ARRAY_COUNT(sRosterEntries) == POLISHED_ROSTER_COUNT, PolishedRosterCount);
#endif

static const struct PolishedTrainerEntry *FindPolishedTrainer(u32 trainer)
{
#if !IS_FRLG
#if POLISHED_JOHTO_DEMO
    if (trainer >= POLISHED_ROSTER_FIRST && trainer < TRAINERS_COUNT)
        return &sRosterEntries[trainer - POLISHED_ROSTER_FIRST];
#endif
    for (u32 i = 0; i < ARRAY_COUNT(sPolishedTrainerEntries); i++)
        if (sPolishedTrainerEntries[i].trainer == trainer)
            return &sPolishedTrainerEntries[i];
#endif
    return NULL;
}

bool32 PolishedTrainerHasPluralName(u32 trainer)
{
    const struct PolishedTrainerEntry *entry = FindPolishedTrainer(trainer);
    return entry != NULL && entry->pluralName;
}

const struct PolishedTrainerAttributes *PolishedGetTrainerAttributes(u32 trainer, u32 slot)
{
    const struct PolishedTrainerEntry *entry = FindPolishedTrainer(trainer);
    return entry != NULL && slot < entry->count ? &entry->mons[slot] : NULL;
}

const enum Item *PolishedGetTrainerItems(u32 trainer)
{
    const struct PolishedTrainerEntry *entry = FindPolishedTrainer(trainer);
    return entry != NULL ? entry->items : NULL;
}

u32 PolishedGetTrainerBaseMoney(u32 trainer)
{
    const struct PolishedTrainerEntry *entry = FindPolishedTrainer(trainer);
    return entry != NULL ? entry->baseMoney : 0;
}

bool32 PolishedGenerateTrainerMon(struct Pokemon *mon, const struct TrainerMon *definition,
    struct TrainerGenerator *generator, u32 trainer, u32 slot)
{
    const struct PolishedTrainerAttributes *attrs = PolishedGetTrainerAttributes(trainer, slot);
    if (!B_POLISHED_MOVE_BALANCE || attrs == NULL)
        return FALSE;
    assertf(definition->species == attrs->species, "Source trainer binding differs from party species");
    struct TrainerMon source = *definition;
#if POLISHED_JOHTO_DEMO
    if (source.species == SPECIES_ARBOK && (source.tags & POLISHED_REGIONAL_ARBOK) && GetCurrentRegion() == REGION_KANTO)
        source.species = SPECIES_ARBOK_KANTO;
#endif
    source.iv = attrs->iv;
    source.ev = attrs->ev;
    source.nature = attrs->nature;
    source.gender = attrs->gender;
    source.ability = GetSpeciesAbility(source.species, attrs->abilitySlot);
    source.friendship = 160;
    source.ball = BALL_POKE;
    GenerateMonFromTrainerMon(mon, &source, generator);
#if !IS_FRLG
    if (trainer == TRAINER_POLISHED_GIOVANNI1 && slot == 0)
    {
        static const u8 nickname[] = _("?????");
        SetMonData(mon, MON_DATA_NICKNAME, nickname);
    }
#endif
    // Duplicate ability names must not collapse the source's stored slot.
    u32 abilitySlot = attrs->abilitySlot;
    SetMonData(mon, MON_DATA_ABILITY_NUM, &abilitySlot);
    return TRUE;
}
