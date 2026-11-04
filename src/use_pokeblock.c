#include "global.h"
#include "main.h"
#include "dma3.h"
#include "pokeblock.h"
#include "malloc.h"
#include "decompress.h"
#include "graphics.h"
#include "palette.h"
#include "pokenav.h"
#include "menu_specialized.h"
#include "scanline_effect.h"
#include "text.h"
#include "bg.h"
#include "window.h"
#include "text_window.h"
#include "constants/rgb.h"
#include "sound.h"
#include "constants/songs.h"
#include "sprite.h"
#include "string_util.h"
#include "strings.h"
#include "menu.h"
#include "gpu_regs.h"
#include "graphics.h"
#include "pokemon_summary_screen.h"
#include "item_menu.h"

/*
    This file handles the screen where the player chooses
    which pokemon to give a pokeblock to. The subsequent scene
    of feeding the pokeblock to the pokemon is handled by
    pokeblock_feed.c, and the rest of the pokeblock menu (and
    other pokeblock-related functions) are in pokeblock.c
*/

enum {
    WIN_TITLE,
    WIN_INDEX,
    WIN_NAME,
    WIN_NATURE,
    WIN_TEXT,
    WIN_CONFIRM,
    WIN_COUNT
};

#define TAG_UP_DOWN     0

// At any one time, the currently selected mon and its two adjacent neighbors can be loaded
// IDs to refer to one of these 3 are called "load id" in this file
#define NUM_SELECTIONS_LOADED 3

struct UsePokeblockSession
{
    void (*callback)(void);
    MainCallback exitCallback;
    struct Pokeblock *pokeblock;
    struct Pokemon *mon;
    u8 stringBuffer[64];
    u8 mainState;
    u8 unused1;
    u8 timer;
    u8 condition;
    u8 numEnhancements;
    u8 unused2;
    bool8 monInTopHalf;
    u8 conditionsBeforeBlock[CONDITION_COUNT];
    u8 conditionsAfterBlock[CONDITION_COUNT];
    u8 enhancements[CONDITION_COUNT];
    s16 pokeblockStatBoosts[CONDITION_COUNT];
    u8 numSelections; // num in party + 1 (for Cancel)
    u8 curSelection;
    bool8 (*loadNewSelection)(void);
    u8 helperState;
    u8 unused3;
    u8 natureText[34];
};

// This struct is identical to PokenavMonListItem, the struct used for managing lists of Pokémon in the PokéNav
// Given that this screen is essentially duplicated in the poknav, this struct was probably the same one with
// a more general name/purpose
// TODO: Once the PokéNav conditions screens are documented, resolve the above
struct UsePokeblockMenuPokemon
{
    u8 boxId; // Because this screen is never used for the PC this is always set to TOTAL_BOXES_COUNT to refer to party
    u8 monId;
    u16 data; // never read
};

struct UsePokeblockMenu
{
    u32 unused;
    u16 partyPalettes[PARTY_SIZE][0x40];
    u8 partySheets[NUM_SELECTIONS_LOADED][MON_PIC_SIZE * MAX_MON_PIC_FRAMES];
    u8 ALIGNED(4) tilemaps[2][BG_SCREEN_SIZE];
    u8 confirmationCursor;
    u8 confirmationSpriteId;
    u8 enhancementSpriteIds[CONDITION_COUNT];
    s16 curMonXOffset;
    u8 curMonSpriteId;
    u16 curMonPalette;
    u16 curMonSheet;
    u8 *curMonTileStart;
    struct Sprite *sparkles[MAX_CONDITION_SPARKLES];
    u8 toLoadSelection;
    u8 locationStrings[NUM_SELECTIONS_LOADED][24]; // Gets an "in party" or "in box #" string that never gets printed
    u8 monNameStrings[NUM_SELECTIONS_LOADED][64];
    struct ConditionGraph graph;
    u8 numSparkles[NUM_SELECTIONS_LOADED];
    s8 curLoadId;
    s8 nextLoadId;
    s8 prevLoadId;
    s8 toLoadId;
    struct UsePokeblockMenuPokemon party[PARTY_SIZE];
    struct UsePokeblockSession info;
};

static void SetUsePokeblockCallback(void (*func)(void));
static void LoadUsePokeblockMenu(void);
static void CB2_UsePokeblockMenu(void);
static void CB2_ReturnToUsePokeblockMenu(void);
static void ShowUsePokeblockMenu(void);
static void CB2_ShowUsePokeblockMenuForResults(void);
static void ShowUsePokeblockMenuForResults(void);
static void LoadPartyInfo(void);
static void LoadHgssUseSprites(void);
static void CreateHgssUseSparkles(void);
static void PrintUseTitle(const u8 *text);
static void PrintBrowseHelp(void);
static void CloseConfirmation(void);
static u8 GetSelectionIdFromPartyId(u8);
static bool8 LoadUsePokeblockMenuGfx(void);
static void UpdateMonPic(u8);
static void UpdateMonInfoText(u16);
static void UsePokeblockMenu(void);
static void UpdateSelection(bool8);
static void CloseUsePokeblockMenu(void);
static void AskUsePokeblock(void);
static s8 HandleAskUsePokeblockInput(void);
static bool8 IsSheenMaxed(void);
static void PrintWontEatAnymore(void);
static void FeedPokeblockToMon(void);
static void EraseMenuWindow(void);
static u8 GetPartyIdFromSelectionId(u8);
static void ShowPokeblockResults(void);
static void CalculateConditionEnhancements(void);
static void LoadAndCreateUpDownSprites(void);
static void CalculateNumAdditionalSparkles(u8);
static void PrintFirstEnhancement(void);
static bool8 TryPrintNextEnhancement(void);
static void BufferEnhancedText(u8 *, u8, s16);
static void PrintMenuWindowText(const u8 *);
static void CalculatePokeblockEffectiveness(struct Pokeblock *, struct Pokemon *);
static void SpriteCB_UpDown(struct Sprite *);
static void LoadInitialMonInfo(void);
static void LoadMonInfo(s16, u8);
static bool8 LoadNewSelection_CancelToMon(void);
static bool8 LoadNewSelection_MonToCancel(void);
static bool8 LoadNewSelection_MonToMon(void);
static void SpriteCB_MonPic(struct Sprite *);

static const u8 sText_GetsAPokeBlockQuestion[] = _(" gets a Pokéblock?");
static const u8 sText_WasEnhanced[] = _("was enhanced!");
static const u8 sText_NothingChanged[] = _("Nothing changed!");
static const u8 sText_WontEatAnymore[] = _("It won't eat anymore…");
static const u8 sText_NatureSlash[] = _("Nature/");

