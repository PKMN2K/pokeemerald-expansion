#include "global.h"
#include "test/battle.h"

static const enum Move sFlinchMoves[] = {
    MOVE_AIR_SLASH, MOVE_STOMP, MOVE_HEADBUTT, MOVE_BITE, MOVE_WATERFALL,
    MOVE_ICICLE_CRASH, MOVE_IRON_HEAD, MOVE_EXTRASENSORY, MOVE_ROCK_SLIDE,
    MOVE_HYPER_FANG, MOVE_ZEN_HEADBUTT, MOVE_ASTONISH, MOVE_DARK_PULSE,
};

SINGLE_BATTLE_TEST("Polished flinch focus rechecks active Inner Focus after all source flinch moves with Mold Breaker")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Ability ability = ABILITY_INNER_FOCUS, attackerAbility = ABILITY_MOLD_BREAKER;
    enum Move move = MOVE_BITE, chosen = MOVE_TACKLE;
    enum Item item = ITEM_NONE, holderItem = ITEM_NONE;
    u32 status = STATUS1_NONE;
    for (u32 m = 0; m < ARRAY_COUNT(sFlinchMoves); m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; move = sFlinchMoves[m]; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 50 : 200); Gender(MON_FEMALE);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == player ? ability : attackerAbility);
            Item(holder == player ? holderItem : item);
            if (holder == player) { Status1(status); MovesWithPP({chosen, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200); Gender(MON_MALE);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? ability : attackerAbility);
            Item(holder == opponent ? holderItem : item);
            if (holder == opponent) { Status1(status); MovesWithPP({chosen, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, move, hit: TRUE, secondaryEffect: TRUE, WITH_RNG(RNG_HOLD_EFFECT_FLINCH, TRUE)); MOVE(holder, chosen); }

    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, other);
        if (B_POLISHED_MOVE_BALANCE) {
            ANIMATION(ANIM_TYPE_MOVE, chosen, holder);
        } else {
            if (holder == player) MESSAGE("Wobbuffet flinched and couldn't move!");
            else MESSAGE("The opposing Wobbuffet flinched and couldn't move!");
            NOT ANIMATION(ANIM_TYPE_MOVE, chosen, holder);
        }

    } THEN {
        EXPECT_EQ(holder->pp[0], B_POLISHED_MOVE_BALANCE ? 4 : 5);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished flinch focus rechecks active Inner Focus after Mold Breaker item flinches")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Ability ability = ABILITY_INNER_FOCUS, attackerAbility = ABILITY_MOLD_BREAKER;
    enum Move move = MOVE_BITE, chosen = MOVE_TACKLE;
    enum Item item = ITEM_NONE, holderItem = ITEM_NONE;
    u32 status = STATUS1_NONE;
    for (u32 rock = 0; rock < 2; rock++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; move = MOVE_TACKLE; item = rock ? ITEM_RAZOR_FANG : ITEM_KINGS_ROCK; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 50 : 200); Gender(MON_FEMALE);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == player ? ability : attackerAbility);
            Item(holder == player ? holderItem : item);
            if (holder == player) { Status1(status); MovesWithPP({chosen, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200); Gender(MON_MALE);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? ability : attackerAbility);
            Item(holder == opponent ? holderItem : item);
            if (holder == opponent) { Status1(status); MovesWithPP({chosen, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, move, hit: TRUE, secondaryEffect: TRUE, WITH_RNG(RNG_HOLD_EFFECT_FLINCH, TRUE)); MOVE(holder, chosen); }

    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, other);
        if (B_POLISHED_MOVE_BALANCE) {
            ANIMATION(ANIM_TYPE_MOVE, chosen, holder);
        } else {
            if (holder == player) MESSAGE("Wobbuffet flinched and couldn't move!");
            else MESSAGE("The opposing Wobbuffet flinched and couldn't move!");
            NOT ANIMATION(ANIM_TYPE_MOVE, chosen, holder);
        }

    } THEN {
        EXPECT_EQ(holder->pp[0], B_POLISHED_MOVE_BALANCE ? 4 : 5);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished flinch focus preserves protection without Mold Breaker and ignores failed effect rolls")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Ability ability = ABILITY_INNER_FOCUS, attackerAbility = ABILITY_MOLD_BREAKER;
    enum Move move = MOVE_BITE, chosen = MOVE_TACKLE;
    enum Item item = ITEM_NONE, holderItem = ITEM_NONE;
    u32 status = STATUS1_NONE;
    bool32 flinch = FALSE;
    for (u32 mold = 0; mold < 2; mold++)
        for (u32 roll = 0; roll < 2; roll++)
            for (u32 side = 0; side < 2; side++)
                PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; attackerAbility = mold ? ABILITY_MOLD_BREAKER : ABILITY_NONE; flinch = roll; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 50 : 200); Gender(MON_FEMALE);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == player ? ability : attackerAbility);
            Item(holder == player ? holderItem : item);
            if (holder == player) { Status1(status); MovesWithPP({chosen, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200); Gender(MON_MALE);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? ability : attackerAbility);
            Item(holder == opponent ? holderItem : item);
            if (holder == opponent) { Status1(status); MovesWithPP({chosen, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, move, hit: TRUE, secondaryEffect: flinch, WITH_RNG(RNG_HOLD_EFFECT_FLINCH, TRUE)); MOVE(holder, chosen); }

    } THEN {
        EXPECT_EQ(holder->pp[0], B_POLISHED_MOVE_BALANCE || attackerAbility != ABILITY_MOLD_BREAKER || !flinch ? 4 : 5);
    }
}

