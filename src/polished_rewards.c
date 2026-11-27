#include "global.h"
#include "battle.h"
#include "battle_util.h"
#include "battle_controllers.h"
#include "pokemon.h"
#include "item.h"
#include "money.h"
#include "polished_tower.h"
#include "polished_pokemon_data.h"
#include "polished_rewards.h"
#include "random.h"

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
        // Safari never sends a player Pokemon in. Expansion still initializes
        // its participant mask with the lead, so only held Exp. Shares count.
        if (!(gBattleTypeFlags & BATTLE_TYPE_SAFARI) && (gSentPokesToOpponent[0] & (1u << i)))
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

// Polished Crystal v3.2.3, data/items/pickup_items.asm.
static const enum Item sPickupItems[] = {
    ITEM_POTION, ITEM_ANTIDOTE, ITEM_SUPER_POTION, ITEM_GREAT_BALL,
    ITEM_REPEL, ITEM_ESCAPE_ROPE, ITEM_FULL_HEAL, ITEM_HYPER_POTION,
    ITEM_ULTRA_BALL, ITEM_REVIVE, ITEM_RARE_CANDY, ITEM_DUSK_STONE,
    ITEM_SHINY_STONE, ITEM_MAX_ETHER, ITEM_FULL_RESTORE, ITEM_MAX_REVIVE,
    ITEM_PP_UP, ITEM_MAX_ELIXIR,
};

static const enum Item sRarePickupItems[] = {
    ITEM_HYPER_POTION, ITEM_NUGGET, ITEM_KINGS_ROCK, ITEM_FULL_RESTORE,
    ITEM_ETHER, ITEM_LUCKY_EGG, ITEM_DESTINY_KNOT, ITEM_ELIXIR,
    ITEM_BIG_NUGGET, ITEM_LEFTOVERS, ITEM_BOTTLE_CAP,
};

bool32 UsesPolishedPostBattleAbilities(void)
{
    return UsesPolishedBattleRules(B_BATTLER_0, B_BATTLER_1)
        && !(gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED | BATTLE_TYPE_RECORDED_LINK
            | BATTLE_TYPE_FRONTIER | BATTLE_TYPE_TRAINER_HILL | BATTLE_TYPE_EREADER_TRAINER
            | BATTLE_TYPE_CATCH_TUTORIAL));
}

bool32 PolishedHasPayDayCoins(void)
{
    if (!gBattleStruct->polishedPayDay || gBattleStruct->moneyMultiplierMove || !gPaydayMoney)
        return FALSE;
#if POLISHED_JOHTO_DEMO
    if ((gBattleTypeFlags & BATTLE_TYPE_BATTLE_TOWER)
        && !(gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED | BATTLE_TYPE_RECORDED_LINK))
        && UsesPolishedBattleRules(B_BATTLER_0, B_BATTLER_1) && IsPolishedTowerChallenge())
        return TRUE;
#endif
    return UsesPolishedPostBattleAbilities();
}

u32 PolishedCollectPayDay(u32 coins, bool32 amuletCoin)
{
    // CheckPayDay saturates the doubled pot. AddBattleMoneyToAccount drops
    // the carry from its three-byte addition before applying the wallet cap.
    u32 payout = amuletCoin ? min(0xFFFFFF, coins * 2) : coins;
    u32 wallet = (GetMoney(&gSaveBlock1Ptr->money) + payout) & 0xFFFFFF;
    SetMoney(&gSaveBlock1Ptr->money, min(wallet, MAX_MONEY));
    return payout;
}

static enum Item GetPickupItem(u32 level)
{
    u32 tier = min((level - 1) / 10, 9);
    u32 roll = PolishedRandomRange(100);
    if (roll < 2)
        return sRarePickupItems[tier + ((Random() & 0xff) < 128)];

    static const u8 thresholds[] = {6, 10, 20, 30, 40, 50, 60, 70};
    for (u32 i = 0; i < ARRAY_COUNT(thresholds); i++)
        if (roll < thresholds[i])
            tier++;
    return sPickupItems[tier];
}

enum Item PolishedApplyPostBattleAbility(struct Pokemon *mon)
{
    enum Species species = GetMonData(mon, MON_DATA_SPECIES_OR_EGG);
    if (species == SPECIES_NONE || species == SPECIES_EGG)
        return ITEM_NONE;

    enum Item reward;
    switch (GetMonAbility(mon))
    {
    case ABILITY_NATURAL_CURE:
    {
        u32 status = STATUS1_NONE;
        SetMonData(mon, MON_DATA_STATUS, &status);
        return ITEM_NONE;
    }
    case ABILITY_HONEY_GATHER:
    {
        u32 divisor = PolishedRandomRange(20) + 1;
        // The release compares the integer quotient with 11, including at level 1.
        if ((GetMonData(mon, MON_DATA_LEVEL) + 9) / divisor < 11)
            return ITEM_NONE;
        reward = ITEM_HONEY;
        break;
    }
    case ABILITY_PICKUP:
        if (PolishedRandomRange(10) != 0)
            return ITEM_NONE;
        reward = GetPickupItem(GetMonData(mon, MON_DATA_LEVEL));
        break;
    default:
        return ITEM_NONE;
    }

    if (GetMonData(mon, MON_DATA_HELD_ITEM) == ITEM_NONE)
        SetMonData(mon, MON_DATA_HELD_ITEM, &reward);
    else if (!AddBagItem(reward, 1))
        return ITEM_NONE;
    return reward;
}
