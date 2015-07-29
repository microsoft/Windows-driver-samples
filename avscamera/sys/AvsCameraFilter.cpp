/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2015, Microsoft Corporation.

    File:

        AvsCameraFilter.cpp

    Abstract:

        Filter class implementation.  Derived from CCaptureFilter.
        This class overrides the default implementation.

    History:

        created 02/24/2015

**************************************************************************/

#include "Common.h"

/**************************************************************************

    PAGEABLE CODE

**************************************************************************/

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA

CAvsCameraFilter::
CAvsCameraFilter(
    _In_    PKSFILTER Filter
)
    : CCaptureFilter( Filter )
{
    PAGED_CODE();
}

//
// ~CCaptureFilter():
//
// The capture filter destructor.
//
CAvsCameraFilter::
~CAvsCameraFilter()
{
    PAGED_CODE();
}


NTSTATUS
CAvsCameraFilter::
DispatchCreate (
    _In_    PKSFILTER Filter,
    _In_    PIRP Irp
)

/*++

Routine Description:

    This is the creation dispatch for the capture filter.  It creates
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

    CAvsCameraFilter *CapFilter = new (NonPagedPoolNx, 'rtlf') CAvsCameraFilter(Filter);

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

//  Get KSPROPERTY_CUSTOMCONTROL_DUMMY.
NTSTATUS
CAvsCameraFilter::
GetCustomDummy(
    _Inout_ ULONG      *pDummy
)
{
    PAGED_CODE();

    *pDummy = m_CustomValue;
    return STATUS_SUCCESS;
}

//  Set KSPROPERTY_CUSTOMCONTROL_DUMMY.
NTSTATUS
CAvsCameraFilter::
SetCustomDummy(
    _In_    ULONG      *pDummy
)
{
    PAGED_CODE();

    m_CustomValue = *pDummy;
    return STATUS_SUCCESS;
}

/**************************************************************************

    DESCRIPTOR AND DISPATCH LAYOUT

**************************************************************************/

GUID g_PINNAME_VIDEO_CAPTURE = {STATIC_PINNAME_VIDEO_CAPTURE};
GUID g_PINNAME_VIDEO_PREVIEW = {STATIC_PINNAME_VIDEO_PREVIEW};
GUID g_PINNAME_IMAGE_CAPTURE = {STATIC_PINNAME_IMAGE};

//
// CaptureFilterCategories:
//
// The list of category GUIDs for the capture filter.
//
const
GUID
FilterCategories [CAPTURE_FILTER_CATEGORIES_COUNT] =
{
    STATICGUIDOF (KSCATEGORY_VIDEO_CAMERA),
    STATICGUIDOF (KSCATEGORY_VIDEO),
    STATICGUIDOF (KSCATEGORY_CAPTURE)
};

DEFINE_KSPROPERTY_TABLE(FocusPropertyItems)
{
    DEFINE_PROP_ITEM( CCaptureFilter, KSPROPERTY_CAMERACONTROL_REGION_OF_INTEREST_PROPERTY_ID, KSPROPERTY_CAMERACONTROL_REGION_OF_INTEREST_S, FocusRect )
};

DEFINE_KSPROPERTY_TABLE(VideoStabPropertyItems)
{
    DEFINE_PROP_ITEM( CCaptureFilter, 0, KSPROPERTY_CAMERACONTROL_VIDEOSTABILIZATION_MODE_S, VideoStabMode )
};

DEFINE_KSPROPERTY_TABLE(FlashPropertyItems)
{
    DEFINE_PROP_ITEM( CCaptureFilter, KSPROPERTY_CAMERACONTROL_FLASH_PROPERTY_ID, KSPROPERTY_CAMERACONTROL_FLASH_S, Flash )
};

DEFINE_KSPROPERTY_TABLE(PinDependencePropertyItems)
{
    //  This control only requires a getter.
    DEFINE_PROP_ITEM_NO_SET( CCaptureFilter, KSPROPERTY_CAMERACONTROL_IMAGE_PIN_CAPABILITY_PROPERTY_ID, KSPROPERTY_CAMERACONTROL_IMAGE_PIN_CAPABILITY_S,
                             PinDependence )
};

