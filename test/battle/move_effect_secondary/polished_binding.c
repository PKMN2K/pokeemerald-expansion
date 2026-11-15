#include "global.h"
#include "test/battle.h"

static const enum Move sSourceBindingMoves[] = {MOVE_WRAP, MOVE_FIRE_SPIN, MOVE_WHIRLPOOL};

SINGLE_BATTLE_TEST("Polished binding releases on the last countdown turn without residual damage")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_WRAP;
    u32 duration = 4;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < ARRAY_COUNT(sSourceBindingMoves); kind++)
            for (u32 length = 0; length < 3; length++)
                PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; move = sSourceBindingMoves[kind]; duration = length == 2 ? 7 : 4 + length; }
    GIVEN {
        WITH_CONFIG(B_BINDING_TURNS, GEN_5);
        WITH_CONFIG(B_BINDING_DAMAGE, GEN_6);
        PLAYER(SPECIES_RATTATA) { Ability(ABILITY_KEEN_EYE); HP(720); MaxHP(720); Item(user == player && duration == 7 ? ITEM_GRIP_CLAW : ITEM_NONE); }
        OPPONENT(SPECIES_RATTATA) { Ability(ABILITY_KEEN_EYE); HP(720); MaxHP(720); Item(user == opponent && duration == 7 ? ITEM_GRIP_CLAW : ITEM_NONE); }
    } WHEN {
        TURN { MOVE(user, move, hit: TRUE, WITH_RNG(RNG_WRAP, duration == 7 ? 4 : duration)); }
        for (u32 turn = 1; turn < duration; turn++) TURN {}
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, user);
        HP_BAR(target);
        for (u32 tick = 0; tick < duration - B_POLISHED_MOVE_BALANCE; tick++) HP_BAR(target, damage: 90);
        NOT HP_BAR(target);
    } THEN {
        EXPECT_EQ((u32)target->volatiles.wrapped, !B_POLISHED_MOVE_BALANCE);
        EXPECT_EQ((u32)target->volatiles.wrapTurns, 0);
    }
}

SINGLE_BATTLE_TEST("Polished binding pauses an Infiltrator bind behind Substitute")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_WRAP;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < ARRAY_COUNT(sSourceBindingMoves); kind++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; move = sSourceBindingMoves[kind]; }
    GIVEN {
        WITH_CONFIG(B_BINDING_TURNS, GEN_5);
        WITH_CONFIG(B_BINDING_DAMAGE, GEN_6);
        PLAYER(SPECIES_RATTATA) { Ability(user == player ? ABILITY_INFILTRATOR : ABILITY_KEEN_EYE); HP(720); MaxHP(720); Speed(target == player ? 100 : 50); }
        OPPONENT(SPECIES_RATTATA) { Ability(user == opponent ? ABILITY_INFILTRATOR : ABILITY_KEEN_EYE); HP(720); MaxHP(720); Speed(target == opponent ? 100 : 50); }
    } WHEN {
        TURN { MOVE(target, MOVE_SUBSTITUTE); MOVE(user, move, hit: TRUE, WITH_RNG(RNG_WRAP, 4)); }
        for (u32 turn = 0; turn < 5; turn++) TURN {}
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SUBSTITUTE, target);
        HP_BAR(target, damage: 180);
        ANIMATION(ANIM_TYPE_MOVE, move, user);
        HP_BAR(target);
        if (!B_POLISHED_MOVE_BALANCE)
            for (u32 tick = 0; tick < 4; tick++) HP_BAR(target, damage: 90);
        NOT HP_BAR(target);
    } THEN {
        EXPECT_EQ((u32)target->volatiles.wrapped, B_POLISHED_MOVE_BALANCE);
        EXPECT_EQ((u32)target->volatiles.wrapTurns, B_POLISHED_MOVE_BALANCE ? 3 : 0);
        EXPECT_EQ((u32)target->volatiles.substitute, TRUE);
    }
}

