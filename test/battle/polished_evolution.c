#include "global.h"
#include "test/battle.h"
#include "polished_evolution.h"
#include "polished_forms.h"

AI_SINGLE_BATTLE_TEST("Polished patterned Magikarp evolves through the real battle UI and retains its breeding pattern")
{
    GIVEN {
        WITH_CONFIG(POLISHED_FORMS, TRUE);
        WITH_CONFIG(POLISHED_EVOLUTIONS, TRUE);
        WITH_CONFIG(POLISHED_SPECIES_DATA, TRUE);
        WITH_CONFIG(POLISHED_LEVEL_UP_LEARNSETS, TRUE);
        WITH_CONFIG(B_SCALED_EXP, GEN_4);
        AI_FLAGS(0);
        PLAYER(SPECIES_MAGIKARP_RAINDROP) { Level(19); Speed(100); Attack(100); Moves(MOVE_TACKLE, MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Level(70); HP(1); Speed(1); Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Level(10); Speed(1); Moves(MOVE_SPLASH); }
    } WHEN {
        TURN { MOVE(player, MOVE_TACKLE); EXPECT_SEND_OUT(opponent, 1); }
        TURN { MOVE(player, MOVE_SPLASH); }
    } SCENE {
        MESSAGE("Magikarp used Tackle!");
        MESSAGE("Gyarados used Splash!");
    } THEN {
        EXPECT_EQ(player->species, SPECIES_GYARADOS);
        EXPECT_EQ(PolishedGetBoxMonForm(&gParties[B_TRAINER_PLAYER][0].box), 20);
        EXPECT(!PolishedBattleEvolutionIsRunning());
    }
}

AI_SINGLE_BATTLE_TEST("Polished evolution refreshes source types, consumes the held item and runs the new entry ability")
{
    GIVEN {
        WITH_CONFIG(POLISHED_EVOLUTIONS, TRUE);
        WITH_CONFIG(POLISHED_SPECIES_DATA, TRUE);
        WITH_CONFIG(POLISHED_LEVEL_UP_LEARNSETS, TRUE);
        WITH_CONFIG(B_SCALED_EXP, GEN_4);
        AI_FLAGS(0);
        PLAYER(SPECIES_POLIWHIRL) { Level(6); Speed(100); Ability(ABILITY_SWIFT_SWIM); Item(ITEM_KINGS_ROCK); Moves(MOVE_TACKLE, MOVE_SPLASH, MOVE_DOUBLE_SLAP); }
        OPPONENT(SPECIES_WOBBUFFET) { Level(15); HP(1); Speed(1); Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Level(10); Speed(1); Moves(MOVE_SPLASH); }
    } WHEN {
        TURN { MOVE(player, MOVE_TACKLE); EXPECT_SEND_OUT(opponent, 1); }
        TURN { MOVE(player, MOVE_SPLASH); }
    } SCENE {
        MESSAGE("Poliwhirl used Tackle!");
        ABILITY_POPUP(player, ABILITY_DRIZZLE);
        MESSAGE("Politoed used Splash!");
    } THEN {
        EXPECT_EQ(player->species, SPECIES_POLITOED);
        EXPECT_EQ(player->types[1], TYPE_GRASS);
        EXPECT_EQ(player->ability, ABILITY_DRIZZLE);
        EXPECT_EQ(player->item, ITEM_NONE);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_HELD_ITEM), ITEM_NONE);
        EXPECT_EQ(player->moves[3], MOVE_GIGA_DRAIN);
        EXPECT(gBattleWeather & B_WEATHER_RAIN);
    }
}

AI_SINGLE_BATTLE_TEST("Polished evolution resumes the trainer battle with the evolved mon and its evolution move")
{
    GIVEN {
        WITH_CONFIG(POLISHED_EVOLUTIONS, TRUE);
        WITH_CONFIG(POLISHED_LEVEL_UP_LEARNSETS, TRUE);
        WITH_CONFIG(B_SCALED_EXP, GEN_4);
        AI_FLAGS(0);
        PLAYER(SPECIES_CATERPIE) { Level(6); Speed(100); Attack(100); Moves(MOVE_TACKLE, MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Level(15); HP(1); Speed(1); Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Level(10); Speed(1); Moves(MOVE_SPLASH); }
    } WHEN {
        TURN { MOVE(player, MOVE_TACKLE); EXPECT_SEND_OUT(opponent, 1); }
        TURN { MOVE(player, MOVE_SPLASH); }
    } SCENE {
        MESSAGE("Caterpie used Tackle!");
        MESSAGE("Metapod used Splash!");
    } THEN {
        EXPECT_EQ(player->species, SPECIES_METAPOD);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_SPECIES), SPECIES_METAPOD);
        EXPECT_EQ(player->ability, ABILITY_SHED_SKIN);
        EXPECT_EQ(player->moves[2], MOVE_DEFENSE_CURL);
        EXPECT_EQ(gLeveledUpInBattle & 1, 0);
        EXPECT(!PolishedBattleEvolutionIsRunning());
    }
}

