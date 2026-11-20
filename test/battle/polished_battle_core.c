#include "global.h"
#include "test/battle.h"
#include "battle_main.h"
#include "battle_util.h"
#include "event_data.h"
#include "polished_battle_core.h"
#include "polished_confusion_vectors.h"

TEST("Polished core confusion matches 8112 source arithmetic vectors")
{
    for (u32 n = 0; n < ARRAY_COUNT(sPolishedConfusionVectors); n++)
    {
        const typeof(*sPolishedConfusionVectors) *v = &sPolishedConfusionVectors[n];
        EXPECT_EQ(PolishedConfusionDamage(v->level, v->attack, v->defense, v->attackStage, v->defenseStage), v->damage);
    }
}

SINGLE_BATTLE_TEST("Polished core confusion uses raw stats and simultaneous byte reduction on both sides")
{
    struct BattlePokemon *holder = player, *other = opponent;
    u32 attack = 255, defense = 256;
    static const u16 pairs[][2] = {{255,256},{256,255},{511,512},{1,1023},{1023,1},{101,100}};
    s16 damage;
    for (u32 side = 0; side < 2; side++)
        for (u32 pair = 0; pair < ARRAY_COUNT(pairs); pair++)
            PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; attack = pairs[pair][0]; defense = pairs[pair][1]; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(50); Ability(ABILITY_NONE); Attack(attack); Defense(defense); Speed(holder == player ? 200 : 100); MaxHP(30000); HP(30000); }
        OPPONENT(SPECIES_WOBBUFFET) { Level(50); Ability(ABILITY_NONE); Attack(attack); Defense(defense); Speed(holder == opponent ? 200 : 100); MaxHP(30000); HP(30000); }
    } WHEN {
        TURN { MOVE(holder, MOVE_SPLASH); MOVE(other, MOVE_CONFUSE_RAY); }
        TURN { MOVE(holder, MOVE_TACKLE, WITH_RNG(RNG_CONFUSION, TRUE)); }
    } SCENE { MESSAGE("It hurt itself in its confusion!"); HP_BAR(holder, captureDamage: &damage); }
    THEN {
        u32 expected = B_POLISHED_MOVE_BALANCE ? PolishedConfusionDamage(50, attack, defense, 6, 6) : 22 * 40 * attack / defense / 50 + 2;
        EXPECT_EQ(damage, min(expected, 10000)); // The controller caps the displayed HP-bar delta.
        EXPECT_EQ(holder->hp, 30000 - expected);
    }
}

SINGLE_BATTLE_TEST("Polished core confusion scales the numerator after compression before dividing by Defense")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Attack(101); Defense(73); Level(50); MaxHP(30000); HP(30000); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN { TURN {} }
    THEN {
        struct DamageContext ctx = {0};
        ctx.battlerAtk = ctx.battlerDef = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);
        ctx.move = MOVE_NONE; ctx.moveType = TYPE_MYSTERY; ctx.fixedBasePower = 40; ctx.isSelfInflicted = TRUE;
        for (u32 a = 0; a <= 12; a++) for (u32 d = 0; d <= 12; d++) {
            player->statStages[STAT_ATK] = a; player->statStages[STAT_DEF] = d;
            u32 atk = a >= 6 ? 101 * (a - 4) / 2 : 101 * 2 / (8 - a);
            u32 def = d >= 6 ? 73 * (d - 4) / 2 : 73 * 2 / (8 - d);
            EXPECT_EQ(CalculateMoveDamage(&ctx), B_POLISHED_MOVE_BALANCE ? PolishedConfusionDamage(50,101,73,a,d) : 22 * 40 * atk / def / 50 + 2);
        }
    }
}

