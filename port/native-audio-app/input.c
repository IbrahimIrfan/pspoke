#include <nitro.h>
#include <pspctrl.h>
#include <psprtc.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
static TPData point={128,96,0,TP_VALIDITY_VALID},sample;
static unsigned rtcReads;
static int touchMode;static unsigned previous;static BOOL sampling,rtcReady;
static TPCalibrateParam calibration;
/* osk.c: while the firmware keyboard owns the screen the game must see no
   buttons at all, otherwise the presses the user makes on the keyboard also
   drive whatever is suspended behind it. */
extern int PSPNativeOskIsActive(void);
static const unsigned buttons[12]={PSP_CTRL_CIRCLE,PSP_CTRL_CROSS,PSP_CTRL_SELECT,PSP_CTRL_START,PSP_CTRL_RIGHT,PSP_CTRL_LEFT,PSP_CTRL_UP,PSP_CTRL_DOWN,PSP_CTRL_RTRIGGER,PSP_CTRL_LTRIGGER,PSP_CTRL_TRIANGLE,PSP_CTRL_SQUARE};
void PSPNativeInputStep(unsigned bits,unsigned ax,unsigned ay){
 unsigned combo=PSP_CTRL_SELECT|PSP_CTRL_CROSS,keys=0;
 /* R toggles stylus mode (user request). SELECT+CROSS still works. R is therefore not
    forwarded to the game as the DS R button; L still is. */
 if((bits&PSP_CTRL_RTRIGGER)&&!(previous&PSP_CTRL_RTRIGGER))touchMode=!touchMode;
 if((bits&combo)==combo&&(previous&combo)!=combo)touchMode=!touchMode;previous=bits;
 bits&=~PSP_CTRL_RTRIGGER;
 for(unsigned i=0;i<12;i++)if(bits&buttons[i])keys|=1u<<i;
 int x=point.x,y=point.y;
 /* Stylus mode (user request): only the analog stick moves the cursor, so the D-pad and face
    buttons keep working as normal DS input. L is the tap and is not passed to the game.
    Speed grows with tilt: dead zone 24, up to 5 px/frame at full tilt, sub-pixel steps kept. */
 if(touchMode){static int fx,fy;int dx=(int)ax-128,dy=(int)ay-128;keys&=~PAD_BUTTON_L;
  dx=dx>-24&&dx<24?0:dx>0?dx-24:dx+24;dy=dy>-24&&dy<24?0:dy>0?dy-24:dy+24;
  fx+=dx*5*256/104;fy+=dy*5*256/104;x+=fx/256;y+=fy/256;fx%=256;fy%=256;}
 point.x=x<0?0:x>255?255:x;point.y=y<0?0:y>191?191:y;
 point.touch=touchMode&&(bits&PSP_CTRL_LTRIGGER)?1:0;point.validity=TP_VALIDITY_VALID;
 s_reg_PAD_KEYINPUT=(~keys)&PAD_KEYPORT_MASK;
 *(vu16*)(uintptr_t)HW_BUTTON_XY_BUF=(~keys)&PAD_RCNTPORT_MASK; // PSP has no closing DS lid.
}
#ifdef PSP_NATIVE_SCRIPTED_INPUT
/* TEST ONLY: frame-indexed replay of a recorded new-game run so an unattended
   probe can reach the player name-entry keyboard. Never build a release with
   PSP_NATIVE_SCRIPTED_INPUT. */
#ifdef PSP_NATIVE_INPUT_SCRIPT
#include PSP_NATIVE_INPUT_SCRIPT
#else
#include "input_script.h"
#endif
extern unsigned PSPNativeRenderFrameCount(void);
void PSPNativeInputPoll(void){
 if(PSPNativeOskIsActive()){PSPNativeInputStep(0,128,128);return;}
 unsigned f=PSPNativeRenderFrameCount(),bits=0;int tx=-1,ty=-1,td=0;touchMode=0;
 for(unsigned i=0;i<sizeof(script)/sizeof(script[0]);i++)
  if(f>=script[i].first&&f<script[i].last){bits|=script[i].bits;if(script[i].x>=0){tx=script[i].x;ty=script[i].y;td=script[i].down;}}
 PSPNativeInputStep(bits,128,128);
 if(tx>=0){touchMode=1;point.x=tx;point.y=ty;point.touch=td;}
}
#else
void PSPNativeInputPoll(void){if(PSPNativeOskIsActive()){PSPNativeInputStep(0,128,128);return;}SceCtrlData pad;if(sceCtrlPeekBufferPositive(&pad,1)<0){printf("[INPUT] PSP pad read failed\n");abort();}PSPNativeInputStep(pad.Buttons,pad.Lx,pad.Ly);}
#endif
void TP_Init(void){sceCtrlSetSamplingCycle(0);sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);touchMode=0;previous=0;sampling=FALSE;point=(TPData){128,96,0,TP_VALIDITY_VALID};PSPNativeInputStep(0,128,128);}
BOOL TP_GetUserInfo(TPCalibrateParam*p){if(!p)return FALSE;*p=(TPCalibrateParam){0,0,256,256};return TRUE;}
void TP_SetCalibrateParam(const TPCalibrateParam*p){if(!p||p->x0||p->y0||p->xDotSize!=256||p->yDotSize!=256){printf("[INPUT] unsupported non-identity touch calibration\n");abort();}calibration=*p;}
void TP_GetCalibratedPoint(TPData*out,const TPData*raw){if(!out||!raw)abort();*out=*raw;}
void TP_RequestSamplingAsync(void){sample=point;sampling=TRUE;}
u32 TP_WaitRawResult(TPData*out){if(!out||!sampling)return TP_RESULT_ILLEGAL_STATUS;*out=sample;sampling=FALSE;return TP_RESULT_SUCCESS;}

