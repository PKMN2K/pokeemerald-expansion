#include "global.h"
#include "test/battle.h"
#include "item_use.h"
#include "party_menu.h"

SINGLE_BATTLE_TEST("Polished medicine closure all-status cures confusion while preserving source attraction")
{
    static const enum Item items[] = {
        ITEM_FULL_HEAL, ITEM_FULL_RESTORE, ITEM_HEAL_POWDER, ITEM_LUM_BERRY,
        ITEM_RAGE_CANDY_BAR, ITEM_PEWTER_CRUNCHIES, ITEM_LAVA_COOKIE,
    };
    enum Item item = ITEM_NONE;
    bool32 enemy = FALSE, tera = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 gimmick = 0; gimmick < 2; gimmick++)
            for (u32 medicine = 0; medicine < ARRAY_COUNT(items); medicine++)
                PARAMETRIZE { enemy = side; tera = gimmick; item = items[medicine]; }
    struct BattlePokemon *recipient = enemy ? opponent : player;
    struct BattlePokemon *other = enemy ? player : opponent;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(ABILITY_NONE); Gender(MON_MALE); HP(100); MaxHP(200); Speed(enemy ? 50 : 100);
            Moves(MOVE_SPLASH, MOVE_ATTRACT, MOVE_CONFUSE_RAY); TeraType(TYPE_NORMAL);
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(ABILITY_NONE); Gender(MON_FEMALE); HP(100); MaxHP(200); Speed(enemy ? 100 : 50);
            Moves(MOVE_SPLASH, MOVE_ATTRACT, MOVE_CONFUSE_RAY); TeraType(TYPE_NORMAL);
        }
    } WHEN {
        TURN { MOVE(recipient, MOVE_SPLASH, gimmick: tera ? GIMMICK_TERA : GIMMICK_NONE); MOVE(other, MOVE_ATTRACT); }
        TURN { MOVE(recipient, MOVE_SPLASH, WITH_RNG(RNG_INFATUATION, FALSE)); MOVE(other, MOVE_CONFUSE_RAY); }
        TURN { USE_ITEM(recipient, item, partyIndex: 0); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_ATTRACT, other);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CONFUSE_RAY, other);
        if (item == ITEM_FULL_RESTORE) HP_BAR(recipient, damage: -100);
    } THEN {
        EXPECT_EQ((u32)recipient->volatiles.confusionTimer, 0);
        EXPECT_EQ(!!recipient->volatiles.infatuation,
                  B_POLISHED_MOVE_BALANCE && !tera && item != ITEM_LAVA_COOKIE);
        EXPECT_EQ(recipient->hp, item == ITEM_FULL_RESTORE ? 200 : 100);
    }
}

SINGLE_BATTLE_TEST("Polished medicine closure matching cures update active and bench statuses on either side")
{
    static const struct { enum Item item; u32 status; } cases[] = {
        {ITEM_ANTIDOTE, STATUS1_POISON}, {ITEM_ANTIDOTE, STATUS1_TOXIC_POISON},
        {ITEM_BURN_HEAL, STATUS1_BURN}, {ITEM_ICE_HEAL, STATUS1_FREEZE},
        {ITEM_AWAKENING, STATUS1_SLEEP}, {ITEM_PARALYZE_HEAL, STATUS1_PARALYSIS},
        {ITEM_PECHA_BERRY, STATUS1_TOXIC_POISON}, {ITEM_RAWST_BERRY, STATUS1_BURN},
        {ITEM_ASPEAR_BERRY, STATUS1_FREEZE}, {ITEM_CHESTO_BERRY, STATUS1_SLEEP},
        {ITEM_CHERI_BERRY, STATUS1_PARALYSIS},
    };
    enum Item item = ITEM_NONE;
    u32 status = 0, index = 0;
    bool32 enemy = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 bench = 0; bench < 2; bench++)
            for (u32 medicine = 0; medicine < ARRAY_COUNT(cases); medicine++)
                PARAMETRIZE { enemy = side; index = bench; item = cases[medicine].item; status = cases[medicine].status; }
    struct BattlePokemon *user = enemy ? opponent : player;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); HP(1000); MaxHP(1000); Status1(!enemy && index == 0 ? status : STATUS1_NONE); }
        PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_NONE); HP(1000); MaxHP(1000); Status1(!enemy && index == 1 ? status : STATUS1_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); HP(1000); MaxHP(1000); Status1(enemy && index == 0 ? status : STATUS1_NONE); }
        OPPONENT(SPECIES_CHANSEY) { Ability(ABILITY_NONE); HP(1000); MaxHP(1000); Status1(enemy && index == 1 ? status : STATUS1_NONE); }
    } WHEN {
        TURN { USE_ITEM(user, item, partyIndex: index); }
        if (index) TURN { SWITCH(user, 1); }
    } THEN {
        EXPECT_EQ(user->status1, STATUS1_NONE);
        EXPECT_EQ(GetMonData(&(enemy ? gParties[B_TRAINER_OPPONENT_A] : gParties[B_TRAINER_PLAYER])[index], MON_DATA_STATUS), STATUS1_NONE);
    }
}

