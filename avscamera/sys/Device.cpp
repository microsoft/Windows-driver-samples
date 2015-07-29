/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2001, Microsoft Corporation.

    File:

        device.cpp

    Abstract:

    This file contains the implementation of CCaptureDevice.

    CCaptureDevice is the base device class object.  It handles basic PnP
    requests and constructs the filter factories.  

    History:

        created 3/9/2001

**************************************************************************/

#include "Common.h"

/**************************************************************************

    PAGEABLE CODE

**************************************************************************/

/////
/// Structures...
//// Profile 0
static KSCAMERA_PROFILE_MEDIAINFO s_Profile0_PreviewMediaInfo[] =
{
    //{ resolution }, {  fps   }, Flags, Data0, Data1, Data2, Data 3
    { { 1920, 1080 }, {  30, 1 },     0,     0,     0,     0,     0 },
    { { 1280,  720 }, {  30, 1 },     0,     0,     0,     0,     0 },
    { {  640,  360 }, {  30, 1 },     0,     0,     0,     0,     0 },
};

static KSCAMERA_PROFILE_MEDIAINFO s_Profile0_RecordMediaInfo[] =
{
    //{ resolution }, {  fps   }, Flags, Data0, Data1, Data2, Data 3
    { { 3840, 2160 }, {  30, 1 },     0,     0,     0,     0,     0 },
    { { 1920, 1080 }, { 120, 1 },     0,     0,     0,     0,     0 },
    { { 1920, 1080 }, {  90, 1 },     0,     0,     0,     0,     0 },
    { { 1920, 1080 }, {  60, 1 },     0,     0,     0,     0,     0 },
    { { 1920, 1080 }, {  30, 1 },     0,     0,     0,     0,     0 },
    { { 1280,  720 }, {  30, 1 },     0,     0,     0,     0,     0 },
    { {  640,  360 }, {  30, 1 },     0,     0,     0,     0,     0 },
};

static KSCAMERA_PROFILE_MEDIAINFO s_Profile0_PhotoMediaInfo[] =
{
    //{ resolution }, {  fps   }, Flags, Data0, Data1, Data2, Data 3
    { { 1920, 1080 }, {  30, 1 },     0,     0,     0,     0,     0 },
    { { 1280,  720 }, {  30, 1 },     0,     0,     0,     0,     0 },
    { {  640,  360 }, {  30, 1 },     0,     0,     0,     0,     0 }
};


//// Profile 1
static KSCAMERA_PROFILE_MEDIAINFO s_Profile1_PreviewMediaInfo[] =
{
    //{ resolution }, {  fps   }, Flags, Data0, Data1, Data2, Data 3
    { { 1920, 1080 }, {  30, 1 },     0,     0,     0,     0,     0 },
    { { 1280,  720 }, {  30, 1 },     0,     0,     0,     0,     0 },
    { {  640,  360 }, {  30, 1 },     0,     0,     0,     0,     0 }
};

static KSCAMERA_PROFILE_MEDIAINFO s_Profile1_PhotoMediaInfo[] =
{
    //{ resolution }, {  fps   }, Flags, Data0, Data1, Data2, Data 3
    { { 7680, 4320 }, {   0, 1 },     0,     0,     0,     0,     0 },
    { { 3840, 2160 }, {  30, 1 },     0,     0,     0,     0,     0 },
    { { 1920, 1080 }, {  30, 1 },     0,     0,     0,     0,     0 },
    { { 1280,  720 }, {  30, 1 },     0,     0,     0,     0,     0 },
    { {  640,  360 }, {  30, 1 },     0,     0,     0,     0,     0 }
};

//// Profile 2
static KSCAMERA_PROFILE_MEDIAINFO s_Profile2_PreviewMediaInfo[] =
{
    //{ resolution }, {  fps   }, Flags, Data0, Data1, Data2, Data 3
    { { 1920, 1080 }, {  30, 1 },     0,     0,     0,     0,     0 },
    { { 1280,  720 }, {  30, 1 },     0,     0,     0,     0,     0 },
    { {  640,  360 }, {  30, 1 },     0,     0,     0,     0,     0 }
};

static KSCAMERA_PROFILE_MEDIAINFO s_Profile2_RecordMediaInfo[] =
{
    //{ resolution }, {  fps   }, Flags, Data0, Data1, Data2, Data 3
    { { 1920, 1080 }, {  30, 1 },     0,     0,     0,     0,     0 },
    { { 1280,  720 }, {  30, 1 },     0,     0,     0,     0,     0 },
    { {  640,  360 }, {  30, 1 },     0,     0,     0,     0,     0 }
};

