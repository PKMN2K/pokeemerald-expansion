#include "global.h"
#include "battle.h"
#include "battle_anim.h"
#include "data.h"
#include "decompress.h"
#include "pokemon.h"
#include "polished_mon_pictures.h"
#include "random.h"
#include "sprite.h"

struct PictureCase {
    u16 species;
    u32 front, back, normal, shiny;
    u16 battle, menu;
};
struct Step { u32 crc; u16 duration; };

static const struct PictureCase sCases[] = {
/* CASES */
};
static const struct Step sSteps[] = {
/* STEPS */
};
static EWRAM_DATA u32 sBuffer[MON_PIC_SIZE * MAX_MON_PIC_FRAMES / 4 + 1] = {};
EWRAM_DATA volatile u32 gPicAudit[6] = {};

static void Check(u32 actual, u32 expected, u32 code)
{
    if (actual != expected && !gPicAudit[1])
    {
        gPicAudit[1] = code;
        gPicAudit[2] = actual;
        gPicAudit[3] = expected;
    }
}

static void CheckPlayback(struct Sprite *sprite, u32 step)
{
    for (; sSteps[step].duration; step++)
    {
        gPicAudit[4] = step;
        for (u32 tick = 0; tick < sSteps[step].duration; tick++)
        {
            Check(sprite->callback != SpriteCallbackDummy, TRUE, 10);
            sprite->callback(sprite);
            if (!tick || tick + 1 == sSteps[step].duration)
                Check(Crc32B((const u8 *)OBJ_VRAM0, MON_PIC_SIZE), sSteps[step].crc, 11);
            if (gPicAudit[1])
                return;
        }
    }
    sprite->callback(sprite);
    Check(Crc32B((const u8 *)OBJ_VRAM0, MON_PIC_SIZE), sSteps[step].crc, 12);
    Check(sprite->callback == SpriteCallbackDummy, TRUE, 13);
}

void CB2_MoveMachineAudit(void)
{
    u32 row = gPicAudit[0];
    if (gPicAudit[1] || gPicAudit[5])
        return;
    if (row == ARRAY_COUNT(sCases))
    {
        // An extra Expansion Pokémon continues to load through the same public API.
        HandleLoadSpecialPokePic(TRUE, sBuffer, SPECIES_RAYQUAZA, 0);
        u32 crc = Crc32B((const u8 *)sBuffer, MON_PIC_SIZE);
        DecompressDataWithHeaderWram(gSpeciesInfo[SPECIES_RAYQUAZA].frontPic, sBuffer);
        Check(crc, Crc32B((const u8 *)sBuffer, MON_PIC_SIZE), 15);
        gPicAudit[5] = 1;
        return;
    }
    const struct PictureCase *pic = &sCases[row];
    bool32 egg = pic->species == SPECIES_EGG;
    enum Species species = egg ? SPECIES_TOGEPI : pic->species;
    for (u32 sex = 0; sex < 2; sex++)
    {
        u32 personality = sex ? 0xffffffff : 0;
        for (u32 face = 0; face < 2; face++)
        {
            sBuffer[MON_PIC_SIZE/4] = 0xa937b601;
            HandleLoadSpecialPokePicIsEgg(face, sBuffer, species, personality, egg);
            Check(Crc32B((const u8 *)sBuffer, MON_PIC_SIZE), face ? pic->front : pic->back, 1+face);
            Check(sBuffer[MON_PIC_SIZE/4], 0xa937b601, 3);
        }
        for (u32 shiny = 0; shiny < 2; shiny++)
        {
            const u16 *pal = GetMonSpritePalFromSpeciesIsEgg(species, shiny, sex, egg);
            Check(Crc32B((const u8 *)pal, 32), shiny ? pic->shiny : pic->normal, 4+shiny);
        }
    }
    struct Sprite *sprite = &gSprites[0];
    if (egg)
        StartPolishedEggAnimation(sprite, 5);
    else
        DoMonFrontSpriteAnimation(sprite, pic->species, TRUE, 1);
    CheckPlayback(sprite, pic->battle);
    if (egg)
        StartPolishedEggAnimation(sprite, 6);
    else
        PokemonSummaryDoMonAnimation(sprite, pic->species, FALSE);
    CheckPlayback(sprite, pic->menu);
    if (egg)
    {
        StartPolishedEggAnimation(sprite, 11);
        Check(sprite->callback == SpriteCallbackDummy, TRUE, 16);
    }
    if (!gPicAudit[1])
        gPicAudit[0]++;
}
