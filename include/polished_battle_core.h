#ifndef GUARD_POLISHED_BATTLE_CORE_H
#define GUARD_POLISHED_BATTLE_CORE_H

// HitSelfInConfusion -> TruncateHL_BC -> ConfusedDamageCalc. These raw stats
// deliberately exclude ability/item/badge/weather/burn damage modifiers.
static inline u32 PolishedConfusionDamage(u32 level, u32 attack, u32 defense, u32 attackStage, u32 defenseStage)
{
    u32 damage;
    while (attack > 255 || defense > 255)
    {
        attack = max(1, attack / 2);
        defense = max(1, defense / 2);
    }
    defense = max(1, defense);
    damage = (2 * level / 5 + 2) * 40 * attack;
    // Source stages scale the damage numerator, not the input stat. Flooring
    // each stat first produces different answers for small and odd values.
    if (attackStage >= DEFAULT_STAT_STAGE)
        damage = damage * (2 + attackStage - DEFAULT_STAT_STAGE) / 2;
    else
        damage = damage * 2 / (2 + DEFAULT_STAT_STAGE - attackStage);
    if (defenseStage >= DEFAULT_STAT_STAGE)
        damage = damage * 2 / (2 + defenseStage - DEFAULT_STAT_STAGE);
    else
        damage = damage * (2 + DEFAULT_STAT_STAGE - defenseStage) / 2;
    damage = damage / defense / 50;
    // ResetDamage clears the low byte before DamagePass4's $ff** clamp.
    return damage >= 0xFF00 ? 0xFF00 : damage + 2;
}

#endif
