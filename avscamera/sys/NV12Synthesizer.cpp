/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2001, Microsoft Corporation.

    File:

        CNV12Synthesizer.h

    Abstract:

        This file contains the implementation of CNV12Synthesizer.
        
        CNV12Synthesizer is derived from CYUVSynthesizer.  It uses the YUV color
        space and defines a Commit() function that reformats pixels into the 
        NV12 format.  From MSDN:

            A format in which all Y samples are found first in memory as an 
            array of unsigned char with an even number of lines (possibly with 
            a larger stride for memory alignment). This is followed immediately 
            by an array of unsigned char containing interleaved Cb and Cr 
            samples. If these samples are addressed as a little-endian WORD 
            type, Cb would be in the least significant bits and Cr would be in 
            the most significant bits with the same total stride as the Y 
            samples. NV12 is the preferred 4:2:0 pixel format.

        A visual representation of the layout:
            YYYY
            YYYY
            UVUV


    History:
        created 4/14/2013

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
CNV12Synthesizer::
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
    //  We assume the output stride is the same as the original width here.
    UNREFERENCED_PARAMETER(Stride);

    //  The code actually handles this gracefully.  It rounds down.
    NT_ASSERT( (m_Width&1) == 0 );
    NT_ASSERT( (m_Height&1) == 0 );

    //  These are impossible conditions.
    if( !(  Buffer &&
            m_Buffer &&
            (Size%6)==0 &&  //  Should be evenly divisible by 1 macropixel
            Size>=6         //  At least 1 macropixel
         ) )
    {
        NT_ASSERT(FALSE);
        return 0;
    }

    ULONG   MacroPixelsWide = m_Width/2;

    //  Impossible.
    if( MacroPixelsWide==0 )
    {
        NT_ASSERT(FALSE);
        return 0;
    }

    //  At most, 2/3rds of the available space is used by the Y plane.
    //  Notice that we limit the number of macro pixel rows to what will
    //  fit in the space available, no matter what is in the original.
    ULONG   Y_limit = (Size /3) * 2;
    ULONG   MacroPixelsHigh = min( m_Height, Y_limit/m_Width)/2;

    //  Impossible.
    if( MacroPixelsHigh==0 )
    {
        NT_ASSERT(FALSE);
        return 0;
    }

    //  Now we work back to arrive at the Y & UV plane sizes.
    ULONG   UV_size  = MacroPixelsWide * MacroPixelsHigh *2;
    ULONG   Y_size   = MacroPixelsWide * MacroPixelsHigh *4;
    ULONG   YUV_size = Y_size + UV_size;

    //  Impossible.
    if( Size < YUV_size )
    {
        NT_ASSERT(FALSE);
        return 0;
    }

    PWORD   pY  = (PWORD) Buffer;
    PWORD   pUV = (PWORD) (Buffer + Y_size);

    for(ULONG row = 0; row < MacroPixelsHigh; row++)
    {
        PKS_RGBQUAD pSrc = (PKS_RGBQUAD) GetImageLocation(0, row*2);
        for(ULONG col = 0; col < MacroPixelsWide; col++)
        {
            KS_RGBQUAD  TL = pSrc[0];
            KS_RGBQUAD  TR = pSrc[1];
            KS_RGBQUAD  BL = pSrc[m_Width+0];
            KS_RGBQUAD  BR = pSrc[m_Width+1];
            pSrc += 2;

            //  Copy luma first
            pY[0]               = MAKEWORD(TL.rgbGreen, TR.rgbGreen);
            pY[MacroPixelsWide] = MAKEWORD(BL.rgbGreen, BR.rgbGreen);
            pY++;

            LONG
            tU  = TL.rgbBlue;    //top left
            tU += BL.rgbBlue;    //bottom left
            tU += TR.rgbBlue;    //top right
            tU += BR.rgbBlue;    //bottom right

            LONG
            tV  = TL.rgbRed;     //top left
            tV += BL.rgbRed;     //bottom left
            tV += TR.rgbRed;     //top right
            tV += BR.rgbRed;     //bottom right

            //  Copy decimated chroma
            *pUV++ = MAKEWORD(((tU+2)>>2), ((tV+2)>>2));
        }
        //  Skip a scan line, since we've already copied it.
        pY += MacroPixelsWide;
    }

    return (ULONG) (((PUCHAR) pUV) - Buffer);
}
// suppressed due to Esp:773
#pragma warning (pop)