static const u32 sHgssGraphTiles[] = INCBIN_U32("graphics/gen4_ui/hgss_condition/graph_bg.4bpp");
static const u16 sHgssGraphMap[] = INCBIN_U16("graphics/gen4_ui/hgss_condition/graph_bg.bin");
static const u16 sHgssTextPal[] = INCBIN_U16("graphics/gen4_ui/hgss_ribbons/summary_bg.bank_1.gbapal");
static const u16 sHgssHeaderPal[] = INCBIN_U16("graphics/gen4_ui/hgss_ribbons/summary_bg.bank_7.gbapal");
static const u32 sHgssFooter[] = INCBIN_U32("graphics/gen4_ui/hgss_pokeblock_use/footer.4bpp");
static const u32 sHgssConfirmation[] = INCBIN_U32("graphics/gen4_ui/hgss_pokeblock_use/confirmation.4bpp");
static const u32 sHgssArrows[] = INCBIN_U32("graphics/gen4_ui/hgss_pokegear/list_arrows.4bpp");
static const u16 sHgssArrowPal[] = INCBIN_U16("graphics/gen4_ui/hgss_pokegear/list_arrows.gbapal");
static const u32 sGraphDataTile[] = {0x99999999, 0x99999999, 0x99999999, 0x99999999, 0x99999999, 0x99999999, 0x99999999, 0x99999999};
static const u8 sTextColors[] = {0, 5, 2};
static const u8 sText_FeedTitle[] = _("FEED POKéBLOCK");
static const u8 sText_ResultTitle[] = _("CONDITION RESULT");
static const u8 sText_Browse[] = _("UP/DOWN: PICK  A: FEED  B: BACK");
static const u8 sText_CancelHelp[] = _("A/B: BACK");
static const u8 sText_ViewChanges[] = _("A/B: VIEW CHANGES");

// The below 3 are saved for returning to the screen after feeding a pokeblock to a mon
// so that the rest of the data can be freed
static EWRAM_DATA struct UsePokeblockSession *sInfo = NULL;
static EWRAM_DATA MainCallback sExitCallback = NULL;
static EWRAM_DATA struct Pokeblock *sPokeblock = NULL;
EWRAM_DATA u8 gPokeblockMonId = 0;
EWRAM_DATA s16 gPokeblockGain = 0;
static EWRAM_DATA struct UsePokeblockMenu *sMenu = NULL;

// The condition/flavors aren't listed in their normal order in this file, they're listed as shown on the graph going counter-clockwise
// Normally they would go Cool/Spicy, Beauty/Dry, Cute/Sweet, Smart/Bitter, Tough/Sour (also graph order, but clockwise)
static const u32 sConditionToMonData[CONDITION_COUNT] =
{
    [CONDITION_COOL]   = MON_DATA_COOL,
    [CONDITION_TOUGH]  = MON_DATA_TOUGH,
    [CONDITION_SMART]  = MON_DATA_SMART,
    [CONDITION_CUTE]   = MON_DATA_CUTE,
    [CONDITION_BEAUTY] = MON_DATA_BEAUTY
};

static const u8 sConditionToFlavor[CONDITION_COUNT] =
{
    [CONDITION_COOL]   = FLAVOR_SPICY,
    [CONDITION_TOUGH]  = FLAVOR_SOUR,
    [CONDITION_SMART]  = FLAVOR_BITTER,
    [CONDITION_CUTE]   = FLAVOR_SWEET,
    [CONDITION_BEAUTY] = FLAVOR_DRY
};

static const struct BgTemplate sBgTemplates[] =
{
    {.bg = 0, .charBaseIndex = 1, .mapBaseIndex = 5, .screenSize = 0, .paletteMode = 0, .priority = 0, .baseTile = 0},
    {.bg = 1, .charBaseIndex = 2, .mapBaseIndex = 4, .screenSize = 0, .paletteMode = 0, .priority = 3, .baseTile = 0},
    {.bg = 2, .charBaseIndex = 3, .mapBaseIndex = 6, .screenSize = 0, .paletteMode = 0, .priority = 2, .baseTile = 0},
};

static const struct WindowTemplate sWindowTemplates[WIN_COUNT + 1] =
{
    [WIN_TITLE] = {.bg = 0, .tilemapLeft = 1, .tilemapTop = 1, .width = 18, .height = 2, .paletteNum = 1, .baseBlock = 1},
    [WIN_INDEX] = {.bg = 0, .tilemapLeft = 22, .tilemapTop = 1, .width = 7, .height = 2, .paletteNum = 1, .baseBlock = 37},
    [WIN_NAME] = {.bg = 0, .tilemapLeft = 1, .tilemapTop = 3, .width = 18, .height = 2, .paletteNum = 1, .baseBlock = 51},
    [WIN_NATURE] = {.bg = 0, .tilemapLeft = 20, .tilemapTop = 3, .width = 9, .height = 2, .paletteNum = 1, .baseBlock = 87},
    [WIN_TEXT] = {.bg = 0, .tilemapLeft = 0, .tilemapTop = 16, .width = 30, .height = 4, .paletteNum = 1, .baseBlock = 105},
    [WIN_CONFIRM] = {.bg = 0, .tilemapLeft = 21, .tilemapTop = 9, .width = 8, .height = 6, .paletteNum = 1, .baseBlock = 225},
    DUMMY_WIN_TEMPLATE
};

static const u8 *const sConditionNames[CONDITION_COUNT] =
{
    [CONDITION_COOL]   = gText_Coolness,
    [CONDITION_TOUGH]  = gText_Toughness,
    [CONDITION_SMART]  = gText_Smartness,
    [CONDITION_CUTE]   = gText_Cuteness,
    [CONDITION_BEAUTY] = gText_Beauty3
};

