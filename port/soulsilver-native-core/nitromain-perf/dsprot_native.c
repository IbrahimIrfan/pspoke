/* DS anti-piracy checks (overlay 123, ds_protect) for the PSP port.
 *
 * The originals inspect the physical cartridge bus, look for flashcart/emulator
 * signatures and hash code; their results feed "offset"/"key" arithmetic that
 * sabotages the game subtly when they disagree with a genuine cartridge. The
 * C units already compile them out under SDK_BUILD_PSP (field_init.c,
 * fieldmap_native.c). Translated assembly on the map-exit path still calls
 * them, so these return exactly the genuine-cartridge answers: every sabotage
 * term stays zero, which is what the original hardware produced.
 * The argument is the signature block the original reads; ignored here. */
#include <nitro/types.h>
BOOL DSProt_DetectFlashcart(const void *sig)    { (void)sig; return FALSE; }
BOOL DSProt_DetectNotFlashcart(const void *sig) { (void)sig; return TRUE;  }
BOOL DSProt_DetectDummy(const void *sig)        { (void)sig; return FALSE; }
BOOL DSProt_DetectNotDummy(const void *sig)     { (void)sig; return TRUE;  }
BOOL DSProt_DetectEmulator(const void *sig)     { (void)sig; return FALSE; }
BOOL DSProt_DetectNotEmulator(const void *sig)  { (void)sig; return TRUE;  }