SINGLE_BATTLE_TEST("Polished core confusion ignores attack boosts from abilities items burn and screens")
{
    enum Ability ability = ABILITY_NONE;
    enum Item item = ITEM_NONE;
    static const enum Ability abilities[] = {ABILITY_NONE, ABILITY_HUGE_POWER, ABILITY_GUTS, ABILITY_MARVEL_SCALE, ABILITY_UNAWARE};
    static const enum Item items[] = {ITEM_NONE, ITEM_CHOICE_BAND, ITEM_LIFE_ORB, ITEM_MUSCLE_BAND, ITEM_BLACK_BELT};
    for (u32 k = 0; k < ARRAY_COUNT(abilities); k++) PARAMETRIZE { ability = abilities[k]; item = items[k]; }
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(50); Ability(ability); Item(item); Attack(101); Defense(73); Speed(200); MaxHP(1000); HP(1000); Status1(STATUS1_BURN); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); }
    } WHEN {
        TURN { MOVE(player, MOVE_REFLECT); MOVE(opponent, MOVE_CONFUSE_RAY); }
        TURN { MOVE(player, MOVE_REFLECT, WITH_RNG(RNG_CONFUSION, TRUE)); }
    } SCENE { MESSAGE("It hurt itself in its confusion!"); HP_BAR(player, captureDamage: &damage); }
    THEN { EXPECT_EQ(damage, PolishedConfusionDamage(50,101,73,6,6)); }
}

SINGLE_BATTLE_TEST("Polished core source confusion bypasses Sturdy Endure and survival item damage clamps")
{
    u32 mode = 0;
    for (u32 m = 0; m < 4; m++) PARAMETRIZE { mode = m; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); HP(1); MaxHP(1); Attack(100); Defense(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN { TURN {} }
    THEN {
        struct DamageContext ctx = {0};
        ctx.battlerAtk = ctx.battlerDef = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);
        ctx.move = MOVE_NONE; ctx.moveType = TYPE_MYSTERY; ctx.fixedBasePower = 40; ctx.isSelfInflicted = TRUE;
        if (mode == 0) player->volatiles.endured = TRUE;
        if (mode == 1) ctx.abilities[0] = ABILITY_STURDY;
        if (mode == 2) ctx.holdEffects[0] = HOLD_EFFECT_FOCUS_SASH;
        if (mode == 3) ctx.holdEffects[0] = HOLD_EFFECT_FOCUS_BAND;
        if (B_POLISHED_MOVE_BALANCE) EXPECT_GT(CalculateMoveDamage(&ctx), 1);
        else if (mode < 3) EXPECT_EQ(CalculateMoveDamage(&ctx), 0);
    }
}

WILD_BATTLE_TEST("Polished core default affection is disabled without changing friendship")
{
    u32 friendship = 255;
    PARAMETRIZE { friendship = 180; }
    PARAMETRIZE { friendship = 220; }
    PARAMETRIZE { friendship = 255; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Friendship(friendship); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Friendship(255); Ability(ABILITY_NONE); }
    } WHEN { TURN {} }
    THEN {
        EXPECT_EQ(player->friendship, friendship);
        EXPECT_EQ(GetBattlerAffectionHearts(GetBattlerAtPosition(B_POSITION_PLAYER_LEFT)), B_POLISHED_MOVE_BALANCE ? AFFECTION_NO_HEARTS : friendship == 255 ? AFFECTION_FIVE_HEARTS : friendship == 220 ? AFFECTION_FOUR_HEARTS : AFFECTION_THREE_HEARTS);
        EXPECT_EQ(GetBattlerAffectionHearts(GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT)), AFFECTION_NO_HEARTS);
    }
}

