#include "global.h"
#include "constants/maps.h"
#include "event_data.h"
#include "field_move.h"
#include "fldeff.h"
#include "fldeff_misc.h"
#include "party_menu.h"
#include "strings.h"
#include "constants/field_move.h"
#include "constants/moves.h"
#include "constants/party_menu.h"

static bool32 IsAlwaysFalse(enum FieldMove fieldMove)
{
    return FALSE;
}

static bool32 IsAlwaysTrue(enum FieldMove fieldMove)
{
    return TRUE;
}

static bool32 HasBadgeForFieldMove(enum FieldMove fieldMove)
{
#if POLISHED_JOHTO_DEMO
    // Preserve the demo's Surf access without claiming a fifth campaign badge.
    if (fieldMove == FIELD_MOVE_SURF && gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(MAP_POLISHED_JOHTO_DEMO_OUTDOOR))
        return TRUE;
#endif
    return FlagGet(gFieldMoveInfo[fieldMove].arg + FLAG_BADGE01_GET);
}

const struct FieldMoveUnlock gFieldMoveUnlocks[FIELD_MOVE_UNLOCK_COUNT] =
{
    [CANT_UNLOCK] =
    {
        .isUnlockedFunc = IsAlwaysFalse,
        .lockedMessage = gText_EmptyString2,
    },
    [ALWAYS_UNLOCKED] =
    {
        .isUnlockedFunc = IsAlwaysTrue,
        .lockedMessage = gText_EmptyString2,
    },
    [BADGE_UNLOCK] =
    {
        .isUnlockedFunc = HasBadgeForFieldMove,
        .lockedMessage = gText_CantUseUntilNewBadge,
    },
};

#define FLAG_TO_BADGE(flag) flag - FLAG_BADGE01_GET

const struct FieldMoveInfo gFieldMoveInfo[FIELD_MOVES_COUNT] =
{
    [FIELD_MOVE_CUT] =
    {
        .fieldMoveFunc = SetUpFieldMove_Cut,
        .unlockType = BADGE_UNLOCK,
        .moveID = MOVE_CUT,
        .partyMsgID = PARTY_MSG_NOTHING_TO_CUT,
        .arg = IS_FRLG ? FLAG_TO_BADGE(FLAG_BADGE02_GET) : FLAG_TO_BADGE(FLAG_BADGE01_GET),
    },

    [FIELD_MOVE_FLASH] =
    {
        .fieldMoveFunc = SetUpFieldMove_Flash,
        .unlockType = BADGE_UNLOCK,
        .moveID = MOVE_FLASH,
        .partyMsgID = PARTY_MSG_CANT_USE_HERE,
        .arg = IS_FRLG ? FLAG_TO_BADGE(FLAG_BADGE01_GET) : FLAG_TO_BADGE(FLAG_BADGE02_GET),
    },

    [FIELD_MOVE_ROCK_SMASH] =
    {
        .fieldMoveFunc = SetUpFieldMove_RockSmash,
        .unlockType = BADGE_UNLOCK,
        .moveID = MOVE_ROCK_SMASH,
        .partyMsgID = PARTY_MSG_CANT_USE_HERE,
        .arg = IS_FRLG ? FLAG_TO_BADGE(FLAG_BADGE06_GET) : FLAG_TO_BADGE(FLAG_BADGE03_GET),
    },

    [FIELD_MOVE_STRENGTH] =
    {
        .fieldMoveFunc = SetUpFieldMove_Strength,
        .unlockType = BADGE_UNLOCK,
        .moveID = MOVE_STRENGTH,
        .partyMsgID = PARTY_MSG_CANT_USE_HERE,
        .arg = FLAG_TO_BADGE(FLAG_BADGE04_GET),
    },

    [FIELD_MOVE_SURF] =
    {
        .fieldMoveFunc = SetUpFieldMove_Surf,
        .unlockType = BADGE_UNLOCK,
        .moveID = MOVE_SURF,
        .partyMsgID = PARTY_MSG_CANT_SURF_HERE,
        .arg = FLAG_TO_BADGE(FLAG_BADGE05_GET),
    },

    [FIELD_MOVE_FLY] =
    {
        .fieldMoveFunc = SetUpFieldMove_Fly,
        .unlockType = BADGE_UNLOCK,
        .moveID = MOVE_FLY,
        .partyMsgID = PARTY_MSG_CANT_USE_HERE,
        .arg = IS_FRLG ? FLAG_TO_BADGE(FLAG_BADGE03_GET) : FLAG_TO_BADGE(FLAG_BADGE06_GET),
    },

    [FIELD_MOVE_DIVE] =
    {
        .fieldMoveFunc = SetUpFieldMove_Dive,
        .unlockType = BADGE_UNLOCK,
        .moveID = MOVE_DIVE,
        .partyMsgID = PARTY_MSG_CANT_USE_HERE,
        .arg = FLAG_TO_BADGE(FLAG_BADGE07_GET),
    },

    [FIELD_MOVE_WATERFALL] =
    {
        .fieldMoveFunc = SetUpFieldMove_Waterfall,
        .unlockType = BADGE_UNLOCK,
        .moveID = MOVE_WATERFALL,
        .partyMsgID = PARTY_MSG_CANT_USE_HERE,
        .arg = IS_FRLG ? FLAG_TO_BADGE(FLAG_BADGE07_GET) : FLAG_TO_BADGE(FLAG_BADGE08_GET),
    },

    [FIELD_MOVE_TELEPORT] =
    {
        .fieldMoveFunc = SetUpFieldMove_Teleport,
        .unlockType = ALWAYS_UNLOCKED,
        .moveID = MOVE_TELEPORT,
        .partyMsgID = PARTY_MSG_CANT_USE_HERE,
    },

    [FIELD_MOVE_DIG] =
    {
        .fieldMoveFunc = SetUpFieldMove_Dig,
        .unlockType = ALWAYS_UNLOCKED,
        .moveID = MOVE_DIG,
        .partyMsgID = PARTY_MSG_CANT_USE_HERE,
    },

    [FIELD_MOVE_SECRET_POWER] =
    {
        .fieldMoveFunc = SetUpFieldMove_SecretPower,
        .unlockType = ALWAYS_UNLOCKED,
        .moveID = MOVE_SECRET_POWER,
        .partyMsgID = PARTY_MSG_CANT_USE_HERE,
    },

    [FIELD_MOVE_MILK_DRINK] =
    {
        .fieldMoveFunc = SetUpFieldMove_SoftBoiled,
        .unlockType = ALWAYS_UNLOCKED,
        .moveID = MOVE_MILK_DRINK,
        .partyMsgID = PARTY_MSG_NOT_ENOUGH_HP,
    },

    [FIELD_MOVE_SOFT_BOILED] =
    {
        .fieldMoveFunc = SetUpFieldMove_SoftBoiled,
        .unlockType = ALWAYS_UNLOCKED,
        .moveID = MOVE_SOFT_BOILED,
        .partyMsgID = PARTY_MSG_NOT_ENOUGH_HP,
    },

    [FIELD_MOVE_SWEET_SCENT] =
    {
        .fieldMoveFunc = SetUpFieldMove_SweetScent,
        .unlockType = ALWAYS_UNLOCKED,
        .moveID = MOVE_SWEET_SCENT,
        .partyMsgID = PARTY_MSG_CANT_USE_HERE,
    },
    [FIELD_MOVE_ROCK_CLIMB] =
    {
        .fieldMoveFunc = SetUpFieldMove_RockClimb,
#if OW_ROCK_CLIMB_FIELD_MOVE
        .unlockType = ALWAYS_UNLOCKED,
#else
        .unlockType = CANT_UNLOCK,
#endif
        .moveID = MOVE_ROCK_CLIMB,
        .partyMsgID = PARTY_MSG_CANT_USE_HERE,
        .hideIfLocked = TRUE,
    },
    [FIELD_MOVE_DEFOG] =
    {
        .fieldMoveFunc = SetUpFieldMove_Defog,
#if OW_DEFOG_FIELD_MOVE
        .unlockType = ALWAYS_UNLOCKED,
#else
        .unlockType = CANT_UNLOCK,
#endif
        .moveID = MOVE_DEFOG,
        .partyMsgID = PARTY_MSG_CANT_USE_HERE,
        .hideIfLocked = TRUE,
    },
};

