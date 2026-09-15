#include <nitro/fs.h>
#include <nitro/card.h>
#include <pspkernel.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "narc.h"
#include "expected.h"
#include "expected_narc.h"
PSP_MODULE_INFO("Native ROM FS probe",0,1,0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER|PSP_THREAD_ATTR_VFPU);
// Standalone probe provides the SDK_PORT memory backing; integrated game owns it.
u8 s_HW_MAIN_MEM[0x800000];
u8 s_HW_MAIN_MEM_SYSTEM[0x400];
extern void CheckForMemoryTampering(void);
static unsigned Hash(const unsigned char*p,unsigned n){unsigned h=2166136261u;while(n--)h=(h^*p++)*16777619u;return h;}
int main(void){
 unsigned failures=0;FS_Init(1);if(!FS_IsAvailable()){printf("[ROMFS] mount failed\n");return 1;}
 CARD_Init();FSArchive*a=FS_FindArchive("rom",3);u32 tableSize=FS_GetTableSize();void*copy=malloc(tableSize);
 if(!a||!tableSize||!copy||!FS_LoadTable(copy,tableSize)||!FS_IsArchiveTableLoaded(a))failures++;
 const CARDRomHeader*h=(const CARDRomHeader*)CARD_GetRomHeader();
 if(!h||a->fat!=h->fat.offset||a->fnt!=h->fnt.offset||memcmp((void*)(uintptr_t)HW_ROM_HEADER_BUF,h,HW_CARD_ROM_HEADER_SIZE))failures++;
 CheckForMemoryTampering();
 for(unsigned i=0;i<CASE_COUNT;i++){
  const ProbeCase*c=&cases[i];FSFileID id;FSFile f;FS_InitFile(&f);unsigned char buf[64];
  if(!FS_ConvertPathToFileID(&id,c->path)||id.file_id!=c->id||id.arc!=a||!FS_OpenFileFast(&f,id)){failures++;continue;}
  if(FS_GetLength(&f)!=c->size||!FS_SeekFile(&f,c->offset,FS_SEEK_SET)||FS_ReadFileAsync(&f,buf,c->count)!=(s32)c->count||!FS_WaitAsync(&f)||Hash(buf,c->count)!=c->hash||FS_GetPosition(&f)!=c->offset+c->count)failures++;
  if(!FS_SeekFile(&f,0,FS_SEEK_END)||FS_ReadFile(&f,buf,1)!=0||FS_WriteFile(&f,"X",1)!=-1)failures++;
  FS_CloseFile(&f);
 }
 FSFile dir;FSDirEntry entry;unsigned children=0;
 if(!FS_FindDir(&dir,"rom:/graphic"))failures++;else{while(FS_ReadDir(&dir,&entry))children++;FS_CloseFile(&dir);}
 if(children!=GRAPHIC_CHILD_COUNT)failures++;
 FSFile f;FS_InitFile(&f);
 if(!FS_ChangeDir("rom:/graphic")||!FS_OpenFile(&f,"./pl_font.narc"))failures++;else FS_CloseFile(&f);
 if(!FS_ChangeDir("..")||!FS_OpenFile(&f,"graphic/pl_font.narc"))failures++;else FS_CloseFile(&f);
 if(FS_OpenFile(&f,"rom:/does-not-exist")||FS_OpenFile(&f,"other:/graphic/pl_font.narc"))failures++;
 for(unsigned i=0;i<NARC_CASE_COUNT;i++){
  const NarcCase*c=&narcCases[i];unsigned char buf[64];
  if(NARC_GetMemberSizeByIndexPair(c->narc,c->member)!=c->size)failures++;
  NARC_ReadFromMemberByIndexPair(buf,c->narc,c->member,c->offset,c->count);
  if(Hash(buf,c->count)!=c->hash)failures++;
 }
 printf("[ROMFS] actual narc.c member cases=%u\n",(unsigned)NARC_CASE_COUNT);
 if(FS_UnloadTable()!=copy)failures++;free(copy);
 printf("[ROMFS] cases=%u failures=%u graphicChildren=%u tableBytes=%lu headerCode=%08lx\n",(unsigned)CASE_COUNT,failures,children,(unsigned long)tableSize,(unsigned long)h->game_code);
 FS_End();sceKernelExitGame();return failures;
}
