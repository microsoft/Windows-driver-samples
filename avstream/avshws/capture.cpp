/**************************************************************************

    AVStream Simulated Hardware Sample

    Copyright (c) 2001, Microsoft Corporation.

    File:

        capture.cpp

    Abstract:

        This file contains source for the video capture pin on the capture
        filter.  The capture sample performs "fake" DMA directly into
        the capture buffers.  Common buffer DMA will work slightly differently.

        For common buffer DMA, the general technique would be DPC schedules
        processing with KsPinAttemptProcessing.  The processing routine grabs
        the leading edge, copies data out of the common buffer and advances.
        Cloning would not be necessary with this technique.  It would be 
        similiar to the way "AVSSamp" works, but it would be pin-centric.

    History:

        created 3/8/2001

**************************************************************************/

#include "avshws.h"
#include <ksmedia.h>
#include "ntintsafe.h"

/**************************************************************************

    PAGEABLE CODE

**************************************************************************/


#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA

#define DMAX_X 320
#define DMAX_Y 240
#define D_X 320
#define D_Y 240

CCapturePin::
CCapturePin (
    IN PKSPIN Pin
    ) :
    m_Pin (Pin)
    ,m_PresentationTime (0)

/*++

Routine Description:

    Construct a new capture pin.

Arguments:

    Pin -
        The AVStream pin object corresponding to the capture pin

Return Value:

    None

--*/

{

    PAGED_CODE();

    PKSDEVICE Device = KsPinGetDevice (Pin);

    //
    // Set up our device pointer.  This gives us access to "hardware I/O"
    // during the capture routines.
    //
    m_Device = reinterpret_cast <CCaptureDevice *> (Device -> Context);
}

/*************************************************/


NTSTATUS
CCapturePin::
DispatchCreate (
    IN PKSPIN Pin,
    IN PIRP Irp
    )

/*++

Routine Description:

    Create a new capture pin.  This is the creation dispatch for
    the video capture pin.

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

    CCapturePin *CapPin = new (NonPagedPoolNx, 'niPC') CCapturePin (Pin);

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
            reinterpret_cast <PVOID> (CapPin),
            reinterpret_cast <PFNKSFREE> (CCapturePin::Cleanup)
            );

        if (!NT_SUCCESS (Status)) {
            delete CapPin;
        } else {
            Pin -> Context = reinterpret_cast <PVOID> (CapPin);
        }

    }

    //
    // If we succeeded so far, stash the video info header away and change
    // our allocator framing to reflect the fact that only now do we know
    // the framing requirements based on the connection format.
    //
    PKS_VIDEOINFOHEADER VideoInfoHeader = NULL;

    if (NT_SUCCESS (Status)) {

        VideoInfoHeader = CapPin -> CaptureVideoInfoHeader ();
        if (!VideoInfoHeader) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if (NT_SUCCESS(Status)) {
        //
        // We need to edit the descriptor to ensure we don't mess up any other
        // pins using the descriptor or touch read-only memory.
        //
        Status = KsEdit (
            Pin, 
            &Pin -> Descriptor, 
            AVSHWS_POOLTAG);

        if (NT_SUCCESS (Status)) { 

            //
            // If the edits proceeded without running out of memory, adjust 
            // the framing based on the video info header.
            //
            Status = KsEdit (
                Pin, 
                &Pin -> Descriptor -> AllocatorFraming, 
                AVSHWS_POOLTAG);

            if (NT_SUCCESS (Status)) {

                //
                // We've KsEdit'ed this...  I'm safe to cast away constness as
                // long as the edit succeeded.
                //
                PKSALLOCATOR_FRAMING_EX Framing =
                    const_cast <PKSALLOCATOR_FRAMING_EX> (
                        Pin -> Descriptor -> AllocatorFraming
                        );

                Framing -> FramingItem [0].Frames = 2;

                //
                // The physical and optimal ranges must be biSizeImage.  We only
                // support one frame size, precisely the size of each capture
                // image.
                //
                Framing -> FramingItem [0].PhysicalRange.MinFrameSize =
                    Framing -> FramingItem [0].PhysicalRange.MaxFrameSize =
                    Framing -> FramingItem [0].FramingRange.Range.MinFrameSize =
                    Framing -> FramingItem [0].FramingRange.Range.MaxFrameSize =
                    VideoInfoHeader -> bmiHeader.biSizeImage;

                Framing -> FramingItem [0].PhysicalRange.Stepping = 
                    Framing -> FramingItem [0].FramingRange.Range.Stepping =
                    0;

            }

        }
    }

    if (NT_SUCCESS (Status)) {
        //
        // Adjust the stream header size.  The video packets have extended
        // header info (KS_FRAME_INFO).
        //
        Pin -> StreamHeaderSize = sizeof (KSSTREAM_HEADER) +
            sizeof (KS_FRAME_INFO);

    }
    return Status;
}

/*************************************************/


PKS_VIDEOINFOHEADER 
CCapturePin::
CaptureVideoInfoHeader (
    )

/*++

Routine Description:

    Capture the video info header out of the connection format.  This
    is what we use to base synthesized images off.

Arguments:

    None

Return Value:

    The captured video info header or NULL if there is insufficient
    memory.

--*/

