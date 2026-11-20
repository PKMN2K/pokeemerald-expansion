#include "global.h"
#include "test/battle.h"
#include "test/escape.h"
#include "polished_escape.h"
#include "polished_escape_vectors.h"
#include "wild_encounter.h"
#include "battle_ai_main.h"
#include "roamer.h"

TEST("Polished escape matches all 5184 source word byte carry and attempt vectors")
{
    for (u32 i = 0; i < ARRAY_COUNT(sPolishedEscapeVectors); i++)
    {
        const u16 *v = sPolishedEscapeVectors[i];
        EXPECT_EQ(PolishedEscapeThreshold(v[0], v[1], v[2]), v[3]);
    }
}

WILD_BATTLE_TEST("Polished escape awake roamers flee after the source player action")
{
    bool32 nativeItem;
    PARAMETRIZE { nativeItem = FALSE; }
    PARAMETRIZE { nativeItem = TRUE; }
    GIVEN {
        AUTONOMOUS_ROAMER();
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_KEEN_EYE); Speed(50); }
        OPPONENT(SPECIES_RAIKOU) { Ability(ABILITY_KEEN_EYE); Speed(100); Item(nativeItem ? ITEM_ENIGMA_BERRY_E_READER : ITEM_NONE); Moves(MOVE_SPLASH); }
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE); } }
    SCENE {
        if (B_POLISHED_MOVE_BALANCE && !nativeItem)
            ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, player);
        else
            NONE_OF { ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, player); }
    } THEN { EXPECT_EQ(gBattleOutcome, B_OUTCOME_MON_FLED); }
}

WILD_BATTLE_TEST("Polished escape real roaming opponents stay asleep or frozen instead of fleeing")
{
    u32 status;
    PARAMETRIZE { status = STATUS1_SLEEP; }
    PARAMETRIZE { status = STATUS1_FREEZE; }
    GIVEN {
        AUTONOMOUS_ROAMER();
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_KEEN_EYE); Speed(50); }
        OPPONENT(SPECIES_RAIKOU) { Ability(ABILITY_RUN_AWAY); Speed(100); Status1(status); Moves(MOVE_SPLASH); }
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE); } }
    THEN { EXPECT_EQ(gBattleOutcome == B_OUTCOME_MON_FLED, !B_POLISHED_MOVE_BALANCE); }
}

WILD_BATTLE_TEST("Polished escape corrected training item Speed also controls real action order")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_KEEN_EYE); Speed(100); Item(ITEM_MACHO_BRACE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_KEEN_EYE); Speed(75); }
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE); MOVE(opponent, MOVE_SPLASH); } }
    SCENE {
        if (B_POLISHED_MOVE_BALANCE) {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, player);
            ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, opponent);
        } else {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, opponent);
            ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, player);
        }
    }
}

WILD_BATTLE_TEST("Polished escape uses the opponents effective Choice Scarf Speed")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_KEEN_EYE); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_KEEN_EYE); Speed(100); Item(ITEM_CHOICE_SCARF); Moves(MOVE_SPLASH); }
    } WHEN {
        TURN { RUN(player, WITH_RNG(RNG_POLISHED_ESCAPE, 255)); }
        if (B_POLISHED_MOVE_BALANCE)
            TURN { RUN(player, WITH_RNG(RNG_POLISHED_ESCAPE, 0)); }
    } THEN {
        EXPECT_EQ(gBattleOutcome, B_OUTCOME_RAN);
        EXPECT_EQ(gBattleStruct->runTries, B_POLISHED_MOVE_BALANCE ? 2 : 1);
    }
}

WILD_BATTLE_TEST("Polished escape preserves native item precedence with an active Terastal")
{
    GIVEN {
        PLAYER(SPECIES_RATTATA) { Ability(ABILITY_RUN_AWAY); Item(ITEM_SMOKE_BALL); TeraType(TYPE_NORMAL); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_KEEN_EYE); }
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE, gimmick: GIMMICK_TERA); } }
    THEN {
        EXPECT(!UsesPolishedEscapeRules(B_BATTLER_0));
        EXPECT(TryRunFromBattle(B_BATTLER_0));
        EXPECT_EQ((u32)gProtectStructs[B_BATTLER_0].fleeType, FLEE_ITEM);
    }
}

