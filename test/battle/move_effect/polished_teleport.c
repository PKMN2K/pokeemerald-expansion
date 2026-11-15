#include "global.h"
#include "test/battle.h"
#include "string_util.h"

SINGLE_BATTLE_TEST("Polished Teleport uses the pinned source move description")
{
    GIVEN {
        PLAYER(SPECIES_RATTATA) { Ability(ABILITY_KEEN_EYE); }
        OPPONENT(SPECIES_RATTATA) { Ability(ABILITY_KEEN_EYE); }
    } WHEN {
        TURN {}
    } THEN {
        EXPECT_EQ(StringCompare(GetMoveDescription(MOVE_TELEPORT), COMPOUND_STRING("A move for fleeing\nfrom battle.")) == 0, B_POLISHED_MOVE_BALANCE);
    }
}

static const struct { enum Species species; enum Ability trap; bool32 trapped; } sTeleportTraps[] = {
    {SPECIES_RATTATA, ABILITY_ARENA_TRAP, TRUE},
    {SPECIES_PIDGEY, ABILITY_ARENA_TRAP, FALSE},
    {SPECIES_MAGNEMITE, ABILITY_MAGNET_PULL, TRUE},
    {SPECIES_RATTATA, ABILITY_MAGNET_PULL, FALSE},
    {SPECIES_RATTATA, ABILITY_SHADOW_TAG, TRUE},
    {SPECIES_GASTLY, ABILITY_SHADOW_TAG, FALSE},
};

WILD_BATTLE_TEST("Polished Teleport flees from wild battles regardless of level or replacement Pokemon")
{
    struct BattlePokemon *user = player;
    bool32 bench = FALSE;
    u32 level = 5, gen = GEN_8;
    for (u32 side = 0; side < 2; side++)
        for (u32 reserve = 0; reserve < 2; reserve++)
            for (u32 high = 0; high < 2; high++)
                for (u32 old = 0; old < 2; old++)
                    PARAMETRIZE { user = side ? opponent : player; bench = reserve; level = high ? 100 : 5; gen = old ? GEN_7 : GEN_8; }
    bool32 nativePlayerSwitch = !B_POLISHED_MOVE_BALANCE && user == player && gen >= GEN_8;
    GIVEN {
        WITH_CONFIG(B_TELEPORT_BEHAVIOR, gen);
        PLAYER(SPECIES_RATTATA) { Ability(ABILITY_KEEN_EYE); Level(user == player ? level : 105 - level); Speed(50); }
        if (bench) PLAYER(SPECIES_PIDGEY) { Ability(ABILITY_KEEN_EYE); Speed(75); }
        OPPONENT(SPECIES_RATTATA) { Ability(ABILITY_KEEN_EYE); Level(user == opponent ? level : 105 - level); Speed(100); }
    } WHEN {
        TURN { MOVE(user, MOVE_TELEPORT); if (nativePlayerSwitch && bench) SEND_OUT(player, 1); }
    } SCENE {
        if (!nativePlayerSwitch || bench) ANIMATION(ANIM_TYPE_MOVE, MOVE_TELEPORT, user);
        else MESSAGE("But it failed!");
    } THEN {
        // Recorded-test teardown also sets PLAYER_TELEPORTED after non-ending turns;
        // the required animation/failure scene distinguishes that harness outcome.
        if (!nativePlayerSwitch) EXPECT_EQ(gBattleOutcome, user == player ? B_OUTCOME_PLAYER_TELEPORTED : B_OUTCOME_MON_TELEPORTED);
        if (nativePlayerSwitch && bench) EXPECT_EQ(player->species, SPECIES_PIDGEY);
    }
}

