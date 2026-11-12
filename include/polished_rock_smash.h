#ifndef GUARD_POLISHED_ROCK_SMASH_H
#define GUARD_POLISHED_ROCK_SMASH_H
#include "pokemon.h"

// Map-visit state is saved by the existing temp variable/flag storage. Native
// warp/connection loading resets it, whereas battle/Continue loading does not.
#define VAR_POLISHED_ROCK_SMASH_ACTIVE VAR_TEMP_5
u8 PolishedRockSmashPartyUser(struct Pokemon *party, u8 count, bool32 ownsTM);
bool32 PolishedRockSmashKnowsMove(struct Pokemon *mon);
bool32 PolishedRockSmashMap(u8 group, u8 num);
bool32 PolishedRockSmashBattleRoll(u8 roll);
enum Species PolishedRockSmashSpecies(u8 roll, u8 *level);
enum Item PolishedRockSmashItem(u8 roll);
bool32 PolishedRockSmashRollOutcome(enum Species *species, u8 *level, enum Item *item);
u16 PolishedRockSmashPrepare(void);
void PolishedRockSmashAnimate(void);
void PolishedRockSmashEncounter(void);
void PolishedRockSmashShowItem(void);
void PolishedRockSmashHideItem(void);
bool32 PolishedRockSmashFromMenu(void);
extern const u8 PolishedRockSmash_Menu[];
#endif