DOUBLE_BATTLE_TEST("Polished escape keeps optional doubles and their item precedence native")
{
    GIVEN {
        PLAYER(SPECIES_RATTATA) { Ability(ABILITY_RUN_AWAY); Item(ITEM_SMOKE_BALL); }
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_KEEN_EYE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_KEEN_EYE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_KEEN_EYE); }
    } WHEN { TURN { MOVE(playerLeft, MOVE_CELEBRATE); } }
    THEN {
        EXPECT(!UsesPolishedEscapeRules(B_BATTLER_0));
        EXPECT(TryRunFromBattle(B_BATTLER_0));
        EXPECT_EQ((u32)gProtectStructs[B_BATTLER_0].fleeType, FLEE_ITEM);
    }
}

WILD_BATTLE_TEST("Polished escape leaves special battle formats and extra field restrictions native")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_KEEN_EYE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_KEEN_EYE); }
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE); } }
    THEN {
        u32 flags = gBattleTypeFlags;
        EXPECT_EQ(UsesPolishedEscapeRules(B_BATTLER_0), B_POLISHED_MOVE_BALANCE);
        const u32 formats[] = {BATTLE_TYPE_TRAINER, BATTLE_TYPE_LINK, BATTLE_TYPE_FRONTIER,
            BATTLE_TYPE_TRAINER_HILL, BATTLE_TYPE_SAFARI, BATTLE_TYPE_FIRST_BATTLE, BATTLE_TYPE_GHOST};
        for (u32 i = 0; i < ARRAY_COUNT(formats); i++) {
            gBattleTypeFlags = flags | formats[i];
            EXPECT(!UsesPolishedEscapeRules(B_BATTLER_0));
        }
        gBattleTypeFlags = flags;
        player->volatiles.root = TRUE;
        EXPECT(!UsesPolishedEscapeRules(B_BATTLER_0));
        player->volatiles.root = FALSE;
        gFieldStatuses |= STATUS_FIELD_FAIRY_LOCK;
        EXPECT(!UsesPolishedEscapeRules(B_BATTLER_0));
        gFieldStatuses &= ~STATUS_FIELD_FAIRY_LOCK;
        player->item = ITEM_ENIGMA_BERRY_E_READER;
        EXPECT(!UsesPolishedEscapeRules(B_BATTLER_0));
        player->item = ITEM_NONE;
        gBattleTypeFlags |= BATTLE_TYPE_ROAMER;
        opponent->volatiles.root = TRUE;
        player->speed = 1000;
        opponent->speed = 1;
        gBattleStruct->runTries = 0;
        EXPECT(TryRunFromBattle(B_BATTLER_0));
        EXPECT_EQ(gBattleStruct->runTries, 1); // Rooted opponent is not a source flee decision.
    }
}

WILD_BATTLE_TEST("Polished escape attempt byte wraps without losing the guaranteed late attempt")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(B_POLISHED_MOVE_BALANCE ? ABILITY_KEEN_EYE : ABILITY_RUN_AWAY); Speed(1); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_KEEN_EYE); Speed(1000); }
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE); } }
    THEN {
        gBattleStruct->runTries = 255;
        EXPECT(TryRunFromBattle(B_BATTLER_0));
        EXPECT_EQ(gBattleStruct->runTries, B_POLISHED_MOVE_BALANCE ? 0 : 255);
    }
}

WILD_BATTLE_TEST("Polished escape locked roamer actions do not grant a free player escape")
{
    u32 lock;
    PARAMETRIZE { lock = 0; }
    PARAMETRIZE { lock = 1; }
    PARAMETRIZE { lock = 2; }
    PARAMETRIZE { lock = 3; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_KEEN_EYE); Speed(1); }
        OPPONENT(SPECIES_RAIKOU) { Ability(ABILITY_KEEN_EYE); Speed(1000); Moves(MOVE_SPLASH); }
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE); } }
    THEN {
        gBattleTypeFlags |= BATTLE_TYPE_ROAMER;
        if (lock == 0) opponent->volatiles.multipleTurns = TRUE;
        if (lock == 1) opponent->volatiles.rechargeTimer = 1;
        if (lock == 2) opponent->volatiles.rampageTurns = 1;
        if (lock == 3) opponent->volatiles.rolloutTimer = 1;
        EXPECT(!CanPolishedRoamerChooseFlee(B_BATTLER_1));
        u32 failures = 0;
        for (u32 i = 0; i < 32; i++) {
            gBattleStruct->runTries = 0;
            failures += !TryRunFromBattle(B_BATTLER_0);
        }
        EXPECT_GT(failures, 0);
    }
}

