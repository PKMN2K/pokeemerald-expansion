#include "global.h"
#include "test/battle.h"
#include "battle_util.h"
#include "event_data.h"
#include "constants/flags.h"

SINGLE_BATTLE_TEST("Polished category option uses source table types before battle type conversion", s16 damage)
{
    static const enum Move moves[] = {MOVE_SHADOW_BALL, MOVE_BITE, MOVE_MOONBLAST, MOVE_WEATHER_BALL, MOVE_TACKLE};
    struct BattlePokemon *user = player, *target = opponent;
    u32 kind = 0;
    bool32 classic = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 move = 0; move < ARRAY_COUNT(moves); move++)
            for (u32 setting = 0; setting < 2; setting++)
                PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; kind = move; classic = setting; }
    GIVEN {
        if (classic) FLAG_SET(FLAG_POLISHED_CLASSIC_CATEGORIES);
        else FlagClear(FLAG_POLISHED_CLASSIC_CATEGORIES);
        PLAYER(user == player ? SPECIES_WOBBUFFET : SPECIES_CLEFABLE) { Level(50); Attack(101); SpAttack(203); Defense(73); SpDefense(127); Speed(user == player ? 200 : 100); MaxHP(1000); HP(1000); Ability(kind == 4 && user == player ? ABILITY_GALVANIZE : ABILITY_NONE); }
        OPPONENT(user == opponent ? SPECIES_WOBBUFFET : SPECIES_CLEFABLE) { Level(50); Attack(101); SpAttack(203); Defense(73); SpDefense(127); Speed(user == opponent ? 200 : 100); MaxHP(1000); HP(1000); Ability(kind == 4 && user == opponent ? ABILITY_GALVANIZE : ABILITY_NONE); }
    } WHEN {
        if (kind == 3) TURN { MOVE(user, MOVE_RAIN_DANCE); MOVE(target, MOVE_SPLASH); }
        TURN { MOVE(user, moves[kind], criticalHit: FALSE, secondaryEffect: FALSE, WITH_RNG(RNG_DAMAGE_MODIFIER, 0)); MOVE(target, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, moves[kind], user);
        HP_BAR(target, captureDamage: &results[i].damage);
    } THEN {
        enum DamageCategory expected = GetMoveCategory(moves[kind]);
        if (B_POLISHED_MOVE_BALANCE && classic)
            expected = kind == 0 || kind == 4 ? DAMAGE_CATEGORY_PHYSICAL : DAMAGE_CATEGORY_SPECIAL;
        EXPECT_EQ(GetBattleMoveCategory(moves[kind]), expected);
        EXPECT_EQ(GetBattleMoveCategory(MOVE_GROWL), DAMAGE_CATEGORY_STATUS);
    } FINALLY {
        for (u32 index = 0; index < 2 * ARRAY_COUNT(moves) * 2; index += 2)
        {
            u32 move = (index / 2) % ARRAY_COUNT(moves);
            if (!B_POLISHED_MOVE_BALANCE || move == 2 || move == 3 || move == 4)
                EXPECT_EQ(results[index].damage, results[index + 1].damage);
            else if (move == 1)
                EXPECT_LT(results[index].damage, results[index + 1].damage);
            else
                EXPECT_GT(results[index].damage, results[index + 1].damage);
        }
    }
}

SINGLE_BATTLE_TEST("Polished category option routes actual damage into Counter or Mirror Coat", s16 hit; s16 returned)
{
    struct BattlePokemon *user = player, *target = opponent;
    bool32 classic = FALSE, counter = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 setting = 0; setting < 2; setting++)
            for (u32 effect = 0; effect < 2; effect++)
                PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; classic = setting; counter = effect; }
    GIVEN {
        if (classic) FLAG_SET(FLAG_POLISHED_CLASSIC_CATEGORIES);
        else FlagClear(FLAG_POLISHED_CLASSIC_CATEGORIES);
        PLAYER(SPECIES_WOBBUFFET) { Level(50); Ability(ABILITY_NONE); Attack(101); SpAttack(203); Defense(73); SpDefense(127); Speed(user == player ? 200 : 100); MaxHP(1000); HP(1000); }
        OPPONENT(SPECIES_WOBBUFFET) { Level(50); Ability(ABILITY_NONE); Attack(101); SpAttack(203); Defense(73); SpDefense(127); Speed(user == opponent ? 200 : 100); MaxHP(1000); HP(1000); }
    } WHEN {
        TURN { MOVE(user, MOVE_SHADOW_BALL, criticalHit: FALSE, secondaryEffect: FALSE, WITH_RNG(RNG_DAMAGE_MODIFIER, 0)); MOVE(target, counter ? MOVE_COUNTER : MOVE_MIRROR_COAT); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SHADOW_BALL, user);
        HP_BAR(target, captureDamage: &results[i].hit);
        if (counter == (B_POLISHED_MOVE_BALANCE && classic)) {
            ANIMATION(ANIM_TYPE_MOVE, counter ? MOVE_COUNTER : MOVE_MIRROR_COAT, target);
            HP_BAR(user, captureDamage: &results[i].returned);
        } else {
            NOT HP_BAR(user);
            MESSAGE("But it failed!");
        }
    } THEN {
        if (counter == (B_POLISHED_MOVE_BALANCE && classic)) EXPECT_EQ(results[i].returned, 2 * results[i].hit);
        else EXPECT_EQ(user->hp, 1000);
    }
}

SINGLE_BATTLE_TEST("Polished category option leaves optional moves and either active gimmick native")
{
    u32 tera = 0;
    for (u32 side = 0; side < 3; side++) PARAMETRIZE { tera = side; }
    GIVEN {
        FLAG_SET(FLAG_POLISHED_CLASSIC_CATEGORIES);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); MaxHP(1000); HP(1000); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); MaxHP(1000); HP(1000); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH, gimmick: tera == 1 ? GIMMICK_TERA : GIMMICK_NONE); MOVE(opponent, MOVE_SPLASH, gimmick: tera == 2 ? GIMMICK_TERA : GIMMICK_NONE); }
        // Finish with an actual opposing target. Two self-targeting Splashes
        // leave both global action IDs on one side, outside this API's context.
        TURN { MOVE(player, MOVE_SHADOW_BALL, hit: TRUE); MOVE(opponent, MOVE_AQUA_STEP, hit: TRUE); }
    } THEN {
        EXPECT_EQ(GetActiveGimmick(B_BATTLER_0), tera == 1 ? GIMMICK_TERA : GIMMICK_NONE);
        EXPECT_EQ(GetActiveGimmick(B_BATTLER_1), tera == 2 ? GIMMICK_TERA : GIMMICK_NONE);
        EXPECT_EQ(GetBattleMoveCategory(MOVE_AQUA_STEP), DAMAGE_CATEGORY_PHYSICAL);
        EXPECT_EQ(GetBattleMoveCategory(MOVE_SHADOW_BALL), B_POLISHED_MOVE_BALANCE && tera == 0 ? DAMAGE_CATEGORY_PHYSICAL : DAMAGE_CATEGORY_SPECIAL);
    }
}