void RTC_Init(void){rtcReady=TRUE;}
RTCResult RTC_GetDateTimeAsync(RTCDate*d,RTCTime*t,RTCCallback cb,void*arg){
 if(!rtcReady)return RTC_RESULT_ILLEGAL_STATUS;if(!d||!t||!cb)return RTC_RESULT_ILLEGAL_PARAMETER;
 ScePspDateTime now;if(sceRtcGetCurrentClockLocalTime(&now)<0)return RTC_RESULT_FATAL_ERROR;
 if(now.year<2000||now.year>2099)return RTC_RESULT_ILLEGAL_PARAMETER;
 *d=(RTCDate){now.year-2000,now.month,now.day,sceRtcGetDayOfWeek(now.year,now.month,now.day)};
 *t=(RTCTime){now.hour,now.minute,now.second};rtcReads++;cb(RTC_RESULT_SUCCESS,arg);return RTC_RESULT_SUCCESS;
}

unsigned PSPNativeRTCReadCount(void){return rtcReads;}
int PSPNativeInputQuitRequested(void){unsigned combo=PSP_CTRL_LTRIGGER|PSP_CTRL_RTRIGGER|PSP_CTRL_SELECT;return (previous&combo)==combo;}

void PSPNativeInputGetRenderState(unsigned*keys,int*mode,int*down,int*x,int*y){if(keys)*keys=PAD_Read();if(mode)*mode=touchMode;if(down)*down=point.touch;if(x)*x=point.x;if(y)*y=point.y;}

static TPData *autoBuffer;
static u16 autoCount,autoFrequency,autoIndex;
static u32 autoError;
void TP_RequestAutoSamplingStartAsync(u16 line,u16 frequency,TPData*buffer,u16 count){
 if(autoBuffer){autoError=TP_RESULT_ILLEGAL_STATUS;return;}
 if(line>=263||!frequency||frequency>16||!buffer||!count){autoError=TP_RESULT_INVALID_PARAMETER;return;}
 autoBuffer=buffer;autoCount=count;autoFrequency=frequency;autoIndex=0;autoError=0;
 for(u16 i=0;i<count;i++)autoBuffer[i]=(TPData){0,0,0,TP_VALIDITY_VALID};
}
void TP_RequestAutoSamplingStopAsync(void){if(!autoBuffer){autoError=TP_RESULT_ILLEGAL_STATUS;return;}autoBuffer=NULL;autoError=0;}
void TP_WaitBusy(TPRequestCommandFlag flags){(void)flags;/* PSP control requests complete synchronously. */}
u32 TP_CheckError(TPRequestCommandFlag flags){(void)flags;return autoError;}
u16 TP_GetLatestIndexInAuto(void){return autoIndex;}
void TP_GetLatestRawPointInAuto(TPData*out){if(!out||!autoBuffer)abort();*out=autoBuffer[autoIndex];}
void PSPNativeInputVBlank(void){if(autoBuffer)for(u16 i=0;i<autoFrequency;i++){autoIndex=(autoIndex+1)%autoCount;autoBuffer[autoIndex]=point;}}
static void RTCResultStore(RTCResult value,void*p){*(RTCResult*)p=value;}
RTCResult RTC_GetDateTime(RTCDate*d,RTCTime*t){RTCResult result=RTC_RESULT_FATAL_ERROR;RTCResult request=RTC_GetDateTimeAsync(d,t,RTCResultStore,&result);return request==RTC_RESULT_SUCCESS?result:request;}
RTCResult RTC_GetDate(RTCDate*d){RTCTime t;return RTC_GetDateTime(d,&t);}
RTCResult RTC_GetTime(RTCTime*t){RTCDate d;return RTC_GetDateTime(&d,t);}
