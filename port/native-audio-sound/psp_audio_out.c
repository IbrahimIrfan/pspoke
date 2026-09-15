/* psp_audio_out.c - speaker output for the native Pokemon Platinum port.
 *
 * The DS sound engine in backend.c mixes interleaved s16 stereo at a nominal
 * 16 kHz (one sample every PSP_AUDIO_STEP_CYCLES = 1048 ticks of the DS sound
 * clock, 16756991/1048 = 15989.5 Hz). That is handed to sceAudio through the
 * sample rate converter channel reserved at 16000 Hz, so the pitch error is
 * 15989.5/16000 = -0.066%, about a hundredth of a semitone, and the SRC does
 * the 16000 -> 44100 conversion in the audio hardware. Nothing is resampled
 * in software.
 *
 * Everything here is failure tolerant by design: if the channel cannot be
 * reserved, or the thread cannot be created, the game keeps running silently
 * and the reason goes into the card log (printf is invisible on hardware).
 *
 * Buffers are static, not malloc'd: the port's heap is measured and tight, and
 * a fixed 32 KB of BSS is easier to reason about than a late allocation.
 */
#include <pspkernel.h>
#include <pspaudio.h>
#include <string.h>
#include <stdio.h>

#ifndef PSP_AUDIO_OUT_FREQ
#define PSP_AUDIO_OUT_FREQ 16000
#endif
#define OUT_FREQ      PSP_AUDIO_OUT_FREQ
#define OUT_CHUNK     256               /* stereo frames per sceAudio call: 16 ms at 16 kHz */
#define RING_FRAMES   8192              /* power of two: 256 ms of slack */
#define RING_MASK     (RING_FRAMES - 1)

/* Steady-state fill the producer aims for: ~2 frames of video at 30 fps. */
#define TARGET_FILL   1024              /* 64 ms: two video frames of slack */

extern void PSPNativeMemLog(const char *fmt, ...);
extern int PSPNativeSoundSilent;   /* backend.c: 1 = sound off */

static short s_ring[RING_FRAMES * 2];
static short __attribute__((aligned(64))) s_chunk[OUT_CHUNK * 2];
static volatile unsigned s_write, s_read;
static SceUID s_thread = -1;
static volatile int s_running;
static int s_ready;
static unsigned s_underruns, s_droppedFrames, s_writtenFrames;

unsigned PSPNativeAudioOutFill(void)
{
    return (s_write - s_read) & RING_MASK;
}

int PSPNativeAudioOutReady(void) { return s_ready; }

static int AudioThread(SceSize args, void *argp)
{
    (void)args; (void)argp;
    while (s_running) {
        /* Sound off (the default; SELECT+SQUARE toggles): nothing is mixed, so do not
         * push silence at 16 ms cadence - that keeps this thread and the SRC channel
         * busy and cost ~2 fps on the PSP-3001 (card log: underruns climbing with
         * written=0). Park until the toggle wakes us. */
        if (PSPNativeSoundSilent) {
            s_read = s_write;           /* drop anything stale so the restart is clean */
            sceKernelDelayThread(50 * 1000);
            continue;
        }
        unsigned read = s_read;
        unsigned avail = (s_write - read) & RING_MASK;
        unsigned take = avail < OUT_CHUNK ? avail : OUT_CHUNK;
        if (take < OUT_CHUNK) s_underruns++;
        if (take) {
            unsigned first = RING_FRAMES - (read & RING_MASK);
            if (first > take) first = take;
            memcpy(s_chunk, s_ring + (read & RING_MASK) * 2, first * 4);
            if (take > first) memcpy(s_chunk + first * 2, s_ring, (take - first) * 4);
            s_read = (read + take) & RING_MASK;
        }
        if (take < OUT_CHUNK) memset(s_chunk + take * 2, 0, (OUT_CHUNK - take) * 4);
        sceAudioSRCOutputBlocking(PSP_AUDIO_VOLUME_MAX, s_chunk);
    }
    return 0;
}

