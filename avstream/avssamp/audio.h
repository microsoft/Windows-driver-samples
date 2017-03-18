/**************************************************************************

    AVStream Filter-Centric Sample

    Copyright (c) 1999 - 2001, Microsoft Corporation

    File:

        audio.h

    Abstract:

        This file contains the audio capture pin header.

    History:

        created 6/28/01

**************************************************************************/

class CAudioCapturePin :
    public CCapturePin

{

private:

    //
    // The wave object used to synthesize audio data.
    //
    CWaveObject *m_WaveObject;

public:

    //
    // CAudioCapturePin():
    //
    // Construct a new audio capture pin.
    //
    CAudioCapturePin (
        IN PKSPIN Pin
        ) : CCapturePin (Pin)
    {
    }

    //
    // ~CAudioCapturePin():
    //
    // Destruct an audio capture pin.
    //
    ~CAudioCapturePin (
        )
    {
    }

    //
    // Acquire():
    //
    // Called when the audio capture pin is transitioning into the acquire
    // state (from either stop or pause).  This routine will get ahold of
    // the wave object from the filter.
    //
    virtual
    NTSTATUS
    Acquire (
        IN KSSTATE FromState
        );

    //
    // CaptureFrame():
    //
    // This is called when the filter processes and wants to trigger processing
    // of an audio frame.  The routine will compute how far into the stream
    // we've progressed and ask the filter's wave object to copy enough
    // "synthesized" audio data from the wave object in order to reach
    // the position.
    //
    virtual
    NTSTATUS
    CaptureFrame (
        IN PKSPROCESSPIN ProcessPin,
        IN ULONG Tick
        );

    /*************************************************

        Dispatch Functions

    *************************************************/

    //
    // DispatchCreate():
    //
    // This is the creation dispatch for the audio capture pin on the filter.
    // It creates the CAudioCapturePin, associates it with the AVStream pin
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
        

};
