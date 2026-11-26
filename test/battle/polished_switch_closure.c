#include "global.h"
#include "test/battle.h"

// Source core.asm:ForceDeferredSwitch/GetUserSwitchTarget use the same
// reserve-selection path in trainer and wild battles. Red Card's item flags
// request a forced switch, without Roar's wild-battle escape behavior.
WILD_BATTLE_TEST("Polished switch closure wild Red Card switches the player before the pending enemy action")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); HP(1000); MaxHP(1000); }
        PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Speed(100); HP(1000); MaxHP(1000); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Item(ITEM_RED_CARD); Speed(50); HP(1000); MaxHP(1000); }
    } WHEN {
        TURN { MOVE(player, MOVE_TACKLE); MOVE(opponent, MOVE_TACKLE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, opponent);
        if (B_POLISHED_MOVE_BALANCE) {
            MESSAGE("Chansey was dragged out!");
            ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, opponent);
            HP_BAR(player);
        } else {
            NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, opponent);
        }
    } THEN {
        EXPECT_EQ(opponent->item, ITEM_NONE);
        if (B_POLISHED_MOVE_BALANCE) {
            EXPECT_EQ(player->species, SPECIES_CHANSEY);
            EXPECT_LT(player->hp, 1000);
            EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_HP), 1000);
        }
    }
}

WILD_BATTLE_TEST("Polished switch closure wild Red Card retains itself without a living player reserve")
{
    bool32 fainted = FALSE;
    PARAMETRIZE { fainted = FALSE; }
    PARAMETRIZE { fainted = TRUE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); Ability(ABILITY_NONE); HP(1000); MaxHP(1000); }
        if (fainted) PLAYER(SPECIES_CHANSEY) { Speed(100); Ability(ABILITY_NONE); HP(0); }
        OPPONENT(SPECIES_WYNAUT) { Speed(50); Ability(ABILITY_NONE); Item(ITEM_RED_CARD); HP(1000); MaxHP(1000); }
    } WHEN {
        TURN { MOVE(player, MOVE_TACKLE); }
        TURN { MOVE(player, MOVE_TACKLE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player);
        NOT ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, opponent);
    } THEN {
        EXPECT_EQ(player->species, SPECIES_WOBBUFFET);
        EXPECT_EQ(opponent->item, ITEM_RED_CARD);
    }
}

WILD_BATTLE_TEST("Polished switch closure optional wild Red Card attacks retain native encounter termination")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); HP(1000); MaxHP(1000); }
        PLAYER(SPECIES_CHANSEY) { Speed(100); Ability(ABILITY_NONE); HP(1000); MaxHP(1000); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Item(ITEM_RED_CARD); Speed(50); HP(1000); MaxHP(1000); }
    } WHEN {
        TURN { MOVE(player, MOVE_OVERHEAT); MOVE(opponent, MOVE_TACKLE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_OVERHEAT, player);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, opponent);
        NONE_OF {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, opponent);
            MESSAGE("Chansey was dragged out!");
        }
    } THEN {
        EXPECT_EQ(opponent->item, ITEM_NONE);
    }
}

// GetUserSwitchTarget rejects the current slot and zero-HP party members,
// leaving each living reserve equally likely. Sweep the bounded picker tag;
// this checks the distribution, without claiming identical source RNG calls.
SINGLE_BATTLE_TEST("Polished switch closure trainer Red Card uniformly selects each living reserve")
{
    struct BattlePokemon *victim = player, *holder = opponent;
    u32 current = 0;
    enum Species expected = SPECIES_BULBASAUR;
    for (u32 side = 0; side < 2; side++)
        for (u32 active = 0; active < 2; active++)
            for (u32 choice = 0; choice < 2; choice++)
                PARAMETRIZE {
                    victim = side ? opponent : player;
                    holder = side ? player : opponent;
                    current = active;
                    expected = choice ? SPECIES_CHARMANDER : active ? SPECIES_WOBBUFFET : SPECIES_BULBASAUR;
                }
    PASSES_RANDOMLY(1, 2, RNG_FORCE_RANDOM_SWITCH);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(holder == player ? ITEM_RED_CARD : ITEM_NONE); Speed(100); HP(1000); MaxHP(1000); }
        if (victim == player) {
            PLAYER(SPECIES_BULBASAUR) { Ability(ABILITY_NONE); Speed(100); HP(1000); MaxHP(1000); }
            PLAYER(SPECIES_SQUIRTLE) { Ability(ABILITY_NONE); Speed(100); HP(0); }
            PLAYER(SPECIES_CHARMANDER) { Ability(ABILITY_NONE); Speed(100); HP(1000); MaxHP(1000); }
        }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(holder == opponent ? ITEM_RED_CARD : ITEM_NONE); Speed(50); HP(1000); MaxHP(1000); }
        if (victim == opponent) {
            OPPONENT(SPECIES_BULBASAUR) { Ability(ABILITY_NONE); Speed(50); HP(1000); MaxHP(1000); }
            OPPONENT(SPECIES_SQUIRTLE) { Ability(ABILITY_NONE); Speed(50); HP(0); }
            OPPONENT(SPECIES_CHARMANDER) { Ability(ABILITY_NONE); Speed(50); HP(1000); MaxHP(1000); }
        }
    } WHEN {
        if (current) TURN { SWITCH(victim, current); MOVE(holder, MOVE_SPLASH); }
        TURN { MOVE(victim, MOVE_TACKLE); MOVE(holder, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, victim);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, holder);
    } THEN {
        EXPECT_EQ(holder->item, ITEM_NONE);
        EXPECT_EQ(victim->species, expected);
    }
}

