#include "global.h"
#include "test/battle.h"

static const enum Move sPoisonMoves[] = {MOVE_POISONPOWDER, MOVE_TOXIC};
static const enum Ability sStatusAbilities[] = {ABILITY_IMMUNITY, ABILITY_PASTEL_VEIL, ABILITY_LEAF_GUARD};

SINGLE_BATTLE_TEST("Polished Corrosion status bypasses prevention before immunity abilities cure the poison")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_POISONPOWDER;
    enum Ability ability = ABILITY_IMMUNITY;
    bool32 corrosion = FALSE;
    for (u32 m = 0; m < ARRAY_COUNT(sPoisonMoves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 a = 0; a < ARRAY_COUNT(sStatusAbilities); a++)
                for (u32 c = 0; c < 2; c++)
                    PARAMETRIZE { move = sPoisonMoves[m]; user = side ? opponent : player; target = side ? player : opponent; ability = sStatusAbilities[a]; corrosion = c; }
    bool32 applied = B_POLISHED_MOVE_BALANCE && corrosion;
    bool32 persists = applied && ability == ABILITY_LEAF_GUARD;
    u32 status = move == MOVE_TOXIC ? STATUS1_TOXIC_POISON : STATUS1_POISON;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(user == player ? 100 : 50); MaxHP(200); HP(200); Ability(user == player ? (corrosion ? ABILITY_CORROSION : ABILITY_NONE) : ability); MovesWithPP({move, 2}, {MOVE_SUNNY_DAY, 5}, {MOVE_CELEBRATE, 5}); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(user == opponent ? 100 : 50); MaxHP(200); HP(200); Ability(user == opponent ? (corrosion ? ABILITY_CORROSION : ABILITY_NONE) : ability); MovesWithPP({move, 2}, {MOVE_SUNNY_DAY, 5}, {MOVE_CELEBRATE, 5}); }
    } WHEN {
        TURN { MOVE(user, MOVE_SUNNY_DAY); }
        TURN { MOVE(user, move, hit: TRUE); MOVE(target, MOVE_CELEBRATE); }
    } SCENE {
        if (applied) {
            ANIMATION(ANIM_TYPE_MOVE, move, user);
            STATUS_ICON(target, status);
            if (ability != ABILITY_LEAF_GUARD) {
                ABILITY_POPUP(target, ability);
                STATUS_ICON(target, none: TRUE);
            } else NOT ABILITY_POPUP(target, ability);
        } else {
            NOT ANIMATION(ANIM_TYPE_MOVE, move, user);
            ABILITY_POPUP(target, ability);
            NOT STATUS_ICON(target, status);
        }
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, target);
    } THEN {
        EXPECT_EQ(target->status1 & (STATUS1_POISON | STATUS1_TOXIC_POISON), persists ? status : 0);
        EXPECT_EQ(user->pp[0], 1);
        if (!persists) EXPECT_EQ(target->hp, 200);
    }
}

SINGLE_BATTLE_TEST("Polished Corrosion status consumes healing berries before Immunity or Pastel Veil can cure")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_POISONPOWDER;
    enum Ability ability = ABILITY_IMMUNITY;
    enum Item item = ITEM_PECHA_BERRY;
    bool32 sub = FALSE;
    for (u32 m = 0; m < ARRAY_COUNT(sPoisonMoves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 a = 0; a < 2; a++)
                for (u32 berry = 0; berry < 2; berry++)
                    for (u32 shield = 0; shield < 2; shield++)
                        PARAMETRIZE { move = sPoisonMoves[m]; user = side ? opponent : player; target = side ? player : opponent; ability = sStatusAbilities[a]; item = berry ? ITEM_LUM_BERRY : ITEM_PECHA_BERRY; sub = shield; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(user == player ? 100 : 50); MaxHP(200); HP(200); Ability(user == player ? ABILITY_CORROSION : ability); Item(target == player ? item : ITEM_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(user == opponent ? 100 : 50); MaxHP(200); HP(200); Ability(user == opponent ? ABILITY_CORROSION : ability); Item(target == opponent ? item : ITEM_NONE); }
    } WHEN {
        if (sub) TURN { MOVE(target, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, move, hit: TRUE); MOVE(target, MOVE_CELEBRATE); }
    } SCENE {
        if (B_POLISHED_MOVE_BALANCE) {
            ANIMATION(ANIM_TYPE_MOVE, move, user);
            STATUS_ICON(target, move == MOVE_TOXIC ? STATUS1_TOXIC_POISON : STATUS1_POISON);
            // Native presentation hides this animation behind Substitute.
            // Status recovery, consumption and absence of an ability cure still apply.
            if (!sub) ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, target);
            STATUS_ICON(target, none: TRUE);
            NOT ABILITY_POPUP(target, ability);
        } else {
            NONE_OF {
                ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, target);
                STATUS_ICON(target, move == MOVE_TOXIC ? STATUS1_TOXIC_POISON : STATUS1_POISON);
            }
        }
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, target);
    } THEN {
        EXPECT_EQ(target->status1, STATUS1_NONE);
        EXPECT_EQ(target->item, B_POLISHED_MOVE_BALANCE ? ITEM_NONE : item);
        EXPECT_EQ(target->hp, sub ? 150 : 200);
        if (sub) EXPECT_EQ((u32)target->volatiles.substituteHP, 50);
    }
}

