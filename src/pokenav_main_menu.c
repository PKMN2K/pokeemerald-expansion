#include "global.h"
#include "pokenav.h"
#include "constants/songs.h"
#include "sound.h"
#include "constants/rgb.h"
#include "palette.h"
#include "bg.h"
#include "window.h"
#include "strings.h"
#include "graphics.h"
#include "decompress.h"
#include "gpu_regs.h"
#include "menu.h"
#include "dma3.h"

struct Pokenav_MainMenu
{
    void (*loopTask)(u32);
    u32 (*isLoopTaskActiveFunc)(void);
    u32 currentTaskId;
    u32 helpBarWindowId;
    u32 palettes;
    ALIGNED(4) u8 tilemapBuffer[BG_SCREEN_SIZE];
};

static void CleanupPokenavMainMenuResources(void);
static void InitPokenavMainMenuResources(void);
static void InitHelpBar(void);
static u32 LoopedTask_SlideMenuHeaderUp(s32);
static u32 LoopedTask_SlideMenuHeaderDown(s32);
static void DrawHgssPokegearHelpBar(u32);
static void LoadHgssPokegearHelpBarStrip(void);
static void LoadHgssPokegearHelpBarPalette(void);
static u32 LoopedTask_InitPokenavMenu(s32);

// Exact retail HGSS PokéGear Phone tooltip pixels/colors. The GBA tile only
// relocates source palette index 1 into an unused BG0 palette slot.
static const u32 sHgssHelpBarTooltip_Gfx[] = INCBIN_U32("graphics/gen4_ui/hgss_pokegear/help_bar_tooltip_gba.4bpp");
static const u16 sHgssHelpBarTooltip_Pal[] = INCBIN_U16("graphics/gen4_ui/hgss_pokegear/help_bar_tooltip.gbapal");

// BG0 no longer carries Emerald PokéNav header artwork. Keep tile 0 explicitly
// transparent so BG1/BG0 cannot obscure the authentic HGSS app-switch on BG2.
static const u32 sTransparentBgTile[8] = {0};

const struct BgTemplate gPokenavMainMenuBgTemplates[] =
{
    {
        .bg = 0,
        .charBaseIndex = 0,
        .mapBaseIndex = 5,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0,
    }
};

// Retail HGSS uses a 32x4 tooltip strip at y=20 with a 32x2 text window at
// y=21. The GBA keeps that vertical geometry and omits only two DS columns.
static const struct WindowTemplate sHgssHelpBarWindowTemplate[] =
{
    {
        .bg = 0,
        .tilemapLeft = 0,
        .tilemapTop = 21,
        .width = 30,
        .height = 2,
        .paletteNum = 0,
        .baseBlock = 0x36,
    },
    DUMMY_WIN_TEMPLATE
};

static const u8 *const sHelpBarTexts[HELPBAR_COUNT] =
{
    [HELPBAR_NONE]                  = COMPOUND_STRING("{CLEAR 0x80}"),
    [HELPBAR_MAP_ZOOMED_OUT]        = COMPOUND_STRING("{A_BUTTON}ZOOM {B_BUTTON}CANCEL"),
    [HELPBAR_MAP_ZOOMED_IN]         = COMPOUND_STRING("{A_BUTTON}FULL {B_BUTTON}CANCEL"),
    [HELPBAR_MAP_ZOOMED_OUT_CANFLY] = COMPOUND_STRING("{A_BUTTON}ZOOM {B_BUTTON}CANCEL {R_BUTTON}FLY"),
    [HELPBAR_MAP_ZOOMED_IN_CANFLY]  = COMPOUND_STRING("{A_BUTTON}FULL {B_BUTTON}CANCEL {R_BUTTON}FLY"),
    [HELPBAR_CONDITION_MON_LIST]    = COMPOUND_STRING("{A_BUTTON}VIEW {B_BUTTON}BACK"),
    [HELPBAR_CONDITION_MON_STATUS]  = COMPOUND_STRING("{A_BUTTON}MARK {B_BUTTON}BACK"),
    [HELPBAR_CONDITION_MARKINGS]    = COMPOUND_STRING("{A_BUTTON}SET {B_BUTTON}BACK"),
    [HELPBAR_MC_TRAINER_LIST]       = COMPOUND_STRING("{A_BUTTON}OPTIONS {B_BUTTON}BACK"),
    [HELPBAR_MC_CALL_MENU]          = COMPOUND_STRING("{A_BUTTON}SELECT {B_BUTTON}BACK"),
    [HELPBAR_MC_CHECK_PAGE]         = COMPOUND_STRING("{B_BUTTON}BACK"),
    [HELPBAR_RIBBONS_MON_LIST]      = COMPOUND_STRING("{A_BUTTON}VIEW {B_BUTTON}BACK"),
    [HELPBAR_RIBBONS_LIST]          = COMPOUND_STRING("{A_BUTTON}DETAILS {B_BUTTON}BACK"),
    [HELPBAR_RIBBONS_CHECK]         = COMPOUND_STRING("D-PAD BROWSE {B_BUTTON}BACK"),
};

