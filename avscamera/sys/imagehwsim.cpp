/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2001, Microsoft Corporation.

    File:

        ImageHwSim.cpp

    Abstract:

        This file contains the implementation of the CImageHardwareSimulation 
        class.

        This is a specialization of CHardwareSimulation that provides photo-
        specific metadata and implements specialized functionality for photo, 
        photo sequence and variable photo sequence.

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


CImageHardwareSimulation::
CImageHardwareSimulation (
    _Inout_ CSensor *Sensor,
    _In_    LONG    PinID
)
    : CHardwareSimulation( Sensor, PinID )
    , m_Clock(NULL)
    , m_PfsLoopLimit(0)
    , m_PfsFrameLimit(0)
    , m_pIspSettings(nullptr)
    , m_PfsLoopNumber(0)
    , m_PfsFrameNumber(0)
    , m_GlobalFrameNumber(0)
    , m_bEndOfSequence(FALSE)
    , m_PastBufferCount(0)         // Zero only when the simulation inits.

/*++

Routine Description:

    Construct a hardware simulation

Arguments:

    Sensor -
        The hardware sink interface.  This is used to trigger
        fake interrupt service routines from.

Return Value:

    Success / Failure

--*/

{
    PAGED_CODE();

    InitializeListHead (&m_BurstList);
}

CImageHardwareSimulation::
~CImageHardwareSimulation()
{

    PAGED_CODE();

    SAFE_DELETE( m_pIspSettings );

    if (m_Clock)
    {
        m_Clock -> Release ();
        m_Clock = NULL;
    }
}

/*************************************************/

NTSTATUS
CImageHardwareSimulation::
Start (
    _In_    CSynthesizer *ImageSynth,
    _In_    ULONG Width,
    _In_    ULONG Height,
    _In_    ULONG ImageSize,
    _In_    PIN_MODE pinMode
)

/*++

Routine Description:

    Start capturing frames.  This turns on the timer and begins frame capture,
    but it we do not deliver frames until we receive a Trigger.  We keep track 
    of starvation starting at this point.

Arguments:

    ImageSynth -
        The image synthesizer to use to generate pictures to display
        on the capture buffer.

    Width -
        The image width

    Height -
        The image height

    ImageSize -
        The size of the image.  We allocate a temporary scratch buffer
        based on this size to fake hardware.

    PinMode - 
        Normal or photosequence.
        

Return Value:

    Success / Failure (typical failure will be out of memory on the
    scratch buffer, etc...)

--*/

{

    PAGED_CODE();

    DBG_ENTER("(Width=%d, Height=%d, ImageSize=%d, pinMode=%s): m_PinID=%d",
              Width, Height, ImageSize, pinMode?"PinBurstMode":"PinNormalMode", m_PinID );

    NTSTATUS Status = STATUS_SUCCESS;

    //  Prevent state-changes during this call.
    KScopedMutex    Lock(m_ListLock);

    m_Synthesizer = ImageSynth;

    NT_ASSERT(ImageSize);
    m_ImageSize = ImageSize;
    m_Height = Height;
    m_Width = Width;

    m_NumMappingsCompleted = 0;
    m_ScatterGatherMappingsQueued = 0;
    m_NumFramesSkipped = 0;
    m_InterruptTime = 0;
    m_bTriggered = FALSE;
    m_bEndOfSequence = FALSE;
    m_pClone = NULL;
    m_bPastBufferTrigger = FALSE;
    m_PinMode = pinMode;
    m_TriggerTime = 0;
    m_bFlashed = FALSE;

    DBG_TRACE("m_bTriggered=FALSE, m_bPastBufferTrigger=FALSE");

    m_FlashStatus = 0;

    //  Initialize VPS counters.
    m_PfsLoopNumber = 0 ;
    m_PfsFrameNumber= 0 ;
    m_GlobalFrameNumber = 0;

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

    DBG_LEAVE("(Width=%d, Height=%d, ImageSize=%d, pinMode=%s): m_PinID=%d, Status=0x%08X",
              Width, Height, ImageSize, pinMode?"PinBurstMode":"PinNormalMode", m_PinID, Status );

    return Status;

}

NTSTATUS
CImageHardwareSimulation::
Trigger(
    _In_    LONG mode
)
/*++

Routine Description:

    Take a picture / Begin delivering frames.

Arguments:

    mode - 
        Normal trigger, start or stop photo sequence.
        

Return Value:

    Success / Failure (typical failure will be out of memory on the
    scratch buffer, etc...)

--*/

