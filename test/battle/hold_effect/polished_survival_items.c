#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished survival Focus Band uses its raw thirty byte outcomes")
{
    if (B_POLISHED_MOVE_BALANCE)
        PASSES_RANDOMLY(30, 256, RNG_POLISHED_FOCUS_BAND);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Level(100); Speed(100); }
        // Enumerate every source byte. Native Focus Band uses an untagged
        // draw, so use deterministic Sash survival as the native control.
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); HP(50); MaxHP(B_POLISHED_MOVE_BALANCE ? 100 : 50); Item(B_POLISHED_MOVE_BALANCE ? ITEM_FOCUS_BAND : ITEM_FOCUS_SASH); }
    } WHEN { TURN { MOVE(player, MOVE_SEISMIC_TOSS); } }
    THEN {
        EXPECT_EQ(opponent->hp, 1);
        EXPECT_EQ(opponent->item, B_POLISHED_MOVE_BALANCE ? ITEM_FOCUS_BAND : ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished survival Focus Band checks byte twenty nine versus thirty")
{
    u32 roll = 0;
    PARAMETRIZE { roll = 0; }
    PARAMETRIZE { roll = 29; }
    PARAMETRIZE { roll = 30; }
    PARAMETRIZE { roll = 255; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Level(100); Speed(100); }
        // Native full-HP Sash is the control: its survival does not depend
        // on the source-only Focus Band byte tag.
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); HP(50); MaxHP(50); Item(B_POLISHED_MOVE_BALANCE ? ITEM_FOCUS_BAND : ITEM_FOCUS_SASH); }
    } WHEN { TURN { MOVE(player, MOVE_SEISMIC_TOSS, WITH_RNG(RNG_POLISHED_FOCUS_BAND, roll)); } }
    THEN { EXPECT_EQ(opponent->hp, !B_POLISHED_MOVE_BALANCE || roll < 30 ? 1 : 0); }
}

SINGLE_BATTLE_TEST("Polished survival a failed Focus Band roll cannot fall back to Sturdy")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Level(100); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_STURDY); Speed(50); HP(50); MaxHP(50); Item(ITEM_FOCUS_BAND); }
    } WHEN { TURN { MOVE(player, MOVE_SEISMIC_TOSS, WITH_RNG(RNG_POLISHED_FOCUS_BAND, 30)); } }
    THEN {
        EXPECT_EQ(opponent->hp, B_POLISHED_MOVE_BALANCE ? 0 : 1);
        EXPECT_EQ(opponent->item, ITEM_FOCUS_BAND);
    }
}

SINGLE_BATTLE_TEST("Polished survival Focus Sash is consumed before Sturdy")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Level(100); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_STURDY); Speed(50); HP(50); MaxHP(50); Item(ITEM_FOCUS_SASH); }
    } WHEN { TURN { MOVE(player, MOVE_SEISMIC_TOSS); } }
    SCENE {
        if (B_POLISHED_MOVE_BALANCE) NOT ABILITY_POPUP(opponent, ABILITY_STURDY);
        else ABILITY_POPUP(opponent, ABILITY_STURDY);
    }
    THEN {
        EXPECT_EQ(opponent->hp, 1);
        EXPECT_EQ(opponent->item, B_POLISHED_MOVE_BALANCE ? ITEM_NONE : ITEM_FOCUS_SASH);
    }
}

SINGLE_BATTLE_TEST("Polished survival Sturdy ignores older native presets and respects Mold Breaker")
{
    enum Ability ability = ABILITY_NONE;
    PARAMETRIZE { ability = ABILITY_NONE; }
    PARAMETRIZE { ability = ABILITY_MOLD_BREAKER; }
    GIVEN {
        WITH_CONFIG(B_STURDY, GEN_4);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ability); Level(100); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_STURDY); Speed(50); HP(50); MaxHP(50); }
    } WHEN { TURN { MOVE(player, MOVE_SEISMIC_TOSS); } }
    THEN { EXPECT_EQ(opponent->hp, B_POLISHED_MOVE_BALANCE && ability == ABILITY_NONE ? 1 : 0); }
}

SINGLE_BATTLE_TEST("Polished survival Endure takes precedence over held survival items")
{
    enum Item item = ITEM_FOCUS_BAND;
    PARAMETRIZE { item = ITEM_FOCUS_BAND; }
    PARAMETRIZE { item = ITEM_FOCUS_SASH; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Level(100); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_STURDY); Speed(50); HP(50); MaxHP(50); Item(item); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_ENDURE); MOVE(player, MOVE_SEISMIC_TOSS, WITH_RNG(RNG_POLISHED_FOCUS_BAND, 255)); }
    } THEN {
        EXPECT_EQ(opponent->hp, 1);
        EXPECT_EQ(opponent->item, item);
    }
}

SINGLE_BATTLE_TEST("Polished survival False Swipe does not consume Focus Sash or activate Sturdy")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Attack(1000); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_STURDY); Speed(50); HP(10); MaxHP(10); Item(ITEM_FOCUS_SASH); }
    } WHEN { TURN { MOVE(player, MOVE_FALSE_SWIPE); } }
    SCENE { NOT ABILITY_POPUP(opponent, ABILITY_STURDY); }
    THEN {
        EXPECT_EQ(opponent->hp, 1);
        EXPECT_EQ(opponent->item, ITEM_FOCUS_SASH);
    }
}