static const struct SpriteSheet sSpriteSheet_UpDown = {sHgssArrows, sizeof(sHgssArrows), TAG_UP_DOWN};
static const struct SpritePalette sSpritePalette_UpDown = {sHgssArrowPal, TAG_UP_DOWN};
static const s16 sUpDownCoordsOnGraph[CONDITION_COUNT][2] =
{
    [CONDITION_COOL] = {176, 48},
    [CONDITION_TOUGH] = {101, 57},
    [CONDITION_SMART] = {101, 103},
    [CONDITION_CUTE] = {211, 103},
    [CONDITION_BEAUTY] = {211, 57},
};
static const struct OamData sOam_UpDown = {.shape = SPRITE_SHAPE(16x8), .size = SPRITE_SIZE(16x8), .priority = 1};
static const union AnimCmd sAnim_Up[] = {ANIMCMD_FRAME(4, 0), ANIMCMD_END};
static const union AnimCmd *const sAnims_UpDown[] = {sAnim_Up};
static const struct SpriteTemplate sSpriteTemplate_UpDown =
{
    .tileTag = TAG_UP_DOWN, .paletteTag = TAG_UP_DOWN,
    .oam = &sOam_UpDown, .anims = sAnims_UpDown,
    .affineAnims = gDummySpriteAffineAnimTable, .callback = SpriteCB_UpDown,
};
static const struct OamData sOam_ConfirmCursor = {.shape = SPRITE_SHAPE(8x16), .size = SPRITE_SIZE(8x16), .priority = 0};
static const union AnimCmd sAnim_ConfirmCursor[] = {ANIMCMD_FRAME(0, 0), ANIMCMD_END};
static const union AnimCmd *const sAnims_ConfirmCursor[] = {sAnim_ConfirmCursor};
static const struct SpriteTemplate sSpriteTemplate_ConfirmCursor =
{
    .tileTag = TAG_UP_DOWN, .paletteTag = TAG_UP_DOWN,
    .oam = &sOam_ConfirmCursor, .anims = sAnims_ConfirmCursor,
    .affineAnims = gDummySpriteAffineAnimTable, .callback = SpriteCallbackDummy,
};

// When first opening the selection screen
void ChooseMonToGivePokeblock(struct Pokeblock *pokeblock, void (*callback)(void))
{
    sMenu = AllocZeroed(sizeof(*sMenu));
    sInfo = &sMenu->info;
    sInfo->pokeblock = pokeblock;
    sInfo->exitCallback = callback;
    SetUsePokeblockCallback(LoadUsePokeblockMenu);
    SetMainCallback2(CB2_UsePokeblockMenu);
}

// When returning to the selection screen after feeding a pokeblock to a mon
static void CB2_ReturnAndChooseMonToGivePokeblock(void)
{
    sMenu = AllocZeroed(sizeof(*sMenu));
    sInfo = &sMenu->info;
    sInfo->pokeblock = sPokeblock;
    sInfo->exitCallback = sExitCallback;
    gPokeblockMonId = GetSelectionIdFromPartyId(gPokeblockMonId);
    sInfo->monInTopHalf = (gPokeblockMonId <= PARTY_SIZE / 2) ? FALSE : TRUE;
    SetUsePokeblockCallback(LoadUsePokeblockMenu);
    SetMainCallback2(CB2_ReturnToUsePokeblockMenu);
}

static void CB2_ReturnToUsePokeblockMenu(void)
{
    sInfo->callback();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
    if (sInfo->callback == ShowUsePokeblockMenu)
    {
        sInfo->mainState = 0;
        SetMainCallback2(CB2_ShowUsePokeblockMenuForResults);
    }
}

static void CB2_ShowUsePokeblockMenuForResults(void)
{
    ShowUsePokeblockMenuForResults();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

static void CB2_UsePokeblockMenu(void)
{
    sInfo->callback();
    AnimateSprites();
    BuildOamBuffer();
    RunTextPrinters();
    UpdatePaletteFade();
}

static void VBlankCB_UsePokeblockMenu(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
    ConditionGraph_Draw(&sMenu->graph);
    ScanlineEffect_InitHBlankDmaTransfer();
}

static void SetUsePokeblockCallback(void (*func)(void))
{
    sInfo->callback = func;
    sInfo->mainState = 0;
}

static void LoadUsePokeblockMenu(void)
{
    switch (sInfo->mainState)
    {
    case 0:
        sMenu->curMonSpriteId = SPRITE_NONE;
        sMenu->confirmationSpriteId = SPRITE_NONE;
        for (u32 i = 0; i < CONDITION_COUNT; i++)
            sMenu->enhancementSpriteIds[i] = SPRITE_NONE;
        ConditionGraph_Init(&sMenu->graph);
        sInfo->mainState++;
        break;
    case 1:
        ResetSpriteData();
        FreeAllSpritePalettes();
        sInfo->mainState++;
        break;
    case 2:
        SetVBlankCallback(NULL);
        CpuFill32(0, (void *)(VRAM), VRAM_SIZE);
        sInfo->mainState++;
        break;
    case 3:
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sBgTemplates, ARRAY_COUNT(sBgTemplates));
        InitWindows(sWindowTemplates);
        DeactivateAllTextPrinters();
        sInfo->mainState++;
        break;
    case 4:
        sInfo->mainState++;
        break;
    case 5:
        sInfo->mainState++;
        break;
    case 6:
        gKeyRepeatStartDelay = 20;
        LoadPartyInfo();
        sInfo->mainState++;
        break;
    case 7:
        if (!LoadUsePokeblockMenuGfx())
            sInfo->mainState++;
        break;
    case 8:
        if (sInfo->curSelection != sInfo->numSelections - 1)
            UpdateMonPic(0);
        LoadHgssUseSprites();
        sInfo->mainState++;
        break;
    case 9:
        if (!MoveConditionMonOnscreen(&sMenu->curMonXOffset))
            sInfo->mainState++;
        break;
    case 10:
        sInfo->mainState++;
        break;
    case 11:
        ConditionGraph_CalcPositions(sMenu->graph.conditions[0], sMenu->graph.savedPositions[0]);
        ConditionGraph_InitResetScanline(&sMenu->graph);
        sInfo->mainState++;
        break;
    case 12:
        if (!ConditionGraph_ResetScanline(&sMenu->graph))
        {
            ConditionGraph_SetNewPositions(&sMenu->graph, sMenu->graph.savedPositions[0], sMenu->graph.savedPositions[0]);
            sInfo->mainState++;
        }
        break;
    case 13:
        ConditionGraph_Update(&sMenu->graph);
        sInfo->mainState++;
        break;
    case 14:
        PrintUseTitle(sText_FeedTitle);
        UpdateMonInfoText(0);
        sInfo->mainState++;
        break;
    case 15:
        SetUsePokeblockCallback(ShowUsePokeblockMenu);
        break;
    }
}

static void ShowUsePokeblockMenu(void)
{
    switch (sInfo->mainState)
    {
    case 0:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
        SetVBlankCallback(VBlankCB_UsePokeblockMenu);
        ShowBg(0);
        ShowBg(1);
        if (sInfo->curSelection != sInfo->numSelections - 1)
            ShowBg(2);
        sInfo->mainState++;
        break;
    case 1:
        if (!gPaletteFade.active)
        {
            CreateHgssUseSparkles();

            SetUsePokeblockCallback(UsePokeblockMenu);
        }
        break;
    }
}

enum {
    STATE_HANDLE_INPUT,
    STATE_UPDATE_SELECTION,
    STATE_2, // unused state
    STATE_CLOSE,
    STATE_4, // unused state
    STATE_CONFIRM_SELECTION,
    STATE_HANDLE_CONFIRMATION,
    STATE_WAIT_MSG,
};

