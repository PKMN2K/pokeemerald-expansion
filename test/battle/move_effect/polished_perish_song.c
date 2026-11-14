#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished Perish Song separates user countdown from opponent ability immunity")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Ability userAbility = ABILITY_NONE, targetAbility = ABILITY_NONE;
    for (u32 side = 0; side < 2; side++)
        for (u32 ability = 0; ability < 3; ability++)
            for (u32 immunity = 0; immunity < 3; immunity++)
                PARAMETRIZE {
                    user = side ? opponent : player; target = side ? player : opponent;
                    userAbility = ability == 0 ? ABILITY_NONE : ability == 1 ? ABILITY_PRANKSTER : ABILITY_MOLD_BREAKER;
                    targetAbility = immunity == 0 ? ABILITY_NONE : immunity == 1 ? ABILITY_SOUNDPROOF : ABILITY_ARMOR_TAIL;
                }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? userAbility : targetAbility); Speed(100); MovesWithPP({MOVE_PERISH_SONG, 5}, {MOVE_CELEBRATE, 10}); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? userAbility : targetAbility); Speed(50); MovesWithPP({MOVE_PERISH_SONG, 5}, {MOVE_CELEBRATE, 10}); }
    } WHEN {
        TURN { MOVE(user, MOVE_PERISH_SONG); }
    } SCENE {
        if (B_POLISHED_MOVE_BALANCE || userAbility != ABILITY_PRANKSTER || targetAbility != ABILITY_ARMOR_TAIL)
            ANIMATION(ANIM_TYPE_MOVE, MOVE_PERISH_SONG, user);
        else
            NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_PERISH_SONG, user);
    } THEN {
        bool32 armorBlocks = userAbility == ABILITY_PRANKSTER && targetAbility == ABILITY_ARMOR_TAIL;
        bool32 soundBlocks = targetAbility == ABILITY_SOUNDPROOF && userAbility != ABILITY_MOLD_BREAKER
            && !(B_POLISHED_MOVE_BALANCE && userAbility == ABILITY_PRANKSTER);
        EXPECT_EQ((u32)user->volatiles.perishSong, B_POLISHED_MOVE_BALANCE || !armorBlocks);
        EXPECT_EQ((u32)user->volatiles.perishSongTimer, B_POLISHED_MOVE_BALANCE || !armorBlocks ? 2 : 0);
        EXPECT_EQ((u32)target->volatiles.perishSong, !armorBlocks && !soundBlocks);
        EXPECT_EQ((u32)target->volatiles.perishSongTimer, !armorBlocks && !soundBlocks ? 2 : 0);
        EXPECT_EQ(user->pp[0], 4);
    }
}

SINGLE_BATTLE_TEST("Polished Perish Song keeps Prankster Dark immunity during Fly and Dig")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move setup = MOVE_CELEBRATE;
    for (u32 side = 0; side < 2; side++)
        for (u32 hidden = 0; hidden < 3; hidden++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; setup = hidden == 0 ? MOVE_CELEBRATE : hidden == 1 ? MOVE_FLY : MOVE_DIG; }
    GIVEN {
        PLAYER(user == player ? SPECIES_WOBBUFFET : SPECIES_UMBREON) { Ability(user == player ? ABILITY_PRANKSTER : ABILITY_NONE); Speed(user == player ? 100 : 50); }
        OPPONENT(user == opponent ? SPECIES_WOBBUFFET : SPECIES_UMBREON) { Ability(user == opponent ? ABILITY_PRANKSTER : ABILITY_NONE); Speed(user == opponent ? 100 : 50); }
    } WHEN {
        TURN { MOVE(target, setup); }
        TURN { MOVE(user, MOVE_PERISH_SONG); if (setup != MOVE_CELEBRATE) SKIP_TURN(target); }
    } THEN {
        EXPECT_EQ((u32)user->volatiles.perishSong, TRUE);
        EXPECT_EQ((u32)target->volatiles.perishSong, FALSE);
    }
}

