/******************************Module*Header*******************************\
 * Module Name: BltFuncs.cxx
 *
 * Basic Display Driver copying functionality
 *
 *
 * Copyright (c) 2010 Microsoft Corporation
\**************************************************************************/

#include "BDD.hxx"

// For the following macros, c must be a UCHAR.
#define UPPER_6_BITS(c)   (((c) & rMaskTable[6 - 1]) >> 2)
#define UPPER_5_BITS(c)   (((c) & rMaskTable[5 - 1]) >> 3)
#define LOWER_6_BITS(c)   (((BYTE)(c)) & lMaskTable[BITS_PER_BYTE - 6])
#define LOWER_5_BITS(c)   (((BYTE)(c)) & lMaskTable[BITS_PER_BYTE - 5])


#define SHIFT_FOR_UPPER_5_IN_565   (6 + 5)
#define SHIFT_FOR_MIDDLE_6_IN_565  (5)
#define SHIFT_UPPER_5_IN_565_BACK  ((BITS_PER_BYTE * 2) + (BITS_PER_BYTE - 5))
#define SHIFT_MIDDLE_6_IN_565_BACK ((BITS_PER_BYTE * 1) + (BITS_PER_BYTE - 6))
#define SHIFT_LOWER_5_IN_565_BACK  ((BITS_PER_BYTE * 0) + (BITS_PER_BYTE - 5))

// For the following macros, pPixel must be a BYTE* pointing to the start of a 32 bit pixel
#define CONVERT_32BPP_TO_16BPP(pPixel) ((UPPER_5_BITS(pPixel[2]) << SHIFT_FOR_UPPER_5_IN_565)  | \
                                        (UPPER_6_BITS(pPixel[1]) << SHIFT_FOR_MIDDLE_6_IN_565) | \
                                        (UPPER_5_BITS(pPixel[0])))

// 8bpp is done with 6 levels per color channel since this gives true grays, even if it leaves 40 empty palette entries
// The 6 levels per color is the reason for dividing below by 43 (43 * 6 == 258, closest multiple of 6 to 256)
// It is also the reason for multiplying the red channel by 36 (== 6*6) and the green channel by 6, as this is the
// equivalent to bit shifting in a 3:3:2 model. Changes to this must be reflected in vesasup.cxx with the Blues/Greens/Reds arrays
#define CONVERT_32BPP_TO_8BPP(pPixel) (((pPixel[2] / 43) * 36) + \
                                       ((pPixel[1] / 43) * 6) + \
                                       ((pPixel[0] / 43)))

// 4bpp is done with strict grayscale since this has been found to be usable
// 30% of the red value, 59% of the green value, and 11% of the blue value is the standard way to convert true color to grayscale
#define CONVERT_32BPP_TO_4BPP(pPixel) ((BYTE)(((pPixel[2] * 30) + \
                                               (pPixel[1] * 59) + \
                                               (pPixel[0] * 11)) / (100 * 16)))


// For the following macro, Pixel must be a WORD representing a 16 bit pixel
#define CONVERT_16BPP_TO_32BPP(Pixel) (((ULONG)LOWER_5_BITS((Pixel) >> SHIFT_FOR_UPPER_5_IN_565) << SHIFT_UPPER_5_IN_565_BACK) | \
                                       ((ULONG)LOWER_6_BITS((Pixel) >> SHIFT_FOR_MIDDLE_6_IN_565) << SHIFT_MIDDLE_6_IN_565_BACK) | \
                                       ((ULONG)LOWER_5_BITS((Pixel)) << SHIFT_LOWER_5_IN_565_BACK))

#pragma code_seg(push)
#pragma code_seg()
// BEGIN: Non-Paged Code

// Bit is 1 from Idx to end of byte, with bit count starting at high order
BYTE lMaskTable[BITS_PER_BYTE] = {0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01};

// Bit is 1 from Idx to start of byte, with bit count starting at high order
BYTE rMaskTable[BITS_PER_BYTE] = {0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff};

// Bit of Idx is 1, with bit count starting at high order
BYTE PixelMask[BITS_PER_BYTE]  = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};

/****************************Internal*Routine******************************\
 * CopyBits32_32
 *
 *
 * Copies rectangles from one surface to another. Both surfaces must have
 * the same resolution.
 *
 * OffsetX, OffsetY - to add to the rectangle coordinates.
 *
 * Copied from %SDXROOT%\windows\Core\dxkernel\cdd\enable.cxx (CopySurfBits)
 *
\**************************************************************************/

