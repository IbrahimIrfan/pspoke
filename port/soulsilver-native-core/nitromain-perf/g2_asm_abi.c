/* g2_asm_abi.c - DS-ABI adapters for the G2x_* register-block SDK entries.
 *
 * Under SDK_PORT these take `u64 addr` (two argument registers), so a call
 * translated from DS assembly - `u32 addr` in r0, the rest shifted by one - lands
 * every argument in the wrong place (Trainer Card: G2x_SetBGyAffine_ read its
 * matrix from centerX=0x80 -> fault at 0x84). The C game code must keep the
 * port's ABI, so these are not --wrap'd: maploader/call-retargets.json rewrites
 * every translated `bl G2x_*_` to the SSAsm_ twin below, which has the DS ABI.
 *
 * A DS I/O address (0x04xxxxxx) cannot be written through: the real routine
 * computes into a scratch block and the registers are forwarded through the
 * I/O router (ds_ioreg.c). Native pointers pass straight through. */
#include <nitro.h>
#include <stdint.h>

extern void ss_io_write16(uint32_t address, uint32_t value);
extern void ss_io_write32(uint32_t address, uint32_t value);
extern void G2x_SetBGyAffine_(u64 addr, const MtxFx22 *mtx, int centerX, int centerY, int x1, int y1);
extern void G2x_SetBlendAlpha_(u64 addr, int plane1, int plane2, int ev1, int ev2);
extern void G2x_SetBlendBrightness_(u64 addr, int plane, int brightness);
extern void G2x_SetBlendBrightnessExt_(u64 addr, int plane1, int plane2, int ev1, int ev2, int brightness);
extern void G2x_ChangeBlendBrightness_(u64 addr, int brightness);

#define IS_DS_IO(a) (((a) & 0xFF000000u) == 0x04000000u)

void SSAsm_G2x_SetBGyAffine_(u32 addr, const MtxFx22 *mtx, int centerX, int centerY, int x1, int y1)
{
    u32 tmp[4] __attribute__((aligned(4)));  /* PA PB PC PD | X | Y */
    const u16 *h = (const u16 *)tmp;
    if (!IS_DS_IO(addr)) { G2x_SetBGyAffine_((u64)(uintptr_t)addr, mtx, centerX, centerY, x1, y1); return; }
    G2x_SetBGyAffine_((u64)(uintptr_t)tmp, mtx, centerX, centerY, x1, y1);
    ss_io_write16(addr + 0, h[0]); ss_io_write16(addr + 2, h[1]);
    ss_io_write16(addr + 4, h[2]); ss_io_write16(addr + 6, h[3]);
    ss_io_write32(addr + 8, tmp[2]); ss_io_write32(addr + 12, tmp[3]);
}

/* BLDCNT @+0, BLDALPHA @+2, BLDY @+4 */
static void RouteBlend(u32 addr, const u16 *b) { ss_io_write16(addr, b[0]); ss_io_write16(addr + 2, b[1]); ss_io_write16(addr + 4, b[2]); }

void SSAsm_G2x_SetBlendAlpha_(u32 addr, int plane1, int plane2, int ev1, int ev2)
{
    u16 tmp[4] __attribute__((aligned(4))) = {0, 0, 0, 0};
    if (!IS_DS_IO(addr)) { G2x_SetBlendAlpha_((u64)(uintptr_t)addr, plane1, plane2, ev1, ev2); return; }
    G2x_SetBlendAlpha_((u64)(uintptr_t)tmp, plane1, plane2, ev1, ev2); RouteBlend(addr, tmp);
}
void SSAsm_G2x_SetBlendBrightness_(u32 addr, int plane, int brightness)
{
    u16 tmp[4] __attribute__((aligned(4))) = {0, 0, 0, 0};
    if (!IS_DS_IO(addr)) { G2x_SetBlendBrightness_((u64)(uintptr_t)addr, plane, brightness); return; }
    G2x_SetBlendBrightness_((u64)(uintptr_t)tmp, plane, brightness); RouteBlend(addr, tmp);
}
void SSAsm_G2x_SetBlendBrightnessExt_(u32 addr, int plane1, int plane2, int ev1, int ev2, int brightness)
{
    u16 tmp[4] __attribute__((aligned(4))) = {0, 0, 0, 0};
    if (!IS_DS_IO(addr)) { G2x_SetBlendBrightnessExt_((u64)(uintptr_t)addr, plane1, plane2, ev1, ev2, brightness); return; }
    G2x_SetBlendBrightnessExt_((u64)(uintptr_t)tmp, plane1, plane2, ev1, ev2, brightness); RouteBlend(addr, tmp);
}
void SSAsm_G2x_ChangeBlendBrightness_(u32 addr, int brightness)
{
    u16 tmp[4] __attribute__((aligned(4))) = {0, 0, 0, 0};
    if (!IS_DS_IO(addr)) { G2x_ChangeBlendBrightness_((u64)(uintptr_t)addr, brightness); return; }
    /* ChangeBlendBrightness only rewrites BLDY relative to the current BLDCNT; feed it the live value */
    extern uint32_t ss_io_read16(uint32_t address);
    tmp[0] = (u16)ss_io_read16(addr); tmp[2] = (u16)ss_io_read16(addr + 4);
    G2x_ChangeBlendBrightness_((u64)(uintptr_t)tmp, brightness); RouteBlend(addr, tmp);
}
