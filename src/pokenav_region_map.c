#include "global.h"
#include "bg.h"
#include "decompress.h"
#include "landmark.h"
#include "event_data.h"
#include "field_effect.h"
#include "main.h"
#include "menu.h"
#include "overworld.h"
#include "palette.h"
#include "pokenav.h"
#include "region_map.h"
#include "sound.h"
#include "sprite.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "window.h"
#include "constants/rgb.h"
#include "constants/songs.h"
#include "constants/region_map_sections.h"


#define NUM_CITY_MAPS 22

struct Pokenav_RegionMapMenu
{
    u8 unused[12];
    bool32 zoomDisabled;
    u32 (*callback)(struct Pokenav_RegionMapMenu *);
};

struct Pokenav_RegionMapGfx
{
    bool32 (*isTaskActiveCB)(void);
    u32 loopTaskId;
    u16 infoWindowId;
    u8 ALIGNED(2) tilemapBuffer[BG_SCREEN_SIZE];
    u8 cityZoomPics[NUM_CITY_MAPS][200];
};

struct CityMapEntry
{
    mapsec_u16_t mapSecId;
    u16 index;
    const u32 *tilemap;
};

static u32 HandleRegionMapInput(struct Pokenav_RegionMapMenu *);
static u32 HandleRegionMapInputZoomDisabled(struct Pokenav_RegionMapMenu *);
static u32 GetExitRegionMapMenuId(struct Pokenav_RegionMapMenu *);
static u32 LoopedTask_OpenRegionMap(s32);
static u32 LoopedTask_DecompressCityMaps(s32);
static bool32 GetCurrentLoopedTaskActive(void);
static void DecompressCityMaps(void);
static bool32 IsDecompressCityMapsActive(void);
static void LoadPokenavRegionMapGfx(struct Pokenav_RegionMapGfx *);
static bool32 TryFreeTempTileDataBuffers(void);
static void UpdateMapSecInfoWindow(struct Pokenav_RegionMapGfx *);
static void PrintHgssMapWindowText(u8 windowId, const u8 *text, u8 x, u8 y);
static void RestoreHgssMapSub2DetailRect(struct Pokenav_RegionMapGfx *, u8, u8, u8, u8);
static bool32 IsDma3ManagerBusyWithBgCopy_(struct Pokenav_RegionMapGfx *);
static void ChangeBgYForZoom(bool32);
static bool32 IsChangeBgYForZoomActive(void);
static void DrawCityMap(struct Pokenav_RegionMapGfx *, mapsec_s32_t, int);
static void PrintLandmarkNames(struct Pokenav_RegionMapGfx *, mapsec_s32_t, int);
static void Task_ChangeBgYForZoom(u8 taskId);
static u32 LoopedTask_UpdateInfoAfterCursorMove(s32);
static u32 LoopedTask_RegionMapZoomOut(s32);
static u32 LoopedTask_RegionMapZoomIn(s32);
static u32 LoopedTask_ExitRegionMap(s32);
static u32 LoopedTask_TreatAsPokeNavFlyMap(s32);

extern const u16 gRegionMapCityZoomTiles_Pal[];

static const u16 sHgssPokeGearMapWindow_Pal[] = INCBIN_U16("graphics/gen4_ui/hgss_pokegear/map_window.gbapal");
static const u32 sRegionMapCityZoomTiles_Gfx[] = INCGFX_U32("graphics/pokenav/region_map/zoom_tiles.png", ".4bpp.smol");

#define HGSS_POKEGEAR_MAP_MAIN1_TILE_BASE 0x1C0
#define HGSS_POKEGEAR_MAP_MAIN1_PAL_BANK_A 5
#define HGSS_POKEGEAR_MAP_MAIN1_PAL_BANK_B 6
static const u32 sHgssPokeGearMapMain1_Gfx[] = INCBIN_U32("graphics/gen4_ui/hgss_pokegear/map_main1.4bpp");
static const u16 sHgssPokeGearMapMain1_Tilemap[] = INCBIN_U16("graphics/gen4_ui/hgss_pokegear/map_main1.tilemap.bin");
static const u16 sHgssPokeGearMapMain1_Pal[] = INCBIN_U16("graphics/gen4_ui/hgss_pokegear/map_main1.gbapal");

