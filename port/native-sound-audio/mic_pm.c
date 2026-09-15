#include <nitro.h>
#include <stdio.h>
static BOOL micReady;
static PMAmpSwitch amp=PM_AMP_OFF;
static PMAmpGain gain=PM_AMPGAIN_DEFAULT;
static PMBackLightSwitch lights[2]={PM_BACKLIGHT_ON,PM_BACKLIGHT_ON};
void MIC_Init(void){micReady=TRUE;}
static MICResult UnsupportedCapture(void){return micReady?MIC_RESULT_INVALID_COMMAND:MIC_RESULT_ILLEGAL_STATUS;}
MICResult MIC_DoSamplingAsync(MICSamplingType type,void*buf,MICCallback cb,void*arg){(void)type;(void)buf;(void)cb;(void)arg;return UnsupportedCapture();}
MICResult MIC_StartAutoSamplingAsync(const MICAutoParam*p,MICCallback cb,void*arg){(void)p;(void)cb;(void)arg;return UnsupportedCapture();}
MICResult MIC_StopAutoSamplingAsync(MICCallback cb,void*arg){(void)cb;(void)arg;return UnsupportedCapture();}
MICResult MIC_AdjustAutoSamplingAsync(u32 rate,MICCallback cb,void*arg){(void)rate;(void)cb;(void)arg;return UnsupportedCapture();}
MICResult MIC_DoSampling(MICSamplingType type,void*buf){return MIC_DoSamplingAsync(type,buf,NULL,NULL);}
MICResult MIC_StartAutoSampling(const MICAutoParam*p){return MIC_StartAutoSamplingAsync(p,NULL,NULL);}
MICResult MIC_StopAutoSampling(void){return MIC_StopAutoSamplingAsync(NULL,NULL);}
MICResult MIC_AdjustAutoSampling(u32 rate){return MIC_AdjustAutoSamplingAsync(rate,NULL,NULL);}
void*MIC_GetLastSamplingAddress(void){return NULL;}
u32 PM_SetAmp(PMAmpSwitch state){if(state!=PM_AMP_OFF&&state!=PM_AMP_ON)return PM_RESULT_ERROR;amp=state;return PM_RESULT_SUCCESS;}
u32 PM_GetAmp(PMAmpSwitch*out){if(!out)return PM_RESULT_ERROR;*out=amp;return PM_RESULT_SUCCESS;}
u32 PM_SetAmpGain(PMAmpGain state){if(state<PM_AMPGAIN_20||state>PM_AMPGAIN_160)return PM_RESULT_ERROR;gain=state;return PM_RESULT_SUCCESS;}
u32 PM_GetAmpGain(PMAmpGain*out){if(!out)return PM_RESULT_ERROR;*out=gain;return PM_RESULT_SUCCESS;}
u32 PM_SetBackLight(PMLCDTarget target,PMBackLightSwitch state){if(target<PM_LCD_TOP||target>PM_LCD_ALL||(state!=PM_BACKLIGHT_ON&&state!=PM_BACKLIGHT_OFF))return PM_RESULT_ERROR;if(target==PM_LCD_TOP||target==PM_LCD_ALL)lights[0]=state;if(target==PM_LCD_BOTTOM||target==PM_LCD_ALL)lights[1]=state;return PM_RESULT_SUCCESS;}
u32 PM_GetBackLight(PMBackLightSwitch*top,PMBackLightSwitch*bottom){if(top)*top=lights[0];if(bottom)*bottom=lights[1];return PM_RESULT_SUCCESS;}
u32 PM_SetAmpAsync(PMAmpSwitch state,PMCallback cb,void*arg){u32 r=PM_SetAmp(state);if(!r&&cb)cb(r,arg);return r;}
u32 PM_SetAmpGainAsync(PMAmpGain state,PMCallback cb,void*arg){u32 r=PM_SetAmpGain(state);if(!r&&cb)cb(r,arg);return r;}
u32 PM_SetBackLightAsync(PMLCDTarget target,PMBackLightSwitch state,PMCallback cb,void*arg){u32 r=PM_SetBackLight(target,state);if(!r&&cb)cb(r,arg);return r;}
