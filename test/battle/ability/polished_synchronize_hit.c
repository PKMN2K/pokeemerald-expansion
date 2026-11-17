#include "global.h"
#include "test/battle.h"

// Independent source pool: Tri Attack has two reflectable outcomes.
static const struct { enum Move move; u32 status; } sSyncHitCases[] = {
    {MOVE_FIRE_PUNCH, STATUS1_BURN}, {MOVE_THUNDERPUNCH, STATUS1_PARALYSIS},
    {MOVE_FLARE_BLITZ, STATUS1_BURN}, {MOVE_BODY_SLAM, STATUS1_PARALYSIS},
    {MOVE_POISON_STING, STATUS1_POISON}, {MOVE_EMBER, STATUS1_BURN},
    {MOVE_FLAMETHROWER, STATUS1_BURN}, {MOVE_THUNDERSHOCK, STATUS1_PARALYSIS},
    {MOVE_THUNDERBOLT, STATUS1_PARALYSIS}, {MOVE_THUNDER, STATUS1_PARALYSIS},
    {MOVE_SCALD, STATUS1_BURN}, {MOVE_LICK, STATUS1_PARALYSIS},
    {MOVE_GUNK_SHOT, STATUS1_POISON}, {MOVE_FIRE_BLAST, STATUS1_BURN},
    {MOVE_POISON_JAB, STATUS1_POISON}, {MOVE_TRI_ATTACK, STATUS1_BURN},
    {MOVE_TRI_ATTACK, STATUS1_PARALYSIS}, {MOVE_SLUDGE_BOMB, STATUS1_POISON},
    {MOVE_ZAP_CANNON, STATUS1_PARALYSIS}, {MOVE_SPARK, STATUS1_PARALYSIS},
    {MOVE_SACRED_FIRE, STATUS1_BURN}, {MOVE_DRAGONBREATH, STATUS1_PARALYSIS},
};
static const enum Move sSyncHitExamples[] = {MOVE_POISON_JAB, MOVE_FIRE_PUNCH, MOVE_THUNDERPUNCH};

static u32 SyncHitStatus(enum Move move)
{
    for (u32 n = 0; n < ARRAY_COUNT(sSyncHitCases); n++)
        if (sSyncHitCases[n].move == move)
            return sSyncHitCases[n].status;
    return STATUS1_NONE;
}

SINGLE_BATTLE_TEST("Polished Synchronize hit protects Substitute users for every source secondary status")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_EMBER;
    u32 status = STATUS1_BURN;
    bool32 sub = FALSE;
    for (u32 m = 0; m < ARRAY_COUNT(sSyncHitCases); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 shield = 0; shield < 2; shield++)
                PARAMETRIZE { move = sSyncHitCases[m].move; status = sSyncHitCases[m].status; user = side ? opponent : player; target = side ? player : opponent; sub = shield; }
    bool32 blocked = B_POLISHED_MOVE_BALANCE && sub;
    // RNG_RANDOM_FROM_LIST selects an index, not a MoveEffect value.
    u32 triStatus = status == STATUS1_BURN ? 0 : 1;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(user == player ? ABILITY_NONE : ABILITY_SYNCHRONIZE); MovesWithPP({move, 2}, {MOVE_SUBSTITUTE, 2}, {MOVE_CELEBRATE, 10}); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(user == opponent ? ABILITY_NONE : ABILITY_SYNCHRONIZE); MovesWithPP({move, 2}, {MOVE_SUBSTITUTE, 2}, {MOVE_CELEBRATE, 10}); }
    } WHEN {
        if (sub) TURN { MOVE(user, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, move, hit: TRUE, secondaryEffect: TRUE, WITH_RNG(RNG_RANDOM_FROM_LIST, triStatus)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, user);
        HP_BAR(target);
        STATUS_ICON(target, status);
        ABILITY_POPUP(target, ABILITY_SYNCHRONIZE);
        if (blocked) NOT STATUS_ICON(user, status);
        else STATUS_ICON(user, status);
    } THEN {
        EXPECT_EQ(user->status1, blocked ? STATUS1_NONE : status);
        EXPECT_EQ(target->status1, status);
        EXPECT_LT(target->hp, 1000);
        EXPECT_EQ(user->pp[0], 1);
        EXPECT_EQ((u32)user->volatiles.substituteHP, sub ? 250 : 0);
    }
}

