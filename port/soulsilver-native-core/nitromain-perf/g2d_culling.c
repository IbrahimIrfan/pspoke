/* Reused public Platinum render_view.c IsObjectInView, matched against
 * original SoulSilver sub_02025C98 assembly. Unsigned arithmetic makes
 * original 32-bit wrap explicit for extreme fixed-point inputs. */
#include <nitro.h>
#include <nnsys.h>
BOOL sub_02025C98(const NNSG2dCellData *cellData, const MtxFx32 *objectCoords, const NNSG2dViewRect *viewRect)
{
    const NNSG2dCellBoundingRectS16 *boundingRect = NNS_G2dGetCellBoundingRect(cellData);
    const fx32 boundingRadius = NNS_G2dGetCellBoundingSphereR(cellData);
    const fx32 originX = (fx32)((u32)objectCoords->_20 - (u32)viewRect->posTopLeft.x);
    const fx32 originY = (fx32)((u32)objectCoords->_21 - (u32)viewRect->posTopLeft.y);

    fx32 minY, maxY;
    fx32 minX, maxX;
    fx32 tmp;

    if (NNS_G2dCellHasBR(cellData) == TRUE) {
        minY = (fx32)((u32)(s32)boundingRect->minY << FX32_SHIFT);
        maxY = (fx32)((u32)(s32)boundingRect->maxY << FX32_SHIFT);
        minX = (fx32)((u32)(s32)boundingRect->minX << FX32_SHIFT);
        maxX = (fx32)((u32)(s32)boundingRect->maxX << FX32_SHIFT);
    } else {
        minY = (fx32)((0u-(u32)boundingRadius) << FX32_SHIFT);
        maxY = (fx32)((u32)boundingRadius << FX32_SHIFT);
        minX = (fx32)((0u-(u32)boundingRadius) << FX32_SHIFT);
        maxX = (fx32)((u32)boundingRadius << FX32_SHIFT);
    }

    minY = (fx32)((u32)FX_Mul(minY, objectCoords->_01) + (u32)FX_Mul(minY, objectCoords->_11) + (u32)originY);
    maxY = (fx32)((u32)FX_Mul(maxY, objectCoords->_01) + (u32)FX_Mul(maxY, objectCoords->_11) + (u32)originY);
    minX = (fx32)((u32)FX_Mul(minX, objectCoords->_00) + (u32)FX_Mul(minX, objectCoords->_10) + (u32)originX);
    maxX = (fx32)((u32)FX_Mul(maxX, objectCoords->_00) + (u32)FX_Mul(maxX, objectCoords->_10) + (u32)originX);

    if (maxY < minY) {
        tmp = maxY;
        maxY = minY;
        minY = tmp;
    }

    if (maxX < minX) {
        tmp = maxX;
        maxX = minX;
        minX = tmp;
    }

    return maxY > 0 && minY < viewRect->sizeView.y && maxX > 0 && minX < viewRect->sizeView.x;
}
