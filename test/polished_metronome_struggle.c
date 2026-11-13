#include "global.h"
#include "battle.h"
#include "polished_metronome.h"
#include "test/test.h"

TEST("Polished Metronome and Struggle use 244 distinct source candidates and twelve source exclusions")
{
    const u32 excluded[] = {0, 68, 194, 203, 118, 243, 182, 166, 214, 168, 144, 255};
    enum Move known[MAX_MON_MOVES] = {MOVE_NONE};
    bool8 seen[MOVES_COUNT] = {FALSE};
    u32 count = 0;
    for (u32 roll = 0; roll < 256; roll++)
    {
        enum Move move = PolishedMetronomeMoveFromRoll(roll, known);
        bool32 banned = FALSE;
        for (u32 n = 0; n < ARRAY_COUNT(excluded); n++)
            banned |= roll == excluded[n];
        EXPECT_EQ(move == MOVE_NONE, banned);
        if (move == MOVE_NONE)
            continue;
        EXPECT_LT(move, MOVES_COUNT);
        if (move >= MOVES_COUNT)
            continue;
        EXPECT(!seen[move]);
        EXPECT_GT(GetMovePP(move), 0);
        seen[move] = TRUE;
        count++;
    }
    EXPECT_EQ(count, 244);
    EXPECT_EQ(PolishedMetronomeMoveFromRoll(249, known), MOVE_BRICK_BREAK);
    EXPECT_EQ(PolishedMetronomeMoveFromRoll(135, known), MOVE_POLISHED_FRESH_SNACK);
    EXPECT_EQ(PolishedMetronomeMoveFromRoll(154, known), MOVE_POLISHED_FURY_STRIKES);
    EXPECT_EQ(PolishedMetronomeMoveFromRoll(234, known), MOVE_POLISHED_HEALING_LIGHT);
    EXPECT_EQ(PolishedMetronomeMoveFromRoll(256, known), MOVE_NONE);
    EXPECT_EQ(PolishedMetronomeMoveFromRoll(0xFFFFFFFF, known), MOVE_NONE);
}

TEST("Polished Metronome and Struggle exclude every known move in every current slot")
{
    enum Move known[MAX_MON_MOVES] = {MOVE_NONE};
    for (u32 roll = 0; roll < 256; roll++)
    {
        enum Move move = PolishedMetronomeMoveFromRoll(roll, known);
        if (move == MOVE_NONE)
            continue;
        for (u32 slot = 0; slot < MAX_MON_MOVES; slot++)
        {
            known[slot] = move;
            EXPECT_EQ(PolishedMetronomeMoveFromRoll(roll, known), MOVE_NONE);
            known[slot] = MOVE_NONE;
            EXPECT_EQ(PolishedMetronomeMoveFromRoll(roll, known), move);
        }
    }
    // Expansion-only IDs never alias a source byte or hide a source move.
    known[0] = MOVE_DYNAMAX_CANNON;
    known[1] = MOVE_DYNAMAX_CANNON;
    known[2] = MOVE_TERA_BLAST;
    known[3] = MOVE_ROCK_SMASH;
    EXPECT_EQ(PolishedMetronomeMoveFromRoll(249, known), MOVE_BRICK_BREAK);
}
