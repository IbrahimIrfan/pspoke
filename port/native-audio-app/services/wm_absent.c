#include <nitro.h>
#include <nitro/wm.h>
#include <pspkernel.h>
#include <stdio.h>
#include <string.h>
/* The PSP has no DS wireless chip, and the SDK's WM library only forwards each call to the ARM7 over PXI tag 10,
 * which nothing here answers (the audio backend aborts on any non-sound tag). This replaces the whole WM API with a
 * radio that works but never finds another DS: every async call completes a frame later with SUCCESS, scans report
 * PARENT_NOT_FOUND, a parent sends beacons and nobody connects. That is exactly a DS on its own, which the game
 * handles: the Underground runs solo (CommSys "alone" mode loops its own commands back), wireless Mystery Gift
 * searches until cancelled. Wi-Fi (DCF, WEP, AP scans) fails at once; online play is not ported.
 * Callbacks run from PSPNativeWMPump on the main thread between frames (frame.c), where the DS would have taken the
 * FIFO interrupt, so they never race the game. Linked ahead of libsdk-filtered.a, so no SDK WM member is pulled in. */
typedef union{WMCallback base;WMStartParentCallback parent;WMStartScanCallback scan;WMStartConnectCallback connect;WMStartMPCallback mp;WMMeasureChannelCallback measure;WMPortSendCallback send;}WMResult;
typedef struct{WMCallbackFunc fn;unsigned long long due;WMResult r;}Pending;
static Pending queue[32];static int queued;
static WMStatus status;
static BOOL initialized,parent;
static WMCallbackFunc parentCallback;
static unsigned long long nextBeacon;
static u16 beaconMs=200,tgid;
static unsigned long long now(void){return sceKernelGetSystemTimeWide();}
/* Queue a result for fn, delivered no sooner than ms (and at least one frame) from now. NULL fn: nothing to deliver. */
static WMResult*post(WMCallbackFunc fn,u16 apiid,unsigned ms){
 static WMResult unused;
 if(!fn){memset(&unused,0,sizeof unused);return &unused;}
 if(queued==(int)(sizeof queue/sizeof queue[0])){puts("[WIRELESS] WM stand-in queue full");return &unused;}
 Pending*p=&queue[queued++];memset(p,0,sizeof *p);p->fn=fn;p->due=now()+(unsigned long long)(ms?ms:1)*1000;
 p->r.base.apiid=apiid;p->r.base.errcode=WM_ERRCODE_SUCCESS;return &p->r;
}
static WMErrCode done(WMCallbackFunc fn,u16 apiid){post(fn,apiid,0);return WM_ERRCODE_OPERATING;}
static void drop(u16 apiid){int j=0;for(int i=0;i<queued;i++)if(queue[i].r.base.apiid!=apiid)queue[j++]=queue[i];queued=j;}
void PSPNativeWMPump(void){
 unsigned long long t=now();
 for(int i=0;i<queued;){
  if(queue[i].due>t){i++;continue;}
  Pending p=queue[i];memmove(&queue[i],&queue[i+1],(size_t)(queued-i-1)*sizeof queue[0]);queued--;
  p.fn(&p.r);   /* may post more; those are due a frame later at the earliest */
 }
 if(parent&&parentCallback&&t>=nextBeacon){
  WMStartParentCallback b;memset(&b,0,sizeof b);b.apiid=WM_APIID_START_PARENT;b.errcode=WM_ERRCODE_SUCCESS;b.state=WM_STATECODE_BEACON_SENT;
  nextBeacon=t+(unsigned long long)beaconMs*1000;parentCallback(&b);
 }
}
static void stop(void){parent=FALSE;parentCallback=NULL;drop(WM_APIID_START_SCAN);}