WILD_BATTLE_TEST("Polished Teleport respects source ability traps and the Run Away exemption")
{
    struct BattlePokemon *user = player;
    u32 row = 0;
    bool32 runaway = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < ARRAY_COUNT(sTeleportTraps); kind++)
            for (u32 run = 0; run < 2; run++)
                PARAMETRIZE { user = side ? opponent : player; row = kind; runaway = run; }
    enum Ability ability = runaway ? ABILITY_RUN_AWAY : ABILITY_KEEN_EYE;
    bool32 flees = (B_POLISHED_MOVE_BALANCE || user == opponent) && (runaway || !sTeleportTraps[row].trapped);
    GIVEN {
        WITH_CONFIG(B_TELEPORT_BEHAVIOR, GEN_8);
        PLAYER(user == player ? sTeleportTraps[row].species : SPECIES_RATTATA) { Speed(user == player ? 50 : 100); Ability(user == player ? ability : sTeleportTraps[row].trap); }
        OPPONENT(user == opponent ? sTeleportTraps[row].species : SPECIES_RATTATA) { Speed(user == opponent ? 50 : 100); Ability(user == opponent ? ability : sTeleportTraps[row].trap); Moves(user == opponent ? MOVE_TELEPORT : MOVE_CELEBRATE); HP(720); MaxHP(720); }
    } WHEN {
        TURN { MOVE(user, MOVE_TELEPORT); MOVE(user == player ? opponent : player, MOVE_CELEBRATE); }
        if (!flees) TURN { MOVE(player, MOVE_TACKLE, hit: TRUE); MOVE(opponent, user == opponent ? MOVE_TELEPORT : MOVE_CELEBRATE); }
    } SCENE {
        if (flees) ANIMATION(ANIM_TYPE_MOVE, MOVE_TELEPORT, user);
        else {
            NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_TELEPORT, user);
            ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player);
            HP_BAR(opponent);
        }
    } THEN {
        if (flees) EXPECT_EQ(gBattleOutcome, user == player ? B_OUTCOME_PLAYER_TELEPORTED : B_OUTCOME_MON_TELEPORTED);
    }
}

WILD_BATTLE_TEST("Polished Teleport permits escape when source trapping abilities cancel each other")
{
    struct BattlePokemon *user = player;
    enum Ability other = ABILITY_ARENA_TRAP;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 2; kind++)
            PARAMETRIZE { user = side ? opponent : player; other = kind ? ABILITY_MAGNET_PULL : ABILITY_ARENA_TRAP; }
    GIVEN {
        WITH_CONFIG(B_TELEPORT_BEHAVIOR, GEN_8);
        PLAYER(SPECIES_MAGNEMITE) { Speed(user == player ? 50 : 100); Ability(ABILITY_ARENA_TRAP); }
        OPPONENT(SPECIES_MAGNEMITE) { Speed(user == opponent ? 50 : 100); Ability(other); Moves(user == opponent ? MOVE_TELEPORT : MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(user, MOVE_TELEPORT); MOVE(user == player ? opponent : player, MOVE_CELEBRATE); }
        if (!B_POLISHED_MOVE_BALANCE) TURN { MOVE(player, MOVE_TACKLE, hit: TRUE); MOVE(opponent, user == opponent ? MOVE_TELEPORT : MOVE_CELEBRATE); }
    } SCENE {
        if (B_POLISHED_MOVE_BALANCE) ANIMATION(ANIM_TYPE_MOVE, MOVE_TELEPORT, user);
        else {
            NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_TELEPORT, user);
            ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player);
            HP_BAR(opponent);
        }
    }
}

WILD_BATTLE_TEST("Polished Teleport checks Run Away after Neutralizing Gas suppression")
{
    struct BattlePokemon *user = player, *foe = opponent;
    bool32 gas = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 active = 0; active < 2; active++)
            PARAMETRIZE { user = side ? opponent : player; foe = side ? player : opponent; gas = active; }
    bool32 flees = !gas && (B_POLISHED_MOVE_BALANCE || user == opponent);
    GIVEN {
        WITH_CONFIG(B_TELEPORT_BEHAVIOR, GEN_8);
        PLAYER(SPECIES_RATTATA) { Speed(user == player ? 50 : 100); Ability(user == player ? ABILITY_RUN_AWAY : gas ? ABILITY_NEUTRALIZING_GAS : ABILITY_KEEN_EYE); }
        OPPONENT(SPECIES_RATTATA) { Speed(user == opponent ? 50 : 100); Ability(user == opponent ? ABILITY_RUN_AWAY : gas ? ABILITY_NEUTRALIZING_GAS : ABILITY_KEEN_EYE); Moves(user == opponent ? MOVE_TELEPORT : MOVE_MEAN_LOOK); HP(720); MaxHP(720); }
    } WHEN {
        TURN { MOVE(foe, MOVE_MEAN_LOOK); MOVE(user, MOVE_TELEPORT); }
        if (!flees) TURN { MOVE(player, MOVE_TACKLE, hit: TRUE); MOVE(opponent, user == opponent ? MOVE_TELEPORT : MOVE_MEAN_LOOK); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_MEAN_LOOK, foe);
        if (flees) ANIMATION(ANIM_TYPE_MOVE, MOVE_TELEPORT, user);
        else {
            NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_TELEPORT, user);
            ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player);
            HP_BAR(opponent);
        }
    }
}

