#include "global.h"
#include "test/battle.h"

static const enum Move sFlinchMoves[] = {
    MOVE_AIR_SLASH, MOVE_STOMP, MOVE_HEADBUTT, MOVE_BITE, MOVE_WATERFALL,
    MOVE_ICICLE_CRASH, MOVE_IRON_HEAD, MOVE_EXTRASENSORY, MOVE_ROCK_SLIDE,
    MOVE_HYPER_FANG, MOVE_ZEN_HEADBUTT, MOVE_ASTONISH, MOVE_DARK_PULSE,
};

SINGLE_BATTLE_TEST("Polished proc recharge clears pending recharge for both items and Stench on either side")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Move recharge = MOVE_HYPER_BEAM;
    u32 mode = 0;
    for (u32 m = 0; m < 3; m++)
        for (u32 r = 0; r < 2; r++)
            for (u32 side = 0; side < 2; side++)
                PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; mode = m; recharge = r ? MOVE_GIGA_IMPACT : MOVE_HYPER_BEAM; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == player ? ABILITY_NONE : (mode == 2 ? ABILITY_STENCH : ABILITY_NONE));
            Item(holder == player ? ITEM_NONE : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == player) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? ABILITY_NONE : (mode == 2 ? ABILITY_STENCH : ABILITY_NONE));
            Item(holder == opponent ? ITEM_NONE : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == opponent) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, recharge, hit: TRUE); }
        TURN { MOVE(other, MOVE_SCRATCH, WITH_RNG(mode == 2 ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, TRUE)); SKIP_TURN(holder); }
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, MOVE_TACKLE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, recharge, holder);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SCRATCH, other);
        if (B_POLISHED_MOVE_BALANCE) {
        if (holder == player) MESSAGE("Wobbuffet flinched and couldn't move!");
        else MESSAGE("The opposing Wobbuffet flinched and couldn't move!");
        } else {
        if (holder == player) MESSAGE("Wobbuffet must recharge!");
        else MESSAGE("The opposing Wobbuffet must recharge!");
        }
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, holder);
    } THEN {
        EXPECT_EQ(holder->pp[0], 4);
        EXPECT_EQ(holder->pp[1], 4);
        EXPECT_EQ((u32)holder->volatiles.rechargeTimer, 0);
    }
}

SINGLE_BATTLE_TEST("Polished proc recharge defers active Inner Focus and repeats the previously selected move")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Move recharge = MOVE_HYPER_BEAM;
    u32 mode = 0;
    for (u32 m = 0; m < 3; m++)
        for (u32 r = 0; r < 2; r++)
            for (u32 side = 0; side < 2; side++)
                PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; mode = m; recharge = r ? MOVE_GIGA_IMPACT : MOVE_HYPER_BEAM; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == player ? ABILITY_INNER_FOCUS : (mode == 2 ? ABILITY_STENCH : ABILITY_NONE));
            Item(holder == player ? ITEM_NONE : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == player) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}, {MOVE_CELEBRATE, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? ABILITY_INNER_FOCUS : (mode == 2 ? ABILITY_STENCH : ABILITY_NONE));
            Item(holder == opponent ? ITEM_NONE : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == opponent) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}, {MOVE_CELEBRATE, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, recharge, hit: TRUE); }
        TURN { MOVE(other, MOVE_SCRATCH, WITH_RNG(mode == 2 ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, TRUE)); SKIP_TURN(holder); }
        TURN { MOVE(other, MOVE_CELEBRATE); if (B_POLISHED_MOVE_BALANCE) { SKIP_TURN(holder); } else { MOVE(holder, MOVE_CELEBRATE); } }
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, MOVE_TACKLE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, recharge, holder);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SCRATCH, other);
        if (B_POLISHED_MOVE_BALANCE) ANIMATION(ANIM_TYPE_MOVE, recharge, holder);
        if (holder == player) MESSAGE("Wobbuffet must recharge!");
        else MESSAGE("The opposing Wobbuffet must recharge!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, holder);
    } THEN {
        EXPECT_EQ(holder->pp[0], B_POLISHED_MOVE_BALANCE ? 3 : 4);
        EXPECT_EQ(holder->pp[1], 4);
        EXPECT_EQ((u32)holder->volatiles.rechargeTimer, 0);
    }
}

