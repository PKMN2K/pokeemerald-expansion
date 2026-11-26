#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished command closure Growth uses effective user weather and independent stat caps")
{
    bool32 enemy = FALSE;
    u32 weather = 0, capped = 0;
    for (u32 side = 0; side < 2; side++)
        for (u32 sky = 0; sky < 6; sky++)
            for (u32 cap = 0; cap < 4; cap++)
                PARAMETRIZE { enemy = side; weather = sky; capped = cap; }
    struct BattlePokemon *user = enemy ? opponent : player;
    bool32 sun = weather >= 1 && weather <= 4;
    u32 stages = sun && weather != 2 && weather != 4 ? 2 : 1;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(enemy && weather == 4 ? ABILITY_CLOUD_NINE : ABILITY_NONE);
            if ((!enemy && weather == 2) || (enemy && weather == 3)) Item(ITEM_UTILITY_UMBRELLA);
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(!enemy && weather == 4 ? ABILITY_CLOUD_NINE : ABILITY_NONE);
            if ((enemy && weather == 2) || (!enemy && weather == 3)) Item(ITEM_UTILITY_UMBRELLA);
        }
    } WHEN {
        if (capped & 1)
            for (u32 turn = 0; turn < 3; turn++) TURN { MOVE(user, MOVE_SWORDS_DANCE); }
        if (capped & 2)
            for (u32 turn = 0; turn < 3; turn++) TURN { MOVE(user, MOVE_NASTY_PLOT); }
        if (sun) TURN { MOVE(user, MOVE_SUNNY_DAY); }
        else if (weather == 5) TURN { MOVE(user, MOVE_RAIN_DANCE); }
        TURN { MOVE(user, MOVE_GROWTH); }
    } THEN {
        EXPECT_EQ(user->statStages[STAT_ATK], capped & 1 ? MAX_STAT_STAGE : DEFAULT_STAT_STAGE + stages);
        EXPECT_EQ(user->statStages[STAT_SPATK], capped & 2 ? MAX_STAT_STAGE : DEFAULT_STAT_STAGE + stages);
    }
}

SINGLE_BATTLE_TEST("Polished command closure Haze resets all seven stages on both sides")
{
    bool32 enemy = FALSE, haze = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 reset = 0; reset < 2; reset++)
            PARAMETRIZE { enemy = side; haze = reset; }
    struct BattlePokemon *user = enemy ? opponent : player;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); HP(4000); MaxHP(4000); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); HP(4000); MaxHP(4000); }
    } WHEN {
        TURN { MOVE(player, MOVE_ANCIENT_POWER, secondaryEffect: TRUE); MOVE(opponent, MOVE_ANCIENT_POWER, secondaryEffect: TRUE); }
        TURN { MOVE(player, MOVE_DOUBLE_TEAM); MOVE(opponent, MOVE_DOUBLE_TEAM); }
        TURN { MOVE(player, MOVE_SAND_ATTACK, hit: TRUE); MOVE(opponent, MOVE_SAND_ATTACK, hit: TRUE); }
        if (haze) TURN { MOVE(user, MOVE_HAZE); }
    } THEN {
        for (u32 battler = 0; battler < 2; battler++)
            for (u32 stat = STAT_ATK; stat <= STAT_EVASION; stat++) {
                s32 change = haze ? 0 : stat == STAT_ACC ? -1 : 1;
                EXPECT_EQ(gBattleMons[battler].statStages[stat], DEFAULT_STAT_STAGE + change);
            }
    }
}

SINGLE_BATTLE_TEST("Polished command closure Haze preserves Focus Energy Minimize and Defense Curl")
{
    bool32 enemy = FALSE;
    PARAMETRIZE { enemy = FALSE; }
    PARAMETRIZE { enemy = TRUE; }
    struct BattlePokemon *user = enemy ? opponent : player;
    GIVEN {
        WITH_CONFIG(B_FOCUS_ENERGY_CRIT_RATIO, GEN_6);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(player, MOVE_FOCUS_ENERGY); MOVE(opponent, MOVE_FOCUS_ENERGY); }
        TURN { MOVE(player, MOVE_MINIMIZE); MOVE(opponent, MOVE_MINIMIZE); }
        TURN { MOVE(player, MOVE_DEFENSE_CURL); MOVE(opponent, MOVE_DEFENSE_CURL); }
        TURN { MOVE(user, MOVE_HAZE); }
    } THEN {
        for (u32 battler = 0; battler < 2; battler++) {
            EXPECT_EQ((u32)gBattleMons[battler].volatiles.focusEnergy, TRUE);
            EXPECT_EQ((u32)gBattleMons[battler].volatiles.minimize, TRUE);
            EXPECT_EQ((u32)gBattleMons[battler].volatiles.defenseCurl, TRUE);
            for (u32 stat = STAT_ATK; stat <= STAT_EVASION; stat++)
                EXPECT_EQ(gBattleMons[battler].statStages[stat], DEFAULT_STAT_STAGE);
        }
    }
}