// This exercises the production bag eligibility function. It does not claim
// that USE_ITEM goes through the bag UI: that harness deliberately bypasses it.
SINGLE_BATTLE_TEST("Polished medicine closure bag eligibility rejects dead mismatched and ineffective targets")
{
    enum Item item = ITEM_NONE;
    u32 hp = 100, status = STATUS1_NONE;
    bool32 expected = TRUE;
    PARAMETRIZE { item = ITEM_ANTIDOTE; status = STATUS1_BURN; }
    PARAMETRIZE { item = ITEM_ANTIDOTE; status = STATUS1_POISON; expected = FALSE; }
    PARAMETRIZE { item = ITEM_FULL_HEAL; status = STATUS1_BURN; hp = 0; expected = B_POLISHED_MOVE_BALANCE; }
    PARAMETRIZE { item = ITEM_FULL_RESTORE; status = STATUS1_PARALYSIS; hp = 0; expected = B_POLISHED_MOVE_BALANCE; }
    PARAMETRIZE { item = ITEM_FULL_HEAL; }
    PARAMETRIZE { item = ITEM_FULL_RESTORE; }
    PARAMETRIZE { item = ITEM_FULL_RESTORE; hp = 99; expected = FALSE; }
    PARAMETRIZE { item = ITEM_REVIVE; }
    PARAMETRIZE { item = ITEM_REVIVE; hp = 0; expected = FALSE; }
    PARAMETRIZE { item = ITEM_LAVA_COOKIE; status = STATUS1_BURN; hp = 0; expected = FALSE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_NONE); HP(hp); MaxHP(100); Status1(status); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN {
        TURN {}
    } THEN {
        gBattlerInMenuId = B_BATTLER_0;
        gPartyMenu.slotId = 1;
        EXPECT_EQ(CannotUseItemsInBattle(item, &gParties[B_TRAINER_PLAYER][1]), expected);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_HP), hp);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_STATUS), status);
    }
}

SINGLE_BATTLE_TEST("Polished medicine closure bag eligibility does not treat attraction as a source all-status cure")
{
    bool32 confusion = FALSE, optional = FALSE;
    for (u32 confused = 0; confused < 2; confused++)
        for (u32 nativeItem = 0; nativeItem < 2; nativeItem++)
            PARAMETRIZE { confusion = confused; optional = nativeItem; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Gender(MON_MALE); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Gender(MON_FEMALE); Speed(50); }
    } WHEN {
        TURN { MOVE(opponent, confusion ? MOVE_CONFUSE_RAY : MOVE_ATTRACT); }
    } THEN {
        gBattlerInMenuId = B_BATTLER_0;
        gPartyMenu.slotId = 0;
        EXPECT_EQ(CannotUseItemsInBattle(optional ? ITEM_LAVA_COOKIE : ITEM_FULL_HEAL, &gParties[B_TRAINER_PLAYER][0]),
                  B_POLISHED_MOVE_BALANCE && !optional && !confusion);
    }
}

