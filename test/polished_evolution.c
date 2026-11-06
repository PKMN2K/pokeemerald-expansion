#include "global.h"
#include "battle.h"
#include "config_changes.h"
#include "pokemon.h"
#include "polished_evolution.h"
#include "test/test.h"

static enum Species At(struct Pokemon *mon, enum EvolutionMode mode, enum Item item, u32 hour)
{
    return PolishedFindEvolution(mon, mode, item, hour, POLISHED_LANDMARK_NONE, 0);
}

TEST("Polished evolutions cover every source identity and preserve optional Expansion species")
{
    u32 count = 0;
    SetConfig(CONFIG_POLISHED_EVOLUTIONS, TRUE);
    for (enum Species species = SPECIES_NONE; species < NUM_SPECIES; species++)
        if (PolishedGetSpeciesEvolutions(species) != NULL)
        {
            count++;
            EXPECT(GetSpeciesEvolutions(species) == PolishedGetSpeciesEvolutions(species));
        }
    EXPECT_EQ(count, 391);
    EXPECT(PolishedGetSpeciesEvolutions(NUM_SPECIES) == NULL);
    EXPECT(PolishedGetSpeciesEvolutions(-1) == NULL);
    EXPECT_EQ(GetSpeciesEvolutions(SPECIES_MR_RIME)[0].method, EVOLUTIONS_END);
    EXPECT_EQ(GetSpeciesEvolutions(SPECIES_FARIGIRAF)[0].method, EVOLUTIONS_END);
    EXPECT_EQ(GetSpeciesEvolutions(SPECIES_FARFETCHD)[0].method, EVOLUTIONS_END);
    EXPECT_EQ(GetSpeciesEvolutions(SPECIES_PICHU_SPIKY_EARED)[0].method, EVOLUTIONS_END);
    EXPECT_EQ(GetSpeciesEvolutions(SPECIES_MUDKIP)[0].targetSpecies, SPECIES_MARSHTOMP);
    SetConfig(CONFIG_POLISHED_EVOLUTIONS, FALSE);
    EXPECT(GetSpeciesEvolutions(SPECIES_CYNDAQUIL) == gSpeciesInfo[SPECIES_CYNDAQUIL].evolutions);
}

TEST("Polished evolution level thresholds come from the source, including regional forms")
{
    enum Species species, target;
    u32 level;
    PARAMETRIZE { species = SPECIES_CYNDAQUIL; level = 14; target = SPECIES_QUILAVA; }
    PARAMETRIZE { species = SPECIES_TOTODILE; level = 18; target = SPECIES_CROCONAW; }
    PARAMETRIZE { species = SPECIES_FLAAFFY; level = 36; target = SPECIES_AMPHAROS; }
    PARAMETRIZE { species = SPECIES_DUNSPARCE; level = 32; target = SPECIES_DUDUNSPARCE; }
    PARAMETRIZE { species = SPECIES_QWILFISH_HISUI; level = 33; target = SPECIES_OVERQWIL; }
    PARAMETRIZE { species = SPECIES_RATTATA_ALOLA; level = 20; target = SPECIES_RATICATE_ALOLA; }
    PARAMETRIZE { species = SPECIES_MEOWTH_ALOLA; level = 28; target = SPECIES_PERSIAN_ALOLA; }
    struct Pokemon mon;
    SetConfig(CONFIG_POLISHED_EVOLUTIONS, TRUE);
    CreateRandomMon(&mon, species, level - 1);
    EXPECT_EQ(GetEvolutionTargetSpecies(&mon, EVO_MODE_NORMAL, ITEM_NONE, NULL, NULL, CHECK_EVO), SPECIES_NONE);
    SetMonData(&mon, MON_DATA_LEVEL, &level);
    EXPECT_EQ(GetEvolutionTargetSpecies(&mon, EVO_MODE_NORMAL, ITEM_NONE, NULL, NULL, CHECK_EVO), target);
}

