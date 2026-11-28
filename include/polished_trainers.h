#ifndef GUARD_POLISHED_TRAINERS_H
#define GUARD_POLISHED_TRAINERS_H

// Marker on roster entries whose unqualified source Arbok changes with the region.
#define POLISHED_REGIONAL_ARBOK (1u << 31)
#if POLISHED_JOHTO_DEMO
enum PolishedAiLayers
{
    POLISHED_AI_BASIC = 1 << 0, POLISHED_AI_SETUP = 1 << 1, POLISHED_AI_TYPES = 1 << 2,
    POLISHED_AI_OFFENSIVE = 1 << 3, POLISHED_AI_SMART = 1 << 4, POLISHED_AI_OPPORTUNIST = 1 << 5,
    POLISHED_AI_AGGRESSIVE = 1 << 6, POLISHED_AI_CAUTIOUS = 1 << 7, POLISHED_AI_STATUS = 1 << 8,
    POLISHED_AI_RISKY = 1 << 9,
};
u32 PolishedTrainerAiLayers(u32 trainer, u32 badges);
void PolishedScoreSetup(const enum Move *moves, u32 limitations, bool32 playerFirstTurn,
                       bool32 enemyGhost, s32 *score);
struct BattlePokemon;
struct PolishedSmartAiContext
{
    u32 weather, userWeather;
    u32 userSpeed, targetSpeed;
    bool8 trickRoom, targetFirstTurn;
    bool8 lastMonAgainstReserve;
    bool8 earlyBird, targetGhost, partyHasStatus;
    u8 spikes;
};
void PolishedScoreBasicSleepMoves(const struct BattlePokemon *user, const struct BattlePokemon *target,
                                 u32 limitations, bool32 safeguard, s32 *scores);
void PolishedScoreSmartMoves(const struct BattlePokemon *user, const struct BattlePokemon *target,
                            u32 limitations, const struct PolishedSmartAiContext *context, s32 *scores);
void PolishedScoreTrainerPolicies(u32 layers, const enum Move *moves, u32 limitations,
                                  u32 hp, u32 maxHP, bool32 hasTakenTurn, s32 *score);
#endif
struct TrainerMon;
struct TrainerGenerator;
struct PolishedTrainerAttributes
{
    u32 iv;
    u16 species;
    u8 ev[6]; // TrainerMon/Showdown order.
    u8 abilitySlot, nature, gender;
};

const struct PolishedTrainerAttributes *PolishedGetTrainerAttributes(u32 trainer, u32 slot);
const enum Item *PolishedGetTrainerItems(u32 trainer);
bool32 PolishedTrainerHasPluralName(u32 trainer);
u32 PolishedGetTrainerBaseMoney(u32 trainer);
bool32 PolishedGenerateTrainerMon(struct Pokemon *mon, const struct TrainerMon *definition,
    struct TrainerGenerator *generator, u32 trainer, u32 slot);

#endif
