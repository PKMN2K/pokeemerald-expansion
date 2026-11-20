#include "global.h"
#include "test/battle.h"

static const enum Move sVariableMoves[] = {MOVE_DOUBLE_SLAP, MOVE_PIN_MISSILE, MOVE_ROCK_BLAST, MOVE_ICICLE_SPEAR, MOVE_POLISHED_FURY_STRIKES};
static const enum Move sAllMoves[] = {MOVE_DOUBLE_SLAP, MOVE_DOUBLE_KICK, MOVE_PIN_MISSILE, MOVE_ROCK_BLAST, MOVE_ICICLE_SPEAR, MOVE_POLISHED_FURY_STRIKES, MOVE_BONEMERANG};
static const enum Move sContactMoves[] = {MOVE_DOUBLE_SLAP, MOVE_POLISHED_FURY_STRIKES, MOVE_DOUBLE_KICK};
static const u8 sSourceCounts[] = {2, 3, 4, 5, 2, 3};

static u32 MaxHits(enum Move move)
{
    return move == MOVE_DOUBLE_KICK || move == MOVE_BONEMERANG ? 2 : 5;
}

SINGLE_BATTLE_TEST("Polished multihit existing sleep and flinch still prevent starting an attack")
{
    enum Move move = MOVE_DOUBLE_SLAP;
    struct BattlePokemon *user = player, *target = opponent;
    bool32 asleep = FALSE;
    for (u32 m = 0; m < ARRAY_COUNT(sAllMoves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 s = 0; s < 2; s++)
                PARAMETRIZE { move = sAllMoves[m]; user = side ? opponent : player; target = side ? player : opponent; asleep = s; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Attack(10); Ability(ABILITY_SKILL_LINK); if (user == player) { Status1(asleep ? STATUS1_SLEEP_TURN(3) : STATUS1_NONE); MovesWithPP({move, 10}); } }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Attack(10); Ability(ABILITY_SKILL_LINK); if (user == opponent) { Status1(asleep ? STATUS1_SLEEP_TURN(3) : STATUS1_NONE); MovesWithPP({move, 10}); } }
    } WHEN {
        TURN { if (!asleep) MOVE(target, MOVE_FAKE_OUT); MOVE(user, move); }
    } SCENE {
        NOT ANIMATION(ANIM_TYPE_MOVE, move, user);
    } THEN {
        EXPECT_EQ(user->pp[0], 10);
        EXPECT_EQ(gMultiHitCounter, 0);
    }
}

SINGLE_BATTLE_TEST("Polished multihit spends PP once and Pressure adds only one PP")
{
    enum Move move = MOVE_DOUBLE_SLAP;
    struct BattlePokemon *user = player, *target = opponent;
    bool32 pressure = FALSE;
    for (u32 m = 0; m < ARRAY_COUNT(sAllMoves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 a = 0; a < 2; a++)
                PARAMETRIZE { move = sAllMoves[m]; user = side ? opponent : player; target = side ? player : opponent; pressure = a; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Attack(10); Ability(user == player ? ABILITY_SKILL_LINK : pressure ? ABILITY_PRESSURE : ABILITY_NONE); MovesWithPP({move, 10}); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Attack(10); Ability(user == opponent ? ABILITY_SKILL_LINK : pressure ? ABILITY_PRESSURE : ABILITY_NONE); MovesWithPP({move, 10}); }
    } WHEN {
        TURN { MOVE(user, move); }
    } SCENE {
        for (u32 hit = 0; hit < MaxHits(move); hit++) { ANIMATION(ANIM_TYPE_MOVE, move, user); HP_BAR(target); }
    } THEN {
        EXPECT_EQ(user->pp[0], pressure ? 8 : 9);
        EXPECT_EQ(gMultiHitCounter, 0);
    }
}

SINGLE_BATTLE_TEST("Polished multihit miss and Protect abort before the first strike")
{
    enum Move move = MOVE_DOUBLE_SLAP;
    struct BattlePokemon *user = player, *target = opponent;
    bool32 protect = FALSE;
    for (u32 m = 0; m < ARRAY_COUNT(sAllMoves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 p = 0; p < 2; p++)
                PARAMETRIZE { move = sAllMoves[m]; user = side ? opponent : player; target = side ? player : opponent; protect = p; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Attack(10); Ability(ABILITY_SKILL_LINK); MovesWithPP({move, 10}, {MOVE_FLASH, 10}, {MOVE_PROTECT, 10}); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Attack(10); Ability(ABILITY_SKILL_LINK); MovesWithPP({move, 10}, {MOVE_FLASH, 10}, {MOVE_PROTECT, 10}); }
    } WHEN {
        if (!protect) TURN { MOVE(target, MOVE_FLASH); }
        TURN {
            if (protect) { MOVE(target, MOVE_PROTECT); MOVE(user, move); }
            else MOVE(user, move, hit: FALSE);
        }
    } SCENE {
        NONE_OF { ANIMATION(ANIM_TYPE_MOVE, move, user); HP_BAR(target); }
    } THEN {
        EXPECT_EQ(user->pp[0], 9);
        EXPECT_EQ(target->hp, 1000);
        EXPECT_EQ(gMultiHitCounter, 0);
    }
}

