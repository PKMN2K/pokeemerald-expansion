#ifndef GUARD_FIELD_MOVE_H
#define GUARD_FIELD_MOVE_H

#include "global.h"
#include "constants/field_move.h"

struct FieldMoveUnlock
{
   bool32 (*isUnlockedFunc)(enum FieldMove);
   const u8 *lockedMessage;
};

enum FieldMoveUnlockType
{
    CANT_UNLOCK,
    ALWAYS_UNLOCKED,
    BADGE_UNLOCK,
    FIELD_MOVE_UNLOCK_COUNT
};

struct FieldMoveInfo
{
    bool32 (*fieldMoveFunc)(void);
    enum FieldMoveUnlockType unlockType:3;
    enum Move moveID:11;
    u32 partyMsgID:7;
    u32 arg:8;
    u32 hideIfLocked:1;
    u32 padding:3;
};

// Polished Crystal Headbutt tree selector; no live Johto map dispatch yet.
enum JohtoHeadbuttTreeScore
{
    JOHTO_HEADBUTT_BAD_TREE,
    JOHTO_HEADBUTT_GOOD_TREE,
    JOHTO_HEADBUTT_RARE_TREE,
};

enum JohtoHeadbuttTreeScore GetJohtoHeadbuttTreeScore(u8 x, u8 y, u16 trainerId);
bool8 JohtoHeadbuttRollStartsBattle(enum JohtoHeadbuttTreeScore score, u8 rollOutOfTen);
bool8 JohtoHeadbuttUsesRareEncounterTable(enum JohtoHeadbuttTreeScore score);
s16 JohtoHeadbuttSelectWeightedEntry(const u8 *weights, u8 count, u8 rollOutOfHundred);

// Source-facing review row. The typed dormant runtime tables are separate;
// this interface preserves original names and expressions for audit tools.
struct JohtoHeadbuttSourceEncounter
{
    u8 weight;
    const char *sourceSpecies;
    const char *sourceForm;
    const char *sourceLevelExpression;
};

bool8 JohtoHeadbuttResolveSourceEncounter(const struct JohtoHeadbuttSourceEncounter *rows,
                                          u8 count, u8 rollOutOfHundred,
                                          struct JohtoHeadbuttSourceEncounter *out);

// Level inputs are source-relative only. The direct level helper accepts a
// verified SetBadgeBaseLevel result; the composition below accepts badge count.
enum JohtoHeadbuttLevelMode
{
    JOHTO_HEADBUTT_LEVEL_FIXED,
    JOHTO_HEADBUTT_LEVEL_BADGE_OFFSET,
};

bool8 JohtoHeadbuttGetBadgeBaseLevel(u8 badgeCount, u8 *outLevel);
bool8 JohtoHeadbuttResolveLevel(enum JohtoHeadbuttLevelMode mode,
                                s16 sourceValue, u8 badgeBaseLevel, u8 *outLevel);

// Accepts an explicit count, not live badge flags. This composes the pinned
// progression table with the isolated level resolver for offline callers.
bool8 JohtoHeadbuttResolveLevelFromBadgeCount(enum JohtoHeadbuttLevelMode mode,
                                              s16 sourceValue, u8 badgeCount, u8 *outLevel);

extern const struct FieldMoveInfo gFieldMoveInfo[];
extern const struct FieldMoveUnlock gFieldMoveUnlocks[];

static inline bool32 SetUpFieldMove(enum FieldMove fieldMove)
{
    return gFieldMoveInfo[fieldMove].fieldMoveFunc();
}

static inline bool32 IsFieldMoveUnlocked(enum FieldMove fieldMove)
{
    return gFieldMoveUnlocks[gFieldMoveInfo[fieldMove].unlockType].isUnlockedFunc(fieldMove);
}

static inline const u8 *FieldMove_GetLockedMessage(enum FieldMove fieldMove)
{
    return gFieldMoveUnlocks[gFieldMoveInfo[fieldMove].unlockType].lockedMessage;
}

static inline enum Move FieldMove_GetMoveId(enum FieldMove fieldMove)
{
    return gFieldMoveInfo[fieldMove].moveID;
}

static inline u32 FieldMove_GetPartyMsgID(enum FieldMove fieldMove)
{
    return gFieldMoveInfo[fieldMove].partyMsgID;
}

static inline bool32 FieldMove_IsVisible(enum FieldMove fieldMove)
{
    return !gFieldMoveInfo[fieldMove].hideIfLocked || IsFieldMoveUnlocked(fieldMove);
}

#endif //GUARD_FIELD_MOVE_H
