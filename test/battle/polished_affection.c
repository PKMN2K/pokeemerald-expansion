#include "global.h"
#include "test/battle.h"
#include "battle_util.h"
#include "event_data.h"
#include "constants/flags.h"

SINGLE_BATTLE_TEST("Polished affection reads both true parties at every friendship threshold")
{
    static const u32 values[] = {0, 179, 180, 219, 220, 254, 255};
    u32 friendship = 0;
    bool32 enabled = FALSE;
    for (u32 setting = 0; setting < 2; setting++)
        for (u32 value = 0; value < ARRAY_COUNT(values); value++)
            PARAMETRIZE { enabled = setting; friendship = values[value]; }
    GIVEN {
        if (enabled) FLAG_SET(FLAG_POLISHED_AFFECTION);
        else FlagClear(FLAG_POLISHED_AFFECTION);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Friendship(friendship); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Friendship(friendship); }
    } WHEN { TURN {} }
    THEN {
        u32 expected = friendship == 255 ? 3 : friendship >= 220 ? 2 : friendship >= 180 ? 1 : 0;
        if (!B_POLISHED_MOVE_BALANCE || !enabled) expected = 0;
        // Distinguish the party attribute from a stale/copied active value.
        player->friendship = opponent->friendship = friendship == 255 ? 0 : 255;
        EXPECT_EQ(GetPolishedAffectionLevel(GetBattlerAtPosition(B_POSITION_PLAYER_LEFT)), expected);
        EXPECT_EQ(GetPolishedAffectionLevel(GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT)), expected);
        if (B_POLISHED_MOVE_BALANCE) {
            EXPECT_EQ(GetBattlerAffectionHearts(GetBattlerAtPosition(B_POSITION_PLAYER_LEFT)), AFFECTION_NO_HEARTS);
            EXPECT_EQ(GetBattlerAffectionHearts(GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT)), AFFECTION_NO_HEARTS);
        }
    }
}

SINGLE_BATTLE_TEST("Polished affection excludes actual link and frontier battles")
{
    GIVEN {
        FLAG_SET(FLAG_POLISHED_AFFECTION);
        PLAYER(SPECIES_WOBBUFFET) { Friendship(255); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Friendship(255); Ability(ABILITY_NONE); }
    } WHEN { TURN {} }
    THEN {
        u32 flags = gBattleTypeFlags;
        for (u32 side = 0; side < 2; side++) {
            enum BattlerId battler = GetBattlerAtPosition(side ? B_POSITION_OPPONENT_LEFT : B_POSITION_PLAYER_LEFT);
            EXPECT_EQ(GetPolishedAffectionLevel(battler), B_POLISHED_MOVE_BALANCE ? 3 : 0);
            gBattleTypeFlags = flags | BATTLE_TYPE_LINK;
            EXPECT_EQ(GetPolishedAffectionLevel(battler), 0);
            gBattleTypeFlags = flags | BATTLE_TYPE_FRONTIER;
            EXPECT_EQ(GetPolishedAffectionLevel(battler), 0);
            gBattleTypeFlags = flags;
        }
    }
}

SINGLE_BATTLE_TEST("Polished affection leaves either active optional gimmick native")
{
    u32 tera = 0;
    PARAMETRIZE { tera = 0; }
    PARAMETRIZE { tera = 1; }
    PARAMETRIZE { tera = 2; }
    GIVEN {
        FLAG_SET(FLAG_POLISHED_AFFECTION);
        PLAYER(SPECIES_WOBBUFFET) { Friendship(255); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Friendship(255); Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH, gimmick: tera == 1 ? GIMMICK_TERA : GIMMICK_NONE); MOVE(opponent, MOVE_SPLASH, gimmick: tera == 2 ? GIMMICK_TERA : GIMMICK_NONE); }
    } THEN {
        EXPECT_EQ(GetPolishedAffectionLevel(GetBattlerAtPosition(B_POSITION_PLAYER_LEFT)), B_POLISHED_MOVE_BALANCE && tera == 0 ? 3 : 0);
        EXPECT_EQ(GetPolishedAffectionLevel(GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT)), B_POLISHED_MOVE_BALANCE && tera == 0 ? 3 : 0);
    }
}

