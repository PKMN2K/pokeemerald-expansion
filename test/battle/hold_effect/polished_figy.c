#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished Figy heals one third without confusion for every nature")
{
    u32 nature = 0;
    for (u32 i = 0; i < NUM_NATURES; i++)
        PARAMETRIZE { nature = i; }
    GIVEN {
        WITH_CONFIG(POLISHED_ITEM_HEALING, TRUE);
        PLAYER(SPECIES_WOBBUFFET) { Nature(nature); HP(100); MaxHP(401); Item(ITEM_FIGY_BERRY); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, player);
        HP_BAR(player, damage: -133);
        NONE_OF { ANIMATION(ANIM_TYPE_STATUS, B_ANIM_STATUS_CONFUSION, player); }
    } THEN {
        EXPECT_EQ(player->hp, 233);
        EXPECT_EQ(player->item, ITEM_NONE);
        EXPECT_EQ((u32)player->volatiles.confusionTimer, 0);
    }
}

SINGLE_BATTLE_TEST("Polished Figy uses inclusive quarter and Gluttony half HP thresholds")
{
    u32 hp, maxHP = 401;
    bool32 gluttony = FALSE, activates;
    PARAMETRIZE { hp = 100; activates = TRUE; }
    PARAMETRIZE { hp = 101; activates = FALSE; }
    PARAMETRIZE { hp = 200; activates = TRUE; gluttony = TRUE; }
    PARAMETRIZE { hp = 201; activates = FALSE; gluttony = TRUE; }
    PARAMETRIZE { hp = 1; maxHP = 2; activates = TRUE; }
    GIVEN {
        WITH_CONFIG(POLISHED_ITEM_HEALING, TRUE);
        PLAYER(SPECIES_SNORLAX) { Ability(gluttony ? ABILITY_GLUTTONY : ABILITY_IMMUNITY); HP(hp); MaxHP(maxHP); Item(ITEM_FIGY_BERRY); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        if (activates)
        {
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, player);
            HP_BAR(player, damage: -max(1, maxHP / 3));
        }
        else
        {
            NONE_OF { ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, player); }
        }
    } THEN {
        EXPECT_EQ(player->hp, hp + (activates ? max(1, maxHP / 3) : 0));
        EXPECT_EQ(player->item, activates ? ITEM_NONE : ITEM_FIGY_BERRY);
    }
}

SINGLE_BATTLE_TEST("Polished Figy triggers once after damage crosses the source threshold")
{
    GIVEN {
        WITH_CONFIG(POLISHED_ITEM_HEALING, TRUE);
        PLAYER(SPECIES_WOBBUFFET) { HP(141); MaxHP(401); Item(ITEM_FIGY_BERRY); Nature(NATURE_BOLD); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(opponent, MOVE_DRAGON_RAGE); }
        TURN { MOVE(opponent, MOVE_DRAGON_RAGE); }
        TURN { MOVE(opponent, MOVE_CELEBRATE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_DRAGON_RAGE, opponent);
        HP_BAR(player, damage: 40);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_DRAGON_RAGE, opponent);
        HP_BAR(player, damage: 40);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, player);
        HP_BAR(player, damage: -133);
        NONE_OF { ANIMATION(ANIM_TYPE_STATUS, B_ANIM_STATUS_CONFUSION, player); }
    } THEN {
        EXPECT_EQ(player->hp, 194);
        EXPECT_EQ(player->item, ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Figy retains native flavor behavior outside the source mapping")
{
    bool32 source = FALSE;
    enum Item item = ITEM_FIGY_BERRY;
    u32 nature = NATURE_BOLD;
    PARAMETRIZE { source = FALSE; }
    PARAMETRIZE { source = TRUE; item = ITEM_WIKI_BERRY; nature = NATURE_ADAMANT; }
    GIVEN {
        WITH_CONFIG(POLISHED_ITEM_HEALING, source);
        PLAYER(SPECIES_WOBBUFFET) { Nature(nature); HP(100); MaxHP(401); Item(item); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE, WITH_RNG(RNG_CONFUSION, FALSE)); }
    } SCENE {
        HP_BAR(player, damage: -133);
        ANIMATION(ANIM_TYPE_STATUS, B_ANIM_STATUS_CONFUSION, player);
    } THEN {
        EXPECT_EQ(player->item, ITEM_NONE);
        EXPECT(player->volatiles.confusionTimer != 0);
    }
}

SINGLE_BATTLE_TEST("Polished Figy remains blocked by Unnerve or Klutz")
{
    bool32 klutz = FALSE;
    PARAMETRIZE { klutz = FALSE; }
    PARAMETRIZE { klutz = TRUE; }
    GIVEN {
        WITH_CONFIG(POLISHED_ITEM_HEALING, TRUE);
        PLAYER(SPECIES_BUNEARY) { Ability(klutz ? ABILITY_KLUTZ : ABILITY_RUN_AWAY); HP(100); MaxHP(401); Item(ITEM_FIGY_BERRY); }
        OPPONENT(SPECIES_JOLTIK) { Ability(klutz ? ABILITY_COMPOUND_EYES : ABILITY_UNNERVE); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        NONE_OF { ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, player); }
    } THEN {
        EXPECT_EQ(player->hp, 100);
        EXPECT_EQ(player->item, ITEM_FIGY_BERRY);
    }
}

SINGLE_BATTLE_TEST("Polished Figy stolen by Bug Bite heals above the pinch without confusing")
{
    u32 hp = 350;
    PARAMETRIZE { hp = 350; }
    PARAMETRIZE { hp = 401; }
    GIVEN {
        WITH_CONFIG(POLISHED_ITEM_HEALING, TRUE);
        PLAYER(SPECIES_WOBBUFFET) { HP(hp); MaxHP(401); Nature(NATURE_BOLD); Item(ITEM_LUM_BERRY); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(401); HP(401); Item(ITEM_FIGY_BERRY); }
    } WHEN {
        TURN { MOVE(player, MOVE_BUG_BITE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_BUG_BITE, player);
        HP_BAR(opponent);
        if (hp < 401)
            HP_BAR(player, damage: -51);
        NONE_OF {
            if (hp == 401)
                HP_BAR(player);
            ANIMATION(ANIM_TYPE_STATUS, B_ANIM_STATUS_CONFUSION, player);
        }
    } THEN {
        EXPECT_EQ(player->hp, 401);
        EXPECT_EQ(player->item, ITEM_LUM_BERRY);
        EXPECT_EQ(opponent->item, ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Figy Cud Chew repeats source healing above the pinch")
{
    GIVEN {
        WITH_CONFIG(POLISHED_ITEM_HEALING, TRUE);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_TAUROS_PALDEA_COMBAT) { Ability(ABILITY_CUD_CHEW); HP(140); MaxHP(401); Nature(NATURE_BOLD); Item(ITEM_FIGY_BERRY); }
    } WHEN {
        TURN { MOVE(player, MOVE_DRAGON_RAGE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_DRAGON_RAGE, player);
        HP_BAR(opponent, damage: 40);
        HP_BAR(opponent, damage: -133);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, player);
        ABILITY_POPUP(opponent, ABILITY_CUD_CHEW);
        HP_BAR(opponent, damage: -133);
        NONE_OF { ANIMATION(ANIM_TYPE_STATUS, B_ANIM_STATUS_CONFUSION, opponent); }
    } THEN {
        EXPECT_EQ(opponent->hp, 366);
        EXPECT_EQ(opponent->item, ITEM_NONE);
    }
}
