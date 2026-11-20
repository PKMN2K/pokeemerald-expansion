#include "global.h"
#include "test/battle.h"
#include "battle_util.h"
#include "polished_damage.h"
#include "polished_damage_vectors.h"

SINGLE_BATTLE_TEST("Polished damage matches all source stat compression stage and critical vectors")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Level(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Level(50); }
    } WHEN { TURN {} }
    THEN {
        if (B_POLISHED_MOVE_BALANCE)
        for (u32 side = 0; side < 2; side++) for (u32 category = 0; category < 2; category++) {
            struct DamageContext ctx = {0};
            ctx.battlerAtk = side; ctx.battlerDef = side ^ 1;
            ctx.move = category ? MOVE_THUNDER_SHOCK : MOVE_TACKLE;
            ctx.moveType = TYPE_NORMAL; ctx.typeEffectivenessModifier = UQ_4_12(1.0);
            struct BattlePokemon *a = &gBattleMons[ctx.battlerAtk], *d = &gBattleMons[ctx.battlerDef];
            for (u32 i = 0; i < ARRAY_COUNT(sPolishedDamageVectors); i++) {
                const typeof(*sPolishedDamageVectors) *v = &sPolishedDamageVectors[i];
                a->level = v->level;
                a->attack = a->spAttack = v->attack; d->defense = d->spDefense = v->defense;
                a->statStages[STAT_ATK] = a->statStages[STAT_SPATK] = v->attackStage;
                d->statStages[STAT_DEF] = d->statStages[STAT_SPDEF] = v->defenseStage;
                ctx.isCrit = v->critical;
                EXPECT_EQ(CalculateMoveDamageVars(&ctx), v->damage);
            }
        }
    }
}

SINGLE_BATTLE_TEST("Polished damage applies offensive ability burn critical and held boosts before adding two")
{
    enum Ability ability = ABILITY_NONE;
    enum Item item = ITEM_NONE;
    u32 numerator = 1, denominator = 1;
    PARAMETRIZE { ability = ABILITY_NONE; item = ITEM_NONE; numerator = 1; denominator = 1; }
    PARAMETRIZE { ability = ABILITY_HUGE_POWER; item = ITEM_NONE; numerator = 2; denominator = 1; }
    PARAMETRIZE { ability = ABILITY_HUSTLE; item = ITEM_NONE; numerator = 3; denominator = 2; }
    PARAMETRIZE { ability = ABILITY_TECHNICIAN; item = ITEM_NONE; numerator = 3; denominator = 2; }
    PARAMETRIZE { ability = ABILITY_NONE; item = ITEM_CHOICE_BAND; numerator = 3; denominator = 2; }
    PARAMETRIZE { ability = ABILITY_NONE; item = ITEM_LIFE_ORB; numerator = 13; denominator = 10; }
    PARAMETRIZE { ability = ABILITY_NONE; item = ITEM_MUSCLE_BAND; numerator = 11; denominator = 10; }
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(50); Attack(101); Ability(ability); Item(item); Speed(200); }
        OPPONENT(SPECIES_WOBBUFFET) { Defense(73); Ability(ABILITY_NONE); Speed(100); }
    } WHEN { TURN { MOVE(player, MOVE_TACKLE, WITH_RNG(RNG_DAMAGE_MODIFIER, 0)); } }
    SCENE { HP_BAR(opponent, captureDamage: &damage); }
    THEN { if (B_POLISHED_MOVE_BALANCE) EXPECT_EQ(damage, (22 * 40 * 101 * numerator / denominator) / 73 / 50 + 2); }
}

SINGLE_BATTLE_TEST("Polished damage rolls after STAB type and weather on both sides")
{
    u32 roll = 0;
    struct BattlePokemon *user = player, *target = opponent;
    for (u32 side = 0; side < 2; side++) for (u32 r = 0; r < 16; r++)
        PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; roll = r; }
    s16 damage;
    GIVEN {
        PLAYER(user == player ? SPECIES_CHARMANDER : SPECIES_BULBASAUR) { Level(50); SpAttack(101); SpDefense(73); Ability(ABILITY_NONE); Speed(user == player ? 200 : 100); MaxHP(1000); HP(1000); }
        OPPONENT(user == opponent ? SPECIES_CHARMANDER : SPECIES_BULBASAUR) { Level(50); SpAttack(101); SpDefense(73); Ability(ABILITY_NONE); Speed(user == opponent ? 200 : 100); MaxHP(1000); HP(1000); }
    } WHEN {
        TURN { MOVE(user, MOVE_SUNNY_DAY); }
        TURN { MOVE(user, MOVE_EMBER, WITH_RNG(RNG_DAMAGE_MODIFIER, roll)); }
    } SCENE { ANIMATION(ANIM_TYPE_MOVE, MOVE_EMBER, user); HP_BAR(target, captureDamage: &damage); }
    THEN { if (B_POLISHED_MOVE_BALANCE) EXPECT_EQ(damage, ((22 * 40 * 101 / 73 / 50 + 2) * 72 / 16) * (100 - roll) / 100); }
}

