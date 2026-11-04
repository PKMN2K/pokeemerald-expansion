#include "global.h"
#include "bg.h"
#include "window.h"
#include "pokenav.h"
#include "decompress.h"
#include "gpu_regs.h"
#include "graphics.h"
#include "menu.h"
#include "menu_specialized.h"
#include "mon_markings.h"
#include "palette.h"
#include "scanline_effect.h"
#include "string_util.h"
#include "strings.h"
#include "text.h"
#include "constants/songs.h"
#include "sound.h"

static u32 LoopedTask_TransitionMons(s32);
static u32 LoopedTask_ExitConditionGraphMenu(s32);
static u32 LoopedTask_MoveCursorNoTransition(s32);
static u32 LoopedTask_SlideMonOut(s32);
static u32 LoopedTask_OpenMonMarkingsWindow(s32);
static u32 LoopedTask_CloseMonMarkingsWindow(s32);

// Retail HGSS panel/font/star assets. The blue fill is data rendered by the
// existing condition scanline windows, using an unchanged HGSS palette entry.
static const u32 sHgssGraphTiles[] = INCBIN_U32("graphics/gen4_ui/hgss_condition/graph_bg.4bpp");
static const u16 sHgssGraphMap[] = INCBIN_U16("graphics/gen4_ui/hgss_condition/graph_bg.bin");
static const u16 sHgssTextPal[] = INCBIN_U16("graphics/gen4_ui/hgss_ribbons/summary_bg.bank_1.gbapal");
static const u16 sHgssHeaderPal[] = INCBIN_U16("graphics/gen4_ui/hgss_ribbons/summary_bg.bank_7.gbapal");
static const u32 sGraphDataTile[] = {0x99999999, 0x99999999, 0x99999999, 0x99999999, 0x99999999, 0x99999999, 0x99999999, 0x99999999};
static const u32 sHgssMarkingsPanel[] = INCBIN_U32("graphics/gen4_ui/hgss_condition/markings_panel.4bpp");
static const u16 sHgssMarkingsPal[] = INCGFX_U16("graphics/gen4_ui/hgss_storage/verified/markings_menu.png", ".gbapal");
static const u32 sHgssMarkingsCombo[] = INCBIN_U32("graphics/gen4_ui/hgss_storage/verified/marking_combo_hgss.4bpp");
// Original /a/0/1/9 member 4 bank 3, reordered to the verified PNG indices.
static const u16 sHgssMarkingsSelectedPal[] = {0x5790, 0x4E94, 0x356B, 0x2D57, 0x41FC, 0x4E94, 0x4E94, 0x39EF, 0x294A, 0, 0, 0, 0, 0, 0, 0};
static const u32 sHgssCursorTiles[] = INCBIN_U32("graphics/gen4_ui/hgss_pokegear/condition_search_cursor.4bpp");
static const u16 sHgssCursorPal[] = INCBIN_U16("graphics/gen4_ui/hgss_pokegear/condition_search_cursor.gbapal");
static const u8 sTextColors[] = {0, 5, 2};
static const u8 sText_Condition[] = _("CONDITION");
static const u8 sText_Number[] = _("No. ");
static const u8 sText_Cancel[] = _("CANCEL");
static const u8 sMarkingBitBySlot[] = {0, 2, 1, 3};

static const struct BgTemplate sMenuBgTemplates[] =
{
    {.bg = 1, .charBaseIndex = 1, .mapBaseIndex = 6, .screenSize = 0, .paletteMode = 0, .priority = 1, .baseTile = 0},
    {.bg = 2, .charBaseIndex = 3, .mapBaseIndex = 7, .screenSize = 0, .paletteMode = 0, .priority = 2, .baseTile = 0},
    {.bg = 3, .charBaseIndex = 2, .mapBaseIndex = 4, .screenSize = 0, .paletteMode = 0, .priority = 3, .baseTile = 0},
};

static const struct WindowTemplate sTitleWindow =
    {.bg = 1, .tilemapLeft = 1, .tilemapTop = 1, .width = 14, .height = 2, .paletteNum = 1, .baseBlock = 16};
static const struct WindowTemplate sIndexWindow =
    {.bg = 1, .tilemapLeft = 22, .tilemapTop = 1, .width = 7, .height = 2, .paletteNum = 1, .baseBlock = 44};
static const struct WindowTemplate sNameWindow =
    {.bg = 1, .tilemapLeft = 1, .tilemapTop = 3, .width = 18, .height = 2, .paletteNum = 1, .baseBlock = 64};
static const struct WindowTemplate sLocationWindow =
    {.bg = 1, .tilemapLeft = 20, .tilemapTop = 3, .width = 9, .height = 2, .paletteNum = 1, .baseBlock = 100};
static const struct WindowTemplate sMarkingsWindow =
    {.bg = 1, .tilemapLeft = 11, .tilemapTop = 6, .width = 4, .height = 1, .paletteNum = 12, .baseBlock = 118};

