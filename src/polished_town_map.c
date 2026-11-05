#include "global.h"
#include "bg.h"
#include "gpu_regs.h"
#include "main.h"
#include "malloc.h"
#include "menu.h"
#include "palette.h"
#include "polished_town_map.h"
#include "sound.h"
#include "sprite.h"
#include "text.h"
#include "text_window.h"
#include "window.h"
#include "constants/maps.h"
#include "constants/rgb.h"
#include "constants/songs.h"

struct PolishedLandmark
{
    u8 x, y;
    const u8 *name;
};
#include "data/polished_johto_landmarks.h"

static const u32 sTiles[] = INCBIN_U32("graphics/polished_town_map/tiles.bin");
static const u16 sMap[] = INCBIN_U16("graphics/polished_town_map/map.bin");
static const u16 sPalettes[] = INCBIN_U16("graphics/polished_town_map/palettes.bin");
static const u32 sCursor[] = INCBIN_U32("graphics/polished_town_map/cursor.bin");
static const struct BgTemplate sBgs[] = {
    {.bg = 0, .charBaseIndex = 0, .mapBaseIndex = 31, .priority = 0},
    {.bg = 1, .charBaseIndex = 1, .mapBaseIndex = 30, .priority = 1},
};
static const struct WindowTemplate sWindows[] = {
    {.bg = 0, .tilemapLeft = 20, .tilemapTop = 1, .width = 10, .height = 18, .paletteNum = 15, .baseBlock = 1},
    DUMMY_WIN_TEMPLATE,
};
static const struct OamData sCursorOam = {.shape = SPRITE_SHAPE(16x16), .size = SPRITE_SIZE(16x16), .priority = 0};
static const struct SpriteTemplate sCursorTemplate = {
    .tileTag = 0, .paletteTag = 0, .oam = &sCursorOam,
    .anims = gDummySpriteAnimTable, .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};
static EWRAM_DATA struct {
    MainCallback callback;
    u16 tilemap[1024];
    u8 selected, location, cursor, state;
} *sJohtoMap = NULL;

bool32 PolishedTownMapApplies(void)
{
    return gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(MAP_POLISHED_NEW_BARK_TOWN);
}

static void DrawMapLabel(void)
{
    static const u8 colors[] = {1, 2, 3};
    FillWindowPixelBuffer(0, PIXEL_FILL(1));
    AddTextPrinterParameterized4(0, FONT_SMALL, 4, 0, 0, 0, colors, TEXT_SKIP_DRAW, COMPOUND_STRING("JOHTO MAP"));
    AddTextPrinterParameterized4(0, FONT_SMALL, 4, 24, 0, 0, colors, TEXT_SKIP_DRAW, COMPOUND_STRING("LOCATION"));
    AddTextPrinterParameterized4(0, FONT_SMALL, 4, 40, 0, 0, colors, TEXT_SKIP_DRAW, sJohtoLandmarks[sJohtoMap->selected].name);
    AddTextPrinterParameterized4(0, FONT_SMALL, 4, 76, 0, 0, colors, TEXT_SKIP_DRAW, COMPOUND_STRING("YOU ARE IN"));
    AddTextPrinterParameterized4(0, FONT_SMALL, 4, 92, 0, 0, colors, TEXT_SKIP_DRAW, sJohtoLandmarks[sJohtoMap->location].name);
    AddTextPrinterParameterized4(0, FONT_SMALL, 4, 124, 0, 0, colors, TEXT_SKIP_DRAW, COMPOUND_STRING("{DPAD_UPDOWN} SELECT\n{B_BUTTON} BACK"));
    PutWindowTilemap(0);
    CopyWindowToVram(0, COPYWIN_FULL);
    gSprites[sJohtoMap->cursor].x = sJohtoLandmarks[sJohtoMap->selected].x;
    gSprites[sJohtoMap->cursor].y = sJohtoLandmarks[sJohtoMap->selected].y + 8;
}