WILD_BATTLE_TEST("Polished core default affection keeps high friendship critical odds at the source base rate")
{
    PASSES_RANDOMLY(1, B_POLISHED_MOVE_BALANCE ? 24 : 2, RNG_CRITICAL_HIT);
    GIVEN {
        WITH_CONFIG(B_CRIT_CHANCE, GEN_9);
        PLAYER(SPECIES_WOBBUFFET) { Friendship(255); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN { TURN { MOVE(player, MOVE_TACKLE); } }
    SCENE { MESSAGE("A critical hit!"); }
}

SINGLE_BATTLE_TEST("Polished core critical stages use source odds independently of native generation")
{
    u32 stage = 0, generation = GEN_3;
    struct BattlePokemon *user = player;
    for (u32 side = 0; side < 2; side++) for (u32 g = 0; g < 3; g++) for (u32 s = 0; s < 4; s++)
        PARAMETRIZE { user = side ? opponent : player; generation = g == 0 ? GEN_3 : g == 1 ? GEN_6 : GEN_9; stage = s; }
    u32 divisor = stage == 0 ? (B_POLISHED_MOVE_BALANCE || generation == GEN_9 ? 24 : 16) : stage == 1 ? 8 : stage == 2 ? (B_POLISHED_MOVE_BALANCE || generation >= GEN_6 ? 2 : 4) : (B_POLISHED_MOVE_BALANCE || generation >= GEN_6 ? 1 : 3);
    PASSES_RANDOMLY(1, divisor, RNG_CRITICAL_HIT);
    GIVEN {
        WITH_CONFIG(B_CRIT_CHANCE, generation);
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player && stage >= 2 ? ABILITY_SUPER_LUCK : ABILITY_NONE); Item(user == player && stage == 2 ? ITEM_SCOPE_LENS : ITEM_NONE); MaxHP(2000); HP(2000); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent && stage >= 2 ? ABILITY_SUPER_LUCK : ABILITY_NONE); Item(user == opponent && stage == 2 ? ITEM_SCOPE_LENS : ITEM_NONE); MaxHP(2000); HP(2000); }
    } WHEN {
        if (stage == 3) TURN { MOVE(user, MOVE_FOCUS_ENERGY); }
        TURN { MOVE(user, stage == 1 ? MOVE_SLASH : MOVE_TACKLE); }
    } SCENE { MESSAGE("A critical hit!"); }
}

SINGLE_BATTLE_TEST("Polished core all nine source high critical moves carry one stage on either side")
{
    static const enum Move moves[] = {MOVE_KARATE_CHOP, MOVE_NIGHT_SLASH, MOVE_RAZOR_LEAF, MOVE_CRABHAMMER, MOVE_SLASH, MOVE_AEROBLAST, MOVE_CROSS_CHOP, MOVE_SHADOW_CLAW, MOVE_STONE_EDGE};
    enum Move move = MOVE_SLASH;
    struct BattlePokemon *user = player;
    for (u32 side = 0; side < 2; side++) for (u32 m = 0; m < ARRAY_COUNT(moves); m++) PARAMETRIZE { user = side ? opponent : player; move = moves[m]; }
    PASSES_RANDOMLY(1, 8, RNG_CRITICAL_HIT);
    GIVEN {
        WITH_CONFIG(B_CRIT_CHANCE, GEN_9);
        PLAYER(SPECIES_MEW) { Ability(ABILITY_NONE); MaxHP(2000); HP(2000); }
        OPPONENT(SPECIES_MEW) { Ability(ABILITY_NONE); MaxHP(2000); HP(2000); }
    } WHEN { TURN { MOVE(user, move, hit: TRUE); } }
    SCENE { MESSAGE("A critical hit!"); }
}

SINGLE_BATTLE_TEST("Polished core critical armor blocks a guaranteed source critical unless bypassed")
{
    enum Ability armor = ABILITY_BATTLE_ARMOR;
    bool32 breaker = FALSE;
    for (u32 a = 0; a < 2; a++) for (u32 b = 0; b < 2; b++) PARAMETRIZE { armor = a ? ABILITY_SHELL_ARMOR : ABILITY_BATTLE_ARMOR; breaker = b; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(breaker ? ABILITY_MOLD_BREAKER : ABILITY_NONE); Item(ITEM_SCOPE_LENS); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(armor); }
    } WHEN { TURN { MOVE(player, MOVE_FOCUS_ENERGY); } TURN { MOVE(player, MOVE_TACKLE); } }
    SCENE { if (breaker) MESSAGE("A critical hit!"); else NOT MESSAGE("A critical hit!"); }
}

