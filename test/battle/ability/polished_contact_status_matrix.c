#include "global.h"
#include "test/battle.h"
#include "polished_contact_vectors.h"

static const enum Ability sAbilities[] = {ABILITY_POISON_POINT, ABILITY_STATIC, ABILITY_FLAME_BODY, ABILITY_EFFECT_SPORE};
static const enum RandomTag sRng[] = {RNG_POISON_POINT, RNG_STATIC, RNG_FLAME_BODY, RNG_EFFECT_SPORE};
static const u32 sStatuses[] = {STATUS1_POISON, STATUS1_PARALYSIS, STATUS1_BURN, STATUS1_POISON};

SINGLE_BATTLE_TEST("Polished contact status type immunities still protect the recipient")
{
    static const enum Species species[] = {SPECIES_ARON, SPECIES_NIDORINO, SPECIES_PIKACHU, SPECIES_CHARMANDER};
    static const u8 abilities[] = {0, 0, 1, 2};
    struct BattlePokemon *user = player;
    u32 mode = 0;
    for (u32 m = 0; m < ARRAY_COUNT(species); m++)
        for (u32 side = 0; side < 2; side++) PARAMETRIZE { mode = m; user = side ? opponent : player; }
    GIVEN {
        PLAYER(user == player ? species[mode] : SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_NONE : sAbilities[abilities[mode]]); }
        OPPONENT(user == opponent ? species[mode] : SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_NONE : sAbilities[abilities[mode]]); }
    } WHEN {
        TURN { MOVE(user, MOVE_TACKLE, WITH_RNG(sRng[abilities[mode]], TRUE)); }
    } THEN {
        EXPECT_EQ(user->status1, STATUS1_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished contact status prevention abilities and sunny Leaf Guard still apply")
{
    static const enum Ability prevent[] = {ABILITY_IMMUNITY, ABILITY_PASTEL_VEIL, ABILITY_LIMBER, ABILITY_WATER_VEIL, ABILITY_INSOMNIA, ABILITY_VITAL_SPIRIT, ABILITY_LEAF_GUARD, ABILITY_LEAF_GUARD, ABILITY_LEAF_GUARD, ABILITY_LEAF_GUARD};
    static const u8 attacking[] = {0, 0, 1, 2, 3, 3, 0, 1, 2, 3};
    struct BattlePokemon *user = player, *target = opponent;
    u32 mode = 0;
    for (u32 m = 0; m < ARRAY_COUNT(prevent); m++)
        for (u32 side = 0; side < 2; side++) PARAMETRIZE { mode = m; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? prevent[mode] : sAbilities[attacking[mode]]); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? prevent[mode] : sAbilities[attacking[mode]]); }
    } WHEN {
        if (mode >= 6) TURN { MOVE(target, MOVE_SUNNY_DAY); }
        TURN { MOVE(user, MOVE_TACKLE, WITH_RNG(sRng[attacking[mode]], attacking[mode] == 3 ? B_POLISHED_MOVE_BALANCE ? 169 : 20 : TRUE)); }
    } THEN {
        EXPECT_EQ(user->status1, STATUS1_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished contact Poison Touch respects suppression poison immunity and Punching Glove")
{
    struct BattlePokemon *user = player, *target = opponent;
    u32 mode = 0;
    for (u32 m = 0; m < 4; m++)
        for (u32 side = 0; side < 2; side++) PARAMETRIZE { mode = m; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(target == player && mode == 2 ? SPECIES_ARON : SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_POISON_TOUCH : mode == 0 ? ABILITY_NEUTRALIZING_GAS : mode == 1 ? ABILITY_IMMUNITY : ABILITY_NONE); Item(user == player && mode == 3 ? ITEM_PUNCHING_GLOVE : ITEM_NONE); }
        OPPONENT(target == opponent && mode == 2 ? SPECIES_ARON : SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_POISON_TOUCH : mode == 0 ? ABILITY_NEUTRALIZING_GAS : mode == 1 ? ABILITY_IMMUNITY : ABILITY_NONE); Item(user == opponent && mode == 3 ? ITEM_PUNCHING_GLOVE : ITEM_NONE); }
    } WHEN {
        TURN { MOVE(user, mode == 3 ? MOVE_MACH_PUNCH : MOVE_TACKLE, WITH_RNG(RNG_POISON_TOUCH, TRUE)); }
    } THEN {
        EXPECT_EQ(target->status1, STATUS1_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished contact optional moves retain native Effect Spore and Poison Touch rules")
{
    struct BattlePokemon *user = player, *target = opponent;
    u32 mode = 0;
    for (u32 m = 0; m < 4; m++)
        for (u32 side = 0; side < 2; side++) PARAMETRIZE { mode = m; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? mode == 1 || mode == 2 ? ABILITY_POISON_TOUCH : ABILITY_NONE : mode == 1 ? ABILITY_SHIELD_DUST : mode == 2 ? ABILITY_NONE : ABILITY_EFFECT_SPORE); Item(user == player && mode == 2 ? ITEM_PROTECTIVE_PADS : ITEM_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? mode == 1 || mode == 2 ? ABILITY_POISON_TOUCH : ABILITY_NONE : mode == 1 ? ABILITY_SHIELD_DUST : mode == 2 ? ABILITY_NONE : ABILITY_EFFECT_SPORE); Item(user == opponent && mode == 2 ? ITEM_PROTECTIVE_PADS : ITEM_NONE); }
    } WHEN {
        TURN { MOVE(user, MOVE_POUND, WITH_RNG(mode == 1 || mode == 2 ? RNG_POISON_TOUCH : RNG_EFFECT_SPORE, mode == 1 || mode == 2 ? TRUE : mode == 3 ? 20 : 40)); }
    } THEN {
        EXPECT_EQ(user->status1, mode == 3 ? STATUS1_SLEEP_TURN(4) : STATUS1_NONE);
        EXPECT_EQ(target->status1, mode == 2 ? STATUS1_POISON : STATUS1_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished contact either active gimmick retains native status selection and protections")
{
    struct BattlePokemon *user = player, *target = opponent;
    u32 mode = 0;
    bool32 targetTera = FALSE;
    for (u32 m = 0; m < 3; m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 who = 0; who < 2; who++) PARAMETRIZE { mode = m; user = side ? opponent : player; target = side ? player : opponent; targetTera = who; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { TeraType(TYPE_NORMAL); Ability(user == player ? mode ? ABILITY_POISON_TOUCH : ABILITY_NONE : mode == 0 ? ABILITY_EFFECT_SPORE : mode == 1 ? ABILITY_SHIELD_DUST : ABILITY_NONE); Item(user == player && mode == 2 ? ITEM_PROTECTIVE_PADS : ITEM_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { TeraType(TYPE_NORMAL); Ability(user == opponent ? mode ? ABILITY_POISON_TOUCH : ABILITY_NONE : mode == 0 ? ABILITY_EFFECT_SPORE : mode == 1 ? ABILITY_SHIELD_DUST : ABILITY_NONE); Item(user == opponent && mode == 2 ? ITEM_PROTECTIVE_PADS : ITEM_NONE); }
    } WHEN {
        TURN {
            if (targetTera) MOVE(target, MOVE_SPLASH, gimmick: GIMMICK_TERA);
            MOVE(user, MOVE_TACKLE, gimmick: targetTera ? GIMMICK_NONE : GIMMICK_TERA, WITH_RNG(mode ? RNG_POISON_TOUCH : RNG_EFFECT_SPORE, mode ? TRUE : 40));
        }
    } THEN {
        EXPECT_EQ(user->status1, STATUS1_NONE);
        EXPECT_EQ(target->status1, mode == 2 ? STATUS1_POISON : STATUS1_NONE);
    }
}

DOUBLE_BATTLE_TEST("Polished contact doubles retain native status selection and protections")
{
    struct BattlePokemon *user = playerLeft, *target = opponentLeft;
    u32 mode = 0;
    for (u32 m = 0; m < 3; m++)
        for (u32 side = 0; side < 2; side++) PARAMETRIZE { mode = m; user = side ? opponentLeft : playerLeft; target = side ? playerLeft : opponentLeft; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == playerLeft ? mode ? ABILITY_POISON_TOUCH : ABILITY_NONE : mode == 0 ? ABILITY_EFFECT_SPORE : mode == 1 ? ABILITY_SHIELD_DUST : ABILITY_NONE); Item(user == playerLeft && mode == 2 ? ITEM_PROTECTIVE_PADS : ITEM_NONE); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponentLeft ? mode ? ABILITY_POISON_TOUCH : ABILITY_NONE : mode == 0 ? ABILITY_EFFECT_SPORE : mode == 1 ? ABILITY_SHIELD_DUST : ABILITY_NONE); Item(user == opponentLeft && mode == 2 ? ITEM_PROTECTIVE_PADS : ITEM_NONE); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(user, MOVE_TACKLE, target: target, WITH_RNG(mode ? RNG_POISON_TOUCH : RNG_EFFECT_SPORE, mode ? TRUE : 40)); }
    } THEN {
        EXPECT_EQ(user->status1, STATUS1_NONE);
        EXPECT_EQ(target->status1, mode == 2 ? STATUS1_POISON : STATUS1_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished contact a multi-hit attack reevaluates both contact abilities after Substitute breaks")
{
    struct BattlePokemon *user = player, *target = opponent;
    for (u32 side = 0; side < 2; side++) PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(50); MaxHP(1000); HP(1000); Attack(300); Defense(100); Ability(user == player ? ABILITY_POISON_TOUCH : ABILITY_STATIC); }
        OPPONENT(SPECIES_WOBBUFFET) { Level(50); MaxHP(1000); HP(1000); Attack(300); Defense(100); Ability(user == opponent ? ABILITY_POISON_TOUCH : ABILITY_STATIC); }
    } WHEN {
        TURN { MOVE(target, MOVE_SUBSTITUTE); }
        for (u32 n = 0; n < 4; n++) TURN { MOVE(user, MOVE_SEISMIC_TOSS); }
        TURN { MOVE(user, MOVE_DRAGON_RAGE); }
        TURN { MOVE(user, MOVE_DOUBLE_KICK, WITH_RNG(RNG_POISON_TOUCH, TRUE)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_DOUBLE_KICK, user);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_SUBSTITUTE_FADE, target);
        if (B_POLISHED_MOVE_BALANCE) { ABILITY_POPUP(target, ABILITY_STATIC); STATUS_ICON(user, paralysis: TRUE); }
        ANIMATION(ANIM_TYPE_MOVE, MOVE_DOUBLE_KICK, user); HP_BAR(target);
        if (!B_POLISHED_MOVE_BALANCE) { ABILITY_POPUP(target, ABILITY_STATIC); STATUS_ICON(user, paralysis: TRUE); }
        ABILITY_POPUP(user, ABILITY_POISON_TOUCH); STATUS_ICON(target, poison: TRUE);
    } THEN {
        EXPECT_EQ(user->status1, STATUS1_PARALYSIS);
        EXPECT_EQ(target->status1, STATUS1_POISON);
    }
}

SINGLE_BATTLE_TEST("Polished contact source classification drives Flame Body for all 124 direct attacks")
{
    const struct PolishedContactVector *vector = &sPolishedContactVectors[0];
    struct BattlePokemon *user = player, *target = opponent;
    for (u32 m = 0; m < ARRAY_COUNT(sPolishedContactVectors); m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { vector = &sPolishedContactVectors[m]; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(user == player ? ABILITY_NONE : ABILITY_FLAME_BODY); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(user == opponent ? ABILITY_NONE : ABILITY_FLAME_BODY); }
    } WHEN {
        TURN { MOVE(user, vector->move, secondaryEffect: FALSE, WITH_RNG(RNG_FLAME_BODY, TRUE)); }
    } THEN {
        EXPECT_EQ(user->status1, (B_POLISHED_MOVE_BALANCE ? vector->contact : MoveMakesContact(vector->move)) ? STATUS1_BURN : STATUS1_NONE);
        EXPECT_LT(target->hp, 1000);
    }
}

SINGLE_BATTLE_TEST("Polished contact Effect Spore resolves both sides of each source byte threshold")
{
    static const u8 rolls[] = {0, 84, 85, 168, 169, 255};
    u32 roll = 0;
    struct BattlePokemon *user = player;
    for (u32 r = 0; r < ARRAY_COUNT(rolls); r++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { roll = rolls[r]; user = side ? opponent : player; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_NONE : ABILITY_EFFECT_SPORE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_NONE : ABILITY_EFFECT_SPORE); }
    } WHEN {
        TURN { MOVE(user, MOVE_TACKLE, WITH_RNG(RNG_EFFECT_SPORE, B_POLISHED_MOVE_BALANCE ? roll : roll < 85 ? 0 : roll < 169 ? 10 : 20)); }
    } THEN {
        if (roll < 85) EXPECT_EQ(user->status1, STATUS1_POISON);
        else if (roll < 169) EXPECT_EQ(user->status1, STATUS1_PARALYSIS);
        else EXPECT_EQ(user->status1, STATUS1_SLEEP_TURN(B_POLISHED_MOVE_BALANCE ? 3 : 4));
    }
}

SINGLE_BATTLE_TEST("Polished contact Effect Spore has a separate three in ten activation roll")
{
    u32 gen = GEN_5;
    for (u32 g = GEN_3; g <= GEN_5; g++) PARAMETRIZE { gen = g; }
    PASSES_RANDOMLY(B_POLISHED_MOVE_BALANCE || gen >= GEN_4 ? 3 : 1, 10, B_POLISHED_MOVE_BALANCE ? RNG_POLISHED_CONTACT_STATUS : RNG_EFFECT_SPORE);
    GIVEN {
        WITH_CONFIG(B_ABILITY_TRIGGER_CHANCE, gen);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_EFFECT_SPORE); }
    } WHEN {
        TURN { MOVE(player, MOVE_TACKLE); }
    } SCENE {
        ABILITY_POPUP(opponent, ABILITY_EFFECT_SPORE);
    }
}

SINGLE_BATTLE_TEST("Polished contact ordinary status activation remains three in ten across generation presets")
{
    u32 gen = GEN_5, ability = 0;
    for (u32 a = 0; a < 3; a++)
        for (u32 g = GEN_3; g <= GEN_5; g++) PARAMETRIZE { ability = a; gen = g; }
    PASSES_RANDOMLY(B_POLISHED_MOVE_BALANCE || gen >= GEN_4 ? 3 : 1, B_POLISHED_MOVE_BALANCE || gen >= GEN_4 ? 10 : 3, sRng[ability]);
    GIVEN {
        WITH_CONFIG(B_ABILITY_TRIGGER_CHANCE, gen);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(sAbilities[ability]); }
    } WHEN {
        TURN { MOVE(player, MOVE_TACKLE); MOVE(opponent, MOVE_SPLASH); }
    } SCENE {
        ABILITY_POPUP(opponent, sAbilities[ability]);
    } THEN {
        EXPECT_EQ(player->status1, sStatuses[ability]);
    }
}

