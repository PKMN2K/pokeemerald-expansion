#include "global.h"
#include "test/battle.h"

static const enum Move sSoundMoves[] = {MOVE_GROWL, MOVE_SCREECH, MOVE_SING, MOVE_SUPERSONIC};

static void ExpectSoundEffect(struct BattlePokemon *battler, enum Move move, bool32 affected)
{
    if (move == MOVE_GROWL) EXPECT_EQ(battler->statStages[STAT_ATK], DEFAULT_STAT_STAGE - affected);
    if (move == MOVE_SCREECH) EXPECT_EQ(battler->statStages[STAT_DEF], DEFAULT_STAT_STAGE - 2 * affected);
    if (move == MOVE_SING) EXPECT_EQ(!!(battler->status1 & STATUS1_SLEEP), !!affected);
    if (move == MOVE_SUPERSONIC) EXPECT_EQ(!!battler->volatiles.confusionTimer, !!affected);
}

SINGLE_BATTLE_TEST("Polished sound priority applies Prankster Dark and Soundproof gates")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_GROWL;
    enum Ability ability = ABILITY_NONE;
    bool32 dark = FALSE;
    for (u32 entry = 0; entry < ARRAY_COUNT(sSoundMoves); entry++)
        for (u32 side = 0; side < 2; side++)
            for (u32 kind = 0; kind < 3; kind++)
                for (u32 type = 0; type < 2; type++)
                    PARAMETRIZE { move = sSoundMoves[entry]; user = side ? opponent : player; target = side ? player : opponent; ability = kind == 2 ? ABILITY_MOLD_BREAKER : kind ? ABILITY_PRANKSTER : ABILITY_NONE; dark = type; }
    bool32 succeeds = ability == ABILITY_MOLD_BREAKER || (B_POLISHED_MOVE_BALANCE && ability == ABILITY_PRANKSTER && !dark && move != MOVE_SING);
    GIVEN {
        PLAYER(target == player && dark ? SPECIES_UMBREON : SPECIES_WOBBUFFET) { Speed(100); Ability(user == player ? ability : ABILITY_SOUNDPROOF); MovesWithPP({move, 2}); }
        OPPONENT(target == opponent && dark ? SPECIES_UMBREON : SPECIES_WOBBUFFET) { Speed(50); Ability(user == opponent ? ability : ABILITY_SOUNDPROOF); MovesWithPP({move, 2}); }
    } WHEN {
        TURN { MOVE(user, move, hit: TRUE); }
    } SCENE {
        if (succeeds) ANIMATION(ANIM_TYPE_MOVE, move, user);
        else NOT ANIMATION(ANIM_TYPE_MOVE, move, user);
    } THEN {
        ExpectSoundEffect(target, move, succeeds);
        ExpectSoundEffect(user, move, FALSE);
        EXPECT_EQ(user->pp[0], 1);
    }
}

SINGLE_BATTLE_TEST("Polished sound priority retains positive-priority Armor Tail and Mold Breaker")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_GROWL;
    enum Ability ability = ABILITY_NONE;
    for (u32 entry = 0; entry < ARRAY_COUNT(sSoundMoves); entry++)
        for (u32 side = 0; side < 2; side++)
            for (u32 kind = 0; kind < 3; kind++)
                PARAMETRIZE { move = sSoundMoves[entry]; user = side ? opponent : player; target = side ? player : opponent; ability = kind == 2 ? ABILITY_MOLD_BREAKER : kind ? ABILITY_PRANKSTER : ABILITY_NONE; }
    GIVEN {
        ASSUME(GetMovePriority(move) == 0);
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); Ability(user == player ? ability : ABILITY_ARMOR_TAIL); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); Ability(user == opponent ? ability : ABILITY_ARMOR_TAIL); }
    } WHEN {
        TURN { MOVE(user, move, hit: TRUE); }
    } SCENE {
        if (ability == ABILITY_PRANKSTER) ABILITY_POPUP(target, ABILITY_ARMOR_TAIL);
        else ANIMATION(ANIM_TYPE_MOVE, move, user);
    } THEN {
        ExpectSoundEffect(target, move, ability != ABILITY_PRANKSTER);
    }
}

