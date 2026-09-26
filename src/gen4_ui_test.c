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

static const u8 sGen4UiTestTiles[] = INCBIN_U8("graphics/gen4_ui/renderer_test.tiles.8bpp");
static const u16 sGen4UiTestTilemap[] = INCBIN_U16("graphics/gen4_ui/renderer_test.tilemap.bin");
static const u16 sGen4UiTestPalette[] = INCBIN_U16("graphics/gen4_ui/renderer_test.gbapal");

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
    .paletteOffset = BG_PLTT_ID(0),
    .paletteSize = sizeof(sGen4UiTestPalette),
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
