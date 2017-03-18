/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2014, Microsoft Corporation.

    File:

        XRGBSynthesizer.h

    Abstract:

        This file contains the definition of CXRGBSynthesizer.

        CXRGBSynthesizer is derived from CSynthesizer.  It uses the RGB 
        color space and defines a Commit() function.  XRGB is known also
        known as RGB32.  It uses 8 bits per primary plus 8 bits of padding
        for 4 bytes per pixel in a single plane.

    History:

        created 4/14/2014

**************************************************************************/

/*************************************************

    CXRGBSynthesizer

    Image synthesizer for XRGB (aka RGB32) format.

*************************************************/

class CXRGBSynthesizer
    : public CSynthesizer
{
protected:
    BOOLEAN m_FlipVertical;

public:
    //
    // DEFAULT CONSTRUCTOR:
    //
    CXRGBSynthesizer (
        LONG Width=0,
        LONG Height=0
    ) :
        CSynthesizer("RGB32", CHANNEL_RGB, Width, ABS(Height))
        , m_FlipVertical(Height==ABS(Height))
    {
        m_OutputStride = Width * sizeof(KS_RGBQUAD);
    }

    //
    // DESTRUCTOR:
    //
    virtual
    ~CXRGBSynthesizer ()
    {}

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

    //
    // Initialize()
    //
    //  Initialize the Gradient bmp for RGB color space.
    //
    virtual
    BOOLEAN
    Initialize();

protected:
    //
    //  GetPalette
    //
    //  Get a pointer to an array of palette colors.  Used mostly to handle
    //  different color primaries.  Location of the primary must agree with
    //  Commit().
    virtual
    const UCHAR4 *
    GetPalette();
};