#define MARKS_LEFT 19
#define MARKS_TOP 1
#define MARKS_WIDTH 11
#define MARKS_HEIGHT 15
#define MARKS_BASE_TILE 256
#define MARKS_PAL 12
#define MARKS_SELECTED_PAL 13

static const struct OamData sMarkCursorOam =
    {.shape = SPRITE_SHAPE(8x16), .size = SPRITE_SIZE(8x16), .priority = 0};
static const union AnimCmd sMarkCursorAnim[] = {ANIMCMD_FRAME(0, 0), ANIMCMD_END};
static const union AnimCmd *const sMarkCursorAnims[] = {sMarkCursorAnim};
static const struct SpriteTemplate sMarkCursorTemplate =
{
    .tileTag = TAG_CONDITION_MARKINGS_MENU,
    .paletteTag = TAG_CONDITION_MARKINGS_MENU,
    .oam = &sMarkCursorOam,
    .anims = sMarkCursorAnims,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};

static const LoopedTask sLoopedTaskFuncs[] =
{
    [CONDITION_FUNC_NONE]           = NULL,
    [CONDITION_FUNC_SLIDE_MON_IN]   = LoopedTask_TransitionMons,
    [CONDITION_FUNC_RETURN]         = LoopedTask_ExitConditionGraphMenu,
    [CONDITION_FUNC_NO_TRANSITION]  = LoopedTask_MoveCursorNoTransition,
    [CONDITION_FUNC_SLIDE_MON_OUT]  = LoopedTask_SlideMonOut,
    [CONDITION_FUNC_ADD_MARKINGS]   = LoopedTask_OpenMonMarkingsWindow,
    [CONDITION_FUNC_CLOSE_MARKINGS] = LoopedTask_CloseMonMarkingsWindow
};

typedef u8 ALIGNED(4) TilemapBuffer[BG_SCREEN_SIZE];

struct Pokenav_ConditionMenuGfx
{
    u32 loopedTaskId;
    TilemapBuffer tilemapBuffers[2];
    u32 (*callback)(void);
    s16 monTransitionX;
    u8 monPicSpriteId;
    u16 monPalIndex;
    u16 monGfxTileStart;
    void *monGfxPtr;
    u8 nameGenderWindowId;
    u8 locationWindowId;
    u8 listIndexWindowId;
    u8 titleWindowId;
    u8 markingsWindowId;
    u8 markings;
    u8 pendingMarkings;
    u8 markingCursor;
    u8 markingCursorSpriteId;
    struct Sprite *conditionSparkleSprites[MAX_CONDITION_SPARKLES];
};

static u32 LoopedTask_OpenConditionGraphMenu(s32);
static u32 GetConditionGraphMenuLoopedTaskActive(void);
static void CreateConditionMonPic(u8);
static void LoadHgssConditionSprites(void);
static void CreateHgssConditionSparkles(void);
static void DrawConditionMarkings(void);
static void OpenConditionMarkings(void);
static void CloseConditionMarkings(void);
static void UpdateConditionMarkings(void);
static void UpdateConditionMarkCursor(void);
static bool32 IsConditionCancelSelected(void);
static bool32 UpdateConditionGraphMenuWindows(u8, u16, bool8);
static void VBlankCB_PokenavConditionGraph(void);
static void DoConditionGraphEnterTransition(void);
static void DoConditionGraphExitTransition(void);
static void SetExitVBlank(void);
static void ToggleGraphData(bool8);

bool32 OpenConditionGraphMenu(void)
{
    struct Pokenav_ConditionMenuGfx *menu = AllocSubstruct(POKENAV_SUBSTRUCT_CONDITION_GRAPH_MENU_GFX, sizeof(struct Pokenav_ConditionMenuGfx));

    if (menu == NULL)
        return FALSE;

    menu->monPicSpriteId = SPRITE_NONE;
    menu->loopedTaskId = CreateLoopedTask(LoopedTask_OpenConditionGraphMenu, 1);
    menu->callback = GetConditionGraphMenuLoopedTaskActive;
    menu->markingCursorSpriteId = SPRITE_NONE;
    return TRUE;
}

void CreateConditionGraphMenuLoopedTask(s32 id)
{
    struct Pokenav_ConditionMenuGfx *menu = GetSubstructPtr(POKENAV_SUBSTRUCT_CONDITION_GRAPH_MENU_GFX);
    menu->loopedTaskId = CreateLoopedTask(sLoopedTaskFuncs[id], 1);
    menu->callback = GetConditionGraphMenuLoopedTaskActive;
}

u32 IsConditionGraphMenuLoopedTaskActive(void)
{
    struct Pokenav_ConditionMenuGfx *menu = GetSubstructPtr(POKENAV_SUBSTRUCT_CONDITION_GRAPH_MENU_GFX);
    return menu->callback();
}

static u32 GetConditionGraphMenuLoopedTaskActive(void)
{
    struct Pokenav_ConditionMenuGfx *menu = GetSubstructPtr(POKENAV_SUBSTRUCT_CONDITION_GRAPH_MENU_GFX);
    return IsLoopedTaskActive(menu->loopedTaskId);
}

