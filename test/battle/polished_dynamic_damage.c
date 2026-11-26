#include "global.h"
#include "test/battle.h"
#include "battle_util.h"
#include "battle_gimmick.h"
#include "polished_damage.h"
#include "polished_dynamic_damage_vectors.h"

SINGLE_BATTLE_TEST("Polished dynamic damage Low Kick uses strict source weight bands")
{
    enum Species species;
    u32 power;
    PARAMETRIZE { species = SPECIES_DIGLETT; power = 20; }
    PARAMETRIZE { species = SPECIES_EXEGGCUTE; power = 40; }
    PARAMETRIZE { species = SPECIES_MEW; power = 60; }
    PARAMETRIZE { species = SPECIES_CUBONE; power = 80; }
    PARAMETRIZE { species = SPECIES_VENUSAUR; power = 80; }
    PARAMETRIZE { species = SPECIES_DONPHAN; power = 100; }
    PARAMETRIZE { species = SPECIES_STEELIX; power = 120; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(50); Attack(101); Ability(ABILITY_NONE); }
        OPPONENT(species) { Defense(73); Ability(ABILITY_NONE); }
    } WHEN { TURN {} }
    THEN {
        if (B_POLISHED_MOVE_BALANCE) {
            struct DamageContext ctx = {0};
            ctx.battlerAtk = 0; ctx.battlerDef = 1; ctx.move = MOVE_LOW_KICK;
            ctx.moveType = TYPE_NORMAL; ctx.typeEffectivenessModifier = UQ_4_12(1.0);
            EXPECT_EQ(CalculateMoveDamageVars(&ctx), 22 * power * 101 / 73 / 50 + 2);
        }
    }
}

SINGLE_BATTLE_TEST("Polished dynamic damage maps every source species form and Light Metal weight")
{
    GIVEN { PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); } OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); } }
    WHEN { TURN {} }
    THEN {
        for (u32 i = 0; i < ARRAY_COUNT(sPolishedWeightVectors); i++) {
            const typeof(*sPolishedWeightVectors) *v = &sPolishedWeightVectors[i];
            EXPECT_EQ(PolishedBattleWeight(v->species), v->weight);
            if (B_POLISHED_MOVE_BALANCE)
            for (u32 side = 0; side < 2; side++) for (u32 light = 0; light < 2; light++) {
                struct DamageContext ctx = {0};
                ctx.battlerAtk = side; ctx.battlerDef = side ^ 1; ctx.move = MOVE_LOW_KICK;
                ctx.moveType = TYPE_NORMAL; ctx.typeEffectivenessModifier = UQ_4_12(1.0);
                ctx.abilities[side ^ 1] = light ? ABILITY_LIGHT_METAL : ABILITY_NONE;
                gBattleMons[side].level = 50; gBattleMons[side].attack = 101;
                gBattleMons[side ^ 1].defense = 73; gBattleMons[side ^ 1].species = v->species;
                u32 w = v->weight >> light;
                if (!w) {
                    // Source loops forever here; the adapter deliberately keeps
                    // zero-weight ability-copy combinations on the native path.
                    EXPECT_EQ(UsesPolishedDamageRules(&ctx), FALSE);
                    continue;
                }
                u32 power = w > 2000 ? 120 : w > 1000 ? 100 : w > 50 ? 80 : w > 25 ? 60 : w > 10 ? 40 : 20;
                EXPECT_EQ(CalculateMoveDamageVars(&ctx), 22 * power * 101 / 73 / 50 + 2);
            }
        }
    }
}

SINGLE_BATTLE_TEST("Polished dynamic damage Low Kick covers every word weight boundary")
{
    GIVEN { PLAYER(SPECIES_WOBBUFFET); OPPONENT(SPECIES_WOBBUFFET); }
    WHEN { TURN {} }
    THEN {
        for (u32 w = 1; w <= 65535; w++) {
            u32 power = w > 2000 ? 120 : w > 1000 ? 100 : w > 50 ? 80 : w > 25 ? 60 : w > 10 ? 40 : 20;
            EXPECT_EQ(PolishedLowKickPower(w), power);
        }
        EXPECT_EQ(PolishedBattleWeight(SPECIES_NONE), 0);
        EXPECT_EQ(PolishedBattleWeight(NUM_SPECIES), 0);
        EXPECT_EQ(PolishedLowKickPower(0), 20); // Defensive API boundary, not source parity.
    }
}

