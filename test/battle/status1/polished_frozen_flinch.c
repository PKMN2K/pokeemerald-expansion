#include "global.h"
#include "test/battle.h"

static u32 NaturalThawRoll(bool32 thaw)
{
    return B_POLISHED_MOVE_BALANCE ? (thaw ? 0 : 255) : thaw;
}

SINGLE_BATTLE_TEST("Polished frozen flinch prevents both natural thaw outcomes and activates Steadfast")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Ability ability = ABILITY_NONE;
    enum Item item = ITEM_KINGS_ROCK;
    bool32 stench = FALSE;
    bool32 thaw = FALSE;
    for (u32 mode = 0; mode < 3; mode++)
        for (u32 side = 0; side < 2; side++)
            for (u32 roll = 0; roll < 2; roll++)
                for (u32 a = 0; a < 2; a++)
                    PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; stench = mode == 2; item = stench ? ITEM_NONE : mode == 1 ? ITEM_RAZOR_FANG : ITEM_KINGS_ROCK; thaw = roll; ability = a ? ABILITY_STEADFAST : ABILITY_NONE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == player ? 20 : 200); Gender(MON_FEMALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == player ? ability : stench ? ABILITY_STENCH : ABILITY_NONE); Item(holder == player ? ITEM_NONE : item); if (holder == player) { Status1(STATUS1_FREEZE); MovesWithPP({MOVE_TACKLE, 5}); } }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponent ? 20 : 200); Gender(MON_MALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == opponent ? ability : stench ? ABILITY_STENCH : ABILITY_NONE); Item(holder == opponent ? ITEM_NONE : item); if (holder == opponent) { Status1(STATUS1_FREEZE); MovesWithPP({MOVE_TACKLE, 5}); } }
    } WHEN {
        TURN { MOVE(other, MOVE_TACKLE, WITH_RNG(stench ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, TRUE)); MOVE(holder, MOVE_TACKLE, WITH_RNG(RNG_FROZEN, NaturalThawRoll(thaw))); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, other);
        if (!B_POLISHED_MOVE_BALANCE && thaw) STATUS_ICON(holder, none: TRUE);
        if (B_POLISHED_MOVE_BALANCE || thaw) {
            if (holder == player) MESSAGE("Wobbuffet flinched and couldn't move!");
            else MESSAGE("The opposing Wobbuffet flinched and couldn't move!");
        } else {
            if (holder == player) MESSAGE("Wobbuffet is frozen solid!");
            else MESSAGE("The opposing Wobbuffet is frozen solid!");
        }
        NONE_OF { STATUS_ICON(holder, none: TRUE); ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, holder); }
    } THEN {
        EXPECT_EQ(holder->status1, B_POLISHED_MOVE_BALANCE || !thaw ? STATUS1_FREEZE : STATUS1_NONE);
        EXPECT_EQ(holder->pp[0], 5);
        EXPECT_EQ(holder->statStages[STAT_SPEED], ability == ABILITY_STEADFAST && (B_POLISHED_MOVE_BALANCE || thaw) ? DEFAULT_STAT_STAGE + 1 : DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished frozen flinch does not interfere when the flinch roll fails")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Item item = ITEM_KINGS_ROCK;
    bool32 stench = FALSE;
    bool32 thaw = FALSE;
    for (u32 mode = 0; mode < 3; mode++)
        for (u32 side = 0; side < 2; side++)
            for (u32 roll = 0; roll < 2; roll++)
                PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; stench = mode == 2; item = stench ? ITEM_NONE : mode == 1 ? ITEM_RAZOR_FANG : ITEM_KINGS_ROCK; thaw = roll; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == player ? 20 : 200); Gender(MON_FEMALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == player ? ABILITY_STEADFAST : stench ? ABILITY_STENCH : ABILITY_NONE); Item(holder == player ? ITEM_NONE : item); if (holder == player) { Status1(STATUS1_FREEZE); MovesWithPP({MOVE_TACKLE, 5}); } }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponent ? 20 : 200); Gender(MON_MALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == opponent ? ABILITY_STEADFAST : stench ? ABILITY_STENCH : ABILITY_NONE); Item(holder == opponent ? ITEM_NONE : item); if (holder == opponent) { Status1(STATUS1_FREEZE); MovesWithPP({MOVE_TACKLE, 5}); } }
    } WHEN {
        TURN { MOVE(other, MOVE_TACKLE, WITH_RNG(stench ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, FALSE)); MOVE(holder, MOVE_TACKLE, WITH_RNG(RNG_FROZEN, NaturalThawRoll(thaw))); }
    } THEN {
        EXPECT_EQ(holder->status1, thaw ? STATUS1_NONE : STATUS1_FREEZE);
        EXPECT_EQ(holder->pp[0], thaw ? 4 : 5);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished frozen flinch uses a fresh thaw decision on the next non-flinched attempt")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Item item = ITEM_KINGS_ROCK;
    bool32 stench = FALSE;
    bool32 thaw = FALSE;
    for (u32 mode = 0; mode < 3; mode++)
        for (u32 side = 0; side < 2; side++)
            for (u32 roll = 0; roll < 2; roll++)
                PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; stench = mode == 2; item = stench ? ITEM_NONE : mode == 1 ? ITEM_RAZOR_FANG : ITEM_KINGS_ROCK; thaw = roll; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == player ? 20 : 200); Gender(MON_FEMALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == player ? ABILITY_STEADFAST : stench ? ABILITY_STENCH : ABILITY_NONE); Item(holder == player ? ITEM_NONE : item); if (holder == player) { Status1(STATUS1_FREEZE); MovesWithPP({MOVE_TACKLE, 5}); } }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponent ? 20 : 200); Gender(MON_MALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == opponent ? ABILITY_STEADFAST : stench ? ABILITY_STENCH : ABILITY_NONE); Item(holder == opponent ? ITEM_NONE : item); if (holder == opponent) { Status1(STATUS1_FREEZE); MovesWithPP({MOVE_TACKLE, 5}); } }
    } WHEN {
        TURN { MOVE(other, MOVE_TACKLE, WITH_RNG(stench ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, TRUE)); MOVE(holder, MOVE_TACKLE, WITH_RNG(RNG_FROZEN, NaturalThawRoll(TRUE))); }
        TURN { MOVE(other, MOVE_CELEBRATE); MOVE(holder, MOVE_TACKLE, WITH_RNG(RNG_FROZEN, NaturalThawRoll(thaw))); }
    } THEN {
        EXPECT_EQ(holder->status1, B_POLISHED_MOVE_BALANCE && !thaw ? STATUS1_FREEZE : STATUS1_NONE);
        EXPECT_EQ(holder->pp[0], B_POLISHED_MOVE_BALANCE && !thaw ? 5 : 4);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + 1);
    }
}

