#include "global.h"
#include "event_data.h"
#include "polished_johto_clock.h"
#include "rtc.h"
#include "constants/vars.h"
#include "test/test.h"

TEST("New Bark weekday selection preserves the native clock and rejects invalid choices")
{
    u32 weekday;
    u32 originalDay = GetDayOfWeek();
    u16 originalOffset = VarGet(VAR_POLISHED_WEEKDAY_OFFSET);
    struct Time originalClock = gSaveBlock2Ptr->localTimeOffset;

    for (weekday = WEEKDAY_SUN; weekday < WEEKDAY_COUNT; weekday++)
    {
        EXPECT(PolishedJohtoSetWeekday(weekday));
        EXPECT_EQ(PolishedJohtoGetWeekday(), weekday);
        EXPECT_EQ(GetDayOfWeek(), originalDay);
        EXPECT_EQ(memcmp(&gSaveBlock2Ptr->localTimeOffset, &originalClock, sizeof(originalClock)), 0);
    }
    EXPECT(!PolishedJohtoSetWeekday(WEEKDAY_COUNT));
    EXPECT(!PolishedJohtoSetWeekday(0xFFFFFFFF));
    EXPECT_EQ(PolishedJohtoGetWeekday(), WEEKDAY_SAT);
    VarSet(VAR_POLISHED_WEEKDAY_OFFSET, originalOffset);
}

TEST("New Bark chosen weekday advances across midnight and wraps after Saturday")
{
    u32 day;
    u16 originalOffset = VarGet(VAR_POLISHED_WEEKDAY_OFFSET);
    struct Time originalClock = gSaveBlock2Ptr->localTimeOffset;
    struct Time base;

    RtcCalcLocalTime();
    base = gLocalTime;
    EXPECT(PolishedJohtoSetWeekday(WEEKDAY_SAT));
    for (day = 1; day <= 8; day++)
    {
        RtcCalcLocalTimeOffset(base.days + day, base.hours, base.minutes, base.seconds);
        EXPECT_EQ(PolishedJohtoGetWeekday(), (WEEKDAY_SAT + day) % WEEKDAY_COUNT);
    }
    gSaveBlock2Ptr->localTimeOffset = originalClock;
    VarSet(VAR_POLISHED_WEEKDAY_OFFSET, originalOffset);
    RtcCalcLocalTime();
}