SINGLE_BATTLE_TEST("Polished core Leek follows source species and form eligibility")
{
    enum Species species = SPECIES_FARFETCHD;
    PARAMETRIZE { species = SPECIES_FARFETCHD; }
    PARAMETRIZE { species = SPECIES_FARFETCHD_GALAR; }
    PARAMETRIZE { species = SPECIES_SIRFETCHD; }
    PASSES_RANDOMLY(1, B_POLISHED_MOVE_BALANCE && species == SPECIES_FARFETCHD_GALAR ? 24 : 2, RNG_CRITICAL_HIT);
    GIVEN {
        WITH_CONFIG(B_CRIT_CHANCE, GEN_9);
        PLAYER(species) { Ability(ABILITY_NONE); Item(ITEM_LEEK); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN { TURN { MOVE(player, MOVE_TACKLE); } }
    SCENE { MESSAGE("A critical hit!"); }
}

SINGLE_BATTLE_TEST("Polished core zero-power Return cannot announce a critical hit")
{
    GIVEN {
        WITH_CONFIG(B_POLISHED_RETURN, TRUE);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_SUPER_LUCK); Item(ITEM_SCOPE_LENS); Friendship(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN { TURN { MOVE(player, MOVE_RETURN, WITH_RNG(RNG_CRITICAL_HIT, TRUE)); } }
    SCENE { if (B_POLISHED_MOVE_BALANCE) NOT MESSAGE("A critical hit!"); else MESSAGE("A critical hit!"); }
}

SINGLE_BATTLE_TEST("Polished core Speed applies paralysis before weather abilities and held items")
{
    static const u16 rawSpeeds[] = {1, 3, 7, 101, 255, 1023, 65535};
    static const enum Ability abilities[] = {ABILITY_NONE, ABILITY_SWIFT_SWIM, ABILITY_CHLOROPHYLL, ABILITY_SAND_RUSH, ABILITY_SLUSH_RUSH, ABILITY_QUICK_FEET, ABILITY_UNBURDEN};
    static const enum HoldEffect holds[] = {HOLD_EFFECT_NONE, HOLD_EFFECT_IRON_BALL, HOLD_EFFECT_CHOICE_SCARF, HOLD_EFFECT_UTILITY_UMBRELLA};
    static const u32 weathers[] = {B_WEATHER_NONE, B_WEATHER_RAIN, B_WEATHER_SUN, B_WEATHER_SANDSTORM, B_WEATHER_HAIL};
    GIVEN { PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); } OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); } }
    WHEN { TURN {} }
    THEN {
        for (u32 r = 0; r < ARRAY_COUNT(rawSpeeds); r++) for (u32 s = 0; s < 13; s++)
        for (u32 p = 0; p < 2; p++) for (u32 a = 0; a < ARRAY_COUNT(abilities); a++)
        for (u32 h = 0; h < ARRAY_COUNT(holds); h++) for (u32 w = 0; w < ARRAY_COUNT(weathers); w++) {
            player->speed = rawSpeeds[r]; player->statStages[STAT_SPEED] = s;
            player->status1 = p ? STATUS1_PARALYSIS : STATUS1_NONE;
            player->volatiles.unburdenActive = TRUE; gBattleWeather = weathers[w];
            u32 expected = s >= 6 ? rawSpeeds[r] * (s - 4) / 2 : rawSpeeds[r] * 2 / (8 - s);
            if (B_POLISHED_MOVE_BALANCE && p && abilities[a] != ABILITY_QUICK_FEET) expected /= 2;
            if ((a == 1 && w == 1 && h != 3) || (a == 2 && w == 2 && h != 3) || (a == 3 && w == 3) || (a == 4 && w == 4) || a == 6) expected *= 2;
            if (a == 5 && p) expected = expected * 3 / 2;
            if (h == 1) expected /= 2;
            if (h == 2) expected = expected * 3 / 2;
            if (!B_POLISHED_MOVE_BALANCE && p && a != 5) expected /= 2;
            if (B_POLISHED_MOVE_BALANCE) expected = min(expected, 65535);
            EXPECT_EQ(GetBattlerTotalSpeedStat(GetBattlerAtPosition(B_POSITION_PLAYER_LEFT), abilities[a], holds[h]), expected);
        }
    }
}