DOUBLE_BATTLE_TEST("Polished affection does not introduce source effects into optional doubles")
{
    GIVEN {
        FLAG_SET(FLAG_POLISHED_AFFECTION);
        PLAYER(SPECIES_WOBBUFFET) { Friendship(255); Ability(ABILITY_NONE); }
        PLAYER(SPECIES_WOBBUFFET) { Friendship(255); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Friendship(255); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Friendship(255); Ability(ABILITY_NONE); }
    } WHEN { TURN {} }
    THEN {
        for (u32 battler = 0; battler < MAX_BATTLERS_COUNT; battler++)
            EXPECT_EQ(GetPolishedAffectionLevel(battler), 0);
    }
}

SINGLE_BATTLE_TEST("Polished affection doubles the critical numerator independently at stages zero one and two")
{
    struct BattlePokemon *user = player;
    u32 stage = 0, friendship = 254;
    bool32 enabled = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 crit = 0; crit < 3; crit++)
            for (u32 setting = 0; setting < 2; setting++)
                for (u32 high = 0; high < 2; high++)
                    PARAMETRIZE { user = side ? opponent : player; stage = crit; enabled = setting; friendship = 254 + high; }
    u32 numerator = stage == 0 ? 1 : stage == 1 ? 3 : 12;
    if (B_POLISHED_MOVE_BALANCE && enabled && friendship == 255) numerator *= 2;
    PASSES_RANDOMLY(numerator, 24, RNG_CRITICAL_HIT);
    GIVEN {
        WITH_CONFIG(B_CRIT_CHANCE, GEN_9);
        if (enabled) FLAG_SET(FLAG_POLISHED_AFFECTION);
        else FlagClear(FLAG_POLISHED_AFFECTION);
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player && stage == 2 ? ABILITY_SUPER_LUCK : ABILITY_NONE); Item(user == player && stage == 2 ? ITEM_SCOPE_LENS : ITEM_NONE); Friendship(user == player ? friendship : 0); Attack(10); MaxHP(1000); HP(1000); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent && stage == 2 ? ABILITY_SUPER_LUCK : ABILITY_NONE); Item(user == opponent && stage == 2 ? ITEM_SCOPE_LENS : ITEM_NONE); Friendship(user == opponent ? friendship : 0); Attack(10); MaxHP(1000); HP(1000); }
    } WHEN { TURN { MOVE(user, stage == 1 ? MOVE_SLASH : MOVE_TACKLE); } }
    SCENE { MESSAGE("A critical hit!"); }
}

SINGLE_BATTLE_TEST("Polished affection after Transform retains the transforming party friendship")
{
    struct BattlePokemon *user = player;
    bool32 affectionate = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 high = 0; high < 2; high++)
            PARAMETRIZE { user = side ? opponent : player; affectionate = high; }
    PASSES_RANDOMLY(B_POLISHED_MOVE_BALANCE && affectionate ? 2 : 1, 24, RNG_CRITICAL_HIT);
    GIVEN {
        FLAG_SET(FLAG_POLISHED_AFFECTION);
        WITH_CONFIG(B_CRIT_CHANCE, GEN_9);
        PLAYER(user == player ? SPECIES_DITTO : SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Friendship((user == player) == affectionate ? 255 : 0); Attack(10); MaxHP(1000); HP(1000); Moves(MOVE_TACKLE, MOVE_TRANSFORM, MOVE_SPLASH); }
        OPPONENT(user == opponent ? SPECIES_DITTO : SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Friendship((user == opponent) == affectionate ? 255 : 0); Attack(10); MaxHP(1000); HP(1000); Moves(MOVE_TACKLE, MOVE_TRANSFORM, MOVE_SPLASH); }
    } WHEN {
        TURN { MOVE(user, MOVE_TRANSFORM); }
        TURN { MOVE(user, MOVE_TACKLE); }
    } SCENE { ANIMATION(ANIM_TYPE_MOVE, MOVE_TRANSFORM, user); MESSAGE("A critical hit!"); }
}

