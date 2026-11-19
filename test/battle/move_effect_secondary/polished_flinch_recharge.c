#include "global.h"
#include "test/battle.h"

static const enum Move sFlinchMoves[] = {
    MOVE_AIR_SLASH, MOVE_STOMP, MOVE_HEADBUTT, MOVE_BITE, MOVE_WATERFALL,
    MOVE_ICICLE_CRASH, MOVE_IRON_HEAD, MOVE_EXTRASENSORY, MOVE_ROCK_SLIDE,
    MOVE_HYPER_FANG, MOVE_ZEN_HEADBUTT, MOVE_ASTONISH, MOVE_DARK_PULSE,
};

SINGLE_BATTLE_TEST("Polished flinch recharge clears both source recharge moves after every source flinch move")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Move recharge = MOVE_HYPER_BEAM;
    enum Move flinch = MOVE_BITE;
    for (u32 m = 0; m < ARRAY_COUNT(sFlinchMoves); m++)
        for (u32 r = 0; r < 2; r++)
            for (u32 side = 0; side < 2; side++)
                PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; recharge = r ? MOVE_GIGA_IMPACT : MOVE_HYPER_BEAM; flinch = sFlinchMoves[m]; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(ABILITY_NONE);
            Item(ITEM_NONE);
            if (holder == player) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(ABILITY_NONE);
            Item(ITEM_NONE);
            if (holder == opponent) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, recharge, hit: TRUE); }
        TURN { MOVE(other, flinch, hit: TRUE, secondaryEffect: TRUE); SKIP_TURN(holder); }
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, MOVE_TACKLE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, recharge, holder);
        ANIMATION(ANIM_TYPE_MOVE, flinch, other);
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

SINGLE_BATTLE_TEST("Polished flinch recharge lets active Inner Focus repeat the locked move and recharge anew")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Move recharge = MOVE_HYPER_BEAM;
    enum Move flinch = MOVE_BITE;
    for (u32 m = 0; m < ARRAY_COUNT(sFlinchMoves); m++)
        for (u32 r = 0; r < 2; r++)
            for (u32 side = 0; side < 2; side++)
                PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; recharge = r ? MOVE_GIGA_IMPACT : MOVE_HYPER_BEAM; flinch = sFlinchMoves[m]; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == player ? ABILITY_INNER_FOCUS : ABILITY_NONE);
            Item(ITEM_NONE);
            if (holder == player) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}, {MOVE_CELEBRATE, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? ABILITY_INNER_FOCUS : ABILITY_NONE);
            Item(ITEM_NONE);
            if (holder == opponent) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}, {MOVE_CELEBRATE, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, recharge, hit: TRUE); }
        TURN { MOVE(other, flinch, hit: TRUE, secondaryEffect: TRUE); SKIP_TURN(holder); }
        TURN { MOVE(other, MOVE_CELEBRATE); if (B_POLISHED_MOVE_BALANCE) { SKIP_TURN(holder); } else { MOVE(holder, MOVE_CELEBRATE); } }
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, MOVE_TACKLE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, recharge, holder);
        ANIMATION(ANIM_TYPE_MOVE, flinch, other);
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

SINGLE_BATTLE_TEST("Polished flinch recharge activates Steadfast instead of consuming the recharge turn")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Move recharge = MOVE_HYPER_BEAM;
    for (u32 r = 0; r < 2; r++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; recharge = r ? MOVE_GIGA_IMPACT : MOVE_HYPER_BEAM; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == player ? ABILITY_STEADFAST : ABILITY_NONE);
            Item(ITEM_NONE);
            if (holder == player) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? ABILITY_STEADFAST : ABILITY_NONE);
            Item(ITEM_NONE);
            if (holder == opponent) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, recharge, hit: TRUE); }
        TURN { MOVE(other, MOVE_BITE, hit: TRUE, secondaryEffect: TRUE); SKIP_TURN(holder); }
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, MOVE_TACKLE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, recharge, holder);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_BITE, other);
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