TEST("Polished Eevee has all eight source stone evolutions")
{
    enum Item item;
    enum Species target;
    PARAMETRIZE { item = ITEM_THUNDER_STONE; target = SPECIES_JOLTEON; }
    PARAMETRIZE { item = ITEM_WATER_STONE; target = SPECIES_VAPOREON; }
    PARAMETRIZE { item = ITEM_FIRE_STONE; target = SPECIES_FLAREON; }
    PARAMETRIZE { item = ITEM_SUN_STONE; target = SPECIES_ESPEON; }
    PARAMETRIZE { item = ITEM_MOON_STONE; target = SPECIES_UMBREON; }
    PARAMETRIZE { item = ITEM_LEAF_STONE; target = SPECIES_LEAFEON; }
    PARAMETRIZE { item = ITEM_ICE_STONE; target = SPECIES_GLACEON; }
    PARAMETRIZE { item = ITEM_SHINY_STONE; target = SPECIES_SYLVEON; }
    struct Pokemon mon;
    enum Item everstone = ITEM_EVERSTONE;
    SetConfig(CONFIG_POLISHED_EVOLUTIONS, TRUE);
    CreateRandomMon(&mon, SPECIES_EEVEE, 5);
    EXPECT_EQ(GetEvolutionTargetSpecies(&mon, EVO_MODE_ITEM_CHECK, item, NULL, NULL, CHECK_EVO), target);
    SetMonData(&mon, MON_DATA_HELD_ITEM, &everstone);
    EXPECT_EQ(GetEvolutionTargetSpecies(&mon, EVO_MODE_ITEM_CHECK, item, NULL, NULL, CHECK_EVO), SPECIES_NONE);
    EXPECT_EQ(GetEvolutionTargetSpecies(&mon, EVO_MODE_ITEM_USE, item, NULL, NULL, DO_EVO), SPECIES_NONE);
}

TEST("Polished Odd Souvenir evolves the three source regional species")
{
    enum Species species, target;
    PARAMETRIZE { species = SPECIES_PIKACHU; target = SPECIES_RAICHU_ALOLA; }
    PARAMETRIZE { species = SPECIES_PIKACHU_SURF; target = SPECIES_RAICHU_ALOLA; }
    PARAMETRIZE { species = SPECIES_EXEGGCUTE; target = SPECIES_EXEGGUTOR_ALOLA; }
    PARAMETRIZE { species = SPECIES_CUBONE; target = SPECIES_MAROWAK_ALOLA; }
    struct Pokemon mon;
    CreateRandomMon(&mon, species, 5);
    EXPECT_EQ(At(&mon, EVO_MODE_ITEM_USE, ITEM_POLISHED_ODD_SOUVENIR, 12), target);
    EXPECT_EQ(At(&mon, EVO_MODE_ITEM_USE, ITEM_STRANGE_SOUVENIR, 12), SPECIES_NONE);
}

TEST("Polished friendship uses 160 and the source clock boundaries; spiky Pichu never evolves")
{
    struct Pokemon mon;
    u32 friendship;
    for (u32 hour = 0; hour < 24; hour++)
    {
        CreateRandomMon(&mon, SPECIES_EEVEE, 5);
        friendship = 159;
        SetMonData(&mon, MON_DATA_FRIENDSHIP, &friendship);
        EXPECT_EQ(At(&mon, EVO_MODE_NORMAL, ITEM_NONE, hour), SPECIES_NONE);
        friendship = 160;
        SetMonData(&mon, MON_DATA_FRIENDSHIP, &friendship);
        EXPECT_EQ(At(&mon, EVO_MODE_NORMAL, ITEM_NONE, hour), hour >= 5 && hour < 17 ? SPECIES_ESPEON : SPECIES_UMBREON);
    }
    CreateRandomMon(&mon, SPECIES_PICHU_SPIKY_EARED, 50);
    friendship = 255;
    SetMonData(&mon, MON_DATA_FRIENDSHIP, &friendship);
    EXPECT_EQ(At(&mon, EVO_MODE_NORMAL, ITEM_NONE, 12), SPECIES_NONE);
    CreateRandomMon(&mon, SPECIES_PICHU, 5);
    SetMonData(&mon, MON_DATA_FRIENDSHIP, &friendship);
    EXPECT_EQ(At(&mon, EVO_MODE_NORMAL, ITEM_NONE, 12), SPECIES_PIKACHU);
}