{

    PAGED_CODE();

    PKS_VIDEOINFOHEADER ConnectionHeader =
        &((reinterpret_cast <PKS_DATAFORMAT_VIDEOINFOHEADER> 
            (m_Pin -> ConnectionFormat)) -> 
            VideoInfoHeader);

    m_VideoInfoHeader = reinterpret_cast <PKS_VIDEOINFOHEADER> (
        ExAllocatePoolWithTag (
            NonPagedPoolNx,
            KS_SIZE_VIDEOHEADER (ConnectionHeader),
            AVSHWS_POOLTAG
            )
        );

    if (!m_VideoInfoHeader)
        return NULL;

    //
    // Bag the newly allocated header space.  This will get cleaned up
    // automatically when the pin closes.
    //
    NTSTATUS Status =
        KsAddItemToObjectBag (
            m_Pin -> Bag,
            reinterpret_cast <PVOID> (m_VideoInfoHeader),
            NULL
            );

    if (!NT_SUCCESS (Status)) {

        ExFreePool (m_VideoInfoHeader);
        return NULL;

    } else {

        //
        // Copy the connection format video info header into the newly 
        // allocated "captured" video info header.
        //
        RtlCopyMemory (
            m_VideoInfoHeader,
            ConnectionHeader,
            KS_SIZE_VIDEOHEADER (ConnectionHeader)
            );

    }

    return m_VideoInfoHeader;

}


NTSTATUS
CCapturePin::
Process (
    )

/*++

Routine Description:

    The process dispatch for the pin bridges to this location.
    We handle setting up scatter gather mappings, etc...

Arguments:

    None

Return Value:

    Success / Failure

--*/

{

    PAGED_CODE();

    NTSTATUS Status = STATUS_SUCCESS;
    PKSSTREAM_POINTER Leading;

    _DbgPrintF(DEBUGLVL_VERBOSE, ("Process"));

    Leading = KsPinGetLeadingEdgeStreamPointer (
        m_Pin,
        KSSTREAM_POINTER_STATE_LOCKED
        );

    while (NT_SUCCESS (Status) && Leading) {

        PKSSTREAM_POINTER ClonePointer;
        PSTREAM_POINTER_CONTEXT SPContext = NULL;

        //
        // If no data is present in the Leading edge stream pointer, just 
        // move on to the next frame
        //
        if ( NULL == Leading -> StreamHeader -> Data ) {
            Status = KsStreamPointerAdvance(Leading);
            continue;
        }
        //
        // For optimization sake in this particular sample, I will only keep
        // one clone stream pointer per frame.  This complicates the logic
        // here but simplifies the completions.
        //
        // I'm also choosing to do this since I need to keep track of the
        // virtual addresses corresponding to each mapping since I'm faking
        // DMA.  It simplifies that too.
        //
        if (!m_PreviousStreamPointer) {
            //
            // First thing we need to do is clone the leading edge.  This allows
            // us to keep reference on the frames while they're in DMA.
            //
            Status = KsStreamPointerClone (
                Leading,
                NULL,
                sizeof (STREAM_POINTER_CONTEXT),
                &ClonePointer
                );

            //
            // I use this for easy chunking of the buffer.  We're not really
            // dealing with physical addresses.  This keeps track of what 
            // virtual address in the buffer the current scatter / gather 
            // mapping corresponds to for the fake hardware.
            //
            if (NT_SUCCESS (Status)) {

                //
                // Set the stream header data used to 0.  We update this 
                // in the DMA completions.  For queues with DMA, we must
                // update this field ourselves.
                //
                ClonePointer -> StreamHeader -> DataUsed = 0;

                SPContext = reinterpret_cast <PSTREAM_POINTER_CONTEXT> 
                    (ClonePointer -> Context);

                SPContext -> BufferVirtual = 
                    reinterpret_cast <PUCHAR> (
                        ClonePointer -> StreamHeader -> Data
                        );
            }

        } else {

            ClonePointer = m_PreviousStreamPointer;
            SPContext = reinterpret_cast <PSTREAM_POINTER_CONTEXT> 
                (ClonePointer -> Context);
            Status = STATUS_SUCCESS;
        }

        //
        // If the clone failed, likely we're out of resources.  Break out
        // of the loop for now.  We may end up starving DMA.
        //
        if (!NT_SUCCESS (Status)) {
            KsStreamPointerUnlock (Leading, FALSE);
            break;
        }

        //
        // Program the fake hardware.  I would use Clone -> OffsetOut.*, but
        // because of the optimization of one stream pointer per frame, it
        // doesn't make complete sense.
        //
        ULONG MappingsUsed =
            m_Device -> ProgramScatterGatherMappings (
                ClonePointer,
                &(SPContext -> BufferVirtual),
                Leading -> OffsetOut.Mappings,
                Leading -> OffsetOut.Remaining
                );

        //
        // In order to keep one clone per frame and simplify the fake DMA
        // logic, make a check to see if we completely used the mappings in
        // the leading edge.  Set a flag.
        //
        if (MappingsUsed == Leading -> OffsetOut.Remaining) {
            m_PreviousStreamPointer = NULL;
        } else {
            m_PreviousStreamPointer = ClonePointer;
        }

        if (MappingsUsed) {
            //
            // If any mappings were added to scatter / gather queues, 
            // advance the leading edge by that number of mappings.  If 
            // we run off the end of the queue, Status will be 
            // STATUS_DEVICE_NOT_READY.  Otherwise, the leading edge will
            // point to a new frame.  The previous one will not have been
            // dismissed (unless "DMA" completed) since there's a clone
            // pointer referencing the frames.
            //
            Status =
                KsStreamPointerAdvanceOffsets (
                    Leading,
                    0,
                    MappingsUsed,
                    FALSE
                    );
        } else {

            //
            // The hardware was incapable of adding more entries.  The S/G
            // table is full.
            //
            Status = STATUS_PENDING;
            break;

        }

    }

    //
    // If the leading edge failed to lock (this is always possible, remember
    // that locking CAN occassionally fail), don't blow up passing NULL
    // into KsStreamPointerUnlock.  Also, set m_PendIo to kick us later...
    //
    if (!Leading) {

        m_PendIo = TRUE;

        //
        // If the lock failed, there's no point in getting called back 
        // immediately.  The lock could fail due to insufficient memory,
        // etc...  In this case, we don't want to get called back immediately.
        // Return pending.  The m_PendIo flag will cause us to get kicked
        // later.
        //
        Status = STATUS_PENDING;
    }

    //
    // If we didn't run the leading edge off the end of the queue, unlock it.
    //
    if (NT_SUCCESS (Status) && Leading) {
        KsStreamPointerUnlock (Leading, FALSE);
    } else {
        //
        // DEVICE_NOT_READY indicates that the advancement ran off the end
        // of the queue.  We couldn't lock the leading edge.
        //
        if (Status == STATUS_DEVICE_NOT_READY) Status = STATUS_SUCCESS;
    }

    //
    // If we failed with something that requires pending, set the pending I/O
    // flag so we know we need to start it again in a completion DPC.
    //
    if (!NT_SUCCESS (Status) || Status == STATUS_PENDING) {
        m_PendIo = TRUE;
    }

    _DbgPrintF(DEBUGLVL_VERBOSE, ("Leaving Process..."));
    return Status;

}

