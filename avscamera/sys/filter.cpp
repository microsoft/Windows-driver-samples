/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2001, Microsoft Corporation.

    File:

        filter.cpp

    Abstract:

        This file contains the filter level implementation for the base capture 
        filter class.  It provides a wrapper on KSFILTER.  
        
        CCaptureFilter holds state information for the filter and contains
        implementations for standard camera controls.  These implementations
        validate arguments and capabilities of the control according to DDI
        definitions.  Once validated, control is passes to a CSensor object
        to manipulate the state of camera.

        To add a new control, derive from CCaptureFilter and use one of the 
        DECLARE_PROPERTY_XXX() macros below to declare the property handlers.
        You can also use the DEFINE_PROP_ITEM_XXX() and DEFINE_STD_EVENT_ITEM()
        macros as aids in constructing a filter's automation table.

    History:

        created 3/12/2001

**************************************************************************/

#include "Common.h"

/**************************************************************************

    PAGEABLE CODE

**************************************************************************/

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA

CCaptureFilter::CCaptureFilter (
    _In_    PKSFILTER Filter
):  
/*++

Routine Description:

    CCaptureFilter object construction.

Arguments:

    Filter -
        The AVStream filter being wrapped.

Return Value:

    None

--*/
    m_pCaptureDevice(NULL),
    m_pPerFrameSettings(nullptr),
    m_pinArray(nullptr),
    m_PhotoModeNotifier( Filter, &KSEVENTSETID_ExtendedCameraControl, KSPROPERTY_CAMERACONTROL_EXTENDED_PHOTOMODE ),
    m_PhotoMaxFrameRateNotifier( Filter, &KSEVENTSETID_ExtendedCameraControl, KSPROPERTY_CAMERACONTROL_EXTENDED_PHOTOMAXFRAMERATE) ,
    m_FocusNotifier( Filter, &KSEVENTSETID_ExtendedCameraControl, KSPROPERTY_CAMERACONTROL_EXTENDED_FOCUSMODE ),
    m_FocusRectNotifier( Filter, &EVENTSETID_VIDCAP_CAMERACONTROL_REGION_OF_INTEREST, KSPROPERTY_CAMERACONTROL_REGION_OF_INTEREST_PROPERTY_ID ),
    m_IsoNotifier( Filter, &KSEVENTSETID_ExtendedCameraControl, KSPROPERTY_CAMERACONTROL_EXTENDED_ISO ),
    m_IsoAdvancedNotifier( Filter, &KSEVENTSETID_ExtendedCameraControl, KSPROPERTY_CAMERACONTROL_EXTENDED_ISO_ADVANCED ),
    m_EvCompensationNotifier( Filter, &KSEVENTSETID_ExtendedCameraControl, KSPROPERTY_CAMERACONTROL_EXTENDED_EVCOMPENSATION ),
    m_WhiteBalanceNotifier( Filter, &KSEVENTSETID_ExtendedCameraControl, KSPROPERTY_CAMERACONTROL_EXTENDED_WHITEBALANCEMODE ),
    m_ExposureNotifier( Filter, &KSEVENTSETID_ExtendedCameraControl, KSPROPERTY_CAMERACONTROL_EXTENDED_EXPOSUREMODE ),
    m_SceneModeNotifier( Filter, &KSEVENTSETID_ExtendedCameraControl, KSPROPERTY_CAMERACONTROL_EXTENDED_SCENEMODE ),
    m_ThumbnailNotifier( Filter, &KSEVENTSETID_ExtendedCameraControl, KSPROPERTY_CAMERACONTROL_EXTENDED_PHOTOTHUMBNAIL ),
    m_WarmStartNotifier( Filter, &KSEVENTSETID_ExtendedCameraControl, KSPROPERTY_CAMERACONTROL_EXTENDED_WARMSTART ),
    m_RoiNotifier( Filter, &KSEVENTSETID_ExtendedCameraControl, KSPROPERTY_CAMERACONTROL_EXTENDED_ROI_ISPCONTROL ),
    m_ProfileNotifier( Filter, &KSEVENTSETID_ExtendedCameraControl, KSPROPERTY_CAMERACONTROL_EXTENDED_PROFILE )
{

    PAGED_CODE();

    DBG_ENTER("(Filter=%p)", Filter);

    m_pKSFilter = Filter;

    m_pCaptureDevice = CCaptureDevice::Recast(KsFilterGetDevice(Filter));

    KeQuerySystemTime (&m_StartTime);

    m_Profile.ProfileId = KSCAMERAPROFILE_Legacy;
    m_Profile.Index = 0;
    m_Profile.Reserved = 0;

    DBG_ENTER("(Filter=%p)", Filter);
}

NTSTATUS
CCaptureFilter::
Initialize()
/*++

Routine Description:

    Post construction initialization.

Arguments:

    [None]

Return Value:

    Success / failure

--*/
{

    PAGED_CODE();

    NTSTATUS Status = STATUS_SUCCESS;

    DBG_ENTER("(Filter=%S)", GetFilterName());

    //  Get Sensor object from our parent device object.

    m_Sensor = m_pCaptureDevice->GetSensor(m_pKSFilter);
    NT_ASSERT(m_Sensor);

    //  We get a nullptr back if the Filter doesn't match one of our's.
    if( !m_Sensor )
    {
        Status = STATUS_INVALID_PARAMETER;
        goto done;
    }

    //  In reality we're just adding a reference to the sensor for this filter.
    //  The sensor has no back pointer to the filter since more than one filter
    //  can be open at a time.
    IFFAILED_EXIT( m_Sensor->AddFilter(m_pKSFilter) );

    IFNULL_EXIT(m_pinArray = new (PagedPool, 'sniP') CCapturePin *[m_Sensor->GetPinCount()]);

    for( ULONG i=0; i<m_Sensor->GetPinCount(); i++ )
    {
        m_pinArray[i] = nullptr;
    }

done:
    DBG_LEAVE("(Filter=%S)=0x%08X", GetFilterName(), Status);
    return Status;
}

void
CCaptureFilter::Cleanup()
/*++

Routine Description:

    Clean up.  Cancels any outstanding operations on the sensor.

Arguments:

    [None]

Return Value:

    [None]

--*/
{
    PAGED_CODE();

    //  Cancel all Async operations.  Ignore any failures.
    m_Sensor->CancelPhotoMode();
    m_Sensor->CancelPhotoMaxFrameRate();
    m_Sensor->CancelFocus();
    m_Sensor->CancelFocusRect();
    m_Sensor->CancelIso();
    m_Sensor->CancelIsoAdvanced();
    m_Sensor->CancelEvCompensation();
    m_Sensor->CancelWhiteBalance();
    m_Sensor->CancelExposure();
    m_Sensor->CancelSceneMode();
    m_Sensor->CancelThumbnail();
    m_Sensor->CancelWarmStart();
    m_Sensor->CancelRoi();
}

//
// ~CCaptureFilter():
//
// The capture filter destructor.
//
CCaptureFilter::~CCaptureFilter()
{
    PAGED_CODE();

    LPCWSTR  Name = GetFilterName();
    DBG_ENTER("(%S)", Name);

    delete [] ((PBYTE) m_pPerFrameSettings);

    //  In reality we're just removing a reference to the sensor for this filter.
    m_Sensor->RemoveFilter(m_pKSFilter);
    
    //  The sensor may need to do some cleanup.  In particular, the image pin 
    //  may need to have some things reset.
    m_Sensor->Reset();

    delete [] m_pinArray;

    DBG_LEAVE("(%S)", Name);
}


NTSTATUS
CCaptureFilter::
DispatchCreate (
    _In_    PKSFILTER Filter,
    _In_    PIRP Irp
)
/*++

Routine Description:

    Static function for creation dispatch for the capture filter.  It creates
    the CCaptureFilter object.

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

    DBG_ENTER("(Filter=0x%p)", Filter);

    NTSTATUS Status = STATUS_SUCCESS;

    CCaptureFilter *CapFilter = new (NonPagedPoolNx, 'rtlf') CCaptureFilter (Filter);

    if (!CapFilter)
    {
        //
        // Return failure if we couldn't create the filter.
        //
        Status = STATUS_INSUFFICIENT_RESOURCES;

    }
    else
    {
        //  Allocate memory or whatever.
        Status = CapFilter->Initialize();

        if (!NT_SUCCESS (Status))
        {
            delete CapFilter;
        }
        else
        {
            Filter -> Context = reinterpret_cast <PVOID> (CapFilter);
        }

    }

    DBG_LEAVE( "()=0x%08X", Status );
    return Status;
}

NTSTATUS
CCaptureFilter::DispatchClose(
    _In_  PKSFILTER Filter,
    _In_  PIRP Irp
)
/*++

Routine Description:

    Static function for the close dispatch for the capture filter.  It handles 
    filter cleanup.

Arguments:

    Filter -
        The AVStream filter being closed

    Irp -
        The creation Irp

Return Value:

    Success / failure

--*/
{
    PAGED_CODE();

    CCaptureFilter *pFilter = reinterpret_cast <CCaptureFilter *>(Filter->Context);

    if( pFilter )
    {
        pFilter->Cleanup();
        delete pFilter;
    }
    return STATUS_SUCCESS;
}

void
CCaptureFilter::
setPin(
    _In_    CCapturePin *pPin,
    _In_    unsigned Id
)
/*++

Routine Description:

    Attach a CCapturePin to the filter.

Arguments:

    pPin -
        The pin.
    Id -
        The pin index.

Return Value:

    Success / failure

--*/
{
    PAGED_CODE( );

    if( Id >= m_Sensor->GetPinCount() )
    {
        __debugbreak();
        return;
    }

    //  Take out a weak reference to the pin object.
    m_pinArray[Id] = pPin;
}

CCapturePin *
CCaptureFilter::
getPin(
    _In_    unsigned Id
)
/*++

Routine Description:

    Get a pointer to an attached CCapturePin.

Arguments:

    Id -
        The pin index.

Return Value:

    Success / failure

--*/
{
    PAGED_CODE( );

    if( Id >= m_Sensor->GetPinCount() )
    {
        __debugbreak();
        return nullptr;
    }

    //  Convert back to a strong reference before returning.
    return m_pinArray[Id] ;
}

NTSTATUS
CCaptureFilter::
UpdateAllocatorFraming( 
    _In_    ULONG PinId
)
/*++

Routine Description:

    Update the Pin's Allocator Framing.

Arguments:

    PinId -
        A pin's index

Return Value:

    Success / failure

--*/
{
    PAGED_CODE();

    NTSTATUS    Status = STATUS_INVALID_PARAMETER;

    //  Acquire the lock and update the Pin's allocator framing.
    LockFilter Lock(m_pKSFilter);
    
    if( m_pinArray[PinId] )
    {
        Status = m_pinArray[PinId]->UpdateAllocatorFraming();
    }

    DBG_LEAVE("()=0x%08X",Status);
    return Status;
}

/**************************************************************************

    Control validation and forwarding.

**************************************************************************/

//  Get for KSPROPERTY_CAMERACONTROL_FLASH_PROPERTY_ID
NTSTATUS
CCaptureFilter::
GetFlash(
    _Inout_ KSPROPERTY_CAMERACONTROL_FLASH_S   *pFlash
)
{
    PAGED_CODE();

    return
        m_Sensor->GetFlash( pFlash );
}

//  Set for KSPROPERTY_CAMERACONTROL_FLASH_PROPERTY_ID
NTSTATUS
CCaptureFilter::
SetFlash(
    _In_    KSPROPERTY_CAMERACONTROL_FLASH_S   *pFlash
)
{
    PAGED_CODE();

    KSPROPERTY_CAMERACONTROL_FLASH_S    Caps;

    RtlZeroMemory( &Caps, sizeof(Caps) );
    NTSTATUS    Status = m_Sensor->GetFlash(&Caps);

    if( NT_SUCCESS(Status) )
    {
        //  Assume an invalid parameter.
        Status = STATUS_INVALID_PARAMETER;

        //  Validate the parameters against our caps.
        switch( pFlash->Flash )
        {
        case KSPROPERTY_CAMERACONTROL_FLASH_OFF:
        case KSPROPERTY_CAMERACONTROL_FLASH_ON:
            if( Caps.Capabilities & KSPROPERTY_CAMERACONTROL_FLASH_FLAGS_MANUAL )
            {
                Status = STATUS_SUCCESS;
            }
            break;

        case KSPROPERTY_CAMERACONTROL_FLASH_AUTO:
            if( Caps.Capabilities & KSPROPERTY_CAMERACONTROL_FLASH_FLAGS_AUTO )
            {
                Status = STATUS_SUCCESS;
            }
            break;
        }
    }
    if( NT_SUCCESS(Status) )
    {
        Status = m_Sensor->SetFlash( pFlash );
    }

    return Status;
}

//  Get for PROPSETID_VIDCAP_CAMERACONTROL_VIDEO_STABILIZATION:0
NTSTATUS
CCaptureFilter::
GetVideoStabMode(
    _Inout_ KSPROPERTY_CAMERACONTROL_VIDEOSTABILIZATION_MODE_S *pVideoStab
)
{
    PAGED_CODE();

    return
        m_Sensor->GetVideoStabMode( pVideoStab );
}

//  Set for PROPSETID_VIDCAP_CAMERACONTROL_VIDEO_STABILIZATION:0
NTSTATUS
CCaptureFilter::
SetVideoStabMode(
    _In_    KSPROPERTY_CAMERACONTROL_VIDEOSTABILIZATION_MODE_S *pVideoStab
)
{
    PAGED_CODE();

    KSPROPERTY_CAMERACONTROL_VIDEOSTABILIZATION_MODE_S  Caps;

    RtlZeroMemory( &Caps, sizeof(Caps) );
    NTSTATUS    Status = m_Sensor->GetVideoStabMode( &Caps );

    DBG_ENTER("(Mode=0x%08X, Caps=0x%08X)", pVideoStab->VideoStabilizationMode, Caps.Capabilities );

    if( NT_SUCCESS(Status) )
    {
        Status = STATUS_INVALID_PARAMETER;
        switch( pVideoStab->VideoStabilizationMode )
        {
        case KSPROPERTY_CAMERACONTROL_VIDEOSTABILIZATION_MODE_OFF:
            Status = STATUS_SUCCESS;
            break;

        case KSPROPERTY_CAMERACONTROL_VIDEOSTABILIZATION_MODE_AUTO:
            if( Caps.Capabilities & KSPROPERTY_CAMERACONTROL_VIDEOSTABILIZATION_MODE_FLAGS_AUTO )
            {
                Status = STATUS_SUCCESS;
            }
            break;

        case KSPROPERTY_CAMERACONTROL_VIDEOSTABILIZATION_MODE_LOW:
        case KSPROPERTY_CAMERACONTROL_VIDEOSTABILIZATION_MODE_MEDIUM:
        case KSPROPERTY_CAMERACONTROL_VIDEOSTABILIZATION_MODE_HIGH:
            if( Caps.Capabilities & KSPROPERTY_CAMERACONTROL_VIDEOSTABILIZATION_MODE_FLAGS_MANUAL )
            {
                Status = STATUS_SUCCESS;
            }
            break;
        }
    }
    if( NT_SUCCESS(Status) )
    {
        Status = m_Sensor->SetVideoStabMode( pVideoStab );
    }
    DBG_LEAVE("(Mode=0x%08X, Caps=0x%08X)=0x%08X", pVideoStab->VideoStabilizationMode, Caps.Capabilities, Status );
    return Status;
}

//  Get for KSPROPERTY_CAMERACONTROL_IMAGE_PIN_CAPABILITY_PROPERTY_ID
NTSTATUS
CCaptureFilter::
GetPinDependence(
    _Inout_ KSPROPERTY_CAMERACONTROL_IMAGE_PIN_CAPABILITY_S *pPinDependence
)
{
    PAGED_CODE();

    return
        m_Sensor->GetPinDependence( pPinDependence );
}

//
//  A macro used for consistent asynchronous SET behavior.
//
#define INVOKE_SET_ASYNC( SENSOR, PROPERTY, OUTPUT, NOTIFIER )                                          \
    ( OUTPUT->Flags & KSCAMERA_EXTENDEDPROP_FLAG_CANCELOPERATION ) ? SENSOR->Cancel ## PROPERTY () :    \
        SENSOR->Set ## PROPERTY ## Async( OUTPUT, &NOTIFIER );

#define INVOKE_SET_ASYNC_NOT_CANCELLABLE( SENSOR, PROPERTY, OUTPUT, NOTIFIER )                          \
    ( OUTPUT->Flags & KSCAMERA_EXTENDEDPROP_FLAG_CANCELOPERATION ) ? STATUS_INVALID_PARAMETER :         \
        SENSOR->Set ## PROPERTY ## Async( OUTPUT, &NOTIFIER );

//  Get for KSPROPERTY_CAMERACONTROL_REGION_OF_INTEREST_PROPERTY_ID
NTSTATUS
CCaptureFilter::GetFocusRect(
    _Inout_ KSPROPERTY_CAMERACONTROL_REGION_OF_INTEREST_S   *pRoi
)
{
    PAGED_CODE();
    return m_Sensor->GetFocusRect( pRoi );
}

