/* Portable implementation of NitroSDK MIi_CpuClearFast, public
 * lib/NitroSDK/asm/mi_memory.s:103. The SDK requires a word-aligned destination.
 * Like the ARM loop, a nonmultiple byte size fills the final complete word.
 * Normal native valid-memory preconditions apply; this is not DS bus emulation.
 */
#include <stdint.h>
typedef uint32_t alias_word __attribute__((__may_alias__));
void MIi_CpuClearFast(uint32_t data, void *destination, uint32_t size)
{
    alias_word *out = (alias_word *)destination;
    uint32_t words = (size >> 2) + ((size & 3) != 0);
    while (words >= 8) {
        out[0] = data; out[1] = data; out[2] = data; out[3] = data;
        out[4] = data; out[5] = data; out[6] = data; out[7] = data;
        out += 8;
        words -= 8;
    }
    while (words != 0) {
        *out++ = data;
        words--;
    }
}
