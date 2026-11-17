#include "global.h"
#include "test/battle.h"

static const enum Move sFlinchMoves[] = {
    MOVE_AIR_SLASH, MOVE_STOMP, MOVE_HEADBUTT, MOVE_BITE, MOVE_WATERFALL,
    MOVE_ICICLE_CRASH, MOVE_IRON_HEAD, MOVE_EXTRASENSORY, MOVE_ROCK_SLIDE,
    MOVE_HYPER_FANG, MOVE_ZEN_HEADBUTT, MOVE_ASTONISH, MOVE_DARK_PULSE,
};

static u32 NaturalThawRoll(bool32 thaw)
{
    return B_POLISHED_MOVE_BALANCE ? (thaw ? 0 : 255) : thaw;
}

SINGLE_BATTLE_TEST("Polished flinch-hit does not stop a frozen target after natural thaw")
{
    struct BattlePokemon *holder = player, *other = opponent;
    bool32 thaw = FALSE;
    enum Move move = MOVE_BITE;
    for (u32 m = 0; m < ARRAY_COUNT(sFlinchMoves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 roll = 0; roll < 2; roll++)
                PARAMETRIZE { move = sFlinchMoves[m]; holder = side ? opponent : player; other = side ? player : opponent; thaw = roll; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == player ? 50 : 100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == player ? ABILITY_STEADFAST : ABILITY_NONE); if (holder == player) { Status1(STATUS1_FREEZE); MovesWithPP({MOVE_TACKLE, 5}); } }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponent ? 50 : 100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == opponent ? ABILITY_STEADFAST : ABILITY_NONE); if (holder == opponent) { Status1(STATUS1_FREEZE); MovesWithPP({MOVE_TACKLE, 5}); } }
    } WHEN {
        TURN { MOVE(other, move, hit: TRUE, secondaryEffect: TRUE); MOVE(holder, MOVE_TACKLE, WITH_RNG(RNG_FROZEN, NaturalThawRoll(thaw))); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, other);
        if (thaw) STATUS_ICON(holder, none: TRUE);
        if (B_POLISHED_MOVE_BALANCE && thaw) ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, holder);
        else NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, holder);
    } THEN {
        EXPECT_EQ(holder->status1, thaw ? STATUS1_NONE : STATUS1_FREEZE);
        EXPECT_EQ(holder->pp[0], B_POLISHED_MOVE_BALANCE && thaw ? 4 : 5);
        EXPECT_EQ(holder->statStages[STAT_SPEED], !B_POLISHED_MOVE_BALANCE && thaw ? DEFAULT_STAT_STAGE + 1 : DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished flinch-hit does not block any source self-thaw move")
{
    struct BattlePokemon *holder = player, *other = opponent;
    static const enum Move thawMoves[] = {MOVE_SACRED_FIRE, MOVE_SCALD, MOVE_FLARE_BLITZ};
    enum Move thawMove = MOVE_SCALD;
    enum Move move = MOVE_BITE;
    for (u32 m = 0; m < ARRAY_COUNT(sFlinchMoves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 t = 0; t < ARRAY_COUNT(thawMoves); t++)
                PARAMETRIZE { move = sFlinchMoves[m]; holder = side ? opponent : player; other = side ? player : opponent; thawMove = thawMoves[t]; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == player ? 50 : 100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == player ? ABILITY_STEADFAST : ABILITY_NONE); if (holder == player) { Status1(STATUS1_FREEZE); MovesWithPP({thawMove, 5}); } }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponent ? 50 : 100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == opponent ? ABILITY_STEADFAST : ABILITY_NONE); if (holder == opponent) { Status1(STATUS1_FREEZE); MovesWithPP({thawMove, 5}); } }
    } WHEN {
        TURN { MOVE(other, move, hit: TRUE, secondaryEffect: TRUE); MOVE(holder, thawMove, hit: TRUE, secondaryEffect: FALSE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, other);
        if (B_POLISHED_MOVE_BALANCE) {
            STATUS_ICON(holder, none: TRUE);
            ANIMATION(ANIM_TYPE_MOVE, thawMove, holder);
        } else NONE_OF { STATUS_ICON(holder, none: TRUE); ANIMATION(ANIM_TYPE_MOVE, thawMove, holder); }
    } THEN {
        EXPECT_EQ(holder->status1, B_POLISHED_MOVE_BALANCE ? STATUS1_NONE : STATUS1_FREEZE);
        EXPECT_EQ(holder->pp[0], B_POLISHED_MOVE_BALANCE ? 4 : 5);
        EXPECT_EQ(holder->statStages[STAT_SPEED], B_POLISHED_MOVE_BALANCE ? DEFAULT_STAT_STAGE : DEFAULT_STAT_STAGE + 1);
    }
}