SINGLE_BATTLE_TEST("Polished contact Effect Spore initial sleep counters are uniform and independent of sleep presets")
{
    u32 counter = 1, gen = GEN_5;
    for (u32 g = GEN_3; g <= GEN_5; g += 2)
        for (u32 c = 1; c <= 3; c++) PARAMETRIZE { counter = c; gen = g; }
    PASSES_RANDOMLY(1, B_POLISHED_MOVE_BALANCE || gen == GEN_5 ? 3 : 4, RNG_SLEEP_TURNS);
    GIVEN {
        WITH_CONFIG(B_SLEEP_TURNS, gen);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); MovesWithPP({MOVE_TACKLE, 10}, {MOVE_SWIFT, 10}); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_EFFECT_SPORE); }
    } WHEN {
        TURN { MOVE(player, MOVE_TACKLE, WITH_RNG(RNG_EFFECT_SPORE, B_POLISHED_MOVE_BALANCE ? 169 : 20)); }
        for (u32 turn = 0; turn < counter + (B_POLISHED_MOVE_BALANCE ? 0 : 1); turn++)
            TURN { MOVE(player, MOVE_SWIFT); }
    } SCENE {
        STATUS_ICON(player, sleep: TRUE);
        for (u32 turn = 1; turn < counter + (B_POLISHED_MOVE_BALANCE ? 0 : 1); turn++)
            MESSAGE("Wobbuffet is fast asleep.");
        MESSAGE("Wobbuffet woke up!");
        STATUS_ICON(player, none: TRUE);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SWIFT, player);
    } THEN {
        EXPECT_EQ(player->status1, STATUS1_NONE);
        EXPECT_EQ(player->pp[1], 9);
    }
}

