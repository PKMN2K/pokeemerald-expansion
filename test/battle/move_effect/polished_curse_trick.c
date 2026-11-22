#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished Curse respects Substitute with the source Infiltrator exception")
{
    enum Ability ability = ABILITY_NONE;
    struct BattlePokemon *user = player, *other = opponent;
    for (u32 side = 0; side < 2; side++)
        for (u32 infiltrator = 0; infiltrator < 2; infiltrator++)
            PARAMETRIZE { user = side ? opponent : player; other = side ? player : opponent; ability = infiltrator ? ABILITY_INFILTRATOR : ABILITY_NONE; }
    GIVEN {
        PLAYER(SPECIES_MISDREAVUS) { Ability(user == player ? ability : ABILITY_NONE); MaxHP(400); HP(400); Speed(user == player ? 50 : 100); }
        OPPONENT(SPECIES_MISDREAVUS) { Ability(user == opponent ? ability : ABILITY_NONE); MaxHP(400); HP(400); Speed(user == opponent ? 50 : 100); }
    } WHEN {
        TURN { MOVE(other, MOVE_SUBSTITUTE); MOVE(user, MOVE_CURSE); }
    } THEN {
        bool32 succeeds = !B_POLISHED_MOVE_BALANCE || ability == ABILITY_INFILTRATOR;
        EXPECT_EQ((u32)other->volatiles.cursed, succeeds);
        EXPECT_EQ(user->hp, succeeds ? 200 : 400);
        EXPECT_EQ(other->hp, succeeds ? 200 : 300);
        EXPECT_EQ((u32)other->volatiles.substituteHP, 100);
    }
}

SINGLE_BATTLE_TEST("Polished Curse rejects Fly and Dig even with No Guard")
{
    enum Move hidden = MOVE_FLY;
    enum Ability ability = ABILITY_NONE;
    for (u32 dig = 0; dig < 2; dig++)
        for (u32 noGuard = 0; noGuard < 2; noGuard++)
            PARAMETRIZE { hidden = dig ? MOVE_DIG : MOVE_FLY; ability = noGuard ? ABILITY_NO_GUARD : ABILITY_NONE; }
    GIVEN {
        PLAYER(SPECIES_MISDREAVUS) { Ability(ability); MaxHP(400); HP(400); Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); MaxHP(400); HP(400); Speed(100); }
    } WHEN {
        TURN { MOVE(opponent, hidden); MOVE(player, MOVE_CURSE); }
    } THEN {
        EXPECT_EQ((u32)opponent->volatiles.cursed, !B_POLISHED_MOVE_BALANCE);
        EXPECT_EQ(player->hp, B_POLISHED_MOVE_BALANCE ? 400 : 200);
    }
}

SINGLE_BATTLE_TEST("Polished Curse after Protean targets the opponent instead of its user")
{
    GIVEN {
        PLAYER(SPECIES_KECLEON) { Ability(ABILITY_PROTEAN); MaxHP(400); HP(400); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); MaxHP(400); HP(400); }
    } WHEN {
        TURN { MOVE(player, MOVE_CURSE, target: player); }
    } THEN {
        EXPECT_EQ((u32)player->volatiles.cursed, !B_POLISHED_MOVE_BALANCE);
        EXPECT_EQ((u32)opponent->volatiles.cursed, B_POLISHED_MOVE_BALANCE);
        EXPECT_EQ(player->hp, B_POLISHED_MOVE_BALANCE ? 200 : 100);
        EXPECT_EQ(opponent->hp, B_POLISHED_MOVE_BALANCE ? 300 : 400);
    }
}

SINGLE_BATTLE_TEST("Polished Trick transfers do not activate Unburden")
{
    bool32 take = FALSE;
    PARAMETRIZE { take = FALSE; }
    PARAMETRIZE { take = TRUE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_UNBURDEN); Item(take ? ITEM_NONE : ITEM_POKE_BALL); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_UNBURDEN); Item(take ? ITEM_POKE_BALL : ITEM_NONE); Speed(50); }
    } WHEN {
        TURN { MOVE(player, MOVE_TRICK); }
    } THEN {
        EXPECT_EQ(player->item, take ? ITEM_POKE_BALL : ITEM_NONE);
        EXPECT_EQ(opponent->item, take ? ITEM_NONE : ITEM_POKE_BALL);
        EXPECT_EQ((u32)player->volatiles.unburdenActive, !B_POLISHED_MOVE_BALANCE && !take);
        EXPECT_EQ((u32)opponent->volatiles.unburdenActive, !B_POLISHED_MOVE_BALANCE && take);
    }
}

