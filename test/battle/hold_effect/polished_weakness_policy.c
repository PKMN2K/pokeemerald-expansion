#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished Weakness Policy precedes ordinary hit abilities and their Mirror Herb checkpoint")
{
    struct BattlePokemon *holder = player, *attacker = opponent;
    enum Ability ability = ABILITY_JUSTIFIED;
    bool32 contrary = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 armor = 0; armor < 2; armor++)
            for (u32 inverse = 0; inverse < 2; inverse++)
                PARAMETRIZE { holder = side ? opponent : player; attacker = side ? player : opponent; ability = armor ? ABILITY_WEAK_ARMOR : ABILITY_JUSTIFIED; contrary = inverse; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(holder == player ? ability : contrary ? ABILITY_CONTRARY : ABILITY_BATTLE_ARMOR); Speed(attacker == player ? 200 : 100); HP(4000); MaxHP(4000); Attack(100); Defense(1000); Item(holder == player ? ITEM_WEAKNESS_POLICY : ITEM_MIRROR_HERB); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(holder == opponent ? ability : contrary ? ABILITY_CONTRARY : ABILITY_BATTLE_ARMOR); Speed(attacker == opponent ? 200 : 100); HP(4000); MaxHP(4000); Attack(100); Defense(1000); Item(holder == opponent ? ITEM_WEAKNESS_POLICY : ITEM_MIRROR_HERB); }
    } WHEN { TURN { MOVE(attacker, MOVE_FEINT_ATTACK, criticalHit: FALSE); MOVE(holder, MOVE_SPLASH); } }
    SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_FEINT_ATTACK, attacker);
        HP_BAR(holder);
        if (B_POLISHED_MOVE_BALANCE) ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, holder);
        ABILITY_POPUP(holder, ability);
        if (!B_POLISHED_MOVE_BALANCE) ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, holder);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, attacker);
    } THEN {
        s32 direction = contrary ? -1 : 1;
        EXPECT_EQ(holder->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 2 + (ability == ABILITY_JUSTIFIED));
        EXPECT_EQ(holder->statStages[STAT_SPATK], DEFAULT_STAT_STAGE + 2);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + 2 * (ability == ABILITY_WEAK_ARMOR));
        EXPECT_EQ(attacker->statStages[STAT_ATK], DEFAULT_STAT_STAGE + direction * (2 + (ability == ABILITY_JUSTIFIED)));
        EXPECT_EQ(attacker->statStages[STAT_SPATK], DEFAULT_STAT_STAGE + direction * 2);
        EXPECT_EQ(attacker->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + direction * 2 * (ability == ABILITY_WEAK_ARMOR));
        EXPECT_EQ(holder->item, ITEM_NONE);
        EXPECT_EQ(attacker->item, ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Weakness Policy follows the final ordinary multihit ability checkpoint")
{
    struct BattlePokemon *holder = player, *attacker = opponent;
    PARAMETRIZE { holder = player; attacker = opponent; }
    PARAMETRIZE { holder = opponent; attacker = player; }
    GIVEN {
        PLAYER(holder == player ? SPECIES_SNEASEL : SPECIES_WOBBUFFET) { Ability(holder == player ? ABILITY_WEAK_ARMOR : ABILITY_BATTLE_ARMOR); Speed(attacker == player ? 200 : 100); HP(4000); MaxHP(4000); Attack(100); Defense(1000); Item(holder == player ? ITEM_WEAKNESS_POLICY : ITEM_MIRROR_HERB); }
        OPPONENT(holder == opponent ? SPECIES_SNEASEL : SPECIES_WOBBUFFET) { Ability(holder == opponent ? ABILITY_WEAK_ARMOR : ABILITY_BATTLE_ARMOR); Speed(attacker == opponent ? 200 : 100); HP(4000); MaxHP(4000); Attack(100); Defense(1000); Item(holder == opponent ? ITEM_WEAKNESS_POLICY : ITEM_MIRROR_HERB); }
    } WHEN { TURN { MOVE(attacker, MOVE_DOUBLE_KICK, criticalHit: FALSE); MOVE(holder, MOVE_SPLASH); } }
    SCENE {
        HP_BAR(holder);
        ABILITY_POPUP(holder, ABILITY_WEAK_ARMOR);
        if (B_POLISHED_MOVE_BALANCE) ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, attacker);
        else ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, holder);
        HP_BAR(holder);
        ABILITY_POPUP(holder, ABILITY_WEAK_ARMOR);
        if (B_POLISHED_MOVE_BALANCE) ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, holder);
        else ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, attacker);
    } THEN {
        EXPECT_EQ(holder->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 2);
        EXPECT_EQ(holder->statStages[STAT_SPATK], DEFAULT_STAT_STAGE + 2);
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + 4);
        EXPECT_EQ(attacker->statStages[STAT_ATK], DEFAULT_STAT_STAGE + (B_POLISHED_MOVE_BALANCE ? 0 : 2));
        EXPECT_EQ(attacker->statStages[STAT_SPATK], DEFAULT_STAT_STAGE + (B_POLISHED_MOVE_BALANCE ? 0 : 2));
        EXPECT_EQ(attacker->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + (B_POLISHED_MOVE_BALANCE ? 2 : 4));
        EXPECT_EQ(holder->item, ITEM_NONE);
        EXPECT_EQ(attacker->item, ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Weakness Policy cannot retry after a secondary reopens a capped stat")
{
    struct BattlePokemon *holder = player, *attacker = opponent;
    bool32 contrary = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 inverse = 0; inverse < 2; inverse++)
            PARAMETRIZE { holder = side ? opponent : player; attacker = side ? player : opponent; contrary = inverse; }
    GIVEN {
        PLAYER(holder == player ? SPECIES_DRAGONITE : SPECIES_WOBBUFFET) { Ability(holder == player && contrary ? ABILITY_CONTRARY : ABILITY_BATTLE_ARMOR); Speed(attacker == player ? 200 : 100); HP(4000); MaxHP(4000); SpAttack(100); SpDefense(1000); Item(holder == player ? ITEM_WEAKNESS_POLICY : ITEM_NONE); }
        OPPONENT(holder == opponent ? SPECIES_DRAGONITE : SPECIES_WOBBUFFET) { Ability(holder == opponent && contrary ? ABILITY_CONTRARY : ABILITY_BATTLE_ARMOR); Speed(attacker == opponent ? 200 : 100); HP(4000); MaxHP(4000); SpAttack(100); SpDefense(1000); Item(holder == opponent ? ITEM_WEAKNESS_POLICY : ITEM_NONE); }
    } WHEN {
        for (u32 use = 0; use < 3; use++) {
            TURN { MOVE(holder, MOVE_SWORDS_DANCE); MOVE(attacker, MOVE_SPLASH); }
            TURN { MOVE(holder, MOVE_NASTY_PLOT); MOVE(attacker, MOVE_SPLASH); }
        }
        TURN { MOVE(attacker, MOVE_MOONBLAST, secondaryEffect: TRUE, criticalHit: FALSE); MOVE(holder, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_MOONBLAST, attacker);
        HP_BAR(holder);
        if (B_POLISHED_MOVE_BALANCE) NOT ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, holder);
        else ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, holder);
    } THEN {
        s32 cap = contrary ? MIN_STAT_STAGE : MAX_STAT_STAGE;
        EXPECT_EQ(holder->statStages[STAT_ATK], cap);
        EXPECT_EQ(holder->statStages[STAT_SPATK], cap + (B_POLISHED_MOVE_BALANCE ? contrary ? 1 : -1 : 0));
        EXPECT_EQ(holder->item, B_POLISHED_MOVE_BALANCE ? ITEM_WEAKNESS_POLICY : ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Weakness Policy accepts super effective False Swipe with zero HP loss")
{
    struct BattlePokemon *holder = player, *attacker = opponent;
    bool32 tera = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 optional = 0; optional < 2; optional++)
            PARAMETRIZE { holder = side ? opponent : player; attacker = side ? player : opponent; tera = optional; }
    GIVEN {
        PLAYER(holder == player ? SPECIES_MILOTIC : SPECIES_WOBBUFFET) { Ability(holder == player ? ABILITY_BATTLE_ARMOR : ABILITY_GALVANIZE); Speed(attacker == player ? 200 : 100); HP(holder == player ? 1 : 4000); MaxHP(4000); Attack(100); Defense(1000); Item(holder == player ? ITEM_WEAKNESS_POLICY : ITEM_NONE); TeraType(TYPE_ELECTRIC); }
        OPPONENT(holder == opponent ? SPECIES_MILOTIC : SPECIES_WOBBUFFET) { Ability(holder == opponent ? ABILITY_BATTLE_ARMOR : ABILITY_GALVANIZE); Speed(attacker == opponent ? 200 : 100); HP(holder == opponent ? 1 : 4000); MaxHP(4000); Attack(100); Defense(1000); Item(holder == opponent ? ITEM_WEAKNESS_POLICY : ITEM_NONE); TeraType(TYPE_ELECTRIC); }
    } WHEN { TURN { MOVE(attacker, MOVE_FALSE_SWIPE, gimmick: tera ? GIMMICK_TERA : GIMMICK_NONE); MOVE(holder, MOVE_SPLASH); } }
    SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_FALSE_SWIPE, attacker);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, holder);
    } THEN {
        // Native marks an attempted zero-damage hit as damagedByAttack, so its
        // retained item path also activates; source does not need that marker.
        EXPECT_EQ(holder->hp, 1);
        EXPECT_EQ(holder->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 2);
        EXPECT_EQ(holder->statStages[STAT_SPATK], DEFAULT_STAT_STAGE + 2);
        EXPECT_EQ(holder->item, ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Weakness Policy uses the first Parental Bond strike before its hit ability")
{
    struct BattlePokemon *holder = player, *attacker = opponent;
    PARAMETRIZE { holder = player; attacker = opponent; }
    PARAMETRIZE { holder = opponent; attacker = player; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(holder == player ? ABILITY_JUSTIFIED : ABILITY_PARENTAL_BOND); Speed(attacker == player ? 200 : 100); HP(4000); MaxHP(4000); Attack(100); Defense(1000); Item(holder == player ? ITEM_WEAKNESS_POLICY : ITEM_MIRROR_HERB); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(holder == opponent ? ABILITY_JUSTIFIED : ABILITY_PARENTAL_BOND); Speed(attacker == opponent ? 200 : 100); HP(4000); MaxHP(4000); Attack(100); Defense(1000); Item(holder == opponent ? ITEM_WEAKNESS_POLICY : ITEM_MIRROR_HERB); }
    } WHEN { TURN { MOVE(attacker, MOVE_FEINT_ATTACK, criticalHit: FALSE); MOVE(holder, MOVE_SPLASH); } }
    SCENE {
        HP_BAR(holder);
        if (B_POLISHED_MOVE_BALANCE) ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, holder);
        ABILITY_POPUP(holder, ABILITY_JUSTIFIED);
        if (B_POLISHED_MOVE_BALANCE) ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, attacker);
        else ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, holder);
        HP_BAR(holder);
        ABILITY_POPUP(holder, ABILITY_JUSTIFIED);
        if (!B_POLISHED_MOVE_BALANCE) ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, attacker);
    } THEN {
        EXPECT_EQ(holder->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 4);
        EXPECT_EQ(holder->statStages[STAT_SPATK], DEFAULT_STAT_STAGE + 2);
        EXPECT_EQ(attacker->statStages[STAT_ATK], DEFAULT_STAT_STAGE + (B_POLISHED_MOVE_BALANCE ? 3 : 4));
        EXPECT_EQ(attacker->statStages[STAT_SPATK], DEFAULT_STAT_STAGE + 2);
        EXPECT_EQ(holder->item, ITEM_NONE);
        EXPECT_EQ(attacker->item, ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Weakness Policy applies source caps at the delayed Future Sight checkpoint")
{
    struct BattlePokemon *holder = player, *attacker = opponent;
    bool32 capped = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 cap = 0; cap < 2; cap++)
            PARAMETRIZE { holder = side ? opponent : player; attacker = side ? player : opponent; capped = cap; }
    GIVEN {
        PLAYER(holder == player ? SPECIES_MACHAMP : SPECIES_WOBBUFFET) { Ability(ABILITY_BATTLE_ARMOR); Speed(attacker == player ? 200 : 100); HP(4000); MaxHP(4000); SpAttack(100); SpDefense(1000); Item(holder == player ? ITEM_WEAKNESS_POLICY : ITEM_NONE); }
        OPPONENT(holder == opponent ? SPECIES_MACHAMP : SPECIES_WOBBUFFET) { Ability(ABILITY_BATTLE_ARMOR); Speed(attacker == opponent ? 200 : 100); HP(4000); MaxHP(4000); SpAttack(100); SpDefense(1000); Item(holder == opponent ? ITEM_WEAKNESS_POLICY : ITEM_NONE); }
    } WHEN {
        if (capped) for (u32 use = 0; use < 3; use++) {
            TURN { MOVE(holder, MOVE_SWORDS_DANCE); MOVE(attacker, MOVE_SPLASH); }
            TURN { MOVE(holder, MOVE_NASTY_PLOT); MOVE(attacker, MOVE_SPLASH); }
        }
        TURN { MOVE(attacker, MOVE_FUTURE_SIGHT); MOVE(holder, MOVE_SPLASH); }
        TURN { MOVE(attacker, MOVE_SPLASH); MOVE(holder, MOVE_SPLASH); }
        TURN { MOVE(attacker, MOVE_SPLASH); MOVE(holder, MOVE_SPLASH); }
    } SCENE {
        HP_BAR(holder);
        if (B_POLISHED_MOVE_BALANCE && capped) NOT ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, holder);
        else ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, holder);
    } THEN {
        EXPECT_LT(holder->hp, 4000);
        EXPECT_GT(holder->hp, 0);
        EXPECT_EQ(holder->statStages[STAT_ATK], capped ? MAX_STAT_STAGE : DEFAULT_STAT_STAGE + 2);
        EXPECT_EQ(holder->statStages[STAT_SPATK], capped ? MAX_STAT_STAGE : DEFAULT_STAT_STAGE + 2);
        EXPECT_EQ(holder->item, B_POLISHED_MOVE_BALANCE && capped ? ITEM_WEAKNESS_POLICY : ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Weakness Policy reaches the early checkpoint after Explosion faints its user")
{
    struct BattlePokemon *holder = player, *attacker = opponent;
    PARAMETRIZE { holder = player; attacker = opponent; }
    PARAMETRIZE { holder = opponent; attacker = player; }
    GIVEN {
        PLAYER(holder == player ? SPECIES_MILOTIC : SPECIES_WOBBUFFET) { Ability(holder == player ? ABILITY_BATTLE_ARMOR : ABILITY_GALVANIZE); Speed(attacker == player ? 200 : 100); HP(4000); MaxHP(4000); Attack(100); Defense(1000); Item(holder == player ? ITEM_WEAKNESS_POLICY : ITEM_NONE); }
        if (attacker == player) PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_BATTLE_ARMOR); Speed(50); }
        OPPONENT(holder == opponent ? SPECIES_MILOTIC : SPECIES_WOBBUFFET) { Ability(holder == opponent ? ABILITY_BATTLE_ARMOR : ABILITY_GALVANIZE); Speed(attacker == opponent ? 200 : 100); HP(4000); MaxHP(4000); Attack(100); Defense(1000); Item(holder == opponent ? ITEM_WEAKNESS_POLICY : ITEM_NONE); }
        if (attacker == opponent) OPPONENT(SPECIES_CHANSEY) { Ability(ABILITY_BATTLE_ARMOR); Speed(50); }
    } WHEN {
        TURN { MOVE(attacker, MOVE_EXPLOSION, criticalHit: FALSE); MOVE(holder, MOVE_SPLASH); SEND_OUT(attacker, 1); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_EXPLOSION, attacker);
        HP_BAR(holder);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, holder);
    } THEN {
        EXPECT_LT(holder->hp, 4000);
        EXPECT_GT(holder->hp, 0);
        EXPECT_EQ(holder->item, ITEM_NONE);
        EXPECT_EQ(holder->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 2);
        EXPECT_EQ(holder->statStages[STAT_SPATK], DEFAULT_STAT_STAGE + 2);
        EXPECT_EQ(GetMonData(&gParties[attacker == player ? B_TRAINER_PLAYER : B_TRAINER_OPPONENT_A][0], MON_DATA_HP), 0);
        EXPECT_EQ(attacker->species, SPECIES_CHANSEY);
    }
}

