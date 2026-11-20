#ifndef GUARD_POLISHED_CAPTURE_H
#define GUARD_POLISHED_CAPTURE_H

#include "constants/pokeball.h"

struct PolishedCaptureContext
{
    enum PokeBall ball;
    u32 status;
    u16 hp, maxHP, weight;
    u16 completedTurns;
    u8 catchRate, userLevel, targetLevel, baseSpeed;
    u8 type1, type2;
    bool8 fishing, surfing, dusk, caught, moonEvolution;
    bool8 loveEligible, tutorial;
};

// Heavy Ball changes the stored catch rate, including across later throws.
u32 PolishedCaptureOdds(struct PolishedCaptureContext *ctx);
u32 PolishedHeavyBallCatchRate(u32 catchRate, u32 weight);
u32 PolishedCriticalCaptureThreshold(u32 odds, u32 caught, bool32 charm);
u32 PolishedCaptureWobbleThreshold(u32 odds);
bool32 UsesPolishedCaptureRules(enum BattlerId user, enum BattlerId target);
u32 GetPolishedCaptureOdds(enum BattlerId user, enum BattlerId target);
u32 PolishedCaptureCaughtCount(void);
bool32 PolishedWasCaught(enum Species species);
void PolishedRecordCaughtSpecies(enum Species species);
bool32 UsesPolishedSafariRules(void);
u32 PolishedSafariFleeThreshold(u32 speed, u32 balls, u32 eating, u32 anger);
void PolishedSafariBaitOrRock(bool32 rock);
u32 PolishedSafariReaction(void);
bool32 PolishedSafariFlees(void);

#endif