DEFINE_KSPROPERTY_TABLE(ExtendedPropertyItems)
{
    DEFINE_PROP_ITEM( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_PHOTOMODE, CExtendedPhotoMode, PhotoMode ),
    DEFINE_PROP_ITEM_NO_SET( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_PHOTOFRAMERATE, CExtendedProperty, PhotoFrameRate ),
    DEFINE_PROP_ITEM( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_PHOTOMAXFRAMERATE, CExtendedProperty, PhotoMaxFrameRate ),
    DEFINE_PROP_ITEM( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_PHOTOTRIGGERTIME, CExtendedProperty, TriggerTime ),
    DEFINE_PROP_ITEM( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_WARMSTART, CExtendedProperty, WarmStart ),
    DEFINE_PROP_ITEM( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_MAXVIDFPS_PHOTORES, CExtendedMaxVideoFpsForPhotoRes, MaxVideoFpsForPhotoRes ),
    DEFINE_PROP_ITEM( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_SCENEMODE, CExtendedProperty, SceneMode ),
    DEFINE_PROP_ITEM( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_TORCHMODE, CExtendedProperty, TorchMode ),
    DEFINE_PROP_ITEM( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_FLASHMODE, CExtendedProperty, ExtendedFlash ),
    DEFINE_PROP_ITEM( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_OPTIMIZATIONHINT, CExtendedProperty, OptimizationHint ),
    DEFINE_PROP_ITEM( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_WHITEBALANCEMODE, CExtendedVidProcSetting, WhiteBalance ),
    DEFINE_PROP_ITEM( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_EXPOSUREMODE, CExtendedVidProcSetting, Exposure ),
    DEFINE_PROP_ITEM( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_FOCUSMODE, CExtendedVidProcSetting, Focus ),
    DEFINE_PROP_ITEM( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_ISO, CExtendedProperty, Iso ),
    DEFINE_PROP_ITEM_NO_SET( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_FIELDOFVIEW, CExtendedFieldOfView, FieldOfView ),
    DEFINE_PROP_ITEM( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_EVCOMPENSATION, CExtendedEvCompensation, EvCompensation ),
    DEFINE_PROP_ITEM_NO_SET( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_CAMERAANGLEOFFSET, CExtendedCameraAngleOffset, CameraAngleOffset ),
    DEFINE_PROP_ITEM( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_PHOTOCONFIRMATION, CExtendedProperty, PhotoConfirmation ),
    DEFINE_PROP_ITEM( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_METADATA, CExtendedMetadata, Metadata ),
    DEFINE_PROP_ITEM_NO_SET( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_FOCUSSTATE, CExtendedProperty, FocusState ),
    DEFINE_PROP_ITEM( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_FOCUSPRIORITY, CExtendedProperty, FocusPriority ),
    DEFINE_PROP_ITEM( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_FACEDETECTION, CExtendedVidProcSetting, FaceDetection ),
    DEFINE_PROP_ITEM_NO_SET( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_ROI_CONFIGCAPS, CRoiConfig, RoiConfigCaps ),
    DEFINE_KSPROPERTY_ITEM(
        KSPROPERTY_CAMERACONTROL_EXTENDED_ROI_ISPCONTROL,
        CCaptureFilter::GetRoiIspControl,
        sizeof(KSPROPERTY),
        0,
        CCaptureFilter::SetRoiIspControl,
        NULL, 0, NULL, NULL, 0
    ),
    DEFINE_PROP_ITEM(CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_ZOOM, CExtendedVidProcSetting, Zoom),
    DEFINE_PROP_ITEM( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_VIDEOHDR, CExtendedProperty, VideoHDR ),
    DEFINE_PROP_ITEM( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_VFR, CExtendedProperty, VFR ),
    DEFINE_PROP_ITEM( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_ISO_ADVANCED, CExtendedVidProcSetting, IsoAdvanced ),
    DEFINE_PROP_ITEM( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_VIDEOSTABILIZATION, CExtendedProperty, VideoStabilization),
    DEFINE_PROP_ITEM( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_HISTOGRAM, CExtendedProperty, Histogram ),
    DEFINE_PROP_ITEM( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_OIS, CExtendedProperty, OpticalImageStabilization ),
    DEFINE_PROP_ITEM( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_ADVANCEDPHOTO, CExtendedProperty, AdvancedPhoto ),
    DEFINE_PROP_ITEM( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_PROFILE, CExtendedProfile, CameraProfile )
};

