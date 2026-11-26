#include "global.h"
#include "test/battle.h"
#include "battle_script_commands.h"

SINGLE_BATTLE_TEST("Polished Sketch replaces the selected slot and persists base PP on either side")
{
    struct BattlePokemon *user = player, *other = opponent;
    u32 slot = 0;
    enum Move copied = MOVE_GROWL;
    for (u32 side = 0; side < 2; side++)
        for (u32 s = 0; s < MAX_MON_MOVES; s++)
            PARAMETRIZE { user = side ? opponent : player; other = side ? player : opponent; slot = s; copied = s & 1 ? MOVE_ROCK_BLAST : MOVE_GROWL; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); MaxHP(1000); HP(1000); Speed(user == player ? 50 : 100); if (user == player) Moves(slot == 0 ? MOVE_SKETCH : MOVE_SPLASH, slot == 1 ? MOVE_SKETCH : MOVE_PROTECT, slot == 2 ? MOVE_SKETCH : MOVE_REST, slot == 3 ? MOVE_SKETCH : MOVE_SLEEP_TALK); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); MaxHP(1000); HP(1000); Speed(user == opponent ? 50 : 100); if (user == opponent) Moves(slot == 0 ? MOVE_SKETCH : MOVE_SPLASH, slot == 1 ? MOVE_SKETCH : MOVE_PROTECT, slot == 2 ? MOVE_SKETCH : MOVE_REST, slot == 3 ? MOVE_SKETCH : MOVE_SLEEP_TALK); }
    } WHEN {
        TURN { MOVE(other, copied); MOVE(user, moveSlot: slot); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, copied, other);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SKETCH, user);
    } THEN {
        struct Pokemon *mon = GetBattlerMon(user == player ? B_BATTLER_0 : B_BATTLER_1);
        EXPECT_EQ(user->moves[slot], copied);
        EXPECT_EQ(user->pp[slot], GetMovePP(copied));
        EXPECT_EQ(GetMonData(mon, MON_DATA_MOVE1 + slot), copied);
        EXPECT_EQ(GetMonData(mon, MON_DATA_PP1 + slot), GetMovePP(copied));
        if (B_POLISHED_MOVE_BALANCE) EXPECT_EQ(gLastMoves[user == player ? B_BATTLER_0 : B_BATTLER_1], MOVE_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Sketch Substitute rejection honors effective Infiltrator on either side")
{
    bool32 enemy = FALSE, infiltrator = FALSE, gas = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 bypass = 0; bypass < 2; bypass++)
            for (u32 suppressed = 0; suppressed < 2; suppressed++)
                PARAMETRIZE { enemy = side; infiltrator = bypass; gas = suppressed; }
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    bool32 copied = !B_POLISHED_MOVE_BALANCE || (infiltrator && !gas);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(enemy ? (gas ? ABILITY_NEUTRALIZING_GAS : ABILITY_NONE) : (infiltrator ? ABILITY_INFILTRATOR : ABILITY_NONE));
            Speed(enemy ? 100 : 50); if (!enemy) Moves(MOVE_SKETCH);
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(!enemy ? (gas ? ABILITY_NEUTRALIZING_GAS : ABILITY_NONE) : (infiltrator ? ABILITY_INFILTRATOR : ABILITY_NONE));
            Speed(enemy ? 50 : 100); if (enemy) Moves(MOVE_SKETCH);
        }
    } WHEN {
        TURN { MOVE(target, MOVE_SUBSTITUTE); MOVE(user, MOVE_SKETCH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SUBSTITUTE, target);
        if (copied) ANIMATION(ANIM_TYPE_MOVE, MOVE_SKETCH, user);
        else NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_SKETCH, user);
    } THEN {
        EXPECT_EQ(user->moves[0], copied ? MOVE_SUBSTITUTE : MOVE_SKETCH);
        EXPECT_EQ(user->pp[0], copied ? GetMovePP(MOVE_SUBSTITUTE) : 0);
    }
}