SINGLE_BATTLE_TEST("Polished damage screens multiply raw Defense before simultaneous compression")
{
    enum Move screen = MOVE_REFLECT, hit = MOVE_TACKLE;
    PARAMETRIZE { screen = MOVE_REFLECT; hit = MOVE_TACKLE; }
    PARAMETRIZE { screen = MOVE_LIGHT_SCREEN; hit = MOVE_THUNDER_SHOCK; }
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(50); Attack(101); SpAttack(101); Ability(ABILITY_NONE); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Defense(131); SpDefense(131); Ability(ABILITY_NONE); Speed(200); }
    } WHEN { TURN { MOVE(opponent, screen); MOVE(player, hit, WITH_RNG(RNG_DAMAGE_MODIFIER, 0)); } }
    SCENE { HP_BAR(opponent, captureDamage: &damage); }
    THEN { if (B_POLISHED_MOVE_BALANCE) EXPECT_EQ(damage, 22 * 40 * 50 / 131 / 50 + 2); }
}

SINGLE_BATTLE_TEST("Polished damage burned Facade doubles final damage while retaining the burn penalty")
{
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(50); Attack(101); Ability(ABILITY_NONE); Status1(STATUS1_BURN); Speed(200); }
        OPPONENT(SPECIES_WOBBUFFET) { Defense(73); Ability(ABILITY_NONE); Speed(100); }
    } WHEN { TURN { MOVE(player, MOVE_FACADE, WITH_RNG(RNG_DAMAGE_MODIFIER, 0)); } }
    SCENE { HP_BAR(opponent, captureDamage: &damage); }
    THEN { if (B_POLISHED_MOVE_BALANCE) EXPECT_EQ(damage, (22 * 70 * 101 / 2 / 73 / 50 + 2) * 2); }
}

SINGLE_BATTLE_TEST("Polished damage applies Acrobatics and Hex boosts before the roll")
{
    enum Move move = MOVE_ACROBATICS;
    PARAMETRIZE { move = MOVE_ACROBATICS; }
    PARAMETRIZE { move = MOVE_HEX; }
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(50); Attack(101); SpAttack(101); Ability(ABILITY_NONE); Speed(200); }
        OPPONENT(SPECIES_WOBBUFFET) { Defense(73); SpDefense(73); Ability(ABILITY_NONE); Status1(move == MOVE_HEX ? STATUS1_PARALYSIS : STATUS1_NONE); Speed(100); }
    } WHEN { TURN { MOVE(player, move, WITH_RNG(RNG_DAMAGE_MODIFIER, 15)); } }
    SCENE { HP_BAR(opponent, captureDamage: &damage); }
    THEN { if (B_POLISHED_MOVE_BALANCE) EXPECT_EQ(damage, (22 * GetMovePower(move) * 101 / 73 / 50 + 2) * (move == MOVE_HEX ? 4 : 2) * 85 / 100); }
}

SINGLE_BATTLE_TEST("Polished damage uses exact ordered defensive ability and Assault Vest fractions")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(50); Attack(101); SpAttack(101); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Defense(73); SpDefense(73); Ability(ABILITY_NONE); }
    } WHEN { TURN {} }
    THEN {
        if (B_POLISHED_MOVE_BALANCE) {
            struct DamageContext ctx = {0};
            ctx.move = MOVE_EMBER; ctx.moveType = TYPE_FIRE; ctx.battlerAtk = 0; ctx.battlerDef = 1;
            ctx.typeEffectivenessModifier = UQ_4_12(1.0);
            ctx.abilities[0] = ABILITY_SOLAR_POWER; ctx.abilities[1] = ABILITY_DRY_SKIN;
            ctx.holdEffects[0] = HOLD_EFFECT_LIFE_ORB; ctx.holdEffects[1] = HOLD_EFFECT_ASSAULT_VEST;
            ctx.weather = B_WEATHER_SUN; ctx.isCrit = TRUE;
            u32 n = 22 * 40 * 101;
            n = n * 3 / 2; n = n * 5 / 4; n = n * 3 / 2; n = n * 13 / 10; n = n * 2 / 3;
            EXPECT_EQ(CalculateMoveDamageVars(&ctx), (n / 73 / 50 + 2) * 24 / 16);
        }
    }
}