//  Front-facing cameras often have limited capabilities.  One way to express that is to use a restricted automation table such as this one.
DEFINE_KSPROPERTY_TABLE(ExtendedPropertyItemsFFC)
{
    DEFINE_PROP_ITEM( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_PHOTOMODE, CExtendedPhotoMode, PhotoMode ),
    DEFINE_PROP_ITEM_NO_SET( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_PHOTOFRAMERATE, CExtendedProperty, PhotoFrameRate ),
    DEFINE_PROP_ITEM( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_PHOTOMAXFRAMERATE, CExtendedProperty, PhotoMaxFrameRate ),
    DEFINE_PROP_ITEM( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_PHOTOTRIGGERTIME, CExtendedProperty, TriggerTime ),
    DEFINE_PROP_ITEM( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_WARMSTART, CExtendedProperty, WarmStart ),
    DEFINE_PROP_ITEM_NO_SET( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_MAXVIDFPS_PHOTORES, CExtendedMaxVideoFpsForPhotoRes, MaxVideoFpsForPhotoRes ),
    DEFINE_PROP_ITEM( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_SCENEMODE, CExtendedProperty, SceneMode ),
    DEFINE_PROP_ITEM( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_TORCHMODE, CExtendedProperty, TorchMode ),
    DEFINE_PROP_ITEM( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_OPTIMIZATIONHINT, CExtendedProperty, OptimizationHint ),
    DEFINE_PROP_ITEM( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_WHITEBALANCEMODE, CExtendedVidProcSetting, WhiteBalance ),
    DEFINE_PROP_ITEM( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_EXPOSUREMODE, CExtendedVidProcSetting, Exposure ),
    DEFINE_PROP_ITEM( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_ISO, CExtendedProperty, Iso ),
    DEFINE_PROP_ITEM_NO_SET( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_FIELDOFVIEW, CExtendedFieldOfView, FieldOfView ),
    DEFINE_PROP_ITEM( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_EVCOMPENSATION, CExtendedEvCompensation, EvCompensation ),
    DEFINE_PROP_ITEM_NO_SET( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_CAMERAANGLEOFFSET, CExtendedCameraAngleOffset, CameraAngleOffset ),
    DEFINE_PROP_ITEM( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_PHOTOCONFIRMATION, CExtendedProperty, PhotoConfirmation ),
    DEFINE_PROP_ITEM( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_METADATA, CExtendedMetadata, Metadata ),
    DEFINE_PROP_ITEM(CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_ZOOM, CExtendedVidProcSetting, Zoom),
    DEFINE_PROP_ITEM( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_FACEDETECTION, CExtendedVidProcSetting, FaceDetection ),
    DEFINE_PROP_ITEM( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_ISO_ADVANCED, CExtendedVidProcSetting, IsoAdvanced ),
    DEFINE_PROP_ITEM( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_VIDEOSTABILIZATION, CExtendedProperty, VideoStabilization ),
    DEFINE_PROP_ITEM( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_ADVANCEDPHOTO, CExtendedProperty, AdvancedPhoto ),
    DEFINE_PROP_ITEM( CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXTENDED_PROFILE, CExtendedProfile, CameraProfile )
};