int PSPNativeAudioOutInit(void)
{
    int rc;
    if (s_ready) return 0;
#if defined(PSP_NATIVE_AUDIO_NOOUT) || defined(PSP_NATIVE_AUDIO_OFF)
    return -1;      /* measurement build: mix but never touch sceAudio */
#endif
    rc = sceAudioSRCChReserve(OUT_CHUNK, OUT_FREQ, 2);
    if (rc < 0) {
        PSPNativeMemLog("[AUDIO] sceAudioSRCChReserve failed %d - running silent", rc);
        return -1;
    }
    s_running = 1;
    s_thread = sceKernelCreateThread("native_audio_out", AudioThread, 0x12, 0x4000,
                                     THREAD_ATTR_USER, NULL);
    if (s_thread < 0) {
        PSPNativeMemLog("[AUDIO] sceKernelCreateThread failed %d - running silent",
                        (int)s_thread);
        s_running = 0;
        sceAudioSRCChRelease();
        return -1;
    }
    sceKernelStartThread(s_thread, 0, NULL);
    s_ready = 1;
    PSPNativeMemLog("[AUDIO] output ready: SRC %d Hz, %d-frame chunks, %d-frame ring",
                    OUT_FREQ, OUT_CHUNK, RING_FRAMES);
    return 0;
}

/* Producer side. Never blocks; drops on overrun so a stalled consumer can never
 * stall the game loop. */
void PSPNativeAudioOutWrite(const short *interleaved, unsigned frames)
{
    unsigned write = s_write;
    unsigned space = RING_MASK - ((write - s_read) & RING_MASK);
    unsigned first;
    if (frames > space) {
        s_droppedFrames += frames - space;
        frames = space;
    }
    if (!frames) return;
    first = RING_FRAMES - write;
    if (first > frames) first = frames;
    memcpy(s_ring + write * 2, interleaved, first * 4);
    if (frames > first) memcpy(s_ring, interleaved + first * 2, (frames - first) * 4);
    s_write = (write + frames) & RING_MASK;
    s_writtenFrames += frames;
}

void PSPNativeAudioOutStats(unsigned *fill, unsigned *underruns, unsigned *dropped,
                            unsigned *written)
{
    if (fill) *fill = PSPNativeAudioOutFill();
    if (underruns) *underruns = s_underruns;
    if (dropped) *dropped = s_droppedFrames;
    if (written) *written = s_writtenFrames;
}

unsigned PSPNativeAudioOutTargetFill(void) { return TARGET_FILL; }
unsigned PSPNativeAudioOutCapacity(void) { return RING_FRAMES; }

void PSPNativeAudioOutShutdown(void)
{
    if (!s_ready) return;
    s_ready = 0;
    s_running = 0;
    if (s_thread >= 0) {
        sceKernelWaitThreadEnd(s_thread, NULL);
        sceKernelDeleteThread(s_thread);
        s_thread = -1;
    }
    sceAudioSRCChRelease();
}

/* ---- PPSSPP verification dump ------------------------------------------------
 * PPSSPP headless has no audio device, so the only way to judge the mix is to
 * write it out. Built only with -DPSP_NATIVE_AUDIO_WAV; the shipping EBOOT has
 * none of this. Raw interleaved s16 LE stereo at 32000 Hz, buffered and
 * appended so the cost does not distort the audio_us measurement too badly.
 */
#ifdef PSP_NATIVE_AUDIO_WAV
#define DUMP_FRAMES 4096
static short s_dump[DUMP_FRAMES * 2];
static unsigned s_dumpUsed, s_dumpStarted, s_dumpTotal;
static const char *const kDumpPath = "native-audio.raw";

static void DumpFlush(void)
{
    SceUID fd;
    if (!s_dumpUsed) return;
    fd = sceIoOpen(kDumpPath,
                   s_dumpStarted ? (PSP_O_WRONLY | PSP_O_APPEND)
                                 : (PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC), 0777);
    if (fd >= 0) {
        s_dumpStarted = 1;
        sceIoWrite(fd, s_dump, s_dumpUsed * 4);
        sceIoClose(fd);
        s_dumpTotal += s_dumpUsed;
    }
    s_dumpUsed = 0;
}

void PSPNativeAudioDump(const short *interleaved, unsigned frames)
{
    while (frames) {
        unsigned room = DUMP_FRAMES - s_dumpUsed;
        unsigned take = frames < room ? frames : room;
        memcpy(s_dump + s_dumpUsed * 2, interleaved, take * 4);
        s_dumpUsed += take;
        interleaved += take * 2;
        frames -= take;
        if (s_dumpUsed == DUMP_FRAMES) DumpFlush();
    }
}

unsigned PSPNativeAudioDumpFrames(void) { return s_dumpTotal + s_dumpUsed; }
void PSPNativeAudioDumpFlush(void) { DumpFlush(); }
#endif