SINGLE_BATTLE_TEST("Polished sound priority rejects Dark targets before Magic Bounce")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_GROWL;
    bool32 dark = FALSE;
    for (u32 entry = 0; entry < ARRAY_COUNT(sSoundMoves); entry++)
        for (u32 side = 0; side < 2; side++)
            for (u32 type = 0; type < 2; type++)
                PARAMETRIZE { move = sSoundMoves[entry]; user = side ? opponent : player; target = side ? player : opponent; dark = type; }
    bool32 reflected = !dark || !B_POLISHED_MOVE_BALANCE;
    GIVEN {
        PLAYER(target == player && dark ? SPECIES_UMBREON : SPECIES_WOBBUFFET) { Speed(100); Ability(user == player ? ABILITY_PRANKSTER : ABILITY_MAGIC_BOUNCE); }
        OPPONENT(target == opponent && dark ? SPECIES_UMBREON : SPECIES_WOBBUFFET) { Speed(50); Ability(user == opponent ? ABILITY_PRANKSTER : ABILITY_MAGIC_BOUNCE); }
    } WHEN {
        TURN { MOVE(user, move, hit: TRUE); }
    } SCENE {
        if (reflected) ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE);
        else NOT ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE);
    } THEN {
        ExpectSoundEffect(user, move, reflected);
        ExpectSoundEffect(target, move, FALSE);
    }
}

SINGLE_BATTLE_TEST("Polished sound priority respects Protect and the sound Substitute bypass")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_GROWL;
    bool32 substitute = FALSE;
    for (u32 entry = 0; entry < ARRAY_COUNT(sSoundMoves); entry++)
        for (u32 side = 0; side < 2; side++)
            for (u32 shield = 0; shield < 2; shield++)
                PARAMETRIZE { move = sSoundMoves[entry]; user = side ? opponent : player; target = side ? player : opponent; substitute = shield; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); Ability(user == player ? ABILITY_PRANKSTER : ABILITY_SOUNDPROOF); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); Ability(user == opponent ? ABILITY_PRANKSTER : ABILITY_SOUNDPROOF); }
    } WHEN {
        if (substitute) TURN { MOVE(target, MOVE_SUBSTITUTE); }
        TURN { if (!substitute) MOVE(target, MOVE_PROTECT); MOVE(user, move, hit: TRUE); }
    } SCENE {
        if (substitute) ANIMATION(ANIM_TYPE_MOVE, MOVE_SUBSTITUTE, target);
        else ANIMATION(ANIM_TYPE_MOVE, MOVE_PROTECT, target);
        if (substitute && B_POLISHED_MOVE_BALANCE && move != MOVE_SING) ANIMATION(ANIM_TYPE_MOVE, move, user);
        else NOT ANIMATION(ANIM_TYPE_MOVE, move, user);
    } THEN {
        ExpectSoundEffect(target, move, substitute && B_POLISHED_MOVE_BALANCE && move != MOVE_SING);
        if (substitute) EXPECT_GT((u32)target->volatiles.substituteHP, 0);
    }
}