SINGLE_BATTLE_TEST("Polished Weakness Policy skips a late multihit checkpoint after contact knocks out its user")
{
    struct BattlePokemon *holder = player, *attacker = opponent;
    bool32 multihit = FALSE, lethal = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 multi = 0; multi < 2; multi++)
            for (u32 ko = 0; ko < 2; ko++)
                PARAMETRIZE { holder = side ? opponent : player; attacker = side ? player : opponent; multihit = multi; lethal = ko; }
    u32 hp = (multihit ? 13 : 1) + (lethal ? 0 : 12);
    bool32 activates = !B_POLISHED_MOVE_BALANCE || !multihit || !lethal;
    GIVEN {
        WITH_CONFIG(B_ROUGH_SKIN_DMG, GEN_4);
        PLAYER(holder == player ? SPECIES_SNEASEL : SPECIES_WOBBUFFET) { Ability(holder == player ? ABILITY_ROUGH_SKIN : ABILITY_BATTLE_ARMOR); Speed(attacker == player ? 200 : 100); HP(holder == player ? 4000 : hp); MaxHP(holder == player ? 4000 : 100); Attack(100); Defense(1000); Item(holder == player ? ITEM_WEAKNESS_POLICY : ITEM_NONE); }
        if (attacker == player) PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_BATTLE_ARMOR); Speed(50); }
        OPPONENT(holder == opponent ? SPECIES_SNEASEL : SPECIES_WOBBUFFET) { Ability(holder == opponent ? ABILITY_ROUGH_SKIN : ABILITY_BATTLE_ARMOR); Speed(attacker == opponent ? 200 : 100); HP(holder == opponent ? 4000 : hp); MaxHP(holder == opponent ? 4000 : 100); Attack(100); Defense(1000); Item(holder == opponent ? ITEM_WEAKNESS_POLICY : ITEM_NONE); }
        if (attacker == opponent) OPPONENT(SPECIES_CHANSEY) { Ability(ABILITY_BATTLE_ARMOR); Speed(50); }
    } WHEN {
        TURN { MOVE(attacker, multihit ? MOVE_DOUBLE_KICK : MOVE_LOW_KICK, criticalHit: FALSE, secondaryEffect: FALSE); MOVE(holder, MOVE_SPLASH); if (lethal) SEND_OUT(attacker, 1); }
    } SCENE {
        HP_BAR(holder);
        if (B_POLISHED_MOVE_BALANCE && !multihit) ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, holder);
        ABILITY_POPUP(holder, ABILITY_ROUGH_SKIN);
        HP_BAR(attacker, damage: !multihit && lethal ? 1 : 12);
        if (!B_POLISHED_MOVE_BALANCE) ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, holder);
        if (multihit) {
            HP_BAR(holder);
            ABILITY_POPUP(holder, ABILITY_ROUGH_SKIN);
            HP_BAR(attacker, damage: lethal ? 1 : 12);
            if (B_POLISHED_MOVE_BALANCE && !lethal) ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, holder);
            else if (B_POLISHED_MOVE_BALANCE) NOT ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, holder);
        }
    } THEN {
        EXPECT_EQ(holder->item, activates ? ITEM_NONE : ITEM_WEAKNESS_POLICY);
        EXPECT_EQ(holder->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 2 * activates);
        EXPECT_EQ(holder->statStages[STAT_SPATK], DEFAULT_STAT_STAGE + 2 * activates);
        if (lethal) EXPECT_EQ(attacker->species, SPECIES_CHANSEY);
        else EXPECT_EQ(attacker->hp, 1);
    }
}