SINGLE_BATTLE_TEST("Polished flinch focus still cancels unprotected actions and activates Steadfast")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Ability ability = ABILITY_INNER_FOCUS, attackerAbility = ABILITY_MOLD_BREAKER;
    enum Move move = MOVE_BITE, chosen = MOVE_TACKLE;
    enum Item item = ITEM_NONE, holderItem = ITEM_NONE;
    u32 status = STATUS1_NONE;
    for (u32 a = 0; a < 2; a++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; ability = a ? ABILITY_STEADFAST : ABILITY_NONE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 50 : 200); Gender(MON_FEMALE);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == player ? ability : attackerAbility);
            Item(holder == player ? holderItem : item);
            if (holder == player) { Status1(status); MovesWithPP({chosen, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200); Gender(MON_MALE);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? ability : attackerAbility);
            Item(holder == opponent ? holderItem : item);
            if (holder == opponent) { Status1(status); MovesWithPP({chosen, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, move, hit: TRUE, secondaryEffect: TRUE, WITH_RNG(RNG_HOLD_EFFECT_FLINCH, TRUE)); MOVE(holder, chosen); }

    } THEN {
        EXPECT_EQ(holder->pp[0], 5);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + (ability == ABILITY_STEADFAST));
    }
}

SINGLE_BATTLE_TEST("Polished flinch focus honors Neutralizing Gas for move and item flinches")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Ability ability = ABILITY_INNER_FOCUS, attackerAbility = ABILITY_MOLD_BREAKER;
    enum Move move = MOVE_BITE, chosen = MOVE_TACKLE;
    enum Item item = ITEM_NONE, holderItem = ITEM_NONE;
    u32 status = STATUS1_NONE;
    for (u32 mode = 0; mode < 3; mode++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; attackerAbility = ABILITY_NEUTRALIZING_GAS; move = mode ? MOVE_TACKLE : MOVE_BITE; item = mode == 1 ? ITEM_KINGS_ROCK : mode == 2 ? ITEM_RAZOR_FANG : ITEM_NONE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 50 : 200); Gender(MON_FEMALE);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == player ? ability : attackerAbility);
            Item(holder == player ? holderItem : item);
            if (holder == player) { Status1(status); MovesWithPP({chosen, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200); Gender(MON_MALE);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? ability : attackerAbility);
            Item(holder == opponent ? holderItem : item);
            if (holder == opponent) { Status1(status); MovesWithPP({chosen, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, move, hit: TRUE, secondaryEffect: TRUE, WITH_RNG(RNG_HOLD_EFFECT_FLINCH, TRUE)); MOVE(holder, chosen); }

    } THEN {
        EXPECT_EQ(holder->pp[0], 5);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished flinch focus protects again after Neutralizing Gas switches out")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Ability ability = ABILITY_INNER_FOCUS, attackerAbility = ABILITY_MOLD_BREAKER;
    enum Move move = MOVE_BITE, chosen = MOVE_TACKLE;
    enum Item item = ITEM_NONE, holderItem = ITEM_NONE;
    u32 status = STATUS1_NONE;
    for (u32 side = 0; side < 2; side++)
        PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; attackerAbility = ABILITY_NEUTRALIZING_GAS; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 50 : 200); Gender(MON_FEMALE);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == player ? ability : attackerAbility);
            Item(holder == player ? holderItem : item);
            if (holder == player) { Status1(status); MovesWithPP({chosen, 5}); }
        }
        PLAYER(SPECIES_WOBBUFFET) { Speed(200); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(ABILITY_MOLD_BREAKER); }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200); Gender(MON_MALE);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? ability : attackerAbility);
            Item(holder == opponent ? holderItem : item);
            if (holder == opponent) { Status1(status); MovesWithPP({chosen, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(200); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(ABILITY_MOLD_BREAKER); }
    } WHEN {
        TURN { MOVE(other, move, hit: TRUE, secondaryEffect: TRUE, WITH_RNG(RNG_HOLD_EFFECT_FLINCH, TRUE)); MOVE(holder, chosen); }
        TURN { SWITCH(other, 1); MOVE(holder, chosen); }
        TURN { MOVE(other, move, hit: TRUE, secondaryEffect: TRUE, WITH_RNG(RNG_HOLD_EFFECT_FLINCH, TRUE)); MOVE(holder, chosen); }

    } THEN {
        EXPECT_EQ(holder->pp[0], B_POLISHED_MOVE_BALANCE ? 3 : 4);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished flinch focus retains optional Gastro Acid suppression")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Ability ability = ABILITY_INNER_FOCUS, attackerAbility = ABILITY_MOLD_BREAKER;
    enum Move move = MOVE_BITE, chosen = MOVE_TACKLE;
    enum Item item = ITEM_NONE, holderItem = ITEM_NONE;
    u32 status = STATUS1_NONE;
    for (u32 side = 0; side < 2; side++)
        PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 50 : 200); Gender(MON_FEMALE);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == player ? ability : attackerAbility);
            Item(holder == player ? holderItem : item);
            if (holder == player) { Status1(status); MovesWithPP({chosen, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200); Gender(MON_MALE);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? ability : attackerAbility);
            Item(holder == opponent ? holderItem : item);
            if (holder == opponent) { Status1(status); MovesWithPP({chosen, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, MOVE_GASTRO_ACID); MOVE(holder, chosen); }
        TURN { MOVE(other, move, hit: TRUE, secondaryEffect: TRUE, WITH_RNG(RNG_HOLD_EFFECT_FLINCH, TRUE)); MOVE(holder, chosen); }

    } THEN {
        EXPECT_EQ(holder->pp[0], 4);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished flinch focus preserves burn poison and both paralysis action outcomes")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Ability ability = ABILITY_INNER_FOCUS, attackerAbility = ABILITY_MOLD_BREAKER;
    enum Move move = MOVE_BITE, chosen = MOVE_TACKLE;
    enum Item item = ITEM_NONE, holderItem = ITEM_NONE;
    u32 status = STATUS1_NONE;
    bool32 blocked = FALSE;
    for (u32 mode = 0; mode < 4; mode++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; status = mode == 0 ? STATUS1_BURN : mode == 1 ? STATUS1_POISON : STATUS1_PARALYSIS; blocked = mode == 3; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 50 : 200); Gender(MON_FEMALE);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == player ? ability : attackerAbility);
            Item(holder == player ? holderItem : item);
            if (holder == player) { Status1(status); MovesWithPP({chosen, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200); Gender(MON_MALE);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? ability : attackerAbility);
            Item(holder == opponent ? holderItem : item);
            if (holder == opponent) { Status1(status); MovesWithPP({chosen, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, move, hit: TRUE, secondaryEffect: TRUE, WITH_RNG(RNG_HOLD_EFFECT_FLINCH, TRUE)); MOVE(holder, chosen, WITH_RNG(RNG_PARALYSIS, blocked)); }

    } THEN {
        EXPECT_EQ(holder->status1, status);
        EXPECT_EQ(holder->pp[0], B_POLISHED_MOVE_BALANCE && !blocked ? 4 : 5);
    }
}

SINGLE_BATTLE_TEST("Polished flinch focus still evaluates confusion and infatuation after discarding flinch")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Ability ability = ABILITY_INNER_FOCUS, attackerAbility = ABILITY_MOLD_BREAKER;
    enum Move move = MOVE_BITE, chosen = MOVE_TACKLE;
    enum Item item = ITEM_NONE, holderItem = ITEM_NONE;
    u32 status = STATUS1_NONE;
    bool32 infatuated = FALSE, blocked = FALSE;
    for (u32 attract = 0; attract < 2; attract++)
        for (u32 roll = 0; roll < 2; roll++)
            for (u32 side = 0; side < 2; side++)
                PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; infatuated = attract; blocked = roll; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 50 : 200); Gender(MON_FEMALE);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == player ? ability : attackerAbility);
            Item(holder == player ? holderItem : item);
            if (holder == player) { Status1(status); MovesWithPP({chosen, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200); Gender(MON_MALE);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? ability : attackerAbility);
            Item(holder == opponent ? holderItem : item);
            if (holder == opponent) { Status1(status); MovesWithPP({chosen, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, infatuated ? MOVE_ATTRACT : MOVE_CONFUSE_RAY); MOVE(holder, chosen, WITH_RNG(infatuated ? RNG_INFATUATION : RNG_CONFUSION, FALSE)); }
        TURN { MOVE(other, move, secondaryEffect: TRUE); MOVE(holder, chosen, WITH_RNG(infatuated ? RNG_INFATUATION : RNG_CONFUSION, blocked)); }

    } THEN {
        EXPECT_EQ(holder->pp[0], B_POLISHED_MOVE_BALANCE && !blocked ? 3 : 4);
    }
}

