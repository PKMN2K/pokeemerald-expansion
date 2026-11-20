#include "global.h"
#include "test/battle.h"
#include "polished_contact_vectors.h"

static const enum Ability sAbilities[] = {ABILITY_CUTE_CHARM, ABILITY_TANGLING_HAIR, ABILITY_PERISH_BODY};

SINGLE_BATTLE_TEST("Polished volatile contact classification covers all 124 direct attacks for all three abilities")
{
    const struct PolishedContactVector *vector = &sPolishedContactVectors[0];
    struct BattlePokemon *user = player, *target = opponent;
    u32 ability = 0;
    for (u32 m = 0; m < ARRAY_COUNT(sPolishedContactVectors); m++)
        for (u32 a = 0; a < ARRAY_COUNT(sAbilities); a++)
            for (u32 side = 0; side < 2; side++)
                PARAMETRIZE { vector = &sPolishedContactVectors[m]; ability = a; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Gender(MON_MALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(user == player ? ABILITY_NONE : sAbilities[ability]); }
        OPPONENT(SPECIES_WOBBUFFET) { Gender(MON_FEMALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(user == opponent ? ABILITY_NONE : sAbilities[ability]); }
    } WHEN {
        TURN { MOVE(user, vector->move, secondaryEffect: FALSE, WITH_RNG(RNG_CUTE_CHARM, TRUE)); MOVE(target, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, vector->move, user);
        if (B_POLISHED_MOVE_BALANCE ? vector->contact : MoveMakesContact(vector->move)) ABILITY_POPUP(target, sAbilities[ability]);
        else NOT ABILITY_POPUP(target, sAbilities[ability]);
    } THEN {
        EXPECT_LT(target->hp, 1000);
    }
}

SINGLE_BATTLE_TEST("Polished volatile contact Protective Pads Punching Glove and optional Long Reach prevent activation")
{
    struct BattlePokemon *user = player, *target = opponent;
    u32 ability = 0, mode = 0;
    for (u32 a = 0; a < ARRAY_COUNT(sAbilities); a++)
        for (u32 m = 0; m < 4; m++)
            for (u32 side = 0; side < 2; side++) PARAMETRIZE { ability = a; mode = m; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Gender(MON_MALE); Ability(user == player ? mode == 3 ? ABILITY_LONG_REACH : ABILITY_NONE : sAbilities[ability]); Item(user == player ? mode == 0 ? ITEM_PROTECTIVE_PADS : mode < 3 ? ITEM_PUNCHING_GLOVE : ITEM_NONE : ITEM_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Gender(MON_FEMALE); Ability(user == opponent ? mode == 3 ? ABILITY_LONG_REACH : ABILITY_NONE : sAbilities[ability]); Item(user == opponent ? mode == 0 ? ITEM_PROTECTIVE_PADS : mode < 3 ? ITEM_PUNCHING_GLOVE : ITEM_NONE : ITEM_NONE); }
    } WHEN {
        TURN { MOVE(user, mode == 1 ? MOVE_MACH_PUNCH : MOVE_TACKLE, WITH_RNG(RNG_CUTE_CHARM, TRUE)); MOVE(target, MOVE_SPLASH); }
    } SCENE {
        if (mode == 2) ABILITY_POPUP(target, sAbilities[ability]);
        else NOT ABILITY_POPUP(target, sAbilities[ability]);
    }
}

SINGLE_BATTLE_TEST("Polished volatile contact rejects misses Protect noncontact damage and status moves")
{
    struct BattlePokemon *user = player, *target = opponent;
    u32 ability = 0, mode = 0;
    for (u32 a = 0; a < ARRAY_COUNT(sAbilities); a++)
        for (u32 m = 0; m < 4; m++)
            for (u32 side = 0; side < 2; side++) PARAMETRIZE { ability = a; mode = m; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Gender(MON_MALE); Ability(user == player ? ABILITY_NONE : sAbilities[ability]); }
        OPPONENT(SPECIES_WOBBUFFET) { Gender(MON_FEMALE); Ability(user == opponent ? ABILITY_NONE : sAbilities[ability]); }
    } WHEN {
        TURN { MOVE(user, mode == 0 ? MOVE_MEGAHORN : mode == 2 ? MOVE_SWIFT : mode == 3 ? MOVE_GROWL : MOVE_TACKLE, hit: mode != 0, WITH_RNG(RNG_CUTE_CHARM, TRUE)); MOVE(target, mode == 1 ? MOVE_PROTECT : MOVE_SPLASH); }
    } SCENE {
        NONE_OF {
            if (mode == 0) ANIMATION(ANIM_TYPE_MOVE, MOVE_MEGAHORN, user);
            ABILITY_POPUP(target, sAbilities[ability]);
        }
    }
}

SINGLE_BATTLE_TEST("Polished volatile contact Mold Breaker does not suppress these abilities but Neutralizing Gas does")
{
    struct BattlePokemon *user = player, *target = opponent;
    u32 ability = 0;
    bool32 gas = FALSE;
    for (u32 a = 0; a < ARRAY_COUNT(sAbilities); a++)
        for (u32 g = 0; g < 2; g++)
            for (u32 side = 0; side < 2; side++) PARAMETRIZE { ability = a; gas = g; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Gender(MON_MALE); Ability(user == player ? gas ? ABILITY_NEUTRALIZING_GAS : ABILITY_MOLD_BREAKER : sAbilities[ability]); }
        OPPONENT(SPECIES_WOBBUFFET) { Gender(MON_FEMALE); Ability(user == opponent ? gas ? ABILITY_NEUTRALIZING_GAS : ABILITY_MOLD_BREAKER : sAbilities[ability]); }
    } WHEN {
        TURN { MOVE(user, MOVE_TACKLE, WITH_RNG(RNG_CUTE_CHARM, TRUE)); MOVE(target, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, user);
        if (gas) NOT ABILITY_POPUP(target, sAbilities[ability]);
        else ABILITY_POPUP(target, sAbilities[ability]);
    }
}

SINGLE_BATTLE_TEST("Polished volatile contact attacker Substitute blocks Tangling Hair but not Cute Charm or Perish Body")
{
    struct BattlePokemon *user = player, *target = opponent;
    u32 ability = 0;
    for (u32 a = 0; a < ARRAY_COUNT(sAbilities); a++)
        for (u32 side = 0; side < 2; side++) PARAMETRIZE { ability = a; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Gender(MON_MALE); Ability(user == player ? ABILITY_NONE : sAbilities[ability]); }
        OPPONENT(SPECIES_WOBBUFFET) { Gender(MON_FEMALE); Ability(user == opponent ? ABILITY_NONE : sAbilities[ability]); }
    } WHEN {
        TURN { MOVE(user, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, MOVE_TACKLE, WITH_RNG(RNG_CUTE_CHARM, TRUE)); MOVE(target, MOVE_SPLASH); }
    } SCENE {
        if (ability == 1) NOT ABILITY_POPUP(target, sAbilities[ability]);
        else ABILITY_POPUP(target, sAbilities[ability]);
    }
}

