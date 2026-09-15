#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <pspkernel.h>
#include <stdio.h>
PSP_MODULE_INFO("Native SDL thread proof",0,1,0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER|PSP_THREAD_ATTR_VFPU);
static SDL_mutex *lock;static int value;
static int worker(void*p){if(SDL_LockMutex(lock))return -1;value=*(int*)p+7;SDL_UnlockMutex(lock);return 23;}
int main(void){int fail=0,input=35,status=0;lock=SDL_CreateMutex();if(!lock)return 1;if(SDL_LockMutex(lock)||SDL_LockMutex(lock))fail++;SDL_Thread*t=SDL_CreateThread(worker,"Proof",&input);if(!t)return 2;SDL_Delay(5);if(value)fail++;SDL_UnlockMutex(lock);SDL_UnlockMutex(lock);SDL_WaitThread(t,&status);if(value!=42||status!=23)fail++;SDL_DestroyMutex(lock);printf("[SDL-THREAD] failures=%d worker=%d status=%d realPSP synchronization adapter\n",fail,value,status);sceKernelExitGame();return fail;}
