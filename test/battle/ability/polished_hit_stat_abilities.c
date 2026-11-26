#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished hit stats Justified Rattled and Weak Armor activate on the exact Substitute break")
{
    struct BattlePokemon *holder = player, *attacker = opponent;
    enum Ability ability = ABILITY_JUSTIFIED;
    enum Move move = MOVE_FEINT_ATTACK;
    u32 maxHP = 428, threshold = 432;
    for (u32 side = 0; side < 2; side++)
        for (u32 boundary = 0; boundary < 3; boundary++) {
            PARAMETRIZE { holder = side ? opponent : player; attacker = side ? player : opponent; ability = ABILITY_JUSTIFIED; move = MOVE_FEINT_ATTACK; maxHP = 428 + boundary * 4; threshold = 432; }
            PARAMETRIZE { holder = side ? opponent : player; attacker = side ? player : opponent; ability = ABILITY_RATTLED; move = MOVE_NIGHT_SHADE; maxHP = 196 + boundary * 4; threshold = 200; }
            PARAMETRIZE { holder = side ? opponent : player; attacker = side ? player : opponent; ability = ABILITY_WEAK_ARMOR; move = MOVE_SEISMIC_TOSS; maxHP = 196 + boundary * 4; threshold = 200; }
        }
    bool32 activated = B_POLISHED_MOVE_BALANCE && maxHP <= threshold;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(50); Attack(200); Defense(100); Ability(holder == player ? ability : ABILITY_NONE); if (holder == player) { HP(maxHP); MaxHP(maxHP); } }
        OPPONENT(SPECIES_WOBBUFFET) { Level(50); Attack(200); Defense(100); Ability(holder == opponent ? ability : ABILITY_NONE); if (holder == opponent) { HP(maxHP); MaxHP(maxHP); } }
    } WHEN {
        TURN { MOVE(holder, MOVE_SUBSTITUTE); }
        TURN { MOVE(attacker, move, WITH_RNG(RNG_DAMAGE_MODIFIER, 0)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, attacker);
        if (activated) ABILITY_POPUP(holder, ability);
        else NOT ABILITY_POPUP(holder, ability);
    } THEN {
        EXPECT_EQ(holder->hp, maxHP - maxHP / 4);
        EXPECT_EQ((u32)holder->volatiles.substituteHP, maxHP > threshold ? 1 : 0);
        EXPECT_EQ(holder->statStages[STAT_ATK], DEFAULT_STAT_STAGE + (activated && ability == ABILITY_JUSTIFIED));
        EXPECT_EQ(holder->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + (activated ? ability == ABILITY_WEAK_ARMOR ? 2 : ability == ABILITY_RATTLED : 0));
        EXPECT_EQ(holder->statStages[STAT_DEF], DEFAULT_STAT_STAGE - (activated && ability == ABILITY_WEAK_ARMOR));
    }
}

SINGLE_BATTLE_TEST("Polished hit stats breaking Substitute preserves Gas suppression but ignores Mold Breaker")
{
    enum Ability ability = ABILITY_JUSTIFIED, attackerAbility = ABILITY_MOLD_BREAKER;
    for (u32 blocker = 0; blocker < 2; blocker++) {
        PARAMETRIZE { ability = ABILITY_JUSTIFIED; attackerAbility = blocker ? ABILITY_NEUTRALIZING_GAS : ABILITY_MOLD_BREAKER; }
        PARAMETRIZE { ability = ABILITY_RATTLED; attackerAbility = blocker ? ABILITY_NEUTRALIZING_GAS : ABILITY_MOLD_BREAKER; }
        PARAMETRIZE { ability = ABILITY_WEAK_ARMOR; attackerAbility = blocker ? ABILITY_NEUTRALIZING_GAS : ABILITY_MOLD_BREAKER; }
    }
    bool32 activated = B_POLISHED_MOVE_BALANCE && attackerAbility != ABILITY_NEUTRALIZING_GAS;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(50); Attack(1000); Ability(attackerAbility); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(200); MaxHP(200); Defense(100); Ability(ability); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_SUBSTITUTE); }
        TURN { MOVE(player, MOVE_FEINT_ATTACK); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_FEINT_ATTACK, player);
        if (activated) ABILITY_POPUP(opponent, ability);
        else NOT ABILITY_POPUP(opponent, ability);
    } THEN {
        EXPECT_EQ(opponent->hp, 150);
        EXPECT_EQ((u32)opponent->volatiles.substituteHP, 0);
        EXPECT_EQ(opponent->statStages[STAT_ATK], DEFAULT_STAT_STAGE + (activated && ability == ABILITY_JUSTIFIED));
        EXPECT_EQ(opponent->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + (activated ? ability == ABILITY_WEAK_ARMOR ? 2 : ability == ABILITY_RATTLED : 0));
    }
}

