#include "global.h"
#include "test/battle.h"
#include "battle_script_commands.h"
#include "battle_util.h"

SINGLE_BATTLE_TEST("Polished forgetting a slot clears only its active player locks, including bench learning")
{
    u32 slot = 0, learner = 0;
    for (u32 s = 0; s < 4; s++)
        for (u32 l = 0; l < 2; l++) PARAMETRIZE { slot = s; learner = l; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_SPLASH, MOVE_GROWL, MOVE_TACKLE, MOVE_PROTECT); }
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN { TURN { MOVE(player, MOVE_SPLASH); } }
    THEN {
        // Invoke the production forgetting boundary before replacement, as
        // Cmd_yesnoboxlearnmove does after the player confirms a slot.
        gBattleStruct->expGetterMonId = learner;
        player->volatiles.disabledMove = MOVE_GROWL;
        player->volatiles.disableTimer = 3;
        player->volatiles.encoredMove = MOVE_TACKLE;
        player->volatiles.encoredMovePos = 2;
        player->volatiles.encoreTimer = 2;
        gBattleStruct->polishedChoiceMove[B_BATTLER_0] = MOVE_TACKLE;
        PolishedForgetMoveLocks(slot);
        EXPECT_EQ((u32)player->volatiles.disabledMove, B_POLISHED_MOVE_BALANCE && slot == 1 ? MOVE_NONE : MOVE_GROWL);
        EXPECT_EQ((u32)player->volatiles.disableTimer, B_POLISHED_MOVE_BALANCE && slot == 1 ? 0 : 3);
        EXPECT_EQ((u32)player->volatiles.encoredMove, B_POLISHED_MOVE_BALANCE && slot == 2 ? MOVE_NONE : MOVE_TACKLE);
        EXPECT_EQ((u32)player->volatiles.encoreTimer, B_POLISHED_MOVE_BALANCE && slot == 2 ? 0 : 2);
        EXPECT_EQ(gBattleStruct->polishedChoiceMove[B_BATTLER_0], B_POLISHED_MOVE_BALANCE && slot == 2 ? MOVE_NONE : MOVE_TACKLE);
    }
}

SINGLE_BATTLE_TEST("Polished rampage called by Sleep Talk never locks the sleeping user")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_SLEEP_TALK, MOVE_THRASH); Status1(STATUS1_SLEEP_TURN(4)); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1000); MaxHP(1000); }
    } WHEN { TURN { MOVE(player, MOVE_SLEEP_TALK); } }
    SCENE { ANIMATION(ANIM_TYPE_MOVE, MOVE_THRASH, player); }
    THEN {
        if (B_POLISHED_MOVE_BALANCE) {
            EXPECT_EQ((u32)player->volatiles.rampageTurns, 0);
            EXPECT_EQ((u32)player->volatiles.multipleTurns, FALSE);
        }
    }
}

SINGLE_BATTLE_TEST("Polished Rollout called by Sleep Talk never advances the sleeping user lock")
{
    bool32 enemy = FALSE;
    PARAMETRIZE { enemy = FALSE; }
    PARAMETRIZE { enemy = TRUE; }
    struct BattlePokemon *user = enemy ? opponent : player;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(ABILITY_NONE); HP(1000); MaxHP(1000);
            if (!enemy) { Moves(MOVE_SLEEP_TALK, MOVE_ROLLOUT); Status1(STATUS1_SLEEP_TURN(4)); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(ABILITY_NONE); HP(1000); MaxHP(1000);
            if (enemy) { Moves(MOVE_SLEEP_TALK, MOVE_ROLLOUT); Status1(STATUS1_SLEEP_TURN(4)); }
        }
    } WHEN { TURN { MOVE(user, MOVE_SLEEP_TALK); } }
    SCENE { ANIMATION(ANIM_TYPE_MOVE, MOVE_ROLLOUT, user); }
    THEN {
        EXPECT_EQ((u32)user->volatiles.rolloutTimer, 0);
        EXPECT_EQ((u32)user->volatiles.multipleTurns, FALSE);
    }
}

