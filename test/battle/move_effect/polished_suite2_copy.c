#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished Suite 2 Transform keeps the target's current Roost types after end turn")
{
    bool32 fullHP = FALSE;
    PARAMETRIZE { fullHP = FALSE; }
    PARAMETRIZE { fullHP = TRUE; }
    GIVEN {
        PLAYER(SPECIES_DITTO) { Ability(ABILITY_NONE); Speed(50); MaxHP(1000); HP(1000); Moves(MOVE_TRANSFORM); }
        OPPONENT(SPECIES_SKARMORY) { Ability(ABILITY_NONE); Speed(100); MaxHP(1000); HP(fullHP ? 1000 : 500); Moves(MOVE_ROOST, MOVE_EARTHQUAKE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_ROOST); MOVE(player, MOVE_TRANSFORM); }
        TURN { MOVE(opponent, MOVE_EARTHQUAKE); }
    } SCENE {
        if (!fullHP) ANIMATION(ANIM_TYPE_MOVE, MOVE_ROOST, opponent);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TRANSFORM, player);
        if (B_POLISHED_MOVE_BALANCE && !fullHP) {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_EARTHQUAKE, opponent);
            HP_BAR(player);
        } else {
            NONE_OF {
                ANIMATION(ANIM_TYPE_MOVE, MOVE_EARTHQUAKE, opponent);
                HP_BAR(player);
            }
        }
    } THEN {
        EXPECT_EQ(player->types[0], TYPE_STEEL);
        EXPECT_EQ(player->types[1], B_POLISHED_MOVE_BALANCE && !fullHP ? TYPE_MYSTERY : TYPE_FLYING);
        EXPECT_EQ(opponent->types[1], TYPE_FLYING);
        EXPECT_EQ((u32)player->volatiles.roostActive, FALSE);
    }
}

SINGLE_BATTLE_TEST("Polished Suite 2 Transform preserves HP item status and party moves while copying stats types and PP")
{
    u32 bonuses = 0xff;
    GIVEN {
        PLAYER(SPECIES_DITTO) { Ability(ABILITY_LIMBER); Level(40); MaxHP(200); HP(150); Speed(50); Item(ITEM_LEFTOVERS); Status1(STATUS1_POISON); Moves(MOVE_TRANSFORM, MOVE_SPLASH); }
        SetMonData(&PLAYER_PARTY[0], MON_DATA_PP_BONUSES, &bonuses);
        OPPONENT(SPECIES_SMEARGLE) { Ability(ABILITY_OWN_TEMPO); Level(80); Speed(100); Attack(123); Defense(234); Moves(MOVE_SKETCH, MOVE_SPLASH, MOVE_GROWL); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_GROWL); MOVE(player, MOVE_TRANSFORM); }
    } THEN {
        EXPECT_EQ(player->species, SPECIES_SMEARGLE);
        EXPECT_EQ(player->attack, 123);
        EXPECT_EQ(player->defense, 234);
        EXPECT_EQ(player->statStages[STAT_ATK], DEFAULT_STAT_STAGE);
        EXPECT_EQ(player->maxHP, 200);
        EXPECT_EQ(player->level, 40);
        EXPECT_EQ(player->item, ITEM_LEFTOVERS);
        EXPECT_EQ(player->status1, STATUS1_POISON);
        EXPECT_EQ(player->pp[0], 1);
        EXPECT_EQ(player->pp[1], 5);
        EXPECT_EQ(player->pp[2], 5);
        EXPECT_EQ(player->pp[3], 0);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_MOVE1), MOVE_TRANSFORM);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_PP_BONUSES), bonuses);
        if (B_POLISHED_MOVE_BALANCE) {
            EXPECT_EQ(player->ppBonuses, 0);
            EXPECT_EQ(gLastMoves[B_BATTLER_0], MOVE_NONE);
        }
    }
}

SINGLE_BATTLE_TEST("Polished Suite 2 Transform activates the copied entry ability")
{
    enum Ability ability = ABILITY_INTIMIDATE;
    PARAMETRIZE { ability = ABILITY_INTIMIDATE; }
    PARAMETRIZE { ability = ABILITY_DOWNLOAD; }
    GIVEN {
        PLAYER(SPECIES_DITTO) { Ability(ABILITY_LIMBER); Speed(50); Moves(MOVE_TRANSFORM); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ability); Speed(100); Defense(100); SpDefense(200); Moves(MOVE_SPLASH); }
    } WHEN {
        TURN { MOVE(player, MOVE_TRANSFORM); MOVE(opponent, MOVE_SPLASH); }
    } SCENE {
        ABILITY_POPUP(opponent, ability);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TRANSFORM, player);
        if (B_POLISHED_MOVE_BALANCE) ABILITY_POPUP(player, ability);
        else NOT ABILITY_POPUP(player, ability);
    } THEN {
        if (ability == ABILITY_INTIMIDATE)
            EXPECT_EQ(opponent->statStages[STAT_ATK], DEFAULT_STAT_STAGE - B_POLISHED_MOVE_BALANCE);
    }
}