SINGLE_BATTLE_TEST("Polished command closure capped Defense Curl still boosts Rollout", s16 damage)
{
    bool32 enemy = FALSE;
    u32 curl = 0;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 3; kind++)
            PARAMETRIZE { enemy = side; curl = kind; }
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); HP(4000); MaxHP(4000); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); HP(4000); MaxHP(4000); }
    } WHEN {
        if (curl == 2)
            for (u32 turn = 0; turn < 3; turn++) TURN { MOVE(user, MOVE_ACID_ARMOR); }
        if (curl) TURN { MOVE(user, MOVE_DEFENSE_CURL); }
        TURN { MOVE(user, MOVE_ROLLOUT); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_ROLLOUT, user);
        HP_BAR(target, captureDamage: &results[i].damage);
    } THEN {
        EXPECT_EQ((u32)user->volatiles.defenseCurl, !!curl);
        if (curl == 2) EXPECT_EQ(user->statStages[STAT_DEF], MAX_STAT_STAGE);
    } FINALLY {
        for (u32 side = 0; side < 2; side++) {
            EXPECT_GT(results[3 * side + 1].damage, results[3 * side].damage);
            EXPECT_EQ(results[3 * side + 1].damage, results[3 * side + 2].damage);
        }
    }
}

SINGLE_BATTLE_TEST("Polished command closure Sucker Punch checks attack status earlier action and item selection")
{
    bool32 enemy = FALSE;
    u32 action = 0;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 4; kind++)
            PARAMETRIZE { enemy = side; action = kind; }
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); HP(500); MaxHP(1000); Speed(enemy ? 100 : 50); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); HP(500); MaxHP(1000); Speed(enemy ? 50 : 100); }
    } WHEN {
        TURN {
            MOVE(user, MOVE_SUCKER_PUNCH);
            if (action == 3) USE_ITEM(target, ITEM_POTION, partyIndex: 0);
            else MOVE(target, action == 0 ? MOVE_TACKLE : action == 1 ? MOVE_SPLASH : MOVE_QUICK_ATTACK);
        }
    } SCENE {
        if (action == 0) ANIMATION(ANIM_TYPE_MOVE, MOVE_SUCKER_PUNCH, user);
        else NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_SUCKER_PUNCH, user);
    } THEN {
        if (action == 0) EXPECT_LT(target->hp, 500);
        else if (action != 3) EXPECT_EQ(target->hp, 500);
    }
}

SINGLE_BATTLE_TEST("Polished command closure Transform rejects hidden targets with No Guard under the oldest preset")
{
    bool32 enemy = FALSE, targetGuard = FALSE;
    enum Move hide = MOVE_FLY;
    for (u32 side = 0; side < 2; side++)
        for (u32 guard = 0; guard < 2; guard++)
            for (u32 underground = 0; underground < 2; underground++)
                PARAMETRIZE { enemy = side; targetGuard = guard; hide = underground ? MOVE_DIG : MOVE_FLY; }
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    GIVEN {
        WITH_CONFIG(B_TRANSFORM_SEMI_INV_FAIL, GEN_1);
        PLAYER(SPECIES_WOBBUFFET) { Ability(enemy == targetGuard ? ABILITY_NO_GUARD : ABILITY_NONE); Speed(enemy ? 100 : 50); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(enemy != targetGuard ? ABILITY_NO_GUARD : ABILITY_NONE); Speed(enemy ? 50 : 100); }
    } WHEN {
        TURN { MOVE(target, hide); MOVE(user, MOVE_TRANSFORM); }
    } THEN {
        EXPECT_EQ((u32)user->volatiles.transformed, !B_POLISHED_MOVE_BALANCE);
    }
}

