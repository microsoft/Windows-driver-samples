/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2001, Microsoft Corporation.

    File:

        hwsim.cpp

    Abstract:

        This file contains the implementation of CHardwareSimulation used to 
        simulate a camera's capture stream.

        Our simulation cannot mimic a real camera with any precision.  A real
        camera would likely have dedicated hardware to provide interrupts, do 
        DMA transfers of frames from an internal buffer as well as do scaling 
        and on-the-fly colorspace and planar format conversions so as to supply
        frames for preview, video and photo in varying resolutions and
        colorspaces.  
        
        This simulation does not have such hardware, so instead it synthesizes
        a frame on the fly for each local pin as required.  The frame is
        synthesized from common state information, so the images produced 
        should be comparable from pin to pin.  CHardwareSimulation is used as
        a base class for each simulated pin / hardware.

    History:

        created 3/9/2001

**************************************************************************/

#include "Common.h"

/**************************************************************************

    PAGEABLE CODE

**************************************************************************/

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA


inline
CHwSimTimer::
CHwSimTimer(
    _In_    CHardwareSimulation *Parent
)
    : KPassiveTimer( Parent->m_Sensor->GetDeviceObject() )
    , m_Parent( Parent )
/*++

Routine Description:

    CHwSimTimer object construction.

Arguments:

    Parent -
        The parent CHardwareSimulation.

Return Value:

    None

--*/
{
    PAGED_CODE();
}

void
CHwSimTimer::Handler(
    _In_opt_    PVOID   Context
)
/*++

Routine Description:

    Punts to the HardwareSim...

Arguments:

    Context -
        Our parent CHardwareSimulation.

Return Value:

    None

--*/
{
    PAGED_CODE();
    ((CHardwareSimulation *) Context)->FakeHardware();
}

/*************************************************/

CHardwareSimulation::
CHardwareSimulation (
    _Inout_ CSensor *Sensor,
    _In_    LONG    PinID
)
    : m_Sensor (Sensor)
    , m_IsrTimer(this)
    , m_ScatterGatherMappingsMax (SCATTER_GATHER_MAPPINGS_MAX)
    , m_TimePerFrame(ONESECOND/30)
    , m_PinID(PinID)
    , m_Synthesizer(nullptr)
    , m_Width(0)
    , m_Height(0)
    , m_ImageSize(0)
    , m_PinState(PinStopped)
    , m_NumMappingsCompleted(0)
    , m_ScatterGatherMappingsQueued(0)
    , m_ScatterGatherBytesQueued(0)
    , m_NumFramesSkipped(0)
    , m_InterruptTime(0)
    , m_LastReportedExposureTime(DEF_EXPOSURE_TIME) // Assume the default exposure time for now.
    , m_LastReportedWhiteBalance(0)
    , m_FaceDetectionDelay(1)                       // Start out reporting immediately.

/*++

Routine Description:

    CHardwareSimulation Construction.

Arguments:

    Sensor -
        The parent CSensor object that contains state for the controls.
    PinID -
        The pin index.

Return Value:

    None

--*/
{

    PAGED_CODE();

    //
    // Initialize the timer's, and locks necessary to simulate this hardware.
    //
    InitializeListHead (&m_ScatterGatherMappings);

    //
    // Initialize the entry lookaside.
    //
    ExInitializeNPagedLookasideList (
        &m_ScatterGatherLookaside,
        NULL,
        NULL,
        POOL_NX_ALLOCATION,
        sizeof (SCATTER_GATHER_ENTRY),
        'nEGS',
        0
    );

    m_StartTime.QuadPart = 0;
}

CHardwareSimulation::
~CHardwareSimulation()
{
    PAGED_CODE();

    //  Cancel the timer, just in case its running.
    m_IsrTimer.Cancel();

    //
    // Delete the scatter / gather lookaside list.
    //
    ExDeleteNPagedLookasideList (&m_ScatterGatherLookaside);
}

/*************************************************/

BOOLEAN
CHardwareSimulation::
Initialize()

/*++

Routine Description:

    Initialize the hardware simulation

Arguments:

    [none]

Return Value:

    TRUE - CHardwareSimulation is fully initialized.

--*/

{
    PAGED_CODE();

    return m_IsrTimer.IsValid() ;
}

/*************************************************/

NTSTATUS
CHardwareSimulation::
Start (
    IN CSynthesizer *ImageSynth,
    IN LONGLONG TimePerFrame,
    IN ULONG Width,
    IN ULONG Height,
    IN ULONG ImageSize
)