#define HGSS_POKEGEAR_MAP_SUB2_TILE_BASE 0x180
#define HGSS_POKEGEAR_MAP_SUB2_PAL_BANK 2
#define HGSS_POKEGEAR_MAP_SUB2_SOURCE_X 16
#define HGSS_POKEGEAR_MAP_SUB2_SOURCE_Y 8
#define HGSS_POKEGEAR_MAP_SUB2_PANEL_WIDTH 16
#define HGSS_POKEGEAR_MAP_SUB2_PANEL_HEIGHT 16
#define HGSS_POKEGEAR_MAP_SUB2_DEST_X 14
#define HGSS_POKEGEAR_MAP_SUB2_DEST_Y 2
static const u32 sHgssPokeGearMapSub2_Gfx[] = INCBIN_U32("graphics/gen4_ui/hgss_pokegear/map_sub2.4bpp");
static const u16 sHgssPokeGearMapSub2_Tilemap[] = INCBIN_U16("graphics/gen4_ui/hgss_pokegear/map_sub2.tilemap.bin");
static const u16 sHgssPokeGearMapSub2_Pal[] = INCBIN_U16("graphics/gen4_ui/hgss_pokegear/map_sub2.gbapal");

// MAIN_1 remains the live retail frame. SUB_2 supplies the authentic
// right-side detail-panel chrome, cropped from the retail 256px DS surface to
// fit the 240px GBA viewport.

#include "data/region_map/city_map_tilemaps.h"

static const struct BgTemplate sRegionMapBgTemplates[3] =
{
    {
        .bg = 1,
        .charBaseIndex = 1,
        .mapBaseIndex = 0x1F,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0
    },
    {
        .bg = 2,
        .charBaseIndex = 2,
        .mapBaseIndex = 0x06,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 2,
        .baseTile = 0
    },
    {
        .bg = 2,
        .charBaseIndex = 0,
        .mapBaseIndex = 0x00,
        .screenSize = 2,
        .paletteMode = 0,
        .priority = 3,
        .baseTile = 0
    },
};

static const LoopedTask sRegionMapLoopTaskFuncs[] =
{
    [POKENAV_MAP_FUNC_NONE]         = NULL,
    [POKENAV_MAP_FUNC_CURSOR_MOVED] = LoopedTask_UpdateInfoAfterCursorMove,
    [POKENAV_MAP_FUNC_ZOOM_OUT]     = LoopedTask_RegionMapZoomOut,
    [POKENAV_MAP_FUNC_ZOOM_IN]      = LoopedTask_RegionMapZoomIn,
    [POKENAV_MAP_FUNC_EXIT]         = LoopedTask_ExitRegionMap,
    [POKENAV_MAP_FUNC_FLY]          = LoopedTask_TreatAsPokeNavFlyMap,
};

static const struct WindowTemplate sMapSecInfoWindowTemplate =
{
    .bg = 1,
    .tilemapLeft = 17,
    .tilemapTop = 4,
    .width = 12,
    .height = 13,
    .paletteNum = 1,
    .baseBlock = 0x4C
};

#include "data/region_map/city_map_entries.h"

u32 PokenavCallback_Init_RegionMap(void)
{
    struct Pokenav_RegionMapMenu *state = AllocSubstruct(POKENAV_SUBSTRUCT_REGION_MAP_STATE, sizeof(struct Pokenav_RegionMapMenu));
    if (!state)
        return FALSE;

    if (!AllocSubstruct(POKENAV_SUBSTRUCT_REGION_MAP, sizeof(struct RegionMap)))
        return FALSE;

    state->zoomDisabled = IsEventIslandMapSecId(gMapHeader.regionMapSectionId);
    if (!state->zoomDisabled)
        state->callback = HandleRegionMapInput;
    else
        state->callback = HandleRegionMapInputZoomDisabled;

    return TRUE;
}

void FreeRegionMapSubstruct1(void)
{
    gSaveBlock2Ptr->regionMapZoom = IsRegionMapZoomed();
    FreePokenavSubstruct(POKENAV_SUBSTRUCT_REGION_MAP);
    FreePokenavSubstruct(POKENAV_SUBSTRUCT_REGION_MAP_STATE);
}

u32 GetRegionMapCallback(void)
{
    struct Pokenav_RegionMapMenu *state = GetSubstructPtr(POKENAV_SUBSTRUCT_REGION_MAP_STATE);
    return state->callback(state);
}