WILD_BATTLE_TEST("Polished escape enemy Ghost exemption still respects sleep")
{
    bool32 sleeping;
    PARAMETRIZE { sleeping = FALSE; }
    PARAMETRIZE { sleeping = TRUE; }
    GIVEN {
        WITH_CONFIG(B_GHOSTS_ESCAPE, GEN_3);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_KEEN_EYE); }
        OPPONENT(SPECIES_GASTLY) { Ability(ABILITY_KEEN_EYE); Moves(MOVE_SPLASH); }
    } WHEN { TURN { MOVE(player, MOVE_MEAN_LOOK); } }
    THEN {
        opponent->status1 = sleeping ? 3 : 0;
        gBattleOutcome = 0;
        gCurrentTurnActionNumber = 0;
        gBattlerByTurnOrder[0] = B_BATTLER_1;
        HandleAction_Run();
        EXPECT_EQ(gBattleOutcome == B_OUTCOME_MON_FLED, B_POLISHED_MOVE_BALANCE && !sleeping);
    }
}

WILD_BATTLE_TEST("Polished escape waking a sleeping roamer does not create a same turn flee action")
{
    GIVEN {
        AUTONOMOUS_ROAMER();
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_KEEN_EYE); Speed(50); }
        OPPONENT(SPECIES_RAIKOU) { Ability(ABILITY_KEEN_EYE); Speed(100); Status1(STATUS1_SLEEP); Moves(MOVE_SPLASH); }
    } WHEN { TURN { MOVE(player, MOVE_WORRY_SEED); } }
    THEN {
        EXPECT_EQ(gBattleOutcome == B_OUTCOME_MON_FLED, !B_POLISHED_MOVE_BALANCE);
        if (B_POLISHED_MOVE_BALANCE) {
            EXPECT_EQ(opponent->status1, 0);
            EXPECT_EQ(opponent->ability, ABILITY_INSOMNIA);
        }
    }
}

WILD_BATTLE_TEST("Polished escape resolves inclusive random boundaries and repeated attempts through Run")
{
    u32 roll;
    PARAMETRIZE { roll = 39; }
    PARAMETRIZE { roll = 40; }
    PARAMETRIZE { roll = 255; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_KEEN_EYE); Speed(B_POLISHED_MOVE_BALANCE ? 31 : 200); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_KEEN_EYE); Speed(100); Moves(MOVE_SPLASH); }
    } WHEN {
        TURN { RUN(player, WITH_RNG(RNG_POLISHED_ESCAPE, roll)); }
        if (B_POLISHED_MOVE_BALANCE && roll >= 40)
            TURN { RUN(player, WITH_RNG(RNG_POLISHED_ESCAPE, 69)); }
    } THEN {
        EXPECT_EQ(gBattleOutcome, B_OUTCOME_RAN);
        EXPECT_EQ(gBattleStruct->runTries, B_POLISHED_MOVE_BALANCE && roll >= 40 ? 2 : 1);
    }
}

WILD_BATTLE_TEST("Polished escape Fight resets the run bonus but switching preserves it")
{
    bool32 fight;
    PARAMETRIZE { fight = FALSE; }
    PARAMETRIZE { fight = TRUE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_KEEN_EYE); Speed(B_POLISHED_MOVE_BALANCE ? 31 : 200); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_KEEN_EYE); Speed(31); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_KEEN_EYE); Speed(100); Moves(MOVE_SPLASH); }
    } WHEN {
        TURN { RUN(player, WITH_RNG(RNG_POLISHED_ESCAPE, 40)); }
        if (B_POLISHED_MOVE_BALANCE)
        {
            TURN { if (fight) MOVE(player, MOVE_CELEBRATE); else SWITCH(player, 1); }
            TURN { RUN(player, WITH_RNG(RNG_POLISHED_ESCAPE, 40)); }
            if (fight)
                TURN { RUN(player, WITH_RNG(RNG_POLISHED_ESCAPE, 69)); }
        }
    } THEN {
        EXPECT_EQ(gBattleOutcome, B_OUTCOME_RAN);
        EXPECT_EQ(gBattleStruct->runTries, B_POLISHED_MOVE_BALANCE ? 2 : 1);
    }
}