/*++

Routine Description:

    Start the hardware simulation.  This will kick the interrupts on,
    begin issuing DPC's, filling in capture information, etc...
    We keep track of starvation starting at this point.

Arguments:

    ImageSynth -
        The image synthesizer to use to generate pictures to display
        on the capture buffer.

    TimePerFrame -
        The time per frame...  we issue interrupts this often.

    Width -
        The image width

    Height -
        The image height

    ImageSize -
        The size of the image.  We allocate a temporary scratch buffer
        based on this size to fake hardware.

Return Value:

    Success / Failure (typical failure will be out of memory on the
    scratch buffer, etc...)

--*/

{

    PAGED_CODE();

    //  Prevent state-changes during this call.
    KScopedMutex    Lock(m_ListLock);

    NTSTATUS Status = STATUS_SUCCESS;

    m_Synthesizer = ImageSynth;
    m_TimePerFrame = TimePerFrame;
    m_ImageSize = ImageSize;
    m_Height = Height;
    m_Width = Width;

    m_NumMappingsCompleted = 0;
    m_ScatterGatherMappingsQueued = 0;
    m_NumFramesSkipped = 0;
    m_InterruptTime = 0;

    KeQuerySystemTime (&m_StartTime);

    if( !m_Synthesizer->Initialize() )
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // If everything is ok, start issuing interrupts.
    //
    if (NT_SUCCESS (Status))
    {
        LARGE_INTEGER NextTime;
        NextTime.QuadPart = m_StartTime.QuadPart + m_TimePerFrame;

        m_PinState = PinRunning;
        m_IsrTimer.Set( NextTime );
    }

    return Status;
}

/*************************************************/

NTSTATUS
CHardwareSimulation::
Pause (
    BOOLEAN Pausing
)

/*++

Routine Description:

    Pause the hardware simulation...  When the hardware simulation is told
    to pause, it stops stops the timer, but it does not reset the counters.

Arguments:

    Pausing -
        Indicates whether the hardware is pausing or not.

        TRUE -
            Pause the hardware

        FALSE -
            Unpause the hardware from a previous pause


Return Value:

    Success / Failure

--*/

{

    PAGED_CODE();

    {
        //  Prevent state-changes during this call.
        KScopedMutex    Lock(m_ListLock);

        if (Pausing && m_PinState == PinRunning)
        {
            //
            // If we were running, stop completing mappings, etc...
            //
            m_PinState = PinPaused;
        }
        else if (!Pausing && m_PinState == PinPaused)
        {
            //
            // For unpausing the hardware, we need to compute the relative time
            // and restart interrupts.
            //
            LARGE_INTEGER UnpauseTime;

            KeQuerySystemTime (&UnpauseTime);
            m_InterruptTime = (ULONG) (
                                  (UnpauseTime.QuadPart - m_StartTime.QuadPart) /
                                  m_TimePerFrame
                              );

            UnpauseTime.QuadPart = m_StartTime.QuadPart +
                                   (m_InterruptTime + 1) * m_TimePerFrame;

            m_PinState = PinRunning;
            m_IsrTimer.Set( UnpauseTime );
        }
    }

    //  If we are pausing, it's best to stop the timer and not wait for another frame.
    if( Pausing )
    {
        m_IsrTimer.Cancel();
    }
    
    return STATUS_SUCCESS;
}

NTSTATUS
CHardwareSimulation::
Stop()

/*++

Routine Description:

    Stop the hardware simulation...

    For us, stop the timer, free the synthesizer and flush the queue.

Arguments:

    None

Return Value:

    Success / Failure

--*/

{
    PAGED_CODE();

    //
    // If the hardware is told to stop while it's running, we need to
    // halt the interrupts first.  If we're already paused, this has
    // already been done.
    //

    DBG_ENTER("(): m_PinID=%d", m_PinID);

    BOOLEAN bCancel = FALSE;

    {
        //  Prevent state-changes during this call.
        KScopedMutex    Lock(m_ListLock);

        bCancel = (m_PinState == PinRunning);
        m_PinState = PinStopped;

        //  Free the synthesis buffer.
        m_Synthesizer->Destroy();

        //
        // Free S/G buffer
        //
        FreeSGList( &m_ScatterGatherMappings, L"StreamPointer Stop SG List" );

        m_ScatterGatherMappingsQueued=0;
        m_NumMappingsCompleted = 0;
        m_ScatterGatherBytesQueued = 0;
    }

    //  If running, stop the timer to make sure we don't try to deliver one last frame.
    if( bCancel )
    {
        m_IsrTimer.Cancel();
    }

    return STATUS_SUCCESS;
}

NTSTATUS
CHardwareSimulation::
Reset()
{
    PAGED_CODE();

    KScopedMutex Lock( m_ListLock );
    DBG_ENTER("(): m_PinID=%d", m_PinID);

    FreeSGList( &m_ScatterGatherMappings, L"StreamPointer Reset SG List" );

    m_ScatterGatherMappingsQueued=0;
    m_NumMappingsCompleted = 0;
    m_ScatterGatherBytesQueued = 0;

    DBG_LEAVE("(): m_PinID=%d", m_PinID);
    return STATUS_SUCCESS;
}

