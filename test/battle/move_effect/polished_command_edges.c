#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished command Attract rejects Fly and Dig even when either battler has No Guard")
{
    bool32 enemy = FALSE, targetNoGuard = FALSE;
    enum Move hide = MOVE_FLY;
    for (u32 side = 0; side < 2; side++)
        for (u32 guard = 0; guard < 2; guard++)
            for (u32 underground = 0; underground < 2; underground++)
                PARAMETRIZE { enemy = side; targetNoGuard = guard; hide = underground ? MOVE_DIG : MOVE_FLY; }
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Gender(enemy ? MON_FEMALE : MON_MALE); Speed(enemy ? 100 : 50);
            Ability(enemy == targetNoGuard ? ABILITY_NO_GUARD : ABILITY_NONE);
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Gender(enemy ? MON_MALE : MON_FEMALE); Speed(enemy ? 50 : 100);
            Ability(enemy != targetNoGuard ? ABILITY_NO_GUARD : ABILITY_NONE);
        }
    } WHEN {
        TURN { MOVE(target, hide); MOVE(user, MOVE_ATTRACT); }
    } SCENE {
        if (B_POLISHED_MOVE_BALANCE) NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_ATTRACT, user);
        else ANIMATION(ANIM_TYPE_MOVE, MOVE_ATTRACT, user);
    } THEN {
        EXPECT_EQ(!!target->volatiles.infatuation, !B_POLISHED_MOVE_BALANCE);
    }
}

SINGLE_BATTLE_TEST("Polished command Attract checks gender before announcing Oblivious")
{
    bool32 enemy = FALSE;
    u32 gender = 0;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 3; kind++)
            PARAMETRIZE { enemy = side; gender = kind; }
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    GIVEN {
        PLAYER(enemy && gender == 2 ? SPECIES_MAGNEMITE : SPECIES_WOBBUFFET) {
            Ability(enemy ? ABILITY_OBLIVIOUS : ABILITY_NONE);
            if (!enemy || gender != 2) Gender(enemy && gender == 1 ? MON_FEMALE : MON_MALE);
        }
        OPPONENT(!enemy && gender == 2 ? SPECIES_MAGNEMITE : SPECIES_WOBBUFFET) {
            Ability(!enemy ? ABILITY_OBLIVIOUS : ABILITY_NONE);
            if (enemy || gender != 2) Gender(!enemy && gender == 1 ? MON_FEMALE : MON_MALE);
        }
    } WHEN { TURN { MOVE(user, MOVE_ATTRACT); } }
    SCENE {
        if (B_POLISHED_MOVE_BALANCE && gender != 1) NOT ABILITY_POPUP(target, ABILITY_OBLIVIOUS);
        else ABILITY_POPUP(target, ABILITY_OBLIVIOUS);
    } THEN { EXPECT_EQ((u32)target->volatiles.infatuation, 0); }
}

SINGLE_BATTLE_TEST("Polished command Attract retains an existing infatuation when its repeat fails")
{
    bool32 enemy = FALSE;
    PARAMETRIZE { enemy = FALSE; }
    PARAMETRIZE { enemy = TRUE; }
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Gender(MON_MALE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Gender(MON_FEMALE); }
    } WHEN {
        TURN { MOVE(user, MOVE_ATTRACT); }
        TURN { MOVE(user, MOVE_ATTRACT); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_ATTRACT, user);
        MESSAGE("But it failed!");
    } THEN { EXPECT_NE((u32)target->volatiles.infatuation, 0); }
}