SINGLE_BATTLE_TEST("Polished volatile contact Cute Charm needs two opposite non-genderless Pokemon")
{
    struct BattlePokemon *user = player, *target = opponent;
    u32 mode = 0;
    for (u32 m = 0; m < 5; m++)
        for (u32 side = 0; side < 2; side++) PARAMETRIZE { mode = m; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER((user == player && mode == 3) || (target == player && mode == 4) ? SPECIES_MAGNEMITE : SPECIES_WOBBUFFET) { Gender((user == player && mode == 3) || (target == player && mode == 4) ? MON_GENDERLESS : mode == 1 ? MON_MALE : mode == 2 ? MON_FEMALE : MON_MALE); Ability(user == player ? ABILITY_NONE : ABILITY_CUTE_CHARM); }
        OPPONENT((user == opponent && mode == 3) || (target == opponent && mode == 4) ? SPECIES_MAGNEMITE : SPECIES_WOBBUFFET) { Gender((user == opponent && mode == 3) || (target == opponent && mode == 4) ? MON_GENDERLESS : mode == 1 ? MON_MALE : MON_FEMALE); Ability(user == opponent ? ABILITY_NONE : ABILITY_CUTE_CHARM); }
    } WHEN {
        TURN { MOVE(user, MOVE_TACKLE, WITH_RNG(RNG_CUTE_CHARM, TRUE)); MOVE(target, MOVE_SPLASH); }
    } THEN {
        EXPECT_EQ(!!user->volatiles.infatuation, mode == 0);
    }
}

