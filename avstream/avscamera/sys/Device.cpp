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

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA

#ifndef DEVPKEY_Device_PanelId
DEFINE_DEVPROPKEY(DEVPKEY_Device_PanelId, 0x8dbc9c86, 0x97a9, 0x4bff, 0x9b, 0xc6, 0xbf, 0xe9, 0x5d, 0x3e, 0x6d, 0xad, 2);     // DEVPROP_TYPE_STRING
#endif

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
IrpSynchronousCompletion(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID          pKevent
    )
{
    if (Irp->PendingReturned)
    {
        NT_ASSERT(pKevent);
        KeSetEvent((PRKEVENT)pKevent, 0, FALSE);
    }

    return STATUS_MORE_PROCESSING_REQUIRED;
}

_Must_inspect_result_
NTSTATUS
CCaptureDevice::
QueryForInterface(
    _In_ PDEVICE_OBJECT TopOfStack,
    _In_ const GUID* InterfaceType,
    _Out_ PINTERFACE Interface,
    _In_ USHORT Size,
    _In_ USHORT Version,
    _In_opt_ PVOID InterfaceSpecificData
    )
{
    PAGED_CODE();

    PIRP pIrp;
    NTSTATUS status;

    if (TopOfStack == nullptr)
    {
        return STATUS_INVALID_PARAMETER;
    }

    pIrp = IoAllocateIrp(TopOfStack->StackSize, FALSE);

    if (pIrp != NULL)
    {
        PIO_STACK_LOCATION stack;
        KEVENT event;

        KeInitializeEvent(&event, NotificationEvent, FALSE);

        IoSetCompletionRoutine(pIrp,
            IrpSynchronousCompletion,
            &event,
            TRUE,
            TRUE,
            TRUE);

        pIrp->IoStatus.Status = STATUS_NOT_SUPPORTED;

        stack = IoGetNextIrpStackLocation(pIrp);

        stack->MajorFunction = IRP_MJ_PNP;
        stack->MinorFunction = IRP_MN_QUERY_INTERFACE;

        stack->Parameters.QueryInterface.Interface = Interface;
        stack->Parameters.QueryInterface.InterfaceSpecificData = InterfaceSpecificData;
        stack->Parameters.QueryInterface.Size = Size;
        stack->Parameters.QueryInterface.Version = Version;
        stack->Parameters.QueryInterface.InterfaceType = InterfaceType;

        status = IoCallDriver(TopOfStack, pIrp);

        if (status == STATUS_PENDING)
        {
            KeWaitForSingleObject(
                &event,
                Executive,
                KernelMode,
                FALSE, // Not alertable
                NULL
                );

            status = pIrp->IoStatus.Status;
        }

        IoFreeIrp(pIrp);
    }
    else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;
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

    PKSDEVICE Device = m_Device;

    //
    // Add the item to the object bag if we were successful.
    // Whenever the device goes away, the bag is cleaned up and
    // we will be freed.
    //
    // For backwards compatibility with DirectX 8.0, we must grab
    // the device mutex before doing this.  For Windows XP, this is
    // not required, but it is still safe.
    //
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
                ACPI_PLD_V2_BUFFER pld = {0};
                pld.Revision = 2;
                pld.Panel = m_Context[i].AcpiPosition;
                pld.Rotation = m_Context[i].AcpiRotation;
                pld.CabinetNumber = 0;
                WCHAR PanelId[MAX_PATH] = { 0 };
                size_t cchDest = MAX_PATH;

                IFFAILED_EXIT(
                    RtlStringCchPrintfW(PanelId, cchDest, L"{00000000-0000-0000-ffff-ffffffffffff}\\%04X\\%u", pld.CabinetNumber, PnpAcpiPanelSideMap[pld.Panel]));

                IFFAILED_EXIT(
                    IoSetDeviceInterfacePropertyData(SymbolicLinkName,
                                                     &DEVPKEY_Device_PhysicalDeviceLocation,
                                                     LOCALE_NEUTRAL,
                                                     PLUGPLAY_PROPERTY_PERSISTENT,
                                                     DEVPROP_TYPE_BINARY,
                                                     sizeof(pld),
                                                     (PVOID)&pld)
                );

                // Also set Panel ID manually, because PnP does not do this for the interface currently
                IoSetDeviceInterfacePropertyData(SymbolicLinkName,
                    &DEVPKEY_Device_PanelId,
                    LOCALE_NEUTRAL,
                    PLUGPLAY_PROPERTY_PERSISTENT,
                    DEVPROP_TYPE_STRING,
                    (ULONG)(wcslen(PanelId)+1)*sizeof(WCHAR),
                    (PVOID)PanelId);
            }

            //  On a real device this object would be constructed whenever the sensor hardware is ready...
            m_Sensor[i] = CreateSensor( m_Context[i].Descriptor );
            if( !m_Sensor[i] )
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            IFFAILED_EXIT(m_Sensor[i]->Initialize());

            m_Sensor[i]->SetMountingOrientation( m_Context[i].AcpiRotation );
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

