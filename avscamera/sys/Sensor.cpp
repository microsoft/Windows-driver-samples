/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2014, Microsoft Corporation.

    File:

        Sensor.cpp

    Abstract:

        Base Sensor class implementation.

        This class also controls access to the pin simulations.  Most cameras
        have a limited set of ISP resources and can only instantiate a fixed
        number of pins.  This class grants access to those resources.

    History:

        created 5/5/2014

**************************************************************************/

#include "Common.h"

/**************************************************************************

    PAGEABLE CODE

**************************************************************************/

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA


CSensor::CSensor(
    _In_    CCaptureDevice *Device,
    _In_    ULONG           PinCount
)
    : m_Device(Device)
    , m_PinCount(PinCount)
    , m_HardwareSimulation(nullptr)
    , m_Synthesizer(nullptr)
    , m_CapturePin(nullptr)
    , m_VideoInfoHeader(nullptr)
    , m_InterruptTime(nullptr)
    , m_LastMappingsCompleted(nullptr)
    , m_PreviewIndex(INVALID_PIN_INDEX)
    , m_StillIndex(INVALID_PIN_INDEX)
    , m_VideoIndex(INVALID_PIN_INDEX)
    , m_FilterInstanceCount(0)
    , m_pIspSettings(nullptr)
    , m_PfsFrameLimit(0)
    , m_PfsLoopLimit(0)
{
    PAGED_CODE();
    NT_ASSERT(Device);
    NT_ASSERT(PinCount);
}


CSensor::~CSensor()
{
    PAGED_CODE();
    for(ULONG i=0; i < m_PinCount; i++)
    {
        if( m_HardwareSimulation)
        {
            SAFE_DELETE( m_HardwareSimulation[i] );
        }
        if( m_Synthesizer )
        {
            SAFE_DELETE( m_Synthesizer[i] );
        }
        if( m_CapturePin )
        {
            m_CapturePin[i] = nullptr;
        }
        if( m_VideoInfoHeader )
        {
            SAFE_DELETE( m_VideoInfoHeader[i] );
        }
    }

    SAFE_DELETE_ARRAY( m_pIspSettings         );
    SAFE_DELETE_ARRAY( m_HardwareSimulation   );
    SAFE_DELETE_ARRAY( m_Synthesizer          );
    SAFE_DELETE_ARRAY( m_CapturePin           );
    SAFE_DELETE_ARRAY( m_VideoInfoHeader      );
    SAFE_DELETE_ARRAY( m_InterruptTime        );
    SAFE_DELETE_ARRAY( m_LastMappingsCompleted);
}


NTSTATUS
CSensor::
AddFilter(PKSFILTER pFilter)
{
    PAGED_CODE();

    //  If your sensor object needs to keep track of the Filters that are
    //  attached to it, you would do that here.  We only want to keep an
    //  outstanding filter count so we know when to reset our defaults.
    UNREFERENCED_PARAMETER(pFilter);

    LONG        Count = IncrementFilterCount();
    NTSTATUS    Status= STATUS_SUCCESS;

    //  If this is the first filter, reprogram the sensor.
    if( Count==1 )
    {
        Status = ProgramDefaults();
        if( !NT_SUCCESS(Status) )
        {
            Count = DecrementFilterCount();
        }
    }
    DBG_LEAVE("(Count=%d)=0x%08X", Count, Status);
    return Status;
}

NTSTATUS
CSensor::
RemoveFilter(PKSFILTER pFilter)
{
    PAGED_CODE();

    //  If your sensor object needs to keep track of the Filters that are
    //  attached to it, you would clean up here.  We only want to keep an
    //  outstanding filter count so we know when to reset our defaults.
    UNREFERENCED_PARAMETER(pFilter);

    LONG    Count = DecrementFilterCount();
    NT_ASSERT( Count>=0 );

    NTSTATUS Status = Count>=0 ? STATUS_SUCCESS : STATUS_INVALID_DEVICE_STATE;

    DBG_LEAVE("(Count=%d)=0x%08X", Count, Status);
    return Status;
}

NTSTATUS
CSensor::
ProgramDefaults()
{
    PAGED_CODE();
    return STATUS_SUCCESS;
}

