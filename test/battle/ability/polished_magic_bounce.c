#include "global.h"
#include "test/battle.h"
#include "test/polished_magic_bounce.h"

SINGLE_BATTLE_TEST("Polished Magic Bounce activates for every source candidate and respects Mold Breaker")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_NONE;
    enum Ability ability = ABILITY_NONE;
    for (u32 entry = 0; entry < ARRAY_COUNT(sSourceMagicBounceMoves); entry++)
        if (sSourceMagicBounceMoves[entry].candidate)
            for (u32 side = 0; side < 2; side++)
                for (u32 breaker = 0; breaker < 2; breaker++)
                    PARAMETRIZE {
                        move = sSourceMagicBounceMoves[entry].move;
                        user = side ? opponent : player; target = side ? player : opponent;
                        ability = breaker ? ABILITY_MOLD_BREAKER : ABILITY_NONE;
                    }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ability : ABILITY_MAGIC_BOUNCE); Gender(MON_MALE); Speed(100); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(75); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ability : ABILITY_MAGIC_BOUNCE); Gender(MON_FEMALE); Speed(50); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(25); }
    } WHEN {
        TURN { MOVE(user, move, hit: TRUE); }
    } SCENE {
        if (ability == ABILITY_MOLD_BREAKER) NOT ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE);
        else ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE);
    }
}

SINGLE_BATTLE_TEST("Polished Magic Bounce excludes source self field and nonreflecting sound effects")
{
    static const enum Move excluded[] = {MOVE_HAZE, MOVE_PERISH_SONG, MOVE_HEAL_BELL, MOVE_SAFEGUARD, MOVE_DRAGON_DANCE, MOVE_POLISHED_HEALING_LIGHT};
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_NONE;
    for (u32 entry = 0; entry < ARRAY_COUNT(excluded); entry++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = excluded[entry]; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_NONE : ABILITY_MAGIC_BOUNCE); Speed(100); MaxHP(200); HP(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_NONE : ABILITY_MAGIC_BOUNCE); Speed(50); MaxHP(200); HP(100); }
    } WHEN {
        TURN { MOVE(user, move); }
    } SCENE {
        NOT ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE);
    }
}

SINGLE_BATTLE_TEST("Polished Magic Bounce respects Substitute for ordinary status moves")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_TOXIC;
    enum Ability ability = ABILITY_NONE;
    u32 status = STATUS1_TOXIC_POISON;
    bool32 sub = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 3; kind++)
            for (u32 bypass = 0; bypass < 3; bypass++)
                for (u32 shield = 0; shield < 2; shield++)
                    PARAMETRIZE {
                        user = side ? opponent : player; target = side ? player : opponent;
                        move = kind == 0 ? MOVE_TOXIC : kind == 1 ? MOVE_THUNDER_WAVE : MOVE_WILL_O_WISP;
                        status = kind == 0 ? STATUS1_TOXIC_POISON : kind == 1 ? STATUS1_PARALYSIS : STATUS1_BURN;
                        ability = bypass == 0 ? ABILITY_NONE : bypass == 1 ? ABILITY_INFILTRATOR : ABILITY_MOLD_BREAKER;
                        sub = shield;
                    }
    bool32 reflected = ability != ABILITY_MOLD_BREAKER && (!B_POLISHED_MOVE_BALANCE || !sub || ability == ABILITY_INFILTRATOR);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ability : ABILITY_MAGIC_BOUNCE); Speed(100); if (user == player) MovesWithPP({move, 1}); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ability : ABILITY_MAGIC_BOUNCE); Speed(50); if (user == opponent) MovesWithPP({move, 1}); }
    } WHEN {
        if (sub) TURN { MOVE(target, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, move, hit: TRUE); }
    } SCENE {
        if (reflected) ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE);
        else NOT ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE);
    } THEN {
        EXPECT_EQ(user->status1 & status, reflected ? status : 0);
        EXPECT_EQ(target->status1 & status, ability == ABILITY_MOLD_BREAKER && !sub ? status : 0);
        EXPECT_EQ(user->pp[0], 0);
        if (sub) EXPECT_GT((u32)target->volatiles.substituteHP, 0);
    }
}

