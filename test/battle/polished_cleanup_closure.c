#include "global.h"
#include "test/battle.h"
#include "battle_script_commands.h"
#include "battle_util.h"
#include "fieldmap.h"
#include "overworld.h"
#include "pokemon_storage_system.h"
#include "constants/battle_string_ids.h"
#include "constants/heal_locations.h"
#include "constants/maps.h"

static const enum Item sCleanupBerries[] = {
    ITEM_ORAN_BERRY, ITEM_PECHA_BERRY, ITEM_RAWST_BERRY,
    ITEM_CHESTO_BERRY, ITEM_ASPEAR_BERRY, ITEM_CHERI_BERRY,
};

// ExitBattle calls PostBattleTasks before the loss/whiteout branch. The live
// battle runner reaches HandleEndTurn_FinishBattle's restoration before THEN;
// these tests deliberately never call TryRestoreHeldItems themselves.
SINGLE_BATTLE_TEST("Polished cleanup actual loss restores every removed party berry before blackout")
{
    GIVEN {
        for (u32 i = 0; i < ARRAY_COUNT(sCleanupBerries); i++)
            PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(sCleanupBerries[i]); Speed(50); Attack(1); HP(1000); MaxHP(1000); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(100); Defense(999); HP(5000); MaxHP(5000); }
    } WHEN {
        for (u32 i = 0; i < ARRAY_COUNT(sCleanupBerries); i++) {
            TURN {
                MOVE(opponent, MOVE_KNOCK_OFF);
                MOVE(player, MOVE_EXPLOSION);
                if (i + 1 < ARRAY_COUNT(sCleanupBerries)) SEND_OUT(player, i + 1);
            }
        }
    } SCENE {
        for (u32 i = 0; i < ARRAY_COUNT(sCleanupBerries); i++) {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_KNOCK_OFF, opponent);
            ANIMATION(ANIM_TYPE_MOVE, MOVE_EXPLOSION, player);
            MESSAGE("Wobbuffet fainted!");
        }
    } THEN {
        EXPECT_EQ(gBattleOutcome, B_OUTCOME_LOST);
        for (u32 i = 0; i < ARRAY_COUNT(sCleanupBerries); i++) {
            EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_HP), 0);
            EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_HELD_ITEM),
                      B_POLISHED_MOVE_BALANCE ? sCleanupBerries[i] : ITEM_NONE);
        }

        // Exercise the shipping whiteout healing/warp boundary on the party
        // produced by the actual loss, without synthesizing another backup.
        // The battle harness does not run the field callback's visual delay.
        struct MapHeader savedMap = gMapHeader;
        SetLastHealLocationWarp(HEAL_LOCATION_POLISHED_NEW_BARK_TOWN);
        DoWhiteOut();
        EXPECT_EQ(gSaveBlock1Ptr->location.mapGroup, MAP_GROUP(MAP_POLISHED_NEW_BARK_TOWN));
        EXPECT_EQ(gSaveBlock1Ptr->location.mapNum, MAP_NUM(MAP_POLISHED_NEW_BARK_TOWN));
        for (u32 i = 0; i < ARRAY_COUNT(sCleanupBerries); i++) {
            struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];
            EXPECT_EQ(GetMonData(mon, MON_DATA_HP), GetMonData(mon, MON_DATA_MAX_HP));
            EXPECT_EQ(GetMonData(mon, MON_DATA_HELD_ITEM), B_POLISHED_MOVE_BALANCE ? sCleanupBerries[i] : ITEM_NONE);
        }
        gMapHeader = savedMap;
    }
}

WILD_BATTLE_TEST("Polished cleanup actual wild loss distinguishes removed berries eaten berries and nonberries")
{
    u32 kind = 0;
    for (u32 k = 0; k < 3; k++) PARAMETRIZE { kind = k; }
    enum Item item = kind == 1 ? ITEM_WHITE_HERB : ITEM_ORAN_BERRY;
    enum Move foeMove = kind == 0 ? MOVE_KNOCK_OFF : kind == 1 ? MOVE_TAIL_WHIP : MOVE_SPLASH;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(item); Speed(50); Attack(1); HP(kind == 2 ? 499 : 1000); MaxHP(1000); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(100); Defense(999); HP(5000); MaxHP(5000); Moves(foeMove); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); MOVE(opponent, foeMove); }
        TURN { MOVE(player, MOVE_EXPLOSION); MOVE(opponent, foeMove); }
    } SCENE {
        if (kind == 0) ANIMATION(ANIM_TYPE_MOVE, MOVE_KNOCK_OFF, opponent);
        else ANIMATION(ANIM_TYPE_GENERAL, kind == 1 ? B_ANIM_HELD_ITEM_EFFECT : B_ANIM_HELD_ITEM_BERRY, player);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_EXPLOSION, player);
        MESSAGE("Wobbuffet fainted!");
    } THEN {
        // Native wild Knock Off cannot remove the player's item. Source
        // removes it during battle and restores its backup on the loss.
        // Native wild consumption does not restore nonberries; source
        // PostBattleTasks restores the White Herb backup even after a loss.
        enum Item expected = kind == 1 && B_POLISHED_MOVE_BALANCE ? ITEM_WHITE_HERB
            : kind == 0 ? ITEM_ORAN_BERRY : ITEM_NONE;
        EXPECT_EQ(gBattleOutcome, B_OUTCOME_LOST);
        EXPECT_EQ(player->item, kind == 0 && !B_POLISHED_MOVE_BALANCE ? ITEM_ORAN_BERRY : ITEM_NONE);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_HP), 0);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_HELD_ITEM), expected);
    }
}