SINGLE_BATTLE_TEST("Polished Trick preserves an Unburden boost already earned by consumption")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_UNBURDEN); Item(ITEM_ORAN_BERRY); MaxHP(400); HP(100); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_POKE_BALL); Speed(150); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); MOVE(opponent, MOVE_SPLASH); }
        TURN { MOVE(player, MOVE_TRICK); MOVE(opponent, MOVE_SPLASH); }
        TURN { MOVE(player, MOVE_CELEBRATE); MOVE(opponent, MOVE_CELEBRATE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TRICK, player);
        if (B_POLISHED_MOVE_BALANCE) {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, player);
            ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, opponent);
        } else {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, opponent);
            ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, player);
        }
    } THEN {
        EXPECT_EQ(player->item, ITEM_POKE_BALL);
        EXPECT_EQ((u32)player->volatiles.unburdenActive, B_POLISHED_MOVE_BALANCE);
    }
}

SINGLE_BATTLE_TEST("Polished Curse uses half maximum HP with floor rounding and a minimum of one")
{
    u32 maxHP = 401, hp = 401;
    PARAMETRIZE { maxHP = 401; hp = 401; }
    PARAMETRIZE { maxHP = 401; hp = 250; }
    PARAMETRIZE { maxHP = 3; hp = 3; }
    GIVEN {
        PLAYER(SPECIES_MISDREAVUS) { Ability(ABILITY_MAGIC_GUARD); MaxHP(maxHP); HP(hp); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); MaxHP(403); HP(403); }
    } WHEN {
        TURN { MOVE(player, MOVE_CURSE); }
    } THEN {
        EXPECT_EQ(player->hp, hp - maxHP / 2);
        EXPECT_EQ(opponent->hp, 303);
        EXPECT_EQ(GetMonData(GetBattlerMon(B_BATTLER_0), MON_DATA_HP), player->hp);
    }
}

SINGLE_BATTLE_TEST("Polished Curse can faint its user and persists after replacement")
{
    u32 maxHP = 400, hp = 200;
    PARAMETRIZE { maxHP = 400; hp = 200; }
    PARAMETRIZE { maxHP = 400; hp = 1; }
    PARAMETRIZE { maxHP = 1; hp = 1; }
    GIVEN {
        PLAYER(SPECIES_MISDREAVUS) { Ability(ABILITY_NONE); MaxHP(maxHP); HP(hp); Speed(100); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(70); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); MaxHP(400); HP(400); Speed(50); }
    } WHEN {
        TURN { MOVE(player, MOVE_CURSE); SEND_OUT(player, 1); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CURSE, player);
        HP_BAR(player, hp: 0);
        MESSAGE("Misdreavus fainted!");
        HP_BAR(opponent, damage: 100);
    } THEN {
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_HP), 0);
        EXPECT(opponent->volatiles.cursed);
    }
}

SINGLE_BATTLE_TEST("Polished Curse cannot charge HP again for an already cursed opponent")
{
    GIVEN {
        PLAYER(SPECIES_MISDREAVUS) { Ability(ABILITY_NONE); MaxHP(400); HP(400); Moves(MOVE_CURSE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); MaxHP(400); HP(400); }
    } WHEN {
        TURN { MOVE(player, MOVE_CURSE); }
        TURN { MOVE(player, MOVE_CURSE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CURSE, player);
        MESSAGE("But it failed!");
    } THEN {
        EXPECT_EQ(player->hp, 200);
        EXPECT_EQ(opponent->hp, 200);
        EXPECT_EQ(player->pp[0], GetMovePP(MOVE_CURSE) - 2);
    }
}

SINGLE_BATTLE_TEST("Polished Curse bypasses Protect and Magic Bounce")
{
    GIVEN {
        PLAYER(SPECIES_MISDREAVUS) { Ability(ABILITY_NONE); MaxHP(400); HP(400); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_MAGIC_BOUNCE); MaxHP(400); HP(400); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_PROTECT); MOVE(player, MOVE_CURSE); }
    } THEN {
        EXPECT(opponent->volatiles.cursed);
        EXPECT(!player->volatiles.cursed);
        EXPECT_EQ(player->hp, 200);
        EXPECT_EQ(opponent->hp, 300);
    }
}

