#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished action Aftermath damages Magic Guard on a source contact knockout")
{
    GIVEN {
        PLAYER(SPECIES_CLEFABLE) { Ability(ABILITY_MAGIC_GUARD); MaxHP(160); HP(160); }
        OPPONENT(SPECIES_VOLTORB) { Ability(ABILITY_AFTERMATH); HP(1); }
    } WHEN { TURN { MOVE(player, MOVE_TACKLE); } }
    THEN { EXPECT_EQ(player->hp, B_POLISHED_MOVE_BALANCE ? 120 : 160); }
}

SINGLE_BATTLE_TEST("Polished action Aftermath uses the source Feint Attack contact identity")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); MaxHP(160); HP(160); }
        OPPONENT(SPECIES_VOLTORB) { Ability(ABILITY_AFTERMATH); HP(1); }
    } WHEN { TURN { MOVE(player, MOVE_FEINT_ATTACK); } }
    THEN { EXPECT_EQ(player->hp, 120); }
}

SINGLE_BATTLE_TEST("Polished action Rocky Helmet triggers on the exact Substitute breaking hit")
{
    u32 level = 49;
    PARAMETRIZE { level = 49; }
    PARAMETRIZE { level = 50; }
    PARAMETRIZE { level = 51; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Level(level); MaxHP(120); HP(120); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_ROCKY_HELMET); MaxHP(200); HP(200); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_SUBSTITUTE); }
        TURN { MOVE(player, MOVE_SEISMIC_TOSS); }
    } THEN { EXPECT_EQ(player->hp, B_POLISHED_MOVE_BALANCE && level >= 50 ? 100 : 120); }
}

SINGLE_BATTLE_TEST("Polished action Air Balloon waits until Substitute breaks")
{
    u32 level = 49;
    PARAMETRIZE { level = 49; }
    PARAMETRIZE { level = 50; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Level(level); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_AIR_BALLOON); MaxHP(200); HP(200); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_SUBSTITUTE); }
        TURN { MOVE(player, MOVE_SEISMIC_TOSS); }
    } THEN { EXPECT_EQ(opponent->item, B_POLISHED_MOVE_BALANCE && level < 50 ? ITEM_AIR_BALLOON : ITEM_NONE); }
}

SINGLE_BATTLE_TEST("Polished action Air Balloon is retained by its fainted holder")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_AIR_BALLOON); HP(1); }
    } WHEN { TURN { MOVE(player, MOVE_TACKLE); } }
    THEN { EXPECT_EQ(opponent->item, B_POLISHED_MOVE_BALANCE ? ITEM_AIR_BALLOON : ITEM_NONE); }
}

SINGLE_BATTLE_TEST("Polished action move recoil precedes Rocky Helmet damage")
{
    s16 dealt, first, second;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); MaxHP(400); HP(400); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_ROCKY_HELMET); MaxHP(1600); HP(1600); }
    } WHEN { TURN { MOVE(player, MOVE_DOUBLE_EDGE); } }
    SCENE {
        HP_BAR(opponent, captureDamage: &dealt);
        HP_BAR(player, captureDamage: &first);
        HP_BAR(player, captureDamage: &second);
    } THEN {
        EXPECT_EQ(first, B_POLISHED_MOVE_BALANCE ? dealt / 3 : 400 / 6);
        EXPECT_EQ(second, B_POLISHED_MOVE_BALANCE ? 400 / 6 : dealt * 33 / 100);
    }
}

SINGLE_BATTLE_TEST("Polished action lethal move recoil prevents later Rocky Helmet damage")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); HP(1); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_ROCKY_HELMET); MaxHP(1600); HP(1600); }
    } WHEN { TURN { MOVE(player, MOVE_DOUBLE_EDGE); } }
    SCENE {
        if (B_POLISHED_MOVE_BALANCE) MESSAGE("Wobbuffet was damaged by the recoil!");
        else NOT MESSAGE("Wobbuffet was damaged by the recoil!");
    }
}

