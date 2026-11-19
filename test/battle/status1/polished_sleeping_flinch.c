#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished sleeping flinch preserves the sleep timer and resolves Steadfast before waking")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Item item = ITEM_KINGS_ROCK;
    enum Ability ability = ABILITY_STEADFAST, attackerAbility = ABILITY_NONE;
    enum Move chosen = MOVE_TACKLE;
    u32 sleep = 1;
    for (u32 mode = 0; mode < 3; mode++)
        for (u32 turns = 1; turns <= 3; turns++)
            for (u32 a = 0; a < 2; a++)
                for (u32 side = 0; side < 2; side++)
                    PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; item = mode == 1 ? ITEM_RAZOR_FANG : mode == 2 ? ITEM_NONE : ITEM_KINGS_ROCK; attackerAbility = mode == 2 ? ABILITY_STENCH : ABILITY_NONE; sleep = turns; ability = a ? ABILITY_STEADFAST : ABILITY_NONE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 20 : 200); MaxHP(1000); HP(1000); Attack(10);
            Ability(holder == player ? ability : attackerAbility);
            Item(holder == player ? ITEM_NONE : item);
            if (holder == player) { Status1(STATUS1_SLEEP_TURN(sleep)); MovesWithPP({chosen, 5}, {MOVE_POUND, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 20 : 200); MaxHP(1000); HP(1000); Attack(10);
            Ability(holder == opponent ? ability : attackerAbility);
            Item(holder == opponent ? ITEM_NONE : item);
            if (holder == opponent) { Status1(STATUS1_SLEEP_TURN(sleep)); MovesWithPP({chosen, 5}, {MOVE_POUND, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, MOVE_TACKLE, WITH_RNG(attackerAbility == ABILITY_STENCH ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, TRUE)); MOVE(holder, chosen); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, other);
        if (B_POLISHED_MOVE_BALANCE || sleep == 1 || chosen == MOVE_SLEEP_TALK) {
            if (holder == player) MESSAGE("Wobbuffet flinched and couldn't move!");
            else MESSAGE("The opposing Wobbuffet flinched and couldn't move!");
        } else {
            if (holder == player) MESSAGE("Wobbuffet is fast asleep.");
            else MESSAGE("The opposing Wobbuffet is fast asleep.");
        }
        NONE_OF { ANIMATION(ANIM_TYPE_MOVE, chosen, holder); ANIMATION(ANIM_TYPE_MOVE, MOVE_POUND, holder); }
    } THEN {
        EXPECT_EQ(holder->status1, STATUS1_SLEEP_TURN(B_POLISHED_MOVE_BALANCE ? sleep : sleep - 1));
        EXPECT_EQ(holder->pp[0], 5);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + (ability == ABILITY_STEADFAST && (B_POLISHED_MOVE_BALANCE || sleep == 1)));
    }
}

SINGLE_BATTLE_TEST("Polished sleeping flinch blocks Sleep Talk without advancing sleep or consuming either move PP")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Item item = ITEM_KINGS_ROCK;
    enum Ability ability = ABILITY_STEADFAST, attackerAbility = ABILITY_NONE;
    enum Move chosen = MOVE_TACKLE;
    u32 sleep = 1;
    for (u32 mode = 0; mode < 3; mode++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; item = mode == 1 ? ITEM_RAZOR_FANG : mode == 2 ? ITEM_NONE : ITEM_KINGS_ROCK; attackerAbility = mode == 2 ? ABILITY_STENCH : ABILITY_NONE; sleep = 3; chosen = MOVE_SLEEP_TALK; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 20 : 200); MaxHP(1000); HP(1000); Attack(10);
            Ability(holder == player ? ability : attackerAbility);
            Item(holder == player ? ITEM_NONE : item);
            if (holder == player) { Status1(STATUS1_SLEEP_TURN(sleep)); MovesWithPP({chosen, 5}, {MOVE_POUND, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 20 : 200); MaxHP(1000); HP(1000); Attack(10);
            Ability(holder == opponent ? ability : attackerAbility);
            Item(holder == opponent ? ITEM_NONE : item);
            if (holder == opponent) { Status1(STATUS1_SLEEP_TURN(sleep)); MovesWithPP({chosen, 5}, {MOVE_POUND, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, MOVE_TACKLE, WITH_RNG(attackerAbility == ABILITY_STENCH ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, TRUE)); MOVE(holder, chosen); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, other);
        if (B_POLISHED_MOVE_BALANCE || sleep == 1 || chosen == MOVE_SLEEP_TALK) {
            if (holder == player) MESSAGE("Wobbuffet flinched and couldn't move!");
            else MESSAGE("The opposing Wobbuffet flinched and couldn't move!");
        } else {
            if (holder == player) MESSAGE("Wobbuffet is fast asleep.");
            else MESSAGE("The opposing Wobbuffet is fast asleep.");
        }
        NONE_OF { ANIMATION(ANIM_TYPE_MOVE, chosen, holder); ANIMATION(ANIM_TYPE_MOVE, MOVE_POUND, holder); }
    } THEN {
        EXPECT_EQ(holder->status1, STATUS1_SLEEP_TURN(B_POLISHED_MOVE_BALANCE ? 3 : 2));
        EXPECT_EQ(holder->pp[0], 5);
        EXPECT_EQ(holder->pp[1], 5);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + 1);
    }
}