AI_SINGLE_BATTLE_TEST("Polished bench evolution preserves the active battler and remaining EXP recipients")
{
    GIVEN {
        WITH_CONFIG(POLISHED_EVOLUTIONS, TRUE);
        WITH_CONFIG(POLISHED_LEVEL_UP_LEARNSETS, TRUE);
        WITH_CONFIG(B_SCALED_EXP, GEN_4);
        WITH_CONFIG(B_SPLIT_EXP, GEN_8);
        AI_FLAGS(0);
        PLAYER(SPECIES_WOBBUFFET) { Level(30); Speed(100); Moves(MOVE_TACKLE, MOVE_SPLASH, MOVE_LIGHT_SCREEN); }
        PLAYER(SPECIES_CATERPIE) { Level(6); Speed(10); Item(ITEM_EXP_SHARE); Moves(MOVE_TACKLE, MOVE_STRING_SHOT); }
        PLAYER(SPECIES_WEEDLE) { Level(6); Speed(10); Item(ITEM_EXP_SHARE); Moves(MOVE_POISON_STING, MOVE_STRING_SHOT); }
        OPPONENT(SPECIES_WOBBUFFET) { Level(15); HP(1); Speed(1); Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Level(10); Speed(1); Moves(MOVE_SPLASH); }
    } WHEN {
        TURN { MOVE(player, MOVE_LIGHT_SCREEN); }
        TURN { MOVE(player, MOVE_TACKLE); EXPECT_SEND_OUT(opponent, 1); }
        TURN { MOVE(player, MOVE_SPLASH); }
    } THEN {
        EXPECT_EQ(player->species, SPECIES_WOBBUFFET);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_SPECIES), SPECIES_METAPOD);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][2], MON_DATA_SPECIES), SPECIES_KAKUNA);
        EXPECT(gSideStatuses[B_SIDE_PLAYER] & SIDE_STATUS_LIGHTSCREEN);
        EXPECT_EQ(gLeveledUpInBattle & 6, 0);
        EXPECT(!PolishedBattleEvolutionIsRunning());
    }
}

WILD_BATTLE_TEST("Polished critical evolution counter counts landed hits, not immune damage rolls")
{
    enum Species species;
    PARAMETRIZE { species = SPECIES_WOBBUFFET; }
    PARAMETRIZE { species = SPECIES_GENGAR; }
    GIVEN {
        WITH_CONFIG(POLISHED_EVOLUTIONS, TRUE);
        PLAYER(SPECIES_FARFETCHD_GALAR) { Attack(10); }
        OPPONENT(species) { HP(1000); Defense(1000); }
    } WHEN {
        TURN { MOVE(player, MOVE_SCRATCH, criticalHit: TRUE); MOVE(opponent, MOVE_SPLASH); }
        TURN { MOVE(player, MOVE_SCRATCH, criticalHit: TRUE); MOVE(opponent, MOVE_SPLASH); }
        TURN { MOVE(player, MOVE_SCRATCH, criticalHit: TRUE); MOVE(opponent, MOVE_SPLASH); }
    } THEN {
        EXPECT_EQ(gPartyCriticalHits[0], species == SPECIES_WOBBUFFET ? 3 : 0);
    }
}

SINGLE_BATTLE_TEST("Polished critical evolution counter excludes link battles")
{
    // SINGLE_BATTLE_TEST uses the engine's recorded-link controller.
    GIVEN {
        WITH_CONFIG(POLISHED_EVOLUTIONS, TRUE);
        PLAYER(SPECIES_FARFETCHD_GALAR);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_SCRATCH, criticalHit: TRUE); MOVE(opponent, MOVE_SPLASH); }
    } THEN {
        EXPECT_EQ(gPartyCriticalHits[0], 0);
    }
}
