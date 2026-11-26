#include "global.h"
#include "test/battle.h"

static const struct {
    enum Move move;
    enum Stat stat;
    u8 amount;
} sPrimaryStatMoves[] = {
    {MOVE_LEER, STAT_DEF, 1}, {MOVE_GROWL, STAT_ATK, 1},
    {MOVE_STRING_SHOT, STAT_SPEED, 2}, {MOVE_SCREECH, STAT_DEF, 2},
    {MOVE_SMOKESCREEN, STAT_ACC, 1}, {MOVE_FLASH, STAT_ACC, 1},
    {MOVE_SCARY_FACE, STAT_SPEED, 2}, {MOVE_CHARM, STAT_ATK, 2},
    {MOVE_SWAGGER, STAT_ATK, 2},
};

SINGLE_BATTLE_TEST("Polished command stat accuracy precedes caps for every primary target stat script")
{
    u32 entry = 0;
    bool32 enemy = FALSE;
    for (u32 m = 0; m < ARRAY_COUNT(sPrimaryStatMoves); m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { entry = m; enemy = side; }
    enum Move move = sPrimaryStatMoves[entry].move;
    enum Stat stat = sPrimaryStatMoves[entry].stat;
    u32 setupTurns = 6 / sPrimaryStatMoves[entry].amount;
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(enemy && move == MOVE_SWAGGER ? ABILITY_OWN_TEMPO : ABILITY_NONE); Speed(enemy ? 50 : 100); Item(enemy ? ITEM_BRIGHT_POWDER : ITEM_BLUNDER_POLICY); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(!enemy && move == MOVE_SWAGGER ? ABILITY_OWN_TEMPO : ABILITY_NONE); Speed(enemy ? 100 : 50); Item(enemy ? ITEM_BLUNDER_POLICY : ITEM_BRIGHT_POWDER); }
    } WHEN {
        for (u32 turn = 0; turn < setupTurns; turn++) TURN { MOVE(user, move, hit: TRUE); }
        TURN { MOVE(user, move, WITH_RNG(RNG_ACCURACY, FALSE)); }
    } THEN {
        EXPECT_EQ(target->statStages[stat], move == MOVE_SWAGGER ? MAX_STAT_STAGE : MIN_STAT_STAGE);
        EXPECT_EQ(user->item, B_POLISHED_MOVE_BALANCE ? ITEM_NONE : ITEM_BLUNDER_POLICY);
        EXPECT_EQ(user->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + 2 * B_POLISHED_MOVE_BALANCE);
    }
}

SINGLE_BATTLE_TEST("Polished command stat accuracy keeps the exact ninety percent capped Growl cutoff")
{
    if (B_POLISHED_MOVE_BALANCE) PASSES_RANDOMLY(90, 100, RNG_ACCURACY);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); Item(ITEM_BLUNDER_POLICY); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); Item(ITEM_BRIGHT_POWDER); }
    } WHEN {
        // Explicit overrides precede PASSES_RANDOMLY; hit: TRUE does not.
        for (u32 turn = 0; turn < 6; turn++) TURN { MOVE(player, MOVE_GROWL, WITH_RNG(RNG_ACCURACY, TRUE)); }
        TURN {
            if (B_POLISHED_MOVE_BALANCE) MOVE(player, MOVE_GROWL);
            else MOVE(player, MOVE_GROWL, WITH_RNG(RNG_ACCURACY, FALSE));
        }
    } SCENE { NOT ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, player); }
    THEN {
        EXPECT_EQ(opponent->statStages[STAT_ATK], MIN_STAT_STAGE);
        EXPECT_EQ(player->item, ITEM_BLUNDER_POLICY);
    }
}