{
    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;

    DBG_ENTER( "(mode=0x%08X), m_PinMode=%d", mode, m_PinMode );

    KScopedMutex Lock( m_ListLock );

    //Start Trigger for Burst Mode
    if(mode  & KS_VideoControlFlag_StartPhotoSequenceCapture)
    {
        if(m_bTriggered == FALSE && m_PinMode == PinBurstMode)
        {
            m_bPastBufferTrigger = TRUE;
            m_bTriggered = TRUE;
            DBG_TRACE("m_bTriggered=TRUE, m_bPastBufferTrigger=TRUE");

            status = STATUS_SUCCESS;
        }
        else
        {
            status = STATUS_INVALID_PARAMETER;
        }
    }
    //Stop Trigger for Burst Mode
    else if(mode  & KS_VideoControlFlag_StopPhotoSequenceCapture)
    {
        if(m_bTriggered == TRUE && m_PinMode == PinBurstMode)
        {
            m_bTriggered = FALSE;
            DBG_TRACE("m_bTriggered=FALSE");

            //  reset the PFS EOS, frame number and loop count.
            m_bEndOfSequence = FALSE;
            m_PfsFrameNumber = 0;
            m_PfsLoopNumber  = 0;

            m_bFlashed = FALSE;
            status = STATUS_SUCCESS;
        }
        else
        {
            status = STATUS_INVALID_PARAMETER;
        }
    }
    //Normal Trigger
    else if(mode & KS_VideoControlFlag_Trigger )
    {
        if(m_PinMode == PinBurstMode)
        {
            status = STATUS_INVALID_PARAMETER;
        }
        else
        {
            m_bTriggered = TRUE;
            m_bEndOfSequence = FALSE;
            DBG_TRACE("m_bTriggered=TRUE");
        }
    }

    DBG_LEAVE("()");

    return STATUS_SUCCESS;
}


NTSTATUS
CImageHardwareSimulation::
SetMode(
    _In_    ULONGLONG Flags,
    _In_    ULONG PastBuffers
)
/*++

Routine Description:

    Set the photo sequence mode.

Arguments:

    Flags - 
        Normal or photo sequence.

    PastBuffers -
        Number of history frames to gather.
        

Return Value:

    Success / Failure (typical failure will be out of memory on the
    scratch buffer, etc...)

--*/

{
    PAGED_CODE();

    DBG_ENTER( "(Flags=0x%016llX, PastBuffers=%d)", Flags, PastBuffers );

    if(m_bTriggered == TRUE)
    {
        return STATUS_INVALID_TRANSACTION;
    }

    m_PastBufferCount = PastBuffers;

    if(Flags & KSCAMERA_EXTENDEDPROP_PHOTOMODE_SEQUENCE)
    {
        m_PinMode = PinBurstMode;
    }
    else
    {
        m_PinMode = PinNormalMode;
    }

    DBG_LEAVE("()");

    return STATUS_SUCCESS;
}

//
//  Set the interval between frames here.
//
NTSTATUS
CImageHardwareSimulation::
SetPhotoFrameRate(
    _In_    ULONGLONG TimePerFrame
)
{
    PAGED_CODE();

    //  Prevent state-changes during this call.
    KScopedMutex    Lock(m_ListLock);

    m_TimePerFrame = TimePerFrame;

    //
    // Reschedule the timer if the hardware isn't being stopped.
    //
    if( m_PinState == PinRunning )  // && !m_StopHardware )
    {
        //  First restart our start time.  We can't use the old time.
        KeQuerySystemTime( &m_StartTime );

        //
        // Reschedule the timer for the next interrupt time.
        //
        m_StartTime.QuadPart += m_TimePerFrame;
        m_InterruptTime = 0;

        m_IsrTimer.Set( m_StartTime );
    }
    return STATUS_SUCCESS;
}

NTSTATUS
CImageHardwareSimulation::
Stop()

/*++

Routine Description:

    Stop the hardware simulation...
    
    Wait until the timer has stopped, flush the queue, dereference the clock 
    and reset our state before returning.

Arguments:

    None

Return Value:

    Success / Failure

--*/

{
    PAGED_CODE();

    // If the hardware is told to stop while it's running, we need to
    // halt the interrupts first.  If we're already paused, this has
    // already been done.
    //

    DBG_ENTER("(): m_PinID=%d", m_PinID);

    //
    // Protect the S/G list
    //
    KScopedMutex Lock( m_ListLock );

    CHardwareSimulation::Stop();

    //
    // Free S/G buffer
    //
    FreeSGList( &m_ScatterGatherMappings, L"StreamPointer Stop Burst List" );

    if (m_Clock)
    {
        m_Clock -> Release ();
        m_Clock = NULL;
    }

    m_bTriggered = FALSE;
    m_bEndOfSequence = FALSE;
    m_pClone = NULL;
    m_bPastBufferTrigger = FALSE;
    m_PinMode = PinNormalMode;
    m_TriggerTime = 0;

    DBG_TRACE("m_bTriggered=FALSE, m_bPastBufferTrigger=FALSE");

    DBG_LEAVE("(): m_PinID=%d", m_PinID);

    return STATUS_SUCCESS;
}


/*************************************************/


