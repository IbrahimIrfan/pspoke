/* Continue the save, close the journal (Start), open the start menu (Triangle), press Up once (the cursor starts on the
 * Pokedex and wraps to Exit) and choose Exit with A. */
struct ScriptEvent{unsigned first,last,bits;int x,y,down;};
static const struct ScriptEvent script[]={
{360,363,PSP_CTRL_CIRCLE,-1,-1,0},
{410,413,PSP_CTRL_CIRCLE,-1,-1,0},
{460,463,PSP_CTRL_CIRCLE,-1,-1,0},
{510,513,PSP_CTRL_CIRCLE,-1,-1,0},
{560,563,PSP_CTRL_CIRCLE,-1,-1,0},
{610,613,PSP_CTRL_CIRCLE,-1,-1,0},
{930,933,PSP_CTRL_START,-1,-1,0},
{1650,1653,PSP_CTRL_TRIANGLE,-1,-1,0},
{1820,1823,PSP_CTRL_UP,-1,-1,0},
{1880,1883,PSP_CTRL_CIRCLE,-1,-1,0},
};
