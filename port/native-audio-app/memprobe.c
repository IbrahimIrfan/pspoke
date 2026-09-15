/* TEST/DIAGNOSTIC instrumentation for the native Platinum port.
 *
 * Purpose: establish, with numbers rather than guesses, how much malloc heap the
 * renderer and the C library actually hold at each scene transition, and in
 * particular at the player name-entry keyboard, which is where a physical
 * PSP-3001 was reported to black-screen and power off.
 *
 * Everything here is event driven (scene transitions and new allocation high
 * water marks), never per frame: sceKernelGetSystemTimeLow and printf are
 * expensive under PPSSPP and would swamp the measurement.
 *
 * PSP_NATIVE_MEMLOG=1 also mirrors every line into a plain text file next to
 * the EBOOT on the memory stick, because printf goes nowhere on real hardware.
 */
#include <pspkernel.h>
#include <pspsysmem.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifndef PSP_NATIVE_MEMLOG
#define PSP_NATIVE_MEMLOG 0
#endif

/* Renderer-side accounting (defined in ../native-osk-render). */
/* Weak so the same probe can also be linked against the unmodified
 * native-next-render library for before/after comparison. */
extern void PSPNativeRenderMemStats(unsigned *count, unsigned *bytes, unsigned *high,
                                    unsigned *calls, unsigned *fails,
                                    unsigned *guHigh, unsigned *guCap) __attribute__((weak));
extern void PSPNativeG3CacheStats(unsigned *entries, unsigned *bytes, unsigned *highBytes,
                                  unsigned *highEntries, unsigned *retries,
                                  unsigned *oversize, unsigned *evictions) __attribute__((weak));
extern unsigned PSPNativeRenderFrameCount(void);

#if PSP_NATIVE_MEMLOG
/* Relative path: a PSP game's working directory is the folder holding the EBOOT. */
static const char *const kLogPath = "native-memlog.txt";
static int logStarted;
#endif

void PSPNativeMemLog(const char *fmt, ...)
{
    char line[400];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(line, sizeof(line), fmt, ap);
    va_end(ap);
    printf("%s\n", line);
#if PSP_NATIVE_MEMLOG
    {
        SceUID fd = sceIoOpen(kLogPath,
                              logStarted ? (PSP_O_WRONLY | PSP_O_APPEND)
                                         : (PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC),
                              0777);
        if (fd >= 0) {
            logStarted = 1;
            sceIoWrite(fd, line, strlen(line));
            sceIoWrite(fd, "\n", 1);
            sceIoClose(fd);
        }
    }
#endif
}

/* uordblks is the honest "bytes currently handed out by malloc": it covers the
 * renderer atlases, the renderer texture cache, the overlay data snapshots and
 * anything newlib itself allocated. arena is how far the heap has grown. */
extern unsigned PSPNativeStackHighWater(unsigned*);
extern void PSPNativeRomFSStats(unsigned*,unsigned*,unsigned*,unsigned*) __attribute__((weak));
void PSPNativeMemReport(const char *tag)
{
    struct mallinfo mi = mallinfo();
    unsigned aCount = 0, aBytes = 0, aHigh = 0, aCalls = 0, aFails = 0, guHigh = 0, guCap = 0;
    unsigned tEnt = 0, tBytes = 0, tHighB = 0, tHighE = 0, tRetry = 0, tOver = 0, tEvict = 0;
    if (PSPNativeRenderMemStats) {
        PSPNativeRenderMemStats(&aCount, &aBytes, &aHigh, &aCalls, &aFails, &guHigh, &guCap);
    }
    if (PSPNativeG3CacheStats) {
        PSPNativeG3CacheStats(&tEnt, &tBytes, &tHighB, &tHighE, &tRetry, &tOver, &tEvict);
    }
    unsigned stackTotal = 0, stackUsed = PSPNativeStackHighWater(&stackTotal);
    unsigned fsOpen = 0, fsHigh = 0, fsOpenFail = 0, fsReadFail = 0;
    if (PSPNativeRomFSStats) {
        PSPNativeRomFSStats(&fsOpen, &fsHigh, &fsOpenFail, &fsReadFail);
    }
    PSPNativeMemLog("[MEM] %s frame=%u stack=%u/%u heap_used=%d heap_free=%d heap_arena=%d "
                    "part_maxfree=%d part_totalfree=%d "
                    "atlas=%u/%u B (high %u B, calls %u, fails %u) "
                    "tex=%u ent/%u B (high %u B/%u ent, retries %u, oversize %u, evict %u) "
                    "gulist_high=%u/%u rom_open=%u/%u (open_fail %u, read_fail %u)",
                    tag, PSPNativeRenderFrameCount(), stackUsed, stackTotal,
                    mi.uordblks, mi.fordblks, mi.arena,
                    (int)sceKernelMaxFreeMemSize(), (int)sceKernelTotalFreeMemSize(),
                    aCount, aBytes, aHigh, aCalls, aFails,
                    tEnt, tBytes, tHighB, tHighE, tRetry, tOver, tEvict,
                    guHigh, guCap, fsOpen, fsHigh, fsOpenFail, fsReadFail);
}

