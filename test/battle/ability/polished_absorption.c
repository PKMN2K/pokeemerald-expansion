#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished absorption Sap Sipper skips source Grass status moves")
{
    enum Move move = MOVE_SPORE;
    PARAMETRIZE { move = MOVE_SPORE; }
    PARAMETRIZE { move = MOVE_SLEEP_POWDER; }
    PARAMETRIZE { move = MOVE_LEECH_SEED; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_SAP_SIPPER); Speed(50); }
    } WHEN {
        TURN { MOVE(player, move, hit: TRUE); }
    } THEN {
        EXPECT_EQ(opponent->statStages[STAT_ATK], DEFAULT_STAT_STAGE + !B_POLISHED_MOVE_BALANCE);
        if (move == MOVE_LEECH_SEED)
            EXPECT_EQ((u32)opponent->volatiles.leechSeed, B_POLISHED_MOVE_BALANCE ? TRUE : FALSE);
        else
            EXPECT_EQ((opponent->status1 & STATUS1_SLEEP) != 0, B_POLISHED_MOVE_BALANCE ? TRUE : FALSE);
    }
}

SINGLE_BATTLE_TEST("Polished absorption Volt Absorb and Motor Drive skip source Thunder Wave")
{
    enum Ability ability = ABILITY_VOLT_ABSORB;
    PARAMETRIZE { ability = ABILITY_VOLT_ABSORB; }
    PARAMETRIZE { ability = ABILITY_MOTOR_DRIVE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ability); HP(80); MaxHP(161); }
    } WHEN { TURN { MOVE(player, MOVE_THUNDER_WAVE, hit: TRUE); } }
    THEN {
        EXPECT_EQ(opponent->status1, B_POLISHED_MOVE_BALANCE ? STATUS1_PARALYSIS : STATUS1_NONE);
        EXPECT_EQ(opponent->hp, !B_POLISHED_MOVE_BALANCE && ability == ABILITY_VOLT_ABSORB ? 120 : 80);
        EXPECT_EQ(opponent->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + (!B_POLISHED_MOVE_BALANCE && ability == ABILITY_MOTOR_DRIVE));
    }
}

SINGLE_BATTLE_TEST("Polished absorption Lightning Rod blocks damage and status despite older native presets")
{
    enum Move move = MOVE_THUNDER_WAVE;
    PARAMETRIZE { move = MOVE_THUNDER_WAVE; }
    PARAMETRIZE { move = MOVE_THUNDERBOLT; }
    GIVEN {
        WITH_CONFIG(B_REDIRECT_ABILITY_IMMUNITY, GEN_4);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_LIGHTNING_ROD); HP(1000); MaxHP(1000); }
    } WHEN { TURN { MOVE(player, move, hit: TRUE, secondaryEffect: FALSE); } }
    THEN {
        EXPECT_EQ(opponent->statStages[STAT_SPATK], DEFAULT_STAT_STAGE + !!B_POLISHED_MOVE_BALANCE);
        EXPECT_EQ(opponent->status1, !B_POLISHED_MOVE_BALANCE && move == MOVE_THUNDER_WAVE ? STATUS1_PARALYSIS : STATUS1_NONE);
        EXPECT_EQ(opponent->hp < 1000, !B_POLISHED_MOVE_BALANCE && move == MOVE_THUNDERBOLT);
    }
}

SINGLE_BATTLE_TEST("Polished absorption Ground immunity takes precedence over electric absorption")
{
    enum Ability ability = ABILITY_LIGHTNING_ROD;
    enum Item item = ITEM_NONE;
    for (u32 i = 0; i < 2; i++) {
        PARAMETRIZE { ability = ABILITY_LIGHTNING_ROD; item = i ? ITEM_RING_TARGET : ITEM_NONE; }
        PARAMETRIZE { ability = ABILITY_MOTOR_DRIVE; item = i ? ITEM_RING_TARGET : ITEM_NONE; }
        PARAMETRIZE { ability = ABILITY_VOLT_ABSORB; item = i ? ITEM_RING_TARGET : ITEM_NONE; }
    }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_RHYHORN) { Ability(ability); Item(item); HP(80); MaxHP(161); }
    } WHEN { TURN { MOVE(player, MOVE_THUNDERBOLT); } }
    THEN {
        bool32 absorbed = !B_POLISHED_MOVE_BALANCE || item == ITEM_RING_TARGET;
        EXPECT_EQ(opponent->hp, absorbed && ability == ABILITY_VOLT_ABSORB ? 120 : 80);
        EXPECT_EQ(opponent->statStages[STAT_SPATK], DEFAULT_STAT_STAGE + (absorbed && ability == ABILITY_LIGHTNING_ROD));
        EXPECT_EQ(opponent->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + (absorbed && ability == ABILITY_MOTOR_DRIVE));
    }
}

