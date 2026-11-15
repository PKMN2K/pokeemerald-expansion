#include "global.h"
#include "test/battle.h"
#include "polished_trapping_vectors.h"

SINGLE_BATTLE_TEST("Polished trapping abilities follow the source type and mutual-cancellation matrix")
{
    const struct PolishedTrappingVector *v = &sPolishedTrappingVectors[0];
    for (u32 row = 0; row < ARRAY_COUNT(sPolishedTrappingVectors); row++)
        PARAMETRIZE { v = &sPolishedTrappingVectors[row]; }
    GIVEN {
        ASSUME(B_SHADOW_TAG_ESCAPE >= GEN_4);
        WITH_CONFIG(B_GHOSTS_ESCAPE, GEN_6);
        PLAYER(v->species[0]) { Ability(v->abilities[0]); }
        OPPONENT(v->species[1]) { Ability(v->abilities[1]); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); MOVE(opponent, MOVE_CELEBRATE); }
    } THEN {
        u32 mask = B_POLISHED_MOVE_BALANCE ? v->sourceMask : v->nativeMask;
        enum BattlerId p = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT), o = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);
        EXPECT_EQ(IsAbilityPreventingEscape(p), mask & 1 ? o + 1 : 0);
        EXPECT_EQ(IsAbilityPreventingEscape(o), mask & 2 ? p + 1 : 0);
    }
}

SINGLE_BATTLE_TEST("Polished trapping abilities respect Neutralizing Gas in both directions")
{
    bool32 gasPlayer = TRUE;
    enum Ability trap = ABILITY_ARENA_TRAP;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 3; kind++)
            PARAMETRIZE { gasPlayer = side; trap = kind == 0 ? ABILITY_ARENA_TRAP : kind == 1 ? ABILITY_MAGNET_PULL : ABILITY_SHADOW_TAG; }
    GIVEN {
        PLAYER(SPECIES_MAGNEMITE) { Ability(gasPlayer ? ABILITY_NEUTRALIZING_GAS : trap); }
        OPPONENT(SPECIES_MAGNEMITE) { Ability(gasPlayer ? trap : ABILITY_NEUTRALIZING_GAS); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); MOVE(opponent, MOVE_CELEBRATE); }
    } THEN {
        EXPECT_EQ(IsAbilityPreventingEscape(GetBattlerAtPosition(B_POSITION_PLAYER_LEFT)), 0);
        EXPECT_EQ(IsAbilityPreventingEscape(GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT)), 0);
    }
}

SINGLE_BATTLE_TEST("Polished trapping abilities use live Flying Levitate Air Balloon and Iron Ball eligibility")
{
    struct BattlePokemon *user = player;
    u32 kind = 0;
    for (u32 side = 0; side < 2; side++)
        for (u32 row = 0; row < 6; row++)
            PARAMETRIZE { user = side ? opponent : player; kind = row; }
    enum Species species = kind == 1 || kind == 4 ? SPECIES_SKARMORY : SPECIES_MAGNEMITE;
    enum Ability ability = kind == 2 || kind == 5 ? ABILITY_LEVITATE : ABILITY_ARENA_TRAP;
    enum Item item = kind == 3 ? ITEM_AIR_BALLOON : kind >= 4 ? ITEM_IRON_BALL : ITEM_NONE;
    bool32 grounded = kind == 0 || kind >= 4;
    GIVEN {
        PLAYER(user == player ? species : SPECIES_MAGNEMITE) { Ability(user == player ? ability : ABILITY_ARENA_TRAP); Item(user == player ? item : ITEM_NONE); }
        OPPONENT(user == opponent ? species : SPECIES_MAGNEMITE) { Ability(user == opponent ? ability : ABILITY_ARENA_TRAP); Item(user == opponent ? item : ITEM_NONE); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); MOVE(opponent, MOVE_CELEBRATE); }
    } THEN {
        enum BattlerId u = GetBattlerAtPosition(user == player ? B_POSITION_PLAYER_LEFT : B_POSITION_OPPONENT_LEFT), f = GetOppositeBattler(u);
        bool32 mutual = grounded && ability == ABILITY_ARENA_TRAP;
        EXPECT_EQ(IsAbilityPreventingEscape(u), grounded && !(B_POLISHED_MOVE_BALANCE && mutual) ? f + 1 : 0);
        EXPECT_EQ(IsAbilityPreventingEscape(f), ability == ABILITY_ARENA_TRAP && !(B_POLISHED_MOVE_BALANCE && mutual) ? u + 1 : 0);
    }
}