SINGLE_BATTLE_TEST("Polished Sketch cannot copy cleared Sleep Talk or Metronome history")
{
    enum Move caller = MOVE_SLEEP_TALK;
    struct BattlePokemon *user = player, *other = opponent;
    for (u32 side = 0; side < 2; side++)
        for (u32 talk = 0; talk < 2; talk++)
            PARAMETRIZE { user = side ? opponent : player; other = side ? player : opponent; caller = talk ? MOVE_SLEEP_TALK : MOVE_METRONOME; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); MaxHP(1000); HP(1000); Speed(user == player ? 50 : 100); if (user == player) Moves(MOVE_SKETCH); else { Moves(caller, MOVE_TACKLE); if (caller == MOVE_SLEEP_TALK) Status1(STATUS1_SLEEP_TURN(3)); } }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); MaxHP(1000); HP(1000); Speed(user == opponent ? 50 : 100); if (user == opponent) Moves(MOVE_SKETCH); else { Moves(caller, MOVE_TACKLE); if (caller == MOVE_SLEEP_TALK) Status1(STATUS1_SLEEP_TURN(3)); } }
    } WHEN {
        if (caller == MOVE_METRONOME)
            TURN { MOVE(other, caller, WITH_RNG(RNG_METRONOME, B_POLISHED_MOVE_BALANCE ? 249 : MOVE_BRICK_BREAK)); MOVE(user, MOVE_SKETCH); }
        else
            TURN { MOVE(other, caller); MOVE(user, MOVE_SKETCH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, caller, other);
        if (B_POLISHED_MOVE_BALANCE) NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_SKETCH, user);
        else ANIMATION(ANIM_TYPE_MOVE, MOVE_SKETCH, user);
    } THEN {
        EXPECT_EQ(user->moves[0], B_POLISHED_MOVE_BALANCE ? MOVE_SKETCH : caller);
    }
}

SINGLE_BATTLE_TEST("Polished Sketch rejects absent history and already known moves")
{
    bool32 duplicate = FALSE;
    PARAMETRIZE { duplicate = FALSE; }
    PARAMETRIZE { duplicate = TRUE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); Moves(MOVE_SKETCH, MOVE_GROWL); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); if (!duplicate) Status1(STATUS1_SLEEP_TURN(3)); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_GROWL); MOVE(player, MOVE_SKETCH); }
    } SCENE {
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_SKETCH, player);
    } THEN {
        EXPECT_EQ(player->moves[0], MOVE_SKETCH);
        EXPECT_EQ(player->pp[0], 0);
        if (B_POLISHED_MOVE_BALANCE) EXPECT_EQ(gLastMoves[B_BATTLER_0], MOVE_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Sketch can copy an ordinary failed move")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); Moves(MOVE_SKETCH); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_RECOVER); MOVE(player, MOVE_SKETCH); }
    } SCENE {
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_RECOVER, opponent);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SKETCH, player);
    } THEN {
        EXPECT_EQ(player->moves[0], MOVE_RECOVER);
        EXPECT_EQ(player->pp[0], GetMovePP(MOVE_RECOVER));
    }
}

SINGLE_BATTLE_TEST("Polished Sketch retains the last executed move when the target cannot act")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); Moves(MOVE_SPORE, MOVE_SKETCH); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_GROWL); MOVE(player, MOVE_SPORE); }
        TURN { MOVE(opponent, MOVE_TACKLE); MOVE(player, MOVE_SKETCH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_GROWL, opponent);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SPORE, player);
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, opponent);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SKETCH, player);
    } THEN {
        EXPECT_EQ(player->moves[1], MOVE_GROWL);
    }
}