SINGLE_BATTLE_TEST("Polished Corrosion status retains Protect Safeguard existing status and accuracy gates")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_POISONPOWDER;
    enum Ability ability = ABILITY_IMMUNITY;
    u32 gate = 0;
    for (u32 m = 0; m < ARRAY_COUNT(sPoisonMoves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 a = 0; a < ARRAY_COUNT(sStatusAbilities); a++)
                for (u32 g = 0; g < 4; g++)
                    PARAMETRIZE { move = sPoisonMoves[m]; user = side ? opponent : player; target = side ? player : opponent; ability = sStatusAbilities[a]; gate = g; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(user == player ? 100 : 50); Ability(user == player ? ABILITY_CORROSION : ability); if (target == player && gate == 2) Status1(STATUS1_BURN); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(user == opponent ? 100 : 50); Ability(user == opponent ? ABILITY_CORROSION : ability); if (target == opponent && gate == 2) Status1(STATUS1_BURN); }
    } WHEN {
        TURN { MOVE(user, MOVE_SUNNY_DAY); if (gate == 1) MOVE(target, MOVE_SAFEGUARD); }
        TURN { if (gate == 0) MOVE(target, MOVE_PROTECT); MOVE(user, move, hit: gate != 3); }
    } SCENE {
        NOT STATUS_ICON(target, move == MOVE_TOXIC ? STATUS1_TOXIC_POISON : STATUS1_POISON);
    } THEN {
        EXPECT_EQ(target->status1, gate == 2 ? STATUS1_BURN : STATUS1_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Corrosion status respects suppression of the actual attacker ability")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_POISONPOWDER;
    enum Ability ability = ABILITY_IMMUNITY;
    for (u32 m = 0; m < ARRAY_COUNT(sPoisonMoves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 a = 0; a < ARRAY_COUNT(sStatusAbilities); a++)
                PARAMETRIZE { move = sPoisonMoves[m]; user = side ? opponent : player; target = side ? player : opponent; ability = sStatusAbilities[a]; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_CORROSION : ability); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_CORROSION : ability); }
    } WHEN {
        TURN { MOVE(target, MOVE_GASTRO_ACID); MOVE(user, MOVE_SUNNY_DAY); }
        TURN { MOVE(user, move, hit: TRUE); }
    } SCENE {
        NOT STATUS_ICON(target, move == MOVE_TOXIC ? STATUS1_TOXIC_POISON : STATUS1_POISON);
    } THEN { EXPECT_EQ(target->status1, STATUS1_NONE); }
}

