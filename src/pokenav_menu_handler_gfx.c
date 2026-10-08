#include "global.h"
#include "malloc.h"
#include "decompress.h"
#include "bg.h"
#include "palette.h"
#include "gpu_regs.h"
#include "menu.h"
#include "window.h"
#include "pokenav.h"
#include "graphics.h"
#include "sound.h"
#include "gym_leader_rematch.h"
#include "window.h"
#include "strings.h"
#include "constants/songs.h"

#define GFXTAG_HGSS_POKEGEAR_CURSOR            2
#define GFXTAG_HGSS_POKEGEAR_PHONE_STATUS      4
#define GFXTAG_HGSS_CONDITION_SEARCH_CURSOR     5

#define PALTAG_HGSS_POKEGEAR_CURSOR            2
#define PALTAG_HGSS_POKEGEAR_PHONE_STATUS      9
#define PALTAG_HGSS_CONDITION_SEARCH_CURSOR    10

#define NUM_HGSS_POKEGEAR_CURSOR_SPRITES 4


#define HGSS_POKEGEAR_APP_SWITCH_TILE_BASE          0x100
#define HGSS_POKEGEAR_APP_SWITCH_SELECTED_TILE_BASE 0x178
#define HGSS_POKEGEAR_APP_SWITCH_PAL_BANK           14
#define HGSS_POKEGEAR_APP_SWITCH_SELECTED_PAL_BANK  15
#define HGSS_POKEGEAR_APP_SWITCH_WIDTH_TILES        30
#define HGSS_POKEGEAR_APP_SWITCH_HEIGHT_TILES       4
#define HGSS_POKEGEAR_APP_BUTTON_WIDTH_TILES        6
#define HGSS_POKEGEAR_APP_CURSOR_Y                   16
#define HGSS_POKEGEAR_APP_CURSOR_X_OFFSET            16
#define HGSS_POKEGEAR_APP_CURSOR_Y_OFFSET            10

// Retail UI sprite 10 is at (197, 48) on the 256px DS PokéGear screen.
// The fixed-shell GBA adaptation keeps source columns 1..30, so subtract
// exactly one 8px source column from x while preserving y unchanged.
#define HGSS_POKEGEAR_PHONE_STATUS_X                  189
#define HGSS_POKEGEAR_PHONE_STATUS_Y                   48

#define HGSS_CONDITION_SEARCH_CHROME_TILE_BASE        0x1F0
#define HGSS_CONDITION_SEARCH_CHROME_PAL_BANK            12
#define HGSS_CONDITION_SEARCH_FONT_PAL_BANK              11
#define HGSS_CONDITION_SEARCH_PANEL_X_TILES              15
#define HGSS_CONDITION_SEARCH_PANEL_WIDTH_TILES           14
#define HGSS_CONDITION_SEARCH_WINDOW_TOP_TILES             4
#define HGSS_CONDITION_SEARCH_WINDOW_HEIGHT_TILES         14
#define HGSS_CONDITION_SEARCH_LABEL_WIDTH                 88
#define HGSS_CONDITION_SEARCH_LABEL_HEIGHT                16
#define HGSS_CONDITION_SEARCH_LABEL_BYTES \
    (HGSS_CONDITION_SEARCH_LABEL_WIDTH * HGSS_CONDITION_SEARCH_LABEL_HEIGHT / 2)
#define HGSS_CONDITION_SEARCH_CURSOR_X                   128

// Keep these in sync with make_hgss_pokegear_screen_shell.py.
#define HGSS_POKEGEAR_SCREEN_SHELL_TILE_BASE 0x40
#define HGSS_POKEGEAR_SCREEN_SHELL_PAL_BANK 13

struct Pokenav_MenuGfx
{
    bool32 (*isTaskActiveCB)(void);
    u32 loopedTaskId;
    u16 optionDescWindowId;
    u16 hgssConditionSearchWindowId;
    u8 cursorPos;
    bool8 pokenavAlreadyOpen;
    struct Sprite *hgssPhoneStatusSprite;
    struct Sprite *hgssConditionSearchCursorSprite;
    struct Sprite *hgssCursorSprites[NUM_HGSS_POKEGEAR_CURSOR_SPRITES];
    u8 bg1TilemapBuffer[BG_SCREEN_SIZE];
    u16 hgssAppSwitchTilemap[32 * HGSS_POKEGEAR_APP_SWITCH_HEIGHT_TILES];
};

static struct Pokenav_MenuGfx * OpenPokenavMenu(void);
static bool32 GetCurrentLoopedTaskActive(void);
static u32 LoopedTask_OpenMenu(s32);
static u32 LoopedTask_MoveMenuCursor(s32);
static u32 LoopedTask_OpenConditionMenu(s32);
static u32 LoopedTask_ReturnToMainMenu(s32);
static u32 LoopedTask_OpenConditionSearchMenu(s32);
static u32 LoopedTask_ReturnToConditionMenu(s32);
static u32 LoopedTask_SelectRibbonsNoWinners(s32);
static u32 LoopedTask_ReShowDescription(s32);
static u32 LoopedTask_OpenPokenavFeature(s32);
static void LoadPokenavOptionPalettes(void);
static void FreeAndDestroyMainMenuSprites(void);
static void CreateMenuOptionSprites(void);
static void DestroyMenuOptionSprites(void);
static void CreateHgssPokegearCursorSprites(void);
static void DestroyHgssPokegearCursorSprites(void);
static void SetHgssPokegearCursorVisible(bool32 visible);
static void UpdateHgssPokegearCursor(void);
static void DrawCurrentMenuOptionLabels(void);
static bool32 IsPokeGearMainMenu(void);
static void StartOptionAnimations_Enter(void);
static void StartOptionAnimations_CursorMoved(void);
static void StartOptionAnimations_Exit(void);
static void CreateHgssPokegearPhoneStatusSprite(void);
static void SpriteCB_BlinkingHgssPhoneStatus(struct Sprite *);
static void DestroyHgssPokegearPhoneStatusSprite(void);
static bool32 IsHgssConditionSearchSubmenu(void);
static void LoadHgssConditionSearchAssets(void);
static void DrawHgssConditionSearchSurface(void);
static void ClearHgssConditionSearchSurface(void);
static void UpdateHgssConditionSearchCursor(void);
static void CreateHgssConditionSearchCursorSprite(void);
static void DestroyHgssConditionSearchCursorSprite(void);
static void AddOptionDescriptionWindow(void);
static void LoadHgssPokegearScreenShell(void);
static void LoadHgssPokegearAppSwitchChrome(void);
static void UpdateHgssPokegearAppSwitchSelection(void);
static s32 GetHgssPokegearAppButtonForCurrentItem(void);
static void PrintCurrentOptionDescription(void);
static void PrintNoRibbonWinners(void);
static bool32 IsDma3ManagerBusyWithBgCopy_(void);
static void ResetBldCnt(void);

