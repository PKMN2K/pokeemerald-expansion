#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished status gates apply shared Infiltrator Substitute rules independently of native presets")
{
    static const enum Move moves[] = {MOVE_THUNDER_WAVE, MOVE_LEER, MOVE_CONFUSE_RAY, MOVE_WRAP, MOVE_TACKLE, MOVE_POWER_UP_PUNCH};
    struct BattlePokemon *user = player, *target = opponent;
    u32 kind = 0;
    bool32 gas = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 effect = 0; effect < ARRAY_COUNT(moves); effect++)
            for (u32 suppressed = 0; suppressed < 2; suppressed++)
                PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; kind = effect; gas = suppressed; }
    bool32 bypass = B_POLISHED_MOVE_BALANCE && !gas && kind != 5;
    GIVEN {
        WITH_CONFIG(B_INFILTRATOR_SUBSTITUTE, GEN_5);
        PLAYER(SPECIES_WOBBUFFET) { Speed(user == player ? 50 : 100); Attack(10); Defense(255); MaxHP(400); HP(400); Ability(user == player ? ABILITY_INFILTRATOR : gas ? ABILITY_NEUTRALIZING_GAS : ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(user == opponent ? 50 : 100); Attack(10); Defense(255); MaxHP(400); HP(400); Ability(user == opponent ? ABILITY_INFILTRATOR : gas ? ABILITY_NEUTRALIZING_GAS : ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(target, MOVE_SUBSTITUTE); MOVE(user, MOVE_SPLASH); }
        TURN { MOVE(target, MOVE_SPLASH); MOVE(user, moves[kind], hit: TRUE, criticalHit: FALSE); }
    } THEN {
        EXPECT(target->volatiles.substitute);
        if (kind == 0)
            EXPECT_EQ(target->status1, bypass ? STATUS1_PARALYSIS : STATUS1_NONE);
        else if (kind == 1)
            EXPECT_EQ(target->statStages[STAT_DEF], DEFAULT_STAT_STAGE - bypass);
        else if (kind == 2)
            EXPECT_EQ(target->volatiles.confusionTimer != 0, bypass);
        else if (kind == 3)
            EXPECT_EQ(target->volatiles.wrapTurns != 0, bypass);
        if (kind >= 3)
        {
            EXPECT_EQ(target->hp < 300, bypass);
            EXPECT_EQ(target->volatiles.substituteHP < 100, !bypass);
        }
        else
            EXPECT_EQ(target->hp, 300);
    }
}

DOUBLE_BATTLE_TEST("Polished status gates preserve the native Infiltrator Substitute preset in doubles")
{
    GIVEN {
        WITH_CONFIG(B_INFILTRATOR_SUBSTITUTE, GEN_5);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_INFILTRATOR); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(opponentLeft, MOVE_SUBSTITUTE); }
        TURN { MOVE(playerLeft, MOVE_THUNDER_WAVE, target: opponentLeft, hit: TRUE); }
    } THEN {
        EXPECT_EQ(opponentLeft->status1, STATUS1_NONE);
        EXPECT(opponentLeft->volatiles.substituteHP);
    }
}

SINGLE_BATTLE_TEST("Polished status gates preserve the native Infiltrator Substitute preset with either active gimmick")
{
    bool32 userTera = FALSE;
    PARAMETRIZE { userTera = FALSE; }
    PARAMETRIZE { userTera = TRUE; }
    GIVEN {
        WITH_CONFIG(B_INFILTRATOR_SUBSTITUTE, GEN_5);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_INFILTRATOR); TeraType(TYPE_NORMAL); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); TeraType(TYPE_NORMAL); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH, gimmick: userTera ? GIMMICK_TERA : GIMMICK_NONE); MOVE(opponent, MOVE_SUBSTITUTE, gimmick: userTera ? GIMMICK_NONE : GIMMICK_TERA); }
        TURN { MOVE(player, MOVE_THUNDER_WAVE, hit: TRUE); }
    } THEN {
        EXPECT_EQ(opponent->status1, STATUS1_NONE);
        EXPECT(opponent->volatiles.substituteHP);
    }
}

SINGLE_BATTLE_TEST("Polished status gates keep Electric paralysis immunity independent of native presets")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_THUNDER_WAVE;
    for (u32 side = 0; side < 2; side++)
        for (u32 secondary = 0; secondary < 2; secondary++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; move = secondary ? MOVE_BODY_SLAM : MOVE_THUNDER_WAVE; }
    GIVEN {
        WITH_CONFIG(B_PARALYZE_ELECTRIC, GEN_5);
        PLAYER(target == player ? SPECIES_PIKACHU : SPECIES_WOBBUFFET) { Speed(user == player ? 100 : 50); Ability(ABILITY_NONE); MaxHP(1000); HP(1000); }
        OPPONENT(target == opponent ? SPECIES_PIKACHU : SPECIES_WOBBUFFET) { Speed(user == opponent ? 100 : 50); Ability(ABILITY_NONE); MaxHP(1000); HP(1000); }
    } WHEN {
        TURN { MOVE(user, move, hit: TRUE, secondaryEffect: TRUE); MOVE(target, MOVE_SPLASH); }
    } SCENE {
        if (B_POLISHED_MOVE_BALANCE)
            NOT STATUS_ICON(target, paralysis: TRUE);
        else
            STATUS_ICON(target, paralysis: TRUE);
    } THEN {
        EXPECT_EQ(target->status1, B_POLISHED_MOVE_BALANCE ? STATUS1_NONE : STATUS1_PARALYSIS);
    }
}