SINGLE_BATTLE_TEST("Polished absorption retains source damage absorption and exact recovery flooring")
{
    enum Ability ability = ABILITY_VOLT_ABSORB;
    enum Move move = MOVE_THUNDERBOLT;
    PARAMETRIZE { ability = ABILITY_VOLT_ABSORB; move = MOVE_THUNDERBOLT; }
    PARAMETRIZE { ability = ABILITY_WATER_ABSORB; move = MOVE_WATER_GUN; }
    PARAMETRIZE { ability = ABILITY_DRY_SKIN; move = MOVE_WATER_GUN; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ability); HP(80); MaxHP(161); }
    } WHEN { TURN { MOVE(player, move); } }
    THEN { EXPECT_EQ(opponent->hp, 120); }
}

SINGLE_BATTLE_TEST("Polished absorption source Bulletproof is not in the Mold Breaker suppression list")
{
    enum Ability attackerAbility = ABILITY_MOLD_BREAKER;
    PARAMETRIZE { attackerAbility = ABILITY_NONE; }
    PARAMETRIZE { attackerAbility = ABILITY_MOLD_BREAKER; }
    PARAMETRIZE { attackerAbility = ABILITY_NEUTRALIZING_GAS; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(attackerAbility); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_BULLETPROOF); HP(1000); MaxHP(1000); }
    } WHEN { TURN { MOVE(player, MOVE_SHADOW_BALL, secondaryEffect: FALSE); } }
    THEN {
        EXPECT_EQ(opponent->hp < 1000,
            attackerAbility == ABILITY_NEUTRALIZING_GAS
            || (attackerAbility == ABILITY_MOLD_BREAKER && !B_POLISHED_MOVE_BALANCE));
    }
}

SINGLE_BATTLE_TEST("Polished absorption Weak Armor always grants two Speed stages")
{
    GIVEN {
        WITH_CONFIG(B_WEAK_ARMOR_SPEED, GEN_6);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_WEAK_ARMOR); HP(1000); MaxHP(1000); }
    } WHEN { TURN { MOVE(player, MOVE_TACKLE); } }
    THEN {
        EXPECT_EQ(opponent->statStages[STAT_DEF], DEFAULT_STAT_STAGE - 1);
        EXPECT_EQ(opponent->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + (B_POLISHED_MOVE_BALANCE ? 2 : 1));
    }
}

DOUBLE_BATTLE_TEST("Polished absorption optional doubles retain native Sap Sipper status immunity")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_SAP_SIPPER); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN { TURN { MOVE(playerLeft, MOVE_LEECH_SEED, target: opponentLeft); } }
    THEN {
        EXPECT_EQ((u32)opponentLeft->volatiles.leechSeed, FALSE);
        EXPECT_EQ(opponentLeft->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 1);
    }
}

SINGLE_BATTLE_TEST("Polished absorption full HP recovery abilities announce that HP is full")
{
    struct BattlePokemon *holder = player, *attacker = opponent;
    enum Ability ability = ABILITY_VOLT_ABSORB;
    enum Move move = MOVE_THUNDERBOLT;
    for (u32 side = 0; side < 2; side++) {
        PARAMETRIZE { holder = side ? opponent : player; attacker = side ? player : opponent; ability = ABILITY_VOLT_ABSORB; move = MOVE_THUNDERBOLT; }
        PARAMETRIZE { holder = side ? opponent : player; attacker = side ? player : opponent; ability = ABILITY_WATER_ABSORB; move = MOVE_WATER_GUN; }
        PARAMETRIZE { holder = side ? opponent : player; attacker = side ? player : opponent; ability = ABILITY_DRY_SKIN; move = MOVE_WATER_GUN; }
    }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(holder == player ? ability : ABILITY_NONE); HP(161); MaxHP(161); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(holder == opponent ? ability : ABILITY_NONE); HP(161); MaxHP(161); }
    } WHEN { TURN { MOVE(attacker, move); } }
    SCENE {
        ABILITY_POPUP(holder, ability);
        if (B_POLISHED_MOVE_BALANCE) {
            if (holder == player) MESSAGE("Wobbuffet's HP is full!");
            else MESSAGE("The opposing Wobbuffet's HP is full!");
        } else {
            if (holder == player) MESSAGE("It doesn't affect Wobbuffet…");
            else MESSAGE("It doesn't affect the opposing Wobbuffet…");
        }
        NOT HP_BAR(holder);
    } THEN { EXPECT_EQ(holder->hp, 161); }
}