WILD_BATTLE_TEST("Polished switch closure wild Red Card uniformly selects each living reserve")
{
    u32 current = 0;
    enum Species expected = SPECIES_BULBASAUR;
    for (u32 active = 0; active < 2; active++)
        for (u32 choice = 0; choice < (B_POLISHED_MOVE_BALANCE ? 2 : 1); choice++)
            PARAMETRIZE {
                current = active;
                expected = choice ? SPECIES_CHARMANDER : active ? SPECIES_WOBBUFFET : SPECIES_BULBASAUR;
            }
    if (B_POLISHED_MOVE_BALANCE) PASSES_RANDOMLY(1, 2, RNG_FORCE_RANDOM_SWITCH);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); HP(1000); MaxHP(1000); }
        PLAYER(SPECIES_BULBASAUR) { Ability(ABILITY_NONE); Speed(100); HP(1000); MaxHP(1000); }
        PLAYER(SPECIES_SQUIRTLE) { Ability(ABILITY_NONE); Speed(100); HP(0); }
        PLAYER(SPECIES_CHARMANDER) { Ability(ABILITY_NONE); Speed(100); HP(1000); MaxHP(1000); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Item(ITEM_RED_CARD); Speed(50); HP(1000); MaxHP(1000); Moves(MOVE_SPLASH); }
    } WHEN {
        if (current) TURN { SWITCH(player, current); MOVE(opponent, MOVE_SPLASH); }
        TURN { MOVE(player, MOVE_TACKLE); MOVE(opponent, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, opponent);
        if (!B_POLISHED_MOVE_BALANCE) NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, opponent);
    } THEN {
        EXPECT_EQ(opponent->item, ITEM_NONE);
        EXPECT_EQ(player->species, B_POLISHED_MOVE_BALANCE ? expected : current ? SPECIES_BULBASAUR : SPECIES_WOBBUFFET);
    }
}

WILD_BATTLE_TEST("Polished switch closure a fainted Mean Look user still traps the pending Teleport")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); Attack(1); HP(1000); MaxHP(1000); }
        PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Speed(100); HP(1000); MaxHP(1000); }
        // The wild opponent uses its actual AI, so give it only Teleport.
        // Toxic Orb starts poison after turn one. The lethal residual after
        // turn two then ends a source battle before the wild replacement UI.
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Item(ITEM_TOXIC_ORB); Speed(50); Defense(999); HP(100); MaxHP(1600); Moves(MOVE_TELEPORT); }
    } WHEN {
        TURN { MOVE(player, MOVE_MEAN_LOOK); MOVE(opponent, MOVE_TELEPORT); }
        TURN { MOVE(player, MOVE_EXPLOSION); MOVE(opponent, MOVE_TELEPORT); }
    } SCENE {
        // Status-icon recording only observes recorded controllers. The wild
        // opponent's real controller still emits the status message.
        MESSAGE("The wild Wynaut was badly poisoned!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_EXPLOSION, player);
        MESSAGE("Wobbuffet fainted!");
        if (B_POLISHED_MOVE_BALANCE) {
            MESSAGE("The wild Wynaut used Teleport!");
            MESSAGE("But it failed!");
            HP_BAR(opponent, hp: 0);
            MESSAGE("The wild Wynaut fainted!");
        } else {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_TELEPORT, opponent);
        }
    } THEN {
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_HP), 0);
        EXPECT_EQ(gBattleOutcome, B_POLISHED_MOVE_BALANCE ? B_OUTCOME_WON : B_OUTCOME_MON_TELEPORTED);
        if (!B_POLISHED_MOVE_BALANCE) EXPECT_EQ(opponent->status1 & STATUS1_ANY, STATUS1_TOXIC_POISON);
    }
}