static void MapVBlank(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void CB2_PolishedTownMap(void)
{
    if (!gPaletteFade.active)
    {
        if (sJohtoMap->state == 1)
        {
            MainCallback callback = sJohtoMap->callback;
            FreeAllWindowBuffers();
            FreeAllSpritePalettes();
            Free(sJohtoMap);
            sJohtoMap = NULL;
            SetMainCallback2(callback);
            return;
        }
        if (JOY_NEW(A_BUTTON | B_BUTTON))
        {
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
            sJohtoMap->state = 1;
        }
        else if (JOY_NEW(DPAD_UP | DPAD_LEFT))
        {
            sJohtoMap->selected = (sJohtoMap->selected + ARRAY_COUNT(sJohtoLandmarks) - 1) % ARRAY_COUNT(sJohtoLandmarks);
            PlaySE(SE_SELECT);
            DrawMapLabel();
        }
        else if (JOY_NEW(DPAD_DOWN | DPAD_RIGHT))
        {
            sJohtoMap->selected = (sJohtoMap->selected + 1) % ARRAY_COUNT(sJohtoLandmarks);
            PlaySE(SE_SELECT);
            DrawMapLabel();
        }
    }
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
    DoScheduledBgTilemapCopiesToVram();
}

void PolishedTownMapInit(MainCallback callback)
{
    u32 i;
    struct SpriteSheet sheet = {sCursor, sizeof(sCursor), 0};
    struct SpritePalette palette = {sPalettes, 0};
    sJohtoMap = AllocZeroed(sizeof(*sJohtoMap));
    if (sJohtoMap == NULL)
    {
        SetMainCallback2(callback);
        return;
    }
    sJohtoMap->callback = callback;
    if (gSaveBlock1Ptr->location.mapNum == MAP_NUM(MAP_POLISHED_ROUTE_29))
        sJohtoMap->location = 1;
    else if (gSaveBlock1Ptr->location.mapNum == MAP_NUM(MAP_POLISHED_ROUTE_29_46_GATE))
        sJohtoMap->location = 1;
    else if (gSaveBlock1Ptr->location.mapNum == MAP_NUM(MAP_POLISHED_ROUTE_31)
          || gSaveBlock1Ptr->location.mapNum == MAP_NUM(MAP_POLISHED_ROUTE_31_VIOLET_GATE))
        sJohtoMap->location = 5;
    else if (gSaveBlock1Ptr->location.mapNum == MAP_NUM(MAP_POLISHED_VIOLET_CITY)
          || gSaveBlock1Ptr->location.mapNum == MAP_NUM(MAP_POLISHED_VIOLET_POKECENTER_1F)
          || gSaveBlock1Ptr->location.mapNum == MAP_NUM(MAP_POLISHED_EARLS_POKEMON_ACADEMY)
          || gSaveBlock1Ptr->location.mapNum == MAP_NUM(MAP_POLISHED_VIOLET_MART))
        sJohtoMap->location = 6;
    else if (gSaveBlock1Ptr->location.mapNum >= MAP_NUM(MAP_POLISHED_ROUTE_30))
        sJohtoMap->location = 4;
    else if (gSaveBlock1Ptr->location.mapNum >= MAP_NUM(MAP_POLISHED_CHERRYGROVE_CITY))
        sJohtoMap->location = 2;
    sJohtoMap->selected = sJohtoMap->location;
    SetVBlankCallback(NULL);
    SetGpuReg(REG_OFFSET_DISPCNT, 0);
    for (i = REG_OFFSET_BG0HOFS; i <= REG_OFFSET_BG3VOFS; i += 2)
        SetGpuReg(i, 0);
    ResetSpriteData();
    FreeAllSpritePalettes();
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sBgs, ARRAY_COUNT(sBgs));
    InitWindows(sWindows);
    DeactivateAllTextPrinters();
    ClearScheduledBgCopiesToVram();
    memcpy(sJohtoMap->tilemap, sMap, sizeof(sMap));
    SetBgTilemapBuffer(1, sJohtoMap->tilemap);
    LoadBgTiles(1, sTiles, sizeof(sTiles), 0);
    CopyBgTilemapBufferToVram(1);
    LoadPalette(sPalettes, 0, sizeof(sPalettes));
    LoadPalette(GetTextWindowPalette(0), BG_PLTT_ID(15), PLTT_SIZE_4BPP);
    LoadSpriteSheet(&sheet);
    LoadSpritePalette(&palette);
    sJohtoMap->cursor = CreateSprite(&sCursorTemplate, 0, 0, 0);
    DrawMapLabel();
    BlendPalettes(PALETTES_ALL, 16, RGB_BLACK);
    BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
    ShowBg(0);
    ShowBg(1);
    SetGpuRegBits(REG_OFFSET_DISPCNT, DISPCNT_OBJ_1D_MAP | DISPCNT_OBJ_ON);
    SetVBlankCallback(MapVBlank);
    SetMainCallback2(CB2_PolishedTownMap);
}