NTSTATUS
CSensor::
Initialize()
{
    PAGED_CODE();

    m_HardwareSimulation = new (NonPagedPoolNx, 'sneS') CHardwareSimulation *[m_PinCount];
    m_Synthesizer = new (NonPagedPoolNx, 'sneS') CSynthesizer *[m_PinCount];
    m_CapturePin = new (NonPagedPoolNx, 'sneS') ICapturePin *[m_PinCount];
    m_VideoInfoHeader = new (NonPagedPoolNx, 'sneS') PKS_VIDEOINFOHEADER[m_PinCount];
    m_InterruptTime = new (NonPagedPoolNx, 'sneS') ULONG[m_PinCount];
    m_LastMappingsCompleted = new (NonPagedPoolNx, 'sneS') ULONG[m_PinCount];

    if( !m_HardwareSimulation   ||
            !m_Synthesizer          ||
            !m_CapturePin           ||
            !m_VideoInfoHeader      ||
            !m_InterruptTime        ||
            !m_LastMappingsCompleted )
    {
        SAFE_DELETE_ARRAY( m_HardwareSimulation   );
        SAFE_DELETE_ARRAY( m_Synthesizer          );
        SAFE_DELETE_ARRAY( m_CapturePin           );
        SAFE_DELETE_ARRAY( m_VideoInfoHeader      );
        SAFE_DELETE_ARRAY( m_InterruptTime        );
        SAFE_DELETE_ARRAY( m_LastMappingsCompleted);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    for(ULONG i=0; i<m_PinCount; i++)
    {
        m_HardwareSimulation[i] = nullptr;
        m_Synthesizer[i] = nullptr;
        m_CapturePin[i] = nullptr;
        m_VideoInfoHeader[i] = nullptr;
        m_InterruptTime[i] = 0;
        m_LastMappingsCompleted[i] = 0;
    }
    return STATUS_SUCCESS;
}

void
CSensor::
Interrupt(
    _In_    LONG PinIndex
)

/*++

Routine Description:

    This is the "faked" interrupt service routine for this device.  It
    is called at dispatch level by the hardware simulation.

Arguments:

    PinIndex -

Return Value:

    None

--*/
{
    PAGED_CODE();

    if( PinIndex<((LONG)m_PinCount) && PinIndex>=0 )
    {
        DBG_ENTER("(PinIndex=%d): m_LastMappingsCompleted[%d]=%d", PinIndex,
                  PinIndex, m_LastMappingsCompleted[PinIndex]);

        m_InterruptTime[PinIndex]++;

        //
        // Realistically, we'd do some hardware manipulation here and then queue
        // a DPC.  Since this is fake hardware, we do what's necessary here.  This
        // is pretty much what the DPC would look like short of the access
        // of hardware registers (ReadNumberOfMappingsCompleted) which would likely
        // be done in the ISR.
        //
        ULONG LastMappingCompleted = m_LastMappingsCompleted[PinIndex];
        if( PinIndex != (LONG) m_StillIndex )
        {
            while( LastMappingCompleted < 
                   m_HardwareSimulation[PinIndex]->ReadNumberOfMappingsCompleted() )
            {
                //  Complete a frame.
                if( !NT_SUCCESS(m_CapturePin[PinIndex]->CompleteMapping()) )
                {
                    break;
                }
                LastMappingCompleted++;
            }
        }
        else
        {
            CImageHardwareSimulation   *pImageHw = (CImageHardwareSimulation *) m_HardwareSimulation[PinIndex];
            PKSSTREAM_POINTER           pClone = pImageHw->m_pClone;

            //  Just make sure the image pin actually generate a frame.
            if( pClone )
            {
                //  Emit a photo confirmation image (we don't send off a copy of this image),
                //  but only if we need one for this photo and if the timestamp is valid.
                if( pImageHw->m_PhotoConfirmationInfo.isRequired() &&
                    (pClone->StreamHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TIMEVALID) &&
                    m_PreviewIndex < m_PinCount )
                {
                    m_HardwareSimulation[m_PreviewIndex]->
                        GeneratePhotoConfirmation(
                            pImageHw->m_PhotoConfirmationInfo.getIndex(),
                            pImageHw->m_PhotoConfirmationInfo.getTime() 
                        );
                }

                //
                // Inform the capture pin that a given number of scatter / gather
                // mappings have completed.
                //
                if( NT_SUCCESS(m_CapturePin[PinIndex]->CompleteMapping(pClone)) )
                {
                    LastMappingCompleted++;
                }

                pImageHw->m_pClone = NULL;
            }
            pImageHw->m_PhotoConfirmationInfo = PHOTOCONFIRMATION_INFO();
        }

        m_LastMappingsCompleted[PinIndex] = LastMappingCompleted;
        DBG_LEAVE("(PinIndex=%d): LastMappingCompleted=%d", PinIndex, LastMappingCompleted);
    }
}

NTSTATUS
CSensor::
AcquireHardwareResources(
    _In_    PKSPIN Pin,
    _In_    ICapturePin *CapturePin,
    _In_    PKS_VIDEOINFOHEADER VideoInfoHeader,
    _Out_   CHardwareSimulation **pSim
)

/*++

Routine Description:

    Acquire hardware resources for the capture hardware.  If the
    resources are already acquired, this will return an error.
    The hardware configuration must be passed as a VideoInfoHeader.

Arguments:

    Pin -
        The pin to acquire.

    CapturePin -
        The capture pin attempting to acquire resources.  When scatter /
        gather mappings are completed, the capture pin specified here is
        what is notified of the completions.

    VideoInfoHeader -
        Information about the capture stream.  This **MUST** remain
        stable until the caller releases hardware resources.  Note
        that this could also be guaranteed by bagging it in the device
        object bag as well.

    pSim -
        A location to store a pointer to the simulation object for this pin.

Return Value:

    Success / Failure

--*/

{
    PAGED_CODE();

    DBG_ENTER( "(Pin=%d, CapturePin=%p, VideoInfoHeader=%p)", Pin->Id, CapturePin, VideoInfoHeader );

    NTSTATUS Status = STATUS_SUCCESS;
    LONG lPindex = Pin->Id;
    //
    // If we're the first pin to go into acquire (remember we can have
    // a filter in another graph going simultaneously), grab the resources for Preview
    //
    if( InterlockedCompareExchangePointer(
                (PVOID *) &m_CapturePin[lPindex],
                CapturePin,
                nullptr) == nullptr)
    {
        m_VideoInfoHeader[lPindex] = VideoInfoHeader;

        LONG    Width = VideoInfoHeader->bmiHeader.biWidth;
        LONG    Height= VideoInfoHeader->bmiHeader.biHeight;

        //
        // If there's an old hardware simulation sitting around for some
        // reason, blow it away.
        //
        SAFE_DELETE( m_Synthesizer[lPindex] );

        DBG_TRACE( "biBitCount   =%d",      VideoInfoHeader->bmiHeader.biBitCount );
        DBG_TRACE( "biWidth      =%d",      VideoInfoHeader->bmiHeader.biWidth );
        DBG_TRACE( "biHeight     =%d",      VideoInfoHeader->bmiHeader.biHeight );
        DBG_TRACE( "biCompression=0x%08X",  VideoInfoHeader->bmiHeader.biCompression );
        DBG_TRACE( "biCompression='%04s'", (PSTR) &VideoInfoHeader->bmiHeader.biCompression );
        DBG_TRACE( "AvgTimePerFrame=%lld",  VideoInfoHeader->AvgTimePerFrame );

        //
        // Create the necessary type of image synthesizer.
        //
        if (VideoInfoHeader->bmiHeader.biBitCount == 24 &&
                VideoInfoHeader->bmiHeader.biCompression == KS_BI_RGB)
        {
            //
            // If we're RGB24, create a new RGB24 synth.  RGB24 surfaces
            // can be in either orientation.  The origin is lower left if
            // height < 0.  Otherwise, it's upper left.
            //
            m_Synthesizer[lPindex] = new (NonPagedPoolNx, 'RysI')
            CRGB24Synthesizer ( Width, Height );
            DBG_TRACE( "Creating CRGB24Synthesizer..." );
        }
        else if (VideoInfoHeader->bmiHeader.biBitCount == 32 &&
                 VideoInfoHeader->bmiHeader.biCompression == KS_BI_RGB)
        {
            //
            // If we're RGB32, create a new RGB32 synth.  RGB32 surfaces
            // can be in either orientation.  The origin is lower left if
            // height < 0.  Otherwise, it's upper left.
            //
            m_Synthesizer[lPindex] = new (NonPagedPoolNx, '23RI')
            CXRGBSynthesizer ( Width, Height );
            DBG_TRACE( "Creating CXRGBSynthesizer..." );
        }
        else if (VideoInfoHeader->bmiHeader.biBitCount == 16 &&
                 (VideoInfoHeader->bmiHeader.biCompression == FOURCC_YUY2))
        {
            //
            // If we're YUY2, create the YUY2 synth.
            //
            m_Synthesizer[lPindex] = new(NonPagedPoolNx, 'YysI') CYUY2Synthesizer(Width, Height);
            DBG_TRACE( "Creating CYUY2Synthesizer..." );

        }
        else if (VideoInfoHeader->bmiHeader.biBitCount == 12 &&
                 (VideoInfoHeader->bmiHeader.biCompression == FOURCC_NV12))
        {

            m_Synthesizer[lPindex] = new(NonPagedPoolNx, 'Nv12') CNV12Synthesizer(Width, Height);
            DBG_TRACE( "Creating CNV12Synthesizer..." );

        }
        else
        {
            Status = STATUS_INVALID_PARAMETER;
        }

        if (NT_SUCCESS(Status) && !m_Synthesizer[lPindex])
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }

        if (NT_SUCCESS (Status))
        {
            //
            // If everything has succeeded thus far, set the capture pin.
            //
            m_CapturePin[lPindex] = CapturePin;
            *pSim = m_HardwareSimulation[lPindex];

        }
        else
        {
            //
            // If anything failed in here, we release the resources we've
            // acquired.
            //
            ReleaseHardwareResources(Pin);
        }
    }
    else
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    DBG_LEAVE( "(Pin=%d, CapturePin=%p, VideoInfoHeader=%p) = 0x%08X", Pin->Id, CapturePin, VideoInfoHeader, Status );

    return Status;

}

