#include "global.h"
#include "item_menu.h"
#include "battle.h"
#include "battle_controllers.h"
#include "battle_pyramid.h"
#include "frontier_util.h"
#include "battle_pyramid_bag.h"
#include "berry_tag_screen.h"
#include "bg.h"
#include "data.h"
#include "decompress.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "event_scripts.h"
#include "field_player_avatar.h"
#include "field_specials.h"
#include "graphics.h"
#include "gpu_regs.h"
#include "international_string_util.h"
#include "item.h"
#include "item_menu_icons.h"
#include "item_use.h"
#include "lilycove_lady.h"
#include "list_menu.h"
#include "link.h"
#include "mail.h"
#include "malloc.h"
#include "map_name_popup.h"
#include "menu.h"
#include "money.h"
#include "overworld.h"
#include "palette.h"
#include "party_menu.h"
#include "player_pc.h"
#include "pokemon.h"
#include "pokemon_summary_screen.h"
#include "scanline_effect.h"
#include "script.h"
#include "shop.h"
#include "sound.h"
#include "sprite.h"
#include "strings.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "menu_helpers.h"
#include "window.h"
#include "apprentice.h"
#include "battle_pike.h"
#include "constants/items.h"
#include "constants/rgb.h"
#include "constants/songs.h"

#define TAG_POCKET_SCROLL_ARROW 110

// The buffer for the bag item list needs to be large enough to hold the maximum
// number of item slots that could fit in a single pocket, + 1 for Cancel.
// This constant picks the max of the existing pocket sizes.
// By default, the largest pocket is BAG_TMHM_COUNT at 64.
#define MAX_POCKET_ITEMS  ((max(BAG_TMHM_COUNT,              \
                            max(BAG_BERRIES_COUNT,           \
                            max(BAG_ITEMS_COUNT,             \
                            max(BAG_KEYITEMS_COUNT,          \
                                BAG_POKEBALLS_COUNT))))) + 1)

// HGSS presents Bag contents in six-item pages.
#define MAX_ITEMS_SHOWN 6

enum {
    SWITCH_POCKET_NONE,
    SWITCH_POCKET_LEFT,
    SWITCH_POCKET_RIGHT
};

enum {
    ACTION_USE,
    ACTION_TOSS,
    ACTION_REGISTER,
    ACTION_GIVE,
    ACTION_CANCEL,
    ACTION_BATTLE_USE,
    ACTION_CHECK,
    ACTION_WALK,
    ACTION_DESELECT,
    ACTION_CHECK_TAG,
    ACTION_CONFIRM,
    ACTION_SHOW,
    ACTION_GIVE_FAVOR_LADY,
    ACTION_CONFIRM_QUIZ_LADY,
    ACTION_BY_NAME,
    ACTION_BY_TYPE,
    ACTION_BY_AMOUNT,
    ACTION_BY_INDEX,
    ACTION_DUMMY,
};

enum {
    WIN_ITEM_LIST,
    WIN_DESCRIPTION,
    WIN_POCKET_NAME,
    WIN_TMHM_INFO_ICONS,
    WIN_TMHM_INFO,
    WIN_MESSAGE, // Identical to ITEMWIN_MESSAGE. Unused?
};

// Item list ID for toSwapPos to indicate an item is not currently being swapped
#define NOT_SWAPPING 0xFF

struct ListBuffer1 {
    struct ListMenuItem subBuffers[MAX_POCKET_ITEMS];
};

struct ListBuffer2 {
    u8 name[MAX_POCKET_ITEMS][max(ITEM_NAME_LENGTH, MOVE_NAME_LENGTH) + 15];
};

struct TempWallyBag {
    struct ItemSlot bagPocket_Items[BAG_ITEMS_COUNT];
    struct ItemSlot bagPocket_PokeBalls[BAG_POKEBALLS_COUNT];
    u16 cursorPosition[POCKETS_COUNT];
    u16 scrollPosition[POCKETS_COUNT];
    u16 unused;
    u16 pocket;
};

static void CB2_Bag(void);
static bool8 SetupBagMenu(void);
static void BagMenu_InitBGs(void);
static bool8 LoadBagMenu_Graphics(void);
static void LoadBagMenuTextWindows(void);
static void AllocateBagItemListBuffers(void);
static void LoadBagItemListBuffers(u8);
static void PrintPocketNames(const u8 *, const u8 *);
static void CopyPocketNameToWindow(u32);
static void DrawPocketIndicatorIcon(u8, bool8);
static void DrawPocketIndicatorIcons(u8);
static void CreatePocketScrollArrowPair(void);
static void PrepareTMHMMoveWindow(void);
static bool8 IsWallysBag(void);
static void Task_WallyTutorialBagMenu(u8);
static void Task_BagMenu_HandleInput(u8);
static void GetItemNameFromPocket(u8 *dest, enum Item itemId);
static void PrintItemDescription(int);
static void BagMenu_PrintCursorAtPos(u8, u8);
static void BagMenu_Print(u8, u8, const u8 *, u8, u8, u8, u8, u8, u8);
static void Task_CloseBagMenu(u8);
static u8 AddItemMessageWindow(u8);
static void RemoveItemMessageWindow(u8);
static void ReturnToItemList(u8);
static void PrintItemQuantity(u8, s16);
static u8 BagMenu_AddWindow(u8);
static u8 GetSwitchBagPocketDirection(void);
static void SwitchBagPocket(u8, s16, bool16);
static void DrawPocketIndicatorIcon(u8 pocket, bool8 isCurrentPocket)
{
    static const u8 sPocketIndicatorXOffset = 4;
    static const u16 sPocketIndicatorInactiveTile = 0x34;
    static const u16 sPocketIndicatorActiveTile = 0x39;
    u16 tile = (isCurrentPocket ? sPocketIndicatorActiveTile : sPocketIndicatorInactiveTile) + pocket;

    FillBgTilemapBufferRect(2, tile, pocket + sPocketIndicatorXOffset, 3, 1, 1, 0);
    ScheduleBgCopyTilemapToVram(2);
}

static void DrawPocketIndicatorIcons(u8 currentPocket)
{
    for (u8 i = 0; i < POCKETS_COUNT; i++)
        DrawPocketIndicatorIcon(i, i == currentPocket);
}

static bool8 CanSwapItems(void);
static void StartItemSwap(u8 taskId);
static void Task_SwitchBagPocket(u8);
static void Task_HandleSwappingItemsInput(u8);
static void DoItemSwap(u8);
static void CancelItemSwap(u8);
static void PrintTMHMMoveData(enum Item itemId);
static void PrintContextMenuItems(u8);
static void PrintContextMenuItemGrid(u8, u8, u8);
static void Task_ItemContext_SingleRow(u8);
static void Task_ItemContext_MultipleRows(u8);
static bool8 IsValidContextMenuPos(s8);
static void BagMenu_RemoveWindow(u8);
static void PrintThereIsNoPokemon(u8);
static void Task_ChooseHowManyToToss(u8);
static void AskTossItems(u8);
static void Task_RemoveItemFromBag(u8);
static void Task_TossItemFromBag(u8 taskId);
static void ItemMenu_Cancel(u8);
static void HandleErrorMessage(u8);
static void PrintItemCantBeHeld(u8);
static void DisplayCurrentMoneyWindow(void);
static void DisplaySellItemPriceAndConfirm(u8);
static void InitSellHowManyInput(u8);
static void AskSellItems(u8);
static void RemoveMoneyWindow(void);
static void Task_ChooseHowManyToSell(u8);
static void SellItem(u8);
static void WaitAfterItemSell(u8);
static void TryDepositItem(u8);
static void Task_ChooseHowManyToDeposit(u8 taskId);
static void WaitDepositErrorMessage(u8);
static void CB2_ApprenticeExitBagMenu(void);
static void CB2_FavorLadyExitBagMenu(void);
static void CB2_QuizLadyExitBagMenu(void);
static void UpdatePocketItemLists(void);
static void InitPocketListPositions(void);
static void InitPocketScrollPositions(void);
static u8 CreateBagInputHandlerTask(u8);
static void DrawItemListBgRow(u8);
static void BagMenu_MoveCursorCallback(s32, bool8, struct ListMenu *);
static void BagMenu_ItemPrintCallback(u8, u32, u8);
static void ItemMenu_UseOutOfBattle(u8);
static void ItemMenu_Toss(u8);
static void ItemMenu_Register(u8);
static void ItemMenu_Give(u8);
static void ItemMenu_Cancel(u8);
static void ItemMenu_UseInBattle(u8);
static void ItemMenu_CheckTag(u8);
static void ItemMenu_Show(u8);
static void ItemMenu_GiveFavorLady(u8);
static void ItemMenu_ConfirmQuizLady(u8);
static void Task_ItemContext_Normal(u8);
static void Task_ItemContext_GiveToParty(u8);
static void Task_ItemContext_Sell(u8);
static void Task_ItemContext_Deposit(u8);
static void Task_ItemContext_GiveToPC(u8);
static void ConfirmToss(u8);
static void CancelToss(u8);
static void ConfirmSell(u8);
static void CancelSell(u8);
static void Task_FadeAndCloseBagMenuIfMulch(u8 taskId);

static const u8 sText_Var1CantBeHeldHere[] = _("The {STR_VAR_1} can't be held\nhere.");
static const u8 sText_DepositHowManyVar1[] = _("Deposit how many\n{STR_VAR_1}?");
static const u8 sText_DepositedVar2Var1s[] = _("Deposited {STR_VAR_2}\n{STR_VAR_1}.");
static const u8 sText_NoRoomForItems[] = _("There's no room to\nstore items.");
static const u8 sText_CantStoreImportantItems[] = _("Important items\ncan't be stored in\nthe PC!");

static void Task_LoadBagSortOptions(u8 taskId);
static void ItemMenu_SortByName(u8 taskId);
static void ItemMenu_SortByType(u8 taskId);
static void ItemMenu_SortByAmount(u8 taskId);
static void ItemMenu_SortByIndex(u8 taskId);
static void SortBagItems(u8 taskId);
static void Task_SortFinish(u8 taskId);
static void MergeSort(struct BagPocket *pocket, s32 (*comparator)(enum Pocket, struct ItemSlot, struct ItemSlot));
static s32 CompareItemsAlphabetically(enum Pocket pocketId, struct ItemSlot item1, struct ItemSlot item2);
static s32 CompareItemsByMost(enum Pocket pocketId, struct ItemSlot item1, struct ItemSlot item2);
static s32 CompareItemsByType(enum Pocket pocketId, struct ItemSlot item1, struct ItemSlot item2);
static s32 CompareItemsByIndex(enum Pocket pocketId, struct ItemSlot item1, struct ItemSlot item2);

static const struct BgTemplate sBgTemplates_ItemMenu[] =
{
    {
        .bg = 0,
        .charBaseIndex = 0,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0,
    },
    {
        .bg = 1,
        .charBaseIndex = 0,
        .mapBaseIndex = 30,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0,
    },
    {
        .bg = 2,
        .charBaseIndex = 3,
        .mapBaseIndex = 29,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 2,
        .baseTile = 0,
    },
    {
        .bg = 3,
        .charBaseIndex = 3,
        .mapBaseIndex = 28,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 3,
        .baseTile = 0,
    },
};

static const struct ListMenuTemplate sItemListMenu =
{
    .items = NULL,
    .moveCursorFunc = BagMenu_MoveCursorCallback,
    .itemPrintFunc = BagMenu_ItemPrintCallback,
    .totalItems = 0,
    .maxShowed = 0,
    .windowId = WIN_ITEM_LIST,
    .header_X = 0,
    .item_X = 12,
    .cursor_X = 0,
    .upText_Y = 2,
    .cursorPal = 1,
    .fillValue = 0,
    .cursorShadowPal = 3,
    .lettersSpacing = 0,
    .itemVerticalPadding = 4,
    .scrollMultiple = LIST_NO_MULTIPLE_SCROLL,
    .fontId = FONT_NARROW,
    .cursorKind = CURSOR_INVISIBLE
};

static const u8 sText_NothingToSort[] = _("There's nothing to sort!");
static const struct MenuAction sItemMenuActions[] = {
    [ACTION_USE]               = {gMenuText_Use,                {ItemMenu_UseOutOfBattle}},
    [ACTION_TOSS]              = {gMenuText_Toss,               {ItemMenu_Toss}},
    [ACTION_REGISTER]          = {gMenuText_Register,           {ItemMenu_Register}},
    [ACTION_GIVE]              = {gMenuText_Give,               {ItemMenu_Give}},
    [ACTION_CANCEL]            = {gText_Cancel2,                {ItemMenu_Cancel}},
    [ACTION_BATTLE_USE]        = {gMenuText_Use,                {ItemMenu_UseInBattle}},
    [ACTION_CHECK]             = {COMPOUND_STRING("CHECK"),     {ItemMenu_UseOutOfBattle}},
    [ACTION_WALK]              = {COMPOUND_STRING("WALK"),      {ItemMenu_UseOutOfBattle}},
    [ACTION_DESELECT]          = {COMPOUND_STRING("DESELECT"),  {ItemMenu_Register}},
    [ACTION_CHECK_TAG]         = {COMPOUND_STRING("CHECK TAG"), {ItemMenu_CheckTag}},
    [ACTION_CONFIRM]           = {gMenuText_Confirm,            {Task_FadeAndCloseBagMenu}},
    [ACTION_SHOW]              = {COMPOUND_STRING("SHOW"),      {ItemMenu_Show}},
    [ACTION_GIVE_FAVOR_LADY]   = {gMenuText_Give2,              {ItemMenu_GiveFavorLady}},
    [ACTION_CONFIRM_QUIZ_LADY] = {gMenuText_Confirm,            {ItemMenu_ConfirmQuizLady}},
    [ACTION_BY_NAME]           = {COMPOUND_STRING("Name"),      {ItemMenu_SortByName}},
    [ACTION_BY_TYPE]           = {COMPOUND_STRING("Type"),      {ItemMenu_SortByType}},
    [ACTION_BY_AMOUNT]         = {COMPOUND_STRING("Amount"),    {ItemMenu_SortByAmount}},
    [ACTION_BY_INDEX]          = {COMPOUND_STRING("Index"),     {ItemMenu_SortByIndex}},
    [ACTION_DUMMY]             = {gText_EmptyString2, {NULL}}
};

// these are all 2D arrays with a width of 2 but are represented as 1D arrays
// ACTION_DUMMY is used to represent blank spaces
static const u8 sContextMenuItems_ItemsPocket[] = {
    ACTION_USE,         ACTION_GIVE,
    ACTION_TOSS,        ACTION_CANCEL
};

static const u8 sContextMenuItems_KeyItemsPocket[] = {
    ACTION_USE,         ACTION_REGISTER,
    ACTION_DUMMY,       ACTION_CANCEL
};

static const u8 sContextMenuItems_BallsPocket[] = {
    ACTION_GIVE,        ACTION_DUMMY,
    ACTION_TOSS,        ACTION_CANCEL
};

static const u8 sContextMenuItems_TmHmPocket[] = {
    ACTION_USE,         ACTION_GIVE,
    ACTION_DUMMY,       ACTION_CANCEL
};

static const u8 sContextMenuItems_BerriesPocket[] = {
    ACTION_CHECK_TAG,   ACTION_DUMMY,
    ACTION_USE,         ACTION_GIVE,
    ACTION_TOSS,        ACTION_CANCEL
};

static const u8 sContextMenuItems_BattleUse[] = {
    ACTION_BATTLE_USE,  ACTION_CANCEL
};

static const u8 sContextMenuItems_Give[] = {
    ACTION_GIVE,        ACTION_CANCEL
};

static const u8 sContextMenuItems_Cancel[] = {
    ACTION_CANCEL
};

static const u8 sContextMenuItems_BerryBlenderCrush[] = {
    ACTION_CONFIRM,     ACTION_CHECK_TAG,
    ACTION_DUMMY,       ACTION_CANCEL
};

static const u8 sContextMenuItems_Apprentice[] = {
    ACTION_SHOW,        ACTION_CANCEL
};

static const u8 sContextMenuItems_FavorLady[] = {
    ACTION_GIVE_FAVOR_LADY, ACTION_CANCEL
};

static const u8 sContextMenuItems_QuizLady[] = {
    ACTION_CONFIRM_QUIZ_LADY, ACTION_CANCEL
};

static const TaskFunc sContextMenuFuncs[] =
{
    [ITEMMENULOCATION_FIELD]                    = Task_ItemContext_Normal,
    [ITEMMENULOCATION_BATTLE]                   = Task_ItemContext_Normal,
    [ITEMMENULOCATION_PARTY]                    = Task_ItemContext_GiveToParty,
    [ITEMMENULOCATION_SHOP]                     = Task_ItemContext_Sell,
    [ITEMMENULOCATION_BERRY_TREE]               = Task_FadeAndCloseBagMenu,
    [ITEMMENULOCATION_BERRY_BLENDER_CRUSH]      = Task_ItemContext_Normal,
    [ITEMMENULOCATION_ITEMPC]                   = Task_ItemContext_Deposit,
    [ITEMMENULOCATION_FAVOR_LADY]               = Task_ItemContext_Normal,
    [ITEMMENULOCATION_QUIZ_LADY]                = Task_ItemContext_Normal,
    [ITEMMENULOCATION_APPRENTICE]               = Task_ItemContext_Normal,
    [ITEMMENULOCATION_WALLY]                    = NULL,
    [ITEMMENULOCATION_PCBOX]                    = Task_ItemContext_GiveToPC,
    [ITEMMENULOCATION_BERRY_TREE_MULCH]         = Task_FadeAndCloseBagMenuIfMulch,
    [ITEMMENULOCATION_RAIDEND]                  = Task_ItemContext_Normal,
};

static const struct YesNoFuncTable sYesNoTossFunctions = {ConfirmToss, CancelToss};

static const struct YesNoFuncTable sYesNoSellItemFunctions = {ConfirmSell, CancelSell};

static const u8 sText_Registered[] = _("REG");

// Bag-only HGSS-style page chevron. The bottom indicator reuses this graphic vertically flipped.
static const ALIGNED(4) u8 sHgssBagScrollArrow_Gfx[] =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x31,
    0x00, 0x00, 0x10, 0x22, 0x00, 0x00, 0x21, 0x01, 0x00, 0x10, 0x12, 0x00, 0x00, 0x21, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00,
    0x22, 0x01, 0x00, 0x00, 0x10, 0x12, 0x00, 0x00, 0x00, 0x21, 0x01, 0x00, 0x00, 0x10, 0x12, 0x00,
    0x00, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const u16 sHgssBagScrollArrow_Pal[] =
{
    RGB(0, 0, 0), RGB(6, 8, 15), RGB(18, 23, 31), RGB(28, 30, 31),
    RGB(0, 0, 0), RGB(0, 0, 0), RGB(0, 0, 0), RGB(0, 0, 0),
    RGB(0, 0, 0), RGB(0, 0, 0), RGB(0, 0, 0), RGB(0, 0, 0),
    RGB(0, 0, 0), RGB(0, 0, 0), RGB(0, 0, 0), RGB(0, 0, 0),
};

static const struct SpriteSheet sHgssBagScrollArrowSpriteSheet =
{
    .data = sHgssBagScrollArrow_Gfx,
    .size = sizeof(sHgssBagScrollArrow_Gfx),
    .tag = TAG_POCKET_SCROLL_ARROW,
};

