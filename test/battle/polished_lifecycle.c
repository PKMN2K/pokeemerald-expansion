#include "global.h"
#include "test/battle.h"
#include "polished_lifecycle.h"

SINGLE_BATTLE_TEST("Polished lifecycle Magic Guard freezes the toxic counter")
{
    GIVEN {
        PLAYER(SPECIES_CLEFABLE) { Ability(ABILITY_MAGIC_GUARD); Status1(STATUS1_TOXIC_POISON); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        EXPECT_EQ(player->status1 & STATUS1_TOXIC_COUNTER, B_POLISHED_MOVE_BALANCE ? 0 : STATUS1_TOXIC_TURN(2));
        EXPECT_EQ(player->hp, player->maxHP);
    }
}

SINGLE_BATTLE_TEST("Polished lifecycle burn is one sixteenth without Heatproof halving")
{
    GIVEN {
        PLAYER(SPECIES_BRONZONG) { Ability(ABILITY_HEATPROOF); MaxHP(160); HP(160); Status1(STATUS1_BURN); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE); } }
    THEN { EXPECT_EQ(player->hp, B_POLISHED_MOVE_BALANCE ? 150 : 155); }
}

SINGLE_BATTLE_TEST("Polished lifecycle Black Sludge damages a non Poison Magic Guard holder")
{
    GIVEN {
        PLAYER(SPECIES_CLEFABLE) { Ability(ABILITY_MAGIC_GUARD); Item(ITEM_BLACK_SLUDGE); MaxHP(160); HP(160); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE); } }
    THEN { EXPECT_EQ(player->hp, B_POLISHED_MOVE_BALANCE ? 140 : 160); }
}

SINGLE_BATTLE_TEST("Polished lifecycle poison damage still triggers immediate healing berries")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); MaxHP(160); HP(90); Status1(STATUS1_POISON); Item(ITEM_SITRUS_BERRY); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); MaxHP(160); HP(160); Status1(STATUS1_POISON); }
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE); } }
    SCENE {
        HP_BAR(player, damage: 20);
        HP_BAR(player, damage: -40);
        HP_BAR(opponent, damage: 20);
    }
}

SINGLE_BATTLE_TEST("Polished lifecycle Leech Seed restores half its nominal drain after Big Root")
{
    enum Item item = ITEM_NONE;
    PARAMETRIZE { item = ITEM_NONE; }
    PARAMETRIZE { item = ITEM_BIG_ROOT; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Item(item); MaxHP(160); HP(50); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(160); HP(160); }
    } WHEN { TURN { MOVE(player, MOVE_LEECH_SEED); } }
    THEN {
        EXPECT_EQ(opponent->hp, 140);
        u32 restored = item == ITEM_BIG_ROOT ? 26 : 20;
        EXPECT_EQ(player->hp, 50 + (B_POLISHED_MOVE_BALANCE ? restored / 2 : restored));
    }
}

SINGLE_BATTLE_TEST("Polished lifecycle Liquid Ooze seed damage bypasses the receiver Magic Guard")
{
    GIVEN {
        PLAYER(SPECIES_CLEFABLE) { Ability(ABILITY_MAGIC_GUARD); MaxHP(160); HP(160); }
        OPPONENT(SPECIES_TENTACRUEL) { Ability(ABILITY_LIQUID_OOZE); MaxHP(160); HP(160); }
    } WHEN { TURN { MOVE(player, MOVE_LEECH_SEED); } }
    THEN {
        EXPECT_EQ(opponent->hp, 140);
        EXPECT_EQ(player->hp, B_POLISHED_MOVE_BALANCE ? 150 : 160);
    }
}

SINGLE_BATTLE_TEST("Polished lifecycle poison finishes both ticks before either faint animation")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); HP(1); Status1(STATUS1_POISON); }
        PLAYER(SPECIES_CHANSEY) { Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); HP(1); Status1(STATUS1_POISON); }
        OPPONENT(SPECIES_CHANSEY) { Speed(50); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); SEND_OUT(player, 1); SEND_OUT(opponent, 1); }
    } SCENE {
        HP_BAR(player);
        if (B_POLISHED_MOVE_BALANCE) {
            HP_BAR(opponent);
            MESSAGE("Wobbuffet fainted!");
        } else {
            MESSAGE("Wobbuffet fainted!");
            HP_BAR(opponent);
        }
        MESSAGE("The opposing Wobbuffet fainted!");
    } THEN {
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_HP), 0);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_HP), 0);
        EXPECT_EQ(player->species, SPECIES_CHANSEY);
        EXPECT_EQ(opponent->species, SPECIES_CHANSEY);
    }
}