SINGLE_BATTLE_TEST("Polished contact Effect Spore waking and Early Bird consume the source counter at action time")
{
    struct BattlePokemon *user = player;
    bool32 early = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 e = 0; e < 2; e++) PARAMETRIZE { user = side ? opponent : player; early = e; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? early ? ABILITY_EARLY_BIRD : ABILITY_NONE : ABILITY_EFFECT_SPORE); MovesWithPP({MOVE_TACKLE, 10}, {MOVE_SWIFT, 10}); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? early ? ABILITY_EARLY_BIRD : ABILITY_NONE : ABILITY_EFFECT_SPORE); MovesWithPP({MOVE_TACKLE, 10}, {MOVE_SWIFT, 10}); }
    } WHEN {
        TURN { MOVE(user, MOVE_TACKLE, WITH_RNG(RNG_EFFECT_SPORE, B_POLISHED_MOVE_BALANCE ? 169 : 20)); }
        for (u32 turn = 0; turn < (early ? 2 : B_POLISHED_MOVE_BALANCE ? 3 : 4); turn++)
            TURN { MOVE(user, MOVE_SWIFT); }
    } SCENE {
        STATUS_ICON(user, sleep: TRUE);
        STATUS_ICON(user, none: TRUE);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SWIFT, user);
    } THEN {
        EXPECT_EQ(user->status1, STATUS1_NONE);
        EXPECT_EQ(user->pp[0], 9);
        EXPECT_EQ(user->pp[1], 9);
    }
}

