#ifndef GUARD_POKEDEX_PLUS_HGSS_H
#define GUARD_POKEDEX_PLUS_HGSS_H

void InitInfoScreenWindows_HGSS(void);
bool8 LoadPokedexListPage_HGSS(u8 page);
bool32 TryMoveMonForInfoScreen_HGSS(struct Sprite *sprite);
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
bool32 TryDrawOrEraseSearchParameterBox_HGSS(bool8 erase);
void HandleDestroyStatBars_HGSS(void);
void HandleDestroyStatBarsBg_HGSS(void);
void HandleCreateStatBars_HGSS(void);
void HandleCreateStatBarsDPAD_HGSS(void);
void HideCaughtMonPageTypeIcons(void);

#endif // GUARD_POKEDEX_PLUS_HGSS_H