SINGLE_BATTLE_TEST("Polished volatile contact Cute Charm respects Oblivious and optional Aroma Veil")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Ability immunity = ABILITY_OBLIVIOUS;
    for (u32 a = 0; a < 2; a++)
        for (u32 side = 0; side < 2; side++) PARAMETRIZE { immunity = a ? ABILITY_AROMA_VEIL : ABILITY_OBLIVIOUS; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Gender(MON_MALE); Ability(user == player ? immunity : ABILITY_CUTE_CHARM); }
        OPPONENT(SPECIES_WOBBUFFET) { Gender(MON_FEMALE); Ability(user == opponent ? immunity : ABILITY_CUTE_CHARM); }
    } WHEN {
        TURN { MOVE(user, MOVE_TACKLE, WITH_RNG(RNG_CUTE_CHARM, TRUE)); MOVE(target, MOVE_SPLASH); }
    } THEN {
        EXPECT_EQ(!!user->volatiles.infatuation, FALSE);
    }
}

SINGLE_BATTLE_TEST("Polished volatile contact Cute Charm keeps Mental Herb curing and Destiny Knot reflection")
{
    struct BattlePokemon *user = player, *target = opponent;
    bool32 knot = FALSE;
    for (u32 item = 0; item < 2; item++)
        for (u32 side = 0; side < 2; side++) PARAMETRIZE { knot = item; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Gender(MON_MALE); Ability(user == player ? ABILITY_NONE : ABILITY_CUTE_CHARM); Item(user == player ? knot ? ITEM_DESTINY_KNOT : ITEM_MENTAL_HERB : ITEM_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Gender(MON_FEMALE); Ability(user == opponent ? ABILITY_NONE : ABILITY_CUTE_CHARM); Item(user == opponent ? knot ? ITEM_DESTINY_KNOT : ITEM_MENTAL_HERB : ITEM_NONE); }
    } WHEN {
        TURN { MOVE(user, MOVE_TACKLE, WITH_RNG(RNG_CUTE_CHARM, TRUE)); MOVE(target, MOVE_SPLASH, WITH_RNG(RNG_INFATUATION, FALSE)); }
    } THEN {
        EXPECT_EQ(!!user->volatiles.infatuation, knot);
        EXPECT_EQ(!!target->volatiles.infatuation, knot);
        EXPECT_EQ(user->item, knot ? ITEM_DESTINY_KNOT : ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished volatile contact Cute Charm does not reactivate while already infatuated")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Gender(MON_MALE); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Gender(MON_FEMALE); Ability(ABILITY_CUTE_CHARM); }
    } WHEN {
        TURN { MOVE(player, MOVE_TACKLE, WITH_RNG(RNG_CUTE_CHARM, TRUE)); MOVE(opponent, MOVE_SPLASH); }
        TURN { MOVE(player, MOVE_TACKLE, WITH_RNG(RNG_INFATUATION, FALSE)); MOVE(opponent, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player);
        ABILITY_POPUP(opponent, ABILITY_CUTE_CHARM);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player);
        NOT ABILITY_POPUP(opponent, ABILITY_CUTE_CHARM);
    }
}

SINGLE_BATTLE_TEST("Polished volatile contact Cute Charm cannot infatuate after its holder faints")
{
    struct BattlePokemon *user = player, *target = opponent;
    for (u32 side = 0; side < 2; side++) PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Gender(MON_MALE); Ability(user == player ? ABILITY_NONE : ABILITY_CUTE_CHARM); if (target == player) HP(1); }
        OPPONENT(SPECIES_WOBBUFFET) { Gender(MON_FEMALE); Ability(user == opponent ? ABILITY_NONE : ABILITY_CUTE_CHARM); if (target == opponent) HP(1); }
    } WHEN {
        TURN { MOVE(user, MOVE_TACKLE, WITH_RNG(RNG_CUTE_CHARM, TRUE)); }
    } SCENE {
        NOT ABILITY_POPUP(target, ABILITY_CUTE_CHARM);
    } THEN {
        EXPECT_EQ(!!user->volatiles.infatuation, FALSE);
    }
}

