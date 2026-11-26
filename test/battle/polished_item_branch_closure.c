#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished item closure sun damage heals before the other weather holder")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Ability ability = ABILITY_SOLAR_POWER;
    bool32 tera = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 dry = 0; dry < 2; dry++)
            for (u32 optional = 0; optional < 2; optional++)
                PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; ability = dry ? ABILITY_DRY_SKIN : ABILITY_SOLAR_POWER; tera = optional; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(holder == player ? ability : ABILITY_SOLAR_POWER); Speed(holder == player ? 200 : 100); MaxHP(161); HP(holder == player ? 81 : 161); Item(holder == player ? ITEM_SITRUS_BERRY : ITEM_NONE); TeraType(TYPE_PSYCHIC); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(holder == opponent ? ability : ABILITY_SOLAR_POWER); Speed(holder == opponent ? 200 : 100); MaxHP(161); HP(holder == opponent ? 81 : 161); Item(holder == opponent ? ITEM_SITRUS_BERRY : ITEM_NONE); TeraType(TYPE_PSYCHIC); }
    } WHEN {
        TURN { MOVE(holder, MOVE_SPLASH, gimmick: tera ? GIMMICK_TERA : GIMMICK_NONE); MOVE(other, MOVE_SUNNY_DAY); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SUNNY_DAY, other);
        ABILITY_POPUP(holder, ability);
        HP_BAR(holder, damage: 20);
        // Source SubtractHP heals before the next weather holder. Native and
        // optional-gimmick battles have no end-turn HP-threshold item sweep.
        if (B_POLISHED_MOVE_BALANCE && !tera) HP_BAR(holder, damage: -40);
        ABILITY_POPUP(other, ABILITY_SOLAR_POWER);
        HP_BAR(other, damage: 20);
        if (!B_POLISHED_MOVE_BALANCE || tera) NOT HP_BAR(holder);
    } THEN {
        EXPECT_EQ(holder->hp, B_POLISHED_MOVE_BALANCE && !tera ? 101 : 61);
        EXPECT_EQ(other->hp, 141);
        EXPECT_EQ(holder->item, B_POLISHED_MOVE_BALANCE && !tera ? ITEM_NONE : ITEM_SITRUS_BERRY);
    }
}

SINGLE_BATTLE_TEST("Polished item closure sun abilities honor holder Umbrella and weather suppression")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Ability ability = ABILITY_SOLAR_POWER;
    u32 kind = 0;
    for (u32 side = 0; side < 2; side++)
        for (u32 dry = 0; dry < 2; dry++)
            for (u32 blocker = 0; blocker < 5; blocker++)
                PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; ability = dry ? ABILITY_DRY_SKIN : ABILITY_SOLAR_POWER; kind = blocker; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(holder == player ? ability : kind == 1 ? ABILITY_CLOUD_NINE : kind == 2 ? ABILITY_NEUTRALIZING_GAS : ABILITY_NONE); Speed(100); MaxHP(161); HP(161); Item((kind == 3 && holder == player) || (kind == 4 && other == player) ? ITEM_UTILITY_UMBRELLA : ITEM_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(holder == opponent ? ability : kind == 1 ? ABILITY_CLOUD_NINE : kind == 2 ? ABILITY_NEUTRALIZING_GAS : ABILITY_NONE); Speed(50); MaxHP(161); HP(161); Item((kind == 3 && holder == opponent) || (kind == 4 && other == opponent) ? ITEM_UTILITY_UMBRELLA : ITEM_NONE); }
    } WHEN { TURN { MOVE(holder, MOVE_SPLASH); MOVE(other, MOVE_SUNNY_DAY); } }
    SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SUNNY_DAY, other);
        if (kind == 0 || kind == 4) { ABILITY_POPUP(holder, ability); HP_BAR(holder, damage: 20); }
        else NONE_OF { ABILITY_POPUP(holder, ability); HP_BAR(holder); }
    } THEN { EXPECT_EQ(holder->hp, kind == 0 || kind == 4 ? 141 : 161); }
}