// Pinned Polished Crystal: engine/events/treemons.asm GetTreeScore.
// Coordinates are the collision tile in front of the player, not the
// player's own coordinates. Source computes ((x * (y + 1) + y) / 5) % 10,
// compares it with trainer ID % 10, and divides the difference into
// rare (0), good (1-4) and bad (5-9) trees.
// Dormant until Johto Headbutt field-move and map wiring is implemented.
enum JohtoHeadbuttTreeScore GetJohtoHeadbuttTreeScore(u8 x, u8 y, u16 trainerId)
{
    u16 coordinateNumerator = (u16)x * (y + 1) + y;
    u8 coordinateScore = (coordinateNumerator / 5) % 10;
    u8 trainerScore = trainerId % 10;
    u8 delta = (coordinateScore + 10 - trainerScore) % 10;

    if (delta == 0)
        return JOHTO_HEADBUTT_RARE_TREE;
    if (delta < 5)
        return JOHTO_HEADBUTT_GOOD_TREE;
    return JOHTO_HEADBUTT_BAD_TREE;
}

// GetTreeMon uses RandomRange(10): bad 1/10, good 5/10, rare 8/10.
// The rare score selects the second distribution; ordinary scores do not.
// This helper does not start battles or consume game RNG by itself.
bool8 JohtoHeadbuttRollStartsBattle(enum JohtoHeadbuttTreeScore score, u8 rollOutOfTen)
{
    if (rollOutOfTen >= 10)
        return FALSE;
    if (score == JOHTO_HEADBUTT_RARE_TREE)
        return rollOutOfTen < 8;
    if (score == JOHTO_HEADBUTT_GOOD_TREE)
        return rollOutOfTen < 5;
    if (score == JOHTO_HEADBUTT_BAD_TREE)
        return rollOutOfTen < 1;
    return FALSE;
}

