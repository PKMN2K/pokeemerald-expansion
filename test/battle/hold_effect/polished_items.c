#include "global.h"
#include "test/battle.h"

WILD_BATTLE_TEST("Polished wild battle startup assigns source guaranteed items")
{
    GIVEN {
        WITH_CONFIG(POLISHED_WILD_HELD_ITEMS, TRUE);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_LAPRAS);
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); MOVE(opponent, MOVE_SPLASH); }
    } THEN {
        EXPECT_EQ(opponent->item, ITEM_MYSTIC_WATER);
    }
}

SINGLE_BATTLE_TEST("Polished Pink Bow boosts Fairy moves by 20 percent and leaves Normal moves unchanged", s16 damage)
{
    enum Item item = ITEM_NONE;
    enum Move move = MOVE_NONE;
    PARAMETRIZE { item = ITEM_NONE; move = MOVE_DISARMING_VOICE; }
    PARAMETRIZE { item = ITEM_POLISHED_PINK_BOW; move = MOVE_DISARMING_VOICE; }
    PARAMETRIZE { item = ITEM_NONE; move = MOVE_SCRATCH; }
    PARAMETRIZE { item = ITEM_POLISHED_PINK_BOW; move = MOVE_SCRATCH; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Item(item); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, move); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_MUL_EQ(results[0].damage, Q_4_12(1.2), results[1].damage);
        EXPECT_EQ(results[2].damage, results[3].damage);
    }
}
