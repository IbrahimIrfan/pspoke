/* Native replacement for the DS fixed-address header cache, not a ROM patch. */
#include <nitro.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static u8 checkedHeader[512] __attribute__((aligned(4)));
static BOOL headerReady;
static u32 read32(const u8 *p) {
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}
BOOL SSNativeValidateHeader(const u8 *header, unsigned size, const FSArchive *arc) {
    if (!header || size < sizeof(checkedHeader) || !arc) return FALSE;
    if (memcmp(header + 0x0c, "IPGE", 4) || memcmp(header + 0x10, "01", 2)) return FALSE;
    /* ROMFS already validates the table ranges against the real file size. */
    return arc->fat == read32(header + 0x48) && arc->fat_size == read32(header + 0x4c)
        && arc->fnt == read32(header + 0x40) && arc->fnt_size == read32(header + 0x44);
}
const u8 *SSNativeCheckedHeader(void) { return headerReady ? checkedHeader : NULL; }
void sub_02027010(void) {
    FSArchive *arc = FS_FindArchive("rom", 3);
    const u8 *header = CARD_GetRomHeader();
    if (!FS_IsAvailable() || !SSNativeValidateHeader(header, 512, arc)) {
        puts("[SS-BOOT] SoulSilver ROM header/archive mismatch");
        OS_Terminate();
        return;
    }
    memcpy(checkedHeader, header, sizeof(checkedHeader));
    /* The original routine writes this internal initialization marker after
       copying the real header. Preserve it only in the separate RAM copy. */
    checkedHeader[12] = 'A'; checkedHeader[13] = 'D';
    checkedHeader[14] = 'A'; checkedHeader[15] = 'J';
    headerReady = TRUE;
    printf("[SS-BOOT] actual IPGE header verified, FAT=%08lx FNT=%08lx\n",
           (unsigned long)arc->fat, (unsigned long)arc->fnt);
}
