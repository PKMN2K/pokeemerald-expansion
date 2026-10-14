#include "global.h"
#include "bg.h"
#include "gen4_ui.h"
#include "gpu_regs.h"
#include "main.h"
#include "overworld.h"
#include "palette.h"
#include "scanline_effect.h"
#include "sprite.h"

#define GEN4_UI_TEST_MAX_X 16
#define GEN4_UI_TEST_MAX_Y 32

static const u8 sGen4UiTestTiles[] = INCBIN_U8("graphics/gen4_ui/hgss_pokedex_member_020.tiles.8bpp");
static const u16 sGen4UiTestTilemap[] = INCBIN_U16("graphics/gen4_ui/hgss_pokedex_member_020.tilemap.bin");
static const u16 sGen4UiTestPalette[] = INCBIN_U16("graphics/gen4_ui/hgss_pokedex_member_020.gbapal");

static s16 sGen4UiTestX;
static s16 sGen4UiTestY;

static const struct BgTemplate sGen4UiTestBgTemplate[] =
{
    {
        .bg = 0,
        .charBaseIndex = 0,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 1,
        .priority = 0,
        .baseTile = 0,
    },
};

static const struct Gen4UiBgAsset sGen4UiTestAsset =
{
    .tiles = sGen4UiTestTiles,
    .tilemap = sGen4UiTestTilemap,
    .palette = sGen4UiTestPalette,
    .tilesSize = sizeof(sGen4UiTestTiles),
    .tilemapSize = sizeof(sGen4UiTestTilemap),
    .paletteOffset = BG_PLTT_ID(8),
    .paletteSize = PLTT_SIZEOF(13),
    .baseTile = 0,
    .tilemapOffset = 0,
    .paletteMode = GEN4_UI_PALETTE_8BPP,
};

static void Gen4UiTest_VBlank(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void Gen4UiTest_Main(void)
{
    bool32 moved = FALSE;

    if (JOY_NEW(B_BUTTON))
    {
        SetVBlankCallback(NULL);
        SetMainCallback2(CB2_ReturnToField);
        return;
    }

    if (JOY_HELD(DPAD_LEFT) && sGen4UiTestX > 0)
    {
        sGen4UiTestX--;
        moved = TRUE;
    }
    if (JOY_HELD(DPAD_RIGHT) && sGen4UiTestX < GEN4_UI_TEST_MAX_X)
    {
        sGen4UiTestX++;
        moved = TRUE;
    }
    if (JOY_HELD(DPAD_UP) && sGen4UiTestY > 0)
    {
        sGen4UiTestY--;
        moved = TRUE;
    }
    if (JOY_HELD(DPAD_DOWN) && sGen4UiTestY < GEN4_UI_TEST_MAX_Y)
    {
        sGen4UiTestY++;
        moved = TRUE;
    }

    if (moved)
    {
        ChangeBgX(0, sGen4UiTestX << 8, BG_COORD_SET);
        ChangeBgY(0, sGen4UiTestY << 8, BG_COORD_SET);
    }
}

void CB2_Gen4UiTest(void)
{
    switch (gMain.state)
    {
    case 0:
        SetVBlankCallback(NULL);
        SetGpuReg(REG_OFFSET_DISPCNT, 0);
        DmaClearLarge16(3, (void *)VRAM, VRAM_SIZE, 0x1000);
        DmaClear32(3, OAM, OAM_SIZE);
        DmaClear16(3, PLTT, PLTT_SIZE);
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sGen4UiTestBgTemplate, ARRAY_COUNT(sGen4UiTestBgTemplate));
        ResetPaletteFade();
        ScanlineEffect_Stop();
        ResetSpriteData();
        FreeAllSpritePalettes();
        sGen4UiTestX = 0;
        sGen4UiTestY = 0;
        gMain.state++;
        break;
    case 1:
        if (!Gen4UiLoadBgAsset(0, &sGen4UiTestAsset))
        {
            SetMainCallback2(CB2_ReturnToField);
            return;
        }
        gMain.state++;
        break;
    case 2:
        if (IsDma3ManagerBusyWithBgCopy())
            break;

        ChangeBgX(0, 0, BG_COORD_SET);
        ChangeBgY(0, 0, BG_COORD_SET);
        ShowBg(0);
        SetVBlankCallback(Gen4UiTest_VBlank);
        SetMainCallback2(Gen4UiTest_Main);
        break;
    }
}