static KSCAMERA_PROFILE_PININFO s_Profile0_PinInfo[] =
{
    { {STATIC_PINNAME_VIDEO_PREVIEW}, 0, ARRAYSIZE(s_Profile0_PreviewMediaInfo), s_Profile0_PreviewMediaInfo },
    { {STATIC_PINNAME_VIDEO_CAPTURE}, 0, ARRAYSIZE(s_Profile0_RecordMediaInfo),  s_Profile0_RecordMediaInfo },
    { {STATIC_PINNAME_IMAGE},         0, ARRAYSIZE(s_Profile0_PhotoMediaInfo),   s_Profile0_PhotoMediaInfo }
};

static KSCAMERA_PROFILE_PININFO s_Profile1_PinInfo[] =
{
    { {STATIC_PINNAME_VIDEO_PREVIEW}, 0, ARRAYSIZE(s_Profile1_PreviewMediaInfo), s_Profile1_PreviewMediaInfo },
    { {STATIC_PINNAME_IMAGE},         0, ARRAYSIZE(s_Profile1_PhotoMediaInfo),   s_Profile1_PhotoMediaInfo }
};

static KSCAMERA_PROFILE_PININFO s_Profile2_PinInfo[] =
{
    { {STATIC_PINNAME_VIDEO_PREVIEW}, 0, ARRAYSIZE(s_Profile2_PreviewMediaInfo), s_Profile2_PreviewMediaInfo },
    { {STATIC_PINNAME_VIDEO_CAPTURE}, 0, ARRAYSIZE(s_Profile2_RecordMediaInfo),  s_Profile2_RecordMediaInfo }
};

static KSCAMERA_PROFILE_INFO s_Profiles[] =
{
    { {STATIC_KSCAMERAPROFILE_BalancedVideoAndPhoto}, 0, ARRAYSIZE(s_Profile0_PinInfo), s_Profile0_PinInfo },
    { {STATIC_KSCAMERAPROFILE_HighQualityPhoto},      0, ARRAYSIZE(s_Profile1_PinInfo), s_Profile1_PinInfo },
    { {STATIC_KSCAMERAPROFILE_VideoRecording},        0, ARRAYSIZE(s_Profile2_PinInfo), s_Profile2_PinInfo }
};

// The front camera is concurrent with the rear camera, so the front camera concurrency
// has the back camera's reference GUID and vice versa.
static KSCAMERA_PROFILE_CONCURRENCYINFO s_Profile2_ConcurrencyInfoFront[] =
{
    { {STATIC_RearCamera_Filter}, 0, 1, &s_Profiles[2] }
};

static KSCAMERA_PROFILE_CONCURRENCYINFO s_Profile2_ConcurrencyInfoBack[] =
{
    { {STATIC_FrontCamera_Filter}, 0, 1, &s_Profiles[2] }
};

const UINT32 s_ProfileCount = 3;

UINT32
InitializeDeviceProfiles(
    _In_    BOOL fFrontCamera,
    _Outptr_result_maybenull_
    PKSDEVICE_PROFILE_INFO *ppDeviceProfiles
)
{
    UINT32                  uiProfileCount = 0;
    PKSDEVICE_PROFILE_INFO  pDeviceProfiles = NULL;

    pDeviceProfiles = (PKSDEVICE_PROFILE_INFO)ExAllocatePoolWithTag( PagedPool, sizeof(KSDEVICE_PROFILE_INFO) * ARRAYSIZE(s_Profiles) , 'fpSC');
    if( !pDeviceProfiles )
    {
        // Can't publish, we're out of memory, silently fail here.
        *ppDeviceProfiles = nullptr;
        goto Exit;
    }

    for (UINT32 i = 0; i < ARRAYSIZE(s_Profiles); i++)
    {
        pDeviceProfiles[i].Type = KSDEVICE_PROFILE_TYPE_CAMERA;
        pDeviceProfiles[i].Size = sizeof(KSDEVICE_PROFILE_INFO);
        pDeviceProfiles[i].Camera.Info = s_Profiles[i];
        pDeviceProfiles[i].Camera.Reserved = 0;
        pDeviceProfiles[i].Camera.ConcurrencyCount = 0;
        pDeviceProfiles[i].Camera.Concurrency = NULL;
    }

    pDeviceProfiles[2].Camera.ConcurrencyCount = 1;
    if (fFrontCamera)
    {
        pDeviceProfiles[2].Camera.Concurrency = (PKSCAMERA_PROFILE_CONCURRENCYINFO)&s_Profile2_ConcurrencyInfoFront;
    }
    else
    {
        pDeviceProfiles[2].Camera.Concurrency = (PKSCAMERA_PROFILE_CONCURRENCYINFO)&s_Profile2_ConcurrencyInfoBack;
    }

    uiProfileCount = ARRAYSIZE(s_Profiles);
    *ppDeviceProfiles = pDeviceProfiles;
    pDeviceProfiles = NULL;

Exit:
    return uiProfileCount;
}

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA

