#ifndef GUARD_BATTLE_AI_ITEMS_H
#define GUARD_BATTLE_AI_ITEMS_H

struct BattlePokemon;
struct Pokemon;
u32 PolishedChooseTrainerMedicine(const struct BattlePokemon *active, struct Pokemon *party, const enum Item *items);
bool32 ShouldUseItem(enum BattlerId battler);

#endif // GUARD_BATTLE_AI_ITEMS_H
