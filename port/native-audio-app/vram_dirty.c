/* opt-tex: DS texture/palette VRAM write tracking for the renderer's texture cache (OPT_TEX_DIRTY).
   Only six linked objects reference s_HW_LCDC_VRAM, so only they can form a pointer into it:
   gx_load3d, gx_load2d (libsdk-filtered.a), system, encounter_effect, motion_blur, ov100_021D4E04
   (libplatinum-overlays.a). opt-tex-src/rebuild_opttex.py redirects every memory-writing call those
   objects make (memset, MIi_CpuCopy16/32, MI_DmaCopy16/32, MI_DmaCopy32Async) to the shims below.
   Each shim stamps the touched 4 KB pages with a new generation before and after the write.
   The renderer skips the byte compare of a cached texture when none of its source pages (resolved
   through the current VRAMCNT mapping) carry a generation newer than the entry's last validation. */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
extern unsigned char s_HW_LCDC_VRAM[];
enum { kVramBytes = 0xA4000, kVramPages = kVramBytes >> 12 };
unsigned PSPNativeVramGen = 1, PSPNativeVramPageGen[kVramPages], PSPNativeVramDirtyCalls;
static void Dirty(const void *p, size_t n)
{
    uintptr_t base = (uintptr_t)s_HW_LCDC_VRAM, a = (uintptr_t)p;
    if (!n || a >= base + kVramBytes || a + n <= base) return;
    size_t lo = a < base ? 0 : a - base, hi = a + n - base;
    if (hi > kVramBytes) hi = kVramBytes;
    unsigned g = ++PSPNativeVramGen;
    for (size_t pg = lo >> 12; pg <= (hi - 1) >> 12; pg++) PSPNativeVramPageGen[pg] = g;
    PSPNativeVramDirtyCalls++;
}
void PSPNativeVramDirty(const void *p, u32 n) { Dirty(p, n); }
/* MI prototypes come from the force-included SDK pch (global_pch.h). */
void *PSPNativeVramW_memset(void *d, int c, size_t n) { Dirty(d, n); memset(d, c, n); Dirty(d, n); return d; }
void PSPNativeVramW_MIi_CpuCopy32(const void *s, void *d, u32 n) { Dirty(d, n); MIi_CpuCopy32(s, d, n); Dirty(d, n); }
void PSPNativeVramW_MIi_CpuCopy16(const void *s, void *d, u32 n) { Dirty(d, n); MIi_CpuCopy16(s, d, n); Dirty(d, n); }
void PSPNativeVramW_MI_DmaCopy32(u32 id, const void *s, void *d, u32 n) { Dirty(d, n); MI_DmaCopy32(id, s, d, n); Dirty(d, n); }
void PSPNativeVramW_MI_DmaCopy16(u32 id, const void *s, void *d, u32 n) { Dirty(d, n); MI_DmaCopy16(id, s, d, n); Dirty(d, n); }
void PSPNativeVramW_MI_DmaCopy32Async(u32 id, const void *s, void *d, u32 n, MIDmaCallback cb, void *arg)
{ Dirty(d, n); MI_DmaCopy32Async(id, s, d, n, cb, arg); Dirty(d, n); }