SINGLE_BATTLE_TEST("Polished flinch-hit checks sleep before the target wakes on its action")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Move move = MOVE_BITE;
    for (u32 m = 0; m < ARRAY_COUNT(sFlinchMoves); m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = sFlinchMoves[m]; holder = side ? opponent : player; other = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == player ? 50 : 100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == player ? ABILITY_STEADFAST : ABILITY_NONE); if (holder == player) { Status1(STATUS1_SLEEP_TURN(1)); MovesWithPP({MOVE_TACKLE, 5}); } }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponent ? 50 : 100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == opponent ? ABILITY_STEADFAST : ABILITY_NONE); if (holder == opponent) { Status1(STATUS1_SLEEP_TURN(1)); MovesWithPP({MOVE_TACKLE, 5}); } }
    } WHEN {
        TURN { MOVE(other, move, hit: TRUE, secondaryEffect: TRUE); MOVE(holder, MOVE_TACKLE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, other);
        STATUS_ICON(holder, none: TRUE);
        if (B_POLISHED_MOVE_BALANCE) ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, holder);
        else NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, holder);
    } THEN {
        EXPECT_EQ(holder->status1, STATUS1_NONE);
        EXPECT_EQ(holder->pp[0], B_POLISHED_MOVE_BALANCE ? 4 : 5);
        EXPECT_EQ(holder->statStages[STAT_SPEED], B_POLISHED_MOVE_BALANCE ? DEFAULT_STAT_STAGE : DEFAULT_STAT_STAGE + 1);
    }
}

SINGLE_BATTLE_TEST("Polished flinch-hit permits Sleep Talk while the target remains asleep")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Move move = MOVE_BITE;
    for (u32 m = 0; m < ARRAY_COUNT(sFlinchMoves); m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = sFlinchMoves[m]; holder = side ? opponent : player; other = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == player ? 50 : 100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == player ? ABILITY_STEADFAST : ABILITY_NONE); if (holder == player) { Status1(STATUS1_SLEEP_TURN(3)); MovesWithPP({MOVE_SLEEP_TALK, 5}, {MOVE_TACKLE, 5}); } }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponent ? 50 : 100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == opponent ? ABILITY_STEADFAST : ABILITY_NONE); if (holder == opponent) { Status1(STATUS1_SLEEP_TURN(3)); MovesWithPP({MOVE_SLEEP_TALK, 5}, {MOVE_TACKLE, 5}); } }
    } WHEN {
        TURN { MOVE(other, move, hit: TRUE, secondaryEffect: TRUE); MOVE(holder, MOVE_SLEEP_TALK); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, other);
        if (B_POLISHED_MOVE_BALANCE) {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_SLEEP_TALK, holder);
            ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, holder);
        } else NONE_OF { ANIMATION(ANIM_TYPE_MOVE, MOVE_SLEEP_TALK, holder); ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, holder); }
    } THEN {
        EXPECT_EQ(holder->status1, STATUS1_SLEEP_TURN(2));
        EXPECT_EQ(holder->pp[0], B_POLISHED_MOVE_BALANCE ? 4 : 5);
        EXPECT_EQ(holder->pp[1], 5);
        EXPECT_EQ(holder->statStages[STAT_SPEED], B_POLISHED_MOVE_BALANCE ? DEFAULT_STAT_STAGE : DEFAULT_STAT_STAGE + 1);
    }
}

