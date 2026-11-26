#include "global.h"
#include "test/battle.h"
#include "battle_util.h"
#include "constants/flags.h"
#include "event_data.h"

WILD_BATTLE_TEST("Polished obedience uses Johto badge thresholds including unchanged intermediate badges")
{
    u32 badge = 0, cap = 10;
    PARAMETRIZE { badge = 0; cap = 10; }
    PARAMETRIZE { badge = FLAG_POLISHED_ZEPHYR_BADGE; cap = 20; }
    PARAMETRIZE { badge = FLAG_POLISHED_HIVE_BADGE; cap = 30; }
    PARAMETRIZE { badge = FLAG_POLISHED_PLAIN_BADGE; cap = 10; }
    PARAMETRIZE { badge = FLAG_POLISHED_FOG_BADGE; cap = 50; }
    PARAMETRIZE { badge = FLAG_POLISHED_STORM_BADGE; cap = 10; }
    PARAMETRIZE { badge = FLAG_POLISHED_MINERAL_BADGE; cap = 70; }
    PARAMETRIZE { badge = FLAG_POLISHED_GLACIER_BADGE; cap = 10; }
    PARAMETRIZE { badge = FLAG_POLISHED_RISING_BADGE; cap = 101; }
    GIVEN {
        FlagClear(FLAG_POLISHED_ZEPHYR_BADGE);
        for (u32 flag = FLAG_POLISHED_HIVE_BADGE; flag <= FLAG_POLISHED_RISING_BADGE; flag++) FlagClear(flag);
        if (badge) FLAG_SET(badge);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN { TURN {} }
    THEN { EXPECT_EQ(GetPolishedObedienceLevel(), cap); }
}

WILD_BATTLE_TEST("Polished obedience checks the source trainer ID without OT name or high word")
{
    u32 otId = 0x12340000;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { OTName("OTHER"); MovesWithPP({MOVE_GROWL, 10}); }
        if (B_POLISHED_MOVE_BALANCE) SetMonData(&PLAYER_PARTY[0], MON_DATA_OT_ID, &otId);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_GROWL); }
    } THEN {
        EXPECT_EQ(player->pp[0], 9);
        EXPECT_EQ(opponent->statStages[STAT_ATK], DEFAULT_STAT_STAGE - 1);
    }
}

WILD_BATTLE_TEST("Polished obedience first accepted roll uses the exact cap boundary")
{
    u32 roll = 9, otId = 1;
    PARAMETRIZE { roll = 9; }
    PARAMETRIZE { roll = 10; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { MovesWithPP({MOVE_GROWL, 10}); }
        if (B_POLISHED_MOVE_BALANCE) SetMonData(&PLAYER_PARTY[0], MON_DATA_OT_ID, &otId);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_GROWL, WITH_RNG(RNG_POLISHED_OBEDIENCE_FIRST, roll)); }
    } THEN {
        EXPECT_EQ(player->pp[0], B_POLISHED_MOVE_BALANCE && roll == 10 ? 10 : 9);
        if (B_POLISHED_MOVE_BALANCE && roll == 10) {
            EXPECT_EQ(gLastMoves[B_BATTLER_0], MOVE_NONE);
            EXPECT_EQ(GetChoiceLockedMove(B_BATTLER_0), MOVE_NONE);
        }
    }
}

WILD_BATTLE_TEST("Polished obedience does not affect an outsider at the exact badge cap")
{
    u32 otId = 1;
    GIVEN {
        FLAG_SET(FLAG_POLISHED_HIVE_BADGE);
        PLAYER(SPECIES_WOBBUFFET) { Level(30); MovesWithPP({MOVE_GROWL, 10}); }
        if (B_POLISHED_MOVE_BALANCE) SetMonData(&PLAYER_PARTY[0], MON_DATA_OT_ID, &otId);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_GROWL, WITH_RNG(RNG_POLISHED_OBEDIENCE_FIRST, 59)); }
    } THEN { EXPECT_EQ(player->pp[0], 9); }
}

