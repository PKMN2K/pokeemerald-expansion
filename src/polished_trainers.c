#include "global.h"
#include "pokemon.h"
#include "data.h"
#include "random.h"
#include "trainer_util.h"
#include "constants/opponents.h"
#include "polished_trainers.h"

struct PolishedTrainerEntry
{
    u16 trainer;
    u8 count, baseMoney;
    enum Item items[MAX_TRAINER_ITEMS];
    struct PolishedTrainerAttributes mons[PARTY_SIZE];
};
#include "data/polished_trainers.h"

const struct PolishedTrainerAttributes *PolishedGetTrainerAttributes(u32 trainer, u32 slot)
{
#if !IS_FRLG
    for (u32 i = 0; i < ARRAY_COUNT(sPolishedTrainerEntries); i++)
        if (sPolishedTrainerEntries[i].trainer == trainer && slot < sPolishedTrainerEntries[i].count)
            return &sPolishedTrainerEntries[i].mons[slot];
#endif
    return NULL;
}

const enum Item *PolishedGetTrainerItems(u32 trainer)
{
#if !IS_FRLG
    for (u32 i = 0; i < ARRAY_COUNT(sPolishedTrainerEntries); i++)
        if (sPolishedTrainerEntries[i].trainer == trainer)
            return sPolishedTrainerEntries[i].items;
#endif
    return NULL;
}

u32 PolishedGetTrainerBaseMoney(u32 trainer)
{
#if !IS_FRLG
    for (u32 i = 0; i < ARRAY_COUNT(sPolishedTrainerEntries); i++)
        if (sPolishedTrainerEntries[i].trainer == trainer)
            return sPolishedTrainerEntries[i].baseMoney;
#endif
    return 0;
}

bool32 PolishedGenerateTrainerMon(struct Pokemon *mon, const struct TrainerMon *definition,
    struct TrainerGenerator *generator, u32 trainer, u32 slot)
{
    const struct PolishedTrainerAttributes *attrs = PolishedGetTrainerAttributes(trainer, slot);
    if (!B_POLISHED_MOVE_BALANCE || attrs == NULL)
        return FALSE;
    assertf(definition->species == attrs->species, "Source trainer binding differs from party species");
    struct TrainerMon source = *definition;
    source.iv = attrs->iv;
    source.ev = attrs->ev;
    source.nature = attrs->nature;
    source.gender = attrs->gender;
    source.ability = GetSpeciesAbility(source.species, attrs->abilitySlot);
    source.friendship = 160;
    source.ball = BALL_POKE;
    GenerateMonFromTrainerMon(mon, &source, generator);
    // Duplicate ability names must not collapse the source's stored slot.
    u32 abilitySlot = attrs->abilitySlot;
    SetMonData(mon, MON_DATA_ABILITY_NUM, &abilitySlot);
    return TRUE;
}