TEST("Polished Razor Claw uses complementary source time groups and retains its item until success")
{
    struct Pokemon mon;
    enum Item item = ITEM_RAZOR_CLAW;
    SetConfig(CONFIG_POLISHED_EVOLUTIONS, TRUE);
    for (u32 hour = 0; hour < 24; hour++)
    {
        CreateRandomMon(&mon, SPECIES_SNEASEL_HISUI, 5);
        SetMonData(&mon, MON_DATA_HELD_ITEM, &item);
        EXPECT_EQ(At(&mon, EVO_MODE_NORMAL, ITEM_NONE, hour), hour >= 5 && hour < 17 ? SPECIES_SNEASLER : SPECIES_NONE);
        EXPECT_EQ(GetMonData(&mon, MON_DATA_HELD_ITEM), item);
        CreateRandomMon(&mon, SPECIES_SNEASEL, 5);
        SetMonData(&mon, MON_DATA_HELD_ITEM, &item);
        EXPECT_EQ(At(&mon, EVO_MODE_NORMAL, ITEM_NONE, hour), hour >= 5 && hour < 17 ? SPECIES_NONE : SPECIES_WEAVILE);
    }
    PolishedCommitEvolution(&mon, SPECIES_WEAVILE);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_HELD_ITEM), ITEM_NONE);
}

TEST("Polished held trade items work on level up, trade and item use in source order")
{
    struct Pokemon mon;
    enum Item item = ITEM_KINGS_ROCK;
    bool32 canStop = TRUE;
    SetConfig(CONFIG_POLISHED_EVOLUTIONS, TRUE);
    CreateRandomMon(&mon, SPECIES_POLIWHIRL, 25);
    SetMonData(&mon, MON_DATA_HELD_ITEM, &item);
    EXPECT_EQ(GetEvolutionTargetSpecies(&mon, EVO_MODE_NORMAL, ITEM_NONE, NULL, &canStop, CHECK_EVO), SPECIES_POLITOED);
    EXPECT(canStop);
    EXPECT_EQ(GetEvolutionTargetSpecies(&mon, EVO_MODE_NORMAL, ITEM_NONE, NULL, NULL, DO_EVO), SPECIES_POLITOED);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_HELD_ITEM), item);
    EXPECT_EQ(GetEvolutionTargetSpecies(&mon, EVO_MODE_TRADE, ITEM_NONE, NULL, &canStop, DO_EVO), SPECIES_POLITOED);
    EXPECT(!canStop);
    EXPECT_EQ(GetEvolutionTargetSpecies(&mon, EVO_MODE_ITEM_USE, ITEM_WATER_STONE, NULL, NULL, CHECK_EVO), SPECIES_POLITOED);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_HELD_ITEM), item);
    PolishedCommitEvolution(&mon, SPECIES_POLITOED);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_HELD_ITEM), ITEM_NONE);
    EXPECT_EQ(At(&mon, EVO_MODE_ITEM_USE, ITEM_WATER_STONE, 12), SPECIES_POLIWRATH);
}

