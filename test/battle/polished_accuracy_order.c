#include "global.h"
#include "test/battle.h"
#include "battle_main.h"
#include "battle_util.h"
#include "polished_accuracy_vectors.h"

SINGLE_BATTLE_TEST("Polished accuracy preserves lowered evasion for Keen Eye Illuminate and Minds Eye")
{
    enum Ability ability = ABILITY_KEEN_EYE;
    struct BattlePokemon *user = player, *target = opponent;
    for (u32 side = 0; side < 2; side++) for (u32 k = 0; k < 3; k++)
        PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; ability = k == 0 ? ABILITY_KEEN_EYE : k == 1 ? ABILITY_ILLUMINATE : ABILITY_MINDS_EYE; }
    PASSES_RANDOMLY(B_POLISHED_MOVE_BALANCE ? 83 : 50, 100, RNG_ACCURACY);
    GIVEN {
        WITH_CONFIG(B_ILLUMINATE_EFFECT, GEN_9);
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ability : ABILITY_NONE); Speed(200); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ability : ABILITY_NONE); Speed(100); }
    } WHEN {
        TURN { MOVE(user, MOVE_SWEET_SCENT, hit: TRUE); }
        TURN { MOVE(user, MOVE_ZAP_CANNON); }
    } SCENE { ANIMATION(ANIM_TYPE_MOVE, MOVE_ZAP_CANNON, user); HP_BAR(target); }
}

SINGLE_BATTLE_TEST("Polished accuracy Illuminate ignores positive evasion under old native presets")
{
    PASSES_RANDOMLY(B_POLISHED_MOVE_BALANCE ? 50 : 37, 100, RNG_ACCURACY);
    GIVEN {
        WITH_CONFIG(B_ILLUMINATE_EFFECT, GEN_3);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_ILLUMINATE); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(200); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_DOUBLE_TEAM); }
        TURN { MOVE(player, MOVE_ZAP_CANNON); }
    } SCENE { ANIMATION(ANIM_TYPE_MOVE, MOVE_ZAP_CANNON, player); HP_BAR(opponent); }
}

SINGLE_BATTLE_TEST("Polished accuracy sun uses the pinned raw Thunder and Hurricane byte")
{
    enum Move move = MOVE_THUNDER;
    u32 drops = 0;
    for (u32 m = 0; m < 2; m++) for (u32 d = 0; d < 3; d++)
        PARAMETRIZE { move = m ? MOVE_HURRICANE : MOVE_THUNDER; drops = d; }
    PASSES_RANDOMLY(min(100, (B_POLISHED_MOVE_BALANCE ? 128 : 50) * 3 / (3 + drops)), 100, RNG_ACCURACY);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(200); }
    } WHEN {
        for (u32 d = 0; d < drops; d++) TURN { MOVE(opponent, MOVE_SAND_ATTACK, hit: TRUE); }
        TURN { MOVE(opponent, MOVE_SUNNY_DAY); MOVE(player, move); }
    } SCENE { ANIMATION(ANIM_TYPE_MOVE, move, player); HP_BAR(opponent); }
}

SINGLE_BATTLE_TEST("Polished accuracy rain checks the targets Utility Umbrella")
{
    enum Move move = MOVE_THUNDER;
    bool32 userUmbrella = FALSE;
    struct BattlePokemon *user = player, *target = opponent;
    for (u32 side = 0; side < 2; side++) for (u32 m = 0; m < 2; m++) for (u32 item = 0; item < 2; item++)
        PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; move = m ? MOVE_HURRICANE : MOVE_THUNDER; userUmbrella = item; }
    if (!(B_POLISHED_MOVE_BALANCE ? userUmbrella : !userUmbrella))
        PASSES_RANDOMLY(70, 100, RNG_ACCURACY);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(user == player ? 100 : 200); Item((user == player) == userUmbrella ? ITEM_UTILITY_UMBRELLA : ITEM_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(user == opponent ? 100 : 200); Item((user == opponent) == userUmbrella ? ITEM_UTILITY_UMBRELLA : ITEM_NONE); }
    } WHEN { TURN { MOVE(target, MOVE_RAIN_DANCE); MOVE(user, move); } }
    SCENE { ANIMATION(ANIM_TYPE_MOVE, move, user); HP_BAR(target); }
}

