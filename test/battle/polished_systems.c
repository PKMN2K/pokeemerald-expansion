#include "global.h"
#include "test/battle.h"
#include "event_data.h"
#include "wild_encounter.h"
#include "polished_capture.h"
#include "polished_rewards.h"
#include "pokedex.h"
#include "battle_util.h"
#include "constants/battle_string_ids.h"
#include "polished_trainers.h"
#include "data.h"
#include "random.h"
#include "trainer_util.h"
#include "constants/opponents.h"
#include "constants/layouts.h"
#include "overworld.h"

#if !IS_FRLG
TEST("Polished systems trainer generation preserves source slots stats gender and friendship")
{
    if (!B_POLISHED_MOVE_BALANCE) return;
    static const u16 trainers[] = {TRAINER_POLISHED_LYRA_CHIKORITA, TRAINER_POLISHED_LYRA_CYNDAQUIL,
        TRAINER_POLISHED_LYRA_TOTODILE, TRAINER_POLISHED_RIVAL_CHIKORITA, TRAINER_POLISHED_RIVAL_CYNDAQUIL,
        TRAINER_POLISHED_RIVAL_TOTODILE, TRAINER_POLISHED_MIKEY, TRAINER_POLISHED_DON, TRAINER_POLISHED_JOEY,
        TRAINER_POLISHED_WADE, TRAINER_POLISHED_FINCH, TRAINER_POLISHED_SPROUT_CHOW, TRAINER_POLISHED_SPROUT_NICO,
        TRAINER_POLISHED_SPROUT_EDMOND, TRAINER_POLISHED_SPROUT_JIN, TRAINER_POLISHED_SPROUT_TROY,
        TRAINER_POLISHED_SPROUT_NEAL, TRAINER_POLISHED_SPROUT_LI, TRAINER_POLISHED_ROD, TRAINER_POLISHED_ABE,
        TRAINER_POLISHED_FALKNER};
    static const u8 counts[] = {1, 1, 1, 2, 2, 2, 2, 2, 1, 4, 2, 3, 3, 3, 1, 2, 2, 3, 2, 1, 3};
    static const struct Trainer trainer = {.trainerName = _("TEST")};
    for (u32 i = 0; i < ARRAY_COUNT(trainers); i++)
    {
        struct TrainerGenerator generator;
        MakeTrainerGenerator(&generator, &trainer);
        for (u32 slot = 0; slot < counts[i]; slot++)
        {
            struct Pokemon mon;
            const struct PolishedTrainerAttributes *attrs = PolishedGetTrainerAttributes(trainers[i], slot);
            EXPECT(attrs != NULL);
            if (attrs == NULL) continue;
            struct TrainerMon definition = {.species = attrs->species, .lvl = 50, .moves = {MOVE_TACKLE}};
            EXPECT(PolishedGenerateTrainerMon(&mon, &definition, &generator, trainers[i], slot));
            EXPECT_EQ(GetMonData(&mon, MON_DATA_SPECIES), attrs->species);
            EXPECT_EQ(GetMonData(&mon, MON_DATA_ABILITY_NUM), attrs->abilitySlot);
            EXPECT_EQ(GetMonData(&mon, MON_DATA_IVS), attrs->iv);
            EXPECT_EQ(GetMonData(&mon, MON_DATA_HP_EV), attrs->ev[0]);
            EXPECT_EQ(GetMonData(&mon, MON_DATA_SPEED_EV), attrs->ev[5]);
            EXPECT_EQ(GetNature(&mon), attrs->nature);
            EXPECT_EQ(GetMonData(&mon, MON_DATA_FRIENDSHIP), 160);
            EXPECT_EQ(GetMonData(&mon, MON_DATA_POKEBALL), BALL_POKE);
            EXPECT_EQ(GetMonGender(&mon), attrs->gender == TRAINER_MON_FEMALE ? MON_FEMALE : MON_MALE);
        }
        EXPECT(PolishedGetTrainerAttributes(trainers[i], counts[i]) == NULL);
    }
    EXPECT(PolishedGetTrainerAttributes(TRAINER_NONE, 0) == NULL);
}