SINGLE_BATTLE_TEST("Polished damage criticals ignore screens negative Attack positive Defense and native generation")
{
    u32 generation = GEN_3;
    PARAMETRIZE { generation = GEN_3; }
    PARAMETRIZE { generation = GEN_9; }
    GIVEN {
        WITH_CONFIG(B_CRIT_MULTIPLIER, generation);
        PLAYER(SPECIES_WOBBUFFET) { Level(50); Attack(101); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Defense(73); Ability(ABILITY_NONE); }
    } WHEN { TURN {} }
    THEN {
        if (B_POLISHED_MOVE_BALANCE) {
            struct DamageContext ctx = {0};
            ctx.move = MOVE_TACKLE; ctx.moveType = TYPE_NORMAL; ctx.battlerDef = 1;
            ctx.typeEffectivenessModifier = UQ_4_12(1.0); ctx.isCrit = TRUE;
            gSideStatuses[B_SIDE_OPPONENT] |= SIDE_STATUS_REFLECT;
            player->statStages[STAT_ATK] = 0; opponent->statStages[STAT_DEF] = 12;
            EXPECT_EQ(CalculateMoveDamageVars(&ctx), 22 * 40 * 101 * 3 / 2 / 73 / 50 + 2);
            ctx.abilities[0] = ABILITY_SNIPER;
            EXPECT_EQ(CalculateMoveDamageVars(&ctx), 22 * 40 * 101 * 9 / 4 / 73 / 50 + 2);
        }
    }
}

SINGLE_BATTLE_TEST("Polished damage Unaware ignores only the opposing stat stages")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(50); Attack(101); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Defense(73); Ability(ABILITY_NONE); }
    } WHEN { TURN {} }
    THEN {
        if (B_POLISHED_MOVE_BALANCE) {
            struct DamageContext ctx = {0};
            ctx.move = MOVE_TACKLE; ctx.moveType = TYPE_NORMAL; ctx.battlerDef = 1;
            ctx.typeEffectivenessModifier = UQ_4_12(1.0);
            player->statStages[STAT_ATK] = 8; opponent->statStages[STAT_DEF] = 8;
            ctx.abilities[0] = ABILITY_UNAWARE;
            EXPECT_EQ(CalculateMoveDamageVars(&ctx), 22 * 40 * 101 * 2 / 73 / 50 + 2);
            ctx.abilities[0] = ABILITY_NONE; ctx.abilities[1] = ABILITY_UNAWARE;
            EXPECT_EQ(CalculateMoveDamageVars(&ctx), 22 * 40 * 101 / 2 / 73 / 50 + 2);
        }
    }
}

SINGLE_BATTLE_TEST("Polished damage weather uses the targets Umbrella and respects suppression")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(50); SpAttack(101); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { SpDefense(73); Ability(ABILITY_NONE); }
    } WHEN { TURN {} }
    THEN {
        if (B_POLISHED_MOVE_BALANCE) {
            struct DamageContext ctx = {0};
            ctx.move = MOVE_EMBER; ctx.moveType = TYPE_FIRE; ctx.battlerDef = 1;
            ctx.typeEffectivenessModifier = UQ_4_12(1.0);
            u32 base = 22 * 40 * 101 / 73 / 50 + 2;
            ctx.weather = B_WEATHER_SUN_NORMAL; ctx.holdEffects[0] = HOLD_EFFECT_UTILITY_UMBRELLA;
            EXPECT_EQ(CalculateMoveDamageVars(&ctx), base * 3 / 2);
            ctx.holdEffects[1] = HOLD_EFFECT_UTILITY_UMBRELLA;
            EXPECT_EQ(CalculateMoveDamageVars(&ctx), base);
            ctx.holdEffects[1] = HOLD_EFFECT_NONE; ctx.weather = 0;
            EXPECT_EQ(CalculateMoveDamageVars(&ctx), base);
            ctx.weather = B_WEATHER_RAIN_NORMAL;
            EXPECT_EQ(CalculateMoveDamageVars(&ctx), base / 2);
        }
    }
}