SINGLE_BATTLE_TEST("Polished command closure Skill Swap clears both earned Flash Fire and Unburden states")
{
    bool32 enemy = FALSE, unburden = FALSE, swap = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 effect = 0; effect < 2; effect++)
            for (u32 change = 0; change < 2; change++)
                PARAMETRIZE { enemy = side; unburden = effect; swap = change; }
    struct BattlePokemon *user = enemy ? opponent : player;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(unburden ? ABILITY_UNBURDEN : ABILITY_FLASH_FIRE);
            HP(unburden ? 100 : 1000); MaxHP(1000); if (unburden) Item(ITEM_ORAN_BERRY);
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(unburden ? ABILITY_UNBURDEN : ABILITY_FLASH_FIRE);
            HP(unburden ? 100 : 1000); MaxHP(1000); if (unburden) Item(ITEM_ORAN_BERRY);
        }
    } WHEN {
        if (unburden) TURN { MOVE(player, MOVE_SPLASH); MOVE(opponent, MOVE_SPLASH); }
        else TURN { MOVE(player, MOVE_EMBER); MOVE(opponent, MOVE_EMBER); }
        if (swap) TURN { MOVE(user, MOVE_SKILL_SWAP); }
    } THEN {
        for (u32 battler = 0; battler < 2; battler++) {
            EXPECT_EQ((u32)gBattleMons[battler].volatiles.flashFireBoosted, !swap && !unburden);
            EXPECT_EQ((u32)gBattleMons[battler].volatiles.unburdenActive, !swap && unburden);
        }
    }
}

SINGLE_BATTLE_TEST("Polished command closure Skill Swap skips Cloud Nine reentry after the swap announcement")
{
    bool32 enemy = FALSE;
    PARAMETRIZE { enemy = FALSE; }
    PARAMETRIZE { enemy = TRUE; }
    struct BattlePokemon *user = enemy ? opponent : player;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(enemy ? ABILITY_CLOUD_NINE : ABILITY_PRESSURE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(enemy ? ABILITY_PRESSURE : ABILITY_CLOUD_NINE); }
    } WHEN {
        TURN { MOVE(user, MOVE_SKILL_SWAP); }
    } SCENE {
        if (enemy) MESSAGE("The opposing Wobbuffet swapped Abilities with its target!");
        else MESSAGE("Wobbuffet swapped Abilities with its target!");
        if (B_POLISHED_MOVE_BALANCE) NOT ABILITY_POPUP(user, ABILITY_CLOUD_NINE);
        else ABILITY_POPUP(user, ABILITY_CLOUD_NINE);
    } THEN {
        EXPECT_EQ(user->ability, ABILITY_CLOUD_NINE);
    }
}

SINGLE_BATTLE_TEST("Polished command closure Skill Swap does not activate a newly acquired Imposter")
{
    bool32 enemy = FALSE;
    PARAMETRIZE { enemy = FALSE; }
    PARAMETRIZE { enemy = TRUE; }
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    GIVEN {
        PLAYER(enemy ? SPECIES_WOBBUFFET : SPECIES_MEWTWO_ARMORED) {
            Ability(enemy ? ABILITY_IMPOSTER : ABILITY_PRESSURE); Speed(enemy ? 50 : 100);
            if (!enemy) Item(ITEM_POLISHED_ARMOR_SUIT);
            Moves(MOVE_SKILL_SWAP, MOVE_SPLASH);
        }
        OPPONENT(enemy ? SPECIES_MEWTWO_ARMORED : SPECIES_WOBBUFFET) {
            Ability(enemy ? ABILITY_PRESSURE : ABILITY_IMPOSTER); Speed(enemy ? 100 : 50);
            if (enemy) Item(ITEM_POLISHED_ARMOR_SUIT);
            Moves(MOVE_SKILL_SWAP, MOVE_SPLASH);
        }
    } WHEN {
        TURN { MOVE(user, MOVE_SKILL_SWAP); MOVE(target, MOVE_SPLASH); }
    } THEN {
        EXPECT_EQ(user->ability, B_POLISHED_MOVE_BALANCE ? ABILITY_IMPOSTER : ABILITY_PRESSURE);
        EXPECT_EQ((u32)user->volatiles.transformed, FALSE);
        EXPECT_EQ(user->species, SPECIES_MEWTWO_ARMORED);
        EXPECT_EQ((u32)target->volatiles.transformed, !B_POLISHED_MOVE_BALANCE);
    }
}

