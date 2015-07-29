/**************************************************************************

    AVStream Filter-Centric Sample

    Copyright (c) 1999 - 2001, Microsoft Corporation

    File:

        video.h

    Abstract:

        This file contains the video capture pin header.

    History:

        created 6/11/01

**************************************************************************/

class CVideoCapturePin :
    public CCapturePin {

private:

    //
    // A scratch buffer to write into.  Due to the fact that we're likely
    // sitting upstream of the VMR and getting video memory, I don't want the
    // image synthesizer writing to video memory a single byte at a time.
    // This will be the buffer that the image synth uses.  After a synthesis,
    // the buffer will get copied into the data buffers for capture.
    //
    PUCHAR m_SynthesisBuffer;

    //
    // The captured video info header.  The settings for image synthesis will
    // be based off this header.
    //
    PKS_VIDEOINFOHEADER m_VideoInfoHeader;

    //
    // The image synthesizer.  This object is used to construct synthesized
    // image data in the video format specified by the connection format.
    //
    CImageSynthesizer *m_ImageSynth;

    //
    // CaptureVideoInfoHeader():
    //
    // This routine stashes the video info header set on the pin connection
    // in the CVideoCapturePin object.  This is used to determine necessary
    // variables for image synthesis, etc...
    //
    PKS_VIDEOINFOHEADER
    CaptureVideoInfoHeader (
        );

protected:

public:

    //
    // CVideoCapturePin():
    //
    // Construct a new video capture pin.
    //
    CVideoCapturePin (
        IN PKSPIN Pin
        ) :
        CCapturePin (Pin)
    {
    }

    //
    // ~CVideoCapturePin():
    //
    // Destruct a video capture pin.
    //
    virtual
    ~CVideoCapturePin (
        )
    {
    }

    //
    // CaptureFrame():
    //
    // Called from the filter processing routine to indicate that the pin
    // should attempt to trigger capture of a video frame.  This routine
    // will copy synthesized image data into the frame buffer and complete
    // the frame buffer.
    //
    virtual
    NTSTATUS
    CaptureFrame (
        IN PKSPROCESSPIN ProcessPin,
        IN ULONG Tick
        );

    //
    // Pause():
    //
    // Called when the video capture pin is transitioning into the pause
    // state.  This will instruct the capture filter to start the timer DPC's
    // at the interval demanded by the video info header in the connection
    // format.
    //
    virtual
    NTSTATUS
    Pause (
        IN KSSTATE FromState
        );

    //
    // Acquire():
    //
    // Called when the video capture pin is transitioning into the acquire
    // state.  This will create the necessary image synthesizer to begin
    // synthesizing frame capture data when the pin transitions to the
    // appropriate state.
    //
    virtual
    NTSTATUS
    Acquire (
        IN KSSTATE FromState
        );

    //
    // Stop():
    //
    // Called when the video capture pin is transitioning into a stop state.
    // This simply destroys the image synthesizer in preparation for creating
    // a new one next acquire.
    //
    virtual
    NTSTATUS
    Stop (
        IN KSSTATE FromState
        );

    /*************************************************

        Dispatch Functions

    *************************************************/

    //
    // DispatchCreate():
    //
    // This is the creation dispatch for the video capture pin on the filter.
    // It creates the CVideoCapturePin, associates it with the AVStream pin
    // object and bags the class object for automatic cleanup when the
    // pin is closed.
    //
    static
    NTSTATUS
    DispatchCreate (
        IN PKSPIN Pin,
        IN PIRP Irp
        );

    //
    // DispatchSetFormat():
    //
    // This is the set data format dispatch for the pin.  This will be called 
    // BEFORE pin creation to validate that a data format selected is a match
    // for the range pulled out of our range list.  It will also be called
    // for format changes.
    //
    // If OldFormat is NULL, this is an indication that it's the initial
    // call and not a format change.  Even fixed format pins get this call
    // once.
    //
    static
    NTSTATUS
    DispatchSetFormat (
        IN PKSPIN Pin,
        IN PKSDATAFORMAT OldFormat OPTIONAL,
        IN PKSMULTIPLE_ITEM OldAttributeList OPTIONAL,
        IN const KSDATARANGE *DataRange,
        IN const KSATTRIBUTE_LIST *AttributeRange OPTIONAL
        );

    //
    // IntersectHandler():
    //
    // This is the data intersection handler for the capture pin.  This 
    // determines an optimal format in the intersection of two ranges,
    // one local and one possibly foreign.  If there is no compatible format,
    // STATUS_NO_MATCH is returned.
    //
    static
    NTSTATUS
    IntersectHandler (
        IN PKSFILTER Filter,
        IN PIRP Irp,
        IN PKSP_PIN PinInstance,
        IN PKSDATARANGE CallerDataRange,
        IN PKSDATARANGE DescriptorDataRange,
        IN ULONG BufferSize,
        OUT PVOID Data OPTIONAL,
        OUT PULONG DataSize
        );

    //
    // CleanupSynth():
    //
    // Called when the Image Synthesizer is removed from the object bag
    // to be cleaned up.  We simply delete the image synth.
    //
    static
    void
    CleanupSynth (
        IN CImageSynthesizer *ImageSynth
        )
    {
        delete ImageSynth;
    }

};

