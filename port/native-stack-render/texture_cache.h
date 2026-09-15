#include <cstdlib>
#include <array>

#include <malloc.h>
extern "C" u8 s_HW_LCDC_VRAM[0xA4000];
extern "C" unsigned char PSPNative_GfxRegisters[];
// Logical DS texture/palette slots can be backed by different physical banks.
// Compare directly on cache hits; gather only on misses, including wired-OR overlaps.
#ifdef OPT_TEX_DIRTY
/* opt-tex: write generations per 4 KB page of s_HW_LCDC_VRAM, stamped by the app's VRAM write shims (vram_dirty.c). */
extern "C" unsigned PSPNativeVramGen,PSPNativeVramPageGen[],PSPNativeVramDirtyCalls;
#endif
/* maxGen (OPT_TEX_DIRTY): instead of copying/comparing, report the newest write generation of the physical pages
   that currently back the range (same bank resolution as the copy/compare). */
static bool TextureRange(unsigned address,unsigned size,bool palette,u8*dst,const u8*compare,unsigned*maxGen=nullptr){
 while(size){unsigned segment=palette?0x4000:0x20000;unsigned slot=address/segment,within=address&(segment-1),n=segment-within;if(n>size)n=size;
 const u8*src[4];unsigned count=0;
 if(!palette){for(unsigned b=0;b<4;b++){unsigned c=PSPNative_GfxRegisters[0x240+b];if((c&0x87)==0x83&&((c>>3)&3)==slot)src[count++]=s_HW_LCDC_VRAM+b*0x20000+within;}}
 else {unsigned e=PSPNative_GfxRegisters[0x244];if((e&0x87)==0x83&&slot<4)src[count++]=s_HW_LCDC_VRAM+0x80000+slot*0x4000+within;
 for(unsigned b=0;b<2;b++){unsigned c=PSPNative_GfxRegisters[0x245+b],ofs=(c>>3)&3;if((c&0x87)==0x83&&((ofs&1)+((ofs&2)<<1))==slot)src[count++]=s_HW_LCDC_VRAM+0x90000+b*0x4000+within;}}
 if(maxGen){
#ifdef OPT_TEX_DIRTY
  for(unsigned b=0;b<count;b++){unsigned o=(unsigned)(src[b]-s_HW_LCDC_VRAM);for(unsigned pg=o>>12,last=(o+n-1)>>12;pg<=last;pg++)if(PSPNativeVramPageGen[pg]>*maxGen)*maxGen=PSPNativeVramPageGen[pg];}
#endif
 }
 else if(count==1){if(compare&&!G3FastEqual(compare,src[0],n))return false;if(dst)memcpy(dst,src[0],n);}
 else {for(unsigned i=0;i<n;i++){u8 v=0;for(unsigned b=0;b<count;b++)v|=src[b][i];if(compare&&compare[i]!=v)return false;if(dst)dst[i]=v;}}
 if(dst)dst+=n;if(compare)compare+=n;address+=n;size-=n;
 }return true;
}
// A-D texture banks; E/F/G palette banks. Sizes are bytes, not pixels.
static bool TextureFootprint(unsigned format,unsigned w,unsigned h,unsigned offset,unsigned pal,
                             unsigned &texBytes,unsigned &palBytes){
 if(w<8||h<8||w>512||h>512||(w&(w-1))||(h&(h-1)))return false;
 unsigned pixels=w*h;
 switch(format){
 case GX_TEXFMT_PLTT4:texBytes=pixels/4;palBytes=8;break;
 case GX_TEXFMT_PLTT16:texBytes=pixels/2;palBytes=32;break;
 case GX_TEXFMT_PLTT256:texBytes=pixels;palBytes=512;break;
 case GX_TEXFMT_A3I5:texBytes=pixels;palBytes=64;break;
 case GX_TEXFMT_A5I3:texBytes=pixels;palBytes=16;break;
 case GX_TEXFMT_DIRECT:texBytes=pixels*2;palBytes=0;break;
 default:return false;
 }
 return offset<=0x80000&&texBytes<=0x80000-offset&&
        (!palBytes||(pal<=0x18000&&palBytes<=0x18000-pal));
}
struct TextureEntry{unsigned offset,pal,format,w,h,color0;u32 *pixels;u8 *snapshot;unsigned texBytes,palBytes;bool queued;unsigned gpuBytes,stride,psm;u32*clut;bool partialAlpha;bool mirS,mirT;unsigned texW,texH;unsigned validGen;u8 vramcnt[7];};
static TextureEntry cache[128];static unsigned cacheSize=0,cacheBytes=0,cacheNext=0;
static const unsigned kCacheMaxEntries=128,kCacheMaxBytes=2*1024*1024;
static unsigned cacheHighBytes=0,cacheHighEntries=0,cacheAllocRetries=0,cacheOversize=0,cacheEvictions=0;
// Per-window profile counters (reset by PSPNativeG3TexProfile): binds, cache hits, decodes, bytes compared on hits.
static unsigned texProfBinds=0,texProfHits=0,texProfMisses=0,texProfCmpBytes=0,texProfEvictions=0;
#ifdef OPT_TEX_DIRTY
/* [TEXDIRTY] per-frame counters: skips = hits accepted without a byte compare (gen equal / pages clean),
   pagechk = hits needing the page scan, full = hits validated by the old byte compare, fail = compares that
   found changed bytes; verify (OPT_TEX_VERIFY) = skipped hits re-compared in full, cumulative. */