TEST("Polished systems trainer ability selection uses source species byte trainer ID and one based slot")
{
    // Lyra1_2/Cyndaquil: 1 xor 2 xor 155 -> slot 0 (Blaze).
    // Rival0_1/Rattata: 1 xor 1 xor 19 -> slot 1 (Guts).
    EXPECT_EQ(PolishedGetTrainerAttributes(TRAINER_POLISHED_LYRA_CYNDAQUIL, 0)->abilitySlot, 0);
    EXPECT_EQ(PolishedGetTrainerAttributes(TRAINER_POLISHED_RIVAL_CHIKORITA, 0)->abilitySlot, 1);
    EXPECT_EQ(PolishedGetTrainerAttributes(TRAINER_POLISHED_RIVAL_CHIKORITA, 1)->abilitySlot, 1);
    EXPECT_EQ(PolishedGetTrainerAttributes(TRAINER_POLISHED_JOEY, 0)->nature, NATURE_ADAMANT);
    EXPECT_EQ(PolishedGetTrainerAttributes(TRAINER_POLISHED_FINCH, 0)->nature, NATURE_RELAXED);
    EXPECT_EQ(PolishedGetTrainerAttributes(TRAINER_POLISHED_FALKNER, 0)->ev[0], 32);
    EXPECT_EQ(PolishedGetTrainerAttributes(TRAINER_POLISHED_FALKNER, 0)->ev[1], 0);
    EXPECT_EQ(PolishedGetTrainerBaseMoney(TRAINER_POLISHED_ROD), 6);
    EXPECT_EQ(PolishedGetTrainerBaseMoney(TRAINER_POLISHED_SPROUT_CHOW), 8);
    EXPECT_EQ(PolishedGetTrainerBaseMoney(TRAINER_POLISHED_SPROUT_LI), 10);
    EXPECT_EQ(PolishedGetTrainerBaseMoney(TRAINER_POLISHED_RIVAL_CHIKORITA), 15);
    EXPECT_EQ(PolishedGetTrainerBaseMoney(TRAINER_POLISHED_FALKNER), 25);
    EXPECT_EQ(PolishedGetTrainerItems(TRAINER_POLISHED_FINCH)[0], ITEM_FULL_HEAL);
    EXPECT_EQ(PolishedGetTrainerItems(TRAINER_POLISHED_FINCH)[1], ITEM_MAX_POTION);
    EXPECT_EQ(PolishedGetTrainerItems(TRAINER_POLISHED_FINCH)[2], ITEM_NONE);
}
#endif

TEST("Polished systems Heavy Ball preserves every inclusive source weight boundary")
{
    static const u16 weights[] = {0, 1023, 1024, 2047, 2048, 2303, 2304, 3072, 3327, 3328, 4096, 4351, 4352, 9999};
    static const s8 changes[] = {-20, -20, 0, 0, 0, 0, 20, 20, 20, 30, 30, 30, 40, 40};
    for (u32 i = 0; i < ARRAY_COUNT(weights); i++)
        for (s32 rate = 0; rate < 256; rate++)
        {
            s32 expected = rate + changes[i];
            if (expected < 0) expected = 1;
            EXPECT_EQ(PolishedHeavyBallCatchRate(rate, weights[i]), min(255, expected));
        }
}

TEST("Polished systems critical capture uses inclusive ownership bands and a byte threshold")
{
    static const u16 caught[] = {0, 30, 31, 100, 101, 150, 151, 200, 201, 250, 251, 289};
    static const u8 ranks[] = {0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5};
    for (u32 i = 0; i < ARRAY_COUNT(caught); i++)
        for (u32 odds = 1; odds < 256; odds++)
        {
            EXPECT_EQ(PolishedCriticalCaptureThreshold(odds, caught[i], FALSE), odds * ranks[i] / 12);
            EXPECT_EQ(PolishedCriticalCaptureThreshold(odds, caught[i], TRUE), odds * ranks[i] / 6);
        }
}

