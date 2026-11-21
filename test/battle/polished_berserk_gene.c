#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished Berserk Gene resolves after entry abilities and before Mirror Herb")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_DOWNLOAD); Item(ITEM_BERSERK_GENE); }
        OPPONENT(SPECIES_WOBBUFFET) { Defense(100); SpDefense(200); Item(ITEM_MIRROR_HERB); }
    } WHEN { TURN {} }
    SCENE {
        if (B_POLISHED_MOVE_BALANCE) {
            ABILITY_POPUP(player, ABILITY_DOWNLOAD);
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, opponent);
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, player);
        } else {
            ABILITY_POPUP(player, ABILITY_DOWNLOAD);
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, player);
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, opponent);
        }
    } THEN {
        EXPECT_EQ(player->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 3);
        EXPECT_EQ(opponent->statStages[STAT_ATK], DEFAULT_STAT_STAGE + (B_POLISHED_MOVE_BALANCE ? 1 : 3));
    }
}

SINGLE_BATTLE_TEST("Polished Berserk Gene copies its actual boost before the first action")
{
    struct BattlePokemon *holder = player, *other = opponent;
    PARAMETRIZE { holder = player; other = opponent; }
    PARAMETRIZE { holder = opponent; other = player; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Item(holder == player ? ITEM_BERSERK_GENE : ITEM_MIRROR_HERB); Ability(ABILITY_OWN_TEMPO); }
        OPPONENT(SPECIES_WOBBUFFET) { Item(holder == opponent ? ITEM_BERSERK_GENE : ITEM_MIRROR_HERB); Ability(ABILITY_OWN_TEMPO); }
    } WHEN { TURN { MOVE(other, MOVE_SPLASH); } }
    SCENE {
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, holder);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, other);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, other);
    } THEN {
        EXPECT_EQ(holder->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 2);
        EXPECT_EQ(other->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 2);
        EXPECT_EQ(holder->item, ITEM_NONE);
        EXPECT_EQ(other->item, ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Berserk Gene waits until the turn after a voluntary switch")
{
    GIVEN {
        PLAYER(SPECIES_WYNAUT);
        PLAYER(SPECIES_WOBBUFFET) { Item(ITEM_BERSERK_GENE); Ability(ABILITY_OWN_TEMPO); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { SWITCH(player, 1); MOVE(opponent, MOVE_SPLASH); }
        TURN { MOVE(player, MOVE_SPLASH); }
    } SCENE {
        if (B_POLISHED_MOVE_BALANCE) {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, opponent);
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, player);
        } else {
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, player);
            ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, opponent);
        }
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, player);
    }
}

SINGLE_BATTLE_TEST("Polished Berserk Gene ignores Safeguard and expires within three checks")
{
    GIVEN {
        PLAYER(SPECIES_WYNAUT);
        PLAYER(SPECIES_WOBBUFFET) { Item(ITEM_BERSERK_GENE); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_SAFEGUARD); }
        TURN { SWITCH(player, 1); }
        TURN { MOVE(player, MOVE_SPLASH, WITH_RNG(RNG_CONFUSION, FALSE)); }
        TURN { MOVE(player, MOVE_SPLASH, WITH_RNG(RNG_CONFUSION, FALSE)); }
        TURN { MOVE(player, MOVE_SPLASH); }
    } SCENE {
        if (B_POLISHED_MOVE_BALANCE) {
            MESSAGE("Wobbuffet became confused!");
            MESSAGE("Wobbuffet snapped out of its confusion!");
        } else {
            NOT MESSAGE("Wobbuffet became confused!");
        }
    } THEN { EXPECT_EQ((u32)player->volatiles.confusionTimer, 0); }
}