static void UsePokeblockMenu(void)
{
    bool8 loading;

    switch (sInfo->mainState)
    {
    case STATE_HANDLE_INPUT:
        if (JOY_HELD(DPAD_UP) && sInfo->numSelections > 1)
        {
            PlaySE(SE_SELECT);
            UpdateSelection(TRUE);
            DestroyConditionSparkleSprites(sMenu->sparkles);
            sInfo->mainState = STATE_UPDATE_SELECTION;
        }
        else if (JOY_HELD(DPAD_DOWN) && sInfo->numSelections > 1)
        {
            PlaySE(SE_SELECT);
            UpdateSelection(FALSE);
            DestroyConditionSparkleSprites(sMenu->sparkles);
            sInfo->mainState = STATE_UPDATE_SELECTION;
        }
        else if (JOY_NEW(B_BUTTON))
        {
            PlaySE(SE_SELECT);
            sInfo->mainState = STATE_CLOSE;
        }
        else if (JOY_NEW(A_BUTTON))
        {
            PlaySE(SE_SELECT);

            // If last item, selected Cancel. Otherwise selected mon
            if (sMenu->info.curSelection == sMenu->info.numSelections - 1)
                sInfo->mainState = STATE_CLOSE;
            else
                sInfo->mainState = STATE_CONFIRM_SELECTION;
        }
        break;
    case STATE_UPDATE_SELECTION:
        loading = sMenu->info.loadNewSelection();
        if (!loading)
            sInfo->mainState = STATE_HANDLE_INPUT;
        break;
    case STATE_2:
        break;
    case STATE_CLOSE:
        SetUsePokeblockCallback(CloseUsePokeblockMenu);
        break;
    case STATE_4:
        break;
    case STATE_CONFIRM_SELECTION:
        AskUsePokeblock();
        sInfo->mainState++;
        break;
    case STATE_HANDLE_CONFIRMATION:
        switch (HandleAskUsePokeblockInput())
        {
        case 1: // NO
        case MENU_B_PRESSED:
            sInfo->mainState = STATE_HANDLE_INPUT;
            break;
        case 0: // YES
            if (IsSheenMaxed())
            {
                PrintWontEatAnymore();
                sInfo->mainState = STATE_WAIT_MSG;
            }
            else
            {
                SetUsePokeblockCallback(FeedPokeblockToMon);
            }
            break;
        }
        break;
    case STATE_WAIT_MSG:
        if (JOY_NEW(A_BUTTON | B_BUTTON))
        {
            EraseMenuWindow();
            sInfo->mainState = STATE_HANDLE_INPUT;
        }
        break;
    }
}

static void FeedPokeblockToMon(void)
{
    switch (sInfo->mainState)
    {
    case 0:
        gPokeblockMonId = GetPartyIdFromSelectionId(sMenu->info.curSelection);
        sExitCallback = sInfo->exitCallback;
        sPokeblock = sInfo->pokeblock;
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        sInfo->mainState++;
        break;
    case 1:
        if (!gPaletteFade.active)
        {
            SetVBlankCallback(NULL);

            FREE_AND_SET_NULL(sMenu);
            FreeAllWindowBuffers();
            gMain.savedCallback = CB2_ReturnAndChooseMonToGivePokeblock;
            PreparePokeblockFeedScene();
        }
        break;
    }
}

static void ShowUsePokeblockMenuForResults(void)
{
    bool8 loading;

    switch (sInfo->mainState)
    {
    case 0:
        if (sMenu->info.curSelection != gPokeblockMonId)
        {
            UpdateSelection(sInfo->monInTopHalf);
            sInfo->mainState++;
        }
        else
        {
            sInfo->mainState = 3;
        }
        break;
    case 1:
        loading = sMenu->info.loadNewSelection();
        if (!loading)
            sInfo->mainState = 0;
        break;
    case 2:
        break;
    case 3:
        BlendPalettes(PALETTES_ALL, 16, RGB_BLACK);
        sInfo->mainState++;
        break;
    case 4:
        ShowBg(0);
        ShowBg(1);
        ShowBg(2);
        sInfo->mainState++;
        break;
    case 5:
        SetVBlankCallback(VBlankCB_UsePokeblockMenu);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
        sInfo->mainState++;
        break;
    case 6:
        if (!gPaletteFade.active)
        {
            ResetConditionSparkleSprites(sMenu->sparkles);
            SetUsePokeblockCallback(ShowPokeblockResults);
            SetMainCallback2(CB2_UsePokeblockMenu);
        }
        break;
    }
}

static void ShowPokeblockResults(void)
{
    switch (sInfo->mainState)
    {
    case 0:
        PrintUseTitle(sText_ResultTitle);
        PrintMenuWindowText(sText_ViewChanges);
        sInfo->mon = gParties[B_TRAINER_PLAYER];
        sInfo->mon += sMenu->party[sMenu->info.curSelection].monId;
        DestroyConditionSparkleSprites(sMenu->sparkles);
        sInfo->mainState++;
        break;
    case 1:
        if (JOY_NEW(A_BUTTON | B_BUTTON))
            sInfo->mainState++;
        break;
    case 2:
        CalculateConditionEnhancements();
        ConditionGraph_CalcPositions(sInfo->conditionsAfterBlock, sMenu->graph.savedPositions[CONDITION_GRAPH_LOAD_MAX - 1]);
        ConditionGraph_SetNewPositions(&sMenu->graph, sMenu->graph.savedPositions[sMenu->curLoadId], sMenu->graph.savedPositions[CONDITION_GRAPH_LOAD_MAX - 1]);
        LoadAndCreateUpDownSprites();
        sInfo->mainState++;
        break;
    case 3:
        if (!ConditionGraph_TryUpdate(&sMenu->graph))
        {
            CalculateNumAdditionalSparkles(GetPartyIdFromSelectionId(sMenu->info.curSelection));
            CreateHgssUseSparkles();

            sInfo->timer = 0;
            sInfo->mainState++;
        }
        break;
    case 4:
        if (++sInfo->timer > 16)
        {
            PrintFirstEnhancement();
            sInfo->mainState++;
        }
        break;
    case 5:
        if (JOY_NEW(A_BUTTON | B_BUTTON) && !TryPrintNextEnhancement())
        {
            TryClearPokeblock((u8)gSpecialVar_ItemId);
            SetUsePokeblockCallback(CloseUsePokeblockMenu);
        }
        break;
    }
}

