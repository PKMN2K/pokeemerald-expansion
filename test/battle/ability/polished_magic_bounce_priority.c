#include "global.h"
#include "test/battle.h"

static const enum Move sRemainingPriorityMoves[] = {
    MOVE_LEER, MOVE_DISABLE, MOVE_LEECH_SEED, MOVE_POISONPOWDER,
    MOVE_STUN_SPORE, MOVE_SLEEP_POWDER, MOVE_STRING_SHOT, MOVE_THUNDER_WAVE,
    MOVE_TOXIC, MOVE_HYPNOSIS, MOVE_SMOKESCREEN, MOVE_CONFUSE_RAY,
    MOVE_GLARE, MOVE_SPORE, MOVE_FLASH, MOVE_WILL_O_WISP,
    MOVE_SCARY_FACE, MOVE_SWEET_KISS, MOVE_FORESIGHT, MOVE_CHARM,
    MOVE_SWAGGER, MOVE_MEAN_LOOK, MOVE_ATTRACT, MOVE_ENCORE,
};
static const enum Move sEffectMoves[] = {
    MOVE_LEER, MOVE_THUNDER_WAVE, MOVE_WILL_O_WISP, MOVE_POISONPOWDER,
    MOVE_HYPNOSIS, MOVE_CONFUSE_RAY, MOVE_LEECH_SEED, MOVE_MEAN_LOOK,
};

static void ExpectReturnedEffect(struct BattlePokemon *battler, enum Move move, bool32 affected)
{
    if (move == MOVE_LEER) EXPECT_EQ(battler->statStages[STAT_DEF], DEFAULT_STAT_STAGE - affected);
    if (move == MOVE_THUNDER_WAVE) EXPECT_EQ(!!(battler->status1 & STATUS1_PARALYSIS), !!affected);
    if (move == MOVE_WILL_O_WISP) EXPECT_EQ(!!(battler->status1 & STATUS1_BURN), !!affected);
    if (move == MOVE_POISONPOWDER) EXPECT_EQ(!!(battler->status1 & STATUS1_POISON), !!affected);
    if (move == MOVE_HYPNOSIS) EXPECT_EQ(!!(battler->status1 & STATUS1_SLEEP), !!affected);
    if (move == MOVE_CONFUSE_RAY) EXPECT_EQ(!!battler->volatiles.confusionTimer, !!affected);
    if (move == MOVE_LEECH_SEED) EXPECT_EQ(!!battler->volatiles.leechSeed, !!affected);
    if (move == MOVE_MEAN_LOOK) EXPECT_EQ(!!battler->volatiles.escapePrevention, !!affected);
}

SINGLE_BATTLE_TEST("Polished Magic Bounce priority checks Dark immunity for all remaining source candidates")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_LEER;
    bool32 dark = FALSE;
    for (u32 entry = 0; entry < ARRAY_COUNT(sRemainingPriorityMoves); entry++)
        for (u32 side = 0; side < 2; side++)
            for (u32 type = 0; type < 2; type++)
                PARAMETRIZE { move = sRemainingPriorityMoves[entry]; user = side ? opponent : player; target = side ? player : opponent; dark = type; }
    GIVEN {
        PLAYER(target == player && dark ? SPECIES_UMBREON : SPECIES_WOBBUFFET) { Speed(100); Gender(MON_MALE); Ability(user == player ? ABILITY_PRANKSTER : ABILITY_MAGIC_BOUNCE); MovesWithPP({move, 2}); }
        OPPONENT(target == opponent && dark ? SPECIES_UMBREON : SPECIES_WOBBUFFET) { Speed(50); Gender(MON_FEMALE); Ability(user == opponent ? ABILITY_PRANKSTER : ABILITY_MAGIC_BOUNCE); MovesWithPP({move, 2}); }
    } WHEN {
        TURN { MOVE(user, move, hit: TRUE); }
    } SCENE {
        if (dark && B_POLISHED_MOVE_BALANCE) NOT ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE);
        else ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE);
    } THEN {
        EXPECT_EQ(user->pp[0], 1);
    }
}

SINGLE_BATTLE_TEST("Polished Magic Bounce priority prevents returned status stat seed and trap effects")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_LEER;
    bool32 dark = FALSE;
    for (u32 entry = 0; entry < ARRAY_COUNT(sEffectMoves); entry++)
        for (u32 side = 0; side < 2; side++)
            for (u32 type = 0; type < 2; type++)
                PARAMETRIZE { move = sEffectMoves[entry]; user = side ? opponent : player; target = side ? player : opponent; dark = type; }
    GIVEN {
        PLAYER(target == player && dark ? SPECIES_UMBREON : SPECIES_WOBBUFFET) { Speed(100); Ability(user == player ? ABILITY_PRANKSTER : ABILITY_MAGIC_BOUNCE); }
        OPPONENT(target == opponent && dark ? SPECIES_UMBREON : SPECIES_WOBBUFFET) { Speed(50); Ability(user == opponent ? ABILITY_PRANKSTER : ABILITY_MAGIC_BOUNCE); }
    } WHEN {
        TURN { MOVE(user, move, hit: TRUE); }
    } SCENE {
        if (dark && B_POLISHED_MOVE_BALANCE) NOT ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE);
        else ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE);
    } THEN {
        ExpectReturnedEffect(user, move, !dark || !B_POLISHED_MOVE_BALANCE);
        ExpectReturnedEffect(target, move, FALSE);
    }
}