SINGLE_BATTLE_TEST("Polished Sketch cannot be used by a transformed battler")
{
    GIVEN {
        PLAYER(SPECIES_DITTO) { Ability(ABILITY_LIMBER); Speed(50); Moves(MOVE_TRANSFORM); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); Moves(MOVE_SKETCH, MOVE_SPLASH); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_SPLASH); MOVE(player, MOVE_TRANSFORM); }
        TURN { MOVE(opponent, MOVE_SPLASH); MOVE(player, moveSlot: 0); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TRANSFORM, player);
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_SKETCH, player);
    } THEN {
        EXPECT_EQ(player->moves[0], MOVE_SKETCH);
        EXPECT_EQ(GetMonData(GetBattlerMon(B_BATTLER_0), MON_DATA_MOVE1), MOVE_TRANSFORM);
    }
}

SINGLE_BATTLE_TEST("Polished Sketch copies survive switching and spend PP from the learned slot")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); Moves(MOVE_SKETCH, MOVE_SPLASH); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(70); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_GROWL); MOVE(player, MOVE_SKETCH); }
        TURN { SWITCH(player, 1); }
        TURN { SWITCH(player, 0); }
        TURN { MOVE(player, moveSlot: 0); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SKETCH, player);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_GROWL, player);
    } THEN {
        EXPECT_EQ(player->moves[0], MOVE_GROWL);
        EXPECT_EQ(player->pp[0], GetMovePP(MOVE_GROWL) - 1);
        EXPECT_EQ(GetMonData(GetBattlerMon(B_BATTLER_0), MON_DATA_PP1), GetMovePP(MOVE_GROWL) - 1);
    }
}

SINGLE_BATTLE_TEST("Polished Sketch preserves other move PP and PP Up bonuses")
{
    u32 bonuses = 0xfc;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); MovesWithPP({MOVE_SKETCH, 1}, {MOVE_TACKLE, 3}, {MOVE_REST, 2}, {MOVE_SLEEP_TALK, 0}); }
        SetMonData(&PLAYER_PARTY[0], MON_DATA_PP_BONUSES, &bonuses);
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_GROWL); MOVE(player, MOVE_SKETCH); }
    } THEN {
        EXPECT_EQ(player->ppBonuses, bonuses);
        EXPECT_EQ(GetMonData(GetBattlerMon(B_BATTLER_0), MON_DATA_PP_BONUSES), bonuses);
        EXPECT_EQ(player->pp[0], GetMovePP(MOVE_GROWL));
        EXPECT_EQ(player->pp[1], 3);
        EXPECT_EQ(player->pp[2], 2);
        EXPECT_EQ(player->pp[3], 0);
    }
}

SINGLE_BATTLE_TEST("Polished Sketch rejects Struggle after the target exhausts every PP")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); Moves(MOVE_SKETCH); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); MovesWithPP({MOVE_TACKLE, 0}); }
    } WHEN {
        TURN { FORCED_MOVE(opponent); MOVE(player, MOVE_SKETCH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_STRUGGLE, opponent);
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_SKETCH, player);
    } THEN {
        EXPECT_EQ(player->moves[0], MOVE_SKETCH);
        EXPECT_EQ(player->pp[0], 0);
    }
}

SINGLE_BATTLE_TEST("Polished Sketch cannot copy a failed awake Sleep Talk")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); Moves(MOVE_SKETCH); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); Moves(MOVE_SLEEP_TALK, MOVE_TACKLE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_SLEEP_TALK); MOVE(player, MOVE_SKETCH); }
    } SCENE {
        if (B_POLISHED_MOVE_BALANCE) {
            NONE_OF {
                ANIMATION(ANIM_TYPE_MOVE, MOVE_SLEEP_TALK, opponent);
                ANIMATION(ANIM_TYPE_MOVE, MOVE_SKETCH, player);
            }
        } else {
            NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_SLEEP_TALK, opponent);
            ANIMATION(ANIM_TYPE_MOVE, MOVE_SKETCH, player);
        }
    } THEN {
        EXPECT_EQ(player->moves[0], B_POLISHED_MOVE_BALANCE ? MOVE_SKETCH : MOVE_SLEEP_TALK);
    }
}

