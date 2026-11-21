#include "global.h"
#include "test/battle.h"
#include "battle_util.h"
#include "overworld.h"
#include "battle_setup.h"
#include "battle_util2.h"
#include "polished_friendship.h"
#include "polished_growth.h"
#include "polished_friendship_vectors.h"
#include "polished_growth_vectors.h"
#include "constants/opponents.h"

static void SetFriendshipData(struct Pokemon *mon, u32 friendship, bool32 home, enum Item item, enum PokeBall ball)
{
    u32 location = GetCurrentRegionMapSectionId() ^ !home;
    SetMonData(mon, MON_DATA_FRIENDSHIP, &friendship);
    SetMonData(mon, MON_DATA_MET_LOCATION, &location);
    SetMonData(mon, MON_DATA_HELD_ITEM, &item);
    SetMonData(mon, MON_DATA_POKEBALL, &ball);
}

TEST("Polished friendship level gains use the source 100 and 160 tiers")
{
    struct Pokemon mon;
    gMain.inBattle = FALSE;
    CreateRandomMon(&mon, SPECIES_CHANSEY, 10);
    SetFriendshipData(&mon, 159, FALSE, ITEM_NONE, BALL_POKE);
    AdjustFriendship(&mon, FRIENDSHIP_EVENT_GROW_LEVEL);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_FRIENDSHIP), B_POLISHED_MOVE_BALANCE ? 161 : 162);
}

TEST("Polished friendship applies additive bonuses even when the base gain is zero")
{
    struct Pokemon mon;
    gMain.inBattle = FALSE;
    CreateRandomMon(&mon, SPECIES_CHANSEY, 10);
    SetFriendshipData(&mon, 160, FALSE, ITEM_SOOTHE_BELL, BALL_LUXURY);
    AdjustFriendship(&mon, FRIENDSHIP_EVENT_GROW_LEVEL);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_FRIENDSHIP), B_POLISHED_MOVE_BALANCE ? 162 : 165);
}

TEST("Polished friendship penalties have the source high tier and no positive bonuses")
{
    struct Pokemon mon;
    gMain.inBattle = FALSE;
    CreateRandomMon(&mon, SPECIES_CHANSEY, 10);
    SetFriendshipData(&mon, 160, TRUE, ITEM_SOOTHE_BELL, BALL_LUXURY);
    AdjustFriendship(&mon, FRIENDSHIP_EVENT_FAINT_LARGE);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_FRIENDSHIP), B_POLISHED_MOVE_BALANCE ? 150 : 157);
}

TEST("Polished friendship growth starts mapped level one Pokemon at zero EXP")
{
    struct Pokemon mon;
    gMain.inBattle = FALSE;
    SetConfig(CONFIG_POLISHED_SPECIES_DATA, TRUE);
    CreateRandomMon(&mon, SPECIES_CHANSEY, 1);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_EXP), 0);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_LEVEL), 1);
    EXPECT_EQ(GetLevelFromMonExp(&mon), 1);
    EXPECT_EQ(GetLevelFromBoxMonExp(&mon.box), 1);
}

TEST("Polished friendship matches all 23552 authenticated byte carry cases")
{
    u32 n = 0;
    for (u32 event = 0; event < POLISHED_HAPPINESS_COUNT; event++)
        for (u32 value = 0; value < 256; value++)
            for (u32 bell = 0; bell < 2; bell++)
                for (u32 luxury = 0; luxury < 2; luxury++)
                    EXPECT_EQ(PolishedFriendshipAfterEvent(value, event, bell, luxury), sPolishedFriendshipVectors[n++]);
    EXPECT_EQ(n, ARRAY_COUNT(sPolishedFriendshipVectors));
    EXPECT_EQ(PolishedFriendshipAfterEvent(160, POLISHED_HAPPINESS_COUNT, TRUE, TRUE), 160);
}