SINGLE_BATTLE_TEST("Polished contact rejection and noncontact hits cannot trigger defender statuses")
{
    struct BattlePokemon *user = player, *target = opponent;
    u32 ability = 0, mode = 0;
    for (u32 a = 0; a < ARRAY_COUNT(sAbilities); a++)
        for (u32 side = 0; side < 2; side++)
            for (u32 m = 0; m < 3; m++) PARAMETRIZE { ability = a; mode = m; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_NONE : sAbilities[ability]); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_NONE : sAbilities[ability]); }
    } WHEN {
        if (mode == 0) TURN { MOVE(target, MOVE_FLASH); }
        TURN {
            if (mode == 0) MOVE(user, MOVE_TACKLE, hit: FALSE, WITH_RNG(sRng[ability], ability == 3 ? 0 : TRUE));
            else if (mode == 1) { MOVE(target, MOVE_PROTECT); MOVE(user, MOVE_TACKLE, WITH_RNG(sRng[ability], ability == 3 ? 0 : TRUE)); }
            else MOVE(user, MOVE_SWIFT, WITH_RNG(sRng[ability], ability == 3 ? 0 : TRUE));
        }
    } THEN {
        EXPECT_EQ(user->status1, STATUS1_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished contact defensive statuses ignore the attacking Pokemon Substitute")
{
    struct BattlePokemon *user = player;
    u32 ability = 0;
    for (u32 a = 0; a < ARRAY_COUNT(sAbilities); a++)
        for (u32 side = 0; side < 2; side++) PARAMETRIZE { ability = a; user = side ? opponent : player; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_NONE : sAbilities[ability]); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_NONE : sAbilities[ability]); }
    } WHEN {
        TURN { MOVE(user, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, MOVE_TACKLE, WITH_RNG(sRng[ability], ability == 3 ? 0 : TRUE)); }
    } THEN {
        EXPECT_EQ(user->status1, sStatuses[ability]);
        EXPECT(user->volatiles.substitute);
    }
}

