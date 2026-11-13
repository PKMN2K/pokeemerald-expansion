#include "global.h"
#include "battle.h"
#include "pokemon.h"
#include "test/test.h"

TEST("Polished Hidden Power selects source data without replacing native tables")
{
    EXPECT_EQ(GetMovePower(MOVE_HIDDEN_POWER), B_POLISHED_HIDDEN_POWER ? 70 : 60);
    EXPECT_EQ(GetMoveType(MOVE_HIDDEN_POWER), B_POLISHED_HIDDEN_POWER ? TYPE_MYSTERY : TYPE_NORMAL);
    EXPECT_EQ(GetMoveCategory(MOVE_HIDDEN_POWER), DAMAGE_CATEGORY_SPECIAL);
    EXPECT_EQ(GetMoveAccuracy(MOVE_HIDDEN_POWER), 100);
    EXPECT_EQ(GetMovePower(MOVE_DAZZLING_GLEAM), 80);
}

TEST("Polished Hidden Power party and optional display typing cover all DV and IV parity patterns")
{
    struct Pokemon mon;
    static const u8 fields[] = {MON_DATA_HP_IV, MON_DATA_ATK_IV, MON_DATA_DEF_IV,
        MON_DATA_SPEED_IV, MON_DATA_SPATK_IV, MON_DATA_SPDEF_IV};
    static const enum Type types[] = {TYPE_FIGHTING, TYPE_FLYING, TYPE_POISON, TYPE_GROUND,
        TYPE_ROCK, TYPE_BUG, TYPE_GHOST, TYPE_STEEL, TYPE_FIRE, TYPE_WATER, TYPE_GRASS,
        TYPE_ELECTRIC, TYPE_PSYCHIC, TYPE_ICE, TYPE_DRAGON, TYPE_DARK, TYPE_FAIRY};
    CreateMonWithIVs(&mon, SPECIES_WOBBUFFET, 50, 0, OTID_STRUCT_PRESET(0), 0);
    for (u32 dvs = 0; dvs < 64; dvs++)
    {
        for (u32 low = 0; low < 64; low++)
        {
            for (u32 stat = 0; stat < ARRAY_COUNT(fields); stat++)
            {
                // Alternate high DV bits while independently varying every
                // source parity bit and every extra GBA IV low bit.
                u32 iv = ((dvs ^ low) & 1 ? 28 : 0) | (((dvs >> stat) & 1) << 1) | ((low >> stat) & 1);
                SetMonData(&mon, fields[stat], &iv);
            }
            enum Type expected = B_POLISHED_HIDDEN_POWER ? types[dvs == 63 ? 16 : dvs / 4] : types[15 * low / 63];
            EXPECT_EQ(GetDynamicMoveType(&mon, MOVE_HIDDEN_POWER, 0, ABILITY_NONE, HOLD_EFFECT_NONE, MON_OUTSIDE_BATTLE), expected);
            EXPECT_EQ(CheckDynamicMoveType(&mon, MOVE_HIDDEN_POWER, 0, MON_OUTSIDE_BATTLE), expected);
        }
    }
}

TEST("Polished Hidden Power display works without an active battle allocation")
{
    struct Pokemon mon;
    struct BattleStruct *saved = gBattleStruct;
    CreateMonWithIVs(&mon, SPECIES_WOBBUFFET, 50, 0, OTID_STRUCT_PRESET(0), 31);
    gBattleStruct = NULL;
    EXPECT_EQ(CheckDynamicMoveType(&mon, MOVE_HIDDEN_POWER, 0, MON_OUTSIDE_BATTLE), B_POLISHED_HIDDEN_POWER ? TYPE_FAIRY : TYPE_DARK);
    EXPECT_EQ(CheckDynamicMoveType(&mon, MOVE_TACKLE, 0, MON_OUTSIDE_BATTLE), TYPE_NORMAL);
    gBattleStruct = saved;
}