TEST("Polished systems ball modifiers and HP compression preserve source integer boundaries")
{
    struct PolishedCaptureContext c = {.hp = 101, .maxHP = 101, .catchRate = 150, .userLevel = 20, .targetLevel = 5};
    static const enum PokeBall balls[] = {BALL_POKE, BALL_GREAT, BALL_ULTRA, BALL_PARK, BALL_SAFARI, BALL_LEVEL, BALL_NEST, BALL_QUICK, BALL_MASTER};
    static const u16 expected[] = {50, 74, 100, 74, 100, 200, 255, 250, 255};
    for (u32 i = 0; i < ARRAY_COUNT(balls); i++)
    {
        c.ball = balls[i];
        EXPECT_EQ(PolishedCaptureOdds(&c), expected[i]);
    }
    c.ball = BALL_POKE; c.maxHP = 256; c.hp = 1; c.catchRate = 45;
    EXPECT_EQ(PolishedCaptureOdds(&c), 44);
    c.status = STATUS1_FREEZE;
    EXPECT_EQ(PolishedCaptureOdds(&c), 112);
    c.status = STATUS1_POISON;
    EXPECT_EQ(PolishedCaptureOdds(&c), 67);
    c.maxHP = c.hp = 1; c.catchRate = 1; c.status = 0;
    EXPECT_EQ(PolishedCaptureOdds(&c), 1);
    c.maxHP = 65535; c.hp = 1; c.catchRate = 255; c.status = STATUS1_FREEZE;
    c.ball = BALL_LEVEL; c.userLevel = 100; c.targetLevel = 1;
    EXPECT_EQ(PolishedCaptureOdds(&c), 255);
}

TEST("Polished systems Timer Ball retains byte overflow and the saturated turn counter")
{
    struct PolishedCaptureContext c = {.ball = BALL_TIMER, .hp = 100, .maxHP = 100, .catchRate = 150};
    static const u16 turns[] = {0, 1, 9, 10, 81, 82, 83, 85, 86, 100, 254, 255, 65535};
    static const u16 expected[] = {50, 65, 185, 200, 200, 1, 15, 45, 60, 200, 20, 20, 20};
    for (u32 i = 0; i < ARRAY_COUNT(turns); i++)
    {
        c.completedTurns = turns[i];
        EXPECT_EQ(PolishedCaptureOdds(&c), expected[i]);
    }
}

TEST("Polished systems EXP boosts precede one combined distribution division")
{
    struct PolishedExpContext c = {.baseExp = 53, .defeatedLevel = 10, .participants = 2, .holders = 1, .participant = TRUE, .holder = TRUE};
    EXPECT_EQ(PolishedExperience(&c), 56);
    c.traded = TRUE; c.trainer = TRUE; c.luckyEgg = TRUE;
    EXPECT_EQ(PolishedExperience(&c), 189);
    c.holder = FALSE;
    EXPECT_EQ(PolishedExperience(&c), 63);
    c.participant = FALSE;
    EXPECT_EQ(PolishedExperience(&c), 0);
    c.holder = TRUE; c.participants = 0; c.holders = 3;
    EXPECT_EQ(PolishedExperience(&c), 84);
    c.baseExp = 1; c.defeatedLevel = 1;
    EXPECT_EQ(PolishedExperience(&c), 1);
}

TEST("Polished systems Safari escape uses raw low byte Speed before bait and anger")
{
    EXPECT_EQ(PolishedSafariFleeThreshold(127, 30, 0, 0), 254);
    EXPECT_EQ(PolishedSafariFleeThreshold(128, 30, 1, 0), 256);
    EXPECT_EQ(PolishedSafariFleeThreshold(256, 30, 0, 0), 0);
    EXPECT_EQ(PolishedSafariFleeThreshold(257, 30, 0, 0), 2);
    EXPECT_EQ(PolishedSafariFleeThreshold(0, 0, 5, 0), 256);
    EXPECT_EQ(PolishedSafariFleeThreshold(100, 30, 1, 0), 50);
    EXPECT_EQ(PolishedSafariFleeThreshold(100, 30, 0, 1), 255);
    EXPECT_EQ(PolishedSafariFleeThreshold(63, 30, 0, 1), 252);
}