WILD_BATTLE_TEST("Polished escape blocked Run returns to action selection without consuming an attempt")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_KEEN_EYE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_KEEN_EYE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_MEAN_LOOK); }
        TURN { RUN(player, allowed: FALSE); MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_MEAN_LOOK, opponent);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, player);
    }
    THEN {
        EXPECT_EQ(gBattleStruct->runTries, 0);
        EXPECT(player->volatiles.escapePrevention);
    }
}

WILD_BATTLE_TEST("Polished escape gives the player an action before a faster roamer flees")
{
    GIVEN {
        AUTONOMOUS_ROAMER();
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_KEEN_EYE); Speed(50); }
        OPPONENT(SPECIES_RAIKOU) { Ability(ABILITY_KEEN_EYE); Speed(100); Moves(MOVE_SPLASH); }
    } WHEN { TURN { MOVE(player, MOVE_MEAN_LOOK); } }
    SCENE {
        if (B_POLISHED_MOVE_BALANCE)
            ANIMATION(ANIM_TYPE_MOVE, MOVE_MEAN_LOOK, player);
        else
            NONE_OF { ANIMATION(ANIM_TYPE_MOVE, MOVE_MEAN_LOOK, player); }
    } THEN {
        EXPECT_EQ((u32)opponent->volatiles.escapePrevention, B_POLISHED_MOVE_BALANCE);
        EXPECT_EQ(gBattleOutcome == B_OUTCOME_MON_FLED, !B_POLISHED_MOVE_BALANCE);
    }
}

WILD_BATTLE_TEST("Polished escape allows fleeing from an untrapped awake roamer without a speed roll")
{
    GIVEN {
        AUTONOMOUS_ROAMER();
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_KEEN_EYE); Speed(B_POLISHED_MOVE_BALANCE ? 1 : 300); }
        OPPONENT(SPECIES_RAIKOU) { Ability(ABILITY_KEEN_EYE); Speed(200); Moves(MOVE_SPLASH); }
    } WHEN { TURN { RUN(player, WITH_RNG(RNG_POLISHED_ESCAPE, 255)); } }
    THEN {
        EXPECT_EQ(gBattleOutcome, B_OUTCOME_RAN);
        EXPECT_EQ(gBattleStruct->runTries, B_POLISHED_MOVE_BALANCE ? 0 : 1);
    }
}

WILD_BATTLE_TEST("Polished escape move traps take precedence over trapping ability messages")
{
    enum Move move;
    PARAMETRIZE { move = MOVE_MEAN_LOOK; }
    PARAMETRIZE { move = MOVE_WRAP; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_KEEN_EYE); HP(1000); MaxHP(1000); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_SHADOW_TAG); }
    } WHEN { TURN { MOVE(opponent, move, hit: TRUE); } }
    THEN {
        EXPECT_EQ(IsRunningFromBattleImpossible(B_BATTLER_0), B_POLISHED_MOVE_BALANCE ? BATTLE_RUN_FORBIDDEN : BATTLE_RUN_FAILURE);
        EXPECT_EQ(gBattleStruct->runTries, 0);
    }
}

WILD_BATTLE_TEST("Polished escape suppressed Run Away falls back to Smoke Ball")
{
    GIVEN {
        PLAYER(SPECIES_RATTATA) { Ability(ABILITY_RUN_AWAY); Item(ITEM_SMOKE_BALL); }
        OPPONENT(SPECIES_WEEZING) { Ability(ABILITY_NEUTRALIZING_GAS); }
    } WHEN { TURN { MOVE(opponent, MOVE_MEAN_LOOK); } }
    THEN {
        EXPECT(TryRunFromBattle(B_BATTLER_0));
        EXPECT_EQ((u32)gProtectStructs[B_BATTLER_0].fleeType, FLEE_ITEM);
        EXPECT_EQ(gBattleStruct->runTries, 0);
    }
}