//--------------------------------------------------------------
//  Legacy Camera Contorl properties
DEFINE_KSPROPERTY_TABLE(CameraControlPropertyTable)
{
    DEFINE_PROP_ITEM_WITH_VALUES(CCaptureFilter, KSPROPERTY_CAMERACONTROL_EXPOSURE, KSPROPERTY_CAMERACONTROL_S, Exposure, &ExposureValues),
    DEFINE_PROP_ITEM_WITH_VALUES(CCaptureFilter, KSPROPERTY_CAMERACONTROL_FOCUS, KSPROPERTY_CAMERACONTROL_S, Focus, &FocusValues),
    DEFINE_PROP_ITEM_WITH_VALUES(CCaptureFilter, KSPROPERTY_CAMERACONTROL_ZOOM, KSPROPERTY_CAMERACONTROL_S, Zoom, &ZoomValues),
    DEFINE_PROP_ITEM_WITH_VALUES(CCaptureFilter, KSPROPERTY_CAMERACONTROL_ZOOM_RELATIVE, KSPROPERTY_CAMERACONTROL_S, ZoomRelative, &ZoomRelativeValues),
    DEFINE_PROP_ITEM_WITH_VALUES(CCaptureFilter, KSPROPERTY_CAMERACONTROL_PAN, KSPROPERTY_CAMERACONTROL_S, Pan, &PanValues),
    DEFINE_PROP_ITEM_WITH_VALUES(CCaptureFilter, KSPROPERTY_CAMERACONTROL_ROLL, KSPROPERTY_CAMERACONTROL_S, Roll, &RollValues),
    DEFINE_PROP_ITEM_WITH_VALUES(CCaptureFilter, KSPROPERTY_CAMERACONTROL_TILT, KSPROPERTY_CAMERACONTROL_S, Tilt, &TiltValues),
    DEFINE_PROP_ITEM_WITH_VALUES(CCaptureFilter, KSPROPERTY_CAMERACONTROL_FOCAL_LENGTH, KSPROPERTY_CAMERACONTROL_FOCAL_LENGTH_S, FocalLength, NULL)
};

DEFINE_KSPROPERTY_TABLE(PFSPropertyTable)
{
    DEFINE_KSPROPERTY_ITEM(
        KSPROPERTY_CAMERACONTROL_PERFRAMESETTING_CAPABILITY,
        CCaptureFilter::GetPFSCaps,             // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        0,                                      // MinData
        NULL,                                   // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM(
        KSPROPERTY_CAMERACONTROL_PERFRAMESETTING_SET,
        CCaptureFilter::GetPerFrameSettings,    // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        0,                                      // MinData
        CCaptureFilter::SetPerFrameSettings,    // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM(
        KSPROPERTY_CAMERACONTROL_PERFRAMESETTING_CLEAR,
        NULL,                                   // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        0,                                      // MinData
        CCaptureFilter::ClearPerFrameSettings,  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    )
};

DEFINE_KSPROPERTY_TABLE(FilterVidcapPropertyTable)
{
    DEFINE_PROP_ITEM( CCaptureFilter, KSPROPERTY_VIDEOCONTROL_MODE, KSPROPERTY_VIDEOCONTROL_MODE_S, VideoControlMode )
};

DEFINE_KSPROPERTY_TABLE( VideoProcampPropertyTable )
{
    DEFINE_PROP_ITEM_WITH_VALUES(CCaptureFilter, KSPROPERTY_VIDEOPROCAMP_BACKLIGHT_COMPENSATION, KSPROPERTY_VIDEOPROCAMP_S, BacklightCompensation,
                                 &BacklightCompensationValues),
    DEFINE_PROP_ITEM_WITH_VALUES(CCaptureFilter, KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS, KSPROPERTY_VIDEOPROCAMP_S, Brightness, &BrightnessValues),
    DEFINE_PROP_ITEM_WITH_VALUES(CCaptureFilter, KSPROPERTY_VIDEOPROCAMP_CONTRAST, KSPROPERTY_VIDEOPROCAMP_S, Contrast, &ContrastValues),
    DEFINE_PROP_ITEM_WITH_VALUES(CCaptureFilter, KSPROPERTY_VIDEOPROCAMP_HUE, KSPROPERTY_VIDEOPROCAMP_S, Hue, &HueValues),
    DEFINE_PROP_ITEM_WITH_VALUES(CCaptureFilter, KSPROPERTY_VIDEOPROCAMP_WHITEBALANCE, KSPROPERTY_VIDEOPROCAMP_S, WhiteBalance, &WhiteBalanceValues),
    DEFINE_PROP_ITEM_WITH_VALUES(CCaptureFilter, KSPROPERTY_VIDEOPROCAMP_POWERLINE_FREQUENCY, KSPROPERTY_VIDEOPROCAMP_S, PowerlineFreq, &PLFValues)
};