SINGLE_BATTLE_TEST("Polished volatile contact Tangling Hair retains Contrary Simple Defiant and stat immunity behavior")
{
    static const enum Ability reactions[] = {ABILITY_CONTRARY, ABILITY_SIMPLE, ABILITY_DEFIANT, ABILITY_COMPETITIVE, ABILITY_CLEAR_BODY, ABILITY_WHITE_SMOKE};
    struct BattlePokemon *user = player, *target = opponent;
    u32 mode = 0;
    for (u32 m = 0; m < ARRAY_COUNT(reactions); m++)
        for (u32 side = 0; side < 2; side++) PARAMETRIZE { mode = m; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? reactions[mode] : ABILITY_TANGLING_HAIR); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? reactions[mode] : ABILITY_TANGLING_HAIR); }
    } WHEN {
        TURN { MOVE(user, MOVE_TACKLE); MOVE(target, MOVE_SPLASH); }
    } THEN {
        EXPECT_EQ(user->statStages[STAT_SPEED], mode == 0 ? DEFAULT_STAT_STAGE + 1 : mode == 1 ? DEFAULT_STAT_STAGE - 2 : mode < 4 ? DEFAULT_STAT_STAGE - 1 : DEFAULT_STAT_STAGE);
        EXPECT_EQ(user->statStages[STAT_ATK], mode == 2 ? DEFAULT_STAT_STAGE + 2 : DEFAULT_STAT_STAGE);
        EXPECT_EQ(user->statStages[STAT_SPATK], mode == 3 ? DEFAULT_STAT_STAGE + 2 : DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished volatile contact Tangling Hair respects Mist Clear Amulet and White Herb")
{
    struct BattlePokemon *user = player, *target = opponent;
    u32 mode = 0;
    for (u32 m = 0; m < 3; m++)
        for (u32 side = 0; side < 2; side++) PARAMETRIZE { mode = m; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_NONE : ABILITY_TANGLING_HAIR); Item(user == player ? mode == 1 ? ITEM_CLEAR_AMULET : mode == 2 ? ITEM_WHITE_HERB : ITEM_NONE : ITEM_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_NONE : ABILITY_TANGLING_HAIR); Item(user == opponent ? mode == 1 ? ITEM_CLEAR_AMULET : mode == 2 ? ITEM_WHITE_HERB : ITEM_NONE : ITEM_NONE); }
    } WHEN {
        if (mode == 0) TURN { MOVE(user, MOVE_MIST); }
        TURN { MOVE(user, MOVE_TACKLE); MOVE(target, MOVE_SPLASH); }
    } THEN {
        EXPECT_EQ(user->statStages[STAT_SPEED], DEFAULT_STAT_STAGE);
        if (mode == 2) EXPECT_EQ(user->item, ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished volatile contact Perish Body ignores Soundproof and counts down on the fourth turn")
{
    struct BattlePokemon *user = player, *target = opponent;
    u32 turns = 1;
    for (u32 n = 1; n <= 4; n++)
        for (u32 side = 0; side < 2; side++) PARAMETRIZE { turns = n; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_SOUNDPROOF : ABILITY_PERISH_BODY); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_SOUNDPROOF : ABILITY_PERISH_BODY); Speed(50); }
    } WHEN {
        TURN { MOVE(user, MOVE_TACKLE); MOVE(target, MOVE_SPLASH); }
        for (u32 n = 1; n < turns; n++) TURN {}
    } THEN {
        if (turns == 4) { EXPECT_EQ(user->hp, 0); EXPECT_EQ(target->hp, 0); }
        else {
            EXPECT_EQ(!!user->volatiles.perishSong, TRUE);
            EXPECT_EQ(!!target->volatiles.perishSong, TRUE);
            EXPECT_EQ((u32)user->volatiles.perishSongTimer, 3 - turns);
            EXPECT_EQ((u32)target->volatiles.perishSongTimer, 3 - turns);
        }
    }
}