// Only rare-score trees select Polished Crystal's second tree encounter
// distribution. Normal and good scores use the first distribution.
// Dormant: no species selection or wild battle is performed here.
bool8 JohtoHeadbuttUsesRareEncounterTable(enum JohtoHeadbuttTreeScore score)
{
    return score == JOHTO_HEADBUTT_RARE_TREE;
}

// Pinned SelectTreeMon: RandomRange(100), subtract each row's weight
// until the roll is below its row weight. Returns an index only, not a
// species, form, level, or battle. Reject invalid/incomplete tables.
s16 JohtoHeadbuttSelectWeightedEntry(const u8 *weights, u8 count, u8 rollOutOfHundred)
{
    u16 total = 0;
    u8 i;
    u8 remaining = rollOutOfHundred;

    if (weights == NULL || count == 0 || rollOutOfHundred >= 100)
        return -1;
    for (i = 0; i < count; i++)
    {
        if (weights[i] == 0 || total + weights[i] > 100)
            return -1;
        total += weights[i];
    }
    if (total != 100)
        return -1;
    for (i = 0; i < count; i++)
    {
        if (remaining < weights[i])
            return i;
        remaining -= weights[i];
    }
    return -1;
}

// Resolve only pinned source identifiers, never Emerald IDs or a battle.
// No RNG is consumed: the caller supplies the same 0..99 roll as SelectTreeMon.
bool8 JohtoHeadbuttResolveSourceEncounter(const struct JohtoHeadbuttSourceEncounter *rows,
                                          u8 count, u8 rollOutOfHundred,
                                          struct JohtoHeadbuttSourceEncounter *out)
{
    u16 total = 0;
    u8 i;
    u8 remaining = rollOutOfHundred;

    if (rows == NULL || out == NULL || count == 0 || rollOutOfHundred >= 100)
        return FALSE;
    for (i = 0; i < count; i++)
    {
        if (rows[i].weight == 0 || total + rows[i].weight > 100
         || rows[i].sourceSpecies == NULL || rows[i].sourceSpecies[0] == '\0'
         || rows[i].sourceLevelExpression == NULL || rows[i].sourceLevelExpression[0] == '\0')
            return FALSE;
        total += rows[i].weight;
    }
    if (total != 100)
        return FALSE;
    for (i = 0; i < count; i++)
    {
        if (remaining < rows[i].weight)
        {
            *out = rows[i];
            return TRUE;
        }
        remaining -= rows[i].weight;
    }
    return FALSE;
}

// Source: pinned data/wild/badge_base_levels.asm, CountSetBits(wBadges).
// No live badge flags are accessed until the map/field move port is ready.
bool8 JohtoHeadbuttGetBadgeBaseLevel(u8 badgeCount, u8 *outLevel)
{
    static const u8 sBadgeBaseLevels[17] =
    {
        4, 8, 12, 16, 20, 24, 28, 32, 36,
        40, 43, 46, 49, 52, 55, 58, 61
    };

    if (outLevel == NULL || badgeCount >= ARRAY_COUNT(sBadgeBaseLevels))
        return FALSE;
    *outLevel = sBadgeBaseLevels[badgeCount];
    return TRUE;
}

// Source: pinned engine/overworld/wildmons.asm AdjustLevelForBadges.
// Fixed levels pass through unchanged; badge offsets clamp to 2..99.
bool8 JohtoHeadbuttResolveLevel(enum JohtoHeadbuttLevelMode mode,
                                s16 sourceValue, u8 badgeBaseLevel, u8 *outLevel)
{
    s32 level;

    if (outLevel == NULL)
        return FALSE;
    if (mode == JOHTO_HEADBUTT_LEVEL_FIXED)
    {
        if (sourceValue < 1 || sourceValue > 100)
            return FALSE;
        *outLevel = sourceValue;
        return TRUE;
    }
    if (mode != JOHTO_HEADBUTT_LEVEL_BADGE_OFFSET
     || badgeBaseLevel < 4 || badgeBaseLevel > 61
     || sourceValue < -128 || sourceValue > 127)
        return FALSE;

    level = badgeBaseLevel + sourceValue;
    if (level < 2)
        level = 2;
    else if (level > 99)
        level = 99;
    *outLevel = level;
    return TRUE;
}

// Dormant composition of the original badge count lookup and level adjustment.
// Fixed-level encounters do not require a badge count.
bool8 JohtoHeadbuttResolveLevelFromBadgeCount(enum JohtoHeadbuttLevelMode mode,
                                              s16 sourceValue, u8 badgeCount, u8 *outLevel)
{
    u8 badgeBaseLevel;

    if (outLevel == NULL)
        return FALSE;
    if (mode == JOHTO_HEADBUTT_LEVEL_FIXED)
        return JohtoHeadbuttResolveLevel(mode, sourceValue, 0, outLevel);
    if (mode != JOHTO_HEADBUTT_LEVEL_BADGE_OFFSET)
        return FALSE;
    if (!JohtoHeadbuttGetBadgeBaseLevel(badgeCount, &badgeBaseLevel))
        return FALSE;
    return JohtoHeadbuttResolveLevel(mode, sourceValue, badgeBaseLevel, outLevel);
}