WMErrCode WM_Init(void*buf,u16 dma){(void)buf;(void)dma;initialized=TRUE;return WM_ERRCODE_SUCCESS;}
WMErrCode WM_Initialize(void*buf,WMCallbackFunc fn,u16 dma){
 static BOOL told;if(!told){told=TRUE;puts("[WIRELESS] WM stand-in: radio on, no other DS in range");}
 (void)buf;(void)dma;initialized=TRUE;return done(fn,WM_APIID_INITIALIZE);
}
WMErrCode WM_InitializeForListening(void*buf,WMCallbackFunc fn,u16 dma,BOOL blink){(void)blink;return WM_Initialize(buf,fn,dma);}
WMErrCode WM_Finish(void){stop();queued=0;initialized=FALSE;return WM_ERRCODE_SUCCESS;}
WMErrCode WM_Enable(WMCallbackFunc fn){return done(fn,WM_APIID_ENABLE);}
WMErrCode WM_Disable(WMCallbackFunc fn){return done(fn,WM_APIID_DISABLE);}
WMErrCode WM_PowerOn(WMCallbackFunc fn){return done(fn,WM_APIID_POWER_ON);}
WMErrCode WM_PowerOff(WMCallbackFunc fn){return done(fn,WM_APIID_POWER_OFF);}
WMErrCode WM_Reset(WMCallbackFunc fn){stop();return done(fn,WM_APIID_RESET);}
WMErrCode WM_End(WMCallbackFunc fn){stop();initialized=FALSE;return done(fn,WM_APIID_END);}
WMErrCode WM_SetIndCallback(WMCallbackFunc fn){(void)fn;return WM_ERRCODE_SUCCESS;}
WMErrCode WM_SetPortCallback(u16 port,WMCallbackFunc fn,void*arg){(void)port;(void)fn;(void)arg;return WM_ERRCODE_SUCCESS;}
WMErrCode WM_SetLifeTime(WMCallbackFunc fn,u16 table,u16 cam,u16 frame,u16 mp){(void)table;(void)cam;(void)frame;(void)mp;return done(fn,WM_APIID_SET_LIFETIME);}
WMErrCode WM_SetParentParameter(WMCallbackFunc fn,const WMParentParam*p){if(p&&p->beaconPeriod)beaconMs=p->beaconPeriod;return done(fn,WM_APIID_SET_P_PARAM);}
WMErrCode WM_SetGameInfo(WMCallbackFunc fn,const u16*info,u16 size,u32 ggid,u16 id,u8 attr){(void)info;(void)size;(void)ggid;(void)id;(void)attr;return done(fn,WM_APIID_SET_GAMEINFO);}
WMErrCode WM_SetEntry(WMCallbackFunc fn,BOOL enabled){(void)enabled;return done(fn,WM_APIID_SET_ENTRY);}
WMErrCode WM_SetBeaconIndication(WMCallbackFunc fn,u16 flag){(void)flag;return done(fn,WM_APIID_SET_BEACON_IND);}
WMErrCode WM_StartParent(WMCallbackFunc fn){
 WMResult*r=post(fn,WM_APIID_START_PARENT,0);r->parent.state=WM_STATECODE_PARENT_START;
 parent=TRUE;parentCallback=fn;nextBeacon=now()+(unsigned long long)beaconMs*1000;return WM_ERRCODE_OPERATING;
}
WMErrCode WM_EndParent(WMCallbackFunc fn){stop();return done(fn,WM_APIID_END_PARENT);}
WMErrCode WM_StartMP(WMCallbackFunc fn,u16*recv,u16 recvSize,u16*send,u16 sendSize,u16 freq){
 (void)recvSize;(void)send;(void)sendSize;(void)freq;
 WMResult*r=post(fn,WM_APIID_START_MP,0);r->mp.state=WM_STATECODE_MP_START;r->mp.recvBuf=(WMMpRecvBuf*)recv;return WM_ERRCODE_OPERATING;
}
WMErrCode WM_StartMPEx(WMCallbackFunc fn,u16*recv,u16 recvSize,u16*send,u16 sendSize,u16 freq,u16 retry,BOOL minPoll,BOOL single,BOOL fixFreq,BOOL ignoreFatal){
 (void)retry;(void)minPoll;(void)single;(void)fixFreq;(void)ignoreFatal;return WM_StartMP(fn,recv,recvSize,send,sendSize,freq);
}
WMErrCode WM_EndMP(WMCallbackFunc fn){return done(fn,WM_APIID_END_MP);}
/* Nobody is connected, so the data reaches no one; report the send itself as done. */
WMErrCode WM_SetMPDataToPortEx(WMCallbackFunc fn,void*arg,const u16*data,u16 size,u16 dest,u16 port,u16 prio){
 (void)prio;WMResult*r=post(fn,WM_APIID_PORT_SEND,0);
 r->send.state=WM_STATECODE_PORT_SEND;r->send.port=port;r->send.destBitmap=dest;r->send.data=data;r->send.size=size;r->send.callback=fn;r->send.arg=arg;
 return WM_ERRCODE_OPERATING;
}
WMMpRecvData*WM_ReadMPData(const WMMpRecvHeader*header,u16 aid){(void)header;(void)aid;return NULL;}
WMErrCode WM_StartScan(WMCallbackFunc fn,const WMScanParam*param){
 WMResult*r=post(fn,WM_APIID_START_SCAN,param&&param->maxChannelTime?param->maxChannelTime:20);
 r->scan.state=WM_STATECODE_PARENT_NOT_FOUND;r->scan.channel=param?param->channel:1;return WM_ERRCODE_OPERATING;
}
WMErrCode WM_EndScan(WMCallbackFunc fn){drop(WM_APIID_START_SCAN);return done(fn,WM_APIID_END_SCAN);}
/* Never reached (scans find nobody); fail the way a vanished parent does. */
WMErrCode WM_StartConnectEx(WMCallbackFunc fn,const WMBssDesc*bss,const u8*ssid,BOOL powerSave,u16 auth){
 (void)bss;(void)ssid;(void)powerSave;(void)auth;post(fn,WM_APIID_START_CONNECT,0)->base.errcode=WM_ERRCODE_FAILED;return WM_ERRCODE_OPERATING;
}
WMErrCode WM_Disconnect(WMCallbackFunc fn,u16 aid){(void)aid;return done(fn,WM_APIID_DISCONNECT);}
WMErrCode WM_MeasureChannel(WMCallbackFunc fn,u16 cca,u16 ed,u16 channel,u16 ms){
 (void)cca;(void)ed;WMResult*r=post(fn,WM_APIID_MEASURE_CHANNEL,ms);r->measure.channel=channel;r->measure.ccaBusyRatio=0;return WM_ERRCODE_OPERATING;
}
/* Channels 1, 7 and 13, as a DS reports them; 0x8000 is the SDK's "not initialized". */
u16 WM_GetAllowedChannel(void){return initialized?0x1041:0x8000;}
WMLinkLevel WM_GetLinkLevel(void){return WM_LINK_LEVEL_0;}
u16 WM_GetDispersionScanPeriod(void){return 40;}
u16 WM_GetDispersionBeaconPeriod(void){return 200;}
u16 WM_GetNextTgid(void){return ++tgid;}
const WMStatus*WMi_GetStatusAddress(void){return &status;}
/* Wi-Fi (access points) only: online play is not ported. */
WMErrCode WM_StartScanEx(WMCallbackFunc fn,const WMScanExParam*param){(void)fn;(void)param;return WM_ERRCODE_FAILED;}
WMErrCode WM_StartDCF(WMCallbackFunc fn,WMDcfRecvBuf*recv,u16 size){(void)fn;(void)recv;(void)size;return WM_ERRCODE_FAILED;}
WMErrCode WM_SetWEPKeyEx(WMCallbackFunc fn,u16 mode,u16 id,const u8*key){(void)fn;(void)mode;(void)id;(void)key;return WM_ERRCODE_FAILED;}