SINGLE_BATTLE_TEST("Polished damage Psystrike uses physical defense modifiers with special attacker boosts")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(50); SpAttack(101); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Defense(73); SpDefense(211); Ability(ABILITY_NONE); Status1(STATUS1_PARALYSIS); }
    } WHEN { TURN {} }
    THEN {
        if (B_POLISHED_MOVE_BALANCE) {
            struct DamageContext ctx = {0};
            ctx.move = MOVE_PSYSTRIKE; ctx.moveType = TYPE_NORMAL; ctx.battlerDef = 1;
            ctx.typeEffectivenessModifier = UQ_4_12(1.0);
            ctx.holdEffects[0] = HOLD_EFFECT_CHOICE_SPECS; ctx.holdEffects[1] = HOLD_EFFECT_ASSAULT_VEST;
            ctx.abilities[1] = ABILITY_MARVEL_SCALE;
            EXPECT_EQ(CalculateMoveDamageVars(&ctx), (22 * 100 * 101 * 2 / 3 * 3 / 2) / 73 / 50 + 2);
            ctx.abilities[1] = ABILITY_FUR_COAT;
            EXPECT_EQ(CalculateMoveDamageVars(&ctx), (22 * 100 * 101 / 2 * 3 / 2) / 73 / 50 + 2);
        }
    }
}

SINGLE_BATTLE_TEST("Polished damage defender abilities reduce numerator before the constant two")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(50); SpAttack(101); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { SpDefense(73); Ability(ABILITY_NONE); }
    } WHEN { TURN {} }
    THEN {
        if (B_POLISHED_MOVE_BALANCE) {
            struct DamageContext ctx = {0};
            ctx.move = MOVE_EMBER; ctx.moveType = TYPE_FIRE; ctx.battlerDef = 1;
            ctx.typeEffectivenessModifier = UQ_4_12(1.0);
            ctx.abilities[1] = ABILITY_THICK_FAT;
            EXPECT_EQ(CalculateMoveDamageVars(&ctx), 22 * 40 * 101 / 2 / 73 / 50 + 2);
            ctx.abilities[1] = ABILITY_MULTISCALE;
            EXPECT_EQ(CalculateMoveDamageVars(&ctx), 22 * 40 * 101 / 2 / 73 / 50 + 2);
            opponent->hp--;
            EXPECT_EQ(CalculateMoveDamageVars(&ctx), 22 * 40 * 101 / 73 / 50 + 2);
            ctx.abilities[1] = ABILITY_DRY_SKIN;
            EXPECT_EQ(CalculateMoveDamageVars(&ctx), 22 * 40 * 101 * 5 / 4 / 73 / 50 + 2);
        }
    }
}

SINGLE_BATTLE_TEST("Polished damage applies Filter Tinted Lens and Expert Belt in the source second pass")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(50); Attack(101); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Defense(73); Ability(ABILITY_NONE); }
    } WHEN { TURN {} }
    THEN {
        if (B_POLISHED_MOVE_BALANCE) {
            struct DamageContext ctx = {0};
            ctx.move = MOVE_TACKLE; ctx.moveType = TYPE_NORMAL; ctx.battlerDef = 1;
            ctx.typeEffectivenessModifier = UQ_4_12(2.0);
            ctx.abilities[1] = ABILITY_FILTER; ctx.holdEffects[0] = HOLD_EFFECT_EXPERT_BELT;
            u32 base = 22 * 40 * 101 / 73 / 50 + 2;
            EXPECT_EQ(CalculateMoveDamageVars(&ctx), base * 32 * 3 / 4 * 6 / 5 / 16);
            ctx.abilities[1] = ABILITY_SOLID_ROCK;
            EXPECT_EQ(CalculateMoveDamageVars(&ctx), base * 32 * 3 / 4 * 6 / 5 / 16);
            ctx.typeEffectivenessModifier = UQ_4_12(0.25); ctx.abilities[0] = ABILITY_TINTED_LENS;
            EXPECT_EQ(CalculateMoveDamageVars(&ctx), base * 4 * 2 / 16);
        }
    }
}

SINGLE_BATTLE_TEST("Polished damage Knock Off and Pursuit boost after the random roll")
{
    enum Move move = MOVE_KNOCK_OFF;
    PARAMETRIZE { move = MOVE_KNOCK_OFF; }
    PARAMETRIZE { move = MOVE_PURSUIT; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(50); Attack(101); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Defense(73); Ability(ABILITY_NONE); Item(ITEM_LEFTOVERS); }
    } WHEN { TURN {} }
    THEN {
        if (B_POLISHED_MOVE_BALANCE) {
            struct DamageContext ctx = {0};
            ctx.move = move; ctx.moveType = TYPE_NORMAL; ctx.battlerDef = 1;
            ctx.typeEffectivenessModifier = UQ_4_12(1.0);
            gBattleStruct->battlerState[1].pursuitTarget = TRUE;
            u32 base = 22 * GetMovePower(move) * 101 / 73 / 50 + 2;
            EXPECT_EQ(CalculateMoveDamageVars(&ctx), base);
            for (u32 roll = 85; roll <= 100; roll++) {
                u32 rolled = base * roll / 100;
                EXPECT_EQ(ApplyModifiersAfterDmgRoll(&ctx, rolled), move == MOVE_PURSUIT ? rolled * 2 : rolled + rolled / 2);
            }
        }
    }
}