SINGLE_BATTLE_TEST("Polished flinch-hit still affects healthy targets and respects Inner Focus and Steadfast")
{
    struct BattlePokemon *holder = player, *other = opponent;
    static const enum Ability abilities[] = {ABILITY_NONE, ABILITY_INNER_FOCUS, ABILITY_STEADFAST};
    enum Ability ability = ABILITY_NONE;
    enum Move move = MOVE_BITE;
    for (u32 m = 0; m < ARRAY_COUNT(sFlinchMoves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 a = 0; a < ARRAY_COUNT(abilities); a++)
                PARAMETRIZE { move = sFlinchMoves[m]; holder = side ? opponent : player; other = side ? player : opponent; ability = abilities[a]; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == player ? 50 : 100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == player ? ability : ABILITY_NONE); if (holder == player) { Status1(STATUS1_NONE); MovesWithPP({MOVE_TACKLE, 5}); } }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponent ? 50 : 100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == opponent ? ability : ABILITY_NONE); if (holder == opponent) { Status1(STATUS1_NONE); MovesWithPP({MOVE_TACKLE, 5}); } }
    } WHEN {
        TURN { MOVE(other, move, hit: TRUE, secondaryEffect: TRUE); MOVE(holder, MOVE_TACKLE); }
    } THEN {
        EXPECT_EQ(holder->status1, STATUS1_NONE);
        EXPECT_EQ(holder->pp[0], ability == ABILITY_INNER_FOCUS ? 4 : 5);
        EXPECT_EQ(holder->statStages[STAT_SPEED], ability == ABILITY_STEADFAST ? DEFAULT_STAT_STAGE + 1 : DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished flinch-hit does not exempt poison burn paralysis or Frostbite")
{
    struct BattlePokemon *holder = player, *other = opponent;
    static const u32 statuses[] = {STATUS1_POISON, STATUS1_TOXIC_POISON, STATUS1_BURN, STATUS1_PARALYSIS, STATUS1_FROSTBITE};
    u32 status = STATUS1_POISON;
    for (u32 s = 0; s < ARRAY_COUNT(statuses); s++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { status = statuses[s]; holder = side ? opponent : player; other = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == player ? 50 : 100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == player ? ABILITY_STEADFAST : ABILITY_NONE); if (holder == player) { Status1(status); MovesWithPP({MOVE_TACKLE, 5}); } }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponent ? 50 : 100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == opponent ? ABILITY_STEADFAST : ABILITY_NONE); if (holder == opponent) { Status1(status); MovesWithPP({MOVE_TACKLE, 5}); } }
    } WHEN {
        TURN { MOVE(other, MOVE_BITE, secondaryEffect: TRUE); MOVE(holder, MOVE_TACKLE, WITH_RNG(RNG_PARALYSIS, FALSE)); }
    } THEN {
        EXPECT_EQ(holder->status1 & status, status);
        EXPECT_EQ(holder->pp[0], 5);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + 1);
    }
}

SINGLE_BATTLE_TEST("Polished flinch-hit keeps Inner Focus and Shield Dust protections")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Ability ability = ABILITY_INNER_FOCUS;
    bool32 asleep = FALSE;
    enum Move chosen = MOVE_SCALD;
    for (u32 side = 0; side < 2; side++)
        for (u32 s = 0; s < 2; s++)
            for (u32 a = 0; a < 2; a++)
                PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; asleep = s; chosen = asleep ? MOVE_SLEEP_TALK : MOVE_SCALD; ability = a ? ABILITY_SHIELD_DUST : ABILITY_INNER_FOCUS; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == player ? 50 : 100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == player ? ability : ABILITY_NONE); if (holder == player) { Status1(asleep ? STATUS1_SLEEP_TURN(3) : STATUS1_FREEZE); MovesWithPP({chosen, 5}, {MOVE_TACKLE, 5}); } }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponent ? 50 : 100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == opponent ? ability : ABILITY_NONE); if (holder == opponent) { Status1(asleep ? STATUS1_SLEEP_TURN(3) : STATUS1_FREEZE); MovesWithPP({chosen, 5}, {MOVE_TACKLE, 5}); } }
    } WHEN {
        TURN { MOVE(other, MOVE_BITE, secondaryEffect: TRUE); MOVE(holder, chosen, secondaryEffect: FALSE); }
    } THEN {
        EXPECT_EQ(holder->status1, asleep ? STATUS1_SLEEP_TURN(2) : STATUS1_NONE);
        EXPECT_EQ(holder->pp[0], 4);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished flinch-hit leaves optional Fake Out on its native path")
{
    struct BattlePokemon *holder = player, *other = opponent;
    bool32 asleep = FALSE;
    enum Move chosen = MOVE_SCALD;
    for (u32 side = 0; side < 2; side++)
        for (u32 s = 0; s < 2; s++)
            PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; asleep = s; chosen = asleep ? MOVE_SLEEP_TALK : MOVE_SCALD; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == player ? 50 : 100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == player ? ABILITY_STEADFAST : ABILITY_NONE); if (holder == player) { Status1(asleep ? STATUS1_SLEEP_TURN(3) : STATUS1_FREEZE); MovesWithPP({chosen, 5}, {MOVE_TACKLE, 5}); } }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponent ? 50 : 100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == opponent ? ABILITY_STEADFAST : ABILITY_NONE); if (holder == opponent) { Status1(asleep ? STATUS1_SLEEP_TURN(3) : STATUS1_FREEZE); MovesWithPP({chosen, 5}, {MOVE_TACKLE, 5}); } }
    } WHEN {
        TURN { MOVE(other, MOVE_FAKE_OUT); MOVE(holder, chosen); }
    } THEN {
        EXPECT_EQ(holder->status1, asleep ? STATUS1_SLEEP_TURN(2) : STATUS1_FREEZE);
        EXPECT_EQ(holder->pp[0], 5);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + 1);
    }
}

