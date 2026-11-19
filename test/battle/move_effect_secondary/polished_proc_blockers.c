#include "global.h"
#include "test/battle.h"

static const enum Move sFlinchMoves[] = {
    MOVE_AIR_SLASH, MOVE_STOMP, MOVE_HEADBUTT, MOVE_BITE, MOVE_WATERFALL,
    MOVE_ICICLE_CRASH, MOVE_IRON_HEAD, MOVE_EXTRASENSORY, MOVE_ROCK_SLIDE,
    MOVE_HYPER_FANG, MOVE_ZEN_HEADBUTT, MOVE_ASTONISH, MOVE_DARK_PULSE,
};

static const enum Move sSecondaryMoves[] = {MOVE_THUNDERBOLT, MOVE_ICE_BEAM, MOVE_FLAMETHROWER};

SINGLE_BATTLE_TEST("Polished proc blockers allows both held items and Stench through Covert Cloak")
{
    struct BattlePokemon *holder = player, *other = opponent;
    u32 mode = 0;
    for (u32 m = 0; m < 3; m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; mode = m; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == player ? ABILITY_STEADFAST : (mode == 2 ? ABILITY_STENCH : ABILITY_NONE));
            Item(holder == player ? ITEM_COVERT_CLOAK : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == player) { MovesWithPP({MOVE_TACKLE, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? ABILITY_STEADFAST : (mode == 2 ? ABILITY_STENCH : ABILITY_NONE));
            Item(holder == opponent ? ITEM_COVERT_CLOAK : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == opponent) { MovesWithPP({MOVE_TACKLE, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, MOVE_SCRATCH, WITH_RNG(mode == 2 ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, TRUE)); MOVE(holder, MOVE_TACKLE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SCRATCH, other);
        if (B_POLISHED_MOVE_BALANCE) {
        if (holder == player) MESSAGE("Wobbuffet flinched and couldn't move!");
        else MESSAGE("The opposing Wobbuffet flinched and couldn't move!");
        } else ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, holder);
    } THEN {
        EXPECT_EQ(holder->pp[0], B_POLISHED_MOVE_BALANCE ? 5 : 4);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + B_POLISHED_MOVE_BALANCE);
    }
}

SINGLE_BATTLE_TEST("Polished proc blockers allows held flinch through Sheer Force while still suppressing intrinsic status")
{
    struct BattlePokemon *holder = player, *other = opponent;
    u32 mode = 0;
    enum Move move = MOVE_THUNDERBOLT;
    bool32 cloak = FALSE;
    for (u32 m = 0; m < 3; m++)
        for (u32 a = 0; a < ARRAY_COUNT(sSecondaryMoves); a++)
            for (u32 c = 0; c < 2; c++)
                for (u32 side = 0; side < 2; side++)
                    PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; mode = m; move = sSecondaryMoves[a]; cloak = c; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == player ? ABILITY_STEADFAST : ABILITY_SHEER_FORCE);
            Item(holder == player ? (cloak ? ITEM_COVERT_CLOAK : ITEM_NONE) : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == player) { MovesWithPP({MOVE_TACKLE, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? ABILITY_STEADFAST : ABILITY_SHEER_FORCE);
            Item(holder == opponent ? (cloak ? ITEM_COVERT_CLOAK : ITEM_NONE) : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == opponent) { MovesWithPP({MOVE_TACKLE, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, move, secondaryEffect: TRUE, WITH_RNG(RNG_HOLD_EFFECT_FLINCH, TRUE)); MOVE(holder, MOVE_TACKLE); }
    } THEN {
        EXPECT_EQ(holder->status1, STATUS1_NONE);
        EXPECT_EQ(holder->pp[0], B_POLISHED_MOVE_BALANCE && mode != 2 ? 5 : 4);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + (B_POLISHED_MOVE_BALANCE && mode != 2));
    }
}

SINGLE_BATTLE_TEST("Polished proc blockers clears recharge through Covert Cloak before the active Inner Focus check")
{
    struct BattlePokemon *holder = player, *other = opponent;
    u32 mode = 0;
    enum Move recharge = MOVE_HYPER_BEAM;
    enum Ability ability = ABILITY_NONE;
    for (u32 a = 0; a < 3; a++)
        for (u32 m = 0; m < 3; m++)
            for (u32 r = 0; r < 2; r++)
                for (u32 side = 0; side < 2; side++)
                    PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; mode = m; recharge = r ? MOVE_GIGA_IMPACT : MOVE_HYPER_BEAM; ability = a == 0 ? ABILITY_NONE : a == 1 ? ABILITY_INNER_FOCUS : ABILITY_STEADFAST; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == player ? ability : (mode == 2 ? ABILITY_STENCH : ABILITY_NONE));
            Item(holder == player ? ITEM_COVERT_CLOAK : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == player) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? ability : (mode == 2 ? ABILITY_STENCH : ABILITY_NONE));
            Item(holder == opponent ? ITEM_COVERT_CLOAK : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == opponent) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, recharge, hit: TRUE); }
        TURN { MOVE(other, MOVE_SCRATCH, WITH_RNG(mode == 2 ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, TRUE)); SKIP_TURN(holder); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, recharge, holder);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SCRATCH, other);
        if (B_POLISHED_MOVE_BALANCE) {
            if (ability == ABILITY_INNER_FOCUS) ANIMATION(ANIM_TYPE_MOVE, recharge, holder);
            else {
        if (holder == player) MESSAGE("Wobbuffet flinched and couldn't move!");
        else MESSAGE("The opposing Wobbuffet flinched and couldn't move!");
            }
        } else {
        if (holder == player) MESSAGE("Wobbuffet must recharge!");
        else MESSAGE("The opposing Wobbuffet must recharge!");
        }
    } THEN {
        EXPECT_EQ(holder->pp[0], B_POLISHED_MOVE_BALANCE && ability == ABILITY_INNER_FOCUS ? 3 : 4);
        EXPECT_EQ((u32)holder->volatiles.rechargeTimer, B_POLISHED_MOVE_BALANCE && ability == ABILITY_INNER_FOCUS ? 1 : 0);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + (B_POLISHED_MOVE_BALANCE && ability == ABILITY_STEADFAST));
    }
}