SINGLE_BATTLE_TEST("Polished Magic Bounce respects Substitute for silent stat drops")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_LEER;
    u32 stat = STAT_DEF, amount = 1;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 3; kind++)
            PARAMETRIZE {
                user = side ? opponent : player; target = side ? player : opponent;
                move = kind == 0 ? MOVE_LEER : kind == 1 ? MOVE_CHARM : MOVE_SCARY_FACE;
                stat = kind == 0 ? STAT_DEF : kind == 1 ? STAT_ATK : STAT_SPEED;
                amount = kind == 0 ? 1 : 2;
            }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_NONE : ABILITY_MAGIC_BOUNCE); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_NONE : ABILITY_MAGIC_BOUNCE); Speed(50); }
    } WHEN {
        TURN { MOVE(target, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, move, hit: TRUE); }
    } SCENE {
        if (B_POLISHED_MOVE_BALANCE) NOT ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE);
        else ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE);
    } THEN {
        EXPECT_EQ(user->statStages[stat], DEFAULT_STAT_STAGE - (B_POLISHED_MOVE_BALANCE ? 0 : amount));
        EXPECT_EQ(target->statStages[stat], DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished Magic Bounce retains all six explicit source Substitute exceptions")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_ATTRACT;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 6; kind++)
            PARAMETRIZE {
                user = side ? opponent : player; target = side ? player : opponent;
                move = kind == 0 ? MOVE_ATTRACT : kind == 1 ? MOVE_DISABLE : kind == 2 ? MOVE_ENCORE : kind == 3 ? MOVE_FORESIGHT : kind == 4 ? MOVE_SPIKES : MOVE_TOXIC_SPIKES;
            }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_NONE : ABILITY_MAGIC_BOUNCE); Gender(MON_MALE); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_NONE : ABILITY_MAGIC_BOUNCE); Gender(MON_FEMALE); Speed(50); }
    } WHEN {
        TURN { MOVE(user, MOVE_CELEBRATE); MOVE(target, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, move, hit: TRUE); }
    } SCENE {
        ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE);
        // Encore can be reflected even when the returned effect has no legal last move.
        if (move != MOVE_ENCORE) ANIMATION(ANIM_TYPE_MOVE, move, target);
    } THEN {
        EXPECT_GT((u32)target->volatiles.substituteHP, 0);
        if (move == MOVE_ATTRACT) EXPECT(user->volatiles.infatuation);
    }
}

SINGLE_BATTLE_TEST("Polished Magic Bounce reflects source sound moves through Substitute including Roar")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_GROWL;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 5; kind++)
            PARAMETRIZE {
                user = side ? opponent : player; target = side ? player : opponent;
                move = kind == 0 ? MOVE_GROWL : kind == 1 ? MOVE_SCREECH : kind == 2 ? MOVE_SING : kind == 3 ? MOVE_SUPERSONIC : MOVE_ROAR;
            }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_NONE : ABILITY_MAGIC_BOUNCE); Speed(100); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(75); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_NONE : ABILITY_MAGIC_BOUNCE); Speed(50); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(25); }
    } WHEN {
        TURN { MOVE(target, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, move, hit: TRUE); }
    } SCENE {
        ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE);
        ANIMATION(ANIM_TYPE_MOVE, move, target);
    } THEN {
        EXPECT_EQ(target->species, SPECIES_WOBBUFFET);
        EXPECT_GT((u32)target->volatiles.substituteHP, 0);
        if (move == MOVE_ROAR) EXPECT_EQ(user->species, SPECIES_WYNAUT);
        if (move == MOVE_GROWL) EXPECT_EQ(user->statStages[STAT_ATK], DEFAULT_STAT_STAGE - 1);
        if (move == MOVE_SCREECH) EXPECT_EQ(user->statStages[STAT_DEF], DEFAULT_STAT_STAGE - 2);
        if (move == MOVE_SING) EXPECT(user->status1 & STATUS1_SLEEP);
        if (move == MOVE_SUPERSONIC) EXPECT(user->volatiles.confusionTimer);
    }
}

