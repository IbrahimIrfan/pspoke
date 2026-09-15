/* PSP port (Pokeathlon, overlay 96): the translated code loads some MWCC soft-float runtime helpers by ADDRESS
 * (ldr r3,=_ffltu; blx r3), so port.py cannot map those calls to its inline helpers and emits a data reference.
 * These real functions give the symbols a native definition with the MWCC register ABI: floats and u32 in r0/r1,
 * doubles as (low r0, high r1) / (low r2, high r3), double results returned in r1:r0 via the translator's pair
 * convention is NOT available through a plain C return, so only helpers whose result fits r0 are defined here;
 * double-returning _dsub is left to the SOFTFLOAT inline path and must be checked for pointer use first.
 * Semantics follow lib/asm/msl.s. Draft - not linked yet. */
#include <stdint.h>
#include <string.h>

static float f_bits(uint32_t w) { float f; memcpy(&f, &w, 4); return f; }
static uint32_t f_word(float f) { uint32_t w; memcpy(&w, &f, 4); return w; }
static double d_get(uint32_t lo, uint32_t hi) { uint64_t b = ((uint64_t)hi << 32) | lo; double d; memcpy(&d, &b, 8); return d; }

/* unsigned int -> float */
uint32_t _ffltu(uint32_t v) { return f_word((float)v); }

/* float -> unsigned int: negative non-NaN -> 0, NaN / too large -> 0xFFFFFFFF */
uint32_t _ffixu(uint32_t a)
{
    float x = f_bits(a);
    if (x != x) return 0xFFFFFFFFu;
    if (x <= 0.0f) return 0;
    if (x >= 4294967296.0f) return 0xFFFFFFFFu;
    return (uint32_t)x;
}

/* double (r1:r0) -> unsigned int */
uint32_t _dfixu(uint32_t lo, uint32_t hi)
{
    double d = d_get(lo, hi);
    if (d != d) return 0xFFFFFFFFu;
    if (d <= 0.0) return 0;
    if (d >= 4294967296.0) return 0xFFFFFFFFu;
    return (uint32_t)d;
}

/* double compares: result 0/1 in r0 (callers through a pointer can only use r0, not flags) */
uint32_t _dls(uint32_t alo, uint32_t ahi, uint32_t blo, uint32_t bhi) { return d_get(alo, ahi) < d_get(blo, bhi); }
uint32_t _dgr(uint32_t alo, uint32_t ahi, uint32_t blo, uint32_t bhi) { return d_get(alo, ahi) > d_get(blo, bhi); }
