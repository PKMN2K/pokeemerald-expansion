#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished Noctowl has its source Ghost immunity in a real battle")
{
    GIVEN {
        WITH_CONFIG(POLISHED_SPECIES_DATA, TRUE);
        PLAYER(SPECIES_NOCTOWL);
        OPPONENT(SPECIES_RATTATA);
    } WHEN {
        TURN { MOVE(opponent, MOVE_TACKLE); }
    } SCENE {
        MESSAGE("It doesn't affect Noctowl…");
        NONE_OF { HP_BAR(player); }
    }
}