SINGLE_BATTLE_TEST("Polished flinch focus does not cancel a charged Solar Beam protected by active Inner Focus")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Ability ability = ABILITY_INNER_FOCUS, attackerAbility = ABILITY_MOLD_BREAKER;
    enum Move move = MOVE_BITE, chosen = MOVE_TACKLE;
    enum Item item = ITEM_NONE, holderItem = ITEM_NONE;
    u32 status = STATUS1_NONE;
    for (u32 side = 0; side < 2; side++)
        PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; chosen = MOVE_SOLAR_BEAM; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 50 : 200); Gender(MON_FEMALE);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == player ? ability : attackerAbility);
            Item(holder == player ? holderItem : item);
            if (holder == player) { Status1(status); MovesWithPP({chosen, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200); Gender(MON_MALE);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? ability : attackerAbility);
            Item(holder == opponent ? holderItem : item);
            if (holder == opponent) { Status1(status); MovesWithPP({chosen, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, chosen); }
        TURN { MOVE(other, move, secondaryEffect: TRUE); SKIP_TURN(holder); }

    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, other);
        if (B_POLISHED_MOVE_BALANCE) {
            ANIMATION(ANIM_TYPE_MOVE, chosen, holder);
        } else {
            if (holder == player) MESSAGE("Wobbuffet flinched and couldn't move!");
            else MESSAGE("The opposing Wobbuffet flinched and couldn't move!");
            NOT ANIMATION(ANIM_TYPE_MOVE, chosen, holder);
        }

    } THEN {
        EXPECT_EQ(holder->pp[0], 4);
    }
}

