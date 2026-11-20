#ifndef GUARD_POLISHED_DAMAGE_H
#define GUARD_POLISHED_DAMAGE_H

struct DamageContext;
bool32 UsesPolishedDamageRules(struct DamageContext *ctx);
u32 PolishedDamageBeforeRoll(struct DamageContext *ctx, u32 power);
u32 PolishedDamageAfterRoll(struct DamageContext *ctx, u32 damage);
u32 PolishedBattleWeight(enum Species species);
u32 PolishedLowKickPower(u32 weight);
u32 PolishedGyroBallPower(u32 userSpeed, u32 targetSpeed);
bool32 IsPolishedFutureSightImpact(enum BattlerId user, enum BattlerId target, enum Move move);

// The source multiplier reads the low three bytes of the previous quotient.
static inline u32 PolishedDamageScale(u32 value, u32 numerator, u32 denominator)
{
    return (value & 0xFFFFFF) * numerator / denominator;
}

static inline u32 PolishedDamageStage(u32 value, u32 stage)
{
    return stage >= DEFAULT_STAT_STAGE
        ? PolishedDamageScale(value, stage - 4, 2)
        : PolishedDamageScale(value, 2, 8 - stage);
}

#endif
