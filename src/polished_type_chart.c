#include "global.h"
#include "gpu_regs.h"
#include "item_menu.h"
#include "item_use.h"
#include "main.h"
#include "palette.h"
#include "constants/rgb.h"

static const u32 sTiles[] = INCBIN_U32("graphics/polished_type_chart/tiles.bin");
static const u16 sMap[] = INCBIN_U16("graphics/polished_type_chart/map.bin");
static const u16 sPalette[] = INCBIN_U16("graphics/polished_type_chart/palette.bin");
static EWRAM_DATA bool8 sClosing = FALSE;

static void ChartVBlank(void)
{
    TransferPlttBuffer();
}

static void CB2_PolishedTypeChart(void)
{
    if (!gPaletteFade.active)
    {
        if (sClosing)
        {
            SetMainCallback2(CB2_ReturnToBagMenuPocket);
            return;
        }
        if (JOY_NEW(A_BUTTON | B_BUTTON))
        {
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
            sClosing = TRUE;
        }
    }
    UpdatePaletteFade();
}

static void OpenChart(void)
{
    SetVBlankCallback(NULL);
    SetHBlankCallback(NULL);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_FORCED_BLANK);
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    SetGpuReg(REG_OFFSET_BG0HOFS, 0);
    SetGpuReg(REG_OFFSET_BG0VOFS, 0);
    SetGpuReg(REG_OFFSET_BG0CNT, BGCNT_256COLOR | BGCNT_SCREENBASE(31));
    CpuCopy32(sTiles, (void *)BG_CHAR_ADDR(0), sizeof(sTiles));
    CpuCopy16(sMap, (void *)BG_SCREEN_ADDR(31), sizeof(sMap));
    ResetPaletteFade();
    LoadPalette(sPalette, 0, sizeof(sPalette));
    BlendPalettes(PALETTES_ALL, 16, RGB_BLACK);
    BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
    sClosing = FALSE;
    SetVBlankCallback(ChartVBlank);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0 | DISPCNT_BG0_ON);
    SetMainCallback2(CB2_PolishedTypeChart);
}

void ItemUse_TypeChart(u8 taskId)
{
    gBagMenu->newScreenCallback = OpenChart;
    Task_FadeAndCloseBagMenu(taskId);
}