SINGLE_BATTLE_TEST("Polished item closure stat and critical berries use inclusive Gluttony pinch boundaries")
{
    struct BattlePokemon *holder = player;
    enum Item item = ITEM_SALAC_BERRY;
    bool32 gluttony = FALSE, above = FALSE, tiny = FALSE;
    u32 blocker = 0;
    for (u32 side = 0; side < 2; side++)
        for (u32 critical = 0; critical < 2; critical++) {
            for (u32 glut = 0; glut < 2; glut++)
                for (u32 high = 0; high < 2; high++)
                    for (u32 suppression = 0; suppression < 3; suppression++)
                        PARAMETRIZE { holder = side ? opponent : player; item = critical ? ITEM_LANSAT_BERRY : ITEM_SALAC_BERRY; gluttony = glut; above = high; blocker = suppression; tiny = FALSE; }
            for (u32 glut = 0; glut < 2; glut++)
                PARAMETRIZE { holder = side ? opponent : player; item = critical ? ITEM_LANSAT_BERRY : ITEM_SALAC_BERRY; gluttony = glut; above = FALSE; blocker = 0; tiny = TRUE; }
        }
    u32 hp = tiny ? 1 : (gluttony ? 80 : 40) + above;
    u32 maxHp = tiny ? (gluttony ? 1 : 2) : 161;
    bool32 activates = tiny ? !!B_POLISHED_MOVE_BALANCE : !above && blocker != 2 && !(gluttony && blocker == 1);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(holder == player ? (gluttony ? ABILITY_GLUTTONY : ABILITY_NONE) : blocker == 1 ? ABILITY_NEUTRALIZING_GAS : blocker == 2 ? ABILITY_UNNERVE : ABILITY_NONE); Speed(100); MaxHP(holder == player ? maxHp : 161); HP(holder == player ? hp : 161); Item(holder == player ? item : ITEM_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(holder == opponent ? (gluttony ? ABILITY_GLUTTONY : ABILITY_NONE) : blocker == 1 ? ABILITY_NEUTRALIZING_GAS : blocker == 2 ? ABILITY_UNNERVE : ABILITY_NONE); Speed(50); MaxHP(holder == opponent ? maxHp : 161); HP(holder == opponent ? hp : 161); Item(holder == opponent ? item : ITEM_NONE); }
    } WHEN { TURN { MOVE(player, MOVE_SPLASH); MOVE(opponent, MOVE_SPLASH); } }
    SCENE {
        if (activates) ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, holder);
        else NOT ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, holder);
    } THEN {
        EXPECT_EQ(holder->hp, hp);
        EXPECT_EQ(holder->item, activates ? ITEM_NONE : item);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + (activates && item == ITEM_SALAC_BERRY));
        EXPECT_EQ((u32)holder->volatiles.focusEnergy, activates && item == ITEM_LANSAT_BERRY);
    }
}