SINGLE_BATTLE_TEST("Polished affection Future Sight reads its original user party slot after replacement")
{
    struct BattlePokemon *user = player, *target = opponent;
    bool32 affectionate = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 high = 0; high < 2; high++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; affectionate = high; }
    PASSES_RANDOMLY(B_POLISHED_MOVE_BALANCE && affectionate ? 2 : 1, 24, RNG_CRITICAL_HIT);
    GIVEN {
        FLAG_SET(FLAG_POLISHED_AFFECTION);
        WITH_CONFIG(B_CRIT_CHANCE, GEN_9);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Friendship(user == player && affectionate ? 255 : 0); MaxHP(1000); HP(1000); }
        PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Friendship(user == player && !affectionate ? 255 : 0); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Friendship(user == opponent && affectionate ? 255 : 0); MaxHP(1000); HP(1000); }
        OPPONENT(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Friendship(user == opponent && !affectionate ? 255 : 0); }
    } WHEN {
        TURN { MOVE(user, MOVE_FUTURE_SIGHT); }
        TURN { SWITCH(user, 1); }
        TURN {}
    } SCENE {
        if (target == player) MESSAGE("Wobbuffet took the Future Sight attack!");
        else MESSAGE("The opposing Wobbuffet took the Future Sight attack!");
        MESSAGE("A critical hit!");
    }
}

SINGLE_BATTLE_TEST("Polished affection independently evades one of ten otherwise accurate hits on either side")
{
    struct BattlePokemon *target = opponent, *user = player;
    PARAMETRIZE { target = opponent; user = player; }
    PARAMETRIZE { target = player; user = opponent; }
    if (B_POLISHED_MOVE_BALANCE) PASSES_RANDOMLY(9, 10, RNG_POLISHED_AFFECTION_EVASION);
    GIVEN {
        FLAG_SET(FLAG_POLISHED_AFFECTION);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Friendship(target == player ? 255 : 0); MaxHP(1000); HP(1000); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Friendship(target == opponent ? 255 : 0); MaxHP(1000); HP(1000); }
    } WHEN { TURN { MOVE(user, MOVE_TACKLE); } }
    SCENE { HP_BAR(target); }
}

SINGLE_BATTLE_TEST("Polished affection evasion requires its enabled maximum level and still triggers Blunder Policy")
{
    u32 friendship = 254, roll = 0;
    bool32 enabled = FALSE;
    for (u32 setting = 0; setting < 2; setting++)
        for (u32 high = 0; high < 2; high++)
            for (u32 random = 0; random < 3; random++)
                PARAMETRIZE { enabled = setting; friendship = 254 + high; roll = random == 2 ? 9 : random; }
    bool32 avoided = B_POLISHED_MOVE_BALANCE && enabled && friendship == 255 && roll == 0;
    GIVEN {
        if (enabled) FLAG_SET(FLAG_POLISHED_AFFECTION);
        else FlagClear(FLAG_POLISHED_AFFECTION);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Friendship(0); Item(ITEM_BLUNDER_POLICY); Speed(200); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Friendship(friendship); Speed(100); MaxHP(1000); HP(1000); }
    } WHEN { TURN { MOVE(player, MOVE_TACKLE, WITH_RNG(RNG_POLISHED_AFFECTION_EVASION, roll)); } }
    SCENE {
        if (avoided) { NOT HP_BAR(opponent); ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_STATS_CHANGE, player); }
        else HP_BAR(opponent);
    } THEN {
        EXPECT_EQ(player->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + (avoided ? 2 : 0));
        EXPECT_EQ(player->item, avoided ? ITEM_NONE : ITEM_BLUNDER_POLICY);
    }
}

SINGLE_BATTLE_TEST("Polished affection passing evasion still performs the ordinary accuracy roll")
{
    PASSES_RANDOMLY(1, 2, RNG_ACCURACY);
    GIVEN {
        FLAG_SET(FLAG_POLISHED_AFFECTION);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Friendship(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Friendship(255); MaxHP(1000); HP(1000); }
    } WHEN { TURN { MOVE(player, MOVE_ZAP_CANNON, WITH_RNG(RNG_POLISHED_AFFECTION_EVASION, 1)); } }
    SCENE { HP_BAR(opponent); }
}