SINGLE_BATTLE_TEST("Polished command stat accuracy misses precede every source defensive stat guard")
{
    static const enum Ability abilities[] = {
        ABILITY_CLEAR_BODY, ABILITY_WHITE_SMOKE, ABILITY_HYPER_CUTTER,
        ABILITY_BIG_PECKS, ABILITY_KEEN_EYE, ABILITY_ILLUMINATE,
        ABILITY_MINDS_EYE, ABILITY_NONE,
    };
    u32 guard = 0;
    bool32 enemy = FALSE, hit = FALSE;
    for (u32 g = 0; g < ARRAY_COUNT(abilities); g++)
        for (u32 side = 0; side < 2; side++)
            for (u32 lands = 0; lands < 2; lands++)
                PARAMETRIZE { guard = g; enemy = side; hit = lands; }
    enum Move move = guard == 2 ? MOVE_CHARM : guard >= 4 && guard <= 6 ? MOVE_FLASH : MOVE_SCREECH;
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    GIVEN {
        WITH_CONFIG(B_ILLUMINATE_EFFECT, GEN_9);
        PLAYER(SPECIES_WOBBUFFET) { Ability(enemy ? abilities[guard] : ABILITY_NONE); Speed(enemy ? 50 : 100); Item(!enemy ? ITEM_BLUNDER_POLICY : guard == 7 ? ITEM_CLEAR_AMULET : ITEM_BRIGHT_POWDER); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(!enemy ? abilities[guard] : ABILITY_NONE); Speed(enemy ? 100 : 50); Item(enemy ? ITEM_BLUNDER_POLICY : guard == 7 ? ITEM_CLEAR_AMULET : ITEM_BRIGHT_POWDER); }
    } WHEN {
        TURN { MOVE(user, move, WITH_RNG(RNG_ACCURACY, hit)); }
    } THEN {
        for (u32 stat = STAT_ATK; stat < NUM_BATTLE_STATS; stat++) EXPECT_EQ(target->statStages[stat], DEFAULT_STAT_STAGE);
        EXPECT_EQ(user->item, B_POLISHED_MOVE_BALANCE && !hit ? ITEM_NONE : ITEM_BLUNDER_POLICY);
        EXPECT_EQ(user->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + 2 * (B_POLISHED_MOVE_BALANCE && !hit));
    }
}

SINGLE_BATTLE_TEST("Polished command stat accuracy precedes ordinary Substitute but keeps Swagger early rejection")
{
    bool32 enemy = FALSE, swagger = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 special = 0; special < 2; special++)
            PARAMETRIZE { enemy = side; swagger = special; }
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(enemy ? 50 : 100); Item(enemy ? ITEM_BRIGHT_POWDER : ITEM_BLUNDER_POLICY); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(enemy ? 100 : 50); Item(enemy ? ITEM_BLUNDER_POLICY : ITEM_BRIGHT_POWDER); }
    } WHEN {
        TURN { MOVE(target, MOVE_SUBSTITUTE); MOVE(user, MOVE_SPLASH); }
        TURN { MOVE(user, swagger ? MOVE_SWAGGER : MOVE_LEER, WITH_RNG(RNG_ACCURACY, FALSE)); }
    } THEN {
        EXPECT_EQ(user->item, B_POLISHED_MOVE_BALANCE && !swagger ? ITEM_NONE : ITEM_BLUNDER_POLICY);
        EXPECT_EQ(user->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + 2 * (B_POLISHED_MOVE_BALANCE && !swagger));
        EXPECT_EQ(target->statStages[STAT_ATK], DEFAULT_STAT_STAGE);
        EXPECT_EQ(target->statStages[STAT_DEF], DEFAULT_STAT_STAGE);
        EXPECT_EQ((u32)target->volatiles.confusionTimer, 0);
    }
}