SINGLE_BATTLE_TEST("Polished sleeping flinch prevents Early Bird decrement on both waking and non-waking attempts")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Item item = ITEM_KINGS_ROCK;
    enum Ability ability = ABILITY_STEADFAST, attackerAbility = ABILITY_NONE;
    enum Move chosen = MOVE_TACKLE;
    u32 sleep = 1;
    for (u32 turns = 1; turns <= 3; turns++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; sleep = turns; ability = ABILITY_EARLY_BIRD; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 20 : 200); MaxHP(1000); HP(1000); Attack(10);
            Ability(holder == player ? ability : attackerAbility);
            Item(holder == player ? ITEM_NONE : item);
            if (holder == player) { Status1(STATUS1_SLEEP_TURN(sleep)); MovesWithPP({chosen, 5}, {MOVE_POUND, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 20 : 200); MaxHP(1000); HP(1000); Attack(10);
            Ability(holder == opponent ? ability : attackerAbility);
            Item(holder == opponent ? ITEM_NONE : item);
            if (holder == opponent) { Status1(STATUS1_SLEEP_TURN(sleep)); MovesWithPP({chosen, 5}, {MOVE_POUND, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, MOVE_TACKLE, WITH_RNG(attackerAbility == ABILITY_STENCH ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, TRUE)); MOVE(holder, chosen); }
    } THEN {
        EXPECT_EQ(holder->status1, STATUS1_SLEEP_TURN(B_POLISHED_MOVE_BALANCE ? sleep : sleep > 2 ? sleep - 2 : 0));
        EXPECT_EQ(holder->pp[0], 5);
    }
}

SINGLE_BATTLE_TEST("Polished sleeping flinch resumes waking on the next non-flinched attempt")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Item item = ITEM_KINGS_ROCK;
    enum Ability ability = ABILITY_STEADFAST, attackerAbility = ABILITY_NONE;
    enum Move chosen = MOVE_TACKLE;
    u32 sleep = 1;
    for (u32 side = 0; side < 2; side++)
        PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 20 : 200); MaxHP(1000); HP(1000); Attack(10);
            Ability(holder == player ? ability : attackerAbility);
            Item(holder == player ? ITEM_NONE : item);
            if (holder == player) { Status1(STATUS1_SLEEP_TURN(sleep)); MovesWithPP({chosen, 5}, {MOVE_POUND, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 20 : 200); MaxHP(1000); HP(1000); Attack(10);
            Ability(holder == opponent ? ability : attackerAbility);
            Item(holder == opponent ? ITEM_NONE : item);
            if (holder == opponent) { Status1(STATUS1_SLEEP_TURN(sleep)); MovesWithPP({chosen, 5}, {MOVE_POUND, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, MOVE_TACKLE, WITH_RNG(attackerAbility == ABILITY_STENCH ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, TRUE)); MOVE(holder, chosen); }
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, chosen); }
    } THEN {
        EXPECT_EQ(holder->status1, STATUS1_NONE);
        EXPECT_EQ(holder->pp[0], 4);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + 1);
    }
}