SINGLE_BATTLE_TEST("Polished rampage clears its lock when the middle turn is protected")
{
    GIVEN {
        WITH_CONFIG(B_RAMPAGE_CONFUSION, GEN_5);
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_THRASH, MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1000); MaxHP(1000); }
    } WHEN {
        TURN { MOVE(player, MOVE_THRASH, WITH_RNG(RNG_RAMPAGE_TURNS, 3)); }
        TURN { SKIP_TURN(player); MOVE(opponent, MOVE_PROTECT); }
    } THEN {
        if (B_POLISHED_MOVE_BALANCE) {
            EXPECT_EQ((u32)player->volatiles.rampageTurns, 0);
            EXPECT_EQ((u32)player->volatiles.multipleTurns, FALSE);
            EXPECT_EQ(gLockedMoves[B_BATTLER_0], MOVE_NONE);
            EXPECT_EQ((u32)player->volatiles.confusionTimer, 0);
        }
    }
}

SINGLE_BATTLE_TEST("Polished rampage confuses on final-turn flinch despite an older native preset")
{
    enum Ability ability = ABILITY_NONE;
    PARAMETRIZE { ability = ABILITY_NONE; }
    PARAMETRIZE { ability = ABILITY_OWN_TEMPO; }
    GIVEN {
        WITH_CONFIG(B_RAMPAGE_CONFUSION, GEN_3);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ability); Speed(50); Moves(MOVE_THRASH, MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(100); HP(1000); MaxHP(1000); }
    } WHEN {
        TURN { MOVE(player, MOVE_THRASH, WITH_RNG(RNG_RAMPAGE_TURNS, 2)); }
        TURN { MOVE(opponent, MOVE_BITE, secondaryEffect: TRUE); SKIP_TURN(player); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_THRASH, player);
        MESSAGE("Wobbuffet flinched and couldn't move!");
        if (B_POLISHED_MOVE_BALANCE && ability != ABILITY_OWN_TEMPO)
            ANIMATION(ANIM_TYPE_STATUS, B_ANIM_STATUS_CONFUSION, player);
        else NOT ANIMATION(ANIM_TYPE_STATUS, B_ANIM_STATUS_CONFUSION, player);
    } THEN {
        EXPECT_EQ((u32)player->volatiles.multipleTurns, FALSE);
    }
}

SINGLE_BATTLE_TEST("Polished Explosion releases Unnerve berries before attack damage")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_UNNERVE); Speed(100); Attack(10); Moves(MOVE_EXPLOSION); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); Defense(1000); HP(20); MaxHP(160); Item(ITEM_SITRUS_BERRY); }
    } WHEN { TURN { MOVE(player, MOVE_EXPLOSION); } }
    SCENE {
        if (B_POLISHED_MOVE_BALANCE) {
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, opponent);
            HP_BAR(opponent, damage: -40);
            ANIMATION(ANIM_TYPE_MOVE, MOVE_EXPLOSION, player);
            HP_BAR(opponent);
        }
    } THEN {
        EXPECT_EQ(opponent->item, ITEM_NONE);
        EXPECT_EQ(player->hp, 0);
    }
}

SINGLE_BATTLE_TEST("Polished Explosion clears status before fainting and respects Damp prevention")
{
    enum Ability damp = ABILITY_NONE;
    PARAMETRIZE { damp = ABILITY_NONE; }
    PARAMETRIZE { damp = ABILITY_DAMP; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); Status1(STATUS1_POISON); Moves(MOVE_EXPLOSION); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(damp); Speed(50); HP(1000); MaxHP(1000); }
    } WHEN { TURN { MOVE(player, MOVE_EXPLOSION); } }
    THEN {
        if (B_POLISHED_MOVE_BALANCE)
            EXPECT_EQ(GetMonData(GetBattlerMon(B_BATTLER_0), MON_DATA_STATUS), damp == ABILITY_DAMP ? STATUS1_POISON : 0);
        EXPECT_EQ(player->hp == 0, damp != ABILITY_DAMP);
    }
}