SINGLE_BATTLE_TEST("Polished Magic Bounce priority uses the reflector ability against a Dark original user")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_LEER;
    for (u32 entry = 0; entry < ARRAY_COUNT(sEffectMoves); entry++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = sEffectMoves[entry]; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(user == player ? SPECIES_UMBREON : SPECIES_WOBBUFFET) { Speed(100); Ability(user == player ? ABILITY_PRANKSTER : ABILITY_MAGIC_BOUNCE); }
        OPPONENT(user == opponent ? SPECIES_UMBREON : SPECIES_WOBBUFFET) { Speed(50); Ability(user == opponent ? ABILITY_PRANKSTER : ABILITY_MAGIC_BOUNCE); }
    } WHEN {
        TURN { MOVE(user, move, hit: TRUE); }
    } SCENE {
        ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE);
        ANIMATION(ANIM_TYPE_MOVE, move, target);
    } THEN {
        ExpectReturnedEffect(user, move, TRUE);
        ExpectReturnedEffect(target, move, FALSE);
    }
}

SINGLE_BATTLE_TEST("Polished Magic Bounce priority preserves the two source hazard exceptions")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_SPIKES;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 2; kind++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; move = kind ? MOVE_TOXIC_SPIKES : MOVE_SPIKES; }
    GIVEN {
        PLAYER(target == player ? SPECIES_UMBREON : SPECIES_WOBBUFFET) { Speed(100); Ability(user == player ? ABILITY_PRANKSTER : ABILITY_MAGIC_BOUNCE); }
        OPPONENT(target == opponent ? SPECIES_UMBREON : SPECIES_WOBBUFFET) { Speed(50); Ability(user == opponent ? ABILITY_PRANKSTER : ABILITY_MAGIC_BOUNCE); }
    } WHEN {
        TURN { MOVE(user, move); }
    } SCENE {
        ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE);
        ANIMATION(ANIM_TYPE_MOVE, move, target);
    } THEN {
        u32 side = user == player ? B_SIDE_PLAYER : B_SIDE_OPPONENT;
        if (move == MOVE_SPIKES) { EXPECT_EQ((u32)gSideTimers[side].spikesAmount, 1); EXPECT_EQ((u32)gSideTimers[side ^ 1].spikesAmount, 0); }
        else { EXPECT_EQ((u32)gSideTimers[side].toxicSpikesAmount, 1); EXPECT_EQ((u32)gSideTimers[side ^ 1].toxicSpikesAmount, 0); }
    }
}

SINGLE_BATTLE_TEST("Polished Magic Bounce priority leaves optional Noble Roar native")
{
    struct BattlePokemon *user = player, *target = opponent;
    PARAMETRIZE { user = player; target = opponent; }
    PARAMETRIZE { user = opponent; target = player; }
    GIVEN {
        PLAYER(target == player ? SPECIES_UMBREON : SPECIES_WOBBUFFET) { Speed(100); Ability(user == player ? ABILITY_PRANKSTER : ABILITY_MAGIC_BOUNCE); }
        OPPONENT(target == opponent ? SPECIES_UMBREON : SPECIES_WOBBUFFET) { Speed(50); Ability(user == opponent ? ABILITY_PRANKSTER : ABILITY_MAGIC_BOUNCE); }
    } WHEN {
        TURN { MOVE(user, MOVE_NOBLE_ROAR); }
    } SCENE {
        ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE);
    } THEN {
        EXPECT_EQ(user->statStages[STAT_ATK], DEFAULT_STAT_STAGE - 1);
        EXPECT_EQ(user->statStages[STAT_SPATK], DEFAULT_STAT_STAGE - 1);
    }
}

