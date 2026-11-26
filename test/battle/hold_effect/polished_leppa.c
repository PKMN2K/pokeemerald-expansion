#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished Leppa held recovery consumes only the selected slots packed PP Ups")
{
    struct BattlePokemon *user = player, *other = opponent;
    u32 slot = 0, count = 0;
    for (u32 side = 0; side < 2; side++)
        for (u32 index = 0; index < MAX_MON_MOVES; index++)
            for (u32 ppUps = 0; ppUps < 4; ppUps++)
                PARAMETRIZE { user = side ? opponent : player; other = side ? player : opponent; slot = index; count = ppUps; }
    u32 bonuses = (0xe4 & ~(3 << (2 * slot))) | (count << (2 * slot));
    GIVEN {
        WITH_CONFIG(B_POLISHED_MOVE_PP, B_POLISHED_MOVE_BALANCE);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(user == player ? ITEM_LEPPA_BERRY : ITEM_NONE); if (user == player) MovesWithPP({MOVE_GROWL, 5}, {MOVE_LEER, 5}, {MOVE_DEFENSE_CURL, 5}, {MOVE_SPLASH, 5}); else Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(user == opponent ? ITEM_LEPPA_BERRY : ITEM_NONE); if (user == opponent) MovesWithPP({MOVE_GROWL, 5}, {MOVE_LEER, 5}, {MOVE_DEFENSE_CURL, 5}, {MOVE_SPLASH, 5}); else Moves(MOVE_SPLASH); }
        struct Pokemon *mon = user == player ? &PLAYER_PARTY[0] : &OPPONENT_PARTY[0];
        u32 pp = 1;
        SetMonData(mon, MON_DATA_PP1 + slot, &pp);
        SetMonData(mon, MON_DATA_PP_BONUSES, &bonuses);
    } WHEN {
        TURN { MOVE(user, moveSlot: slot); MOVE(other, MOVE_SPLASH); }
    } THEN {
        u32 restored = B_POLISHED_MOVE_BALANCE && count ? CalculatePPWithBonus(user->moves[slot], bonuses, slot) : 10;
        u32 remainingBonuses = B_POLISHED_MOVE_BALANCE ? bonuses & ~(3 << (2 * slot)) : bonuses;
        struct Pokemon *mon = GetBattlerMon(user == player ? B_BATTLER_0 : B_BATTLER_1);
        EXPECT_EQ(user->item, ITEM_NONE);
        EXPECT_EQ(user->ppBonuses, remainingBonuses);
        EXPECT_EQ(GetMonData(mon, MON_DATA_PP_BONUSES), remainingBonuses);
        for (u32 i = 0; i < MAX_MON_MOVES; i++)
        {
            EXPECT_EQ(user->pp[i], i == slot ? restored : 5);
            EXPECT_EQ(GetMonData(mon, MON_DATA_PP1 + i), user->pp[i]);
        }
    }
}