SINGLE_BATTLE_TEST("Polished multihit accuracy is checked once for the entire sequence")
{
    enum Move move = MOVE_DOUBLE_SLAP;
    u32 accuracy = 85;
    for (u32 m = 0; m < ARRAY_COUNT(sAllMoves); m++)
        PARAMETRIZE { move = sAllMoves[m]; accuracy = move == MOVE_DOUBLE_SLAP || move == MOVE_POLISHED_FURY_STRIKES ? 85 : move == MOVE_DOUBLE_KICK || move == MOVE_ICICLE_SPEAR ? 100 : move == MOVE_BONEMERANG || move == MOVE_ROCK_BLAST ? 90 : 95; }
    PASSES_RANDOMLY(accuracy, 100, RNG_ACCURACY);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Attack(10); Ability(ABILITY_SKILL_LINK); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(player, move); }
    } SCENE {
        for (u32 hit = 0; hit < MaxHits(move); hit++) ANIMATION(ANIM_TYPE_MOVE, move, player);
        if (MaxHits(move) == 2) MESSAGE("The Pokémon was hit 2 times!");
        else MESSAGE("The Pokémon was hit 5 times!");
    }
}

SINGLE_BATTLE_TEST("Polished multihit processes intact and broken Substitute separately on each strike")
{
    enum Move move = MOVE_DOUBLE_SLAP;
    struct BattlePokemon *user = player, *target = opponent;
    bool32 broken = FALSE;
    for (u32 m = 0; m < ARRAY_COUNT(sAllMoves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 b = 0; b < 2; b++)
                PARAMETRIZE { move = sAllMoves[m]; user = side ? opponent : player; target = side ? player : opponent; broken = b; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(50); MaxHP(1000); HP(1000); Attack(broken ? 300 : 10); Defense(100); Ability(ABILITY_SKILL_LINK); }
        OPPONENT(SPECIES_WOBBUFFET) { Level(50); MaxHP(1000); HP(1000); Attack(broken ? 300 : 10); Defense(100); Ability(ABILITY_SKILL_LINK); }
    } WHEN {
        TURN { MOVE(target, MOVE_SUBSTITUTE); }
        if (broken)
        {
            for (u32 i = 0; i < 4; i++) TURN { MOVE(user, MOVE_SEISMIC_TOSS); }
            TURN { MOVE(user, MOVE_DRAGON_RAGE); }
        }
        TURN { MOVE(user, move); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SUBSTITUTE, target);
        if (broken)
        {
            for (u32 i = 0; i < 4; i++) { ANIMATION(ANIM_TYPE_MOVE, MOVE_SEISMIC_TOSS, user); SUB_HIT(target); }
            ANIMATION(ANIM_TYPE_MOVE, MOVE_DRAGON_RAGE, user); SUB_HIT(target);
        }
        for (u32 hit = 0; hit < MaxHits(move); hit++)
        {
            ANIMATION(ANIM_TYPE_MOVE, move, user);
            if (broken && hit > 0) HP_BAR(target);
            else SUB_HIT(target);
        }
    } THEN {
        if (broken) EXPECT_LT(target->hp, 750);
        else EXPECT_EQ(target->hp, 750);
        EXPECT_GT(target->hp, 0);
        EXPECT_EQ(gMultiHitCounter, 0);
    }
}

SINGLE_BATTLE_TEST("Polished multihit target fainting stops further strikes and clears next-turn state")
{
    enum Move move = MOVE_DOUBLE_SLAP;
    struct BattlePokemon *user = player, *target = opponent;
    for (u32 m = 0; m < ARRAY_COUNT(sAllMoves); m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = sAllMoves[m]; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(1000); HP(user == player ? 1000 : 1); Attack(10000); Ability(ABILITY_SKILL_LINK); MovesWithPP({move, 10}, {MOVE_TACKLE, 5}); }
        PLAYER(SPECIES_WYNAUT) { MaxHP(1000); HP(1000); Defense(10000); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(1000); HP(user == opponent ? 1000 : 1); Attack(10000); Ability(ABILITY_SKILL_LINK); MovesWithPP({move, 10}, {MOVE_TACKLE, 5}); }
        OPPONENT(SPECIES_WYNAUT) { MaxHP(1000); HP(1000); Defense(10000); Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(user, move); SEND_OUT(target, 1); }
        TURN { MOVE(user, MOVE_TACKLE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, user);
        HP_BAR(target);
        MESSAGE("The Pokémon was hit 1 time!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, user);
        HP_BAR(target);
    } THEN {
        EXPECT_EQ(user->pp[0], 9);
        EXPECT_EQ(user->pp[1], 4);
        EXPECT_EQ(gMultiHitCounter, 0);
    }
}

SINGLE_BATTLE_TEST("Polished multihit attacker fainting to Rocky Helmet ends the attack")
{
    enum Move move = MOVE_DOUBLE_SLAP;
    struct BattlePokemon *user = player, *target = opponent;
    for (u32 m = 0; m < ARRAY_COUNT(sContactMoves); m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = sContactMoves[m]; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(user == player ? 200 : 100); MaxHP(1000); HP(user == player ? 1 : 1000); Attack(10); Ability(ABILITY_SKILL_LINK); Item(user == player ? ITEM_NONE : ITEM_ROCKY_HELMET); }
        PLAYER(SPECIES_WYNAUT) { Speed(50); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(user == opponent ? 200 : 100); MaxHP(1000); HP(user == opponent ? 1 : 1000); Attack(10); Ability(ABILITY_SKILL_LINK); Item(user == opponent ? ITEM_NONE : ITEM_ROCKY_HELMET); }
        OPPONENT(SPECIES_WYNAUT) { Speed(50); Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(user, move); SEND_OUT(user, 1); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, user);
        HP_BAR(target);
        HP_BAR(user);
        NOT ANIMATION(ANIM_TYPE_MOVE, move, user);
    } THEN {
        EXPECT_EQ(gMultiHitCounter, 0);
    }
}