SINGLE_BATTLE_TEST("Polished command Foresight rejects a repeat under the native generation three preset")
{
    bool32 enemy = FALSE;
    PARAMETRIZE { enemy = FALSE; }
    PARAMETRIZE { enemy = TRUE; }
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    GIVEN {
        WITH_CONFIG(B_FORESIGHT_FAIL, GEN_3);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); if (!enemy) MovesWithPP({MOVE_FORESIGHT, 10}); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); if (enemy) MovesWithPP({MOVE_FORESIGHT, 10}); }
    } WHEN {
        TURN { MOVE(user, MOVE_FORESIGHT); }
        TURN { MOVE(user, MOVE_FORESIGHT); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_FORESIGHT, user);
        if (B_POLISHED_MOVE_BALANCE) MESSAGE("But it failed!");
        else ANIMATION(ANIM_TYPE_MOVE, MOVE_FORESIGHT, user);
    } THEN {
        EXPECT_EQ((u32)target->volatiles.foresight, TRUE);
        EXPECT_EQ(user->pp[0], 8);
    }
}

SINGLE_BATTLE_TEST("Polished command Minimize sets its volatile even when Evasion is already capped", s16 damage)
{
    bool32 enemy = FALSE, minimize = FALSE;
    enum Move attack = MOVE_STOMP;
    for (u32 side = 0; side < 2; side++)
        for (u32 move = 0; move < 2; move++)
            for (u32 use = 0; use < 2; use++)
                PARAMETRIZE { enemy = side; attack = move ? MOVE_BODY_SLAM : MOVE_STOMP; minimize = use; }
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *other = enemy ? player : opponent;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); HP(2000); MaxHP(2000); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); HP(2000); MaxHP(2000); }
    } WHEN {
        for (u32 turn = 0; turn < 6; turn++)
            TURN { MOVE(user, MOVE_DOUBLE_TEAM); }
        if (minimize) TURN { MOVE(user, MOVE_MINIMIZE); }
        TURN { MOVE(other, attack, hit: TRUE, secondaryEffect: FALSE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, attack, other);
        HP_BAR(user, captureDamage: &results[i].damage);
    } THEN {
        EXPECT_EQ(user->statStages[STAT_EVASION], MAX_STAT_STAGE);
        EXPECT_EQ((u32)user->volatiles.minimize, B_POLISHED_MOVE_BALANCE && minimize);
    } FINALLY {
        for (u32 pair = 0; pair < 4; pair++) {
            if (B_POLISHED_MOVE_BALANCE)
                EXPECT_MUL_EQ(results[2 * pair].damage, UQ_4_12(2.0), results[2 * pair + 1].damage);
            else
                EXPECT_EQ(results[2 * pair].damage, results[2 * pair + 1].damage);
        }
    }
}