/*************************************************/


NTSTATUS
CCapturePin::
CleanupReferences (
    )

/*++

Routine Description:

    Clean up any references we're holding on frames after we abruptly
    stop the hardware.

Arguments:

    None

Return Value:

    Success / Failure

--*/

{

    PAGED_CODE();

    PKSSTREAM_POINTER Clone = KsPinGetFirstCloneStreamPointer (m_Pin);
    PKSSTREAM_POINTER NextClone = NULL;

    //
    // Walk through the clones, deleting them, and setting DataUsed to
    // zero since we didn't use any data!
    //
    while (Clone) {

        NextClone = KsStreamPointerGetNextClone (Clone);

        Clone -> StreamHeader -> DataUsed = 0;
        KsStreamPointerDelete (Clone);

        Clone = NextClone;

    }

    return STATUS_SUCCESS;

}

/*************************************************/


NTSTATUS
CCapturePin::
SetState (
    IN KSSTATE ToState,
    IN KSSTATE FromState
    )

/*++

Routine Description:

    This is called when the caputre pin transitions state.  The routine
    attempts to acquire / release any hardware resources and start up
    or shut down capture based on the states we are transitioning to
    and away from.

Arguments:

    ToState -
        The state we're transitioning to

    FromState -
        The state we're transitioning away from

Return Value:

    Success / Failure

--*/

{

    PAGED_CODE();

    NTSTATUS Status = STATUS_SUCCESS;

    switch (ToState) {

        case KSSTATE_STOP:

            //
            // First, stop the hardware if we actually did anything to it.
            //
            if (m_HardwareState != HardwareStopped) {
                Status = m_Device -> Stop ();
                NT_ASSERT (NT_SUCCESS (Status));

                m_HardwareState = HardwareStopped;
            }

            //
            // We've stopped the "fake hardware".  It has cleared out
            // it's scatter / gather tables and will no longer be 
            // completing clones.  We had locks on some frames that were,
            // however, in hardware.  This will clean them up.  An
            // alternative location would be in the reset dispatch.
            // Note, however, that the reset dispatch can occur in any
            // state and this should be understood.
            //
            // Some hardware may fill all S/G mappings before stopping...
            // in this case, you may not have to do this.  The 
            // "fake hardware" here simply stops filling mappings and 
            // cleans its scatter / gather tables out on the Stop call.
            //
            Status = CleanupReferences ();

            //
            // Release any hardware resources related to this pin.
            //
            if (m_AcquiredResources) {
                //
                // If we got an interface to the clock, we must release it.
                //
                if (m_Clock) {
                    m_Clock -> Release ();
                    m_Clock = NULL;
                }

                m_Device -> ReleaseHardwareResources (
                    );

                m_AcquiredResources = FALSE;
            }

            break;

        case KSSTATE_ACQUIRE:
            //
            // Acquire any hardware resources related to this pin.  We should
            // only acquire them here -- **NOT** at filter create time. 
            // This means we do not fail creation of a filter because of
            // limited hardware resources.
            //
            if (FromState == KSSTATE_STOP) {
                Status = m_Device -> AcquireHardwareResources (
                    this,
                    m_VideoInfoHeader
                    );

                if (NT_SUCCESS (Status)) {
                    m_AcquiredResources = TRUE;

                    //
                    // Attempt to get an interface to the master clock.
                    // This will fail if one has not been assigned.  Since
                    // one must be assigned while the pin is still in 
                    // KSSTATE_STOP, this is a guranteed method of getting
                    // the clock should one be assigned.
                    //
                    if (!NT_SUCCESS (
                        KsPinGetReferenceClockInterface (
                            m_Pin,
                            &m_Clock
                            )
                        )) {

                        //
                        // If we could not get an interface to the clock,
                        // don't use one.  
                        //
                        m_Clock = NULL;

                    }

                } else {
                    m_AcquiredResources = FALSE;
                }

            } else {
                //
                // Standard transport pins will always receive transitions in
                // +/- 1 manner.  This means we'll always see a PAUSE->ACQUIRE
                // transition before stopping the pin.  
                //
                // The below is done because on DirectX 8.0, when the pin gets
                // a message to stop, the queue is inaccessible.  The reset 
                // which comes on every stop happens after this (at which time
                // the queue is inaccessible also).  So, for compatibility with
                // DirectX 8.0, I am stopping the "fake" hardware at this
                // point and cleaning up all references we have on frames.  See
                // the comments above regarding the CleanupReferences call.
                //
                // If this sample were targeting XP only, the below code would
                // not be here.  Again, I only do this so the sample does not
                // hang when it is stopped running on a configuration such as
                // Win2K + DX8. 
                //
                if (m_HardwareState != HardwareStopped) {
                    Status = m_Device -> Stop ();
                    NT_ASSERT (NT_SUCCESS (Status));

                    m_HardwareState = HardwareStopped;
                }

                Status = CleanupReferences ();
            }

            m_FrameNumber   = 0;
            m_DroppedFrames = 0;
            break;

        case KSSTATE_PAUSE:
            //
            // Stop the hardware simulation if we're coming down from run.
            //
            if (FromState == KSSTATE_RUN) {

                m_PresentationTime = 0;
                Status = m_Device -> Pause (TRUE);

                if (NT_SUCCESS (Status)) {
                    m_HardwareState = HardwarePaused;
                }

            }
            m_FrameNumber   = 0;
            break;

        case KSSTATE_RUN:
            //
            // Start the hardware simulation or unpause it depending on
            // whether we're initially running or we've paused and restarted.
            //
            if (m_HardwareState == HardwarePaused) {
                Status = m_Device -> Pause (FALSE);
            } else {
                Status = m_Device -> Start ();
            }

            if (NT_SUCCESS (Status)) {
                m_HardwareState = HardwareRunning;
            }

            break;

    }

    return Status;

}