SINGLE_BATTLE_TEST("Polished hit stats Anger Point and its Mirror Herb copy include intact and broken Substitutes")
{
    struct BattlePokemon *holder = player, *attacker = opponent;
    enum Ability attackerAbility = ABILITY_NONE;
    u32 mode = 0;
    for (u32 side = 0; side < 2; side++)
        for (u32 sub = 0; sub < 3; sub++)
            for (u32 blocker = 0; blocker < 3; blocker++)
                PARAMETRIZE { holder = side ? opponent : player; attacker = side ? player : opponent; mode = sub; attackerAbility = blocker == 0 ? ABILITY_NONE : blocker == 1 ? ABILITY_MOLD_BREAKER : ABILITY_NEUTRALIZING_GAS; }
    bool32 activated = attackerAbility != ABILITY_NEUTRALIZING_GAS && (B_POLISHED_MOVE_BALANCE || mode == 0);
    u32 maxHP = mode == 2 ? 80 : 1000;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(50); Attack(101); Defense(73); Ability(holder == player ? ABILITY_ANGER_POINT : attackerAbility); Item(attacker == player ? ITEM_MIRROR_HERB : ITEM_NONE); HP(holder == player ? maxHP : 1000); MaxHP(holder == player ? maxHP : 1000); }
        OPPONENT(SPECIES_WOBBUFFET) { Level(50); Attack(101); Defense(73); Ability(holder == opponent ? ABILITY_ANGER_POINT : attackerAbility); Item(attacker == opponent ? ITEM_MIRROR_HERB : ITEM_NONE); HP(holder == opponent ? maxHP : 1000); MaxHP(holder == opponent ? maxHP : 1000); }
    } WHEN {
        TURN { MOVE(holder, mode ? MOVE_SUBSTITUTE : MOVE_SPLASH); }
        TURN { MOVE(attacker, MOVE_SCRATCH, criticalHit: TRUE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SCRATCH, attacker);
        if (activated) {
            ABILITY_POPUP(holder, ABILITY_ANGER_POINT);
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, attacker);
        } else {
            NOT ABILITY_POPUP(holder, ABILITY_ANGER_POINT);
        }
    } THEN {
        EXPECT_EQ(holder->statStages[STAT_ATK], activated ? MAX_STAT_STAGE : DEFAULT_STAT_STAGE);
        EXPECT_EQ(attacker->statStages[STAT_ATK], activated ? MAX_STAT_STAGE : DEFAULT_STAT_STAGE);
        EXPECT_EQ(attacker->item, activated ? ITEM_NONE : ITEM_MIRROR_HERB);
        if (mode) {
            EXPECT_EQ(holder->hp, maxHP - maxHP / 4);
            EXPECT_EQ(holder->volatiles.substituteHP != 0, mode == 1);
        }
    }
}