SINGLE_BATTLE_TEST("Polished item closure Starf uniformly selects only uncapped battle stats")
{
    static const u32 masks[] = {0, 1u << STAT_ATK, (1u << STAT_ATK) | (1u << STAT_DEF) | (1u << STAT_SPEED), (1u << STAT_ATK) | (1u << STAT_DEF) | (1u << STAT_SPEED) | (1u << STAT_SPATK)};
    static const enum Move raiseMoves[NUM_STATS] = {[STAT_ATK] = MOVE_SWORDS_DANCE, [STAT_DEF] = MOVE_IRON_DEFENSE, [STAT_SPEED] = MOVE_AGILITY, [STAT_SPATK] = MOVE_NASTY_PLOT, [STAT_SPDEF] = MOVE_AMNESIA};
    struct BattlePokemon *holder = player, *other = opponent;
    u32 capped = 0;
    enum Stat expected = STAT_ATK;
    for (u32 side = 0; side < 2; side++)
        for (u32 mask = 0; mask < ARRAY_COUNT(masks); mask++)
            for (u32 stat = STAT_ATK; stat < NUM_STATS; stat++)
                if (!(masks[mask] & (1u << stat)))
                    PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; capped = masks[mask]; expected = stat; }
    PASSES_RANDOMLY(1, 5 - __builtin_popcount(capped), RNG_RANDOM_STAT_UP);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); MaxHP(161); HP(holder == player ? 80 : 161); Item(holder == player ? ITEM_STARF_BERRY : ITEM_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); MaxHP(161); HP(holder == opponent ? 80 : 161); Item(holder == opponent ? ITEM_STARF_BERRY : ITEM_NONE); }
    } WHEN {
        for (u32 stat = STAT_ATK; stat < NUM_STATS; stat++)
            if (capped & (1u << stat))
                for (u32 use = 0; use < 3; use++)
                    TURN { MOVE(holder, raiseMoves[stat]); MOVE(other, MOVE_SPLASH); }
        TURN { MOVE(holder, capped ? MOVE_SWORDS_DANCE : MOVE_SPLASH); MOVE(other, MOVE_DRAGON_RAGE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_DRAGON_RAGE, other);
        HP_BAR(holder, damage: 40);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, holder);
        // Select the successful tagged trial in SCENE, as the existing Starf
        // probability control does. THEN still verifies every stage and cap.
        switch (expected) {
        case STAT_ATK:
            if (holder == player) MESSAGE("The Starf Berry sharply boosted Wobbuffet's Attack!");
            else MESSAGE("The Starf Berry sharply boosted the opposing Wobbuffet's Attack!");
            break;
        case STAT_DEF:
            if (holder == player) MESSAGE("The Starf Berry sharply boosted Wobbuffet's Defense!");
            else MESSAGE("The Starf Berry sharply boosted the opposing Wobbuffet's Defense!");
            break;
        case STAT_SPEED:
            if (holder == player) MESSAGE("The Starf Berry sharply boosted Wobbuffet's Speed!");
            else MESSAGE("The Starf Berry sharply boosted the opposing Wobbuffet's Speed!");
            break;
        case STAT_SPATK:
            if (holder == player) MESSAGE("The Starf Berry sharply boosted Wobbuffet's Sp. Atk!");
            else MESSAGE("The Starf Berry sharply boosted the opposing Wobbuffet's Sp. Atk!");
            break;
        case STAT_SPDEF:
            if (holder == player) MESSAGE("The Starf Berry sharply boosted Wobbuffet's Sp. Def!");
            else MESSAGE("The Starf Berry sharply boosted the opposing Wobbuffet's Sp. Def!");
            break;
        default:
            break;
        }
    } THEN {
        EXPECT_EQ(holder->item, ITEM_NONE);
        for (u32 stat = STAT_ATK; stat < NUM_STATS; stat++)
            EXPECT_EQ(holder->statStages[stat], capped & (1u << stat) ? MAX_STAT_STAGE : DEFAULT_STAT_STAGE + (stat == expected ? 2 : 0));
        EXPECT_EQ(holder->statStages[STAT_ACC], DEFAULT_STAT_STAGE);
        EXPECT_EQ(holder->statStages[STAT_EVASION], DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished item closure Starf rejects all raw capped candidates before Contrary")
{
    struct BattlePokemon *holder = player, *other = opponent;
    bool32 contrary = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 inverse = 0; inverse < 2; inverse++)
            PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; contrary = inverse; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); MaxHP(holder == player ? 161 : 8000); HP(holder == player ? (contrary ? 161 : 80) : 8000); Defense(1000); SpDefense(1000); Item(holder == player && !contrary ? ITEM_STARF_BERRY : ITEM_NONE); if (holder == player) MovesWithPP({MOVE_ANCIENT_POWER, 8}, {MOVE_BATON_PASS, 40}, {MOVE_SPLASH, 40}); }
        if (holder == player && contrary) PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_CONTRARY); Speed(100); MaxHP(161); HP(80); Item(ITEM_STARF_BERRY); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); MaxHP(holder == opponent ? 161 : 8000); HP(holder == opponent ? (contrary ? 161 : 80) : 8000); Defense(1000); SpDefense(1000); Item(holder == opponent && !contrary ? ITEM_STARF_BERRY : ITEM_NONE); if (holder == opponent) MovesWithPP({MOVE_ANCIENT_POWER, 8}, {MOVE_BATON_PASS, 40}, {MOVE_SPLASH, 40}); }
        if (holder == opponent && contrary) OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_CONTRARY); Speed(50); MaxHP(161); HP(80); Item(ITEM_STARF_BERRY); }
    } WHEN {
        for (u32 use = 0; use < 6; use++) TURN { MOVE(holder, MOVE_ANCIENT_POWER, secondaryEffect: TRUE); MOVE(other, MOVE_SPLASH); }
        if (contrary) TURN { MOVE(holder, MOVE_BATON_PASS); SEND_OUT(holder, 1); MOVE(other, MOVE_SPLASH); }
        TURN { MOVE(holder, MOVE_SPLASH); MOVE(other, MOVE_DRAGON_RAGE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_DRAGON_RAGE, other);
        HP_BAR(holder, damage: 40);
        if (!B_POLISHED_MOVE_BALANCE && contrary) ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, holder);
        else NOT ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, holder);
    } THEN {
        u32 lowered = 0;
        EXPECT_EQ(holder->item, !B_POLISHED_MOVE_BALANCE && contrary ? ITEM_NONE : ITEM_STARF_BERRY);
        for (u32 stat = STAT_ATK; stat < NUM_STATS; stat++) {
            lowered += holder->statStages[stat] == MAX_STAT_STAGE - 2;
            EXPECT(holder->statStages[stat] == MAX_STAT_STAGE || (!B_POLISHED_MOVE_BALANCE && contrary && holder->statStages[stat] == MAX_STAT_STAGE - 2));
        }
        EXPECT_EQ(lowered, !B_POLISHED_MOVE_BALANCE && contrary);
    }
}