VOID CopyBits32_32(
    BLT_INFO* pDst,
    CONST BLT_INFO* pSrc,
    UINT  NumRects,
    _In_reads_(NumRects) CONST RECT *pRects)
{
    NT_ASSERT((pDst->BitsPerPel == 32) &&
              (pSrc->BitsPerPel == 32));
    NT_ASSERT((pDst->Rotation == D3DKMDT_VPPR_IDENTITY) &&
              (pSrc->Rotation == D3DKMDT_VPPR_IDENTITY));

    for (UINT iRect = 0; iRect < NumRects; iRect++)
    {
        CONST RECT* pRect = &pRects[iRect];

        NT_ASSERT(pRect->right >= pRect->left);
        NT_ASSERT(pRect->bottom >= pRect->top);

        UINT NumPixels = pRect->right - pRect->left;
        UINT NumRows = pRect->bottom - pRect->top;
        UINT BytesToCopy = NumPixels * 4;
        BYTE* pStartDst = ((BYTE*)pDst->pBits +
                          (pRect->top + pDst->Offset.y) * pDst->Pitch +
                          (pRect->left + pDst->Offset.x) * 4);
        CONST BYTE* pStartSrc = ((BYTE*)pSrc->pBits +
                                (pRect->top + pSrc->Offset.y) * pSrc->Pitch +
                                (pRect->left + pSrc->Offset.x) * 4);

        for (UINT i = 0; i < NumRows; ++i)
        {
            RtlCopyMemory(pStartDst, pStartSrc, BytesToCopy);
            pStartDst += pDst->Pitch;
            pStartSrc += pSrc->Pitch;
        }
    }
}


VOID GetPitches(_In_ CONST BLT_INFO* pBltInfo, _Out_ LONG* pPixelPitch, _Out_ LONG* pRowPitch)
{
    switch (pBltInfo->Rotation)
    {
        case D3DKMDT_VPPR_IDENTITY:
        {
            *pPixelPitch = (pBltInfo->BitsPerPel / BITS_PER_BYTE);
            *pRowPitch = pBltInfo->Pitch;
            return;
        }
        case D3DKMDT_VPPR_ROTATE90:
        {
            *pPixelPitch = -((LONG)pBltInfo->Pitch);
            *pRowPitch = (pBltInfo->BitsPerPel / BITS_PER_BYTE);
            return;
        }
        case D3DKMDT_VPPR_ROTATE180:
        {
            *pPixelPitch = -((LONG)pBltInfo->BitsPerPel / BITS_PER_BYTE);
            *pRowPitch = -((LONG)pBltInfo->Pitch);
            return;
        }
        case D3DKMDT_VPPR_ROTATE270:
        {
            *pPixelPitch = pBltInfo->Pitch;
            *pRowPitch = -((LONG)pBltInfo->BitsPerPel / BITS_PER_BYTE);
            return;
        }
        default:
        {
            BDD_LOG_ASSERTION1("Invalid rotation (0x%I64x) specified", pBltInfo->Rotation);
            *pPixelPitch = 0;
            *pRowPitch = 0;
            return;
        }
    }
}

BYTE* GetRowStart(_In_ CONST BLT_INFO* pBltInfo, CONST RECT* pRect)
{
    BYTE* pRet = NULL;
    LONG OffLeft = pRect->left + pBltInfo->Offset.x;
    LONG OffTop = pRect->top + pBltInfo->Offset.y;
    LONG BytesPerPixel = (pBltInfo->BitsPerPel / BITS_PER_BYTE);
    switch (pBltInfo->Rotation)
    {
        case D3DKMDT_VPPR_IDENTITY:
        {
            pRet = ((BYTE*)pBltInfo->pBits +
                           OffTop * pBltInfo->Pitch +
                           OffLeft * BytesPerPixel);
            break;
        }
        case D3DKMDT_VPPR_ROTATE90:
        {
            pRet = ((BYTE*)pBltInfo->pBits +
                           (pBltInfo->Height - 1 - OffLeft) * pBltInfo->Pitch +
                           OffTop * BytesPerPixel);
            break;
        }
        case D3DKMDT_VPPR_ROTATE180:
        {
            pRet = ((BYTE*)pBltInfo->pBits +
                           (pBltInfo->Height - 1 - OffTop) * pBltInfo->Pitch +
                           (pBltInfo->Width - 1 - OffLeft) * BytesPerPixel);
            break;
        }
        case D3DKMDT_VPPR_ROTATE270:
        {
            pRet = ((BYTE*)pBltInfo->pBits +
                           OffLeft * pBltInfo->Pitch +
                           (pBltInfo->Width - 1 - OffTop) * BytesPerPixel);
            break;
        }
        default:
        {
            BDD_LOG_ASSERTION1("Invalid rotation (0x%I64x) specified", pBltInfo->Rotation);
            break;
        }
    }

    return pRet;
}

/****************************Internal*Routine******************************\
 * CopyBitsGeneric
 *
 *
 * Blt function which can handle a rotated dst/src, offset rects in dst/src
 * and bpp combinations of:
 *   dst | src
 *    32 | 32   // For identity rotation this is much faster in CopyBits32_32
 *    32 | 24
 *    32 | 16
 *    24 | 32
 *    16 | 32
 *     8 | 32
 *    24 | 24   // untested
 *
\**************************************************************************/

