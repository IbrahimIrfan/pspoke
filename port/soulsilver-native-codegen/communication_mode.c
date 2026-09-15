/* Full original Thumb jump-table decision, unk_02033AE0.s:861.
 * uint32 arithmetic preserves the original unsigned bounds test after SUB. */
#include <stdint.h>
int sub_02034044(int mode)
{
    uint32_t index = (uint32_t)mode - 19u;
    if (index > 17u) return 0;
    return index <= 6u || index == 10u || index >= 14u;
}
