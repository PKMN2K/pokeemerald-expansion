#include "global.h"
#include "test/battle.h"

ROAMING_BATTLE_TEST("Polished Roar roaming level exemption belongs only to the opponent")
{
    struct BattlePokemon *user = player;
    u32 level = 49;
    bool32 bench = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 delta = 0; delta < 3; delta++)
            for (u32 reserve = 0; reserve < 2; reserve++)
                PARAMETRIZE { user = side ? opponent : player; level = 49 + delta; bench = reserve; }
    bool32 succeeds = level >= 50 || (B_POLISHED_MOVE_BALANCE && user == opponent);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(user == player ? level : 50); Speed(100); Ability(ABILITY_KEEN_EYE); }
        if (bench) PLAYER(SPECIES_WYNAUT) { Speed(75); Ability(ABILITY_KEEN_EYE); }
        OPPONENT(SPECIES_WOBBUFFET) { Level(user == opponent ? level : 50); Speed(50); HP(720); MaxHP(720); Ability(ABILITY_KEEN_EYE); Moves(user == opponent ? MOVE_ROAR : MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(user, MOVE_ROAR); }
        if (!succeeds) TURN { MOVE(player, MOVE_TACKLE, hit: TRUE); }
    } SCENE {
        if (succeeds) ANIMATION(ANIM_TYPE_MOVE, MOVE_ROAR, user);
        else { MESSAGE("But it failed!"); ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player); HP_BAR(opponent); }
    } THEN {
        EXPECT(gBattleTypeFlags & BATTLE_TYPE_ROAMER);
        EXPECT(!(gBattleTypeFlags & BATTLE_TYPE_TRAINER));
        if (succeeds) EXPECT_EQ(gBattleOutcome, user == player ? B_OUTCOME_PLAYER_TELEPORTED : B_OUTCOME_MON_TELEPORTED);
    }
}

ROAMING_BATTLE_TEST("Polished Roar roaming exemption still respects ability gates")
{
    enum Ability userAbility = ABILITY_KEEN_EYE, targetAbility = ABILITY_KEEN_EYE;
    u32 level = 49;
    for (u32 delta = 0; delta < 2; delta++)
        for (u32 userKind = 0; userKind < 3; userKind++)
            for (u32 targetKind = 0; targetKind < 3; targetKind++)
                PARAMETRIZE {
                    level = 49 + delta;
                    userAbility = userKind == 2 ? ABILITY_PRANKSTER : userKind ? ABILITY_MOLD_BREAKER : ABILITY_KEEN_EYE;
                    targetAbility = targetKind == 2 ? ABILITY_SOUNDPROOF : targetKind ? ABILITY_SUCTION_CUPS : ABILITY_KEEN_EYE;
                }
    bool32 blocked = targetAbility != ABILITY_KEEN_EYE && userAbility != ABILITY_MOLD_BREAKER
        && !(B_POLISHED_MOVE_BALANCE && userAbility == ABILITY_PRANKSTER && targetAbility == ABILITY_SOUNDPROOF);
    bool32 succeeds = !blocked && (level >= 50 || B_POLISHED_MOVE_BALANCE);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(50); Speed(100); Ability(targetAbility); }
        OPPONENT(SPECIES_WOBBUFFET) { Level(level); Speed(50); HP(720); MaxHP(720); Ability(userAbility); Moves(MOVE_ROAR); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_ROAR); }
        if (!succeeds) TURN { MOVE(player, MOVE_TACKLE, hit: TRUE); }
    } SCENE {
        if (succeeds) ANIMATION(ANIM_TYPE_MOVE, MOVE_ROAR, opponent);
        else {
            NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_ROAR, opponent);
            ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player); HP_BAR(opponent);
        }
    } THEN {
        if (succeeds) EXPECT_EQ(gBattleOutcome, B_OUTCOME_MON_TELEPORTED);
    }
}

ROAMING_BATTLE_TEST("Polished Roar roaming exemption respects Protect and bypasses Substitute")
{
    bool32 protect = FALSE, substitute = FALSE;
    u32 level = 49;
    for (u32 delta = 0; delta < 2; delta++)
        for (u32 shield = 0; shield < 2; shield++)
            for (u32 sub = 0; sub < 2; sub++)
                PARAMETRIZE { level = 49 + delta; protect = shield; substitute = sub; }
    bool32 succeeds = B_POLISHED_MOVE_BALANCE ? !protect : level >= 50;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(50); Speed(100); Ability(ABILITY_KEEN_EYE); }
        OPPONENT(SPECIES_WOBBUFFET) { Level(level); Speed(50); HP(720); MaxHP(720); Ability(ABILITY_KEEN_EYE); Moves(MOVE_CELEBRATE, MOVE_ROAR); }
    } WHEN {
        if (substitute) TURN { MOVE(player, MOVE_SUBSTITUTE); MOVE(opponent, MOVE_CELEBRATE); }
        TURN { MOVE(player, protect ? MOVE_PROTECT : MOVE_CELEBRATE); MOVE(opponent, MOVE_ROAR); }
        if (!succeeds) TURN { MOVE(player, MOVE_TACKLE, hit: TRUE); MOVE(opponent, MOVE_CELEBRATE); }
    } SCENE {
        if (substitute) ANIMATION(ANIM_TYPE_MOVE, MOVE_SUBSTITUTE, player);
        if (protect) ANIMATION(ANIM_TYPE_MOVE, MOVE_PROTECT, player);
        if (succeeds) ANIMATION(ANIM_TYPE_MOVE, MOVE_ROAR, opponent);
        else {
            NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_ROAR, opponent);
            ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player); HP_BAR(opponent);
        }
    } THEN {
        if (succeeds) EXPECT_EQ(gBattleOutcome, B_OUTCOME_MON_TELEPORTED);
    }
}