/*************************************************/


NTSTATUS
CCapturePin::
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

    This routine handles video pin intersection queries by determining the
    intersection between two data ranges.

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

    const GUID VideoInfoSpecifier = 
        {STATICGUIDOF(KSDATAFORMAT_SPECIFIER_VIDEOINFO)};
    
    NT_ASSERT(Filter);
    NT_ASSERT(Irp);
    NT_ASSERT(PinInstance);
    NT_ASSERT(CallerDataRange);
    NT_ASSERT(DescriptorDataRange);
    NT_ASSERT(DataSize);
    
    ULONG DataFormatSize;
    
    //
    // Specifier FORMAT_VideoInfo for VIDEOINFOHEADER
    //
    if (IsEqualGUID(CallerDataRange->Specifier, VideoInfoSpecifier) &&
        CallerDataRange -> FormatSize >= sizeof (KS_DATARANGE_VIDEO)) {
            
        PKS_DATARANGE_VIDEO callerDataRange = 
            reinterpret_cast <PKS_DATARANGE_VIDEO> (CallerDataRange);

        PKS_DATARANGE_VIDEO descriptorDataRange = 
            reinterpret_cast <PKS_DATARANGE_VIDEO> (DescriptorDataRange);

        PKS_DATAFORMAT_VIDEOINFOHEADER FormatVideoInfoHeader;

        //
        // Check that the other fields match
        //
        if ((callerDataRange->bFixedSizeSamples != 
                descriptorDataRange->bFixedSizeSamples) ||
            (callerDataRange->bTemporalCompression != 
                descriptorDataRange->bTemporalCompression) ||
            (callerDataRange->StreamDescriptionFlags != 
                descriptorDataRange->StreamDescriptionFlags) ||
            (callerDataRange->MemoryAllocationFlags != 
                descriptorDataRange->MemoryAllocationFlags) ||
            (RtlCompareMemory (&callerDataRange->ConfigCaps,
                    &descriptorDataRange->ConfigCaps,
                    sizeof (KS_VIDEO_STREAM_CONFIG_CAPS)) != 
                    sizeof (KS_VIDEO_STREAM_CONFIG_CAPS))) 
        {
            return STATUS_NO_MATCH;
        }

        //
        // KS_SIZE_VIDEOHEADER() below is relying on bmiHeader.biSize from
        // the caller's data range.  This **MUST** be validated; the
        // extended bmiHeader size (biSize) must not extend past the end
        // of the range buffer.  Possible arithmetic overflow is also
        // checked for.
        //
        {
            ULONG VideoHeaderSize = KS_SIZE_VIDEOHEADER (
                &callerDataRange->VideoInfoHeader
                );

            ULONG DataRangeSize = 
                FIELD_OFFSET (KS_DATARANGE_VIDEO, VideoInfoHeader) +
                VideoHeaderSize;

            //
            // Check that biSize does not extend past the buffer.  The 
            // first two checks are for arithmetic overflow on the 
            // operations to compute the alleged size.  (On unsigned
            // math, a+b < a iff an arithmetic overflow occurred).
            //
            if (
                VideoHeaderSize < callerDataRange->
                    VideoInfoHeader.bmiHeader.biSize ||
                DataRangeSize < VideoHeaderSize ||
                DataRangeSize > callerDataRange -> DataRange.FormatSize
                ) {

                return STATUS_INVALID_PARAMETER;

            }

        }

        DataFormatSize = 
            sizeof (KSDATAFORMAT) + 
            KS_SIZE_VIDEOHEADER (&callerDataRange->VideoInfoHeader);

            
        //
        // If the passed buffer size is 0, it indicates that this is a size
        // only query.  Return the size of the intersecting data format and
        // pass back STATUS_BUFFER_OVERFLOW.
        //
        if (BufferSize == 0) {

            *DataSize = DataFormatSize;
            return STATUS_BUFFER_OVERFLOW;

        }
        
        //
        // Verify that the provided structure is large enough to
        // accept the result.
        //
        if (BufferSize < DataFormatSize) 
        {
            return STATUS_BUFFER_TOO_SMALL;
        }

        //
        // Copy over the KSDATAFORMAT, followed by the actual VideoInfoHeader
        //
        *DataSize = DataFormatSize;
            
        FormatVideoInfoHeader = PKS_DATAFORMAT_VIDEOINFOHEADER( Data );

        //
        // Copy over the KSDATAFORMAT.  This is precisely the same as the
        // KSDATARANGE (it's just the GUIDs, etc...  not the format information
        // following any data format.
        // 
        RtlCopyMemory (
            &FormatVideoInfoHeader->DataFormat, 
            DescriptorDataRange, 
            sizeof (KSDATAFORMAT));

        FormatVideoInfoHeader->DataFormat.FormatSize = DataFormatSize;

        //
        // Copy over the callers requested VIDEOINFOHEADER
        //

        RtlCopyMemory (
            &FormatVideoInfoHeader->VideoInfoHeader, 
            &callerDataRange->VideoInfoHeader,
            KS_SIZE_VIDEOHEADER (&callerDataRange->VideoInfoHeader) 
            );

        //
        // Calculate biSizeImage for this request, and put the result in both
        // the biSizeImage field of the bmiHeader AND in the SampleSize field
        // of the DataFormat.
        //
        // Note that for compressed sizes, this calculation will probably not
        // be just width * height * bitdepth
        //
        FormatVideoInfoHeader->VideoInfoHeader.bmiHeader.biSizeImage =
            FormatVideoInfoHeader->DataFormat.SampleSize = 
            KS_DIBSIZE (FormatVideoInfoHeader->VideoInfoHeader.bmiHeader);

        //
        // REVIEW - Perform other validation such as cropping and scaling checks
        // 
        
        return STATUS_SUCCESS;
        
    } // End of VIDEOINFOHEADER specifier
    
    return STATUS_NO_MATCH;
}