SINGLE_BATTLE_TEST("Polished hit stats immediate Liechi consumption precedes critical text and Anger Point caps")
{
    struct BattlePokemon *holder = player, *attacker = opponent;
    u32 initialBoost = 0;
    bool32 sheerForce = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 cap = 0; cap < 3; cap++)
            for (u32 suppressed = 0; suppressed < 2; suppressed++)
                PARAMETRIZE { holder = side ? opponent : player; attacker = side ? player : opponent; initialBoost = cap == 0 ? 0 : cap + 4; sheerForce = suppressed; }
    bool32 consumed = B_POLISHED_MOVE_BALANCE && initialBoost < 6;
    bool32 activated = initialBoost < 6 && (!B_POLISHED_MOVE_BALANCE || initialBoost == 0);
    enum Move move = sheerForce ? MOVE_BODY_SLAM : MOVE_SCRATCH;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(holder == player ? ABILITY_ANGER_POINT : sheerForce ? ABILITY_SHEER_FORCE : ABILITY_NONE); Level(50); Attack(100); Defense(100); HP(holder == player ? 251 : 1000); MaxHP(1000); Item(holder == player ? ITEM_LIECHI_BERRY : ITEM_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(holder == opponent ? ABILITY_ANGER_POINT : sheerForce ? ABILITY_SHEER_FORCE : ABILITY_NONE); Level(50); Attack(100); Defense(100); HP(holder == opponent ? 251 : 1000); MaxHP(1000); Item(holder == opponent ? ITEM_LIECHI_BERRY : ITEM_NONE); }
    } WHEN {
        if (initialBoost) {
            TURN { MOVE(holder, MOVE_SWORDS_DANCE); }
            TURN { MOVE(holder, MOVE_SWORDS_DANCE); }
            TURN { MOVE(holder, initialBoost == 5 ? MOVE_HOWL : MOVE_SWORDS_DANCE); }
        }
        TURN { MOVE(attacker, move, criticalHit: TRUE, secondaryEffect: FALSE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, attacker);
        HP_BAR(holder);
        if (consumed) ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, holder);
        MESSAGE("A critical hit!");
        if (activated) ABILITY_POPUP(holder, ABILITY_ANGER_POINT);
        else NOT ABILITY_POPUP(holder, ABILITY_ANGER_POINT);
    } THEN {
        EXPECT_EQ(holder->statStages[STAT_ATK], MAX_STAT_STAGE);
        EXPECT_EQ(holder->item, consumed ? ITEM_NONE : ITEM_LIECHI_BERRY);
    }
}