static u32 LoopedTask_OpenConditionGraphMenu(s32 state)
{
    struct Pokenav_ConditionMenuGfx *menu = GetSubstructPtr(POKENAV_SUBSTRUCT_CONDITION_GRAPH_MENU_GFX);

    switch (state)
    {
    case 0:
        if (!LoadConditionGraphMenuGfx())
            return LT_PAUSE;
        return LT_INC_AND_PAUSE;
    case 1:
        InitBgTemplates(sMenuBgTemplates, ARRAY_COUNT(sMenuBgTemplates));
        for (u32 bg = 1; bg <= 3; bg++)
        {
            ChangeBgX(bg, 0, BG_COORD_SET);
            ChangeBgY(bg, 0, BG_COORD_SET);
        }
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_WIN0_ON | DISPCNT_WIN1_ON | DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP | DISPCNT_BG0_ON);
        SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG2 | BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_BG3);
        SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(11, 4));
        LoadBgTiles(3, sHgssGraphTiles, sizeof(sHgssGraphTiles), 0);
        SetBgTilemapBuffer(3, menu->tilemapBuffers[0]);
        CopyToBgTilemapBuffer(3, sHgssGraphMap, sizeof(sHgssGraphMap), 0);
        CopyBgTilemapBufferToVram(3);
        CopyPaletteIntoBufferUnfaded(sHgssTextPal, BG_PLTT_ID(1), sizeof(sHgssTextPal));
        CopyPaletteIntoBufferUnfaded(sHgssHeaderPal, BG_PLTT_ID(7), sizeof(sHgssHeaderPal));
        CopyPaletteIntoBufferUnfaded(sHgssMarkingsPal, BG_PLTT_ID(MARKS_PAL), sizeof(sHgssMarkingsPal));
        CopyPaletteIntoBufferUnfaded(sHgssMarkingsSelectedPal, BG_PLTT_ID(MARKS_SELECTED_PAL), sizeof(sHgssMarkingsSelectedPal));
        menu->monTransitionX = -80;
        return LT_INC_AND_PAUSE;
    case 2:
        if (FreeTempTileDataBuffersIfPossible())
            return LT_PAUSE;
        LoadBgTiles(2, sGraphDataTile, sizeof(sGraphDataTile), 1);
        SetBgTilemapBuffer(2, menu->tilemapBuffers[1]);
        FillBgTilemapBufferRect(2, 1, 0, 0, 32, 32, 1);
        CopyBgTilemapBufferToVram(2);
        ConditionGraph_InitWindow(2);
        return LT_INC_AND_PAUSE;
    case 3:
        BgDmaFill(1, 0, 0, 1);
        menu->titleWindowId = AddWindow(&sTitleWindow);
        menu->listIndexWindowId = AddWindow(&sIndexWindow);
        menu->nameGenderWindowId = AddWindow(&sNameWindow);
        menu->locationWindowId = AddWindow(&sLocationWindow);
        menu->markingsWindowId = AddWindow(&sMarkingsWindow);
        // AddWindow owns BG1's map. Popup edits must use that same buffer.
        FillBgTilemapBufferRect_Palette0(1, 0, 0, 0, 32, 32);
        FillWindowPixelBuffer(menu->titleWindowId, PIXEL_FILL(0));
        AddTextPrinterParameterized3(menu->titleWindowId, FONT_NORMAL, 0, 1, sTextColors, TEXT_SKIP_DRAW, sText_Condition);
        PutWindowTilemap(menu->titleWindowId);
        CopyWindowToVram(menu->titleWindowId, COPYWIN_FULL);
        return LT_INC_AND_PAUSE;
    case 4:
        if (!IsConditionCancelSelected())
            CreateConditionMonPic(0);
        LoadHgssConditionSprites();
        return LT_INC_AND_PAUSE;
    case 5:
        UpdateConditionGraphMenuWindows(0, GetConditionGraphMenuCurrentLoadIndex(), TRUE);
        UpdateConditionGraphMenuWindows(1, GetConditionGraphMenuCurrentLoadIndex(), TRUE);
        UpdateConditionGraphMenuWindows(2, GetConditionGraphMenuCurrentLoadIndex(), TRUE);
        UpdateConditionGraphMenuWindows(3, GetConditionGraphMenuCurrentLoadIndex(), TRUE);
        return LT_INC_AND_PAUSE;
    case 6:
        ShowBg(1);
        HideBg(2);
        ShowBg(3);
        PrintHelpBarText(IsConditionMenuSearchMode() ? HELPBAR_CONDITION_MON_STATUS : HELPBAR_CONDITION_PARTY);
        PokenavFadeScreen(POKENAV_FADE_FROM_BLACK);
        return LT_INC_AND_PAUSE;
    case 7:
        if (IsPaletteFadeActive())
            return LT_PAUSE;
        SetVBlankCallback_(VBlankCB_PokenavConditionGraph);
        DoConditionGraphEnterTransition();
        ConditionGraph_InitResetScanline(GetConditionGraphPtr());
        return LT_INC_AND_PAUSE;
    case 8:
        if (ConditionGraph_ResetScanline(GetConditionGraphPtr()))
            return LT_PAUSE;
        ToggleGraphData(!IsConditionCancelSelected());
        return LT_INC_AND_PAUSE;
    case 9:
        if (ConditionMenu_UpdateMonEnter(GetConditionGraphPtr(), &menu->monTransitionX))
            return LT_PAUSE;
        CreateHgssConditionSparkles();
        return LT_FINISH;
    }
    return LT_FINISH;
}