SINGLE_BATTLE_TEST("Polished contact Poison Touch stays blocked on intact and breaking Substitute hits")
{
    struct BattlePokemon *user = player, *target = opponent;
    u32 maxHP = 200;
    for (u32 side = 0; side < 2; side++)
        for (u32 hp = 196; hp <= 204; hp += 4) PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; maxHP = hp; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(50); Ability(user == player ? ABILITY_POISON_TOUCH : ABILITY_NONE); if (target == player) { MaxHP(maxHP); HP(maxHP); } }
        OPPONENT(SPECIES_WOBBUFFET) { Level(50); Ability(user == opponent ? ABILITY_POISON_TOUCH : ABILITY_NONE); if (target == opponent) { MaxHP(maxHP); HP(maxHP); } }
    } WHEN {
        TURN { MOVE(target, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, MOVE_SEISMIC_TOSS, WITH_RNG(RNG_POISON_TOUCH, TRUE)); }
    } THEN {
        EXPECT_EQ(target->status1, STATUS1_NONE);
        EXPECT_EQ((u32)target->volatiles.substituteHP, maxHP > 200 ? 1 : 0);
    }
}

SINGLE_BATTLE_TEST("Polished contact Safeguard protects the status recipient for all five abilities")
{
    struct BattlePokemon *user = player, *target = opponent;
    u32 ability = 0;
    for (u32 a = 0; a < 5; a++)
        for (u32 side = 0; side < 2; side++) PARAMETRIZE { ability = a; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ability == 4 ? ABILITY_POISON_TOUCH : ABILITY_NONE : ability == 4 ? ABILITY_NONE : sAbilities[ability]); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ability == 4 ? ABILITY_POISON_TOUCH : ABILITY_NONE : ability == 4 ? ABILITY_NONE : sAbilities[ability]); }
    } WHEN {
        TURN { MOVE(ability == 4 ? target : user, MOVE_SAFEGUARD); }
        TURN { MOVE(user, MOVE_TACKLE, WITH_RNG(ability == 4 ? RNG_POISON_TOUCH : sRng[ability], ability == 3 ? 0 : TRUE)); }
    } THEN {
        EXPECT_EQ(user->status1, STATUS1_NONE);
        EXPECT_EQ(target->status1, STATUS1_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished contact existing major status is not overwritten")
{
    struct BattlePokemon *user = player;
    u32 ability = 0, status = STATUS1_POISON;
    for (u32 a = 0; a < ARRAY_COUNT(sAbilities); a++)
        for (u32 side = 0; side < 2; side++)
            for (u32 s = 0; s < 2; s++) PARAMETRIZE { ability = a; user = side ? opponent : player; status = s ? STATUS1_BURN : STATUS1_POISON; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_NONE : sAbilities[ability]); Status1(user == player ? status : STATUS1_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_NONE : sAbilities[ability]); Status1(user == opponent ? status : STATUS1_NONE); }
    } WHEN {
        TURN { MOVE(user, MOVE_TACKLE, WITH_RNG(sRng[ability], ability == 3 ? 0 : TRUE)); }
    } THEN {
        EXPECT_EQ(user->status1, status);
    }
}

