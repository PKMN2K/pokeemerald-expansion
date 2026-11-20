#include "global.h"
#include "battle.h"
#include "battle_util.h"
#include "pokemon.h"
#include "pokedex.h"
#include "item.h"
#include "overworld.h"
#include "rtc.h"
#include "wild_encounter.h"
#include "random.h"
#include "safari_zone.h"
#include "constants/battle_string_ids.h"
#include "polished_damage.h"
#include "polished_pokemon_data.h"
#include "polished_capture.h"
#include "data/polished_capture.h"

#define POLISHED_OWNERSHIP_MAGIC 0x50434F31 // PCO1, pinned source table indices.
STATIC_ASSERT(!B_POLISHED_MOVE_BALANCE || !FREE_EXTRA_SEEN_FLAGS_SAVEBLOCK2, PolishedOwnershipNeedsReservedDexBytes);

bool32 PolishedWasCaught(enum Species species)
{
    u32 index = species < NUM_SPECIES ? sPolishedCaptureForm[species] : 0;
#if FREE_EXTRA_SEEN_FLAGS_SAVEBLOCK2 == FALSE
    if (B_POLISHED_MOVE_BALANCE && index && gSaveBlock2Ptr->pokedex.unknown3 == POLISHED_OWNERSHIP_MAGIC)
        return (gSaveBlock2Ptr->pokedex.polishedCaughtForms[index / 8] >> (index % 8)) & 1;
#endif
    // Legacy saves only retained one bit per National Dex species. Preserve
    // that record for the base form; they cannot recover historical variants.
    return (!B_POLISHED_MOVE_BALANCE || index <= 291)
        && GetSetPokedexFlag(SpeciesToNationalPokedexNum(species), FLAG_GET_CAUGHT);
}

void PolishedRecordCaughtSpecies(enum Species species)
{
#if FREE_EXTRA_SEEN_FLAGS_SAVEBLOCK2 == FALSE
    u32 index = species < NUM_SPECIES ? sPolishedCaptureForm[species] : 0;
    if (!B_POLISHED_MOVE_BALANCE || !index)
        return;
    if (gSaveBlock2Ptr->pokedex.unknown3 != POLISHED_OWNERSHIP_MAGIC)
    {
        memset(gSaveBlock2Ptr->pokedex.polishedCaughtForms, 0, sizeof(gSaveBlock2Ptr->pokedex.polishedCaughtForms));
        for (u32 i = 0; i < ARRAY_COUNT(sPolishedCaptureDexSpecies); i++)
        {
            u32 oldSpecies = sPolishedCaptureDexSpecies[i];
            u32 oldIndex = sPolishedCaptureForm[oldSpecies];
            if (GetSetPokedexFlag(SpeciesToNationalPokedexNum(oldSpecies), FLAG_GET_CAUGHT))
                gSaveBlock2Ptr->pokedex.polishedCaughtForms[oldIndex / 8] |= 1u << (oldIndex % 8);
        }
        gSaveBlock2Ptr->pokedex.unknown3 = POLISHED_OWNERSHIP_MAGIC;
    }
    gSaveBlock2Ptr->pokedex.polishedCaughtForms[index / 8] |= 1u << (index % 8);
#endif
}

u32 PolishedHeavyBallCatchRate(u32 rate, u32 weight)
{
    u32 high = weight >> 8;
    if (high < 4)
        return rate < 20 ? 1 : rate - 20;
    // The pinned source compares inclusive HIGH() values, not full weights.
    return min(255, rate + (high <= 8 ? 0 : high <= 12 ? 20 : high <= 16 ? 30 : 40));
}