DOUBLE_BATTLE_TEST("Polished flinch-hit leaves doubles on their native path")
{
    struct BattlePokemon *holder = playerLeft, *other = opponentLeft;
    bool32 asleep = FALSE;
    enum Move chosen = MOVE_SCALD;
    for (u32 side = 0; side < 2; side++)
        for (u32 s = 0; s < 2; s++)
            PARAMETRIZE { holder = side ? opponentLeft : playerLeft; other = side ? playerLeft : opponentLeft; asleep = s; chosen = asleep ? MOVE_SLEEP_TALK : MOVE_SCALD; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == playerLeft ? 50 : 100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == playerLeft ? ABILITY_STEADFAST : ABILITY_NONE); if (holder == playerLeft) { Status1(asleep ? STATUS1_SLEEP_TURN(3) : STATUS1_FREEZE); MovesWithPP({chosen, 5}, {MOVE_TACKLE, 5}); } }
        PLAYER(SPECIES_WYNAUT) { Speed(25); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponentLeft ? 50 : 100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == opponentLeft ? ABILITY_STEADFAST : ABILITY_NONE); if (holder == opponentLeft) { Status1(asleep ? STATUS1_SLEEP_TURN(3) : STATUS1_FREEZE); MovesWithPP({chosen, 5}, {MOVE_TACKLE, 5}); } }
        OPPONENT(SPECIES_WYNAUT) { Speed(20); Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(other, MOVE_BITE, target: holder, secondaryEffect: TRUE); MOVE(holder, chosen, target: other); }
    } THEN {
        EXPECT_EQ(holder->status1, asleep ? STATUS1_SLEEP_TURN(2) : STATUS1_FREEZE);
        EXPECT_EQ(holder->pp[0], 5);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + 1);
    }
}

SINGLE_BATTLE_TEST("Polished flinch-hit leaves either active Terastal on its native path")
{
    struct BattlePokemon *holder = player, *other = opponent;
    bool32 holderTera = FALSE;
    bool32 asleep = FALSE;
    enum Move chosen = MOVE_SCALD;
    for (u32 side = 0; side < 2; side++)
        for (u32 s = 0; s < 2; s++)
            for (u32 t = 0; t < 2; t++)
                PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; asleep = s; chosen = asleep ? MOVE_SLEEP_TALK : MOVE_SCALD; holderTera = t; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == player ? 50 : 100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == player ? ABILITY_STEADFAST : ABILITY_NONE); if (holder == player) { Status1(asleep ? STATUS1_SLEEP_TURN(3) : STATUS1_FREEZE); MovesWithPP({chosen, 5}, {MOVE_TACKLE, 5}); } TeraType(TYPE_NORMAL); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponent ? 50 : 100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == opponent ? ABILITY_STEADFAST : ABILITY_NONE); if (holder == opponent) { Status1(asleep ? STATUS1_SLEEP_TURN(3) : STATUS1_FREEZE); MovesWithPP({chosen, 5}, {MOVE_TACKLE, 5}); } TeraType(TYPE_NORMAL); }
    } WHEN {
        TURN { MOVE(other, MOVE_BITE, secondaryEffect: TRUE, gimmick: holderTera ? GIMMICK_NONE : GIMMICK_TERA); MOVE(holder, chosen, gimmick: holderTera ? GIMMICK_TERA : GIMMICK_NONE); }
    } THEN {
        EXPECT_EQ(holder->status1, asleep ? STATUS1_SLEEP_TURN(2) : STATUS1_FREEZE);
        EXPECT_EQ(holder->pp[0], 5);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + 1);
    }
}