SINGLE_BATTLE_TEST("Polished damage Rollout multiplies the completed formula and caps before the roll")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(50); Attack(101); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Defense(73); Ability(ABILITY_NONE); }
    } WHEN { TURN {} }
    THEN {
        if (B_POLISHED_MOVE_BALANCE) {
            struct DamageContext ctx = {0};
            ctx.move = MOVE_ROLLOUT; ctx.moveType = TYPE_NORMAL; ctx.battlerDef = 1;
            ctx.typeEffectivenessModifier = UQ_4_12(1.0);
            u32 base = 22 * GetMovePower(MOVE_ROLLOUT) * 101 / 73 / 50 + 2;
            for (u32 curled = 0; curled <= 1; curled++) for (u32 hits = 0; hits < 5; hits++) {
                player->volatiles.rolloutTimer = hits; player->volatiles.defenseCurl = curled;
                EXPECT_EQ(CalculateMoveDamageVars(&ctx), base << (hits + curled));
            }
        }
    }
}

SINGLE_BATTLE_TEST("Polished damage Metal Powder boosts transformed Ditto Defense by one half")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(50); Attack(101); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_DITTO) { Defense(73); Ability(ABILITY_NONE); Item(ITEM_METAL_POWDER); }
    } WHEN { TURN {} }
    THEN {
        if (B_POLISHED_MOVE_BALANCE) {
            struct DamageContext ctx = {0};
            ctx.move = MOVE_TACKLE; ctx.moveType = TYPE_NORMAL; ctx.battlerDef = 1;
            ctx.typeEffectivenessModifier = UQ_4_12(1.0); ctx.holdEffects[1] = HOLD_EFFECT_METAL_POWDER;
            EXPECT_EQ(CalculateMoveDamageVars(&ctx), 22 * 40 * 101 / 109 / 50 + 2);
            opponent->volatiles.transformed = TRUE; opponent->volatiles.transformedMonSpecies = SPECIES_DITTO; opponent->species = SPECIES_WOBBUFFET;
            EXPECT_EQ(CalculateMoveDamageVars(&ctx), 22 * 40 * 101 / 109 / 50 + 2);
            opponent->volatiles.transformedMonSpecies = SPECIES_WOBBUFFET; opponent->species = SPECIES_DITTO;
            EXPECT_EQ(CalculateMoveDamageVars(&ctx), 22 * 40 * 101 / 73 / 50 + 2);
        }
    }
}

SINGLE_BATTLE_TEST("Polished damage Light Ball and Thick Club double raw Attack before compression")
{
    GIVEN {
        PLAYER(SPECIES_PIKACHU) { Level(50); Attack(131); SpAttack(131); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Defense(101); SpDefense(101); Ability(ABILITY_NONE); }
    } WHEN { TURN {} }
    THEN {
        if (B_POLISHED_MOVE_BALANCE) {
            struct DamageContext ctx = {0};
            ctx.move = MOVE_TACKLE; ctx.moveType = TYPE_NORMAL; ctx.battlerDef = 1;
            ctx.typeEffectivenessModifier = UQ_4_12(1.0); ctx.holdEffects[0] = HOLD_EFFECT_LIGHT_BALL;
            EXPECT_EQ(CalculateMoveDamageVars(&ctx), 22 * 40 * 131 / 50 / 50 + 2);
            static const enum Species forms[] = {SPECIES_PIKACHU_FLY, SPECIES_PIKACHU_SURF, SPECIES_PIKACHU_RED, SPECIES_PIKACHU_YELLOW, SPECIES_PIKACHU_SPARK};
            for (u32 i = 0; i < ARRAY_COUNT(forms); i++) {
                player->species = forms[i];
                EXPECT_EQ(CalculateMoveDamageVars(&ctx), 22 * 40 * 131 / 50 / 50 + 2);
            }
            ctx.move = MOVE_EMBER;
            EXPECT_EQ(CalculateMoveDamageVars(&ctx), 22 * 40 * 131 / 50 / 50 + 2);
            player->species = SPECIES_MAROWAK_ALOLA; ctx.holdEffects[0] = HOLD_EFFECT_THICK_CLUB; ctx.move = MOVE_TACKLE;
            EXPECT_EQ(CalculateMoveDamageVars(&ctx), 22 * 40 * 131 / 50 / 50 + 2);
            ctx.move = MOVE_EMBER;
            EXPECT_EQ(CalculateMoveDamageVars(&ctx), 22 * 40 * 131 / 101 / 50 + 2);
        }
    }
}