u32 PolishedCaptureOdds(struct PolishedCaptureContext *ctx)
{
    u32 value, maxHP = ctx->maxHP;
    u32 level = ctx->userLevel;
    u32 turns = min(254, ctx->completedTurns);

    if (ctx->tutorial || ctx->ball == BALL_MASTER)
        return 255;
    if (!maxHP)
        return 1; // Defensive boundary; a living source target has nonzero HP.
    value = (u16)(3 * maxHP - 2 * ctx->hp);

    switch (ctx->ball)
    {
    case BALL_GREAT:
    case BALL_PARK:
        value = value * 3 / 2;
        break;
    case BALL_ULTRA:
    case BALL_SAFARI:
        value *= 2;
        break;
    case BALL_HEAVY:
        ctx->catchRate = PolishedHeavyBallCatchRate(ctx->catchRate, ctx->weight);
        break;
    case BALL_LURE:
        if (ctx->fishing) value *= 5;
        break;
    case BALL_MOON:
        if (ctx->moonEvolution) value *= 4;
        break;
    case BALL_LOVE:
        if (ctx->loveEligible) value *= 8;
        break;
    case BALL_FAST:
        if (ctx->baseSpeed >= 100) value *= 4;
        break;
    case BALL_LEVEL:
        for (u32 i = 0; i < 3; i++)
            if (ctx->targetLevel < level)
            {
                value *= 2;
                level >>= 1;
            }
        break;
    case BALL_REPEAT:
        if (ctx->caught) value = value * 7 / 2;
        break;
    case BALL_TIMER:
        // Preserve the source byte additions, including their overflow.
        value = value * min(40, (u8)(3 * turns + 10)) / 10;
        break;
    case BALL_NEST:
        if (ctx->targetLevel < 36) value = value * (41 - ctx->targetLevel) / 5;
        break;
    case BALL_NET:
        if (ctx->type1 == TYPE_WATER || ctx->type2 == TYPE_WATER
         || ctx->type1 == TYPE_BUG || ctx->type2 == TYPE_BUG)
            value = value * 7 / 2;
        break;
    case BALL_DIVE:
        if (ctx->surfing || ctx->fishing) value = value * 7 / 2;
        break;
    case BALL_QUICK:
        if (!turns) value *= 5;
        break;
    case BALL_DUSK:
        if (ctx->dusk) value *= 3;
        break;
    case BALL_DREAM:
        if (ctx->status & STATUS1_SLEEP) value *= 4;
        break;
    default:
        break;
    }

    value *= ctx->catchRate;
    if (value > 0xFFFFFF)
        return 255;
    if (ctx->status & STATUS1_FREEZE)
        value = value * 5 / 2;
    else if (ctx->status & (STATUS1_PARALYSIS | STATUS1_BURN | STATUS1_POISON | STATUS1_TOXIC_POISON))
        value = value * 3 / 2;
    else if (ctx->status & STATUS1_SLEEP)
        value = value * 5 / 2;
    if (value > 0xFFFFFF)
        return 255;
    value /= 3;
    while (maxHP > 255)
    {
        maxHP >>= 1;
        value >>= 1;
    }
    return max(1, min(255, value / maxHP));
}

u32 PolishedCriticalCaptureThreshold(u32 odds, u32 caught, bool32 charm)
{
    u32 rank = caught <= 30 ? 0 : caught <= 100 ? 1 : caught <= 150 ? 2
        : caught <= 200 ? 3 : caught <= 250 ? 4 : 5;
    return odds * rank / (charm ? 6 : 12);
}

u32 PolishedCaptureWobbleThreshold(u32 odds)
{
    return sPolishedCaptureWobble[min(255, odds)];
}

u32 PolishedCaptureCaughtCount(void)
{
    u32 count = 0;
    for (u32 i = 0; i < ARRAY_COUNT(sPolishedCaptureDexSpecies); i++)
        count += GetSetPokedexFlag(SpeciesToNationalPokedexNum(sPolishedCaptureDexSpecies[i]), FLAG_GET_CAUGHT) != 0;
    return count;
}

bool32 UsesPolishedCaptureRules(enum BattlerId user, enum BattlerId target)
{
    enum PokeBall ball = ItemIdToBallId(gLastThrownBall);
    return B_POLISHED_MOVE_BALANCE && UsesPolishedBattleRules(user, target)
        && ball != BALL_BEAST && ball != BALL_SPORT && ball != BALL_STRANGE
        && PolishedGetSpeciesBaseData(GetMonData(GetBattlerMon(target), MON_DATA_SPECIES)) != NULL;
}

bool32 UsesPolishedSafariRules(void)
{
    return B_POLISHED_MOVE_BALANCE && (gBattleTypeFlags & BATTLE_TYPE_SAFARI)
        && UsesPolishedBattleRules(B_BATTLER_0, B_BATTLER_1)
        && PolishedGetSpeciesBaseData(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_SPECIES)) != NULL;
}

