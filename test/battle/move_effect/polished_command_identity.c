#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished command identity Attract uses original party genders after either participant transforms")
{
    bool32 enemy = FALSE, userTransforms = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 actor = 0; actor < 2; actor++)
            PARAMETRIZE { enemy = side; userTransforms = actor; }
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    struct BattlePokemon *transformed = userTransforms ? user : target;
    GIVEN {
        PLAYER(SPECIES_TAUROS) { Ability(ABILITY_NONE); Speed(100); Gender(MON_MALE); Moves(MOVE_TRANSFORM, MOVE_ATTRACT, MOVE_SPLASH); }
        OPPONENT(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Speed(50); Gender(MON_FEMALE); Moves(MOVE_TRANSFORM, MOVE_ATTRACT, MOVE_SPLASH); }
    } WHEN {
        TURN { MOVE(transformed, MOVE_TRANSFORM); MOVE(transformed == user ? target : user, MOVE_SPLASH); }
        TURN { MOVE(user, MOVE_ATTRACT); MOVE(target, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TRANSFORM, transformed);
        if (B_POLISHED_MOVE_BALANCE) ANIMATION(ANIM_TYPE_MOVE, MOVE_ATTRACT, user);
        else NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_ATTRACT, user);
    } THEN {
        EXPECT_EQ(!!target->volatiles.infatuation, B_POLISHED_MOVE_BALANCE);
        EXPECT_EQ(GetBattlerGender(B_BATTLER_0), GetBattlerGender(B_BATTLER_1));
        EXPECT_NE(GetMonGender(GetBattlerMon(B_BATTLER_0)), GetMonGender(GetBattlerMon(B_BATTLER_1)));
    }
}

SINGLE_BATTLE_TEST("Polished command identity Cute Charm uses party genders after either participant transforms")
{
    bool32 enemy = FALSE, userTransforms = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 actor = 0; actor < 2; actor++)
            PARAMETRIZE { enemy = side; userTransforms = actor; }
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    struct BattlePokemon *transformed = userTransforms ? user : target;
    GIVEN {
        PLAYER(SPECIES_TAUROS) { Ability(ABILITY_CUTE_CHARM); Speed(100); Gender(MON_MALE); HP(1000); MaxHP(1000); Moves(MOVE_TRANSFORM, MOVE_TACKLE, MOVE_SPLASH); }
        OPPONENT(SPECIES_CHANSEY) { Ability(ABILITY_CUTE_CHARM); Speed(50); Gender(MON_FEMALE); HP(1000); MaxHP(1000); Moves(MOVE_TRANSFORM, MOVE_TACKLE, MOVE_SPLASH); }
    } WHEN {
        TURN { MOVE(transformed, MOVE_TRANSFORM); MOVE(transformed == user ? target : user, MOVE_SPLASH); }
        TURN { MOVE(user, MOVE_TACKLE, WITH_RNG(RNG_CUTE_CHARM, TRUE)); MOVE(target, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TRANSFORM, transformed);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, user);
        if (B_POLISHED_MOVE_BALANCE) ABILITY_POPUP(target, ABILITY_CUTE_CHARM);
        else NOT ABILITY_POPUP(target, ABILITY_CUTE_CHARM);
    } THEN {
        EXPECT_EQ(!!user->volatiles.infatuation, B_POLISHED_MOVE_BALANCE);
        EXPECT_EQ(GetBattlerGender(B_BATTLER_0), GetBattlerGender(B_BATTLER_1));
    }
}

SINGLE_BATTLE_TEST("Polished command identity Rivalry retains original opposite and genderless identities after Transform", s16 damage)
{
    bool32 enemy = FALSE, genderless = FALSE, rivalry = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 unknown = 0; unknown < 2; unknown++)
            for (u32 boost = 0; boost < 2; boost++)
                PARAMETRIZE { enemy = side; genderless = unknown; rivalry = boost; }
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    GIVEN {
        PLAYER(enemy ? SPECIES_CHANSEY : genderless ? SPECIES_DITTO : SPECIES_TAUROS) {
            // A nonzero inert ability avoids Tauros's natural Intimidate;
            // Ability(NONE) does not override a species with no empty slot.
            Ability(rivalry ? ABILITY_RIVALRY : ABILITY_BATTLE_ARMOR); Speed(100); HP(1000); MaxHP(1000); Attack(200); Defense(100);
            if (enemy || !genderless) Gender(enemy ? MON_FEMALE : MON_MALE);
            Moves(MOVE_TRANSFORM, MOVE_TACKLE, MOVE_SPLASH);
        }
        OPPONENT(!enemy ? SPECIES_CHANSEY : genderless ? SPECIES_DITTO : SPECIES_TAUROS) {
            Ability(rivalry ? ABILITY_RIVALRY : ABILITY_BATTLE_ARMOR); Speed(50); HP(1000); MaxHP(1000); Attack(200); Defense(100);
            if (!enemy || !genderless) Gender(enemy ? MON_MALE : MON_FEMALE);
            Moves(MOVE_TRANSFORM, MOVE_TACKLE, MOVE_SPLASH);
        }
    } WHEN {
        TURN { MOVE(user, MOVE_TRANSFORM); MOVE(target, MOVE_SPLASH); }
        TURN { MOVE(user, MOVE_TACKLE, criticalHit: FALSE, WITH_RNG(RNG_DAMAGE_MODIFIER, 0)); MOVE(target, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, user);
        HP_BAR(target, captureDamage: &results[i].damage);
    } THEN {
        EXPECT_EQ(user->statStages[STAT_ATK], DEFAULT_STAT_STAGE);
        EXPECT_EQ(target->statStages[STAT_ATK], DEFAULT_STAT_STAGE);
    } FINALLY {
        for (u32 side = 0; side < 2; side++) {
            if (B_POLISHED_MOVE_BALANCE) {
                EXPECT_LT(results[4 * side + 1].damage, results[4 * side].damage);
                EXPECT_EQ(results[4 * side + 3].damage, results[4 * side + 2].damage);
            } else {
                EXPECT_GT(results[4 * side + 1].damage, results[4 * side].damage);
                EXPECT_GT(results[4 * side + 3].damage, results[4 * side + 2].damage);
            }
        }
    }
}