static unsigned texDirtySkips,texDirtyPageChecks,texDirtyFull,texDirtyFail,texDirtyDirty0,texVerifyChecked,texVerifyMismatch;
extern "C" void PSPNativeMemLog(const char*,...);
extern "C" void PSPNativeTexDirtyLine(unsigned frames){
 if(!frames)return;
 PSPNativeMemLog("[TEXDIRTY] frames=%u skips=%u pagechk=%u full=%u fail=%u vram_writes=%u verify_checked=%u verify_mismatch=%u",frames,texDirtySkips/frames,texDirtyPageChecks/frames,texDirtyFull/frames,texDirtyFail/frames,(PSPNativeVramDirtyCalls-texDirtyDirty0)/frames,texVerifyChecked,texVerifyMismatch);
 texDirtySkips=texDirtyPageChecks=texDirtyFull=texDirtyFail=0;texDirtyDirty0=PSPNativeVramDirtyCalls;}
#endif
extern "C" void PSPNativeG3TexProfile(unsigned*binds,unsigned*hits,unsigned*misses,unsigned*cmpBytes,unsigned*evictions){
 if(binds)*binds=texProfBinds;if(hits)*hits=texProfHits;if(misses)*misses=texProfMisses;if(cmpBytes)*cmpBytes=texProfCmpBytes;if(evictions)*evictions=texProfEvictions;
 texProfBinds=texProfHits=texProfMisses=texProfCmpBytes=texProfEvictions=0;}

