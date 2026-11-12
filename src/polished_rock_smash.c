#include "global.h"
#include "polished_rock_smash.h"
#include "polished_pokemon_data.h"
#include "battle_setup.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "field_camera.h"
#include "field_player_avatar.h"
#include "fldeff.h"
#include "item.h"
#include "item_icon.h"
#include "gpu_regs.h"
#include "palette.h"
#include "sprite.h"
#include "overworld.h"
#include "party_menu.h"
#include "random.h"
#include "script.h"
#include "script_pokemon_util.h"
#include "sound.h"
#include "string_util.h"
#include "task.h"
#include "constants/event_objects.h"
#include "constants/maps.h"
#include "constants/songs.h"
#include "constants/vars.h"

struct RockMon { u8 weight; enum Species species; u8 level; };
struct RockItem { u8 weight; enum Item item; };
#include "data/polished_rock_smash.h"

bool32 PolishedRockSmashKnowsMove(struct Pokemon *mon)
{
    if (GetMonData(mon, MON_DATA_SPECIES) == SPECIES_NONE || GetMonData(mon, MON_DATA_IS_EGG))
        return FALSE;
    for (u32 i = 0; i < MAX_MON_MOVES; i++)
        if (GetMonData(mon, MON_DATA_MOVE1 + i) == MOVE_BRICK_BREAK)
            return TRUE;
    return FALSE;
}

u8 PolishedRockSmashPartyUser(struct Pokemon *party, u8 count, bool32 ownsTM)
{
    count = min(count, PARTY_SIZE);
    // The source completes the entire known-move scan before checking the TM.
    for (u32 i = 0; i < count; i++)
        if (PolishedRockSmashKnowsMove(&party[i]))
            return i;
    if (ownsTM)
        for (u32 i = 0; i < count; i++)
        {
            if (GetMonData(&party[i], MON_DATA_IS_EGG))
                continue;
            const u16 *moves = PolishedGetTeachableLearnset(GetMonData(&party[i], MON_DATA_SPECIES));
            if (moves != NULL)
                for (; *moves != MOVE_UNAVAILABLE; moves++)
                    if (*moves == MOVE_BRICK_BREAK)
                        return i;
        }
    return PARTY_SIZE;
}

bool32 PolishedRockSmashMap(u8 group, u8 num)
{
    return group == MAP_GROUP(MAP_POLISHED_DARK_CAVE_VIOLET_ENTRANCE)
        && num == MAP_NUM(MAP_POLISHED_DARK_CAVE_VIOLET_ENTRANCE);
}

static bool32 CurrentRock(u8 localId)
{
    if (!PolishedRockSmashMap(gSaveBlock1Ptr->location.mapGroup, gSaveBlock1Ptr->location.mapNum))
        return FALSE;
    u8 id = GetObjectEventIdByLocalIdAndMap(localId, gSaveBlock1Ptr->location.mapNum, gSaveBlock1Ptr->location.mapGroup);
    return localId >= 5 && localId <= 8 && id < OBJECT_EVENTS_COUNT
        && gObjectEvents[id].active && gObjectEvents[id].graphicsId == OBJ_EVENT_GFX_POLISHED_ROCK;
}

static void PrepareMon(u8 slot)
{
    GetMonData(&gParties[B_TRAINER_PLAYER][slot], MON_DATA_NICKNAME, gStringVar1);
    StringGet_Nickname(gStringVar1);
    gSpecialVar_0x8004 = GetMonData(&gParties[B_TRAINER_PLAYER][slot], MON_DATA_SPECIES);
}

u16 PolishedRockSmashPrepare(void)
{
    if (!CurrentRock(gSpecialVar_LastTalked))
        return FALSE;
    u8 slot = PolishedRockSmashPartyUser(gParties[B_TRAINER_PLAYER], gPartiesCount[B_TRAINER_PLAYER], CheckBagHasItem(ITEM_TM_BRICK_BREAK, 1));
    if (slot == PARTY_SIZE)
        return FALSE;
    PrepareMon(slot);
    return TRUE;
}

static void MenuCallback(void)
{
    PrepareMon(GetCursorSelectionMonId());
    ScriptContext_SetupScript(PolishedRockSmash_Menu);
}

bool32 PolishedRockSmashFromMenu(void)
{
    if (!PolishedRockSmashKnowsMove(&gParties[B_TRAINER_PLAYER][GetCursorSelectionMonId()]))
        return FALSE;
    s16 x, y;
    GetXYCoordsOneStepInFrontOfPlayer(&x, &y);
    u8 id = GetObjectEventIdByPosition(x, y, PlayerGetElevation());
    if (id >= OBJECT_EVENTS_COUNT || !CurrentRock(gObjectEvents[id].localId))
        return FALSE;
    gSpecialVar_LastTalked = gObjectEvents[id].localId;
    gFieldCallback2 = FieldCallback_PrepareFadeInFromMenu;
    gPostMenuFieldCallback = MenuCallback;
    return TRUE;
}