SINGLE_BATTLE_TEST("Polished Berserk Gene uses a finite two or three confusion counter")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Item(ITEM_BERSERK_GENE); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN { TURN { MOVE(player, MOVE_SPLASH, WITH_RNG(RNG_CONFUSION, FALSE)); } }
    THEN {
        if (B_POLISHED_MOVE_BALANCE) {
            EXPECT_GE((u32)player->volatiles.confusionTimer, 1);
            EXPECT_LE((u32)player->volatiles.confusionTimer, 2);
        } else {
            EXPECT_EQ((u32)player->volatiles.confusionTimer, PERMANENT_VOLATILE);
        }
    }
}

SINGLE_BATTLE_TEST("Polished Berserk Gene gained through Trick waits until after the current actions")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Item(ITEM_BERSERK_GENE); Ability(ABILITY_KLUTZ); Speed(200); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_OWN_TEMPO); Speed(100); }
    } WHEN {
        TURN { MOVE(player, MOVE_TRICK); MOVE(opponent, MOVE_SPLASH); }
        TURN { MOVE(opponent, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TRICK, player);
        if (B_POLISHED_MOVE_BALANCE) {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, opponent);
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, opponent);
        } else {
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, opponent);
            ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, opponent);
        }
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, opponent);
    } THEN { EXPECT_EQ(opponent->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 2); }
}

SINGLE_BATTLE_TEST("Polished Berserk Gene retries a capped holder after a later stat drop")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Item(ITEM_BERSERK_GENE); Ability(ABILITY_KLUTZ); Speed(200); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_OWN_TEMPO); Speed(100); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_SWORDS_DANCE); }
        TURN { MOVE(opponent, MOVE_SWORDS_DANCE); }
        TURN { MOVE(opponent, MOVE_SWORDS_DANCE); }
        TURN { MOVE(player, MOVE_TRICK); }
        TURN { MOVE(player, MOVE_GROWL); }
        TURN { MOVE(opponent, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_GROWL, player);
        if (B_POLISHED_MOVE_BALANCE) ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, opponent);
        else { NOT ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, opponent); }
    } THEN {
        EXPECT_EQ(opponent->statStages[STAT_ATK], MAX_STAT_STAGE - !B_POLISHED_MOVE_BALANCE);
        EXPECT_EQ(opponent->item, B_POLISHED_MOVE_BALANCE ? ITEM_NONE : ITEM_BERSERK_GENE);
    }
}

SINGLE_BATTLE_TEST("Polished Berserk Gene respects Contrary direction and retains a failed item")
{
    u32 drops;
    PARAMETRIZE { drops = 0; }
    PARAMETRIZE { drops = 3; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Item(ITEM_BERSERK_GENE); Ability(ABILITY_KLUTZ); Speed(200); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_CONTRARY); Speed(100); }
    } WHEN {
        for (u32 j = 0; j < drops; j++) { TURN { MOVE(opponent, MOVE_SWORDS_DANCE); } }
        TURN { MOVE(player, MOVE_TRICK); }
        TURN { MOVE(opponent, MOVE_SPLASH, WITH_RNG(RNG_CONFUSION, FALSE)); }
    } THEN {
        EXPECT_EQ(opponent->statStages[STAT_ATK], drops ? MIN_STAT_STAGE : DEFAULT_STAT_STAGE - 2);
        EXPECT_EQ(opponent->item, drops ? ITEM_BERSERK_GENE : ITEM_NONE);
        if (drops) EXPECT_EQ((u32)opponent->volatiles.confusionTimer, 0);
    }
}

SINGLE_BATTLE_TEST("Polished Berserk Gene checks Own Tempo after Neutralizing Gas")
{
    bool32 gas;
    PARAMETRIZE { gas = FALSE; }
    PARAMETRIZE { gas = TRUE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Item(ITEM_BERSERK_GENE); Ability(ABILITY_OWN_TEMPO); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(gas ? ABILITY_NEUTRALIZING_GAS : ABILITY_NONE); }
    } WHEN { TURN { MOVE(player, MOVE_SPLASH, WITH_RNG(RNG_CONFUSION, FALSE)); } }
    THEN {
        EXPECT_EQ(player->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 2);
        EXPECT_EQ(player->item, ITEM_NONE);
        EXPECT_EQ(player->volatiles.confusionTimer != 0, gas);
    }
}

