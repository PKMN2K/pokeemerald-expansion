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
    bool8 pluralName;
    enum Item items[MAX_TRAINER_ITEMS];
    struct PolishedTrainerAttributes mons[PARTY_SIZE];
};
#include "data/polished_trainers.h"

static const struct PolishedTrainerEntry *FindPolishedTrainer(u32 trainer)
{
#if !IS_FRLG
    for (u32 i = 0; i < ARRAY_COUNT(sPolishedTrainerEntries); i++)
        if (sPolishedTrainerEntries[i].trainer == trainer)
            return &sPolishedTrainerEntries[i];
#endif
    return NULL;
}

bool32 PolishedTrainerHasPluralName(u32 trainer)
{
    const struct PolishedTrainerEntry *entry = FindPolishedTrainer(trainer);
    return entry != NULL && entry->pluralName;
}

const struct PolishedTrainerAttributes *PolishedGetTrainerAttributes(u32 trainer, u32 slot)
{
    const struct PolishedTrainerEntry *entry = FindPolishedTrainer(trainer);
    return entry != NULL && slot < entry->count ? &entry->mons[slot] : NULL;
}

const enum Item *PolishedGetTrainerItems(u32 trainer)
{
    const struct PolishedTrainerEntry *entry = FindPolishedTrainer(trainer);
    return entry != NULL ? entry->items : NULL;
}

u32 PolishedGetTrainerBaseMoney(u32 trainer)
{
    const struct PolishedTrainerEntry *entry = FindPolishedTrainer(trainer);
    return entry != NULL ? entry->baseMoney : 0;
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
#if !IS_FRLG
    if (trainer == TRAINER_POLISHED_GIOVANNI1 && slot == 0)
    {
        static const u8 nickname[] = _("?????");
        SetMonData(mon, MON_DATA_NICKNAME, nickname);
    }
#endif
    // Duplicate ability names must not collapse the source's stored slot.
    u32 abilitySlot = attrs->abilitySlot;
    SetMonData(mon, MON_DATA_ABILITY_NUM, &abilitySlot);
    return TRUE;
}
