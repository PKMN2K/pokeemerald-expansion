#include "global.h"
#include "test/battle.h"

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
