#include "global.h"
#include "bg.h"
#include "data.h"
#include "decompress.h"
#include "dma3.h"
#include "international_string_util.h"
#include "main.h"
#include "match_call.h"
#include "menu.h"
#include "overworld.h"
#include "palette.h"
#include "pokenav.h"
#include "region_map.h"
#include "sound.h"
#include "sprite.h"
#include "string_util.h"
#include "strings.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "trig.h"
#include "window.h"
#include "constants/game_stat.h"
#include "constants/region_map_sections.h"
#include "constants/songs.h"

#define GFXTAG_TRAINER_PIC 8
#define PALTAG_TRAINER_PIC 13

struct Pokenav_MatchCallGfx
{
    bool32 (*isTaskActiveCB)(void);
    u32 loopTaskId;
    u8 filler8[6];
    bool8 skipHangUpSE;
    bool8 newRematchRequest;
    u16 locWindowId;
    u16 infoBoxWindowId;
    u16 msgBoxWindowId;
    u16 actionWindowId;
    s16 pageDelta;
    u8 unused18;
    u8 unused19;
    u16 trainerPicPalOffset;
    struct Sprite *trainerPicSprite;
    u8 bgTilemapBuffer1[BG_SCREEN_SIZE];
    u8 unusedTilemapBuffer[BG_SCREEN_SIZE];
    u8 bgTilemapBuffer2[BG_SCREEN_SIZE];
    u8 *trainerPicGfxPtr;
    u8 trainerPicGfx[TRAINER_PIC_SIZE];
    u8 trainerPicPal[0x20];
};

static bool32 GetCurrentLoopedTaskActive(void);
static u32 LoopedTask_OpenMatchCall(s32);
static void CreateMatchCallList(void);
static void DestroyMatchCallList(void);
static void FreeMatchCallSprites(void);
static void LoadCallWindowAndFade(struct Pokenav_MatchCallGfx *);
static void DrawHgssPhoneCallSurface(struct Pokenav_MatchCallGfx *);
static void DrawHgssTrainerCardPortraitPanel(u16);
static void DrawMatchCallLeftColumnWindows(struct Pokenav_MatchCallGfx *);
static void UpdateMatchCallInfoBox(struct Pokenav_MatchCallGfx *);
static void PrintMatchCallLocation(struct Pokenav_MatchCallGfx *, int);
static void AllocMatchCallSprites(void);
static void SetPokeballIconsFlashing(bool32);
static void DrawHgssPhoneActionMenu(struct Pokenav_MatchCallGfx *);
static void PrintMatchCallSelectionOptions(struct Pokenav_MatchCallGfx *);
static bool32 WaitForActionMenu(struct Pokenav_MatchCallGfx *);
static bool32 IsDma3ManagerBusyWithBgCopy1(struct Pokenav_MatchCallGfx *);
static void UpdateWindowsReturnToTrainerList(struct Pokenav_MatchCallGfx *);
static void DrawMsgBoxForMatchCallMsg(struct Pokenav_MatchCallGfx *);
static bool32 IsDma3ManagerBusyWithBgCopy2(struct Pokenav_MatchCallGfx *);
static void PrintCallingDots(struct Pokenav_MatchCallGfx *);
static bool32 WaitForCallingDotsText(struct Pokenav_MatchCallGfx *);
static void PrintMatchCallMessage(struct Pokenav_MatchCallGfx *);
static bool32 WaitForMatchCallMessageText(struct Pokenav_MatchCallGfx *);
static void DrawMsgBoxForCloseByMsg(struct Pokenav_MatchCallGfx *);
static void PrintTrainerIsCloseBy(struct Pokenav_MatchCallGfx *);
static bool32 WaitForTrainerIsCloseByText(struct Pokenav_MatchCallGfx *);
static void EraseCallMessageBox(struct Pokenav_MatchCallGfx *);
static bool32 WaitForCallMessageBoxErase(struct Pokenav_MatchCallGfx *);
static void UpdateWindowsToShowCheckPage(struct Pokenav_MatchCallGfx *);
static void LoadCheckPageTrainerPic(struct Pokenav_MatchCallGfx *);
static bool32 WaitForTrainerPic(struct Pokenav_MatchCallGfx *);
static void TrainerPicSlideOffscreen(struct Pokenav_MatchCallGfx *);
static void Task_FlashPokeballIcons(u8);
static void DrawHgssPhoneContactRow(u16, u32, u32);
static void TryDrawRematchPokeballIcon(u16, u32, u32);
static void PrintNumberRegisteredLabel(u16);
static void PrintNumberRegistered(u16);
static void PrintNumberOfBattlesLabel(u16);
static void PrintNumberOfBattles(u16);
static void PrintMatchCallInfoLabel(u16, const u8 *, int);
static void PrintMatchCallInfoNumber(u16, const u8 *, int);
static void CloseMatchCallSelectOptionsWindow(struct Pokenav_MatchCallGfx *);
static struct Sprite *CreateTrainerPicSprite(void);
static void SpriteCB_TrainerPicSlideOnscreen(struct Sprite *);
static void SpriteCB_TrainerPicSlideOffscreen(struct Sprite *);
static u32 MatchCallListCursorDown(s32);
static u32 MatchCallListCursorUp(s32);
static u32 MatchCallListPageDown(s32);
static u32 MatchCallListPageUp(s32);
static u32 SelectMatchCallEntry(s32);
static u32 MoveMatchCallOptionsCursor(s32);
static u32 CancelMatchCallSelection(s32);
static u32 DoMatchCallMessage(s32);
static u32 DoTrainerCloseByMessage(s32);
static u32 CloseMatchCallMessage(s32);
static u32 ShowCheckPage(s32);
static u32 ShowCheckPageUp(s32);
static u32 ShowCheckPageDown(s32);
static u32 ExitCheckPage(s32);
static u32 ExitMatchCall(s32);

static const u32 sHgssMatchCallContact_Gfx[] = INCBIN_U32("graphics/gen4_ui/hgss_pokegear/match_call_contact.4bpp");
static const u16 sHgssMatchCallContact_Tilemap[] = INCBIN_U16("graphics/gen4_ui/hgss_pokegear/match_call_contact.tilemap.bin");
static const u16 sHgssMatchCallContact_Pal[] = INCBIN_U16("graphics/gen4_ui/hgss_pokegear/match_call_contact.gbapal");
static const u32 sHgssMatchCallCall_Gfx[] = INCBIN_U32("graphics/gen4_ui/hgss_pokegear/match_call_call.4bpp");
static const u16 sHgssMatchCallCall_Tilemap[] = INCBIN_U16("graphics/gen4_ui/hgss_pokegear/match_call_call.tilemap.bin");
static const u16 sHgssMatchCallCall_Pal[] = INCBIN_U16("graphics/gen4_ui/hgss_pokegear/match_call_call.gbapal");
static const u16 sHgssMatchCallCallText_Pal[] = INCBIN_U16("graphics/gen4_ui/hgss_pokegear/match_call_call_text.gbapal");
static const u32 sHgssActionMenu_Gfx[] = INCBIN_U32("graphics/gen4_ui/hgss_pokegear/match_call_action_menu.4bpp");
static const u16 sHgssActionMenu_Pal[] = INCBIN_U16("graphics/gen4_ui/hgss_pokegear/match_call_action_menu.gbapal");
static const u32 sHgssCheckPortrait_Gfx[] = INCBIN_U32("graphics/gen4_ui/hgss_pokegear/match_call_check_portrait.4bpp");
static const u16 sHgssCheckPortrait_Pal[] = INCBIN_U16("graphics/gen4_ui/hgss_pokegear/match_call_check_portrait.gbapal");
static const u16 sHgssContactRows_Pal[] = INCBIN_U16("graphics/gen4_ui/hgss_pokegear/match_call_contact_rows.gbapal");
static const u32 sHgssRematchBadge_Gfx[] = INCBIN_U32("graphics/gen4_ui/hgss_pokegear/rematch_badge.4bpp");
static const u16 sHgssRematchBadge_Pal[] = INCBIN_U16("graphics/gen4_ui/hgss_pokegear/rematch_badge.gbapal");