SINGLE_BATTLE_TEST("Polished Suite 2 Transform permits a second transformation and restores the original party identity")
{
    GIVEN {
        PLAYER(SPECIES_DITTO) { Ability(ABILITY_LIMBER); Speed(50); Moves(MOVE_TRANSFORM); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(60); }
        OPPONENT(SPECIES_SMEARGLE) { Ability(ABILITY_OWN_TEMPO); Speed(100); Moves(MOVE_TRANSFORM, MOVE_SPLASH); }
    } WHEN {
        TURN { MOVE(player, MOVE_TRANSFORM); MOVE(opponent, MOVE_SPLASH); }
        TURN { MOVE(player, moveSlot: 0); MOVE(opponent, MOVE_SPLASH); }
        TURN { SWITCH(player, 1); MOVE(opponent, MOVE_SPLASH); }
        TURN { SWITCH(player, 0); MOVE(opponent, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TRANSFORM, player);
        if (B_POLISHED_MOVE_BALANCE) ANIMATION(ANIM_TYPE_MOVE, MOVE_TRANSFORM, player);
        else NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_TRANSFORM, player);
    } THEN {
        EXPECT_EQ(player->species, SPECIES_DITTO);
        EXPECT_EQ(player->moves[0], MOVE_TRANSFORM);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_SPECIES), SPECIES_DITTO);
    }
}

SINGLE_BATTLE_TEST("Polished Suite 2 Transform rejects a transformed target")
{
    GIVEN {
        PLAYER(SPECIES_DITTO) { Ability(ABILITY_LIMBER); Speed(50); Moves(MOVE_TRANSFORM, MOVE_SPLASH); }
        OPPONENT(SPECIES_SMEARGLE) { Ability(ABILITY_OWN_TEMPO); Speed(100); Moves(MOVE_TRANSFORM, MOVE_SPLASH); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_TRANSFORM); MOVE(player, MOVE_TRANSFORM); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TRANSFORM, opponent);
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_TRANSFORM, player);
    } THEN {
        EXPECT_EQ((u32)player->volatiles.transformed, FALSE);
    }
}

SINGLE_BATTLE_TEST("Polished Suite 2 Transform respects Substitute and Infiltrator")
{
    enum Ability ability = ABILITY_LIMBER;
    PARAMETRIZE { ability = ABILITY_LIMBER; }
    PARAMETRIZE { ability = ABILITY_INFILTRATOR; }
    GIVEN {
        PLAYER(SPECIES_DITTO) { Ability(ability); Speed(50); Moves(MOVE_TRANSFORM); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); Moves(MOVE_SUBSTITUTE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_SUBSTITUTE); MOVE(player, MOVE_TRANSFORM); }
    } THEN {
        EXPECT_EQ((u32)player->volatiles.transformed, B_POLISHED_MOVE_BALANCE && ability == ABILITY_INFILTRATOR);
    }
}

SINGLE_BATTLE_TEST("Polished Suite 2 Transform and Imposter cannot copy Armored Mewtwo")
{
    enum Ability ability = ABILITY_LIMBER;
    PARAMETRIZE { ability = ABILITY_LIMBER; }
    PARAMETRIZE { ability = ABILITY_IMPOSTER; }
    GIVEN {
        PLAYER(SPECIES_DITTO) { Ability(ability); Moves(MOVE_TRANSFORM); }
        OPPONENT(SPECIES_MEWTWO_ARMORED) { Ability(ABILITY_INSOMNIA); Item(ITEM_POLISHED_ARMOR_SUIT); Moves(MOVE_SPLASH); }
    } WHEN {
        if (ability == ABILITY_LIMBER) TURN { MOVE(player, MOVE_TRANSFORM); MOVE(opponent, MOVE_SPLASH); }
        else TURN { MOVE(player, moveSlot: 0); MOVE(opponent, MOVE_SPLASH); }
    } THEN {
        EXPECT_EQ((u32)player->volatiles.transformed, !B_POLISHED_MOVE_BALANCE);
    }
}