SINGLE_BATTLE_TEST("Polished affection is bypassed by perfect accuracy rules before its independent roll")
{
    struct BattlePokemon *user = player, *target = opponent;
    u32 kind = 0;
    static const enum Move moves[] = {MOVE_SWIFT, MOVE_TACKLE, MOVE_ZAP_CANNON, MOVE_THUNDER, MOVE_BLIZZARD, MOVE_TOXIC, MOVE_STOMP};
    for (u32 side = 0; side < 2; side++)
        for (u32 bypass = 0; bypass < ARRAY_COUNT(moves); bypass++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; kind = bypass; }
    GIVEN {
        FLAG_SET(FLAG_POLISHED_AFFECTION);
        PLAYER(user == player && kind == 5 ? SPECIES_EKANS : SPECIES_WOBBUFFET) { Ability(user == player && kind == 1 ? ABILITY_NO_GUARD : ABILITY_NONE); Friendship(target == player ? 255 : 0); MaxHP(1000); HP(1000); }
        OPPONENT(user == opponent && kind == 5 ? SPECIES_EKANS : SPECIES_WOBBUFFET) { Ability(user == opponent && kind == 1 ? ABILITY_NO_GUARD : ABILITY_NONE); Friendship(target == opponent ? 255 : 0); MaxHP(1000); HP(1000); }
    } WHEN {
        if (kind == 2) TURN { MOVE(user, MOVE_LOCK_ON); }
        if (kind == 3) TURN { MOVE(user, MOVE_RAIN_DANCE); }
        if (kind == 4) TURN { MOVE(user, MOVE_HAIL); }
        if (kind == 6) TURN { MOVE(target, MOVE_MINIMIZE); }
        TURN { MOVE(user, moves[kind], WITH_RNG(RNG_POLISHED_AFFECTION_EVASION, 0)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, moves[kind], user);
        if (kind == 5) STATUS_ICON(target, badPoison: TRUE);
        else HP_BAR(target);
    }
}

SINGLE_BATTLE_TEST("Polished affection cures each primary status with one of five outcomes on either side")
{
    static const u32 statuses[] = {STATUS1_POISON, STATUS1_TOXIC_POISON, STATUS1_BURN, STATUS1_PARALYSIS, STATUS1_FREEZE, STATUS1_SLEEP_TURN(3)};
    struct BattlePokemon *holder = player;
    u32 status = STATUS1_POISON;
    for (u32 side = 0; side < 2; side++)
        for (u32 ailment = 0; ailment < ARRAY_COUNT(statuses); ailment++)
            PARAMETRIZE { holder = side ? opponent : player; status = statuses[ailment]; }
    if (B_POLISHED_MOVE_BALANCE) PASSES_RANDOMLY(1, 5, RNG_POLISHED_AFFECTION_CURE);
    GIVEN {
        FLAG_SET(FLAG_POLISHED_AFFECTION);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Friendship(holder == player ? 220 : 0); Status1(holder == player ? status : STATUS1_NONE); HP(160); MaxHP(200); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Friendship(holder == opponent ? 220 : 0); Status1(holder == opponent ? status : STATUS1_NONE); HP(160); MaxHP(200); }
    } WHEN {
        // A bag action avoids independent wake/thaw/paralysis action checks.
        TURN { USE_ITEM(holder, ITEM_POTION, partyIndex: 0); }
    } SCENE {
        if (B_POLISHED_MOVE_BALANCE) STATUS_ICON(holder, none: TRUE);
        else NOT STATUS_ICON(holder, none: TRUE);
    } THEN {
        if (B_POLISHED_MOVE_BALANCE) {
            EXPECT_EQ(holder->status1, STATUS1_NONE);
            EXPECT_EQ(GetMonData(GetBattlerMon(holder - gBattleMons), MON_DATA_STATUS), STATUS1_NONE);
        } else EXPECT(holder->status1 & status);
    }
}