SINGLE_BATTLE_TEST("Polished command Focus Energy keeps two source stages under the native generation two preset")
{
    bool32 enemy = FALSE;
    PARAMETRIZE { enemy = FALSE; }
    PARAMETRIZE { enemy = TRUE; }
    struct BattlePokemon *user = enemy ? opponent : player;
    PASSES_RANDOMLY(1, B_POLISHED_MOVE_BALANCE ? 2 : 8, RNG_CRITICAL_HIT);
    GIVEN {
        WITH_CONFIG(B_CRIT_CHANCE, GEN_3);
        WITH_CONFIG(B_FOCUS_ENERGY_CRIT_RATIO, GEN_2);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); HP(2000); MaxHP(2000); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); HP(2000); MaxHP(2000); }
    } WHEN {
        TURN { MOVE(user, MOVE_FOCUS_ENERGY); }
        TURN { MOVE(user, MOVE_FOCUS_ENERGY); }
        TURN { MOVE(user, MOVE_TACKLE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_FOCUS_ENERGY, user);
        MESSAGE("But it failed!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, user);
        MESSAGE("A critical hit!");
    } THEN {
        EXPECT_EQ((u32)user->volatiles.focusEnergy, B_POLISHED_MOVE_BALANCE);
        EXPECT_EQ((u32)user->volatiles.dragonCheer, !B_POLISHED_MOVE_BALANCE);
    }
}

SINGLE_BATTLE_TEST("Polished command sleeping Rollout skips the Defense Curl damage multiplier", s16 damage)
{
    bool32 enemy = FALSE, curled = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 curl = 0; curl < 2; curl++)
            PARAMETRIZE { enemy = side; curled = curl; }
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *other = enemy ? player : opponent;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(ABILITY_NONE); Level(50); Attack(200); Defense(100); HP(2000); MaxHP(2000); Speed(enemy ? 50 : 100);
            if (!enemy) Moves(MOVE_METRONOME, MOVE_SLEEP_TALK, MOVE_ROLLOUT);
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(ABILITY_NONE); Level(50); Attack(200); Defense(100); HP(2000); MaxHP(2000); Speed(enemy ? 100 : 50);
            if (enemy) Moves(MOVE_METRONOME, MOVE_SLEEP_TALK, MOVE_ROLLOUT);
        }
    } WHEN {
        // Calling Curl leaves Sleep Talk with Rollout as its only eligible slot.
        TURN {
            MOVE(user, MOVE_METRONOME, WITH_RNG(RNG_METRONOME,
                B_POLISHED_MOVE_BALANCE ? (curled ? 111 : 150) : (curled ? MOVE_DEFENSE_CURL : MOVE_SPLASH)));
            MOVE(other, MOVE_SPORE, WITH_RNG(RNG_SLEEP_TURNS, 4));
        }
        TURN { MOVE(user, MOVE_SLEEP_TALK); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, curled ? MOVE_DEFENSE_CURL : MOVE_SPLASH, user);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SPORE, other);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_ROLLOUT, user);
        HP_BAR(other, captureDamage: &results[i].damage);
    } THEN {
        EXPECT_EQ((u32)user->volatiles.defenseCurl, curled);
        EXPECT_NE(user->status1 & STATUS1_SLEEP, 0);
    } FINALLY {
        for (u32 side = 0; side < 2; side++) {
            if (B_POLISHED_MOVE_BALANCE)
                EXPECT_EQ(results[2 * side].damage, results[2 * side + 1].damage);
            else
                EXPECT_GT(results[2 * side + 1].damage, results[2 * side].damage);
        }
    }
}

SINGLE_BATTLE_TEST("Polished command Bug Bite consumes through an intact Substitute while optional Pluck stays native")
{
    bool32 enemy = FALSE, infiltrator = FALSE;
    enum Move move = MOVE_BUG_BITE;
    for (u32 side = 0; side < 2; side++)
        for (u32 bypass = 0; bypass < 2; bypass++)
            for (u32 optional = 0; optional < 2; optional++)
                PARAMETRIZE { enemy = side; infiltrator = bypass; move = optional ? MOVE_PLUCK : MOVE_BUG_BITE; }
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    bool32 eats = infiltrator || (B_POLISHED_MOVE_BALANCE && move == MOVE_BUG_BITE);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(!enemy && infiltrator ? ABILITY_INFILTRATOR : ABILITY_NONE); Attack(10); Defense(1000);
            HP(enemy ? 1000 : 200); MaxHP(enemy ? 1000 : 400); if (enemy) Item(ITEM_ORAN_BERRY);
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(enemy && infiltrator ? ABILITY_INFILTRATOR : ABILITY_NONE); Attack(10); Defense(1000);
            HP(!enemy ? 1000 : 200); MaxHP(!enemy ? 1000 : 400); if (!enemy) Item(ITEM_ORAN_BERRY);
        }
    } WHEN {
        TURN { MOVE(target, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, move); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, user);
        if (infiltrator) HP_BAR(target);
        else SUB_HIT(target);
    } THEN {
        EXPECT_EQ((u32)target->volatiles.substitute, TRUE);
        EXPECT_EQ(target->item, eats ? ITEM_NONE : ITEM_ORAN_BERRY);
        EXPECT_EQ(user->hp, eats ? 210 : 200);
    }
}

