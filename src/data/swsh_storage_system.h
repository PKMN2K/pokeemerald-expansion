// ============================================================================
// PC Storage System - Data
// ============================================================================

// Wallpaper IDs
enum {
	//Wallpapers Page 1
	WALLPAPER_BASE,			//Default
    WALLPAPER_PLAINS, 		//Plains (Grass / Meadow)
    WALLPAPER_CITY,			//City
    WALLPAPER_DESERT, 		//Desert
    WALLPAPER_CENTER, 		//PokeCenter
	//Wallpapers Page 2
    WALLPAPER_SHORE,		//Surface Water
    WALLPAPER_OCEAN,		//Underwater
    WALLPAPER_MOUNTAIN, 	//Mountain
    WALLPAPER_VOLCANO,		//Volcano
    WALLPAPER_CAVE, 		//Cave
	//Wallpapers Page 3
    WALLPAPER_BEACH,		//Beach
    WALLPAPER_SNOW, 		//Snow
    WALLPAPER_SKY, 			//Sky
    WALLPAPER_COMPUTA, 		//PC
    WALLPAPER_CUTE, 		//Cute (Cross Stitch)
	// Wallpapers 16-24. Keep the legacy symbol names for IDs 15-19 so
	// existing saves/source references retain their numeric values; the live
	// HGSS menu labels below reflect the authenticated native wallpaper order.
    WALLPAPER_SPACE,
    WALLPAPER_DAYCARE,
    WALLPAPER_CONTEST,
    WALLPAPER_CLASSIC,
    WALLPAPER_CLASSIC2,
    WALLPAPER_SPECIAL_5,
    WALLPAPER_SPECIAL_6,
    WALLPAPER_SPECIAL_7,
    WALLPAPER_SPECIAL_8,
    WALLPAPER_COUNT
};

// ============================================================================
// Structs
// ============================================================================

struct StorageMessage
{
    const u8 *text;
    u8 format;
};

// ============================================================================
// Graphics - Storage System UI
// ============================================================================

// Authentic HGSS Storage Pokemon-info panel: /a/0/1/9 NSCR 9 + NCGR 14 + NCLR 4.
static const u32 sHgssMonInfoPanel_Gfx[]       = INCGFX_U32("graphics/gen4_ui/hgss_storage/verified/mon_info_panel.png", ".4bpp");
static const u16 sHgssMonInfoPanel_Pal[]       = INCGFX_U16("graphics/gen4_ui/hgss_storage/verified/mon_info_panel.png", ".gbapal");

// Authentic HGSS Storage message bar: /a/0/1/9 NSCR 11 + NCGR 14 + NCLR 4.
static const u32 sHgssMessageWindow_Gfx[]       = INCGFX_U32("graphics/gen4_ui/hgss_storage/verified/message_window.png", ".4bpp");
static const u16 sHgssMessageWindow_Pal[]       = INCGFX_U16("graphics/gen4_ui/hgss_storage/verified/message_window.png", ".gbapal");

// Authentic HGSS Storage Yes/No panel: /a/0/1/9 NSCR 12 + NCGR 14 + NCLR 4.
static const u32 sHgssYesNo_Gfx[]               = INCGFX_U32("graphics/gen4_ui/hgss_storage/verified/yes_no.png", ".4bpp");
static const u16 sHgssYesNo_Pal[]               = INCGFX_U16("graphics/gen4_ui/hgss_storage/verified/yes_no.png", ".gbapal");

// Authentic HGSS Storage context-menu frame: /a/0/1/9 NSCR 86 + NCGR 14 + NCLR 4.
static const u32 sHgssContextMenu_Gfx[]          = INCGFX_U32("graphics/gen4_ui/hgss_storage/verified/context_menu.png", ".4bpp");
static const u16 sHgssContextMenu_Pal[]          = INCGFX_U16("graphics/gen4_ui/hgss_storage/verified/context_menu.png", ".gbapal");

// Authentic HGSS Storage wallpaper-selector swatch.
// /a/0/1/9 NCGR 74 + NCLR 75 + NCER 76 + NANR 77.
// The 24x24 export is the exact static NCER cell, compacted losslessly from
// 8bpp {transparent, placeholder} pixels to a 4bpp mask for GBA OBJ use.
static const u32 sHgssWallpaperSelectorSwatch_Gfx[] = INCGFX_U32("graphics/gen4_ui/hgss_storage/verified/wallpaper_selector_swatch.png", ".4bpp");

// Exact BGR555 colors from HGSS /a/0/1/9 NCLR member 75.
// IDs 0-15 use the directly verified native selector indices 16-31.
// IDs 16-23 use the remaining authentic first eight member-75 colors as the
// compact GBA continuation; no synthetic colors are introduced.
static const u16 sHgssWallpaperSelectorColors[WALLPAPER_COUNT] =
{
    [WALLPAPER_BASE]      = 0x523F,
    [WALLPAPER_PLAINS]    = 0x7A2C,
    [WALLPAPER_CITY]      = 0x43B3,
    [WALLPAPER_DESERT]    = 0x7B71,
    [WALLPAPER_CENTER]    = 0x4EDC,
    [WALLPAPER_SHORE]     = 0x3F9F,
    [WALLPAPER_OCEAN]     = 0x4E19,
    [WALLPAPER_MOUNTAIN]  = 0x327E,
    [WALLPAPER_VOLCANO]   = 0x0000,
    [WALLPAPER_CAVE]      = 0x761F,
    [WALLPAPER_BEACH]     = 0x7BDE,
    [WALLPAPER_SNOW]      = 0x5ED6,
    [WALLPAPER_SKY]       = 0x1A1A,
    [WALLPAPER_COMPUTA]   = 0x41CE,
    [WALLPAPER_CUTE]      = 0x18DC,
    [WALLPAPER_SPACE]     = 0x7528,
    [WALLPAPER_DAYCARE]   = 0x3F0F,
    [WALLPAPER_CONTEST]   = 0x5294,
    [WALLPAPER_CLASSIC]   = 0x3B38,
    [WALLPAPER_CLASSIC2]  = 0x2EF5,
    [WALLPAPER_SPECIAL_5] = 0x6658,
    [WALLPAPER_SPECIAL_6] = 0x427B,
    [WALLPAPER_SPECIAL_7] = 0x7BBA,
    [WALLPAPER_SPECIAL_8] = 0x4A76,
};

static const u16 sTextWindows_Pal[]           = INCGFX_U16("graphics/pokemon_storage/swsh/text_windows.pal", ".gbapal");

// Generated from visually verified HGSS /a/0/1/9 role bindings.
#include "hgss_storage_main_frame.inc.h"
#include "hgss_storage_party_panel.inc.h"