// Exact retail HGSS default-skin fixed PokéGear screen shell.
// The NCGR-derived tile sheet is compiled directly; the generated map keeps
// the retail tile/flip/palette semantics while removing only DS-only geometry.
static const u32 sHgssPokegearScreenShellTiles[] = INCGFX_U32("graphics/gen4_ui/hgss_pokegear/verified/pgear_skin0_screen_shell_tiles.png", ".4bpp");
static const u16 sHgssPokegearScreenShellPal[] = INCBIN_U16("graphics/gen4_ui/hgss_pokegear/screen_shell_gba.palette.bin");
static const u16 sHgssPokegearScreenShellTilemap[] = INCBIN_U16("graphics/gen4_ui/hgss_pokegear/screen_shell_gba.tilemap.bin");

// Exact retail HGSS default-skin PokéGear app-switch pixels, reconstructed
// losslessly from pgear_gra members 48 (NCGR), 30 (NCLR), and 54 (NSCR).
// The 256-pixel DS strip is cropped only by its empty 8-pixel side margins.
static const u16 sHgssPokegearAppSwitchPal[] = INCGFX_U16("graphics/gen4_ui/hgss_pokegear/app_switch_gba.png", ".gbapal");
static const u32 sHgssPokegearAppSwitchTiles[] = INCGFX_U32("graphics/gen4_ui/hgss_pokegear/app_switch_gba.png", ".4bpp");
static const u16 sHgssPokegearAppSwitchSelectedPal[] = INCGFX_U16("graphics/gen4_ui/hgss_pokegear/app_switch_selected_gba.png", ".gbapal");
static const u32 sHgssPokegearAppSwitchSelectedTiles[] = INCGFX_U32("graphics/gen4_ui/hgss_pokegear/app_switch_selected_gba.png", ".4bpp");

// Exact retail HGSS PokéGear cursor-corner pixels. The build generator resolves
// retail NANR sequences 4..7 -> NCER cells 20..23, whose four corners are one
// 16x16 cell plus H/V flips. The base cell keeps its member-6 pixel indices
// and member-0 palette bank 1 byte-for-byte.
static const u32 sHgssPokegearCursorCornerTiles[] = INCBIN_U32("graphics/gen4_ui/hgss_pokegear/cursor_corner.4bpp");
static const u16 sHgssPokegearCursorCornerPal[] = INCBIN_U16("graphics/gen4_ui/hgss_pokegear/cursor_corner.gbapal");

// Exact retail HGSS PokéGear phone-status frames. The generator validates
// NANR sequence 3 -> NCER cells 18/19 -> member-6 tiles 200/204 using
// member-0 palette bank 3. Both 16x16 frames are preserved byte-for-byte.
static const u32 sHgssPokegearPhoneStatusTiles[] = INCBIN_U32("graphics/gen4_ui/hgss_pokegear/phone_status.4bpp");
static const u16 sHgssPokegearPhoneStatusPal[] = INCBIN_U16("graphics/gen4_ui/hgss_pokegear/phone_status.gbapal");

static const u32 sHgssConditionSearchChromeTiles[] = INCBIN_U32("graphics/gen4_ui/hgss_pokegear/condition_search_chrome.4bpp");
static const u16 sHgssConditionSearchChromePal[] = INCBIN_U16("graphics/gen4_ui/hgss_pokegear/condition_search_chrome.gbapal");
static const ALIGNED(4) u8 sHgssConditionSearchLabels[] = INCBIN_U8("graphics/gen4_ui/hgss_pokegear/condition_search_labels.4bpp");
static const u16 sHgssConditionSearchFontPal[] = INCBIN_U16("graphics/gen4_ui/hgss_storage/verified/text_windows_hgss.gbapal");
static const u32 sHgssConditionSearchCursorTiles[] = INCBIN_U32("graphics/gen4_ui/hgss_pokegear/condition_search_cursor.4bpp");
static const u16 sHgssConditionSearchCursorPal[] = INCBIN_U16("graphics/gen4_ui/hgss_pokegear/condition_search_cursor.gbapal");

static const u8 gText_NoRibbonWinners[] = _("There are no RIBBON winners.");

// BG1 remains active for functional windows/text, but its shared base layer is
// transparent so the verified retail HGSS fixed shell on BG2 remains visible.
static const u32 sTransparentMessageBoxBgTile[8] = {0};

static const struct BgTemplate sPokenavMainMenuBgTemplates[] = {
    {
        .bg = 1,
        .charBaseIndex = 1,
        .mapBaseIndex = 15,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0x000
    }, {
        .bg = 2,
        .charBaseIndex = 2,
        .mapBaseIndex = 23,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 2,
        .baseTile = 0x000
    }, {
        .bg = 3,
        .charBaseIndex = 3,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 3,
        .baseTile = 0x000
    }
};

static const LoopedTask sMenuHandlerLoopTaskFuncs[] =
{
    [POKENAV_MENU_FUNC_NONE]                  = NULL,
    [POKENAV_MENU_FUNC_MOVE_CURSOR]           = LoopedTask_MoveMenuCursor,
    [POKENAV_MENU_FUNC_OPEN_CONDITION]        = LoopedTask_OpenConditionMenu,
    [POKENAV_MENU_FUNC_RETURN_TO_MAIN]        = LoopedTask_ReturnToMainMenu,
    [POKENAV_MENU_FUNC_OPEN_CONDITION_SEARCH] = LoopedTask_OpenConditionSearchMenu,
    [POKENAV_MENU_FUNC_RETURN_TO_CONDITION]   = LoopedTask_ReturnToConditionMenu,
    [POKENAV_MENU_FUNC_NO_RIBBON_WINNERS]     = LoopedTask_SelectRibbonsNoWinners,
    [POKENAV_MENU_FUNC_RESHOW_DESCRIPTION]    = LoopedTask_ReShowDescription,
    [POKENAV_MENU_FUNC_OPEN_FEATURE]          = LoopedTask_OpenPokenavFeature
};

static const struct SpriteSheet sHgssPokegearCursorSpriteSheet =
{
    .data = sHgssPokegearCursorCornerTiles,
    .size = sizeof(sHgssPokegearCursorCornerTiles),
    .tag = GFXTAG_HGSS_POKEGEAR_CURSOR,
};

static const struct SpritePalette sHgssPokegearCursorSpritePalette =
{
    .data = sHgssPokegearCursorCornerPal,
    .tag = PALTAG_HGSS_POKEGEAR_CURSOR,
};

static const struct SpriteSheet sHgssPokegearPhoneStatusSpriteSheet =
{
    .data = sHgssPokegearPhoneStatusTiles,
    .size = sizeof(sHgssPokegearPhoneStatusTiles),
    .tag = GFXTAG_HGSS_POKEGEAR_PHONE_STATUS,
};

static const struct SpritePalette sHgssPokegearPhoneStatusSpritePalette =
{
    .data = sHgssPokegearPhoneStatusPal,
    .tag = PALTAG_HGSS_POKEGEAR_PHONE_STATUS,
};

// Retail app-button cursor centers after the lossless GBA width adaptation.
// Source x centers 32/80/128/176 lose the removed 8px left margin; Cancel's
// source center 230 loses both removed 8px columns (0 and 25).
static const s16 sHgssPokegearAppButtonCenterX[] = {24, 72, 120, 168, 214};

