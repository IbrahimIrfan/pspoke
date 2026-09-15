/* Diagnostic replays only (make NO_WILD=1): no walking encounters, so a long exploration
 * script is not interrupted by a battle it cannot answer. Never part of a release link. */
#include <nitro.h>
BOOL __real_MetatileBehavior_CanGenerateWalkingEncounters(u8 tile);
BOOL __wrap_MetatileBehavior_CanGenerateWalkingEncounters(u8 tile) { (void)tile; return FALSE; }