SINGLE_BATTLE_TEST("Polished dynamic damage Gyro Ball includes zero and truncated zero speeds")
{
    GIVEN { PLAYER(SPECIES_WOBBUFFET); OPPONENT(SPECIES_WOBBUFFET); }
    WHEN { TURN {} }
    THEN {
        for (u32 i = 0; i < ARRAY_COUNT(sPolishedGyroVectors); i++) {
            const typeof(*sPolishedGyroVectors) *v = &sPolishedGyroVectors[i];
            EXPECT_EQ(PolishedGyroBallPower(v->user, v->target), v->power);
        }
    }
}

SINGLE_BATTLE_TEST("Polished dynamic damage Low Kick respects Light Metal after Mold Breaker")
{
    enum Ability ability;
    PARAMETRIZE { ability = ABILITY_NONE; }
    PARAMETRIZE { ability = ABILITY_MOLD_BREAKER; }
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ability); Level(50); Attack(101); Speed(100); }
        OPPONENT(SPECIES_MEW) { Ability(ABILITY_LIGHT_METAL); Defense(73); Speed(200); }
    } WHEN { TURN { MOVE(player, MOVE_LOW_KICK, WITH_RNG(RNG_DAMAGE_MODIFIER, 0)); } }
    SCENE { HP_BAR(opponent, captureDamage: &damage); }
    THEN { if (B_POLISHED_MOVE_BALANCE) EXPECT_EQ(damage, (22 * (ability == ABILITY_MOLD_BREAKER ? 60 : 40) * 101 / 73 / 50 + 2) / 2); }
}

SINGLE_BATTLE_TEST("Polished dynamic damage Gyro Ball uses effective paralysis and held Speed")
{
    enum Item item;
    u32 speed;
    PARAMETRIZE { item = ITEM_NONE; speed = 127; }
    PARAMETRIZE { item = ITEM_IRON_BALL; speed = 63; }
    PARAMETRIZE { item = ITEM_CHOICE_SCARF; speed = 190; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Level(50); Attack(101); Speed(255); Status1(STATUS1_PARALYSIS); Item(item); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Defense(73); Speed(513); }
    } WHEN { TURN {} }
    THEN {
        if (B_POLISHED_MOVE_BALANCE) {
            struct DamageContext ctx = {0};
            ctx.battlerAtk = 0; ctx.battlerDef = 1; ctx.move = MOVE_GYRO_BALL;
            ctx.moveType = TYPE_NORMAL; ctx.typeEffectivenessModifier = UQ_4_12(1.0);
            ctx.holdEffects[0] = GetBattlerHoldEffect(0);
            EXPECT_EQ(CalculateMoveDamageVars(&ctx), 22 * min(150, 25 * 513 / speed) * 101 / 73 / 50 + 2);
        }
    }
}

SINGLE_BATTLE_TEST("Polished dynamic damage native weights items and gimmicks retain their paths")
{
    GIVEN { PLAYER(SPECIES_WOBBUFFET); OPPONENT(SPECIES_WOBBUFFET); }
    WHEN { TURN {} }
    THEN {
        struct DamageContext ctx = {0};
        ctx.battlerAtk = 0; ctx.battlerDef = 1; ctx.move = MOVE_LOW_KICK;
        EXPECT_EQ(UsesPolishedDamageRules(&ctx), B_POLISHED_MOVE_BALANCE);
        ctx.holdEffects[1] = HOLD_EFFECT_FLOAT_STONE;
        EXPECT_EQ(UsesPolishedDamageRules(&ctx), FALSE);
        ctx.holdEffects[1] = HOLD_EFFECT_NONE;
        gBattleMons[1].volatiles.autotomizeCount = 1;
        EXPECT_EQ(UsesPolishedDamageRules(&ctx), FALSE);
        gBattleMons[1].volatiles.autotomizeCount = 0;
        gBattleMons[1].species = SPECIES_MUDKIP;
        EXPECT_EQ(UsesPolishedDamageRules(&ctx), FALSE);
        gBattleMons[1].species = SPECIES_WOBBUFFET;
        ctx.abilities[1] = ABILITY_HEAVY_METAL;
        EXPECT_EQ(UsesPolishedDamageRules(&ctx), FALSE);
        ctx.abilities[1] = ABILITY_NONE;
        ctx.move = MOVE_GYRO_BALL;
        SetActiveGimmick(1, GIMMICK_TERA);
        EXPECT_EQ(UsesPolishedDamageRules(&ctx), FALSE);
        SetActiveGimmick(1, GIMMICK_NONE); SetActiveGimmick(0, GIMMICK_TERA);
        EXPECT_EQ(UsesPolishedDamageRules(&ctx), FALSE);
    }
}

