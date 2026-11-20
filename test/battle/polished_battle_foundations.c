#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished foundations confusion uses the source byte probability independently of generation")
{
    u32 config = GEN_6;
    struct BattlePokemon *holder = player, *other = opponent;
    for (u32 side = 0; side < 2; side++)
        for (u32 gen = 0; gen < 2; gen++)
            PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; config = gen ? GEN_9 : GEN_6; }
    PASSES_RANDOMLY(B_POLISHED_MOVE_BALANCE ? 85 : config == GEN_6 ? 50 : 33, B_POLISHED_MOVE_BALANCE ? 256 : 100, RNG_CONFUSION);
    GIVEN {
        WITH_CONFIG(B_CONFUSION_SELF_DMG_CHANCE, config);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(holder == player ? 200 : 100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(holder == opponent ? 200 : 100); }
    } WHEN {
        TURN { MOVE(holder, MOVE_SPLASH); MOVE(other, MOVE_CONFUSE_RAY); }
        TURN { MOVE(holder, MOVE_TACKLE); }
    } SCENE {
        MESSAGE("It hurt itself in its confusion!");
        HP_BAR(holder);
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, holder);
    }
}

SINGLE_BATTLE_TEST("Polished foundations confusion decrements and consumes PP only when the attack executes")
{
    bool32 selfHit = FALSE;
    struct BattlePokemon *holder = player, *other = opponent;
    for (u32 side = 0; side < 2; side++)
        for (u32 hit = 0; hit < 2; hit++)
            PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; selfHit = hit; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(holder == player ? 200 : 100); MovesWithPP({MOVE_TACKLE, 5}, {MOVE_SPLASH, 5}, {MOVE_CONFUSE_RAY, 5}); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(holder == opponent ? 200 : 100); MovesWithPP({MOVE_TACKLE, 5}, {MOVE_SPLASH, 5}, {MOVE_CONFUSE_RAY, 5}); }
    } WHEN {
        TURN { MOVE(holder, MOVE_SPLASH); MOVE(other, MOVE_CONFUSE_RAY, WITH_RNG(RNG_CONFUSION_TURNS, 2)); }
        TURN { MOVE(holder, MOVE_TACKLE, WITH_RNG(RNG_CONFUSION, selfHit)); }
        TURN { MOVE(holder, MOVE_TACKLE); }
    } SCENE {
        if (selfHit) { MESSAGE("It hurt itself in its confusion!"); HP_BAR(holder); }
        else ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, holder);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, holder);
    } THEN {
        EXPECT_EQ((u32)holder->volatiles.confusionTimer, 0);
        EXPECT_EQ(holder->pp[0], selfHit ? 4 : 3);
    }
}

SINGLE_BATTLE_TEST("Polished foundations paralysis keeps a one in four stop chance under Champions settings")
{
    struct BattlePokemon *holder = player;
    u32 config = GEN_CHAMPIONS;
    for (u32 side = 0; side < 2; side++)
        for (u32 gen = 0; gen < 2; gen++)
            PARAMETRIZE { holder = side ? opponent : player; config = gen ? GEN_9 : GEN_CHAMPIONS; }
    PASSES_RANDOMLY(1, B_POLISHED_MOVE_BALANCE || config == GEN_9 ? 4 : 8, RNG_PARALYSIS);
    GIVEN {
        WITH_CONFIG(B_PARALYSIS_CHANCE, config);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); if (holder == player) Status1(STATUS1_PARALYSIS); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); if (holder == opponent) Status1(STATUS1_PARALYSIS); }
    } WHEN {
        TURN { MOVE(holder, MOVE_TACKLE); }
    } SCENE {
        if (holder == player) MESSAGE("Wobbuffet couldn't move because it's paralyzed!");
        else MESSAGE("The opposing Wobbuffet couldn't move because it's paralyzed!");
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, holder);
    }
}

