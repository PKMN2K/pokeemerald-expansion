#include "global.h"
#include "test/battle.h"

static const enum Item sOrbs[] = {ITEM_TOXIC_ORB, ITEM_FLAME_ORB};

static u32 OrbStatus(enum Item item)
{
    return item == ITEM_TOXIC_ORB ? STATUS1_TOXIC_POISON : STATUS1_BURN;
}

SINGLE_BATTLE_TEST("Polished status orbs bypass Safeguard without consuming the item or dealing immediate residual damage")
{
    struct BattlePokemon *holder = player;
    enum Item item = ITEM_TOXIC_ORB;
    bool32 safeguard = FALSE;
    for (u32 entry = 0; entry < ARRAY_COUNT(sOrbs); entry++)
        for (u32 side = 0; side < 2; side++)
            for (u32 guard = 0; guard < 2; guard++)
                PARAMETRIZE { item = sOrbs[entry]; holder = side ? opponent : player; safeguard = guard; }
    bool32 affected = !safeguard || B_POLISHED_MOVE_BALANCE;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); MaxHP(200); HP(200); Item(holder == player ? item : ITEM_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); MaxHP(200); HP(200); Item(holder == opponent ? item : ITEM_NONE); }
    } WHEN { TURN { MOVE(holder, safeguard ? MOVE_SAFEGUARD : MOVE_CELEBRATE); } }
    SCENE {
        if (affected) STATUS_ICON(holder, OrbStatus(item));
        else NOT STATUS_ICON(holder, OrbStatus(item));
    } THEN {
        EXPECT_EQ(holder->status1, affected ? OrbStatus(item) : STATUS1_NONE);
        EXPECT_EQ(holder->item, item);
        EXPECT_EQ(holder->hp, 200);
    }
}

SINGLE_BATTLE_TEST("Polished status orbs first deal residual damage on the following turn")
{
    struct BattlePokemon *holder = player;
    enum Item item = ITEM_TOXIC_ORB;
    for (u32 entry = 0; entry < ARRAY_COUNT(sOrbs); entry++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { item = sOrbs[entry]; holder = side ? opponent : player; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); MaxHP(200); HP(200); Item(holder == player ? item : ITEM_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); MaxHP(200); HP(200); Item(holder == opponent ? item : ITEM_NONE); }
    } WHEN {
        TURN { MOVE(holder, MOVE_SAFEGUARD); }
        TURN { MOVE(holder, MOVE_CELEBRATE); }
    } SCENE {
        if (B_POLISHED_MOVE_BALANCE) STATUS_ICON(holder, OrbStatus(item));
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, holder);
        if (B_POLISHED_MOVE_BALANCE) HP_BAR(holder);
        else NOT HP_BAR(holder);
    } THEN {
        EXPECT_EQ(holder->status1 & STATUS1_ANY, B_POLISHED_MOVE_BALANCE ? OrbStatus(item) : STATUS1_NONE);
        EXPECT_EQ(holder->item, item);
        if (B_POLISHED_MOVE_BALANCE) EXPECT_LT(holder->hp, 200);
        else EXPECT_EQ(holder->hp, 200);
    }
}

SINGLE_BATTLE_TEST("Polished status orbs use the Toxic Orb holder Corrosion for Poison and Steel types")
{
    static const enum Species species[] = {SPECIES_WOBBUFFET, SPECIES_ARBOK, SPECIES_STEELIX};
    struct BattlePokemon *holder = player;
    enum Species victim = SPECIES_WOBBUFFET;
    bool32 corrosion = FALSE, safeguard = FALSE;
    for (u32 s = 0; s < ARRAY_COUNT(species); s++)
        for (u32 side = 0; side < 2; side++)
            for (u32 ability = 0; ability < 2; ability++)
                for (u32 guard = 0; guard < 2; guard++)
                    PARAMETRIZE { victim = species[s]; holder = side ? opponent : player; corrosion = ability; safeguard = guard; }
    bool32 affected = (victim == SPECIES_WOBBUFFET || corrosion) && (!safeguard || B_POLISHED_MOVE_BALANCE);
    GIVEN {
        PLAYER(holder == player ? victim : SPECIES_WOBBUFFET) { Ability(holder == player && corrosion ? ABILITY_CORROSION : ABILITY_NONE); Item(holder == player ? ITEM_TOXIC_ORB : ITEM_NONE); }
        OPPONENT(holder == opponent ? victim : SPECIES_WOBBUFFET) { Ability(holder == opponent && corrosion ? ABILITY_CORROSION : ABILITY_NONE); Item(holder == opponent ? ITEM_TOXIC_ORB : ITEM_NONE); }
    } WHEN { TURN { MOVE(holder, safeguard ? MOVE_SAFEGUARD : MOVE_CELEBRATE); } }
    THEN { EXPECT_EQ(holder->status1, affected ? STATUS1_TOXIC_POISON : STATUS1_NONE); }
}