SINGLE_BATTLE_TEST("Polished status gates check primary Safeguard before status type and ability rejection")
{
    struct BattlePokemon *user = player, *target = opponent;
    u32 kind = 0;
    for (u32 side = 0; side < 2; side++)
        for (u32 gate = 0; gate < 5; gate++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; kind = gate; }
    enum Move move = kind == 0 ? MOVE_HYPNOSIS : kind == 1 ? MOVE_TOXIC : kind == 3 ? MOVE_THUNDER_WAVE : MOVE_WILL_O_WISP;
    enum Ability ability = kind == 0 ? ABILITY_INSOMNIA : kind == 1 ? ABILITY_IMMUNITY : kind == 2 ? ABILITY_WATER_VEIL : kind == 3 ? ABILITY_LIMBER : ABILITY_NONE;
    enum Species targetSpecies = kind == 4 ? SPECIES_MAGMAR : SPECIES_WOBBUFFET;
    GIVEN {
        PLAYER(target == player ? targetSpecies : SPECIES_WOBBUFFET) { Speed(user == player ? 100 : 50); Ability(target == player ? ability : ABILITY_NONE); }
        OPPONENT(target == opponent ? targetSpecies : SPECIES_WOBBUFFET) { Speed(user == opponent ? 100 : 50); Ability(target == opponent ? ability : ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(target, MOVE_SAFEGUARD); MOVE(user, MOVE_SPLASH); }
        TURN { MOVE(user, move, hit: TRUE); MOVE(target, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SAFEGUARD, target);
        if (B_POLISHED_MOVE_BALANCE) {
            NONE_OF {
                ABILITY_POPUP(target, ability);
                ANIMATION(ANIM_TYPE_MOVE, move, user);
            }
            if (kind == 4) {
                if (target == player) MESSAGE("Magmar is protected by Safeguard!");
                else MESSAGE("The opposing Magmar is protected by Safeguard!");
            } else {
                if (target == player) MESSAGE("Wobbuffet is protected by Safeguard!");
                else MESSAGE("The opposing Wobbuffet is protected by Safeguard!");
            }
        } else if (kind < 4) {
            ABILITY_POPUP(target, ability);
        }
    } THEN {
        EXPECT_EQ(target->status1, STATUS1_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished status gates preserve Safeguard Infiltrator and Mold Breaker distinctions")
{
    enum Ability ability = ABILITY_NONE;
    PARAMETRIZE { ability = ABILITY_NONE; }
    PARAMETRIZE { ability = ABILITY_INFILTRATOR; }
    PARAMETRIZE { ability = ABILITY_MOLD_BREAKER; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); Ability(ability); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_SAFEGUARD); MOVE(player, MOVE_SPLASH); }
        TURN { MOVE(player, MOVE_THUNDER_WAVE, hit: TRUE); MOVE(opponent, MOVE_SPLASH); }
    } THEN {
        EXPECT_EQ(opponent->status1, ability == ABILITY_INFILTRATOR ? STATUS1_PARALYSIS : STATUS1_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished status gates retain type rejection before Substitute and immunity after it")
{
    u32 kind = 0;
    PARAMETRIZE { kind = 0; }
    PARAMETRIZE { kind = 1; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); Ability(ABILITY_NONE); }
        OPPONENT(kind ? SPECIES_PIKACHU : SPECIES_WOBBUFFET) { Speed(50); Ability(kind ? ABILITY_NONE : ABILITY_LIMBER); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_SUBSTITUTE); MOVE(player, MOVE_SPLASH); }
        TURN { MOVE(player, MOVE_THUNDER_WAVE, hit: TRUE); MOVE(opponent, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SUBSTITUTE, opponent);
        NOT ABILITY_POPUP(opponent, ABILITY_LIMBER);
        if (kind && B_POLISHED_MOVE_BALANCE)
            MESSAGE("It doesn't affect the opposing Pikachu…");
        else
            MESSAGE("But it failed!");
    } THEN {
        EXPECT_EQ(opponent->status1, STATUS1_NONE);
        EXPECT(opponent->volatiles.substituteHP);
    }
}

DOUBLE_BATTLE_TEST("Polished status gates leave native doubles Electric paralysis presets available")
{
    GIVEN {
        WITH_CONFIG(B_PARALYZE_ELECTRIC, GEN_5);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_PIKACHU) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_THUNDER_WAVE, target: opponentLeft, hit: TRUE); }
    } THEN {
        EXPECT_EQ(opponentLeft->status1, STATUS1_PARALYSIS);
    }
}

SINGLE_BATTLE_TEST("Polished status gates leave either active gimmick on native Electric paralysis presets")
{
    bool32 userTera = FALSE;
    PARAMETRIZE { userTera = FALSE; }
    PARAMETRIZE { userTera = TRUE; }
    GIVEN {
        WITH_CONFIG(B_PARALYZE_ELECTRIC, GEN_5);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); TeraType(TYPE_NORMAL); }
        OPPONENT(SPECIES_PIKACHU) { Ability(ABILITY_NONE); TeraType(TYPE_ELECTRIC); }
    } WHEN {
        TURN { MOVE(player, MOVE_THUNDER_WAVE, hit: TRUE, gimmick: userTera ? GIMMICK_TERA : GIMMICK_NONE); MOVE(opponent, MOVE_SPLASH, gimmick: userTera ? GIMMICK_NONE : GIMMICK_TERA); }
    } THEN {
        EXPECT_EQ(opponent->status1, STATUS1_PARALYSIS);
    }
}