WILD_BATTLE_TEST("Polished escape roamer AI and action execution both reject sleep and freeze")
{
    u32 status;
    PARAMETRIZE { status = 0; }
    PARAMETRIZE { status = 1; }
    PARAMETRIZE { status = STATUS1_SLEEP; }
    PARAMETRIZE { status = STATUS1_FREEZE; }
    PARAMETRIZE { status = STATUS1_BURN; }
    PARAMETRIZE { status = STATUS1_POISON; }
    PARAMETRIZE { status = STATUS1_PARALYSIS; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_KEEN_EYE); }
        OPPONENT(SPECIES_RAIKOU) { Ability(ABILITY_RUN_AWAY); Moves(MOVE_SPLASH); }
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE); } }
    THEN {
        bool32 canFlee = !B_POLISHED_MOVE_BALANCE || !(status & (STATUS1_SLEEP | STATUS1_FREEZE));
        gBattleTypeFlags |= BATTLE_TYPE_ROAMER;
        opponent->status1 = status;
        gAiThinkingStruct->aiFlags[B_BATTLER_1] = AI_FLAG_ROAMING;
        gAiBattleData->actionFlee = FALSE;
        ComputeAiBattlerDecisions(B_BATTLER_1);
        EXPECT_EQ((u32)gAiBattleData->actionFlee, canFlee);
        gBattleOutcome = 0;
        gCurrentTurnActionNumber = 0;
        gBattlerByTurnOrder[0] = B_BATTLER_1;
        HandleAction_Run();
        EXPECT_EQ(gBattleOutcome == B_OUTCOME_MON_FLED, canFlee);
    }
}

WILD_BATTLE_TEST("Polished escape enemy Smoke Ball cannot bypass source move traps")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_KEEN_EYE); }
        OPPONENT(SPECIES_RAIKOU) { Ability(ABILITY_KEEN_EYE); Item(ITEM_SMOKE_BALL); Moves(MOVE_SPLASH); }
    } WHEN { TURN { MOVE(player, MOVE_MEAN_LOOK); } }
    THEN {
        gBattleTypeFlags |= BATTLE_TYPE_ROAMER;
        gAiThinkingStruct->aiFlags[B_BATTLER_1] = AI_FLAG_ROAMING;
        gAiBattleData->actionFlee = FALSE;
        ComputeAiBattlerDecisions(B_BATTLER_1);
        EXPECT_EQ((u32)gAiBattleData->actionFlee, !B_POLISHED_MOVE_BALANCE);
        gBattleOutcome = 0;
        gCurrentTurnActionNumber = 0;
        gBattlerByTurnOrder[0] = B_BATTLER_1;
        HandleAction_Run();
        EXPECT_EQ(gBattleOutcome == B_OUTCOME_MON_FLED, !B_POLISHED_MOVE_BALANCE);
    }
}

WILD_BATTLE_TEST("Polished escape rechecks newly applied traps when an enemy flee action executes")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_KEEN_EYE); }
        OPPONENT(SPECIES_RAIKOU) { Ability(ABILITY_KEEN_EYE); Moves(MOVE_SPLASH); }
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE); } }
    THEN {
        gBattleTypeFlags |= BATTLE_TYPE_ROAMER;
        gAiThinkingStruct->aiFlags[B_BATTLER_1] = AI_FLAG_ROAMING;
        gAiBattleData->actionFlee = FALSE;
        ComputeAiBattlerDecisions(B_BATTLER_1);
        EXPECT(gAiBattleData->actionFlee);
        opponent->volatiles.escapePrevention = TRUE;
        gBattleOutcome = 0;
        gCurrentTurnActionNumber = 0;
        gBattlerByTurnOrder[0] = B_BATTLER_1;
        HandleAction_Run();
        EXPECT_NE(gBattleOutcome, B_OUTCOME_MON_FLED);
    }
}

