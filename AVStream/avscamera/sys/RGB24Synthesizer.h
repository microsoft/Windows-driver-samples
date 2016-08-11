/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2014, Microsoft Corporation.

    File:

        RGB24Synthesizer.h

    Abstract:

        This file contains the definition of CRGB24Synthesizer.
        
        CRGB24Synthesizer is derived from CXRGBSynthesizer.  It uses the RGB 
        color space and defines a Commit() function that reformats pixels into 
        the RGB24 format.  RGB24 uses 8 bits per primary and 3 bytes per pixel 
        in a single plane.

    History:

        created 4/14/2014

**************************************************************************/

/*************************************************

    CRGB24Synthesizer

    Image synthesizer for RGB24 format.

*************************************************/

class CRGB24Synthesizer
    : public CXRGBSynthesizer
{
public:

    //
    // Default constructor
    //
    CRGB24Synthesizer(
        LONG Width=0,
        LONG Height=0
    )
        : CXRGBSynthesizer(Width, Height)
    {
        //  Round up to the next full dword for a row on output.
        m_OutputStride = (( Width * 3 ) + 3) & ((LONG) ~3);
        m_FormatName = "RGB24";
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