SINGLE_BATTLE_TEST("Polished proc recharge activates Steadfast after removing pending recharge")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Move recharge = MOVE_HYPER_BEAM;
    u32 mode = 0;
    for (u32 m = 0; m < 3; m++)
        for (u32 r = 0; r < 2; r++)
            for (u32 side = 0; side < 2; side++)
                PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; mode = m; recharge = r ? MOVE_GIGA_IMPACT : MOVE_HYPER_BEAM; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == player ? ABILITY_STEADFAST : (mode == 2 ? ABILITY_STENCH : ABILITY_NONE));
            Item(holder == player ? ITEM_NONE : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == player) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? ABILITY_STEADFAST : (mode == 2 ? ABILITY_STENCH : ABILITY_NONE));
            Item(holder == opponent ? ITEM_NONE : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == opponent) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, recharge, hit: TRUE); }
        TURN { MOVE(other, MOVE_SCRATCH, WITH_RNG(mode == 2 ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, TRUE)); SKIP_TURN(holder); }
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, MOVE_TACKLE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, recharge, holder);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SCRATCH, other);
        if (B_POLISHED_MOVE_BALANCE) {
        if (holder == player) MESSAGE("Wobbuffet flinched and couldn't move!");
        else MESSAGE("The opposing Wobbuffet flinched and couldn't move!");
        } else {
        if (holder == player) MESSAGE("Wobbuffet must recharge!");
        else MESSAGE("The opposing Wobbuffet must recharge!");
        }
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, holder);
    } THEN {
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + B_POLISHED_MOVE_BALANCE);
        EXPECT_EQ(holder->pp[0], 4);
        EXPECT_EQ(holder->pp[1], 4);
    }
}

SINGLE_BATTLE_TEST("Polished proc recharge clears newly set recharge after the target acted without leaking flinch")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Move recharge = MOVE_HYPER_BEAM;
    u32 mode = 0;
    enum Ability ability = ABILITY_NONE;
    for (u32 a = 0; a < 3; a++)
        for (u32 m = 0; m < 3; m++)
            for (u32 r = 0; r < 2; r++)
                for (u32 side = 0; side < 2; side++)
                    PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; mode = m; recharge = r ? MOVE_GIGA_IMPACT : MOVE_HYPER_BEAM; ability = a == 0 ? ABILITY_NONE : a == 1 ? ABILITY_INNER_FOCUS : ABILITY_STEADFAST; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 200 : 50);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == player ? ability : (mode == 2 ? ABILITY_STENCH : ABILITY_NONE));
            Item(holder == player ? ITEM_NONE : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == player) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 200 : 50);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? ability : (mode == 2 ? ABILITY_STENCH : ABILITY_NONE));
            Item(holder == opponent ? ITEM_NONE : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == opponent) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
        }
    } WHEN {
        TURN { MOVE(holder, recharge, hit: TRUE); MOVE(other, MOVE_SCRATCH, WITH_RNG(mode == 2 ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, TRUE)); }
        TURN { MOVE(other, MOVE_CELEBRATE); if (B_POLISHED_MOVE_BALANCE) { MOVE(holder, MOVE_TACKLE); } else { SKIP_TURN(holder); } }
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, MOVE_TACKLE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, recharge, holder);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SCRATCH, other);
        if (B_POLISHED_MOVE_BALANCE) ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, holder);
        else {
        if (holder == player) MESSAGE("Wobbuffet must recharge!");
        else MESSAGE("The opposing Wobbuffet must recharge!");
        }
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, holder);
    } THEN {
        EXPECT_EQ(holder->pp[0], 4);
        EXPECT_EQ(holder->pp[1], B_POLISHED_MOVE_BALANCE ? 3 : 4);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE);
        EXPECT_EQ((u32)holder->volatiles.rechargeTimer, 0);
    }
}

