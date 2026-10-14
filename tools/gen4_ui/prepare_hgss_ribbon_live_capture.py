#!/usr/bin/env python3
"""Patch ONLY a disposable Actions checkout to boot the *live* PokéNav
ribbon summary with a complete test Pokémon ribbon collection.

Neither the patched source nor the temporary .gba file is uploaded or
committed. The normal feature branch keeps its retail startup and saves.
"""
from pathlib import Path

linker = Path("ld_script_modern.ld")
original_linker = "gInitialMainCB2 = CB2_InitCopyrightScreenAfterBootup;"
new_linker = "gInitialMainCB2 = CB2_HgssRibbonLiveCapture;"
link_text = linker.read_text()
if link_text.count(original_linker) != 1:
    raise SystemExit("Refusing patch: unexpected initial callback")
linker.write_text(link_text.replace(original_linker, new_linker))

path = Path("src/pokenav.c")
text = path.read_text()
include_marker = '#include "pokenav.h"\n'
if text.count(include_marker) != 1:
    raise SystemExit("Refusing patch: missing PokéNav include")
text = text.replace(include_marker, include_marker +
                    '#include "pokemon.h"\n'
                    '#include "load_save.h"\n'
                    '#include "event_data.h"\n'
                    '#include "constants/flags.h"\n'
                    '#include "constants/species.h"\n')
# IMPORTANT: Do NOT skip the real PokéNav main menu. The original
# shortcut bypassed its app-switch, window, and palette setup and left
# an almost entirely black owner list. Exercise the normal menu instead.
assert text.count("        SetActivePokenavMenu(POKENAV_MAIN_MENU);") == 1

# The fixture is appended only to the CI checkout's pokenav.c so it can
# share the real Pokenav main menu -> ribbon list -> summary route.
text += r"""

// TEMPORARY CI SCREENSHOT FIXTURE. NOT PART OF THE COMMITTED GAME CODE.
void CB2_HgssRibbonLiveCapture(void)
{
    static const s32 sSingleRibbons[] =
    {
        MON_DATA_CHAMPION_RIBBON,
        MON_DATA_WINNING_RIBBON,
        MON_DATA_VICTORY_RIBBON,
        MON_DATA_ARTIST_RIBBON,
        MON_DATA_EFFORT_RIBBON,
        MON_DATA_MARINE_RIBBON,
        MON_DATA_LAND_RIBBON,
        MON_DATA_SKY_RIBBON,
        MON_DATA_COUNTRY_RIBBON,
        MON_DATA_NATIONAL_RIBBON,
        MON_DATA_EARTH_RIBBON,
        MON_DATA_WORLD_RIBBON,
    };
    static const s32 sContestRibbons[] =
    {
        MON_DATA_COOL_RIBBON,
        MON_DATA_BEAUTY_RIBBON,
        MON_DATA_CUTE_RIBBON,
        MON_DATA_SMART_RIBBON,
        MON_DATA_TOUGH_RIBBON,
    };
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][0];
    u8 enabled = 1;
    u8 master = 4;
    u32 i;

    ZeroPlayerPartyMons();
    CreateMon(mon, SPECIES_PIKACHU, 50, 0, OTID_STRUCT_PRESET(0));
    gPartiesCount[B_TRAINER_PLAYER] = 1;

    // 1 champion + (5 * 4 contests) + 4 single special = 25 ordinary,
    // followed by seven gift ribbons. Actual game GetMonRibbons() reads them.
    for (i = 0; i < ARRAY_COUNT(sContestRibbons); i++)
        SetMonData(mon, sContestRibbons[i], &master);
    for (i = 0; i < ARRAY_COUNT(sSingleRibbons); i++)
        SetMonData(mon, sSingleRibbons[i], &enabled);
    for (i = 0; i < 7; i++)
        gSaveBlock1Ptr->giftRibbons[i] = i + 1;

    // Activate the two real menu flags in this disposable CI memory only.
    // The full PokéNav main menu must initialize backgrounds and palettes.
    InitEventData();
    FlagSet(FLAG_ADDED_MATCH_CALL_TO_POKENAV);
    FlagSet(FLAG_SYS_RIBBON_GET);
    CB2_InitPokeNav();
}
"""
path.write_text(text)
print("Prepared disposable full-menu CI boot: 25 normal + 7 gift ribbons")