SINGLE_BATTLE_TEST("Polished Rest uses the opponent Umbrella for Leaf Guard independent of generation")
{
    enum Item ownItem = ITEM_NONE, otherItem = ITEM_NONE;
    u32 gen = GEN_4;
    for (u32 own = 0; own < 2; own++)
        for (u32 other = 0; other < 2; other++)
            for (u32 g = GEN_4; g <= GEN_5; g++)
                PARAMETRIZE { ownItem = own ? ITEM_UTILITY_UMBRELLA : ITEM_NONE; otherItem = other ? ITEM_UTILITY_UMBRELLA : ITEM_NONE; gen = g; }
    bool32 blocked = B_POLISHED_MOVE_BALANCE ? otherItem == ITEM_NONE : gen >= GEN_5 && ownItem == ITEM_NONE;
    GIVEN {
        WITH_CONFIG(B_LEAF_GUARD_PREVENTS_REST, gen);
        PLAYER(SPECIES_LEAFEON) { Ability(ABILITY_LEAF_GUARD); Item(ownItem); HP(100); MaxHP(200); Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Item(otherItem); Speed(100); }
    } WHEN { TURN { MOVE(opponent, MOVE_SUNNY_DAY); MOVE(player, MOVE_REST); } }
    SCENE {
        if (blocked) NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_REST, player);
        else { ANIMATION(ANIM_TYPE_MOVE, MOVE_REST, player); HP_BAR(player, damage: -100); }
    } THEN {
        EXPECT_EQ(player->hp, blocked ? 100 : 200);
        EXPECT_EQ(player->status1 & STATUS1_SLEEP, blocked ? 0 : 3);
    }
}

SINGLE_BATTLE_TEST("Polished Rest rechecks weather and ability suppression for Leaf Guard")
{
    enum Ability ability = ABILITY_NONE;
    enum Move weather = MOVE_RAIN_DANCE;
    PARAMETRIZE { ability = ABILITY_NONE; weather = MOVE_RAIN_DANCE; }
    PARAMETRIZE { ability = ABILITY_CLOUD_NINE; weather = MOVE_SUNNY_DAY; }
    PARAMETRIZE { ability = ABILITY_NEUTRALIZING_GAS; weather = MOVE_SUNNY_DAY; }
    GIVEN {
        PLAYER(SPECIES_LEAFEON) { Ability(ABILITY_LEAF_GUARD); HP(100); MaxHP(200); Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ability); Speed(100); }
    } WHEN { TURN { MOVE(opponent, weather); MOVE(player, MOVE_REST); } }
    SCENE { ANIMATION(ANIM_TYPE_MOVE, MOVE_REST, player); HP_BAR(player, damage: -100); }
    THEN { EXPECT_EQ(player->hp, 200); EXPECT_EQ(player->status1 & STATUS1_SLEEP, 3); }
}

SINGLE_BATTLE_TEST("Polished Rest replaces poison and heals before its immediate berry cure")
{
    enum Item berry = ITEM_NONE;
    PARAMETRIZE { berry = ITEM_NONE; }
    PARAMETRIZE { berry = ITEM_CHESTO_BERRY; }
    PARAMETRIZE { berry = ITEM_LUM_BERRY; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HP(100); MaxHP(200); Item(berry); Status1(berry == ITEM_NONE ? STATUS1_POISON : STATUS1_NONE); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN { TURN { MOVE(player, MOVE_REST); } }
    SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_REST, player);
        HP_BAR(player, damage: -100);
        if (berry != ITEM_NONE) ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, player);
    } THEN {
        EXPECT_EQ(player->hp, 200);
        EXPECT_EQ(player->status1, berry == ITEM_NONE ? 3 : 0);
        EXPECT_EQ(player->item, ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished retaliation uses only the last incoming strike")
{
    enum Move attack = MOVE_DOUBLE_KICK, response = MOVE_COUNTER;
    enum Ability ability = ABILITY_NONE;
    PARAMETRIZE { attack = MOVE_DOUBLE_KICK; response = MOVE_COUNTER; ability = ABILITY_NONE; }
    PARAMETRIZE { attack = MOVE_POUND; response = MOVE_COUNTER; ability = ABILITY_PARENTAL_BOND; }
    PARAMETRIZE { attack = MOVE_WATER_GUN; response = MOVE_MIRROR_COAT; ability = ABILITY_PARENTAL_BOND; }
    s16 first, last, reflected;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); HP(1000); MaxHP(1000); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ability); HP(1000); MaxHP(1000); }
    } WHEN { TURN { MOVE(opponent, attack); MOVE(player, response); } }
    SCENE {
        ANIMATION(ANIM_TYPE_MOVE, attack, opponent);
        HP_BAR(player, captureDamage: &first);
        HP_BAR(player, captureDamage: &last);
        ANIMATION(ANIM_TYPE_MOVE, response, player);
        HP_BAR(opponent, captureDamage: &reflected);
    } THEN { EXPECT_EQ(reflected, last * 2); EXPECT(first > 0); }
}