static void CloseUsePokeblockMenu(void)
{
    u8 i;

    switch (sInfo->mainState)
    {
    case 0:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        sInfo->mainState++;
        break;
    case 1:
        if (!gPaletteFade.active)
            sInfo->mainState = 2;
        break;
    case 2:
        gScanlineEffect.state = 3;
        ScanlineEffect_InitHBlankDmaTransfer();
        sInfo->mainState++;
        break;
    case 3:
        SetMainCallback2(sInfo->exitCallback);
        FreeConditionSparkles(sMenu->sparkles);
        for (i = 0; i < CONDITION_COUNT; i++)
            if (sMenu->enhancementSpriteIds[i] != SPRITE_NONE)
                DestroySprite(&gSprites[sMenu->enhancementSpriteIds[i]]);
        FreeSpriteTilesByTag(TAG_UP_DOWN);
        FreeSpritePaletteByTag(TAG_UP_DOWN);

        if (sMenu->curMonSpriteId != SPRITE_NONE)
            DestroySprite(&gSprites[sMenu->curMonSpriteId]);
        FreeSpriteTilesByTag(TAG_CONDITION_MON);
        FreeSpritePaletteByTag(TAG_CONDITION_MON);

        SetVBlankCallback(NULL);

        FREE_AND_SET_NULL(sMenu);
        FreeAllWindowBuffers();
        break;
    }
}

static void AskUsePokeblock(void)
{
    u8 stringBuffer[64];
    GetMonData(&gParties[B_TRAINER_PLAYER][GetPartyIdFromSelectionId(sInfo->curSelection)], MON_DATA_NICKNAME, stringBuffer);
    StringGet_Nickname(stringBuffer);
    StringAppend(stringBuffer, sText_GetsAPokeBlockQuestion);
    PrintMenuWindowText(stringBuffer);
    BlitBitmapToWindow(WIN_CONFIRM, (const u8 *)sHgssConfirmation, 0, 0, 64, 48);
    AddTextPrinterParameterized3(WIN_CONFIRM, FONT_NORMAL, 24, 8, sTextColors, TEXT_SKIP_DRAW, gText_Yes);
    AddTextPrinterParameterized3(WIN_CONFIRM, FONT_NORMAL, 24, 24, sTextColors, TEXT_SKIP_DRAW, gText_No);
    PutWindowTilemap(WIN_CONFIRM);
    CopyWindowToVram(WIN_CONFIRM, COPYWIN_FULL);
    HideBg(2);
    sMenu->confirmationCursor = 0;
    sMenu->confirmationSpriteId = CreateSprite(&sSpriteTemplate_ConfirmCursor, 180, 88, 0);
}

static s8 HandleAskUsePokeblockInput(void)
{
    if (JOY_NEW(B_BUTTON))
    {
        PlaySE(SE_SELECT);
        CloseConfirmation();
        PrintBrowseHelp();
        return MENU_B_PRESSED;
    }
    if (JOY_NEW(A_BUTTON))
    {
        s8 choice = sMenu->confirmationCursor;
        PlaySE(SE_SELECT);
        CloseConfirmation();
        if (choice != 0)
            PrintBrowseHelp();
        return choice;
    }
    if (JOY_NEW(DPAD_UP | DPAD_DOWN))
    {
        u8 choice = JOY_NEW(DPAD_DOWN) ? 1 : 0;
        if (choice != sMenu->confirmationCursor)
        {
            PlaySE(SE_SELECT);
            sMenu->confirmationCursor = choice;
            if (sMenu->confirmationSpriteId != SPRITE_NONE)
                gSprites[sMenu->confirmationSpriteId].y = 88 + choice * 16;
        }
    }
    return MENU_NOTHING_CHOSEN;
}

static void CloseConfirmation(void)
{
    if (sMenu->confirmationSpriteId != SPRITE_NONE)
    {
        DestroySprite(&gSprites[sMenu->confirmationSpriteId]);
        sMenu->confirmationSpriteId = SPRITE_NONE;
    }
    ClearWindowTilemap(WIN_CONFIRM);
    CopyWindowToVram(WIN_CONFIRM, COPYWIN_MAP);
    ShowBg(2);
}

static void PrintFirstEnhancement(void)
{

    for (sInfo->condition = 0; sInfo->condition < CONDITION_COUNT; sInfo->condition++)
    {
        if (sInfo->enhancements[sInfo->condition] != 0)
            break;
    }

    if (sInfo->condition < CONDITION_COUNT)
        BufferEnhancedText(gStringVar4, sInfo->condition, sInfo->enhancements[sInfo->condition]);
    else
        BufferEnhancedText(gStringVar4, sInfo->condition, 0);

    PrintMenuWindowText(gStringVar4);
    PutWindowTilemap(WIN_TEXT);
    CopyWindowToVram(WIN_TEXT, COPYWIN_FULL);
}

static bool8 TryPrintNextEnhancement(void)
{

    while (1)
    {
        sInfo->condition++;
        if (sInfo->condition < CONDITION_COUNT)
        {
            if (sInfo->enhancements[sInfo->condition] != 0)
                break;
        }
        else
        {
            sInfo->condition = CONDITION_COUNT;
            return FALSE;
        }
    }

    BufferEnhancedText(gStringVar4, sInfo->condition, sInfo->enhancements[sInfo->condition]);
    PrintMenuWindowText(gStringVar4);
    CopyWindowToVram(WIN_TEXT, COPYWIN_GFX);

    return TRUE;
}

static void PrintWontEatAnymore(void)
{
    PrintMenuWindowText(sText_WontEatAnymore);
}

static void EraseMenuWindow(void)
{
    PrintBrowseHelp();
}

static void PrintMenuWindowText(const u8 *message)
{
    BlitBitmapToWindow(WIN_TEXT, (const u8 *)sHgssFooter, 0, 0, 240, 32);
    AddTextPrinterParameterized3(WIN_TEXT, FONT_NORMAL, 8, 9, sTextColors, TEXT_SKIP_DRAW, message);
    PutWindowTilemap(WIN_TEXT);
    CopyWindowToVram(WIN_TEXT, COPYWIN_FULL);
}

static void PrintBrowseHelp(void)
{
    PrintMenuWindowText(sInfo->curSelection == sInfo->numSelections - 1 ? sText_CancelHelp : sText_Browse);
}

static void PrintUseTitle(const u8 *text)
{
    FillWindowPixelBuffer(WIN_TITLE, PIXEL_FILL(0));
    AddTextPrinterParameterized3(WIN_TITLE, FONT_NORMAL, 0, 1, sTextColors, TEXT_SKIP_DRAW, text);
    PutWindowTilemap(WIN_TITLE);
    CopyWindowToVram(WIN_TITLE, COPYWIN_FULL);
}

