/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2014, Microsoft Corporation.

    File:

        XRGBSynthesizer.cpp

    Abstract:

        This file contains the implementation of CXRGBSynthesizer.

        CXRGBSynthesizer is derived from CSynthesizer.  It uses the RGB 
        color space and defines a Commit() function.  XRGB is known also
        known as RGB32.  It uses 8 bits per primary plus 8 bits of padding
        for 4 bytes per pixel in a single plane.

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
#pragma warning( disable:6101  )
_Success_(return > 0)
ULONG
CXRGBSynthesizer::
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

    //  In case stride isn't initialized.
    if( Stride==0 )
    {
        Stride = m_OutputStride;
    }

    if( !(  Buffer &&
            m_Buffer &&
            Size   >= sizeof(KS_RGBQUAD) &&
            m_Length >= sizeof(KS_RGBQUAD) &&
            (Stride%4)==0 &&
            (Size%4)==0
         ) )
    {
        NT_ASSERT(FALSE);
        return 0;
    }

    ULONG   OutputStride = min( Stride, (ULONG) m_OutputStride );
    ULONG   OutputSize   = min( Size, m_Length );
    ULONG   Limit = min( m_Height, OutputSize/Stride );

    //  Is the row order in the bitmap inverted?
    if( m_FlipVertical )
    {
        PUCHAR  Start = Buffer;
        for( ULONG y=Limit; y; y-- )
        {
            // Inverted row copy, with possible stride, width or height difference.
            RtlCopyMemory( Buffer, GetImageLocation( 0, y-1 ), OutputStride );
            Buffer += Stride;
        }
        return (ULONG) (Buffer - Start);
    }
    else
    {
        PUCHAR  Start = Buffer;

        if( Limit * OutputStride == OutputSize )
        {
            //  1:1 copy case.  (Path normally taken, so fastest.)
            RtlCopyMemory( Buffer, m_Buffer, OutputSize );
            return OutputSize;
        }
        else
        {
            for( ULONG y=0; y<Limit; y++ )
            {
                //  Normal copy, with possible stride, width or height difference.
                RtlCopyMemory( Buffer, GetImageLocation( 0, y ), OutputStride );
                Buffer += Stride;
            }
            return (ULONG) (Buffer - Start);
        }
    }
}
// suppressed due to Esp:773
#pragma warning (pop)

const UCHAR4 *
CXRGBSynthesizer::
GetPalette()
/*++

Routine Description:

    Get a pointer to an array of palette colors.  Used mostly to handle
    different color primaries.  Location of the primary must agree with
    Commit().

Arguments:

    none

Return Value:

    A pointer to an array of color primaries used for rendering.

--*/
{
    PAGED_CODE();

    static
    UCHAR4 Colors [MAX_COLOR] =
    {
        {0, 0, 0},          // BLACK
        {255, 255, 255},    // WHITE
        {0, 255, 255},      // YELLOW
        {255, 255, 0},      // CYAN
        {0, 255, 0},        // GREEN
        {255, 0, 255},      // MAGENTA
        {0, 0, 255},        // RED
        {255, 0, 0},        // BLUE
        {128, 128, 128}     // GREY
    };

    return Colors;
}

//  Macros used to do fixed-point arithmetic in initialization.
#define   TO_Q24_8bit_to_32bit(_x_)     (       (  (ULONG) (_x_) ) << 24 )
#define FROM_Q24_32bit_to_8bit(_x_)     ((UCHAR) (( (ULONG) (_x_) ) >> 24 ))

BOOLEAN
CXRGBSynthesizer::
Initialize()
/*++

Routine Description:

    Class initialization.

    Used to pre-initialize a bitmap that contains a gradient bar that starts
    with one of our rendering colors and fades to black.  Each row will contain
    the gradient for a single starting color.  To paint a gradient bar, simply
    replicate the row as many times as necessary.  
    
    This front-loads all gradient calculations to initialization and caches the 
    result, allowing us to render a gradient bar in the time it takes to copy
    the pixels even on a relatively slow machine.

Arguments:

    none

Return Value:

    TRUE - success.

--*/
{
    PAGED_CODE();

    //  First, do the base class initialization.
    BOOLEAN Status = CSynthesizer::Initialize();

    if( Status )
    {
        //  Now initialize the Gradient bmp...
        CKsRgbQuad  *Bmp = m_GradientBmp;

        for( ULONG color=BLACK; color<MAX_COLOR; color++ )
        {
            ULONG   HALF    = TO_Q24_8bit_to_32bit(1)>>1;   // round, don't truncate.
            ULONG   B       = TO_Q24_8bit_to_32bit(m_Colors[color][0])+HALF;
            ULONG   G       = TO_Q24_8bit_to_32bit(m_Colors[color][1])+HALF;
            ULONG   R       = TO_Q24_8bit_to_32bit(m_Colors[color][2])+HALF;
            ULONG   BDelta  = B-TO_Q24_8bit_to_32bit(m_Colors[BLACK][0]);
            ULONG   GDelta  = G-TO_Q24_8bit_to_32bit(m_Colors[BLACK][1]);
            ULONG   RDelta  = R-TO_Q24_8bit_to_32bit(m_Colors[BLACK][2]);

            BDelta /= (ULONG) m_Width;
            GDelta /= (ULONG) m_Width;
            RDelta /= (ULONG) m_Width;

            for(ULONG i = 0; i < m_Width; i++)
            {
                *Bmp++ = CKsRgbQuad(
                             FROM_Q24_32bit_to_8bit(R),
                             FROM_Q24_32bit_to_8bit(G),
                             FROM_Q24_32bit_to_8bit(B)
                         );
                B -= BDelta;
                G -= GDelta;
                R -= RDelta;
            }
        }
    }
    return Status;
}
