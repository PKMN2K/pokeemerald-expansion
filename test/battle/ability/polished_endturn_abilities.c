#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished endturn Shed Skin uses the source 77 of 256 byte outcomes")
{
    u32 config = GEN_9;
    PARAMETRIZE { config = GEN_3; }
    PARAMETRIZE { config = GEN_4; }
    PARAMETRIZE { config = GEN_9; }
    PASSES_RANDOMLY(B_POLISHED_MOVE_BALANCE ? 77 : config == GEN_4 ? 30 : 1,
        B_POLISHED_MOVE_BALANCE ? 256 : config == GEN_4 ? 100 : 3, RNG_SHED_SKIN);
    GIVEN {
        WITH_CONFIG(B_ABILITY_TRIGGER_CHANCE, config);
        PLAYER(SPECIES_ARBOK) { Ability(ABILITY_SHED_SKIN); Status1(STATUS1_POISON); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN { TURN { MOVE(player, MOVE_SPLASH); } }
    SCENE { ABILITY_POPUP(player, ABILITY_SHED_SKIN); }
    THEN { EXPECT_EQ(player->status1, STATUS1_NONE); }
}

SINGLE_BATTLE_TEST("Polished endturn Shed Skin uses its full byte distribution on either side")
{
    struct BattlePokemon *holder = player;
    u32 status = STATUS1_POISON;
    for (u32 side = 0; side < 2; side++)
        for (u32 sleep = 0; sleep < 2; sleep++)
            PARAMETRIZE { holder = side ? opponent : player; status = sleep ? STATUS1_SLEEP_TURN(3) : STATUS1_POISON; }
    // Endturn runs after the move-specific WITH_RNG scope. The runner's
    // tagged distribution traverses every byte outcome at this boundary.
    PASSES_RANDOMLY(B_POLISHED_MOVE_BALANCE ? 77 : 1,
        B_POLISHED_MOVE_BALANCE ? 256 : 3, RNG_SHED_SKIN);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(holder == player ? ABILITY_SHED_SKIN : ABILITY_NONE); Status1(holder == player ? status : STATUS1_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(holder == opponent ? ABILITY_SHED_SKIN : ABILITY_NONE); Status1(holder == opponent ? status : STATUS1_NONE); }
    } WHEN {
        TURN { MOVE(holder, MOVE_SPLASH); }
    } SCENE { ABILITY_POPUP(holder, ABILITY_SHED_SKIN); }
    THEN { EXPECT_EQ(holder->status1, STATUS1_NONE); }
}

SINGLE_BATTLE_TEST("Polished endturn Shed Skin is suppressed by Neutralizing Gas")
{
    PASSES_RANDOMLY(1, 1, RNG_SHED_SKIN);
    GIVEN {
        PLAYER(SPECIES_ARBOK) { Ability(ABILITY_SHED_SKIN); Status1(STATUS1_POISON); }
        OPPONENT(SPECIES_WEEZING) { Ability(ABILITY_NEUTRALIZING_GAS); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); }
    } SCENE { NOT ABILITY_POPUP(player, ABILITY_SHED_SKIN); }
    THEN { EXPECT_EQ(player->status1, STATUS1_POISON); }
}

SINGLE_BATTLE_TEST("Polished endturn Ice Body heals during source Dig and Fly charging")
{
    enum Move move = MOVE_DIG;
    PARAMETRIZE { move = MOVE_DIG; }
    PARAMETRIZE { move = MOVE_FLY; }
    GIVEN {
        PLAYER(SPECIES_GLALIE) { Ability(ABILITY_ICE_BODY); Speed(50); HP(80); MaxHP(161); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(100); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_HAIL); MOVE(player, move); }
    } THEN {
        EXPECT_EQ(player->hp, B_POLISHED_MOVE_BALANCE || move == MOVE_FLY ? 90 : 80);
    }
}