// Verified authentic HGSS PC wallpaper set.
// /a/0/1/9: shared NSCR 15 + NCGR 16-39 + NCLR 40-63.
// Every source is the 1:1 168x160 verified PNG; no legacy wallpaper art is used
// by the live BG3 loader once a valid wallpaper ID is selected.
static const u32 sHgssStorageWallpaper01_Gfx[] = INCGFX_U32("graphics/gen4_ui/hgss_storage/verified/wallpaper_01.png", ".4bpp");
static const u16 sHgssStorageWallpaper01_Pal[] = INCGFX_U16("graphics/gen4_ui/hgss_storage/verified/wallpaper_01.png", ".gbapal");
static const u32 sHgssStorageWallpaper02_Gfx[] = INCGFX_U32("graphics/gen4_ui/hgss_storage/verified/wallpaper_02.png", ".4bpp");
static const u16 sHgssStorageWallpaper02_Pal[] = INCGFX_U16("graphics/gen4_ui/hgss_storage/verified/wallpaper_02.png", ".gbapal");
static const u32 sHgssStorageWallpaper03_Gfx[] = INCGFX_U32("graphics/gen4_ui/hgss_storage/verified/wallpaper_03.png", ".4bpp");
static const u16 sHgssStorageWallpaper03_Pal[] = INCGFX_U16("graphics/gen4_ui/hgss_storage/verified/wallpaper_03.png", ".gbapal");
static const u32 sHgssStorageWallpaper04_Gfx[] = INCGFX_U32("graphics/gen4_ui/hgss_storage/verified/wallpaper_04.png", ".4bpp");
static const u16 sHgssStorageWallpaper04_Pal[] = INCGFX_U16("graphics/gen4_ui/hgss_storage/verified/wallpaper_04.png", ".gbapal");
static const u32 sHgssStorageWallpaper05_Gfx[] = INCGFX_U32("graphics/gen4_ui/hgss_storage/verified/wallpaper_05.png", ".4bpp");
static const u16 sHgssStorageWallpaper05_Pal[] = INCGFX_U16("graphics/gen4_ui/hgss_storage/verified/wallpaper_05.png", ".gbapal");
static const u32 sHgssStorageWallpaper06_Gfx[] = INCGFX_U32("graphics/gen4_ui/hgss_storage/verified/wallpaper_06.png", ".4bpp");
static const u16 sHgssStorageWallpaper06_Pal[] = INCGFX_U16("graphics/gen4_ui/hgss_storage/verified/wallpaper_06.png", ".gbapal");
static const u32 sHgssStorageWallpaper07_Gfx[] = INCGFX_U32("graphics/gen4_ui/hgss_storage/verified/wallpaper_07.png", ".4bpp");
static const u16 sHgssStorageWallpaper07_Pal[] = INCGFX_U16("graphics/gen4_ui/hgss_storage/verified/wallpaper_07.png", ".gbapal");
static const u32 sHgssStorageWallpaper08_Gfx[] = INCGFX_U32("graphics/gen4_ui/hgss_storage/verified/wallpaper_08.png", ".4bpp");
static const u16 sHgssStorageWallpaper08_Pal[] = INCGFX_U16("graphics/gen4_ui/hgss_storage/verified/wallpaper_08.png", ".gbapal");
static const u32 sHgssStorageWallpaper09_Gfx[] = INCGFX_U32("graphics/gen4_ui/hgss_storage/verified/wallpaper_09.png", ".4bpp");
static const u16 sHgssStorageWallpaper09_Pal[] = INCGFX_U16("graphics/gen4_ui/hgss_storage/verified/wallpaper_09.png", ".gbapal");
static const u32 sHgssStorageWallpaper10_Gfx[] = INCGFX_U32("graphics/gen4_ui/hgss_storage/verified/wallpaper_10.png", ".4bpp");
static const u16 sHgssStorageWallpaper10_Pal[] = INCGFX_U16("graphics/gen4_ui/hgss_storage/verified/wallpaper_10.png", ".gbapal");
static const u32 sHgssStorageWallpaper11_Gfx[] = INCGFX_U32("graphics/gen4_ui/hgss_storage/verified/wallpaper_11.png", ".4bpp");
static const u16 sHgssStorageWallpaper11_Pal[] = INCGFX_U16("graphics/gen4_ui/hgss_storage/verified/wallpaper_11.png", ".gbapal");
static const u32 sHgssStorageWallpaper12_Gfx[] = INCGFX_U32("graphics/gen4_ui/hgss_storage/verified/wallpaper_12.png", ".4bpp");
static const u16 sHgssStorageWallpaper12_Pal[] = INCGFX_U16("graphics/gen4_ui/hgss_storage/verified/wallpaper_12.png", ".gbapal");
static const u32 sHgssStorageWallpaper13_Gfx[] = INCGFX_U32("graphics/gen4_ui/hgss_storage/verified/wallpaper_13.png", ".4bpp");
static const u16 sHgssStorageWallpaper13_Pal[] = INCGFX_U16("graphics/gen4_ui/hgss_storage/verified/wallpaper_13.png", ".gbapal");
static const u32 sHgssStorageWallpaper14_Gfx[] = INCGFX_U32("graphics/gen4_ui/hgss_storage/verified/wallpaper_14.png", ".4bpp");
static const u16 sHgssStorageWallpaper14_Pal[] = INCGFX_U16("graphics/gen4_ui/hgss_storage/verified/wallpaper_14.png", ".gbapal");
static const u32 sHgssStorageWallpaper15_Gfx[] = INCGFX_U32("graphics/gen4_ui/hgss_storage/verified/wallpaper_15.png", ".4bpp");
static const u16 sHgssStorageWallpaper15_Pal[] = INCGFX_U16("graphics/gen4_ui/hgss_storage/verified/wallpaper_15.png", ".gbapal");
static const u32 sHgssStorageWallpaper16_Gfx[] = INCGFX_U32("graphics/gen4_ui/hgss_storage/verified/wallpaper_16.png", ".4bpp");
static const u16 sHgssStorageWallpaper16_Pal[] = INCGFX_U16("graphics/gen4_ui/hgss_storage/verified/wallpaper_16.png", ".gbapal");
static const u32 sHgssStorageWallpaper17_Gfx[] = INCGFX_U32("graphics/gen4_ui/hgss_storage/verified/wallpaper_17.png", ".4bpp");
static const u16 sHgssStorageWallpaper17_Pal[] = INCGFX_U16("graphics/gen4_ui/hgss_storage/verified/wallpaper_17.png", ".gbapal");
static const u32 sHgssStorageWallpaper18_Gfx[] = INCGFX_U32("graphics/gen4_ui/hgss_storage/verified/wallpaper_18.png", ".4bpp");
static const u16 sHgssStorageWallpaper18_Pal[] = INCGFX_U16("graphics/gen4_ui/hgss_storage/verified/wallpaper_18.png", ".gbapal");
static const u32 sHgssStorageWallpaper19_Gfx[] = INCGFX_U32("graphics/gen4_ui/hgss_storage/verified/wallpaper_19.png", ".4bpp");
static const u16 sHgssStorageWallpaper19_Pal[] = INCGFX_U16("graphics/gen4_ui/hgss_storage/verified/wallpaper_19.png", ".gbapal");
static const u32 sHgssStorageWallpaper20_Gfx[] = INCGFX_U32("graphics/gen4_ui/hgss_storage/verified/wallpaper_20.png", ".4bpp");
static const u16 sHgssStorageWallpaper20_Pal[] = INCGFX_U16("graphics/gen4_ui/hgss_storage/verified/wallpaper_20.png", ".gbapal");
static const u32 sHgssStorageWallpaper21_Gfx[] = INCGFX_U32("graphics/gen4_ui/hgss_storage/verified/wallpaper_21.png", ".4bpp");
static const u16 sHgssStorageWallpaper21_Pal[] = INCGFX_U16("graphics/gen4_ui/hgss_storage/verified/wallpaper_21.png", ".gbapal");
static const u32 sHgssStorageWallpaper22_Gfx[] = INCGFX_U32("graphics/gen4_ui/hgss_storage/verified/wallpaper_22.png", ".4bpp");
static const u16 sHgssStorageWallpaper22_Pal[] = INCGFX_U16("graphics/gen4_ui/hgss_storage/verified/wallpaper_22.png", ".gbapal");
static const u32 sHgssStorageWallpaper23_Gfx[] = INCGFX_U32("graphics/gen4_ui/hgss_storage/verified/wallpaper_23.png", ".4bpp");
static const u16 sHgssStorageWallpaper23_Pal[] = INCGFX_U16("graphics/gen4_ui/hgss_storage/verified/wallpaper_23.png", ".gbapal");
static const u32 sHgssStorageWallpaper24_Gfx[] = INCGFX_U32("graphics/gen4_ui/hgss_storage/verified/wallpaper_24.png", ".4bpp");
static const u16 sHgssStorageWallpaper24_Pal[] = INCGFX_U16("graphics/gen4_ui/hgss_storage/verified/wallpaper_24.png", ".gbapal");

