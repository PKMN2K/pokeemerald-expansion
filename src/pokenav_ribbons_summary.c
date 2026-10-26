#include "global.h"
#include "bg.h"
#include "decompress.h"
#include "dynamic_placeholder_text_util.h"
#include "graphics.h"
#include "international_string_util.h"
#include "palette.h"
#include "pokenav.h"
#include "sound.h"
#include "sprite.h"
#include "string_util.h"
#include "strings.h"
#include "text.h"
#include "trainer_pokemon_sprites.h"
#include "window.h"
#include "constants/songs.h"

enum
{
    RIBBONS_SUMMARY_FUNC_NONE,
    RIBBONS_SUMMARY_FUNC_SWITCH_MONS,
    RIBBONS_SUMMARY_FUNC_SELECT_RIBBON,
    RIBBONS_SUMMARY_FUNC_EXPANDED_CURSOR_MOVE,
    RIBBONS_SUMMARY_FUNC_EXPANDED_CANCEL,
    RIBBONS_SUMMARY_FUNC_EXIT,
};

#define GFXTAG_RIBBON_ICONS_BIG 9
#define GFXTAG_HGSS_RIBBON_CURSOR 0xF120
#define PALTAG_HGSS_RIBBON_CURSOR 0xF120
#define HGSS_RIBBON_CURSOR_CORNERS 4

#define PALTAG_RIBBON_ICONS_1 15
#define PALTAG_RIBBON_ICONS_2 16
#define PALTAG_RIBBON_ICONS_3 17
#define PALTAG_RIBBON_ICONS_4 18
#define PALTAG_RIBBON_ICONS_5 19

// Native HGSS ribbon icons are 32x32. A four-column, two-row page
// fits the existing ribbon-grid viewport at x=88..216, y=32..96.
#define RIBBONS_PER_ROW 4
#define RIBBON_GRID_ROWS 2
#define RIBBONS_PER_PAGE (RIBBONS_PER_ROW * RIBBON_GRID_ROWS)
#define RIBBON_ICON_TILES 4
#define RIBBON_GRID_LEFT_TILE 11
#define RIBBON_GRID_TOP_TILE 4
#define GIFT_RIBBON_ROW (1 + (FIRST_GIFT_RIBBON / RIBBONS_PER_ROW))
#define GIFT_RIBBON_START_POS (RIBBONS_PER_ROW * GIFT_RIBBON_ROW)

#define MON_SPRITE_X_ON  40
#define MON_SPRITE_X_OFF -32
#define MON_SPRITE_Y     104

static const u8 gText_RibbonsF700[] = _("RIBBONS {DYNAMIC 0}");

struct Pokenav_RibbonsSummaryList
{
    u8 unused1[8];
    struct PokenavMonList *monList;
    u16 selectedPos;
    u16 normalRibbonLastRowStart;
    u16 numNormalRibbons;
    u16 numGiftRibbons;
    u32 ribbonIds[FIRST_GIFT_RIBBON];
    u32 giftRibbonIds[NUM_GIFT_RIBBONS];
    u32 unused2;
    u32 (*callback)(struct Pokenav_RibbonsSummaryList *);
};

struct Pokenav_RibbonsSummaryMenu
{
    u32 (*callback)(void);
    u32 loopedTaskId;
    u16 nameWindowId;
    u16 ribbonCountWindowId;
    u16 listIdxWindowId;
    u16 gridFrameWindowId;
    u16 unusedWindowId;
    u16 monSpriteId;
    struct Sprite *bigRibbonSprite;
    struct Sprite *hgssCursorCorners[HGSS_RIBBON_CURSOR_CORNERS];
    u32 unused;
    u8 tilemapBuffers[2][BG_SCREEN_SIZE];
};

// Used for iterating over the ribbons on the current page.
static u32 sRibbonDraw_Total;

static void PrintCurrentMonRibbonCount(struct Pokenav_RibbonsSummaryMenu *);
static void PrintRibbbonsSummaryMonInfo(struct Pokenav_RibbonsSummaryMenu *);
static void PrintRibbonsMonListIndex(struct Pokenav_RibbonsSummaryMenu *);
static void DrawPokeGearRibbonGridFrame(u16);
static void DrawPokeGearRibbonSelectionFocus(struct Pokenav_RibbonsSummaryMenu *, bool8);
static void GetHgssRibbonScreenPosition(u32, s16 *, s16 *);
static void CreateHgssRibbonCursor(struct Pokenav_RibbonsSummaryMenu *);
static void DestroyHgssRibbonCursor(struct Pokenav_RibbonsSummaryMenu *);
static void DrawPokeGearRibbonDetailPanel(u16);
static void DrawPokeGearRibbonMonCard(u16);
static void DrawPokeGearRibbonIndexCard(u16);
static void ZoomOutSelectedRibbon(struct Pokenav_RibbonsSummaryMenu *);
static void UpdateAndZoomInSelectedRibbon(struct Pokenav_RibbonsSummaryMenu *);
static void PrintRibbonNameAndDescription(struct Pokenav_RibbonsSummaryMenu *);
static void ResetSpritesAndDrawMonFrontPic(struct Pokenav_RibbonsSummaryMenu *);
static void AddRibbonListIndexWindow(struct Pokenav_RibbonsSummaryMenu *);
static void DestroyRibbonsMonFrontPic(struct Pokenav_RibbonsSummaryMenu *);
static void SlideMonSpriteOff(struct Pokenav_RibbonsSummaryMenu *);
static void SlideMonSpriteOn(struct Pokenav_RibbonsSummaryMenu *);
static void AddRibbonCountWindow(struct Pokenav_RibbonsSummaryMenu *);
static void AddRibbonGridFrameWindow(struct Pokenav_RibbonsSummaryMenu *);
static void CreateBigRibbonSprite(struct Pokenav_RibbonsSummaryMenu *);
static void AddRibbonSummaryMonNameWindow(struct Pokenav_RibbonsSummaryMenu *);
static void DrawAllRibbonsHgss(struct Pokenav_RibbonsSummaryMenu *);
static bool32 IsRibbonAnimating(struct Pokenav_RibbonsSummaryMenu *);
static bool32 IsMonSpriteAnimating(struct Pokenav_RibbonsSummaryMenu *);
static void GetMonRibbons(struct Pokenav_RibbonsSummaryList *);
static u32 HandleExpandedRibbonInput(struct Pokenav_RibbonsSummaryList *);
static u32 RibbonsSummaryHandleInput(struct Pokenav_RibbonsSummaryList *);
static u32 ReturnToRibbonsListFromSummary(struct Pokenav_RibbonsSummaryList *);
static bool32 TrySelectRibbonUp(struct Pokenav_RibbonsSummaryList *);
static bool32 TrySelectRibbonRight(struct Pokenav_RibbonsSummaryList *);
static bool32 TrySelectRibbonLeft(struct Pokenav_RibbonsSummaryList *);
static bool32 TrySelectRibbonDown(struct Pokenav_RibbonsSummaryList *);
static bool32 GetCurrentLoopedTaskActive(void);
static u32 GetRibbonsSummaryCurrentIndex(void);
static u32 GetRibbonsSummaryMonListCount(void);
static u16 DrawRibbonsMonFrontPic(s32, s32);
static void StartMonSpriteSlide(struct Sprite *, s32, s32, s32);
static void SpriteCB_MonSpriteSlide(struct Sprite *);
static void ClearRibbonsSummaryBg(void);
static void BufferHgssRibbonGfxData(u16 *, u32);
static void DrawRibbonHgss(u32, u32);
static void SpriteCB_WaitForRibbonAnimation(struct Sprite *);
static u32 LoopedTask_OpenRibbonsSummaryMenu(s32);
static u32 LoopedTask_SwitchRibbonsSummaryMon(s32);
static u32 LoopedTask_ExpandSelectedRibbon(s32);
static u32 LoopedTask_MoveRibbonsCursorExpanded(s32);
static u32 LoopedTask_ShrinkExpandedRibbon(s32);
static u32 LoopedTask_ExitRibbonsSummaryMenu(s32);

struct
{
    u8 numBits; // The number of bits needed to represent numRibbons
    u8 numRibbons; // Never read. The contest ribbons have 4 (1 for each rank), the rest are just 1 ribbon
    u8 ribbonId;
    bool8 isGiftRibbon;
} static  const sRibbonData[] =
{
    {1, 1, CHAMPION_RIBBON,      FALSE},
    {3, 4, COOL_RIBBON_NORMAL,   FALSE},
    {3, 4, BEAUTY_RIBBON_NORMAL, FALSE},
    {3, 4, CUTE_RIBBON_NORMAL,   FALSE},
    {3, 4, SMART_RIBBON_NORMAL,  FALSE},
    {3, 4, TOUGH_RIBBON_NORMAL,  FALSE},
    {1, 1, WINNING_RIBBON,       FALSE},
    {1, 1, VICTORY_RIBBON,       FALSE},
    {1, 1, ARTIST_RIBBON,        FALSE},
    {1, 1, EFFORT_RIBBON,        FALSE},
    {1, 1, MARINE_RIBBON,        TRUE},
    {1, 1, LAND_RIBBON,          TRUE},
    {1, 1, SKY_RIBBON,           TRUE},
    {1, 1, COUNTRY_RIBBON,       TRUE},
    {1, 1, NATIONAL_RIBBON,      TRUE},
    {1, 1, EARTH_RIBBON,         TRUE},
    {1, 1, WORLD_RIBBON,         TRUE}
};