SINGLE_BATTLE_TEST("Polished command closure rampage preserves existing confusion on its final action")
{
    bool32 enemy = FALSE;
    PARAMETRIZE { enemy = FALSE; }
    PARAMETRIZE { enemy = TRUE; }
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    GIVEN {
        WITH_CONFIG(B_RAMPAGE_CONFUSION, GEN_5);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); HP(4000); MaxHP(4000); Speed(enemy ? 100 : 50); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); HP(4000); MaxHP(4000); Speed(enemy ? 50 : 100); }
    } WHEN {
        TURN { MOVE(user, MOVE_THRASH, WITH_RNG(RNG_RAMPAGE_TURNS, 2)); }
        TURN { MOVE(target, MOVE_CONFUSE_RAY, WITH_RNG(RNG_CONFUSION_TURNS, 5)); SKIP_TURN(user); }
    } THEN {
        EXPECT_EQ((u32)user->volatiles.confusionTimer, 4);
        EXPECT_EQ((u32)user->volatiles.rampageTurns, 0);
        EXPECT_EQ((u32)user->volatiles.multipleTurns, FALSE);
    }
}

SINGLE_BATTLE_TEST("Polished command closure rampage confuses after a final protected action under an old preset")
{
    bool32 enemy = FALSE;
    PARAMETRIZE { enemy = FALSE; }
    PARAMETRIZE { enemy = TRUE; }
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    GIVEN {
        WITH_CONFIG(B_RAMPAGE_CONFUSION, GEN_3);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); HP(4000); MaxHP(4000); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); HP(4000); MaxHP(4000); }
    } WHEN {
        TURN { MOVE(user, MOVE_THRASH, WITH_RNG(RNG_RAMPAGE_TURNS, 2)); }
        TURN { MOVE(target, MOVE_PROTECT); SKIP_TURN(user); }
    } THEN {
        EXPECT_EQ(!!user->volatiles.confusionTimer, B_POLISHED_MOVE_BALANCE);
        EXPECT_EQ((u32)user->volatiles.multipleTurns, FALSE);
    }
}

SINGLE_BATTLE_TEST("Polished command closure a final rampage contact knockout never confuses the fainted user")
{
    bool32 enemy = FALSE;
    PARAMETRIZE { enemy = FALSE; }
    PARAMETRIZE { enemy = TRUE; }
    struct BattlePokemon *user = enemy ? opponent : player;
    GIVEN {
        WITH_CONFIG(B_RAMPAGE_CONFUSION, GEN_5);
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(ABILITY_NONE); HP(enemy ? 4000 : 40); MaxHP(enemy ? 4000 : 160); Speed(enemy ? 50 : 100);
            if (enemy) Item(ITEM_ROCKY_HELMET);
        }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(ABILITY_NONE); HP(enemy ? 40 : 4000); MaxHP(enemy ? 160 : 4000); Speed(enemy ? 100 : 50);
            if (!enemy) Item(ITEM_ROCKY_HELMET);
        }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(100); }
    } WHEN {
        TURN { MOVE(user, MOVE_THRASH, WITH_RNG(RNG_RAMPAGE_TURNS, 2)); }
        TURN { SKIP_TURN(user); SEND_OUT(user, 1); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_THRASH, user);
        HP_BAR(user);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_THRASH, user);
        HP_BAR(user);
        NOT ANIMATION(ANIM_TYPE_STATUS, B_ANIM_STATUS_CONFUSION, user);
        if (enemy) MESSAGE("The opposing Wobbuffet fainted!");
        else MESSAGE("Wobbuffet fainted!");
    }
}

SINGLE_BATTLE_TEST("Polished command closure Future Sight cannot finish a running rampage")
{
    bool32 enemy = FALSE;
    PARAMETRIZE { enemy = FALSE; }
    PARAMETRIZE { enemy = TRUE; }
    struct BattlePokemon *user = enemy ? opponent : player;
    GIVEN {
        WITH_CONFIG(B_RAMPAGE_CONFUSION, GEN_5);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); HP(4000); MaxHP(4000); Speed(enemy ? 50 : 100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); HP(4000); MaxHP(4000); Speed(enemy ? 100 : 50); }
    } WHEN {
        TURN { MOVE(user, MOVE_FUTURE_SIGHT); }
        TURN { MOVE(user, MOVE_THRASH, WITH_RNG(RNG_RAMPAGE_TURNS, 3)); }
        TURN { SKIP_TURN(user); }
    } THEN {
        EXPECT_EQ((u32)user->volatiles.rampageTurns, B_POLISHED_MOVE_BALANCE ? 1 : 0);
        EXPECT_EQ(!!user->volatiles.confusionTimer, !B_POLISHED_MOVE_BALANCE);
        EXPECT_EQ((u32)user->volatiles.multipleTurns, B_POLISHED_MOVE_BALANCE);
    }
}