static u32 HandleRegionMapInput(struct Pokenav_RegionMapMenu *state)
{
    struct RegionMap* regionMap = GetSubstructPtr(POKENAV_SUBSTRUCT_REGION_MAP);

    switch (DoRegionMapInputCallback())
    {
    case MAP_INPUT_MOVE_END:
        return POKENAV_MAP_FUNC_CURSOR_MOVED;
    case MAP_INPUT_A_BUTTON:
        if (!IsRegionMapZoomed())
            return POKENAV_MAP_FUNC_ZOOM_IN;
        return POKENAV_MAP_FUNC_ZOOM_OUT;
    case MAP_INPUT_B_BUTTON:
        state->callback = GetExitRegionMapMenuId;
        return POKENAV_MAP_FUNC_EXIT;
    case MAP_INPUT_R_BUTTON:
        if (regionMap->mapSecType == MAPSECTYPE_CITY_CANFLY && FlagGet(OW_FLAG_POKE_RIDER)
        && Overworld_MapTypeAllowsTeleportAndFly(gMapHeader.mapType) == TRUE)
            return POKENAV_MAP_FUNC_FLY;
    }

    return POKENAV_MAP_FUNC_NONE;
}

static u32 HandleRegionMapInputZoomDisabled(struct Pokenav_RegionMapMenu *state)
{
    if (JOY_NEW(B_BUTTON))
    {
        state->callback = GetExitRegionMapMenuId;
        return POKENAV_MAP_FUNC_EXIT;
    }

    return POKENAV_MAP_FUNC_NONE;
}

static u32 GetExitRegionMapMenuId(struct Pokenav_RegionMapMenu *state)
{
    return POKENAV_MAIN_MENU_CURSOR_ON_MAP;
}

bool32 GetZoomDisabled(void)
{
    struct Pokenav_RegionMapMenu *state = GetSubstructPtr(POKENAV_SUBSTRUCT_REGION_MAP_STATE);
    return state->zoomDisabled;
}

bool32 OpenPokenavRegionMap(void)
{
    struct Pokenav_RegionMapGfx *state = AllocSubstruct(POKENAV_SUBSTRUCT_REGION_MAP_ZOOM, sizeof(struct Pokenav_RegionMapGfx));
    if (!state)
        return FALSE;

    state->loopTaskId = CreateLoopedTask(LoopedTask_OpenRegionMap, 1);
    state->isTaskActiveCB = GetCurrentLoopedTaskActive;
    return TRUE;
}

void CreateRegionMapLoopedTask(s32 index)
{
    struct Pokenav_RegionMapGfx *state = GetSubstructPtr(POKENAV_SUBSTRUCT_REGION_MAP_ZOOM);
    state->loopTaskId = CreateLoopedTask(sRegionMapLoopTaskFuncs[index], 1);
    state->isTaskActiveCB = GetCurrentLoopedTaskActive;
}

bool32 IsRegionMapLoopedTaskActive(void)
{
    struct Pokenav_RegionMapGfx *state = GetSubstructPtr(POKENAV_SUBSTRUCT_REGION_MAP_ZOOM);
    return state->isTaskActiveCB();
}

void FreeRegionMapSubstruct2(void)
{
    struct Pokenav_RegionMapGfx *state = GetSubstructPtr(POKENAV_SUBSTRUCT_REGION_MAP_ZOOM);
    FreeRegionMapIconResources();
    RemoveWindow(state->infoWindowId);
    FreePokenavSubstruct(POKENAV_SUBSTRUCT_REGION_MAP);
    FreePokenavSubstruct(POKENAV_SUBSTRUCT_REGION_MAP_ZOOM);
    SetPokenavVBlankCallback();
    SetBgMode(0);
}

static void VBlankCB_RegionMap(void)
{
    TransferPlttBuffer();
    LoadOam();
    ProcessSpriteCopyRequests();
    UpdateRegionMapVideoRegs();
}

static bool32 GetCurrentLoopedTaskActive(void)
{
    struct Pokenav_RegionMapGfx *state = GetSubstructPtr(POKENAV_SUBSTRUCT_REGION_MAP_ZOOM);
    return IsLoopedTaskActive(state->loopTaskId);
}

static bool8 ShouldOpenRegionMapZoomed(void)
{
    if (GetZoomDisabled())
        return FALSE;

    return gSaveBlock2Ptr->regionMapZoom == TRUE;
}