#include "data/text/ribbon_descriptions.h"
#include "data/text/gift_ribbon_descriptions.h"

static const u16 sMonInfo_Pal[] = INCGFX_U16("graphics/pokenav/ribbons/mon_info.pal", ".gbapal"); // palette for Pokémon's name/gender/level text
// All grid and expanded icons use the same unmodified retail HGSS tiles.
static const u32 sRibbonIconsBig_Gfx[] = INCBIN_U32("graphics/gen4_ui/hgss_ribbons/big_icons_hgss.4bpp");
static const u16 sHgssRibbonBigPal0[] = INCBIN_U16("graphics/gen4_ui/hgss_ribbons/palette_0.gbapal");
static const u16 sHgssRibbonBigPal1[] = INCBIN_U16("graphics/gen4_ui/hgss_ribbons/palette_1.gbapal");
static const u16 sHgssRibbonBigPal2[] = INCBIN_U16("graphics/gen4_ui/hgss_ribbons/palette_2.gbapal");
static const u16 sHgssRibbonBigPal3[] = INCBIN_U16("graphics/gen4_ui/hgss_ribbons/palette_3.gbapal");
static const u16 sHgssRibbonBigPal4[] = INCBIN_U16("graphics/gen4_ui/hgss_ribbons/palette_4.gbapal");

static const struct BgTemplate sBgTemplates[] =
{
    {
        .bg = 1,
        .charBaseIndex = 3,
        .mapBaseIndex = 0x07,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0
    },
    {
        .bg = 2,
        .charBaseIndex = 1,
        .mapBaseIndex = 0x06,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 2,
        .baseTile = 0
    }
};

static const LoopedTask sRibbonsSummaryMenuLoopTaskFuncs[] =
{
    [RIBBONS_SUMMARY_FUNC_NONE]                 = NULL,
    [RIBBONS_SUMMARY_FUNC_SWITCH_MONS]          = LoopedTask_SwitchRibbonsSummaryMon,
    [RIBBONS_SUMMARY_FUNC_SELECT_RIBBON]        = LoopedTask_ExpandSelectedRibbon,
    [RIBBONS_SUMMARY_FUNC_EXPANDED_CURSOR_MOVE] = LoopedTask_MoveRibbonsCursorExpanded,
    [RIBBONS_SUMMARY_FUNC_EXPANDED_CANCEL]      = LoopedTask_ShrinkExpandedRibbon,
    [RIBBONS_SUMMARY_FUNC_EXIT]                 = LoopedTask_ExitRibbonsSummaryMenu
};

bool32 PokenavCallback_Init_RibbonsSummaryMenu(void)
{
    struct Pokenav_RibbonsSummaryList *list = AllocSubstruct(POKENAV_SUBSTRUCT_RIBBONS_SUMMARY_LIST, sizeof(struct Pokenav_RibbonsSummaryList));
    if (list == NULL)
        return FALSE;

    list->monList = GetSubstructPtr(POKENAV_SUBSTRUCT_MON_LIST);
    if (list->monList == NULL)
        return FALSE;

    GetMonRibbons(list);
    list->callback = RibbonsSummaryHandleInput;
    gKeyRepeatContinueDelay = 3;
    gKeyRepeatStartDelay = 10;
    return TRUE;
}

u32 GetRibbonsSummaryMenuCallback(void)
{
    struct Pokenav_RibbonsSummaryList *list = GetSubstructPtr(POKENAV_SUBSTRUCT_RIBBONS_SUMMARY_LIST);
    return list->callback(list);
}

void FreeRibbonsSummaryScreen1(void)
{
    FreePokenavSubstruct(POKENAV_SUBSTRUCT_RIBBONS_SUMMARY_LIST);
}

// Handles input when a specific ribbon is not currently selected
static u32 RibbonsSummaryHandleInput(struct Pokenav_RibbonsSummaryList *list)
{
    // Handle Up/Down movement to select a new Pokémon to show ribbons for
    if (JOY_REPEAT(DPAD_UP) && list->monList->currIndex != 0)
    {
        list->monList->currIndex--;
        list->selectedPos = 0;
        GetMonRibbons(list);
        return RIBBONS_SUMMARY_FUNC_SWITCH_MONS;
    }
    if (JOY_REPEAT(DPAD_DOWN) && list->monList->currIndex < list->monList->listCount - 1)
    {
        list->monList->currIndex++;
        list->selectedPos = 0;
        GetMonRibbons(list);
        return RIBBONS_SUMMARY_FUNC_SWITCH_MONS;
    }

    if (JOY_NEW(A_BUTTON) && (list->numNormalRibbons != 0 || list->numGiftRibbons != 0))
    {
        // Do not inspect an uninitialized ribbon when this Pokémon has none.
        list->callback = HandleExpandedRibbonInput;
        return RIBBONS_SUMMARY_FUNC_SELECT_RIBBON;
    }
    if (JOY_NEW(B_BUTTON))
    {
        // Exit ribbon summary menu
        list->callback = ReturnToRibbonsListFromSummary;
        return RIBBONS_SUMMARY_FUNC_EXIT;
    }
    return RIBBONS_SUMMARY_FUNC_NONE;
}

// Handles input when a ribbon is selected
static u32 HandleExpandedRibbonInput(struct Pokenav_RibbonsSummaryList *list)
{
    // Handle movement while a ribbon is selected
    if (JOY_REPEAT(DPAD_UP) && TrySelectRibbonUp(list))
        return RIBBONS_SUMMARY_FUNC_EXPANDED_CURSOR_MOVE;
    if (JOY_REPEAT(DPAD_DOWN) && TrySelectRibbonDown(list))
        return RIBBONS_SUMMARY_FUNC_EXPANDED_CURSOR_MOVE;
    if (JOY_REPEAT(DPAD_LEFT) && TrySelectRibbonLeft(list))
        return RIBBONS_SUMMARY_FUNC_EXPANDED_CURSOR_MOVE;
    if (JOY_REPEAT(DPAD_RIGHT) && TrySelectRibbonRight(list))
        return RIBBONS_SUMMARY_FUNC_EXPANDED_CURSOR_MOVE;

    if (JOY_NEW(B_BUTTON))
    {
        // Exit ribbon selection
        list->callback = RibbonsSummaryHandleInput;
        return RIBBONS_SUMMARY_FUNC_EXPANDED_CANCEL;
    }
    return RIBBONS_SUMMARY_FUNC_NONE;
}

static u32 ReturnToRibbonsListFromSummary(struct Pokenav_RibbonsSummaryList *list)
{
    return POKENAV_RIBBONS_RETURN_TO_MON_LIST;
}

static bool32 TrySelectRibbonUp(struct Pokenav_RibbonsSummaryList *list)
{
    if (list->selectedPos < FIRST_GIFT_RIBBON)
    {
        // In normal ribbons, try to move up a row
        if (list->selectedPos < RIBBONS_PER_ROW)
            return FALSE;

        list->selectedPos -= RIBBONS_PER_ROW;
        return TRUE;
    }
    // Gift ribbons can now span multiple four-icon rows.
    if (list->selectedPos - GIFT_RIBBON_START_POS >= RIBBONS_PER_ROW)
    {
        list->selectedPos -= RIBBONS_PER_ROW;
        return TRUE;
    }
    if (list->numNormalRibbons != 0)
    {
        // Move back into the last populated normal-ribbon row.
        u32 ribbonPos = list->selectedPos - GIFT_RIBBON_START_POS;
        list->selectedPos = ribbonPos + list->normalRibbonLastRowStart;
        if (list->selectedPos >= list->numNormalRibbons)
            list->selectedPos = list->numNormalRibbons - 1;
        return TRUE;
    }
    return FALSE;
}

static bool32 TrySelectRibbonDown(struct Pokenav_RibbonsSummaryList *list)
{
    if (list->selectedPos >= GIFT_RIBBON_START_POS)
    {
        u32 nextRibbon = list->selectedPos - GIFT_RIBBON_START_POS + RIBBONS_PER_ROW;
        if (nextRibbon < list->numGiftRibbons)
        {
            list->selectedPos += RIBBONS_PER_ROW;
            return TRUE;
        }
        return FALSE;
    }
    if (list->selectedPos < list->normalRibbonLastRowStart)
    {
        // Not in last row of normal ribbons, advance to next row
        list->selectedPos += RIBBONS_PER_ROW;
        if (list->selectedPos >= list->numNormalRibbons)
            list->selectedPos = list->numNormalRibbons - 1;
        return TRUE;
    }
    if (list->numGiftRibbons != 0)
    {
        // In/beyond last of row of normal ribbons and gift ribbons present, move down to gift ribbon row
        int ribbonPos = list->selectedPos - list->normalRibbonLastRowStart;
        if (ribbonPos >= list->numGiftRibbons)
            ribbonPos = list->numGiftRibbons - 1;

        list->selectedPos = ribbonPos + GIFT_RIBBON_START_POS;
        return TRUE;
    }
    return FALSE;
}

