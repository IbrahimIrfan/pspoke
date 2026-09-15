#include "native_gpu.h"
namespace GPU {u32 VCount=0;alignas(8) u8 Palette[2048]={},OAM[2048]={};
u8 *VRAM[4]={};u32 VRAMMap_LCDC=0;Bits VRAMDirty[4];
u8 *VRAMFlat_ABG=s_HW_BG_VRAM,*VRAMFlat_BBG=s_HW_DB_BG_VRAM,*VRAMFlat_AOBJ=s_HW_OBJ_VRAM,*VRAMFlat_BOBJ=s_HW_DB_OBJ_VRAM;
u8 VRAMFlat_ABGExtPal[32768]={},VRAMFlat_BBGExtPal[32768]={},VRAMFlat_AOBJExtPal[8192]={},VRAMFlat_BOBJExtPal[8192]={};
Tracking VRAMDirty_ABG;u32 VRAMMap_ABG[32];
Tracking VRAMDirty_BBG;u32 VRAMMap_BBG[32];
Tracking VRAMDirty_AOBJ;u32 VRAMMap_AOBJ[32];
Tracking VRAMDirty_BOBJ;u32 VRAMMap_BOBJ[32];
Tracking VRAMDirty_ABGExtPal;u32 VRAMMap_ABGExtPal[32];
Tracking VRAMDirty_BBGExtPal;u32 VRAMMap_BBGExtPal[32];
Tracking VRAMDirty_AOBJExtPal;u32 VRAMMap_AOBJExtPal;
Tracking VRAMDirty_BOBJExtPal;u32 VRAMMap_BOBJExtPal;
}
namespace GPU3D {R renderer;R*CurrentRenderer=&renderer;u32 RenderXPos;u32*GetLine(unsigned){static u32 line[256]={};return line;}}
namespace GPU2D {
Unit::Unit(u32 num)
{
    Num = num;
}
void Unit::Reset()
{
    Enabled = false;
    DispCnt = 0;
    memset(BGCnt, 0, 4*2);
    memset(BGXPos, 0, 4*2);
    memset(BGYPos, 0, 4*2);
    memset(BGXRef, 0, 2*4);
    memset(BGYRef, 0, 2*4);
    memset(BGXRefInternal, 0, 2*4);
    memset(BGYRefInternal, 0, 2*4);
    memset(BGRotA, 0, 2*2);
    memset(BGRotB, 0, 2*2);
    memset(BGRotC, 0, 2*2);
    memset(BGRotD, 0, 2*2);

    memset(Win0Coords, 0, 4);
    memset(Win1Coords, 0, 4);
    memset(WinCnt, 0, 4);

    Win0Active = 0;
    Win1Active = 0;

    BGMosaicSize[0] = 0;
    BGMosaicSize[1] = 0;
    OBJMosaicSize[0] = 0;
    OBJMosaicSize[1] = 0;
    BGMosaicY = 0;
    BGMosaicYMax = 0;
    OBJMosaicY = 0;
    OBJMosaicYMax = 0;
    OBJMosaicYCount = 0;

    BlendCnt = 0;
    EVA = 16;
    EVB = 0;
    EVY = 0;

    memset(DispFIFO, 0, 16*2);
    DispFIFOReadPtr = 0;
    DispFIFOWritePtr = 0;

    memset(DispFIFOBuffer, 0, 256*2);

    CaptureCnt = 0;
    CaptureLatch = false;

    MasterBrightness = 0;
}
void Unit::Write16(u32 addr, u16 val)
{
    switch (addr & 0x00000FFF)
    {
    case 0x000:
        DispCnt = (DispCnt & 0xFFFF0000) | val;
        if (Num) DispCnt &= 0xC0B1FFF7;
        return;
    case 0x002:
        DispCnt = (DispCnt & 0x0000FFFF) | (val << 16);
        if (Num) DispCnt &= 0xC0B1FFF7;
        return;

    case 0x010:
        if (!Num) GPU3D::SetRenderXPos(val);
        break;

    case 0x068:
        DispFIFO[DispFIFOWritePtr] = val;
        return;
    case 0x06A:
        DispFIFO[DispFIFOWritePtr+1] = val;
        DispFIFOWritePtr += 2;
        DispFIFOWritePtr &= 0xF;
        return;

    case 0x06C: MasterBrightness = val; return;
    }

    if (!Enabled) return;

    switch (addr & 0x00000FFF)
    {
    case 0x008: BGCnt[0] = val; return;
    case 0x00A: BGCnt[1] = val; return;
    case 0x00C: BGCnt[2] = val; return;
    case 0x00E: BGCnt[3] = val; return;

    case 0x010: BGXPos[0] = val; return;
    case 0x012: BGYPos[0] = val; return;
    case 0x014: BGXPos[1] = val; return;
    case 0x016: BGYPos[1] = val; return;
    case 0x018: BGXPos[2] = val; return;
    case 0x01A: BGYPos[2] = val; return;
    case 0x01C: BGXPos[3] = val; return;
    case 0x01E: BGYPos[3] = val; return;

    case 0x020: BGRotA[0] = val; return;
    case 0x022: BGRotB[0] = val; return;
    case 0x024: BGRotC[0] = val; return;
    case 0x026: BGRotD[0] = val; return;
    case 0x028:
        BGXRef[0] = (BGXRef[0] & 0xFFFF0000) | val;
        if (GPU::VCount < 192) BGXRefInternal[0] = BGXRef[0];
        return;
    case 0x02A:
        if (val & 0x0800) val |= 0xF000;
        BGXRef[0] = (BGXRef[0] & 0xFFFF) | (val << 16);
        if (GPU::VCount < 192) BGXRefInternal[0] = BGXRef[0];
        return;
    case 0x02C:
        BGYRef[0] = (BGYRef[0] & 0xFFFF0000) | val;
        if (GPU::VCount < 192) BGYRefInternal[0] = BGYRef[0];
        return;
    case 0x02E:
        if (val & 0x0800) val |= 0xF000;
        BGYRef[0] = (BGYRef[0] & 0xFFFF) | (val << 16);
        if (GPU::VCount < 192) BGYRefInternal[0] = BGYRef[0];
        return;

    case 0x030: BGRotA[1] = val; return;
    case 0x032: BGRotB[1] = val; return;
    case 0x034: BGRotC[1] = val; return;
    case 0x036: BGRotD[1] = val; return;
    case 0x038:
        BGXRef[1] = (BGXRef[1] & 0xFFFF0000) | val;
        if (GPU::VCount < 192) BGXRefInternal[1] = BGXRef[1];
        return;
    case 0x03A:
        if (val & 0x0800) val |= 0xF000;
        BGXRef[1] = (BGXRef[1] & 0xFFFF) | (val << 16);
        if (GPU::VCount < 192) BGXRefInternal[1] = BGXRef[1];
        return;
    case 0x03C:
        BGYRef[1] = (BGYRef[1] & 0xFFFF0000) | val;
        if (GPU::VCount < 192) BGYRefInternal[1] = BGYRef[1];
        return;
    case 0x03E:
        if (val & 0x0800) val |= 0xF000;
        BGYRef[1] = (BGYRef[1] & 0xFFFF) | (val << 16);
        if (GPU::VCount < 192) BGYRefInternal[1] = BGYRef[1];
        return;

    case 0x040:
        Win0Coords[1] = val & 0xFF;
        Win0Coords[0] = val >> 8;
        return;
    case 0x042:
        Win1Coords[1] = val & 0xFF;
        Win1Coords[0] = val >> 8;
        return;

    case 0x044:
        Win0Coords[3] = val & 0xFF;
        Win0Coords[2] = val >> 8;
        return;
    case 0x046:
        Win1Coords[3] = val & 0xFF;
        Win1Coords[2] = val >> 8;
        return;

    case 0x048:
        WinCnt[0] = val & 0xFF;
        WinCnt[1] = val >> 8;
        return;
    case 0x04A:
        WinCnt[2] = val & 0xFF;
        WinCnt[3] = val >> 8;
        return;

    case 0x04C:
        BGMosaicSize[0] = val & 0xF;
        BGMosaicSize[1] = (val >> 4) & 0xF;
        OBJMosaicSize[0] = (val >> 8) & 0xF;
        OBJMosaicSize[1] = val >> 12;
        return;

    case 0x050: BlendCnt = val & 0x3FFF; return;
    case 0x052:
        BlendAlpha = val & 0x1F1F;
        EVA = val & 0x1F;
        if (EVA > 16) EVA = 16;
        EVB = (val >> 8) & 0x1F;
        if (EVB > 16) EVB = 16;
        return;
    case 0x054:
        EVY = val & 0x1F;
        if (EVY > 16) EVY = 16;
        return;
    }

    //printf("unknown GPU write16 %08X %04X\n", addr, val);
}
void Unit::Write32(u32 addr, u32 val)
{
    switch (addr & 0x00000FFF)
    {
    case 0x000:
        DispCnt = val;
        if (Num) DispCnt &= 0xC0B1FFF7;
        return;

    case 0x064:
        CaptureCnt = val & 0xEF3F1F1F;
        return;

    case 0x068:
        DispFIFO[DispFIFOWritePtr] = val & 0xFFFF;
        DispFIFO[DispFIFOWritePtr+1] = val >> 16;
        DispFIFOWritePtr += 2;
        DispFIFOWritePtr &= 0xF;
        return;
    }

    if (Enabled)
    {
        switch (addr & 0x00000FFF)
        {
        case 0x028:
            if (val & 0x08000000) val |= 0xF0000000;
            BGXRef[0] = val;
            if (GPU::VCount < 192) BGXRefInternal[0] = BGXRef[0];
            return;
        case 0x02C:
            if (val & 0x08000000) val |= 0xF0000000;
            BGYRef[0] = val;
            if (GPU::VCount < 192) BGYRefInternal[0] = BGYRef[0];
            return;

        case 0x038:
            if (val & 0x08000000) val |= 0xF0000000;
            BGXRef[1] = val;
            if (GPU::VCount < 192) BGXRefInternal[1] = BGXRef[1];
            return;
        case 0x03C:
            if (val & 0x08000000) val |= 0xF0000000;
            BGYRef[1] = val;
            if (GPU::VCount < 192) BGYRefInternal[1] = BGYRef[1];
            return;
        }
    }

    Write16(addr, val&0xFFFF);
    Write16(addr+2, val>>16);
}
void Unit::UpdateMosaicCounters(u32 line)
{
    // Y mosaic uses incrementing 4-bit counters
    // the transformed Y position is updated every time the counter matches the MOSAIC register

    if (OBJMosaicYCount == OBJMosaicSize[1])
    {
        OBJMosaicYCount = 0;
        OBJMosaicY = line + 1;
    }
    else
    {
        OBJMosaicYCount++;
        OBJMosaicYCount &= 0xF;
    }
}
void Unit::VBlankEnd()
{
    // TODO: find out the exact time this happens
    BGXRefInternal[0] = BGXRef[0];
    BGXRefInternal[1] = BGXRef[1];
    BGYRefInternal[0] = BGYRef[0];
    BGYRefInternal[1] = BGYRef[1];

    BGMosaicY = 0;
    BGMosaicYMax = BGMosaicSize[1];
    //OBJMosaicY = 0;
    //OBJMosaicYMax = OBJMosaicSize[1];
    //OBJMosaicY = 0;
    //OBJMosaicYCount = 0;
}
void Unit::CheckWindows(u32 line)
{
    line &= 0xFF;
    if (line == Win0Coords[3])      Win0Active &= ~0x1;
    else if (line == Win0Coords[2]) Win0Active |=  0x1;
    if (line == Win1Coords[3])      Win1Active &= ~0x1;
    else if (line == Win1Coords[2]) Win1Active |=  0x1;
}
void Unit::CalculateWindowMask(u32 line, u8* windowMask, u8* objWindow)
{
    for (u32 i = 0; i < 256; i++)
        windowMask[i] = WinCnt[2]; // window outside

    if (DispCnt & (1<<15))
    {
        // OBJ window
        for (int i = 0; i < 256; i++)
        {
            if (objWindow[i])
                windowMask[i] = WinCnt[3];
        }
    }

    if (DispCnt & (1<<14))
    {
        // window 1
        u8 x1 = Win1Coords[0];
        u8 x2 = Win1Coords[1];

        for (int i = 0; i < 256; i++)
        {
            if (i == x2)      Win1Active &= ~0x2;
            else if (i == x1) Win1Active |=  0x2;

            if (Win1Active == 0x3) windowMask[i] = WinCnt[1];
        }
    }

    if (DispCnt & (1<<13))
    {
        // window 0
        u8 x1 = Win0Coords[0];
        u8 x2 = Win0Coords[1];

        for (int i = 0; i < 256; i++)
        {
            if (i == x2)      Win0Active &= ~0x2;
            else if (i == x1) Win0Active |=  0x2;

            if (Win0Active == 0x3) windowMask[i] = WinCnt[0];
        }
    }
}
u16* Unit::GetBGExtPal(u32 slot, u32 pal)
{
    const u32 PaletteSize = 256 * 2;
    const u32 SlotSize = PaletteSize * 16;
    return (u16*)&(Num == 0
         ? GPU::VRAMFlat_ABGExtPal
         : GPU::VRAMFlat_BBGExtPal)[slot * SlotSize + pal * PaletteSize];
}
u16* Unit::GetOBJExtPal()
{
    return Num == 0
         ? (u16*)GPU::VRAMFlat_AOBJExtPal
         : (u16*)GPU::VRAMFlat_BOBJExtPal;
}
void Unit::GetBGVRAM(u8*& data, u32& mask)
{
    if (Num == 0)
    {
        data = GPU::VRAMFlat_ABG;
        mask = 0x7FFFF;
    }
    else
    {
        data = GPU::VRAMFlat_BBG;
        mask = 0x1FFFF;
    }
}
void Unit::GetOBJVRAM(u8*& data, u32& mask)
{
    if (Num == 0)
    {
        data = GPU::VRAMFlat_AOBJ;
        mask = 0x3FFFF;
    }
    else
    {
        data = GPU::VRAMFlat_BOBJ;
        mask = 0x1FFFF;
    }
}
}