SINGLE_BATTLE_TEST("Polished action Moxie precedes Aftermath before either faint resolves")
{
    GIVEN {
        PLAYER(SPECIES_GYARADOS) { Ability(ABILITY_MOXIE); MaxHP(160); HP(160); }
        OPPONENT(SPECIES_VOLTORB) { Ability(ABILITY_AFTERMATH); HP(1); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN { TURN { MOVE(player, MOVE_TACKLE); SEND_OUT(opponent, 1); } }
    SCENE {
        if (B_POLISHED_MOVE_BALANCE) {
            ABILITY_POPUP(player, ABILITY_MOXIE);
            ABILITY_POPUP(opponent, ABILITY_AFTERMATH);
            MESSAGE("The opposing Voltorb fainted!");
        } else {
            ABILITY_POPUP(opponent, ABILITY_AFTERMATH);
            MESSAGE("The opposing Voltorb fainted!");
            ABILITY_POPUP(player, ABILITY_MOXIE);
        }
    } THEN { EXPECT_EQ(player->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 1); }
}

SINGLE_BATTLE_TEST("Polished action Destiny Bond replaces the faint ability branch")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); MaxHP(160); HP(160); }
        OPPONENT(SPECIES_VOLTORB) { Ability(ABILITY_AFTERMATH); Speed(100); HP(1); }
    } WHEN { TURN { MOVE(opponent, MOVE_DESTINY_BOND); MOVE(player, MOVE_TACKLE); } }
    SCENE {
        if (B_POLISHED_MOVE_BALANCE) NOT ABILITY_POPUP(opponent, ABILITY_AFTERMATH);
        else ABILITY_POPUP(opponent, ABILITY_AFTERMATH);
    } THEN { EXPECT_EQ(player->hp, 0); EXPECT_EQ(opponent->hp, 0); }
}

SINGLE_BATTLE_TEST("Polished action Liquid Ooze bypasses Magic Guard even at full HP")
{
    s16 dealt;
    GIVEN {
        PLAYER(SPECIES_CLEFABLE) { Ability(ABILITY_MAGIC_GUARD); MaxHP(400); HP(400); }
        OPPONENT(SPECIES_TENTACRUEL) { Ability(ABILITY_LIQUID_OOZE); MaxHP(1600); HP(1600); }
    } WHEN { TURN { MOVE(player, MOVE_GIGA_DRAIN); } }
    SCENE { HP_BAR(opponent, captureDamage: &dealt); }
    THEN { EXPECT_EQ(player->hp, B_POLISHED_MOVE_BALANCE ? 400 - max(1, dealt / 2) : 400); }
}

SINGLE_BATTLE_TEST("Polished action Destiny Bond repeated use fails independently of native generation")
{
    GIVEN {
        WITH_CONFIG(B_DESTINY_BOND_FAIL, GEN_6);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(player, MOVE_DESTINY_BOND); }
        TURN { MOVE(player, MOVE_DESTINY_BOND); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_DESTINY_BOND, player);
        if (B_POLISHED_MOVE_BALANCE) MESSAGE("But it failed!");
        else ANIMATION(ANIM_TYPE_MOVE, MOVE_DESTINY_BOND, player);
    }
}

SINGLE_BATTLE_TEST("Polished action U turn reserves its switch before Eject Button")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); }
        PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); Item(ITEM_EJECT_BUTTON); MaxHP(1600); HP(1600); }
        OPPONENT(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Speed(50); }
    } WHEN {
        TURN { MOVE(player, MOVE_U_TURN); if (B_POLISHED_MOVE_BALANCE) SEND_OUT(player, 1); else SEND_OUT(opponent, 1); }
    } THEN {
        EXPECT_EQ(player->species, B_POLISHED_MOVE_BALANCE ? SPECIES_CHANSEY : SPECIES_WOBBUFFET);
        EXPECT_EQ(opponent->species, B_POLISHED_MOVE_BALANCE ? SPECIES_WOBBUFFET : SPECIES_CHANSEY);
        if (B_POLISHED_MOVE_BALANCE) EXPECT_EQ(opponent->item, ITEM_EJECT_BUTTON);
    }
}