static bool32 TrySelectRibbonLeft(struct Pokenav_RibbonsSummaryList *list)
{
    u16 column = list->selectedPos % RIBBONS_PER_ROW;
    if (column != 0)
    {
        list->selectedPos--;
        return TRUE;
    }

    return FALSE;
}

static bool32 TrySelectRibbonRight(struct Pokenav_RibbonsSummaryList *list)
{
    int column = list->selectedPos % RIBBONS_PER_ROW;

    if (column >= RIBBONS_PER_ROW - 1)
        return FALSE;

    if (list->selectedPos < GIFT_RIBBON_START_POS)
    {
        // Move right in normal ribbon row
        if (list->selectedPos < list->numNormalRibbons - 1)
        {
            list->selectedPos++;
            return TRUE;
        }
    }
    else
    {
        // Move right in gift ribbon row
        if (list->selectedPos - GIFT_RIBBON_START_POS + 1 < list->numGiftRibbons)
        {
            list->selectedPos++;
            return TRUE;
        }
    }
    return FALSE;
}

static u32 GetRibbonsSummaryCurrentIndex(void)
{
    struct Pokenav_RibbonsSummaryList *list = GetSubstructPtr(POKENAV_SUBSTRUCT_RIBBONS_SUMMARY_LIST);
    return list->monList->currIndex;
}

static u32 GetRibbonsSummaryMonListCount(void)
{
    struct Pokenav_RibbonsSummaryList *list = GetSubstructPtr(POKENAV_SUBSTRUCT_RIBBONS_SUMMARY_LIST);
    return list->monList->listCount;
}

static void GetMonNicknameLevelGender(u8 *nick, u8 *level, u8 *gender)
{
    struct Pokenav_RibbonsSummaryList *list = GetSubstructPtr(POKENAV_SUBSTRUCT_RIBBONS_SUMMARY_LIST);
    struct PokenavMonList *mons = list->monList;
    struct PokenavMonListItem *monInfo = &mons->monData[mons->currIndex];

    if (monInfo->boxId == TOTAL_BOXES_COUNT)
    {
        // Get info for party mon
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][monInfo->monId];
        GetMonData(mon, MON_DATA_NICKNAME, nick);
        *level = GetLevelFromMonExp(mon);
        *gender = GetMonGender(mon);
    }
    else
    {
        // Get info for PC box mon
        struct BoxPokemon *boxMon = GetBoxedMonPtr(monInfo->boxId, monInfo->monId);
        *gender = GetBoxMonGender(boxMon);
        *level = GetLevelFromBoxMonExp(boxMon);
        GetBoxMonData(boxMon, MON_DATA_NICKNAME, nick);
    }
    StringGet_Nickname(nick);
}

static void GetMonSpeciesPersonalityShiny(enum Species *species, u32 *personality, bool32 *isShiny)
{
    struct Pokenav_RibbonsSummaryList *list = GetSubstructPtr(POKENAV_SUBSTRUCT_RIBBONS_SUMMARY_LIST);
    struct PokenavMonList *mons = list->monList;
    struct PokenavMonListItem *monInfo = &mons->monData[mons->currIndex];

    if (monInfo->boxId == TOTAL_BOXES_COUNT)
    {
        // Get info for party mon
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][monInfo->monId];
        *species = GetMonData(mon, MON_DATA_SPECIES);
        *personality = GetMonData(mon, MON_DATA_PERSONALITY);
        *isShiny = GetMonData(mon, MON_DATA_IS_SHINY);
    }
    else
    {
        // Get info for PC box mon
        struct BoxPokemon *boxMon = GetBoxedMonPtr(monInfo->boxId, monInfo->monId);
        *species = GetBoxMonData(boxMon, MON_DATA_SPECIES);
        *personality = GetBoxMonData(boxMon, MON_DATA_PERSONALITY);
        *isShiny = GetBoxMonData(boxMon, MON_DATA_IS_SHINY);
    }
}

static u32 GetCurrMonRibbonCount(void)
{
    struct Pokenav_RibbonsSummaryList *list = GetSubstructPtr(POKENAV_SUBSTRUCT_RIBBONS_SUMMARY_LIST);
    struct PokenavMonList *mons = list->monList;
    struct PokenavMonListItem *monInfo = &mons->monData[mons->currIndex];

    if (monInfo->boxId == TOTAL_BOXES_COUNT)
        return GetMonData(&gParties[B_TRAINER_PLAYER][monInfo->monId], MON_DATA_RIBBON_COUNT);
    else
        return GetBoxMonDataAt(monInfo->boxId, monInfo->monId, MON_DATA_RIBBON_COUNT);
}

static void GetMonRibbons(struct Pokenav_RibbonsSummaryList *list)
{
    u32 ribbonFlags;
    s32 i, j;
    struct PokenavMonList *mons = list->monList;
    struct PokenavMonListItem *monInfo = &mons->monData[mons->currIndex];

    if (monInfo->boxId == TOTAL_BOXES_COUNT)
        ribbonFlags = GetMonData(&gParties[B_TRAINER_PLAYER][monInfo->monId], MON_DATA_RIBBONS);
    else
        ribbonFlags = GetBoxMonDataAt(monInfo->boxId, monInfo->monId, MON_DATA_RIBBONS);

    list->numNormalRibbons = 0;
    list->numGiftRibbons = 0;
    for (i = 0; i < ARRAY_COUNT(sRibbonData); i++)
    {
        // For all non-contest ribbons, numRibbons will be 1 if they have it, 0 if they don't
        // For contest ribbons, numRibbons will be 0-4
        s32 numRibbons = ((1 << sRibbonData[i].numBits) - 1) & ribbonFlags;
        if (!sRibbonData[i].isGiftRibbon)
        {
            for (j = 0; j < numRibbons; j++)
                list->ribbonIds[list->numNormalRibbons++] = sRibbonData[i].ribbonId + j;
        }
        else
        {
            for (j = 0; j < numRibbons; j++)
                list->giftRibbonIds[list->numGiftRibbons++] = sRibbonData[i].ribbonId + j;
        }
        ribbonFlags >>= sRibbonData[i].numBits;
    }

    if (list->numNormalRibbons != 0)
    {
        list->normalRibbonLastRowStart = ((list->numNormalRibbons - 1) / RIBBONS_PER_ROW) * RIBBONS_PER_ROW;
        list->selectedPos = 0;
    }
    else
    {
        // With gift ribbons only, start at their first slot. If this Pokémon
        // has no ribbons at all, retain a valid first-page cursor position.
        list->normalRibbonLastRowStart = 0;
        list->selectedPos = list->numGiftRibbons != 0 ? GIFT_RIBBON_START_POS : 0;
    }
}

static u32 *GetNormalRibbonIds(u32 *size)
{
    struct Pokenav_RibbonsSummaryList *list = GetSubstructPtr(POKENAV_SUBSTRUCT_RIBBONS_SUMMARY_LIST);
    *size = list->numNormalRibbons;
    return list->ribbonIds;
}

static u32 *GetGiftRibbonIds(u32 *size)
{
    struct Pokenav_RibbonsSummaryList *list = GetSubstructPtr(POKENAV_SUBSTRUCT_RIBBONS_SUMMARY_LIST);
    *size = list->numGiftRibbons;
    return list->giftRibbonIds;
}

static u16 GetSelectedPosition(void)
{
    struct Pokenav_RibbonsSummaryList *list = GetSubstructPtr(POKENAV_SUBSTRUCT_RIBBONS_SUMMARY_LIST);
    return list->selectedPos;
}

static u32 GetRibbonId(void)
{
    struct Pokenav_RibbonsSummaryList *list = GetSubstructPtr(POKENAV_SUBSTRUCT_RIBBONS_SUMMARY_LIST);
    int ribbonPos = list->selectedPos;
    if (ribbonPos < FIRST_GIFT_RIBBON)
        return list->ribbonIds[ribbonPos];
    else
        return list->giftRibbonIds[ribbonPos - GIFT_RIBBON_START_POS];
}

bool32 OpenRibbonsSummaryMenu(void)
{
    struct Pokenav_RibbonsSummaryMenu *menu = AllocSubstruct(POKENAV_SUBSTRUCT_RIBBONS_SUMMARY_MENU, sizeof(struct Pokenav_RibbonsSummaryMenu));
    if (menu == NULL)
        return FALSE;

    menu->loopedTaskId = CreateLoopedTask(LoopedTask_OpenRibbonsSummaryMenu, 1);
    menu->callback = GetCurrentLoopedTaskActive;
    return TRUE;
}

void CreateRibbonsSummaryLoopedTask(s32 id)
{
    struct Pokenav_RibbonsSummaryMenu *menu = GetSubstructPtr(POKENAV_SUBSTRUCT_RIBBONS_SUMMARY_MENU);
    menu->loopedTaskId = CreateLoopedTask(sRibbonsSummaryMenuLoopTaskFuncs[id], 1);
    menu->callback = GetCurrentLoopedTaskActive;
}