//  Set for KSPROPERTY_CAMERACONTROL_REGION_OF_INTEREST_PROPERTY_ID
NTSTATUS
CCaptureFilter::
SetFocusRect(
    _In_    KSPROPERTY_CAMERACONTROL_REGION_OF_INTEREST_S   *pRoi
)
{
    PAGED_CODE();

    return m_Sensor->SetFocusRectAsync( pRoi, &m_FocusRectNotifier );
}

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_PHOTOMODE
NTSTATUS
CCaptureFilter::GetPhotoMode(
    _Inout_ CExtendedPhotoMode  *pMode
)
{
    PAGED_CODE();

    DBG_ENTER( "()" );

    NTSTATUS    Status = STATUS_INVALID_PARAMETER;

    if( pMode->isValid() &&
            m_Sensor->IsStillIndex( pMode->PinId ) )
    {
        Status = m_Sensor->GetPhotoMode( pMode );
    }

    DBG_TRACE( "Capability=0x%016llX", pMode->Capability );
    DBG_TRACE( "PinId=%d", pMode->PinId );
    DBG_TRACE( "MaxHistoryFrames=%d", pMode->MaxHistoryFrames() );

    DBG_LEAVE( "()=0x%08X", Status );
    return Status;
}

//  Set KSPROPERTY_CAMERACONTROL_EXTENDED_PHOTOMODE
NTSTATUS
CCaptureFilter::
SetPhotoMode(
    _In_    CExtendedPhotoMode  *pMode
)
{
    PAGED_CODE();

    NTSTATUS    Status = STATUS_INVALID_PARAMETER;

    DBG_ENTER( "()" );

    if( pMode->isValid() &&
            m_Sensor->IsStillIndex( pMode->PinId ) &&
            (pMode->Capability & KSCAMERA_EXTENDEDPROP_CAPS_ASYNCCONTROL) )
    {
        CExtendedPhotoMode  Caps(*pMode);
        Status = m_Sensor->GetPhotoMode( &Caps );

        if( NT_SUCCESS(Status) )
        {
            //  Assume an invalid parameter again.
            Status = STATUS_INVALID_PARAMETER;

            if( (pMode->Flags & Caps.Capability) == pMode->Flags )
            {
                //  We don't support cancel here.
                switch( pMode->Flags )
                {
                case KSCAMERA_EXTENDEDPROP_PHOTOMODE_SEQUENCE:
                {
                    //  We cache the Per-Frame Settings in the filter as a simplification for the getter.
                    //  But we can also use it here to tell us basic info about the current PFS.
                    PKSCAMERA_PERFRAMESETTING_HEADER    pPFS = m_pPerFrameSettings;
                    if( pMode->SubMode() == KSCAMERA_EXTENDEDPROP_PHOTOMODE_SEQUENCE_SUB_VARIABLE && 
                        !pPFS )
                    {
                        DBG_TRACE( "VPS requested without Per Frame Settings." );
                        break;
                    }

                    //  Reject any request for too many history frames.
                    if( pMode->RequestedHistoryFrames() > Caps.MaxHistoryFrames() )
                    {
                        DBG_TRACE( "Too many history frames requested." );
                        break;
                    }

                    DBG_TRACE( "Flags=0x%08X, RequestedHistoryFrames=%d, SubMode=0x%08X",
                               (ULONG) pMode->Flags,
                               pMode->RequestedHistoryFrames(),
                               pMode->SubMode() );

                    //  Assume the normal photo sequence case.
                    //  For normal photo sequence, we provide history frames +1 or at least a minimum of 3 frames.
                    ULONG   TotalFrames = max( (Caps.RequestedHistoryFrames() + 1), IMAGE_CAPTURE_PIN_MINIMUM_FRAMES );

                    //  Modify TotalFrames for the VPS case.
                    //  For VPS, we need LoopCount * FrameCount; but limit by the minimum(3) and maximum (20) frames.
                    if( KSCAMERA_EXTENDEDPROP_PHOTOMODE_SEQUENCE_SUB_VARIABLE==pMode->SubMode() )
                    {
                        ULONG   LoopCount = max( pPFS->LoopCount, 1 );
                        ULONG   FrameCount= pPFS->FrameCount;

                        TotalFrames = max( IMAGE_CAPTURE_PIN_MINIMUM_FRAMES, Caps.RequestedHistoryFrames() + (LoopCount * FrameCount) );
                        ASSERT( TotalFrames <= IMAGE_CAPTURE_PIN_MAXIMUM_FRAMES );

                        DBG_TRACE( "TotalFrames=%d, LoopCount=%d, FrameCount=%d", TotalFrames, LoopCount, FrameCount );
                    }

                    TotalFrames = min( TotalFrames, IMAGE_CAPTURE_PIN_MAXIMUM_FRAMES );
                    DBG_TRACE( "Advertising Framing requirement: %d", TotalFrames );

                    Status = UpdateAllocatorFraming( pMode->PinId );
                    break;
                }
                case KSCAMERA_EXTENDEDPROP_PHOTOMODE_NORMAL:
                {
                    //  Set it back to 4 frames.
                    Status = UpdateAllocatorFraming( pMode->PinId );
                    break;
                }
                }

                //  Update the sensor object...
                if( NT_SUCCESS(Status) )
                {
                    Status = m_Sensor->SetPhotoModeAsync( pMode, &m_PhotoModeNotifier );
                }
            }
        }
    }

    DBG_TRACE( "Capability=0x%016llX", pMode->Capability );
    DBG_TRACE( "Flags=0x%016llX", pMode->Flags );
    DBG_TRACE( "PinId=%d", pMode->PinId );
    DBG_TRACE( "MaxHistoryFrames=%d", pMode->MaxHistoryFrames() );
    DBG_TRACE( "RequestedHistoryFrames=%d", pMode->RequestedHistoryFrames() );
    DBG_TRACE( "SubMode=0x%08X", pMode->SubMode() );
    DBG_LEAVE( "()=0x%08X", Status );
    return Status;
}

/*
 *  KSPROPERTY_CAMERACONTROL_EXTENDED_PHOTOFRAMERATE Get Handler
 */
NTSTATUS
CCaptureFilter::GetPhotoFrameRate(
    _Inout_ CExtendedProperty   *pFrameRate
)
{
    PAGED_CODE();

    NTSTATUS    Status = STATUS_INVALID_PARAMETER;

    if( pFrameRate->isValid() &&
            m_Sensor->IsStillIndex(pFrameRate->PinId) )
    {
        ULONGLONG TimePerFrame = m_Sensor->GetPhotoFrameRate( );
        if( TimePerFrame > ULONG_MAX )
        {
            // Handle very large (slow) frame rates in seconds.
            pFrameRate->m_Value.Value.ratio.HighPart = 1;
            pFrameRate->m_Value.Value.ratio.LowPart = (ULONG) (TimePerFrame / ONESECOND);
        }
        else
        {
            // Handle smaller frame rates as 100ns units.
            pFrameRate->m_Value.Value.ratio.HighPart = ONESECOND;
            pFrameRate->m_Value.Value.ratio.LowPart = (ULONG) TimePerFrame;
        }
        Status = STATUS_SUCCESS;
    }

    DBG_LEAVE("(PinId=%d, estimated FrameRate=%d/%d)=0x%08X",
              pFrameRate->PinId,
              pFrameRate->m_Value.Value.ratio.HighPart,
              pFrameRate->m_Value.Value.ratio.LowPart,
              Status );
    return Status;
}

/*
 *  KSPROPERTY_CAMERACONTROL_EXTENDED_MAXVIDFPS_PHOTORES Get Handler
 */
NTSTATUS
CCaptureFilter::
GetMaxVideoFpsForPhotoRes(
    _Inout_ CExtendedMaxVideoFpsForPhotoRes *pFps
)
{
    PAGED_CODE();

    NTSTATUS    Status = STATUS_INVALID_PARAMETER;

    if( pFps->isValid() &&
            pFps->PinId == KSCAMERA_EXTENDEDPROP_FILTERSCOPE &&
            pFps->Capability == 0 &&
            pFps->Flags == 0 )
    {
        Status = m_Sensor->GetMaxVideoFpsForPhotoRes( pFps );
    }
    return Status;
}

/*
 *  KSPROPERTY_CAMERACONTROL_EXTENDED_MAXVIDFPS_PHOTORES Set Handler
 */
NTSTATUS
CCaptureFilter::SetMaxVideoFpsForPhotoRes(
    _In_    CExtendedMaxVideoFpsForPhotoRes *pFps
)
{
    PAGED_CODE();

    NTSTATUS    Status = STATUS_INVALID_PARAMETER;

    if( pFps->isValid() &&
            pFps->PinId == KSCAMERA_EXTENDEDPROP_FILTERSCOPE &&
            pFps->Capability == 0 &&
            pFps->Flags == 0 )
    {
        Status = m_Sensor->SetMaxVideoFpsForPhotoRes( pFps );
    }
    return Status;
}

/*
 *  KSPROPERTY_CAMERACONTROL_EXTENDED_PHOTOMAXFRAMERATE Get Handler
 */
NTSTATUS
CCaptureFilter::GetPhotoMaxFrameRate(
    _Inout_ CExtendedProperty   *pFrameRate
)
{
    PAGED_CODE();

    NTSTATUS    Status = STATUS_INVALID_PARAMETER;

    if( pFrameRate->isValid() &&
            m_Sensor->IsStillIndex(pFrameRate->PinId) )
    {
        Status = m_Sensor->GetPhotoMaxFrameRate( pFrameRate );
    }
    return Status;
}

/*
 *  KSPROPERTY_CAMERACONTROL_EXTENDED_PHOTOMAXFRAMERATE Set Handler
 */