TEST("Polished friendship level gains distinguish home away and both source boundaries")
{
    const u8 starts[] = {0, 99, 100, 159, 160, 199, 200, 254, 255};
    struct Pokemon mon;
    gMain.inBattle = FALSE;
    CreateRandomMon(&mon, SPECIES_CHANSEY, 10);
    for (u32 home = 0; home < 2; home++)
        for (u32 i = 0; i < ARRAY_COUNT(starts); i++)
        {
            u32 initial = starts[i];
            SetFriendshipData(&mon, initial, home, ITEM_NONE, BALL_POKE);
            AdjustFriendship(&mon, FRIENDSHIP_EVENT_GROW_LEVEL);
            u32 delta = initial < 100 ? 3 : initial < 160 ? 2 : 0;
            u32 expected = min(255, initial + delta * (home ? 2 : 1));
            if (!B_POLISHED_MOVE_BALANCE)
                expected = min(255, initial + (initial < 100 ? 5 : initial < 200 ? 3 : 2) + home);
            EXPECT_EQ(GetMonData(&mon, MON_DATA_FRIENDSHIP), expected);
        }
}

TEST("Polished friendship negative events clamp without item ball or location bonuses")
{
    const u8 events[] = {FRIENDSHIP_EVENT_FAINT_SMALL, FRIENDSHIP_EVENT_FAINT_LARGE, FRIENDSHIP_EVENT_FAINT_FIELD_PSN};
    const u8 starts[] = {0, 1, 4, 99, 100, 159, 160, 200, 255};
    struct Pokemon mon;
    gMain.inBattle = FALSE;
    CreateRandomMon(&mon, SPECIES_CHANSEY, 10);
    for (u32 i = 0; i < ARRAY_COUNT(events); i++)
        for (u32 j = 0; j < ARRAY_COUNT(starts); j++)
        {
            SetFriendshipData(&mon, starts[j], TRUE, ITEM_SOOTHE_BELL, BALL_LUXURY);
            AdjustFriendship(&mon, events[i]);
            s32 penalty = i == 0 ? 1 : starts[j] >= (B_POLISHED_MOVE_BALANCE ? 160 : 200) ? 10 : 5;
            EXPECT_EQ(GetMonData(&mon, MON_DATA_FRIENDSHIP), min(255, max(0, starts[j] - penalty + (B_POLISHED_MOVE_BALANCE ? 0 : 2))));
        }
}

TEST("Polished friendship ignores eggs and preserves unmapped species behavior")
{
    struct Pokemon mon;
    u32 egg = TRUE;
    gMain.inBattle = FALSE;
    CreateRandomMon(&mon, SPECIES_CHANSEY, 10);
    SetFriendshipData(&mon, 160, FALSE, ITEM_NONE, BALL_POKE);
    SetMonData(&mon, MON_DATA_IS_EGG, &egg);
    AdjustFriendship(&mon, FRIENDSHIP_EVENT_GROW_LEVEL);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_FRIENDSHIP), 160);
    CreateRandomMon(&mon, SPECIES_MUDKIP, 10);
    SetFriendshipData(&mon, 160, FALSE, ITEM_NONE, BALL_POKE);
    EXPECT(!UsesPolishedFriendship(&mon));
    AdjustFriendship(&mon, FRIENDSHIP_EVENT_GROW_LEVEL);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_FRIENDSHIP), 163);
}

TEST("Polished friendship growth matches every level in all four source curves")
{
    const enum Species species[] = {SPECIES_PIKACHU, SPECIES_BULBASAUR, SPECIES_CHANSEY, SPECIES_MAGIKARP};
    const u32 growth[] = {GROWTH_MEDIUM_FAST, GROWTH_MEDIUM_SLOW, GROWTH_FAST, GROWTH_SLOW};
    struct Pokemon mon;
    gMain.inBattle = FALSE;
    SetConfig(CONFIG_POLISHED_SPECIES_DATA, TRUE);
    for (u32 i = 0; i < ARRAY_COUNT(species); i++)
    {
        EXPECT_EQ(GetSpeciesBaseData(species[i])->growthRate, growth[i]);
        CreateRandomMon(&mon, species[i], 1);
        for (u32 level = 1; level <= MAX_LEVEL; level++)
        {
            u32 exp = sPolishedGrowthVectors[i][level - 1];
            EXPECT_EQ(GetSpeciesExperienceAtLevel(species[i], level), exp);
            SetMonData(&mon, MON_DATA_EXP, &exp);
            EXPECT_EQ(GetLevelFromMonExp(&mon), level);
            EXPECT_EQ(GetLevelFromBoxMonExp(&mon.box), level);
            if (level > 1)
            {
                exp--;
                SetMonData(&mon, MON_DATA_EXP, &exp);
                EXPECT_EQ(GetLevelFromMonExp(&mon), level - 1);
            }
        }
    }
}

