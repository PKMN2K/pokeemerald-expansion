#ifndef GUARD_POKEDEX_PLUS_HGSS_H
#define GUARD_POKEDEX_PLUS_HGSS_H

#define GEN4_UI_HGSS_SEARCH_SELECTED_PAL_SLOT 4

extern const u16 sPokedexPlusHGSS_Gen4SearchOverlayTilemap[];

void InitInfoScreenWindows_HGSS(void);
bool8 LoadPokedexListPage_HGSS(u8 page);
void Task_LoadInfoScreen_HGSS(u8 taskId);
void HandleInfoScreenInput_HGSS(u8 taskId);
void Task_LoadAreaScreen_HGSS(u8 taskId);
void Task_SwitchScreensFromAreaScreen_HGSS(u8 taskId);
void Task_DisplayCaughtMonDexPage(u8 taskId);
void Task_LoadCryScreen_HGSS(u8 taskId);
void Task_SwitchScreensFromCryScreen_HGSS(u8 taskId);
void Task_LoadSizeScreen_HGSS(u8 taskId);
void LoadPlayArrowPalette_HGSS(bool8 cryPlaying);
void LoadSearchMenu_HGSS(u8 taskId);
void HandleDestroyStatBars_HGSS(void);
void HandleDestroyStatBarsBg_HGSS(void);
void HandleCreateStatBars_HGSS(void);
void HandleCreateStatBarsDPAD_HGSS(void);
void HideCaughtMonPageTypeIcons(void);

#endif // GUARD_POKEDEX_PLUS_HGSS_H
