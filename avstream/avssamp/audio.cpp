/**************************************************************************

    AVStream Filter-Centric Sample

    Copyright (c) 1999 - 2001, Microsoft Corporation

    File:

        audio.cpp

    Abstract:

        This file contains the audio capture pin implementation.

    History:

        created 6/28/01

**************************************************************************/

#include "avssamp.h"

/**************************************************************************

    PAGED CODE

**************************************************************************/

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA


NTSTATUS
CAudioCapturePin::
DispatchCreate (
    IN PKSPIN Pin,
    IN PIRP Irp
    )

/*++

Routine Description:

    Create a new audio capture pin.  This is the creation dispatch for
    the audio capture pin.

Arguments:

    Pin -
        The pin being created

    Irp -
        The creation Irp

Return Value:

    Success / Failure

--*/

{

    PAGED_CODE();

    NTSTATUS Status = STATUS_SUCCESS;

    CAudioCapturePin *CapPin = new (NonPagedPool) CAudioCapturePin (Pin);
    CCapturePin *BasePin = static_cast <CCapturePin *> (CapPin);

    if (!CapPin) {
        //
        // Return failure if we couldn't create the pin.
        //
        Status = STATUS_INSUFFICIENT_RESOURCES;

    } else {
        //
        // Add the item to the object bag if we we were successful. 
        // Whenever the pin closes, the bag is cleaned up and we will be
        // freed.
        //
        Status = KsAddItemToObjectBag (
            Pin -> Bag,
            reinterpret_cast <PVOID> (BasePin),
            reinterpret_cast <PFNKSFREE> (CCapturePin::BagCleanup)
            );

        if (!NT_SUCCESS (Status)) {
            delete CapPin;
        } else {
            Pin -> Context = reinterpret_cast <PVOID> (BasePin);
        }

    }

    return Status;

}

/*************************************************/


NTSTATUS
CAudioCapturePin::
Acquire (
    IN KSSTATE FromState
    )

/*++

Routine Description:

    Called when the pin transitions into acquire, this gets and releases
    our hold on the wave object we use to synthesize audio streams.

Arguments:

    FromState -
        The state the pin is transitioning away from

Return Value:

    Success / Failure

--*/

{

    PAGED_CODE();

    NTSTATUS Status = STATUS_SUCCESS;

    if (FromState == KSSTATE_STOP) {
        //
        // On the transition into acquire from stop, get ahold of the
        // wave object we're synthesizing from.
        //
        m_WaveObject = m_ParentFilter -> GetWaveObject ();
        NT_ASSERT (m_WaveObject);

        //
        // There must be a wave object or something is really wrong.
        //
        if (!m_WaveObject) {
            Status = STATUS_INTERNAL_ERROR;
        } else {
            m_WaveObject -> Reset ();
        }

    } else {
        //
        // Ensure we hold no reference on the wave object.
        //
        m_WaveObject = NULL;

    }

    return Status;
            
}

/*************************************************/


NTSTATUS
CAudioCapturePin::
IntersectHandler (
    IN PKSFILTER Filter,
    IN PIRP Irp,
    IN PKSP_PIN PinInstance,
    IN PKSDATARANGE CallerDataRange,
    IN PKSDATARANGE DescriptorDataRange,
    IN ULONG BufferSize,
    OUT PVOID Data OPTIONAL,
    OUT PULONG DataSize
    )

/*++

Routine Description:

    The intersect handler for the audio capture pin.  This is really quite
    simple because the audio pin only exposes the number of channels,
    sampling frequency, etc...  that the wave file it is synthesizing from
    contains.

Arguments:

    Filter -
        Contains a void pointer to the  filter structure.

    Irp -
        Contains a pointer to the data intersection property request.

    PinInstance -
        Contains a pointer to a structure indicating the pin in question.

    CallerDataRange -
        Contains a pointer to one of the data ranges supplied by the client
        in the data intersection request.  The format type, subtype and
        specifier are compatible with the DescriptorDataRange.

    DescriptorDataRange -
        Contains a pointer to one of the data ranges from the pin descriptor
        for the pin in question.  The format type, subtype and specifier are
        compatible with the CallerDataRange.

    BufferSize -
        Contains the size in bytes of the buffer pointed to by the Data
        argument.  For size queries, this value will be zero.

    Data -
        Optionally contains a pointer to the buffer to contain the data 
        format structure representing the best format in the intersection 
        of the two data ranges.  For size queries, this pointer will be 
        NULL.

    DataSize -
        Contains a pointer to the location at which to deposit the size 
        of the data format.  This information is supplied by the function 
        when the format is actually delivered and in response to size 
        queries.

Return Value:

    STATUS_SUCCESS if there is an intersection and it fits in the supplied
    buffer, STATUS_BUFFER_OVERFLOW for successful size queries, 
    STATUS_NO_MATCH if the intersection is empty, or 
    STATUS_BUFFER_TOO_SMALL if the supplied buffer is too small.

--*/