SINGLE_BATTLE_TEST("Polished frozen flinch can repeat without advancing the native freeze timer")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Item item = ITEM_KINGS_ROCK;
    bool32 stench = FALSE;
    for (u32 mode = 0; mode < 3; mode++)
        for (u32 side = 0; side < 2; side++)
                PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; stench = mode == 2; item = stench ? ITEM_NONE : mode == 1 ? ITEM_RAZOR_FANG : ITEM_KINGS_ROCK; }
    GIVEN {
        WITH_CONFIG(B_FREEZE_TURNS, GEN_CHAMPIONS);
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == player ? 20 : 200); Gender(MON_FEMALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == player ? ABILITY_STEADFAST : stench ? ABILITY_STENCH : ABILITY_NONE); Item(holder == player ? ITEM_NONE : item); if (holder == player) { Status1(STATUS1_FREEZE); MovesWithPP({MOVE_TACKLE, 5}); } }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponent ? 20 : 200); Gender(MON_MALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == opponent ? ABILITY_STEADFAST : stench ? ABILITY_STENCH : ABILITY_NONE); Item(holder == opponent ? ITEM_NONE : item); if (holder == opponent) { Status1(STATUS1_FREEZE); MovesWithPP({MOVE_TACKLE, 5}); } }
    } WHEN {
        for (u32 turn = 0; turn < 3; turn++)
            TURN { MOVE(other, MOVE_TACKLE, WITH_RNG(stench ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, TRUE)); MOVE(holder, MOVE_TACKLE, WITH_RNG(RNG_FROZEN, NaturalThawRoll(FALSE))); }
    } THEN {
        EXPECT_EQ(holder->status1, B_POLISHED_MOVE_BALANCE ? STATUS1_FREEZE : STATUS1_NONE);
        EXPECT_EQ(holder->pp[0], 5);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + (B_POLISHED_MOVE_BALANCE ? 3 : 1));
    }
}