enum {
    HGSS_MATCH_CALL_CONTACT_TILE_OFFSET = 0x100,
    HGSS_MATCH_CALL_CALL_TILE_OFFSET = 0x80,
    HGSS_MATCH_CALL_CONTACT_PALETTE = 6,
    HGSS_MATCH_CALL_ROWS_PALETTE = 7,
    HGSS_MATCH_CALL_CALL_PALETTE = 9,
    HGSS_MATCH_CALL_CHECK_PALETTE = 10,
    HGSS_MATCH_CALL_ROW_HEIGHT = 24,
    HGSS_MATCH_CALL_BUFFER_ROWS = 8,
    HGSS_PHONE_BADGE_ACTIVE = 0x5000,
    HGSS_PHONE_BADGE_DISABLED = 0x5004,
    HGSS_PHONE_BADGE_EMPTY = 0x5008,
    HGSS_PHONE_BADGE_COLUMN = 28,
    HGSS_PHONE_BADGE_TILE_COUNT = 9,
};

static const u8 gText_NumberRegistered[] = _("CONTACTS");
static const u8 gText_NumberOfBattles[] = _("BATTLES");
static const u8 gText_TrainerCloseBy[] = _("That TRAINER is close by.\nTalk to the TRAINER in person!");
static const u8 gText_Unknown[] = _("UNKNOWN");

static const struct BgTemplate sMatchCallBgTemplates[3] =
{
    {
        .bg = 1,
        .charBaseIndex = 3,
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
        .baseTile = 0x80
    },
    {
        .bg = 3,
        .charBaseIndex = 1,
        .mapBaseIndex = 0x07,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 3,
        .baseTile = 0
    }
};

static const LoopedTask sMatchCallLoopTaskFuncs[] =
{
    [POKENAV_MC_FUNC_NONE]                = NULL,
    [POKENAV_MC_FUNC_DOWN]                = MatchCallListCursorDown,
    [POKENAV_MC_FUNC_UP]                  = MatchCallListCursorUp,
    [POKENAV_MC_FUNC_PG_DOWN]             = MatchCallListPageDown,
    [POKENAV_MC_FUNC_PG_UP]               = MatchCallListPageUp,
    [POKENAV_MC_FUNC_SELECT]              = SelectMatchCallEntry,
    [POKENAV_MC_FUNC_MOVE_OPTIONS_CURSOR] = MoveMatchCallOptionsCursor,
    [POKENAV_MC_FUNC_CANCEL]              = CancelMatchCallSelection,
    [POKENAV_MC_FUNC_CALL_MSG]            = DoMatchCallMessage,
    [POKENAV_MC_FUNC_NEARBY_MSG]          = DoTrainerCloseByMessage,
    [POKENAV_MC_FUNC_EXIT_CALL]           = CloseMatchCallMessage,
    [POKENAV_MC_FUNC_SHOW_CHECK_PAGE]     = ShowCheckPage,
    [POKENAV_MC_FUNC_CHECK_PAGE_UP]       = ShowCheckPageUp,
    [POKENAV_MC_FUNC_CHECK_PAGE_DOWN]     = ShowCheckPageDown,
    [POKENAV_MC_FUNC_EXIT_CHECK_PAGE]     = ExitCheckPage,
    [POKENAV_MC_FUNC_EXIT]                = ExitMatchCall
};

static const struct WindowTemplate sMatchCallLocationWindowTemplate =
{
    .bg = 2,
    .tilemapLeft = 0,
    .tilemapTop = 5,
    .width = 11,
    .height = 2,
    .paletteNum = HGSS_MATCH_CALL_CONTACT_PALETTE,
    .baseBlock = 16
};

static const struct WindowTemplate sMatchCallInfoBoxWindowTemplate =
{
    .bg = 2,
    .tilemapLeft = 0,
    .tilemapTop = 9,
    .width = 11,
    .height = 8,
    .paletteNum = HGSS_MATCH_CALL_CONTACT_PALETTE,
    .baseBlock = 38
};

// BG1 tiles 256..435 are separate from the call window (10..121).
// Retail 18x10 footprint translated left one tile; BG palette 8 is reserved.
static const struct WindowTemplate sHgssActionMenuWindowTemplate =
{
    .bg = 1,
    .tilemapLeft = 12,
    .tilemapTop = 9,
    .width = 18,
    .height = 10,
    .paletteNum = 8,
    .baseBlock = 256
};

static const u8 *const sMatchCallOptionTexts[MATCH_CALL_OPTION_COUNT] =
{
    [MATCH_CALL_OPTION_CALL]   = COMPOUND_STRING("CALL"),
    [MATCH_CALL_OPTION_CHECK]  = COMPOUND_STRING("CHECK"),
    [MATCH_CALL_OPTION_CANCEL] = COMPOUND_STRING("CANCEL")
};

// The series of 5 dots that appear when someone is called with Match Call
static const u8 sText_CallingDots[] = _("CALLING{PAUSE 4}·{PAUSE 4}·{PAUSE 4}·{PAUSE 4}·\p");

// Retail HGSS Phone window 0 is (2,19), 27x4. The GBA adaptation
// removes only four blank source rows, so its live origin becomes (2,15).
static const struct WindowTemplate sHgssCallMsgBoxWindowTemplate =
{
    .bg = 1,
    .tilemapLeft = 2,
    .tilemapTop = 15,
    .width = 27,
    .height = 4,
    .paletteNum = 1,
    .baseBlock = 10
};

static const struct OamData sTrainerPicOamData =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(64x64),
    .x = 0,
    .size = SPRITE_SIZE(64x64),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
};

static const struct SpriteTemplate sTrainerPicSpriteTemplate =
{
    .tileTag = GFXTAG_TRAINER_PIC,
    .paletteTag = PALTAG_TRAINER_PIC,
    .oam = &sTrainerPicOamData,
};

bool32 OpenMatchCall(void)
{
    struct Pokenav_MatchCallGfx *gfx = AllocSubstruct(POKENAV_SUBSTRUCT_MATCH_CALL_OPEN, sizeof(struct Pokenav_MatchCallGfx));
    if (!gfx)
        return FALSE;

    gfx->unused19 = 0;
    gfx->actionWindowId = WINDOW_NONE;
    gfx->loopTaskId = CreateLoopedTask(LoopedTask_OpenMatchCall, 1);
    gfx->isTaskActiveCB = GetCurrentLoopedTaskActive;
    return TRUE;
}

void CreateMatchCallLoopedTask(s32 index)
{
    struct Pokenav_MatchCallGfx *gfx = GetSubstructPtr(POKENAV_SUBSTRUCT_MATCH_CALL_OPEN);
    gfx->loopTaskId = CreateLoopedTask(sMatchCallLoopTaskFuncs[index], 1);
    gfx->isTaskActiveCB = GetCurrentLoopedTaskActive;
}