SINGLE_BATTLE_TEST("Polished command Thief and Knock Off check the current Substitute after damage and honor Infiltrator")
{
    bool32 enemy = FALSE, breaks = FALSE, infiltrator = FALSE;
    enum Move move = MOVE_THIEF;
    for (u32 side = 0; side < 2; side++)
        for (u32 broken = 0; broken < 2; broken++)
            for (u32 bypass = 0; bypass < 2; bypass++)
                for (u32 knock = 0; knock < 2; knock++)
                    PARAMETRIZE { enemy = side; breaks = broken; infiltrator = bypass; move = knock ? MOVE_KNOCK_OFF : MOVE_THIEF; }
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    bool32 removed = infiltrator || (B_POLISHED_MOVE_BALANCE && breaks);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(!enemy && infiltrator ? ABILITY_INFILTRATOR : ABILITY_NONE);
            Level(50); Attack(!enemy && breaks ? 400 : 10); Defense(50); HP(1000); MaxHP(1000);
            if (enemy) Item(ITEM_POKE_BALL);
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(enemy && infiltrator ? ABILITY_INFILTRATOR : ABILITY_NONE);
            Level(50); Attack(enemy && breaks ? 400 : 10); Defense(50); HP(1000); MaxHP(1000);
            if (!enemy) Item(ITEM_POKE_BALL);
        }
    } WHEN {
        TURN { MOVE(target, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, move); }
    } THEN {
        EXPECT_EQ((u32)target->volatiles.substitute, !breaks || infiltrator);
        EXPECT_EQ(target->item, removed ? ITEM_NONE : ITEM_POKE_BALL);
        EXPECT_EQ(user->item, removed && move == MOVE_THIEF ? ITEM_POKE_BALL : ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished command Infiltrator Knock Off reserves the targets healing berry before HP damage")
{
    bool32 enemy = FALSE;
    PARAMETRIZE { enemy = FALSE; }
    PARAMETRIZE { enemy = TRUE; }
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(!enemy ? ABILITY_INFILTRATOR : ABILITY_NONE); Level(50); Attack(400); Defense(100); HP(1000); MaxHP(1000);
            if (enemy) Item(ITEM_SITRUS_BERRY);
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(enemy ? ABILITY_INFILTRATOR : ABILITY_NONE); Level(50); Attack(400); Defense(100); HP(1000); MaxHP(1000);
            if (!enemy) Item(ITEM_SITRUS_BERRY);
        }
    } WHEN {
        TURN { MOVE(target, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, MOVE_KNOCK_OFF); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_KNOCK_OFF, user);
        HP_BAR(target, captureDamage: &damage);
        if (B_POLISHED_MOVE_BALANCE) NOT ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, target);
    } THEN {
        EXPECT_EQ((u32)target->volatiles.substitute, TRUE);
        EXPECT_GT(damage, 250);
        EXPECT_EQ(target->item, ITEM_NONE);
        if (B_POLISHED_MOVE_BALANCE) {
            EXPECT_EQ(target->hp, 750 - damage);
            EXPECT_EQ(GetBattlerPartyState(enemy ? B_BATTLER_0 : B_BATTLER_1)->usedHeldItem, ITEM_NONE);
        }
    }
}

SINGLE_BATTLE_TEST("Polished command item attacks preserve items after a miss or Protect")
{
    enum Move move = MOVE_THIEF;
    bool32 protect = FALSE;
    for (u32 kind = 0; kind < 3; kind++)
        for (u32 blocked = 0; blocked < 2; blocked++)
            PARAMETRIZE { move = kind == 0 ? MOVE_THIEF : kind == 1 ? MOVE_KNOCK_OFF : MOVE_BUG_BITE; protect = blocked; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_HUSTLE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_SITRUS_BERRY); }
    } WHEN {
        TURN { MOVE(player, move, hit: protect); if (protect) MOVE(opponent, MOVE_PROTECT); }
    } THEN {
        EXPECT_EQ(player->item, ITEM_NONE);
        EXPECT_EQ(opponent->item, ITEM_SITRUS_BERRY);
    }
}