static const struct SpritePalette sHgssBagScrollArrowSpritePalette =
{
    .data = sHgssBagScrollArrow_Pal,
    .tag = TAG_POCKET_SCROLL_ARROW,
};

static const struct OamData sHgssBagScrollArrowOam =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(16x16),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(16x16),
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 0,
    .affineParam = 0,
};

static const union AnimCmd sHgssBagScrollArrowAnim[] =
{
    ANIMCMD_FRAME(0, 0),
    ANIMCMD_END
};

static const union AnimCmd *const sHgssBagScrollArrowAnimTable[] =
{
    sHgssBagScrollArrowAnim
};

static const struct SpriteTemplate sHgssBagScrollArrowSpriteTemplate =
{
    .tileTag = TAG_POCKET_SCROLL_ARROW,
    .paletteTag = TAG_POCKET_SCROLL_ARROW,
    .oam = &sHgssBagScrollArrowOam,
    .anims = sHgssBagScrollArrowAnimTable,
    .callback = SpriteCallbackDummy,
};

#define HGSS_BAG_LIST_CURSOR_WIDTH  8
#define HGSS_BAG_LIST_CURSOR_HEIGHT 16
#define HGSS_BAG_ITEM_CELL_HEIGHT   20
#define HGSS_BAG_CELL_NORMAL_COLOR  3
#define HGSS_BAG_CELL_ACTIVE_COLOR  4
#define HGSS_BAG_CELL_CLOSE_COLOR   6

// HGSS adaption: use a touch-style selection tab instead of Emerald's text arrow.
// Pixel values reference the gender-specific Bag palette:
// 4 = active accent, 6 = muted swap marker, 2 = light border.
static const u8 sHgssBagListCursor_Gfx[] =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x22, 0x22, 0x00,
    0x20, 0x44, 0x44, 0x02, 0x42, 0x44, 0x44, 0x24,
    0x42, 0x44, 0x44, 0x24, 0x42, 0x44, 0x44, 0x24,
    0x42, 0x44, 0x44, 0x24, 0x42, 0x44, 0x44, 0x24,
    0x42, 0x44, 0x44, 0x24, 0x42, 0x44, 0x44, 0x24,
    0x42, 0x44, 0x44, 0x24, 0x20, 0x44, 0x44, 0x02,
    0x42, 0x44, 0x44, 0x24, 0x42, 0x44, 0x44, 0x24,
    0x00, 0x22, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const u8 sHgssBagListCursorGray_Gfx[] =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x22, 0x22, 0x00,
    0x20, 0x66, 0x66, 0x02, 0x62, 0x66, 0x66, 0x26,
    0x62, 0x66, 0x66, 0x26, 0x62, 0x66, 0x66, 0x26,
    0x62, 0x66, 0x66, 0x26, 0x62, 0x66, 0x66, 0x26,
    0x62, 0x66, 0x66, 0x26, 0x62, 0x66, 0x66, 0x26,
    0x62, 0x66, 0x66, 0x26, 0x20, 0x66, 0x66, 0x02,
    0x62, 0x66, 0x66, 0x26, 0x62, 0x66, 0x66, 0x26,
    0x00, 0x22, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00,
};

enum {
    COLORID_NORMAL,
    COLORID_POCKET_NAME,
    COLORID_POCKET_TITLE,
    COLORID_GRAY_CURSOR,
    COLORID_QUANTITY,
    COLORID_UNUSED,
    COLORID_TMHM_INFO,
    COLORID_NONE = 0xFF
};
static const u8 sFontColorTable[][3] = {
                            // bgColor, textColor, shadowColor
    [COLORID_NORMAL]       = {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_WHITE,      TEXT_COLOR_LIGHT_GRAY},
    [COLORID_POCKET_NAME]  = {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_WHITE,      TEXT_COLOR_RED},
    [COLORID_POCKET_TITLE] = {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_WHITE,      HGSS_BAG_CELL_ACTIVE_COLOR},
    [COLORID_GRAY_CURSOR]  = {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_LIGHT_GRAY, TEXT_COLOR_GREEN},
    [COLORID_QUANTITY]    = {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_LIGHT_GRAY, TEXT_COLOR_DARK_GRAY},
    [COLORID_UNUSED]      = {TEXT_COLOR_DARK_GRAY,   TEXT_COLOR_WHITE,      TEXT_COLOR_LIGHT_GRAY},
    [COLORID_TMHM_INFO]   = {TEXT_COLOR_TRANSPARENT, TEXT_DYNAMIC_COLOR_5,  TEXT_DYNAMIC_COLOR_1}
};

static const struct WindowTemplate sDefaultBagWindows[] =
{
    [WIN_ITEM_LIST] = {
        .bg = 0,
        .tilemapLeft = 14,
        .tilemapTop = 2,
        .width = 15,
        .height = 16,
        .paletteNum = 1,
        .baseBlock = 0x27,
    },
    [WIN_DESCRIPTION] = {
        .bg = 0,
        .tilemapLeft = 0,
        .tilemapTop = 13,
        .width = 14,
        .height = 6,
        .paletteNum = 1,
        .baseBlock = 0x117,
    },
    [WIN_POCKET_NAME] = {
        .bg = 0,
        .tilemapLeft = 4,
        .tilemapTop = 1,
        .width = 8,
        .height = 2,
        .paletteNum = 1,
        .baseBlock = 0x1A1,
    },
    [WIN_TMHM_INFO_ICONS] = {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 13,
        .width = 5,
        .height = 6,
        .paletteNum = 12,
        .baseBlock = 0x16B,
    },
    [WIN_TMHM_INFO] = {
        .bg = 0,
        .tilemapLeft = 7,
        .tilemapTop = 13,
        .width = 4,
        .height = 6,
        .paletteNum = 12,
        .baseBlock = 0x189,
    },
    [WIN_MESSAGE] = {
        .bg = 1,
        .tilemapLeft = 2,
        .tilemapTop = 15,
        .width = 27,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 0x1B1,
    },
    DUMMY_WIN_TEMPLATE,
};

static const struct WindowTemplate sContextMenuWindowTemplates[] =
{
    [ITEMWIN_1x1] = {
        .bg = 1,
        .tilemapLeft = 22,
        .tilemapTop = 17,
        .width = 7,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x21D,
    },
    [ITEMWIN_1x2] = {
        .bg = 1,
        .tilemapLeft = 22,
        .tilemapTop = 15,
        .width = 7,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 0x21D,
    },
    [ITEMWIN_2x2] = {
        .bg = 1,
        .tilemapLeft = 15,
        .tilemapTop = 15,
        .width = 14,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 0x21D,
    },
    [ITEMWIN_2x3] = {
        .bg = 1,
        .tilemapLeft = 15,
        .tilemapTop = 13,
        .width = 14,
        .height = 6,
        .paletteNum = 15,
        .baseBlock = 0x21D,
    },
    [ITEMWIN_MESSAGE] = {
        .bg = 1,
        .tilemapLeft = 2,
        .tilemapTop = 15,
        .width = 27,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 0x1B1,
    },
    [ITEMWIN_YESNO_LOW] = { // Yes/No tucked in corner, for toss confirm
        .bg = 1,
        .tilemapLeft = 24,
        .tilemapTop = 15,
        .width = 5,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 0x21D,
    },
    [ITEMWIN_YESNO_HIGH] = { // Yes/No higher up, positioned above a lower message box
        .bg = 1,
        .tilemapLeft = 21,
        .tilemapTop = 9,
        .width = 5,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 0x21D,
    },
    [ITEMWIN_QUANTITY] = { // Used for quantity of items to Toss/Deposit
        .bg = 1,
        .tilemapLeft = 24,
        .tilemapTop = 17,
        .width = 5,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x21D,
    },
    [ITEMWIN_QUANTITY_WIDE] = { // Used for quantity and price of items to Sell
        .bg = 1,
        .tilemapLeft = 18,
        .tilemapTop = 11,
        .width = 10,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x245,
    },
    [ITEMWIN_MONEY] = {
        .bg = 1,
        .tilemapLeft = 1,
        .tilemapTop = 1,
        .width = 13,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x259,
    },
};

EWRAM_DATA struct BagMenu *gBagMenu = 0;
EWRAM_DATA struct BagPosition gBagPosition = {0};
static EWRAM_DATA struct ListBuffer1 *sListBuffer1 = 0;
static EWRAM_DATA struct ListBuffer2 *sListBuffer2 = 0;
EWRAM_DATA enum Item gSpecialVar_ItemId = 0;
static EWRAM_DATA struct TempWallyBag *sTempWallyBag = 0;
static EWRAM_DATA struct YesNoFuncTable sHgssBagYesNoFuncs = {0};
static EWRAM_DATA u8 sHgssBagYesNoWindowType = ITEMWIN_YESNO_LOW;
static EWRAM_DATA u8 sHgssBagYesNoChoice = 0;
static EWRAM_DATA TaskFunc sHgssBagMessageCallback = NULL;

void ResetBagScrollPositions(void)
{
    gBagPosition.pocket = POCKET_ITEMS;
    memset(gBagPosition.cursorPosition, 0, sizeof(gBagPosition.cursorPosition));
    memset(gBagPosition.scrollPosition, 0, sizeof(gBagPosition.scrollPosition));
}

void CB2_BagMenuFromStartMenu(void)
{
    GoToBagMenu(ITEMMENULOCATION_FIELD, POCKETS_COUNT, CB2_ReturnToFieldWithOpenMenu);
}

void CB2_BagMenuFromBattle(void)
{
    if (CurrentBattlePyramidLocation() == PYRAMID_LOCATION_NONE)
        GoToBagMenu(ITEMMENULOCATION_BATTLE, POCKETS_COUNT, CB2_SetUpReshowBattleScreenAfterMenu2);
    else
        GoToBattlePyramidBagMenu(PYRAMIDBAG_LOC_BATTLE, CB2_SetUpReshowBattleScreenAfterMenu2);
}

// Choosing berry to plant
void CB2_ChooseBerry(void)
{
    GoToBagMenu(ITEMMENULOCATION_BERRY_TREE, POCKET_BERRIES, CB2_ReturnToFieldContinueScript);
}

// Choosing mulch to use
void CB2_ChooseMulch(void)
{
    GoToBagMenu(ITEMMENULOCATION_BERRY_TREE_MULCH, POCKET_ITEMS, CB2_ReturnToFieldContinueScript);
}

// Choosing berry for Berry Blender or Berry Crush
void ChooseBerryForMachine(MainCallback exitCallback)
{
    GoToBagMenu(ITEMMENULOCATION_BERRY_BLENDER_CRUSH, POCKET_BERRIES, exitCallback);
}

void CB2_ChooseBall(void)
{
    GoToBagMenu(ITEMMENULOCATION_RAIDEND, POCKET_POKE_BALLS, CB2_SetUpReshowBattleScreenAfterMenu2);
}

void CB2_GoToSellMenu(void)
{
    GoToBagMenu(ITEMMENULOCATION_SHOP, POCKETS_COUNT, CB2_ExitSellMenu);
}

void CB2_GoToItemDepositMenu(void)
{
    GoToBagMenu(ITEMMENULOCATION_ITEMPC, POCKETS_COUNT, CB2_PlayerPCExitBagMenu);
}

void ApprenticeOpenBagMenu(void)
{
    GoToBagMenu(ITEMMENULOCATION_APPRENTICE, POCKETS_COUNT, CB2_ApprenticeExitBagMenu);
    gSpecialVar_0x8005 = ITEM_NONE;
    gSpecialVar_Result = FALSE;
}

void FavorLadyOpenBagMenu(void)
{
    GoToBagMenu(ITEMMENULOCATION_FAVOR_LADY, POCKETS_COUNT, CB2_FavorLadyExitBagMenu);
    gSpecialVar_Result = FALSE;
}

void QuizLadyOpenBagMenu(void)
{
    GoToBagMenu(ITEMMENULOCATION_QUIZ_LADY, POCKETS_COUNT, CB2_QuizLadyExitBagMenu);
    gSpecialVar_Result = FALSE;
}

void GoToBagMenu(u8 location, u8 pocket, MainCallback exitCallback)
{
    gBagMenu = AllocZeroed(sizeof(*gBagMenu));
    if (gBagMenu == NULL)
    {
        // Alloc failed, exit
        SetMainCallback2(exitCallback);
    }
    else
    {
        if (location != ITEMMENULOCATION_LAST)
            gBagPosition.location = location;
        if (exitCallback)
            gBagPosition.exitCallback = exitCallback;
        if (pocket < POCKETS_COUNT)
            gBagPosition.pocket = pocket;
        if (gBagPosition.location == ITEMMENULOCATION_BERRY_TREE
         || gBagPosition.location == ITEMMENULOCATION_BERRY_BLENDER_CRUSH
         || gBagPosition.location == ITEMMENULOCATION_BERRY_TREE_MULCH
         || gBagPosition.location == ITEMMENULOCATION_RAIDEND)
            gBagMenu->pocketSwitchDisabled = TRUE;
        gBagMenu->newScreenCallback = NULL;
        gBagMenu->toSwapPos = NOT_SWAPPING;
        gBagMenu->pocketScrollArrowsTask = TASK_NONE;
        memset(gBagMenu->spriteIds, SPRITE_NONE, sizeof(gBagMenu->spriteIds));
        memset(gBagMenu->windowIds, WINDOW_NONE, sizeof(gBagMenu->windowIds));
        SetMainCallback2(CB2_Bag);
    }
}

void CB2_BagMenuRun(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    DoScheduledBgTilemapCopiesToVram();
    UpdatePaletteFade();
}

void VBlankCB_BagMenuRun(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

#define tListTaskId        data[0]
#define tListPosition      data[1]
#define tQuantity          data[2]
#define tNeverRead         data[3]
#define tItemCount         data[8]
#define tMsgWindowId       data[10]
#define tPocketSwitchDir   data[11]
#define tPocketSwitchTimer data[12]
#define tPocketSwitchState data[13]

static void CB2_Bag(void)
{
    while (MenuHelpers_ShouldWaitForLinkRecv() != TRUE && SetupBagMenu() != TRUE && MenuHelpers_IsLinkActive() != TRUE)
        {};
}

static bool8 SetupBagMenu(void)
{
    u8 taskId;

    switch (gMain.state)
    {
    case 0:
        SetVBlankHBlankCallbacksToNull();
        ClearScheduledBgCopiesToVram();
        gMain.state++;
        break;
    case 1:
        ScanlineEffect_Stop();
        gMain.state++;
        break;
    case 2:
        FreeAllSpritePalettes();
        gMain.state++;
        break;
    case 3:
        ResetPaletteFade();
        gPaletteFade.bufferTransferDisabled = TRUE;
        gMain.state++;
        break;
    case 4:
        ResetSpriteData();
        gMain.state++;
        break;
    case 5:
        gMain.state++;
        break;
    case 6:
        if (!MenuHelpers_IsLinkActive())
            ResetTasks();
        gMain.state++;
        break;
    case 7:
        BagMenu_InitBGs();
        gBagMenu->graphicsLoadState = 0;
        gMain.state++;
        break;
    case 8:
        if (!LoadBagMenu_Graphics())
            break;
        gMain.state++;
        break;
    case 9:
        LoadBagMenuTextWindows();
        gMain.state++;
        break;
    case 10:
        UpdatePocketItemLists();
        InitPocketListPositions();
        InitPocketScrollPositions();
        gMain.state++;
        break;
    case 11:
        AllocateBagItemListBuffers();
        gMain.state++;
        break;
    case 12:
        LoadBagItemListBuffers(gBagPosition.pocket);
        gMain.state++;
        break;
    case 13:
        PrintPocketNames(gPocketNamesStringsTable[gBagPosition.pocket], 0);
        CopyPocketNameToWindow(0);
        DrawPocketIndicatorIcons(gBagPosition.pocket);
        gMain.state++;
        break;
    case 14:
        taskId = CreateBagInputHandlerTask(gBagPosition.location);
        gTasks[taskId].tListTaskId = ListMenuInit(&gMultiuseListMenuTemplate, gBagPosition.scrollPosition[gBagPosition.pocket], gBagPosition.cursorPosition[gBagPosition.pocket]);
        gTasks[taskId].tNeverRead = 0;
        gTasks[taskId].tItemCount = 0;
        gMain.state++;
        break;
    case 15:
        AddBagVisualSprite(gBagPosition.pocket);
        gMain.state++;
        break;
    case 16:
        gMain.state++;
        break;
    case 17:
        CreatePocketScrollArrowPair();
        gMain.state++;
        break;
    case 18:
        PrepareTMHMMoveWindow();
        gMain.state++;
        break;
    case 19:
        BlendPalettes(PALETTES_ALL, 16, 0);
        gMain.state++;
        break;
    case 20:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
        gPaletteFade.bufferTransferDisabled = FALSE;
        gMain.state++;
        break;
    default:
        SetVBlankCallback(VBlankCB_BagMenuRun);
        SetMainCallback2(CB2_BagMenuRun);
        return TRUE;
    }
    return FALSE;
}

static void BagMenu_InitBGs(void)
{
    ResetVramOamAndBgCntRegs();
    memset(gBagMenu->tilemapBuffer, 0, sizeof(gBagMenu->tilemapBuffer));
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sBgTemplates_ItemMenu, ARRAY_COUNT(sBgTemplates_ItemMenu));
    SetBgTilemapBuffer(2, gBagMenu->tilemapBuffer[BAG_MENU_BG_NORMAL]);
    SetBgTilemapBuffer(3, gBagMenu->tilemapBuffer[BAG_MENU_BG_SCROLLING]);
    ResetAllBgsCoordinates();
    ScheduleBgCopyTilemapToVram(2);
    ScheduleBgCopyTilemapToVram(3);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
    ShowBg(0);
    ShowBg(1);
    ShowBg(2);
    ShowBg(3);
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
}

static const u32 sBagMenuScrollingBgTilemap[] = INCGFX_U32("graphics/bag/scrolling_bg.bin", ".smolTM");

static bool8 LoadBagMenu_Graphics(void)
{
    switch (gBagMenu->graphicsLoadState)
    {
    case 0:
        ResetTempTileDataBuffers();
        DecompressAndCopyTileDataToVram(2, gBagScreen_Gfx, 0, 0, 0);
        gBagMenu->graphicsLoadState++;
        break;
    case 1:
        if (FreeTempTileDataBuffersIfPossible() != TRUE)
        {
            DecompressDataWithHeaderWram(gBagScreen_GfxTileMap, gBagMenu->tilemapBuffer[BAG_MENU_BG_NORMAL]);
            gBagMenu->graphicsLoadState++;
        }
        break;
    case 2:
        DecompressDataWithHeaderWram(sBagMenuScrollingBgTilemap, gBagMenu->tilemapBuffer[BAG_MENU_BG_SCROLLING]);
        gBagMenu->graphicsLoadState++;
        break;
    case 3:
        if (!IsWallysBag() && gSaveBlock2Ptr->playerGender != MALE)
            LoadPalette(gBagScreenFemale_Pal, BG_PLTT_ID(0), 2 * PLTT_SIZE_4BPP);
        else
            LoadPalette(gBagScreenMale_Pal, BG_PLTT_ID(0), 2 * PLTT_SIZE_4BPP);
        gBagMenu->graphicsLoadState++;
        break;
    case 4:
        if (IsWallysBag() == TRUE || gSaveBlock2Ptr->playerGender == MALE)
            LoadCompressedSpriteSheet(&gBagMaleSpriteSheet);
        else
            LoadCompressedSpriteSheet(&gBagFemaleSpriteSheet);
        gBagMenu->graphicsLoadState++;
        break;
    case 5:
        if (IsWallysBag() == TRUE || gSaveBlock2Ptr->playerGender == MALE)
            LoadSpritePalette(&gBagMalePaletteTable);
        else
            LoadSpritePalette(&gBagFemalePaletteTable);
        gBagMenu->graphicsLoadState++;
        break;
    default:
        gBagMenu->graphicsLoadState = 0;
        return TRUE;
    }
    return FALSE;
}