SINGLE_BATTLE_TEST("Polished command stat accuracy leaves self buffs and optional target stat moves native")
{
    bool32 self = FALSE;
    PARAMETRIZE { self = FALSE; }
    PARAMETRIZE { self = TRUE; }
    enum Move move = self ? MOVE_SWORDS_DANCE : MOVE_TICKLE;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_BLUNDER_POLICY); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_BRIGHT_POWDER); }
    } WHEN {
        for (u32 turn = 0; turn < (self ? 3 : 6); turn++) TURN { MOVE(player, move, hit: TRUE); }
        TURN { MOVE(player, move, WITH_RNG(RNG_ACCURACY, FALSE)); }
    } THEN {
        EXPECT_EQ(player->item, ITEM_BLUNDER_POLICY);
        EXPECT_EQ(player->statStages[STAT_SPEED], DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished command stat accuracy preserves either established native gimmick")
{
    bool32 userTera = FALSE;
    PARAMETRIZE { userTera = FALSE; }
    PARAMETRIZE { userTera = TRUE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); Item(ITEM_BLUNDER_POLICY); TeraType(TYPE_NORMAL); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); Item(ITEM_BRIGHT_POWDER); TeraType(TYPE_NORMAL); }
    } WHEN {
        for (u32 turn = 0; turn < 6; turn++) TURN { MOVE(player, MOVE_GROWL, hit: TRUE); }
        TURN { MOVE(player, MOVE_SPLASH, gimmick: userTera ? GIMMICK_TERA : GIMMICK_NONE); MOVE(opponent, MOVE_SPLASH, gimmick: userTera ? GIMMICK_NONE : GIMMICK_TERA); }
        TURN { MOVE(player, MOVE_GROWL, WITH_RNG(RNG_ACCURACY, FALSE)); }
    } THEN {
        EXPECT_EQ(player->item, ITEM_BLUNDER_POLICY);
        EXPECT_EQ(player->statStages[STAT_SPEED], DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished command Guard Spec stops source Infiltrator while optional stat moves keep native bypass")
{
    static const enum Move moves[] = {MOVE_CHARM, MOVE_TICKLE, MOVE_OCTAZOOKA};
    bool32 enemy = FALSE, infiltrator = FALSE;
    enum Move move = MOVE_CHARM;
    for (u32 side = 0; side < 2; side++)
        for (u32 ability = 0; ability < 2; ability++)
            for (u32 entry = 0; entry < ARRAY_COUNT(moves); entry++)
                PARAMETRIZE { enemy = side; infiltrator = ability; move = moves[entry]; }
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    bool32 blocked = !infiltrator || (B_POLISHED_MOVE_BALANCE && move != MOVE_TICKLE);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(!enemy && infiltrator ? ABILITY_INFILTRATOR : ABILITY_NONE); Speed(100); HP(1000); MaxHP(1000); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(enemy && infiltrator ? ABILITY_INFILTRATOR : ABILITY_NONE); Speed(50); HP(1000); MaxHP(1000); }
    } WHEN {
        TURN { USE_ITEM(target, ITEM_GUARD_SPEC, partyIndex: 0); MOVE(user, MOVE_SPLASH); }
        TURN { MOVE(user, move, hit: TRUE, secondaryEffect: TRUE); }
    } THEN {
        EXPECT_EQ(target->statStages[STAT_ATK], DEFAULT_STAT_STAGE - (blocked || move == MOVE_OCTAZOOKA ? 0 : move == MOVE_TICKLE ? 1 : 2));
        EXPECT_EQ(target->statStages[STAT_DEF], DEFAULT_STAT_STAGE - (!blocked && move == MOVE_TICKLE));
        EXPECT_EQ(target->statStages[STAT_ACC], DEFAULT_STAT_STAGE - (!blocked && move == MOVE_OCTAZOOKA));
        EXPECT_GT(gSideTimers[GetBattlerSide(target == player ? B_BATTLER_0 : B_BATTLER_1)].mistTimer, 0);
    }
}