WILD_BATTLE_TEST("Polished core Speed ignores native badge boosts in ordinary battles")
{
    bool32 badge = FALSE;
    PARAMETRIZE { badge = FALSE; }
    PARAMETRIZE { badge = TRUE; }
    GIVEN {
        WITH_CONFIG(B_BADGE_BOOST, GEN_3);
        if (badge) FlagSet(B_FLAG_BADGE_BOOST_SPEED);
        else FlagClear(B_FLAG_BADGE_BOOST_SPEED);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(101); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); }
    } WHEN { TURN {} }
    THEN {
        EXPECT_EQ(GetBattlerTotalSpeedStat(GetBattlerAtPosition(B_POSITION_PLAYER_LEFT), ABILITY_NONE, HOLD_EFFECT_NONE), !B_POLISHED_MOVE_BALANCE && badge ? 111 : 101);
    }
}

SINGLE_BATTLE_TEST("Polished core fainted Pokemon use raw Speed without stages status ability or item")
{
    GIVEN { PLAYER(SPECIES_DITTO) { Ability(ABILITY_NONE); Speed(101); } OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); } }
    WHEN { TURN {} }
    THEN {
        player->hp = 0; player->status1 = STATUS1_PARALYSIS; player->statStages[STAT_SPEED] = 12;
        player->volatiles.unburdenActive = TRUE;
        EXPECT_EQ(GetBattlerTotalSpeedStat(GetBattlerAtPosition(B_POSITION_PLAYER_LEFT), ABILITY_UNBURDEN, HOLD_EFFECT_QUICK_POWDER), B_POLISHED_MOVE_BALANCE ? 101 : 808);
    }
}

SINGLE_BATTLE_TEST("Polished core Quick Powder checks copied species after Transform on either side")
{
    struct BattlePokemon *holder = player, *target = opponent;
    enum Species species = SPECIES_DITTO;
    for (u32 side = 0; side < 2; side++) for (u32 ditto = 0; ditto < 2; ditto++)
        PARAMETRIZE { holder = side ? opponent : player; target = side ? player : opponent; species = ditto ? SPECIES_DITTO : SPECIES_MEW; }
    GIVEN {
        PLAYER(holder == player ? SPECIES_DITTO : species) { Ability(ABILITY_NONE); Item(holder == player ? ITEM_QUICK_POWDER : ITEM_NONE); Speed(holder == player ? 200 : 101); Moves(MOVE_TRANSFORM, MOVE_SPLASH); }
        OPPONENT(holder == opponent ? SPECIES_DITTO : species) { Ability(ABILITY_NONE); Item(holder == opponent ? ITEM_QUICK_POWDER : ITEM_NONE); Speed(holder == opponent ? 200 : 101); Moves(MOVE_TRANSFORM, MOVE_SPLASH); }
    } WHEN { TURN { MOVE(holder, MOVE_TRANSFORM); MOVE(target, MOVE_SPLASH); } }
    THEN {
        EXPECT_EQ(holder->species, species);
        EXPECT(holder->volatiles.transformed);
        EXPECT_EQ(GetBattlerTotalSpeedStat(holder == player ? GetBattlerAtPosition(B_POSITION_PLAYER_LEFT) : GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT), ABILITY_NONE, HOLD_EFFECT_QUICK_POWDER), B_POLISHED_MOVE_BALANCE && species == SPECIES_DITTO ? 202 : 101);
    }
}

