#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished lifecycle Baton Pass preserves incoming source binding counters")
{
    enum Move bind = MOVE_WRAP;
    PARAMETRIZE { bind = MOVE_WRAP; }
    PARAMETRIZE { bind = MOVE_FIRE_SPIN; }
    PARAMETRIZE { bind = MOVE_WHIRLPOOL; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); MaxHP(1600); HP(1600); }
        PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_NONE); MaxHP(1600); HP(1600); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(opponent, bind, hit: TRUE, WITH_RNG(RNG_WRAP, 5)); }
        TURN { MOVE(player, MOVE_BATON_PASS); SEND_OUT(player, 1); }
    } THEN {
        EXPECT_EQ(player->species, SPECIES_CHANSEY);
        EXPECT_EQ((u32)player->volatiles.wrapped, B_POLISHED_MOVE_BALANCE);
        EXPECT_EQ(player->hp, B_POLISHED_MOVE_BALANCE ? 1400 : 1600);
    }
}

SINGLE_BATTLE_TEST("Polished lifecycle Baton Pass keeps the opponent bound to the departing side")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); MaxHP(1600); HP(1600); }
    } WHEN {
        TURN { MOVE(player, MOVE_WRAP, hit: TRUE, WITH_RNG(RNG_WRAP, 5)); }
        TURN { MOVE(player, MOVE_BATON_PASS); SEND_OUT(player, 1); }
    } THEN { EXPECT_EQ((u32)opponent->volatiles.wrapped, B_POLISHED_MOVE_BALANCE); }
}

SINGLE_BATTLE_TEST("Polished lifecycle Baton Pass releases its Mean Look target but inherits the opponents trap")
{
    bool32 trappingUser = FALSE;
    u32 generation = GEN_3;
    PARAMETRIZE { trappingUser = FALSE; generation = GEN_3; }
    PARAMETRIZE { trappingUser = TRUE; generation = GEN_3; }
    PARAMETRIZE { trappingUser = FALSE; generation = GEN_5; }
    PARAMETRIZE { trappingUser = TRUE; generation = GEN_5; }
    GIVEN {
        WITH_CONFIG(B_BATON_PASS_TRAPPING, generation);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(trappingUser ? player : opponent, MOVE_MEAN_LOOK); }
        TURN { MOVE(player, MOVE_BATON_PASS); SEND_OUT(player, 1); }
    } THEN {
        // Source CANT_RUN is stored on the trap's user, whereas the target
        // engine stores escapePrevention on its victim. SendInUserPkmn clears
        // the departing user's bit, not the opponent's bit.
        EXPECT_EQ((u32)player->volatiles.escapePrevention, !trappingUser);
        EXPECT_EQ((u32)opponent->volatiles.escapePrevention,
            trappingUser && !B_POLISHED_MOVE_BALANCE && generation < GEN_5);
    }
}

SINGLE_BATTLE_TEST("Polished lifecycle Baton Pass carries the toxic counter to an already toxic replacement")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); MaxHP(1600); HP(1600); Status1(STATUS1_TOXIC_POISON); }
        PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_NONE); MaxHP(1600); HP(1600); Status1(STATUS1_TOXIC_POISON); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_BATON_PASS); SEND_OUT(player, 1); }
    } THEN {
        EXPECT_EQ(player->status1 & STATUS1_TOXIC_COUNTER, STATUS1_TOXIC_TURN(B_POLISHED_MOVE_BALANCE ? 2 : 1));
        EXPECT_EQ(player->hp, B_POLISHED_MOVE_BALANCE ? 1400 : 1500);
    }
}

SINGLE_BATTLE_TEST("Polished lifecycle ordinary switching resets toxic and both sides binding")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_RUN_AWAY); MaxHP(1600); HP(1600); Status1(STATUS1_TOXIC_POISON); }
        PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_NONE); MaxHP(1600); HP(1600); Status1(STATUS1_TOXIC_POISON); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); MaxHP(1600); HP(1600); }
    } WHEN {
        TURN { MOVE(player, MOVE_WRAP, hit: TRUE, WITH_RNG(RNG_WRAP, 5)); }
        TURN { SWITCH(player, 1); }
    } THEN {
        EXPECT_EQ(player->status1 & STATUS1_TOXIC_COUNTER, STATUS1_TOXIC_TURN(1));
        EXPECT_EQ((u32)opponent->volatiles.wrapped, FALSE);
    }
}

SINGLE_BATTLE_TEST("Polished lifecycle Spikes precede Toxic Spikes regardless of placement order")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_NONE); MaxHP(160); HP(160); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_TOXIC_SPIKES); }
        TURN { MOVE(opponent, MOVE_SPIKES); }
        TURN { SWITCH(player, 1); }
    } SCENE {
        MESSAGE("Go! Chansey!");
        if (B_POLISHED_MOVE_BALANCE) {
            MESSAGE("Chansey was hurt by the spikes!");
            MESSAGE("Chansey was poisoned!");
        } else {
            MESSAGE("Chansey was poisoned!");
            MESSAGE("Chansey was hurt by the spikes!");
        }
    } THEN { EXPECT_EQ(player->hp, 120); }
}

