#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished Forewarn uses the source listed power for dynamic and fixed damage moves")
{
    enum Move move = MOVE_RETURN;
    PARAMETRIZE { move = MOVE_RETURN; }
    PARAMETRIZE { move = MOVE_LOW_KICK; }
    PARAMETRIZE { move = MOVE_NIGHT_SHADE; }
    PARAMETRIZE { move = MOVE_SONIC_BOOM; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_FOREWARN); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(move, MOVE_GUST, MOVE_SPLASH); }
    } WHEN { TURN { MOVE(opponent, MOVE_SPLASH); } }
    SCENE {
        ABILITY_POPUP(player, ABILITY_FOREWARN);
        if (B_POLISHED_MOVE_BALANCE) MESSAGE("Forewarn alerted Wobbuffet to the opposing Wobbuffet's Gust!");
        else {
            if (move == MOVE_RETURN) MESSAGE("Forewarn alerted Wobbuffet to the opposing Wobbuffet's Return!");
            if (move == MOVE_LOW_KICK) MESSAGE("Forewarn alerted Wobbuffet to the opposing Wobbuffet's Low Kick!");
            if (move == MOVE_NIGHT_SHADE) MESSAGE("Forewarn alerted Wobbuffet to the opposing Wobbuffet's Night Shade!");
            if (move == MOVE_SONIC_BOOM) MESSAGE("Forewarn alerted Wobbuffet to the opposing Wobbuffet's Sonic Boom!");
        }
    }
}

SINGLE_BATTLE_TEST("Polished Forewarn compares Counter at 120 despite the source comment saying 160")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_FOREWARN); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_COUNTER, MOVE_OVERHEAT, MOVE_SPLASH); }
    } WHEN { TURN { MOVE(opponent, MOVE_SPLASH); } }
    SCENE {
        ABILITY_POPUP(player, ABILITY_FOREWARN);
        MESSAGE("Forewarn alerted Wobbuffet to the opposing Wobbuffet's Overheat!");
    }
}

SINGLE_BATTLE_TEST("Polished Forewarn does not activate against only status moves")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_FOREWARN); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_GROWL, MOVE_SPLASH); }
    } WHEN { TURN { MOVE(opponent, MOVE_SPLASH); } }
    SCENE {
        if (B_POLISHED_MOVE_BALANCE) NOT ABILITY_POPUP(player, ABILITY_FOREWARN);
        else ABILITY_POPUP(player, ABILITY_FOREWARN);
    }
}

SINGLE_BATTLE_TEST("Polished Forewarn chooses equal source powers uniformly")
{
    PASSES_RANDOMLY(1, 3, RNG_FOREWARN);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_FOREWARN); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_RETURN, MOVE_LOW_KICK, MOVE_NIGHT_SHADE, MOVE_SPLASH); }
    } WHEN { TURN { MOVE(opponent, MOVE_SPLASH); } }
    SCENE { MESSAGE("Forewarn alerted Wobbuffet to the opposing Wobbuffet's Return!"); }
}

SINGLE_BATTLE_TEST("Polished Forewarn retains native handling when optional moves are present")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_FOREWARN); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_FISSURE, MOVE_OVERHEAT, MOVE_SPLASH); }
    } WHEN { TURN { MOVE(opponent, MOVE_SPLASH); } }
    SCENE { MESSAGE("Forewarn alerted Wobbuffet to the opposing Wobbuffet's Fissure!"); }
}

SINGLE_BATTLE_TEST("Polished Anticipation ignores Thunder Wave despite its super effective type")
{
    GIVEN {
        PLAYER(SPECIES_MAGIKARP) { Ability(ABILITY_ANTICIPATION); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_THUNDER_WAVE, MOVE_SPLASH); }
    } WHEN { TURN { MOVE(opponent, MOVE_SPLASH); } }
    SCENE {
        if (B_POLISHED_MOVE_BALANCE) NOT ABILITY_POPUP(player, ABILITY_ANTICIPATION);
        else ABILITY_POPUP(player, ABILITY_ANTICIPATION);
    }
}