SINGLE_BATTLE_TEST("Polished hit stats Anger Point resolves during critical text before the burn command thaws")
{
    struct BattlePokemon *holder = player, *attacker = opponent;
    PARAMETRIZE { holder = player; attacker = opponent; }
    PARAMETRIZE { holder = opponent; attacker = player; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(holder == player ? ABILITY_ANGER_POINT : ABILITY_NONE); Level(50); HP(1000); MaxHP(1000); SpAttack(100); SpDefense(100); Status1(holder == player ? STATUS1_FREEZE : STATUS1_NONE); Speed(attacker == player ? 200 : 100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(holder == opponent ? ABILITY_ANGER_POINT : ABILITY_NONE); Level(50); HP(1000); MaxHP(1000); SpAttack(100); SpDefense(100); Status1(holder == opponent ? STATUS1_FREEZE : STATUS1_NONE); Speed(attacker == opponent ? 200 : 100); }
    } WHEN { TURN { MOVE(attacker, MOVE_FLAMETHROWER, criticalHit: TRUE, secondaryEffect: FALSE); } }
    SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_FLAMETHROWER, attacker);
        MESSAGE("A critical hit!");
        ABILITY_POPUP(holder, ABILITY_ANGER_POINT);
        if (holder == player) MESSAGE("Wobbuffet maxed its Attack!");
        else MESSAGE("The opposing Wobbuffet maxed its Attack!");
        if (holder == player) MESSAGE("Wobbuffet thawed out!");
        else MESSAGE("The opposing Wobbuffet thawed out!");
    } THEN {
        EXPECT_EQ(holder->statStages[STAT_ATK], MAX_STAT_STAGE);
        EXPECT_EQ(holder->status1, STATUS1_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished hit stats lethal Enigma restores its holder before faint effects and its pending action")
{
    struct BattlePokemon *holder = player, *attacker = opponent;
    bool32 destinyBond = FALSE, tera = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 bond = 0; bond < 2; bond++)
            for (u32 gimmick = 0; gimmick < 2; gimmick++)
                PARAMETRIZE { holder = side ? opponent : player; attacker = side ? player : opponent; destinyBond = bond; tera = gimmick; }
    bool32 revived = B_POLISHED_MOVE_BALANCE && !tera;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(holder == player ? ABILITY_NONE : ABILITY_MOXIE); Level(50); Attack(100); Defense(100); HP(holder == player ? 1 : 1000); MaxHP(holder == player ? 400 : 1000); Item(holder == player ? ITEM_ENIGMA_BERRY : ITEM_NONE); TeraType(TYPE_PSYCHIC); Speed(attacker == player ? 200 : 100); }
        PLAYER(SPECIES_MAGIKARP) { HP(100); MaxHP(100); Speed(90); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(holder == opponent ? ABILITY_NONE : ABILITY_MOXIE); Level(50); Attack(100); Defense(100); HP(holder == opponent ? 1 : 1000); MaxHP(holder == opponent ? 400 : 1000); Item(holder == opponent ? ITEM_ENIGMA_BERRY : ITEM_NONE); TeraType(TYPE_PSYCHIC); Speed(attacker == opponent ? 200 : 100); }
        OPPONENT(SPECIES_MAGIKARP) { HP(100); MaxHP(100); Speed(80); }
    } WHEN {
        TURN { MOVE(holder, destinyBond ? MOVE_DESTINY_BOND : MOVE_SPLASH, gimmick: tera ? GIMMICK_TERA : GIMMICK_NONE); }
        TURN {
            MOVE(attacker, MOVE_BITE, criticalHit: FALSE, secondaryEffect: FALSE);
            MOVE(holder, MOVE_SPLASH);
            if (!revived) {
                SEND_OUT(holder, 1);
                if (destinyBond) SEND_OUT(attacker, 1);
            }
        }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_BITE, attacker);
        HP_BAR(holder, damage: 1);
        if (revived) {
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, holder);
            HP_BAR(holder, damage: -100);
            NOT ABILITY_POPUP(attacker, ABILITY_MOXIE);
            ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, holder);
        } else {
            NOT ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, holder);
            if (destinyBond) HP_BAR(attacker, damage: 1000);
            else ABILITY_POPUP(attacker, ABILITY_MOXIE);
            NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, holder);
        }
    } THEN {
        EXPECT_EQ(GetMonData(&GetBattlerParty(holder - gBattleMons)[0], MON_DATA_HP), revived ? 100 : 0);
        EXPECT_EQ(GetMonData(&GetBattlerParty(holder - gBattleMons)[0], MON_DATA_HELD_ITEM), revived ? ITEM_NONE : ITEM_ENIGMA_BERRY);
        EXPECT_EQ(GetMonData(&GetBattlerParty(attacker - gBattleMons)[0], MON_DATA_HP), !revived && destinyBond ? 0 : 1000);
        EXPECT_EQ(attacker->statStages[STAT_ATK], DEFAULT_STAT_STAGE + (!revived && !destinyBond));
        if (revived) {
            EXPECT_EQ(gBattlerPartyIndexes[holder - gBattleMons], 0);
            EXPECT_EQ(holder->hp, 100);
            EXPECT_EQ(holder->item, ITEM_NONE);
            EXPECT_EQ(GetMonData(GetBattlerMon(holder - gBattleMons), MON_DATA_HP), 100);
        } else {
            EXPECT_EQ(gBattlerPartyIndexes[holder - gBattleMons], 1);
        }
    }
}

SINGLE_BATTLE_TEST("Polished hit stats Future Sight consumes Liechi before critical text and Anger Point")
{
    struct BattlePokemon *holder = player, *attacker = opponent;
    PARAMETRIZE { holder = player; attacker = opponent; }
    PARAMETRIZE { holder = opponent; attacker = player; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(holder == player ? ABILITY_ANGER_POINT : ABILITY_PRESSURE); Level(50); SpAttack(100); SpDefense(100); HP(holder == player ? 251 : 1000); MaxHP(1000); Item(holder == player ? ITEM_LIECHI_BERRY : ITEM_SCOPE_LENS); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(holder == opponent ? ABILITY_ANGER_POINT : ABILITY_PRESSURE); Level(50); SpAttack(100); SpDefense(100); HP(holder == opponent ? 251 : 1000); MaxHP(1000); Item(holder == opponent ? ITEM_LIECHI_BERRY : ITEM_SCOPE_LENS); }
    } WHEN {
        TURN { MOVE(attacker, MOVE_FUTURE_SIGHT); }
        TURN { MOVE(attacker, MOVE_FOCUS_ENERGY); }
        TURN {}
    } SCENE {
        if (holder == player) MESSAGE("Wobbuffet took the Future Sight attack!");
        else MESSAGE("The opposing Wobbuffet took the Future Sight attack!");
        HP_BAR(holder);
        if (B_POLISHED_MOVE_BALANCE) ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, holder);
        MESSAGE("A critical hit!");
        ABILITY_POPUP(holder, ABILITY_ANGER_POINT);
    } THEN {
        EXPECT_EQ(holder->statStages[STAT_ATK], MAX_STAT_STAGE);
        EXPECT_EQ(holder->item, B_POLISHED_MOVE_BALANCE ? ITEM_NONE : ITEM_LIECHI_BERRY);
        EXPECT_GT(holder->hp, 0);
        EXPECT_LT(holder->hp, 251);
    }
}