SINGLE_BATTLE_TEST("Polished command Illuminate protects source accuracy drops under legacy presets and respects suppression")
{
    static const enum Move moves[] = {MOVE_FLASH, MOVE_OCTAZOOKA, MOVE_KINESIS};
    static const enum Ability attackers[] = {ABILITY_NONE, ABILITY_MOLD_BREAKER, ABILITY_NEUTRALIZING_GAS};
    enum Move move = MOVE_FLASH;
    enum Ability ability = ABILITY_NONE;
    bool32 enemy = FALSE, old = FALSE;
    for (u32 m = 0; m < ARRAY_COUNT(moves); m++)
        for (u32 a = 0; a < ARRAY_COUNT(attackers); a++)
            for (u32 side = 0; side < 2; side++)
                for (u32 preset = 0; preset < 2; preset++)
                    PARAMETRIZE { move = moves[m]; ability = attackers[a]; enemy = side; old = preset; }
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    bool32 blocked = ability == ABILITY_NONE && (!old || (B_POLISHED_MOVE_BALANCE && move != MOVE_KINESIS));
    GIVEN {
        WITH_CONFIG(B_ILLUMINATE_EFFECT, old ? GEN_3 : GEN_9);
        PLAYER(SPECIES_WOBBUFFET) { Ability(enemy ? ABILITY_ILLUMINATE : ability); HP(1000); MaxHP(1000); Speed(enemy ? 50 : 100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(!enemy ? ABILITY_ILLUMINATE : ability); HP(1000); MaxHP(1000); Speed(enemy ? 100 : 50); }
    } WHEN {
        TURN { MOVE(user, move, hit: TRUE, secondaryEffect: TRUE); }
    } THEN { EXPECT_EQ(target->statStages[STAT_ACC], DEFAULT_STAT_STAGE - !blocked); }
}

SINGLE_BATTLE_TEST("Polished command stat misses precede Soundproof Armor Tail and Prankster Dark nullification")
{
    u32 guard = 0;
    bool32 enemy = FALSE, hit = FALSE;
    for (u32 kind = 0; kind < 3; kind++)
        for (u32 side = 0; side < 2; side++)
            for (u32 lands = 0; lands < 2; lands++)
                PARAMETRIZE { guard = kind; enemy = side; hit = lands; }
    enum Ability targetAbility = guard == 0 ? ABILITY_SOUNDPROOF : guard == 1 ? ABILITY_ARMOR_TAIL : ABILITY_NONE;
    enum Ability userAbility = guard == 0 ? ABILITY_NONE : ABILITY_PRANKSTER;
    enum Species targetSpecies = guard == 2 ? SPECIES_UMBREON : SPECIES_WOBBUFFET;
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    GIVEN {
        PLAYER(enemy ? targetSpecies : SPECIES_WOBBUFFET) { Ability(enemy ? targetAbility : userAbility); Speed(enemy ? 50 : 100); Item(enemy ? ITEM_NONE : ITEM_BLUNDER_POLICY); }
        OPPONENT(!enemy ? targetSpecies : SPECIES_WOBBUFFET) { Ability(!enemy ? targetAbility : userAbility); Speed(enemy ? 100 : 50); Item(enemy ? ITEM_BLUNDER_POLICY : ITEM_NONE); }
    } WHEN {
        TURN { MOVE(user, MOVE_SCREECH, WITH_RNG(RNG_ACCURACY, hit)); }
    } SCENE {
        if (B_POLISHED_MOVE_BALANCE && !hit) {
            NONE_OF { ABILITY_POPUP(target, targetAbility); ANIMATION(ANIM_TYPE_MOVE, MOVE_SCREECH, user); }
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, user);
        } else if (guard != 2) {
            ABILITY_POPUP(target, targetAbility);
        }
    } THEN {
        EXPECT_EQ(target->statStages[STAT_DEF], DEFAULT_STAT_STAGE);
        EXPECT_EQ(user->item, B_POLISHED_MOVE_BALANCE && !hit ? ITEM_NONE : ITEM_BLUNDER_POLICY);
        EXPECT_EQ(user->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + 2 * (B_POLISHED_MOVE_BALANCE && !hit));
    }
}

