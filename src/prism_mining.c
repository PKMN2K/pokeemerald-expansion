#include "global.h"
#include "random.h"
#include "constants/items.h"

// Pokémon Prism's level-0 mining window is the first 100 entries of
// MiningPickItemTable. Prism-only rewards remain ITEM_NONE until they
// receive explicit Expansion equivalents; no substitute items are invented.
static const u16 sPrismMiningLevel0Rewards[100] =
{
    ITEM_NONE, // 00
    ITEM_NONE, // 01
    ITEM_NONE, // 02
    ITEM_NONE, // 03
    ITEM_NONE, // 04
    ITEM_NONE, // 05
    ITEM_NONE, // 06
    ITEM_NONE, // 07
    ITEM_NONE, // 08
    ITEM_NONE, // 09
    ITEM_NONE, // 10
    ITEM_NONE, // 11
    ITEM_NONE, // 12
    ITEM_NONE, // 13
    ITEM_NONE, // 14
    ITEM_NONE, // 15
    ITEM_NONE, // 16
    ITEM_NONE, // 17
    ITEM_NONE, // 18
    ITEM_NONE, // 19
    ITEM_NONE, // 20
    ITEM_NONE, // 21
    ITEM_NONE, // 22
    ITEM_NONE, // 23
    ITEM_NONE, // 24
    ITEM_NONE, // 25
    ITEM_NONE, // 26
    ITEM_NONE, // 27
    ITEM_NONE, // 28
    ITEM_NONE, // 29
    ITEM_NONE, // 30
    ITEM_NONE, // 31
    ITEM_NONE, // 32
    ITEM_NONE, // 33
    ITEM_NONE, // 34
    ITEM_NONE, // 35
    ITEM_HEART_SCALE, // 36
    ITEM_HEART_SCALE, // 37
    ITEM_HEART_SCALE, // 38
    ITEM_HEART_SCALE, // 39
    ITEM_HEART_SCALE, // 40
    ITEM_HEART_SCALE, // 41
    ITEM_HEART_SCALE, // 42
    ITEM_HEART_SCALE, // 43
    ITEM_HEART_SCALE, // 44
    ITEM_HEART_SCALE, // 45
    ITEM_NONE, // 46
    ITEM_NONE, // 47
    ITEM_NONE, // 48
    ITEM_NONE, // 49
    ITEM_NONE, // 50
    ITEM_NONE, // 51
    ITEM_NONE, // 52
    ITEM_NONE, // 53
    ITEM_NONE, // 54
    ITEM_NONE, // 55
    ITEM_NONE, // 56
    ITEM_NONE, // 57
    ITEM_NONE, // 58
    ITEM_NONE, // 59
    ITEM_NONE, // 60
    ITEM_NONE, // 61
    ITEM_NONE, // 62
    ITEM_NONE, // 63
    ITEM_NONE, // 64
    ITEM_NONE, // 65
    ITEM_NONE, // 66
    ITEM_NONE, // 67
    ITEM_NONE, // 68
    ITEM_NONE, // 69
    ITEM_NONE, // 70
    ITEM_NONE, // 71
    ITEM_HARD_STONE, // 72
    ITEM_REVIVE, // 73
    ITEM_NONE, // 74
    ITEM_NONE, // 75
    ITEM_EVERSTONE, // 76
    ITEM_EVERSTONE, // 77
    ITEM_EVERSTONE, // 78
    ITEM_EVERSTONE, // 79
    ITEM_EVERSTONE, // 80
    ITEM_HARD_STONE, // 81
    ITEM_NONE, // 82
    ITEM_KINGS_ROCK, // 83
    ITEM_NONE, // 84
    ITEM_REVIVE, // 85
    ITEM_NONE, // 86
    ITEM_NONE, // 87
    ITEM_NONE, // 88
    ITEM_NONE, // 89
    ITEM_NONE, // 90
    ITEM_HEART_SCALE, // 91
    ITEM_HEART_SCALE, // 92
    ITEM_HEART_SCALE, // 93
    ITEM_HEART_SCALE, // 94
    ITEM_NONE, // 95
    ITEM_LEAF_STONE, // 96
    ITEM_FIRE_STONE, // 97
    ITEM_WATER_STONE, // 98
    ITEM_THUNDER_STONE, // 99
};

u16 PrismMiningRollBaseReward(void)
{
    return RandomElement(RNG_NONE, sPrismMiningLevel0Rewards);
}