SINGLE_BATTLE_TEST("Polished volatile contact Perish Body repeats do not refresh either countdown")
{
    struct BattlePokemon *user = player, *target = opponent;
    for (u32 side = 0; side < 2; side++) PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_NONE : ABILITY_PERISH_BODY); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_NONE : ABILITY_PERISH_BODY); }
    } WHEN {
        TURN { MOVE(user, MOVE_TACKLE); MOVE(target, MOVE_SPLASH); }
        TURN { MOVE(user, MOVE_TACKLE); MOVE(target, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, user);
        ABILITY_POPUP(target, ABILITY_PERISH_BODY);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, user);
        NOT ABILITY_POPUP(target, ABILITY_PERISH_BODY);
    } THEN {
        EXPECT_EQ((u32)user->volatiles.perishSongTimer, 1);
        EXPECT_EQ((u32)target->volatiles.perishSongTimer, 1);
    }
}

SINGLE_BATTLE_TEST("Polished volatile contact Perish Body can start only the attacker's missing countdown")
{
    struct BattlePokemon *user = player, *target = opponent;
    for (u32 side = 0; side < 2; side++) PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_SOUNDPROOF : ABILITY_PERISH_BODY); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_SOUNDPROOF : ABILITY_PERISH_BODY); }
    } WHEN {
        TURN { MOVE(target, MOVE_PERISH_SONG); }
        TURN { MOVE(user, MOVE_TACKLE); MOVE(target, MOVE_SPLASH); }
    } THEN {
        EXPECT_EQ((u32)user->volatiles.perishSongTimer, 2);
        EXPECT_EQ((u32)target->volatiles.perishSongTimer, 1);
    }
}

SINGLE_BATTLE_TEST("Polished volatile contact Perish Body does not add a holder count if the attacker is already counted")
{
    struct BattlePokemon *user = player, *target = opponent;
    for (u32 side = 0; side < 2; side++) PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_PERISH_BODY); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_PERISH_BODY); }
    } WHEN {
        TURN { MOVE(user, MOVE_PERISH_SONG); }
        TURN { SWITCH(target, 1); }
        TURN { MOVE(user, MOVE_TACKLE); MOVE(target, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, user);
        NOT ABILITY_POPUP(target, ABILITY_PERISH_BODY);
    } THEN {
        EXPECT_EQ((u32)user->volatiles.perishSongTimer, 0);
        EXPECT_EQ(!!target->volatiles.perishSong, FALSE);
    }
}

SINGLE_BATTLE_TEST("Polished volatile contact switching clears the attacker count and permits a new Perish Body activation")
{
    struct BattlePokemon *user = player, *target = opponent;
    for (u32 side = 0; side < 2; side++) PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_NONE : ABILITY_PERISH_BODY); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_NONE : ABILITY_PERISH_BODY); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(user, MOVE_TACKLE); MOVE(target, MOVE_SPLASH); }
        TURN { SWITCH(user, 1); }
        TURN { MOVE(user, MOVE_TACKLE); MOVE(target, MOVE_SPLASH); }
    } THEN {
        EXPECT_EQ((u32)user->volatiles.perishSongTimer, 2);
        EXPECT_EQ((u32)target->volatiles.perishSongTimer, 0);
    }
}

