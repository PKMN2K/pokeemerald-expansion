#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished Magic Bounce accuracy preserves original status misses through reflection")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_TOXIC;
    u32 status = STATUS1_TOXIC_POISON;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 4; kind++)
            PARAMETRIZE {
                user = side ? opponent : player; target = side ? player : opponent;
                move = kind == 0 ? MOVE_TOXIC : kind == 1 ? MOVE_THUNDER_WAVE : kind == 2 ? MOVE_WILL_O_WISP : MOVE_SING;
                status = kind == 0 ? STATUS1_TOXIC_POISON : kind == 1 ? STATUS1_PARALYSIS : kind == 2 ? STATUS1_BURN : STATUS1_SLEEP;
            }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_NONE : ABILITY_MAGIC_BOUNCE); Item(ITEM_BRIGHT_POWDER); Speed(user == player ? 50 : 100); MovesWithPP({move, 2}); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_NONE : ABILITY_MAGIC_BOUNCE); Item(ITEM_BRIGHT_POWDER); Speed(user == opponent ? 50 : 100); MovesWithPP({move, 2}); }
    } WHEN {
        TURN { MOVE(user, move, WITH_RNG(RNG_ACCURACY, FALSE)); }
    } SCENE {
        ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE);
        NOT ANIMATION(ANIM_TYPE_MOVE, move, target);
    } THEN {
        EXPECT_EQ(user->status1 & status, 0);
        EXPECT_EQ(target->status1, STATUS1_NONE);
        EXPECT_EQ(user->pp[0], 1);
        EXPECT_EQ(target->pp[0], 2);
    }
}

SINGLE_BATTLE_TEST("Polished Magic Bounce accuracy preserves original stat misses through reflection")
{
    static const struct { enum Move move; u32 stat; } drops[] = {
        {MOVE_LEER, STAT_DEF}, {MOVE_GROWL, STAT_ATK}, {MOVE_SCREECH, STAT_DEF},
        {MOVE_CHARM, STAT_ATK}, {MOVE_SMOKESCREEN, STAT_ACC},
        {MOVE_SCARY_FACE, STAT_SPEED}, {MOVE_STRING_SHOT, STAT_SPEED},
    };
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_NONE;
    u32 stat = STAT_DEF;
    for (u32 side = 0; side < 2; side++)
        for (u32 entry = 0; entry < ARRAY_COUNT(drops); entry++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; move = drops[entry].move; stat = drops[entry].stat; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_NONE : ABILITY_MAGIC_BOUNCE); Item(ITEM_BRIGHT_POWDER); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_NONE : ABILITY_MAGIC_BOUNCE); Item(ITEM_BRIGHT_POWDER); Speed(50); }
    } WHEN {
        TURN { MOVE(user, move, WITH_RNG(RNG_ACCURACY, FALSE)); }
    } SCENE {
        ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE);
        NOT ANIMATION(ANIM_TYPE_MOVE, move, target);
    } THEN {
        EXPECT_EQ(user->statStages[stat], DEFAULT_STAT_STAGE);
        EXPECT_EQ(target->statStages[stat], DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished Magic Bounce accuracy preserves confusion seed and source Foresight misses")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_SUPERSONIC;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 3; kind++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; move = kind == 0 ? MOVE_SUPERSONIC : kind == 1 ? MOVE_LEECH_SEED : MOVE_FORESIGHT; }
    bool32 lands = !B_POLISHED_MOVE_BALANCE && move == MOVE_FORESIGHT;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_NONE : ABILITY_MAGIC_BOUNCE); Item(ITEM_BRIGHT_POWDER); Speed(user == player ? 50 : 100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_NONE : ABILITY_MAGIC_BOUNCE); Item(ITEM_BRIGHT_POWDER); Speed(user == opponent ? 50 : 100); }
    } WHEN {
        // Source Foresight has 255 accuracy, so lower its original chance
        // below 100%. Native Foresight still bypasses the accuracy roll.
        if (move == MOVE_FORESIGHT)
            for (u32 turn = 0; turn < 6; turn++) TURN { MOVE(target, MOVE_DOUBLE_TEAM); }
        TURN { MOVE(user, move, WITH_RNG(RNG_ACCURACY, FALSE)); }
    } SCENE {
        ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE);
        if (lands) ANIMATION(ANIM_TYPE_MOVE, move, target);
        else NOT ANIMATION(ANIM_TYPE_MOVE, move, target);
    } THEN {
        if (move == MOVE_SUPERSONIC) EXPECT_EQ(!!user->volatiles.confusionTimer, lands);
        if (move == MOVE_LEECH_SEED) EXPECT_EQ(!!user->volatiles.leechSeed, lands);
        if (move == MOVE_FORESIGHT) EXPECT_EQ(!!user->volatiles.foresight, lands);
    }
}