SINGLE_BATTLE_TEST("Polished flinch recharge reuses the selected move slot through Mold Breaker and active Inner Focus")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Move recharge = MOVE_HYPER_BEAM;
    u32 slot = 0;
    for (u32 r = 0; r < 2; r++)
        for (u32 s = 0; s < 4; s++)
            for (u32 side = 0; side < 2; side++)
                PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; recharge = r ? MOVE_GIGA_IMPACT : MOVE_HYPER_BEAM; slot = s; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == player ? ABILITY_INNER_FOCUS : ABILITY_MOLD_BREAKER);
            Item(ITEM_NONE);
            if (holder == player) { MovesWithPP({slot == 0 ? recharge : MOVE_CELEBRATE, 5}, {slot == 1 ? recharge : MOVE_TACKLE, 5}, {slot == 2 ? recharge : MOVE_SPLASH, 5}, {slot == 3 ? recharge : MOVE_GROWTH, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? ABILITY_INNER_FOCUS : ABILITY_MOLD_BREAKER);
            Item(ITEM_NONE);
            if (holder == opponent) { MovesWithPP({slot == 0 ? recharge : MOVE_CELEBRATE, 5}, {slot == 1 ? recharge : MOVE_TACKLE, 5}, {slot == 2 ? recharge : MOVE_SPLASH, 5}, {slot == 3 ? recharge : MOVE_GROWTH, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, recharge, hit: TRUE); }
        TURN { MOVE(other, MOVE_BITE, hit: TRUE, secondaryEffect: TRUE); SKIP_TURN(holder); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, recharge, holder);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_BITE, other);
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

SINGLE_BATTLE_TEST("Polished flinch recharge flinches through Inner Focus suppressed by Neutralizing Gas")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Move recharge = MOVE_HYPER_BEAM;
    for (u32 r = 0; r < 2; r++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; recharge = r ? MOVE_GIGA_IMPACT : MOVE_HYPER_BEAM; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == player ? ABILITY_INNER_FOCUS : ABILITY_NEUTRALIZING_GAS);
            Item(ITEM_NONE);
            if (holder == player) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? ABILITY_INNER_FOCUS : ABILITY_NEUTRALIZING_GAS);
            Item(ITEM_NONE);
            if (holder == opponent) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, recharge, hit: TRUE); }
        TURN { MOVE(other, MOVE_BITE, hit: TRUE, secondaryEffect: TRUE); SKIP_TURN(holder); }
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, MOVE_TACKLE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, recharge, holder);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_BITE, other);
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

SINGLE_BATTLE_TEST("Polished flinch recharge preserves recharge after misses or failed secondary rolls")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Move recharge = MOVE_HYPER_BEAM;
    bool32 miss = FALSE;
    for (u32 r = 0; r < 2; r++)
        for (u32 m = 0; m < 2; m++)
            for (u32 side = 0; side < 2; side++)
                PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; recharge = r ? MOVE_GIGA_IMPACT : MOVE_HYPER_BEAM; miss = m; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == player ? ABILITY_STEADFAST : ABILITY_NONE);
            Item(ITEM_NONE);
            if (holder == player) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? ABILITY_STEADFAST : ABILITY_NONE);
            Item(ITEM_NONE);
            if (holder == opponent) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, recharge, hit: TRUE); }
        TURN { MOVE(other, MOVE_ROCK_SLIDE, hit: !miss, secondaryEffect: miss); SKIP_TURN(holder); }
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, MOVE_TACKLE); }
    } SCENE {
        if (holder == player) MESSAGE("Wobbuffet must recharge!");
        else MESSAGE("The opposing Wobbuffet must recharge!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, holder);
    } THEN {
        EXPECT_EQ(holder->pp[0], 4);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished flinch recharge honors Shield Dust Covert Cloak and Sheer Force with a Mold Breaker control")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Move recharge = MOVE_HYPER_BEAM;
    u32 mode = 0;
    for (u32 m = 0; m < 4; m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; mode = m; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == player ? (mode == 0 || mode == 3 ? ABILITY_SHIELD_DUST : ABILITY_NONE) : (mode == 2 ? ABILITY_SHEER_FORCE : mode == 3 ? ABILITY_MOLD_BREAKER : ABILITY_NONE));
            Item(holder == player ? (mode == 1 ? ITEM_COVERT_CLOAK : ITEM_NONE) : ITEM_NONE);
            if (holder == player) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? (mode == 0 || mode == 3 ? ABILITY_SHIELD_DUST : ABILITY_NONE) : (mode == 2 ? ABILITY_SHEER_FORCE : mode == 3 ? ABILITY_MOLD_BREAKER : ABILITY_NONE));
            Item(holder == opponent ? (mode == 1 ? ITEM_COVERT_CLOAK : ITEM_NONE) : ITEM_NONE);
            if (holder == opponent) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, recharge, hit: TRUE); }
        TURN { MOVE(other, MOVE_BITE, hit: TRUE, secondaryEffect: TRUE); SKIP_TURN(holder); }
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, MOVE_TACKLE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_BITE, other);
        if (B_POLISHED_MOVE_BALANCE && mode == 3) {
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

SINGLE_BATTLE_TEST("Polished flinch recharge preserves recharge behind intact and broken Substitutes")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Move recharge = MOVE_HYPER_BEAM;
    bool32 broken = FALSE;
    for (u32 b = 0; b < 2; b++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; broken = b; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(holder == player ? 10 : (broken ? 5000 : 10)); SpAttack(10);
            Ability(holder == player ? ABILITY_STEADFAST : ABILITY_NONE);
            Item(ITEM_NONE);
            if (holder == player) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}, {MOVE_SUBSTITUTE, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(holder == opponent ? 10 : (broken ? 5000 : 10)); SpAttack(10);
            Ability(holder == opponent ? ABILITY_STEADFAST : ABILITY_NONE);
            Item(ITEM_NONE);
            if (holder == opponent) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}, {MOVE_SUBSTITUTE, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, MOVE_SUBSTITUTE); }
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, recharge, hit: TRUE); }
        TURN { MOVE(other, MOVE_BITE, hit: TRUE, secondaryEffect: TRUE); SKIP_TURN(holder); }
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, MOVE_TACKLE); }
    } SCENE {
        if (holder == player) MESSAGE("Wobbuffet must recharge!");
        else MESSAGE("The opposing Wobbuffet must recharge!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, holder);
    } THEN {
        EXPECT_EQ(holder->pp[0], 4);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE);
        EXPECT_EQ((u32)holder->volatiles.substitute, !broken);
    }
}

