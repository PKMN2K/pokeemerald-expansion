#include "global.h"
#include "malloc.h"
#include "battle.h"
#include "bg.h"
#include "chooseboxmon.h"
#include "data.h"
#include "decompress.h"
#include "dma3.h"
#include "dynamic_placeholder_text_util.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "field_screen_effect.h"
#include "field_weather.h"
#include "fldeff_misc.h"
#include "gpu_regs.h"
#include "graphics.h"
#include "international_string_util.h"
#include "item.h"
#include "item_icon.h"
#include "item_menu.h"
#include "list_menu.h"
#include "mail.h"
#include "main.h"
#include "menu.h"
#include "mon_markings.h"
#include "naming_screen.h"
#include "overworld.h"
#include "palette.h"
#include "party_menu.h"
#include "pc_screen_effect.h"
#include "pokemon.h"
#include "pokemon_icon.h"
#include "pokemon_summary_screen.h"
#include "pokemon_storage_system.h"
#include "script.h"
#include "sound.h"
#include "string_util.h"
#include "strings.h"
#include "text.h"
#include "text_window.h"
#include "trig.h"
#include "walda_phrase.h"
#include "window.h"
#include "constants/form_change_types.h"
#include "constants/items.h"
#include "constants/party_menu.h"
#include "constants/moves.h"
#include "constants/rgb.h"
#include "constants/songs.h"
#include "constants/pokemon_icon.h"
#include "swsh_storage_system.h"

/*
    NOTE: This file is large. Some general groups of functions have
          been labeled with commented headers to make navigation easier.
          Search for "SECTION:" to locate them. These sections are not
          hard and fast rules, but give a basic idea of where certain
          types of functions are likely located.
*/

// PC main menu options
enum {
#if OW_PC_MOVE_ORDER <= GEN_3
    OPTION_WITHDRAW,
    OPTION_DEPOSIT,
    OPTION_MOVE_MONS,
#elif OW_PC_MOVE_ORDER >= GEN_4 && OW_PC_MOVE_ORDER <= GEN_6_XY
    OPTION_DEPOSIT,
    OPTION_WITHDRAW,
    OPTION_MOVE_MONS,
#elif OW_PC_MOVE_ORDER >= GEN_7
    OPTION_MOVE_MONS,
    OPTION_DEPOSIT,
    OPTION_WITHDRAW,
#endif
    OPTION_MOVE_ITEMS,
    OPTION_EXIT,
    OPTIONS_COUNT,
    OPTION_SELECT_MON,
};

// IDs for messages to print with PrintMessage
enum {
    MSG_BOX_IS_FULL,
    MSG_RELEASE_POKE,
    MSG_WAS_RELEASED,
    MSG_BYE_BYE,
    MSG_LAST_POKE,
    MSG_PARTY_FULL,
    MSG_HOLDING_POKE,
    MSG_WHICH_ONE_WILL_TAKE,
    MSG_CANT_RELEASE_EGG,
    MSG_CONTINUE_BOX,
    MSG_CAME_BACK,
    MSG_WORRIED,
    MSG_SURPRISE,
    MSG_PLEASE_REMOVE_MAIL,
    MSG_PLACED_IN_BAG,
    MSG_BAG_FULL,
    MSG_PUT_IN_BAG,
    MSG_CANT_STORE_MAIL,
};

// IDs for how to resolve variables in the above messages
enum {
    MSG_VAR_NONE,
    MSG_VAR_MON_NAME_1,
    MSG_VAR_MON_NAME_2, // Unused
    MSG_VAR_MON_NAME_3, // Unused
    MSG_VAR_RELEASE_MON_1,
    MSG_VAR_RELEASE_MON_2, // Unused
    MSG_VAR_RELEASE_MON_3,
    MSG_VAR_ITEM_NAME,
};

// IDs for menu selection items. See SetMenuText, HandleMenuInput, etc
enum {
    MENU_CANCEL,
    MENU_STORE,
    MENU_WITHDRAW,
    MENU_MOVE,
    MENU_SHIFT,
    MENU_PLACE,
    MENU_SUMMARY,
    MENU_RELEASE,
    MENU_MARK,
    MENU_JUMP,
    MENU_WALLPAPER,
    MENU_NAME,
    MENU_TAKE,
    MENU_GIVE,
    MENU_GIVE_2,
    MENU_SWITCH,
    MENU_BAG,
    MENU_SELECT,
	//Wallpapers Page 1
    MENU_BASE,			//Default
    MENU_PLAINS, 		//Plains (Grass / Meadow)
    MENU_CITY,			//City
    MENU_DESERT, 		//Desert
    MENU_CENTER, 		//PokeCenter
	//Wallpapers Page 2
    MENU_SHORE,			//Surface Water
    MENU_OCEAN,		 	//Underwater)
    MENU_MOUNTAIN, 		//Mountain
    MENU_VOLCANO,		//Volcano
    MENU_CAVE, 			//Cave
	//Wallpapers Page 3
    MENU_BEACH,			//Beach
    MENU_SNOW, 			//Snow
    MENU_SKY, 			//Sky
    MENU_COMPUTA, 		//PC
    MENU_CUTE, 			//Cute (Cross Stitch)
	// Wallpapers 16-24
    MENU_SPACE,
    MENU_DAYCARE,
    MENU_CONTEST,
    MENU_CLASSIC,
    MENU_CLASSIC2,
    MENU_SPECIAL_5,
    MENU_SPECIAL_6,
    MENU_SPECIAL_7,
    MENU_SPECIAL_8,
    MENU_COUNT,
};

#define SPECIES_MASK 0x3FFF

// Return IDs for input handlers
enum {
    INPUT_NONE,
    INPUT_MOVE_CURSOR,
    INPUT_2, // Unused
    INPUT_3, // Unused
    INPUT_HIDE_PARTY,
    INPUT_BOX_OPTIONS,
    INPUT_IN_MENU,
    INPUT_SCROLL_RIGHT,
    INPUT_SCROLL_LEFT,
    INPUT_DEPOSIT,
    INPUT_WITHDRAW,
    INPUT_MOVE_MON,
    INPUT_SHIFT_MON,
    INPUT_PLACE_MON,
    INPUT_TAKE_ITEM,
    INPUT_GIVE_ITEM,
    INPUT_SWITCH_ITEMS,
    INPUT_PRESSED_B,
    INPUT_MULTIMOVE_START,
    INPUT_MULTIMOVE_CHANGE_SELECTION,
    INPUT_MULTIMOVE_SINGLE,
    INPUT_MULTIMOVE_GRAB_SELECTION,
    INPUT_MULTIMOVE_UNABLE,
    INPUT_MULTIMOVE_MOVE_MONS,
    INPUT_MULTIMOVE_PLACE_MONS,
    INPUT_TOGGLE_MON_INFO,
    INPUT_SELECT_MON,
};

enum {
    SCREEN_CHANGE_EXIT_BOX,
    SCREEN_CHANGE_SUMMARY_SCREEN,
    SCREEN_CHANGE_NAME_BOX,
    SCREEN_CHANGE_ITEM_FROM_BAG,
};

enum {
    MODE_PARTY,
    MODE_BOX,
    MODE_MOVE,
};

enum {
    CURSOR_AREA_IN_BOX,
    CURSOR_AREA_IN_PARTY,
    CURSOR_AREA_BOX_TITLE,
};
#define CURSOR_AREA_IN_HAND CURSOR_AREA_BOX_TITLE // Alt name for cursor area used by Move Items

enum {
    CURSOR_ANIM_BOUNCE,
    CURSOR_ANIM_MAIN,
};

// Special box ids for the choose box menu
#define BOXID_NONE_CHOSEN 200
#define BOXID_CANCELED    201

#define HGSS_CHOOSE_BOX_NAV_LEFT_NORMAL   0
#define HGSS_CHOOSE_BOX_NAV_LEFT_PRESSED  1
#define HGSS_CHOOSE_BOX_NAV_RIGHT_NORMAL  2
#define HGSS_CHOOSE_BOX_NAV_RIGHT_PRESSED 3

enum {
    PALTAG_MON_ICON_0 = POKE_ICON_BASE_PAL_TAG,
    PALTAG_MON_ICON_1, // Used implicitly in CreateMonIconSprite
    PALTAG_MON_ICON_2, // Used implicitly in CreateMonIconSprite
    PALTAG_MON_ICON_3, // Used implicitly in CreateMonIconSprite
    PALTAG_MON_ICON_4, // Used implicitly in CreateMonIconSprite
    PALTAG_MON_ICON_5, // Used implicitly in CreateMonIconSprite
    PALTAG_MISC_1,
    PALTAG_MISC_2,
    PALTAG_MISC_3,
    PALTAG_TYPE_ICON,
    PALTAG_MARKING_COMBO,
    PALTAG_ITEM_ICON_0,
    PALTAG_ITEM_ICON_1, // Used implicitly in CreateItemIconSprites
    PALTAG_ITEM_ICON_2, // Used implicitly in CreateItemIconSprites
    PALTAG_HGSS_CHOOSE_BOX_NAV,
    PALTAG_HGSS_WALLPAPER_SELECTOR,
    PALTAG_HGSS_STORAGE_CURSOR,
    PALTAG_HGSS_GENDER_GLYPHS,
    PALTAG_HGSS_SHINY_STAR,
    PALTAG_HGSS_POKERUS_SYMBOL,
    PALTAG_HGSS_BOX_THUMBNAIL,
    PALTAG_HGSS_BOX_THUMBNAIL_MARKERS,
};

enum {
    GFXTAG_CURSOR,
    GFXTAG_GENDER_ICON,
    GFXTAG_TYPE_ICON,
    GFXTAG_SHINY_ICON,
    GFXTAG_STAT_LABELS,
    GFXTAG_BOX_TITLE,
    GFXTAG_ITEM_ICON_0,
    GFXTAG_ITEM_ICON_1, // Used implicitly in CreateItemIconSprites
    GFXTAG_ITEM_ICON_2, // Used implicitly in CreateItemIconSprites
    GFXTAG_BOX_SELECTION_PER_30,
    GFXTAG_MARKING_COMBO,
    GFXTAG_MON_ICON,
	GFXTAG_PKRS_ICON,
    GFXTAG_HGSS_CHOOSE_BOX_NAV,
    GFXTAG_HGSS_WALLPAPER_SELECTOR,
    GFXTAG_HGSS_BOX_THUMBNAIL,
    GFXTAG_HGSS_BOX_THUMBNAIL_MARKERS,
};

// The maximum number of Pokémon icons that can appear on-screen.
// By default the limit is 40 (though in practice only 37 can be).
#define MAX_MON_ICONS max(IN_BOX_COUNT + PARTY_SIZE + 1, 40)

// The maximum number of item icons that can appear on-screen while
// moving held items. 1 in the cursor, and 2 more while switching
// between 2 Pokémon with held items
#define MAX_ITEM_ICONS 3

// IDs for the item icons affine anims
enum {
    ITEM_ANIM_NONE,
    ITEM_ANIM_APPEAR,
    ITEM_ANIM_DISAPPEAR,
    ITEM_ANIM_PICK_UP,
    ITEM_ANIM_PUT_DOWN,
    ITEM_ANIM_PUT_AWAY,
    ITEM_ANIM_LARGE,
};

// IDs for the item icon sprite callbacks
enum {
    ITEM_CB_WAIT_ANIM,
    ITEM_CB_TO_HAND,
    ITEM_CB_TO_MON,
    ITEM_CB_SWAP_TO_HAND,
    ITEM_CB_SWAP_TO_MON,
    ITEM_CB_UNUSED_1,
    ITEM_CB_UNUSED_2,
    ITEM_CB_HIDE_PARTY,
};

enum {
    RELEASE_ANIM_RELEASE,
    RELEASE_ANIM_CAME_BACK,
};

// IDs for InitMonPlaceChange
enum {
    CHANGE_GRAB,
    CHANGE_PLACE,
    CHANGE_SHIFT,
};

// Cursor interaction modes, cycled by pressing SELECT
enum {
    CURSOR_MODE_NORMAL,        // Default - show menu on A press
    CURSOR_MODE_AUTO_ACTION,   // Quick single-mon actions (no menu)
    CURSOR_MODE_MULTI_MOVE,    // Multi-selection mode
};

// Modes for selecting and moving Pokémon in the box.
// "MULTIPLE" mode allows up to an entire box to be
// picked up at once by pressing Select then holding
// down the A button. While holding A down, the player
// may move the cursor around to select multiple Pokémon.
// This is MOVE_MODE_MULTIPLE_SELECTING. After releasing A
// those Pokémon will be picked up and can be moved around
// as a single unit. This is MOVE_MODE_MULTIPLE_MOVING
enum {
    MOVE_MODE_NORMAL,
    MOVE_MODE_MULTIPLE_SELECTING,
    MOVE_MODE_MULTIPLE_MOVING,
};

// IDs for the main functions for moving multiple Pokémon.
// Given as arguments to MultiMove_SetFunction
enum {
    MULTIMOVE_START,
    MULTIMOVE_CANCEL, // If only 1 Pokémon is grabbed
    MULTIMOVE_CHANGE_SELECTION,
    MULTIMOVE_GRAB_SELECTION,
    MULTIMOVE_MOVE_MONS,
    MULTIMOVE_PLACE_MONS,
};

// Window IDs for sWindowTemplates
enum {
    WIN_MESSAGE,
    WIN_ITEM_DESC,
    WIN_MON_INFO_NICKNAME_LEFT,
    WIN_MON_INFO_LEVEL_LEFT,
    WIN_MON_INFO_STATS_COL1_LEFT,
    WIN_MON_INFO_STATS_COL2_LEFT,
    WIN_MON_INFO_ABILITY_LEFT,
    WIN_MON_INFO_ITEM_LEFT,
    WIN_MON_INFO_NICKNAME_RIGHT,
    WIN_MON_INFO_LEVEL_RIGHT,
    WIN_MON_INFO_STATS_COL1_RIGHT,
    WIN_MON_INFO_STATS_COL2_RIGHT,
    WIN_MON_INFO_ABILITY_RIGHT,
    WIN_MON_INFO_ITEM_RIGHT,
};

struct StorageMenu
{
    const u8 *text;
    int textId;
};

struct ChooseBoxMenu
{
    struct Sprite *monCountSprite;
    struct Sprite *navSprites[2];
    struct Sprite *overviewSprites[6];
    struct Sprite *overviewMarkerSprites[6];
    u8 curBox;
    u8 overviewGroupBase;
    bool8 active;
    bool8 savedCursorInvisible;
    u8 ALIGNED(4) monCountTiles[256];
    u8 ALIGNED(4) navTiles[4 * 32 * 32 / 2];
    u8 ALIGNED(4) overviewBaseTiles[6 * 32 * 32 / 2];
    // All native occupancy pixels fit in the centered 16x16 region of each
    // 32x32 thumbnail (base-local x=8..23, y=8..23).
    u8 ALIGNED(4) overviewMarkerTiles[6 * 16 * 16 / 2];
    u16 ALIGNED(4) overviewBasePal[16];
    u16 ALIGNED(4) overviewMarkerPal[16];
};

struct ItemIcon
{
    struct Sprite *sprite;
    u8 *tiles;
    u16 palIndex;
    u8 area;
    u8 pos;
    bool8 active;
};

struct DisplayMonInfo
{
    u8 nickname[POKEMON_NAME_LENGTH + 1];
    enum Species species;
    u8 level;
    u8 gender;
    enum Type types[2];
    bool32 isShiny;
    bool8 isEgg;
    u16 maxHP;
    u16 atk;
    u16 def;
    u16 spatk;
    u16 spdef;
    u16 speed;
    enum Ability ability;
    enum Item heldItem;
    u8 markings;
    u32 personality;
    const u16 *palette;
    u8 nicknameText[36];
    u8 speciesNameText[36];
    u8 genderLvlText[36];
    u8 itemNameText[36];
	bool32 hasPokerus;
};

struct HgssMarkingsMenu
{
    u8 markings;
    s8 cursorPos;
    bool8 markingsArray[NUM_MON_MARKINGS];
    bool8 savedStorageCursorInvisible;
    struct Sprite *cursorSprite;
};

struct PokemonStorageSystemData
{
    u8 state;
    u8 boxOption;
    u8 screenChangeType;
    bool8 isReopening;
    u8 taskId;
    u16 partyMenuTilemapBuffer[0x108];
    s16 newCurrBoxId;
    s16 scrollSpeed;
    u16 scrollTimer;
    u8 ALIGNED(2) boxTitleTiles[1024];
    u8 boxTitleCycleId;
    u8 wallpaperLoadBoxId;
    s8 wallpaperLoadDir;
    u16 boxTitlePal[16];
    u16 boxTitlePalOffset;
    u16 boxTitleAltPalOffset;
    struct Sprite *curBoxTitleSprites[2];
    struct Sprite *nextBoxTitleSprites[2];
    u32 wallpaperPalBits;
    s16 wallpaperSetId;
    s16 wallpaperId;
    u16 wallpaperTilemap[360];
    u8 wallpaperChangeState;
    u8 scrollState;
    u8 scrollToBoxId;
    s8 scrollDirection;
    struct Sprite *movingMonSprite;
    struct Sprite *partySprites[PARTY_SIZE];
    struct Sprite *boxMonsSprites[IN_BOX_COUNT];
    struct Sprite **shiftMonSpritePtr;
    struct Sprite **releaseMonSpritePtr;
    u16 numIconsPerSpecies[MAX_MON_ICONS];
    enum Species iconSpeciesList[MAX_MON_ICONS];
    enum Species boxSpecies[IN_BOX_COUNT];
    u32 boxPersonalities[IN_BOX_COUNT];
    bool8 boxIsEgg[IN_BOX_COUNT];
    u8 incomingBoxId;
    u8 shiftTimer;
    u8 numPartyToCompact;
    u16 iconScrollDistance;
    s16 iconScrollPos;
    s16 iconScrollSpeed;
    u16 iconScrollNumIncoming;
    u8 iconScrollCurColumn;
    s8 iconScrollDirection; // Unnecessary duplicate of scrollDirection
    u8 iconScrollState;
    struct WindowTemplate menuWindow;
    struct StorageMenu menuItems[20];
    struct ListMenuTemplate listMenuTemplate;
    u16 listMenuScrollRow;
    u16 listMenuSelectedRow;
    u8 listMenuTaskId;
    u8 menuItemsCount;
    u8 menuWidth;
    u16 menuWindowId;
    struct Sprite *wallpaperSelectorSprites[4];
    u8 ALIGNED(4) wallpaperSelectorTiles[4 * 32 * 32 / 2];
    struct Sprite *cursorSprite;
    s32 cursorNewX;
    s32 cursorNewY;
    u32 cursorSpeedX;
    u32 cursorSpeedY;
    s16 cursorTargetX;
    s16 cursorTargetY;
    u16 cursorMoveSteps;
    s8 cursorVerticalWrap;
    s8 cursorHorizontalWrap;
    u8 newCursorArea;
    u8 newCursorPosition;
    u8 cursorPrevHorizPos;
    u8 cursorFlipTimer;
    u8 cursorPalNums[3];
    struct DisplayMonInfo displayMon;
    u16 displayUnusedVar;
    bool8 setMosaic;
    bool8 (*monPlaceChangeFunc)(void);
    u8 monPlaceChangeState;
    u8 shiftBoxId;
    struct Sprite *markingComboSprite;
    u16 *markingComboTilesPtr;
    struct HgssMarkingsMenu markMenuHgss;
    struct ChooseBoxMenu chooseBoxMenu;
    struct Pokemon movingMon;
    struct Pokemon tempMon;
    s8 canReleaseMon;
    bool8 releaseStatusResolved;
    s8 releaseCheckBoxId;
    s8 releaseCheckBoxPos;
    s8 releaseBoxId;
    s8 releaseBoxPos;
    u16 releaseCheckState;
    u16 restrictedReleaseMonMoves;
    u16 restrictedMoveList[8];
    u8 summaryMaxPos;
    u8 summaryStartPos;
    u8 summaryScreenMode;
    union
    {
        struct Pokemon *mon;
        struct BoxPokemon *box;
    } summaryMon;
    u8 messageText[40];
    u8 releaseMonName[POKEMON_NAME_LENGTH + 1];
    u8 itemName[20];
    u8 inBoxMovingMode;
    u16 multiMoveWindowId;
    struct ItemIcon itemIcons[MAX_ITEM_ICONS];
    u16 movingItemId;
    struct Sprite *genderIconSprite;
    struct Sprite *typeIconSprites[2];
    struct Sprite *shinyIconSprite;
    struct Sprite *pokerusIconSprite;
    struct Sprite *statLabelSprites[2];
    u16 *typeIconTilesPtr[2];
    u8 ALIGNED(4) tileBuffer[MON_PIC_SIZE * MAX_MON_PIC_FRAMES];
    u8 ALIGNED(4) itemIconBuffer[0x800];
    u16 boxOverlayTilemapBuffer[0x400];
    u16 hgssWallpaperTilemapBuffer[0x400];
    u16 ALIGNED(4) displayMenuTilemapBuffer[0x400];
    u16 infoTilemapBuffer[0x800];
    u16 bg0_Y;
    bool8 showMonInfo;
    bool8 inMenuInteraction;
    u8 monInfoTilemapId;
    u8 displayMonInfoLoadState;
    u8 graphicsLoadState;
};

struct HgssStorageBgAsset
{
    const u8 *tiles;
    const u16 *tilemap;
    const u16 *palette;
    u16 tileCount;
    u8 widthTiles;
    u8 heightTiles;
    u8 paletteBankCount;
};

#include "data/swsh_storage_system.h"

static u32 sItemIconGfxBuffer[98];

EWRAM_DATA static u8 sPreviousBoxOption = 0;
EWRAM_DATA static struct ChooseBoxMenu *sChooseBoxMenu = NULL;
// Two authentic HGSS 32x32 cursor frames, repacked from the verified 64x32
// source strip into contiguous GBA OBJ cells at runtime.
EWRAM_DATA static u32 sHgssStorageCursorTiles[(2 * 32 * 32 / 2) / sizeof(u32)] = {0};
EWRAM_DATA static struct PokemonStorageSystemData *sStorage = NULL;
EWRAM_DATA static u8 sCurrentBoxOption = 0;
EWRAM_DATA static u8 sWhichToReshow = 0;
EWRAM_DATA static u8 sLastUsedBox = 0;
EWRAM_DATA static u16 sMovingItemId = 0;
EWRAM_DATA static struct Pokemon sSavedMovingMon = {0};
EWRAM_DATA static s8 sCursorArea = 0;
EWRAM_DATA static s8 sCursorPosition = 0;
EWRAM_DATA static u8 sLastBoxColumn = 0;
EWRAM_DATA static bool8 sIsMonBeingMoved = 0;
EWRAM_DATA static u8 sMovingMonOrigBoxId = 0;
EWRAM_DATA static u8 sMovingMonOrigBoxPos = 0;
EWRAM_DATA static u8 sCursorMode = 0;
EWRAM_DATA static bool8 sJustOpenedBag = 0;
EWRAM_DATA static bool8 sRefreshDisplayMonGfx = FALSE;
EWRAM_DATA static struct HgssMarkingsMenu *sMarkMenu = NULL;
EWRAM_DATA static u8 sHgssYesNoWindowId = 0;

#define HGSS_WALLPAPER_SELECTOR_ITEMS_PER_PAGE 4

// Main tasks
static void Task_InitPokeStorage(u8);
static void Task_PlaceMon(u8);
static void Task_ChangeScreen(u8);
static void Task_ShowPokeStorage(u8);
static void Task_OnBPressed(u8);
static void Task_HandleBoxOptions(u8);
static void Task_OnSelectedMon(u8);
static void Task_HidePartyPokemon(u8);
static void Task_DepositMon(u8);
static void Task_MoveMon(u8);
static void Task_GiveMovingItemToMon(u8);
static void Task_SwitchSelectedItem(u8);
static void Task_TakeItemForMoving(u8);
static void Task_WithdrawMon(u8);
static void Task_ShiftMon(u8);
static void Task_GiveItemFromBag(u8);
static void Task_ItemToBag(u8);
static void Task_TakeItemForMoving(u8);
static void Task_ShowMarkMenu(u8);
static void Task_ShowMonSummary(u8);
static void Task_ReleaseMon(u8);
static void Task_ReshowPokeStorage(u8);
static void Task_PokeStorageMain(u8);
static void Task_JumpBox(u8);
static void Task_HandleWallpapers(u8);
static void Task_NameBox(u8);
static void Task_PrintCantStoreMail(u8);
static void Task_HandleMovingMonFromParty(u8);
static void Task_WallpaperChange(u8);

// Input handlers
static u8 InBoxInput_Normal(void);
static u8 InBoxInput_MovingMultiple(void);
static u8 InBoxInput_SelectingMultiple(void);
static u8 HandleInput(void);
static void AddBoxOptionsMenu(void);
static u8 SetSelectionMenuTexts(void);
static bool8 SetMenuTexts_Mon(void);
static bool8 SetMenuTexts_Item(void);

// Choose box menu
static void ChooseBoxMenu_CreateSprites(u8);
static void ChooseBoxMenu_DestroySprites(void);
static void ChooseBoxMenu_MoveCursor(s8);
static void ChooseBoxMenu_UpdateHover(void);
static void ChooseBoxMenu_UpdateOverviewGroup(void);
static void ChooseBoxMenu_PrintInfo(void);

// Options menus
static void InitMenu(void);
static void SetMenuText(u8);
static s8 GetMenuItemTextId(u8);
static void AddMenu(void);
static bool8 IsMenuLoading(void);
static s16 HandleMenuInput(void);
static void RemoveMenu(void);
static void LoadHgssWallpaperSelectorSprites(void);
static void FreeHgssWallpaperSelectorSprites(void);

// Pokémon sprites
static void InitMonIconFields(void);
static void SpriteCB_BoxMonIconScrollOut(struct Sprite *);
static void GetIncomingBoxMonData(u8);
static void CreatePartyMonsSprites(bool8);
static void CompactPartySprites(void);
static u8 GetNumPartySpritesCompacting(void);
static void MovePartySpriteToNextSlot(struct Sprite *, u16);
static void SpriteCB_MovePartyMonToNextSlot(struct Sprite *);
static void ReshowReleaseMon(void);
static bool8 ResetReleaseMonSpritePtr(void);
static void SetMovingMonPriority(u8);
static void SpriteCB_HeldMon(struct Sprite *);
static void SpriteCB_HeldMonInChooseBox(struct Sprite *);
static struct Sprite *CreateMonIconSprite(enum Species species, u32 personality, s16 x, s16 y, u8 oamPriority, u8 subpriority, bool32 isEgg);
static void DestroyBoxMonIcon(struct Sprite *);

// Pokémon data
static void MoveMon(void);
static void PlaceMon(void);
static void RefreshDisplayMon(void);
static void SetMovingMonData(u8, u8);
static void SetPlacedMonData(u8, u8);
static void PurgeMonOrBoxMon(u8, u8);
static void SetShiftedMonData(u8, u8);
static void InitReleaseMon(void);
static bool8 TryHideReleaseMon(void);
static void InitCanReleaseMonVars(void);
static void ReleaseMon(void);
static bool32 AtLeastThreeUsableMons(void);
static s8 RunCanReleaseMon(void);
static void SaveMovingMon(void);
static void LoadSavedMovingMon(void);
static void InitSummaryScreenData(void);
static void SetSelectionAfterSummaryScreen(void);
static void SetMonMarkings(u8);
static bool8 IsRemovingLastPartyMon(void);
static bool8 CanPlaceMon(void);
static bool8 CanShiftMon(void);
static bool8 IsMonBeingMoved(void);
static void TryRefreshDisplayMon(void);
static void ReshowDisplayMon(void);
static void SetDisplayMonData(void *, u8);
static u16 GetSpeciesAtCursorPosition(void);

// Moving multiple Pokémon at once
static void MultiMove_Free(void);
static bool8 MultiMove_Init(void);
static bool8 MultiMove_RunFunction(void);
static bool8 MultiMove_TryMoveGroup(u8);
static bool8 MultiMove_CanPlaceSelection(void);
static void MultiMove_SetFunction(u8);
static u8 MultiMove_GetOrigin(void);
static bool8 MultiMove_Start(void);
static bool8 MultiMove_Cancel(void);
static bool8 MultiMove_ChangeSelection(void);
static bool8 MultiMove_GrabSelection(void);
static bool8 MultiMove_MoveMons(void);
static bool8 MultiMove_PlaceMons(void);
static void MultiMove_SetIconToBg(u8, u8);
static void MultiMove_ClearIconFromBg(u8, u8);
static void MultiMove_ResetBg(void);
static void MultiMove_UpdateSelectedIcons(void);
static void MultiMove_InitMove(u16, u16, u16);
static void MultiMove_GetMonsFromSelection(void);
static void MultiMove_RemoveMonsFromBox(void);
static void MultiMove_CreatePlacedMonIcons(void);
static void MultiMove_SetPlacedMonData(void);
static u8 MultiMove_UpdateMove(void);
static void MultiMove_DeselectRow(u8, u8, u8);
static void MultiMove_SelectRow(u8, u8, u8);
static void MultiMove_SelectColumn(u8, u8, u8);
static void MultiMove_DeselectColumn(u8, u8, u8);

// Move Items mode
static bool32 IsItemIconAtPosition(u8, u8);
static u8 GetNewItemIconIdx(void);
static void SetItemIconPosition(u8, u8, u8);
static void LoadItemIconGfx(u8 id, const u32 *itemTiles, const u16 *itemPal);
static void SetItemIconAffineAnim(u8, u8);
static void SetItemIconActive(u8, bool8);
static u8 GetItemIconIdxByPosition(u8, u8);
static void CreateItemIconSprites(void);
static void TryLoadItemIconAtPos(u8, u8);
static void TryHideItemIconAtPos(u8, u8);
static void TakeItemFromMon(u8, u8);
static void InitItemIconInCursor(enum Item);
static void SwapItemsWithMon(u8, u8);
static void GiveItemToMon(u8, u8);
static void MoveItemFromMonToBag(u8, u8);
static void MoveItemFromCursorToBag(void);
static void MoveHeldItemWithPartyMenu(void);
static bool8 IsItemIconAnimActive(void);
static bool8 IsMovingItem(void);
static const u8 *GetMovingItemName(void);
static u16 GetMovingItemId(void);
static void SetItemIconCallback(u8, u8, u8, u8);
static void SpriteCB_ItemIcon_SetPosToCursor(struct Sprite *);
static void SpriteCB_ItemIcon_WaitAnim(struct Sprite *);
static void SpriteCB_ItemIcon_ToHand(struct Sprite *);
static void SpriteCB_ItemIcon_ToMon(struct Sprite *);
static void SpriteCB_ItemIcon_SwapToHand(struct Sprite *);
static void SpriteCB_ItemIcon_HideParty(struct Sprite *);
static void SpriteCB_ItemIcon_SwapToMon(struct Sprite *);

// Cursor
static void LoadHgssStorageCursorGfx(void);
static void CreateCursorSprites(void);
static void ToggleCursorAutoAction(void);
static u8 GetCursorPosition(void);
static void StartCursorAnim(u8);
static void TryHideItemAtCursor(void);
static void TryShowItemAtCursor(void);
static void InitCursor(void);
static void InitCursorOnReopen(void);
static void GetCursorCoordsByPos(u8, u8, u16 *, u16 *);
static bool8 UpdateCursorPos(void);
static void InitNewCursorPos(u8, u8);
static void InitCursorMove(void);
static void DoCursorNewPosUpdate(void);
static void SetCursorInParty(void);
static void SetCursorBoxPosition(u8);
static void SetCursorPosition(u8, u8);
static void ClearSavedCursorPos(void);
static void SaveCursorPos(void);
static u8 GetSavedCursorPos(void);
static void InitMonPlaceChange(u8);
static bool8 DoMonPlaceChange(void);
static bool8 MonPlaceChange_Shift(void);
static bool8 MonPlaceChange_Grab(void);
static bool8 MonPlaceChange_Place(void);
static bool8 MultiMonPlaceChange_Up(void);
static bool8 MultiMonPlaceChange_Down(void);
static bool8 MonPlaceChange_CursorDown(void);
static bool8 MonPlaceChange_CursorUp(void);
static void TrySetCursorFistAnim(void);
static bool8 IsCursorOnBoxTitle(void);

// Box name
static void UpdateBoxTitle(u8);
static void UpdateBoxTitlePalette(void);

// Box title
static void InitBoxTitle(u8);
static void RenderBoxTitleCentered(const u8 *);

// Wallpaper
static void SetWallpaperForCurrentBox(u8);
static bool8 DoWallpaperGfxChange(void);
static void LoadWallpaperGfx(u8, s8);
static bool32 WaitForWallpaperGfxLoad(void);
static void AddWallpaperMenu(void);

static u8 GetBoxWallpaper(u8);
static void SetBoxWallpaper(u8, u8);

// General box
static void CreateInitBoxTask(u8);
static bool8 IsInitBoxActive(void);
static bool32 LoadHgssStorageBgAsset(const struct HgssStorageBgAsset *, u8, u16, u8, u16 *, u8, u8);
static void DrawHgssStorageMainFrame(void);
static void DrawHgssStoragePartySlots(void);
static void UpdateHgssStorageSlotHighlight(void);
static void Task_InitBox(u8);
static void SetUpScrollToBox(u8);
static bool8 ScrollToBox(void);
static s8 DetermineBoxScrollDirection(u8);
static void SetCurrentBox(u8);
static struct BoxPokemon *GetCursorBoxMon(void);

// Mon info panel
static void ClearMonInfoTilemap(void);
static void UpdateMonInfoTilemap(void);
static void RestoreHgssMonInfoPanel(void);
static void DrawHgssMessageWindow(void);
static void DrawHgssYesNoPanel(void);
static void DrawHgssContextMenuFrame(u8 windowId);
static void ClearHgssContextMenuFrame(u8 windowId);

// Misc
static void CreateMainMenu(u8, s16 *);
static void EnterPokeStorage(u8 boxOption);
static u8 GetCurrentBoxOption(void);
//static void ScrollBackground(void);
static void GiveChosenBagItem(void);
static void LoadPokeStorageMenuGfx(void);
static void InitPokeStorageBg0(void);
static void SetScrollingBackground(void);
static void UpdateBoxToSendMons(void);
static void InitCursorItemIcon(void);
static bool8 InitPalettesAndSprites(void);
static void RefreshDisplayMonData(void);
static void CreateMarkingComboSprite(void);
static void UpdateMarkingComboSprite(void);
static void ClearBottomWindow(void);
static void UpdateGenderIconSprite(u8);
static void UpdateTypeIconTiles(u8, void *);
static void SpriteCB_TypeIcon(struct Sprite *);
static void UpdateTypeIconsSprite(void);
static void UpdateStatLabelsSprites(void);
static void UpdateShinyIconSprite(void);
static void UpdatePokerusIconSprite(void);
static void HideInfoPanelSprites(void);
static void ClearMonInfoPanel(void);
static u16 GetTargetBg0Y(void);
static void PrintDisplayMonStats(u8);
static void PrintDisplayMonAbility(u8);
static void PrintDisplayMonHeldItem(u8);
static void PrintDisplayMonNickname(u8);
static void PrintDisplayMonLevel(u8);
static bool8 PrintDisplayMonInfo(void);
static void FreePokeStorageData(void);
static bool8 InitPokeStorageWindows(void);
static void ShowYesNoWindow(s8);
static s8 ProcessHgssYesNoInput(void);
static void PrintMessage(u8 id);

// Tilemap utility
static void TilemapUtil_Move(u8, u8, s8);
static void TilemapUtil_SetMap(u8, u8, const void *, u16, u16);
static void TilemapUtil_SetPos(u8, u16, u16);
static void TilemapUtil_Update(u8);
static void TilemapUtil_DrawPrev(u8);
static void TilemapUtil_Draw(u8);

// Form changing
void SetMonFormPSS(struct BoxPokemon *boxMon, enum FormChanges method);
void SetMonFormPSS_ItemHold(struct BoxPokemon *boxMon);
void UpdateSpeciesSpritePSS(struct BoxPokemon *boxmon);

//------------------------------------------------------------------------------
//  SECTION: Misc utility
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//  SECTION: Main menu
//
//  The below functions generally handle the PC main menu where the main
//  options can be selected (Withdraw, Deposit, etc.), as well as exiting
//  Pokémon Storage back to this menu.
//------------------------------------------------------------------------------


enum {
    STATE_LOAD,
    STATE_FADE_IN,
    STATE_HANDLE_INPUT,
    STATE_ERROR_MSG,
    STATE_ENTER_PC,
};

#define tState          data[0]
#define tSelectedOption data[1]
#define tInput          data[2]
#define tNextOption     data[3]
#define tWindowId       data[15]

static void Task_PCMainMenu(u8 taskId)
{
    struct Task *task = &gTasks[taskId];

    switch (task->tState)
    {
    case STATE_LOAD:
        CreateMainMenu(task->tSelectedOption, &task->tWindowId);
        LoadMessageBoxAndBorderGfx();
        DrawDialogueFrame(0, FALSE);
        FillWindowPixelBuffer(0, PIXEL_FILL(1));
        AddTextPrinterParameterized4(0, FONT_NORMAL, 0, 1, 0, 0, sTextColors[0], TEXT_SKIP_DRAW, sMainMenuTexts[task->tSelectedOption].desc);
        CopyWindowToVram(0, COPYWIN_FULL);
        CopyWindowToVram(task->tWindowId, COPYWIN_FULL);
        task->tState++;
        break;
    case STATE_FADE_IN:
        if (IsWeatherNotFadingIn())
            task->tState++;
        break;
    case STATE_HANDLE_INPUT:
        task->tInput = Menu_ProcessInput();
        switch (task->tInput)
        {
        case MENU_NOTHING_CHOSEN:
            task->tNextOption = task->tSelectedOption;
            if (JOY_NEW(DPAD_UP) && --task->tNextOption < 0)
                task->tNextOption = OPTIONS_COUNT - 1;
            if (JOY_NEW(DPAD_DOWN) && ++task->tNextOption > OPTIONS_COUNT - 1)
                task->tNextOption = 0;

            if (task->tSelectedOption != task->tNextOption)
            {
                task->tSelectedOption = task->tNextOption;
                FillWindowPixelBuffer(0, PIXEL_FILL(1));
                AddTextPrinterParameterized4(0, FONT_NORMAL, 0, 1, 0, 0, sTextColors[0], 0, sMainMenuTexts[task->tSelectedOption].desc);
            }
            break;
        case MENU_B_PRESSED:
        case OPTION_EXIT:
            ClearStdWindowAndFrame(task->tWindowId, TRUE);
            UnlockPlayerFieldControls();
            ScriptContext_Enable();
            RemoveWindow(task->tWindowId);
            DestroyTask(taskId);
            break;
        default:
            if (task->tInput == OPTION_WITHDRAW && CountPartyMons() == PARTY_SIZE)
            {
                // Can't withdraw
                FillWindowPixelBuffer(0, PIXEL_FILL(1));
                AddTextPrinterParameterized4(0, FONT_NORMAL, 0, 1, 0, 0, sTextColors[0], 0, gText_PartyFull);
                task->tState = STATE_ERROR_MSG;
            }
            else if (task->tInput == OPTION_DEPOSIT && CountPartyMons() == 1)
            {
                // Can't deposit
                FillWindowPixelBuffer(0, PIXEL_FILL(1));
                AddTextPrinterParameterized4(0, FONT_NORMAL, 0, 1, 0, 0, sTextColors[0], 0, gText_JustOnePkmn);
                task->tState = STATE_ERROR_MSG;
            }
            else
            {
                // Enter PC
                FadeScreen(FADE_TO_BLACK, 0);
                task->tState = STATE_ENTER_PC;
            }
            break;
        }
        break;
    case STATE_ERROR_MSG:
        // Printed "can't do PC option message"
        // Wait for new input after message
        if (JOY_NEW(A_BUTTON | B_BUTTON))
        {
            FillWindowPixelBuffer(0, PIXEL_FILL(1));
            AddTextPrinterParameterized4(0, FONT_NORMAL, 0, 1, 0, 0, sTextColors[0], 0, sMainMenuTexts[task->tSelectedOption].desc);
            task->tState = STATE_HANDLE_INPUT;
        }
        else if (JOY_NEW(DPAD_UP))
        {
            if (--task->tSelectedOption < 0)
                task->tSelectedOption = OPTIONS_COUNT - 1;
            Menu_MoveCursor(-1);
            task->tSelectedOption = Menu_GetCursorPos();
            FillWindowPixelBuffer(0, PIXEL_FILL(1));
            AddTextPrinterParameterized4(0, FONT_NORMAL, 0, 1, 0, 0, sTextColors[0], 0, sMainMenuTexts[task->tSelectedOption].desc);
            task->tState = STATE_HANDLE_INPUT;
        }
        else if (JOY_NEW(DPAD_DOWN))
        {
            if (++task->tSelectedOption >= OPTIONS_COUNT - 1)
                task->tSelectedOption = 0;
            Menu_MoveCursor(1);
            task->tSelectedOption = Menu_GetCursorPos();
            FillWindowPixelBuffer(0, PIXEL_FILL(1));
            AddTextPrinterParameterized4(0, FONT_NORMAL, 0, 1, 0, 0, sTextColors[0], 0, sMainMenuTexts[task->tSelectedOption].desc);
            task->tState = STATE_HANDLE_INPUT;
        }
        break;
    case STATE_ENTER_PC:
        if (!gPaletteFade.active)
        {
            CleanupOverworldWindowsAndTilemaps();
            EnterPokeStorage(task->tInput);
            RemoveWindow(task->tWindowId);
            DestroyTask(taskId);
        }
        break;
    }
}

void ShowPokemonStorageSystemPC_SwSh(void)
{
    u8 taskId = CreateTask(Task_PCMainMenu, 80);
    gTasks[taskId].tState = 0;
    gTasks[taskId].tSelectedOption = 0;
    LockPlayerFieldControls();
}

void ShowPokemonPCFromParty_SwSh(void)
{
    EnterPokeStorage(OPTION_MOVE_MONS);
}

static void FieldTask_ReturnToPcMenu(void)
{
    u8 taskId;
    MainCallback vblankCb = gMain.vblankCallback;

    SetVBlankCallback(NULL);
    taskId = CreateTask(Task_PCMainMenu, 80);
    gTasks[taskId].tState = 0;
    gTasks[taskId].tSelectedOption = sPreviousBoxOption;
    Task_PCMainMenu(taskId);
    SetVBlankCallback(vblankCb);
    FadeInFromBlack();
}

#undef tState
#undef tSelectedOption
#undef tInput
#undef tNextOption
#undef tWindowId

static void CreateMainMenu(u8 whichMenu, s16 *windowIdPtr)
{
    s16 windowId;
    struct WindowTemplate template = sWindowTemplate_MainMenu;
    template.width = GetMaxWidthInMenuTable((void *)sMainMenuTexts, OPTIONS_COUNT);
    windowId = AddWindow(&template);

    DrawStdWindowFrame(windowId, FALSE);
    PrintMenuTable(windowId, OPTIONS_COUNT, (void *)sMainMenuTexts);
    InitMenuInUpperLeftCornerNormal(windowId, OPTIONS_COUNT, whichMenu);
    *windowIdPtr = windowId;
}

static void CB2_ExitPokeStorage(void)
{
    sPreviousBoxOption = GetCurrentBoxOption();
    gFieldCallback = FieldTask_ReturnToPcMenu;
    SetMainCallback2(CB2_ReturnToField);
}

//------------------------------------------------------------------------------
//  SECTION: Choose Box menu
//
//  HGSS does not use a standalone all-box grid for this interaction. Keep the
//  authenticated normal box presentation visible and select the destination as
//  a title-level carousel. Left/right changes the candidate box; A confirms and
//  B cancels. Runtime title/count data is updated without replacement chrome.
//------------------------------------------------------------------------------

// HGSS ov14_021F4958 rebuilds each 32x32 overview cell from member 70. It
// replaces source palette index 8 (BGR555 0x03E0) with the box wallpaper color,
// then ov14_021F4A64 stamps one 2x1 body-color marker per occupied slot.
//
// GBA 4bpp OBJ cannot address HGSS's original 8bpp indices 0x20-0x2F in the
// same sprite palette. Preserve the exact pixels/colors by splitting each live
// cell into two coincident 4bpp OBJ layers: member-70 base/wallpaper below and
// the authenticated member-75 marker colors above.
static u8 ChooseBoxMenu_Get4bppPixel(const u8 *tiles, u8 x, u8 y)
{
    u32 tile = (y / 8) * 4 + x / 8;
    u32 offset = tile * TILE_SIZE_4BPP + (y & 7) * 4 + ((x & 7) / 2);
    u8 packed = tiles[offset];

    return (x & 1) ? packed >> 4 : packed & 0xF;
}

static void ChooseBoxMenu_Set4bppPixel(u8 *tiles, u8 widthTiles, u8 x, u8 y, u8 color)
{
    u32 tile = (y / 8) * widthTiles + x / 8;
    u32 offset = tile * TILE_SIZE_4BPP + (y & 7) * 4 + ((x & 7) / 2);

    if (x & 1)
        tiles[offset] = (tiles[offset] & 0x0F) | (color << 4);
    else
        tiles[offset] = (tiles[offset] & 0xF0) | color;
}

static u8 ChooseBoxMenu_FindOrAddColor(u16 *palette, u8 *colorCount, u16 color)
{
    u8 i;

    for (i = 1; i < *colorCount; i++)
    {
        if (palette[i] == color)
            return i;
    }

    // The base uses three fixed visible member-70 colors plus at most six
    // wallpaper colors in a visible HGSS group, so this cannot exhaust 4bpp.
    if (*colorCount >= 16)
        return 0;

    palette[*colorCount] = color;
    return (*colorCount)++;
}

static void ChooseBoxMenu_BuildOverviewGroup(u8 groupBase)
{
    const u8 *sourceTiles = (const u8 *)sHgssBoxThumbnailBase_Gfx;
    u8 baseColorCount = 1;
    u8 frame;
    u8 bodyColor;

    CpuFill32(0, sChooseBoxMenu->overviewBaseTiles, sizeof(sChooseBoxMenu->overviewBaseTiles));
    CpuFill32(0, sChooseBoxMenu->overviewMarkerTiles, sizeof(sChooseBoxMenu->overviewMarkerTiles));
    CpuFill32(0, sChooseBoxMenu->overviewBasePal, sizeof(sChooseBoxMenu->overviewBasePal));
    CpuFill32(0, sChooseBoxMenu->overviewMarkerPal, sizeof(sChooseBoxMenu->overviewMarkerPal));

    for (bodyColor = BODY_COLOR_RED; bodyColor <= BODY_COLOR_PINK; bodyColor++)
        sChooseBoxMenu->overviewMarkerPal[bodyColor + 1] = sHgssBoxThumbnailMarkerColors[bodyColor];

    for (frame = 0; frame < 6; frame++)
    {
        u8 boxId = groupBase + frame;
        u8 *baseFrame = sChooseBoxMenu->overviewBaseTiles + frame * (32 * 32 / 2);
        u8 *markerFrame = sChooseBoxMenu->overviewMarkerTiles + frame * (16 * 16 / 2);
        u8 wallpaperId;
        u16 wallpaperColor;
        u8 x;
        u8 y;
        u8 slot;

        if (boxId >= TOTAL_BOXES_COUNT)
            continue;

        // HGSS ov14_021E7930 folds bonus wallpaper ids 16-23 back to 0-7 for
        // overview thumbnails before ov14_021F46B0 adds the member-75 +0x10
        // palette offset.
        wallpaperId = GetBoxWallpaper(boxId);
        if (wallpaperId >= 16)
            wallpaperId -= 16;
        wallpaperColor = sHgssWallpaperSelectorColors[wallpaperId];

        for (y = 0; y < 32; y++)
        {
            for (x = 0; x < 32; x++)
            {
                u8 sourceIndex = ChooseBoxMenu_Get4bppPixel(sourceTiles, x, y);
                u16 color;
                u8 destIndex;

                // Member-70 source index 0 is transparent in the native OBJ.
                if (sourceIndex == 0)
                    continue;

                color = sHgssBoxThumbnailBase_Pal[sourceIndex];
                if (color == 0x03E0)
                    color = wallpaperColor;

                destIndex = ChooseBoxMenu_FindOrAddColor(
                    sChooseBoxMenu->overviewBasePal,
                    &baseColorCount,
                    color);
                ChooseBoxMenu_Set4bppPixel(baseFrame, 4, x, y, destIndex);
            }
        }

        // ov14_021F4A64 walks 5 rows x 6 columns. Markers are 2x1 pixels at
        // x=10,12,14,16,18,20 and y=11,13,15,17,19.
        for (slot = 0; slot < IN_BOX_COUNT; slot++)
        {
            enum Species species = GetBoxMonDataAt(boxId, slot, MON_DATA_SPECIES);
            bool32 isEgg;
            u8 markerIndex;

            if (species == SPECIES_NONE)
                continue;

            isEgg = GetBoxMonDataAt(boxId, slot, MON_DATA_IS_EGG);
            if (isEgg)
                bodyColor = (species == SPECIES_MANAPHY) ? BODY_COLOR_BLUE : BODY_COLOR_WHITE;
            else
                bodyColor = gSpeciesInfo[species].bodyColor;

            if (bodyColor > BODY_COLOR_PINK)
                bodyColor = BODY_COLOR_WHITE;

            markerIndex = bodyColor + 1;

            // The marker OBJ is centered on the 32x32 base but only stores its
            // occupied 16x16 middle. Local (2,3) therefore lands at native
            // base-local (10,11), preserving ov14_021F4A64 exactly.
            x = 2 + 2 * (slot % IN_BOX_COLUMNS);
            y = 3 + 2 * (slot / IN_BOX_COLUMNS);
            ChooseBoxMenu_Set4bppPixel(markerFrame, 2, x, y, markerIndex);
            ChooseBoxMenu_Set4bppPixel(markerFrame, 2, x + 1, y, markerIndex);
        }
    }
}

static void ChooseBoxMenu_DestroyOverviewSprites(void)
{
    u8 i;

    for (i = 0; i < ARRAY_COUNT(sChooseBoxMenu->overviewSprites); i++)
    {
        if (sChooseBoxMenu->overviewSprites[i])
        {
            DestroySprite(sChooseBoxMenu->overviewSprites[i]);
            sChooseBoxMenu->overviewSprites[i] = NULL;
        }

        if (sChooseBoxMenu->overviewMarkerSprites[i])
        {
            DestroySprite(sChooseBoxMenu->overviewMarkerSprites[i]);
            sChooseBoxMenu->overviewMarkerSprites[i] = NULL;
        }
    }
}

static void ChooseBoxMenu_LoadOverviewGroup(u8 groupBase)
{
    struct SpriteSheet baseSheet =
    {
        .data = sChooseBoxMenu->overviewBaseTiles,
        .size = sizeof(sChooseBoxMenu->overviewBaseTiles),
        .tag = GFXTAG_HGSS_BOX_THUMBNAIL,
    };
    struct SpritePalette basePalette =
    {
        .data = sChooseBoxMenu->overviewBasePal,
        .tag = PALTAG_HGSS_BOX_THUMBNAIL,
    };
    struct SpriteSheet markerSheet =
    {
        .data = sChooseBoxMenu->overviewMarkerTiles,
        .size = sizeof(sChooseBoxMenu->overviewMarkerTiles),
        .tag = GFXTAG_HGSS_BOX_THUMBNAIL_MARKERS,
    };
    struct SpritePalette markerPalette =
    {
        .data = sChooseBoxMenu->overviewMarkerPal,
        .tag = PALTAG_HGSS_BOX_THUMBNAIL_MARKERS,
    };
    u8 i;

    ChooseBoxMenu_DestroyOverviewSprites();
    ChooseBoxMenu_BuildOverviewGroup(groupBase);

    FreeSpriteTilesByTag(GFXTAG_HGSS_BOX_THUMBNAIL);
    FreeSpritePaletteByTag(PALTAG_HGSS_BOX_THUMBNAIL);
    FreeSpriteTilesByTag(GFXTAG_HGSS_BOX_THUMBNAIL_MARKERS);
    FreeSpritePaletteByTag(PALTAG_HGSS_BOX_THUMBNAIL_MARKERS);
    LoadSpriteSheet(&baseSheet);
    LoadSpritePalette(&basePalette);
    LoadSpriteSheet(&markerSheet);
    LoadSpritePalette(&markerPalette);

    for (i = 0; i < 6; i++)
    {
        u8 boxId = groupBase + i;
        u8 spriteId;

        if (boxId >= TOTAL_BOXES_COUNT)
            continue;

        // ov14_021F4278: six cells, x=43+34*i. ov14_021F4BC0: y=84.
        spriteId = CreateSprite(&sSpriteTemplate_HgssBoxThumbnail, 43 + 34 * i, 84, 1);
        if (spriteId != MAX_SPRITES)
        {
            sChooseBoxMenu->overviewSprites[i] = &gSprites[spriteId];
            StartSpriteAnim(sChooseBoxMenu->overviewSprites[i], i);
            sChooseBoxMenu->overviewSprites[i]->data[0] = boxId;
        }

        // The compact 16x16 marker OBJ shares the 32x32 base center, so its
        // visible pixels land at the exact native base-local coordinates.
        // Lower subpriority places it earlier in OAM, above the base OBJ.
        spriteId = CreateSprite(&sSpriteTemplate_HgssBoxThumbnailMarkers, 43 + 34 * i, 84, 0);
        if (spriteId != MAX_SPRITES)
        {
            sChooseBoxMenu->overviewMarkerSprites[i] = &gSprites[spriteId];
            StartSpriteAnim(sChooseBoxMenu->overviewMarkerSprites[i], i);
            sChooseBoxMenu->overviewMarkerSprites[i]->data[0] = boxId;
        }
    }

    sChooseBoxMenu->overviewGroupBase = groupBase;
}

static void LoadChooseBoxMenuGfx(struct ChooseBoxMenu *menu)
{
    const u8 *srcTiles = (const u8 *)sHgssChooseBoxNav_Gfx;
    struct SpriteSheet spriteSheet =
    {
        .data = menu->navTiles,
        .size = sizeof(menu->navTiles),
        .tag = GFXTAG_HGSS_CHOOSE_BOX_NAV,
    };
    struct SpritePalette spritePalette =
    {
        .data = sHgssChooseBoxNav_Pal,
        .tag = PALTAG_HGSS_CHOOSE_BOX_NAV,
    };
    u8 frame;
    u8 row;
    u8 col;

    // The verified export is four 24x24 frames in one 96x24 strip. GBA OBJ
    // hardware has no 24x24 shape, so repack each frame into the upper-left
    // 24x24 of a transparent 32x32 cell. Visible HGSS pixels remain untouched
    // and 1:1; the extra row/column are transparent hardware padding only.
    CpuFill32(0, menu->navTiles, sizeof(menu->navTiles));
    for (frame = 0; frame < 4; frame++)
    {
        for (row = 0; row < 3; row++)
        {
            for (col = 0; col < 3; col++)
            {
                u32 srcTile = row * 12 + frame * 3 + col;
                u32 dstTile = frame * 16 + row * 4 + col;

                CpuCopy32(
                    srcTiles + srcTile * TILE_SIZE_4BPP,
                    menu->navTiles + dstTile * TILE_SIZE_4BPP,
                    TILE_SIZE_4BPP);
            }
        }
    }

    FreeSpriteTilesByTag(GFXTAG_HGSS_CHOOSE_BOX_NAV);
    FreeSpritePaletteByTag(PALTAG_HGSS_CHOOSE_BOX_NAV);
    LoadSpriteSheet(&spriteSheet);
    LoadSpritePalette(&spritePalette);

    // The authentic overview is rebuilt dynamically after sChooseBoxMenu is
    // assigned, matching HGSS's member-70/member-75 runtime mutation path.
    sChooseBoxMenu = menu;
}

static void FreeChooseBoxMenu(void)
{
    FreeSpriteTilesByTag(GFXTAG_BOX_SELECTION_PER_30);
    FreeSpriteTilesByTag(GFXTAG_HGSS_CHOOSE_BOX_NAV);
    FreeSpritePaletteByTag(PALTAG_HGSS_CHOOSE_BOX_NAV);
    FreeSpriteTilesByTag(GFXTAG_HGSS_BOX_THUMBNAIL);
    FreeSpritePaletteByTag(PALTAG_HGSS_BOX_THUMBNAIL);
    FreeSpriteTilesByTag(GFXTAG_HGSS_BOX_THUMBNAIL_MARKERS);
    FreeSpritePaletteByTag(PALTAG_HGSS_BOX_THUMBNAIL_MARKERS);
    sChooseBoxMenu = NULL;
}

static u8 HandleChooseBoxMenuInput(void)
{
    if (JOY_NEW(B_BUTTON))
    {
        PlaySE(SE_SELECT);
        return BOXID_CANCELED;
    }

    if (JOY_NEW(A_BUTTON))
        return sChooseBoxMenu->curBox;

    if (JOY_REPEAT(DPAD_LEFT))
    {
        PlaySE(SE_SELECT);
        if (sChooseBoxMenu->navSprites[0])
            StartSpriteAnim(sChooseBoxMenu->navSprites[0], HGSS_CHOOSE_BOX_NAV_LEFT_PRESSED);
        ChooseBoxMenu_MoveCursor(-1);
    }
    else if (JOY_REPEAT(DPAD_RIGHT))
    {
        PlaySE(SE_SELECT);
        if (sChooseBoxMenu->navSprites[1])
            StartSpriteAnim(sChooseBoxMenu->navSprites[1], HGSS_CHOOSE_BOX_NAV_RIGHT_PRESSED);
        ChooseBoxMenu_MoveCursor(1);
    }

    return BOXID_NONE_CHOSEN;
}

static void ChooseBoxMenu_CreateSprites(u8 curBox)
{
    u8 spriteId;

    sChooseBoxMenu->curBox = curBox;
    sChooseBoxMenu->active = TRUE;
    sChooseBoxMenu->overviewGroupBase = 0xFF;

    ChooseBoxMenu_UpdateOverviewGroup();

    // HGSS native templates use centers x=12/y=28 and x=156/y=28 for 24x24
    // cells. The live GBA objects are transparent-padded 32x32 cells, so shift
    // their OBJ centers by +4/+4 to preserve those exact visible bounds.
    spriteId = CreateSprite(&sSpriteTemplate_HgssChooseBoxNav, 16, 32, 1);
    if (spriteId != MAX_SPRITES)
    {
        sChooseBoxMenu->navSprites[0] = &gSprites[spriteId];
        StartSpriteAnim(sChooseBoxMenu->navSprites[0], HGSS_CHOOSE_BOX_NAV_LEFT_NORMAL);
    }

    spriteId = CreateSprite(&sSpriteTemplate_HgssChooseBoxNav, 160, 32, 1);
    if (spriteId != MAX_SPRITES)
    {
        sChooseBoxMenu->navSprites[1] = &gSprites[spriteId];
        StartSpriteAnim(sChooseBoxMenu->navSprites[1], HGSS_CHOOSE_BOX_NAV_RIGHT_NORMAL);
    }

    // The old storage cursor is not part of HGSS's Choose Box controls.
    // Keep its logical position intact and hide only its legacy visual while
    // the authenticated HGSS left/right controls own this interaction.
    if (sStorage->cursorSprite)
    {
        sChooseBoxMenu->savedCursorInvisible = sStorage->cursorSprite->invisible;
        sStorage->cursorSprite->invisible = TRUE;
    }

    if (sStorage->movingMonSprite)
    {
        sStorage->movingMonSprite->callback = SpriteCB_HeldMonInChooseBox;
        sStorage->movingMonSprite->subpriority = 1;
    }

    ChooseBoxMenu_UpdateHover();
}

static void ChooseBoxMenu_DestroySprites(void)
{
    u8 i;

    sChooseBoxMenu->active = FALSE;

    for (i = 0; i < ARRAY_COUNT(sChooseBoxMenu->navSprites); i++)
    {
        if (sChooseBoxMenu->navSprites[i])
        {
            DestroySprite(sChooseBoxMenu->navSprites[i]);
            sChooseBoxMenu->navSprites[i] = NULL;
        }
    }

    ChooseBoxMenu_DestroyOverviewSprites();

    if (sChooseBoxMenu->monCountSprite)
    {
        DestroySprite(sChooseBoxMenu->monCountSprite);
        sChooseBoxMenu->monCountSprite = NULL;
    }
    if (sStorage->cursorSprite)
        sStorage->cursorSprite->invisible = sChooseBoxMenu->savedCursorInvisible;
    if (sStorage->movingMonSprite)
    {
        sStorage->movingMonSprite->callback = SpriteCB_HeldMon;
        sStorage->movingMonSprite->subpriority = 3;
    }
    if (sStorage->boxOption == OPTION_MOVE_ITEMS)
    {
        u8 id = GetItemIconIdxByPosition(CURSOR_AREA_IN_HAND, 0);
        if (id < MAX_ITEM_ICONS)
            sStorage->itemIcons[id].sprite->subpriority = 9;
    }
    UpdateBoxTitle(StorageGetCurrentBox());
    UpdateBoxTitlePalette();
}

static void ChooseBoxMenu_MoveCursor(s8 delta)
{
    if (delta < 0)
        sChooseBoxMenu->curBox = (sChooseBoxMenu->curBox == 0)
            ? TOTAL_BOXES_COUNT - 1
            : sChooseBoxMenu->curBox - 1;
    else if (delta > 0)
        sChooseBoxMenu->curBox = (sChooseBoxMenu->curBox + 1) % TOTAL_BOXES_COUNT;

    ChooseBoxMenu_UpdateHover();
}

static void ChooseBoxMenu_UpdateHover(void)
{
    ChooseBoxMenu_UpdateOverviewGroup();
    ChooseBoxMenu_PrintInfo();
}

static void ChooseBoxMenu_UpdateOverviewGroup(void)
{
    u8 groupBase = (sChooseBoxMenu->curBox / 6) * 6;

    if (sChooseBoxMenu->overviewGroupBase != groupBase)
        ChooseBoxMenu_LoadOverviewGroup(groupBase);
}

static void ChooseBoxMenu_PrintInfo(void)
{
    u8 numBoxMonsText[16];
    struct WindowTemplate template;
    struct SpriteSheet spriteSheet;
    u8 windowId;
    u8 numInBox = CountMonsInBox(sChooseBoxMenu->curBox);
    u32 winTileData;
    u8 spriteId;

    if (sChooseBoxMenu->monCountSprite)
    {
        DestroySprite(sChooseBoxMenu->monCountSprite);
        sChooseBoxMenu->monCountSprite = NULL;
    }

    UpdateBoxTitle(sChooseBoxMenu->curBox);
    UpdateBoxTitlePalette();

    memset(&template, 0, sizeof(template));
    template.width = 2;
    template.height = 2;

    windowId = AddWindow(&template);
    FillWindowPixelBuffer(windowId, PIXEL_FILL(0));
    ConvertIntToDecimalStringN(numBoxMonsText, numInBox, STR_CONV_MODE_RIGHT_ALIGN, 2);
    AddTextPrinterParameterized3(windowId, FONT_NORMAL, 0, 1, sTextColors[2], TEXT_SKIP_DRAW, numBoxMonsText);

    winTileData = GetWindowAttribute(windowId, WINDOW_TILE_DATA);
    CpuCopy32((void *)winTileData, sChooseBoxMenu->monCountTiles, 0x100);
    RemoveWindow(windowId);

    FreeSpriteTilesByTag(GFXTAG_BOX_SELECTION_PER_30);
    spriteSheet = (struct SpriteSheet){sChooseBoxMenu->monCountTiles, 0x100, GFXTAG_BOX_SELECTION_PER_30};
    LoadSpriteSheet(&spriteSheet);
    spriteId = CreateSprite(&sSpriteTemplate_ChooseBoxMenu_MonCount, 220, 21, 2);
    if (spriteId != MAX_SPRITES)
    {
        sChooseBoxMenu->monCountSprite = &gSprites[spriteId];
        sChooseBoxMenu->monCountSprite->oam.priority = 1;
        sChooseBoxMenu->monCountSprite->subpriority = 2;
    }
}


//------------------------------------------------------------------------------
//  SECTION: Main tasks
//
//  Below are the main task callbacks that handle the primary actions the
//  player can take in the PC. The most 'important' of these tasks is the
//  primary one, Task_PokeStorageMain. Also included are some basic
//  initialization functions.
//------------------------------------------------------------------------------


static void VBlankCB_PokeStorage(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
    if (sStorage != NULL)
    {
        if (sCursorMode != CURSOR_MODE_MULTI_MOVE)
            SetGpuReg(REG_OFFSET_BG0VOFS, sStorage->bg0_Y);
    }
}

static void CB2_PokeStorage(void)
{
    RunTasks();
    DoScheduledBgTilemapCopiesToVram();
    //ScrollBackground();
    AnimateSprites();
    BuildOamBuffer();
}

static void EnterPokeStorage(u8 boxOption)
{
    ResetTasks();
    sHgssYesNoWindowId = WINDOW_NONE;
    sCurrentBoxOption = boxOption;
    sStorage = AllocZeroed(sizeof(*sStorage));
    if (sStorage == NULL)
    {
        if (boxOption == OPTION_SELECT_MON)
            SetMainCallback2(CB2_ReturnToFieldContinueScript);
        else
            SetMainCallback2(CB2_ExitPokeStorage);
    }
    else
    {
        sStorage->boxOption = boxOption;
        sStorage->isReopening = FALSE;
        sMovingItemId = ITEM_NONE;
        sStorage->state = 0;
        sStorage->taskId = CreateTask(Task_InitPokeStorage, 3);
        sLastUsedBox = StorageGetCurrentBox();
        SetMainCallback2(CB2_PokeStorage);
    }
}

static void CB2_ReturnToPokeStorage(void)
{
    ResetTasks();
    sHgssYesNoWindowId = WINDOW_NONE;
    sStorage = AllocZeroed(sizeof(*sStorage));
    if (sStorage == NULL)
    {
        if (sStorage->boxOption == OPTION_SELECT_MON)
            SetMainCallback2(CB2_ReturnToFieldContinueScript);
        else
            SetMainCallback2(CB2_ExitPokeStorage);
    }
    else
    {
        sStorage->boxOption = sCurrentBoxOption;
        sStorage->isReopening = TRUE;
        sStorage->state = 0;
        sStorage->taskId = CreateTask(Task_InitPokeStorage, 3);
        SetMainCallback2(CB2_PokeStorage);
    }
}

static void ResetAllBgCoords(void)
{
    SetGpuReg(REG_OFFSET_BG0HOFS, 0);
    SetGpuReg(REG_OFFSET_BG0VOFS, 0);
    SetGpuReg(REG_OFFSET_BG1HOFS, 0);
    SetGpuReg(REG_OFFSET_BG1VOFS, 0);
    SetGpuReg(REG_OFFSET_BG2HOFS, 0);
    SetGpuReg(REG_OFFSET_BG2VOFS, 0);
    SetGpuReg(REG_OFFSET_BG3HOFS, 0);
    SetGpuReg(REG_OFFSET_BG3VOFS, 0);
}

static void ResetForPokeStorage(void)
{
    ResetPaletteFade();
    ResetSpriteData();
    FreeSpriteTileRanges();
    FreeAllSpritePalettes();
    ClearDma3Requests();
    gReservedSpriteTileCount = 0x280;
    gKeyRepeatStartDelay = 20;
    ClearScheduledBgCopiesToVram();
}

static void InitStartingPosData(void)
{
    ClearSavedCursorPos();
}

static void SetMonIconTransparency(void)
{
    if (sStorage->boxOption == OPTION_MOVE_ITEMS || sStorage->boxOption == OPTION_SELECT_MON)
    {
        // BG2 (TGT1) blends with BG1+BG2+BG3 (TGT2)
        // Sprites with objMode=BLEND (mons w/o items or ineligible mons, auto TGT1) blend with BG1+BG2+BG3 (TGT2)
        // BG1 is needed for party area on left side of screen
        SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG2 | BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_BG1 | BLDCNT_TGT2_BG2 | BLDCNT_TGT2_BG3);
        SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(8, 8));
    }
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_BG_ALL_ON | DISPCNT_OBJ_1D_MAP);
}

static void SetPokeStorageTask(TaskFunc newFunc)
{
    gTasks[sStorage->taskId].func = newFunc;
    sStorage->state = 0;
    // clears menu interaction flag when returning to main task
    // to allow mon info to come back
    if (newFunc == Task_PokeStorageMain)
    {
        sStorage->inMenuInteraction = FALSE;
        UpdateMonInfoTilemap();
    }
}

static void Task_InitPokeStorage(u8 taskId)
{
    switch (sStorage->state)
    {
    case 0:
        SetVBlankCallback(NULL);
        SetGpuReg(REG_OFFSET_DISPCNT, 0);
        CpuFill32(0, (void *)VRAM, VRAM_SIZE);
        ResetForPokeStorage();
        if (sStorage->isReopening)
        {
            switch (sWhichToReshow)
            {
            case SCREEN_CHANGE_NAME_BOX - 1:
                // Return from naming box
                LoadSavedMovingMon();
                break;
            case SCREEN_CHANGE_SUMMARY_SCREEN - 1:
                // Return from summary screen
                SetSelectionAfterSummaryScreen();
                break;
            case SCREEN_CHANGE_ITEM_FROM_BAG - 1:
                // Return from bag menu
                GiveChosenBagItem();
                break;
            }
        }
        LoadPokeStorageMenuGfx();
        break;
    case 1:
        if (!InitPokeStorageWindows())
        {
            SetPokeStorageTask(Task_ChangeScreen);
            return;
        }
        break;
    case 2:
        ClearWindowTilemap(WIN_MESSAGE);
        CpuFill32(0, (void *)VRAM, 0x200);
        break;
    case 3:
        ResetAllBgCoords();
        if (!sStorage->isReopening)
            InitStartingPosData();
        break;
    case 4:
        InitMonIconFields();
        if (!sStorage->isReopening)
            InitCursor();
        else
            InitCursorOnReopen();
        break;
    case 5:
        if (!MultiMove_Init())
        {
            SetPokeStorageTask(Task_ChangeScreen);
            return;
        }
        else
        {
            SetScrollingBackground();
            InitPokeStorageBg0();
        }
        break;
    case 6:
        if (!InitPalettesAndSprites())
            return;
        break;
    case 7:
        CreateCursorSprites();
        RefreshDisplayMonData();
        break;
    case 8:
        CreatePartyMonsSprites(TRUE);
        ScheduleBgCopyTilemapToVram(1);
        break;
    case 9:
        CreateInitBoxTask(StorageGetCurrentBox());
        break;
    case 10:
        if (IsInitBoxActive())
            return;

        if (sStorage->boxOption == OPTION_MOVE_ITEMS)
        {
            CreateItemIconSprites();
            InitCursorItemIcon();
        }
        break;
    case 11:
        // Alpha blend BG2 with BG3
        SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG2 | BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_BG3);
        SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(4, 12));
        SetMonIconTransparency();
        if (!sStorage->isReopening)
        {
            BlendPalettes(PALETTES_ALL, 16, RGB_BLACK);
            SetPokeStorageTask(Task_ShowPokeStorage);
        }
        else
        {
            BlendPalettes(PALETTES_ALL, 16, RGB_BLACK);
            SetPokeStorageTask(Task_ReshowPokeStorage);
        }
        SetVBlankCallback(VBlankCB_PokeStorage);
        return;
    default:
        return;
    }

    sStorage->state++;
}

static void Task_ShowPokeStorage(u8 taskId)
{
    switch (sStorage->state)
    {
    case 0:
        PlaySE(SE_PC_LOGIN);
        ComputerScreenOpenEffect(20, 0, 1);
        sStorage->state++;
        break;
    case 1:
        if (!IsComputerScreenOpenEffectActive())
            SetPokeStorageTask(Task_PokeStorageMain);
        break;
    }
}

static void Task_ReshowPokeStorage(u8 taskId)
{
    switch (sStorage->state)
    {
    case 0:
        BeginNormalPaletteFade(PALETTES_ALL, -1, 0x10, 0, RGB_BLACK);
        sStorage->state++;
        break;
    case 1:
        if (!UpdatePaletteFade())
        {
            SetPokeStorageTask(Task_PokeStorageMain);
        }
        break;
    }
}

// States for the outer switch in Task_PokeStorageMain
enum {
    MSTATE_HANDLE_INPUT,
    MSTATE_MOVE_CURSOR,
    MSTATE_HIDE_PANEL_SPRITES,
    MSTATE_SCROLL_BOX,
    MSTATE_WAIT_MSG,
    MSTATE_ERROR_LAST_PARTY_MON,
    MSTATE_ERROR_HAS_MAIL,
    MSTATE_WAIT_ERROR_MSG,
    MSTATE_MULTIMOVE_RUN,
    MSTATE_MULTIMOVE_RUN_CANCEL,
    MSTATE_MULTIMOVE_RUN_MOVED,
    MSTATE_SCROLL_BOX_ITEM,
    MSTATE_WAIT_ITEM_ANIM,
};

static void Task_PokeStorageMain(u8 taskId)
{
    switch (sStorage->state)
    {
    case MSTATE_HANDLE_INPUT:
        switch (HandleInput())
        {
        case INPUT_MOVE_CURSOR:
            PlaySE(SE_SELECT);
            sStorage->state = MSTATE_MOVE_CURSOR;
            break;
        case INPUT_TOGGLE_MON_INFO:
            PlaySE(SE_SELECT);
            sStorage->showMonInfo ^= 1;
            TryRefreshDisplayMon();
            UpdateMonInfoTilemap();
            break;
        case INPUT_HIDE_PARTY:
            if (sStorage->boxOption == OPTION_MOVE_MONS || sStorage->boxOption == OPTION_SELECT_MON)
            {
                if (IsMonBeingMoved() && ItemIsMail(sStorage->displayMon.heldItem))
                    sStorage->state = MSTATE_ERROR_HAS_MAIL;
                else
                    SetPokeStorageTask(Task_HidePartyPokemon);
            }
            else if (sStorage->boxOption == OPTION_MOVE_ITEMS)
            {
                SetPokeStorageTask(Task_HidePartyPokemon);
            }
            break;
        case INPUT_PRESSED_B:
            SetPokeStorageTask(Task_OnBPressed);
            break;
        case INPUT_BOX_OPTIONS:
            PlaySE(SE_SELECT);
            SetPokeStorageTask(Task_HandleBoxOptions);
            break;
        case INPUT_IN_MENU:
            SetPokeStorageTask(Task_OnSelectedMon);
            break;
        case INPUT_SCROLL_RIGHT:
			if (sStorage->state == MSTATE_SCROLL_BOX) //Prevent spamming left/right breaking the box title lol
			break;
			
            PlaySE(SE_SELECT);
            sStorage->newCurrBoxId = StorageGetCurrentBox() + 1;
            if (sStorage->newCurrBoxId >= TOTAL_BOXES_COUNT)
                sStorage->newCurrBoxId = 0;
            if (sStorage->boxOption != OPTION_MOVE_ITEMS)
            {
                SetUpScrollToBox(sStorage->newCurrBoxId);
                sStorage->state = MSTATE_SCROLL_BOX;
            }
            else
            {
                TryHideItemAtCursor();
                sStorage->state = MSTATE_SCROLL_BOX_ITEM;
            }
            break;
        case INPUT_SCROLL_LEFT:
			if (sStorage->state == MSTATE_SCROLL_BOX) //Prevent spamming left/right breaking the box title lol
			break;
	
            PlaySE(SE_SELECT);
            sStorage->newCurrBoxId = StorageGetCurrentBox() - 1;
            if (sStorage->newCurrBoxId < 0)
                sStorage->newCurrBoxId = TOTAL_BOXES_COUNT - 1;
            if (sStorage->boxOption != OPTION_MOVE_ITEMS)
            {
                SetUpScrollToBox(sStorage->newCurrBoxId);
                sStorage->state = MSTATE_SCROLL_BOX;
            }
            else
            {
                TryHideItemAtCursor();
                sStorage->state = MSTATE_SCROLL_BOX_ITEM;
            }
            break;
        case INPUT_DEPOSIT:
            if (!IsRemovingLastPartyMon())
            {
                if (ItemIsMail(sStorage->displayMon.heldItem))
                {
                    sStorage->state = MSTATE_ERROR_HAS_MAIL;
                }
                else
                {
                    PlaySE(SE_SELECT);
                    SetPokeStorageTask(Task_DepositMon);
                }
            }
            else
            {
                sStorage->state = MSTATE_ERROR_LAST_PARTY_MON;
            }
            break;
        case INPUT_MOVE_MON:
            if (IsRemovingLastPartyMon())
            {
                sStorage->state = MSTATE_ERROR_LAST_PARTY_MON;
            }
            else
            {
                PlaySE(SE_SELECT);
                SetPokeStorageTask(Task_MoveMon);
            }
            break;
        case INPUT_SHIFT_MON:
            if (!CanShiftMon())
            {
                sStorage->state = MSTATE_ERROR_LAST_PARTY_MON;
            }
            else
            {
                PlaySE(SE_SELECT);
                SetPokeStorageTask(Task_ShiftMon);
            }
            break;
        case INPUT_WITHDRAW:
            PlaySE(SE_SELECT);
            SetPokeStorageTask(Task_WithdrawMon);
            break;
        case INPUT_PLACE_MON:
            PlaySE(SE_SELECT);
            SetPokeStorageTask(Task_PlaceMon);
            break;
        case INPUT_TAKE_ITEM:
            PlaySE(SE_SELECT);
            SetPokeStorageTask(Task_TakeItemForMoving);
            break;
        case INPUT_GIVE_ITEM:
            PlaySE(SE_SELECT);
            SetPokeStorageTask(Task_GiveMovingItemToMon);
            break;
        case INPUT_SWITCH_ITEMS:
            PlaySE(SE_SELECT);
            SetPokeStorageTask(Task_SwitchSelectedItem);
            break;
        case INPUT_MULTIMOVE_START:
            PlaySE(SE_SELECT);
            MultiMove_SetFunction(MULTIMOVE_START);
            sStorage->state = MSTATE_MULTIMOVE_RUN;
            break;
        case INPUT_MULTIMOVE_SINGLE:
            MultiMove_SetFunction(MULTIMOVE_CANCEL);
            sStorage->state = MSTATE_MULTIMOVE_RUN_CANCEL;
            break;
        case INPUT_MULTIMOVE_CHANGE_SELECTION:
            PlaySE(SE_SELECT);
            MultiMove_SetFunction(MULTIMOVE_CHANGE_SELECTION);
            sStorage->state = MSTATE_MULTIMOVE_RUN_MOVED;
            break;
        case INPUT_MULTIMOVE_GRAB_SELECTION:
            MultiMove_SetFunction(MULTIMOVE_GRAB_SELECTION);
            sStorage->state = MSTATE_MULTIMOVE_RUN;
            break;
        case INPUT_MULTIMOVE_MOVE_MONS:
            PlaySE(SE_SELECT);
            MultiMove_SetFunction(MULTIMOVE_MOVE_MONS);
            sStorage->state = MSTATE_MULTIMOVE_RUN_MOVED;
            break;
        case INPUT_MULTIMOVE_PLACE_MONS:
            PlaySE(SE_SELECT);
            MultiMove_SetFunction(MULTIMOVE_PLACE_MONS);
            sStorage->state = MSTATE_MULTIMOVE_RUN;
            break;
        case INPUT_MULTIMOVE_UNABLE:
            // When selecting/moving multiple Pokémon the
            // cursor may not wrap around the edges.
            PlaySE(SE_FAILURE);
            break;
        }
        break;
    case MSTATE_MOVE_CURSOR:
        if (!UpdateCursorPos())
        {
            u16 targetBg0Y = GetTargetBg0Y();
            if (targetBg0Y != sStorage->bg0_Y && targetBg0Y != 0 && sStorage->bg0_Y != 0)
            {
                HideInfoPanelSprites();
                sStorage->state = MSTATE_HIDE_PANEL_SPRITES;
            }
            else
            {
                UpdateMonInfoTilemap();
                if (sStorage->setMosaic)
                    RefreshDisplayMonData();
                sStorage->state = MSTATE_HANDLE_INPUT;
            }
        }
        break;
    case MSTATE_HIDE_PANEL_SPRITES:
        UpdateMonInfoTilemap();
        if (sStorage->setMosaic)
            RefreshDisplayMonData();
        sStorage->state = MSTATE_HANDLE_INPUT;
        break;
    case MSTATE_SCROLL_BOX:
        if (!ScrollToBox())
        {
            SetCurrentBox(sStorage->newCurrBoxId);
            if (sCursorArea != CURSOR_AREA_IN_PARTY && !IsMonBeingMoved())
            {
                RefreshDisplayMon();
                RefreshDisplayMonData();
            }

            if (sStorage->boxOption == OPTION_MOVE_ITEMS)
            {
                TryShowItemAtCursor();
                sStorage->state = MSTATE_WAIT_ITEM_ANIM;
            }
            else
            {
                sStorage->state = MSTATE_HANDLE_INPUT;
                UpdateMonInfoTilemap();
            }
        }
        break;
    case MSTATE_WAIT_MSG:
        if (JOY_NEW(A_BUTTON | B_BUTTON | DPAD_ANY))
        {
            ClearBottomWindow();
            sStorage->state = MSTATE_HANDLE_INPUT;
        }
        break;
    case MSTATE_ERROR_LAST_PARTY_MON:
        PlaySE(SE_FAILURE);
        PrintMessage(MSG_LAST_POKE);
        sStorage->state = MSTATE_WAIT_ERROR_MSG;
        break;
    case MSTATE_ERROR_HAS_MAIL:
        PlaySE(SE_FAILURE);
        PrintMessage(MSG_PLEASE_REMOVE_MAIL);
        sStorage->state = MSTATE_WAIT_ERROR_MSG;
        break;
    case MSTATE_WAIT_ERROR_MSG:
        if (JOY_NEW(A_BUTTON | B_BUTTON | DPAD_ANY))
        {
            ClearBottomWindow();
            SetPokeStorageTask(Task_PokeStorageMain);
        }
        break;
    case MSTATE_MULTIMOVE_RUN:
        if (!MultiMove_RunFunction())
            sStorage->state = MSTATE_HANDLE_INPUT;
        break;
    case MSTATE_MULTIMOVE_RUN_CANCEL:
        // Began a multiple Pokémon selection but
        // ended up selecting a single Pokémon.
        // Wait for multi move to cancel, then
        // do a normal move.
        if (!MultiMove_RunFunction())
            SetPokeStorageTask(Task_MoveMon);
        break;
    case MSTATE_MULTIMOVE_RUN_MOVED:
        if (!MultiMove_RunFunction())
        {
            if (sStorage->setMosaic)
                RefreshDisplayMonData();
            sStorage->state = MSTATE_HANDLE_INPUT;
        }
        break;
    case MSTATE_SCROLL_BOX_ITEM:
        if (!IsItemIconAnimActive())
        {
            SetUpScrollToBox(sStorage->newCurrBoxId);
            sStorage->state = MSTATE_SCROLL_BOX;
        }
        break;
    case MSTATE_WAIT_ITEM_ANIM:
        if (!IsItemIconAnimActive())
            sStorage->state = MSTATE_HANDLE_INPUT;
        break;
    }
}


static void Task_HidePartyPokemon(u8 taskId)
{
    switch (sStorage->state)
    {
    case 0:
        PlaySE(SE_SELECT);
        if (sStorage->boxOption == OPTION_MOVE_ITEMS)
            MoveHeldItemWithPartyMenu();
        SetCursorBoxPosition(GetSavedCursorPos());
        sStorage->state++;
        break;
    case 1:
        if (!UpdateCursorPos())
        {
            UpdateMonInfoTilemap();
            if (sStorage->setMosaic)
                RefreshDisplayMonData();
            SetPokeStorageTask(Task_PokeStorageMain);
        }
        break;
    }
}

static void Task_OnSelectedMon(u8 taskId)
{
    switch (sStorage->state)
    {
    case 0:
        PlaySE(SE_SELECT);
        // sets flag to prevent mon info panel from appearing
        sStorage->inMenuInteraction = TRUE;
        AddMenu();
        sStorage->state = 1;
        break;
    case 1:
        if (!IsMenuLoading())
            sStorage->state = 2;
        break;
    case 2:
        switch (HandleMenuInput())
        {
        case MENU_B_PRESSED:
        case MENU_CANCEL:
            ClearBottomWindow();
            SetPokeStorageTask(Task_PokeStorageMain);
            break;
        case MENU_MOVE:
            if (IsRemovingLastPartyMon())
            {
                sStorage->state = 3;
            }
            else
            {
                PlaySE(SE_SELECT);
                ClearBottomWindow();
                SetPokeStorageTask(Task_MoveMon);
            }
            break;
        case MENU_PLACE:
            PlaySE(SE_SELECT);
            ClearBottomWindow();
            SetPokeStorageTask(Task_PlaceMon);
            break;
        case MENU_SHIFT:
            if (!CanShiftMon())
            {
                sStorage->state = 3;
            }
            else
            {
                PlaySE(SE_SELECT);
                ClearBottomWindow();
                SetPokeStorageTask(Task_ShiftMon);
            }
            break;
        case MENU_WITHDRAW:
            PlaySE(SE_SELECT);
            SetPokeStorageTask(Task_WithdrawMon);
            break;
        case MENU_STORE:
            if (IsRemovingLastPartyMon())
            {
                sStorage->state = 3;
            }
            else if (ItemIsMail(sStorage->displayMon.heldItem))
            {
                sStorage->state = 4;
            }
            else
            {
                PlaySE(SE_SELECT);
                SetPokeStorageTask(Task_DepositMon);
            }
            break;
        case MENU_RELEASE:
            if (IsRemovingLastPartyMon())
            {
                sStorage->state = 3;
            }
            else if (sStorage->displayMon.isEgg)
            {
                sStorage->state = 5; // Cannot release an Egg.
            }
            else if (ItemIsMail(sStorage->displayMon.heldItem))
            {
                sStorage->state = 4;
            }
            else
            {
                PlaySE(SE_SELECT);
                SetPokeStorageTask(Task_ReleaseMon);
            }
            break;
        case MENU_SUMMARY:
            PlaySE(SE_SELECT);
            SetPokeStorageTask(Task_ShowMonSummary);
            break;
        case MENU_MARK:
            PlaySE(SE_SELECT);
            ClearBottomWindow();
            SetPokeStorageTask(Task_ShowMarkMenu);
            break;
        case MENU_TAKE:
            PlaySE(SE_SELECT);
            SetPokeStorageTask(Task_TakeItemForMoving);
            break;
        case MENU_GIVE:
            PlaySE(SE_SELECT);
            SetPokeStorageTask(Task_GiveMovingItemToMon);
            break;
        case MENU_BAG:
            SetPokeStorageTask(Task_ItemToBag);
            break;
        case MENU_SWITCH:
            PlaySE(SE_SELECT);
            SetPokeStorageTask(Task_SwitchSelectedItem);
            break;
        case MENU_GIVE_2:
            PlaySE(SE_SELECT);
            SetPokeStorageTask(Task_GiveItemFromBag);
            break;
        case MENU_SELECT:
            PlaySE(SE_SELECT);
            struct BoxPokemon *boxmon = GetCursorBoxMon();
            if (sCursorArea == CURSOR_AREA_IN_PARTY)
            {
                gSpecialVar_0x8004 = sCursorPosition;
            }
            else
            {
                gSpecialVar_0x8004 = PC_MON_CHOSEN;
                gSpecialVar_MonBoxPos = sCursorPosition;
                gSpecialVar_MonBoxId = StorageGetCurrentBox();
            }
            if (IsBoxMonExcluded(boxmon))
                gSpecialVar_Result = FALSE;
            else
                gSpecialVar_Result = TRUE;
            sStorage->screenChangeType = SCREEN_CHANGE_EXIT_BOX;
            SetPokeStorageTask(Task_ChangeScreen);
            break;
        }
        break;
    case 3:
        PlaySE(SE_FAILURE);
        PrintMessage(MSG_LAST_POKE);
        sStorage->state = 6;
        break;
    case 5:
        PlaySE(SE_FAILURE);
        PrintMessage(MSG_CANT_RELEASE_EGG);
        sStorage->state = 6;
        break;
    case 4:
        PlaySE(SE_FAILURE);
        PrintMessage(MSG_PLEASE_REMOVE_MAIL);
        sStorage->state = 6;
        break;
    case 6:
        if (JOY_NEW(A_BUTTON | B_BUTTON | DPAD_ANY))
        {
            ClearBottomWindow();
            SetPokeStorageTask(Task_PokeStorageMain);
        }
        break;
    }
}

static void Task_MoveMon(u8 taskId)
{
    switch (sStorage->state)
    {
    case 0:
        if (sCursorArea == CURSOR_AREA_IN_PARTY && CalculatePlayerPartyCount() == 1)
        {
            SetPokeStorageTask(Task_PokeStorageMain);
            sStorage->state = MSTATE_ERROR_LAST_PARTY_MON;
            return;
        }

        InitMonPlaceChange(CHANGE_GRAB);
        sStorage->state++;
        break;
    case 1:
        if (!DoMonPlaceChange())
        {
            if (sCursorArea == CURSOR_AREA_IN_PARTY)
                SetPokeStorageTask(Task_HandleMovingMonFromParty);
            else
                SetPokeStorageTask(Task_PokeStorageMain);
        }
        break;
    }
}

static void Task_PlaceMon(u8 taskId)
{
    switch (sStorage->state)
    {
    case 0:
        InitMonPlaceChange(CHANGE_PLACE);
        sStorage->state++;
        break;
    case 1:
        if (!DoMonPlaceChange())
        {
            if (sCursorArea == CURSOR_AREA_IN_PARTY)
                SetPokeStorageTask(Task_HandleMovingMonFromParty);
            else
                SetPokeStorageTask(Task_PokeStorageMain);
        }
        break;
    }
}

static void Task_ShiftMon(u8 taskId)
{
    switch (sStorage->state)
    {
    case 0:
        InitMonPlaceChange(CHANGE_SHIFT);
        sStorage->state++;
        break;
    case 1:
        if (!DoMonPlaceChange())
        {
            RefreshDisplayMonData();
            if (sCursorArea == CURSOR_AREA_IN_PARTY)
                SetPokeStorageTask(Task_HandleMovingMonFromParty);
            else
                SetPokeStorageTask(Task_PokeStorageMain);
        }
        break;
    }
}

static void Task_WithdrawMon(u8 taskId)
{
    switch (sStorage->state)
    {
    case 0:
        if (CalculatePlayerPartyCount() == PARTY_SIZE)
        {
            PrintMessage(MSG_PARTY_FULL);
            sStorage->state = 1;
        }
        else
        {
            SaveCursorPos();
            InitMonPlaceChange(CHANGE_GRAB);
            sStorage->state = 2;
        }
        break;
    case 1:
        if (JOY_NEW(A_BUTTON | B_BUTTON | DPAD_ANY))
        {
            ClearBottomWindow();
            SetPokeStorageTask(Task_PokeStorageMain);
        }
        break;
    case 2:
        if (!DoMonPlaceChange())
        {
            SetMovingMonPriority(1);
            SetCursorInParty();
            sStorage->state++;
        }
        break;
    case 3:
        if (!UpdateCursorPos())
        {
            InitMonPlaceChange(CHANGE_PLACE);
            sStorage->state++;
        }
        break;
    case 4:
        if (!DoMonPlaceChange())
        {
            SetPokeStorageTask(Task_HidePartyPokemon);
        }
        break;
    }
}

static void Task_DepositMon(u8 taskId)
{
    u8 boxId;
    s16 boxPosition;

    switch (sStorage->state)
    {
    case 0:
        if (!sIsMonBeingMoved)
        {
            InitMonPlaceChange(CHANGE_GRAB);
            sStorage->state = 14;
        }
        else
        {
            LoadChooseBoxMenuGfx(&sStorage->chooseBoxMenu);
            ChooseBoxMenu_CreateSprites(StorageGetCurrentBox());
            sStorage->state++;
        }
        break;
    case 1:
        boxId = HandleChooseBoxMenuInput();
        switch (boxId)
        {
        case BOXID_NONE_CHOSEN:
            break;
        case BOXID_CANCELED:
            ClearBottomWindow();
            ChooseBoxMenu_DestroySprites();
            FreeChooseBoxMenu();
            sStorage->state = 13;
            break;
        default:
            if (GetFirstFreeBoxSpot(boxId) == -1)
            {
                PlaySE(SE_FAILURE);
            }
            else
            {
                PlaySE(SE_SELECT);
                sStorage->newCurrBoxId = boxId;

                // The HGSS chooser has already previewed this box. Commit only
                // now, on A, and continue without replaying the box scroll.
                SetCurrentBox(sStorage->newCurrBoxId);
                ClearBottomWindow();
                ChooseBoxMenu_DestroySprites();
                FreeChooseBoxMenu();
                sStorage->state = 4;
            }
            break;
        }
        break;
    case 2:
        SetUpScrollToBox(sStorage->newCurrBoxId);
        sStorage->state++;
        break;
    case 3:
        if (!ScrollToBox())
        {
            SetCurrentBox(sStorage->newCurrBoxId);
            sStorage->state++;
        }
        break;
    case 4:
        SaveCursorPos();
        InitMonPlaceChange(CHANGE_GRAB);
        sStorage->state++;
        break;
    case 5:
        if (!DoMonPlaceChange())
        {
            SetMovingMonPriority(1);
            sStorage->state++;
        }
        break;
    case 6:
        CompactPartySlots();
        CompactPartySprites();
        sStorage->state++;
        break;
    case 7:
        if (GetNumPartySpritesCompacting() == 0)
        {
            boxPosition = GetFirstFreeBoxSpot(sStorage->newCurrBoxId);
            SetCursorBoxPosition(boxPosition);
            sStorage->state++;
        }
        break;
    case 8:
        if (!UpdateCursorPos())
        {
            InitMonPlaceChange(CHANGE_PLACE);
            sStorage->state++;
        }
        break;
    case 9:
        if (!DoMonPlaceChange())
        {
            sStorage->state++;
        }
        break;
    case 10:
        {
            u8 partyCount = CalculatePlayerPartyCount();
            u8 savedPos = GetSavedCursorPos();
            u8 returnPos;

            if (savedPos < partyCount)
                returnPos = savedPos;
            else if (partyCount > 0)
                returnPos = partyCount - 1;
            else
                returnPos = 0;

            SetCursorPosition(CURSOR_AREA_IN_PARTY, returnPos);
            sStorage->state++;
        }
        break;
    case 11:
        if (!UpdateCursorPos())
        {
            RefreshDisplayMonData();
            sStorage->state++;
        }
        break;
    case 12:
        StartSpriteAnim(sStorage->cursorSprite, CURSOR_ANIM_BOUNCE);
        SetPokeStorageTask(Task_PokeStorageMain);
        break;
    case 13:
        if (!UpdateCursorPos())
            SetPokeStorageTask(Task_PokeStorageMain);
        break;
    case 14:
        if (!DoMonPlaceChange())
        {
            SetMovingMonPriority(1);
            LoadChooseBoxMenuGfx(&sStorage->chooseBoxMenu);
            ChooseBoxMenu_CreateSprites(StorageGetCurrentBox());
            sStorage->state++;
        }
        break;
    case 15:
        boxId = HandleChooseBoxMenuInput();
        switch (boxId)
        {
        case BOXID_NONE_CHOSEN:
            break;
        case BOXID_CANCELED:
            ClearBottomWindow();
            ChooseBoxMenu_DestroySprites();
            FreeChooseBoxMenu();
            sStorage->state = 16;
            break;
        default:
            if (GetFirstFreeBoxSpot(boxId) == -1)
            {
                PlaySE(SE_FAILURE);
            }
            else
            {
                PlaySE(SE_SELECT);
                sStorage->newCurrBoxId = boxId;

                // The HGSS chooser has already previewed this box. Commit only
                // now, on A, and continue without replaying the box scroll.
                SetCurrentBox(sStorage->newCurrBoxId);
                ClearBottomWindow();
                ChooseBoxMenu_DestroySprites();
                FreeChooseBoxMenu();
                sStorage->state = 4;
            }
            break;
        }
        break;
    case 16:
        if (!UpdateCursorPos())
        {
            InitMonPlaceChange(CHANGE_PLACE);
            sStorage->state++;
        }
        break;
    case 17:
        if (!DoMonPlaceChange())
            SetPokeStorageTask(Task_PokeStorageMain);
        break;
    }
}

static void Task_ReleaseMon(u8 taskId)
{
    switch (sStorage->state)
    {
    case 0:
        PrintMessage(MSG_RELEASE_POKE);
        ShowYesNoWindow(1);
        sStorage->state++;
        // fallthrough
    case 1:
        switch (ProcessHgssYesNoInput())
        {
        case MENU_B_PRESSED:
        case  1: // No
            ClearBottomWindow();
            SetPokeStorageTask(Task_PokeStorageMain);
            break;
        case  0: // Yes
            ClearBottomWindow();
            InitCanReleaseMonVars();
            InitReleaseMon();
            sStorage->state++;
            break;
        }
        break;
    case 2:
        RunCanReleaseMon();
        if (!TryHideReleaseMon())
        {
            while (1)
            {
                s8 canRelease = RunCanReleaseMon();
                if (canRelease == TRUE)
                {
                    sStorage->state++;
                    break;
                }
                else if (!canRelease)
                {
                    sStorage->state = 8;
                    break;
                }
            }
        }
        break;
    case 3:
        ReleaseMon();
        RefreshDisplayMonData();
        PrintMessage(MSG_WAS_RELEASED);
        sStorage->state++;
        break;
    case 4:
        if (JOY_NEW(A_BUTTON | B_BUTTON | DPAD_ANY))
        {
            PrintMessage(MSG_BYE_BYE);
            sStorage->state++;
        }
        break;
    case 5:
        if (JOY_NEW(A_BUTTON | B_BUTTON | DPAD_ANY))
        {
            ClearBottomWindow();
            if (sCursorArea == CURSOR_AREA_IN_PARTY)
            {
                CompactPartySlots();
                CompactPartySprites();
                sStorage->state++;
            }
            else
            {
                sStorage->state = 7;
            }
        }
        break;
    case 6:
        if (GetNumPartySpritesCompacting() == 0)
        {
            RefreshDisplayMon();
            RefreshDisplayMonData();
            sStorage->state++;
        }
        break;
    case 7:
        SetPokeStorageTask(Task_PokeStorageMain);
        break;
    case 8:
        // Start "can't release" sequence
        PrintMessage(MSG_WAS_RELEASED);
        sStorage->state++;
        break;
    case 9:
        if (JOY_NEW(A_BUTTON | B_BUTTON | DPAD_ANY))
        {
            PrintMessage(MSG_SURPRISE);
            sStorage->state++;
        }
        break;
    case 10:
        if (JOY_NEW(A_BUTTON | B_BUTTON | DPAD_ANY))
        {
            ClearBottomWindow();
            ReshowReleaseMon();
            sStorage->state++;
        }
        break;
    case 11:
        if (!ResetReleaseMonSpritePtr())
        {
            TrySetCursorFistAnim();
            PrintMessage(MSG_CAME_BACK);
            sStorage->state++;
        }
        break;
    case 12:
        if (JOY_NEW(A_BUTTON | B_BUTTON | DPAD_ANY))
        {
            PrintMessage(MSG_WORRIED);
            sStorage->state++;
        }
        break;
    case 13:
        if (JOY_NEW(A_BUTTON | B_BUTTON | DPAD_ANY))
        {
            ClearBottomWindow();
            SetPokeStorageTask(Task_PokeStorageMain);
        }
        break;
    }
}

// ============================================================================
// Authentic HGSS markings menu
// ============================================================================

#define HGSS_MARKINGS_MENU_LEFT              19
#define HGSS_MARKINGS_MENU_TOP               2
#define HGSS_MARKINGS_MENU_WIDTH             11
#define HGSS_MARKINGS_MENU_BASE_TILE         256
#define HGSS_MARKINGS_MENU_PAL_BANK          15
#define HGSS_MARKINGS_MENU_SELECTED_PAL_BANK 12

static const u8 sHgssMarkingsMenuMarkX[NUM_MON_MARKINGS] = {180, 180, 212, 212};
static const u8 sHgssMarkingsMenuMarkY[NUM_MON_MARKINGS] = {36, 60, 36, 60};
static const u8 sHgssMarkingsMenuButtonLeft[NUM_MON_MARKINGS] = {1, 1, 6, 6};
static const u8 sHgssMarkingsMenuButtonTop[NUM_MON_MARKINGS] = {1, 4, 1, 4};

static void UpdateHgssMarkingsMenuStates(void)
{
    u16 *tilemap = sStorage->infoTilemapBuffer;
    u8 i;
    u8 x;
    u8 y;

    for (i = 0; i < NUM_MON_MARKINGS; i++)
    {
        u16 palette = sMarkMenu->markingsArray[i]
                    ? HGSS_MARKINGS_MENU_SELECTED_PAL_BANK
                    : HGSS_MARKINGS_MENU_PAL_BANK;

        for (y = 0; y < 3; y++)
        {
            for (x = 0; x < 4; x++)
            {
                u16 *entry = &tilemap[
                    (HGSS_MARKINGS_MENU_TOP + sHgssMarkingsMenuButtonTop[i] + y) * 32
                    + HGSS_MARKINGS_MENU_LEFT + sHgssMarkingsMenuButtonLeft[i] + x];

                *entry = (*entry & 0x0FFF) | (palette << 12);
            }
        }
    }

    ScheduleBgCopyTilemapToVram(0);
}

static void DrawHgssMarkingsMenuPanel(void)
{
    u16 *tilemap = sStorage->infoTilemapBuffer;
    u8 dstRow = 0;
    u8 srcRow;
    u8 x;

    LoadBgTiles(0, sHgssMarkingsMenu_Gfx, sizeof(sHgssMarkingsMenu_Gfx), HGSS_MARKINGS_MENU_BASE_TILE);
    LoadPalette(sHgssMarkingsMenu_Pal, BG_PLTT_ID(HGSS_MARKINGS_MENU_PAL_BANK), PLTT_SIZE_4BPP);
    LoadPalette(sHgssMarkingsMenuSelected_Pal,
                BG_PLTT_ID(HGSS_MARKINGS_MENU_SELECTED_PAL_BANK),
                PLTT_SIZE_4BPP);

    // BoxPokemon persists four marking bits. Use only complete authentic rows:
    // 0-6 contain circle/triangle + square/heart; 10-17 are the action bars.
    for (srcRow = 0; srcRow <= 6; srcRow++, dstRow++)
    {
        for (x = 0; x < HGSS_MARKINGS_MENU_WIDTH; x++)
            tilemap[(HGSS_MARKINGS_MENU_TOP + dstRow) * 32 + HGSS_MARKINGS_MENU_LEFT + x]
                = (HGSS_MARKINGS_MENU_BASE_TILE + srcRow * HGSS_MARKINGS_MENU_WIDTH + x)
                | (HGSS_MARKINGS_MENU_PAL_BANK << 12);
    }

    for (srcRow = 10; srcRow < 18; srcRow++, dstRow++)
    {
        for (x = 0; x < HGSS_MARKINGS_MENU_WIDTH; x++)
            tilemap[(HGSS_MARKINGS_MENU_TOP + dstRow) * 32 + HGSS_MARKINGS_MENU_LEFT + x]
                = (HGSS_MARKINGS_MENU_BASE_TILE + srcRow * HGSS_MARKINGS_MENU_WIDTH + x)
                | (HGSS_MARKINGS_MENU_PAL_BANK << 12);
    }

    UpdateHgssMarkingsMenuStates();
    sStorage->inMenuInteraction = TRUE;
    sStorage->bg0_Y = 0;
    SetGpuReg(REG_OFFSET_BG0VOFS, 0);
    ShowBg(0);
    ScheduleBgCopyTilemapToVram(0);
}

static void RestoreHgssMarkingsMenuPanel(void)
{
    LoadPalette(sTextWindows_Pal, BG_PLTT_ID(HGSS_MARKINGS_MENU_PAL_BANK), sizeof(sTextWindows_Pal));
    RestoreHgssMonInfoPanel();
    sStorage->inMenuInteraction = FALSE;
    UpdateMonInfoTilemap();
    ScheduleBgCopyTilemapToVram(0);
}

static void SetHgssMarkingsMenuCursorPos(void)
{
    if (sMarkMenu->cursorSprite != NULL)
    {
        // The verified HGSS storage hand points left; place it immediately to
        // the right of the active native marking button.
        sMarkMenu->cursorSprite->x = sHgssMarkingsMenuMarkX[sMarkMenu->cursorPos] + 12;
        sMarkMenu->cursorSprite->y = sHgssMarkingsMenuMarkY[sMarkMenu->cursorPos] - 4;
    }
}

static void OpenMonMarkingsMenu_Hgss(u8 markings)
{
    u8 i;
    u8 spriteId;

    sMarkMenu->cursorPos = 0;
    sMarkMenu->markings = markings;
    for (i = 0; i < NUM_MON_MARKINGS; i++)
        sMarkMenu->markingsArray[i] = (markings >> i) & 1;

    sMarkMenu->savedStorageCursorInvisible = TRUE;
    if (sStorage->cursorSprite != NULL)
    {
        sMarkMenu->savedStorageCursorInvisible = sStorage->cursorSprite->invisible;
        sStorage->cursorSprite->invisible = TRUE;
    }

    DrawHgssMarkingsMenuPanel();

    spriteId = CreateSprite(&sSpriteTemplate_Cursor, 0, 0, 1);
    if (spriteId != MAX_SPRITES)
    {
        sMarkMenu->cursorSprite = &gSprites[spriteId];
        sMarkMenu->cursorSprite->oam.paletteNum = IndexOfSpritePaletteTag(PALTAG_HGSS_STORAGE_CURSOR);
        sMarkMenu->cursorSprite->x2 = 6;
        sMarkMenu->cursorSprite->y2 = 10;
        StartSpriteAnim(sMarkMenu->cursorSprite, CURSOR_ANIM_MAIN);
        SetHgssMarkingsMenuCursorPos();
    }
    else
        sMarkMenu->cursorSprite = NULL;
}

static bool8 HandleMonMarkingsMenuInput_Hgss(void)
{
    u8 i;

    if (JOY_NEW(DPAD_LEFT | DPAD_RIGHT))
    {
        PlaySE(SE_SELECT);
        sMarkMenu->cursorPos ^= 2;
        SetHgssMarkingsMenuCursorPos();
        return TRUE;
    }
    if (JOY_NEW(DPAD_UP | DPAD_DOWN))
    {
        PlaySE(SE_SELECT);
        sMarkMenu->cursorPos ^= 1;
        SetHgssMarkingsMenuCursorPos();
        return TRUE;
    }

    if (JOY_NEW(A_BUTTON))
    {
        PlaySE(SE_SELECT);
        sMarkMenu->markingsArray[sMarkMenu->cursorPos] = !sMarkMenu->markingsArray[sMarkMenu->cursorPos];
        UpdateHgssMarkingsMenuStates();
        return TRUE;
    }

    if (JOY_NEW(B_BUTTON))
    {
        PlaySE(SE_SELECT);
        sMarkMenu->markings = 0;
        for (i = 0; i < NUM_MON_MARKINGS; i++)
            sMarkMenu->markings |= sMarkMenu->markingsArray[i] << i;
        return FALSE;
    }

    return TRUE;
}

static void FreeMonMarkingsMenu_Hgss(void)
{
    if (sMarkMenu->cursorSprite != NULL)
    {
        DestroySprite(sMarkMenu->cursorSprite);
        sMarkMenu->cursorSprite = NULL;
    }

    if (sStorage->cursorSprite != NULL)
        sStorage->cursorSprite->invisible = sMarkMenu->savedStorageCursorInvisible;
}

static void Task_ShowMarkMenu(u8 taskId)
{
    switch (sStorage->state)
    {
    case 0:
        sMarkMenu = &sStorage->markMenuHgss;
        sMarkMenu->markings = sStorage->displayMon.markings;
        HideInfoPanelSprites();
        OpenMonMarkingsMenu_Hgss(sStorage->displayMon.markings);
        sStorage->state++;
        break;
    case 1:
        if (!HandleMonMarkingsMenuInput_Hgss())
        {
            FreeMonMarkingsMenu_Hgss();
            SetMonMarkings(sMarkMenu->markings);
            RefreshDisplayMonData();
            RestoreHgssMarkingsMenuPanel();
            SetPokeStorageTask(Task_PokeStorageMain);
        }
        break;
    }
}

static void Task_TakeItemForMoving(u8 taskId)
{
    switch (sStorage->state)
    {
    case 0:
        if (!ItemIsMail(sStorage->displayMon.heldItem))
        {
            ClearBottomWindow();
            sStorage->state++;
        }
        else
        {
            SetPokeStorageTask(Task_PrintCantStoreMail);
        }
        break;
    case 1:
        StartCursorAnim(CURSOR_ANIM_MAIN);
        TakeItemFromMon(sCursorArea, GetCursorPosition());
        sStorage->state++;
        sJustOpenedBag = FALSE;
        break;
    case 2:
        if (!IsItemIconAnimActive())
        {
            StartCursorAnim(CURSOR_ANIM_MAIN);
            ClearBottomWindow();
            RefreshDisplayMon();
            sStorage->displayMonInfoLoadState = 0;
            sStorage->state++;
        }
        break;
    case 3:
        if (PrintDisplayMonInfo())
            sStorage->state++;
        else if (sStorage->inMenuInteraction)
            HideInfoPanelSprites();
        break;
    case 4:
        if (!IsDma3ManagerBusyWithBgCopy())
            SetPokeStorageTask(Task_PokeStorageMain);
        break;
    }
}

static void Task_GiveMovingItemToMon(u8 taskId)
{
    switch (sStorage->state)
    {
    case 0:
        ClearBottomWindow();
        sStorage->state++;
        break;
    case 1:
        StartCursorAnim(CURSOR_ANIM_MAIN);
        GiveItemToMon(sCursorArea, GetCursorPosition());
        sStorage->state++;
        break;
    case 2:
        if (!IsItemIconAnimActive())
        {
            StartCursorAnim(CURSOR_ANIM_BOUNCE);
            RefreshDisplayMon();
            sStorage->displayMonInfoLoadState = 0;
            sStorage->state++;
        }
        break;
    case 3:
        if (PrintDisplayMonInfo())
        {
            SetPokeStorageTask(Task_PokeStorageMain);
        }
        else if (sStorage->inMenuInteraction)
        {
            HideInfoPanelSprites();
        }
        break;
    }
}

static void Task_ItemToBag(u8 taskId)
{
    switch (sStorage->state)
    {
    case 0:
        if (!AddBagItem(sStorage->displayMon.heldItem, 1))
        {
            PlaySE(SE_FAILURE);
            PrintMessage(MSG_BAG_FULL);
            sStorage->state = 3;
        }
        else
        {
            PlaySE(SE_SELECT);
            MoveItemFromMonToBag(sCursorArea, GetCursorPosition());
            sStorage->state = 1;
        }
        break;
    case 1:
        if (!IsItemIconAnimActive())
        {
            PrintMessage(MSG_PLACED_IN_BAG);
            sStorage->state = 2;
        }
        break;
    case 2:
        if (JOY_NEW(A_BUTTON | B_BUTTON | DPAD_ANY))
        {
            ClearBottomWindow();
            RefreshDisplayMon();
            sStorage->displayMonInfoLoadState = 0;
            sStorage->state++;
        }
        break;
    case 3:
        if (PrintDisplayMonInfo())
            sStorage->state++;
        else if (sStorage->inMenuInteraction)
            HideInfoPanelSprites();
        break;
    case 4:
        if (!IsDma3ManagerBusyWithBgCopy())
            SetPokeStorageTask(Task_PokeStorageMain);
        break;
    case 5:
        if (JOY_NEW(A_BUTTON | B_BUTTON | DPAD_ANY))
        {
            ClearBottomWindow();
            SetPokeStorageTask(Task_PokeStorageMain);
        }
        break;
    }
}

static void Task_SwitchSelectedItem(u8 taskId)
{
    switch (sStorage->state)
    {
    case 0:
        if (!ItemIsMail(sStorage->displayMon.heldItem))
        {
            ClearBottomWindow();
            sStorage->state++;
        }
        else
        {
            SetPokeStorageTask(Task_PrintCantStoreMail);
        }
        break;
    case 1:
        StartCursorAnim(CURSOR_ANIM_MAIN);
        SwapItemsWithMon(sCursorArea, GetCursorPosition());
        sStorage->state++;
        break;
    case 2:
        if (!IsItemIconAnimActive())
        {
            StartCursorAnim(CURSOR_ANIM_MAIN);
            RefreshDisplayMon();
            sStorage->displayMonInfoLoadState = 0;
            sStorage->state++;
        }
        break;
    case 3:
        if (PrintDisplayMonInfo())
        {
            SetPokeStorageTask(Task_PokeStorageMain);
        }
        else if (sStorage->inMenuInteraction)
        {
            HideInfoPanelSprites();
        }
        break;
    }
}

static void Task_CloseBoxWhileHoldingItem(u8 taskId)
{
    switch (sStorage->state)
    {
    case 0:
        PlaySE(SE_SELECT);
        PrintMessage(MSG_PUT_IN_BAG);
        ShowYesNoWindow(0);
        sStorage->state = 1;
        break;
    case 1:
        switch (ProcessHgssYesNoInput())
        {
        case MENU_B_PRESSED:
        case 1: // No
            ClearBottomWindow();
            SetPokeStorageTask(Task_PokeStorageMain);
            break;
        case 0:// Yes
            if (AddBagItem(sStorage->movingItemId, 1) == TRUE)
            {
                ClearBottomWindow();
                sStorage->state = 3;
            }
            else
            {
                PrintMessage(MSG_BAG_FULL);
                sStorage->state = 2;
            }
            break;
        }
        break;
    case 2:
        if (JOY_NEW(A_BUTTON | B_BUTTON | DPAD_ANY))
        {
            ClearBottomWindow();
            sStorage->state = 5;
        }
        break;
    case 3:
        MoveItemFromCursorToBag();
        sStorage->state = 4;
        break;
    case 4:
        if (!IsItemIconAnimActive())
        {
            StartCursorAnim(CURSOR_ANIM_BOUNCE);
            SetPokeStorageTask(Task_PokeStorageMain);
        }
        break;
    case 5:
        if (!IsDma3ManagerBusyWithBgCopy())
            SetPokeStorageTask(Task_PokeStorageMain);
        break;
    }
}

static void Task_HandleMovingMonFromParty(u8 taskId)
{
    switch (sStorage->state)
    {
    case 0:
        CompactPartySlots();
        CompactPartySprites();
        sStorage->state++;
        break;
    case 1:
        if (GetNumPartySpritesCompacting() == 0)
        {
            SetPokeStorageTask(Task_PokeStorageMain);
        }
        break;
    }
}

static void Task_PrintCantStoreMail(u8 taskId)
{
    switch (sStorage->state)
    {
    case 0:
        PrintMessage(MSG_CANT_STORE_MAIL);
        sStorage->state++;
        break;
    case 1:
        if (!IsDma3ManagerBusyWithBgCopy())
            sStorage->state++;
        break;
    case 2:
        if (JOY_NEW(A_BUTTON | B_BUTTON | DPAD_ANY))
        {
            ClearBottomWindow();
            sStorage->state++;
        }
        break;
    case 3:
        if (!IsDma3ManagerBusyWithBgCopy())
            SetPokeStorageTask(Task_PokeStorageMain);
        break;
    }
}

// Handle options menu that shows when the box title bar is selected
static void Task_HandleBoxOptions(u8 taskId)
{
    switch (sStorage->state)
    {
    case 0:
        AddMenu();
        sStorage->state++;
        break;
    case 1:
        if (IsMenuLoading())
            return;
        sStorage->state++;
    case 2:
        switch (HandleMenuInput())
        {
        case MENU_B_PRESSED:
        case MENU_CANCEL:
            SetPokeStorageTask(Task_PokeStorageMain);
            break;
        case MENU_NAME:
            PlaySE(SE_SELECT);
            SetPokeStorageTask(Task_NameBox);
            break;
        case MENU_WALLPAPER:
            PlaySE(SE_SELECT);
            SetPokeStorageTask(Task_HandleWallpapers);
            break;
        case MENU_JUMP:
            PlaySE(SE_SELECT);
            SetPokeStorageTask(Task_JumpBox);
            break;
        }
        break;
    }
}

static void Task_HandleWallpapers(u8 taskId)
{
    s32 input;
    u8 maxPage = (MENU_COUNT - MENU_BASE - 1) / HGSS_WALLPAPER_SELECTOR_ITEMS_PER_PAGE;

    switch (sStorage->state)
    {
    case 0:
        sStorage->listMenuScrollRow = 0;
        sStorage->listMenuSelectedRow = 0;
        AddWallpaperMenu();
        sStorage->state++;
        break;
    case 1:
        if (JOY_NEW(DPAD_LEFT) && sStorage->listMenuScrollRow > 0)
        {
            PlaySE(SE_SELECT);
            sStorage->listMenuScrollRow--;
            sStorage->listMenuSelectedRow = 0;
            DestroyListMenuTask(sStorage->listMenuTaskId, NULL, NULL);
            FreeHgssWallpaperSelectorSprites();
            RemoveMenu();
            AddWallpaperMenu();
        }
        else if (JOY_NEW(DPAD_RIGHT) && sStorage->listMenuScrollRow < maxPage)
        {
            PlaySE(SE_SELECT);
            sStorage->listMenuScrollRow++;
            sStorage->listMenuSelectedRow = 0;
            DestroyListMenuTask(sStorage->listMenuTaskId, NULL, NULL);
            FreeHgssWallpaperSelectorSprites();
            RemoveMenu();
            AddWallpaperMenu();
        }
        else
        {
            input = ListMenu_ProcessInput(sStorage->listMenuTaskId);
            ListMenuGetScrollAndRow(sStorage->listMenuTaskId, NULL, &sStorage->listMenuSelectedRow);
            if (input != LIST_NOTHING_CHOSEN)
            {
                if (input == LIST_CANCEL)
                {
                    DestroyListMenuTask(sStorage->listMenuTaskId, NULL, NULL);
                    FreeHgssWallpaperSelectorSprites();
                    RemoveMenu();
                    ClearBottomWindow();
                    SetPokeStorageTask(Task_PokeStorageMain);
                }
                else
                {
                    PlaySE(SE_SELECT);
                    DestroyListMenuTask(sStorage->listMenuTaskId, NULL, NULL);
                    FreeHgssWallpaperSelectorSprites();
                    RemoveMenu();
                    sStorage->wallpaperId = input - MENU_BASE;
                    SetWallpaperForCurrentBox(sStorage->wallpaperId);
                    ClearBottomWindow();
                    SetPokeStorageTask(Task_WallpaperChange);
                }
            }
        }
        break;
    }
}

static void Task_WallpaperChange(u8 taskId)
{
    if (DoWallpaperGfxChange() == FALSE)
    {
        SetPokeStorageTask(Task_PokeStorageMain);
    }
}

static void Task_JumpBox(u8 taskId)
{
    switch (sStorage->state)
    {
    case 0:
        {
            u8 curBox = StorageGetCurrentBox();
            LoadChooseBoxMenuGfx(&sStorage->chooseBoxMenu);
            ChooseBoxMenu_CreateSprites(curBox);
        }
        sStorage->state++;
        break;
    case 1:
        sStorage->newCurrBoxId = HandleChooseBoxMenuInput();
        switch (sStorage->newCurrBoxId)
        {
        case BOXID_NONE_CHOSEN:
            break;
        default:
            if (sStorage->newCurrBoxId != BOXID_CANCELED)
                PlaySE(SE_SELECT);

            ClearBottomWindow();
            ChooseBoxMenu_DestroySprites();
            FreeChooseBoxMenu();

            // The authenticated HGSS overview owns candidate selection.
            // Only a confirmed different box enters the normal Storage
            // transition; browsing thumbnails never scrolls the legacy
            // full-box presentation underneath the chooser.
            if (sStorage->newCurrBoxId == BOXID_CANCELED || sStorage->newCurrBoxId == StorageGetCurrentBox())
                sStorage->state = 4;
            else
                sStorage->state = 2;
            break;
        }
        break;
    case 2:
        if (!UpdateCursorPos())
        {
            SetUpScrollToBox(sStorage->newCurrBoxId);
            sStorage->state++;
        }
        break;
    case 3:
        if (!ScrollToBox())
        {
            SetCurrentBox(sStorage->newCurrBoxId);
            SetPokeStorageTask(Task_PokeStorageMain);
        }
        break;
    case 4:
        if (!UpdateCursorPos())
            SetPokeStorageTask(Task_PokeStorageMain);
        break;
    }
}

static void Task_NameBox(u8 taskId)
{
    switch (sStorage->state)
    {
    case 0:
        SaveMovingMon();
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        sStorage->state++;
        break;
    case 1:
        if (!UpdatePaletteFade())
        {
            sWhichToReshow = SCREEN_CHANGE_NAME_BOX - 1;
            sStorage->screenChangeType = SCREEN_CHANGE_NAME_BOX;
            SetPokeStorageTask(Task_ChangeScreen);
        }
        break;
    }
}

static void Task_ShowMonSummary(u8 taskId)
{
    switch (sStorage->state)
    {
    case 0:
        InitSummaryScreenData();
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        sStorage->state++;
        break;
    case 1:
        if (!UpdatePaletteFade())
        {
            sWhichToReshow = SCREEN_CHANGE_SUMMARY_SCREEN - 1;
            sStorage->screenChangeType = SCREEN_CHANGE_SUMMARY_SCREEN;
            SetPokeStorageTask(Task_ChangeScreen);
        }
        break;
    }
}

static void Task_GiveItemFromBag(u8 taskId)
{
    switch (sStorage->state)
    {
    case 0:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        sStorage->state++;
        break;
    case 1:
        if (!UpdatePaletteFade())
        {
            sWhichToReshow = SCREEN_CHANGE_ITEM_FROM_BAG - 1;
            sStorage->screenChangeType = SCREEN_CHANGE_ITEM_FROM_BAG;
            SetPokeStorageTask(Task_ChangeScreen);
            sJustOpenedBag = TRUE;
        }
        break;
    }
}

static void Task_OnBPressed(u8 taskId)
{
    switch (sStorage->state)
    {
    case 0:
        if (IsMonBeingMoved())
        {
            if (OW_PC_PRESS_B < GEN_4)
            {
                PlaySE(SE_FAILURE);
                PrintMessage(MSG_HOLDING_POKE);
                sStorage->state = 1;
            }
            else if (CanPlaceMon())
            {
                PlaySE(SE_SELECT);
                SetPokeStorageTask(Task_PlaceMon);
            }
            else
            {
                SetPokeStorageTask(Task_PokeStorageMain);
            }
        }
        else if (IsMovingItem())
        {
            SetPokeStorageTask(Task_CloseBoxWhileHoldingItem);
        }
        else
        {
            PlaySE(SE_SELECT);
            PrintMessage(MSG_CONTINUE_BOX);
            ShowYesNoWindow(0);
            sStorage->state = 2;
        }
        break;
    case 1:
        if (JOY_NEW(A_BUTTON | B_BUTTON | DPAD_ANY))
        {
            ClearBottomWindow();
            SetPokeStorageTask(Task_PokeStorageMain);
        }
        break;
    case 2:
        switch (ProcessHgssYesNoInput())
        {
        case 0:
            ClearBottomWindow();
            SetPokeStorageTask(Task_PokeStorageMain);
            break;
        case 1:
        case MENU_B_PRESSED:
            PlaySE(SE_PC_OFF);
            ClearBottomWindow();
            sStorage->state++;
            break;
        }
        break;
    case 3:
        ComputerScreenCloseEffect(20, 0, 0);
        sStorage->state++;
        break;
    case 4:
        if (!IsComputerScreenCloseEffectActive())
        {
            UpdateBoxToSendMons();
            gPartiesCount[B_TRAINER_PLAYER] = CalculatePlayerPartyCount();
            if (sStorage->boxOption == OPTION_SELECT_MON)
            {
                gSpecialVar_0x8004  = PARTY_NOTHING_CHOSEN;
                gSpecialVar_Result  = FALSE;
            }
            sStorage->screenChangeType = SCREEN_CHANGE_EXIT_BOX;
            SetPokeStorageTask(Task_ChangeScreen);
        }
        break;
    }
}

static void Task_ChangeScreen(u8 taskId)
{
    struct BoxPokemon *boxMons;
    u8 mode, monIndex, maxMonIndex;
    u8 screenChangeType = sStorage->screenChangeType;

    if (sStorage->boxOption == OPTION_MOVE_ITEMS && IsMovingItem() == TRUE)
        sMovingItemId = GetMovingItemId();
    else
        sMovingItemId = ITEM_NONE;

    switch (screenChangeType)
    {
    case SCREEN_CHANGE_EXIT_BOX:
    default:
        if (sStorage->boxOption == OPTION_SELECT_MON)
            SetMainCallback2(CB2_ReturnToFieldContinueScript);
        else
            SetMainCallback2(CB2_ExitPokeStorage);
        FreePokeStorageData();
        break;
    case SCREEN_CHANGE_SUMMARY_SCREEN:
        boxMons = sStorage->summaryMon.box;
        monIndex = sStorage->summaryStartPos;
        maxMonIndex = sStorage->summaryMaxPos;
        mode = sStorage->summaryScreenMode;
        FreePokeStorageData();
        ShowPokemonSummaryScreen(mode, boxMons, monIndex, maxMonIndex, CB2_ReturnToPokeStorage);
        break;
    case SCREEN_CHANGE_NAME_BOX:
        FreePokeStorageData();
        DoNamingScreen(NAMING_SCREEN_BOX, GetBoxNamePtr(StorageGetCurrentBox()), 0, 0, 0, CB2_ReturnToPokeStorage);
        break;
    case SCREEN_CHANGE_ITEM_FROM_BAG:
        FreePokeStorageData();
        GoToBagMenu(ITEMMENULOCATION_PCBOX, 0, CB2_ReturnToPokeStorage);
        break;
    }

    DestroyTask(taskId);
}

static void GiveChosenBagItem(void)
{
    enum Item itemId = gSpecialVar_ItemId;

    if (itemId != ITEM_NONE)
    {
        u8 pos = GetCursorPosition();
        if (sCursorArea == CURSOR_AREA_IN_PARTY)
        {
            struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][pos];
            SetMonData(&gParties[B_TRAINER_PLAYER][pos], MON_DATA_HELD_ITEM, &itemId);
            SetMonFormPSS_ItemHold(&mon->box);
        }
        else
        {
            SetCurrentBoxMonData(pos, MON_DATA_HELD_ITEM, &itemId);
            SetMonFormPSS_ItemHold(&gPokemonStoragePtr->boxes[StorageGetCurrentBox()][pos]);
        }

        RemoveBagItem(itemId, 1);
    }
}

static void FreePokeStorageData(void)
{
    MultiMove_Free();
    FREE_AND_SET_NULL(sStorage);
    FreeAllWindowBuffers();
}


//------------------------------------------------------------------------------
//  SECTION: Misc
//
//  No real uniform section below. Misc functions including more initialization,
//  showing/hiding the party menu, updating the Close Box button, printing
//  messages, doing the mosaic effect when transitioning between Pokémon, etc.
//------------------------------------------------------------------------------


#define HGSS_STORAGE_WALLPAPER_LEFT       9
#define HGSS_STORAGE_WALLPAPER_TOP        0
#define HGSS_STORAGE_WALLPAPER_WIDTH      21
#define HGSS_STORAGE_WALLPAPER_HEIGHT     20
#define HGSS_STORAGE_WALLPAPER_BASE_TILE  1
#define HGSS_STORAGE_WALLPAPER_PAL_BANK   1

static bool32 LoadHgssStorageWallpaper(u8 wallpaperId)
{
    u8 x, y;
    u16 *tilemap = sStorage->hgssWallpaperTilemapBuffer;
    static const ALIGNED(4) u8 sBlankTile[32] = {0};

    if (wallpaperId >= WALLPAPER_COUNT)
        wallpaperId = WALLPAPER_BASE;

    CpuFill16(0, tilemap, sizeof(sStorage->hgssWallpaperTilemapBuffer));
    SetBgTilemapBuffer(3, tilemap);

    // One complete authentic HGSS composition occupies 420 4bpp tiles.
    // BG3 charblock 3 has room for tiles 0-447 before screenblock 31, so the
    // full 21x20 source fits at 1:1 scale with no crop, deduplication, or redraw.
    LoadBgTiles(3, sBlankTile, sizeof(sBlankTile), 0);
    LoadBgTiles(
        3,
        sHgssStorageWallpaperGfx[wallpaperId],
        sizeof(sHgssStorageWallpaper01_Gfx),
        HGSS_STORAGE_WALLPAPER_BASE_TILE);
    LoadPalette(
        sHgssStorageWallpaperPal[wallpaperId],
        BG_PLTT_ID(HGSS_STORAGE_WALLPAPER_PAL_BANK),
        PLTT_SIZE_4BPP);

    for (y = 0; y < HGSS_STORAGE_WALLPAPER_HEIGHT; y++)
    {
        for (x = 0; x < HGSS_STORAGE_WALLPAPER_WIDTH; x++)
        {
            tilemap[(HGSS_STORAGE_WALLPAPER_TOP + y) * 32
                  + HGSS_STORAGE_WALLPAPER_LEFT + x] =
                (HGSS_STORAGE_WALLPAPER_BASE_TILE
                 + y * HGSS_STORAGE_WALLPAPER_WIDTH + x)
                | (HGSS_STORAGE_WALLPAPER_PAL_BANK << 12);
        }
    }

    ShowBg(3);
    ScheduleBgCopyTilemapToVram(3);
    return TRUE;
}

static void SetScrollingBackground(void)
{
    SetGpuReg(REG_OFFSET_BG3CNT, BGCNT_PRIORITY(3) | BGCNT_CHARBASE(3) | BGCNT_16COLOR | BGCNT_SCREENBASE(31));
    LoadHgssStorageWallpaper(GetBoxWallpaper(StorageGetCurrentBox()));
}

/*
static void ScrollBackground(void)
{
    ChangeBgX(3, 64, BG_COORD_ADD);
    ChangeBgY(3, 64, BG_COORD_ADD);
}
*/

static void LoadPokeStorageMenuGfx(void)
{
    InitBgsFromTemplates(0, sBgTemplates, ARRAY_COUNT(sBgTemplates));

    // The authentic HGSS Storage composition owns BG1 now. Start from an
    // explicit transparent tilemap instead of decompressing the removed SWSH
    // frame underneath it.
    CpuFill16(0, sStorage->displayMenuTilemapBuffer, sizeof(sStorage->displayMenuTilemapBuffer));
    SetBgTilemapBuffer(1, sStorage->displayMenuTilemapBuffer);
    DrawHgssStorageMainFrame();
    ShowBg(1);
    ScheduleBgCopyTilemapToVram(1);

}

static bool8 InitPokeStorageWindows(void)
{
    if (!InitWindows(sWindowTemplates))
    {
        return FALSE;
    }
    else
    {
        DeactivateAllTextPrinters();
        return TRUE;
    }
}

static bool8 InitPalettesAndSprites(void)
{
    switch (sStorage->graphicsLoadState)
    {
    case 0:
        LoadPalette(sTextWindows_Pal, BG_PLTT_ID(15), sizeof(sTextWindows_Pal));
        sStorage->graphicsLoadState++;
        break;
    case 1:
        LoadPalette(sTypeIcons_Pal, OBJ_PLTT_ID(13), 3 * PLTT_SIZE_4BPP);
        sStorage->graphicsLoadState++;
        break;
    case 2:
        LoadSpriteSheet(&sSpriteSheet_TypeIcons);
        sStorage->graphicsLoadState++;
        break;
    case 3:
        LoadCompressedSpriteSheet(&sSpriteSheet_StatLabels);
        sStorage->graphicsLoadState++;
        break;
    case 4:
        LoadCompressedSpriteSheet(&sSpriteSheet_GenderIcons);
        sStorage->graphicsLoadState++;
        break;
    case 5:
        LoadCompressedSpriteSheet(&sSpriteSheet_ShinyIcon);
        sStorage->graphicsLoadState++;
        break;
    case 6:
        LoadHgssStorageCursorGfx();
        sStorage->graphicsLoadState++;
        break;
    case 7:
        LoadSpritePalettes(sSpritePal_StatLabels);
        LoadSpritePalettes(sSpritePal_HgssGenderGlyphs);
        LoadSpritePalettes(sSpritePal_HgssShinyStar);
        LoadSpritePalettes(sSpritePal_HgssPokerusSymbol);
        sStorage->graphicsLoadState++;
        break;
    case 8:
        LoadCompressedSpriteSheet(&sSpriteSheet_PokerusIcon);
        sStorage->graphicsLoadState = 0;
        return TRUE;
    }
    return FALSE;
}

static void CreateMarkingComboSprite(void)
{
    sStorage->markingComboSprite = CreateMonMarkingComboSprite(GFXTAG_MARKING_COMBO, PALTAG_MARKING_COMBO, sMarkings_Pal);
    sStorage->markingComboSprite->oam.priority = 0;
    sStorage->markingComboSprite->subpriority = 1;
    sStorage->markingComboSprite->x = 52 + (136 * sStorage->monInfoTilemapId);
    sStorage->markingComboSprite->y = 150;
    sStorage->markingComboTilesPtr = (void *) OBJ_VRAM0 + 32 * GetSpriteTileStartByTag(GFXTAG_MARKING_COMBO);
}

static void HideInfoPanelSprites(void)
{
    if (sStorage->genderIconSprite != NULL)
        sStorage->genderIconSprite->invisible = TRUE;
    if (sStorage->typeIconSprites[0] != NULL)
        sStorage->typeIconSprites[0]->invisible = TRUE;
    if (sStorage->typeIconSprites[1] != NULL)
        sStorage->typeIconSprites[1]->invisible = TRUE;
    if (sStorage->shinyIconSprite != NULL)
        sStorage->shinyIconSprite->invisible = TRUE;
	if (sStorage->pokerusIconSprite != NULL)
        sStorage->pokerusIconSprite->invisible = TRUE;
    if (sStorage->statLabelSprites[0] != NULL)
        sStorage->statLabelSprites[0]->invisible = TRUE;
    if (sStorage->statLabelSprites[1] != NULL)
        sStorage->statLabelSprites[1]->invisible = TRUE;
    if (sStorage->markingComboSprite != NULL)
        sStorage->markingComboSprite->invisible = TRUE;
}

static void UpdateMarkingComboSprite(void)
{
    if (!sStorage->showMonInfo
        || sStorage->displayMon.species == SPECIES_NONE
        || (GetSpeciesAtCursorPosition() == SPECIES_NONE && !sIsMonBeingMoved))
    {
        if (sStorage->markingComboSprite != NULL)
            sStorage->markingComboSprite->invisible = TRUE;
        return;
    }

    u8 spriteX = 52 + (136 * sStorage->monInfoTilemapId);
    u8 spriteY = 150;

    if (sStorage->markingComboSprite == NULL)
    {
        CreateMarkingComboSprite();
    }
    else
    {
        sStorage->markingComboSprite->x = spriteX;
        sStorage->markingComboSprite->y = spriteY;
        sStorage->markingComboSprite->invisible = FALSE;
    }

    if (sStorage->markingComboSprite != NULL)
        UpdateMonMarkingTiles(sStorage->displayMon.markings, sStorage->markingComboTilesPtr);
}

static void RefreshDisplayMonData(void)
{
    sStorage->displayMonInfoLoadState = 0;
    while (!PrintDisplayMonInfo());
    ScheduleBgCopyTilemapToVram(0);
}

static void UpdateGenderIconSprite(u8 fontId)
{
    if (!sStorage->showMonInfo
        || sStorage->displayMon.species == SPECIES_NONE
        || (GetSpeciesAtCursorPosition() == SPECIES_NONE && !sIsMonBeingMoved))
    {
        if (sStorage->genderIconSprite != NULL)
            sStorage->genderIconSprite->invisible = TRUE;
        return;
    }

    if (sStorage->displayMon.gender != MON_GENDERLESS && !sStorage->displayMon.isEgg)
    {
        u16 nicknameWidth = GetStringWidth(fontId, sStorage->displayMon.nickname, 0);
        // The old expansion symbol was 8 px wide. HGSS font ID 4 uses a
        // 16x16 glyph cell, so keep the same left edge by moving the sprite
        // center 4 px right.
        u8 spriteX = 6 + nicknameWidth + 1 + 8 + (136 * sStorage->monInfoTilemapId);
        u8 spriteY = 32;

        if (sStorage->genderIconSprite == NULL)
        {
            sStorage->genderIconSprite = &gSprites[CreateSprite(&sSpriteTemplate_GenderIcons, spriteX, spriteY, 0)];
        }
        else
        {
            sStorage->genderIconSprite->x = spriteX;
            sStorage->genderIconSprite->y = spriteY;
            sStorage->genderIconSprite->invisible = FALSE;
        }
        StartSpriteAnim(sStorage->genderIconSprite, (sStorage->displayMon.gender == MON_MALE) ? 1 : 0);
    }
    else if (sStorage->genderIconSprite != NULL)
    {
        sStorage->genderIconSprite->invisible = TRUE;
    }
}

static void UpdateShinyIconSprite(void)
{
    if (!sStorage->showMonInfo
        || sStorage->displayMon.species == SPECIES_NONE
        || (GetSpeciesAtCursorPosition() == SPECIES_NONE && !sIsMonBeingMoved))
    {
        if (sStorage->shinyIconSprite != NULL)
            sStorage->shinyIconSprite->invisible = TRUE;
        return;
    }

    if (sStorage->displayMon.isShiny && !sStorage->displayMon.isEgg)
    {
        u8 spriteX = 82 + (136 * sStorage->monInfoTilemapId);
        u8 spriteY = 50;

        if (sStorage->shinyIconSprite == NULL)
        {
            sStorage->shinyIconSprite = &gSprites[CreateSprite(&sSpriteTemplate_ShinyIcon, spriteX, spriteY, 0)];
        }
        else
        {
            sStorage->shinyIconSprite->x = spriteX;
            sStorage->shinyIconSprite->y = spriteY;
            sStorage->shinyIconSprite->invisible = FALSE;
        }
    }
    else if (sStorage->shinyIconSprite != NULL)
    {
        sStorage->shinyIconSprite->invisible = TRUE;
    }
}

static void UpdatePokerusIconSprite(void)
{
    if (!sStorage->showMonInfo
        || sStorage->displayMon.species == SPECIES_NONE
        || (GetSpeciesAtCursorPosition() == SPECIES_NONE && !sIsMonBeingMoved))
    {
        if (sStorage->pokerusIconSprite != NULL)
            sStorage->pokerusIconSprite->invisible = TRUE;
        return;
    }

    if (sStorage->displayMon.hasPokerus && !sStorage->displayMon.isEgg)
    {
        // Keep the prior Storage indicator center while reducing the live OBJ
        // from the legacy 32x8 banner to the native HGSS 8x8 symbol.
        u8 spriteX = 72 + (136 * sStorage->monInfoTilemapId);
        u8 spriteY = 150;

        if (sStorage->pokerusIconSprite == NULL)
        {
            sStorage->pokerusIconSprite = &gSprites[CreateSprite(&sSpriteTemplate_PokerusIcon, spriteX, spriteY, 0)];
        }
        else
        {
            sStorage->pokerusIconSprite->x = spriteX;
            sStorage->pokerusIconSprite->y = spriteY;
            sStorage->pokerusIconSprite->invisible = FALSE;
        }
    }
    else if (sStorage->pokerusIconSprite != NULL)
    {
        sStorage->pokerusIconSprite->invisible = TRUE;
    }
}

static void UpdateTypeIconTiles(u8 typeId, void *dest)
{
    u32 offset = (typeId + 1) * 0x100; // +1 to skip placeholder tiles at start of graphics
    RequestDma3Copy(&sTypeIcons_Gfx[offset], dest, 0x100, 0x10);
}

static void SpriteCB_TypeIcon(struct Sprite *sprite)
{
    if (sprite->data[0] != 0xFF)
    {
        u8 typeId = sprite->data[0];
        u8 index = sprite->data[1];
        UpdateTypeIconTiles(typeId, sStorage->typeIconTilesPtr[index]);
        sprite->data[0] = 0xFF;
    }
}

static void UpdateTypeIconsSprite(void)
{
    enum Species species = sStorage->displayMon.species;
    enum Type type1, type2;
    u8 spriteX1, spriteX2, spriteY;

    if (!sStorage->showMonInfo
        || species == SPECIES_NONE
        || (GetSpeciesAtCursorPosition() == SPECIES_NONE && !sIsMonBeingMoved))
    {
        if (sStorage->typeIconSprites[0] != NULL)
            sStorage->typeIconSprites[0]->invisible = TRUE;
        if (sStorage->typeIconSprites[1] != NULL)
            sStorage->typeIconSprites[1]->invisible = TRUE;
        return;
    }

    if (sStorage->displayMon.isEgg)
    {
        type1 = TYPE_MYSTERY;
        type2 = TYPE_MYSTERY;
    }
    else
    {
        type1 = gSpeciesInfo[species].types[0];
        type2 = gSpeciesInfo[species].types[1];
    }

    spriteX1 = 20 + (136 * sStorage->monInfoTilemapId);
    spriteX2 = 56 + (136 * sStorage->monInfoTilemapId);
    spriteY = 50;

    if (sStorage->typeIconSprites[0] == NULL)
    {
        sStorage->typeIconSprites[0] = &gSprites[CreateSprite(&sSpriteTemplate_TypeIcons, spriteX1, spriteY, 0)];
        sStorage->typeIconTilesPtr[0] = (void *) OBJ_VRAM0 + 32 * GetSpriteTileStartByTag(GFXTAG_TYPE_ICON);
        sStorage->typeIconSprites[0]->callback = SpriteCB_TypeIcon;
        sStorage->typeIconSprites[0]->data[1] = 0;
    }
    else
    {
        sStorage->typeIconSprites[0]->x = spriteX1;
        sStorage->typeIconSprites[0]->y = spriteY;
    }

    if (type1 < NUMBER_OF_MON_TYPES)
        sStorage->typeIconSprites[0]->oam.paletteNum = gTypesInfo[type1].palette;
    sStorage->typeIconSprites[0]->data[0] = type1;
    sStorage->typeIconSprites[0]->invisible = FALSE;

    if (!sStorage->displayMon.isEgg && type1 != type2)
    {
        if (sStorage->typeIconSprites[1] == NULL)
        {
            sStorage->typeIconSprites[1] = &gSprites[CreateSprite(&sSpriteTemplate_TypeIcons, spriteX2, spriteY, 0)];
            sStorage->typeIconTilesPtr[1] = (void *) OBJ_VRAM0 + 32 * (GetSpriteTileStartByTag(GFXTAG_TYPE_ICON) + 8);
            sStorage->typeIconSprites[1]->oam.tileNum += 8;
            sStorage->typeIconSprites[1]->callback = SpriteCB_TypeIcon;
            sStorage->typeIconSprites[1]->data[1] = 1;
        }
        else
        {
            sStorage->typeIconSprites[1]->x = spriteX2;
            sStorage->typeIconSprites[1]->y = spriteY;
        }

        if (type2 < NUMBER_OF_MON_TYPES)
            sStorage->typeIconSprites[1]->oam.paletteNum = gTypesInfo[type2].palette;
        sStorage->typeIconSprites[1]->data[0] = type2;
        sStorage->typeIconSprites[1]->invisible = FALSE;
    }
    else if (sStorage->typeIconSprites[1] != NULL)
    {
        sStorage->typeIconSprites[1]->invisible = TRUE;
    }
}

static void UpdateStatLabelsSprites(void)
{
    enum Species species = sStorage->displayMon.species;
    u8 natureUpStat, natureDownStat;
    u8 upStatAnimIndex, downStatAnimIndex;
    u8 upStatX, upStatY, downStatX, downStatY;
    u16 upStatPalTag, downStatPalTag;
    u8 nature;
    u8 animIndexMap[NUM_STATS] = {0, 0, 1, 4, 2, 3};

    if (!sStorage->showMonInfo
        || species == SPECIES_NONE
        || sStorage->displayMon.isEgg
        || (GetSpeciesAtCursorPosition() == SPECIES_NONE && !sIsMonBeingMoved))
    {
        if (sStorage->statLabelSprites[0] != NULL)
            sStorage->statLabelSprites[0]->invisible = TRUE;
        if (sStorage->statLabelSprites[1] != NULL)
            sStorage->statLabelSprites[1]->invisible = TRUE;
        return;
    }

    nature = GetNatureFromPersonality(sStorage->displayMon.personality);
    natureUpStat = gNaturesInfo[nature].statUp;
    natureDownStat = gNaturesInfo[nature].statDown;

    if (natureUpStat == natureDownStat)
    {
        if (sStorage->statLabelSprites[0] != NULL)
            sStorage->statLabelSprites[0]->invisible = TRUE;
        if (sStorage->statLabelSprites[1] != NULL)
            sStorage->statLabelSprites[1]->invisible = TRUE;
        return;
    }

    switch (natureUpStat)
    {
    case STAT_ATK:
        upStatX = 52 + (136 * sStorage->monInfoTilemapId);
        upStatY = 68;
        upStatAnimIndex = animIndexMap[STAT_ATK];
        break;
    case STAT_DEF:
        upStatX = 12 + (136 * sStorage->monInfoTilemapId);
        upStatY = 84;
        upStatAnimIndex = animIndexMap[STAT_DEF];
        break;
    case STAT_SPATK:
        upStatX = 52 + (136 * sStorage->monInfoTilemapId);
        upStatY = 84;
        upStatAnimIndex = animIndexMap[STAT_SPATK];
        break;
    case STAT_SPDEF:
        upStatX = 12 + (136 * sStorage->monInfoTilemapId);
        upStatY = 100;
        upStatAnimIndex = animIndexMap[STAT_SPDEF];
        break;
    case STAT_SPEED:
        upStatX = 52 + (136 * sStorage->monInfoTilemapId);
        upStatY = 100;
        upStatAnimIndex = animIndexMap[STAT_SPEED];
        break;
    default:
        return;
    }

    switch (natureDownStat)
    {
    case STAT_ATK:
        downStatX = 52 + (136 * sStorage->monInfoTilemapId);
        downStatY = 68;
        downStatAnimIndex = animIndexMap[STAT_ATK];
        break;
    case STAT_DEF:
        downStatX = 12 + (136 * sStorage->monInfoTilemapId);
        downStatY = 84;
        downStatAnimIndex = animIndexMap[STAT_DEF];
        break;
    case STAT_SPATK:
        downStatX = 52 + (136 * sStorage->monInfoTilemapId);
        downStatY = 84;
        downStatAnimIndex = animIndexMap[STAT_SPATK];
        break;
    case STAT_SPDEF:
        downStatX = 12 + (136 * sStorage->monInfoTilemapId);
        downStatY = 100;
        downStatAnimIndex = animIndexMap[STAT_SPDEF];
        break;
    case STAT_SPEED:
        downStatX = 52 + (136 * sStorage->monInfoTilemapId);
        downStatY = 100;
        downStatAnimIndex = animIndexMap[STAT_SPEED];
        break;
    default:
        return;
    }

    upStatPalTag = PALTAG_MISC_1;
    downStatPalTag = PALTAG_MISC_2;

    if (sStorage->statLabelSprites[0] == NULL)
    {
        struct SpriteTemplate template = sSpriteTemplate_StatLabels;
        template.paletteTag = upStatPalTag;
        sStorage->statLabelSprites[0] = &gSprites[CreateSprite(&template, upStatX, upStatY, 0)];
    }
    else
    {
        sStorage->statLabelSprites[0]->x = upStatX;
        sStorage->statLabelSprites[0]->y = upStatY;
    }

    StartSpriteAnim(sStorage->statLabelSprites[0], upStatAnimIndex);
    sStorage->statLabelSprites[0]->oam.paletteNum = IndexOfSpritePaletteTag(upStatPalTag);
    sStorage->statLabelSprites[0]->invisible = FALSE;

    if (sStorage->statLabelSprites[1] == NULL)
    {
        struct SpriteTemplate template = sSpriteTemplate_StatLabels;
        template.paletteTag = downStatPalTag;
        sStorage->statLabelSprites[1] = &gSprites[CreateSprite(&template, downStatX, downStatY, 0)];
    }
    else
    {
        sStorage->statLabelSprites[1]->x = downStatX;
        sStorage->statLabelSprites[1]->y = downStatY;
    }

    StartSpriteAnim(sStorage->statLabelSprites[1], downStatAnimIndex);
    sStorage->statLabelSprites[1]->oam.paletteNum = IndexOfSpritePaletteTag(downStatPalTag);
    sStorage->statLabelSprites[1]->invisible = FALSE;
}

static void BufferAndPrintStat(u8 windowId, u8 font, u8 xOffset, u8 y, u16 statValue)
{
    u8 statStr[4];
    u16 statWidth;

    ConvertIntToDecimalStringN(statStr, statValue, STR_CONV_MODE_LEFT_ALIGN, 3);
    statWidth = GetStringWidth(font, statStr, 0);
    AddTextPrinterParameterized4(windowId, font, xOffset - statWidth, y, 0, 0, sTextColors[0], 0, statStr);
}

static void PrintDisplayMonAbility(u8 font)
{
    u8 windowId;
    const u8 *abilityName;
    u8 windowWidthPx = 76;
    u8 xPos = 3;
    u8 fontId;

    if (sStorage->monInfoTilemapId == 0)
        windowId = WIN_MON_INFO_ABILITY_LEFT;
    else
        windowId = WIN_MON_INFO_ABILITY_RIGHT;
    FillWindowPixelBuffer(windowId, PIXEL_FILL(1));

    if (!sStorage->showMonInfo
        || (GetSpeciesAtCursorPosition() == SPECIES_NONE && !sIsMonBeingMoved)
        || sStorage->displayMon.isEgg)
    {
        CopyWindowToVram(windowId, COPYWIN_FULL);
        return;
    }

    abilityName = gAbilitiesInfo[sStorage->displayMon.ability].name;
    fontId = GetFontIdToFit(abilityName, font, 0, windowWidthPx - 2);

    AddTextPrinterParameterized4(windowId, fontId, xPos, 1, 0, 0, sTextColors[0], 0, abilityName);
    PutWindowTilemap(windowId);
    CopyWindowToVram(windowId, COPYWIN_FULL);
}

static void PrintDisplayMonHeldItem(u8 font)
{
    u8 windowId;
    const u8 *itemName;
    u8 windowWidthPx = 76;
    u8 xPos = 3;
    u8 fontId;

    if (sStorage->monInfoTilemapId == 0)
        windowId = WIN_MON_INFO_ITEM_LEFT;
    else
        windowId = WIN_MON_INFO_ITEM_RIGHT;
    FillWindowPixelBuffer(windowId, PIXEL_FILL(1));

    if (!sStorage->showMonInfo
        || (GetSpeciesAtCursorPosition() == SPECIES_NONE && !sIsMonBeingMoved)
        || sStorage->displayMon.isEgg)
    {
        CopyWindowToVram(windowId, COPYWIN_FULL);
        return;
    }

    if (sStorage->displayMon.heldItem == ITEM_NONE)
        itemName = gText_TwoDashes;
    else
        itemName = GetItemName(sStorage->displayMon.heldItem);
    fontId = GetFontIdToFit(itemName, font, 0, windowWidthPx - 2);

    AddTextPrinterParameterized4(windowId, fontId, xPos, 1, 0, 0, sTextColors[0], 0, itemName);
    PutWindowTilemap(windowId);
    CopyWindowToVram(windowId, COPYWIN_FULL);
}

static void PrintDisplayMonStats(u8 font)
{
    u8 col1WindowId, col2WindowId;
    u8 windowWidth = 3 * 8;

    if (sStorage->monInfoTilemapId == 0)
    {
        col1WindowId = WIN_MON_INFO_STATS_COL1_LEFT;
        col2WindowId = WIN_MON_INFO_STATS_COL2_LEFT;
    }
    else
    {
        col1WindowId = WIN_MON_INFO_STATS_COL1_RIGHT;
        col2WindowId = WIN_MON_INFO_STATS_COL2_RIGHT;
    }
    FillWindowPixelBuffer(col1WindowId, PIXEL_FILL(1));
    FillWindowPixelBuffer(col2WindowId, PIXEL_FILL(1));

    if (!sStorage->showMonInfo
        || (GetSpeciesAtCursorPosition() == SPECIES_NONE && !sIsMonBeingMoved)
        || sStorage->displayMon.isEgg)
    {
        CopyWindowToVram(col1WindowId, COPYWIN_FULL);
        CopyWindowToVram(col2WindowId, COPYWIN_FULL);
        return;
    }

    // COL1: HP (y=4), Def (y=20), SpDef (y=36)
    BufferAndPrintStat(col1WindowId, font, windowWidth - 1, 4, sStorage->displayMon.maxHP);
    BufferAndPrintStat(col1WindowId, font, windowWidth - 1, 20, sStorage->displayMon.def);
    BufferAndPrintStat(col1WindowId, font, windowWidth - 1, 36, sStorage->displayMon.spdef);

    // COL2: Atk (y=4), SpAtk (y=20), Speed (y=36)
    BufferAndPrintStat(col2WindowId, font, windowWidth - 5, 4, sStorage->displayMon.atk);
    BufferAndPrintStat(col2WindowId, font, windowWidth - 5, 20, sStorage->displayMon.spatk);
    BufferAndPrintStat(col2WindowId, font, windowWidth - 5, 36, sStorage->displayMon.speed);

    PutWindowTilemap(col1WindowId);
    PutWindowTilemap(col2WindowId);
    CopyWindowToVram(col1WindowId, COPYWIN_FULL);
    CopyWindowToVram(col2WindowId, COPYWIN_FULL);
}

static void PrintDisplayMonNickname(u8 font)
{
    u8 windowId;

    if (sStorage->showMonInfo && (GetSpeciesAtCursorPosition() != SPECIES_NONE || sIsMonBeingMoved))
    {
        if (sStorage->monInfoTilemapId == 0)
            windowId = WIN_MON_INFO_NICKNAME_LEFT;
        else
            windowId = WIN_MON_INFO_NICKNAME_RIGHT;
        FillWindowPixelBuffer(windowId, PIXEL_FILL(4));

        u8 fontId = GetFontIdToFit(sStorage->displayMon.nickname, font, 0, 40);
        AddTextPrinterParameterized4(windowId, fontId, 6, 1, 0, 0, sTextColors[1], 0, sStorage->displayMon.nickname);
        PutWindowTilemap(windowId);
        CopyWindowToVram(windowId, COPYWIN_FULL);
    }
}

static void PrintDisplayMonLevel(u8 font)
{
    u8 windowId;

    if (sStorage->showMonInfo
        && (GetSpeciesAtCursorPosition() != SPECIES_NONE || sIsMonBeingMoved)
        && !sStorage->displayMon.isEgg)
    {
        if (sStorage->monInfoTilemapId == 0)
            windowId = WIN_MON_INFO_LEVEL_LEFT;
        else
            windowId = WIN_MON_INFO_LEVEL_RIGHT;
        FillWindowPixelBuffer(windowId, PIXEL_FILL(4));

        u8 levelStr[4];
        ConvertIntToDecimalStringN(levelStr, sStorage->displayMon.level, STR_CONV_MODE_LEFT_ALIGN, 3);

        u16 lvWidth = GetStringWidth(font, sText_Lv, 0);
        u16 numWidth = GetStringWidth(font, levelStr, 0);
        u16 totalWidth = lvWidth + numWidth;
        u16 lvStartX = 28 - totalWidth;

        AddTextPrinterParameterized4(windowId, FONT_SHORT_NARROWER, lvStartX, 1, 0, 0, sTextColors[1], 0, sText_Lv);
        AddTextPrinterParameterized4(windowId, font, lvStartX + lvWidth, 1, 0, 0, sTextColors[1], 0, levelStr);
        PutWindowTilemap(windowId);
        CopyWindowToVram(windowId, COPYWIN_FULL);
    }
}

static bool8 PrintDisplayMonInfo(void)
{
    u8 font = FONT_SHORT_NARROW;

    if (sCursorMode == CURSOR_MODE_MULTI_MOVE)
    {
        HideInfoPanelSprites();
        return TRUE;
    }

    if (sStorage->showMonInfo && (GetSpeciesAtCursorPosition() != SPECIES_NONE || sIsMonBeingMoved))
    {
        switch (sStorage->displayMonInfoLoadState)
        {
        case 0:
            u8 fontId = GetFontIdToFit(sStorage->displayMon.nickname, font, 0, 40);
            UpdateGenderIconSprite(fontId);
            sStorage->displayMonInfoLoadState++;
            break;
        case 1:
            UpdateTypeIconsSprite();
            UpdateStatLabelsSprites();
            sStorage->displayMonInfoLoadState++;
            break;
        case 2:
            UpdateShinyIconSprite();
			UpdatePokerusIconSprite();
            sStorage->displayMonInfoLoadState++;
            break;
        case 3:
            PrintDisplayMonNickname(font);
            sStorage->displayMonInfoLoadState++;
            break;
        case 4:
            PrintDisplayMonLevel(font);
            sStorage->displayMonInfoLoadState++;
            break;
        case 5:
            PrintDisplayMonStats(font);
            sStorage->displayMonInfoLoadState++;
            break;
        case 6:
            PrintDisplayMonAbility(font);
            sStorage->displayMonInfoLoadState++;
            break;
        case 7:
            PrintDisplayMonHeldItem(font);
            sStorage->displayMonInfoLoadState++;
            break;
        case 8:
            UpdateMarkingComboSprite();
            sStorage->displayMonInfoLoadState++;
            break;
        case 9:
            sStorage->displayMonInfoLoadState = 0;
            return TRUE;
        }
    }
    else
    {
        UpdateGenderIconSprite(font);
        UpdateTypeIconsSprite();
        UpdateStatLabelsSprites();
        UpdateShinyIconSprite();
		UpdatePokerusIconSprite();
        UpdateMarkingComboSprite();
        sStorage->displayMonInfoLoadState = 0;
        return TRUE;
    }

    return FALSE;
}

static void UpdateBoxToSendMons(void)
{
    if (sLastUsedBox != StorageGetCurrentBox())
    {
        FlagClear(FLAG_SHOWN_BOX_WAS_FULL_MESSAGE);
        VarSet(VAR_PC_BOX_TO_SEND_MON, StorageGetCurrentBox());
    }
}

#define HGSS_MON_INFO_PANEL_BASE_TILE      512
#define HGSS_MON_INFO_PANEL_BLANK_TILE     710
#define HGSS_MON_INFO_PANEL_PAL_BANK       11
#define HGSS_MON_INFO_PANEL_WIDTH          11
#define HGSS_MON_INFO_PANEL_HEIGHT         18
#define HGSS_MON_INFO_LEFT                 0
#define HGSS_MON_INFO_RIGHT                17
#define HGSS_MON_INFO_LEFT_TOP             20
#define HGSS_MON_INFO_RIGHT_TOP            40

static const u32 sHgssMonInfoBlankTile[8] = {0};

static void DrawHgssMonInfoPanelTilemaps(void)
{
    u16 *tilemap = sStorage->infoTilemapBuffer;
    u8 x;
    u8 y;

    // The verified PNG is the exact 88x144 NSCR 9 composition rendered 1:1.
    // grit emits its 11x18 tiles row-major, so both left/right virtual screens
    // can reference one authentic tile set without redrawing any pixels.
    for (y = 0; y < HGSS_MON_INFO_PANEL_HEIGHT; y++)
    {
        for (x = 0; x < HGSS_MON_INFO_PANEL_WIDTH; x++)
        {
            u16 entry = (HGSS_MON_INFO_PANEL_BASE_TILE + y * HGSS_MON_INFO_PANEL_WIDTH + x)
                      | (HGSS_MON_INFO_PANEL_PAL_BANK << 12);

            tilemap[(HGSS_MON_INFO_LEFT_TOP + y) * 32 + HGSS_MON_INFO_LEFT + x] = entry;
            tilemap[(HGSS_MON_INFO_RIGHT_TOP + y) * 32 + HGSS_MON_INFO_RIGHT + x] = entry;
        }
    }
}

static void RestoreHgssMonInfoPanel(void)
{
    u16 i;
    const u16 blankEntry = HGSS_MON_INFO_PANEL_BLANK_TILE
                         | (HGSS_MON_INFO_PANEL_PAL_BANK << 12);

    // The legacy SWSH mon-info tiles and shared base tilemap are gone. Build BG0
    // from a known transparent tile so message/menu windows can layer onto a
    // deterministic blank surface without retaining any legacy visual data.
    LoadBgTiles(0, sHgssMonInfoPanel_Gfx, sizeof(sHgssMonInfoPanel_Gfx), HGSS_MON_INFO_PANEL_BASE_TILE);
    LoadBgTiles(0, sHgssMonInfoBlankTile, sizeof(sHgssMonInfoBlankTile), HGSS_MON_INFO_PANEL_BLANK_TILE);
    LoadPalette(sHgssMonInfoPanel_Pal, BG_PLTT_ID(HGSS_MON_INFO_PANEL_PAL_BANK), PLTT_SIZE_4BPP);

    for (i = 0; i < ARRAY_COUNT(sStorage->infoTilemapBuffer); i++)
        sStorage->infoTilemapBuffer[i] = blankEntry;

    DrawHgssMonInfoPanelTilemaps();
}

#define HGSS_MESSAGE_WINDOW_BASE_TILE      712
#define HGSS_MESSAGE_WINDOW_PAL_BANK       10
#define HGSS_MESSAGE_WINDOW_SRC_WIDTH      32
#define HGSS_MESSAGE_WINDOW_WIDTH          30
#define HGSS_MESSAGE_WINDOW_HEIGHT         6
#define HGSS_MESSAGE_WINDOW_TOP            14

static void DrawHgssMessageWindow(void)
{
    u16 *tilemap = sStorage->infoTilemapBuffer;
    u8 x;
    u8 y;

    LoadBgTiles(0,
                sHgssMessageWindow_Gfx,
                sizeof(sHgssMessageWindow_Gfx),
                HGSS_MESSAGE_WINDOW_BASE_TILE);
    LoadPalette(sHgssMessageWindow_Pal,
                BG_PLTT_ID(HGSS_MESSAGE_WINDOW_PAL_BANK),
                PLTT_SIZE_4BPP);

    for (y = 0; y < HGSS_MESSAGE_WINDOW_HEIGHT; y++)
    {
        for (x = 0; x < HGSS_MESSAGE_WINDOW_WIDTH; x++)
        {
            // Source columns 1-30 are identical interior repeats on each row.
            // Keep source column 0, use 28 untouched interior columns, and keep
            // source column 31 so both native HGSS rounded edges survive 1:1.
            u8 srcX = (x == HGSS_MESSAGE_WINDOW_WIDTH - 1)
                    ? HGSS_MESSAGE_WINDOW_SRC_WIDTH - 1
                    : x;
            u16 tile = HGSS_MESSAGE_WINDOW_BASE_TILE
                     + y * HGSS_MESSAGE_WINDOW_SRC_WIDTH
                     + srcX;

            tilemap[(HGSS_MESSAGE_WINDOW_TOP + y) * 32 + x]
                = tile | (HGSS_MESSAGE_WINDOW_PAL_BANK << 12);
        }
    }
}

#define HGSS_YES_NO_BASE_TILE              256
#define HGSS_YES_NO_PAL_BANK               10
#define HGSS_YES_NO_SRC_WIDTH              32
#define HGSS_YES_NO_WIDTH                  30
#define HGSS_YES_NO_HEIGHT                 7
#define HGSS_YES_NO_TOP                    13

static void DrawHgssYesNoPanel(void)
{
    u16 *tilemap = sStorage->infoTilemapBuffer;
    u8 x;
    u8 y;

    LoadBgTiles(0,
                sHgssYesNo_Gfx,
                sizeof(sHgssYesNo_Gfx),
                HGSS_YES_NO_BASE_TILE);
    LoadPalette(sHgssYesNo_Pal,
                BG_PLTT_ID(HGSS_YES_NO_PAL_BANK),
                PLTT_SIZE_4BPP);

    for (y = 0; y < HGSS_YES_NO_HEIGHT; y++)
    {
        for (x = 0; x < HGSS_YES_NO_WIDTH; x++)
        {
            // NSCR 12 has identical repeated interior columns 1-30.
            // Preserve both native rounded edges (0 and 31) and omit only two
            // repeated interior columns to fit the GBA's 30-tile display.
            u8 srcX = (x == HGSS_YES_NO_WIDTH - 1)
                    ? HGSS_YES_NO_SRC_WIDTH - 1
                    : x;
            u16 tile = HGSS_YES_NO_BASE_TILE
                     + y * HGSS_YES_NO_SRC_WIDTH
                     + srcX;

            tilemap[(HGSS_YES_NO_TOP + y) * 32 + x]
                = tile | (HGSS_YES_NO_PAL_BANK << 12);
        }
    }
}

#define HGSS_CONTEXT_MENU_BASE_TILE         904
#define HGSS_CONTEXT_MENU_PAL_BANK          10
#define HGSS_CONTEXT_MENU_SRC_WIDTH         12
#define HGSS_CONTEXT_MENU_SRC_HEIGHT        10
#define HGSS_CONTEXT_MENU_TOP_ROW_0         0
#define HGSS_CONTEXT_MENU_TOP_ROW_1         1
#define HGSS_CONTEXT_MENU_MIDDLE_ROW        2
#define HGSS_CONTEXT_MENU_BOTTOM_ROW_0      8
#define HGSS_CONTEXT_MENU_BOTTOM_ROW_1      9

static void DrawHgssContextMenuFrame(u8 windowId)
{
    u16 *tilemap = sStorage->infoTilemapBuffer;
    u16 textPal[16] = {0};
    u8 left = GetWindowAttribute(windowId, WINDOW_TILEMAP_LEFT);
    u8 top = GetWindowAttribute(windowId, WINDOW_TILEMAP_TOP);
    u8 width = GetWindowAttribute(windowId, WINDOW_WIDTH);
    u8 height = GetWindowAttribute(windowId, WINDOW_HEIGHT);
    u8 frameLeft = left - 1;
    u8 frameTop = top - 2;
    u8 frameWidth = width + 2;
    u8 frameHeight = height + 4;
    u8 x;
    u8 y;

    LoadBgTiles(0,
                sHgssContextMenu_Gfx,
                sizeof(sHgssContextMenu_Gfx),
                HGSS_CONTEXT_MENU_BASE_TILE);
    LoadPalette(sHgssContextMenu_Pal,
                BG_PLTT_ID(HGSS_CONTEXT_MENU_PAL_BANK),
                PLTT_SIZE_4BPP);

    // Menu text/cursor code assumes palette indices 1/2/3 are
    // background/foreground/shadow. Reorder only authentic NSCR 86 colors into
    // those slots; no new RGB value is introduced.
    textPal[0] = sHgssContextMenu_Pal[0];
    textPal[1] = sHgssContextMenu_Pal[0];
    textPal[2] = sHgssContextMenu_Pal[1];
    textPal[3] = sHgssContextMenu_Pal[2];
    textPal[4] = sHgssContextMenu_Pal[3];
    textPal[5] = sHgssContextMenu_Pal[4];
    textPal[6] = sHgssContextMenu_Pal[5];
    textPal[7] = sHgssContextMenu_Pal[6];
    LoadPalette(textPal, BG_PLTT_ID(15), PLTT_SIZE_4BPP);

    // NSCR 86 is screen-right open: column 0 is the native left edge and
    // columns 1-11 are repeated interior/top/bottom continuation tiles. Keep
    // that authentic geometry by right-aligning the dynamic menu and extending
    // the final interior tile to the screen edge. Vertically, preserve both
    // two-tile top and bottom bands while repeating only the verified middle row.
    for (y = 0; y < frameHeight; y++)
    {
        u8 srcY;

        if (y == 0)
            srcY = HGSS_CONTEXT_MENU_TOP_ROW_0;
        else if (y == 1)
            srcY = HGSS_CONTEXT_MENU_TOP_ROW_1;
        else if (y == frameHeight - 2)
            srcY = HGSS_CONTEXT_MENU_BOTTOM_ROW_0;
        else if (y == frameHeight - 1)
            srcY = HGSS_CONTEXT_MENU_BOTTOM_ROW_1;
        else
            srcY = HGSS_CONTEXT_MENU_MIDDLE_ROW;

        for (x = 0; x < frameWidth; x++)
        {
            u8 srcX = (x == 0) ? 0 : 1;
            u16 tile = HGSS_CONTEXT_MENU_BASE_TILE
                     + srcY * HGSS_CONTEXT_MENU_SRC_WIDTH
                     + srcX;

            tilemap[(frameTop + y) * 32 + frameLeft + x]
                = tile | (HGSS_CONTEXT_MENU_PAL_BANK << 12);
        }
    }
}

static void ClearHgssContextMenuFrame(u8 windowId)
{
    u16 *tilemap = sStorage->infoTilemapBuffer;
    const u16 blankEntry = HGSS_MON_INFO_PANEL_BLANK_TILE
                         | (HGSS_MON_INFO_PANEL_PAL_BANK << 12);
    u8 left = GetWindowAttribute(windowId, WINDOW_TILEMAP_LEFT);
    u8 top = GetWindowAttribute(windowId, WINDOW_TILEMAP_TOP);
    u8 width = GetWindowAttribute(windowId, WINDOW_WIDTH);
    u8 height = GetWindowAttribute(windowId, WINDOW_HEIGHT);
    u8 frameLeft = left - 1;
    u8 frameTop = top - 2;
    u8 frameWidth = width + 2;
    u8 frameHeight = height + 4;
    u8 x;
    u8 y;

    for (y = 0; y < frameHeight; y++)
    {
        for (x = 0; x < frameWidth; x++)
            tilemap[(frameTop + y) * 32 + frameLeft + x] = blankEntry;
    }
}

static void InitPokeStorageBg0(void)
{
    SetGpuReg(REG_OFFSET_BG0CNT, BGCNT_PRIORITY(0) | BGCNT_CHARBASE(0) | BGCNT_SCREENBASE(29) | BGCNT_TXT256x512);
    SetBgTilemapBuffer(0, sStorage->infoTilemapBuffer);
    // Storage BG0 chrome is now sourced from verified HGSS assets only.
    RestoreHgssMonInfoPanel();
    sStorage->bg0_Y = 0;
    UpdateMonInfoTilemap();
    CopyBgTilemapBufferToVram(0);
}

static u16 GetTargetBg0Y(void)
{
    if (sCursorMode == CURSOR_MODE_MULTI_MOVE)
        return 0;

    if (sStorage->showMonInfo && (GetSpeciesAtCursorPosition() != SPECIES_NONE || sIsMonBeingMoved))
    {
        if (sCursorArea == CURSOR_AREA_IN_BOX)
        {
            u8 column = sCursorPosition % IN_BOX_COLUMNS;
            if (column >= 2 && column <= 5)
                return DISPLAY_HEIGHT;
            else
                return DISPLAY_HEIGHT * 2;
        }
        else if (sCursorArea == CURSOR_AREA_IN_PARTY)
        {
            return DISPLAY_HEIGHT * 2;
        }
    }
    return 0;
}

static void UpdateMonInfoTilemap(void)
{
    if (sCursorMode == CURSOR_MODE_MULTI_MOVE)
    {
        sStorage->bg0_Y = 0;
        HideInfoPanelSprites();
        return;
    }

    if (sStorage->inMenuInteraction)
    {
        sStorage->bg0_Y = 0;
        HideInfoPanelSprites();
        return;
    }

    if (sStorage->showMonInfo && (GetSpeciesAtCursorPosition() != SPECIES_NONE || sIsMonBeingMoved))
    {
        if (sCursorArea == CURSOR_AREA_IN_BOX)
        {
            // Column-based positioning: columns 2-5 use left panel, columns 0-1 use right panel
            u8 column = sCursorPosition % IN_BOX_COLUMNS;
            if (column >= 2 && column <= 5)
            {
                sStorage->monInfoTilemapId = 0;
                sStorage->bg0_Y = DISPLAY_HEIGHT;
            }
            else
            {
                sStorage->monInfoTilemapId = 1;
                sStorage->bg0_Y = DISPLAY_HEIGHT * 2;
            }
        }
        else if (sCursorArea == CURSOR_AREA_IN_PARTY)
        {
            sStorage->monInfoTilemapId = 1;
            sStorage->bg0_Y = DISPLAY_HEIGHT * 2;
        }

        sStorage->displayMonInfoLoadState = 0;
        while (!PrintDisplayMonInfo());
        ScheduleBgCopyTilemapToVram(0);
    }
    else
    {
        sStorage->bg0_Y = 0;
        UpdateGenderIconSprite(FONT_SHORT_NARROW);
        UpdateTypeIconsSprite();
        UpdateStatLabelsSprites();
        UpdateShinyIconSprite();
		UpdatePokerusIconSprite();
        UpdateMarkingComboSprite();
    }
}

static void ClearMonInfoTilemap(void)
{
    if (sStorage->showMonInfo)
    {
        sStorage->bg0_Y = 0;
        HideInfoPanelSprites();
    }
}

static void ClearMonInfoPanel(void)
{
    u8 i;

    for (i = WIN_MON_INFO_NICKNAME_LEFT; i <= WIN_MON_INFO_ITEM_RIGHT; i++)
    {
        FillWindowPixelBuffer(i, PIXEL_FILL(0));
        CopyWindowToVram(i, COPYWIN_GFX);
    }

    HideInfoPanelSprites();
    sStorage->bg0_Y = 0;
    SetGpuReg(REG_OFFSET_BG0VOFS, 0);
}

static void PrintMessage(u8 id)
{
    u8 *txtPtr;

    ClearMonInfoTilemap();
    DynamicPlaceholderTextUtil_Reset();
    switch (sMessages[id].format)
    {
    case MSG_VAR_NONE:
        break;
    case MSG_VAR_MON_NAME_1:
    case MSG_VAR_MON_NAME_2:
    case MSG_VAR_MON_NAME_3:
        DynamicPlaceholderTextUtil_SetPlaceholderPtr(0, sStorage->displayMon.nickname);
        break;
    case MSG_VAR_RELEASE_MON_1:
    case MSG_VAR_RELEASE_MON_2:
    case MSG_VAR_RELEASE_MON_3:
        DynamicPlaceholderTextUtil_SetPlaceholderPtr(0, sStorage->releaseMonName);
        break;
    case MSG_VAR_ITEM_NAME:
        if (IsMovingItem())
            txtPtr = StringCopy(sStorage->itemName, GetMovingItemName());
        else
            txtPtr = StringCopy(sStorage->itemName, sStorage->displayMon.itemNameText);

        while (*(txtPtr - 1) == CHAR_SPACE)
            txtPtr--;

        *txtPtr = EOS;
        DynamicPlaceholderTextUtil_SetPlaceholderPtr(0, sStorage->itemName);
        break;
    }

    DynamicPlaceholderTextUtil_ExpandPlaceholders(sStorage->messageText, sMessages[id].text);
    DrawHgssMessageWindow();
    FillWindowPixelBuffer(WIN_MESSAGE, PIXEL_FILL(0));
    AddTextPrinterParameterized4(WIN_MESSAGE, FONT_NORMAL, 0, 1, 0, 0, sTextColors[3], TEXT_SKIP_DRAW, sStorage->messageText);
    PutWindowTilemap(WIN_MESSAGE);
    CopyWindowToVram(WIN_MESSAGE, COPYWIN_GFX);
    ScheduleBgCopyTilemapToVram(0);
}

static void ShowYesNoWindow(s8 cursorPos)
{
    ClearMonInfoTilemap();

    // Create only the functional text/cursor window. Do not call
    // CreateYesNoMenu: that helper always draws an Emerald standard frame.
    sHgssYesNoWindowId = AddWindow(&sYesNoWindowTemplate);
    FillWindowPixelBuffer(sHgssYesNoWindowId, PIXEL_FILL(1));
    AddTextPrinterParameterized(sHgssYesNoWindowId,
                                FONT_NORMAL,
                                gText_YesNo,
                                8,
                                1,
                                TEXT_SKIP_DRAW,
                                NULL);
    InitMenuInUpperLeftCornerNormal(sHgssYesNoWindowId, 2, cursorPos);

    DrawHgssYesNoPanel();

    // NSCR 12 uses palette index 1 for its cream interior. Re-render the
    // question text against the authentic panel palette.
    FillWindowPixelBuffer(WIN_MESSAGE, PIXEL_FILL(1));
    AddTextPrinterParameterized4(WIN_MESSAGE,
                                 FONT_NORMAL,
                                 0,
                                 1,
                                 0,
                                 0,
                                 sTextColors[0],
                                 TEXT_SKIP_DRAW,
                                 sStorage->messageText);
    PutWindowTilemap(WIN_MESSAGE);
    CopyWindowToVram(WIN_MESSAGE, COPYWIN_GFX);

    PutWindowTilemap(sHgssYesNoWindowId);
    CopyWindowToVram(sHgssYesNoWindowId, COPYWIN_GFX);
    ScheduleBgCopyTilemapToVram(0);
}

static s8 ProcessHgssYesNoInput(void)
{
    s8 result = Menu_ProcessInputNoWrap();

    if (result != MENU_NOTHING_CHOSEN && sHgssYesNoWindowId != WINDOW_NONE)
    {
        ClearWindowTilemap(sHgssYesNoWindowId);
        RemoveWindow(sHgssYesNoWindowId);
        sHgssYesNoWindowId = WINDOW_NONE;
        ScheduleBgCopyTilemapToVram(0);
    }

    return result;
}

static void ClearBottomWindow(void)
{
    ClearWindowTilemap(WIN_MESSAGE);
	// Reinitialize the authentic HGSS info-panel surface.
    RestoreHgssMonInfoPanel();
    UpdateMonInfoTilemap();
    ScheduleBgCopyTilemapToVram(0);
}


static void FreeHgssWallpaperSelectorSprites(void)
{
    u8 i;

    for (i = 0; i < ARRAY_COUNT(sStorage->wallpaperSelectorSprites); i++)
    {
        if (sStorage->wallpaperSelectorSprites[i] != NULL)
        {
            DestroySprite(sStorage->wallpaperSelectorSprites[i]);
            sStorage->wallpaperSelectorSprites[i] = NULL;
        }
    }

    FreeSpriteTilesByTag(GFXTAG_HGSS_WALLPAPER_SELECTOR);
    FreeSpritePaletteByTag(PALTAG_HGSS_WALLPAPER_SELECTOR);
}

static void LoadHgssWallpaperSelectorSprites(void)
{
    const u8 *srcTiles = (const u8 *)sHgssWallpaperSelectorSwatch_Gfx;
    struct SpriteSheet spriteSheet =
    {
        .data = sStorage->wallpaperSelectorTiles,
        .size = sizeof(sStorage->wallpaperSelectorTiles),
        .tag = GFXTAG_HGSS_WALLPAPER_SELECTOR,
    };
    u16 palette[16] = {0};
    struct SpritePalette spritePalette =
    {
        .data = palette,
        .tag = PALTAG_HGSS_WALLPAPER_SELECTOR,
    };
    const u8 page = sStorage->listMenuScrollRow;
    const s16 x = sStorage->menuWindow.tilemapLeft * 8 + 20;
    const s16 top = sStorage->menuWindow.tilemapTop * 8;
    u8 item;
    u8 row;
    u8 col;
    u8 byte;

    // The verified HGSS cell is 24x24. GBA OBJ has no 24x24 shape, so each
    // authentic cell is copied 1:1 into the upper-left 24x24 of a transparent
    // 32x32 hardware cell. The four copies share one 4bpp palette; only the
    // nonzero mask index is remapped per copy so each swatch can use its
    // authentic HGSS member-75 color.
    CpuFill32(0, sStorage->wallpaperSelectorTiles, sizeof(sStorage->wallpaperSelectorTiles));

    for (item = 0; item < HGSS_WALLPAPER_SELECTOR_ITEMS_PER_PAGE; item++)
    {
        const u8 colorIndex = item + 1;
        const u8 wallpaperId = page * HGSS_WALLPAPER_SELECTOR_ITEMS_PER_PAGE + item;

        palette[colorIndex] = sHgssWallpaperSelectorColors[wallpaperId];

        for (row = 0; row < 3; row++)
        {
            for (col = 0; col < 3; col++)
            {
                const u32 srcTile = row * 3 + col;
                const u32 dstTile = item * 16 + row * 4 + col;
                const u8 *src = srcTiles + srcTile * TILE_SIZE_4BPP;
                u8 *dst = sStorage->wallpaperSelectorTiles + dstTile * TILE_SIZE_4BPP;

                for (byte = 0; byte < TILE_SIZE_4BPP; byte++)
                {
                    const u8 value = src[byte];
                    const u8 lo = (value & 0x0F) ? colorIndex : 0;
                    const u8 hi = (value & 0xF0) ? colorIndex : 0;
                    dst[byte] = lo | (hi << 4);
                }
            }
        }
    }

    FreeSpriteTilesByTag(GFXTAG_HGSS_WALLPAPER_SELECTOR);
    FreeSpritePaletteByTag(PALTAG_HGSS_WALLPAPER_SELECTOR);
    LoadSpriteSheet(&spriteSheet);
    LoadSpritePalette(&spritePalette);

    for (item = 0; item < HGSS_WALLPAPER_SELECTOR_ITEMS_PER_PAGE; item++)
    {
        const u8 spriteId = CreateSprite(
            &sSpriteTemplate_HgssWallpaperSelector,
            x,
            top + 12 + item * 16,
            0);

        if (spriteId != MAX_SPRITES)
        {
            sStorage->wallpaperSelectorSprites[item] = &gSprites[spriteId];
            StartSpriteAnim(sStorage->wallpaperSelectorSprites[item], item);
        }
    }
}

static void AddWallpaperMenu(void)
{
    u16 i;
    u8 currentPage = sStorage->listMenuScrollRow;
    u8 itemsPerPage = HGSS_WALLPAPER_SELECTOR_ITEMS_PER_PAGE;
    u8 startIdx = MENU_BASE + (currentPage * itemsPerPage);
    u8 endIdx = startIdx + itemsPerPage;
    u8 maxWidth = 0;
    u8 len;

    if (endIdx > MENU_COUNT)
        endIdx = MENU_COUNT;

    for (i = MENU_BASE; i < MENU_COUNT; i++)
    {
        len = GetStringWidth(FONT_NORMAL, sMenuTexts[i], 0);
        if (len > maxWidth)
            maxWidth = len;
    }

    InitMenu();
    for (i = startIdx; i < endIdx; i++)
        SetMenuText(i);

    sStorage->menuItems[sStorage->menuItemsCount].text = NULL;
    sStorage->menuItems[sStorage->menuItemsCount].textId = 0;

    // Reserve the first 32 px for the authentic HGSS selector swatch.
    sStorage->menuWindow.width = (32 + maxWidth + 8 + 7) / 8;
    if (sStorage->menuWindow.width > 28)
        sStorage->menuWindow.width = 28;

    sStorage->menuWindow.height = 8;
    sStorage->menuWindow.tilemapLeft = 29 - sStorage->menuWindow.width;
    sStorage->menuWindow.tilemapTop = 5;
    sStorage->menuWindowId = AddWindow(&sStorage->menuWindow);
    ClearMonInfoTilemap();
    DrawHgssContextMenuFrame(sStorage->menuWindowId);
    PutWindowTilemap(sStorage->menuWindowId);

    sStorage->listMenuTemplate.items = (struct ListMenuItem *)sStorage->menuItems;
    sStorage->listMenuTemplate.moveCursorFunc = ListMenuDefaultCursorMoveFunc;
    sStorage->listMenuTemplate.itemPrintFunc = NULL;
    sStorage->listMenuTemplate.totalItems = sStorage->menuItemsCount;
    sStorage->listMenuTemplate.maxShowed = sStorage->menuItemsCount;
    sStorage->listMenuTemplate.windowId = sStorage->menuWindowId;
    sStorage->listMenuTemplate.header_X = 0;
    sStorage->listMenuTemplate.item_X = 32;
    sStorage->listMenuTemplate.cursor_X = 24;
    sStorage->listMenuTemplate.upText_Y = 1;
    sStorage->listMenuTemplate.cursorPal = 2;
    sStorage->listMenuTemplate.fillValue = 1;
    sStorage->listMenuTemplate.cursorShadowPal = 3;
    sStorage->listMenuTemplate.lettersSpacing = 1;
    sStorage->listMenuTemplate.itemVerticalPadding = 0;
    sStorage->listMenuTemplate.scrollMultiple = LIST_NO_MULTIPLE_SCROLL;
    sStorage->listMenuTemplate.fontId = FONT_NORMAL;
    sStorage->listMenuTemplate.cursorKind = CURSOR_INVISIBLE;

    sStorage->listMenuTaskId = ListMenuInit(&sStorage->listMenuTemplate, 0, sStorage->listMenuSelectedRow);

    LoadHgssWallpaperSelectorSprites();
    ScheduleBgCopyTilemapToVram(0);
}

static u8 GetCurrentBoxOption(void)
{
    return sCurrentBoxOption;
}

static void InitCursorItemIcon(void)
{
    if (!IsCursorOnBoxTitle())
    {
        if (sCursorArea == CURSOR_AREA_IN_PARTY)
            TryLoadItemIconAtPos(CURSOR_AREA_IN_PARTY, GetCursorPosition());
        else
            TryLoadItemIconAtPos(CURSOR_AREA_IN_BOX, GetCursorPosition());
    }

    if (sMovingItemId != ITEM_NONE)
    {
        InitItemIconInCursor(sMovingItemId);
        StartCursorAnim(CURSOR_ANIM_MAIN);
    }
}


//------------------------------------------------------------------------------
//  SECTION: Pokémon sprites
//
//  The below functions generally handle the Pokémon icon sprites, including
//  moving them with a scrolling box, shifting the party sprites, and
//  animating released Pokémon.
//------------------------------------------------------------------------------


static void InitMonIconFields(void)
{
    u16 i;

    LoadMonIconPalettes();
    for (i = 0; i < MAX_MON_ICONS; i++)
        sStorage->numIconsPerSpecies[i] = 0;
    for (i = 0; i < MAX_MON_ICONS; i++)
        sStorage->iconSpeciesList[i] = SPECIES_NONE;
    for (i = 0; i < PARTY_SIZE; i++)
        sStorage->partySprites[i] = NULL;
    for (i = 0; i < IN_BOX_COUNT; i++)
        sStorage->boxMonsSprites[i] = NULL;

    sStorage->movingMonSprite = NULL;
}

static void CreateMovingMonIcon(void)
{
    u32 personality = GetMonData(&sStorage->movingMon, MON_DATA_PERSONALITY);
    enum Species species = GetMonData(&sStorage->movingMon, MON_DATA_SPECIES);
    u8 priority = 1;
    bool32 isEgg = GetMonData(&sStorage->movingMon, MON_DATA_IS_EGG);

    sStorage->movingMonSprite = CreateMonIconSprite(species, personality, 0, 0, priority, 3, isEgg);
    sStorage->movingMonSprite->callback = SpriteCB_HeldMon;
}

static bool32 ShouldBoxmonSpriteBeTransparent(u32 boxId, u32 boxPosition)
{
    if (sStorage->boxOption == OPTION_MOVE_ITEMS
        && GetBoxMonDataAt(boxId, boxPosition, MON_DATA_HELD_ITEM) == ITEM_NONE)
        return TRUE;
    if (sStorage->boxOption == OPTION_SELECT_MON
        && IsBoxMonExcluded(GetBoxedMonPtr(boxId, boxPosition)))
        return TRUE;
    return FALSE;
}

static void InitBoxMonSprites(u8 boxId)
{
    u8 boxPosition;
    u16 i, j, count;
    enum Species species;
    u32 personality;

    count = 0;
    boxPosition = 0;

    // For each box slot, create a Pokémon icon if a species is present
    for (i = 0; i < IN_BOX_ROWS; i++)
    {
        for (j = 0; j < IN_BOX_COLUMNS; j++)
        {
            species = GetBoxMonDataAt(boxId, boxPosition, MON_DATA_SPECIES);
            bool32 isEgg = GetBoxMonDataAt(boxId, boxPosition, MON_DATA_IS_EGG);
            if (species != SPECIES_NONE)
            {
                personality = GetBoxMonDataAt(boxId, boxPosition, MON_DATA_PERSONALITY);
                sStorage->boxMonsSprites[count] = CreateMonIconSprite(species, personality, 8 * (3 * j) + 92, 8 * (3 * i) + 40, 2, 19 - j, isEgg);

                if (ShouldBoxmonSpriteBeTransparent(boxId, boxPosition))
                    sStorage->boxMonsSprites[count]->oam.objMode = ST_OAM_OBJ_BLEND;
            }
            else
            {
                sStorage->boxMonsSprites[count] = NULL;
            }
            boxPosition++;
            count++;
        }
    }
}

static void CreateBoxMonIconAtPos(u8 boxPosition)
{
    enum Species species = GetCurrentBoxMonData(boxPosition, MON_DATA_SPECIES);
    bool32 isEgg = GetCurrentBoxMonData(boxPosition, MON_DATA_IS_EGG);

    if (species != SPECIES_NONE)
    {
        s16 x = 8 * (3 * (boxPosition % IN_BOX_COLUMNS)) + 92;
        s16 y = 8 * (3 * (boxPosition / IN_BOX_COLUMNS)) + 40;
        u32 personality = GetCurrentBoxMonData(boxPosition, MON_DATA_PERSONALITY);

        sStorage->boxMonsSprites[boxPosition] = CreateMonIconSprite(species, personality, x, y, 2, 19 - (boxPosition % IN_BOX_COLUMNS), isEgg);
        if (ShouldBoxmonSpriteBeTransparent(StorageGetCurrentBox(), boxPosition))
            sStorage->boxMonsSprites[boxPosition]->oam.objMode = ST_OAM_OBJ_BLEND;
    }
}

#define sDistance      data[1]
#define sSpeed         data[2]
#define sScrollInDestX data[3]
#define sDelay         data[4]
#define sScrollOutX    data[5]

static void StartBoxMonIconsScrollOut(s16 speed)
{
    u16 i;

    for (i = 0; i < IN_BOX_COUNT; i++)
    {
        if (sStorage->boxMonsSprites[i] != NULL)
        {
            sStorage->boxMonsSprites[i]->sSpeed = speed;
            sStorage->boxMonsSprites[i]->sDelay = 1;
            sStorage->boxMonsSprites[i]->callback = SpriteCB_BoxMonIconScrollOut;
        }
    }
}

static void SpriteCB_BoxMonIconScrollIn(struct Sprite *sprite)
{
    if (sprite->sDistance != 0)
    {
        // Icon moving
        sprite->sDistance--;
        sprite->x += sprite->sSpeed;
        if (sprite->x <= 64 || sprite->x >= 252)
            sprite->invisible = TRUE;
        else
            sprite->invisible = FALSE;
    }
    else
    {
        // Icon arrived
        sStorage->iconScrollNumIncoming--;
        sprite->x = sprite->sScrollInDestX;
        sprite->invisible = FALSE;
        sprite->callback = SpriteCallbackDummy;
    }
}

static void SpriteCB_BoxMonIconScrollOut(struct Sprite *sprite)
{
    if (sprite->sDelay != 0)
    {
        sprite->sDelay--;
    }
    else
    {
        // Icon moving
        sprite->x += sprite->sSpeed;
        sprite->sScrollOutX = sprite->x + sprite->x2;

        // Check if icon offscreen
        if (sprite->sScrollOutX <= 64 || sprite->sScrollOutX >= 252)
            sprite->callback = SpriteCallbackDummy;
    }
}

// Sprites for Pokémon icons are destroyed during
// the box scroll once they've gone offscreen
static void DestroyBoxMonIconsInColumn(u8 column)
{
    u16 row;
    u8 boxPosition = column;

    for (row = 0; row < IN_BOX_ROWS; row++)
    {
        if (sStorage->boxMonsSprites[boxPosition] != NULL)
        {
            DestroyBoxMonIcon(sStorage->boxMonsSprites[boxPosition]);
            sStorage->boxMonsSprites[boxPosition] = NULL;
        }
        boxPosition += IN_BOX_COLUMNS;
    }
}

// Create the appearing icons for the incoming scrolling box
static u8 CreateBoxMonIconsInColumn(u8 column, u16 distance, s16 speed)
{
    s32 i;
    u16 y = 40;
    s16 xDest = 8 * (3 * column) + 92;
    u16 x = xDest - ((distance + 1) * speed);
    u8 subpriority = 19 - column;
    u8 iconsCreated = 0;
    u8 boxPosition = column;

    for (i = 0; i < IN_BOX_ROWS; i++)
    {
        if (sStorage->boxSpecies[boxPosition] != SPECIES_NONE)
        {
            sStorage->boxMonsSprites[boxPosition] = CreateMonIconSprite(sStorage->boxSpecies[boxPosition],
                                                                        sStorage->boxPersonalities[boxPosition],
                                                                        x, y, 2, subpriority,
                                                                        sStorage->boxIsEgg[boxPosition]);
            if (sStorage->boxMonsSprites[boxPosition] != NULL)
            {
                sStorage->boxMonsSprites[boxPosition]->sDistance = distance;
                sStorage->boxMonsSprites[boxPosition]->sSpeed = speed;
                sStorage->boxMonsSprites[boxPosition]->sScrollInDestX = xDest;
                sStorage->boxMonsSprites[boxPosition]->callback = SpriteCB_BoxMonIconScrollIn;
                if (ShouldBoxmonSpriteBeTransparent(sStorage->incomingBoxId, boxPosition))
                    sStorage->boxMonsSprites[boxPosition]->oam.objMode = ST_OAM_OBJ_BLEND;
                iconsCreated++;
            }
        }
        boxPosition += IN_BOX_COLUMNS;
        y += 24;
    }

    return iconsCreated;
}

#undef sDistance
#undef sSpeed
#undef sScrollInDestX
#undef sDelay
#undef sScrollOutX

static void InitBoxMonIconScroll(u8 boxId, s8 direction)
{
    sStorage->iconScrollState = 0;
    sStorage->iconScrollDirection = direction;
    sStorage->iconScrollDistance = 32;
    sStorage->iconScrollSpeed = -(6 * direction);
    sStorage->iconScrollNumIncoming = 0;
    GetIncomingBoxMonData(boxId);
    if (direction > 0)
        sStorage->iconScrollCurColumn = 0;
    else
        sStorage->iconScrollCurColumn = IN_BOX_COLUMNS - 1;

    sStorage->iconScrollPos = (24 * sStorage->iconScrollCurColumn) + 92;
    StartBoxMonIconsScrollOut(sStorage->iconScrollSpeed);
}

static bool8 UpdateBoxMonIconScroll(void)
{
    if (sStorage->iconScrollDistance != 0)
        sStorage->iconScrollDistance--;

    switch (sStorage->iconScrollState)
    {
    case 0:
        sStorage->iconScrollPos += sStorage->iconScrollSpeed;
        if (sStorage->iconScrollPos <= 60 || sStorage->iconScrollPos >= 252)
        {
            // A column of icons has gone offscreen, destroy them
            DestroyBoxMonIconsInColumn(sStorage->iconScrollCurColumn);
            sStorage->iconScrollPos += sStorage->iconScrollDirection * 24;
            sStorage->iconScrollState++;
        }
        break;
    case 1:
        // Create the new incoming column of icons
        sStorage->iconScrollPos += sStorage->iconScrollSpeed;
        sStorage->iconScrollNumIncoming += CreateBoxMonIconsInColumn(sStorage->iconScrollCurColumn, sStorage->iconScrollDistance, sStorage->iconScrollSpeed);

        if ((sStorage->iconScrollDirection > 0 && sStorage->iconScrollCurColumn == IN_BOX_COLUMNS - 1)
            || (sStorage->iconScrollDirection < 0 && sStorage->iconScrollCurColumn == 0))
        {
            // Scroll has reached final column
            sStorage->iconScrollState++;
        }
        else
        {
            // Continue scrolling
            sStorage->iconScrollCurColumn += sStorage->iconScrollDirection;
            sStorage->iconScrollState = 0;
        }
        break;
    case 2:
        // Wait to make sure all icons have arrived
        if (sStorage->iconScrollNumIncoming == 0)
        {
            sStorage->iconScrollDistance++;
            return FALSE;
        }
        break;
    default:
        return FALSE;
    }

    return TRUE;
}

static void GetIncomingBoxMonData(u8 boxId)
{
    s32 i, j, boxPosition;

    boxPosition = 0;
    for (i = 0; i < IN_BOX_ROWS; i++)
    {
        for (j = 0; j < IN_BOX_COLUMNS; j++)
        {
            sStorage->boxSpecies[boxPosition] = GetBoxMonDataAt(boxId, boxPosition, MON_DATA_SPECIES);
            sStorage->boxIsEgg[boxPosition] = GetBoxMonDataAt(boxId, boxPosition, MON_DATA_IS_EGG);
            if (sStorage->boxSpecies[boxPosition] != SPECIES_NONE)
                sStorage->boxPersonalities[boxPosition] = GetBoxMonDataAt(boxId, boxPosition, MON_DATA_PERSONALITY);
            boxPosition++;
        }
    }

    sStorage->incomingBoxId = boxId;
}

static void DestroyBoxMonIconAtPosition(u8 boxPosition)
{
    if (sStorage->boxMonsSprites[boxPosition] != NULL)
    {
        DestroyBoxMonIcon(sStorage->boxMonsSprites[boxPosition]);
        sStorage->boxMonsSprites[boxPosition] = NULL;
    }
}

static void SetBoxMonIconObjMode(u8 boxPosition, u8 objMode)
{
    if (sStorage->boxMonsSprites[boxPosition] != NULL)
        sStorage->boxMonsSprites[boxPosition]->oam.objMode = objMode;
}

static void CreatePartyMonsSprites(bool8 visible)
{
    u16 i, count;
    enum Species species = GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_SPECIES);
    bool32 isEgg = GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_IS_EGG);
    u32 personality = GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_PERSONALITY);

    if (sStorage->partySprites[0] != NULL)
    {
        DestroyBoxMonIcon(sStorage->partySprites[0]);
        sStorage->partySprites[0] = NULL;
    }
    sStorage->partySprites[0] = CreateMonIconSprite(species, personality, 38, 16, 1, 12, isEgg);
    count = 1;
    for (i = 1; i < PARTY_SIZE; i++)
    {
        if (sStorage->partySprites[i] != NULL)
        {
            DestroyBoxMonIcon(sStorage->partySprites[i]);
            sStorage->partySprites[i] = NULL;
        }

        species = GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_SPECIES);
        isEgg = GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_IS_EGG);
        if (species != SPECIES_NONE)
        {
            personality = GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_PERSONALITY);
            sStorage->partySprites[i] = CreateMonIconSprite(species, personality, 38,  24 * i + 16, 1, 12, isEgg);
            count++;
        }
    }

    if (sStorage->boxOption == OPTION_MOVE_ITEMS)
    {
        for (i = 0; i < PARTY_SIZE; i++)
        {
            if (sStorage->partySprites[i] != NULL && GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_HELD_ITEM) == ITEM_NONE)
                sStorage->partySprites[i]->oam.objMode = ST_OAM_OBJ_BLEND;
        }
    }

    if (sStorage->boxOption == OPTION_SELECT_MON)
    {
        for (i = 0; i < PARTY_SIZE; i++)
        {
            if (sStorage->partySprites[i] != NULL && IsBoxMonExcluded(&(gParties[B_TRAINER_PLAYER][i].box)))
                sStorage->partySprites[i]->oam.objMode = ST_OAM_OBJ_BLEND;
        }
    }
}

static void CompactPartySprites(void)
{
    u16 i, targetSlot;

    sStorage->numPartyToCompact = 0;
    for (i = 0, targetSlot = 0; i < PARTY_SIZE; i++)
    {
        if (sStorage->partySprites[i] != NULL)
        {
            if (i != targetSlot)
            {
                MovePartySpriteToNextSlot(sStorage->partySprites[i], targetSlot);
                sStorage->partySprites[i] = NULL;
                sStorage->numPartyToCompact++;
            }
            targetSlot++;
        }
    }
}

static u8 GetNumPartySpritesCompacting(void)
{
    return sStorage->numPartyToCompact;
}

#define sPartyId   data[1]
#define sMonX      data[2]
#define sMonY      data[3]
#define sSpeedX    data[4]
#define sSpeedY    data[5]
#define sMoveSteps data[6]

static void MovePartySpriteToNextSlot(struct Sprite *sprite, u16 partyId)
{
    s16 x, y;

    sprite->sPartyId = partyId;
    x = 40;
    y = 24 * partyId + 16;

    sprite->sMonX = (u16)(sprite->x) * 8;
    sprite->sMonY = (u16)(sprite->y) * 8;
    sprite->sSpeedX = ((x * 8) - sprite->sMonX) / 8;
    sprite->sSpeedY = ((y * 8) - sprite->sMonY) / 8;
    sprite->sMoveSteps = 8;
    sprite->callback = SpriteCB_MovePartyMonToNextSlot;
}

static void SpriteCB_MovePartyMonToNextSlot(struct Sprite *sprite)
{
    if (sprite->sMoveSteps != 0)
    {
        s16 x = sprite->sMonX += sprite->sSpeedX;
        s16 y = sprite->sMonY += sprite->sSpeedY;
        sprite->x = x / 8u;
        sprite->y = y / 8u;
        sprite->sMoveSteps--;
    }
    else
    {
        sprite->x = 40;
        sprite->y = 24 * sprite->sPartyId + 16;
        sprite->callback = SpriteCallbackDummy;
        sStorage->partySprites[sprite->sPartyId] = sprite;
        sStorage->numPartyToCompact--;
    }
}

#undef sPartyId
#undef sMonX
#undef sMonY
#undef sSpeedX
#undef sSpeedY
#undef sMoveSteps

static void SetPartyMonIconObjMode(u8 partyId, u8 objMode)
{
    if (sStorage->partySprites[partyId] != NULL)
    {
        sStorage->partySprites[partyId]->oam.objMode = objMode;
    }
}

static void SetMovingMonSprite(u8 mode, u8 id)
{
    if (mode == MODE_PARTY)
    {
        sStorage->movingMonSprite = sStorage->partySprites[id];
        sStorage->partySprites[id] = NULL;
    }
    else if (mode == MODE_BOX)
    {
        sStorage->movingMonSprite = sStorage->boxMonsSprites[id];
        sStorage->boxMonsSprites[id] = NULL;
    }
    else
    {
        return;
    }

    sStorage->movingMonSprite->callback = SpriteCB_HeldMon;
    sStorage->movingMonSprite->oam.priority = 1;
    sStorage->movingMonSprite->subpriority = 3;
}

static void SetPlacedMonSprite(u8 boxId, u8 position)
{
    if (boxId == TOTAL_BOXES_COUNT) // party mon
    {
        sStorage->partySprites[position] = sStorage->movingMonSprite;
        sStorage->partySprites[position]->oam.priority = 1;
        sStorage->partySprites[position]->subpriority = 12;
        sStorage->partySprites[position]->x = 40;
        sStorage->partySprites[position]->y = 24 * position + 16;
    }
    else
    {
        sStorage->boxMonsSprites[position] = sStorage->movingMonSprite;
        sStorage->boxMonsSprites[position]->oam.priority = 1;
        sStorage->boxMonsSprites[position]->subpriority = 19 - (position % IN_BOX_COLUMNS);
    }
    sStorage->movingMonSprite->callback = SpriteCallbackDummy;
    sStorage->movingMonSprite = NULL;
}

static void SaveMonSpriteAtPos(u8 boxId, u8 position)
{
    if (boxId == TOTAL_BOXES_COUNT) // party mon
        sStorage->shiftMonSpritePtr = &sStorage->partySprites[position];
    else
        sStorage->shiftMonSpritePtr = &sStorage->boxMonsSprites[position];

    sStorage->movingMonSprite->callback = SpriteCallbackDummy;
    sStorage->shiftTimer = 0;
}

static bool8 MoveShiftingMons(void)
{
    if (sStorage->shiftTimer == 16)
        return FALSE;

    sStorage->shiftTimer++;
    if ((sStorage->shiftTimer & 3) == 0)
    {
        (*sStorage->shiftMonSpritePtr)->y--;
        sStorage->movingMonSprite->y++;
    }

    (*sStorage->shiftMonSpritePtr)->x2 = gSineTable[sStorage->shiftTimer * 8] / 16;
    sStorage->movingMonSprite->x2 = -(gSineTable[sStorage->shiftTimer * 8] / 16);
    if (sStorage->shiftTimer == 8)
    {
        sStorage->movingMonSprite->oam.priority = (*sStorage->shiftMonSpritePtr)->oam.priority;
        sStorage->movingMonSprite->subpriority = (*sStorage->shiftMonSpritePtr)->subpriority;
        (*sStorage->shiftMonSpritePtr)->oam.priority = 1;
        (*sStorage->shiftMonSpritePtr)->subpriority = 3;
    }

    if (sStorage->shiftTimer == 16)
    {
        struct Sprite *sprite = sStorage->movingMonSprite;
        sStorage->movingMonSprite = (*sStorage->shiftMonSpritePtr);
        *sStorage->shiftMonSpritePtr = sprite;

        sStorage->movingMonSprite->callback = SpriteCB_HeldMon;
        (*sStorage->shiftMonSpritePtr)->callback = SpriteCallbackDummy;
    }

    return TRUE;
}

static void SetReleaseMon(u8 mode, u8 position)
{
    switch (mode)
    {
    case MODE_PARTY:
        sStorage->releaseMonSpritePtr = &sStorage->partySprites[position];
        break;
    case MODE_BOX:
        sStorage->releaseMonSpritePtr = &sStorage->boxMonsSprites[position];
        break;
    case MODE_MOVE:
        sStorage->releaseMonSpritePtr = &sStorage->movingMonSprite;
        break;
    default:
        return;
    }

    if (*sStorage->releaseMonSpritePtr != NULL)
    {
        InitSpriteAffineAnim(*sStorage->releaseMonSpritePtr);
        (*sStorage->releaseMonSpritePtr)->oam.affineMode = ST_OAM_AFFINE_NORMAL;
        (*sStorage->releaseMonSpritePtr)->affineAnims = sAffineAnims_ReleaseMon;
        StartSpriteAffineAnim(*sStorage->releaseMonSpritePtr, RELEASE_ANIM_RELEASE);
    }
}

static bool8 TryHideReleaseMonSprite(void)
{
    if (*sStorage->releaseMonSpritePtr == NULL
        || (*sStorage->releaseMonSpritePtr)->invisible)
        return FALSE;

    if ((*sStorage->releaseMonSpritePtr)->affineAnimEnded)
        (*sStorage->releaseMonSpritePtr)->invisible = TRUE;

    return TRUE;
}

static void DestroyReleaseMonIcon(void)
{
    if (*sStorage->releaseMonSpritePtr != NULL)
    {
        FreeOamMatrix((*sStorage->releaseMonSpritePtr)->oam.matrixNum);
        DestroyBoxMonIcon(*sStorage->releaseMonSpritePtr);
        *sStorage->releaseMonSpritePtr = NULL;
    }
}

static void ReshowReleaseMon(void)
{
    if (*sStorage->releaseMonSpritePtr != NULL)
    {
        (*sStorage->releaseMonSpritePtr)->invisible = FALSE;
        StartSpriteAffineAnim(*sStorage->releaseMonSpritePtr, RELEASE_ANIM_CAME_BACK);
    }
}

static bool8 ResetReleaseMonSpritePtr(void)
{
    if (sStorage->releaseMonSpritePtr == NULL)
        return FALSE;

    if ((*sStorage->releaseMonSpritePtr)->affineAnimEnded)
        sStorage->releaseMonSpritePtr = NULL;

    return TRUE;
}

static void SetMovingMonPriority(u8 priority)
{
    sStorage->movingMonSprite->oam.priority = priority;
}

static void SpriteCB_HeldMon(struct Sprite *sprite)
{
    sprite->x = sStorage->cursorSprite->x;
    sprite->y = sStorage->cursorSprite->y + (sStorage->cursorSprite->y2 / 2) + 4;
}

static void SpriteCB_HeldMonInChooseBox(struct Sprite *sprite)
{
    sprite->x = sStorage->cursorSprite->x;
    sprite->y = sStorage->cursorSprite->y + 4;
}

static u16 TryLoadMonIconTiles(enum Species species, u32 personality, bool32 isEgg)
{
    u16 i, offset;

#if P_GENDER_DIFFERENCES
    // Treat female mons as a seperate species as they may have a different icon than males
    if (gSpeciesInfo[species].iconSpriteFemale != NULL && IsPersonalityFemale(species, personality))
        species |= (1 << 15);
#endif

    // Treat eggs as a seperate species as they might have unique sprites
    if (isEgg)
        species |= (1 << 14);

    // Search icon list for this species
    for (i = 0; i < MAX_MON_ICONS; i++)
    {
        if (sStorage->iconSpeciesList[i] == species)
            break;
    }

    if (i == MAX_MON_ICONS)
    {
        // Species not present in the list
        // Find first empty spot in the list to put it
        for (i = 0; i < MAX_MON_ICONS; i++)
        {
            if (sStorage->iconSpeciesList[i] == 0)
                break;
        }

        // Failed to find an empty spot
        if (i == MAX_MON_ICONS)
            return 0xFFFF;
    }

    // Add species to icon list and load tiles
    sStorage->iconSpeciesList[i] = species;
    sStorage->numIconsPerSpecies[i]++;
    offset = 16 * i;
    species &= SPECIES_MASK;
    CpuCopy32(GetMonIconTilesIsEgg(species, personality, isEgg), (void *)(OBJ_VRAM0) + offset * TILE_SIZE_4BPP, 0x200);

    return offset;
}

static void RemoveSpeciesFromIconList(enum Species species)
{
    u16 i;
    bool8 hasFemale = FALSE;

    for (i = 0; i < MAX_MON_ICONS; i++)
    {
        if (sStorage->iconSpeciesList[i] == (species | 0x8000))
        {
            hasFemale = TRUE;
            break;
        }
    }

    for (i = 0; i < MAX_MON_ICONS; i++)
    {
        if (sStorage->iconSpeciesList[i] == species && !hasFemale)
        {
            if (--sStorage->numIconsPerSpecies[i] == 0)
                sStorage->iconSpeciesList[i] = SPECIES_NONE;
            break;
        }
    }
}

static struct Sprite *CreateMonIconSprite(enum Species species, u32 personality, s16 x, s16 y, u8 oamPriority, u8 subpriority, bool32 isEgg)
{
    u16 tileNum;
    u8 spriteId;
    struct SpriteTemplate template = sSpriteTemplate_MonIcon;

    species = GetIconSpecies(species, personality);
    if (isEgg)
    {
        if (gSpeciesInfo[species].eggId != EGG_ID_NONE)
            template.paletteTag = PALTAG_MON_ICON_0 + gEggDatas[gSpeciesInfo[species].eggId].eggIconPalIndex;
        else
            template.paletteTag = PALTAG_MON_ICON_0 + gSpeciesInfo[SPECIES_EGG].iconPalIndex;
    }
#if P_GENDER_DIFFERENCES
    else if (gSpeciesInfo[species].iconSpriteFemale != NULL && IsPersonalityFemale(species, personality))
    {
        template.paletteTag = PALTAG_MON_ICON_0 + gSpeciesInfo[species].iconPalIndexFemale;
    }
#endif
    else
    {
        template.paletteTag = PALTAG_MON_ICON_0 + gSpeciesInfo[species].iconPalIndex;
    }

    tileNum = TryLoadMonIconTiles(species, personality, isEgg);
    if (tileNum == 0xFFFF)
        return NULL;

    spriteId = CreateSprite(&template, x, y, subpriority);
    if (spriteId == MAX_SPRITES)
    {
        RemoveSpeciesFromIconList(species);
        return NULL;
    }

    gSprites[spriteId].oam.tileNum = tileNum;
    gSprites[spriteId].oam.priority = oamPriority;
    gSprites[spriteId].data[0] = species;
    return &gSprites[spriteId];
}

static void DestroyBoxMonIcon(struct Sprite *sprite)
{
    RemoveSpeciesFromIconList(sprite->data[0]);
    DestroySprite(sprite);
}


//------------------------------------------------------------------------------
//  SECTION: General box
//
//  Some basic box functions, including initializing the box and scrolling.
//------------------------------------------------------------------------------


#define tState  data[0]
#define tDmaIdx data[1]
#define tBoxId  data[2]

static void CreateInitBoxTask(u8 boxId)
{
    u8 taskId = CreateTask(Task_InitBox, 2);

    gTasks[taskId].tBoxId = boxId;
}

static bool8 IsInitBoxActive(void)
{
    return FuncIsActiveTask(Task_InitBox);
}

static bool32 LoadHgssStorageBgAsset(
    const struct HgssStorageBgAsset *asset,
    u8 bg,
    u16 baseTile,
    u8 basePalette,
    u16 *dstTilemap,
    u8 dstLeft,
    u8 dstTop)
{
    u8 x, y;

    if (asset == NULL
     || asset->tiles == NULL
     || asset->tilemap == NULL
     || asset->palette == NULL
     || asset->tileCount == 0
     || asset->widthTiles == 0
     || asset->heightTiles == 0
     || asset->paletteBankCount == 0)
        return FALSE;

    // Storage BG1/BG2 share charbase 2. Keep authentic role tiles inside it.
    if (baseTile + asset->tileCount > 512)
        return FALSE;
    if (basePalette + asset->paletteBankCount > 16)
        return FALSE;
    if (dstLeft + asset->widthTiles > 32 || dstTop + asset->heightTiles > 32)
        return FALSE;

    LoadBgTiles(bg, asset->tiles, asset->tileCount * TILE_SIZE_4BPP, baseTile);
    LoadPalette(
        asset->palette,
        BG_PLTT_ID(basePalette),
        asset->paletteBankCount * PLTT_SIZE_4BPP);

    for (y = 0; y < asset->heightTiles; y++)
    {
        for (x = 0; x < asset->widthTiles; x++)
        {
            u16 src = asset->tilemap[y * asset->widthTiles + x];
            u16 tile = (src & 0x03FF) + baseTile;
            u16 palette = ((src >> 12) & 0x0F) + basePalette;

            if (tile >= 1024 || palette >= 16)
                return FALSE;

            dstTilemap[(dstTop + y) * 32 + dstLeft + x] =
                tile | (src & 0x0C00) | (palette << 12);
        }
    }

    return TRUE;
}

static void DrawHgssStorageMainFrame(void)
{
    u16 *tilemap = (u16 *)sStorage->displayMenuTilemapBuffer;

    // Phase 3: BG1 is an authentic-HGSS-only composition. Tile 0 is the
    // transparent tile carried by the verified main-frame pack.
    LoadHgssStorageBgAsset(
        &sHgssStorageMainFrameAsset,
        1,
        0,
        4,
        tilemap,
        0,
        0);
}

static void DrawHgssStoragePartySlots(void)
{
    u16 *tilemap = (u16 *)sStorage->displayMenuTilemapBuffer;
    u16 baseTile = sHgssStorageMainFrameAsset.tileCount;

    LoadHgssStorageBgAsset(
        &sHgssStoragePartyPanelAsset,
        1,
        baseTile,
        2,
        tilemap,
        2,
        0);

    ScheduleBgCopyTilemapToVram(1);
}

static void UpdateHgssStorageSlotHighlight(void)
{
    if (sChooseBoxMenu != NULL && sChooseBoxMenu->active)
        return;

    DrawHgssStoragePartySlots();
}

static void Task_InitBox(u8 taskId)
{
    struct Task *task = &gTasks[taskId];

    switch (task->tState)
    {
    case 0:
        task->tDmaIdx = RequestDma3Fill(0, sStorage->boxOverlayTilemapBuffer, sizeof(sStorage->boxOverlayTilemapBuffer), 1);
        break;
    case 1:
        if (CheckForSpaceForDma3Request(task->tDmaIdx) == -1)
            return;

        SetBgTilemapBuffer(2, sStorage->boxOverlayTilemapBuffer);
        ShowBg(2);
        break;
    case 2:
        LoadWallpaperGfx(task->tBoxId, 0);
        break;
    case 3:
        if (!WaitForWallpaperGfxLoad())
            return;

        InitBoxTitle(task->tBoxId);
        InitBoxMonSprites(task->tBoxId);
        UpdateHgssStorageSlotHighlight();
        SetGpuReg(REG_OFFSET_BG2CNT, BGCNT_PRIORITY(2) | BGCNT_CHARBASE(2) | BGCNT_SCREENBASE(27) | BGCNT_TXT256x256);
        break;
    case 4:
        DestroyTask(taskId);
        break;
    default:
        task->tState = 0;
        return;
    }

    task->tState++;
}

#undef tState
#undef tDmaIdx
#undef tBoxId

static void SetUpScrollToBox(u8 boxId)
{
    s8 direction = DetermineBoxScrollDirection(boxId);

    sStorage->scrollSpeed = (direction > 0) ? 6 : -6;
    sStorage->scrollTimer = 32;

    sStorage->scrollToBoxId = boxId;
    sStorage->scrollDirection = direction;
    sStorage->scrollState = 0;
}

#define BOX_TITLE_SPRITE_X  136


static void UpdateBoxTitle(u8 boxId)
{
    u16 i;
    u16 colors[2];
    struct SpriteSheet spriteSheet = {sStorage->boxTitleTiles, 0x200, GFXTAG_BOX_TITLE};
    struct SpriteTemplate template = sSpriteTemplate_BoxTitle;

    for (i = 0; i < 2; i++)
    {
        if (sStorage->curBoxTitleSprites[i])
            DestroySprite(sStorage->curBoxTitleSprites[i]);
    }

    FreeSpriteTilesByTag(GFXTAG_BOX_TITLE);
    RenderBoxTitleCentered(GetBoxNamePtr(boxId));
    LoadSpriteSheet(&spriteSheet);

    if (sCursorArea == CURSOR_AREA_BOX_TITLE || (sChooseBoxMenu != NULL && sChooseBoxMenu->active))
    {
        colors[0] = BOX_TITLE_SHADOW_HOVER;
        colors[1] = BOX_TITLE_TEXT_HOVER;
    }
    else
    {
        colors[0] = BOX_TITLE_SHADOW_MAIN;
        colors[1] = BOX_TITLE_TEXT_MAIN;
    }
    LoadPalette(colors, sStorage->boxTitlePalOffset, PLTT_SIZEOF(2));

    for (i = 0; i < 2; i++)
    {
        u8 spriteId = CreateSprite(&template, BOX_TITLE_SPRITE_X + i * 32, 21, 24);
        sStorage->curBoxTitleSprites[i] = &gSprites[spriteId];
        StartSpriteAnim(sStorage->curBoxTitleSprites[i], i);
    }
}

static bool8 ScrollToBox(void)
{
    bool8 iconsScrolling;

    switch (sStorage->scrollState)
    {
    case 0:
		BeginNormalPaletteFade(1 << 1, 0, 0, 16, RGB_WHITEALPHA);
        sStorage->scrollState++;
		return TRUE;
    case 1:
		UpdatePaletteFade();
		
		if (gPaletteFade.active)
            return TRUE;
		
        if (!WaitForWallpaperGfxLoad())
            return TRUE;

		
        InitBoxMonIconScroll(sStorage->scrollToBoxId, sStorage->scrollDirection);
		
		LoadWallpaperGfx(sStorage->scrollToBoxId, 0);
		
		BeginNormalPaletteFade(1 << 1, 0, 16, 0, RGB_WHITEALPHA);
		
		sStorage->scrollState++;
        break;
    case 2:
        iconsScrolling = UpdateBoxMonIconScroll();
		UpdatePaletteFade();
		
        if (sStorage->scrollTimer != 0)
        {
            if (--sStorage->scrollTimer != 0)
                return TRUE;
            sStorage->scrollState++;
            return TRUE;
        }
        return iconsScrolling;
    case 3:
        iconsScrolling = UpdateBoxMonIconScroll();
        UpdatePaletteFade();
		
        sStorage->scrollState++;
        return TRUE;
    case 4:
        iconsScrolling = UpdateBoxMonIconScroll();
		UpdatePaletteFade();
		
        if (!WaitForWallpaperGfxLoad())
            return TRUE;


        if (iconsScrolling)
            return TRUE;
		
		if (gPaletteFade.active)
            return TRUE;

        UpdateBoxTitle(sStorage->scrollToBoxId);
        return FALSE;
    }

    sStorage->scrollState++;
    return TRUE;
}

static s8 DetermineBoxScrollDirection(u8 boxId)
{
    u8 i;
    u8 currentBox = StorageGetCurrentBox();

    for (i = 0; currentBox != boxId; i++)
    {
        currentBox++;
        if (currentBox >= TOTAL_BOXES_COUNT)
            currentBox = 0;
    }

    return (i < TOTAL_BOXES_COUNT / 2) ? 1 : -1;
}


//------------------------------------------------------------------------------
//  SECTION: Wallpaper gfx
//------------------------------------------------------------------------------


static void SetWallpaperForCurrentBox(u8 wallpaperId)
{
    u8 boxId = StorageGetCurrentBox();
    SetBoxWallpaper(boxId, wallpaperId);
    sStorage->wallpaperChangeState = 0;
}

static bool8 DoWallpaperGfxChange(void)
{
    switch (sStorage->wallpaperChangeState)
    {
    case 0:
        BeginNormalPaletteFade(1 << 1, 0, 0, 16, RGB_WHITEALPHA);
        sStorage->wallpaperChangeState++;
        break;
    case 1:
        if (!UpdatePaletteFade())
        {
            u8 curBox = StorageGetCurrentBox();
            LoadWallpaperGfx(curBox, 0);
            sStorage->wallpaperChangeState++;
        }
        break;
    case 2:
        if (WaitForWallpaperGfxLoad() == TRUE)
        {
            BeginNormalPaletteFade(1 << 1, 0, 16, 0, RGB_WHITEALPHA);
            sStorage->wallpaperChangeState++;
        }
        break;
    case 3:
        if (!UpdatePaletteFade())
            sStorage->wallpaperChangeState++;
        break;
    case 4:
        return FALSE;
    }

    return TRUE;
}

static void LoadWallpaperGfx(u8 boxId, s8 direction)
{
    (void)direction;
    LoadHgssStorageWallpaper(GetBoxWallpaper(boxId));
}

static bool32 WaitForWallpaperGfxLoad(void)
{
    if (IsDma3ManagerBusyWithBgCopy())
        return FALSE;
    return TRUE;
}

//------------------------------------------------------------------------------
//  SECTION: Box Title
//------------------------------------------------------------------------------

static void InitBoxTitle(u8 boxId)
{
    u8 tagIndex;
    u16 i;
    struct SpriteSheet spriteSheet = {sStorage->boxTitleTiles, 0x200, GFXTAG_BOX_TITLE};

    CpuCopy16(sStatLabels_Pal, sStorage->boxTitlePal, sizeof(sStorage->boxTitlePal));

    if (sCursorArea == CURSOR_AREA_BOX_TITLE || (sChooseBoxMenu != NULL && sChooseBoxMenu->active))
    {
        sStorage->boxTitlePal[13] = BOX_TITLE_FRAME_HOVER;
        sStorage->boxTitlePal[14] = BOX_TITLE_SHADOW_HOVER;
        sStorage->boxTitlePal[15] = BOX_TITLE_TEXT_HOVER;
    }
    else
    {
        sStorage->boxTitlePal[13] = BOX_TITLE_FRAME_MAIN;
        sStorage->boxTitlePal[14] = BOX_TITLE_SHADOW_MAIN;
        sStorage->boxTitlePal[15] = BOX_TITLE_TEXT_MAIN;
    }

    tagIndex = IndexOfSpritePaletteTag(PALTAG_MISC_1);
    LoadPalette(sStorage->boxTitlePal, OBJ_PLTT_ID(tagIndex), 32);
    sStorage->wallpaperPalBits = 0x3f0;

    sStorage->boxTitlePalOffset = OBJ_PLTT_ID(tagIndex) + 14;
    sStorage->wallpaperPalBits |= (1 << 16) << tagIndex;

    sStorage->boxTitleAltPalOffset = OBJ_PLTT_ID(tagIndex) + 14;
    sStorage->wallpaperPalBits |= (1 << 16) << tagIndex;

    RenderBoxTitleCentered(GetBoxNamePtr(boxId));
    LoadSpriteSheet(&spriteSheet);

    for (i = 0; i < 2; i++)
    {
        u8 spriteId = CreateSprite(&sSpriteTemplate_BoxTitle, BOX_TITLE_SPRITE_X + i * 32, 21, 23);
        sStorage->curBoxTitleSprites[i] = &gSprites[spriteId];
        StartSpriteAnim(sStorage->curBoxTitleSprites[i], i);
    }
    sStorage->boxTitleCycleId = 0;
}

static void UpdateBoxTitlePalette(void)
{
    u16 colors[3];
    if (sCursorArea == CURSOR_AREA_BOX_TITLE || (sChooseBoxMenu != NULL && sChooseBoxMenu->active))
    {
        colors[0] = BOX_TITLE_FRAME_HOVER;
        colors[1] = BOX_TITLE_SHADOW_HOVER;
        colors[2] = BOX_TITLE_TEXT_HOVER;
    }
    else
    {
        colors[0] = BOX_TITLE_FRAME_MAIN;
        colors[1] = BOX_TITLE_SHADOW_MAIN;
        colors[2] = BOX_TITLE_TEXT_MAIN;
    }

    LoadPalette(&colors[0], sStorage->boxTitlePalOffset - 14 + 13, PLTT_SIZEOF(1));
    LoadPalette(&colors[0], sStorage->boxTitleAltPalOffset - 14 + 13, PLTT_SIZEOF(1));
    LoadPalette(&colors[1], sStorage->boxTitlePalOffset, PLTT_SIZEOF(2));
    LoadPalette(&colors[1], sStorage->boxTitleAltPalOffset, PLTT_SIZEOF(2));
}

static void RenderBoxTitleCentered(const u8 *boxName)
{
    u16 i;
    u8 *tileData1, *tileData2;
    u8 txtColor[3];
    struct WindowTemplate winTemplate = {0};
    u16 windowId;
    s16 xOffset = 32 - GetStringWidth(FONT_NORMAL, boxName, 0) / 2;

    if (xOffset < 3)
        xOffset = 3;

    winTemplate.width = 24;
    winTemplate.height = 2;
    windowId = AddWindow(&winTemplate);
    FillWindowPixelBuffer(windowId, PIXEL_FILL(0));

    tileData1 = (u8 *)GetWindowAttribute(windowId, WINDOW_TILE_DATA);
    tileData2 = tileData1 + winTemplate.width * TILE_SIZE_4BPP;
    txtColor[0] = TEXT_COLOR_TRANSPARENT;
    txtColor[1] = TEXT_DYNAMIC_COLOR_6;
    txtColor[2] = TEXT_DYNAMIC_COLOR_5;
    AddTextPrinterParameterized4(windowId, FONT_NORMAL, xOffset, 1, 0, 0, txtColor, TEXT_SKIP_DRAW, boxName);
    for (i = 0; i < 2; i++, tileData1 += 0x80, tileData2 += 0x80)
    {
        CpuCopy16(tileData1, sStorage->boxTitleTiles + i * 0x100, 0x80);
        CpuCopy16(tileData2, sStorage->boxTitleTiles + i * 0x100 + 0x80, 0x80);
    }
    RemoveWindow(windowId);
}


//------------------------------------------------------------------------------
//  SECTION: Cursor movement
//
//  The functions below generally handle the cursor's movement, including
//  moving around the box and picking up/putting down Pokémon.
//------------------------------------------------------------------------------


static void InitCursor(void)
{
    if (sStorage->boxOption != OPTION_DEPOSIT)
        sCursorArea = CURSOR_AREA_IN_BOX;
    else
        sCursorArea = CURSOR_AREA_IN_PARTY;

    if (sStorage->partySprites[0] == NULL)
        CreatePartyMonsSprites(TRUE);

    sCursorPosition = 0;
    sIsMonBeingMoved = FALSE;
    sMovingMonOrigBoxId = 0;
    sMovingMonOrigBoxPos = 0;
    sCursorMode = CURSOR_MODE_NORMAL;
    ClearSavedCursorPos();
    sStorage->cursorPrevHorizPos = 1;
    sStorage->inBoxMovingMode = MOVE_MODE_NORMAL;
    TryRefreshDisplayMon();
}

static void InitCursorOnReopen(void)
{
    if (sStorage->partySprites[0] == NULL)
        CreatePartyMonsSprites(TRUE);
    ReshowDisplayMon();
    sStorage->cursorPrevHorizPos = 1;
    sStorage->inBoxMovingMode = MOVE_MODE_NORMAL;
    if (sIsMonBeingMoved)
    {
        sStorage->movingMon = sSavedMovingMon;
        CreateMovingMonIcon();
    }
}

static void GetCursorCoordsByPos(u8 cursorArea, u8 cursorPosition, u16 *x, u16 *y)
{
    switch (cursorArea)
    {
    case CURSOR_AREA_IN_BOX:
        *x = (cursorPosition % IN_BOX_COLUMNS) * 24 + 92;
        *y = (cursorPosition / IN_BOX_COLUMNS) * 24 + 32;
        break;
    case CURSOR_AREA_IN_PARTY:
        *x = 40;
        *y = cursorPosition * 24 + 8;
        break;
    case CURSOR_AREA_BOX_TITLE:
        *x = 152;
        *y = 10;
        break;
    case 4:
        *x = 160;
        *y = 96;
        break;
    }
}

static u16 GetSpeciesAtCursorPosition(void)
{
    switch (sCursorArea)
    {
    case CURSOR_AREA_IN_PARTY:
        return GetMonData(&gParties[B_TRAINER_PLAYER][sCursorPosition], MON_DATA_SPECIES);
    case CURSOR_AREA_IN_BOX:
        return GetCurrentBoxMonData(sCursorPosition, MON_DATA_SPECIES);
    default:
        return SPECIES_NONE;
    }
}

static bool8 UpdateCursorPos(void)
{
    s16 tmp;

    if (sStorage->cursorMoveSteps == 0)
    {
        if (sStorage->boxOption != OPTION_MOVE_ITEMS)
            return FALSE;
        else
            return IsItemIconAnimActive();
    }
    else if (--sStorage->cursorMoveSteps != 0)
    {
        // Update position toward target
        sStorage->cursorNewX += sStorage->cursorSpeedX;
        sStorage->cursorNewY += sStorage->cursorSpeedY;
        sStorage->cursorSprite->x = sStorage->cursorNewX >> 8;
        sStorage->cursorSprite->y = sStorage->cursorNewY >> 8;

        // Limit cursor on right
        if (sStorage->cursorSprite->x > DISPLAY_WIDTH + 16)
        {
            tmp = sStorage->cursorSprite->x - (DISPLAY_WIDTH + 16);
            sStorage->cursorSprite->x = tmp + 64;
        }

        // Limit cursor on left
        if (sStorage->cursorSprite->x < -16)
        {
            tmp = -16 - sStorage->cursorSprite->x;
            sStorage->cursorSprite->x = DISPLAY_WIDTH + 16 - tmp;
        }

        // Limit cursor on bottom
        if (sStorage->cursorSprite->y > DISPLAY_HEIGHT + 16)
        {
            tmp = sStorage->cursorSprite->y - (DISPLAY_HEIGHT + 16);
            sStorage->cursorSprite->y = tmp - 16;
        }

        // Limit cursor on top
        if (sStorage->cursorSprite->y < -16)
        {
            tmp = -16 - sStorage->cursorSprite->y;
            sStorage->cursorSprite->y = DISPLAY_HEIGHT + 16 - tmp;
        }

        // Cursor flips vertically when moving on/off the top buttons
        if (sStorage->cursorFlipTimer && --sStorage->cursorFlipTimer == 0)
            sStorage->cursorSprite->vFlip = (sStorage->cursorSprite->vFlip == FALSE);
    }
    else
    {
        // Time is up for cursor movement, make sure it's exactly at target
        sStorage->cursorSprite->x = sStorage->cursorTargetX;
        sStorage->cursorSprite->y = sStorage->cursorTargetY;
        DoCursorNewPosUpdate();
    }

    return TRUE;
}

static void InitNewCursorPos(u8 newCursorArea, u8 newCursorPosition)
{
    u16 x = 0, y = 0;

    GetCursorCoordsByPos(newCursorArea, newCursorPosition, &x, &y);
    sStorage->newCursorArea = newCursorArea;
    sStorage->newCursorPosition = newCursorPosition;
    sStorage->cursorTargetX = x;
    sStorage->cursorTargetY = y;
}

static void InitCursorMove(void)
{
    int yDistance, xDistance;

    if (sStorage->cursorVerticalWrap != 0 || sStorage->cursorHorizontalWrap != 0)
        sStorage->cursorMoveSteps = 12;
    else
        sStorage->cursorMoveSteps = 6;

    if (sStorage->cursorFlipTimer)
        sStorage->cursorFlipTimer = sStorage->cursorMoveSteps >> 1;

    switch (sStorage->cursorVerticalWrap)
    {
    default: // No wrap
        yDistance = sStorage->cursorTargetY - sStorage->cursorSprite->y;
        break;
    case -1: // Wrap from top to bottom
        yDistance = sStorage->cursorTargetY - 192 - sStorage->cursorSprite->y;
        break;
    case 1: // Wrap from bottom to top
        yDistance = sStorage->cursorTargetY + 192 - sStorage->cursorSprite->y;
        break;
    }

    switch (sStorage->cursorHorizontalWrap)
    {
    default: // No Wrap
        xDistance = sStorage->cursorTargetX - sStorage->cursorSprite->x;
        break;
    case -1: // Wrap from left to right
        xDistance = sStorage->cursorTargetX - 192 - sStorage->cursorSprite->x;
        break;
    case 1: // Wrap from right to left
        xDistance = sStorage->cursorTargetX + 192 - sStorage->cursorSprite->x;
        break;
    }

    yDistance <<= 8;
    xDistance <<= 8;
    sStorage->cursorSpeedX = xDistance / sStorage->cursorMoveSteps;
    sStorage->cursorSpeedY = yDistance / sStorage->cursorMoveSteps;
    sStorage->cursorNewX = sStorage->cursorSprite->x << 8;
    sStorage->cursorNewY = sStorage->cursorSprite->y << 8;
}

static void SetCursorPosition(u8 newCursorArea, u8 newCursorPosition)
{
    InitNewCursorPos(newCursorArea, newCursorPosition);
    InitCursorMove();
    if (sStorage->boxOption != OPTION_MOVE_ITEMS)
    {
        if (sStorage->inBoxMovingMode == MOVE_MODE_NORMAL && !sIsMonBeingMoved)
            StartSpriteAnim(sStorage->cursorSprite, CURSOR_ANIM_MAIN);
    }
    else
    {
        if (!IsMovingItem())
            StartSpriteAnim(sStorage->cursorSprite, CURSOR_ANIM_MAIN);
    }

    if (sStorage->boxOption == OPTION_MOVE_ITEMS)
    {
        if (sCursorArea == CURSOR_AREA_IN_BOX)
            TryHideItemIconAtPos(CURSOR_AREA_IN_BOX, sCursorPosition);
        else if (sCursorArea == CURSOR_AREA_IN_PARTY)
            TryHideItemIconAtPos(CURSOR_AREA_IN_PARTY, sCursorPosition);

        if (newCursorArea == CURSOR_AREA_IN_BOX)
            TryLoadItemIconAtPos(newCursorArea, newCursorPosition);
        else if (newCursorArea == CURSOR_AREA_IN_PARTY)
            TryLoadItemIconAtPos(newCursorArea, newCursorPosition);
    }

    if (newCursorArea == CURSOR_AREA_IN_PARTY && sCursorArea != CURSOR_AREA_IN_PARTY)
    {
        sStorage->cursorPrevHorizPos = 1;
    }

    switch (newCursorArea)
    {
    case CURSOR_AREA_IN_PARTY:
    case CURSOR_AREA_BOX_TITLE:
        sStorage->cursorSprite->oam.priority = 1;
        break;
    case CURSOR_AREA_IN_BOX:
        if (sStorage->inBoxMovingMode != MOVE_MODE_NORMAL)
        {
            sStorage->cursorSprite->oam.priority = 0;
        }
        else
        {
            sStorage->cursorSprite->oam.priority = 1;
            if (sCursorArea == CURSOR_AREA_IN_BOX && sIsMonBeingMoved)
                SetMovingMonPriority(1);
        }
        break;
    }
}

static void DoCursorNewPosUpdate(void)
{
    sCursorArea = sStorage->newCursorArea;
    sCursorPosition = sStorage->newCursorPosition;
    if (sStorage->boxOption != OPTION_MOVE_ITEMS)
    {
        if (sStorage->inBoxMovingMode == MOVE_MODE_NORMAL && !sIsMonBeingMoved)
            StartSpriteAnim(sStorage->cursorSprite, CURSOR_ANIM_BOUNCE);
    }
    else
    {
        if (!IsMovingItem())
            StartSpriteAnim(sStorage->cursorSprite, CURSOR_ANIM_BOUNCE);
    }

    switch (sCursorArea)
    {
    case CURSOR_AREA_BOX_TITLE:
        TryRefreshDisplayMon();
        UpdateBoxTitlePalette();
        break;
    case CURSOR_AREA_IN_PARTY:
        TryRefreshDisplayMon();
        UpdateBoxTitlePalette();
        SetMovingMonPriority(1);
        break;
    case CURSOR_AREA_IN_BOX:
        TryRefreshDisplayMon();
        UpdateBoxTitlePalette();
        if (sStorage->inBoxMovingMode == MOVE_MODE_NORMAL)
        {
            sStorage->cursorSprite->oam.priority = 1;
            SetMovingMonPriority(1);
        }
        break;
    }

    UpdateHgssStorageSlotHighlight();
}

static void SetCursorInParty(void)
{
    u8 partyCount;

    if (!sIsMonBeingMoved)
    {
        partyCount = 0;
    }
    else
    {
        partyCount = CalculatePlayerPartyCount();
        if (partyCount >= PARTY_SIZE)
            partyCount = PARTY_SIZE - 1;
    }
    if (sStorage->cursorSprite->vFlip)
        sStorage->cursorFlipTimer = 1;
    SetCursorPosition(CURSOR_AREA_IN_PARTY, partyCount);
}

static void SetCursorBoxPosition(u8 cursorBoxPosition)
{
    SetCursorPosition(CURSOR_AREA_IN_BOX, cursorBoxPosition);
}

EWRAM_DATA static u8 sSavedCursorPosition = 0;

static void ClearSavedCursorPos(void)
{
    sSavedCursorPosition = 0;
}

static void SaveCursorPos(void)
{
    sSavedCursorPosition = sCursorPosition;
}

static u8 GetSavedCursorPos(void)
{
    return sSavedCursorPosition;
}

static void InitMonPlaceChange(u8 type)
{
    static bool8 (*const placeChangeFuncs[])(void) =
    {
        [CHANGE_GRAB]  = MonPlaceChange_Grab,
        [CHANGE_PLACE] = MonPlaceChange_Place,
        [CHANGE_SHIFT] = MonPlaceChange_Shift,
    };

    sStorage->monPlaceChangeFunc = placeChangeFuncs[type];
    sStorage->monPlaceChangeState = 0;
}

// No Shift while moving multiple Pokémon, only grab and place
// For both grab/place, the cursor moves down, then up
static void InitMultiMonPlaceChange(bool8 up)
{
    if (!up)
        sStorage->monPlaceChangeFunc = MultiMonPlaceChange_Down;
    else
        sStorage->monPlaceChangeFunc = MultiMonPlaceChange_Up;

    sStorage->monPlaceChangeState = 0;
}

static bool8 DoMonPlaceChange(void)
{
    if (sStorage->monPlaceChangeFunc())
        return TRUE;

    UpdateMonInfoTilemap();
    return FALSE;
}

static bool8 MonPlaceChange_Grab(void)
{
    switch (sStorage->monPlaceChangeState)
    {
    case 0:
        if (sIsMonBeingMoved)
            return FALSE;
        StartSpriteAnim(sStorage->cursorSprite, CURSOR_ANIM_MAIN);
        sStorage->monPlaceChangeState++;
        break;
    case 1:
        if (!MonPlaceChange_CursorDown())
        {
            StartSpriteAnim(sStorage->cursorSprite, CURSOR_ANIM_MAIN);
            MoveMon();
            sStorage->monPlaceChangeState++;
        }
        break;
    case 2:
        if (!MonPlaceChange_CursorUp())
            sStorage->monPlaceChangeState++;
        break;
    case 3:
        return FALSE;
    }

    return TRUE;
}

static bool8 MonPlaceChange_Place(void)
{
    switch (sStorage->monPlaceChangeState)
    {
    case 0:
        if (!MonPlaceChange_CursorDown())
        {
            StartSpriteAnim(sStorage->cursorSprite, CURSOR_ANIM_MAIN);
            PlaceMon();
            sStorage->monPlaceChangeState++;
        }
        break;
    case 1:
        if (!MonPlaceChange_CursorUp())
        {
            StartSpriteAnim(sStorage->cursorSprite, CURSOR_ANIM_BOUNCE);
            sStorage->monPlaceChangeState++;
        }
        break;
    case 2:
        return FALSE;
    }

    return TRUE;
}

static bool8 MonPlaceChange_Shift(void)
{
    switch (sStorage->monPlaceChangeState)
    {
    case 0:
        switch (sCursorArea)
        {
        case CURSOR_AREA_IN_PARTY:
            sStorage->shiftBoxId = TOTAL_BOXES_COUNT;
            break;
        case CURSOR_AREA_IN_BOX:
            sStorage->shiftBoxId = StorageGetCurrentBox();
            break;
        default:
            return FALSE;
        }
        StartSpriteAnim(sStorage->cursorSprite, CURSOR_ANIM_MAIN);
        SaveMonSpriteAtPos(sStorage->shiftBoxId, sCursorPosition);
        sStorage->monPlaceChangeState++;
        break;
    case 1:
        if (!MoveShiftingMons())
        {
            StartSpriteAnim(sStorage->cursorSprite, CURSOR_ANIM_MAIN);
            SetShiftedMonData(sStorage->shiftBoxId, sCursorPosition);
            sStorage->monPlaceChangeState++;
        }
        break;
    case 2:
        return FALSE;
    }

    return TRUE;
}

static bool8 MultiMonPlaceChange_Down(void)
{
    return MonPlaceChange_CursorDown();
}

static bool8 MultiMonPlaceChange_Up(void)
{
    return MonPlaceChange_CursorUp();
}

static bool8 MonPlaceChange_CursorDown(void)
{
    switch (sStorage->cursorSprite->y2)
    {
    default:
        sStorage->cursorSprite->y2++;
        break;
    case 0:
        sStorage->cursorSprite->y2++;
        break;
    case 8: // Cursor has reached bottom
        return FALSE;
    }

    return TRUE;
}

static bool8 MonPlaceChange_CursorUp(void)
{
    switch (sStorage->cursorSprite->y2)
    {
    case 0: // Cursor has reached top
        return FALSE;
    default:
        sStorage->cursorSprite->y2--;
        break;
    }

    return TRUE;
}


//------------------------------------------------------------------------------
//  SECTION: Pokémon data
//
//  The functions below handle moving Pokémon data around while using the PC,
//  including changing the positions of Pokémon, releasing Pokémon, viewing the
//  summary screen, and updating the display of the currently selected Pokémon.
//------------------------------------------------------------------------------


static void MoveMon(void)
{
    switch (sCursorArea)
    {
    case CURSOR_AREA_IN_PARTY:
        SetMovingMonData(TOTAL_BOXES_COUNT, sCursorPosition);
        SetMovingMonSprite(MODE_PARTY, sCursorPosition);
        break;
    case CURSOR_AREA_IN_BOX:
        if (sStorage->inBoxMovingMode == MOVE_MODE_NORMAL)
        {
            SetMovingMonData(StorageGetCurrentBox(), sCursorPosition);
            SetMovingMonSprite(MODE_BOX, sCursorPosition);
        }
        break;
    default:
        return;
    }

    sIsMonBeingMoved = TRUE;
}

static void PlaceMon(void)
{
    u8 boxId;

    switch (sCursorArea)
    {
    case CURSOR_AREA_IN_PARTY:
        SetPlacedMonData(TOTAL_BOXES_COUNT, sCursorPosition);
        SetPlacedMonSprite(TOTAL_BOXES_COUNT, sCursorPosition);
        UpdateSpeciesSpritePSS(&gParties[B_TRAINER_PLAYER][sCursorPosition].box);
        break;
    case CURSOR_AREA_IN_BOX:
        boxId = StorageGetCurrentBox();
        SetPlacedMonData(boxId, sCursorPosition);
        SetPlacedMonSprite(boxId, sCursorPosition);
        UpdateSpeciesSpritePSS(&gPokemonStoragePtr->boxes[boxId][sCursorPosition]);
        break;
    default:
        return;
    }

    sIsMonBeingMoved = FALSE;
}

static void RefreshDisplayMon(void)
{
    TryRefreshDisplayMon();
}

static void SetMovingMonData(u8 boxId, u8 position)
{
    if (boxId == TOTAL_BOXES_COUNT)
    {
        sStorage->movingMon = gParties[B_TRAINER_PLAYER][sCursorPosition];
        if (&gParties[B_TRAINER_PLAYER][sCursorPosition] == GetFirstLiveMon())
            gFollowerSteps = 0;
    }
    else
    {
        BoxMonAtToMon(boxId, position, &sStorage->movingMon);
    }

    PurgeMonOrBoxMon(boxId, position);
    sMovingMonOrigBoxId = boxId;
    sMovingMonOrigBoxPos = position;
}

static void SetPlacedMonData(u8 boxId, u8 position)
{
    if (OW_PC_HEAL <= GEN_7)
        HealPokemon(&sStorage->movingMon);

    if (boxId == TOTAL_BOXES_COUNT)
    {
        gParties[B_TRAINER_PLAYER][position] = sStorage->movingMon;
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][position];
        if (mon == GetFirstLiveMon())
            gFollowerSteps = 0;
        SetMonFormPSS(&mon->box, FORM_CHANGE_WITHDRAW);
    }
    else
    {
        SetBoxMonAt(boxId, position, &sStorage->movingMon.box);
        SetMonFormPSS(&gPokemonStoragePtr->boxes[boxId][position], FORM_CHANGE_DEPOSIT);
    }
}

static void PurgeMonOrBoxMon(u8 boxId, u8 position)
{
    if (boxId == TOTAL_BOXES_COUNT)
        ZeroMonData(&gParties[B_TRAINER_PLAYER][position]);
    else
        ZeroBoxMonAt(boxId, position);
}

static void SetShiftedMonData(u8 boxId, u8 position)
{
    if (boxId == TOTAL_BOXES_COUNT)
        sStorage->tempMon = gParties[B_TRAINER_PLAYER][position];
    else
        BoxMonAtToMon(boxId, position, &sStorage->tempMon);

    SetPlacedMonData(boxId, position);
    
    if (boxId == TOTAL_BOXES_COUNT)
        UpdateSpeciesSpritePSS(&gParties[B_TRAINER_PLAYER][position].box);
    else
        UpdateSpeciesSpritePSS(&gPokemonStoragePtr->boxes[boxId][position]);
    
    sStorage->movingMon = sStorage->tempMon;
    SetDisplayMonData(&sStorage->movingMon, MODE_PARTY);
    sMovingMonOrigBoxId = boxId;
    sMovingMonOrigBoxPos = position;
}

static void InitReleaseMon(void)
{
    u8 mode;

    if (sIsMonBeingMoved)
        mode = MODE_MOVE;
    else if (sCursorArea == CURSOR_AREA_IN_PARTY)
        mode = MODE_PARTY;
    else
        mode = MODE_BOX;

    SetReleaseMon(mode, sCursorPosition);
    StringCopy(sStorage->releaseMonName, sStorage->displayMon.nickname);
}

static bool8 TryHideReleaseMon(void)
{
    if (!TryHideReleaseMonSprite())
    {
        StartSpriteAnim(sStorage->cursorSprite, CURSOR_ANIM_BOUNCE);
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

static void ReleaseMon(void)
{
    u8 boxId;
    enum Item item = ITEM_NONE;

    DestroyReleaseMonIcon();
    if (sIsMonBeingMoved)
    {
        sIsMonBeingMoved = FALSE;
    }
    else
    {
        if (sCursorArea == CURSOR_AREA_IN_PARTY)
        {
            boxId = TOTAL_BOXES_COUNT;
            if (OW_PC_RELEASE_ITEM >= GEN_8)
                item = GetMonData(&gParties[B_TRAINER_PLAYER][sCursorPosition], MON_DATA_HELD_ITEM);
        }
        else
        {
            boxId = StorageGetCurrentBox();
            if (OW_PC_RELEASE_ITEM >= GEN_8)
                item = GetBoxMonDataAt(boxId, sCursorPosition, MON_DATA_HELD_ITEM);
        }

        PurgeMonOrBoxMon(boxId, sCursorPosition);
        if (item != ITEM_NONE)
            AddBagItem(item, 1);
    }
    TryRefreshDisplayMon();
}

static void TrySetCursorFistAnim(void)
{
    if (sIsMonBeingMoved)
        StartSpriteAnim(sStorage->cursorSprite, CURSOR_ANIM_MAIN);
}

// If the player is on the listed map (or any map, if none is specified),
// they may not release their last Pokémon that knows the specified move.
// This is to stop the player from softlocking themselves by not having
// a Pokémon that knows a required field move.
struct
{
    s8 mapGroup;
    s8 mapNum;
    u16 move;
} static const sRestrictedReleaseMoves[] =
{
    {MAP_GROUPS_COUNT, 0, MOVE_SURF},
    {MAP_GROUPS_COUNT, 0, MOVE_DIVE},
    {MAP_GROUP(MAP_EVER_GRANDE_CITY_POKEMON_LEAGUE_1F), MAP_NUM(MAP_EVER_GRANDE_CITY_POKEMON_LEAGUE_1F), MOVE_STRENGTH},
    {MAP_GROUP(MAP_EVER_GRANDE_CITY_POKEMON_LEAGUE_1F), MAP_NUM(MAP_EVER_GRANDE_CITY_POKEMON_LEAGUE_1F), MOVE_ROCK_SMASH},
    {MAP_GROUP(MAP_EVER_GRANDE_CITY_POKEMON_LEAGUE_2F), MAP_NUM(MAP_EVER_GRANDE_CITY_POKEMON_LEAGUE_2F), MOVE_STRENGTH},
    {MAP_GROUP(MAP_EVER_GRANDE_CITY_POKEMON_LEAGUE_2F), MAP_NUM(MAP_EVER_GRANDE_CITY_POKEMON_LEAGUE_2F), MOVE_ROCK_SMASH},
};

static void GetRestrictedReleaseMoves(u16 *moves)
{
    s32 i;

    for (i = 0; i < ARRAY_COUNT(sRestrictedReleaseMoves); i++)
    {
        if (sRestrictedReleaseMoves[i].mapGroup == MAP_GROUPS_COUNT
            || (sRestrictedReleaseMoves[i].mapGroup == gSaveBlock1Ptr->location.mapGroup
                && sRestrictedReleaseMoves[i].mapNum == gSaveBlock1Ptr->location.mapNum))
        {
            *moves = sRestrictedReleaseMoves[i].move;
            moves++;
        }
    }
    *moves = MOVES_COUNT;
}

static void InitCanReleaseMonVars(void)
{
    if (!AtLeastThreeUsableMons())
    {
        // The player only has 1 or 2 usable
        // Pokémon, this one can't be released
        sStorage->releaseStatusResolved = TRUE;
        sStorage->canReleaseMon = FALSE;
        return;
    }

    if (sIsMonBeingMoved)
    {
        sStorage->tempMon = sStorage->movingMon;
        sStorage->releaseBoxId = -1;
        sStorage->releaseBoxPos = -1;
    }
    else
    {
        if (sCursorArea == CURSOR_AREA_IN_PARTY)
        {
            sStorage->tempMon = gParties[B_TRAINER_PLAYER][sCursorPosition];
            sStorage->releaseBoxId = TOTAL_BOXES_COUNT;
        }
        else
        {
            BoxMonAtToMon(StorageGetCurrentBox(), sCursorPosition, &sStorage->tempMon);
            sStorage->releaseBoxId = StorageGetCurrentBox();
        }
        sStorage->releaseBoxPos = sCursorPosition;
    }

    GetRestrictedReleaseMoves(sStorage->restrictedMoveList);
    sStorage->restrictedReleaseMonMoves = GetMonData(&sStorage->tempMon, MON_DATA_KNOWN_MOVES, (u8 *)sStorage->restrictedMoveList);
    if (sStorage->restrictedReleaseMonMoves != 0)
    {
        // Pokémon knows at least one restricted release move
        // Need to check if another Pokémon has this move first
        sStorage->releaseStatusResolved = FALSE;
    }
    else
    {
        // Pokémon knows no restricted moves, can be released
        sStorage->releaseStatusResolved = TRUE;
        sStorage->canReleaseMon = TRUE;
    }

    sStorage->releaseCheckState = 0;
}

static bool32 AtLeastThreeUsableMons(void)
{
    s32 i, j;
    s32 count = (sIsMonBeingMoved != FALSE);

    // Check party for usable Pokémon
    for (j = 0; j < PARTY_SIZE; j++)
    {
        if (GetMonData(&gParties[B_TRAINER_PLAYER][j], MON_DATA_SANITY_HAS_SPECIES))
            count++;
    }

    if (count >= 3)
        return TRUE;

    // Check PC for usable Pokémon
    for (i = 0; i < TOTAL_BOXES_COUNT; i++)
    {
        for (j = 0; j < IN_BOX_COUNT; j++)
        {
            if (CheckBoxMonSanityAt(i, j))
            {
                if (++count >= 3)
                    return TRUE;
            }
        }
    }

    return FALSE;
}

static s8 RunCanReleaseMon(void)
{
    u16 i;
    u16 knownMoves;

    if (sStorage->releaseStatusResolved)
        return sStorage->canReleaseMon;

    switch (sStorage->releaseCheckState)
    {
    case 0:
        // Check party for other Pokémon that know any restricted
        // moves the release Pokémon knows
        for (i = 0; i < PARTY_SIZE; i++)
        {
            // Make sure party Pokémon isn't the one we're releasing first
            if (sStorage->releaseBoxId != TOTAL_BOXES_COUNT || sStorage->releaseBoxPos != i)
            {
                knownMoves = GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_KNOWN_MOVES, (u8 *)sStorage->restrictedMoveList);
                sStorage->restrictedReleaseMonMoves &= ~(knownMoves);
            }
        }
        if (sStorage->restrictedReleaseMonMoves == 0)
        {
            // No restricted moves on release Pokémon that
            // aren't resolved by the party, it can be released.
            sStorage->releaseStatusResolved = TRUE;
            sStorage->canReleaseMon = TRUE;
        }
        else
        {
            // Release Pokémon has restricted moves not resolved by the party.
            // Continue and check the PC next
            sStorage->releaseCheckBoxId = 0;
            sStorage->releaseCheckBoxPos = 0;
            sStorage->releaseCheckState++;
        }
        break;
    case 1:
        // Check PC for other Pokémon that know any restricted
        // moves the release Pokémon knows
        for (i = 0; i < IN_BOX_COUNT; i++)
        {
            knownMoves = GetAndCopyBoxMonDataAt(sStorage->releaseCheckBoxId, sStorage->releaseCheckBoxPos, MON_DATA_KNOWN_MOVES, (u8 *)sStorage->restrictedMoveList);
            if (knownMoves != 0 && !(sStorage->releaseBoxId == sStorage->releaseCheckBoxId
                                    && sStorage->releaseBoxPos == sStorage->releaseCheckBoxPos))
            {
                // Found PC Pokémon with restricted move, clear move from list
                sStorage->restrictedReleaseMonMoves &= ~(knownMoves);
                if (sStorage->restrictedReleaseMonMoves == 0)
                {
                    // No restricted moves on release Pokémon that
                    // aren't resolved, it can be released.
                    sStorage->releaseStatusResolved = TRUE;
                    sStorage->canReleaseMon = TRUE;
                    break;
                }
            }
            if (++sStorage->releaseCheckBoxPos >= IN_BOX_COUNT)
            {
                sStorage->releaseCheckBoxPos = 0;
                if (++sStorage->releaseCheckBoxId >= TOTAL_BOXES_COUNT)
                {
                    // Checked every Pokémon in the PC, release Pokémon is
                    // the sole owner of at least one restricted move.
                    // It cannot be released.
                    sStorage->releaseStatusResolved = TRUE;
                    sStorage->canReleaseMon = FALSE;
                }
            }
        }
        break;
    }

    return -1;
}

static void SaveMovingMon(void)
{
    if (sIsMonBeingMoved)
        sSavedMovingMon = sStorage->movingMon;
}

static void LoadSavedMovingMon(void)
{
    if (sIsMonBeingMoved)
    {
        // If it came from the party load a struct Pokemon,
        // otherwise load a BoxPokemon
        if (sMovingMonOrigBoxId == TOTAL_BOXES_COUNT)
            sStorage->movingMon = sSavedMovingMon;
        else
            sStorage->movingMon.box = sSavedMovingMon.box;
    }
}

static void InitSummaryScreenData(void)
{
    if (sIsMonBeingMoved)
    {
        SaveMovingMon();
        sStorage->summaryMon.mon = &sSavedMovingMon;
        sStorage->summaryStartPos = 0;
        sStorage->summaryMaxPos = 0;
        sStorage->summaryScreenMode = SUMMARY_MODE_BOX_CURSOR;
    }
    else if (sCursorArea == CURSOR_AREA_IN_PARTY)
    {
        sStorage->summaryMon.mon = gParties[B_TRAINER_PLAYER];
        sStorage->summaryStartPos = sCursorPosition;
        sStorage->summaryMaxPos = CountPartyMons() - 1;
        sStorage->summaryScreenMode = SUMMARY_MODE_NORMAL;
    }
    else
    {
        sStorage->summaryMon.box = GetBoxedMonPtr(StorageGetCurrentBox(), 0);
        sStorage->summaryStartPos = sCursorPosition;
        sStorage->summaryMaxPos = IN_BOX_COUNT - 1;
        sStorage->summaryScreenMode = SUMMARY_MODE_BOX;
    }
}

static void SetSelectionAfterSummaryScreen(void)
{
    if (sIsMonBeingMoved)
        LoadSavedMovingMon();
    else
        sCursorPosition = gLastViewedMonIndex;
}

static void SetMonMarkings(u8 markings)
{
    sStorage->displayMon.markings = markings;
    if (sIsMonBeingMoved)
    {
        SetMonData(&sStorage->movingMon, MON_DATA_MARKINGS, &markings);
    }
    else
    {
        if (sCursorArea == CURSOR_AREA_IN_PARTY)
            SetMonData(&gParties[B_TRAINER_PLAYER][sCursorPosition], MON_DATA_MARKINGS, &markings);
        if (sCursorArea == CURSOR_AREA_IN_BOX)
            SetCurrentBoxMonData(sCursorPosition, MON_DATA_MARKINGS, &markings);
    }
}

static bool8 IsRemovingLastPartyMon(void)
{
    if (sCursorArea == CURSOR_AREA_IN_PARTY
        && !sIsMonBeingMoved
        && CountPartyAliveNonEggMonsExcept(sCursorPosition) == 0)
        return TRUE;
    else
        return FALSE;
}

static bool8 CanPlaceMon(void)
{
    if (sIsMonBeingMoved)
    {
        if (sCursorArea == CURSOR_AREA_IN_PARTY
            && GetMonData(&gParties[B_TRAINER_PLAYER][sCursorPosition], MON_DATA_SPECIES) == SPECIES_NONE)
            return TRUE;
        else if (sCursorArea == CURSOR_AREA_IN_BOX
                && GetBoxMonDataAt(StorageGetCurrentBox(), sCursorPosition, MON_DATA_SPECIES_OR_EGG) == SPECIES_NONE)
            return TRUE;
        else
            return FALSE;
    }
    return FALSE;
}

static bool8 CanShiftMon(void)
{
    if (sIsMonBeingMoved)
    {
        if (sCursorArea == CURSOR_AREA_IN_PARTY && CountPartyAliveNonEggMonsExcept(sCursorPosition) == 0)
        {
            if (sStorage->displayMon.isEgg || GetMonData(&sStorage->movingMon, MON_DATA_HP) == 0)
                return FALSE;
        }
        return TRUE;
    }
    return FALSE;
}

static bool8 IsMonBeingMoved(void)
{
    return sIsMonBeingMoved;
}

static bool8 IsCursorOnBoxTitle(void)
{
    return (sCursorArea == CURSOR_AREA_BOX_TITLE);
}

static void TryRefreshDisplayMon(void)
{
    // If a Pokémon is currently being moved, don't start
    // mosaic or update display. Keep displaying the
    // currently held Pokémon.
    sStorage->setMosaic = (sIsMonBeingMoved == FALSE);
    if (!sIsMonBeingMoved)
    {
        // Update display Pokémon
        switch (sCursorArea)
        {
        case CURSOR_AREA_IN_PARTY:
            if (sCursorPosition < PARTY_SIZE)
            {
                SetDisplayMonData(&gParties[B_TRAINER_PLAYER][sCursorPosition], MODE_PARTY);
                break;
            }
            // fallthrough
        case CURSOR_AREA_BOX_TITLE:
            SetDisplayMonData(NULL, MODE_MOVE);
            break;
        case CURSOR_AREA_IN_BOX:
            SetDisplayMonData(GetBoxedMonPtr(StorageGetCurrentBox(), sCursorPosition), MODE_BOX);
            break;
        }
    }
}

static void ReshowDisplayMon(void)
{
    if (sIsMonBeingMoved)
        SetDisplayMonData(&sSavedMovingMon, MODE_PARTY);
    else
        TryRefreshDisplayMon();
}

void SetMonFormPSS_SwSh(struct BoxPokemon *boxMon, enum FormChanges method)
{
    if (TryBoxMonFormChange(boxMon, method))
        sRefreshDisplayMonGfx = TRUE;
}

void SetMonFormPSS_ItemHold_SwSh(struct BoxPokemon *boxMon)
{
    if (TryBoxMonFormChange(boxMon, FORM_CHANGE_ITEM_HOLD))
        sRefreshDisplayMonGfx = TRUE;
    UpdateSpeciesSpritePSS_SwSh(boxMon);
}

static void SetDisplayMonData(void *pokemon, u8 mode)
{
    u8 *txtPtr;
    u8 gender;
    bool8 sanityIsBadEgg;

    sStorage->displayMon.heldItem = ITEM_NONE;
    gender = MON_MALE;
    sanityIsBadEgg = FALSE;
    if (mode == MODE_PARTY)
    {
        struct Pokemon *mon = (struct Pokemon *)pokemon;

        sStorage->displayMon.species = GetMonData(mon, MON_DATA_SPECIES);
        if (sStorage->displayMon.species != SPECIES_NONE)
        {
			u8 pokerusDays = GetMonData(mon, MON_DATA_POKERUS_DAYS_LEFT);
            sanityIsBadEgg = GetMonData(mon, MON_DATA_SANITY_IS_BAD_EGG);
            if (sanityIsBadEgg)
                sStorage->displayMon.isEgg = TRUE;
            else
                sStorage->displayMon.isEgg = GetMonData(mon, MON_DATA_IS_EGG);

            GetMonData(mon, MON_DATA_NICKNAME, sStorage->displayMon.nickname);
            StringGet_Nickname(sStorage->displayMon.nickname);
            sStorage->displayMon.level = GetMonData(mon, MON_DATA_LEVEL);
            sStorage->displayMon.markings = GetMonData(mon, MON_DATA_MARKINGS);
            sStorage->displayMon.personality = GetMonData(mon, MON_DATA_PERSONALITY);
            sStorage->displayMon.palette = GetMonFrontSpritePal(mon);
            sStorage->displayMon.isShiny = GetMonData(mon, MON_DATA_IS_SHINY);
			sStorage->displayMon.hasPokerus = (pokerusDays > 0);
            gender = GetMonGender(mon);
            sStorage->displayMon.gender = gender;
            sStorage->displayMon.heldItem = GetMonData(mon, MON_DATA_HELD_ITEM);

            sStorage->displayMon.types[0] = gSpeciesInfo[sStorage->displayMon.species].types[0];
            sStorage->displayMon.types[1] = gSpeciesInfo[sStorage->displayMon.species].types[1];

            sStorage->displayMon.maxHP = GetMonData(mon, MON_DATA_MAX_HP);
            sStorage->displayMon.atk = GetMonData(mon, MON_DATA_ATK);
            sStorage->displayMon.def = GetMonData(mon, MON_DATA_DEF);
            sStorage->displayMon.spatk = GetMonData(mon, MON_DATA_SPATK);
            sStorage->displayMon.spdef = GetMonData(mon, MON_DATA_SPDEF);
            sStorage->displayMon.speed = GetMonData(mon, MON_DATA_SPEED);

            sStorage->displayMon.ability = GetMonAbility(mon);
        }
    }
    else if (mode == MODE_BOX)
    {
        struct BoxPokemon *boxMon = (struct BoxPokemon *)pokemon;
        struct Pokemon tempMon;

        sStorage->displayMon.species = GetBoxMonData(pokemon, MON_DATA_SPECIES);
        if (sStorage->displayMon.species != SPECIES_NONE)
        {
            bool32 isShiny = GetBoxMonData(boxMon, MON_DATA_IS_SHINY);
			u8 pokerusDays = GetBoxMonData(boxMon, MON_DATA_POKERUS_DAYS_LEFT);
			bool32 hasActivePokerus = (pokerusDays > 0);
            sanityIsBadEgg = GetBoxMonData(boxMon, MON_DATA_SANITY_IS_BAD_EGG);
            if (sanityIsBadEgg)
                sStorage->displayMon.isEgg = TRUE;
            else
                sStorage->displayMon.isEgg = GetBoxMonData(boxMon, MON_DATA_IS_EGG);


            GetBoxMonData(boxMon, MON_DATA_NICKNAME, sStorage->displayMon.nickname);
            StringGet_Nickname(sStorage->displayMon.nickname);
            sStorage->displayMon.level = GetLevelFromBoxMonExp(boxMon);
            sStorage->displayMon.markings = GetBoxMonData(boxMon, MON_DATA_MARKINGS);
            sStorage->displayMon.personality = GetBoxMonData(boxMon, MON_DATA_PERSONALITY);
            sStorage->displayMon.isEgg = GetBoxMonData(boxMon, MON_DATA_IS_EGG);
            sStorage->displayMon.palette = GetMonSpritePalFromSpeciesAndPersonalityIsEgg(sStorage->displayMon.species, isShiny, sStorage->displayMon.personality,
            sStorage->displayMon.isEgg);
            sStorage->displayMon.isShiny = isShiny;
			sStorage->displayMon.hasPokerus = hasActivePokerus;
            gender = GetGenderFromSpeciesAndPersonality(sStorage->displayMon.species, sStorage->displayMon.personality);
            sStorage->displayMon.gender = gender;
            sStorage->displayMon.heldItem = GetBoxMonData(boxMon, MON_DATA_HELD_ITEM);

            sStorage->displayMon.types[0] = gSpeciesInfo[sStorage->displayMon.species].types[0];
            sStorage->displayMon.types[1] = gSpeciesInfo[sStorage->displayMon.species].types[1];

            // Convert BoxPokemon to Pokemon to get stats
            BoxMonToMon(boxMon, &tempMon);
            sStorage->displayMon.maxHP = GetMonData(&tempMon, MON_DATA_MAX_HP);
            sStorage->displayMon.atk = GetMonData(&tempMon, MON_DATA_ATK);
            sStorage->displayMon.def = GetMonData(&tempMon, MON_DATA_DEF);
            sStorage->displayMon.spatk = GetMonData(&tempMon, MON_DATA_SPATK);
            sStorage->displayMon.spdef = GetMonData(&tempMon, MON_DATA_SPDEF);
            sStorage->displayMon.speed = GetMonData(&tempMon, MON_DATA_SPEED);

            u8 abilityNum = GetBoxMonData(boxMon, MON_DATA_ABILITY_NUM);
            sStorage->displayMon.ability = GetAbilityBySpecies(sStorage->displayMon.species, abilityNum);
        }
    }
    else
    {
        sStorage->displayMon.species = SPECIES_NONE;
        sStorage->displayMon.heldItem = ITEM_NONE;
    }

    if (sStorage->displayMon.species == SPECIES_NONE)
    {
        StringFill(sStorage->displayMon.nickname, CHAR_SPACE, 5);
        StringFill(sStorage->displayMon.nicknameText, CHAR_SPACE, 8);
        StringFill(sStorage->displayMon.speciesNameText, CHAR_SPACE, 8);
        StringFill(sStorage->displayMon.genderLvlText, CHAR_SPACE, 8);
        StringFill(sStorage->displayMon.itemNameText, CHAR_SPACE, 8);
    }
    else if (sStorage->displayMon.isEgg)
    {
        if (sanityIsBadEgg)
            StringCopyPadded(sStorage->displayMon.nicknameText, sStorage->displayMon.nickname, CHAR_SPACE, 5);
        else
            StringCopyPadded(sStorage->displayMon.nicknameText, gText_EggNickname, CHAR_SPACE, 8);

        StringFill(sStorage->displayMon.speciesNameText, CHAR_SPACE, 8);
        StringFill(sStorage->displayMon.genderLvlText, CHAR_SPACE, 8);
        StringFill(sStorage->displayMon.itemNameText, CHAR_SPACE, 8);
    }
    else
    {
        if (sStorage->displayMon.species == SPECIES_NIDORAN_F || sStorage->displayMon.species == SPECIES_NIDORAN_M)
            gender = MON_GENDERLESS;

        StringCopyPadded(sStorage->displayMon.nicknameText, sStorage->displayMon.nickname, CHAR_SPACE, 5);

        txtPtr = sStorage->displayMon.speciesNameText;
        *(txtPtr++) = CHAR_SLASH;
        StringCopyPadded(txtPtr, GetSpeciesName(sStorage->displayMon.species), CHAR_SPACE, 5);

        txtPtr = sStorage->displayMon.genderLvlText;
        *(txtPtr++) = EXT_CTRL_CODE_BEGIN;
        *(txtPtr++) = EXT_CTRL_CODE_COLOR_HIGHLIGHT_SHADOW;
        *(txtPtr++) = TEXT_COLOR_WHITE;
        *(txtPtr++) = EXT_CTRL_CODE_BEGIN;
        *(txtPtr++) = EXT_CTRL_CODE_TEXT_COLORS;
        switch (gender)
        {
        case MON_MALE:
            *(txtPtr++) = TEXT_COLOR_RED;
            *(txtPtr++) = TEXT_COLOR_LIGHT_RED;
            *(txtPtr++) = TEXT_COLOR_WHITE;
            *(txtPtr++) = CHAR_MALE;
            break;
        case MON_FEMALE:
            *(txtPtr++) = TEXT_COLOR_GREEN;
            *(txtPtr++) = TEXT_COLOR_LIGHT_GREEN;
            *(txtPtr++) = TEXT_COLOR_WHITE;
            *(txtPtr++) = CHAR_FEMALE;
            break;
        default:
            *(txtPtr++) = TEXT_COLOR_DARK_GRAY;
            *(txtPtr++) = TEXT_COLOR_LIGHT_GRAY;
            *(txtPtr++) = TEXT_COLOR_WHITE;
            *(txtPtr++) = CHAR_SPACER; // Genderless
            break;
        }

        *(txtPtr++) = EXT_CTRL_CODE_BEGIN;
        *(txtPtr++) = EXT_CTRL_CODE_TEXT_COLORS;
        *(txtPtr++) = TEXT_COLOR_DARK_GRAY;
        *(txtPtr++) = TEXT_COLOR_LIGHT_GRAY;
        *(txtPtr++) = TEXT_COLOR_WHITE;
        *(txtPtr++) = CHAR_SPACE;
        *(txtPtr++) = CHAR_EXTRA_SYMBOL;
        *(txtPtr++) = CHAR_LV_2;

        txtPtr = ConvertIntToDecimalStringN(txtPtr, sStorage->displayMon.level, STR_CONV_MODE_LEFT_ALIGN, 3);
        txtPtr[0] = CHAR_SPACE;
        txtPtr[1] = EOS;

        if (sStorage->displayMon.heldItem != ITEM_NONE)
            StringCopyPadded(sStorage->displayMon.itemNameText, GetItemName(sStorage->displayMon.heldItem), CHAR_SPACE, 8);
        else
            StringFill(sStorage->displayMon.itemNameText, CHAR_SPACE, 8);
    }
}


//------------------------------------------------------------------------------
//  SECTION: Input handlers
//
//  The functions below process context-dependent input
//------------------------------------------------------------------------------


static u8 HandleInput_InBox(void)
{
    switch (sStorage->inBoxMovingMode)
    {
    case MOVE_MODE_NORMAL:
    default:
        return InBoxInput_Normal();
    case MOVE_MODE_MULTIPLE_SELECTING:
        return InBoxInput_SelectingMultiple();
    case MOVE_MODE_MULTIPLE_MOVING:
        return InBoxInput_MovingMultiple();
    }
}

static u8 InBoxInput_Normal(void)
{
    u8 retVal;
    s8 cursorArea;
    s8 cursorPosition;

    do
    {
        cursorArea = sCursorArea;
        cursorPosition = sCursorPosition;
        sStorage->cursorVerticalWrap = 0;
        sStorage->cursorHorizontalWrap = 0;
        sStorage->cursorFlipTimer = 0;

        if (JOY_REPEAT(DPAD_UP))
        {
            retVal = INPUT_MOVE_CURSOR;
            if (sCursorPosition >= IN_BOX_COLUMNS)
            {
                cursorPosition -= IN_BOX_COLUMNS;
            }
            else
            {
                sLastBoxColumn = sCursorPosition % IN_BOX_COLUMNS;
                cursorArea = CURSOR_AREA_BOX_TITLE;
                cursorPosition = 0;
            }
            break;
        }
        else if (JOY_REPEAT(DPAD_DOWN))
        {
            retVal = INPUT_MOVE_CURSOR;
            cursorPosition += IN_BOX_COLUMNS;
            if (cursorPosition >= IN_BOX_COUNT)
            {
                sLastBoxColumn = sCursorPosition % IN_BOX_COLUMNS;
                cursorArea = CURSOR_AREA_BOX_TITLE;
                cursorPosition = 0;
                sStorage->cursorVerticalWrap = 1;
            }
            break;
        }
        else if (JOY_REPEAT(DPAD_LEFT))
        {
            retVal = INPUT_MOVE_CURSOR;
            if (sCursorPosition % IN_BOX_COLUMNS != 0)
            {
                cursorPosition--;
            }
            else
            {
                if (sStorage->boxOption == OPTION_WITHDRAW)
                {
                    u8 row = sCursorPosition / IN_BOX_COLUMNS;
                    cursorPosition = row * IN_BOX_COLUMNS + (IN_BOX_COLUMNS - 1);
                    sStorage->cursorHorizontalWrap = 1;
                    break;
                }
                // Go to party
                cursorArea = CURSOR_AREA_IN_PARTY;
                if (sCursorPosition / IN_BOX_COLUMNS == 0)
                    cursorPosition = 1; // Row 0 -> Party 1
                else
                    cursorPosition = (sCursorPosition / IN_BOX_COLUMNS) + 1;

                if (cursorPosition >= PARTY_SIZE)
                    cursorPosition = PARTY_SIZE - 1;
            }
            break;
        }
        else if (JOY_REPEAT(DPAD_RIGHT))
        {
            retVal = INPUT_MOVE_CURSOR;
            if ((sCursorPosition + 1) % IN_BOX_COLUMNS != 0)
            {
                cursorPosition++;
            }
            else
            {
                if (sStorage->boxOption == OPTION_WITHDRAW)
                {
                    u8 row = sCursorPosition / IN_BOX_COLUMNS;
                    cursorPosition = row * IN_BOX_COLUMNS;
                    sStorage->cursorHorizontalWrap = 1;
                    break;
                }
                // Wraparound to party
                cursorArea = CURSOR_AREA_IN_PARTY;
                if (sCursorPosition / IN_BOX_COLUMNS == 0)
                    cursorPosition = 1; // Row 0 -> Party 1
                else
                    cursorPosition = (sCursorPosition / IN_BOX_COLUMNS) + 1;

                if (cursorPosition >= PARTY_SIZE)
                    cursorPosition = PARTY_SIZE - 1;
            }
            break;
        }

        if ((JOY_NEW(A_BUTTON)) && SetSelectionMenuTexts())
        {
            if (sCursorMode == CURSOR_MODE_NORMAL)
                return INPUT_IN_MENU;

            if (sCursorMode == CURSOR_MODE_AUTO_ACTION)
            {
                switch (GetMenuItemTextId(0))
                {
                case MENU_STORE:
                    return INPUT_DEPOSIT;
                case MENU_WITHDRAW:
                    return INPUT_WITHDRAW;
                case MENU_MOVE:
                    return INPUT_MOVE_MON;
                case MENU_SHIFT:
                    return INPUT_SHIFT_MON;
                case MENU_PLACE:
                    return INPUT_PLACE_MON;
                case MENU_TAKE:
                    return INPUT_TAKE_ITEM;
                case MENU_GIVE:
                    return INPUT_GIVE_ITEM;
                case MENU_SWITCH:
                    return INPUT_SWITCH_ITEMS;
                case MENU_SELECT:
                    return INPUT_SELECT_MON;
                }
            }

            if (sCursorMode == CURSOR_MODE_MULTI_MOVE)
            {
                if (sStorage->boxOption == OPTION_MOVE_MONS && !sIsMonBeingMoved)
                {
                    sStorage->inBoxMovingMode = MOVE_MODE_MULTIPLE_SELECTING;
                    return INPUT_MULTIMOVE_START;
                }
                else
                {
                    switch (GetMenuItemTextId(0))
                    {
                    case MENU_STORE:
                        return INPUT_DEPOSIT;
                    case MENU_WITHDRAW:
                        return INPUT_WITHDRAW;
                    case MENU_MOVE:
                        return INPUT_MOVE_MON;
                    case MENU_SHIFT:
                        return INPUT_SHIFT_MON;
                    case MENU_PLACE:
                        return INPUT_PLACE_MON;
                    case MENU_TAKE:
                        return INPUT_TAKE_ITEM;
                    case MENU_GIVE:
                        return INPUT_GIVE_ITEM;
                    case MENU_SWITCH:
                        return INPUT_SWITCH_ITEMS;
                    case MENU_SELECT:
                        return INPUT_SELECT_MON;
                    }
                }
            }
        }

        if (JOY_NEW(B_BUTTON))
        {
            if (sStorage->showMonInfo)
            {
                PlaySE(SE_SELECT);
                sStorage->showMonInfo = FALSE;
                TryRefreshDisplayMon();
                UpdateMonInfoTilemap();
                return INPUT_NONE;
            }
            return INPUT_PRESSED_B;
        }

        if (gSaveBlock2Ptr->optionsButtonMode == OPTIONS_BUTTON_MODE_LR)
        {
            if (JOY_HELD(L_BUTTON))
                return INPUT_SCROLL_LEFT;
            if (JOY_HELD(R_BUTTON))
                return INPUT_SCROLL_RIGHT;
        }

        if (JOY_NEW(SELECT_BUTTON))
        {
            ToggleCursorAutoAction();
            return INPUT_NONE;
        }

        retVal = INPUT_NONE;

    } while (0);

    if (retVal)
        SetCursorPosition(cursorArea, cursorPosition);

    return retVal;
}

static u8 InBoxInput_SelectingMultiple(void)
{
    if (JOY_HELD(A_BUTTON))
    {
        if (JOY_REPEAT(DPAD_UP))
        {
            if (sCursorPosition / IN_BOX_COLUMNS != 0)
            {
                SetCursorPosition(CURSOR_AREA_IN_BOX, sCursorPosition - IN_BOX_COLUMNS);
                return INPUT_MULTIMOVE_CHANGE_SELECTION;
            }
            else
            {
                return INPUT_MULTIMOVE_UNABLE;
            }
        }
        else if (JOY_REPEAT(DPAD_DOWN))
        {
            if (sCursorPosition + IN_BOX_COLUMNS < IN_BOX_COUNT)
            {
                SetCursorPosition(CURSOR_AREA_IN_BOX, sCursorPosition + IN_BOX_COLUMNS);
                return INPUT_MULTIMOVE_CHANGE_SELECTION;
            }
            else
            {
                return INPUT_MULTIMOVE_UNABLE;
            }
        }
        else if (JOY_REPEAT(DPAD_LEFT))
        {
            if (sCursorPosition % IN_BOX_COLUMNS != 0)
            {
                SetCursorPosition(CURSOR_AREA_IN_BOX, sCursorPosition - 1);
                return INPUT_MULTIMOVE_CHANGE_SELECTION;
            }
            else
            {
                return INPUT_MULTIMOVE_UNABLE;
            }
        }
        else if (JOY_REPEAT(DPAD_RIGHT))
        {
            if ((sCursorPosition + 1) % IN_BOX_COLUMNS != 0)
            {
                SetCursorPosition(CURSOR_AREA_IN_BOX, sCursorPosition + 1);
                return INPUT_MULTIMOVE_CHANGE_SELECTION;
            }
            else
            {
                return INPUT_MULTIMOVE_UNABLE;
            }
        }
        else
        {
            return INPUT_NONE;
        }
    }
    else
    {
        if (MultiMove_GetOrigin() == sCursorPosition)
        {
            // Doing a multiple mon selection but only chose 1 mon
            sStorage->inBoxMovingMode = MOVE_MODE_NORMAL;
            return INPUT_MULTIMOVE_SINGLE;
        }
        else
        {
            sIsMonBeingMoved = (sStorage->displayMon.species != SPECIES_NONE);
            sStorage->inBoxMovingMode = MOVE_MODE_MULTIPLE_MOVING;
            sMovingMonOrigBoxId = StorageGetCurrentBox();
            return INPUT_MULTIMOVE_GRAB_SELECTION;
        }
    }
}

static u8 InBoxInput_MovingMultiple(void)
{
    if (JOY_REPEAT(DPAD_UP))
    {
        if (MultiMove_TryMoveGroup(0))
        {
            SetCursorPosition(CURSOR_AREA_IN_BOX, sCursorPosition - IN_BOX_COLUMNS);
            return INPUT_MULTIMOVE_MOVE_MONS;
        }
        else
        {
            return INPUT_MULTIMOVE_UNABLE;
        }
    }
    else if (JOY_REPEAT(DPAD_DOWN))
    {
        if (MultiMove_TryMoveGroup(1))
        {
            SetCursorPosition(CURSOR_AREA_IN_BOX, sCursorPosition + IN_BOX_COLUMNS);
            return INPUT_MULTIMOVE_MOVE_MONS;
        }
        else
        {
            return INPUT_MULTIMOVE_UNABLE;
        }
    }
    else if (JOY_REPEAT(DPAD_LEFT))
    {
        if (MultiMove_TryMoveGroup(2))
        {
            SetCursorPosition(CURSOR_AREA_IN_BOX, sCursorPosition - 1);
            return INPUT_MULTIMOVE_MOVE_MONS;
        }
        else
        {
            return INPUT_SCROLL_LEFT;
        }
    }
    else if (JOY_REPEAT(DPAD_RIGHT))
    {
        if (MultiMove_TryMoveGroup(3))
        {
            SetCursorPosition(CURSOR_AREA_IN_BOX, sCursorPosition + 1);
            return INPUT_MULTIMOVE_MOVE_MONS;
        }
        else
        {
            return INPUT_SCROLL_RIGHT;
        }
    }
    else if (JOY_NEW(A_BUTTON))
    {
        if (MultiMove_CanPlaceSelection())
        {
            sIsMonBeingMoved = FALSE;
            sStorage->inBoxMovingMode = MOVE_MODE_NORMAL;
            return INPUT_MULTIMOVE_PLACE_MONS;
        }
        else
        {
            return INPUT_MULTIMOVE_UNABLE;
        }
    }
    else if (JOY_NEW(B_BUTTON))
    {
        return INPUT_MULTIMOVE_UNABLE;
    }
    else
    {
        if (gSaveBlock2Ptr->optionsButtonMode == OPTIONS_BUTTON_MODE_LR)
        {
            if (JOY_HELD(L_BUTTON))
                return INPUT_SCROLL_LEFT;
            if (JOY_HELD(R_BUTTON))
                return INPUT_SCROLL_RIGHT;
        }

        return INPUT_NONE;
    }
}

static u8 HandleInput_InParty(void)
{
    u8 retVal;
    s8 cursorArea;
    s8 cursorPosition;

    do
    {
        cursorArea = sCursorArea;
        cursorPosition = sCursorPosition;
        sStorage->cursorHorizontalWrap = 0;
        sStorage->cursorVerticalWrap = 0;
        sStorage->cursorFlipTimer = 0;
        retVal = INPUT_NONE;

        if (JOY_REPEAT(DPAD_UP))
        {
            if (--cursorPosition < 0)
                cursorPosition = PARTY_SIZE - 1;
            if (cursorPosition != sCursorPosition)
                retVal = INPUT_MOVE_CURSOR;
            break;
        }
        else if (JOY_REPEAT(DPAD_DOWN))
        {
            if (++cursorPosition > PARTY_SIZE - 1)
                cursorPosition = 0;
            if (cursorPosition != sCursorPosition)
                retVal = INPUT_MOVE_CURSOR;
            break;
        }
        else if (JOY_REPEAT(DPAD_LEFT))
        {
            if (sStorage->boxOption == OPTION_DEPOSIT)
                break;

            retVal = INPUT_MOVE_CURSOR;
            cursorArea = CURSOR_AREA_IN_BOX;

            // Map party slot to box row
            if (sCursorPosition <= 1)
                cursorPosition = 0; // Party 0/1 -> Row 0
            else
                cursorPosition = (sCursorPosition - 1) * IN_BOX_COLUMNS;

            // Wraparound to rightmost column
            cursorPosition += (IN_BOX_COLUMNS - 1);

            if (cursorPosition >= IN_BOX_COUNT)
                cursorPosition = IN_BOX_COUNT - 1;
            break;
        }
        else if (JOY_REPEAT(DPAD_RIGHT))
        {
            if (sStorage->boxOption == OPTION_DEPOSIT)
                break;

            retVal = INPUT_MOVE_CURSOR;
            cursorArea = CURSOR_AREA_IN_BOX;

            // Map party slot to box row
            if (sCursorPosition <= 1)
                cursorPosition = 0; // Party 0/1 -> Row 0
            else
                cursorPosition = (sCursorPosition - 1) * IN_BOX_COLUMNS;

            if (cursorPosition >= IN_BOX_COUNT)
                cursorPosition = IN_BOX_COUNT - IN_BOX_COLUMNS;
            break;
        }

        if (JOY_NEW(A_BUTTON))
        {
            if (SetSelectionMenuTexts())
            {
                if (sCursorMode == CURSOR_MODE_NORMAL)
                    return INPUT_IN_MENU;

                switch (GetMenuItemTextId(0))
                {
                case MENU_STORE:
                    return INPUT_DEPOSIT;
                case MENU_WITHDRAW:
                    return INPUT_WITHDRAW;
                case MENU_MOVE:
                    return INPUT_MOVE_MON;
                case MENU_SHIFT:
                    return INPUT_SHIFT_MON;
                case MENU_PLACE:
                    return INPUT_PLACE_MON;
                case MENU_TAKE:
                    return INPUT_TAKE_ITEM;
                case MENU_GIVE:
                    return INPUT_GIVE_ITEM;
                case MENU_SWITCH:
                    return INPUT_SWITCH_ITEMS;
                }
            }
        }

        if (JOY_NEW(B_BUTTON))
        {
            if (sStorage->showMonInfo)
            {
                PlaySE(SE_SELECT);
                sStorage->showMonInfo = FALSE;
                TryRefreshDisplayMon();
                UpdateMonInfoTilemap();
                return INPUT_NONE;
            }
            return INPUT_PRESSED_B;
        }

        if (gSaveBlock2Ptr->optionsButtonMode == OPTIONS_BUTTON_MODE_LR)
        {
            if (JOY_HELD(L_BUTTON))
                return INPUT_SCROLL_LEFT;
            if (JOY_HELD(R_BUTTON))
                return INPUT_SCROLL_RIGHT;
        }

        if (JOY_NEW(SELECT_BUTTON))
        {
            ToggleCursorAutoAction();
            return INPUT_NONE;
        }

    } while (0);

    if (retVal != INPUT_NONE)
    {
        if (retVal != INPUT_HIDE_PARTY)
            SetCursorPosition(cursorArea, cursorPosition);
    }

    return retVal;
}

static u8 HandleInput_OnBox(void)
{
    u8 retVal;
    s8 cursorArea;
    s8 cursorPosition;

    do
    {
        sStorage->cursorHorizontalWrap = 0;
        sStorage->cursorVerticalWrap = 0;
        sStorage->cursorFlipTimer = 0;

        if (JOY_REPEAT(DPAD_UP))
        {
            retVal = INPUT_MOVE_CURSOR;
            cursorArea = CURSOR_AREA_IN_BOX;
            cursorPosition = IN_BOX_COUNT - IN_BOX_COLUMNS + sLastBoxColumn;
            sStorage->cursorVerticalWrap = -1;
            break;
        }
        else if (JOY_REPEAT(DPAD_DOWN))
        {
            retVal = INPUT_MOVE_CURSOR;
            cursorArea = CURSOR_AREA_IN_BOX;
            cursorPosition = sLastBoxColumn;
            break;
        }

        if (JOY_HELD(DPAD_LEFT))
            return INPUT_SCROLL_LEFT;
        if (JOY_HELD(DPAD_RIGHT))
            return INPUT_SCROLL_RIGHT;

        if (gSaveBlock2Ptr->optionsButtonMode == OPTIONS_BUTTON_MODE_LR)
        {
            if (JOY_HELD(L_BUTTON))
                return INPUT_SCROLL_LEFT;
            if (JOY_HELD(R_BUTTON))
                return INPUT_SCROLL_RIGHT;
        }

        if (JOY_NEW(A_BUTTON))
        {
            AddBoxOptionsMenu();
            return INPUT_BOX_OPTIONS;
        }

        if (JOY_NEW(B_BUTTON))
        {
            if (sStorage->showMonInfo)
            {
                PlaySE(SE_SELECT);
                sStorage->showMonInfo = FALSE;
                TryRefreshDisplayMon();
                UpdateMonInfoTilemap();
                return INPUT_NONE;
            }
            return INPUT_PRESSED_B;
        }

        if (JOY_NEW(SELECT_BUTTON))
        {
            ToggleCursorAutoAction();
            return INPUT_NONE;
        }

        retVal = INPUT_NONE;

    } while (0);

    if (retVal != INPUT_NONE)
    {
        SetCursorPosition(cursorArea, cursorPosition);
    }

    return retVal;
}

static u8 HandleInput(void)
{
    if (JOY_NEW(START_BUTTON) && sCursorMode != CURSOR_MODE_MULTI_MOVE)
        return INPUT_TOGGLE_MON_INFO;
    struct
    {
        u8 (*func)(void);
        s8 area;
    } static const inputFuncs[] =
    {
        {HandleInput_InBox,     CURSOR_AREA_IN_BOX},
        {HandleInput_InParty,   CURSOR_AREA_IN_PARTY},
        {HandleInput_OnBox,     CURSOR_AREA_BOX_TITLE},
        {NULL, 0},
    };

    u16 i = 0;
    while (inputFuncs[i].func != NULL)
    {
        if (inputFuncs[i].area == sCursorArea)
            return inputFuncs[i].func();
        i++;
    }

    return INPUT_NONE;
}

static void AddBoxOptionsMenu(void)
{
    InitMenu();
    SetMenuText(MENU_JUMP);
    SetMenuText(MENU_WALLPAPER);
    SetMenuText(MENU_NAME);
}

static u8 SetSelectionMenuTexts(void)
{
    InitMenu();
    if (sStorage->boxOption != OPTION_MOVE_ITEMS)
        return SetMenuTexts_Mon();
    else
        return SetMenuTexts_Item();
}

static bool8 SetMenuTexts_Mon(void)
{
    enum Species species = GetSpeciesAtCursorPosition();

    switch (sStorage->boxOption)
    {
    case OPTION_DEPOSIT:
        if (species != SPECIES_NONE)
            SetMenuText(MENU_STORE);
        else
            return FALSE;
        break;
    case OPTION_WITHDRAW:
        if (species != SPECIES_NONE)
            SetMenuText(MENU_WITHDRAW);
        else
            return FALSE;
        break;
    case OPTION_MOVE_MONS:
        if (sIsMonBeingMoved)
        {
            if (species != SPECIES_NONE)
                SetMenuText(MENU_SHIFT);
            else
                SetMenuText(MENU_PLACE);
        }
        else
        {
            if (species != SPECIES_NONE)
                SetMenuText(MENU_MOVE);
            else
                return FALSE;
        }
        break;
    case OPTION_SELECT_MON:
        if (species != SPECIES_NONE && CanBoxMonBeSelected(GetCursorBoxMon()))
            SetMenuText(MENU_SELECT);
        else
            return FALSE;
        break;
    case OPTION_MOVE_ITEMS:
    default:
        return FALSE;
    }

    SetMenuText(MENU_SUMMARY);
    if (sStorage->boxOption == OPTION_MOVE_MONS)
    {
        if (sCursorArea == CURSOR_AREA_IN_BOX)
            SetMenuText(MENU_WITHDRAW);
        else
            SetMenuText(MENU_STORE);
    }

    SetMenuText(MENU_MARK);
    if (sStorage->boxOption != OPTION_SELECT_MON)
        SetMenuText(MENU_RELEASE);
    return TRUE;
}

static bool8 SetMenuTexts_Item(void)
{
    if (sStorage->displayMon.species == SPECIES_EGG)
        return FALSE;

    if (!IsMovingItem())
    {
        if (sStorage->displayMon.heldItem == ITEM_NONE)
        {
            if (sStorage->displayMon.species == SPECIES_NONE)
                return FALSE;

            SetMenuText(MENU_GIVE_2);
        }
        else
        {
            if (!ItemIsMail(sStorage->displayMon.heldItem))
            {
                SetMenuText(MENU_TAKE);
                SetMenuText(MENU_BAG);
            }
        }
    }
    else
    {
        if (sStorage->displayMon.heldItem == ITEM_NONE)
        {
            if (sStorage->displayMon.species == SPECIES_NONE)
                return FALSE;

            SetMenuText(MENU_GIVE);
        }
        else
        {
            if (ItemIsMail(sStorage->displayMon.heldItem) == TRUE)
                return FALSE;

            SetMenuText(MENU_SWITCH);
        }
    }

    return TRUE;
}


//------------------------------------------------------------------------------
//  SECTION: Cursor
//
//  The functions below handle a few of the generic cursor features.
//------------------------------------------------------------------------------


static void LoadHgssStorageCursorGfx(void)
{
    const u8 *srcTiles = (const u8 *)sHgssStorageCursor_Gfx;
    u8 *dstTiles = (u8 *)sHgssStorageCursorTiles;
    struct SpriteSheet spriteSheet =
    {
        .data = dstTiles,
        .size = sizeof(sHgssStorageCursorTiles),
        .tag = GFXTAG_CURSOR,
    };
    struct SpritePalette spritePalette =
    {
        .data = sHgssStorageCursor_Pal,
        .tag = PALTAG_HGSS_STORAGE_CURSOR,
    };
    u8 frame;
    u8 row;
    u8 col;

    // verified/cursor.png is two horizontal 32x32 frames (64x32 total).
    // gbagfx stores tiles row-major across the full image, while a 32x32 OBJ
    // frame needs its 16 tiles contiguous. Repack only tile order; every visible
    // HGSS pixel and palette index remains unchanged.
    CpuFill32(0, dstTiles, sizeof(sHgssStorageCursorTiles));
    for (frame = 0; frame < 2; frame++)
    {
        for (row = 0; row < 4; row++)
        {
            for (col = 0; col < 4; col++)
            {
                u32 srcTile = row * 8 + frame * 4 + col;
                u32 dstTile = frame * 16 + row * 4 + col;

                CpuCopy32(
                    srcTiles + srcTile * TILE_SIZE_4BPP,
                    dstTiles + dstTile * TILE_SIZE_4BPP,
                    TILE_SIZE_4BPP);
            }
        }
    }

    FreeSpriteTilesByTag(GFXTAG_CURSOR);
    FreeSpritePaletteByTag(PALTAG_HGSS_STORAGE_CURSOR);
    LoadSpriteSheet(&spriteSheet);
    LoadSpritePalette(&spritePalette);
}

static void CreateCursorSprites(void)
{
    u16 x, y;
    u8 spriteId;

    sStorage->cursorPalNums[CURSOR_MODE_NORMAL]       = IndexOfSpritePaletteTag(PALTAG_HGSS_STORAGE_CURSOR);
    sStorage->cursorPalNums[CURSOR_MODE_AUTO_ACTION] = IndexOfSpritePaletteTag(PALTAG_HGSS_STORAGE_CURSOR);
    sStorage->cursorPalNums[CURSOR_MODE_MULTI_MOVE]  = IndexOfSpritePaletteTag(PALTAG_HGSS_STORAGE_CURSOR);

    GetCursorCoordsByPos(sCursorArea, sCursorPosition, &x, &y);
    spriteId = CreateSprite(&sSpriteTemplate_Cursor, x, y, 2);
    if (spriteId != MAX_SPRITES)
    {
        sStorage->cursorSprite = &gSprites[spriteId];
        sStorage->cursorSprite->oam.paletteNum = sStorage->cursorPalNums[sCursorMode];
        sStorage->cursorSprite->oam.priority = 1;
        // NCER cells 13/14 have bounds x=-10..21, y=-6..25. The verified
        // export normalizes those bounds to a 32x32 frame, so offset only the
        // visual OBJ center (+6,+10) to keep the native origin on the existing
        // logical cursor coordinate. Held Pokemon/items still follow x/y.
        sStorage->cursorSprite->x2 = 6;
        sStorage->cursorSprite->y2 = 10;
        if (sIsMonBeingMoved)
            StartSpriteAnim(sStorage->cursorSprite, CURSOR_ANIM_MAIN);
    }
    else
    {
        sStorage->cursorSprite = NULL;
    }
}

//Tiles Array
static const u16 sModeIconTiles[3][4] = {
    [CURSOR_MODE_NORMAL]     = {27, 28, 43, 44},
    [CURSOR_MODE_AUTO_ACTION] = {29, 30, 45, 46},
    [CURSOR_MODE_MULTI_MOVE]  = {77, 78, 39, 31},
};

static void ToggleCursorAutoAction(void)
{
    if (sCursorMode == CURSOR_MODE_AUTO_ACTION)
    {
        ClearMonInfoPanel();
    }

    sCursorMode = (sCursorMode + 1) % (sStorage->boxOption == OPTION_MOVE_ITEMS ? 2 : 3);
    sStorage->cursorSprite->oam.paletteNum = sStorage->cursorPalNums[sCursorMode];

    if (sCursorMode == CURSOR_MODE_MULTI_MOVE)
    {
        sStorage->inMenuInteraction = TRUE;
        UpdateMonInfoTilemap();
    }
    else if (sCursorMode == CURSOR_MODE_NORMAL || sCursorMode == CURSOR_MODE_AUTO_ACTION)
    {
        if (sStorage->inBoxMovingMode != MOVE_MODE_NORMAL)
        {
            sStorage->inBoxMovingMode = MOVE_MODE_NORMAL;
            UpdateMonInfoTilemap();
        }
        if (sStorage->inMenuInteraction)
        {
            sStorage->inMenuInteraction = FALSE;
            UpdateMonInfoTilemap();
        }
    }
	
	u16 *tilemap = GetBgTilemapBuffer(1);
    u16 *modeIcons = (u16*)sModeIconTiles[sCursorMode];
	int pos = (0 * 32) + 24; 
	u16 paletteBits = (2 << 12);
	tilemap[pos]           = modeIcons[0] | paletteBits;
    tilemap[pos + 1]       = modeIcons[1] | paletteBits;
    tilemap[pos + 32]      = modeIcons[2] | paletteBits;
    tilemap[pos + 33]      = modeIcons[3] | paletteBits;
	ScheduleBgCopyTilemapToVram(1);
	
	PlaySE(SE_CLICK);
}

static u8 GetCursorPosition(void)
{
    return sCursorPosition;
}

static void GetCursorBoxColumnAndRow(u8 *column, u8 *row)
{
    if (sCursorArea == CURSOR_AREA_IN_BOX)
    {
        *column = sCursorPosition % IN_BOX_COLUMNS;
        *row = sCursorPosition / IN_BOX_COLUMNS;
    }
    else
    {
        *column = 0;
        *row = 0;
    }
}

static void StartCursorAnim(u8 animNum)
{
    StartSpriteAnim(sStorage->cursorSprite, animNum);
}

static void SetCursorPriorityTo1(void)
{
    sStorage->cursorSprite->oam.priority = 1;
}

static void TryHideItemAtCursor(void)
{
    if (sCursorArea == CURSOR_AREA_IN_BOX)
        TryHideItemIconAtPos(CURSOR_AREA_IN_BOX, sCursorPosition);
}

static void TryShowItemAtCursor(void)
{
    if (sCursorArea == CURSOR_AREA_IN_BOX)
        TryLoadItemIconAtPos(CURSOR_AREA_IN_BOX, sCursorPosition);
}


//------------------------------------------------------------------------------
//  SECTION: Menu
//
//  The functions below handle the generic options menu that comes up whenever
//  something in the PC is selected.
//------------------------------------------------------------------------------


static void InitMenu(void)
{
    sStorage->menuItemsCount = 0;
    sStorage->menuWidth = 0;
    sStorage->menuWindow.bg = 0;
    sStorage->menuWindow.paletteNum = 15;
    sStorage->menuWindow.baseBlock = 202;
}

static void SetMenuText(u8 textId)
{
    if (sStorage->menuItemsCount < ARRAY_COUNT(sStorage->menuItems))
    {
        u8 len;
        struct StorageMenu *menu = &sStorage->menuItems[sStorage->menuItemsCount];

        menu->text = sMenuTexts[textId];
        menu->textId = textId;
        len = GetStringWidth(FONT_NORMAL, menu->text, 0);
        if (len > sStorage->menuWidth)
            sStorage->menuWidth = len;

        sStorage->menuItemsCount++;
    }
}

static s8 GetMenuItemTextId(u8 menuIdx)
{
    if (menuIdx >= sStorage->menuItemsCount)
        return -1;
    else
        return sStorage->menuItems[menuIdx].textId;
}

static void GetMenuPosition(u8 cursorArea, u8 cursorPos, u8 *outLeft, u8 *outTop)
{
    static const u8 validTopPositions[] = {2, 5, 8, 11, 14, 17};
    u8 slotRow = 0;
    u8 menuHeight = 2 * sStorage->menuItemsCount;
    
    *outLeft = 9;
    *outTop = 2;
    
    if (cursorArea == CURSOR_AREA_BOX_TITLE)
    {
        *outLeft = 19;
        *outTop = 5;
        return;
    }
    else if (cursorArea == CURSOR_AREA_IN_PARTY)
    {
        slotRow = cursorPos;
    }
    else if (cursorArea == CURSOR_AREA_IN_BOX)
    {
        slotRow = (cursorPos / IN_BOX_COLUMNS) + 1;
        u8 col = cursorPos % IN_BOX_COLUMNS;
        
        if (col < 3)
            *outLeft = 15 + (col * 3);
        else
            *outLeft = (u8[]){16, 19, 22}[col - 3] - sStorage->menuWindow.width + 1;
    }
    
    u8 preferredTop = validTopPositions[min(slotRow, ARRAY_COUNT(validTopPositions) - 1)];
    
    if (preferredTop + menuHeight <= 19)
    {
        *outTop = preferredTop;
    }
    else
    {
        for (s8 i = ARRAY_COUNT(validTopPositions) - 1; i >= 0; i--)
        {
            if (validTopPositions[i] + menuHeight <= 19)
            {
                *outTop = validTopPositions[i];
                break;
            }
        }
    }
}

static void AddMenu(void)
{
    u8 tilemapLeft, tilemapTop;
    
    sStorage->menuWindow.width = (sStorage->menuWidth + 7) / 8 + 2;
    if (sStorage->menuWindow.width > 28)
        sStorage->menuWindow.width = 28;
    sStorage->menuWindow.height = 2 * sStorage->menuItemsCount;
    GetMenuPosition(sCursorArea, sCursorPosition, &tilemapLeft, &tilemapTop);

    // NSCR 86 terminates at the screen edge rather than using a right border.
    // Keep the existing vertical placement, but align the content window so the
    // authentic repeated interior reaches tile 29 exactly.
    tilemapLeft = 29 - sStorage->menuWindow.width;
    if (tilemapTop < 2)
        tilemapTop = 2;
    if (tilemapTop + sStorage->menuWindow.height + 2 > 20)
        tilemapTop = 18 - sStorage->menuWindow.height;

    sStorage->menuWindow.tilemapLeft = tilemapLeft;
    sStorage->menuWindow.tilemapTop = tilemapTop;
    sStorage->menuWindowId = AddWindow(&sStorage->menuWindow);
    ClearMonInfoTilemap();
    ClearWindowTilemap(sStorage->menuWindowId);
    DrawHgssContextMenuFrame(sStorage->menuWindowId);
    FillWindowPixelBuffer(sStorage->menuWindowId, PIXEL_FILL(1));
    PutWindowTilemap(sStorage->menuWindowId);
    PrintMenuTable(sStorage->menuWindowId, sStorage->menuItemsCount, (void *)sStorage->menuItems);
    InitMenuInUpperLeftCornerNormal(sStorage->menuWindowId, sStorage->menuItemsCount, 0);
    ScheduleBgCopyTilemapToVram(0);
}

// Called after AddMenu to determine whether or not the handler callback should
// wait to move on to the next state. Evidently there was no need to wait, and
// now it always returns FALSE
static bool8 IsMenuLoading(void)
{
    return FALSE;
}

static s16 HandleMenuInput(void)
{
    s32 input = MENU_NOTHING_CHOSEN;

    do
    {
        if (JOY_NEW(A_BUTTON))
        {
            input = Menu_GetCursorPos();
            break;
        }
        else if (JOY_NEW(B_BUTTON))
        {
            PlaySE(SE_SELECT);
            input = MENU_B_PRESSED;
        }

        if (JOY_NEW(DPAD_UP))
        {
            PlaySE(SE_SELECT);
            Menu_MoveCursor(-1);
        }
        else if (JOY_NEW(DPAD_DOWN))
        {
            PlaySE(SE_SELECT);
            Menu_MoveCursor(1);
        }
    } while (0);

    if (input != MENU_NOTHING_CHOSEN)
        RemoveMenu();

    if (input >= 0)
        input = sStorage->menuItems[input].textId;

    return input;
}

static void RemoveMenu(void)
{
    ClearHgssContextMenuFrame(sStorage->menuWindowId);
    ClearWindowTilemap(sStorage->menuWindowId);
    RemoveWindow(sStorage->menuWindowId);
    LoadPalette(sTextWindows_Pal, BG_PLTT_ID(15), sizeof(sTextWindows_Pal));
    ScheduleBgCopyTilemapToVram(0);
    UpdateMonInfoTilemap();
}


//------------------------------------------------------------------------------
//  SECTION: MultiMove
//
//  The functions below handle moving and selecting multiple Pokémon at once.
//  The icon sprites are moved to bg 0, and this bg is manipulated to move
//  them as a group.
//------------------------------------------------------------------------------

EWRAM_DATA static struct
{
    u8 funcId;
    u8 state;
    u8 fromColumn;
    u8 fromRow;
    u8 toColumn;
    u8 toRow;
    u8 cursorColumn;
    u8 cursorRow;
    u8 minColumn;
    u8 minRow;
    u8 columnsTotal;
    u8 rowsTotal;
    u16 bgX;
    u16 bgY;
    u16 bgMoveSteps;
    struct BoxPokemon boxMons[IN_BOX_COUNT];
} *sMultiMove = NULL;

static bool8 MultiMove_Init(void)
{
    sMultiMove = Alloc(sizeof(*sMultiMove));
    if (sMultiMove != NULL)
    {
        sStorage->multiMoveWindowId = AddWindow8Bit(&sWindowTemplate_MultiMove);
        if (sStorage->multiMoveWindowId != WINDOW_NONE)
        {
            FillWindowPixelBuffer(sStorage->multiMoveWindowId, PIXEL_FILL(0));
            return TRUE;
        }
    }

    return FALSE;
}

static void MultiMove_Free(void)
{
    if (sMultiMove != NULL)
        Free(sMultiMove);
}

static void MultiMove_SetFunction(u8 id)
{
    sMultiMove->funcId = id;
    sMultiMove->state = 0;
}

// Returns TRUE if the called function has more to do, FALSE otherwise
static bool8 MultiMove_RunFunction(void)
{
    switch (sMultiMove->funcId)
    {
    case MULTIMOVE_START:
        return MultiMove_Start();
    case MULTIMOVE_CANCEL:
        return MultiMove_Cancel();
    case MULTIMOVE_CHANGE_SELECTION:
        return MultiMove_ChangeSelection();
    case MULTIMOVE_GRAB_SELECTION:
        return MultiMove_GrabSelection();
    case MULTIMOVE_MOVE_MONS:
        return MultiMove_MoveMons();
    case MULTIMOVE_PLACE_MONS:
        return MultiMove_PlaceMons();
    }
    return FALSE;
}

#define MULTIMOVE_TINT_COLOR RGB(21, 31, 24) // RGB_WHITE default
#define MULTIMOVE_TINT_COEFF 8

static bool8 MultiMove_Start(void)
{
    switch (sMultiMove->state)
    {
    case 0:
        HideBg(0);
        ClearMonInfoTilemap();
        DmaClear16(3, (void *)(VRAM + 0x0000), 0x1000);
        sMultiMove->state++;
        break;
    case 1:
        TryLoadAllMonIconPalettesAtOffset(BG_PLTT_ID(8));
        sMultiMove->state++;
        break;
    case 2:
        GetCursorBoxColumnAndRow(&sMultiMove->fromColumn, &sMultiMove->fromRow);
        sMultiMove->toColumn = sMultiMove->fromColumn;
        sMultiMove->toRow = sMultiMove->fromRow;
        ChangeBgX(0, 1024, BG_COORD_SET);
        ChangeBgY(0, 0, BG_COORD_SET);
        FillBgTilemapBufferRect_Palette0(0, 0, 0, 0, 0x20, 0x20);
        FillWindowPixelBuffer8Bit(sStorage->multiMoveWindowId, PIXEL_FILL(0));
        MultiMove_SetIconToBg(sMultiMove->fromColumn, sMultiMove->fromRow);
        SetBgAttribute(0, BG_ATTR_PALETTEMODE, 1);
        SetBgAttribute(0, BG_ATTR_CHARBASEINDEX, 0);
        SetBgAttribute(0, BG_ATTR_SCREENSIZE, 0);
        PutWindowTilemap(sStorage->multiMoveWindowId);
        CopyWindowToVram8Bit(sStorage->multiMoveWindowId, COPYWIN_FULL);
        BlendPalettes(0x3F00, MULTIMOVE_TINT_COEFF, MULTIMOVE_TINT_COLOR);
        StartCursorAnim(CURSOR_ANIM_MAIN);
        sStorage->cursorSprite->oam.paletteNum = IndexOfSpritePaletteTag(PALTAG_HGSS_STORAGE_CURSOR);
        sMultiMove->state++;
        break;
    case 3:
        if (!IsDma3ManagerBusyWithBgCopy())
        {
            ShowBg(0);
            return FALSE;
        }
        break;
    }

    return TRUE;
}

static bool8 MultiMove_Cancel(void)
{
    switch (sMultiMove->state)
    {
    case 0:
        HideBg(0);
        sMultiMove->state++;
        break;
    case 1:
        MultiMove_ResetBg();
        StartCursorAnim(CURSOR_ANIM_BOUNCE);
        sStorage->cursorSprite->oam.paletteNum = sStorage->cursorPalNums[sCursorMode];
        sMultiMove->state++;
        break;
    case 2:
        if (!IsDma3ManagerBusyWithBgCopy())
        {
            SetCursorPriorityTo1();
            LoadPalette(GetTextWindowPalette(3), BG_PLTT_ID(13), PLTT_SIZE_4BPP);
            ShowBg(0);
            return FALSE;
        }
        break;
    }

    return TRUE;
}

static bool8 MultiMove_ChangeSelection(void)
{
    switch (sMultiMove->state)
    {
    case 0:
        if (!UpdateCursorPos())
        {
            GetCursorBoxColumnAndRow(&sMultiMove->cursorColumn, &sMultiMove->cursorRow);
            MultiMove_UpdateSelectedIcons();
            sMultiMove->toColumn = sMultiMove->cursorColumn;
            sMultiMove->toRow = sMultiMove->cursorRow;
            CopyWindowToVram8Bit(sStorage->multiMoveWindowId, COPYWIN_GFX);
            sMultiMove->state++;
        }
        break;
    case 1:
        return IsDma3ManagerBusyWithBgCopy();
    }

    return TRUE;
}

static bool8 MultiMove_GrabSelection(void)
{
    bool8 movingBg, movingMon;

    switch (sMultiMove->state)
    {
    case 0:
        MultiMove_GetMonsFromSelection();
        MultiMove_RemoveMonsFromBox();
        InitMultiMonPlaceChange(FALSE);
        sMultiMove->state++;
        break;
    case 1:
        if (!DoMonPlaceChange())
        {
            StartCursorAnim(CURSOR_ANIM_MAIN);
            MultiMove_InitMove(0, Q_8_8(1), 8);
            InitMultiMonPlaceChange(TRUE);
            sMultiMove->state++;
        }
        break;
    case 2:
        movingBg = MultiMove_UpdateMove();
        movingMon = DoMonPlaceChange();
        if (!movingBg && !movingMon)
            return FALSE; // Finished
        break;
    }

    return TRUE;
}

static bool8 MultiMove_MoveMons(void)
{
    bool8 movingCursor = UpdateCursorPos();
    bool8 movingBg = MultiMove_UpdateMove();

    if (!movingCursor && !movingBg)
        return FALSE;
    else
        return TRUE;
}

static bool8 MultiMove_PlaceMons(void)
{
    switch (sMultiMove->state)
    {
    case 0:
        MultiMove_SetPlacedMonData();
        MultiMove_InitMove(0, Q_8_8(-1), 8);
        InitMultiMonPlaceChange(FALSE);
        sMultiMove->state++;
        break;
    case 1:
        if (!DoMonPlaceChange() && !MultiMove_UpdateMove())
        {
            MultiMove_CreatePlacedMonIcons();
            StartCursorAnim(CURSOR_ANIM_MAIN);
            InitMultiMonPlaceChange(TRUE);
            HideBg(0);
            sMultiMove->state++;
        }
        break;
    case 2:
        if (!DoMonPlaceChange())
        {
            StartCursorAnim(CURSOR_ANIM_BOUNCE);
            sStorage->cursorSprite->oam.paletteNum = sStorage->cursorPalNums[sCursorMode];
            MultiMove_ResetBg();
            sMultiMove->state++;
        }
        break;
    case 3:
        if (!IsDma3ManagerBusyWithBgCopy())
        {
            LoadPalette(GetTextWindowPalette(3), BG_PLTT_ID(13), PLTT_SIZE_4BPP);
            SetCursorPriorityTo1();
            ShowBg(0);
            return FALSE;
        }
        break;
    }
    return TRUE;
}

// Returns TRUE if the movement was successful, FALSE otherwise
static bool8 MultiMove_TryMoveGroup(u8 dir)
{
    switch (dir)
    {
    case 0: // Up
        if (sMultiMove->minRow == 0)
            return FALSE;
        sMultiMove->minRow--;
        MultiMove_InitMove(0, Q_8_8(4), 6);
        break;
    case 1: // Down
        if (sMultiMove->minRow + sMultiMove->rowsTotal >= IN_BOX_ROWS)
            return FALSE;
        sMultiMove->minRow++;
        MultiMove_InitMove(0, Q_8_8(-4), 6);
        break;
    case 2: // Left
        if (sMultiMove->minColumn == 0)
            return FALSE;
        sMultiMove->minColumn--;
        MultiMove_InitMove(Q_8_8(4), 0, 6);
        break;
    case 3: // Right
        if (sMultiMove->minColumn + sMultiMove->columnsTotal >= IN_BOX_COLUMNS)
            return FALSE;
        sMultiMove->minColumn++;
        MultiMove_InitMove(Q_8_8(-4), 0, 6);
        break;
    }
    return TRUE;
}

static void MultiMove_UpdateSelectedIcons(void)
{
    s16 columnChange = (abs(sMultiMove->fromColumn - sMultiMove->cursorColumn)) - (abs(sMultiMove->fromColumn - sMultiMove->toColumn));
    s16 rowChange = (abs(sMultiMove->fromRow - sMultiMove->cursorRow)) - (abs(sMultiMove->fromRow - sMultiMove->toRow));

    if (columnChange > 0)
        MultiMove_SelectColumn(sMultiMove->cursorColumn, sMultiMove->fromRow, sMultiMove->toRow);

    if (columnChange < 0)
    {
        MultiMove_DeselectColumn(sMultiMove->toColumn, sMultiMove->fromRow, sMultiMove->toRow);
        MultiMove_SelectColumn(sMultiMove->cursorColumn, sMultiMove->fromRow, sMultiMove->toRow);
    }

    if (rowChange > 0)
        MultiMove_SelectRow(sMultiMove->cursorRow, sMultiMove->fromColumn, sMultiMove->toColumn);

    if (rowChange < 0)
    {
        MultiMove_DeselectRow(sMultiMove->toRow, sMultiMove->fromColumn, sMultiMove->toColumn);
        MultiMove_SelectRow(sMultiMove->cursorRow, sMultiMove->fromColumn, sMultiMove->toColumn);
    }
}

static void MultiMove_SelectColumn(u8 column, u8 minRow, u8 maxRow)
{
    if (minRow > maxRow)
    {
        u8 temp;
        SWAP(minRow, maxRow, temp);
    }

    while (minRow <= maxRow)
        MultiMove_SetIconToBg(column, minRow++);
}

static void MultiMove_SelectRow(u8 row, u8 minColumn, u8 maxColumn)
{
    if (minColumn > maxColumn)
    {
        u8 temp;
        SWAP(minColumn, maxColumn, temp);
    }

    while (minColumn <= maxColumn)
        MultiMove_SetIconToBg(minColumn++, row);
}

static void MultiMove_DeselectColumn(u8 column, u8 minRow, u8 maxRow)
{
    if (minRow > maxRow)
    {
        u8 temp;
        SWAP(minRow, maxRow, temp);
    }

    while (minRow <= maxRow)
        MultiMove_ClearIconFromBg(column, minRow++);
}

static void MultiMove_DeselectRow(u8 row, u8 minColumn, u8 maxColumn)
{
    if (minColumn > maxColumn)
    {
        u8 temp;
        SWAP(minColumn, maxColumn, temp);
    }

    while (minColumn <= maxColumn)
        MultiMove_ClearIconFromBg(minColumn++, row);
}

static void MultiMove_SetIconToBg(u8 x, u8 y)
{
    u8 position = x + (IN_BOX_COLUMNS * y);
    enum Species species = GetCurrentBoxMonData(position, MON_DATA_SPECIES);
    u32 personality = GetCurrentBoxMonData(position, MON_DATA_PERSONALITY);
    bool32 isEgg = GetCurrentBoxMonData(position, MON_DATA_IS_EGG);

    if (species != SPECIES_NONE)
    {
        const u8 *iconGfx = GetMonIconPtrIsEgg(species, personality, isEgg);
        u8 index = GetValidMonIconPalIndex(species) + 8;

        BlitBitmapRectToWindow4BitTo8Bit(sStorage->multiMoveWindowId,
                                         iconGfx,
                                         0,
                                         0,
                                         32,
                                         32,
                                         24 * x,
                                         24 * y,
                                         32,
                                         32,
                                         index);
    }
}

static void MultiMove_ClearIconFromBg(u8 x, u8 y)
{
    u8 position = x + (IN_BOX_COLUMNS * y);
    enum Species species = GetCurrentBoxMonData(position, MON_DATA_SPECIES_OR_EGG);

    if (species != SPECIES_NONE)
    {
        FillWindowPixelRect8Bit(sStorage->multiMoveWindowId,
                                PIXEL_FILL(0),
                                24 * x,
                                24 * y,
                                32,
                                32);
    }
}

static void MultiMove_InitMove(u16 x, u16 y, u16 moveSteps)
{
    sMultiMove->bgX = x;
    sMultiMove->bgY = y;
    sMultiMove->bgMoveSteps = moveSteps;
}

static u8 MultiMove_UpdateMove(void)
{
    if (sMultiMove->bgMoveSteps != 0)
    {
        ChangeBgX(0, sMultiMove->bgX, BG_COORD_ADD);
        ChangeBgY(0, sMultiMove->bgY, BG_COORD_ADD);
        sMultiMove->bgMoveSteps--;
    }

    return sMultiMove->bgMoveSteps;
}

// Store the Pokémon that the player is picking up
static void MultiMove_GetMonsFromSelection(void)
{
    s32 i, j;
    s32 columnCount, rowCount;
    u8 boxId;
    u8 monArrayId;

    sMultiMove->minColumn = min(sMultiMove->fromColumn, sMultiMove->toColumn);
    sMultiMove->minRow = min(sMultiMove->fromRow, sMultiMove->toRow);
    sMultiMove->columnsTotal = abs(sMultiMove->fromColumn - sMultiMove->toColumn) + 1;
    sMultiMove->rowsTotal = abs(sMultiMove->fromRow - sMultiMove->toRow) + 1;
    boxId = StorageGetCurrentBox();
    monArrayId = 0;
    columnCount = sMultiMove->minColumn + sMultiMove->columnsTotal;
    rowCount = sMultiMove->minRow + sMultiMove->rowsTotal;
    for (i = sMultiMove->minRow; i < rowCount; i++)
    {
        u8 boxPosition = (IN_BOX_COLUMNS * i) + sMultiMove->minColumn;
        for (j = sMultiMove->minColumn; j < columnCount; j++)
        {
            struct BoxPokemon *boxMon = GetBoxedMonPtr(boxId, boxPosition);
            // UB: possible null dereference
#ifdef UBFIX
            if (boxMon != NULL)
#endif
                sMultiMove->boxMons[monArrayId] = *boxMon;

            monArrayId++;
            boxPosition++;
        }
    }
}

// The Pokémon the player has picked up have been stored, now delete
// them from their original positions
static void MultiMove_RemoveMonsFromBox(void)
{
    s32 i, j;
    s32 columnCount = sMultiMove->minColumn + sMultiMove->columnsTotal;
    s32 rowCount = sMultiMove->minRow + sMultiMove->rowsTotal;
    u8 boxId = StorageGetCurrentBox();

    for (i = sMultiMove->minRow; i < rowCount; i++)
    {
        u8 boxPosition = (IN_BOX_COLUMNS * i) + sMultiMove->minColumn;
        for (j = sMultiMove->minColumn; j < columnCount; j++)
        {
            DestroyBoxMonIconAtPosition(boxPosition);
            ZeroBoxMonAt(boxId, boxPosition);
            boxPosition++;
        }
    }
}

static void MultiMove_CreatePlacedMonIcons(void)
{
    s32 i, j;
    s32 columnCount = sMultiMove->minColumn + sMultiMove->columnsTotal;
    s32 rowCount = sMultiMove->minRow + sMultiMove->rowsTotal;
    u8 monArrayId = 0;

    for (i = sMultiMove->minRow; i < rowCount; i++)
    {
        u8 boxPosition = (IN_BOX_COLUMNS * i) + sMultiMove->minColumn;
        for (j = sMultiMove->minColumn; j < columnCount; j++)
        {
            if (GetBoxMonData(&sMultiMove->boxMons[monArrayId], MON_DATA_SANITY_HAS_SPECIES))
                CreateBoxMonIconAtPos(boxPosition);
            monArrayId++;
            boxPosition++;
        }
    }
}

static void MultiMove_SetPlacedMonData(void)
{
    s32 i, j;
    s32 columnCount = sMultiMove->minColumn + sMultiMove->columnsTotal;
    s32 rowCount = sMultiMove->minRow + sMultiMove->rowsTotal;
    u8 boxId = StorageGetCurrentBox();
    u8 monArrayId = 0;

    for (i = sMultiMove->minRow; i < rowCount; i++)
    {
        u8 boxPosition = (IN_BOX_COLUMNS * i) + sMultiMove->minColumn;
        for (j = sMultiMove->minColumn; j < columnCount; j++)
        {
            if (OW_PC_HEAL <= GEN_7)
                HealBoxPokemon(&sMultiMove->boxMons[monArrayId]);
            if (GetBoxMonData(&sMultiMove->boxMons[monArrayId], MON_DATA_SANITY_HAS_SPECIES))
                SetBoxMonAt(boxId, boxPosition, &sMultiMove->boxMons[monArrayId]);
            boxPosition++;
            monArrayId++;
        }
    }
}

static void MultiMove_ResetBg(void)
{
    ChangeBgX(0, 0, BG_COORD_SET);
    ChangeBgY(0, 0, BG_COORD_SET);
    SetBgAttribute(0, BG_ATTR_PALETTEMODE, 0);
    SetBgAttribute(0, BG_ATTR_CHARBASEINDEX, 0);
    SetBgAttribute(0, BG_ATTR_SCREENSIZE, 2);

    SetGpuReg(REG_OFFSET_BG0CNT, BGCNT_PRIORITY(0) | BGCNT_CHARBASE(0) | BGCNT_SCREENBASE(29) | BGCNT_TXT256x512);
    InitPokeStorageBg0();
    UpdateMonInfoTilemap();
}

static u8 MultiMove_GetOrigin(void)
{
    return (IN_BOX_COLUMNS * sMultiMove->fromRow) + sMultiMove->fromColumn;
}

static bool8 MultiMove_CanPlaceSelection(void)
{
    s32 i, j;
    s32 columnCount = sMultiMove->minColumn + sMultiMove->columnsTotal;
    s32 rowCount = sMultiMove->minRow + sMultiMove->rowsTotal;
    u8 monArrayId = 0;

    for (i = sMultiMove->minRow; i < rowCount; i++)
    {
        u8 boxPosition = (IN_BOX_COLUMNS * i) + sMultiMove->minColumn;
        for (j = sMultiMove->minColumn; j < columnCount; j++)
        {
            if (GetBoxMonData(&sMultiMove->boxMons[monArrayId], MON_DATA_SANITY_HAS_SPECIES)
                && GetCurrentBoxMonData(boxPosition, MON_DATA_SANITY_HAS_SPECIES))
                return FALSE;

            monArrayId++;
            boxPosition++;
        }
    }
    return TRUE;
}


//------------------------------------------------------------------------------
//  SECTION: Item mode
//
//  The functions below handle the Move Items mode
//------------------------------------------------------------------------------

static void CreateItemIconSprites(void)
{
    s32 i;
    u8 spriteId;
    struct CompressedSpriteSheet spriteSheet;
    struct SpriteTemplate spriteTemplate;

    if (sStorage->boxOption == OPTION_MOVE_ITEMS)
    {
        spriteSheet.data = sItemIconGfxBuffer;
        spriteSheet.size = 0x200;
        spriteTemplate = sSpriteTemplate_ItemIcon;

        for (i = 0; i < MAX_ITEM_ICONS; i++)
        {
            spriteSheet.tag = GFXTAG_ITEM_ICON_0 + i;
            LoadCompressedSpriteSheet(&spriteSheet);
            sStorage->itemIcons[i].tiles = GetSpriteTileStartByTag(spriteSheet.tag) * TILE_SIZE_4BPP + (void *)(OBJ_VRAM0);
            sStorage->itemIcons[i].palIndex = AllocSpritePalette(PALTAG_ITEM_ICON_0 + i);
            sStorage->itemIcons[i].palIndex = OBJ_PLTT_ID(sStorage->itemIcons[i].palIndex);
            spriteTemplate.tileTag = GFXTAG_ITEM_ICON_0 + i;
            spriteTemplate.paletteTag = PALTAG_ITEM_ICON_0 + i;
            spriteId = CreateSprite(&spriteTemplate, 0, 0, 11);
            sStorage->itemIcons[i].sprite = &gSprites[spriteId];
            sStorage->itemIcons[i].sprite->invisible = TRUE;
            sStorage->itemIcons[i].active = FALSE;
        }
    }
    sStorage->movingItemId = ITEM_NONE;
}

static void TryLoadItemIconAtPos(u8 cursorArea, u8 cursorPos)
{
    enum Item heldItem;

    if (sStorage->boxOption != OPTION_MOVE_ITEMS)
        return;

    // If we've already loaded the item here, stop
    if (IsItemIconAtPosition(cursorArea, cursorPos))
        return;

    switch (cursorArea)
    {
    case CURSOR_AREA_IN_BOX:
        if (!GetCurrentBoxMonData(cursorPos, MON_DATA_SANITY_HAS_SPECIES))
            return;
        heldItem = GetCurrentBoxMonData(cursorPos, MON_DATA_HELD_ITEM);
        break;
    case CURSOR_AREA_IN_PARTY:
        if (cursorPos >= PARTY_SIZE || !GetMonData(&gParties[B_TRAINER_PLAYER][cursorPos], MON_DATA_SANITY_HAS_SPECIES))
            return;
        heldItem = GetMonData(&gParties[B_TRAINER_PLAYER][cursorPos], MON_DATA_HELD_ITEM);
        break;
    default:
        return;
    }

    if (heldItem != ITEM_NONE)
    {
        const u32 *tiles = GetItemIconPic(heldItem);
        const u16 *pal = GetItemIconPalette(heldItem);
        u8 id = GetNewItemIconIdx();

        SetItemIconPosition(id, cursorArea, cursorPos);
        LoadItemIconGfx(id, tiles, pal);
        SetItemIconAffineAnim(id, ITEM_ANIM_APPEAR);
        SetItemIconActive(id, TRUE);
    }
}

static void TryHideItemIconAtPos(u8 cursorArea, u8 cursorPos)
{
    u8 id;

    if (sStorage->boxOption != OPTION_MOVE_ITEMS)
        return;

    id = GetItemIconIdxByPosition(cursorArea, cursorPos);
    SetItemIconAffineAnim(id, ITEM_ANIM_DISAPPEAR);
    SetItemIconCallback(id, ITEM_CB_WAIT_ANIM, cursorArea, cursorPos);
}

static void TakeItemFromMon(u8 cursorArea, u8 cursorPos)
{
    u8 id;
    enum Item itemId;

    if (sStorage->boxOption != OPTION_MOVE_ITEMS)
        return;

    id = GetItemIconIdxByPosition(cursorArea, cursorPos);
    itemId = ITEM_NONE;
    SetItemIconAffineAnim(id, ITEM_ANIM_PICK_UP);
    SetItemIconCallback(id, ITEM_CB_TO_HAND, cursorArea, cursorPos);
    SetItemIconPosition(id, CURSOR_AREA_IN_HAND, 0);
    if (cursorArea == CURSOR_AREA_IN_BOX)
    {
        SetCurrentBoxMonData(cursorPos, MON_DATA_HELD_ITEM, &itemId);
        SetBoxMonIconObjMode(cursorPos, ST_OAM_OBJ_BLEND);
        SetMonFormPSS_ItemHold(&gPokemonStoragePtr->boxes[StorageGetCurrentBox()][cursorPos]);
    }
    else
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][cursorPos];
        SetMonData(&gParties[B_TRAINER_PLAYER][cursorPos], MON_DATA_HELD_ITEM, &itemId);
        SetPartyMonIconObjMode(cursorPos, ST_OAM_OBJ_BLEND);
        SetMonFormPSS_ItemHold(&mon->box);
    }

    sStorage->movingItemId = sStorage->displayMon.heldItem;
}

static void InitItemIconInCursor(enum Item itemId)
{
    const u32 *tiles = GetItemIconPic(itemId);
    const u16 *pal = GetItemIconPalette(itemId);
    u8 id = GetNewItemIconIdx();
    LoadItemIconGfx(id, tiles, pal);
    SetItemIconAffineAnim(id, ITEM_ANIM_LARGE);
    SetItemIconCallback(id, ITEM_CB_TO_HAND, CURSOR_AREA_IN_BOX, 0);
    SetItemIconPosition(id, CURSOR_AREA_IN_HAND, 0);
    SetItemIconActive(id, TRUE);
    sStorage->movingItemId = itemId;
}

static void SwapItemsWithMon(u8 cursorArea, u8 cursorPos)
{
    u8 id;
    enum Item itemId;

    if (sStorage->boxOption != OPTION_MOVE_ITEMS)
        return;

    id = GetItemIconIdxByPosition(cursorArea, cursorPos);
    SetItemIconAffineAnim(id, ITEM_ANIM_PICK_UP);
    SetItemIconCallback(id, ITEM_CB_SWAP_TO_HAND, CURSOR_AREA_IN_HAND, 0);
    if (cursorArea == CURSOR_AREA_IN_BOX)
    {
        itemId = GetCurrentBoxMonData(cursorPos, MON_DATA_HELD_ITEM);
        SetCurrentBoxMonData(cursorPos, MON_DATA_HELD_ITEM, &sStorage->movingItemId);
        sStorage->movingItemId = itemId;
        SetMonFormPSS_ItemHold(&gPokemonStoragePtr->boxes[StorageGetCurrentBox()][cursorPos]);
    }
    else
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][cursorPos];
        itemId = GetMonData(&gParties[B_TRAINER_PLAYER][cursorPos], MON_DATA_HELD_ITEM);
        SetMonData(&gParties[B_TRAINER_PLAYER][cursorPos], MON_DATA_HELD_ITEM, &sStorage->movingItemId);
        sStorage->movingItemId = itemId;
        SetMonFormPSS_ItemHold(&mon->box);
    }

    id = GetItemIconIdxByPosition(CURSOR_AREA_IN_HAND, 0);
    SetItemIconAffineAnim(id, ITEM_ANIM_PUT_DOWN);
    SetItemIconCallback(id, ITEM_CB_SWAP_TO_MON, cursorArea, cursorPos);
}

static void GiveItemToMon(u8 cursorArea, u8 cursorPos)
{
    u8 id;

    if (sStorage->boxOption != OPTION_MOVE_ITEMS)
        return;

    id = GetItemIconIdxByPosition(CURSOR_AREA_IN_HAND, 0);
    SetItemIconAffineAnim(id, ITEM_ANIM_PUT_DOWN);
    SetItemIconCallback(id, ITEM_CB_TO_MON, cursorArea, cursorPos);
    if (cursorArea == CURSOR_AREA_IN_BOX)
    {
        SetCurrentBoxMonData(cursorPos, MON_DATA_HELD_ITEM, &sStorage->movingItemId);
        SetBoxMonIconObjMode(cursorPos, ST_OAM_OBJ_NORMAL);
        SetMonFormPSS_ItemHold(&gPokemonStoragePtr->boxes[StorageGetCurrentBox()][cursorPos]);
    }
    else
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][cursorPos];
        SetMonData(&gParties[B_TRAINER_PLAYER][cursorPos], MON_DATA_HELD_ITEM, &sStorage->movingItemId);
        SetPartyMonIconObjMode(cursorPos, ST_OAM_OBJ_NORMAL);
        SetMonFormPSS_ItemHold(&mon->box);
    }
}

static void MoveItemFromMonToBag(u8 cursorArea, u8 cursorPos)
{
    u8 id;
    enum Item itemId;

    if (sStorage->boxOption != OPTION_MOVE_ITEMS)
        return;

    itemId = ITEM_NONE;
    id = GetItemIconIdxByPosition(cursorArea, cursorPos);
    SetItemIconAffineAnim(id, ITEM_ANIM_DISAPPEAR);
    SetItemIconCallback(id, ITEM_CB_WAIT_ANIM, cursorArea, cursorPos);
    if (cursorArea == CURSOR_AREA_IN_BOX)
    {
        SetCurrentBoxMonData(cursorPos, MON_DATA_HELD_ITEM, &itemId);
        SetBoxMonIconObjMode(cursorPos, ST_OAM_OBJ_BLEND);
        SetMonFormPSS_ItemHold(&gPokemonStoragePtr->boxes[StorageGetCurrentBox()][cursorPos]);
    }
    else
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][cursorPos];
        SetMonData(&gParties[B_TRAINER_PLAYER][cursorPos], MON_DATA_HELD_ITEM, &itemId);
        SetPartyMonIconObjMode(cursorPos, ST_OAM_OBJ_BLEND);
        SetMonFormPSS_ItemHold(&mon->box);
    }
}

static void MoveItemFromCursorToBag(void)
{
    if (sStorage->boxOption == OPTION_MOVE_ITEMS)
    {
        u8 id = GetItemIconIdxByPosition(CURSOR_AREA_IN_HAND, 0);
        SetItemIconAffineAnim(id, ITEM_ANIM_PUT_AWAY);
        SetItemIconCallback(id, ITEM_CB_WAIT_ANIM, CURSOR_AREA_IN_HAND, 0);
    }
}

// The party menu is being closed, if the cursor is on
// a Pokémon that has a held item make sure it slides
// up along with the closing menu.
static void MoveHeldItemWithPartyMenu(void)
{
    s32 i;

    if (sStorage->boxOption != OPTION_MOVE_ITEMS)
        return;

    for (i = 0; i < MAX_ITEM_ICONS; i++)
    {
        if (sStorage->itemIcons[i].active
         && sStorage->itemIcons[i].area == CURSOR_AREA_IN_PARTY)
            SetItemIconCallback(i, ITEM_CB_HIDE_PARTY, CURSOR_AREA_IN_HAND, 0);
    }
}

static bool8 IsItemIconAnimActive(void)
{
    s32 i;

    for (i = 0; i < MAX_ITEM_ICONS; i++)
    {
        if (sStorage->itemIcons[i].active)
        {
            if (!sStorage->itemIcons[i].sprite->affineAnimEnded
                && sStorage->itemIcons[i].sprite->affineAnimBeginning)
                return TRUE;
            if (sStorage->itemIcons[i].sprite->callback != SpriteCallbackDummy
                && sStorage->itemIcons[i].sprite->callback != SpriteCB_ItemIcon_SetPosToCursor)
                return TRUE;
        }
    }
    return FALSE;
}

static bool8 IsMovingItem(void)
{
    s32 i;

    if (sStorage->boxOption == OPTION_MOVE_ITEMS)
    {
        for (i = 0; i < MAX_ITEM_ICONS; i++)
        {
            if (sStorage->itemIcons[i].active
                && sStorage->itemIcons[i].area == CURSOR_AREA_IN_HAND)
                return TRUE;
        }
    }
    return FALSE;
}

static const u8 *GetMovingItemName(void)
{
    return GetItemName(sStorage->movingItemId);
}

static u16 GetMovingItemId(void)
{
    return sStorage->movingItemId;
}

static u8 GetNewItemIconIdx(void)
{
    u8 i;

    for (i = 0; i < MAX_ITEM_ICONS; i++)
    {
        if (!sStorage->itemIcons[i].active)
        {
            sStorage->itemIcons[i].active = TRUE;
            return i;
        }
    }
    return MAX_ITEM_ICONS;
}

static bool32 IsItemIconAtPosition(u8 cursorArea, u8 cursorPos)
{
    s32 i;

    for (i = 0; i < MAX_ITEM_ICONS; i++)
    {
        if (sStorage->itemIcons[i].active
            && sStorage->itemIcons[i].area == cursorArea
            && sStorage->itemIcons[i].pos == cursorPos)
            return TRUE;
    }
    return FALSE;
}

static u8 GetItemIconIdxByPosition(u8 cursorArea, u8 cursorPos)
{
    u8 i;

    for (i = 0; i < MAX_ITEM_ICONS; i++)
    {
        if (sStorage->itemIcons[i].active
            && sStorage->itemIcons[i].area == cursorArea
            && sStorage->itemIcons[i].pos == cursorPos)
            return i;
    }
    return MAX_ITEM_ICONS;
}

static u8 GetItemIconIdxBySprite(struct Sprite *sprite)
{
    u8 i;

    for (i = 0; i < MAX_ITEM_ICONS; i++)
    {
        if (sStorage->itemIcons[i].active
            && sStorage->itemIcons[i].sprite == sprite)
            return i;
    }
    return MAX_ITEM_ICONS;
}

static void SetItemIconPosition(u8 id, u8 cursorArea, u8 cursorPos)
{
    u8 x, y;

    if (id >= MAX_ITEM_ICONS)
        return;

    switch (cursorArea)
    {
    case CURSOR_AREA_IN_BOX:
        x = cursorPos % IN_BOX_COLUMNS;
        y = cursorPos / IN_BOX_COLUMNS;
        sStorage->itemIcons[id].sprite->x = (24 * x) + 104;
        sStorage->itemIcons[id].sprite->y = (24 * y) + 52;
        sStorage->itemIcons[id].sprite->oam.priority = 1;
        sStorage->itemIcons[id].sprite->subpriority = 11;
        break;
    case CURSOR_AREA_IN_PARTY:
        sStorage->itemIcons[id].sprite->x = 52;
        sStorage->itemIcons[id].sprite->y = 24 * cursorPos + 28;
        sStorage->itemIcons[id].sprite->oam.priority = 1;
        sStorage->itemIcons[id].sprite->subpriority = 11;
        break;
    }

    sStorage->itemIcons[id].area = cursorArea;
    sStorage->itemIcons[id].pos = cursorPos;
}

static void LoadItemIconGfx(u8 id, const u32 *itemTiles, const u16 *itemPal)
{
    s32 i;

    if (id >= MAX_ITEM_ICONS)
        return;

    CpuFastFill(0, sStorage->itemIconBuffer, 0x200);
    DecompressDataWithHeaderWram(itemTiles, sStorage->tileBuffer);
    for (i = 0; i < 3; i++)
        CpuFastCopy(&sStorage->tileBuffer[i * 0x60], &sStorage->itemIconBuffer[i * 0x80], 0x60);

    CpuFastCopy(sStorage->itemIconBuffer, sStorage->itemIcons[id].tiles, 0x200);
    LoadPalette(itemPal, sStorage->itemIcons[id].palIndex, PLTT_SIZE_4BPP);
}

static void SetItemIconAffineAnim(u8 id, u8 animNum)
{
    if (id >= MAX_ITEM_ICONS)
        return;

    StartSpriteAffineAnim(sStorage->itemIcons[id].sprite, animNum);
}

#define sItemIconId data[0]
#define sState      data[0]
#define sCursorArea data[6]
#define sCursorPos  data[7]

static void SetItemIconCallback(u8 id, u8 callbackId, u8 cursorArea, u8 cursorPos)
{
    if (id >= MAX_ITEM_ICONS)
        return;

    switch (callbackId)
    {
    case ITEM_CB_WAIT_ANIM:
        sStorage->itemIcons[id].sprite->sItemIconId = id;
        sStorage->itemIcons[id].sprite->callback = SpriteCB_ItemIcon_WaitAnim;
        break;
    case ITEM_CB_TO_HAND:
        sStorage->itemIcons[id].sprite->sState = 0;
        sStorage->itemIcons[id].sprite->callback = SpriteCB_ItemIcon_ToHand;
        break;
    case ITEM_CB_TO_MON:
        sStorage->itemIcons[id].sprite->sState = 0;
        sStorage->itemIcons[id].sprite->sCursorArea = cursorArea;
        sStorage->itemIcons[id].sprite->sCursorPos = cursorPos;
        sStorage->itemIcons[id].sprite->callback = SpriteCB_ItemIcon_ToMon;
        break;
    case ITEM_CB_SWAP_TO_HAND:
        sStorage->itemIcons[id].sprite->sState = 0;
        sStorage->itemIcons[id].sprite->callback = SpriteCB_ItemIcon_SwapToHand;
        sStorage->itemIcons[id].sprite->sCursorArea = cursorArea;
        sStorage->itemIcons[id].sprite->sCursorPos = cursorPos;
        break;
    case ITEM_CB_SWAP_TO_MON:
        sStorage->itemIcons[id].sprite->sState = 0;
        sStorage->itemIcons[id].sprite->sCursorArea = cursorArea;
        sStorage->itemIcons[id].sprite->sCursorPos = cursorPos;
        sStorage->itemIcons[id].sprite->callback = SpriteCB_ItemIcon_SwapToMon;
        break;
    case ITEM_CB_HIDE_PARTY:
        // If cursor is on a Pokémon with a held item and
        // the player closes the party menu, have the held
        // item follow the Pokémon as the menu slides out
        sStorage->itemIcons[id].sprite->callback = SpriteCB_ItemIcon_HideParty;
        break;
    }
}

static void SetItemIconActive(u8 id, bool8 active)
{
    if (id >= MAX_ITEM_ICONS)
        return;

    sStorage->itemIcons[id].active = active;
    sStorage->itemIcons[id].sprite->invisible = (active == FALSE);
}

static void SpriteCB_ItemIcon_WaitAnim(struct Sprite *sprite)
{
    if (sprite->affineAnimEnded)
    {
        SetItemIconActive(sprite->sItemIconId, FALSE);
        sprite->callback = SpriteCallbackDummy;
    }
}

static void SpriteCB_ItemIcon_ToHand(struct Sprite *sprite)
{
    switch (sprite->sState)
    {
    case 0:
        sprite->data[1] = sprite->x << 4;
        sprite->data[2] = sprite->y << 4;
        sprite->data[3] = 10;
        sprite->data[4] = 16;
        sprite->data[5] = 0;
        sprite->sState++;
    case 1:
        sprite->data[1] -= sprite->data[3];
        sprite->data[2] -= sprite->data[4];
        sprite->x = sprite->data[1] >> 4;
        sprite->y = sprite->data[2] >> 4;
        if (++sprite->data[5] > 11)
            sprite->callback = SpriteCB_ItemIcon_SetPosToCursor;
        break;
    }
}

static void SpriteCB_ItemIcon_SetPosToCursor(struct Sprite *sprite)
{
    sprite->x = sStorage->cursorSprite->x + 4;
    sprite->y = sStorage->cursorSprite->y + sStorage->cursorSprite->y2 + 8;
    sprite->oam.priority = sStorage->cursorSprite->oam.priority;
    sprite->subpriority = (sChooseBoxMenu != NULL) ? 1 : 9;
}

static void SpriteCB_ItemIcon_ToMon(struct Sprite *sprite)
{
    switch (sprite->sState)
    {
    case 0:
        sprite->data[1] = sprite->x << 4;
        sprite->data[2] = sprite->y << 4;
        sprite->data[3] = 10;
        sprite->data[4] = 16;
        sprite->data[5] = 0;
        sprite->sState++;
    case 1:
        sprite->data[1] += sprite->data[3];
        sprite->data[2] += sprite->data[4];
        sprite->x = sprite->data[1] >> 4;
        sprite->y = sprite->data[2] >> 4;
        if (++sprite->data[5] > 11)
        {
            SetItemIconPosition(GetItemIconIdxBySprite(sprite), sprite->sCursorArea, sprite->sCursorPos);
            sprite->callback = SpriteCallbackDummy;
        }
        break;
    }
}

static void SpriteCB_ItemIcon_SwapToHand(struct Sprite *sprite)
{
    switch (sprite->sState)
    {
    case 0:
        sprite->data[1] = sprite->x;
        sprite->data[2] = sprite->y;
        sprite->data[3] = sStorage->cursorSprite->x + 4;
        sprite->data[4] = sStorage->cursorSprite->y + sStorage->cursorSprite->y2 + 8;
        sprite->data[5] = 0;
        sprite->sState++;
    case 1:
        if (++sprite->data[5] > 16)
        {
            SetItemIconPosition(GetItemIconIdxBySprite(sprite), sprite->sCursorArea, sprite->sCursorPos);
            sprite->x2 = 0;
            sprite->y2 = 0;
            sprite->callback = SpriteCB_ItemIcon_SetPosToCursor;
        }
        else
        {
            s32 t = sprite->data[5];
            s32 startX = sprite->data[1];
            s32 startY = sprite->data[2];
            s32 endX = sprite->data[3];
            s32 endY = sprite->data[4];

            sprite->x = startX + (endX - startX) * t / 16;
            sprite->y = startY + (endY - startY) * t / 16;

            s32 dx = endX - startX;
            s32 dy = endY - startY;
            sprite->x2 = -dy * t * (16 - t) / 64;
            sprite->y2 = dx * t * (16 - t) / 64;
        }
        break;
    }
}

static void SpriteCB_ItemIcon_SwapToMon(struct Sprite *sprite)
{
    switch (sprite->sState)
    {
    case 0:
        sprite->data[1] = sprite->x;
        sprite->data[2] = sprite->y;
        if (sprite->sCursorArea == CURSOR_AREA_IN_BOX)
        {
            u8 x = sprite->sCursorPos % IN_BOX_COLUMNS;
            u8 y = sprite->sCursorPos / IN_BOX_COLUMNS;
            sprite->data[3] = (24 * x) + 104;
            sprite->data[4] = (24 * y) + 52;
        }
        else
        {
            sprite->data[3] = 52;
            sprite->data[4] = 24 * sprite->sCursorPos + 28;
        }
        sprite->data[5] = 0;
        sprite->sState++;
    case 1:
        if (++sprite->data[5] > 16)
        {
            SetItemIconPosition(GetItemIconIdxBySprite(sprite), sprite->sCursorArea, sprite->sCursorPos);
            sprite->callback = SpriteCallbackDummy;
            sprite->x2 = 0;
            sprite->y2 = 0;
        }
        else
        {
            s32 t = sprite->data[5];
            s32 startX = sprite->data[1];
            s32 startY = sprite->data[2];
            s32 endX = sprite->data[3];
            s32 endY = sprite->data[4];

            sprite->x = startX + (endX - startX) * t / 16;
            sprite->y = startY + (endY - startY) * t / 16;

            s32 dx = endX - startX;
            s32 dy = endY - startY;
            sprite->x2 = -dy * t * (16 - t) / 64;
            sprite->y2 = dx * t * (16 - t) / 64;
        }
        break;
    }
}

static void SpriteCB_ItemIcon_HideParty(struct Sprite *sprite)
{
    sprite->y -= 8;
    if (sprite->y + sprite->y2 < -16)
    {
        sprite->callback = SpriteCallbackDummy;
        SetItemIconActive(GetItemIconIdxBySprite(sprite), FALSE);
    }
}

#undef sState
#undef sItemIconId
#undef sCursorArea
#undef sCursorPos


//------------------------------------------------------------------------------
//  SECTION: General utility
//------------------------------------------------------------------------------


// Leftover from FRLG
static void UNUSED BackupPokemonStorage(void/*struct PokemonStorage * dest*/)
{
    //*dest = *gPokemonStoragePtr;
}

// Leftover from FRLG
static void UNUSED RestorePokemonStorage(void/*struct PokemonStorage * src*/)
{
    //*gPokemonStoragePtr = *src;
}

// Functions here are general utility functions.
static void SetCurrentBox(u8 boxId)
{
    if (boxId < TOTAL_BOXES_COUNT)
        gPokemonStoragePtr->currentBox = boxId;
}

static struct BoxPokemon *GetCursorBoxMon(void)
{
    struct BoxPokemon *boxmon;
    if (sCursorArea == CURSOR_AREA_IN_PARTY)
        boxmon = &(gParties[B_TRAINER_PLAYER][sCursorPosition].box);
    else
        boxmon = GetBoxedMonPtr(StorageGetCurrentBox(), sCursorPosition);
    return boxmon;
}

static u8 GetBoxWallpaper(u8 boxId)
{
    if (boxId < TOTAL_BOXES_COUNT)
    {
        u8 wallpaperId = gPokemonStoragePtr->boxWallpapers[boxId];
        if (wallpaperId >= WALLPAPER_COUNT)
            return WALLPAPER_BASE;
        return wallpaperId;
    }
    return WALLPAPER_BASE;
}

static void SetBoxWallpaper(u8 boxId, u8 wallpaperId)
{
    if (boxId < TOTAL_BOXES_COUNT && wallpaperId < WALLPAPER_COUNT)
        gPokemonStoragePtr->boxWallpapers[boxId] = wallpaperId;
}


//------------------------------------------------------------------------------
//  SECTION: TilemapUtil
//
//  Handles 3 particular tilemaps in a somewhat unusual way.
//  For example, while the cursor is on the Close Box button it flashes between
//  two states alternately. Both these states are their own part of the same
//  tilemap that's always present. The utility shifts the tilemap up and down
//  to show/hide the states, and limits the view with a rectangle that only
//  reveals one at a time.
//  Each tilemap is tracked with a TILEMAPID that can be used to reference it.
//------------------------------------------------------------------------------


struct TilemapUtil_RectData
{
    s16 x;
    s16 y;
    u16 width;
    u16 height;
    s16 destX;
    s16 destY;
};

struct TilemapUtil
{
    struct TilemapUtil_RectData prev; // Only read in unused function
    struct TilemapUtil_RectData cur;
    const void *savedTilemap; // Only written in unused function
    const void *tilemap;
    u16 altWidth;
    u16 altHeight; // Never read
    u16 width;
    u16 height; // Never read
    u16 rowSize; // Never read
    u8 tileSize;
    u8 bg;
    bool8 active;
};

EWRAM_DATA static struct TilemapUtil *sTilemapUtil = NULL;
EWRAM_DATA static u16 sNumTilemapUtilIds = 0;

static void UNUSED TilemapUtil_UpdateAll(void)
{
    s32 i;

    for (i = 0; i < sNumTilemapUtilIds; i++)
    {
        if (sTilemapUtil[i].active == TRUE)
            TilemapUtil_Update(i);
    }
}

struct
{
    u16 width;
    u16 height;
} static const sTilemapDimensions[][4] =
{
    [BG_TYPE_NORMAL] = {
        { 256,  256},
        { 512,  256},
        { 256,  512},
        { 512,  512},
    },
    [BG_TYPE_AFFINE] = {
        { 128,  128},
        { 256,  256},
        { 512,  512},
        {1024, 1024},
    },
};

static void UNUSED TilemapUtil_SetMap(u8 id, u8 bg, const void *tilemap, u16 width, u16 height)
{
    u16 bgScreenSize, bgType;

    if (id >= sNumTilemapUtilIds)
        return;

    sTilemapUtil[id].savedTilemap = NULL;
    sTilemapUtil[id].tilemap = tilemap;
    sTilemapUtil[id].bg = bg;
    sTilemapUtil[id].width = width;
    sTilemapUtil[id].height = height;

    bgScreenSize = GetBgAttribute(bg, BG_ATTR_SCREENSIZE);
    bgType = GetBgAttribute(bg, BG_ATTR_TYPE);
    sTilemapUtil[id].altWidth = sTilemapDimensions[bgType][bgScreenSize].width;
    sTilemapUtil[id].altHeight = sTilemapDimensions[bgType][bgScreenSize].height;
    if (bgType != BG_TYPE_NORMAL)
        sTilemapUtil[id].tileSize = 1;
    else
        sTilemapUtil[id].tileSize = 2;

    sTilemapUtil[id].rowSize = sTilemapUtil[id].tileSize * width;
    sTilemapUtil[id].cur.width = width;
    sTilemapUtil[id].cur.height = height;
    sTilemapUtil[id].cur.x = 0;
    sTilemapUtil[id].cur.y = 0;
    sTilemapUtil[id].cur.destX = 0;
    sTilemapUtil[id].cur.destY = 0;
    sTilemapUtil[id].prev = sTilemapUtil[id].cur;
    sTilemapUtil[id].active = TRUE;
}

static void UNUSED TilemapUtil_SetSavedMap(u8 id, const void *tilemap)
{
    if (id >= sNumTilemapUtilIds)
        return;

    sTilemapUtil[id].savedTilemap = tilemap;
    sTilemapUtil[id].active = TRUE;
}

static void UNUSED TilemapUtil_SetPos(u8 id, u16 x, u16 y)
{
    if (id >= sNumTilemapUtilIds)
        return;

    sTilemapUtil[id].cur.destX = x;
    sTilemapUtil[id].cur.destY = y;
    sTilemapUtil[id].active = TRUE;
}

static void UNUSED TilemapUtil_Move(u8 id, u8 mode, s8 val)
{
    if (id >= sNumTilemapUtilIds)
        return;

    switch (mode)
    {
    case 0:
        sTilemapUtil[id].cur.destX += val;
        sTilemapUtil[id].cur.width -= val;
        break;
    case 1:
        sTilemapUtil[id].cur.x += val;
        sTilemapUtil[id].cur.width += val;
        break;
    case 2:
        sTilemapUtil[id].cur.destY += val;
        sTilemapUtil[id].cur.height -= val;
        break;
    case 3:
        sTilemapUtil[id].cur.y -= val;
        sTilemapUtil[id].cur.height += val;
        break;
    case 4:
        sTilemapUtil[id].cur.destX += val;
        break;
    case 5:
        sTilemapUtil[id].cur.destY += val;
        break;
    }

    sTilemapUtil[id].active = TRUE;
}

static void TilemapUtil_Update(u8 id)
{
    if (id >= sNumTilemapUtilIds)
        return;

    if (sTilemapUtil[id].savedTilemap != NULL)
        TilemapUtil_DrawPrev(id); // Never called, above always FALSE

    TilemapUtil_Draw(id);
    sTilemapUtil[id].prev = sTilemapUtil[id].cur;
}

static void TilemapUtil_DrawPrev(u8 id)
{
    s32 i;
    u32 adder = sTilemapUtil[id].tileSize * sTilemapUtil[id].altWidth;
    const void *tiles = (sTilemapUtil[id].savedTilemap + (adder * sTilemapUtil[id].prev.destY))
                      + (sTilemapUtil[id].tileSize * sTilemapUtil[id].prev.destX);

    for (i = 0; i < sTilemapUtil[id].prev.height; i++)
    {
        CopyToBgTilemapBufferRect(sTilemapUtil[id].bg,
                                  tiles,
                                  sTilemapUtil[id].prev.destX,
                                  sTilemapUtil[id].prev.destY + i,
                                  sTilemapUtil[id].prev.width,
                                  1);
        tiles += adder;
    }
}

static void TilemapUtil_Draw(u8 id)
{
    s32 i;
    u32 adder = sTilemapUtil[id].tileSize * sTilemapUtil[id].width;
    const void *tiles = (sTilemapUtil[id].tilemap + (adder * sTilemapUtil[id].cur.y))
                      + (sTilemapUtil[id].tileSize * sTilemapUtil[id].cur.x);

    for (i = 0; i < sTilemapUtil[id].cur.height; i++)
    {
        CopyToBgTilemapBufferRect(sTilemapUtil[id].bg,
                                  tiles,
                                  sTilemapUtil[id].cur.destX,
                                  sTilemapUtil[id].cur.destY + i,
                                  sTilemapUtil[id].cur.width,
                                  1);
        tiles += adder;
    }
}


//------------------------------------------------------------------------------
//  SECTION: UnkUtil
//
//  Some data transfer utility that goes functionally unused.
//  It gets initialized with UnkUtil_Init, and run every vblank in Pokémon
//  Storage with UnkUtil_Run, but neither of the Add functions are ever used,
//  so UnkUtil_Run performs no actions.
//------------------------------------------------------------------------------

void UpdateSpeciesSpritePSS_SwSh(struct BoxPokemon *boxMon)
{
    enum Species species = GetBoxMonData(boxMon, MON_DATA_SPECIES);
    bool32 isShiny = GetBoxMonData(boxMon, MON_DATA_IS_SHINY);
    u32 pid = GetBoxMonData(boxMon, MON_DATA_PERSONALITY);
    bool32 isEgg = GetBoxMonData(boxMon, MON_DATA_IS_EGG);

    // Update front sprite
    sStorage->displayMon.species = species;
    sStorage->displayMon.palette = GetMonSpritePalFromSpeciesAndPersonalityIsEgg(species, isShiny, pid, isEgg);
    sStorage->displayMon.isEgg = isEgg;
    if (!sJustOpenedBag)
    {
        if (sRefreshDisplayMonGfx)
        {
            RefreshDisplayMonData();
            sRefreshDisplayMonGfx = FALSE;
        }

        // Recreate icon sprite
        if (sCursorArea == CURSOR_AREA_IN_PARTY)
        {
            if (sStorage->partySprites[sCursorPosition] != NULL)
            {
                DestroyBoxMonIcon(sStorage->partySprites[sCursorPosition]);
                sStorage->partySprites[sCursorPosition] = NULL;
            }
            species = GetMonData(&gParties[B_TRAINER_PLAYER][sCursorPosition], MON_DATA_SPECIES);
            isEgg = GetMonData(&gParties[B_TRAINER_PLAYER][sCursorPosition], MON_DATA_IS_EGG);
            if (species != SPECIES_NONE)
            {
                u32 personality = GetMonData(&gParties[B_TRAINER_PLAYER][sCursorPosition], MON_DATA_PERSONALITY);
                sStorage->partySprites[sCursorPosition] = CreateMonIconSprite(species, personality, 40, 24 * sCursorPosition + 16, 1, 12, isEgg);
                if (sStorage->boxOption == OPTION_MOVE_ITEMS)
                    SetPartyMonIconObjMode(sCursorPosition, (GetMonData(&gParties[B_TRAINER_PLAYER][sCursorPosition], MON_DATA_HELD_ITEM) == ITEM_NONE ? ST_OAM_OBJ_BLEND : ST_OAM_OBJ_NORMAL));
            }
        }
        else
        {
            DestroyBoxMonIconAtPosition(sCursorPosition);
            CreateBoxMonIconAtPos(sCursorPosition);
            if (sStorage->boxOption == OPTION_MOVE_ITEMS)
                SetBoxMonIconObjMode(sCursorPosition, (GetBoxMonData(boxMon, MON_DATA_HELD_ITEM) == ITEM_NONE ? ST_OAM_OBJ_BLEND : ST_OAM_OBJ_NORMAL));
        }
    }
    sJustOpenedBag = FALSE;
}

void ChooseMonFromStorage_SwSh(void)
{
    EnterPokeStorage(OPTION_SELECT_MON);
}