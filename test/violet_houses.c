#include "global.h"
#include "event_data.h"
#include "overworld.h"
#include "pokemon.h"
#include "string_util.h"
#include "trade.h"
#include "constants/items.h"
#include "constants/maps.h"
#include "test/test.h"

extern u16 GetInGameTradeSpeciesInfo(void);
extern void CreateInGameTradePokemon(void);

TEST("Violet houses preserve earlier saved doors and have reciprocal exits")
{
    const struct MapHeader *city = Overworld_GetMapHeaderByGroupAndId(MAP_GROUP(MAP_POLISHED_VIOLET_CITY), MAP_NUM(MAP_POLISHED_VIOLET_CITY));
    const struct MapHeader *nickname = Overworld_GetMapHeaderByGroupAndId(MAP_GROUP(MAP_POLISHED_VIOLET_NICKNAME_SPEECH_HOUSE), MAP_NUM(MAP_POLISHED_VIOLET_NICKNAME_SPEECH_HOUSE));
    const struct MapHeader *trade = Overworld_GetMapHeaderByGroupAndId(MAP_GROUP(MAP_POLISHED_VIOLET_ONIX_TRADE_HOUSE), MAP_NUM(MAP_POLISHED_VIOLET_ONIX_TRADE_HOUSE));
    EXPECT_EQ(city->events->warps[3].mapNum, MAP_NUM(MAP_POLISHED_EARLS_POKEMON_ACADEMY));
    EXPECT_EQ(city->events->warps[4].mapNum, MAP_NUM(MAP_POLISHED_VIOLET_MART));
    EXPECT_EQ(city->events->warps[5].mapNum, MAP_NUM(MAP_POLISHED_VIOLET_NICKNAME_SPEECH_HOUSE));
    EXPECT_EQ(city->events->warps[6].mapNum, MAP_NUM(MAP_POLISHED_VIOLET_ONIX_TRADE_HOUSE));
    EXPECT_EQ(nickname->events->warps[0].warpId, 5);
    EXPECT_EQ(nickname->events->warps[1].warpId, 5);
    EXPECT_EQ(trade->events->warps[0].warpId, 6);
    EXPECT_EQ(trade->events->warps[1].warpId, 6);
}

TEST("Violet houses Kyle creates source Mimic at the offered Pokemon's level")
{
    u32 level;
    PARAMETRIZE { level = 5; }
    PARAMETRIZE { level = 15; }
    PARAMETRIZE { level = 100; }
    struct Pokemon originalPlayer = gParties[B_TRAINER_PLAYER][0], originalEnemy = gParties[B_TRAINER_OPPONENT_A][0];
    u16 slot = gSpecialVar_0x8004, trade = gSpecialVar_0x8005;
    u8 name[POKEMON_NAME_BUFFER_SIZE];
    gSpecialVar_0x8004 = 0;
    gSpecialVar_0x8005 = INGAME_TRADE_POLISHED_KYLE;
    CreateMon(&gParties[B_TRAINER_PLAYER][0], SPECIES_POLIWAG, level, 0, OTID_STRUCT_PLAYER_ID);
    EXPECT_EQ(GetInGameTradeSpeciesInfo(), SPECIES_POLIWAG);
    CreateInGameTradePokemon();
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_SPECIES), SPECIES_VOLTORB);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_LEVEL), level);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_OT_ID), 48926);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_POKEBALL), ITEM_PREMIER_BALL);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_HELD_ITEM), ITEM_PERSIM_BERRY);
    EXPECT_EQ(GetMonAbility(&gParties[B_TRAINER_OPPONENT_A][0]), ABILITY_AFTERMATH);
    EXPECT_EQ(GetNature(&gParties[B_TRAINER_OPPONENT_A][0]), NATURE_HASTY);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_HP_IV), 28);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_ATK_IV), 28);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_DEF_IV), 28);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_SPEED_IV), 28);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_SPATK_IV), 28);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_SPDEF_IV), 28);
    GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_NICKNAME, name);
    EXPECT_EQ(StringCompare(name, COMPOUND_STRING("Mimic")), 0);
    GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_OT_NAME, name);
    EXPECT_EQ(StringCompare(name, COMPOUND_STRING("Kyle")), 0);
    // Existing trades retain their default ball when no override is declared.
    gSpecialVar_0x8005 = INGAME_TRADE_SEEDOT;
    CreateInGameTradePokemon();
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_POKEBALL), ITEM_POKE_BALL);
    gParties[B_TRAINER_PLAYER][0] = originalPlayer;
    gParties[B_TRAINER_OPPONENT_A][0] = originalEnemy;
    gSpecialVar_0x8004 = slot;
    gSpecialVar_0x8005 = trade;
}