SINGLE_BATTLE_TEST("Polished damage keeps optional moves abilities items terrain and gimmicks on native paths")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(50); Attack(101); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Defense(73); Ability(ABILITY_NONE); }
    } WHEN { TURN {} }
    THEN {
        struct DamageContext ctx = {0};
        ctx.move = MOVE_TACKLE; ctx.moveType = TYPE_NORMAL; ctx.battlerDef = 1;
        ctx.typeEffectivenessModifier = UQ_4_12(1.0);
        EXPECT_EQ(UsesPolishedDamageRules(&ctx), B_POLISHED_MOVE_BALANCE);
        ctx.move = MOVE_POUND; EXPECT_EQ(UsesPolishedDamageRules(&ctx), FALSE);
        ctx.move = MOVE_TACKLE; ctx.abilities[0] = ABILITY_PROTOSYNTHESIS; EXPECT_EQ(UsesPolishedDamageRules(&ctx), FALSE);
        ctx.abilities[0] = ABILITY_NONE; ctx.holdEffects[1] = HOLD_EFFECT_RESIST_BERRY; EXPECT_EQ(UsesPolishedDamageRules(&ctx), FALSE);
        ctx.holdEffects[1] = HOLD_EFFECT_NONE; ctx.terrain = B_TERRAIN_GRASSY; EXPECT_EQ(UsesPolishedDamageRules(&ctx), FALSE);
        ctx.terrain = 0;
        ctx.chosenMove = MOVE_ME_FIRST; EXPECT_EQ(UsesPolishedDamageRules(&ctx), FALSE);
        ctx.chosenMove = MOVE_TACKLE;
        opponent->volatiles.glaiveRush = TRUE; EXPECT_EQ(UsesPolishedDamageRules(&ctx), FALSE);
        opponent->volatiles.glaiveRush = FALSE;
        opponent->volatiles.semiInvulnerable = STATE_UNDERWATER; EXPECT_EQ(UsesPolishedDamageRules(&ctx), FALSE);
        opponent->volatiles.semiInvulnerable = STATE_NONE;
        SetActiveGimmick(0, GIMMICK_TERA); EXPECT_EQ(UsesPolishedDamageRules(&ctx), FALSE);
        SetActiveGimmick(0, GIMMICK_NONE); SetActiveGimmick(1, GIMMICK_TERA); EXPECT_EQ(UsesPolishedDamageRules(&ctx), FALSE);
        SetActiveGimmick(1, GIMMICK_NONE);
    }
}

DOUBLE_BATTLE_TEST("Polished damage leaves doubles arithmetic on the native path")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET); PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET); OPPONENT(SPECIES_WOBBUFFET);
    } WHEN { TURN {} }
    THEN {
        struct DamageContext ctx = {0}; ctx.move = MOVE_TACKLE; ctx.battlerDef = 1;
        EXPECT_EQ(UsesPolishedDamageRules(&ctx), FALSE);
    }
}

SINGLE_BATTLE_TEST("Polished damage Reckless follows source effects including the Flare Blitz exception")
{
    enum Move move = MOVE_TAKE_DOWN;
    PARAMETRIZE { move = MOVE_TAKE_DOWN; }
    PARAMETRIZE { move = MOVE_DOUBLE_EDGE; }
    PARAMETRIZE { move = MOVE_BRAVE_BIRD; }
    PARAMETRIZE { move = MOVE_WILD_CHARGE; }
    PARAMETRIZE { move = MOVE_FLARE_BLITZ; }
    PARAMETRIZE { move = MOVE_STRUGGLE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(50); Attack(101); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Defense(73); Ability(ABILITY_NONE); }
    } WHEN { TURN {} }
    THEN {
        if (B_POLISHED_MOVE_BALANCE) {
            struct DamageContext ctx = {0};
            ctx.move = move; ctx.moveType = TYPE_NORMAL; ctx.battlerDef = 1;
            ctx.typeEffectivenessModifier = UQ_4_12(1.0); ctx.abilities[0] = ABILITY_RECKLESS;
            u32 numerator = 22 * GetMovePower(move) * 101;
            if (move != MOVE_FLARE_BLITZ && move != MOVE_STRUGGLE) numerator = numerator * 6 / 5;
            EXPECT_EQ(CalculateMoveDamageVars(&ctx), numerator / 73 / 50 + 2);
        }
    }
}