u32 IsRibbonsSummaryLoopedTaskActive(void)
{
    struct Pokenav_RibbonsSummaryMenu *menu = GetSubstructPtr(POKENAV_SUBSTRUCT_RIBBONS_SUMMARY_MENU);
    return menu->callback();
}

void FreeRibbonsSummaryScreen2(void)
{
    struct Pokenav_RibbonsSummaryMenu *menu = GetSubstructPtr(POKENAV_SUBSTRUCT_RIBBONS_SUMMARY_MENU);
    RemoveWindow(menu->ribbonCountWindowId);
    RemoveWindow(menu->nameWindowId);
    RemoveWindow(menu->listIdxWindowId);
    RemoveWindow(menu->gridFrameWindowId);
#ifndef BUGFIX
    RemoveWindow(menu->unusedWindowId); // Removing window, but window id is never set
#endif
    DestroyRibbonsMonFrontPic(menu);
    FreeSpriteTilesByTag(GFXTAG_RIBBON_ICONS_BIG);
    FreeSpritePaletteByTag(PALTAG_RIBBON_ICONS_1);
    FreeSpritePaletteByTag(PALTAG_RIBBON_ICONS_2);
    FreeSpritePaletteByTag(PALTAG_RIBBON_ICONS_3);
    FreeSpritePaletteByTag(PALTAG_RIBBON_ICONS_4);
    FreeSpritePaletteByTag(PALTAG_RIBBON_ICONS_5);
    DestroyHgssRibbonCursor(menu);
    FreeSpriteOamMatrix(menu->bigRibbonSprite);
    DestroySprite(menu->bigRibbonSprite);
    FreePokenavSubstruct(POKENAV_SUBSTRUCT_RIBBONS_SUMMARY_MENU);
}

static bool32 GetCurrentLoopedTaskActive(void)
{
    struct Pokenav_RibbonsSummaryMenu *menu = GetSubstructPtr(POKENAV_SUBSTRUCT_RIBBONS_SUMMARY_MENU);
    return IsLoopedTaskActive(menu->loopedTaskId);
}

static u32 LoopedTask_OpenRibbonsSummaryMenu(s32 state)
{
    struct Pokenav_RibbonsSummaryMenu *menu = GetSubstructPtr(POKENAV_SUBSTRUCT_RIBBONS_SUMMARY_MENU);
    switch (state)
    {
    case 0:
        InitBgTemplates(sBgTemplates, ARRAY_COUNT(sBgTemplates));
        DecompressAndCopyTileDataToVram(2, gPokenavRibbonsSummaryBg_Gfx, 0, 0, 0);
        SetBgTilemapBuffer(2, menu->tilemapBuffers[0]);
        CopyToBgTilemapBuffer(2, gPokenavRibbonsSummaryBg_Tilemap, 0, 0);
        CopyPaletteIntoBufferUnfaded(gPokenavRibbonsSummaryBg_Pal, BG_PLTT_ID(1), PLTT_SIZE_4BPP);
        CopyBgTilemapBufferToVram(2);
        return LT_INC_AND_PAUSE;
    case 1:
        if (!FreeTempTileDataBuffersIfPossible())
        {
            BgDmaFill(1, 0, 0, 1);
            // Leave tile zero blank for the BG clear operation.
            // 12 * 16 retail 4bpp tiles occupy only 0x1800 of BG1's 0x4000 bytes.
            LoadBgTiles(1, sRibbonIconsBig_Gfx, sizeof(sRibbonIconsBig_Gfx), 1);
            SetBgTilemapBuffer(1, menu->tilemapBuffers[1]);
            FillBgTilemapBufferRect_Palette0(1, 0, 0, 0, 32, 20);
            CopyPaletteIntoBufferUnfaded(sHgssRibbonBigPal0, BG_PLTT_ID(2), PLTT_SIZE_4BPP);
            CopyPaletteIntoBufferUnfaded(sHgssRibbonBigPal1, BG_PLTT_ID(3), PLTT_SIZE_4BPP);
            CopyPaletteIntoBufferUnfaded(sHgssRibbonBigPal2, BG_PLTT_ID(4), PLTT_SIZE_4BPP);
            CopyPaletteIntoBufferUnfaded(sHgssRibbonBigPal3, BG_PLTT_ID(5), PLTT_SIZE_4BPP);
            CopyPaletteIntoBufferUnfaded(sHgssRibbonBigPal4, BG_PLTT_ID(6), PLTT_SIZE_4BPP);
            CopyPaletteIntoBufferUnfaded(sMonInfo_Pal, BG_PLTT_ID(10), sizeof(sMonInfo_Pal));
            CopyBgTilemapBufferToVram(1);
            return LT_INC_AND_PAUSE;
        }
        return LT_PAUSE;
    case 2:
        if (!FreeTempTileDataBuffersIfPossible())
        {
            AddRibbonGridFrameWindow(menu);
            AddRibbonCountWindow(menu);
            return LT_INC_AND_PAUSE;
        }
        return LT_PAUSE;
    case 3:
        if (!FreeTempTileDataBuffersIfPossible())
        {
            AddRibbonSummaryMonNameWindow(menu);
            return LT_INC_AND_PAUSE;
        }
        return LT_PAUSE;
    case 4:
        if (!FreeTempTileDataBuffersIfPossible())
        {
            AddRibbonListIndexWindow(menu);
            return LT_INC_AND_PAUSE;
        }
        return LT_PAUSE;
    case 5:
        if (!IsDma3ManagerBusyWithBgCopy())
        {
            CopyBgTilemapBufferToVram(2);
            return LT_INC_AND_PAUSE;
        }
        return LT_PAUSE;
    case 6:
        if (!IsDma3ManagerBusyWithBgCopy())
        {
            ResetSpritesAndDrawMonFrontPic(menu);
            return LT_INC_AND_CONTINUE;
        }
        return LT_PAUSE;
    case 7:
        DrawAllRibbonsHgss(menu);
        PrintHelpBarText(HELPBAR_RIBBONS_LIST);
        return LT_INC_AND_PAUSE;
    case 8:
        if (!IsDma3ManagerBusyWithBgCopy())
        {
            CreateBigRibbonSprite(menu);
            CreateHgssRibbonCursor(menu);
            ChangeBgX(1, 0, BG_COORD_SET);
            ChangeBgY(1, 0, BG_COORD_SET);
            ChangeBgX(2, 0, BG_COORD_SET);
            ChangeBgY(2, 0, BG_COORD_SET);
            ShowBg(1);
            ShowBg(2);
            HideBg(3);
            PokenavFadeScreen(POKENAV_FADE_FROM_BLACK);
            return LT_INC_AND_PAUSE;
        }
        return LT_PAUSE;
    case 9:
        if (IsPaletteFadeActive())
            return LT_PAUSE;
    }
    return LT_FINISH;
}

static u32 LoopedTask_ExitRibbonsSummaryMenu(s32 state)
{
    switch (state)
    {
    case 0:
        PlaySE(SE_SELECT);
        PokenavFadeScreen(POKENAV_FADE_TO_BLACK);
        return LT_INC_AND_PAUSE;
    case 1:
        if (IsPaletteFadeActive())
            return LT_PAUSE;
        return LT_FINISH;
    }
    return LT_FINISH;
}

static u32 LoopedTask_SwitchRibbonsSummaryMon(s32 state)
{
    struct Pokenav_RibbonsSummaryMenu *menu = GetSubstructPtr(POKENAV_SUBSTRUCT_RIBBONS_SUMMARY_MENU);
    switch (state)
    {
    case 0:
        PlaySE(SE_SELECT);
        SlideMonSpriteOff(menu);
        return LT_INC_AND_PAUSE;
    case 1:
        if (!IsMonSpriteAnimating(menu))
        {
            PrintRibbbonsSummaryMonInfo(menu);
            return LT_INC_AND_CONTINUE;
        }
        return LT_PAUSE;
    case 2:
        DrawAllRibbonsHgss(menu);
        return LT_INC_AND_CONTINUE;
    case 3:
        PrintRibbonsMonListIndex(menu);
        return LT_INC_AND_CONTINUE;
    case 4:
        PrintCurrentMonRibbonCount(menu);
        return LT_INC_AND_CONTINUE;
    case 5:
        if (!IsDma3ManagerBusyWithBgCopy())
        {
            SlideMonSpriteOn(menu);
            return LT_INC_AND_PAUSE;
        }
        return LT_PAUSE;
    case 6:
        if (IsMonSpriteAnimating(menu))
            return LT_PAUSE;
    }
    return LT_FINISH;
}

static u32 LoopedTask_ExpandSelectedRibbon(s32 state)
{
    struct Pokenav_RibbonsSummaryMenu *menu = GetSubstructPtr(POKENAV_SUBSTRUCT_RIBBONS_SUMMARY_MENU);
    switch (state)
    {
    case 0:
        PlaySE(SE_SELECT);
        UpdateAndZoomInSelectedRibbon(menu);
        return LT_INC_AND_PAUSE;
    case 1:
        if (!IsRibbonAnimating(menu))
        {
            PrintRibbonNameAndDescription(menu);
            PrintHelpBarText(HELPBAR_RIBBONS_CHECK);
            return LT_INC_AND_PAUSE;
        }
        return LT_PAUSE;
    case 2:
        if (IsDma3ManagerBusyWithBgCopy())
            return LT_PAUSE;
    }
    return LT_FINISH;
}