SINGLE_BATTLE_TEST("Polished command reflected original miss gives Blunder Policy only to the reflector")
{
    bool32 enemy = FALSE, userPolicy = FALSE, reflectorPolicy = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 first = 0; first < 2; first++)
            for (u32 second = 0; second < 2; second++)
                PARAMETRIZE { enemy = side; userPolicy = first; reflectorPolicy = second; }
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(enemy ? ABILITY_MAGIC_BOUNCE : ABILITY_NONE); Speed(100); Item((enemy ? reflectorPolicy : userPolicy) ? ITEM_BLUNDER_POLICY : ITEM_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(!enemy ? ABILITY_MAGIC_BOUNCE : ABILITY_NONE); Speed(50); Item((!enemy ? reflectorPolicy : userPolicy) ? ITEM_BLUNDER_POLICY : ITEM_NONE); }
    } WHEN {
        // Screech is below 100% on both paths. In source the first miss is
        // retained; native independently misses the returned accuracy roll.
        TURN { MOVE(user, MOVE_SCREECH, WITH_RNG(RNG_ACCURACY, FALSE)); }
    } SCENE {
        ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE);
        if (reflectorPolicy) ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, target);
        NOT ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, user);
    } THEN {
        EXPECT_EQ(user->item, userPolicy ? ITEM_BLUNDER_POLICY : ITEM_NONE);
        EXPECT_EQ(target->item, ITEM_NONE);
        EXPECT_EQ(user->statStages[STAT_SPEED], DEFAULT_STAT_STAGE);
        EXPECT_EQ(target->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + 2 * reflectorPolicy);
        EXPECT_EQ(user->statStages[STAT_DEF], DEFAULT_STAT_STAGE);
        EXPECT_EQ(target->statStages[STAT_DEF], DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished command source stat ability precedes Clear Amulet while misses precede both")
{
    bool32 enemy = FALSE, hit = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 lands = 0; lands < 2; lands++)
            PARAMETRIZE { enemy = side; hit = lands; }
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    GIVEN {
        WITH_CONFIG(B_ILLUMINATE_EFFECT, GEN_9);
        PLAYER(SPECIES_WOBBUFFET) { Ability(enemy ? ABILITY_ILLUMINATE : ABILITY_NONE); Item(enemy ? ITEM_CLEAR_AMULET : ITEM_BLUNDER_POLICY); Speed(enemy ? 50 : 100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(!enemy ? ABILITY_ILLUMINATE : ABILITY_NONE); Item(enemy ? ITEM_BLUNDER_POLICY : ITEM_CLEAR_AMULET); Speed(enemy ? 100 : 50); }
    } WHEN {
        // Evasion makes even source Flash's 100 accuracy a real hit check.
        TURN { MOVE(target, MOVE_DOUBLE_TEAM); MOVE(user, MOVE_SPLASH); }
        TURN { MOVE(user, MOVE_FLASH, WITH_RNG(RNG_ACCURACY, hit)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_DOUBLE_TEAM, target);
        if (B_POLISHED_MOVE_BALANCE && hit) {
            ABILITY_POPUP(target, ABILITY_ILLUMINATE);
        } else if (B_POLISHED_MOVE_BALANCE) {
            NOT ABILITY_POPUP(target, ABILITY_ILLUMINATE);
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, user);
        } else if (enemy) {
            MESSAGE("The effects of the Clear Amulet held by Wobbuffet prevents its stats from being lowered!");
        } else {
            MESSAGE("The effects of the Clear Amulet held by the opposing Wobbuffet prevents its stats from being lowered!");
        }
    } THEN {
        EXPECT_EQ(target->item, ITEM_CLEAR_AMULET);
        EXPECT_EQ(target->statStages[STAT_ACC], DEFAULT_STAT_STAGE);
        EXPECT_EQ(user->item, B_POLISHED_MOVE_BALANCE && !hit ? ITEM_NONE : ITEM_BLUNDER_POLICY);
    }
}

SINGLE_BATTLE_TEST("Polished command manual miss text does not consume Blunder Policy")
{
    static const enum Move moves[] = {
        MOVE_LEECH_SEED, MOVE_ATTRACT, MOVE_FORESIGHT, MOVE_SUPERSONIC,
        MOVE_CONFUSE_RAY, MOVE_SWEET_KISS, MOVE_TRICK,
    };
    enum Move move = MOVE_LEECH_SEED;
    bool32 enemy = FALSE;
    for (u32 entry = 0; entry < ARRAY_COUNT(moves); entry++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = moves[entry]; enemy = side; }
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    bool32 consumes = !B_POLISHED_MOVE_BALANCE && move != MOVE_FORESIGHT;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Gender(MON_MALE); Item(enemy ? ITEM_BRIGHT_POWDER : ITEM_BLUNDER_POLICY); Speed(enemy ? 50 : 100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Gender(MON_FEMALE); Item(enemy ? ITEM_BLUNDER_POLICY : ITEM_BRIGHT_POWDER); Speed(enemy ? 100 : 50); }
    } WHEN {
        // Foresight's raw source 255 is a real roll at this evasion stage;
        // native Foresight remains guaranteed and is the explicit control.
        for (u32 turn = 0; turn < 6; turn++) TURN { MOVE(target, MOVE_DOUBLE_TEAM); MOVE(user, MOVE_SPLASH); }
        TURN { MOVE(user, move, WITH_RNG(RNG_ACCURACY, FALSE)); }
    } THEN {
        EXPECT_EQ(user->item, consumes ? ITEM_NONE : ITEM_BLUNDER_POLICY);
        EXPECT_EQ(user->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + 2 * consumes);
        EXPECT_EQ((u32)target->volatiles.leechSeed, 0);
        EXPECT_EQ((u32)target->volatiles.infatuation, 0);
        EXPECT_EQ((u32)target->volatiles.confusionTimer, 0);
        EXPECT_EQ(!!target->volatiles.foresight, !B_POLISHED_MOVE_BALANCE && move == MOVE_FORESIGHT);
    }
}

SINGLE_BATTLE_TEST("Polished command Sing nullification overrides a miss before the separate priority helper")
{
    static const enum Move moves[] = {MOVE_SING, MOVE_SCREECH, MOVE_SUPERSONIC};
    enum Move move = MOVE_SING;
    bool32 enemy = FALSE, prankster = FALSE, dark = FALSE;
    for (u32 entry = 0; entry < ARRAY_COUNT(moves); entry++)
        for (u32 side = 0; side < 2; side++)
            for (u32 ability = 0; ability < 2; ability++)
                for (u32 type = 0; type < 2; type++)
                    PARAMETRIZE { move = moves[entry]; enemy = side; prankster = ability; dark = type; }
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    bool32 consumes = B_POLISHED_MOVE_BALANCE && move != MOVE_SING
        && (move == MOVE_SCREECH || !prankster || dark);
    GIVEN {
        PLAYER(enemy && dark ? SPECIES_UMBREON : SPECIES_WOBBUFFET) { Ability(enemy ? ABILITY_SOUNDPROOF : prankster ? ABILITY_PRANKSTER : ABILITY_NONE); Item(enemy ? ITEM_NONE : ITEM_BLUNDER_POLICY); Speed(enemy ? 50 : 100); }
        OPPONENT(!enemy && dark ? SPECIES_UMBREON : SPECIES_WOBBUFFET) { Ability(!enemy ? ABILITY_SOUNDPROOF : prankster ? ABILITY_PRANKSTER : ABILITY_NONE); Item(enemy ? ITEM_BLUNDER_POLICY : ITEM_NONE); Speed(enemy ? 100 : 50); }
    } WHEN {
        TURN { MOVE(user, move, WITH_RNG(RNG_ACCURACY, FALSE)); }
    } SCENE {
        if (move == MOVE_SING || !B_POLISHED_MOVE_BALANCE) ABILITY_POPUP(target, ABILITY_SOUNDPROOF);
        else NOT ABILITY_POPUP(target, ABILITY_SOUNDPROOF);
    } THEN {
        EXPECT_EQ(user->item, consumes ? ITEM_NONE : ITEM_BLUNDER_POLICY);
        EXPECT_EQ(user->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + 2 * consumes);
        EXPECT_EQ(target->status1, STATUS1_NONE);
        EXPECT_EQ((u32)target->volatiles.confusionTimer, 0);
        EXPECT_EQ(target->statStages[STAT_DEF], DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished command Trick and Pain Split retain distinct manual and priority miss paths")
{
    enum Move move = MOVE_TRICK;
    bool32 enemy = FALSE, dark = FALSE, hit = FALSE, protect = FALSE;
    for (u32 kind = 0; kind < 2; kind++)
        for (u32 side = 0; side < 2; side++)
            for (u32 type = 0; type < 2; type++)
                for (u32 lands = 0; lands < 2; lands++)
                    for (u32 shield = 0; shield < 2; shield++)
                        PARAMETRIZE { move = kind ? MOVE_PAIN_SPLIT : MOVE_TRICK; enemy = side; dark = type; hit = lands; protect = shield; }
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    bool32 swaps = move == MOVE_TRICK && !dark && hit && !protect;
    bool32 shares = move == MOVE_PAIN_SPLIT && !dark && (!protect || B_POLISHED_MOVE_BALANCE);
    bool32 consumes = !hit && !protect
        && (B_POLISHED_MOVE_BALANCE ? dark : move == MOVE_TRICK && !dark);
    GIVEN {
        PLAYER(enemy && dark ? SPECIES_UMBREON : SPECIES_WOBBUFFET) { Ability(enemy ? ABILITY_NONE : ABILITY_PRANKSTER); HP(enemy ? 200 : 40); MaxHP(200); Item(enemy ? ITEM_BRIGHT_POWDER : ITEM_BLUNDER_POLICY); Speed(enemy ? 50 : 100); }
        OPPONENT(!enemy && dark ? SPECIES_UMBREON : SPECIES_WOBBUFFET) { Ability(enemy ? ABILITY_PRANKSTER : ABILITY_NONE); HP(enemy ? 40 : 200); MaxHP(200); Item(enemy ? ITEM_BLUNDER_POLICY : ITEM_BRIGHT_POWDER); Speed(enemy ? 100 : 50); }
    } WHEN {
        for (u32 turn = 0; turn < 6; turn++) TURN { MOVE(target, MOVE_DOUBLE_TEAM); MOVE(user, MOVE_SPLASH); }
        TURN { if (protect) MOVE(target, MOVE_PROTECT); MOVE(user, move, WITH_RNG(RNG_ACCURACY, hit)); }
    } THEN {
        EXPECT_EQ(user->hp, shares ? 120 : 40);
        EXPECT_EQ(target->hp, shares ? 120 : 200);
        EXPECT_EQ(user->item, swaps ? ITEM_BRIGHT_POWDER : consumes ? ITEM_NONE : ITEM_BLUNDER_POLICY);
        EXPECT_EQ(target->item, swaps ? ITEM_BLUNDER_POLICY : ITEM_BRIGHT_POWDER);
        EXPECT_EQ(user->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + 2 * consumes);
    }
}

SINGLE_BATTLE_TEST("Polished command damaging misses precede Armor Tail and Soundproof after Sucker Punch eligibility")
{
    static const enum Move moves[] = {MOVE_QUICK_ATTACK, MOVE_HYPER_VOICE, MOVE_SUCKER_PUNCH};
    enum Move move = MOVE_QUICK_ATTACK;
    bool32 enemy = FALSE, hit = FALSE, eligible = FALSE;
    for (u32 kind = 0; kind < ARRAY_COUNT(moves); kind++)
        for (u32 side = 0; side < 2; side++)
            for (u32 lands = 0; lands < 2; lands++)
                for (u32 selected = 0; selected < 2; selected++)
                    PARAMETRIZE { move = moves[kind]; enemy = side; hit = lands; eligible = selected; }
    enum Ability guard = move == MOVE_HYPER_VOICE ? ABILITY_SOUNDPROOF : ABILITY_ARMOR_TAIL;
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    bool32 reachesHit = eligible || move != MOVE_SUCKER_PUNCH;
    bool32 consumes = B_POLISHED_MOVE_BALANCE && !hit && reachesHit;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(enemy ? guard : ABILITY_NONE); Item(enemy ? ITEM_BRIGHT_POWDER : ITEM_BLUNDER_POLICY); HP(1000); MaxHP(1000); Speed(enemy ? 50 : 100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(enemy ? ABILITY_NONE : guard); Item(enemy ? ITEM_BLUNDER_POLICY : ITEM_BRIGHT_POWDER); HP(1000); MaxHP(1000); Speed(enemy ? 100 : 50); }
    } WHEN {
        TURN { MOVE(user, move, WITH_RNG(RNG_ACCURACY, hit)); MOVE(target, eligible ? MOVE_POUND : MOVE_SPLASH); }
    } SCENE {
        if (reachesHit && !consumes) ABILITY_POPUP(target, guard);
        else NOT ABILITY_POPUP(target, guard);
    } THEN {
        EXPECT_EQ(target->hp, 1000);
        EXPECT_EQ(user->item, consumes ? ITEM_NONE : ITEM_BLUNDER_POLICY);
        EXPECT_EQ(user->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + 2 * consumes);
    }
}

SINGLE_BATTLE_TEST("Polished command called priority uses the actual script move against Armor Tail")
{
    bool32 enemy = FALSE, hit = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 lands = 0; lands < 2; lands++)
            PARAMETRIZE { enemy = side; hit = lands; }
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(enemy ? ABILITY_ARMOR_TAIL : ABILITY_NONE); Item(enemy ? ITEM_BRIGHT_POWDER : ITEM_BLUNDER_POLICY); HP(1000); MaxHP(1000); Speed(enemy ? 50 : 100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(enemy ? ABILITY_NONE : ABILITY_ARMOR_TAIL); Item(enemy ? ITEM_BLUNDER_POLICY : ITEM_BRIGHT_POWDER); HP(1000); MaxHP(1000); Speed(enemy ? 100 : 50); }
    } WHEN {
        // Quick Attack retains the same source/native numeric identity.
        TURN { MOVE(user, MOVE_METRONOME, WITH_RNG(RNG_METRONOME, MOVE_QUICK_ATTACK), hit: hit); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_METRONOME, user);
        if (B_POLISHED_MOVE_BALANCE && hit) ABILITY_POPUP(target, ABILITY_ARMOR_TAIL);
        else NOT ABILITY_POPUP(target, ABILITY_ARMOR_TAIL);
    } THEN {
        if (!B_POLISHED_MOVE_BALANCE && hit) EXPECT_LT(target->hp, 1000);
        else EXPECT_EQ(target->hp, 1000);
        EXPECT_EQ(user->item, hit ? ITEM_BLUNDER_POLICY : ITEM_NONE);
        EXPECT_EQ(user->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + 2 * !hit);
    }
}