SINGLE_BATTLE_TEST("Polished Perish Song countdown reaches zero on the fourth turn including use")
{
    struct BattlePokemon *user = player, *target = opponent;
    u32 turns = 1;
    for (u32 side = 0; side < 2; side++)
        for (u32 count = 1; count <= 4; count++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; turns = count; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_NONE : ABILITY_SOUNDPROOF); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_NONE : ABILITY_SOUNDPROOF); Speed(50); }
    } WHEN {
        TURN { MOVE(user, MOVE_PERISH_SONG); }
        for (u32 i = 1; i < turns; i++)
            TURN {}
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_PERISH_SONG, user);
        if (turns == 4)
            HP_BAR(user, hp: 0);
        else
            NONE_OF { HP_BAR(user); HP_BAR(target); }
    } THEN {
        if (turns == 4)
            EXPECT_EQ(user->hp, 0);
        else
        {
            EXPECT_GT(user->hp, 0);
            EXPECT_EQ((u32)user->volatiles.perishSongTimer, 3 - turns);
        }
        EXPECT_EQ((u32)target->volatiles.perishSong, FALSE);
    }
}

SINGLE_BATTLE_TEST("Polished Perish Song repeat fails without refreshing either countdown")
{
    struct BattlePokemon *user = player;
    PARAMETRIZE { user = player; }
    PARAMETRIZE { user = opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); MovesWithPP({MOVE_PERISH_SONG, 2}, {MOVE_CELEBRATE, 10}); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); MovesWithPP({MOVE_PERISH_SONG, 2}, {MOVE_CELEBRATE, 10}); }
    } WHEN {
        TURN { MOVE(user, MOVE_PERISH_SONG); }
        TURN { MOVE(user, MOVE_PERISH_SONG); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_PERISH_SONG, user);
        MESSAGE("But it failed!");
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_PERISH_SONG, user);
    } THEN {
        EXPECT_EQ((u32)player->volatiles.perishSongTimer, 1);
        EXPECT_EQ((u32)opponent->volatiles.perishSongTimer, 1);
        EXPECT_EQ(user->pp[0], 0);
    }
}

SINGLE_BATTLE_TEST("Polished Perish Song clears on switching and can afflict only the new opponent")
{
    struct BattlePokemon *user = player, *target = opponent;
    PARAMETRIZE { user = player; target = opponent; }
    PARAMETRIZE { user = opponent; target = player; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(75); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(25); }
    } WHEN {
        TURN { MOVE(user, MOVE_PERISH_SONG); }
        TURN { SWITCH(target, 1); }
        TURN { MOVE(user, MOVE_PERISH_SONG); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_PERISH_SONG, user);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_PERISH_SONG, user);
    } THEN {
        EXPECT_EQ((u32)user->volatiles.perishSongTimer, 0);
        EXPECT_EQ((u32)target->volatiles.perishSongTimer, 2);
        EXPECT_EQ((u32)user->volatiles.perishSong, TRUE);
        EXPECT_EQ((u32)target->volatiles.perishSong, TRUE);
    }
}

SINGLE_BATTLE_TEST("Polished Perish Song bypasses Protect Substitute and hiding without Prankster")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move setup = MOVE_PROTECT;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 4; kind++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; setup = kind == 0 ? MOVE_PROTECT : kind == 1 ? MOVE_SUBSTITUTE : kind == 2 ? MOVE_DIG : MOVE_FLY; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(user == player ? 100 : 50); MaxHP(1000); HP(1000); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(user == opponent ? 100 : 50); MaxHP(1000); HP(1000); }
    } WHEN {
        if (setup != MOVE_PROTECT)
            TURN { MOVE(target, setup); }
        TURN { MOVE(user, MOVE_PERISH_SONG); if (setup == MOVE_PROTECT) MOVE(target, setup); if (setup == MOVE_DIG || setup == MOVE_FLY) SKIP_TURN(target); }
    } THEN {
        EXPECT_EQ((u32)user->volatiles.perishSong, TRUE);
        EXPECT_EQ((u32)target->volatiles.perishSong, B_POLISHED_MOVE_BALANCE || setup == MOVE_PROTECT || setup == MOVE_SUBSTITUTE);
        EXPECT_EQ((u32)user->volatiles.perishSongTimer, 2);
        EXPECT_EQ((u32)target->volatiles.perishSongTimer, B_POLISHED_MOVE_BALANCE || setup == MOVE_PROTECT || setup == MOVE_SUBSTITUTE ? 2 : 0);
    }
}