CCaptureDevice::
CCaptureDevice (
    _In_ PKSDEVICE Device
)
    : m_Device (Device)
    , m_FilterDescriptorCount(0)
    , m_Sensor(nullptr)
    , m_Context(nullptr)
{
    PAGED_CODE();
}

CCaptureDevice::
~CCaptureDevice()
{
    PAGED_CODE();

    if( m_Sensor )
    {
        for( size_t i=0; i<m_FilterDescriptorCount; i++ )
        {
            SAFE_DELETE( m_Sensor[i] );
        }

        SAFE_DELETE_ARRAY( m_Sensor );
    }
}

NTSTATUS
CCaptureDevice::
Initialize()
{
    PAGED_CODE();

    m_Sensor = new (NonPagedPoolNx, 'iveD') CSensor *[m_FilterDescriptorCount];

    if( !m_Sensor )
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    return STATUS_SUCCESS;
}

ULONG
CCaptureDevice::
GetFilterIndex(PKSFILTER Filter)
{
    PAGED_CODE();
    ULONG i;

    for( i=0; i<m_FilterDescriptorCount; i++ )
    {
        if( Filter->Descriptor->ReferenceGuid &&
            IsEqualGUID(*(m_Context[i].Descriptor->ReferenceGuid), *Filter->Descriptor->ReferenceGuid))
        {
            break;
        }
    }

    //  This is a complete failure.  We really need to bail.
    NT_ASSERT(i<m_FilterDescriptorCount);

    return i;
}

NTSTATUS
CCaptureDevice::
Prepare()
{
    PAGED_CODE();

    NTSTATUS Status = Initialize();

    if( !NT_SUCCESS(Status) )
    {
        return Status;
    }

    //
    // Add the item to the object bag if we were successful.
    // Whenever the device goes away, the bag is cleaned up and
    // we will be freed.
    //
    // For backwards compatibility with DirectX 8.0, we must grab
    // the device mutex before doing this.  For Windows XP, this is
    // not required, but it is still safe.
    //
    PKSDEVICE Device = m_Device;
    LockDevice Lock(Device);

    Status = KsAddItemToObjectBag (
                 Device -> Bag,
                 reinterpret_cast <PVOID> (this),
                 reinterpret_cast <PFNKSFREE> (CCaptureDevice::Cleanup)
             );

    if( NT_SUCCESS (Status) )
    {
        Device->Context=this;
    }
    return Status;
}

NTSTATUS
CCaptureDevice::
DispatchCreate (
    _In_ PKSDEVICE Device
)

/*++

Routine Description:

    Create the capture device.  This is the creation dispatch for the
    capture device.

Arguments:

    Device -
        The AVStream device being created.

Return Value:

    Success / Failure

--*/

