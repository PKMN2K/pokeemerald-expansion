#include "global.h"
#include "test/battle.h"

WILD_BATTLE_TEST("Polished move history Metronome records the disobedient alternate action")
{
    u32 otId = 1;
    s16 alternate, repeat;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_METRONOME); MovesWithPP({MOVE_TACKLE, 10}, {MOVE_SCRATCH, 10}); }
        if (B_POLISHED_MOVE_BALANCE) SetMonData(&PLAYER_PARTY[0], MON_DATA_OT_ID, &otId);
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); HP(1000); MaxHP(1000); Moves(MOVE_SPLASH); }
    } WHEN {
        TURN { MOVE(player, MOVE_TACKLE, WITH_RNG(RNG_POLISHED_OBEDIENCE_FIRST, 0)); }
        TURN { MOVE(player, MOVE_TACKLE, WITH_RNG(RNG_POLISHED_OBEDIENCE_SECOND, 0)); }
        TURN { MOVE(player, MOVE_SCRATCH, WITH_RNG(RNG_POLISHED_OBEDIENCE_FIRST, 0)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player);
        ANIMATION(ANIM_TYPE_MOVE, B_POLISHED_MOVE_BALANCE ? MOVE_SCRATCH : MOVE_TACKLE, player);
        HP_BAR(opponent, captureDamage: &alternate);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SCRATCH, player);
        HP_BAR(opponent, captureDamage: &repeat);
    } THEN {
        if (B_POLISHED_MOVE_BALANCE) {
            EXPECT_GT(repeat, alternate);
            EXPECT_EQ(gBattleStruct->polishedMetronomeMove[B_BATTLER_0], MOVE_SCRATCH);
            EXPECT_EQ(gBattleStruct->polishedMetronomeCount[B_BATTLER_0], 1);
        } else EXPECT_LT(repeat, alternate);
        EXPECT_EQ(player->pp[0], B_POLISHED_MOVE_BALANCE ? 9 : 8);
        EXPECT_EQ(player->pp[1], B_POLISHED_MOVE_BALANCE ? 8 : 9);
    }
}

SINGLE_BATTLE_TEST("Polished move history Metronome retains attempted repeats across Protect and misses")
{
    bool32 protect = FALSE;
    PARAMETRIZE { protect = FALSE; }
    PARAMETRIZE { protect = TRUE; }
    s16 first, after;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_METRONOME); Moves(MOVE_TACKLE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); HP(1000); MaxHP(1000); Item(ITEM_BRIGHTPOWDER); }
    } WHEN {
        TURN { MOVE(player, MOVE_TACKLE, hit: TRUE); }
        TURN { MOVE(player, MOVE_TACKLE, hit: protect); if (protect) MOVE(opponent, MOVE_PROTECT); }
        TURN { MOVE(player, MOVE_TACKLE, hit: TRUE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player);
        HP_BAR(opponent, captureDamage: &first);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player);
        HP_BAR(opponent, captureDamage: &after);
    } THEN {
        if (B_POLISHED_MOVE_BALANCE) {
            EXPECT_GT(after, first);
            EXPECT_EQ(gBattleStruct->polishedMetronomeCount[B_BATTLER_0], 2);
        } else EXPECT_EQ(after, first);
    }
}

SINGLE_BATTLE_TEST("Polished move history Metronome ignores a flinched selection of another move")
{
    s16 first, after;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_METRONOME); Speed(50); Moves(MOVE_TACKLE, MOVE_GROWL); HP(1000); MaxHP(1000); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); HP(1000); MaxHP(1000); }
    } WHEN {
        TURN { MOVE(player, MOVE_TACKLE); }
        TURN { MOVE(opponent, MOVE_BITE, secondaryEffect: TRUE); MOVE(player, MOVE_GROWL); }
        TURN { MOVE(player, MOVE_TACKLE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player);
        HP_BAR(opponent, captureDamage: &first);
        MESSAGE("Wobbuffet flinched and couldn't move!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player);
        HP_BAR(opponent, captureDamage: &after);
    } THEN {
        if (B_POLISHED_MOVE_BALANCE) {
            EXPECT_GT(after, first);
            EXPECT_EQ(gBattleStruct->polishedMetronomeCount[B_BATTLER_0], 1);
        } else EXPECT_EQ(after, first);
    }
}

SINGLE_BATTLE_TEST("Polished move history Metronome counts the caller when its called move changes")
{
    s16 first, after;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_METRONOME); Moves(MOVE_METRONOME); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); HP(1000); MaxHP(1000); }
    } WHEN {
        TURN { MOVE(player, MOVE_METRONOME, WITH_RNG(RNG_METRONOME, MOVE_TACKLE)); }
        TURN { MOVE(player, MOVE_METRONOME, WITH_RNG(RNG_METRONOME, MOVE_GROWL)); }
        TURN { MOVE(player, MOVE_METRONOME, WITH_RNG(RNG_METRONOME, MOVE_TACKLE)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player);
        HP_BAR(opponent, captureDamage: &first);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_GROWL, player);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player);
        HP_BAR(opponent, captureDamage: &after);
    } THEN {
        if (B_POLISHED_MOVE_BALANCE) {
            EXPECT_GT(after, first);
            EXPECT_EQ(gBattleStruct->polishedMetronomeCount[B_BATTLER_0], 2);
        } else EXPECT_EQ(after, first);
    }
}