SINGLE_BATTLE_TEST("Polished flinch focus does not carry a rejected flag into the next action")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Ability ability = ABILITY_INNER_FOCUS, attackerAbility = ABILITY_MOLD_BREAKER;
    enum Move move = MOVE_BITE, chosen = MOVE_TACKLE;
    enum Item item = ITEM_NONE, holderItem = ITEM_NONE;
    u32 status = STATUS1_NONE;
    for (u32 side = 0; side < 2; side++)
        PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 50 : 200); Gender(MON_FEMALE);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == player ? ability : attackerAbility);
            Item(holder == player ? holderItem : item);
            if (holder == player) { Status1(status); MovesWithPP({chosen, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200); Gender(MON_MALE);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? ability : attackerAbility);
            Item(holder == opponent ? holderItem : item);
            if (holder == opponent) { Status1(status); MovesWithPP({chosen, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, move, hit: TRUE, secondaryEffect: TRUE, WITH_RNG(RNG_HOLD_EFFECT_FLINCH, TRUE)); MOVE(holder, chosen); }
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, chosen); }

    } THEN {
        EXPECT_EQ(holder->pp[0], B_POLISHED_MOVE_BALANCE ? 3 : 4);
    }
}

SINGLE_BATTLE_TEST("Polished flinch focus applies the active ability check to optional Fake Out")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Ability ability = ABILITY_INNER_FOCUS, attackerAbility = ABILITY_MOLD_BREAKER;
    enum Move move = MOVE_BITE, chosen = MOVE_TACKLE;
    enum Item item = ITEM_NONE, holderItem = ITEM_NONE;
    u32 status = STATUS1_NONE;
    for (u32 side = 0; side < 2; side++)
        PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; move = MOVE_FAKE_OUT; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 50 : 200); Gender(MON_FEMALE);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == player ? ability : attackerAbility);
            Item(holder == player ? holderItem : item);
            if (holder == player) { Status1(status); MovesWithPP({chosen, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200); Gender(MON_MALE);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? ability : attackerAbility);
            Item(holder == opponent ? holderItem : item);
            if (holder == opponent) { Status1(status); MovesWithPP({chosen, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, move, hit: TRUE, secondaryEffect: TRUE, WITH_RNG(RNG_HOLD_EFFECT_FLINCH, TRUE)); MOVE(holder, chosen); }

    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, other);
        if (B_POLISHED_MOVE_BALANCE) {
            ANIMATION(ANIM_TYPE_MOVE, chosen, holder);
        } else {
            if (holder == player) MESSAGE("Wobbuffet flinched and couldn't move!");
            else MESSAGE("The opposing Wobbuffet flinched and couldn't move!");
            NOT ANIMATION(ANIM_TYPE_MOVE, chosen, holder);
        }

    } THEN {
        EXPECT_EQ(holder->pp[0], B_POLISHED_MOVE_BALANCE ? 4 : 5);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE);
    }
}