SINGLE_BATTLE_TEST("Polished Safari bait and rock consume every rejected draw and one accepted draw")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_CLEFFA);
    } WHEN { TURN {} }
    THEN {
        SeedRng(0); // First roll is zero: one turn of bait.
        u32 before = gRngValue.ctr;
        PolishedSafariBaitOrRock(FALSE);
        EXPECT_EQ(gBattleStruct->safariPkblThrowCounter, 1);
        EXPECT_EQ(gRngValue.ctr - before, 1);

        SeedRng(5); // Low three bits are 6, 5, 7, 4: five turns of anger.
        before = gRngValue.ctr;
        gBattleStruct->safariGoNearCounter = 252;
        PolishedSafariBaitOrRock(TRUE);
        EXPECT_EQ(gBattleStruct->safariGoNearCounter, 255);
        EXPECT_EQ(gBattleStruct->safariPkblThrowCounter, 0);
        EXPECT_EQ(gRngValue.ctr - before, 4);
    }
}

SINGLE_BATTLE_TEST("Polished systems Safari bait can reach zero rate and anger expiry restores the base")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_CLEFFA);
    } WHEN { TURN {} }
    THEN {
        for (u32 i = 0; i < 8; i++) PolishedSafariBaitOrRock(FALSE);
        EXPECT_EQ(gBattleStruct->polishedCatchRate, 0);
        EXPECT_GT(gBattleStruct->safariPkblThrowCounter, 0);
        EXPECT_EQ(gBattleStruct->safariGoNearCounter, 0);
        gBattleStruct->safariPkblThrowCounter = 1;
        EXPECT_EQ(PolishedSafariReaction(), B_MSG_MON_EATING);
        EXPECT_EQ(gBattleStruct->safariPkblThrowCounter, 0);
        EXPECT_EQ(gBattleStruct->polishedCatchRate, 0); // Eating expiry does not reset it.
        PolishedSafariBaitOrRock(TRUE);
        EXPECT_EQ(gBattleStruct->polishedCatchRate, 0);
        gBattleStruct->safariGoNearCounter = 1;
        EXPECT_EQ(PolishedSafariReaction(), B_MSG_MON_ANGRY);
        EXPECT_EQ(gBattleStruct->polishedCatchRate, 150);
        PolishedSafariBaitOrRock(TRUE);
        EXPECT_EQ(gBattleStruct->polishedCatchRate, 255);
        gBattleStruct->safariGoNearCounter = 254;
        for (u32 i = 0; i < 5; i++) PolishedSafariBaitOrRock(TRUE);
        EXPECT_EQ(gBattleStruct->safariGoNearCounter, 255);
    }
}

TEST("Polished systems Repeat Ball shares cosmetic ownership and separates regional variants")
{
    if (!B_POLISHED_MOVE_BALANCE) return;
    ResetPokedex();
    PolishedRecordCaughtSpecies(SPECIES_UNOWN_B);
    EXPECT(PolishedWasCaught(SPECIES_UNOWN));
    EXPECT(PolishedWasCaught(SPECIES_UNOWN_C));
    EXPECT(!PolishedWasCaught(SPECIES_RATTATA));
    PolishedRecordCaughtSpecies(SPECIES_RATTATA_ALOLA);
    EXPECT(PolishedWasCaught(SPECIES_RATTATA_ALOLA));
    EXPECT(!PolishedWasCaught(SPECIES_RATTATA));
    PolishedRecordCaughtSpecies(SPECIES_RATTATA);
    EXPECT(PolishedWasCaught(SPECIES_RATTATA));
    EXPECT(PolishedWasCaught(SPECIES_RATTATA_ALOLA));
    // The reserved-byte union must not move any later save field.
    EXPECT_EQ(sizeof(gSaveBlock2Ptr->pokedex.filler), 0x68);
    EXPECT_EQ(sizeof(gSaveBlock2Ptr->pokedex), 0x78);
    ResetPokedex();
    EXPECT(!PolishedWasCaught(SPECIES_RATTATA_ALOLA));
}

