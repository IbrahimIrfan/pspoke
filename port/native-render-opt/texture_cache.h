#include <cstdlib>

#include <malloc.h>
extern "C" u8 s_HW_LCDC_VRAM[0xA4000];
struct TextureEntry{unsigned offset,pal,format,w,h,color0;u32 *pixels;u8 *snapshot;};
static TextureEntry cache[16];static unsigned cacheSize=0,cacheBytes=0;
static void BindTexture(){
 const auto&p=s_texImageParam;
 if(p.textureFormat==0){sceGuDisable(GU_TEXTURE_2D);return;}
 if(p.textureFormat!=GX_TEXFMT_PLTT16&&p.textureFormat!=GX_TEXFMT_PLTT4&&p.textureFormat!=GX_TEXFMT_A3I5){printf("[TEXTURE] unsupported format%u\n",p.textureFormat);sceKernelExitGame();return;}
 if(p.flipS||p.flipT){printf("[TEXTURE] unsupported mirror\n");sceKernelExitGame();return;}
 unsigned footprint=p.textureSSize*p.textureTSize;
 if(p.textureSSize>512||p.textureTSize>512||p.textureOffset>0x20000||footprint>0x20000-p.textureOffset||s_texPlttBase>0x10000-64){printf("[TEXTURE] invalid input range\n");abort();}
 TextureEntry *entry=nullptr;
 for(unsigned i=0;i<cacheSize;i++){auto&e=cache[i];if(e.offset==p.textureOffset&&e.pal==s_texPlttBase&&e.format==p.textureFormat&&e.w==p.textureSSize&&e.h==p.textureTSize&&e.color0==p.color0&&!memcmp(e.snapshot,s_HW_LCDC_VRAM+p.textureOffset,e.w*e.h)&&!memcmp(e.snapshot+e.w*e.h,s_HW_LCDC_VRAM+0x80000+s_texPlttBase,64)){entry=&e;break;}}
 if(!entry){
  unsigned w=p.textureSSize,h=p.textureTSize,bytes=w*h*4;
  if(cacheSize>=16||cacheBytes+bytes>1024*1024||w>512||h>512){printf("[TEXTURE] cache limit\n");sceKernelExitGame();return;}
  if(p.textureOffset+w*h>0x20000||s_texPlttBase+512>0x10000){printf("[TEXTURE] range error\n");sceKernelExitGame();return;}
  entry=&cache[cacheSize++];*entry={p.textureOffset,s_texPlttBase,p.textureFormat,w,h,p.color0,(u32*)memalign(16,bytes),(u8*)malloc(w*h+64)};if(!entry->pixels||!entry->snapshot)abort();memcpy(entry->snapshot,s_HW_LCDC_VRAM+p.textureOffset,w*h);memcpy(entry->snapshot+w*h,s_HW_LCDC_VRAM+0x80000+s_texPlttBase,64);cacheBytes+=bytes;
  u8*src=s_HW_LCDC_VRAM+p.textureOffset;u16*pal=(u16*)(s_HW_LCDC_VRAM+0x80000+s_texPlttBase);u8*out=(u8*)entry->pixels;
  switch(p.textureFormat){
   case GX_TEXFMT_PLTT4:G3SIM_DecodeTex4(src,pal,out,w,h);break;
   case GX_TEXFMT_PLTT16:G3SIM_DecodeTex16(src,pal,out,w,h);break;
   case GX_TEXFMT_PLTT256:G3SIM_DecodeTex256(src,pal,out,w,h);break;
   case GX_TEXFMT_A3I5:G3SIM_DecodeTexA3I5(src,pal,out,w,h);break;
   case GX_TEXFMT_A5I3:G3SIM_DecodeTexA5I3(src,pal,out,w,h);break;
   case GX_TEXFMT_DIRECT:G3SIM_DecodeTexDirect(src,out,w,h);break;
   default:printf("[TEXTURE] unsupported format%u\n",p.textureFormat);sceKernelExitGame();return;
  }
  sceKernelDcacheWritebackRange(entry->pixels,bytes);
  printf("[TEXTURE] decoded format=%u size=%ux%u offset=%u pal=%u bytes=%u\n",p.textureFormat,w,h,p.textureOffset,s_texPlttBase,cacheBytes);printf("[TEXTURE] repeat=%u,%u mirror=%u,%u color0=%u\n",p.repeatS,p.repeatT,p.flipS,p.flipT,p.color0);
 }
 sceGuEnable(GU_BLEND);sceGuBlendFunc(GU_ADD,GU_SRC_ALPHA,GU_ONE_MINUS_SRC_ALPHA,0,0);
 sceGuEnable(GU_TEXTURE_2D);sceGuTexMode(GU_PSM_8888,0,0,GU_FALSE);sceGuTexImage(0,entry->w,entry->h,entry->w,entry->pixels);sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);sceGuTexFilter(GU_NEAREST,GU_NEAREST);sceGuTexWrap(p.repeatS?GU_REPEAT:GU_CLAMP,p.repeatT?GU_REPEAT:GU_CLAMP);
}