SINGLE_BATTLE_TEST("Polished command identity Room Service snapshots Speed order after direct Trick Room activation")
{
    bool32 enemy = FALSE, playerFaster = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 fast = 0; fast < 2; fast++)
            PARAMETRIZE { enemy = side; playerFaster = fast; }
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *first = B_POLISHED_MOVE_BALANCE && playerFaster ? opponent : player;
    struct BattlePokemon *second = first == player ? opponent : player;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(playerFaster ? 100 : 50); Item(ITEM_ROOM_SERVICE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(playerFaster ? 50 : 100); Item(ITEM_ROOM_SERVICE); }
    } WHEN {
        TURN { MOVE(user, MOVE_TRICK_ROOM); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TRICK_ROOM, user);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, first);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_STATS_CHANGE, first);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, second);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_STATS_CHANGE, second);
    } THEN {
        EXPECT_EQ(player->item, ITEM_NONE);
        EXPECT_EQ(opponent->item, ITEM_NONE);
        EXPECT_EQ(player->statStages[STAT_SPEED], DEFAULT_STAT_STAGE - 1);
        EXPECT_EQ(opponent->statStages[STAT_SPEED], DEFAULT_STAT_STAGE - 1);
        EXPECT_NE(gFieldStatuses & STATUS_FIELD_TRICK_ROOM, 0);
        EXPECT_EQ(gFieldTimers.trickRoomTimer, 4);
    }
}

SINGLE_BATTLE_TEST("Polished command identity capped Room Service holders keep their items on activation")
{
    bool32 enemy = FALSE;
    PARAMETRIZE { enemy = FALSE; }
    PARAMETRIZE { enemy = TRUE; }
    struct BattlePokemon *user = enemy ? opponent : player;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_ROOM_SERVICE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_ROOM_SERVICE); }
    } WHEN {
        for (u32 turn = 0; turn < 3; turn++)
            TURN { MOVE(player, MOVE_SCARY_FACE, hit: TRUE); MOVE(opponent, MOVE_SCARY_FACE, hit: TRUE); }
        TURN { MOVE(user, MOVE_TRICK_ROOM); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TRICK_ROOM, user);
        NONE_OF {
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, player);
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, opponent);
        }
    } THEN {
        EXPECT_EQ(player->item, ITEM_ROOM_SERVICE);
        EXPECT_EQ(opponent->item, ITEM_ROOM_SERVICE);
        EXPECT_EQ(player->statStages[STAT_SPEED], MIN_STAT_STAGE);
        EXPECT_EQ(opponent->statStages[STAT_SPEED], MIN_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished command identity Room Service does not activate when Trick Room is toggled off or expires")
{
    bool32 enemy = FALSE, expire = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 end = 0; end < 2; end++)
            PARAMETRIZE { enemy = side; expire = end; }
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *other = enemy ? player : opponent;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); Item(ITEM_ROOM_SERVICE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); Item(ITEM_ROOM_SERVICE); }
    } WHEN {
        for (u32 turn = 0; turn < 3; turn++)
            TURN { MOVE(player, MOVE_SCARY_FACE, hit: TRUE); MOVE(opponent, MOVE_SCARY_FACE, hit: TRUE); }
        TURN { MOVE(user, MOVE_TRICK_ROOM); }
        if (expire) {
            for (u32 turn = 0; turn < 3; turn++) TURN {}
            TURN { MOVE(other, MOVE_HAZE); }
        } else {
            TURN { MOVE(other, MOVE_HAZE); MOVE(user, MOVE_TRICK_ROOM); }
        }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_HAZE, other);
        NONE_OF {
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, player);
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, opponent);
        }
    } THEN {
        EXPECT_EQ(player->item, ITEM_ROOM_SERVICE);
        EXPECT_EQ(opponent->item, ITEM_ROOM_SERVICE);
        EXPECT_EQ(player->statStages[STAT_SPEED], DEFAULT_STAT_STAGE);
        EXPECT_EQ(opponent->statStages[STAT_SPEED], DEFAULT_STAT_STAGE);
        EXPECT_EQ(gFieldStatuses & STATUS_FIELD_TRICK_ROOM, 0);
        EXPECT_EQ(gFieldTimers.trickRoomTimer, 0);
    }
}