SINGLE_BATTLE_TEST("Polished multihit contact paralysis and burn do not interrupt remaining strikes")
{
    enum Move move = MOVE_DOUBLE_SLAP;
    struct BattlePokemon *user = player, *target = opponent;
    bool32 burn = FALSE;
    for (u32 m = 0; m < ARRAY_COUNT(sContactMoves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 b = 0; b < 2; b++)
                PARAMETRIZE { move = sContactMoves[m]; user = side ? opponent : player; target = side ? player : opponent; burn = b; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Attack(10); Ability(user == player ? ABILITY_SKILL_LINK : burn ? ABILITY_FLAME_BODY : ABILITY_STATIC); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Attack(10); Ability(user == opponent ? ABILITY_SKILL_LINK : burn ? ABILITY_FLAME_BODY : ABILITY_STATIC); }
    } WHEN {
        TURN { MOVE(user, move, WITH_RNG(burn ? RNG_FLAME_BODY : RNG_STATIC, TRUE)); }
    } SCENE {
        for (u32 hit = 0; hit < MaxHits(move); hit++) { ANIMATION(ANIM_TYPE_MOVE, move, user); HP_BAR(target); }
    } THEN {
        EXPECT_EQ(user->status1, burn ? STATUS1_BURN : STATUS1_PARALYSIS);
    }
}

SINGLE_BATTLE_TEST("Polished multihit recalculates damage after each Weak Armor activation")
{
    enum Move move = MOVE_DOUBLE_SLAP;
    struct BattlePokemon *user = player, *target = opponent;
    s16 first, last;
    for (u32 m = 0; m < ARRAY_COUNT(sAllMoves); m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = sAllMoves[m]; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Attack(100); Defense(200); Ability(user == player ? ABILITY_SKILL_LINK : ABILITY_WEAK_ARMOR); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Attack(100); Defense(200); Ability(user == opponent ? ABILITY_SKILL_LINK : ABILITY_WEAK_ARMOR); }
    } WHEN {
        TURN { MOVE(user, move); }
    } SCENE {
        for (u32 hit = 0; hit < MaxHits(move); hit++)
        {
            ANIMATION(ANIM_TYPE_MOVE, move, user);
            if (hit == 0) HP_BAR(target, captureDamage: &first);
            else if (hit == MaxHits(move) - 1) HP_BAR(target, captureDamage: &last);
            else HP_BAR(target);
            ABILITY_POPUP(target, ABILITY_WEAK_ARMOR);
        }
    } THEN {
        EXPECT_GT(last, first);
        EXPECT_EQ(target->statStages[STAT_DEF], DEFAULT_STAT_STAGE - MaxHits(move));
        EXPECT_EQ(target->statStages[STAT_SPEED], min(MAX_STAT_STAGE, DEFAULT_STAT_STAGE + MaxHits(move) * 2));
    }
}

SINGLE_BATTLE_TEST("Polished multihit handles critical and ordinary hits across the whole sequence")
{
    enum Move move = MOVE_DOUBLE_SLAP;
    struct BattlePokemon *user = player, *target = opponent;
    bool32 critical = FALSE;
    for (u32 m = 0; m < ARRAY_COUNT(sAllMoves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 crit = 0; crit < 2; crit++)
                PARAMETRIZE { move = sAllMoves[m]; user = side ? opponent : player; target = side ? player : opponent; critical = crit; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Attack(10); Ability(ABILITY_SKILL_LINK); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Attack(10); Ability(ABILITY_SKILL_LINK); }
    } WHEN {
        TURN { MOVE(user, move, criticalHit: critical); }
    } SCENE {
        for (u32 hit = 0; hit < MaxHits(move); hit++) { ANIMATION(ANIM_TYPE_MOVE, move, user); HP_BAR(target); if (critical) MESSAGE("A critical hit!"); else NOT MESSAGE("A critical hit!"); }
    }
}