DOUBLE_BATTLE_TEST("Polished dynamic damage leaves all deferred moves native in doubles")
{
    GIVEN { PLAYER(SPECIES_WOBBUFFET); PLAYER(SPECIES_WOBBUFFET); OPPONENT(SPECIES_WOBBUFFET); OPPONENT(SPECIES_WOBBUFFET); }
    WHEN { TURN {} }
    THEN {
        struct DamageContext ctx = {0};
        ctx.battlerAtk = 0; ctx.battlerDef = 1;
        ctx.move = MOVE_LOW_KICK; EXPECT_EQ(UsesPolishedDamageRules(&ctx), FALSE);
        ctx.move = MOVE_GYRO_BALL; EXPECT_EQ(UsesPolishedDamageRules(&ctx), FALSE);
        ctx.move = MOVE_FUTURE_SIGHT; EXPECT_EQ(UsesPolishedDamageRules(&ctx), FALSE);
    }
}

SINGLE_BATTLE_TEST("Polished dynamic damage Future Sight uses original party stats and STAB after switching")
{
    bool32 external;
    PARAMETRIZE { external = FALSE; }
    PARAMETRIZE { external = TRUE; }
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_ABRA) { Ability(ABILITY_NONE); Item(ITEM_CHOICE_SPECS); Level(50); SpAttack(101); Speed(200); }
        PLAYER(SPECIES_PIKACHU) { Ability(ABILITY_NONE); Item(ITEM_LIFE_ORB); Level(10); SpAttack(20); Speed(150); }
        OPPONENT(SPECIES_CHANSEY) { Ability(ABILITY_NONE); SpDefense(73); MaxHP(1000); HP(1000); Speed(100); }
    } WHEN {
        TURN { MOVE(player, MOVE_FUTURE_SIGHT); }
        TURN { if (external) SWITCH(player, 1); else MOVE(player, MOVE_FUTURE_SIGHT); }
        TURN { if (!external) MOVE(player, MOVE_FUTURE_SIGHT); }
    } SCENE { MESSAGE("The opposing Chansey took the Future Sight attack!"); HP_BAR(opponent, captureDamage: &damage); }
    THEN {
        if (external) { EXPECT_EQ(player->species, SPECIES_PIKACHU); EXPECT_EQ(player->spAttack, 20); EXPECT_EQ(player->item, ITEM_LIFE_ORB); EXPECT_EQ(player->hp, player->maxHP); }
        if (B_POLISHED_MOVE_BALANCE) {
            u32 n = 22 * 120 * 101;
            if (!external) n = n * 3 / 2;
            EXPECT_EQ(damage, ((n / 73 / 50 + 2) * 3 / 2) * 85 / 100);
        }
    }
}

SINGLE_BATTLE_TEST("Polished dynamic damage Future Sight takes active stages at impact")
{
    bool32 boost;
    PARAMETRIZE { boost = FALSE; }
    PARAMETRIZE { boost = TRUE; }
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_ABRA) { Ability(ABILITY_NONE); Level(50); SpAttack(101); Speed(200); }
        OPPONENT(SPECIES_CHANSEY) { Ability(ABILITY_NONE); SpDefense(73); MaxHP(1000); HP(1000); Speed(100); }
    } WHEN {
        TURN { MOVE(player, MOVE_FUTURE_SIGHT); }
        TURN { if (boost) MOVE(player, MOVE_NASTY_PLOT); }
        TURN {}
    } SCENE { MESSAGE("The opposing Chansey took the Future Sight attack!"); HP_BAR(opponent, captureDamage: &damage); }
    THEN { if (B_POLISHED_MOVE_BALANCE) EXPECT_EQ(damage, ((22 * 120 * 101 * (boost ? 2 : 1) / 73 / 50 + 2) * 3 / 2) * 85 / 100); }
}