static const u32 *const sHgssStorageWallpaperGfx[WALLPAPER_COUNT] =
{
    [WALLPAPER_BASE] = sHgssStorageWallpaper01_Gfx,
    [WALLPAPER_PLAINS] = sHgssStorageWallpaper02_Gfx,
    [WALLPAPER_CITY] = sHgssStorageWallpaper03_Gfx,
    [WALLPAPER_DESERT] = sHgssStorageWallpaper04_Gfx,
    [WALLPAPER_CENTER] = sHgssStorageWallpaper05_Gfx,
    [WALLPAPER_SHORE] = sHgssStorageWallpaper06_Gfx,
    [WALLPAPER_OCEAN] = sHgssStorageWallpaper07_Gfx,
    [WALLPAPER_MOUNTAIN] = sHgssStorageWallpaper08_Gfx,
    [WALLPAPER_VOLCANO] = sHgssStorageWallpaper09_Gfx,
    [WALLPAPER_CAVE] = sHgssStorageWallpaper10_Gfx,
    [WALLPAPER_BEACH] = sHgssStorageWallpaper11_Gfx,
    [WALLPAPER_SNOW] = sHgssStorageWallpaper12_Gfx,
    [WALLPAPER_SKY] = sHgssStorageWallpaper13_Gfx,
    [WALLPAPER_COMPUTA] = sHgssStorageWallpaper14_Gfx,
    [WALLPAPER_CUTE] = sHgssStorageWallpaper15_Gfx,
    [WALLPAPER_SPACE] = sHgssStorageWallpaper16_Gfx,
    [WALLPAPER_DAYCARE] = sHgssStorageWallpaper17_Gfx,
    [WALLPAPER_CONTEST] = sHgssStorageWallpaper18_Gfx,
    [WALLPAPER_CLASSIC] = sHgssStorageWallpaper19_Gfx,
    [WALLPAPER_CLASSIC2] = sHgssStorageWallpaper20_Gfx,
    [WALLPAPER_SPECIAL_5] = sHgssStorageWallpaper21_Gfx,
    [WALLPAPER_SPECIAL_6] = sHgssStorageWallpaper22_Gfx,
    [WALLPAPER_SPECIAL_7] = sHgssStorageWallpaper23_Gfx,
    [WALLPAPER_SPECIAL_8] = sHgssStorageWallpaper24_Gfx,
};

static const u16 *const sHgssStorageWallpaperPal[WALLPAPER_COUNT] =
{
    [WALLPAPER_BASE] = sHgssStorageWallpaper01_Pal,
    [WALLPAPER_PLAINS] = sHgssStorageWallpaper02_Pal,
    [WALLPAPER_CITY] = sHgssStorageWallpaper03_Pal,
    [WALLPAPER_DESERT] = sHgssStorageWallpaper04_Pal,
    [WALLPAPER_CENTER] = sHgssStorageWallpaper05_Pal,
    [WALLPAPER_SHORE] = sHgssStorageWallpaper06_Pal,
    [WALLPAPER_OCEAN] = sHgssStorageWallpaper07_Pal,
    [WALLPAPER_MOUNTAIN] = sHgssStorageWallpaper08_Pal,
    [WALLPAPER_VOLCANO] = sHgssStorageWallpaper09_Pal,
    [WALLPAPER_CAVE] = sHgssStorageWallpaper10_Pal,
    [WALLPAPER_BEACH] = sHgssStorageWallpaper11_Pal,
    [WALLPAPER_SNOW] = sHgssStorageWallpaper12_Pal,
    [WALLPAPER_SKY] = sHgssStorageWallpaper13_Pal,
    [WALLPAPER_COMPUTA] = sHgssStorageWallpaper14_Pal,
    [WALLPAPER_CUTE] = sHgssStorageWallpaper15_Pal,
    [WALLPAPER_SPACE] = sHgssStorageWallpaper16_Pal,
    [WALLPAPER_DAYCARE] = sHgssStorageWallpaper17_Pal,
    [WALLPAPER_CONTEST] = sHgssStorageWallpaper18_Pal,
    [WALLPAPER_CLASSIC] = sHgssStorageWallpaper19_Pal,
    [WALLPAPER_CLASSIC2] = sHgssStorageWallpaper20_Pal,
    [WALLPAPER_SPECIAL_5] = sHgssStorageWallpaper21_Pal,
    [WALLPAPER_SPECIAL_6] = sHgssStorageWallpaper22_Pal,
    [WALLPAPER_SPECIAL_7] = sHgssStorageWallpaper23_Pal,
    [WALLPAPER_SPECIAL_8] = sHgssStorageWallpaper24_Pal,
};