static u32 LoopedTask_OpenRegionMap(s32 taskState)
{
    struct RegionMap *regionMap;
    struct Pokenav_RegionMapGfx *state = GetSubstructPtr(POKENAV_SUBSTRUCT_REGION_MAP_ZOOM);
    switch (taskState)
    {
    case 0:
        SetVBlankCallback_(NULL);
        HideBg(1);
        HideBg(2);
        HideBg(3);
        SetBgMode(1);
        InitBgTemplates(sRegionMapBgTemplates, ARRAY_COUNT(sRegionMapBgTemplates) - 1);
        regionMap = GetSubstructPtr(POKENAV_SUBSTRUCT_REGION_MAP);
        InitRegionMapData(regionMap, &sRegionMapBgTemplates[1], ShouldOpenRegionMapZoomed());
        return LT_INC_AND_PAUSE;
    case 1:
        if (LoadRegionMapGfx())
            return LT_PAUSE;

        if (!GetZoomDisabled())
        {
            CreateRegionMapPlayerIcon(4, 9);
            CreateRegionMapCursor(5, 10);
            TrySetPlayerIconBlink();
            if (regionMap->playerIconSprite != NULL)
                regionMap->playerIconSprite->oam.priority = 0;
        }
        else
        {
            // Dim the region map when zoom is disabled
            // (when the player is off the map)
            BlendRegionMap(RGB_BLACK, 6);
        }
        return LT_INC_AND_PAUSE;
    case 2:
        DecompressCityMaps();
        return LT_INC_AND_CONTINUE;
    case 3:
        if (IsDecompressCityMapsActive())
            return LT_PAUSE;

        LoadPokenavRegionMapGfx(state);
        return LT_INC_AND_CONTINUE;
    case 4:
        if (TryFreeTempTileDataBuffers())
            return LT_PAUSE;

        UpdateMapSecInfoWindow(state);
        FadeToBlackExceptPrimary();
        return LT_INC_AND_PAUSE;
    case 5:
        if (IsDma3ManagerBusyWithBgCopy_(state))
            return LT_PAUSE;

        ShowBg(1);
        ShowBg(2);
        SetVBlankCallback_(VBlankCB_RegionMap);
        return LT_INC_AND_PAUSE;
    case 6:
        UpdateRegionMapHelpBarText();
        PokenavFadeScreen(POKENAV_FADE_FROM_BLACK);
        return LT_INC_AND_PAUSE;
    case 7:
        if (IsPaletteFadeActive())
            return LT_PAUSE;
        return LT_INC_AND_CONTINUE;
    default:
        return LT_FINISH;
    }
}

static u32 LoopedTask_UpdateInfoAfterCursorMove(s32 taskState)
{
    struct Pokenav_RegionMapGfx *state = GetSubstructPtr(POKENAV_SUBSTRUCT_REGION_MAP_ZOOM);
    switch (taskState)
    {
    case 0:
        UpdateMapSecInfoWindow(state);
        UpdateRegionMapHelpBarText();
        return LT_INC_AND_PAUSE;
    case 1:
        if (IsDma3ManagerBusyWithBgCopy_(state))
            return LT_PAUSE;
        break;
    }

    return LT_FINISH;
}

static u32 LoopedTask_RegionMapZoomOut(s32 taskState)
{
    switch (taskState)
    {
    case 0:
        PlaySE(SE_SELECT);
        ChangeBgYForZoom(FALSE);
        SetRegionMapDataForZoom();
        return LT_INC_AND_PAUSE;
    case 1:
        if (UpdateRegionMapZoom() || IsChangeBgYForZoomActive())
            return LT_PAUSE;

        UpdateRegionMapHelpBarText();
        return LT_INC_AND_PAUSE;
    case 2:
        if (WaitForHelpBar())
            return LT_PAUSE;

        break;
    }

    return LT_FINISH;
}

static u32 LoopedTask_RegionMapZoomIn(s32 taskState)
{
    struct Pokenav_RegionMapGfx *state = GetSubstructPtr(POKENAV_SUBSTRUCT_REGION_MAP_ZOOM);
    switch (taskState)
    {
    case 0:
        PlaySE(SE_SELECT);
        UpdateMapSecInfoWindow(state);
        return LT_INC_AND_PAUSE;
    case 1:
        if (IsDma3ManagerBusyWithBgCopy_(state))
            return LT_PAUSE;

        ChangeBgYForZoom(TRUE);
        SetRegionMapDataForZoom();
        return LT_INC_AND_PAUSE;
    case 2:
        if (UpdateRegionMapZoom() || IsChangeBgYForZoomActive())
            return LT_PAUSE;

        UpdateRegionMapHelpBarText();
        return LT_INC_AND_PAUSE;
    case 3:
        if (WaitForHelpBar())
            return LT_PAUSE;

        break;
    }

    return LT_FINISH;
}