SINGLE_BATTLE_TEST("Polished Curse Magic Guard blocks residual damage but does not block application")
{
    GIVEN {
        PLAYER(SPECIES_MISDREAVUS) { Ability(ABILITY_NONE); MaxHP(400); HP(400); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_MAGIC_GUARD); MaxHP(400); HP(400); }
    } WHEN {
        TURN { MOVE(player, MOVE_CURSE); }
        TURN { MOVE(player, MOVE_SKILL_SWAP); }
    } THEN {
        EXPECT(opponent->volatiles.cursed);
        EXPECT_EQ(opponent->hp, 300);
    }
}

SINGLE_BATTLE_TEST("Polished Curse residual damage ignores a Substitute created afterward")
{
    GIVEN {
        PLAYER(SPECIES_MISDREAVUS) { Ability(ABILITY_NONE); MaxHP(400); HP(400); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); MaxHP(400); HP(400); Speed(50); }
    } WHEN {
        TURN { MOVE(player, MOVE_CURSE); MOVE(opponent, MOVE_SUBSTITUTE); }
    } THEN {
        EXPECT_EQ(opponent->hp, 200);
        EXPECT_EQ((u32)opponent->volatiles.substituteHP, 100);
    }
}

SINGLE_BATTLE_TEST("Polished Curse switching clears the curse while Baton Pass transfers it")
{
    bool32 pass = FALSE;
    PARAMETRIZE { pass = FALSE; }
    PARAMETRIZE { pass = TRUE; }
    GIVEN {
        PLAYER(SPECIES_MISDREAVUS) { Ability(ABILITY_NONE); MaxHP(400); HP(400); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); MaxHP(400); HP(400); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); MaxHP(400); HP(400); }
    } WHEN {
        TURN { MOVE(player, MOVE_CURSE); }
        if (pass) TURN { MOVE(opponent, MOVE_BATON_PASS); SEND_OUT(opponent, 1); }
        else TURN { SWITCH(opponent, 1); }
    } THEN {
        EXPECT_EQ((u32)opponent->volatiles.cursed, pass);
        EXPECT_EQ(opponent->hp, pass ? 300 : 400);
    }
}

SINGLE_BATTLE_TEST("Polished Curse non Ghost stat changes honor Contrary Simple and individual caps")
{
    enum Ability ability = ABILITY_NONE;
    bool32 capped = FALSE;
    PARAMETRIZE { ability = ABILITY_NONE; }
    PARAMETRIZE { ability = ABILITY_CONTRARY; }
    PARAMETRIZE { ability = ABILITY_SIMPLE; }
    PARAMETRIZE { ability = ABILITY_NONE; capped = TRUE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ability); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN {
        if (capped)
            for (u32 i = 0; i < 3; i++)
                TURN { MOVE(player, MOVE_SWORDS_DANCE); MOVE(opponent, MOVE_SCARY_FACE, hit: TRUE); }
        TURN { MOVE(player, MOVE_CURSE); MOVE(opponent, MOVE_PROTECT); }
    } THEN {
        s32 delta = ability == ABILITY_SIMPLE ? 2 : ability == ABILITY_CONTRARY ? -1 : 1;
        EXPECT_EQ(player->statStages[STAT_ATK], capped ? MAX_STAT_STAGE : DEFAULT_STAT_STAGE + delta);
        EXPECT_EQ(player->statStages[STAT_DEF], DEFAULT_STAT_STAGE + delta);
        EXPECT_EQ(player->statStages[STAT_SPEED], capped ? MIN_STAT_STAGE : DEFAULT_STAT_STAGE - delta);
        EXPECT(!opponent->volatiles.cursed);
        EXPECT_EQ(player->hp, player->maxHP);
    }
}

