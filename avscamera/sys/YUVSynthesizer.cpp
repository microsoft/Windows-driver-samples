/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2014, Microsoft Corporation.

    File:

        YUVSynthesizer.cpp

    Abstract:

        This file contains the implementation of CYUVSynthesizer.

        A Base image synthesizer for all YUV formats.  It provides a YUV color
        palette and sets up cached gradient bars.

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

const UCHAR4 *
CYUVSynthesizer::
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
        {128, 16, 128},     // BLACK
        {128, 235, 128},    // WHITE
        {16, 211, 146},     // YELLOW
        {166, 170, 16},     // CYAN
        {54, 145, 34},      // GREEN
        {202, 106, 222},    // MAGENTA
        {90, 81, 240},      // RED
        {240, 41, 109},     // BLUE
        {128, 125, 128},    // GREY
    };

    return Colors;
};

//  Macros used to do fixed-point arithmetic in initialization.
#define   TO_Q24_8bit_to_32bit_signed(_x_)     (        (  ( LONG) (_x_) ) << 24 )
#define FROM_Q24_32bit_to_8bit_signed(_x_)     (( CHAR) (( ( LONG) (_x_) ) >> 24 ))
#define   TO_Q24_8bit_to_32bit_unsigned(_x_)   (        (  (ULONG) (_x_) ) << 24 )
#define FROM_Q24_32bit_to_8bit_unsigned(_x_)   ((UCHAR) (( (ULONG) (_x_) ) >> 24 ))

BOOLEAN
CYUVSynthesizer::
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
        //  {54, 145, 34},      // GREEN to {128, 16, 128},     // BLACK
        for( ULONG color=BLACK; color<MAX_COLOR; color++ )
        {
            //  Must treat chroma as signed & luma as unsigned here.
            ULONG   HALF     = TO_Q24_8bit_to_32bit_unsigned(1)>>1;   // round, don't truncate.
            LONG    U        = TO_Q24_8bit_to_32bit_signed  (m_Colors[color][0])+HALF;
            ULONG   Y        = TO_Q24_8bit_to_32bit_unsigned(m_Colors[color][1])+HALF;
            LONG    V        = TO_Q24_8bit_to_32bit_signed  (m_Colors[color][2])+HALF;
            LONG    UDelta   = U-TO_Q24_8bit_to_32bit_signed  (m_Colors[BLACK][0]);
            ULONG   YDelta   = Y-TO_Q24_8bit_to_32bit_unsigned(m_Colors[BLACK][1]);
            LONG    VDelta   = V-TO_Q24_8bit_to_32bit_signed  (m_Colors[BLACK][2]);

            UDelta /= (LONG) m_Width;
            YDelta /= (LONG) m_Width;
            VDelta /= (LONG) m_Width;

            for(ULONG i = 0; i < m_Width; i++)
            {
                *Bmp++ = CKsRgbQuad(
                             FROM_Q24_32bit_to_8bit_signed  (V),
                             FROM_Q24_32bit_to_8bit_unsigned(Y),
                             FROM_Q24_32bit_to_8bit_signed  (U)
                         );
                U -= UDelta;
                Y -= YDelta;
                V -= VDelta;
            }
        }
    }
    return Status;
}