SINGLE_BATTLE_TEST("Polished Magic Bounce accuracy reuses an original status hit without a reflected roll")
{
    struct BattlePokemon *user = player, *target = opponent;
    PARAMETRIZE { user = player; target = opponent; }
    PARAMETRIZE { user = opponent; target = player; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_NONE : ABILITY_MAGIC_BOUNCE); Item(user == player ? ITEM_BRIGHT_POWDER : ITEM_NONE); Speed(user == player ? 50 : 100); MovesWithPP({MOVE_CONFUSE_RAY, 2}); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_NONE : ABILITY_MAGIC_BOUNCE); Item(user == opponent ? ITEM_BRIGHT_POWDER : ITEM_NONE); Speed(user == opponent ? 50 : 100); MovesWithPP({MOVE_CONFUSE_RAY, 2}); }
    } WHEN {
        // The original target has no accuracy modifier: Confuse Ray is a
        // certain hit. Only a new reflected roll would see Bright Powder.
        TURN { MOVE(user, MOVE_CONFUSE_RAY, WITH_RNG(RNG_ACCURACY, FALSE)); }
    } SCENE {
        ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE);
        if (B_POLISHED_MOVE_BALANCE) ANIMATION(ANIM_TYPE_MOVE, MOVE_CONFUSE_RAY, target);
        else NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_CONFUSE_RAY, target);
    } THEN {
        EXPECT_EQ(!!user->volatiles.confusionTimer, !!B_POLISHED_MOVE_BALANCE);
        EXPECT_EQ((u32)target->volatiles.confusionTimer, 0);
        EXPECT_EQ(user->pp[0], 1);
        EXPECT_EQ(target->pp[0], 2);
    }
}

SINGLE_BATTLE_TEST("Polished Magic Bounce accuracy restores ordinary rolls after reflection")
{
    struct BattlePokemon *user = player, *target = opponent;
    PARAMETRIZE { user = player; target = opponent; }
    PARAMETRIZE { user = opponent; target = player; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_NONE : ABILITY_MAGIC_BOUNCE); Item(ITEM_BRIGHT_POWDER); Speed(100); MovesWithPP({MOVE_LEER, 3}); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_NONE : ABILITY_MAGIC_BOUNCE); Item(ITEM_BRIGHT_POWDER); Speed(50); MovesWithPP({MOVE_LEER, 3}); }
    } WHEN {
        TURN { MOVE(user, MOVE_LEER, hit: TRUE); }
        TURN { MOVE(target, MOVE_LEER, WITH_RNG(RNG_ACCURACY, FALSE)); }
    } SCENE {
        ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE);
    } THEN {
        EXPECT_EQ(user->statStages[STAT_DEF], DEFAULT_STAT_STAGE - 1);
        EXPECT_EQ(target->statStages[STAT_DEF], DEFAULT_STAT_STAGE);
        EXPECT_EQ(user->pp[0], 2);
        EXPECT_EQ(target->pp[0], 2);
    }
}