SINGLE_BATTLE_TEST("Polished sleeping flinch can repeat without consuming sleep turns and then resumes Sleep Talk")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Item item = ITEM_KINGS_ROCK;
    enum Ability ability = ABILITY_STEADFAST, attackerAbility = ABILITY_NONE;
    enum Move chosen = MOVE_TACKLE;
    u32 sleep = 1;
    for (u32 side = 0; side < 2; side++)
        PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; sleep = 3; chosen = MOVE_SLEEP_TALK; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 20 : 200); MaxHP(1000); HP(1000); Attack(10);
            Ability(holder == player ? ability : attackerAbility);
            Item(holder == player ? ITEM_NONE : item);
            if (holder == player) { Status1(STATUS1_SLEEP_TURN(sleep)); MovesWithPP({chosen, 5}, {MOVE_POUND, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 20 : 200); MaxHP(1000); HP(1000); Attack(10);
            Ability(holder == opponent ? ability : attackerAbility);
            Item(holder == opponent ? ITEM_NONE : item);
            if (holder == opponent) { Status1(STATUS1_SLEEP_TURN(sleep)); MovesWithPP({chosen, 5}, {MOVE_POUND, 5}); }
        }
    } WHEN {
        for (u32 turn = 0; turn < 3; turn++)
            TURN { MOVE(other, MOVE_TACKLE, WITH_RNG(attackerAbility == ABILITY_STENCH ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, TRUE)); MOVE(holder, chosen); }
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, chosen); }
    } SCENE {
        if (B_POLISHED_MOVE_BALANCE) {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_SLEEP_TALK, holder);
            ANIMATION(ANIM_TYPE_MOVE, MOVE_POUND, holder);
        } else {
            NONE_OF { ANIMATION(ANIM_TYPE_MOVE, MOVE_SLEEP_TALK, holder); ANIMATION(ANIM_TYPE_MOVE, MOVE_POUND, holder); }
        }
    } THEN {
        EXPECT_EQ(holder->status1, STATUS1_SLEEP_TURN(B_POLISHED_MOVE_BALANCE ? 2 : 0));
        EXPECT_EQ(holder->pp[0], 4);
        EXPECT_EQ(holder->pp[1], 5);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + 3);
    }
}

