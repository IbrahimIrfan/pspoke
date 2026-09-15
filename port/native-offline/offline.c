/* Explicit unavailable optional voice/network boundary for offline native build. */
#include <vct.h>
#include <stdio.h>
#include <stdlib.h>
static void unavailable(const char*name){printf("[NATIVE-OFFLINE] %s unavailable: voice/network not ported\n",name);}
static void unsupported(const char*name){unavailable(name);abort();}
BOOL VCT_Init(VCTConfig*c){(void)c;unavailable(__func__);return FALSE;}
void VCT_Cleanup(void){/* No successful initialization or resources to release. */}
void VCT_Main(void){unsupported(__func__);}
BOOL VCT_HandleData(u8 a,u8*b,int n){(void)a;(void)b;(void)n;unavailable(__func__);return FALSE;}
int VCT_Request(VCTSession*s,VCTRequestCode r){(void)s;(void)r;unavailable(__func__);return VCT_ERROR_BAD_MODE;}
int VCT_Response(VCTSession*s,VCTResponseCode r){(void)s;(void)r;unavailable(__func__);return VCT_ERROR_BAD_MODE;}
int VCT_AddConferenceClient(u8 a){(void)a;unavailable(__func__);return VCT_ERROR_BAD_MODE;}
VCTSession* VCT_CreateSession(u8 a){(void)a;unavailable(__func__);return NULL;}
BOOL VCT_DeleteSession(VCTSession*s){(void)s;unavailable(__func__);return FALSE;}
BOOL VCT_StartStreaming(VCTSession*s){(void)s;unavailable(__func__);return FALSE;}
void VCT_StopStreaming(VCTSession*s){(void)s;unsupported(__func__);}
BOOL VCT_SendAudio(void*p,u32 n){(void)p;(void)n;unavailable(__func__);return FALSE;}
BOOL VCT_ReceiveAudio(void*p,u32 n,u32*bitmap){(void)p;(void)n;(void)bitmap;unavailable(__func__);return FALSE;}
BOOL VCT_SetCodec(VCTCodec c){(void)c;unavailable(__func__);return FALSE;}
void VCT_GetVADInfo(VCTVADInfo*p){(void)p;unsupported(__func__);}
void VCT_EnableVAD(BOOL x){(void)x;unsupported(__func__);}
void VCT_EnableEchoCancel(BOOL x){(void)x;unsupported(__func__);}
/* SDK PSP branch defines HOSTENT as struct hostent. No fabricated local IP. */
struct hostent;
struct hostent* getlocalhost(void){unavailable(__func__);return NULL;}
