/* Continue the save, close the journal (Start), then in the double battle started by DOUBLE_SING=1 (diag_double.c)
 * press A every 150 frames: Fight, Sing and a target for both Clefairy. The turn then plays both sides' Sing. */
struct ScriptEvent{unsigned first,last,bits;int x,y,down;};
static const struct ScriptEvent script[]={
{360,363,PSP_CTRL_CIRCLE,-1,-1,0},
{410,413,PSP_CTRL_CIRCLE,-1,-1,0},
{460,463,PSP_CTRL_CIRCLE,-1,-1,0},
{510,513,PSP_CTRL_CIRCLE,-1,-1,0},
{560,563,PSP_CTRL_CIRCLE,-1,-1,0},
{610,613,PSP_CTRL_CIRCLE,-1,-1,0},
{930,933,PSP_CTRL_START,-1,-1,0},
{2950,2953,PSP_CTRL_CIRCLE,-1,-1,0},
{3120,3123,PSP_CTRL_CIRCLE,-1,-1,0},
{3290,3293,PSP_CTRL_CIRCLE,-1,-1,0},
{3460,3463,PSP_CTRL_CIRCLE,-1,-1,0},
{3630,3633,PSP_CTRL_CIRCLE,-1,-1,0},
{3800,3803,PSP_CTRL_CIRCLE,-1,-1,0},
{3970,3973,PSP_CTRL_CIRCLE,-1,-1,0},
{4140,4143,PSP_CTRL_CIRCLE,-1,-1,0},
};