SINGLE_BATTLE_TEST("Polished Suite 2 Transform copies stored Neutralizing Gas and clears its move history")
{
    GIVEN {
        PLAYER(SPECIES_DITTO) { Ability(ABILITY_LIMBER); Moves(MOVE_TRANSFORM); }
        OPPONENT(SPECIES_KOFFING) { Ability(ABILITY_NEUTRALIZING_GAS); Moves(MOVE_SPLASH); }
    } WHEN {
        TURN { MOVE(player, MOVE_TRANSFORM); MOVE(opponent, MOVE_SPLASH); }
    } THEN {
        EXPECT_EQ(player->ability, ABILITY_NEUTRALIZING_GAS);
        EXPECT_EQ((u32)player->volatiles.overwrittenAbility, ABILITY_NEUTRALIZING_GAS);
        if (B_POLISHED_MOVE_BALANCE) EXPECT_EQ(gLastMoves[B_BATTLER_0], MOVE_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Suite 2 Skill Swap rejects Neutralizing Gas")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NEUTRALIZING_GAS); Moves(MOVE_SKILL_SWAP); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_SOUNDPROOF); }
    } WHEN {
        TURN { MOVE(player, MOVE_SKILL_SWAP); }
    } THEN {
        EXPECT_EQ(player->ability, ABILITY_NEUTRALIZING_GAS);
    }
}

SINGLE_BATTLE_TEST("Polished Suite 2 Skill Swap rejects a hidden target")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NO_GUARD); Speed(50); Moves(MOVE_SKILL_SWAP); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_LIMBER); Speed(100); Moves(MOVE_DIG); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_DIG); MOVE(player, MOVE_SKILL_SWAP); }
    } THEN {
        EXPECT_EQ(player->ability, B_POLISHED_MOVE_BALANCE ? ABILITY_NO_GUARD : ABILITY_LIMBER);
    }
}

SINGLE_BATTLE_TEST("Polished Suite 2 Skill Swap runs the users entry ability before the targets")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_DRIZZLE); Speed(50); Moves(MOVE_SKILL_SWAP); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_DROUGHT); Speed(100); }
    } WHEN {
        TURN { MOVE(player, MOVE_SKILL_SWAP); }
    } THEN {
        if (B_POLISHED_MOVE_BALANCE) EXPECT(gBattleWeather & B_WEATHER_RAIN);
        EXPECT_EQ(player->ability, ABILITY_DROUGHT);
        EXPECT_EQ(opponent->ability, ABILITY_DRIZZLE);
    }
}

SINGLE_BATTLE_TEST("Polished Suite 2 Cud Chew keeps its berry separate from Recycle history")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_CUD_CHEW); MaxHP(400); HP(150); Item(ITEM_SITRUS_BERRY); Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); }
        TURN { MOVE(player, MOVE_SPLASH); }
        TURN { MOVE(player, MOVE_SPLASH); }
    } THEN {
        EXPECT_EQ(GetBattlerPartyState(B_BATTLER_0)->usedHeldItem, B_POLISHED_MOVE_BALANCE ? ITEM_SITRUS_BERRY : ITEM_NONE);
        EXPECT_EQ(player->item, ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Suite 2 Skill Swap clears pending Cud Chew on both sides")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_CUD_CHEW); Speed(50); MaxHP(400); HP(150); Item(ITEM_SITRUS_BERRY); Moves(MOVE_SPLASH, MOVE_SKILL_SWAP); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_SOUNDPROOF); Speed(100); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); }
        TURN { MOVE(player, MOVE_SKILL_SWAP); }
        TURN { MOVE(player, MOVE_SKILL_SWAP); }
    } THEN {
        if (B_POLISHED_MOVE_BALANCE) {
            EXPECT_EQ((u32)player->volatiles.polishedCudChewBerry, ITEM_NONE);
            EXPECT_EQ((u32)player->volatiles.cudChew, FALSE);
            EXPECT_EQ(player->hp, 250);
        }
    }
}

SINGLE_BATTLE_TEST("Polished Suite 2 two voluntary switches resolve entries sequentially")
{
    bool32 playerFirst = FALSE;
    PARAMETRIZE { playerFirst = FALSE; }
    PARAMETRIZE { playerFirst = TRUE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(playerFirst ? 100 : 50); }
        PLAYER(SPECIES_EKANS) { Ability(ABILITY_INTIMIDATE); Speed(60); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(playerFirst ? 50 : 100); }
        OPPONENT(SPECIES_EKANS) { Ability(ABILITY_INTIMIDATE); Speed(70); }
    } WHEN {
        TURN { SWITCH(player, 1); SWITCH(opponent, 1); }
    } THEN {
        EXPECT_EQ(player->statStages[STAT_ATK], DEFAULT_STAT_STAGE - (B_POLISHED_MOVE_BALANCE ? playerFirst : TRUE));
        EXPECT_EQ(opponent->statStages[STAT_ATK], DEFAULT_STAT_STAGE - (B_POLISHED_MOVE_BALANCE ? !playerFirst : FALSE));
    }
}