// Authentic HGSS Choose Box navigation controls.
// /a/0/1/9 members 66-69; four verified 24x24 frames preserved 1:1.
static const u32 sHgssChooseBoxNav_Gfx[] = INCGFX_U32("graphics/gen4_ui/hgss_storage/verified/choose_box_nav.png", ".4bpp");
static const u16 sHgssChooseBoxNav_Pal[] = INCGFX_U16("graphics/gen4_ui/hgss_storage/verified/choose_box_nav.png", ".gbapal");

// Authentic HGSS box-overview thumbnail base.
// /a/0/1/9 members 70-73; verified NCER cell is exactly 32x32.
static const u32 sHgssBoxThumbnailBase_Gfx[] = INCGFX_U32("graphics/gen4_ui/hgss_storage/verified/box_thumbnail_base.png", ".4bpp");
static const u16 sHgssBoxThumbnailBase_Pal[] = INCGFX_U16("graphics/gen4_ui/hgss_storage/verified/box_thumbnail_base.png", ".gbapal");

// Authentic HGSS normal Storage hand pointer: NANR animation 14,
// NCER cells 13 and 14, 20 ticks per frame.
static const u32 sHgssStorageCursor_Gfx[] = INCGFX_U32("graphics/gen4_ui/hgss_storage/verified/cursor.png", ".4bpp");
static const u16 sHgssStorageCursor_Pal[] = INCGFX_U16("graphics/gen4_ui/hgss_storage/verified/cursor.png", ".gbapal");

static const u32 sGenderIcons_Gfx[]           = INCGFX_U32("graphics/pokemon_storage/swsh/gender_icons.png", ".4bpp.smol");
static const u16 sMarkings_Pal[]              = INCGFX_U16("graphics/pokemon_storage/swsh/markings.pal", ".gbapal");

// Authentic HGSS markings panel: /a/0/1/9 NSCR 10 + NCGR 14 + NCLR 4.
static const u32 sHgssMarkingsMenu_Gfx[]       = INCGFX_U32("graphics/gen4_ui/hgss_storage/verified/markings_menu.png", ".4bpp");
static const u16 sHgssMarkingsMenu_Pal[]       = INCGFX_U16("graphics/gen4_ui/hgss_storage/verified/markings_menu.png", ".gbapal");

// Authentic /a/0/1/9 member 4 NCLR bank 3, reordered only to the verified
// markings_menu.png palette indices. This is the active-mark button state.
static const u16 sHgssMarkingsMenuSelected_Pal[16] =
{
    0x5790, 0x4E94, 0x356B, 0x2D57,
    0x41FC, 0x4E94, 0x4E94, 0x39EF,
    0x294A, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000,
};
static const u32 sShinyIcon_Gfx[]             = INCGFX_U32("graphics/pokemon_storage/swsh/shiny_icon.png", ".4bpp.smol");
static const u32 sPokerusIcon_Gfx[]           = INCGFX_U32("graphics/pokemon_storage/swsh/pokerus_icon.png", ".4bpp.smol");
static const u32 sStatLabels_Gfx[]            = INCGFX_U32("graphics/pokemon_storage/swsh/stat_labels.png", ".4bpp.smol");
static const u16 sStatLabels_Pal[]             = INCGFX_U16("graphics/pokemon_storage/swsh/stat_labels.png", ".gbapal");
static const ALIGNED(4) u8 sTypeIcons_Gfx[]   = INCGFX_U8("graphics/pokemon_storage/swsh/type_icons.png", ".4bpp");
static const u16 sTypeIcons_Pal[]             = INCGFX_U16("graphics/pokemon_storage/swsh/type_icons.png", ".gbapal");

// ============================================================================
// Text Strings
// ============================================================================

static const u8 gText_JustOnePkmn[]   = _("There is just one Pokémon with you.");
static const u8 gText_PartyFull[]     = _("Your party is full!");
static const u8 sText_Lv[]            = _("Lv");
static const u8 gPCText_Give[]        = _("Give");

struct {
    const u8 *text;
    const u8 *desc;
} static const sMainMenuTexts[OPTIONS_COUNT] =
{
    [OPTION_WITHDRAW]   = {COMPOUND_STRING("Withdraw Pokémon"), COMPOUND_STRING("Move Pokémon stored in boxes to\nyour party.")},
    [OPTION_DEPOSIT]    = {COMPOUND_STRING("Deposit Pokémon"),  COMPOUND_STRING("Store Pokémon in your party in boxes.")},
    [OPTION_MOVE_MONS]  = {COMPOUND_STRING("Move Pokémon"),     COMPOUND_STRING("Organize the Pokémon in boxes and\nin your party.")},
    [OPTION_MOVE_ITEMS] = {COMPOUND_STRING("Move Items"),       COMPOUND_STRING("Move items held by any Pokémon\nin a box or your party.")},
    [OPTION_EXIT]       = {COMPOUND_STRING("See ya!"),          COMPOUND_STRING("Return to the previous menu.")}
};

static const u8 *const sMenuTexts[] =
{
    [MENU_CANCEL]     = COMPOUND_STRING("Cancel"),
    [MENU_STORE]      = COMPOUND_STRING("Store"),
    [MENU_WITHDRAW]   = COMPOUND_STRING("Withdraw"),
    [MENU_MOVE]       = COMPOUND_STRING("Move"),
    [MENU_SHIFT]      = COMPOUND_STRING("Shift"),
    [MENU_PLACE]      = COMPOUND_STRING("Place"),
    [MENU_SUMMARY]    = COMPOUND_STRING("Summary"),
    [MENU_RELEASE]    = COMPOUND_STRING("Release"),
    [MENU_MARK]       = COMPOUND_STRING("Mark"),
    [MENU_JUMP]       = COMPOUND_STRING("Jump"),
    [MENU_WALLPAPER]  = COMPOUND_STRING("Wallpaper"),
    [MENU_NAME]       = COMPOUND_STRING("Name"),
    [MENU_TAKE]       = COMPOUND_STRING("Take"),
    [MENU_GIVE]       = gPCText_Give,
    [MENU_GIVE_2]     = gPCText_Give,
    [MENU_SWITCH]     = COMPOUND_STRING("Switch"),
    [MENU_BAG]        = COMPOUND_STRING("Bag"),
    [MENU_SELECT]     = COMPOUND_STRING("Select"),
    // Authentic HGSS normal wallpaper order: /a/0/1/9 members 16-31.
    [MENU_BASE]       = COMPOUND_STRING("Forest"),
    [MENU_PLAINS]     = COMPOUND_STRING("City"),
    [MENU_CITY]       = COMPOUND_STRING("Desert"),
    [MENU_DESERT]     = COMPOUND_STRING("Savanna"),
    [MENU_CENTER]     = COMPOUND_STRING("Crag"),
    [MENU_SHORE]      = COMPOUND_STRING("Volcano"),
    [MENU_OCEAN]      = COMPOUND_STRING("Snow"),
    [MENU_MOUNTAIN]   = COMPOUND_STRING("Cave"),
    [MENU_VOLCANO]    = COMPOUND_STRING("Beach"),
    [MENU_CAVE]       = COMPOUND_STRING("Seafloor"),
    [MENU_BEACH]      = COMPOUND_STRING("River"),
    [MENU_SNOW]       = COMPOUND_STRING("Sky"),
    [MENU_SKY]        = COMPOUND_STRING("Polkadot"),
    [MENU_COMPUTA]    = COMPOUND_STRING("PokéCenter"),
    [MENU_CUTE]       = COMPOUND_STRING("Machine"),
    [MENU_SPACE]      = COMPOUND_STRING("Simple"),
    // The final eight are authenticated special HGSS wallpapers. Their art is
    // wired in native order; neutral labels avoid inventing unsupported names.
    [MENU_DAYCARE]    = COMPOUND_STRING("Special 1"),
    [MENU_CONTEST]    = COMPOUND_STRING("Special 2"),
    [MENU_CLASSIC]    = COMPOUND_STRING("Special 3"),
    [MENU_CLASSIC2]   = COMPOUND_STRING("Special 4"),
    [MENU_SPECIAL_5]  = COMPOUND_STRING("Special 5"),
    [MENU_SPECIAL_6]  = COMPOUND_STRING("Special 6"),
    [MENU_SPECIAL_7]  = COMPOUND_STRING("Special 7"),
    [MENU_SPECIAL_8]  = COMPOUND_STRING("Special 8"),
    [MENU_COUNT]      = gText_EmptyString2,
};