SINGLE_BATTLE_TEST("Polished frozen flinch rechecks Inner Focus after a Mold Breaker item hit")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Item item = ITEM_KINGS_ROCK;
    bool32 moldBreaker = FALSE, thaw = FALSE;
    for (u32 rock = 0; rock < 2; rock++)
        for (u32 side = 0; side < 2; side++)
            for (u32 mold = 0; mold < 2; mold++)
                for (u32 roll = 0; roll < 2; roll++)
                    PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; item = rock ? ITEM_RAZOR_FANG : ITEM_KINGS_ROCK; moldBreaker = mold; thaw = roll; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == player ? 20 : 200); Gender(MON_FEMALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == player ? ABILITY_INNER_FOCUS : moldBreaker ? ABILITY_MOLD_BREAKER : ABILITY_NONE); Item(holder == player ? ITEM_NONE : item); if (holder == player) { Status1(STATUS1_FREEZE); MovesWithPP({MOVE_TACKLE, 5}); } }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponent ? 20 : 200); Gender(MON_MALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == opponent ? ABILITY_INNER_FOCUS : moldBreaker ? ABILITY_MOLD_BREAKER : ABILITY_NONE); Item(holder == opponent ? ITEM_NONE : item); if (holder == opponent) { Status1(STATUS1_FREEZE); MovesWithPP({MOVE_TACKLE, 5}); } }
    } WHEN {
        TURN { MOVE(other, MOVE_TACKLE, WITH_RNG(RNG_HOLD_EFFECT_FLINCH, TRUE)); MOVE(holder, MOVE_TACKLE, WITH_RNG(RNG_FROZEN, NaturalThawRoll(thaw))); }
    } THEN {
        EXPECT_EQ(holder->status1, thaw ? STATUS1_NONE : STATUS1_FREEZE);
        EXPECT_EQ(holder->pp[0], thaw && (B_POLISHED_MOVE_BALANCE || !moldBreaker) ? 4 : 5);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished frozen flinch does not bypass Shield Dust protection")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Item item = ITEM_KINGS_ROCK;
    bool32 stench = FALSE;
    bool32 thaw = FALSE;
    for (u32 mode = 0; mode < 3; mode++)
        for (u32 side = 0; side < 2; side++)
            for (u32 roll = 0; roll < 2; roll++)
                PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; stench = mode == 2; item = stench ? ITEM_NONE : mode == 1 ? ITEM_RAZOR_FANG : ITEM_KINGS_ROCK; thaw = roll; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == player ? 20 : 200); Gender(MON_FEMALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == player ? ABILITY_SHIELD_DUST : stench ? ABILITY_STENCH : ABILITY_NONE); Item(holder == player ? ITEM_NONE : item); if (holder == player) { Status1(STATUS1_FREEZE); MovesWithPP({MOVE_TACKLE, 5}); } }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponent ? 20 : 200); Gender(MON_MALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == opponent ? ABILITY_SHIELD_DUST : stench ? ABILITY_STENCH : ABILITY_NONE); Item(holder == opponent ? ITEM_NONE : item); if (holder == opponent) { Status1(STATUS1_FREEZE); MovesWithPP({MOVE_TACKLE, 5}); } }
    } WHEN {
        TURN { MOVE(other, MOVE_TACKLE, WITH_RNG(stench ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, TRUE)); MOVE(holder, MOVE_TACKLE, WITH_RNG(RNG_FROZEN, NaturalThawRoll(thaw))); }
    } THEN {
        EXPECT_EQ(holder->status1, thaw ? STATUS1_NONE : STATUS1_FREEZE);
        EXPECT_EQ(holder->pp[0], thaw ? 4 : 5);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished frozen flinch precedes freeze and later confusion or infatuation blockers")
{
    struct BattlePokemon *holder = player, *other = opponent;
    bool32 attract = FALSE;
    enum Item item = ITEM_KINGS_ROCK;
    bool32 stench = FALSE;
    for (u32 mode = 0; mode < 3; mode++)
        for (u32 side = 0; side < 2; side++)
            for (u32 block = 0; block < 2; block++)
                PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; stench = mode == 2; item = stench ? ITEM_NONE : mode == 1 ? ITEM_RAZOR_FANG : ITEM_KINGS_ROCK; attract = block; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == player ? 20 : 200); Gender(MON_FEMALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == player ? ABILITY_STEADFAST : stench ? ABILITY_STENCH : ABILITY_NONE); Item(holder == player ? ITEM_NONE : item); if (holder == player) { Status1(STATUS1_FREEZE); MovesWithPP({MOVE_TACKLE, 5}); } }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponent ? 20 : 200); Gender(MON_MALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == opponent ? ABILITY_STEADFAST : stench ? ABILITY_STENCH : ABILITY_NONE); Item(holder == opponent ? ITEM_NONE : item); if (holder == opponent) { Status1(STATUS1_FREEZE); MovesWithPP({MOVE_TACKLE, 5}); } }
    } WHEN {
        TURN { MOVE(other, attract ? MOVE_ATTRACT : MOVE_CONFUSE_RAY); MOVE(holder, MOVE_TACKLE, WITH_RNG(RNG_FROZEN, NaturalThawRoll(FALSE))); }
        TURN { MOVE(other, MOVE_TACKLE, WITH_RNG(stench ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, TRUE)); MOVE(holder, MOVE_TACKLE, WITH_RNG(RNG_FROZEN, NaturalThawRoll(TRUE))); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, other);
        NONE_OF { ANIMATION(ANIM_TYPE_STATUS, B_ANIM_STATUS_CONFUSION, holder); ANIMATION(ANIM_TYPE_STATUS, B_ANIM_STATUS_INFATUATION, holder); ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, holder); }
    } THEN {
        EXPECT_EQ(holder->status1, B_POLISHED_MOVE_BALANCE ? STATUS1_FREEZE : STATUS1_NONE);
        EXPECT_EQ(holder->pp[0], 5);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + 1);
    }
}

