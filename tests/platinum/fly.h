/* Continue the save, close the journal (Start), open the party (Triangle, Pokemon), pick the first Pokemon (taught Fly by
 * TEACH_MOVE=0:19, with ALL_BADGES=1), choose Fly, move the town map cursor left four times (Oreburgh to Jubilife from
 * WARP_TO=45,302,775) and fly. The save must have visited Jubilife City. */
struct ScriptEvent{unsigned first,last,bits;int x,y,down;};
static const struct ScriptEvent script[]={
{360,363,PSP_CTRL_CIRCLE,-1,-1,0},
{410,413,PSP_CTRL_CIRCLE,-1,-1,0},
{460,463,PSP_CTRL_CIRCLE,-1,-1,0},
{510,513,PSP_CTRL_CIRCLE,-1,-1,0},
{560,563,PSP_CTRL_CIRCLE,-1,-1,0},
{610,613,PSP_CTRL_CIRCLE,-1,-1,0},
{930,933,PSP_CTRL_START,-1,-1,0},
{1850,1853,PSP_CTRL_TRIANGLE,-1,-1,0},
{2020,2023,PSP_CTRL_DOWN,-1,-1,0},
{2100,2103,PSP_CTRL_CIRCLE,-1,-1,0},
{2420,2423,PSP_CTRL_CIRCLE,-1,-1,0},
{2590,2593,PSP_CTRL_DOWN,-1,-1,0},
{2670,2673,PSP_CTRL_CIRCLE,-1,-1,0},
{3090,3093,PSP_CTRL_LEFT,-1,-1,0},
{3140,3143,PSP_CTRL_LEFT,-1,-1,0},
{3190,3193,PSP_CTRL_LEFT,-1,-1,0},
{3240,3243,PSP_CTRL_LEFT,-1,-1,0},
{3320,3323,PSP_CTRL_CIRCLE,-1,-1,0},
{3400,3403,PSP_CTRL_CIRCLE,-1,-1,0},
{3480,3483,PSP_CTRL_CIRCLE,-1,-1,0},
};