SINGLE_BATTLE_TEST("Polished retaliation Parental Bond repeats the same doubled amount")
{
    enum Move attack = MOVE_POUND, response = MOVE_COUNTER;
    PARAMETRIZE { attack = MOVE_POUND; response = MOVE_COUNTER; }
    PARAMETRIZE { attack = MOVE_WATER_GUN; response = MOVE_MIRROR_COAT; }
    s16 incoming, first, second;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_PARENTAL_BOND); HP(1000); MaxHP(1000); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); HP(1000); MaxHP(1000); }
    } WHEN { TURN { MOVE(opponent, attack); MOVE(player, response); } }
    SCENE {
        ANIMATION(ANIM_TYPE_MOVE, attack, opponent);
        HP_BAR(player, captureDamage: &incoming);
        ANIMATION(ANIM_TYPE_MOVE, response, player);
        HP_BAR(opponent, captureDamage: &first);
        HP_BAR(opponent, captureDamage: &second);
    } THEN { EXPECT_EQ(first, incoming * 2); EXPECT_EQ(second, first); }
}

SINGLE_BATTLE_TEST("Polished charging moves pay PP once including Pressure with continuation or Power Herb")
{
    enum Move move = MOVE_FLY;
    enum Item herb = ITEM_NONE;
    enum Ability opposingAbility = ABILITY_NONE;
    const enum Move moves[] = {MOVE_FLY, MOVE_DIG, MOVE_SOLAR_BEAM};
    for (u32 m = 0; m < ARRAY_COUNT(moves); m++)
        for (u32 h = 0; h < 2; h++)
            for (u32 p = 0; p < 2; p++)
                PARAMETRIZE { move = moves[m]; herb = h ? ITEM_POWER_HERB : ITEM_NONE; opposingAbility = p ? ABILITY_PRESSURE : ABILITY_NONE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Item(herb); MovesWithPP({move, 5}, {MOVE_SPLASH, 5}); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1000); MaxHP(1000); Ability(opposingAbility); }
    } WHEN {
        TURN { MOVE(player, move); }
        if (herb == ITEM_NONE) TURN { SKIP_TURN(player); }
    } SCENE {
        if (herb != ITEM_NONE) ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, player);
        ANIMATION(ANIM_TYPE_MOVE, move, player);
        HP_BAR(opponent);
    } THEN {
        EXPECT_EQ(player->pp[0], opposingAbility == ABILITY_PRESSURE ? 3 : 4);
        EXPECT_EQ(player->item, ITEM_NONE);
        EXPECT_EQ((u32)player->volatiles.multipleTurns, FALSE);
        EXPECT_EQ((u32)player->volatiles.semiInvulnerable, STATE_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Solar Beam sunlight skips charging without consuming Power Herb")
{
    enum Item item = ITEM_NONE;
    PARAMETRIZE { item = ITEM_NONE; }
    PARAMETRIZE { item = ITEM_POWER_HERB; }
    PARAMETRIZE { item = ITEM_UTILITY_UMBRELLA; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Item(item); Speed(50); MovesWithPP({MOVE_SOLAR_BEAM, 5}); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(100); HP(1000); MaxHP(1000); Item(ITEM_UTILITY_UMBRELLA); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_SUNNY_DAY); MOVE(player, MOVE_SOLAR_BEAM); }
        if (item == ITEM_UTILITY_UMBRELLA) TURN { SKIP_TURN(player); }
    } SCENE {
        if (item == ITEM_UTILITY_UMBRELLA) {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_SOLAR_BEAM, player);
            ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, opponent);
        }
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SOLAR_BEAM, player);
        HP_BAR(opponent);
        NOT ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, player);
    } THEN { EXPECT_EQ(player->pp[0], 4); EXPECT_EQ(player->item, item); }
}