{
    PAGED_CODE();

    DBG_ENTER("()");

    CCaptureDevice *CapDevice = new (NonPagedPoolNx, 'eviD') CCaptureDevice (Device);
    if( !CapDevice)
    {
        //
        //  Return failure if we couldn't create the device.
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS Status = CapDevice->Prepare();
    if (!NT_SUCCESS (Status))
    {
        //  Init failure.
        delete CapDevice;
    }

    DBG_LEAVE("()");
    return Status;
}

//
// DispatchStart():
//
// This is the Pnp Start dispatch for the capture device.  It simply
// bridges to PnpStart() in the context of the CCaptureDevice.
//
NTSTATUS
CCaptureDevice::
DispatchStart (
    _In_ PKSDEVICE Device,
    _In_ PIRP Irp,
    _In_ PCM_RESOURCE_LIST TranslatedResourceList,
    _In_ PCM_RESOURCE_LIST UntranslatedResourceList
)
{
    PAGED_CODE();

    DBG_ENTER("()");

    NTSTATUS Status =
        Recast(Device)->
        PnpStart (
            TranslatedResourceList,
            UntranslatedResourceList
        );

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

//
// DispatchStop():
//
// This is the Pnp stop dispatch for the capture device.  It simply
// bridges to PnpStop() in the context of the CCaptureDevice.
//
void
CCaptureDevice::
DispatchStop (
    _In_ PKSDEVICE Device,
    _In_ PIRP Irp
)
{
    PAGED_CODE();

    DBG_ENTER("()");
    Recast(Device)->PnpStop(Irp);
    DBG_LEAVE("()");
}

NTSTATUS
CCaptureDevice::
DispatchPostStart(
    _In_ PKSDEVICE Device
)
{
    PAGED_CODE();

    DBG_ENTER("()");

    NTSTATUS Status =
        Recast(Device)->PnpPostStart();

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

NTSTATUS
CCaptureDevice::
DispatchQueryStop(
    _In_ PKSDEVICE Device,
    _In_ PIRP Irp
)
{
    PAGED_CODE();

    DBG_ENTER("()");

    NTSTATUS Status =
        Recast(Device)->PnpQueryStop(Irp);

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

void
CCaptureDevice::
DispatchCancelStop(
    _In_ PKSDEVICE Device,
    _In_ PIRP Irp
)
{
    PAGED_CODE();

    DBG_ENTER("()");
    Recast(Device)->PnpCancelStop(Irp);
    DBG_LEAVE("()");
}

NTSTATUS
CCaptureDevice::
DispatchQueryRemove(
    _In_ PKSDEVICE Device,
    _In_ PIRP Irp
)
{
    PAGED_CODE();

    DBG_ENTER("()");

    NTSTATUS Status =
        Recast(Device)->PnpQueryRemove(Irp);

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

void
CCaptureDevice::
DispatchCancelRemove(
    _In_ PKSDEVICE Device,
    _In_ PIRP Irp
)
{
    PAGED_CODE();

    DBG_ENTER("()");
    Recast(Device)->PnpCancelRemove(Irp);
    DBG_LEAVE("()");
}

void
CCaptureDevice::
DispatchRemove(
    _In_ PKSDEVICE Device,
    _In_ PIRP Irp
)
{
    PAGED_CODE();

    DBG_ENTER("()");
    Recast(Device)->PnpRemove(Irp);
    DBG_LEAVE("()");
}

void
CCaptureDevice::
DispatchSurpriseRemoval(
    _In_ PKSDEVICE Device,
    _In_ PIRP Irp
)
{
    PAGED_CODE();

    DBG_ENTER("()");
    Recast(Device)->PnpSurpriseRemoval(Irp);
    DBG_LEAVE("()");
}

NTSTATUS
CCaptureDevice::
DispatchQueryCapabilities(
    _In_ PKSDEVICE Device,
    _In_ PIRP Irp,
    _Inout_ PDEVICE_CAPABILITIES Capabilities
)
{
    PAGED_CODE();

    DBG_ENTER("()");

    NTSTATUS Status =
        Recast(Device)->
        PnpQueryCapabilities( Irp, Capabilities );

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}


//  Make Power callbacks non-pageable.
#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif // ALLOC_PRAGMA

NTSTATUS
CCaptureDevice::
DispatchQueryPower(
    _In_ PKSDEVICE Device,
    _In_ PIRP Irp,
    _In_ DEVICE_POWER_STATE DeviceTo,
    _In_ DEVICE_POWER_STATE DeviceFrom,
    _In_ SYSTEM_POWER_STATE SystemTo,
    _In_ SYSTEM_POWER_STATE SystemFrom,
    _In_ POWER_ACTION Action
)
{
    //  If this device supports the the DO_POWER_INRUSH flag, this function
    //  can be called at DISPATCH.

    DBG_ENTER("()");
    NTSTATUS Status =
        Recast(Device)->
        PnpQueryPower(
            Irp,
            DeviceTo,
            DeviceFrom,
            SystemTo,
            SystemFrom,
            Action );

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}

void
CCaptureDevice::
DispatchSetPower(
    _In_ PKSDEVICE Device,
    _In_ PIRP Irp,
    _In_ DEVICE_POWER_STATE To,
    _In_ DEVICE_POWER_STATE From
)
{
    //  If this device supports the the DO_POWER_INRUSH flag, this function
    //  can be called at DISPATCH.

    DBG_ENTER("()");
    Recast(Device)->
    PnpSetPower( Irp, To, From );
    DBG_LEAVE("()");
}

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA

NTSTATUS
CCaptureDevice::
DispatchQueryInterface(
    _In_ PKSDEVICE Device,
    _In_ PIRP Irp
)
{
    PAGED_CODE();

    DBG_ENTER("()");

    NTSTATUS Status =
        Recast(Device)->
        PnpQueryInterface(Irp);

    DBG_LEAVE("()=0x%08X", Status);
    return Status;
}


/*************************************************/


NTSTATUS
CCaptureDevice::
PnpStart (
    _In_ PCM_RESOURCE_LIST TranslatedResourceList,
    _In_ PCM_RESOURCE_LIST UntranslatedResourceList
)

/*++

Routine Description:

    Called at Pnp start.  We start up our  hardware simulation.

Arguments:

    TranslatedResourceList -
        The translated resource list from Pnp

    UntranslatedResourceList -
        The untranslated resource list from Pnp

Return Value:

    Success / Failure

--*/

{

    PAGED_CODE();

    //
    // Normally, we'd do things here like parsing the resource lists and
    // connecting our interrupt.  Since this is a simulation, there isn't
    // much to parse.  The parsing and connection should be the same as
    // any WDM driver.  The sections that will differ are illustrated below
    // in setting up a simulated DMA.
    //

    NTSTATUS Status = STATUS_SUCCESS;

    //
    // By PnP, it's possible to receive multiple starts without an intervening
    // stop (to reevaluate resources, for example).  Thus, we only perform
    // creations of the simulation on the initial start and ignore any
    // subsequent start.  Hardware drivers with resources should evaluate
    // resources and make changes on 2nd start.
    //
    // Walk the list of descriptors and enumerate the h/w sims described.
    //

    if (!m_Device -> Started)
    {
        // Create the Filter for the device
        for( size_t i=0; i<m_FilterDescriptorCount; i++ )
        {
            PKSFILTERFACTORY FilterFactory=nullptr;
            IFFAILED_EXIT(
                KsCreateFilterFactory( m_Device->FunctionalDeviceObject,
                                       m_Context[i].Descriptor,
                                       m_Context[i].Name,
                                       NULL,
                                       KSCREATE_ITEM_FREEONSTOP,
                                       NULL,
                                       NULL,
                                       &FilterFactory )
            );

            // Set primary camera to its interface, this allows device to be queried by GUID
            if( FilterFactory )
            {
                PKSDEVICE_PROFILE_INFO pDeviceProfiles = nullptr;
                UINT32 uiProfileCount = 0;
                PUNICODE_STRING SymbolicLinkName = KsFilterFactoryGetSymbolicLink(FilterFactory);
                ACPI_PLD_BUFFER pld = {0};
                BOOL fFrontCamera = FALSE;
                pld.Panel = m_Context[i].AcpiPosition;
                static
                const
                GUID FFC_Filter = {STATIC_FrontCamera_Filter};

                IFFAILED_EXIT(
                    IoSetDeviceInterfacePropertyData(SymbolicLinkName,
                                                     &DEVPKEY_Device_PhysicalDeviceLocation,
                                                     LOCALE_NEUTRAL,
                                                     PLUGPLAY_PROPERTY_PERSISTENT,
                                                     DEVPROP_TYPE_BINARY,
                                                     sizeof(ACPI_PLD_BUFFER),
                                                     (PVOID)&pld)
                );

                if( m_Context[i].Descriptor->ReferenceGuid &&
                    IsEqualGUID(*(m_Context[i].Descriptor->ReferenceGuid), FFC_Filter))
                {
                    fFrontCamera = TRUE;
                }

                // Publish our profile here.
                uiProfileCount = InitializeDeviceProfiles(fFrontCamera, &pDeviceProfiles);
                if( uiProfileCount > 0 )
                {
                    if( NT_SUCCESS(KsInitializeDeviceProfile(FilterFactory)) )
                    {
                        for( UINT32 j=0; j<uiProfileCount; j++ )
                        {
                            if( !NT_SUCCESS(KsPublishDeviceProfile(FilterFactory, &pDeviceProfiles[j])) )
                            {
                                // Bail...
                                break;
                            }
                        }
                        if( !NT_SUCCESS(KsPersistDeviceProfile(FilterFactory)) )
                        {
                            // Trace here?
                        }
                    }

                    ExFreePool(pDeviceProfiles);
                    pDeviceProfiles = NULL;
                }
            }

            //  On a real device this object would be constructed whenever the sensor hardware is ready...
            m_Sensor[i] = CreateSensor( m_Context[i].Descriptor );
            if( !m_Sensor[i] )
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            IFFAILED_EXIT(m_Sensor[i]->Initialize());
        }
    }

done:
    if( !NT_SUCCESS(Status) )
    {
        //  Free any sensor objects we created during this failed attempt.
        for( size_t i=0; i<m_FilterDescriptorCount; i++ )
        {
            SAFE_DELETE( m_Sensor[i] );
        }
    }
    return Status;
}

CSensor *
CCaptureDevice::
CreateSensor(
    _In_    const KSFILTER_DESCRIPTOR  *Descriptors
)
{
    PAGED_CODE();

    return new (NonPagedPoolNx, 'sneS') CSensorSimulation( this, Descriptors );
}

PDEVICE_OBJECT
CCaptureDevice::
GetDeviceObject()
/*++

Routine Description:

    Return the pointer to the FDO for our device.

Arguments:

    [none]

Return Value:

    PDEVICE_OBJECT

--*/
{
    PAGED_CODE();

    return m_Device->FunctionalDeviceObject;
}

/*************************************************/


void
CCaptureDevice::
PnpStop (
    _In_ PIRP Irp
)

/*++

Routine Description:

    This is the pnp stop dispatch for the capture device.  It releases any
    adapter object previously allocated by IoGetDmaAdapter during Pnp Start.

Arguments:

    None

Return Value:

    None

--*/

{
    UNREFERENCED_PARAMETER(Irp);
    PAGED_CODE();

    if (m_DmaAdapterObject)
    {
        //
        // Return the DMA adapter back to the system.
        //
        m_DmaAdapterObject -> DmaOperations ->
        PutDmaAdapter (m_DmaAdapterObject);

        m_DmaAdapterObject = NULL;
    }
}

NTSTATUS
CCaptureDevice::
PnpPostStart()
{
    PAGED_CODE();
    return STATUS_SUCCESS;
}

NTSTATUS
CCaptureDevice::
PnpQueryStop(
    _In_ PIRP Irp
)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(Irp);
    return STATUS_SUCCESS;
}

void
CCaptureDevice::
PnpCancelStop(
    _In_ PIRP Irp
)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(Irp);
}

NTSTATUS
CCaptureDevice::
PnpQueryRemove(
    _In_ PIRP Irp
)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(Irp);
    return STATUS_SUCCESS;
}

void
CCaptureDevice::
PnpCancelRemove(
    _In_ PIRP Irp
)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(Irp);
}

void
CCaptureDevice::
PnpRemove(
    _In_ PIRP Irp
)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(Irp);
}