SINGLE_BATTLE_TEST("Polished flinch-hit keeps item and Stench flinches separate from move-effect immunity")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Item item = ITEM_KINGS_ROCK;
    bool32 stench = FALSE;
    bool32 asleep = FALSE;
    enum Move chosen = MOVE_SCALD;
    for (u32 side = 0; side < 2; side++)
        for (u32 s = 0; s < 2; s++)
            for (u32 mode = 0; mode < 3; mode++)
                PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; asleep = s; chosen = asleep ? MOVE_SLEEP_TALK : MOVE_SCALD; stench = mode == 2; item = stench ? ITEM_NONE : mode == 1 ? ITEM_RAZOR_FANG : ITEM_KINGS_ROCK; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == player ? 50 : 100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == player ? ABILITY_STEADFAST : stench ? ABILITY_STENCH : ABILITY_NONE); Item(holder == player ? ITEM_NONE : item); if (holder == player) { Status1(asleep ? STATUS1_SLEEP_TURN(3) : STATUS1_FREEZE); MovesWithPP({chosen, 5}, {MOVE_TACKLE, 5}); } }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponent ? 50 : 100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == opponent ? ABILITY_STEADFAST : stench ? ABILITY_STENCH : ABILITY_NONE); Item(holder == opponent ? ITEM_NONE : item); if (holder == opponent) { Status1(asleep ? STATUS1_SLEEP_TURN(3) : STATUS1_FREEZE); MovesWithPP({chosen, 5}, {MOVE_TACKLE, 5}); } }
    } WHEN {
        TURN { MOVE(other, MOVE_TACKLE, WITH_RNG(stench ? RNG_STENCH : RNG_HOLD_EFFECT_FLINCH, TRUE)); MOVE(holder, chosen); }
    } THEN {
        EXPECT_EQ(holder->status1, asleep ? STATUS1_SLEEP_TURN(2) : STATUS1_FREEZE);
        EXPECT_EQ(holder->pp[0], 5);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + 1);
    }
}

SINGLE_BATTLE_TEST("Polished flinch-hit does not carry sleep immunity into the next healthy turn")
{
    struct BattlePokemon *holder = player, *other = opponent;
    PARAMETRIZE { holder = player; other = opponent; }
    PARAMETRIZE { holder = opponent; other = player; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == player ? 50 : 100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == player ? ABILITY_STEADFAST : ABILITY_NONE); if (holder == player) { Status1(STATUS1_SLEEP_TURN(1)); MovesWithPP({MOVE_TACKLE, 5}); } }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponent ? 50 : 100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == opponent ? ABILITY_STEADFAST : ABILITY_NONE); if (holder == opponent) { Status1(STATUS1_SLEEP_TURN(1)); MovesWithPP({MOVE_TACKLE, 5}); } }
    } WHEN {
        TURN { MOVE(other, MOVE_BITE, secondaryEffect: TRUE); MOVE(holder, MOVE_TACKLE); }
        TURN { MOVE(other, MOVE_BITE, secondaryEffect: TRUE); MOVE(holder, MOVE_TACKLE); }
    } THEN {
        EXPECT_EQ(holder->status1, STATUS1_NONE);
        EXPECT_EQ(holder->pp[0], B_POLISHED_MOVE_BALANCE ? 4 : 5);
        EXPECT_EQ(holder->statStages[STAT_SPEED], B_POLISHED_MOVE_BALANCE ? DEFAULT_STAT_STAGE + 1 : DEFAULT_STAT_STAGE + 2);
    }
}