SINGLE_BATTLE_TEST("Polished action lethal contact items finish damage before first faint on either side")
{
    u32 item = ITEM_ROCKY_HELMET, reverse = FALSE;
    PARAMETRIZE { item = ITEM_ROCKY_HELMET; reverse = FALSE; }
    PARAMETRIZE { item = ITEM_ROCKY_HELMET; reverse = TRUE; }
    PARAMETRIZE { item = ITEM_JABOCA_BERRY; reverse = FALSE; }
    PARAMETRIZE { item = ITEM_JABOCA_BERRY; reverse = TRUE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); HP(1); Item(reverse ? item : ITEM_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); HP(1); Item(reverse ? ITEM_NONE : item); }
    } WHEN { TURN { if (reverse) MOVE(opponent, MOVE_TACKLE); else MOVE(player, MOVE_TACKLE); } }
    SCENE {
        if (reverse) {
            HP_BAR(player); HP_BAR(opponent);
            MESSAGE("The opposing Wobbuffet fainted!"); MESSAGE("Wobbuffet fainted!");
        } else {
            HP_BAR(opponent); HP_BAR(player);
            MESSAGE("Wobbuffet fainted!"); MESSAGE("The opposing Wobbuffet fainted!");
        }
    } THEN {
        EXPECT_EQ(player->hp, 0); EXPECT_EQ(opponent->hp, 0);
        if (B_POLISHED_MOVE_BALANCE) EXPECT_EQ(gBattleOutcome, B_OUTCOME_LOST);
    }
}

SINGLE_BATTLE_TEST("Polished action Aftermath overrides the direct target first faint")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); HP(1); }
        OPPONENT(SPECIES_VOLTORB) { Ability(ABILITY_AFTERMATH); HP(1); }
    } WHEN { TURN { MOVE(player, MOVE_TACKLE); } }
    SCENE {
        HP_BAR(opponent); HP_BAR(player);
        MESSAGE("Wobbuffet fainted!"); MESSAGE("The opposing Voltorb fainted!");
    } THEN { if (B_POLISHED_MOVE_BALANCE) EXPECT_EQ(gBattleOutcome, B_OUTCOME_LOST); }
}

SINGLE_BATTLE_TEST("Polished action ordinary recoil retains the direct target first faint")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); HP(1); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); HP(3); }
    } WHEN { TURN { MOVE(player, MOVE_DOUBLE_EDGE); } }
    SCENE {
        HP_BAR(opponent);
        if (B_POLISHED_MOVE_BALANCE) {
            HP_BAR(player); MESSAGE("The opposing Wobbuffet fainted!"); MESSAGE("Wobbuffet fainted!");
        }
    } THEN {
        if (B_POLISHED_MOVE_BALANCE) { EXPECT_EQ(player->hp, 0); EXPECT_EQ(gBattleOutcome, B_OUTCOME_LOST); }
    }
}

SINGLE_BATTLE_TEST("Polished action Liquid Ooze overrides the direct target first faint")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); HP(1); }
        OPPONENT(SPECIES_TENTACRUEL) { Ability(ABILITY_LIQUID_OOZE); HP(2); }
    } WHEN { TURN { MOVE(player, MOVE_GIGA_DRAIN); } }
    SCENE {
        HP_BAR(opponent); HP_BAR(player);
        MESSAGE("Wobbuffet fainted!"); MESSAGE("The opposing Tentacruel fainted!");
    } THEN { if (B_POLISHED_MOVE_BALANCE) EXPECT_EQ(gBattleOutcome, B_OUTCOME_LOST); }
}