SINGLE_BATTLE_TEST("Polished damage punching slicing and launcher boosts use the pinned move memberships")
{
    enum Move move = MOVE_FIRE_PUNCH;
    enum Ability ability = ABILITY_IRON_FIST;
    u32 n = 6, d = 5;
    PARAMETRIZE { move = MOVE_FIRE_PUNCH; ability = ABILITY_IRON_FIST; n = 6; d = 5; }
    PARAMETRIZE { move = MOVE_MACH_PUNCH; ability = ABILITY_IRON_FIST; n = 6; d = 5; }
    PARAMETRIZE { move = MOVE_TACKLE; ability = ABILITY_IRON_FIST; n = 1; d = 1; }
    PARAMETRIZE { move = MOVE_CUT; ability = ABILITY_SHARPNESS; n = 3; d = 2; }
    PARAMETRIZE { move = MOVE_AERIAL_ACE; ability = ABILITY_SHARPNESS; n = 3; d = 2; }
    PARAMETRIZE { move = MOVE_TACKLE; ability = ABILITY_SHARPNESS; n = 1; d = 1; }
    PARAMETRIZE { move = MOVE_AURA_SPHERE; ability = ABILITY_MEGA_LAUNCHER; n = 3; d = 2; }
    PARAMETRIZE { move = MOVE_WATER_PULSE; ability = ABILITY_MEGA_LAUNCHER; n = 3; d = 2; }
    PARAMETRIZE { move = MOVE_TACKLE; ability = ABILITY_MEGA_LAUNCHER; n = 1; d = 1; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(50); Attack(101); SpAttack(101); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Defense(73); SpDefense(73); Ability(ABILITY_NONE); }
    } WHEN { TURN {} }
    THEN {
        if (B_POLISHED_MOVE_BALANCE) {
            struct DamageContext ctx = {0};
            ctx.move = move; ctx.moveType = TYPE_NORMAL; ctx.battlerDef = 1;
            ctx.typeEffectivenessModifier = UQ_4_12(1.0); ctx.abilities[0] = ability;
            EXPECT_EQ(CalculateMoveDamageVars(&ctx), 22 * GetMovePower(move) * 101 * n / d / 73 / 50 + 2);
        }
    }
}

SINGLE_BATTLE_TEST("Polished damage Sheer Force and Punching Glove multiply separately before division")
{
    enum Move move = MOVE_FIRE_PUNCH;
    PARAMETRIZE { move = MOVE_FIRE_PUNCH; }
    PARAMETRIZE { move = MOVE_MACH_PUNCH; }
    PARAMETRIZE { move = MOVE_TACKLE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(50); Attack(101); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Defense(73); Ability(ABILITY_NONE); }
    } WHEN { TURN {} }
    THEN {
        if (B_POLISHED_MOVE_BALANCE) {
            struct DamageContext ctx = {0};
            ctx.move = move; ctx.moveType = TYPE_NORMAL; ctx.battlerDef = 1;
            ctx.typeEffectivenessModifier = UQ_4_12(1.0);
            ctx.abilities[0] = ABILITY_SHEER_FORCE; ctx.holdEffects[0] = HOLD_EFFECT_PUNCHING_GLOVE;
            u32 n = 22 * GetMovePower(move) * 101;
            if (move == MOVE_FIRE_PUNCH) n = n * 13 / 10;
            if (move != MOVE_TACKLE) n = n * 11 / 10;
            EXPECT_EQ(CalculateMoveDamageVars(&ctx), n / 73 / 50 + 2);
        }
    }
}

SINGLE_BATTLE_TEST("Polished damage Pixilate boosts only moves actually converted and ignores the native multiplier preset")
{
    u32 generation = GEN_6;
    PARAMETRIZE { generation = GEN_6; }
    PARAMETRIZE { generation = GEN_9; }
    s16 damage;
    GIVEN {
        WITH_CONFIG(B_ATE_MULTIPLIER, generation);
        PLAYER(SPECIES_WOBBUFFET) { Level(50); Attack(101); Ability(ABILITY_PIXILATE); Speed(200); }
        OPPONENT(SPECIES_WOBBUFFET) { Defense(73); Ability(ABILITY_NONE); Speed(100); }
    } WHEN { TURN { MOVE(player, MOVE_TACKLE, WITH_RNG(RNG_DAMAGE_MODIFIER, 0)); } }
    SCENE { HP_BAR(opponent, captureDamage: &damage); }
    THEN { if (B_POLISHED_MOVE_BALANCE) EXPECT_EQ(damage, 22 * 40 * 101 * 6 / 5 / 73 / 50 + 2); }
}