WILD_BATTLE_TEST("Polished systems capture floors the ball multiplier before the catch rate")
{
    u32 odds;
    GIVEN {
        WITH_CONFIG(B_MISSING_BADGE_CATCH_MALUS, GEN_7);
        WITH_CONFIG(B_LOW_LEVEL_CATCH_BONUS, GEN_7);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_CLEFFA) { Level(50); MaxHP(101); HP(101); }
    } WHEN {
        TURN { USE_ITEM(player, ITEM_GREAT_BALL); }
    } SCENE {
        CATCHING_CHANCE(&odds);
    } THEN {
        EXPECT_EQ(odds, B_POLISHED_MOVE_BALANCE ? 74 : 75);
    }
}

WILD_BATTLE_TEST("Polished systems Nest Ball uses the source denominator of five")
{
    u32 odds;
    GIVEN {
        WITH_CONFIG(B_MISSING_BADGE_CATCH_MALUS, GEN_7);
        WITH_CONFIG(B_LOW_LEVEL_CATCH_BONUS, GEN_7);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_CLEFFA) { Level(26); MaxHP(100); HP(100); }
    } WHEN {
        TURN { USE_ITEM(player, ITEM_NEST_BALL); }
    } SCENE {
        CATCHING_CHANCE(&odds);
    } THEN {
        EXPECT_EQ(odds, B_POLISHED_MOVE_BALANCE ? 150 : 75);
    }
}

WILD_BATTLE_TEST("Polished systems Level Ball requires a strictly greater quarter level")
{
    u32 odds;
    GIVEN {
        WITH_CONFIG(B_MISSING_BADGE_CATCH_MALUS, GEN_7);
        WITH_CONFIG(B_LOW_LEVEL_CATCH_BONUS, GEN_7);
        PLAYER(SPECIES_WOBBUFFET) { Level(20); }
        OPPONENT(SPECIES_CLEFFA) { Level(5); MaxHP(100); HP(100); }
    } WHEN {
        TURN { USE_ITEM(player, ITEM_LEVEL_BALL); }
    } SCENE {
        CATCHING_CHANCE(&odds);
    } THEN {
        EXPECT_EQ(odds, B_POLISHED_MOVE_BALANCE ? 200 : 400);
    }
}

WILD_BATTLE_TEST("Polished systems capture ignores native low level and missing badge modifiers")
{
    u32 odds;
    GIVEN {
        WITH_CONFIG(B_MISSING_BADGE_CATCH_MALUS, GEN_9);
        WITH_CONFIG(B_LOW_LEVEL_CATCH_BONUS, GEN_9);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_CLEFFA) { Level(10); MaxHP(100); HP(100); }
    } WHEN {
        TURN { USE_ITEM(player, ITEM_POKE_BALL); }
    } SCENE {
        CATCHING_CHANCE(&odds);
    } THEN {
        EXPECT_EQ(odds, B_POLISHED_MOVE_BALANCE ? 50 : 80);
    }
}

WILD_BATTLE_TEST("Polished systems Heavy Ball changes persist into subsequent ordinary throws")
{
    u32 a, b, c;
    GIVEN {
        WITH_CONFIG(B_MISSING_BADGE_CATCH_MALUS, GEN_7);
        WITH_CONFIG(B_LOW_LEVEL_CATCH_BONUS, GEN_7);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_CLEFFA) { Level(50); MaxHP(100); HP(100); Moves(MOVE_SPLASH); }
    } WHEN {
        TURN { USE_ITEM(player, ITEM_HEAVY_BALL); }
        TURN { USE_ITEM(player, ITEM_HEAVY_BALL); }
        TURN { USE_ITEM(player, ITEM_POKE_BALL); }
    } SCENE { CATCHING_CHANCE(&a); CATCHING_CHANCE(&b); CATCHING_CHANCE(&c); }
    THEN {
        EXPECT_EQ(a, 43);
        EXPECT_EQ(b, B_POLISHED_MOVE_BALANCE ? 36 : 43);
        EXPECT_EQ(c, B_POLISHED_MOVE_BALANCE ? 36 : 50);
    }
}

WILD_BATTLE_TEST("Polished systems Friend Ball delivers source friendship with capture EXP")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(50); }
        OPPONENT(SPECIES_CLEFFA) { Level(50); HP(1); }
    } WHEN {
        TURN { USE_ITEM(player, ITEM_FRIEND_BALL, WITH_RNG(B_POLISHED_MOVE_BALANCE ? RNG_POLISHED_CAPTURE_SHAKE : RNG_BALLTHROW_SHAKE, 0)); }
    } SCENE { EXPERIENCE_BAR(player); }
    THEN {
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_SPECIES), SPECIES_CLEFFA);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_FRIENDSHIP), B_POLISHED_MOVE_BALANCE ? 200 : 150);
    }
}