SINGLE_BATTLE_TEST("Polished sound priority retains optional Noble Roar and either active Terastal")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_NOBLE_ROAR;
    u32 tera = 0;
    for (u32 side = 0; side < 2; side++)
        PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; }
    for (u32 entry = 0; entry < ARRAY_COUNT(sSoundMoves); entry++)
        for (u32 gimmick = 1; gimmick <= 2; gimmick++)
            PARAMETRIZE { move = sSoundMoves[entry]; tera = gimmick; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); Ability(user == player ? ABILITY_PRANKSTER : ABILITY_SOUNDPROOF); TeraType(TYPE_NORMAL); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); Ability(user == opponent ? ABILITY_PRANKSTER : ABILITY_SOUNDPROOF); TeraType(TYPE_NORMAL); }
    } WHEN {
        TURN { MOVE(user, move, gimmick: tera == 1 ? GIMMICK_TERA : GIMMICK_NONE); MOVE(target, MOVE_CELEBRATE, gimmick: tera == 2 ? GIMMICK_TERA : GIMMICK_NONE); }
    } SCENE {
        ABILITY_POPUP(target, ABILITY_SOUNDPROOF);
        NOT ANIMATION(ANIM_TYPE_MOVE, move, user);
    } THEN {
        ExpectSoundEffect(target, move, FALSE);
        EXPECT_EQ(target->statStages[STAT_ATK], DEFAULT_STAT_STAGE);
        EXPECT_EQ(target->statStages[STAT_SPATK], DEFAULT_STAT_STAGE);
    }
}

DOUBLE_BATTLE_TEST("Polished sound priority leaves doubles native")
{
    enum Move move = MOVE_GROWL;
    for (u32 entry = 0; entry < ARRAY_COUNT(sSoundMoves); entry++)
        PARAMETRIZE { move = sSoundMoves[entry]; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); Ability(ABILITY_PRANKSTER); }
        PLAYER(SPECIES_WYNAUT) { Speed(75); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); Ability(ABILITY_SOUNDPROOF); }
        OPPONENT(SPECIES_WYNAUT) { Speed(25); Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(playerLeft, move, target: opponentLeft, hit: TRUE); }
    } SCENE {
        ABILITY_POPUP(opponentLeft, ABILITY_SOUNDPROOF);
    } THEN {
        ExpectSoundEffect(opponentLeft, move, FALSE);
        EXPECT_EQ(opponentRight->statStages[STAT_ATK], DEFAULT_STAT_STAGE - (move == MOVE_GROWL));
    }
}

SINGLE_BATTLE_TEST("Polished sound priority uses the actual reflector ability")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_GROWL;
    for (u32 entry = 0; entry < ARRAY_COUNT(sSoundMoves); entry++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = sSoundMoves[entry]; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(user == player ? SPECIES_UMBREON : SPECIES_WOBBUFFET) { Speed(100); Ability(user == player ? ABILITY_PRANKSTER : ABILITY_MAGIC_BOUNCE); }
        OPPONENT(user == opponent ? SPECIES_UMBREON : SPECIES_WOBBUFFET) { Speed(50); Ability(user == opponent ? ABILITY_PRANKSTER : ABILITY_MAGIC_BOUNCE); }
    } WHEN {
        TURN { MOVE(user, move, hit: TRUE); }
    } SCENE {
        ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE);
        ANIMATION(ANIM_TYPE_MOVE, move, target);
    } THEN {
        ExpectSoundEffect(user, move, TRUE);
        ExpectSoundEffect(target, move, FALSE);
    }
}

SINGLE_BATTLE_TEST("Polished sound priority leaves dynamically retyped moves native")
{
    enum Move move = MOVE_GROWL;
    for (u32 entry = 0; entry < ARRAY_COUNT(sSoundMoves); entry++)
        PARAMETRIZE { move = sSoundMoves[entry]; }
    GIVEN {
        ASSUME(GetMovePriority(MOVE_ION_DELUGE) == 1);
        PLAYER(SPECIES_WOBBUFFET) { Speed(50); Ability(ABILITY_PRANKSTER); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(100); Ability(ABILITY_SOUNDPROOF); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_ION_DELUGE); MOVE(player, move, hit: TRUE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_ION_DELUGE, opponent);
        ABILITY_POPUP(opponent, ABILITY_SOUNDPROOF);
        NOT ANIMATION(ANIM_TYPE_MOVE, move, player);
    } THEN {
        ExpectSoundEffect(opponent, move, FALSE);
    }
}