static u32 LoopedTask_ExitRegionMap(s32 taskState)
{
    switch (taskState)
    {
    case 0:
        PlaySE(SE_SELECT);
        PokenavFadeScreen(POKENAV_FADE_TO_BLACK);
        return LT_INC_AND_PAUSE;
    case 1:
        if (IsPaletteFadeActive())
            return LT_PAUSE;

        SlideMenuHeaderDown();
        return LT_INC_AND_PAUSE;
    case 2:
        if (MainMenuLoopedTaskIsBusy())
            return LT_PAUSE;

        HideBg(1);
        HideBg(2);
        HideBg(3);
        return LT_INC_AND_PAUSE;
    }

    return LT_FINISH;
}

static u32 LoopedTask_TreatAsPokeNavFlyMap(s32 taskState)
{
    switch (taskState)
    {
    case 0:
        PlaySE(SE_SELECT);
        struct RegionMap* regionMap = GetSubstructPtr(POKENAV_SUBSTRUCT_REGION_MAP);
        SetFlyDestination(regionMap);
        gSkipShowMonAnim = TRUE;
        ReturnToFieldFromFlyMapSelect();

        return LT_FINISH;
    }

    return LT_FINISH;
}



static void LoadPokenavRegionMapGfx(struct Pokenav_RegionMapGfx *state)
{
    // MAIN_1 is the live retail frame: its transparent tile-0 opening exposes
    // the affine region map on BG2. SUB_2 supplies the retail detail surface.
    LoadBgTiles(1, sHgssPokeGearMapMain1_Gfx, sizeof(sHgssPokeGearMapMain1_Gfx), HGSS_POKEGEAR_MAP_MAIN1_TILE_BASE);
    CopyPaletteIntoBufferUnfaded(sHgssPokeGearMapMain1_Pal, BG_PLTT_ID(HGSS_POKEGEAR_MAP_MAIN1_PAL_BANK_A), PLTT_SIZE_4BPP);
    CopyPaletteIntoBufferUnfaded(&sHgssPokeGearMapMain1_Pal[16], BG_PLTT_ID(HGSS_POKEGEAR_MAP_MAIN1_PAL_BANK_B), PLTT_SIZE_4BPP);
    LoadBgTiles(1, sHgssPokeGearMapSub2_Gfx, sizeof(sHgssPokeGearMapSub2_Gfx), HGSS_POKEGEAR_MAP_SUB2_TILE_BASE);
    CopyPaletteIntoBufferUnfaded(sHgssPokeGearMapSub2_Pal, BG_PLTT_ID(HGSS_POKEGEAR_MAP_SUB2_PAL_BANK), PLTT_SIZE_4BPP);

    CpuCopy16(sHgssPokeGearMapMain1_Tilemap, state->tilemapBuffer, sizeof(sHgssPokeGearMapMain1_Tilemap));

    // Retail SUB_2 is 256px wide. Keep its 16x16 right-side detail surface
    // exactly, shifted two tiles left so the complete chrome fits the 240px
    // GBA viewport. The dynamic Emerald/custom-region data remains separate.
    for (int y = 0; y < HGSS_POKEGEAR_MAP_SUB2_PANEL_HEIGHT; y++)
    {
        CpuCopy16(
            &sHgssPokeGearMapSub2_Tilemap[(HGSS_POKEGEAR_MAP_SUB2_SOURCE_Y + y) * 32 + HGSS_POKEGEAR_MAP_SUB2_SOURCE_X],
            &((u16 *)state->tilemapBuffer)[(HGSS_POKEGEAR_MAP_SUB2_DEST_Y + y) * 32 + HGSS_POKEGEAR_MAP_SUB2_DEST_X],
            HGSS_POKEGEAR_MAP_SUB2_PANEL_WIDTH * sizeof(u16));
    }

    SetBgTilemapBuffer(1, state->tilemapBuffer);
    state->infoWindowId = AddWindow(&sMapSecInfoWindowTemplate);
    DecompressAndCopyTileDataToVram(1, sRegionMapCityZoomTiles_Gfx, 0, 0, 0);
    FillWindowPixelBuffer(state->infoWindowId, PIXEL_FILL(0));
    PutWindowTilemap(state->infoWindowId);
    CopyWindowToVram(state->infoWindowId, COPYWIN_FULL);
    CopyPaletteIntoBufferUnfaded(sHgssPokeGearMapWindow_Pal, BG_PLTT_ID(1), sizeof(sHgssPokeGearMapWindow_Pal));
    CopyPaletteIntoBufferUnfaded(gRegionMapCityZoomTiles_Pal, BG_PLTT_ID(3), PLTT_SIZE_4BPP);
    if (!IsRegionMapZoomed())
        ChangeBgY(1, -0x6000, BG_COORD_SET);
    else
        ChangeBgY(1, 0, BG_COORD_SET);

    ChangeBgX(1, 0, BG_COORD_SET);
}

