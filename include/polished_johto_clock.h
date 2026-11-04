#ifndef GUARD_POLISHED_JOHTO_CLOCK_H
#define GUARD_POLISHED_JOHTO_CLOCK_H

// Sunday = 0. A Johto weekday choice does not rewind native daily timers.
u32 PolishedJohtoGetWeekday(void);
bool32 PolishedJohtoSetWeekday(u32 weekday);
void PolishedJohtoBufferChosenWeekday(void);
void PolishedJohtoCommitWeekday(void);
void PolishedJohtoBufferClock(void);

#endif