enum
{
    HGSS_HELP_BAR_TILE = 0x35,
    HGSS_HELP_BAR_STRIP_TOP = 20,
    HGSS_HELP_BAR_STRIP_WIDTH = 30,
    HGSS_HELP_BAR_STRIP_HEIGHT = 4,
    HGSS_HELP_BAR_STRIP_COLOR = 7,
    HGSS_HELP_BAR_SHADOW_COLOR = 8,
    HGSS_HELP_BAR_TEXT_COLOR = 9,
    HGSS_HELP_BAR_FILL_COLOR = 10,
};

// HGSS MAKE_TEXT_COLOR(3, 2, 5) -> GBA background/foreground/shadow.
static const u8 sHgssHelpBarTextColors[3] =
{
    HGSS_HELP_BAR_FILL_COLOR,
    HGSS_HELP_BAR_TEXT_COLOR,
    HGSS_HELP_BAR_SHADOW_COLOR,
};

bool32 InitPokenavMainMenu(void)
{
    struct Pokenav_MainMenu *menu;

    menu = AllocSubstruct(POKENAV_SUBSTRUCT_MAIN_MENU, sizeof(struct Pokenav_MainMenu));
    if (menu == NULL)
        return FALSE;

    ResetSpriteData();
    FreeAllSpritePalettes();
    menu->currentTaskId = CreateLoopedTask(LoopedTask_InitPokenavMenu, 1);
    return TRUE;
}

u32 PokenavMainMenuLoopedTaskIsActive(void)
{
    struct Pokenav_MainMenu *menu = GetSubstructPtr(POKENAV_SUBSTRUCT_MAIN_MENU);
    return IsLoopedTaskActive(menu->currentTaskId);
}

void ShutdownPokenav(void)
{
    PlaySE(SE_POKENAV_OFF);
    ResetBldCnt_();
    BeginNormalPaletteFade(PALETTES_ALL, -1, 0, 16, RGB_BLACK);
}

bool32 WaitForPokenavShutdownFade(void)
{
    if (!gPaletteFade.active)
    {
        FreeMenuHandlerSubstruct2();
        CleanupPokenavMainMenuResources();
        FreeAllWindowBuffers();
        return FALSE;
    }

    return TRUE;
}

static u32 LoopedTask_InitPokenavMenu(s32 state)
{
    struct Pokenav_MainMenu *menu;

    switch (state)
    {
    case 0:
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
        FreeAllWindowBuffers();
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, gPokenavMainMenuBgTemplates, ARRAY_COUNT(gPokenavMainMenuBgTemplates));
        ResetBgPositions();
        ResetTempTileDataBuffers();
        return LT_INC_AND_CONTINUE;
    case 1:
        menu = GetSubstructPtr(POKENAV_SUBSTRUCT_MAIN_MENU);
        SetBgTilemapBuffer(0, menu->tilemapBuffer);
        LoadBgTiles(0, sTransparentBgTile, sizeof(sTransparentBgTile), 0);
        FillBgTilemapBufferRect_Palette0(0, 0, 0, 0, 32, 32);
        CopyBgTilemapBufferToVram(0);
        return LT_INC_AND_PAUSE;
    case 2:
        if (FreeTempTileDataBuffersIfPossible())
            return LT_PAUSE;

        InitHelpBar();
        return LT_INC_AND_PAUSE;
    case 3:
        if (IsDma3ManagerBusyWithBgCopy())
            return LT_PAUSE;

        InitPokenavMainMenuResources();
        ShowBg(0);
        return LT_FINISH;
    default:
        return LT_FINISH;
    }
}


void SetActiveMenuLoopTasks(void *createLoopTask, void *isLoopTaskActive) // Fix types later.
{
    struct Pokenav_MainMenu *menu = GetSubstructPtr(POKENAV_SUBSTRUCT_MAIN_MENU);
    menu->loopTask = createLoopTask;
    menu->isLoopTaskActiveFunc = isLoopTaskActive;
}

void RunMainMenuLoopedTask(u32 state)
{
    struct Pokenav_MainMenu *menu = GetSubstructPtr(POKENAV_SUBSTRUCT_MAIN_MENU);
    menu->loopTask(state);
}