SINGLE_BATTLE_TEST("Polished move history Metronome saturates after five increments")
{
    s16 sixth, seventh, eighth;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_METRONOME); Attack(10); Moves(MOVE_TACKLE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Defense(100); HP(1000); MaxHP(1000); }
    } WHEN {
        for (u32 turn = 0; turn < 8; turn++) TURN { MOVE(player, MOVE_TACKLE); }
    } SCENE {
        for (u32 turn = 0; turn < 5; turn++) ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player); HP_BAR(opponent, captureDamage: &sixth);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player); HP_BAR(opponent, captureDamage: &seventh);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player); HP_BAR(opponent, captureDamage: &eighth);
    } THEN {
        EXPECT_EQ(sixth, seventh);
        EXPECT_EQ(seventh, eighth);
        if (B_POLISHED_MOVE_BALANCE) EXPECT_EQ(gBattleStruct->polishedMetronomeCount[B_BATTLER_0], 5);
    }
}

SINGLE_BATTLE_TEST("Polished move history Metronome resets for an executed different move and switching")
{
    bool32 switching = FALSE;
    PARAMETRIZE { switching = FALSE; }
    PARAMETRIZE { switching = TRUE; }
    s16 first, after;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_METRONOME); Moves(MOVE_TACKLE, MOVE_GROWL); }
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); HP(1000); MaxHP(1000); }
    } WHEN {
        TURN { MOVE(player, MOVE_TACKLE); }
        if (switching) {
            TURN { SWITCH(player, 1); }
            TURN { SWITCH(player, 0); }
        } else TURN { MOVE(player, MOVE_GROWL); }
        TURN { MOVE(player, MOVE_TACKLE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player); HP_BAR(opponent, captureDamage: &first);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player); HP_BAR(opponent, captureDamage: &after);
    } THEN { EXPECT_EQ(first, after); }
}

SINGLE_BATTLE_TEST("Polished move history Rage survives an interrupted repeat without starting on failure")
{
    bool32 prior = FALSE, protect = FALSE;
    for (u32 p = 0; p < 2; p++)
        for (u32 q = 0; q < 2; q++) PARAMETRIZE { prior = p; protect = q; }
    GIVEN {
        WITH_CONFIG(B_RAGE_BUILDS, GEN_4);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Moves(MOVE_RAGE); Speed(100); HP(1000); MaxHP(1000); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_BRIGHTPOWDER); Speed(50); HP(1000); MaxHP(1000); }
    } WHEN {
        if (prior) TURN { MOVE(player, MOVE_RAGE, hit: TRUE); }
        TURN { MOVE(player, MOVE_RAGE, hit: protect); if (protect) MOVE(opponent, MOVE_PROTECT); }
        TURN { MOVE(opponent, MOVE_QUICK_ATTACK); MOVE(player, MOVE_RAGE, hit: TRUE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_QUICK_ATTACK, opponent);
        if (B_POLISHED_MOVE_BALANCE && prior) MESSAGE("Wobbuffet's rage is building!");
        else NOT MESSAGE("Wobbuffet's rage is building!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_RAGE, player);
    } THEN { EXPECT_EQ(player->statStages[STAT_ATK], DEFAULT_STAT_STAGE + (B_POLISHED_MOVE_BALANCE && prior)); }
}

SINGLE_BATTLE_TEST("Polished move history using a player item clears Rage before the opponent attacks")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Moves(MOVE_RAGE); Speed(100); HP(500); MaxHP(1000); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); HP(1000); MaxHP(1000); }
    } WHEN {
        TURN { MOVE(player, MOVE_RAGE); }
        TURN { USE_ITEM(player, ITEM_POTION, partyIndex: 0); MOVE(opponent, MOVE_TACKLE); }
    } THEN {
        if (B_POLISHED_MOVE_BALANCE) {
            EXPECT_EQ((u32)player->volatiles.rage, FALSE);
            EXPECT_EQ(player->statStages[STAT_ATK], DEFAULT_STAT_STAGE);
        }
    }
}