SINGLE_BATTLE_TEST("Polished proc blockers clears recharge through Covert Cloak after the target acted without stale flinch")
{
    struct BattlePokemon *holder = player, *other = opponent;
    u32 mode = 0;
    enum Move recharge = MOVE_HYPER_BEAM;
    for (u32 m = 0; m < 3; m++)
        for (u32 r = 0; r < 2; r++)
            for (u32 side = 0; side < 2; side++)
                PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; mode = m; recharge = r ? MOVE_GIGA_IMPACT : MOVE_HYPER_BEAM; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 200 : 50);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == player ? ABILITY_STEADFAST : (mode == 2 ? ABILITY_STENCH : ABILITY_NONE));
            Item(holder == player ? ITEM_COVERT_CLOAK : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == player) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 200 : 50);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? ABILITY_STEADFAST : (mode == 2 ? ABILITY_STENCH : ABILITY_NONE));
            Item(holder == opponent ? ITEM_COVERT_CLOAK : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == opponent) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
        }
    } WHEN {
        TURN { MOVE(holder, recharge, hit: TRUE); MOVE(other, MOVE_SCRATCH, WITH_RNG(mode == 2 ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, TRUE)); }
        TURN { MOVE(other, MOVE_CELEBRATE); if (B_POLISHED_MOVE_BALANCE) { MOVE(holder, MOVE_TACKLE); } else { SKIP_TURN(holder); } }
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, MOVE_TACKLE); }
    } THEN {
        EXPECT_EQ(holder->pp[0], 4);
        EXPECT_EQ(holder->pp[1], B_POLISHED_MOVE_BALANCE ? 3 : 4);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE);
        EXPECT_EQ((u32)holder->volatiles.rechargeTimer, 0);
    }
}