SINGLE_BATTLE_TEST("Polished command closure Substitute honors minimum equal-cost and repeated creation boundaries")
{
    bool32 enemy = FALSE, enough = FALSE, repeat = FALSE;
    u32 maximum = 3;
    for (u32 side = 0; side < 2; side++)
        for (u32 hp = 0; hp < 4; hp++)
            for (u32 threshold = 0; threshold < 2; threshold++)
                for (u32 twice = 0; twice < 2; twice++)
                    PARAMETRIZE { enemy = side; maximum = hp < 2 ? hp + 3 : hp + 98; enough = threshold; repeat = twice; }
    u32 cost = max(1, maximum / 4), current = cost + enough;
    struct BattlePokemon *user = enemy ? opponent : player;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); if (!enemy) { HP(current); MaxHP(maximum); MovesWithPP({MOVE_SUBSTITUTE, 10}); } }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); if (enemy) { HP(current); MaxHP(maximum); MovesWithPP({MOVE_SUBSTITUTE, 10}); } }
    } WHEN {
        TURN { MOVE(user, MOVE_SUBSTITUTE); }
        if (repeat) TURN { MOVE(user, MOVE_SUBSTITUTE); }
    } THEN {
        EXPECT_EQ(user->hp, enough ? current - cost : current);
        EXPECT_EQ((u32)user->volatiles.substitute, enough);
        EXPECT_EQ((u32)user->volatiles.substituteHP, enough ? cost : 0);
        EXPECT_EQ(user->pp[0], 9 - repeat);
    }
}

SINGLE_BATTLE_TEST("Polished command closure screen durations and failed refresh stay independent with Light Clay")
{
    bool32 enemy = FALSE, clay = FALSE;
    u32 elapsed = 4;
    for (u32 side = 0; side < 2; side++)
        for (u32 item = 0; item < 2; item++)
            for (u32 end = 0; end < 3; end++)
                PARAMETRIZE { enemy = side; clay = item; elapsed = end == 0 ? 4 : (item ? 8 : 5) + end - 1; }
    u32 duration = clay ? 8 : 5;
    u32 side = enemy ? B_SIDE_OPPONENT : B_SIDE_PLAYER;
    struct BattlePokemon *user = enemy ? opponent : player;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); if (!enemy && clay) Item(ITEM_LIGHT_CLAY); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); if (enemy && clay) Item(ITEM_LIGHT_CLAY); }
    } WHEN {
        TURN { MOVE(user, MOVE_REFLECT); }
        TURN { MOVE(user, MOVE_LIGHT_SCREEN); }
        TURN { MOVE(user, MOVE_REFLECT); }
        TURN { MOVE(user, MOVE_LIGHT_SCREEN); }
        for (u32 turn = 4; turn < elapsed; turn++) TURN {}
    } THEN {
        u32 reflect = elapsed < duration ? duration - elapsed : 0;
        u32 screen = elapsed < duration + 1 ? duration + 1 - elapsed : 0;
        EXPECT_EQ(gSideTimers[side].reflectTimer, reflect);
        EXPECT_EQ(gSideTimers[side].lightscreenTimer, screen);
        EXPECT_EQ(!!(gSideStatuses[side] & SIDE_STATUS_REFLECT), !!reflect);
        EXPECT_EQ(!!(gSideStatuses[side] & SIDE_STATUS_LIGHTSCREEN), !!screen);
    }
}

