/* Blank save: hold L+R+SELECT on the title screen; the game must log [QUIT] and exit to the PSP menu before the frame
 * bound (issue #16). */
struct ScriptEvent{unsigned first,last,bits;int x,y,down;};
static const struct ScriptEvent script[]={
{2000,2030,PSP_CTRL_LTRIGGER|PSP_CTRL_RTRIGGER|PSP_CTRL_SELECT,-1,-1,0},
};