SINGLE_BATTLE_TEST("Polished survival damaged Focus Sash and Sturdy holders faint normally")
{
    enum Item item = ITEM_NONE;
    PARAMETRIZE { item = ITEM_NONE; }
    PARAMETRIZE { item = ITEM_FOCUS_SASH; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Level(100); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_STURDY); Speed(50); HP(49); MaxHP(50); Item(item); }
    } WHEN { TURN { MOVE(player, MOVE_SEISMIC_TOSS); } }
    THEN {
        EXPECT_EQ(opponent->hp, 0);
        EXPECT_EQ(opponent->item, item);
    }
}

DOUBLE_BATTLE_TEST("Polished survival optional doubles retain native Sturdy before Focus Sash")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Level(100); Speed(100); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(90); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_STURDY); Speed(50); HP(50); MaxHP(50); Item(ITEM_FOCUS_SASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(40); }
    } WHEN { TURN { MOVE(playerLeft, MOVE_SEISMIC_TOSS, target: opponentLeft); } }
    THEN {
        EXPECT_EQ(opponentLeft->hp, 1);
        EXPECT_EQ(opponentLeft->item, ITEM_FOCUS_SASH);
    }
}

SINGLE_BATTLE_TEST("Polished survival Focus Sash is consumed on the first lethal hit and cannot save the second")
{
    enum Ability ability = ABILITY_NONE;
    PARAMETRIZE { ability = ABILITY_NONE; }
    PARAMETRIZE { ability = ABILITY_STURDY; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Attack(1000); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ability); Defense(10); Speed(50); HP(50); MaxHP(50); Item(ITEM_FOCUS_SASH); }
    } WHEN { TURN { MOVE(player, MOVE_DOUBLE_KICK); } }
    SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_DOUBLE_KICK, player);
        HP_BAR(opponent, hp: 1);
        if (B_POLISHED_MOVE_BALANCE || ability == ABILITY_NONE)
            MESSAGE("The opposing Wobbuffet hung on using its Focus Sash!");
        else
            ABILITY_POPUP(opponent, ABILITY_STURDY);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_DOUBLE_KICK, player);
        HP_BAR(opponent, hp: 0);
    } THEN {
        EXPECT_EQ(opponent->hp, 0);
        EXPECT_EQ(opponent->item, !B_POLISHED_MOVE_BALANCE && ability == ABILITY_STURDY ? ITEM_FOCUS_SASH : ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished survival a nonlethal first hit makes Focus Sash ineligible for the lethal second hit")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Level(50); Attack(10); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Defense(20); Speed(50); HP(6); MaxHP(6); Item(ITEM_FOCUS_SASH); }
    } WHEN { TURN { MOVE(player, MOVE_DOUBLE_KICK); } }
    SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_DOUBLE_KICK, player);
        HP_BAR(opponent);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_DOUBLE_KICK, player);
        HP_BAR(opponent, hp: 0);
        NOT MESSAGE("The opposing Wobbuffet hung on using its Focus Sash!");
    } THEN {
        EXPECT_EQ(opponent->hp, 0);
        EXPECT_EQ(opponent->item, ITEM_FOCUS_SASH);
    }
}

SINGLE_BATTLE_TEST("Polished survival Focus Band rolls again after surviving the first multihit strike")
{
    u32 laterRoll = 29;
    PARAMETRIZE { laterRoll = 29; }
    PARAMETRIZE { laterRoll = 30; }
    PARAMETRIZE { laterRoll = 255; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Attack(1000); Speed(100); }
        // Native RNG is untagged. Endure and Sash provide deterministic
        // controls for surviving both hits or only the first, respectively.
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Defense(10); Speed(50); HP(50); MaxHP(50); Item(B_POLISHED_MOVE_BALANCE || laterRoll < 30 ? ITEM_FOCUS_BAND : ITEM_FOCUS_SASH); }
    } WHEN {
        TURN {
            if (!B_POLISHED_MOVE_BALANCE && laterRoll < 30) MOVE(opponent, MOVE_ENDURE);
            MOVE(player, MOVE_DOUBLE_KICK, WITH_RNG(RNG_POLISHED_FOCUS_BAND, 29), WITH_RNG_AFTER_FIRST_HIT(RNG_POLISHED_FOCUS_BAND, laterRoll));
        }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_DOUBLE_KICK, player);
        HP_BAR(opponent, hp: 1);
        if (B_POLISHED_MOVE_BALANCE)
            MESSAGE("The opposing Wobbuffet hung on using its Focus Band!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_DOUBLE_KICK, player);
        if (laterRoll < 30) {
            if (B_POLISHED_MOVE_BALANCE)
                MESSAGE("The opposing Wobbuffet hung on using its Focus Band!");
        } else {
            HP_BAR(opponent, hp: 0);
        }
    } THEN {
        EXPECT_EQ(opponent->hp, laterRoll < 30 ? 1 : 0);
        EXPECT_EQ(opponent->item, B_POLISHED_MOVE_BALANCE || laterRoll < 30 ? ITEM_FOCUS_BAND : ITEM_NONE);
    }
}