WILD_BATTLE_TEST("Polished Teleport checks an existing Mean Look before a later Ghost type")
{
    GIVEN {
        WITH_CONFIG(B_TELEPORT_BEHAVIOR, GEN_8);
        WITH_CONFIG(B_GHOSTS_ESCAPE, GEN_6);
        PLAYER(SPECIES_RATTATA) { Speed(100); Ability(ABILITY_KEEN_EYE); }
        OPPONENT(SPECIES_RATTATA) { Speed(50); Ability(ABILITY_KEEN_EYE); Moves(MOVE_TELEPORT); }
    } WHEN {
        TURN { MOVE(player, MOVE_MEAN_LOOK); MOVE(opponent, MOVE_TELEPORT); }
        TURN { MOVE(player, MOVE_TRICK_OR_TREAT); MOVE(opponent, MOVE_TELEPORT); }
        if (B_POLISHED_MOVE_BALANCE) TURN { MOVE(player, MOVE_BITE, hit: TRUE); MOVE(opponent, MOVE_TELEPORT); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TRICK_OR_TREAT, player);
        if (!B_POLISHED_MOVE_BALANCE) ANIMATION(ANIM_TYPE_MOVE, MOVE_TELEPORT, opponent);
        else {
            NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_TELEPORT, opponent);
            ANIMATION(ANIM_TYPE_MOVE, MOVE_BITE, player);
            HP_BAR(opponent);
        }
    } THEN {
        EXPECT_EQ((u32)opponent->volatiles.escapePrevention, TRUE);
    }
}

WILD_BATTLE_TEST("Polished Teleport retains native wild behavior when the player Terastallizes")
{
    struct BattlePokemon *user = player, *foe = opponent;
    PARAMETRIZE { user = player; foe = opponent; }
    PARAMETRIZE { user = opponent; foe = player; }
    GIVEN {
        WITH_CONFIG(B_TELEPORT_BEHAVIOR, GEN_8);
        PLAYER(SPECIES_RATTATA) { Speed(user == player ? 50 : 100); Ability(ABILITY_KEEN_EYE); HP(720); MaxHP(720); TeraType(TYPE_NORMAL); }
        OPPONENT(SPECIES_RATTATA) { Speed(user == opponent ? 50 : 100); Ability(ABILITY_KEEN_EYE); HP(720); MaxHP(720); Moves(user == opponent ? MOVE_TELEPORT : MOVE_WRAP); }
    } WHEN {
        TURN { MOVE(foe, MOVE_WRAP, hit: TRUE, gimmick: foe == player ? GIMMICK_TERA : GIMMICK_NONE); MOVE(user, MOVE_TELEPORT, gimmick: user == player ? GIMMICK_TERA : GIMMICK_NONE); }
        TURN { MOVE(player, MOVE_TACKLE, hit: TRUE); MOVE(opponent, user == opponent ? MOVE_TELEPORT : MOVE_WRAP); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_WRAP, foe);
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_TELEPORT, user);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player);
        HP_BAR(opponent);
    }
}

WILD_BATTLE_TEST("Polished Teleport still uses the ordinary sleep paralysis and Taunt cancelers")
{
    struct BattlePokemon *user = player, *foe = opponent;
    u32 reason = 0;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 3; kind++)
            PARAMETRIZE { user = side ? opponent : player; foe = side ? player : opponent; reason = kind; }
    u32 status = reason == 0 ? STATUS1_SLEEP : reason == 1 ? STATUS1_PARALYSIS : STATUS1_NONE;
    GIVEN {
        WITH_CONFIG(B_TELEPORT_BEHAVIOR, GEN_8);
        PLAYER(SPECIES_RATTATA) { Speed(user == player ? 50 : 100); Ability(ABILITY_KEEN_EYE); Status1(user == player ? status : STATUS1_NONE); }
        OPPONENT(SPECIES_RATTATA) { Speed(user == opponent ? 50 : 100); Ability(ABILITY_KEEN_EYE); Status1(user == opponent ? status : STATUS1_NONE); Moves(user == opponent ? MOVE_TELEPORT : reason == 2 ? MOVE_TAUNT : MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(foe, reason == 2 ? MOVE_TAUNT : MOVE_CELEBRATE); MOVE(user, MOVE_TELEPORT, WITH_RNG(RNG_PARALYSIS, TRUE)); }
    } SCENE {
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_TELEPORT, user);
    }
}