static u8 CreateBagInputHandlerTask(u8 location)
{
    u8 taskId;
    if (location == ITEMMENULOCATION_WALLY)
        taskId = CreateTask(Task_WallyTutorialBagMenu, 0);
    else
        taskId = CreateTask(Task_BagMenu_HandleInput, 0);
    return taskId;
}

static void AllocateBagItemListBuffers(void)
{
    sListBuffer1 = Alloc(sizeof(*sListBuffer1));
    sListBuffer2 = Alloc(sizeof(*sListBuffer2));
}

static void LoadBagItemListBuffers(u8 pocketId)
{
    u16 i;
    struct ListMenuItem *subBuffer;

    if (!gBagMenu->hideCloseBagText)
    {
        for (i = 0; i < gBagMenu->numItemStacks[pocketId] - 1; i++)
        {
            GetItemNameFromPocket(sListBuffer2->name[i], GetBagItemId(pocketId, i));
            subBuffer = sListBuffer1->subBuffers;
            subBuffer[i].name = sListBuffer2->name[i];
            subBuffer[i].id = i;
        }
        subBuffer = sListBuffer1->subBuffers;
        subBuffer[i].name = gText_EmptyString2;
        subBuffer[i].id = LIST_CANCEL;
    }
    else
    {
        for (i = 0; i < gBagMenu->numItemStacks[pocketId]; i++)
        {
            GetItemNameFromPocket(sListBuffer2->name[i], GetBagItemId(pocketId, i));
            subBuffer = sListBuffer1->subBuffers;
            subBuffer[i].name = sListBuffer2->name[i];
            subBuffer[i].id = i;
        }
    }
    gMultiuseListMenuTemplate = sItemListMenu;
    gMultiuseListMenuTemplate.totalItems = gBagMenu->numItemStacks[pocketId];
    gMultiuseListMenuTemplate.items = sListBuffer1->subBuffers;
    gMultiuseListMenuTemplate.maxShowed = gBagMenu->numShownItems[pocketId];
}

static void GetItemNameFromPocket(u8 *dest, enum Item itemId)
{
    u8 *end;
    switch (gBagPosition.pocket)
    {
    case POCKET_TM_HM:
        end = StringCopy(gStringVar2, GetMoveName(ItemIdToBattleMoveId(itemId)));
        PrependFontIdToFit(gStringVar2, end, FONT_NARROW, NUM_TECHNICAL_MACHINES >= 100 ? 56 : 61);
        if (GetItemTMHMIndex(itemId) > NUM_TECHNICAL_MACHINES)
        {
            // Get HM number
            ConvertIntToDecimalStringN(gStringVar1, GetItemTMHMIndex(itemId) - NUM_TECHNICAL_MACHINES, STR_CONV_MODE_LEADING_ZEROS, 1);
            StringExpandPlaceholders(dest, gText_NumberItem_HM);
        }
        else
        {
            // Get TM number
            ConvertIntToDecimalStringN(gStringVar1, GetItemTMHMIndex(itemId), STR_CONV_MODE_LEADING_ZEROS, NUM_TECHNICAL_MACHINES >= 100 ? 3 : 2);
            StringExpandPlaceholders(dest, gText_NumberItem_TMBerry);
        }
        break;
    case POCKET_BERRIES:
        ConvertIntToDecimalStringN(gStringVar1, ItemIdToBerryType(itemId), STR_CONV_MODE_LEADING_ZEROS, 2);
        end = CopyItemName(itemId, gStringVar2);
        PrependFontIdToFit(gStringVar2, end, FONT_NARROW, 57);
        StringExpandPlaceholders(dest, gText_NumberItem_TMBerry);
        break;
    default:
        end = CopyItemName(itemId, dest);
        PrependFontIdToFit(dest, end, FONT_NARROW,
                           (gBagPosition.pocket == POCKET_KEY_ITEMS || GetItemImportance(itemId)) ? 82 : 84);
        break;
    }
}

static void DrawHgssBagItemCellFrame(u8 windowId, u8 y, u8 color)
{
    u8 top = y - 2;
    u8 left = HGSS_BAG_LIST_CURSOR_WIDTH;
    u8 width = GetWindowAttribute(windowId, WINDOW_WIDTH) * 8 - left;

    // HGSS list cells use softened one-pixel corners instead of a hard rectangular box.
    FillWindowPixelRect(windowId, PIXEL_FILL(color), left + 1, top, width - 2, 1);
    FillWindowPixelRect(windowId, PIXEL_FILL(color), left + 1, top + HGSS_BAG_ITEM_CELL_HEIGHT - 1, width - 2, 1);
    FillWindowPixelRect(windowId, PIXEL_FILL(color), left, top + 1, 1, HGSS_BAG_ITEM_CELL_HEIGHT - 2);
    FillWindowPixelRect(windowId, PIXEL_FILL(color), left + width - 1, top + 1, 1, HGSS_BAG_ITEM_CELL_HEIGHT - 2);

    // HGSS gives the selected touch cell a bright inner edge inside its stronger accent border.
    if (color == HGSS_BAG_CELL_ACTIVE_COLOR)
    {
        FillWindowPixelRect(windowId, PIXEL_FILL(2), left + 2, top + 1, width - 4, 1);
        FillWindowPixelRect(windowId, PIXEL_FILL(2), left + 2, top + HGSS_BAG_ITEM_CELL_HEIGHT - 2, width - 4, 1);
        FillWindowPixelRect(windowId, PIXEL_FILL(2), left + 1, top + 2, 1, HGSS_BAG_ITEM_CELL_HEIGHT - 4);
        FillWindowPixelRect(windowId, PIXEL_FILL(2), left + width - 2, top + 2, 1, HGSS_BAG_ITEM_CELL_HEIGHT - 4);
    }
}

static void DrawHgssBagRegisteredMarker(u8 windowId, u8 y)
{
    const u8 left = 95;
    const u8 top = y;
    const u8 width = 23;
    const u8 height = 16;

    // Compact HGSS-style registration tab, replacing Emerald's SELECT-button badge.
    BagMenu_Print(windowId, FONT_SMALL, sText_Registered,
                  left + 1 + GetStringCenterAlignXOffset(FONT_SMALL, sText_Registered, width - 2), y + 2,
                  0, 0, TEXT_SKIP_DRAW, COLORID_POCKET_TITLE);

    // Restore the tab frame above the label.
    FillWindowPixelRect(windowId, PIXEL_FILL(HGSS_BAG_CELL_ACTIVE_COLOR), left + 1, top, width - 2, 1);
    FillWindowPixelRect(windowId, PIXEL_FILL(HGSS_BAG_CELL_ACTIVE_COLOR), left + 1, top + height - 1, width - 2, 1);
    FillWindowPixelRect(windowId, PIXEL_FILL(HGSS_BAG_CELL_ACTIVE_COLOR), left, top + 1, 1, height - 2);
    FillWindowPixelRect(windowId, PIXEL_FILL(HGSS_BAG_CELL_ACTIVE_COLOR), left + width - 1, top + 1, 1, height - 2);
}

static void BagMenu_MoveCursorCallback(s32 itemIndex, bool8 onInit, struct ListMenu *list)
{
    u8 cursorY = list->selectedRow * (GetFontAttribute(list->template.fontId, FONTATTR_MAX_LETTER_HEIGHT)
                                   + list->template.itemVerticalPadding)
               + list->template.upText_Y;

    // The list engine no longer draws an Emerald arrow, so refresh the Bag-specific selector column here.
    FillWindowPixelRect(WIN_ITEM_LIST, PIXEL_FILL(0), 0, 0,
                        HGSS_BAG_LIST_CURSOR_WIDTH,
                        GetWindowAttribute(WIN_ITEM_LIST, WINDOW_HEIGHT) * 8);

    // Reset all visible cell frames before highlighting the current selection.
    for (u8 row = 0; row < list->template.maxShowed; row++)
    {
        u8 rowY = row * (GetFontAttribute(list->template.fontId, FONTATTR_MAX_LETTER_HEIGHT)
                       + list->template.itemVerticalPadding)
                + list->template.upText_Y;
        u8 frameColor = list->template.items[list->scrollOffset + row].id == LIST_CANCEL
                      ? HGSS_BAG_CELL_CLOSE_COLOR
                      : HGSS_BAG_CELL_NORMAL_COLOR;

        DrawHgssBagItemCellFrame(WIN_ITEM_LIST, rowY, frameColor);
    }
    DrawHgssBagItemCellFrame(WIN_ITEM_LIST, cursorY, HGSS_BAG_CELL_ACTIVE_COLOR);
    if (itemIndex == LIST_CANCEL)
    {
        u8 closeTop = cursorY - 2;
        u8 closeLeft = HGSS_BAG_LIST_CURSOR_WIDTH;
        u8 closeWidth = GetWindowAttribute(WIN_ITEM_LIST, WINDOW_WIDTH) * 8 - closeLeft;

        // Keep the close row visually distinct even while it carries the active selection frame.
        FillWindowPixelRect(WIN_ITEM_LIST, PIXEL_FILL(HGSS_BAG_CELL_CLOSE_COLOR),
                            closeLeft + 2, closeTop + 1, closeWidth - 4, 1);
        FillWindowPixelRect(WIN_ITEM_LIST, PIXEL_FILL(HGSS_BAG_CELL_CLOSE_COLOR),
                            closeLeft + 2, closeTop + HGSS_BAG_ITEM_CELL_HEIGHT - 2,
                            closeWidth - 4, 1);
    }

    // Preserve the source marker while moving an item, then draw the current selection on top.
    if (gBagMenu->toSwapPos != NOT_SWAPPING
     && gBagMenu->toSwapPos >= list->scrollOffset
     && gBagMenu->toSwapPos < list->scrollOffset + list->template.maxShowed)
    {
        u8 swapRow = gBagMenu->toSwapPos - list->scrollOffset;
        u8 swapY = swapRow * (GetFontAttribute(list->template.fontId, FONTATTR_MAX_LETTER_HEIGHT)
                            + list->template.itemVerticalPadding)
                 + list->template.upText_Y;
        BagMenu_PrintCursorAtPos(swapY, COLORID_GRAY_CURSOR);
    }
    BagMenu_PrintCursorAtPos(cursorY, COLORID_NORMAL);

    if (onInit != TRUE)
    {
        PlaySE(SE_SELECT);
        ShakeBagSprite();
    }
    if (gBagMenu->toSwapPos == NOT_SWAPPING)
    {
        RemoveBagItemIconSprite(gBagMenu->itemIconSlot ^ 1);
        if (itemIndex != LIST_CANCEL)
           AddBagItemIconSprite(GetBagItemId(gBagPosition.pocket, itemIndex), gBagMenu->itemIconSlot);
        else
           AddBagItemIconSprite(ITEM_LIST_END, gBagMenu->itemIconSlot);
        gBagMenu->itemIconSlot ^= 1;
        if (!gBagMenu->inhibitItemDescriptionPrint)
            PrintItemDescription(itemIndex);
    }
}

static void BagMenu_ItemPrintCallback(u8 windowId, u32 itemIndex, u8 y)
{
    DrawHgssBagItemCellFrame(windowId, y,
                             itemIndex == LIST_CANCEL ? HGSS_BAG_CELL_CLOSE_COLOR : HGSS_BAG_CELL_NORMAL_COLOR);

    if (itemIndex == LIST_CANCEL)
    {
        u8 closeLeft = HGSS_BAG_LIST_CURSOR_WIDTH + 1;
        u8 closeWidth = GetWindowAttribute(windowId, WINDOW_WIDTH) * 8 - closeLeft - 1;

        BagMenu_Print(windowId, FONT_NARROW, gText_CloseBag,
                      closeLeft + GetStringCenterAlignXOffset(FONT_NARROW, gText_CloseBag, closeWidth), y,
                      0, 0, TEXT_SKIP_DRAW, COLORID_QUANTITY);
    }
    else
    {
        s32 offset;

        if (gBagMenu->toSwapPos != NOT_SWAPPING)
        {
            // Swapping items, draw cursor at original item's location
            if (gBagMenu->toSwapPos == (u8)itemIndex)
                BagMenu_PrintCursorAtPos(y, COLORID_GRAY_CURSOR);
            else
                BagMenu_PrintCursorAtPos(y, COLORID_NONE);
        }

        struct ItemSlot itemSlot = GetBagItemIdAndQuantity(gBagPosition.pocket, itemIndex);

        // Draw HM icon
        if (gBagPosition.pocket == POCKET_TM_HM && GetItemTMHMIndex(itemSlot.itemId) > NUM_TECHNICAL_MACHINES)
            BlitBitmapToWindow(windowId, gBagMenuHMIcon_Gfx, 10, y, 16, 16);

        if (gBagPosition.pocket != POCKET_KEY_ITEMS && GetItemImportance(itemSlot.itemId) == FALSE)
        {
            // Print item quantity
            ConvertIntToDecimalStringN(gStringVar1, itemSlot.quantity, STR_CONV_MODE_RIGHT_ALIGN, MAX_ITEM_DIGITS);
            StringExpandPlaceholders(gStringVar4, gText_xVar1);
            offset = GetStringRightAlignXOffset(FONT_NARROW, gStringVar4, 118);
            BagMenu_Print(windowId, FONT_NARROW, gStringVar4, offset, y, 0, 0, TEXT_SKIP_DRAW, COLORID_QUANTITY);
        }
        else
        {
            // Print registered icon
            if (gSaveBlock1Ptr->registeredItem != ITEM_NONE && gSaveBlock1Ptr->registeredItem == itemSlot.itemId)
                DrawHgssBagRegisteredMarker(windowId, y);
        }
    }
}

static void PrepareHgssBagDescriptionPanel(void)
{
    u8 width = GetWindowAttribute(WIN_DESCRIPTION, WINDOW_WIDTH) * 8;
    u8 height = GetWindowAttribute(WIN_DESCRIPTION, WINDOW_HEIGHT) * 8;

    FillWindowPixelBuffer(WIN_DESCRIPTION, PIXEL_FILL(0));
    FillWindowPixelRect(WIN_DESCRIPTION, PIXEL_FILL(HGSS_BAG_CELL_ACTIVE_COLOR), 1, 0, width - 2, 1);
    FillWindowPixelRect(WIN_DESCRIPTION, PIXEL_FILL(HGSS_BAG_CELL_NORMAL_COLOR), 0, 1, 1, height - 1);
    FillWindowPixelRect(WIN_DESCRIPTION, PIXEL_FILL(HGSS_BAG_CELL_NORMAL_COLOR), width - 1, 1, 1, height - 1);
}

static void PrintItemDescription(int itemIndex)
{
    const u8 *str;
    if (itemIndex != LIST_CANCEL)
    {
        str = GetItemDescription(GetBagItemId(gBagPosition.pocket, itemIndex));
    }
    else
    {
        // Print 'Cancel' description
        StringCopy(gStringVar1, gBagMenu_ReturnToStrings[gBagPosition.location]);
        StringExpandPlaceholders(gStringVar4, gText_ReturnToVar1);
        str = gStringVar4;
    }
    PrepareHgssBagDescriptionPanel();
    BagMenu_Print(WIN_DESCRIPTION, FONT_NORMAL, str, 3, 1, 0, 0, 0, COLORID_NORMAL);
}

static void BagMenu_PrintCursor(u8 listTaskId, u8 colorIndex)
{
    BagMenu_PrintCursorAtPos(ListMenuGetYCoordForPrintingArrowCursor(listTaskId), colorIndex);
}

static void BagMenu_PrintCursorAtPos(u8 y, u8 colorIndex)
{
    FillWindowPixelRect(WIN_ITEM_LIST, PIXEL_FILL(0), 0, y,
                        HGSS_BAG_LIST_CURSOR_WIDTH, HGSS_BAG_LIST_CURSOR_HEIGHT);

    if (colorIndex == COLORID_GRAY_CURSOR)
        BlitBitmapToWindow(WIN_ITEM_LIST, sHgssBagListCursorGray_Gfx, 0, y,
                           HGSS_BAG_LIST_CURSOR_WIDTH, HGSS_BAG_LIST_CURSOR_HEIGHT);
    else if (colorIndex != COLORID_NONE)
        BlitBitmapToWindow(WIN_ITEM_LIST, sHgssBagListCursor_Gfx, 0, y,
                           HGSS_BAG_LIST_CURSOR_WIDTH, HGSS_BAG_LIST_CURSOR_HEIGHT);
}

#define tTopArrowSpriteId    data[0]
#define tBottomArrowSpriteId data[1]

static void Task_HgssBagScrollArrowPair(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    u8 pocket = gBagPosition.pocket;
    u16 scroll = gBagPosition.scrollPosition[pocket];
    u16 maxScroll = 0;

    if (gBagMenu->numItemStacks[pocket] > gBagMenu->numShownItems[pocket])
        maxScroll = gBagMenu->numItemStacks[pocket] - gBagMenu->numShownItems[pocket];

    gSprites[tTopArrowSpriteId].invisible = (scroll == 0);
    gSprites[tBottomArrowSpriteId].invisible = (scroll >= maxScroll);
}

static void CreatePocketScrollArrowPair(void)
{
    s16 *data;
    u8 topSpriteId;
    u8 bottomSpriteId;

    if (gBagMenu->pocketScrollArrowsTask != TASK_NONE)
        return;

    LoadSpriteSheet(&sHgssBagScrollArrowSpriteSheet);
    LoadSpritePalette(&sHgssBagScrollArrowSpritePalette);

    topSpriteId = CreateSprite(&sHgssBagScrollArrowSpriteTemplate, 108, 24, 0);
    bottomSpriteId = CreateSprite(&sHgssBagScrollArrowSpriteTemplate, 108, 136, 0);
    if (topSpriteId == MAX_SPRITES || bottomSpriteId == MAX_SPRITES)
    {
        if (topSpriteId != MAX_SPRITES)
            DestroySprite(&gSprites[topSpriteId]);
        if (bottomSpriteId != MAX_SPRITES)
            DestroySprite(&gSprites[bottomSpriteId]);
        FreeSpriteTilesByTag(TAG_POCKET_SCROLL_ARROW);
        FreeSpritePaletteByTag(TAG_POCKET_SCROLL_ARROW);
        return;
    }

    SetSpriteOamFlipBits(&gSprites[bottomSpriteId], FALSE, TRUE);
    gBagMenu->pocketScrollArrowsTask = CreateTask(Task_HgssBagScrollArrowPair, 0);
    data = gTasks[gBagMenu->pocketScrollArrowsTask].data;
    tTopArrowSpriteId = topSpriteId;
    tBottomArrowSpriteId = bottomSpriteId;
    Task_HgssBagScrollArrowPair(gBagMenu->pocketScrollArrowsTask);
}