SINGLE_BATTLE_TEST("Polished lifecycle simultaneous final poison knockouts are campaign losses")
{
    u32 speed = 100;
    PARAMETRIZE { speed = 100; }
    PARAMETRIZE { speed = 25; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(speed); HP(1); Status1(STATUS1_POISON); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); HP(1); Status1(STATUS1_POISON); }
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE); } }
    THEN {
        if (B_POLISHED_MOVE_BALANCE) {
            EXPECT_EQ(player->hp, 0);
            EXPECT_EQ(opponent->hp, 0);
            EXPECT_EQ(gBattleOutcome, B_OUTCOME_LOST);
        }
    }
}

SINGLE_BATTLE_TEST("Polished lifecycle weather batches simultaneous faints before deciding the result")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); HP(1); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); HP(1); }
    } WHEN { TURN { MOVE(player, MOVE_SANDSTORM); } }
    THEN {
        if (B_POLISHED_MOVE_BALANCE) {
            EXPECT_EQ(player->hp, 0);
            EXPECT_EQ(opponent->hp, 0);
            EXPECT_EQ(gBattleOutcome, B_OUTCOME_LOST);
        }
    }
}

SINGLE_BATTLE_TEST("Polished lifecycle poison replacement waits until the living opponent burns")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HP(1); Status1(STATUS1_POISON); }
        PLAYER(SPECIES_CHANSEY) { Item(ITEM_LEFTOVERS); MaxHP(160); HP(80); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(160); HP(160); Status1(STATUS1_BURN); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); SEND_OUT(player, 1); }
    } SCENE {
        MESSAGE("Wobbuffet fainted!");
        HP_BAR(opponent, damage: 10);
        MESSAGE("Go! Chansey!");
    } THEN {
        EXPECT_EQ(player->hp, 80);
        EXPECT_EQ(opponent->hp, 150);
    }
}

SINGLE_BATTLE_TEST("Polished lifecycle Poison Heal advances toxic at full HP")
{
    GIVEN {
        PLAYER(SPECIES_GLISCOR) { Ability(ABILITY_POISON_HEAL); Status1(STATUS1_TOXIC_POISON); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        EXPECT_EQ(player->hp, player->maxHP);
        EXPECT_EQ(player->status1 & STATUS1_TOXIC_COUNTER, STATUS1_TOXIC_TURN(2));
    }
}

SINGLE_BATTLE_TEST("Polished lifecycle recomputes residual Speed after rain expires")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_SWIFT_SWIM); Speed(100); MaxHP(1600); HP(1600); Status1(STATUS1_POISON); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(150); MaxHP(1600); HP(1600); Status1(STATUS1_POISON); }
    } WHEN {
        TURN { MOVE(player, MOVE_RAIN_DANCE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        MESSAGE("The rain stopped.");
        if (B_POLISHED_MOVE_BALANCE) {
            HP_BAR(opponent, damage: 200);
            HP_BAR(player, damage: 200);
        } else {
            HP_BAR(player, damage: 200);
            HP_BAR(opponent, damage: 200);
        }
    }
}

SINGLE_BATTLE_TEST("Polished lifecycle screens expire in current Speed order")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(100); }
    } WHEN {
        TURN { MOVE(player, MOVE_REFLECT); MOVE(opponent, MOVE_REFLECT); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        if (B_POLISHED_MOVE_BALANCE) {
            MESSAGE("The opposing side's Reflect wore off!");
            MESSAGE("Your side's Reflect wore off!");
        } else {
            MESSAGE("Your side's Reflect wore off!");
            MESSAGE("The opposing side's Reflect wore off!");
        }
    }
}

