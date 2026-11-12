#include "global.h"
#include "battle.h"
#include "move.h"
#include "pokemon.h"
#include "polished_moves.h"
#include "test/test.h"

TEST("Polished move PP selects all 255 source records and preserves native optional moves")
{
    SetConfig(CONFIG_B_POLISHED_MOVE_PP, TRUE);
    u32 mapped = 0;
    for (u32 move = 0; move < MOVES_COUNT_ALL; move++)
    {
        u32 sourcePP = move < MOVES_COUNT ? gPolishedMovePP[move] : 0;
        EXPECT_EQ(GetMovePP(move), sourcePP ? sourcePP : gMovesInfo[move].pp);
        if (sourcePP)
            mapped++;
    }
    EXPECT_EQ(mapped, 255);
    EXPECT_EQ(GetMovePP(MOVE_HONE_CLAWS), 40);
    EXPECT_EQ(GetMovePP(MOVE_ROCK_BLAST), 25);
    EXPECT_EQ(GetMovePP(MOVE_METRONOME), 20);
    EXPECT_EQ(GetMovePP(MOVE_RAPID_SPIN), 20);
    EXPECT_EQ(GetMovePP(MOVE_TAKE_DOWN), 15);
    EXPECT_EQ(GetMovePP(MOVE_NONE), 0);
    // The public getter deliberately asserts invalid IDs in test builds;
    // the adapter must safely reject them before any source-table indexing.
    EXPECT_EQ(PolishedGetMovePP(MOVES_COUNT_ALL), 0);
    EXPECT_EQ(PolishedGetMovePP((enum Move)-1), 0);
    EXPECT_EQ(PolishedGetMovePP(MOVE_LEAFAGE), 0);
    EXPECT_EQ(CalculatePPWithBonus(MOVE_LEAFAGE, 0xff, 0), 64);
    SetConfig(CONFIG_B_POLISHED_MOVE_PP, FALSE);
    for (u32 move = 0; move < MOVES_COUNT_ALL; move++)
        EXPECT_EQ(GetMovePP(move), gMovesInfo[move].pp);
    EXPECT_EQ(CalculatePPWithBonus(MOVE_GROWL, 0xff, 0), 64);
}

TEST("Polished move PP calculates every source maximum in every move slot")
{
    SetConfig(CONFIG_B_POLISHED_MOVE_PP, TRUE);
    for (u32 move = 1; move < MOVES_COUNT; move++)
    {
        u32 base = gPolishedMovePP[move];
        if (!base)
            continue;
        for (u32 slot = 0; slot < MAX_MON_MOVES; slot++)
        {
            for (u32 count = 0; count < 4; count++)
            {
                // Other slots are maxed, so their bonus bits must be ignored.
                u32 bonuses = (0xff & ~(3 << (2 * slot))) | (count << (2 * slot));
                EXPECT_EQ(CalculatePPWithBonus(move, bonuses, slot), base + min(base / 5, 7) * count);
            }
        }
    }
    // Also exhaust all packed bonus bytes for the source's exceptional 40 PP case.
    for (u32 bonuses = 0; bonuses < 256; bonuses++)
        for (u32 slot = 0; slot < MAX_MON_MOVES; slot++)
            EXPECT_EQ(CalculatePPWithBonus(MOVE_GROWL, bonuses, slot), 40 + 7 * ((bonuses >> (2 * slot)) & 3));
}

TEST("Polished move PP initializes and restores source moves through party and box APIs")
{
    SetConfig(CONFIG_B_POLISHED_MOVE_PP, TRUE);
    struct Pokemon mon;
    CreateRandomMon(&mon, SPECIES_SMEARGLE, 50);
    const enum Move moves[] = {MOVE_HONE_CLAWS, MOVE_ROCK_BLAST, MOVE_RAPID_SPIN, MOVE_SKETCH};
    const u32 base[] = {40, 25, 20, 1}, full[] = {61, 40, 32, 1};
    for (u32 slot = 0; slot < MAX_MON_MOVES; slot++)
    {
        SetMonMoveSlot(&mon, moves[slot], slot);
        EXPECT_EQ(GetMonData(&mon, MON_DATA_PP1 + slot), base[slot]);
    }
    u32 bonuses = 0xff, empty = 0;
    SetMonData(&mon, MON_DATA_PP_BONUSES, &bonuses);
    for (u32 boxed = 0; boxed < 2; boxed++)
    {
        for (u32 slot = 0; slot < MAX_MON_MOVES; slot++)
            SetMonData(&mon, MON_DATA_PP1 + slot, &empty);
        if (boxed)
            BoxMonRestorePP(&mon.box);
        else
            MonRestorePP(&mon);
        for (u32 slot = 0; slot < MAX_MON_MOVES; slot++)
            EXPECT_EQ(GetMonData(&mon, MON_DATA_PP1 + slot), full[slot]);
        EXPECT_EQ(GetMonData(&mon, MON_DATA_PP_BONUSES), 0xff);
    }
}