SINGLE_BATTLE_TEST("Polished sleeping flinch keeps ordinary sleep progression after a failed flinch roll")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Item item = ITEM_KINGS_ROCK;
    enum Ability ability = ABILITY_STEADFAST, attackerAbility = ABILITY_NONE;
    enum Move chosen = MOVE_TACKLE;
    u32 sleep = 1;
    for (u32 mode = 0; mode < 3; mode++)
        for (u32 turns = 1; turns <= 3; turns++)
            for (u32 side = 0; side < 2; side++)
                PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; item = mode == 1 ? ITEM_RAZOR_FANG : mode == 2 ? ITEM_NONE : ITEM_KINGS_ROCK; attackerAbility = mode == 2 ? ABILITY_STENCH : ABILITY_NONE; sleep = turns; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 20 : 200); MaxHP(1000); HP(1000); Attack(10);
            Ability(holder == player ? ability : attackerAbility);
            Item(holder == player ? ITEM_NONE : item);
            if (holder == player) { Status1(STATUS1_SLEEP_TURN(sleep)); MovesWithPP({chosen, 5}, {MOVE_POUND, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 20 : 200); MaxHP(1000); HP(1000); Attack(10);
            Ability(holder == opponent ? ability : attackerAbility);
            Item(holder == opponent ? ITEM_NONE : item);
            if (holder == opponent) { Status1(STATUS1_SLEEP_TURN(sleep)); MovesWithPP({chosen, 5}, {MOVE_POUND, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, MOVE_TACKLE, WITH_RNG(attackerAbility == ABILITY_STENCH ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, FALSE)); MOVE(holder, chosen); }
    } THEN {
        EXPECT_EQ(holder->status1, STATUS1_SLEEP_TURN(sleep - 1));
        EXPECT_EQ(holder->pp[0], sleep == 1 ? 4 : 5);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished sleeping flinch rechecks Inner Focus after Mold Breaker for waking and Sleep Talk")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Item item = ITEM_KINGS_ROCK;
    enum Ability ability = ABILITY_STEADFAST, attackerAbility = ABILITY_NONE;
    enum Move chosen = MOVE_TACKLE;
    u32 sleep = 1;
    for (u32 mold = 0; mold < 2; mold++)
        for (u32 turns = 1; turns <= 2; turns++)
            for (u32 side = 0; side < 2; side++)
                PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; sleep = turns; chosen = turns == 1 ? MOVE_TACKLE : MOVE_SLEEP_TALK; ability = ABILITY_INNER_FOCUS; attackerAbility = mold ? ABILITY_MOLD_BREAKER : ABILITY_NONE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 20 : 200); MaxHP(1000); HP(1000); Attack(10);
            Ability(holder == player ? ability : attackerAbility);
            Item(holder == player ? ITEM_NONE : item);
            if (holder == player) { Status1(STATUS1_SLEEP_TURN(sleep)); MovesWithPP({chosen, 5}, {MOVE_POUND, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 20 : 200); MaxHP(1000); HP(1000); Attack(10);
            Ability(holder == opponent ? ability : attackerAbility);
            Item(holder == opponent ? ITEM_NONE : item);
            if (holder == opponent) { Status1(STATUS1_SLEEP_TURN(sleep)); MovesWithPP({chosen, 5}, {MOVE_POUND, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, MOVE_TACKLE, WITH_RNG(attackerAbility == ABILITY_STENCH ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, TRUE)); MOVE(holder, chosen); }
    } THEN {
        EXPECT_EQ(holder->status1, STATUS1_SLEEP_TURN(sleep - 1));
        EXPECT_EQ(holder->pp[0], B_POLISHED_MOVE_BALANCE || attackerAbility != ABILITY_MOLD_BREAKER ? 4 : 5);
        EXPECT_EQ(holder->pp[1], 5);
    }
}

SINGLE_BATTLE_TEST("Polished sleeping flinch honors Neutralizing Gas suppression of Inner Focus and Steadfast")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Item item = ITEM_KINGS_ROCK;
    enum Ability ability = ABILITY_STEADFAST, attackerAbility = ABILITY_NONE;
    enum Move chosen = MOVE_TACKLE;
    u32 sleep = 1;
    for (u32 a = 0; a < 2; a++)
        for (u32 turns = 1; turns <= 2; turns++)
            for (u32 side = 0; side < 2; side++)
                PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; sleep = turns; ability = a ? ABILITY_STEADFAST : ABILITY_INNER_FOCUS; attackerAbility = ABILITY_NEUTRALIZING_GAS; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 20 : 200); MaxHP(1000); HP(1000); Attack(10);
            Ability(holder == player ? ability : attackerAbility);
            Item(holder == player ? ITEM_NONE : item);
            if (holder == player) { Status1(STATUS1_SLEEP_TURN(sleep)); MovesWithPP({chosen, 5}, {MOVE_POUND, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 20 : 200); MaxHP(1000); HP(1000); Attack(10);
            Ability(holder == opponent ? ability : attackerAbility);
            Item(holder == opponent ? ITEM_NONE : item);
            if (holder == opponent) { Status1(STATUS1_SLEEP_TURN(sleep)); MovesWithPP({chosen, 5}, {MOVE_POUND, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, MOVE_TACKLE, WITH_RNG(attackerAbility == ABILITY_STENCH ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, TRUE)); MOVE(holder, chosen); }
    } THEN {
        EXPECT_EQ(holder->status1, STATUS1_SLEEP_TURN(B_POLISHED_MOVE_BALANCE ? sleep : sleep - 1));
        EXPECT_EQ(holder->pp[0], 5);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished sleeping flinch does not bypass Shield Dust protection")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Item item = ITEM_KINGS_ROCK;
    enum Ability ability = ABILITY_STEADFAST, attackerAbility = ABILITY_NONE;
    enum Move chosen = MOVE_TACKLE;
    u32 sleep = 1;
    for (u32 mode = 0; mode < 3; mode++)
        for (u32 turns = 1; turns <= 2; turns++)
            for (u32 side = 0; side < 2; side++)
                PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; item = mode == 1 ? ITEM_RAZOR_FANG : mode == 2 ? ITEM_NONE : ITEM_KINGS_ROCK; attackerAbility = mode == 2 ? ABILITY_STENCH : ABILITY_NONE; sleep = turns; ability = ABILITY_SHIELD_DUST; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 20 : 200); MaxHP(1000); HP(1000); Attack(10);
            Ability(holder == player ? ability : attackerAbility);
            Item(holder == player ? ITEM_NONE : item);
            if (holder == player) { Status1(STATUS1_SLEEP_TURN(sleep)); MovesWithPP({chosen, 5}, {MOVE_POUND, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 20 : 200); MaxHP(1000); HP(1000); Attack(10);
            Ability(holder == opponent ? ability : attackerAbility);
            Item(holder == opponent ? ITEM_NONE : item);
            if (holder == opponent) { Status1(STATUS1_SLEEP_TURN(sleep)); MovesWithPP({chosen, 5}, {MOVE_POUND, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, MOVE_TACKLE, WITH_RNG(attackerAbility == ABILITY_STENCH ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, TRUE)); MOVE(holder, chosen); }
    } THEN {
        EXPECT_EQ(holder->status1, STATUS1_SLEEP_TURN(sleep - 1));
        EXPECT_EQ(holder->pp[0], sleep == 1 ? 4 : 5);
    }
}

SINGLE_BATTLE_TEST("Polished sleeping flinch applies the status ordering to an optional Fake Out producer")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Item item = ITEM_KINGS_ROCK;
    enum Ability ability = ABILITY_STEADFAST, attackerAbility = ABILITY_NONE;
    enum Move chosen = MOVE_TACKLE;
    u32 sleep = 1;
    for (u32 turns = 1; turns <= 2; turns++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; sleep = turns; item = ITEM_NONE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 20 : 200); MaxHP(1000); HP(1000); Attack(10);
            Ability(holder == player ? ability : attackerAbility);
            Item(holder == player ? ITEM_NONE : item);
            if (holder == player) { Status1(STATUS1_SLEEP_TURN(sleep)); MovesWithPP({chosen, 5}, {MOVE_POUND, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 20 : 200); MaxHP(1000); HP(1000); Attack(10);
            Ability(holder == opponent ? ability : attackerAbility);
            Item(holder == opponent ? ITEM_NONE : item);
            if (holder == opponent) { Status1(STATUS1_SLEEP_TURN(sleep)); MovesWithPP({chosen, 5}, {MOVE_POUND, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, MOVE_FAKE_OUT); MOVE(holder, chosen); }
    } THEN {
        EXPECT_EQ(holder->status1, STATUS1_SLEEP_TURN(B_POLISHED_MOVE_BALANCE ? sleep : sleep - 1));
        EXPECT_EQ(holder->pp[0], 5);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + (B_POLISHED_MOVE_BALANCE || sleep == 1));
    }
}

DOUBLE_BATTLE_TEST("Polished sleeping flinch retains native sleep and flinch order in doubles")
{
    struct BattlePokemon *holder = playerLeft, *other = opponentLeft;
    enum Item item = ITEM_KINGS_ROCK;
    enum Ability ability = ABILITY_STEADFAST, attackerAbility = ABILITY_NONE;
    enum Move chosen = MOVE_TACKLE;
    u32 sleep = 1;
    for (u32 turns = 1; turns <= 2; turns++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { holder = side ? opponentLeft : playerLeft; other = side ? playerLeft : opponentLeft; sleep = turns; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == playerLeft ? 20 : 200); MaxHP(1000); HP(1000); Attack(10);
            Ability(holder == playerLeft ? ability : attackerAbility);
            Item(holder == playerLeft ? ITEM_NONE : item);
            if (holder == playerLeft) { Status1(STATUS1_SLEEP_TURN(sleep)); MovesWithPP({chosen, 5}, {MOVE_POUND, 5}); }
        }
        PLAYER(SPECIES_WYNAUT) { Speed(70); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponentLeft ? 20 : 200); MaxHP(1000); HP(1000); Attack(10);
            Ability(holder == opponentLeft ? ability : attackerAbility);
            Item(holder == opponentLeft ? ITEM_NONE : item);
            if (holder == opponentLeft) { Status1(STATUS1_SLEEP_TURN(sleep)); MovesWithPP({chosen, 5}, {MOVE_POUND, 5}); }
        }
        OPPONENT(SPECIES_WYNAUT) { Speed(90); Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(other, MOVE_TACKLE, target: holder, WITH_RNG(attackerAbility == ABILITY_STENCH ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, TRUE)); MOVE(holder, chosen, target: other); }
    } THEN {
        EXPECT_EQ(holder->status1, STATUS1_SLEEP_TURN(sleep - 1));
        EXPECT_EQ(holder->pp[0], 5);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + (sleep == 1));
    }
}

SINGLE_BATTLE_TEST("Polished sleeping flinch checks either active Terastal with a self-targeted move")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Item item = ITEM_KINGS_ROCK;
    enum Ability ability = ABILITY_STEADFAST, attackerAbility = ABILITY_NONE;
    enum Move chosen = MOVE_TACKLE;
    u32 sleep = 1;
    bool32 holderTera = FALSE;
    for (u32 tera = 0; tera < 2; tera++)
        for (u32 turns = 1; turns <= 2; turns++)
            for (u32 side = 0; side < 2; side++)
                PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; sleep = turns; holderTera = tera; chosen = MOVE_CELEBRATE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 20 : 200); MaxHP(1000); HP(1000); Attack(10);
            Ability(holder == player ? ability : attackerAbility);
            Item(holder == player ? ITEM_NONE : item);
            if (holder == player) { Status1(STATUS1_SLEEP_TURN(sleep)); MovesWithPP({chosen, 5}, {MOVE_POUND, 5}); }
            TeraType(TYPE_NORMAL);
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 20 : 200); MaxHP(1000); HP(1000); Attack(10);
            Ability(holder == opponent ? ability : attackerAbility);
            Item(holder == opponent ? ITEM_NONE : item);
            if (holder == opponent) { Status1(STATUS1_SLEEP_TURN(sleep)); MovesWithPP({chosen, 5}, {MOVE_POUND, 5}); }
            TeraType(TYPE_NORMAL);
        }
    } WHEN {
        TURN { MOVE(other, MOVE_TACKLE, gimmick: holderTera ? GIMMICK_NONE : GIMMICK_TERA, WITH_RNG(attackerAbility == ABILITY_STENCH ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, TRUE)); MOVE(holder, chosen, gimmick: holderTera ? GIMMICK_TERA : GIMMICK_NONE); }
    } THEN {
        EXPECT_EQ(holder->status1, STATUS1_SLEEP_TURN(sleep - 1));
        EXPECT_EQ(holder->pp[0], 5);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + (sleep == 1));
    }
}