{
    
    PAGED_CODE();

    //
    // Verify that the inpassed range is valid size. 
    //
    if (CallerDataRange -> FormatSize < sizeof (KSDATARANGE_AUDIO)) {
        return STATUS_NO_MATCH;
    }

    //
    // Because the only range we expose is such that it will match
    // KSDATARANGE_AUDIO, it is safe to interpret the data structures as
    // KSDATARANGE_AUDIO.  This is due to the fact that AVStream will have
    // prematched the GUIDs for us.
    //
    PKSDATARANGE_AUDIO CallerAudioRange =
        reinterpret_cast <PKSDATARANGE_AUDIO> (CallerDataRange);

    PKSDATARANGE_AUDIO DescriptorAudioRange =
        reinterpret_cast <PKSDATARANGE_AUDIO> (DescriptorDataRange);

    //
    // We are returning a KSDATAFORMAT_WAVEFORMATEX.  Specify such if a size
    // query happens.
    //
    if (BufferSize == 0) {
        *DataSize = sizeof (KSDATAFORMAT_WAVEFORMATEX);
        return STATUS_BUFFER_OVERFLOW;
    }

    if (BufferSize < sizeof (KSDATAFORMAT_WAVEFORMATEX)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Match the blocks.  We only support one format (not really a range), so
    // this intersection aught to be really simple.  It's more of a check
    // if the format we are going to use intersects somewhere in 
    // CallerAudioRange.
    //
    if (DescriptorAudioRange -> MaximumChannels > 
            CallerAudioRange -> MaximumChannels ||
        DescriptorAudioRange -> MinimumBitsPerSample <
            CallerAudioRange -> MinimumBitsPerSample ||
        DescriptorAudioRange -> MinimumBitsPerSample >
            CallerAudioRange -> MaximumBitsPerSample ||
        DescriptorAudioRange -> MinimumSampleFrequency <
            CallerAudioRange -> MinimumSampleFrequency ||
        DescriptorAudioRange -> MinimumSampleFrequency >
            CallerAudioRange -> MaximumSampleFrequency) {

        //
        // If the descriptor's "range" (more of a single format specified
        // in a range) doesn't intersect the caller's, no match the call.
        //
        *DataSize = sizeof (KSDATAFORMAT_WAVEFORMATEX);
        return STATUS_NO_MATCH;

    }

    //
    // Build the format.
    //
    PKSDATAFORMAT_WAVEFORMATEX WaveFormat =
        reinterpret_cast <PKSDATAFORMAT_WAVEFORMATEX> (Data);

    RtlCopyMemory (
        &WaveFormat -> DataFormat,
        &DescriptorAudioRange -> DataRange,
        sizeof (KSDATAFORMAT)
        );

    WaveFormat -> WaveFormatEx.wFormatTag = WAVE_FORMAT_PCM;
    WaveFormat -> WaveFormatEx.nChannels = 
        (WORD)DescriptorAudioRange -> MaximumChannels;
    WaveFormat -> WaveFormatEx.nSamplesPerSec =
        DescriptorAudioRange -> MaximumSampleFrequency;
    WaveFormat -> WaveFormatEx.wBitsPerSample =
        (WORD)DescriptorAudioRange -> MaximumBitsPerSample;
    WaveFormat -> WaveFormatEx.nBlockAlign =
        (WaveFormat -> WaveFormatEx.wBitsPerSample / 8) *
        WaveFormat -> WaveFormatEx.nChannels;
    WaveFormat -> WaveFormatEx.nAvgBytesPerSec =
        WaveFormat -> WaveFormatEx.nBlockAlign *
        WaveFormat -> WaveFormatEx.nSamplesPerSec;
    WaveFormat -> WaveFormatEx.cbSize = 0;
    WaveFormat -> DataFormat.SampleSize = 
        WaveFormat -> WaveFormatEx.nBlockAlign;
    
    WaveFormat -> DataFormat.FormatSize = 
    *DataSize = sizeof (KSDATAFORMAT_WAVEFORMATEX);

    return STATUS_SUCCESS;

}

/*************************************************/


NTSTATUS
CAudioCapturePin::
DispatchSetFormat (
    IN PKSPIN Pin,
    IN PKSDATAFORMAT OldFormat OPTIONAL,
    IN PKSMULTIPLE_ITEM OldAttributeList OPTIONAL,
    IN const KSDATARANGE *DataRange,
    IN const KSATTRIBUTE_LIST *AttributeRange OPTIONAL
    )

/*++

Routine Description:

    This is the set data format dispatch for the capture pin.  It is called
    in two circumstances.

        1: before Pin's creation dispatch has been made to verify that
           Pin -> ConnectionFormat is an acceptable format for the range
           DataRange.  In this case OldFormat is NULL.

        2: after Pin's creation dispatch has been made and an initial format
           selected in order to change the format for the pin.  In this case,
           OldFormat will not be NULL.

    Validate that the format is acceptible and perform the actions necessary
    to change format if appropriate.

Arguments:

    Pin -
        The pin this format is being set on.  The format itself will be in
        Pin -> ConnectionFormat.

    OldFormat -
        The previous format used on this pin.  If this is NULL, it is an
        indication that Pin's creation dispatch has not yet been made and
        that this is a request to validate the initial format and not to
        change formats.

    OldAttributeList -
        The old attribute list for the prior format

    DataRange -
        A range out of our list of data ranges which was determined to be
        at least a partial match for Pin -> ConnectionFormat.  If the format
        there is unacceptable for the range, STATUS_NO_MATCH should be
        returned.

    AttributeRange -
        The attribute range

Return Value:

    Success / Failure

        STATUS_SUCCESS -
            The format is acceptable / the format has been changed

        STATUS_NO_MATCH -
            The format is not-acceptable / the format has not been changed

--*/

{

    PAGED_CODE();

    //
    // This pin does not accept any format changes.  It is fixed format based
    // on what the wave file we're synthesizing from is.  Thus, we don't
    // need to worry about this being called in any context except pin
    // creation (KSPIN_FLAG_FIXED_FORMAT ensures this).  Knowing that the
    // format already is a GUID match for the range and we only have one
    // range, the interpretation without any guid checks is safe.
    //
    NT_ASSERT (!OldFormat);

    const KSDATARANGE_AUDIO *DataRangeAudio =
        reinterpret_cast <const KSDATARANGE_AUDIO *> (DataRange);

    //
    // Verify the format is the right size.
    //
    if (Pin -> ConnectionFormat -> FormatSize <
        sizeof (KSDATAFORMAT_WAVEFORMATEX)) {

        return STATUS_NO_MATCH;
    }

    PKSDATAFORMAT_WAVEFORMATEX WaveFormat =
        reinterpret_cast <PKSDATAFORMAT_WAVEFORMATEX> (
            Pin -> ConnectionFormat
            );

    //
    // This is not an intersection, but rather a direct comparison due to
    // the fact that we're fixed to a single format and do not really have
    // a range.
    //
    if (WaveFormat -> WaveFormatEx.wFormatTag != WAVE_FORMAT_PCM ||
        WaveFormat -> WaveFormatEx.nChannels !=
            DataRangeAudio -> MaximumChannels ||
        WaveFormat -> WaveFormatEx.nSamplesPerSec !=
            DataRangeAudio -> MaximumSampleFrequency ||
        WaveFormat -> WaveFormatEx.wBitsPerSample !=
            DataRangeAudio -> MaximumBitsPerSample) {

        return STATUS_NO_MATCH;

    }

    //
    // The format passes consideration.  Allow the pin creation with this
    // particular format.
    //
    return STATUS_SUCCESS;
    
}

/**************************************************************************

    LOCKED CODE

**************************************************************************/

#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif // ALLOC_PRAGMA


NTSTATUS
CAudioCapturePin::
CaptureFrame (
    IN PKSPROCESSPIN ProcessPin,
    IN ULONG Tick
    )

/*++

Routine Description:

    Called to synthesize a frame of audio data from the wave object.

Arguments:

    ProcessPin -
        The process pin from the filter's process pins index

    Tick -
        The tick counter from the filter (the number of DPC's that have 
        happened since the DPC timer started).  Note that the DPC timer
        starts at pause and capture starts at run.

Return Value:

    Success / Failure

--*/

{

    NT_ASSERT (ProcessPin -> Pin == m_Pin);

    //
    // Increment the frame number.  This is the total count of frames which
    // have attempted capture.
    //
    m_FrameNumber++;

    //
    // Find out how much time worth of audio data to synthesize into
    // the buffer a buffer (or how much time to skip if there are no available
    // capture buffers).
    //
    LONGLONG TimerInterval = m_ParentFilter -> GetTimerInterval ();

    //
    // Since this pin is KSPIN_FLAG_FRAMES_NOT_REQUIRED_FOR_PROCESSING, it
    // means that we do not require frames available in order to process.
    // This means that this routine can get called from our DPC with no
    // buffers available to capture into.  In this case, we increment our
    // dropped frame counter and skip forward into the audio stream.
    //
    if (ProcessPin -> BytesAvailable) {
        //
        // Synthesize a fixed amount of audio data based on the timer interval.
        //
        ULONG BytesUsed = m_WaveObject -> SynthesizeFixed (
            TimerInterval,
            ProcessPin -> Data,
            ProcessPin -> BytesAvailable
            );
    
        ProcessPin -> BytesUsed = BytesUsed;
        ProcessPin -> Terminate = TRUE;
    
        //
        // Time stamp the packet if there is a clock assigned.
        //
        if (m_Clock) {
            PKSSTREAM_HEADER StreamHeader = 
                ProcessPin -> StreamPointer -> StreamHeader;

            StreamHeader -> PresentationTime.Time = m_Clock -> GetTime ();
            StreamHeader -> PresentationTime.Numerator =
                StreamHeader -> PresentationTime.Denominator = 1;
            StreamHeader -> OptionsFlags |=
                KSSTREAM_HEADER_OPTIONSF_TIMEVALID;
        }

    } else {
        m_DroppedFrames++;

        //
        // Since we've skipped an audio frame, inform the wave object to
        // skip forward this much.
        //
        m_WaveObject -> SkipFixed (TimerInterval);
    }
    
    return STATUS_SUCCESS;

}

/**************************************************************************

    DESCRIPTOR / DISPATCH LAYOUT

**************************************************************************/

//
// AudioCapturePinDispatch:
//
// This is the dispatch table for the capture pin.  It provides notifications
// about creation, closure, processing, data formats, etc...
//
const
KSPIN_DISPATCH
AudioCapturePinDispatch = {
    CAudioCapturePin::DispatchCreate,       // Pin Create
    NULL,                                   // Pin Close
    NULL,                                   // Pin Process
    NULL,                                   // Pin Reset
    CAudioCapturePin::DispatchSetFormat,    // Pin Set Data Format
    CCapturePin::DispatchSetState,          // Pin Set Device State
    NULL,                                   // Pin Connect
    NULL,                                   // Pin Disconnect
    NULL,                                   // Clock Dispatch
    NULL                                    // Allocator Dispatch
};

//
// AudioDefaultAllocatorFraming:
//
// A default framing for the audio pin.  In order for this to work properly,
// the frame size must be at least 1/fps * bytes_per_sec large.  Otherwise,
// the audio stream will fall behind.  This is dynamically adjusted when
// the actual pin is created.
//
DECLARE_SIMPLE_FRAMING_EX (
    AudioDefaultAllocatorFraming,
    STATICGUIDOF (KSMEMORY_TYPE_KERNEL_NONPAGED),
    KSALLOCATOR_REQUIREMENTF_SYSTEM_MEMORY |
        KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY,
    25,
    0,
    2 * PAGE_SIZE,
    2 * PAGE_SIZE
    );

//
// g_PINNAME_AUDIO_CAPTURE:
//
// A GUID identifying the name of the audio capture pin.  I use the standard
// STATIC_PINNAME_VIDEO_CAPTURE for the video capture pin, but a custom name
// as defined in avssamp.inf for the audio capture pin.
//
GUID g_PINNAME_AUDIO_CAPTURE = 
    {0xba1184b9, 0x1fe6, 0x488a, 0xae, 0x78, 0x6e, 0x99, 0x7b, 0x2, 0xca, 0xea};

//
// AudioPinDescriptorTemplate:
//
// The template for the audio pin descriptor.  The audio pin on this filter
// is created dynamically -- if and only if c:\avssamp.wav exists and is
// a valid and readable wave file.
//
const
KSPIN_DESCRIPTOR_EX
AudioPinDescriptorTemplate = {
    //
    // Audio Capture Pin
    //
    &AudioCapturePinDispatch,
    NULL,
    { 
        NULL,                               // Interfaces (NULL, 0 == default)
        0,                      
        NULL,                               // Mediums (NULL, 0 == default)
        0,
        0,                                  // Range count (filled in later)
        NULL,                               // Ranges (filled in later)
        KSPIN_DATAFLOW_OUT,                 // Dataflow
        KSPIN_COMMUNICATION_BOTH,           // Communication
        &KSCATEGORY_AUDIO,                  // Category
        &g_PINNAME_AUDIO_CAPTURE,           // Name
        0                                   // Reserved
    },
    KSPIN_FLAG_FRAMES_NOT_REQUIRED_FOR_PROCESSING | // Flags
        KSPIN_FLAG_DO_NOT_INITIATE_PROCESSING | 
        KSPIN_FLAG_PROCESS_IN_RUN_STATE_ONLY |
        KSPIN_FLAG_FIXED_FORMAT,
    1,                                      // Instances Possible
    0,                                      // Instances Necessary
    &AudioDefaultAllocatorFraming,          // Allocator Framing (filled later)
    reinterpret_cast <PFNKSINTERSECTHANDLEREX> // Intersect Handler
        (CAudioCapturePin::IntersectHandler)
};