SINGLE_BATTLE_TEST("Polished dynamic damage Future Sight never receives Analytic or Metronome boosts")
{
    GIVEN { PLAYER(SPECIES_WOBBUFFET) { Level(50); SpAttack(101); } OPPONENT(SPECIES_WOBBUFFET) { SpDefense(73); } }
    WHEN { TURN {} }
    THEN {
        if (B_POLISHED_MOVE_BALANCE) {
            struct DamageContext ctx = {0};
            ctx.battlerAtk = 0; ctx.battlerDef = 1; ctx.move = MOVE_FUTURE_SIGHT;
            ctx.moveType = TYPE_NORMAL; ctx.typeEffectivenessModifier = UQ_4_12(1.0);
            ctx.abilities[0] = ABILITY_ANALYTIC; ctx.holdEffects[0] = HOLD_EFFECT_METRONOME;
            for (u32 count = 0; count <= 5; count++) {
                gBattleMons[0].volatiles.metronomeItemCounter = count;
                gBattleStruct->polishedMetronomeCount[0] = count;
                EXPECT_EQ(CalculateMoveDamageVars(&ctx), 22 * 120 * 101 / 73 / 50 + 2);
            }
        }
    }
}

SINGLE_BATTLE_TEST("Polished dynamic damage Future Sight blocks Dark targets after either side switches")
{
    bool32 external;
    PARAMETRIZE { external = FALSE; }
    PARAMETRIZE { external = TRUE; }
    GIVEN {
        PLAYER(SPECIES_ABRA) { Ability(ABILITY_NONE); }
        PLAYER(SPECIES_PIKACHU) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_CHANSEY) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_UMBREON) { Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(player, MOVE_FUTURE_SIGHT); }
        TURN { SWITCH(opponent, 1); if (external) SWITCH(player, 1); }
        TURN {}
    } SCENE {
        MESSAGE("The opposing Umbreon took the Future Sight attack!");
        MESSAGE("It doesn't affect the opposing Umbreon…"); NOT HP_BAR(opponent);
    }
}

SINGLE_BATTLE_TEST("Polished dynamic damage Future Sight bypasses Substitute at impact")
{
    GIVEN {
        PLAYER(SPECIES_ABRA) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_CHANSEY) { Ability(ABILITY_NONE); MaxHP(1000); HP(1000); }
    } WHEN {
        TURN { MOVE(player, MOVE_FUTURE_SIGHT); }
        TURN { MOVE(opponent, MOVE_SUBSTITUTE); }
        TURN {}
    } SCENE { MESSAGE("The opposing Chansey took the Future Sight attack!"); if (B_POLISHED_MOVE_BALANCE) HP_BAR(opponent); else NOT HP_BAR(opponent); }
    THEN { EXPECT_NE((u32)opponent->volatiles.substituteHP, 0); }
}

SINGLE_BATTLE_TEST("Polished dynamic damage Future Sight checks Fly Dig and No Guard at impact")
{
    enum Move move;
    enum Ability ability;
    for (u32 dig = 0; dig < 2; dig++) for (u32 guard = 0; guard < 2; guard++)
        PARAMETRIZE { move = dig ? MOVE_DIG : MOVE_FLY; ability = guard ? ABILITY_NO_GUARD : ABILITY_NONE; }
    GIVEN {
        PLAYER(SPECIES_ABRA) { Ability(ability); }
        OPPONENT(SPECIES_CHANSEY) { Ability(ABILITY_NONE); MaxHP(1000); HP(1000); }
    } WHEN {
        TURN { MOVE(player, MOVE_FUTURE_SIGHT); }
        TURN {}
        TURN { MOVE(opponent, move); }
    } SCENE {
        MESSAGE("The opposing Chansey took the Future Sight attack!");
        if (ability == ABILITY_NO_GUARD || !B_POLISHED_MOVE_BALANCE) HP_BAR(opponent);
        else NOT HP_BAR(opponent);
    }
}

SINGLE_BATTLE_TEST("Polished dynamic damage Future Sight preserves Endure through impact")
{
    GIVEN {
        PLAYER(SPECIES_ABRA) { Ability(ABILITY_NONE); SpAttack(999); }
        OPPONENT(SPECIES_CHANSEY) { Ability(ABILITY_NONE); HP(10); }
        OPPONENT(SPECIES_PIKACHU);
    } WHEN {
        TURN { MOVE(player, MOVE_FUTURE_SIGHT); }
        TURN {}
        TURN { MOVE(opponent, MOVE_ENDURE); if (!B_POLISHED_MOVE_BALANCE) SEND_OUT(opponent, 1); }
    } SCENE {
        MESSAGE("The opposing Chansey took the Future Sight attack!"); HP_BAR(opponent);
        if (B_POLISHED_MOVE_BALANCE) MESSAGE("The opposing Chansey endured the hit!");
    } THEN { if (B_POLISHED_MOVE_BALANCE) EXPECT_EQ(opponent->hp, 1); }
}