WILD_BATTLE_TEST("Polished systems EXP uses the source yield and unscaled default")
{
    s32 gained;
    GIVEN {
        WITH_CONFIG(B_SCALED_EXP, GEN_3);
        WITH_CONFIG(B_UNEVOLVED_EXP_MULTIPLIER, GEN_3);
        PLAYER(SPECIES_WOBBUFFET) { Level(20); }
        OPPONENT(SPECIES_CATERPIE) { Level(10); HP(1); }
    } WHEN { TURN { MOVE(player, MOVE_SCRATCH); } }
    SCENE { EXPERIENCE_BAR(player, captureGainedExp: &gained); }
    THEN { EXPECT_EQ(gained, B_POLISHED_MOVE_BALANCE ? 75 : GetSpeciesBaseData(SPECIES_CATERPIE)->expYield * 10 / 7); }
}

WILD_BATTLE_TEST("Polished systems a different OT name alone does not boost source EXP")
{
    s32 gained;
    GIVEN {
        WITH_CONFIG(B_SCALED_EXP, GEN_3);
        WITH_CONFIG(B_UNEVOLVED_EXP_MULTIPLIER, GEN_3);
        PLAYER(SPECIES_WOBBUFFET) { Level(20); OTName("OTHER"); Item(ITEM_LUCKY_EGG); }
        OPPONENT(SPECIES_CATERPIE) { Level(10); HP(1); }
    } WHEN { TURN { MOVE(player, MOVE_SCRATCH); } }
    SCENE { EXPERIENCE_BAR(player, captureGainedExp: &gained); }
    THEN { EXPECT_EQ(gained, B_POLISHED_MOVE_BALANCE ? 112 : GetSpeciesBaseData(SPECIES_CATERPIE)->expYield * 10 / 7 * 3 / 2 * 3 / 2); }
}

WILD_BATTLE_TEST("Polished systems EXP combines participant and held share fractions before rounding")
{
    s32 gained;
    GIVEN {
        WITH_CONFIG(B_SCALED_EXP, GEN_3);
        // Keep the native control on its current full-participation rules.
        // The source path must still combine the two fractional shares.
        WITH_CONFIG(B_SPLIT_EXP, GEN_9);
        WITH_CONFIG(B_UNEVOLVED_EXP_MULTIPLIER, GEN_3);
        PLAYER(SPECIES_WOBBUFFET) { Level(20); Item(ITEM_EXP_SHARE); }
        PLAYER(SPECIES_WOBBUFFET) { Level(20); }
        OPPONENT(SPECIES_CATERPIE) { Level(10); HP(1); Moves(MOVE_SPLASH); }
    } WHEN {
        TURN { SWITCH(player, 1); }
        TURN { SWITCH(player, 0); }
        TURN { MOVE(player, MOVE_SCRATCH); }
    } SCENE { EXPERIENCE_BAR(player, captureGainedExp: &gained); }
    THEN {
        u32 base = GetSpeciesBaseData(SPECIES_CATERPIE)->expYield * 10 / 7;
        EXPECT_EQ(gained, B_POLISHED_MOVE_BALANCE ? 56 : base);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_EXP), gExperienceTables[GetSpeciesBaseData(SPECIES_WOBBUFFET)->growthRate][20] + (B_POLISHED_MOVE_BALANCE ? 18 : base));
    }
}

WILD_BATTLE_TEST("Polished systems level 100 share holders gain EVs from the active power item")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(100); Item(ITEM_POWER_WEIGHT); }
        PLAYER(SPECIES_WOBBUFFET) { Level(100); Item(ITEM_EXP_SHARE); }
        PLAYER(SPECIES_WOBBUFFET) { Level(100); Item(ITEM_EXP_SHARE); HP(0); }
        OPPONENT(SPECIES_CATERPIE) { Level(10); HP(1); }
    } WHEN { TURN { MOVE(player, MOVE_SCRATCH); } }
    THEN {
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_HP_EV), 9);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_HP_EV), B_POLISHED_MOVE_BALANCE ? 9 : 1);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][2], MON_DATA_HP_EV), 0);
    }
}

