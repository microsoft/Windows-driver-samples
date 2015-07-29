/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2014, Microsoft Corporation.

    File:

        Synthesizer.h

    Abstract:

        This file contains the definition of CSynthesizer.

        The base image synthesis and overlay class.  These classes provide
        image synthesis (pixel, color-bar, etc...) onto buffers of various
        formats.

        Internally, all CSynthesizer objects represent a pixel as a 32 bit
        quantity with 8 bits per sample.  The base CSynthesizer implements
        all rendering functionality with this assumption in mind.  It
        simplifies rendering substantially and means we do not need to re-
        implement rendering functions for each format.  
        
        Most end formats can be synthesized from this uncompressed format 
        with overhead that is slightly worse than a copy.  Color spaces are
        handled by using a rendering palette that is unique for each.

    History:

        created 04/14/2014

**************************************************************************/

#pragma once

//
// COLOR:
//
// Pixel color for placement onto the synthesis buffer.
//
typedef enum
{

    BLACK = 0,
    WHITE,
    YELLOW,
    CYAN,
    GREEN,
    MAGENTA,
    RED,
    BLUE,
    GREY,

    MAX_COLOR,
    TRANSPARENT,

} COLOR;

#define CHANNEL_YCrCb   ( MF_HISTOGRAM_CHANNEL_Y | MF_HISTOGRAM_CHANNEL_Cr | MF_HISTOGRAM_CHANNEL_Cb )
#define CHANNEL_RGB     ( MF_HISTOGRAM_CHANNEL_R | MF_HISTOGRAM_CHANNEL_G  | MF_HISTOGRAM_CHANNEL_B  )
#define CHANNEL_NONE    ( 0 )

//
// POSITION_CENTER:
//
// Only useful for text overlay.  This can be substituted for LocX or LocY
// in order to center the text screen on the synthesis buffer.
//
#define POSITION_CENTER ((ULONG)-1)

//
//  Initializer class for a KS_RGBQUAD
//
class CKsRgbQuad : public KS_RGBQUAD
{
public:
    //  Initializer ctor
    CKsRgbQuad(
        BYTE r=0,
        BYTE g=0,
        BYTE b=0 )
    {
        rgbBlue     = b;
        rgbGreen    = g;
        rgbRed      = r;
        rgbReserved = 0;
    }
};

/*************************************************

    CSynthesizer

    This class synthesizes images in various formats for output from the
    capture filter.  It is capable of performing various text overlays onto
    the image surface.

    The base synthesizer is essentially the XRGB Synthesizer.  For YUV, we
    repurpose the color primaries and provide a different definition for the
    color pallette.  A call to CSynthesizer::Commit(buffer) will reformat the
    internal representation and down-sample the color primaries, as necessary
    into the supplied buffer.

*************************************************/

typedef
UCHAR   UCHAR4[4];