SINGLE_BATTLE_TEST("Polished action Dream Eater Ooze uses source rounding and ignores native generation")
{
    s16 dealt;
    GIVEN {
        WITH_CONFIG(B_DREAM_EATER_LIQUID_OOZE, GEN_4);
        PLAYER(SPECIES_CLEFABLE) { Ability(ABILITY_MAGIC_GUARD); Item(ITEM_BIG_ROOT); MaxHP(400); HP(400); }
        OPPONENT(SPECIES_TENTACRUEL) { Ability(ABILITY_LIQUID_OOZE); MaxHP(1600); HP(1600); Status1(STATUS1_SLEEP); }
    } WHEN { TURN { MOVE(player, MOVE_DREAM_EATER); } }
    SCENE { HP_BAR(opponent, captureDamage: &dealt); }
    THEN { EXPECT_EQ(player->hp, B_POLISHED_MOVE_BALANCE ? 400 - max(1, (dealt * 13 / 10) / 2) : 400); }
}

SINGLE_BATTLE_TEST("Polished action source contact items retain Magic Guard protection")
{
    u32 item = ITEM_ROCKY_HELMET;
    PARAMETRIZE { item = ITEM_ROCKY_HELMET; }
    PARAMETRIZE { item = ITEM_JABOCA_BERRY; }
    GIVEN {
        PLAYER(SPECIES_CLEFABLE) { Ability(ABILITY_MAGIC_GUARD); MaxHP(160); HP(160); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); HP(1); Item(item); }
    } WHEN { TURN { MOVE(player, MOVE_TACKLE); } }
    THEN { EXPECT_EQ(player->hp, 160); EXPECT_EQ(opponent->item, item); }
}

SINGLE_BATTLE_TEST("Polished action Aftermath respects contact blockers but is not Mold Breaker suppressed")
{
    u32 ability = ABILITY_DAMP, item = ITEM_NONE;
    PARAMETRIZE { ability = ABILITY_DAMP; }
    PARAMETRIZE { ability = ABILITY_MOLD_BREAKER; }
    PARAMETRIZE { ability = ABILITY_LONG_REACH; }
    PARAMETRIZE { ability = ABILITY_NONE; item = ITEM_PROTECTIVE_PADS; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ability); Item(item); MaxHP(160); HP(160); }
        OPPONENT(SPECIES_VOLTORB) { Ability(ABILITY_AFTERMATH); HP(1); }
    } WHEN { TURN { MOVE(player, MOVE_TACKLE); } }
    THEN { EXPECT_EQ(player->hp, ability == ABILITY_MOLD_BREAKER ? 120 : 160); }
}

SINGLE_BATTLE_TEST("Polished action Aftermath rejects noncontact hits")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); MaxHP(160); HP(160); }
        OPPONENT(SPECIES_VOLTORB) { Ability(ABILITY_AFTERMATH); HP(1); }
    } WHEN { TURN { MOVE(player, MOVE_WATER_GUN); } }
    SCENE { NOT ABILITY_POPUP(opponent, ABILITY_AFTERMATH); }
    THEN { EXPECT_EQ(player->hp, 160); }
}

SINGLE_BATTLE_TEST("Polished action Moxie cannot boost a user already knocked out by recoil")
{
    GIVEN {
        PLAYER(SPECIES_GYARADOS) { Ability(ABILITY_MOXIE); HP(1); }
        PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); HP(6); }
        OPPONENT(SPECIES_CHANSEY) { Ability(ABILITY_NONE); }
    } WHEN { TURN { MOVE(player, MOVE_DOUBLE_EDGE); SEND_OUT(player, 1); SEND_OUT(opponent, 1); } }
    SCENE { if (B_POLISHED_MOVE_BALANCE) NOT ABILITY_POPUP(player, ABILITY_MOXIE); }
}