/* ------------------------------------------------------------------ scenes */
/* Every scene in this game is entered through ApplicationManager_New with a
 * known const template, so wrapping it gives exact, cheap scene markers. */
#define T(name) extern const char name[];
T(gTitleScreenAppTemplate) T(gMainMenuAppTemplate) T(gOpeningCutsceneAppTemplate)
T(gGameStartNewSaveAppTemplate) T(gGameStartLoadSaveAppTemplate)
T(gGameStartRowanIntroAppTemplate) T(gRowanIntroAppTemplate)
T(gNamingScreenAppTemplate) T(gPokemonPartyAppTemplate)
T(gFieldMapTemplate) T(gFieldSystemNewGameTemplate)
#undef T

static const char *TemplateName(const void *t)
{
#define M(name) if ((const char *)t == name) return #name;
    M(gTitleScreenAppTemplate) M(gMainMenuAppTemplate) M(gOpeningCutsceneAppTemplate)
    M(gGameStartNewSaveAppTemplate) M(gGameStartLoadSaveAppTemplate)
    M(gGameStartRowanIntroAppTemplate) M(gRowanIntroAppTemplate)
    M(gNamingScreenAppTemplate) M(gPokemonPartyAppTemplate)
    M(gFieldMapTemplate) M(gFieldSystemNewGameTemplate)
#undef M
    return NULL;
}

extern void *__real_ApplicationManager_New(const void *tmpl, void *args, int heapID);

/* naming_osk.c: hands back the substitute naming application. */
extern const void *PSPNativeNamingOskTemplate(int heapID);

void *__wrap_ApplicationManager_New(const void *tmpl, void *args, int heapID)
{
    const char *name = TemplateName(tmpl);
    char tag[64];
    if (name) {
        snprintf(tag, sizeof(tag), "enter %s", name);
    } else {
        snprintf(tag, sizeof(tag), "enter app@%p", tmpl);
    }
    PSPNativeMemReport(tag);

    /* The one screen that kills real hardware. Every naming context in the
     * game - player, rival, nickname, PC box, Pal Pad, group - reaches it
     * through this call, so substituting here covers all of them at once. */
    if ((const char *)tmpl == gNamingScreenAppTemplate) {
#ifndef PSP_NATIVE_KEEP_DS_NAMING
        const void *sub = PSPNativeNamingOskTemplate(heapID);
        PSPNativeMemLog("[OSK] substituting PSP keyboard for gNamingScreenAppTemplate");
        return __real_ApplicationManager_New(sub, args, heapID);
#endif
    }

    {
        void *r = __real_ApplicationManager_New(tmpl, args, heapID);
        if (name && (const char *)tmpl == gNamingScreenAppTemplate) {
            PSPNativeMemReport("NAME-ENTRY constructed");
        }
        return r;
    }
}

/* New-high-water tracking: reported only when malloc usage climbs past the next
 * 128 KiB step, so it is an event log, not a per-frame log. */
void PSPNativeMemPoll(void)
{
    static int step;
    struct mallinfo mi = mallinfo();
    int next = mi.uordblks / (128 * 1024);
    if (next > step) {
        step = next;
        PSPNativeMemReport("heap high water");
    }
}

/* --------------------------------------------------------------- fatalities
 * Every fatal path in the port used to be printf + abort(). On a real PSP that
 * is a black screen and a power-off with no evidence at all. Route them through
 * the card log first, and record the last scene and the memory picture with
 * them, so the user can read native-memlog.txt off the memory stick afterwards. */
void PSPNativeFatal(const char *what)
{
    PSPNativeMemLog("[FATAL] %s", what);
    PSPNativeMemReport("at fatal");
    abort();
}