SINGLE_BATTLE_TEST("Polished Magic Bounce accuracy retains returned status immunities")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Species species = SPECIES_EKANS;
    enum Move move = MOVE_TOXIC;
    enum Ability ability = ABILITY_NONE;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 4; kind++)
            PARAMETRIZE {
                user = side ? opponent : player; target = side ? player : opponent;
                species = kind == 0 ? SPECIES_EKANS : kind == 1 ? SPECIES_CHARMANDER : kind == 2 ? SPECIES_DIGLETT : SPECIES_WOBBUFFET;
                move = kind == 0 ? MOVE_TOXIC : kind == 1 ? MOVE_WILL_O_WISP : kind == 2 ? MOVE_THUNDER_WAVE : MOVE_SING;
                ability = kind == 3 ? ABILITY_INSOMNIA : ABILITY_NONE;
            }
    GIVEN {
        PLAYER(user == player ? species : SPECIES_WOBBUFFET) { Ability(user == player ? ability : ABILITY_MAGIC_BOUNCE); Speed(user == player ? 50 : 100); }
        OPPONENT(user == opponent ? species : SPECIES_WOBBUFFET) { Ability(user == opponent ? ability : ABILITY_MAGIC_BOUNCE); Speed(user == opponent ? 50 : 100); }
    } WHEN {
        TURN { MOVE(user, move, hit: TRUE); }
    } SCENE {
        ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE);
    } THEN {
        EXPECT_EQ(user->status1, STATUS1_NONE);
        EXPECT_EQ(target->status1, STATUS1_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Magic Bounce accuracy retains original type immunity through reflection")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_TOXIC;
    bool32 immune = FALSE, hit = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 2; kind++)
            for (u32 immunity = 0; immunity < 2; immunity++)
                for (u32 lands = 0; lands < 2; lands++)
                    PARAMETRIZE {
                        user = side ? opponent : player; target = side ? player : opponent;
                        move = kind ? MOVE_THUNDER_WAVE : MOVE_TOXIC; immune = immunity; hit = lands;
                    }
    enum Species reflector = !immune ? SPECIES_WOBBUFFET : move == MOVE_TOXIC ? SPECIES_SKARMORY : SPECIES_DIGLETT;
    u32 status = move == MOVE_TOXIC ? STATUS1_TOXIC_POISON : STATUS1_PARALYSIS;
    bool32 lands = hit && (!B_POLISHED_MOVE_BALANCE || !immune);
    GIVEN {
        PLAYER(target == player ? reflector : SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_NONE : ABILITY_MAGIC_BOUNCE); Item(ITEM_BRIGHT_POWDER); Speed(user == player ? 50 : 100); MovesWithPP({move, 2}); }
        OPPONENT(target == opponent ? reflector : SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_NONE : ABILITY_MAGIC_BOUNCE); Item(ITEM_BRIGHT_POWDER); Speed(user == opponent ? 50 : 100); MovesWithPP({move, 2}); }
    } WHEN {
        // checkpowder/resettypematchup precedes source bounceback. The
        // original immunity remains wAttackMissed after the sides reverse.
        TURN { MOVE(user, move, WITH_RNG(RNG_ACCURACY, hit)); }
    } SCENE {
        ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE);
        if (lands) ANIMATION(ANIM_TYPE_MOVE, move, target);
        else NOT ANIMATION(ANIM_TYPE_MOVE, move, target);
    } THEN {
        EXPECT_EQ(!!(user->status1 & status), lands);
        EXPECT_EQ(target->status1, STATUS1_NONE);
        EXPECT_EQ(user->pp[0], 1);
        EXPECT_EQ(target->pp[0], 2);
    }
}

SINGLE_BATTLE_TEST("Polished Magic Bounce accuracy keeps ordinary and Mold Breaker move rolls")
{
    struct BattlePokemon *user = player, *target = opponent;
    bool32 breaker = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 2; kind++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; breaker = kind; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(!breaker ? ABILITY_NONE : user == player ? ABILITY_MOLD_BREAKER : ABILITY_MAGIC_BOUNCE); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(!breaker ? ABILITY_NONE : user == opponent ? ABILITY_MOLD_BREAKER : ABILITY_MAGIC_BOUNCE); Speed(50); }
    } WHEN {
        TURN { MOVE(user, MOVE_TOXIC, WITH_RNG(RNG_ACCURACY, FALSE)); }
    } SCENE {
        NOT ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE);
    } THEN {
        EXPECT_EQ(user->status1, STATUS1_NONE);
        EXPECT_EQ(target->status1, STATUS1_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Magic Bounce accuracy leaves optional moves Magic Coat and gimmicks native")
{
    struct BattlePokemon *user = player, *target = opponent;
    u32 kind = 0;
    for (u32 side = 0; side < 2; side++)
        for (u32 option = 0; option < 4; option++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; kind = option; }
    enum Move move = kind == 0 ? MOVE_GRASS_WHISTLE : MOVE_TOXIC;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player || kind == 1 ? ABILITY_NONE : ABILITY_MAGIC_BOUNCE); TeraType(TYPE_NORMAL); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent || kind == 1 ? ABILITY_NONE : ABILITY_MAGIC_BOUNCE); TeraType(TYPE_NORMAL); Speed(50); }
    } WHEN {
        if (kind == 3) TURN { MOVE(target, MOVE_CELEBRATE, gimmick: GIMMICK_TERA); }
        TURN { if (kind == 1) MOVE(target, MOVE_MAGIC_COAT); MOVE(user, move, WITH_RNG(RNG_ACCURACY, FALSE), gimmick: kind == 2 ? GIMMICK_TERA : GIMMICK_NONE); }
    } SCENE {
        if (kind != 1) ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE);
        NOT ANIMATION(ANIM_TYPE_MOVE, move, target);
    } THEN {
        EXPECT_EQ(user->status1, STATUS1_NONE);
        EXPECT_EQ(target->status1, STATUS1_NONE);
    }
}