SINGLE_BATTLE_TEST("Polished accuracy Zoom Lens activates against a replacement that switched first")
{
    PASSES_RANDOMLY(B_POLISHED_MOVE_BALANCE ? 60 : 50, 100, RNG_ACCURACY);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_ZOOM_LENS); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); }
    } WHEN { TURN { SWITCH(opponent, 1); MOVE(player, MOVE_ZAP_CANNON); } }
    SCENE { ANIMATION(ANIM_TYPE_MOVE, MOVE_ZAP_CANNON, player); HP_BAR(opponent); }
}

SINGLE_BATTLE_TEST("Polished accuracy Lock On cannot bypass Fly and Dig")
{
    enum Move hidden = MOVE_FLY;
    PARAMETRIZE { hidden = MOVE_FLY; }
    PARAMETRIZE { hidden = MOVE_DIG; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(200); }
    } WHEN {
        TURN { MOVE(player, MOVE_LOCK_ON); }
        TURN { MOVE(opponent, hidden); MOVE(player, MOVE_TACKLE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_LOCK_ON, player);
        ANIMATION(ANIM_TYPE_MOVE, hidden, opponent);
        if (B_POLISHED_MOVE_BALANCE) NOT HP_BAR(opponent);
        else HP_BAR(opponent);
    }
}

SINGLE_BATTLE_TEST("Polished order Custap waits when the two selected moves have different priority")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_CUSTAP_BERRY); HP(40); MaxHP(160); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Attack(1); }
    } WHEN { TURN { MOVE(player, MOVE_SPLASH); MOVE(opponent, MOVE_QUICK_ATTACK); } }
    THEN { EXPECT_EQ(player->item, B_POLISHED_MOVE_BALANCE ? ITEM_CUSTAP_BERRY : ITEM_NONE); }
}

SINGLE_BATTLE_TEST("Polished accuracy all 255 source rows use exact combined stage arithmetic")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN { TURN {} }
    THEN {
        struct BattleCalcValues cv = {0};
        cv.battlerAtk = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);
        cv.battlerDef = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);
        for (u32 v = 0; v < ARRAY_COUNT(sPolishedAccuracyVectors); v++)
        {
            const typeof(*sPolishedAccuracyVectors) *vector = &sPolishedAccuracyVectors[v];
            player->statStages[STAT_ACC] = vector->attackStage;
            opponent->statStages[STAT_EVASION] = vector->defenseStage;
            for (u32 m = 0; m < ARRAY_COUNT(sPolishedAccuracyMoves); m++)
            {
                if (sPolishedAccuracyMoves[m].accuracy != vector->accuracy)
                    continue;
                cv.move = sPolishedAccuracyMoves[m].move;
                EXPECT_EQ(GetPolishedTotalAccuracy(&cv, 0), vector->result);
            }
            if (vector->accuracy == 128)
            {
                cv.move = MOVE_THUNDER;
                EXPECT_EQ(GetPolishedTotalAccuracy(&cv, B_WEATHER_SUN), vector->result);
                cv.move = MOVE_HURRICANE;
                EXPECT_EQ(GetPolishedTotalAccuracy(&cv, B_WEATHER_SUN), vector->result);
            }
        }
    }
}

SINGLE_BATTLE_TEST("Polished accuracy modifier ordering floors stages abilities and each held item separately")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN { TURN {} }
    THEN {
        static const enum Ability attackers[] = {ABILITY_NONE, ABILITY_COMPOUND_EYES, ABILITY_HUSTLE};
        static const enum Ability defenders[] = {ABILITY_NONE, ABILITY_TANGLED_FEET, ABILITY_SAND_VEIL, ABILITY_SNOW_CLOAK};
        struct BattleCalcValues cv = {0};
        cv.battlerAtk = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);
        cv.battlerDef = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);
        for (u32 a = 0; a < 3; a++) for (u32 d = 0; d < 4; d++)
        for (u32 stage = 0; stage <= 12; stage++) for (u32 item = 0; item < 4; item++)
        for (u32 physical = 0; physical < 2; physical++)
        {
            u32 expected = 50 * (3 + (stage > 6 ? stage - 6 : 0)) / (3 + (stage < 6 ? 6 - stage : 0));
            cv.move = physical ? MOVE_DYNAMIC_PUNCH : MOVE_ZAP_CANNON;
            cv.abilities[cv.battlerAtk] = attackers[a];
            cv.abilities[cv.battlerDef] = defenders[d];
            player->statStages[STAT_ACC] = stage;
            opponent->volatiles.confusionTimer = d == 1;
            player->item = item & 1 ? ITEM_WIDE_LENS : ITEM_NONE;
            opponent->item = item & 2 ? ITEM_BRIGHT_POWDER : ITEM_NONE;
            cv.holdEffects[cv.battlerAtk] = item & 1 ? HOLD_EFFECT_WIDE_LENS : HOLD_EFFECT_NONE;
            cv.holdEffects[cv.battlerDef] = item & 2 ? HOLD_EFFECT_EVASION_UP : HOLD_EFFECT_NONE;
            if (a == 1) expected = expected * 13 / 10;
            if (a == 2 && physical) expected = expected * 4 / 5;
            if (d == 1) expected /= 2;
            if (d >= 2) expected = expected * 4 / 5;
            if (item & 1) expected = expected * 11 / 10;
            if (item & 2) expected = expected * 9 / 10;
            EXPECT_EQ(GetPolishedTotalAccuracy(&cv, d == 2 ? B_WEATHER_SANDSTORM : d == 3 ? B_WEATHER_HAIL : 0), expected);
        }
    }
}