SINGLE_BATTLE_TEST("Polished medicine closure manual PP healing preserves PP Ups and leaves temporary moves unchanged")
{
    static const enum Item items[] = {ITEM_ETHER, ITEM_MAX_ETHER, ITEM_ELIXIR, ITEM_MAX_ELIXIR, ITEM_LEPPA_BERRY};
    enum Item item = ITEM_NONE;
    bool32 enemy = FALSE, transform = FALSE;
    u32 bonuses = 0xff;
    for (u32 side = 0; side < 2; side++)
        for (u32 transformed = 0; transformed < 2; transformed++)
            for (u32 medicine = 0; medicine < ARRAY_COUNT(items); medicine++)
                PARAMETRIZE { enemy = side; transform = transformed; item = items[medicine]; }
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *other = enemy ? player : opponent;
    GIVEN {
        WITH_CONFIG(B_POLISHED_MOVE_PP, B_POLISHED_MOVE_BALANCE);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(enemy ? 50 : 100); MovesWithPP({MOVE_TRANSFORM, 1}, {MOVE_GROWL, 1}, {MOVE_ROCK_BLAST, 2}, {MOVE_RECOVER, 1}); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(enemy ? 100 : 50); MovesWithPP({MOVE_TRANSFORM, 1}, {MOVE_GROWL, 1}, {MOVE_ROCK_BLAST, 2}, {MOVE_RECOVER, 1}); }
        SetMonData(&PLAYER_PARTY[0], MON_DATA_PP_BONUSES, &bonuses);
        SetMonData(&OPPONENT_PARTY[0], MON_DATA_PP_BONUSES, &bonuses);
    } WHEN {
        if (transform) TURN { MOVE(user, MOVE_TRANSFORM); USE_ITEM(other, ITEM_POTION, partyIndex: 0); }
        TURN {
            if (item == ITEM_ELIXIR || item == ITEM_MAX_ELIXIR) USE_ITEM(user, item, partyIndex: 0);
            else USE_ITEM(user, item, partyIndex: 0, move: MOVE_GROWL);
            USE_ITEM(other, ITEM_POTION, partyIndex: 0);
        }
    } THEN {
        struct Pokemon *mon = &(enemy ? gParties[B_TRAINER_OPPONENT_A] : gParties[B_TRAINER_PLAYER])[0];
        u32 expected = item == ITEM_MAX_ETHER || item == ITEM_MAX_ELIXIR ? (B_POLISHED_MOVE_BALANCE ? 61 : 64) : 11;
        EXPECT_EQ(GetMonData(mon, MON_DATA_PP_BONUSES), 0xff);
        EXPECT_EQ(GetMonData(mon, MON_DATA_PP2), expected);
        EXPECT_EQ(user->pp[1], transform ? 5 : expected);
        EXPECT_EQ(user->moves[1], MOVE_GROWL);
        EXPECT_EQ(GetMonData(&(enemy ? gParties[B_TRAINER_PLAYER] : gParties[B_TRAINER_OPPONENT_A])[0], MON_DATA_PP2), 1);
    }
}

SINGLE_BATTLE_TEST("Polished medicine closure PP bag eligibility checks all four slots and source maxima")
{
    u32 missingSlot = 0;
    for (u32 slot = 0; slot < 5; slot++) PARAMETRIZE { missingSlot = slot; }
    GIVEN {
        WITH_CONFIG(B_POLISHED_MOVE_PP, B_POLISHED_MOVE_BALANCE);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Moves(MOVE_GROWL, MOVE_ROCK_BLAST, MOVE_RAPID_SPIN, MOVE_RECOVER); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN {
        TURN {}
    } THEN {
        u32 bonuses = 0xff;
        SetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_PP_BONUSES, &bonuses);
        for (u32 slot = 0; slot < MAX_MON_MOVES; slot++) {
            u32 pp = CalculatePPWithBonus(GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_MOVE1 + slot), bonuses, slot);
            if (slot == missingSlot) pp--;
            SetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_PP1 + slot, &pp);
        }
        gBattlerInMenuId = B_BATTLER_0;
        gPartyMenu.slotId = 1;
        EXPECT_EQ(CannotUseItemsInBattle(ITEM_ELIXIR, &gParties[B_TRAINER_PLAYER][1]), missingSlot == 4);
        EXPECT_EQ(CannotUseItemsInBattle(ITEM_MAX_ELIXIR, &gParties[B_TRAINER_PLAYER][1]), missingSlot == 4);
        for (u32 slot = 0; slot < MAX_MON_MOVES; slot++) {
            gPartyMenu.data1 = slot;
            EXPECT_EQ(CannotUseItemsInBattle(ITEM_ETHER, &gParties[B_TRAINER_PLAYER][1]), slot != missingSlot);
            EXPECT_EQ(CannotUseItemsInBattle(ITEM_MAX_ETHER, &gParties[B_TRAINER_PLAYER][1]), slot != missingSlot);
        }
    }
}