SINGLE_BATTLE_TEST("Polished move history Magic Bounce preserves source charging through a later own action")
{
    bool32 enemy = FALSE;
    u32 otId = 1;
    enum Move nextMove = MOVE_TACKLE;
    for (u32 side = 0; side < 2; side++)
        for (u32 different = 0; different < 2; different++)
            PARAMETRIZE { enemy = side; nextMove = different ? MOVE_SCRATCH : MOVE_TACKLE; }
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    enum BattlerId battler = enemy ? B_BATTLER_1 : B_BATTLER_0;
    s16 first, after;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(enemy ? ABILITY_NONE : ABILITY_MAGIC_BOUNCE);
            Speed(enemy ? 100 : 50); HP(1000); MaxHP(1000);
            if (!enemy) { Item(ITEM_METRONOME); MovesWithPP({MOVE_TACKLE, 10}, {MOVE_SCRATCH, 10}); }
            else Moves(MOVE_SPLASH, MOVE_GROWL);
        }
        // The first player attack obeys explicitly. Its later own attack
        // must bypass obedience while the source ResetTurn flag remains set.
        if (B_POLISHED_MOVE_BALANCE && !enemy) SetMonData(&PLAYER_PARTY[0], MON_DATA_OT_ID, &otId);
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(enemy ? ABILITY_MAGIC_BOUNCE : ABILITY_NONE);
            Speed(enemy ? 50 : 100); HP(1000); MaxHP(1000);
            if (enemy) { Item(ITEM_METRONOME); MovesWithPP({MOVE_TACKLE, 10}, {MOVE_SCRATCH, 10}); }
            else Moves(MOVE_SPLASH, MOVE_GROWL);
        }
    } WHEN {
        TURN { MOVE(target, MOVE_SPLASH); MOVE(user, MOVE_TACKLE, WITH_RNG(RNG_POLISHED_OBEDIENCE_FIRST, 0)); }
        TURN { MOVE(target, MOVE_GROWL); MOVE(user, nextMove); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, user);
        HP_BAR(target, captureDamage: &first);
        ABILITY_POPUP(user, ABILITY_MAGIC_BOUNCE);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_GROWL, user);
        ANIMATION(ANIM_TYPE_MOVE, nextMove, user);
        HP_BAR(target, captureDamage: &after);
    } THEN {
        // bounceback saves/restores the move and turn direction, but its
        // ResetTurn leaves wCharging=1 until the next free selection.
        EXPECT_EQ(user->pp[0], B_POLISHED_MOVE_BALANCE || nextMove != MOVE_TACKLE ? 9 : 8);
        EXPECT_EQ(user->pp[1], B_POLISHED_MOVE_BALANCE || nextMove == MOVE_TACKLE ? 10 : 9);
        EXPECT_EQ(gLastMoves[battler], B_POLISHED_MOVE_BALANCE ? MOVE_TACKLE : nextMove);
        if (B_POLISHED_MOVE_BALANCE) {
            EXPECT_EQ(gBattleStruct->polishedMetronomeMove[battler], MOVE_TACKLE);
            EXPECT_EQ(gBattleStruct->polishedMetronomeCount[battler], 0);
            EXPECT_EQ(after, first);
        } else {
            // Native reflection records Growl as the resulting move, so
            // the later Tackle/Scratch resets its ordinary Metronome chain.
            EXPECT_EQ(after, first);
            EXPECT_EQ((u32)user->volatiles.metronomeItemCounter, 1);
        }
    }
}