SINGLE_BATTLE_TEST("Polished accuracy Wonder Skin applies before stages Compound Eyes and Wide Lens")
{
    enum Move move = MOVE_FORESIGHT;
    PARAMETRIZE { move = MOVE_FORESIGHT; }
    PARAMETRIZE { move = MOVE_SING; }
    if (B_POLISHED_MOVE_BALANCE || move != MOVE_FORESIGHT)
        PASSES_RANDOMLY(52, 100, RNG_ACCURACY);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_COMPOUND_EYES); Item(ITEM_WIDE_LENS); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_WONDER_SKIN); Speed(200); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_SAND_ATTACK, hit: TRUE); }
        TURN { MOVE(player, move); }
    } SCENE { ANIMATION(ANIM_TYPE_MOVE, move, player); }
}

SINGLE_BATTLE_TEST("Polished accuracy Unaware ignores either sign of opposing stages")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN { TURN {} }
    THEN {
        struct BattleCalcValues cv = {0};
        cv.battlerAtk = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);
        cv.battlerDef = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);
        cv.move = MOVE_ZAP_CANNON;
        for (u32 stage = 0; stage <= 12; stage++)
        {
            player->statStages[STAT_ACC] = opponent->statStages[STAT_EVASION] = stage;
            cv.abilities[cv.battlerAtk] = cv.abilities[cv.battlerDef] = ABILITY_UNAWARE;
            EXPECT_EQ(GetPolishedTotalAccuracy(&cv, 0), 50);
        }
    }
}

SINGLE_BATTLE_TEST("Polished accuracy Mold Breaker bypasses accuracy abilities while Neutralizing Gas suppresses both sides")
{
    enum Ability ability = ABILITY_MOLD_BREAKER;
    PARAMETRIZE { ability = ABILITY_MOLD_BREAKER; }
    PARAMETRIZE { ability = ABILITY_NEUTRALIZING_GAS; }
    PASSES_RANDOMLY(GetMoveAccuracy(MOVE_SING), 100, RNG_ACCURACY);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ability); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_WONDER_SKIN); }
    } WHEN { TURN { MOVE(player, MOVE_SING); } }
    SCENE { ANIMATION(ANIM_TYPE_MOVE, MOVE_SING, player); STATUS_ICON(opponent, sleep: TRUE); }
}

SINGLE_BATTLE_TEST("Polished accuracy hail and sand abilities use exact four fifths and ignore Utility Umbrella")
{
    enum Ability ability = ABILITY_SNOW_CLOAK;
    enum Move weather = MOVE_HAIL;
    bool32 umbrella = FALSE;
    for (u32 w = 0; w < 2; w++) for (u32 item = 0; item < 2; item++)
        PARAMETRIZE { ability = w ? ABILITY_SAND_VEIL : ABILITY_SNOW_CLOAK; weather = w ? MOVE_SANDSTORM : MOVE_HAIL; umbrella = item; }
    PASSES_RANDOMLY(40, 100, RNG_ACCURACY);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ability); Item(umbrella ? ITEM_UTILITY_UMBRELLA : ITEM_NONE); Speed(200); }
    } WHEN { TURN { MOVE(opponent, weather); MOVE(player, MOVE_ZAP_CANNON); } }
    SCENE { ANIMATION(ANIM_TYPE_MOVE, MOVE_ZAP_CANNON, player); HP_BAR(opponent); }
}

