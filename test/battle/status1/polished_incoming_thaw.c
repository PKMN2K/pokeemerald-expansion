#include "global.h"
#include "test/battle.h"

// Authenticated effects.asm: these seven scripts execute burntarget, whose
// existing-status branch is independent of the secondary-effect chance.
static const enum Move sBurnCommandMoves[] = {
    MOVE_FIRE_PUNCH, MOVE_FLARE_BLITZ, MOVE_EMBER, MOVE_FLAMETHROWER,
    MOVE_SCALD, MOVE_FIRE_BLAST, MOVE_SACRED_FIRE,
};

SINGLE_BATTLE_TEST("Polished incoming thaw follows all seven burn commands after Normalize despite secondary blockers")
{
    enum Move move = MOVE_EMBER;
    bool32 enemy = FALSE;
    u32 blocker = 0;
    for (u32 m = 0; m < ARRAY_COUNT(sBurnCommandMoves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 b = 0; b < 3; b++)
                PARAMETRIZE { move = sBurnCommandMoves[m]; enemy = side; blocker = b; }
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    bool32 thaws = B_POLISHED_MOVE_BALANCE || move == MOVE_SCALD || move == MOVE_FLARE_BLITZ || move == MOVE_SACRED_FIRE;
    GIVEN {
        WITH_CONFIG(B_HIT_THAW, GEN_6);
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(enemy ? (blocker == 1 ? ABILITY_SHIELD_DUST : ABILITY_NONE) : ABILITY_NORMALIZE);
            Speed(enemy ? 50 : 100); HP(4000); MaxHP(4000); Attack(50); SpAttack(50);
            if (enemy) { Status1(STATUS1_FREEZE); if (blocker == 2) Item(ITEM_COVERT_CLOAK); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(!enemy ? (blocker == 1 ? ABILITY_SHIELD_DUST : ABILITY_NONE) : ABILITY_NORMALIZE);
            Speed(enemy ? 100 : 50); HP(4000); MaxHP(4000); Attack(50); SpAttack(50);
            if (!enemy) { Status1(STATUS1_FREEZE); if (blocker == 2) Item(ITEM_COVERT_CLOAK); }
        }
    } WHEN {
        TURN { MOVE(user, move, hit: TRUE, secondaryEffect: FALSE); MOVE(target, MOVE_CELEBRATE, WITH_RNG(RNG_FROZEN, B_POLISHED_MOVE_BALANCE ? 255 : FALSE)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, user);
        HP_BAR(target);
        if (thaws) STATUS_ICON(target, none: TRUE);
        else NOT STATUS_ICON(target, none: TRUE);
    } THEN {
        EXPECT_EQ(target->status1, thaws ? STATUS1_NONE : STATUS1_FREEZE);
    }
}

SINGLE_BATTLE_TEST("Polished incoming thaw separates effective Sheer Force from the current Fire type and native preset")
{
    static const enum Move moves[] = {MOVE_SCALD, MOVE_FLAMETHROWER, MOVE_FIRE_SPIN};
    enum Move move = MOVE_SCALD;
    bool32 enemy = FALSE, sheerForce = FALSE, gas = FALSE, old = FALSE;
    for (u32 m = 0; m < ARRAY_COUNT(moves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 sf = 0; sf < 2; sf++)
                for (u32 suppressed = 0; suppressed < 2; suppressed++)
                    for (u32 preset = 0; preset < 2; preset++)
                        PARAMETRIZE { move = moves[m]; enemy = side; sheerForce = sf; gas = suppressed; old = preset; }
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    bool32 thaws = move != MOVE_SCALD || !sheerForce || gas;
    if (!B_POLISHED_MOVE_BALANCE && old) thaws = move != MOVE_FIRE_SPIN;
    GIVEN {
        WITH_CONFIG(B_HIT_THAW, old ? GEN_2 : GEN_6);
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(enemy ? (gas ? ABILITY_NEUTRALIZING_GAS : ABILITY_NONE) : (sheerForce ? ABILITY_SHEER_FORCE : ABILITY_NONE));
            Speed(enemy ? 50 : 100); HP(4000); MaxHP(4000); SpAttack(50);
            if (enemy) Status1(STATUS1_FREEZE);
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(!enemy ? (gas ? ABILITY_NEUTRALIZING_GAS : ABILITY_NONE) : (sheerForce ? ABILITY_SHEER_FORCE : ABILITY_NONE));
            Speed(enemy ? 100 : 50); HP(4000); MaxHP(4000); SpAttack(50);
            if (!enemy) Status1(STATUS1_FREEZE);
        }
    } WHEN {
        TURN { MOVE(user, move, hit: TRUE, secondaryEffect: FALSE); MOVE(target, MOVE_CELEBRATE, WITH_RNG(RNG_FROZEN, B_POLISHED_MOVE_BALANCE ? 255 : FALSE)); }
    } THEN { EXPECT_EQ(target->status1, thaws ? STATUS1_NONE : STATUS1_FREEZE); }
}

SINGLE_BATTLE_TEST("Polished incoming thaw places burn commands before hit abilities and current Fire before Air Balloon")
{
    bool32 enemy = FALSE, burnCommand = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 phase = 0; phase < 2; phase++)
            PARAMETRIZE { enemy = side; burnCommand = phase; }
    enum Move move = burnCommand ? MOVE_SCALD : MOVE_FIRE_SPIN;
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    GIVEN {
        WITH_CONFIG(B_HIT_THAW, GEN_6);
        PLAYER(SPECIES_WOBBUFFET) { Ability(enemy ? ABILITY_CURSED_BODY : ABILITY_NONE); Speed(enemy ? 50 : 100); HP(4000); MaxHP(4000); if (enemy) { Status1(STATUS1_FREEZE); Item(ITEM_AIR_BALLOON); } }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(!enemy ? ABILITY_CURSED_BODY : ABILITY_NONE); Speed(enemy ? 100 : 50); HP(4000); MaxHP(4000); if (!enemy) { Status1(STATUS1_FREEZE); Item(ITEM_AIR_BALLOON); } }
    } WHEN {
        TURN { MOVE(user, move, hit: TRUE, secondaryEffect: FALSE, WITH_RNG(RNG_CURSED_BODY, TRUE)); MOVE(target, MOVE_CELEBRATE, WITH_RNG(RNG_FROZEN, B_POLISHED_MOVE_BALANCE ? 255 : FALSE)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, user);
        HP_BAR(target);
        if (B_POLISHED_MOVE_BALANCE && burnCommand) STATUS_ICON(target, none: TRUE);
        ABILITY_POPUP(target, ABILITY_CURSED_BODY);
        if (B_POLISHED_MOVE_BALANCE && !burnCommand) STATUS_ICON(target, none: TRUE);
        if (enemy) MESSAGE("Wobbuffet's Air Balloon popped!");
        else MESSAGE("The opposing Wobbuffet's Air Balloon popped!");
        if (!B_POLISHED_MOVE_BALANCE) STATUS_ICON(target, none: TRUE);
    } THEN {
        EXPECT_EQ(target->status1, STATUS1_NONE);
        EXPECT_EQ(target->item, ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished incoming thaw rejects intact and broken Substitute hits but allows Infiltrator real damage")
{
    bool32 enemy = FALSE, bypass = FALSE, breaks = FALSE, burnCommand = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 infiltrator = 0; infiltrator < 2; infiltrator++)
            for (u32 large = 0; large < 2; large++)
                for (u32 phase = 0; phase < 2; phase++)
                    PARAMETRIZE { enemy = side; bypass = infiltrator; breaks = large; burnCommand = phase; }
    enum Move move = burnCommand ? MOVE_SCALD : MOVE_FIRE_SPIN;
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    GIVEN {
        WITH_CONFIG(B_HIT_THAW, GEN_6);
        PLAYER(SPECIES_WOBBUFFET) { Ability(!enemy && bypass ? ABILITY_INFILTRATOR : ABILITY_NONE); Speed(enemy ? 200 : 100); HP(enemy ? 400 : 4000); MaxHP(enemy ? 400 : 4000); SpAttack(breaks ? 2000 : 10); }
        if (enemy) PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); Status1(STATUS1_FREEZE); HP(4000); MaxHP(4000); SpDefense(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(enemy && bypass ? ABILITY_INFILTRATOR : ABILITY_NONE); Speed(enemy ? 100 : 200); HP(enemy ? 4000 : 400); MaxHP(enemy ? 4000 : 400); SpAttack(breaks ? 2000 : 10); }
        if (!enemy) OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); Status1(STATUS1_FREEZE); HP(4000); MaxHP(4000); SpDefense(100); }
    } WHEN {
        TURN { MOVE(target, MOVE_SUBSTITUTE); MOVE(user, MOVE_SPLASH); }
        TURN { MOVE(target, MOVE_BATON_PASS); SEND_OUT(target, 1); MOVE(user, MOVE_SPLASH); }
        TURN { MOVE(user, move, hit: TRUE, secondaryEffect: FALSE); MOVE(target, MOVE_CELEBRATE, WITH_RNG(RNG_FROZEN, B_POLISHED_MOVE_BALANCE ? 255 : FALSE)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, user);
        if (bypass) STATUS_ICON(target, none: TRUE);
        else NOT STATUS_ICON(target, none: TRUE);
    } THEN {
        EXPECT_EQ(target->status1, bypass ? STATUS1_NONE : STATUS1_FREEZE);
        EXPECT_EQ(!!target->volatiles.substitute, bypass || !breaks);
        if (bypass) EXPECT_LT(target->hp, 4000);
        else EXPECT_EQ(target->hp, 4000);
    }
}

SINGLE_BATTLE_TEST("Polished incoming thaw rejects misses immunity and an interrupted action")
{
    bool32 enemy = FALSE;
    u32 failure = 0;
    for (u32 side = 0; side < 2; side++)
        for (u32 cause = 0; cause < 3; cause++)
            PARAMETRIZE { enemy = side; failure = cause; }
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    GIVEN {
        PLAYER(enemy && failure == 1 ? SPECIES_GENGAR : SPECIES_WOBBUFFET) { Ability(enemy ? ABILITY_NONE : ABILITY_NORMALIZE); Speed(enemy ? 50 : 100); if (enemy) Status1(STATUS1_FREEZE); else if (failure == 2) Status1(3); }
        OPPONENT(!enemy && failure == 1 ? SPECIES_GENGAR : SPECIES_WOBBUFFET) { Ability(!enemy ? ABILITY_NONE : ABILITY_NORMALIZE); Speed(enemy ? 100 : 50); if (!enemy) Status1(STATUS1_FREEZE); else if (failure == 2) Status1(3); }
    } WHEN {
        TURN { MOVE(user, MOVE_FIRE_BLAST, hit: failure != 0); MOVE(target, MOVE_CELEBRATE, WITH_RNG(RNG_FROZEN, B_POLISHED_MOVE_BALANCE ? 255 : FALSE)); }
    } THEN { EXPECT_EQ(target->status1, STATUS1_FREEZE); }
}

SINGLE_BATTLE_TEST("Polished incoming thaw burn command still clears a fainted target before its faint animation")
{
    bool32 enemy = FALSE, burnCommand = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 phase = 0; phase < 2; phase++)
            PARAMETRIZE { enemy = side; burnCommand = phase; }
    enum Move move = burnCommand ? MOVE_EMBER : MOVE_FIRE_SPIN;
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(enemy ? 50 : 100); HP(enemy ? 1 : 4000); MaxHP(4000); if (enemy) Status1(STATUS1_FREEZE); }
        if (enemy) PLAYER(SPECIES_WYNAUT) { Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(enemy ? 100 : 50); HP(!enemy ? 1 : 4000); MaxHP(4000); if (!enemy) Status1(STATUS1_FREEZE); }
        if (!enemy) OPPONENT(SPECIES_WYNAUT) { Speed(50); }
    } WHEN {
        TURN { MOVE(user, move, hit: TRUE, secondaryEffect: FALSE); SEND_OUT(target, 1); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, user);
        HP_BAR(target);
        if (B_POLISHED_MOVE_BALANCE && burnCommand) STATUS_ICON(target, none: TRUE);
        else NOT STATUS_ICON(target, none: TRUE);
        if (enemy) MESSAGE("Wobbuffet fainted!");
        else MESSAGE("The opposing Wobbuffet fainted!");
    }
}

