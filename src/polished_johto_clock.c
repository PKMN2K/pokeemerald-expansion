#include "global.h"
#include "event_data.h"
#include "polished_johto_clock.h"
#include "rtc.h"
#include "string_util.h"
#include "strings.h"
#include "constants/vars.h"

static const u8 *const sWeekdays[] = {
    COMPOUND_STRING("Sunday"), COMPOUND_STRING("Monday"),
    COMPOUND_STRING("Tuesday"), COMPOUND_STRING("Wednesday"),
    COMPOUND_STRING("Thursday"), COMPOUND_STRING("Friday"),
    COMPOUND_STRING("Saturday"),
};

u32 PolishedJohtoGetWeekday(void)
{
    return (GetDayOfWeek() + VarGet(VAR_POLISHED_WEEKDAY_OFFSET)) % ARRAY_COUNT(sWeekdays);
}

bool32 PolishedJohtoSetWeekday(u32 weekday)
{
    if (weekday >= ARRAY_COUNT(sWeekdays))
        return FALSE;
    VarSet(VAR_POLISHED_WEEKDAY_OFFSET, (weekday + ARRAY_COUNT(sWeekdays) - GetDayOfWeek()) % ARRAY_COUNT(sWeekdays));
    return TRUE;
}

void PolishedJohtoBufferChosenWeekday(void)
{
    if (gSpecialVar_0x8004 < ARRAY_COUNT(sWeekdays))
        StringCopy(gStringVar1, sWeekdays[gSpecialVar_0x8004]);
}

void PolishedJohtoCommitWeekday(void)
{
    gSpecialVar_Result = PolishedJohtoSetWeekday(gSpecialVar_0x8004);
}

void PolishedJohtoBufferClock(void)
{
    RtcCalcLocalTime();
    FormatDecimalTimeWithoutSeconds(gStringVar1, gLocalTime.hours, gLocalTime.minutes, TRUE);
}