SINGLE_BATTLE_TEST("Polished Leppa stolen recovery compares and wraps the raw packed PP byte")
{
    struct BattlePokemon *user = player, *holder = opponent;
    u32 kind = 0;
    for (u32 side = 0; side < 2; side++)
        for (u32 value = 0; value < 4; value++)
            PARAMETRIZE { user = side ? opponent : player; holder = side ? player : opponent; kind = value; }
    const u32 initial[] = {53, 54, 60, B_POLISHED_MOVE_BALANCE ? 61 : 64};
    const u32 sourceResult[] = {61, 0, 6, 7};
    const u32 nativeResult[] = {63, 64, 64, 64};
    u32 bonuses = 3;
    GIVEN {
        WITH_CONFIG(B_POLISHED_MOVE_PP, B_POLISHED_MOVE_BALANCE);
        PLAYER(SPECIES_WOBBUFFET) { Speed(user == player ? 100 : 50); Attack(10); Defense(255); MaxHP(1000); HP(1000); Ability(ABILITY_NONE); Item(holder == player ? ITEM_LEPPA_BERRY : ITEM_NONE); if (user == player) MovesWithPP({MOVE_SPLASH, initial[kind]}, {MOVE_BUG_BITE, 10}); else Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(user == opponent ? 100 : 50); Attack(10); Defense(255); MaxHP(1000); HP(1000); Ability(ABILITY_NONE); Item(holder == opponent ? ITEM_LEPPA_BERRY : ITEM_NONE); if (user == opponent) MovesWithPP({MOVE_SPLASH, initial[kind]}, {MOVE_BUG_BITE, 10}); else Moves(MOVE_SPLASH); }
        SetMonData(user == player ? &PLAYER_PARTY[0] : &OPPONENT_PARTY[0], MON_DATA_PP_BONUSES, &bonuses);
    } WHEN {
        TURN { MOVE(user, MOVE_BUG_BITE); MOVE(holder, MOVE_SPLASH); }
    } THEN {
        struct Pokemon *mon = GetBattlerMon(user == player ? B_BATTLER_0 : B_BATTLER_1);
        EXPECT_EQ(user->pp[0], B_POLISHED_MOVE_BALANCE ? sourceResult[kind] : nativeResult[kind]);
        EXPECT_EQ(user->pp[1], !B_POLISHED_MOVE_BALANCE && kind == 3 ? 19 : 9);
        EXPECT_EQ(user->ppBonuses, B_POLISHED_MOVE_BALANCE ? 0 : bonuses);
        EXPECT_EQ(GetMonData(mon, MON_DATA_PP1), user->pp[0]);
        EXPECT_EQ(GetMonData(mon, MON_DATA_PP2), user->pp[1]);
        EXPECT_EQ(GetMonData(mon, MON_DATA_PP_BONUSES), user->ppBonuses);
        EXPECT_EQ(holder->item, ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Leppa Cud Chew recovery preserves source packed byte wrap")
{
    struct BattlePokemon *user = player, *other = opponent;
    PARAMETRIZE { user = player; other = opponent; }
    PARAMETRIZE { user = opponent; other = player; }
    u32 bonuses = 3;
    GIVEN {
        WITH_CONFIG(B_POLISHED_MOVE_PP, B_POLISHED_MOVE_BALANCE);
        PLAYER(SPECIES_WOBBUFFET) { Speed(user == player ? 100 : 50); Ability(user == player ? ABILITY_CUD_CHEW : ABILITY_NONE); Item(user == player ? ITEM_LEPPA_BERRY : ITEM_NONE); if (user == player) MovesWithPP({MOVE_SPLASH, 54}, {MOVE_GROWL, 1}); else Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(user == opponent ? 100 : 50); Ability(user == opponent ? ABILITY_CUD_CHEW : ABILITY_NONE); Item(user == opponent ? ITEM_LEPPA_BERRY : ITEM_NONE); if (user == opponent) MovesWithPP({MOVE_SPLASH, 54}, {MOVE_GROWL, 1}); else Moves(MOVE_SPLASH); }
        SetMonData(user == player ? &PLAYER_PARTY[0] : &OPPONENT_PARTY[0], MON_DATA_PP_BONUSES, &bonuses);
    } WHEN {
        TURN { MOVE(user, MOVE_GROWL); MOVE(other, MOVE_SPLASH); }
        TURN { MOVE(user, MOVE_GROWL); MOVE(other, MOVE_SPLASH); }
        TURN { MOVE(user, MOVE_GROWL); MOVE(other, MOVE_SPLASH); }
    } THEN {
        struct Pokemon *mon = GetBattlerMon(user == player ? B_BATTLER_0 : B_BATTLER_1);
        EXPECT_EQ(user->pp[0], B_POLISHED_MOVE_BALANCE ? 0 : 64);
        EXPECT_EQ(user->pp[1], 8);
        EXPECT_EQ(user->ppBonuses, B_POLISHED_MOVE_BALANCE ? 0 : bonuses);
        EXPECT_EQ(GetMonData(mon, MON_DATA_PP1), user->pp[0]);
        EXPECT_EQ(GetMonData(mon, MON_DATA_PP_BONUSES), user->ppBonuses);
        EXPECT_EQ(user->item, ITEM_NONE);
        if (B_POLISHED_MOVE_BALANCE)
            EXPECT_EQ((u32)user->volatiles.polishedCudChewBerry, ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Leppa stolen recovery selects the first nonfull slot before a later empty slot")
{
    struct BattlePokemon *user = player, *holder = opponent;
    PARAMETRIZE { user = player; holder = opponent; }
    PARAMETRIZE { user = opponent; holder = player; }
    GIVEN {
        WITH_CONFIG(B_POLISHED_MOVE_PP, B_POLISHED_MOVE_BALANCE);
        PLAYER(SPECIES_WOBBUFFET) { Speed(user == player ? 100 : 50); Attack(10); Defense(255); MaxHP(1000); HP(1000); Ability(ABILITY_NONE); Item(holder == player ? ITEM_LEPPA_BERRY : ITEM_NONE); if (user == player) MovesWithPP({MOVE_SCRATCH, 6}, {MOVE_GROWL, 0}, {MOVE_BUG_BITE, 10}, {MOVE_SPLASH, 40}); else Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(user == opponent ? 100 : 50); Attack(10); Defense(255); MaxHP(1000); HP(1000); Ability(ABILITY_NONE); Item(holder == opponent ? ITEM_LEPPA_BERRY : ITEM_NONE); if (user == opponent) MovesWithPP({MOVE_SCRATCH, 6}, {MOVE_GROWL, 0}, {MOVE_BUG_BITE, 10}, {MOVE_SPLASH, 40}); else Moves(MOVE_SPLASH); }
    } WHEN {
        TURN { MOVE(user, MOVE_BUG_BITE); MOVE(holder, MOVE_SPLASH); }
    } THEN {
        EXPECT_EQ(user->pp[0], B_POLISHED_MOVE_BALANCE ? 16 : 6);
        EXPECT_EQ(user->pp[1], B_POLISHED_MOVE_BALANCE ? 0 : 10);
        EXPECT_EQ(user->pp[2], 9);
        EXPECT_EQ(holder->item, ITEM_NONE);
        struct Pokemon *mon = GetBattlerMon(user == player ? B_BATTLER_0 : B_BATTLER_1);
        EXPECT_EQ(GetMonData(mon, MON_DATA_PP1), user->pp[0]);
        EXPECT_EQ(GetMonData(mon, MON_DATA_PP2), user->pp[1]);
    }
}

SINGLE_BATTLE_TEST("Polished Leppa Cud Chew reconsumption also selects the first nonfull slot")
{
    struct BattlePokemon *user = player, *other = opponent;
    PARAMETRIZE { user = player; other = opponent; }
    PARAMETRIZE { user = opponent; other = player; }
    GIVEN {
        WITH_CONFIG(B_POLISHED_MOVE_PP, B_POLISHED_MOVE_BALANCE);
        PLAYER(SPECIES_WOBBUFFET) { Speed(user == player ? 100 : 50); Ability(user == player ? ABILITY_CUD_CHEW : ABILITY_NONE); Item(user == player ? ITEM_LEPPA_BERRY : ITEM_NONE); if (user == player) MovesWithPP({MOVE_SCRATCH, 6}, {MOVE_GROWL, 1}, {MOVE_SPLASH, 1}); else Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(user == opponent ? 100 : 50); Ability(user == opponent ? ABILITY_CUD_CHEW : ABILITY_NONE); Item(user == opponent ? ITEM_LEPPA_BERRY : ITEM_NONE); if (user == opponent) MovesWithPP({MOVE_SCRATCH, 6}, {MOVE_GROWL, 1}, {MOVE_SPLASH, 1}); else Moves(MOVE_SPLASH); }
    } WHEN {
        TURN { MOVE(user, MOVE_GROWL); MOVE(other, MOVE_SPLASH); }
        TURN { MOVE(user, MOVE_SPLASH); MOVE(other, MOVE_SPLASH); }
        TURN { MOVE(user, MOVE_GROWL); MOVE(other, MOVE_SPLASH); }
    } THEN {
        EXPECT_EQ(user->pp[0], B_POLISHED_MOVE_BALANCE ? 16 : 6);
        EXPECT_EQ(user->pp[1], 9);
        EXPECT_EQ(user->pp[2], B_POLISHED_MOVE_BALANCE ? 0 : 10);
        EXPECT_EQ(user->item, ITEM_NONE);
        if (B_POLISHED_MOVE_BALANCE)
            EXPECT_EQ((u32)user->volatiles.polishedCudChewBerry, ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Leppa uses transformed PP without restoring the original party move")
{
    struct BattlePokemon *user = player, *other = opponent;
    enum Move copied = MOVE_SCRATCH;
    u32 repeats = 0;
    u32 bonuses = 0xff;
    for (u32 side = 0; side < 2; side++)
        for (u32 move = 0; move < 2; move++)
            for (u32 finish = 0; finish < 2; finish++)
                PARAMETRIZE { user = side ? opponent : player; other = side ? player : opponent; copied = move ? MOVE_RECOVER : MOVE_SCRATCH; repeats = finish ? 5 : 0; }
    GIVEN {
        WITH_CONFIG(B_POLISHED_MOVE_PP, B_POLISHED_MOVE_BALANCE);
        PLAYER(SPECIES_WOBBUFFET) { Speed(user == player ? 100 : 50); Attack(10); Defense(255); MaxHP(1000); HP(1000); Ability(ABILITY_NONE); Item(user == player ? ITEM_LEPPA_BERRY : ITEM_NONE); if (user == player) MovesWithPP({MOVE_TRANSFORM, 1}, {MOVE_SPLASH, 40}); else Moves(copied, MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(user == opponent ? 100 : 50); Attack(10); Defense(255); MaxHP(1000); HP(1000); Ability(ABILITY_NONE); Item(user == opponent ? ITEM_LEPPA_BERRY : ITEM_NONE); if (user == opponent) MovesWithPP({MOVE_TRANSFORM, 1}, {MOVE_SPLASH, 40}); else Moves(copied, MOVE_SPLASH); }
        SetMonData(user == player ? &PLAYER_PARTY[0] : &OPPONENT_PARTY[0], MON_DATA_PP_BONUSES, &bonuses);
    } WHEN {
        TURN { MOVE(user, MOVE_TRANSFORM); MOVE(other, MOVE_SPLASH); }
        for (u32 i = 0; i < repeats; i++)
            TURN { MOVE(user, moveSlot: 0); MOVE(other, MOVE_SPLASH); }
    } THEN {
        EXPECT(user->volatiles.transformed);
        EXPECT_EQ(user->moves[0], copied);
        u32 restored = min(10, GetMovePP(copied));
        EXPECT_EQ(user->pp[0], repeats ? (B_POLISHED_MOVE_BALANCE ? restored : 0) : 5);
        EXPECT_EQ(user->item, B_POLISHED_MOVE_BALANCE && !repeats ? ITEM_LEPPA_BERRY : ITEM_NONE);
        struct Pokemon *mon = GetBattlerMon(user == player ? B_BATTLER_0 : B_BATTLER_1);
        EXPECT_EQ(GetMonData(mon, MON_DATA_MOVE1), MOVE_TRANSFORM);
        EXPECT_EQ(GetMonData(mon, MON_DATA_PP1), B_POLISHED_MOVE_BALANCE ? 0 : 10);
        EXPECT_EQ(GetMonData(mon, MON_DATA_PP_BONUSES), bonuses);
        EXPECT_EQ(user->ppBonuses, B_POLISHED_MOVE_BALANCE ? 0 : bonuses);
    }
}

SINGLE_BATTLE_TEST("Polished Leppa restores the exhausted caller slot after Pressure without spending the called move PP")
{
    struct BattlePokemon *user = player, *other = opponent;
    enum Move caller = MOVE_METRONOME;
    for (u32 side = 0; side < 2; side++)
        for (u32 asleep = 0; asleep < 2; asleep++)
            PARAMETRIZE { user = side ? opponent : player; other = side ? player : opponent; caller = asleep ? MOVE_SLEEP_TALK : MOVE_METRONOME; }
    enum Move called = caller == MOVE_METRONOME ? MOVE_SCRATCH : MOVE_TACKLE;
    GIVEN {
        WITH_CONFIG(B_POLISHED_MOVE_PP, B_POLISHED_MOVE_BALANCE);
        PLAYER(SPECIES_WOBBUFFET) { Speed(user == player ? 100 : 50); Attack(10); Defense(255); MaxHP(1000); HP(1000); Ability(user == player ? ABILITY_NONE : ABILITY_PRESSURE); Item(user == player ? ITEM_LEPPA_BERRY : ITEM_NONE); if (user == player) { MovesWithPP({MOVE_TACKLE, 17}, {caller, 1}); if (caller == MOVE_SLEEP_TALK) Status1(STATUS1_SLEEP_TURN(4)); } else Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(user == opponent ? 100 : 50); Attack(10); Defense(255); MaxHP(1000); HP(1000); Ability(user == opponent ? ABILITY_NONE : ABILITY_PRESSURE); Item(user == opponent ? ITEM_LEPPA_BERRY : ITEM_NONE); if (user == opponent) { MovesWithPP({MOVE_TACKLE, 17}, {caller, 1}); if (caller == MOVE_SLEEP_TALK) Status1(STATUS1_SLEEP_TURN(4)); } else Moves(MOVE_SPLASH); }
    } WHEN {
        TURN { MOVE(user, caller, WITH_RNG(RNG_METRONOME, called)); MOVE(other, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, called, user);
        if (B_POLISHED_MOVE_BALANCE) {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, other);
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, user);
        } else {
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, user);
            ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, other);
        }
    } THEN {
        EXPECT_EQ(user->pp[0], 17);
        EXPECT_EQ(user->pp[1], 10);
        EXPECT_EQ(user->item, ITEM_NONE);
        if (B_POLISHED_MOVE_BALANCE)
            EXPECT_EQ(gLastMoves[user == player ? B_BATTLER_0 : B_BATTLER_1], MOVE_NONE);
    }
}
