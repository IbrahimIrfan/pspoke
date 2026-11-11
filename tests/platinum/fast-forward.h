/* Blank save: SELECT+SQUARE at frame 600 turns fast-forward on (the game clock runs at 2x); the intro keeps running to
 * the frame bound. A DEV build's [FPS] lines show the speed: about 30 before, about 60 after in PPSSPP. */
struct ScriptEvent{unsigned first,last,bits;int x,y,down;};
static const struct ScriptEvent script[]={
{300,302,PSP_CTRL_CIRCLE,-1,-1,0},
{600,606,PSP_CTRL_SELECT|PSP_CTRL_SQUARE,-1,-1,0},
};