SINGLE_BATTLE_TEST("Polished item closure Weakness Policy consumes only for an actual stat change")
{
    struct BattlePokemon *holder = player, *other = opponent;
    u32 capped = 0;
    bool32 contrary = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 mask = 0; mask < 4; mask++)
            for (u32 inverse = 0; inverse < 2; inverse++)
                PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; capped = mask; contrary = inverse; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(holder == player && contrary ? ABILITY_CONTRARY : ABILITY_NONE); Speed(100); HP(1600); MaxHP(1600); Attack(1); Defense(1000); Item(holder == player ? ITEM_WEAKNESS_POLICY : ITEM_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(holder == opponent && contrary ? ABILITY_CONTRARY : ABILITY_NONE); Speed(50); HP(1600); MaxHP(1600); Attack(1); Defense(1000); Item(holder == opponent ? ITEM_WEAKNESS_POLICY : ITEM_NONE); }
    } WHEN {
        if (capped & 1) for (u32 use = 0; use < 3; use++) TURN { MOVE(holder, MOVE_SWORDS_DANCE); MOVE(other, MOVE_SPLASH); }
        if (capped & 2) for (u32 use = 0; use < 3; use++) TURN { MOVE(holder, MOVE_NASTY_PLOT); MOVE(other, MOVE_SPLASH); }
        TURN { MOVE(holder, MOVE_SPLASH); MOVE(other, MOVE_BITE, secondaryEffect: FALSE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_BITE, other);
        HP_BAR(holder);
        if (!B_POLISHED_MOVE_BALANCE || capped != 3) ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, holder);
        else NOT ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, holder);
    } THEN {
        EXPECT_EQ(holder->item, B_POLISHED_MOVE_BALANCE && capped == 3 ? ITEM_WEAKNESS_POLICY : ITEM_NONE);
        EXPECT_EQ(holder->statStages[STAT_ATK], DEFAULT_STAT_STAGE + (contrary ? -1 : 1) * (capped & 1 ? 6 : 2));
        EXPECT_EQ(holder->statStages[STAT_SPATK], DEFAULT_STAT_STAGE + (contrary ? -1 : 1) * (capped & 2 ? 6 : 2));
    }
}