//
//  Helper function that collects current settings into our metadata structure.
//
METADATA_IMAGEAGGREGATION
CImageHardwareSimulation::
GetMetadata()
{
    PAGED_CODE();

    METADATA_IMAGEAGGREGATION Metadata;
    ISP_FRAME_SETTINGS *pSettings = GetIspSettings();

    //  Wipe the metadata so all settings will default to "Not Set".
    RtlZeroMemory( &Metadata, sizeof(Metadata) );

    //  Identify the current PFS frame number.
    //  If PFS not active, then this item is not present.
    Metadata.FrameId.Set = IsPfsActive();
    Metadata.FrameId.Value = (ULONG) m_PfsFrameNumber;
    DBG_TRACE("Metadata.FrameId.Set=%s, Metadata.FrameId.Value=%d",
              (Metadata.FrameId.Set?"Yes":"No"), Metadata.FrameId.Value);

    //  Just reflect the exposure time from the setting.
    //Metadata.ExposureTime.Set = TRUE;
    //Metadata.ExposureTime.Value = GetCurrentExposureTime();

    //  Just reflect the ISO Speed from the setting.
    Metadata.ISOSpeed = CMetadataLong(GetCurrentISOSpeed());
    DBG_TRACE("ISO=%d, ISO Flags=0x%016llX", Metadata.ISOSpeed.Value, pSettings->ISOMode);

    //  TODO: Do we need to bracket this by whether or not a flash has been taken?
    //  Report the current flash mode.
    Metadata.FlashOn = CMetadataLong((ULONG) pSettings->FlashMode);

    //  Report the current flash power.
    Metadata.FlashPower = CMetadataLong(pSettings->FlashValue);

    //  Set the White Balance lock state.
    Metadata.WhiteBalanceLocked = CMetadataLong(
                                      ( (pSettings->WhiteBalanceMode & KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_LOCK)
                                        == KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_LOCK) );

    //  Set the Exposure lock state.
    Metadata.ExposureLocked = CMetadataLong(
                                  ( (pSettings->ExposureMode & KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_LOCK)
                                    == KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_LOCK) );

    Metadata.ExposureTime =
        //CMetadataRational(GetCurrentExposureTime(), 10000000);
        CMetadataLongLong( GetCurrentExposureTime() );

    Metadata.LensPosition = CMetadataLong( pSettings->FocusSetting.VideoProc.Value.ul );

    Metadata.SceneMode = CMetadataULongLong(KSCAMERA_EXTENDEDPROP_SCENEMODE_AUTO);   //TODO: Need to fill in real value from CCaptureFilter::m_SceneMode

    Metadata.WhiteBalanceMode = CMetadataLong((ULONG) pSettings->WhiteBalanceMode);

    CExtendedVidProcSetting Zoom;
    m_Sensor->GetZoom( &Zoom );
    Metadata.ZoomFactor = CMetadataLong(Zoom.GetLONG());  //TODO: Fill in a real value from zoom simulation.

    Metadata.FocusLocked = CMetadataLong(FALSE);    //TODO: Fill in a real value when we complete the focus changes.

    //  Add EVCompensation metadata...
    Metadata.EVCompensation = CMetadataEVCompensation(pSettings->EVCompensation.Mode, pSettings->EVCompensation.Value);

    Metadata.Orientation = CMetadataShort(Metadata_Orientation_TopBottomLeftRight); //TODO: Randomize?

    {
        LARGE_INTEGER   SystemTime;
        LARGE_INTEGER   LocalTime;

        KeQuerySystemTimePrecise( &SystemTime );
        ExSystemTimeToLocalTime( &SystemTime, &LocalTime );
        RtlTimeToTimeFields( &LocalTime, &Metadata.LocalTime.Time );
        Metadata.LocalTime.Set = TRUE;
    }

    Metadata.Make = CMetadataShortString("Make: Microsoft SOC Camera");
    Metadata.Model = CMetadataShortString( "Model: AvsCam" );
    Metadata.Software = CMetadataShortString( "Software: Microsoft Camera Sim" );

    Metadata.ColorSpace.Set = TRUE;
    Metadata.ColorSpace.Value = 0xFFFF; // 0xFFFF Means "uncalibrated".  Use this value for all non-RGB formats.

    Metadata.Gamma = CMetadataRational();

    Metadata.MakerNote = CMetadataShortString( "Maker's Note..." );

    //  Just reflect the exposure time from the setting.
    //Metadata.ExposureTime =
    //    CMetadataRational( GetCurrentExposureTime(), 1000 );    // report exposure time as milliseconds.

    Metadata.FNumber = CMetadataRational(4);   // Fake an FNumber of 4. // TODO: It looks like we might be able to calculate this.

    Metadata.ExposureProgram.Set = TRUE;
    Metadata.ExposureProgram.Value = GetExposureProgram();

    Metadata.ShutterSpeedValue = CMetadataSRational();   // TODO: Calculate this from the ExposureTime.  ***
    Metadata.Aperture = CMetadataRational(4);   // TODO: Calculate this from the F-Number. (We're currently faking the FNumber.)
    Metadata.Brightness = CMetadataSRational(0);    // TODO: Find a more reasonable brightness value.
    Metadata.ExposureBias = CMetadataSRational(0);  // TODO: More reasonable?
    Metadata.SubjectDistance = CMetadataRational(0xFFFFFFFF);   // Distance in meters.  Infinity.  (Anything better?)

    Metadata.MeteringMode.Set = TRUE;
    Metadata.MeteringMode.Value = (USHORT) GetRandom( (ULONG) 1, (ULONG) 6);    // Pick a number ... any number.

    Metadata.LightSource.Set = TRUE;
    Metadata.LightSource.Value = 1;     // TODO: Pick a random value; but override when a flash occurs.

    Metadata.Flash.Set = TRUE;
    Metadata.Flash.Value = (UINT16) pSettings->FlashMode;    // We assume that the flash fires when requested!
    DBG_TRACE("FlashMode=0x%016llX, FlashPower=%d", pSettings->FlashMode, pSettings->FlashValue);

    Metadata.FocalLength = CMetadataRational(); //  TODO: Calculate?
    Metadata.FocalPlaneXResolution = CMetadataRational();   //  TODO: Calculate?
    Metadata.FocalPlaneYResolution = CMetadataRational();   //  TODO: Calculate?
    Metadata.ExposureIndex = CMetadataRational();           //  TODO: Calculate?

    Metadata.ExposureMode.Set = TRUE;
    Metadata.ExposureMode.Value = 0 ;   // Assume Auto exposure.
    if( pSettings->ExposureMode & KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_MANUAL )
    {
        Metadata.ExposureMode.Value = 0 ;   // Manual exposure.
    }

    Metadata.WhiteBalance.Set = TRUE;
    Metadata.WhiteBalance.Value = 0 ;   // Assume Auto white balance.
    if( pSettings->WhiteBalanceMode & KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_MANUAL )
    {
        Metadata.WhiteBalance.Value = 0 ;   // Manual while balance.
    }

    Metadata.DigitalZoomRatio = CMetadataRational(1);

    Metadata.FocalLengthIn35mmFilm = CMetadataShort(0);
    Metadata.SceneCaptureType = CMetadataShort(0);
    Metadata.GainControl = CMetadataRational();
    Metadata.Contrast = CMetadataShort(0);
    Metadata.Saturation = CMetadataShort(0);
    Metadata.Sharpness = CMetadataShort(0);
    Metadata.SubjectDistanceRange = CMetadataShort(0);

    //  Report (optional) focus state.
    KSCAMERA_EXTENDEDPROP_FOCUSSTATE    State = KSCAMERA_EXTENDEDPROP_FOCUSSTATE_UNINITIALIZED;
    if( NT_SUCCESS(m_Sensor->GetFocusState( &State )) )
    {
        Metadata.FocusState = CMetadataLong((UINT32)State);
    }

    return Metadata;
}