/*************************************************/

BOOL
MultiplyCheckOverflow (
    ULONG a,
    ULONG b,
    ULONG *pab
    )

/*++

Routine Description:

    Perform a 32 bit unsigned multiplication and check for arithmetic overflow.

Arguments:

    a -
        First operand

    b -
        Second operand

    pab -
        Result

Return Value:

    TRUE -
        no overflow

    FALSE -
        overflow occurred

--*/

{
    PAGED_CODE();

    *pab = a * b;
    if ((a == 0) || (((*pab) / a) == b)) {
        return TRUE;
    }
    return FALSE;
}

/*************************************************/


NTSTATUS
CCapturePin::
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

    NTSTATUS Status = STATUS_NO_MATCH;

    const GUID VideoInfoSpecifier = 
        {STATICGUIDOF(KSDATAFORMAT_SPECIFIER_VIDEOINFO)};

    CCapturePin *CapPin = NULL;

    //
    // Find the pin, if it exists yet.  OldFormat will be an indication of 
    // this.  If we're changing formats, OldFormat will be non-NULL.
    //
    // You cannot use Pin -> Context to make the determination.  AVStream
    // preinitializes this to the filter's context.
    //
    if (OldFormat) {
        CapPin = reinterpret_cast <CCapturePin *> (Pin -> Context);
    }

    if (IsEqualGUID (Pin -> ConnectionFormat -> Specifier,
            VideoInfoSpecifier) &&
        Pin -> ConnectionFormat -> FormatSize >=
            sizeof (KS_DATAFORMAT_VIDEOINFOHEADER)) {

        PKS_DATAFORMAT_VIDEOINFOHEADER ConnectionFormat =
            reinterpret_cast <PKS_DATAFORMAT_VIDEOINFOHEADER> 
                (Pin -> ConnectionFormat);

        //
        // DataRange comes out of OUR data range list.  I know the range
        // is valid as such.
        //
        const KS_DATARANGE_VIDEO *VIRange =
            reinterpret_cast <const KS_DATARANGE_VIDEO *>
                (DataRange);

        //
        // Check that bmiHeader.biSize is valid since we use it later.
        //
        ULONG VideoHeaderSize = KS_SIZE_VIDEOHEADER (
            &ConnectionFormat -> VideoInfoHeader
            );

        ULONG DataFormatSize = FIELD_OFFSET (
            KS_DATAFORMAT_VIDEOINFOHEADER, VideoInfoHeader
            ) + VideoHeaderSize;

        if (
            VideoHeaderSize < ConnectionFormat->
                VideoInfoHeader.bmiHeader.biSize ||
            DataFormatSize < VideoHeaderSize ||
            DataFormatSize > ConnectionFormat -> DataFormat.FormatSize
            ) {

            Status = STATUS_INVALID_PARAMETER;

        }

        //
        // Check that the format is a match for the selected range. 
        //
        else if (
            (ConnectionFormat -> VideoInfoHeader.bmiHeader.biWidth !=
                VIRange -> VideoInfoHeader.bmiHeader.biWidth) ||

            (ConnectionFormat -> VideoInfoHeader.bmiHeader.biHeight !=
                VIRange -> VideoInfoHeader.bmiHeader.biHeight) ||

            (ConnectionFormat -> VideoInfoHeader.bmiHeader.biCompression !=
                VIRange -> VideoInfoHeader.bmiHeader.biCompression) 

            ) {

            Status = STATUS_NO_MATCH;

        } else {

            //
            // Compute the minimum size of our buffers to validate against.
            // The image synthesis routines synthesize |biHeight| rows of
            // biWidth pixels in either RGB24 or UYVY.  In order to ensure
            // safe synthesis into the buffer, we need to know how large an
            // image this will produce.
            //
            // I do this explicitly because of the method that the data is
            // synthesized.  A variation of this may or may not be necessary
            // depending on the mechanism the driver in question fills the 
            // capture buffers.  The important thing is to ensure that they
            // aren't overrun during capture.
            //
            ULONG ImageSize;

            if (!MultiplyCheckOverflow (
                (ULONG)ConnectionFormat->VideoInfoHeader.bmiHeader.biWidth,
                (ULONG)abs (ConnectionFormat->
                    VideoInfoHeader.bmiHeader.biHeight),
                &ImageSize
                )) {

                Status = STATUS_INVALID_PARAMETER;
            }

            //
            // We only support KS_BI_RGB (24) and KS_BI_YUV422 (16), so
            // this is valid for those formats.
            //
            else if (!MultiplyCheckOverflow (
                ImageSize,
                (ULONG)(ConnectionFormat->
                    VideoInfoHeader.bmiHeader.biBitCount / 8),
                &ImageSize
                )) {

                Status = STATUS_INVALID_PARAMETER;

            }

            //
            // Valid for the formats we use.  Otherwise, this would be
            // checked later.
            //
            else if (ConnectionFormat->VideoInfoHeader.bmiHeader.biSizeImage <
                    ImageSize) {

                Status = STATUS_INVALID_PARAMETER;

            } else {

                //
                // We can accept the format. 
                //
                Status = STATUS_SUCCESS;

                //
                // OldFormat is an indication that this is a format change.  
                // Since I do not implement the 
                // KSPROPERTY_CONNECTION_PROPOSEDATAFORMAT, by default, I do 
                // not handle dynamic format changes.
                //
                // If something changes while we're in the stop state, we're 
                // fine to handle it since we haven't "configured the hardware"
                // yet.
                //
                if (OldFormat) {
                    //
                    // If we're in the stop state, we can handle just about any
                    // change.  We don't support dynamic format changes. 
                    //
                    if (Pin -> DeviceState == KSSTATE_STOP) {
                        if (!CapPin -> CaptureVideoInfoHeader ()) {
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                        }
                    } else {
                        //
                        // Because we don't accept dynamic format changes, we
                        // should never get here.  Just being over-protective.
                        //
                        Status = STATUS_INVALID_DEVICE_STATE;
                    }
                }

            }

        }

    }
    return Status;
}