SINGLE_BATTLE_TEST("Polished Magic Bounce priority leaves either active Terastal native")
{
    static const enum Move moves[] = {MOVE_LEER, MOVE_THUNDER_WAVE, MOVE_HYPNOSIS, MOVE_WILL_O_WISP};
    enum Move move = MOVE_LEER;
    bool32 userTera = FALSE;
    for (u32 entry = 0; entry < ARRAY_COUNT(moves); entry++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = moves[entry]; userTera = side; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); Ability(ABILITY_PRANKSTER); TeraType(TYPE_NORMAL); }
        OPPONENT(SPECIES_UMBREON) { Speed(50); Ability(ABILITY_MAGIC_BOUNCE); TeraType(TYPE_DARK); }
    } WHEN {
        TURN { MOVE(player, move, hit: TRUE, gimmick: userTera ? GIMMICK_TERA : GIMMICK_NONE); MOVE(opponent, MOVE_CELEBRATE, gimmick: userTera ? GIMMICK_NONE : GIMMICK_TERA); }
    } SCENE {
        ABILITY_POPUP(opponent, ABILITY_MAGIC_BOUNCE);
    } THEN {
        ExpectReturnedEffect(player, move, TRUE);
    }
}

DOUBLE_BATTLE_TEST("Polished Magic Bounce priority leaves doubles native")
{
    enum Move move = MOVE_LEER;
    PARAMETRIZE { move = MOVE_LEER; }
    PARAMETRIZE { move = MOVE_TOXIC; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); Ability(ABILITY_PRANKSTER); }
        PLAYER(SPECIES_WYNAUT) { Speed(75); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_UMBREON) { Speed(50); Ability(ABILITY_MAGIC_BOUNCE); }
        OPPONENT(SPECIES_WYNAUT) { Speed(25); Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(playerLeft, move, target: opponentLeft, hit: TRUE); }
    } SCENE {
        ABILITY_POPUP(opponentLeft, ABILITY_MAGIC_BOUNCE);
    } THEN {
        if (move == MOVE_TOXIC) EXPECT(playerLeft->status1 & STATUS1_TOXIC_POISON);
        else EXPECT_EQ(playerLeft->statStages[STAT_DEF], DEFAULT_STAT_STAGE - 1);
    }
}

SINGLE_BATTLE_TEST("Polished Magic Bounce priority leaves dynamically retyped moves native")
{
    static const enum Move moves[] = {MOVE_LEER, MOVE_FLASH, MOVE_SCARY_FACE, MOVE_SMOKESCREEN};
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_LEER;
    bool32 hit = FALSE;
    for (u32 entry = 0; entry < ARRAY_COUNT(moves); entry++)
        for (u32 side = 0; side < 2; side++)
            for (u32 lands = 0; lands < 2; lands++)
                PARAMETRIZE { move = moves[entry]; user = side ? opponent : player; target = side ? player : opponent; hit = lands; }
    GIVEN {
        PLAYER(target == player ? SPECIES_UMBREON : SPECIES_WOBBUFFET) { Speed(user == player ? 50 : 100); Ability(user == player ? ABILITY_PRANKSTER : ABILITY_MAGIC_BOUNCE); Item(ITEM_BRIGHT_POWDER); }
        OPPONENT(target == opponent ? SPECIES_UMBREON : SPECIES_WOBBUFFET) { Speed(user == opponent ? 50 : 100); Ability(user == opponent ? ABILITY_PRANKSTER : ABILITY_MAGIC_BOUNCE); Item(ITEM_BRIGHT_POWDER); }
    } WHEN {
        TURN { MOVE(target, MOVE_ION_DELUGE); MOVE(user, move, WITH_RNG(RNG_ACCURACY, hit)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_ION_DELUGE, target);
        ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE);
        if (hit) ANIMATION(ANIM_TYPE_MOVE, move, target);
        else NOT ANIMATION(ANIM_TYPE_MOVE, move, target);
    } THEN {
        enum Stat stat = move == MOVE_LEER ? STAT_DEF : move == MOVE_SCARY_FACE ? STAT_SPEED : STAT_ACC;
        u32 amount = move == MOVE_SCARY_FACE ? 2 : 1;
        EXPECT_EQ(user->statStages[stat], DEFAULT_STAT_STAGE - (hit ? amount : 0));
        EXPECT_EQ(target->statStages[stat], DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished Magic Bounce priority leaves optional Magic Coat ordering native")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_LEER;
    for (u32 entry = 0; entry < ARRAY_COUNT(sEffectMoves); entry++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = sEffectMoves[entry]; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(target == player ? SPECIES_UMBREON : SPECIES_WOBBUFFET) { Speed(100); Ability(user == player ? ABILITY_PRANKSTER : ABILITY_NONE); }
        OPPONENT(target == opponent ? SPECIES_UMBREON : SPECIES_WOBBUFFET) { Speed(50); Ability(user == opponent ? ABILITY_PRANKSTER : ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(target, MOVE_MAGIC_COAT); MOVE(user, move, hit: TRUE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_MAGIC_COAT, target);
        ANIMATION(ANIM_TYPE_MOVE, move, target);
    } THEN {
        ExpectReturnedEffect(user, move, TRUE);
        ExpectReturnedEffect(target, move, FALSE);
    }
}