DEFINE_KSPROPERTY_TABLE( CustomPropertyTable )
{
    DEFINE_PROP_ITEM(CAvsCameraFilter, KSPROPERTY_CUSTOMCONTROL_DUMMY, ULONG, CustomDummy)
};

DEFINE_KSEVENT_TABLE(VidCapRoiEventTable)
{
    DEFINE_STD_EVENT_ITEM(KSPROPERTY_CAMERACONTROL_REGION_OF_INTEREST_PROPERTY_ID)
};

DEFINE_KSEVENT_TABLE(ExtendedPropertyEventTable)
{
    DEFINE_STD_EVENT_ITEM(KSPROPERTY_CAMERACONTROL_EXTENDED_PHOTOMODE),
    DEFINE_STD_EVENT_ITEM(KSPROPERTY_CAMERACONTROL_EXTENDED_PHOTOMAXFRAMERATE),
    DEFINE_STD_EVENT_ITEM(KSPROPERTY_CAMERACONTROL_EXTENDED_WARMSTART),
    DEFINE_STD_EVENT_ITEM(KSPROPERTY_CAMERACONTROL_EXTENDED_SCENEMODE),
    DEFINE_STD_EVENT_ITEM(KSPROPERTY_CAMERACONTROL_EXTENDED_WHITEBALANCEMODE),
    DEFINE_STD_EVENT_ITEM(KSPROPERTY_CAMERACONTROL_EXTENDED_EXPOSUREMODE),
    DEFINE_STD_EVENT_ITEM(KSPROPERTY_CAMERACONTROL_EXTENDED_FOCUSMODE),
    DEFINE_STD_EVENT_ITEM(KSPROPERTY_CAMERACONTROL_EXTENDED_ISO),
    DEFINE_STD_EVENT_ITEM(KSPROPERTY_CAMERACONTROL_EXTENDED_EVCOMPENSATION),
    DEFINE_STD_EVENT_ITEM(KSPROPERTY_CAMERACONTROL_EXTENDED_ROI_ISPCONTROL),
    DEFINE_STD_EVENT_ITEM(KSPROPERTY_CAMERACONTROL_EXTENDED_ISO_ADVANCED),
    DEFINE_STD_EVENT_ITEM(KSPROPERTY_CAMERACONTROL_EXTENDED_PROFILE)
};

DEFINE_KSPROPERTY_SET_TABLE(PropertySets)
{
    DEFINE_STD_PROPERTY_SET( PROPSETID_VIDCAP_CAMERACONTROL_REGION_OF_INTEREST,   FocusPropertyItems            ),
    DEFINE_STD_PROPERTY_SET( PROPSETID_VIDCAP_CAMERACONTROL_FLASH,                FlashPropertyItems            ),
    DEFINE_STD_PROPERTY_SET( PROPSETID_VIDCAP_CAMERACONTROL_VIDEO_STABILIZATION,  VideoStabPropertyItems        ),
    DEFINE_STD_PROPERTY_SET( PROPSETID_VIDCAP_VIDEOCONTROL,                       FilterVidcapPropertyTable     ),
    DEFINE_STD_PROPERTY_SET( PROPSETID_VIDCAP_CAMERACONTROL_IMAGE_PIN_CAPABILITY, PinDependencePropertyItems    ),
    DEFINE_STD_PROPERTY_SET( KSPROPERTYSETID_ExtendedCameraControl,               ExtendedPropertyItems         ),
    DEFINE_STD_PROPERTY_SET( KSPROPERTYSETID_PerFrameSettingControl,              PFSPropertyTable              ),
    DEFINE_STD_PROPERTY_SET( PROPSETID_VIDCAP_CAMERACONTROL,                      CameraControlPropertyTable    ),
    DEFINE_STD_PROPERTY_SET( PROPSETID_VIDCAP_VIDEOPROCAMP,                       VideoProcampPropertyTable     ),
    DEFINE_STD_PROPERTY_SET( PROPSETID_VIDCAP_CUSTOMCONTROL,                      CustomPropertyTable           )
};