SINGLE_BATTLE_TEST("Polished endturn Ice Body respects weather and ability suppression while digging")
{
    enum Ability blocker = ABILITY_NONE;
    enum Item item = ITEM_NONE;
    PARAMETRIZE { blocker = ABILITY_NONE; item = ITEM_NONE; }
    PARAMETRIZE { blocker = ABILITY_CLOUD_NINE; item = ITEM_NONE; }
    PARAMETRIZE { blocker = ABILITY_NEUTRALIZING_GAS; item = ITEM_NONE; }
    PARAMETRIZE { blocker = ABILITY_NONE; item = ITEM_UTILITY_UMBRELLA; }
    GIVEN {
        PLAYER(SPECIES_GLALIE) { Ability(ABILITY_ICE_BODY); Item(item); Speed(50); HP(80); MaxHP(161); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(blocker); Speed(100); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_HAIL); MOVE(player, MOVE_DIG); }
    } THEN {
        // Utility Umbrella ignores rain and sun, not hail.
        EXPECT_EQ(player->hp, B_POLISHED_MOVE_BALANCE && blocker == ABILITY_NONE ? 90 : 80);
    }
}

SINGLE_BATTLE_TEST("Polished endturn weather recovery floors its amount and stops at full HP")
{
    enum Ability ability = ABILITY_ICE_BODY;
    enum Move weather = MOVE_HAIL;
    u32 hp = 80;
    for (u32 full = 0; full < 2; full++) {
        PARAMETRIZE { ability = ABILITY_ICE_BODY; weather = MOVE_HAIL; hp = full ? 161 : 80; }
        PARAMETRIZE { ability = ABILITY_RAIN_DISH; weather = MOVE_RAIN_DANCE; hp = full ? 161 : 80; }
        PARAMETRIZE { ability = ABILITY_DRY_SKIN; weather = MOVE_RAIN_DANCE; hp = full ? 161 : 80; }
    }
    GIVEN {
        PLAYER(SPECIES_GLALIE) { Ability(ability); HP(hp); MaxHP(161); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN { TURN { MOVE(opponent, weather); } }
    THEN {
        EXPECT_EQ(player->hp, hp == 161 ? 161 : hp + (ability == ABILITY_DRY_SKIN ? 20 : 10));
    }
}

SINGLE_BATTLE_TEST("Polished endturn Hydration uses the holder Umbrella and active weather")
{
    enum Ability blocker = ABILITY_NONE;
    enum Item item = ITEM_NONE;
    PARAMETRIZE { blocker = ABILITY_NONE; item = ITEM_NONE; }
    PARAMETRIZE { blocker = ABILITY_NONE; item = ITEM_UTILITY_UMBRELLA; }
    PARAMETRIZE { blocker = ABILITY_CLOUD_NINE; item = ITEM_NONE; }
    PARAMETRIZE { blocker = ABILITY_NEUTRALIZING_GAS; item = ITEM_NONE; }
    GIVEN {
        PLAYER(SPECIES_VAPOREON) { Ability(ABILITY_HYDRATION); Item(item); Status1(STATUS1_POISON); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(blocker); }
    } WHEN { TURN { MOVE(opponent, MOVE_RAIN_DANCE); } }
    THEN {
        EXPECT_EQ(player->status1, blocker == ABILITY_NONE && item == ITEM_NONE ? STATUS1_NONE : STATUS1_POISON);
    }
}

SINGLE_BATTLE_TEST("Polished endturn Moody raises one of five stats under older native presets")
{
    PASSES_RANDOMLY(1, B_POLISHED_MOVE_BALANCE ? 5 : 7, RNG_MOODY_INCREASE);
    GIVEN {
        WITH_CONFIG(B_MOODY_ACC_EVASION, GEN_7);
        PLAYER(SPECIES_OCTILLERY) { Ability(ABILITY_MOODY); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN { TURN { MOVE(player, MOVE_SPLASH); } }
    SCENE {
        ABILITY_POPUP(player, ABILITY_MOODY);
        MESSAGE("Octillery's Attack rose sharply!");
    }
    THEN {
        EXPECT_EQ(player->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 2);
        if (B_POLISHED_MOVE_BALANCE) {
            EXPECT_EQ(player->statStages[STAT_ACC], DEFAULT_STAT_STAGE);
            EXPECT_EQ(player->statStages[STAT_EVASION], DEFAULT_STAT_STAGE);
        }
    }
}

SINGLE_BATTLE_TEST("Polished endturn Moody lowers a distinct stat from its source candidate pool")
{
    PASSES_RANDOMLY(1, B_POLISHED_MOVE_BALANCE ? 4 : 6, RNG_MOODY_DECREASE);
    GIVEN {
        WITH_CONFIG(B_MOODY_ACC_EVASION, GEN_7);
        PLAYER(SPECIES_OCTILLERY) { Ability(ABILITY_MOODY); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN { TURN { MOVE(player, MOVE_SPLASH); } }
    SCENE {
        ABILITY_POPUP(player, ABILITY_MOODY);
        MESSAGE("Octillery's Attack fell!");
    }
    THEN {
        EXPECT_EQ(player->statStages[STAT_ATK], DEFAULT_STAT_STAGE - 1);
        u32 raised = 0;
        for (u32 stat = STAT_ATK; stat < NUM_BATTLE_STATS; stat++)
            raised += player->statStages[stat] == DEFAULT_STAT_STAGE + 2;
        EXPECT_EQ(raised, 1);
        if (B_POLISHED_MOVE_BALANCE) {
            EXPECT_EQ(player->statStages[STAT_ACC], DEFAULT_STAT_STAGE);
            EXPECT_EQ(player->statStages[STAT_EVASION], DEFAULT_STAT_STAGE);
        }
    }
}

DOUBLE_BATTLE_TEST("Polished endturn optional doubles keep native Ice Body hiding behavior")
{
    GIVEN {
        PLAYER(SPECIES_GLALIE) { Ability(ABILITY_ICE_BODY); HP(80); MaxHP(161); }
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(opponentLeft, MOVE_HAIL); MOVE(playerLeft, MOVE_DIG, target: opponentLeft); }
    } THEN { EXPECT_EQ(playerLeft->hp, 80); }
}

SINGLE_BATTLE_TEST("Polished endturn Moody skips a fully capped source stat mask")
{
    bool32 minimum = FALSE;
    PARAMETRIZE { minimum = FALSE; }
    PARAMETRIZE { minimum = TRUE; }
    GIVEN {
        WITH_CONFIG(B_MOODY_ACC_EVASION, GEN_7);
        PLAYER(SPECIES_WOBBUFFET) { Ability(minimum ? ABILITY_CONTRARY : ABILITY_PRESSURE); Speed(1000); MovesWithPP({MOVE_ANCIENT_POWER, 8}, {MOVE_SKILL_SWAP, 10}); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); HP(10000); MaxHP(10000); }
        OPPONENT(SPECIES_OCTILLERY) { Ability(ABILITY_MOODY); Speed(50); }
    } WHEN {
        // Acquire Moody only after preparing all five ordinary stat caps.
        for (u32 turn = 0; turn < 6; turn++) {
            TURN { MOVE(player, MOVE_ANCIENT_POWER, secondaryEffect: TRUE); }
        }
        TURN { SWITCH(opponent, 1); MOVE(player, MOVE_SKILL_SWAP); }
    } THEN {
        s32 sum = 0;
        u32 changed = 0;
        for (u32 stat = STAT_ATK; stat < NUM_BATTLE_STATS; stat++)
            sum += player->statStages[stat];
        for (u32 stat = STAT_ATK; stat < NUM_STATS; stat++)
            changed += player->statStages[stat] != (minimum ? MIN_STAT_STAGE : MAX_STAT_STAGE);
        EXPECT_EQ(sum, 5 * (minimum ? MIN_STAT_STAGE : MAX_STAT_STAGE)
            + 2 * DEFAULT_STAT_STAGE + (B_POLISHED_MOVE_BALANCE ? minimum ? 2 : -1 : 1));
        if (B_POLISHED_MOVE_BALANCE) {
            EXPECT_EQ(changed, 1);
            EXPECT_EQ(player->statStages[STAT_ACC], DEFAULT_STAT_STAGE);
            EXPECT_EQ(player->statStages[STAT_EVASION], DEFAULT_STAT_STAGE);
        }
    }
}

SINGLE_BATTLE_TEST("Polished endturn Moody clips the sole eligible raise and lowers a different stat")
{
    GIVEN {
        WITH_CONFIG(B_MOODY_ACC_EVASION, GEN_9);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_PRESSURE); Speed(1000); MovesWithPP({MOVE_ANCIENT_POWER, 8}, {MOVE_SKILL_SWAP, 10}, {MOVE_SPLASH, 10}); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); HP(10000); MaxHP(10000); }
        OPPONENT(SPECIES_OCTILLERY) { Ability(ABILITY_MOODY); Speed(50); }
    } WHEN {
        for (u32 turn = 0; turn < 6; turn++) {
            TURN { MOVE(player, MOVE_ANCIENT_POWER, secondaryEffect: TRUE); }
        }
        TURN { MOVE(player, MOVE_SPLASH); MOVE(opponent, MOVE_GROWL); }
        TURN { SWITCH(opponent, 1); MOVE(player, MOVE_SKILL_SWAP); }
    } THEN {
        EXPECT_EQ(player->statStages[STAT_ATK], MAX_STAT_STAGE);
        u32 lowered = 0;
        for (u32 stat = STAT_DEF; stat < NUM_STATS; stat++) {
            EXPECT_GE(player->statStages[stat], MAX_STAT_STAGE - 1);
            lowered += player->statStages[stat] == MAX_STAT_STAGE - 1;
        }
        EXPECT_EQ(lowered, 1);
        EXPECT_EQ(player->statStages[STAT_ACC], DEFAULT_STAT_STAGE);
        EXPECT_EQ(player->statStages[STAT_EVASION], DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished endturn Moody excludes a raised stat from the sole eligible lower")
{
    PASSES_RANDOMLY(1, 1, RNG_MOODY_INCREASE);
    GIVEN {
        WITH_CONFIG(B_MOODY_ACC_EVASION, GEN_9);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_CONTRARY); Speed(1000); MovesWithPP({MOVE_ANCIENT_POWER, 8}, {MOVE_SKILL_SWAP, 10}, {MOVE_SPLASH, 10}); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); HP(10000); MaxHP(10000); }
        OPPONENT(SPECIES_OCTILLERY) { Ability(ABILITY_MOODY); Speed(50); }
    } WHEN {
        for (u32 turn = 0; turn < 6; turn++) {
            TURN { MOVE(player, MOVE_ANCIENT_POWER, secondaryEffect: TRUE); }
        }
        // Contrary leaves Attack one stage above the minimum.
        TURN { MOVE(player, MOVE_SPLASH); MOVE(opponent, MOVE_GROWL); }
        TURN { SWITCH(opponent, 1); MOVE(player, MOVE_SKILL_SWAP); }
    } THEN {
        u32 raisedOther = 0;
        for (u32 stat = STAT_DEF; stat < NUM_STATS; stat++) {
            EXPECT(player->statStages[stat] == MIN_STAT_STAGE || player->statStages[stat] == MIN_STAT_STAGE + 2);
            raisedOther += player->statStages[stat] == MIN_STAT_STAGE + 2;
        }
        // Enumerate all five choices: raising Attack excludes the only
        // lowerable stat; each other choice must lower Attack to its cap.
        EXPECT_EQ(player->statStages[STAT_ATK], raisedOther ? MIN_STAT_STAGE : MIN_STAT_STAGE + 3);
        EXPECT_LE(raisedOther, 1);
        EXPECT_EQ(player->statStages[STAT_ACC], DEFAULT_STAT_STAGE);
        EXPECT_EQ(player->statStages[STAT_EVASION], DEFAULT_STAT_STAGE);
    }
}