SINGLE_BATTLE_TEST("Polished action Destiny Bond suppresses Moxie as well as Aftermath")
{
    GIVEN {
        PLAYER(SPECIES_GYARADOS) { Ability(ABILITY_MOXIE); Speed(50); }
        PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Speed(50); }
        OPPONENT(SPECIES_VOLTORB) { Ability(ABILITY_AFTERMATH); Speed(100); HP(1); }
        OPPONENT(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Speed(100); }
    } WHEN { TURN { MOVE(opponent, MOVE_DESTINY_BOND); MOVE(player, MOVE_TACKLE); SEND_OUT(player, 1); SEND_OUT(opponent, 1); } }
    SCENE {
        if (B_POLISHED_MOVE_BALANCE) { NONE_OF { ABILITY_POPUP(player, ABILITY_MOXIE); ABILITY_POPUP(opponent, ABILITY_AFTERMATH); } }
    }
}

SINGLE_BATTLE_TEST("Polished action Destiny Bond choice survives flinch but a different choice clears it")
{
    u32 move = MOVE_DESTINY_BOND;
    PARAMETRIZE { move = MOVE_DESTINY_BOND; }
    PARAMETRIZE { move = MOVE_TACKLE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); }
    } WHEN {
        TURN { MOVE(player, MOVE_DESTINY_BOND); SWITCH(opponent, 1); }
        TURN { MOVE(opponent, MOVE_FAKE_OUT); MOVE(player, move); }
    } THEN {
        if (B_POLISHED_MOVE_BALANCE) EXPECT_EQ((u32)player->volatiles.destinyBond, move == MOVE_DESTINY_BOND ? 1 : 0);
    }
}

SINGLE_BATTLE_TEST("Polished action U turn takes Life Orb damage before the outgoing switch")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); MaxHP(160); HP(160); Item(ITEM_LIFE_ORB); }
        PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); MaxHP(1600); HP(1600); }
    } WHEN { TURN { MOVE(player, MOVE_U_TURN); SEND_OUT(player, 1); } }
    SCENE { HP_BAR(player, damage: 16); MESSAGE("Wobbuffet went back to 1!"); }
    THEN { EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_HP), 144); }
}

SINGLE_BATTLE_TEST("Polished action Life Orb self KO cancels the reserved U turn switch")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); MaxHP(160); HP(1); Item(ITEM_LIFE_ORB); }
        PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); MaxHP(1600); HP(1600); }
    } WHEN { TURN { MOVE(player, MOVE_U_TURN); SEND_OUT(player, 1); } }
    SCENE { HP_BAR(player); MESSAGE("Wobbuffet fainted!"); NOT MESSAGE("Wobbuffet went back to 1!"); }
    THEN { EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_HP), 0); }
}

SINGLE_BATTLE_TEST("Polished action recoil is capped by target HP before contact items")
{
    s16 recoil;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); MaxHP(400); HP(400); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); HP(9); }
    } WHEN { TURN { MOVE(player, MOVE_DOUBLE_EDGE); } }
    SCENE { HP_BAR(player, captureDamage: &recoil); }
    THEN { EXPECT_EQ(recoil, B_POLISHED_MOVE_BALANCE ? 3 : 2); }
}

SINGLE_BATTLE_TEST("Polished action switch items respond to damage absorbed by Substitute")
{
    u32 item = ITEM_EJECT_BUTTON, level = 49;
    PARAMETRIZE { item = ITEM_EJECT_BUTTON; level = 49; }
    PARAMETRIZE { item = ITEM_EJECT_BUTTON; level = 50; }
    PARAMETRIZE { item = ITEM_RED_CARD; level = 49; }
    PARAMETRIZE { item = ITEM_RED_CARD; level = 50; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Level(level); Speed(100); }
        PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(item); Speed(50); MaxHP(200); HP(200); }
        OPPONENT(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Speed(50); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_SUBSTITUTE); }
        TURN { MOVE(player, MOVE_SEISMIC_TOSS); if (B_POLISHED_MOVE_BALANCE && item == ITEM_EJECT_BUTTON) SEND_OUT(opponent, 1); }
    } THEN {
        EXPECT_EQ(player->species, B_POLISHED_MOVE_BALANCE && item == ITEM_RED_CARD ? SPECIES_CHANSEY : SPECIES_WOBBUFFET);
        EXPECT_EQ(opponent->species, B_POLISHED_MOVE_BALANCE && item == ITEM_EJECT_BUTTON ? SPECIES_CHANSEY : SPECIES_WOBBUFFET);
    }
}