//
//  Emit metadata here for still pin.
//
void
CImageHardwareSimulation::
EmitMetadata(
    _Inout_ PKSSTREAM_HEADER    pStreamHeader
)
/*++

Routine Description:

    Emit metadata for a photo.

Arguments:

    None

Return Value:

    Success / Failure

--*/
{
    PAGED_CODE();

    NT_ASSERT(pStreamHeader);

    //  Add the normal frame info to the metadata
    CHardwareSimulation::EmitMetadata( pStreamHeader );

    if (0 != (pStreamHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_METADATA))
    {
        PKS_FRAME_INFO          pFrameInfo = (PKS_FRAME_INFO)(pStreamHeader + 1);
        PKSSTREAM_METADATA_INFO pMetadata = (PKSSTREAM_METADATA_INFO) (pFrameInfo + 1);
        PCAMERA_METADATA_IMAGEAGGREGATION   pAggregation =
            (PCAMERA_METADATA_IMAGEAGGREGATION) (((PBYTE) pMetadata->SystemVa) + pMetadata->UsedSize);
        ULONG                   BytesLeft = pMetadata->BufferSize - pMetadata->UsedSize;

        if( BytesLeft >= sizeof(*pAggregation) )
        {
            pAggregation->Header.MetadataId = (ULONG) MetadataId_Custom_ImageAggregation;
            pAggregation->Header.Size = sizeof(*pAggregation);

            //  Just copy over the current frame's ISP settings for now.
            //  We still need to develop a contract between the driver and the MFT0 for these settings.
            pAggregation->Data = GetMetadata();

            pMetadata->UsedSize += sizeof(*pAggregation);
            BytesLeft -= sizeof(*pAggregation);
        }

        CExtendedVidProcSetting FaceDetect;
        m_Sensor->GetFaceDetection(&FaceDetect);

        if( FaceDetect.Flags & KSCAMERA_EXTENDEDPROP_FACEDETECTION_PHOTO )
        {
            DBG_TRACE("IMAGE");
            EmitFaceMetadata(
                pStreamHeader,
                FaceDetect.GetULONG(),
                FaceDetect.Flags & KSCAMERA_EXTENDEDPROP_FACEDETECTION_ADVANCED_MASK,
                1);
        }
    }
}

NTSTATUS
CImageHardwareSimulation::
FillScatterGatherBuffers()

/*++

Routine Description:

    The hardware has synthesized a buffer in scratch space and we're to
    fill scatter / gather buffers.

Arguments:

    None

Return Value:

    Success / Failure

--*/