static u32 LoopedTask_MoveRibbonsCursorExpanded(s32 state)
{
    struct Pokenav_RibbonsSummaryMenu *menu = GetSubstructPtr(POKENAV_SUBSTRUCT_RIBBONS_SUMMARY_MENU);
    switch (state)
    {
    case 0:
        PlaySE(SE_SELECT);
        ZoomOutSelectedRibbon(menu);
        return LT_INC_AND_PAUSE;
    case 1:
        if (!IsRibbonAnimating(menu))
        {
            // Page to the selected ribbon before moving the expanded cursor.
            DrawAllRibbonsHgss(menu);
            UpdateAndZoomInSelectedRibbon(menu);
            return LT_INC_AND_PAUSE;
        }
        return LT_PAUSE;
    case 2:
        if (!IsRibbonAnimating(menu))
        {
            PrintRibbonNameAndDescription(menu);
            return LT_INC_AND_PAUSE;
        }
        return LT_PAUSE;
    case 3:
        if (IsDma3ManagerBusyWithBgCopy())
            return LT_PAUSE;
    }
    return LT_FINISH;
}

static u32 LoopedTask_ShrinkExpandedRibbon(s32 state)
{
    struct Pokenav_RibbonsSummaryMenu *menu = GetSubstructPtr(POKENAV_SUBSTRUCT_RIBBONS_SUMMARY_MENU);
    switch (state)
    {
    case 0:
        PlaySE(SE_SELECT);
        ZoomOutSelectedRibbon(menu);
        return LT_INC_AND_PAUSE;
    case 1:
        if (!IsRibbonAnimating(menu))
        {
            PrintCurrentMonRibbonCount(menu);
            PrintHelpBarText(HELPBAR_RIBBONS_LIST);
            return LT_INC_AND_PAUSE;
        }
        return LT_PAUSE;
    case 2:
        if (IsDma3ManagerBusyWithBgCopy())
            return LT_PAUSE;
    }
    return LT_FINISH;
}

static const struct WindowTemplate sRibbonGridFrameWindowTemplate =
{
    .bg = 2,
    .tilemapLeft = 10,
    .tilemapTop = 3,
    .width = 20,
    .height = 10,
    .paletteNum = 1,
    .baseBlock = 0x7C,
};

static void DrawPokeGearRibbonGridFrame(u16 windowId)
{
    // No retail HGSS ribbon-grid chrome is verified for this window.
    // Transparent pixels preserve the existing BG presentation and ribbon icons,
    // rather than substituting programmatically drawn medal-case artwork.
    FillWindowPixelBuffer(windowId, PIXEL_FILL(0));
}

// Reuse the verified HGSS Pokégear selection-corner pixels and palette.
static const u32 sHgssRibbonCursorTiles[] = INCBIN_U32("graphics/gen4_ui/hgss_pokegear/cursor_corner.4bpp");
static const u16 sHgssRibbonCursorPalette[] = INCBIN_U16("graphics/gen4_ui/hgss_pokegear/cursor_corner.gbapal");

static const struct SpriteSheet sHgssRibbonCursorSheet =
{
    .data = sHgssRibbonCursorTiles,
    .size = sizeof(sHgssRibbonCursorTiles),
    .tag = GFXTAG_HGSS_RIBBON_CURSOR,
};

static const struct SpritePalette sHgssRibbonCursorPal =
{
    .data = sHgssRibbonCursorPalette,
    .tag = PALTAG_HGSS_RIBBON_CURSOR,
};

static const struct OamData sHgssRibbonCursorOam =
{
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(16x16),
    .size = SPRITE_SIZE(16x16),
    .priority = 0,
};

static const union AnimCmd sHgssRibbonCursorAnim[] =
{
    ANIMCMD_FRAME(0, 1),
    ANIMCMD_END,
};

static const union AnimCmd *const sHgssRibbonCursorAnims[] =
{
    sHgssRibbonCursorAnim,
};

static const struct SpriteTemplate sHgssRibbonCursorTemplate =
{
    .tileTag = GFXTAG_HGSS_RIBBON_CURSOR,
    .paletteTag = PALTAG_HGSS_RIBBON_CURSOR,
    .oam = &sHgssRibbonCursorOam,
    .anims = sHgssRibbonCursorAnims,
    .callback = SpriteCallbackDummy,
};

static void CreateHgssRibbonCursor(struct Pokenav_RibbonsSummaryMenu *menu)
{
    u32 i;
    LoadSpriteSheet(&sHgssRibbonCursorSheet);
    LoadSpritePalette(&sHgssRibbonCursorPal);
    for (i = 0; i < HGSS_RIBBON_CURSOR_CORNERS; i++)
    {
        u8 id = CreateSprite(&sHgssRibbonCursorTemplate, 0, 0, 0);
        menu->hgssCursorCorners[i] = id == MAX_SPRITES ? NULL : &gSprites[id];
        if (menu->hgssCursorCorners[i] != NULL)
        {
            menu->hgssCursorCorners[i]->invisible = TRUE;
            SetSpriteOamFlipBits(menu->hgssCursorCorners[i], (i & 2) != 0, (i & 1) != 0);
        }
    }
}

static void DestroyHgssRibbonCursor(struct Pokenav_RibbonsSummaryMenu *menu)
{
    u32 i;
    for (i = 0; i < HGSS_RIBBON_CURSOR_CORNERS; i++)
        if (menu->hgssCursorCorners[i] != NULL)
            DestroySprite(menu->hgssCursorCorners[i]);
    FreeSpriteTilesByTag(GFXTAG_HGSS_RIBBON_CURSOR);
    FreeSpritePaletteByTag(PALTAG_HGSS_RIBBON_CURSOR);
}

// Both the 32x32 selection-corner sprites and affine zoom sprite use
// the same page-relative BG-cell center. No independent pixel arithmetic.
static void GetHgssRibbonScreenPosition(u32 position, s16 *x, s16 *y)
{
    u32 pageSlot = position % RIBBONS_PER_PAGE;
    *x = (RIBBON_GRID_LEFT_TILE + RIBBON_ICON_TILES * (pageSlot % RIBBONS_PER_ROW)) * 8 + 16;
    *y = (RIBBON_GRID_TOP_TILE + RIBBON_ICON_TILES * (pageSlot / RIBBONS_PER_ROW)) * 8 + 16;
}

static void DrawPokeGearRibbonSelectionFocus(struct Pokenav_RibbonsSummaryMenu *menu, bool8 show)
{
    s16 x, y;
    u32 i;
    GetHgssRibbonScreenPosition(GetSelectedPosition(), &x, &y);

    // Four authentic HGSS corners frame the selected 32x32 ribbon icon.
    for (i = 0; i < HGSS_RIBBON_CURSOR_CORNERS; i++)
    {
        struct Sprite *corner = menu->hgssCursorCorners[i];
        if (corner == NULL)
            continue;
        corner->x = x + ((i & 2) ? 16 : -16);
        corner->y = y + ((i & 1) ? 16 : -16);
        corner->invisible = !show;
    }
}

static void DrawPokeGearRibbonDetailPanel(u16 windowId)
{
    // Clear previous text without drawing a fabricated HGSS-style bevel.
    // The authentic HGSS ribbon-details surface has not yet been wired.
    FillWindowPixelBuffer(windowId, PIXEL_FILL(4));
}

static void DrawPokeGearRibbonMonCard(u16 windowId)
{
    // Retain the readable name/level text surface without fabricated chrome.
    // No retail HGSS ribbon-summary card art has been imported for this window.
    FillWindowPixelBuffer(windowId, PIXEL_FILL(1));
}

static void DrawPokeGearRibbonIndexCard(u16 windowId)
{
    // Keep functional ribbon-list position text without fabricated borders.
    // Authentic HGSS replacement art for this window remains to be verified.
    FillWindowPixelBuffer(windowId, PIXEL_FILL(4));
}

static void AddRibbonGridFrameWindow(struct Pokenav_RibbonsSummaryMenu *menu)
{
    menu->gridFrameWindowId = AddWindow(&sRibbonGridFrameWindowTemplate);
    DrawPokeGearRibbonGridFrame(menu->gridFrameWindowId);
    PutWindowTilemap(menu->gridFrameWindowId);
    CopyWindowToVram(menu->gridFrameWindowId, COPYWIN_FULL);
}

static const struct WindowTemplate sRibbonCountWindowTemplate =
{
    .bg = 2,
    .tilemapLeft = 12,
    .tilemapTop = 12,
    .width = 16,
    .height = 4,
    .paletteNum = 1,
    .baseBlock = 0x14,
};

static void AddRibbonCountWindow(struct Pokenav_RibbonsSummaryMenu *menu)
{
    menu->ribbonCountWindowId = AddWindow(&sRibbonCountWindowTemplate);
    PutWindowTilemap(menu->ribbonCountWindowId);
    PrintCurrentMonRibbonCount(menu);
}