SINGLE_BATTLE_TEST("Polished action U turn reserves before Red Card and keeps the item unconsumed")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); }
        PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_RED_CARD); Speed(50); MaxHP(1600); HP(1600); }
    } WHEN { TURN { MOVE(player, MOVE_U_TURN); if (B_POLISHED_MOVE_BALANCE) SEND_OUT(player, 1); } }
    THEN { EXPECT_EQ(opponent->item, B_POLISHED_MOVE_BALANCE ? ITEM_RED_CARD : ITEM_NONE); }
}

SINGLE_BATTLE_TEST("Polished action Eject Button requires a living holder after the action")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_EJECT_BUTTON); Speed(50); HP(1); }
        OPPONENT(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Speed(50); }
    } WHEN { TURN { MOVE(player, MOVE_TACKLE); SEND_OUT(opponent, 1); } }
    THEN { EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_HELD_ITEM), ITEM_EJECT_BUTTON); }
}

SINGLE_BATTLE_TEST("Polished action Life Orb recoil precedes Eject Button and Red Card")
{
    u32 item = ITEM_EJECT_BUTTON;
    PARAMETRIZE { item = ITEM_EJECT_BUTTON; }
    PARAMETRIZE { item = ITEM_RED_CARD; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); Item(ITEM_LIFE_ORB); MaxHP(160); HP(160); }
        PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); Item(item); MaxHP(1600); HP(1600); }
        OPPONENT(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Speed(50); }
    } WHEN { TURN { MOVE(player, MOVE_TACKLE); if (item == ITEM_EJECT_BUTTON) SEND_OUT(opponent, 1); } }
    SCENE {
        if (B_POLISHED_MOVE_BALANCE) HP_BAR(player, damage: 16);
        if (item == ITEM_EJECT_BUTTON) MESSAGE("The opposing Wobbuffet is switched out with the Eject Button!");
        else MESSAGE("The opposing Wobbuffet held up its Red Card against Wobbuffet!");
    }
}

SINGLE_BATTLE_TEST("Polished action Destiny Bond damage precedes both faint animations")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); HP(1); }
    } WHEN { TURN { MOVE(opponent, MOVE_DESTINY_BOND); MOVE(player, MOVE_TACKLE); } }
    SCENE {
        HP_BAR(opponent);
        if (B_POLISHED_MOVE_BALANCE) {
            MESSAGE("The opposing Wobbuffet took its attacker down with it!"); HP_BAR(player);
            MESSAGE("The opposing Wobbuffet fainted!"); MESSAGE("Wobbuffet fainted!");
        } else {
            MESSAGE("The opposing Wobbuffet fainted!");
            MESSAGE("The opposing Wobbuffet took its attacker down with it!"); HP_BAR(player);
        }
    }
}

SINGLE_BATTLE_TEST("Polished action Explosion does not trigger an existing Destiny Bond")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); HP(1); }
    } WHEN { TURN { MOVE(opponent, MOVE_DESTINY_BOND); MOVE(player, MOVE_EXPLOSION); } }
    SCENE { NOT MESSAGE("The opposing Wobbuffet took its attacker down with it!"); }
    THEN { if (B_POLISHED_MOVE_BALANCE) EXPECT_EQ(gBattleOutcome, B_OUTCOME_LOST); }
}