NTSTATUS
CHardwareSimulation::
Reset (IN PKSPIN Pin)
{
    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;
    if(Pin->ResetState == KSRESET_END)
    {
        KsPinAttemptProcessing(Pin, TRUE);
    }
    else
    {
        status = Reset();
    }

    return status;
}

ULONG
CHardwareSimulation::
ReadNumberOfMappingsCompleted()

/*++

Routine Description:

    Read the number of scatter / gather mappings which have been
    completed since the last reset of the simulated hardware

Arguments:

    None

Return Value:

    Total number of completed mappings.

--*/

{
    PAGED_CODE();

    return m_NumMappingsCompleted;
}

/*************************************************/


ULONG
CHardwareSimulation::
ProgramScatterGatherMappings (
    IN PKSSTREAM_POINTER *Clone,
    IN PUCHAR *Buffer,
    IN PKSMAPPING Mappings,
    IN ULONG MappingsCount,
    IN ULONG MappingStride
)

/*++

Routine Description:

    Add a single entry to our queue.  In practice our simulation can only 
    accept one frame at a time, but we still base this code off of the 
    avshws sample.

Arguments:

    Buffer -
        The virtual address of the buffer mapped by the mapping list

    ClonePointer
        The KSStreamClone Pointer to be associated with the entry item

    Mappings -
        The KSMAPPINGS array corresponding to the buffer

    MappingsCount -
        The number of mappings in the mappings array

    MappingStride -
        The mapping stride used in initialization of AVStream DMA

Return Value:

    Number of mappings actually inserted.

--*/

{
    PAGED_CODE();

    ULONG MappingsInserted = 0;

    DBG_ENTER("(Clone=%p, Buffer=%p, MappingsCount=%d)",
              Clone, Buffer, MappingsCount);

    //
    // Protect our S/G list with a spinlock.
    //
    KScopedMutex Lock( m_ListLock );

    //
    // Loop through the scatter / gather list and break the buffer up into
    // chunks equal to the scatter / gather mappings.  Stuff the virtual
    // addresses of these chunks on a list somewhere.  We update the buffer
    // pointer the caller passes as a more convenient way of doing this.
    //
    // If I could just remap physical in the list to virtual easily here,
    // I wouldn't need to do it.
    //
    do
    {
        PSCATTER_GATHER_ENTRY Entry =
            reinterpret_cast <PSCATTER_GATHER_ENTRY> (
                ExAllocateFromNPagedLookasideList (
                    &m_ScatterGatherLookaside
                )
            );

        if (!Entry)
        {
            break;
        }

        Entry ->CloneEntry = nullptr;
        Entry -> Virtual    = nullptr;
        Entry -> ByteCount  = 0;


        Entry ->CloneEntry = *Clone;
        Entry -> CloneEntry -> StreamHeader -> PresentationTime.Time = 0;
        Entry -> CloneEntry -> StreamHeader -> OptionsFlags &= ~KSSTREAM_HEADER_OPTIONSF_TIMEVALID;

        Entry -> Virtual    = *Buffer;
        Entry -> ByteCount  = MappingsCount;
        Entry->PhotoConfirmationInfo = PHOTOCONFIRMATION_INFO();

        //
        // Move forward a specific number of bytes in chunking this into
        // mapping sized va buffers.
        //
        *Buffer += MappingsCount;
        Mappings = reinterpret_cast <PKSMAPPING> (
                       (reinterpret_cast <PUCHAR> (Mappings) + MappingStride)
                   );

        InsertTailList (&m_ScatterGatherMappings, &(Entry -> ListEntry));
        m_ScatterGatherMappingsQueued++;
        MappingsInserted = MappingsCount;
        DBG_TRACE("m_ScatterGatherMappingsQueued=%d", m_ScatterGatherMappingsQueued);
        m_ScatterGatherBytesQueued += MappingsCount;

    }
    while(FALSE);

    DBG_LEAVE(" MappingsInserted=%d", MappingsInserted );

    return MappingsInserted;

}

/**************************************************************************

    The following are helper functions used in the production of metadata.
    Since this is just a simulation, most "auto" modes get converted into
    a random value that is in range.

    While simple, this still requires writing a helper function for each
    piece of data.

**************************************************************************/