SINGLE_BATTLE_TEST("Polished flinch recharge rejects move-effect flinch on sleeping and frozen recharge targets")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Move recharge = MOVE_HYPER_BEAM;
    bool32 frozen = FALSE;
    for (u32 f = 0; f < 2; f++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; frozen = f; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 200 : 150);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == player ? ABILITY_STEADFAST : ABILITY_SPEED_BOOST);
            Item(ITEM_NONE);
            if (holder == player) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 200 : 150);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? ABILITY_STEADFAST : ABILITY_SPEED_BOOST);
            Item(ITEM_NONE);
            if (holder == opponent) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
        }
    } WHEN {
        TURN { MOVE(holder, recharge, hit: TRUE); MOVE(other, frozen ? MOVE_ICE_BEAM : MOVE_SPORE, secondaryEffect: TRUE); }
        TURN { MOVE(other, MOVE_BITE, secondaryEffect: TRUE); SKIP_TURN(holder); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, recharge, holder);
        STATUS_ICON(holder, status1: frozen ? STATUS1_FREEZE : STATUS1_SLEEP);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_BITE, other);
        if (holder == player) MESSAGE("Wobbuffet must recharge!");
        else MESSAGE("The opposing Wobbuffet must recharge!");
    } THEN {
        EXPECT_EQ(holder->pp[0], 4);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE);
        EXPECT_EQ((u32)holder->volatiles.rechargeTimer, 0);
        EXPECT(holder->status1 & (frozen ? STATUS1_FREEZE : STATUS1_SLEEP));
    }
}

SINGLE_BATTLE_TEST("Polished flinch recharge does not clear newly set recharge when the target already acted")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Move recharge = MOVE_HYPER_BEAM;
    for (u32 r = 0; r < 2; r++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; recharge = r ? MOVE_GIGA_IMPACT : MOVE_HYPER_BEAM; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 200 : 50);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == player ? ABILITY_STEADFAST : ABILITY_NONE);
            Item(ITEM_NONE);
            if (holder == player) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 200 : 50);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? ABILITY_STEADFAST : ABILITY_NONE);
            Item(ITEM_NONE);
            if (holder == opponent) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
        }
    } WHEN {
        TURN { MOVE(holder, recharge, hit: TRUE); MOVE(other, MOVE_BITE, secondaryEffect: TRUE); }
        TURN { SKIP_TURN(holder); MOVE(other, MOVE_CELEBRATE); }
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, MOVE_TACKLE); }
    } SCENE {
        if (holder == player) MESSAGE("Wobbuffet must recharge!");
        else MESSAGE("The opposing Wobbuffet must recharge!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, holder);
    } THEN {
        EXPECT_EQ(holder->pp[0], 4);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished flinch recharge integrates the source item and Stench pending-recharge adapter")
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
        TURN { MOVE(other, MOVE_TACKLE, WITH_RNG(mode == 2 ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, TRUE)); SKIP_TURN(holder); }
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, MOVE_TACKLE); }
    } SCENE {
        if (B_POLISHED_MOVE_BALANCE) {
            if (holder == player) MESSAGE("Wobbuffet flinched and couldn't move!");
            else MESSAGE("The opposing Wobbuffet flinched and couldn't move!");
        } else {
            if (holder == player) MESSAGE("Wobbuffet must recharge!");
            else MESSAGE("The opposing Wobbuffet must recharge!");
        }
    } THEN {
        EXPECT_EQ(holder->pp[0], 4);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + B_POLISHED_MOVE_BALANCE);
    }
}

