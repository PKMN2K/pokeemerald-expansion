// Minimal input/screenshot driver for the real GBA ROM. Requires libmgba-dev.
// Does not patch ROM or game RAM. State snapshots only speed up independent probes.
#include <mgba/core/core.h>
#include <mgba/core/config.h>
#include <mgba/core/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static color_t pixels[240 * 160];

static void LogFatal(struct mLogger *logger, int category, enum mLogLevel level,
                     const char *format, va_list args)
{
    (void)logger;
    (void)category;
    if (level == mLOG_FATAL)
    {
        vfprintf(stderr, format, args);
        fputc('\n', stderr);
    }
}

int main(int argc, char **argv)
{
    struct mLogger logger = {.log = LogFatal};
    struct mCore *core;
    char command[1024], path[800];
    unsigned a, b;
    if (argc != 2)
        return 1;
    mLogSetDefaultLogger(&logger);
    core = mCoreFind(argv[1]);
    if (!core || !core->init(core))
        return 2;
    mCoreInitConfig(core, "polished-johto-test");
    core->setVideoBuffer(core, pixels, 240);
    if (!mCoreLoadFile(core, argv[1]))
        return 3;
    core->reset(core);
    while (fgets(command, sizeof(command), stdin))
    {
        if (sscanf(command, "frames %u %u", &a, &b) == 2 && a <= 10000)
        {
            core->setKeys(core, b);
            for (unsigned i = 0; i < a; i++)
                core->runFrame(core);
            core->setKeys(core, 0);
            printf("frames %u\n", core->frameCounter(core));
        }
        else if (sscanf(command, "rtc %u", &a) == 1)
        {
            core->rtc.override = RTC_FIXED;
            core->rtc.value = (int64_t)a * 1000;
            puts("rtc");
        }
        else if (sscanf(command, "read %x %u", &a, &b) == 2 && b <= 4096)
        {
            printf("read");
            for (unsigned i = 0; i < b; i++)
                printf(" %02x", core->busRead8(core, a + i));
            puts("");
        }
        else if (sscanf(command, "shot %799s", path) == 1)
        {
            FILE *file = fopen(path, "wb");
            if (!file)
                return 4;
            fprintf(file, "P6\n240 160\n255\n");
            for (unsigned i = 0; i < 240 * 160; i++)
            {
                unsigned v = pixels[i];
                fputc(v & 255, file);
                fputc((v >> 8) & 255, file);
                fputc((v >> 16) & 255, file);
            }
            fclose(file);
            puts("shot");
        }
        else if (sscanf(command, "save %799s", path) == 1)
        {
            void *state = malloc(core->stateSize(core));
            FILE *file = fopen(path, "wb");
            if (!state || !file || !core->saveState(core, state))
                return 5;
            if (fwrite(state, 1, core->stateSize(core), file) != core->stateSize(core))
                return 6;
            fclose(file);
            free(state);
            puts("save");
        }
        else if (sscanf(command, "load %799s", path) == 1)
        {
            void *state = malloc(core->stateSize(core));
            FILE *file = fopen(path, "rb");
            if (!state || !file || fread(state, 1, core->stateSize(core), file) != core->stateSize(core))
                return 7;
            fclose(file);
            if (!core->loadState(core, state))
                return 8;
            free(state);
            puts("load");
        }
        else if (!strcmp(command, "reset\n"))
        {
            // A console reset preserves cartridge save data; no RAM injection.
            core->reset(core);
            puts("reset");
        }
        else if (!strncmp(command, "quit", 4))
            break;
        else
            return 9;
        fflush(stdout);
    }
    core->deinit(core);
    return 0;
}
