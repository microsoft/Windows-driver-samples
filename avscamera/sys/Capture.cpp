/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2001, Microsoft Corporation.

    File:

        capture.cpp

    Abstract:

    This is the implementation of the CCapturePin class.

    CCapturePin wraps a PKSPIN object and does:
        1. Object construction and cleanup, 
        2. State management,
        3. Accepts new frame buffers for filling,
        4. Completes frame buffers delivered by the simulation,
        5. Negotiates for and sets new formats / data ranges, and
        6. Manages allocator framing.

    History:

        created 3/8/2001

**************************************************************************/

#include "Common.h"
#include <ksmedia.h>
#include "ntintsafe.h"

/**************************************************************************

    PAGEABLE CODE

**************************************************************************/


#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA

CCapturePin::
CCapturePin (
    _In_    PKSPIN Pin
)
    : m_Pin (Pin)
    , m_PinState(PinStopped)
    , m_Clock(nullptr)
    , m_PendIo(FALSE)
    , m_AcquiredResources(FALSE)
    , m_VideoInfoHeader(nullptr)
    , m_pBitmapInfoHeader(nullptr)
    , m_PreviousStreamPointer(nullptr)
    , m_PresentationTime(0)
    , m_FrameNumber(0)
    , m_DroppedFrames(0)
    , m_DesiredFrames(2)
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

    NT_ASSERT(Pin);

    PKSDEVICE Device = KsPinGetDevice (Pin);

    NT_ASSERT(Device);

    //
    // Set up our device pointer.  This gives us access to "hardware I/O"
    // during the capture routines.
    //
    m_Device = CCaptureDevice::Recast(Device);

    NT_ASSERT( m_Device );

    m_Sensor = m_Device->GetSensor( KsPinGetParentFilter(Pin ) );

    NT_ASSERT( m_Sensor );
}

CCapturePin::
~CCapturePin()
{
    PAGED_CODE();

    SAFE_FREE( m_pBitmapInfoHeader );
    SAFE_FREE( m_VideoInfoHeader );
}

/*************************************************/

NTSTATUS
CCapturePin::
ReleaseAllFrames()

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
    while (Clone)
    {

        NextClone = KsStreamPointerGetNextClone (Clone);

        Clone->StreamHeader->DataUsed = 0;
        KsStreamPointerDelete (Clone);

        Clone = NextClone;

    }

    return STATUS_SUCCESS;

}

NTSTATUS 
CCapturePin::
Initialize() 
/*++

Routine Description:

    Post construction initialization:
        1. Add us to the Pin's bag,
        2. Capture information about the image/video format, and
        3. Update the allocator's framing to match.

Arguments:

    None

Return Value:

    Success / Failure

--*/
{
    PAGED_CODE();

    DBG_ENTER("(Pin=%d)", m_Pin->Id);

    NTSTATUS Status = STATUS_SUCCESS;

    if( !m_Sensor )
    {
        // Fail if we couldn't find the sensor.
        Status = STATUS_INVALID_PARAMETER;
    }
    else 
    {
        //
        // Add the item to the object bag if we we were successful. 
        // Whenever the pin closes, the bag is cleaned up and we will be
        // freed.
        //
        Status = KsAddItemToObjectBag (
            m_Pin->Bag,
            this,
            reinterpret_cast<PFNKSFREE> (Cleanup)
            );

        if( NT_SUCCESS(Status) )
        {
            m_Pin->Context = this;
        }
    }

    //
    // If we succeeded so far, stash the video info header away and change
    // our allocator framing to reflect the fact that only now do we know
    // the framing requirements based on the connection format.
    //
    if (NT_SUCCESS (Status)) 
    {
        Status = CaptureBitmapInfoHeader();
    }
    
    if (NT_SUCCESS(Status)) 
    {
        //
        //  Edit the framing descriptors to match our pin's requirements given the current format.
        //
        Status = UpdateAllocatorFraming();
    }

    DBG_LEAVE("(Pin=%d)=0x%08X", m_Pin->Id, Status);
    return Status;
}

/*************************************************/