static const struct WindowTemplate sOptionDescWindowTemplate =
{
    .bg = 1,
    .tilemapLeft = 3,
    .tilemapTop = 18,
    .width = 24,
    .height = 2,
    .paletteNum = HGSS_CONDITION_SEARCH_FONT_PAL_BANK,
    .baseBlock = 8
};

static const struct WindowTemplate sHgssConditionSearchWindowTemplate =
{
    .bg = 1,
    .tilemapLeft = HGSS_CONDITION_SEARCH_PANEL_X_TILES,
    .tilemapTop = HGSS_CONDITION_SEARCH_WINDOW_TOP_TILES,
    .width = HGSS_CONDITION_SEARCH_PANEL_WIDTH_TILES,
    .height = HGSS_CONDITION_SEARCH_WINDOW_HEIGHT_TILES,
    .paletteNum = HGSS_CONDITION_SEARCH_FONT_PAL_BANK,
    .baseBlock = 56
};

static const u8 *const sPageDescriptions[] =
{
    [POKENAV_MENUITEM_MAP]                     = COMPOUND_STRING("Check the map of the HOENN region"),
    [POKENAV_MENUITEM_CONDITION]               = COMPOUND_STRING("Check POKéMON in detail."),
    [POKENAV_MENUITEM_MATCH_CALL]              = COMPOUND_STRING("Call a registered TRAINER."),
    [POKENAV_MENUITEM_RIBBONS]                 = COMPOUND_STRING("Check obtained RIBBONS."),
    [POKENAV_MENUITEM_SWITCH_OFF]              = COMPOUND_STRING("Put away the POKéGEAR."),
    [POKENAV_MENUITEM_CONDITION_PARTY]         = COMPOUND_STRING("Check party POKéMON in detail."),
    [POKENAV_MENUITEM_CONDITION_SEARCH]        = COMPOUND_STRING("Check all POKéMON in detail."),
    [POKENAV_MENUITEM_CONDITION_CANCEL]        = COMPOUND_STRING("Return to the POKéGEAR menu."),
    [POKENAV_MENUITEM_CONDITION_SEARCH_COOL]   = COMPOUND_STRING("Find cool POKéMON."),
    [POKENAV_MENUITEM_CONDITION_SEARCH_BEAUTY] = COMPOUND_STRING("Find beautiful POKéMON."),
    [POKENAV_MENUITEM_CONDITION_SEARCH_CUTE]   = COMPOUND_STRING("Find cute POKéMON."),
    [POKENAV_MENUITEM_CONDITION_SEARCH_SMART]  = COMPOUND_STRING("Find smart POKéMON."),
    [POKENAV_MENUITEM_CONDITION_SEARCH_TOUGH]  = COMPOUND_STRING("Find tough POKéMON."),
    [POKENAV_MENUITEM_CONDITION_SEARCH_CANCEL] = COMPOUND_STRING("Return to the CONDITION menu.")
};

// Retail HGSS Storage font palette uses MAKE_TEXT_COLOR(1, 2, 0).
// GBA printer order is background, foreground, shadow -> {0, 1, 2}.
static const u8 sOptionDescTextColors[]  = {0, 1, 2};
static const u8 sOptionDescTextColors2[] = {0, 1, 2};

static const struct OamData sOamData_HgssPokegearCursor =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(16x16),
    .x = 0,
    .size = SPRITE_SIZE(16x16),
    .tileNum = 0,
    .priority = 2,
    .paletteNum = 0,
};

static const union AnimCmd sAnim_HgssPokegearCursor[] =
{
    ANIMCMD_FRAME(0, 1),
    ANIMCMD_END,
};

static const union AnimCmd *const sAnims_HgssPokegearCursor[] =
{
    sAnim_HgssPokegearCursor,
};

static const struct SpriteTemplate sHgssPokegearCursorSpriteTemplate =
{
    .tileTag = GFXTAG_HGSS_POKEGEAR_CURSOR,
    .paletteTag = PALTAG_HGSS_POKEGEAR_CURSOR,
    .oam = &sOamData_HgssPokegearCursor,
    .anims = sAnims_HgssPokegearCursor,
    .callback = SpriteCallbackDummy,
};

static const struct OamData sOamData_HgssPokegearPhoneStatus =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(16x16),
    .x = 0,
    .size = SPRITE_SIZE(16x16),
    .tileNum = 0,
    .priority = 2,
    .paletteNum = 0,
};

static const union AnimCmd sAnim_HgssPokegearPhoneStatus[] =
{
    // Phase 2 uses the authentic active/available frame (cell 18 / tile 200)
    // as the rematch notification. The disabled retail frame remains preserved
    // in the same sprite sheet for later phone-state use.
    ANIMCMD_FRAME(0, 1),
    ANIMCMD_END,
};

static const union AnimCmd *const sAnims_HgssPokegearPhoneStatus[] =
{
    sAnim_HgssPokegearPhoneStatus,
};

static const struct SpriteTemplate sHgssPokegearPhoneStatusSpriteTemplate =
{
    .tileTag = GFXTAG_HGSS_POKEGEAR_PHONE_STATUS,
    .paletteTag = PALTAG_HGSS_POKEGEAR_PHONE_STATUS,
    .oam = &sOamData_HgssPokegearPhoneStatus,
    .anims = sAnims_HgssPokegearPhoneStatus,
    .callback = SpriteCallbackDummy,
};

static const struct OamData sOamData_HgssConditionSearchCursor =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(8x16),
    .x = 0,
    .size = SPRITE_SIZE(8x16),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
};

static const union AnimCmd sAnim_HgssConditionSearchCursor[] =
{
    ANIMCMD_FRAME(0, 1),
    ANIMCMD_END,
};

static const union AnimCmd *const sAnims_HgssConditionSearchCursor[] =
{
    sAnim_HgssConditionSearchCursor,
};

static const struct SpriteTemplate sHgssConditionSearchCursorSpriteTemplate =
{
    .tileTag = GFXTAG_HGSS_CONDITION_SEARCH_CURSOR,
    .paletteTag = PALTAG_HGSS_CONDITION_SEARCH_CURSOR,
    .oam = &sOamData_HgssConditionSearchCursor,
    .anims = sAnims_HgssConditionSearchCursor,
    .callback = SpriteCallbackDummy,
};

static bool32 AreAnyTrainerRematchesNearby(void)
{
#if FREE_MATCH_CALL == FALSE
    s32 i;

    for (i = 0; i < REMATCH_TABLE_ENTRIES; i++)
    {
        if (GetMatchTableMapSectionId(i) == gMapHeader.regionMapSectionId
            && IsRematchEntryRegistered(i)
            && gSaveBlock1Ptr->trainerRematches[i])
            return TRUE;
    }
#endif //FREE_MATCH_CALL

    return FALSE;
}

bool32 OpenPokenavMenuInitial(void)
{
    struct Pokenav_MenuGfx *gfx = OpenPokenavMenu();

    if (gfx == NULL)
        return FALSE;

    gfx->pokenavAlreadyOpen = FALSE;
    return TRUE;
}