SINGLE_BATTLE_TEST("Polished binding resumes on the turn Substitute breaks")
{
    struct BattlePokemon *user = player, *target = opponent;
    PARAMETRIZE { user = player; target = opponent; }
    PARAMETRIZE { user = opponent; target = player; }
    GIVEN {
        WITH_CONFIG(B_BINDING_TURNS, GEN_5);
        WITH_CONFIG(B_BINDING_DAMAGE, GEN_6);
        PLAYER(SPECIES_RATTATA) { Ability(user == player ? ABILITY_INFILTRATOR : ABILITY_KEEN_EYE); Level(100); Attack(20); Defense(1000); HP(240); MaxHP(240); Speed(target == player ? 100 : 50); }
        OPPONENT(SPECIES_RATTATA) { Ability(user == opponent ? ABILITY_INFILTRATOR : ABILITY_KEEN_EYE); Level(100); Attack(20); Defense(1000); HP(240); MaxHP(240); Speed(target == opponent ? 100 : 50); }
    } WHEN {
        TURN { MOVE(target, MOVE_SUBSTITUTE); MOVE(user, MOVE_WRAP, hit: TRUE, WITH_RNG(RNG_WRAP, 4)); }
        TURN { MOVE(target, MOVE_SKILL_SWAP); }
        TURN { MOVE(user, MOVE_SEISMIC_TOSS); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SUBSTITUTE, target);
        HP_BAR(target, damage: 60);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_WRAP, user);
        HP_BAR(target);
        if (!B_POLISHED_MOVE_BALANCE) HP_BAR(target, damage: 30);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SKILL_SWAP, target);
        if (!B_POLISHED_MOVE_BALANCE) HP_BAR(target, damage: 30);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SEISMIC_TOSS, user);
        HP_BAR(target, damage: 30);
        NOT HP_BAR(target);
    } THEN {
        EXPECT_EQ((u32)target->volatiles.substitute, FALSE);
        EXPECT_EQ((u32)target->volatiles.wrapped, TRUE);
        EXPECT_EQ((u32)target->volatiles.wrapTurns, B_POLISHED_MOVE_BALANCE ? 2 : 1);
    }
}

SINGLE_BATTLE_TEST("Polished binding pauses the countdown while Magic Guard prevents damage")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_WRAP;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < ARRAY_COUNT(sSourceBindingMoves); kind++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; move = sSourceBindingMoves[kind]; }
    GIVEN {
        WITH_CONFIG(B_BINDING_TURNS, GEN_5);
        PLAYER(SPECIES_RATTATA) { Ability(target == player ? ABILITY_MAGIC_GUARD : ABILITY_KEEN_EYE); HP(720); MaxHP(720); }
        OPPONENT(SPECIES_RATTATA) { Ability(target == opponent ? ABILITY_MAGIC_GUARD : ABILITY_KEEN_EYE); HP(720); MaxHP(720); }
    } WHEN {
        TURN { MOVE(user, move, hit: TRUE, WITH_RNG(RNG_WRAP, 4)); }
        for (u32 turn = 0; turn < 5; turn++) TURN {}
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, user);
        HP_BAR(target);
        NOT HP_BAR(target);
    } THEN {
        EXPECT_EQ((u32)target->volatiles.wrapped, B_POLISHED_MOVE_BALANCE);
        EXPECT_EQ((u32)target->volatiles.wrapTurns, B_POLISHED_MOVE_BALANCE ? 3 : 0);
    }
}

SINGLE_BATTLE_TEST("Polished binding resumes when Magic Guard is suppressed")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_WRAP;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < ARRAY_COUNT(sSourceBindingMoves); kind++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; move = sSourceBindingMoves[kind]; }
    GIVEN {
        WITH_CONFIG(B_BINDING_TURNS, GEN_5);
        WITH_CONFIG(B_BINDING_DAMAGE, GEN_6);
        PLAYER(SPECIES_RATTATA) { Ability(target == player ? ABILITY_MAGIC_GUARD : ABILITY_KEEN_EYE); HP(720); MaxHP(720); }
        OPPONENT(SPECIES_RATTATA) { Ability(target == opponent ? ABILITY_MAGIC_GUARD : ABILITY_KEEN_EYE); HP(720); MaxHP(720); }
    } WHEN {
        TURN { MOVE(user, move, hit: TRUE, WITH_RNG(RNG_WRAP, 4)); }
        for (u32 turn = 0; turn < 4; turn++) TURN {}
        TURN { MOVE(user, MOVE_GASTRO_ACID); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, user);
        HP_BAR(target);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_GASTRO_ACID, user);
        if (B_POLISHED_MOVE_BALANCE) HP_BAR(target, damage: 90);
        NOT HP_BAR(target);
    } THEN {
        EXPECT_EQ((u32)target->volatiles.wrapped, B_POLISHED_MOVE_BALANCE);
        EXPECT_EQ((u32)target->volatiles.wrapTurns, B_POLISHED_MOVE_BALANCE ? 2 : 0);
    }
}

