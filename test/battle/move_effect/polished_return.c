#include "global.h"
#include "test/battle.h"
#include "battle_util.h"

SINGLE_BATTLE_TEST("Polished Return uses source power for all 256 happiness values on both sides")
{
    bool32 source;
    PARAMETRIZE { source = TRUE; }
    PARAMETRIZE { source = FALSE; }
    GIVEN {
        WITH_CONFIG(B_POLISHED_RETURN, source);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); }
    } THEN {
        for (u32 side = 0; side < 2; side++)
        {
            struct DamageContext ctx = {
                .battlerAtk = side, .battlerDef = side ^ 1,
                .move = MOVE_RETURN, .moveType = TYPE_NORMAL,
                .typeEffectivenessModifier = UQ_4_12(1.0), .aiCalc = TRUE,
            };
            for (u32 happiness = 0; happiness <= MAX_FRIENDSHIP; happiness++)
            {
                gBattleMons[side].friendship = happiness;
                s32 damage = CalculateMoveDamageVars(&ctx);
                u32 expected = source ? min(102, happiness * 102 / 160) : max(1, happiness * 10 / 25);
                EXPECT_EQ(gBattleMovePower, expected);
                if (source && happiness < 2)
                {
                    EXPECT_EQ(damage, 1);
                    EXPECT_EQ(ApplyModifiersAfterDmgRoll(&ctx, 0), 1);
                }
            }
        }
    }
}

SINGLE_BATTLE_TEST("Polished Return reaches its source cap at 160 happiness", s16 damage)
{
    u32 happiness;
    PARAMETRIZE { happiness = 150; }
    PARAMETRIZE { happiness = 159; }
    PARAMETRIZE { happiness = 160; }
    PARAMETRIZE { happiness = 200; }
    PARAMETRIZE { happiness = 255; }
    GIVEN {
        WITH_CONFIG(B_POLISHED_RETURN, TRUE);
        PLAYER(SPECIES_WOBBUFFET) { Friendship(happiness); Level(100); Attack(200); }
        OPPONENT(SPECIES_WOBBUFFET) { Defense(100); HP(1000); MaxHP(1000); }
    } WHEN {
        TURN { MOVE(player, MOVE_RETURN, WITH_RNG(RNG_DAMAGE_MODIFIER, 0), criticalHit: FALSE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_RETURN, player);
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } THEN {
        if (i == 1 || i == 2)
            EXPECT_GT(results[i].damage, results[i - 1].damage);
        if (i > 2)
            EXPECT_EQ(results[i].damage, results[2].damage);
    }
}

SINGLE_BATTLE_TEST("Polished Return zero power becomes one damage after boosts and resistance")
{
    u32 happiness;
    enum Species target;
    PARAMETRIZE { happiness = 0; target = SPECIES_WOBBUFFET; }
    PARAMETRIZE { happiness = 1; target = SPECIES_WOBBUFFET; }
    PARAMETRIZE { happiness = 0; target = SPECIES_ARON; }
    PARAMETRIZE { happiness = 1; target = SPECIES_ARON; }
    GIVEN {
        WITH_CONFIG(B_POLISHED_RETURN, TRUE);
        PLAYER(SPECIES_CINCCINO) { Friendship(happiness); Level(100); Attack(1000); Ability(ABILITY_TECHNICIAN); Item(ITEM_CHOICE_BAND); }
        OPPONENT(target) { Defense(10); }
    } WHEN {
        TURN { MOVE(player, MOVE_RETURN, criticalHit: TRUE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_RETURN, player);
        HP_BAR(opponent, damage: 1);
    }
}

SINGLE_BATTLE_TEST("Polished Return zero-power floor does not bypass Ghost immunity")
{
    GIVEN {
        WITH_CONFIG(B_POLISHED_RETURN, TRUE);
        PLAYER(SPECIES_WOBBUFFET) { Friendship(0); }
        OPPONENT(SPECIES_GASTLY);
    } WHEN {
        TURN { MOVE(player, MOVE_RETURN); }
    } SCENE {
        MESSAGE("It doesn't affect the opposing Gastly…");
        NONE_OF { HP_BAR(opponent); }
    }
}

SINGLE_BATTLE_TEST("Polished Return immediately reflects bitter-medicine happiness losses")
{
    s16 firstDamage, secondDamage;
    GIVEN {
        WITH_CONFIG(B_POLISHED_RETURN, TRUE);
        WITH_CONFIG(POLISHED_ITEM_HEALING, TRUE);
        PLAYER(SPECIES_WOBBUFFET) { Friendship(160); Level(100); Attack(200); HP(500); MaxHP(1000); }
        OPPONENT(SPECIES_WOBBUFFET) { Defense(100); HP(1000); MaxHP(1000); }
    } WHEN {
        TURN { MOVE(player, MOVE_RETURN, WITH_RNG(RNG_DAMAGE_MODIFIER, 0), criticalHit: FALSE); }
        TURN { USE_ITEM(player, ITEM_ENERGY_POWDER, partyIndex: 0); }
        TURN { MOVE(player, MOVE_RETURN, WITH_RNG(RNG_DAMAGE_MODIFIER, 0), criticalHit: FALSE); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &firstDamage);
        HP_BAR(player, damage: -60);
        HP_BAR(opponent, captureDamage: &secondDamage);
    } THEN {
        EXPECT_EQ(player->friendship, 150);
        EXPECT_GT(firstDamage, secondDamage);
        EXPECT_EQ(gBattleMovePower, 95);
    }
}

SINGLE_BATTLE_TEST("Polished Return preserves optional shared effects and fixed-power overrides")
{
    GIVEN {
        WITH_CONFIG(B_POLISHED_RETURN, TRUE);
        PLAYER(SPECIES_WOBBUFFET) { Friendship(100); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); }
    } THEN {
        const enum Move moves[] = {MOVE_PIKA_PAPOW, MOVE_VEEVEE_VOLLEY, MOVE_FRUSTRATION};
        const u32 expected[] = {40, 40, 62};
        struct DamageContext ctx = {
            .battlerAtk = 0, .battlerDef = 1,
            .typeEffectivenessModifier = UQ_4_12(1.0), .aiCalc = TRUE,
        };
        for (u32 n = 0; n < ARRAY_COUNT(moves); n++)
        {
            ctx.move = moves[n];
            ctx.moveType = GetMoveType(ctx.move);
            CalculateMoveDamageVars(&ctx);
            EXPECT_EQ(gBattleMovePower, expected[n]);
        }
        ctx.move = MOVE_RETURN;
        ctx.moveType = TYPE_NORMAL;
        ctx.fixedBasePower = 80;
        player->friendship = 0;
        EXPECT_GT(CalculateMoveDamageVars(&ctx), 1);
        EXPECT_EQ(gBattleMovePower, 80);
    }
}

AI_SINGLE_BATTLE_TEST("Polished Return AI estimates use the selected power profile")
{
    bool32 source;
    PARAMETRIZE { source = TRUE; }
    PARAMETRIZE { source = FALSE; }
    GIVEN {
        WITH_CONFIG(B_POLISHED_RETURN, source);
        AI_FLAGS(AI_FLAG_CHECK_BAD_MOVE | AI_FLAG_TRY_TO_FAINT | AI_FLAG_CHECK_VIABILITY);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET) { Friendship(80); Moves(MOVE_RETURN, MOVE_TACKLE); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); EXPECT_MOVE(opponent, source ? MOVE_RETURN : MOVE_TACKLE); }
    }
}