//
//  Translate Exposure settings into a "current value".
//
LONGLONG
CHardwareSimulation::
GetCurrentExposureTime()
{
    PAGED_CODE();

    ISP_FRAME_SETTINGS *pSettings = GetIspSettings();
    LONGLONG   Value = 0;
    LPCSTR      Mode = "[UNKNOWN]";

    if( pSettings->ExposureMode & KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO )
    {
        //  Get random value in global setting's bound (LONG)
        //  I'm abandoning the reported min/max and using something more reasonable.
        Value = GetRandom( MIN_EXPOSURE_TIME*5, DEF_EXPOSURE_TIME*5 );
        Mode = "KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO";
    }

    if( pSettings->ExposureMode & KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_MANUAL )
    {
        Value = pSettings->ExposureSetting.VideoProc.Value.ll;
        Mode = "KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_MANUAL";
    }

    //  Locked just reports the last value set...
    if( pSettings->ExposureMode & KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_LOCK )
    {
        Value = m_LastReportedExposureTime;
        Mode = "KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_LOCK";
    }

    DBG_TRACE("ExposureMode=0x%016llX (%s), Time=%llu00ns, LastReported=%llu00ns",
              pSettings->ExposureMode, Mode, Value, m_LastReportedExposureTime );

    m_LastReportedExposureTime = Value;
    return Value;
}

//
//  Translate White Balance into a "current value".
//
ULONG
CHardwareSimulation::
GetCurrentWhiteBalance()
{
    PAGED_CODE();

    ISP_FRAME_SETTINGS *pSettings = GetIspSettings();
    ULONG   Value = 0;

    if( pSettings->WhiteBalanceMode & KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO )
    {
        Value = (ULONG) GetRandom( pSettings->WhiteBalanceSetting.Min, pSettings->WhiteBalanceSetting.Max );
    }

    if( pSettings->WhiteBalanceMode & KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_MANUAL )
    {
        Value = pSettings->WhiteBalanceSetting.VideoProc.Value.ul;
    }

    //  Locked just reports the last value set...
    if( pSettings->WhiteBalanceMode & KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_LOCK )
    {
        Value = m_LastReportedWhiteBalance;
    }

    m_LastReportedWhiteBalance= Value;
    return Value;
}

//
//  Translate ISO settings into a "current value".
//
ULONG
CHardwareSimulation::
GetCurrentISOSpeed()
{
    PAGED_CODE();

    ISP_FRAME_SETTINGS *pSettings = GetIspSettings();
    ULONG   Value = 0;

    if( pSettings->ISOMode & KSCAMERA_EXTENDEDPROP_ISO_AUTO )
    {
        Value = GetRandom( (ULONG) 50, (ULONG) 3200 );
    }
    else if( pSettings->ISOMode & KSCAMERA_EXTENDEDPROP_ISO_MANUAL )
    {
        Value = pSettings->ISOValue;
    }
    else
    {
        Value =             //  Try converting any legacy presets to a manual value.
            IsoPreset2Value( pSettings->ISOMode );
    }
    return Value;
}

//
//  Translate Exposure Mode to an Exif Exposure Program value
//
USHORT
CHardwareSimulation::
GetExposureProgram()
{
    PAGED_CODE();

    ISP_FRAME_SETTINGS *pSettings = GetIspSettings();
    USHORT   Value = 0;  // Undefined.

    if( pSettings->ExposureMode & KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_MANUAL )
    {
        Value = 1;  // Manual
    }
    if( pSettings->ExposureMode & KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO )
    {
        Value = 2;  // Normal
    }

    //  TODO: Extend SceneMode to ISP settings so I can use it here to capture Portrait mode, etc.
    return Value;
}

/*************************************************/

//
//  Emit metadata here that is common to all pins.
//
//  All derived classes' EmitMetadata() function must
//  call this function first.
//
void
CHardwareSimulation::
EmitMetadata(
    _Inout_ PKSSTREAM_HEADER   pStreamHeader
)
{
    PAGED_CODE();

    NT_ASSERT(pStreamHeader);

    if (0 != (pStreamHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_METADATA))
    {
        PKS_FRAME_INFO          pFrameInfo = (PKS_FRAME_INFO)(pStreamHeader + 1);
        PKSSTREAM_METADATA_INFO pMetadata = (PKSSTREAM_METADATA_INFO) (pFrameInfo + 1);
        //PBYTE                   pData = (PBYTE) pMetadata->SystemVa;
        //ULONG                   BytesLeft = pMetadata->BufferSize;

        // A real driver might write focus state or other info here.  We've got nothing.
        pMetadata->UsedSize = 0;
    }
    else
    {
        DBG_TRACE("Metadata not present...");
    }
}