SINGLE_BATTLE_TEST("Polished volatile contact Double Kick reevaluates after a Substitute breaks")
{
    struct BattlePokemon *user = player, *target = opponent;
    u32 ability = 0;
    for (u32 a = 0; a < ARRAY_COUNT(sAbilities); a++)
        for (u32 side = 0; side < 2; side++) PARAMETRIZE { ability = a; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Gender(MON_MALE); Level(50); Attack(300); Defense(100); MaxHP(1000); HP(1000); Ability(user == player ? ABILITY_NONE : sAbilities[ability]); }
        OPPONENT(SPECIES_WOBBUFFET) { Gender(MON_FEMALE); Level(50); Attack(300); Defense(100); MaxHP(1000); HP(1000); Ability(user == opponent ? ABILITY_NONE : sAbilities[ability]); }
    } WHEN {
        TURN { MOVE(target, MOVE_SUBSTITUTE); }
        for (u32 n = 0; n < 4; n++) TURN { MOVE(user, MOVE_SEISMIC_TOSS); }
        TURN { MOVE(user, MOVE_DRAGON_RAGE); }
        TURN { MOVE(user, MOVE_DOUBLE_KICK, WITH_RNG(RNG_CUTE_CHARM, TRUE)); MOVE(target, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_DOUBLE_KICK, user);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_SUBSTITUTE_FADE, target);
        if (B_POLISHED_MOVE_BALANCE) ABILITY_POPUP(target, sAbilities[ability]);
        HP_BAR(target);
        if (!B_POLISHED_MOVE_BALANCE || ability == 1) ABILITY_POPUP(target, sAbilities[ability]);
        MESSAGE("The Pokémon was hit 2 times!");
    } THEN {
        if (ability == 1) EXPECT_EQ(user->statStages[STAT_SPEED], DEFAULT_STAT_STAGE - (B_POLISHED_MOVE_BALANCE ? 2 : 1));
    }
}

SINGLE_BATTLE_TEST("Polished volatile contact Poison Touch precedes the defender ability")
{
    struct BattlePokemon *user = player, *target = opponent;
    u32 ability = 0;
    for (u32 a = 0; a < ARRAY_COUNT(sAbilities); a++)
        for (u32 side = 0; side < 2; side++) PARAMETRIZE { ability = a; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Gender(MON_MALE); Ability(user == player ? ABILITY_POISON_TOUCH : sAbilities[ability]); }
        OPPONENT(SPECIES_WOBBUFFET) { Gender(MON_FEMALE); Ability(user == opponent ? ABILITY_POISON_TOUCH : sAbilities[ability]); }
    } WHEN {
        TURN { MOVE(user, MOVE_TACKLE, WITH_RNG(RNG_POISON_TOUCH, TRUE)); MOVE(target, MOVE_SPLASH); }
    } SCENE {
        if (B_POLISHED_MOVE_BALANCE) { ABILITY_POPUP(user, ABILITY_POISON_TOUCH); ABILITY_POPUP(target, sAbilities[ability]); }
        else { ABILITY_POPUP(target, sAbilities[ability]); ABILITY_POPUP(user, ABILITY_POISON_TOUCH); }
    }
}

SINGLE_BATTLE_TEST("Polished volatile contact optional moves retain native knockout gates")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Ability ability = ABILITY_TANGLING_HAIR;
    for (u32 a = 1; a < ARRAY_COUNT(sAbilities); a++)
        for (u32 side = 0; side < 2; side++) PARAMETRIZE { ability = sAbilities[a]; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_NONE : ability); if (target == player) HP(1); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_NONE : ability); if (target == opponent) HP(1); }
    } WHEN {
        TURN { MOVE(user, MOVE_POUND); }
    } SCENE {
        if (ability == ABILITY_TANGLING_HAIR) ABILITY_POPUP(target, ability);
        else NOT ABILITY_POPUP(target, ability);
    }
}