static void BufferEnhancedText(u8 *dest, u8 condition, s16 enhancement)
{
    switch (enhancement)
    {
    case 1 ... 32767: // if > 0
        enhancement = 0;
        // fallthrough
    case -32768 ... -1: // if < 0
        if (enhancement)
            dest[(u16)enhancement] += 0; // something you can't imagine
        StringCopy(dest, sConditionNames[condition]);
        StringAppend(dest, sText_WasEnhanced);
        break;
    case 0:
        StringCopy(dest, sText_NothingChanged);
        break;
    }
}

static void GetMonConditions(struct Pokemon *mon, u8 *data)
{
    u16 i;

    for (i = 0; i < CONDITION_COUNT; i++)
        data[i] = GetMonData(mon, sConditionToMonData[i]);
}

static void AddPokeblockToConditions(struct Pokeblock *pokeblock, struct Pokemon *mon)
{
    u16 i;
    s16 stat;
    u8 data;

    if (GetMonData(mon, MON_DATA_SHEEN) != MAX_SHEEN)
    {
        CalculatePokeblockEffectiveness(pokeblock, mon);
        for (i = 0; i < CONDITION_COUNT; i++)
        {
            data = GetMonData(mon, sConditionToMonData[i]);
            stat = data +  sInfo->pokeblockStatBoosts[i];
            if (stat < 0)
                stat = 0;
            if (stat > MAX_CONDITION)
                stat = MAX_CONDITION;
            data = stat;
            SetMonData(mon, sConditionToMonData[i], &data);
        }

        stat = (u8)(GetMonData(mon, MON_DATA_SHEEN)) + pokeblock->feel;
        if (stat > MAX_SHEEN)
            stat = MAX_SHEEN;

        data = stat;
        SetMonData(mon, MON_DATA_SHEEN, &data);
    }
}

static void CalculateConditionEnhancements(void)
{
    u16 i;
    struct Pokemon *mon = gParties[B_TRAINER_PLAYER];
    mon += sMenu->party[sMenu->info.curSelection].monId;

    GetMonConditions(mon, sInfo->conditionsBeforeBlock);
    AddPokeblockToConditions(sInfo->pokeblock, mon);
    GetMonConditions(mon, sInfo->conditionsAfterBlock);
    for (i = 0; i < CONDITION_COUNT; i++)
        sInfo->enhancements[i] = sInfo->conditionsAfterBlock[i] - sInfo->conditionsBeforeBlock[i];
}

static void CalculatePokeblockEffectiveness(struct Pokeblock *pokeblock, struct Pokemon *mon)
{
    s8 i, direction, flavor;

    sInfo->pokeblockStatBoosts[CONDITION_COOL] = pokeblock->spicy;
    sInfo->pokeblockStatBoosts[CONDITION_TOUGH] = pokeblock->sour;
    sInfo->pokeblockStatBoosts[CONDITION_SMART] = pokeblock->bitter;
    sInfo->pokeblockStatBoosts[CONDITION_CUTE] = pokeblock->sweet;
    sInfo->pokeblockStatBoosts[CONDITION_BEAUTY] = pokeblock->dry;

    if (gPokeblockGain > 0)
        direction = 1;
    else if (gPokeblockGain < 0)
        direction = -1;
    else
        return;

    for (i = 0; i < CONDITION_COUNT; i++)
    {
        s16 amount = sInfo->pokeblockStatBoosts[i];
        s8 boost = amount / 10;

        if (amount % 10 >= 5) // round to the nearest
            boost++;

        flavor = GetMonFlavorRelation(mon, sConditionToFlavor[i]);
        if (flavor == direction)
            sInfo->pokeblockStatBoosts[i] += boost * flavor;
    }
}

static bool8 IsSheenMaxed(void)
{
    if (GetBoxOrPartyMonData(sMenu->party[sMenu->info.curSelection].boxId,
                             sMenu->party[sMenu->info.curSelection].monId,
                             MON_DATA_SHEEN,
                             NULL) == MAX_SHEEN)
        return TRUE;
    else
        return FALSE;
}

static u8 GetPartyIdFromSelectionId(u8 selectionId)
{
    u8 i;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (!GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_IS_EGG))
        {
            if (selectionId == 0)
                return i;
            selectionId--;
        }
    }

    return 0;
}

// Eggs are not viewable on the condition screen, so count how many are skipped over to reach the party id
static u8 GetSelectionIdFromPartyId(u8 partyId)
{
    u8 i, numEggs;
    for (i = 0, numEggs = 0; i < partyId; i++)
    {
        if (GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_IS_EGG))
            numEggs++;
    }

    return partyId - numEggs;
}

static void LoadAndCreateUpDownSprites(void)
{
    u16 i;

    sInfo->numEnhancements = 0;

    for (i = 0; i < CONDITION_COUNT; i++)
    {
        if (sInfo->enhancements[i] != 0)
        {
            u16 spriteId = CreateSpriteUnchecked(&sSpriteTemplate_UpDown, sUpDownCoordsOnGraph[i][0], sUpDownCoordsOnGraph[i][1], 0);
            if (spriteId != MAX_SPRITES)
            {
                gSprites[spriteId].data[1] = i;
                sMenu->enhancementSpriteIds[i] = spriteId;
                sInfo->numEnhancements++;
            }
        }
    }
}

#define tTimer data[0]

static void SpriteCB_UpDown(struct Sprite *sprite)
{
    if (sprite->tTimer < 3)
        sprite->y2--;
    else if (sprite->tTimer < 6)
        sprite->y2++;

    if (++sprite->tTimer > 60)
    {
        sMenu->enhancementSpriteIds[sprite->data[1]] = SPRITE_NONE;
        DestroySprite(sprite);
        sInfo->numEnhancements--;
    }
}

#undef tTimer

static void LoadPartyInfo(void)
{
    u16 i;
    u16 numMons;

    for (i = 0, numMons = 0; i < CalculatePlayerPartyCount(); i++)
    {
        if (!GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_IS_EGG))
        {
            sMenu->party[numMons].boxId = TOTAL_BOXES_COUNT;
            sMenu->party[numMons].monId = i;
            sMenu->party[numMons].data = 0;
            numMons++;
        }
    }

    sMenu->info.curSelection = 0;
    sMenu->info.numSelections = numMons + 1;
    LoadInitialMonInfo();
}

static void LoadInitialMonInfo(void)
{
    s16 nextSelection, prevSelection;

    LoadMonInfo(sMenu->info.curSelection, 0);
    sMenu->curLoadId = 0;
    sMenu->nextLoadId = 1;
    sMenu->prevLoadId = 2;

    nextSelection = sMenu->info.curSelection + 1;
    if (nextSelection >= sMenu->info.numSelections)
        nextSelection = 0;

    prevSelection = sMenu->info.curSelection - 1;
    if (prevSelection < 0)
        prevSelection = sMenu->info.numSelections - 1;

    LoadMonInfo(nextSelection, 1);
    LoadMonInfo(prevSelection, 2);
}