bool32 IsMatchCallLoopedTaskActive(void)
{
    struct Pokenav_MatchCallGfx *gfx = GetSubstructPtr(POKENAV_SUBSTRUCT_MATCH_CALL_OPEN);
    return gfx->isTaskActiveCB();
}

void FreeMatchCallSubstruct2(void)
{
    struct Pokenav_MatchCallGfx *gfx = GetSubstructPtr(POKENAV_SUBSTRUCT_MATCH_CALL_OPEN);
    CloseMatchCallSelectOptionsWindow(gfx);
    FreeMatchCallSprites();
    DestroyMatchCallList();
    RemoveWindow(gfx->infoBoxWindowId);
    RemoveWindow(gfx->locWindowId);
    RemoveWindow(gfx->msgBoxWindowId);
    FreePokenavSubstruct(POKENAV_SUBSTRUCT_MATCH_CALL_OPEN);
}

static bool32 GetCurrentLoopedTaskActive(void)
{
    struct Pokenav_MatchCallGfx *gfx = GetSubstructPtr(POKENAV_SUBSTRUCT_MATCH_CALL_OPEN);
    return IsLoopedTaskActive(gfx->loopTaskId);
}

static u32 LoopedTask_OpenMatchCall(s32 state)
{
    struct Pokenav_MatchCallGfx *gfx = GetSubstructPtr(POKENAV_SUBSTRUCT_MATCH_CALL_OPEN);
    switch (state)
    {
    case 0:
        InitBgTemplates(sMatchCallBgTemplates, ARRAY_COUNT(sMatchCallBgTemplates));
        ChangeBgX(2, 0, BG_COORD_SET);
        ChangeBgY(2, 0, BG_COORD_SET);
        SetBgTilemapBuffer(2, gfx->bgTilemapBuffer2);

        // CI #658 validated the retail HGSS Phone contact surface. It is now
        // the sole BG2 source; the superseded Emerald ui.png/ui.bin path is gone.
        LoadBgTiles(2, sHgssMatchCallContact_Gfx, sizeof(sHgssMatchCallContact_Gfx), HGSS_MATCH_CALL_CONTACT_TILE_OFFSET);
        CopyToBgTilemapBuffer(2, sHgssMatchCallContact_Tilemap, sizeof(sHgssMatchCallContact_Tilemap), 0);
        CopyPaletteIntoBufferUnfaded(sHgssMatchCallContact_Pal, BG_PLTT_ID(HGSS_MATCH_CALL_CONTACT_PALETTE), sizeof(sHgssMatchCallContact_Pal));
        CopyBgTilemapBufferToVram(2);
        return LT_INC_AND_PAUSE;
    case 1:
        BgDmaFill(1, 0, 0, 1);
        SetBgTilemapBuffer(1, gfx->bgTilemapBuffer1);
        FillBgTilemapBufferRect_Palette0(1, 0x1000, 0, 0, 32, 20);
        // Retail HGSS Phone call-text windows use member-4 NCLR palette bank 1.
        CopyPaletteIntoBufferUnfaded(sHgssMatchCallCallText_Pal, BG_PLTT_ID(1), sizeof(sHgssMatchCallCallText_Pal));
        CopyBgTilemapBufferToVram(1);
        return LT_INC_AND_PAUSE;
    case 2:
        if (FreeTempTileDataBuffersIfPossible())
            return LT_PAUSE;

        LoadCallWindowAndFade(gfx);
        LoadBgTiles(3, sHgssRematchBadge_Gfx, sizeof(sHgssRematchBadge_Gfx), 0);
        CopyPaletteIntoBufferUnfaded(sHgssContactRows_Pal, BG_PLTT_ID(HGSS_MATCH_CALL_ROWS_PALETTE), sizeof(sHgssContactRows_Pal));
        CopyPaletteIntoBufferUnfaded(sHgssRematchBadge_Pal, BG_PLTT_ID(5), sizeof(sHgssRematchBadge_Pal));
        return LT_INC_AND_PAUSE;
    case 3:
        if (FreeTempTileDataBuffersIfPossible() || !IsMatchCallListInitFinished())
            return LT_PAUSE;

        CreateMatchCallList();
        return LT_INC_AND_PAUSE;
    case 4:
        if (IsCreatePokenavListTaskActive())
            return LT_PAUSE;

        DrawMatchCallLeftColumnWindows(gfx);
        return LT_INC_AND_PAUSE;
    case 5:
        UpdateMatchCallInfoBox(gfx);
        PrintMatchCallLocation(gfx, 0);
        return LT_INC_AND_PAUSE;
    case 6:
        ChangeBgX(1, 0, BG_COORD_SET);
        ChangeBgY(1, 0, BG_COORD_SET);
        ShowBg(2);
        ShowBg(3);
        ShowBg(1);
        AllocMatchCallSprites();
        PokenavFadeScreen(POKENAV_FADE_FROM_BLACK);
        return LT_INC_AND_PAUSE;
    case 7:
        if (IsPaletteFadeActive())
            return LT_PAUSE;

        SetPokeballIconsFlashing(TRUE);
        return LT_FINISH;
    default:
        return LT_FINISH;
    }
}

static u32 MatchCallListCursorDown(s32 state)
{
    struct Pokenav_MatchCallGfx *gfx = GetSubstructPtr(POKENAV_SUBSTRUCT_MATCH_CALL_OPEN);
    switch (state)
    {
    case 0:
        switch (PokenavList_MoveCursorDown())
        {
        case 0:
            break;
        case 1:
            PlaySE(SE_SELECT);
            return LT_SET_STATE(2);
        case 2:
            PlaySE(SE_SELECT);
            // fall through
        default:
            return LT_INC_AND_PAUSE;
        }
        break;
    case 1:
        if (PokenavList_IsMoveWindowTaskActive())
            return LT_PAUSE;

        PrintMatchCallLocation(gfx, 0);
        return LT_INC_AND_PAUSE;
    case 2:
        PrintMatchCallLocation(gfx, 0);
        return LT_INC_AND_PAUSE;
    case 3:
        if (IsDma3ManagerBusyWithBgCopy())
            return LT_PAUSE;
        break;
    }
    return LT_FINISH;
}

static u32 MatchCallListCursorUp(s32 state)
{
    struct Pokenav_MatchCallGfx *gfx = GetSubstructPtr(POKENAV_SUBSTRUCT_MATCH_CALL_OPEN);
    switch (state)
    {
    case 0:
        switch (PokenavList_MoveCursorUp())
        {
        case 0:
            break;
        case 1:
            PlaySE(SE_SELECT);
            return LT_SET_STATE(2);
        case 2:
            PlaySE(SE_SELECT);
            // fall through
        default:
            return LT_INC_AND_PAUSE;
        }
        break;
    case 1:
        if (PokenavList_IsMoveWindowTaskActive())
            return LT_PAUSE;

        PrintMatchCallLocation(gfx, 0);
        return LT_INC_AND_PAUSE;
    case 2:
        PrintMatchCallLocation(gfx, 0);
        return LT_INC_AND_PAUSE;
    case 3:
        if (IsDma3ManagerBusyWithBgCopy())
            return LT_PAUSE;
        break;
    }
    return LT_FINISH;
}

