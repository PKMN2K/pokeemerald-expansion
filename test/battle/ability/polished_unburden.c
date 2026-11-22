#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished Unburden is not activated by Thief or Knock Off on either side")
{
    enum Move move = MOVE_THIEF;
    struct BattlePokemon *holder = player, *other = opponent;
    for (u32 side = 0; side < 2; side++)
        for (u32 knock = 0; knock < 2; knock++)
            PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; move = knock ? MOVE_KNOCK_OFF : MOVE_THIEF; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_UNBURDEN); Item(holder == player ? ITEM_POKE_BALL : ITEM_NONE); Speed(holder == player ? 100 : 150); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_UNBURDEN); Item(holder == opponent ? ITEM_POKE_BALL : ITEM_NONE); Speed(holder == opponent ? 100 : 150); }
    } WHEN {
        TURN { MOVE(other, move); MOVE(holder, MOVE_SPLASH); }
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, MOVE_CELEBRATE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, other);
        if (B_POLISHED_MOVE_BALANCE) {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, other);
            ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, holder);
        } else {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, holder);
            ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, other);
        }
    } THEN {
        EXPECT_EQ(holder->item, ITEM_NONE);
        EXPECT_EQ(other->item, move == MOVE_THIEF ? ITEM_POKE_BALL : ITEM_NONE);
        EXPECT_EQ((u32)holder->volatiles.unburdenActive, !B_POLISHED_MOVE_BALANCE);
        EXPECT_EQ((u32)other->volatiles.unburdenActive, FALSE);
    }
}

SINGLE_BATTLE_TEST("Polished Unburden survives receiving a held item through Thief")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_UNBURDEN); Item(ITEM_ORAN_BERRY); MaxHP(400); HP(100); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_POKE_BALL); Speed(150); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); MOVE(opponent, MOVE_SPLASH); }
        TURN { MOVE(player, MOVE_THIEF); MOVE(opponent, MOVE_SPLASH); }
        TURN { MOVE(player, MOVE_CELEBRATE); MOVE(opponent, MOVE_CELEBRATE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_THIEF, player);
        if (B_POLISHED_MOVE_BALANCE) {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, player);
            ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, opponent);
        } else {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, opponent);
            ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, player);
        }
    } THEN {
        EXPECT_EQ(player->item, ITEM_POKE_BALL);
        EXPECT_EQ((u32)player->volatiles.unburdenActive, B_POLISHED_MOVE_BALANCE);
    }
}

SINGLE_BATTLE_TEST("Polished Unburden is not activated when Pickpocket steals its item")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_UNBURDEN); Item(ITEM_POKE_BALL); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_PICKPOCKET); Speed(150); }
    } WHEN {
        TURN { MOVE(player, MOVE_TACKLE); MOVE(opponent, MOVE_SPLASH); }
        TURN { MOVE(player, MOVE_CELEBRATE); MOVE(opponent, MOVE_CELEBRATE); }
    } SCENE {
        ABILITY_POPUP(opponent, ABILITY_PICKPOCKET);
        if (B_POLISHED_MOVE_BALANCE) {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, opponent);
            ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, player);
        } else {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, player);
            ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, opponent);
        }
    } THEN {
        EXPECT_EQ(player->item, ITEM_NONE);
        EXPECT_EQ(opponent->item, ITEM_POKE_BALL);
        EXPECT_EQ((u32)player->volatiles.unburdenActive, !B_POLISHED_MOVE_BALANCE);
    }
}

SINGLE_BATTLE_TEST("Polished Unburden failed removal behind Substitute leaves its item and flag unchanged")
{
    enum Move move = MOVE_THIEF;
    PARAMETRIZE { move = MOVE_THIEF; }
    PARAMETRIZE { move = MOVE_KNOCK_OFF; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_UNBURDEN); Item(ITEM_POKE_BALL); Speed(100); MaxHP(1000); HP(1000); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); }
    } WHEN {
        TURN { MOVE(player, MOVE_SUBSTITUTE); MOVE(opponent, move); }
    } THEN {
        EXPECT_EQ(player->item, ITEM_POKE_BALL);
        EXPECT_EQ((u32)player->volatiles.unburdenActive, FALSE);
    }
}

SINGLE_BATTLE_TEST("Polished Unburden Bug Bite activates the berry owner rather than its eater")
{
    enum Item berry = ITEM_ORAN_BERRY;
    PARAMETRIZE { berry = ITEM_ORAN_BERRY; }
    PARAMETRIZE { berry = ITEM_PECHA_BERRY; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_UNBURDEN); Item(ITEM_POKE_BALL); Speed(150); MaxHP(400); HP(200); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_UNBURDEN); Item(berry); Speed(100); MaxHP(1000); HP(1000); }
    } WHEN {
        TURN { MOVE(player, MOVE_BUG_BITE); MOVE(opponent, MOVE_SPLASH); }
        TURN { MOVE(player, MOVE_CELEBRATE); MOVE(opponent, MOVE_CELEBRATE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_BUG_BITE, player);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, opponent);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, player);
    } THEN {
        EXPECT_EQ(player->item, ITEM_POKE_BALL);
        EXPECT_EQ(opponent->item, ITEM_NONE);
        EXPECT_EQ((u32)player->volatiles.unburdenActive, FALSE);
        EXPECT_EQ((u32)opponent->volatiles.unburdenActive, TRUE);
    }
}

