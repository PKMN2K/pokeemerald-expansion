#!/usr/bin/env python3
"""Create a disposable live Condition-search screenshot fixture.

Run only in a capture checkout. No fixture ROM, save or patched runtime C
is committed or uploaded. All navigation follows the real PokéNav menus.
"""
from pathlib import Path


def replace_once(path, old, new):
    path = Path(path)
    source = path.read_text()
    if source.count(old) != 1:
        raise SystemExit(f"Unexpected capture hook in {path}: {old[:70]}")
    path.write_text(source.replace(old, new, 1))


replace_once("ld_script_modern.ld", "gInitialMainCB2 = CB2_InitCopyrightScreenAfterBootup;",
             "gInitialMainCB2 = CB2_HgssConditionSearchCapture;")
handler = Path("src/pokenav_menu_handler.c")
source = handler.read_text()
start = source.index("bool32 PokenavCallback_Init_MainMenuCursorOnMap(void)")
end = source.index("bool32 PokenavCallback_Init_MainMenuCursorOnMatchCall(void)", start)
block = source[start:end]
assert block.count("POKENAV_MENUITEM_MAP;") == 2
handler.write_text(source[:start] + block.replace("POKENAV_MENUITEM_MAP;", "POKENAV_MENUITEM_CONDITION;") + source[end:])

nav = Path("src/pokenav.c")
source = nav.read_text()
source = source.replace('#include "pokenav.h"\n', '#include "pokenav.h"\n'
    '#include "pokemon.h"\n#include "pokemon_storage_system.h"\n'
    '#include "string_util.h"\n#include "load_save.h"\n#include "event_data.h"\n'
    '#include "sprite.h"\n#include "palette.h"\n#include "constants/flags.h"\n'
    '#include "constants/species.h"\n', 1)
source += r'''

// TEMPORARY SCREENSHOT FIXTURE: never part of production startup or saves.
void CB2_HgssConditionSearchCapture(void)
{
    static const u16 species[] = {SPECIES_PIKACHU, SPECIES_EEVEE, SPECIES_DITTO, SPECIES_BULBASAUR};
    static const u8 levels[] = {50, 35, 20, 5};
    static const s32 dataIds[] = {MON_DATA_COOL, MON_DATA_BEAUTY, MON_DATA_CUTE, MON_DATA_SMART, MON_DATA_TOUGH};
    static const u8 conditions[][5] = {
        {255, 40, 80, 120, 10}, {255, 200, 60, 20, 80},
        {200, 255, 40, 100, 255}, {180, 180, 255, 255, 120},
    };
    u32 i, j;
    u8 value;
    SetSaveBlocksPointers(0);
    ResetPokemonStorageSystem(); // Includes EOS-terminated box names, as in a new save.
    ZeroPlayerPartyMons();
    for (i = 0; i < ARRAY_COUNT(species); i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];
        CreateMon(mon, species[i], levels[i], 0, OTID_STRUCT_PRESET(0));
        for (j = 0; j < ARRAY_COUNT(dataIds); j++)
            SetMonData(mon, dataIds[j], &conditions[i][j]);
    }
    for (i = 0; i < 20; i++)
    {
        struct BoxPokemon *mon = GetBoxedMonPtr(0, i);
        u8 stats[] = {160 - i * 5, i * 10, 120 + i * 5, 200 - i * 5, 80 + i * 5};
        CreateBoxMon(mon, SPECIES_UNOWN, 100, 0, OTID_STRUCT_PRESET(0));
        StringCopy(gStringVar1, COMPOUND_STRING("WWWWWWWWWW"));
        ConvertIntToDecimalStringN(gStringVar1 + 10, i + 1, STR_CONV_MODE_LEADING_ZEROS, 2);
        SetBoxMonData(mon, MON_DATA_NICKNAME, gStringVar1);
        for (j = 0; j < ARRAY_COUNT(dataIds); j++)
            SetBoxMonData(mon, dataIds[j], &stats[j]);
    }
    // A high-stat egg must be excluded without disturbing the tied leaders.
    CreateMon(&gParties[B_TRAINER_PLAYER][4], SPECIES_TOGEPI, 5, 0, OTID_STRUCT_PRESET(0));
    value = 255;
    for (j = 0; j < ARRAY_COUNT(dataIds); j++)
        SetMonData(&gParties[B_TRAINER_PLAYER][4], dataIds[j], &value);
    value = TRUE;
    SetMonData(&gParties[B_TRAINER_PLAYER][4], MON_DATA_IS_EGG, &value);
    gPartiesCount[B_TRAINER_PLAYER] = 5;
    InitEventData();
    FlagSet(FLAG_ADDED_MATCH_CALL_TO_POKENAV);
    FlagSet(FLAG_SYS_RIBBON_GET);
    ResetSpriteData();
    FreeAllSpritePalettes();
    CB2_InitPokeNav();
}
'''
nav.write_text(source)

# Each category is opened through its real menu. After five categories and
# a full re-entry, remove fixture data only to test short and single lists.
results = "src/pokenav_conditions_search_results.c"
replace_once(results, "bool32 PokenavCallback_Init_ConditionSearch(void)\n{", r'''
bool32 PokenavCallback_Init_ConditionSearch(void)
{
    static u32 captureEntry;
    u32 i;
    captureEntry++;
    if (captureEntry == 7)
        for (i = 0; i < 20; i++)
            ZeroBoxMonAt(0, i);
    if (captureEntry == 8)
    {
        for (i = 1; i < PARTY_SIZE; i++)
            ZeroMonData(&gParties[B_TRAINER_PLAYER][i]);
        gPartiesCount[B_TRAINER_PLAYER] = 1;
    }
'''.lstrip())
print("Prepared disposable 24-result Condition search fixture (five categories, ties, egg, short and single lists)")