DOUBLE_BATTLE_TEST("Polished Berserk Gene preserves native immediate entry and permanent confusion in doubles")
{
    GIVEN {
        PLAYER(SPECIES_WYNAUT);
        PLAYER(SPECIES_WOBBUFFET);
        PLAYER(SPECIES_WOBBUFFET) { Item(ITEM_BERSERK_GENE); }
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN { TURN { SWITCH(playerLeft, 2); MOVE(opponentLeft, MOVE_SPLASH); } }
    SCENE {
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, playerLeft);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, opponentLeft);
    } THEN {
        EXPECT_EQ((u32)playerLeft->volatiles.confusionTimer, PERMANENT_VOLATILE);
        EXPECT_EQ(playerLeft->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 2);
    }
}

SINGLE_BATTLE_TEST("Polished Berserk Gene visits both holders in current Speed order")
{
    bool32 playerFirst;
    PARAMETRIZE { playerFirst = FALSE; }
    PARAMETRIZE { playerFirst = TRUE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Item(ITEM_BERSERK_GENE); Ability(ABILITY_OWN_TEMPO); Speed(playerFirst ? 200 : 100); }
        OPPONENT(SPECIES_WOBBUFFET) { Item(ITEM_BERSERK_GENE); Ability(ABILITY_OWN_TEMPO); Speed(playerFirst ? 100 : 200); }
    } WHEN { TURN {} }
    SCENE {
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, playerFirst ? player : opponent);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, playerFirst ? opponent : player);
    } THEN {
        EXPECT_EQ(player->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 2);
        EXPECT_EQ(opponent->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 2);
    }
}

SINGLE_BATTLE_TEST("Polished Berserk Gene preserves an existing confusion counter")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Item(ITEM_BERSERK_GENE); Ability(ABILITY_KLUTZ); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(200); }
    } WHEN {
        TURN { MOVE(player, MOVE_CONFUSE_RAY, WITH_RNG(RNG_CONFUSION_TURNS, 5)); }
        TURN { MOVE(player, MOVE_TRICK); MOVE(opponent, MOVE_SPLASH, WITH_RNG(RNG_CONFUSION, FALSE)); }
        TURN { MOVE(opponent, MOVE_SPLASH, WITH_RNG(RNG_CONFUSION, FALSE)); }
    } THEN {
        EXPECT_EQ((u32)opponent->volatiles.confusionTimer, 3);
        EXPECT_EQ(opponent->item, ITEM_NONE);
        EXPECT_EQ(opponent->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 2);
    }
}

SINGLE_BATTLE_TEST("Polished Berserk Gene is not blocked by Unnerve")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Item(ITEM_BERSERK_GENE); Ability(ABILITY_OWN_TEMPO); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_UNNERVE); Item(ITEM_MIRROR_HERB); }
    } WHEN { TURN {} }
    THEN {
        EXPECT_EQ(player->item, ITEM_NONE);
        EXPECT_EQ(opponent->item, ITEM_NONE);
        EXPECT_EQ(player->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 2);
        EXPECT_EQ(opponent->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 2);
    }
}

SINGLE_BATTLE_TEST("Polished Berserk Gene checks newly unsuppressed Klutz at the next boundary")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Item(ITEM_BERSERK_GENE); Ability(ABILITY_KLUTZ); Speed(200); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_OWN_TEMPO); Item(ITEM_MIRROR_HERB); Speed(100); }
    } WHEN {
        TURN { MOVE(player, MOVE_SKILL_SWAP); MOVE(opponent, MOVE_SPLASH); }
        TURN { MOVE(player, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SKILL_SWAP, player);
        if (B_POLISHED_MOVE_BALANCE) {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, opponent);
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, player);
        }
    } THEN {
        if (B_POLISHED_MOVE_BALANCE) {
            EXPECT_EQ(player->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 2);
            EXPECT_EQ(player->item, ITEM_NONE);
        }
        EXPECT_EQ(opponent->statStages[STAT_ATK], DEFAULT_STAT_STAGE);
        EXPECT_EQ(opponent->item, ITEM_MIRROR_HERB);
    }
}