static void PrintCurrentMonRibbonCount(struct Pokenav_RibbonsSummaryMenu *menu)
{
    u8 color[] = {TEXT_COLOR_RED, TEXT_COLOR_DARK_GRAY, TEXT_COLOR_LIGHT_GRAY};

    ConvertIntToDecimalStringN(gStringVar1, GetCurrMonRibbonCount(), STR_CONV_MODE_LEFT_ALIGN, 2);
    DynamicPlaceholderTextUtil_Reset();
    DynamicPlaceholderTextUtil_SetPlaceholderPtr(0, gStringVar1);
    DynamicPlaceholderTextUtil_ExpandPlaceholders(gStringVar4, gText_RibbonsF700);
    DrawPokeGearRibbonDetailPanel(menu->ribbonCountWindowId);
    AddTextPrinterParameterized3(menu->ribbonCountWindowId, FONT_NORMAL, 12, 1, color, TEXT_SKIP_DRAW, gStringVar4);
    CopyWindowToVram(menu->ribbonCountWindowId, COPYWIN_GFX);
}

static void PrintRibbonNameAndDescription(struct Pokenav_RibbonsSummaryMenu *menu)
{
    s32 i;
    u32 ribbonId = GetRibbonId();
    u8 color[] = {TEXT_COLOR_RED, TEXT_COLOR_DARK_GRAY, TEXT_COLOR_LIGHT_GRAY};

    DrawPokeGearRibbonDetailPanel(menu->ribbonCountWindowId);
    if (ribbonId < FIRST_GIFT_RIBBON)
    {
        // Print normal ribbon name/description
        for (i = 0; i < 2; i++)
            AddTextPrinterParameterized3(menu->ribbonCountWindowId, FONT_SMALL, 12, (i * 14) + 1, color, TEXT_SKIP_DRAW, gRibbonDescriptionPointers[ribbonId][i]);
    }
    else
    {
        // ribbonId here is one of the 'gift' ribbon slots, used to read
        // its actual value from giftRibbons to determine which specific
        // gift ribbon it is
        ribbonId = gSaveBlock1Ptr->giftRibbons[ribbonId - FIRST_GIFT_RIBBON];

        // Refresh the cleared panel even when this gift slot is unoccupied.
        if (ribbonId == 0)
        {
            CopyWindowToVram(menu->ribbonCountWindowId, COPYWIN_GFX);
            return;
        }

        // Print gift ribbon name/description
        ribbonId--;
        for (i = 0; i < 2; i++)
            AddTextPrinterParameterized3(menu->ribbonCountWindowId, FONT_SMALL, 12, (i * 14) + 1, color, TEXT_SKIP_DRAW, gGiftRibbonDescriptionPointers[ribbonId][i]);
    }

    CopyWindowToVram(menu->ribbonCountWindowId, COPYWIN_GFX);
}

static const struct WindowTemplate sRibbonSummaryMonNameWindowTemplate =
{
    .bg = 2,
    .tilemapLeft = 14,
    .tilemapTop = 1,
    .width = 13,
    .height = 2,
    .paletteNum = 10,
    .baseBlock = 0x54,
};

static void AddRibbonSummaryMonNameWindow(struct Pokenav_RibbonsSummaryMenu *menu)
{
    menu->nameWindowId = AddWindow(&sRibbonSummaryMonNameWindowTemplate);
    PutWindowTilemap(menu->nameWindowId);
    PrintRibbbonsSummaryMonInfo(menu);
}

static const u8 sText_MaleSymbol[] = _("{TEXT_COLORS LIGHT_RED GREEN WHITE}{BACKGROUND WHITE}♂{TEXT_COLORS DARK_GRAY LIGHT_GRAY WHITE}{BACKGROUND WHITE}");
static const u8 sText_FemaleSymbol[] = _("{TEXT_COLORS LIGHT_GREEN BLUE WHITE}{BACKGROUND WHITE}♀{TEXT_COLORS DARK_GRAY LIGHT_GRAY WHITE}{BACKGROUND WHITE}");
static const u8 sText_NoGenderSymbol[] = _("{UNK_SPACER}");

static void PrintRibbbonsSummaryMonInfo(struct Pokenav_RibbonsSummaryMenu *menu)
{
    const u8 *genderTxt;
    u8 *txtPtr;
    u8 level, gender;
    u16 windowId = menu->nameWindowId;

    DrawPokeGearRibbonMonCard(windowId);
    GetMonNicknameLevelGender(gStringVar3, &level, &gender);
    switch (gender)
    {
    case MON_MALE:
        genderTxt = sText_MaleSymbol;
        break;
    case MON_FEMALE:
        genderTxt = sText_FemaleSymbol;
        break;
    default:
        genderTxt = sText_NoGenderSymbol;
        break;
    }
    AddTextPrinterParameterized(windowId, GetFontIdToFit(gStringVar3, FONT_NORMAL, 0, 56), gStringVar3, 6, 1, TEXT_SKIP_DRAW, NULL);

    txtPtr = StringCopy(gStringVar1, genderTxt);
    *(txtPtr++) = CHAR_SLASH;
    *(txtPtr++) = CHAR_EXTRA_SYMBOL;
    *(txtPtr++) = CHAR_LV_2;
    ConvertIntToDecimalStringN(txtPtr, level, STR_CONV_MODE_LEFT_ALIGN, 3);
    AddTextPrinterParameterized(windowId, FONT_NORMAL, gStringVar1, 64, 1, TEXT_SKIP_DRAW, NULL);
    CopyWindowToVram(windowId, COPYWIN_GFX);
}

static const struct WindowTemplate sRibbonMonListIndexWindowTemplate[] =
{
    {
        .bg = 2,
        .tilemapLeft = 1,
        .tilemapTop = 5,
        .width = 7,
        .height = 2,
        .paletteNum = 1,
        .baseBlock = 0x6E,
    },
    {},
};

static void AddRibbonListIndexWindow(struct Pokenav_RibbonsSummaryMenu *menu)
{
    menu->listIdxWindowId = AddWindow(sRibbonMonListIndexWindowTemplate);
    DrawPokeGearRibbonIndexCard(menu->listIdxWindowId);
    PutWindowTilemap(menu->listIdxWindowId);
    PrintRibbonsMonListIndex(menu);
}

static void PrintRibbonsMonListIndex(struct Pokenav_RibbonsSummaryMenu *menu)
{
    s32 x;
    u8 *txtPtr;
    u32 id = GetRibbonsSummaryCurrentIndex() + 1;
    u32 count = GetRibbonsSummaryMonListCount();

    DrawPokeGearRibbonIndexCard(menu->listIdxWindowId);
    txtPtr = ConvertIntToDecimalStringN(gStringVar1, id, STR_CONV_MODE_RIGHT_ALIGN, 3);
    *(txtPtr++) = CHAR_SLASH;
    ConvertIntToDecimalStringN(txtPtr, count, STR_CONV_MODE_RIGHT_ALIGN, 3);
    x = GetStringCenterAlignXOffset(FONT_NORMAL, gStringVar1, 56);
    AddTextPrinterParameterized(menu->listIdxWindowId, FONT_NORMAL, gStringVar1, x, 1, TEXT_SKIP_DRAW, NULL);
    CopyWindowToVram(menu->listIdxWindowId, COPYWIN_GFX);
}

static void ResetSpritesAndDrawMonFrontPic(struct Pokenav_RibbonsSummaryMenu *menu)
{
    enum Species species;
    u32 personality;
    bool32 isShiny;

    GetMonSpeciesPersonalityShiny(&species, &personality, &isShiny);
    ResetAllPicSprites();
    menu->monSpriteId = DrawRibbonsMonFrontPic(MON_SPRITE_X_ON, MON_SPRITE_Y);
    PokenavFillPalette(15, 0);
}

static void DestroyRibbonsMonFrontPic(struct Pokenav_RibbonsSummaryMenu *menu)
{
    FreeAndDestroyMonPicSprite(menu->monSpriteId);
}

// x and y arguments are ignored
// y is always given as MON_SPRITE_Y
// x is given as either MON_SPRITE_X_ON or MON_SPRITE_X_OFF (but ignored and MON_SPRITE_X_ON is used)
static u16 DrawRibbonsMonFrontPic(s32 x, s32 y)
{
    enum Species species;
    u16 spriteId;
    u32 personality;
    bool32 isShiny;

    GetMonSpeciesPersonalityShiny(&species, &personality, &isShiny);
    spriteId = CreateMonPicSprite(species, isShiny, personality, TRUE, MON_SPRITE_X_ON, MON_SPRITE_Y, 15, TAG_NONE);
    gSprites[spriteId].oam.priority = 0;
    return spriteId;
}

static void SlideMonSpriteOff(struct Pokenav_RibbonsSummaryMenu *menu)
{
    StartMonSpriteSlide(&gSprites[menu->monSpriteId], MON_SPRITE_X_ON, MON_SPRITE_X_OFF, 6);
}