static u32 MatchCallListPageDown(s32 state)
{
    struct Pokenav_MatchCallGfx *gfx = GetSubstructPtr(POKENAV_SUBSTRUCT_MATCH_CALL_OPEN);
    switch (state)
    {
    case 0:
        switch (PokenavList_PageDown())
        {
        case 0:
            break;
        case 1:
            PlaySE(SE_SELECT);
            return LT_SET_STATE(2);
        case 2:
            PlaySE(SE_SELECT);
            // fall through
        default:
            return LT_INC_AND_PAUSE;
        }
        break;
    case 1:
        if (PokenavList_IsMoveWindowTaskActive())
            return LT_PAUSE;

        PrintMatchCallLocation(gfx, 0);
        return LT_INC_AND_PAUSE;
    case 2:
        PrintMatchCallLocation(gfx, 0);
        return LT_INC_AND_PAUSE;
    case 3:
        if (IsDma3ManagerBusyWithBgCopy())
            return LT_PAUSE;
        break;
    }
    return LT_FINISH;
}

static u32 MatchCallListPageUp(s32 state)
{
    struct Pokenav_MatchCallGfx *gfx = GetSubstructPtr(POKENAV_SUBSTRUCT_MATCH_CALL_OPEN);
    switch (state)
    {
    case 0:
        switch (PokenavList_PageUp())
        {
        case 0:
            break;
        case 1:
            PlaySE(SE_SELECT);
            return LT_SET_STATE(2);
        case 2:
            PlaySE(SE_SELECT);
            // fall through
        default:
            return LT_INC_AND_PAUSE;
        }
        break;
    case 1:
        if (PokenavList_IsMoveWindowTaskActive())
            return LT_PAUSE;

        PrintMatchCallLocation(gfx, 0);
        return LT_INC_AND_PAUSE;
    case 2:
        PrintMatchCallLocation(gfx, 0);
        return LT_INC_AND_PAUSE;
    case 3:
        if (IsDma3ManagerBusyWithBgCopy())
            return LT_PAUSE;
        break;
    }
    return LT_FINISH;
}

static u32 SelectMatchCallEntry(s32 state)
{
    struct Pokenav_MatchCallGfx *gfx = GetSubstructPtr(POKENAV_SUBSTRUCT_MATCH_CALL_OPEN);
    switch (state)
    {
    case 0:
        PlaySE(SE_SELECT);
        PrintMatchCallSelectionOptions(gfx);
        PrintHelpBarText(HELPBAR_MC_CALL_MENU);
        return LT_INC_AND_PAUSE;
    case 1:
        if (WaitForActionMenu(gfx))
            return LT_PAUSE;
        break;
    }

    return LT_FINISH;
}

static u32 MoveMatchCallOptionsCursor(s32 state)
{
    struct Pokenav_MatchCallGfx *gfx;
    if (state == 0)
    {
        PlaySE(SE_SELECT);
        gfx = GetSubstructPtr(POKENAV_SUBSTRUCT_MATCH_CALL_OPEN);
        DrawHgssPhoneActionMenu(gfx);
        return LT_INC_AND_PAUSE;
    }
    return IsDma3ManagerBusyWithBgCopy() ? LT_PAUSE : LT_FINISH;
}

static u32 CancelMatchCallSelection(s32 state)
{
    struct Pokenav_MatchCallGfx *gfx = GetSubstructPtr(POKENAV_SUBSTRUCT_MATCH_CALL_OPEN);
    switch (state)
    {
    case 0:
        PlaySE(SE_SELECT);
        UpdateWindowsReturnToTrainerList(gfx);
        PrintHelpBarText(HELPBAR_MC_TRAINER_LIST);
        return LT_INC_AND_PAUSE;
    case 1:
        if (IsDma3ManagerBusyWithBgCopy1(gfx))
            return LT_PAUSE;
        break;
    }

    return LT_FINISH;
}

static u32 DoMatchCallMessage(s32 state)
{
    struct Pokenav_MatchCallGfx *gfx = GetSubstructPtr(POKENAV_SUBSTRUCT_MATCH_CALL_OPEN);
    switch (state)
    {
    case 0:
        PokenavList_ToggleVerticalArrows(TRUE);
        PrintHelpBarText(HELPBAR_NONE);
        DrawMsgBoxForMatchCallMsg(gfx);
        return LT_INC_AND_PAUSE;
    case 1:
        if (IsDma3ManagerBusyWithBgCopy2(gfx))
            return LT_PAUSE;

        PrintCallingDots(gfx);
        PlaySE(SE_POKENAV_CALL);
        gfx->skipHangUpSE = FALSE;
        return LT_INC_AND_PAUSE;
    case 2:
        if (WaitForCallingDotsText(gfx))
            return LT_PAUSE;

        PrintMatchCallMessage(gfx);
        return LT_INC_AND_PAUSE;
    case 3:
        if (WaitForMatchCallMessageText(gfx))
            return LT_PAUSE;
        break;
    }

    return LT_FINISH;
}

static u32 DoTrainerCloseByMessage(s32 state)
{
    struct Pokenav_MatchCallGfx *gfx = GetSubstructPtr(POKENAV_SUBSTRUCT_MATCH_CALL_OPEN);
    switch (state)
    {
    case 0:
        PlaySE(SE_SELECT);
        PrintHelpBarText(HELPBAR_NONE);
        DrawMsgBoxForCloseByMsg(gfx);
        PokenavList_ToggleVerticalArrows(TRUE);
        gfx->skipHangUpSE = TRUE;
        return LT_INC_AND_PAUSE;
    case 1:
        if (IsDma3ManagerBusyWithBgCopy2(gfx))
            return LT_PAUSE;

        PrintTrainerIsCloseBy(gfx);
        return LT_INC_AND_PAUSE;
    case 2:
        if (WaitForTrainerIsCloseByText(gfx))
            return LT_PAUSE;
        break;
    }

    return LT_FINISH;
}

static u32 CloseMatchCallMessage(s32 state)
{
    struct Pokenav_MatchCallGfx *gfx = GetSubstructPtr(POKENAV_SUBSTRUCT_MATCH_CALL_OPEN);
    u32 result = LT_INC_AND_PAUSE;

    switch (state)
    {
    case 0:
        if (!gfx->skipHangUpSE)
            PlaySE(SE_POKENAV_HANG_UP);

        PlaySE(SE_SELECT);
        break;
    case 1:
        EraseCallMessageBox(gfx);
        break;
    case 2:
        if (WaitForCallMessageBoxErase(gfx))
            result = LT_PAUSE;
        break;
    case 3:
        UpdateWindowsReturnToTrainerList(gfx);
        break;
    case 4:
        if (IsDma3ManagerBusyWithBgCopy1(gfx))
            result = LT_PAUSE;

        PrintHelpBarText(HELPBAR_MC_TRAINER_LIST);
        break;
    case 5:
        if (WaitForHelpBar())
        {
            result = LT_PAUSE;
        }
        else
        {
            if (gfx->newRematchRequest)
            {
                // This call was a new rematch request,
                // add the Pokéball icon to their entry
                PokenavList_DrawCurrentItemIcon();
                result = LT_INC_AND_CONTINUE;
            }
            else
            {
                PokenavList_ToggleVerticalArrows(FALSE);
                result = LT_FINISH;
            }
        }
        break;
    case 6:
        if (IsDma3ManagerBusyWithBgCopy())
        {
            result = LT_PAUSE;
        }
        else
        {
            PokenavList_ToggleVerticalArrows(FALSE);
            result = LT_FINISH;
        }
        break;
    }

    return result;
}