/**************************************************************************

    LOCKED CODE

**************************************************************************/

#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif // ALLOC_PRAGMA

void
CCapturePin::
CompleteMappings (
    IN ULONG NumMappings
    )

/*++

Routine Description:

    Called to notify the pin that a given number of scatter / gather
    mappings have completed.  Let the buffers go if possible.
    We're called at DPC.

Arguments:

    NumMappings -
        The number of mappings that have completed.

Return Value:

    None

--*/

{

    ULONG MappingsRemaining = NumMappings;

    //
    // Walk through the clones list and delete clones whose time has come.
    // The list is guaranteed to be kept in the order they were cloned.
    //
    PKSSTREAM_POINTER Clone = KsPinGetFirstCloneStreamPointer (m_Pin);

    while (MappingsRemaining && Clone) {

        PKSSTREAM_POINTER NextClone = KsStreamPointerGetNextClone (Clone);

#if defined(_X86_)
        //
        // Count up the number of bytes we've completed and mark this
        // in the Stream Header.  In mapped queues 
        // (KSPIN_FLAG_GENERATE_MAPPINGS), this is the responsibility of
        // the minidriver.  In non-mapped queues, AVStream performs this.
        //
        ULONG MappingsToCount = 
            (MappingsRemaining > Clone -> OffsetOut.Remaining) ?
                 Clone -> OffsetOut.Remaining :
                 MappingsRemaining;

        //
        // Update DataUsed according to the mappings.
        //
        for (ULONG CurMapping = 0; CurMapping < MappingsToCount; CurMapping++) {
            Clone -> StreamHeader -> DataUsed +=
                Clone -> OffsetOut.Mappings [CurMapping].ByteCount;
        }
#endif

        // 
        // If we have completed all remaining mappings in this clone, it
        // is an indication that the clone is ready to be deleted and the
        // buffer released.  Set anything required in the stream header which
        // has not yet been set.  If we have a clock, we can timestamp the
        // sample.
        //
#if !defined(_X86_)
        if (Clone -> StreamHeader -> DataUsed >= Clone -> OffsetOut.Remaining) {
#else
        if (MappingsRemaining >= Clone -> OffsetOut.Remaining) {
#endif
            Clone -> StreamHeader -> Duration =
                m_VideoInfoHeader -> AvgTimePerFrame;

            Clone -> StreamHeader -> PresentationTime.Numerator =
                Clone -> StreamHeader -> PresentationTime.Denominator = 1;

            //
            // If a clock has been assigned, timestamp the packets with the
            // time shown on the clock. 
            //
            if (m_Clock) {

                LONGLONG ClockTime = m_Clock -> GetTime ();

                Clone -> StreamHeader -> PresentationTime.Time = ClockTime;

                Clone -> StreamHeader -> OptionsFlags =
                    KSSTREAM_HEADER_OPTIONSF_TIMEVALID |
                    KSSTREAM_HEADER_OPTIONSF_DURATIONVALID;

            } else {
	      //
	      // If there is no clock, don't time stamp the packets.
	      //
	      Clone -> StreamHeader -> PresentationTime.Time = 0;
	      
            }

            //
            // Increment the frame number.  This is the total count of frames which
            // have attempted capture.
            //
            m_FrameNumber++;

            //
            // Double check the Stream Header size.  AVStream makes no guarantee
            // that because StreamHeaderSize is set to a specific size that you
            // will get that size.  If the proper data type handlers are not 
            // installed, the stream header will be of default size.
            //
            if ( Clone -> StreamHeader -> Size >= sizeof (KSSTREAM_HEADER) +
                sizeof (KS_FRAME_INFO)) {

                PKS_FRAME_INFO FrameInfo = reinterpret_cast <PKS_FRAME_INFO> (
                    Clone -> StreamHeader + 1
                    );
    
                FrameInfo -> ExtendedHeaderSize = sizeof (KS_FRAME_INFO);
                FrameInfo -> dwFrameFlags       = KS_VIDEO_FLAG_FRAME;
                FrameInfo -> PictureNumber      = (LONGLONG)m_FrameNumber;

                // I don't really have a way to tell if the device has dropped a frame 
                // or was not able to send a frame on time.
                FrameInfo -> DropCount = (LONGLONG)m_DroppedFrames;
            }


            //
            // If all of the mappings in this clone have been completed,
            // delete the clone.  We've already updated DataUsed above.
            //

#if !defined(_X86_)
            MappingsRemaining--;
#else
            MappingsRemaining -= Clone -> OffsetOut.Remaining;
#endif
            KsStreamPointerDelete (Clone);

        } else {
            //
            // If only part of the mappings in this clone have been completed,
            // update the pointers.  Since we're guaranteed this won't advance
            // to a new frame by the check above, it won't fail.
            //
#if !defined(_X86_)
            (void)KsStreamPointerAdvanceOffsets (
                Clone,
                0,
                Clone -> StreamHeader -> DataUsed,
                FALSE
                );

#else
            (void)KsStreamPointerAdvanceOffsets (
                Clone,
                0,
                MappingsRemaining,
                FALSE
                );

#endif
            MappingsRemaining = 0;

        }

        //
        // Go to the next clone.
        //
        Clone = NextClone;

    }

    //
    // If we've used all the mappings in hardware and pended, we can kick
    // processing to happen again if we've completed mappings.
    //
    if (m_PendIo) {
        m_PendIo = TRUE;
        KsPinAttemptProcessing (m_Pin, TRUE);
    }

}

/**************************************************************************

    DISPATCH AND DESCRIPTOR LAYOUT

**************************************************************************/

//
// FormatRGB24Bpp_Capture:
//
// This is the data range description of the RGB24 capture format we support.
//
const 
KS_DATARANGE_VIDEO 
FormatRGB24Bpp_Capture = {

    //
    // KSDATARANGE
    //
    {   
        sizeof (KS_DATARANGE_VIDEO),                // FormatSize
        0,                                          // Flags
        D_X * D_Y * 3,                              // SampleSize
        0,                                          // Reserved

        STATICGUIDOF (KSDATAFORMAT_TYPE_VIDEO),     // aka. MEDIATYPE_Video
        0xe436eb7d, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 
            0xaf, 0x0b, 0xa7, 0x70,                 // aka. MEDIASUBTYPE_RGB24,
        STATICGUIDOF (KSDATAFORMAT_SPECIFIER_VIDEOINFO) // aka. FORMAT_VideoInfo
    },

    TRUE,               // BOOL,  bFixedSizeSamples (all samples same size?)
    FALSE,              // BOOL,  bTemporalCompression (all I frames?)
    0,                  // Reserved (was StreamDescriptionFlags)
    0,                  // Reserved (was MemoryAllocationFlags   
                        //           (KS_VIDEO_ALLOC_*))
    //
    // _KS_VIDEO_STREAM_CONFIG_CAPS  
    //
    {
        STATICGUIDOF( KSDATAFORMAT_SPECIFIER_VIDEOINFO ), // GUID
        KS_AnalogVideo_None,                            // AnalogVideoStandard
        D_X,D_Y,        // InputSize, (the inherent size of the incoming signal
                        //             with every digitized pixel unique)
        D_X,D_Y,        // MinCroppingSize, smallest rcSrc cropping rect allowed
        D_X,D_Y,        // MaxCroppingSize, largest  rcSrc cropping rect allowed
        8,              // CropGranularityX, granularity of cropping size
        1,              // CropGranularityY
        8,              // CropAlignX, alignment of cropping rect 
        1,              // CropAlignY;
        D_X, D_Y,       // MinOutputSize, smallest bitmap stream can produce
        D_X, D_Y,       // MaxOutputSize, largest  bitmap stream can produce
        8,              // OutputGranularityX, granularity of output bitmap size
        1,              // OutputGranularityY;
        0,              // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,              // StretchTapsY
        0,              // ShrinkTapsX 
        0,              // ShrinkTapsY 
        333667,         // MinFrameInterval, 100 nS units
        640000000,      // MaxFrameInterval, 100 nS units
        8 * 3 * 30 * D_X * D_Y,  // MinBitsPerSecond;
        8 * 3 * 30 * D_X * D_Y   // MaxBitsPerSecond;
    }, 
        
    //
    // KS_VIDEOINFOHEADER (default format)
    //
    {
        0,0,0,0,                            // RECT  rcSource; 
        0,0,0,0,                            // RECT  rcTarget; 
        D_X * D_Y * 3 * 8 * 30,             // DWORD dwBitRate;
        0L,                                 // DWORD dwBitErrorRate; 
        333667,                             // REFERENCE_TIME  AvgTimePerFrame;   
        sizeof (KS_BITMAPINFOHEADER),       // DWORD biSize;
        D_X,                                // LONG  biWidth;
        D_Y,                                // LONG  biHeight;
        1,                                  // WORD  biPlanes;
        24,                                 // WORD  biBitCount;
        KS_BI_RGB,                          // DWORD biCompression;
        D_X * D_Y * 3,                      // DWORD biSizeImage;
        0,                                  // LONG  biXPelsPerMeter;
        0,                                  // LONG  biYPelsPerMeter;
        0,                                  // DWORD biClrUsed;
        0                                   // DWORD biClrImportant;
    }
}; 

//
// FormatYUY2_Capture:
//
// This is the data range description of the YUY2 format we support.
//
const 
KS_DATARANGE_VIDEO 
FormatYUY2_Capture = {

    //
    // KSDATARANGE
    //
    {   
        sizeof (KS_DATARANGE_VIDEO),            // FormatSize
        0,                                      // Flags
        DMAX_X * DMAX_Y * 2,                    // SampleSize
        0,                                      // Reserved
        STATICGUIDOF (KSDATAFORMAT_TYPE_VIDEO), // aka. MEDIATYPE_Video
        0x32595559, 0x0000, 0x0010, 0x80, 0x00, 
        0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71,     //aka. MEDIASUBTYPE_YUY2,
        STATICGUIDOF (KSDATAFORMAT_SPECIFIER_VIDEOINFO) // aka. FORMAT_VideoInfo
    },

    TRUE,               // BOOL,  bFixedSizeSamples (all samples same size?)
    FALSE,              // BOOL,  bTemporalCompression (all I frames?)
    0,                  // Reserved (was StreamDescriptionFlags)
    0,                  // Reserved (was MemoryAllocationFlags   
                        //           (KS_VIDEO_ALLOC_*))

    //
    // _KS_VIDEO_STREAM_CONFIG_CAPS  
    //
    {
        STATICGUIDOF( KSDATAFORMAT_SPECIFIER_VIDEOINFO ), // GUID
        KS_AnalogVideo_None,                            // AnalogVideoStandard
        DMAX_X, DMAX_Y, // InputSize, (the inherent size of the incoming signal
                        //             with every digitized pixel unique)
        D_X,D_Y,        // MinCroppingSize, smallest rcSrc cropping rect allowed
        DMAX_X, DMAX_Y, // MaxCroppingSize, largest  rcSrc cropping rect allowed
        8,              // CropGranularityX, granularity of cropping size
        1,              // CropGranularityY
        8,              // CropAlignX, alignment of cropping rect 
        1,              // CropAlignY;
        D_X, D_Y,       // MinOutputSize, smallest bitmap stream can produce
        DMAX_X, DMAX_Y, // MaxOutputSize, largest  bitmap stream can produce
        8,              // OutputGranularityX, granularity of output bitmap size
        1,              // OutputGranularityY;
        0,              // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,              // StretchTapsY
        0,              // ShrinkTapsX 
        0,              // ShrinkTapsY 
        333667,         // MinFrameInterval, 100 nS units
        640000000,      // MaxFrameInterval, 100 nS units
        8 * 2 * 30 * D_X * D_Y,  // MinBitsPerSecond;
        8 * 2 * 30 * DMAX_X * DMAX_Y,   // MaxBitsPerSecond;
    }, 
        
    //
    // KS_VIDEOINFOHEADER (default format)
    //
    {
        0, 0, 0, 0,                         // RECT  rcSource; 
        0, 0, 0, 0,                         // RECT  rcTarget; 
        DMAX_X * DMAX_Y * 2 * 8 * 30,       // DWORD dwBitRate;
        0L,                                 // DWORD dwBitErrorRate; 
        333667,                             // REFERENCE_TIME  AvgTimePerFrame;   
        sizeof (KS_BITMAPINFOHEADER),       // DWORD biSize;
        DMAX_X,                             // LONG  biWidth;
        DMAX_Y,                             // LONG  biHeight;
        1,                                  // WORD  biPlanes;
        16,                                 // WORD  biBitCount;
        FOURCC_YUY2,                        // DWORD biCompression;
        DMAX_X * DMAX_Y * 2,                // DWORD biSizeImage;
        0,                                  // LONG  biXPelsPerMeter;
        0,                                  // LONG  biYPelsPerMeter;
        0,                                  // DWORD biClrUsed;
        0                                   // DWORD biClrImportant;
    }
}; 

//
// CapturePinDispatch:
//
// This is the dispatch table for the capture pin.  It provides notifications
// about creation, closure, processing, data formats, etc...
//
const
KSPIN_DISPATCH
CapturePinDispatch = {
    CCapturePin::DispatchCreate,            // Pin Create
    NULL,                                   // Pin Close
    CCapturePin::DispatchProcess,           // Pin Process
    NULL,                                   // Pin Reset
    CCapturePin::DispatchSetFormat,         // Pin Set Data Format
    CCapturePin::DispatchSetState,          // Pin Set Device State
    NULL,                                   // Pin Connect
    NULL,                                   // Pin Disconnect
    NULL,                                   // Clock Dispatch
    NULL                                    // Allocator Dispatch
};

//
// CapturePinAllocatorFraming:
//
// This is the simple framing structure for the capture pin.  Note that this
// will be modified via KsEdit when the actual capture format is determined.
//
DECLARE_SIMPLE_FRAMING_EX (
    CapturePinAllocatorFraming,
    STATICGUIDOF (KSMEMORY_TYPE_KERNEL_NONPAGED),
    KSALLOCATOR_REQUIREMENTF_SYSTEM_MEMORY |
        KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY,
    2,
    0,
    2 * PAGE_SIZE,
    2 * PAGE_SIZE
    );

//
// CapturePinDataRanges:
//
// This is the list of data ranges supported on the capture pin.  We support
// two: one RGB24, and one YUY2.
//
const 
PKSDATARANGE 
CapturePinDataRanges [CAPTURE_PIN_DATA_RANGE_COUNT] = {
    (PKSDATARANGE) &FormatYUY2_Capture,
    (PKSDATARANGE) &FormatRGB24Bpp_Capture
    };