SINGLE_BATTLE_TEST("Polished frozen flinch preserves party freeze across switching out and back")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Item item = ITEM_KINGS_ROCK;
    bool32 stench = FALSE;
    for (u32 mode = 0; mode < 3; mode++)
        for (u32 side = 0; side < 2; side++)
                PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; stench = mode == 2; item = stench ? ITEM_NONE : mode == 1 ? ITEM_RAZOR_FANG : ITEM_KINGS_ROCK; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == player ? 20 : 200); Gender(MON_FEMALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == player ? ABILITY_STEADFAST : stench ? ABILITY_STENCH : ABILITY_NONE); Item(holder == player ? ITEM_NONE : item); if (holder == player) { Status1(STATUS1_FREEZE); MovesWithPP({MOVE_TACKLE, 5}); } }
        PLAYER(SPECIES_WYNAUT) { Speed(75); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponent ? 20 : 200); Gender(MON_MALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == opponent ? ABILITY_STEADFAST : stench ? ABILITY_STENCH : ABILITY_NONE); Item(holder == opponent ? ITEM_NONE : item); if (holder == opponent) { Status1(STATUS1_FREEZE); MovesWithPP({MOVE_TACKLE, 5}); } }
        OPPONENT(SPECIES_WYNAUT) { Speed(25); Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(other, MOVE_TACKLE, WITH_RNG(stench ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, TRUE)); MOVE(holder, MOVE_TACKLE, WITH_RNG(RNG_FROZEN, NaturalThawRoll(TRUE))); }
        TURN { SWITCH(holder, 1); MOVE(other, MOVE_CELEBRATE); }
        TURN { SWITCH(holder, 0); MOVE(other, MOVE_CELEBRATE); }
    } THEN {
        EXPECT_EQ(holder->status1, B_POLISHED_MOVE_BALANCE ? STATUS1_FREEZE : STATUS1_NONE);
        EXPECT_EQ(holder->pp[0], 5);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished frozen flinch also orders an optional Fake Out before the frozen action")
{
    struct BattlePokemon *holder = player, *other = opponent;
    bool32 thaw = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 roll = 0; roll < 2; roll++)
            PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; thaw = roll; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == player ? 20 : 200); Gender(MON_FEMALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == player ? ABILITY_STEADFAST : ABILITY_NONE); Item(holder == player ? ITEM_NONE : ITEM_NONE); if (holder == player) { Status1(STATUS1_FREEZE); MovesWithPP({MOVE_TACKLE, 5}); } }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponent ? 20 : 200); Gender(MON_MALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == opponent ? ABILITY_STEADFAST : ABILITY_NONE); Item(holder == opponent ? ITEM_NONE : ITEM_NONE); if (holder == opponent) { Status1(STATUS1_FREEZE); MovesWithPP({MOVE_TACKLE, 5}); } }
    } WHEN {
        TURN { MOVE(other, MOVE_FAKE_OUT); MOVE(holder, MOVE_TACKLE, WITH_RNG(RNG_FROZEN, NaturalThawRoll(thaw))); }
    } THEN {
        EXPECT_EQ(holder->status1, B_POLISHED_MOVE_BALANCE || !thaw ? STATUS1_FREEZE : STATUS1_NONE);
        EXPECT_EQ(holder->pp[0], 5);
        EXPECT_EQ(holder->statStages[STAT_SPEED], B_POLISHED_MOVE_BALANCE || thaw ? DEFAULT_STAT_STAGE + 1 : DEFAULT_STAT_STAGE);
    }
}