static void SlideMonSpriteOn(struct Pokenav_RibbonsSummaryMenu *menu)
{
    // Switch to new mon sprite
    FreeAndDestroyMonPicSprite(menu->monSpriteId);
    menu->monSpriteId = DrawRibbonsMonFrontPic(MON_SPRITE_X_OFF, MON_SPRITE_Y);

    // Slide on
    StartMonSpriteSlide(&gSprites[menu->monSpriteId], MON_SPRITE_X_OFF, MON_SPRITE_X_ON, 6);
}

// Is Pokémon summary sprite still sliding off/on
static bool32 IsMonSpriteAnimating(struct Pokenav_RibbonsSummaryMenu *menu)
{
    return (gSprites[menu->monSpriteId].callback != SpriteCallbackDummy);
}

#define sCurrX    data[0]
#define sMoveIncr data[1]
#define sTime     data[2]
#define sDestX    data[3]

static void StartMonSpriteSlide(struct Sprite *sprite, s32 startX, s32 destX, s32 time)
{
    u32 delta = destX - startX;

    sprite->x = startX;
    sprite->sCurrX = startX << 4;
    sprite->sMoveIncr = (delta << 4) / time;
    sprite->sTime = time;
    sprite->sDestX = destX;

    sprite->callback = SpriteCB_MonSpriteSlide;
}

static void SpriteCB_MonSpriteSlide(struct Sprite *sprite)
{
    if (sprite->sTime != 0)
    {
        sprite->sTime--;
        sprite->sCurrX += sprite->sMoveIncr;
        sprite->x = sprite->sCurrX >> 4;
        if (sprite->x <= MON_SPRITE_X_OFF)
            sprite->invisible = TRUE;
        else
            sprite->invisible = FALSE;
    }
    else
    {
        sprite->x = sprite->sDestX;
        sprite->callback = SpriteCallbackDummy;
    }
}

#undef sCurrX
#undef sMoveIncr
#undef sTime
#undef sDestX

static void DrawAllRibbonsHgss(struct Pokenav_RibbonsSummaryMenu *menu)
{
    u32 *ribbonIds;
    u32 i;
    u32 pageStart = (GetSelectedPosition() / RIBBONS_PER_PAGE) * RIBBONS_PER_PAGE;

    // Each page contains at most 8 unscaled 32x32 HGSS icons. Their
    // logical slots preserve the gap between ordinary and gift ribbons.
    ClearRibbonsSummaryBg();

    ribbonIds = GetNormalRibbonIds(&sRibbonDraw_Total);
    for (i = 0; i < sRibbonDraw_Total; i++)
    {
        if (i >= pageStart && i < pageStart + RIBBONS_PER_PAGE)
            DrawRibbonHgss(i - pageStart, ribbonIds[i]);
    }

    ribbonIds = GetGiftRibbonIds(&sRibbonDraw_Total);
    for (i = 0; i < sRibbonDraw_Total; i++)
    {
        u32 slot = i + GIFT_RIBBON_START_POS;
        if (slot >= pageStart && slot < pageStart + RIBBONS_PER_PAGE)
            DrawRibbonHgss(slot - pageStart, ribbonIds[i]);
    }

    CopyBgTilemapBufferToVram(1);
}

// Tile zero is kept blank; all remaining BG1 tiles are original HGSS art.
static void ClearRibbonsSummaryBg(void)
{
    FillBgTilemapBufferRect_Palette0(1, 0, 0, 0, 32, 20);
}

static void DrawRibbonHgss(u32 slot, u32 ribbonId)
{
    u16 bgData[RIBBON_ICON_TILES * RIBBON_ICON_TILES];
    u32 destX = (slot % RIBBONS_PER_ROW) * RIBBON_ICON_TILES + RIBBON_GRID_LEFT_TILE;
    u32 destY = (slot / RIBBONS_PER_ROW) * RIBBON_ICON_TILES + RIBBON_GRID_TOP_TILE;

    BufferHgssRibbonGfxData(bgData, ribbonId);
    CopyToBgTilemapBufferRect(1, bgData, destX, destY, RIBBON_ICON_TILES, RIBBON_ICON_TILES);
}

// Below correspond to a ribbon icon in ribbons/icons.png and ribbons/icons_big.png; 0 at top, 11 at bottom
enum {
    RIBBONGFX_CHAMPION,
    RIBBONGFX_CONTEST_NORMAL,
    RIBBONGFX_CONTEST_SUPER,
    RIBBONGFX_CONTEST_HYPER,
    RIBBONGFX_CONTEST_MASTER,
    RIBBONGFX_WINNING,
    RIBBONGFX_VICTORY,
    RIBBONGFX_ARTIST,
    RIBBONGFX_EFFORT,
    RIBBONGFX_GIFT_1,
    RIBBONGFX_GIFT_2,
    RIBBONGFX_GIFT_3,
};

#define TO_PAL_OFFSET(palNum) ((palNum) - PALTAG_RIBBON_ICONS_1)

struct
{
    u16 tileNumOffset;
    u16 palNumOffset;
} static const sRibbonGfxData[] =
{
    [CHAMPION_RIBBON]      = { RIBBONGFX_CHAMPION,       TO_PAL_OFFSET(PALTAG_RIBBON_ICONS_1)},
    [COOL_RIBBON_NORMAL]   = { RIBBONGFX_CONTEST_NORMAL, TO_PAL_OFFSET(PALTAG_RIBBON_ICONS_1)},
    [COOL_RIBBON_SUPER]    = { RIBBONGFX_CONTEST_SUPER,  TO_PAL_OFFSET(PALTAG_RIBBON_ICONS_1)},
    [COOL_RIBBON_HYPER]    = { RIBBONGFX_CONTEST_HYPER,  TO_PAL_OFFSET(PALTAG_RIBBON_ICONS_1)},
    [COOL_RIBBON_MASTER]   = { RIBBONGFX_CONTEST_MASTER, TO_PAL_OFFSET(PALTAG_RIBBON_ICONS_1)},
    [BEAUTY_RIBBON_NORMAL] = { RIBBONGFX_CONTEST_NORMAL, TO_PAL_OFFSET(PALTAG_RIBBON_ICONS_2)},
    [BEAUTY_RIBBON_SUPER]  = { RIBBONGFX_CONTEST_SUPER,  TO_PAL_OFFSET(PALTAG_RIBBON_ICONS_2)},
    [BEAUTY_RIBBON_HYPER]  = { RIBBONGFX_CONTEST_HYPER,  TO_PAL_OFFSET(PALTAG_RIBBON_ICONS_2)},
    [BEAUTY_RIBBON_MASTER] = { RIBBONGFX_CONTEST_MASTER, TO_PAL_OFFSET(PALTAG_RIBBON_ICONS_2)},
    [CUTE_RIBBON_NORMAL]   = { RIBBONGFX_CONTEST_NORMAL, TO_PAL_OFFSET(PALTAG_RIBBON_ICONS_3)},
    [CUTE_RIBBON_SUPER]    = { RIBBONGFX_CONTEST_SUPER,  TO_PAL_OFFSET(PALTAG_RIBBON_ICONS_3)},
    [CUTE_RIBBON_HYPER]    = { RIBBONGFX_CONTEST_HYPER,  TO_PAL_OFFSET(PALTAG_RIBBON_ICONS_3)},
    [CUTE_RIBBON_MASTER]   = { RIBBONGFX_CONTEST_MASTER, TO_PAL_OFFSET(PALTAG_RIBBON_ICONS_3)},
    [SMART_RIBBON_NORMAL]  = { RIBBONGFX_CONTEST_NORMAL, TO_PAL_OFFSET(PALTAG_RIBBON_ICONS_4)},
    [SMART_RIBBON_SUPER]   = { RIBBONGFX_CONTEST_SUPER,  TO_PAL_OFFSET(PALTAG_RIBBON_ICONS_4)},
    [SMART_RIBBON_HYPER]   = { RIBBONGFX_CONTEST_HYPER,  TO_PAL_OFFSET(PALTAG_RIBBON_ICONS_4)},
    [SMART_RIBBON_MASTER]  = { RIBBONGFX_CONTEST_MASTER, TO_PAL_OFFSET(PALTAG_RIBBON_ICONS_4)},
    [TOUGH_RIBBON_NORMAL]  = { RIBBONGFX_CONTEST_NORMAL, TO_PAL_OFFSET(PALTAG_RIBBON_ICONS_5)},
    [TOUGH_RIBBON_SUPER]   = { RIBBONGFX_CONTEST_SUPER,  TO_PAL_OFFSET(PALTAG_RIBBON_ICONS_5)},
    [TOUGH_RIBBON_HYPER]   = { RIBBONGFX_CONTEST_HYPER,  TO_PAL_OFFSET(PALTAG_RIBBON_ICONS_5)},
    [TOUGH_RIBBON_MASTER]  = { RIBBONGFX_CONTEST_MASTER, TO_PAL_OFFSET(PALTAG_RIBBON_ICONS_5)},
    [WINNING_RIBBON]       = { RIBBONGFX_WINNING,        TO_PAL_OFFSET(PALTAG_RIBBON_ICONS_1)},
    [VICTORY_RIBBON]       = { RIBBONGFX_VICTORY,        TO_PAL_OFFSET(PALTAG_RIBBON_ICONS_1)},
    [ARTIST_RIBBON]        = { RIBBONGFX_ARTIST,         TO_PAL_OFFSET(PALTAG_RIBBON_ICONS_2)},
    [EFFORT_RIBBON]        = { RIBBONGFX_EFFORT,         TO_PAL_OFFSET(PALTAG_RIBBON_ICONS_3)},
    [MARINE_RIBBON]        = { RIBBONGFX_GIFT_1,         TO_PAL_OFFSET(PALTAG_RIBBON_ICONS_2)},
    [LAND_RIBBON]          = { RIBBONGFX_GIFT_1,         TO_PAL_OFFSET(PALTAG_RIBBON_ICONS_4)},
    [SKY_RIBBON]           = { RIBBONGFX_GIFT_1,         TO_PAL_OFFSET(PALTAG_RIBBON_ICONS_5)},
    [COUNTRY_RIBBON]       = { RIBBONGFX_GIFT_2,         TO_PAL_OFFSET(PALTAG_RIBBON_ICONS_4)},
    [NATIONAL_RIBBON]      = { RIBBONGFX_GIFT_2,         TO_PAL_OFFSET(PALTAG_RIBBON_ICONS_5)},
    [EARTH_RIBBON]         = { RIBBONGFX_GIFT_3,         TO_PAL_OFFSET(PALTAG_RIBBON_ICONS_1)},
    [WORLD_RIBBON]         = { RIBBONGFX_GIFT_3,         TO_PAL_OFFSET(PALTAG_RIBBON_ICONS_2)},
};

