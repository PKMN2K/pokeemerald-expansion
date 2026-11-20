#include "global.h"
#include "test/battle.h"

static const enum Ability sVolatileContactAbilities[] = {ABILITY_CUTE_CHARM, ABILITY_TANGLING_HAIR, ABILITY_PERISH_BODY};

SINGLE_BATTLE_TEST("Polished volatile contact abilities use the exact Substitute break boundary")
{
    struct BattlePokemon *user = player, *target = opponent;
    u32 ability = 0, maxHP = 200;
    for (u32 side = 0; side < 2; side++)
        for (u32 a = 0; a < ARRAY_COUNT(sVolatileContactAbilities); a++)
            for (u32 hp = 196; hp <= 204; hp += 4)
                PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; ability = a; maxHP = hp; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(50); Gender(MON_MALE); Ability(user == player ? ABILITY_NONE : sVolatileContactAbilities[ability]); if (target == player) { MaxHP(maxHP); HP(maxHP); } }
        OPPONENT(SPECIES_WOBBUFFET) { Level(50); Gender(MON_FEMALE); Ability(user == opponent ? ABILITY_NONE : sVolatileContactAbilities[ability]); if (target == opponent) { MaxHP(maxHP); HP(maxHP); } }
    } WHEN {
        TURN { MOVE(target, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, MOVE_SEISMIC_TOSS, WITH_RNG(RNG_CUTE_CHARM, TRUE)); MOVE(target, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SEISMIC_TOSS, user);
        if (B_POLISHED_MOVE_BALANCE && maxHP <= 200) ABILITY_POPUP(target, sVolatileContactAbilities[ability]);
        else NOT ABILITY_POPUP(target, sVolatileContactAbilities[ability]);
    } THEN {
        bool32 activated = B_POLISHED_MOVE_BALANCE && maxHP <= 200;
        EXPECT_EQ(!!user->volatiles.infatuation, activated && ability == 0);
        EXPECT_EQ(user->statStages[STAT_SPEED], activated && ability == 1 ? DEFAULT_STAT_STAGE - 1 : DEFAULT_STAT_STAGE);
        EXPECT_EQ(!!user->volatiles.perishSong, activated && ability == 2);
        EXPECT_EQ(target->hp, maxHP - maxHP / 4);
        EXPECT_EQ((u32)target->volatiles.substituteHP, maxHP > 200 ? 1 : 0);
    }
}

SINGLE_BATTLE_TEST("Polished volatile contact Cute Charm uses source three-in-ten activation under every preset")
{
    u32 config = GEN_3;
    struct BattlePokemon *user = player, *target = opponent;
    for (u32 side = 0; side < 2; side++)
        for (u32 gen = GEN_3; gen <= GEN_5; gen++)
            PARAMETRIZE { config = gen; user = side ? opponent : player; target = side ? player : opponent; }
    PASSES_RANDOMLY(B_POLISHED_MOVE_BALANCE || config >= GEN_4 ? 3 : 1, B_POLISHED_MOVE_BALANCE || config >= GEN_4 ? 10 : 3, RNG_CUTE_CHARM);
    GIVEN {
        WITH_CONFIG(B_ABILITY_TRIGGER_CHANCE, config);
        PLAYER(SPECIES_WOBBUFFET) { Gender(MON_MALE); Ability(user == player ? ABILITY_NONE : ABILITY_CUTE_CHARM); }
        OPPONENT(SPECIES_WOBBUFFET) { Gender(MON_FEMALE); Ability(user == opponent ? ABILITY_NONE : ABILITY_CUTE_CHARM); }
    } WHEN {
        TURN { MOVE(user, MOVE_TACKLE); MOVE(target, MOVE_SPLASH); }
    } SCENE {
        ABILITY_POPUP(target, ABILITY_CUTE_CHARM);
    }
}

SINGLE_BATTLE_TEST("Polished volatile contact Tangling Hair cannot lower stats after its holder faints")
{
    struct BattlePokemon *user = player, *target = opponent;
    for (u32 side = 0; side < 2; side++) PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_NONE : ABILITY_TANGLING_HAIR); if (target == player) HP(1); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_NONE : ABILITY_TANGLING_HAIR); if (target == opponent) HP(1); }
    } WHEN {
        TURN { MOVE(user, MOVE_TACKLE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, user);
        if (B_POLISHED_MOVE_BALANCE) NOT ABILITY_POPUP(target, ABILITY_TANGLING_HAIR);
        else ABILITY_POPUP(target, ABILITY_TANGLING_HAIR);
    } THEN {
        EXPECT_EQ(user->statStages[STAT_SPEED], B_POLISHED_MOVE_BALANCE ? DEFAULT_STAT_STAGE : DEFAULT_STAT_STAGE - 1);
    }
}

SINGLE_BATTLE_TEST("Polished volatile contact Tangling Hair respects the attacking Pokemon's Substitute")
{
    struct BattlePokemon *user = player, *target = opponent;
    for (u32 side = 0; side < 2; side++) PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_NONE : ABILITY_TANGLING_HAIR); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_NONE : ABILITY_TANGLING_HAIR); }
    } WHEN {
        TURN { MOVE(user, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, MOVE_TACKLE); MOVE(target, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, user);
        NOT ABILITY_POPUP(target, ABILITY_TANGLING_HAIR);
    } THEN {
        EXPECT_EQ(user->statStages[STAT_SPEED], DEFAULT_STAT_STAGE);
        EXPECT_EQ(!!user->volatiles.substitute, TRUE);
    }
}

SINGLE_BATTLE_TEST("Polished volatile contact Perish Body starts the attacker's count even when its holder faints")
{
    struct BattlePokemon *user = player, *target = opponent;
    for (u32 side = 0; side < 2; side++) PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_NONE : ABILITY_PERISH_BODY); if (target == player) HP(1); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_NONE : ABILITY_PERISH_BODY); if (target == opponent) HP(1); }
    } WHEN {
        TURN { MOVE(user, MOVE_TACKLE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, user);
        if (B_POLISHED_MOVE_BALANCE) ABILITY_POPUP(target, ABILITY_PERISH_BODY);
        else NOT ABILITY_POPUP(target, ABILITY_PERISH_BODY);
    } THEN {
        EXPECT_EQ(!!user->volatiles.perishSong, B_POLISHED_MOVE_BALANCE);
    }
}