SINGLE_BATTLE_TEST("Polished charging interruption releases Fly Dig and Solar Beam without a second PP cost")
{
    enum Move move = MOVE_FLY;
    PARAMETRIZE { move = MOVE_FLY; }
    PARAMETRIZE { move = MOVE_DIG; }
    PARAMETRIZE { move = MOVE_SOLAR_BEAM; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(50); HP(1000); MaxHP(1000); MovesWithPP({move, 5}, {MOVE_SPLASH, 5}); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(100); Ability(ABILITY_NO_GUARD); }
    } WHEN {
        TURN { MOVE(player, move); }
        TURN { MOVE(opponent, MOVE_BITE, secondaryEffect: TRUE); SKIP_TURN(player); }
        TURN { MOVE(player, MOVE_SPLASH); }
    } SCENE {
        MESSAGE("Wobbuffet flinched and couldn't move!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, player);
    } THEN {
        EXPECT_EQ(player->pp[0], 4);
        EXPECT_EQ((u32)player->volatiles.multipleTurns, FALSE);
        EXPECT_EQ((u32)player->volatiles.semiInvulnerable, STATE_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished charging called by Metronome preserves caller PP and cleared history against Pressure")
{
    enum Move move = MOVE_FLY;
    enum Item item = ITEM_NONE;
    bool32 enemy = FALSE;
    const enum Move moves[] = {MOVE_FLY, MOVE_DIG, MOVE_SOLAR_BEAM};
    for (u32 m = 0; m < ARRAY_COUNT(moves); m++)
        for (u32 h = 0; h < 2; h++)
            for (u32 side = 0; side < 2; side++)
                PARAMETRIZE { move = moves[m]; item = h ? ITEM_POWER_HERB : ITEM_NONE; enemy = side; }
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(enemy ? ABILITY_PRESSURE : ABILITY_NONE); MaxHP(1000); HP(1000);
            if (!enemy) { Item(item); MovesWithPP({MOVE_METRONOME, 5}, {MOVE_SPLASH, 5}); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(enemy ? ABILITY_NONE : ABILITY_PRESSURE); MaxHP(1000); HP(1000);
            if (enemy) { Item(item); MovesWithPP({MOVE_METRONOME, 5}, {MOVE_SPLASH, 5}); }
        }
    } WHEN {
        // These three source IDs match the stable native identifiers.
        TURN { MOVE(user, MOVE_METRONOME, WITH_RNG(RNG_METRONOME, move)); }
        if (item == ITEM_NONE) TURN { SKIP_TURN(user); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_METRONOME, user);
        if (item != ITEM_NONE) ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, user);
        ANIMATION(ANIM_TYPE_MOVE, move, user);
        HP_BAR(target);
    } THEN {
        EXPECT_EQ(user->pp[0], B_POLISHED_MOVE_BALANCE ? 4 : 3);
        EXPECT_EQ(user->pp[1], 5);
        EXPECT_EQ(user->item, ITEM_NONE);
        EXPECT_EQ((u32)user->volatiles.multipleTurns, FALSE);
        EXPECT_EQ((u32)user->volatiles.semiInvulnerable, STATE_NONE);
        if (B_POLISHED_MOVE_BALANCE)
            EXPECT_EQ(gLastMoves[enemy ? B_BATTLER_1 : B_BATTLER_0], MOVE_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Rest rejection preserves HP and status for both battlers")
{
    u32 reason = 0;
    bool32 enemy = FALSE;
    for (u32 r = 0; r < 4; r++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { reason = r; enemy = side; }
    struct BattlePokemon *user = enemy ? opponent : player;
    enum Ability ability = reason == 1 ? ABILITY_INSOMNIA : reason == 2 ? ABILITY_VITAL_SPIRIT : ABILITY_NONE;
    u32 hp = reason == 0 ? 200 : 100;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            if (!enemy) {
                MaxHP(200); HP(hp); Ability(ability); MovesWithPP({MOVE_REST, 5}, {MOVE_SLEEP_TALK, 5});
                if (reason == 3) Status1(STATUS1_SLEEP_TURN(3));
            }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            if (enemy) {
                MaxHP(200); HP(hp); Ability(ability); MovesWithPP({MOVE_REST, 5}, {MOVE_SLEEP_TALK, 5});
                if (reason == 3) Status1(STATUS1_SLEEP_TURN(3));
            }
        }
    } WHEN { TURN { MOVE(user, reason == 3 ? MOVE_SLEEP_TALK : MOVE_REST); } }
    SCENE { NONE_OF { ANIMATION(ANIM_TYPE_MOVE, MOVE_REST, user); HP_BAR(user); } }
    THEN {
        EXPECT_EQ(user->hp, hp);
        EXPECT_EQ(user->status1, reason == 3 ? STATUS1_SLEEP_TURN(2) : STATUS1_NONE);
        EXPECT_EQ(user->pp[0], reason == 3 ? 5 : 4);
        EXPECT_EQ(user->pp[1], reason == 3 ? 4 : 5);
    }
}

SINGLE_BATTLE_TEST("Polished called charging history ends on the next selection or replacement")
{
    u32 ending = 0;
    PARAMETRIZE { ending = 0; }
    PARAMETRIZE { ending = 1; }
    PARAMETRIZE { ending = 2; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); HP(1000); MaxHP(1000); MovesWithPP({MOVE_METRONOME, 5}, {MOVE_SPLASH, 5}); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(75); Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NO_GUARD); Speed(100); HP(1000); MaxHP(1000); }
    } WHEN {
        TURN { MOVE(player, MOVE_METRONOME, WITH_RNG(RNG_METRONOME, MOVE_FLY)); }
        if (ending == 1)
            TURN { MOVE(opponent, MOVE_BITE, secondaryEffect: TRUE); SKIP_TURN(player); }
        else TURN { SKIP_TURN(player); }
        if (ending == 2) TURN { SWITCH(player, 1); }
        else TURN { MOVE(player, MOVE_SPLASH); }
    } THEN {
        EXPECT_EQ(gBattleStruct->polishedCalledMoveBattlers & (1u << B_BATTLER_0), 0);
        EXPECT_EQ(gLastMoves[B_BATTLER_0], ending == 2 ? MOVE_NONE : MOVE_SPLASH);
        if (ending != 2) {
            EXPECT_EQ(player->pp[0], 4);
            EXPECT_EQ(player->pp[1], 4);
        }
    }
}