DEFINE_KSEVENT_SET_TABLE(EventSets)
{
    DEFINE_KSEVENT_SET
    (
        &KSEVENTSETID_ExtendedCameraControl,
        SIZEOF_ARRAY(ExtendedPropertyEventTable),
        ExtendedPropertyEventTable
    ),
    DEFINE_KSEVENT_SET
    (
        &EVENTSETID_VIDCAP_CAMERACONTROL_REGION_OF_INTEREST,
        SIZEOF_ARRAY(VidCapRoiEventTable),
        VidCapRoiEventTable
    )
};

DEFINE_KSAUTOMATION_TABLE(AvsCameraFilterAutomationTable)
{
    DEFINE_KSAUTOMATION_PROPERTIES(PropertySets),
    DEFINE_KSAUTOMATION_METHODS_NULL,
    DEFINE_KSAUTOMATION_EVENTS(EventSets)
};

//FFC
DEFINE_KSPROPERTY_SET_TABLE(PropertySetsFFC)
{
    DEFINE_STD_PROPERTY_SET( PROPSETID_VIDCAP_VIDEOCONTROL,                       FilterVidcapPropertyTable     ),
    DEFINE_STD_PROPERTY_SET( PROPSETID_VIDCAP_CAMERACONTROL_IMAGE_PIN_CAPABILITY, PinDependencePropertyItems    ),
    DEFINE_STD_PROPERTY_SET( KSPROPERTYSETID_ExtendedCameraControl,               ExtendedPropertyItemsFFC      ),
    DEFINE_STD_PROPERTY_SET( PROPSETID_VIDCAP_VIDEOPROCAMP,                       VideoProcampPropertyTable     ),
    DEFINE_STD_PROPERTY_SET( PROPSETID_VIDCAP_CUSTOMCONTROL,                      CustomPropertyTable           )
};

DEFINE_KSEVENT_SET_TABLE(EventSetsFFC)
{
    DEFINE_KSEVENT_SET
    (
        &KSEVENTSETID_ExtendedCameraControl,
        SIZEOF_ARRAY(ExtendedPropertyEventTable),
        ExtendedPropertyEventTable
    )
};

DEFINE_KSAUTOMATION_TABLE(AvsCameraFilterAutomationTableFFC)
{
    DEFINE_KSAUTOMATION_PROPERTIES(PropertySetsFFC),
    DEFINE_KSAUTOMATION_METHODS_NULL,
    DEFINE_KSAUTOMATION_EVENTS(EventSetsFFC)
};

//
// CaptureFilterPinDescriptors:
//
// The list of pin descriptors on the capture filter.
//
const
KSPIN_DESCRIPTOR_EX
PinDescriptors[] =
{
    //
    // Video Capture Pin
    //
    {
        &VideoCapturePinDispatch,
        NULL,
        {
            0,                              // Interfaces (NULL, 0 == default)
            NULL,
            0,                              // Mediums (NULL, 0 == default)
            NULL,
            SIZEOF_ARRAY(VideoCapturePinDataRanges),// Range Count
            VideoCapturePinDataRanges,           // Ranges
            KSPIN_DATAFLOW_OUT,             // Dataflow
            KSPIN_COMMUNICATION_BOTH,       // Communication
            &PIN_CATEGORY_CAPTURE,          // Category
            &g_PINNAME_VIDEO_CAPTURE,       // Name
            0                               // Reserved
        },
        KSPIN_FLAG_PROCESS_IN_RUN_STATE_ONLY,
        1,                                  // Instances Possible
        0,                                  // Instances Necessary
        &VideoCapturePinAllocatorFraming,        // Allocator Framing
        reinterpret_cast <PFNKSINTERSECTHANDLEREX>
        (CVideoCapturePin::IntersectHandler)
    },
    //
    // Video Preview Pin
    //
    {
        &VideoCapturePinDispatch,
        NULL,
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(VideoPreviewPinDataRanges),// Range Count
            VideoPreviewPinDataRanges,           // Ranges
            KSPIN_DATAFLOW_OUT,             // Dataflow
            KSPIN_COMMUNICATION_BOTH,       // Communication
            &PIN_CATEGORY_PREVIEW,          // Category
            &g_PINNAME_VIDEO_PREVIEW,       // Name
            0                               // Reserved
        },
        KSPIN_FLAG_PROCESS_IN_RUN_STATE_ONLY,
        1,                                  // Instances Possible
        0,                                  // Instances Necessary
        &VideoCapturePinAllocatorFraming,        // Allocator Framing
        reinterpret_cast <PFNKSINTERSECTHANDLEREX>
        (CVideoCapturePin::IntersectHandler)
    },
    //
    // Image Capture Pin
    //
    {
        &ImageCapturePinDispatch,
        NULL,
        {
            0,                              // Interfaces (NULL, 0 == default)
            NULL,
            0,                              // Mediums (NULL, 0 == default)
            NULL,
            SIZEOF_ARRAY(ImageCapturePinDataRanges),// Range Count
            ImageCapturePinDataRanges,           // Ranges
            KSPIN_DATAFLOW_OUT,             // Dataflow
            KSPIN_COMMUNICATION_BOTH,       // Communication
            &PINNAME_IMAGE,          // Category
            &g_PINNAME_IMAGE_CAPTURE,          // Name
            0                               // Reserved
        },
//////////////////////////////////////////////////////////////////////////////////////////////////////
        KSPIN_FLAG_INITIATE_PROCESSING_ON_EVERY_ARRIVAL |
//////////////////////////////////////////////////////////////////////////////////////////////////////
        KSPIN_FLAG_PROCESS_IN_RUN_STATE_ONLY,
        1,                                  // Instances Possible
        0,                                  // Instances Necessary
        &ImageCapturePinAllocatorFraming,        // Allocator Framing
        reinterpret_cast <PFNKSINTERSECTHANDLEREX>
        (CImageCapturePin::IntersectHandler)
    }
};