SINGLE_BATTLE_TEST("Polished volatile contact either active gimmick retains native Substitute gates")
{
    struct BattlePokemon *user = player, *target = opponent;
    u32 ability = 0;
    bool32 targetTera = FALSE;
    for (u32 a = 0; a < ARRAY_COUNT(sAbilities); a++)
        for (u32 who = 0; who < 2; who++)
            for (u32 side = 0; side < 2; side++) PARAMETRIZE { ability = a; targetTera = who; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Gender(MON_MALE); Level(50); MaxHP(200); HP(200); TeraType(TYPE_NORMAL); Ability(user == player ? ABILITY_NONE : sAbilities[ability]); }
        OPPONENT(SPECIES_WOBBUFFET) { Gender(MON_FEMALE); Level(50); MaxHP(200); HP(200); TeraType(TYPE_NORMAL); Ability(user == opponent ? ABILITY_NONE : sAbilities[ability]); }
    } WHEN {
        TURN { MOVE(target, MOVE_SUBSTITUTE, gimmick: targetTera ? GIMMICK_TERA : GIMMICK_NONE); }
        TURN { MOVE(user, MOVE_SEISMIC_TOSS, gimmick: targetTera ? GIMMICK_NONE : GIMMICK_TERA, WITH_RNG(RNG_CUTE_CHARM, TRUE)); MOVE(target, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_SUBSTITUTE_FADE, target);
        NOT ABILITY_POPUP(target, sAbilities[ability]);
    }
}

DOUBLE_BATTLE_TEST("Polished volatile contact doubles retain native Substitute gates")
{
    struct BattlePokemon *user = playerLeft, *target = opponentLeft;
    u32 ability = 0;
    for (u32 a = 0; a < ARRAY_COUNT(sAbilities); a++)
        for (u32 side = 0; side < 2; side++) PARAMETRIZE { ability = a; user = side ? opponentLeft : playerLeft; target = side ? playerLeft : opponentLeft; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Gender(MON_MALE); Level(50); MaxHP(200); HP(200); Ability(user == playerLeft ? ABILITY_NONE : sAbilities[ability]); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Gender(MON_FEMALE); Level(50); MaxHP(200); HP(200); Ability(user == opponentLeft ? ABILITY_NONE : sAbilities[ability]); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(target, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, MOVE_SEISMIC_TOSS, target: target, WITH_RNG(RNG_CUTE_CHARM, TRUE)); MOVE(target, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_SUBSTITUTE_FADE, target);
        NOT ABILITY_POPUP(target, sAbilities[ability]);
    }
}

SINGLE_BATTLE_TEST("Polished volatile contact optional Gooey retains native knockout behavior")
{
    struct BattlePokemon *user = player, *target = opponent;
    for (u32 side = 0; side < 2; side++) PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_NONE : ABILITY_GOOEY); if (target == player) HP(1); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_NONE : ABILITY_GOOEY); if (target == opponent) HP(1); }
    } WHEN {
        TURN { MOVE(user, MOVE_TACKLE); }
    } SCENE {
        ABILITY_POPUP(target, ABILITY_GOOEY);
    } THEN {
        EXPECT_EQ(user->statStages[STAT_SPEED], DEFAULT_STAT_STAGE - 1);
    }
}