void BagDestroyPocketScrollArrowPair(void)
{
    if (gBagMenu->pocketScrollArrowsTask != TASK_NONE)
    {
        s16 *data = gTasks[gBagMenu->pocketScrollArrowsTask].data;

        DestroySprite(&gSprites[tTopArrowSpriteId]);
        DestroySprite(&gSprites[tBottomArrowSpriteId]);
        DestroyTask(gBagMenu->pocketScrollArrowsTask);
        FreeSpriteTilesByTag(TAG_POCKET_SCROLL_ARROW);
        FreeSpritePaletteByTag(TAG_POCKET_SCROLL_ARROW);
        gBagMenu->pocketScrollArrowsTask = TASK_NONE;
    }
}

#undef tTopArrowSpriteId
#undef tBottomArrowSpriteId

static void FreeBagMenu(void)
{
    Free(sListBuffer2);
    Free(sListBuffer1);
    FreeAllWindowBuffers();
    Free(gBagMenu);
}

void Task_FadeAndCloseBagMenu(u8 taskId)
{
    BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
    gTasks[taskId].func = Task_CloseBagMenu;
}

static void Task_FadeAndCloseBagMenuIfMulch(u8 taskId)
{
    if (gSpecialVar_ItemId == ITEM_GROWTH_MULCH ||
        gSpecialVar_ItemId == ITEM_DAMP_MULCH ||
        gSpecialVar_ItemId == ITEM_STABLE_MULCH ||
        gSpecialVar_ItemId == ITEM_GOOEY_MULCH ||
        gSpecialVar_ItemId == ITEM_RICH_MULCH ||
        gSpecialVar_ItemId == ITEM_SURPRISE_MULCH ||
        gSpecialVar_ItemId == ITEM_BOOST_MULCH ||
        gSpecialVar_ItemId == ITEM_AMAZE_MULCH)
    {
        Task_FadeAndCloseBagMenu(taskId);
        return;
    }
    DisplayDadsAdviceCannotUseItemMessage(taskId, FALSE);
}

static void Task_CloseBagMenu(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    if (!gPaletteFade.active)
    {
        DestroyListMenuTask(tListTaskId, &gBagPosition.scrollPosition[gBagPosition.pocket], &gBagPosition.cursorPosition[gBagPosition.pocket]);

        // If ready for a new screen (e.g. party menu for giving an item) go to that screen
        // Otherwise exit the bag and use callback set up when the bag was first opened
        if (gBagMenu->newScreenCallback != NULL)
            SetMainCallback2(gBagMenu->newScreenCallback);
        else
            SetMainCallback2(gBagPosition.exitCallback);

        BagDestroyPocketScrollArrowPair();
        ResetSpriteData();
        FreeAllSpritePalettes();
        FreeBagMenu();
        DestroyTask(taskId);
    }
}

void UpdatePocketItemList(enum Pocket pocketId)
{
    if (pocketId >= POCKETS_COUNT)
        return; // shouldn't even get here

    struct BagPocket *pocket = &gBagPockets[pocketId];
    switch (pocketId)
    {
    case POCKET_TM_HM:
    case POCKET_BERRIES:
        SortItemsInBag(pocket, SORT_BY_INDEX);
        break;
    default:
        CompactItemsInBagPocket(pocketId);
        break;
    }

    gBagMenu->numItemStacks[pocketId] = 0;

    for (u32 i = 0; i < pocket->capacity && BagPocket_GetSlotData(pocket, i).itemId; i++)
        gBagMenu->numItemStacks[pocketId]++;

    if (!gBagMenu->hideCloseBagText)
        gBagMenu->numItemStacks[pocketId]++;

    if (gBagMenu->numItemStacks[pocketId] > MAX_ITEMS_SHOWN)
        gBagMenu->numShownItems[pocketId] = MAX_ITEMS_SHOWN;
    else
        gBagMenu->numShownItems[pocketId] = gBagMenu->numItemStacks[pocketId];
}

static void UpdatePocketItemLists(void)
{
    u8 i;
    for (i = 0; i < POCKETS_COUNT; i++)
        UpdatePocketItemList(i);
}

void UpdatePocketListPosition(u8 pocketId)
{
    SetCursorWithinListBounds(&gBagPosition.scrollPosition[pocketId], &gBagPosition.cursorPosition[pocketId], gBagMenu->numShownItems[pocketId], gBagMenu->numItemStacks[pocketId]);
}

static void InitPocketListPositions(void)
{
    u8 i;
    for (i = 0; i < POCKETS_COUNT; i++)
        UpdatePocketListPosition(i);
}

static void InitPocketScrollPositions(void)
{
    u8 i;
    for (i = 0; i < POCKETS_COUNT; i++)
        SetCursorScrollWithinListBounds(&gBagPosition.scrollPosition[i], &gBagPosition.cursorPosition[i], gBagMenu->numShownItems[i], gBagMenu->numItemStacks[i], MAX_ITEMS_SHOWN);
}

u8 GetItemListPosition(u8 pocketId)
{
    return gBagPosition.scrollPosition[pocketId] + gBagPosition.cursorPosition[pocketId];
}

static void PrepareHgssBagMessagePanel(u8 windowId)
{
    u8 width = GetWindowAttribute(windowId, WINDOW_WIDTH) * 8;
    u8 height = GetWindowAttribute(windowId, WINDOW_HEIGHT) * 8;

    FillWindowPixelBuffer(windowId, PIXEL_FILL(1));
    FillWindowPixelRect(windowId, PIXEL_FILL(TEXT_COLOR_RED), 1, 0, width - 2, 1);
    FillWindowPixelRect(windowId, PIXEL_FILL(TEXT_COLOR_LIGHT_GRAY), 0, 1, 1, height - 1);
    FillWindowPixelRect(windowId, PIXEL_FILL(TEXT_COLOR_LIGHT_GRAY), width - 1, 1, 1, height - 1);
}

static void Task_HgssBagContinueMessage(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    if (!RunTextPrintersRetIsActive(tMsgWindowId))
        sHgssBagMessageCallback(taskId);
}

void DisplayItemMessage(u8 taskId, u8 fontId, const u8 *str, TaskFunc callback)
{
    s16 *data = gTasks[taskId].data;
    const u8 *text = str;

    tMsgWindowId = AddItemMessageWindow(ITEMWIN_MESSAGE);
    PrepareHgssBagMessagePanel(tMsgWindowId);

    if (str != gStringVar4)
    {
        StringExpandPlaceholders(gStringVar4, str);
        text = gStringVar4;
    }

    gTextFlags.canABSpeedUpPrint = 1;
    sHgssBagMessageCallback = callback;
    BagMenu_Print(tMsgWindowId, fontId, text, 4, 1, 0, 0,
                  GetPlayerTextSpeedDelay(), COLORID_POCKET_NAME);
    CopyWindowToVram(tMsgWindowId, COPYWIN_GFX);
    ScheduleBgCopyTilemapToVram(1);
    gTasks[taskId].func = Task_HgssBagContinueMessage;
}

void CloseItemMessage(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    u16 *scrollPos = &gBagPosition.scrollPosition[gBagPosition.pocket];
    u16 *cursorPos = &gBagPosition.cursorPosition[gBagPosition.pocket];
    RemoveItemMessageWindow(ITEMWIN_MESSAGE);
    DestroyListMenuTask(tListTaskId, scrollPos, cursorPos);
    UpdatePocketItemList(gBagPosition.pocket);
    UpdatePocketListPosition(gBagPosition.pocket);
    LoadBagItemListBuffers(gBagPosition.pocket);
    tListTaskId = ListMenuInit(&gMultiuseListMenuTemplate, *scrollPos, *cursorPos);
    ScheduleBgCopyTilemapToVram(0);
    ReturnToItemList(taskId);
}

static void AddItemQuantityWindow(u8 windowType)
{
    PrintItemQuantity(BagMenu_AddWindow(windowType), 1);
}

static void DrawHgssBagQuantityPanelFrame(u8 windowId)
{
    u8 width = GetWindowAttribute(windowId, WINDOW_WIDTH) * 8;
    u8 height = GetWindowAttribute(windowId, WINDOW_HEIGHT) * 8;

    FillWindowPixelRect(windowId, PIXEL_FILL(TEXT_COLOR_RED), 1, 0, width - 2, 1);
    FillWindowPixelRect(windowId, PIXEL_FILL(TEXT_COLOR_RED), 1, height - 1, width - 2, 1);
    FillWindowPixelRect(windowId, PIXEL_FILL(TEXT_COLOR_RED), 0, 1, 1, height - 2);
    FillWindowPixelRect(windowId, PIXEL_FILL(TEXT_COLOR_RED), width - 1, 1, 1, height - 2);
}

static void PrepareHgssBagQuantityPanel(u8 windowId)
{
    FillWindowPixelBuffer(windowId, PIXEL_FILL(1));
    DrawHgssBagQuantityPanelFrame(windowId);
}

static void PrintItemQuantity(u8 windowId, s16 quantity)
{
    u8 width = GetWindowAttribute(windowId, WINDOW_WIDTH) * 8;

    PrepareHgssBagQuantityPanel(windowId);
    ConvertIntToDecimalStringN(gStringVar1, quantity, STR_CONV_MODE_LEADING_ZEROS, MAX_ITEM_DIGITS);
    StringExpandPlaceholders(gStringVar4, gText_xVar1);
    BagMenu_Print(windowId, FONT_SMALL, gStringVar4,
                  1 + GetStringCenterAlignXOffset(FONT_SMALL, gStringVar4, width - 2), 2,
                  0, 0, TEXT_SKIP_DRAW, COLORID_POCKET_NAME);
    DrawHgssBagQuantityPanelFrame(windowId);
    CopyWindowToVram(windowId, COPYWIN_GFX);
}

// Prints the quantity of items to be sold and the amount that would be earned
static void PrintItemSoldAmount(int windowId, int numSold, int moneyEarned)
{
    PrepareHgssBagQuantityPanel(windowId);
    FillWindowPixelRect(windowId, PIXEL_FILL(TEXT_COLOR_RED), 24, 2, 1,
                        GetWindowAttribute(windowId, WINDOW_HEIGHT) * 8 - 4);
    ConvertIntToDecimalStringN(gStringVar1, numSold, STR_CONV_MODE_LEADING_ZEROS, MAX_ITEM_DIGITS);
    StringExpandPlaceholders(gStringVar4, gText_xVar1);
    BagMenu_Print(windowId, FONT_SMALL, gStringVar4,
                  1 + GetStringCenterAlignXOffset(FONT_SMALL, gStringVar4, 23), 2,
                  0, 0, TEXT_SKIP_DRAW, COLORID_POCKET_NAME);
    PrintMoneyAmount(windowId, CalculateMoneyTextHorizontalPosition(moneyEarned), 1, moneyEarned, 0);
    FillWindowPixelRect(windowId, PIXEL_FILL(TEXT_COLOR_RED), 24, 2, 1,
                        GetWindowAttribute(windowId, WINDOW_HEIGHT) * 8 - 4);
    DrawHgssBagQuantityPanelFrame(windowId);
    CopyWindowToVram(windowId, COPYWIN_GFX);
}

static void Task_BagMenu_HandleInput(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    u16 *scrollPos = &gBagPosition.scrollPosition[gBagPosition.pocket];
    u16 *cursorPos = &gBagPosition.cursorPosition[gBagPosition.pocket];
    s32 listPosition;

    ChangeBgY(3, 128, BG_COORD_ADD);

    if (MenuHelpers_ShouldWaitForLinkRecv() != TRUE && !gPaletteFade.active)
    {
        switch (GetSwitchBagPocketDirection())
        {
        case SWITCH_POCKET_LEFT:
            SwitchBagPocket(taskId, MENU_CURSOR_DELTA_LEFT, FALSE);
            return;
        case SWITCH_POCKET_RIGHT:
            SwitchBagPocket(taskId, MENU_CURSOR_DELTA_RIGHT, FALSE);
            return;
        default:
            if (JOY_NEW(SELECT_BUTTON))
            {
                if (CanSwapItems() == TRUE)
                {
                    ListMenuGetScrollAndRow(tListTaskId, scrollPos, cursorPos);
                    if ((*scrollPos + *cursorPos) != gBagMenu->numItemStacks[gBagPosition.pocket] - 1)
                    {
                        PlaySE(SE_SELECT);
                        StartItemSwap(taskId);
                    }
                }
                return;
            }
            else if (JOY_NEW(START_BUTTON))
            {
                if ((gBagMenu->numItemStacks[gBagPosition.pocket] - 1) <= 1) //can't sort with 0 or 1 item in bag
                {
                    static const u8 sText_NothingToSort[] = _("There's nothing to sort!");
                    PlaySE(SE_FAILURE);
                    DisplayItemMessage(taskId, 1, sText_NothingToSort, HandleErrorMessage);
                    break;
                }
                else
                {
                    struct ItemSlot tempItem;
                    data[1] = GetItemListPosition(gBagPosition.pocket);
                    tempItem = GetBagItemIdAndQuantity(gBagPosition.pocket, data[1]);
                    data[2] = tempItem.quantity;
                    if (gBagPosition.cursorPosition[gBagPosition.pocket] == gBagMenu->numItemStacks[gBagPosition.pocket])
                        break;
                    else
                        gSpecialVar_ItemId = tempItem.itemId;

                    PlaySE(SE_SELECT);
                    BagDestroyPocketScrollArrowPair();
                    BagMenu_PrintCursor(tListTaskId, COLORID_GRAY_CURSOR);
                    ListMenuGetScrollAndRow(data[0], scrollPos, cursorPos);
                    gTasks[taskId].func = Task_LoadBagSortOptions;
                    return;
                }
            }
            break;
        }

        listPosition = ListMenu_ProcessInput(tListTaskId);
        ListMenuGetScrollAndRow(tListTaskId, scrollPos, cursorPos);
        switch (listPosition)
        {
        case LIST_NOTHING_CHOSEN:
            break;
        case LIST_CANCEL:
            if (gBagPosition.location == ITEMMENULOCATION_BERRY_BLENDER_CRUSH)
            {
                PlaySE(SE_FAILURE);
                break;
            }
            PlaySE(SE_SELECT);
            gSpecialVar_ItemId = ITEM_NONE;
            gTasks[taskId].func = Task_FadeAndCloseBagMenu;
            break;
        default: // A_BUTTON
            {
                struct ItemSlot itemSlot = GetBagItemIdAndQuantity(gBagPosition.pocket, listPosition);
                PlaySE(SE_SELECT);
                BagDestroyPocketScrollArrowPair();
                BagMenu_PrintCursor(tListTaskId, COLORID_GRAY_CURSOR);
                tListPosition = listPosition;
                gSpecialVar_ItemId = itemSlot.itemId;
                tQuantity = itemSlot.quantity;
                sContextMenuFuncs[gBagPosition.location](taskId);
            }
            break;
        }
    }
}

static void ReturnToItemList(u8 taskId)
{
    CreatePocketScrollArrowPair();
    ClearWindowTilemap(WIN_TMHM_INFO_ICONS);
    ClearWindowTilemap(WIN_TMHM_INFO);
    PutWindowTilemap(WIN_DESCRIPTION);
    ScheduleBgCopyTilemapToVram(0);
    gTasks[taskId].func = Task_BagMenu_HandleInput;
}

static u8 GetSwitchBagPocketDirection(void)
{
    u8 LRKeys;
    if (gBagMenu->pocketSwitchDisabled)
        return SWITCH_POCKET_NONE;
    LRKeys = GetLRKeysPressed();
    if (JOY_NEW(DPAD_LEFT) || LRKeys == MENU_L_PRESSED)
    {
        PlaySE(SE_SELECT);
        return SWITCH_POCKET_LEFT;
    }
    if (JOY_NEW(DPAD_RIGHT) || LRKeys == MENU_R_PRESSED)
    {
        PlaySE(SE_SELECT);
        return SWITCH_POCKET_RIGHT;
    }
    return SWITCH_POCKET_NONE;
}

static void ChangeBagPocketId(u8 *bagPocketId, s8 deltaBagPocketId)
{
    if (deltaBagPocketId == MENU_CURSOR_DELTA_RIGHT && *bagPocketId == POCKETS_COUNT - 1)
        *bagPocketId = 0;
    else if (deltaBagPocketId == MENU_CURSOR_DELTA_LEFT && *bagPocketId == 0)
        *bagPocketId = POCKETS_COUNT - 1;
    else
        *bagPocketId += deltaBagPocketId;

    if (IsVictoryCatch() && *bagPocketId == POCKET_POKE_BALLS)
        *bagPocketId += 1;

}

static void SwitchBagPocket(u8 taskId, s16 deltaBagPocketId, bool16 skipEraseList)
{
    s16 *data = gTasks[taskId].data;
    u8 newPocket;

    ChangeBgY(3, 128, BG_COORD_ADD);

    tPocketSwitchState = 0;
    tPocketSwitchTimer = 0;
    tPocketSwitchDir = deltaBagPocketId;
    if (!skipEraseList)
    {
        ClearWindowTilemap(WIN_ITEM_LIST);
        ClearWindowTilemap(WIN_DESCRIPTION);
        DestroyListMenuTask(tListTaskId, &gBagPosition.scrollPosition[gBagPosition.pocket], &gBagPosition.cursorPosition[gBagPosition.pocket]);
        ScheduleBgCopyTilemapToVram(0);
        gSprites[gBagMenu->spriteIds[ITEMMENUSPRITE_ITEM + (gBagMenu->itemIconSlot ^ 1)]].invisible = TRUE;
        BagDestroyPocketScrollArrowPair();
    }
    newPocket = gBagPosition.pocket;
    ChangeBagPocketId(&newPocket, deltaBagPocketId);
    if (deltaBagPocketId == MENU_CURSOR_DELTA_RIGHT)
    {
        PrintPocketNames(gPocketNamesStringsTable[gBagPosition.pocket], gPocketNamesStringsTable[newPocket]);
        CopyPocketNameToWindow(0);
    }
    else
    {
        PrintPocketNames(gPocketNamesStringsTable[newPocket], gPocketNamesStringsTable[gBagPosition.pocket]);
        CopyPocketNameToWindow(8);
    }
    DrawPocketIndicatorIcon(gBagPosition.pocket, FALSE);
    DrawPocketIndicatorIcon(newPocket, TRUE);
    FillBgTilemapBufferRect_Palette0(2, 11, 14, 2, 15, 16);
    ScheduleBgCopyTilemapToVram(2);
    SetBagVisualPocketId(newPocket, TRUE);
    SetTaskFuncWithFollowupFunc(taskId, Task_SwitchBagPocket, gTasks[taskId].func);
}