SINGLE_BATTLE_TEST("Polished Magic Bounce keeps Protect and hidden-target gates ahead of reflection")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move setup = MOVE_PROTECT;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 3; kind++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; setup = kind == 0 ? MOVE_PROTECT : kind == 1 ? MOVE_FLY : MOVE_DIG; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_NO_GUARD : ABILITY_MAGIC_BOUNCE); Speed(user == player ? 50 : 100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_NO_GUARD : ABILITY_MAGIC_BOUNCE); Speed(user == opponent ? 50 : 100); }
    } WHEN {
        TURN { MOVE(target, setup); MOVE(user, MOVE_TOXIC); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, setup, target);
        NOT ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE);
    } THEN {
        EXPECT_EQ(user->status1 & STATUS1_TOXIC_POISON, 0);
        EXPECT_EQ(target->status1 & STATUS1_TOXIC_POISON, setup == MOVE_PROTECT ? 0 : STATUS1_TOXIC_POISON);
    }
}

SINGLE_BATTLE_TEST("Polished Magic Bounce cannot reflect twice")
{
    struct BattlePokemon *user = player, *target = opponent;
    PARAMETRIZE { user = player; target = opponent; }
    PARAMETRIZE { user = opponent; target = player; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_MAGIC_BOUNCE); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_MAGIC_BOUNCE); Speed(50); }
    } WHEN {
        TURN { MOVE(user, MOVE_TOXIC, hit: TRUE); }
    } SCENE {
        ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE);
        NOT ABILITY_POPUP(user, ABILITY_MAGIC_BOUNCE);
    } THEN {
        EXPECT(user->status1 & STATUS1_TOXIC_POISON);
        EXPECT_EQ(target->status1 & STATUS1_TOXIC_POISON, 0);
    }
}

SINGLE_BATTLE_TEST("Polished Magic Bounce recomputes reflection after Substitute breaks")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Attack(400); Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_MAGIC_BOUNCE); HP(100); MaxHP(100); Defense(100); Speed(100); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_SUBSTITUTE); MOVE(player, MOVE_TACKLE, hit: TRUE); }
        TURN { MOVE(player, MOVE_TOXIC, hit: TRUE); }
    } SCENE {
        MESSAGE("The opposing Wobbuffet's substitute faded!");
        ABILITY_POPUP(opponent, ABILITY_MAGIC_BOUNCE);
    } THEN {
        EXPECT_EQ((u32)opponent->volatiles.substituteHP, 0);
        EXPECT(player->status1 & STATUS1_TOXIC_POISON);
    }
}

SINGLE_BATTLE_TEST("Polished Magic Bounce retains optional moves Magic Coat and active gimmicks")
{
    u32 kind = 0;
    PARAMETRIZE { kind = 0; } // Optional Spider Web.
    PARAMETRIZE { kind = 1; } // Optional Magic Coat.
    PARAMETRIZE { kind = 2; } // Attacker Terastal.
    PARAMETRIZE { kind = 3; } // Defender Terastal.
    enum Move move = kind == 0 ? MOVE_SPIDER_WEB : MOVE_TOXIC;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); TeraType(TYPE_NORMAL); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(kind == 1 ? ABILITY_NONE : ABILITY_MAGIC_BOUNCE); TeraType(TYPE_NORMAL); Speed(50); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_SUBSTITUTE, gimmick: kind == 3 ? GIMMICK_TERA : GIMMICK_NONE); }
        TURN { if (kind == 1) MOVE(opponent, MOVE_MAGIC_COAT); MOVE(player, move, hit: TRUE, gimmick: kind == 2 ? GIMMICK_TERA : GIMMICK_NONE); }
    } SCENE {
        if (kind != 1) ABILITY_POPUP(opponent, ABILITY_MAGIC_BOUNCE);
        ANIMATION(ANIM_TYPE_MOVE, move, opponent);
    } THEN {
        if (kind == 0) EXPECT(player->volatiles.escapePrevention);
        else EXPECT(player->status1 & STATUS1_TOXIC_POISON);
    }
}

DOUBLE_BATTLE_TEST("Polished Magic Bounce retains native Substitute reflection in optional doubles")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_MAGIC_BOUNCE); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(opponentLeft, MOVE_SUBSTITUTE); }
        TURN { MOVE(playerLeft, MOVE_TOXIC, target: opponentLeft, hit: TRUE); }
    } SCENE {
        ABILITY_POPUP(opponentLeft, ABILITY_MAGIC_BOUNCE);
        STATUS_ICON(playerLeft, badPoison: TRUE);
    }
}