SINGLE_BATTLE_TEST("Polished dynamic damage Future Sight rejects duplicate scheduling and reopens after impact")
{
    GIVEN {
        PLAYER(SPECIES_ABRA) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_CHANSEY) { Ability(ABILITY_NONE); MaxHP(1000); HP(1000); }
    } WHEN {
        TURN { MOVE(player, MOVE_FUTURE_SIGHT); }
        TURN { MOVE(player, MOVE_FUTURE_SIGHT); }
        TURN {}
        TURN { MOVE(player, MOVE_FUTURE_SIGHT); }
        TURN {}
        TURN {}
    } SCENE {
        MESSAGE("But it failed!");
        MESSAGE("The opposing Chansey took the Future Sight attack!"); HP_BAR(opponent);
        MESSAGE("The opposing Chansey took the Future Sight attack!"); HP_BAR(opponent);
    }
}

SINGLE_BATTLE_TEST("Polished dynamic damage Future Sight preserves selected move history and Metronome counter")
{
    GIVEN {
        PLAYER(SPECIES_ABRA) { Ability(ABILITY_NONE); Item(ITEM_METRONOME); Speed(200); }
        OPPONENT(SPECIES_CHANSEY) { Ability(ABILITY_NONE); MaxHP(1000); HP(1000); Speed(100); }
    } WHEN {
        TURN { MOVE(player, MOVE_FUTURE_SIGHT); }
        TURN { MOVE(player, MOVE_TACKLE); MOVE(opponent, MOVE_GROWL); }
        TURN { MOVE(player, MOVE_TACKLE); MOVE(opponent, MOVE_GROWL); }
    } THEN {
        if (B_POLISHED_MOVE_BALANCE) {
            EXPECT_EQ(gLastResultingMoves[0], MOVE_TACKLE);
            EXPECT_EQ(gLastLandedMoves[1], MOVE_TACKLE);
            EXPECT_EQ((u32)player->volatiles.metronomeItemCounter, 2);
            EXPECT_EQ(gBattleStruct->polishedMetronomeCount[0], 1);
        }
    }
}

SINGLE_BATTLE_TEST("Polished dynamic damage Future Sight does not advance or clear Rollout")
{
    GIVEN {
        PLAYER(SPECIES_ABRA) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_CHANSEY) { Ability(ABILITY_NONE); MaxHP(1000); HP(1000); }
    } WHEN {
        TURN { MOVE(player, MOVE_FUTURE_SIGHT); }
        TURN {}
        TURN { MOVE(player, MOVE_ROLLOUT); }
    } THEN { if (B_POLISHED_MOVE_BALANCE) EXPECT_EQ((u32)player->volatiles.rolloutTimer, 1); }
}

SINGLE_BATTLE_TEST("Polished dynamic damage Future Sight does not take its user down through Destiny Bond")
{
    GIVEN {
        PLAYER(SPECIES_ABRA) { Ability(ABILITY_NONE); SpAttack(999); }
        PLAYER(SPECIES_PIKACHU);
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); HP(10); }
        OPPONENT(SPECIES_PIKACHU);
    } WHEN {
        TURN { MOVE(player, MOVE_FUTURE_SIGHT); }
        TURN {}
        TURN { MOVE(opponent, MOVE_DESTINY_BOND); SEND_OUT(opponent, 1); if (!B_POLISHED_MOVE_BALANCE) SEND_OUT(player, 1); }
    } SCENE {
        MESSAGE("The opposing Wobbuffet took the Future Sight attack!");
        if (B_POLISHED_MOVE_BALANCE) NOT HP_BAR(player);
    } THEN { if (B_POLISHED_MOVE_BALANCE) EXPECT_EQ(player->hp, player->maxHP); }
}

