#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished stat items copy Download before the slower entry ability")
{
    GIVEN {
        PLAYER(SPECIES_PORYGON) { Ability(ABILITY_DOWNLOAD); Speed(100); }
        OPPONENT(SPECIES_ARBOK) { Ability(ABILITY_INTIMIDATE); Item(ITEM_MIRROR_HERB); Speed(50); Defense(100); SpDefense(200); }
    } WHEN { TURN {} }
    SCENE {
        ABILITY_POPUP(player, ABILITY_DOWNLOAD);
        if (B_POLISHED_MOVE_BALANCE) {
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, opponent);
            ABILITY_POPUP(opponent, ABILITY_INTIMIDATE);
        } else {
            ABILITY_POPUP(opponent, ABILITY_INTIMIDATE);
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, opponent);
        }
    } THEN {
        EXPECT_EQ(player->statStages[STAT_ATK], DEFAULT_STAT_STAGE);
        EXPECT_EQ(opponent->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 1);
    }
}

SINGLE_BATTLE_TEST("Polished stat items copy Weak Armor after the first hit only")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Item(ITEM_MIRROR_HERB); }
        OPPONENT(SPECIES_MAGCARGO) { Ability(ABILITY_WEAK_ARMOR); MaxHP(1600); HP(1600); Defense(1000); }
    } WHEN { TURN { MOVE(player, MOVE_DOUBLE_KICK); } }
    SCENE {
        HP_BAR(opponent);
        ABILITY_POPUP(opponent, ABILITY_WEAK_ARMOR);
        if (B_POLISHED_MOVE_BALANCE) ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, player);
        HP_BAR(opponent);
        ABILITY_POPUP(opponent, ABILITY_WEAK_ARMOR);
        if (!B_POLISHED_MOVE_BALANCE) ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, player);
    } THEN {
        EXPECT_EQ(player->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + (B_POLISHED_MOVE_BALANCE ? 2 : 4));
        EXPECT_EQ(opponent->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + 4);
        EXPECT_EQ(opponent->statStages[STAT_DEF], DEFAULT_STAT_STAGE - 2);
    }
}

SINGLE_BATTLE_TEST("Polished stat items copy Weak Armor before White Herb restores Defense")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Item(ITEM_MIRROR_HERB); Speed(50); }
        OPPONENT(SPECIES_MAGCARGO) { Ability(ABILITY_WEAK_ARMOR); Item(ITEM_WHITE_HERB); Speed(100); }
    } WHEN { TURN { MOVE(player, MOVE_TACKLE); } }
    SCENE {
        ABILITY_POPUP(opponent, ABILITY_WEAK_ARMOR);
        if (B_POLISHED_MOVE_BALANCE) {
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, player);
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, opponent);
        } else {
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, opponent);
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, player);
        }
    } THEN {
        EXPECT_EQ(opponent->statStages[STAT_DEF], DEFAULT_STAT_STAGE);
        EXPECT_EQ(player->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + 2);
    }
}

SINGLE_BATTLE_TEST("Polished stat items preserve the primary queue across nested Defiant and Competitive")
{
    enum Ability ability;
    enum Stat stat;
    PARAMETRIZE { ability = ABILITY_DEFIANT; stat = STAT_ATK; }
    PARAMETRIZE { ability = ABILITY_COMPETITIVE; stat = STAT_SPATK; }
    // Tickle is optional content: exercise the source ability checkpoint while
    // preserving the native move's two-stat continuation.
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Item(ITEM_MIRROR_HERB); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ability); }
    } WHEN { TURN { MOVE(player, MOVE_TICKLE); } }
    SCENE {
        ABILITY_POPUP(opponent, ability);
        if (B_POLISHED_MOVE_BALANCE) ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, player);
        ABILITY_POPUP(opponent, ability);
        if (!B_POLISHED_MOVE_BALANCE) ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, player);
    } THEN {
        EXPECT_EQ(player->statStages[stat], DEFAULT_STAT_STAGE + (B_POLISHED_MOVE_BALANCE ? 2 : 4));
        EXPECT_EQ(opponent->statStages[stat], DEFAULT_STAT_STAGE + (ability == ABILITY_DEFIANT ? 3 : 4));
        EXPECT_EQ(opponent->statStages[STAT_DEF], DEFAULT_STAT_STAGE - 1);
    }
}