bool32 OpenPokenavMenuNotInitial(void)
{
    struct Pokenav_MenuGfx *gfx = OpenPokenavMenu();

    if (gfx == NULL)
        return FALSE;

    gfx->pokenavAlreadyOpen = TRUE;
    return TRUE;
}

static struct Pokenav_MenuGfx * OpenPokenavMenu(void)
{
    struct Pokenav_MenuGfx *gfx = AllocSubstruct(POKENAV_SUBSTRUCT_MENU_GFX, sizeof(struct Pokenav_MenuGfx));

    if (gfx != NULL)
    {
        gfx->loopedTaskId = CreateLoopedTask(LoopedTask_OpenMenu, 1);
        gfx->isTaskActiveCB = GetCurrentLoopedTaskActive;
    }

    return gfx;
}

void CreateMenuHandlerLoopedTask(s32 ltIdx)
{
    struct Pokenav_MenuGfx *gfx = GetSubstructPtr(POKENAV_SUBSTRUCT_MENU_GFX);
    gfx->loopedTaskId = CreateLoopedTask(sMenuHandlerLoopTaskFuncs[ltIdx], 1);
    gfx->isTaskActiveCB = GetCurrentLoopedTaskActive;
}

bool32 IsMenuHandlerLoopedTaskActive(void)
{
    struct Pokenav_MenuGfx *gfx = GetSubstructPtr(POKENAV_SUBSTRUCT_MENU_GFX);
    return gfx->isTaskActiveCB();
}

void FreeMenuHandlerSubstruct2(void)
{
    struct Pokenav_MenuGfx *gfx = GetSubstructPtr(POKENAV_SUBSTRUCT_MENU_GFX);

    RemoveWindow(gfx->optionDescWindowId);
    RemoveWindow(gfx->hgssConditionSearchWindowId);
    FreeAndDestroyMainMenuSprites();
    FreePokenavSubstruct(POKENAV_SUBSTRUCT_MENU_GFX);
}

static bool32 GetCurrentLoopedTaskActive(void)
{
    struct Pokenav_MenuGfx *gfx = GetSubstructPtr(POKENAV_SUBSTRUCT_MENU_GFX);

    return IsLoopedTaskActive(gfx->loopedTaskId);
}

static u32 LoopedTask_OpenMenu(s32 state)
{
    struct Pokenav_MenuGfx *gfx = GetSubstructPtr(POKENAV_SUBSTRUCT_MENU_GFX);

    switch (state)
    {
    case 0:
        InitBgTemplates(sPokenavMainMenuBgTemplates, ARRAY_COUNT(sPokenavMainMenuBgTemplates));
        SetBgTilemapBuffer(1, gfx->bg1TilemapBuffer);
        LoadBgTiles(1, sTransparentMessageBoxBgTile, sizeof(sTransparentMessageBoxBgTile), 0);
        FillBgTilemapBufferRect_Palette0(1, 0, 0, 0, 32, 32);
        CopyBgTilemapBufferToVram(1);
        ChangeBgX(1, 0, BG_COORD_SET);
        ChangeBgY(1, 0, BG_COORD_SET);
        ChangeBgX(2, 0, BG_COORD_SET);
        ChangeBgY(2, 0, BG_COORD_SET);
        ChangeBgX(3, 0, BG_COORD_SET);
        ChangeBgY(3, 0, BG_COORD_SET);
        return LT_INC_AND_PAUSE;
    case 1:
        if (FreeTempTileDataBuffersIfPossible())
            return LT_PAUSE;
        // Preserve the original load cadence, but the superseded PokéNav
        // device shell is gone; the authentic HGSS shell loads in state 3.
        return LT_INC_AND_PAUSE;
    case 2:
        if (FreeTempTileDataBuffersIfPossible())
            return LT_PAUSE;
        // Retail HGSS has no shared animated layer behind the core PokéGear
        // shell. Keep BG3 available for app-specific surfaces, but hidden here.
        HideBg(3);
        return LT_INC_AND_PAUSE;
    case 3:
        if (FreeTempTileDataBuffersIfPossible())
            return LT_PAUSE;
        LoadHgssPokegearScreenShell();
        LoadHgssPokegearAppSwitchChrome();
        LoadHgssConditionSearchAssets();
        AddOptionDescriptionWindow();
        return LT_INC_AND_CONTINUE;
    case 4:
        LoadPokenavOptionPalettes();
        return LT_INC_AND_CONTINUE;
    case 5:
        PrintCurrentOptionDescription();
        CreateMenuOptionSprites();
        CreateHgssPokegearPhoneStatusSprite();
        DrawCurrentMenuOptionLabels();
        return LT_INC_AND_PAUSE;
    case 6:
        if (IsDma3ManagerBusyWithBgCopy_())
            return LT_PAUSE;
        return LT_INC_AND_CONTINUE;
    case 7:
        ShowBg(1);
        ShowBg(2);
        HideBg(3);
        if (gfx->pokenavAlreadyOpen)
        {
            PokenavFadeScreen(POKENAV_FADE_FROM_BLACK);
        }
        else
        {
            PlaySE(SE_POKENAV_ON);
            PokenavFadeScreen(POKENAV_FADE_FROM_BLACK_ALL);
        }
        return LT_INC_AND_PAUSE;
    case 8:
        if (IsPaletteFadeActive())
            return LT_PAUSE;
        StartOptionAnimations_Enter();
        SetPokenavVBlankCallback();
        return LT_INC_AND_CONTINUE;
    case 9:
        break;
    }
    return LT_FINISH;
}

static u32 LoopedTask_MoveMenuCursor(s32 state)
{
    switch (state)
    {
    case 0:
        StartOptionAnimations_CursorMoved();
        PrintCurrentOptionDescription();
        PlaySE(SE_SELECT);
        return LT_INC_AND_PAUSE;
    case 1:
        if (IsDma3ManagerBusyWithBgCopy_())
            return LT_PAUSE;
        break;
    }
    return LT_FINISH;
}

static u32 LoopedTask_OpenConditionMenu(s32 state)
{
    switch (state)
    {
    case 0:
        ResetBldCnt();
        StartOptionAnimations_Exit();
        PlaySE(SE_SELECT);
        return LT_INC_AND_PAUSE;
    case 1:
        DrawCurrentMenuOptionLabels();
        return LT_INC_AND_PAUSE;
    case 2:
        StartOptionAnimations_Enter();
        PrintCurrentOptionDescription();
        return LT_INC_AND_PAUSE;
    case 3:
        if (IsDma3ManagerBusyWithBgCopy_())
            return LT_PAUSE;
        break;
    }
    return LT_FINISH;
}

static u32 LoopedTask_ReturnToMainMenu(s32 state)
{
    switch (state)
    {
    case 0:
        ResetBldCnt();
        StartOptionAnimations_Exit();
        return LT_INC_AND_PAUSE;
    case 1:
        DrawCurrentMenuOptionLabels();
        return LT_INC_AND_PAUSE;
    case 2:
        StartOptionAnimations_Enter();
        PrintCurrentOptionDescription();
        return LT_INC_AND_PAUSE;
    case 3:
        if (IsDma3ManagerBusyWithBgCopy_())
            return LT_PAUSE;
        break;
    }
    return LT_FINISH;
}

