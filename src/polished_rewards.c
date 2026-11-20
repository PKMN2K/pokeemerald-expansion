#include "global.h"
#include "battle.h"
#include "battle_util.h"
#include "battle_controllers.h"
#include "pokemon.h"
#include "item.h"
#include "polished_pokemon_data.h"
#include "polished_rewards.h"

u32 PolishedExperience(const struct PolishedExpContext *ctx)
{
    u32 exp = ctx->baseExp * ctx->defeatedLevel / 7;
    if ((!ctx->participant && !ctx->holder) || (!ctx->participants && !ctx->holders))
        return 0;
    if (ctx->traded) exp = exp * 3 / 2;
    if (ctx->trainer) exp = exp * 3 / 2;
    if (ctx->luckyEgg) exp = exp * 3 / 2;
    if (ctx->participants && ctx->holders)
        exp = exp * ((ctx->holder ? ctx->participants : 0) + (ctx->participant ? ctx->holders : 0))
            / (2 * ctx->participants * ctx->holders);
    else
        exp /= ctx->participants ? ctx->participants : ctx->holders;
    return max(1, exp);
}

bool32 UsesPolishedRewardRules(enum BattlerId defeated)
{
    return UsesPolishedBattleRules(GetBattlerAtPosition(B_POSITION_PLAYER_LEFT), defeated)
        && PolishedGetSpeciesBaseData(GetMonData(GetBattlerMon(defeated), MON_DATA_SPECIES)) != NULL;
}

bool32 PolishedIsTraded(struct Pokemon *mon)
{
    // The source has a two-byte public ID, and never compares OT name/language.
    return (u16)GetMonData(mon, MON_DATA_OT_ID)
        != (gSaveBlock2Ptr->playerTrainerId[0] | gSaveBlock2Ptr->playerTrainerId[1] << 8);
}

static void GetGroups(u32 *participants, u32 *holders)
{
    *participants = *holders = 0;
    for (u32 i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];
        if (!IsValidForBattle(mon))
            continue;
        if (gSentPokesToOpponent[0] & (1u << i))
            *participants |= 1u << i;
        if (GetMonData(mon, MON_DATA_HELD_ITEM) == ITEM_EXP_SHARE)
            *holders |= 1u << i;
    }
}

u32 PolishedPrepareExperience(enum BattlerId defeated)
{
    u32 participants, holders, count = 0;
    GetGroups(&participants, &holders);
    gBattleStruct->expSentInMons = participants;
    for (u32 i = 0; i < PARTY_SIZE; i++)
        if ((participants | holders) & (1u << i))
            gBattleStruct->expGettersOrder[count++] = i;
    if (count < PARTY_SIZE)
        gBattleStruct->expGettersOrder[count] = PARTY_SIZE;
    gBattleStruct->expOrderId = 0;
    gBattleStruct->expGetterMonId = gBattleStruct->expGettersOrder[0];
    return count;
}

u32 PolishedBattleExperience(u32 partyId, enum BattlerId defeated)
{
    u32 participants, holders;
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][partyId];
    enum Species species = GetMonData(GetBattlerMon(defeated), MON_DATA_SPECIES);
    const struct SpeciesBaseData *base = PolishedGetSpeciesBaseData(species);
    GetGroups(&participants, &holders);
    struct PolishedExpContext ctx = {
        .baseExp = base->expYield,
        .defeatedLevel = gBattleMons[defeated].level,
        .participant = (participants & (1u << partyId)) != 0,
        .holder = (holders & (1u << partyId)) != 0,
        .traded = PolishedIsTraded(mon),
        .trainer = (gBattleTypeFlags & BATTLE_TYPE_TRAINER) != 0,
        .luckyEgg = GetMonData(mon, MON_DATA_HELD_ITEM) == ITEM_LUCKY_EGG,
    };
    for (u32 i = 0; i < PARTY_SIZE; i++)
    {
        ctx.participants += (participants >> i) & 1;
        ctx.holders += (holders >> i) & 1;
    }
    return PolishedExperience(&ctx);
}

void PolishedGainBattleEVs(struct Pokemon *mon, enum Species defeatedSpecies)
{
    const struct SpeciesBaseData *base = PolishedGetSpeciesBaseData(defeatedSpecies);
    // The source explicitly retains this quirk for Exp. Share synergy: it
    // reads the active player's item, not the recipient's item.
    enum Item item = gBattleMons[GetBattlerAtPosition(B_POSITION_PLAYER_LEFT)].item;
    enum HoldEffect effect = GetItemHoldEffect(item);
    u32 powerStat = GetItemSecondaryId(item);
    u32 multiplier = (GetMonData(mon, MON_DATA_POKERUS) & 15) ? 2 : 1;
    u8 evs[NUM_STATS];
    u8 yields[NUM_STATS] = {base->evYield_HP, base->evYield_Attack, base->evYield_Defense,
        base->evYield_Speed, base->evYield_SpAttack, base->evYield_SpDefense};
    u32 total = 0;
    if (effect == HOLD_EFFECT_MACHO_BRACE) multiplier *= 2;
    for (u32 i = 0; i < NUM_STATS; i++)
    {
        evs[i] = GetMonData(mon, MON_DATA_HP_EV + i);
        total += evs[i];
    }
    for (u32 i = 0; i < NUM_STATS && total < 510; i++)
    {
        u32 gain = yields[i];
        if (effect == HOLD_EFFECT_POWER_ITEM && powerStat == i) gain += 8;
        gain = min(gain * multiplier, 510 - total);
        gain = min(gain, 252 - min(252, evs[i]));
        evs[i] += gain;
        total += gain;
        SetMonData(mon, MON_DATA_HP_EV + i, &evs[i]);
    }
}