static u32 ShowCheckPage(s32 state)
{
    struct Pokenav_MatchCallGfx *gfx = GetSubstructPtr(POKENAV_SUBSTRUCT_MATCH_CALL_OPEN);
    switch (state)
    {
    case 0:
        PlaySE(SE_SELECT);
        PokenavList_EraseListForCheckPage();
        UpdateWindowsToShowCheckPage(gfx);
        return LT_INC_AND_PAUSE;
    case 1:
        if (PokenavList_IsTaskActive() || IsDma3ManagerBusyWithBgCopy1(gfx))
            return LT_PAUSE;

        PrintHelpBarText(HELPBAR_MC_CHECK_PAGE);
        return LT_INC_AND_PAUSE;
    case 2:
        PrintCheckPageInfo(0);
        LoadCheckPageTrainerPic(gfx);
        return LT_INC_AND_PAUSE;
    case 3:
        if (PokenavList_IsTaskActive() || WaitForTrainerPic(gfx) || WaitForHelpBar())
            return LT_PAUSE;
        break;
    }

    return LT_FINISH;
}

static u32 ShowCheckPageDown(s32 state)
{
    int topId;
    int delta;
    struct Pokenav_MatchCallGfx *gfx = GetSubstructPtr(POKENAV_SUBSTRUCT_MATCH_CALL_OPEN);
    switch (state)
    {
    case 0:
        topId = PokenavList_GetTopIndex();
        delta = GetIndexDeltaOfNextCheckPageDown(topId);
        if (delta)
        {
            PlaySE(SE_SELECT);
            gfx->pageDelta = delta;
            TrainerPicSlideOffscreen(gfx);
            return LT_INC_AND_PAUSE;
        }
        break;
    case 1:
        if (WaitForTrainerPic(gfx))
            return LT_PAUSE;

        PrintMatchCallLocation(gfx, gfx->pageDelta);
        return LT_INC_AND_PAUSE;
    case 2:
        PrintCheckPageInfo(gfx->pageDelta);
        return LT_INC_AND_PAUSE;
    case 3:
        LoadCheckPageTrainerPic(gfx);
        return LT_INC_AND_PAUSE;
    case 4:
        if (PokenavList_IsTaskActive() || WaitForTrainerPic(gfx))
            return LT_PAUSE;
        break;
    }

    return LT_FINISH;
}

static u32 ExitCheckPage(s32 state)
{
    struct Pokenav_MatchCallGfx *gfx = GetSubstructPtr(POKENAV_SUBSTRUCT_MATCH_CALL_OPEN);
    switch (state)
    {
    case 0:
        PlaySE(SE_SELECT);
        TrainerPicSlideOffscreen(gfx);
        PokenavList_ReshowListFromCheckPage();
        return LT_INC_AND_PAUSE;
    case 1:
        if (PokenavList_IsTaskActive() || WaitForTrainerPic(gfx))
            return LT_PAUSE;

        PrintHelpBarText(HELPBAR_MC_TRAINER_LIST);
        UpdateMatchCallInfoBox(gfx);
        return LT_INC_AND_PAUSE;
    case 2:
        if (IsDma3ManagerBusyWithBgCopy())
            return LT_PAUSE;
        break;
    }

    return LT_FINISH;
}

static u32 ShowCheckPageUp(s32 state)
{
    int topId;
    int delta;
    struct Pokenav_MatchCallGfx *gfx = GetSubstructPtr(POKENAV_SUBSTRUCT_MATCH_CALL_OPEN);
    switch (state)
    {
    case 0:
        topId = PokenavList_GetTopIndex();
        delta = GetIndexDeltaOfNextCheckPageUp(topId);
        if (delta)
        {
            PlaySE(SE_SELECT);
            gfx->pageDelta = delta;
            TrainerPicSlideOffscreen(gfx);
            return LT_INC_AND_PAUSE;
        }
        break;
    case 1:
        if (WaitForTrainerPic(gfx))
            return LT_PAUSE;

        PrintMatchCallLocation(gfx, gfx->pageDelta);
        return LT_INC_AND_PAUSE;
    case 2:
        PrintCheckPageInfo(gfx->pageDelta);
        return LT_INC_AND_PAUSE;
    case 3:
        LoadCheckPageTrainerPic(gfx);
        return LT_INC_AND_PAUSE;
    case 4:
        if (PokenavList_IsTaskActive() || WaitForTrainerPic(gfx))
            return LT_PAUSE;
        break;
    }

    return LT_FINISH;
}

static u32 ExitMatchCall(s32 state)
{
    switch (state)
    {
    case 0:
        PlaySE(SE_SELECT);
        SetPokeballIconsFlashing(FALSE);
        PokenavFadeScreen(POKENAV_FADE_TO_BLACK);
        SlideMenuHeaderDown();
        return LT_INC_AND_PAUSE;
    case 1:
        if (IsPaletteFadeActive() || MainMenuLoopedTaskIsBusy())
            return LT_PAUSE;

        break;
    }

    return LT_FINISH;
}

static void CreateMatchCallList(void)
{
    struct PokenavListTemplate template;
    template.list = (struct PokenavListItem *)GetMatchCallList();
    template.count = GetNumberRegistered();
    template.itemSize = sizeof(struct PokenavListItem);
    template.startIndex = 0;
    template.item_X = 12;
    template.windowWidth = 16;
    template.listTop = 1;
    template.maxShowed = 6;
    template.fillValue = 3;
    template.fontId = FONT_NARROW;
    template.bufferItemFunc = (PokenavListBufferItemFunc)BufferMatchCallNameAndDesc;
    template.iconDrawFunc = TryDrawRematchPokeballIcon;
    CreatePokenavListWithHgssPhoneRows(&sMatchCallBgTemplates[2], &template, HGSS_PHONE_BADGE_TILE_COUNT, HGSS_MATCH_CALL_ROWS_PALETTE, DrawHgssPhoneContactRow);
    CreateTask(Task_FlashPokeballIcons, 7);
}

static void DestroyMatchCallList(void)
{
    DestroyPokenavList();
    DestroyTask(FindTaskIdByFunc(Task_FlashPokeballIcons));
}

#define tSinIdx data[0]
#define tActive data[15]

static void SetPokeballIconsFlashing(bool32 active)
{
    u8 taskId = FindTaskIdByFunc(Task_FlashPokeballIcons);
    if (taskId != TASK_NONE)
        gTasks[taskId].tActive = active;
}

static void Task_FlashPokeballIcons(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    if (tActive)
    {
        u16 *tilemap;
        u32 row;
        u16 frame;

        tSinIdx += 4;
        tSinIdx &= 0x7F;
        if (tSinIdx != 0 && tSinIdx != 64)
            return;

        // Switch untouched retail frames; never interpolate their palette.
        frame = (tSinIdx & 64) ? HGSS_PHONE_BADGE_DISABLED : HGSS_PHONE_BADGE_ACTIVE;
        tilemap = GetBgTilemapBuffer(3);
        if (tilemap == NULL)
            return;
        for (row = 0; row < HGSS_MATCH_CALL_BUFFER_ROWS; row++)
        {
            u16 *badge = tilemap + row * 96 + HGSS_PHONE_BADGE_COLUMN;
            if (badge[0] == HGSS_PHONE_BADGE_ACTIVE || badge[0] == HGSS_PHONE_BADGE_DISABLED)
            {
                badge[0] = frame;
                badge[1] = frame + 1;
                badge[32] = frame + 2;
                badge[33] = frame + 3;
            }
        }
        CopyBgTilemapBufferToVram(3);
    }
}