SINGLE_BATTLE_TEST("Polished item closure Weakness Policy applies through dolls and only to super effective hits")
{
    struct BattlePokemon *holder = player, *other = opponent;
    u32 kind = 0;
    for (u32 side = 0; side < 2; side++)
        for (u32 mode = 0; mode < 6; mode++)
            PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; kind = mode; }
    enum Move move = kind == 1 ? MOVE_TACKLE : kind == 2 ? MOVE_CONFUSION : kind == 3 ? MOVE_NIGHT_SHADE : MOVE_BITE;
    bool32 activates = kind == 0 || ((kind == 4 || kind == 5) && B_POLISHED_MOVE_BALANCE);
    GIVEN {
        PLAYER(holder == player && kind == 3 ? SPECIES_CHANSEY : SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Level(50); Speed(100); MaxHP(kind == 5 ? 100 : 400); HP(kind == 5 ? 100 : 400); Attack(kind == 5 ? 100 : 1); Defense(kind == 5 ? 100 : 1000); SpAttack(1); SpDefense(1000); Item(holder == player ? ITEM_WEAKNESS_POLICY : ITEM_NONE); }
        OPPONENT(holder == opponent && kind == 3 ? SPECIES_CHANSEY : SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Level(50); Speed(50); MaxHP(kind == 5 ? 100 : 400); HP(kind == 5 ? 100 : 400); Attack(kind == 5 ? 100 : 1); Defense(kind == 5 ? 100 : 1000); SpAttack(1); SpDefense(1000); Item(holder == opponent ? ITEM_WEAKNESS_POLICY : ITEM_NONE); }
    } WHEN {
        if (kind >= 4) TURN { MOVE(holder, MOVE_SUBSTITUTE); MOVE(other, MOVE_SPLASH); }
        TURN { MOVE(holder, MOVE_SPLASH); MOVE(other, move, secondaryEffect: FALSE); }
    } SCENE {
        if (kind >= 4) ANIMATION(ANIM_TYPE_MOVE, MOVE_SUBSTITUTE, holder);
        if (kind == 5) ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_SUBSTITUTE_FADE, holder);
        if (activates) {
            // Headless skips the fade task that clears the sprite's doll bit,
            // so both doll cases use actual stat and item state as evidence.
            ANIMATION(ANIM_TYPE_GENERAL, kind >= 4 ? B_ANIM_STATS_CHANGE : B_ANIM_HELD_ITEM_EFFECT, holder);
        }
        else NOT ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, holder);
    } THEN {
        EXPECT_EQ(holder->item, activates ? ITEM_NONE : ITEM_WEAKNESS_POLICY);
        EXPECT_EQ(holder->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 2 * activates);
        EXPECT_EQ(holder->statStages[STAT_SPATK], DEFAULT_STAT_STAGE + 2 * activates);
        if (kind == 4) EXPECT(holder->volatiles.substituteHP > 0);
        if (kind == 5) EXPECT_EQ((u32)holder->volatiles.substituteHP, 0);
    }
}

SINGLE_BATTLE_TEST("Polished item closure Weakness Policy waits for the final ordinary multi hit")
{
    struct BattlePokemon *holder = player, *other = opponent;
    bool32 parental = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 bond = 0; bond < 2; bond++)
            PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; parental = bond; }
    GIVEN {
        PLAYER(parental || holder != player ? SPECIES_WOBBUFFET : SPECIES_CHANSEY) { Ability(other == player && parental ? ABILITY_PARENTAL_BOND : ABILITY_NONE); Speed(100); MaxHP(1600); HP(1600); Attack(100); Defense(1000); Item(holder == player ? ITEM_WEAKNESS_POLICY : ITEM_NONE); }
        OPPONENT(parental || holder != opponent ? SPECIES_WOBBUFFET : SPECIES_CHANSEY) { Ability(other == opponent && parental ? ABILITY_PARENTAL_BOND : ABILITY_NONE); Speed(50); MaxHP(1600); HP(1600); Attack(100); Defense(1000); Item(holder == opponent ? ITEM_WEAKNESS_POLICY : ITEM_NONE); }
    } WHEN { TURN { MOVE(holder, MOVE_SPLASH); MOVE(other, parental ? MOVE_BITE : MOVE_DOUBLE_KICK, secondaryEffect: FALSE); } }
    SCENE {
        HP_BAR(holder);
        if (!B_POLISHED_MOVE_BALANCE || parental) ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, holder);
        HP_BAR(holder);
        if (B_POLISHED_MOVE_BALANCE && !parental) ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, holder);
    } THEN {
        EXPECT_EQ(holder->item, ITEM_NONE);
        EXPECT_EQ(holder->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 2);
        EXPECT_EQ(holder->statStages[STAT_SPATK], DEFAULT_STAT_STAGE + 2);
    }
}