SINGLE_BATTLE_TEST("Polished foundations full paralysis preserves PP and the status on either side")
{
    bool32 stopped = FALSE;
    struct BattlePokemon *holder = player;
    for (u32 side = 0; side < 2; side++)
        for (u32 stop = 0; stop < 2; stop++)
            PARAMETRIZE { holder = side ? opponent : player; stopped = stop; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); MovesWithPP({MOVE_TACKLE, 5}); if (holder == player) Status1(STATUS1_PARALYSIS); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); MovesWithPP({MOVE_TACKLE, 5}); if (holder == opponent) Status1(STATUS1_PARALYSIS); }
    } WHEN {
        TURN { MOVE(holder, MOVE_TACKLE, WITH_RNG(RNG_PARALYSIS, stopped)); }
    } THEN {
        EXPECT_EQ(holder->pp[0], stopped ? 5 : 4);
        EXPECT_EQ(holder->status1, STATUS1_PARALYSIS);
    }
}

SINGLE_BATTLE_TEST("Polished foundations paralysis speed is halved even under older generation settings")
{
    u32 speed = 198;
    struct BattlePokemon *holder = player, *other = opponent;
    for (u32 side = 0; side < 2; side++)
        for (u32 fast = 0; fast < 2; fast++)
            PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; speed = fast ? 202 : 198; }
    GIVEN {
        WITH_CONFIG(B_PARALYSIS_SPEED, GEN_6);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(holder == player ? speed : 100); if (holder == player) Status1(STATUS1_PARALYSIS); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(holder == opponent ? speed : 100); if (holder == opponent) Status1(STATUS1_PARALYSIS); }
    } WHEN {
        TURN { MOVE(holder, MOVE_TACKLE, WITH_RNG(RNG_PARALYSIS, FALSE)); MOVE(other, MOVE_TACKLE); }
    } SCENE {
        if (B_POLISHED_MOVE_BALANCE && speed == 202) {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, holder); ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, other);
        } else {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, other); ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, holder);
        }
    }
}

SINGLE_BATTLE_TEST("Polished foundations Quick Feet bypasses paralysis speed loss but not full paralysis")
{
    GIVEN {
        WITH_CONFIG(B_PARALYSIS_SPEED, GEN_6);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_QUICK_FEET); Speed(80); Status1(STATUS1_PARALYSIS); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); }
    } WHEN {
        TURN { MOVE(player, MOVE_TACKLE, WITH_RNG(RNG_PARALYSIS, FALSE)); MOVE(opponent, MOVE_TACKLE); }
        TURN { MOVE(player, MOVE_TACKLE, WITH_RNG(RNG_PARALYSIS, TRUE)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player); ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, opponent);
        MESSAGE("Wobbuffet couldn't move because it's paralyzed!");
    }
}

SINGLE_BATTLE_TEST("Polished foundations direct sleep stores a uniform two to four counter")
{
    static const enum Move moves[] = {MOVE_HYPNOSIS, MOVE_SING, MOVE_SLEEP_POWDER, MOVE_SPORE};
    enum Move move = MOVE_HYPNOSIS;
    u32 counter = 2, config = GEN_CHAMPIONS;
    struct BattlePokemon *holder = player, *other = opponent;
    for (u32 side = 0; side < 2; side++)
        for (u32 m = 0; m < ARRAY_COUNT(moves); m++)
            for (u32 gen = 0; gen < 2; gen++)
                for (u32 turn = 2; turn <= (B_POLISHED_MOVE_BALANCE ? 4 : gen ? 3 : 5); turn++)
                    PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; move = moves[m]; counter = turn; config = gen ? GEN_CHAMPIONS : GEN_3; }
    PASSES_RANDOMLY(B_POLISHED_MOVE_BALANCE || config == GEN_3 || counter == 2 ? 1 : 2, B_POLISHED_MOVE_BALANCE || config != GEN_3 ? 3 : 4, RNG_SLEEP_TURNS);
    GIVEN {
        WITH_CONFIG(B_SLEEP_TURNS, config);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(holder == player ? 200 : 100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(holder == opponent ? 200 : 100); }
    } WHEN {
        TURN { MOVE(holder, MOVE_SPLASH); MOVE(other, move, hit: TRUE); }
        for (u32 turn = 0; turn < counter; turn++) TURN { MOVE(holder, MOVE_TACKLE); }
    } SCENE {
        STATUS_ICON(holder, sleep: TRUE);
        for (u32 turn = 1; turn < counter; turn++) {
            if (holder == player) MESSAGE("Wobbuffet is fast asleep.");
            else MESSAGE("The opposing Wobbuffet is fast asleep.");
        }
        if (holder == player) MESSAGE("Wobbuffet woke up!");
        else MESSAGE("The opposing Wobbuffet woke up!");
        STATUS_ICON(holder, none: TRUE);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, holder);
    }
}