DOUBLE_BATTLE_TEST("Polished flinch focus retains native Mold Breaker behavior in doubles")
{
    struct BattlePokemon *holder = playerLeft, *other = opponentLeft;
    enum Ability ability = ABILITY_INNER_FOCUS, attackerAbility = ABILITY_MOLD_BREAKER;
    enum Move move = MOVE_BITE, chosen = MOVE_TACKLE;
    enum Item item = ITEM_NONE, holderItem = ITEM_NONE;
    u32 status = STATUS1_NONE;
    for (u32 mode = 0; mode < 3; mode++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { holder = side ? opponentLeft : playerLeft; other = side ? playerLeft : opponentLeft; move = mode ? MOVE_TACKLE : MOVE_BITE; item = mode == 1 ? ITEM_KINGS_ROCK : mode == 2 ? ITEM_RAZOR_FANG : ITEM_NONE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == playerLeft ? 50 : 200); Gender(MON_FEMALE);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == playerLeft ? ability : attackerAbility);
            Item(holder == playerLeft ? holderItem : item);
            if (holder == playerLeft) { Status1(status); MovesWithPP({chosen, 5}); }
        }
        PLAYER(SPECIES_WYNAUT) { Speed(75); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponentLeft ? 50 : 200); Gender(MON_MALE);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponentLeft ? ability : attackerAbility);
            Item(holder == opponentLeft ? holderItem : item);
            if (holder == opponentLeft) { Status1(status); MovesWithPP({chosen, 5}); }
        }
        OPPONENT(SPECIES_WYNAUT) { Speed(90); Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(other, move, target: holder, hit: TRUE, secondaryEffect: TRUE, WITH_RNG(RNG_HOLD_EFFECT_FLINCH, TRUE)); MOVE(holder, chosen, target: other); }

    } THEN {
        EXPECT_EQ(holder->pp[0], 5);
    }
}