static u32 LoopedTask_ExitConditionGraphMenu(s32 state)
{
    struct Pokenav_ConditionMenuGfx *menu = GetSubstructPtr(POKENAV_SUBSTRUCT_CONDITION_GRAPH_MENU_GFX);

    switch (state)
    {
    case 0:
        DoConditionGraphExitTransition();
        DestroyConditionSparkleSprites(menu->conditionSparkleSprites);
        return LT_INC_AND_CONTINUE;
    case 1:
        if (ConditionMenu_UpdateMonExit(GetConditionGraphPtr(), &menu->monTransitionX))
            return 2;
        ToggleGraphData(FALSE);
        return LT_INC_AND_CONTINUE;
    case 2:
        PokenavFadeScreen(POKENAV_FADE_TO_BLACK);
        if (!IsConditionMenuSearchMode())
            SlideMenuHeaderDown();
        return LT_INC_AND_PAUSE;
    case 3:
        if (IsPaletteFadeActive() || MainMenuLoopedTaskIsBusy())
            return LT_PAUSE;
        FreeConditionSparkles(menu->conditionSparkleSprites);
        HideBg(1);
        HideBg(2);
        HideBg(3);
        return LT_INC_AND_CONTINUE;
    }

    return LT_FINISH;
}

static u32 LoopedTask_TransitionMons(s32 state)
{
    struct Pokenav_ConditionMenuGfx *menu = GetSubstructPtr(POKENAV_SUBSTRUCT_CONDITION_GRAPH_MENU_GFX);
    struct ConditionGraph *graph = GetConditionGraphPtr();

    switch (state)
    {
    case 0:
        LoadNextConditionMenuMonData(CONDITION_LOAD_MON_INFO);
        return LT_INC_AND_CONTINUE;
    case 1:
        LoadNextConditionMenuMonData(CONDITION_LOAD_GRAPH);
        return LT_INC_AND_CONTINUE;
    case 2:
        LoadNextConditionMenuMonData(CONDITION_LOAD_MON_PIC);
        DestroyConditionSparkleSprites(menu->conditionSparkleSprites);
        return LT_INC_AND_CONTINUE;
    case 3:
        ConditionGraph_TryUpdate(graph);
        return LT_INC_AND_CONTINUE;
    case 4:
        if (!MoveConditionMonOffscreen(&menu->monTransitionX))
        {
            CreateConditionMonPic(GetConditionGraphMenuCurrentLoadIndex());
            return LT_INC_AND_CONTINUE;
        }
        return LT_PAUSE;
    case 5:
        UpdateConditionGraphMenuWindows(0, GetConditionGraphMenuCurrentLoadIndex(), FALSE);
        return LT_INC_AND_CONTINUE;
    case 6:
        UpdateConditionGraphMenuWindows(1, GetConditionGraphMenuCurrentLoadIndex(), FALSE);
        return LT_INC_AND_CONTINUE;
    case 7:
        UpdateConditionGraphMenuWindows(2, GetConditionGraphMenuCurrentLoadIndex(), FALSE);
        return LT_INC_AND_CONTINUE;
    case 8:
        if (UpdateConditionGraphMenuWindows(3, GetConditionGraphMenuCurrentLoadIndex(), FALSE) == TRUE)
            return LT_INC_AND_CONTINUE;
        return LT_PAUSE;
    case 9:
        graph = GetConditionGraphPtr();
        if (!ConditionMenu_UpdateMonEnter(graph, &menu->monTransitionX))
        {
            CreateHgssConditionSparkles();
            return LT_INC_AND_CONTINUE;
        }
        return LT_PAUSE;
    }

    return LT_FINISH;
}