SINGLE_BATTLE_TEST("Polished Synchronize hit does not return Tri Attack freeze")
{
    struct BattlePokemon *user = player, *target = opponent;
    bool32 sub = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 shield = 0; shield < 2; shield++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; sub = shield; }
    GIVEN {
        // The holder acts first so later thawing is outside this return check.
        PLAYER(SPECIES_WOBBUFFET) { Speed(user == player ? 50 : 100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(user == player ? ABILITY_NONE : ABILITY_SYNCHRONIZE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(user == opponent ? 50 : 100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(user == opponent ? ABILITY_NONE : ABILITY_SYNCHRONIZE); }
    } WHEN {
        if (sub) TURN { MOVE(user, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, MOVE_TRI_ATTACK, secondaryEffect: TRUE, WITH_RNG(RNG_RANDOM_FROM_LIST, 2)); MOVE(target, MOVE_CELEBRATE); }
    } SCENE {
        FREEZE_OR_FROSTBURN_STATUS(target, TRUE);
        NOT ABILITY_POPUP(target, ABILITY_SYNCHRONIZE);
    } THEN {
        EXPECT_EQ(user->status1, STATUS1_NONE);
        EXPECT_EQ(target->status1, B_USE_FROSTBITE ? STATUS1_FROSTBITE : STATUS1_FREEZE);
        EXPECT_EQ((u32)user->volatiles.substituteHP, sub ? 250 : 0);
    }
}

SINGLE_BATTLE_TEST("Polished Synchronize hit uses the reflector ability for the return")
{
    static const enum Ability abilities[] = {ABILITY_INFILTRATOR, ABILITY_CORROSION, ABILITY_MOLD_BREAKER};
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_POISON_JAB;
    enum Ability ability = ABILITY_INFILTRATOR;
    for (u32 m = 0; m < ARRAY_COUNT(sSyncHitExamples); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 a = 0; a < ARRAY_COUNT(abilities); a++)
                PARAMETRIZE { move = sSyncHitExamples[m]; user = side ? opponent : player; target = side ? player : opponent; ability = abilities[a]; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(user == player ? ability : ABILITY_SYNCHRONIZE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(user == opponent ? ability : ABILITY_SYNCHRONIZE); }
    } WHEN {
        TURN { MOVE(user, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, move, secondaryEffect: TRUE); }
    } SCENE {
        ABILITY_POPUP(target, ABILITY_SYNCHRONIZE);
    } THEN {
        EXPECT_EQ(user->status1, B_POLISHED_MOVE_BALANCE ? STATUS1_NONE : SyncHitStatus(move));
        EXPECT_EQ(target->status1, SyncHitStatus(move));
        EXPECT_EQ((u32)user->volatiles.substituteHP, 250);
    }
}

SINGLE_BATTLE_TEST("Polished Synchronize hit separates outgoing Substitute protection and Infiltrator")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_POISON_JAB;
    bool32 infiltrator = FALSE;
    for (u32 m = 0; m < ARRAY_COUNT(sSyncHitExamples); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 bypass = 0; bypass < 2; bypass++)
                PARAMETRIZE { move = sSyncHitExamples[m]; user = side ? opponent : player; target = side ? player : opponent; infiltrator = bypass; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(user == player ? infiltrator ? ABILITY_INFILTRATOR : ABILITY_NONE : ABILITY_SYNCHRONIZE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(user == opponent ? infiltrator ? ABILITY_INFILTRATOR : ABILITY_NONE : ABILITY_SYNCHRONIZE); }
    } WHEN {
        TURN { MOVE(user, MOVE_SUBSTITUTE); MOVE(target, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, move, secondaryEffect: TRUE); }
    } SCENE {
        if (infiltrator) ABILITY_POPUP(target, ABILITY_SYNCHRONIZE);
        else NOT ABILITY_POPUP(target, ABILITY_SYNCHRONIZE);
    } THEN {
        EXPECT_EQ(user->status1, infiltrator && !B_POLISHED_MOVE_BALANCE ? SyncHitStatus(move) : STATUS1_NONE);
        EXPECT_EQ(target->status1, infiltrator ? SyncHitStatus(move) : STATUS1_NONE);
        EXPECT_EQ((u32)user->volatiles.substituteHP, 250);
        if (infiltrator) EXPECT_EQ((u32)target->volatiles.substituteHP, 250);
        else EXPECT_LT((u32)target->volatiles.substituteHP, 250);
    }
}

SINGLE_BATTLE_TEST("Polished Synchronize hit preserves attacker berries and checks return before reflector healing")
{
    struct BattlePokemon *user = player, *target = opponent, *holder = player;
    enum Move move = MOVE_POISON_JAB;
    bool32 sub = FALSE;
    for (u32 m = 0; m < ARRAY_COUNT(sSyncHitExamples); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 berry = 0; berry < 2; berry++)
                for (u32 shield = 0; shield < 2; shield++)
                    PARAMETRIZE { move = sSyncHitExamples[m]; user = side ? opponent : player; target = side ? player : opponent; holder = berry ? target : user; sub = shield; }
    bool32 blocked = B_POLISHED_MOVE_BALANCE && sub;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(user == player ? ABILITY_NONE : ABILITY_SYNCHRONIZE); Item(holder == player ? ITEM_LUM_BERRY : ITEM_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(user == opponent ? ABILITY_NONE : ABILITY_SYNCHRONIZE); Item(holder == opponent ? ITEM_LUM_BERRY : ITEM_NONE); }
    } WHEN {
        if (sub) TURN { MOVE(user, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, move, secondaryEffect: TRUE); }
    } SCENE {
        STATUS_ICON(target, SyncHitStatus(move));
        ABILITY_POPUP(target, ABILITY_SYNCHRONIZE);
        if (holder == target) {
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, target);
            STATUS_ICON(target, none: TRUE);
        }
    } THEN {
        EXPECT_EQ(user->status1, blocked || holder == user ? STATUS1_NONE : SyncHitStatus(move));
        EXPECT_EQ(target->status1, holder == target ? STATUS1_NONE : SyncHitStatus(move));
        EXPECT_EQ(holder->item, blocked && holder == user ? ITEM_LUM_BERRY : ITEM_NONE);
        EXPECT_EQ((u32)user->volatiles.substituteHP, sub ? 250 : 0);
    }
}