SINGLE_BATTLE_TEST("Polished accuracy Cloud Nine suppresses sun rain hail and sand accuracy changes")
{
    enum Move weather = MOVE_SUNNY_DAY, move = MOVE_THUNDER;
    enum Ability ability = ABILITY_NONE;
    for (u32 w = 0; w < 4; w++) PARAMETRIZE {
        weather = w == 0 ? MOVE_SUNNY_DAY : w == 1 ? MOVE_RAIN_DANCE : w == 2 ? MOVE_HAIL : MOVE_SANDSTORM;
        move = w < 2 ? MOVE_THUNDER : MOVE_ZAP_CANNON;
        ability = w < 2 ? ABILITY_NONE : w == 2 ? ABILITY_SNOW_CLOAK : ABILITY_SAND_VEIL;
    }
    PASSES_RANDOMLY(GetMoveAccuracy(move), 100, RNG_ACCURACY);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_CLOUD_NINE); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ability); Speed(200); }
    } WHEN { TURN { MOVE(opponent, weather); MOVE(player, move); } }
    SCENE { ANIMATION(ANIM_TYPE_MOVE, move, player); HP_BAR(opponent); }
}

SINGLE_BATTLE_TEST("Polished accuracy No Guard still hits through Fly and Dig on either side")
{
    enum Move hidden = MOVE_FLY;
    bool32 defenderGuard = FALSE;
    for (u32 h = 0; h < 2; h++) for (u32 side = 0; side < 2; side++)
        PARAMETRIZE { hidden = h ? MOVE_DIG : MOVE_FLY; defenderGuard = side; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(defenderGuard ? ABILITY_NONE : ABILITY_NO_GUARD); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(defenderGuard ? ABILITY_NO_GUARD : ABILITY_NONE); Speed(200); }
    } WHEN { TURN { MOVE(opponent, hidden); MOVE(player, MOVE_TACKLE, WITH_RNG(RNG_ACCURACY, FALSE)); } }
    SCENE { HP_BAR(opponent); }
}

SINGLE_BATTLE_TEST("Polished accuracy Poison type Toxic bypasses Fly and Dig without spending a hit roll")
{
    enum Move hidden = MOVE_FLY;
    PARAMETRIZE { hidden = MOVE_FLY; }
    PARAMETRIZE { hidden = MOVE_DIG; }
    GIVEN {
        PLAYER(SPECIES_NIDOKING) { Ability(ABILITY_NONE); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(200); }
    } WHEN { TURN { MOVE(opponent, hidden); MOVE(player, MOVE_TOXIC, WITH_RNG(RNG_ACCURACY, FALSE)); } }
    SCENE { STATUS_ICON(opponent, badPoison: TRUE); }
}

SINGLE_BATTLE_TEST("Polished accuracy explicit guaranteed moves bypass stages items and abilities")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_HUSTLE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_WONDER_SKIN); Item(ITEM_BRIGHT_POWDER); }
    } WHEN { TURN {} }
    THEN {
        struct BattleCalcValues cv = {0};
        cv.battlerAtk = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);
        cv.battlerDef = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);
        cv.abilities[cv.battlerAtk] = ABILITY_HUSTLE;
        cv.abilities[cv.battlerDef] = ABILITY_WONDER_SKIN;
        cv.holdEffects[cv.battlerDef] = HOLD_EFFECT_EVASION_UP;
        player->statStages[STAT_ACC] = MIN_STAT_STAGE;
        opponent->statStages[STAT_EVASION] = MAX_STAT_STAGE;
        for (u32 i = 0; i < ARRAY_COUNT(sPolishedAccuracyMoves); i++) if (sPolishedAccuracyMoves[i].bypass)
        {
            cv.move = sPolishedAccuracyMoves[i].move;
            EXPECT_EQ(DoesMoveMissTarget(&cv), FALSE);
        }
    }
}