SINGLE_BATTLE_TEST("Polished sleeping flinch preserves party sleep when switched out after flinching")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Item item = ITEM_KINGS_ROCK;
    enum Ability ability = ABILITY_STEADFAST, attackerAbility = ABILITY_NONE;
    enum Move chosen = MOVE_TACKLE;
    u32 sleep = 1;
    for (u32 side = 0; side < 2; side++)
        PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 20 : 200); MaxHP(1000); HP(1000); Attack(10);
            Ability(holder == player ? ability : attackerAbility);
            Item(holder == player ? ITEM_NONE : item);
            if (holder == player) { Status1(STATUS1_SLEEP_TURN(sleep)); MovesWithPP({chosen, 5}, {MOVE_POUND, 5}); }
        }
        PLAYER(SPECIES_WYNAUT) { Speed(70); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 20 : 200); MaxHP(1000); HP(1000); Attack(10);
            Ability(holder == opponent ? ability : attackerAbility);
            Item(holder == opponent ? ITEM_NONE : item);
            if (holder == opponent) { Status1(STATUS1_SLEEP_TURN(sleep)); MovesWithPP({chosen, 5}, {MOVE_POUND, 5}); }
        }
        OPPONENT(SPECIES_WYNAUT) { Speed(90); Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(other, MOVE_TACKLE, WITH_RNG(attackerAbility == ABILITY_STENCH ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, TRUE)); MOVE(holder, chosen); }
        TURN { SWITCH(holder, 1); MOVE(other, MOVE_CELEBRATE); }
    } THEN {
        EXPECT_EQ(GetMonData(holder == player ? &gParties[B_TRAINER_PLAYER][0] : &gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_STATUS), B_POLISHED_MOVE_BALANCE ? STATUS1_SLEEP_TURN(1) : STATUS1_NONE);
        EXPECT_EQ(GetMonData(holder == player ? &gParties[B_TRAINER_PLAYER][0] : &gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_PP1), 5);
    }
}