SINGLE_BATTLE_TEST("Polished item closure defend hit berries respect category caps dolls and Sheer Force")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Item item = ITEM_KEE_BERRY;
    u32 kind = 0;
    for (u32 side = 0; side < 2; side++)
        for (u32 special = 0; special < 2; special++)
            for (u32 mode = 0; mode < 7; mode++)
                PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; item = special ? ITEM_MARANGA_BERRY : ITEM_KEE_BERRY; kind = mode; }
    bool32 special = item == ITEM_MARANGA_BERRY;
    enum Stat stat = special ? STAT_SPDEF : STAT_DEF;
    enum Move move = kind == 5 ? (special ? MOVE_EMBER : MOVE_ROCK_SLIDE) : (special != (kind == 1) ? MOVE_SWIFT : MOVE_SCRATCH);
    bool32 activates = kind == 0 || ((kind == 2 || kind == 3) && B_POLISHED_MOVE_BALANCE);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(other == player ? (kind == 5 ? ABILITY_SHEER_FORCE : kind == 6 ? ABILITY_UNNERVE : ABILITY_NONE) : ABILITY_NONE); Speed(100); MaxHP(kind == 3 ? 100 : 400); HP(kind == 3 ? 100 : 400); Attack(kind == 3 ? 100 : 1); SpAttack(kind == 3 ? 100 : 1); Defense(kind == 3 ? 100 : 1000); SpDefense(kind == 3 ? 100 : 1000); Item(holder == player ? item : ITEM_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(other == opponent ? (kind == 5 ? ABILITY_SHEER_FORCE : kind == 6 ? ABILITY_UNNERVE : ABILITY_NONE) : ABILITY_NONE); Speed(50); MaxHP(kind == 3 ? 100 : 400); HP(kind == 3 ? 100 : 400); Attack(kind == 3 ? 100 : 1); SpAttack(kind == 3 ? 100 : 1); Defense(kind == 3 ? 100 : 1000); SpDefense(kind == 3 ? 100 : 1000); Item(holder == opponent ? item : ITEM_NONE); }
    } WHEN {
        if (kind == 2 || kind == 3) TURN { MOVE(holder, MOVE_SUBSTITUTE); MOVE(other, MOVE_SPLASH); }
        if (kind == 4) for (u32 use = 0; use < 3; use++) TURN { MOVE(holder, special ? MOVE_AMNESIA : MOVE_IRON_DEFENSE); MOVE(other, MOVE_SPLASH); }
        TURN { MOVE(holder, MOVE_SPLASH); MOVE(other, move, secondaryEffect: FALSE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, other);
        if (kind == 3) ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_SUBSTITUTE_FADE, holder);
        if (activates) ANIMATION(ANIM_TYPE_GENERAL, kind == 2 || kind == 3 ? B_ANIM_STATS_CHANGE : B_ANIM_HELD_ITEM_BERRY, holder);
        else NOT ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, holder);
    } THEN {
        EXPECT_EQ(holder->item, activates ? ITEM_NONE : item);
        EXPECT_EQ(holder->statStages[stat], kind == 4 ? MAX_STAT_STAGE : DEFAULT_STAT_STAGE + activates);
        if (kind == 2) EXPECT(holder->volatiles.substituteHP > 0);
        if (kind == 3) EXPECT_EQ((u32)holder->volatiles.substituteHP, 0);
    }
}