#undef tSinIdx
#undef tActive

static void DrawHgssPhoneContactRow(u16 windowId, u32 listItemId, u32 row)
{
    static const u8 sFill1[] = {11, 14};
    static const u8 sBg1[] = {9, 12};
    static const u8 sBg2[] = {10, 13};
    static const u8 sFill2[] = {10, 13};
    u32 colorIdx = (listItemId + 1) & 1;
    u32 width = GetWindowAttribute(windowId, WINDOW_WIDTH) * 8;
    u32 y = row * HGSS_MATCH_CALL_ROW_HEIGHT;
    u32 rectWidth;

    // Exact retail PhoneContactListUI_DrawNameSlotBG geometry. The GBA
    // contact pane is 128 px wide, so this is a left crop of the retail
    // 216-pixel row: coordinates and palette indices are unchanged.
    FillWindowPixelRect(windowId, PIXEL_FILL(sFill1[colorIdx]), 0, y, width, 24);

    rectWidth = width - 8;
    if (rectWidth > 82)
        rectWidth = 82;
    FillWindowPixelRect(windowId, PIXEL_FILL(sBg1[colorIdx]), 8, y, rectWidth, 20);

    if (width > 90)
    {
        rectWidth = width - 90;
        if (rectWidth > 126)
            rectWidth = 126;
        FillWindowPixelRect(windowId, PIXEL_FILL(sBg2[colorIdx]), 90, y, rectWidth, 20);
    }

    FillWindowPixelRect(windowId, PIXEL_FILL(sFill2[colorIdx]), 1, y + 1, 2, 2);
    FillWindowPixelRect(windowId, PIXEL_FILL(sFill1[colorIdx]), 8, y, 2, 7);
    FillWindowPixelRect(windowId, PIXEL_FILL(sFill1[colorIdx]), 9, y + 9, 2, 2);
    FillWindowPixelRect(windowId, PIXEL_FILL(sFill1[colorIdx]), 9, y + 13, 2, 2);
    FillWindowPixelRect(windowId, PIXEL_FILL(sFill1[colorIdx]), 9, y + 17, 2, 2);
}

static void TryDrawRematchPokeballIcon(u16 windowId, u32 rematchId, u32 tileOffset)
{
    u8 bg = GetWindowAttribute(windowId, WINDOW_BG);
    u16 *tilemap = GetBgTilemapBuffer(bg);

    tilemap += tileOffset * 96 + HGSS_PHONE_BADGE_COLUMN;
    if (ShouldDrawRematchPokeballIcon(rematchId))
    {
        u8 taskId = FindTaskIdByFunc(Task_FlashPokeballIcons);
        u16 frame = HGSS_PHONE_BADGE_ACTIVE;
        if (taskId != TASK_NONE && (gTasks[taskId].data[0] & 64))
            frame = HGSS_PHONE_BADGE_DISABLED;
        tilemap[0] = frame;
        tilemap[1] = frame + 1;
        tilemap[0x20] = frame + 2;
        tilemap[0x21] = frame + 3;
    }
    else
    {
        tilemap[0] = HGSS_PHONE_BADGE_EMPTY;
        tilemap[1] = HGSS_PHONE_BADGE_EMPTY;
        tilemap[0x20] = HGSS_PHONE_BADGE_EMPTY;
        tilemap[0x21] = HGSS_PHONE_BADGE_EMPTY;
    }
}

void ClearRematchPokeballIcon(u16 windowId, u32 tileOffset, u32 rowHeight)
{
    u8 bg = GetWindowAttribute(windowId, WINDOW_BG);
    u16 *tilemap = GetBgTilemapBuffer(bg);
    tilemap += tileOffset * (rowHeight / 8) * 32 + HGSS_PHONE_BADGE_COLUMN;
    tilemap[0] = HGSS_PHONE_BADGE_EMPTY;
    tilemap[1] = HGSS_PHONE_BADGE_EMPTY;
    tilemap[0x20] = HGSS_PHONE_BADGE_EMPTY;
    tilemap[0x21] = HGSS_PHONE_BADGE_EMPTY;
}

static void DrawMatchCallLeftColumnWindows(struct Pokenav_MatchCallGfx *gfx)
{
    gfx->locWindowId = AddWindow(&sMatchCallLocationWindowTemplate);
    gfx->infoBoxWindowId = AddWindow(&sMatchCallInfoBoxWindowTemplate);
    FillWindowPixelBuffer(gfx->locWindowId, PIXEL_FILL(0));
    PutWindowTilemap(gfx->locWindowId);
    FillWindowPixelBuffer(gfx->infoBoxWindowId, PIXEL_FILL(0));
    PutWindowTilemap(gfx->infoBoxWindowId);
    CopyWindowToVram(gfx->locWindowId, COPYWIN_FULL);
    CopyWindowToVram(gfx->infoBoxWindowId, COPYWIN_FULL);
}

static void UpdateMatchCallInfoBox(struct Pokenav_MatchCallGfx *gfx)
{
    // CHECK temporarily maps this window to the dedicated authentic Trainer
    // Card palette. Restore the Phone contact palette before redrawing stats.
    SetWindowAttribute(gfx->infoBoxWindowId, WINDOW_PALETTE_NUM, HGSS_MATCH_CALL_CONTACT_PALETTE);
    PutWindowTilemap(gfx->infoBoxWindowId);
    FillWindowPixelBuffer(gfx->infoBoxWindowId, PIXEL_FILL(0));
    PrintNumberRegisteredLabel(gfx->infoBoxWindowId);
    PrintNumberRegistered(gfx->infoBoxWindowId);
    PrintNumberOfBattlesLabel(gfx->infoBoxWindowId);
    PrintNumberOfBattles(gfx->infoBoxWindowId);
    CopyWindowToVram(gfx->infoBoxWindowId, COPYWIN_FULL);
}

static void PrintNumberRegisteredLabel(u16 windowId)
{
    PrintMatchCallInfoLabel(windowId, gText_NumberRegistered, 0);
}

static void PrintNumberRegistered(u16 windowId)
{
    u8 str[3];
    ConvertIntToDecimalStringN(str, GetNumberRegistered(), STR_CONV_MODE_LEFT_ALIGN, 3);
    PrintMatchCallInfoNumber(windowId, str, 1);
}

static void PrintNumberOfBattlesLabel(u16 windowId)
{
    PrintMatchCallInfoLabel(windowId, gText_NumberOfBattles, 2);
}

static void PrintNumberOfBattles(u16 windowId)
{
    u8 str[5];
    int numTrainerBattles = GetGameStat(GAME_STAT_TRAINER_BATTLES);
    if (numTrainerBattles > 99999)
        numTrainerBattles = 99999;

    ConvertIntToDecimalStringN(str, numTrainerBattles, STR_CONV_MODE_LEFT_ALIGN, 5);
    PrintMatchCallInfoNumber(windowId, str, 3);
}