DOUBLE_BATTLE_TEST("Polished frozen flinch keeps doubles at the native stage")
{
    struct BattlePokemon *holder = playerLeft, *other = opponentLeft;
    enum Item item = ITEM_KINGS_ROCK;
    bool32 stench = FALSE;
    bool32 thaw = FALSE;
    for (u32 mode = 0; mode < 3; mode++)
        for (u32 side = 0; side < 2; side++)
            for (u32 roll = 0; roll < 2; roll++)
                PARAMETRIZE { holder = side ? opponentLeft : playerLeft; other = side ? playerLeft : opponentLeft; stench = mode == 2; item = stench ? ITEM_NONE : mode == 1 ? ITEM_RAZOR_FANG : ITEM_KINGS_ROCK; thaw = roll; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == playerLeft ? 20 : 200); Gender(MON_FEMALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == playerLeft ? ABILITY_STEADFAST : stench ? ABILITY_STENCH : ABILITY_NONE); Item(holder == playerLeft ? ITEM_NONE : item); if (holder == playerLeft) { Status1(STATUS1_FREEZE); MovesWithPP({MOVE_TACKLE, 5}); } }
        PLAYER(SPECIES_WYNAUT) { Speed(75); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponentLeft ? 20 : 200); Gender(MON_MALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == opponentLeft ? ABILITY_STEADFAST : stench ? ABILITY_STENCH : ABILITY_NONE); Item(holder == opponentLeft ? ITEM_NONE : item); if (holder == opponentLeft) { Status1(STATUS1_FREEZE); MovesWithPP({MOVE_TACKLE, 5}); } }
        OPPONENT(SPECIES_WYNAUT) { Speed(25); Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(other, MOVE_TACKLE, target: holder, WITH_RNG(stench ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, TRUE)); MOVE(holder, MOVE_TACKLE, target: other, WITH_RNG(RNG_FROZEN, thaw)); }
    } THEN {
        EXPECT_EQ(holder->status1, thaw ? STATUS1_NONE : STATUS1_FREEZE);
        EXPECT_EQ(holder->pp[0], 5);
        EXPECT_EQ(holder->statStages[STAT_SPEED], thaw ? DEFAULT_STAT_STAGE + 1 : DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished frozen flinch checks either active Terastal even for a self-targeting move")
{
    struct BattlePokemon *holder = player, *other = opponent;
    bool32 holderTera = FALSE;
    enum Item item = ITEM_KINGS_ROCK;
    bool32 stench = FALSE;
    bool32 thaw = FALSE;
    for (u32 mode = 0; mode < 3; mode++)
        for (u32 side = 0; side < 2; side++)
            for (u32 roll = 0; roll < 2; roll++)
                for (u32 tera = 0; tera < 2; tera++)
                    PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; stench = mode == 2; item = stench ? ITEM_NONE : mode == 1 ? ITEM_RAZOR_FANG : ITEM_KINGS_ROCK; thaw = roll; holderTera = tera; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == player ? 20 : 200); Gender(MON_FEMALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == player ? ABILITY_STEADFAST : stench ? ABILITY_STENCH : ABILITY_NONE); Item(holder == player ? ITEM_NONE : item); if (holder == player) { Status1(STATUS1_FREEZE); MovesWithPP({MOVE_CELEBRATE, 5}); } TeraType(TYPE_NORMAL); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponent ? 20 : 200); Gender(MON_MALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == opponent ? ABILITY_STEADFAST : stench ? ABILITY_STENCH : ABILITY_NONE); Item(holder == opponent ? ITEM_NONE : item); if (holder == opponent) { Status1(STATUS1_FREEZE); MovesWithPP({MOVE_CELEBRATE, 5}); } TeraType(TYPE_NORMAL); }
    } WHEN {
        TURN { MOVE(other, MOVE_TACKLE, gimmick: holderTera ? GIMMICK_NONE : GIMMICK_TERA, WITH_RNG(stench ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, TRUE)); MOVE(holder, MOVE_CELEBRATE, gimmick: holderTera ? GIMMICK_TERA : GIMMICK_NONE, WITH_RNG(RNG_FROZEN, thaw)); }
    } THEN {
        EXPECT_EQ(holder->status1, thaw ? STATUS1_NONE : STATUS1_FREEZE);
        EXPECT_EQ(holder->pp[0], 5);
        EXPECT_EQ(holder->statStages[STAT_SPEED], thaw ? DEFAULT_STAT_STAGE + 1 : DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished frozen flinch leaves sleeping targets at their existing stage")
{
    struct BattlePokemon *holder = player, *other = opponent;
    bool32 wake = FALSE;
    enum Move chosen = MOVE_SLEEP_TALK;
    enum Item item = ITEM_KINGS_ROCK;
    bool32 stench = FALSE;
    for (u32 mode = 0; mode < 3; mode++)
        for (u32 side = 0; side < 2; side++)
            for (u32 sleep = 0; sleep < 2; sleep++)
                PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; stench = mode == 2; item = stench ? ITEM_NONE : mode == 1 ? ITEM_RAZOR_FANG : ITEM_KINGS_ROCK; wake = sleep; chosen = wake ? MOVE_TACKLE : MOVE_SLEEP_TALK; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == player ? 20 : 200); Gender(MON_FEMALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == player ? ABILITY_STEADFAST : stench ? ABILITY_STENCH : ABILITY_NONE); Item(holder == player ? ITEM_NONE : item); if (holder == player) { Status1(STATUS1_SLEEP_TURN(wake ? 1 : 3)); MovesWithPP({chosen, 5}, {MOVE_CELEBRATE, 5}); } }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponent ? 20 : 200); Gender(MON_MALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == opponent ? ABILITY_STEADFAST : stench ? ABILITY_STENCH : ABILITY_NONE); Item(holder == opponent ? ITEM_NONE : item); if (holder == opponent) { Status1(STATUS1_SLEEP_TURN(wake ? 1 : 3)); MovesWithPP({chosen, 5}, {MOVE_CELEBRATE, 5}); } }
    } WHEN {
        TURN { MOVE(other, MOVE_TACKLE, WITH_RNG(stench ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, TRUE)); MOVE(holder, chosen); }
    } THEN {
        EXPECT_EQ(holder->status1, wake ? STATUS1_NONE : STATUS1_SLEEP_TURN(2));
        EXPECT_EQ(holder->pp[0], 5);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + 1);
    }
}