static bool32 TryFreeTempTileDataBuffers(void)
{
    return FreeTempTileDataBuffersIfPossible();
}

static void RestoreHgssMapSub2DetailRect(struct Pokenav_RegionMapGfx *state, u8 x, u8 y, u8 width, u8 height)
{
    const u8 sourceX = HGSS_POKEGEAR_MAP_SUB2_SOURCE_X + x - HGSS_POKEGEAR_MAP_SUB2_DEST_X;
    const u8 sourceY = HGSS_POKEGEAR_MAP_SUB2_SOURCE_Y + y - HGSS_POKEGEAR_MAP_SUB2_DEST_Y;

    for (u8 row = 0; row < height; row++)
    {
        CpuCopy16(
            &sHgssPokeGearMapSub2_Tilemap[(sourceY + row) * 32 + sourceX],
            &((u16 *)state->tilemapBuffer)[(y + row) * 32 + x],
            width * sizeof(u16));
    }
}

static void PrintHgssMapWindowText(u8 windowId, const u8 *text, u8 x, u8 y)
{
    struct TextPrinterTemplate printer;

    printer.currentChar = text;
    printer.type = WINDOW_TEXT_PRINTER;
    printer.windowId = windowId;
    printer.fontId = FONT_NARROW;
    printer.x = x;
    printer.y = y;
    printer.currentX = x;
    printer.currentY = y;
    printer.letterSpacing = gFonts[FONT_NARROW].letterSpacing;
    printer.lineSpacing = gFonts[FONT_NARROW].lineSpacing;

    // Retail HGSS Map windows use MAKE_TEXT_COLOR(1, 2, 0):
    // foreground 1, shadow 2, background 0.
    printer.color.background = 0;
    printer.color.foreground = 1;
    printer.color.shadow = 2;
    printer.color.accent = 0;

    AddTextPrinter(&printer, TEXT_SKIP_DRAW, NULL);
}

static void UpdateMapSecInfoWindow(struct Pokenav_RegionMapGfx *state)
{
    struct RegionMap *regionMap = GetSubstructPtr(POKENAV_SUBSTRUCT_REGION_MAP);
    FillWindowPixelBuffer(state->infoWindowId, PIXEL_FILL(0));
    switch (regionMap->mapSecType)
    {
    case MAPSECTYPE_CITY_CANFLY:
        PutWindowRectTilemap(state->infoWindowId, 0, 0, 12, 2);
        PrintHgssMapWindowText(state->infoWindowId, regionMap->mapSecName, 4, 1);
        DrawCityMap(state, regionMap->mapSecId, regionMap->posWithinMapSec);
        CopyWindowToVram(state->infoWindowId, COPYWIN_FULL);
        break;
    case MAPSECTYPE_CITY_CANTFLY:
        PutWindowRectTilemap(state->infoWindowId, 0, 0, 12, 2);
        PrintHgssMapWindowText(state->infoWindowId, regionMap->mapSecName, 4, 1);
        RestoreHgssMapSub2DetailRect(state, 17, 6, 12, 11);
        CopyWindowToVram(state->infoWindowId, COPYWIN_FULL);
        break;
    case MAPSECTYPE_ROUTE:
    case MAPSECTYPE_BATTLE_FRONTIER:
        PutWindowTilemap(state->infoWindowId);
        PrintHgssMapWindowText(state->infoWindowId, regionMap->mapSecName, 4, 1);
        PrintLandmarkNames(state, regionMap->mapSecId, regionMap->posWithinMapSec);
        CopyWindowToVram(state->infoWindowId, COPYWIN_FULL);
        break;
    case MAPSECTYPE_NONE:
        RestoreHgssMapSub2DetailRect(state, 17, 4, 12, 13);
        CopyBgTilemapBufferToVram(1);
        break;
    }
}