SINGLE_BATTLE_TEST("Polished command closure Brick Break removes both screens only after a successful hit")
{
    bool32 enemy = FALSE;
    u32 outcome = 0;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 6; kind++)
            PARAMETRIZE { enemy = side; outcome = kind; }
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    bool32 succeeds = outcome <= 2;
    GIVEN {
        PLAYER(enemy && outcome == 5 ? SPECIES_GASTLY : SPECIES_WOBBUFFET) {
            Ability(enemy ? ABILITY_NONE : ABILITY_HUSTLE); HP(enemy && outcome == 2 ? 80 : 4000); MaxHP(enemy && outcome == 2 ? 80 : 4000); Speed(enemy ? 100 : 50);
            if (enemy && outcome == 2) Defense(10);
            if (enemy) Item(ITEM_LIGHT_CLAY);
        }
        OPPONENT(!enemy && outcome == 5 ? SPECIES_GASTLY : SPECIES_WOBBUFFET) {
            Ability(enemy ? ABILITY_HUSTLE : ABILITY_NONE); HP(!enemy && outcome == 2 ? 80 : 4000); MaxHP(!enemy && outcome == 2 ? 80 : 4000); Speed(enemy ? 50 : 100);
            if (!enemy && outcome == 2) Defense(10);
            if (!enemy) Item(ITEM_LIGHT_CLAY);
        }
    } WHEN {
        TURN { MOVE(target, MOVE_REFLECT); }
        TURN { MOVE(target, MOVE_LIGHT_SCREEN); }
        if (outcome == 1 || outcome == 2) TURN { MOVE(target, MOVE_SUBSTITUTE); }
        TURN {
            if (outcome == 3) MOVE(target, MOVE_PROTECT);
            MOVE(user, MOVE_BRICK_BREAK, hit: outcome != 4);
        }
    } THEN {
        u32 side = enemy ? B_SIDE_PLAYER : B_SIDE_OPPONENT;
        EXPECT_EQ(!!(gSideStatuses[side] & SIDE_STATUS_REFLECT), !succeeds);
        EXPECT_EQ(!!(gSideStatuses[side] & SIDE_STATUS_LIGHTSCREEN), !succeeds);
        if (outcome == 1 || outcome == 2) EXPECT_EQ((u32)target->volatiles.substitute, outcome == 1);
    }
}

SINGLE_BATTLE_TEST("Polished command closure Brick Break clears screens after HP damage including a lethal hit")
{
    bool32 enemy = FALSE, lethal = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 faint = 0; faint < 2; faint++)
            PARAMETRIZE { enemy = side; lethal = faint; }
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); HP(enemy && lethal ? 1 : 4000); MaxHP(4000); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); HP(!enemy && lethal ? 1 : 4000); MaxHP(4000); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(target, MOVE_REFLECT); }
        TURN { MOVE(target, MOVE_LIGHT_SCREEN); }
        TURN { MOVE(user, MOVE_BRICK_BREAK); if (lethal) SEND_OUT(target, 1); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_BRICK_BREAK, user);
        if (B_POLISHED_MOVE_BALANCE) HP_BAR(target);
        if (enemy) {
            MESSAGE("Your side's Reflect wore off!");
            MESSAGE("Your side's Light Screen wore off!");
        } else {
            MESSAGE("The opposing side's Reflect wore off!");
            MESSAGE("The opposing side's Light Screen wore off!");
        }
        if (!B_POLISHED_MOVE_BALANCE) HP_BAR(target);
    } THEN {
        u32 side = enemy ? B_SIDE_PLAYER : B_SIDE_OPPONENT;
        EXPECT_EQ(gSideStatuses[side] & (SIDE_STATUS_REFLECT | SIDE_STATUS_LIGHTSCREEN), 0);
        if (B_POLISHED_MOVE_BALANCE) {
            EXPECT_EQ(gSideTimers[side].reflectTimer, 0);
            EXPECT_EQ(gSideTimers[side].lightscreenTimer, 0);
        }
    }
}

SINGLE_BATTLE_TEST("Polished command closure Safeguard rejects refresh and expires after exactly five turns")
{
    bool32 enemy = FALSE;
    u32 elapsed = 2;
    for (u32 side = 0; side < 2; side++)
        for (u32 turn = 2; turn <= 6; turn++)
            PARAMETRIZE { enemy = side; elapsed = turn; }
    struct BattlePokemon *user = enemy ? opponent : player;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(user, MOVE_SAFEGUARD); }
        TURN { MOVE(user, MOVE_SAFEGUARD); }
        for (u32 turn = 2; turn < elapsed; turn++) TURN {}
    } THEN {
        u32 side = enemy ? B_SIDE_OPPONENT : B_SIDE_PLAYER;
        EXPECT_EQ(gSideTimers[side].safeguardTimer, elapsed < 5 ? 5 - elapsed : 0);
        EXPECT_EQ(!!(gSideStatuses[side] & SIDE_STATUS_SAFEGUARD), elapsed < 5);
    }
}