static void LoadMonInfo(s16 partyId, u8 loadId)
{
    u8 boxId = TOTAL_BOXES_COUNT;
    u8 monId = 0;
    u8 numSelections = sMenu->info.numSelections;
    bool8 excludesCancel = FALSE;

    // Cancel is after the final real party entry, including with six mons.
    if (partyId < numSelections - 1)
    {
        boxId = sMenu->party[partyId].boxId;
        monId = sMenu->party[partyId].monId;
    }

    GetConditionMenuMonNameAndLocString(sMenu->locationStrings[loadId], sMenu->monNameStrings[loadId], boxId, monId, partyId, numSelections, excludesCancel);
    GetConditionMenuMonConditions(&sMenu->graph, sMenu->numSparkles, boxId, monId, partyId, loadId, numSelections, excludesCancel);
    GetConditionMenuMonGfx(sMenu->partySheets[loadId], sMenu->partyPalettes[loadId], boxId, monId, partyId, numSelections, excludesCancel);
}

static void UpdateMonPic(u8 loadId)
{
    u8 spriteId;
    struct SpriteTemplate spriteTemplate;
    struct SpriteSheet spriteSheet;
    struct SpritePalette spritePal;

    if (sMenu->curMonSpriteId == SPRITE_NONE)
    {
        LoadConditionMonPicTemplate(&spriteSheet, &spriteTemplate, &spritePal);
        spriteSheet.data = sMenu->partySheets[loadId];
        spritePal.data = sMenu->partyPalettes[loadId];
        sMenu->curMonPalette = LoadSpritePalette(&spritePal);
        sMenu->curMonSheet = LoadSpriteSheet(&spriteSheet);
        spriteId = CreateSpriteUnchecked(&spriteTemplate, 40, 80, 0);
        sMenu->curMonSpriteId = spriteId;
        if (spriteId == MAX_SPRITES)
        {
            FreeSpriteTilesByTag(TAG_CONDITION_MON);
            FreeSpritePaletteByTag(TAG_CONDITION_MON);
            sMenu->curMonSpriteId = SPRITE_NONE;
        }
        else
        {
            sMenu->curMonSpriteId = spriteId;
            gSprites[sMenu->curMonSpriteId].callback = SpriteCB_MonPic;
            sMenu->curMonTileStart = (void *)(OBJ_VRAM0 + (sMenu->curMonSheet * 32));
            sMenu->curMonPalette = OBJ_PLTT_ID(sMenu->curMonPalette);
        }
    }
    else
    {
        Dma3CopyLarge16_(sMenu->partySheets[loadId], sMenu->curMonTileStart, MON_PIC_SIZE);
        LoadPalette(sMenu->partyPalettes[loadId], sMenu->curMonPalette, PLTT_SIZE_4BPP);
    }
}

static void LoadHgssUseSprites(void)
{
    struct SpriteSheet sheet;
    struct SpritePalette pal;
    LoadHgssConditionSparkle(&sheet, &pal);
    LoadSpriteSheet(&sheet);
    LoadSpritePalette(&pal);
    LoadSpriteSheet(&sSpriteSheet_UpDown);
    LoadSpritePalette(&sSpritePalette_UpDown);
}

static void CreateHgssUseSparkles(void)
{
    ResetConditionSparkleSprites(sMenu->sparkles);
    if (sInfo->curSelection != sInfo->numSelections - 1 && sMenu->curMonSpriteId != SPRITE_NONE)
        CreateHgssConditionSparkleSprites(sMenu->sparkles, sMenu->curMonSpriteId, sMenu->numSparkles[sMenu->curLoadId]);
}

static bool8 LoadUsePokeblockMenuGfx(void)
{
    switch (sInfo->helperState++)
    {
    case 0:
        for (u32 bg = 0; bg < 3; bg++)
        {
            ChangeBgX(bg, 0, BG_COORD_SET);
            ChangeBgY(bg, 0, BG_COORD_SET);
        }
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_1D_MAP | DISPCNT_OBJ_ON | DISPCNT_WIN0_ON | DISPCNT_WIN1_ON);
        SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG2 | BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_BG1);
        SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(11, 4));
        LoadBgTiles(1, sHgssGraphTiles, sizeof(sHgssGraphTiles), 0);
        SetBgTilemapBuffer(1, sMenu->tilemaps[0]);
        CopyToBgTilemapBuffer(1, sHgssGraphMap, sizeof(sHgssGraphMap), 0);
        CopyBgTilemapBufferToVram(1);
        LoadPalette(sHgssTextPal, BG_PLTT_ID(1), sizeof(sHgssTextPal));
        LoadPalette(sHgssHeaderPal, BG_PLTT_ID(7), sizeof(sHgssHeaderPal));
        sMenu->curMonXOffset = -80;
        break;
    case 1:
        LoadBgTiles(2, sGraphDataTile, sizeof(sGraphDataTile), 1);
        SetBgTilemapBuffer(2, sMenu->tilemaps[1]);
        FillBgTilemapBufferRect(2, 1, 0, 0, 32, 32, 1);
        CopyBgTilemapBufferToVram(2);
        ConditionGraph_InitWindow(2);
        break;
    default:
        sInfo->helperState = 0;
        return FALSE;
    }
    return TRUE;
}

static void UpdateMonInfoText(u16 loadId)
{
    u8 text[16], *end;
    bool32 cancel = sInfo->curSelection == sInfo->numSelections - 1;
    FillWindowPixelBuffer(WIN_NAME, PIXEL_FILL(0));
    FillWindowPixelBuffer(WIN_NATURE, PIXEL_FILL(0));
    FillWindowPixelBuffer(WIN_INDEX, PIXEL_FILL(0));
    if (cancel)
    {
        AddTextPrinterParameterized3(WIN_NAME, FONT_NORMAL, 0, 1, sTextColors, TEXT_SKIP_DRAW, gText_Cancel);
        StringCopy(text, gText_Cancel);
        HideBg(2);
    }
    else
    {
        u8 partyIndex = GetPartyIdFromSelectionId(sInfo->curSelection);
        u8 nature = GetNature(&gParties[B_TRAINER_PLAYER][partyIndex]);
        u8 *str = StringCopy(sInfo->natureText, sText_NatureSlash);
        StringCopy(str, gNaturesInfo[nature].name);
        AddTextPrinterParameterized3(WIN_NAME, FONT_NORMAL, 0, 1, sTextColors, TEXT_SKIP_DRAW, sMenu->monNameStrings[loadId]);
        AddTextPrinterParameterized3(WIN_NATURE, FONT_SMALL, 0, 2, sTextColors, TEXT_SKIP_DRAW, sInfo->natureText);
        end = ConvertIntToDecimalStringN(text, sInfo->curSelection + 1, STR_CONV_MODE_LEFT_ALIGN, 1);
        end = StringCopy(end, COMPOUND_STRING(" / "));
        ConvertIntToDecimalStringN(end, sInfo->numSelections - 1, STR_CONV_MODE_LEFT_ALIGN, 1);
        ShowBg(2);
    }
    AddTextPrinterParameterized3(WIN_INDEX, FONT_NORMAL, 0, 1, sTextColors, TEXT_SKIP_DRAW, text);
    for (u32 win = WIN_INDEX; win <= WIN_NATURE; win++)
    {
        PutWindowTilemap(win);
        CopyWindowToVram(win, COPYWIN_FULL);
    }
    PrintBrowseHelp();
}