SINGLE_BATTLE_TEST("Polished foundations sleep decrement and Early Bird wake at zero without spending blocked PP")
{
    struct BattlePokemon *holder = player;
    bool32 earlyBird = FALSE;
    u32 counter = 1;
    for (u32 side = 0; side < 2; side++)
        for (u32 ability = 0; ability < 2; ability++)
            for (u32 turns = 1; turns <= 4; turns++)
                PARAMETRIZE { holder = side ? opponent : player; earlyBird = ability; counter = turns; }
    u32 remaining = counter > (earlyBird ? 2 : 1) ? counter - (earlyBird ? 2 : 1) : 0;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(holder == player && earlyBird ? ABILITY_EARLY_BIRD : ABILITY_NONE); MovesWithPP({MOVE_TACKLE, 5}); if (holder == player) Status1(STATUS1_SLEEP_TURN(counter)); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(holder == opponent && earlyBird ? ABILITY_EARLY_BIRD : ABILITY_NONE); MovesWithPP({MOVE_TACKLE, 5}); if (holder == opponent) Status1(STATUS1_SLEEP_TURN(counter)); }
    } WHEN { TURN { MOVE(holder, MOVE_TACKLE); } }
    THEN {
        EXPECT_EQ(holder->status1, STATUS1_SLEEP_TURN(remaining));
        EXPECT_EQ(holder->pp[0], remaining ? 5 : 4);
    }
}