SINGLE_BATTLE_TEST("Polished damage Hidden Power does not receive a Pixilate bonus from its listed type")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(50); SpAttack(101); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { SpDefense(73); Ability(ABILITY_NONE); }
    } WHEN { TURN {} }
    THEN {
        if (B_POLISHED_MOVE_BALANCE) {
            struct DamageContext ctx = {0};
            ctx.move = MOVE_HIDDEN_POWER; ctx.moveType = TYPE_FIRE; ctx.battlerDef = 1;
            ctx.typeEffectivenessModifier = UQ_4_12(1.0); ctx.abilities[0] = ABILITY_PIXILATE;
            EXPECT_EQ(CalculateMoveDamageVars(&ctx), 22 * GetMovePower(MOVE_HIDDEN_POWER) * 101 / 73 / 50 + 2);
        }
    }
}

SINGLE_BATTLE_TEST("Polished damage preserves immunity and zero power Return through the AI roll interface")
{
    GIVEN {
        WITH_CONFIG(B_POLISHED_RETURN, TRUE);
        PLAYER(SPECIES_WOBBUFFET) { Friendship(1); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN { TURN {} }
    THEN {
        if (B_POLISHED_MOVE_BALANCE) {
            struct DamageContext ctx = {0};
            ctx.move = MOVE_RETURN; ctx.moveType = TYPE_NORMAL; ctx.battlerDef = 1;
            ctx.typeEffectivenessModifier = UQ_4_12(1.0);
            EXPECT_EQ(CalculateMoveDamageVars(&ctx), 1);
            EXPECT_EQ(ApplyModifiersAfterDmgRoll(&ctx, 0), 1);
            ctx.typeEffectivenessModifier = UQ_4_12(0.0);
            EXPECT_EQ(ApplyModifiersAfterDmgRoll(&ctx, 1), 0);
        }
    }
}

DOUBLE_BATTLE_TEST("Polished damage AI cached native modifiers match fresh rolls and reset per calculation")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(50); Attack(101); Ability(ABILITY_NONE); }
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Defense(73); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN { TURN {} }
    THEN {
        struct DamageContext ctx = {0};
        ctx.battlerDef = 1; ctx.move = MOVE_TACKLE; ctx.moveType = TYPE_NORMAL;
        ctx.aiCalc = TRUE;
        for (u32 variant = 0; variant < 4; variant++) {
            ctx.typeEffectivenessModifier = variant & 1 ? UQ_4_12(2.0) : UQ_4_12(1.0);
            ctx.holdEffects[0] = variant & 1 ? HOLD_EFFECT_LIFE_ORB : HOLD_EFFECT_EXPERT_BELT;
            ctx.holdEffects[1] = variant & 2 ? HOLD_EFFECT_RESIST_BERRY : HOLD_EFFECT_NONE;
            ctx.abilities[1] = variant & 1 ? ABILITY_FILTER : ABILITY_MULTISCALE;
            ctx.abilities[3] = variant & 2 ? ABILITY_FRIEND_GUARD : ABILITY_NONE;
            gBattleMons[1].item = variant & 2 ? ITEM_CHILAN_BERRY : ITEM_NONE;
            gSideStatuses[B_SIDE_OPPONENT] = variant & 1 ? SIDE_STATUS_REFLECT : 0;
            ctx.aiCheckBerryModifier = FALSE;
            s32 base = CalculateMoveDamageVars(&ctx);
            EXPECT_EQ((u32)ctx.aiOtherModifier, 0);
            for (u32 roll = 85; roll <= 100; roll++) {
                struct DamageContext fresh = ctx;
                fresh.aiOtherModifier = 0;
                fresh.aiCheckBerryModifier = FALSE;
                s32 expected = ApplyModifiersAfterDmgRoll(&fresh, base * roll / 100);
                EXPECT_EQ(ApplyModifiersAfterDmgRoll(&ctx, base * roll / 100), expected);
                EXPECT_EQ((u32)ctx.aiCheckBerryModifier, (u32)fresh.aiCheckBerryModifier);
            }
            EXPECT_NE((u32)ctx.aiOtherModifier, 0);
        }
    }
}
