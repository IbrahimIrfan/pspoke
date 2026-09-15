#include <vct.h>
#include <pspkernel.h>
#include <stdio.h>
#include <string.h>
PSP_MODULE_INFO("Native offline proof",0,1,0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER|PSP_THREAD_ATTR_VFPU);
struct hostent;extern struct hostent*getlocalhost(void);
int main(void){int failures=0;VCTConfig c={0};unsigned char buf[16],before[16];memset(buf,0xa5,sizeof(buf));memcpy(before,buf,sizeof(buf));u32 bitmap=0x12345678;
if(VCT_Init(&c)||VCT_CreateSession(0)||VCT_SetCodec(VCT_CODEC_8BIT_RAW)||VCT_StartStreaming(NULL)||VCT_SendAudio(buf,sizeof(buf))||VCT_ReceiveAudio(buf,sizeof(buf),&bitmap)||VCT_HandleData(0,buf,sizeof(buf))||VCT_DeleteSession(NULL))failures++;
if(VCT_Request(NULL,VCT_REQUEST_INVITE)!=VCT_ERROR_BAD_MODE||VCT_Response(NULL,VCT_RESPONSE_OK)!=VCT_ERROR_BAD_MODE||VCT_AddConferenceClient(0)!=VCT_ERROR_BAD_MODE||getlocalhost())failures++;
if(memcmp(buf,before,sizeof(buf))||bitmap!=0x12345678)failures++;VCT_Cleanup();printf("[OFFLINE] failures=%d unsupported calls return errors, receivebuffer intact\n",failures);sceKernelExitGame();return failures;}