SINGLE_BATTLE_TEST("Polished hit stats Future Sight Parental Bond repeats and cancels Berserk after intermediate healing", s16 firstDamage; s16 secondDamage)
{
    struct BattlePokemon *holder = player, *attacker = opponent;
    bool32 parentalBond = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 bond = 0; bond < 2; bond++)
            PARAMETRIZE { holder = side ? opponent : player; attacker = side ? player : opponent; parentalBond = bond; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(holder == player ? ABILITY_BERSERK : parentalBond ? ABILITY_PARENTAL_BOND : ABILITY_PRESSURE); Level(50); SpAttack(100); SpDefense(100); HP(holder == player ? 501 : 1000); MaxHP(1000); Item(holder == player ? ITEM_SITRUS_BERRY : ITEM_SCOPE_LENS); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(holder == opponent ? ABILITY_BERSERK : parentalBond ? ABILITY_PARENTAL_BOND : ABILITY_PRESSURE); Level(50); SpAttack(100); SpDefense(100); HP(holder == opponent ? 501 : 1000); MaxHP(1000); Item(holder == opponent ? ITEM_SITRUS_BERRY : ITEM_SCOPE_LENS); }
    } WHEN {
        TURN { MOVE(attacker, MOVE_FUTURE_SIGHT); }
        TURN { MOVE(attacker, MOVE_FOCUS_ENERGY); }
        TURN {}
    } SCENE {
        if (holder == player) MESSAGE("Wobbuffet took the Future Sight attack!");
        else MESSAGE("The opposing Wobbuffet took the Future Sight attack!");
        HP_BAR(holder, captureDamage: &results[i].firstDamage);
        if (B_POLISHED_MOVE_BALANCE) {
            if (!parentalBond) ABILITY_POPUP(holder, ABILITY_BERSERK);
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, holder);
            HP_BAR(holder, damage: -250);
            MESSAGE("A critical hit!");
        } else {
            MESSAGE("A critical hit!");
            if (!parentalBond) ABILITY_POPUP(holder, ABILITY_BERSERK);
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, holder);
            HP_BAR(holder, damage: -250);
        }
        if (parentalBond) {
            HP_BAR(holder, captureDamage: &results[i].secondDamage);
            if (B_POLISHED_MOVE_BALANCE) MESSAGE("A critical hit!");
            NOT ABILITY_POPUP(holder, ABILITY_BERSERK);
        }
    } THEN {
        EXPECT_EQ(holder->statStages[STAT_SPATK], DEFAULT_STAT_STAGE + !parentalBond);
        EXPECT_EQ(holder->item, ITEM_NONE);
        EXPECT_GT(results[i].firstDamage, 0);
        if (parentalBond) EXPECT_GT(results[i].secondDamage, 0);
        EXPECT_EQ(holder->hp, 751 - results[i].firstDamage - (parentalBond ? results[i].secondDamage : 0));
        EXPECT_GT(holder->hp, 500);
        EXPECT_LT(holder->hp, 751);
    }
}

