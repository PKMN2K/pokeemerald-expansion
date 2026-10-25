#include "global.h"
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
