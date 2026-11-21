#include "global.h"
#include "battle.h"
#include "battle_util.h"
#include "battle_setup.h"
#include "main.h"
#include "overworld.h"
#include "polished_friendship.h"
#include "polished_pokemon_data.h"
#include "polished_trainers.h"
#include "constants/opponents.h"
#include "data/polished_happiness.h"

bool32 UsesPolishedFriendship(struct Pokemon *mon)
{
    if (!B_POLISHED_MOVE_BALANCE
        || PolishedGetSpeciesBaseData(GetMonData(mon, MON_DATA_SPECIES)) == NULL)
        return FALSE;
    if (!gMain.inBattle)
        return TRUE;
    if (gBattleStruct == NULL
        || (gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_FRONTIER
            | BATTLE_TYPE_TRAINER_HILL | BATTLE_TYPE_EREADER_TRAINER))
        || !UsesPolishedBattleRules(B_BATTLER_0, B_BATTLER_1))
        return FALSE;
    // Source happiness events address the player's party. Optional opponent
    // item controllers keep their native behavior.
    for (u32 i = 0; i < PARTY_SIZE; i++)
        if (mon == &gParties[B_TRAINER_PLAYER][i])
            return TRUE;
    return FALSE;
}

u32 PolishedFriendshipAfterEvent(u8 friendship, u32 event, bool32 sootheBell, bool32 luxuryBall)
{
    if (event >= POLISHED_HAPPINESS_COUNT)
        return friendship;
    u32 tier = (friendship >= POLISHED_HAPPINESS_LOW) + (friendship >= POLISHED_HAPPINESS_HIGH);
    s32 delta = sPolishedHappinessChanges[event][tier];
    s32 value = friendship + delta;
    // The source's nonnegative branch also applies extras to a zero base gain.
    // Negative changes never receive item, ball or location bonuses.
    if (delta >= 0)
        value += !!sootheBell + !!luxuryBall;
    return max(0, min(MAX_FRIENDSHIP, value));
}

bool32 PolishedIsBossTrainer(u32 trainer)
{
#if !IS_FRLG
    for (u32 i = 0; i < ARRAY_COUNT(sPolishedBossTrainers); i++)
        if (trainer == sPolishedBossTrainers[i])
            return TRUE;
#endif
    return FALSE;
}

bool32 UsesPolishedBattleItemFriendship(u32 battler, enum Item item)
{
    switch (item)
    {
    case ITEM_X_ATTACK:
    case ITEM_X_DEFENSE:
    case ITEM_X_SP_ATK:
    case ITEM_X_SP_DEF:
    case ITEM_X_SPEED:
    case ITEM_X_ACCURACY:
    case ITEM_GUARD_SPEC:
    case ITEM_DIRE_HIT:
        return UsesPolishedFriendship(GetBattlerMon(battler));
    default:
        return FALSE;
    }
}

bool32 PolishedTryAdjustFriendship(struct Pokemon *mon, u32 event)
{
    u32 sourceEvent;
    if (!UsesPolishedFriendship(mon))
        return FALSE;
    switch (event)
    {
    case FRIENDSHIP_EVENT_GROW_LEVEL:
        // Preserve target save identifiers; source caught-location equality is
        // represented by the existing mapped region sections.
        sourceEvent = GetMonData(mon, MON_DATA_MET_LOCATION) == GetCurrentRegionMapSectionId()
            ? POLISHED_HAPPINESS_GAINLEVELATHOME : POLISHED_HAPPINESS_GAINLEVEL;
        break;
    case FRIENDSHIP_EVENT_VITAMIN: sourceEvent = POLISHED_HAPPINESS_USEDVITAMIN; break;
    case FRIENDSHIP_EVENT_BATTLE_ITEM: sourceEvent = POLISHED_HAPPINESS_USEDXITEM; break;
    case FRIENDSHIP_EVENT_LEARN_TMHM: sourceEvent = POLISHED_HAPPINESS_LEARNMOVE; break;
    case FRIENDSHIP_EVENT_FAINT_SMALL: sourceEvent = POLISHED_HAPPINESS_FAINTED; break;
    case FRIENDSHIP_EVENT_FAINT_FIELD_PSN: sourceEvent = POLISHED_HAPPINESS_POISONFAINT; break;
    case FRIENDSHIP_EVENT_FAINT_LARGE: sourceEvent = POLISHED_HAPPINESS_BEATENBYSTRONGFOE; break;
    case FRIENDSHIP_EVENT_LEAGUE_BATTLE:
        if (!gMain.inBattle || !(gBattleTypeFlags & BATTLE_TYPE_TRAINER))
            return TRUE;
        // Unregistered optional trainers retain native class rules. Every
        // imported trainer is checked against the authenticated source class.
        if (PolishedGetTrainerAttributes(TRAINER_BATTLE_PARAM.opponentA, 0) == NULL)
            return FALSE;
        if (!PolishedIsBossTrainer(TRAINER_BATTLE_PARAM.opponentA) || GetMonData(mon, MON_DATA_HP) == 0)
            return TRUE;
        sourceEvent = POLISHED_HAPPINESS_GYMBATTLE;
        break;
    default:
        // Walking cadence and unrelated field services are separate work.
        return FALSE;
    }
    if (GetMonData(mon, MON_DATA_IS_EGG))
        return TRUE;
    u32 friendship = PolishedFriendshipAfterEvent(GetMonData(mon, MON_DATA_FRIENDSHIP), sourceEvent,
        GetMonData(mon, MON_DATA_HELD_ITEM) == ITEM_SOOTHE_BELL,
        GetMonData(mon, MON_DATA_POKEBALL) == BALL_LUXURY);
    SetMonData(mon, MON_DATA_FRIENDSHIP, &friendship);
    if (gMain.inBattle)
        for (enum BattlerId battler = 0; battler < gBattlersCount; battler++)
            if (gBattlerPositions[battler] != B_POSITION_ABSENT && GetBattlerMon(battler) == mon)
                gBattleMons[battler].friendship = friendship;
    return TRUE;
}