{
    PAGED_CODE();

    DBG_ENTER("() m_PinID=0x%08X, m_ImageSize=0x%08X, m_ScatterGatherMappingsQueued=%d, "
              "m_ScatterGatherBytesQueue=0x%08X",
              m_PinID, m_ImageSize, m_ScatterGatherMappingsQueued, m_ScatterGatherBytesQueued);

    //
    // We're using this list lock to protect our scatter / gather lists instead
    // of some hardware mechanism / KeSynchronizeExecution / whatever.
    //
    //KeAcquireSpinLockAtDpcLevel (&m_ListLock);

    ULONG BufferRemaining = m_ImageSize;

    //
    // If there aren't enough scatter / gather buffers queued, consider it starvation.
    //
    while(  BufferRemaining &&
            !IsListEmpty(&m_ScatterGatherMappings) &&
            m_ScatterGatherBytesQueued >= BufferRemaining)
    {
        DBG_TRACE( "BufferRemaining=0x%08X, m_ScatterGatherBytesQueued=0x%08X, m_ScatterGatherMappingsQueued=%d",
                   BufferRemaining, m_ScatterGatherBytesQueued, m_ScatterGatherMappingsQueued );

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
        ULONG BytesToCopy = min( BufferRemaining, SGEntry->ByteCount );

        //  Have the synthesizer output a frame to the buffer.
        DBG_TRACE( "DataUsed before Commit() = %d", SGEntry->CloneEntry->StreamHeader->DataUsed );
        ULONG   BytesCopied =
            m_Synthesizer->DoCommit( SGEntry->Virtual, BytesToCopy );
        NT_ASSERT(BytesCopied);
        DBG_TRACE( "BytesCopied = %d", BytesCopied );

        BufferRemaining = 0; //-= BytesCopied;

        //  Add metadata to the sample.
        EmitMetadata( SGEntry -> CloneEntry -> StreamHeader );

        ULONGLONG time = ConvertQPCtoTimeStamp(NULL);

        if (IsPhotoConfirmationNeeded())
        {
            DBG_TRACE( "PhotoConfirmation is needed.  Frame=%d, Time=0x%016llX", m_PfsFrameNumber, (LONGLONG) time );
            SGEntry->PhotoConfirmationInfo = PHOTOCONFIRMATION_INFO( m_PfsFrameNumber, (LONGLONG) time );
        }

        SGEntry -> CloneEntry -> StreamHeader -> PresentationTime.Time =  time;
        DBG_TRACE("PresentationTime = 0x%016llX", SGEntry->CloneEntry->StreamHeader->PresentationTime.Time );

        SGEntry -> CloneEntry -> StreamHeader -> OptionsFlags |= KSSTREAM_HEADER_OPTIONSF_TIMEVALID;

        DBG_TRACE("m_FlashStatus=0x%016llX", m_FlashStatus);

        if(m_FlashStatus & KSCAMERA_EXTENDEDPROP_FLASH_ON || m_FlashStatus & KSCAMERA_EXTENDEDPROP_FLASH_ON_ADJUSTABLEPOWER ||
                m_FlashStatus & KSCAMERA_EXTENDEDPROP_FLASH_AUTO || m_FlashStatus & KSCAMERA_EXTENDEDPROP_FLASH_AUTO_ADJUSTABLEPOWER)
        {
            if(m_FlashStatus & KSCAMERA_EXTENDEDPROP_FLASH_SINGLEFLASH && time >= m_TriggerTime && m_TriggerTime != 0 && !m_bFlashed)
            {
                m_bFlashed = TRUE;
                DBG_TRACE("(Single) FLASHED!!!");
            }
        }

        //
        // Release the scatter / gather entry back to our lookaside.
        //
        if( m_bTriggered && !IsPfsEOS() )
        {
            DBG_TRACE("m_PinMode=%d", m_PinMode);
            m_pClone = SGEntry->CloneEntry;
            m_PhotoConfirmationInfo = SGEntry->PhotoConfirmationInfo;
            m_NumMappingsCompleted++;
            m_ScatterGatherBytesQueued -= SGEntry -> ByteCount;

            DBG_TRACE( "m_NumMappingsCompleted=%d, m_PhotoConfirmationInfo.isRequired()=%s", m_NumMappingsCompleted, m_PhotoConfirmationInfo.isRequired()?"TRUE":"FALSE" );

            if(m_PinMode != PinBurstMode)
            {
                m_bTriggered = FALSE;
                DBG_TRACE("m_bTriggered=FALSE");
            }

            //  Update the VPS frame and loop numbers here.  Mark the frame as the EOS
            //  if we've completed the sequence.
            //
            //  Note: It's actually up to DevProxy to stop feeding us frames!
            if( AdvanceFrameCounter() )
            {
                //  We've reached the end of a VPS sequence!  Mark the frame as EOS.
                SGEntry->CloneEntry->StreamHeader->OptionsFlags |= KSSTREAM_HEADER_OPTIONSF_ENDOFPHOTOSEQUENCE;
                m_bEndOfSequence = TRUE;
            }

            ExFreeToNPagedLookasideList (
                &m_ScatterGatherLookaside,
                reinterpret_cast <PVOID> (SGEntry)
            );
        }
        else
        {
            InsertTailList( &m_ScatterGatherMappings, listEntry );
            m_ScatterGatherMappingsQueued++;
            m_pClone = NULL;
        }
    }

    DBG_LEAVE("()");

    if (BufferRemaining)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        return STATUS_SUCCESS;
    }
}

/*************************************************/

