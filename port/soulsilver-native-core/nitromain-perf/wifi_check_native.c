/* PSP port: native sub_0203A05C (asm/unk_02037C94.s). Returns TRUE when the save holds a Wi-Fi profile made on a valid
 * console. Used by the Pokemon Center Wi-Fi Club desk (ScrCmd_564) and the Goldenrod Global Terminal (ScrCmd_165).
 * The assembly passes the same profile pointer to both DWC checks; the port links the real DWC account code, so with
 * no profile on the PSP this returns FALSE and the scripts show their "not set up" branch instead of trapping. */
#include <nitro.h>

extern void *sub_0202C6F4(void *saveData);
extern void *sub_0202C08C(void *a0);
extern BOOL DWC_CheckHasProfile(const void *userdata);
extern BOOL DWC_CheckValidConsole(const void *userdata);

BOOL sub_0203A05C(void *saveData)
{
    void *userdata = sub_0202C08C(sub_0202C6F4(saveData));
    if (!DWC_CheckHasProfile(userdata)) {
        return FALSE;
    }
    return DWC_CheckValidConsole(userdata) ? TRUE : FALSE;
}