SINGLE_BATTLE_TEST("Polished item closure retaliation berries distinguish category doll and immunity gates")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Item item = ITEM_JABOCA_BERRY;
    u32 kind = 0;
    for (u32 side = 0; side < 2; side++)
        for (u32 special = 0; special < 2; special++)
            for (u32 mode = 0; mode < 8; mode++)
                PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; item = special ? ITEM_ROWAP_BERRY : ITEM_JABOCA_BERRY; kind = mode; }
    bool32 special = item == ITEM_ROWAP_BERRY;
    enum Move move = kind == 7 ? (special ? MOVE_EMBER : MOVE_ROCK_SLIDE) : (special != (kind == 1) ? MOVE_SWIFT : MOVE_SCRATCH);
    bool32 activates = kind == 0 || kind == 4 || kind == 7 || (kind == 3 && B_POLISHED_MOVE_BALANCE);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(other == player ? (kind == 5 ? ABILITY_MAGIC_GUARD : kind == 6 ? ABILITY_UNNERVE : kind == 7 ? ABILITY_SHEER_FORCE : ABILITY_NONE) : ABILITY_NONE); Speed(100); MaxHP(holder == player ? (kind == 3 ? 100 : 400) : 161); HP(holder == player ? (kind == 4 ? 1 : kind == 3 ? 100 : 400) : 161); Attack(kind == 3 ? 100 : 1); SpAttack(kind == 3 ? 100 : 1); Defense(kind == 3 ? 100 : 1000); SpDefense(kind == 3 ? 100 : 1000); Item(holder == player ? item : ITEM_NONE); }
        if (holder == player && kind == 4) PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(other == opponent ? (kind == 5 ? ABILITY_MAGIC_GUARD : kind == 6 ? ABILITY_UNNERVE : kind == 7 ? ABILITY_SHEER_FORCE : ABILITY_NONE) : ABILITY_NONE); Speed(50); MaxHP(holder == opponent ? (kind == 3 ? 100 : 400) : 161); HP(holder == opponent ? (kind == 4 ? 1 : kind == 3 ? 100 : 400) : 161); Attack(kind == 3 ? 100 : 1); SpAttack(kind == 3 ? 100 : 1); Defense(kind == 3 ? 100 : 1000); SpDefense(kind == 3 ? 100 : 1000); Item(holder == opponent ? item : ITEM_NONE); }
        if (holder == opponent && kind == 4) OPPONENT(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Speed(50); }
    } WHEN {
        if (kind == 2 || kind == 3) TURN { MOVE(holder, MOVE_SUBSTITUTE); MOVE(other, MOVE_SPLASH); }
        TURN { MOVE(holder, MOVE_SPLASH); MOVE(other, move, secondaryEffect: FALSE); if (kind == 4) SEND_OUT(holder, 1); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, other);
        if (kind == 3) ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_SUBSTITUTE_FADE, holder);
        if (activates) {
            // The shipping retaliation script skips the animation for a
            // fainted holder. Headless also retains the doll sprite bit after
            // a recorded fade; exact attacker HP and item state prove use.
            if (kind != 3 && kind != 4) ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, holder);
            HP_BAR(other, damage: 20);
        }
        else NONE_OF { ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, holder); HP_BAR(other); }
    } THEN {
        EXPECT_EQ(other->hp, activates ? 141 : 161);
        if (kind != 4) EXPECT_EQ(holder->item, activates ? ITEM_NONE : item);
        else EXPECT_EQ(holder->species, SPECIES_CHANSEY);
        if (kind == 2) EXPECT(holder->volatiles.substituteHP > 0);
        if (kind == 3) EXPECT_EQ((u32)holder->volatiles.substituteHP, 0);
    }
}