static u32 LoopedTask_MoveCursorNoTransition(s32 state)
{
    struct Pokenav_ConditionMenuGfx *menu = GetSubstructPtr(POKENAV_SUBSTRUCT_CONDITION_GRAPH_MENU_GFX);

    switch (state)
    {
    case 0:
        LoadNextConditionMenuMonData(CONDITION_LOAD_MON_INFO);
        return LT_INC_AND_CONTINUE;
    case 1:
        LoadNextConditionMenuMonData(CONDITION_LOAD_GRAPH);
        return LT_INC_AND_CONTINUE;
    case 2:
        LoadNextConditionMenuMonData(CONDITION_LOAD_MON_PIC);
        return LT_INC_AND_CONTINUE;
    case 3:
        CreateConditionMonPic(GetConditionGraphMenuCurrentLoadIndex());
        return LT_INC_AND_CONTINUE;
    case 4:
        UpdateConditionGraphMenuWindows(0, GetConditionGraphMenuCurrentLoadIndex(), FALSE);
        return LT_INC_AND_CONTINUE;
    case 5:
        UpdateConditionGraphMenuWindows(1, GetConditionGraphMenuCurrentLoadIndex(), FALSE);
        return LT_INC_AND_CONTINUE;
    case 6:
        UpdateConditionGraphMenuWindows(2, GetConditionGraphMenuCurrentLoadIndex(), FALSE);
        return LT_INC_AND_CONTINUE;
    case 7:
        if (UpdateConditionGraphMenuWindows(3, GetConditionGraphMenuCurrentLoadIndex(), FALSE) == TRUE)
            return LT_INC_AND_CONTINUE;
        return LT_PAUSE;
    case 8:
        if (!ConditionMenu_UpdateMonEnter(GetConditionGraphPtr(), &menu->monTransitionX))
        {
            CreateHgssConditionSparkles();
            return LT_INC_AND_CONTINUE;
        }
        return LT_PAUSE;
    }

    return LT_FINISH;
}

static u32 LoopedTask_SlideMonOut(s32 state)
{
    struct Pokenav_ConditionMenuGfx *menu = GetSubstructPtr(POKENAV_SUBSTRUCT_CONDITION_GRAPH_MENU_GFX);

    switch (state)
    {
    case 0:
        LoadNextConditionMenuMonData(CONDITION_LOAD_MON_INFO);
        return LT_INC_AND_CONTINUE;
    case 1:
        LoadNextConditionMenuMonData(CONDITION_LOAD_GRAPH);
        return LT_INC_AND_CONTINUE;
    case 2:
        LoadNextConditionMenuMonData(CONDITION_LOAD_MON_PIC);
        DestroyConditionSparkleSprites(menu->conditionSparkleSprites);
        return LT_INC_AND_CONTINUE;
    case 3:
        if (!ConditionMenu_UpdateMonExit(GetConditionGraphPtr(), &menu->monTransitionX))
            return LT_INC_AND_CONTINUE;
        return LT_PAUSE;
    case 4:
        UpdateConditionGraphMenuWindows(0, GetConditionGraphMenuCurrentLoadIndex(), FALSE);
        return LT_INC_AND_CONTINUE;
    case 5:
        UpdateConditionGraphMenuWindows(1, GetConditionGraphMenuCurrentLoadIndex(), FALSE);
        return LT_INC_AND_CONTINUE;
    case 6:
        UpdateConditionGraphMenuWindows(2, GetConditionGraphMenuCurrentLoadIndex(), FALSE);
        return LT_INC_AND_CONTINUE;
    case 7:
        if (UpdateConditionGraphMenuWindows(3, GetConditionGraphMenuCurrentLoadIndex(), FALSE) == TRUE)
            return LT_INC_AND_CONTINUE;
        return LT_PAUSE;
    }

    return LT_FINISH;
}

static u32 LoopedTask_OpenMonMarkingsWindow(s32 state)
{
    switch (state)
    {
    case 0:
        OpenConditionMarkings();
        return LT_INC_AND_CONTINUE;
    case 1:
        PrintHelpBarText(HELPBAR_CONDITION_MARKINGS);
        return LT_INC_AND_CONTINUE;
    case 2:
        if (WaitForHelpBar() == TRUE)
            return LT_PAUSE;
        return LT_INC_AND_CONTINUE;
    }

    return LT_FINISH;
}

static u32 LoopedTask_CloseMonMarkingsWindow(s32 state)
{
    switch (state)
    {
    case 0:
        CloseConditionMarkings();
        return LT_INC_AND_CONTINUE;
    case 1:
        PrintHelpBarText(HELPBAR_CONDITION_MON_STATUS);
        return LT_INC_AND_CONTINUE;
    case 2:
        if (WaitForHelpBar() == TRUE)
            return LT_PAUSE;
        return LT_INC_AND_CONTINUE;
    }

    return LT_FINISH;
}

static bool32 IsConditionCancelSelected(void)
{
    return !IsConditionMenuSearchMode() && GetConditionGraphCurrentListIndex() == GetMonListCount() - 1;
}

static void DrawConditionMarkings(void)
{
    struct Pokenav_ConditionMenuGfx *menu = GetSubstructPtr(POKENAV_SUBSTRUCT_CONDITION_GRAPH_MENU_GFX);
    u8 markings = IsConditionCancelSelected() ? 0 : TryGetMonMarkId();
    FillWindowPixelBuffer(menu->markingsWindowId, PIXEL_FILL(0));
    for (u32 slot = 0; slot < NUM_MON_MARKINGS; slot++)
        if (markings & (1 << sMarkingBitBySlot[slot]))
            BlitBitmapToWindow(menu->markingsWindowId, (const u8 *)sHgssMarkingsCombo + slot * TILE_SIZE_4BPP, slot * 8, 0, 8, 8);
    PutWindowTilemap(menu->markingsWindowId);
    CopyWindowToVram(menu->markingsWindowId, COPYWIN_FULL);
}