SINGLE_BATTLE_TEST("Polished Berserk Gene can be cured by a bag item on the first action")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Item(ITEM_BERSERK_GENE); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN { TURN { USE_ITEM(player, ITEM_FULL_HEAL, partyIndex: 0); } }
    SCENE {
        ANIMATION(ANIM_TYPE_STATUS, B_ANIM_STATUS_CONFUSION, player);
        MESSAGE("Wobbuffet snapped out of its confusion!");
    } THEN {
        EXPECT_EQ((u32)player->volatiles.confusionTimer, 0);
        EXPECT_EQ(player->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 2);
    }
}

SINGLE_BATTLE_TEST("Polished Berserk Gene gives each source confusion duration equal probability")
{
    PASSES_RANDOMLY(1, B_POLISHED_MOVE_BALANCE ? 2 : 1, RNG_CONFUSION_TURNS);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Item(ITEM_BERSERK_GENE); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN { TURN { MOVE(player, MOVE_SPLASH, WITH_RNG(RNG_CONFUSION, FALSE)); } }
    THEN { EXPECT_EQ((u32)player->volatiles.confusionTimer, B_POLISHED_MOVE_BALANCE ? 1 : PERMANENT_VOLATILE); }
}

SINGLE_BATTLE_TEST("Polished Berserk Gene uses Trick Room order for simultaneous new holders")
{
    bool32 trickRoom;
    PARAMETRIZE { trickRoom = FALSE; }
    PARAMETRIZE { trickRoom = TRUE; }
    GIVEN {
        PLAYER(SPECIES_WYNAUT) { Speed(200); }
        PLAYER(SPECIES_WOBBUFFET) { Item(ITEM_BERSERK_GENE); Ability(ABILITY_OWN_TEMPO); Speed(200); }
        OPPONENT(SPECIES_WYNAUT) { Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Item(ITEM_BERSERK_GENE); Ability(ABILITY_OWN_TEMPO); Speed(100); }
    } WHEN {
        if (trickRoom) { TURN { MOVE(player, MOVE_TRICK_ROOM); } }
        TURN { SWITCH(player, 1); SWITCH(opponent, 1); }
        TURN { MOVE(player, MOVE_SPLASH); MOVE(opponent, MOVE_SPLASH); }
    } SCENE {
        if (B_POLISHED_MOVE_BALANCE) {
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, trickRoom ? opponent : player);
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, trickRoom ? player : opponent);
            ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, trickRoom ? opponent : player);
        }
    } THEN {
        EXPECT_EQ(player->item, ITEM_NONE);
        EXPECT_EQ(opponent->item, ITEM_NONE);
        EXPECT_EQ(player->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 2);
        EXPECT_EQ(opponent->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 2);
    }
}

SINGLE_BATTLE_TEST("Polished Berserk Gene does not activate on a holder knocked out before the next turn")
{
    GIVEN {
        PLAYER(SPECIES_WYNAUT);
        PLAYER(SPECIES_WOBBUFFET) { Item(ITEM_BERSERK_GENE); HP(1); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN { TURN { SWITCH(player, 1); MOVE(opponent, MOVE_TACKLE); SEND_OUT(player, 0); } }
    SCENE {
        if (B_POLISHED_MOVE_BALANCE) { NOT ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, player); }
        else ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, player);
        HP_BAR(player);
        MESSAGE("Wobbuffet fainted!");
    }
}