SINGLE_BATTLE_TEST("Polished incoming thaw Flare Blitz follows lethal recoil without requiring a living user")
{
    bool32 enemy = FALSE;
    PARAMETRIZE { enemy = FALSE; }
    PARAMETRIZE { enemy = TRUE; }
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(enemy ? 50 : 100); HP(enemy ? 4000 : 1); MaxHP(4000); if (enemy) Status1(STATUS1_FREEZE); }
        if (!enemy) PLAYER(SPECIES_WYNAUT) { Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(enemy ? 100 : 50); HP(!enemy ? 4000 : 1); MaxHP(4000); if (!enemy) Status1(STATUS1_FREEZE); }
        if (enemy) OPPONENT(SPECIES_WYNAUT) { Speed(100); }
    } WHEN {
        TURN { MOVE(user, MOVE_FLARE_BLITZ, secondaryEffect: FALSE); SEND_OUT(user, 1); MOVE(target, MOVE_CELEBRATE, WITH_RNG(RNG_FROZEN, B_POLISHED_MOVE_BALANCE ? 255 : FALSE)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_FLARE_BLITZ, user);
        HP_BAR(target);
        if (!B_POLISHED_MOVE_BALANCE) STATUS_ICON(target, none: TRUE);
        HP_BAR(user);
        if (B_POLISHED_MOVE_BALANCE) STATUS_ICON(target, none: TRUE);
        if (enemy) MESSAGE("The opposing Wobbuffet fainted!");
        else MESSAGE("Wobbuffet fainted!");
    } THEN { EXPECT_EQ(target->status1, STATUS1_NONE); }
}