static bool32 UpdateConditionGraphMenuWindows(u8 mode, u16 bufferIndex, bool8 winMode)
{
    struct Pokenav_ConditionMenuGfx *menu = GetSubstructPtr(POKENAV_SUBSTRUCT_CONDITION_GRAPH_MENU_GFX);
    u8 text[32], *end;

    switch (mode)
    {
    case 0:
        FillWindowPixelBuffer(menu->nameGenderWindowId, PIXEL_FILL(0));
        FillWindowPixelBuffer(menu->locationWindowId, PIXEL_FILL(0));
        FillWindowPixelBuffer(menu->listIndexWindowId, PIXEL_FILL(0));
        break;
    case 1:
        AddTextPrinterParameterized3(menu->nameGenderWindowId, FONT_NORMAL, 0, 1, sTextColors, TEXT_SKIP_DRAW,
                                     IsConditionCancelSelected() ? gText_Cancel : GetConditionMonNameText(bufferIndex));
        break;
    case 2:
        ToggleGraphData(!IsConditionCancelSelected());
        if (IsConditionCancelSelected())
            StringCopy(text, sText_Cancel);
        else if (IsConditionMenuSearchMode())
        {
            end = StringCopy(text, sText_Number);
            ConvertIntToDecimalStringN(end, GetConditionMonDataBuffer(), STR_CONV_MODE_RIGHT_ALIGN, 3);
        }
        else
        {
            end = ConvertIntToDecimalStringN(text, GetConditionGraphCurrentListIndex() + 1, STR_CONV_MODE_RIGHT_ALIGN, 1);
            end = StringCopy(end, COMPOUND_STRING(" / "));
            ConvertIntToDecimalStringN(end, GetMonListCount() - 1, STR_CONV_MODE_LEFT_ALIGN, 1);
        }
        AddTextPrinterParameterized3(menu->listIndexWindowId, FONT_NORMAL, 0, 1, sTextColors, TEXT_SKIP_DRAW, text);
        if (!IsConditionCancelSelected())
            AddTextPrinterParameterized3(menu->locationWindowId, FONT_NORMAL, 0, 1, sTextColors, TEXT_SKIP_DRAW, GetConditionMonLocationText(bufferIndex));
        DrawConditionMarkings();
        break;
    case 3:
        PutWindowTilemap(menu->nameGenderWindowId);
        PutWindowTilemap(menu->locationWindowId);
        PutWindowTilemap(menu->listIndexWindowId);
        CopyWindowToVram(menu->nameGenderWindowId, winMode ? COPYWIN_FULL : COPYWIN_GFX);
        CopyWindowToVram(menu->locationWindowId, winMode ? COPYWIN_FULL : COPYWIN_GFX);
        CopyWindowToVram(menu->listIndexWindowId, winMode ? COPYWIN_FULL : COPYWIN_GFX);
        break;
    }
    return TRUE;
}

static void LoadHgssConditionSprites(void)
{
    struct SpriteSheet sheet;
    struct SpritePalette pal;
    LoadHgssConditionSparkle(&sheet, &pal);
    LoadSpriteSheet(&sheet);
    Pokenav_AllocAndLoadPalettes((const struct SpritePalette[]){{pal.data, pal.tag}, {NULL}});
}

static void CreateHgssConditionSparkles(void)
{
    struct Pokenav_ConditionMenuGfx *menu = GetSubstructPtr(POKENAV_SUBSTRUCT_CONDITION_GRAPH_MENU_GFX);
    ResetConditionSparkleSprites(menu->conditionSparkleSprites);
    if (IsConditionCancelSelected() || menu->monPicSpriteId == SPRITE_NONE)
        return;
    CreateHgssConditionSparkleSprites(menu->conditionSparkleSprites, menu->monPicSpriteId, GetNumConditionMonSparkles());
}

static void UpdateConditionMarkCursor(void)
{
    static const s16 positions[][2] = {{165, 28}, {197, 28}, {165, 52}, {197, 52}, {164, 80}, {164, 112}};
    struct Pokenav_ConditionMenuGfx *menu = GetSubstructPtr(POKENAV_SUBSTRUCT_CONDITION_GRAPH_MENU_GFX);
    if (menu->markingCursorSpriteId != SPRITE_NONE)
    {
        gSprites[menu->markingCursorSpriteId].x = positions[menu->markingCursor][0];
        gSprites[menu->markingCursorSpriteId].y = positions[menu->markingCursor][1];
    }
}