SINGLE_BATTLE_TEST("Polished Teleport retains trainer switching through source move and ability traps")
{
    struct BattlePokemon *user = player, *foe = opponent;
    u32 kind = 0;
    for (u32 side = 0; side < 2; side++)
        for (u32 trap = 0; trap < 6; trap++)
            PARAMETRIZE { user = side ? opponent : player; foe = side ? player : opponent; kind = trap; }
    enum Ability ability = kind == 3 ? ABILITY_ARENA_TRAP : kind == 4 ? ABILITY_MAGNET_PULL : kind == 5 ? ABILITY_SHADOW_TAG : ABILITY_KEEN_EYE;
    enum Move move = kind == 1 ? MOVE_MEAN_LOOK : kind == 2 ? MOVE_WRAP : MOVE_CELEBRATE;
    GIVEN {
        WITH_CONFIG(B_TELEPORT_BEHAVIOR, GEN_8);
        PLAYER(SPECIES_MAGNEMITE) { Ability(foe == player ? ability : ABILITY_KEEN_EYE); HP(720); MaxHP(720); }
        PLAYER(SPECIES_RATTATA) { Ability(ABILITY_KEEN_EYE); }
        OPPONENT(SPECIES_MAGNEMITE) { Ability(foe == opponent ? ability : ABILITY_KEEN_EYE); HP(720); MaxHP(720); }
        OPPONENT(SPECIES_RATTATA) { Ability(ABILITY_KEEN_EYE); }
    } WHEN {
        TURN { MOVE(foe, move, hit: TRUE); }
        TURN { MOVE(user, MOVE_TELEPORT); SEND_OUT(user, 1); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TELEPORT, user);
    } THEN {
        EXPECT_EQ(user->species, SPECIES_RATTATA);
        EXPECT_EQ((u32)user->volatiles.wrapped, FALSE);
        EXPECT_EQ((u32)user->volatiles.escapePrevention, FALSE);
    }
}

SINGLE_BATTLE_TEST("Polished Teleport still needs a living trainer-battle replacement")
{
    struct BattlePokemon *user = player;
    bool32 fainted = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 reserve = 0; reserve < 2; reserve++)
            PARAMETRIZE { user = side ? opponent : player; fainted = reserve; }
    GIVEN {
        WITH_CONFIG(B_TELEPORT_BEHAVIOR, GEN_8);
        PLAYER(SPECIES_RATTATA) { Ability(ABILITY_RUN_AWAY); }
        if (fainted) PLAYER(SPECIES_PIDGEY) { HP(0); }
        OPPONENT(SPECIES_RATTATA) { Ability(ABILITY_RUN_AWAY); }
        if (fainted) OPPONENT(SPECIES_PIDGEY) { HP(0); }
    } WHEN {
        TURN { MOVE(user, MOVE_TELEPORT); }
    } SCENE {
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_TELEPORT, user);
        MESSAGE("But it failed!");
    }
}