SINGLE_BATTLE_TEST("Polished core odd paralysis weather and Quick Powder rounding changes live action order")
{
    struct BattlePokemon *holder = player, *other = opponent;
    PARAMETRIZE { holder = player; other = opponent; }
    PARAMETRIZE { holder = opponent; other = player; }
    GIVEN {
        PLAYER(holder == player ? SPECIES_DITTO : SPECIES_WOBBUFFET) { Ability(holder == player ? ABILITY_SWIFT_SWIM : ABILITY_NONE); Item(holder == player ? ITEM_QUICK_POWDER : ITEM_NONE); Speed(holder == player ? 101 : 201); if (holder == player) Status1(STATUS1_PARALYSIS); }
        OPPONENT(holder == opponent ? SPECIES_DITTO : SPECIES_WOBBUFFET) { Ability(holder == opponent ? ABILITY_SWIFT_SWIM : ABILITY_NONE); Item(holder == opponent ? ITEM_QUICK_POWDER : ITEM_NONE); Speed(holder == opponent ? 101 : 201); if (holder == opponent) Status1(STATUS1_PARALYSIS); }
    } WHEN {
        TURN { MOVE(other, MOVE_RAIN_DANCE); MOVE(holder, MOVE_SPLASH, WITH_RNG(RNG_PARALYSIS, FALSE)); }
        TURN { MOVE(other, MOVE_TACKLE); MOVE(holder, MOVE_TACKLE, WITH_RNG(RNG_PARALYSIS, FALSE)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_RAIN_DANCE, other);
        if (B_POLISHED_MOVE_BALANCE) { ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, other); ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, holder); }
        else { ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, holder); ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, other); }
    }
}

SINGLE_BATTLE_TEST("Polished core weather abilities use source finite durations under older native presets")
{
    static const enum Ability abilities[] = {ABILITY_DRIZZLE, ABILITY_DROUGHT, ABILITY_SAND_STREAM, ABILITY_SNOW_WARNING};
    static const enum Item rocks[] = {ITEM_DAMP_ROCK, ITEM_HEAT_ROCK, ITEM_SMOOTH_ROCK, ITEM_ICY_ROCK};
    enum Ability ability = ABILITY_DRIZZLE;
    enum Item item = ITEM_NONE;
    u32 duration = 5, gen = GEN_5, elapsed = 5;
    struct BattlePokemon *holder = player;
    for (u32 side = 0; side < 2; side++) for (u32 a = 0; a < 4; a++) for (u32 rock = 0; rock < 2; rock++) for (u32 modern = 0; modern < 2; modern++) for (u32 before = 0; before < 2; before++)
        PARAMETRIZE { holder = side ? opponent : player; ability = abilities[a]; item = rock ? rocks[a] : ITEM_NONE; duration = rock ? 8 : 5; gen = modern ? GEN_6 : GEN_5; elapsed = duration - before; }
    GIVEN {
        WITH_CONFIG(B_ABILITY_WEATHER, gen);
        WITH_CONFIG(B_SNOW_WARNING, GEN_9);
        PLAYER(SPECIES_WOBBUFFET) { Ability(holder == player ? ability : ABILITY_NONE); Item(holder == player ? item : ITEM_NONE); MaxHP(10000); HP(10000); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(holder == opponent ? ability : ABILITY_NONE); Item(holder == opponent ? item : ITEM_NONE); MaxHP(10000); HP(10000); }
    } WHEN { for (u32 turn = 0; turn < elapsed; turn++) TURN {} }
    THEN {
        if ((B_POLISHED_MOVE_BALANCE || gen >= GEN_6) && elapsed == duration) EXPECT_EQ(gBattleWeather, B_WEATHER_NONE);
        else EXPECT_NE(gBattleWeather, B_WEATHER_NONE);
        if ((B_POLISHED_MOVE_BALANCE || gen >= GEN_6) && elapsed < duration) EXPECT_EQ(gBattleStruct->weatherDuration, 1);
    }
}

