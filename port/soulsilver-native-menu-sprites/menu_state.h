#include "sprite.h"
#include "bg_window.h"
#include <stddef.h>
struct MenuGraphics {
 BgConfig *bg;u32 unk4;enum HeapID heap;u32 opaqueC[10];
 SpriteList *list;G2dRenderer renderer;
 GF_2DGfxResMan *managers[6];SpriteResource *resources[2][6];SpriteResourcesHeader headers[2];
 u8 opaque1f0[16];u32 unknown200,unknown204;Sprite *sliding;u32 delay;fx32 subY;
};
extern unsigned char ssdata_overlay_74_thumb_ov74_0223D454[];
#define MENU ((struct MenuGraphics*)ssdata_overlay_74_thumb_ov74_0223D454)
#define ASSERT_OFFSET(field,off) typedef char assert_##field[(offsetof(struct MenuGraphics,field)==(off))?1:-1]
ASSERT_OFFSET(list,0x34);ASSERT_OFFSET(renderer,0x38);ASSERT_OFFSET(managers,0x160);ASSERT_OFFSET(resources,0x178);ASSERT_OFFSET(headers,0x1a8);ASSERT_OFFSET(sliding,0x208);ASSERT_OFFSET(delay,0x20c);ASSERT_OFFSET(subY,0x210);