SINGLE_BATTLE_TEST("Polished proc recharge preserves all four slots through Mold Breaker and Inner Focus")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Move recharge = MOVE_HYPER_BEAM;
    u32 mode = 0;
    u32 slot = 0;
    for (u32 m = 0; m < 2; m++)
        for (u32 r = 0; r < 2; r++)
            for (u32 s = 0; s < 4; s++)
                for (u32 side = 0; side < 2; side++)
                    PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; mode = m; recharge = r ? MOVE_GIGA_IMPACT : MOVE_HYPER_BEAM; slot = s; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == player ? ABILITY_INNER_FOCUS : ABILITY_MOLD_BREAKER);
            Item(holder == player ? ITEM_NONE : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == player) { MovesWithPP({slot == 0 ? recharge : MOVE_CELEBRATE, 5}, {slot == 1 ? recharge : MOVE_TACKLE, 5}, {slot == 2 ? recharge : MOVE_SPLASH, 5}, {slot == 3 ? recharge : MOVE_GROWTH, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? ABILITY_INNER_FOCUS : ABILITY_MOLD_BREAKER);
            Item(holder == opponent ? ITEM_NONE : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == opponent) { MovesWithPP({slot == 0 ? recharge : MOVE_CELEBRATE, 5}, {slot == 1 ? recharge : MOVE_TACKLE, 5}, {slot == 2 ? recharge : MOVE_SPLASH, 5}, {slot == 3 ? recharge : MOVE_GROWTH, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, recharge, hit: TRUE); }
        TURN { MOVE(other, MOVE_SCRATCH, WITH_RNG(mode == 2 ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, TRUE)); SKIP_TURN(holder); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, recharge, holder);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SCRATCH, other);
        if (B_POLISHED_MOVE_BALANCE) ANIMATION(ANIM_TYPE_MOVE, recharge, holder);
        else {
        if (holder == player) MESSAGE("Wobbuffet must recharge!");
        else MESSAGE("The opposing Wobbuffet must recharge!");
        }
    } THEN {
        EXPECT_EQ(holder->pp[slot], B_POLISHED_MOVE_BALANCE ? 3 : 4);
        for (u32 i = 0; i < MAX_MON_MOVES; i++)
            if (i != slot) EXPECT_EQ(holder->pp[i], 5);
        EXPECT_EQ((u32)holder->volatiles.rechargeTimer, B_POLISHED_MOVE_BALANCE ? 1 : 0);
    }
}

SINGLE_BATTLE_TEST("Polished proc recharge honors Neutralizing Gas suppression of Inner Focus")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Move recharge = MOVE_HYPER_BEAM;
    u32 mode = 0;
    for (u32 m = 0; m < 2; m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; mode = m; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == player ? ABILITY_INNER_FOCUS : ABILITY_NEUTRALIZING_GAS);
            Item(holder == player ? ITEM_NONE : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == player) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? ABILITY_INNER_FOCUS : ABILITY_NEUTRALIZING_GAS);
            Item(holder == opponent ? ITEM_NONE : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == opponent) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, recharge, hit: TRUE); }
        TURN { MOVE(other, MOVE_SCRATCH, WITH_RNG(mode == 2 ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, TRUE)); SKIP_TURN(holder); }
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, MOVE_TACKLE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, recharge, holder);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SCRATCH, other);
        if (B_POLISHED_MOVE_BALANCE) {
        if (holder == player) MESSAGE("Wobbuffet flinched and couldn't move!");
        else MESSAGE("The opposing Wobbuffet flinched and couldn't move!");
        } else {
        if (holder == player) MESSAGE("Wobbuffet must recharge!");
        else MESSAGE("The opposing Wobbuffet must recharge!");
        }
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, holder);
    } THEN {
        EXPECT_EQ(holder->pp[0], 4);
        EXPECT_EQ(holder->pp[1], 4);
    }
}

