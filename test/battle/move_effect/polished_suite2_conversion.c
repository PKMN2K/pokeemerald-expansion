#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished Suite 2 Conversion selects any known moves listed type without using its PP")
{
    u32 slot = 1;
    PARAMETRIZE { slot = 1; }
    PARAMETRIZE { slot = 2; }
    PARAMETRIZE { slot = 3; }
    GIVEN {
        PLAYER(SPECIES_PORYGON) { MovesWithPP({MOVE_CONVERSION, 30}, {MOVE_WATER_GUN, 0}, {MOVE_EMBER, 1}, {MOVE_THUNDER_SHOCK, 2}); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CONVERSION); if (B_POLISHED_MOVE_BALANCE) CONVERSION_CHOICE(player, slot); }
    } THEN {
        EXPECT_EQ(player->types[0], B_POLISHED_MOVE_BALANCE ? GetMoveType(player->moves[slot]) : TYPE_NORMAL);
        EXPECT_EQ(player->types[1], player->types[0]);
        EXPECT_EQ(player->pp[0], 29);
        EXPECT_EQ(player->pp[1], 0);
        EXPECT_EQ(player->pp[2], 1);
        EXPECT_EQ(player->pp[3], 2);
    }
}

SINGLE_BATTLE_TEST("Polished Suite 2 Conversion reopens selection after a current type empty slot or cancel")
{
    u32 invalid = 0;
    PARAMETRIZE { invalid = 0; }
    PARAMETRIZE { invalid = 2; }
    PARAMETRIZE { invalid = 0xfe; }
    GIVEN {
        PLAYER(SPECIES_PORYGON) { Moves(MOVE_CONVERSION, MOVE_EMBER); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN {
            MOVE(player, MOVE_CONVERSION);
            if (B_POLISHED_MOVE_BALANCE) {
                CONVERSION_CHOICE(player, invalid);
                CONVERSION_CHOICE(player, 1);
            }
        }
    } SCENE {
        if (B_POLISHED_MOVE_BALANCE) {
            MESSAGE("Change into which move's type?");
            MESSAGE("Cannot change into that type!");
            ANIMATION(ANIM_TYPE_MOVE, MOVE_CONVERSION, player);
        } else NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_CONVERSION, player);
    } THEN {
        EXPECT_EQ(player->types[0], B_POLISHED_MOVE_BALANCE ? TYPE_FIRE : TYPE_NORMAL);
        EXPECT_EQ(player->pp[0], GetMovePP(MOVE_CONVERSION) - 1);
    }
}

SINGLE_BATTLE_TEST("Polished Suite 2 Conversion fails without opening a menu when no new type exists")
{
    GIVEN {
        PLAYER(SPECIES_PORYGON) { Moves(MOVE_CONVERSION, MOVE_TACKLE, MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CONVERSION); }
    } SCENE {
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_CONVERSION, player);
    } THEN {
        EXPECT_EQ(player->types[0], TYPE_NORMAL);
    }
}

WILD_BATTLE_TEST("Polished Suite 2 wild Conversion selects a valid later slot")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_PORYGON) { Moves(MOVE_CONVERSION, MOVE_TACKLE, MOVE_EMBER); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_CONVERSION); }
    } THEN {
        EXPECT_EQ(opponent->types[0], B_POLISHED_MOVE_BALANCE ? TYPE_FIRE : TYPE_NORMAL);
    }
}

SINGLE_BATTLE_TEST("Polished linked Conversion plays both trainers' choices and leaves the next turn aligned")
{
    ASSUME(B_POLISHED_MOVE_BALANCE);
    GIVEN {
        PLAYER(SPECIES_PORYGON) { Moves(MOVE_CONVERSION, MOVE_EMBER, MOVE_TACKLE); }
        OPPONENT(SPECIES_PORYGON) { Moves(MOVE_CONVERSION, MOVE_TACKLE, MOVE_WATER_GUN); }
    } WHEN {
        TURN {
            MOVE(player, MOVE_CONVERSION); CONVERSION_CHOICE(player, 1);
            MOVE(opponent, MOVE_CONVERSION); CONVERSION_CHOICE(opponent, 0); CONVERSION_CHOICE(opponent, 2);
        }
        TURN { MOVE(player, MOVE_TACKLE); MOVE(opponent, MOVE_TACKLE); }
    } THEN {
        EXPECT_EQ(player->types[0], TYPE_FIRE);
        EXPECT_EQ(opponent->types[0], TYPE_WATER);
        EXPECT_EQ(player->pp[2], GetMovePP(MOVE_TACKLE) - 1);
        EXPECT_EQ(opponent->pp[1], GetMovePP(MOVE_TACKLE) - 1);
    }
}