SINGLE_BATTLE_TEST("Polished core Snow Warning creates source Hail rather than native Snow")
{
    struct BattlePokemon *holder = player, *target = opponent;
    s16 damage;
    PARAMETRIZE { holder = player; target = opponent; }
    PARAMETRIZE { holder = opponent; target = player; }
    GIVEN {
        WITH_CONFIG(B_SNOW_WARNING, GEN_9);
        PLAYER(SPECIES_WOBBUFFET) { Ability(holder == player ? ABILITY_SNOW_WARNING : ABILITY_NONE); HP(240); MaxHP(240); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(holder == opponent ? ABILITY_SNOW_WARNING : ABILITY_NONE); HP(240); MaxHP(240); }
    } WHEN { TURN {} }
    SCENE { if (B_POLISHED_MOVE_BALANCE) HP_BAR(target, captureDamage: &damage); else NOT HP_BAR(target); }
    THEN {
        EXPECT_EQ(gBattleWeather, B_POLISHED_MOVE_BALANCE ? B_WEATHER_HAIL : B_WEATHER_SNOW);
        if (B_POLISHED_MOVE_BALANCE) EXPECT_EQ(damage, 15);
    }
}

SINGLE_BATTLE_TEST("Polished core Hail residual ability gates include source Slush Rush")
{
    static const enum Ability abilities[] = {ABILITY_NONE, ABILITY_SNOW_CLOAK, ABILITY_OVERCOAT, ABILITY_MAGIC_GUARD, ABILITY_ICE_BODY, ABILITY_SLUSH_RUSH};
    enum Ability ability = ABILITY_NONE;
    struct BattlePokemon *holder = player, *other = opponent;
    for (u32 side = 0; side < 2; side++) for (u32 a = 0; a < ARRAY_COUNT(abilities); a++)
        PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; ability = abilities[a]; }
    bool32 hurt = ability == ABILITY_NONE || (ability == ABILITY_SLUSH_RUSH && !B_POLISHED_MOVE_BALANCE);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(holder == player ? ability : ABILITY_NONE); MaxHP(240); HP(240); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(holder == opponent ? ability : ABILITY_NONE); MaxHP(240); HP(240); }
    } WHEN { TURN { MOVE(other, MOVE_HAIL); } }
    THEN { EXPECT_EQ(holder->hp, hurt ? 225 : 240); }
}

SINGLE_BATTLE_TEST("Polished core Safety Goggles and Cloud Nine prevent Hail residual without extending duration")
{
    bool32 cloud = FALSE;
    PARAMETRIZE { cloud = FALSE; }
    PARAMETRIZE { cloud = TRUE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(cloud ? ITEM_NONE : ITEM_SAFETY_GOGGLES); MaxHP(240); HP(240); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(cloud ? ABILITY_CLOUD_NINE : ABILITY_NONE); MaxHP(1000); HP(1000); }
    } WHEN { TURN { MOVE(opponent, MOVE_HAIL); } TURN {} TURN {} TURN {} TURN {} }
    THEN { EXPECT_EQ(player->hp, 240); EXPECT_EQ(gBattleWeather, B_WEATHER_NONE); }
}

SINGLE_BATTLE_TEST("Polished core source weather replaces overworld weather and installs its own timer")
{
    static const enum BattleWeather weathers[] = {BATTLE_WEATHER_RAIN, BATTLE_WEATHER_SUN, BATTLE_WEATHER_HAIL, BATTLE_WEATHER_SANDSTORM};
    u32 n = 0;
    for (u32 w = 0; w < ARRAY_COUNT(weathers); w++) PARAMETRIZE { n = w; }
    GIVEN { PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); } OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); } }
    WHEN { TURN {} }
    THEN {
        gBattleWeather = B_WEATHER_NONE; gBattleStruct->overworldWeatherPresent = TRUE;
        EXPECT_EQ(TryChangeBattleWeather(GetBattlerAtPosition(B_POSITION_PLAYER_LEFT), weathers[n], ABILITY_NONE), B_POLISHED_MOVE_BALANCE ? WEATHER_FAILURE_SUCCESS : WEATHER_FAILURE_OVERWORLD);
        if (B_POLISHED_MOVE_BALANCE) { EXPECT_EQ(gBattleStruct->weatherDuration, 5); EXPECT_EQ((u32)gBattleStruct->overworldWeatherPresent, FALSE); }
    }
}