SINGLE_BATTLE_TEST("Polished affection cure starts at friendship two hundred twenty and ignores ability suppression")
{
    u32 friendship = 219;
    enum Ability ability = ABILITY_NONE;
    bool32 enabled = FALSE;
    for (u32 setting = 0; setting < 2; setting++)
        for (u32 value = 0; value < 3; value++)
            for (u32 gas = 0; gas < 2; gas++)
                PARAMETRIZE { enabled = setting; friendship = value == 2 ? 255 : 219 + value; ability = gas ? ABILITY_NEUTRALIZING_GAS : ABILITY_NONE; }
    bool32 eligible = B_POLISHED_MOVE_BALANCE && enabled && friendship >= 220;
    if (eligible) PASSES_RANDOMLY(1, 5, RNG_POLISHED_AFFECTION_CURE);
    GIVEN {
        if (enabled) FLAG_SET(FLAG_POLISHED_AFFECTION);
        else FlagClear(FLAG_POLISHED_AFFECTION);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Friendship(friendship); Status1(STATUS1_POISON); }
        OPPONENT(SPECIES_WEEZING) { Ability(ability); Friendship(0); }
    } WHEN { TURN { MOVE(player, MOVE_SPLASH); } }
    SCENE {
        if (eligible) STATUS_ICON(player, none: TRUE);
        else NOT STATUS_ICON(player, none: TRUE);
    } THEN { EXPECT_EQ(player->status1, eligible ? STATUS1_NONE : STATUS1_POISON); }
}

SINGLE_BATTLE_TEST("Polished affection self cure precedes an already scheduled Future Sight")
{
    struct BattlePokemon *holder = player, *other = opponent;
    PARAMETRIZE { holder = player; other = opponent; }
    PARAMETRIZE { holder = opponent; other = player; }
    if (B_POLISHED_MOVE_BALANCE) PASSES_RANDOMLY(1, 5, RNG_POLISHED_AFFECTION_CURE);
    GIVEN {
        FLAG_SET(FLAG_POLISHED_AFFECTION);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Friendship(holder == player ? 220 : 0); MaxHP(1000); HP(1000); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Friendship(holder == opponent ? 220 : 0); MaxHP(1000); HP(1000); }
    } WHEN {
        TURN { MOVE(other, MOVE_FUTURE_SIGHT); }
        TURN {}
        TURN { MOVE(other, MOVE_TOXIC); }
    } SCENE {
        STATUS_ICON(holder, badPoison: TRUE);
        if (B_POLISHED_MOVE_BALANCE) STATUS_ICON(holder, none: TRUE);
        else NOT STATUS_ICON(holder, none: TRUE);
        if (holder == player) MESSAGE("Wobbuffet took the Future Sight attack!");
        else MESSAGE("The opposing Wobbuffet took the Future Sight attack!");
        HP_BAR(holder);
    } THEN { EXPECT_EQ(holder->status1 & STATUS1_ANY, B_POLISHED_MOVE_BALANCE ? STATUS1_NONE : STATUS1_TOXIC_POISON); }
}