SINGLE_BATTLE_TEST("Polished dynamic damage Gyro Ball uses source speed division boundaries")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(50); Attack(101); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Defense(73); Ability(ABILITY_NONE); }
    } WHEN { TURN {} }
    THEN {
        if (B_POLISHED_MOVE_BALANCE)
        for (u32 side = 0; side < 2; side++) {
            struct DamageContext ctx = {0};
            ctx.battlerAtk = side; ctx.battlerDef = side ^ 1; ctx.move = MOVE_GYRO_BALL;
            ctx.moveType = TYPE_NORMAL; ctx.typeEffectivenessModifier = UQ_4_12(1.0);
            gBattleMons[side].level = 50; gBattleMons[side].attack = 101; gBattleMons[side ^ 1].defense = 73;
            for (u32 i = 0; i < ARRAY_COUNT(sPolishedGyroVectors); i++) {
                const typeof(*sPolishedGyroVectors) *v = &sPolishedGyroVectors[i];
                gBattleMons[side].speed = v->user; gBattleMons[side ^ 1].speed = v->target;
                // GetSpeed floors living effective Speed at one.
                if (!v->user || !v->target) continue;
                EXPECT_EQ(CalculateMoveDamageVars(&ctx), 22 * v->power * 101 / 73 / 50 + 2);
            }
        }
    }
}

SINGLE_BATTLE_TEST("Polished dynamic damage Low Kick uses a transformed target current species")
{
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_SNORLAX) { Ability(ABILITY_NONE); Level(50); Attack(101); Defense(73); Speed(100); }
        OPPONENT(SPECIES_DITTO) { Ability(ABILITY_NONE); Speed(200); }
    } WHEN { TURN { MOVE(opponent, MOVE_TRANSFORM); MOVE(player, MOVE_LOW_KICK, WITH_RNG(RNG_DAMAGE_MODIFIER, 0)); } }
    SCENE { HP_BAR(opponent, captureDamage: &damage); }
    THEN { if (B_POLISHED_MOVE_BALANCE) EXPECT_EQ(damage, (22 * 120 * 101 / 73 / 50 + 2) * 2); }
}

SINGLE_BATTLE_TEST("Polished dynamic damage Future Sight uses source critical probability with an external user")
{
    bool32 external;
    PARAMETRIZE { external = FALSE; }
    PARAMETRIZE { external = TRUE; }
    PASSES_RANDOMLY(1, B_POLISHED_MOVE_BALANCE ? 24 : 16, RNG_CRITICAL_HIT);
    GIVEN {
        WITH_CONFIG(B_CRIT_CHANCE, GEN_6);
        PLAYER(SPECIES_ABRA) { Ability(ABILITY_NONE); }
        PLAYER(SPECIES_PIKACHU) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_CHANSEY) { Ability(ABILITY_NONE); MaxHP(1000); HP(1000); }
    } WHEN {
        TURN { MOVE(player, MOVE_FUTURE_SIGHT); }
        TURN { if (external) SWITCH(player, 1); }
        TURN {}
    } SCENE { MESSAGE("The opposing Chansey took the Future Sight attack!"); MESSAGE("A critical hit!"); }
}

SINGLE_BATTLE_TEST("Polished dynamic damage external Future Sight discards replacement stages and crit items")
{
    GIVEN { PLAYER(SPECIES_WOBBUFFET); OPPONENT(SPECIES_WOBBUFFET); }
    WHEN { TURN {} }
    THEN {
        if (B_POLISHED_MOVE_BALANCE) {
            struct DamageContext ctx = {0};
            ctx.battlerAtk = 0; ctx.battlerDef = 1; ctx.move = MOVE_FUTURE_SIGHT;
            gSpecialStatuses[0].attackerInParty = TRUE;
            gBattleMons[0].volatiles.focusEnergy = TRUE;
            ctx.holdEffects[0] = HOLD_EFFECT_SCOPE_LENS;
            EXPECT_EQ(CalcCritChanceStage(&ctx), 0);
            ctx.abilities[1] = ABILITY_BATTLE_ARMOR;
            EXPECT_EQ(CalcCritChanceStage(&ctx), -1);
        }
    }
}