DOUBLE_BATTLE_TEST("Polished Magic Bounce accuracy leaves optional doubles native")
{
    struct BattlePokemon *user = playerLeft, *target = opponentLeft;
    PARAMETRIZE { user = playerLeft; target = opponentLeft; }
    PARAMETRIZE { user = opponentLeft; target = playerLeft; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == playerLeft ? ABILITY_NONE : ABILITY_MAGIC_BOUNCE); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponentLeft ? ABILITY_NONE : ABILITY_MAGIC_BOUNCE); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(user, MOVE_TOXIC, target: target, WITH_RNG(RNG_ACCURACY, FALSE)); }
    } SCENE {
        ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE);
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_TOXIC, target);
    } THEN {
        EXPECT_EQ(user->status1, STATUS1_NONE);
        EXPECT_EQ(target->status1, STATUS1_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Magic Bounce accuracy recognizes Magic Bounce before Magic Coat")
{
    struct BattlePokemon *user = player, *target = opponent;
    PARAMETRIZE { user = player; target = opponent; }
    PARAMETRIZE { user = opponent; target = player; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_NONE : ABILITY_MAGIC_BOUNCE); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_NONE : ABILITY_MAGIC_BOUNCE); Speed(50); }
    } WHEN {
        TURN { MOVE(target, MOVE_MAGIC_COAT); MOVE(user, MOVE_TOXIC, WITH_RNG(RNG_ACCURACY, FALSE)); }
    } SCENE {
        ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE);
    } THEN {
        EXPECT_EQ(user->status1 & STATUS1_TOXIC_POISON, 0);
        EXPECT_EQ(target->status1, STATUS1_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Magic Bounce accuracy reuses the original hit despite reflected evasion")
{
    struct BattlePokemon *user = player, *target = opponent;
    PARAMETRIZE { user = player; target = opponent; }
    PARAMETRIZE { user = opponent; target = player; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_NONE : ABILITY_MAGIC_BOUNCE); Item(user == player ? ITEM_BRIGHT_POWDER : ITEM_NONE); Speed(user == player ? 50 : 100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_NONE : ABILITY_MAGIC_BOUNCE); Item(user == opponent ? ITEM_BRIGHT_POWDER : ITEM_NONE); Speed(user == opponent ? 50 : 100); }
    } WHEN {
        for (u32 turns = 0; turns < 6; turns++) TURN { MOVE(user, MOVE_DOUBLE_TEAM); }
        TURN { MOVE(user, MOVE_LEER, WITH_RNG(RNG_ACCURACY, FALSE)); }
    } SCENE {
        ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE);
        if (B_POLISHED_MOVE_BALANCE) ANIMATION(ANIM_TYPE_MOVE, MOVE_LEER, target);
        else NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_LEER, target);
    } THEN {
        EXPECT_EQ(user->statStages[STAT_EVASION], MAX_STAT_STAGE);
        EXPECT_EQ(user->statStages[STAT_DEF], DEFAULT_STAT_STAGE - !!B_POLISHED_MOVE_BALANCE);
        EXPECT_EQ(target->statStages[STAT_DEF], DEFAULT_STAT_STAGE);
    }
}