SINGLE_BATTLE_TEST("Polished proc blockers clears recharge for Sheer Force item procs through Covert Cloak and Inner Focus")
{
    struct BattlePokemon *holder = player, *other = opponent;
    u32 mode = 0;
    enum Move recharge = MOVE_HYPER_BEAM;
    for (u32 m = 0; m < 2; m++)
        for (u32 r = 0; r < 2; r++)
            for (u32 side = 0; side < 2; side++)
                PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; mode = m; recharge = r ? MOVE_GIGA_IMPACT : MOVE_HYPER_BEAM; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == player ? ABILITY_INNER_FOCUS : ABILITY_SHEER_FORCE);
            Item(holder == player ? ITEM_COVERT_CLOAK : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == player) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? ABILITY_INNER_FOCUS : ABILITY_SHEER_FORCE);
            Item(holder == opponent ? ITEM_COVERT_CLOAK : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == opponent) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, recharge, hit: TRUE); }
        TURN { MOVE(other, MOVE_THUNDERBOLT, secondaryEffect: TRUE, WITH_RNG(RNG_HOLD_EFFECT_FLINCH, TRUE)); SKIP_TURN(holder); }
    } THEN {
        EXPECT_EQ(holder->status1, STATUS1_NONE);
        EXPECT_EQ(holder->pp[0], B_POLISHED_MOVE_BALANCE ? 3 : 4);
        EXPECT_EQ((u32)holder->volatiles.rechargeTimer, B_POLISHED_MOVE_BALANCE ? 1 : 0);
    }
}