WILD_BATTLE_TEST("Polished obedience can choose another move despite a Choice restriction")
{
    u32 otId = 1;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Item(ITEM_CHOICE_BAND); MovesWithPP({MOVE_SPLASH, 10}, {MOVE_GROWL, 10}); }
        if (B_POLISHED_MOVE_BALANCE) SetMonData(&PLAYER_PARTY[0], MON_DATA_OT_ID, &otId);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH, WITH_RNG(RNG_POLISHED_OBEDIENCE_SECOND, 0)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, B_POLISHED_MOVE_BALANCE ? MOVE_GROWL : MOVE_SPLASH, player);
    } THEN {
        EXPECT_EQ(player->pp[0], B_POLISHED_MOVE_BALANCE ? 10 : 9);
        EXPECT_EQ(player->pp[1], B_POLISHED_MOVE_BALANCE ? 9 : 10);
        if (B_POLISHED_MOVE_BALANCE) {
            EXPECT_EQ(gLastMoves[B_BATTLER_0], MOVE_NONE);
            EXPECT_EQ(GetChoiceLockedMove(B_BATTLER_0), MOVE_NONE);
        }
    }
}

WILD_BATTLE_TEST("Polished obedience with no alternate PP loafs without spending selected PP")
{
    u32 otId = 1;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { MovesWithPP({MOVE_SPLASH, 10}, {MOVE_GROWL, 0}); }
        if (B_POLISHED_MOVE_BALANCE) SetMonData(&PLAYER_PARTY[0], MON_DATA_OT_ID, &otId);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH, WITH_RNG(RNG_POLISHED_OBEDIENCE_SECOND, 0)); }
    } THEN { EXPECT_EQ(player->pp[0], B_POLISHED_MOVE_BALANCE ? 10 : 9); }
}

WILD_BATTLE_TEST("Polished obedience napping overwrites poison below the source outcome boundary")
{
    u32 otId = 1, roll = 0;
    PARAMETRIZE { roll = 0; }
    PARAMETRIZE { roll = 89; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Status1(STATUS1_POISON); Ability(ABILITY_NONE); MovesWithPP({MOVE_GROWL, 10}); }
        if (B_POLISHED_MOVE_BALANCE) SetMonData(&PLAYER_PARTY[0], MON_DATA_OT_ID, &otId);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_GROWL,
                    WITH_RNG(RNG_POLISHED_OBEDIENCE_OUTCOME, roll)); }
    } THEN {
        EXPECT_EQ(player->status1, B_POLISHED_MOVE_BALANCE ? 7 : STATUS1_POISON);
        EXPECT_EQ(player->pp[0], B_POLISHED_MOVE_BALANCE ? 10 : 9);
    }
}

WILD_BATTLE_TEST("Polished obedience self damage occurs between the two source outcome boundaries")
{
    u32 otId = 1, roll = 90;
    PARAMETRIZE { roll = 90; }
    PARAMETRIZE { roll = 179; }
    PARAMETRIZE { roll = 180; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Ability(ABILITY_NONE); MovesWithPP({MOVE_GROWL, 10}); }
        if (B_POLISHED_MOVE_BALANCE) SetMonData(&PLAYER_PARTY[0], MON_DATA_OT_ID, &otId);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_GROWL, WITH_RNG(RNG_POLISHED_OBEDIENCE_OUTCOME, roll)); }
    } THEN {
        if (B_POLISHED_MOVE_BALANCE && roll < 180) EXPECT_LT(player->hp, 1000);
        else EXPECT_EQ(player->hp, 1000);
        EXPECT_EQ(player->pp[0], B_POLISHED_MOVE_BALANCE ? 10 : 9);
    }
}