VOID CopyBitsGeneric(
    BLT_INFO* pDst,
    CONST BLT_INFO* pSrc,
    UINT  NumRects,
    _In_reads_(NumRects) CONST RECT *pRects)
{
    LONG DstPixelPitch = 0;
    LONG DstRowPitch = 0;
    LONG SrcPixelPitch = 0;
    LONG SrcRowPitch = 0;

    GetPitches(pDst, &DstPixelPitch, &DstRowPitch);
    GetPitches(pSrc, &SrcPixelPitch, &SrcRowPitch);

    for (UINT iRect = 0; iRect < NumRects; iRect++)
    {
        CONST RECT* pRect = &pRects[iRect];

        NT_ASSERT(pRect->right >= pRect->left);
        NT_ASSERT(pRect->bottom >= pRect->top);

        UINT NumPixels = pRect->right - pRect->left;
        UINT NumRows = pRect->bottom - pRect->top;

        BYTE* pDstRow = GetRowStart(pDst, pRect);
        CONST BYTE* pSrcRow = GetRowStart(pSrc, pRect);

        for (UINT y=0; y < NumRows; y++)
        {
            BYTE* pDstPixel = pDstRow;
            CONST BYTE* pSrcPixel = pSrcRow;

            for (UINT x=0; x < NumPixels; x++)
            {
                if ((pDst->BitsPerPel == 24) ||
                    (pSrc->BitsPerPel == 24))
                {
                    pDstPixel[0] = pSrcPixel[0];
                    pDstPixel[1] = pSrcPixel[1];
                    pDstPixel[2] = pSrcPixel[2];
                    // pPixel[3] is the alpha channel and is ignored for whichever of Src/Dst is 32bpp
                }
                else if (pDst->BitsPerPel == 32)
                {
                    if (pSrc->BitsPerPel == 32)
                    {
                        UINT32* pDstPixelAs32 = (UINT32*)pDstPixel;
                        UINT32* pSrcPixelAs32 = (UINT32*)pSrcPixel;
                        *pDstPixelAs32 = *pSrcPixelAs32;
                    }
                    else if (pSrc->BitsPerPel == 16)
                    {
                        UINT32* pDstPixelAs32 = (UINT32*)pDstPixel;
                        UINT16* pSrcPixelAs16 = (UINT16*)pSrcPixel;

                        *pDstPixelAs32 = CONVERT_16BPP_TO_32BPP(*pSrcPixelAs16);
                    }
                    else
                    {
                        // Invalid pSrc->BitsPerPel on a pDst->BitsPerPel of 32
                        NT_ASSERT(FALSE);
                    }
                }
                else if (pDst->BitsPerPel == 16)
                {
                    NT_ASSERT(pSrc->BitsPerPel == 32);

                    UINT16* pDstPixelAs16 = (UINT16*)pDstPixel;
                    *pDstPixelAs16 = CONVERT_32BPP_TO_16BPP(pSrcPixel);
                }
                else if (pDst->BitsPerPel == 8)
                {
                    NT_ASSERT(pSrc->BitsPerPel == 32);

                    *pDstPixel = CONVERT_32BPP_TO_8BPP(pSrcPixel);
                }
                else
                {
                    // Invalid pDst->BitsPerPel
                    NT_ASSERT(FALSE);
                }
                pDstPixel += DstPixelPitch;
                pSrcPixel += SrcPixelPitch;
            }

            pDstRow += DstRowPitch;
            pSrcRow += SrcRowPitch;
        }
    }
}

/****************************Internal*Routine******************************\
 * BltBits
 *
 *
 * Logic to decide which of the above functions to call based on Rotation/BPP
 *
\**************************************************************************/
VOID BltBits(
    BLT_INFO* pDst,
    CONST BLT_INFO* pSrc,
    UINT  NumRects,
    _In_reads_(NumRects) CONST RECT *pRects)
{
    // pSrc->pBits might be coming from user-mode. User-mode addresses when accessed by kernel need to be protected by a __try/__except.
    // This usage is redundant in the sample driver since it is already being used for MmProbeAndLockPages. However, it is very important
    // to have this in place and to make sure developers don't miss it, it is in these two locations.
    __try
    {
        if (pDst->BitsPerPel == 32 &&
            pSrc->BitsPerPel == 32 &&
            pDst->Rotation == D3DKMDT_VPPR_IDENTITY &&
            pSrc->Rotation == D3DKMDT_VPPR_IDENTITY)
        {
            // This is by far the most common copy function being called
            CopyBits32_32(pDst, pSrc, NumRects, pRects);
        }
        else
        {
            CopyBitsGeneric(pDst, pSrc, NumRects, pRects);
        }
    }
    #pragma prefast(suppress: __WARNING_EXCEPTIONEXECUTEHANDLER, "try/except is only able to protect against user-mode errors and these are the only errors we try to catch here");
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        BDD_LOG_ERROR2("Either dst (0x%I64x) or src (0x%I64x) bits encountered exception during access.", pDst->pBits, pSrc->pBits);
    }
}

// END: Non-Paged Code
#pragma code_seg(pop)