// ============================================================================
// Messages
// ============================================================================

static const struct StorageMessage sMessages[] =
{
    [MSG_BOX_IS_FULL]          = {COMPOUND_STRING("The box is full."),           MSG_VAR_NONE},
    [MSG_RELEASE_POKE]         = {COMPOUND_STRING("Release this Pokémon?"),      MSG_VAR_NONE},
    [MSG_WAS_RELEASED]         = {COMPOUND_STRING("{DYNAMIC 0} was released."),  MSG_VAR_RELEASE_MON_1},
    [MSG_BYE_BYE]              = {COMPOUND_STRING("Bye-bye, {DYNAMIC 0}!"),      MSG_VAR_RELEASE_MON_3},
    [MSG_LAST_POKE]            = {COMPOUND_STRING("That's your last Pokémon!"),  MSG_VAR_NONE},
    [MSG_PARTY_FULL]           = {gText_YourPartysFull,                          MSG_VAR_NONE},
    [MSG_HOLDING_POKE]         = {COMPOUND_STRING("You're holding a Pokémon!"),  MSG_VAR_NONE},
    [MSG_WHICH_ONE_WILL_TAKE]  = {COMPOUND_STRING("Which one will you take?"),   MSG_VAR_NONE},
    [MSG_CANT_RELEASE_EGG]     = {COMPOUND_STRING("You can't release an egg."),  MSG_VAR_NONE},
    [MSG_CONTINUE_BOX]         = {COMPOUND_STRING("Continue P.C. operations?"),  MSG_VAR_NONE},
    [MSG_CAME_BACK]            = {COMPOUND_STRING("{DYNAMIC 0} came back!"),     MSG_VAR_MON_NAME_1},
    [MSG_WORRIED]              = {COMPOUND_STRING("Was it worried about you?"),  MSG_VAR_NONE},
    [MSG_SURPRISE]             = {COMPOUND_STRING("… … … … !"),                  MSG_VAR_NONE},
    [MSG_PLEASE_REMOVE_MAIL]   = {COMPOUND_STRING("Please remove the mail."),    MSG_VAR_NONE},
    [MSG_PLACED_IN_BAG]        = {COMPOUND_STRING("Placed item in the bag."),    MSG_VAR_ITEM_NAME},
    [MSG_BAG_FULL]             = {COMPOUND_STRING("The bag is full."),           MSG_VAR_NONE},
    [MSG_PUT_IN_BAG]           = {COMPOUND_STRING("Put this item in the bag?"),  MSG_VAR_NONE},
    [MSG_CANT_STORE_MAIL]      = {COMPOUND_STRING("Mail can't be stored!"),      MSG_VAR_NONE},
};

// ============================================================================
// Window Templates
// ============================================================================

static const struct WindowTemplate sWindowTemplate_MainMenu =
{
    .bg = 0,
    .tilemapLeft = 1,
    .tilemapTop = 1,
    .width = 17,
    .height = 10,
    .paletteNum = 15,
    .baseBlock = 0x1,
};

static const struct WindowTemplate sYesNoWindowTemplate =
{
    .bg = 0,
    .tilemapLeft = 24,
    .tilemapTop = 14,
    .width = 5,
    .height = 4,
    .paletteNum = 10,
    .baseBlock = 0x5C,
};

static const struct WindowTemplate sWindowTemplate_MultiMove =
{
    .bg = 0,
    .tilemapLeft = 10,
    .tilemapTop = 3,
    .width = 19,
    .height = 16,
    .paletteNum = 9,
    .baseBlock = 0x1,
};

static const struct WindowTemplate sWindowTemplates[] =
{
    [WIN_MESSAGE] = {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 16,
        .width = 28,
        .height = 2,
        .paletteNum = 10,
        .baseBlock = 44,
    },
    [WIN_ITEM_DESC] = {
        .bg = 0,
        .tilemapLeft = 0,
        .tilemapTop = 13,
        .width = 21,
        .height = 7,
        .paletteNum = 15,
        .baseBlock = 44,
    },
    [WIN_MON_INFO_NICKNAME_LEFT] = {
        .bg = 0,
        .tilemapLeft = 0,
        .tilemapTop = 23,
        .width = 7,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 202
    },
    [WIN_MON_INFO_LEVEL_LEFT] = {
        .bg = 0,
        .tilemapLeft = 7,
        .tilemapTop = 23,
        .width = 4,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 218
    },
    [WIN_MON_INFO_STATS_COL1_LEFT] = {
        .bg = 0,
        .tilemapLeft = 3,
        .tilemapTop = 27,
        .width = 3,
        .height = 6,
        .paletteNum = 15,
        .baseBlock = 226
    },
    [WIN_MON_INFO_STATS_COL2_LEFT] = {
        .bg = 0,
        .tilemapLeft = 8,
        .tilemapTop = 27,
        .width = 3,
        .height = 6,
        .paletteNum = 15,
        .baseBlock = 244
    },
    [WIN_MON_INFO_ABILITY_LEFT] = {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 34,
        .width = 10,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 262
    },
    [WIN_MON_INFO_ITEM_LEFT] = {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 36,
        .width = 10,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 280
    },
    [WIN_MON_INFO_NICKNAME_RIGHT] = {
        .bg = 0,
        .tilemapLeft = 17,
        .tilemapTop = 43,
        .width = 7,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 202
    },
    [WIN_MON_INFO_LEVEL_RIGHT] = {
        .bg = 0,
        .tilemapLeft = 24,
        .tilemapTop = 43,
        .width = 4,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 218
    },
    [WIN_MON_INFO_STATS_COL1_RIGHT] = {
        .bg = 0,
        .tilemapLeft = 20,
        .tilemapTop = 47,
        .width = 3,
        .height = 6,
        .paletteNum = 15,
        .baseBlock = 226
    },
    [WIN_MON_INFO_STATS_COL2_RIGHT] = {
        .bg = 0,
        .tilemapLeft = 25,
        .tilemapTop = 47,
        .width = 3,
        .height = 6,
        .paletteNum = 15,
        .baseBlock = 244
    },
    [WIN_MON_INFO_ABILITY_RIGHT] = {
        .bg = 0,
        .tilemapLeft = 18,
        .tilemapTop = 54,
        .width = 10,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 262
    },
    [WIN_MON_INFO_ITEM_RIGHT] = {
        .bg = 0,
        .tilemapLeft = 18,
        .tilemapTop = 56,
        .width = 10,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 280
    },
    DUMMY_WIN_TEMPLATE
};

