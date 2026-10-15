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
                    '#include "sprite.h"\n'
                    '#include "palette.h"\n'
                    '#include "constants/flags.h"\n'
                    '#include "constants/species.h"\n')
# IMPORTANT: Do NOT skip the real PokéNav main menu. The original
# shortcut bypassed its app-switch, window, and palette setup and left
# an almost entirely black owner list. Exercise the normal menu instead.
assert text.count("        SetActivePokenavMenu(POKENAV_MAIN_MENU);") == 1

# Use the NORMAL initial PokéNav graphics path while moving its start cursor
# to Ribbons. The previous fixture's Down presses accidentally entered the
# region map and every subsequent screenshot captured the wrong application.
handler = Path("src/pokenav_menu_handler.c")
handler_text = handler.read_text()
start = handler_text.index("bool32 PokenavCallback_Init_MainMenuCursorOnMap(void)")
end = handler_text.index("bool32 PokenavCallback_Init_MainMenuCursorOnMatchCall(void)", start)
block = handler_text[start:end]
for old, new in (
    ("menu->cursorPos = POKENAV_MENUITEM_MAP;", "menu->cursorPos = POKENAV_MENUITEM_RIBBONS;"),
    ("menu->currMenuItem = POKENAV_MENUITEM_MAP;", "menu->currMenuItem = POKENAV_MENUITEM_RIBBONS;"),
):
    if block.count(old) != 1:
        raise SystemExit("Unexpected initial menu cursor setup")
    block = block.replace(old, new)
handler.write_text(handler_text[:start] + block + handler_text[end:])

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
    // A cold boot has not passed through the field menu, which normally
    // resets sprite/OAM state before opening the PokéNav. Match the real
    // PokéNav tutorial entry path for this isolated capture fixture.
    ResetSpriteData();
    FreeAllSpritePalettes();
    CB2_InitPokeNav();
}
"""
path.write_text(text)
# Inject mGBA's documented debug-register logging only into the disposable
# CI ROM. Logs are emitted through the screenshot utility's stderr logger.
nav = Path("src/pokenav.c")
nav_text = nav.read_text()
def patch_once(old, new):
    global nav_text
    if nav_text.count(old) != 1:
        raise SystemExit(f"Unexpected PokéNav diagnostic hook: {old[:70]}")
    nav_text = nav_text.replace(old, new, 1)

logger = r"""
// Disposable CI-only mGBA console tracer; no production source is changed.
static void CaptureLog(const char *msg)
{
    volatile u16 *enable = (volatile u16 *)0x04FFF780;
    volatile u16 *flags = (volatile u16 *)0x04FFF700;
    volatile char *buffer = (volatile char *)0x04FFF600;
    u32 i;
    *enable = 0xC0DE;
    if (*enable != 0x1DEA)
        return;
    for (i = 0; msg[i] && i < 254; i++)
        buffer[i] = msg[i];
    buffer[i] = 0;
    *flags = 0x103; // mGBA INFO, trigger logging
}
"""
patch_once("static void CB2_Pokenav(void)\n{",
           logger + "\nstatic void CB2_Pokenav(void)\n{")
patch_once("        if (menuId == POKENAV_MENU_FUNC_EXIT)",
           '        if (menuId == POKENAV_RIBBONS_MON_LIST) CaptureLog("RIBBON_STAGE: dispatch requested");\n'
           "        if (menuId == POKENAV_MENU_FUNC_EXIT)")
patch_once("    if (!PokenavMenuCallbacks[index].init())",
           '    if (menuId == POKENAV_RIBBONS_MON_LIST) CaptureLog("RIBBON_STAGE: before init");\n'
           "    if (!PokenavMenuCallbacks[index].init())")
patch_once("    if (!PokenavMenuCallbacks[index].open())",
           '    if (menuId == POKENAV_RIBBONS_MON_LIST) CaptureLog("RIBBON_STAGE: before open");\n'
           "    if (!PokenavMenuCallbacks[index].open())")
patch_once("    SetActiveMenuLoopTasks(PokenavMenuCallbacks[index].createLoopTask",
           '    if (menuId == POKENAV_RIBBONS_MON_LIST) CaptureLog("RIBBON_STAGE: open succeeded");\n'
           "    SetActiveMenuLoopTasks(PokenavMenuCallbacks[index].createLoopTask")
nav.write_text(nav_text)

print("Prepared disposable full-menu CI boot: initial Ribbons cursor, 25 normal + 7 gift ribbons")