static void PrintMatchCallInfoLabel(u16 windowId, const u8 *str, int top)
{
    int y = top * 16 + 1;
    AddTextPrinterParameterized(windowId, FONT_NARROW, str, 2, y, TEXT_SKIP_DRAW, NULL);
}

static void PrintMatchCallInfoNumber(u16 windowId, const u8 *str, int top)
{
    int x = GetStringRightAlignXOffset(FONT_NARROW, str, 86);
    int y = top * 16 + 1;
    AddTextPrinterParameterized(windowId, FONT_NARROW, str, x, y, TEXT_SKIP_DRAW, NULL);
}

static void PrintMatchCallLocation(struct Pokenav_MatchCallGfx *gfx, int delta)
{
    u8 mapName[32];
    int x;
    int index = PokenavList_GetSelectedIndex() + delta;
    mapsec_s32_t mapSec = GetMatchCallMapSec(index);
    if (mapSec != MAPSEC_NONE)
        GetMapName(mapName, mapSec, 0);
    else
        StringCopy(mapName, gText_Unknown);

    x = GetStringCenterAlignXOffset(FONT_NARROW, mapName, 88);
    FillWindowPixelBuffer(gfx->locWindowId, PIXEL_FILL(0));
    AddTextPrinterParameterized(gfx->locWindowId, FONT_NARROW, mapName, x, 1, 0, NULL);
    CopyWindowToVram(gfx->locWindowId, COPYWIN_GFX);
}

// Copy the exact retail 8x8 tiles; interior rows use the retail fill indices.
static void BlitHgssActionTile(u16 windowId, u32 tile, u32 x, u32 y)
{
    BlitBitmapToWindow(windowId, (const u8 *)sHgssActionMenu_Gfx + tile * 32,
                       x * 8, y * 8, 8, 8);
}

static void DrawHgssPhoneActionMenu(struct Pokenav_MatchCallGfx *gfx)
{
    u32 count = 0;
    u32 row, col, tile;
    u32 selected = GetMatchCallOptionCursorPos();
    u16 windowId = gfx->actionWindowId;
    // Emerald color order is background, foreground, shadow.
    static const u8 colors[2][3] = {{3, 1, 2}, {6, 4, 5}};

    if (windowId == WINDOW_NONE)
        return;
    while (count < MATCH_CALL_OPTION_COUNT && GetMatchCallOptionId(count) != MATCH_CALL_OPTION_COUNT)
        count++;
    FillWindowPixelBuffer(windowId, PIXEL_FILL(0));
    for (row = 0; row <= count * 3; row++)
    {
        if (row == 0)
            tile = selected == 0 ? 12 : 0;
        else if (row == count * 3)
            tile = selected == count - 1 ? 21 : 9;
        else if (row % 3 == 0)
            tile = selected == row / 3 - 1 ? 18 : (selected == row / 3 ? 24 : 6);
        else
            tile = selected == row / 3 ? 15 : 3;
        for (col = 0; col < 18; col++)
        {
            if (row % 3 != 0 && col > 0 && col < 17)
                continue;
            BlitHgssActionTile(windowId, tile + (col == 0 ? 0 : col == 17 ? 2 : 1), col, row);
        }
    }
    for (row = 0; row < count; row++)
    {
        const u8 *label = sMatchCallOptionTexts[GetMatchCallOptionId(row)];
        u32 active = selected == row;
        u32 x = 8 + (128 - GetStringWidth(FONT_NARROW, label, 0)) / 2;
        FillWindowPixelRect(windowId, PIXEL_FILL(colors[active][0]), 8, 8 + row * 24, 128, 16);
        AddTextPrinterParameterized3(windowId, FONT_NARROW, x, 8 + row * 24,
                                     colors[active], TEXT_SKIP_DRAW, label);
    }
    PutWindowTilemap(windowId);
    CopyWindowToVram(windowId, COPYWIN_FULL);
}

static void PrintMatchCallSelectionOptions(struct Pokenav_MatchCallGfx *gfx)
{
    if (gfx->actionWindowId == WINDOW_NONE)
        gfx->actionWindowId = AddWindow(&sHgssActionMenuWindowTemplate);
    LoadPalette(sHgssActionMenu_Pal, BG_PLTT_ID(8), sizeof(sHgssActionMenu_Pal));
    DrawHgssPhoneActionMenu(gfx);
}

static bool32 WaitForActionMenu(struct Pokenav_MatchCallGfx *gfx)
{
    // Selection is now shown by the authentic sbox_gra selected border.
    return IsDma3ManagerBusyWithBgCopy();
}

static void UpdateWindowsReturnToTrainerList(struct Pokenav_MatchCallGfx *gfx)
{
    CloseMatchCallSelectOptionsWindow(gfx);
    UpdateMatchCallInfoBox(gfx);
}

static bool32 IsDma3ManagerBusyWithBgCopy1(struct Pokenav_MatchCallGfx *gfx)
{
    return IsDma3ManagerBusyWithBgCopy();
}

static void DrawHgssTrainerCardPortraitPanel(u16 windowId)
{
    // Functional reuse of the authentic HGSS Trainer Card front, not a
    // retail PokéGear Phone CHECK page. The generator crops source tiles
    // x=20..30, y=6..13 at 1:1 pixels and remaps only palette indices.
    SetWindowAttribute(windowId, WINDOW_PALETTE_NUM, HGSS_MATCH_CALL_CHECK_PALETTE);
    LoadPalette(sHgssCheckPortrait_Pal, BG_PLTT_ID(HGSS_MATCH_CALL_CHECK_PALETTE), sizeof(sHgssCheckPortrait_Pal));
    FillWindowPixelBuffer(windowId, PIXEL_FILL(0));
    BlitBitmapToWindow(windowId, (const u8 *)sHgssCheckPortrait_Gfx, 0, 0, 88, 64);
    PutWindowTilemap(windowId);
}

static void UpdateWindowsToShowCheckPage(struct Pokenav_MatchCallGfx *gfx)
{
    CloseMatchCallSelectOptionsWindow(gfx);
    DrawHgssTrainerCardPortraitPanel(gfx->infoBoxWindowId);
    CopyWindowToVram(gfx->infoBoxWindowId, COPYWIN_FULL);
}

static void LoadCallWindowAndFade(struct Pokenav_MatchCallGfx *gfx)
{
    gfx->msgBoxWindowId = AddWindow(&sHgssCallMsgBoxWindowTemplate);
    LoadMatchCallWindowGfx(gfx->msgBoxWindowId, 1, 4);
    FadeToBlackExceptPrimary();
}

static void DrawHgssPhoneCallSurface(struct Pokenav_MatchCallGfx *gfx)
{
    LoadBgTiles(1, sHgssMatchCallCall_Gfx, sizeof(sHgssMatchCallCall_Gfx), HGSS_MATCH_CALL_CALL_TILE_OFFSET);
    CopyToBgTilemapBuffer(1, sHgssMatchCallCall_Tilemap, sizeof(sHgssMatchCallCall_Tilemap), 0);
    LoadPalette(sHgssMatchCallCall_Pal, BG_PLTT_ID(HGSS_MATCH_CALL_CALL_PALETTE), sizeof(sHgssMatchCallCall_Pal));

    // Retail HGSS fills the 27x4 Phone call text window with color index 0,
    // then prints the conversation over the member-22 background.
    FillWindowPixelBuffer(gfx->msgBoxWindowId, PIXEL_FILL(0));
    PutWindowTilemap(gfx->msgBoxWindowId);
    CopyWindowToVram(gfx->msgBoxWindowId, COPYWIN_FULL);
    CopyBgTilemapBufferToVram(1);
}