u32 IsActiveMenuLoopTaskActive(void)
{
    struct Pokenav_MainMenu *menu = GetSubstructPtr(POKENAV_SUBSTRUCT_MAIN_MENU);
    return menu->isLoopTaskActiveFunc();
}

void SlideMenuHeaderUp(void)
{
    struct Pokenav_MainMenu *menu = GetSubstructPtr(POKENAV_SUBSTRUCT_MAIN_MENU);
    menu->currentTaskId = CreateLoopedTask(LoopedTask_SlideMenuHeaderUp, 4);
}

void SlideMenuHeaderDown(void)
{
    struct Pokenav_MainMenu *menu = GetSubstructPtr(POKENAV_SUBSTRUCT_MAIN_MENU);
    menu->currentTaskId = CreateLoopedTask(LoopedTask_SlideMenuHeaderDown, 4);
}

bool32 MainMenuLoopedTaskIsBusy(void)
{
    struct Pokenav_MainMenu *menu = GetSubstructPtr(POKENAV_SUBSTRUCT_MAIN_MENU);
    return IsLoopedTaskActive(menu->currentTaskId);
}

static u32 LoopedTask_SlideMenuHeaderUp(s32 state)
{
    switch (state)
    {
    default:
        return LT_FINISH;
    case 1:
        return LT_INC_AND_PAUSE;
    case 0:
        return LT_INC_AND_PAUSE;
    case 2:
        if (ChangeBgY(0, 384, BG_COORD_ADD) >= 0x2000u)
        {
            ChangeBgY(0, 0x2000, BG_COORD_SET);
            return LT_FINISH;
        }

        return LT_PAUSE;
    }
}

static u32 LoopedTask_SlideMenuHeaderDown(s32 state)
{
    if (ChangeBgY(0, 384, BG_COORD_SUB) <= 0)
    {
        ChangeBgY(0, 0, BG_COORD_SET);
        return LT_FINISH;
    }
    return LT_PAUSE;
}

void CopyPaletteIntoBufferUnfaded(const u16 *palette, u32 bufferOffset, u32 size)
{
    CpuCopy16(palette, &gPlttBufferUnfaded[bufferOffset], size);
}

void Pokenav_AllocAndLoadPalettes(const struct SpritePalette *palettes)
{
    const struct SpritePalette *current;
    u32 index;

    for (current = palettes; current->data != NULL; current++)
    {
        index = AllocSpritePalette(current->tag);
        if (index == 0xFF)
        {
            break;
        }
        else
        {
            index = OBJ_PLTT_ID(index);
            CopyPaletteIntoBufferUnfaded(current->data, index, PLTT_SIZE_4BPP);
        }
    }
}

void PokenavFillPalette(u32 palIndex, u16 fillValue)
{
    CpuFill16(fillValue, &gPlttBufferFaded[OBJ_PLTT_ID(palIndex)], PLTT_SIZE_4BPP);
}

void PokenavCopyPalette(const u16 *src, const u16 *dest, int size, int a3, int a4, u16 *palette)
{
    if (a4 == 0)
    {
        CpuCopy16(src, palette, size * 2);
    }
    else if (a4 >= a3)
    {
        CpuCopy16(dest, palette, size * 2);
    }
    else
    {
        int r, g, b;
        int r1, g1, b1;
        while (size--)
        {
            r = GET_R(*src);
            g = GET_G(*src);
            b = GET_B(*src);

            r1 = ((((GET_R(*dest) << 8) - (r << 8)) / a3) * a4) >> 8;
            g1 = ((((GET_G(*dest) << 8) - (g << 8)) / a3) * a4) >> 8;
            b1 = ((((GET_B(*dest) << 8) - (b << 8)) / a3) * a4) >> 8;

            r = (r + r1) & 0x1F; //_RGB(r + r1, g + g1, b + b1); doesn't match
            g = (g + g1) & 0x1F;
            b = (b + b1) & 0x1F;

            *palette = RGB2(r, g, b);

            src++, dest++;
            palette++;
        }
    }
}

void PokenavFadeScreen(s32 fadeType)
{
    struct Pokenav_MainMenu *menu = GetSubstructPtr(POKENAV_SUBSTRUCT_MAIN_MENU);

    switch (fadeType)
    {
    case POKENAV_FADE_TO_BLACK:
        BeginNormalPaletteFade(menu->palettes, -2, 0, 16, RGB_BLACK);
        break;
    case POKENAV_FADE_FROM_BLACK:
        BeginNormalPaletteFade(menu->palettes, -2, 16, 0, RGB_BLACK);
        break;
    case POKENAV_FADE_TO_BLACK_ALL:
        BeginNormalPaletteFade(PALETTES_ALL, -2, 0, 16, RGB_BLACK);
        break;
    case POKENAV_FADE_FROM_BLACK_ALL:
        BeginNormalPaletteFade(PALETTES_ALL, -2, 16, 0, RGB_BLACK);
        break;
    }
}