SINGLE_BATTLE_TEST("Polished proc blockers preserves Shield Dust with Covert Cloak and either Sheer Force or Stench")
{
    struct BattlePokemon *holder = player, *other = opponent;
    u32 mode = 0;
    for (u32 m = 0; m < 3; m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; mode = m; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == player ? ABILITY_SHIELD_DUST : (mode == 2 ? ABILITY_STENCH : ABILITY_SHEER_FORCE));
            Item(holder == player ? ITEM_COVERT_CLOAK : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == player) { MovesWithPP({MOVE_TACKLE, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? ABILITY_SHIELD_DUST : (mode == 2 ? ABILITY_STENCH : ABILITY_SHEER_FORCE));
            Item(holder == opponent ? ITEM_COVERT_CLOAK : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == opponent) { MovesWithPP({MOVE_TACKLE, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, MOVE_THUNDERBOLT, secondaryEffect: TRUE, WITH_RNG(mode == 2 ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, TRUE)); MOVE(holder, MOVE_TACKLE); }
    } THEN {
        EXPECT_EQ(holder->pp[0], 4);
        EXPECT_EQ(holder->status1, STATUS1_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished proc blockers bypasses Shield Dust with Mold Breaker but rechecks active Inner Focus")
{
    struct BattlePokemon *holder = player, *other = opponent;
    u32 mode = 0;
    enum Ability ability = ABILITY_SHIELD_DUST;
    for (u32 a = 0; a < 2; a++)
        for (u32 m = 0; m < 2; m++)
            for (u32 side = 0; side < 2; side++)
                PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; mode = m; ability = a ? ABILITY_INNER_FOCUS : ABILITY_SHIELD_DUST; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == player ? ability : ABILITY_MOLD_BREAKER);
            Item(holder == player ? ITEM_COVERT_CLOAK : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == player) { MovesWithPP({MOVE_TACKLE, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? ability : ABILITY_MOLD_BREAKER);
            Item(holder == opponent ? ITEM_COVERT_CLOAK : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == opponent) { MovesWithPP({MOVE_TACKLE, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, MOVE_SCRATCH, WITH_RNG(mode == 2 ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, TRUE)); MOVE(holder, MOVE_TACKLE); }
    } THEN {
        EXPECT_EQ(holder->pp[0], B_POLISHED_MOVE_BALANCE && ability == ABILITY_SHIELD_DUST ? 5 : 4);
    }
}

SINGLE_BATTLE_TEST("Polished proc blockers does not flinch through Covert Cloak after misses or failed rolls")
{
    struct BattlePokemon *holder = player, *other = opponent;
    u32 mode = 0;
    bool32 miss = FALSE;
    for (u32 m = 0; m < 3; m++)
        for (u32 f = 0; f < 2; f++)
            for (u32 side = 0; side < 2; side++)
                PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; mode = m; miss = f; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == player ? ABILITY_STEADFAST : (mode == 2 ? ABILITY_STENCH : ABILITY_NONE));
            Item(holder == player ? ITEM_COVERT_CLOAK : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == player) { MovesWithPP({MOVE_TACKLE, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? ABILITY_STEADFAST : (mode == 2 ? ABILITY_STENCH : ABILITY_NONE));
            Item(holder == opponent ? ITEM_COVERT_CLOAK : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == opponent) { MovesWithPP({MOVE_TACKLE, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, MOVE_HYDRO_PUMP, hit: !miss, WITH_RNG(mode == 2 ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, miss)); MOVE(holder, MOVE_TACKLE); }
    } THEN {
        EXPECT_EQ(holder->pp[0], 4);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished proc blockers keeps Covert Cloak protection against all intrinsic source flinch moves")
{
    struct BattlePokemon *holder = player, *other = opponent;
    u32 mode = 0;
    enum Move move = MOVE_BITE;
    for (u32 f = 0; f < ARRAY_COUNT(sFlinchMoves); f++)
        for (u32 m = 0; m < 3; m++)
            for (u32 side = 0; side < 2; side++)
                PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; mode = m; move = sFlinchMoves[f]; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == player ? ABILITY_STEADFAST : (mode == 2 ? ABILITY_STENCH : ABILITY_NONE));
            Item(holder == player ? ITEM_COVERT_CLOAK : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == player) { MovesWithPP({MOVE_TACKLE, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? ABILITY_STEADFAST : (mode == 2 ? ABILITY_STENCH : ABILITY_NONE));
            Item(holder == opponent ? ITEM_COVERT_CLOAK : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == opponent) { MovesWithPP({MOVE_TACKLE, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, move, hit: TRUE, secondaryEffect: TRUE, WITH_RNG(mode == 2 ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, TRUE)); MOVE(holder, MOVE_TACKLE); }
    } THEN {
        EXPECT_EQ(holder->pp[0], 4);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished proc blockers keeps Covert Cloak protection against other intrinsic secondary effects")
{
    struct BattlePokemon *holder = player, *other = opponent;
    u32 mode = 0;
    static const enum Move moves[] = {MOVE_THUNDERBOLT, MOVE_SLUDGE_BOMB, MOVE_ICY_WIND};
    enum Move move = MOVE_THUNDERBOLT;
    for (u32 m = 0; m < ARRAY_COUNT(moves); m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; mode = m; move = moves[m]; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == player ? ABILITY_STEADFAST : ABILITY_NONE);
            Item(holder == player ? ITEM_COVERT_CLOAK : (mode == 0 ? ITEM_NONE : ITEM_NONE));
            if (holder == player) { MovesWithPP({MOVE_TACKLE, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? ABILITY_STEADFAST : ABILITY_NONE);
            Item(holder == opponent ? ITEM_COVERT_CLOAK : (mode == 0 ? ITEM_NONE : ITEM_NONE));
            if (holder == opponent) { MovesWithPP({MOVE_TACKLE, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, move, hit: TRUE, secondaryEffect: TRUE); MOVE(holder, MOVE_TACKLE); }
    } THEN {
        EXPECT_EQ(holder->pp[0], 4);
        EXPECT_EQ(holder->status1, STATUS1_NONE);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished proc blockers retains the inherited intact and broken Substitute proc gates")
{
    struct BattlePokemon *holder = player, *other = opponent;
    u32 mode = 0;
    bool32 broken = FALSE;
    for (u32 b = 0; b < 2; b++)
        for (u32 m = 0; m < 3; m++)
            for (u32 side = 0; side < 2; side++)
                PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; mode = m; broken = b; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(holder == player ? 10 : (broken ? 10000 : 10)); SpAttack(10);
            Ability(holder == player ? ABILITY_STEADFAST : (mode == 2 ? ABILITY_STENCH : ABILITY_NONE));
            Item(holder == player ? ITEM_COVERT_CLOAK : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == player) { MovesWithPP({MOVE_TACKLE, 5}, {MOVE_SUBSTITUTE, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(holder == opponent ? 10 : (broken ? 10000 : 10)); SpAttack(10);
            Ability(holder == opponent ? ABILITY_STEADFAST : (mode == 2 ? ABILITY_STENCH : ABILITY_NONE));
            Item(holder == opponent ? ITEM_COVERT_CLOAK : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == opponent) { MovesWithPP({MOVE_TACKLE, 5}, {MOVE_SUBSTITUTE, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, MOVE_SUBSTITUTE); }
        TURN { MOVE(other, MOVE_SCRATCH, WITH_RNG(mode == 2 ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, TRUE)); MOVE(holder, MOVE_TACKLE); }
    } THEN {
        EXPECT_EQ(holder->pp[0], 4);
        EXPECT_EQ((u32)holder->volatiles.substitute, !broken);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished proc blockers keeps optional Fling blocked by Covert Cloak")
{
    struct BattlePokemon *holder = player, *other = opponent;
    u32 mode = 0;
    for (u32 m = 0; m < 2; m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; mode = m; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == player ? ABILITY_STEADFAST : (mode == 2 ? ABILITY_STENCH : ABILITY_NONE));
            Item(holder == player ? ITEM_COVERT_CLOAK : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == player) { MovesWithPP({MOVE_TACKLE, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? ABILITY_STEADFAST : (mode == 2 ? ABILITY_STENCH : ABILITY_NONE));
            Item(holder == opponent ? ITEM_COVERT_CLOAK : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == opponent) { MovesWithPP({MOVE_TACKLE, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, MOVE_FLING); MOVE(holder, MOVE_TACKLE); }
    } THEN {
        EXPECT_EQ(holder->pp[0], 4);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE);
        EXPECT_EQ(other->item, ITEM_NONE);
    }
}

DOUBLE_BATTLE_TEST("Polished proc blockers retains native blocker behavior in doubles")
{
    struct BattlePokemon *holder = playerLeft, *other = opponentLeft;
    u32 mode = 0;
    for (u32 m = 0; m < 3; m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { holder = side ? opponentLeft : playerLeft; other = side ? playerLeft : opponentLeft; mode = m; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == playerLeft ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == playerLeft ? ABILITY_STEADFAST : (mode == 2 ? ABILITY_STENCH : ABILITY_SHEER_FORCE));
            Item(holder == playerLeft ? ITEM_COVERT_CLOAK : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == playerLeft) { MovesWithPP({MOVE_TACKLE, 5}); }
        }
        PLAYER(SPECIES_WYNAUT) { Speed(75); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponentLeft ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponentLeft ? ABILITY_STEADFAST : (mode == 2 ? ABILITY_STENCH : ABILITY_SHEER_FORCE));
            Item(holder == opponentLeft ? ITEM_COVERT_CLOAK : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == opponentLeft) { MovesWithPP({MOVE_TACKLE, 5}); }
        }
        OPPONENT(SPECIES_WYNAUT) { Speed(90); Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(other, MOVE_THUNDERBOLT, target: holder, secondaryEffect: TRUE, WITH_RNG(mode == 2 ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, TRUE)); MOVE(holder, MOVE_TACKLE, target: other); }
    } THEN {
        EXPECT_EQ(holder->pp[0], 4);
        EXPECT_EQ(holder->status1, STATUS1_NONE);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished proc blockers retains native blocker behavior with either active Terastal")
{
    struct BattlePokemon *holder = player, *other = opponent;
    u32 mode = 0;
    bool32 holderTera = FALSE;
    for (u32 t = 0; t < 2; t++)
        for (u32 m = 0; m < 3; m++)
            for (u32 side = 0; side < 2; side++)
                PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; mode = m; holderTera = t; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == player ? ABILITY_STEADFAST : (mode == 2 ? ABILITY_STENCH : ABILITY_SHEER_FORCE));
            Item(holder == player ? ITEM_COVERT_CLOAK : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == player) { MovesWithPP({MOVE_TACKLE, 5}); }
            TeraType(TYPE_NORMAL);
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? ABILITY_STEADFAST : (mode == 2 ? ABILITY_STENCH : ABILITY_SHEER_FORCE));
            Item(holder == opponent ? ITEM_COVERT_CLOAK : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == opponent) { MovesWithPP({MOVE_TACKLE, 5}); }
            TeraType(TYPE_NORMAL);
        }
    } WHEN {
        TURN { MOVE(other, MOVE_THUNDERBOLT, gimmick: holderTera ? GIMMICK_NONE : GIMMICK_TERA, secondaryEffect: TRUE, WITH_RNG(mode == 2 ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, TRUE)); MOVE(holder, MOVE_TACKLE, gimmick: holderTera ? GIMMICK_TERA : GIMMICK_NONE); }
    } THEN {
        EXPECT_EQ(holder->pp[0], 4);
        EXPECT_EQ(holder->status1, STATUS1_NONE);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE);
    }
}