SINGLE_BATTLE_TEST("Polished multihit Loaded Dice gives four and five hits equally")
{
    enum Move move = MOVE_DOUBLE_SLAP;
    for (u32 m = 0; m < ARRAY_COUNT(sVariableMoves); m++)
        PARAMETRIZE { move = sVariableMoves[m]; }
    PASSES_RANDOMLY(1, 2, RNG_LOADED_DICE);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Attack(10); Ability(ABILITY_NONE); Item(ITEM_LOADED_DICE); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(player, move); }
    } SCENE {
        for (u32 hit = 0; hit < 4; hit++) ANIMATION(ANIM_TYPE_MOVE, move, player);
        MESSAGE("The Pokémon was hit 4 times!");
    }
}

SINGLE_BATTLE_TEST("Polished multihit uses active Skill Link and Loaded Dice after suppression")
{
    enum Move move = MOVE_DOUBLE_SLAP;
    struct BattlePokemon *user = player, *target = opponent;
    u32 mode = 0;
    for (u32 m = 0; m < ARRAY_COUNT(sVariableMoves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 a = 0; a < 3; a++)
                PARAMETRIZE { move = sVariableMoves[m]; user = side ? opponent : player; target = side ? player : opponent; mode = a; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Attack(10); Ability(user == player ? mode == 0 ? ABILITY_SKILL_LINK : mode == 1 ? ABILITY_KLUTZ : ABILITY_NONE : mode == 0 ? ABILITY_NEUTRALIZING_GAS : mode == 2 ? ABILITY_UNNERVE : ABILITY_NONE); Item(user == player && mode != 0 ? ITEM_LOADED_DICE : ITEM_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Attack(10); Ability(user == opponent ? mode == 0 ? ABILITY_SKILL_LINK : mode == 1 ? ABILITY_KLUTZ : ABILITY_NONE : mode == 0 ? ABILITY_NEUTRALIZING_GAS : mode == 2 ? ABILITY_UNNERVE : ABILITY_NONE); Item(user == opponent && mode != 0 ? ITEM_LOADED_DICE : ITEM_NONE); }
    } WHEN {
        TURN { MOVE(user, move, WITH_RNG(mode == 2 ? RNG_LOADED_DICE : RNG_HITS, mode == 2 ? 4 : B_POLISHED_MOVE_BALANCE ? 0 : 2)); }
    } SCENE {
        for (u32 hit = 0; hit < (mode == 2 ? 4 : 2); hit++) { ANIMATION(ANIM_TYPE_MOVE, move, user); HP_BAR(target); }
        if (mode == 2) MESSAGE("The Pokémon was hit 4 times!");
        else MESSAGE("The Pokémon was hit 2 times!");
    }
}

SINGLE_BATTLE_TEST("Polished multihit Cursed Body does not cancel an attack already in progress")
{
    enum Move move = MOVE_DOUBLE_SLAP;
    struct BattlePokemon *user = player, *target = opponent;
    for (u32 m = 0; m < ARRAY_COUNT(sAllMoves); m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = sAllMoves[m]; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Attack(10); Ability(user == player ? ABILITY_SKILL_LINK : ABILITY_CURSED_BODY); MovesWithPP({move, 10}); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Attack(10); Ability(user == opponent ? ABILITY_SKILL_LINK : ABILITY_CURSED_BODY); MovesWithPP({move, 10}); }
    } WHEN {
        TURN { MOVE(user, move, WITH_RNG(RNG_CURSED_BODY, TRUE)); }
    } SCENE {
        for (u32 hit = 0; hit < MaxHits(move); hit++) { ANIMATION(ANIM_TYPE_MOVE, move, user); HP_BAR(target); }
    } THEN {
        EXPECT_GT((u32)user->volatiles.disableTimer, 0);
        EXPECT_EQ(user->pp[0], 9);
    }
}

SINGLE_BATTLE_TEST("Polished multihit called through Metronome uses the executed move and only caller PP")
{
    static const u16 ids[] = {3, 24, 42, 106, 131, 154, 155};
    enum Move move = MOVE_DOUBLE_SLAP;
    u32 sourceId = 3;
    struct BattlePokemon *user = player, *target = opponent;
    for (u32 m = 0; m < ARRAY_COUNT(sAllMoves); m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE {
                // Native Metronome excludes the custom Fury Strikes identity.
                // Exercise its optional Fury Swipes counterpart in that profile.
                move = !B_POLISHED_MOVE_BALANCE && sAllMoves[m] == MOVE_POLISHED_FURY_STRIKES ? MOVE_FURY_SWIPES : sAllMoves[m];
                sourceId = ids[m]; user = side ? opponent : player; target = side ? player : opponent;
            }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Attack(10); Ability(ABILITY_NONE); MovesWithPP({MOVE_METRONOME, 10}); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Attack(10); Ability(ABILITY_NONE); MovesWithPP({MOVE_METRONOME, 10}); }
    } WHEN {
        TURN { MOVE(user, MOVE_METRONOME, WITH_RNG(RNG_METRONOME, B_POLISHED_MOVE_BALANCE ? sourceId : move)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_METRONOME, user);
        for (u32 hit = 0; hit < (MaxHits(move) == 2 ? 2 : B_POLISHED_MOVE_BALANCE ? 3 : 5); hit++) { ANIMATION(ANIM_TYPE_MOVE, move, user); HP_BAR(target); }
    } THEN {
        EXPECT_EQ(user->pp[0], 9);
    }
}