SINGLE_BATTLE_TEST("Polished binding checks the live Binding Band including under Unnerve")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_WRAP;
    bool32 startsWithBand = TRUE, unnerve = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < ARRAY_COUNT(sSourceBindingMoves); kind++)
            for (u32 band = 0; band < 2; band++)
                for (u32 nerve = 0; nerve < 2; nerve++)
                    PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; move = sSourceBindingMoves[kind]; startsWithBand = band; unnerve = nerve; }
    GIVEN {
        WITH_CONFIG(B_BINDING_TURNS, GEN_5);
        WITH_CONFIG(B_BINDING_DAMAGE, GEN_6);
        PLAYER(SPECIES_RATTATA) { Ability(target == player && unnerve ? ABILITY_UNNERVE : ABILITY_KEEN_EYE); HP(720); MaxHP(720); Item((user == player) == startsWithBand ? ITEM_BINDING_BAND : ITEM_NONE); }
        OPPONENT(SPECIES_RATTATA) { Ability(target == opponent && unnerve ? ABILITY_UNNERVE : ABILITY_KEEN_EYE); HP(720); MaxHP(720); Item((user == opponent) == startsWithBand ? ITEM_BINDING_BAND : ITEM_NONE); }
    } WHEN {
        TURN { MOVE(user, move, hit: TRUE, WITH_RNG(RNG_WRAP, 5)); }
        TURN { MOVE(target, MOVE_TRICK); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, user);
        HP_BAR(target);
        HP_BAR(target, damage: startsWithBand ? 120 : 90);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TRICK, target);
        HP_BAR(target, damage: (B_POLISHED_MOVE_BALANCE ? !startsWithBand : startsWithBand) ? 120 : 90);
    } THEN {
        EXPECT_EQ(user->item, startsWithBand ? ITEM_NONE : ITEM_BINDING_BAND);
        EXPECT_EQ((u32)target->volatiles.wrapped, TRUE);
    }
}

SINGLE_BATTLE_TEST("Polished binding does not refresh an existing countdown")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_WRAP;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < ARRAY_COUNT(sSourceBindingMoves); kind++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; move = sSourceBindingMoves[kind]; }
    GIVEN {
        WITH_CONFIG(B_BINDING_TURNS, GEN_5);
        PLAYER(SPECIES_RATTATA) { Ability(ABILITY_KEEN_EYE); HP(720); MaxHP(720); }
        OPPONENT(SPECIES_RATTATA) { Ability(ABILITY_KEEN_EYE); HP(720); MaxHP(720); }
    } WHEN {
        TURN { MOVE(user, move, hit: TRUE, WITH_RNG(RNG_WRAP, 4)); }
        TURN { MOVE(user, move, hit: TRUE, WITH_RNG(RNG_WRAP, 5)); }
    } THEN {
        EXPECT_EQ((u32)target->volatiles.wrapped, TRUE);
        EXPECT_EQ((u32)target->volatiles.wrapTurns, B_POLISHED_MOVE_BALANCE ? 1 : 2);
    }
}

SINGLE_BATTLE_TEST("Polished binding keeps damaging and counting during Fly or Dig")
{
    enum Move move = MOVE_WRAP, hidden = MOVE_FLY;
    for (u32 kind = 0; kind < ARRAY_COUNT(sSourceBindingMoves); kind++)
        for (u32 hide = 0; hide < 2; hide++)
            PARAMETRIZE { move = sSourceBindingMoves[kind]; hidden = hide ? MOVE_DIG : MOVE_FLY; }
    GIVEN {
        WITH_CONFIG(B_BINDING_TURNS, GEN_5);
        WITH_CONFIG(B_BINDING_DAMAGE, GEN_6);
        PLAYER(SPECIES_RATTATA) { Ability(ABILITY_KEEN_EYE); HP(720); MaxHP(720); }
        OPPONENT(SPECIES_RATTATA) { Ability(ABILITY_KEEN_EYE); HP(720); MaxHP(720); }
    } WHEN {
        TURN { MOVE(player, move, hit: TRUE, WITH_RNG(RNG_WRAP, 4)); }
        TURN { MOVE(opponent, hidden); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, player);
        HP_BAR(opponent);
        HP_BAR(opponent, damage: 90);
        ANIMATION(ANIM_TYPE_MOVE, hidden, opponent);
        HP_BAR(opponent, damage: 90);
    } THEN {
        EXPECT_EQ((u32)opponent->volatiles.wrapTurns, B_POLISHED_MOVE_BALANCE ? 1 : 2);
    }
}