void
CImageHardwareSimulation::
FakeHardware()

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

    //  Prevent state-changes during this call.
    KScopedMutex    Lock(m_ListLock);

    m_InterruptTime++;

    //
    // The hardware can be in a pause state in which case, it issues interrupts
    // but does not complete mappings.  In this case, don't bother synthesizing
    // a frame and doing the work of looking through the mappings table.
    //
    if( m_PinState == PinRunning )
    {
        if(m_PinMode == PinBurstMode && m_bTriggered && m_bPastBufferTrigger)
        {
            CompletePastBuffers();
        }

        m_Synthesizer->DoSynthesize();

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

NTSTATUS
CImageHardwareSimulation::
CompletePastBuffers()

/*++

Routine Description:

    Find and complete any history frames.

Arguments:

    None

Return Value:

    Success / Failure

--*/

{
    PAGED_CODE();

    ULONG ulNumBuffers = m_PastBufferCount;
    BOOLEAN bContinue = TRUE;
    LIST_ENTRY *listEntry = NULL;

    DBG_ENTER("()");
    DBG_TRACE("m_PastBufferCount=%d, m_TriggerTime=0x%016llX", m_PastBufferCount, m_TriggerTime );

    //  If we're in burst mode and have ISP settings, we can't
    //  really support changing the ISP settings in the past...
    //  ... so we'll just treat them all past frames.
    //  Note: The upper layer will determine past frames from the
    //  metadata FrameId.
    if( m_PinMode == PinBurstMode &&
            !m_pIspSettings )
    {
        //  Walk through the entire list of buffers, find the most
        //  recent frame presentation time and put all past buffers
        //  into another list.
        while( !IsListEmpty(&m_ScatterGatherMappings) )
        {
            listEntry = RemoveTailList(&m_ScatterGatherMappings);
            m_ScatterGatherMappingsQueued--;

            PSCATTER_GATHER_ENTRY SGEntry =
                reinterpret_cast <PSCATTER_GATHER_ENTRY> (
                    CONTAINING_RECORD (
                        listEntry,
                        SCATTER_GATHER_ENTRY,
                        ListEntry
                    )
                );

            NT_ASSERT(SGEntry);
            NT_ASSERT(SGEntry->CloneEntry);
            NT_ASSERT(SGEntry->CloneEntry->StreamHeader);

            //  We've found one that's not stamped.  We must be at the end.
            //  Push it back and exit the loop.
            if(SGEntry->CloneEntry->StreamHeader->PresentationTime.Time == 0)
            {
                InsertTailList(&m_ScatterGatherMappings, listEntry);
                m_ScatterGatherMappingsQueued++;
                DBG_TRACE( "No past frames found." );
                bContinue = FALSE;
                break;
            }

            //  Since we're walking from the most recent to least recent frame,
            //  This one is a valid "future" frame.
            DBG_TRACE( "Adding 'future frame' to the list" );
            PushCloneList(SGEntry);

            //  If the presentation time is less than the trigger time, stop here
            //  and use this as the first triggered frame.
            if((ULONGLONG)(SGEntry->CloneEntry->StreamHeader->PresentationTime.Time) < m_TriggerTime)
            {
                DBG_TRACE( "First matching frame time=0x%016llX", SGEntry->CloneEntry->StreamHeader->PresentationTime.Time );
                break;
            }

            //  Watch out!  We might actually need to pick up multiple frames since
            //  in theory we could have generated several since the trigger time.
        }
    }

    //  Grab N past frames from the queue, if we have them.
    while( bContinue &&
            !IsListEmpty(&m_ScatterGatherMappings) &&
            ulNumBuffers)
    {
        listEntry = RemoveTailList(&m_ScatterGatherMappings);
        m_ScatterGatherMappingsQueued--;

        PSCATTER_GATHER_ENTRY SGEntry =
            reinterpret_cast <PSCATTER_GATHER_ENTRY> (
                CONTAINING_RECORD (
                    listEntry,
                    SCATTER_GATHER_ENTRY,
                    ListEntry
                )
            );

        if(!(SGEntry -> CloneEntry -> StreamHeader -> OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TIMEVALID) )
        {
            InsertTailList(&m_ScatterGatherMappings, listEntry);
            m_ScatterGatherMappingsQueued++;
            DBG_TRACE( "No more past frames found." );
            bContinue = FALSE;
        }
        else
        {
            PushCloneList(SGEntry);
            ulNumBuffers--;
            DBG_TRACE( "Past frame #%d found (%p)", ulNumBuffers, SGEntry->CloneEntry->StreamHeader );
        }
    }

    //  If we got all of the past frames we needed and didn't consume the entire queue...
    if(bContinue && (m_ScatterGatherMappingsQueued > 0))
    {
        //Mark the next tail as Time = 0 so that we don't output any super old frames.
        listEntry = RemoveTailList(&m_ScatterGatherMappings);
        m_ScatterGatherMappingsQueued--;

        PSCATTER_GATHER_ENTRY SGEntry =
            reinterpret_cast <PSCATTER_GATHER_ENTRY> (
                CONTAINING_RECORD (
                    listEntry,
                    SCATTER_GATHER_ENTRY,
                    ListEntry
                )
            );
        //  Mark the PTS as invalid.
        SGEntry->CloneEntry->StreamHeader->PresentationTime.Time = 0;
        SGEntry->CloneEntry->StreamHeader->OptionsFlags &= ~KSSTREAM_HEADER_OPTIONSF_TIMEVALID;

        InsertTailList(&m_ScatterGatherMappings, listEntry);
        m_ScatterGatherMappingsQueued++;
    }

    CompleteCloneList();

    m_bPastBufferTrigger = FALSE;

    DBG_LEAVE("()");

    return STATUS_SUCCESS;

}

//
//  Put an item onto the history list.
//
void
CImageHardwareSimulation::
PushCloneList(
    _Inout_  PSCATTER_GATHER_ENTRY SGEntry
)
{
    PAGED_CODE();

    DBG_TRACE( "Frame %p, PresentationTime=0x%016llX",
               SGEntry->CloneEntry->StreamHeader,
               SGEntry->CloneEntry->StreamHeader->PresentationTime.Time );

    InsertHeadList( &m_BurstList, &SGEntry->ListEntry );
}

//
//  Complete the history list.
//
NTSTATUS
CImageHardwareSimulation::
CompleteCloneList()
{
    PAGED_CODE();

    int i = 0;
    while(!IsListEmpty(&m_BurstList))
    {
        LIST_ENTRY *listEntry = RemoveHeadList(&m_BurstList);

        PSCATTER_GATHER_ENTRY SGEntry =
            reinterpret_cast <PSCATTER_GATHER_ENTRY> (
                CONTAINING_RECORD (
                    listEntry,
                    SCATTER_GATHER_ENTRY,
                    ListEntry
                )
            );

        m_pClone = SGEntry->CloneEntry;
        m_PhotoConfirmationInfo = SGEntry->PhotoConfirmationInfo;

        m_NumMappingsCompleted++;
        m_ScatterGatherBytesQueued -= SGEntry -> ByteCount;

        DBG_TRACE( "m_NumMappingsCompleted=%d, m_PhotoConfirmationInfo.isRequired()=%s", m_NumMappingsCompleted, m_PhotoConfirmationInfo.isRequired( )?"TRUE":"FALSE" );
        DBG_TRACE( "Frame %p, PresentationTime=0x%016llX",
                   SGEntry->CloneEntry->StreamHeader,
                   SGEntry->CloneEntry->StreamHeader->PresentationTime.Time );

        m_Sensor -> Interrupt (m_PinID);

        ExFreeToNPagedLookasideList (
            &m_ScatterGatherLookaside,
            reinterpret_cast <PVOID> (SGEntry)
        );
        i++;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
CImageHardwareSimulation::
SetClock(PKSPIN pin)
{
    PAGED_CODE();

    if(!NT_SUCCESS(KsPinGetReferenceClockInterface(pin, &m_Clock)))
    {
        m_Clock = NULL;
    }

    return STATUS_SUCCESS;
}

void
CImageHardwareSimulation::
SetTriggerTime(
    _In_    ULONGLONG TriggerTime
)
/*++

Routine Description:

    Identify exactly when the user pressed that button.

Arguments:

    TriggerTime -
        The QPC time in 100ns when the user asked for the photo.

Return Value:

    void

--*/
{
    PAGED_CODE();

    m_TriggerTime = TriggerTime;
    DBG_TRACE( "Setting Trigger Time = 0x%016llX", TriggerTime );
}

NTSTATUS
CImageHardwareSimulation::
Reset()
{
    PAGED_CODE();

    KScopedMutex Lock( m_ListLock );
    DBG_ENTER("(): m_PinID=%d", m_PinID);

    //  Parent class reset first...
    CHardwareSimulation::Reset();

    FreeSGList( &m_ScatterGatherMappings, L"StreamPointer Reset Burst List" );

    m_bTriggered = FALSE;
    m_bEndOfSequence = FALSE;
    m_pClone = NULL;
    m_bPastBufferTrigger = FALSE;
    m_TriggerTime = 0;

    DBG_LEAVE("(): m_PinID=%d", m_PinID);

    return STATUS_SUCCESS;
}

NTSTATUS
CImageHardwareSimulation::
SetFlashStatus(
    _In_    ULONGLONG ullFlashStatus
)
{
    PAGED_CODE();

    m_FlashStatus = ullFlashStatus;

    DBG_TRACE("FlashStatus=0x%016llX", ullFlashStatus);

    return STATUS_SUCCESS;
}

//
//  Program the Per Frame Settings for simulation
//  We do it before calling start so we can be ready
//  to program our simulation's hardware.
//
//  Note: We make a local copy.
//
NTSTATUS
CImageHardwareSimulation::
SetPFS(
    _In_    ISP_FRAME_SETTINGS  *pIspSettings,
    _In_    ULONG               FrameLimit,
    _In_    ULONG               LoopLimit
)
{
    PAGED_CODE();

    //  Set the current ISP settings and free any prior.
    //  This has to be done at simulation start or stop or we have
    //  a synchronization problem.
    SAFE_DELETE_ARRAY( m_pIspSettings );

    if( pIspSettings )
    {
        m_pIspSettings = new (NonPagedPoolNx) ISP_FRAME_SETTINGS[FrameLimit];
        if( !m_pIspSettings )
        {
            m_PfsFrameLimit = 0;
            m_PfsLoopLimit = 0;
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory( m_pIspSettings, pIspSettings, FrameLimit*sizeof(ISP_FRAME_SETTINGS) );
        m_PfsLoopLimit  = LoopLimit;
        m_PfsFrameLimit = FrameLimit;
    }
    else
    {
        m_PfsFrameLimit = 0;
        m_PfsLoopLimit = 0;
    }

    return STATUS_SUCCESS;
}

//  Function:
//      bool CImageHardwareSimulation::AdvanceFrameCounter(void)
//
//  Description:
//      Advance our frame and loop pointers to the next PFS settings.
//
//  Parameters:
//      [None]
//
//  Returns:
//      bool - true if we've reached the end of our Per Frame Settings.
//
bool
CImageHardwareSimulation::
AdvanceFrameCounter(void)
{
    PAGED_CODE();

    bool    bEOS = false;

    DBG_ENTER( "()" );

    m_GlobalFrameNumber ++;
    DBG_TRACE( "m_GlobalFrameNumber=%lld", m_GlobalFrameNumber );

    //
    //  Calculate the PFS frame & loop numbers, but only if we've
    //  gotten ISP settings.
    //
    if( m_bTriggered &&
            m_PinMode == PinBurstMode &&
            m_pIspSettings )
    {
        m_PfsFrameNumber ++;

        if( m_PfsFrameNumber >= m_PfsFrameLimit )
        {
            m_PfsFrameNumber = 0;
            m_PfsLoopNumber ++;
        }

        //  Only mark EOS if we're not in an infinite loop.
        if( m_PfsLoopLimit != 0 )
        {
            NT_ASSERT( !(m_PfsLoopNumber > m_PfsLoopLimit) );
            //  Check to see if we've hit our limit.
            if( m_PfsLoopNumber >= m_PfsLoopLimit )
            {
                DBG_TRACE( "Marking EOS" );
                bEOS = true;
            }
        }
    }

    DBG_TRACE( "m_PfsFrameNumber=%d, m_PfsLoopNumber=%d", m_PfsFrameNumber, m_PfsLoopNumber );
    DBG_TRACE( "m_PfsFrameLimit=%d,  m_PfsLoopLimit=%d",  m_PfsFrameLimit, m_PfsLoopLimit );
    DBG_LEAVE( "() = %s", bEOS ? "true" : "false" );
    return bEOS;
}


//  Function:
//      bool CImageHardwareSimulation::IsPfsEOS(void)
//
//  Description:
//      Determine if we're at the EOS.
//
//  Parameters:
//      [None]
//
//  Returns:
//      bool - true if we've reached the end of our Per Frame Settings.
//
bool
CImageHardwareSimulation::
IsPfsEOS(void)
{
    PAGED_CODE();

    DBG_ENTER( "()" );
    DBG_TRACE( "m_bTriggered=%s, m_bEndOfSequence=%s, m_PinMode=%d, m_pIspSetting=0x%p",
               ( m_bTriggered ? "true" : "false" ),
               ( m_bEndOfSequence ? "true" : "false" ),
               m_PinMode,
               m_pIspSettings );

    //
    //  Make sure we're in a Variable Photo Sequence.
    //
    bool result =
        bool( m_bTriggered == TRUE &&
              m_bEndOfSequence &&
              m_PinMode == PinBurstMode &&
              m_pIspSettings );       // Must have ISP settings set to be PFS EOS.

    DBG_LEAVE( "() = %s", (result ? "true" : "false") );
    return result;
}

//  Function:
//      BOOL CImageHardwareSimulation::IsPfsActive(void)
//
//  Description:
//      Determine if we're in a Variable Photo Sequence.
//
//  Parameters:
//      [None]
//
//  Returns:
//      BOOL - TRUE if we are actively processing Per Frame Settings.
//
BOOL
CImageHardwareSimulation::
IsPfsActive(void)
{
    PAGED_CODE();

    return
        BOOL( m_bTriggered && !m_bEndOfSequence &&
              m_PinMode == PinBurstMode &&
              m_pIspSettings ) ;
}

//  Get the current frame settings.
//
//  Note:
//      Call this function to acquire ISP settings for the current frame's
//      simulation.  Initially we'll just use it to report back the ISP
//      settings originally requested in the PFS by the user.
//
ISP_FRAME_SETTINGS *
CImageHardwareSimulation::
GetIspSettings(void)
{
    PAGED_CODE();

    return
        IsPfsActive()
        ? &m_pIspSettings[m_PfsFrameNumber]
        : CHardwareSimulation::GetIspSettings() ;
}

BOOLEAN
CImageHardwareSimulation::
IsPhotoConfirmationNeeded()
/*++

Routine Description:

    Check flags for photo confirmation and return
    whether driver should issue confirmation.

--*/
{
    PAGED_CODE();

    ISP_FRAME_SETTINGS *pSettings = GetIspSettings();

    return pSettings ? pSettings->bPhotoConfirmation : FALSE;
}
