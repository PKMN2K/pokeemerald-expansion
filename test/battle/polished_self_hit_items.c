#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished self hit Hi Jump Kick crashes defer berries and bypass Magic Guard while optional moves stay native")
{
    struct BattlePokemon *user = player, *target = opponent;
    u32 failure = 0;
    bool32 magicGuard = FALSE, optional = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 3; kind++)
            for (u32 guard = 0; guard < 2; guard++)
                for (u32 extra = 0; extra < 2; extra++)
                    PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; failure = kind; magicGuard = guard; optional = extra; }
    bool32 source = B_POLISHED_MOVE_BALANCE && !optional;
    GIVEN {
        PLAYER(target == player && failure == 2 ? SPECIES_GENGAR : SPECIES_WOBBUFFET) { Ability(user == player && magicGuard ? ABILITY_MAGIC_GUARD : ABILITY_NONE); Speed(user == player ? 200 : 100); MaxHP(160); HP(user == player ? 101 : 160); if (user == player) { Status1(STATUS1_POISON); Item(ITEM_SITRUS_BERRY); } }
        OPPONENT(target == opponent && failure == 2 ? SPECIES_GENGAR : SPECIES_WOBBUFFET) { Ability(user == opponent && magicGuard ? ABILITY_MAGIC_GUARD : ABILITY_NONE); Speed(user == opponent ? 200 : 100); MaxHP(160); HP(user == opponent ? 101 : 160); if (user == opponent) { Status1(STATUS1_POISON); Item(ITEM_SITRUS_BERRY); } }
    } WHEN {
        TURN { MOVE(user, optional ? MOVE_JUMP_KICK : MOVE_HI_JUMP_KICK, hit: failure != 0); MOVE(target, failure == 1 ? MOVE_PROTECT : MOVE_SPLASH); }
    } SCENE {
        if (user == player) MESSAGE("Wobbuffet kept going and crashed!");
        else MESSAGE("The opposing Wobbuffet kept going and crashed!");
        if (source || !magicGuard) HP_BAR(user, damage: 80);
        if (!source && !magicGuard) {
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, user);
            HP_BAR(user, damage: -40);
        }
        if (!magicGuard) HP_BAR(user, damage: 20);
        if (source) {
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, user);
            HP_BAR(user, damage: -40);
        }
    } THEN {
        EXPECT_EQ(user->hp, magicGuard ? source ? 61 : 101 : 41);
        EXPECT_EQ(user->item, !source && magicGuard ? ITEM_SITRUS_BERRY : ITEM_NONE);
        EXPECT_EQ(gBattleStruct->polishedSelfHitBattlers, 0);
        EXPECT_EQ(gBattleStruct->polishedSelfHitHpWrites, 0);
    }
}

SINGLE_BATTLE_TEST("Polished self hit berries wait for actual damage or the final healing sweep")
{
    struct BattlePokemon *holder = player, *other = opponent;
    u32 followup = 0, tera = 0;
    for (u32 side = 0; side < 2; side++)
        for (u32 action = 0; action < 3; action++)
            for (u32 gimmick = 0; gimmick < 3; gimmick++)
                PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; followup = action; tera = gimmick; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Level(50); Attack(100); Defense(100); Speed(holder == player ? 200 : 100); MaxHP(100); HP(holder == player ? 51 : 100); TeraType(TYPE_PSYCHIC); if (holder == player) Item(ITEM_SITRUS_BERRY); }
        PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Speed(80); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Level(50); Attack(100); Defense(100); Speed(holder == opponent ? 200 : 100); MaxHP(100); HP(holder == opponent ? 51 : 100); TeraType(TYPE_PSYCHIC); if (holder == opponent) Item(ITEM_SITRUS_BERRY); }
        OPPONENT(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Speed(80); }
    } WHEN {
        TURN { MOVE(holder, MOVE_SPLASH, gimmick: tera == 1 ? GIMMICK_TERA : GIMMICK_NONE); MOVE(other, MOVE_CONFUSE_RAY, gimmick: tera == 2 ? GIMMICK_TERA : GIMMICK_NONE); }
        TURN {
            MOVE(holder, MOVE_TACKLE, WITH_RNG(RNG_CONFUSION, TRUE));
            MOVE(other, followup == 0 ? MOVE_SPLASH : followup == 1 ? MOVE_SONIC_BOOM : MOVE_DRAGON_RAGE);
            if (B_POLISHED_MOVE_BALANCE && tera == 0 && followup == 2) SEND_OUT(holder, 1);
        }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CONFUSE_RAY, other);
        MESSAGE("It hurt itself in its confusion!");
        HP_BAR(holder, damage: 19);
        if (!B_POLISHED_MOVE_BALANCE || tera != 0) {
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, holder);
            HP_BAR(holder, damage: -25);
        }
        ANIMATION(ANIM_TYPE_MOVE, followup == 0 ? MOVE_SPLASH : followup == 1 ? MOVE_SONIC_BOOM : MOVE_DRAGON_RAGE, other);
        if (followup != 0) HP_BAR(holder, damage: followup == 1 ? 20 : B_POLISHED_MOVE_BALANCE && tera == 0 ? 32 : 40);
        if (B_POLISHED_MOVE_BALANCE && tera == 0) {
            if (followup == 2) {
                NOT ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, holder);
                if (holder == player) MESSAGE("Wobbuffet fainted!");
                else MESSAGE("The opposing Wobbuffet fainted!");
            } else {
                ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, holder);
                HP_BAR(holder, damage: -25);
            }
        }
    } THEN {
        if (B_POLISHED_MOVE_BALANCE && tera == 0 && followup == 2) {
            EXPECT_EQ(holder->species, SPECIES_CHANSEY);
            EXPECT_EQ(GetMonData(&gParties[holder == player ? B_TRAINER_PLAYER : B_TRAINER_OPPONENT_A][0], MON_DATA_HP), 0);
        } else {
            EXPECT_EQ(holder->hp, followup == 0 ? 57 : followup == 1 ? 37 : 17);
            EXPECT_EQ(holder->item, ITEM_NONE);
        }
        EXPECT_EQ(gBattleStruct->polishedSelfHitBattlers, 0);
    }
}