SINGLE_BATTLE_TEST("Polished status orbs retain Flame Orb Fire immunity even with Corrosion")
{
    struct BattlePokemon *holder = player;
    bool32 fire = FALSE, corrosion = FALSE, safeguard = FALSE;
    for (u32 type = 0; type < 2; type++)
        for (u32 side = 0; side < 2; side++)
            for (u32 ability = 0; ability < 2; ability++)
                for (u32 guard = 0; guard < 2; guard++)
                    PARAMETRIZE { fire = type; holder = side ? opponent : player; corrosion = ability; safeguard = guard; }
    bool32 affected = !fire && (!safeguard || B_POLISHED_MOVE_BALANCE);
    GIVEN {
        PLAYER(holder == player && fire ? SPECIES_CHARMANDER : SPECIES_WOBBUFFET) { Ability(holder == player && corrosion ? ABILITY_CORROSION : ABILITY_NONE); Item(holder == player ? ITEM_FLAME_ORB : ITEM_NONE); }
        OPPONENT(holder == opponent && fire ? SPECIES_CHARMANDER : SPECIES_WOBBUFFET) { Ability(holder == opponent && corrosion ? ABILITY_CORROSION : ABILITY_NONE); Item(holder == opponent ? ITEM_FLAME_ORB : ITEM_NONE); }
    } WHEN { TURN { MOVE(holder, safeguard ? MOVE_SAFEGUARD : MOVE_CELEBRATE); } }
    THEN { EXPECT_EQ(holder->status1, affected ? STATUS1_BURN : STATUS1_NONE); }
}

SINGLE_BATTLE_TEST("Polished status orbs retain holder ability immunities even against Mold Breaker")
{
    static const enum Ability abilities[] = {ABILITY_IMMUNITY, ABILITY_PASTEL_VEIL, ABILITY_LEAF_GUARD, ABILITY_WATER_VEIL, ABILITY_LEAF_GUARD};
    struct BattlePokemon *holder = player, *foe = opponent;
    enum Ability ability = ABILITY_IMMUNITY;
    enum Item item = ITEM_TOXIC_ORB;
    bool32 moldBreaker = FALSE;
    for (u32 a = 0; a < ARRAY_COUNT(abilities); a++)
        for (u32 side = 0; side < 2; side++)
            for (u32 mold = 0; mold < 2; mold++)
                PARAMETRIZE { ability = abilities[a]; item = a < 3 ? ITEM_TOXIC_ORB : ITEM_FLAME_ORB; holder = side ? opponent : player; foe = side ? player : opponent; moldBreaker = mold; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(holder == player ? ability : (moldBreaker ? ABILITY_MOLD_BREAKER : ABILITY_NONE)); Item(holder == player ? item : ITEM_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(holder == opponent ? ability : (moldBreaker ? ABILITY_MOLD_BREAKER : ABILITY_NONE)); Item(holder == opponent ? item : ITEM_NONE); }
    } WHEN { TURN { MOVE(holder, MOVE_SAFEGUARD); MOVE(foe, MOVE_SUNNY_DAY); } }
    SCENE { NOT STATUS_ICON(holder, OrbStatus(item)); }
    THEN { EXPECT_EQ(holder->status1, STATUS1_NONE); }
}