// --------------------------------------------------------------------------
// Debug-only HGSS ribbon grid preview. This renders real 32x32 HGSS tiles
// into GBA BG0 VRAM, independently of save data and the live PokéNav.
// It intentionally does not imply that live PokéNav navigation was tested.
// DPAD Left/Right cycles the five physical ribbon pages; B exits.
// --------------------------------------------------------------------------

#define HGSS_RIBBON_PREVIEW_COLUMNS 4
#define HGSS_RIBBON_PREVIEW_PAGE_SIZE 8
#define HGSS_RIBBON_PREVIEW_PAGE_COUNT 5
#define HGSS_RIBBON_PREVIEW_ICON_TILES 4
#define HGSS_RIBBON_PREVIEW_LEFT_TILE 11
#define HGSS_RIBBON_PREVIEW_TOP_TILE 4
#define HGSS_RIBBON_PREVIEW_GIFT_START 28

static const u32 sHgssRibbonPreviewTiles[] = INCBIN_U32("graphics/gen4_ui/hgss_ribbons/big_icons_hgss.4bpp");
static const u16 sHgssRibbonPreviewPalette0[] = INCBIN_U16("graphics/gen4_ui/hgss_ribbons/palette_0.gbapal");
static const u16 sHgssRibbonPreviewPalette1[] = INCBIN_U16("graphics/gen4_ui/hgss_ribbons/palette_1.gbapal");
static const u16 sHgssRibbonPreviewPalette2[] = INCBIN_U16("graphics/gen4_ui/hgss_ribbons/palette_2.gbapal");
static const u16 sHgssRibbonPreviewPalette3[] = INCBIN_U16("graphics/gen4_ui/hgss_ribbons/palette_3.gbapal");
static const u16 sHgssRibbonPreviewPalette4[] = INCBIN_U16("graphics/gen4_ui/hgss_ribbons/palette_4.gbapal");

struct HgssRibbonPreviewIcon
{
    u8 frame;
    u8 bank;
};

// Ordered by Emerald ribbon ID, using the verified retail HGSS map.
static const struct HgssRibbonPreviewIcon sHgssRibbonPreviewIcons[] =
{
    {0, 0},  // Champion
    {1, 0}, {2, 0}, {3, 0}, {4, 0}, // Cool
    {1, 1}, {2, 1}, {3, 1}, {4, 1}, // Beauty
    {1, 2}, {2, 2}, {3, 2}, {4, 2}, // Cute
    {1, 3}, {2, 3}, {3, 3}, {4, 3}, // Smart
    {1, 4}, {2, 4}, {3, 4}, {4, 4}, // Tough
    {5, 0}, // Winning (HGSS member 78)
    {6, 0}, // Victory (HGSS member 77)
    {7, 1}, // Artist
    {8, 2}, // Effort
    {9, 1}, {9, 3}, {9, 4}, // Marine, Land, Sky
    {10, 3}, {10, 4}, // Country, National
    {11, 0}, {11, 1}, // Earth, World
};

static const struct BgTemplate sHgssRibbonPreviewBgTemplates[] =
{
    {
        .bg = 0,
        .charBaseIndex = 0,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0,
    },
};

static u16 sHgssRibbonPreviewMap[BG_SCREEN_SIZE / sizeof(u16)];
static u8 sHgssRibbonPreviewPage;

