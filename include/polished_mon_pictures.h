#ifndef GUARD_POLISHED_MON_PICTURES_H
#define GUARD_POLISHED_MON_PICTURES_H

#if POLISHED_JOHTO_DEMO
struct Sprite;

bool32 HasPolishedMonPicture(enum Species species);
u32 GetPolishedMonPictureWidth(enum Species species, bool32 front);
void LoadPolishedMonPicture(void *dest, enum Species species, bool32 front);
const u16 *GetPolishedMonPalette(enum Species species, bool32 shiny);
void StartPolishedEggAnimation(struct Sprite *sprite, u32 cycles);
void StartPolishedMonAnimation(struct Sprite *sprite, enum Species species, bool32 summary);
#endif

#endif