SINGLE_BATTLE_TEST("Polished dynamic damage Future Sight external accuracy retains only target stages")
{
    GIVEN { PLAYER(SPECIES_WOBBUFFET); OPPONENT(SPECIES_WOBBUFFET); }
    WHEN { TURN {} }
    THEN {
        if (B_POLISHED_MOVE_BALANCE) {
            struct BattleCalcValues cv = {0};
            cv.battlerAtk = 0; cv.battlerDef = 1; cv.move = MOVE_FUTURE_SIGHT;
            for (u32 external = 0; external < 2; external++) {
                gSpecialStatuses[0].attackerInParty = external;
                for (u32 acc = 0; acc <= 12; acc++) for (u32 eva = 0; eva <= 12; eva++) {
                    gBattleMons[0].statStages[STAT_ACC] = acc; gBattleMons[1].statStages[STAT_EVASION] = eva;
                    s32 delta = (external ? 6 : acc) - eva;
                    delta = max(-6, min(6, delta));
                    EXPECT_EQ(GetPolishedTotalAccuracy(&cv, 0), 100 * (3 + max(0, delta)) / (3 + max(0, -delta)));
                }
            }
        }
    }
}

SINGLE_BATTLE_TEST("Polished dynamic damage external Future Sight ignores the replacement accuracy stage")
{
    GIVEN { PLAYER(SPECIES_WOBBUFFET); OPPONENT(SPECIES_WOBBUFFET); }
    WHEN { TURN {} }
    THEN {
        if (B_POLISHED_MOVE_BALANCE) {
            struct BattleCalcValues cv = {0};
            cv.battlerAtk = 0; cv.battlerDef = 1; cv.move = MOVE_FUTURE_SIGHT;
            gSpecialStatuses[0].attackerInParty = TRUE;
            gBattleMons[0].statStages[STAT_ACC] = MIN_STAT_STAGE;
            EXPECT_EQ(GetPolishedTotalAccuracy(&cv, 0), 100);
        }
    }
}

SINGLE_BATTLE_TEST("Polished dynamic damage Future Sight respects Protect at impact")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(player, MOVE_FUTURE_SIGHT); }
        TURN {}
        TURN { MOVE(opponent, MOVE_PROTECT); }
    } SCENE {
        MESSAGE("The opposing Wobbuffet took the Future Sight attack!");
        if (B_POLISHED_MOVE_BALANCE) {
            MESSAGE("The opposing Wobbuffet protected itself!");
            NOT HP_BAR(opponent);
        } else { HP_BAR(opponent); }
    }
}

SINGLE_BATTLE_TEST("Polished dynamic damage Future Sight resolves by user speed")
{
    bool32 trickRoom;
    PARAMETRIZE { trickRoom = FALSE; }
    PARAMETRIZE { trickRoom = TRUE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(200); MaxHP(1000); HP(1000); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(100); MaxHP(1000); HP(1000); Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(player, MOVE_FUTURE_SIGHT); MOVE(opponent, MOVE_FUTURE_SIGHT); }
        TURN { if (trickRoom) MOVE(player, MOVE_TRICK_ROOM); }
        TURN {}
    } SCENE {
        if (B_POLISHED_MOVE_BALANCE && !trickRoom) {
            MESSAGE("The opposing Wobbuffet took the Future Sight attack!"); HP_BAR(opponent);
            MESSAGE("Wobbuffet took the Future Sight attack!"); HP_BAR(player);
        } else {
            MESSAGE("Wobbuffet took the Future Sight attack!"); HP_BAR(player);
            MESSAGE("The opposing Wobbuffet took the Future Sight attack!"); HP_BAR(opponent);
        }
    }
}

SINGLE_BATTLE_TEST("Polished dynamic damage Future Sight makes a fresh random choice on tied Speed")
{
    if (B_POLISHED_MOVE_BALANCE)
        PASSES_RANDOMLY(1, 2, RNG_POLISHED_FUTURE_SIGHT_SPEED_TIE);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); MaxHP(1000); HP(1000); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(100); MaxHP(1000); HP(1000); Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(player, MOVE_FUTURE_SIGHT); MOVE(opponent, MOVE_FUTURE_SIGHT); }
        TURN {}
        TURN {}
    } SCENE {
        if (B_POLISHED_MOVE_BALANCE) {
            MESSAGE("The opposing Wobbuffet took the Future Sight attack!"); HP_BAR(opponent);
            MESSAGE("Wobbuffet took the Future Sight attack!"); HP_BAR(player);
        } else {
            MESSAGE("Wobbuffet took the Future Sight attack!"); HP_BAR(player);
            MESSAGE("The opposing Wobbuffet took the Future Sight attack!"); HP_BAR(opponent);
        }
    }
}
