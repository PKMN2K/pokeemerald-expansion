#include "global.h"
#if POLISHED_JOHTO_DEMO
#include "pokemon.h"
#include "sprite.h"
#include "polished_mon_pictures.h"

struct MonPicture
{
    u16 front, back;
    u16 normalPal, shinyPal;
    u16 battleScript, menuScript;
    u8 frameCount, width;
};

static const u32 sTiles[] = INCBIN_U32("graphics/pokemon/polished_battle/tiles.bin");
static const u16 sFrames[] = INCBIN_U16("graphics/pokemon/polished_battle/maps.bin");
static const u8 sScripts[] = INCBIN_U8("graphics/pokemon/polished_battle/scripts.bin");
static const u16 sPalettes[] = INCBIN_U16("graphics/pokemon/polished_battle/palettes.bin");

static const struct MonPicture sPictures[] = {
#include "data/graphics/polished_mon_pictures.h"
};

// Keep the battle controller's sprite data intact. Destroying or reusing
// a sprite also replaces its callback, so interrupted playback cannot resume.
static EWRAM_DATA struct {
    u16 species, step, remaining;
} sAnimation[MAX_SPRITES] = {};

bool32 HasPolishedMonPicture(enum Species species)
{
    return (u32)species < ARRAY_COUNT(sPictures) && sPictures[species].frameCount != 0;
}

u32 GetPolishedMonPictureWidth(enum Species species, bool32 front)
{
    return front ? sPictures[species].width : 48;
}

static void DrawPictureFrame(void *dest, u32 frame)
{
    const u16 *map = &sFrames[frame * 64];
    u32 *out = dest;
    for (u32 i = 0; i < 64; i++, out += TILE_SIZE_4BPP / sizeof(u32))
        CpuCopy32(&sTiles[map[i] * (TILE_SIZE_4BPP / sizeof(u32))], out, TILE_SIZE_4BPP);
}

void LoadPolishedMonPicture(void *dest, enum Species species, bool32 front)
{
    DrawPictureFrame(dest, front ? sPictures[species].front : sPictures[species].back);
}

const u16 *GetPolishedMonPalette(enum Species species, bool32 shiny)
{
    u32 index = shiny ? sPictures[species].shinyPal : sPictures[species].normalPal;
    return &sPalettes[index * 16];
}

static void PlayPictureFrame(struct Sprite *sprite)
{
    u32 id = sprite - gSprites;
    if (sAnimation[id].remaining)
    {
        sAnimation[id].remaining--;
        return;
    }

    u32 step = sAnimation[id].step++ * 2;
    u32 duration = sScripts[step + 1];
    u32 frame = sScripts[step];
    u8 *tiles = (u8 *)OBJ_VRAM0 + sprite->oam.tileNum * TILE_SIZE_4BPP;
    DrawPictureFrame(tiles, sPictures[sAnimation[id].species].front + frame);
    if (duration)
        sAnimation[id].remaining = duration - 1;
    else
        sprite->callback = SpriteCallbackDummy;
}

void StartPolishedMonAnimation(struct Sprite *sprite, enum Species species, bool32 summary)
{
    u32 id = sprite - gSprites;
    sAnimation[id].species = species;
    sAnimation[id].step = summary ? sPictures[species].menuScript : sPictures[species].battleScript;
    sAnimation[id].remaining = 0;
    sprite->callback = PlayPictureFrame;
}

void StartPolishedEggAnimation(struct Sprite *sprite, u32 cycles)
{
    if (cycles >= 11)
        sprite->callback = SpriteCallbackDummy;
    else
        StartPolishedMonAnimation(sprite, SPECIES_EGG, cycles >= 6);
}
#endif