DOUBLE_BATTLE_TEST("Polished Sketch retains native Substitute behavior in doubles")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); Moves(MOVE_SKETCH); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(70); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(80); }
    } WHEN {
        TURN { MOVE(opponentLeft, MOVE_SUBSTITUTE); MOVE(playerLeft, MOVE_SKETCH, target: opponentLeft); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SUBSTITUTE, opponentLeft);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SKETCH, playerLeft);
    } THEN {
        EXPECT_EQ(playerLeft->moves[0], MOVE_SUBSTITUTE);
    }
}

SINGLE_BATTLE_TEST("Polished Sketch retains native behavior with either active gimmick")
{
    bool32 userTera = FALSE;
    PARAMETRIZE { userTera = FALSE; }
    PARAMETRIZE { userTera = TRUE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); Moves(MOVE_SKETCH, MOVE_SPLASH); TeraType(TYPE_PSYCHIC); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); TeraType(TYPE_PSYCHIC); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_SUBSTITUTE, gimmick: userTera ? GIMMICK_NONE : GIMMICK_TERA); MOVE(player, MOVE_SPLASH, gimmick: userTera ? GIMMICK_TERA : GIMMICK_NONE); }
        TURN { MOVE(opponent, MOVE_SUBSTITUTE); MOVE(player, MOVE_SKETCH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SUBSTITUTE, opponent);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SKETCH, player);
    } THEN {
        EXPECT_EQ(player->moves[0], MOVE_SUBSTITUTE);
    }
}

// Seed otherwise unreachable states at the production Sketch command boundary.
// Source callers exclude Sketch, and Sketch rejects transformed users. The
// party-slot guard and lock remapping also protect optional content and save data.
static void PrepareSketchBoundary(void)
{
    gBattlerAttacker = B_BATTLER_0;
    gBattlerTarget = B_BATTLER_1;
    gCurrMovePos = 0;
    gLastMoves[B_BATTLER_1] = MOVE_GROWL;
}

SINGLE_BATTLE_TEST("Polished Sketch boundary keeps a borrowed move temporary and other party slots intact")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); MovesWithPP({MOVE_TACKLE, 3}, {MOVE_REST, 2}); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN { TURN {} }
    THEN {
        PrepareSketchBoundary();
        player->moves[0] = MOVE_SKETCH;
        player->moves[1] = MOVE_SPLASH;
        player->pp[1] = 5;
        EXPECT(TryPolishedSketch());
        EXPECT_EQ(player->moves[0], MOVE_GROWL);
        EXPECT_EQ(player->pp[0], GetMovePP(MOVE_GROWL));
        EXPECT_EQ((u32)(player->volatiles.mimickedMoves & 1), 1);
        EXPECT_EQ(GetMonData(GetBattlerMon(B_BATTLER_0), MON_DATA_MOVE1), MOVE_TACKLE);
        EXPECT_EQ(GetMonData(GetBattlerMon(B_BATTLER_0), MON_DATA_PP1), 3);
        EXPECT_EQ(GetMonData(GetBattlerMon(B_BATTLER_0), MON_DATA_MOVE2), MOVE_REST);
        EXPECT_EQ(GetMonData(GetBattlerMon(B_BATTLER_0), MON_DATA_PP2), 2);
    }
}