// PokeBallEffect.SendToPC copies the current enemy item and leaves all six
// existing party backups untouched. The native control has one free slot so
// its optional full-party UI is covered separately at the command boundary.
WILD_BATTLE_TEST("Polished cleanup full party capture preserves current boxed items and existing backups")
{
    u32 kind = 0;
    for (u32 k = 0; k < 3; k++) PARAMETRIZE { kind = k; }
    u32 members = B_POLISHED_MOVE_BALANCE ? PARTY_SIZE : PARTY_SIZE - 1;
    enum Item item = kind == 0 ? ITEM_POTION : kind == 1 ? ITEM_SMOKE_BALL : ITEM_ORAN_BERRY;
    GIVEN {
        ResetPokemonStorageSystem();
        for (u32 i = 0; i < members; i++)
            PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(sCleanupBerries[i]); Speed(50); Attack(1); HP(1000); MaxHP(1000); }
        OPPONENT(SPECIES_CLEFFA) { Ability(ABILITY_NONE); Item(item); Speed(100); Defense(999); HP(1000); MaxHP(1000); Moves(MOVE_KNOCK_OFF); }
    } WHEN {
        TURN { MOVE(player, kind == 1 ? MOVE_KNOCK_OFF : kind == 2 ? MOVE_SUPER_FANG : MOVE_SPLASH, hit: TRUE); MOVE(opponent, MOVE_KNOCK_OFF); }
        TURN { USE_ITEM(player, ITEM_MASTER_BALL); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_KNOCK_OFF, opponent);
        if (kind == 1) ANIMATION(ANIM_TYPE_MOVE, MOVE_KNOCK_OFF, player);
        if (kind == 2) {
            // Cross the threshold with an actual action, after the foe's
            // Knock Off, rather than starting below it before turn one.
            ANIMATION(ANIM_TYPE_MOVE, MOVE_SUPER_FANG, player);
            HP_BAR(opponent, damage: 500);
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, opponent);
        }
        // The catch string embeds audio controls not normalized by MESSAGE.
        // Actual capture, party identity and storage are asserted below.
    } THEN {
        enum Item expected = kind == 0 ? ITEM_POTION
            : kind == 1 && !B_POLISHED_MOVE_BALANCE ? ITEM_SMOKE_BALL : ITEM_NONE;
        EXPECT_EQ(gBattleOutcome, B_OUTCOME_CAUGHT);
        EXPECT_EQ(CalculatePlayerPartyCount(), PARTY_SIZE);
        for (u32 i = 0; i < members; i++) {
            EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_SPECIES), SPECIES_WOBBUFFET);
            // Source restores the removed lead berry; native wild Knock Off
            // leaves it held. Every original party slot must keep its item.
            EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_HELD_ITEM), sCleanupBerries[i]);
        }
        if (B_POLISHED_MOVE_BALANCE) {
            EXPECT_EQ(GetBoxMonDataAt(0, 0, MON_DATA_SPECIES), SPECIES_CLEFFA);
            EXPECT_EQ(GetBoxMonDataAt(0, 0, MON_DATA_HELD_ITEM), expected);
            EXPECT_EQ(CountMonsInBox(0), 1);
        } else {
            EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][members], MON_DATA_SPECIES), SPECIES_CLEFFA);
            EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][members], MON_DATA_HELD_ITEM), expected);
            EXPECT_EQ(CountMonsInBox(0), 0);
        }
    }
}

WILD_BATTLE_TEST("Polished cleanup full party capture keeps the optional native swap prompt")
{
    GIVEN {
        for (u32 i = 0; i < PARTY_SIZE; i++)
            PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_CLEFFA) { Ability(ABILITY_NONE); Moves(MOVE_SPLASH); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); MOVE(opponent, MOVE_SPLASH); }
    } THEN {
        // Invoke only the real delivery command's first state. This observes
        // the native prompt request without trying to automate its input UI.
        static const u8 command[] = {B_SCR_OP_GIVECAUGHTMON, 0, 0, 0, 0};
        const u8 *savedScript = gBattlescriptCurrInstr;
        enum Item savedBall = gLastThrownBall;
        enum BattlerId savedAttacker = gBattlerAttacker;
        u32 savedControllerFlags = gBattleControllerExecFlags;
        u8 savedCommunication[BATTLE_COMMUNICATION_ENTRIES_COUNT];
        memcpy(savedCommunication, gBattleCommunication, sizeof(savedCommunication));
        gBattlescriptCurrInstr = command;
        gLastThrownBall = ITEM_MASTER_BALL;
        gBattlerAttacker = B_BATTLER_0;
        gBattleControllerExecFlags = 0;
        gBattleCommunication[MULTIUSE_STATE] = 0;
        gBattleScriptingCommandsTable[B_SCR_OP_GIVECAUGHTMON]();
        bool32 requested = gBattleControllerExecFlags != 0;
        u32 stringId = gBattleResources->bufferA[B_BATTLER_0][2]
            | (gBattleResources->bufferA[B_BATTLER_0][3] << 8);
        gBattlescriptCurrInstr = savedScript;
        gLastThrownBall = savedBall;
        gBattlerAttacker = savedAttacker;
        gBattleControllerExecFlags = savedControllerFlags;
        memcpy(gBattleCommunication, savedCommunication, sizeof(savedCommunication));
        EXPECT_EQ(requested, !B_POLISHED_MOVE_BALANCE && B_CATCH_SWAP_INTO_PARTY >= GEN_7);
        if (requested) EXPECT_EQ(stringId, STRINGID_SENDCAUGHTMONPARTYORBOX);
    }
}
