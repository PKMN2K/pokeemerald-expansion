#ifndef GUARD_POLISHED_FLASH_H
#define GUARD_POLISHED_FLASH_H

#include "pokemon.h"

bool32 PolishedFlashMap(u8 group, u8 num);
bool32 PolishedFlashKnowsMove(struct Pokemon *mon);
u8 PolishedFlashPartyUser(struct Pokemon *party, u8 count, bool32 ownsTM);
const u8 *PolishedFlashInteraction(void);
bool32 PolishedFlashFromMenu(void);
void PolishedFlashIlluminate(void);

extern const u8 PolishedFlash_Ask[];
extern const u8 PolishedFlash_Menu[];

#endif
