/*****************************************************************************\
     Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#include "filter_epx_unsafe.h"

#undef  AVERAGE_565
#define AVERAGE_565(el0, el1) (((el0) & (el1)) + ((((el0) ^ (el1)) & 0xF7DE) >> 1))

/* Allows vertical overlap. We need this to avoid seams when threading */
void EPX_16_unsafe (uint8_t *srcPtr,
                    int srcPitch,
                    uint8_t *dstPtr,
                    int dstPitch,
                    int width,
                    int height)
{
    uint16_t  colorX, colorA, colorB, colorC, colorD;
    uint16_t  *sP, *uP, *lP;
    uint32_t  *dP1, *dP2;
    int     w;

    for (; height; height--)
    {
        sP  = (uint16_t *) srcPtr;
        uP  = (uint16_t *) (srcPtr - srcPitch);
        lP  = (uint16_t *) (srcPtr + srcPitch);
        dP1 = (uint32_t *) dstPtr;
        dP2 = (uint32_t *) (dstPtr + dstPitch);

        // left edge

        colorX = *sP;
        colorC = *++sP;
        colorB = *lP++;
        colorD = *uP++;

        if ((colorX != colorC) && (colorB != colorD))
        {
        #ifdef __BIG_ENDIAN__
            *dP1 = (colorX << 16) + ((colorC == colorD) ? colorC : colorX);
            *dP2 = (colorX << 16) + ((colorB == colorC) ? colorB : colorX);
        #else
            *dP1 = colorX + (((colorC == colorD) ? colorC : colorX) << 16);
            *dP2 = colorX + (((colorB == colorC) ? colorB : colorX) << 16);
        #endif
        }
        else
            *dP1 = *dP2 = (colorX << 16) + colorX;

        dP1++;
        dP2++;

        //

        for (w = width - 2; w; w--)
        {
            colorA = colorX;
            colorX = colorC;
            colorC = *++sP;
            colorB = *lP++;
            colorD = *uP++;

            if ((colorA != colorC) && (colorB != colorD))
            {
            #ifdef __BIG_ENDIAN__
                *dP1 = (((colorD == colorA) ? colorD : colorX) << 16) + ((colorC == colorD) ? colorC : colorX);
                *dP2 = (((colorA == colorB) ? colorA : colorX) << 16) + ((colorB == colorC) ? colorB : colorX);
            #else
                *dP1 = ((colorD == colorA) ? colorD : colorX) + (((colorC == colorD) ? colorC : colorX) << 16);
                *dP2 = ((colorA == colorB) ? colorA : colorX) + (((colorB == colorC) ? colorB : colorX) << 16);
            #endif
            }
            else
                *dP1 = *dP2 = (colorX << 16) + colorX;

            dP1++;
            dP2++;
        }

        // right edge

        colorA = colorX;
        colorX = colorC;
        colorB = *lP;
        colorD = *uP;

        if ((colorA != colorX) && (colorB != colorD))
        {
        #ifdef __BIG_ENDIAN__
            *dP1 = (((colorD == colorA) ? colorD : colorX) << 16) + colorX;
            *dP2 = (((colorA == colorB) ? colorA : colorX) << 16) + colorX;
        #else
            *dP1 = ((colorD == colorA) ? colorD : colorX) + (colorX << 16);
            *dP2 = ((colorA == colorB) ? colorA : colorX) + (colorX << 16);
        #endif
        }
        else
            *dP1 = *dP2 = (colorX << 16) + colorX;

        srcPtr += srcPitch;
        dstPtr += dstPitch << 1;
    }
}

/* Blends with edge pixel instead of just using it directly. */
void EPX_16_smooth_unsafe (uint8_t *srcPtr,
                           int srcPitch,
                           uint8_t *dstPtr,
                           int dstPitch,
                           int width,
                           int height)
{
    uint16_t  colorX, colorA, colorB, colorC, colorD;
    uint16_t  *sP, *uP, *lP;
    uint32_t  *dP1, *dP2;
    int     w;

    for (; height; height--)
    {
        sP  = (uint16_t *) srcPtr;
        uP  = (uint16_t *) (srcPtr - srcPitch);
        lP  = (uint16_t *) (srcPtr + srcPitch);
        dP1 = (uint32_t *) dstPtr;
        dP2 = (uint32_t *) (dstPtr + dstPitch);

        // left edge

        colorX = *sP;
        colorC = *++sP;
        colorB = *lP++;
        colorD = *uP++;

        if ((colorX != colorC) && (colorB != colorD))
        {
        #ifdef __BIG_ENDIAN__
            *dP1 = (colorX << 16) + ((colorC == colorD) ? AVERAGE_565 (colorC, colorX) : colorX);
            *dP2 = (colorX << 16) + ((colorB == colorC) ? AVERAGE_565 (colorB, colorX) : colorX);
        #else
            *dP1 = colorX + (((colorC == colorD) ? AVERAGE_565 (colorC, colorX) : colorX) << 16);
            *dP2 = colorX + (((colorB == colorC) ? AVERAGE_565 (colorB, colorX) : colorX) << 16);
        #endif
        }
        else
            *dP1 = *dP2 = (colorX << 16) + colorX;

        dP1++;
        dP2++;

        //

        for (w = width - 2; w; w--)
        {
            colorA = colorX;
            colorX = colorC;
            colorC = *++sP;
            colorB = *lP++;
            colorD = *uP++;

            if ((colorA != colorC) && (colorB != colorD))
            {
            #ifdef __BIG_ENDIAN__
                *dP1 = (((colorD == colorA) ? AVERAGE_565 (colorD, colorX) : colorX) << 16) + ((colorC == colorD) ? AVERAGE_565 (colorC, colorX) : colorX);
                *dP2 = (((colorA == colorB) ? AVERAGE_565 (colorA, colorX) : colorX) << 16) + ((colorB == colorC) ? AVERAGE_565 (colorB, colorX) : colorX);
            #else
                *dP1 = ((colorD == colorA) ? AVERAGE_565 (colorD, colorX) : colorX) + (((colorC == colorD) ? AVERAGE_565 (colorC, colorX) : colorX) << 16);
                *dP2 = ((colorA == colorB) ? AVERAGE_565 (colorA, colorX) : colorX) + (((colorB == colorC) ? AVERAGE_565 (colorB, colorX) : colorX) << 16);
            #endif
            }
            else
                *dP1 = *dP2 = (colorX << 16) + colorX;

            dP1++;
            dP2++;
        }

        // right edge

        colorA = colorX;
        colorX = colorC;
        colorB = *lP;
        colorD = *uP;

        if ((colorA != colorX) && (colorB != colorD))
        {
        #ifdef __BIG_ENDIAN__
            *dP1 = (((colorD == colorA) ? AVERAGE_565 (colorD, colorX) : colorX) << 16) + colorX;
            *dP2 = (((colorA == colorB) ? AVERAGE_565 (colorA, colorX) : colorX) << 16) + colorX;
        #else
            *dP1 = ((colorD == colorA) ? AVERAGE_565 (colorD, colorX) : colorX) + (colorX << 16);
            *dP2 = ((colorA == colorB) ? AVERAGE_565 (colorA, colorX) : colorX) + (colorX << 16);
        #endif
        }
        else
            *dP1 = *dP2 = (colorX << 16) + colorX;

        srcPtr += srcPitch;
        dstPtr += dstPitch << 1;
    }
}