TEST("Polished Linking Cord and actual trade respect Kadabra's Everstone")
{
    struct Pokemon mon;
    enum Item item = ITEM_EVERSTONE;
    CreateRandomMon(&mon, SPECIES_KADABRA, 20);
    EXPECT_EQ(At(&mon, EVO_MODE_NORMAL, ITEM_NONE, 12), SPECIES_NONE);
    EXPECT_EQ(At(&mon, EVO_MODE_ITEM_USE, ITEM_LINKING_CORD, 12), SPECIES_ALAKAZAM);
    EXPECT_EQ(At(&mon, EVO_MODE_TRADE, ITEM_NONE, 12), SPECIES_ALAKAZAM);
    SetMonData(&mon, MON_DATA_HELD_ITEM, &item);
    EXPECT_EQ(At(&mon, EVO_MODE_TRADE, ITEM_NONE, 12), SPECIES_NONE);
    EXPECT_EQ(At(&mon, EVO_MODE_ITEM_USE, ITEM_LINKING_CORD, 12), SPECIES_NONE);
}

TEST("Polished move evolutions use source moves and source Everstone behavior")
{
    enum Species species, target;
    enum Move move;
    PARAMETRIZE { species = SPECIES_PRIMEAPE; move = MOVE_OUTRAGE; target = SPECIES_ANNIHILAPE; }
    PARAMETRIZE { species = SPECIES_AIPOM; move = MOVE_DOUBLE_SLAP; target = SPECIES_AMBIPOM; }
    PARAMETRIZE { species = SPECIES_GIRAFARIG; move = MOVE_ZEN_HEADBUTT; target = SPECIES_FARIGIRAF; }
    PARAMETRIZE { species = SPECIES_BONSLY; move = MOVE_ROCK_THROW; target = SPECIES_SUDOWOODO; }
    struct Pokemon mon;
    enum Item item = ITEM_EVERSTONE;
    CreateRandomMon(&mon, species, 5);
    for (u32 slot = 0; slot < MAX_MON_MOVES; slot++)
        SetMonMoveSlot(&mon, MOVE_NONE, slot);
    EXPECT_EQ(At(&mon, EVO_MODE_NORMAL, ITEM_NONE, 12), SPECIES_NONE);
    SetMonMoveSlot(&mon, move, 2);
    SetMonData(&mon, MON_DATA_HELD_ITEM, &item);
    EXPECT_EQ(At(&mon, EVO_MODE_NORMAL, ITEM_NONE, 12), target);
}

TEST("Polished landmark evolutions use explicit source locations and retain row priority")
{
    struct Pokemon mon;
    CreateRandomMon(&mon, SPECIES_MIME_JR, 30);
    EXPECT_EQ(PolishedFindEvolution(&mon, EVO_MODE_NORMAL, ITEM_NONE, 12, POLISHED_LANDMARK_ICE_PATH, 0), SPECIES_MR_MIME_GALAR);
    EXPECT_EQ(At(&mon, EVO_MODE_NORMAL, ITEM_NONE, 12), SPECIES_MR_MIME);
    CreateRandomMon(&mon, SPECIES_MAGNETON, 5);
    EXPECT_EQ(PolishedFindEvolution(&mon, EVO_MODE_NORMAL, ITEM_NONE, 12, POLISHED_LANDMARK_DIM_CAVE, 0), SPECIES_MAGNEZONE);
    EXPECT_EQ(PolishedFindEvolution(&mon, EVO_MODE_NORMAL, ITEM_NONE, 12, POLISHED_LANDMARK_ICE_PATH, 0), SPECIES_NONE);
    CreateRandomMon(&mon, SPECIES_STANTLER, 5);
    EXPECT_EQ(PolishedFindEvolution(&mon, EVO_MODE_NORMAL, ITEM_NONE, 12, POLISHED_LANDMARK_SINJOH_RUINS, 0), SPECIES_WYRDEER);
    EXPECT_EQ(PolishedGetEvolutionLandmark(), POLISHED_LANDMARK_NONE);
}