SINGLE_BATTLE_TEST("Polished called attacks spend only caller PP against Pressure")
{
    enum Move caller = MOVE_METRONOME;
    bool32 enemy = FALSE;
    for (u32 kind = 0; kind < 2; kind++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { caller = kind ? MOVE_SLEEP_TALK : MOVE_METRONOME; enemy = side; }
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(enemy ? ABILITY_PRESSURE : ABILITY_NONE); HP(1000); MaxHP(1000);
            if (!enemy) {
                MovesWithPP({caller, 5}, {caller == MOVE_SLEEP_TALK ? MOVE_TACKLE : MOVE_SPLASH, 0});
                if (caller == MOVE_SLEEP_TALK) Status1(STATUS1_SLEEP_TURN(3));
            }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(enemy ? ABILITY_NONE : ABILITY_PRESSURE); HP(1000); MaxHP(1000);
            if (enemy) {
                MovesWithPP({caller, 5}, {caller == MOVE_SLEEP_TALK ? MOVE_TACKLE : MOVE_SPLASH, 0});
                if (caller == MOVE_SLEEP_TALK) Status1(STATUS1_SLEEP_TURN(3));
            }
        }
    } WHEN { TURN { MOVE(user, caller, WITH_RNG(RNG_METRONOME, MOVE_TACKLE)); } }
    SCENE { ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, user); HP_BAR(target); }
    THEN {
        EXPECT_EQ(user->pp[0], B_POLISHED_MOVE_BALANCE ? 4 : 3);
        EXPECT_EQ(user->pp[1], 0);
        if (B_POLISHED_MOVE_BALANCE)
            EXPECT_EQ(gLastMoves[enemy ? B_BATTLER_1 : B_BATTLER_0], MOVE_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished retaliation saturates doubled source damage at the 16-bit boundary")
{
    enum Move move = MOVE_COUNTER;
    bool32 enemy = FALSE;
    for (u32 kind = 0; kind < 2; kind++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = kind ? MOVE_MIRROR_COAT : MOVE_COUNTER; enemy = side; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN { TURN {} }
    THEN {
        // Exercise the production damage calculator before the HP controller
        // clamps the visible loss to the target's remaining HP. The fixture
        // includes the last nonoverflowing value and both sides of overflow.
        const u32 incoming[] = {32767, 32768, 65534};
        struct DamageContext ctx = {0};
        ctx.battlerAtk = enemy ? B_BATTLER_1 : B_BATTLER_0;
        ctx.battlerDef = enemy ? B_BATTLER_0 : B_BATTLER_1;
        ctx.move = move;
        ctx.moveType = GetBattleMoveType(move);
        for (u32 i = 0; i < ARRAY_COUNT(incoming); i++) {
            // Native counters store damage+1 so zero can mean "no hit".
            gProtectStructs[ctx.battlerAtk].physicalDmg = incoming[i] + 1;
            gProtectStructs[ctx.battlerAtk].specialDmg = incoming[i] + 1;
            EXPECT_EQ(CalculateMoveDamage(&ctx), B_POLISHED_MOVE_BALANCE ? min(0xFFFF, incoming[i] * 2) : incoming[i] * 2);
        }
    }
}

SINGLE_BATTLE_TEST("Polished charging starts against a fainted opponent and resumes only after replacement")
{
    enum Move move = MOVE_FLY;
    enum Item item = ITEM_NONE;
    const enum Move moves[] = {MOVE_FLY, MOVE_DIG, MOVE_SOLAR_BEAM};
    for (u32 m = 0; m < ARRAY_COUNT(moves); m++)
        for (u32 h = 0; h < 2; h++)
            PARAMETRIZE { move = moves[m]; item = h ? ITEM_POWER_HERB : ITEM_NONE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); Defense(1000); HP(1000); MaxHP(1000); Item(item); MovesWithPP({move, 5}, {MOVE_SPLASH, 5}); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_PRESSURE); Speed(100); Attack(1); Moves(MOVE_EXPLOSION); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(75); HP(1000); MaxHP(1000); Moves(MOVE_SPLASH); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_EXPLOSION); MOVE(player, move); SEND_OUT(opponent, 1); }
        if (item == ITEM_NONE) TURN { SKIP_TURN(player); MOVE(opponent, MOVE_SPLASH); }
        else TURN { MOVE(player, MOVE_SPLASH); MOVE(opponent, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_EXPLOSION, opponent);
        if (B_POLISHED_MOVE_BALANCE) {
            ANIMATION(ANIM_TYPE_MOVE, move, player);
            if (item != ITEM_NONE) ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, player);
            MESSAGE("2 sent out Wynaut!");
        }
        if (item == ITEM_NONE) { ANIMATION(ANIM_TYPE_MOVE, move, player); HP_BAR(opponent); }
    } THEN {
        EXPECT_EQ((u32)player->volatiles.multipleTurns, FALSE);
        EXPECT_EQ((u32)player->volatiles.semiInvulnerable, STATE_NONE);
        EXPECT_EQ(player->pp[0], 4);
        EXPECT_EQ(player->item, ITEM_NONE);
        if (item == ITEM_NONE) EXPECT_LT(opponent->hp, 1000);
        else EXPECT_EQ(opponent->hp, 1000);
    }
}