SINGLE_BATTLE_TEST("Polished status orbs respect ability suppression for both immunity and Corrosion")
{
    struct BattlePokemon *holder = player, *foe = opponent;
    enum Item item = ITEM_TOXIC_ORB;
    enum Ability ability = ABILITY_IMMUNITY;
    bool32 gas = FALSE, poisonType = FALSE;
    for (u32 kind = 0; kind < 3; kind++)
        for (u32 side = 0; side < 2; side++)
            for (u32 suppression = 0; suppression < 2; suppression++)
                PARAMETRIZE { item = kind == 1 ? ITEM_FLAME_ORB : ITEM_TOXIC_ORB; ability = kind == 0 ? ABILITY_IMMUNITY : (kind == 1 ? ABILITY_WATER_VEIL : ABILITY_CORROSION); poisonType = kind == 2; holder = side ? opponent : player; foe = side ? player : opponent; gas = suppression; }
    GIVEN {
        PLAYER(holder == player && poisonType ? SPECIES_ARBOK : SPECIES_WOBBUFFET) { Ability(holder == player ? ability : (gas ? ABILITY_NEUTRALIZING_GAS : ABILITY_NONE)); Item(holder == player ? item : ITEM_NONE); }
        OPPONENT(holder == opponent && poisonType ? SPECIES_ARBOK : SPECIES_WOBBUFFET) { Ability(holder == opponent ? ability : (gas ? ABILITY_NEUTRALIZING_GAS : ABILITY_NONE)); Item(holder == opponent ? item : ITEM_NONE); }
    } WHEN { TURN { MOVE(holder, MOVE_SAFEGUARD); if (!gas) MOVE(foe, MOVE_GASTRO_ACID); } }
    THEN { EXPECT_EQ(holder->status1, B_POLISHED_MOVE_BALANCE && !poisonType ? OrbStatus(item) : STATUS1_NONE); }
}

SINGLE_BATTLE_TEST("Polished status orbs never overwrite an existing major status")
{
    static const u32 statuses[] = {STATUS1_BURN, STATUS1_POISON, STATUS1_PARALYSIS};
    struct BattlePokemon *holder = player;
    enum Item item = ITEM_TOXIC_ORB;
    u32 status = STATUS1_BURN;
    for (u32 entry = 0; entry < ARRAY_COUNT(sOrbs); entry++)
        for (u32 side = 0; side < 2; side++)
            for (u32 s = 0; s < ARRAY_COUNT(statuses); s++)
                PARAMETRIZE { item = sOrbs[entry]; holder = side ? opponent : player; status = statuses[s]; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(holder == player ? item : ITEM_NONE); if (holder == player) Status1(status); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(holder == opponent ? item : ITEM_NONE); if (holder == opponent) Status1(status); }
    } WHEN { TURN {} }
    THEN { EXPECT_EQ(holder->status1, status); EXPECT_EQ(holder->item, item); }
}

SINGLE_BATTLE_TEST("Polished status orbs retain activation through Protect and Substitute")
{
    struct BattlePokemon *holder = player;
    enum Item item = ITEM_TOXIC_ORB;
    bool32 sub = FALSE;
    for (u32 entry = 0; entry < ARRAY_COUNT(sOrbs); entry++)
        for (u32 side = 0; side < 2; side++)
            for (u32 shield = 0; shield < 2; shield++)
                PARAMETRIZE { item = sOrbs[entry]; holder = side ? opponent : player; sub = shield; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); MaxHP(200); HP(200); Item(holder == player ? item : ITEM_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); MaxHP(200); HP(200); Item(holder == opponent ? item : ITEM_NONE); }
    } WHEN { TURN { MOVE(holder, sub ? MOVE_SUBSTITUTE : MOVE_PROTECT); } }
    THEN {
        EXPECT_EQ(holder->status1, OrbStatus(item));
        EXPECT_EQ(holder->hp, sub ? 150 : 200);
        if (sub) EXPECT_EQ((u32)holder->volatiles.substituteHP, 50);
    }
}

