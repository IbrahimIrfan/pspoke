/* Small SDL2 synchronization ABI adapter; real PSP kernels, no SDL video runtime. */
#include <SDL2/SDL_thread.h>
#include <SDL2/SDL_mutex.h>
#include <SDL2/SDL_timer.h>
#include <pspkernel.h>
#include <stdlib.h>
struct SDL_mutex {SceUID semaphore;SceUID owner;unsigned depth;};
struct SDL_Thread {SceUID id;SDL_ThreadFunction fn;void *arg;int result;};
SDL_mutex *SDL_CreateMutex(void){SDL_mutex*m=calloc(1,sizeof(*m));if(!m)return NULL;m->semaphore=sceKernelCreateSema("NativeSDLMutex",0,1,1,NULL);if(m->semaphore<0){free(m);return NULL;}m->owner=-1;return m;}
int SDL_LockMutex(SDL_mutex*m){if(!m)return -1;SceUID id=sceKernelGetThreadId();if(m->owner==id){m->depth++;return 0;}int r=sceKernelWaitSema(m->semaphore,1,NULL);if(r<0)return -1;m->owner=id;m->depth=1;return 0;}
int SDL_UnlockMutex(SDL_mutex*m){if(!m||m->owner!=sceKernelGetThreadId()||!m->depth)return -1;if(--m->depth)return 0;m->owner=-1;return sceKernelSignalSema(m->semaphore,1)<0?-1:0;}
void SDL_DestroyMutex(SDL_mutex*m){if(!m)return;if(m->depth)abort();if(sceKernelDeleteSema(m->semaphore)<0)abort();free(m);}
static int entry(SceSize n,void*p){(void)n;SDL_Thread*t=*(SDL_Thread**)p;t->result=t->fn(t->arg);return 0;}
SDL_Thread *SDL_CreateThread(SDL_ThreadFunction fn,const char*name,void*arg){if(!fn)return NULL;SDL_Thread*t=calloc(1,sizeof(*t));if(!t)return NULL;t->fn=fn;t->arg=arg;t->id=sceKernelCreateThread(name?name:"NativeSDL",entry,0x30,32768,PSP_THREAD_ATTR_USER|PSP_THREAD_ATTR_VFPU,NULL);if(t->id<0){free(t);return NULL;}if(sceKernelStartThread(t->id,sizeof(t),&t)<0){sceKernelDeleteThread(t->id);free(t);return NULL;}return t;}
void SDL_WaitThread(SDL_Thread*t,int*status){if(!t)return;if(sceKernelWaitThreadEnd(t->id,NULL)<0)abort();if(status)*status=t->result;if(sceKernelDeleteThread(t->id)<0)abort();free(t);}
void SDL_Delay(Uint32 ms){while(ms){unsigned part=ms>1000000?1000000:ms;if(sceKernelDelayThread(part*1000)<0)abort();ms-=part;}}