static void UpdateConditionMarkings(void)
{
    struct Pokenav_ConditionMenuGfx *menu = GetSubstructPtr(POKENAV_SUBSTRUCT_CONDITION_GRAPH_MENU_GFX);
    u16 *map = GetBgTilemapBuffer(1);
    for (u32 slot = 0; slot < NUM_MON_MARKINGS; slot++)
    {
        u32 left = MARKS_LEFT + 1 + (slot % 2) * 5;
        u32 top = MARKS_TOP + 1 + (slot / 2) * 3;
        u32 pal = (menu->pendingMarkings & (1 << sMarkingBitBySlot[slot])) ? MARKS_SELECTED_PAL : MARKS_PAL;
        for (u32 y = 0; y < 3; y++)
            for (u32 x = 0; x < 4; x++)
                map[(top + y) * 32 + left + x] = (map[(top + y) * 32 + left + x] & 0xFFF) | (pal << 12);
    }
    CopyBgTilemapBufferToVram(1);
}

static void OpenConditionMarkings(void)
{
    struct Pokenav_ConditionMenuGfx *menu = GetSubstructPtr(POKENAV_SUBSTRUCT_CONDITION_GRAPH_MENU_GFX);
    const struct SpriteSheet sheet = {sHgssCursorTiles, sizeof(sHgssCursorTiles), TAG_CONDITION_MARKINGS_MENU};
    const struct SpritePalette pal = {sHgssCursorPal, TAG_CONDITION_MARKINGS_MENU};
    u16 *map = GetBgTilemapBuffer(1);
    menu->markings = menu->pendingMarkings = TryGetMonMarkId();
    menu->markingCursor = 0;
    LoadBgTiles(1, sHgssMarkingsPanel, sizeof(sHgssMarkingsPanel), MARKS_BASE_TILE);
    for (u32 y = 0; y < MARKS_HEIGHT; y++)
        for (u32 x = 0; x < MARKS_WIDTH; x++)
            map[(MARKS_TOP + y) * 32 + MARKS_LEFT + x] = (MARKS_BASE_TILE + y * MARKS_WIDTH + x) | (MARKS_PAL << 12);
    UpdateConditionMarkings();
    HideBg(2);
    LoadSpriteSheet(&sheet);
    LoadSpritePalette(&pal);
    menu->markingCursorSpriteId = CreateSprite(&sMarkCursorTemplate, 0, 0, 0);
    UpdateConditionMarkCursor();
}

bool8 HandleConditionMarkingsMenuInput(void)
{
    struct Pokenav_ConditionMenuGfx *menu = GetSubstructPtr(POKENAV_SUBSTRUCT_CONDITION_GRAPH_MENU_GFX);
    u8 cursor = menu->markingCursor;
    if (JOY_NEW(B_BUTTON))
    {
        PlaySE(SE_SELECT);
        return FALSE; // Cancel keeps the original four saved marking bits.
    }
    if (JOY_NEW(A_BUTTON))
    {
        PlaySE(SE_SELECT);
        if (cursor >= NUM_MON_MARKINGS)
        {
            if (cursor == NUM_MON_MARKINGS)
                menu->markings = menu->pendingMarkings;
            return FALSE;
        }
        menu->pendingMarkings ^= 1 << sMarkingBitBySlot[cursor];
        UpdateConditionMarkings();
    }
    else if (JOY_NEW(DPAD_LEFT | DPAD_RIGHT) && cursor < NUM_MON_MARKINGS)
        menu->markingCursor ^= 1;
    else if (JOY_NEW(DPAD_UP))
        menu->markingCursor = cursor < 2 ? 5 : cursor < 4 ? cursor - 2 : cursor - 1;
    else if (JOY_NEW(DPAD_DOWN))
        menu->markingCursor = cursor < 2 ? cursor + 2 : cursor < 4 ? 4 : cursor == 4 ? 5 : 0;
    if (cursor != menu->markingCursor)
    {
        PlaySE(SE_SELECT);
        UpdateConditionMarkCursor();
    }
    return TRUE;
}

static void CloseConditionMarkings(void)
{
    struct Pokenav_ConditionMenuGfx *menu = GetSubstructPtr(POKENAV_SUBSTRUCT_CONDITION_GRAPH_MENU_GFX);
    if (menu->markingCursorSpriteId != SPRITE_NONE)
    {
        DestroySprite(&gSprites[menu->markingCursorSpriteId]);
        menu->markingCursorSpriteId = SPRITE_NONE;
    }
    FreeSpriteTilesByTag(TAG_CONDITION_MARKINGS_MENU);
    FreeSpritePaletteByTag(TAG_CONDITION_MARKINGS_MENU);
    FillBgTilemapBufferRect_Palette0(1, 0, MARKS_LEFT, MARKS_TOP, MARKS_WIDTH, MARKS_HEIGHT);
    PutWindowTilemap(menu->titleWindowId);
    PutWindowTilemap(menu->nameGenderWindowId);
    PutWindowTilemap(menu->locationWindowId);
    PutWindowTilemap(menu->listIndexWindowId);
    DrawConditionMarkings();
    CopyBgTilemapBufferToVram(1);
    ShowBg(2);
}

static void FreeConditionMenuGfx(struct Pokenav_ConditionMenuGfx *menu)
{
    if (menu->markingCursorSpriteId != SPRITE_NONE)
        CloseConditionMarkings();
    if (menu->monPicSpriteId != SPRITE_NONE)
    {
        DestroySprite(&gSprites[menu->monPicSpriteId]);
        FreeSpriteTilesByTag(TAG_CONDITION_MON);
        FreeSpritePaletteByTag(TAG_CONDITION_MON);
    }
}