SINGLE_BATTLE_TEST("Polished flinch recharge retains optional native flinch move behavior")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Move recharge = MOVE_HYPER_BEAM;
    for (u32 side = 0; side < 2; side++)
        PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == player ? ABILITY_STEADFAST : ABILITY_NONE);
            Item(ITEM_NONE);
            if (holder == player) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? ABILITY_STEADFAST : ABILITY_NONE);
            Item(ITEM_NONE);
            if (holder == opponent) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, recharge, hit: TRUE); }
        TURN { MOVE(other, MOVE_DRAGON_RUSH, hit: TRUE, secondaryEffect: TRUE); SKIP_TURN(holder); }
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, MOVE_TACKLE); }
    } SCENE {
        if (holder == player) MESSAGE("Wobbuffet must recharge!");
        else MESSAGE("The opposing Wobbuffet must recharge!");
    } THEN {
        EXPECT_EQ(holder->pp[0], 4);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE);
    }
}

DOUBLE_BATTLE_TEST("Polished flinch recharge retains native recharge in doubles")
{
    struct BattlePokemon *holder = playerLeft, *other = opponentLeft;
    enum Move recharge = MOVE_HYPER_BEAM;
    for (u32 r = 0; r < 2; r++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { holder = side ? opponentLeft : playerLeft; other = side ? playerLeft : opponentLeft; recharge = r ? MOVE_GIGA_IMPACT : MOVE_HYPER_BEAM; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == playerLeft ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == playerLeft ? ABILITY_INNER_FOCUS : ABILITY_NONE);
            Item(ITEM_NONE);
            if (holder == playerLeft) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
        }
        PLAYER(SPECIES_WYNAUT) { Speed(75); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponentLeft ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponentLeft ? ABILITY_INNER_FOCUS : ABILITY_NONE);
            Item(ITEM_NONE);
            if (holder == opponentLeft) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
        }
        OPPONENT(SPECIES_WYNAUT) { Speed(90); Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, recharge, target: other, hit: TRUE); }
        TURN { MOVE(other, MOVE_BITE, target: holder, secondaryEffect: TRUE); SKIP_TURN(holder); }
    } SCENE {
        if (holder == playerLeft) MESSAGE("Wobbuffet must recharge!");
        else MESSAGE("The opposing Wobbuffet must recharge!");
    } THEN {
        EXPECT_EQ(holder->pp[0], 4);
        EXPECT_EQ((u32)holder->volatiles.rechargeTimer, 0);
    }
}

SINGLE_BATTLE_TEST("Polished flinch recharge retains native recharge with either active Terastal")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Move recharge = MOVE_HYPER_BEAM;
    bool32 holderTera = FALSE;
    for (u32 r = 0; r < 2; r++)
        for (u32 t = 0; t < 2; t++)
            for (u32 side = 0; side < 2; side++)
                PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; recharge = r ? MOVE_GIGA_IMPACT : MOVE_HYPER_BEAM; holderTera = t; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == player ? ABILITY_INNER_FOCUS : ABILITY_NONE);
            Item(ITEM_NONE);
            if (holder == player) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
            TeraType(TYPE_NORMAL);
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? ABILITY_INNER_FOCUS : ABILITY_NONE);
            Item(ITEM_NONE);
            if (holder == opponent) { MovesWithPP({recharge, 5}, {MOVE_TACKLE, 5}); }
            TeraType(TYPE_NORMAL);
        }
    } WHEN {
        TURN { MOVE(other, MOVE_CELEBRATE, gimmick: holderTera ? GIMMICK_NONE : GIMMICK_TERA); MOVE(holder, recharge, hit: TRUE, gimmick: holderTera ? GIMMICK_TERA : GIMMICK_NONE); }
        TURN { MOVE(other, MOVE_BITE, hit: TRUE, secondaryEffect: TRUE); SKIP_TURN(holder); }
    } SCENE {
        if (holder == player) MESSAGE("Wobbuffet must recharge!");
        else MESSAGE("The opposing Wobbuffet must recharge!");
    } THEN {
        EXPECT_EQ(holder->pp[0], 4);
        EXPECT_EQ((u32)holder->volatiles.rechargeTimer, 0);
    }
}
