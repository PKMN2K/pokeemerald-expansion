#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished Hidden Power uses all 64 DV patterns and fixed power in battle")
{
    u32 bits = 0, low = 0, hp = 0, atk = 0, def = 0, spe = 0, spa = 0, spd = 0;
    static const enum Type types[] = {TYPE_FIGHTING, TYPE_FLYING, TYPE_POISON, TYPE_GROUND,
        TYPE_ROCK, TYPE_BUG, TYPE_GHOST, TYPE_STEEL, TYPE_FIRE, TYPE_WATER, TYPE_GRASS,
        TYPE_ELECTRIC, TYPE_PSYCHIC, TYPE_ICE, TYPE_DRAGON, TYPE_DARK, TYPE_FAIRY};
    for (u32 b = 0; b < 64; b++)
    {
        for (u32 l = 0; l < 2; l++)
        {
            PARAMETRIZE {
                bits = b; low = l;
                hp = 28 | ((b & 1) << 1) | l;
                atk = 28 | (((b >> 1) & 1) << 1) | l;
                def = 28 | (((b >> 2) & 1) << 1) | l;
                spe = 28 | (((b >> 3) & 1) << 1) | l;
                spa = 28 | (((b >> 4) & 1) << 1) | l;
                spd = 28 | (((b >> 5) & 1) << 1) | l;
            }
        }
    }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HPIV(hp); AttackIV(atk); DefenseIV(def); SpeedIV(spe); SpAttackIV(spa); SpDefenseIV(spd); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1000); MaxHP(1000); }
    } WHEN {
        TURN { MOVE(player, MOVE_HIDDEN_POWER); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_HIDDEN_POWER, player);
        HP_BAR(opponent);
        enum Type type = B_POLISHED_HIDDEN_POWER ? types[bits == 63 ? 16 : bits / 4] : (low ? TYPE_DARK : TYPE_FIGHTING);
        if (type == TYPE_BUG || type == TYPE_GHOST || type == TYPE_DARK)
            MESSAGE("It's super effective!");
        else if (type == TYPE_FIGHTING || type == TYPE_PSYCHIC)
            MESSAGE("It's not very effective…");
        else
        {
            NONE_OF {
                MESSAGE("It's super effective!");
                MESSAGE("It's not very effective…");
            }
        }
    } THEN {
        enum BattlerId battler = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);
        EXPECT_EQ(gBattleMovePower, B_POLISHED_HIDDEN_POWER ? 70 : 60);
        EXPECT_EQ(GetDynamicMoveType(GetBattlerMon(battler), MOVE_HIDDEN_POWER, battler, ABILITY_NONE, HOLD_EFFECT_NONE, MON_IN_BATTLE), B_POLISHED_HIDDEN_POWER ? types[bits == 63 ? 16 : bits / 4] : (low ? TYPE_DARK : TYPE_FIGHTING));
    }
}

SINGLE_BATTLE_TEST("Polished Hidden Power can hit Dragons super effectively as Fairy")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HPIV(31); AttackIV(31); DefenseIV(31); SpeedIV(31); SpAttackIV(31); SpDefenseIV(31); }
        OPPONENT(SPECIES_DRATINI) { HP(1000); MaxHP(1000); }
    } WHEN {
        TURN { MOVE(player, MOVE_HIDDEN_POWER); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_HIDDEN_POWER, player);
        HP_BAR(opponent);
        if (B_POLISHED_HIDDEN_POWER)
            MESSAGE("It's super effective!");
        else
            NOT MESSAGE("It's super effective!");
    }
}

SINGLE_BATTLE_TEST("Polished Hidden Power respects Ghost immunity to its source Fighting type")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HPIV(1); AttackIV(1); DefenseIV(1); SpeedIV(1); SpAttackIV(1); SpDefenseIV(1); }
        OPPONENT(SPECIES_GASTLY);
    } WHEN {
        TURN { MOVE(player, MOVE_HIDDEN_POWER); }
    } SCENE {
        if (B_POLISHED_HIDDEN_POWER)
        {
            MESSAGE("It doesn't affect the opposing Gastly…");
            NONE_OF { HP_BAR(opponent); }
        }
        else
        {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_HIDDEN_POWER, player);
            HP_BAR(opponent);
            MESSAGE("It's super effective!");
        }
    }
}

AI_SINGLE_BATTLE_TEST("Polished Hidden Power AI uses source Fairy typing and power")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_CHECK_BAD_MOVE | AI_FLAG_TRY_TO_FAINT | AI_FLAG_CHECK_VIABILITY);
        PLAYER(SPECIES_DRATINI) { HP(1000); MaxHP(1000); }
        OPPONENT(SPECIES_WOBBUFFET) { HPIV(31); AttackIV(31); DefenseIV(31); SpeedIV(31); SpAttackIV(31); SpDefenseIV(31); Moves(MOVE_HIDDEN_POWER, MOVE_DRAGON_BREATH); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); EXPECT_MOVE(opponent, B_POLISHED_HIDDEN_POWER ? MOVE_HIDDEN_POWER : MOVE_DRAGON_BREATH); }
    }
}

SINGLE_BATTLE_TEST("Polished Hidden Power Anticipation uses the listed unknown type")
{
    GIVEN {
        PLAYER(SPECIES_DRATINI) { Ability(ABILITY_ANTICIPATION); }
        OPPONENT(SPECIES_WOBBUFFET) { HPIV(31); AttackIV(31); DefenseIV(31); SpeedIV(31); SpAttackIV(31); SpDefenseIV(31); Moves(MOVE_HIDDEN_POWER); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); MOVE(opponent, MOVE_HIDDEN_POWER); }
    } SCENE {
        // Source Fairy would be super-effective, but Anticipation reads the
        // original unknown row. Native Dark is neutral against Dragon.
        NONE_OF { ABILITY_POPUP(player, ABILITY_ANTICIPATION); }
        ANIMATION(ANIM_TYPE_MOVE, MOVE_HIDDEN_POWER, opponent);
        HP_BAR(player);
    }
}