TEST("Polished friendship growth preserves native extra species and existing EXP records")
{
    struct Pokemon mon;
    gMain.inBattle = FALSE;
    SetConfig(CONFIG_POLISHED_SPECIES_DATA, FALSE);
    CreateRandomMon(&mon, SPECIES_CHANSEY, 1);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_EXP), 1);
    SetConfig(CONFIG_POLISHED_SPECIES_DATA, TRUE);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_EXP), 1);
    EXPECT_EQ(GetLevelFromMonExp(&mon), 1);
    EXPECT_EQ(GetSpeciesExperienceAtLevel(SPECIES_MUDKIP, 1), 1);
    CreateRandomMon(&mon, SPECIES_MUDKIP, 1);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_EXP), 1);
}

WILD_BATTLE_TEST("Polished friendship successful battle items synchronize the live and party values")
{
    enum Item item;
    PARAMETRIZE { item = ITEM_X_ATTACK; }
    PARAMETRIZE { item = ITEM_X_DEFENSE; }
    PARAMETRIZE { item = ITEM_X_SP_ATK; }
    PARAMETRIZE { item = ITEM_X_SP_DEF; }
    PARAMETRIZE { item = ITEM_X_SPEED; }
    PARAMETRIZE { item = ITEM_X_ACCURACY; }
    PARAMETRIZE { item = ITEM_GUARD_SPEC; }
    PARAMETRIZE { item = ITEM_DIRE_HIT; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Friendship(160); Item(ITEM_SOOTHE_BELL); }
        PLAYER(SPECIES_CHANSEY) { Friendship(75); }
        OPPONENT(SPECIES_WOBBUFFET) { Friendship(75); }
    } WHEN { TURN { USE_ITEM(player, item, partyIndex: 0); } }
    THEN {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][0];
        u32 expected = B_POLISHED_MOVE_BALANCE ? 161 : 160 + CalculateFriendshipBonuses(mon, X_ITEM_FRIENDSHIP_INCREASE, HOLD_EFFECT_FRIENDSHIP_UP);
        EXPECT_EQ(player->friendship, expected);
        EXPECT_EQ(GetMonData(mon, MON_DATA_FRIENDSHIP), expected);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_FRIENDSHIP), 75);
        EXPECT_EQ(opponent->friendship, 75);
    }
}

WILD_BATTLE_TEST("Polished friendship failed battle item effects do not reuse prior success")
{
    enum Item item;
    PARAMETRIZE { item = ITEM_GUARD_SPEC; }
    PARAMETRIZE { item = ITEM_DIRE_HIT; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Friendship(160); Item(ITEM_SOOTHE_BELL); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { USE_ITEM(player, item, partyIndex: 0); }
        TURN { USE_ITEM(player, item, partyIndex: 0); }
    } THEN {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][0];
        u32 expected = B_POLISHED_MOVE_BALANCE ? 161 : 160 + 2 * CalculateFriendshipBonuses(mon, X_ITEM_FRIENDSHIP_INCREASE, HOLD_EFFECT_FRIENDSHIP_UP);
        EXPECT_EQ(GetMonData(mon, MON_DATA_FRIENDSHIP), expected);
        EXPECT_EQ(player->friendship, expected);
    }
}