void
CCaptureDevice::
PnpSurpriseRemoval(
    _In_ PIRP Irp
)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(Irp);
}

NTSTATUS
CCaptureDevice::
PnpQueryCapabilities(
    _In_ PIRP Irp,
    _Inout_ PDEVICE_CAPABILITIES Capabilities
)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(Capabilities);
    return STATUS_SUCCESS;
}

//  Make Power callbacks non-pageable.
#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif // ALLOC_PRAGMA

NTSTATUS
CCaptureDevice::
PnpQueryPower(
    _In_ PIRP Irp,
    _In_ DEVICE_POWER_STATE DeviceTo,
    _In_ DEVICE_POWER_STATE DeviceFrom,
    _In_ SYSTEM_POWER_STATE SystemTo,
    _In_ SYSTEM_POWER_STATE SystemFrom,
    _In_ POWER_ACTION Action
)
{
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(DeviceTo);
    UNREFERENCED_PARAMETER(DeviceFrom);
    UNREFERENCED_PARAMETER(SystemTo);
    UNREFERENCED_PARAMETER(SystemFrom);
    UNREFERENCED_PARAMETER(Action);
    return STATUS_SUCCESS;
}

void
CCaptureDevice::
PnpSetPower(
    _In_ PIRP Irp,
    _In_ DEVICE_POWER_STATE To,
    _In_ DEVICE_POWER_STATE From
)
{
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(To);
    UNREFERENCED_PARAMETER(From);
}

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA

NTSTATUS
CCaptureDevice::
PnpQueryInterface(
    _In_ PIRP Irp
)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(Irp);
    return STATUS_SUCCESS;
}

/*************************************************/