void
CHardwareSimulation::
EmitFaceMetadata(
    _Inout_ PKSSTREAM_HEADER    pStreamHeader,
    _In_    ULONG               Count,
    _In_    ULONGLONG           Flags,
    _In_    ULONG               DelayLimit
)
/*++

Routine Description:

     Helper function that emits FaceDetection metadata.

     Note: This function assumes a metadata buffer is present.

Arguments:

    pStreamHeader -
        The frame's stream header.

    Count -
        The maximum number of faces to detect.

    Flags -
        A combination of the KSCAMERA_EXTENDEDPROP_FACEDETECTION_BLINK and
        KSCAMERA_EXTENDEDPROP_FACEDETECTION_BLINK flags passed to the face
        detection control.  Used to decide what characterization data to
        present in the metadata.

    DelayLimit -
        The maximum number of frames to delay before generating the next
        face detection.

Return Value:

    None.

--*/
{
    PAGED_CODE();

    NT_ASSERT(pStreamHeader);
    NT_ASSERT(Count <= SIZEOF_ARRAY(m_LastFaceDetect.Data));

    //  Do nothing if we're turned on, but they want no faces.
    if( Count==0 )
    {
        return;
    }

    if( --m_FaceDetectionDelay != 0 ||      // Count down our random delay for new face data.
            Count > m_LastFaceDetect.Count )    // Make sure we never emit more faces than requested.
    {
        ULONG   CountReported = GetRandom( 0, Count );

        DBG_TRACE("Generating new faces (Count=%d, Flags=0x%016llX, Delay=%d)...",
                  Count, Flags, DelayLimit);

        //  Synthesize a new set of "detected" faces...
        m_LastFaceDetect.Header.MetadataId = (ULONG) MetadataId_Custom_FaceDetection;
        m_LastFaceDetect.Header.Size =
            sizeof(CAMERA_METADATA_FACEHEADER) + (sizeof(METADATA_FACEDATA) * CountReported);

        m_LastFaceDetect.Count = CountReported;
        m_LastFaceDetect.Flags = Flags;
        m_LastFaceDetect.Timestamp = pStreamHeader->PresentationTime.Time ;

        DBG_TRACE("Count=%d, Flags=0x%016llX, Time=0x%016llX",
                  m_LastFaceDetect.Count,
                  m_LastFaceDetect.Flags,
                  m_LastFaceDetect.Timestamp );

        //  Report face data...
        for( ULONG i=0; i<CountReported; i++ )
        {
            RtlZeroMemory( &m_LastFaceDetect.Data[i], sizeof(m_LastFaceDetect.Data[i]) );

            RECT &rect = m_LastFaceDetect.Data[i].Region;
            rect.top = GetRandom((LONG)0, TO_Q31(1)-2);
            rect.left = GetRandom((LONG)0, TO_Q31(1)-2);
            rect.bottom = GetRandom(rect.top+1, TO_Q31(1)-1);
            rect.right = GetRandom(rect.left+1, TO_Q31(1)-1);

            m_LastFaceDetect.Data[i].confidenceLevel = GetRandom((ULONG)25, 100);    // Don't bother reporting really low confidence faces.

            DBG_TRACE("  #%d: [(0x%08X,0x%08X), (0x%08X,0x%08X)](Conf=%d%%)",
                      i,
                      rect.top, rect.left, rect.bottom, rect.right,
                      m_LastFaceDetect.Data[i].confidenceLevel);

            if( Flags & KSCAMERA_EXTENDEDPROP_FACEDETECTION_BLINK )
            {
                m_LastFaceDetect.Data[i].BlinkScoreLeft = GetRandom((ULONG)0, 100);     // Should really skew these numbers more in favor of no blink.
                m_LastFaceDetect.Data[i].BlinkScoreRight = GetRandom((ULONG)0, 100);

                DBG_TRACE("      --- Blink(left=%d%%, right=%d%%)",
                          m_LastFaceDetect.Data[i].BlinkScoreLeft,
                          m_LastFaceDetect.Data[i].BlinkScoreRight);
            }

            if( Flags & KSCAMERA_EXTENDEDPROP_FACEDETECTION_SMILE )
            {
                m_LastFaceDetect.Data[i].FacialExpression = GetRandom((ULONG)0, 1) ? EXPRESSION_SMILE : 0;
                m_LastFaceDetect.Data[i].FacialExpressionScore = GetRandom((ULONG)0, 100);

                DBG_TRACE("      --- Smile(exp=%s, score=%d%%)",
                          (m_LastFaceDetect.Data[i].FacialExpression==EXPRESSION_SMILE ? "Smile":"[unknown]"),
                          m_LastFaceDetect.Data[i].FacialExpressionScore);
            }
        }

        //  Randomly choose when to generate new facedetection data.
        m_FaceDetectionDelay = GetRandom( 1, DelayLimit );
        DBG_TRACE("Delay to next is %d", m_FaceDetectionDelay);
    }

    //  The flags can change between face detection updates.
    //  Drop the additional characterization data immediately, if requested.
    //  We can hardly add it that fast since it might not be generated straight away.
    ULONGLONG   LostFlags = m_LastFaceDetect.Flags & ~Flags;
    for( ULONG i=0; i<m_LastFaceDetect.Count; i++ )
    {
        if( LostFlags & KSCAMERA_EXTENDEDPROP_FACEDETECTION_BLINK )
        {
            m_LastFaceDetect.Data[i].BlinkScoreLeft = 0;
            m_LastFaceDetect.Data[i].BlinkScoreRight = 0;
        }
        if( LostFlags & KSCAMERA_EXTENDEDPROP_FACEDETECTION_SMILE )
        {
            m_LastFaceDetect.Data[i].FacialExpression = 0;
            m_LastFaceDetect.Data[i].FacialExpressionScore = 0;
        }
    }
    m_LastFaceDetect.Flags &= Flags;

    PKS_FRAME_INFO          pFrameInfo = (PKS_FRAME_INFO)(pStreamHeader + 1);
    PKSSTREAM_METADATA_INFO pMetadata = (PKSSTREAM_METADATA_INFO) (pFrameInfo + 1);
    ULONG                   BytesLeft = pMetadata->BufferSize - pMetadata->UsedSize;

    //  Write Face Detection Info here
    //  Note: If we derived Video and Preview from a common base class, we
    //        might use that to emit this information for both types.
    if( BytesLeft >= m_LastFaceDetect.Header.Size )
    {
        //  Always write the most recently generated face detection info.
        RtlCopyMemory(
            ((PBYTE)pMetadata->SystemVa) + pMetadata->UsedSize,
            &m_LastFaceDetect,
            m_LastFaceDetect.Header.Size );

        DBG_TRACE("Emitting Face metadata...");

        BytesLeft -= m_LastFaceDetect.Header.Size;
        pMetadata->UsedSize += m_LastFaceDetect.Header.Size;
    }
    else
    {
        DBG_TRACE("WARNING: Out of metadata buffer...");
    }
}