SINGLE_BATTLE_TEST("Polished Sketch boundary remaps locks by identity and clears only copied slot PP bonuses")
{
    bool32 retain = FALSE;
    PARAMETRIZE { retain = FALSE; }
    PARAMETRIZE { retain = TRUE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Moves(MOVE_SKETCH, MOVE_REST); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN { TURN {} }
    THEN {
        u32 bonuses = 0xff;
        PrepareSketchBoundary();
        SetMonData(GetBattlerMon(B_BATTLER_0), MON_DATA_PP_BONUSES, &bonuses);
        player->ppBonuses = bonuses;
        player->volatiles.disabledMove = retain ? MOVE_REST : MOVE_SKETCH;
        player->volatiles.disableTimer = 3;
        player->volatiles.encoredMove = retain ? MOVE_REST : MOVE_SKETCH;
        player->volatiles.encoreTimer = 2;
        player->volatiles.encoredMovePos = 0;
        gBattleStruct->choicedMove[B_BATTLER_0] = retain ? MOVE_REST : MOVE_SKETCH;
        EXPECT(TryPolishedSketch());
        EXPECT_EQ(player->ppBonuses, 0xfc);
        EXPECT_EQ(GetMonData(GetBattlerMon(B_BATTLER_0), MON_DATA_PP_BONUSES), 0xfc);
        EXPECT_EQ((u32)player->volatiles.disabledMove, retain ? MOVE_REST : MOVE_NONE);
        EXPECT_EQ((u32)player->volatiles.disableTimer, retain ? 3 : 0);
        EXPECT_EQ((u32)player->volatiles.encoredMove, retain ? MOVE_REST : MOVE_NONE);
        EXPECT_EQ((u32)player->volatiles.encoreTimer, retain ? 2 : 0);
        EXPECT_EQ((u32)player->volatiles.encoredMovePos, retain ? 1 : 0);
        EXPECT_EQ(gBattleStruct->choicedMove[B_BATTLER_0], retain ? MOVE_REST : MOVE_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Sketch boundary rejects link mode and invalid or duplicate Sketch history without mutation")
{
    u32 kind = 0;
    for (u32 k = 0; k < 4; k++) PARAMETRIZE { kind = k; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Moves(MOVE_SKETCH); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN { TURN {} }
    THEN {
        u32 flags = gBattleTypeFlags;
        PrepareSketchBoundary();
        gLastMoves[B_BATTLER_0] = MOVE_SKETCH;
        if (kind == 0) gBattleTypeFlags |= BATTLE_TYPE_LINK;
        else gLastMoves[B_BATTLER_1] = kind == 1 ? MOVE_SKETCH : kind == 2 ? MOVE_UNAVAILABLE : MOVES_COUNT;
        EXPECT(!TryPolishedSketch());
        gBattleTypeFlags = flags;
        EXPECT_EQ(gLastMoves[B_BATTLER_0], MOVE_NONE);
        EXPECT_EQ(player->moves[0], MOVE_SKETCH);
        EXPECT_EQ(player->pp[0], 1);
        EXPECT_EQ(GetMonData(GetBattlerMon(B_BATTLER_0), MON_DATA_MOVE1), MOVE_SKETCH);
    }
}

SINGLE_BATTLE_TEST("Polished Sketch clears Choice locks only after replacing the locked move")
{
    enum Item item = ITEM_CHOICE_BAND;
    bool32 success = FALSE;
    for (u32 k = 0; k < 3; k++)
        for (u32 ok = 0; ok < 2; ok++)
            PARAMETRIZE { item = k == 0 ? ITEM_CHOICE_BAND : k == 1 ? ITEM_CHOICE_SPECS : ITEM_CHOICE_SCARF; success = ok; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(item); Speed(25); Moves(MOVE_SKETCH, MOVE_RECOVER); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); }
    } WHEN {
        TURN { MOVE(opponent, success ? MOVE_GROWL : MOVE_SUBSTITUTE); MOVE(player, MOVE_SKETCH); }
    } THEN {
        if (B_POLISHED_MOVE_BALANCE) {
            EXPECT_EQ(player->moves[0], success ? MOVE_GROWL : MOVE_SKETCH);
            EXPECT_EQ(gBattleStruct->choicedMove[B_BATTLER_0], success ? MOVE_NONE : MOVE_SKETCH);
            EXPECT_EQ(CheckMoveLimitations(B_BATTLER_0, 0, MOVE_LIMITATION_CHOICE_ITEM) & (1u << 1), success ? 0 : (1u << 1));
        } else {
            EXPECT_EQ(gBattleStruct->choicedMove[B_BATTLER_0], MOVE_SKETCH);
        }
    }
}