SINGLE_BATTLE_TEST("Polished trapping abilities recompute eligibility when one trapping ability is suppressed")
{
    struct BattlePokemon *user = player, *foe = opponent;
    PARAMETRIZE { user = player; foe = opponent; }
    PARAMETRIZE { user = opponent; foe = player; }
    GIVEN {
        PLAYER(SPECIES_MAGNEMITE) { Ability(ABILITY_ARENA_TRAP); }
        OPPONENT(SPECIES_MAGNEMITE) { Ability(ABILITY_ARENA_TRAP); }
    } WHEN {
        TURN { MOVE(foe, MOVE_GASTRO_ACID); }
    } THEN {
        enum BattlerId u = GetBattlerAtPosition(user == player ? B_POSITION_PLAYER_LEFT : B_POSITION_OPPONENT_LEFT), f = GetOppositeBattler(u);
        EXPECT_EQ(IsAbilityPreventingEscape(u), f + 1);
        EXPECT_EQ(IsAbilityPreventingEscape(f), 0);
    }
}

SINGLE_BATTLE_TEST("Polished trapping abilities cancellation does not clear Mean Look or Wrap")
{
    struct BattlePokemon *user = player, *foe = opponent;
    enum Move move = MOVE_MEAN_LOOK;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 2; kind++)
            PARAMETRIZE { user = side ? opponent : player; foe = side ? player : opponent; move = kind ? MOVE_WRAP : MOVE_MEAN_LOOK; }
    GIVEN {
        PLAYER(SPECIES_MAGNEMITE) { Ability(ABILITY_ARENA_TRAP); HP(1000); MaxHP(1000); }
        OPPONENT(SPECIES_MAGNEMITE) { Ability(ABILITY_ARENA_TRAP); HP(1000); MaxHP(1000); }
    } WHEN {
        TURN { MOVE(foe, move, hit: TRUE); }
    } THEN {
        enum BattlerId u = GetBattlerAtPosition(user == player ? B_POSITION_PLAYER_LEFT : B_POSITION_OPPONENT_LEFT);
        EXPECT_EQ(IsAbilityPreventingEscape(u), B_POLISHED_MOVE_BALANCE ? 0 : GetOppositeBattler(u) + 1);
        EXPECT_EQ(CanBattlerEscape(u), FALSE);
        EXPECT_EQ((u32)user->volatiles.wrapped, move == MOVE_WRAP);
        EXPECT_EQ((u32)user->volatiles.escapePrevention, move == MOVE_MEAN_LOOK);
    }
}

SINGLE_BATTLE_TEST("Polished trapping abilities retain native gates during either side's Terastal")
{
    enum Ability a = ABILITY_ARENA_TRAP, b = ABILITY_ARENA_TRAP;
    bool32 playerTera = TRUE;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 3; kind++)
            PARAMETRIZE { playerTera = side; a = kind == 1 ? ABILITY_MAGNET_PULL : ABILITY_ARENA_TRAP; b = kind == 0 ? ABILITY_ARENA_TRAP : kind == 1 ? ABILITY_MAGNET_PULL : ABILITY_SHADOW_TAG; }
    GIVEN {
        PLAYER(SPECIES_MAGNEMITE) { Ability(a); TeraType(TYPE_STEEL); }
        OPPONENT(SPECIES_MAGNEMITE) { Ability(b); TeraType(TYPE_STEEL); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE, gimmick: playerTera ? GIMMICK_TERA : GIMMICK_NONE); MOVE(opponent, MOVE_CELEBRATE, gimmick: playerTera ? GIMMICK_NONE : GIMMICK_TERA); }
    } THEN {
        enum BattlerId p = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT), o = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);
        EXPECT_EQ(IsAbilityPreventingEscape(p), o + 1);
        EXPECT_EQ(IsAbilityPreventingEscape(o), p + 1);
    }
}