static void DrawMsgBoxForMatchCallMsg(struct Pokenav_MatchCallGfx *gfx)
{
    CloseMatchCallSelectOptionsWindow(gfx);
    DrawHgssPhoneCallSurface(gfx);
}

static void DrawMsgBoxForCloseByMsg(struct Pokenav_MatchCallGfx *gfx)
{
    CloseMatchCallSelectOptionsWindow(gfx);
    DrawHgssPhoneCallSurface(gfx);
}

static bool32 IsDma3ManagerBusyWithBgCopy2(struct Pokenav_MatchCallGfx *gfx)
{
    return IsDma3ManagerBusyWithBgCopy();
}

static void PrintCallingDots(struct Pokenav_MatchCallGfx *gfx)
{
    AddTextPrinterParameterized(gfx->msgBoxWindowId, FONT_NORMAL, sText_CallingDots, 32, 1, 1, NULL);
}

static bool32 WaitForCallingDotsText(struct Pokenav_MatchCallGfx *gfx)
{
    RunTextPrinters();
    return IsTextPrinterActiveOnWindow(gfx->msgBoxWindowId);
}

static void PrintTrainerIsCloseBy(struct Pokenav_MatchCallGfx *gfx)
{
    AddTextPrinterParameterized(gfx->msgBoxWindowId, FONT_NORMAL, gText_TrainerCloseBy, 0, 1, 1, NULL);
}

static bool32 WaitForTrainerIsCloseByText(struct Pokenav_MatchCallGfx *gfx)
{
    RunTextPrinters();
    return IsTextPrinterActiveOnWindow(gfx->msgBoxWindowId);
}

static void PrintMatchCallMessage(struct Pokenav_MatchCallGfx *gfx)
{
    int index = PokenavList_GetSelectedIndex();
    const u8 *str = GetMatchCallMessageText(index, &gfx->newRematchRequest);
    u8 speed = GetPlayerTextSpeedDelay();
    AddTextPrinterParameterized(gfx->msgBoxWindowId, FONT_NORMAL, str, 32, 1, speed, NULL);
}

static bool32 WaitForMatchCallMessageText(struct Pokenav_MatchCallGfx *gfx)
{
    if (JOY_HELD(A_BUTTON))
        gTextFlags.canABSpeedUpPrint = TRUE;
    else
        gTextFlags.canABSpeedUpPrint = FALSE;

    RunTextPrinters();
    return IsTextPrinterActiveOnWindow(gfx->msgBoxWindowId);
}

static void EraseCallMessageBox(struct Pokenav_MatchCallGfx *gfx)
{
    FillBgTilemapBufferRect_Palette0(1, 0, 0, 0, 32, 20);
    CopyBgTilemapBufferToVram(1);
}

static bool32 WaitForCallMessageBoxErase(struct Pokenav_MatchCallGfx *gfx)
{
    return IsDma3ManagerBusyWithBgCopy();
}

static void AllocMatchCallSprites(void)
{
    u8 paletteNum;
    struct SpriteSheet spriteSheet;
    struct Pokenav_MatchCallGfx *gfx = GetSubstructPtr(POKENAV_SUBSTRUCT_MATCH_CALL_OPEN);

    // Load trainer pic gfx
    spriteSheet.data = gfx->trainerPicGfx;
    spriteSheet.size = sizeof(gfx->trainerPicGfx);
    spriteSheet.tag = GFXTAG_TRAINER_PIC;
    gfx->trainerPicGfxPtr = (u8 *)OBJ_VRAM0 + LoadSpriteSheet(&spriteSheet) * 0x20;
    paletteNum = AllocSpritePalette(PALTAG_TRAINER_PIC);
    gfx->trainerPicPalOffset = OBJ_PLTT_ID(paletteNum);
    gfx->trainerPicSprite = CreateTrainerPicSprite();
    gfx->trainerPicSprite->invisible = TRUE;
}

static void FreeMatchCallSprites(void)
{
    struct Pokenav_MatchCallGfx *gfx = GetSubstructPtr(POKENAV_SUBSTRUCT_MATCH_CALL_OPEN);
    if (gfx->trainerPicSprite)
        DestroySprite(gfx->trainerPicSprite);

    FreeSpriteTilesByTag(GFXTAG_TRAINER_PIC);
    FreeSpritePaletteByTag(PALTAG_TRAINER_PIC);
}

static void CloseMatchCallSelectOptionsWindow(struct Pokenav_MatchCallGfx *gfx)
{
    if (gfx->actionWindowId != WINDOW_NONE)
    {
        ClearWindowTilemap(gfx->actionWindowId);
        CopyWindowToVram(gfx->actionWindowId, COPYWIN_MAP);
        RemoveWindow(gfx->actionWindowId);
        gfx->actionWindowId = WINDOW_NONE;
    }
}

static struct Sprite *CreateTrainerPicSprite(void)
{
    u8 spriteId = CreateSprite(&sTrainerPicSpriteTemplate, 44, 104, 6);
    return &gSprites[spriteId];
}

static void LoadCheckPageTrainerPic(struct Pokenav_MatchCallGfx *gfx)
{
    u16 cursor;
    enum TrainerPicID trainerPic = GetMatchCallTrainerPic(PokenavList_GetSelectedIndex());
    if (trainerPic >= 0)
    {
        DecompressDataWithHeaderWram(GetTrainerFrontPicData(trainerPic), gfx->trainerPicGfx);
        memcpy(gfx->trainerPicPal, GetTrainerFrontPicPalette(trainerPic), 32);
        cursor = RequestDma3Copy(gfx->trainerPicGfx, gfx->trainerPicGfxPtr, sizeof(gfx->trainerPicGfx), 1);
        LoadPalette(gfx->trainerPicPal, gfx->trainerPicPalOffset, sizeof(gfx->trainerPicPal));
        gfx->trainerPicSprite->data[0] = 0;
        gfx->trainerPicSprite->data[7] = cursor;
        gfx->trainerPicSprite->callback = SpriteCB_TrainerPicSlideOnscreen;
    }
}

static void TrainerPicSlideOffscreen(struct Pokenav_MatchCallGfx *gfx)
{
    gfx->trainerPicSprite->callback = SpriteCB_TrainerPicSlideOffscreen;
}

static bool32 WaitForTrainerPic(struct Pokenav_MatchCallGfx *gfx)
{
    return gfx->trainerPicSprite->callback != SpriteCallbackDummy;
}

static void SpriteCB_TrainerPicSlideOnscreen(struct Sprite *sprite)
{
    switch (sprite->data[0])
    {
    case 0:
        if (CheckForSpaceForDma3Request(sprite->data[7]) != -1)
        {
            sprite->x2 = -80;
            sprite->invisible = FALSE;
            sprite->data[0]++;
        }
        break;
    case 1:
        sprite->x2 += 8;
        if (sprite->x2 >= 0)
        {
            sprite->x2 = 0;
            sprite->callback = SpriteCallbackDummy;
        }
        break;
    }
}

static void SpriteCB_TrainerPicSlideOffscreen(struct Sprite *sprite)
{
    sprite->x2 -= 8;
    if (sprite->x2 <= -80)
    {
        sprite->invisible = TRUE;
        sprite->callback = SpriteCallbackDummy;
    }
}