TEST("Polished Tyrogue compares actual attack and defense in the default EV profile")
{
    struct Pokemon mon;
    u32 attack, defense = 40;
    CreateRandomMon(&mon, SPECIES_TYROGUE, 20);
    SetMonData(&mon, MON_DATA_DEF, &defense);
    attack = 39;
    SetMonData(&mon, MON_DATA_ATK, &attack);
    EXPECT_EQ(At(&mon, EVO_MODE_NORMAL, ITEM_NONE, 12), SPECIES_HITMONCHAN);
    attack = 40;
    SetMonData(&mon, MON_DATA_ATK, &attack);
    EXPECT_EQ(At(&mon, EVO_MODE_NORMAL, ITEM_NONE, 12), SPECIES_HITMONTOP);
    attack = 41;
    SetMonData(&mon, MON_DATA_ATK, &attack);
    EXPECT_EQ(At(&mon, EVO_MODE_NORMAL, ITEM_NONE, 12), SPECIES_HITMONLEE);
}

TEST("Polished Mantyke requires Remoraid in the active party")
{
    struct Pokemon mon;
    CreateRandomMon(&mon, SPECIES_MANTYKE, 5);
    ZeroPlayerPartyMons();
    CreateRandomMon(&gParties[B_TRAINER_PLAYER][0], SPECIES_REMORAID, 5);
    EXPECT_EQ(At(&mon, EVO_MODE_NORMAL, ITEM_NONE, 12), SPECIES_NONE);
    gPartiesCount[B_TRAINER_PLAYER] = 1;
    EXPECT_EQ(At(&mon, EVO_MODE_NORMAL, ITEM_NONE, 12), SPECIES_MANTINE);
}

TEST("Polished three-critical evolution checks require a successful battle and never a whiteout")
{
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][0];
    SetConfig(CONFIG_POLISHED_EVOLUTIONS, TRUE);
    ZeroPlayerPartyMons();
    CreateRandomMon(mon, SPECIES_FARFETCHD_GALAR, 5);
    gPartiesCount[B_TRAINER_PLAYER] = 1;
    gBattleOutcome = B_OUTCOME_WON;
    gPartyCriticalHits[0] = 2;
    EXPECT_EQ(GetEvolutionTargetSpecies(mon, EVO_MODE_BATTLE_SPECIAL, 0, NULL, NULL, CHECK_EVO), SPECIES_NONE);
    gPartyCriticalHits[0] = 3;
    EXPECT_EQ(GetEvolutionTargetSpecies(mon, EVO_MODE_BATTLE_SPECIAL, 0, NULL, NULL, CHECK_EVO), SPECIES_SIRFETCHD);
    gBattleOutcome = B_OUTCOME_LOST;
    EXPECT_EQ(GetEvolutionTargetSpecies(mon, EVO_MODE_BATTLE_SPECIAL, 0, NULL, NULL, CHECK_EVO), SPECIES_NONE);
    CreateRandomMon(mon, SPECIES_CYNDAQUIL, 14);
    EXPECT_EQ(GetEvolutionTargetSpecies(mon, EVO_MODE_BATTLE_ONLY, 0, NULL, NULL, CHECK_EVO), SPECIES_NONE);
    gBattleOutcome = B_OUTCOME_WON;
    EXPECT_EQ(GetEvolutionTargetSpecies(mon, EVO_MODE_BATTLE_SPECIAL, 0, NULL, NULL, CHECK_EVO), SPECIES_QUILAVA);
}

TEST("Polished level-100 Rare Candy evolution is forced, as in the source item effect")
{
    struct Pokemon mon;
    bool32 canStop = TRUE;
    SetConfig(CONFIG_POLISHED_EVOLUTIONS, TRUE);
    CreateRandomMon(&mon, SPECIES_CYNDAQUIL, 100);
    EXPECT_EQ(GetEvolutionTargetSpecies(&mon, EVO_MODE_NORMAL, ITEM_RARE_CANDY, NULL, &canStop, CHECK_EVO), SPECIES_QUILAVA);
    EXPECT(!canStop);
}