SINGLE_BATTLE_TEST("Polished status orbs cannot borrow the opposing Corrosion ability")
{
    struct BattlePokemon *holder = player;
    enum Item item = ITEM_TOXIC_ORB;
    for (u32 entry = 0; entry < ARRAY_COUNT(sOrbs); entry++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { item = sOrbs[entry]; holder = side ? opponent : player; }
    enum Species victim = item == ITEM_TOXIC_ORB ? SPECIES_ARBOK : SPECIES_CHARMANDER;
    GIVEN {
        PLAYER(holder == player ? victim : SPECIES_WOBBUFFET) { Ability(holder == player ? ABILITY_NONE : ABILITY_CORROSION); Item(holder == player ? item : ITEM_NONE); }
        OPPONENT(holder == opponent ? victim : SPECIES_WOBBUFFET) { Ability(holder == opponent ? ABILITY_NONE : ABILITY_CORROSION); Item(holder == opponent ? item : ITEM_NONE); }
    } WHEN { TURN { MOVE(holder, MOVE_SAFEGUARD); } }
    THEN { EXPECT_EQ(holder->status1, STATUS1_NONE); }
}

DOUBLE_BATTLE_TEST("Polished status orbs leave doubles Safeguard behavior native")
{
    enum Item item = ITEM_TOXIC_ORB;
    bool32 right = FALSE;
    for (u32 entry = 0; entry < ARRAY_COUNT(sOrbs); entry++)
        for (u32 slot = 0; slot < 2; slot++)
            PARAMETRIZE { item = sOrbs[entry]; right = slot; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(right ? ITEM_NONE : item); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Item(right ? item : ITEM_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); }
    } WHEN { TURN { MOVE(playerLeft, MOVE_SAFEGUARD); } }
    THEN { EXPECT_EQ(playerLeft->status1, STATUS1_NONE); EXPECT_EQ(playerRight->status1, STATUS1_NONE); }
}

SINGLE_BATTLE_TEST("Polished status orbs leave either active Terastal native")
{
    struct BattlePokemon *holder = player, *foe = opponent;
    enum Item item = ITEM_TOXIC_ORB;
    bool32 holderTera = FALSE;
    for (u32 entry = 0; entry < ARRAY_COUNT(sOrbs); entry++)
        for (u32 side = 0; side < 2; side++)
            for (u32 active = 0; active < 2; active++)
                PARAMETRIZE { item = sOrbs[entry]; holder = side ? opponent : player; foe = side ? player : opponent; holderTera = active; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); TeraType(TYPE_NORMAL); Item(holder == player ? item : ITEM_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); TeraType(TYPE_NORMAL); Item(holder == opponent ? item : ITEM_NONE); }
    } WHEN {
        TURN { MOVE(holder, MOVE_SAFEGUARD, gimmick: holderTera ? GIMMICK_TERA : GIMMICK_NONE); MOVE(foe, MOVE_CELEBRATE, gimmick: holderTera ? GIMMICK_NONE : GIMMICK_TERA); }
    } THEN { EXPECT_EQ(holder->status1, STATUS1_NONE); }
}

SINGLE_BATTLE_TEST("Polished status orbs retain optional item suppression")
{
    struct BattlePokemon *holder = player, *foe = opponent;
    enum Item item = ITEM_TOXIC_ORB;
    u32 gate = 0;
    for (u32 entry = 0; entry < ARRAY_COUNT(sOrbs); entry++)
        for (u32 side = 0; side < 2; side++)
            for (u32 g = 0; g < 3; g++)
                PARAMETRIZE { item = sOrbs[entry]; holder = side ? opponent : player; foe = side ? player : opponent; gate = g; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(holder == player && gate == 0 ? ABILITY_KLUTZ : ABILITY_NONE); Item(holder == player ? item : ITEM_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(holder == opponent && gate == 0 ? ABILITY_KLUTZ : ABILITY_NONE); Item(holder == opponent ? item : ITEM_NONE); }
    } WHEN {
        TURN { MOVE(holder, MOVE_SAFEGUARD); if (gate == 1) MOVE(foe, MOVE_MAGIC_ROOM); if (gate == 2) MOVE(foe, MOVE_EMBARGO); }
    } THEN { EXPECT_EQ(holder->status1, STATUS1_NONE); EXPECT_EQ(holder->item, item); }
}

SINGLE_BATTLE_TEST("Polished status orbs do not relax Safeguard for ordinary status moves")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_TOXIC;
    for (u32 entry = 0; entry < 2; entry++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = entry ? MOVE_WILL_O_WISP : MOVE_TOXIC; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(target, MOVE_SAFEGUARD); }
        TURN { MOVE(user, move, hit: TRUE); }
    } THEN { EXPECT_EQ(target->status1, STATUS1_NONE); }
}
