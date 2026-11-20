#include "global.h"
#include "test/battle.h"

static const enum Ability sContactStatusAbilities[] = {ABILITY_POISON_POINT, ABILITY_STATIC, ABILITY_FLAME_BODY, ABILITY_EFFECT_SPORE};
static const enum RandomTag sContactStatusRng[] = {RNG_POISON_POINT, RNG_STATIC, RNG_FLAME_BODY, RNG_EFFECT_SPORE};
static const u32 sContactStatuses[] = {STATUS1_POISON, STATUS1_PARALYSIS, STATUS1_BURN, STATUS1_POISON};

SINGLE_BATTLE_TEST("Polished contact source Effect Spore byte selection differs from the native percentage roll")
{
    struct BattlePokemon *user = player, *target = opponent;
    for (u32 side = 0; side < 2; side++)
        PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_NONE : ABILITY_EFFECT_SPORE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_NONE : ABILITY_EFFECT_SPORE); }
    } WHEN {
        TURN { MOVE(user, MOVE_TACKLE, WITH_RNG(RNG_EFFECT_SPORE, 40)); }
    } THEN {
        EXPECT_EQ(user->status1, B_POLISHED_MOVE_BALANCE ? STATUS1_POISON : STATUS1_NONE);
        EXPECT_EQ(target->status1, STATUS1_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished contact defender status abilities activate on the exact Substitute break boundary")
{
    struct BattlePokemon *user = player, *target = opponent;
    u32 ability = 0, maxHP = 200;
    for (u32 side = 0; side < 2; side++)
        for (u32 a = 0; a < ARRAY_COUNT(sContactStatusAbilities); a++)
            for (u32 hp = 196; hp <= 204; hp += 4)
                PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; ability = a; maxHP = hp; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(50); Ability(user == player ? ABILITY_NONE : sContactStatusAbilities[ability]); if (target == player) { MaxHP(maxHP); HP(maxHP); } }
        OPPONENT(SPECIES_WOBBUFFET) { Level(50); Ability(user == opponent ? ABILITY_NONE : sContactStatusAbilities[ability]); if (target == opponent) { MaxHP(maxHP); HP(maxHP); } }
    } WHEN {
        TURN { MOVE(target, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, MOVE_SEISMIC_TOSS, WITH_RNG(sContactStatusRng[ability], ability == 3 ? 0 : TRUE)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SEISMIC_TOSS, user);
        if (B_POLISHED_MOVE_BALANCE && maxHP <= 200) ABILITY_POPUP(target, sContactStatusAbilities[ability]);
        else NOT ABILITY_POPUP(target, sContactStatusAbilities[ability]);
    } THEN {
        EXPECT_EQ(user->status1, B_POLISHED_MOVE_BALANCE && maxHP <= 200 ? sContactStatuses[ability] : STATUS1_NONE);
        EXPECT_EQ((u32)target->volatiles.substituteHP, maxHP > 200 ? 1 : 0);
        EXPECT_EQ(target->hp, maxHP - maxHP / 4);
    }
}

SINGLE_BATTLE_TEST("Polished contact Poison Touch uses the source b equals one Shield Dust and Covert Cloak branch")
{
    struct BattlePokemon *user = player, *target = opponent;
    bool32 cloak = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 item = 0; item < 2; item++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; cloak = item; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_POISON_TOUCH : cloak ? ABILITY_NONE : ABILITY_SHIELD_DUST); Item(target == player && cloak ? ITEM_COVERT_CLOAK : ITEM_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_POISON_TOUCH : cloak ? ABILITY_NONE : ABILITY_SHIELD_DUST); Item(target == opponent && cloak ? ITEM_COVERT_CLOAK : ITEM_NONE); }
    } WHEN {
        TURN { MOVE(user, MOVE_TACKLE, WITH_RNG(RNG_POISON_TOUCH, TRUE)); }
    } THEN {
        EXPECT_EQ(target->status1, B_POLISHED_MOVE_BALANCE ? STATUS1_POISON : STATUS1_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished contact Protective Pads suppress Poison Touch on the source path")
{
    struct BattlePokemon *user = player, *target = opponent;
    for (u32 side = 0; side < 2; side++)
        PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_POISON_TOUCH : ABILITY_NONE); Item(user == player ? ITEM_PROTECTIVE_PADS : ITEM_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_POISON_TOUCH : ABILITY_NONE); Item(user == opponent ? ITEM_PROTECTIVE_PADS : ITEM_NONE); }
    } WHEN {
        TURN { MOVE(user, MOVE_TACKLE, WITH_RNG(RNG_POISON_TOUCH, TRUE)); }
    } THEN {
        EXPECT_EQ(target->status1, B_POLISHED_MOVE_BALANCE ? STATUS1_NONE : STATUS1_POISON);
    }
}

SINGLE_BATTLE_TEST("Polished contact Poison Touch resolves before the defender status ability")
{
    struct BattlePokemon *user = player, *target = opponent;
    for (u32 side = 0; side < 2; side++)
        PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_POISON_TOUCH : ABILITY_STATIC); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_POISON_TOUCH : ABILITY_STATIC); }
    } WHEN {
        TURN { MOVE(user, MOVE_TACKLE, WITH_RNG(RNG_POISON_TOUCH, TRUE)); }
    } SCENE {
        if (B_POLISHED_MOVE_BALANCE) {
            ABILITY_POPUP(user, ABILITY_POISON_TOUCH); STATUS_ICON(target, poison: TRUE);
            ABILITY_POPUP(target, ABILITY_STATIC); STATUS_ICON(user, paralysis: TRUE);
        } else {
            ABILITY_POPUP(target, ABILITY_STATIC); STATUS_ICON(user, paralysis: TRUE);
            ABILITY_POPUP(user, ABILITY_POISON_TOUCH); STATUS_ICON(target, poison: TRUE);
        }
    }
}

SINGLE_BATTLE_TEST("Polished contact Feint Attack uses its source physical contact identity")
{
    struct BattlePokemon *user = player;
    for (u32 side = 0; side < 2; side++)
        PARAMETRIZE { user = side ? opponent : player; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_NONE : ABILITY_FLAME_BODY); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_NONE : ABILITY_FLAME_BODY); }
    } WHEN {
        TURN { MOVE(user, MOVE_FEINT_ATTACK, WITH_RNG(RNG_FLAME_BODY, TRUE)); }
    } THEN {
        EXPECT_EQ(user->status1, B_POLISHED_MOVE_BALANCE || MoveMakesContact(MOVE_FEINT_ATTACK) ? STATUS1_BURN : STATUS1_NONE);
    }
}