SINGLE_BATTLE_TEST("Polished flinch focus checks either active Terastal even for self-targeting moves")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Ability ability = ABILITY_INNER_FOCUS, attackerAbility = ABILITY_MOLD_BREAKER;
    enum Move move = MOVE_BITE, chosen = MOVE_TACKLE;
    enum Item item = ITEM_NONE, holderItem = ITEM_NONE;
    u32 status = STATUS1_NONE;
    bool32 holderTera = FALSE;
    for (u32 mode = 0; mode < 3; mode++)
        for (u32 tera = 0; tera < 2; tera++)
            for (u32 side = 0; side < 2; side++)
                PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; move = mode ? MOVE_TACKLE : MOVE_BITE; item = mode == 1 ? ITEM_KINGS_ROCK : mode == 2 ? ITEM_RAZOR_FANG : ITEM_NONE; holderTera = tera; chosen = MOVE_CELEBRATE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 50 : 200); Gender(MON_FEMALE);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == player ? ability : attackerAbility);
            Item(holder == player ? holderItem : item);
            if (holder == player) { Status1(status); MovesWithPP({chosen, 5}); }
            TeraType(TYPE_NORMAL);
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200); Gender(MON_MALE);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? ability : attackerAbility);
            Item(holder == opponent ? holderItem : item);
            if (holder == opponent) { Status1(status); MovesWithPP({chosen, 5}); }
            TeraType(TYPE_NORMAL);
        }
    } WHEN {
        TURN { MOVE(other, move, gimmick: holderTera ? GIMMICK_NONE : GIMMICK_TERA, hit: TRUE, secondaryEffect: TRUE, WITH_RNG(RNG_HOLD_EFFECT_FLINCH, TRUE)); MOVE(holder, chosen, gimmick: holderTera ? GIMMICK_TERA : GIMMICK_NONE); }

    } THEN {
        EXPECT_EQ(holder->pp[0], 5);
    }
}

SINGLE_BATTLE_TEST("Polished flinch focus retains optional Ability Shield protection")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Ability ability = ABILITY_INNER_FOCUS, attackerAbility = ABILITY_MOLD_BREAKER;
    enum Move move = MOVE_BITE, chosen = MOVE_TACKLE;
    enum Item item = ITEM_NONE, holderItem = ITEM_NONE;
    u32 status = STATUS1_NONE;
    for (u32 side = 0; side < 2; side++)
        PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; holderItem = ITEM_ABILITY_SHIELD; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(holder == player ? 50 : 200); Gender(MON_FEMALE);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == player ? ability : attackerAbility);
            Item(holder == player ? holderItem : item);
            if (holder == player) { Status1(status); MovesWithPP({chosen, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(holder == opponent ? 50 : 200); Gender(MON_MALE);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            Ability(holder == opponent ? ability : attackerAbility);
            Item(holder == opponent ? holderItem : item);
            if (holder == opponent) { Status1(status); MovesWithPP({chosen, 5}); }
        }
    } WHEN {
        TURN { MOVE(other, move, hit: TRUE, secondaryEffect: TRUE, WITH_RNG(RNG_HOLD_EFFECT_FLINCH, TRUE)); MOVE(holder, chosen); }

    } THEN {
        EXPECT_EQ(holder->pp[0], 4);
    }
}