extern "C" void PSPNativeG3CacheStats(unsigned*entries,unsigned*bytes,unsigned*highBytes,
                                      unsigned*highEntries,unsigned*retries,unsigned*oversize,unsigned*evictions){
 if(entries)*entries=cacheSize;if(bytes)*bytes=cacheBytes;if(highBytes)*highBytes=cacheHighBytes;
 if(highEntries)*highEntries=cacheHighEntries;if(retries)*retries=cacheAllocRetries;
 if(oversize)*oversize=cacheOversize;if(evictions)*evictions=cacheEvictions;
}
extern "C" void PSPNativeGUTextureFence();
// Called only after sceGuSync has completed every queued texture read.
extern "C" void PSPNativeG3TexturesComplete(){for(unsigned i=0;i<cacheSize;i++)cache[i].queued=false;}
static void EvictOne(){
 if(!cacheSize)return;
 unsigned i=cacheNext++%cacheSize;
 if(cache[i].queued)PSPNativeGUTextureFence();
 TextureEntry&e=cache[i];
 if(e.queued){PSPNativeFatal("incomplete texture eviction fence");}
 cacheBytes-=e.gpuBytes+e.texBytes+e.palBytes;free(e.pixels);free(e.snapshot);cacheEvictions++;texProfEvictions++;
 if(i!=cacheSize-1)e=cache[cacheSize-1];cache[--cacheSize]={};
}
/* True when the bound texture has texels with 0<alpha<255 (DS translucent texels). */
static bool g3TexPartialAlpha=false;
static void BindTexture(){
 const auto&p=s_texImageParam;g3TexPartialAlpha=false;
 if(p.textureFormat==0){sceGuDisable(GU_TEXTURE_2D);return;}
 if(p.textureFormat==GX_TEXFMT_COMP4x4){printf("[TEXTURE] unsupported format%u (4x4 compressed)\n",p.textureFormat);sceGuDisable(GU_TEXTURE_2D);return;}
 /* DS mirrored repeat (repeat+flip on an axis): the GE has no mirror mode, so the texture is cached
    twice as wide/tall with the mirrored copy appended and drawn with GU_REPEAT. Through-mode UVs are
    texels of the DS size (unchanged); raw 3D UVs are normalised, so TexScale halves that axis.
    Doubling past the GE's 512 limit keeps the old plain-repeat approximation. */
 bool mS=p.repeatS&&p.flipS,mT=p.repeatT&&p.flipT;
 if((mS&&p.textureSSize*2>512)||(mT&&p.textureTSize*2>512)){if(p.textureSSize*2>512)mS=false;if(p.textureTSize*2>512)mT=false;static unsigned warned;if(warned<8){warned++;printf("[TEXTURE] mirrored wrap %ux%u exceeds GE size; approximated as repeat\n",p.textureSSize,p.textureTSize);}}
 unsigned texBytes=0,palBytes=0;
 if(!TextureFootprint(p.textureFormat,p.textureSSize,p.textureTSize,p.textureOffset,s_texPlttBase,texBytes,palBytes)){static unsigned warned;if(warned<8){warned++;printf("[TEXTURE] invalid input range fmt=%u %ux%u off=%u pal=%u; drawing untextured\n",p.textureFormat,p.textureSSize,p.textureTSize,p.textureOffset,s_texPlttBase);}sceGuDisable(GU_TEXTURE_2D);return;}
 TextureEntry *entry=nullptr;texProfBinds++;
 for(unsigned i=0;i<cacheSize;i++){auto&e=cache[i];if(e.offset==p.textureOffset&&e.pal==s_texPlttBase&&e.format==p.textureFormat&&e.w==p.textureSSize&&e.h==p.textureTSize&&e.color0==p.color0&&e.mirS==mS&&e.mirT==mT){
#ifdef OPT_TEX_DIRTY
  /* Exact skip: the snapshot equalled VRAM when validated at generation validGen under the same VRAMCNT mapping,
     and no page backing the texture or palette has been written since. */
  if(memcmp(e.vramcnt,PSPNative_GfxRegisters+0x240,7)==0){
   bool clean=e.validGen==PSPNativeVramGen;
   if(!clean){unsigned g=0;TextureRange(p.textureOffset,texBytes,false,nullptr,nullptr,&g);if(palBytes)TextureRange(s_texPlttBase,palBytes,true,nullptr,nullptr,&g);clean=g<=e.validGen;texDirtyPageChecks++;}
   if(clean){
#ifdef OPT_TEX_VERIFY
    texVerifyChecked++;
    if(!((!palBytes||TextureRange(s_texPlttBase,palBytes,true,nullptr,e.snapshot+texBytes))&&TextureRange(p.textureOffset,texBytes,false,nullptr,e.snapshot))){
     if(texVerifyMismatch++<20){printf("[TEX-VERIFY] MISMATCH off=%u pal=%u fmt=%u %ux%u gen=%u valid=%u\n",p.textureOffset,s_texPlttBase,p.textureFormat,p.textureSSize,p.textureTSize,PSPNativeVramGen,e.validGen);PSPNativeMemLog("[TEX-VERIFY] MISMATCH off=%u pal=%u fmt=%u",p.textureOffset,s_texPlttBase,p.textureFormat);}
     continue;}   /* fall back to the old behaviour (entry stale) */
    if(!(texVerifyChecked%100000))printf("[TEX-VERIFY] checked=%u mismatches=%u\n",texVerifyChecked,texVerifyMismatch);
#endif
    e.validGen=PSPNativeVramGen;texDirtySkips++;entry=&e;break;}
  }
#endif
  texProfCmpBytes+=texBytes+palBytes;if((!palBytes||TextureRange(s_texPlttBase,palBytes,true,nullptr,e.snapshot+texBytes))&&TextureRange(p.textureOffset,texBytes,false,nullptr,e.snapshot)){
#ifdef OPT_TEX_DIRTY
   e.validGen=PSPNativeVramGen;memcpy(e.vramcnt,PSPNative_GfxRegisters+0x240,7);texDirtyFull++;
#endif
   entry=&e;break;}
#ifdef OPT_TEX_DIRTY
  texDirtyFail++;
#endif
 }}
 if(entry)texProfHits++;else texProfMisses++;
 if(!entry){
  unsigned w=p.textureSSize,h=p.textureTSize,stride=w,psm=GU_PSM_8888,clutBytes=0;
  unsigned W=mS?w*2:w,H=mT?h*2:h,stride0=w;
  if(p.textureFormat==GX_TEXFMT_PLTT4||p.textureFormat==GX_TEXFMT_PLTT16){psm=GU_PSM_T4;stride0=w<32?32:w;stride=W<32?32:W;clutBytes=16*4;}
  if(p.textureFormat==GX_TEXFMT_PLTT256||p.textureFormat==GX_TEXFMT_A3I5||p.textureFormat==GX_TEXFMT_A5I3){psm=GU_PSM_T8;stride0=w<16?16:w;stride=W<16?16:W;clutBytes=256*4;}
  if(psm==GU_PSM_8888)stride=W;
  unsigned imageBytes=psm==GU_PSM_T4?stride*H/2:psm==GU_PSM_T8?stride*H:W*H*4;
  unsigned imageBytes0=psm==GU_PSM_T4?stride0*h/2:psm==GU_PSM_T8?stride0*h:w*h*4;
  unsigned bytes=imageBytes+clutBytes,totalBytes=bytes+texBytes+palBytes;
  if(totalBytes>kCacheMaxBytes){
   cacheOversize++;
   if(cacheOversize<=8||cacheOversize%600==0)
   printf("[TEXTURE] oversize texture %ux%u fmt=%u needs=%u cap=%u; drawing untextured\n",w,h,p.textureFormat,totalBytes,kCacheMaxBytes);
   sceGuDisable(GU_TEXTURE_2D);return;
  }
  while(cacheSize>=kCacheMaxEntries||(cacheSize&&cacheBytes+totalBytes>kCacheMaxBytes))EvictOne();
  u32*pixels=nullptr;u8*snapshot=nullptr;
  for(;;){
   pixels=(u32*)memalign(16,bytes);snapshot=pixels?(u8*)malloc(texBytes+palBytes):nullptr;
   if(pixels&&snapshot)break;
   free(pixels);free(snapshot);pixels=nullptr;snapshot=nullptr;
   cacheAllocRetries++;
   if(!cacheSize){
    if(cacheAllocRetries<=8||cacheAllocRetries%600==0)
    printf("[TEXTURE] allocation FAILED pixels=%u snapshot=%u maxfree=%u totalfree=%u; drawing untextured\n",
           bytes,texBytes+palBytes,(unsigned)sceKernelMaxFreeMemSize(),(unsigned)sceKernelTotalFreeMemSize());
    sceGuDisable(GU_TEXTURE_2D);return;
   }
   EvictOne();
  }
  entry=&cache[cacheSize++];*entry={p.textureOffset,s_texPlttBase,p.textureFormat,w,h,p.color0,pixels,snapshot,texBytes,palBytes,false,bytes,stride,psm,nullptr,false,mS,mT,W,H};
#ifdef OPT_TEX_DIRTY
  entry->validGen=PSPNativeVramGen;memcpy(entry->vramcnt,PSPNative_GfxRegisters+0x240,7);
#endif
  TextureRange(p.textureOffset,texBytes,false,entry->snapshot,nullptr);
  if(palBytes)TextureRange(s_texPlttBase,palBytes,true,entry->snapshot+texBytes,nullptr);
  cacheBytes+=totalBytes;
  if(cacheBytes>cacheHighBytes)cacheHighBytes=cacheBytes;
  if(cacheSize>cacheHighEntries)cacheHighEntries=cacheSize;
  u8*src=entry->snapshot;u16*pal=palBytes?(u16*)(entry->snapshot+texBytes):nullptr;u8*const image=(u8*)entry->pixels;
  /* Mirrored entries decode at the DS size into a scratch image, then get composed below. */
  u8*out=image;
  if(mS||mT){out=(u8*)malloc(imageBytes0);
   if(!out){printf("[TEXTURE] mirror buffer failed %ux%u; drawing untextured\n",w,h);cacheBytes-=totalBytes;free(entry->pixels);free(entry->snapshot);*entry={};cacheSize--;sceGuDisable(GU_TEXTURE_2D);return;}}
  if(clutBytes){
   memset(out,0,imageBytes0);unsigned rowBytes=psm==GU_PSM_T4?w/2:w,step=psm==GU_PSM_T4?stride0/2:stride0;
   if(p.textureFormat==GX_TEXFMT_PLTT4){
    static constexpr auto expand=[](){std::array<u16,256>a{};for(unsigned i=0;i<256;i++)a[i]=(i&3)|((i&12)<<2)|((i&48)<<4)|((i&192)<<6);return a;}();
    for(unsigned y=0;y<h;y++){u16*row=(u16*)(out+y*step);for(unsigned x=0;x<w/4;x++)row[x]=expand[src[y*(w/4)+x]];}
   }else for(unsigned y=0;y<h;y++)memcpy(out+y*step,src+y*rowBytes,rowBytes);
   entry->clut=(u32*)(image+imageBytes);
   if(p.textureFormat==GX_TEXFMT_A3I5||p.textureFormat==GX_TEXFMT_A5I3){
    // Preserve the existing decoder alpha expansion exactly, including max248.
    u8 codes[256];for(unsigned i=0;i<256;i++)codes[i]=i;
    if(p.textureFormat==GX_TEXFMT_A3I5)G3SIM_DecodeTexA3I5(codes,pal,(u8*)entry->clut,16,16);
    else G3SIM_DecodeTexA5I3(codes,pal,(u8*)entry->clut,16,16);
   }else for(unsigned i=0;i<clutBytes/4;i++){if(p.textureFormat==GX_TEXFMT_PLTT4&&i>=4){entry->clut[i]=0;continue;}u8 r,g,b;SIM_u16ToRGB(pal[i],&r,&g,&b);entry->clut[i]=r|(g<<8)|(b<<16)|((p.color0&&i==0)?0:0xff000000u);}
  }else switch(p.textureFormat){
   case GX_TEXFMT_PLTT4:G3SIM_DecodeTex4(src,pal,out,w,h);break;
   case GX_TEXFMT_PLTT16:G3SIM_DecodeTex16(src,pal,out,w,h);break;
   case GX_TEXFMT_PLTT256:G3SIM_DecodeTex256(src,pal,out,w,h);break;
   case GX_TEXFMT_A3I5:G3SIM_DecodeTexA3I5(src,pal,out,w,h);break;
   case GX_TEXFMT_A5I3:G3SIM_DecodeTexA5I3(src,pal,out,w,h);break;
   case GX_TEXFMT_DIRECT:G3SIM_DecodeTexDirect(src,out,w,h);break;
   default:printf("[TEXTURE] unsupported format%u\n",p.textureFormat);sceKernelExitGame();return;
  }
  if(out!=image){
   /* DS mirrored repeat: texel X in [w,2w) shows source texel 2w-1-X (same for T). */
   if(clutBytes)memset(image,0,imageBytes);
   for(unsigned Y=0;Y<H;Y++){unsigned y=Y<h?Y:2*h-1-Y;
    for(unsigned X=0;X<W;X++){unsigned x=X<w?X:2*w-1-X;
     if(psm==GU_PSM_T4){unsigned v=(out[y*(stride0/2)+(x>>1)]>>((x&1)*4))&15;image[Y*(stride/2)+(X>>1)]|=u8(v<<((X&1)*4));}
     else if(psm==GU_PSM_T8)image[Y*stride+X]=out[y*stride0+x];
     else memcpy(image+(Y*W+X)*4,out+(y*w+x)*4,4);}}
   free(out);out=image;
  }
  // Alpha-zero texels are rejected before color/depth writes. Canonicalize
  // their RGB too: PPSSPP's software CLUT optimization incorrectly tests
  // the whole RGBA bitwise AND when deciding whether alpha can be zero.
  if(entry->clut)for(unsigned i=0;i<clutBytes/4;i++)
   if(!(entry->clut[i]>>24))entry->clut[i]=0;
  {bool pa=false;if(entry->clut){for(unsigned i=0;i<clutBytes/4&&!pa;i++){unsigned a=entry->clut[i]>>24;pa=a&&a!=255;}}
   else {for(unsigned i=0;i<W*H&&!pa;i++){unsigned a=entry->pixels[i]>>24;pa=a&&a!=255;}}
   entry->partialAlpha=pa;}
  sceKernelDcacheWritebackRange(entry->pixels,bytes);
  printf("[TEXTURE] decoded format=%u size=%ux%u offset=%u pal=%u bytes=%u\n",p.textureFormat,w,h,p.textureOffset,s_texPlttBase,cacheBytes);printf("[TEXTURE] repeat=%u,%u mirror=%u,%u color0=%u gpu=%ux%u\n",p.repeatS,p.repeatT,p.flipS,p.flipT,p.color0,W,H);
 }
 entry->queued=true;g3TexPartialAlpha=entry->partialAlpha;
 sceGuEnable(GU_BLEND);sceGuBlendFunc(GU_ADD,GU_SRC_ALPHA,GU_ONE_MINUS_SRC_ALPHA,0,0);
 sceGuEnable(GU_TEXTURE_2D);
 if(entry->clut){sceGuClutMode(GU_PSM_8888,0,entry->psm==GU_PSM_T4?15:255,0);sceGuClutLoad(entry->psm==GU_PSM_T4?2:32,entry->clut);}
 sceGuTexMode(entry->psm,0,0,GU_FALSE);sceGuTexImage(0,entry->texW,entry->texH,entry->stride,entry->pixels);sceGuTexScale(entry->mirS?0.5f:1.0f,entry->mirT?0.5f:1.0f);sceGuTexOffset(0.0f,0.0f);sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);sceGuTexFilter(GU_NEAREST,GU_NEAREST);sceGuTexWrap(p.repeatS?GU_REPEAT:GU_CLAMP,p.repeatT?GU_REPEAT:GU_CLAMP);
}