SINGLE_BATTLE_TEST("Polished affection endurance uses level plus one of twenty outcomes before Focus Sash")
{
    struct BattlePokemon *holder = opponent, *user = player;
    u32 level = 1;
    for (u32 side = 0; side < 2; side++)
        for (u32 affection = 1; affection <= 3; affection++)
            PARAMETRIZE { holder = side ? player : opponent; user = side ? opponent : player; level = affection; }
    if (B_POLISHED_MOVE_BALANCE) PASSES_RANDOMLY(level + 1, 20, RNG_POLISHED_AFFECTION_ENDURE);
    u32 friendship = level == 1 ? 180 : level == 2 ? 220 : 255;
    GIVEN {
        FLAG_SET(FLAG_POLISHED_AFFECTION);
        // Sash keeps every probability trial alive. Its retention distinguishes
        // affection from the fallback; the separate strict-cutoff test covers
        // injured HP without a survival item.
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Friendship(holder == player ? friendship : 0); Level(100); HP(50); MaxHP(50); Speed(user == player ? 200 : 100); Item(holder == player ? ITEM_FOCUS_SASH : ITEM_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Friendship(holder == opponent ? friendship : 0); Level(100); HP(50); MaxHP(50); Speed(user == opponent ? 200 : 100); Item(holder == opponent ? ITEM_FOCUS_SASH : ITEM_NONE); }
    } WHEN { TURN { MOVE(user, MOVE_SEISMIC_TOSS); } }
    SCENE {
        if (B_POLISHED_MOVE_BALANCE)
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_AFFECTION_HANGED_ON, holder);
    } THEN {
        EXPECT_EQ(holder->hp, 1);
        EXPECT_EQ(holder->item, B_POLISHED_MOVE_BALANCE ? ITEM_FOCUS_SASH : ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished affection endurance has exact strict cutoffs and stays disabled without its option")
{
    u32 level = 1, roll = 0;
    bool32 enabled = FALSE;
    for (u32 setting = 0; setting < 2; setting++)
        for (u32 affection = 1; affection <= 3; affection++)
            for (u32 boundary = 0; boundary < 4; boundary++)
                PARAMETRIZE { enabled = setting; level = affection; roll = boundary == 0 ? 0 : boundary == 1 ? affection : boundary == 2 ? affection + 1 : 19; }
    GIVEN {
        if (enabled) FLAG_SET(FLAG_POLISHED_AFFECTION);
        else FlagClear(FLAG_POLISHED_AFFECTION);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Friendship(0); Level(100); Speed(200); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Friendship(level == 1 ? 180 : level == 2 ? 220 : 255); HP(50); MaxHP(100); Speed(100); }
    } WHEN { TURN { MOVE(player, MOVE_SEISMIC_TOSS, WITH_RNG(RNG_POLISHED_AFFECTION_ENDURE, roll)); } }
    THEN { EXPECT_EQ(opponent->hp, B_POLISHED_MOVE_BALANCE && enabled && roll < level + 1 ? 1 : 0); }
}

SINGLE_BATTLE_TEST("Polished affection endurance respects Endure and False Swipe before items and Sturdy")
{
    u32 mode = 0;
    for (u32 branch = 0; branch < 6; branch++) PARAMETRIZE { mode = branch; }
    GIVEN {
        FLAG_SET(FLAG_POLISHED_AFFECTION);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Friendship(0); Level(100); Attack(1000); Speed(200); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(mode == 2 || mode == 4 || mode == 5 ? ABILITY_STURDY : ABILITY_NONE); Friendship(255); HP(50); MaxHP(50); Defense(10); Speed(100); Item(mode == 4 ? ITEM_NONE : ITEM_FOCUS_SASH); }
    } WHEN {
        TURN {
            if (mode == 0) MOVE(opponent, MOVE_ENDURE);
            MOVE(player, mode == 5 ? MOVE_FALSE_SWIPE : MOVE_SEISMIC_TOSS, WITH_RNG(RNG_POLISHED_AFFECTION_ENDURE, mode == 3 || mode == 4 ? 19 : 0));
        }
    } SCENE {
        if ((mode == 2 && !B_POLISHED_MOVE_BALANCE) || mode == 4) ABILITY_POPUP(opponent, ABILITY_STURDY);
        else NOT ABILITY_POPUP(opponent, ABILITY_STURDY);
    } THEN {
        EXPECT_EQ(opponent->hp, 1);
        EXPECT_EQ(opponent->item, mode == 4 || mode == 3 || (mode == 1 && !B_POLISHED_MOVE_BALANCE) ? ITEM_NONE : ITEM_FOCUS_SASH);
    }
}

SINGLE_BATTLE_TEST("Polished affection endurance rolls again for each lethal multihit strike")
{
    u32 laterRoll = 0;
    PARAMETRIZE { laterRoll = 0; }
    PARAMETRIZE { laterRoll = 19; }
    GIVEN {
        FLAG_SET(FLAG_POLISHED_AFFECTION);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Friendship(0); Attack(1000); Speed(200); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Friendship(255); Defense(10); HP(50); MaxHP(50); Speed(100); Item(B_POLISHED_MOVE_BALANCE || laterRoll == 0 ? ITEM_NONE : ITEM_FOCUS_SASH); }
    } WHEN {
        TURN {
            if (!B_POLISHED_MOVE_BALANCE && laterRoll == 0) MOVE(opponent, MOVE_ENDURE);
            MOVE(player, MOVE_DOUBLE_KICK, WITH_RNG(RNG_POLISHED_AFFECTION_ENDURE, 0), WITH_RNG_AFTER_FIRST_HIT(RNG_POLISHED_AFFECTION_ENDURE, laterRoll));
        }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_DOUBLE_KICK, player);
        HP_BAR(opponent, hp: 1);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_DOUBLE_KICK, player);
        if (laterRoll != 0) HP_BAR(opponent, hp: 0);
    } THEN { EXPECT_EQ(opponent->hp, laterRoll == 0 ? 1 : 0); }
}