/*************************************************/


NTSTATUS
CSensor::
Start (
    _In_    PKSPIN Pin
)

/*++

Routine Description:

    Start the capture device based on the video info header we were told
    about when resources were acquired.

Arguments:

    Pin -
        The pin to start

Return Value:

    Success / Failure

--*/

{
    PAGED_CODE();

    DBG_ENTER( "( Pin=%d )\n", Pin->Id ) ;

    LONG lPindex = Pin->Id;

    m_LastMappingsCompleted[lPindex] = 0;
    m_InterruptTime[lPindex] = 0;

    NT_ASSERT( m_VideoInfoHeader[lPindex] != nullptr );
    if( !m_VideoInfoHeader[lPindex] )
    {
        return STATUS_INVALID_DEVICE_STATE;
    }

    //  Ideally we'd do this when the filter is constructed; but the
    //  simulation is constructed first and these values aren't used until
    //  we call start.

    if(lPindex != (LONG) m_StillIndex)
    {
        return
            m_HardwareSimulation[lPindex] -> Start (
                m_Synthesizer[lPindex],
                m_VideoInfoHeader[lPindex] -> AvgTimePerFrame,
                m_VideoInfoHeader[lPindex] -> bmiHeader.biWidth,
                ABS (m_VideoInfoHeader[lPindex] -> bmiHeader.biHeight),
                m_VideoInfoHeader[lPindex] -> bmiHeader.biSizeImage
            );
    }
    else
    {
        CImageHardwareSimulation   *pHwSim = (CImageHardwareSimulation *) m_HardwareSimulation[lPindex] ;

        //  The following is necessary if Per-Frame Settings are active.
        if( IsVPSActive() )
        {
            //  Program the simulation with our settings
            pHwSim->SetPFS( m_pIspSettings, m_PfsFrameLimit, m_PfsLoopLimit );
        }
        else
        {
            //  Clear out any previous settings, if they still exist.
            pHwSim->SetPFS(nullptr, 0, 0);
        }

        //  Init to the default frame rate.
        LONGLONG        TimePerFrame = m_VideoInfoHeader[lPindex]->AvgTimePerFrame;
        DBG_TRACE("Image Pin's AvgTimePerFrame=%lld", TimePerFrame);

        //  Query our control for the max frame rate and convert it into a "PERFORMACE TIME".
        CExtendedProperty   MaxFrameRate;
        GetPhotoMaxFrameRate( &MaxFrameRate );
        LARGE_INTEGER   PerformanceTime = { MaxFrameRate.m_Value.Value.ratio.LowPart };
        LONGLONG        Frequency = MaxFrameRate.m_Value.Value.ratio.HighPart;

        //  Handle an user-specified frame-rate override.
        if( Frequency != 0 )
        {
            DBG_TRACE( "MaxFrameRate = %lld/%lld", PerformanceTime.QuadPart, Frequency );

            SetPhotoFrameRate(
                KSCONVERT_PERFORMANCE_TIME( Frequency, PerformanceTime )
            );
        }

        CExtendedPhotoMode  Mode;
        GetPhotoMode( &Mode );

        NTSTATUS status =  pHwSim -> Start (
                               m_Synthesizer[lPindex],
                               m_VideoInfoHeader[lPindex] -> bmiHeader.biWidth,
                               ABS (m_VideoInfoHeader[lPindex] -> bmiHeader.biHeight),
                               m_VideoInfoHeader[lPindex] -> bmiHeader.biSizeImage,
                               (Mode.Flags == 0 ? PinNormalMode : PinBurstMode)
                           );

        if(NT_SUCCESS(status))
        {
            status = pHwSim -> SetClock(Pin);
        }

        DBG_LEAVE( "( Pin=%d )=0x%08X\n", Pin->Id, status );

        return status;
    }
}