static void Task_SwitchBagPocket(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    if (!MenuHelpers_IsLinkActive() && !IsWallysBag())
    {
        switch (GetSwitchBagPocketDirection())
        {
        case SWITCH_POCKET_LEFT:
            ChangeBagPocketId(&gBagPosition.pocket, tPocketSwitchDir);
            SwitchTaskToFollowupFunc(taskId);
            SwitchBagPocket(taskId, MENU_CURSOR_DELTA_LEFT, TRUE);
            return;
        case SWITCH_POCKET_RIGHT:
            ChangeBagPocketId(&gBagPosition.pocket, tPocketSwitchDir);
            SwitchTaskToFollowupFunc(taskId);
            SwitchBagPocket(taskId, MENU_CURSOR_DELTA_RIGHT, TRUE);
            return;
        }
    }
    switch (tPocketSwitchState)
    {
    case 0:
        DrawItemListBgRow(tPocketSwitchTimer);
        if (!(++tPocketSwitchTimer & 1))
        {
            if (tPocketSwitchDir == MENU_CURSOR_DELTA_RIGHT)
                CopyPocketNameToWindow((u8)(tPocketSwitchTimer >> 1));
            else
                CopyPocketNameToWindow((u8)(8 - (tPocketSwitchTimer >> 1)));
        }
        if (tPocketSwitchTimer == 16)
            tPocketSwitchState++;
        break;
    case 1:
        ChangeBagPocketId(&gBagPosition.pocket, tPocketSwitchDir);
        LoadBagItemListBuffers(gBagPosition.pocket);
        tListTaskId = ListMenuInit(&gMultiuseListMenuTemplate, gBagPosition.scrollPosition[gBagPosition.pocket], gBagPosition.cursorPosition[gBagPosition.pocket]);
        PutWindowTilemap(WIN_DESCRIPTION);
        PutWindowTilemap(WIN_POCKET_NAME);
        ScheduleBgCopyTilemapToVram(0);
        CreatePocketScrollArrowPair();
        SwitchTaskToFollowupFunc(taskId);
    }
}

// The background of the item list is a lighter color than the surrounding menu
// When the pocket is switched this lighter background is redrawn row by row
static void DrawItemListBgRow(u8 y)
{
    FillBgTilemapBufferRect_Palette0(2, 17, 14, y + 2, 15, 1);
    ScheduleBgCopyTilemapToVram(2);
}

static bool8 CanSwapItems(void)
{
    // Swaps can only be done from the field or in battle (as opposed to while selling items, for example)
    if (gBagPosition.location == ITEMMENULOCATION_FIELD
     || gBagPosition.location == ITEMMENULOCATION_BATTLE)
    {
        // TMHMs and berries are numbered, and so may not be swapped
        if (gBagPosition.pocket != POCKET_TM_HM
         && gBagPosition.pocket != POCKET_BERRIES)
            return TRUE;
    }
    return FALSE;
}

static void StartItemSwap(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    ListMenuSetTemplateField(tListTaskId, LISTFIELD_CURSORKIND, CURSOR_INVISIBLE);
    tListPosition = gBagPosition.scrollPosition[gBagPosition.pocket] + gBagPosition.cursorPosition[gBagPosition.pocket];
    gBagMenu->toSwapPos = tListPosition;
    CopyItemName(GetBagItemId(gBagPosition.pocket, tListPosition), gStringVar1);
    StringExpandPlaceholders(gStringVar4, gText_MoveVar1Where);
    PrepareHgssBagDescriptionPanel();
    BagMenu_Print(WIN_DESCRIPTION, FONT_NORMAL, gStringVar4, 3, 1, 0, 0, 0, COLORID_NORMAL);
    BagMenu_PrintCursor(tListTaskId, COLORID_GRAY_CURSOR);
    gTasks[taskId].func = Task_HandleSwappingItemsInput;
}

static void Task_HandleSwappingItemsInput(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    ChangeBgY(3, 128, BG_COORD_ADD);

    if (MenuHelpers_ShouldWaitForLinkRecv() != TRUE)
    {
        if (JOY_NEW(SELECT_BUTTON))
        {
            PlaySE(SE_SELECT);
            ListMenuGetScrollAndRow(tListTaskId, &gBagPosition.scrollPosition[gBagPosition.pocket], &gBagPosition.cursorPosition[gBagPosition.pocket]);
            DoItemSwap(taskId);
        }
        else
        {
            s32 input = ListMenu_ProcessInput(tListTaskId);
            ListMenuGetScrollAndRow(tListTaskId, &gBagPosition.scrollPosition[gBagPosition.pocket], &gBagPosition.cursorPosition[gBagPosition.pocket]);
            // The HGSS source marker and active cell border replace Emerald's horizontal swap line.
            switch (input)
            {
            case LIST_NOTHING_CHOSEN:
                break;
            case LIST_CANCEL:
                PlaySE(SE_SELECT);
                if (JOY_NEW(A_BUTTON))
                    DoItemSwap(taskId);
                else
                    CancelItemSwap(taskId);
                break;
            default:
                PlaySE(SE_SELECT);
                DoItemSwap(taskId);
                break;
            }
        }
    }
}

static void DoItemSwap(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    u16 *scrollPos = &gBagPosition.scrollPosition[gBagPosition.pocket];
    u16 *cursorPos = &gBagPosition.cursorPosition[gBagPosition.pocket];
    u16 realPos = (*scrollPos + *cursorPos);

    if (tListPosition == realPos || tListPosition == realPos - 1)
    {
        // Position is the same as the original, cancel
        CancelItemSwap(taskId);
    }
    else
    {
        MoveItemSlotInPocket(gBagPosition.pocket, tListPosition, realPos);
        gBagMenu->toSwapPos = NOT_SWAPPING;
        DestroyListMenuTask(tListTaskId, scrollPos, cursorPos);
        if (tListPosition < realPos)
            gBagPosition.cursorPosition[gBagPosition.pocket]--;
        LoadBagItemListBuffers(gBagPosition.pocket);
        tListTaskId = ListMenuInit(&gMultiuseListMenuTemplate, *scrollPos, *cursorPos);
        gTasks[taskId].func = Task_BagMenu_HandleInput;
    }
}

static void CancelItemSwap(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    u16 *scrollPos = &gBagPosition.scrollPosition[gBagPosition.pocket];
    u16 *cursorPos = &gBagPosition.cursorPosition[gBagPosition.pocket];

    gBagMenu->toSwapPos = NOT_SWAPPING;
    DestroyListMenuTask(tListTaskId, scrollPos, cursorPos);
    if (tListPosition < *scrollPos + *cursorPos)
        gBagPosition.cursorPosition[gBagPosition.pocket]--;
    LoadBagItemListBuffers(gBagPosition.pocket);
    tListTaskId = ListMenuInit(&gMultiuseListMenuTemplate, *scrollPos, *cursorPos);
    gTasks[taskId].func = Task_BagMenu_HandleInput;
}

static void OpenContextMenu(u8 taskId)
{
    switch (gBagPosition.location)
    {
    case ITEMMENULOCATION_BATTLE:
    case ITEMMENULOCATION_WALLY:
    case ITEMMENULOCATION_RAIDEND:
        if (GetItemBattleUsage(gSpecialVar_ItemId))
        {
            gBagMenu->contextMenuItemsPtr = sContextMenuItems_BattleUse;
            gBagMenu->contextMenuNumItems = ARRAY_COUNT(sContextMenuItems_BattleUse);
        }
        else
        {
            gBagMenu->contextMenuItemsPtr = sContextMenuItems_Cancel;
            gBagMenu->contextMenuNumItems = ARRAY_COUNT(sContextMenuItems_Cancel);
        }
        break;
    case ITEMMENULOCATION_BERRY_BLENDER_CRUSH:
        gBagMenu->contextMenuItemsPtr = sContextMenuItems_BerryBlenderCrush;
        gBagMenu->contextMenuNumItems = ARRAY_COUNT(sContextMenuItems_BerryBlenderCrush);
        break;
    case ITEMMENULOCATION_APPRENTICE:
        if (!GetItemImportance(gSpecialVar_ItemId) && gSpecialVar_ItemId != ITEM_ENIGMA_BERRY_E_READER)
        {
            gBagMenu->contextMenuItemsPtr = sContextMenuItems_Apprentice;
            gBagMenu->contextMenuNumItems = ARRAY_COUNT(sContextMenuItems_Apprentice);
        }
        else
        {
            gBagMenu->contextMenuItemsPtr = sContextMenuItems_Cancel;
            gBagMenu->contextMenuNumItems = ARRAY_COUNT(sContextMenuItems_Cancel);
        }
        break;
    case ITEMMENULOCATION_FAVOR_LADY:
        if (!GetItemImportance(gSpecialVar_ItemId) && gSpecialVar_ItemId != ITEM_ENIGMA_BERRY_E_READER)
        {
            gBagMenu->contextMenuItemsPtr = sContextMenuItems_FavorLady;
            gBagMenu->contextMenuNumItems = ARRAY_COUNT(sContextMenuItems_FavorLady);
        }
        else
        {
            gBagMenu->contextMenuItemsPtr = sContextMenuItems_Cancel;
            gBagMenu->contextMenuNumItems = ARRAY_COUNT(sContextMenuItems_Cancel);
        }
        break;
    case ITEMMENULOCATION_QUIZ_LADY:
        if (!GetItemImportance(gSpecialVar_ItemId) && gSpecialVar_ItemId != ITEM_ENIGMA_BERRY_E_READER)
        {
            gBagMenu->contextMenuItemsPtr = sContextMenuItems_QuizLady;
            gBagMenu->contextMenuNumItems = ARRAY_COUNT(sContextMenuItems_QuizLady);
        }
        else
        {
            gBagMenu->contextMenuItemsPtr = sContextMenuItems_Cancel;
            gBagMenu->contextMenuNumItems = ARRAY_COUNT(sContextMenuItems_Cancel);
        }
        break;
    case ITEMMENULOCATION_PARTY:
    case ITEMMENULOCATION_SHOP:
    case ITEMMENULOCATION_BERRY_TREE:
    case ITEMMENULOCATION_ITEMPC:
    case ITEMMENULOCATION_BERRY_TREE_MULCH:
    default:
        if (MenuHelpers_IsLinkActive() == TRUE || InUnionRoom() == TRUE)
        {
            if (gBagPosition.pocket == POCKET_KEY_ITEMS || !IsHoldingItemAllowed(gSpecialVar_ItemId))
            {
                gBagMenu->contextMenuItemsPtr = sContextMenuItems_Cancel;
                gBagMenu->contextMenuNumItems = ARRAY_COUNT(sContextMenuItems_Cancel);
            }
            else
            {
                gBagMenu->contextMenuItemsPtr = sContextMenuItems_Give;
                gBagMenu->contextMenuNumItems = ARRAY_COUNT(sContextMenuItems_Give);
            }
        }
        else
        {
            switch (gBagPosition.pocket)
            {
            case POCKET_ITEMS:
                gBagMenu->contextMenuItemsPtr = gBagMenu->contextMenuItemsBuffer;
                gBagMenu->contextMenuNumItems = ARRAY_COUNT(sContextMenuItems_ItemsPocket);
                memcpy(&gBagMenu->contextMenuItemsBuffer, &sContextMenuItems_ItemsPocket, sizeof(sContextMenuItems_ItemsPocket));
                if (ItemIsMail(gSpecialVar_ItemId) == TRUE)
                    gBagMenu->contextMenuItemsBuffer[0] = ACTION_CHECK;
                break;
            case POCKET_KEY_ITEMS:
                gBagMenu->contextMenuItemsPtr = gBagMenu->contextMenuItemsBuffer;
                gBagMenu->contextMenuNumItems = ARRAY_COUNT(sContextMenuItems_KeyItemsPocket);
                memcpy(&gBagMenu->contextMenuItemsBuffer, &sContextMenuItems_KeyItemsPocket, sizeof(sContextMenuItems_KeyItemsPocket));
                if (gSaveBlock1Ptr->registeredItem == gSpecialVar_ItemId)
                    gBagMenu->contextMenuItemsBuffer[1] = ACTION_DESELECT;
                if (gSpecialVar_ItemId == ITEM_MACH_BIKE || gSpecialVar_ItemId == ITEM_ACRO_BIKE || gSpecialVar_ItemId == ITEM_BICYCLE)
                {
                    if (TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_MACH_BIKE | PLAYER_AVATAR_FLAG_ACRO_BIKE))
                        gBagMenu->contextMenuItemsBuffer[0] = ACTION_WALK;
                }
                break;
            case POCKET_POKE_BALLS:
                gBagMenu->contextMenuItemsPtr = sContextMenuItems_BallsPocket;
                gBagMenu->contextMenuNumItems = ARRAY_COUNT(sContextMenuItems_BallsPocket);
                break;
            case POCKET_TM_HM:
                gBagMenu->contextMenuItemsPtr = sContextMenuItems_TmHmPocket;
                gBagMenu->contextMenuNumItems = ARRAY_COUNT(sContextMenuItems_TmHmPocket);
                break;
            case POCKET_BERRIES:
                gBagMenu->contextMenuItemsPtr = sContextMenuItems_BerriesPocket;
                gBagMenu->contextMenuNumItems = ARRAY_COUNT(sContextMenuItems_BerriesPocket);
                break;
            }
        }
    }
    if (gBagPosition.pocket == POCKET_TM_HM)
    {
        ClearWindowTilemap(WIN_DESCRIPTION);
        PrintTMHMMoveData(gSpecialVar_ItemId);
        PutWindowTilemap(WIN_TMHM_INFO_ICONS);
        PutWindowTilemap(WIN_TMHM_INFO);
        ScheduleBgCopyTilemapToVram(0);
    }
    else
    {
        u8 *end = CopyItemName(gSpecialVar_ItemId, gStringVar1);
        WrapFontIdToFit(gStringVar1, end, FONT_NORMAL, WindowWidthPx(WIN_DESCRIPTION) - 10 - 6);
        StringExpandPlaceholders(gStringVar4, gText_Var1IsSelected);
        PrepareHgssBagDescriptionPanel();
        BagMenu_Print(WIN_DESCRIPTION, FONT_NORMAL, gStringVar4, 3, 1, 0, 0, 0, COLORID_NORMAL);
    }
    if (gBagMenu->contextMenuNumItems == 1)
        PrintContextMenuItems(BagMenu_AddWindow(ITEMWIN_1x1));
    else if (gBagMenu->contextMenuNumItems == 2)
        PrintContextMenuItems(BagMenu_AddWindow(ITEMWIN_1x2));
    else if (gBagMenu->contextMenuNumItems == 4)
        PrintContextMenuItemGrid(BagMenu_AddWindow(ITEMWIN_2x2), 2, 2);
    else
        PrintContextMenuItemGrid(BagMenu_AddWindow(ITEMWIN_2x3), 2, 3);
}

static void DrawHgssBagContextMenuAccent(u8 windowId)
{
    u8 width = GetWindowAttribute(windowId, WINDOW_WIDTH) * 8;
    u8 height = GetWindowAttribute(windowId, WINDOW_HEIGHT) * 8;

    // Match the softened corners used by the HGSS-style item cells.
    FillWindowPixelRect(windowId, PIXEL_FILL(TEXT_COLOR_RED), 1, 0, width - 2, 1);
    FillWindowPixelRect(windowId, PIXEL_FILL(TEXT_COLOR_RED), 1, height - 1, width - 2, 1);
    FillWindowPixelRect(windowId, PIXEL_FILL(TEXT_COLOR_RED), 0, 1, 1, height - 2);
    FillWindowPixelRect(windowId, PIXEL_FILL(TEXT_COLOR_RED), width - 1, 1, 1, height - 2);
}

static void DrawHgssBagContextMenuCursor(u8 windowId, u8 cursorPos, u8 columns, u8 rows)
{
    for (u8 row = 0; row < rows; row++)
    {
        for (u8 column = 0; column < columns; column++)
        {
            u8 clearX = column * 56;
            u8 clearWidth = 8;
            u8 clearHeight = (row == rows - 1) ? 14 : 15;

            // Preserve the chamfered panel edges while erasing the old cursor.
            if (column == 0)
            {
                clearX++;
                clearWidth--;
            }
            FillWindowPixelRect(windowId, PIXEL_FILL(1),
                                clearX, 1 + row * 16,
                                clearWidth, clearHeight);
        }
    }

    // The stock menu engine redraws its Emerald arrow first; restore the HGSS frame over any edge pixels it leaves behind.
    DrawHgssBagContextMenuAccent(windowId);

    {
        u8 cursorX = (cursorPos % columns) * 56 + 2;
        u8 cursorY = 3 + (cursorPos / columns) * 16;

        FillWindowPixelRect(windowId, PIXEL_FILL(TEXT_COLOR_RED), cursorX + 1, cursorY, 2, 1);
        FillWindowPixelRect(windowId, PIXEL_FILL(TEXT_COLOR_RED), cursorX, cursorY + 1, 4, 9);
        FillWindowPixelRect(windowId, PIXEL_FILL(TEXT_COLOR_RED), cursorX + 1, cursorY + 10, 2, 1);
    }
    CopyWindowToVram(windowId, COPYWIN_GFX);
}

static void PrintContextMenuItems(u8 windowId)
{
    DrawHgssBagContextMenuAccent(windowId);
    for (u8 i = 0; i < gBagMenu->contextMenuNumItems; i++)
    {
        u8 actionId = gBagMenu->contextMenuItemsPtr[i];
        if (actionId != ACTION_DUMMY)
            BagMenu_Print(windowId, FONT_SMALL, sItemMenuActions[actionId].text, 8, 2 + i * 16, 0, 0, TEXT_SKIP_DRAW, COLORID_POCKET_NAME);
    }
    InitMenuInUpperLeftCornerNormal(windowId, gBagMenu->contextMenuNumItems, 0);
    DrawHgssBagContextMenuCursor(windowId, 0, 1, gBagMenu->contextMenuNumItems);
}

static void PrintContextMenuItemGrid(u8 windowId, u8 columns, u8 rows)
{
    DrawHgssBagContextMenuAccent(windowId);
    for (u8 row = 0; row < rows; row++)
    {
        for (u8 column = 0; column < columns; column++)
        {
            u8 index = row * columns + column;
            u8 actionId = gBagMenu->contextMenuItemsPtr[index];
            if (actionId != ACTION_DUMMY)
                BagMenu_Print(windowId, FONT_SMALL, sItemMenuActions[actionId].text, 8 + column * 56, 2 + row * 16, 0, 0, TEXT_SKIP_DRAW, COLORID_POCKET_NAME);
        }
    }
    InitMenuActionGrid(windowId, 56, columns, rows, 0);
    DrawHgssBagContextMenuCursor(windowId, 0, columns, rows);
}

static void Task_ItemContext_Normal(u8 taskId)
{
    OpenContextMenu(taskId);

    // Context menu width is never greater than 2 columns, so if
    // there are more than 2 items then there are multiple rows
    if (gBagMenu->contextMenuNumItems <= 2)
        gTasks[taskId].func = Task_ItemContext_SingleRow;
    else
        gTasks[taskId].func = Task_ItemContext_MultipleRows;
}

static void Task_ItemContext_SingleRow(u8 taskId)
{
    ChangeBgY(3, 128, BG_COORD_ADD);

    if (MenuHelpers_ShouldWaitForLinkRecv() != TRUE)
    {
        s8 selection = Menu_ProcessInputNoWrap();
        switch (selection)
        {
        case MENU_NOTHING_CHOSEN:
            DrawHgssBagContextMenuCursor(
                gBagMenu->contextMenuNumItems == 1 ? gBagMenu->windowIds[ITEMWIN_1x1] : gBagMenu->windowIds[ITEMWIN_1x2],
                Menu_GetCursorPos(), 1, gBagMenu->contextMenuNumItems);
            break;
        case MENU_B_PRESSED:
            PlaySE(SE_SELECT);
            sItemMenuActions[ACTION_CANCEL].func.void_u8(taskId);
            break;
        default:
            PlaySE(SE_SELECT);
            sItemMenuActions[gBagMenu->contextMenuItemsPtr[selection]].func.void_u8(taskId);
            break;
        }
    }
}

