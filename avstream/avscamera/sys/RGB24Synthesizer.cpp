/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2014, Microsoft Corporation.

    File:

        RGB24Synthesizer.cpp

    Abstract:

        This file contains the implementation of CRGB24Synthesizer.
        
        CRGB24Synthesizer is derived from CRGBSynthesizer.  It uses the RGB 
        color space and defines a Commit() function that reformats pixels into 
        the RGB24 format.  RGB24 uses 8 bits per primary and 3 bytes per pixel 
        in a single plane.

    History:

        created 4/14/2014

**************************************************************************/

#include "Common.h"

/**************************************************************************

    LOCKED CODE

**************************************************************************/

#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif // ALLOC_PRAGMA

// suppressed due to Esp:773
#pragma warning (push)
#pragma warning( disable:26015 )    // Suppress OACR error.  Seems nonsensical.  TODO: Must revisit.
#pragma warning( disable:26019 )
_Success_(return > 0)
ULONG
CRGB24Synthesizer::
Commit(
    _Out_writes_bytes_(Size)
    PUCHAR  Buffer,
    _In_    ULONG   Size,
    _In_    ULONG   Stride
)
/*++

Routine Description:

    Copy (and reformat, if necessary) pixels from the internal scratch
    buffer.  If the output format decimates chrominance, do it here.

Arguments:

    Buffer -
        The output buffer to fill.

    Size -
        The size of the output buffer in bytes.

    Stride -
        The length of a row in bytes.

Return Value:

    Number of bytes copied into Buffer.

--*/
{
    //  In case stride isn't initialized.
    if( Stride==0 )
    {
        Stride = m_OutputStride;
    }

    //  These are impossible conditions.
    if( !(  Buffer &&
            m_Buffer &&
            (Size&3)==0 &&      // the rows are dword aligned, so the size must be too.
            Size>3 &&           // there must be room for at least 1 pixel
            (Stride&3)==0 &&    // rows should be DWORD aligned
            Stride>3 &&         // rows should contain at least 1 pixel
            m_Height>0 &&
            m_Width>0
         ) )
    {
        NT_ASSERT(FALSE);
        return 0;
    }

    ULONG   limit = min( Size/Stride, m_Height );
    for( ULONG row=0; row<limit; row++ )
    {
        PDWORD  pSrc = (PDWORD) GetImageLocation( 0, m_FlipVertical ? (m_Height - row -1) : row );
        PUCHAR  pDst = Buffer + (row * Stride);
        ULONG   col  = min( m_Width, Stride/3 );

        while( col )
        {
            //  This optimizes the copy fairly well on most machines.
            if( col>=4 )
            {
                DWORD   P0 = pSrc[0];
                DWORD   P1 = pSrc[1];
                DWORD   P2 = pSrc[2];
                DWORD   P3 = pSrc[3];
                ((PDWORD) pDst)[0] = ((P0      ) & 0x00FFFFFF) | (P1 << 24);
                ((PDWORD) pDst)[1] = ((P1 >>  8) & 0x0000FFFF) | (P2 << 16);
                ((PDWORD) pDst)[2] = ((P2 >> 16) & 0x000000FF) | (P3 <<  8);
                pDst += 4 * 3;
                pSrc += 4;
                col  -= 4;
            }
            else
            {
                DWORD   P0 = *pSrc++;
                *pDst++ = (UCHAR) (P0);
                *pDst++ = (UCHAR) (P0 >> 8);
                *pDst++ = (UCHAR) (P0 >> 16);
                col--;
            }
        }
    }

    return limit * Stride;
}
// suppressed due to Esp:773
#pragma warning (pop)