SINGLE_BATTLE_TEST("Polished item closure Throat Spray preserves sound failure and cap gates")
{
    struct BattlePokemon *holder = player, *other = opponent;
    u32 kind = 0;
    for (u32 side = 0; side < 2; side++)
        for (u32 mode = 0; mode < 8; mode++)
            PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; kind = mode; }
    enum Move move = kind == 1 ? MOVE_HYPER_VOICE : kind == 2 ? MOVE_SCREECH : kind == 7 ? MOVE_ROAR : MOVE_GROWL;
    bool32 activates = kind == 0 || kind == 1 || kind == 5;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(other == player ? (kind == 4 ? ABILITY_SOUNDPROOF : kind == 5 ? ABILITY_CLEAR_BODY : ABILITY_NONE) : ABILITY_NONE); Speed(100); HP(1600); MaxHP(1600); SpAttack(1); SpDefense(1000); Item(holder == player ? ITEM_THROAT_SPRAY : ITEM_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(other == opponent ? (kind == 4 ? ABILITY_SOUNDPROOF : kind == 5 ? ABILITY_CLEAR_BODY : ABILITY_NONE) : ABILITY_NONE); Speed(50); HP(1600); MaxHP(1600); SpAttack(1); SpDefense(1000); Item(holder == opponent ? ITEM_THROAT_SPRAY : ITEM_NONE); }
    } WHEN {
        if (kind == 6) for (u32 use = 0; use < 3; use++) TURN { MOVE(holder, MOVE_NASTY_PLOT); MOVE(other, MOVE_SPLASH); }
        TURN { MOVE(holder, move, hit: kind != 2); MOVE(other, kind == 3 ? MOVE_PROTECT : MOVE_SPLASH); }
    } SCENE {
        if (activates) ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, holder);
        else NOT ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, holder);
    } THEN {
        EXPECT_EQ(holder->item, activates ? ITEM_NONE : ITEM_THROAT_SPRAY);
        EXPECT_EQ(holder->statStages[STAT_SPATK], kind == 6 ? MAX_STAT_STAGE : DEFAULT_STAT_STAGE + activates);
    }
}

SINGLE_BATTLE_TEST("Polished item closure Perish Song Spray distinguishes priority failure from repeated timers")
{
    struct BattlePokemon *holder = player, *other = opponent;
    bool32 repeat = FALSE, tera = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 existing = 0; existing < 2; existing++)
            for (u32 optional = 0; optional < 2; optional++)
                PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; repeat = existing; tera = optional; }
    bool32 source = B_POLISHED_MOVE_BALANCE && !tera;
    // Native's all-target success predicate still sees the user as affected
    // when the effect only prints repeat failure. Source explicitly retains
    // that case too, but rejects the opponent's Soundproof priority failure.
    bool32 activates = source ? repeat : TRUE;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(other == player && !repeat ? ABILITY_SOUNDPROOF : ABILITY_NONE); Speed(100); Item(holder == player ? ITEM_THROAT_SPRAY : ITEM_NONE); TeraType(TYPE_PSYCHIC); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(other == opponent && !repeat ? ABILITY_SOUNDPROOF : ABILITY_NONE); Speed(50); Item(holder == opponent ? ITEM_THROAT_SPRAY : ITEM_NONE); TeraType(TYPE_PSYCHIC); }
    } WHEN {
        if (repeat) TURN { MOVE(holder, MOVE_SPLASH, gimmick: tera ? GIMMICK_TERA : GIMMICK_NONE); MOVE(other, MOVE_PERISH_SONG); }
        TURN { MOVE(holder, MOVE_PERISH_SONG, gimmick: tera && !repeat ? GIMMICK_TERA : GIMMICK_NONE); MOVE(other, MOVE_SPLASH); }
    } SCENE {
        if (activates) ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, holder);
        else NOT ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, holder);
    } THEN {
        EXPECT_EQ(holder->item, activates ? ITEM_NONE : ITEM_THROAT_SPRAY);
        EXPECT_EQ(holder->statStages[STAT_SPATK], DEFAULT_STAT_STAGE + activates);
        EXPECT(holder->volatiles.perishSong);
        EXPECT_EQ((u32)other->volatiles.perishSong, repeat);
        EXPECT_EQ(gBattleStruct->polishedPerishSongSprayEligible, FALSE);
    }
}