SINGLE_BATTLE_TEST("Polished lifecycle optional battle formats retain native scheduling eligibility")
{
    GIVEN { PLAYER(SPECIES_WOBBUFFET); OPPONENT(SPECIES_WOBBUFFET); }
    WHEN { TURN { MOVE(player, MOVE_CELEBRATE); } }
    THEN {
        u32 flags = gBattleTypeFlags;
        EXPECT_EQ(UsesPolishedLifecycle(), B_POLISHED_MOVE_BALANCE);
        const u32 formats[] = {BATTLE_TYPE_DOUBLE, BATTLE_TYPE_LINK,
            BATTLE_TYPE_FRONTIER, BATTLE_TYPE_TRAINER_HILL, BATTLE_TYPE_EREADER_TRAINER,
            BATTLE_TYPE_SAFARI, BATTLE_TYPE_CATCH_TUTORIAL};
        for (u32 i = 0; i < ARRAY_COUNT(formats); i++) {
            gBattleTypeFlags = flags | formats[i];
            EXPECT(!UsesPolishedLifecycle());
        }
        gBattleTypeFlags = flags;
        EXPECT(!PolishedEndTurnDefersFaints());
        EXPECT(!PolishedEndTurnDefersItems());
    }
}

SINGLE_BATTLE_TEST("Polished lifecycle Leppa waits until end turn and can be knocked off first")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); Item(ITEM_LEPPA_BERRY); MovesWithPP({MOVE_CELEBRATE, 1}); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); }
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE); MOVE(opponent, MOVE_KNOCK_OFF); } }
    THEN {
        EXPECT_EQ(player->pp[0], B_POLISHED_MOVE_BALANCE ? 0 : 10);
        EXPECT_EQ(player->item, ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished lifecycle Unnerve faint releases healing before the next residual phase")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); MaxHP(160); HP(1); Ability(ABILITY_UNNERVE); Status1(STATUS1_POISON); }
        PLAYER(SPECIES_CHANSEY) { Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); MaxHP(160); HP(5); Item(ITEM_SITRUS_BERRY); Status1(STATUS1_BURN); }
        OPPONENT(SPECIES_CHANSEY) { Speed(50); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); SEND_OUT(player, 1); if (!B_POLISHED_MOVE_BALANCE) SEND_OUT(opponent, 1); }
    } SCENE {
        MESSAGE("Wobbuffet fainted!");
        if (B_POLISHED_MOVE_BALANCE) {
            HP_BAR(opponent, damage: -40);
            HP_BAR(opponent, damage: 10);
        }
    } THEN {
        if (B_POLISHED_MOVE_BALANCE) {
            EXPECT_EQ(opponent->species, SPECIES_WOBBUFFET);
            EXPECT_EQ(opponent->hp, 35);
        }
    }
}

SINGLE_BATTLE_TEST("Polished lifecycle Unnerve switch releases Leppa before the opposing move")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_UNNERVE); }
        PLAYER(SPECIES_CHANSEY);
        OPPONENT(SPECIES_WOBBUFFET) { Item(ITEM_LEPPA_BERRY); MovesWithPP({MOVE_CELEBRATE, 0}, {MOVE_TACKLE, 10}); }
    } WHEN { TURN { SWITCH(player, 1); MOVE(opponent, MOVE_TACKLE); } }
    SCENE {
        if (B_POLISHED_MOVE_BALANCE) {
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, opponent);
            MESSAGE("The opposing Wobbuffet used Tackle!");
        }
    } THEN {
        if (B_POLISHED_MOVE_BALANCE) EXPECT_EQ(opponent->pp[0], 10);
    }
}

SINGLE_BATTLE_TEST("Polished lifecycle both lethal Future Sight attacks finish before faint resolution")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); HP(1); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); HP(1); Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(player, MOVE_FUTURE_SIGHT); MOVE(opponent, MOVE_FUTURE_SIGHT); }
        TURN {}
        TURN {}
    } SCENE {
        if (B_POLISHED_MOVE_BALANCE) {
            MESSAGE("The opposing Wobbuffet took the Future Sight attack!"); HP_BAR(opponent);
            MESSAGE("Wobbuffet took the Future Sight attack!"); HP_BAR(player);
            MESSAGE("The opposing Wobbuffet fainted!");
            MESSAGE("Wobbuffet fainted!");
        }
    } THEN {
        if (B_POLISHED_MOVE_BALANCE) {
            EXPECT_EQ(player->hp, 0);
            EXPECT_EQ(opponent->hp, 0);
            EXPECT_EQ(gBattleOutcome, B_OUTCOME_LOST);
        }
    }
}