WILD_BATTLE_TEST("Polished systems transformed captures retain the original source catch rate")
{
    u32 odds;
    GIVEN {
        WITH_CONFIG(B_TRANSFORM_CATCH_RATE, GEN_3);
        WITH_CONFIG(B_MISSING_BADGE_CATCH_MALUS, GEN_7);
        WITH_CONFIG(B_LOW_LEVEL_CATCH_BONUS, GEN_7);
        PLAYER(SPECIES_CLEFFA);
        OPPONENT(SPECIES_DITTO) { Ability(ABILITY_IMPOSTER); MaxHP(100); HP(100); }
    } WHEN { TURN { USE_ITEM(player, ITEM_POKE_BALL); } }
    SCENE { CATCHING_CHANCE(&odds); }
    THEN { EXPECT_EQ(odds, B_POLISHED_MOVE_BALANCE ? 11 : 50); }
}

WILD_BATTLE_TEST("Polished systems transformed rewards retain the original source EXP and EV yields")
{
    s32 gained;
    GIVEN {
        WITH_CONFIG(B_TRANSFORM_BATTLE_REWARDS, GEN_3);
        WITH_CONFIG(B_SCALED_EXP, GEN_3);
        WITH_CONFIG(B_UNEVOLVED_EXP_MULTIPLIER, GEN_3);
        PLAYER(SPECIES_CLEFFA) { Level(20); }
        OPPONENT(SPECIES_DITTO) { Level(10); Ability(ABILITY_IMPOSTER); HP(1); }
    } WHEN { TURN { MOVE(player, MOVE_SCRATCH); } }
    SCENE { EXPERIENCE_BAR(player, captureGainedExp: &gained); }
    THEN {
        EXPECT_EQ(gained, B_POLISHED_MOVE_BALANCE ? 87 : GetSpeciesBaseData(SPECIES_CLEFFA)->expYield * 10 / 7);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_HP_EV), B_POLISHED_MOVE_BALANCE ? 1 : 0);
    }
}

WILD_BATTLE_TEST("Polished systems capture does not restore a knocked off held item")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Attack(1); Level(50); }
        OPPONENT(SPECIES_CLEFFA) { Level(50); MaxHP(400); HP(400); Defense(200); Item(ITEM_SMOKE_BALL); Moves(MOVE_SPLASH); }
    } WHEN {
        TURN { MOVE(player, MOVE_KNOCK_OFF); }
        TURN { USE_ITEM(player, ITEM_MASTER_BALL); }
    } THEN {
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_SPECIES), SPECIES_CLEFFA);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_HELD_ITEM), B_POLISHED_MOVE_BALANCE ? ITEM_NONE : ITEM_SMOKE_BALL);
    }
}

WILD_BATTLE_TEST("Polished systems owned captures do not inherit the native forced critical display")
{
    GIVEN {
        GetSetPokedexFlag(NATIONAL_DEX_CATERPIE, FLAG_SET_CAUGHT);
        WITH_CONFIG(B_CRITICAL_CAPTURE_IF_OWNED, GEN_9);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_CATERPIE);
    } WHEN {
        TURN { USE_ITEM(player, ITEM_POKE_BALL,
            WITH_RNG(B_POLISHED_MOVE_BALANCE ? RNG_POLISHED_CAPTURE_SHAKE : RNG_BALLTHROW_SHAKE, 0)); }
    } SCENE {
        if (B_POLISHED_MOVE_BALANCE)
        {
            ANIMATION(ANIM_TYPE_SPECIAL, B_ANIM_BALL_THROW);
            NOT ANIMATION(ANIM_TYPE_SPECIAL, B_ANIM_CRITICAL_CAPTURE_THROW);
        }
        else
        {
            ANIMATION(ANIM_TYPE_SPECIAL, B_ANIM_CRITICAL_CAPTURE_THROW);
            NOT ANIMATION(ANIM_TYPE_SPECIAL, B_ANIM_BALL_THROW);
        }
    } THEN {
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_SPECIES), SPECIES_CATERPIE);
    }
}