SINGLE_BATTLE_TEST("Polished hit stats immediate Enigma precedes reserved Knock Off and Bug Bite loss", s16 damage)
{
    struct BattlePokemon *holder = player, *attacker = opponent;
    enum Move move = MOVE_KNOCK_OFF;
    bool32 unnerve = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 theft = 0; theft < 2; theft++)
            for (u32 blocker = 0; blocker < 2; blocker++)
                PARAMETRIZE { holder = side ? opponent : player; attacker = side ? player : opponent; move = theft ? MOVE_BUG_BITE : MOVE_KNOCK_OFF; unnerve = blocker; }
    bool32 holderHeals = !unnerve && (B_POLISHED_MOVE_BALANCE || move == MOVE_KNOCK_OFF);
    // The attacker's Unnerve blocks the victim's held trigger, but it does
    // not block the attacker's later forced consumption through Bug Bite.
    bool32 attackerHeals = move == MOVE_BUG_BITE && (!B_POLISHED_MOVE_BALANCE || unnerve);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(attacker == player && unnerve ? ABILITY_UNNERVE : ABILITY_NONE); Level(50); Attack(100); Defense(100); HP(holder == player ? 500 : 400); MaxHP(800); Item(holder == player ? ITEM_ENIGMA_BERRY : ITEM_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(attacker == opponent && unnerve ? ABILITY_UNNERVE : ABILITY_NONE); Level(50); Attack(100); Defense(100); HP(holder == opponent ? 500 : 400); MaxHP(800); Item(holder == opponent ? ITEM_ENIGMA_BERRY : ITEM_NONE); }
    } WHEN { TURN { MOVE(attacker, move, criticalHit: TRUE); } }
    SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, attacker);
        HP_BAR(holder, captureDamage: &results[i].damage);
        if (B_POLISHED_MOVE_BALANCE && holderHeals) {
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, holder);
            HP_BAR(holder, damage: -200);
        }
        MESSAGE("A critical hit!");
        if (!B_POLISHED_MOVE_BALANCE && holderHeals) {
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, holder);
            HP_BAR(holder, damage: -200);
        }
        if (attackerHeals) HP_BAR(attacker, damage: -200);
    } THEN {
        EXPECT_GT(results[i].damage, 0);
        EXPECT_EQ(holder->hp, 500 - results[i].damage + (holderHeals ? 200 : 0));
        EXPECT_EQ(attacker->hp, attackerHeals ? 600 : 400);
        EXPECT_EQ(holder->item, ITEM_NONE);
    }
}

DOUBLE_BATTLE_TEST("Polished hit stats optional doubles retain native Anger Point Substitute rejection")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Level(50); Attack(101); }
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_ANGER_POINT); HP(80); MaxHP(80); Defense(73); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(opponentLeft, MOVE_SUBSTITUTE); }
        TURN { MOVE(playerLeft, MOVE_SCRATCH, target: opponentLeft, criticalHit: TRUE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SCRATCH, playerLeft);
        NOT ABILITY_POPUP(opponentLeft, ABILITY_ANGER_POINT);
    } THEN {
        EXPECT_EQ(opponentLeft->statStages[STAT_ATK], DEFAULT_STAT_STAGE);
        EXPECT_EQ(opponentLeft->hp, 60);
    }
}

SINGLE_BATTLE_TEST("Polished hit stats Berserk remains suppressed by Sheer Force under Champions presets")
{
    struct BattlePokemon *holder = player, *attacker = opponent;
    enum Ability attackerAbility = ABILITY_NONE;
    u32 config = GEN_9;
    for (u32 side = 0; side < 2; side++)
        for (u32 force = 0; force < 2; force++) {
            PARAMETRIZE { holder = side ? opponent : player; attacker = side ? player : opponent; attackerAbility = force ? ABILITY_SHEER_FORCE : ABILITY_NONE; config = GEN_9; }
            PARAMETRIZE { holder = side ? opponent : player; attacker = side ? player : opponent; attackerAbility = force ? ABILITY_SHEER_FORCE : ABILITY_NONE; config = GEN_CHAMPIONS; }
        }
    bool32 activated = attackerAbility != ABILITY_SHEER_FORCE || (!B_POLISHED_MOVE_BALANCE && config >= GEN_CHAMPIONS);
    GIVEN {
        WITH_CONFIG(B_SHEER_FORCE_AGAINST_ABILITIES, config);
        PLAYER(SPECIES_WOBBUFFET) { Ability(holder == player ? ABILITY_BERSERK : attackerAbility); HP(holder == player ? 251 : 500); MaxHP(500); SpAttack(100); SpDefense(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(holder == opponent ? ABILITY_BERSERK : attackerAbility); HP(holder == opponent ? 251 : 500); MaxHP(500); SpAttack(100); SpDefense(100); }
    } WHEN { TURN { MOVE(attacker, MOVE_EMBER, secondaryEffect: FALSE); } }
    SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_EMBER, attacker);
        if (activated) ABILITY_POPUP(holder, ABILITY_BERSERK);
        else NOT ABILITY_POPUP(holder, ABILITY_BERSERK);
    } THEN { EXPECT_EQ(holder->statStages[STAT_SPATK], DEFAULT_STAT_STAGE + activated); }
}

