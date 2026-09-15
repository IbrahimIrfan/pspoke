/* Native replacement for SoulSilver src/timer3.c, not an SDK timer override. */
#include <nitro.h>
#include <pspkernel.h>

#if defined(SS_TIMER3_CLOCK_PROOF)
extern u64 SS_Timer3ProofClockUs(void);
#define TIMER3_NOW_US() SS_Timer3ProofClockUs()
#else
#define TIMER3_NOW_US() sceKernelGetSystemTimeWide()
#endif

static u64 timer3StartUs;
/* Kept for source ABI compatibility. Neither has users outside timer3.c. */
vu64 _021D2214;
int _021D2210;

void Init_Timer3(void)
{
    timer3StartUs = TIMER3_NOW_US();
    _021D2214 = 0;
    _021D2210 = 0;
}

u64 sub_02025488(void)
{
    u64 elapsed = TIMER3_NOW_US() - timer3StartUs;
    /* The DS timer uses system clock / 64, not microseconds. Splitting
     * the product gives exact floor division across the full u64 input
     * range, without overflowing elapsed * OS_SYSTEM_CLOCK. */
    u64 ticks = (elapsed / 64000000u) * OS_SYSTEM_CLOCK
        + ((elapsed % 64000000u) * OS_SYSTEM_CLOCK) / 64000000u;
    _021D2214 = ticks >> 16;
    return ticks;
}

u64 sub_020254FC(void)
{
    return sub_02025488();
}

u64 sub_02025504(u64 ticks)
{
    /* Preserve the game's unsigned multiplication and whole-second units. */
    return (ticks * 64) / OS_SYSTEM_CLOCK;
}