SINGLE_BATTLE_TEST("Polished systems Safari rewards exclude the unsent lead and retain held shares")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        PLAYER(SPECIES_WOBBUFFET) { Item(ITEM_EXP_SHARE); }
        OPPONENT(SPECIES_CATERPIE) { Level(10); }
    } WHEN { TURN {} }
    THEN {
        u32 originalFlags = gBattleTypeFlags;
        gBattleTypeFlags = BATTLE_TYPE_SAFARI;
        EXPECT_EQ(PolishedPrepareExperience(B_BATTLER_1), 1);
        EXPECT_EQ(gBattleStruct->expSentInMons, 0);
        EXPECT_EQ(gBattleStruct->expGetterMonId, 1);
        EXPECT_EQ(PolishedBattleExperience(0, B_BATTLER_1), 0);
        EXPECT_EQ(PolishedBattleExperience(1, B_BATTLER_1), 75);
        gBattleTypeFlags = originalFlags;
    }
}

SINGLE_BATTLE_TEST("Polished systems active Pokerus doubles the power item before stat and total EV caps")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Item(ITEM_POWER_WEIGHT); }
        OPPONENT(SPECIES_CATERPIE);
    } WHEN { TURN {} }
    THEN {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][0];
        u8 value = 0x11;
        SetMonData(mon, MON_DATA_POKERUS, &value);
        PolishedGainBattleEVs(mon, SPECIES_CATERPIE);
        EXPECT_EQ(GetMonData(mon, MON_DATA_HP_EV), 18);
        value = 0x10; // Cured Pokerus does not double source EVs.
        SetMonData(mon, MON_DATA_POKERUS, &value);
        PolishedGainBattleEVs(mon, SPECIES_CATERPIE);
        EXPECT_EQ(GetMonData(mon, MON_DATA_HP_EV), 27);
        value = 250;
        SetMonData(mon, MON_DATA_HP_EV, &value);
        PolishedGainBattleEVs(mon, SPECIES_CATERPIE);
        EXPECT_EQ(GetMonData(mon, MON_DATA_HP_EV), 252);
        value = 250;
        SetMonData(mon, MON_DATA_HP_EV, &value);
        value = 252;
        SetMonData(mon, MON_DATA_ATK_EV, &value);
        value = 7;
        SetMonData(mon, MON_DATA_DEF_EV, &value);
        PolishedGainBattleEVs(mon, SPECIES_CATERPIE);
        EXPECT_EQ(GetMonData(mon, MON_DATA_HP_EV), 251);
    }
}

WILD_BATTLE_TEST("Polished Dusk Ball triples catch probability in Dark Cave and uses normal odds in Sprout Tower")
{
    ASSUME(B_POLISHED_MOVE_BALANCE);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_RAIKOU);
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); MOVE(opponent, MOVE_SPLASH); }
    } THEN {
        u16 savedLayout = gMapHeader.mapLayoutId;
        enum Item savedItem = gLastUsedItem;
        gMapHeader.mapLayoutId = LAYOUT_POLISHED_SPROUT_TOWER_1F;
        gLastUsedItem = ITEM_POKE_BALL;
        u32 base = GetPolishedCaptureOdds(B_BATTLER_0, B_BATTLER_1);
        gLastUsedItem = ITEM_DUSK_BALL;
        EXPECT_EQ(GetPolishedCaptureOdds(B_BATTLER_0, B_BATTLER_1), base);
        gMapHeader.mapLayoutId = LAYOUT_POLISHED_NEW_BARK_TOWN;
        EXPECT_EQ(GetPolishedCaptureOdds(B_BATTLER_0, B_BATTLER_1), base);
        gMapHeader.mapLayoutId = LAYOUT_POLISHED_DARK_CAVE_VIOLET_ENTRANCE;
        EXPECT_EQ(GetPolishedCaptureOdds(B_BATTLER_0, B_BATTLER_1), 3 * base);
        gMapHeader.mapLayoutId = savedLayout;
        gLastUsedItem = savedItem;
    }
}