SINGLE_BATTLE_TEST("Polished hit stats Enigma heals before Berserk while ordinary healing waits")
{
    enum Item item = ITEM_ENIGMA_BERRY;
    PARAMETRIZE { item = ITEM_ENIGMA_BERRY; }
    PARAMETRIZE { item = ITEM_SITRUS_BERRY; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); SpAttack(100); Level(50); }
        OPPONENT(SPECIES_BULBASAUR) { Ability(ABILITY_BERSERK); Item(item); HP(251); MaxHP(500); SpDefense(100); }
    } WHEN { TURN { MOVE(player, MOVE_EMBER, secondaryEffect: FALSE); } }
    SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_EMBER, player);
        if (item == ITEM_SITRUS_BERRY) ABILITY_POPUP(opponent, ABILITY_BERSERK);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, opponent);
        NOT ABILITY_POPUP(opponent, ABILITY_BERSERK);
    } THEN {
        EXPECT_EQ(opponent->statStages[STAT_SPATK], DEFAULT_STAT_STAGE + (item == ITEM_SITRUS_BERRY));
        EXPECT_GT(opponent->hp, 250);
        EXPECT_EQ(opponent->item, ITEM_NONE);
    }
}

DOUBLE_BATTLE_TEST("Polished hit stats optional doubles retain native Champions Berserk with Sheer Force")
{
    GIVEN {
        WITH_CONFIG(B_SHEER_FORCE_AGAINST_ABILITIES, GEN_CHAMPIONS);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_SHEER_FORCE); SpAttack(100); }
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_BERSERK); HP(251); MaxHP(500); SpDefense(100); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN { TURN { MOVE(playerLeft, MOVE_EMBER, target: opponentLeft, secondaryEffect: FALSE); } }
    SCENE { ABILITY_POPUP(opponentLeft, ABILITY_BERSERK); }
    THEN { EXPECT_EQ(opponentLeft->statStages[STAT_SPATK], DEFAULT_STAT_STAGE + 1); }
}