ROAMING_BATTLE_TEST("Polished Roar roaming exemption excludes Whirlwind and either active Terastal")
{
    u32 kind = 0, level = 49;
    for (u32 variant = 0; variant < 3; variant++)
        for (u32 delta = 0; delta < 2; delta++)
            PARAMETRIZE { kind = variant; level = 49 + delta; }
    enum Move move = kind == 0 ? MOVE_WHIRLWIND : MOVE_ROAR;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(50); Speed(100); Ability(ABILITY_KEEN_EYE); TeraType(TYPE_NORMAL); }
        OPPONENT(SPECIES_WOBBUFFET) { Level(level); Speed(50); HP(720); MaxHP(720); Ability(ABILITY_KEEN_EYE); TeraType(TYPE_NORMAL); Moves(move, MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE, gimmick: kind == 2 ? GIMMICK_TERA : GIMMICK_NONE); MOVE(opponent, move, gimmick: kind == 1 ? GIMMICK_TERA : GIMMICK_NONE); }
        if (level < 50) TURN { MOVE(player, MOVE_TACKLE, hit: TRUE); MOVE(opponent, MOVE_CELEBRATE); }
    } SCENE {
        if (level >= 50) ANIMATION(ANIM_TYPE_MOVE, move, opponent);
        else { MESSAGE("But it failed!"); ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player); HP_BAR(opponent); }
    } THEN {
        if (level >= 50) EXPECT_EQ(gBattleOutcome, B_OUTCOME_MON_TELEPORTED);
    }
}

ROAMING_BATTLE_TEST("Polished Roar roaming exemption follows the user after Magic Bounce")
{
    struct BattlePokemon *user = player, *reflector = opponent;
    u32 playerLevel = 49;
    for (u32 side = 0; side < 2; side++)
        for (u32 delta = 0; delta < 2; delta++)
            PARAMETRIZE { user = side ? opponent : player; reflector = side ? player : opponent; playerLevel = 49 + 2 * delta; }
    bool32 succeeds = reflector == opponent ? (playerLevel < 50 || B_POLISHED_MOVE_BALANCE) : playerLevel > 50;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(playerLevel); Speed(100); Ability(reflector == player ? ABILITY_MAGIC_BOUNCE : ABILITY_KEEN_EYE); }
        OPPONENT(SPECIES_WOBBUFFET) { Level(50); Speed(50); HP(720); MaxHP(720); Ability(reflector == opponent ? ABILITY_MAGIC_BOUNCE : ABILITY_KEEN_EYE); Moves(user == opponent ? MOVE_ROAR : MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(user, MOVE_ROAR); }
        if (!succeeds) TURN { MOVE(player, MOVE_TACKLE, hit: TRUE); }
    } SCENE {
        ABILITY_POPUP(reflector, ABILITY_MAGIC_BOUNCE);
        if (succeeds) ANIMATION(ANIM_TYPE_MOVE, MOVE_ROAR, reflector);
        else { MESSAGE("But it failed!"); ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player, target: opponent); HP_BAR(opponent); }
    } THEN {
        if (succeeds) EXPECT_EQ(gBattleOutcome, reflector == player ? B_OUTCOME_PLAYER_TELEPORTED : B_OUTCOME_MON_TELEPORTED);
    }
}

SINGLE_BATTLE_TEST("Polished Roar reflection does not skip the reflector's next attack")
{
    struct BattlePokemon *user = player, *reflector = opponent;
    enum Move move = MOVE_ROAR;
    bool32 coat = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 2; kind++)
            for (u32 mechanism = 0; mechanism < 2; mechanism++)
                PARAMETRIZE { user = side ? opponent : player; reflector = side ? player : opponent; move = kind ? MOVE_GROWL : MOVE_ROAR; coat = mechanism; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); HP(720); MaxHP(720); Ability(!coat && reflector == player ? ABILITY_MAGIC_BOUNCE : ABILITY_KEEN_EYE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); HP(720); MaxHP(720); Ability(!coat && reflector == opponent ? ABILITY_MAGIC_BOUNCE : ABILITY_KEEN_EYE); }
    } WHEN {
        TURN { if (coat) MOVE(reflector, MOVE_MAGIC_COAT); MOVE(user, move); }
        TURN { MOVE(reflector, MOVE_TACKLE, hit: TRUE); }
    } SCENE {
        if (coat) ANIMATION(ANIM_TYPE_MOVE, MOVE_MAGIC_COAT, reflector);
        else ABILITY_POPUP(reflector, ABILITY_MAGIC_BOUNCE);
        if (move == MOVE_ROAR) MESSAGE("But it failed!");
        else ANIMATION(ANIM_TYPE_MOVE, MOVE_GROWL, reflector);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, reflector, target: user);
        HP_BAR(user);
    } THEN {
        EXPECT_LT(user->hp, 720);
    }
}