SINGLE_BATTLE_TEST("Polished Perish Song still afflicts its Soundproof user")
{
    struct BattlePokemon *user = player;
    u32 config = GEN_3;
    for (u32 side = 0; side < 2; side++)
        for (u32 old = 0; old < 2; old++)
            PARAMETRIZE { user = side ? opponent : player; config = old ? GEN_3 : GEN_LATEST; }
    GIVEN {
        WITH_CONFIG(B_CHECK_USER_FAILURE, config);
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_SOUNDPROOF : ABILITY_NONE); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_SOUNDPROOF : ABILITY_NONE); Speed(50); }
    } WHEN {
        TURN { MOVE(user, MOVE_PERISH_SONG); }
    } THEN {
        EXPECT_EQ((u32)user->volatiles.perishSong, B_POLISHED_MOVE_BALANCE || config >= GEN_5);
    }
}

SINGLE_BATTLE_TEST("Polished Perish Song retains native ability blocking during active Terastal")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Ability immunity = ABILITY_SOUNDPROOF;
    for (u32 side = 0; side < 2; side++)
        for (u32 armor = 0; armor < 2; armor++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; immunity = armor ? ABILITY_ARMOR_TAIL : ABILITY_SOUNDPROOF; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_PRANKSTER : immunity); Speed(100); TeraType(TYPE_NORMAL); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_PRANKSTER : immunity); Speed(50); TeraType(TYPE_NORMAL); }
    } WHEN {
        TURN { MOVE(user, MOVE_PERISH_SONG, gimmick: GIMMICK_TERA); }
    } THEN {
        EXPECT_EQ((u32)user->volatiles.perishSong, immunity != ABILITY_ARMOR_TAIL);
        EXPECT_EQ((u32)target->volatiles.perishSong, FALSE);
    }
}

DOUBLE_BATTLE_TEST("Polished Perish Song retains native ability blocking in optional doubles")
{
    struct BattlePokemon *user = playerLeft, *target = opponentLeft;
    enum Ability immunity = ABILITY_SOUNDPROOF;
    for (u32 side = 0; side < 2; side++)
        for (u32 armor = 0; armor < 2; armor++)
            PARAMETRIZE { user = side ? opponentLeft : playerLeft; target = side ? playerLeft : opponentLeft; immunity = armor ? ABILITY_ARMOR_TAIL : ABILITY_SOUNDPROOF; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == playerLeft ? ABILITY_PRANKSTER : immunity); Speed(100); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(75); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponentLeft ? ABILITY_PRANKSTER : immunity); Speed(50); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(25); }
    } WHEN {
        TURN { MOVE(user, MOVE_PERISH_SONG); }
    } THEN {
        EXPECT_EQ((u32)user->volatiles.perishSong, immunity != ABILITY_ARMOR_TAIL);
        EXPECT_EQ((u32)target->volatiles.perishSong, FALSE);
        EXPECT_EQ((u32)playerRight->volatiles.perishSong, immunity != ABILITY_ARMOR_TAIL);
        EXPECT_EQ((u32)opponentRight->volatiles.perishSong, immunity != ABILITY_ARMOR_TAIL);
    }
}