SINGLE_BATTLE_TEST("Polished switch closure replacement releases the fainted Mean Look users trap")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); }
        PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Speed(100); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(50); HP(5000); MaxHP(5000); }
    } WHEN {
        TURN { MOVE(player, MOVE_MEAN_LOOK); MOVE(opponent, MOVE_SPLASH); }
        TURN { MOVE(player, MOVE_EXPLOSION); MOVE(opponent, MOVE_SPLASH); SEND_OUT(player, 1); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_EXPLOSION, player);
        MESSAGE("Wobbuffet fainted!");
        SEND_IN_MESSAGE("Chansey");
    } THEN {
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_HP), 0);
        EXPECT_EQ(player->species, SPECIES_CHANSEY);
        EXPECT_EQ((u32)opponent->volatiles.escapePrevention, FALSE);
    }
}

WILD_BATTLE_TEST("Polished switch closure a last party faint ends battle before the pending Teleport")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(50); HP(5000); MaxHP(5000); Moves(MOVE_TELEPORT); }
    } WHEN {
        TURN { MOVE(player, MOVE_MEAN_LOOK); MOVE(opponent, MOVE_TELEPORT); }
        TURN { MOVE(player, MOVE_EXPLOSION); MOVE(opponent, MOVE_TELEPORT); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_EXPLOSION, player);
        MESSAGE("Wobbuffet fainted!");
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_TELEPORT, opponent);
    } THEN {
        EXPECT_EQ(gBattleOutcome, B_OUTCOME_LOST);
    }
}

SINGLE_BATTLE_TEST("Polished switch closure Red Card Toxic Spikes synchronize before curing the entrant")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); }
        PLAYER(SPECIES_ABRA) { Speed(100); Ability(ABILITY_SYNCHRONIZE); Item(ITEM_PECHA_BERRY); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Item(ITEM_RED_CARD); Speed(50); HP(1000); MaxHP(1000); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); MOVE(opponent, MOVE_TOXIC_SPIKES); }
        TURN { MOVE(player, MOVE_TACKLE); MOVE(opponent, MOVE_SPLASH); }
    } SCENE {
        MESSAGE("Abra was dragged out!");
        if (B_POLISHED_MOVE_BALANCE) {
            ABILITY_POPUP(player, ABILITY_SYNCHRONIZE);
            STATUS_ICON(opponent, STATUS1_POISON);
        } else {
            NOT ABILITY_POPUP(player, ABILITY_SYNCHRONIZE);
        }
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, player);
    } THEN {
        EXPECT_EQ(player->species, SPECIES_ABRA);
        EXPECT_EQ(player->status1 & STATUS1_ANY, STATUS1_NONE);
        EXPECT_EQ(opponent->status1 & STATUS1_ANY, B_POLISHED_MOVE_BALANCE ? STATUS1_POISON : STATUS1_NONE);
    }
}