SINGLE_BATTLE_TEST("Polished action contact multihit stops when the user faints from Helmet")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_SKILL_LINK); MaxHP(120); HP(35); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_ROCKY_HELMET); MaxHP(1600); HP(1600); }
    } WHEN { TURN { MOVE(player, MOVE_FURY_SWIPES); } }
    SCENE { HP_BAR(opponent); HP_BAR(player); HP_BAR(opponent); HP_BAR(player); MESSAGE("Wobbuffet fainted!"); }
    THEN { EXPECT_EQ(player->hp, 0); }
}

SINGLE_BATTLE_TEST("Polished action shell bell recovery precedes the opposing faint animation")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_SHELL_BELL); HP(10); MaxHP(160); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); HP(8); }
    } WHEN { TURN { MOVE(player, MOVE_TACKLE); } }
    SCENE {
        HP_BAR(opponent);
        if (B_POLISHED_MOVE_BALANCE) { HP_BAR(player, damage: -1); MESSAGE("The opposing Wobbuffet fainted!"); }
        else { MESSAGE("The opposing Wobbuffet fainted!"); }
    } THEN { if (B_POLISHED_MOVE_BALANCE) EXPECT_EQ(player->hp, 11); }
}

SINGLE_BATTLE_TEST("Polished action Pursuit intercepts U turn after Life Orb exactly once")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_LIFE_ORB); Speed(100); MaxHP(1600); HP(1600); }
        PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); Moves(MOVE_PURSUIT); MaxHP(1600); HP(1600); }
    } WHEN { TURN { MOVE(player, MOVE_U_TURN); MOVE(opponent, MOVE_PURSUIT); SEND_OUT(player, 1); } }
    SCENE {
        HP_BAR(opponent); HP_BAR(player, damage: 160);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_PURSUIT, opponent); HP_BAR(player);
        MESSAGE("Go! Chansey!"); NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_PURSUIT, opponent);
    } THEN { EXPECT_EQ(opponent->pp[0], GetMovePP(MOVE_PURSUIT) - 1); }
}

SINGLE_BATTLE_TEST("Polished action Aftermath still resolves after lethal recoil and changes faint order")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); HP(1); }
        OPPONENT(SPECIES_VOLTORB) { Ability(ABILITY_AFTERMATH); HP(6); }
    } WHEN { TURN { MOVE(player, MOVE_DOUBLE_EDGE); } }
    SCENE {
        HP_BAR(opponent);
        if (B_POLISHED_MOVE_BALANCE) { HP_BAR(player); MESSAGE("Wobbuffet was damaged by the recoil!"); }
        ABILITY_POPUP(opponent, ABILITY_AFTERMATH);
        MESSAGE("Wobbuffet fainted!"); MESSAGE("The opposing Voltorb fainted!");
    }
}

