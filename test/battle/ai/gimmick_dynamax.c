#include "global.h"
#include "test/battle.h"
#include "battle_ai_util.h"

AI_SINGLE_BATTLE_TEST("AI uses Dynamax")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_CHECK_BAD_MOVE | AI_FLAG_CHECK_VIABILITY | AI_FLAG_TRY_TO_FAINT | AI_FLAG_OMNISCIENT );
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_SCRATCH); DynamaxLevel(10); }
    } WHEN {
        TURN { EXPECT_MOVE(opponent, MOVE_SCRATCH, gimmick: GIMMICK_DYNAMAX); }
    }
}

AI_SINGLE_BATTLE_TEST("AI uses Dynamax -- Max Moves are scored based on max move effects, not base effects")
{
    enum Move baseMove;
    PARAMETRIZE { baseMove = MOVE_EXPLOSION; }
    PARAMETRIZE { baseMove = MOVE_HYPER_BEAM; }
    GIVEN {
        AI_FLAGS(AI_FLAG_CHECK_BAD_MOVE | AI_FLAG_CHECK_VIABILITY | AI_FLAG_TRY_TO_FAINT | AI_FLAG_OMNISCIENT );
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_SCRATCH, baseMove); DynamaxLevel(10); }
    } WHEN {
        TURN { EXPECT_MOVE(opponent, baseMove, gimmick: GIMMICK_DYNAMAX); }
    }
}

AI_SINGLE_BATTLE_TEST("AI Max Move damage retains the base move's physical or special category")
{
    bool32 physical;
    PARAMETRIZE { physical = TRUE; }
    PARAMETRIZE { physical = FALSE; }
    GIVEN {
        AI_FLAGS(AI_FLAG_CHECK_BAD_MOVE | AI_FLAG_CHECK_VIABILITY | AI_FLAG_TRY_TO_FAINT | AI_FLAG_OMNISCIENT);
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(2000); Defense(1000); SpDefense(1000); }
        OPPONENT(SPECIES_WOBBUFFET) {
            Moves(MOVE_SCRATCH, MOVE_HYPER_BEAM);
            Attack(physical ? 300 : 10);
            SpAttack(physical ? 10 : 300);
            DynamaxLevel(10);
        }
    } WHEN {
        TURN { EXPECT_MOVE(opponent, physical ? MOVE_SCRATCH : MOVE_HYPER_BEAM, gimmick: GIMMICK_DYNAMAX); }
    } THEN {
        enum BattlerId atk = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);
        enum BattlerId def = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);
        EXPECT_EQ(gAiLogicData->simulatedDmg[atk][def][0].median > gAiLogicData->simulatedDmg[atk][def][1].median, physical);
    }
}

AI_SINGLE_BATTLE_TEST("AI uses Dynamax -- AI does not dynamax before using a utility move")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_CHECK_BAD_MOVE | AI_FLAG_CHECK_VIABILITY | AI_FLAG_TRY_TO_FAINT | AI_FLAG_OMNISCIENT );
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_FAKE_OUT); DynamaxLevel(10); }
    } WHEN {
        TURN { EXPECT_MOVE(opponent, MOVE_FAKE_OUT, gimmick: GIMMICK_NONE); }
        TURN { EXPECT_MOVE(opponent, MOVE_FAKE_OUT, gimmick: GIMMICK_DYNAMAX); }
    }
}

AI_SINGLE_BATTLE_TEST("AI does not defer Dynamax when Fake Out cannot flinch")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_CHECK_BAD_MOVE | AI_FLAG_CHECK_VIABILITY | AI_FLAG_TRY_TO_FAINT | AI_FLAG_OMNISCIENT);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_INNER_FOCUS); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_FAKE_OUT); DynamaxLevel(10); }
    } WHEN {
        TURN { EXPECT_MOVE(opponent, MOVE_FAKE_OUT, gimmick: GIMMICK_DYNAMAX); }
    }
}

AI_TWO_VS_ONE_BATTLE_TEST("AI only Dynamaxes once per trainer in 2v1 multi battles")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_CHECK_BAD_MOVE | AI_FLAG_CHECK_VIABILITY | AI_FLAG_TRY_TO_FAINT | AI_FLAG_OMNISCIENT);
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_SPLASH); }
        PARTNER(SPECIES_WOBBUFFET) { Moves(MOVE_SPLASH); }
        OPPONENT_A(SPECIES_WOBBUFFET) { Moves(MOVE_SPLASH); DynamaxLevel(10); }
        OPPONENT_A(SPECIES_WOBBUFFET) { Moves(MOVE_SPLASH); DynamaxLevel(10); }
    } WHEN {
        TURN {
            MOVE(playerLeft, MOVE_SPLASH);
            MOVE(playerRight, MOVE_SPLASH);
            EXPECT_MOVE(opponentLeft, MOVE_SPLASH, gimmick: GIMMICK_DYNAMAX);
            EXPECT_MOVE(opponentRight, MOVE_SPLASH, gimmick: GIMMICK_NONE);
        }
        TURN {
            MOVE(playerLeft, MOVE_SPLASH);
            MOVE(playerRight, MOVE_SPLASH);
            EXPECT_MOVE(opponentLeft, MOVE_SPLASH, gimmick: GIMMICK_NONE);
            EXPECT_MOVE(opponentRight, MOVE_SPLASH, gimmick: GIMMICK_NONE);
        }
    }
}

AI_SINGLE_BATTLE_TEST("AI uses the best Max Move")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_CHECK_BAD_MOVE | AI_FLAG_CHECK_VIABILITY | AI_FLAG_TRY_TO_FAINT | AI_FLAG_OMNISCIENT );
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_SCRATCH, MOVE_HEADBUTT); DynamaxLevel(10); }
    } WHEN {
        TURN { EXPECT_MOVE(opponent, MOVE_HEADBUTT, gimmick: GIMMICK_DYNAMAX); }
    }
}

// Copycatting an ally's Max Guard rendition of Trick Room was a notable strategy.
TO_DO_BATTLE_TEST("TODO: AI uses Dynamax -- AI uses Copycat against a Dynamaxed Pokemon intelligently")