SINGLE_BATTLE_TEST("Polished action checks confusion before a newly applied Disable")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(50); MaxHP(1000); HP(1000); Moves(MOVE_GROWL); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(100); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_CONFUSE_RAY); MOVE(player, MOVE_GROWL, WITH_RNG(RNG_CONFUSION, FALSE)); }
        TURN { MOVE(opponent, MOVE_DISABLE); MOVE(player, MOVE_GROWL, WITH_RNG(RNG_CONFUSION, TRUE)); }
    } SCENE {
        if (B_POLISHED_MOVE_BALANCE) MESSAGE("It hurt itself in its confusion!");
        else MESSAGE("Wobbuffet's Growl is disabled!");
    } THEN {
        if (B_POLISHED_MOVE_BALANCE) EXPECT_LT(player->hp, 1000);
        else EXPECT_EQ(player->hp, 1000);
    }
}

WILD_BATTLE_TEST("Polished obedience an active Disable prevents all alternate move choices")
{
    u32 otId = 1;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); MovesWithPP({MOVE_GROWL, 10}, {MOVE_SPLASH, 10}, {MOVE_TACKLE, 10}); }
        if (B_POLISHED_MOVE_BALANCE) SetMonData(&PLAYER_PARTY[0], MON_DATA_OT_ID, &otId);
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); }
    } WHEN {
        TURN { MOVE(player, MOVE_GROWL, WITH_RNG(RNG_POLISHED_OBEDIENCE_FIRST, 0)); MOVE(opponent, MOVE_DISABLE); }
        TURN { MOVE(player, MOVE_SPLASH, WITH_RNG(RNG_POLISHED_OBEDIENCE_SECOND, 0)); }
    } THEN {
        EXPECT_EQ(player->pp[1], B_POLISHED_MOVE_BALANCE ? 10 : 9);
        EXPECT_EQ(player->pp[2], 10);
    }
}

WILD_BATTLE_TEST("Polished obedience does not run before flinch or spend selected PP")
{
    u32 otId = 1;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(50); MovesWithPP({MOVE_GROWL, 10}); }
        if (B_POLISHED_MOVE_BALANCE) SetMonData(&PLAYER_PARTY[0], MON_DATA_OT_ID, &otId);
        OPPONENT(SPECIES_WOBBUFFET) { Speed(100); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_FAKE_OUT); MOVE(player, MOVE_GROWL, WITH_RNG(RNG_POLISHED_OBEDIENCE_OUTCOME, 0)); }
    } SCENE { MESSAGE("Wobbuffet flinched and couldn't move!"); }
    THEN {
        EXPECT_EQ(player->pp[0], 10);
        EXPECT_EQ(player->status1, STATUS1_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished action sleep cancellation clears a repeated Destiny Bond")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(50); Ability(ABILITY_NONE); Moves(MOVE_DESTINY_BOND); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(100); Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(player, MOVE_DESTINY_BOND); }
        TURN { MOVE(opponent, MOVE_SPORE); MOVE(player, MOVE_DESTINY_BOND); }
    } THEN { EXPECT_EQ((u32)player->volatiles.destinyBond, 0); }
}

WILD_BATTLE_TEST("Polished obedience napping bypasses Insomnia and Vital Spirit without a generic cure")
{
    u32 otId = 1;
    enum Ability ability = ABILITY_INSOMNIA;
    PARAMETRIZE { ability = ABILITY_INSOMNIA; }
    PARAMETRIZE { ability = ABILITY_VITAL_SPIRIT; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ability); Speed(100); Moves(MOVE_GROWL); }
        if (B_POLISHED_MOVE_BALANCE) SetMonData(&PLAYER_PARTY[0], MON_DATA_OT_ID, &otId);
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); }
    } WHEN {
        TURN { MOVE(player, MOVE_GROWL, WITH_RNG(RNG_POLISHED_OBEDIENCE_OUTCOME, 0)); MOVE(opponent, MOVE_SPLASH); }
        TURN { MOVE(player, MOVE_GROWL); MOVE(opponent, MOVE_SPLASH); }
    } THEN {
        EXPECT_EQ(player->status1, B_POLISHED_MOVE_BALANCE ? 6 : STATUS1_NONE);
    }
}

