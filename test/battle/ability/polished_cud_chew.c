#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished Cud Chew silently expires a healing retry at full HP")
{
    struct BattlePokemon *holder = player, *other = opponent;
    PARAMETRIZE { holder = player; other = opponent; }
    PARAMETRIZE { holder = opponent; other = player; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(holder == player ? ABILITY_CUD_CHEW : ABILITY_NONE); Item(holder == player ? ITEM_ORAN_BERRY : ITEM_NONE); Speed(holder == player ? 100 : 50); HP(60); MaxHP(60); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(holder == opponent ? ABILITY_CUD_CHEW : ABILITY_NONE); Item(holder == opponent ? ITEM_ORAN_BERRY : ITEM_NONE); Speed(holder == opponent ? 100 : 50); HP(60); MaxHP(60); }
    } WHEN {
        TURN { MOVE(holder, MOVE_SPLASH); MOVE(other, MOVE_DRAGON_RAGE); }
        TURN { MOVE(holder, MOVE_RECOVER); MOVE(other, MOVE_SPLASH); }
        TURN { MOVE(holder, MOVE_SPLASH); MOVE(other, MOVE_DRAGON_RAGE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_DRAGON_RAGE, other);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, holder);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_RECOVER, holder);
        if (B_POLISHED_MOVE_BALANCE) NOT ABILITY_POPUP(holder, ABILITY_CUD_CHEW);
        else ABILITY_POPUP(holder, ABILITY_CUD_CHEW);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_DRAGON_RAGE, other);
        NONE_OF {
            ABILITY_POPUP(holder, ABILITY_CUD_CHEW);
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, holder);
        }
    } THEN {
        EXPECT_EQ(holder->hp, 20);
        EXPECT_EQ(holder->item, ITEM_NONE);
        if (B_POLISHED_MOVE_BALANCE)
            EXPECT_EQ((u32)holder->volatiles.polishedCudChewBerry, ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Cud Chew announces a status retry only when it can cure the new status")
{
    struct BattlePokemon *holder = player, *other = opponent;
    bool32 afflicted = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 repeat = 0; repeat < 2; repeat++)
            PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; afflicted = repeat; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(holder == player ? ABILITY_CUD_CHEW : ABILITY_NONE); Item(holder == player ? ITEM_LUM_BERRY : ITEM_NONE); Speed(holder == player ? 100 : 50); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(holder == opponent ? ABILITY_CUD_CHEW : ABILITY_NONE); Item(holder == opponent ? ITEM_LUM_BERRY : ITEM_NONE); Speed(holder == opponent ? 100 : 50); }
    } WHEN {
        TURN { MOVE(holder, MOVE_SPLASH); MOVE(other, MOVE_THUNDER_WAVE, hit: TRUE); }
        TURN { MOVE(holder, MOVE_SPLASH); MOVE(other, afflicted ? MOVE_THUNDER_WAVE : MOVE_SPLASH, hit: TRUE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_THUNDER_WAVE, other);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, holder);
        if (afflicted) {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_THUNDER_WAVE, other);
            ABILITY_POPUP(holder, ABILITY_CUD_CHEW);
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, holder);
        } else {
            if (B_POLISHED_MOVE_BALANCE) {
                NONE_OF {
                    ABILITY_POPUP(holder, ABILITY_CUD_CHEW);
                    ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, holder);
                }
            } else {
                ABILITY_POPUP(holder, ABILITY_CUD_CHEW);
                NOT ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, holder);
            }
        }
    } THEN {
        EXPECT_EQ(holder->status1, STATUS1_NONE);
        EXPECT_EQ(holder->item, ITEM_NONE);
        if (B_POLISHED_MOVE_BALANCE)
            EXPECT_EQ((u32)holder->volatiles.polishedCudChewBerry, ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Cud Chew silently expires Lansat while its critical boost is still active")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_CUD_CHEW); Item(ITEM_LANSAT_BERRY); HP(60); MaxHP(80); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_DRAGON_RAGE); }
        TURN { MOVE(player, MOVE_SPLASH); MOVE(opponent, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, player);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, player);
        if (B_POLISHED_MOVE_BALANCE) {
            NONE_OF {
                ABILITY_POPUP(player, ABILITY_CUD_CHEW);
                ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, player);
            }
        } else {
            ABILITY_POPUP(player, ABILITY_CUD_CHEW);
            NOT ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, player);
        }
    } THEN {
        EXPECT(player->volatiles.focusEnergy);
        EXPECT_EQ(player->item, ITEM_NONE);
        if (B_POLISHED_MOVE_BALANCE)
            EXPECT_EQ((u32)player->volatiles.polishedCudChewBerry, ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Cud Chew silently expires a stat retry after reaching the cap")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_CUD_CHEW); Item(ITEM_LIECHI_BERRY); HP(60); MaxHP(80); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); }
    } WHEN {
        TURN { MOVE(player, MOVE_SWORDS_DANCE); }
        TURN { MOVE(player, MOVE_SWORDS_DANCE); }
        TURN { MOVE(player, MOVE_SWORDS_DANCE); MOVE(opponent, MOVE_GROWL); }
        TURN { MOVE(player, MOVE_SPLASH); MOVE(opponent, MOVE_DRAGON_RAGE); }
        TURN { MOVE(player, MOVE_SPLASH); MOVE(opponent, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, player);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, player);
        if (B_POLISHED_MOVE_BALANCE) {
            NONE_OF {
                ABILITY_POPUP(player, ABILITY_CUD_CHEW);
                ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, player);
            }
        } else {
            ABILITY_POPUP(player, ABILITY_CUD_CHEW);
            NOT ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, player);
        }
    } THEN {
        EXPECT_EQ(player->statStages[STAT_ATK], MAX_STAT_STAGE);
        EXPECT_EQ(player->item, ITEM_NONE);
        if (B_POLISHED_MOVE_BALANCE)
            EXPECT_EQ((u32)player->volatiles.polishedCudChewBerry, ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Cud Chew retries confusion once and silently expires an ineffective Persim")
{
    struct BattlePokemon *holder = player, *other = opponent;
    bool32 afflicted = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 repeat = 0; repeat < 2; repeat++)
            PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; afflicted = repeat; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(holder == player ? ABILITY_CUD_CHEW : ABILITY_NONE); Item(holder == player ? ITEM_PERSIM_BERRY : ITEM_NONE); Speed(holder == player ? 100 : 50); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(holder == opponent ? ABILITY_CUD_CHEW : ABILITY_NONE); Item(holder == opponent ? ITEM_PERSIM_BERRY : ITEM_NONE); Speed(holder == opponent ? 100 : 50); }
    } WHEN {
        // The holder moves first so each new confusion reaches the healing
        // checkpoint without an intervening confusion-duration or self-hit roll.
        TURN { MOVE(holder, MOVE_SPLASH); MOVE(other, MOVE_CONFUSE_RAY); }
        TURN { MOVE(holder, MOVE_SPLASH); MOVE(other, afflicted ? MOVE_CONFUSE_RAY : MOVE_SPLASH); }
        TURN { MOVE(holder, MOVE_SPLASH); MOVE(other, MOVE_CONFUSE_RAY); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CONFUSE_RAY, other);
        ANIMATION(ANIM_TYPE_STATUS, B_ANIM_STATUS_CONFUSION, holder);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, holder);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, holder);
        if (afflicted) {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_CONFUSE_RAY, other);
            ANIMATION(ANIM_TYPE_STATUS, B_ANIM_STATUS_CONFUSION, holder);
            ABILITY_POPUP(holder, ABILITY_CUD_CHEW);
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, holder);
        } else if (B_POLISHED_MOVE_BALANCE) {
            NONE_OF {
                ABILITY_POPUP(holder, ABILITY_CUD_CHEW);
                ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, holder);
            }
        } else {
            ABILITY_POPUP(holder, ABILITY_CUD_CHEW);
            NOT ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, holder);
        }
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, holder);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CONFUSE_RAY, other);
        ANIMATION(ANIM_TYPE_STATUS, B_ANIM_STATUS_CONFUSION, holder);
        NONE_OF {
            ABILITY_POPUP(holder, ABILITY_CUD_CHEW);
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, holder);
        }
    } THEN {
        EXPECT(holder->volatiles.confusionTimer > 0);
        EXPECT_EQ(holder->item, ITEM_NONE);
        if (B_POLISHED_MOVE_BALANCE)
            EXPECT_EQ((u32)holder->volatiles.polishedCudChewBerry, ITEM_NONE);
    }
}
