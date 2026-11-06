#include "global.h"
#include "config_changes.h"
#include "polished_forms.h"
#include "random.h"

struct PolishedFormIdentity
{
    enum Species species, baseSpecies, eggSpecies;
    u8 form, maxHiddenForm;
};

#include "data/polished_forms.h"

static const struct PolishedFormIdentity *Identity(enum Species species)
{
    for (u32 i = 0; i < ARRAY_COUNT(sPolishedFormIdentities); i++)
        if (sPolishedFormIdentities[i].species == species)
            return &sPolishedFormIdentities[i];
    return NULL;
}

u8 PolishedInitialForm(enum Species species)
{
    const struct PolishedFormIdentity *identity = Identity(species);
    return identity == NULL ? 0 : identity->form;
}

u8 PolishedGetBoxMonForm(struct BoxPokemon *mon)
{
    const struct PolishedFormIdentity *identity = Identity(GetBoxMonData(mon, MON_DATA_SPECIES));
    u8 stored;
    if (identity == NULL)
        return 0;
    stored = GetBoxMonData(mon, MON_DATA_POLISHED_FORM);
    // Visible variants are authoritative. Hidden values are interpreted only
    // for source identities that carry them; old zero fields never reroll.
    if (stored != 0 && stored <= identity->maxHiddenForm)
        return stored;
    return identity->form;
}

enum Species PolishedSpeciesForForm(enum Species species, u8 form)
{
    const struct PolishedFormIdentity *identity = Identity(species);
    if (identity == NULL)
        return species;
    for (u32 i = 0; i < ARRAY_COUNT(sPolishedFormIdentities); i++)
        if (sPolishedFormIdentities[i].baseSpecies == identity->baseSpecies
            && sPolishedFormIdentities[i].form == form)
            return sPolishedFormIdentities[i].species;
    // Ekans/Dunsparce forms, ordinary Gyarados patterns and the pinned
    // daycare's form-21 Magikarp use the base identity plus stored form bits.
    return identity->baseSpecies;
}

enum Species PolishedEggSpecies(enum Species species)
{
    const struct PolishedFormIdentity *identity = Identity(species);
    if (!GetConfig(POLISHED_FORMS) || identity == NULL)
        return SPECIES_NONE;
    return PolishedSpeciesForForm(identity->eggSpecies, identity->form);
}

u8 PolishedSelectWildForm(enum Species *species, u8 explicitForm)
{
    const struct PolishedFormIdentity *identity = Identity(*species);
    u8 form;
    if (!GetConfig(POLISHED_FORMS) || identity == NULL)
        return 0;
    form = explicitForm;
    if (form == 0 && identity->form != 1)
        form = identity->form;
    if (form == 0)
    {
        switch (identity->baseSpecies)
        {
        case SPECIES_EKANS:
        case SPECIES_ARBOK:
            form = RandomUniform(RNG_POLISHED_WILD_FORM, 1, 2);
            break;
        case SPECIES_MAGIKARP:
            form = RandomUniform(RNG_POLISHED_WILD_FORM, 1, POLISHED_MAGIKARP_FORMS);
            break;
        case SPECIES_DUNSPARCE:
        case SPECIES_DUDUNSPARCE:
            form = RandomUniform(RNG_POLISHED_WILD_FORM, 0, POLISHED_RARE_SEGMENT_DENOMINATOR - 1) == 0 ? 2 : 1;
            break;
        default:
            form = 1;
        }
    }
    *species = PolishedSpeciesForForm(*species, form);
    return form;
}

u8 PolishedInheritEggForm(struct DayCare *daycare, enum Species *eggSpecies)
{
    struct BoxPokemon *parents[2] = {&daycare->mons[0].mon, &daycare->mons[1].mon};
    enum Species species[2] = {GetBoxMonData(parents[0], MON_DATA_SPECIES), GetBoxMonData(parents[1], MON_DATA_SPECIES)};
    const struct PolishedFormIdentity *identities[2] = {Identity(species[0]), Identity(species[1])};
    u8 parent, form;
    if (!GetConfig(POLISHED_FORMS))
        return 0;
    if (species[0] == SPECIES_DITTO)
        parent = 1;
    else if (species[1] == SPECIES_DITTO)
        parent = 0;
    else
        parent = GetBoxMonGender(parents[0]) == MON_FEMALE ? 0 : 1;
    if (identities[parent] == NULL)
        return 0;
    if (identities[0] != NULL && identities[1] != NULL
        && identities[0]->baseSpecies == identities[1]->baseSpecies)
        parent = RandomUniform(RNG_POLISHED_FORM_PARENT, 0, 1);
    form = PolishedGetBoxMonForm(parents[parent]);
    // Pinned DayCare_GenerateEgg retains d across its invalid-variant check,
    // then copies d into NO_FORM offspring. Preserve that observable quirk,
    // including named Arbok forms and Red Gyarados -> hidden form-21 Magikarp.
    *eggSpecies = PolishedSpeciesForForm(identities[parent]->eggSpecies, form);
    return form;
}