SINGLE_BATTLE_TEST("Polished action AI Destiny Bond failure queries cannot clear the live volatile")
{
    GIVEN {
        WITH_CONFIG(B_DESTINY_BOND_FAIL, GEN_6);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN { TURN { MOVE(player, MOVE_DESTINY_BOND); } }
    THEN {
        EXPECT_EQ(DoesDestinyBondFail(B_BATTLER_0), B_POLISHED_MOVE_BALANCE != 0);
        EXPECT_EQ(DoesDestinyBondFail(B_BATTLER_0), B_POLISHED_MOVE_BALANCE != 0);
        EXPECT_EQ((u32)player->volatiles.destinyBond, 1);
    }
}

DOUBLE_BATTLE_TEST("Polished action doubles retain native Aftermath Magic Guard handling")
{
    GIVEN {
        PLAYER(SPECIES_CLEFABLE) { Ability(ABILITY_MAGIC_GUARD); MaxHP(160); HP(160); }
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_VOLTORB) { Ability(ABILITY_AFTERMATH); HP(1); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN { TURN { MOVE(playerLeft, MOVE_TACKLE, target: opponentLeft); } }
    THEN { EXPECT_EQ(playerLeft->hp, 160); }
}

SINGLE_BATTLE_TEST("Polished action unmapped optional moves retain native Aftermath handling")
{
    GIVEN {
        PLAYER(SPECIES_CLEFABLE) { Ability(ABILITY_MAGIC_GUARD); MaxHP(160); HP(160); }
        OPPONENT(SPECIES_VOLTORB) { Ability(ABILITY_AFTERMATH); HP(1); }
    } WHEN { TURN { MOVE(player, MOVE_WOOD_HAMMER); } }
    THEN { EXPECT_EQ(player->hp, 160); }
}

SINGLE_BATTLE_TEST("Polished action an active optional gimmick retains native Aftermath handling")
{
    GIVEN {
        PLAYER(SPECIES_CLEFABLE) { Ability(ABILITY_MAGIC_GUARD); MaxHP(160); HP(160); }
        OPPONENT(SPECIES_VOLTORB) { Ability(ABILITY_AFTERMATH); HP(1); }
    } WHEN { TURN { MOVE(player, MOVE_TACKLE, gimmick: GIMMICK_TERA); } }
    THEN { EXPECT_EQ(player->hp, 160); }
}

SINGLE_BATTLE_TEST("Polished action a reserved U turn cancelled by Life Orb does not fall back to Eject Button")
{
    // Source lethal reservation case; native control tests nonlethal item precedence.
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); MaxHP(160); HP(B_POLISHED_MOVE_BALANCE ? 1 : 160); Item(ITEM_LIFE_ORB); }
        PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); MaxHP(1600); HP(1600); Item(ITEM_EJECT_BUTTON); }
        OPPONENT(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Speed(50); }
    } WHEN {
        TURN { MOVE(player, MOVE_U_TURN); if (B_POLISHED_MOVE_BALANCE) SEND_OUT(player, 1); else SEND_OUT(opponent, 1); }
    } THEN {
        EXPECT_EQ(opponent->species, B_POLISHED_MOVE_BALANCE ? SPECIES_WOBBUFFET : SPECIES_CHANSEY);
        if (B_POLISHED_MOVE_BALANCE) EXPECT_EQ(opponent->item, ITEM_EJECT_BUTTON);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_HP), B_POLISHED_MOVE_BALANCE ? 0 : 144);
    }
}

SINGLE_BATTLE_TEST("Polished action recoil healing berry prevents the subsequent Aftermath knockout")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); MaxHP(160); HP(81); Attack(500); Item(ITEM_SITRUS_BERRY); }
        OPPONENT(SPECIES_VOLTORB) { Ability(ABILITY_AFTERMATH); MaxHP(150); HP(150); Defense(1); }
    } WHEN { TURN { MOVE(player, MOVE_DOUBLE_EDGE); } }
    SCENE {
        HP_BAR(opponent);
        if (B_POLISHED_MOVE_BALANCE) {
            HP_BAR(player, damage: 50); HP_BAR(player, damage: -40);
            ABILITY_POPUP(opponent, ABILITY_AFTERMATH); HP_BAR(player, damage: 40);
        }
    } THEN { if (B_POLISHED_MOVE_BALANCE) EXPECT_EQ(player->hp, 31); }
}

SINGLE_BATTLE_TEST("Polished action Aftermath healing berry precedes Rocky Helmet")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); MaxHP(160); HP(81); Item(ITEM_SITRUS_BERRY); }
        OPPONENT(SPECIES_VOLTORB) { Ability(ABILITY_AFTERMATH); HP(1); Item(ITEM_ROCKY_HELMET); }
    } WHEN { TURN { MOVE(player, MOVE_TACKLE); } }
    SCENE {
        HP_BAR(opponent);
        if (B_POLISHED_MOVE_BALANCE) {
            ABILITY_POPUP(opponent, ABILITY_AFTERMATH); HP_BAR(player, damage: 40);
            HP_BAR(player, damage: -40); HP_BAR(player, damage: 26);
        }
    } THEN { if (B_POLISHED_MOVE_BALANCE) EXPECT_EQ(player->hp, 55); }
}