/*************************************************/


NTSTATUS
CSensor::
Pause (
    _In_    PKSPIN Pin,
    _In_    BOOLEAN Pausing
)

/*++

Routine Description:

    Pause or unpause the hardware simulation.  This is an effective start
    or stop without resetting counters and formats.  Note that this can
    only be called to transition from started -> paused -> started.  Calling
    this without starting the hardware with Start() does nothing.

Arguments:

    Pin -
        The pin to pause.

    Pausing -
        An indicatation of whether we are pausing or unpausing

        TRUE -
            Pause the hardware simulation

        FALSE -
            Unpause the hardware simulation

Return Value:

    Success / Failure

--*/

{

    PAGED_CODE();

    return
        m_HardwareSimulation[Pin->Id]->Pause(Pausing);
}

/*************************************************/


NTSTATUS
CSensor::
Stop (
    _In_    PKSPIN Pin
)

/*++

Routine Description:

    Stop the capture device.

Arguments:

    None

Return Value:

    Success / Failure

--*/

{

    PAGED_CODE();

    LONG lPindex = Pin->Id;
    CHardwareSimulation *pHwSim = m_HardwareSimulation[lPindex];

    if( !pHwSim )
    {
        return STATUS_INVALID_DEVICE_STATE;
    }

    NTSTATUS    Status = pHwSim->Stop();

    if( lPindex == (LONG) m_StillIndex )
    {
        //  Clear out any previous Per Frame Settings, if they still exist.
        ((CImageHardwareSimulation *)pHwSim)->SetPFS(nullptr, 0, 0) ;
    }

    return Status;
}