WILD_BATTLE_TEST("Polished obedience traded as OT option bypasses only the foreign player check")
{
    struct BattlePokemon *user = player, *target = opponent;
    bool32 tradedAsOt = FALSE, foreign = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 setting = 0; setting < 2; setting++)
            for (u32 outsider = 0; outsider < 2; outsider++)
                PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; tradedAsOt = setting; foreign = outsider; }
    u32 otId = 0x12340000 | foreign;
    bool32 disobeys = B_POLISHED_MOVE_BALANCE && user == player && foreign && !tradedAsOt;
    GIVEN {
        if (tradedAsOt) FLAG_SET(FLAG_POLISHED_TRADED_AS_OT);
        else FlagClear(FLAG_POLISHED_TRADED_AS_OT);
        // This native badge is deliberately irrelevant to the source caps.
        FlagSet(FLAG_BADGE08_GET);
        PLAYER(SPECIES_WOBBUFFET) { Level(100); OTName("OTHER"); Speed(user == player ? 100 : 50); MovesWithPP({MOVE_GROWL, 10}, {MOVE_SPLASH, 10}); }
        OPPONENT(SPECIES_WOBBUFFET) { Level(100); Speed(user == opponent ? 100 : 50); MovesWithPP({MOVE_GROWL, 10}, {MOVE_SPLASH, 10}); }
        SetMonData(user == player ? &PLAYER_PARTY[0] : &OPPONENT_PARTY[0], MON_DATA_OT_ID, &otId);
    } WHEN {
        TURN { MOVE(user, MOVE_GROWL, WITH_RNG(RNG_POLISHED_OBEDIENCE_FIRST, 10)); MOVE(target, MOVE_SPLASH); }
    } THEN {
        EXPECT_EQ(user->pp[0], disobeys ? 10 : 9);
        EXPECT_EQ(target->statStages[STAT_ATK], DEFAULT_STAT_STAGE - !disobeys);
        EXPECT_EQ(user->status1, STATUS1_NONE);
        FlagClear(FLAG_BADGE08_GET);
    }
}

SINGLE_BATTLE_TEST("Polished obedience traded as OT option preserves either active native gimmick")
{
    bool32 tradedAsOt = FALSE, userTera = FALSE;
    for (u32 setting = 0; setting < 2; setting++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { tradedAsOt = setting; userTera = side; }
    u32 otId = 1;
    GIVEN {
        if (tradedAsOt) FLAG_SET(FLAG_POLISHED_TRADED_AS_OT);
        else FlagClear(FLAG_POLISHED_TRADED_AS_OT);
        FlagSet(FLAG_BADGE08_GET);
        PLAYER(SPECIES_WOBBUFFET) { Level(100); Speed(100); TeraType(TYPE_NORMAL); MovesWithPP({MOVE_GROWL, 10}); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); TeraType(TYPE_NORMAL); }
        SetMonData(&PLAYER_PARTY[0], MON_DATA_OT_ID, &otId);
    } WHEN {
        TURN { MOVE(player, MOVE_GROWL, gimmick: userTera ? GIMMICK_TERA : GIMMICK_NONE, WITH_RNG(RNG_POLISHED_OBEDIENCE_FIRST, 0)); MOVE(opponent, MOVE_SPLASH, gimmick: userTera ? GIMMICK_NONE : GIMMICK_TERA); }
        TURN { MOVE(player, MOVE_GROWL, WITH_RNG(RNG_POLISHED_OBEDIENCE_FIRST, 10)); MOVE(opponent, MOVE_SPLASH); }
    } THEN {
        EXPECT_EQ(GetActiveGimmick(B_BATTLER_0), userTera ? GIMMICK_TERA : GIMMICK_NONE);
        EXPECT_EQ(GetActiveGimmick(B_BATTLER_1), userTera ? GIMMICK_NONE : GIMMICK_TERA);
        EXPECT_EQ(player->pp[0], 8);
        EXPECT_EQ(opponent->statStages[STAT_ATK], DEFAULT_STAT_STAGE - 2);
        FlagClear(FLAG_BADGE08_GET);
    }
}