SINGLE_BATTLE_TEST("Polished Synchronize hit respects ability suppression")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_POISON_JAB;
    for (u32 m = 0; m < ARRAY_COUNT(sSyncHitExamples); m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = sSyncHitExamples[m]; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(user == player ? ABILITY_NONE : ABILITY_SYNCHRONIZE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(user == opponent ? ABILITY_NONE : ABILITY_SYNCHRONIZE); }
    } WHEN {
        TURN { MOVE(user, MOVE_GASTRO_ACID); }
        TURN { MOVE(user, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, move, secondaryEffect: TRUE); }
    } SCENE {
        NOT ABILITY_POPUP(target, ABILITY_SYNCHRONIZE);
    } THEN {
        EXPECT_EQ(user->status1, STATUS1_NONE);
        EXPECT_EQ(target->status1, SyncHitStatus(move));
    }
}

SINGLE_BATTLE_TEST("Polished Synchronize hit does not return a failed secondary roll")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_EMBER;
    for (u32 m = 0; m < ARRAY_COUNT(sSyncHitCases); m++)
        if (sSyncHitCases[m].move != MOVE_ZAP_CANNON && !(sSyncHitCases[m].move == MOVE_TRI_ATTACK && sSyncHitCases[m].status == STATUS1_PARALYSIS))
            for (u32 side = 0; side < 2; side++)
                PARAMETRIZE { move = sSyncHitCases[m].move; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(user == player ? ABILITY_NONE : ABILITY_SYNCHRONIZE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(user == opponent ? ABILITY_NONE : ABILITY_SYNCHRONIZE); }
    } WHEN {
        TURN { MOVE(user, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, move, hit: TRUE, secondaryEffect: FALSE); }
    } SCENE {
        HP_BAR(target);
        NOT ABILITY_POPUP(target, ABILITY_SYNCHRONIZE);
    } THEN {
        EXPECT_EQ(user->status1, STATUS1_NONE);
        EXPECT_EQ(target->status1, STATUS1_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Synchronize hit respects missed and protected attacks")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_POISON_JAB;
    bool32 protect = FALSE;
    for (u32 m = 0; m < ARRAY_COUNT(sSyncHitExamples); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 shield = 0; shield < 2; shield++)
                PARAMETRIZE { move = sSyncHitExamples[m]; user = side ? opponent : player; target = side ? player : opponent; protect = shield; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(user == player ? ABILITY_NONE : ABILITY_SYNCHRONIZE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(user == opponent ? ABILITY_NONE : ABILITY_SYNCHRONIZE); }
    } WHEN {
        // These moves have 100 accuracy; lower it so a miss roll is consulted.
        TURN { MOVE(target, MOVE_SAND_ATTACK); }
        TURN { MOVE(user, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, move, hit: protect, secondaryEffect: TRUE); MOVE(target, protect ? MOVE_PROTECT : MOVE_CELEBRATE); }
    } SCENE {
        NOT ABILITY_POPUP(target, ABILITY_SYNCHRONIZE);
    } THEN {
        EXPECT_EQ(user->status1, STATUS1_NONE);
        EXPECT_EQ(target->status1, STATUS1_NONE);
        EXPECT_EQ(target->hp, 1000);
    }
}

DOUBLE_BATTLE_TEST("Polished Synchronize hit leaves doubles native")
{
    struct BattlePokemon *user = playerLeft, *target = opponentLeft;
    enum Move move = MOVE_POISON_JAB;
    for (u32 m = 0; m < ARRAY_COUNT(sSyncHitExamples); m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = sSyncHitExamples[m]; user = side ? opponentLeft : playerLeft; target = side ? playerLeft : opponentLeft; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); MaxHP(1000); HP(1000); Attack(10); Ability(user == playerLeft ? ABILITY_NONE : ABILITY_SYNCHRONIZE); }
        PLAYER(SPECIES_WYNAUT) { Speed(75); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); MaxHP(1000); HP(1000); Attack(10); Ability(user == opponentLeft ? ABILITY_NONE : ABILITY_SYNCHRONIZE); }
        OPPONENT(SPECIES_WYNAUT) { Speed(25); Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(user, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, move, target: target, secondaryEffect: TRUE); }
    } THEN {
        EXPECT_EQ(user->status1, SyncHitStatus(move));
        EXPECT_EQ(target->status1, SyncHitStatus(move));
    }
}

