#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished X items always raise two stages independently of the native preset")
{
    enum Item item = ITEM_X_ATTACK;
    enum Stat stat = STAT_ATK;
    PARAMETRIZE { item = ITEM_X_ATTACK; stat = STAT_ATK; }
    PARAMETRIZE { item = ITEM_X_DEFENSE; stat = STAT_DEF; }
    PARAMETRIZE { item = ITEM_X_SP_ATK; stat = STAT_SPATK; }
    PARAMETRIZE { item = ITEM_X_SP_DEF; stat = STAT_SPDEF; }
    PARAMETRIZE { item = ITEM_X_SPEED; stat = STAT_SPEED; }
    PARAMETRIZE { item = ITEM_X_ACCURACY; stat = STAT_ACC; }
    GIVEN {
        WITH_CONFIG(B_X_ITEMS_BUFF, GEN_3);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN { TURN { USE_ITEM(player, item); } }
    THEN { EXPECT_EQ(player->statStages[stat], DEFAULT_STAT_STAGE + (B_POLISHED_MOVE_BALANCE ? 2 : 1)); }
}

SINGLE_BATTLE_TEST("Polished X item boosts cannot be copied by a later Mirror Herb checkpoint")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(100); Item(ITEM_MIRROR_HERB); }
    } WHEN {
        TURN { USE_ITEM(player, ITEM_X_ATTACK); }
        TURN { MOVE(player, MOVE_SPLASH); MOVE(opponent, MOVE_SPLASH); }
    } THEN {
        if (B_POLISHED_MOVE_BALANCE) {
            EXPECT_EQ(opponent->item, ITEM_MIRROR_HERB);
            EXPECT_EQ(opponent->statStages[STAT_ATK], DEFAULT_STAT_STAGE);
        }
    }
}