SINGLE_BATTLE_TEST("Polished proc recharge preserves recharge after misses or failed proc rolls")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Move recharge = MOVE_HYPER_BEAM;
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
            Item(holder == player ? ITEM_NONE : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == player) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? ABILITY_STEADFAST : (mode == 2 ? ABILITY_STENCH : ABILITY_NONE));
            Item(holder == opponent ? ITEM_NONE : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == opponent) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, recharge, hit: TRUE); }
        TURN { MOVE(other, MOVE_HYDRO_PUMP, hit: !miss, WITH_RNG(mode == 2 ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, miss)); SKIP_TURN(holder); }
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, MOVE_TACKLE); }
    } SCENE {
        if (holder == player) MESSAGE("Wobbuffet must recharge!");
        else MESSAGE("The opposing Wobbuffet must recharge!");
    } THEN {
        EXPECT_EQ(holder->pp[0], 4);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished proc recharge clears recharge even when the same hit freezes the target")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Move recharge = MOVE_HYPER_BEAM;
    u32 mode = 0;
    for (u32 m = 0; m < 3; m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; mode = m; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == player ? ABILITY_STEADFAST : (mode == 2 ? ABILITY_STENCH : ABILITY_NONE));
            Item(holder == player ? ITEM_NONE : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == player) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? ABILITY_STEADFAST : (mode == 2 ? ABILITY_STENCH : ABILITY_NONE));
            Item(holder == opponent ? ITEM_NONE : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == opponent) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, recharge, hit: TRUE); }
        TURN { MOVE(other, MOVE_ICE_BEAM, secondaryEffect: TRUE, WITH_RNG(mode == 2 ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, TRUE)); SKIP_TURN(holder); }
    } SCENE {
        STATUS_ICON(holder, status1: STATUS1_FREEZE);
        if (B_POLISHED_MOVE_BALANCE) {
        if (holder == player) MESSAGE("Wobbuffet flinched and couldn't move!");
        else MESSAGE("The opposing Wobbuffet flinched and couldn't move!");
        } else {
        if (holder == player) MESSAGE("Wobbuffet must recharge!");
        else MESSAGE("The opposing Wobbuffet must recharge!");
        }
    } THEN {
        EXPECT_EQ(holder->pp[0], 4);
        EXPECT_EQ(holder->status1, STATUS1_FREEZE);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + B_POLISHED_MOVE_BALANCE);
        EXPECT_EQ((u32)holder->volatiles.rechargeTimer, 0);
    }
}

SINGLE_BATTLE_TEST("Polished proc recharge clears recharge while the target remains asleep")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Move recharge = MOVE_HYPER_BEAM;
    u32 mode = 0;
    for (u32 m = 0; m < 3; m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; mode = m; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == player ? ABILITY_STEADFAST : (mode == 2 ? ABILITY_STENCH : ABILITY_NONE));
            Item(holder == player ? ITEM_NONE : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == player) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}, {MOVE_CELEBRATE, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? ABILITY_STEADFAST : (mode == 2 ? ABILITY_STENCH : ABILITY_NONE));
            Item(holder == opponent ? ITEM_NONE : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == opponent) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}, {MOVE_CELEBRATE, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, MOVE_YAWN); MOVE(holder, MOVE_CELEBRATE); }
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, recharge, hit: TRUE); }
        TURN { MOVE(other, MOVE_SCRATCH, WITH_RNG(mode == 2 ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, TRUE)); SKIP_TURN(holder); }
    } SCENE {
        STATUS_ICON(holder, status1: STATUS1_SLEEP);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SCRATCH, other);
        if (B_POLISHED_MOVE_BALANCE) {
        if (holder == player) MESSAGE("Wobbuffet flinched and couldn't move!");
        else MESSAGE("The opposing Wobbuffet flinched and couldn't move!");
        } else {
        if (holder == player) MESSAGE("Wobbuffet must recharge!");
        else MESSAGE("The opposing Wobbuffet must recharge!");
        }
    } THEN {
        EXPECT_EQ(holder->pp[0], 4);
        EXPECT(holder->status1 & STATUS1_SLEEP);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + B_POLISHED_MOVE_BALANCE);
        EXPECT_EQ((u32)holder->volatiles.rechargeTimer, 0);
    }
}