SINGLE_BATTLE_TEST("Polished binding preserves native duration for optional binding moves")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_BIND;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 3; kind++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; move = kind == 0 ? MOVE_BIND : kind == 1 ? MOVE_SAND_TOMB : MOVE_INFESTATION; }
    GIVEN {
        WITH_CONFIG(B_BINDING_TURNS, GEN_5);
        PLAYER(SPECIES_RATTATA) { Ability(ABILITY_KEEN_EYE); HP(720); MaxHP(720); }
        OPPONENT(SPECIES_RATTATA) { Ability(ABILITY_KEEN_EYE); HP(720); MaxHP(720); }
    } WHEN {
        TURN { MOVE(user, move, hit: TRUE, WITH_RNG(RNG_WRAP, 4)); }
        TURN {}
        TURN {}
        TURN {}
    } THEN {
        EXPECT_EQ((u32)target->volatiles.wrapped, TRUE);
        EXPECT_EQ((u32)target->volatiles.wrapTurns, 0);
    }
}

SINGLE_BATTLE_TEST("Polished binding preserves native duration with either battler Terastallized")
{
    enum Move move = MOVE_WRAP;
    bool32 userTera = FALSE;
    for (u32 kind = 0; kind < ARRAY_COUNT(sSourceBindingMoves); kind++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = sSourceBindingMoves[kind]; userTera = side; }
    GIVEN {
        WITH_CONFIG(B_BINDING_TURNS, GEN_5);
        PLAYER(SPECIES_RATTATA) { Ability(ABILITY_KEEN_EYE); HP(720); MaxHP(720); TeraType(TYPE_NORMAL); }
        OPPONENT(SPECIES_RATTATA) { Ability(ABILITY_KEEN_EYE); HP(720); MaxHP(720); TeraType(TYPE_NORMAL); }
    } WHEN {
        TURN { MOVE(player, move, hit: TRUE, WITH_RNG(RNG_WRAP, 4), gimmick: userTera ? GIMMICK_TERA : GIMMICK_NONE); MOVE(opponent, MOVE_CELEBRATE, gimmick: userTera ? GIMMICK_NONE : GIMMICK_TERA); }
        TURN {}
        TURN {}
        TURN {}
    } THEN {
        EXPECT_EQ((u32)opponent->volatiles.wrapped, TRUE);
        EXPECT_EQ((u32)opponent->volatiles.wrapTurns, 0);
    }
}

DOUBLE_BATTLE_TEST("Polished binding preserves native Substitute cleanup in doubles")
{
    enum Move move = MOVE_WRAP;
    bool32 substitute = FALSE;
    for (u32 kind = 0; kind < ARRAY_COUNT(sSourceBindingMoves); kind++)
        for (u32 sub = 0; sub < 2; sub++)
            PARAMETRIZE { move = sSourceBindingMoves[kind]; substitute = sub; }
    GIVEN {
        WITH_CONFIG(B_BINDING_TURNS, GEN_5);
        PLAYER(SPECIES_RATTATA) { Ability(ABILITY_KEEN_EYE); HP(720); MaxHP(720); }
        PLAYER(SPECIES_RATTATA) { Ability(ABILITY_KEEN_EYE); HP(720); MaxHP(720); }
        OPPONENT(SPECIES_RATTATA) { Ability(ABILITY_KEEN_EYE); HP(720); MaxHP(720); }
        OPPONENT(SPECIES_RATTATA) { Ability(ABILITY_KEEN_EYE); HP(720); MaxHP(720); }
    } WHEN {
        TURN { MOVE(playerLeft, move, target: opponentLeft, hit: TRUE, WITH_RNG(RNG_WRAP, 4)); }
        TURN { MOVE(opponentLeft, substitute ? MOVE_SUBSTITUTE : MOVE_CELEBRATE); }
        TURN {}
        TURN {}
    } THEN {
        EXPECT_EQ((u32)opponentLeft->volatiles.wrapped, !substitute);
        if (!substitute) EXPECT_EQ((u32)opponentLeft->volatiles.wrapTurns, 0);
    }
}