SINGLE_BATTLE_TEST("Polished Curse non Ghost branch ignores opposing Substitute and hidden state")
{
    enum Move move = MOVE_SUBSTITUTE;
    PARAMETRIZE { move = MOVE_SUBSTITUTE; }
    PARAMETRIZE { move = MOVE_FLY; }
    PARAMETRIZE { move = MOVE_DIG; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); }
    } WHEN {
        TURN { MOVE(opponent, move); MOVE(player, MOVE_CURSE); }
    } THEN {
        EXPECT_EQ(player->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 1);
        EXPECT_EQ(player->statStages[STAT_DEF], DEFAULT_STAT_STAGE + 1);
        EXPECT_EQ(player->statStages[STAT_SPEED], DEFAULT_STAT_STAGE - 1);
    }
}

SINGLE_BATTLE_TEST("Polished Curse suppressed Infiltrator cannot bypass Substitute")
{
    GIVEN {
        PLAYER(SPECIES_MISDREAVUS) { Ability(ABILITY_INFILTRATOR); Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NEUTRALIZING_GAS); Speed(100); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_SUBSTITUTE); MOVE(player, MOVE_CURSE); }
    } THEN {
        EXPECT_EQ((u32)opponent->volatiles.cursed, !B_POLISHED_MOVE_BALANCE);
    }
}

SINGLE_BATTLE_TEST("Polished Curse keeps native Substitute behavior with either active gimmick")
{
    bool32 userTera = FALSE;
    PARAMETRIZE { userTera = FALSE; }
    PARAMETRIZE { userTera = TRUE; }
    GIVEN {
        PLAYER(SPECIES_MISDREAVUS) { Ability(ABILITY_NONE); TeraType(TYPE_GHOST); Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); TeraType(TYPE_PSYCHIC); Speed(100); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_SUBSTITUTE, gimmick: userTera ? GIMMICK_NONE : GIMMICK_TERA); MOVE(player, MOVE_CURSE, gimmick: userTera ? GIMMICK_TERA : GIMMICK_NONE); }
    } THEN {
        EXPECT(opponent->volatiles.cursed);
    }
}

DOUBLE_BATTLE_TEST("Polished Curse keeps native target selection and Substitute behavior in doubles")
{
    GIVEN {
        PLAYER(SPECIES_MISDREAVUS) { Ability(ABILITY_NONE); Speed(50); }
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(60); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(70); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(100); }
    } WHEN {
        TURN { MOVE(opponentRight, MOVE_SUBSTITUTE); MOVE(playerLeft, MOVE_CURSE, target: opponentRight); }
    } THEN {
        EXPECT(opponentRight->volatiles.cursed);
        EXPECT(!opponentLeft->volatiles.cursed);
        EXPECT(!playerLeft->volatiles.cursed);
    }
}

SINGLE_BATTLE_TEST("Polished Trick keeps native Unburden behavior for optional Switcheroo")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_UNBURDEN); Item(ITEM_POKE_BALL); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(player, MOVE_SWITCHEROO); }
    } THEN {
        EXPECT(player->volatiles.unburdenActive);
    }
}

SINGLE_BATTLE_TEST("Polished Trick keeps native Unburden behavior with either active gimmick")
{
    bool32 userTera = FALSE;
    PARAMETRIZE { userTera = FALSE; }
    PARAMETRIZE { userTera = TRUE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_UNBURDEN); Item(ITEM_POKE_BALL); TeraType(TYPE_PSYCHIC); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); TeraType(TYPE_PSYCHIC); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_SPLASH, gimmick: userTera ? GIMMICK_NONE : GIMMICK_TERA); MOVE(player, MOVE_TRICK, gimmick: userTera ? GIMMICK_TERA : GIMMICK_NONE); }
    } THEN {
        EXPECT(player->volatiles.unburdenActive);
    }
}

DOUBLE_BATTLE_TEST("Polished Trick keeps native Unburden behavior in doubles")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_UNBURDEN); Item(ITEM_POKE_BALL); }
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_TRICK, target: opponentRight); }
    } THEN {
        EXPECT(playerLeft->volatiles.unburdenActive);
    }
}
