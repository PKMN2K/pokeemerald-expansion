#include "global.h"
#include "pokenav.h"
#include "test/test.h"

// These tests invoke the real movement code from pokenav_ribbons_summary.c,
// not the Python reference model or the offline PNG preview.
#define HGSS_NORMAL_LIMIT 25
#define HGSS_GIFT_LIMIT 7
#define HGSS_GIFT_START 28
#define HGSS_VIRTUAL_SLOT_COUNT 36
#define HGSS_GRID_COLUMNS 4
#define HGSS_PAGE_SIZE 8

TEST("HGSS ribbon D-pad navigation reaches every owned ribbon in all 207 layouts")
{
    u32 normalCount, giftCount;

    for (normalCount = 0; normalCount <= HGSS_NORMAL_LIMIT; normalCount++)
    {
        for (giftCount = 0; giftCount <= HGSS_GIFT_LIMIT; giftCount++)
        {
            bool8 visited[HGSS_VIRTUAL_SLOT_COUNT] = {0};
            u16 queue[HGSS_VIRTUAL_SLOT_COUNT];
            u16 initial, head = 0, tail = 0;
            u32 direction;

            if (normalCount + giftCount == 0)
                continue;

            initial = normalCount != 0 ? 0 : HGSS_GIFT_START;
            queue[tail++] = initial;
            visited[initial] = TRUE;

            while (head < tail)
            {
                u16 current = queue[head++];
                for (direction = HGSS_RIBBON_TEST_UP; direction <= HGSS_RIBBON_TEST_RIGHT; direction++)
                {
                    u16 next = Test_HgssRibbonNavigate(current, normalCount, giftCount, direction);
                    bool8 isNormal = next < normalCount;
                    bool8 isGift = next >= HGSS_GIFT_START && next < HGSS_GIFT_START + giftCount;

                    EXPECT_LT(next, HGSS_VIRTUAL_SLOT_COUNT);
                    EXPECT(isNormal || isGift);

                    if (!visited[next])
                    {
                        visited[next] = TRUE;
                        EXPECT_LT(tail, HGSS_VIRTUAL_SLOT_COUNT);
                        queue[tail++] = next;
                    }
                }
            }
            EXPECT_EQ(tail, normalCount + giftCount);
        }
    }
}

TEST("HGSS ribbon native cursor centers agree with all five paginated BG cells")
{
    u32 slot;
    for (slot = 0; slot < 40; slot++)
    {
        s16 x, y;
        u32 cell = slot % HGSS_PAGE_SIZE;
        Test_HgssRibbonScreenPosition(slot, &x, &y);

        EXPECT_EQ(x, 104 + 32 * (cell % HGSS_GRID_COLUMNS));
        EXPECT_EQ(y, 48 + 32 * (cell / HGSS_GRID_COLUMNS));
        EXPECT_GE(x - 16, 88);
        EXPECT_LE(x + 16, 216);
        EXPECT_GE(y - 16, 32);
        EXPECT_LE(y + 16, 96);
    }
}