static bool32 IsDma3ManagerBusyWithBgCopy_(struct Pokenav_RegionMapGfx *state)
{
    return IsDma3ManagerBusyWithBgCopy();
}

#define tZoomIn data[0]

static void ChangeBgYForZoom(bool32 zoomIn)
{
    u8 taskId = CreateTask(Task_ChangeBgYForZoom, 3);
    gTasks[taskId].tZoomIn = zoomIn;
}

static bool32 IsChangeBgYForZoomActive(void)
{
    return FuncIsActiveTask(Task_ChangeBgYForZoom);
}

static void Task_ChangeBgYForZoom(u8 taskId)
{
    if (gTasks[taskId].tZoomIn)
    {
        if (ChangeBgY(1, 0x480, BG_COORD_ADD) >= 0)
        {
            ChangeBgY(1, 0, BG_COORD_SET);
            DestroyTask(taskId);
        }

    }
    else
    {
        if (ChangeBgY(1, 0x480, BG_COORD_SUB) <= -0x6000)
        {
            ChangeBgY(1, -0x6000, BG_COORD_SET);
            DestroyTask(taskId);
        }

    }
}

#undef tZoomIn

static void DecompressCityMaps(void)
{
    CreateLoopedTask(LoopedTask_DecompressCityMaps, 1);
}

static bool32 IsDecompressCityMapsActive(void)
{
    return FuncIsActiveLoopedTask(LoopedTask_DecompressCityMaps);
}

static u32 LoopedTask_DecompressCityMaps(s32 taskState)
{
    struct Pokenav_RegionMapGfx *state = GetSubstructPtr(POKENAV_SUBSTRUCT_REGION_MAP_ZOOM);
    if (taskState < NUM_CITY_MAPS)
    {
        DecompressDataWithHeaderWram(sPokenavCityMaps[taskState].tilemap, state->cityZoomPics[taskState]);
        return LT_INC_AND_CONTINUE;
    }

    return LT_FINISH;
}

static void DrawCityMap(struct Pokenav_RegionMapGfx *state, mapsec_s32_t mapSecId, int pos)
{
    int i;
    for (i = 0; i < NUM_CITY_MAPS && (sPokenavCityMaps[i].mapSecId != mapSecId || sPokenavCityMaps[i].index != pos); i++)
        ;

    if (i == NUM_CITY_MAPS)
        return;

    RestoreHgssMapSub2DetailRect(state, 17, 6, 12, 11);
    CopyToBgTilemapBufferRect(1, state->cityZoomPics[i], 18, 6, 10, 10);
}

static void PrintLandmarkNames(struct Pokenav_RegionMapGfx *state, mapsec_s32_t mapSecId, int pos)
{
    int i = 0;
    while (1)
    {
        const u8 *landmarkName = GetLandmarkName(mapSecId, pos, i);
        if (!landmarkName)
            break;

        StringCopyPadded(gStringVar1, landmarkName, CHAR_SPACE, 12);
        PrintHgssMapWindowText(state->infoWindowId, gStringVar1, 4, i * 16 + 17);
        i++;
    }
}

void UpdateRegionMapHelpBarText(void)
{
    struct RegionMap* regionMap = GetSubstructPtr(POKENAV_SUBSTRUCT_REGION_MAP);

    if (regionMap->mapSecType == MAPSECTYPE_CITY_CANFLY && FlagGet(OW_FLAG_POKE_RIDER)
        && Overworld_MapTypeAllowsTeleportAndFly(gMapHeader.mapType) == TRUE)
    {
        if (IsRegionMapZoomed())
            PrintHelpBarText(HELPBAR_MAP_ZOOMED_IN_CANFLY);
        else
            PrintHelpBarText(HELPBAR_MAP_ZOOMED_OUT_CANFLY);
    }
    else
    {
        if (IsRegionMapZoomed())
            PrintHelpBarText(HELPBAR_MAP_ZOOMED_IN);
        else
            PrintHelpBarText(HELPBAR_MAP_ZOOMED_OUT);
    }
}