SINGLE_BATTLE_TEST("Polished Synchronize hit leaves either active Terastal native")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_POISON_JAB;
    bool32 userTera = FALSE;
    for (u32 m = 0; m < ARRAY_COUNT(sSyncHitExamples); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 tera = 0; tera < 2; tera++)
                PARAMETRIZE { move = sSyncHitExamples[m]; user = side ? opponent : player; target = side ? player : opponent; userTera = tera; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(user == player ? ABILITY_NONE : ABILITY_SYNCHRONIZE); TeraType(TYPE_NORMAL); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(user == opponent ? ABILITY_NONE : ABILITY_SYNCHRONIZE); TeraType(TYPE_NORMAL); }
    } WHEN {
        TURN { MOVE(user, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, move, secondaryEffect: TRUE, gimmick: userTera ? GIMMICK_TERA : GIMMICK_NONE); MOVE(target, MOVE_CELEBRATE, gimmick: userTera ? GIMMICK_NONE : GIMMICK_TERA); }
    } THEN {
        EXPECT_EQ(user->status1, SyncHitStatus(move));
        EXPECT_EQ(target->status1, SyncHitStatus(move));
    }
}

SINGLE_BATTLE_TEST("Polished Synchronize hit leaves dynamic retyping native")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_POISON_JAB;
    for (u32 m = 0; m < ARRAY_COUNT(sSyncHitExamples); m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = sSyncHitExamples[m]; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(user == player ? ABILITY_NORMALIZE : ABILITY_SYNCHRONIZE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(user == opponent ? ABILITY_NORMALIZE : ABILITY_SYNCHRONIZE); }
    } WHEN {
        TURN { MOVE(user, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, move, secondaryEffect: TRUE); }
    } THEN {
        EXPECT_EQ(user->status1, SyncHitStatus(move));
        EXPECT_EQ(target->status1, SyncHitStatus(move));
    }
}

SINGLE_BATTLE_TEST("Polished Synchronize hit keeps Poison Touch separate from the move secondary status")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_POISON_JAB;
    for (u32 m = 0; m < ARRAY_COUNT(sSyncHitExamples); m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = sSyncHitExamples[m]; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(user == player ? ABILITY_POISON_TOUCH : ABILITY_SYNCHRONIZE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(user == opponent ? ABILITY_POISON_TOUCH : ABILITY_SYNCHRONIZE); }
    } WHEN {
        TURN { MOVE(user, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, move, secondaryEffect: FALSE, WITH_RNG(RNG_POISON_TOUCH, TRUE)); }
    } SCENE {
        ABILITY_POPUP(user, ABILITY_POISON_TOUCH);
        STATUS_ICON(target, poison: TRUE);
        ABILITY_POPUP(target, ABILITY_SYNCHRONIZE);
        STATUS_ICON(user, poison: TRUE);
    } THEN {
        EXPECT_EQ(user->status1, STATUS1_POISON);
        EXPECT_EQ(target->status1, STATUS1_POISON);
        EXPECT_EQ((u32)user->volatiles.substituteHP, 250);
    }
}