//
//  PIN_STATE text for debug output.
//
static
const char *
GetPinStateTxt( PIN_STATE state )
{
    PAGED_CODE();

    switch( state )
    {
    case PinStopped:
        return "STOPPED";
    case PinPaused:
        return "PAUSED";
    case PinRunning:
        return "RUNNING";
    }
    return "[UNKNOWN]";
}

NTSTATUS
CHardwareSimulation::
FillScatterGatherBuffers()

/*++

Routine Description:

    The hardware has synthesized a buffer and we're to fill a frame buffer.

Arguments:

    None

Return Value:

    Success / Failure

--*/

{
    PAGED_CODE();

    DBG_ENTER("() m_PinState=%s, m_PinID=0x%08X, m_ImageSize=0x%08X, m_ScatterGatherMappingsQueued=%u, "
              "m_ScatterGatherBytesQueue=0x%08X",
              GetPinStateTxt(m_PinState),
              m_PinID, m_ImageSize, m_ScatterGatherMappingsQueued, m_ScatterGatherBytesQueued);

    NTSTATUS    ntStatus = STATUS_SUCCESS;

    ULONG BufferRemaining = m_ImageSize;

    //
    //  If there isn't a frame buffer queued, we justskip the frame and consider 
    //  it starvation.
    //
    while (BufferRemaining &&
            !IsListEmpty(&m_ScatterGatherMappings) &&
            m_ScatterGatherBytesQueued >= BufferRemaining)
    {
        LIST_ENTRY *listEntry = RemoveHeadList (&m_ScatterGatherMappings);
        m_ScatterGatherMappingsQueued--;

        PSCATTER_GATHER_ENTRY SGEntry =
            reinterpret_cast <PSCATTER_GATHER_ENTRY> (
                CONTAINING_RECORD (
                    listEntry,
                    SCATTER_GATHER_ENTRY,
                    ListEntry
                )
            );

        //  Deal with cancellation.
        PIRP pIrp = KsStreamPointerGetIrp(SGEntry->CloneEntry, FALSE, FALSE);
        if (pIrp)
        {
            if (pIrp->Cancel)
            {
                DBG_TRACE( "Cancelling..." );
                FreeSGEntry( listEntry, L"StreamPointer Cancel SG List" );
                continue;
            }
        }

        //
        // Since we're software, we'll be accessing this by virtual address...
        //
        ULONG Stride = 0;
        if ( SGEntry->CloneEntry -> StreamHeader -> Size >= sizeof (KSSTREAM_HEADER) +
                sizeof (KS_FRAME_INFO))
        {
            PKS_FRAME_INFO FrameInfo = reinterpret_cast <PKS_FRAME_INFO> (SGEntry->CloneEntry->StreamHeader+1);
            Stride = (ULONG) ABS(FrameInfo->lSurfacePitch);
        }

        //  Have the synthesizer output a frame to the buffer.
        ULONG   BytesCopied = m_Synthesizer->DoCommit( SGEntry->Virtual, SGEntry->ByteCount, Stride );
        NT_ASSERT( BytesCopied );
        DBG_TRACE( "BytesCopied = %d", BytesCopied );

        //Adding time stamp
        if(m_PhotoConfirmationEntry.isRequired())
        {
            SGEntry -> CloneEntry -> StreamHeader -> PresentationTime.Time =
                m_PhotoConfirmationEntry.getTime();
            DBG_TRACE("Using Photo Confirmation time = 0x%016llX", m_PhotoConfirmationEntry.getTime());
        }
        else
        {
            SGEntry -> CloneEntry -> StreamHeader -> PresentationTime.Time = ConvertQPCtoTimeStamp(NULL);
            DBG_TRACE("PresentationTime = 0x%016llX", SGEntry->CloneEntry->StreamHeader->PresentationTime.Time );
        }

        SGEntry -> CloneEntry -> StreamHeader -> PresentationTime.Numerator =
            SGEntry -> CloneEntry -> StreamHeader -> PresentationTime.Denominator = 1;

        SGEntry -> CloneEntry -> StreamHeader -> OptionsFlags |= KSSTREAM_HEADER_OPTIONSF_TIMEVALID ;

        // Add metadata to the sample.
        EmitMetadata( SGEntry -> CloneEntry -> StreamHeader );

        BufferRemaining = 0; //-= BytesCopied;
        m_NumMappingsCompleted++;
        m_ScatterGatherBytesQueued -= SGEntry -> ByteCount;

        //
        // Release the scatter / gather entry back to our lookaside.
        //
        ExFreeToNPagedLookasideList (
            &m_ScatterGatherLookaside,
            reinterpret_cast <PVOID> (SGEntry)
        );

    }

    //  Report an error if we used the last buffer.
    if (BufferRemaining)
    {
        //DBG_TRACE("BufferRemaining=%u", BufferRemaining);
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    DBG_LEAVE("() = 0x%08X", ntStatus);
    return ntStatus;
}

/**************************************************************************

    Debug helpers

**************************************************************************/

PCCHAR
DVS_Text( ULONGLONG Flags )
{
    PAGED_CODE();

    switch( Flags )
    {
    case KSCAMERA_EXTENDEDPROP_VIDEOSTABILIZATION_OFF:  return "Off";
    case KSCAMERA_EXTENDEDPROP_VIDEOSTABILIZATION_ON:   return "On";
    case KSCAMERA_EXTENDEDPROP_VIDEOSTABILIZATION_AUTO: return "Auto";
    default:                                            return "Unknown";
    }
}

PCCHAR
OIS_Text( ULONGLONG Flags )
{
    PAGED_CODE();

    switch( Flags )
    {
    case KSCAMERA_EXTENDEDPROP_OIS_OFF:     return "Off";
    case KSCAMERA_EXTENDEDPROP_OIS_ON:      return "On";
    case KSCAMERA_EXTENDEDPROP_OIS_AUTO:    return "Auto";
    default:                                return "Unknown";
    }
}

/**************************************************************************/

void
CHardwareSimulation::
FakeHardware (
)

/*++

Routine Description:

    Simulate an interrupt and what the hardware would have done in the
    time since the previous interrupt.

Arguments:

    None

Return Value:

    None

--*/

{
    PAGED_CODE();

    KScopedMutex Lock( m_ListLock );

    m_InterruptTime++;

    //
    // The hardware can be in a pause state in which case, it issues interrupts
    // but does not complete mappings.  In this case, don't bother synthesizing
    // a frame and doing the work of looking through the mappings table.
    //
    if (m_PinState == PinRunning)
    {
        //
        // Generate a "time stamp" just to overlay it onto the capture image.
        // It makes it more exciting than bars that do nothing.
        //
        ULONGLONG time = ConvertQPCtoTimeStamp(NULL);
        DBG_TRACE("QPC=0x%016llX", time);

        m_Synthesizer->SetFrameNumber( m_InterruptTime );
        m_Synthesizer->SetRelativePts( (m_InterruptTime + 1) * m_TimePerFrame );
        m_Synthesizer->SetQpcTime( time );

        m_Synthesizer->DoSynthesize();

        CHAR Text[64];

        CExtendedProperty   Control;
        m_Sensor->GetVideoStabilization( &Control );
        RtlStringCbPrintfA(Text, sizeof(Text), "DVS: %s", DVS_Text(Control.Flags));
        m_Synthesizer->OverlayText( 0, m_Height-38, 1, Text, BLACK, WHITE );

        m_Sensor->GetOpticalImageStabilization( &Control );
        RtlStringCbPrintfA(Text, sizeof(Text), "OIS: %s", OIS_Text(Control.Flags));
        m_Synthesizer->OverlayText( 0, m_Height-48, 1, Text, BLACK, WHITE );

        //
        // Fill scatter gather buffers
        //
        if (!NT_SUCCESS (FillScatterGatherBuffers ()))
        {
            InterlockedIncrement (PLONG (&m_NumFramesSkipped));
        }

    }

    //
    // Issue an interrupt to our hardware sink.  This is a "fake" interrupt.
    // It will occur at DISPATCH_LEVEL.
    //
    m_Sensor -> Interrupt (m_PinID);

    //
    //  Schedule the timer for the next interrupt time, if the pin is still running.
    //
    if( m_PinState == PinRunning )
    {
        LARGE_INTEGER NextTime;
        NextTime.QuadPart = m_StartTime.QuadPart +
                            (m_TimePerFrame * (m_InterruptTime + 1));

#ifdef ENABLE_TRACING  // To keep us from a tight spin when trying to debug this code...
        LARGE_INTEGER Now;
        KeQuerySystemTime(&Now);

        if( Now.QuadPart >= NextTime.QuadPart )
        {
            NextTime.QuadPart = 0LL - m_TimePerFrame ;
        }

#endif
        m_IsrTimer.Set( NextTime );
    }
}

ULONGLONG
CHardwareSimulation::
ConvertQPCtoTimeStamp(
    _In_opt_    PLARGE_INTEGER pQpcTime
)
/*++

Routine Description:

    Get the QPC measured in 100ns units.

Arguments:

    pQpcTime - 
        An optional time value to convert.  If NULL, use the current QPC.

Return Value:

    ULONLONG -
        The time in 100ns increments.

--*/
{
    PAGED_CODE();

    LARGE_INTEGER qpcTime = {0};
    LARGE_INTEGER qpcFrequency;

    qpcTime = KeQueryPerformanceCounter(&qpcFrequency);

    if( pQpcTime )
    {
        qpcTime = *pQpcTime;
    }

    return KSCONVERT_PERFORMANCE_TIME( qpcFrequency.QuadPart, qpcTime );
}

NTSTATUS
CHardwareSimulation::
GeneratePhotoConfirmation(
    _In_ ULONG      PfsFrameNumber,
    _In_ LONGLONG   time
)
/*++

Routine Description:

    So a photo sim can ask a preview sim for a confirmation frame.

Arguments:

    PfsFrameNumber -
        The VPS shot number.
    time -
        The precise time of the associated photo.

Return Value:

    None

--*/
{
    PAGED_CODE();

    KScopedMutex Lock( m_ListLock );

    if( PinRunning != m_PinState )
    {
        return STATUS_DEVICE_NOT_READY;
    }

    DBG_ENTER("( Index=%d, Time=0x%016llX )", PfsFrameNumber, time );

    m_PhotoConfirmationEntry = PHOTOCONFIRMATION_INFO( PfsFrameNumber, time );

    // We basically used the previously synthesized preview image
    NTSTATUS status = FillScatterGatherBuffers();

    //Clean up the photo confiramtion flag
    m_PhotoConfirmationEntry = PHOTOCONFIRMATION_INFO();

    if(NT_SUCCESS(status))
    {
        m_Sensor->Interrupt (m_PinID);
    }

    DBG_LEAVE("() = 0x%08X", status);

    return status;
}

void
CHardwareSimulation::
FreeSGEntry(
    _In_        PLIST_ENTRY Entry,
    _In_        PCWSTR      EventName,
    _In_opt_    LPCGUID     Activity,
    _In_        ULONG       DataUsed
)
{
    PAGED_CODE();

    PSCATTER_GATHER_ENTRY SGEntry =
        reinterpret_cast <PSCATTER_GATHER_ENTRY> (
            CONTAINING_RECORD (
                Entry,
                SCATTER_GATHER_ENTRY,
                ListEntry
            )
        );

    NT_ASSERT(SGEntry->CloneEntry);
    NT_ASSERT(SGEntry->CloneEntry->StreamHeader);

    SGEntry->CloneEntry->StreamHeader->DataUsed = DataUsed;

    KsStreamPointerDelete(SGEntry->CloneEntry);

    //
    // Release the scatter / gather entry back to our lookaside.
    //
    ExFreeToNPagedLookasideList (
        &m_ScatterGatherLookaside,
        SGEntry
    );
}

void
CHardwareSimulation::
FreeSGList(
    _In_        PLIST_ENTRY List,
    _In_        PCWSTR      EventName,
    _In_opt_    LPCGUID     Activity
)
{
    PAGED_CODE();

    while(!IsListEmpty(List))
    {
        PLIST_ENTRY head = RemoveHeadList(List);
        m_ScatterGatherMappingsQueued--;
        FreeSGEntry( head, EventName, Activity );
    }
}

//
//  Get the current frame settings.
//
ISP_FRAME_SETTINGS *
CHardwareSimulation::
GetIspSettings(void)
{
    PAGED_CODE();

    return m_Sensor->GetGlobalIspSettings();
}