SINGLE_BATTLE_TEST("Polished accuracy optional moves retain native accuracy and Illuminate configuration")
{
    PASSES_RANDOMLY(37, 100, RNG_ACCURACY);
    GIVEN {
        WITH_CONFIG(B_ILLUMINATE_EFFECT, GEN_3);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_ILLUMINATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN { TURN { MOVE(opponent, MOVE_DOUBLE_TEAM); } TURN { MOVE(player, MOVE_INFERNO); } }
    SCENE { ANIMATION(ANIM_TYPE_MOVE, MOVE_INFERNO, player); HP_BAR(opponent); }
}

DOUBLE_BATTLE_TEST("Polished accuracy optional doubles retain native sun and Umbrella rules")
{
    PASSES_RANDOMLY(50, 100, RNG_ACCURACY);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(75); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(200); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(50); }
    } WHEN { TURN { MOVE(opponentLeft, MOVE_SUNNY_DAY); MOVE(playerLeft, MOVE_THUNDER, target: opponentLeft); } }
    SCENE { ANIMATION(ANIM_TYPE_MOVE, MOVE_THUNDER, playerLeft); HP_BAR(opponentLeft); }
}

SINGLE_BATTLE_TEST("Polished accuracy either active Terastal retains native sun rules")
{
    bool32 targetTera = FALSE;
    PARAMETRIZE { targetTera = FALSE; }
    PARAMETRIZE { targetTera = TRUE; }
    PASSES_RANDOMLY(50, 100, RNG_ACCURACY);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); TeraType(TYPE_PSYCHIC); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(200); TeraType(TYPE_PSYCHIC); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_SUNNY_DAY, gimmick: targetTera ? GIMMICK_TERA : GIMMICK_NONE); MOVE(player, MOVE_THUNDER, gimmick: targetTera ? GIMMICK_NONE : GIMMICK_TERA); }
    } SCENE { ANIMATION(ANIM_TYPE_MOVE, MOVE_THUNDER, player); HP_BAR(opponent); }
}

SINGLE_BATTLE_TEST("Polished order all 255 move priorities and Prankster increments match the source table")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN { TURN {} }
    THEN {
        for (u32 i = 0; i < ARRAY_COUNT(sPolishedAccuracyMoves); i++)
        {
            enum Move move = sPolishedAccuracyMoves[i].move;
            s32 base = B_POLISHED_MOVE_BALANCE ? sPolishedAccuracyMoves[i].priority : GetMovePriority(move);
            for (u32 side = 0; side < 2; side++)
            {
                enum BattlerId user = GetBattlerAtPosition(side ? B_POSITION_OPPONENT_LEFT : B_POSITION_PLAYER_LEFT);
                EXPECT_EQ(GetBattleMovePriority(user, ABILITY_NONE, move), base);
                EXPECT_EQ(GetBattleMovePriority(user, ABILITY_PRANKSTER, move), base + (GetMoveCategory(move) == DAMAGE_CATEGORY_STATUS));
            }
        }
    }
}

SINGLE_BATTLE_TEST("Polished order Quick Draw overrides its holders Lagging Tail when both quick effects trigger")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_QUICK_DRAW); Item(ITEM_LAGGING_TAIL); Speed(200); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_QUICK_DRAW); Speed(100); }
    } WHEN { TURN { MOVE(player, MOVE_TACKLE, WITH_RNG(RNG_QUICK_DRAW, TRUE)); MOVE(opponent, MOVE_TACKLE, WITH_RNG(RNG_QUICK_DRAW, TRUE)); } }
    SCENE {
        if (B_POLISHED_MOVE_BALANCE) { HP_BAR(opponent); HP_BAR(player); }
        else { HP_BAR(player); HP_BAR(opponent); }
    }
}

SINGLE_BATTLE_TEST("Polished order raw Speed comparisons ignore move only quick and lagging flags")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN { TURN {} }
    THEN {
        struct BattleCalcValues cv = {0};
        cv.battlerAtk = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);
        cv.battlerDef = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);
        for (u32 quick = 0; quick < 2; quick++) for (u32 room = 0; room < 2; room++)
        {
            gProtectStructs[cv.battlerAtk].laggingTail = !quick;
            gProtectStructs[cv.battlerDef].quickDraw = quick;
            if (room) gFieldStatuses |= STATUS_FIELD_TRICK_ROOM;
            else gFieldStatuses &= ~STATUS_FIELD_TRICK_ROOM;
            EXPECT_EQ(GetWhichBattlerFasterArgs(&cv, TRUE, 200, 100, 0, 0), B_POLISHED_MOVE_BALANCE && !room ? 1 : -1);
            EXPECT_EQ(GetWhichBattlerFasterArgs(&cv, FALSE, 200, 100, 0, 0), -1);
        }
    }
}