SINGLE_BATTLE_TEST("Polished contact suppression and Mold Breaker preserve the correct ability owner")
{
    struct BattlePokemon *user = player;
    u32 ability = 0;
    bool32 gas = FALSE;
    for (u32 a = 0; a < ARRAY_COUNT(sAbilities); a++)
        for (u32 side = 0; side < 2; side++)
            for (u32 g = 0; g < 2; g++) PARAMETRIZE { ability = a; user = side ? opponent : player; gas = g; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? gas ? ABILITY_NEUTRALIZING_GAS : ABILITY_MOLD_BREAKER : sAbilities[ability]); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? gas ? ABILITY_NEUTRALIZING_GAS : ABILITY_MOLD_BREAKER : sAbilities[ability]); }
    } WHEN {
        TURN { MOVE(user, MOVE_TACKLE, WITH_RNG(sRng[ability], ability == 3 ? 0 : TRUE)); }
    } THEN {
        EXPECT_EQ(user->status1, gas ? STATUS1_NONE : sStatuses[ability]);
    }
}

SINGLE_BATTLE_TEST("Polished contact powder immunity blocks every Effect Spore outcome")
{
    struct BattlePokemon *user = player;
    u32 guard = 0, status = 0;
    for (u32 side = 0; side < 2; side++)
        for (u32 g = 0; g < 3; g++)
            for (u32 s = 0; s < 3; s++) PARAMETRIZE { user = side ? opponent : player; guard = g; status = s; }
    GIVEN {
        PLAYER(user == player && guard == 0 ? SPECIES_CHIKORITA : SPECIES_WOBBUFFET) { Ability(user == player ? guard == 1 ? ABILITY_OVERCOAT : ABILITY_NONE : ABILITY_EFFECT_SPORE); Item(user == player && guard == 2 ? ITEM_SAFETY_GOGGLES : ITEM_NONE); }
        OPPONENT(user == opponent && guard == 0 ? SPECIES_CHIKORITA : SPECIES_WOBBUFFET) { Ability(user == opponent ? guard == 1 ? ABILITY_OVERCOAT : ABILITY_NONE : ABILITY_EFFECT_SPORE); Item(user == opponent && guard == 2 ? ITEM_SAFETY_GOGGLES : ITEM_NONE); }
    } WHEN {
        TURN { MOVE(user, MOVE_TACKLE, WITH_RNG(RNG_EFFECT_SPORE, status == 0 ? 0 : status == 1 ? B_POLISHED_MOVE_BALANCE ? 85 : 10 : B_POLISHED_MOVE_BALANCE ? 169 : 20)); }
    } THEN {
        EXPECT_EQ(user->status1, STATUS1_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished contact Protective Pads Punching Glove and optional Long Reach block the proper contact")
{
    struct BattlePokemon *user = player;
    u32 ability = 0, guard = 0;
    enum Move move = MOVE_TACKLE;
    for (u32 side = 0; side < 2; side++)
        for (u32 a = 0; a < ARRAY_COUNT(sAbilities); a++)
            for (u32 g = 0; g < 3; g++)
                for (u32 m = 0; m < 2; m++) PARAMETRIZE { user = side ? opponent : player; ability = a; guard = g; move = m ? MOVE_MACH_PUNCH : MOVE_TACKLE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? guard == 2 ? ABILITY_LONG_REACH : ABILITY_NONE : sAbilities[ability]); Item(user == player ? guard == 0 ? ITEM_PROTECTIVE_PADS : guard == 1 ? ITEM_PUNCHING_GLOVE : ITEM_NONE : ITEM_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? guard == 2 ? ABILITY_LONG_REACH : ABILITY_NONE : sAbilities[ability]); Item(user == opponent ? guard == 0 ? ITEM_PROTECTIVE_PADS : guard == 1 ? ITEM_PUNCHING_GLOVE : ITEM_NONE : ITEM_NONE); }
    } WHEN {
        TURN { MOVE(user, move, WITH_RNG(sRng[ability], ability == 3 ? 0 : TRUE)); }
    } THEN {
        EXPECT_EQ(user->status1, guard == 1 && move == MOVE_TACKLE ? sStatuses[ability] : STATUS1_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished contact defender status abilities can activate when their holder faints")
{
    struct BattlePokemon *user = player, *target = opponent;
    u32 ability = 0;
    for (u32 side = 0; side < 2; side++)
        for (u32 a = 0; a < ARRAY_COUNT(sAbilities); a++) PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; ability = a; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_NONE : sAbilities[ability]); HP(user == player ? 100 : 1); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_NONE : sAbilities[ability]); HP(user == opponent ? 100 : 1); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(user, MOVE_TACKLE, WITH_RNG(sRng[ability], ability == 3 ? 0 : TRUE)); SEND_OUT(target, 1); }
    } THEN {
        EXPECT_EQ(user->status1, sStatuses[ability]);
    }
}

