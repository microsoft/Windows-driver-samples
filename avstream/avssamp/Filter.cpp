/**************************************************************************

    AVStream Filter-Centric Sample

    Copyright (c) 1999 - 2001, Microsoft Corporation

    File:

        filter.cpp

    Abstract:

        This file contails the capture filter implementation (including
        frame synthesis) for the fake capture filter.

    History:

        created 5/31/01

**************************************************************************/

#include "avssamp.h"

//
// TimerRoutine():
//
// This is the timer routine called every 1/Nth of a second to trigger
// capture by the filter.
//
KDEFERRED_ROUTINE TimerRoutine;
void
TimerRoutine (
    IN PKDPC Dpc,
    IN PVOID This,
    IN PVOID SystemArg1,
    IN PVOID SystemArg2
    )
{
    CCaptureFilter *pCCaptureFilter = (CCaptureFilter*)This;
    if (pCCaptureFilter)
    {
        pCCaptureFilter -> TimerDpc ();
    }
}


/**************************************************************************

    PAGEABLE CODE

**************************************************************************/

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA


CCaptureFilter::
CCaptureFilter (
    IN PKSFILTER Filter
    ) :
    m_Filter (Filter)

/*++

Routine Description:

    This is the constructor for the capture filter.  It initializes all the
    structures necessary to kick off timer DPC's for capture.

Arguments:

    Filter -
        The AVStream filter being created.

Return Value:

    None

--*/

{
    PAGED_CODE();

    //
    // Initialize the DPC's, timers, and events necessary to cause a 
    // capture trigger to happen.
    //
    KeInitializeDpc (
        &m_TimerDpc,
        TimerRoutine,
        this
        );

    KeInitializeEvent (
        &m_StopDPCEvent,
        SynchronizationEvent,
        FALSE
        );

    KeInitializeTimer (&m_Timer);

}

/*************************************************/


NTSTATUS
CCaptureFilter::
DispatchCreate (
    IN PKSFILTER Filter,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the creation dispatch for the capture filter.  It creates
    the CCaptureFilter object, associates it with the AVStream filter
    object, and bag the CCaptureFilter for later cleanup.

Arguments:

    Filter -
        The AVStream filter being created

    Irp -
        The creation Irp

Return Value:
    
    Success / failure

--*/

{

    PAGED_CODE();

    NTSTATUS Status = STATUS_SUCCESS;

    CCaptureFilter *CapFilter = new (NonPagedPool) CCaptureFilter (Filter);

    if (!CapFilter) {
        //
        // Return failure if we couldn't create the filter.
        //
        Status = STATUS_INSUFFICIENT_RESOURCES;

    } else {
        //
        // Add the item to the object bag if we we were successful. 
        // Whenever the filter closes, the bag is cleaned up and we will be
        // freed.
        //
        Status = KsAddItemToObjectBag (
            Filter -> Bag,
            reinterpret_cast <PVOID> (CapFilter),
            reinterpret_cast <PFNKSFREE> (CCaptureFilter::Cleanup)
            );

        if (!NT_SUCCESS (Status)) {
            delete CapFilter;
        } else {
            Filter -> Context = reinterpret_cast <PVOID> (CapFilter);
        }

    }

    //
    // Create the wave reader.  We need it at this point because the data
    // ranges exposed on the audio pin need to change dynamically right
    // now.
    //
    if (NT_SUCCESS (Status)) {

        CapFilter -> m_WaveObject =  
            new (NonPagedPool, 'evaW') CWaveObject (
                L"\\DosDevices\\c:\\avssamp.wav"
                );

        if (!CapFilter -> m_WaveObject) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        } else {
            Status = CapFilter -> m_WaveObject -> ParseAndRead ();

            //
            // If the file cannot be found, don't fail to create the filter.
            // This simply means that audio cannot be synthesized.
            //
            if (Status == STATUS_OBJECT_NAME_NOT_FOUND ||
                Status == STATUS_ACCESS_DENIED) {
                delete CapFilter -> m_WaveObject;
                CapFilter -> m_WaveObject = NULL;
                Status = STATUS_SUCCESS;
            }
            
        }

    }

    if (NT_SUCCESS (Status) && CapFilter -> m_WaveObject) {
        //
        // Add the wave object to the filter's bag for auto-cleanup.
        //
        Status = KsAddItemToObjectBag (
            Filter -> Bag,
            reinterpret_cast <PVOID> (CapFilter -> m_WaveObject),
            reinterpret_cast <PFNKSFREE> (CWaveObject::Cleanup)
            );

        if (!NT_SUCCESS (Status)) {
            delete CapFilter -> m_WaveObject;
            CapFilter -> m_WaveObject = NULL;
        } else {
            Status = CapFilter -> BindAudioToWaveObject ();
        }
    }

    return Status;

}