SINGLE_BATTLE_TEST("Polished multihit powder immunity prevents contact sleep without shortening the attack")
{
    enum Move move = MOVE_DOUBLE_SLAP;
    struct BattlePokemon *user = player, *target = opponent;
    bool32 goggles = FALSE;
    for (u32 m = 0; m < ARRAY_COUNT(sContactMoves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 item = 0; item < 2; item++)
                PARAMETRIZE { move = sContactMoves[m]; user = side ? opponent : player; target = side ? player : opponent; goggles = item; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Attack(10); Ability(user == player ? goggles ? ABILITY_SKILL_LINK : ABILITY_OVERCOAT : ABILITY_EFFECT_SPORE); Item(user == player ? goggles ? ITEM_SAFETY_GOGGLES : ITEM_LOADED_DICE : ITEM_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Attack(10); Ability(user == opponent ? goggles ? ABILITY_SKILL_LINK : ABILITY_OVERCOAT : ABILITY_EFFECT_SPORE); Item(user == opponent ? goggles ? ITEM_SAFETY_GOGGLES : ITEM_LOADED_DICE : ITEM_NONE); }
    } WHEN {
        TURN { MOVE(user, move, WITH_RNG(RNG_EFFECT_SPORE, 20)); }
    } SCENE {
        for (u32 hit = 0; hit < MaxHits(move); hit++) { ANIMATION(ANIM_TYPE_MOVE, move, user); HP_BAR(target); }
    } THEN {
        EXPECT_EQ(user->status1, STATUS1_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished multihit optional attacks retain their native sleep interruption")
{
    enum Move move = MOVE_FURY_SWIPES;
    struct BattlePokemon *user = player, *target = opponent;
    for (u32 m = 0; m < 2; m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = m ? MOVE_DOUBLE_HIT : MOVE_FURY_SWIPES; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Attack(10); Ability(user == player ? ABILITY_SKILL_LINK : ABILITY_EFFECT_SPORE); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Attack(10); Ability(user == opponent ? ABILITY_SKILL_LINK : ABILITY_EFFECT_SPORE); }
    } WHEN {
        TURN { MOVE(user, move, WITH_RNG(RNG_EFFECT_SPORE, 20)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, user); HP_BAR(target);
        STATUS_ICON(user, sleep: TRUE);
        NOT ANIMATION(ANIM_TYPE_MOVE, move, user);
    }
}

SINGLE_BATTLE_TEST("Polished multihit selects every source slot independently of generation presets")
{
    enum Move move = MOVE_DOUBLE_SLAP;
    struct BattlePokemon *user = player, *target = opponent;
    u32 roll = 0, generation = GEN_5;
    for (u32 m = 0; m < ARRAY_COUNT(sVariableMoves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 r = 0; r < 6; r++)
                for (u32 g = GEN_4; g <= GEN_5; g++)
                    PARAMETRIZE { move = sVariableMoves[m]; user = side ? opponent : player; target = side ? player : opponent; roll = r; generation = g; }
    GIVEN {
        WITH_CONFIG(B_MULTI_HIT_CHANCE, generation);
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Attack(10); Ability(ABILITY_NONE); MovesWithPP({move, 10}); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Attack(10); Ability(ABILITY_NONE); MovesWithPP({move, 10}); }
    } WHEN {
        TURN { MOVE(user, move, WITH_RNG(RNG_HITS, B_POLISHED_MOVE_BALANCE ? roll : sSourceCounts[roll])); }
    } SCENE {
        for (u32 hit = 0; hit < sSourceCounts[roll]; hit++)
        {
            ANIMATION(ANIM_TYPE_MOVE, move, user);
            HP_BAR(target);
        }
        if (sSourceCounts[roll] == 2) MESSAGE("The Pokémon was hit 2 times!");
        if (sSourceCounts[roll] == 3) MESSAGE("The Pokémon was hit 3 times!");
        if (sSourceCounts[roll] == 4) MESSAGE("The Pokémon was hit 4 times!");
        if (sSourceCounts[roll] == 5) MESSAGE("The Pokémon was hit 5 times!");
    } THEN {
        EXPECT_EQ(user->pp[0], 9);
        EXPECT_EQ(gMultiHitCounter, 0);
    }
}

SINGLE_BATTLE_TEST("Polished multihit fixed double strikes ignore Skill Link and Loaded Dice")
{
    enum Move move = MOVE_DOUBLE_KICK;
    struct BattlePokemon *user = player, *target = opponent;
    bool32 skill = FALSE, dice = FALSE;
    for (u32 m = 0; m < 2; m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 a = 0; a < 2; a++)
                for (u32 i = 0; i < 2; i++)
                    PARAMETRIZE { move = m ? MOVE_BONEMERANG : MOVE_DOUBLE_KICK; user = side ? opponent : player; target = side ? player : opponent; skill = a; dice = i; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Attack(10); Ability(skill ? ABILITY_SKILL_LINK : ABILITY_NONE); Item(dice ? ITEM_LOADED_DICE : ITEM_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Attack(10); Ability(skill ? ABILITY_SKILL_LINK : ABILITY_NONE); Item(dice ? ITEM_LOADED_DICE : ITEM_NONE); }
    } WHEN {
        TURN { MOVE(user, move); }
    } SCENE {
        for (u32 hit = 0; hit < 2; hit++) { ANIMATION(ANIM_TYPE_MOVE, move, user); HP_BAR(target); }
        MESSAGE("The Pokémon was hit 2 times!");
    }
}

SINGLE_BATTLE_TEST("Polished multihit Skill Link takes priority over the Loaded Dice roll")
{
    enum Move move = MOVE_DOUBLE_SLAP;
    struct BattlePokemon *user = player, *target = opponent;
    bool32 dice = FALSE;
    for (u32 m = 0; m < ARRAY_COUNT(sVariableMoves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 item = 0; item < 2; item++)
                PARAMETRIZE { move = sVariableMoves[m]; user = side ? opponent : player; target = side ? player : opponent; dice = item; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Attack(10); Ability(ABILITY_SKILL_LINK); Item(dice ? ITEM_LOADED_DICE : ITEM_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Attack(10); Ability(ABILITY_SKILL_LINK); Item(dice ? ITEM_LOADED_DICE : ITEM_NONE); }
    } WHEN {
        TURN { MOVE(user, move, WITH_RNG(RNG_LOADED_DICE, 4)); }
    } SCENE {
        for (u32 hit = 0; hit < 5; hit++) { ANIMATION(ANIM_TYPE_MOVE, move, user); HP_BAR(target); }
        MESSAGE("The Pokémon was hit 5 times!");
    }
}

SINGLE_BATTLE_TEST("Polished multihit Loaded Dice resolves either endpoint on both sides")
{
    enum Move move = MOVE_DOUBLE_SLAP;
    struct BattlePokemon *user = player, *target = opponent;
    u32 hits = 4;
    for (u32 m = 0; m < ARRAY_COUNT(sVariableMoves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 count = 4; count <= 5; count++)
                PARAMETRIZE { move = sVariableMoves[m]; user = side ? opponent : player; target = side ? player : opponent; hits = count; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Attack(10); Ability(ABILITY_NONE); Item(ITEM_LOADED_DICE); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Attack(10); Ability(ABILITY_NONE); Item(ITEM_LOADED_DICE); }
    } WHEN {
        TURN { MOVE(user, move, WITH_RNG(RNG_LOADED_DICE, hits)); }
    } SCENE {
        for (u32 hit = 0; hit < hits; hit++) { ANIMATION(ANIM_TYPE_MOVE, move, user); HP_BAR(target); }
        if (hits == 4) MESSAGE("The Pokémon was hit 4 times!");
        else MESSAGE("The Pokémon was hit 5 times!");
    }
}

SINGLE_BATTLE_TEST("Polished multihit completes the current loop after Effect Spore sleep")
{
    enum Move move = MOVE_DOUBLE_SLAP;
    struct BattlePokemon *user = player, *target = opponent;
    for (u32 m = 0; m < ARRAY_COUNT(sContactMoves); m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = sContactMoves[m]; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(user == player ? 200 : 100); MaxHP(1000); HP(1000); Attack(10); Ability(user == player ? ABILITY_SKILL_LINK : ABILITY_EFFECT_SPORE); MovesWithPP({move, 10}, {MOVE_TACKLE, 5}); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(user == opponent ? 200 : 100); MaxHP(1000); HP(1000); Attack(10); Ability(user == opponent ? ABILITY_SKILL_LINK : ABILITY_EFFECT_SPORE); MovesWithPP({move, 10}, {MOVE_TACKLE, 5}); }
    } WHEN {
        TURN { MOVE(user, move, WITH_RNG(RNG_EFFECT_SPORE, 20)); }
        TURN { MOVE(user, MOVE_TACKLE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, user);
        HP_BAR(target);
        STATUS_ICON(user, sleep: TRUE);
        for (u32 hit = 1; hit < (B_POLISHED_MOVE_BALANCE ? MaxHits(move) : 1); hit++)
        { ANIMATION(ANIM_TYPE_MOVE, move, user); HP_BAR(target); }
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, user);
    } THEN {
        EXPECT_NE(user->status1 & STATUS1_SLEEP, 0);
        EXPECT_EQ(user->pp[0], 9);
        EXPECT_EQ(user->pp[1], 5);
        EXPECT_EQ(gMultiHitCounter, 0);
    }
}

SINGLE_BATTLE_TEST("Polished multihit source probability for 2 hits covers every variable attack")
{
    enum Move move = MOVE_DOUBLE_SLAP;
    for (u32 m = 0; m < ARRAY_COUNT(sVariableMoves); m++)
        PARAMETRIZE { move = sVariableMoves[m]; }
    PASSES_RANDOMLY(B_POLISHED_MOVE_BALANCE || move == MOVE_POLISHED_FURY_STRIKES ? 1 : 7, B_POLISHED_MOVE_BALANCE || move == MOVE_POLISHED_FURY_STRIKES ? 3 : 20, RNG_HITS);
    GIVEN {
        WITH_CONFIG(B_MULTI_HIT_CHANCE, GEN_5);
        PLAYER(SPECIES_WOBBUFFET) { Attack(10); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(player, move); }
    } SCENE {
        for (u32 hit = 0; hit < 2; hit++) ANIMATION(ANIM_TYPE_MOVE, move, player);
        MESSAGE("The Pokémon was hit 2 times!");
    }
}

SINGLE_BATTLE_TEST("Polished multihit source probability for 3 hits covers every variable attack")
{
    enum Move move = MOVE_DOUBLE_SLAP;
    for (u32 m = 0; m < ARRAY_COUNT(sVariableMoves); m++)
        PARAMETRIZE { move = sVariableMoves[m]; }
    PASSES_RANDOMLY(B_POLISHED_MOVE_BALANCE || move == MOVE_POLISHED_FURY_STRIKES ? 1 : 7, B_POLISHED_MOVE_BALANCE || move == MOVE_POLISHED_FURY_STRIKES ? 3 : 20, RNG_HITS);
    GIVEN {
        WITH_CONFIG(B_MULTI_HIT_CHANCE, GEN_5);
        PLAYER(SPECIES_WOBBUFFET) { Attack(10); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(player, move); }
    } SCENE {
        for (u32 hit = 0; hit < 3; hit++) ANIMATION(ANIM_TYPE_MOVE, move, player);
        MESSAGE("The Pokémon was hit 3 times!");
    }
}

SINGLE_BATTLE_TEST("Polished multihit source probability for 4 hits covers every variable attack")
{
    enum Move move = MOVE_DOUBLE_SLAP;
    for (u32 m = 0; m < ARRAY_COUNT(sVariableMoves); m++)
        PARAMETRIZE { move = sVariableMoves[m]; }
    PASSES_RANDOMLY(B_POLISHED_MOVE_BALANCE || move == MOVE_POLISHED_FURY_STRIKES ? 1 : 3, B_POLISHED_MOVE_BALANCE || move == MOVE_POLISHED_FURY_STRIKES ? 6 : 20, RNG_HITS);
    GIVEN {
        WITH_CONFIG(B_MULTI_HIT_CHANCE, GEN_5);
        PLAYER(SPECIES_WOBBUFFET) { Attack(10); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(player, move); }
    } SCENE {
        for (u32 hit = 0; hit < 4; hit++) ANIMATION(ANIM_TYPE_MOVE, move, player);
        MESSAGE("The Pokémon was hit 4 times!");
    }
}

SINGLE_BATTLE_TEST("Polished multihit source probability for 5 hits covers every variable attack")
{
    enum Move move = MOVE_DOUBLE_SLAP;
    for (u32 m = 0; m < ARRAY_COUNT(sVariableMoves); m++)
        PARAMETRIZE { move = sVariableMoves[m]; }
    PASSES_RANDOMLY(B_POLISHED_MOVE_BALANCE || move == MOVE_POLISHED_FURY_STRIKES ? 1 : 3, B_POLISHED_MOVE_BALANCE || move == MOVE_POLISHED_FURY_STRIKES ? 6 : 20, RNG_HITS);
    GIVEN {
        WITH_CONFIG(B_MULTI_HIT_CHANCE, GEN_5);
        PLAYER(SPECIES_WOBBUFFET) { Attack(10); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(player, move); }
    } SCENE {
        for (u32 hit = 0; hit < 5; hit++) ANIMATION(ANIM_TYPE_MOVE, move, player);
        MESSAGE("The Pokémon was hit 5 times!");
    }
}

DOUBLE_BATTLE_TEST("Polished multihit doubles retain native hit-count selection")
{
    enum Move move = MOVE_DOUBLE_SLAP;
    struct BattlePokemon *user = playerLeft, *target = opponentLeft;
    for (u32 m = 0; m < ARRAY_COUNT(sVariableMoves); m++)
        for (u32 side = 0; side < 2; side++)
            
                PARAMETRIZE { move = sVariableMoves[m]; user = side ? opponentLeft : playerLeft; target = side ? playerLeft : opponentLeft;  }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Attack(10); Ability(ABILITY_NONE); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Attack(10); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(user, move, target: target, WITH_RNG(RNG_HITS, 5)); }
    } SCENE {
        for (u32 hit = 0; hit < 5; hit++) { ANIMATION(ANIM_TYPE_MOVE, move, user); HP_BAR(target); }
        MESSAGE("The Pokémon was hit 5 times!");
    }
}

DOUBLE_BATTLE_TEST("Polished multihit doubles retain native sleep interruption")
{
    enum Move move = MOVE_DOUBLE_SLAP;
    struct BattlePokemon *user = playerLeft, *target = opponentLeft;
    for (u32 m = 0; m < ARRAY_COUNT(sContactMoves); m++)
        for (u32 side = 0; side < 2; side++)
            
                PARAMETRIZE { move = sContactMoves[m]; user = side ? opponentLeft : playerLeft; target = side ? playerLeft : opponentLeft;  }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Attack(10); Ability(user == playerLeft ? ABILITY_SKILL_LINK : ABILITY_EFFECT_SPORE); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Attack(10); Ability(user == opponentLeft ? ABILITY_SKILL_LINK : ABILITY_EFFECT_SPORE); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(user, move, target: target, WITH_RNG(RNG_EFFECT_SPORE, 20)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, user); HP_BAR(target);
        STATUS_ICON(user, sleep: TRUE);
        NOT ANIMATION(ANIM_TYPE_MOVE, move, user);
    }
}

SINGLE_BATTLE_TEST("Polished multihit either active gimmick retain native hit-count selection")
{
    enum Move move = MOVE_DOUBLE_SLAP;
    struct BattlePokemon *user = player, *target = opponent;
    bool32 userTera = FALSE;
    for (u32 m = 0; m < ARRAY_COUNT(sVariableMoves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 t = 0; t < 2; t++)
                PARAMETRIZE { move = sVariableMoves[m]; user = side ? opponent : player; target = side ? player : opponent; userTera = t; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Attack(10); Ability(ABILITY_NONE); TeraType(TYPE_NORMAL); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Attack(10); Ability(ABILITY_NONE); TeraType(TYPE_NORMAL); }
    } WHEN {
        TURN { MOVE(user, MOVE_CELEBRATE, gimmick: userTera ? GIMMICK_TERA : GIMMICK_NONE); MOVE(target, MOVE_CELEBRATE, gimmick: userTera ? GIMMICK_NONE : GIMMICK_TERA); }
        TURN { MOVE(user, move, WITH_RNG(RNG_HITS, 5)); }
    } SCENE {
        for (u32 hit = 0; hit < 5; hit++) { ANIMATION(ANIM_TYPE_MOVE, move, user); HP_BAR(target); }
        MESSAGE("The Pokémon was hit 5 times!");
    }
}

SINGLE_BATTLE_TEST("Polished multihit either active gimmick retain native sleep interruption")
{
    enum Move move = MOVE_DOUBLE_SLAP;
    struct BattlePokemon *user = player, *target = opponent;
    bool32 userTera = FALSE;
    for (u32 m = 0; m < ARRAY_COUNT(sContactMoves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 t = 0; t < 2; t++)
                PARAMETRIZE { move = sContactMoves[m]; user = side ? opponent : player; target = side ? player : opponent; userTera = t; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Attack(10); Ability(user == player ? ABILITY_SKILL_LINK : ABILITY_EFFECT_SPORE); TeraType(TYPE_NORMAL); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Attack(10); Ability(user == opponent ? ABILITY_SKILL_LINK : ABILITY_EFFECT_SPORE); TeraType(TYPE_NORMAL); }
    } WHEN {
        TURN { MOVE(user, MOVE_CELEBRATE, gimmick: userTera ? GIMMICK_TERA : GIMMICK_NONE); MOVE(target, MOVE_CELEBRATE, gimmick: userTera ? GIMMICK_NONE : GIMMICK_TERA); }
        TURN { MOVE(user, move, WITH_RNG(RNG_EFFECT_SPORE, 20)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, user); HP_BAR(target);
        STATUS_ICON(user, sleep: TRUE);
        NOT ANIMATION(ANIM_TYPE_MOVE, move, user);
    }
}

SINGLE_BATTLE_TEST("Polished multihit optional variable attacks retain native selection")
{
    static const enum Move optional[] = {MOVE_BULLET_SEED, MOVE_FURY_SWIPES, MOVE_TAIL_SLAP, MOVE_SCALE_SHOT};
    enum Move move = MOVE_BULLET_SEED;
    struct BattlePokemon *user = player, *target = opponent;
    for (u32 m = 0; m < ARRAY_COUNT(optional); m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = optional[m]; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Attack(10); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Attack(10); Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(user, move, WITH_RNG(RNG_HITS, 5)); }
    } SCENE {
        for (u32 hit = 0; hit < 5; hit++) { ANIMATION(ANIM_TYPE_MOVE, move, user); HP_BAR(target); }
        MESSAGE("The Pokémon was hit 5 times!");
    }
}
