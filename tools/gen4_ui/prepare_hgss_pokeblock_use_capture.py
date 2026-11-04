#!/usr/bin/env python3
"""Disposable real-case feeding fixture; never commit patched C, ROM or saves."""
from pathlib import Path

link = Path('ld_script_modern.ld')
s = link.read_text()
old = 'gInitialMainCB2 = CB2_InitCopyrightScreenAfterBootup;'
assert s.count(old) == 1
link.write_text(s.replace(old, 'gInitialMainCB2 = CB2_HgssPokeblockUseCapture;'))
p = Path('src/pokeblock.c')
s = p.read_text().replace('#include "pokeblock.h"', '#include "pokeblock.h"\n#include "load_save.h"\n#include "pokemon_storage_system.h"\n#include "constants/species.h"')
s = s.replace('static void ReturnToPokeblockCaseOnField(void)\n{', 'static void ReturnToPokeblockCaseOnField(void)\n{\n    HgssCaptureAudit();')
s = s.replace('#define MAX_MENU_ITEMS', 'static void HgssCaptureAudit(void);\n\n#define MAX_MENU_ITEMS', 1)
s += r'''

// TEMPORARY acceptance fixture. Audit reads actual game state after real returns.
EWRAM_DATA u8 gHgssFeedAudit[50] = {0};
static const s32 sCaptureStats[] = {MON_DATA_COOL, MON_DATA_BEAUTY, MON_DATA_CUTE, MON_DATA_SMART, MON_DATA_TOUGH, MON_DATA_SHEEN};
static void HgssCaptureAudit(void)
{
    static const u8 marker[] = {'H','G','S','S','F','E','E','D'};
    u32 i, j;
    memcpy(gHgssFeedAudit, marker, sizeof(marker));
    gHgssFeedAudit[8]++;
    gHgssFeedAudit[9] = 0;
    for (i = 0; i < POKEBLOCKS_COUNT; i++)
        if (gSaveBlock1Ptr->pokeblocks[i].color != PBLOCK_CLR_NONE)
            gHgssFeedAudit[9]++;
    for (i = 0; i < PARTY_SIZE; i++)
        for (j = 0; j < ARRAY_COUNT(sCaptureStats); j++)
            gHgssFeedAudit[10 + i * 6 + j] = GetMonData(&gParties[B_TRAINER_PLAYER][i], sCaptureStats[j]);
    for (i = 0; i < 4; i++)
        gHgssFeedAudit[46 + i] = gSaveBlock1Ptr->pokeblocks[i].color;
}

static void CB2_HgssCaptureCaseExit(void)
{
    OpenPokeblockCase(PBLOCK_CASE_FIELD, CB2_HgssCaptureCaseExit);
}

void CB2_HgssPokeblockUseCapture(void)
{
    static const u16 species[] = {SPECIES_PIKACHU, SPECIES_EEVEE, SPECIES_DITTO, SPECIES_TOGEPI, SPECIES_BULBASAUR, SPECIES_NIDORAN_M};
    static const u8 levels[] = {50, 35, 20, 5, 5, 100};
    static const u8 natures[] = {0, 3, 18, 0, 0, 20};
    static const u8 stats[][6] = {{0,0,0,0,0,0}, {60,100,140,180,220,80}, {250,250,250,250,250,240}, {255,255,255,255,255,255}, {255,40,80,120,10,255}, {5,5,5,5,5,0}};
    u32 i, j;
    u8 egg = TRUE;
    SetSaveBlocksPointers(0);
    ResetPokemonStorageSystem();
    ZeroPlayerPartyMons();
    for (i = 0; i < ARRAY_COUNT(species); i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];
        CreateMon(mon, species[i], levels[i], natures[i], OTID_STRUCT_PRESET(0));
        for (j = 0; j < ARRAY_COUNT(sCaptureStats); j++)
            SetMonData(mon, sCaptureStats[j], &stats[i][j]);
    }
    SetMonData(&gParties[B_TRAINER_PLAYER][5], MON_DATA_NICKNAME, COMPOUND_STRING("WWWWWWWWWWWW"));
    if (!JOY_HELD(R_BUTTON))
        SetMonData(&gParties[B_TRAINER_PLAYER][3], MON_DATA_IS_EGG, &egg);
    if (JOY_HELD(L_BUTTON))
        for (i = 1; i < PARTY_SIZE; i++)
            ZeroMonData(&gParties[B_TRAINER_PLAYER][i]);
    if (JOY_HELD(SELECT_BUTTON))
        for (i = 0; i < PARTY_SIZE; i++)
            SetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_IS_EGG, &egg);
    CalculatePlayerPartyCount();
    memset(gSaveBlock1Ptr->pokeblocks, 0, sizeof(gSaveBlock1Ptr->pokeblocks));
    gSaveBlock1Ptr->pokeblocks[0] = (struct Pokeblock){PBLOCK_CLR_WHITE, 20,30,40,50,60,40};
    gSaveBlock1Ptr->pokeblocks[1] = (struct Pokeblock){PBLOCK_CLR_RED, 60,0,0,0,0,20};
    gSaveBlock1Ptr->pokeblocks[2] = (struct Pokeblock){PBLOCK_CLR_WHITE, 0,0,0,0,0,20};
    gSaveBlock1Ptr->pokeblocks[3] = (struct Pokeblock){PBLOCK_CLR_WHITE, 20,30,40,50,60,40};
    ResetSpriteData();
    FreeAllSpritePalettes();
    HgssCaptureAudit();
    OpenPokeblockCase(PBLOCK_CASE_FIELD, CB2_HgssCaptureCaseExit);
}
'''
p.write_text(s)
print('Prepared disposable feeding fixture (five/six/single/egg-only parties, real case/feed/results, state audit)')