static const u8 sTextColors[][3] =
{
    {1, 2, 3}, // Standard menus, mon info (stats, ability, item)
    {4, 2, 5}, // Mon info (nickname and level) (grey BG)
    {0, 4, 7}, // Choose box menu - actually uses PALTAG_MISC_3 and not bg pal 15
    {0, 2, 3}, // HGSS Storage message window (cream / dark / gray)
};


// ============================================================================
// BG Templates
// ============================================================================

static const struct BgTemplate sBgTemplates[] =
{
    {
        .bg = 0,
        .charBaseIndex = 0,
        .mapBaseIndex = 29,
        .screenSize = 2,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0
    },
    {
        .bg = 1,
        .charBaseIndex = 2,
        .mapBaseIndex = 26,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0
    },
    {
        .bg = 2,
        .charBaseIndex = 2,
        .mapBaseIndex = 27,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 2,
        .baseTile = 0
    },
    {
        .bg = 3,
        .charBaseIndex = 3,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 3,
        .baseTile = 0
    },
};

// ============================================================================
// Authentic HGSS Choose Box Navigation Controls
// ============================================================================

static const struct OamData sOamData_HgssChooseBoxNav =
{
    .shape = SPRITE_SHAPE(32x32),
    .size = SPRITE_SIZE(32x32),
    .priority = 1,
};

static const union AnimCmd sAnim_HgssChooseBoxNav_LeftNormal[] =
{
    ANIMCMD_FRAME(0, 0),
    ANIMCMD_END
};

static const union AnimCmd sAnim_HgssChooseBoxNav_LeftPressed[] =
{
    ANIMCMD_FRAME(16, 4),
    ANIMCMD_FRAME(0, 0),
    ANIMCMD_END
};

static const union AnimCmd sAnim_HgssChooseBoxNav_RightNormal[] =
{
    ANIMCMD_FRAME(32, 0),
    ANIMCMD_END
};

static const union AnimCmd sAnim_HgssChooseBoxNav_RightPressed[] =
{
    ANIMCMD_FRAME(48, 4),
    ANIMCMD_FRAME(32, 0),
    ANIMCMD_END
};

static const union AnimCmd *const sAnims_HgssChooseBoxNav[] =
{
    sAnim_HgssChooseBoxNav_LeftNormal,
    sAnim_HgssChooseBoxNav_LeftPressed,
    sAnim_HgssChooseBoxNav_RightNormal,
    sAnim_HgssChooseBoxNav_RightPressed,
};

static const struct SpriteTemplate sSpriteTemplate_HgssChooseBoxNav =
{
    .tileTag = GFXTAG_HGSS_CHOOSE_BOX_NAV,
    .paletteTag = PALTAG_HGSS_CHOOSE_BOX_NAV,
    .oam = &sOamData_HgssChooseBoxNav,
    .anims = sAnims_HgssChooseBoxNav,
};

// ============================================================================
// Authentic HGSS Box-Overview Thumbnail Base
// ============================================================================

static const struct OamData sOamData_HgssBoxThumbnail =
{
    .shape = SPRITE_SHAPE(32x32),
    .size = SPRITE_SIZE(32x32),
    .priority = 1,
};

static const union AnimCmd sAnim_HgssBoxThumbnail[] =
{
    ANIMCMD_FRAME(0, 4),
    ANIMCMD_END
};

static const union AnimCmd *const sAnims_HgssBoxThumbnail[] =
{
    sAnim_HgssBoxThumbnail,
};

static const struct SpriteTemplate sSpriteTemplate_HgssBoxThumbnail =
{
    .tileTag = GFXTAG_HGSS_BOX_THUMBNAIL,
    .paletteTag = PALTAG_HGSS_BOX_THUMBNAIL,
    .oam = &sOamData_HgssBoxThumbnail,
    .anims = sAnims_HgssBoxThumbnail,
};

// ============================================================================
// Authentic HGSS Wallpaper Selector Swatches
// ============================================================================

static const struct OamData sOamData_HgssWallpaperSelector =
{
    .shape = SPRITE_SHAPE(32x32),
    .size = SPRITE_SIZE(32x32),
    .priority = 0,
};

static const union AnimCmd sAnim_HgssWallpaperSelector0[] =
{
    ANIMCMD_FRAME(0, 0),
    ANIMCMD_END
};

static const union AnimCmd sAnim_HgssWallpaperSelector1[] =
{
    ANIMCMD_FRAME(16, 0),
    ANIMCMD_END
};

static const union AnimCmd sAnim_HgssWallpaperSelector2[] =
{
    ANIMCMD_FRAME(32, 0),
    ANIMCMD_END
};

static const union AnimCmd sAnim_HgssWallpaperSelector3[] =
{
    ANIMCMD_FRAME(48, 0),
    ANIMCMD_END
};

static const union AnimCmd *const sAnims_HgssWallpaperSelector[] =
{
    sAnim_HgssWallpaperSelector0,
    sAnim_HgssWallpaperSelector1,
    sAnim_HgssWallpaperSelector2,
    sAnim_HgssWallpaperSelector3,
};

static const struct SpriteTemplate sSpriteTemplate_HgssWallpaperSelector =
{
    .tileTag = GFXTAG_HGSS_WALLPAPER_SELECTOR,
    .paletteTag = PALTAG_HGSS_WALLPAPER_SELECTOR,
    .oam = &sOamData_HgssWallpaperSelector,
    .anims = sAnims_HgssWallpaperSelector,
};

// ============================================================================
// Box Selection Mon Count Sprite (loaded dynamically)
// ============================================================================