WILD_BATTLE_TEST("Polished Teleport retains native optional Ingrain and Fairy Lock behavior")
{
    enum Move field = MOVE_INGRAIN;
    PARAMETRIZE { field = MOVE_INGRAIN; }
    PARAMETRIZE { field = MOVE_FAIRY_LOCK; }
    GIVEN {
        WITH_CONFIG(B_TELEPORT_BEHAVIOR, GEN_8);
        PLAYER(SPECIES_RATTATA) { Speed(100); Ability(ABILITY_KEEN_EYE); }
        PLAYER(SPECIES_PIDGEY) { Speed(100); Ability(ABILITY_KEEN_EYE); }
        OPPONENT(SPECIES_RATTATA) { Speed(50); Ability(ABILITY_KEEN_EYE); Moves(field == MOVE_FAIRY_LOCK ? MOVE_FAIRY_LOCK : MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, field == MOVE_INGRAIN ? MOVE_INGRAIN : MOVE_CELEBRATE); MOVE(opponent, field == MOVE_FAIRY_LOCK ? MOVE_FAIRY_LOCK : MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_TELEPORT); SEND_OUT(player, 1); MOVE(opponent, field == MOVE_FAIRY_LOCK ? MOVE_FAIRY_LOCK : MOVE_CELEBRATE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TELEPORT, player);
    } THEN {
        EXPECT_EQ(player->species, SPECIES_PIDGEY);
    }
}

DOUBLE_BATTLE_TEST("Polished Teleport retains optional doubles switching")
{
    enum Move trap = MOVE_MEAN_LOOK;
    PARAMETRIZE { trap = MOVE_MEAN_LOOK; }
    PARAMETRIZE { trap = MOVE_WRAP; }
    GIVEN {
        WITH_CONFIG(B_TELEPORT_BEHAVIOR, GEN_8);
        PLAYER(SPECIES_RATTATA) { Speed(100); Ability(ABILITY_KEEN_EYE); HP(720); MaxHP(720); }
        PLAYER(SPECIES_RATTATA) { Speed(100); Ability(ABILITY_KEEN_EYE); }
        PLAYER(SPECIES_PIDGEY) { Speed(100); Ability(ABILITY_KEEN_EYE); }
        OPPONENT(SPECIES_RATTATA) { Speed(50); Ability(ABILITY_SHADOW_TAG); }
        OPPONENT(SPECIES_RATTATA) { Speed(50); Ability(ABILITY_KEEN_EYE); }
    } WHEN {
        TURN { MOVE(opponentLeft, trap, target: playerLeft, hit: TRUE); }
        TURN { MOVE(playerLeft, MOVE_TELEPORT); SEND_OUT(playerLeft, 2); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TELEPORT, playerLeft);
    } THEN {
        EXPECT_EQ(playerLeft->species, SPECIES_PIDGEY);
    }
}

WILD_BATTLE_TEST("Polished Teleport ignores Wrap but only Run Away bypasses Mean Look")
{
    struct BattlePokemon *user = player, *foe = opponent;
    enum Move trap = MOVE_MEAN_LOOK;
    enum Ability ability = ABILITY_KEEN_EYE;
    enum Item item = ITEM_NONE;
    for (u32 side = 0; side < 2; side++)
        for (u32 wrap = 0; wrap < 2; wrap++)
            for (u32 runaway = 0; runaway < 2; runaway++)
                for (u32 held = 0; held < 3; held++)
                    PARAMETRIZE { user = side ? opponent : player; foe = side ? player : opponent; trap = wrap ? MOVE_WRAP : MOVE_MEAN_LOOK; ability = runaway ? ABILITY_RUN_AWAY : ABILITY_KEEN_EYE; item = held == 0 ? ITEM_NONE : held == 1 ? ITEM_SMOKE_BALL : ITEM_SHED_SHELL; }
    bool32 flees = B_POLISHED_MOVE_BALANCE ? trap == MOVE_WRAP || ability == ABILITY_RUN_AWAY : user == opponent && (ability == ABILITY_RUN_AWAY || item == ITEM_SMOKE_BALL);
    GIVEN {
        WITH_CONFIG(B_TELEPORT_BEHAVIOR, GEN_8);
        PLAYER(SPECIES_RATTATA) { Speed(user == player ? 50 : 100); Ability(user == player ? ability : ABILITY_KEEN_EYE); Item(user == player ? item : ITEM_NONE); HP(720); MaxHP(720); }
        OPPONENT(SPECIES_RATTATA) { Speed(user == opponent ? 50 : 100); Ability(user == opponent ? ability : ABILITY_KEEN_EYE); Item(user == opponent ? item : ITEM_NONE); HP(720); MaxHP(720); Moves(user == opponent ? MOVE_TELEPORT : trap); }
    } WHEN {
        TURN { MOVE(foe, trap, hit: TRUE); MOVE(user, MOVE_TELEPORT); }
        if (!flees) TURN { MOVE(player, MOVE_TACKLE, hit: TRUE); MOVE(opponent, user == opponent ? MOVE_TELEPORT : trap, hit: TRUE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, trap, foe);
        if (flees) ANIMATION(ANIM_TYPE_MOVE, MOVE_TELEPORT, user);
        else {
            NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_TELEPORT, user);
            ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player);
            HP_BAR(opponent);
        }
    } THEN {
        if (flees) EXPECT_EQ(gBattleOutcome, user == player ? B_OUTCOME_PLAYER_TELEPORTED : B_OUTCOME_MON_TELEPORTED);
    }
}