SINGLE_BATTLE_TEST("Polished order Custap waits through an opposing switch")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_CUSTAP_BERRY); HP(40); MaxHP(160); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); }
    } WHEN { TURN { SWITCH(opponent, 1); MOVE(player, MOVE_SPLASH); } }
    THEN { EXPECT_EQ(player->item, B_POLISHED_MOVE_BALANCE ? ITEM_CUSTAP_BERRY : ITEM_NONE); }
}

SINGLE_BATTLE_TEST("Polished order Quick Claw and Quick Draw do not announce activation across priority brackets")
{
    bool32 claw = FALSE;
    PARAMETRIZE { claw = FALSE; }
    PARAMETRIZE { claw = TRUE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(claw ? ABILITY_NONE : ABILITY_QUICK_DRAW); Item(claw ? ITEM_QUICK_CLAW : ITEM_NONE); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(200); }
    } WHEN { TURN { MOVE(player, MOVE_TACKLE, WITH_RNG(claw ? RNG_QUICK_CLAW : RNG_QUICK_DRAW, TRUE)); MOVE(opponent, MOVE_QUICK_ATTACK); } }
    SCENE {
        if (B_POLISHED_MOVE_BALANCE) {
            NONE_OF {
                ABILITY_POPUP(player, ABILITY_QUICK_DRAW);
                ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, player);
            }
        }
        HP_BAR(player); HP_BAR(opponent);
    }
}

SINGLE_BATTLE_TEST("Polished accuracy exact fourth and fifth stage penalties reach the live hit roll")
{
    u32 drops = 4;
    PARAMETRIZE { drops = 4; }
    PARAMETRIZE { drops = 5; }
    PASSES_RANDOMLY(B_POLISHED_MOVE_BALANCE ? (drops == 4 ? 42 : 37) : (drops == 4 ? 43 : 36), 100, RNG_ACCURACY);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN {
        for (u32 i = 0; i < drops; i++) TURN { MOVE(opponent, MOVE_SAND_ATTACK, hit: TRUE); }
        TURN { MOVE(player, MOVE_TACKLE); }
    } SCENE { ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player); HP_BAR(opponent); }
}

SINGLE_BATTLE_TEST("Polished accuracy Protect blocks No Guard Toxic and guaranteed moves")
{
    enum Move move = MOVE_TACKLE;
    PARAMETRIZE { move = MOVE_TACKLE; }
    PARAMETRIZE { move = MOVE_AERIAL_ACE; }
    PARAMETRIZE { move = MOVE_TOXIC; }
    GIVEN {
        PLAYER(SPECIES_NIDOKING) { Ability(ABILITY_NO_GUARD); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN { TURN { MOVE(opponent, MOVE_PROTECT); MOVE(player, move); } }
    SCENE { NONE_OF { HP_BAR(opponent); STATUS_ICON(opponent, badPoison: TRUE); } }
    THEN { EXPECT_EQ(opponent->hp, opponent->maxHP); EXPECT_EQ(opponent->status1, 0); }
}

SINGLE_BATTLE_TEST("Polished accuracy Blizzard in hail ignores both Umbrellas and accuracy stages")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_UTILITY_UMBRELLA); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_SNOW_CLOAK); Item(ITEM_UTILITY_UMBRELLA); Speed(200); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_SAND_ATTACK, hit: TRUE); }
        TURN { MOVE(opponent, MOVE_HAIL); MOVE(player, MOVE_BLIZZARD, WITH_RNG(RNG_ACCURACY, FALSE)); }
    } SCENE { ANIMATION(ANIM_TYPE_MOVE, MOVE_BLIZZARD, player); HP_BAR(opponent); }
}

SINGLE_BATTLE_TEST("Polished accuracy sun Thunder follows target Umbrella and then ability and item rounding")
{
    bool32 targetUmbrella = FALSE;
    PARAMETRIZE { targetUmbrella = FALSE; }
    PARAMETRIZE { targetUmbrella = TRUE; }
    u32 chance = (B_POLISHED_MOVE_BALANCE ? (targetUmbrella ? 70 : 128) : 50) * 3 / 5 * 13 / 10;
    if (!targetUmbrella) chance = chance * 9 / 10;
    PASSES_RANDOMLY(chance, 100, RNG_ACCURACY);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_COMPOUND_EYES); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(200); Item(targetUmbrella ? ITEM_UTILITY_UMBRELLA : ITEM_BRIGHT_POWDER); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_SAND_ATTACK, hit: TRUE); }
        TURN { MOVE(opponent, MOVE_SAND_ATTACK, hit: TRUE); }
        TURN { MOVE(opponent, MOVE_SUNNY_DAY); MOVE(player, MOVE_THUNDER); }
    } SCENE { ANIMATION(ANIM_TYPE_MOVE, MOVE_THUNDER, player); HP_BAR(opponent); }
}