static const struct OamData sOamData_ChooseBoxMenu_MonCount =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .size = SPRITE_SIZE(16x16),
    .x = 0,
    .matrixNum = 0,
    .shape = SPRITE_SHAPE(16x16),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
    .affineParam = 0,
};

static const struct SpriteTemplate sSpriteTemplate_ChooseBoxMenu_MonCount =
{
    .tileTag = GFXTAG_BOX_SELECTION_PER_30,
    .paletteTag = PALTAG_MISC_3,
    .oam = &sOamData_ChooseBoxMenu_MonCount,
};

// ============================================================================
// Box Title Sprites
// ============================================================================

static const u16 sUnusedColor = RGB(26, 29, 8);

#define BOX_TITLE_FRAME_MAIN    RGB_WHITE
#define BOX_TITLE_TEXT_MAIN     RGB(29, 29, 29)
#define BOX_TITLE_SHADOW_MAIN   RGB(5, 5, 5)

#define BOX_TITLE_FRAME_HOVER   RGB(5, 5, 5)
#define BOX_TITLE_TEXT_HOVER    RGB_WHITE
#define BOX_TITLE_SHADOW_HOVER  RGB(14, 14, 14)

static const struct OamData sOamData_BoxTitle =
{
    .shape = SPRITE_SHAPE(32x16),
    .size = SPRITE_SIZE(32x16),
    .priority = 2
};

static const union AnimCmd sAnim_BoxTitle_Left[] =
{
    ANIMCMD_FRAME(0, 5),
    ANIMCMD_END
};

static const union AnimCmd sAnim_BoxTitle_Right[] =
{
    ANIMCMD_FRAME(8, 5),
    ANIMCMD_END
};

static const union AnimCmd *const sAnims_BoxTitle[] =
{
    sAnim_BoxTitle_Left,
    sAnim_BoxTitle_Right
};

static const struct SpriteTemplate sSpriteTemplate_BoxTitle =
{
    .tileTag = GFXTAG_BOX_TITLE,
    .paletteTag = PALTAG_MISC_1,
    .oam = &sOamData_BoxTitle,
    .anims = sAnims_BoxTitle,
};

// ============================================================================
// Cursor Sprites
// ============================================================================

// PALTAG_MISC_1/2/3 still serve unrelated stat-label/title roles. Their
// colors now come from the stat-label asset itself, not from the removed
// legacy cursor sheet.
static const struct SpritePalette sSpritePal_StatLabels[] =
{
    {
        .data = sStatLabels_Pal,
        .tag = PALTAG_MISC_1,
    },
    {
        .data = sStatLabels_Pal + 16,
        .tag = PALTAG_MISC_2,
    },
    {
        .data = sStatLabels_Pal + 32,
        .tag = PALTAG_MISC_3,
    },
    {},
};

static const struct OamData sOamData_Cursor =
{
    .shape = SPRITE_SHAPE(32x32),
    .size = SPRITE_SIZE(32x32),
    .priority = 1,
};

// HGSS NANR animation 14: cells 13 -> 14, 20 ticks each, looping.
// Both existing pokeemerald cursor states use the native HGSS motion so no
// legacy cursor frame is displayed during movement or normal interaction.
static const union AnimCmd sAnim_Cursor_Bouncing[] =
{
    ANIMCMD_FRAME(0, 20),
    ANIMCMD_FRAME(16, 20),
    ANIMCMD_JUMP(0)
};

static const union AnimCmd sAnim_Cursor_Main[] =
{
    ANIMCMD_FRAME(0, 20),
    ANIMCMD_FRAME(16, 20),
    ANIMCMD_JUMP(0)
};

static const union AnimCmd *const sAnims_Cursor[] =
{
    [CURSOR_ANIM_BOUNCE] = sAnim_Cursor_Bouncing,
    [CURSOR_ANIM_MAIN]   = sAnim_Cursor_Main,
};

static const struct SpriteTemplate sSpriteTemplate_Cursor =
{
    .tileTag = GFXTAG_CURSOR,
    .paletteTag = PALTAG_HGSS_STORAGE_CURSOR,
    .oam = &sOamData_Cursor,
    .anims = sAnims_Cursor,
};

// ============================================================================
// Mon Icon Sprites
// ============================================================================

static const struct OamData sOamData_MonIcon;
static const struct SpriteTemplate sSpriteTemplate_MonIcon =
{
    .tileTag = GFXTAG_MON_ICON,
    .paletteTag = PALTAG_MON_ICON_0,
    .oam = &sOamData_MonIcon,
};

static const struct OamData sOamData_MonIcon =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(32x32),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(32x32),
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 0,
    .affineParam = 0
};

static const union AffineAnimCmd sAffineAnim_ReleaseMon_Release[] =
{
    AFFINEANIMCMD_FRAME(-2, -2, 0, 120),
    AFFINEANIMCMD_END
};

static const union AffineAnimCmd sAffineAnim_ReleaseMon_CameBack[] =
{
    AFFINEANIMCMD_FRAME(16, 16, 0, 0),
    AFFINEANIMCMD_FRAME(16, 16, 0, 15),
    AFFINEANIMCMD_END
};

static const union AffineAnimCmd *const sAffineAnims_ReleaseMon[] =
{
    [RELEASE_ANIM_RELEASE]   = sAffineAnim_ReleaseMon_Release,
    [RELEASE_ANIM_CAME_BACK] = sAffineAnim_ReleaseMon_CameBack
};

// ============================================================================
// Gender Icon Sprites
// ============================================================================

static const struct OamData sOamData_GenderIcons =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(8x16),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(8x16),
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 7,
    .affineParam = 0,
};

static const union AnimCmd sSpriteAnim_GenderFemale[] = {
    ANIMCMD_FRAME(0, 0, FALSE, FALSE),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_GenderMale[] = {
    ANIMCMD_FRAME(2, 0, FALSE, FALSE),
    ANIMCMD_END
};

static const union AnimCmd *const sSpriteAnimTable_GenderIcons[] = {
    sSpriteAnim_GenderFemale,
    sSpriteAnim_GenderMale,
};

static const struct CompressedSpriteSheet sSpriteSheet_GenderIcons =
{
    .data = sGenderIcons_Gfx,
    .size = (8 * 16 * 2) / 2,
    .tag = GFXTAG_GENDER_ICON
};

static const struct SpriteTemplate sSpriteTemplate_GenderIcons =
{
    .tileTag = GFXTAG_GENDER_ICON,
    .paletteTag = PALTAG_MISC_2,
    .oam = &sOamData_GenderIcons,
    .anims = sSpriteAnimTable_GenderIcons,
    .images = NULL,
};

// ============================================================================
// Shiny Icon Sprites
// ============================================================================

static const struct OamData sOamData_ShinyIcon =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(8x8),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(8x8),
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 0,
    .affineParam = 0,
};