class CSynthesizer :
    public CNonCopyable
{
protected:

    static
    const UCHAR m_FontData [256][8];
    static
    const COLOR m_ColorBars[8];

    //  These values are used by Synthesize() for display purposes.  They are
    //  static so that setting the values on the preview pin affects all pins.
    //
    //  TODO: Change to references to a shared object.  Use of static values
    //        could cause confusion if two (or more) cameras are in use at
    //        the same time.
    static
    LONGLONG    m_RelPts;
    static
    LONGLONG    m_QpcTime;
    static
    ULONG       m_Frame;

    //
    // The width and height the synthesizer is set to.
    //
    ULONG m_Width;
    ULONG m_Height;

    //
    //  The synthesis buffer:
    //
    //  This buffer points to an array of KS_RGBQUAD.  Internally we synthesize
    //  images in an uncompressed format, because it is at least as fast to
    //  composite an image in such a format and convert to the target compressed
    //  format in a final pass over the image.
    //
    //  An uncompressed format also has less loss of fidelity.  For instance,
    //  NV12 only contains 1 chroma sample per macro-pixel (2x2).  Once this
    //  macro pixel is rendered, compositing additional color information
    //  into it results in a loss of color precision / fidelity.
    //
    PUCHAR  m_Buffer;               //  pointer to pixel data
    ULONG   m_Length;               //  size of the buffer in bytes
    LONG    m_SynthesisStride;      //  size of scan line in bytes

    //
    //  The assumed stride of our output format.  Currently used by YUY2 and
    //  all image captures, except NV12.  This value should be initialized by
    //  the derived classes.
    //
    LONG    m_OutputStride;

    //  Bitmap with a gradient applied for each color in the color pallet.
    CKsRgbQuad *m_GradientBmp;

    //
    // The default cursor.  This is a pointer into the synthesis buffer where
    // a non specific PutPixel will be placed.
    //
    PUCHAR m_Cursor;

    //
    //  A printable name identifying this format.
    //
    PCCHAR  m_FormatName;

    //
    //  A color palette for this colorspace.
    //
    //  Specifying a unique color palette is a cheap method for switching
    //  between RGB and YUV color spaces.  A derived class is free to assign
    //  this to any table it chooses.  It can be used to either change the
    //  colors used for rendering, or it can be used to support a new (or
    //  slightly different) color space.
    //
    const UCHAR4    *m_Colors;

    //
    //  A bitset specifying the color primaries used by this format.
    //  (Used by Histogram.)
    //
    ULONG   m_ChannelMask;

    //
    //  Debugging values
    //
    LARGE_INTEGER   m_Frequency;
    LONGLONG    m_SynthesisTime;
    ULONG       m_SynthesisCount;
    LONGLONG    m_CommitTime;
    ULONG       m_CommitCount;

public:

    //
    // DEFAULT CONSTRUCTOR
    //
    CSynthesizer(
        PCCHAR Name="[Unknown]",
        ULONG ChannelMask=0,
        ULONG Width=0,
        ULONG Height=0
    )
        : m_Width(Width)
        , m_Height(Height)
        , m_Buffer(nullptr)
        , m_Cursor(nullptr)
        , m_GradientBmp(nullptr)
        , m_SynthesisStride(m_Width * sizeof(KS_RGBQUAD))
        , m_OutputStride(0)
        , m_FormatName(Name)
        , m_ChannelMask(ChannelMask)
        , m_SynthesisCount(0)
        , m_SynthesisTime(0)
        , m_CommitCount(0)
        , m_CommitTime(0)
    {
        m_Length = Height * m_SynthesisStride;
        KeQueryPerformanceCounter(&m_Frequency);
    }

    //
    // DESTRUCTOR:
    //
    virtual
    ~CSynthesizer()
    {}

    //
    // PutPixel():
    //
    // Place a pixel at the default cursor location.  The cursor location
    // must be set via GetImageLocation(x, y).
    //
    virtual
    void
    PutPixel (
        COLOR Color
    );

    //
    // PutPixel():
    //
    // Place a pixel at the default cursor location.  The cursor location
    // must be set via GetImageLocation(x, y).
    //
    virtual
    void
    PutPixel (
        UCHAR colorR,
        UCHAR colorB,
        UCHAR colorG
    );

    //
    //  Create a horizontal color bar that slowly fades from the specified
    //  color to black.
    //
    //  Note: To generalize, this function could specify the height of the
    //        color bar instead of inferring it.
    //
    void
    ApplyGradient(
        _In_    ULONG LocY,
        _In_    COLOR Gradient
    );


    //
    // SetImageSize():
    //
    // Set the image size of the synthesis buffer.
    //
    void
    SetImageSize (
        _In_    ULONG Width,
        _In_    ULONG Height
    )
    {
        m_Width = Width;
        m_Height = Height;
    }

    //
    // Initialize()
    //
    //  Set the buffer the synthesizer generates images to.
    //  Override to do any additional processing you think is needed.
    //
    virtual
    BOOLEAN
    Initialize();

    //
    //  Destroy()
    //
    //  Clean up from initialize.
    //
    virtual
    void
    Destroy();

    //
    // SynthesizeBars():
    //
    // Synthesize EIA-189-A standard color bars.
    //
    virtual
    NTSTATUS
    SynthesizeBars();

    //
    //  Synthesize
    //
    //  Fill the image buffer with some base image. All h/w simulations will
    //  call this function to generate a base image
    //
    virtual
    NTSTATUS
    Synthesize();

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
    )=0;

    //
    //  Commit
    //
    //  Note: The stride needs to be a function of the bits per pixel of the
    //        OUTPUT format - not the format we store it in.  The class ctor
    //        must initialize m_OutputStride to a default stride value.
    //
    _Success_(return > 0)
    ULONG
    Commit(
        _Out_writes_bytes_(Size)
        PUCHAR  Buffer,
        _In_    ULONG   Size
    )
    {
        return Commit( Buffer, Size, m_OutputStride );
    }

    //
    //  Histogram
    //
    //  Fill an array with histogram data.  Each parameter is optional.
    //
    void
    Histogram(
        _Out_writes_opt_(256)
        PULONG  HistogramP0,        //  rgbRed
        _Out_writes_opt_(256)
        PULONG  HistogramP1,        //  rgbGreen
        _Out_writes_opt_(256)
        PULONG  HistogramP2         //  rgbBlue
    );

    //
    //  Synthesize
    //
    //  Fill the image buffer with some base image. All h/w simulations will
    //  call this function to generate a base image
    //
    NTSTATUS
    DoSynthesize();

    //
    //  Commit
    //
    //  Copy (and reformat, if necessary) pixels from the internal scratch
    //  buffer.  If the output format decimates chrominance, do it here.
    //
    _Success_(return > 0)
    ULONG
    DoCommit(
        _Out_writes_bytes_(Size)
        PUCHAR  Buffer,
        _In_    ULONG   Size,
        _In_    ULONG   Stride
    );

    //
    //  DoCommit
    //
    //  Note: The stride needs to be a function of the bits per pixel of the
    //        OUTPUT format - not the format we store it in.  The class ctor
    //        must initialize m_OutputStride to a default stride value.
    //
    _Success_(return > 0)
    ULONG
    DoCommit(
        _Out_writes_bytes_(Size)
        PUCHAR  Buffer,
        _In_    ULONG   Size
    );

    //
    // OverlayText():
    //
    // Overlay a text string onto the image.
    //
    void
    OverlayText (
        _In_ ULONG LocX,
        _In_ ULONG LocY,
        _In_ ULONG Scaling,
        _In_ LPSTR Text,
        _In_ COLOR BgColor,
        _In_ COLOR FgColor
    );

    virtual
    void
    EncodeNumber(
        _In_ ULONG LocY,
        _In_ UINT32 Number,
        _In_ COLOR LowColor,
        _In_ COLOR HighColor
    );

    void
    SetRelativePts( LONGLONG Pts )
    {
        m_RelPts = Pts;
    }

    void
    SetQpcTime( LONGLONG Pts )
    {
        m_QpcTime = Pts;
    }

    void
    SetFrameNumber( ULONG Frame )
    {
        m_Frame = Frame;
    }

    //
    //  GetChannelMask
    //
    //  Get the color channel mask associated with this format.
    //
    ULONG
    GetChannelMask()
    {
        return m_ChannelMask;
    }

protected:
    //
    //  GetPalette
    //
    //  Get a pointer to an array of palette colors.  Used mostly to handle
    //  different color primaries.  Location of the primary must agree with
    //  Commit().
    //
    virtual
    const UCHAR4 *
    GetPalette()
    {
        return nullptr;
    }

    //
    // GetImageLocation
    //
    // Get the location into the image buffer for a specific X/Y location.
    // This also sets the synthesizer's default cursor to the position
    // LocX, LocY.
    //
    PUCHAR
    GetImageLocation (
        _In_    ULONG LocX,
        _In_    ULONG LocY
    )
    {
        return
            m_Cursor =
                (m_Buffer + (sizeof(CKsRgbQuad) * LocX) + (LocY * m_SynthesisStride));
    }

};

