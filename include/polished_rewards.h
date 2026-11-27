#ifndef GUARD_POLISHED_REWARDS_H
#define GUARD_POLISHED_REWARDS_H

struct PolishedExpContext
{
    u16 baseExp;
    u8 defeatedLevel, participants, holders;
    bool8 participant, holder, traded, trainer, luckyEgg;
};

bool32 UsesPolishedPostBattleAbilities(void);
enum Item PolishedApplyPostBattleAbility(struct Pokemon *mon);
u32 PolishedExperience(const struct PolishedExpContext *ctx);
bool32 UsesPolishedRewardRules(enum BattlerId defeated);
bool32 PolishedIsTraded(struct Pokemon *mon);
u32 PolishedPrepareExperience(enum BattlerId defeated);
u32 PolishedBattleExperience(u32 partyId, enum BattlerId defeated);
void PolishedGainBattleEVs(struct Pokemon *mon, enum Species defeatedSpecies);

#endif