SINGLE_BATTLE_TEST("Polished volatile contact optional moves and either active gimmick retain native Cute Charm probability")
{
    struct BattlePokemon *user = player, *target = opponent;
    u32 mode = 0;
    for (u32 m = 0; m < 3; m++)
        for (u32 side = 0; side < 2; side++) PARAMETRIZE { mode = m; user = side ? opponent : player; target = side ? player : opponent; }
    PASSES_RANDOMLY(1, 3, RNG_CUTE_CHARM);
    GIVEN {
        WITH_CONFIG(B_ABILITY_TRIGGER_CHANCE, GEN_3);
        PLAYER(SPECIES_WOBBUFFET) { Gender(MON_MALE); TeraType(TYPE_NORMAL); Ability(user == player ? ABILITY_NONE : ABILITY_CUTE_CHARM); }
        OPPONENT(SPECIES_WOBBUFFET) { Gender(MON_FEMALE); TeraType(TYPE_NORMAL); Ability(user == opponent ? ABILITY_NONE : ABILITY_CUTE_CHARM); }
    } WHEN {
        TURN { MOVE(user, mode == 0 ? MOVE_POUND : MOVE_TACKLE, gimmick: mode == 1 ? GIMMICK_TERA : GIMMICK_NONE); MOVE(target, MOVE_SPLASH, gimmick: mode == 2 ? GIMMICK_TERA : GIMMICK_NONE); }
    } SCENE {
        ABILITY_POPUP(target, ABILITY_CUTE_CHARM);
    }
}

DOUBLE_BATTLE_TEST("Polished volatile contact doubles retain native Cute Charm probability")
{
    struct BattlePokemon *user = playerLeft, *target = opponentLeft;
    for (u32 side = 0; side < 2; side++) PARAMETRIZE { user = side ? opponentLeft : playerLeft; target = side ? playerLeft : opponentLeft; }
    PASSES_RANDOMLY(1, 3, RNG_CUTE_CHARM);
    GIVEN {
        WITH_CONFIG(B_ABILITY_TRIGGER_CHANCE, GEN_3);
        PLAYER(SPECIES_WOBBUFFET) { Gender(MON_MALE); Ability(user == playerLeft ? ABILITY_NONE : ABILITY_CUTE_CHARM); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Gender(MON_FEMALE); Ability(user == opponentLeft ? ABILITY_NONE : ABILITY_CUTE_CHARM); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(user, MOVE_TACKLE, target: target); MOVE(target, MOVE_SPLASH); }
    } SCENE {
        ABILITY_POPUP(target, ABILITY_CUTE_CHARM);
    }
}

SINGLE_BATTLE_TEST("Polished volatile contact Tangling Hair respects both Speed stage limits and Contrary")
{
    struct BattlePokemon *user = player, *target = opponent;
    u32 mode = 0;
    for (u32 m = 0; m < 3; m++)
        for (u32 side = 0; side < 2; side++) PARAMETRIZE { mode = m; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? mode == 2 ? ABILITY_CONTRARY : ABILITY_NONE : ABILITY_TANGLING_HAIR); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? mode == 2 ? ABILITY_CONTRARY : ABILITY_NONE : ABILITY_TANGLING_HAIR); }
    } WHEN {
        for (u32 n = 0; n < 3; n++) {
            TURN { if (mode == 1) MOVE(user, MOVE_AGILITY); else MOVE(target, MOVE_SCARY_FACE); }
        }
        TURN { MOVE(user, MOVE_TACKLE); MOVE(target, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, user);
        if (mode == 1) ABILITY_POPUP(target, ABILITY_TANGLING_HAIR);
        else NOT ABILITY_POPUP(target, ABILITY_TANGLING_HAIR);
    } THEN {
        EXPECT_EQ(user->statStages[STAT_SPEED], mode == 0 ? MIN_STAT_STAGE : mode == 1 ? MAX_STAT_STAGE - 1 : MAX_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished volatile contact Perish Body countdown survives its holder being replaced after knockout")
{
    struct BattlePokemon *user = player, *target = opponent;
    for (u32 side = 0; side < 2; side++) PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_NONE : ABILITY_PERISH_BODY); if (target == player) HP(1); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_NONE : ABILITY_PERISH_BODY); if (target == opponent) HP(1); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(user, MOVE_TACKLE); SEND_OUT(target, 1); }
        TURN {}
    } THEN {
        EXPECT_EQ(!!user->volatiles.perishSong, B_POLISHED_MOVE_BALANCE);
        if (B_POLISHED_MOVE_BALANCE) EXPECT_EQ((u32)user->volatiles.perishSongTimer, 1);
        EXPECT_EQ(!!target->volatiles.perishSong, FALSE);
    }
}