SINGLE_BATTLE_TEST("Polished absorption capped stat abilities retain immunity and report no effect")
{
    struct BattlePokemon *holder = player, *attacker = opponent;
    enum Ability ability = ABILITY_MOTOR_DRIVE;
    enum Move boost = MOVE_AGILITY, move = MOVE_THUNDERBOLT;
    enum Stat stat = STAT_SPEED;
    for (u32 side = 0; side < 2; side++) {
        PARAMETRIZE { holder = side ? opponent : player; attacker = side ? player : opponent; ability = ABILITY_MOTOR_DRIVE; boost = MOVE_AGILITY; move = MOVE_THUNDERBOLT; stat = STAT_SPEED; }
        PARAMETRIZE { holder = side ? opponent : player; attacker = side ? player : opponent; ability = ABILITY_LIGHTNING_ROD; boost = MOVE_NASTY_PLOT; move = MOVE_THUNDERBOLT; stat = STAT_SPATK; }
        PARAMETRIZE { holder = side ? opponent : player; attacker = side ? player : opponent; ability = ABILITY_SAP_SIPPER; boost = MOVE_SWORDS_DANCE; move = MOVE_RAZOR_LEAF; stat = STAT_ATK; }
    }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(holder == player ? ability : ABILITY_NONE); Speed(holder == player ? 1000 : 50); HP(1000); MaxHP(1000); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(holder == opponent ? ability : ABILITY_NONE); Speed(holder == opponent ? 1000 : 50); HP(1000); MaxHP(1000); }
    } WHEN {
        for (u32 turn = 0; turn < 3; turn++) {
            TURN { MOVE(holder, boost); }
        }
        TURN { MOVE(attacker, move, hit: TRUE); }
    } SCENE {
        ABILITY_POPUP(holder, ability);
        if (holder == player) MESSAGE("It doesn't affect Wobbuffet…");
        else MESSAGE("It doesn't affect the opposing Wobbuffet…");
        NOT HP_BAR(holder);
    } THEN {
        EXPECT_EQ(holder->hp, 1000);
        EXPECT_EQ(holder->statStages[stat], MAX_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished absorption frozen Flash Fire ignores the native legacy eligibility preset")
{
    enum Move move = MOVE_FLAMETHROWER;
    u32 config = GEN_4;
    for (u32 gen = GEN_4; gen <= GEN_5; gen++) {
        PARAMETRIZE { move = MOVE_FLAMETHROWER; config = gen; }
        PARAMETRIZE { move = MOVE_WILL_O_WISP; config = gen; }
    }
    bool32 absorbed = B_POLISHED_MOVE_BALANCE || config >= GEN_5;
    GIVEN {
        WITH_CONFIG(B_FLASH_FIRE_FROZEN, config);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_FLASH_FIRE); Speed(50); HP(1000); MaxHP(1000); Status1(STATUS1_FREEZE); }
    } WHEN {
        TURN { MOVE(player, move, hit: TRUE, secondaryEffect: FALSE); MOVE(opponent, MOVE_SPLASH, WITH_RNG(RNG_FROZEN, B_POLISHED_MOVE_BALANCE ? 255 : FALSE)); }
    } SCENE {
        if (absorbed) ABILITY_POPUP(opponent, ABILITY_FLASH_FIRE);
        else NOT ABILITY_POPUP(opponent, ABILITY_FLASH_FIRE);
    } THEN {
        EXPECT_EQ((u32)opponent->volatiles.flashFireBoosted, absorbed);
        EXPECT_EQ(opponent->hp < 1000, !absorbed && move == MOVE_FLAMETHROWER);
        EXPECT_EQ(opponent->status1, !absorbed && move == MOVE_FLAMETHROWER ? STATUS1_NONE : STATUS1_FREEZE);
    }
}
