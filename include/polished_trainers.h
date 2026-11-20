#ifndef GUARD_POLISHED_TRAINERS_H
#define GUARD_POLISHED_TRAINERS_H

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
u32 PolishedGetTrainerBaseMoney(u32 trainer);
bool32 PolishedGenerateTrainerMon(struct Pokemon *mon, const struct TrainerMon *definition,
    struct TrainerGenerator *generator, u32 trainer, u32 slot);

#endif