WILD_BATTLE_TEST("Polished friendship capped X item boosts award nothing after successful boosts")
{
    GIVEN {
        WITH_CONFIG(B_X_ITEMS_BUFF, GEN_7);
        PLAYER(SPECIES_WOBBUFFET) { Friendship(160); Item(ITEM_SOOTHE_BELL); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { USE_ITEM(player, ITEM_X_ATTACK); }
        TURN { USE_ITEM(player, ITEM_X_ATTACK); }
        TURN { USE_ITEM(player, ITEM_X_ATTACK); }
        TURN { USE_ITEM(player, ITEM_X_ATTACK); }
    } THEN {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][0];
        u32 expected = B_POLISHED_MOVE_BALANCE ? 163 : 160 + 4 * CalculateFriendshipBonuses(mon, X_ITEM_FRIENDSHIP_INCREASE, HOLD_EFFECT_FRIENDSHIP_UP);
        EXPECT_EQ(GetMonData(mon, MON_DATA_FRIENDSHIP), expected);
        EXPECT_EQ(player->friendship, expected);
        EXPECT_EQ(player->statStages[STAT_ATK], MAX_STAT_STAGE);
    }
}

WILD_BATTLE_TEST("Polished friendship boss adapter uses source class and skips fainted party members")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Friendship(160); Item(ITEM_SOOTHE_BELL); }
        PLAYER(SPECIES_CHANSEY) { Friendship(99); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE); } }
    THEN {
        u32 flags = gBattleTypeFlags;
        u32 trainer = TRAINER_BATTLE_PARAM.opponentA;
        gBattleTypeFlags |= BATTLE_TYPE_TRAINER;
        TRAINER_BATTLE_PARAM.opponentA = TRAINER_POLISHED_FALKNER;
        EXPECT(PolishedIsBossTrainer(TRAINER_POLISHED_FALKNER));
        EXPECT(!PolishedIsBossTrainer(TRAINER_POLISHED_SPROUT_LI));
        if (B_POLISHED_MOVE_BALANCE)
        {
            AdjustFriendship(&gParties[B_TRAINER_PLAYER][0], FRIENDSHIP_EVENT_LEAGUE_BATTLE);
            AdjustFriendship(&gParties[B_TRAINER_PLAYER][1], FRIENDSHIP_EVENT_LEAGUE_BATTLE);
            EXPECT_EQ(player->friendship, 161);
            EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_FRIENDSHIP), 99);
            TRAINER_BATTLE_PARAM.opponentA = TRAINER_POLISHED_SPROUT_LI;
            AdjustFriendship(&gParties[B_TRAINER_PLAYER][0], FRIENDSHIP_EVENT_LEAGUE_BATTLE);
            EXPECT_EQ(player->friendship, 161);
        }
        gBattleTypeFlags = flags;
        TRAINER_BATTLE_PARAM.opponentA = trainer;
    }
}

WILD_BATTLE_TEST("Polished friendship faint threshold is exactly thirty levels and skips an already fainted foe")
{
    u32 difference;
    bool32 foeAlive;
    PARAMETRIZE { difference = 29; foeAlive = TRUE; }
    PARAMETRIZE { difference = 30; foeAlive = TRUE; }
    PARAMETRIZE { difference = 31; foeAlive = TRUE; }
    PARAMETRIZE { difference = 30; foeAlive = FALSE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(10); Friendship(160); }
        OPPONENT(SPECIES_WOBBUFFET) { Level(10 + difference); }
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE); } }
    THEN {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][0];
        SetFriendshipData(mon, 160, FALSE, ITEM_NONE, BALL_POKE);
        if (!foeAlive) opponent->hp = 0;
        AdjustFriendshipOnBattleFaint(B_BATTLER_0);
        u32 penalty = B_POLISHED_MOVE_BALANCE ? (!foeAlive ? 0 : difference >= 30 ? 10 : 1) : difference >= 30 ? 5 : 1;
        EXPECT_EQ(GetMonData(mon, MON_DATA_FRIENDSHIP), 160 - penalty);
        if (B_POLISHED_MOVE_BALANCE) EXPECT_EQ(player->friendship, 160 - penalty);
    }
}

DOUBLE_BATTLE_TEST("Polished friendship preserves native optional doubles")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Friendship(160); }
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN { TURN { MOVE(playerLeft, MOVE_CELEBRATE); } }
    THEN {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][0];
        EXPECT(!UsesPolishedFriendship(mon));
        SetFriendshipData(mon, 160, FALSE, ITEM_NONE, BALL_POKE);
        AdjustFriendship(mon, FRIENDSHIP_EVENT_GROW_LEVEL);
        EXPECT_EQ(GetMonData(mon, MON_DATA_FRIENDSHIP), 163);
    }
}