SINGLE_BATTLE_TEST("Polished move history Metronome retains native boost when either side activates a gimmick", s16 damage)
{
    bool32 userTera = FALSE;
    enum Item item = ITEM_NONE;
    for (u32 side = 0; side < 2; side++)
        for (u32 held = 0; held < 2; held++)
            PARAMETRIZE { userTera = side; item = held ? ITEM_METRONOME : ITEM_NONE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(ABILITY_NONE); Level(50); Attack(100); Speed(100);
            TeraType(TYPE_PSYCHIC); Item(item); MovesWithPP({MOVE_TACKLE, 10});
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(ABILITY_NONE); Defense(101); Speed(50); HP(1000); MaxHP(1000);
            TeraType(TYPE_PSYCHIC); Moves(MOVE_SPLASH);
        }
    } WHEN {
        TURN { MOVE(player, MOVE_TACKLE); MOVE(opponent, MOVE_SPLASH); }
        TURN {
            MOVE(player, MOVE_TACKLE, gimmick: userTera ? GIMMICK_TERA : GIMMICK_NONE);
            MOVE(opponent, MOVE_SPLASH, gimmick: userTera ? GIMMICK_NONE : GIMMICK_TERA);
        }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player);
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } THEN {
        EXPECT_EQ(player->pp[0], 8);
        EXPECT_EQ((u32)player->volatiles.metronomeItemCounter, 2);
        EXPECT_EQ(gBattleStruct->polishedMetronomeMove[B_BATTLER_0], MOVE_NONE);
        EXPECT_EQ(gBattleStruct->polishedMetronomeCount[B_BATTLER_0], 0);
    } FINALLY {
        for (u32 side = 0; side < 2; side++)
            EXPECT_MUL_EQ(results[2 * side].damage, UQ_4_12(1.2), results[2 * side + 1].damage);
    }
}

SINGLE_BATTLE_TEST("Polished move history only enemy Baton Pass retains Metronome caller history")
{
    bool32 enemy = FALSE;
    PARAMETRIZE { enemy = FALSE; }
    PARAMETRIZE { enemy = TRUE; }
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    enum BattlerId battler = enemy ? B_BATTLER_1 : B_BATTLER_0;
    s16 first, after;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(ABILITY_NONE); Level(50); Attack(100); Defense(101); Speed(100); HP(1000); MaxHP(1000);
            if (!enemy) { Item(ITEM_METRONOME); MovesWithPP({MOVE_METRONOME, 10}); }
            else Moves(MOVE_SPLASH);
        }
        if (!enemy) PLAYER(SPECIES_WOBBUFFET) {
            Ability(ABILITY_NONE); Level(50); Attack(100); Speed(100);
            Item(ITEM_METRONOME); MovesWithPP({MOVE_METRONOME, 10});
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(ABILITY_NONE); Level(50); Attack(100); Defense(101); Speed(50); HP(1000); MaxHP(1000);
            if (enemy) { Item(ITEM_METRONOME); MovesWithPP({MOVE_METRONOME, 10}); }
            else Moves(MOVE_SPLASH);
        }
        if (enemy) OPPONENT(SPECIES_WOBBUFFET) {
            Ability(ABILITY_NONE); Level(50); Attack(100); Speed(50);
            Item(ITEM_METRONOME); MovesWithPP({MOVE_METRONOME, 10});
        }
    } WHEN {
        TURN { MOVE(user, MOVE_METRONOME, WITH_RNG(RNG_METRONOME, MOVE_TACKLE)); MOVE(target, MOVE_SPLASH); }
        TURN { MOVE(user, MOVE_METRONOME, WITH_RNG(RNG_METRONOME, MOVE_BATON_PASS)); SEND_OUT(user, 1); MOVE(target, MOVE_SPLASH); }
        TURN { MOVE(user, MOVE_METRONOME, WITH_RNG(RNG_METRONOME, MOVE_TACKLE)); MOVE(target, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, user);
        HP_BAR(target, captureDamage: &first);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_BATON_PASS, user);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, user);
        HP_BAR(target, captureDamage: &after);
    } THEN {
        EXPECT_EQ(user->pp[0], 9);
        if (B_POLISHED_MOVE_BALANCE) {
            EXPECT_EQ(gBattleStruct->polishedMetronomeMove[battler], MOVE_METRONOME);
            EXPECT_EQ(gBattleStruct->polishedMetronomeCount[battler], enemy ? 2 : 0);
            if (enemy) EXPECT_GT(after, first);
            else EXPECT_EQ(after, first);
        } else EXPECT_EQ(after, first);
    }
}