bool32 PolishedRockSmashBattleRoll(u8 roll)
{
    return roll < POLISHED_ROCK_GATE_HITS;
}

enum Species PolishedRockSmashSpecies(u8 roll, u8 *level)
{
    for (u32 i = 0; i < ARRAY_COUNT(sRockMons); i++)
    {
        if (roll < sRockMons[i].weight)
        {
            *level = sRockMons[i].level;
            return sRockMons[i].species;
        }
        roll -= sRockMons[i].weight;
    }
    return SPECIES_NONE;
}

enum Item PolishedRockSmashItem(u8 roll)
{
    for (u32 i = 0; i < ARRAY_COUNT(sRockItems); i++)
    {
        if (roll < sRockItems[i].weight)
            return sRockItems[i].item;
        roll -= sRockItems[i].weight;
    }
    return ITEM_NONE;
}

// Match RandomRange's byte rejection rather than the native multiply-scaled
// range helper (which gives a small bias for divisors of neither 256 nor 65536).
static u8 SourceRandomRange(u8 range)
{
    u8 roll;
    u16 limit = 256 - 256 % range;
    do { roll = Random() & 0xff; } while (roll >= limit);
    return roll % range;
}

bool32 PolishedRockSmashRollOutcome(enum Species *species, u8 *level, enum Item *item)
{
    *species = SPECIES_NONE;
    *level = 0;
    *item = ITEM_NONE;
    if (PolishedRockSmashBattleRoll(SourceRandomRange(POLISHED_ROCK_GATE_RANGE)))
    {
        *species = PolishedRockSmashSpecies(SourceRandomRange(100), level);
        return TRUE;
    }
    *item = PolishedRockSmashItem(Random() & 0xff);
    return FALSE;
}

void PolishedRockSmashEncounter(void)
{
    enum Species species;
    enum Item item;
    u8 level;
    bool32 battle = PolishedRockSmashRollOutcome(&species, &level, &item);
    gSpecialVar_0x8004 = item;
    gSpecialVar_Result = battle;
    if (battle)
    {
        CreateScriptedWildMon(species, level, ITEM_NONE);
        BattleSetup_StartWildBattleContinueScript();
    }
}

// Source earthquake 84: 40 frames, vertical displacement +/-2. Then
// rock_smash 10 alternates visible/hidden on ten consecutive frames.
static void AnimateTask(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    if (data[0] < 40)
    {
        s16 remaining = 39 - data[0];
        SetCameraPanning(0, remaining == 0 ? 0 : (remaining & 1) ? -2 : 2);
    }
    else if (data[0] < 50)
    {
        gObjectEvents[data[1]].invisible = (data[0] - 40) & 1;
    }
    else
    {
        InstallCameraPanAheadCallback();
        DestroyTask(taskId);
        ScriptContext_Enable();
        return;
    }
    data[0]++;
}

void PolishedRockSmashAnimate(void)
{
    u8 task = CreateTask(AnimateTask, 8);
    gTasks[task].data[1] = GetObjectEventIdByLocalIdAndMap(gSpecialVar_LastTalked,
        gSaveBlock1Ptr->location.mapNum, gSaveBlock1Ptr->location.mapGroup);
    SetCameraPanningCallback(NULL);
    PlaySE(SE_M_STRENGTH);
}

// Source shows the 24x24 icon in the right side of the message box, including
// on a full-pocket result. This is independent of optional native descriptions.
#define ROCK_ITEM_ICON_TAG (0x2723 | BLEND_IMMUNE_FLAG)
static EWRAM_DATA u8 sRockItemSpritePlusOne = 0;

void PolishedRockSmashShowItem(void)
{
    u8 spriteId = AddItemIconSprite(ROCK_ITEM_ICON_TAG, ROCK_ITEM_ICON_TAG, gSpecialVar_0x8000);
    if (spriteId == MAX_SPRITES)
        return;
    sRockItemSpritePlusOne = spriteId + 1;
    struct Sprite *sprite = &gSprites[spriteId];
    sprite->x2 = 224;
    sprite->y2 = 136;
    sprite->oam.priority = 0;
    if (GetFlashLevel() > 0)
    {
        SetGpuRegBits(REG_OFFSET_DISPCNT, DISPCNT_OBJWIN_ON);
        SetGpuRegBits(REG_OFFSET_WINOUT, WINOUT_WINOBJ_OBJ);
        sprite->copyToObjWin = TRUE;
    }
}

void PolishedRockSmashHideItem(void)
{
    if (sRockItemSpritePlusOne == 0)
        return;
    FreeSpriteTilesByTag(ROCK_ITEM_ICON_TAG);
    FreeSpritePaletteByTag(ROCK_ITEM_ICON_TAG);
    DestroySprite(&gSprites[sRockItemSpritePlusOne - 1]);
    sRockItemSpritePlusOne = 0;
}