static u32 LoopedTask_OpenConditionSearchMenu(s32 state)
{
    switch (state)
    {
    case 0:
        ResetBldCnt();
        StartOptionAnimations_Exit();
        PlaySE(SE_SELECT);
        return LT_INC_AND_PAUSE;
    case 1:
        DrawCurrentMenuOptionLabels();
        return LT_INC_AND_PAUSE;
    case 2:
        StartOptionAnimations_Enter();
        PrintCurrentOptionDescription();
        return LT_INC_AND_PAUSE;
    case 3:
        break;
    }
    return LT_FINISH;
}

static u32 LoopedTask_ReturnToConditionMenu(s32 state)
{
    switch (state)
    {
    case 0:
        ResetBldCnt();
        StartOptionAnimations_Exit();
        return LT_INC_AND_PAUSE;
    case 1:
        DrawCurrentMenuOptionLabels();
        return LT_INC_AND_PAUSE;
    case 2:
        StartOptionAnimations_Enter();
        PrintCurrentOptionDescription();
        return LT_INC_AND_PAUSE;
    case 3:
        break;
    }
    return LT_FINISH;
}

static u32 LoopedTask_SelectRibbonsNoWinners(s32 state)
{
    switch (state)
    {
    case 0:
        PlaySE(SE_FAILURE);
        PrintNoRibbonWinners();
        return LT_INC_AND_PAUSE;
    case 1:
        if (IsDma3ManagerBusyWithBgCopy())
            return LT_PAUSE;
        break;
    }
    return LT_FINISH;
}

// For redisplaying the Ribbons description to replace the No Ribbon Winners message
static u32 LoopedTask_ReShowDescription(s32 state)
{
    switch (state)
    {
    case 0:
        PlaySE(SE_SELECT);
        PrintCurrentOptionDescription();
        return LT_INC_AND_PAUSE;
    case 1:
        if (IsDma3ManagerBusyWithBgCopy())
            return LT_PAUSE;
        break;
    }
    return LT_FINISH;
}

// For selecting a feature option from a menu, e.g. the Map, Match Call, Beauty search, etc.
static u32 LoopedTask_OpenPokenavFeature(s32 state)
{
    switch (state)
    {
    case 0:
        PrintHelpBarText(GetHelpBarTextId());
        return LT_INC_AND_PAUSE;
    case 1:
        if (WaitForHelpBar())
            return LT_PAUSE;
        SlideMenuHeaderUp();
        ResetBldCnt();
        StartOptionAnimations_Exit();
        switch (GetPokenavMenuType())
        {
        case POKENAV_MENU_TYPE_CONDITION_SEARCH:
            // fallthrough
        case POKENAV_MENU_TYPE_CONDITION:
            break;
        default:
            break;
        }
        PlaySE(SE_SELECT);
        return LT_INC_AND_PAUSE;
    case 2:
        PokenavFadeScreen(POKENAV_FADE_TO_BLACK);
        return LT_INC_AND_PAUSE;
    case 3:
        if (IsPaletteFadeActive())
            return LT_PAUSE;
        break;
    }
    return LT_FINISH;
}

static void LoadPokenavOptionPalettes(void)
{
    LoadSpriteSheet(&sHgssPokegearCursorSpriteSheet);
    LoadSpritePalette(&sHgssPokegearCursorSpritePalette);
    LoadSpriteSheet(&sHgssPokegearPhoneStatusSpriteSheet);
    LoadSpritePalette(&sHgssPokegearPhoneStatusSpritePalette);

    {
        const struct SpriteSheet sheet =
        {
            .data = sHgssConditionSearchCursorTiles,
            .size = sizeof(sHgssConditionSearchCursorTiles),
            .tag = GFXTAG_HGSS_CONDITION_SEARCH_CURSOR,
        };
        const struct SpritePalette palette =
        {
            .data = sHgssConditionSearchCursorPal,
            .tag = PALTAG_HGSS_CONDITION_SEARCH_CURSOR,
        };

        LoadSpriteSheet(&sheet);
        LoadSpritePalette(&palette);
    }
}

static void FreeAndDestroyMainMenuSprites(void)
{
    FreeSpriteTilesByTag(GFXTAG_HGSS_POKEGEAR_CURSOR);
    FreeSpriteTilesByTag(GFXTAG_HGSS_POKEGEAR_PHONE_STATUS);
    FreeSpriteTilesByTag(GFXTAG_HGSS_CONDITION_SEARCH_CURSOR);
    FreeSpritePaletteByTag(PALTAG_HGSS_POKEGEAR_CURSOR);
    FreeSpritePaletteByTag(PALTAG_HGSS_POKEGEAR_PHONE_STATUS);
    FreeSpritePaletteByTag(PALTAG_HGSS_CONDITION_SEARCH_CURSOR);
    DestroyMenuOptionSprites();
    DestroyHgssPokegearPhoneStatusSprite();
    DestroyHgssConditionSearchCursorSprite();
}

static void CreateMenuOptionSprites(void)
{
    CreateHgssPokegearCursorSprites();
    CreateHgssConditionSearchCursorSprite();
}

static void DestroyMenuOptionSprites(void)
{
    DestroyHgssPokegearCursorSprites();
}

static void CreateHgssPokegearCursorSprites(void)
{
    struct Pokenav_MenuGfx *gfx = GetSubstructPtr(POKENAV_SUBSTRUCT_MENU_GFX);
    s32 i;

    for (i = 0; i < NUM_HGSS_POKEGEAR_CURSOR_SPRITES; i++)
    {
        u8 spriteId = CreateSprite(&sHgssPokegearCursorSpriteTemplate, 0, 0, 2);
        gfx->hgssCursorSprites[i] = &gSprites[spriteId];
        gfx->hgssCursorSprites[i]->invisible = TRUE;
    }

    // Retail sequences 4..7 are, respectively: normal, V-flip, H-flip,
    // H+V-flip of the same 16x16 corner cell.
    SetSpriteOamFlipBits(gfx->hgssCursorSprites[0], FALSE, FALSE);
    SetSpriteOamFlipBits(gfx->hgssCursorSprites[1], FALSE, TRUE);
    SetSpriteOamFlipBits(gfx->hgssCursorSprites[2], TRUE, FALSE);
    SetSpriteOamFlipBits(gfx->hgssCursorSprites[3], TRUE, TRUE);
    UpdateHgssPokegearCursor();
}

static void DestroyHgssPokegearCursorSprites(void)
{
    struct Pokenav_MenuGfx *gfx = GetSubstructPtr(POKENAV_SUBSTRUCT_MENU_GFX);
    s32 i;

    for (i = 0; i < NUM_HGSS_POKEGEAR_CURSOR_SPRITES; i++)
        DestroySprite(gfx->hgssCursorSprites[i]);
}

static void SetHgssPokegearCursorVisible(bool32 visible)
{
    struct Pokenav_MenuGfx *gfx = GetSubstructPtr(POKENAV_SUBSTRUCT_MENU_GFX);
    s32 i;

    for (i = 0; i < NUM_HGSS_POKEGEAR_CURSOR_SPRITES; i++)
        gfx->hgssCursorSprites[i]->invisible = !visible;
}