SINGLE_BATTLE_TEST("Polished lifecycle entry item notification precedes entry abilities")
{
    GIVEN {
        PLAYER(SPECIES_GYARADOS) { Speed(100); Ability(ABILITY_INTIMIDATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); Item(ITEM_AIR_BALLOON); }
    } WHEN { TURN {} }
    SCENE {
        if (B_POLISHED_MOVE_BALANCE) {
            MESSAGE("The opposing Wobbuffet floats in the air with its Air Balloon!");
            ABILITY_POPUP(player, ABILITY_INTIMIDATE);
        } else {
            ABILITY_POPUP(player, ABILITY_INTIMIDATE);
            MESSAGE("The opposing Wobbuffet floats in the air with its Air Balloon!");
        }
    }
}

SINGLE_BATTLE_TEST("Polished lifecycle seed absorption floors at one and uses nominal damage on a KO")
{
    u32 maxHP;
    PARAMETRIZE { maxHP = 7; }
    PARAMETRIZE { maxHP = 8; }
    PARAMETRIZE { maxHP = 99; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(160); HP(30); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(maxHP); HP(1); }
        OPPONENT(SPECIES_CHANSEY);
    } WHEN { TURN { MOVE(player, MOVE_LEECH_SEED); SEND_OUT(opponent, 1); } }
    THEN {
        u32 nominal = maxHP < 8 ? 1 : maxHP / 8;
        EXPECT_EQ(player->hp, 30 + (B_POLISHED_MOVE_BALANCE ? (nominal < 2 ? 1 : nominal / 2) : nominal));
    }
}

SINGLE_BATTLE_TEST("Polished lifecycle Leppa consumption follows the Harvest recovery phase")
{
    GIVEN {
        PLAYER(SPECIES_EXEGGUTOR) { Ability(ABILITY_HARVEST); Item(ITEM_LEPPA_BERRY); MovesWithPP({MOVE_CELEBRATE, 1}); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE); MOVE(opponent, MOVE_SUNNY_DAY); } }
    THEN {
        EXPECT_EQ(player->pp[0], 10);
        EXPECT_EQ(player->item, B_POLISHED_MOVE_BALANCE ? ITEM_NONE : ITEM_LEPPA_BERRY);
    }
}

SINGLE_BATTLE_TEST("Polished lifecycle Trick Room expiry refreshes Speed before block B")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); Ability(ABILITY_SPEED_BOOST); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); Ability(ABILITY_SPEED_BOOST); }
    } WHEN {
        TURN { MOVE(player, MOVE_TRICK_ROOM); }
        TURN {}
        TURN {}
        TURN {}
        TURN {}
    } SCENE {
        // The source reverses each phase while Trick Room is active. Native
        // end-turn sorting always runs fast-to-slow, including under Trick Room.
        if (B_POLISHED_MOVE_BALANCE) {
            ABILITY_POPUP(opponent, ABILITY_SPEED_BOOST);
            ABILITY_POPUP(player, ABILITY_SPEED_BOOST);
        } else {
            ABILITY_POPUP(player, ABILITY_SPEED_BOOST);
            ABILITY_POPUP(opponent, ABILITY_SPEED_BOOST);
        }
        MESSAGE("The twisted dimensions returned to normal!");
        ABILITY_POPUP(player, ABILITY_SPEED_BOOST);
        ABILITY_POPUP(opponent, ABILITY_SPEED_BOOST);
    }
}

SINGLE_BATTLE_TEST("Polished lifecycle Room Service changes the order of incoming abilities")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(50); }
        PLAYER(SPECIES_EKANS) { Speed(135); Ability(ABILITY_INTIMIDATE); Item(ITEM_ROOM_SERVICE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(100); }
        OPPONENT(SPECIES_GYARADOS) { Speed(100); Ability(ABILITY_INTIMIDATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_TRICK_ROOM); }
        TURN { SWITCH(player, 1); SWITCH(opponent, 1); }
    } SCENE {
        if (B_POLISHED_MOVE_BALANCE) {
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, player);
            ABILITY_POPUP(player, ABILITY_INTIMIDATE);
            ABILITY_POPUP(opponent, ABILITY_INTIMIDATE);
        } else {
            // Native entry is fast-to-slow and completes each entrant's
            // abilities/items before moving to the next entrant.
            ABILITY_POPUP(player, ABILITY_INTIMIDATE);
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, player);
            ABILITY_POPUP(opponent, ABILITY_INTIMIDATE);
        }
    } THEN { EXPECT_EQ(player->statStages[STAT_SPEED], DEFAULT_STAT_STAGE - 1); }
}