static const struct CompressedSpriteSheet sSpriteSheet_ShinyIcon =
{
    .data = sShinyIcon_Gfx,
    .size = (8 * 8) / 2,
    .tag = GFXTAG_SHINY_ICON
};

static const struct SpriteTemplate sSpriteTemplate_ShinyIcon =
{
    .tileTag = GFXTAG_SHINY_ICON,
    .paletteTag = PALTAG_MISC_2,
    .oam = &sOamData_ShinyIcon,
};

// ============================================================================
// Pokerus Icon Sprites
// ============================================================================

static const struct OamData sOamData_PokerusIcon =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(32x8),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(32x8),
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 0,
    .affineParam = 0,
};

static const struct CompressedSpriteSheet sSpriteSheet_PokerusIcon =
{
    .data = sPokerusIcon_Gfx,
    .size = (32 * 8) / 2,
    .tag = GFXTAG_PKRS_ICON
};

static const struct SpriteTemplate sSpriteTemplate_PokerusIcon =
{
    .tileTag = GFXTAG_PKRS_ICON,
    .paletteTag = PALTAG_MISC_1,
    .oam = &sOamData_PokerusIcon,
	.anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};

// ============================================================================
// Stat Label Sprites
// ============================================================================

static const struct OamData sOamData_StatLabels =
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
};

static const union AnimCmd sSpriteAnim_StatAtk[] = {
    ANIMCMD_FRAME(0, 0, FALSE, FALSE),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_StatDef[] = {
    ANIMCMD_FRAME(4, 0, FALSE, FALSE),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_StatSpAtk[] = {
    ANIMCMD_FRAME(8, 0, FALSE, FALSE),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_StatSpDef[] = {
    ANIMCMD_FRAME(12, 0, FALSE, FALSE),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_StatSpeed[] = {
    ANIMCMD_FRAME(16, 0, FALSE, FALSE),
    ANIMCMD_END
};

static const union AnimCmd *const sSpriteAnimTable_StatLabels[] = {
    sSpriteAnim_StatAtk,
    sSpriteAnim_StatDef,
    sSpriteAnim_StatSpAtk,
    sSpriteAnim_StatSpDef,
    sSpriteAnim_StatSpeed,
};

static const struct CompressedSpriteSheet sSpriteSheet_StatLabels =
{
    .data = sStatLabels_Gfx,
    .size = (16 * 16 * 5) / 2,
    .tag = GFXTAG_STAT_LABELS
};

static const struct SpriteTemplate sSpriteTemplate_StatLabels =
{
    .tileTag = GFXTAG_STAT_LABELS,
    .paletteTag = PALTAG_MISC_2,
    .oam = &sOamData_StatLabels,
    .anims = sSpriteAnimTable_StatLabels,
};

// ============================================================================
// Type Icon Sprites
// ============================================================================

static const struct OamData sOamData_TypeIcons =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(32x16),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(32x16),
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 0,
    .affineParam = 0,
};

// Type icons now use DMA copy instead of animations to save VRAM (only loads 2 slots instead of all 21 types)
// Uncompressed sprite sheet (only 2 slots loaded to save VRAM)
static const struct SpriteSheet sSpriteSheet_TypeIcons =
{
    .data = sTypeIcons_Gfx,
    .size = 2 * 0x100, // Only load 2 type icon slots (saves 4.75 KB VRAM)
    .tag = GFXTAG_TYPE_ICON,
};

static const struct SpriteTemplate sSpriteTemplate_TypeIcons =
{
    .tileTag = GFXTAG_TYPE_ICON,
    .paletteTag = PALTAG_TYPE_ICON,
    .oam = &sOamData_TypeIcons,
};

// ============================================================================
// Item Icon Sprites
// ============================================================================

static const struct OamData sOamData_ItemIcon =
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

static const union AffineAnimCmd sAffineAnim_ItemIcon_Small[] =
{
    AFFINEANIMCMD_FRAME(192, 192, 0, 0),
    AFFINEANIMCMD_END
};

static const union AffineAnimCmd sAffineAnim_ItemIcon_Appear[] =
{
    AFFINEANIMCMD_FRAME(152, 152, 0, 0),
    AFFINEANIMCMD_FRAME(5, 5, 0, 8),
    AFFINEANIMCMD_END
};

static const union AffineAnimCmd sAffineAnim_ItemIcon_Disappear[] =
{
    AFFINEANIMCMD_FRAME(192, 192, 0, 0),
    AFFINEANIMCMD_FRAME(-5, -5, 0, 8),
    AFFINEANIMCMD_END
};

static const union AffineAnimCmd sAffineAnim_ItemIcon_PickUp[] =
{
    AFFINEANIMCMD_FRAME(192, 192, 0, 0),
    AFFINEANIMCMD_FRAME(6, 6, 0, 12),
    AFFINEANIMCMD_FRAME(256, 256, 0, 0),
    AFFINEANIMCMD_END
};

static const union AffineAnimCmd sAffineAnim_ItemIcon_PutDown[] =
{
    AFFINEANIMCMD_FRAME(256, 256, 0, 0),
    AFFINEANIMCMD_FRAME(-6, -6, 0, 12),
    AFFINEANIMCMD_FRAME(192, 192, 0, 0),
    AFFINEANIMCMD_END
};

static const union AffineAnimCmd sAffineAnim_ItemIcon_PutAway[] =
{
    AFFINEANIMCMD_FRAME(256, 256, 0, 0),
    AFFINEANIMCMD_FRAME(-5, -5, 0, 16),
    AFFINEANIMCMD_END
};

static const union AffineAnimCmd sAffineAnim_ItemIcon_Large[] =
{
    AFFINEANIMCMD_FRAME(256, 256, 0, 0),
    AFFINEANIMCMD_END
};

static const union AffineAnimCmd *const sAffineAnims_ItemIcon[] =
{
    [ITEM_ANIM_NONE]      = sAffineAnim_ItemIcon_Small,
    [ITEM_ANIM_APPEAR]    = sAffineAnim_ItemIcon_Appear,
    [ITEM_ANIM_DISAPPEAR] = sAffineAnim_ItemIcon_Disappear,
    [ITEM_ANIM_PICK_UP]   = sAffineAnim_ItemIcon_PickUp,
    [ITEM_ANIM_PUT_DOWN]  = sAffineAnim_ItemIcon_PutDown,
    [ITEM_ANIM_PUT_AWAY]  = sAffineAnim_ItemIcon_PutAway,
    [ITEM_ANIM_LARGE]     = sAffineAnim_ItemIcon_Large
};

static const struct SpriteTemplate sSpriteTemplate_ItemIcon =
{
    .tileTag = GFXTAG_ITEM_ICON_0,
    .paletteTag = PALTAG_ITEM_ICON_0,
    .oam = &sOamData_ItemIcon,
    .affineAnims = sAffineAnims_ItemIcon,
};