SINGLE_BATTLE_TEST("Polished order equal priority Quick Draw and Quick Claw retain source probabilities")
{
    bool32 claw = FALSE;
    PARAMETRIZE { claw = FALSE; }
    PARAMETRIZE { claw = TRUE; }
    PASSES_RANDOMLY(claw ? 20 : 30, 100, claw ? RNG_QUICK_CLAW : RNG_QUICK_DRAW);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(claw ? ABILITY_NONE : ABILITY_QUICK_DRAW); Item(claw ? ITEM_QUICK_CLAW : ITEM_NONE); Speed(1); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); }
    } WHEN { TURN { MOVE(player, MOVE_TACKLE); MOVE(opponent, MOVE_SPLASH); } }
    SCENE { ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player); ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, opponent); }
}

SINGLE_BATTLE_TEST("Polished order equal priority Custap checks odd HP Gluttony and Unnerve on both sides")
{
    struct BattlePokemon *user = player, *target = opponent;
    bool32 gluttony = FALSE, unnerve = FALSE, above = FALSE;
    for (u32 side = 0; side < 2; side++) for (u32 g = 0; g < 2; g++) for (u32 u = 0; u < 2; u++) for (u32 high = 0; high < 2; high++)
        PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; gluttony = g; unnerve = u; above = high; }
    bool32 activates = !above && (!B_POLISHED_MOVE_BALANCE || !unnerve);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? (gluttony ? ABILITY_GLUTTONY : ABILITY_NONE) : (unnerve ? ABILITY_UNNERVE : ABILITY_NONE)); Item(user == player ? ITEM_CUSTAP_BERRY : ITEM_NONE); Speed(user == player ? 100 : 200); MaxHP(161); HP((gluttony ? 80 : 40) + above); Attack(1); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? (gluttony ? ABILITY_GLUTTONY : ABILITY_NONE) : (unnerve ? ABILITY_UNNERVE : ABILITY_NONE)); Item(user == opponent ? ITEM_CUSTAP_BERRY : ITEM_NONE); Speed(user == opponent ? 100 : 200); MaxHP(161); HP((gluttony ? 80 : 40) + above); Attack(1); }
    } WHEN { TURN { MOVE(user, MOVE_TACKLE); MOVE(target, MOVE_SPLASH); } }
    SCENE {
        if (activates) { ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, user); ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, target); }
        else { ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, target); ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, user); }
    }
    THEN { EXPECT_EQ(user->item, activates ? ITEM_NONE : ITEM_CUSTAP_BERRY); }
}

DOUBLE_BATTLE_TEST("Polished order doubles retain native Custap consumption across priorities")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_CUSTAP_BERRY); HP(40); MaxHP(160); Speed(100); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(75); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Attack(1); Speed(200); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(50); }
    } WHEN { TURN { MOVE(playerLeft, MOVE_SPLASH); MOVE(opponentLeft, MOVE_QUICK_ATTACK, target: playerLeft); } }
    THEN { EXPECT_EQ(playerLeft->item, ITEM_NONE); }
}

SINGLE_BATTLE_TEST("Polished order either active Terastal retains native quick effect speed comparison")
{
    bool32 targetTera = FALSE;
    PARAMETRIZE { targetTera = FALSE; }
    PARAMETRIZE { targetTera = TRUE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); TeraType(TYPE_PSYCHIC); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(200); TeraType(TYPE_PSYCHIC); }
    } WHEN { TURN { MOVE(targetTera ? opponent : player, MOVE_SPLASH, gimmick: GIMMICK_TERA); } }
    THEN {
        struct BattleCalcValues cv = {0};
        cv.battlerAtk = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);
        cv.battlerDef = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);
        gProtectStructs[cv.battlerAtk].quickDraw = gProtectStructs[cv.battlerDef].quickDraw = TRUE;
        gProtectStructs[cv.battlerAtk].laggingTail = TRUE;
        EXPECT_EQ(GetWhichBattlerFasterArgs(&cv, FALSE, 200, 100, 0, 0), -1);
    }
}