SINGLE_BATTLE_TEST("Polished lifecycle both replacements take hazards before entry abilities")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); HP(1); Status1(STATUS1_POISON); }
        PLAYER(SPECIES_ARCANINE) { Ability(ABILITY_INTIMIDATE); Speed(50); MaxHP(160); HP(160); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); HP(1); Status1(STATUS1_POISON); }
        OPPONENT(SPECIES_NINETALES) { Ability(ABILITY_DROUGHT); Speed(100); MaxHP(160); HP(160); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPIKES); MOVE(opponent, MOVE_SPIKES); SEND_OUT(player, 1); SEND_OUT(opponent, 1); }
    } SCENE {
        HP_BAR(opponent, damage: 20);
        if (B_POLISHED_MOVE_BALANCE) {
            HP_BAR(player, damage: 20);
            ABILITY_POPUP(opponent, ABILITY_DROUGHT);
        } else {
            ABILITY_POPUP(opponent, ABILITY_DROUGHT);
            HP_BAR(player, damage: 20);
        }
        ABILITY_POPUP(player, ABILITY_INTIMIDATE);
    } THEN {
        EXPECT_EQ(player->hp, 140);
        EXPECT_EQ(opponent->hp, 140);
    }
}

SINGLE_BATTLE_TEST("Polished lifecycle simultaneous replacements use the departed battlers Speed order")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); HP(1); Status1(STATUS1_POISON); }
        PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Speed(200); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); HP(1); Status1(STATUS1_POISON); }
        OPPONENT(SPECIES_CLEFABLE) { Ability(ABILITY_NONE); Speed(25); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); SEND_OUT(player, 1); SEND_OUT(opponent, 1); }
    } SCENE {
        if (B_POLISHED_MOVE_BALANCE) {
            MESSAGE("2 sent out Clefable!");
            MESSAGE("Go! Chansey!");
        } else {
            MESSAGE("Go! Chansey!");
            MESSAGE("2 sent out Clefable!");
        }
    }
}

SINGLE_BATTLE_TEST("Polished lifecycle hazard knockouts repeat replacement without repeating residuals")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); HP(1); Status1(STATUS1_POISON); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); HP(1); }
        PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_NONE); MaxHP(160); HP(160); Status1(STATUS1_BURN); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_SPIKES); SEND_OUT(player, 1); SEND_OUT(player, 2); }
    } SCENE {
        MESSAGE("Wobbuffet fainted!");
        MESSAGE("Wynaut was hurt by the spikes!");
        MESSAGE("Wynaut fainted!");
        MESSAGE("Chansey was hurt by the spikes!");
    } THEN {
        EXPECT_EQ(player->species, SPECIES_CHANSEY);
        EXPECT_EQ(player->hp, 140);
    }
}

SINGLE_BATTLE_TEST("Polished lifecycle incoming Gas suppresses hazards before its announcement")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); HP(1); Status1(STATUS1_POISON); }
        PLAYER(SPECIES_CLEFABLE) { Ability(ABILITY_MAGIC_GUARD); Speed(100); MaxHP(160); HP(160); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); HP(1); Status1(STATUS1_POISON); }
        OPPONENT(SPECIES_KOFFING) { Ability(ABILITY_NEUTRALIZING_GAS); Speed(50); MaxHP(160); HP(160); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_SPIKES); SEND_OUT(player, 1); SEND_OUT(opponent, 1); }
    } SCENE {
        if (B_POLISHED_MOVE_BALANCE) {
            HP_BAR(player, damage: 20);
            ABILITY_POPUP(opponent, ABILITY_NEUTRALIZING_GAS);
        } else {
            ABILITY_POPUP(opponent, ABILITY_NEUTRALIZING_GAS);
            HP_BAR(player, damage: 20);
        }
    } THEN { EXPECT_EQ(player->hp, 140); }
}

SINGLE_BATTLE_TEST("Polished lifecycle entry Unnerve follows a faster entry ability")
{
    GIVEN {
        PLAYER(SPECIES_ARCANINE) { Ability(ABILITY_INTIMIDATE); Speed(100); }
        OPPONENT(SPECIES_ARBOK) { Ability(ABILITY_UNNERVE); Speed(50); }
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE); } }
    SCENE {
        if (B_POLISHED_MOVE_BALANCE) {
            ABILITY_POPUP(player, ABILITY_INTIMIDATE);
            ABILITY_POPUP(opponent, ABILITY_UNNERVE);
        } else {
            ABILITY_POPUP(opponent, ABILITY_UNNERVE);
            ABILITY_POPUP(player, ABILITY_INTIMIDATE);
        }
    }
}

SINGLE_BATTLE_TEST("Polished lifecycle dual Neutralizing Gas announces once in Speed order")
{
    GIVEN {
        PLAYER(SPECIES_KOFFING) { Ability(ABILITY_NEUTRALIZING_GAS); Speed(100); }
        OPPONENT(SPECIES_KOFFING) { Ability(ABILITY_NEUTRALIZING_GAS); Speed(50); }
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE); } }
    SCENE {
        ABILITY_POPUP(player, ABILITY_NEUTRALIZING_GAS);
        if (B_POLISHED_MOVE_BALANCE) { NOT ABILITY_POPUP(opponent, ABILITY_NEUTRALIZING_GAS); }
        else { ABILITY_POPUP(opponent, ABILITY_NEUTRALIZING_GAS); }
    }
}