DOUBLE_BATTLE_TEST("Polished trapping abilities retain native gates in optional doubles")
{
    enum Ability a = ABILITY_ARENA_TRAP, b = ABILITY_ARENA_TRAP;
    for (u32 reverse = 0; reverse < 2; reverse++)
        for (u32 kind = 0; kind < 3; kind++)
            PARAMETRIZE { a = kind == 1 ? ABILITY_MAGNET_PULL : ABILITY_ARENA_TRAP; b = kind == 0 ? ABILITY_ARENA_TRAP : kind == 1 ? ABILITY_MAGNET_PULL : ABILITY_SHADOW_TAG; if (reverse) { enum Ability tmp = a; a = b; b = tmp; } }
    GIVEN {
        PLAYER(SPECIES_MAGNEMITE) { Ability(a); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_KEEN_EYE); }
        OPPONENT(SPECIES_MAGNEMITE) { Ability(b); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_KEEN_EYE); }
    } WHEN {
        TURN {}
    } THEN {
        enum BattlerId p = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT), o = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);
        EXPECT_EQ(IsAbilityPreventingEscape(p), o + 1);
        EXPECT_EQ(IsAbilityPreventingEscape(o), p + 1);
    }
}

AI_SINGLE_BATTLE_TEST("Polished trapping abilities allow AI switching when mutual traps cancel")
{
    enum Ability a = ABILITY_ARENA_TRAP, b = ABILITY_ARENA_TRAP;
    for (u32 kind = 0; kind < 3; kind++)
        PARAMETRIZE { a = kind == 1 ? ABILITY_MAGNET_PULL : ABILITY_ARENA_TRAP; b = kind == 0 ? ABILITY_ARENA_TRAP : kind == 1 ? ABILITY_MAGNET_PULL : ABILITY_SHADOW_TAG; }
    GIVEN {
        AI_FLAGS(AI_FLAG_CHECK_BAD_MOVE | AI_FLAG_CHECK_VIABILITY | AI_FLAG_TRY_TO_FAINT);
        ASSUME(SHOULD_SWITCH_PERISH_SONG_PERCENTAGE == 100);
        PLAYER(SPECIES_MAGNEMITE) { Ability(a); Item(ITEM_SHED_SHELL); Speed(100); HP(1000); MaxHP(1000); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_KEEN_EYE); Speed(75); }
        OPPONENT(SPECIES_MAGNEMITE) { Ability(b); Moves(MOVE_SCRATCH); Speed(50); HP(1000); MaxHP(1000); }
        OPPONENT(SPECIES_CROBAT) { Ability(ABILITY_KEEN_EYE); Moves(MOVE_SCRATCH); Speed(150); HP(1000); MaxHP(1000); }
    } WHEN {
        TURN { MOVE(player, MOVE_PERISH_SONG); EXPECT_MOVE(opponent, MOVE_SCRATCH); }
        TURN { EXPECT_MOVE(opponent, MOVE_SCRATCH); }
        TURN { EXPECT_MOVE(opponent, MOVE_SCRATCH); }
        TURN {
            SWITCH(player, 1);
            if (B_POLISHED_MOVE_BALANCE)
                EXPECT_SWITCH(opponent, 1);
            else {
                EXPECT_MOVE(opponent, MOVE_SCRATCH);
                EXPECT_SEND_OUT(opponent, 1);
            }
        }
    } THEN {
        EXPECT_EQ(opponent->species, SPECIES_CROBAT);
    }
}
