#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished typeless moves keep Struggle neutral against Ghost Rock Steel and ordinary types", s16 damage)
{
    enum Species species = SPECIES_WOBBUFFET;
    PARAMETRIZE { species = SPECIES_WOBBUFFET; }
    PARAMETRIZE { species = SPECIES_GASTLY; }
    PARAMETRIZE { species = SPECIES_GEODUDE; }
    PARAMETRIZE { species = SPECIES_STEELIX; }
    PARAMETRIZE { species = SPECIES_MACHOP; }
    GIVEN {
        PLAYER(SPECIES_SNORLAX) { Ability(ABILITY_NONE); Attack(100); MaxHP(1000); HP(1000); }
        OPPONENT(species) { Ability(ABILITY_NONE); Defense(100); MaxHP(1000); HP(1000); }
    } WHEN {
        TURN { MOVE(player, MOVE_STRUGGLE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_STRUGGLE, player);
        HP_BAR(opponent, captureDamage: &results[i].damage);
        HP_BAR(player);
        NONE_OF { MESSAGE("It's super effective!"); MESSAGE("It's not very effective…"); }
    } FINALLY {
        EXPECT_GT(results[0].damage, 0);
        for (u32 n = 1; n < 5; n++)
            EXPECT_EQ(results[0].damage, results[n].damage);
    }
}

SINGLE_BATTLE_TEST("Polished typeless moves give Struggle no Normal STAB or Normal item boost", s16 damage)
{
    enum Species species = SPECIES_WOBBUFFET;
    enum Item item = ITEM_NONE;
    PARAMETRIZE { species = SPECIES_WOBBUFFET; item = ITEM_NONE; }
    PARAMETRIZE { species = SPECIES_SNORLAX; item = ITEM_NONE; }
    PARAMETRIZE { species = SPECIES_SNORLAX; item = ITEM_SILK_SCARF; }
    GIVEN {
        PLAYER(species) { Ability(ABILITY_NONE); Item(item); Attack(100); MaxHP(1000); HP(1000); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Defense(100); MaxHP(1000); HP(1000); }
    } WHEN {
        TURN { MOVE(player, MOVE_STRUGGLE); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_EQ(results[0].damage, results[1].damage);
        EXPECT_EQ(results[0].damage, results[2].damage);
    }
}

SINGLE_BATTLE_TEST("Polished typeless moves keep Struggle typeless with Normalize and Pixilate", s16 damage)
{
    enum Ability ability = ABILITY_NONE;
    PARAMETRIZE { ability = ABILITY_NONE; }
    PARAMETRIZE { ability = ABILITY_NORMALIZE; }
    PARAMETRIZE { ability = ABILITY_PIXILATE; }
    GIVEN {
        PLAYER(SPECIES_SNORLAX) { Ability(ability); Attack(100); MaxHP(1000); HP(1000); }
        OPPONENT(SPECIES_GASTLY) { Ability(ABILITY_NONE); Defense(100); MaxHP(1000); HP(1000); }
    } WHEN {
        TURN { MOVE(player, MOVE_STRUGGLE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_STRUGGLE, player);
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } THEN {
        enum BattlerId battler = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);
        EXPECT_EQ(GetDynamicMoveType(GetBattlerMon(battler), MOVE_STRUGGLE, battler, ability, HOLD_EFFECT_NONE, MON_IN_BATTLE), TYPE_MYSTERY);
    } FINALLY {
        EXPECT_EQ(results[0].damage, results[1].damage);
        EXPECT_EQ(results[0].damage, results[2].damage);
    }
}

SINGLE_BATTLE_TEST("Polished typeless moves still let Protect block Struggle without recoil")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_GASTLY);
    } WHEN {
        TURN { MOVE(opponent, MOVE_PROTECT); MOVE(player, MOVE_STRUGGLE); }
    } SCENE {
        MESSAGE("The opposing Gastly protected itself!");
        NONE_OF { HP_BAR(player); HP_BAR(opponent); }
    } THEN {
        EXPECT_EQ(player->hp, player->maxHP);
        EXPECT_EQ(opponent->hp, opponent->maxHP);
    }
}

SINGLE_BATTLE_TEST("Polished typeless moves make Metronome use the called move type and STAB", s16 damage)
{
    bool32 called = FALSE;
    PARAMETRIZE { called = FALSE; }
    PARAMETRIZE { called = TRUE; }
    GIVEN {
        PLAYER(SPECIES_SQUIRTLE) { Ability(ABILITY_NONE); SpAttack(100); if (called) Moves(MOVE_METRONOME); else Moves(MOVE_WATER_GUN); }
        OPPONENT(SPECIES_GEODUDE) { Ability(ABILITY_NONE); SpDefense(100); MaxHP(1000); HP(1000); }
    } WHEN {
        if (called)
            TURN { MOVE(player, MOVE_METRONOME, WITH_RNG(RNG_METRONOME, MOVE_WATER_GUN)); }
        else
            TURN { MOVE(player, MOVE_WATER_GUN); }
    } SCENE {
        if (called)
            ANIMATION(ANIM_TYPE_MOVE, MOVE_METRONOME, player);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_WATER_GUN, player);
        HP_BAR(opponent, captureDamage: &results[i].damage);
        MESSAGE("It's extremely effective!");
    } THEN {
        if (called)
            EXPECT_EQ(GetMonData(GetBattlerMon(GetBattlerAtPosition(B_POSITION_PLAYER_LEFT)), MON_DATA_PP1), GetMovePP(MOVE_METRONOME) - 1);
    } FINALLY {
        EXPECT_EQ(results[0].damage, results[1].damage);
    }
}

SINGLE_BATTLE_TEST("Polished typeless moves do not bypass Ghost immunity for Metronome's called Normal attack")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_GASTLY);
    } WHEN {
        TURN { MOVE(player, MOVE_METRONOME, WITH_RNG(RNG_METRONOME, MOVE_TACKLE)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_METRONOME, player);
        MESSAGE("It doesn't affect the opposing Gastly…");
        NOT HP_BAR(opponent);
    } THEN {
        EXPECT_EQ(opponent->hp, opponent->maxHP);
    }
}

SINGLE_BATTLE_TEST("Polished typeless moves retain protection and absorption for Metronome's called attack")
{
    bool32 protect = FALSE;
    PARAMETRIZE { protect = FALSE; }
    PARAMETRIZE { protect = TRUE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_VAPOREON) { Ability(ABILITY_WATER_ABSORB); MaxHP(200); HP(100); }
    } WHEN {
        if (protect)
            TURN { MOVE(opponent, MOVE_PROTECT); MOVE(player, MOVE_METRONOME, WITH_RNG(RNG_METRONOME, MOVE_WATER_GUN)); }
        else
            TURN { MOVE(player, MOVE_METRONOME, WITH_RNG(RNG_METRONOME, MOVE_WATER_GUN)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_METRONOME, player);
        if (!protect)
            HP_BAR(opponent);
        else
            NOT HP_BAR(opponent);
    } THEN {
        EXPECT_EQ(opponent->hp, protect ? 100 : 150);
    }
}