WILD_BATTLE_TEST("Polished friendship excludes opponent parties special formats and active gimmicks")
{
    bool32 tera;
    PARAMETRIZE { tera = FALSE; }
    PARAMETRIZE { tera = TRUE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { TeraType(TYPE_NORMAL); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE, gimmick: tera ? GIMMICK_TERA : GIMMICK_NONE); } }
    THEN {
        EXPECT(!UsesPolishedFriendship(&gParties[B_TRAINER_OPPONENT_A][0]));
        EXPECT_EQ(UsesPolishedFriendship(&gParties[B_TRAINER_PLAYER][0]), B_POLISHED_MOVE_BALANCE && !tera);
        u32 flags = gBattleTypeFlags;
        const u32 formats[] = {BATTLE_TYPE_LINK, BATTLE_TYPE_FRONTIER, BATTLE_TYPE_TRAINER_HILL, BATTLE_TYPE_EREADER_TRAINER};
        for (u32 i = 0; i < ARRAY_COUNT(formats); i++)
        {
            gBattleTypeFlags = flags | formats[i];
            EXPECT(!UsesPolishedFriendship(&gParties[B_TRAINER_PLAYER][0]));
        }
        gBattleTypeFlags = flags;
    }
}

WILD_BATTLE_TEST("Polished friendship level one EXP awards cross several levels with one happiness event")
{
    GIVEN {
        WITH_CONFIG(POLISHED_SPECIES_DATA, TRUE);
        PLAYER(SPECIES_MAGIKARP) { Level(1); Friendship(160); Item(ITEM_SOOTHE_BELL); }
        OPPONENT(SPECIES_CATERPIE) { Level(5); HP(1); }
    } WHEN { TURN { MOVE(player, MOVE_SCRATCH); } }
    SCENE { EXPERIENCE_BAR(player); }
    THEN {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][0];
        EXPECT_GT(GetMonData(mon, MON_DATA_LEVEL), 2);
        EXPECT_EQ(GetLevelFromMonExp(mon), GetMonData(mon, MON_DATA_LEVEL));
        if (B_POLISHED_MOVE_BALANCE)
        {
            EXPECT_EQ(GetMonData(mon, MON_DATA_FRIENDSHIP), 161);
            EXPECT_EQ(player->friendship, 161);
        }
    }
}

SINGLE_BATTLE_TEST("Polished friendship applies the strong foe penalty through the live faint and replacement flow")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(10); HP(1); Friendship(160); Item(ITEM_SOOTHE_BELL); }
        PLAYER(SPECIES_CHANSEY) { Friendship(90); }
        OPPONENT(SPECIES_WOBBUFFET) { Level(40); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_SONIC_BOOM); SEND_OUT(player, 1); }
    } THEN {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][0];
        u32 expected = B_POLISHED_MOVE_BALANCE ? 150 : 160 + CalculateFriendshipBonuses(mon, -5, HOLD_EFFECT_FRIENDSHIP_UP);
        EXPECT_EQ(GetMonData(mon, MON_DATA_HP), 0);
        EXPECT_EQ(GetMonData(mon, MON_DATA_FRIENDSHIP), expected);
        EXPECT_EQ(player->friendship, 90);
    }
}

SINGLE_BATTLE_TEST("Polished friendship lethal recoil skips happiness loss when both active Pokemon faint")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(10); HP(1); Friendship(160); Item(ITEM_SOOTHE_BELL); }
        PLAYER(SPECIES_CHANSEY) { Friendship(90); }
        OPPONENT(SPECIES_WOBBUFFET) { Level(40); HP(1); }
        OPPONENT(SPECIES_CHANSEY);
    } WHEN {
        TURN { MOVE(player, MOVE_DOUBLE_EDGE); SEND_OUT(player, 1); SEND_OUT(opponent, 1); }
    } THEN {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][0];
        EXPECT_EQ(GetMonData(mon, MON_DATA_HP), 0);
        if (B_POLISHED_MOVE_BALANCE) EXPECT_EQ(GetMonData(mon, MON_DATA_FRIENDSHIP), 160);
        EXPECT_EQ(player->friendship, 90);
    }
}