SINGLE_BATTLE_TEST("Polished Unburden activates after its own berry or White Herb is consumed")
{
    enum Item item = ITEM_ORAN_BERRY;
    PARAMETRIZE { item = ITEM_ORAN_BERRY; }
    PARAMETRIZE { item = ITEM_WHITE_HERB; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_UNBURDEN); Item(item); MaxHP(400); HP(100); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(150); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); MOVE(opponent, item == ITEM_WHITE_HERB ? MOVE_TAIL_WHIP : MOVE_SPLASH); }
        TURN { MOVE(player, MOVE_CELEBRATE); MOVE(opponent, MOVE_CELEBRATE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, player);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, opponent);
    } THEN {
        EXPECT_EQ(player->item, ITEM_NONE);
        EXPECT_EQ((u32)player->volatiles.unburdenActive, TRUE);
    }
}

SINGLE_BATTLE_TEST("Polished Unburden activates when its Air Balloon bursts")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_UNBURDEN); Item(ITEM_AIR_BALLOON); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(150); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); MOVE(opponent, MOVE_TACKLE); }
        TURN { MOVE(player, MOVE_CELEBRATE); MOVE(opponent, MOVE_CELEBRATE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, player);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, opponent);
    } THEN {
        EXPECT_EQ(player->item, ITEM_NONE);
        EXPECT_EQ((u32)player->volatiles.unburdenActive, TRUE);
    }
}

SINGLE_BATTLE_TEST("Polished Unburden cannot earn a boost while Neutralizing Gas suppresses it")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_UNBURDEN); Item(ITEM_ORAN_BERRY); MaxHP(400); HP(100); Speed(100); }
        OPPONENT(SPECIES_WEEZING) { Ability(ABILITY_NEUTRALIZING_GAS); Speed(150); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(150); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); MOVE(opponent, MOVE_SPLASH); }
        TURN { SWITCH(opponent, 1); MOVE(player, MOVE_SPLASH); }
        TURN { MOVE(player, MOVE_CELEBRATE); MOVE(opponent, MOVE_CELEBRATE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, opponent);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, player);
    } THEN {
        EXPECT_EQ(player->item, ITEM_NONE);
        EXPECT_EQ((u32)player->volatiles.unburdenActive, FALSE);
    }
}

SINGLE_BATTLE_TEST("Polished Unburden consumption boost resumes after Neutralizing Gas leaves")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_UNBURDEN); Item(ITEM_ORAN_BERRY); MaxHP(400); HP(100); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(150); }
        OPPONENT(SPECIES_WEEZING) { Ability(ABILITY_NEUTRALIZING_GAS); Speed(150); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); MOVE(opponent, MOVE_SPLASH); }
        TURN { SWITCH(opponent, 1); MOVE(player, MOVE_SPLASH); }
        TURN { MOVE(player, MOVE_CELEBRATE); MOVE(opponent, MOVE_CELEBRATE); }
        TURN { SWITCH(opponent, 0); MOVE(player, MOVE_SPLASH); }
        TURN { MOVE(player, MOVE_CELEBRATE); MOVE(opponent, MOVE_CELEBRATE); }
    } SCENE {
        ABILITY_POPUP(opponent, ABILITY_NEUTRALIZING_GAS);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, opponent);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, player);
        MESSAGE("The effects of the neutralizing gas wore off!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, player);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, opponent);
    } THEN {
        EXPECT_EQ((u32)player->volatiles.unburdenActive, TRUE);
    }
}

SINGLE_BATTLE_TEST("Polished Unburden optional Covet retains native item loss activation")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_UNBURDEN); Item(ITEM_POKE_BALL); }
    } WHEN {
        TURN { MOVE(player, MOVE_COVET); }
    } THEN {
        EXPECT_EQ(player->item, ITEM_POKE_BALL);
        EXPECT_EQ((u32)opponent->volatiles.unburdenActive, TRUE);
    }
}

SINGLE_BATTLE_TEST("Polished Unburden optional Sticky Barb keeps native activation even on Thief")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_UNBURDEN); Item(ITEM_STICKY_BARB); }
    } WHEN {
        TURN { MOVE(player, MOVE_THIEF); }
    } THEN {
        EXPECT_EQ(player->item, ITEM_STICKY_BARB);
        EXPECT_EQ((u32)opponent->volatiles.unburdenActive, TRUE);
    }
}

SINGLE_BATTLE_TEST("Polished Unburden optional Magician retains native activation")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_MAGICIAN); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_UNBURDEN); Item(ITEM_POKE_BALL); }
    } WHEN {
        TURN { MOVE(player, MOVE_WATER_GUN); }
    } THEN {
        EXPECT_EQ(player->item, ITEM_POKE_BALL);
        EXPECT_EQ((u32)opponent->volatiles.unburdenActive, TRUE);
    }
}

DOUBLE_BATTLE_TEST("Polished Unburden doubles retain native Thief and Knock Off activation")
{
    enum Move move = MOVE_THIEF;
    PARAMETRIZE { move = MOVE_THIEF; }
    PARAMETRIZE { move = MOVE_KNOCK_OFF; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_UNBURDEN); Item(ITEM_POKE_BALL); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(playerLeft, move, target: opponentLeft); }
    } THEN {
        EXPECT_EQ(opponentLeft->item, ITEM_NONE);
        EXPECT_EQ((u32)opponentLeft->volatiles.unburdenActive, TRUE);
    }
}

SINGLE_BATTLE_TEST("Polished Unburden active Terastallization retains native item loss activation")
{
    enum Move move = MOVE_THIEF;
    PARAMETRIZE { move = MOVE_THIEF; }
    PARAMETRIZE { move = MOVE_KNOCK_OFF; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { TeraType(TYPE_NORMAL); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_UNBURDEN); Item(ITEM_POKE_BALL); }
    } WHEN {
        TURN { MOVE(player, move, gimmick: GIMMICK_TERA); }
    } THEN {
        EXPECT_EQ(opponent->item, ITEM_NONE);
        EXPECT_EQ((u32)opponent->volatiles.unburdenActive, TRUE);
    }
}