SINGLE_BATTLE_TEST("Polished foundations Hail and Sandstorm defend the source type and damage category")
{
    enum Species species = SPECIES_GLALIE;
    enum Move weather = MOVE_HAIL, attack = MOVE_TACKLE;
    bool32 cloudNine = FALSE, umbrella = FALSE;
    s16 damage;
    for (u32 rock = 0; rock < 2; rock++)
        for (u32 special = 0; special < 2; special++)
            for (u32 suppressed = 0; suppressed < 2; suppressed++)
                for (u32 item = 0; item < 2; item++)
                    PARAMETRIZE { species = rock ? SPECIES_SUDOWOODO : SPECIES_GLALIE; weather = rock ? MOVE_SANDSTORM : MOVE_HAIL; attack = special ? MOVE_WATER_GUN : MOVE_TACKLE; cloudNine = suppressed; umbrella = item; }
    bool32 boosted = !cloudNine && ((weather == MOVE_HAIL && attack == MOVE_TACKLE && B_POLISHED_MOVE_BALANCE) || (weather == MOVE_SANDSTORM && attack == MOVE_WATER_GUN && B_POLISHED_MOVE_BALANCE));
    GIVEN {
        WITH_CONFIG(B_SANDSTORM_SPDEF_BOOST, GEN_3);
        PLAYER(SPECIES_WOBBUFFET) { Level(50); Ability(cloudNine ? ABILITY_CLOUD_NINE : ABILITY_NONE); Attack(100); SpAttack(100); }
        OPPONENT(species) { Ability(ABILITY_NONE); Defense(101); SpDefense(101); Item(umbrella ? ITEM_UTILITY_UMBRELLA : ITEM_NONE); MaxHP(1000); HP(1000); }
    } WHEN {
        TURN { MOVE(opponent, weather); }
        TURN { MOVE(player, attack, WITH_RNG(RNG_DAMAGE_MODIFIER, 0)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, attack, player); HP_BAR(opponent, captureDamage: &damage);
    } THEN {
        EXPECT_EQ(damage, (22 * 40 * 100 / (boosted ? 151 : 101) / 50 + 2) * (species == SPECIES_SUDOWOODO && attack == MOVE_WATER_GUN ? 2 : 1) / (species == SPECIES_SUDOWOODO && attack == MOVE_TACKLE ? 2 : 1));
    }
}

SINGLE_BATTLE_TEST("Polished foundations Hail physical defense applies to Psystrike and both defending sides")
{
    struct BattlePokemon *holder = player, *other = opponent;
    s16 damage;
    for (u32 side = 0; side < 2; side++)
        PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_GLALIE) { Level(50); Ability(ABILITY_NONE); SpAttack(100); Defense(101); SpDefense(201); MaxHP(1000); HP(1000); }
        OPPONENT(SPECIES_GLALIE) { Level(50); Ability(ABILITY_NONE); SpAttack(100); Defense(101); SpDefense(201); MaxHP(1000); HP(1000); }
    } WHEN {
        TURN { MOVE(holder, MOVE_HAIL); }
        TURN { MOVE(other, MOVE_PSYSTRIKE, WITH_RNG(RNG_DAMAGE_MODIFIER, 0)); }
    } SCENE { ANIMATION(ANIM_TYPE_MOVE, MOVE_PSYSTRIKE, other); HP_BAR(holder, captureDamage: &damage); }
    THEN { EXPECT_EQ(damage, 22 * 100 * 100 / (B_POLISHED_MOVE_BALANCE ? 151 : 101) / 50 + 2); }
}

DOUBLE_BATTLE_TEST("Polished foundations doubles retain native paralysis speed and Hail defense")
{
    s16 damage;
    GIVEN {
        WITH_CONFIG(B_PARALYSIS_SPEED, GEN_6);
        PLAYER(SPECIES_WOBBUFFET) { Level(50); Ability(ABILITY_NONE); Speed(202); Attack(100); Status1(STATUS1_PARALYSIS); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(75); }
        OPPONENT(SPECIES_GLALIE) { Ability(ABILITY_NONE); Speed(100); Defense(101); MaxHP(1000); HP(1000); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(25); }
    } WHEN {
        TURN { MOVE(opponentLeft, MOVE_HAIL); }
        TURN { MOVE(opponentLeft, MOVE_SPLASH); MOVE(playerLeft, MOVE_TACKLE, target: opponentLeft, WITH_RNG(RNG_PARALYSIS, FALSE)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, opponentLeft);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, playerLeft); HP_BAR(opponentLeft, captureDamage: &damage);
    } THEN { EXPECT_GT(damage, 22 * 40 * 100 / 151 / 50 + 2); }
}

SINGLE_BATTLE_TEST("Polished foundations either active gimmick retains native sleep duration probabilities")
{
    bool32 holderTera = FALSE;
    u32 counter = 2;
    for (u32 tera = 0; tera < 2; tera++)
        for (u32 turns = 2; turns <= 3; turns++)
            PARAMETRIZE { holderTera = tera; counter = turns; }
    PASSES_RANDOMLY(counter == 2 ? 1 : 2, 3, RNG_SLEEP_TURNS);
    GIVEN {
        WITH_CONFIG(B_SLEEP_TURNS, GEN_CHAMPIONS);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); TeraType(TYPE_NORMAL); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(200); TeraType(TYPE_NORMAL); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_SPLASH, gimmick: holderTera ? GIMMICK_TERA : GIMMICK_NONE); MOVE(player, MOVE_HYPNOSIS, hit: TRUE, gimmick: holderTera ? GIMMICK_NONE : GIMMICK_TERA); }
    } THEN { EXPECT_EQ(opponent->status1, STATUS1_SLEEP_TURN(counter)); }
}