static void UpdateSelection(bool8 up)
{
    u16 newLoadId;
    bool32 startedOnMon, endedOnMon;

    if (up)
        newLoadId = sMenu->prevLoadId;
    else
        newLoadId = sMenu->nextLoadId;

    ConditionGraph_SetNewPositions(&sMenu->graph, sMenu->graph.savedPositions[sMenu->curLoadId], sMenu->graph.savedPositions[newLoadId]);

    if (sMenu->info.curSelection == sMenu->info.numSelections - 1)
        startedOnMon = FALSE; // moving off of Cancel
    else
        startedOnMon = TRUE;

    if (up)
    {
        sMenu->prevLoadId = sMenu->nextLoadId; // temporarily store nextLoadId, prevLoadId no longer needed
        sMenu->nextLoadId = sMenu->curLoadId;
        sMenu->curLoadId = newLoadId;
        sMenu->toLoadId = sMenu->prevLoadId; // next load will be the mon that's one up from new selection

        // Check for wrap to bottom of list
        sMenu->info.curSelection = (sMenu->info.curSelection == 0)
            ? sMenu->info.numSelections - 1
            : sMenu->info.curSelection - 1;

        sMenu->toLoadSelection = (sMenu->info.curSelection == 0)
            ? sMenu->info.numSelections - 1
            : sMenu->info.curSelection - 1;
    }
    else
    {
        sMenu->nextLoadId = sMenu->prevLoadId; // temporarily store prevLoadId, nextLoadId no longer needed
        sMenu->prevLoadId = sMenu->curLoadId;
        sMenu->curLoadId = newLoadId;
        sMenu->toLoadId = sMenu->nextLoadId; // next load will be the mon that's one down from new selection

        // Check for wrap to top of list
        sMenu->info.curSelection = (sMenu->info.curSelection < sMenu->info.numSelections - 1)
            ? sMenu->info.curSelection + 1
            : 0;

        sMenu->toLoadSelection = (sMenu->info.curSelection < sMenu->info.numSelections - 1)
            ? sMenu->info.curSelection + 1
            : 0;
    }

    if (sMenu->info.curSelection == sMenu->info.numSelections - 1)
        endedOnMon = FALSE; // moving onto Cancel
    else
        endedOnMon = TRUE;

    DestroyConditionSparkleSprites(sMenu->sparkles);

    if (!startedOnMon)
        sMenu->info.loadNewSelection = LoadNewSelection_CancelToMon;
    else if (!endedOnMon)
        sMenu->info.loadNewSelection = LoadNewSelection_MonToCancel;
    else
        sMenu->info.loadNewSelection = LoadNewSelection_MonToMon;
}

static bool8 LoadNewSelection_CancelToMon(void)
{
    switch (sMenu->info.helperState)
    {
    case 0:
        UpdateMonPic(sMenu->curLoadId);
        sMenu->info.helperState++;
        break;
    case 1:
        UpdateMonInfoText(sMenu->curLoadId);
        sMenu->info.helperState++;
        break;
    case 2:
        if (!ConditionMenu_UpdateMonEnter(&sMenu->graph, &sMenu->curMonXOffset))
        {
            // Load the new adjacent Pokémon (not the one being shown)
            LoadMonInfo(sMenu->toLoadSelection, sMenu->toLoadId);
            sMenu->info.helperState++;
        }
        break;
    case 3:
        CreateHgssUseSparkles();

        sMenu->info.helperState = 0;
        return FALSE;
    }

    return TRUE;
}

static bool8 LoadNewSelection_MonToCancel(void)
{
    switch (sMenu->info.helperState)
    {
    case 0:
        if (!ConditionMenu_UpdateMonExit(&sMenu->graph, &sMenu->curMonXOffset))
            sMenu->info.helperState++;
        break;
    case 1:
        UpdateMonInfoText(sMenu->curLoadId);
        sMenu->info.helperState++;
        break;
    case 2:
        LoadMonInfo(sMenu->toLoadSelection, sMenu->toLoadId);
        sMenu->info.helperState++;
        break;
    case 3:
        sMenu->info.helperState = 0;
        return FALSE;
    }

    return TRUE;
}

static bool8 LoadNewSelection_MonToMon(void)
{
    switch (sMenu->info.helperState)
    {
    case 0:
        ConditionGraph_TryUpdate(&sMenu->graph);
        if (!MoveConditionMonOffscreen(&sMenu->curMonXOffset))
        {
            UpdateMonPic(sMenu->curLoadId);
            sMenu->info.helperState++;
        }
        break;
    case 1:
        UpdateMonInfoText(sMenu->curLoadId);
        sMenu->info.helperState++;
        break;
    case 2:
        if (!ConditionMenu_UpdateMonEnter(&sMenu->graph, &sMenu->curMonXOffset))
        {
            // Load the new adjacent Pokémon (not the one being shown)
            LoadMonInfo(sMenu->toLoadSelection, sMenu->toLoadId);
            sMenu->info.helperState++;
        }
        break;
    case 3:
        CreateHgssUseSparkles();

        sMenu->info.helperState = 0;
        return FALSE;
    }

    return TRUE;
}

static void SpriteCB_MonPic(struct Sprite *sprite)
{
    sprite->x = sMenu->curMonXOffset + 40;
}

// Calculate the max id for sparkles/stars that appear around the Pokémon on the condition screen
// All Pokémon start with 1 sparkle (added by CreateConditionSparkleSprites), so the number here +1
// is the total number of sparkles that appear
static void CalculateNumAdditionalSparkles(u8 monIndex)
{
    u8 sheen = GetMonData(&gParties[B_TRAINER_PLAYER][monIndex], MON_DATA_SHEEN);
    sMenu->numSparkles[sMenu->curLoadId] = GET_NUM_CONDITION_SPARKLES(sheen);
}