SINGLE_BATTLE_TEST("Polished Corrosion status leaves either active Terastal native")
{
    enum Move move = MOVE_POISONPOWDER;
    enum Ability ability = ABILITY_IMMUNITY;
    bool32 userTera = FALSE;
    for (u32 m = 0; m < ARRAY_COUNT(sPoisonMoves); m++)
        for (u32 a = 0; a < ARRAY_COUNT(sStatusAbilities); a++)
            for (u32 side = 0; side < 2; side++)
                PARAMETRIZE { move = sPoisonMoves[m]; ability = sStatusAbilities[a]; userTera = side; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); Ability(ABILITY_CORROSION); TeraType(TYPE_NORMAL); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); Ability(ability); TeraType(TYPE_NORMAL); }
    } WHEN {
        TURN { MOVE(player, MOVE_SUNNY_DAY, gimmick: userTera ? GIMMICK_TERA : GIMMICK_NONE); MOVE(opponent, MOVE_CELEBRATE, gimmick: userTera ? GIMMICK_NONE : GIMMICK_TERA); }
        TURN { MOVE(player, move, hit: TRUE); }
    } SCENE {
        NOT STATUS_ICON(opponent, move == MOVE_TOXIC ? STATUS1_TOXIC_POISON : STATUS1_POISON);
    } THEN { EXPECT_EQ(opponent->status1, STATUS1_NONE); }
}

DOUBLE_BATTLE_TEST("Polished Corrosion status leaves doubles native")
{
    enum Move move = MOVE_POISONPOWDER;
    enum Ability ability = ABILITY_IMMUNITY;
    for (u32 m = 0; m < ARRAY_COUNT(sPoisonMoves); m++)
        for (u32 a = 0; a < ARRAY_COUNT(sStatusAbilities); a++)
            PARAMETRIZE { move = sPoisonMoves[m]; ability = sStatusAbilities[a]; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_CORROSION); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ability); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_SUNNY_DAY); }
        TURN { MOVE(playerLeft, move, target: opponentLeft, hit: TRUE); }
    } SCENE {
        NOT STATUS_ICON(opponentLeft, move == MOVE_TOXIC ? STATUS1_TOXIC_POISON : STATUS1_POISON);
    } THEN { EXPECT_EQ(opponentLeft->status1, STATUS1_NONE); }
}

SINGLE_BATTLE_TEST("Polished Corrosion status leaves dynamically retyped moves native")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_POISONPOWDER;
    enum Ability ability = ABILITY_IMMUNITY;
    for (u32 m = 0; m < ARRAY_COUNT(sPoisonMoves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 a = 0; a < ARRAY_COUNT(sStatusAbilities); a++)
                PARAMETRIZE { move = sPoisonMoves[m]; user = side ? opponent : player; target = side ? player : opponent; ability = sStatusAbilities[a]; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(target == player ? 100 : 50); Ability(user == player ? ABILITY_CORROSION : ability); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(target == opponent ? 100 : 50); Ability(user == opponent ? ABILITY_CORROSION : ability); }
    } WHEN {
        TURN { MOVE(user, MOVE_SUNNY_DAY); }
        TURN { MOVE(target, MOVE_ELECTRIFY); MOVE(user, move, hit: TRUE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_ELECTRIFY, target);
        NOT STATUS_ICON(target, move == MOVE_TOXIC ? STATUS1_TOXIC_POISON : STATUS1_POISON);
    } THEN { EXPECT_EQ(target->status1, STATUS1_NONE); }
}

SINGLE_BATTLE_TEST("Polished Corrosion status leaves optional Magic Coat returns native")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_POISONPOWDER;
    enum Ability ability = ABILITY_IMMUNITY;
    for (u32 m = 0; m < ARRAY_COUNT(sPoisonMoves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 a = 0; a < ARRAY_COUNT(sStatusAbilities); a++)
                PARAMETRIZE { move = sPoisonMoves[m]; user = side ? opponent : player; target = side ? player : opponent; ability = sStatusAbilities[a]; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(target == player ? ABILITY_CORROSION : ability); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(target == opponent ? ABILITY_CORROSION : ability); }
    } WHEN {
        TURN { MOVE(user, MOVE_SUNNY_DAY); }
        TURN { MOVE(target, MOVE_MAGIC_COAT); MOVE(user, move, hit: TRUE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_MAGIC_COAT, target);
        NOT STATUS_ICON(user, move == MOVE_TOXIC ? STATUS1_TOXIC_POISON : STATUS1_POISON);
    } THEN {
        EXPECT_EQ(user->status1, STATUS1_NONE);
        EXPECT_EQ(target->status1, STATUS1_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Corrosion status leaves other status moves native")
{
    static const enum Move moves[] = {MOVE_HYPNOSIS, MOVE_THUNDER_WAVE, MOVE_WILL_O_WISP, MOVE_POISON_GAS};
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_HYPNOSIS;
    for (u32 m = 0; m < ARRAY_COUNT(moves); m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = moves[m]; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_CORROSION : ABILITY_LEAF_GUARD); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_CORROSION : ABILITY_LEAF_GUARD); }
    } WHEN {
        TURN { MOVE(user, MOVE_SUNNY_DAY); }
        TURN { MOVE(user, move, hit: TRUE); }
    } THEN { EXPECT_EQ(target->status1, STATUS1_NONE); }
}