WILD_BATTLE_TEST("Polished switch closure wild Red Card uses ordinary entry status reflection")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); }
        PLAYER(SPECIES_ABRA) { Speed(100); Ability(ABILITY_SYNCHRONIZE); Item(ITEM_PECHA_BERRY); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Item(ITEM_RED_CARD); Speed(50); HP(1000); MaxHP(1000); Moves(MOVE_TOXIC_SPIKES); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); MOVE(opponent, MOVE_TOXIC_SPIKES); }
        TURN { MOVE(player, MOVE_TACKLE); MOVE(opponent, MOVE_TOXIC_SPIKES); }
    } SCENE {
        if (B_POLISHED_MOVE_BALANCE) {
            MESSAGE("Abra was dragged out!");
            ABILITY_POPUP(player, ABILITY_SYNCHRONIZE);
            MESSAGE("The wild Wynaut was poisoned!");
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, player);
        } else {
            NONE_OF {
                MESSAGE("Abra was dragged out!");
                ABILITY_POPUP(player, ABILITY_SYNCHRONIZE);
            }
        }
    } THEN {
        EXPECT_EQ(player->species, B_POLISHED_MOVE_BALANCE ? SPECIES_ABRA : SPECIES_WOBBUFFET);
        EXPECT_EQ(player->status1 & STATUS1_ANY, STATUS1_NONE);
        EXPECT_EQ(opponent->status1 & STATUS1_ANY, B_POLISHED_MOVE_BALANCE ? STATUS1_POISON : STATUS1_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished switch closure Red Card does not carry outgoing Mold Breaker into entry hazards")
{
    GIVEN {
        PLAYER(SPECIES_PINSIR) { Ability(ABILITY_MOLD_BREAKER); Speed(100); HP(1000); MaxHP(1000); }
        PLAYER(SPECIES_KOFFING) { Speed(100); Ability(ABILITY_LEVITATE); HP(1600); MaxHP(1600); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Item(ITEM_RED_CARD); Speed(50); HP(1000); MaxHP(1000); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); MOVE(opponent, MOVE_SPIKES); }
        TURN { MOVE(player, MOVE_TACKLE); MOVE(opponent, MOVE_SPLASH); }
    } THEN {
        EXPECT_EQ(player->species, SPECIES_KOFFING);
        if (B_POLISHED_MOVE_BALANCE) EXPECT_EQ(player->hp, 1600);
    }
}

SINGLE_BATTLE_TEST("Polished switch closure Eject Button does not carry attacking Mold Breaker into entry hazards")
{
    GIVEN {
        PLAYER(SPECIES_PINSIR) { Ability(ABILITY_MOLD_BREAKER); Speed(100); HP(1000); MaxHP(1000); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Item(ITEM_EJECT_BUTTON); Speed(50); HP(1000); MaxHP(1000); }
        OPPONENT(SPECIES_KOFFING) { Speed(50); Ability(ABILITY_LEVITATE); HP(1600); MaxHP(1600); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPIKES); MOVE(opponent, MOVE_SPLASH); }
        TURN { MOVE(player, MOVE_TACKLE); MOVE(opponent, MOVE_SPLASH); SEND_OUT(opponent, 1); }
    } THEN {
        EXPECT_EQ(opponent->species, SPECIES_KOFFING);
        if (B_POLISHED_MOVE_BALANCE) EXPECT_EQ(opponent->hp, 1600);
    }
}

SINGLE_BATTLE_TEST("Polished switch closure Roar discards the outgoing targets pending Roar")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); }
        PLAYER(SPECIES_CHANSEY) { Speed(100); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(50); Moves(MOVE_ROAR); }
        OPPONENT(SPECIES_RATTATA) { Speed(50); Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(player, MOVE_ROAR); MOVE(opponent, MOVE_ROAR); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_ROAR, player);
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_ROAR, opponent);
    } THEN {
        EXPECT_EQ(player->species, SPECIES_WOBBUFFET);
        EXPECT_EQ(opponent->species, SPECIES_RATTATA);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_PP1), GetMovePP(MOVE_ROAR));
    }
}

SINGLE_BATTLE_TEST("Polished switch closure Eject Button discards the outgoing targets pending attack")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); HP(1000); MaxHP(1000); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Item(ITEM_EJECT_BUTTON); Speed(50); HP(1000); MaxHP(1000); Moves(MOVE_TACKLE); }
        OPPONENT(SPECIES_RATTATA) { Speed(50); Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(player, MOVE_TACKLE); MOVE(opponent, MOVE_TACKLE); SEND_OUT(opponent, 1); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player);
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, opponent);
    } THEN {
        EXPECT_EQ(player->hp, 1000);
        EXPECT_EQ(opponent->species, SPECIES_RATTATA);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_PP1), GetMovePP(MOVE_TACKLE));
    }
}

SINGLE_BATTLE_TEST("Polished switch closure Pursuit knockout prevents the outgoing Regenerator recovery")
{
    bool32 knockout = FALSE;
    PARAMETRIZE { knockout = FALSE; }
    PARAMETRIZE { knockout = TRUE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); Ability(ABILITY_REGENERATOR); HP(knockout ? 1 : 1000); MaxHP(1000); }
        PLAYER(SPECIES_CHANSEY) { Speed(100); Ability(ABILITY_NONE); HP(1000); MaxHP(1000); }
        OPPONENT(SPECIES_WYNAUT) { Speed(50); Ability(ABILITY_NONE); }
    } WHEN {
        TURN { SWITCH(player, 1); MOVE(opponent, MOVE_PURSUIT); if (knockout) SEND_OUT(player, 1); }
    } THEN {
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_HP), knockout ? 0 : 1000);
        EXPECT_EQ(player->species, SPECIES_CHANSEY);
        EXPECT_EQ(player->hp, 1000);
    }
}