SINGLE_BATTLE_TEST("Polished frozen flinch allows all source self-thaw moves after Inner Focus discards a pending flinch")
{
    struct BattlePokemon *holder = player, *other = opponent;
    static const enum Move moves[] = {MOVE_SACRED_FIRE, MOVE_SCALD, MOVE_FLARE_BLITZ};
    enum Move move = MOVE_SCALD;
    enum Item item = ITEM_KINGS_ROCK;
    for (u32 m = 0; m < ARRAY_COUNT(moves); m++)
        for (u32 rock = 0; rock < 2; rock++)
            for (u32 side = 0; side < 2; side++)
                PARAMETRIZE { move = moves[m]; item = rock ? ITEM_RAZOR_FANG : ITEM_KINGS_ROCK; holder = side ? opponent : player; other = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == player ? 20 : 200); Gender(MON_FEMALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == player ? ABILITY_INNER_FOCUS : ABILITY_MOLD_BREAKER); Item(holder == player ? ITEM_NONE : item); if (holder == player) { Status1(STATUS1_FREEZE); MovesWithPP({move, 5}); } }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponent ? 20 : 200); Gender(MON_MALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == opponent ? ABILITY_INNER_FOCUS : ABILITY_MOLD_BREAKER); Item(holder == opponent ? ITEM_NONE : item); if (holder == opponent) { Status1(STATUS1_FREEZE); MovesWithPP({move, 5}); } }
    } WHEN {
        TURN { MOVE(other, MOVE_TACKLE, WITH_RNG(RNG_HOLD_EFFECT_FLINCH, TRUE)); MOVE(holder, move, hit: TRUE, secondaryEffect: FALSE); }
    } THEN {
        EXPECT_EQ(holder->status1, B_POLISHED_MOVE_BALANCE ? STATUS1_NONE : STATUS1_FREEZE);
        EXPECT_EQ(holder->pp[0], B_POLISHED_MOVE_BALANCE ? 4 : 5);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished frozen flinch respects Neutralizing Gas suppression of Inner Focus and Steadfast")
{
    struct BattlePokemon *holder = player, *other = opponent;
    bool32 thaw = FALSE;
    enum Ability ability = ABILITY_INNER_FOCUS;
    for (u32 a = 0; a < 2; a++)
        for (u32 side = 0; side < 2; side++)
            for (u32 roll = 0; roll < 2; roll++)
                PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; ability = a ? ABILITY_STEADFAST : ABILITY_INNER_FOCUS; thaw = roll; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == player ? 20 : 200); Gender(MON_FEMALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == player ? ability : ABILITY_NEUTRALIZING_GAS); Item(holder == player ? ITEM_NONE : ITEM_KINGS_ROCK); if (holder == player) { Status1(STATUS1_FREEZE); MovesWithPP({MOVE_TACKLE, 5}); } }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponent ? 20 : 200); Gender(MON_MALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == opponent ? ability : ABILITY_NEUTRALIZING_GAS); Item(holder == opponent ? ITEM_NONE : ITEM_KINGS_ROCK); if (holder == opponent) { Status1(STATUS1_FREEZE); MovesWithPP({MOVE_TACKLE, 5}); } }
    } WHEN {
        TURN { MOVE(other, MOVE_TACKLE, WITH_RNG(RNG_HOLD_EFFECT_FLINCH, TRUE)); MOVE(holder, MOVE_TACKLE, WITH_RNG(RNG_FROZEN, NaturalThawRoll(thaw))); }
    } THEN {
        EXPECT_EQ(holder->status1, B_POLISHED_MOVE_BALANCE || !thaw ? STATUS1_FREEZE : STATUS1_NONE);
        EXPECT_EQ(holder->pp[0], 5);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE);
    }
}