WILD_BATTLE_TEST("Polished escape roaming persistence converts toxic poison without shrinking HP")
{
    u32 status;
    PARAMETRIZE { status = STATUS1_TOXIC_POISON | 0x500; }
    PARAMETRIZE { status = STATUS1_POISON; }
    PARAMETRIZE { status = STATUS1_BURN; }
    PARAMETRIZE { status = STATUS1_FREEZE; }
    PARAMETRIZE { status = STATUS1_PARALYSIS; }
    PARAMETRIZE { status = 3; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_KEEN_EYE); }
        OPPONENT(SPECIES_RAIKOU) { Ability(ABILITY_KEEN_EYE); HP(600); MaxHP(600); }
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE); } }
    THEN {
        SetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_STATUS, &status);
        gEncounteredRoamerIndex = 0;
        UpdateRoamerHPStatus(&gParties[B_TRAINER_OPPONENT_A][0]);
        u32 saved = gSaveBlock1Ptr->roamer[0].statusA | gSaveBlock1Ptr->roamer[0].statusB << 8;
        EXPECT_EQ(saved, B_POLISHED_MOVE_BALANCE && (status & STATUS1_TOXIC_POISON) ? STATUS1_POISON : status);
        EXPECT_EQ(gSaveBlock1Ptr->roamer[0].hp, 600);
    }
}

WILD_BATTLE_TEST("Polished escape prioritizes Run Away over an escape item")
{
    GIVEN {
        PLAYER(SPECIES_RATTATA) { Ability(ABILITY_RUN_AWAY); Item(ITEM_SMOKE_BALL); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_KEEN_EYE); }
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE); } }
    THEN {
        EXPECT(TryRunFromBattle(B_BATTLER_0));
        EXPECT_EQ((u32)gProtectStructs[B_BATTLER_0].fleeType, B_POLISHED_MOVE_BALANCE ? FLEE_ABILITY : FLEE_ITEM);
        EXPECT_EQ(gBattleStruct->runTries, 0);
    }
}

WILD_BATTLE_TEST("Polished escape uses effective paralysis Speed rather than raw Speed")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_KEEN_EYE); Speed(100); Status1(STATUS1_PARALYSIS); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_KEEN_EYE); Speed(80); }
    } WHEN { TURN { MOVE(opponent, MOVE_CELEBRATE); } }
    THEN {
        u32 failures = 0;
        for (u32 i = 0; i < 32; i++)
        {
            gBattleStruct->runTries = 0;
            failures += !TryRunFromBattle(B_BATTLER_0);
        }
        EXPECT_EQ(failures != 0, B_POLISHED_MOVE_BALANCE);
        EXPECT_EQ(gBattleStruct->runTries, 1);
    }
}

WILD_BATTLE_TEST("Polished escape Ghost exemption ignores the native generation preset")
{
    GIVEN {
        WITH_CONFIG(B_GHOSTS_ESCAPE, GEN_3);
        PLAYER(SPECIES_GASTLY) { Ability(ABILITY_KEEN_EYE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_KEEN_EYE); }
    } WHEN { TURN { MOVE(opponent, MOVE_MEAN_LOOK); } }
    THEN {
        EXPECT_EQ(IsRunningFromBattleImpossible(B_BATTLER_0), B_POLISHED_MOVE_BALANCE ? BATTLE_RUN_SUCCESS : BATTLE_RUN_FORBIDDEN);
    }
}

WILD_BATTLE_TEST("Polished escape source Speed has no training item penalty")
{
    enum Item item;
    PARAMETRIZE { item = ITEM_MACHO_BRACE; }
    PARAMETRIZE { item = ITEM_POWER_WEIGHT; }
    PARAMETRIZE { item = ITEM_POWER_BRACER; }
    PARAMETRIZE { item = ITEM_POWER_BELT; }
    PARAMETRIZE { item = ITEM_POWER_LENS; }
    PARAMETRIZE { item = ITEM_POWER_BAND; }
    PARAMETRIZE { item = ITEM_POWER_ANKLET; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_KEEN_EYE); Speed(100); Item(item); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_KEEN_EYE); Speed(50); }
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE); } }
    THEN {
        EXPECT_EQ(GetBattlerTotalSpeedStat(B_BATTLER_0, ABILITY_KEEN_EYE, GetItemHoldEffect(item)), B_POLISHED_MOVE_BALANCE ? 100 : 50);
    }
}