static void UpdateHgssPokegearCursor(void)
{
    struct Pokenav_MenuGfx *gfx = GetSubstructPtr(POKENAV_SUBSTRUCT_MENU_GFX);
    s32 selectedButton;
    s32 centerX;

    if (!IsPokeGearMainMenu())
    {
        SetHgssPokegearCursorVisible(FALSE);
        return;
    }

    selectedButton = GetHgssPokegearAppButtonForCurrentItem();
    if (selectedButton < 0 || selectedButton >= ARRAY_COUNT(sHgssPokegearAppButtonCenterX))
    {
        SetHgssPokegearCursorVisible(FALSE);
        return;
    }

    centerX = sHgssPokegearAppButtonCenterX[selectedButton];

    // Retail PokegearCursorManager positions the four managed cursor sprites at
    // x +/- 16 and y +/- 10 around app-button centers. The GBA strip preserves
    // those offsets exactly; only the centers account for the two omitted DS
    // tile columns. Cursor pixels remain native 16x16 with retail flip states.
    gfx->hgssCursorSprites[0]->x = centerX - HGSS_POKEGEAR_APP_CURSOR_X_OFFSET;
    gfx->hgssCursorSprites[0]->y = HGSS_POKEGEAR_APP_CURSOR_Y - HGSS_POKEGEAR_APP_CURSOR_Y_OFFSET;
    gfx->hgssCursorSprites[1]->x = centerX - HGSS_POKEGEAR_APP_CURSOR_X_OFFSET;
    gfx->hgssCursorSprites[1]->y = HGSS_POKEGEAR_APP_CURSOR_Y + HGSS_POKEGEAR_APP_CURSOR_Y_OFFSET;
    gfx->hgssCursorSprites[2]->x = centerX + HGSS_POKEGEAR_APP_CURSOR_X_OFFSET;
    gfx->hgssCursorSprites[2]->y = HGSS_POKEGEAR_APP_CURSOR_Y - HGSS_POKEGEAR_APP_CURSOR_Y_OFFSET;
    gfx->hgssCursorSprites[3]->x = centerX + HGSS_POKEGEAR_APP_CURSOR_X_OFFSET;
    gfx->hgssCursorSprites[3]->y = HGSS_POKEGEAR_APP_CURSOR_Y + HGSS_POKEGEAR_APP_CURSOR_Y_OFFSET;

    SetHgssPokegearCursorVisible(TRUE);
}

static bool32 IsHgssConditionSearchSubmenu(void)
{
    return GetPokenavMenuType() == POKENAV_MENU_TYPE_CONDITION
        || GetPokenavMenuType() == POKENAV_MENU_TYPE_CONDITION_SEARCH;
}

static void LoadHgssConditionSearchAssets(void)
{
    LoadBgTiles(2, sHgssConditionSearchChromeTiles, sizeof(sHgssConditionSearchChromeTiles), HGSS_CONDITION_SEARCH_CHROME_TILE_BASE);
    CopyPaletteIntoBufferUnfaded(sHgssConditionSearchChromePal, BG_PLTT_ID(HGSS_CONDITION_SEARCH_CHROME_PAL_BANK), sizeof(sHgssConditionSearchChromePal));
    CopyPaletteIntoBufferUnfaded(sHgssConditionSearchFontPal, BG_PLTT_ID(HGSS_CONDITION_SEARCH_FONT_PAL_BANK), sizeof(sHgssConditionSearchFontPal));
}

static void DrawHgssConditionSearchChrome(u32 topTile, u32 bottomTile)
{
    u16 row[HGSS_CONDITION_SEARCH_PANEL_WIDTH_TILES];
    u32 y;
    u32 x;

    for (y = topTile; y <= bottomTile; y++)
    {
        bool32 isRule = y == topTile || y == bottomTile;

        for (x = 0; x < ARRAY_COUNT(row); x++)
            row[x] = (HGSS_CONDITION_SEARCH_CHROME_PAL_BANK << 12)
                | (HGSS_CONDITION_SEARCH_CHROME_TILE_BASE + (isRule ? 1 : 0));

        LoadBgTilemap(2, row, sizeof(row), y * 32 + HGSS_CONDITION_SEARCH_PANEL_X_TILES);
    }
}

static void DrawHgssConditionSearchLabels(u32 menuType)
{
    struct Pokenav_MenuGfx *gfx = GetSubstructPtr(POKENAV_SUBSTRUCT_MENU_GFX);
    static const u8 sConditionLabels[] = {0, 1, 2};
    static const u8 sSearchLabels[] = {3, 4, 5, 6, 7, 2};
    const u8 *labels;
    u32 count;
    u32 firstCenterY;
    u32 deltaY;
    u32 i;

    FillWindowPixelBuffer(gfx->hgssConditionSearchWindowId, PIXEL_FILL(0));
    PutWindowTilemap(gfx->hgssConditionSearchWindowId);

    if (menuType == POKENAV_MENU_TYPE_CONDITION)
    {
        labels = sConditionLabels;
        count = ARRAY_COUNT(sConditionLabels);
        firstCenterY = 56;
        deltaY = 20;
    }
    else
    {
        labels = sSearchLabels;
        count = ARRAY_COUNT(sSearchLabels);
        firstCenterY = 40;
        deltaY = 16;
    }

    for (i = 0; i < count; i++)
    {
        u32 destY = firstCenterY + i * deltaY
            - HGSS_CONDITION_SEARCH_WINDOW_TOP_TILES * 8
            - HGSS_CONDITION_SEARCH_LABEL_HEIGHT / 2;
        const u8 *label = sHgssConditionSearchLabels + labels[i] * HGSS_CONDITION_SEARCH_LABEL_BYTES;

        BlitBitmapToWindow(gfx->hgssConditionSearchWindowId, label, 16, destY,
                           HGSS_CONDITION_SEARCH_LABEL_WIDTH, HGSS_CONDITION_SEARCH_LABEL_HEIGHT);
    }

    CopyWindowToVram(gfx->hgssConditionSearchWindowId, COPYWIN_FULL);
}

static void DrawHgssConditionSearchSurface(void)
{
    u32 menuType = GetPokenavMenuType();

    LoadHgssPokegearScreenShell();
    LoadHgssPokegearAppSwitchChrome();

    if (menuType == POKENAV_MENU_TYPE_CONDITION)
        DrawHgssConditionSearchChrome(5, 14);
    else if (menuType == POKENAV_MENU_TYPE_CONDITION_SEARCH)
        DrawHgssConditionSearchChrome(4, 17);

    DrawHgssConditionSearchLabels(menuType);
    UpdateHgssConditionSearchCursor();
}

static void ClearHgssConditionSearchSurface(void)
{
    struct Pokenav_MenuGfx *gfx = GetSubstructPtr(POKENAV_SUBSTRUCT_MENU_GFX);

    gfx->hgssConditionSearchCursorSprite->invisible = TRUE;
    FillWindowPixelBuffer(gfx->hgssConditionSearchWindowId, PIXEL_FILL(0));
    ClearWindowTilemap(gfx->hgssConditionSearchWindowId);
    CopyWindowToVram(gfx->hgssConditionSearchWindowId, COPYWIN_FULL);
    LoadHgssPokegearScreenShell();
    LoadHgssPokegearAppSwitchChrome();
}