SINGLE_BATTLE_TEST("Polished hit stats Berserk remembers final ordinary berry healing across multihit and Parental Bond", s16 firstDamage; s16 secondDamage; s16 healing)
{
    struct BattlePokemon *holder = opponent, *attacker = player;
    enum Item item = ITEM_SITRUS_BERRY;
    bool32 parentalBond = FALSE, finalHitBerry = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 bond = 0; bond < 2; bond++)
            for (u32 finalHit = 0; finalHit < 2; finalHit++) {
                PARAMETRIZE { holder = side ? player : opponent; attacker = side ? opponent : player; parentalBond = bond; finalHitBerry = finalHit; item = ITEM_SITRUS_BERRY; }
                PARAMETRIZE { holder = side ? player : opponent; attacker = side ? opponent : player; parentalBond = bond; finalHitBerry = finalHit; item = ITEM_ENIGMA_BERRY; }
            }
    // Fixed-damage moves clear effectiveness flags and cannot exercise Enigma.
    // Ordinary super-effective Bite gives a 92/23 Parental Bond pair here.
    enum Move move = parentalBond ? MOVE_BITE : MOVE_DOUBLE_KICK;
    u32 firstHitDamage = parentalBond ? 92 : 48;
    u32 secondHitDamage = parentalBond ? (B_POLISHED_MOVE_BALANCE ? 23 : 22) : 48;
    u32 initialHP = finalHitBerry ? 200 + firstHitDamage + secondHitDamage / 2 : 225;
    bool32 firstHitBerry = item == ITEM_ENIGMA_BERRY || !finalHitBerry;
    bool32 activated = B_POLISHED_MOVE_BALANCE && item == ITEM_SITRUS_BERRY && finalHitBerry;
    GIVEN {
        PLAYER(holder == player && !parentalBond ? SPECIES_RATTATA : SPECIES_WOBBUFFET) { Level(50); Attack(100); Defense(60); Ability(holder == player ? ABILITY_BERSERK : parentalBond ? ABILITY_PARENTAL_BOND : ABILITY_NONE); Item(holder == player ? item : ITEM_NONE); HP(holder == player ? initialHP : 400); MaxHP(400); }
        OPPONENT(holder == opponent && !parentalBond ? SPECIES_RATTATA : SPECIES_WOBBUFFET) { Level(50); Attack(100); Defense(60); Ability(holder == opponent ? ABILITY_BERSERK : parentalBond ? ABILITY_PARENTAL_BOND : ABILITY_NONE); Item(holder == opponent ? item : ITEM_NONE); HP(holder == opponent ? initialHP : 400); MaxHP(400); }
    } WHEN { TURN { MOVE(attacker, move, criticalHit: FALSE, secondaryEffect: FALSE, WITH_RNG(RNG_DAMAGE_MODIFIER, 0)); } }
    SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, attacker);
        HP_BAR(holder, captureDamage: &results[i].firstDamage);
        if (firstHitBerry) {
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, holder);
            HP_BAR(holder, captureDamage: &results[i].healing);
        }
        HP_BAR(holder, captureDamage: &results[i].secondDamage);
        if (!firstHitBerry) {
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, holder);
            HP_BAR(holder, captureDamage: &results[i].healing);
        }
        if (activated) ABILITY_POPUP(holder, ABILITY_BERSERK);
        else NOT ABILITY_POPUP(holder, ABILITY_BERSERK);
    } THEN {
        EXPECT_EQ(results[i].firstDamage, firstHitDamage);
        EXPECT_EQ(results[i].secondDamage, secondHitDamage);
        EXPECT_EQ(results[i].healing, -100);
        EXPECT_EQ(holder->hp, initialHP - firstHitDamage - secondHitDamage + 100);
        EXPECT_GT(holder->hp, 200);
        EXPECT_EQ(holder->item, ITEM_NONE);
        EXPECT_EQ(holder->statStages[STAT_SPATK], DEFAULT_STAT_STAGE + activated);
    }
}

SINGLE_BATTLE_TEST("Polished hit stats suppressed Berserk cannot leak across a new action a canceled hit or replacement")
{
    u32 followup = 0;
    PARAMETRIZE { followup = 0; }
    PARAMETRIZE { followup = 1; }
    PARAMETRIZE { followup = 2; }
    GIVEN {
        WITH_CONFIG(B_SHEER_FORCE_AGAINST_ABILITIES, GEN_9);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_SHEER_FORCE); SpAttack(100); Level(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_BERSERK); SpDefense(100); HP(201); MaxHP(400); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_BERSERK); HP(190); MaxHP(400); }
    } WHEN {
        TURN { MOVE(player, MOVE_EMBER, secondaryEffect: FALSE); }
        if (followup == 0) TURN { MOVE(player, MOVE_SPLASH); }
        if (followup == 1) TURN { MOVE(opponent, MOVE_PROTECT); MOVE(player, MOVE_NIGHT_SHADE); }
        if (followup == 2) TURN { SWITCH(opponent, 1); MOVE(player, MOVE_SPLASH); }
        TURN { MOVE(player, MOVE_NIGHT_SHADE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_EMBER, player);
        NOT ABILITY_POPUP(opponent, ABILITY_BERSERK);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_NIGHT_SHADE, player);
        NOT ABILITY_POPUP(opponent, ABILITY_BERSERK);
    } THEN {
        EXPECT_LT(opponent->hp, 200);
        EXPECT_EQ(opponent->statStages[STAT_SPATK], DEFAULT_STAT_STAGE);
    }
}
