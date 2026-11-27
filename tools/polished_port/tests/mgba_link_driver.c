// Two production-linked GBA ROMs on mGBA's multiplayer serial cable.
// The host schedules CPU slices, reads RAM and sends ordinary controller input.
#include <mgba/flags.h>
#include <mgba/core/core.h>
#include <mgba/core/config.h>
#include <mgba/core/log.h>
#include <mgba/internal/gba/gba.h>
#include <mgba/internal/gba/sio/lockstep.h>
#include <stdio.h>
#include <string.h>

static struct mCore *cores[2];
static struct GBASIOLockstep cable;
static struct GBASIOLockstepNode nodes[2];
static color_t pixels[2][240 * 160];
static bool awake[2] = {true, true};
static unsigned waitingFor;
static int32_t credit[2];

static bool Signal(struct mLockstep *state, unsigned mask)
{
    (void)state;
    waitingFor &= ~mask;
    if (waitingFor || awake[0])
        return false;
    awake[0] = true;
    return true;
}

static bool Wait(struct mLockstep *state, unsigned mask)
{
    (void)state;
    bool wasAwake = awake[0];
    waitingFor |= mask;
    awake[0] = false;
    return wasAwake;
}

static void AddCycles(struct mLockstep *state, int id, int32_t count)
{
    (void)state;
    credit[1] += count;
    if (id == 0)
    {
        if (!awake[1])
            nodes[1].nextEvent += credit[1];
        awake[1] = true;
    }
}

static int32_t UseCycles(struct mLockstep *state, int id, int32_t count)
{
    (void)state;
    credit[id] -= count;
    if (credit[id] <= 0)
        awake[id] = false;
    return credit[id];
}

static int32_t UnusedCycles(struct mLockstep *state, int id)
{
    (void)state;
    return credit[id];
}

static void Unload(struct mLockstep *state, int id)
{
    if (id == 0)
        AddCycles(state, 0, nodes[0].eventDiff);
    else
    {
        credit[id] = 0;
        Signal(state, 1u << id);
    }
}

static void LogFatal(struct mLogger *logger, int category, enum mLogLevel level, const char *format, va_list args)
{
    (void)logger;
    (void)category;
    if (level == mLOG_FATAL)
    {
        vfprintf(stderr, format, args);
        fputc('\n', stderr);
    }
}

static bool Frames(unsigned count, unsigned firstKeys, unsigned secondKeys)
{
    unsigned target[2];
    for (int i = 0; i < 2; i++)
        target[i] = cores[i]->frameCounter(cores[i]) + count;
    cores[0]->setKeys(cores[0], firstKeys);
    cores[1]->setKeys(cores[1], secondKeys);
    unsigned slices = 0;
    for (;;)
    {
        unsigned first = cores[0]->frameCounter(cores[0]);
        unsigned second = cores[1]->frameCounter(cores[1]);
        if (first >= target[0] && second >= target[1])
            return true;
        if (++slices > 100000000 || (!awake[0] && !awake[1]))
            return false;
        unsigned next;
        if (!awake[0] || !awake[1])
            next = !awake[0];
        else if (first != second)
            next = first > second;
        else
            next = slices % 2;
        cores[next]->runLoop(cores[next]);
    }
}

int main(int argc, char **argv)
{
    // mGBA's public header changes the serial structs with NDEBUG. Check the
    // installed library's initialization of the debug field before attaching.
    if (argc == 2 && !strcmp(argv[1], "--abi"))
    {
        struct { struct mLockstep state; unsigned padding[2]; } probe;
        memset(&probe, 0xa5, sizeof(probe));
        mLockstepInit(&probe.state);
        size_t end = offsetof(struct mLockstep, context) + sizeof(probe.state.context);
        bool debug = *((unsigned char *)&probe + end) == 0;
#ifdef NDEBUG
        return debug;
#else
        return !debug;
#endif
    }
    if (argc != 3)
        return 1;
    struct mLogger logger = {.log = LogFatal};
    mLogSetDefaultLogger(&logger);
    mLockstepInit(&cable.d);
    GBASIOLockstepInit(&cable);
    cable.d.signal = Signal;
    cable.d.wait = Wait;
    cable.d.addCycles = AddCycles;
    cable.d.useCycles = UseCycles;
    cable.d.unusedCycles = UnusedCycles;
    cable.d.unload = Unload;
    for (int i = 0; i < 2; i++)
    {
        struct mCore *core = cores[i] = mCoreFind(argv[i + 1]);
        if (!core || !core->init(core))
            return 2;
        mCoreInitConfig(core, "polished-link-test");
        core->setVideoBuffer(core, pixels[i], 240);
        if (!mCoreLoadFile(core, argv[i + 1]))
            return 3;
        core->rtc.override = RTC_FIXED;
        core->rtc.value = 1793786400000LL;
        core->reset(core);
        GBASIOLockstepNodeCreate(&nodes[i]);
        if (!GBASIOLockstepAttachNode(&cable, &nodes[i]))
            return 4;
        struct GBA *gba = core->board;
        GBASIOSetDriver(&gba->sio, &nodes[i].d, SIO_MULTI);
        GBASIOSetDriver(&gba->sio, &nodes[i].d, SIO_NORMAL_32);
    }
    char line[128];
    unsigned count, first, second;
    while (fgets(line, sizeof(line), stdin))
    {
        if (sscanf(line, "frames %u %u %u", &count, &first, &second) == 3 && count <= 10000)
        {
            if (!Frames(count, first, second))
                return 5;
            puts("frames");
        }
        else if (sscanf(line, "read %u %x %u", &first, &second, &count) == 3 && first < 2 && count <= 100)
        {
            printf("read");
            for (unsigned i = 0; i < count; i++)
                printf(" %02x", cores[first]->busRead8(cores[first], second + i));
            putchar('\n');
        }
        else if (!strcmp(line, "quit\n"))
            return 0;
        else
            return 6;
        fflush(stdout);
    }
    return 0;
}
