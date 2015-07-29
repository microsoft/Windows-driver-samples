/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2014, Microsoft Corporation.

    File:

        YUY2Synthesizer.h

    Abstract:

        This file contains the definition of CYUY2Synthesizer.

        CYUY2Synthesizer is derived from CYUVSynthesizer.  An image 
        synthesizer for YUY2 format.  It uses the YUV color space and defines 
        a Commit() function.
        
        YUY2 is a 4:2:2 format that uses a single plane that decimates chroma 
        samples on the x axis with an effective bit rate of 16 bits per pixel.
        A plane is an array of rows.  Rows consist of pairs of pixels in the 
        sequence: Y0|U|Y1|V.

        The synthesizer stores the YUV information in 32 bit format internally
        for speed, accuracy and simplicity.  The Commit function does the 
        format conversion.

    History:

        created 4/14/2014

**************************************************************************/

class CYUY2Synthesizer : public CYUVSynthesizer
{
public:
    //
    // DEFAULT CONSTRUCTOR
    //
    CYUY2Synthesizer (
        ULONG Width=0,
        ULONG Height=0
    ) :
        CYUVSynthesizer("YUY2", Width, Height)
    {
        NT_ASSERT( ( Width % 2 ) == 0 );
        m_OutputStride = Width * sizeof(WORD);
    }

    //
    //  Commit
    //
    //  Copy (and reformat, if necessary) pixels from the internal scratch
    //  buffer.  If the output format decimates chrominance, do it here.
    //
    virtual
    _Success_(return > 0)
    ULONG
    Commit(
        _Out_writes_bytes_(Size)
        PUCHAR  Buffer,
        _In_    ULONG   Size,
        _In_    ULONG   Stride
    );

};