SINGLE_BATTLE_TEST("Polished Corrosion status leaves optional status barriers native")
{
    static const enum Ability abilities[] = {ABILITY_COMATOSE, ABILITY_PURIFYING_SALT, ABILITY_FLOWER_VEIL};
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_POISONPOWDER;
    enum Ability ability = ABILITY_COMATOSE;
    for (u32 m = 0; m < ARRAY_COUNT(sPoisonMoves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 a = 0; a < ARRAY_COUNT(abilities); a++)
                PARAMETRIZE { move = sPoisonMoves[m]; user = side ? opponent : player; target = side ? player : opponent; ability = abilities[a]; }
    GIVEN {
        PLAYER(target == player ? SPECIES_TANGELA : SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_CORROSION : ability); }
        OPPONENT(target == opponent ? SPECIES_TANGELA : SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_CORROSION : ability); }
    } WHEN { TURN { MOVE(user, move, hit: TRUE); } }
    THEN { EXPECT_EQ(target->status1, STATUS1_NONE); }
}

SINGLE_BATTLE_TEST("Polished Corrosion status does not bypass ability protection for damaging poison effects")
{
    static const enum Move moves[] = {MOVE_POISON_STING, MOVE_SLUDGE_BOMB};
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_POISON_STING;
    enum Ability ability = ABILITY_IMMUNITY;
    for (u32 m = 0; m < ARRAY_COUNT(moves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 a = 0; a < ARRAY_COUNT(sStatusAbilities); a++)
                PARAMETRIZE { move = moves[m]; user = side ? opponent : player; target = side ? player : opponent; ability = sStatusAbilities[a]; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_CORROSION : ability); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_CORROSION : ability); }
    } WHEN {
        TURN { MOVE(user, MOVE_SUNNY_DAY); }
        TURN { MOVE(user, move, hit: TRUE, secondaryEffect: TRUE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, user);
        HP_BAR(target);
        NOT STATUS_ICON(target, poison: TRUE);
    } THEN { EXPECT_EQ(target->status1, STATUS1_NONE); }
}

SINGLE_BATTLE_TEST("Polished Corrosion status allows ordinary berry healing after bypassing Leaf Guard")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_POISONPOWDER;
    enum Item item = ITEM_PECHA_BERRY;
    for (u32 m = 0; m < ARRAY_COUNT(sPoisonMoves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 berry = 0; berry < 2; berry++)
                PARAMETRIZE { move = sPoisonMoves[m]; user = side ? opponent : player; target = side ? player : opponent; item = berry ? ITEM_LUM_BERRY : ITEM_PECHA_BERRY; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_CORROSION : ABILITY_LEAF_GUARD); Item(target == player ? item : ITEM_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_CORROSION : ABILITY_LEAF_GUARD); Item(target == opponent ? item : ITEM_NONE); }
    } WHEN {
        TURN { MOVE(user, MOVE_SUNNY_DAY); }
        TURN { MOVE(user, move, hit: TRUE); }
    } SCENE {
        if (B_POLISHED_MOVE_BALANCE) {
            STATUS_ICON(target, move == MOVE_TOXIC ? STATUS1_TOXIC_POISON : STATUS1_POISON);
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, target);
            STATUS_ICON(target, none: TRUE);
        } else NOT ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, target);
    } THEN {
        EXPECT_EQ(target->status1, STATUS1_NONE);
        EXPECT_EQ(target->item, B_POLISHED_MOVE_BALANCE ? ITEM_NONE : item);
    }
}