void FreeConditionGraphMenuSubstruct2(void)
{
    struct Pokenav_ConditionMenuGfx *menu = GetSubstructPtr(POKENAV_SUBSTRUCT_CONDITION_GRAPH_MENU_GFX);

    FreeConditionMenuGfx(menu);
    RemoveWindow(menu->nameGenderWindowId);
    RemoveWindow(menu->locationWindowId);
    RemoveWindow(menu->listIndexWindowId);
    RemoveWindow(menu->titleWindowId);
    RemoveWindow(menu->markingsWindowId);

    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_BG0_ON | DISPCNT_OBJ_1D_MAP);
    SetExitVBlank();
    FreePokenavSubstruct(POKENAV_SUBSTRUCT_CONDITION_GRAPH_MENU_GFX);
}

void MonPicGfxSpriteCallback(struct Sprite *sprite)
{
    struct Pokenav_ConditionMenuGfx *menu = GetSubstructPtr(POKENAV_SUBSTRUCT_CONDITION_GRAPH_MENU_GFX);
    sprite->x = menu->monTransitionX + 40;
}

static void CreateConditionMonPic(u8 id)
{
    struct SpriteTemplate sprTemplate;
    struct SpriteSheet sprSheet;
    struct SpritePalette sprPal;
    u8 spriteId;
    struct Pokenav_ConditionMenuGfx *menu = GetSubstructPtr(POKENAV_SUBSTRUCT_CONDITION_GRAPH_MENU_GFX);

    if (menu->monPicSpriteId == SPRITE_NONE)
    {
        LoadConditionMonPicTemplate(&sprSheet, &sprTemplate, &sprPal);
        sprSheet.data = GetConditionMonPicGfx(id);
        sprPal.data = GetConditionMonPal(id);
        menu->monPalIndex = LoadSpritePalette(&sprPal);
        menu->monGfxTileStart = LoadSpriteSheet(&sprSheet);
        spriteId = CreateSpriteUnchecked(&sprTemplate, 40, 80, 0);
        menu->monPicSpriteId = spriteId;
        if (spriteId == MAX_SPRITES)
        {
            FreeSpriteTilesByTag(TAG_CONDITION_MON);
            FreeSpritePaletteByTag(TAG_CONDITION_MON);
            menu->monPicSpriteId = SPRITE_NONE;
        }
        else
        {
            menu->monPicSpriteId = spriteId;
            gSprites[menu->monPicSpriteId].callback = MonPicGfxSpriteCallback;
            menu->monGfxPtr = (void *)VRAM + BG_VRAM_SIZE + (menu->monGfxTileStart * 32);
            menu->monPalIndex = OBJ_PLTT_ID(menu->monPalIndex);
        }
    }
    else
    {
        DmaCopy16Defvars(3, GetConditionMonPicGfx(id), menu->monGfxPtr, MON_PIC_SIZE);
        LoadPalette(GetConditionMonPal(id), menu->monPalIndex, PLTT_SIZE_4BPP);
    }
}

static void VBlankCB_PokenavConditionGraph(void)
{
    struct ConditionGraph *graph = GetConditionGraphPtr();
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
    ConditionGraph_Draw(graph);
    ScanlineEffect_InitHBlankDmaTransfer();
}

static void SetExitVBlank(void)
{
    SetPokenavVBlankCallback();
}

static void ToggleGraphData(bool8 showBg)
{
    if (showBg)
        ShowBg(2);
    else
        HideBg(2);
}

static void DoConditionGraphEnterTransition(void)
{
    struct ConditionGraph *graph = GetConditionGraphPtr();
    u8 id = GetConditionGraphMenuCurrentLoadIndex();

    ConditionGraph_SetNewPositions(graph, graph->savedPositions[CONDITION_GRAPH_LOAD_MAX - 1], graph->savedPositions[id]);
    ConditionGraph_TryUpdate(graph);
}

// Transition the graph back to empty before exiting.
// This is skipped if the player is in party mode and the cursor
// is on Cancel, in which case the graph is already empty.
static void DoConditionGraphExitTransition(void)
{
    struct ConditionGraph *graph = GetConditionGraphPtr();

    if (IsConditionMenuSearchMode() || GetConditionGraphCurrentListIndex() != GetMonListCount() - 1)
        ConditionGraph_SetNewPositions(graph, graph->savedPositions[GetConditionGraphMenuCurrentLoadIndex()], graph->savedPositions[CONDITION_GRAPH_LOAD_MAX - 1]);
}

u8 GetMonMarkingsData(void)
{
    struct Pokenav_ConditionMenuGfx *menu = GetSubstructPtr(POKENAV_SUBSTRUCT_CONDITION_GRAPH_MENU_GFX);

    if (IsConditionMenuSearchMode() == 1)
        return menu->markings;
    else
        return 0;
}