/*************************************************/

NTSTATUS
CCaptureFilter::
BindAudioToWaveObject (
    )

/*++

Routine Description:

    Create an audio pin directly bound to m_WaveObject (aka: it only exposes
    the format (channels, frequency, etc...) that m_WaveObject represents.
    This will actually create a pin on the filter dynamically.

Arguments:

    None

Return Value:

    Success / Failure

--*/

{

    PAGED_CODE();

    NT_ASSERT (m_WaveObject);

    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Build a pin descriptor from the template.  This descriptor is
    // temporary scratch space because the call to AVStream to create the
    // pin will actually duplicate the descriptor.
    //
    KSPIN_DESCRIPTOR_EX PinDescriptor = AudioPinDescriptorTemplate;

    //
    // The data range must be dynamically created since we're basing it
    // on dynamic reading of a wave file!
    //
    PKSDATARANGE_AUDIO DataRangeAudio = 
        reinterpret_cast <PKSDATARANGE_AUDIO> (
            ExAllocatePoolWithTag (PagedPool, sizeof (KSDATARANGE_AUDIO), AVSSMP_POOLTAG)
            );

    PKSDATARANGE_AUDIO *DataRanges =
        reinterpret_cast <PKSDATARANGE_AUDIO *> (
            ExAllocatePoolWithTag (PagedPool, sizeof (PKSDATARANGE_AUDIO), AVSSMP_POOLTAG)
            );

    PKSALLOCATOR_FRAMING_EX Framing =
        reinterpret_cast <PKSALLOCATOR_FRAMING_EX> (
            ExAllocatePoolWithTag (PagedPool, sizeof (KSALLOCATOR_FRAMING_EX), AVSSMP_POOLTAG)
            );

    if (DataRangeAudio && DataRanges && Framing) {
        DataRangeAudio -> DataRange.FormatSize = sizeof (KSDATARANGE_AUDIO);
        DataRangeAudio -> DataRange.Flags = 0;
        DataRangeAudio -> DataRange.SampleSize = 0;
        DataRangeAudio -> DataRange.Reserved = 0;
        DataRangeAudio -> DataRange.MajorFormat = KSDATAFORMAT_TYPE_AUDIO;
        DataRangeAudio -> DataRange.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
        DataRangeAudio -> DataRange.Specifier = 
            KSDATAFORMAT_SPECIFIER_WAVEFORMATEX;

        m_WaveObject -> WriteRange (DataRangeAudio);

        *DataRanges = DataRangeAudio;

    } else {        
        if (DataRangeAudio) {
            ExFreePool (DataRangeAudio);        
            DataRangeAudio = NULL;
        }
        if (DataRanges) {
            ExFreePool (DataRanges);        
            DataRanges = NULL;
        }
        if (Framing) {
            ExFreePool (Framing);
            Framing = NULL;
        }
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (NT_SUCCESS (Status)) {
        //
        // Bag the newly created range information in the filter's bag since
        // this will be alive for the lifetime of the filter.
        //
        Status = KsAddItemToObjectBag (
            m_Filter -> Bag,
            DataRangeAudio,
            NULL
            );

        if (!NT_SUCCESS (Status)) {
            ExFreePool (DataRangeAudio);
            ExFreePool (DataRanges);
            ExFreePool (Framing);
        }

    }

    if (NT_SUCCESS (Status)) {

        Status = KsAddItemToObjectBag (
            m_Filter -> Bag,
            DataRanges,
            NULL
            );

        if (!NT_SUCCESS (Status)) {
            ExFreePool (DataRanges);
            ExFreePool (Framing);
        }

    }

    if (NT_SUCCESS (Status)) {
        
        Status = KsAddItemToObjectBag (
            m_Filter -> Bag,
            Framing,
            NULL
            );

        if (!NT_SUCCESS (Status)) {
            ExFreePool (Framing);
        }

    }

    if (NT_SUCCESS (Status)) {
        //
        // The physical and optimal ranges must block aligned and 
        // the size of 1/(fps) * bytes_per_sec in size.  It's true
        // that we don't know the frame rate at this point due
        // to the fact that the video pin doesn't exist yet; however, that
        // would also be true if this were edited at audio pin creation.
        //
        // Thus, we instead adjust the allocator for the minimum frame rate
        // we support (which is 1/30 of a second).
        //
        *Framing = *PinDescriptor.AllocatorFraming;

        Framing -> FramingItem [0].PhysicalRange.MinFrameSize =
            Framing -> FramingItem [0].PhysicalRange.MaxFrameSize =
            Framing -> FramingItem [0].FramingRange.Range.MinFrameSize =
            Framing -> FramingItem [0].FramingRange.Range.MaxFrameSize =
                ((DataRangeAudio -> MaximumSampleFrequency *
                DataRangeAudio -> MaximumBitsPerSample *
                DataRangeAudio -> MaximumChannels) + 29) / 30;

        Framing -> FramingItem [0].PhysicalRange.Stepping = 
            Framing -> FramingItem [0].FramingRange.Range.Stepping =
            0;

        PinDescriptor.AllocatorFraming = Framing;

        PinDescriptor.PinDescriptor.DataRangesCount = 1;
        PinDescriptor.PinDescriptor.DataRanges = 
            reinterpret_cast <const PKSDATARANGE *> (DataRanges);

        //
        // Create the actual pin.  We need to save the pin id returned.  It
        // is how we refer to the audio pin in the future.
        //
        Status = KsFilterCreatePinFactory (
            m_Filter, 
            &PinDescriptor, 
            &m_AudioPinId
            );

    }

    return Status;

}


/*************************************************/


void
CCaptureFilter::
StartDPC (
    IN LONGLONG TimerInterval
    )

/*++

Routine Description:

    This routine starts the timer DPC running at a specified interval.  The 
    specified interval is the amount of time between triggering frame captures.
    Once this routine returns, the timer DPC should be running and attempting
    to trigger processing on the capture filter as a whole.

Arguments:

    TimerInterval -
        The amount of time between timer DPC's.  This is the amount of delay
        between one frame and the next.  Since the DPC is driven off the 
        video capture pin, this should be an amount of time specified by
        the video info header.

Return Value:

    None

--*/

{

    PAGED_CODE();

    //
    // Initialize any variables used by the timer DPC.
    //
    m_Tick = 0;
    m_TimerInterval = TimerInterval;
    KeQuerySystemTime (&m_StartTime);

    //
    // Schedule the DPC to happen one frame time from now.
    //
    LARGE_INTEGER NextTime;
    NextTime.QuadPart = m_StartTime.QuadPart + m_TimerInterval;

    KeSetTimer (&m_Timer, NextTime, &m_TimerDpc);

}

/*************************************************/


void
CCaptureFilter::
StopDPC (
    )

/*++

Routine Description:

    Stop the timer DPC from firing.  After this routine returns, there is
    a guarantee that no more timer DPC's will fire and no more processing
    attempts will occur.  Note that this routine does block.

Arguments:

    None

Return Value:

    None

--*/

{

    PAGED_CODE();

    m_StoppingDPC = TRUE;

    KeWaitForSingleObject (
        &m_StopDPCEvent,
        Suspended,
        KernelMode,
        FALSE,
        NULL
        );

    NT_ASSERT (m_StoppingDPC == FALSE);

}

/**************************************************************************

    LOCKED CODE

**************************************************************************/

#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif // ALLOC_PRAGMA


LONGLONG
CCaptureFilter::
GetTimerInterval (
    )

/*++

Routine Description:

    Return the timer interval being used to fire DPC's.

Arguments:

    None

Return Value:

    The timer interval being used to fire DPC's.

--*/

{

    return m_TimerInterval;

}

/*************************************************/


NTSTATUS
CCaptureFilter::
Process (
    IN PKSPROCESSPIN_INDEXENTRY ProcessPinsIndex
    )

/*++

Routine Description:

    This is the processing function for the capture filter.  It is responsible
    for copying synthesized image data into the image buffers.  The timer DPC
    will attempt to trigger processing (and hence indirectly call this routine)
    to trigger a capture.

Arguments:

    ProcessPinsIndex -
        Contains a pointer to an array of process pin index entries.  This
        array is indexed by pin ID.  An index entry indicates the number
        of pin instances for the corresponding filter type and points to the
        first corresponding process pin structure in the ProcessPins array.
        This allows the process pin structure to be quickly accessed by pin ID
        when the number of instances per type is not known in advance.

Return Value:

    Indication of whether more processing should be done if frames are
    available.  A value of STATUS_PENDING indicates that processing should not
    continue even if frames are available on all required queues. 
    STATUS_SUCCESS indicates processing should continue if frames are available
    on all required queues.

--*/

{

    //
    // The audio and video pins do not necessarily need to exist (one could
    // be capturing video w/o audio or vice-versa).  Do not assume the
    // existence by checking Index[ID].Pins[0].  Always check the Count
    // field first.
    //
    PKSPROCESSPIN VideoPin = NULL;
    CCapturePin *VidCapPin = NULL;
    PKSPROCESSPIN AudioPin = NULL;
    CCapturePin *AudCapPin = NULL;
    ULONG VidCapDrop = 0;
    ULONG AudCapDrop = (ULONG)-1;

    if (ProcessPinsIndex [VIDEO_PIN_ID].Count != 0) {
        //
        // There can be at most one instance via the possible instances field,
        // so the below is safe.
        //
        VideoPin = ProcessPinsIndex [VIDEO_PIN_ID].Pins [0];
        VidCapPin = 
            reinterpret_cast <CCapturePin *> (VideoPin -> Pin -> Context);
    }

    //
    // The audio pin only exists on the filter if the wave object does.
    // They're tied together at filter create time.
    //
    if (m_WaveObject && ProcessPinsIndex [m_AudioPinId].Count != 0) {
        //
        // There can be at most one instance via the possible instances field,
        // so the below is safe.
        //
        AudioPin = ProcessPinsIndex [m_AudioPinId].Pins [0];
        AudCapPin =
            reinterpret_cast <CCapturePin *> (AudioPin -> Pin -> Context);
    }

    if (VidCapPin) {
        VidCapDrop = VidCapPin -> QueryFrameDrop ();
    } 
    
    if (AudCapPin) {
        AudCapDrop = AudCapPin -> QueryFrameDrop ();
    }

    //
    // If there's a video pin around, trigger capture on it.  We call the
    // pin object to actually synthesize the frame; however, we could just
    // as easily have done that here.
    //
    if (VidCapPin) {
        //
        // This is used to notify the pin how many frames have been dropped
        // on each pin to allow that to be rendered.
        //
        VidCapPin -> NotifyDrops (VidCapDrop, AudCapDrop);
        VidCapPin -> CaptureFrame (VideoPin, m_Tick);
    }

    //
    // If there's an audio pin around, trigger capture on it.  Since the
    // audio capture pin isn't necessary for capture, there might be an
    // instance which is connected and is in the stop state when we get 
    // called [there will never be one in acquire or pause since we specify
    // KSPIN_FLAG_PROCESS_IN_RUN_STATE_ONLY].  Don't bother triggering capture
    // on the pin unless it's actually running.
    //
    // On DX8.x platforms, the Pin -> ClientState field does not exist.
    // Hence, we check the state we maintain ourselves.  DeviceState is not
    // the right thing to check here.
    //
    if (AudCapPin && AudCapPin -> GetState () == KSSTATE_RUN) {
        AudCapPin -> CaptureFrame (AudioPin, m_Tick);
    }

    //
    // STATUS_PENDING indicates that we do not want to be called back if
    // there is more data available.  We only want to trigger processing
    // (and hence capture) on the timer ticks.
    //
    return STATUS_PENDING;

}

/*************************************************/


void
CCaptureFilter::
TimerDpc (
    )

/*++

Routine Description:

    This is the timer function for our timer (bridged to from TimerRoutine
    in the context of the appropriate CCaptureFilter).  It is called every
    1/Nth of a second as specified in StartDpc() to trigger capture of a video
    frame.

Arguments:

    None

Return Value:

    None

--*/

{

    //
    // Increment the tick counter.  This keeps track of the number of ticks
    // that have happened since the timer DPC started running.  Note that the
    // timer DPC starts running before the pins go into run state and this
    // variable gets incremented from the original start point.
    //
    m_Tick++;

    //
    // Trigger processing on the filter.  Since the filter is prepared to
    // run at DPC, we do not request asynchronous processing.  Thus, if 
    // possible, processing will occur in the context of this DPC.
    //
    KsFilterAttemptProcessing (m_Filter, FALSE);

    //
    // Reschedule the timer if the hardware isn't being stopped.
    //
    if (!m_StoppingDPC) {
        
        LARGE_INTEGER NextTime;

        NextTime.QuadPart = m_StartTime.QuadPart +
            (m_TimerInterval * (m_Tick + 1));

        KeSetTimer (&m_Timer, NextTime, &m_TimerDpc);

    } else {

        //
        // If another thread is waiting on the DPC to stop running, raise
        // the stop event and clear the flag.
        //
        m_StoppingDPC = FALSE;
        KeSetEvent (&m_StopDPCEvent, IO_NO_INCREMENT, FALSE);

    }

}

/**************************************************************************

    DESCRIPTOR AND DISPATCH LAYOUT

**************************************************************************/

GUID g_PINNAME_VIDEO_CAPTURE = {STATIC_PINNAME_VIDEO_CAPTURE};

//
// CaptureFilterCategories:
//
// The list of category GUIDs for the capture filter.
//
const
GUID
CaptureFilterCategories [CAPTURE_FILTER_CATEGORIES_COUNT] = {
    STATICGUIDOF (KSCATEGORY_VIDEO),
    STATICGUIDOF (KSCATEGORY_CAPTURE)
};

//
// CaptureFilterPinDescriptors:
//
// The list of pin descriptors on the capture filter.  
//
const 
KSPIN_DESCRIPTOR_EX
CaptureFilterPinDescriptors [CAPTURE_FILTER_PIN_COUNT] = {
    //
    // Video Capture Pin
    //
    {
        &VideoCapturePinDispatch,
        NULL,             
        {
            NULL,                           // Interfaces (NULL, 0 == default)
            0,
            NULL,                           // Mediums (NULL, 0 == default)
            0,
            SIZEOF_ARRAY (VideoCapturePinDataRanges), // Range Count
            VideoCapturePinDataRanges,      // Ranges
            KSPIN_DATAFLOW_OUT,             // Dataflow
            KSPIN_COMMUNICATION_BOTH,       // Communication
            &KSCATEGORY_VIDEO,              // Category
            &g_PINNAME_VIDEO_CAPTURE,       // Name
            0                               // Reserved
        },
        KSPIN_FLAG_FRAMES_NOT_REQUIRED_FOR_PROCESSING | // Flags
            KSPIN_FLAG_DO_NOT_INITIATE_PROCESSING |
            KSPIN_FLAG_PROCESS_IN_RUN_STATE_ONLY,
        1,                                  // Instances Possible
        1,                                  // Instances Necessary
        &VideoCapturePinAllocatorFraming,   // Allocator Framing
        reinterpret_cast <PFNKSINTERSECTHANDLEREX> 
            (CVideoCapturePin::IntersectHandler)
    }
};

//
// CaptureFilterDispatch:
//
// This is the dispatch table for the capture filter.  It provides notification
// of creation, closure, processing, and resets.
//
const 
KSFILTER_DISPATCH
CaptureFilterDispatch = {
    CCaptureFilter::DispatchCreate,         // Filter Create
    NULL,                                   // Filter Close
    CCaptureFilter::DispatchProcess,        // Filter Process
    NULL                                    // Filter Reset
};

//
// CaptureFilterDescription:
//
// The descriptor for the capture filter.  We don't specify any topology
// since there's only one pin on the filter.  Realistically, there would
// be some topological relationships here because there would be input 
// pins from crossbars and the like.
//
const 
KSFILTER_DESCRIPTOR 
CaptureFilterDescriptor = {
    &CaptureFilterDispatch,                 // Dispatch Table
    NULL,                                   // Automation Table
    KSFILTER_DESCRIPTOR_VERSION,            // Version
    KSFILTER_FLAG_DISPATCH_LEVEL_PROCESSING,// Flags
    &KSNAME_Filter,                         // Reference GUID
    DEFINE_KSFILTER_PIN_DESCRIPTORS (CaptureFilterPinDescriptors),
    DEFINE_KSFILTER_CATEGORIES (CaptureFilterCategories),

    DEFINE_KSFILTER_NODE_DESCRIPTORS_NULL,
    DEFINE_KSFILTER_DEFAULT_CONNECTIONS,

    NULL                                    // Component ID
};