NTSTATUS
CSensor::
Reset(
    _In_    PKSPIN Pin
)
{
    PAGED_CODE();

    if( m_StillIndex > m_PinCount           ||
            !m_HardwareSimulation[m_StillIndex] )
    {
        return STATUS_INVALID_DEVICE_STATE;
    }

    if( Pin->Id == m_StillIndex )
    {
        return Reset();
    }
    return STATUS_SUCCESS;
}

//Resets both filter's first PIN
NTSTATUS
CSensor::
Reset(
)
{
    PAGED_CODE();

    if( m_StillIndex > m_PinCount           ||
            !m_HardwareSimulation[m_StillIndex] )
    {
        return STATUS_INVALID_DEVICE_STATE;
    }

    CHardwareSimulation *pHwSim = m_HardwareSimulation[m_StillIndex];

    pHwSim->Reset();
    m_LastMappingsCompleted[m_StillIndex] = pHwSim->ReadNumberOfMappingsCompleted();

    for( ULONG i=0; i<m_PinCount; i++ )
    {
        DBG_TRACE("m_LastMappingsCompleted[%d] = %d", i, m_LastMappingsCompleted[i] );
    }
    return STATUS_SUCCESS;
}


NTSTATUS
CSensor::
Trigger (
    _In_    ULONG   PinId,
    _In_    LONG    mode
)
/*++

Routine Description:

    For a photo pin, take a picture or begin or end a photo sequence.

Arguments:

    PinId -
        This must be a photo pin.

    mode -
        The trigger mode, ie: normal or start/stop sequence.

Return Value:

    Success / Failure

--*/
{

    PAGED_CODE();

    if( PinId > m_PinCount              ||
        !m_HardwareSimulation[PinId]    ||
        m_HardwareSimulation[PinId]->GetState() != PinRunning )
    {
        return STATUS_INVALID_DEVICE_STATE;
    }

    return
        ((CImageHardwareSimulation *)m_HardwareSimulation[PinId])->Trigger(mode);

}

NTSTATUS
CSensor::
SetPhotoFrameRate(
    _In_    ULONGLONG   TimePerFrame
)
/*++

Routine Description:

    For a photo pin, set the desired frame rate.

Arguments:

    TimePerFrame -
        Time in 100ns between each frame.

Return Value:

    Success / Failure

--*/
{
    PAGED_CODE();

    if( m_StillIndex > m_PinCount           ||
        !m_VideoInfoHeader[m_StillIndex]    ||
        !m_HardwareSimulation[m_StillIndex] )
    {
        return STATUS_INVALID_DEVICE_STATE;
    }

    ULONGLONG   AvgTimePerFrame =
        (ULONGLONG) m_VideoInfoHeader[m_StillIndex]->AvgTimePerFrame;

    DBG_TRACE( "TimePerFrame=%lld, AvgTimePerFrame=%lld", TimePerFrame, AvgTimePerFrame );

    //  Handle a reset back to the standard frame rate.
    if( TimePerFrame == 0 )
    {
        TimePerFrame = AvgTimePerFrame;
    }
    //  Handle an user-specified frame-rate override.
    else
    {
        //  It is unrealistic to allow a frame rate faster than the rate specified for this format.
        TimePerFrame = max( TimePerFrame, AvgTimePerFrame );
    }

    DBG_TRACE( "Setting new TimePerFrame=%lld", TimePerFrame );

    return
        ((CImageHardwareSimulation *) m_HardwareSimulation[m_StillIndex])->SetPhotoFrameRate( TimePerFrame );
}

ULONGLONG
CSensor::
GetPhotoFrameRate()
/*++

Routine Description:

    Get the desired frame rate for a photo pin.

Arguments:

    None

Return Value:

    Time in 100ns between each frame.

--*/
{
    PAGED_CODE();

    if( m_StillIndex > m_PinCount           ||
            !m_HardwareSimulation[m_StillIndex] )
    {
        NT_ASSERT(FALSE);
        return 0;
    }

    CImageHardwareSimulation *pSim = (CImageHardwareSimulation *) (m_HardwareSimulation[m_StillIndex]);
    return pSim->GetPhotoFrameRate( );
}

