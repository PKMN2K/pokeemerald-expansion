#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished retaliation accuracy bypasses accuracy drops, evasion boosts and Bright Powder")
{
    enum Move move = MOVE_COUNTER, attack = MOVE_POUND;
    u32 modifier = 0;
    for (u32 m = 0; m < 2; m++)
        for (u32 n = 0; n < 3; n++)
            PARAMETRIZE { move = m ? MOVE_MIRROR_COAT : MOVE_COUNTER; attack = m ? MOVE_WATER_GUN : MOVE_POUND; modifier = n; }
    s16 damage, reflected;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HP(500); MaxHP(500); Ability(ABILITY_NONE); Moves(move); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(500); MaxHP(500); Ability(ABILITY_NONE); Item(modifier == 2 ? ITEM_BRIGHT_POWDER : ITEM_NONE); }
    } WHEN {
        if (modifier == 0)
            TURN { MOVE(opponent, MOVE_SAND_ATTACK); }
        if (modifier == 1)
            TURN { MOVE(opponent, MOVE_DOUBLE_TEAM); }
        TURN { MOVE(opponent, attack); MOVE(player, move, WITH_RNG(RNG_ACCURACY, FALSE)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, attack, opponent);
        HP_BAR(player, captureDamage: &damage);
        if (B_POLISHED_RETALIATION_ACCURACY)
        {
            ANIMATION(ANIM_TYPE_MOVE, move, player);
            HP_BAR(opponent, captureDamage: &reflected);
        }
        else
            NONE_OF { ANIMATION(ANIM_TYPE_MOVE, move, player); HP_BAR(opponent); }
    } THEN {
        if (B_POLISHED_RETALIATION_ACCURACY)
            EXPECT_EQ(reflected, damage * 2);
        EXPECT_EQ(opponent->hp, B_POLISHED_RETALIATION_ACCURACY ? 500 - damage * 2 : 500);
        EXPECT_EQ(GetMonData(GetBattlerMon(GetBattlerAtPosition(B_POSITION_PLAYER_LEFT)), MON_DATA_PP1), 19);
    }
}

SINGLE_BATTLE_TEST("Polished retaliation accuracy retains double damage for either user side and nonzero matchups")
{
    enum Move move = MOVE_COUNTER, attack = MOVE_POUND;
    enum Species species = SPECIES_WOBBUFFET;
    u32 side = 0;
    const enum Species targets[] = {SPECIES_WOBBUFFET, SPECIES_MACHOP, SPECIES_ARON};
    for (u32 m = 0; m < 2; m++)
        for (u32 s = 0; s < 2; s++)
            for (u32 t = 0; t < ARRAY_COUNT(targets); t++)
                PARAMETRIZE { move = m ? MOVE_MIRROR_COAT : MOVE_COUNTER; attack = m ? MOVE_WATER_GUN : MOVE_POUND; side = s; species = targets[t]; }
    s16 damage, reflected;
    GIVEN {
        PLAYER(side ? species : SPECIES_WOBBUFFET) { HP(500); MaxHP(500); Ability(ABILITY_NONE); }
        OPPONENT(side ? SPECIES_WOBBUFFET : species) { HP(500); MaxHP(500); Ability(ABILITY_NONE); }
    } WHEN {
        if (side)
            TURN { MOVE(player, attack); MOVE(opponent, move); }
        else
            TURN { MOVE(opponent, attack); MOVE(player, move); }
    } SCENE {
        if (side)
        {
            ANIMATION(ANIM_TYPE_MOVE, attack, player);
            HP_BAR(opponent, captureDamage: &damage);
            ANIMATION(ANIM_TYPE_MOVE, move, opponent);
            HP_BAR(player, captureDamage: &reflected);
        }
        else
        {
            ANIMATION(ANIM_TYPE_MOVE, attack, opponent);
            HP_BAR(player, captureDamage: &damage);
            ANIMATION(ANIM_TYPE_MOVE, move, player);
            HP_BAR(opponent, captureDamage: &reflected);
        }
    } THEN {
        EXPECT_EQ(reflected, damage * 2);
    }
}

SINGLE_BATTLE_TEST("Polished retaliation accuracy still requires damage of the matching category this turn")
{
    enum Move move = MOVE_COUNTER, attack = MOVE_WATER_GUN;
    for (u32 m = 0; m < 2; m++)
        for (u32 status = 0; status < 2; status++)
            PARAMETRIZE { move = m ? MOVE_MIRROR_COAT : MOVE_COUNTER; attack = status ? MOVE_SPLASH : m ? MOVE_POUND : MOVE_WATER_GUN; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HP(500); MaxHP(500); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(500); MaxHP(500); }
    } WHEN {
        TURN { MOVE(opponent, attack); MOVE(player, move); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, attack, opponent);
        NONE_OF { ANIMATION(ANIM_TYPE_MOVE, move, player); HP_BAR(opponent); }
    }
}

SINGLE_BATTLE_TEST("Polished retaliation accuracy preserves Ghost and Dark immunities")
{
    enum Move move = MOVE_COUNTER, attack = MOVE_POUND;
    enum Species target = SPECIES_GASTLY;
    PARAMETRIZE { move = MOVE_COUNTER; attack = MOVE_POUND; target = SPECIES_GASTLY; }
    PARAMETRIZE { move = MOVE_MIRROR_COAT; attack = MOVE_WATER_GUN; target = SPECIES_UMBREON; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HP(500); MaxHP(500); Ability(ABILITY_NO_GUARD); }
        OPPONENT(target) { HP(500); MaxHP(500); }
    } WHEN {
        TURN { MOVE(opponent, attack); MOVE(player, move); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, attack, opponent);
        HP_BAR(player);
        NONE_OF { ANIMATION(ANIM_TYPE_MOVE, move, player); HP_BAR(opponent); }
    }
}

SINGLE_BATTLE_TEST("Polished retaliation accuracy does not reflect damage absorbed by the user's Substitute")
{
    enum Move move = MOVE_COUNTER, attack = MOVE_POUND;
    PARAMETRIZE { move = MOVE_COUNTER; attack = MOVE_POUND; }
    PARAMETRIZE { move = MOVE_MIRROR_COAT; attack = MOVE_WATER_GUN; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HP(500); MaxHP(500); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(500); MaxHP(500); }
    } WHEN {
        TURN { MOVE(player, MOVE_SUBSTITUTE); }
        TURN { MOVE(opponent, attack); MOVE(player, move); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SUBSTITUTE, player);
        HP_BAR(player, damage: 125);
        ANIMATION(ANIM_TYPE_MOVE, attack, opponent);
        NONE_OF { ANIMATION(ANIM_TYPE_MOVE, move, player); HP_BAR(opponent); }
    }
}

SINGLE_BATTLE_TEST("Polished retaliation accuracy cannot reuse previous-turn damage")
{
    enum Move move = MOVE_COUNTER, attack = MOVE_POUND;
    PARAMETRIZE { move = MOVE_COUNTER; attack = MOVE_POUND; }
    PARAMETRIZE { move = MOVE_MIRROR_COAT; attack = MOVE_WATER_GUN; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HP(500); MaxHP(500); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(500); MaxHP(500); }
    } WHEN {
        TURN { MOVE(opponent, attack); }
        TURN { MOVE(player, move); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, attack, opponent);
        HP_BAR(player);
        NONE_OF { ANIMATION(ANIM_TYPE_MOVE, move, player); HP_BAR(opponent); }
    }
}