SINGLE_BATTLE_TEST("Polished self hit defers HP and stat berries past a harmless action into residual healing")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Item item = ITEM_SITRUS_BERRY;
    static const enum Item items[] = {ITEM_SITRUS_BERRY, ITEM_ORAN_BERRY, ITEM_SALAC_BERRY};
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < ARRAY_COUNT(items); kind++)
            PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; item = items[kind]; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Level(50); Attack(item == ITEM_SALAC_BERRY ? 278 : 100); Defense(100); Speed(holder == player ? 200 : 100); MaxHP(160); HP(holder == player ? 101 : 160); if (holder == player) { Item(item); Status1(STATUS1_POISON); } }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Level(50); Attack(item == ITEM_SALAC_BERRY ? 278 : 100); Defense(100); Speed(holder == opponent ? 200 : 100); MaxHP(160); HP(holder == opponent ? 101 : 160); if (holder == opponent) { Item(item); Status1(STATUS1_POISON); } }
    } WHEN {
        TURN { MOVE(holder, MOVE_SPLASH); MOVE(other, MOVE_CONFUSE_RAY); }
        TURN { MOVE(holder, MOVE_TACKLE, WITH_RNG(RNG_CONFUSION, TRUE)); MOVE(other, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CONFUSE_RAY, other);
        HP_BAR(holder, damage: 20);
        MESSAGE("It hurt itself in its confusion!");
        HP_BAR(holder, damage: item == ITEM_SALAC_BERRY ? 50 : 19);
        if (!B_POLISHED_MOVE_BALANCE) {
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, holder);
            if (item != ITEM_SALAC_BERRY) HP_BAR(holder, damage: item == ITEM_SITRUS_BERRY ? -40 : -10);
            else ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_STATS_CHANGE, holder);
        }
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, other);
        HP_BAR(holder, damage: 20);
        if (B_POLISHED_MOVE_BALANCE) {
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, holder);
            if (item != ITEM_SALAC_BERRY) HP_BAR(holder, damage: item == ITEM_SITRUS_BERRY ? -40 : -10);
            else ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_STATS_CHANGE, holder);
        }
    } THEN {
        EXPECT_EQ(holder->hp, item == ITEM_SITRUS_BERRY ? 82 : item == ITEM_ORAN_BERRY ? 52 : 11);
        EXPECT_EQ(holder->item, ITEM_NONE);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + (item == ITEM_SALAC_BERRY));
        EXPECT_EQ(gBattleStruct->polishedSelfHitBattlers, 0);
    }
}

WILD_BATTLE_TEST("Polished self hit from disobedience waits for the pending opponents actual damage")
{
    u32 otId = 1;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Level(50); Attack(100); Defense(100); Speed(200); MaxHP(100); HP(51); Item(ITEM_SITRUS_BERRY); MovesWithPP({MOVE_GROWL, 10}); }
        if (B_POLISHED_MOVE_BALANCE) SetMonData(&PLAYER_PARTY[0], MON_DATA_OT_ID, &otId);
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); Moves(MOVE_SONIC_BOOM); }
    } WHEN {
        TURN {
            MOVE(player, MOVE_GROWL, WITH_RNG(RNG_POLISHED_OBEDIENCE_OUTCOME, 60));
            MOVE(opponent, MOVE_SONIC_BOOM);
        }
    } SCENE {
        if (B_POLISHED_MOVE_BALANCE) HP_BAR(player, damage: 19);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SONIC_BOOM, opponent);
        HP_BAR(player, damage: 20);
        HP_BAR(player, damage: -25);
    } THEN {
        EXPECT_EQ(player->hp, B_POLISHED_MOVE_BALANCE ? 37 : 56);
        EXPECT_EQ(player->item, ITEM_NONE);
        EXPECT_EQ(player->pp[0], B_POLISHED_MOVE_BALANCE ? 10 : 9);
        EXPECT_EQ(gBattleStruct->polishedSelfHitBattlers, 0);
    }
}