//
// CaptureFilterDispatch:
//
// This is the dispatch table for the capture filter.  It provides notification
// of creation, closure, processing (for filter-centrics, not for the capture
// filter), and resets (for filter-centrics, not for the capture filter).
//
const
KSFILTER_DISPATCH
AvsCameraFilterDispatch =
{
    CAvsCameraFilter::DispatchCreate,       // Filter Create
    CCaptureFilter::DispatchClose,          // Filter Close
    NULL,                                   // Filter Process
    NULL                                    // Filter Reset
};

// {B27E3887-AD10-4A4E-BFB8-D6765ADD0E38}
const
GUID
AvsCam_FrontCamera_Filter = {STATIC_FrontCamera_Filter};

// {4EE16166-F358-4F10-8889-93107806B7A7}
const
GUID
AvsCam_RearCamera_Filter = {STATIC_RearCamera_Filter};

//
// AvsCameraFilterDescriptor:
//
// The descriptor for the capture filter.  We don't specify any topology
// since there's only one pin on the filter.  Realistically, there would
// be some topological relationships here because there would be input
// pins from crossbars and the like.
//
const
KSFILTER_DESCRIPTOR
AvsCameraFilterDescriptor =
{
    &AvsCameraFilterDispatch,               // Dispatch Table
    &AvsCameraFilterAutomationTable,        // Automation Table
    KSFILTER_DESCRIPTOR_VERSION,            // Version
    KSFILTER_FLAG_PRIORITIZE_REFERENCEGUID, // Flags
    &AvsCam_RearCamera_Filter,                 // Reference GUID
    DEFINE_KSFILTER_PIN_DESCRIPTORS(PinDescriptors),
    DEFINE_KSFILTER_CATEGORIES(FilterCategories),
    0,
    sizeof (KSNODE_DESCRIPTOR),
    NULL,
    0,
    NULL,
    NULL                                    // Component ID
};

const
KSFILTER_DESCRIPTOR
AvsCameraFilterDescriptorFFC =
{
    &AvsCameraFilterDispatch,               // Dispatch Table
    &AvsCameraFilterAutomationTableFFC,     // Automation Table
    KSFILTER_DESCRIPTOR_VERSION,            // Version
    KSFILTER_FLAG_PRIORITIZE_REFERENCEGUID, // Flags
    &AvsCam_FrontCamera_Filter,          // Reference GUID
    DEFINE_KSFILTER_PIN_DESCRIPTORS(PinDescriptors),
    DEFINE_KSFILTER_CATEGORIES(FilterCategories),
    0,
    sizeof (KSNODE_DESCRIPTOR),
    NULL,
    0,
    NULL,
    NULL                                    // Component ID
};