#undef TO_PAL_OFFSET

static void BufferHgssRibbonGfxData(u16 *dst, u32 ribbonId)
{
    u16 palNum = sRibbonGfxData[ribbonId].palNumOffset + 2;
    u16 tileNum = (sRibbonGfxData[ribbonId].tileNumOffset * 16) + 1;
    u32 i;

    // A 32x32 NCGR is 16 independent 8x8 tiles, row-major and unflipped.
    for (i = 0; i < RIBBON_ICON_TILES * RIBBON_ICON_TILES; i++)
        dst[i] = (tileNum + i) | (palNum << 12);
}

static const struct SpriteSheet sSpriteSheet_RibbonIconsBig =
{
    sRibbonIconsBig_Gfx, sizeof(sRibbonIconsBig_Gfx), GFXTAG_RIBBON_ICONS_BIG
};

static const struct SpritePalette sSpritePalettes_RibbonIcons[] =
{
    {sHgssRibbonBigPal0, PALTAG_RIBBON_ICONS_1},
    {sHgssRibbonBigPal1, PALTAG_RIBBON_ICONS_2},
    {sHgssRibbonBigPal2, PALTAG_RIBBON_ICONS_3},
    {sHgssRibbonBigPal3, PALTAG_RIBBON_ICONS_4},
    {sHgssRibbonBigPal4, PALTAG_RIBBON_ICONS_5},
    {},
};

static const struct OamData sOamData_RibbonIconBig =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_NORMAL,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(32x32),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(32x32),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
    .affineParam = 0
};

static const union AffineAnimCmd sAffineAnim_RibbonIconBig_Normal[] =
{
    AFFINEANIMCMD_FRAME(128, 128, 0, 0),
    AFFINEANIMCMD_END
};

static const union AffineAnimCmd sAffineAnim_RibbonIconBig_ZoomIn[] =
{
    AFFINEANIMCMD_FRAME(128, 128, 0, 0),
    AFFINEANIMCMD_FRAME(32, 32, 0, 4),
    AFFINEANIMCMD_END
};

static const union AffineAnimCmd sAffineAnim_RibbonIconBig_ZoomOut[] =
{
    AFFINEANIMCMD_FRAME(256, 256, 0, 0),
    AFFINEANIMCMD_FRAME(-32, -32, 0, 4),
    AFFINEANIMCMD_END
};

enum {
    RIBBONANIM_NORMAL,
    RIBBONANIM_ZOOM_IN,
    RIBBONANIM_ZOOM_OUT,
};

static const union AffineAnimCmd *const sAffineAnims_RibbonIconBig[] =
{
    [RIBBONANIM_NORMAL]   = sAffineAnim_RibbonIconBig_Normal,
    [RIBBONANIM_ZOOM_IN]  = sAffineAnim_RibbonIconBig_ZoomIn,
    [RIBBONANIM_ZOOM_OUT] = sAffineAnim_RibbonIconBig_ZoomOut
};

static const struct SpriteTemplate sSpriteTemplate_RibbonIconBig =
{
    .tileTag = GFXTAG_RIBBON_ICONS_BIG,
    .paletteTag = PALTAG_RIBBON_ICONS_1,
    .oam = &sOamData_RibbonIconBig,
    .affineAnims = sAffineAnims_RibbonIconBig,
};

// Create dummy sprite to be used for the zoomed in version of the selected ribbon
static void CreateBigRibbonSprite(struct Pokenav_RibbonsSummaryMenu *menu)
{
    u8 spriteId;

    LoadSpriteSheet(&sSpriteSheet_RibbonIconsBig);
    Pokenav_AllocAndLoadPalettes(sSpritePalettes_RibbonIcons);

    spriteId = CreateSprite(&sSpriteTemplate_RibbonIconBig, 0, 0, 0);
    menu->bigRibbonSprite = &gSprites[spriteId];
    menu->bigRibbonSprite->invisible = TRUE;
}

#define sInvisibleWhenDone data[0]

static void UpdateAndZoomInSelectedRibbon(struct Pokenav_RibbonsSummaryMenu *menu)
{
    u32 ribbonId;
    s16 x, y;
    GetHgssRibbonScreenPosition(GetSelectedPosition(), &x, &y);

    menu->bigRibbonSprite->x = x;
    menu->bigRibbonSprite->y = y;
    DrawPokeGearRibbonSelectionFocus(menu, TRUE);

    // Set new selected ribbon's gfx data
    ribbonId = GetRibbonId();
    menu->bigRibbonSprite->oam.tileNum = (sRibbonGfxData[ribbonId].tileNumOffset * 16) + GetSpriteTileStartByTag(GFXTAG_RIBBON_ICONS_BIG);
    menu->bigRibbonSprite->oam.paletteNum = IndexOfSpritePaletteTag(sRibbonGfxData[ribbonId].palNumOffset + PALTAG_RIBBON_ICONS_1);

    // Start zoom in animation
    StartSpriteAffineAnim(menu->bigRibbonSprite, RIBBONANIM_ZOOM_IN);
    menu->bigRibbonSprite->invisible = FALSE;
    menu->bigRibbonSprite->sInvisibleWhenDone = FALSE;
    menu->bigRibbonSprite->callback = SpriteCB_WaitForRibbonAnimation;
}

// Start animation to zoom out of selected ribbon
static void ZoomOutSelectedRibbon(struct Pokenav_RibbonsSummaryMenu *menu)
{
    DrawPokeGearRibbonSelectionFocus(menu, FALSE);
    menu->bigRibbonSprite->sInvisibleWhenDone = TRUE;
    StartSpriteAffineAnim(menu->bigRibbonSprite, RIBBONANIM_ZOOM_OUT);
    menu->bigRibbonSprite->callback = SpriteCB_WaitForRibbonAnimation;
}

static bool32 IsRibbonAnimating(struct Pokenav_RibbonsSummaryMenu *menu)
{
    return (menu->bigRibbonSprite->callback != SpriteCallbackDummy);
}

static void SpriteCB_WaitForRibbonAnimation(struct Sprite *sprite)
{
    if (sprite->affineAnimEnded)
    {
        sprite->invisible = sprite->sInvisibleWhenDone;
        sprite->callback = SpriteCallbackDummy;
    }
}

#if TESTING
// Test-only adapter around the *live* ribbon movement functions.
// These functions are compiled only into the mGBA test binary.
u16 Test_HgssRibbonNavigate(u16 selectedPos, u8 normalCount, u8 giftCount, u8 direction)
{
    struct Pokenav_RibbonsSummaryList list = {0};

    list.selectedPos = selectedPos;
    list.numNormalRibbons = normalCount;
    list.numGiftRibbons = giftCount;
    if (normalCount != 0)
        list.normalRibbonLastRowStart = ((normalCount - 1) / RIBBONS_PER_ROW) * RIBBONS_PER_ROW;

    switch (direction)
    {
    case HGSS_RIBBON_TEST_UP:
        TrySelectRibbonUp(&list);
        break;
    case HGSS_RIBBON_TEST_DOWN:
        TrySelectRibbonDown(&list);
        break;
    case HGSS_RIBBON_TEST_LEFT:
        TrySelectRibbonLeft(&list);
        break;
    case HGSS_RIBBON_TEST_RIGHT:
        TrySelectRibbonRight(&list);
        break;
    }
    return list.selectedPos;
}

void Test_HgssRibbonScreenPosition(u16 selectedPos, s16 *x, s16 *y)
{
    GetHgssRibbonScreenPosition(selectedPos, x, y);
}
#endif