SINGLE_BATTLE_TEST("Polished stat items copy Tangling Hair Contrary boosts between hits")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_CONTRARY); }
        OPPONENT(SPECIES_DUGTRIO_ALOLA) { Ability(ABILITY_TANGLING_HAIR); Item(ITEM_MIRROR_HERB); MaxHP(1600); HP(1600); Defense(1000); }
    } WHEN { TURN { MOVE(player, MOVE_DOUBLE_KICK); } }
    THEN {
        EXPECT_EQ(player->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + 2);
        EXPECT_EQ(opponent->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + (B_POLISHED_MOVE_BALANCE ? 1 : 2));
    }
}

SINGLE_BATTLE_TEST("Polished stat items copy Justified between hits and honor Contrary on the holder")
{
    enum Ability ability;
    PARAMETRIZE { ability = ABILITY_NONE; }
    PARAMETRIZE { ability = ABILITY_CONTRARY; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ability); Item(ITEM_MIRROR_HERB); }
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_ARCANINE) { Ability(ABILITY_JUSTIFIED); MaxHP(1600); HP(1600); Defense(1000); }
    } WHEN { TURN { MOVE(player, MOVE_BEAT_UP); } }
    THEN {
        EXPECT_EQ(player->statStages[STAT_ATK], DEFAULT_STAT_STAGE + (ability == ABILITY_CONTRARY ? -1 : 1) * (B_POLISHED_MOVE_BALANCE ? 1 : 2));
        EXPECT_EQ(opponent->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 2);
    }
}

SINGLE_BATTLE_TEST("Polished stat items copy Speed Boost before the next end turn ability")
{
    GIVEN {
        PLAYER(SPECIES_NINJASK) { Ability(ABILITY_SPEED_BOOST); Speed(100); }
        OPPONENT(SPECIES_OCTILLERY) { Ability(ABILITY_MOODY); Item(ITEM_MIRROR_HERB); Speed(50); }
    } WHEN { TURN {} }
    SCENE {
        ABILITY_POPUP(player, ABILITY_SPEED_BOOST);
        if (B_POLISHED_MOVE_BALANCE) ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, opponent);
        ABILITY_POPUP(opponent, ABILITY_MOODY);
        if (!B_POLISHED_MOVE_BALANCE) ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, opponent);
    }
}

SINGLE_BATTLE_TEST("Polished stat items leave White Herb waiting after Moody")
{
    GIVEN {
        PLAYER(SPECIES_OCTILLERY) { Ability(ABILITY_MOODY); Item(ITEM_WHITE_HERB); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN { TURN {} }
    SCENE {
        ABILITY_POPUP(player, ABILITY_MOODY);
        if (B_POLISHED_MOVE_BALANCE) { NOT ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, player); }
        else ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, player);
    } THEN {
        u32 negative = 0;
        for (u32 stat = STAT_ATK; stat < NUM_BATTLE_STATS; stat++)
            negative += player->statStages[stat] < DEFAULT_STAT_STAGE;
        EXPECT_EQ(negative, !!B_POLISHED_MOVE_BALANCE);
        EXPECT_EQ(player->item, B_POLISHED_MOVE_BALANCE ? ITEM_WHITE_HERB : ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished stat items restore Moody's negative stage at the next move checkpoint")
{
    GIVEN {
        PLAYER(SPECIES_OCTILLERY) { Ability(ABILITY_MOODY); Item(ITEM_WHITE_HERB); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); }
    } WHEN {
        TURN {}
        TURN { MOVE(player, MOVE_SPLASH); }
    } SCENE {
        ABILITY_POPUP(player, ABILITY_MOODY);
        if (!B_POLISHED_MOVE_BALANCE) ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, player);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, player);
        if (B_POLISHED_MOVE_BALANCE) ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, player);
        ABILITY_POPUP(player, ABILITY_MOODY);
    } THEN { EXPECT_EQ(player->item, ITEM_NONE); }
}

