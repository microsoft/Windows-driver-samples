/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2014, Microsoft Corporation.

    File:

        YUVSynthesizer.cpp

    Abstract:

        This file contains the implementation of CYUY2Synthesizer.

        CYUY2Synthesizer is derived from CYUVSynthesizer.  An image 
        synthesizer for YUY2 format.  It uses the YUV color space and defines 
        a Commit() function.
        
        YUY2 is a 4:2:2 format that uses a single plane that decimates chroma 
        samples on the x axis with an effective bit rate of 16 bits per pixel.
        A plane is an array of rows.  Rows consist of pairs of pixels in the 
        sequence: Y0|U|Y1|V.

        The synthesizer stores the YUV information in 32 bit format internally
        for speed and accuracy.  The Commit function does the format conversion.

    History:

        created 4/14/2014

**************************************************************************/

#include "Common.h"

/**************************************************************************

    PAGED CODE

**************************************************************************/

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA

// suppressed due to Esp:773
#pragma warning (push)
#pragma warning( disable:26015 )    // Suppress OACR error.  Seems nonsensical.  TODO: Must revisit.
#pragma warning( disable:26019 )
_Success_(return > 0)
ULONG
CYUY2Synthesizer::
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
    PAGED_CODE();

    NT_ASSERT( Buffer );
    NT_ASSERT( m_Buffer );
    NT_ASSERT( Size >= Stride * m_Height );
    NT_ASSERT( (Stride&3)==0 );
    NT_ASSERT( (m_Width&1) == 0 );
    NT_ASSERT( (m_Height&1) == 0 );
    NT_ASSERT( m_Width>1 && m_Height>1 );
    NT_ASSERT( (Size&1)==0 );

    //  In most cases, stride isn't initialized.
    //  If so, just assume its the Width.
    if( Stride==0 )
    {
        Stride = m_OutputStride;
    }

    if( !(  Buffer &&
            m_Buffer &&
            Size >= Stride * m_Height &&
            (Stride&3)==0 &&
            (m_Width&1) == 0 &&
            (m_Height&1) == 0 &&
            m_Width>1 && m_Height>1 &&
            (Size&1)==0
         ) )
    {
        //  Wipe the destination before returning failure.
        return 0;
    }

    PKS_RGBQUAD pSrc = (PKS_RGBQUAD) m_Buffer;
    ULONG   RowLimit = min( m_Height, Size / Stride );
    ULONG   ColLimit = min( m_Width,  Stride/sizeof(WORD) ) & ~0x1;

    for(ULONG row = 0; row < RowLimit; row++)
    {
        PDWORD  pY  = (PDWORD) &Buffer[ row * Stride ];

        for(ULONG col = 0; col < ColLimit; col+=2)
        {
            KS_RGBQUAD  L = pSrc[col+0];
            KS_RGBQUAD  R = pSrc[col+1];

            *pY++ =
                MAKELONG( MAKEWORD( L.rgbGreen, (ULONG(L.rgbBlue) + R.rgbBlue)/2),
                          MAKEWORD( R.rgbGreen, (ULONG(L.rgbRed ) + R.rgbRed )/2) );
        }
        pSrc = (PKS_RGBQUAD) (((PUCHAR) pSrc) + m_SynthesisStride);
    }

    return RowLimit * Stride;
}
// suppressed due to Esp:773
#pragma warning (pop)