SINGLE_BATTLE_TEST("Polished contact Effect Spore source poison byte probability ignores native generation presets")
{
    u32 gen = GEN_5;
    for (u32 g = GEN_3; g <= GEN_5; g++) PARAMETRIZE { gen = g; }
    PASSES_RANDOMLY(B_POLISHED_MOVE_BALANCE ? 85 : gen == GEN_5 ? 9 : 1, B_POLISHED_MOVE_BALANCE ? 256 : gen == GEN_3 ? 30 : gen == GEN_4 ? 10 : 100, RNG_EFFECT_SPORE);
    GIVEN {
        WITH_CONFIG(B_ABILITY_TRIGGER_CHANCE, gen);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_EFFECT_SPORE); }
    } WHEN {
        TURN { MOVE(player, MOVE_TACKLE); }
    } SCENE {
        STATUS_ICON(player, poison: TRUE);
    }
}

SINGLE_BATTLE_TEST("Polished contact Effect Spore source paralysis byte probability ignores native generation presets")
{
    u32 gen = GEN_5;
    for (u32 g = GEN_3; g <= GEN_5; g++) PARAMETRIZE { gen = g; }
    PASSES_RANDOMLY(B_POLISHED_MOVE_BALANCE ? 84 : 1, B_POLISHED_MOVE_BALANCE ? 256 : gen == GEN_3 ? 30 : 10, RNG_EFFECT_SPORE);
    GIVEN {
        WITH_CONFIG(B_ABILITY_TRIGGER_CHANCE, gen);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_EFFECT_SPORE); }
    } WHEN {
        TURN { MOVE(player, MOVE_TACKLE); }
    } SCENE {
        STATUS_ICON(player, paralysis: TRUE);
    }
}

SINGLE_BATTLE_TEST("Polished contact Effect Spore source sleep byte probability ignores native generation presets")
{
    u32 gen = GEN_5;
    for (u32 g = GEN_3; g <= GEN_5; g++) PARAMETRIZE { gen = g; }
    PASSES_RANDOMLY(B_POLISHED_MOVE_BALANCE ? 87 : gen == GEN_5 ? 11 : 1, B_POLISHED_MOVE_BALANCE ? 256 : gen == GEN_3 ? 30 : gen == GEN_4 ? 10 : 100, RNG_EFFECT_SPORE);
    GIVEN {
        WITH_CONFIG(B_ABILITY_TRIGGER_CHANCE, gen);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_EFFECT_SPORE); }
    } WHEN {
        TURN { MOVE(player, MOVE_TACKLE); }
    } SCENE {
        STATUS_ICON(player, sleep: TRUE);
    }
}