SINGLE_BATTLE_TEST("Polished proc recharge does not add a proc when a source move already has flinchtarget")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Move recharge = MOVE_HYPER_BEAM;
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
            Item(holder == player ? ITEM_NONE : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == player) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? ABILITY_STEADFAST : (mode == 2 ? ABILITY_STENCH : ABILITY_NONE));
            Item(holder == opponent ? ITEM_NONE : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == opponent) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, recharge, hit: TRUE); }
        TURN { MOVE(other, move, hit: TRUE, secondaryEffect: FALSE, WITH_RNG(mode == 2 ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, TRUE)); SKIP_TURN(holder); }
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, MOVE_TACKLE); }
    } SCENE {
        if (holder == player) MESSAGE("Wobbuffet must recharge!");
        else MESSAGE("The opposing Wobbuffet must recharge!");
    } THEN {
        EXPECT_EQ(holder->pp[0], 4);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished proc recharge retains Shield Dust and integrates source Covert Cloak and Sheer Force rules")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Move recharge = MOVE_HYPER_BEAM;
    u32 mode = 0;
    u32 blocker = 0;
    for (u32 b = 0; b < 3; b++)
        for (u32 m = 0; m < (b == 2 ? 2 : 3); m++)
            for (u32 side = 0; side < 2; side++)
                PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; mode = m; blocker = b; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == player ? (blocker == 0 ? ABILITY_SHIELD_DUST : ABILITY_STEADFAST) : (blocker == 2 ? ABILITY_SHEER_FORCE : mode == 2 ? ABILITY_STENCH : ABILITY_NONE));
            Item(holder == player ? (blocker == 1 ? ITEM_COVERT_CLOAK : ITEM_NONE) : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == player) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? (blocker == 0 ? ABILITY_SHIELD_DUST : ABILITY_STEADFAST) : (blocker == 2 ? ABILITY_SHEER_FORCE : mode == 2 ? ABILITY_STENCH : ABILITY_NONE));
            Item(holder == opponent ? (blocker == 1 ? ITEM_COVERT_CLOAK : ITEM_NONE) : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == opponent) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, recharge, hit: TRUE); }
        TURN { MOVE(other, blocker == 2 ? MOVE_THUNDERBOLT : MOVE_SCRATCH, secondaryEffect: FALSE, WITH_RNG(mode == 2 ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, TRUE)); SKIP_TURN(holder); }
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, MOVE_TACKLE); }
    } SCENE {
        if (B_POLISHED_MOVE_BALANCE && blocker != 0) {
            if (holder == player) MESSAGE("Wobbuffet flinched and couldn't move!");
            else MESSAGE("The opposing Wobbuffet flinched and couldn't move!");
        } else {
            if (holder == player) MESSAGE("Wobbuffet must recharge!");
            else MESSAGE("The opposing Wobbuffet must recharge!");
        }
    } THEN {
        EXPECT_EQ(holder->pp[0], 4);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + (B_POLISHED_MOVE_BALANCE && blocker != 0));
    }
}

SINGLE_BATTLE_TEST("Polished proc recharge retains inherited intact and broken Substitute gates")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Move recharge = MOVE_HYPER_BEAM;
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
            Item(holder == player ? ITEM_NONE : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == player) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}, {MOVE_SUBSTITUTE, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(holder == opponent ? 10 : (broken ? 10000 : 10)); SpAttack(10);
            Ability(holder == opponent ? ABILITY_STEADFAST : (mode == 2 ? ABILITY_STENCH : ABILITY_NONE));
            Item(holder == opponent ? ITEM_NONE : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == opponent) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}, {MOVE_SUBSTITUTE, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, MOVE_SUBSTITUTE); }
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, recharge, hit: TRUE); }
        TURN { MOVE(other, MOVE_SCRATCH, WITH_RNG(mode == 2 ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, TRUE)); SKIP_TURN(holder); }
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, MOVE_TACKLE); }
    } SCENE {
        if (holder == player) MESSAGE("Wobbuffet must recharge!");
        else MESSAGE("The opposing Wobbuffet must recharge!");
    } THEN {
        EXPECT_EQ(holder->pp[0], 4);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE);
        EXPECT_EQ((u32)holder->volatiles.substitute, !broken);
    }
}

SINGLE_BATTLE_TEST("Polished proc recharge does not treat optional Fling as a held-item proc")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Move recharge = MOVE_HYPER_BEAM;
    u32 mode = 0;
    for (u32 m = 0; m < 2; m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; mode = m; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == player ? ABILITY_STEADFAST : (mode == 2 ? ABILITY_STENCH : ABILITY_NONE));
            Item(holder == player ? ITEM_NONE : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == player) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? ABILITY_STEADFAST : (mode == 2 ? ABILITY_STENCH : ABILITY_NONE));
            Item(holder == opponent ? ITEM_NONE : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == opponent) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, recharge, hit: TRUE); }
        TURN { MOVE(other, MOVE_FLING); SKIP_TURN(holder); }
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, MOVE_TACKLE); }
    } SCENE {
        if (holder == player) MESSAGE("Wobbuffet must recharge!");
        else MESSAGE("The opposing Wobbuffet must recharge!");
    } THEN {
        EXPECT_EQ(holder->pp[0], 4);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE);
        EXPECT_EQ(other->item, ITEM_NONE);
    }
}