SINGLE_BATTLE_TEST("Polished core reapplying the same weather does not refresh its timer")
{
    GIVEN { PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); } OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); } }
    WHEN { TURN { MOVE(player, MOVE_HAIL); } TURN {} TURN {} }
    THEN {
        EXPECT_EQ(gBattleStruct->weatherDuration, 2);
        EXPECT_EQ(TryChangeBattleWeather(GetBattlerAtPosition(B_POSITION_PLAYER_LEFT), BATTLE_WEATHER_HAIL, ABILITY_SNOW_WARNING), WEATHER_FAILURE_SAME_WEATHER);
        EXPECT_EQ(gBattleStruct->weatherDuration, 2);
    }
}

DOUBLE_BATTLE_TEST("Polished core doubles keep native Snow Warning Slush Rush and critical odds")
{
    PASSES_RANDOMLY(1, 16, RNG_CRITICAL_HIT);
    GIVEN {
        WITH_CONFIG(B_SNOW_WARNING, GEN_9); WITH_CONFIG(B_CRIT_CHANCE, GEN_3);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_SNOW_WARNING); Speed(200); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(150); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_SLUSH_RUSH); Speed(100); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(50); }
    } WHEN { TURN { MOVE(playerLeft, MOVE_TACKLE, target: opponentLeft); } }
    SCENE { MESSAGE("A critical hit!"); }
    THEN { EXPECT_EQ(gBattleWeather, B_WEATHER_SNOW); }
}

SINGLE_BATTLE_TEST("Polished core either active gimmick retains native critical odds and affection")
{
    bool32 holderTera = FALSE;
    PARAMETRIZE { holderTera = FALSE; }
    PARAMETRIZE { holderTera = TRUE; }
    PASSES_RANDOMLY(1, 16, RNG_CRITICAL_HIT);
    GIVEN {
        WITH_CONFIG(B_CRIT_CHANCE, GEN_3);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Friendship(255); TeraType(TYPE_PSYCHIC); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); TeraType(TYPE_PSYCHIC); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH, gimmick: holderTera ? GIMMICK_TERA : GIMMICK_NONE); MOVE(opponent, MOVE_SPLASH, gimmick: holderTera ? GIMMICK_NONE : GIMMICK_TERA); }
        TURN { MOVE(player, MOVE_TACKLE); }
    } SCENE { MESSAGE("A critical hit!"); }
    THEN {
        EXPECT_EQ(player->affectionHearts, 5);
        EXPECT_EQ(UsesPolishedBattleRules(GetBattlerAtPosition(B_POSITION_PLAYER_LEFT), GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT)), FALSE);
        // Scripted singles are recorded-link replays, which suppress affection.
        // Remove only that test-harness flag to exercise ordinary singles here.
        gBattleTypeFlags &= ~BATTLE_TYPE_RECORDED_LINK;
        EXPECT_EQ(GetBattlerAffectionHearts(GetBattlerAtPosition(B_POSITION_PLAYER_LEFT)), 5);
    }
}

SINGLE_BATTLE_TEST("Polished core either active gimmick retains native self confusion arithmetic")
{
    bool32 holderTera = FALSE;
    s16 damage;
    PARAMETRIZE { holderTera = FALSE; }
    PARAMETRIZE { holderTera = TRUE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(50); Ability(ABILITY_NONE); Attack(255); Defense(256); Speed(200); TeraType(TYPE_PSYCHIC); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); TeraType(TYPE_PSYCHIC); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH, gimmick: holderTera ? GIMMICK_TERA : GIMMICK_NONE); MOVE(opponent, MOVE_CONFUSE_RAY, gimmick: holderTera ? GIMMICK_NONE : GIMMICK_TERA); }
        TURN { MOVE(player, MOVE_TACKLE, WITH_RNG(RNG_CONFUSION, TRUE)); }
    } SCENE { MESSAGE("It hurt itself in its confusion!"); HP_BAR(player, captureDamage: &damage); }
    THEN { EXPECT_EQ(damage, 22 * 40 * 255 / 256 / 50 + 2); }
}