NTSTATUS
CCaptureFilter::
SetPhotoMaxFrameRate(
    _In_    CExtendedProperty   *pFrameRate
)
{
    PAGED_CODE();

    DBG_ENTER("(MaxFrameRate = %d/%d)",
              pFrameRate->m_Value.Value.ratio.HighPart,
              pFrameRate->m_Value.Value.ratio.LowPart );

    NTSTATUS    Status = STATUS_INVALID_PARAMETER;

    if( pFrameRate->isValid() &&
            m_Sensor->IsStillIndex( pFrameRate->PinId ) &&
            (pFrameRate->Capability & KSCAMERA_EXTENDEDPROP_CAPS_ASYNCCONTROL) )
    {
        //  Don't allow either the numerator or denominator alone to be 0.
        //  We don't want a rate of 0 fps or undefined.
        if( pFrameRate->m_Value.Value.ll == 0 ||
                ( pFrameRate->m_Value.Value.ratio.LowPart != 0 &&
                  pFrameRate->m_Value.Value.ratio.HighPart != 0 ) )
        {
            //  Take what we're given here; SetPhotoFrameRate handles default setting.
            DBG_TRACE("[Override] MaxFrameRate = %d/%d",
                      pFrameRate->m_Value.Value.ratio.HighPart,
                      pFrameRate->m_Value.Value.ratio.LowPart );

            Status = m_Sensor->SetPhotoMaxFrameRateAsync( pFrameRate, &m_PhotoMaxFrameRateNotifier );
        }
    }
    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_PHOTOTRIGGERTIME
NTSTATUS
CCaptureFilter::
GetTriggerTime(
    _Inout_ CExtendedProperty  *pTriggerTime
)
{
    PAGED_CODE();

    NTSTATUS    Status = STATUS_INVALID_PARAMETER;

    if( pTriggerTime->isValid() &&
            m_Sensor->IsStillIndex(pTriggerTime->PinId) )
    {
        Status = m_Sensor->GetTriggerTime( pTriggerTime );
    }
    return Status;
}

//  Set KSPROPERTY_CAMERACONTROL_EXTENDED_PHOTOTRIGGERTIME
NTSTATUS
CCaptureFilter::
SetTriggerTime(
    _In_    CExtendedProperty   *pTriggerTime
)
{
    PAGED_CODE();

    NTSTATUS    Status = STATUS_INVALID_PARAMETER;

    if( pTriggerTime->isValid() &&
            pTriggerTime->Capability == 0  &&
            m_Sensor->IsStillIndex(pTriggerTime->PinId) )
    {
        Status = m_Sensor->SetTriggerTime( pTriggerTime );
    }
    return Status;
}

/*
 *  KSPROPERTY_CAMERACONTROL_EXTENDED_WARMSTART
 */
NTSTATUS
CCaptureFilter::
GetWarmStart(
    _Inout_ CExtendedProperty   *pWarmStart
)
{
    PAGED_CODE();

    NTSTATUS    Status = STATUS_INVALID_PARAMETER;

    if( pWarmStart->isValid() &&
            m_Sensor->IsValidIndex( pWarmStart->PinId ) )
    {
        Status = m_Sensor->GetWarmStart( pWarmStart );
    }
    DBG_LEAVE("(PinId=%d, Flags=0x%016llX, Cap=0x%016llX)=0x%08X",
                pWarmStart->PinId, pWarmStart->Flags, pWarmStart->Capability, Status );
    return Status;
}

/*
 *  KSPROPERTY_CAMERACONTROL_EXTENDED_WARMSTART Set Handler
 */
NTSTATUS
CCaptureFilter::SetWarmStart(
    _In_    CExtendedProperty   *pWarmStart
)
{
    PAGED_CODE();
    DBG_LEAVE("(PinId=%d, Flags=0x%016llX, Cap=0x%016llX)",
                pWarmStart->PinId, pWarmStart->Flags, pWarmStart->Capability );

    NTSTATUS    Status = STATUS_INVALID_PARAMETER;

    if( pWarmStart->isValid() &&
        m_Sensor->IsValidIndex( pWarmStart->PinId ) &&
        (pWarmStart->Capability & KSCAMERA_EXTENDEDPROP_CAPS_ASYNCCONTROL) )
    {
        CExtendedProperty   Caps(*pWarmStart);
        Status = m_Sensor->GetWarmStart( &Caps );

        if( NT_SUCCESS(Status ) )
        {
            if( pWarmStart->Flags == (pWarmStart->Flags & Caps.Capability) )
            {
                Status = INVOKE_SET_ASYNC( m_Sensor, WarmStart, pWarmStart, m_WarmStartNotifier );
            }
        }
        else
        {
            Status = STATUS_INVALID_PARAMETER;
        }
    }

    DBG_LEAVE("()=0x%08X", Status );
    return Status;
}

//  Set KSPROPERTY_CAMERACONTROL_EXTENDED_PHOTOTHUMBNAIL
NTSTATUS
CCaptureFilter::
SetThumbnail(
    _In_    CExtendedProperty   *pThumbnail
)
{
    PAGED_CODE();
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    DBG_ENTER("()");

    //  Validate
    if( (pThumbnail->Capability & KSCAMERA_EXTENDEDPROP_CAPS_ASYNCCONTROL) &&
            pThumbnail->isValid() &&
            m_Sensor->IsStillIndex(pThumbnail->PinId) )
    {
        CExtendedProperty   Caps;
        Status = m_Sensor->GetThumbnail( &Caps );

        if( NT_SUCCESS(Status) )
        {
            if( pThumbnail->Flags == (Caps.Capability & pThumbnail->Flags) )
            {
                Status = INVOKE_SET_ASYNC( m_Sensor, Thumbnail, pThumbnail, m_ThumbnailNotifier );
            }
            else
            {
                Status = STATUS_INVALID_PARAMETER;
            }
        }
    }

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_PHOTOTHUMBNAIL
NTSTATUS
CCaptureFilter::
GetThumbnail(
    _Inout_ CExtendedProperty   *pThumbnail
)
{
    PAGED_CODE();
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    DBG_ENTER("()");

    //  Validate
    if( pThumbnail->isValid() &&
            m_Sensor->IsStillIndex(pThumbnail->PinId) )
    {
        Status = m_Sensor->GetThumbnail( pThumbnail );
    }

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_SCENEMODE
NTSTATUS
CCaptureFilter::
GetSceneMode(
    _Inout_ CExtendedProperty   *pSceneMode
)
{
    PAGED_CODE();
    DBG_ENTER("()");

    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    if( pSceneMode->isValid() &&
            pSceneMode->PinId == KSCAMERA_EXTENDEDPROP_FILTERSCOPE )
    {
        Status = m_Sensor->GetSceneMode( pSceneMode );
    }

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

//  Set KSPROPERTY_CAMERACONTROL_EXTENDED_SCENEMODE
NTSTATUS
CCaptureFilter::
SetSceneMode(
    _In_    CExtendedProperty   *pSceneMode
)
{
    PAGED_CODE();
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    DBG_ENTER("()");

    if( (pSceneMode->Capability & KSCAMERA_EXTENDEDPROP_CAPS_ASYNCCONTROL) &&
            pSceneMode->isValid() &&
            pSceneMode->PinId == KSCAMERA_EXTENDEDPROP_FILTERSCOPE )
    {
        CExtendedProperty   Caps;
        Status = m_Sensor->GetSceneMode( &Caps );

        if( NT_SUCCESS(Status) )
        {
            if( pSceneMode->Flags == (Caps.Capability & pSceneMode->Flags) )
            {
                Status = INVOKE_SET_ASYNC_NOT_CANCELLABLE( m_Sensor, SceneMode, pSceneMode, m_SceneModeNotifier );
            }
            else
            {
                Status = STATUS_INVALID_PARAMETER;
            }
        }
    }

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_TORCHMODE
NTSTATUS
CCaptureFilter::
GetTorchMode(
    _Inout_ CExtendedProperty  *pTorchMode
)
{
    PAGED_CODE();

    NTSTATUS    Status = STATUS_INVALID_PARAMETER;

    if( pTorchMode->isValid() &&
            pTorchMode->PinId == KSCAMERA_EXTENDEDPROP_FILTERSCOPE )
    {
        Status = m_Sensor->GetTorchMode( pTorchMode );
    }
    DBG_LEAVE("(PinId=%d, Flags=0x%016llX, Cap=0x%016llX)=0x%08X",
              pTorchMode->PinId, pTorchMode->Flags, pTorchMode->Capability, Status );
    return Status;
}

//  Set KSPROPERTY_CAMERACONTROL_EXTENDED_TORCHMODE
NTSTATUS
CCaptureFilter::
SetTorchMode(
    _In_    CExtendedProperty  *pTorchMode
)
{
    PAGED_CODE();

    NTSTATUS    Status = STATUS_INVALID_PARAMETER;

    if( !(pTorchMode->Capability & KSCAMERA_EXTENDEDPROP_CAPS_ASYNCCONTROL) &&
            pTorchMode->isValid() &&
            pTorchMode->PinId == KSCAMERA_EXTENDEDPROP_FILTERSCOPE &&
            pTorchMode->m_Value.Value.ul <= 100 )
    {
        CExtendedProperty   Caps(*pTorchMode);
        Status = m_Sensor->GetTorchMode(&Caps);

        if( NT_SUCCESS(Status) )
        {
            //  Assume an invalid parameter.
            Status = STATUS_INVALID_PARAMETER;

            if( pTorchMode->Flags == (pTorchMode->Flags & Caps.Capability) )
            {
                switch( pTorchMode->Flags )
                {
                case KSCAMERA_EXTENDEDPROP_VIDEOTORCH_OFF:
                case KSCAMERA_EXTENDEDPROP_VIDEOTORCH_ON:
                case KSCAMERA_EXTENDEDPROP_VIDEOTORCH_ON_ADJUSTABLEPOWER:
                    Status = m_Sensor->SetTorchMode( pTorchMode );
                    break;
                }
            }
        }
    }
    DBG_LEAVE("(PinId=%d, Flags=0x%016llX, Cap=0x%016llX)=0x%08X",
              pTorchMode->PinId, pTorchMode->Flags, pTorchMode->Capability, Status );
    return Status;
}

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_FLASHMODE
NTSTATUS
CCaptureFilter::
GetExtendedFlash(
    _Inout_ CExtendedProperty  *pFlash
)
{
    PAGED_CODE();

    NTSTATUS    Status = STATUS_INVALID_PARAMETER;

    if( pFlash->isValid() &&
            pFlash->PinId == KSCAMERA_EXTENDEDPROP_FILTERSCOPE)
    {
        Status = m_Sensor->GetExtendedFlash( pFlash );
    }
    DBG_LEAVE("(PinId=%d, Flags=0x%016llX, Cap=0x%016llX)=0x%08X",
              pFlash->PinId, pFlash->Flags, pFlash->Capability, Status );
    return Status;
}

//  Set KSPROPERTY_CAMERACONTROL_EXTENDED_FLASHMODE
NTSTATUS
CCaptureFilter::
SetExtendedFlash(
    _In_    CExtendedProperty  *pFlash
)
{
    PAGED_CODE();

    NTSTATUS    Status = STATUS_INVALID_PARAMETER;

    if( !(pFlash->Capability & KSCAMERA_EXTENDEDPROP_CAPS_ASYNCCONTROL) &&
            pFlash->isValid() &&
            pFlash->PinId == KSCAMERA_EXTENDEDPROP_FILTERSCOPE)
    {
        CExtendedProperty   Caps;
        Status = m_Sensor->GetExtendedFlash( &Caps );

        if( NT_SUCCESS(Status) )
        {
            //  Assume an invalid parameter.
            Status = STATUS_INVALID_PARAMETER;

            if( pFlash->Flags == (pFlash->Flags & Caps.Capability) )
            {
                //  Validate flash settings.
                //  First validate the basic flash controls
                switch( pFlash->Flags & KSCAMERA_EXTENDEDPROP_FLASH_MODE_MASK )
                {
                //  Valid combinations
                case KSCAMERA_EXTENDEDPROP_FLASH_OFF:
                case KSCAMERA_EXTENDEDPROP_FLASH_ON:
                case KSCAMERA_EXTENDEDPROP_FLASH_ON_ADJUSTABLEPOWER:
                case KSCAMERA_EXTENDEDPROP_FLASH_AUTO:
                case KSCAMERA_EXTENDEDPROP_FLASH_AUTO_ADJUSTABLEPOWER:
                    Status = STATUS_SUCCESS;
                    break;
                }

                //  Validate red eye reduction, single flash & multiflash supported.
                if( !(pFlash->Flags &
                        ( KSCAMERA_EXTENDEDPROP_FLASH_REDEYEREDUCTION   |
                          KSCAMERA_EXTENDEDPROP_FLASH_SINGLEFLASH       |
                          KSCAMERA_EXTENDEDPROP_FLASH_MULTIFLASHSUPPORTED )) ||
                        (pFlash->Flags & KSCAMERA_EXTENDEDPROP_FLASH_MODE_MASK) != KSCAMERA_EXTENDEDPROP_FLASH_OFF )
                {
                    Status = STATUS_SUCCESS;
                }

                //  Validate Flash Assistant parameters
                switch( pFlash->Flags & KSCAMERA_EXTENDEDPROP_FLASH_ASSISTANT_MASK )
                {
                //  Valid combinations
                case KSCAMERA_EXTENDEDPROP_FLASH_ASSISTANT_OFF:
                case KSCAMERA_EXTENDEDPROP_FLASH_ASSISTANT_ON:
                case KSCAMERA_EXTENDEDPROP_FLASH_ASSISTANT_AUTO:
                    Status = STATUS_SUCCESS;
                    break;
                }
            }
        }
    }

    if( NT_SUCCESS(Status) )
    {
        Status = m_Sensor->SetExtendedFlash( pFlash );
    }

    DBG_LEAVE("(PinId=%d, Flags=0x%016llX, Cap=0x%016llX)=0x%08X",
              pFlash->PinId, pFlash->Flags, pFlash->Capability, Status );
    return Status;
}

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_OPTIMIZATIONHINT
NTSTATUS
CCaptureFilter::
GetOptimizationHint(
    _Inout_ CExtendedProperty  *pHint
)
{
    PAGED_CODE();
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    if( pHint->isValid() &&
            !(pHint->Capability & KSCAMERA_EXTENDEDPROP_CAPS_ASYNCCONTROL) &&
            pHint->PinId == KSCAMERA_EXTENDEDPROP_FILTERSCOPE )
    {
        Status = m_Sensor->GetOptimizationHint(pHint);
    }
    DBG_LEAVE("(PinId=%d, Flags=0x%016llX, Cap=0x%016llX)=0x%08X",
              pHint->PinId, pHint->Flags, pHint->Capability, Status );
    return Status;
}

//  Set KSPROPERTY_CAMERACONTROL_EXTENDED_OPTIMIZATIONHINT
NTSTATUS
CCaptureFilter::
SetOptimizationHint(
    _In_    CExtendedProperty   *pHint
)
{
    PAGED_CODE();
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    if( pHint->isValid() &&
            !(pHint->Capability & KSCAMERA_EXTENDEDPROP_CAPS_ASYNCCONTROL) &&
            pHint->PinId == KSCAMERA_EXTENDEDPROP_FILTERSCOPE )
    {
        CExtendedProperty   Caps(*pHint);
        Status = m_Sensor->GetOptimizationHint(&Caps);

        if( NT_SUCCESS(Status) )
        {
            //  Assume an invalid parameter.
            Status = STATUS_INVALID_PARAMETER;

            if( pHint->Flags == (pHint->Flags & Caps.Capability) )
            {
                //  Inherit the previous primary use flag settings, if they were not set here.
                if( KSCAMERA_EXTENDEDPROP_OPTIMIZATION_DEFAULT == pHint->Flags ||
                        0 == (pHint->Flags & KSCAMERA_EXTENDEDPROP_OPTIMIZATION_PRIMARYUSE_MASK) )
                {
                    pHint->Flags |= (Caps.Flags & KSCAMERA_EXTENDEDPROP_OPTIMIZATION_PRIMARYUSE_MASK);
                }
                else
                {
                    //  Inherit the previous perf flag settings, if they were not set here and if DEFAULT is not set.
                    if( 0 == (pHint->Flags & KSCAMERA_EXTENDEDPROP_OPTIMIZATION_PERF_MASK) )
                    {
                        pHint->Flags |= (Caps.Flags & KSCAMERA_EXTENDEDPROP_OPTIMIZATION_PERF_MASK);
                    }
                }

                //  Make sure the requested flags are a valid capability combination.
                switch( pHint->Flags )
                {
                case KSCAMERA_EXTENDEDPROP_OPTIMIZATION_PHOTO:
                case KSCAMERA_EXTENDEDPROP_OPTIMIZATION_PHOTO | KSCAMERA_EXTENDEDPROP_OPTIMIZATION_QUALITY:
                case KSCAMERA_EXTENDEDPROP_OPTIMIZATION_PHOTO | KSCAMERA_EXTENDEDPROP_OPTIMIZATION_LATENCY:
                case KSCAMERA_EXTENDEDPROP_OPTIMIZATION_PHOTO | KSCAMERA_EXTENDEDPROP_OPTIMIZATION_POWER  :
                case KSCAMERA_EXTENDEDPROP_OPTIMIZATION_VIDEO:
                case KSCAMERA_EXTENDEDPROP_OPTIMIZATION_VIDEO | KSCAMERA_EXTENDEDPROP_OPTIMIZATION_QUALITY:
                case KSCAMERA_EXTENDEDPROP_OPTIMIZATION_VIDEO | KSCAMERA_EXTENDEDPROP_OPTIMIZATION_LATENCY:
                case KSCAMERA_EXTENDEDPROP_OPTIMIZATION_VIDEO | KSCAMERA_EXTENDEDPROP_OPTIMIZATION_POWER  :
                case KSCAMERA_EXTENDEDPROP_OPTIMIZATION_PHOTO       |
                        KSCAMERA_EXTENDEDPROP_OPTIMIZATION_QUALITY  |
                        KSCAMERA_EXTENDEDPROP_OPTIMIZATION_LATENCY  :
                case KSCAMERA_EXTENDEDPROP_OPTIMIZATION_PHOTO       |
                        KSCAMERA_EXTENDEDPROP_OPTIMIZATION_QUALITY  |
                        KSCAMERA_EXTENDEDPROP_OPTIMIZATION_POWER    :
                case KSCAMERA_EXTENDEDPROP_OPTIMIZATION_PHOTO       |
                        KSCAMERA_EXTENDEDPROP_OPTIMIZATION_LATENCY  |
                        KSCAMERA_EXTENDEDPROP_OPTIMIZATION_POWER    :
                case KSCAMERA_EXTENDEDPROP_OPTIMIZATION_VIDEO       |
                        KSCAMERA_EXTENDEDPROP_OPTIMIZATION_QUALITY  |
                        KSCAMERA_EXTENDEDPROP_OPTIMIZATION_LATENCY  :
                case KSCAMERA_EXTENDEDPROP_OPTIMIZATION_VIDEO       |
                        KSCAMERA_EXTENDEDPROP_OPTIMIZATION_QUALITY  |
                        KSCAMERA_EXTENDEDPROP_OPTIMIZATION_POWER    :
                case KSCAMERA_EXTENDEDPROP_OPTIMIZATION_VIDEO       |
                        KSCAMERA_EXTENDEDPROP_OPTIMIZATION_LATENCY  |
                        KSCAMERA_EXTENDEDPROP_OPTIMIZATION_POWER    :
                    Status = m_Sensor->SetOptimizationHint(pHint);
                    break;
                }
            }
        }
    }
    DBG_LEAVE("(PinId=%d, Flags=0x%016llX, Cap=0x%016llX)=0x%08X",
              pHint->PinId, pHint->Flags, pHint->Capability, Status );
    return Status;
}

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_FIELDOFVIEW
NTSTATUS
CCaptureFilter::
GetFieldOfView(
    _Inout_ CExtendedFieldOfView    *pFieldOfView
)
{
    PAGED_CODE();
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    if( pFieldOfView->isValid() &&
            pFieldOfView->PinId == KSCAMERA_EXTENDEDPROP_FILTERSCOPE )
    {
        Status = m_Sensor->GetFieldOfView(pFieldOfView);
    }
    return Status;
}

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_CAMERAANGLEOFFSET
NTSTATUS
CCaptureFilter::
GetCameraAngleOffset(
    _Inout_ CExtendedCameraAngleOffset  *pAngleOffset
)
{
    PAGED_CODE();
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    if( pAngleOffset->isValid() &&
            pAngleOffset->PinId == KSCAMERA_EXTENDEDPROP_FILTERSCOPE )
    {
        Status = m_Sensor->GetCameraAngleOffset(pAngleOffset);
    }
    return Status;
}

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_METADATA
NTSTATUS
CCaptureFilter::
GetMetadata(
    _Inout_ CExtendedMetadata   *pMetadata
)
{
    PAGED_CODE();

    if( pMetadata->PinId >= m_Sensor->GetPinCount() )
    {
        return STATUS_INVALID_PARAMETER;
    }

    return m_Sensor->GetMetadata( pMetadata );
}

//  Set KSPROPERTY_CAMERACONTROL_EXTENDED_METADATA
NTSTATUS
CCaptureFilter::
SetMetadata(
    _In_    CExtendedMetadata   *pMetadata
)
{
    PAGED_CODE();
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    if( !(pMetadata->Capability & KSCAMERA_EXTENDEDPROP_CAPS_ASYNCCONTROL) &&
            pMetadata->isValid() &&
            m_Sensor->IsValidIndex( pMetadata->PinId ) )
    {
        if( m_pinArray[pMetadata->PinId] &&
                m_pinArray[pMetadata->PinId]->GetState() == PinRunning )
        {
            Status = STATUS_INVALID_DEVICE_REQUEST;
        }
        else
        {
            Status = m_Sensor->SetMetadata( pMetadata );
        }
    }
    return Status;
}

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_ISO
NTSTATUS
CCaptureFilter::
GetIso(
    _Inout_ CExtendedProperty  *pIso
)
{
    PAGED_CODE();
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
        
    if( pIso->isValid() &&
        m_Sensor->IsStillIndex( pIso->PinId ) )
    {
        Status = m_Sensor->GetIso(pIso);
    }

    DBG_LEAVE("(PinId=%d, Flags=0x%016llX, Cap=0x%016llX)=0x%08X",
              pIso->PinId, pIso->Flags, pIso->Capability, Status );
    return Status;
}

//  Set KSPROPERTY_CAMERACONTROL_EXTENDED_ISO
NTSTATUS
CCaptureFilter::
SetIso(
    _In_    CExtendedProperty  *pIso
)
{
    PAGED_CODE();
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    DBG_ENTER("(Flags=0x%016llX)", pIso->Flags);

    if( (pIso->Capability & KSCAMERA_EXTENDEDPROP_CAPS_ASYNCCONTROL) &&
            pIso->isValid() &&
            m_Sensor->IsStillIndex( pIso->PinId ) )
    {
        CExtendedProperty   Caps;
        Status = m_Sensor->GetIso( &Caps );

        if( NT_SUCCESS(Status) )
        {
            //  Assume an invalid parameter.
            Status = STATUS_INVALID_PARAMETER;

            if( pIso->Flags == (pIso->Flags & Caps.Capability) )
            {
                //  Make sure only one preset was set.
                ULONGLONG   presets = pIso->Flags &
                                      ( KSCAMERA_EXTENDEDPROP_ISO_AUTO |
                                        KSCAMERA_EXTENDEDPROP_ISO_50 | KSCAMERA_EXTENDEDPROP_ISO_80 |
                                        KSCAMERA_EXTENDEDPROP_ISO_100 | KSCAMERA_EXTENDEDPROP_ISO_200 |
                                        KSCAMERA_EXTENDEDPROP_ISO_400 | KSCAMERA_EXTENDEDPROP_ISO_800 |
                                        KSCAMERA_EXTENDEDPROP_ISO_1600 | KSCAMERA_EXTENDEDPROP_ISO_3200 |
                                        KSCAMERA_EXTENDEDPROP_ISO_6400 | KSCAMERA_EXTENDEDPROP_ISO_12800 |
                                        KSCAMERA_EXTENDEDPROP_ISO_25600 ) ;

                DBG_TRACE("presets=0x%016llX", presets);

                //  Check to see if there is exactly one set.
                if( presets && ( (presets-1) & presets ) == 0 )
                {
                    Status = INVOKE_SET_ASYNC( m_Sensor, Iso, pIso, m_IsoNotifier );
                }
            }
        }
    }

    DBG_LEAVE("(PinId=%d, Flags=0x%016llX, Cap=0x%016llX)=0x%08X",
              pIso->PinId, pIso->Flags, pIso->Capability, Status );

    return Status;
}

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_EVCOMPENSATION
NTSTATUS
CCaptureFilter::
GetEvCompensation(
    _Inout_ CExtendedEvCompensation *pEvComp
)
{
    PAGED_CODE();
    DBG_ENTER("()");

    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    if( pEvComp->isValid() &&
            pEvComp->PinId == KSCAMERA_EXTENDEDPROP_FILTERSCOPE )
    {
        Status = m_Sensor->GetEvCompensation( pEvComp );
    }

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

//  Set KSPROPERTY_CAMERACONTROL_EXTENDED_EVCOMPENSATION
NTSTATUS
CCaptureFilter::
SetEvCompensation(
    _In_    CExtendedEvCompensation *pEvComp
)
{
    PAGED_CODE();
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    DBG_ENTER("()");
    if( (pEvComp->Capability & KSCAMERA_EXTENDEDPROP_CAPS_ASYNCCONTROL) &&
            pEvComp->isValid() &&
            pEvComp->PinId == KSCAMERA_EXTENDEDPROP_FILTERSCOPE )
    {
        CExtendedEvCompensation Caps;
        Status = m_Sensor->GetEvCompensation( &Caps );

        if( NT_SUCCESS(Status) )
        {
            //  Assume an invalid parameter.
            Status = STATUS_INVALID_PARAMETER;

            if( (pEvComp->Flags & Caps.Capability ) == pEvComp->Flags )
            {
                Status = INVOKE_SET_ASYNC( m_Sensor, EvCompensation, pEvComp, m_EvCompensationNotifier );
            }
        }
    }

    DBG_LEAVE("(PinId=%d, Flags=0x%016llX, Cap=0x%016llX)=0x%08X",
              pEvComp->PinId, pEvComp->Flags, pEvComp->Capability, Status );
    return Status;
}

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_WHITEBALANCEMODE
NTSTATUS
CCaptureFilter::
GetWhiteBalance(
    _Inout_ CExtendedVidProcSetting *pWhiteBalance
)
{
    PAGED_CODE();
    DBG_ENTER("()");

    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    if( pWhiteBalance->isValid() &&
            pWhiteBalance->PinId == KSCAMERA_EXTENDEDPROP_FILTERSCOPE )
    {
        Status = m_Sensor->GetWhiteBalance( pWhiteBalance );
    }

    DBG_LEAVE("(PinId=%d, Flags=0x%016llX, Cap=0x%016llX, Mode=0x%08X, Val=0x%016llX)=0x%08X",
              pWhiteBalance->PinId, pWhiteBalance->Flags, pWhiteBalance->Capability,
              pWhiteBalance->Mode(), pWhiteBalance->GetLONGLONG(), Status );
    return Status;
}

//  Set KSPROPERTY_CAMERACONTROL_EXTENDED_WHITEBALANCEMODE
NTSTATUS
CCaptureFilter::
SetWhiteBalance(
    _In_    CExtendedVidProcSetting *pWhiteBalance
)
{
    PAGED_CODE();
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    DBG_ENTER("()");
    if( (pWhiteBalance->Capability & KSCAMERA_EXTENDEDPROP_CAPS_ASYNCCONTROL) &&
            pWhiteBalance->isValid() &&
            pWhiteBalance->PinId == KSCAMERA_EXTENDEDPROP_FILTERSCOPE )
    {
        CExtendedVidProcSetting Caps;
        Status = m_Sensor->GetWhiteBalance( &Caps );

        if( NT_SUCCESS(Status) )
        {
            if( (pWhiteBalance->Flags & Caps.Capability ) == pWhiteBalance->Flags )
            {
                switch( pWhiteBalance->Flags )
                {
                case KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO:
                    break;
                case KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO | KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_LOCK:
                case KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_LOCK:
                    pWhiteBalance->Flags = KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO | KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_LOCK;
                    break;

                case KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_MANUAL:
                    switch( pWhiteBalance->Mode() )
                    {
                    case KSCAMERA_EXTENDEDPROP_WHITEBALANCE_TEMPERATURE:
                        Status = Caps.BoundsCheck( pWhiteBalance->GetLONGLONG() );
                        break;

                    case KSCAMERA_EXTENDEDPROP_WHITEBALANCE_PRESET:
                        switch( pWhiteBalance->GetLONGLONG() )
                        {
                        case KSCAMERA_EXTENDEDPROP_WBPRESET_CLOUDY:
                        case KSCAMERA_EXTENDEDPROP_WBPRESET_DAYLIGHT:
                        case KSCAMERA_EXTENDEDPROP_WBPRESET_FLASH:
                        case KSCAMERA_EXTENDEDPROP_WBPRESET_FLUORESCENT:
                        case KSCAMERA_EXTENDEDPROP_WBPRESET_TUNGSTEN:
                        case KSCAMERA_EXTENDEDPROP_WBPRESET_CANDLELIGHT:
                            break;

                        default:
                            Status = STATUS_INVALID_PARAMETER;
                            break;
                        }
                        break;

                    default:
                        Status = STATUS_INVALID_PARAMETER;
                        break;
                    }
                    break;

                default:
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }

                if( NT_SUCCESS(Status) )
                {
                    Status = INVOKE_SET_ASYNC( m_Sensor, WhiteBalance, pWhiteBalance, m_WhiteBalanceNotifier );
                }
            }
            else
            {
                Status = STATUS_INVALID_PARAMETER;
            }
        }
    }

    DBG_LEAVE("(PinId=%d, Flags=0x%016llX, Cap=0x%016llX, Mode=0x%08X, Val=0x%016llX)=0x%08X",
              pWhiteBalance->PinId, pWhiteBalance->Flags, pWhiteBalance->Capability,
              pWhiteBalance->Mode(), pWhiteBalance->GetLONGLONG(), Status );
    return Status;
}

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_EXPOSUREMODE
NTSTATUS
CCaptureFilter::
GetExposure(
    _Inout_ CExtendedVidProcSetting *pExposure
)
{
    PAGED_CODE();
    DBG_ENTER("()");

    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    if( pExposure->isValid() &&
        pExposure->PinId == KSCAMERA_EXTENDEDPROP_FILTERSCOPE )
    {
        Status = m_Sensor->GetExposure( pExposure );
    }

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

//  Set KSPROPERTY_CAMERACONTROL_EXTENDED_EXPOSUREMODE
NTSTATUS
CCaptureFilter::
SetExposure(
    _In_    CExtendedVidProcSetting *pExposure
)
{
    PAGED_CODE();
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    DBG_ENTER("()");
    if( (pExposure->Capability & KSCAMERA_EXTENDEDPROP_CAPS_ASYNCCONTROL) &&
         pExposure->isValid() &&
         pExposure->PinId == KSCAMERA_EXTENDEDPROP_FILTERSCOPE )
    {
        CExtendedVidProcSetting Caps;
        Status = m_Sensor->GetExposure( &Caps );

        if( NT_SUCCESS(Status) )
        {
            if( (pExposure->Flags & Caps.Capability ) == pExposure->Flags )
            {
                if( pExposure->Flags == KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_MANUAL )
                {
                    Status = Caps.BoundsCheck( pExposure->GetULONGLONG() );
                }

                if( NT_SUCCESS(Status) )
                {
                    Status = INVOKE_SET_ASYNC( m_Sensor, Exposure, pExposure, m_ExposureNotifier );
                }
            }
            else
            {
                Status = STATUS_INVALID_PARAMETER;
            }
        }
    }

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_FOCUSMODE
NTSTATUS
CCaptureFilter::
GetFocus(
    _Inout_ CExtendedVidProcSetting *pFocus
)
{
    PAGED_CODE();
    DBG_ENTER("()");

    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    if( pFocus->isValid() &&
            pFocus->PinId == KSCAMERA_EXTENDEDPROP_FILTERSCOPE )
    {
        Status = m_Sensor->GetFocus( pFocus );
    }

    DBG_LEAVE("(FocusMode=0x%016llX)=0x%08X", pFocus->Flags, Status);
    return Status;
}

//  Set KSPROPERTY_CAMERACONTROL_EXTENDED_FOCUSMODE
NTSTATUS
CCaptureFilter::
SetFocus(
    _In_    CExtendedVidProcSetting *pFocus
)
{
    PAGED_CODE();

    NTSTATUS    Status = STATUS_INVALID_PARAMETER;
    ULONGLONG   Flags = pFocus->Flags;

    DBG_ENTER("(Preset Flags=0x%016llX)", Flags);

    CExtendedVidProcSetting Caps(*pFocus);

    //  This part is just setting validation.
    if( (pFocus->Capability & KSCAMERA_EXTENDEDPROP_CAPS_ASYNCCONTROL) &&
            pFocus->isValid()   &&
            pFocus->Mode() == 0 &&
            pFocus->PinId == KSCAMERA_EXTENDEDPROP_FILTERSCOPE )
    {
        Status = m_Sensor->GetFocus( &Caps );
    }

    if( NT_SUCCESS(Status) )
    {
        //  Make sure we have a valid combinations of Flags
        switch( Flags &
                (KSCAMERA_EXTENDEDPROP_FOCUS_MODE_MASK |
                 KSCAMERA_EXTENDEDPROP_FOCUS_MODE_ADVANCED_MASK) )
        {
        //  Auto-focus combinations:
        case KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO:
        case KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO |
                KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_LOCK:
        case KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO |
                KSCAMERA_EXTENDEDPROP_FOCUS_REGIONBASED:
        case KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO |
                KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_LOCK |
                KSCAMERA_EXTENDEDPROP_FOCUS_REGIONBASED:
        case KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO |
                KSCAMERA_EXTENDEDPROP_FOCUS_DRIVERFALLBACK_OFF:
        case KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO |
                KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_LOCK |
                KSCAMERA_EXTENDEDPROP_FOCUS_DRIVERFALLBACK_OFF:
        case KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO |
                KSCAMERA_EXTENDEDPROP_FOCUS_REGIONBASED  |
                KSCAMERA_EXTENDEDPROP_FOCUS_DRIVERFALLBACK_OFF:
        case KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO |
                KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_LOCK |
                KSCAMERA_EXTENDEDPROP_FOCUS_REGIONBASED  |
                KSCAMERA_EXTENDEDPROP_FOCUS_DRIVERFALLBACK_OFF:
            break;

        //  Continuous-focus combinations:
        case KSCAMERA_EXTENDEDPROP_FOCUS_CONTINUOUS:
        case KSCAMERA_EXTENDEDPROP_FOCUS_CONTINUOUS     |
                KSCAMERA_EXTENDEDPROP_FOCUS_CONTINUOUSLOCK:
        case KSCAMERA_EXTENDEDPROP_FOCUS_CONTINUOUS     |
                KSCAMERA_EXTENDEDPROP_FOCUS_REGIONBASED:
        case KSCAMERA_EXTENDEDPROP_FOCUS_CONTINUOUS     |
                KSCAMERA_EXTENDEDPROP_FOCUS_CONTINUOUSLOCK |
                KSCAMERA_EXTENDEDPROP_FOCUS_REGIONBASED:
        case KSCAMERA_EXTENDEDPROP_FOCUS_CONTINUOUS     |
                KSCAMERA_EXTENDEDPROP_FOCUS_DRIVERFALLBACK_OFF:
        case KSCAMERA_EXTENDEDPROP_FOCUS_CONTINUOUS     |
                KSCAMERA_EXTENDEDPROP_FOCUS_CONTINUOUSLOCK |
                KSCAMERA_EXTENDEDPROP_FOCUS_DRIVERFALLBACK_OFF:
        case KSCAMERA_EXTENDEDPROP_FOCUS_CONTINUOUS     |
                KSCAMERA_EXTENDEDPROP_FOCUS_REGIONBASED    |
                KSCAMERA_EXTENDEDPROP_FOCUS_DRIVERFALLBACK_OFF:
        case KSCAMERA_EXTENDEDPROP_FOCUS_CONTINUOUS     |
                KSCAMERA_EXTENDEDPROP_FOCUS_CONTINUOUSLOCK |
                KSCAMERA_EXTENDEDPROP_FOCUS_REGIONBASED    |
                KSCAMERA_EXTENDEDPROP_FOCUS_DRIVERFALLBACK_OFF:
            break;

        //  Manual stands alone.
        case KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_MANUAL:
            Status = BoundsCheckSigned(
                         pFocus->GetLONG(),
                         Caps.m_Setting );

            if( !NT_SUCCESS(Status) )
            {
                DBG_TRACE( "ERROR: Focus request out of bounds.  Value=%d", pFocus->GetLONG() );
            }
            break;

        //  Unlock must stand alone.
        //  Also note that by definition unlock is accepted regardless of our current state.
        case KSCAMERA_EXTENDEDPROP_FOCUS_UNLOCK:
            break;

        //  Lock can stand alone.
        case KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_LOCK:
            //  Make sure we are actually being asked during an auto or continuous.
            if( !(Caps.Flags &
                    (KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO | KSCAMERA_EXTENDEDPROP_FOCUS_CONTINUOUS)) )
            {
                //  This is an ambiguous case under the WinBlue DDI.  However,
                //  producing an error code here will cause a test failure.
                //  Instead we will silently consume the error and just skip this
                //  request.
                //
                //  Actually the WinBlue WinRT layer interprets a "Manual Preset" as
                //  a LOCK-only request when there is no value involved.  That is kind
                //  rediculous since the value is sent without considering the previous
                //  mode.  Instead, it thinks that just setting a LOCK-only mode is
                //  synonymous with "Manual at the current position."
                //
                //  Therefore, our interpretation will be, If we're in manual, we can
                //  just silently consume the entire request and do nothing.
                if( Caps.Flags & KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_MANUAL )
                {
                    Status = STATUS_INVALID_LOCK_SEQUENCE;
                }
                else
                {
                    Status = STATUS_INVALID_PARAMETER;
                }
            }
            break;

        //  Anything else is just wrong.
        default:
            Status = STATUS_INVALID_PARAMETER;
            break;
        }
    }

    //  Validate focus ranges.
    if( NT_SUCCESS(Status) )
    {
        //  If AUTO or CONTINUOUS is requested, we may support RANGE flags.
        if( (Flags &
             (KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO |
              KSCAMERA_EXTENDEDPROP_FOCUS_CONTINUOUS)) )
        {
            switch( Flags & KSCAMERA_EXTENDEDPROP_FOCUS_RANGE_MASK )
            {
            case KSCAMERA_EXTENDEDPROP_FOCUS_RANGE_MACRO:
            case KSCAMERA_EXTENDEDPROP_FOCUS_RANGE_NORMAL:
            case KSCAMERA_EXTENDEDPROP_FOCUS_RANGE_FULLRANGE:
            //  Depreciated flags are not supported:
            //case KSCAMERA_EXTENDEDPROP_FOCUS_RANGE_INFINITY:
            //case KSCAMERA_EXTENDEDPROP_FOCUS_RANGE_HYPERFOCAL:
            case 0:
                break;

            default:
                Status = STATUS_INVALID_PARAMETER;
                break;
            }
        }
        //  Otherwise no RANGE flags are permitted.
        else if( Flags & KSCAMERA_EXTENDEDPROP_FOCUS_RANGE_MASK )
        {
            Status = STATUS_INVALID_PARAMETER;
        }
    }

    //  Validate focus distances.
    if( NT_SUCCESS(Status) )
    {
        //  If MANUAL is requested, we may support DISTANCE flags.
        if( Flags & KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_MANUAL )
        {
            switch( Flags & KSCAMERA_EXTENDEDPROP_FOCUS_DISTANCE_MASK )
            {
            case KSCAMERA_EXTENDEDPROP_FOCUS_DISTANCE_INFINITY:
            case KSCAMERA_EXTENDEDPROP_FOCUS_DISTANCE_HYPERFOCAL:
            case KSCAMERA_EXTENDEDPROP_FOCUS_DISTANCE_NEAREST:
            case 0:
                break;

            default:
                Status = STATUS_INVALID_PARAMETER;
                break;
            }
        }
        //  Otherwise no DISTANCE flags are permitted.
        else if( Flags & KSCAMERA_EXTENDEDPROP_FOCUS_DISTANCE_MASK )
        {
            Status = STATUS_INVALID_PARAMETER;
        }
    }

    if( NT_SUCCESS(Status) )
    {
        Status = INVOKE_SET_ASYNC( m_Sensor, Focus, pFocus, m_FocusNotifier );
    }
    //  This is the failure case.  In this case, we don't change state; but
    //  if we were ASYNC, we need to signal the client that we completed.
    else
    {
        //  Don't need to set the event if this was [erroneously] an sync request.
        if( pFocus->Capability & KSCAMERA_EXTENDEDPROP_CAPS_ASYNCCONTROL )
        {
            m_FocusNotifier.Set();
        }

        //  Silently consume Lock after Manual requests:
        if( STATUS_INVALID_LOCK_SEQUENCE == Status )
        {
            Status = STATUS_SUCCESS;
        }
    }
    DBG_LEAVE("(Preset Flags=0x%016llX)=0x%08X", Flags, Status);
    return Status;
}

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_PHOTOCONFIRMATION
NTSTATUS
CCaptureFilter::
GetPhotoConfirmation(
    _Inout_ CExtendedProperty   *pConfirmation
)
{
    PAGED_CODE();
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    if( pConfirmation->isValid() &&
            pConfirmation->PinId == KSCAMERA_EXTENDEDPROP_FILTERSCOPE )
    {
        Status =  m_Sensor->GetPhotoConfirmation( pConfirmation );
    }
    return Status;
}

//  Set KSPROPERTY_CAMERACONTROL_EXTENDED_PHOTOCONFIRMATION
NTSTATUS
CCaptureFilter::
SetPhotoConfirmation(
    _In_    CExtendedProperty   *pConfirmation
)
{
    PAGED_CODE();
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    if( !(pConfirmation->Capability & KSCAMERA_EXTENDEDPROP_CAPS_ASYNCCONTROL) &&
            pConfirmation->isValid() &&
            pConfirmation->PinId == KSCAMERA_EXTENDEDPROP_FILTERSCOPE )
    {
        switch( pConfirmation->Flags )
        {
        case KSCAMERA_EXTENDEDPROP_PHOTOCONFIRMATION_ON:
        case KSCAMERA_EXTENDEDPROP_PHOTOCONFIRMATION_OFF:
            Status = m_Sensor->SetPhotoConfirmation( pConfirmation );
            break;
        }
    }
    return Status;
}

//  Get KSPROPERTY_CAMERACONTROL_PERFRAMESETTING_CAPABILITY
NTSTATUS
CCaptureFilter::GetPFSCaps(
    _In_    PIRP        pIrp,
    _In_    PKSPROPERTY pProperty,
    _Inout_ PVOID       pData
)
{
    PAGED_CODE();

    NT_ASSERT(pIrp);
    NT_ASSERT(pProperty);

    CCaptureFilter *pFilter = reinterpret_cast <CCaptureFilter *>(KsGetFilterFromIrp(pIrp)->Context);
    PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    ULONG ulOutputBufferLength = pIrpStack->Parameters.DeviceIoControl.OutputBufferLength;
    ULONG RequiredLength = 0;

    NTSTATUS Status =
        pFilter->m_Sensor->GetPfsCaps( nullptr, &RequiredLength );

    // Note: We may want to determine capabilities at runtime in the future.

    //  We only handle GETs
    if( NT_SUCCESS(Status) )
    {
        if (ulOutputBufferLength == 0)
        {
            pIrp->IoStatus.Information = RequiredLength;
            Status = STATUS_BUFFER_OVERFLOW;
        }
        else if (ulOutputBufferLength < RequiredLength)
        {
            Status = STATUS_BUFFER_TOO_SMALL;
        }
        else if (pData && ulOutputBufferLength >= RequiredLength)
        {
            Status =
                pFilter->m_Sensor->GetPfsCaps(
                    (KSCAMERA_PERFRAMESETTING_CAP_HEADER *) pData,
                    &RequiredLength );
            pIrp->IoStatus.Information = RequiredLength;
        }
        else
        {
            Status = STATUS_INVALID_PARAMETER;
        }
    }
    return Status;
}

//  Get KSPROPERTY_CAMERACONTROL_PERFRAMESETTING_SET
NTSTATUS
CCaptureFilter::GetPerFrameSettings(
    _In_    PIRP pIrp,
    _In_    PKSPROPERTY pProperty,
    _Inout_ PVOID pData
)
{
    PAGED_CODE();
    NTSTATUS ntStatus = STATUS_SUCCESS;

    NT_ASSERT(pIrp);
    NT_ASSERT(pProperty);

    CCaptureFilter *pFilter = reinterpret_cast <CCaptureFilter *>(KsGetFilterFromIrp(pIrp)->Context);
    PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    ULONG ulOutputBufferLength = pIrpStack->Parameters.DeviceIoControl.OutputBufferLength;
    ULONG ulOutputSize =
        pFilter->m_pPerFrameSettings ? pFilter->m_pPerFrameSettings->Size : 0;

    //  Only handle GETs
    if (ulOutputBufferLength == 0)
    {
        pIrp->IoStatus.Information = ulOutputSize;
        ntStatus = STATUS_BUFFER_OVERFLOW;
    }
    else if (ulOutputBufferLength < ulOutputSize)
    {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
    }
    else if (pData && ulOutputBufferLength >= ulOutputSize)
    {
        //  Return the cached copy.
        RtlCopyMemory( pData, pFilter->m_pPerFrameSettings, ulOutputSize );
        pIrp->IoStatus.Information = ulOutputSize;
        ntStatus = STATUS_SUCCESS;
    }
    else
    {
        ntStatus = STATUS_INVALID_PARAMETER;
    }

    return ntStatus;
}

//  Set KSPROPERTY_CAMERACONTROL_PERFRAMESETTING_SET
NTSTATUS
CCaptureFilter::
SetPerFrameSettings(
    _In_    PIRP pIrp,
    _In_    PKSPROPERTY pProperty,
    _Inout_ PVOID pData
)
{
    PAGED_CODE();
    NTSTATUS Status = STATUS_SUCCESS;

    NT_ASSERT(pIrp);
    NT_ASSERT(pProperty);

    CCaptureFilter *pFilter = reinterpret_cast <CCaptureFilter *>(KsGetFilterFromIrp(pIrp)->Context);
    PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    ULONG ulOutputBufferLength = pIrpStack->Parameters.DeviceIoControl.OutputBufferLength;

    //  Only handle SETs
    if (ulOutputBufferLength == 0)
    {
        //  This is pretty meaningless.
        //  We can't really tell the caller the size of this buffer.
        pIrp->IoStatus.Information =
            sizeof(KSCAMERA_PERFRAMESETTING_HEADER) +
            sizeof(KSCAMERA_PERFRAMESETTING_FRAME_HEADER) +
            sizeof(KSCAMERA_PERFRAMESETTING_ITEM_HEADER) ;
        Status = STATUS_BUFFER_OVERFLOW;
    }
    else if (ulOutputBufferLength < sizeof(KSCAMERA_PERFRAMESETTING_HEADER))
    {
        Status = STATUS_BUFFER_TOO_SMALL;
    }
    else
    {
        PKSCAMERA_PERFRAMESETTING_HEADER pPFS =
            (PKSCAMERA_PERFRAMESETTING_HEADER) pData ;
        ISP_FRAME_SETTINGS  *pSettings = nullptr;

        //  Parse, but don't save the PerFrameSettings
        Status = pFilter->ParsePFSBuffer( pPFS, ulOutputBufferLength, &pSettings );

        //  Cache a copy of the unparsed PFS for the GET.
        //  Pass the parsed settings down to the sensor.
        if( NT_SUCCESS(Status) )
        {
            delete [] ((PBYTE) pFilter->m_pPerFrameSettings);
            pFilter->m_pPerFrameSettings = (PKSCAMERA_PERFRAMESETTING_HEADER)
                                           new (PagedPool, '-SFP') BYTE [pPFS->Size];

            if( pFilter->m_pPerFrameSettings )
            {
                //  Cache a copy of the settings for the GET.
                RtlCopyMemory( pFilter->m_pPerFrameSettings, pPFS, pPFS->Size );

                //  Program the sensor with a copy of the parsed settings.
                //  The sensor object should copy what it needs.
                pFilter->m_Sensor->SetPFS( pSettings, pPFS->FrameCount, pPFS->LoopCount );
            }
            else
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        //  Delete the settings buffer ParsePFSBuffer created.
        delete [] pSettings;
    }

    return Status;
}

//  Set KSPROPERTY_CAMERACONTROL_PERFRAMESETTING_CLEAR
NTSTATUS
CCaptureFilter::ClearPerFrameSettings(
    _In_    PIRP pIrp,
    _In_    PKSPROPERTY pProperty,
    _Inout_ PVOID pData
)
{
    PAGED_CODE();
    NTSTATUS ntStatus = STATUS_SUCCESS;

    NT_ASSERT(pIrp);
    NT_ASSERT(pProperty);

    CCaptureFilter *pFilter = reinterpret_cast <CCaptureFilter *>(KsGetFilterFromIrp(pIrp)->Context);

    //  Only handle SETs
    delete [] ((PBYTE) pFilter->m_pPerFrameSettings);
    pFilter->m_pPerFrameSettings = nullptr;
    pFilter->m_Sensor->SetPFS( nullptr, 0, 0 );

    return ntStatus;
}

PKSCAMERA_PERFRAMESETTING_CAP_ITEM_HEADER
Find(
    _In_    PKSCAMERA_PERFRAMESETTING_CAP_HEADER Caps,
    _In_    ULONG                                Type
)
/*++

Routine Description:

    Helper function that finds a specific PFS capability.

Arguments:

    Caps -
        A list of PFS capabilities.
    Type -
        The capability type to search for.

Return Value:

    A pointer to the capability item header.

--*/
{
    PAGED_CODE();

    if( Caps )
    {
        PKSCAMERA_PERFRAMESETTING_CAP_ITEM_HEADER   Item =
            (PKSCAMERA_PERFRAMESETTING_CAP_ITEM_HEADER) (Caps+1);

        for( ULONG i=0; i<Caps->ItemCount; i++ )
        {
            if( Item->Type == Type )
            {
                return Item;
            }
            Item = (PKSCAMERA_PERFRAMESETTING_CAP_ITEM_HEADER) (((PBYTE) Item) + Item->Size);
        }
    }
    return nullptr;
}

//  This function can be used to validate PerFrameSettings, or
//  it can be used to populate a buffer containing PFS settings.
_Success_(return == 0)
NTSTATUS
CCaptureFilter::
ParsePFSBuffer(
    _In_reads_bytes_(BufferLimit)
    PKSCAMERA_PERFRAMESETTING_HEADER    pPFS,
    _In_    ULONG                       BufferLimit,
    _Outptr_opt_result_maybenull_
    ISP_FRAME_SETTINGS                **ppSettings
)
/*++

Routine Description:

    Helper function that parses Per-Frame Settings.

    This function validates the API's PerFrameSettings list and optionally 
    returns an array of ISP_FRAME_SETTINGS for each frame.

    Any returned array of ISP_FRAME_SETTINGS must be deleted by the caller.

Arguments:

    pPFS -
        A variable length list of per frame settings.
    BufferLimit -
        The maximum amount of space, in bytes, used by the PFS list.
    ppSettings -
        An optional pointer to an array of frame settings to be populated.

Return Value:

    Success / failure

--*/
{
    PAGED_CODE();
    NTSTATUS Status = STATUS_SUCCESS;

    DBG_ENTER("[%S]()", GetFilterName());

    NT_ASSERT(pPFS);
    NT_ASSERT(BufferLimit!=0);

    if( !pPFS ||
            BufferLimit == 0 ||
            pPFS->Size > BufferLimit ||
            pPFS->Size < sizeof(KSCAMERA_PERFRAMESETTING_HEADER) ||
            pPFS->FrameCount > MAX_FRAME_COUNT ||
            pPFS->FrameCount == 0 )
    {
        Status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        ULONG i=0 ;
        ISP_FRAME_SETTINGS *pSettings = nullptr;
        LPVOID  pEnd = ((LPBYTE) pPFS) + BufferLimit ;
        PKSCAMERA_PERFRAMESETTING_FRAME_HEADER  pFrame =
            (PKSCAMERA_PERFRAMESETTING_FRAME_HEADER) &pPFS[1];

        //  Make sure we have at least one frame in the array.
        ULONG FrameCount = pPFS->FrameCount ? pPFS->FrameCount : 1 ;

        //  Query the PFS Caps from the Sensor
        PKSCAMERA_PERFRAMESETTING_CAP_HEADER    Caps = nullptr;
        ULONG                                   RequiredSize = 0;
        Status = m_Sensor->GetPfsCaps( nullptr, &RequiredSize );

        if( NT_SUCCESS(Status) )
        {
            Caps = (PKSCAMERA_PERFRAMESETTING_CAP_HEADER) new (PagedPool, '-SFP') BYTE[RequiredSize];

            if( Caps )
            {
                RtlZeroMemory( Caps, RequiredSize );
                Status = m_Sensor->GetPfsCaps( Caps, &RequiredSize );
            }
            else
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        if( NT_SUCCESS(Status) )
        {
            //  Allocate an array
            pSettings = new (NonPagedPoolNx, '-SFP') ISP_FRAME_SETTINGS[FrameCount];
            if( !pSettings )
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
            else
            {
                //  Initialize the array to our globals
                pSettings[0] = *m_Sensor->GetGlobalIspSettings();
                for( i=1; i<FrameCount; i++ )
                {
                    pSettings[i] = pSettings[0];
                }

                while( ((pFrame+1)<=pEnd) )
                {
                    PKSCAMERA_PERFRAMESETTING_FRAME_HEADER  pNextFrame =
                        (PKSCAMERA_PERFRAMESETTING_FRAME_HEADER)
                        (((LPBYTE)pFrame) + pFrame->Size);
                    PKSCAMERA_PERFRAMESETTING_ITEM_HEADER pItem =
                        (PKSCAMERA_PERFRAMESETTING_ITEM_HEADER) &pFrame[1];

                    //  Verify that the frame is within the buffer.
                    if( pFrame->Id >= pPFS->FrameCount ||
                            pNextFrame > pEnd ||
                            pFrame->Size < sizeof(KSCAMERA_PERFRAMESETTING_FRAME_HEADER) )
                    {
                        DBG_TRACE( "Malformed PFS Buffer.  pPFS=%p pFrame=%p",
                                   pPFS, pFrame );

                        Status=STATUS_INVALID_PARAMETER;
                        break;
                    }

                    DBG_TRACE("Frame Id=%d",pFrame->Id);

                    ISP_FRAME_SETTINGS &FrameSetting = pSettings[pFrame->Id] ;

                    //  Scan for at most n Items regardless.
                    for( i=0;
                            i < pFrame->ItemCount &&
                            (pItem+1) <= (PVOID)pNextFrame;
                            i++ )
                    {
                        PKSCAMERA_PERFRAMESETTING_ITEM_HEADER  pNextItem =
                            (PKSCAMERA_PERFRAMESETTING_ITEM_HEADER)
                            (((LPBYTE)pItem) + pItem->Size);

                        PKSCAMERA_PERFRAMESETTING_CAP_ITEM_HEADER   ItemCap =
                            Find( Caps, pItem->Type );

                        //  Verify that the item is within the frame ...
                        //  ... and a supported capability.
                        if( pNextItem>(LPVOID)pNextFrame ||
                                pItem->Size<sizeof(KSCAMERA_PERFRAMESETTING_ITEM_HEADER) ||
                                !ItemCap )
                        {
                            Status=STATUS_INVALID_PARAMETER;
                            break;
                        }

                        switch( pItem->Type )
                        {
                        case KSCAMERA_PERFRAMESETTING_ITEM_EXPOSURE_TIME:
                            DBG_TRACE("KSCAMERA_PERFRAMESETTING_ITEM_EXPOSURE_TIME");

                            FrameSetting.ExposureMode =
                                TranslatePFS2VideoProcFlags( pItem->Flags );

                            if( pItem->Flags & KSCAMERA_PERFRAMESETTING_MANUAL )
                            {
                                if( pItem->Size==
                                        sizeof(KSCAMERA_PERFRAMESETTING_ITEM_HEADER)+
                                        sizeof(KSCAMERA_EXTENDEDPROP_VALUE) )
                                {
                                    FrameSetting.ExposureSetting.VideoProc.Value =
                                        ((PKSCAMERA_EXTENDEDPROP_VALUE) &(pItem[1]))->Value;
                                    //  Make sure the setting is in range.
                                    Status =
                                        BoundsCheckSigned(
                                            FrameSetting.ExposureSetting.VideoProc.Value.ll,
                                            ((SOC_CAP_WITH_STEPPING_LONGLONG *) ItemCap)->Stepping );
                                    DBG_TRACE("Status=0x%08X, ExposureMode=KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_MANUAL, ExposureTime=%lldns, element=%d",
                                              Status, FrameSetting.ExposureSetting.VideoProc.Value.ll, i);
                                }
                                else
                                {
                                    Status=STATUS_INVALID_PARAMETER;
                                    DBG_TRACE("Status=STATUS_INVALID_PARAMETER");
                                }
                            }
                            else if( pItem->Size!=sizeof(KSCAMERA_PERFRAMESETTING_ITEM_HEADER) )
                            {
                                Status=STATUS_INVALID_PARAMETER;
                                DBG_TRACE("Status=STATUS_INVALID_PARAMETER");
                            }
                            break;

                        case KSCAMERA_PERFRAMESETTING_ITEM_FLASH:
                            DBG_TRACE("KSCAMERA_PERFRAMESETTING_ITEM_FLASH");
                            FrameSetting.FlashMode = pItem->Flags;
                            if( pItem->Flags &
                                    (KSCAMERA_EXTENDEDPROP_FLASH_ON_ADJUSTABLEPOWER |
                                     KSCAMERA_EXTENDEDPROP_FLASH_AUTO_ADJUSTABLEPOWER) )
                            {
                                if( pItem->Size==
                                        sizeof(KSCAMERA_PERFRAMESETTING_ITEM_HEADER)+
                                        sizeof(KSCAMERA_EXTENDEDPROP_VALUE) )
                                {
                                    FrameSetting.FlashValue =
                                        ((PKSCAMERA_EXTENDEDPROP_VALUE) &(pItem[1]))->Value.ul;

                                    //  Make sure the setting is in range.
                                    if( FrameSetting.FlashValue > 100 )
                                    {
                                        Status=STATUS_INVALID_PARAMETER;
                                    }
                                    else
                                    {
                                        DBG_TRACE("FlashPower (%d%) accepted...", FrameSetting.FlashValue);
                                    }
                                }
                                else
                                {
                                    Status=STATUS_INVALID_PARAMETER;
                                }
                            }
                            else if( pItem->Size!=sizeof(KSCAMERA_PERFRAMESETTING_ITEM_HEADER) )
                            {
                                Status=STATUS_INVALID_PARAMETER;
                            }

                            if( NT_SUCCESS(Status) )
                            {
                                DBG_TRACE("FlashMode=0x%016llX, FlashPower=%d", FrameSetting.FlashMode, FrameSetting.FlashValue);
                            }
                            break;

                        case KSCAMERA_PERFRAMESETTING_ITEM_EXPOSURE_COMPENSATION:
                            DBG_TRACE("KSCAMERA_PERFRAMESETTING_ITEM_EXPOSURE_COMPENSATION");
                            {
                                LONG    Denominator = EVFlags2Denominator( pItem->Flags );
                                KSPROPERTY_STEPPING_LONG
                                Bounds = ((SOC_CAP_WITH_STEPPING *) ItemCap)->Stepping;

                                //  Adjust the bounds for the step size... (A quirk of EV)
                                Bounds.Bounds.SignedMinimum *= Denominator;
                                Bounds.Bounds.SignedMaximum *= Denominator;

                                FrameSetting.EVCompensation.Mode = (ULONG)
                                                                   pItem->Flags & ~(KSCAMERA_PERFRAMESETTING_AUTO);

                                //  Validate manual setting
                                if( (pItem->Flags != KSCAMERA_PERFRAMESETTING_AUTO) )
                                {
                                    if( pItem->Size==
                                            sizeof(KSCAMERA_PERFRAMESETTING_ITEM_HEADER)+
                                            sizeof(KSCAMERA_EXTENDEDPROP_VALUE) &&
                                            (Denominator != 0) )
                                    {
                                        FrameSetting.EVCompensation.Value =
                                            ((PKSCAMERA_EXTENDEDPROP_VALUE) &(pItem[1]))->Value.l;

                                        //  Make sure the setting is in range.
                                        Status =
                                            BoundsCheckSigned(
                                                FrameSetting.EVCompensation.Value,
                                                Bounds );
                                    }
                                    else
                                    {
                                        Status=STATUS_INVALID_PARAMETER;
                                    }
                                }
                                //  Auto can be the only flag, if auto is enabled.
                                else //if( pItem->Flags == KSCAMERA_PERFRAMESETTING_AUTO )
                                {
                                    if( pItem->Size==sizeof(KSCAMERA_PERFRAMESETTING_ITEM_HEADER) )
                                    {
                                        static
                                        ULONG step[] =
                                        {
                                            0,
                                            KSCAMERA_EXTENDEDPROP_EVCOMP_FULLSTEP,      //1
                                            KSCAMERA_EXTENDEDPROP_EVCOMP_HALFSTEP,      //2
                                            KSCAMERA_EXTENDEDPROP_EVCOMP_THIRDSTEP,     //3
                                            KSCAMERA_EXTENDEDPROP_EVCOMP_QUARTERSTEP,   //4
                                            0,                                          //5
                                            KSCAMERA_EXTENDEDPROP_EVCOMP_SIXTHSTEP      //6
                                        };

                                        //  Work back from the randomly selected denominator ...
                                        /// ... to the step flag.
                                        FrameSetting.EVCompensation.Mode = step[Denominator];

                                        //  Now pick a "random" numerator that is in range.
                                        FrameSetting.EVCompensation.Value = (LONG)
                                                                            GetRandom(  Bounds.Bounds.SignedMinimum,
                                                                                    Bounds.Bounds.SignedMaximum );
                                    }
                                    else
                                    {
                                        Status=STATUS_INVALID_PARAMETER;
                                    }
                                }
                            }
                            break;

                        case KSCAMERA_PERFRAMESETTING_ITEM_ISO:
                            DBG_TRACE("KSCAMERA_PERFRAMESETTING_ITEM_ISO: Flags=0x%016llX", pItem->Flags);

                            FrameSetting.ISOMode = pItem->Flags;
                            //  Handle Auto.
                            if( pItem->Flags == KSCAMERA_EXTENDEDPROP_ISO_AUTO )
                            {
                                if( pItem->Size==sizeof(KSCAMERA_PERFRAMESETTING_ITEM_HEADER) )
                                {
                                    FrameSetting.ISOValue = 0;
                                }
                                else
                                {
                                    Status=STATUS_INVALID_PARAMETER;
                                }
                            }
                            //  Handle manual settings.
                            else if( pItem->Flags == KSCAMERA_EXTENDEDPROP_ISO_MANUAL )
                            {
                                if( pItem->Size==
                                        sizeof(KSCAMERA_PERFRAMESETTING_ITEM_HEADER)+
                                        sizeof(KSCAMERA_EXTENDEDPROP_VALUE) )
                                {
                                    FrameSetting.ISOValue =
                                        ((PKSCAMERA_EXTENDEDPROP_VALUE) &(pItem[1]))->Value.ul;
                                    //  Make sure the setting is in range.
                                    Status =
                                        BoundsCheckUnsigned(
                                            FrameSetting.ISOValue,
                                            ((SOC_CAP_WITH_STEPPING *)ItemCap)->Stepping );

                                    DBG_TRACE("ISOValue=%d", FrameSetting.ISOValue);
                                }
                                else
                                {
                                    Status=STATUS_INVALID_PARAMETER;
                                }
                            }
                            else
                            {
                                Status=STATUS_INVALID_PARAMETER;
                            }
                            break;

                        case KSCAMERA_PERFRAMESETTING_ITEM_FOCUS:
                            DBG_TRACE("KSCAMERA_PERFRAMESETTING_ITEM_FOCUS");
                            if( pItem->Flags == KSCAMERA_PERFRAMESETTING_MANUAL )
                            {
                                if( pItem->Size==
                                        sizeof(KSCAMERA_PERFRAMESETTING_ITEM_HEADER)+
                                        sizeof(KSCAMERA_EXTENDEDPROP_VALUE) )
                                {
                                    FrameSetting.FocusMode =
                                        KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_MANUAL;
                                    FrameSetting.FocusSetting.VideoProc =
                                        *((PKSCAMERA_EXTENDEDPROP_VALUE) &(pItem[1]));
                                    //  Make sure the setting is in range.
                                    Status =
                                        BoundsCheckUnsigned(
                                            FrameSetting.FocusSetting.VideoProc.Value.ul,
                                            ((SOC_CAP_WITH_STEPPING *)ItemCap)->Stepping );
                                }
                                else
                                {
                                    Status=STATUS_INVALID_PARAMETER;
                                }
                            }
                            else
                            {
                                Status=STATUS_INVALID_PARAMETER;
                            }
                            break;

                        case KSCAMERA_PERFRAMESETTING_ITEM_PHOTOCONFIRMATION:
                            DBG_TRACE("KSCAMERA_PERFRAMESETTING_ITEM_PHOTOCONFIRMATION");

                            if( pItem->Size!=sizeof(KSCAMERA_PERFRAMESETTING_ITEM_HEADER) )
                            {
                                Status=STATUS_INVALID_PARAMETER;
                            }
                            else
                            {
                                switch( pItem->Flags )
                                {
                                case KSCAMERA_EXTENDEDPROP_PHOTOCONFIRMATION_ON:
                                    FrameSetting.bPhotoConfirmation = TRUE;
                                    DBG_TRACE("PFS Photo Confirmation ON for frame #%d", pFrame->Id);
                                    break;

                                case KSCAMERA_EXTENDEDPROP_PHOTOCONFIRMATION_OFF:
                                    FrameSetting.bPhotoConfirmation = FALSE;
                                    DBG_TRACE("PFS Photo Confirmation OFF for frame #%d", pFrame->Id);
                                    break;

                                default:
                                    Status=STATUS_INVALID_PARAMETER;
                                    break;
                                }
                            }
                            break;

                        case KSCAMERA_PERFRAMESETTING_ITEM_CUSTOM:
                            DBG_TRACE("KSCAMERA_PERFRAMESETTING_ITEM_CUSTOM");
                            break;

                        default:
                            //  We don't understand this item.  Do nothing.
                            NT_ASSERT(FALSE);
                            break;
                        }

                        //  Bail here (a little early) if any of the parsing failed.
                        if( !NT_SUCCESS(Status) )
                        {
                            break;
                        }

                        //  Move to the next Item
                        pItem = pNextItem;
                    }

                    //  Bail here (a little early) if any of the parsing failed.
                    if( !NT_SUCCESS(Status) )
                    {
                        DBG_TRACE( "Malformed PFS Buffer.  pPFS=%p pItem=%p",
                                   pPFS, pItem );
                        break;
                    }

                    //  Move to next Frame
                    pFrame = pNextFrame;
                }

                //  We need to log where in the buffer we encountered an error.
                //  For now we just emit info to the debugger.
            }

            if( NT_SUCCESS(Status) && ppSettings )
            {
                *ppSettings = pSettings;
            }
            else
            {
                delete [] pSettings;
            }
        }

        delete [] ((PBYTE) Caps);
    }

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_FOCUSSTATE
NTSTATUS
CCaptureFilter::GetFocusState(
    _Inout_ CExtendedProperty  *pState
)
{
    PAGED_CODE();

    NTSTATUS    Status = STATUS_INVALID_PARAMETER;

    if( pState->isValid() &&
            pState->PinId == KSCAMERA_EXTENDEDPROP_FILTERSCOPE )
    {
        KSCAMERA_EXTENDEDPROP_FOCUSSTATE    FocusState = KSCAMERA_EXTENDEDPROP_FOCUSSTATE_UNINITIALIZED;
        Status = m_Sensor->GetFocusState( &FocusState );
        if( NT_SUCCESS(Status) )
        {
            *pState = CExtendedProperty( (ULONGLONG) FocusState );
            pState->Capability = 0;
        }
    }
    DBG_LEAVE("(PinId=%u, Flags=0x%016llX, Version=%u)=0x%08X",
              pState->PinId, pState->Flags, pState->Version, Status);
    return Status;
}

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_FOCUSPRIORITY
NTSTATUS
CCaptureFilter::
GetFocusPriority(
    _Inout_ CExtendedProperty  *pPriority
)
{
    PAGED_CODE();

    NTSTATUS    Status = STATUS_INVALID_PARAMETER;

    if( pPriority->isValid() &&
            pPriority->PinId == KSCAMERA_EXTENDEDPROP_FILTERSCOPE )
    {
        Status = m_Sensor->GetFocusPriority( pPriority );
    }
    DBG_LEAVE("(PinId=%u, Flags=0x%016llX, Version=%u)=0x%08X",
              pPriority->PinId, pPriority->Flags, pPriority->Version, Status);
    return Status;
}

//  Set KSPROPERTY_CAMERACONTROL_EXTENDED_FOCUSPRIORITY
NTSTATUS
CCaptureFilter::
SetFocusPriority(
    _In_    CExtendedProperty  *pPriority
)
{
    PAGED_CODE();

    NTSTATUS    Status = STATUS_INVALID_PARAMETER;

    if( pPriority->isValid() &&
            pPriority->Capability == 0 &&
            pPriority->PinId == KSCAMERA_EXTENDEDPROP_FILTERSCOPE &&
            (pPriority->Flags & ~KSCAMERA_EXTENDEDPROP_FOCUSPRIORITY_ON) == 0 )
    {
        Status = m_Sensor->SetFocusPriority( pPriority );
    }
    DBG_LEAVE("(PinId=%u, Flags=0x%016llX, Version=%u)=0x%08X",
              pPriority->PinId, pPriority->Flags, pPriority->Version, Status);
    return Status;
}

//  KSPROPERTY_CAMERACONTROL_EXTENDED_ROI_CONFIGCAPS Get handler
NTSTATUS
CCaptureFilter::
GetRoiConfigCaps(
    _Inout_ CRoiConfig  *pOutput
)
{
    PAGED_CODE();

    NTSTATUS    Status = STATUS_INVALID_PARAMETER;

    if( pOutput->isValid() &&
            pOutput->PinId == KSCAMERA_EXTENDEDPROP_FILTERSCOPE )
    {
        Status = m_Sensor->GetRoiConfigCaps( pOutput );
    }
    return Status;
}

//  KSPROPERTY_CAMERACONTROL_EXTENDED_ROI_ISPCONTROL Get handler
//
//  Note: ROI handlers are left as static member functions because
//        it wasn't worth the code needed to generalize handling of a
//        variable length parameter.
NTSTATUS
CCaptureFilter::
GetRoiIspControl(
    _In_    PIRP pIrp,
    _In_    PKSPROPERTY pProperty,
    _Inout_ PVOID pData
)
{
    PAGED_CODE();
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    DBG_ENTER("()");

    NT_ASSERT(pIrp);
    NT_ASSERT(pProperty);

    CCaptureFilter *pFilter = reinterpret_cast <CCaptureFilter *>(KsGetFilterFromIrp(pIrp)->Context);
    CSensor *pSensor = pFilter->m_Sensor;
    PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    ULONG ulOutputBufferLength = pIrpStack->Parameters.DeviceIoControl.OutputBufferLength;
    CRoiProperty *pRoi = (reinterpret_cast<CRoiProperty *>(pData));
    ULONG RequiredSize = pSensor->SizeOfRoi();

    //  Only handle GETs
    if( ulOutputBufferLength == 0 )
    {
        pIrp->IoStatus.Information = RequiredSize;;
        Status = STATUS_BUFFER_OVERFLOW;
    }
    else if( ulOutputBufferLength < RequiredSize )
    {
        Status = STATUS_BUFFER_TOO_SMALL;
    }
    else if( pData )
    {
        Status = pSensor->GetRoi( pRoi );
        if( NT_SUCCESS(Status) )
        {
            pIrp->IoStatus.Information = pRoi->GetSize();
        }
    }

    DBG_LEAVE("()=0x%08X",Status);
    return Status;
}

//  KSPROPERTY_CAMERACONTROL_EXTENDED_ROI_ISPCONTROL Set handler
NTSTATUS
CCaptureFilter::
SetRoiIspControl(
    _In_    PIRP pIrp,
    _In_    PKSPROPERTY pProperty,
    _Inout_ PVOID pData
)
{
    PAGED_CODE();
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    DBG_ENTER("()");
    NT_ASSERT(pIrp);
    NT_ASSERT(pProperty);

    CCaptureFilter *pFilter = reinterpret_cast <CCaptureFilter *>(KsGetFilterFromIrp(pIrp)->Context);
    CSensor *pSensor = pFilter->m_Sensor;
    PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    ULONG ulOutputBufferLength = pIrpStack->Parameters.DeviceIoControl.OutputBufferLength;
    CRoiProperty *pRoi = (reinterpret_cast<CRoiProperty *>(pData));
    ULONG RequiredSize = sizeof(*pRoi);

    //  Only handle SETs
    if( ulOutputBufferLength == 0 )
    {
        //  Minimum size for this control.
        pIrp->IoStatus.Information = RequiredSize;
        Status = STATUS_BUFFER_OVERFLOW;
    }
    else if( ulOutputBufferLength < RequiredSize )
    {
        DBG_TRACE("***BUFFER TOO SMALL*** ulOutputBufferLength=%d, Size=%d",
                  ulOutputBufferLength, pRoi->Size );
        Status = STATUS_BUFFER_TOO_SMALL;
    }
    else if( pData && ulOutputBufferLength >= pRoi->Size )
    {
        //  Validate the data blob.
        if( pRoi->isValid() &&
            pRoi->PinId == KSCAMERA_EXTENDEDPROP_FILTERSCOPE &&
            pRoi->Flags == 0 )
        {
            Status = INVOKE_SET_ASYNC( pSensor, Roi, pRoi, pFilter->m_RoiNotifier );
        }
    }

    DBG_LEAVE("()=0x%08X",Status);
    return Status;
}

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_ISO_ADVANCED.
NTSTATUS
CCaptureFilter::
GetIsoAdvanced(
    _Inout_ CExtendedVidProcSetting *pIso
)
{
    PAGED_CODE();
    DBG_ENTER("()");

    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    if( pIso->isValid() &&
        m_Sensor->IsStillIndex(pIso->PinId) )
    {
        Status = m_Sensor->GetIsoAdvanced( pIso );
    }

    //  Report to the debug output...
    DBG_TRACE("          Flags = 0x%016llX", pIso->Flags);
    DBG_TRACE("            Min = %d", pIso->m_Setting.Min);
    DBG_TRACE("            Max = %d", pIso->m_Setting.Max);
    DBG_TRACE("           Step = %d", pIso->m_Setting.Step);
    DBG_TRACE("VideoProc.Value = %lld", pIso->m_Setting.VideoProc.Value.ll);

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

//  Set KSPROPERTY_CAMERACONTROL_EXTENDED_ISO_ADVANCED.
NTSTATUS
CCaptureFilter::
SetIsoAdvanced(
    _In_    CExtendedVidProcSetting *pIso
)
{
    PAGED_CODE();
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    DBG_ENTER("()");

    //  Report to the debug output...
    DBG_TRACE("          Flags = 0x%016llX", pIso->Flags);
    DBG_TRACE("            Min = %d", pIso->m_Setting.Min);
    DBG_TRACE("            Max = %d", pIso->m_Setting.Max);
    DBG_TRACE("           Step = %d", pIso->m_Setting.Step);
    DBG_TRACE("VideoProc.Value = %lld", pIso->m_Setting.VideoProc.Value.ll);

    //  Validate
    switch( pIso->Flags )
    {
    case KSCAMERA_EXTENDEDPROP_ISO_AUTO:
    case KSCAMERA_EXTENDEDPROP_FLAG_CANCELOPERATION:
    case KSCAMERA_EXTENDEDPROP_ISO_AUTO   | KSCAMERA_EXTENDEDPROP_FLAG_CANCELOPERATION:
    case KSCAMERA_EXTENDEDPROP_ISO_MANUAL:
    case KSCAMERA_EXTENDEDPROP_ISO_MANUAL | KSCAMERA_EXTENDEDPROP_FLAG_CANCELOPERATION:
        if( pIso->isValid() &&
                (pIso->Capability & KSCAMERA_EXTENDEDPROP_CAPS_ASYNCCONTROL) &&
                m_Sensor->IsStillIndex(pIso->PinId) )
        {
            CExtendedVidProcSetting    Caps;
            Status = m_Sensor->GetIsoAdvanced(&Caps);

            if( NT_SUCCESS(Status) )
            {
                //  Just save the parameters
                if( pIso->Flags & KSCAMERA_EXTENDEDPROP_ISO_MANUAL )
                {
                    Status = BoundsCheckSigned( pIso->GetLONGLONG(), Caps.m_Setting );
                }
                if( NT_SUCCESS(Status) )
                {
                    Status = INVOKE_SET_ASYNC( m_Sensor, IsoAdvanced, pIso, m_IsoAdvancedNotifier )
                }
            }
            else
            {
                Status = STATUS_INVALID_PARAMETER;
            }
        }
        break;
    }

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_VFR.
NTSTATUS
CCaptureFilter::
GetVFR(
    _Inout_ CExtendedProperty   *pVFR
)
{
    PAGED_CODE();

    NTSTATUS    Status = STATUS_INVALID_PARAMETER;

    if( m_Sensor->IsVideoIndex(pVFR->PinId) &&
            pVFR->isValid() )
    {
        //Get the current VFR state
        Status = m_Sensor->GetVFR( pVFR );
    }
    DBG_LEAVE("PinId=%u, VideoIndex=%u, Flags=0x%016llX, Version=%u, Status=0x%08X",
        pVFR->PinId, m_Sensor->GetVideoIndex(), pVFR->Flags, pVFR->Version, Status);
    return Status;
}

//  Set KSPROPERTY_CAMERACONTROL_EXTENDED_VFR.
NTSTATUS
CCaptureFilter::
SetVFR(
    _In_    CExtendedProperty   *pVFR
)
{
    PAGED_CODE();

    NTSTATUS    Status = STATUS_INVALID_PARAMETER;

    if( m_Sensor->IsVideoIndex(pVFR->PinId) &&
            pVFR->isValid() &&
            pVFR->Capability == 0 &&
            (KSCAMERA_EXTENDEDPROP_VFR_OFF == pVFR->Flags ||
             KSCAMERA_EXTENDEDPROP_VFR_ON  == pVFR->Flags ||
             KSCAMERA_EXTENDEDPROP_VIDEOHDR_AUTO == pVFR->Flags ) )
    {
        //Set the current VFR state
        Status = m_Sensor->SetVFR( pVFR );
    }
    DBG_LEAVE("PinId=%u, VideoIndex=%u, Flags=0x%016llX, Version=%u, Status=0x%08X",
        pVFR->PinId, m_Sensor->GetVideoIndex(), pVFR->Flags, pVFR->Version, Status);
    return Status;
}

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_VIDEOHDR.
NTSTATUS
CCaptureFilter::
GetVideoHDR(
    _Inout_ CExtendedProperty   *pVideoHDR
)
{
    PAGED_CODE();

    NTSTATUS    Status = STATUS_INVALID_PARAMETER;

    if( m_Sensor->IsVideoIndex(pVideoHDR->PinId) &&
            pVideoHDR->isValid() )
    {
        //Get the current Video HDR state
        Status = m_Sensor->GetVideoHDR(pVideoHDR);
    }
    DBG_LEAVE("PinId=%u, VideoIndex=%u, Flags=0x%016llX, Version=%u, Status=0x%08X",
        pVideoHDR->PinId, m_Sensor->GetVideoIndex(), pVideoHDR->Flags, pVideoHDR->Version, Status);
    return Status;
}

//  Set KSPROPERTY_CAMERACONTROL_EXTENDED_VIDEOHDR.
NTSTATUS
CCaptureFilter::
SetVideoHDR(
    _In_    CExtendedProperty   *pVideoHDR
)
{
    PAGED_CODE();

    NTSTATUS    Status = STATUS_INVALID_PARAMETER;

    // Call must come for Video PIN only and flags must be supported and mutually exclusive and version must be 1
    if( m_Sensor->IsVideoIndex(pVideoHDR->PinId) &&
            pVideoHDR->isValid() &&
            !(pVideoHDR->Capability & KSCAMERA_EXTENDEDPROP_CAPS_ASYNCCONTROL) &&
            (KSCAMERA_EXTENDEDPROP_VIDEOHDR_OFF == pVideoHDR->Flags ||
             KSCAMERA_EXTENDEDPROP_VIDEOHDR_ON  == pVideoHDR->Flags ||
             KSCAMERA_EXTENDEDPROP_VIDEOHDR_AUTO== pVideoHDR->Flags) )
    {
        if( m_pinArray[pVideoHDR->PinId]->GetState() == PinStopped )
        {
            CExtendedProperty   Caps(*pVideoHDR);
            Status = m_Sensor->GetVideoHDR( &Caps );

            if( NT_SUCCESS(Status) )
            {
                //  Assume an invalid parameter.
                Status = STATUS_INVALID_PARAMETER;

                // Call must come for Video PIN only and flags must be supported and mutually exclusive and version must be 1
                if( pVideoHDR->Flags == (pVideoHDR->Flags & Caps.Capability) )
                {
                    Status = m_Sensor->SetVideoHDR(pVideoHDR);
                }
            }
        }
        else
        {
            Status = STATUS_INVALID_DEVICE_STATE;
        }
    }

    DBG_LEAVE("PinId=%u, VideoIndex=%u, Flags=0x%016llX, Version=%u, Status=0x%08X",
        pVideoHDR->PinId, m_Sensor->GetVideoIndex(), pVideoHDR->Flags, pVideoHDR->Version, Status);
    return Status;
}


//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_HISTOGRAM.
NTSTATUS
CCaptureFilter::
GetHistogram(
    _Inout_ CExtendedProperty   *pHistogram
)
{
    PAGED_CODE();
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    if( m_Sensor->IsPreviewIndex(pHistogram->PinId) &&
            pHistogram->isValid() &&
            pHistogram->Capability == 0 )
    {
        //Get the histogram state
        Status = m_Sensor->GetHistogram( pHistogram );
    }
    DBG_TRACE("pHistogram->PinId = %u, m_sensor->PreviewIndex = %u, pHistogram->Flags = %016llx, pHistogram->Version = %u",
              pHistogram->PinId, m_Sensor->GetPreviewIndex(), pHistogram->Flags, pHistogram->Version);
    return Status;
}

//  Set KSPROPERTY_CAMERACONTROL_EXTENDED_HISTOGRAM.
NTSTATUS
CCaptureFilter::
SetHistogram(
    _In_    CExtendedProperty   *pHistogram
)
{
    PAGED_CODE();
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    if( m_Sensor->IsPreviewIndex(pHistogram->PinId) &&
            pHistogram->isValid() &&
            !(pHistogram->Capability & KSCAMERA_EXTENDEDPROP_CAPS_ASYNCCONTROL) &&
            (KSCAMERA_EXTENDEDPROP_HISTOGRAM_OFF == pHistogram->Flags ||
             KSCAMERA_EXTENDEDPROP_HISTOGRAM_ON  == pHistogram->Flags ) )
    {
        if( m_pinArray[pHistogram->PinId] &&
                m_pinArray[pHistogram->PinId]->GetState() != PinStopped )
        {
            Status = STATUS_INVALID_DEVICE_STATE;
        }
        else
        {
            //Set the histogram state.
            Status = m_Sensor->SetHistogram( pHistogram );
        }
    }

    DBG_TRACE("pHistogram->PinId = %u, m_sensor->PreviewIndex = %u, pHistogram->Flags = %016llx, pHistogram->Version = %u",
              pHistogram->PinId, m_Sensor->GetPreviewIndex(), pHistogram->Flags, pHistogram->Version);
    return Status;
}

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_FACEDETECTION.
NTSTATUS
CCaptureFilter::
GetFaceDetection(
    _Inout_ CExtendedVidProcSetting   *pFaceDetect
)
{
    PAGED_CODE();
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    DBG_ENTER("()");

    if( pFaceDetect->PinId==KSCAMERA_EXTENDEDPROP_FILTERSCOPE &&
        pFaceDetect->isValid() )
    {
        //Get the FaceDetection state
        Status = m_Sensor->GetFaceDetection( pFaceDetect );
    }

    DBG_LEAVE("(): PinId=0x%08X, Flags=0x%016llX, Version=%u, Value=%d, Status=0x%08X",
              pFaceDetect->PinId, pFaceDetect->Flags, pFaceDetect->Version, pFaceDetect->GetULONG(), Status);
    return Status;
}

//  Set KSPROPERTY_CAMERACONTROL_EXTENDED_FACEDETECTION.
NTSTATUS
CCaptureFilter::
SetFaceDetection(
    _In_    CExtendedVidProcSetting *pFaceDetect
)
{
    PAGED_CODE();
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    DBG_ENTER("(): PinId=0x%08X, Flags=0x%016llX, Version=%u, Value=%d",
              pFaceDetect->PinId, pFaceDetect->Flags, pFaceDetect->Version, pFaceDetect->GetULONG());

    if( pFaceDetect->PinId==KSCAMERA_EXTENDEDPROP_FILTERSCOPE &&
        pFaceDetect->isValid() &&
        (    KSCAMERA_EXTENDEDPROP_FACEDETECTION_OFF == pFaceDetect->Flags ||
                (KSCAMERA_EXTENDEDPROP_FACEDETECTION_MASK & pFaceDetect->Flags) !=0 ) )
    {
        CExtendedVidProcSetting Caps(*pFaceDetect);
        Status = m_Sensor->GetFaceDetection( &Caps );

        if( NT_SUCCESS(Status) )
        {
            //  Assume an invalid parameter.
            Status = STATUS_INVALID_PARAMETER;

            if( pFaceDetect->Flags == (pFaceDetect->Flags & Caps.Capability) &&
                    ( KSCAMERA_EXTENDEDPROP_FACEDETECTION_OFF == pFaceDetect->Flags ||
                      NT_SUCCESS( Status = Caps.BoundsCheck( pFaceDetect->GetULONG() ) ) ) )
            {
                //Set the FaceDetection state.
                Status = m_Sensor->SetFaceDetection( pFaceDetect );
            }
        }
    }

    DBG_LEAVE("(): Status=0x%08X", Status);
    return Status;
}

//  KSPROPERTY_CAMERACONTROL_EXTENDED_ZOOM Get handler
NTSTATUS
CCaptureFilter::
GetZoom(
    _Inout_ CExtendedVidProcSetting *pZoom
)
{
    PAGED_CODE();
    DBG_ENTER("()");

    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    if( pZoom->isValid() &&
            pZoom->PinId == KSCAMERA_EXTENDEDPROP_FILTERSCOPE )
    {
        Status = m_Sensor->GetZoom(pZoom);
    }

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

//  KSPROPERTY_CAMERACONTROL_EXTENDED_ZOOM Set handler
NTSTATUS
CCaptureFilter::
SetZoom(
    _In_    CExtendedVidProcSetting *pZoom
)
{
    PAGED_CODE();
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    DBG_ENTER("()");

    //  Validate
    if( pZoom->isValid() &&
            pZoom->PinId == KSCAMERA_EXTENDEDPROP_FILTERSCOPE &&
            !(pZoom->Capability & KSCAMERA_EXTENDEDPROP_CAPS_ASYNCCONTROL) )
    {
        CExtendedVidProcSetting Caps(*pZoom);
        Status = m_Sensor->GetZoom( &Caps );

        if( NT_SUCCESS(Status) )
        {
            //  Report to the debug output...
            DBG_TRACE("          Flags = 0x%016llX", Caps.Flags);
            DBG_TRACE("            Min = %d", Caps.Min());
            DBG_TRACE("            Max = %d", Caps.Max());
            DBG_TRACE("           Step = %d", Caps.Step());
            DBG_TRACE("VideoProc.Value = %d", pZoom->GetLONG());

            if( pZoom->Flags == (pZoom->Flags & Caps.Capability) )
            {
                Status = BoundsCheckSigned( pZoom->GetLONG(), Caps.m_Setting );

                if (NT_SUCCESS(Status))
                {
                    Status = m_Sensor->SetZoom(pZoom);
                }
            }
            else
            {
                Status = STATUS_INVALID_PARAMETER;
            }
        }
    }

    DBG_LEAVE("(PinId = %u, Flags = 0x%016llX, Caps = 0x%016llX)=0x%08X",
              pZoom->PinId, pZoom->Flags, pZoom->Capability, Status);
    return Status;
}

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_VIDEOSTABILIZATION.
NTSTATUS
CCaptureFilter::
GetVideoStabilization(
    _Inout_ CExtendedProperty   *pVideoStab
)
{
    PAGED_CODE();

    if (m_Sensor->IsVideoIndex(pVideoStab->PinId) &&
            pVideoStab->isValid())
    {
        //Get the current state
        return m_Sensor->GetVideoStabilization(pVideoStab);
    }
    return STATUS_INVALID_PARAMETER;
}

//  Set KSPROPERTY_CAMERACONTROL_EXTENDED_VIDEOSTABILIZATION.
NTSTATUS
CCaptureFilter::
SetVideoStabilization(
    _In_    CExtendedProperty   *pVideoStab
)
{
    PAGED_CODE();

    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    // Call must come for Video PIN only and flags must be supported and mutually exclusive and version must be 1
    if( m_Sensor->IsVideoIndex(pVideoStab->PinId) &&
            pVideoStab->isValid() &&
            !(pVideoStab->Capability & KSCAMERA_EXTENDEDPROP_CAPS_ASYNCCONTROL) &&
            (KSCAMERA_EXTENDEDPROP_VIDEOSTABILIZATION_OFF == pVideoStab->Flags ||
             KSCAMERA_EXTENDEDPROP_VIDEOSTABILIZATION_ON == pVideoStab->Flags ||
             KSCAMERA_EXTENDEDPROP_VIDEOSTABILIZATION_AUTO == pVideoStab->Flags) )
    {
        CExtendedProperty   Caps(*pVideoStab);
        Status = m_Sensor->GetVideoStabilization( &Caps );

        if( NT_SUCCESS(Status) )
        {
            //  Assume an invalid parameter.
            Status = STATUS_INVALID_PARAMETER;

            // Call must come for Video PIN only and flags must be supported and mutually exclusive and version must be 1
            if( pVideoStab->Flags == (pVideoStab->Flags & Caps.Capability) )
            {
                LockFilter Lock(m_pKSFilter);

                if( m_pinArray[pVideoStab->PinId] &&
                        m_pinArray[pVideoStab->PinId]->GetState() != PinStopped )
                {
                    Status = STATUS_INVALID_DEVICE_STATE;
                }
                else
                {
                    Status = m_Sensor->SetVideoStabilization(pVideoStab);
                }
            }
        }
    }
    DBG_TRACE("pVideoStab = %p, PinId = %u, m_sensor->VideoIndex = %u, Flags = %llu, Version = %u",
              pVideoStab, pVideoStab->PinId, m_Sensor->GetVideoIndex(), pVideoStab->Flags, pVideoStab->Version);

    return Status;
}

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_OIS.
NTSTATUS
CCaptureFilter::
GetOpticalImageStabilization(
    _Inout_ CExtendedProperty   *pOIS
)
{
    PAGED_CODE();
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    if( pOIS->PinId==KSCAMERA_EXTENDEDPROP_FILTERSCOPE &&
            pOIS->isValid() )
    {
        //Get the current state
        Status = m_Sensor->GetOpticalImageStabilization(pOIS);
    }

    DBG_TRACE("pOIS = %p, PinId = %u, Flags = %llu, Version = %u",
              pOIS, pOIS->PinId, pOIS->Flags, pOIS->Version);

    return Status;
}

//  Set KSPROPERTY_CAMERACONTROL_EXTENDED_OIS.
NTSTATUS
CCaptureFilter::
SetOpticalImageStabilization(
    _In_    CExtendedProperty   *pOIS
)
{
    PAGED_CODE();

    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    // Call must come for Video PIN only and flags must be supported and mutually exclusive and version must be 1
    if( pOIS->PinId==KSCAMERA_EXTENDEDPROP_FILTERSCOPE &&
            pOIS->isValid() &&
            !(pOIS->Capability & KSCAMERA_EXTENDEDPROP_CAPS_ASYNCCONTROL) &&
            (KSCAMERA_EXTENDEDPROP_OIS_OFF == pOIS->Flags ||
             KSCAMERA_EXTENDEDPROP_OIS_ON  == pOIS->Flags ||
             KSCAMERA_EXTENDEDPROP_OIS_AUTO== pOIS->Flags))
    {
        CExtendedProperty   Caps(*pOIS);
        Status = m_Sensor->GetOpticalImageStabilization(&Caps);

        if( NT_SUCCESS(Status) )
        {
            //  Assume an invalid parameter.
            Status = STATUS_INVALID_PARAMETER;

            if( pOIS->Flags == (pOIS->Flags & Caps.Capability) )
            {
                Status = m_Sensor->SetOpticalImageStabilization(pOIS);
            }
        }
    }
    DBG_TRACE("pOIS = %p, PinId = %u, Flags = %llu, Version = %u",
              pOIS, pOIS->PinId, pOIS->Flags, pOIS->Version);

    return Status;
}

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_ADVANCEDPHOTO.
NTSTATUS
CCaptureFilter::
GetAdvancedPhoto(
    _Inout_ CExtendedProperty   *pAdvancedPhoto
)
{
    PAGED_CODE();
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    if( m_Sensor->IsStillIndex(pAdvancedPhoto->PinId) &&
            pAdvancedPhoto->isValid() )
    {
        //Get the current state
        Status = m_Sensor->GetAdvancedPhoto(pAdvancedPhoto);
    }

    DBG_LEAVE(" PinId=%u, Flags=0x%016llX, Version=%u, Status=0x%08X",
        pAdvancedPhoto->PinId, pAdvancedPhoto->Flags, pAdvancedPhoto->Version, Status);
    return Status;
}

//  Set KSPROPERTY_CAMERACONTROL_EXTENDED_ADVANCEDPHOTO.
NTSTATUS
CCaptureFilter::
SetAdvancedPhoto(
    _In_    CExtendedProperty   *pAdvancedPhoto
)
{
    PAGED_CODE();

    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    // Valid request, still pin & no unsupported flags.
    if( m_Sensor->IsStillIndex(pAdvancedPhoto->PinId) &&
            pAdvancedPhoto->isValid() &&
            !(pAdvancedPhoto->Capability & KSCAMERA_EXTENDEDPROP_CAPS_ASYNCCONTROL) )
    {
        CExtendedProperty   Caps(*pAdvancedPhoto);
        Status = m_Sensor->GetAdvancedPhoto(&Caps);

        if( NT_SUCCESS(Status) )
        {
            //  Assume an invalid parameter.
            Status = STATUS_INVALID_PARAMETER;

            if( pAdvancedPhoto->Flags == (pAdvancedPhoto->Flags & Caps.Capability) )
            {
                //  Make sure the requested flags are a valid capability combination.
                switch( pAdvancedPhoto->Flags )
                {
                case KSCAMERA_EXTENDEDPROP_ADVANCEDPHOTO_OFF          :
                case KSCAMERA_EXTENDEDPROP_ADVANCEDPHOTO_AUTO         :
                case KSCAMERA_EXTENDEDPROP_ADVANCEDPHOTO_HDR          :
                case KSCAMERA_EXTENDEDPROP_ADVANCEDPHOTO_FNF          :
                case KSCAMERA_EXTENDEDPROP_ADVANCEDPHOTO_ULTRALOWLIGHT:
                case KSCAMERA_EXTENDEDPROP_ADVANCEDPHOTO_AUTO   |
                        KSCAMERA_EXTENDEDPROP_ADVANCEDPHOTO_HDR          :
                case KSCAMERA_EXTENDEDPROP_ADVANCEDPHOTO_AUTO   |
                        KSCAMERA_EXTENDEDPROP_ADVANCEDPHOTO_FNF          :
                case KSCAMERA_EXTENDEDPROP_ADVANCEDPHOTO_AUTO   |
                        KSCAMERA_EXTENDEDPROP_ADVANCEDPHOTO_ULTRALOWLIGHT:
                    Status = m_Sensor->SetAdvancedPhoto(pAdvancedPhoto);
                }
            }
        }
    }

    DBG_LEAVE(" PinId=%u, Flags=0x%016llX, Version=%u, Status=0x%08X",
        pAdvancedPhoto->PinId, pAdvancedPhoto->Flags, pAdvancedPhoto->Version, Status);
    return Status;
}

//  Get [legacy] KSPROPERTY_CAMERACONTROL_EXPOSURE
NTSTATUS
CCaptureFilter::
GetCameraProfile(
    _Inout_ CExtendedProfile   *pProfile
)
{
    PAGED_CODE();
    NTSTATUS Status = STATUS_SUCCESS;

    pProfile->Version = KSCAMERA_EXTENDEDPROP_VERSION;
    pProfile->PinId = KSCAMERA_EXTENDEDPROP_FILTERSCOPE;
    pProfile->Size = sizeof(KSCAMERA_EXTENDEDPROP_HEADER) + sizeof (KSCAMERA_EXTENDEDPROP_PROFILE);
    pProfile->Result = 0;
    pProfile->Flags = 0;
    pProfile->Capability = KSCAMERA_EXTENDEDPROP_CAPS_ASYNCCONTROL;
    pProfile->m_Profile = m_Profile;

    return Status;
}

//  Set KSPROPERTY_CAMERACONTROL_EXTENDED_PROFILE.
NTSTATUS
CCaptureFilter::
SetCameraProfile(
    _In_    CExtendedProfile   *pProfile
)
{
    PAGED_CODE();

    if (pProfile->PinId != KSCAMERA_EXTENDEDPROP_FILTERSCOPE ||
        !pProfile->isValid() ||
        0 == (pProfile->Capability & KSCAMERA_EXTENDEDPROP_CAPS_ASYNCCONTROL) ||
        pProfile->Flags != 0)
    {
        return STATUS_INVALID_PARAMETER;
    }
    m_Profile = pProfile->m_Profile;

    m_ProfileNotifier.Set();
    return STATUS_SUCCESS;
}


NTSTATUS
CCaptureFilter::
GetExposure(
    _Inout_ KSPROPERTY_CAMERACONTROL_S  *pProperty
)
{
    PAGED_CODE();
    DBG_ENTER("()");

    NTSTATUS Status = m_Sensor->GetExposure(pProperty);

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

//  Set [legacy] KSPROPERTY_CAMERACONTROL_EXPOSURE
NTSTATUS
CCaptureFilter::
SetExposure(
    _In_    KSPROPERTY_CAMERACONTROL_S  *pProperty
)
{
    PAGED_CODE();
    DBG_ENTER("()");

    NTSTATUS Status = m_Sensor->SetExposure(pProperty);

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

//  Get [legacy] KSPROPERTY_CAMERACONTROL_FOCUS
NTSTATUS
CCaptureFilter::
GetFocus(
    _Inout_ PKSPROPERTY_CAMERACONTROL_S             pProperty
)
{
    PAGED_CODE();
    DBG_ENTER("()");

    NTSTATUS Status = m_Sensor->GetFocus(pProperty);

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

//  Set [legacy] KSPROPERTY_CAMERACONTROL_FOCUS
NTSTATUS
CCaptureFilter::
SetFocus(
    _In_    PKSPROPERTY_CAMERACONTROL_S pProperty
)
{
    PAGED_CODE();
    DBG_ENTER("()");

    m_Sensor->CancelFocus();

    NTSTATUS Status = m_Sensor->SetFocus(pProperty);

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

//  Get [legacy] KSPROPERTY_CAMERACONTROL_ZOOM
NTSTATUS
CCaptureFilter::
GetZoom(
    _Inout_ PKSPROPERTY_CAMERACONTROL_S pProperty
)
{
    PAGED_CODE();
    DBG_ENTER("()");

    NTSTATUS Status = m_Sensor->GetZoom(pProperty);

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

//  Set [legacy] KSPROPERTY_CAMERACONTROL_ZOOM
NTSTATUS
CCaptureFilter::
SetZoom(
    _In_    PKSPROPERTY_CAMERACONTROL_S pProperty
)
{
    PAGED_CODE();
    DBG_ENTER("()");

    NTSTATUS Status = m_Sensor->SetZoom(pProperty);

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

//  Get [legacy] KSPROPERTY_CAMERACONTROL_ZOOM_RELATIVE
NTSTATUS
CCaptureFilter::
GetZoomRelative(
    _Inout_ PKSPROPERTY_CAMERACONTROL_S pProperty
)
{
    PAGED_CODE();
    DBG_ENTER("()");

    NTSTATUS Status = m_Sensor->GetZoomRelative(pProperty);

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

//  Set [legacy] KSPROPERTY_CAMERACONTROL_ZOOM_RELATIVE
NTSTATUS
CCaptureFilter::
SetZoomRelative(
    _In_    PKSPROPERTY_CAMERACONTROL_S pProperty
)
{
    PAGED_CODE();
    DBG_ENTER("()");

    NTSTATUS Status = m_Sensor->SetZoomRelative(pProperty);

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

//  Get KSPROPERTY_CAMERACONTROL_PAN
NTSTATUS
CCaptureFilter::
GetPan(
    _Inout_ PKSPROPERTY_CAMERACONTROL_S pProperty
)
{
    PAGED_CODE();
    DBG_ENTER("()");

    NTSTATUS Status = m_Sensor->GetPan(pProperty);

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

//  Set KSPROPERTY_CAMERACONTROL_PAN
NTSTATUS
CCaptureFilter::
SetPan(
    _In_    PKSPROPERTY_CAMERACONTROL_S pProperty
)
{
    PAGED_CODE();
    DBG_ENTER("()");

    NTSTATUS Status = m_Sensor->SetPan(pProperty);

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

//  Get KSPROPERTY_CAMERACONTROL_ROLL
NTSTATUS
CCaptureFilter::
GetRoll(
    _Inout_ PKSPROPERTY_CAMERACONTROL_S pProperty
)
{
    PAGED_CODE();
    DBG_ENTER("()");

    NTSTATUS Status = m_Sensor->GetRoll(pProperty);

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

//  Set KSPROPERTY_CAMERACONTROL_ROLL
NTSTATUS
CCaptureFilter::
SetRoll(
    _In_    PKSPROPERTY_CAMERACONTROL_S pProperty
)
{
    PAGED_CODE();
    DBG_ENTER("()");

    NTSTATUS Status = m_Sensor->SetRoll(pProperty);

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

//  Get KSPROPERTY_CAMERACONTROL_TILT
NTSTATUS
CCaptureFilter::
GetTilt(
    _Inout_ PKSPROPERTY_CAMERACONTROL_S pProperty
)
{
    PAGED_CODE();
    DBG_ENTER("()");

    NTSTATUS Status = m_Sensor->GetTilt(pProperty);

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

//  Set KSPROPERTY_CAMERACONTROL_TILT
NTSTATUS
CCaptureFilter::
SetTilt(
    _In_    PKSPROPERTY_CAMERACONTROL_S pProperty
)
{
    PAGED_CODE();
    DBG_ENTER("()");

    NTSTATUS Status = m_Sensor->SetTilt(pProperty);

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

//  Get KSPROPERTY_CAMERACONTROL_FOCAL_LENGTH
NTSTATUS
CCaptureFilter::
GetFocalLength(
    _Inout_ PKSPROPERTY_CAMERACONTROL_FOCAL_LENGTH_S    pProperty
)
{
    PAGED_CODE();
    DBG_ENTER("()");

    NTSTATUS Status = m_Sensor->GetFocalLength(pProperty);

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

//  Set KSPROPERTY_CAMERACONTROL_FOCAL_LENGTH
NTSTATUS
CCaptureFilter::
SetFocalLength(
    _In_    PKSPROPERTY_CAMERACONTROL_FOCAL_LENGTH_S    pProperty
)
{
    PAGED_CODE();
    DBG_ENTER("()");

    NTSTATUS Status = m_Sensor->SetFocalLength(pProperty);

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

//  Get KSPROPERTY_VIDEOPROCAMP_BACKLIGHT_COMPENSATION
NTSTATUS
CCaptureFilter::
GetBacklightCompensation(
    _Inout_ PKSPROPERTY_VIDEOPROCAMP_S  pProperty
)
{
    PAGED_CODE();
    DBG_ENTER("()");

    NTSTATUS Status = m_Sensor->GetBacklightCompensation(pProperty);

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

//  Set KSPROPERTY_VIDEOPROCAMP_BACKLIGHT_COMPENSATION
NTSTATUS
CCaptureFilter::
SetBacklightCompensation(
    _In_    PKSPROPERTY_VIDEOPROCAMP_S  pProperty
)
{
    PAGED_CODE();
    DBG_ENTER("()");

    NTSTATUS Status = m_Sensor->SetBacklightCompensation(pProperty);

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

//  Get KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS
NTSTATUS
CCaptureFilter::
GetBrightness(
    _Inout_ PKSPROPERTY_VIDEOPROCAMP_S  pProperty
)
{
    PAGED_CODE();
    DBG_ENTER("()");

    NTSTATUS Status = m_Sensor->GetBrightness(pProperty);

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

//  Set KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS
NTSTATUS
CCaptureFilter::
SetBrightness(
    _In_    PKSPROPERTY_VIDEOPROCAMP_S  pProperty
)
{
    PAGED_CODE();
    DBG_ENTER("()");

    NTSTATUS Status = m_Sensor->SetBrightness(pProperty);

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

//  Get KSPROPERTY_VIDEOPROCAMP_CONTRAST
NTSTATUS
CCaptureFilter::
GetContrast(
    _Inout_ PKSPROPERTY_VIDEOPROCAMP_S  pProperty
)
{
    PAGED_CODE();
    DBG_ENTER("()");

    NTSTATUS Status = m_Sensor->GetContrast(pProperty);

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

//  Set KSPROPERTY_VIDEOPROCAMP_CONTRAST
NTSTATUS
CCaptureFilter::
SetContrast(
    _In_    PKSPROPERTY_VIDEOPROCAMP_S  pProperty
)
{
    PAGED_CODE();
    DBG_ENTER("()");

    NTSTATUS Status = m_Sensor->SetContrast(pProperty);

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

//  Get KSPROPERTY_VIDEOPROCAMP_HUE
NTSTATUS
CCaptureFilter::
GetHue(
    _Inout_ PKSPROPERTY_VIDEOPROCAMP_S  pProperty
)
{
    PAGED_CODE();
    DBG_ENTER("()");

    NTSTATUS Status = m_Sensor->GetHue(pProperty);

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

//  Set KSPROPERTY_VIDEOPROCAMP_HUE
NTSTATUS
CCaptureFilter::
SetHue(
    _In_    PKSPROPERTY_VIDEOPROCAMP_S  pProperty
)
{
    PAGED_CODE();
    DBG_ENTER("()");

    NTSTATUS Status = m_Sensor->SetHue(pProperty);

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

//  Get [legacy] KSPROPERTY_VIDEOPROCAMP_WHITEBALANCE
NTSTATUS
CCaptureFilter::
GetWhiteBalance(
    _Inout_ PKSPROPERTY_VIDEOPROCAMP_S  pProperty
)
{
    PAGED_CODE();
    DBG_ENTER("()");

    NTSTATUS Status = m_Sensor->GetWhiteBalance(pProperty);

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

//  Set [legacy] KSPROPERTY_VIDEOPROCAMP_WHITEBALANCE
NTSTATUS
CCaptureFilter::
SetWhiteBalance(
    _In_    PKSPROPERTY_VIDEOPROCAMP_S  pProperty
)
{
    PAGED_CODE();
    DBG_ENTER("()");

    NTSTATUS Status = m_Sensor->SetWhiteBalance(pProperty);

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

//  Get KSPROPERTY_VIDEOPROCAMP_POWERLINE_FREQUENCY
NTSTATUS
CCaptureFilter::
GetPowerlineFreq(
    _Inout_ PKSPROPERTY_VIDEOPROCAMP_S  pProperty
)
{
    PAGED_CODE();
    DBG_ENTER("()");

    NTSTATUS Status = m_Sensor->GetPowerlineFreq(pProperty);

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

//  Set KSPROPERTY_VIDEOPROCAMP_POWERLINE_FREQUENCY
NTSTATUS
CCaptureFilter::
SetPowerlineFreq(
    _In_    PKSPROPERTY_VIDEOPROCAMP_S  pProperty
)
{
    PAGED_CODE();
    DBG_ENTER("()");

    NTSTATUS Status = m_Sensor->SetPowerlineFreq(pProperty);

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

//  Get KSPROPERTY_VIDEOCONTROL_MODE.
NTSTATUS
CCaptureFilter::
GetVideoControlMode(
    _Inout_ KSPROPERTY_VIDEOCONTROL_MODE_S  *pMode
)
{
    PAGED_CODE();
    DBG_ENTER("()");

    NTSTATUS    Status = STATUS_INVALID_PARAMETER;

    if( m_Sensor->IsStillIndex(pMode->StreamIndex) )
    {
        Status = m_Sensor->GetVideoControlMode( pMode );
    }

    DBG_LEAVE("( Pin=%d, Mode=0x%08X )=0x%08X",
              pMode->StreamIndex, pMode->Mode, Status );
    return Status;
}

//  Set KSPROPERTY_VIDEOCONTROL_MODE.
NTSTATUS
CCaptureFilter::
SetVideoControlMode(
    _In_    KSPROPERTY_VIDEOCONTROL_MODE_S  *pMode
)
{
    PAGED_CODE();
    DBG_ENTER("()");

    NTSTATUS    Status = STATUS_INVALID_PARAMETER;

    if( m_Sensor->IsStillIndex(pMode->StreamIndex) )
    {
        KSPROPERTY_VIDEOCONTROL_MODE_S  Caps;
        RtlZeroMemory( &Caps, sizeof(Caps) );
        
        Status = m_Sensor->GetVideoControlMode( &Caps );
        if( NT_SUCCESS(Status) )//&&
        //    pMode->Mode == (pMode->Mode & Caps.Mode) )
        {
            DBG_TRACE("Mode=0x%08X, Caps.Mode=0x%08X", pMode->Mode, Caps.Mode);
            Status = m_Sensor->SetVideoControlMode( pMode );
        }
        else
        {
            Status = STATUS_INVALID_PARAMETER;
        }
    }

    DBG_LEAVE("( Pin=%d, Mode=0x%08X )=0x%08X",
              pMode->StreamIndex, pMode->Mode, Status );
    return Status;
}

