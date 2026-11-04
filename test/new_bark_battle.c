#include "global.h"
#include "battle.h"
#include "battle_setup.h"
#include "overworld.h"
#include "pokemon.h"
#include "task.h"
#include "trainer_see.h"
#include "constants/map_groups.h"
#include "constants/opponents.h"
#include "test/test.h"

TEST("New Bark healing-only rival battles do not enable the tutorial")
{
    u8 flags = RIVAL_BATTLE_HEAL_AFTER;
    bool32 tutorial = FALSE;
    PARAMETRIZE { flags = 0; tutorial = FALSE; }
    PARAMETRIZE { flags = RIVAL_BATTLE_HEAL_AFTER; tutorial = FALSE; }
    PARAMETRIZE { flags = RIVAL_BATTLE_TUTORIAL; tutorial = TRUE; }

    // Exercise the production setup path. The composite tutorial constant
    // includes healing, but a healing-only battle must retain its own roster.
    gMapHeader = *Overworld_GetMapHeaderByGroupAndId(MAP_GROUP(MAP_POLISHED_ELMS_LAB), MAP_NUM(MAP_POLISHED_ELMS_LAB));
    gSaveBlock1Ptr->location.mapGroup = MAP_GROUP(MAP_POLISHED_ELMS_LAB);
    gSaveBlock1Ptr->location.mapNum = MAP_NUM(MAP_POLISHED_ELMS_LAB);
    CreateMon(&gParties[B_TRAINER_PLAYER][0], SPECIES_TOTODILE, 5, 0, OTID_STRUCT_PRESET(0));
    gPartiesCount[B_TRAINER_PLAYER] = 1;
    gNoOfApproachingTrainers = 0;
    InitTrainerBattleParameter();
    // TESTING replaces the production roster with its control trainers.
    // The real-input ROM suite checks all three production Lyra parties.
    TRAINER_BATTLE_PARAM.opponentA = TRAINER_NONE;
    TRAINER_BATTLE_PARAM.earlyRival = TRUE;
    TRAINER_BATTLE_PARAM.rivalBattleFlags = flags;
    BattleSetup_StartTrainerBattle();
    EXPECT(gBattleTypeFlags & BATTLE_TYPE_TRAINER);
    EXPECT_EQ(!!(gBattleTypeFlags & BATTLE_TYPE_FIRST_BATTLE), tutorial);
    ResetTasks();
}