NTSTATUS
CSensor::
SetFlashStatus(
    _In_    ULONGLONG ulFlashStatus
)
{
    PAGED_CODE();

    if( m_StillIndex > m_PinCount           ||
            !m_HardwareSimulation[m_StillIndex] )
    {
        NT_ASSERT(FALSE);
        return 0;
    }

    return
        ((CImageHardwareSimulation *) m_HardwareSimulation[m_StillIndex])->SetFlashStatus (ulFlashStatus);
}

/*************************************************/


NTSTATUS
CSensor::
SetPinMode(
    _In_    ULONGLONG Flags,
    _In_    ULONG PastBuffers
)
{
    PAGED_CODE();

    if( m_StillIndex > m_PinCount           ||
            !m_HardwareSimulation[m_StillIndex] )
    {
        return STATUS_INVALID_DEVICE_STATE;
    }

    return
        ((CImageHardwareSimulation *) m_HardwareSimulation[m_StillIndex])->
        SetMode( Flags, PastBuffers );
}

NTSTATUS
CSensor::
GetQPC(
    _Out_   PULONGLONG TriggerTime
)
{
    PAGED_CODE();

    if( m_StillIndex > m_PinCount           ||
            !m_HardwareSimulation[m_StillIndex] )
    {
        return STATUS_INVALID_DEVICE_STATE;
    }

    *TriggerTime =
        ((CImageHardwareSimulation *) m_HardwareSimulation[m_StillIndex])->
        GetTriggerTime();

    return STATUS_SUCCESS;
}