DOUBLE_BATTLE_TEST("Polished foundations doubles retain native confusion and paralysis probabilities")
{
    bool32 confusion = FALSE;
    PARAMETRIZE { confusion = FALSE; }
    PARAMETRIZE { confusion = TRUE; }
    PASSES_RANDOMLY(1, confusion ? 2 : 8, confusion ? RNG_CONFUSION : RNG_PARALYSIS);
    GIVEN {
        WITH_CONFIG(B_CONFUSION_SELF_DMG_CHANCE, GEN_6);
        WITH_CONFIG(B_PARALYSIS_CHANCE, GEN_CHAMPIONS);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(200); if (!confusion) Status1(STATUS1_PARALYSIS); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(25); }
    } WHEN {
        if (confusion) TURN { MOVE(playerLeft, MOVE_SPLASH); MOVE(opponentLeft, MOVE_CONFUSE_RAY, target: playerLeft); }
        TURN { MOVE(playerLeft, MOVE_TACKLE, target: opponentLeft); }
    } SCENE {
        if (confusion) MESSAGE("It hurt itself in its confusion!");
        else MESSAGE("Wobbuffet couldn't move because it's paralyzed!");
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, playerLeft);
    }
}

SINGLE_BATTLE_TEST("Polished foundations either active gimmick retains native confusion and paralysis probabilities")
{
    bool32 confusion = FALSE, holderTera = FALSE;
    for (u32 kind = 0; kind < 2; kind++)
        for (u32 tera = 0; tera < 2; tera++)
            PARAMETRIZE { confusion = kind; holderTera = tera; }
    PASSES_RANDOMLY(1, confusion ? 2 : 8, confusion ? RNG_CONFUSION : RNG_PARALYSIS);
    GIVEN {
        WITH_CONFIG(B_CONFUSION_SELF_DMG_CHANCE, GEN_6);
        WITH_CONFIG(B_PARALYSIS_CHANCE, GEN_CHAMPIONS);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(200); TeraType(TYPE_PSYCHIC); if (!confusion) Status1(STATUS1_PARALYSIS); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); TeraType(TYPE_PSYCHIC); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH, gimmick: holderTera ? GIMMICK_TERA : GIMMICK_NONE, WITH_RNG(RNG_PARALYSIS, FALSE)); MOVE(opponent, confusion ? MOVE_CONFUSE_RAY : MOVE_SPLASH, gimmick: holderTera ? GIMMICK_NONE : GIMMICK_TERA); }
        TURN { MOVE(player, MOVE_TACKLE); }
    } SCENE {
        if (confusion) MESSAGE("It hurt itself in its confusion!");
        else MESSAGE("Wobbuffet couldn't move because it's paralyzed!");
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player);
    }
}

SINGLE_BATTLE_TEST("Polished foundations either active gimmick keeps Hail defense native")
{
    bool32 holderTera = FALSE;
    s16 damage;
    PARAMETRIZE { holderTera = FALSE; }
    PARAMETRIZE { holderTera = TRUE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(50); Ability(ABILITY_NONE); Attack(100); TeraType(TYPE_PSYCHIC); }
        OPPONENT(SPECIES_GLALIE) { Ability(ABILITY_NONE); Defense(101); MaxHP(1000); HP(1000); TeraType(TYPE_ICE); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH, gimmick: holderTera ? GIMMICK_NONE : GIMMICK_TERA); MOVE(opponent, MOVE_HAIL, gimmick: holderTera ? GIMMICK_TERA : GIMMICK_NONE); }
        TURN { MOVE(player, MOVE_TACKLE, WITH_RNG(RNG_DAMAGE_MODIFIER, 0)); }
    } SCENE { ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player); HP_BAR(opponent, captureDamage: &damage); }
    THEN { EXPECT_EQ(damage, 22 * 40 * 100 / 101 / 50 + 2); }
}