static void InitializeCatchRate(enum Species species)
{
    if (gBattleStruct->polishedCatchRateSpecies != species)
    {
        gBattleStruct->polishedCatchRateSpecies = species;
        gBattleStruct->polishedCatchRate = PolishedGetSpeciesBaseData(species)->catchRate;
    }
}

u32 PolishedSafariFleeThreshold(u32 speed, u32 balls, u32 eating, u32 anger)
{
    u32 threshold = (u8)speed * 2;
    if (!balls || threshold > 255)
        return 256; // Source exits before consuming a random byte.
    if (eating) threshold >>= 2;
    if (anger) threshold = min(255, threshold * 2);
    return threshold;
}

void PolishedSafariBaitOrRock(bool32 rock)
{
    InitializeCatchRate(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_SPECIES));
    u8 *counter = rock ? &gBattleStruct->safariGoNearCounter : &gBattleStruct->safariPkblThrowCounter;
    if (rock)
    {
        gBattleStruct->polishedCatchRate = min(255, gBattleStruct->polishedCatchRate * 2);
        gBattleStruct->safariPkblThrowCounter = 0;
    }
    else
    {
        gBattleStruct->polishedCatchRate >>= 1;
        gBattleStruct->safariGoNearCounter = 0;
    }
    // The source rejection-samples three bits for a uniform 1..5 duration.
    *counter = min(255, *counter + RandomUniform(RNG_POLISHED_SAFARI_DURATION, 1, 5));
}

u32 PolishedSafariReaction(void)
{
    if (gBattleStruct->safariPkblThrowCounter)
    {
        gBattleStruct->safariPkblThrowCounter--;
        return B_MSG_MON_EATING;
    }
    if (gBattleStruct->safariGoNearCounter)
    {
        if (--gBattleStruct->safariGoNearCounter == 0)
            gBattleStruct->polishedCatchRate = PolishedGetSpeciesBaseData(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_SPECIES))->catchRate;
        return B_MSG_MON_ANGRY;
    }
    return B_MSG_MON_WATCHING;
}

bool32 PolishedSafariFlees(void)
{
    u32 chance = PolishedSafariFleeThreshold(gBattleMons[GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT)].speed,
        gNumSafariBalls, gBattleStruct->safariPkblThrowCounter, gBattleStruct->safariGoNearCounter);
    return chance == 256 || RandomUniform(RNG_POLISHED_SAFARI_FLEE, 0, MAX_u8) < chance;
}

u32 GetPolishedCaptureOdds(enum BattlerId user, enum BattlerId target)
{
    enum Species original = GetMonData(GetBattlerMon(target), MON_DATA_SPECIES);
    const struct SpeciesBaseData *base = PolishedGetSpeciesBaseData(original);
    struct BattlePokemon *mon = &gBattleMons[target];
    enum TimeOfDay time = GetTimeOfDay();
    InitializeCatchRate(original);
    struct PolishedCaptureContext ctx = {
        .ball = ItemIdToBallId(gLastUsedItem),
        .status = mon->status1, .hp = mon->hp, .maxHP = mon->maxHP,
        .weight = PolishedBattleWeight(mon->species),
        .completedTurns = gBattleResults.battleTurnCounter,
        .catchRate = gBattleStruct->polishedCatchRate,
        .userLevel = gBattleMons[user].level, .targetLevel = mon->level,
        .baseSpeed = base->baseSpeed, .type1 = mon->types[0], .type2 = mon->types[1],
        .fishing = gIsFishingEncounter, .surfing = gIsSurfingEncounter,
        .dusk = time == TIME_EVENING || time == TIME_NIGHT || gMapHeader.cave || gMapHeader.mapType == MAP_TYPE_UNDERGROUND,
        .caught = PolishedWasCaught(original),
        .moonEvolution = sPolishedMoonBallSpecies[original],
        // The pinned Love Ball compares masked user form with the unmasked
        // target form. Valid source targets always carry a nonzero form.
        .loveEligible = FALSE,
        .tutorial = (gBattleTypeFlags & BATTLE_TYPE_CATCH_TUTORIAL) != 0,
    };
    u32 odds = PolishedCaptureOdds(&ctx);
    gBattleStruct->polishedCatchRate = ctx.catchRate;
    return odds;
}
