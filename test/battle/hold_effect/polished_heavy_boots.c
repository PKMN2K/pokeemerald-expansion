#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished Heavy Boots prevent Poison types from absorbing Toxic Spikes")
{
    struct BattlePokemon *entrant = player, *setter = opponent;
    u32 layers = 1, side = B_SIDE_PLAYER;
    bool32 boots = FALSE;
    for (u32 s = 0; s < 2; s++)
        for (u32 count = 1; count <= 2; count++)
            for (u32 item = 0; item < 2; item++)
                PARAMETRIZE { entrant = s ? opponent : player; setter = s ? player : opponent; side = s; layers = count; boots = item; }
    bool32 retained = B_POLISHED_MOVE_BALANCE && boots;
    GIVEN {
        ASSUME(gItemsInfo[ITEM_HEAVY_DUTY_BOOTS].holdEffect == HOLD_EFFECT_HEAVY_DUTY_BOOTS);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        if (entrant == player) PLAYER(SPECIES_EKANS) { Ability(ABILITY_NONE); Item(boots ? ITEM_HEAVY_DUTY_BOOTS : ITEM_NONE); MaxHP(400); HP(400); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        if (entrant == opponent) OPPONENT(SPECIES_EKANS) { Ability(ABILITY_NONE); Item(boots ? ITEM_HEAVY_DUTY_BOOTS : ITEM_NONE); MaxHP(400); HP(400); }
    } WHEN {
        TURN { MOVE(setter, MOVE_TOXIC_SPIKES); }
        if (layers == 2) TURN { MOVE(setter, MOVE_TOXIC_SPIKES); }
        TURN { SWITCH(entrant, 1); }
    } SCENE {
        if (side == B_SIDE_PLAYER) {
            if (retained) NOT MESSAGE("The toxic spikes disappeared from the ground around your side!");
            else MESSAGE("The toxic spikes disappeared from the ground around your side!");
        } else {
            if (retained) NOT MESSAGE("The toxic spikes disappeared from the ground around the opposing side!");
            else MESSAGE("The toxic spikes disappeared from the ground around the opposing side!");
        }
    } THEN {
        EXPECT_EQ((u32)gSideTimers[side].toxicSpikesAmount, retained ? layers : 0);
        EXPECT_EQ(entrant->status1, STATUS1_NONE);
        EXPECT_EQ(entrant->hp, 400);
        EXPECT_EQ(entrant->item, boots ? ITEM_HEAVY_DUTY_BOOTS : ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Heavy Boots leave Toxic Spikes active for the next susceptible entrant")
{
    struct BattlePokemon *entrant = player, *setter = opponent;
    u32 layers = 1, side = B_SIDE_PLAYER;
    for (u32 s = 0; s < 2; s++)
        for (u32 count = 1; count <= 2; count++)
            PARAMETRIZE { entrant = s ? opponent : player; setter = s ? player : opponent; side = s; layers = count; }
    u32 status = layers == 2 ? STATUS1_TOXIC_POISON : STATUS1_POISON;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        if (entrant == player) {
            PLAYER(SPECIES_EKANS) { Ability(ABILITY_NONE); Item(ITEM_HEAVY_DUTY_BOOTS); }
            PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); MaxHP(400); HP(400); }
        }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        if (entrant == opponent) {
            OPPONENT(SPECIES_EKANS) { Ability(ABILITY_NONE); Item(ITEM_HEAVY_DUTY_BOOTS); }
            OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); MaxHP(400); HP(400); }
        }
    } WHEN {
        TURN { MOVE(setter, MOVE_TOXIC_SPIKES); }
        if (layers == 2) TURN { MOVE(setter, MOVE_TOXIC_SPIKES); }
        TURN { SWITCH(entrant, 1); }
        TURN { SWITCH(entrant, 2); }
    } SCENE {
        if (B_POLISHED_MOVE_BALANCE) STATUS_ICON(entrant, status);
        else NOT STATUS_ICON(entrant, status);
    } THEN {
        EXPECT_EQ((u32)gSideTimers[side].toxicSpikesAmount, B_POLISHED_MOVE_BALANCE ? layers : 0);
        EXPECT_EQ(entrant->status1 & (STATUS1_POISON | STATUS1_TOXIC_POISON), B_POLISHED_MOVE_BALANCE ? status : 0);
        if (B_POLISHED_MOVE_BALANCE) EXPECT_LT(entrant->hp, 400);
        else EXPECT_EQ(entrant->hp, 400);
    }
}

