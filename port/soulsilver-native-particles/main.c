#include <pspkernel.h>
#include <psppower.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "spl_manager.h"
#include "spl_random.h"
PSP_MODULE_INFO("SS particle engine proof",0,1,0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER|PSP_THREAD_ATTR_VFPU);
extern unsigned char fixture[],fixture_end[];
extern void sub_02015484(void*,int,const VecFx32*);extern void sub_020154C4(void*,SPLEmitter*);
static unsigned checks,failures,used,allocs;static unsigned char arena[0x100000] __attribute__((aligned(16)));
#define CHECK(x) do{checks++;if(!(x)){if(failures<10)printf("FAIL %d\n",__LINE__);failures++;}}while(0)
void GF_AssertFail(void){puts("fatal assertion");abort();}
static void *Allocate(u32 n){used=(used+3)&~3u;if(used+n>sizeof arena)abort();void*p=arena+used;used+=n;allocs++;return p;}
static void lists(SPLManager*m){int active=0,inactive=0,particles=0;SPLEmitter*prev=NULL;for(SPLEmitter*e=m->activeEmitters.first;e;e=e->next){CHECK(e->prev==prev);prev=e;active++;CHECK(active<=m->maxEmitters);int np=0;for(SPLParticle*p=e->particles.first;p;p=p->next){np++;CHECK(np<=m->maxParticles);}CHECK(np==e->particles.count);particles+=np;np=0;for(SPLParticle*p=e->childParticles.first;p;p=p->next){np++;CHECK(np<=m->maxParticles);}CHECK(np==e->childParticles.count);particles+=np;}CHECK(active==m->activeEmitters.count);for(SPLEmitter*e=m->inactiveEmitters.first;e;e=e->next){inactive++;CHECK(inactive<=m->maxEmitters);}CHECK(active+inactive==m->maxEmitters&&inactive==m->inactiveEmitters.count);int n=0;for(SPLParticle*p=m->inactiveParticles.first;p;p=p->next){n++;CHECK(n<=m->maxParticles);}CHECK(n==m->inactiveParticles.count&&n+particles==m->maxParticles);}
int main(void){scePowerSetClockFrequency(333,333,166);CHECK(sizeof(SPLManager)==0x4c);CHECK(sizeof(SPLEmitter)==0x9c);CHECK(sizeof(SPLParticle)==0x44);CHECK(sizeof(SPLResource)==0x20);CHECK(offsetof(SPLEmitter,position)==0x28);CHECK(sizeof(SPLResourceHeader)==0x58);
 for(int a=0;a<64;a++){used=allocs=0;memset(arena,0xa5,sizeof arena);SPLManager*m=SPLManager_New(Allocate,a%21,a*5,a,a+1,a+2);CHECK(allocs==3);CHECK(m->polygonID.fix==(a&63)&&m->polygonID.min==((a+1)&63)&&m->polygonID.max==((a+2)&63));CHECK(m->resources==NULL&&m->textures==NULL);lists(m);}
 used=allocs=0;SPLManager*m=SPLManager_New(Allocate,20,200,5,6,63);SPLManager_LoadResources(m,fixture);printf("[SS-PARTICLES] resources=%u textures=%u bytes=%u\n",m->resCount,m->texCount,used);CHECK(m->resCount==*(u16*)(fixture+8));CHECK(m->texCount==*(u16*)(fixture+10));
 for(unsigned i=0;i<m->resCount;i++){SPLResource*r=m->resources+i;CHECK((u8*)r->header>=fixture&&(u8*)r->header+sizeof *r->header<=fixture_end);}
 unsigned maxParticles=0,emitters=0;
 for(unsigned r=0;r<m->resCount;r++){gSPLRandomState=0;VecFx32 pos={0,0,64};SPLEmitter*e=SPLManager_CreateEmitter(m,r,&pos);CHECK(m->activeEmitters.count==1);emitters++;for(int frame=0;frame<240;frame++){SPLManager_Update(m);lists(m);unsigned p=200-m->inactiveParticles.count;if(p>maxParticles)maxParticles=p;if(!m->activeEmitters.count)break;}SPLManager_DeleteAllEmitters(m);lists(m);CHECK(m->activeEmitters.count==0&&m->inactiveParticles.count==200);}
 for(int r=0;r<m->resCount;r++){struct{SPLManager*m;void*data;SPLEmitter*active;}sys={m,0,0};VecFx32 pos={r*4096,0,64};sub_02015484(&sys,r,&pos);CHECK(m->activeEmitters.count==1);if(sys.active){CHECK(sys.active->resource==m->resources+r);sub_020154C4(&sys,sys.active);CHECK(m->activeEmitters.count==0);}else SPLManager_DeleteAllEmitters(m);lists(m);}
 printf("[SS-PARTICLES] checks=%u failures=%u emitters=%u max_particles=%u\n",checks,failures,emitters,maxParticles);sceKernelExitGame();return failures!=0;}