NTSTATUS
CSensor::
SetQPC(
    _In_    ULONGLONG TriggerTime
)
{
    PAGED_CODE();

    if( m_StillIndex > m_PinCount           ||
            !m_HardwareSimulation[m_StillIndex] )
    {
        return STATUS_INVALID_DEVICE_STATE;
    }

    ((CImageHardwareSimulation *) m_HardwareSimulation[m_StillIndex])->
    SetTriggerTime( TriggerTime );

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
CSensor::
SetPFS(
    _In_    ISP_FRAME_SETTINGS  *pIspSettings,
    _In_    ULONG               FrameLimit,
    _In_    ULONG               LoopLimit
)
/*++

Routine Description:

    Program the Per Frame Settings for simulation
    We do it before calling start so we can be ready
    to program our simulation's hardware.

    Note: We make a local copy.

Arguments:

    pIspSettings -
        The Per-Frame Settings to use.

    FrameLimit -
        The number of frames to capture.

    LoopLimit -
        The number of times to loop over the sequence.

Return Value:

    Success / Failure.

--*/
{
    PAGED_CODE();
    KScopedMutex    Lock(m_SensorMutex);

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

/*************************************************/


void
CSensor::
ReleaseHardwareResources (
    _In_ PKSPIN Pin
)

/*++

Routine Description:

    Release hardware resources.  This should only be called by
    an object which has acquired them.

Arguments:

    Pin -
        The pin to acquire.

Return Value:

    None

--*/

{

    PAGED_CODE();

    DBG_ENTER( "( Pin=%d )\n", Pin->Id ) ;

    LONG lPindex = Pin->Id;
    //
    // Blow away the image synth.
    //
    m_HardwareSimulation[lPindex]->Reset();

    SAFE_DELETE( m_Synthesizer[lPindex] );
    m_VideoInfoHeader[lPindex] = NULL;

    //
    // Release our "lock" on hardware resources.  This will allow another
    // pin (perhaps in another graph) to acquire them.
    //
    InterlockedExchangePointer(
        (PVOID *) &(m_CapturePin[lPindex]),
        nullptr
    );

    DBG_LEAVE( "( Pin=%d )\n", Pin->Id ) ;
}

ULONG
CSensor::
ProgramScatterGatherMappings (
    _In_    PKSPIN Pin,
    _In_    PKSSTREAM_POINTER *Clone,
    _In_    PUCHAR *Buffer,
    _In_    PKSMAPPING Mappings,
    _In_    ULONG MappingsCount
)

/*++

Routine Description:

    Program the scatter / gather mappings for the "fake" hardware.

    Punts the request to the H/W simulation (CHardwareSimulation) object for 
    that pin.

Arguments:

    Pin -
        The pin to program.

    Clone -
        A stream pointer.

    Buffer -
        Points to a pointer to the virtual address of the topmost
        scatter / gather chunk.  The pointer will be updated as the
        device "programs" mappings.  Reason for this is that we get
        the physical addresses and sizes, but must calculate the virtual
        addresses...  This is used as scratch space for that.

    Mappings -
        An array of mappings to program

    MappingsCount -
        The count of mappings in the array

Return Value:

    The number of mappings successfully programmed

--*/

{
    PAGED_CODE();

    return
        m_HardwareSimulation[Pin->Id]->ProgramScatterGatherMappings (
            Clone,
            Buffer,
            Mappings,
            MappingsCount,
            sizeof (KSMAPPING)
        );
}

//  Expose a pointer to the global ISP_FRAME_SETTINGS
ISP_FRAME_SETTINGS *
CSensor::
GetGlobalIspSettings()
{
    PAGED_CODE();
    return &m_GlobalIspSettings;
}

void
CSensor::
UpdateZoom(void)
{
    PAGED_CODE();
}

//
//  This "Null" definition fails the request as if it doesn't exist in the
//  automation table.  This is the default behavior for most properties in
//  CSensor.
//
#define DEFINE_NULL_PROPERTY_FUNC( _sal_, type, func )      \
NTSTATUS                                                    \
func(                                                       \
    _sal_ type    *Value                                    \
    )                                                       \
{                                                           \
    PAGED_CODE();                                           \
    UNREFERENCED_PARAMETER(Value);                          \
    return STATUS_NOT_FOUND;                                \
}

#define DEFINE_NULL_SIZEOF_FUNC( func )                     \
ULONG                                                       \
func()                                                      \
{                                                           \
    PAGED_CODE();                                           \
    return 0;                                               \
}

#define DEFINE_NULL_PROPERTY_GET( T, type, name )           \
    DEFINE_NULL_PROPERTY_FUNC( _Inout_, type, T::Get##name )

#define DEFINE_NULL_PROPERTY_SET( T, type, name )           \
    DEFINE_NULL_PROPERTY_FUNC( _In_, type, T::Set##name )

#define DEFINE_NULL_PROPERTY( T, type, name )               \
    DEFINE_NULL_PROPERTY_GET( T, type, name )               \
    DEFINE_NULL_PROPERTY_SET( T, type, name )

#define DEFINE_NULL_PROPERTY_ASYNC( T, type, name )         \
    DEFINE_NULL_PROPERTY_GET( T, type, name )               \
                                                            \
NTSTATUS                                                    \
T::                                                         \
Set##name##Async(                                           \
    _In_    type        *pProperty,                         \
    _In_    CNotifier   *Notifier                           \
    )                                                       \
{                                                           \
    PAGED_CODE();                                           \
    UNREFERENCED_PARAMETER(pProperty);                      \
    UNREFERENCED_PARAMETER(Notifier);                       \
    return STATUS_NOT_FOUND;                                \
}                                                           \
                                                            \
NTSTATUS                                                    \
T::                                                         \
Cancel##name()                                              \
{                                                           \
    PAGED_CODE();                                           \
    return STATUS_UNSUCCESSFUL;                             \
}

#define DEFINE_NULL_PROPERTY_VARSIZE_ASYNC( T, type, name ) \
    DEFINE_NULL_PROPERTY_ASYNC( T, type, name )             \
                                                            \
ULONG                                                       \
T::                                                         \
SizeOf##name()                                              \
{                                                           \
    PAGED_CODE();                                           \
    return 0;                                               \
}

//
//  Define a bunch of "not implemented" function stubs.
//
DEFINE_NULL_PROPERTY_ASYNC(CSensor, KSPROPERTY_CAMERACONTROL_REGION_OF_INTEREST_S, FocusRect)
DEFINE_NULL_PROPERTY_ASYNC(CSensor, CExtendedPhotoMode, PhotoMode)

DEFINE_NULL_PROPERTY(CSensor, KSPROPERTY_VIDEOCONTROL_MODE_S, VideoControlMode)
DEFINE_NULL_PROPERTY(CSensor, KSPROPERTY_CAMERACONTROL_FLASH_S, Flash)
DEFINE_NULL_PROPERTY_GET(CSensor, KSPROPERTY_CAMERACONTROL_IMAGE_PIN_CAPABILITY_S, PinDependence)

DEFINE_NULL_PROPERTY(CSensor, CExtendedProperty, TriggerTime)
DEFINE_NULL_PROPERTY(CSensor, CExtendedProperty, TorchMode)
DEFINE_NULL_PROPERTY(CSensor, CExtendedProperty, ExtendedFlash)

DEFINE_NULL_PROPERTY_GET(CSensor, CExtendedProperty, PhotoFrameRate)
DEFINE_NULL_PROPERTY_ASYNC(CSensor, CExtendedProperty, PhotoMaxFrameRate)
DEFINE_NULL_PROPERTY_ASYNC(CSensor, CExtendedProperty, WarmStart)
DEFINE_NULL_PROPERTY(CSensor, CExtendedMaxVideoFpsForPhotoRes, MaxVideoFpsForPhotoRes)
DEFINE_NULL_PROPERTY_GET(CSensor, CExtendedFieldOfView, FieldOfView)
DEFINE_NULL_PROPERTY_GET(CSensor, CExtendedCameraAngleOffset, CameraAngleOffset)
DEFINE_NULL_PROPERTY_GET(CSensor, KSCAMERA_EXTENDEDPROP_FOCUSSTATE, FocusState)
DEFINE_NULL_PROPERTY_ASYNC(CSensor, CExtendedVidProcSetting, Focus)
DEFINE_NULL_PROPERTY_ASYNC(CSensor, CExtendedProperty, Iso)
DEFINE_NULL_PROPERTY_ASYNC(CSensor, CExtendedVidProcSetting, IsoAdvanced)
DEFINE_NULL_PROPERTY_ASYNC(CSensor, CExtendedEvCompensation, EvCompensation)
DEFINE_NULL_PROPERTY_ASYNC(CSensor, CExtendedVidProcSetting, WhiteBalance)
DEFINE_NULL_PROPERTY_ASYNC(CSensor, CExtendedVidProcSetting, Exposure)
DEFINE_NULL_PROPERTY_ASYNC(CSensor, CExtendedProperty, SceneMode)
DEFINE_NULL_PROPERTY_ASYNC(CSensor, CExtendedProperty, Thumbnail)
DEFINE_NULL_PROPERTY(CSensor, CExtendedProperty, PhotoConfirmation)
DEFINE_NULL_PROPERTY(CSensor, CExtendedProperty, FocusPriority)
DEFINE_NULL_PROPERTY(CSensor, CExtendedProperty, VideoHDR)
DEFINE_NULL_PROPERTY(CSensor, CExtendedProperty, VFR)
DEFINE_NULL_PROPERTY(CSensor, CExtendedVidProcSetting, Zoom)
DEFINE_NULL_PROPERTY(CSensor, CExtendedProperty, VideoStabilization)
DEFINE_NULL_PROPERTY(CSensor, CExtendedProperty, Histogram)
DEFINE_NULL_PROPERTY(CSensor, CExtendedMetadata, Metadata)
DEFINE_NULL_PROPERTY(CSensor, CExtendedProperty, OpticalImageStabilization)
DEFINE_NULL_PROPERTY(CSensor, CExtendedProperty, OptimizationHint)
DEFINE_NULL_PROPERTY(CSensor, CExtendedProperty, AdvancedPhoto)
DEFINE_NULL_PROPERTY(CSensor, CExtendedVidProcSetting, FaceDetection)

DEFINE_NULL_PROPERTY(CSensor, KSPROPERTY_CAMERACONTROL_VIDEOSTABILIZATION_MODE_S, VideoStabMode)

DEFINE_NULL_PROPERTY(CSensor, KSPROPERTY_CAMERACONTROL_S, Exposure)
DEFINE_NULL_PROPERTY(CSensor, KSPROPERTY_CAMERACONTROL_S, Focus)
DEFINE_NULL_PROPERTY(CSensor, KSPROPERTY_CAMERACONTROL_S, Zoom)
DEFINE_NULL_PROPERTY(CSensor, KSPROPERTY_CAMERACONTROL_S, ZoomRelative)
DEFINE_NULL_PROPERTY(CSensor, KSPROPERTY_CAMERACONTROL_S, Pan)
DEFINE_NULL_PROPERTY(CSensor, KSPROPERTY_CAMERACONTROL_S, Roll)
DEFINE_NULL_PROPERTY(CSensor, KSPROPERTY_CAMERACONTROL_S, Tilt)
DEFINE_NULL_PROPERTY(CSensor, KSPROPERTY_CAMERACONTROL_FOCAL_LENGTH_S, FocalLength)

DEFINE_NULL_PROPERTY(CSensor, KSPROPERTY_VIDEOPROCAMP_S, BacklightCompensation);
DEFINE_NULL_PROPERTY(CSensor, KSPROPERTY_VIDEOPROCAMP_S, Brightness);
DEFINE_NULL_PROPERTY(CSensor, KSPROPERTY_VIDEOPROCAMP_S, Contrast);
DEFINE_NULL_PROPERTY(CSensor, KSPROPERTY_VIDEOPROCAMP_S, Hue);
DEFINE_NULL_PROPERTY(CSensor, KSPROPERTY_VIDEOPROCAMP_S, WhiteBalance);
DEFINE_NULL_PROPERTY(CSensor, KSPROPERTY_VIDEOPROCAMP_S, PowerlineFreq);

DEFINE_NULL_PROPERTY_GET(CSensor, CRoiConfig, RoiConfigCaps);
DEFINE_NULL_PROPERTY_VARSIZE_ASYNC(CSensor, CRoiProperty, Roi);

_Success_(return == 0)
NTSTATUS
CSensor::
GetPfsCaps(
    _Inout_opt_ KSCAMERA_PERFRAMESETTING_CAP_HEADER *Caps,
    _Inout_     ULONG                               *Size
)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(Caps);
    *Size = 0;
    return STATUS_NOT_FOUND;
}