SINGLE_BATTLE_TEST("Polished binding pauses even when only the release turn remains")
{
    struct BattlePokemon *user = player, *target = opponent;
    bool32 suppress = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 release = 0; release < 2; release++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; suppress = release; }
    GIVEN {
        WITH_CONFIG(B_BINDING_TURNS, GEN_5);
        WITH_CONFIG(B_BINDING_DAMAGE, GEN_6);
        PLAYER(SPECIES_RATTATA) { Ability(user == player ? ABILITY_MAGIC_GUARD : ABILITY_KEEN_EYE); HP(720); MaxHP(720); }
        OPPONENT(SPECIES_RATTATA) { Ability(user == opponent ? ABILITY_MAGIC_GUARD : ABILITY_KEEN_EYE); HP(720); MaxHP(720); }
    } WHEN {
        TURN { MOVE(user, MOVE_WRAP, hit: TRUE, WITH_RNG(RNG_WRAP, 4)); }
        TURN {}
        TURN {}
        TURN { MOVE(target, MOVE_SKILL_SWAP); }
        TURN {}
        TURN {}
        if (suppress) TURN { MOVE(user, MOVE_GASTRO_ACID); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_WRAP, user);
        HP_BAR(target);
        for (u32 tick = 0; tick < 3; tick++) HP_BAR(target, damage: 90);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SKILL_SWAP, target);
        NOT HP_BAR(target);
    } THEN {
        EXPECT_EQ((u32)target->volatiles.wrapped, B_POLISHED_MOVE_BALANCE && !suppress);
        EXPECT_EQ((u32)target->volatiles.wrapTurns, 0);
    }
}

SINGLE_BATTLE_TEST("Polished binding is cleared by creating a new Substitute")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_WRAP;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < ARRAY_COUNT(sSourceBindingMoves); kind++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; move = sSourceBindingMoves[kind]; }
    GIVEN {
        WITH_CONFIG(B_BINDING_TURNS, GEN_5);
        WITH_CONFIG(B_BINDING_DAMAGE, GEN_6);
        PLAYER(SPECIES_RATTATA) { Ability(ABILITY_KEEN_EYE); HP(720); MaxHP(720); }
        OPPONENT(SPECIES_RATTATA) { Ability(ABILITY_KEEN_EYE); HP(720); MaxHP(720); }
    } WHEN {
        TURN { MOVE(user, move, hit: TRUE, WITH_RNG(RNG_WRAP, 5)); }
        TURN { MOVE(target, MOVE_SUBSTITUTE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, user);
        HP_BAR(target);
        HP_BAR(target, damage: 90);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SUBSTITUTE, target);
        HP_BAR(target, damage: 180);
        NOT HP_BAR(target);
    } THEN {
        EXPECT_EQ((u32)target->volatiles.wrapped, FALSE);
    }
}

SINGLE_BATTLE_TEST("Polished binding does not apply through an existing Substitute")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_WRAP;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < ARRAY_COUNT(sSourceBindingMoves); kind++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; move = sSourceBindingMoves[kind]; }
    GIVEN {
        PLAYER(SPECIES_RATTATA) { Ability(ABILITY_KEEN_EYE); HP(720); MaxHP(720); Speed(target == player ? 100 : 50); }
        OPPONENT(SPECIES_RATTATA) { Ability(ABILITY_KEEN_EYE); HP(720); MaxHP(720); Speed(target == opponent ? 100 : 50); }
    } WHEN {
        TURN { MOVE(target, MOVE_SUBSTITUTE); MOVE(user, move, hit: TRUE); }
    } THEN {
        EXPECT_EQ((u32)target->volatiles.substitute, TRUE);
        EXPECT_EQ((u32)target->volatiles.wrapped, FALSE);
    }
}