SINGLE_BATTLE_TEST("Polished incoming thaw Tri Attack burn branch never defrosts under the legacy native preset")
{
    bool32 enemy = FALSE, old = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 preset = 0; preset < 2; preset++)
            PARAMETRIZE { enemy = side; old = preset; }
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    GIVEN {
        WITH_CONFIG(B_HIT_THAW, old ? GEN_2 : GEN_6);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(enemy ? 50 : 100); HP(4000); MaxHP(4000); if (enemy) Status1(STATUS1_FREEZE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(enemy ? 100 : 50); HP(4000); MaxHP(4000); if (!enemy) Status1(STATUS1_FREEZE); }
    } WHEN {
        TURN { MOVE(user, MOVE_TRI_ATTACK, secondaryEffect: TRUE, WITH_RNG(RNG_RANDOM_FROM_LIST, 0)); MOVE(target, MOVE_CELEBRATE, WITH_RNG(RNG_FROZEN, B_POLISHED_MOVE_BALANCE ? 255 : FALSE)); }
    } THEN { EXPECT_EQ(target->status1, !B_POLISHED_MOVE_BALANCE && old ? STATUS1_NONE : STATUS1_FREEZE); }
}

SINGLE_BATTLE_TEST("Polished incoming thaw either established native gimmick keeps Normalize on native thaw rules")
{
    bool32 enemy = FALSE, userTera = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 actor = 0; actor < 2; actor++)
            PARAMETRIZE { enemy = side; userTera = actor; }
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    GIVEN {
        WITH_CONFIG(B_HIT_THAW, GEN_6);
        WITH_CONFIG(B_FREEZE_TURNS, GEN_9);
        PLAYER(SPECIES_WOBBUFFET) { Ability(enemy ? ABILITY_NONE : ABILITY_NORMALIZE); Speed(enemy ? 50 : 100); HP(4000); MaxHP(4000); TeraType(TYPE_NORMAL); if (enemy) Status1(STATUS1_FREEZE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(!enemy ? ABILITY_NONE : ABILITY_NORMALIZE); Speed(enemy ? 100 : 50); HP(4000); MaxHP(4000); TeraType(TYPE_NORMAL); if (!enemy) Status1(STATUS1_FREEZE); }
    } WHEN {
        // Start already frozen: fresh native freeze deliberately has a forced
        // first thaw, which would obscure the incoming-move scope check.
        TURN { MOVE(user, MOVE_SPLASH, gimmick: userTera ? GIMMICK_TERA : GIMMICK_NONE); MOVE(target, MOVE_SPLASH, gimmick: userTera ? GIMMICK_NONE : GIMMICK_TERA, WITH_RNG(RNG_FROZEN, FALSE)); }
        TURN { MOVE(user, MOVE_EMBER, secondaryEffect: FALSE); MOVE(target, MOVE_CELEBRATE, WITH_RNG(RNG_FROZEN, FALSE)); }
    } THEN { EXPECT_EQ(target->status1, STATUS1_FREEZE); }
}

SINGLE_BATTLE_TEST("Polished incoming thaw optional Flame Wheel preserves the legacy native burn rule")
{
    GIVEN {
        WITH_CONFIG(B_HIT_THAW, GEN_2);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_SHEER_FORCE); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); Status1(STATUS1_FREEZE); }
    } WHEN {
        TURN { MOVE(player, MOVE_FLAME_WHEEL, secondaryEffect: FALSE); MOVE(opponent, MOVE_CELEBRATE, WITH_RNG(RNG_FROZEN, FALSE)); }
    } THEN { EXPECT_EQ(opponent->status1, STATUS1_NONE); }
}

DOUBLE_BATTLE_TEST("Polished incoming thaw optional doubles keep normalized burn moves native")
{
    GIVEN {
        WITH_CONFIG(B_HIT_THAW, GEN_6);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NORMALIZE); Speed(100); }
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(90); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); Status1(STATUS1_FREEZE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(40); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_EMBER, target: opponentLeft, secondaryEffect: FALSE); MOVE(opponentLeft, MOVE_CELEBRATE, WITH_RNG(RNG_FROZEN, FALSE)); }
    } THEN { EXPECT_EQ(opponentLeft->status1, STATUS1_FREEZE); }
}