TEST("Polished move PP Up and PP Max add source increments without restoring spent PP")
{
    u32 slot = 0;
    for (u32 n = 0; n < MAX_MON_MOVES; n++)
        PARAMETRIZE { slot = n; }
    SetConfig(CONFIG_B_POLISHED_MOVE_PP, TRUE);
    gMain.inBattle = FALSE;
    struct Pokemon mon;
    CreateRandomMon(&mon, SPECIES_SMEARGLE, 50);
    SetMonMoveSlot(&mon, MOVE_GROWL, slot);
    u32 bonuses = 0xff & ~(3 << (2 * slot)), pp = 10;
    SetMonData(&mon, MON_DATA_PP_BONUSES, &bonuses);
    SetMonData(&mon, MON_DATA_PP1 + slot, &pp);
    for (u32 count = 1; count <= 3; count++)
    {
        EXPECT(!PokemonUseItemEffects(&mon, ITEM_PP_UP, 0, slot, FALSE));
        EXPECT_EQ(GetMonData(&mon, MON_DATA_PP1 + slot), 10 + 7 * count);
        EXPECT_EQ(GetMonData(&mon, MON_DATA_PP_BONUSES), bonuses | (count << (2 * slot)));
    }
    EXPECT(PokemonUseItemEffects(&mon, ITEM_PP_UP, 0, slot, FALSE));
    for (u32 count = 0; count < 3; count++)
    {
        u32 oldBonuses = bonuses | (count << (2 * slot));
        SetMonData(&mon, MON_DATA_PP_BONUSES, &oldBonuses);
        SetMonData(&mon, MON_DATA_PP1 + slot, &pp);
        EXPECT(!PokemonUseItemEffects(&mon, ITEM_PP_MAX, 0, slot, FALSE));
        EXPECT_EQ(GetMonData(&mon, MON_DATA_PP1 + slot), 10 + 7 * (3 - count));
        EXPECT_EQ(GetMonData(&mon, MON_DATA_PP_BONUSES), 0xff);
        EXPECT(PokemonUseItemEffects(&mon, ITEM_PP_MAX, 0, slot, FALSE));
    }
    SetMonMoveSlot(&mon, MOVE_SKETCH, slot);
    SetMonData(&mon, MON_DATA_PP_BONUSES, &bonuses);
    EXPECT(PokemonUseItemEffects(&mon, ITEM_PP_UP, 0, slot, FALSE));
    EXPECT(PokemonUseItemEffects(&mon, ITEM_PP_MAX, 0, slot, FALSE));
    EXPECT_EQ(GetMonData(&mon, MON_DATA_PP1 + slot), 1);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_PP_BONUSES), bonuses);
}

TEST("Polished move PP restoration items respect source caps and reject full moves")
{
    SetConfig(CONFIG_B_POLISHED_MOVE_PP, TRUE);
    gMain.inBattle = FALSE;
    struct Pokemon mon;
    CreateRandomMon(&mon, SPECIES_SMEARGLE, 50);
    const enum Move moves[] = {MOVE_GROWL, MOVE_ROCK_BLAST, MOVE_RAPID_SPIN, MOVE_SKETCH};
    const u32 caps[] = {61, 40, 32, 1};
    for (u32 slot = 0; slot < MAX_MON_MOVES; slot++)
        SetMonMoveSlot(&mon, moves[slot], slot);
    u32 bonuses = 0xff;
    SetMonData(&mon, MON_DATA_PP_BONUSES, &bonuses);
    for (u32 slot = 0; slot < MAX_MON_MOVES; slot++)
    {
        u32 pp = caps[slot] - 1;
        SetMonData(&mon, MON_DATA_PP1 + slot, &pp);
        EXPECT(!PokemonUseItemEffects(&mon, ITEM_ETHER, 0, slot, FALSE));
        EXPECT_EQ(GetMonData(&mon, MON_DATA_PP1 + slot), caps[slot]);
        EXPECT(PokemonUseItemEffects(&mon, ITEM_ETHER, 0, slot, FALSE));
        pp = 0;
        SetMonData(&mon, MON_DATA_PP1 + slot, &pp);
        EXPECT(!PokemonUseItemEffects(&mon, ITEM_MAX_ETHER, 0, slot, FALSE));
        EXPECT_EQ(GetMonData(&mon, MON_DATA_PP1 + slot), caps[slot]);
    }
    for (u32 slot = 0; slot < MAX_MON_MOVES; slot++)
    {
        u32 pp = 0;
        SetMonData(&mon, MON_DATA_PP1 + slot, &pp);
    }
    EXPECT(!PokemonUseItemEffects(&mon, ITEM_ELIXIR, 0, 0, FALSE));
    for (u32 slot = 0; slot < MAX_MON_MOVES; slot++)
        EXPECT_EQ(GetMonData(&mon, MON_DATA_PP1 + slot), min(10, caps[slot]));
    EXPECT(!PokemonUseItemEffects(&mon, ITEM_MAX_ELIXIR, 0, 0, FALSE));
    for (u32 slot = 0; slot < MAX_MON_MOVES; slot++)
        EXPECT_EQ(GetMonData(&mon, MON_DATA_PP1 + slot), caps[slot]);
    EXPECT(PokemonUseItemEffects(&mon, ITEM_MAX_ELIXIR, 0, 0, FALSE));
    EXPECT_EQ(GetMonData(&mon, MON_DATA_PP_BONUSES), 0xff);
}