SINGLE_BATTLE_TEST("Polished stat items defer Moody's Eject Pack to the next action")
{
    GIVEN {
        PLAYER(SPECIES_OCTILLERY) { Ability(ABILITY_MOODY); Item(ITEM_EJECT_PACK); Speed(100); }
        PLAYER(SPECIES_CHANSEY) { Speed(100); Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); }
    } WHEN {
        TURN { if (!B_POLISHED_MOVE_BALANCE) SEND_OUT(player, 1); }
        TURN { MOVE(player, MOVE_SPLASH); if (B_POLISHED_MOVE_BALANCE) SEND_OUT(player, 1); }
    } SCENE {
        ABILITY_POPUP(player, ABILITY_MOODY);
        if (!B_POLISHED_MOVE_BALANCE) MESSAGE("Octillery is switched out with the Eject Pack!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, player);
        if (B_POLISHED_MOVE_BALANCE) MESSAGE("Octillery is switched out with the Eject Pack!");
    } THEN { EXPECT_EQ(player->species, SPECIES_CHANSEY); }
}

SINGLE_BATTLE_TEST("Polished stat items keep an inherited negative stage until an action checkpoint")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(50); }
        PLAYER(SPECIES_CHANSEY) { Speed(50); Item(ITEM_WHITE_HERB); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(100); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_GROWL); }
        TURN { MOVE(player, MOVE_BATON_PASS); SEND_OUT(player, 1); }
    } THEN {
        EXPECT_EQ(player->statStages[STAT_ATK], DEFAULT_STAT_STAGE - !!B_POLISHED_MOVE_BALANCE);
        EXPECT_EQ(player->item, B_POLISHED_MOVE_BALANCE ? ITEM_WHITE_HERB : ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished stat items retain White Herb on optional Sticky Web entry")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        PLAYER(SPECIES_CHANSEY) { Item(ITEM_WHITE_HERB); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(opponent, MOVE_STICKY_WEB); }
        TURN { SWITCH(player, 1); }
    } THEN {
        EXPECT_EQ(player->statStages[STAT_SPEED], DEFAULT_STAT_STAGE);
        EXPECT_EQ(player->item, ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished stat items copy Anger Point before Rocky Helmet damage")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Item(ITEM_MIRROR_HERB); }
        OPPONENT(SPECIES_TAUROS) { Ability(ABILITY_ANGER_POINT); Item(ITEM_ROCKY_HELMET); MaxHP(1600); HP(1600); }
    } WHEN { TURN { MOVE(player, MOVE_TACKLE, criticalHit: TRUE); } }
    SCENE {
        HP_BAR(opponent);
        ABILITY_POPUP(opponent, ABILITY_ANGER_POINT);
        if (B_POLISHED_MOVE_BALANCE) ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, player);
        HP_BAR(player);
        if (!B_POLISHED_MOVE_BALANCE) ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, player);
    } THEN { EXPECT_EQ(player->statStages[STAT_ATK], MAX_STAT_STAGE); }
}

SINGLE_BATTLE_TEST("Polished stat items do not copy Moxie onto a fainted holder or its replacement")
{
    GIVEN {
        PLAYER(SPECIES_GYARADOS) { Ability(ABILITY_MOXIE); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1); Item(ITEM_MIRROR_HERB); }
        OPPONENT(SPECIES_CHANSEY) { Item(ITEM_MIRROR_HERB); }
    } WHEN { TURN { MOVE(player, MOVE_TACKLE); SEND_OUT(opponent, 1); } }
    SCENE {
        ABILITY_POPUP(player, ABILITY_MOXIE);
        NOT ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, opponent);
    } THEN {
        EXPECT_EQ(player->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 1);
        EXPECT_EQ(opponent->statStages[STAT_ATK], DEFAULT_STAT_STAGE);
        EXPECT_EQ(opponent->item, ITEM_MIRROR_HERB);
    }
}

DOUBLE_BATTLE_TEST("Polished stat items retain native multihit copying in doubles")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Item(ITEM_MIRROR_HERB); }
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_MAGCARGO) { Ability(ABILITY_WEAK_ARMOR); MaxHP(1600); HP(1600); Defense(1000); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN { TURN { MOVE(playerLeft, MOVE_DOUBLE_KICK, target: opponentLeft); } }
    SCENE {
        HP_BAR(opponentLeft);
        ABILITY_POPUP(opponentLeft, ABILITY_WEAK_ARMOR);
        HP_BAR(opponentLeft);
        ABILITY_POPUP(opponentLeft, ABILITY_WEAK_ARMOR);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, playerLeft);
    } THEN {
        EXPECT_EQ(playerLeft->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + 4);
        EXPECT_EQ(opponentLeft->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + 4);
    }
}