static void Task_ItemContext_MultipleRows(u8 taskId)
{
    ChangeBgY(3, 128, BG_COORD_ADD);

    if (MenuHelpers_ShouldWaitForLinkRecv() != TRUE)
    {
        s8 cursorPos = Menu_GetCursorPos();
        if (JOY_NEW(DPAD_UP))
        {
            if (cursorPos > 0 && IsValidContextMenuPos(cursorPos - 2))
            {
                PlaySE(SE_SELECT);
                ChangeMenuGridCursorPosition(MENU_CURSOR_DELTA_NONE, MENU_CURSOR_DELTA_UP);
                DrawHgssBagContextMenuCursor(
                    gBagMenu->contextMenuNumItems == 4 ? gBagMenu->windowIds[ITEMWIN_2x2] : gBagMenu->windowIds[ITEMWIN_2x3],
                    Menu_GetCursorPos(), 2, gBagMenu->contextMenuNumItems / 2);
            }
        }
        else if (JOY_NEW(DPAD_DOWN))
        {
            if (cursorPos < (gBagMenu->contextMenuNumItems - 2) && IsValidContextMenuPos(cursorPos + 2))
            {
                PlaySE(SE_SELECT);
                ChangeMenuGridCursorPosition(MENU_CURSOR_DELTA_NONE, MENU_CURSOR_DELTA_DOWN);
                DrawHgssBagContextMenuCursor(
                    gBagMenu->contextMenuNumItems == 4 ? gBagMenu->windowIds[ITEMWIN_2x2] : gBagMenu->windowIds[ITEMWIN_2x3],
                    Menu_GetCursorPos(), 2, gBagMenu->contextMenuNumItems / 2);
            }
        }
        else if (JOY_NEW(DPAD_LEFT) || GetLRKeysPressed() == MENU_L_PRESSED)
        {
            if ((cursorPos & 1) && IsValidContextMenuPos(cursorPos - 1))
            {
                PlaySE(SE_SELECT);
                ChangeMenuGridCursorPosition(MENU_CURSOR_DELTA_LEFT, MENU_CURSOR_DELTA_NONE);
                DrawHgssBagContextMenuCursor(
                    gBagMenu->contextMenuNumItems == 4 ? gBagMenu->windowIds[ITEMWIN_2x2] : gBagMenu->windowIds[ITEMWIN_2x3],
                    Menu_GetCursorPos(), 2, gBagMenu->contextMenuNumItems / 2);
            }
        }
        else if (JOY_NEW(DPAD_RIGHT) || GetLRKeysPressed() == MENU_R_PRESSED)
        {
            if (!(cursorPos & 1) && IsValidContextMenuPos(cursorPos + 1))
            {
                PlaySE(SE_SELECT);
                ChangeMenuGridCursorPosition(MENU_CURSOR_DELTA_RIGHT, MENU_CURSOR_DELTA_NONE);
                DrawHgssBagContextMenuCursor(
                    gBagMenu->contextMenuNumItems == 4 ? gBagMenu->windowIds[ITEMWIN_2x2] : gBagMenu->windowIds[ITEMWIN_2x3],
                    Menu_GetCursorPos(), 2, gBagMenu->contextMenuNumItems / 2);
            }
        }
        else if (JOY_NEW(A_BUTTON))
        {
            PlaySE(SE_SELECT);
            sItemMenuActions[gBagMenu->contextMenuItemsPtr[cursorPos]].func.void_u8(taskId);
        }
        else if (JOY_NEW(B_BUTTON))
        {
            PlaySE(SE_SELECT);
            sItemMenuActions[ACTION_CANCEL].func.void_u8(taskId);
        }
    }
}

static bool8 IsValidContextMenuPos(s8 cursorPos)
{
    if (cursorPos < 0)
        return FALSE;
    if (cursorPos > gBagMenu->contextMenuNumItems)
        return FALSE;
    if (gBagMenu->contextMenuItemsPtr[cursorPos] == ACTION_DUMMY)
        return FALSE;
    return TRUE;
}

static void RemoveContextWindow(void)
{
    if (gBagMenu->contextMenuNumItems == 1)
        BagMenu_RemoveWindow(ITEMWIN_1x1);
    else if (gBagMenu->contextMenuNumItems == 2)
        BagMenu_RemoveWindow(ITEMWIN_1x2);
    else if (gBagMenu->contextMenuNumItems == 4)
        BagMenu_RemoveWindow(ITEMWIN_2x2);
    else
        BagMenu_RemoveWindow(ITEMWIN_2x3);
}

static void ItemMenu_UseOutOfBattle(u8 taskId)
{
    if (GetItemFieldFunc(gSpecialVar_ItemId))
    {
        RemoveContextWindow();
        if (CalculatePlayerPartyCount() == 0 && GetItemType(gSpecialVar_ItemId) == ITEM_USE_PARTY_MENU)
        {
            PrintThereIsNoPokemon(taskId);
        }
        else
        {
            PrepareHgssBagDescriptionPanel();
            ScheduleBgCopyTilemapToVram(0);
            if (gBagPosition.pocket != POCKET_BERRIES)
                GetItemFieldFunc(gSpecialVar_ItemId)(taskId);
            else
                ItemUseOutOfBattle_Berry(taskId);
        }
    }
}

static void ItemMenu_Toss(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    RemoveContextWindow();
    tItemCount = 1;
    if (tQuantity == 1)
    {
        AskTossItems(taskId);
    }
    else
    {
        u8 *end = CopyItemNameHandlePlural(gSpecialVar_ItemId, gStringVar1, 2);
        WrapFontIdToFit(gStringVar1, end, FONT_NORMAL, WindowWidthPx(WIN_DESCRIPTION) - 10 - 6);
        StringExpandPlaceholders(gStringVar4, gText_TossHowManyVar1s);
        PrepareHgssBagDescriptionPanel();
        BagMenu_Print(WIN_DESCRIPTION, FONT_NORMAL, gStringVar4, 3, 1, 0, 0, 0, COLORID_NORMAL);
        AddItemQuantityWindow(ITEMWIN_QUANTITY);
        gTasks[taskId].func = Task_ChooseHowManyToToss;
    }
}

static void AskTossItems(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    u8 *end = CopyItemNameHandlePlural(gSpecialVar_ItemId, gStringVar1, tItemCount);
    WrapFontIdToFit(gStringVar1, end, FONT_NORMAL, WindowWidthPx(WIN_DESCRIPTION) - 10 - 6);
    ConvertIntToDecimalStringN(gStringVar2, tItemCount, STR_CONV_MODE_LEFT_ALIGN, MAX_ITEM_DIGITS);
    StringExpandPlaceholders(gStringVar4, gText_ConfirmTossItems);
    PrepareHgssBagDescriptionPanel();
    BagMenu_Print(WIN_DESCRIPTION, FONT_NORMAL, gStringVar4, 3, 1, 0, 0, 0, COLORID_NORMAL);
    BagMenu_YesNo(taskId, ITEMWIN_YESNO_LOW, &sYesNoTossFunctions);
}

static void CancelToss(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    PrintItemDescription(tListPosition);
    BagMenu_PrintCursor(tListTaskId, COLORID_NORMAL);
    ReturnToItemList(taskId);
}

static void Task_ChooseHowManyToToss(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    if (AdjustQuantityAccordingToDPadInput(&tItemCount, tQuantity) == TRUE)
    {
        PrintItemQuantity(gBagMenu->windowIds[ITEMWIN_QUANTITY], tItemCount);
    }
    else if (JOY_NEW(A_BUTTON))
    {
        PlaySE(SE_SELECT);
        BagMenu_RemoveWindow(ITEMWIN_QUANTITY);
        AskTossItems(taskId);
    }
    else if (JOY_NEW(B_BUTTON))
    {
        PlaySE(SE_SELECT);
        BagMenu_RemoveWindow(ITEMWIN_QUANTITY);
        CancelToss(taskId);
    }
}

static void ConfirmToss(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    u8 *end = CopyItemNameHandlePlural(gSpecialVar_ItemId, gStringVar1, tItemCount);
    WrapFontIdToFit(gStringVar1, end, FONT_NORMAL, WindowWidthPx(WIN_DESCRIPTION) - 10 - 6);
    ConvertIntToDecimalStringN(gStringVar2, tItemCount, STR_CONV_MODE_LEFT_ALIGN, MAX_ITEM_DIGITS);
    StringExpandPlaceholders(gStringVar4, gText_ThrewAwayVar2Var1s);
    PrepareHgssBagDescriptionPanel();
    BagMenu_Print(WIN_DESCRIPTION, FONT_NORMAL, gStringVar4, 3, 1, 0, 0, 0, COLORID_NORMAL);
    if (CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE || FlagGet(FLAG_STORING_ITEMS_IN_PYRAMID_BAG) == TRUE)
        gTasks[taskId].func = Task_RemoveItemFromBag;
    else
        gTasks[taskId].func = Task_TossItemFromBag;
}

static void Task_TossItemFromBag(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    u16 *scrollPos = &gBagPosition.scrollPosition[gBagPosition.pocket];
    u16 *cursorPos = &gBagPosition.cursorPosition[gBagPosition.pocket];

    if (JOY_NEW(A_BUTTON | B_BUTTON))
    {
        PlaySE(SE_SELECT);
        RemoveBagItemFromSlot(&gBagPockets[gBagPosition.pocket], *scrollPos + *cursorPos, tItemCount);
        DestroyListMenuTask(tListTaskId, scrollPos, cursorPos);
        UpdatePocketItemList(gBagPosition.pocket);
        UpdatePocketListPosition(gBagPosition.pocket);
        LoadBagItemListBuffers(gBagPosition.pocket);
        tListTaskId = ListMenuInit(&gMultiuseListMenuTemplate, *scrollPos, *cursorPos);
        ScheduleBgCopyTilemapToVram(0);
        ReturnToItemList(taskId);
    }
}

// Remove selected item(s) from the bag and update list
// For when items are deposited
static void Task_RemoveItemFromBag(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    u16 *scrollPos = &gBagPosition.scrollPosition[gBagPosition.pocket];
    u16 *cursorPos = &gBagPosition.cursorPosition[gBagPosition.pocket];

    if (JOY_NEW(A_BUTTON | B_BUTTON))
    {
        PlaySE(SE_SELECT);
        RemoveBagItem(gSpecialVar_ItemId, tItemCount);
        DestroyListMenuTask(tListTaskId, scrollPos, cursorPos);
        UpdatePocketItemList(gBagPosition.pocket);
        UpdatePocketListPosition(gBagPosition.pocket);
        LoadBagItemListBuffers(gBagPosition.pocket);
        tListTaskId = ListMenuInit(&gMultiuseListMenuTemplate, *scrollPos, *cursorPos);
        ScheduleBgCopyTilemapToVram(0);
        ReturnToItemList(taskId);
    }
}

static void ItemMenu_Register(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    u16 *scrollPos = &gBagPosition.scrollPosition[gBagPosition.pocket];
    u16 *cursorPos = &gBagPosition.cursorPosition[gBagPosition.pocket];

    if (gSaveBlock1Ptr->registeredItem == gSpecialVar_ItemId)
        gSaveBlock1Ptr->registeredItem = ITEM_NONE;
    else
        gSaveBlock1Ptr->registeredItem = gSpecialVar_ItemId;
    DestroyListMenuTask(tListTaskId, scrollPos, cursorPos);
    LoadBagItemListBuffers(gBagPosition.pocket);
    tListTaskId = ListMenuInit(&gMultiuseListMenuTemplate, *scrollPos, *cursorPos);
    ScheduleBgCopyTilemapToVram(0);
    ItemMenu_Cancel(taskId);
}

static void ItemMenu_Give(u8 taskId)
{
    RemoveContextWindow();
    if (!IsWritingMailAllowed(gSpecialVar_ItemId))
    {
        DisplayItemMessage(taskId, FONT_NORMAL, gText_CantWriteMail, HandleErrorMessage);
    }
    else if (!GetItemImportance(gSpecialVar_ItemId))
    {
        if (CalculatePlayerPartyCount() == 0)
        {
            PrintThereIsNoPokemon(taskId);
        }
        else
        {
            gBagMenu->newScreenCallback = CB2_ChooseMonToGiveItem;
            Task_FadeAndCloseBagMenu(taskId);
        }
    }
    else
    {
        PrintItemCantBeHeld(taskId);
    }
}

static void PrintThereIsNoPokemon(u8 taskId)
{
    DisplayItemMessage(taskId, FONT_NORMAL, gText_NoPokemon, HandleErrorMessage);
}

static void PrintItemCantBeHeld(u8 taskId)
{
    CopyItemName(gSpecialVar_ItemId, gStringVar1);
    StringExpandPlaceholders(gStringVar4, gText_Var1CantBeHeld);
    DisplayItemMessage(taskId, FONT_NORMAL, gStringVar4, HandleErrorMessage);
}

static void HandleErrorMessage(u8 taskId)
{
    if (JOY_NEW(A_BUTTON))
    {
        PlaySE(SE_SELECT);
        CloseItemMessage(taskId);
    }
}

static void ItemMenu_CheckTag(u8 taskId)
{
    gBagMenu->newScreenCallback = DoBerryTagScreen;
    Task_FadeAndCloseBagMenu(taskId);
}

static void ItemMenu_Cancel(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    RemoveContextWindow();
    PrintItemDescription(tListPosition);
    ScheduleBgCopyTilemapToVram(0);
    ScheduleBgCopyTilemapToVram(1);
    BagMenu_PrintCursor(tListTaskId, COLORID_NORMAL);
    ReturnToItemList(taskId);
}

static void ItemMenu_UseInBattle(u8 taskId)
{
    // Safety check
    enum ItemType type = GetItemType(gSpecialVar_ItemId);
    if (!GetItemBattleUsage(gSpecialVar_ItemId))
        return;

    RemoveContextWindow();
    if (type == ITEM_USE_BAG_MENU || (type == ITEM_USE_BATTLER && !IsDoubleBattle()))
        ItemUseInBattle_BagMenu(taskId);
    else if (type == ITEM_USE_PARTY_MENU || (type == ITEM_USE_BATTLER && IsDoubleBattle()))
        ItemUseInBattle_PartyMenu(taskId);
    else if (type == ITEM_USE_PARTY_MENU_MOVES)
        ItemUseInBattle_PartyMenuChooseMove(taskId);
}

void CB2_ReturnToBagMenuPocket(void)
{
    GoToBagMenu(ITEMMENULOCATION_LAST, POCKETS_COUNT, NULL);
}

static void Task_ItemContext_GiveToParty(u8 taskId)
{
    if (!IsWritingMailAllowed(gSpecialVar_ItemId))
    {
        DisplayItemMessage(taskId, FONT_NORMAL, gText_CantWriteMail, HandleErrorMessage);
    }
    else if (!IsHoldingItemAllowed(gSpecialVar_ItemId))
    {
        CopyItemName(gSpecialVar_ItemId, gStringVar1);
        StringExpandPlaceholders(gStringVar4, sText_Var1CantBeHeldHere);
        DisplayItemMessage(taskId, FONT_NORMAL, gStringVar4, HandleErrorMessage);
    }
    else if (gBagPosition.pocket != POCKET_KEY_ITEMS && !GetItemImportance(gSpecialVar_ItemId))
    {
        Task_FadeAndCloseBagMenu(taskId);
    }
    else
    {
        PrintItemCantBeHeld(taskId);
    }
}

// Selected item to give to a Pokémon in PC storage
static void Task_ItemContext_GiveToPC(u8 taskId)
{
    if (ItemIsMail(gSpecialVar_ItemId) == TRUE)
        DisplayItemMessage(taskId, FONT_NORMAL, gText_CantWriteMail, HandleErrorMessage);
    else if (gBagPosition.pocket != POCKET_KEY_ITEMS && !GetItemImportance(gSpecialVar_ItemId))
        gTasks[taskId].func = Task_FadeAndCloseBagMenu;
    else
        PrintItemCantBeHeld(taskId);
}

#define tUsingRegisteredKeyItem data[3] // See usage in item_use.c

bool8 UseRegisteredKeyItemOnField(void)
{
    u8 taskId;

    if (InUnionRoom() == TRUE || CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE || InBattlePike() || InMultiPartnerRoom() == TRUE)
        return FALSE;
    HideMapNamePopUpWindow();
    ChangeBgY_ScreenOff(0, 0, BG_COORD_SET);
    if (gSaveBlock1Ptr->registeredItem != ITEM_NONE)
    {
        if (CheckBagHasItem(gSaveBlock1Ptr->registeredItem, 1) == TRUE)
        {
            LockPlayerFieldControls();
            FreezeObjectEvents();
            PlayerFreeze();
            StopPlayerAvatar();
            gSpecialVar_ItemId = gSaveBlock1Ptr->registeredItem;
            taskId = CreateTask(GetItemFieldFunc(gSaveBlock1Ptr->registeredItem), 8);
            gTasks[taskId].tUsingRegisteredKeyItem = TRUE;
            return TRUE;
        }
        else
        {
            gSaveBlock1Ptr->registeredItem = ITEM_NONE;
        }
    }
    ScriptContext_SetupScript(EventScript_SelectWithoutRegisteredItem);
    return TRUE;
}

#undef tUsingRegisteredKeyItem

static void Task_ItemContext_Sell(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    if (GetItemPrice(gSpecialVar_ItemId) == 0 || GetItemImportance(gSpecialVar_ItemId))
    {
        CopyItemName(gSpecialVar_ItemId, gStringVar2);
        StringExpandPlaceholders(gStringVar4, gText_CantBuyKeyItem);
        DisplayItemMessage(taskId, FONT_NORMAL, gStringVar4, CloseItemMessage);
    }
    else
    {
        tItemCount = 1;
        if (tQuantity == 1)
        {
            DisplayCurrentMoneyWindow();
            DisplaySellItemPriceAndConfirm(taskId);
        }
        else
        {
            u32 maxQuantity = MAX_MONEY / GetItemSellPrice(gSpecialVar_ItemId);

            if (tQuantity > maxQuantity)
                tQuantity = maxQuantity;

            CopyItemName(gSpecialVar_ItemId, gStringVar2);
            StringExpandPlaceholders(gStringVar4, gText_HowManyToSell);
            DisplayItemMessage(taskId, FONT_NORMAL, gStringVar4, InitSellHowManyInput);
        }
    }
}

static void DisplaySellItemPriceAndConfirm(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    ConvertIntToDecimalStringN(gStringVar1, GetItemSellPrice(gSpecialVar_ItemId) * tItemCount, STR_CONV_MODE_LEFT_ALIGN, MAX_MONEY_DIGITS);
    StringExpandPlaceholders(gStringVar4, gText_ICanPayVar1);
    DisplayItemMessage(taskId, FONT_NORMAL, gStringVar4, AskSellItems);
}

static void AskSellItems(u8 taskId)
{
    BagMenu_YesNo(taskId, ITEMWIN_YESNO_HIGH, &sYesNoSellItemFunctions);
}

static void CancelSell(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    RemoveMoneyWindow();
    RemoveItemMessageWindow(ITEMWIN_MESSAGE);
    BagMenu_PrintCursor(tListTaskId, COLORID_NORMAL);
    ReturnToItemList(taskId);
}

static void InitSellHowManyInput(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    u8 windowId = BagMenu_AddWindow(ITEMWIN_QUANTITY_WIDE);

    PrintItemSoldAmount(windowId, 1, GetItemSellPrice(gSpecialVar_ItemId) * tItemCount);
    DisplayCurrentMoneyWindow();
    gTasks[taskId].func = Task_ChooseHowManyToSell;
}