bool32 IsPaletteFadeActive(void)
{
    return gPaletteFade.active;
}

// Excludes the first obj and bg palettes
void FadeToBlackExceptPrimary(void)
{
    BlendPalettes(PALETTES_ALL & ~(1 << 16 | 1), 16, RGB_BLACK);
}

void InitBgTemplates(const struct BgTemplate *templates, int count)
{
    int i;

    for (i = 0; i < count; i++)
        InitBgFromTemplate(templates++);
}

static void InitHelpBar(void)
{
    struct Pokenav_MainMenu *menu = GetSubstructPtr(POKENAV_SUBSTRUCT_MAIN_MENU);

    InitWindows(&sHgssHelpBarWindowTemplate[0]);
    menu->helpBarWindowId = 0;
    LoadHgssPokegearHelpBarStrip();
    DrawHgssPokegearHelpBar(menu->helpBarWindowId);
    PutWindowTilemap(menu->helpBarWindowId);
    CopyWindowToVram(menu->helpBarWindowId, COPYWIN_FULL);
    CopyBgTilemapBufferToVram(0);
}

void PrintHelpBarText(u32 textId)
{
    struct Pokenav_MainMenu *menu = GetSubstructPtr(POKENAV_SUBSTRUCT_MAIN_MENU);
    s32 textX;
    s32 textWidth;

    DrawHgssPokegearHelpBar(menu->helpBarWindowId);
    textWidth = GetStringWidth(FONT_NORMAL, sHelpBarTexts[textId], 0);
    textX = (DISPLAY_WIDTH - textWidth) / 2;
    if (textX < 0)
        textX = 0;
    AddTextPrinterParameterized3(menu->helpBarWindowId, FONT_NORMAL, textX, 0, sHgssHelpBarTextColors, 0, sHelpBarTexts[textId]);
}

bool32 WaitForHelpBar(void)
{
    return IsDma3ManagerBusyWithBgCopy();
}

static void LoadHgssPokegearHelpBarPalette(void)
{
    // BG0 palette slots 7..10 are dedicated to the authentic HGSS tooltip
    // after legacy header removal. Copy exact retail member-10 colors there.
    CopyPaletteIntoBufferUnfaded(&sHgssHelpBarTooltip_Pal[1], BG_PLTT_ID(0) + HGSS_HELP_BAR_STRIP_COLOR, sizeof(u16));
    CopyPaletteIntoBufferUnfaded(&sHgssHelpBarTooltip_Pal[2], BG_PLTT_ID(0) + HGSS_HELP_BAR_SHADOW_COLOR, sizeof(u16));
    CopyPaletteIntoBufferUnfaded(&sHgssHelpBarTooltip_Pal[3], BG_PLTT_ID(0) + HGSS_HELP_BAR_TEXT_COLOR, sizeof(u16));
    CopyPaletteIntoBufferUnfaded(&sHgssHelpBarTooltip_Pal[5], BG_PLTT_ID(0) + HGSS_HELP_BAR_FILL_COLOR, sizeof(u16));
}

static void LoadHgssPokegearHelpBarStrip(void)
{
    LoadBgTiles(0, sHgssHelpBarTooltip_Gfx, sizeof(sHgssHelpBarTooltip_Gfx), HGSS_HELP_BAR_TILE);
    LoadHgssPokegearHelpBarPalette();

    // Retail destination is 32x4 at y=20. The GBA viewport keeps 30 columns.
    FillBgTilemapBufferRect_Palette0(
        0,
        HGSS_HELP_BAR_TILE,
        0,
        HGSS_HELP_BAR_STRIP_TOP,
        HGSS_HELP_BAR_STRIP_WIDTH,
        HGSS_HELP_BAR_STRIP_HEIGHT
    );
}

static void DrawHgssPokegearHelpBar(u32 windowId)
{
    // Retail fills the tooltip text window with source palette index 5.
    FillWindowPixelBuffer(windowId, PIXEL_FILL(HGSS_HELP_BAR_FILL_COLOR));
}

static void InitPokenavMainMenuResources(void)
{
    struct Pokenav_MainMenu *menu = GetSubstructPtr(POKENAV_SUBSTRUCT_MAIN_MENU);

    // The removed Emerald spinner no longer owns OBJ palette 0, so all OBJ
    // palettes participate normally in PokéNav fades.
    menu->palettes = ~1;
}

static void CleanupPokenavMainMenuResources(void)
{
}