NTSTATUS 
CCapturePin::
CaptureBitmapInfoHeader()
/*++

Routine Description:

    Capture information about the image/video format for the pin.

Arguments:

    None

Return Value:

    Success / Failure

--*/
{
    PAGED_CODE( );

    const GUID ImageInfoSpecifier ={ STATICGUIDOF( KSDATAFORMAT_SPECIFIER_IMAGE ) };
    const GUID VideoInfoSpecifier ={ STATICGUIDOF( KSDATAFORMAT_SPECIFIER_VIDEOINFO ) };

    //  Free any previous copy of these header.
    SAFE_FREE(m_pBitmapInfoHeader);
    SAFE_FREE(m_VideoInfoHeader);

    m_pBitmapInfoHeader = reinterpret_cast <PKS_BITMAPINFOHEADER> (
        ExAllocatePoolWithTag (
            NonPagedPoolNx,
            sizeof(KS_BITMAPINFOHEADER),
            AVSHWS_POOLTAG
            )
        );
    if( !m_pBitmapInfoHeader )
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if( IsEqualGUID( m_Pin->ConnectionFormat->Specifier, ImageInfoSpecifier ) &&
            m_Pin->ConnectionFormat->FormatSize >= sizeof (KS_DATAFORMAT_IMAGEINFO) )
    {

        PKS_BITMAPINFOHEADER ConnectionHeader =
            &((reinterpret_cast <PKS_DATAFORMAT_IMAGEINFO>
               (m_Pin->ConnectionFormat))->ImageInfoHeader);

        //
        // Copy the connection format video info header into the newly 
        // allocated "captured" video info header.
        //
        *m_pBitmapInfoHeader = *ConnectionHeader;

        m_VideoInfoHeader = reinterpret_cast <PKS_VIDEOINFOHEADER> (
            ExAllocatePoolWithTag (
                NonPagedPoolNx,
                sizeof(KS_VIDEOINFOHEADER),
                AVSHWS_POOLTAG
                )
            );
        if( !m_VideoInfoHeader )
        {
            SAFE_FREE( m_pBitmapInfoHeader );
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // Copy the connection format video info header into the newly 
        // allocated "captured" video info header.
        //
        m_VideoInfoHeader->bmiHeader = *ConnectionHeader;

        //  If we don't have a known bit-depth (compressed formats), assume the worse-case.
        if( m_pBitmapInfoHeader->biBitCount==0 )
        {
            m_pBitmapInfoHeader->biBitCount=32;
        }

        //  Estimate the image size.  The derived pin can chose to override this value.
        m_VideoInfoHeader->bmiHeader.biSizeImage =
            (m_pBitmapInfoHeader->biWidth*m_pBitmapInfoHeader->biHeight*m_pBitmapInfoHeader->biBitCount)/8;

        //  Estimate a frame rate.  The derived pin can chose to override this value.
        m_VideoInfoHeader->AvgTimePerFrame = ONESECOND/30;
    }
    else
    if( IsEqualGUID( m_Pin->ConnectionFormat->Specifier, VideoInfoSpecifier ) &&
        m_Pin->ConnectionFormat->FormatSize >= sizeof (KS_DATAFORMAT_VIDEOINFOHEADER) )
    {
        PKS_VIDEOINFOHEADER ConnectionHeader =
            &((reinterpret_cast <PKS_DATAFORMAT_VIDEOINFOHEADER> 
                (m_Pin->ConnectionFormat))->
                VideoInfoHeader);

        m_VideoInfoHeader = reinterpret_cast <PKS_VIDEOINFOHEADER> (
            ExAllocatePoolWithTag (
                NonPagedPoolNx,
                KS_SIZE_VIDEOHEADER (ConnectionHeader),
                AVSHWS_POOLTAG
                )
            );
        if( !m_VideoInfoHeader )
        {
            SAFE_FREE( m_pBitmapInfoHeader );
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // Copy the connection format video info header into the newly 
        // allocated "captured" video info header.
        //
        RtlCopyMemory (
            m_VideoInfoHeader,
            ConnectionHeader,
            KS_SIZE_VIDEOHEADER (ConnectionHeader)
            );

        //
        // Copy the connection format video info header into the newly 
        // allocated "captured" video info header.
        //
        *m_pBitmapInfoHeader = m_VideoInfoHeader->bmiHeader;
    }

    DBG_TRACE( "+++ AvgTimePerFrame = %lld +++", m_VideoInfoHeader->AvgTimePerFrame );

    return STATUS_SUCCESS;
}

/*************************************************/

NTSTATUS
CCapturePin::
SetState (
    _In_    KSSTATE ToState,
    _In_    KSSTATE FromState
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

    DBG_ENTER( "( Pin=%d, ToState=%s, FromState=%s )\n",
               m_Pin->Id, KSStateToStateName(ToState), KSStateToStateName(FromState) ) ;

    switch (ToState)
    {

    case KSSTATE_STOP:

        //
        // First, stop the hardware if we actually did anything to it.
        //
        if (m_PinState != PinStopped)
        {
            Status = m_Sensor->Stop(m_Pin);
            NT_ASSERT (NT_SUCCESS (Status));

            m_PinState = PinStopped;
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
        Status = ReleaseAllFrames ();

        //
        // Release any hardware resources related to this pin.
        //
        if (m_AcquiredResources)
        {
            //
            // If we got an interface to the clock, we must release it.
            //
            if (m_Clock)
            {
                m_Clock->Release ();
                m_Clock = NULL;
            }

            m_Sensor->ReleaseHardwareResources (m_Pin);

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
        //  TODO: Move this to a derived CCapturePin class - one
        //        specifically for testing, not as a sample.
        if(GetAcquireFailureKey())
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        if (FromState == KSSTATE_STOP)
        {
            Status =
                m_Sensor->
                AcquireHardwareResources (
                    m_Pin,
                    this,
                    m_VideoInfoHeader,
                    &m_HardwareSimulation
                );

            if (NT_SUCCESS (Status))
            {
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
                        ))
                {
                    //
                    // If we could not get an interface to the clock,
                    // don't use one.
                    //
                    m_Clock = NULL;
                }
            }
            else
            {
                m_AcquiredResources = FALSE;
            }

        }

        //
        // Standard transport pins will always receive transitions in
        // +/- 1 manner.  This means we'll always see a PAUSE->ACQUIRE
        // transition before stopping the pin.
        //

        m_FrameNumber   = 0;
        m_DroppedFrames = 0;
        break;

    case KSSTATE_PAUSE:
        //
        // Stop the hardware simulation if we're coming down from run.
        //
        if (FromState == KSSTATE_RUN)
        {
            m_PresentationTime = 0;
            Status = m_Sensor->Pause (m_Pin, TRUE);

            if (NT_SUCCESS (Status))
            {
                m_PinState = PinPaused;
            }
        }
        m_FrameNumber   = 0;
        break;

    case KSSTATE_RUN:
        //
        // Start the hardware simulation or unpause it depending on
        // whether we're initially running or we've paused and restarted.
        //
        if (FromState == KSSTATE_PAUSE && m_PinState == PinPaused)
        {
            Status = m_Sensor->Pause (m_Pin, FALSE);
        }
        else
        {
            Status = m_Sensor->Start (m_Pin);
        }

        if (NT_SUCCESS (Status))
        {
            m_PinState = PinRunning;
        }
        break;

    }

    DBG_LEAVE( "( Pin=%d, ToState=%s, FromState=%s ) = 0x%08X\n",
               m_Pin->Id, KSStateToStateName(ToState), KSStateToStateName(FromState), Status ) ;

    return Status;

}

NTSTATUS
CCapturePin::
Process()

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
    PAGED_CODE( );

    DBG_ENTER( "( Pin=%d )", m_Pin->Id );

    NTSTATUS Status = STATUS_SUCCESS;
    PKSSTREAM_POINTER Leading;

    Leading = KsPinGetLeadingEdgeStreamPointer( m_Pin, KSSTREAM_POINTER_STATE_LOCKED );

    while( NT_SUCCESS( Status ) && Leading )
    {

        PKSSTREAM_POINTER ClonePointer;
        PSTREAM_POINTER_CONTEXT SPContext = NULL;

        //
        // If no data is present in the Leading edge stream pointer, just
        // move on to the next frame
        //
        if( NULL == Leading->StreamHeader->Data )
        {
            Status = KsStreamPointerAdvance( Leading );
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
        if( !m_PreviousStreamPointer )
        {
            //
            // First thing we need to do is clone the leading edge.  This allows
            // us to keep reference on the frames while they're in DMA.
            //
            Status = KsStreamPointerClone(
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
            if( NT_SUCCESS( Status ) )
            {
                //
                // Set the stream header data used to 0.  We update this
                // in the DMA completions.  For queues with DMA, we must
                // update this field ourselves.
                //
                ClonePointer->StreamHeader->DataUsed = 0;

                SPContext = reinterpret_cast <PSTREAM_POINTER_CONTEXT>
                            (ClonePointer->Context);

                SPContext->BufferVirtual =
                    reinterpret_cast <PUCHAR> (
                        ClonePointer->StreamHeader->Data
                    );
            }
        }
        else
        {
            ClonePointer = m_PreviousStreamPointer;
            SPContext = reinterpret_cast <PSTREAM_POINTER_CONTEXT>
                        (ClonePointer->Context);
        }

        //
        // If the clone failed, likely we're out of resources.  Break out
        // of the loop for now.  We may end up starving DMA.
        //
        if( !NT_SUCCESS( Status ) )
        {
            KsStreamPointerUnlock( Leading, FALSE );
            break;
        }

        //
        // Program the fake hardware.  I would use Clone->OffsetOut.*, but
        // because of the optimization of one stream pointer per frame, it
        // doesn't make complete sense.
        //
        ULONG MappingsUsed =
            m_Sensor->ProgramScatterGatherMappings(
                m_Pin,
                &ClonePointer,
                &(SPContext->BufferVirtual),
                Leading->OffsetOut.Mappings,
                Leading->OffsetOut.Remaining
            );

        //
        // In order to keep one clone per frame and simplify the fake DMA
        // logic, make a check to see if we completely used the mappings in
        // the leading edge.  Set a flag.
        //
        if( MappingsUsed == Leading->OffsetOut.Remaining )
        {
            m_PreviousStreamPointer = NULL;
        }
        else
        {
            m_PreviousStreamPointer = ClonePointer;
        }

        if( MappingsUsed )
        {
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
                KsStreamPointerAdvanceOffsets(
                    Leading,
                    0,
                    MappingsUsed,
                    FALSE
                );
        }
        else
        {
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
    if( !Leading )
    {
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
    if( NT_SUCCESS( Status ) && Leading )
    {
        KsStreamPointerUnlock( Leading, FALSE );
    }
    else
    {
        //
        // DEVICE_NOT_READY indicates that the advancement ran off the end
        // of the queue.  We couldn't lock the leading edge.
        //
        if( Status == STATUS_DEVICE_NOT_READY )
        {
            Status = STATUS_SUCCESS;
        }
    }

    //
    // If we failed with something that requires pending, set the pending I/O
    // flag so we know we need to start it again.
    //
    if( !NT_SUCCESS( Status ) || Status == STATUS_PENDING )
    {
        m_PendIo = TRUE;
    }

    DBG_LEAVE( "( Pin=%d )=0x%08X", m_Pin->Id, Status );

    return Status;
}

//
//  Emit metadata here for video or preview pin.
//
//  This function gives us one last chance to tack metadata onto the sample.
//
void
CCapturePin::
EmitMetadata(
    _Inout_ PKSSTREAM_HEADER   pStreamHeader
)
{
    PAGED_CODE();

    NT_ASSERT(pStreamHeader);
}


NTSTATUS
CCapturePin::
CompleteMapping(
    _In_ PKSSTREAM_POINTER Clone
    )

/*++

Routine Description:

    Called to notify the pin that a given number of scatter / gather
    mappings have completed.  Let the buffers go if possible.

Arguments:

    Clone -
        The stream pointer for the frame to complete.
        If Clone is null, use the head of the queue.

Return Value:

    Success / Failure

--*/

{
    PAGED_CODE();

    DBG_ENTER("(Clone=%p)", Clone);

    NTSTATUS    Status = STATUS_SUCCESS;

    if( !Clone )
    {
        Clone = KsPinGetFirstCloneStreamPointer(m_Pin);
    }

    // 
    // If we have completed all remaining mappings in this clone, it
    // is an indication that the clone is ready to be deleted and the
    // buffer released.  Set anything required in the stream header which
    // has not yet been set.  If we have a clock, we can timestamp the
    // sample.
    //
    if( Clone )
    {
        if( Clone->StreamHeader->DataUsed >= Clone->OffsetOut.Remaining )
        {
            Clone->StreamHeader->Duration =
                m_VideoInfoHeader->AvgTimePerFrame;

            Clone->StreamHeader->OptionsFlags |=
                KSSTREAM_HEADER_OPTIONSF_DURATIONVALID;

            //
            // Increment the frame number.  This is the total count of frames which
            // have attempted capture.
            //
            m_FrameNumber++;
            DBG_TRACE( "m_FrameNumber=%lld", m_FrameNumber );

            //
            // Double check the Stream Header size.  AVStream makes no guarantee
            // that because StreamHeaderSize is set to a specific size that you
            // will get that size.  If the proper data type handlers are not
            // installed, the stream header will be of default size.
            //
            if ( Clone->StreamHeader->Size >= sizeof (KSSTREAM_HEADER) +
                    sizeof (KS_FRAME_INFO))
            {
                PKS_FRAME_INFO FrameInfo = reinterpret_cast <PKS_FRAME_INFO> (
                                                Clone->StreamHeader + 1
                                            );

                FrameInfo->ExtendedHeaderSize = sizeof (KS_FRAME_INFO);
                FrameInfo->dwFrameFlags       = KS_VIDEO_FLAG_FRAME;
                FrameInfo->PictureNumber      = (LONGLONG)m_FrameNumber;

                // I don't really have a way to tell if the device has dropped a frame
                // or was not able to send a frame on time.
                FrameInfo->DropCount = (LONGLONG)m_DroppedFrames;
            }

            KsStreamPointerDelete (Clone);
        }
        else
        {
            //
            // If only part of the mappings in this clone have been completed,
            // update the pointers.  Since we're guaranteed this won't advance
            // to a new frame by the check above, it won't fail.
            //
            Status =
                KsStreamPointerAdvanceOffsets(
                    Clone,
                    0,
                    Clone->StreamHeader->DataUsed,
                    FALSE
                );
        }
    }
    else
    {
        //  We had nothing to process.
        Status = STATUS_UNSUCCESSFUL;
    }

    //
    // If we've used all the mappings in hardware and pended, we can kick
    // processing to happen again if we've completed mappings.
    //
    if (m_PendIo)
    {
        KsPinAttemptProcessing (m_Pin, TRUE);
        m_PendIo = FALSE;
    }

    DBG_LEAVE("(Clone=%p)=0x%08X", Clone, Status);
    return Status;
}


bool
CCapturePin::
GetAcquireFailureKey()
{
    NTSTATUS            ntstatus;
    bool                acquireFailure = false;
    UNICODE_STRING      RegistryKeyName;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    HANDLE              handleRegKey = NULL;
    PKEY_VALUE_FULL_INFORMATION pKeyInfo = NULL;
    ULONG               ulKeyInfoSizeNeeded = 0;
    UNICODE_STRING      ValueName = {0};


    PAGED_CODE();

    // Get the Registry key
    RtlInitUnicodeString(&RegistryKeyName, L"\\Registry\\Machine\\Software\\Microsoft\\wtt\\MachineConfig");
    InitializeObjectAttributes(&ObjectAttributes,
                               &RegistryKeyName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,    // handle
                               NULL);

    ntstatus = ZwOpenKey(&handleRegKey, KEY_READ, &ObjectAttributes);
    // If we can't open the key, we're done. Worst case, we cannot stream.
    if(NT_SUCCESS(ntstatus))
    {
        // Get the location from the registry key
        RtlInitUnicodeString(&ValueName, L"AcquireFailure");

        // Figure out how big keyInfo needs to be
        ntstatus = ZwQueryValueKey(handleRegKey,
                                   &ValueName,
                                   KeyValueFullInformation,
                                   pKeyInfo,
                                   0,
                                   &ulKeyInfoSizeNeeded );

        // We expect one of these errors
        if( (ntstatus == STATUS_BUFFER_TOO_SMALL) || (ntstatus == STATUS_BUFFER_OVERFLOW) )
        {
            // Allocate the memory needed for the key
            pKeyInfo = (PKEY_VALUE_FULL_INFORMATION) new (NonPagedPoolNx) BYTE[ulKeyInfoSizeNeeded];
            RtlZeroMemory(pKeyInfo, ulKeyInfoSizeNeeded);

            // Now get the actual key data
            ntstatus = ZwQueryValueKey( handleRegKey,
                                        &ValueName,
                                        KeyValueFullInformation,
                                        pKeyInfo,
                                        ulKeyInfoSizeNeeded,
                                        &ulKeyInfoSizeNeeded );

            if(ntstatus == STATUS_SUCCESS)
            {
                acquireFailure = true;
            }

            delete[] pKeyInfo;
        }
    }

    // All done with the registry
    if (NULL != handleRegKey)
    {
        ZwClose(handleRegKey);
    }

    return acquireFailure;
}


NTSTATUS
CCapturePin::
DispatchClose(
    _In_    PKSPIN Pin,
    _In_    PIRP Irp
)
/*++

Routine Description:

    Static thunking function to forward Close

Arguments:

    Pin -
        Our pin
    Irp -
        The IRP the request came in on.

Return Value:

    Success / Failure

--*/
{
    PAGED_CODE( );

    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    DBG_ENTER("()");

    NT_ASSERT( Pin );
    NT_ASSERT( Pin->Context );
    if( Pin && Pin->Context )
    {
        CCapturePin *pPin = reinterpret_cast <CCapturePin *> (Pin->Context);
        Status = pPin->Close( Irp );
    }

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

NTSTATUS
CCapturePin::
DispatchSetState(
    _In_    PKSPIN Pin,
    _In_    KSSTATE ToState,
    _In_    KSSTATE FromState
)
/*++

Routine Description:

    Static thunking function to forward SetState

Arguments:

    Pin -
        Our pin
    Irp -
        The IRP the request came in on.

Return Value:

    Success / Failure

--*/
{
    PAGED_CODE( );

    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    DBG_ENTER("()");

    NT_ASSERT( Pin );
    NT_ASSERT( Pin->Context );
    if( Pin && Pin->Context )
    {
        CCapturePin *pPin = reinterpret_cast <CCapturePin *> (Pin->Context);
        Status = pPin->SetState( ToState, FromState );
    }

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

NTSTATUS
CCapturePin::
DispatchSetFormat(
    _In_        PKSPIN Pin,
    _In_opt_    PKSDATAFORMAT OldFormat,
    _In_opt_    PKSMULTIPLE_ITEM OldAttributeList,
    _In_        const KSDATARANGE *DataRange,
    _In_opt_    const KSATTRIBUTE_LIST *AttributeRange
)

/*++

Routine Description:

    This is the set data format dispatch for the capture pin.  It is called
    in two circumstances.

    1: Before Pin's creation dispatch has been made to verify that
        Pin->ConnectionFormat is an acceptable format for the range
        DataRange.  In this case OldFormat is NULL.

    2: After Pin's creation dispatch has been made and an initial format
        selected in order to change the format for the pin.  In this case,
        OldFormat will not be NULL.

    Validate that the format is acceptible and perform the actions necessary
    to change format if appropriate.

Arguments:

    Pin -
        The pin this format is being set on.  The format itself will be in
        Pin->ConnectionFormat.

    OldFormat -
        The previous format used on this pin.  If this is NULL, it is an
        indication that Pin's creation dispatch has not yet been made and
        that this is a request to validate the initial format and not to
        change formats.

    OldAttributeList -
        The old attribute list for the prior format

    DataRange -
        A range out of our list of data ranges which was determined to be
        at least a partial match for Pin->ConnectionFormat.  If the format
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

    PAGED_CODE( );

    DBG_ENTER( "(Pin->Id=%d)", Pin->Id );

    NTSTATUS Status = STATUS_NO_MATCH;
    const GUID ImageInfoSpecifier = { STATICGUIDOF( KSDATAFORMAT_SPECIFIER_IMAGE ) };
    const GUID VideoInfoSpecifier = { STATICGUIDOF( KSDATAFORMAT_SPECIFIER_VIDEOINFO ) };

    //
    // Find the pin, if it exists yet.  OldFormat will be an indication of
    // this.  If we're changing formats, OldFormat will be non-NULL.
    //
    // You cannot use Pin->Context to make the determination.  AVStream
    // preinitializes this to the filter's context.
    //
    CCapturePin *CapPin = reinterpret_cast <CCapturePin *> (Pin->Context);

    if( IsEqualGUID( Pin->ConnectionFormat->Specifier, ImageInfoSpecifier ) &&
            Pin->ConnectionFormat->FormatSize >= sizeof (KS_DATAFORMAT_IMAGEINFO) )
    {
        PKS_DATAFORMAT_IMAGEINFO ConnectionFormat = reinterpret_cast <PKS_DATAFORMAT_IMAGEINFO> (Pin->ConnectionFormat);

        //
        // DataRange comes out of OUR data range list.  I know the range
        // is valid as such.
        //
        const KS_DATARANGE_IMAGE *VIRange = reinterpret_cast <const KS_DATARANGE_IMAGE *> (DataRange);

        //
        // Check that bmiHeader.biSize is valid since we use it later.
        //
        ULONG ImageHeaderSize = ConnectionFormat->ImageInfoHeader.biSize;
        ULONG DataFormatSize = sizeof (KSDATAFORMAT) +sizeof(KS_BITMAPINFOHEADER);

        if( ImageHeaderSize < ConnectionFormat->ImageInfoHeader.biSize ||
                DataFormatSize < ImageHeaderSize ||
                DataFormatSize > ConnectionFormat->DataFormat.FormatSize )
        {
            Status = STATUS_INVALID_PARAMETER;
        }

        //
        // Check that the format is a match for the selected range.
        //
        else if( (ConnectionFormat->ImageInfoHeader.biWidth != VIRange->ImageInfoHeader.biWidth) ||
                 (ConnectionFormat->ImageInfoHeader.biHeight != VIRange->ImageInfoHeader.biHeight) ||
                 (ConnectionFormat->ImageInfoHeader.biCompression != VIRange->ImageInfoHeader.biCompression) )
        {
            Status = STATUS_NO_MATCH;
        }
        else
        {

            ULONG ImageSize;

            if( !MultiplyCheckOverflow( (ULONG) ConnectionFormat->ImageInfoHeader.biWidth, (ULONG) abs( ConnectionFormat->ImageInfoHeader.biHeight ), &ImageSize ) )
            {
                Status = STATUS_INVALID_PARAMETER;
            }

            else if( !MultiplyCheckOverflow( ImageSize, (ULONG) (ConnectionFormat->ImageInfoHeader.biBitCount/8), &ImageSize ) )
            {
                Status = STATUS_INVALID_PARAMETER;
            }

            //
            // Valid for the formats we use.  Otherwise, this would be
            // checked later.
            //
            else if( ConnectionFormat->ImageInfoHeader.biSizeImage <  ImageSize )
            {
                Status = STATUS_INVALID_PARAMETER;
            }
            else
            {
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
                if( OldFormat && CapPin )
                {
                    //
                    // If we're in the stop state, we can handle just about any
                    // change.  We don't support dynamic format changes.
                    //
                    if( Pin->DeviceState == KSSTATE_STOP )
                    {
                        Status = CapPin->CaptureBitmapInfoHeader();
                    }
                    else
                    {
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

    else if( IsEqualGUID( Pin->ConnectionFormat->Specifier,
                          VideoInfoSpecifier ) &&
             Pin->ConnectionFormat->FormatSize >=
             sizeof (KS_DATAFORMAT_VIDEOINFOHEADER) )
    {

        PKS_DATAFORMAT_VIDEOINFOHEADER ConnectionFormat =
            reinterpret_cast <PKS_DATAFORMAT_VIDEOINFOHEADER>
            (Pin->ConnectionFormat);

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
        ULONG VideoHeaderSize = KS_SIZE_VIDEOHEADER(
                                    &ConnectionFormat->VideoInfoHeader
                                );

        ULONG DataFormatSize = FIELD_OFFSET(
                                   KS_DATAFORMAT_VIDEOINFOHEADER, VideoInfoHeader
                               ) + VideoHeaderSize;

        if(
            VideoHeaderSize < ConnectionFormat->
            VideoInfoHeader.bmiHeader.biSize ||
            DataFormatSize < VideoHeaderSize ||
            DataFormatSize > ConnectionFormat->DataFormat.FormatSize
        )
        {
            Status = STATUS_INVALID_PARAMETER;
        }

        //
        // Check that the format is a match for the selected range.
        //
        else if(
            (ConnectionFormat->VideoInfoHeader.bmiHeader.biWidth !=
             VIRange->VideoInfoHeader.bmiHeader.biWidth) ||

            (ConnectionFormat->VideoInfoHeader.bmiHeader.biHeight !=
             VIRange->VideoInfoHeader.bmiHeader.biHeight) ||

            (ConnectionFormat->VideoInfoHeader.bmiHeader.biCompression !=
             VIRange->VideoInfoHeader.bmiHeader.biCompression)
        )
        {
            Status = STATUS_NO_MATCH;
        }
        else
        {

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

            if( !MultiplyCheckOverflow(
                        (ULONG) ConnectionFormat->VideoInfoHeader.bmiHeader.biWidth,
                        (ULONG) abs( ConnectionFormat->
                                     VideoInfoHeader.bmiHeader.biHeight ),
                        &ImageSize
                    ) )
            {
                Status = STATUS_INVALID_PARAMETER;
            }

            //
            // We only support KS_BI_RGB (24) and KS_BI_YUV422 (16), so
            // this is valid for those formats.
            //
            else if( !MultiplyCheckOverflow(
                         ImageSize,
                         (ULONG) (ConnectionFormat->
                                  VideoInfoHeader.bmiHeader.biBitCount / 8),
                         &ImageSize
                     ) )
            {
                Status = STATUS_INVALID_PARAMETER;
            }

            //
            // Valid for the formats we use.  Otherwise, this would be
            // checked later.
            //
            else if( ConnectionFormat->VideoInfoHeader.bmiHeader.biSizeImage <
                     ImageSize )
            {
                Status = STATUS_INVALID_PARAMETER;
            }
            else
            {
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
                if( OldFormat && CapPin )
                {
                    //
                    // If we're in the stop state, we can handle just about any
                    // change.  We don't support dynamic format changes.
                    //
                    if( Pin->DeviceState == KSSTATE_STOP )
                    {
                        if( !CapPin->CaptureBitmapInfoHeader( ) )
                        {
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                        }
                    }
                    else
                    {
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

    DBG_LEAVE( "(Pin->Id=%d)=0x%08X", Pin->Id, Status );
    return Status;
}

NTSTATUS
CCapturePin::
IntersectHandler(
    _In_        PKSFILTER Filter,
    _In_        PIRP Irp,
    _In_        PKSP_PIN PinInstance,
    _In_        PKSDATARANGE CallerDataRange,
    _In_        PKSDATARANGE DescriptorDataRange,
    _In_        ULONG BufferSize,
    _Out_opt_   PVOID Data OPTIONAL,
    _Out_       PULONG DataSize
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
    PAGED_CODE( );

    const GUID ImageInfoSpecifier = { STATICGUIDOF( KSDATAFORMAT_SPECIFIER_IMAGE ) };
    const GUID VideoInfoSpecifier = { STATICGUIDOF( KSDATAFORMAT_SPECIFIER_VIDEOINFO ) };
    ULONG DataFormatSize;

    NT_ASSERT( Filter );
    NT_ASSERT( Irp );
    NT_ASSERT( PinInstance );
    NT_ASSERT( CallerDataRange );
    NT_ASSERT( DescriptorDataRange );
    NT_ASSERT( DataSize );

    //
    // Specifier FORMAT_VideoInfo for VIDEOINFOHEADER
    //
    if( IsEqualGUID( CallerDataRange->Specifier, ImageInfoSpecifier ) &&
            CallerDataRange->FormatSize >= sizeof (KS_DATARANGE_IMAGE) )
    {

        PKS_DATARANGE_IMAGE callerDataRange =
            reinterpret_cast <PKS_DATARANGE_IMAGE> (CallerDataRange);

        PKS_DATARANGE_IMAGE descriptorDataRange =
            reinterpret_cast <PKS_DATARANGE_IMAGE> (DescriptorDataRange);

        //
        // Check that the other fields match
        //
        if( (RtlCompareMemory( &callerDataRange->ConfigCaps, &descriptorDataRange->ConfigCaps, sizeof (KS_VIDEO_STREAM_CONFIG_CAPS) )
                != sizeof (KS_VIDEO_STREAM_CONFIG_CAPS)) )
        {
            return STATUS_NO_MATCH;
        }

        {
            ULONG ImageHeaderSize = ((PKS_DATARANGE_IMAGE) callerDataRange)->ImageInfoHeader.biSize;
            ULONG DataRangeSize = FIELD_OFFSET( KS_DATARANGE_IMAGE, ImageInfoHeader ) + ImageHeaderSize;

            if( ImageHeaderSize < callerDataRange->ImageInfoHeader.biSize ||
                    DataRangeSize < ImageHeaderSize ||
                    DataRangeSize > callerDataRange->DataRange.FormatSize )
            {
                return STATUS_INVALID_PARAMETER;
            }
        }

        DataFormatSize = sizeof (KSDATAFORMAT) +sizeof(KS_BITMAPINFOHEADER);
        //
        // If the passed buffer size is 0, it indicates that this is a size
        // only query.  Return the size of the intersecting data format and
        // pass back STATUS_BUFFER_OVERFLOW.
        //
        if( BufferSize == 0 )
        {
            *DataSize = DataFormatSize;
            return STATUS_BUFFER_OVERFLOW;
        }

        //
        // Verify that the provided structure is large enough to
        // accept the result.
        //
        if( BufferSize < DataFormatSize )
        {
            return STATUS_BUFFER_TOO_SMALL;
        }

        //
        // Copy over the KSDATAFORMAT, followed by the actual VideoInfoHeader
        //
        *DataSize = DataFormatSize;

        PKS_DATAFORMAT_IMAGEINFO pFormatImageInfoHeader = PKS_DATAFORMAT_IMAGEINFO( Data );

        //
        // Copy over the KSDATAFORMAT.  This is precisely the same as the
        // KSDATARANGE (it's just the GUIDs, etc...  not the format information
        // following any data format.
        //
        RtlCopyMemory( pFormatImageInfoHeader, DescriptorDataRange, sizeof (KSDATAFORMAT) );
        pFormatImageInfoHeader->DataFormat.FormatSize = DataFormatSize;

        //
        // Copy over the callers requested KS_BITMAPINFOHEADER
        //
        RtlCopyMemory( &pFormatImageInfoHeader->ImageInfoHeader, &callerDataRange->ImageInfoHeader, sizeof(KS_BITMAPINFOHEADER) );
        return STATUS_SUCCESS;
    }
    else if( IsEqualGUID( CallerDataRange->Specifier, VideoInfoSpecifier ) &&
             CallerDataRange->FormatSize >= sizeof (KS_DATARANGE_VIDEO) )
    {

        PKS_DATARANGE_VIDEO callerDataRange =
            reinterpret_cast <PKS_DATARANGE_VIDEO> (CallerDataRange);

        PKS_DATARANGE_VIDEO descriptorDataRange =
            reinterpret_cast <PKS_DATARANGE_VIDEO> (DescriptorDataRange);

        PKS_DATAFORMAT_VIDEOINFOHEADER FormatVideoInfoHeader;

        //
        // Check that the other fields match
        //
        if( (callerDataRange->bFixedSizeSamples !=
                descriptorDataRange->bFixedSizeSamples) ||
                (callerDataRange->bTemporalCompression !=
                 descriptorDataRange->bTemporalCompression) ||
                (callerDataRange->StreamDescriptionFlags !=
                 descriptorDataRange->StreamDescriptionFlags) ||
                (callerDataRange->MemoryAllocationFlags !=
                 descriptorDataRange->MemoryAllocationFlags) ||
                (RtlCompareMemory( &callerDataRange->ConfigCaps,
                                   &descriptorDataRange->ConfigCaps,
                                   sizeof (KS_VIDEO_STREAM_CONFIG_CAPS) ) !=
                 sizeof (KS_VIDEO_STREAM_CONFIG_CAPS)) )
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
        ULONG VideoHeaderSize = KS_SIZE_VIDEOHEADER(
                                    &callerDataRange->VideoInfoHeader
                                );

        ULONG DataRangeSize =
            FIELD_OFFSET( KS_DATARANGE_VIDEO, VideoInfoHeader ) +
            VideoHeaderSize;

        //
        // Check that biSize does not extend past the buffer.  The
        // first two checks are for arithmetic overflow on the
        // operations to compute the alleged size.  (On unsigned
        // math, a+b < a iff an arithmetic overflow occurred).
        //
        if(
            VideoHeaderSize < callerDataRange->
            VideoInfoHeader.bmiHeader.biSize ||
            DataRangeSize < VideoHeaderSize ||
            DataRangeSize > callerDataRange->DataRange.FormatSize
        )
        {
            return STATUS_INVALID_PARAMETER;
        }

        DataFormatSize =
            sizeof (KSDATAFORMAT) +
            KS_SIZE_VIDEOHEADER( &callerDataRange->VideoInfoHeader );


        //
        // If the passed buffer size is 0, it indicates that this is a size
        // only query.  Return the size of the intersecting data format and
        // pass back STATUS_BUFFER_OVERFLOW.
        //
        if( BufferSize == 0 )
        {
            *DataSize = DataFormatSize;
            return STATUS_BUFFER_OVERFLOW;
        }

        //
        // Verify that the provided structure is large enough to
        // accept the result.
        //
        if( BufferSize < DataFormatSize )
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
        RtlCopyMemory(
            &FormatVideoInfoHeader->DataFormat,
            DescriptorDataRange,
            sizeof (KSDATAFORMAT) );

        FormatVideoInfoHeader->DataFormat.FormatSize = DataFormatSize;

        //
        // Copy over the callers requested VIDEOINFOHEADER
        //

        RtlCopyMemory(
            &FormatVideoInfoHeader->VideoInfoHeader,
            &callerDataRange->VideoInfoHeader,
            KS_SIZE_VIDEOHEADER( &callerDataRange->VideoInfoHeader )
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
                KS_DIBSIZE( FormatVideoInfoHeader->VideoInfoHeader.bmiHeader );

        //
        // REVIEW - Perform other validation such as cropping and scaling checks
        //

        return STATUS_SUCCESS;

    } // End of VIDEOINFOHEADER specifier

    return STATUS_NO_MATCH;
}

NTSTATUS
CCapturePin::
DispatchProcess(
    _In_    PKSPIN Pin
)
/*++

Routine Description:

    Static thunking function to forward Process

Arguments:

    Pin -
        Our pin

Return Value:

    Success / Failure

--*/
{
    PAGED_CODE( );

    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    DBG_ENTER("()");

    NT_ASSERT( Pin );
    NT_ASSERT( Pin->Context );
    if( Pin && Pin->Context )
    {
        CCapturePin *pPin = reinterpret_cast <CCapturePin *> (Pin->Context);
        Status = pPin->Process();
    }

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

void
CCapturePin::
DispatchReset(
    _In_    PKSPIN Pin
)
/*++

Routine Description:

    Static thunking function to forward Reset

Arguments:

    Pin -
        Our pin

Return Value:

    Success / Failure

--*/
{
    PAGED_CODE( );

    DBG_ENTER("()");

    NT_ASSERT( Pin );
    NT_ASSERT( Pin->Context );
    if( Pin && Pin->Context )
    {
        CCapturePin *pPin = reinterpret_cast <CCapturePin *> (Pin->Context);
        pPin->Reset( );
    }
    DBG_LEAVE("()");
}

NTSTATUS
CCapturePin::
DispatchConnect(
    _In_    PKSPIN Pin
)
/*++

Routine Description:

    Static thunking function to forward Reset

Arguments:

    Pin -
        Our pin

Return Value:

    Success / Failure

--*/
{
    PAGED_CODE( );

    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    DBG_ENTER("()");

    NT_ASSERT( Pin );
    NT_ASSERT( Pin->Context );
    if( Pin && Pin->Context )
    {
        CCapturePin *pPin = reinterpret_cast <CCapturePin *> (Pin->Context);
        Status = pPin->Connect( );
    }

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

void
CCapturePin::
DispatchDisconnect(
    _In_    PKSPIN Pin
)
/*++

Routine Description:

    Static thunking function to forward Disconnect

Arguments:

    Pin -
        Our pin

Return Value:

    Success / Failure

--*/
{
    PAGED_CODE( );

    DBG_ENTER("()");

    NT_ASSERT( Pin );
    NT_ASSERT( Pin->Context );
    if( Pin && Pin->Context )
    {
        CCapturePin *pPin = reinterpret_cast <CCapturePin *> (Pin->Context);
        pPin->Disconnect( );
    }
    DBG_LEAVE("()");
}


NTSTATUS
CCapturePin::
Close(
    _In_ PIRP Irp
)
/*++

Routine Description:

    Close handler.

    Replace or overload if you need special handling.

Arguments:

    Irp -
        The IRP the request came in on.

Return Value:

    Success / Failure

--*/
{
    PAGED_CODE( );

    //  Some reasonable default behavior.
    return STATUS_SUCCESS;
}

NTSTATUS
CCapturePin::
SetFormat(
    _In_opt_    PKSDATAFORMAT OldFormat,
    _In_opt_    PKSMULTIPLE_ITEM OldAttributeList,
    _In_        const KSDATARANGE *DataRange,
    _In_opt_    const KSATTRIBUTE_LIST *AttributeRange
)
/*++

Routine Description:

    SetFormat handler.

    Replace or overload if you need special handling.

Arguments:

    OldFormat -
        The previous format used on this pin.  If this is NULL, it is an
        indication that Pin's creation dispatch has not yet been made and
        that this is a request to validate the initial format and not to
        change formats.

    OldAttributeList -
        The old attribute list for the prior format

    DataRange -
        A range out of our list of data ranges which was determined to be
        at least a partial match for Pin->ConnectionFormat.  If the format
        there is unacceptable for the range, STATUS_NO_MATCH should be
        returned.

    AttributeRange -
        The attribute range

Return Value:

    Success / Failure

--*/
{
    PAGED_CODE( );

    //  Some reasonable default behavior here.
    return STATUS_SUCCESS;
}

void
CCapturePin::
Reset()
/*++

Routine Description:

    Reset handler.

    Replace or overload if you need special handling.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PAGED_CODE( );

    //  Some reasonable default behavior here.
    m_Sensor->Reset(m_Pin);
}

NTSTATUS
CCapturePin::
Connect()
/*++

Routine Description:

    Connect handler.

    Replace or overload if you need special handling.

Arguments:

    None.

Return Value:

    Success / Failure

--*/
{
    PAGED_CODE( );

    //  Some reasonable default behavior here.
    return STATUS_SUCCESS;
}

void
CCapturePin::
Disconnect()
/*++

Routine Description:

    Disconnect handler.

    Replace or overload if you need special handling.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PAGED_CODE( );

    //  Some reasonable default behavior here.
}

//  Update the Pin's Allocator Framing
NTSTATUS
CCapturePin::
UpdateAllocatorFraming()
{
    PAGED_CODE();

    //  Acquire the lock and update the Pin's allocator framing.
    NTSTATUS Status = 
        KsEdit( m_Pin, &m_Pin->Descriptor, AVSHWS_POOLTAG);

    if( NT_SUCCESS( Status ) )
    {
        //
        // If the edits proceeded without running out of memory, adjust 
        // the framing based on the video info header.
        //
        Status = KsEdit( m_Pin, &m_Pin->Descriptor->AllocatorFraming, AVSHWS_POOLTAG );

        if( NT_SUCCESS( Status ) )
        {
            //
            // We've KsEdit'ed this...  I'm safe to cast away constness as
            // long as the edit succeeded.
            //
            PKSALLOCATOR_FRAMING_EX Framing =
                const_cast <PKSALLOCATOR_FRAMING_EX> (
                m_Pin->Descriptor->AllocatorFraming
                );

            Framing->FramingItem[0].Frames = m_DesiredFrames;
            Framing->FramingItem[0].PhysicalRange.MinFrameSize = m_VideoInfoHeader->bmiHeader.biSize;
            Framing->FramingItem[0].PhysicalRange.MaxFrameSize = m_VideoInfoHeader->bmiHeader.biSize;
            Framing->FramingItem [0].FramingRange.Range.MinFrameSize = m_VideoInfoHeader->bmiHeader.biSize;
            Framing->FramingItem [0].FramingRange.Range.MaxFrameSize = m_VideoInfoHeader->bmiHeader.biSize;

            Framing->FramingItem [0].PhysicalRange.Stepping = 0;
            Framing->FramingItem [0].FramingRange.Range.Stepping = 0;

            DBG_TRACE( "Advertising Frame requirement: %d", m_DesiredFrames );
            DBG_TRACE("Image size estimate of: %d bytes", m_VideoInfoHeader->bmiHeader.biSize );
        }
    }

    DBG_LEAVE("()=0x%08X",Status);
    return Status;
}