static void Task_ChooseHowManyToSell(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    if (AdjustQuantityAccordingToDPadInput(&tItemCount, tQuantity) == TRUE)
    {
        PrintItemSoldAmount(gBagMenu->windowIds[ITEMWIN_QUANTITY_WIDE], tItemCount, GetItemSellPrice(gSpecialVar_ItemId) * tItemCount);
    }
    else if (JOY_NEW(A_BUTTON))
    {
        PlaySE(SE_SELECT);
        BagMenu_RemoveWindow(ITEMWIN_QUANTITY_WIDE);
        DisplaySellItemPriceAndConfirm(taskId);
    }
    else if (JOY_NEW(B_BUTTON))
    {
        PlaySE(SE_SELECT);
        BagMenu_PrintCursor(tListTaskId, COLORID_NORMAL);
        RemoveMoneyWindow();
        BagMenu_RemoveWindow(ITEMWIN_QUANTITY_WIDE);
        RemoveItemMessageWindow(ITEMWIN_MESSAGE);
        ReturnToItemList(taskId);
    }
}

static void ConfirmSell(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    CopyItemName(gSpecialVar_ItemId, gStringVar2);
    ConvertIntToDecimalStringN(gStringVar1, GetItemSellPrice(gSpecialVar_ItemId) * tItemCount, STR_CONV_MODE_LEFT_ALIGN, MAX_MONEY_DIGITS);
    StringExpandPlaceholders(gStringVar4, gText_TurnedOverVar1ForVar2);
    DisplayItemMessage(taskId, FONT_NORMAL, gStringVar4, SellItem);
}

static void SellItem(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    u16 *scrollPos = &gBagPosition.scrollPosition[gBagPosition.pocket];
    u16 *cursorPos = &gBagPosition.cursorPosition[gBagPosition.pocket];

    PlaySE(SE_SHOP);
    RemoveBagItem(gSpecialVar_ItemId, tItemCount);
    AddMoney(&gSaveBlock1Ptr->money, GetItemSellPrice(gSpecialVar_ItemId) * tItemCount);
    DestroyListMenuTask(tListTaskId, scrollPos, cursorPos);
    UpdatePocketItemList(gBagPosition.pocket);
    UpdatePocketListPosition(gBagPosition.pocket);
    LoadBagItemListBuffers(gBagPosition.pocket);
    tListTaskId = ListMenuInit(&gMultiuseListMenuTemplate, *scrollPos, *cursorPos);
    BagMenu_PrintCursor(tListTaskId, COLORID_GRAY_CURSOR);
    DisplayCurrentMoneyWindow();
    gTasks[taskId].func = WaitAfterItemSell;
}

static void WaitAfterItemSell(u8 taskId)
{
    if (JOY_NEW(A_BUTTON | B_BUTTON))
    {
        PlaySE(SE_SELECT);
        RemoveMoneyWindow();
        CloseItemMessage(taskId);
    }
}

static void Task_ItemContext_Deposit(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    tItemCount = 1;
    if (tQuantity == 1)
    {
        TryDepositItem(taskId);
    }
    else
    {
        u8 *end = CopyItemNameHandlePlural(gSpecialVar_ItemId, gStringVar1, 2);
        WrapFontIdToFit(gStringVar1, end, FONT_NORMAL, WindowWidthPx(WIN_DESCRIPTION) - 10 - 6);
        StringExpandPlaceholders(gStringVar4, sText_DepositHowManyVar1);
        PrepareHgssBagDescriptionPanel();
        BagMenu_Print(WIN_DESCRIPTION, FONT_NORMAL, gStringVar4, 3, 1, 0, 0, 0, COLORID_NORMAL);
        AddItemQuantityWindow(ITEMWIN_QUANTITY);
        gTasks[taskId].func = Task_ChooseHowManyToDeposit;
    }
}

static void Task_ChooseHowManyToDeposit(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    if (AdjustQuantityAccordingToDPadInput(&tItemCount, tQuantity) == TRUE)
    {
        PrintItemQuantity(gBagMenu->windowIds[ITEMWIN_QUANTITY], tItemCount);
    }
    else if (JOY_NEW(A_BUTTON))
    {
        PlaySE(SE_SELECT);
        BagMenu_RemoveWindow(ITEMWIN_QUANTITY);
        TryDepositItem(taskId);
    }
    else if (JOY_NEW(B_BUTTON))
    {
        PlaySE(SE_SELECT);
        PrintItemDescription(tListPosition);
        BagMenu_PrintCursor(tListTaskId, COLORID_NORMAL);
        BagMenu_RemoveWindow(ITEMWIN_QUANTITY);
        ReturnToItemList(taskId);
    }
}

static void TryDepositItem(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    PrepareHgssBagDescriptionPanel();
    if (GetItemImportance(gSpecialVar_ItemId))
    {
        // Can't deposit important items
        BagMenu_Print(WIN_DESCRIPTION, FONT_NORMAL, sText_CantStoreImportantItems, 3, 1, 0, 0, 0, COLORID_NORMAL);
        gTasks[taskId].func = WaitDepositErrorMessage;
    }
    else if (AddPCItem(gSpecialVar_ItemId, tItemCount) == TRUE)
    {
        // Successfully deposited
        u8 *end = CopyItemNameHandlePlural(gSpecialVar_ItemId, gStringVar1, tItemCount);
        WrapFontIdToFit(gStringVar1, end, FONT_NORMAL, WindowWidthPx(WIN_DESCRIPTION) - 10 - 6);
        ConvertIntToDecimalStringN(gStringVar2, tItemCount, STR_CONV_MODE_LEFT_ALIGN, MAX_ITEM_DIGITS);
        StringExpandPlaceholders(gStringVar4, sText_DepositedVar2Var1s);
        BagMenu_Print(WIN_DESCRIPTION, FONT_NORMAL, gStringVar4, 3, 1, 0, 0, 0, COLORID_NORMAL);
        gTasks[taskId].func = Task_RemoveItemFromBag;
    }
    else
    {
        // No room to deposit
        BagMenu_Print(WIN_DESCRIPTION, FONT_NORMAL, sText_NoRoomForItems, 3, 1, 0, 0, 0, COLORID_NORMAL);
        gTasks[taskId].func = WaitDepositErrorMessage;
    }
}

static void WaitDepositErrorMessage(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    if (JOY_NEW(A_BUTTON | B_BUTTON))
    {
        PlaySE(SE_SELECT);
        PrintItemDescription(tListPosition);
        BagMenu_PrintCursor(tListTaskId, COLORID_NORMAL);
        ReturnToItemList(taskId);
    }
}

static bool8 IsWallysBag(void)
{
    if (gBagPosition.location == ITEMMENULOCATION_WALLY)
        return TRUE;
    return FALSE;
}

static void PrepareBagForWallyTutorial(void)
{
    u32 i;

    sTempWallyBag = AllocZeroed(sizeof(*sTempWallyBag));
    memcpy(sTempWallyBag->bagPocket_Items, gSaveBlock1Ptr->bag.items, sizeof(gSaveBlock1Ptr->bag.items));
    memcpy(sTempWallyBag->bagPocket_PokeBalls, gSaveBlock1Ptr->bag.pokeBalls, sizeof(gSaveBlock1Ptr->bag.pokeBalls));
    sTempWallyBag->pocket = gBagPosition.pocket;
    for (i = 0; i < POCKETS_COUNT; i++)
    {
        sTempWallyBag->cursorPosition[i] = gBagPosition.cursorPosition[i];
        sTempWallyBag->scrollPosition[i] = gBagPosition.scrollPosition[i];
    }
    memset(gSaveBlock1Ptr->bag.items, 0, sizeof(gSaveBlock1Ptr->bag.items));
    memset(gSaveBlock1Ptr->bag.pokeBalls, 0, sizeof(gSaveBlock1Ptr->bag.pokeBalls));
    ResetBagScrollPositions();
}

static void RestoreBagAfterWallyTutorial(void)
{
    u32 i;

    memcpy(gSaveBlock1Ptr->bag.items, sTempWallyBag->bagPocket_Items, sizeof(sTempWallyBag->bagPocket_Items));
    memcpy(gSaveBlock1Ptr->bag.pokeBalls, sTempWallyBag->bagPocket_PokeBalls, sizeof(sTempWallyBag->bagPocket_PokeBalls));
    gBagPosition.pocket = sTempWallyBag->pocket;
    for (i = 0; i < POCKETS_COUNT; i++)
    {
        gBagPosition.cursorPosition[i] = sTempWallyBag->cursorPosition[i];
        gBagPosition.scrollPosition[i] = sTempWallyBag->scrollPosition[i];
    }
    Free(sTempWallyBag);
}

void DoWallyTutorialBagMenu(void)
{
    PrepareBagForWallyTutorial();
    AddBagItem(ITEM_POTION, 1);
    AddBagItem(ITEM_POKE_BALL, 1);
    GoToBagMenu(ITEMMENULOCATION_WALLY, POCKET_ITEMS, CB2_SetUpReshowBattleScreenAfterMenu2);
}

void InitOldManBag(void)
{
    PrepareBagForWallyTutorial();
    AddBagItem(ITEM_POTION, 1);
    AddBagItem(ITEM_POKE_BALL, 1);
    GoToBagMenu(ITEMMENULOCATION_WALLY, POCKET_ITEMS, CB2_SetUpReshowBattleScreenAfterMenu2);
}

#define tTimer data[8]
#define WALLY_BAG_DELAY 102 // The number of frames between each action Wally takes in the bag

static void Task_WallyTutorialBagMenu(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    if (!gPaletteFade.active)
    {
        switch (tTimer)
        {
        case WALLY_BAG_DELAY * 1:
            PlaySE(SE_SELECT);
            SwitchBagPocket(taskId, MENU_CURSOR_DELTA_RIGHT, FALSE);
            tTimer++;
            break;
        case WALLY_BAG_DELAY * 2:
            PlaySE(SE_SELECT);
            BagMenu_PrintCursor(tListTaskId, COLORID_GRAY_CURSOR);
            gSpecialVar_ItemId = ITEM_POKE_BALL;
            OpenContextMenu(taskId);
            tTimer++;
            break;
        case WALLY_BAG_DELAY * 3:
            PlaySE(SE_SELECT);
            RemoveContextWindow();
            DestroyListMenuTask(tListTaskId, 0, 0);
            RestoreBagAfterWallyTutorial();
            Task_FadeAndCloseBagMenu(taskId);
            break;
        default:
            tTimer++;
            break;
        }
    }
}

#undef tTimer

// This action is used to show the Apprentice an item when
// they ask what item they should make their Pokémon hold
static void ItemMenu_Show(u8 taskId)
{
    gSpecialVar_0x8005 = gSpecialVar_ItemId;
    gSpecialVar_Result = TRUE;
    RemoveContextWindow();
    Task_FadeAndCloseBagMenu(taskId);
}

static void CB2_ApprenticeExitBagMenu(void)
{
    gFieldCallback = Apprentice_ScriptContext_Enable;
    SetMainCallback2(CB2_ReturnToField);
}

static void ItemMenu_GiveFavorLady(u8 taskId)
{
    RemoveBagItem(gSpecialVar_ItemId, 1);
    gSpecialVar_Result = TRUE;
    RemoveContextWindow();
    Task_FadeAndCloseBagMenu(taskId);
}

static void CB2_FavorLadyExitBagMenu(void)
{
    gFieldCallback = FieldCallback_FavorLadyEnableScriptContexts;
    SetMainCallback2(CB2_ReturnToField);
}

// This action is used to confirm which item to use as
// a prize for a custom quiz with the Lilycove Quiz Lady
static void ItemMenu_ConfirmQuizLady(u8 taskId)
{
    gSpecialVar_Result = TRUE;
    RemoveContextWindow();
    Task_FadeAndCloseBagMenu(taskId);
}

static void CB2_QuizLadyExitBagMenu(void)
{
    gFieldCallback = FieldCallback_QuizLadyEnableScriptContexts;
    SetMainCallback2(CB2_ReturnToField);
}

static void PrintPocketNames(const u8 *pocketName1, const u8 *pocketName2)
{
    struct WindowTemplate window = {0};
    u16 windowId;
    int offset;

    window.width = 16;
    window.height = 2;
    windowId = AddWindow(&window);
    FillWindowPixelBuffer(windowId, PIXEL_FILL(0));
    offset = GetStringCenterAlignXOffset(FONT_SMALL, pocketName1, 0x40);
    BagMenu_Print(windowId, FONT_SMALL, pocketName1, offset, 1, 0, 0, TEXT_SKIP_DRAW, COLORID_POCKET_TITLE);
    FillWindowPixelRect(windowId, PIXEL_FILL(HGSS_BAG_CELL_ACTIVE_COLOR), 8, 14, 48, 1);
    FillWindowPixelRect(windowId, PIXEL_FILL(HGSS_BAG_CELL_ACTIVE_COLOR), 10, 15, 44, 1);
    if (pocketName2)
    {
        offset = GetStringCenterAlignXOffset(FONT_SMALL, pocketName2, 0x40);
        BagMenu_Print(windowId, FONT_SMALL, pocketName2, offset + 0x40, 1, 0, 0, TEXT_SKIP_DRAW, COLORID_POCKET_TITLE);
        FillWindowPixelRect(windowId, PIXEL_FILL(HGSS_BAG_CELL_ACTIVE_COLOR), 72, 14, 48, 1);
        FillWindowPixelRect(windowId, PIXEL_FILL(HGSS_BAG_CELL_ACTIVE_COLOR), 74, 15, 44, 1);
    }
    CpuCopy32((u8 *)GetWindowAttribute(windowId, WINDOW_TILE_DATA), gBagMenu->pocketNameBuffer, sizeof(gBagMenu->pocketNameBuffer));
    RemoveWindow(windowId);
}

static void CopyPocketNameToWindow(u32 a)
{
    u8 (*tileDataBuffer)[32][32];
    u8 *windowTileData;
    int b;
    if (a > 8)
        a = 8;
    tileDataBuffer = &gBagMenu->pocketNameBuffer;
    windowTileData = (u8 *)GetWindowAttribute(2, WINDOW_TILE_DATA);
    CpuCopy32(&tileDataBuffer[0][a], windowTileData, 0x100); // Top half of pocket name
    b = a + 16;
    CpuCopy32(&tileDataBuffer[0][b], windowTileData + 0x100, 0x100); // Bottom half of pocket name
    CopyWindowToVram(WIN_POCKET_NAME, COPYWIN_GFX);
}

static void LoadBagMenuTextWindows(void)
{
    u8 i;

    InitWindows(sDefaultBagWindows);
    DeactivateAllTextPrinters();
    // HGSS Bag panels draw their own pixel borders; no Emerald window/message frame tiles are needed here.
    ListMenuLoadStdPalAt(BG_PLTT_ID(12), 1);
    LoadPalette(&gStandardMenuPalette, BG_PLTT_ID(15), PLTT_SIZE_4BPP);
    for (i = 0; i <= WIN_POCKET_NAME; i++)
    {
        FillWindowPixelBuffer(i, PIXEL_FILL(0));
        PutWindowTilemap(i);
    }
    ScheduleBgCopyTilemapToVram(0);
    ScheduleBgCopyTilemapToVram(1);
}

static void BagMenu_Print(u8 windowId, u8 fontId, const u8 *str, u8 left, u8 top, u8 letterSpacing, u8 lineSpacing, u8 speed, u8 colorIndex)
{
    AddTextPrinterParameterized4(windowId, fontId, left, top, letterSpacing, lineSpacing, sFontColorTable[colorIndex], speed, str);
}

static u8 UNUSED BagMenu_GetWindowId(u8 windowType)
{
    return gBagMenu->windowIds[windowType];
}

static u8 BagMenu_AddWindow(u8 windowType)
{
    u8 *windowId = &gBagMenu->windowIds[windowType];
    if (*windowId == WINDOW_NONE)
    {
        *windowId = AddWindow(&sContextMenuWindowTemplates[windowType]);
        FillWindowPixelBuffer(*windowId, PIXEL_FILL(1));
        PutWindowTilemap(*windowId);
        ScheduleBgCopyTilemapToVram(1);
    }
    return *windowId;
}

static void BagMenu_RemoveWindow(u8 windowType)
{
    u8 *windowId = &gBagMenu->windowIds[windowType];
    if (*windowId != WINDOW_NONE)
    {
        FillWindowPixelBuffer(*windowId, PIXEL_FILL(0));
        ClearWindowTilemap(*windowId);
        RemoveWindow(*windowId);
        ScheduleBgCopyTilemapToVram(1);
        *windowId = WINDOW_NONE;
    }
}

static u8 AddItemMessageWindow(u8 windowType)
{
    u8 *windowId = &gBagMenu->windowIds[windowType];
    if (*windowId == WINDOW_NONE)
        *windowId = AddWindow(&sContextMenuWindowTemplates[windowType]);
    return *windowId;
}

static void RemoveItemMessageWindow(u8 windowType)
{
    u8 *windowId = &gBagMenu->windowIds[windowType];
    if (*windowId != WINDOW_NONE)
    {
        FillWindowPixelBuffer(*windowId, PIXEL_FILL(0));
        ClearWindowTilemap(*windowId);
        RemoveWindow(*windowId);
        ScheduleBgCopyTilemapToVram(1);
        *windowId = WINDOW_NONE;
    }
}

static void DrawHgssBagYesNoMenu(void)
{
    u8 windowId = gBagMenu->windowIds[sHgssBagYesNoWindowType];
    u8 width = GetWindowAttribute(windowId, WINDOW_WIDTH) * 8;
    u8 height = GetWindowAttribute(windowId, WINDOW_HEIGHT) * 8;

    FillWindowPixelBuffer(windowId, PIXEL_FILL(1));
    BagMenu_Print(windowId, FONT_SMALL, gText_YesNo, 8, 2, 0, 4, TEXT_SKIP_DRAW, COLORID_POCKET_NAME);
    {
        u8 cursorY = 3 + sHgssBagYesNoChoice * 16;

        FillWindowPixelRect(windowId, PIXEL_FILL(TEXT_COLOR_RED), 3, cursorY, 2, 1);
        FillWindowPixelRect(windowId, PIXEL_FILL(TEXT_COLOR_RED), 2, cursorY + 1, 4, 9);
        FillWindowPixelRect(windowId, PIXEL_FILL(TEXT_COLOR_RED), 3, cursorY + 10, 2, 1);
    }

    // Keep the HGSS frame above both choice labels.
    FillWindowPixelRect(windowId, PIXEL_FILL(TEXT_COLOR_RED), 1, 0, width - 2, 1);
    FillWindowPixelRect(windowId, PIXEL_FILL(TEXT_COLOR_RED), 1, height - 1, width - 2, 1);
    FillWindowPixelRect(windowId, PIXEL_FILL(TEXT_COLOR_RED), 0, 1, 1, height - 2);
    FillWindowPixelRect(windowId, PIXEL_FILL(TEXT_COLOR_RED), width - 1, 1, 1, height - 2);
    CopyWindowToVram(windowId, COPYWIN_GFX);
}