static void UpdateHgssConditionSearchCursor(void)
{
    struct Pokenav_MenuGfx *gfx = GetSubstructPtr(POKENAV_SUBSTRUCT_MENU_GFX);
    s32 yStart;
    s32 deltaY;

    if (!IsHgssConditionSearchSubmenu())
    {
        gfx->hgssConditionSearchCursorSprite->invisible = TRUE;
        return;
    }

    if (GetPokenavMenuType() == POKENAV_MENU_TYPE_CONDITION)
    {
        yStart = 56;
        deltaY = 20;
    }
    else
    {
        yStart = 40;
        deltaY = 16;
    }

    gfx->hgssConditionSearchCursorSprite->x = HGSS_CONDITION_SEARCH_CURSOR_X;
    gfx->hgssConditionSearchCursorSprite->y = yStart + GetPokenavCursorPos() * deltaY;
    gfx->hgssConditionSearchCursorSprite->invisible = FALSE;
}

static void CreateHgssConditionSearchCursorSprite(void)
{
    struct Pokenav_MenuGfx *gfx = GetSubstructPtr(POKENAV_SUBSTRUCT_MENU_GFX);
    u8 spriteId = CreateSprite(&sHgssConditionSearchCursorSpriteTemplate, HGSS_CONDITION_SEARCH_CURSOR_X, 0, 1);

    gfx->hgssConditionSearchCursorSprite = &gSprites[spriteId];
    gfx->hgssConditionSearchCursorSprite->invisible = TRUE;
}

static void DestroyHgssConditionSearchCursorSprite(void)
{
    struct Pokenav_MenuGfx *gfx = GetSubstructPtr(POKENAV_SUBSTRUCT_MENU_GFX);
    DestroySprite(gfx->hgssConditionSearchCursorSprite);
}

static void LoadHgssPokegearScreenShell(void)
{
    // Phase 2 deliberately leaves the legacy PokéNav shell load in state 1.
    // This verified retail HGSS layer replaces it visually for validation;
    // the legacy code/assets are not removed until the next gated phase.
    LoadBgTiles(
        2,
        sHgssPokegearScreenShellTiles,
        sizeof(sHgssPokegearScreenShellTiles),
        HGSS_POKEGEAR_SCREEN_SHELL_TILE_BASE);
    CopyPaletteIntoBufferUnfaded(
        sHgssPokegearScreenShellPal,
        BG_PLTT_ID(HGSS_POKEGEAR_SCREEN_SHELL_PAL_BANK),
        sizeof(sHgssPokegearScreenShellPal));
    LoadBgTilemap(
        2,
        sHgssPokegearScreenShellTilemap,
        sizeof(sHgssPokegearScreenShellTilemap),
        0);
}

static void LoadHgssPokegearAppSwitchChrome(void)
{
    // Member 54 contains both normal (rows 0..3) and selected (rows 4..7)
    // button states. Both are reconstructed from exact member-48 pixels.
    LoadBgTiles(
        2,
        sHgssPokegearAppSwitchTiles,
        sizeof(sHgssPokegearAppSwitchTiles),
        HGSS_POKEGEAR_APP_SWITCH_TILE_BASE);
    LoadBgTiles(
        2,
        sHgssPokegearAppSwitchSelectedTiles,
        sizeof(sHgssPokegearAppSwitchSelectedTiles),
        HGSS_POKEGEAR_APP_SWITCH_SELECTED_TILE_BASE);
    CopyPaletteIntoBufferUnfaded(
        sHgssPokegearAppSwitchPal,
        BG_PLTT_ID(HGSS_POKEGEAR_APP_SWITCH_PAL_BANK),
        sizeof(sHgssPokegearAppSwitchPal));
    CopyPaletteIntoBufferUnfaded(
        sHgssPokegearAppSwitchSelectedPal,
        BG_PLTT_ID(HGSS_POKEGEAR_APP_SWITCH_SELECTED_PAL_BANK),
        sizeof(sHgssPokegearAppSwitchSelectedPal));

    UpdateHgssPokegearAppSwitchSelection();
}

static s32 GetHgssPokegearAppButtonForCurrentItem(void)
{
    // Preserve the five retail HGSS button slots. Three are direct functional
    // matches; the remaining two reuse authentic slots without redrawing them.
    switch (GetCurrentMenuItemId())
    {
    case POKENAV_MENUITEM_CONDITION:
        return 0; // Configure slot
    case POKENAV_MENUITEM_RIBBONS:
        return 1; // Radio slot
    case POKENAV_MENUITEM_MAP:
        return 2; // Map slot
    case POKENAV_MENUITEM_MATCH_CALL:
        return 3; // Phone slot
    case POKENAV_MENUITEM_SWITCH_OFF:
        return 4; // Cancel slot
    default:
        return -1;
    }
}

static void UpdateHgssPokegearAppSwitchSelection(void)
{
    struct Pokenav_MenuGfx *gfx = GetSubstructPtr(POKENAV_SUBSTRUCT_MENU_GFX);
    s32 selectedButton = IsPokeGearMainMenu()
        ? GetHgssPokegearAppButtonForCurrentItem()
        : -1;
    u32 x;
    u32 y;

    // Start from the authentic normal-state strip every time so moving the
    // cursor restores the previously selected button losslessly.
    for (y = 0; y < HGSS_POKEGEAR_APP_SWITCH_HEIGHT_TILES; y++)
    {
        for (x = 0; x < 32; x++)
            gfx->hgssAppSwitchTilemap[y * 32 + x] = 0;

        for (x = 0; x < HGSS_POKEGEAR_APP_SWITCH_WIDTH_TILES; x++)
        {
            gfx->hgssAppSwitchTilemap[y * 32 + x]
                = (HGSS_POKEGEAR_APP_SWITCH_PAL_BANK << 12)
                | (HGSS_POKEGEAR_APP_SWITCH_TILE_BASE
                   + y * HGSS_POKEGEAR_APP_SWITCH_WIDTH_TILES
                   + x);
        }
    }

    if (selectedButton >= 0)
    {
        u32 buttonStart = selectedButton * HGSS_POKEGEAR_APP_BUTTON_WIDTH_TILES;

        for (y = 0; y < HGSS_POKEGEAR_APP_SWITCH_HEIGHT_TILES; y++)
        {
            for (x = 0; x < HGSS_POKEGEAR_APP_BUTTON_WIDTH_TILES; x++)
            {
                u32 dstX = buttonStart + x;
                gfx->hgssAppSwitchTilemap[y * 32 + dstX]
                    = (HGSS_POKEGEAR_APP_SWITCH_SELECTED_PAL_BANK << 12)
                    | (HGSS_POKEGEAR_APP_SWITCH_SELECTED_TILE_BASE
                       + y * HGSS_POKEGEAR_APP_SWITCH_WIDTH_TILES
                       + dstX);
            }
        }
    }

    LoadBgTilemap(
        2,
        gfx->hgssAppSwitchTilemap,
        sizeof(gfx->hgssAppSwitchTilemap),
        0);
}