static void HgssRibbonPreview_DrawPage(void)
{
    u32 i;
    FillBgTilemapBufferRect_Palette0(0, 0, 0, 0, 32, 20);

    for (i = 0; i < HGSS_RIBBON_PREVIEW_PAGE_SIZE; i++)
    {
        u32 virtualSlot = sHgssRibbonPreviewPage * HGSS_RIBBON_PREVIEW_PAGE_SIZE + i;
        u32 ribbonId;
        const struct HgssRibbonPreviewIcon *icon;
        u16 tilemap[HGSS_RIBBON_PREVIEW_ICON_TILES * HGSS_RIBBON_PREVIEW_ICON_TILES];
        u32 j;

        if (virtualSlot < 25)
            ribbonId = virtualSlot;
        else if (virtualSlot >= HGSS_RIBBON_PREVIEW_GIFT_START
              && virtualSlot < HGSS_RIBBON_PREVIEW_GIFT_START + 7)
            ribbonId = virtualSlot - HGSS_RIBBON_PREVIEW_GIFT_START + 25;
        else
            continue;

        icon = &sHgssRibbonPreviewIcons[ribbonId];
        for (j = 0; j < ARRAY_COUNT(tilemap); j++)
            tilemap[j] = (1 + icon->frame * 16 + j) | ((icon->bank + 2) << 12);

        CopyToBgTilemapBufferRect(0, tilemap,
            HGSS_RIBBON_PREVIEW_LEFT_TILE + (i % HGSS_RIBBON_PREVIEW_COLUMNS) * HGSS_RIBBON_PREVIEW_ICON_TILES,
            HGSS_RIBBON_PREVIEW_TOP_TILE + (i / HGSS_RIBBON_PREVIEW_COLUMNS) * HGSS_RIBBON_PREVIEW_ICON_TILES,
            HGSS_RIBBON_PREVIEW_ICON_TILES, HGSS_RIBBON_PREVIEW_ICON_TILES);
    }
    CopyBgTilemapBufferToVram(0);
}

static void HgssRibbonPreview_Main(void)
{
    if (JOY_NEW(B_BUTTON))
    {
        SetVBlankCallback(NULL);
        SetMainCallback2(CB2_ReturnToField);
        return;
    }
    if (JOY_NEW(DPAD_RIGHT))
    {
        sHgssRibbonPreviewPage = (sHgssRibbonPreviewPage + 1) % HGSS_RIBBON_PREVIEW_PAGE_COUNT;
        HgssRibbonPreview_DrawPage();
    }
    if (JOY_NEW(DPAD_LEFT))
    {
        sHgssRibbonPreviewPage = (sHgssRibbonPreviewPage + HGSS_RIBBON_PREVIEW_PAGE_COUNT - 1)
                                % HGSS_RIBBON_PREVIEW_PAGE_COUNT;
        HgssRibbonPreview_DrawPage();
    }
}

void CB2_HgssRibbonGridTest(void)
{
    switch (gMain.state)
    {
    case 0:
        SetVBlankCallback(NULL);
        SetGpuReg(REG_OFFSET_DISPCNT, 0);
        DmaClearLarge16(3, (void *)VRAM, VRAM_SIZE, 0x1000);
        DmaClear32(3, OAM, OAM_SIZE);
        DmaClear16(3, PLTT, PLTT_SIZE);
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sHgssRibbonPreviewBgTemplates, ARRAY_COUNT(sHgssRibbonPreviewBgTemplates));
        ResetPaletteFade();
        ScanlineEffect_Stop();
        ResetSpriteData();
        FreeAllSpritePalettes();
        SetBgTilemapBuffer(0, sHgssRibbonPreviewMap);
        sHgssRibbonPreviewPage = 0;
        gMain.state++;
        break;
    case 1:
        if (LoadBgTiles(0, sHgssRibbonPreviewTiles, sizeof(sHgssRibbonPreviewTiles), 1) == 0xFFFF)
        {
            SetMainCallback2(CB2_ReturnToField);
            return;
        }
        LoadPalette(sHgssRibbonPreviewPalette0, BG_PLTT_ID(2), sizeof(sHgssRibbonPreviewPalette0));
        LoadPalette(sHgssRibbonPreviewPalette1, BG_PLTT_ID(3), sizeof(sHgssRibbonPreviewPalette1));
        LoadPalette(sHgssRibbonPreviewPalette2, BG_PLTT_ID(4), sizeof(sHgssRibbonPreviewPalette2));
        LoadPalette(sHgssRibbonPreviewPalette3, BG_PLTT_ID(5), sizeof(sHgssRibbonPreviewPalette3));
        LoadPalette(sHgssRibbonPreviewPalette4, BG_PLTT_ID(6), sizeof(sHgssRibbonPreviewPalette4));
        HgssRibbonPreview_DrawPage();
        gMain.state++;
        break;
    case 2:
        if (IsDma3ManagerBusyWithBgCopy())
            break;
        ShowBg(0);
        SetVBlankCallback(Gen4UiTest_VBlank);
        SetMainCallback2(HgssRibbonPreview_Main);
        break;
    }
}