SINGLE_BATTLE_TEST("Polished Suite 2 Heal Bell clears active and benched Soundproof status")
{
    GIVEN {
        WITH_CONFIG(B_HEAL_BELL_SOUNDPROOF, GEN_4);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_SOUNDPROOF); Status1(STATUS1_POISON); Moves(MOVE_HEAL_BELL); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_SOUNDPROOF); Status1(STATUS1_BURN); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Status1(STATUS1_POISON); }
    } WHEN {
        TURN { MOVE(player, MOVE_HEAL_BELL); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_HEAL_BELL, player);
    } THEN {
        EXPECT_EQ(player->status1, B_POLISHED_MOVE_BALANCE ? 0 : STATUS1_POISON);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_STATUS), B_POLISHED_MOVE_BALANCE ? 0 : STATUS1_POISON);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_STATUS), B_POLISHED_MOVE_BALANCE ? 0 : STATUS1_BURN);
        EXPECT_EQ(opponent->status1, STATUS1_POISON);
    }
}

SINGLE_BATTLE_TEST("Polished Suite 2 Transform remaps a surviving Disable identity and clears its countdown")
{
    bool32 keepsMove = FALSE;
    PARAMETRIZE { keepsMove = FALSE; }
    PARAMETRIZE { keepsMove = TRUE; }
    GIVEN {
        PLAYER(SPECIES_DITTO) { Ability(ABILITY_LIMBER); Speed(100); Moves(MOVE_SPLASH, MOVE_TRANSFORM); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_SOUNDPROOF); Speed(50); Moves(MOVE_DISABLE, keepsMove ? MOVE_SPLASH : MOVE_GROWL); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); MOVE(opponent, MOVE_DISABLE, hit: TRUE); }
        TURN { MOVE(player, MOVE_TRANSFORM); MOVE(opponent, moveSlot: 1); }
    } THEN {
        EXPECT_EQ((u32)player->volatiles.disabledMove, B_POLISHED_MOVE_BALANCE && keepsMove ? MOVE_SPLASH : MOVE_NONE);
        EXPECT_EQ((u32)player->volatiles.disableTimer, 0);
    }
}

SINGLE_BATTLE_TEST("Polished Suite 2 voluntary switches obey Trick Room Lagging Tail and Custap")
{
    enum Item item = ITEM_NONE;
    bool32 room = TRUE;
    PARAMETRIZE { item = ITEM_NONE; room = TRUE; }
    PARAMETRIZE { item = ITEM_LAGGING_TAIL; room = FALSE; }
    PARAMETRIZE { item = ITEM_CUSTAP_BERRY; room = FALSE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_SOUNDPROOF); Speed(item == ITEM_CUSTAP_BERRY ? 50 : 100); MaxHP(100); HP(20); Item(item); Moves(MOVE_TRICK_ROOM); }
        PLAYER(SPECIES_EKANS) { Ability(ABILITY_INTIMIDATE); Speed(70); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_SOUNDPROOF); Speed(item == ITEM_CUSTAP_BERRY ? 100 : 50); }
        OPPONENT(SPECIES_EKANS) { Ability(ABILITY_INTIMIDATE); Speed(60); }
    } WHEN {
        if (room) TURN { MOVE(player, MOVE_TRICK_ROOM); }
        TURN { SWITCH(player, 1); SWITCH(opponent, 1); }
    } THEN {
        bool32 playerFirst = !B_POLISHED_MOVE_BALANCE || item == ITEM_CUSTAP_BERRY;
        EXPECT_EQ(player->statStages[STAT_ATK], DEFAULT_STAT_STAGE - playerFirst);
        EXPECT_EQ(opponent->statStages[STAT_ATK], DEFAULT_STAT_STAGE - !playerFirst);
        if (item == ITEM_CUSTAP_BERRY)
            EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_HELD_ITEM), B_POLISHED_MOVE_BALANCE ? ITEM_NONE : ITEM_CUSTAP_BERRY);
    }
}

SINGLE_BATTLE_TEST("Polished Suite 2 Quick Claw can order two voluntary switches")
{
    PASSES_RANDOMLY(B_POLISHED_MOVE_BALANCE ? 20 : 100, 100, RNG_QUICK_CLAW);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_SOUNDPROOF); Speed(50); Item(ITEM_QUICK_CLAW); }
        PLAYER(SPECIES_EKANS) { Ability(ABILITY_INTIMIDATE); Speed(70); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_SOUNDPROOF); Speed(100); }
        OPPONENT(SPECIES_EKANS) { Ability(ABILITY_INTIMIDATE); Speed(60); }
    } WHEN {
        TURN { SWITCH(player, 1); SWITCH(opponent, 1); }
    } THEN {
        EXPECT_EQ(player->statStages[STAT_ATK], DEFAULT_STAT_STAGE - 1);
        EXPECT_EQ(opponent->statStages[STAT_ATK], DEFAULT_STAT_STAGE);
    }
}