static void DrawCurrentMenuOptionLabels(void)
{
    if (IsHgssConditionSearchSubmenu())
        DrawHgssConditionSearchSurface();
    else
        ClearHgssConditionSearchSurface();
}

static bool32 IsPokeGearMainMenu(void)
{
    switch (GetPokenavMenuType())
    {
    case POKENAV_MENU_TYPE_DEFAULT:
    case POKENAV_MENU_TYPE_UNLOCK_MC:
    case POKENAV_MENU_TYPE_UNLOCK_MC_RIBBONS:
        return TRUE;
    default:
        return FALSE;
    }
}

static void StartOptionAnimations_Enter(void)
{
    struct Pokenav_MenuGfx *gfx = GetSubstructPtr(POKENAV_SUBSTRUCT_MENU_GFX);

    gfx->cursorPos = GetPokenavCursorPos();
    UpdateHgssPokegearAppSwitchSelection();

    if (IsPokeGearMainMenu())
        UpdateHgssPokegearCursor();
    else
        UpdateHgssConditionSearchCursor();
}

static void StartOptionAnimations_CursorMoved(void)
{
    struct Pokenav_MenuGfx *gfx = GetSubstructPtr(POKENAV_SUBSTRUCT_MENU_GFX);

    gfx->cursorPos = GetPokenavCursorPos();
    UpdateHgssPokegearAppSwitchSelection();

    if (IsPokeGearMainMenu())
        UpdateHgssPokegearCursor();
    else
        UpdateHgssConditionSearchCursor();
}

static void StartOptionAnimations_Exit(void)
{
    struct Pokenav_MenuGfx *gfx = GetSubstructPtr(POKENAV_SUBSTRUCT_MENU_GFX);

    SetHgssPokegearCursorVisible(FALSE);
    gfx->hgssConditionSearchCursorSprite->invisible = TRUE;
}


static void CreateHgssPokegearPhoneStatusSprite(void)
{
    struct Pokenav_MenuGfx *gfx = GetSubstructPtr(POKENAV_SUBSTRUCT_MENU_GFX);
    u8 spriteId = CreateSprite(
        &sHgssPokegearPhoneStatusSpriteTemplate,
        HGSS_POKEGEAR_PHONE_STATUS_X,
        HGSS_POKEGEAR_PHONE_STATUS_Y,
        4);

    gfx->hgssPhoneStatusSprite = &gSprites[spriteId];
    gfx->hgssPhoneStatusSprite->data[1] = AreAnyTrainerRematchesNearby();
    gfx->hgssPhoneStatusSprite->data[2] = TRUE;
    gfx->hgssPhoneStatusSprite->callback = SpriteCB_BlinkingHgssPhoneStatus;
    gfx->hgssPhoneStatusSprite->invisible = !gfx->hgssPhoneStatusSprite->data[1];
}

static void DestroyHgssPokegearPhoneStatusSprite(void)
{
    struct Pokenav_MenuGfx *gfx = GetSubstructPtr(POKENAV_SUBSTRUCT_MENU_GFX);
    DestroySprite(gfx->hgssPhoneStatusSprite);
}

static void SpriteCB_BlinkingHgssPhoneStatus(struct Sprite *sprite)
{
    if (!sprite->data[1] || !IsPokeGearMainMenu())
    {
        sprite->data[0] = 0;
        sprite->data[2] = FALSE;
        sprite->invisible = TRUE;
        return;
    }

    // Returning from a submenu restores the authentic indicator immediately,
    // then resumes the original rematch-notification blink cadence.
    if (!sprite->data[2])
    {
        sprite->data[2] = TRUE;
        sprite->invisible = FALSE;
        return;
    }

    sprite->data[0]++;
    if (sprite->data[0] > 8)
    {
        sprite->data[0] = 0;
        sprite->invisible ^= 1;
    }
}

static void AddOptionDescriptionWindow(void)
{
    struct Pokenav_MenuGfx *gfx = GetSubstructPtr(POKENAV_SUBSTRUCT_MENU_GFX);

    gfx->optionDescWindowId = AddWindow(&sOptionDescWindowTemplate);
    gfx->hgssConditionSearchWindowId = AddWindow(&sHgssConditionSearchWindowTemplate);
    PutWindowTilemap(gfx->optionDescWindowId);
    ClearWindowTilemap(gfx->hgssConditionSearchWindowId);
    FillWindowPixelBuffer(gfx->hgssConditionSearchWindowId, PIXEL_FILL(0));

    // Do not draw a fabricated HGSS-style panel here. A transparent window
    // leaves the verified retail HGSS fixed shell visible underneath.
    FillWindowPixelBuffer(gfx->optionDescWindowId, PIXEL_FILL(0));
    CopyWindowToVram(gfx->optionDescWindowId, COPYWIN_FULL);
}

static void PrintCurrentOptionDescription(void)
{
    struct Pokenav_MenuGfx *gfx = GetSubstructPtr(POKENAV_SUBSTRUCT_MENU_GFX);
    int menuItem = GetCurrentMenuItemId();
    const u8 *desc = sPageDescriptions[menuItem];
    u32 windowWidth = GetWindowAttribute(gfx->optionDescWindowId, WINDOW_WIDTH) * 8;
    u32 width = GetStringWidth(FONT_NORMAL, desc, -1);

    FillWindowPixelBuffer(gfx->optionDescWindowId, PIXEL_FILL(0));
    AddTextPrinterParameterized3(gfx->optionDescWindowId, FONT_NORMAL, (windowWidth - width) / 2, 1, sOptionDescTextColors, 0, desc);
    CopyWindowToVram(gfx->optionDescWindowId, COPYWIN_GFX);
}

// Printed when Ribbons is selected if no PC/party mons have ribbons
// Can occur by obtaining a mon with a ribbon and then releasing all ribbon winners
static void PrintNoRibbonWinners(void)
{
    struct Pokenav_MenuGfx *gfx = GetSubstructPtr(POKENAV_SUBSTRUCT_MENU_GFX);
    const u8 *s = gText_NoRibbonWinners;
    u32 windowWidth = GetWindowAttribute(gfx->optionDescWindowId, WINDOW_WIDTH) * 8;
    u32 width = GetStringWidth(FONT_NORMAL, s, -1);

    FillWindowPixelBuffer(gfx->optionDescWindowId, PIXEL_FILL(0));
    AddTextPrinterParameterized3(gfx->optionDescWindowId, FONT_NORMAL, (windowWidth - width) / 2, 1, sOptionDescTextColors2, 0, s);
    CopyWindowToVram(gfx->optionDescWindowId, COPYWIN_GFX);
}

static bool32 IsDma3ManagerBusyWithBgCopy_(void)
{
    return IsDma3ManagerBusyWithBgCopy();
}

static void ResetBldCnt(void)
{
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
}

void ResetBldCnt_(void)
{
    ResetBldCnt();
}