SINGLE_BATTLE_TEST("Polished Heavy Boots retain Toxic Spikes layer count and the two layer cap")
{
    struct BattlePokemon *entrant = player, *setter = opponent;
    u32 layers = 1, side = B_SIDE_PLAYER;
    for (u32 s = 0; s < 2; s++)
        for (u32 count = 1; count <= 2; count++)
            PARAMETRIZE { entrant = s ? opponent : player; setter = s ? player : opponent; side = s; layers = count; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        if (entrant == player) PLAYER(SPECIES_EKANS) { Ability(ABILITY_NONE); Item(ITEM_HEAVY_DUTY_BOOTS); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        if (entrant == opponent) OPPONENT(SPECIES_EKANS) { Ability(ABILITY_NONE); Item(ITEM_HEAVY_DUTY_BOOTS); }
    } WHEN {
        TURN { MOVE(setter, MOVE_TOXIC_SPIKES); }
        if (layers == 2) TURN { MOVE(setter, MOVE_TOXIC_SPIKES); }
        TURN { SWITCH(entrant, 1); }
        TURN { MOVE(setter, MOVE_TOXIC_SPIKES); }
    } THEN {
        EXPECT_EQ((u32)gSideTimers[side].toxicSpikesAmount, B_POLISHED_MOVE_BALANCE ? 2 : 1);
        EXPECT_EQ(entrant->status1, STATUS1_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Heavy Boots skip stacked Spikes and Toxic Spikes in either hazard order")
{
    struct BattlePokemon *entrant = player, *setter = opponent;
    u32 spikes = 1, side = B_SIDE_PLAYER;
    bool32 poisonType = FALSE, toxicFirst = FALSE;
    for (u32 s = 0; s < 2; s++)
        for (u32 count = 1; count <= 3; count++)
            for (u32 type = 0; type < 2; type++)
                for (u32 order = 0; order < 2; order++)
                    PARAMETRIZE { entrant = s ? opponent : player; setter = s ? player : opponent; side = s; spikes = count; poisonType = type; toxicFirst = order; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        if (entrant == player) PLAYER(poisonType ? SPECIES_EKANS : SPECIES_WYNAUT) { Ability(ABILITY_NONE); Item(ITEM_HEAVY_DUTY_BOOTS); MaxHP(400); HP(400); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        if (entrant == opponent) OPPONENT(poisonType ? SPECIES_EKANS : SPECIES_WYNAUT) { Ability(ABILITY_NONE); Item(ITEM_HEAVY_DUTY_BOOTS); MaxHP(400); HP(400); }
    } WHEN {
        if (toxicFirst) TURN { MOVE(setter, MOVE_TOXIC_SPIKES); }
        TURN { MOVE(setter, MOVE_SPIKES); }
        if (spikes >= 2) TURN { MOVE(setter, MOVE_SPIKES); }
        if (spikes >= 3) TURN { MOVE(setter, MOVE_SPIKES); }
        if (!toxicFirst) TURN { MOVE(setter, MOVE_TOXIC_SPIKES); }
        TURN { SWITCH(entrant, 1); }
        TURN { MOVE(entrant, MOVE_CELEBRATE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, entrant);
    } THEN {
        EXPECT_EQ(entrant->hp, 400);
        EXPECT_EQ(entrant->status1, STATUS1_NONE);
        EXPECT_EQ((u32)gSideTimers[side].spikesAmount, spikes);
        EXPECT_EQ((u32)gSideTimers[side].toxicSpikesAmount, !poisonType || B_POLISHED_MOVE_BALANCE ? 1 : 0);
    }
}

SINGLE_BATTLE_TEST("Polished Heavy Boots retain ordinary susceptible and Steel type hazard protection")
{
    struct BattlePokemon *entrant = player, *setter = opponent;
    u32 layers = 1, side = B_SIDE_PLAYER;
    bool32 steel = FALSE, boots = FALSE;
    for (u32 s = 0; s < 2; s++)
        for (u32 count = 1; count <= 2; count++)
            for (u32 type = 0; type < 2; type++)
                for (u32 item = 0; item < 2; item++)
                    PARAMETRIZE { entrant = s ? opponent : player; setter = s ? player : opponent; side = s; layers = count; steel = type; boots = item; }
    u32 status = layers == 2 ? STATUS1_TOXIC_POISON : STATUS1_POISON;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        if (entrant == player) PLAYER(steel ? SPECIES_BELDUM : SPECIES_WYNAUT) { Ability(ABILITY_NONE); Item(boots ? ITEM_HEAVY_DUTY_BOOTS : ITEM_NONE); MaxHP(400); HP(400); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        if (entrant == opponent) OPPONENT(steel ? SPECIES_BELDUM : SPECIES_WYNAUT) { Ability(ABILITY_NONE); Item(boots ? ITEM_HEAVY_DUTY_BOOTS : ITEM_NONE); MaxHP(400); HP(400); }
    } WHEN {
        TURN { MOVE(setter, MOVE_TOXIC_SPIKES); }
        if (layers == 2) TURN { MOVE(setter, MOVE_TOXIC_SPIKES); }
        TURN { SWITCH(entrant, 1); }
    } THEN {
        EXPECT_EQ(entrant->status1 & (STATUS1_POISON | STATUS1_TOXIC_POISON), !steel && !boots ? status : 0);
        EXPECT_EQ((u32)gSideTimers[side].toxicSpikesAmount, layers);
        EXPECT_EQ(entrant->item, boots ? ITEM_HEAVY_DUTY_BOOTS : ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Heavy Boots retain airborne Poison type nonabsorption")
{
    struct BattlePokemon *entrant = player, *setter = opponent;
    u32 layers = 1, side = B_SIDE_PLAYER;
    bool32 flying = FALSE, boots = FALSE;
    for (u32 s = 0; s < 2; s++)
        for (u32 count = 1; count <= 2; count++)
            for (u32 air = 0; air < 2; air++)
                for (u32 item = 0; item < 2; item++)
                    PARAMETRIZE { entrant = s ? opponent : player; setter = s ? player : opponent; side = s; layers = count; flying = air; boots = item; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        if (entrant == player) PLAYER(flying ? SPECIES_ZUBAT : SPECIES_KOFFING) { Ability(flying ? ABILITY_NONE : ABILITY_LEVITATE); Item(boots ? ITEM_HEAVY_DUTY_BOOTS : ITEM_NONE); MaxHP(400); HP(400); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        if (entrant == opponent) OPPONENT(flying ? SPECIES_ZUBAT : SPECIES_KOFFING) { Ability(flying ? ABILITY_NONE : ABILITY_LEVITATE); Item(boots ? ITEM_HEAVY_DUTY_BOOTS : ITEM_NONE); MaxHP(400); HP(400); }
    } WHEN {
        TURN { MOVE(setter, MOVE_TOXIC_SPIKES); }
        if (layers == 2) TURN { MOVE(setter, MOVE_TOXIC_SPIKES); }
        TURN { SWITCH(entrant, 1); }
    } THEN {
        EXPECT_EQ((u32)gSideTimers[side].toxicSpikesAmount, layers);
        EXPECT_EQ(entrant->status1, STATUS1_NONE);
        EXPECT_EQ(entrant->hp, 400);
    }
}

SINGLE_BATTLE_TEST("Polished Heavy Boots use the effective held item after optional suppression")
{
    struct BattlePokemon *entrant = player, *setter = opponent;
    u32 layers = 1, side = B_SIDE_PLAYER, suppression = 0;
    for (u32 s = 0; s < 2; s++)
        for (u32 count = 1; count <= 2; count++)
            for (u32 mode = 0; mode < 3; mode++)
                PARAMETRIZE { entrant = s ? opponent : player; setter = s ? player : opponent; side = s; layers = count; suppression = mode; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        if (entrant == player) PLAYER(SPECIES_EKANS) { Ability(suppression == 1 ? ABILITY_KLUTZ : ABILITY_NONE); Item(ITEM_HEAVY_DUTY_BOOTS); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        if (entrant == opponent) OPPONENT(SPECIES_EKANS) { Ability(suppression == 1 ? ABILITY_KLUTZ : ABILITY_NONE); Item(ITEM_HEAVY_DUTY_BOOTS); }
    } WHEN {
        TURN { MOVE(setter, MOVE_TOXIC_SPIKES); }
        if (layers == 2) TURN { MOVE(setter, MOVE_TOXIC_SPIKES); }
        if (suppression == 2) TURN { MOVE(setter, MOVE_MAGIC_ROOM); }
        TURN { SWITCH(entrant, 1); }
    } THEN {
        EXPECT_EQ((u32)gSideTimers[side].toxicSpikesAmount, B_POLISHED_MOVE_BALANCE && !suppression ? layers : 0);
        EXPECT_EQ(entrant->item, ITEM_HEAVY_DUTY_BOOTS);
        EXPECT_EQ(entrant->status1, STATUS1_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Heavy Boots removed by Knock Off allow absorption on reentry")
{
    struct BattlePokemon *entrant = player, *setter = opponent;
    u32 layers = 1, side = B_SIDE_PLAYER;
    for (u32 s = 0; s < 2; s++)
        for (u32 count = 1; count <= 2; count++)
            PARAMETRIZE { entrant = s ? opponent : player; setter = s ? player : opponent; side = s; layers = count; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); MaxHP(400); HP(400); }
        if (entrant == player) PLAYER(SPECIES_EKANS) { Ability(ABILITY_NONE); Item(ITEM_HEAVY_DUTY_BOOTS); MaxHP(400); HP(400); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); MaxHP(400); HP(400); }
        if (entrant == opponent) OPPONENT(SPECIES_EKANS) { Ability(ABILITY_NONE); Item(ITEM_HEAVY_DUTY_BOOTS); MaxHP(400); HP(400); }
    } WHEN {
        TURN { MOVE(setter, MOVE_TOXIC_SPIKES); }
        if (layers == 2) TURN { MOVE(setter, MOVE_TOXIC_SPIKES); }
        TURN { SWITCH(entrant, 1); }
        TURN { MOVE(setter, MOVE_KNOCK_OFF); }
        TURN { SWITCH(entrant, 0); MOVE(setter, MOVE_TOXIC_SPIKES); }
        TURN { SWITCH(entrant, 1); }
    } THEN {
        EXPECT_EQ(entrant->item, ITEM_NONE);
        EXPECT_EQ(entrant->status1, STATUS1_NONE);
        EXPECT_EQ((u32)gSideTimers[side].toxicSpikesAmount, 0);
    }
}

SINGLE_BATTLE_TEST("Polished Heavy Boots protect forced Poison type entrants from absorbing Toxic Spikes")
{
    struct BattlePokemon *entrant = player, *setter = opponent;
    u32 layers = 1, side = B_SIDE_PLAYER;
    enum Move move = MOVE_ROAR;
    for (u32 s = 0; s < 2; s++)
        for (u32 count = 1; count <= 2; count++)
            for (u32 force = 0; force < 2; force++)
                PARAMETRIZE { entrant = s ? opponent : player; setter = s ? player : opponent; side = s; layers = count; move = force ? MOVE_WHIRLWIND : MOVE_ROAR; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        if (entrant == player) PLAYER(SPECIES_EKANS) { Ability(ABILITY_NONE); Item(ITEM_HEAVY_DUTY_BOOTS); MaxHP(400); HP(400); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        if (entrant == opponent) OPPONENT(SPECIES_EKANS) { Ability(ABILITY_NONE); Item(ITEM_HEAVY_DUTY_BOOTS); MaxHP(400); HP(400); }
    } WHEN {
        TURN { MOVE(setter, MOVE_TOXIC_SPIKES); }
        if (layers == 2) TURN { MOVE(setter, MOVE_TOXIC_SPIKES); }
        TURN { MOVE(setter, move, hit: TRUE); }
    } THEN {
        EXPECT_EQ(entrant->species, SPECIES_EKANS);
        EXPECT_EQ((u32)gSideTimers[side].toxicSpikesAmount, B_POLISHED_MOVE_BALANCE ? layers : 0);
        EXPECT_EQ(entrant->status1, STATUS1_NONE);
        EXPECT_EQ(entrant->hp, 400);
    }
}

DOUBLE_BATTLE_TEST("Polished Heavy Boots leave doubles absorption native")
{
    struct BattlePokemon *entrant = playerLeft, *setter = opponentLeft;
    u32 layers = 1, side = B_SIDE_PLAYER;
    for (u32 s = 0; s < 2; s++)
        for (u32 count = 1; count <= 2; count++)
            PARAMETRIZE { entrant = s ? opponentLeft : playerLeft; setter = s ? playerLeft : opponentLeft; side = s; layers = count; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); }
        if (entrant == playerLeft) PLAYER(SPECIES_EKANS) { Ability(ABILITY_NONE); Item(ITEM_HEAVY_DUTY_BOOTS); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); }
        if (entrant == opponentLeft) OPPONENT(SPECIES_EKANS) { Ability(ABILITY_NONE); Item(ITEM_HEAVY_DUTY_BOOTS); }
    } WHEN {
        TURN { MOVE(setter, MOVE_TOXIC_SPIKES); }
        if (layers == 2) TURN { MOVE(setter, MOVE_TOXIC_SPIKES); }
        TURN { SWITCH(entrant, 2); }
    } THEN {
        EXPECT_EQ((u32)gSideTimers[side].toxicSpikesAmount, 0);
        EXPECT_EQ(entrant->status1, STATUS1_NONE);
        EXPECT_EQ(entrant->item, ITEM_HEAVY_DUTY_BOOTS);
    }
}

SINGLE_BATTLE_TEST("Polished Heavy Boots leave either active Terastal absorption native")
{
    struct BattlePokemon *entrant = player, *setter = opponent;
    u32 layers = 1, side = B_SIDE_PLAYER;
    bool32 entrantTera = FALSE;
    for (u32 s = 0; s < 2; s++)
        for (u32 count = 1; count <= 2; count++)
            for (u32 tera = 0; tera < 2; tera++)
                PARAMETRIZE { entrant = s ? opponent : player; setter = s ? player : opponent; side = s; layers = count; entrantTera = tera; }
    GIVEN {
        PLAYER(entrant == player ? SPECIES_EKANS : SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(entrant == player ? ITEM_HEAVY_DUTY_BOOTS : ITEM_NONE); TeraType(entrant == player ? TYPE_POISON : TYPE_NORMAL); }
        if (entrant == player) PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); }
        OPPONENT(entrant == opponent ? SPECIES_EKANS : SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(entrant == opponent ? ITEM_HEAVY_DUTY_BOOTS : ITEM_NONE); TeraType(entrant == opponent ? TYPE_POISON : TYPE_NORMAL); }
        if (entrant == opponent) OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(setter, MOVE_TOXIC_SPIKES, gimmick: entrantTera ? GIMMICK_NONE : GIMMICK_TERA); MOVE(entrant, MOVE_CELEBRATE, gimmick: entrantTera ? GIMMICK_TERA : GIMMICK_NONE); }
        if (layers == 2) TURN { MOVE(setter, MOVE_TOXIC_SPIKES); }
        TURN { SWITCH(entrant, 1); }
        TURN { SWITCH(entrant, 0); }
    } THEN {
        EXPECT_EQ(entrant->species, SPECIES_EKANS);
        EXPECT_EQ((u32)gSideTimers[side].toxicSpikesAmount, 0);
        EXPECT_EQ(entrant->status1, STATUS1_NONE);
    }
}