static void Task_HgssBagYesNo(u8 taskId)
{
    if (JOY_NEW(DPAD_UP) && sHgssBagYesNoChoice != 0)
    {
        PlaySE(SE_SELECT);
        sHgssBagYesNoChoice = 0;
        DrawHgssBagYesNoMenu();
    }
    else if (JOY_NEW(DPAD_DOWN) && sHgssBagYesNoChoice != 1)
    {
        PlaySE(SE_SELECT);
        sHgssBagYesNoChoice = 1;
        DrawHgssBagYesNoMenu();
    }
    else if (JOY_NEW(A_BUTTON))
    {
        TaskFunc callback = sHgssBagYesNoChoice == 0 ? sHgssBagYesNoFuncs.yesFunc : sHgssBagYesNoFuncs.noFunc;

        PlaySE(SE_SELECT);
        BagMenu_RemoveWindow(sHgssBagYesNoWindowType);
        callback(taskId);
    }
    else if (JOY_NEW(B_BUTTON))
    {
        PlaySE(SE_SELECT);
        BagMenu_RemoveWindow(sHgssBagYesNoWindowType);
        sHgssBagYesNoFuncs.noFunc(taskId);
    }
}

void BagMenu_YesNo(u8 taskId, u8 windowType, const struct YesNoFuncTable *funcTable)
{
    sHgssBagYesNoFuncs = *funcTable;
    sHgssBagYesNoWindowType = windowType;
    sHgssBagYesNoChoice = 0;

    BagMenu_AddWindow(windowType);
    DrawHgssBagYesNoMenu();
    gTasks[taskId].func = Task_HgssBagYesNo;
}

static void DisplayCurrentMoneyWindow(void)
{
    static const u8 sText_Money[] = _("MONEY");
    u8 windowId = BagMenu_AddWindow(ITEMWIN_MONEY);

    PrepareHgssBagQuantityPanel(windowId);
    FillWindowPixelRect(windowId, PIXEL_FILL(TEXT_COLOR_RED), 44, 2, 1,
                        GetWindowAttribute(windowId, WINDOW_HEIGHT) * 8 - 4);
    BagMenu_Print(windowId, FONT_SMALL, sText_Money,
                  1 + GetStringCenterAlignXOffset(FONT_SMALL, sText_Money, 43), 2,
                  0, 0, TEXT_SKIP_DRAW, COLORID_POCKET_NAME);
    PrintMoneyAmount(windowId, 48, 1, GetMoney(&gSaveBlock1Ptr->money), 0);
    FillWindowPixelRect(windowId, PIXEL_FILL(TEXT_COLOR_RED), 44, 2, 1,
                        GetWindowAttribute(windowId, WINDOW_HEIGHT) * 8 - 4);
    DrawHgssBagQuantityPanelFrame(windowId);
    CopyWindowToVram(windowId, COPYWIN_GFX);
}

static void RemoveMoneyWindow(void)
{
    BagMenu_RemoveWindow(ITEMWIN_MONEY);
}

static void PrepareTMHMMoveWindow(void)
{
    u8 width = GetWindowAttribute(WIN_TMHM_INFO_ICONS, WINDOW_WIDTH) * 8;
    u8 height = GetWindowAttribute(WIN_TMHM_INFO_ICONS, WINDOW_HEIGHT) * 8;

    FillWindowPixelBuffer(WIN_TMHM_INFO_ICONS, PIXEL_FILL(0));
    BlitMenuInfoIcon(WIN_TMHM_INFO_ICONS, MENU_INFO_ICON_TYPE, 0, 0);
    BlitMenuInfoIcon(WIN_TMHM_INFO_ICONS, MENU_INFO_ICON_POWER, 0, 12);
    BlitMenuInfoIcon(WIN_TMHM_INFO_ICONS, MENU_INFO_ICON_ACCURACY, 0, 24);
    BlitMenuInfoIcon(WIN_TMHM_INFO_ICONS, MENU_INFO_ICON_PP, 0, 36);

    // The 42 px labels are clipped by this 40 px window, so restore the HGSS frame after blitting them.
    FillWindowPixelRect(WIN_TMHM_INFO_ICONS, PIXEL_FILL(TEXT_DYNAMIC_COLOR_5), 1, 0, width - 2, 1);
    FillWindowPixelRect(WIN_TMHM_INFO_ICONS, PIXEL_FILL(TEXT_DYNAMIC_COLOR_5), 0, 1, 1, height - 2);
    FillWindowPixelRect(WIN_TMHM_INFO_ICONS, PIXEL_FILL(TEXT_DYNAMIC_COLOR_5), width - 1, 1, 1, height - 2);
    CopyWindowToVram(WIN_TMHM_INFO_ICONS, COPYWIN_GFX);
}

static void PrintTMHMMoveData(enum Item itemId)
{
    u8 i;
    u8 width = GetWindowAttribute(WIN_TMHM_INFO, WINDOW_WIDTH) * 8;
    u8 height = GetWindowAttribute(WIN_TMHM_INFO, WINDOW_HEIGHT) * 8;
    enum Move move;
    const u8 *text;

    FillWindowPixelBuffer(WIN_TMHM_INFO, PIXEL_FILL(0));
    if (itemId == ITEM_NONE)
    {
        for (i = 0; i < 4; i++)
            BagMenu_Print(WIN_TMHM_INFO, FONT_SMALL, gText_ThreeDashes,
                          1 + GetStringCenterAlignXOffset(FONT_SMALL, gText_ThreeDashes, width - 2), i * 12,
                          0, 0, TEXT_SKIP_DRAW, COLORID_TMHM_INFO);
    }
    else
    {
        move = ItemIdToBattleMoveId(itemId);
        BlitMenuInfoIcon(WIN_TMHM_INFO, GetMoveType(move) + 1, 0, 0);

        // Print TMHM power
        u32 power = GetMovePower(move);
        if (power <= 1)
        {
            text = gText_ThreeDashes;
        }
        else
        {
            ConvertIntToDecimalStringN(gStringVar1, power, STR_CONV_MODE_RIGHT_ALIGN, 3);
            text = gStringVar1;
        }
        BagMenu_Print(WIN_TMHM_INFO, FONT_SMALL, text,
                      1 + GetStringCenterAlignXOffset(FONT_SMALL, text, width - 2), 12,
                      0, 0, TEXT_SKIP_DRAW, COLORID_TMHM_INFO);

        u32 accuracy = GetMoveAccuracy(move);
        // Print TMHM accuracy
        if (accuracy == 0)
        {
            text = gText_ThreeDashes;
        }
        else
        {
            ConvertIntToDecimalStringN(gStringVar1, accuracy, STR_CONV_MODE_RIGHT_ALIGN, 3);
            text = gStringVar1;
        }
        BagMenu_Print(WIN_TMHM_INFO, FONT_SMALL, text,
                      1 + GetStringCenterAlignXOffset(FONT_SMALL, text, width - 2), 24,
                      0, 0, TEXT_SKIP_DRAW, COLORID_TMHM_INFO);

        // Print TMHM pp
        ConvertIntToDecimalStringN(gStringVar1, GetMovePP(move), STR_CONV_MODE_RIGHT_ALIGN, 3);
        BagMenu_Print(WIN_TMHM_INFO, FONT_SMALL, gStringVar1,
                      1 + GetStringCenterAlignXOffset(FONT_SMALL, gStringVar1, width - 2), 36,
                      0, 0, TEXT_SKIP_DRAW, COLORID_TMHM_INFO);

    }

    // Keep the HGSS frame above the type badge and top-row placeholder text.
    FillWindowPixelRect(WIN_TMHM_INFO, PIXEL_FILL(TEXT_DYNAMIC_COLOR_1), 1, 0, width - 2, 1);
    FillWindowPixelRect(WIN_TMHM_INFO, PIXEL_FILL(TEXT_DYNAMIC_COLOR_1), 0, 1, 1, height - 2);
    FillWindowPixelRect(WIN_TMHM_INFO, PIXEL_FILL(TEXT_DYNAMIC_COLOR_1), width - 1, 1, 1, height - 2);
    CopyWindowToVram(WIN_TMHM_INFO, COPYWIN_GFX);
}

static const u8 sText_SortItemsHow[] = _("Sort items how?");
static const u8 sText_ItemsSorted[] = _("Items sorted by {STR_VAR_1}!");
static const u8 *const sSortTypeStrings[] =
{
    [SORT_ALPHABETICALLY] = COMPOUND_STRING("name"),
    [SORT_BY_TYPE] = COMPOUND_STRING("type"),
    [SORT_BY_AMOUNT] = COMPOUND_STRING("amount"),
    [SORT_BY_INDEX] = COMPOUND_STRING("index")
};

static const u8 sBagMenuSortItems[] =
{
    ACTION_BY_NAME,
    ACTION_BY_TYPE,
    ACTION_BY_AMOUNT,
    ACTION_CANCEL,
};

static const u8 sBagMenuSortKeyItems[] =
{
    ACTION_BY_NAME,
    ACTION_CANCEL,
};

static const u8 sBagMenuSortPokeBalls[] =
{
    ACTION_BY_NAME,
    ACTION_BY_AMOUNT,
    ACTION_DUMMY,
    ACTION_CANCEL,
};

static const u8 sBagMenuSortBerriesTMsHMs[] =
{
    ACTION_BY_NAME,
    ACTION_BY_AMOUNT,
    ACTION_BY_INDEX,
    ACTION_CANCEL,
};

static void AddBagSortSubMenu(void)
{
    switch (gBagPosition.pocket)
    {
    case POCKET_KEY_ITEMS:
        gBagMenu->contextMenuItemsPtr = sBagMenuSortKeyItems;
        memcpy(&gBagMenu->contextMenuItemsBuffer, &sBagMenuSortKeyItems, NELEMS(sBagMenuSortKeyItems));
        gBagMenu->contextMenuNumItems = NELEMS(sBagMenuSortKeyItems);
        break;
    case POCKET_POKE_BALLS:
        gBagMenu->contextMenuItemsPtr = sBagMenuSortPokeBalls;
        memcpy(&gBagMenu->contextMenuItemsBuffer, &sBagMenuSortPokeBalls, NELEMS(sBagMenuSortPokeBalls));
        gBagMenu->contextMenuNumItems = NELEMS(sBagMenuSortPokeBalls);
        break;
    case POCKET_BERRIES:
    case POCKET_TM_HM:
        gBagMenu->contextMenuItemsPtr = sBagMenuSortBerriesTMsHMs;
        memcpy(&gBagMenu->contextMenuItemsBuffer, &sBagMenuSortBerriesTMsHMs, NELEMS(sBagMenuSortBerriesTMsHMs));
        gBagMenu->contextMenuNumItems = NELEMS(sBagMenuSortBerriesTMsHMs);
        break;
    default:
        gBagMenu->contextMenuItemsPtr = sBagMenuSortItems;
        memcpy(&gBagMenu->contextMenuItemsBuffer, &sBagMenuSortItems, NELEMS(sBagMenuSortItems));
        gBagMenu->contextMenuNumItems = NELEMS(sBagMenuSortItems);
        break;
    }

    StringExpandPlaceholders(gStringVar4, sText_SortItemsHow);
    PrepareHgssBagDescriptionPanel();
    BagMenu_Print(WIN_DESCRIPTION, FONT_NORMAL, gStringVar4, 3, 1, 0, 0, 0, COLORID_NORMAL);
    CopyWindowToVram(WIN_DESCRIPTION, COPYWIN_GFX);

    if (gBagMenu->contextMenuNumItems == 2)
        PrintContextMenuItems(BagMenu_AddWindow(ITEMWIN_1x2));
    else if (gBagMenu->contextMenuNumItems == 4)
        PrintContextMenuItemGrid(BagMenu_AddWindow(ITEMWIN_2x2), 2, 2);
    else
        PrintContextMenuItemGrid(BagMenu_AddWindow(ITEMWIN_2x3), 2, 3);
}

static void Task_LoadBagSortOptions(u8 taskId)
{
    AddBagSortSubMenu();
    if (gBagMenu->contextMenuNumItems <= 2)
        gTasks[taskId].func = Task_ItemContext_SingleRow;
    else
        gTasks[taskId].func = Task_ItemContext_MultipleRows;
}

#define tSortType data[2]
static void ItemMenu_SortByName(u8 taskId)
{
    gTasks[taskId].tSortType = SORT_ALPHABETICALLY;
    StringCopy(gStringVar1, sSortTypeStrings[SORT_ALPHABETICALLY]);
    gTasks[taskId].func = SortBagItems;
}

static void ItemMenu_SortByType(u8 taskId)
{
    gTasks[taskId].tSortType = SORT_BY_TYPE;
    StringCopy(gStringVar1, sSortTypeStrings[SORT_BY_TYPE]);
    gTasks[taskId].func = SortBagItems;
}

static void ItemMenu_SortByAmount(u8 taskId)
{
    gTasks[taskId].tSortType = SORT_BY_AMOUNT; //greatest->least
    StringCopy(gStringVar1, sSortTypeStrings[SORT_BY_AMOUNT]);
    gTasks[taskId].func = SortBagItems;
}

static void ItemMenu_SortByIndex(u8 taskId)
{
    gTasks[taskId].tSortType = SORT_BY_INDEX;
    StringCopy(gStringVar1, sSortTypeStrings[SORT_BY_INDEX]);
    gTasks[taskId].func = SortBagItems;
}

static void SortBagItems(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    u16 *scrollPos = &gBagPosition.scrollPosition[gBagPosition.pocket];
    u16 *cursorPos = &gBagPosition.cursorPosition[gBagPosition.pocket];

    RemoveContextWindow();

    SortItemsInBag(&gBagPockets[gBagPosition.pocket], tSortType);
    DestroyListMenuTask(data[0], scrollPos, cursorPos);
    UpdatePocketListPosition(gBagPosition.pocket);
    LoadBagItemListBuffers(gBagPosition.pocket);
    data[0] = ListMenuInit(&gMultiuseListMenuTemplate, *scrollPos, *cursorPos);
    ScheduleBgCopyTilemapToVram(0);

    StringCopy(gStringVar1, sSortTypeStrings[tSortType]);
    StringExpandPlaceholders(gStringVar4, sText_ItemsSorted);
    DisplayItemMessage(taskId, 1, gStringVar4, Task_SortFinish);
}

#undef tSortType

static void Task_SortFinish(u8 taskId)
{
    if (gMain.newKeys & (A_BUTTON | B_BUTTON))
    {
        RemoveItemMessageWindow(4);
        ReturnToItemList(taskId);
    }
}

void SortItemsInBag(struct BagPocket *pocket, enum BagSortOptions type)
{
    switch (type)
    {
    case SORT_ALPHABETICALLY:
        MergeSort(pocket, CompareItemsAlphabetically);
        break;
    case SORT_BY_AMOUNT:
        MergeSort(pocket, CompareItemsByMost);
        break;
    case SORT_BY_INDEX:
        MergeSort(pocket, CompareItemsByIndex);
        break;
    default:
        MergeSort(pocket, CompareItemsByType);
        break;
    }
}

static inline __attribute__((always_inline)) void Merge(struct BagPocket *pocket, u32 iLeft, u32 iRight, u32 iEnd, struct ItemSlot *dummySlots, s32 (*comparator)(enum Pocket, struct ItemSlot, struct ItemSlot))
{
    struct ItemSlot item_i, item_j;
    u32 i = iLeft, j = iRight;
    for (u32 k = iLeft; k < iEnd; k++)
    {
        item_i = BagPocket_GetSlotData(pocket, i);
        item_j = BagPocket_GetSlotData(pocket, j);
        if (i < iRight && (j >= iEnd || comparator(pocket->id, item_i, item_j) < 0))
        {
            dummySlots[k] = item_i;
            i++;
        }
        else
        {
            dummySlots[k] = item_j;
            j++;
        }
    }
}

// Source: https://en.wikipedia.org/wiki/Merge_sort#Bottom-up_implementation
static void MergeSort(struct BagPocket *pocket, s32 (*comparator)(enum Pocket, struct ItemSlot, struct ItemSlot))
{
    struct ItemSlot *dummySlots = AllocZeroed(sizeof(struct ItemSlot) * pocket->capacity);

    u32 usedCapacity = 0;
    for (u32 i = 0; i < pocket->capacity; i++)
    {
        if (BagPocket_GetSlotData(pocket, i).itemId != ITEM_NONE)
            usedCapacity = i + 1;
    }

    for (u32 width = 1; width < usedCapacity; width *= 2)
    {
        for (u32 i = 0; i < usedCapacity; i += 2 * width)
            Merge(pocket, i, min(i + width, usedCapacity), min(i + 2 * width, usedCapacity), dummySlots, comparator);

        for (u32 j = 0; j < usedCapacity; j++)
            BagPocket_SetSlotData(pocket, j, dummySlots[j]);
    }

    Free(dummySlots);
}

static s32 CompareItemsAlphabetically(enum Pocket pocketId, struct ItemSlot item1, struct ItemSlot item2)
{
    const u8 *name1, *name2;

    if (item1.itemId == ITEM_NONE)
        return 1;
    else if (item2.itemId == ITEM_NONE)
        return -1;

    if (pocketId == POCKET_TM_HM)
    {
        name1 = GetMoveName(GetTMHMMoveId(GetItemTMHMIndex(item1.itemId)));
        name2 = GetMoveName(GetTMHMMoveId(GetItemTMHMIndex(item2.itemId)));
    }
    else
    {
        name1 = GetItemName(item1.itemId);
        name2 = GetItemName(item2.itemId);
    }

    return StringCompare(name1, name2);
}

static s32 CompareItemsByMost(enum Pocket pocketId, struct ItemSlot item1, struct ItemSlot item2)
{
    if (item1.itemId == ITEM_NONE)
        return 1;
    else if (item2.itemId == ITEM_NONE)
        return -1;

    if (item1.quantity < item2.quantity)
        return 1;
    else if (item1.quantity > item2.quantity)
        return -1;

    return CompareItemsAlphabetically(pocketId, item1, item2); // Items have same quantity so sort alphabetically
}

static s32 CompareItemsByType(enum Pocket pocketId, struct ItemSlot item1, struct ItemSlot item2)
{
    if (item1.itemId == ITEM_NONE)
        return 1;
    else if (item2.itemId == ITEM_NONE)
        return -1;

    enum ItemSortType type1 = gItemsInfo[item1.itemId].sortType;
    enum ItemSortType type2 = gItemsInfo[item2.itemId].sortType;

    // Uncategorized items go last.
    if (type1 != ITEM_TYPE_UNCATEGORIZED && type2 == ITEM_TYPE_UNCATEGORIZED)
        return -1;
    else if (type2 != ITEM_TYPE_UNCATEGORIZED && type1 == ITEM_TYPE_UNCATEGORIZED)
        return 1;
    else if (type1 < type2)
        return -1;
    else if (type1 > type2)
        return 1;

    return CompareItemsAlphabetically(pocketId, item1, item2); // Items are of same type so sort alphabetically
}

static s32 CompareItemsByIndex(enum Pocket pocketId, struct ItemSlot item1, struct ItemSlot item2)
{
    u16 index1 = 0, index2 = 0;

    if (item1.itemId == ITEM_NONE)
        return 1;
    else if (item2.itemId == ITEM_NONE)
        return -1;

    switch (pocketId)
    {
    case POCKET_TM_HM:
        index1 = GetItemTMHMIndex(item1.itemId);
        index2 = GetItemTMHMIndex(item2.itemId);
        break;
    case POCKET_BERRIES: // To do - requires #7305
        index1 = item1.itemId;
        index2 = item2.itemId;
        break;
    default:
        return 0;
    }

    if (index1 < index2)
        return -1;
    else if (index1 > index2)
        return 1;

    return 0; // Cannot have multiple stacks of indexed items
}