DOUBLE_BATTLE_TEST("Polished proc recharge retains native recharge in doubles")
{
    struct BattlePokemon *holder = playerLeft, *other = opponentLeft;
    enum Move recharge = MOVE_HYPER_BEAM;
    u32 mode = 0;
    for (u32 m = 0; m < 3; m++)
        for (u32 r = 0; r < 2; r++)
            for (u32 side = 0; side < 2; side++)
                PARAMETRIZE { holder = side ? opponentLeft : playerLeft; other = side ? playerLeft : opponentLeft; mode = m; recharge = r ? MOVE_GIGA_IMPACT : MOVE_HYPER_BEAM; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == playerLeft ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == playerLeft ? ABILITY_INNER_FOCUS : (mode == 2 ? ABILITY_STENCH : ABILITY_NONE));
            Item(holder == playerLeft ? ITEM_NONE : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == playerLeft) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
        }
        PLAYER(SPECIES_WYNAUT) { Speed(75); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponentLeft ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponentLeft ? ABILITY_INNER_FOCUS : (mode == 2 ? ABILITY_STENCH : ABILITY_NONE));
            Item(holder == opponentLeft ? ITEM_NONE : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == opponentLeft) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
        }
        OPPONENT(SPECIES_WYNAUT) { Speed(90); Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, recharge, target: other, hit: TRUE); }
        TURN { MOVE(other, MOVE_SCRATCH, target: holder, WITH_RNG(mode == 2 ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, TRUE)); SKIP_TURN(holder); }
    } SCENE {
        if (holder == playerLeft) MESSAGE("Wobbuffet must recharge!");
        else MESSAGE("The opposing Wobbuffet must recharge!");
    } THEN {
        EXPECT_EQ(holder->pp[0], 4);
    }
}

SINGLE_BATTLE_TEST("Polished proc recharge retains native recharge with either active Terastal")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Move recharge = MOVE_HYPER_BEAM;
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
            Ability(holder == player ? ABILITY_INNER_FOCUS : (mode == 2 ? ABILITY_STENCH : ABILITY_NONE));
            Item(holder == player ? ITEM_NONE : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == player) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
            TeraType(TYPE_NORMAL);
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? ABILITY_INNER_FOCUS : (mode == 2 ? ABILITY_STENCH : ABILITY_NONE));
            Item(holder == opponent ? ITEM_NONE : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == opponent) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
            TeraType(TYPE_NORMAL);
        }
    } WHEN {
        TURN { MOVE(other, MOVE_CELEBRATE, gimmick: holderTera ? GIMMICK_NONE : GIMMICK_TERA); MOVE(holder, recharge, hit: TRUE, gimmick: holderTera ? GIMMICK_TERA : GIMMICK_NONE); }
        TURN { MOVE(other, MOVE_SCRATCH, WITH_RNG(mode == 2 ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, TRUE)); SKIP_TURN(holder); }
    } SCENE {
        if (holder == player) MESSAGE("Wobbuffet must recharge!");
        else MESSAGE("The opposing Wobbuffet must recharge!");
    } THEN {
        EXPECT_EQ(holder->pp[0], 4);
    }
}

SINGLE_BATTLE_TEST("Polished proc recharge requires a damaging hit before attempting a proc")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Move recharge = MOVE_HYPER_BEAM;
    u32 mode = 0;
    for (u32 m = 0; m < 3; m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; mode = m; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == player ? ABILITY_INNER_FOCUS : (mode == 2 ? ABILITY_STENCH : ABILITY_NONE));
            Item(holder == player ? ITEM_NONE : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == player) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? ABILITY_INNER_FOCUS : (mode == 2 ? ABILITY_STENCH : ABILITY_NONE));
            Item(holder == opponent ? ITEM_NONE : (mode == 0 ? ITEM_KINGS_ROCK : mode == 1 ? ITEM_RAZOR_FANG : ITEM_NONE));
            if (holder == opponent) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, recharge, hit: TRUE); }
        TURN { MOVE(other, MOVE_SCARY_FACE, hit: TRUE, WITH_RNG(mode == 2 ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, TRUE)); SKIP_TURN(holder); }
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, MOVE_TACKLE); }
    } SCENE {
        if (holder == player) MESSAGE("Wobbuffet must recharge!");
        else MESSAGE("The opposing Wobbuffet must recharge!");
    } THEN {
        EXPECT_EQ(holder->pp[0], 4);
        EXPECT_EQ(holder->pp[1], 4);
    }
}
