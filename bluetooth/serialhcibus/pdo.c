/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    Pdo.c

Abstract:

    This module create a PDO and handles plug & play calls for the child device (PDO).

Environment:

    kernel mode only

--*/

#include "driver.h"
#include "pdo.tmh"


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PdoCreate)
#pragma alloc_text(PAGE, PdoDevD0Exit)
#pragma alloc_text(PAGE, PdoDevD0Entry)
#pragma alloc_text(PAGE, PdoDevPrepareHardware)
#pragma alloc_text(PAGE, PdoDevReleaseHardware)
#pragma alloc_text(PAGE, PdoResetHandler)
#ifdef DYNAMIC_ENUM
#pragma alloc_text(PAGE, PdoCreateDynamic)
#pragma alloc_text(PAGE, PdoResetHandlerDynamic)
#endif // DYNAMIC_ENUM
#endif // ALLOC_PRAGMA

#define MAX_ID_LEN 80

#ifdef DYNAMIC_ENUM

_Use_decl_annotations_
NTSTATUS
PdoResetHandlerDynamic(
    PVOID               _InterfaceContext,
    DEVICE_RESET_TYPE   _ResetType,
    ULONG               _Flags,
    PVOID               _ResetParameters
    )
/*++

Routine Description:

    This is the reset handler invoked by a driver that queries our GUID_DEVICE_RESET_INTERFACE_STANDARD interface.
    This is the version used if PDOs are created using WDF dynamic enumeration.

Arguments:

    _InterfaceContext - This is expected to be a PPDO_EXTENSION.

    _ResetType - This is expected to be PlatformLevelDeviceReset because that is all we support.

    _Flags - Unused, because no flags are defined currently.

    _ResetParameters - Unused, because it is only used for FunctionLevelDeviceReset.

Return Value:

    NTSTATUS code.

--*/
{
    PPDO_EXTENSION PdoExtension = (PPDO_EXTENSION) _InterfaceContext;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(_Flags);
    UNREFERENCED_PARAMETER(_ResetParameters);

    if (_ResetType != PlatformLevelDeviceReset) {
        return STATUS_NOT_SUPPORTED;
    }

    DeviceDoPLDR(PdoExtension->FdoExtension->WdfDevice);

    //
    // The bus driver is expected to cause the device to be surprise removed as part of PLDR. For
    // many hardware buses, this happens naturally, but for a software bus like us, we need to do it
    // ourselves. WDF dynamic enumeration makes this convenient because it already supports
    // GUID_REENUMERATE_SELF_INTERFACE_STANDARD, so we can simply call WdfDeviceSetFailed to
    // re-enumerate the device.
    //

    WdfDeviceSetFailed((WDFDEVICE) WdfObjectContextGetObject(PdoExtension), WdfDeviceFailedAttemptRestart);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
PdoCreateDynamic(
    WDFDEVICE       _Device,
    PWDFDEVICE_INIT _DeviceInit,
    PWCHAR          _HardwareIds,
    ULONG           _SerialNo
    )
/*++

Routine Description:

    This routine creates and initialize a PDO.

Arguments:

Return Value:

    NT Status code.

--*/
{
    NTSTATUS                        Status;
    PPDO_EXTENSION                  PdoExtension = NULL;
    WDFDEVICE                       ChildDevice = NULL;
    WDF_OBJECT_ATTRIBUTES           PdoAttributes;
    WDF_DEVICE_PNP_CAPABILITIES     PnpCaps;
    WDF_DEVICE_POWER_CAPABILITIES   PowerCaps;
    DECLARE_CONST_UNICODE_STRING(   CompatId, BT_PDO_COMPATIBLE_IDS);
    DECLARE_CONST_UNICODE_STRING(   DeviceLocation, L"Serial HCI Bus - Bluetooth Function");
    DECLARE_UNICODE_STRING_SIZE(    Buffer, MAX_ID_LEN);
    DECLARE_UNICODE_STRING_SIZE(    DeviceId, MAX_ID_LEN);
    WDF_IO_QUEUE_CONFIG             QueueConfig;
    WDFQUEUE                        Queue;
    WDF_QUERY_INTERFACE_CONFIG      DeviceResetInterfaceConfig;
    DEVICE_RESET_INTERFACE_STANDARD ResetInterface;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(_Device);

    KdPrint(("Entered PdoCreateDynamic\n"));

    //
    // Set DeviceType
    //
    WdfDeviceInitSetDeviceType(_DeviceInit, FILE_DEVICE_BUS_EXTENDER);

    //
    // Provide DeviceID, HardwareIDs, CompatibleIDs and InstanceId
    //
    RtlInitUnicodeString(&DeviceId, _HardwareIds);

    Status = WdfPdoInitAssignDeviceID(_DeviceInit, &DeviceId);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // NOTE: same string  is used to initialize hardware id too
    //
    Status = WdfPdoInitAddHardwareID(_DeviceInit, &DeviceId);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    Status = WdfPdoInitAddCompatibleID(_DeviceInit, &CompatId );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    Status =  RtlUnicodeStringPrintf(&Buffer, L"%02d", _SerialNo);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    Status = WdfPdoInitAssignInstanceID(_DeviceInit, &Buffer);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // Provide a description about the device. This text is usually read from
    // the device. In the case of USB device, this text comes from the string
    // descriptor. This text is displayed momentarily by the PnP manager while
    // it's looking for a matching INF. If it finds one, it uses the Device
    // Description from the INF file or the friendly name created by
    // coinstallers to display in the device manager. FriendlyName takes
    // precedence over the DeviceDesc from the INF file.
    //
    Status = RtlUnicodeStringPrintf( &Buffer,
                                     L"SerialHciBus_%02d",
                                     _SerialNo );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // You can call WdfPdoInitAddDeviceText multiple times, adding device
    // text for multiple locales. When the system displays the text, it
    // chooses the text that matches the current locale, if available.
    // Otherwise it will use the string for the default locale.
    // The driver can specify the driver's default locale by calling
    // WdfPdoInitSetDefaultLocale.
    //
    Status = WdfPdoInitAddDeviceText(_DeviceInit,
                                    &Buffer,
                                    &DeviceLocation,
                                     0x409 );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    WdfPdoInitSetDefaultLocale(_DeviceInit, 0x409);

    //
    // Initialize the attributes to specify the size of PDO device extension.
    // All the state information private to the PDO will be tracked here.
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&PdoAttributes, PDO_EXTENSION);

    //
    // Allow to forward requests to its FDO of this bus driver by using 
    // WdfRequestForwardToParentDeviceIoQueue()in the DeviceIoControl callback.
    //    
    WdfPdoInitAllowForwardingRequestToParent(_DeviceInit);
    
    //
    // Create a framework device object to represent PDO of this bus driver. In response 
    // to this call, framework creates a WDM deviceobject.
    //
    Status = WdfDeviceCreate(&_DeviceInit, 
                             &PdoAttributes, 
                             &ChildDevice);
    
    if (!NT_SUCCESS(Status)) {
        return Status;
    }


    //
    // Note: Once the device is created successfully, framework frees the
    // _DeviceInit memory and sets the _DeviceInit to NULL. So don't
    // call any WdfDeviceInit functions after that.
    //

    //
    // Initalize the PDO extension
    //
    PdoExtension = PdoGetExtension(ChildDevice);

    RtlZeroMemory(PdoExtension, sizeof(PDO_EXTENSION));

    PdoExtension->FdoExtension = FdoGetExtension(_Device);

    PdoExtension->SerialNo = _SerialNo;

    PdoExtension->ResetRecoveryType = SUPPORTED_RESET_RECOVERY_TYPE;

    //
    // Set some properties for the child device.
    //
    WDF_DEVICE_PNP_CAPABILITIES_INIT(&PnpCaps);  // Zeros this structure (note: WdfFalse is 0)

    //
    // Bus driver sets this value to WdfFalse for this embedded device, which cannot 
    // be physically removed; its FDO must not set/override this.
    //
    PnpCaps.Removable         = WdfFalse;      

    //
    // Bus driver sets this value to WdfTrue.  FDO can override this value (when this irp is on its 
    // way up) if it determines that this device cannot be safely surprise (not orderly) removed 
    // without data loss.
    //
    PnpCaps.SurpriseRemovalOK = WdfTrue;  
    
    PnpCaps.Address  = _SerialNo;
    PnpCaps.UINumber = _SerialNo;

    WdfDeviceSetPnpCapabilities(ChildDevice, &PnpCaps);

    WDF_DEVICE_POWER_CAPABILITIES_INIT(&PowerCaps);

    PowerCaps.DeviceD1 = WdfFalse;
    PowerCaps.DeviceD2 = WdfTrue;

    PowerCaps.WakeFromD0 = WdfFalse;    
    PowerCaps.WakeFromD1 = WdfFalse;
    PowerCaps.WakeFromD2 = WdfTrue;    
    PowerCaps.WakeFromD3 = WdfTrue;    
    
    PowerCaps.DeviceWake = PowerDeviceD2;

    PowerCaps.DeviceState[PowerSystemWorking]   = PowerDeviceD0;
    PowerCaps.DeviceState[PowerSystemSleeping1] = PowerDeviceD2;
    PowerCaps.DeviceState[PowerSystemSleeping2] = PowerDeviceD2;
    PowerCaps.DeviceState[PowerSystemSleeping3] = PowerDeviceD2;
    PowerCaps.DeviceState[PowerSystemHibernate] = PowerDeviceD3;
    PowerCaps.DeviceState[PowerSystemShutdown]  = PowerDeviceD3;

    WdfDeviceSetPowerCapabilities(ChildDevice, &PowerCaps);


    //
    // Configure a default queue so that requests that are not
    // configure-forwarded using WdfDeviceConfigureRequestDispatching to goto
    // other queues get dispatched here.
    //

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&QueueConfig, WdfIoQueueDispatchParallel);

    //
    // Cannot be power managed queue (dispatch only at D0) as
    // BthMini issues BthX DDI to get version and capabilities 
    // before enter D0.  A deadlock occurs if this is power managed.
    //
    QueueConfig.PowerManaged = WdfFalse;
    
    QueueConfig.EvtIoDeviceControl = PdoIoQuDeviceControl;

    Status = WdfIoQueueCreate(ChildDevice, 
                              &QueueConfig, 
                              WDF_NO_OBJECT_ATTRIBUTES,
                              &Queue);
    DoTrace(LEVEL_INFO, TFLAG_PNP, (" WdfIoQueueCreate (%!STATUS!)", Status));    
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }       

    if (PdoExtension->ResetRecoveryType == ResetRecoveryTypeParentResetInterface) {

        //
        // Instruct WDF to simply forward a request for the reset interface to the parent stack, so
        // that it reaches the ACPI bus/filter driver. That way when the reset interface is invoked,
        // it is the parent stack that gets reset and re-enumerated.
        //

        WDF_QUERY_INTERFACE_CONFIG_INIT(&DeviceResetInterfaceConfig, NULL, &GUID_DEVICE_RESET_INTERFACE_STANDARD, NULL);
        DeviceResetInterfaceConfig.SendQueryToParentStack = TRUE;

        Status = WdfDeviceAddQueryInterface(ChildDevice, &DeviceResetInterfaceConfig);
        if (!NT_SUCCESS(Status)) {
            DoTrace(LEVEL_ERROR, TFLAG_PNP, ("WdfDeviceAddQueryInterface failed, %!STATUS!", Status));
            goto Cleanup;
        }

    } else if (PdoExtension->ResetRecoveryType == ResetRecoveryTypeDriverImplemented) {

        RtlZeroMemory(&ResetInterface, sizeof(ResetInterface));
        ResetInterface.Size = sizeof(ResetInterface);
        ResetInterface.Version = DEVICE_RESET_INTERFACE_VERSION;
        ResetInterface.Context = PdoExtension;

        //
        // Since this interface is expected to be used only by drivers in the same stack, reference
        // counting is not required. If there is an expectation that drivers may query for the
        // interface using a remote I/O target to this stack (unusual for this interface), reference
        // counting must be implemented.
        //
        ResetInterface.InterfaceReference = WdfDeviceInterfaceReferenceNoOp;
        ResetInterface.InterfaceDereference = WdfDeviceInterfaceDereferenceNoOp;

        ResetInterface.SupportedResetTypes = (1 << PlatformLevelDeviceReset);
        ResetInterface.DeviceReset = PdoResetHandlerDynamic;

        WDF_QUERY_INTERFACE_CONFIG_INIT(&DeviceResetInterfaceConfig, (PINTERFACE) &ResetInterface, &GUID_DEVICE_RESET_INTERFACE_STANDARD, NULL);
        Status = WdfDeviceAddQueryInterface(ChildDevice, &DeviceResetInterfaceConfig);
        if (!NT_SUCCESS(Status))
        {
            DoTrace(LEVEL_ERROR, TFLAG_PNP, ("WdfDeviceAddQueryInterface failed, %!STATUS!", Status));
            goto Cleanup;
        }
    }

Cleanup:


    //
    // Call WdfDeviceInitFree if you encounter an error before the
    // device is created. Once the device is created, framework
    // NULLs the DeviceInit value.
    //
    if (!NT_SUCCESS(Status)) {
        
        DoTrace(LEVEL_ERROR, TFLAG_PNP, (" -PdoCreateDynamic: exit %!STATUS!", Status));        

        if(ChildDevice) {
            WdfObjectDelete(ChildDevice);
        }
    }
    else
    {
        DoTrace(LEVEL_INFO, TFLAG_PNP, (" -PdoCreateDynamic: exit %!STATUS!", Status));          
    }


    return Status;
}

#endif

_Use_decl_annotations_
NTSTATUS
PdoResetHandler(
    PVOID               _InterfaceContext,
    DEVICE_RESET_TYPE   _ResetType,
    ULONG               _Flags,
    PVOID               _ResetParameters
)
/*++

Routine Description:

    This is the reset handler invoked by a driver that queries our GUID_DEVICE_RESET_INTERFACE_STANDARD interface.
    This is the version used if PDOs are created using WDF static enumeration.

Arguments:

    _InterfaceContext - This is expected to be a PPDO_EXTENSION.

    _ResetType - This is expected to be PlatformLevelDeviceReset because that is all we support.

    _Flags - Unused, because no flags are defined currently.

    _ResetParameters - Unused, because it is only used for FunctionLevelDeviceReset.

Return Value:

    NTSTATUS code.

--*/
{
    PPDO_EXTENSION  PdoExtension = (PPDO_EXTENSION) _InterfaceContext;
    WDFDEVICE       Fdo = PdoExtension->FdoExtension->WdfDevice;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(_Flags);
    UNREFERENCED_PARAMETER(_ResetParameters);

    if (_ResetType != PlatformLevelDeviceReset) {
        return STATUS_NOT_SUPPORTED;
    }

    DeviceDoPLDR(Fdo);

    //
    // The bus driver is expected to cause the device to be surprise removed as part of PLDR. For
    // many hardware buses, this happens naturally, but for a software bus like us, we need to do it
    // ourselves.
    //

    FdoRemoveOneChildDevice(Fdo, BLUETOOTH_FUNC_IDS);
    FdoCreateOneChildDevice(Fdo, BT_PDO_HARDWARE_IDS, BLUETOOTH_FUNC_IDS);

    return STATUS_SUCCESS;
}

NTSTATUS
PdoCreate(
    _In_ WDFDEVICE  _Device,
    _In_ PWSTR      _HardwareIds,
    _In_ ULONG      _SerialNo
)
/*++

Routine Description:

    This routine creates and initialize a PDO to service a Bluetooth function.

Arguments:

    _Device - A framework device object
    
    _HardwareIds - a hardware ID for this device

    -SerialNo - serial number of the child DO
    
Return Value:

    NT Status code.

--*/
{
    NTSTATUS                            Status;
    PWDFDEVICE_INIT                     DeviceInit = NULL;
    WDF_PNPPOWER_EVENT_CALLBACKS        PnpPowerCallbacks;
    PPDO_EXTENSION                      PdoExtension = NULL;
    WDFDEVICE                           ChildDevice = NULL;
    WDF_OBJECT_ATTRIBUTES               Attributes;
    WDF_DEVICE_PNP_CAPABILITIES         PnpCaps;
    WDF_DEVICE_POWER_CAPABILITIES       PowerCaps;
    UNICODE_STRING                      StaticString = {0};
    UNICODE_STRING                      DeviceId;
    DECLARE_UNICODE_STRING_SIZE(        Buffer, MAX_ID_LEN);
    UNICODE_STRING ContainerID =        {0};
    WDF_PDO_EVENT_CALLBACKS             Callbacks;
    WDF_IO_QUEUE_CONFIG                 QueueConfig;
    WDFQUEUE                            Queue;
    WDF_QUERY_INTERFACE_CONFIG          DeviceResetInterfaceConfig;
    DEVICE_RESET_INTERFACE_STANDARD     ResetInterface;

    DoTrace(LEVEL_INFO, TFLAG_PNP, (" +PdoCreate: HWID(%S), compatID(%S)", _HardwareIds, BT_PDO_COMPATIBLE_IDS));

    PAGED_CODE();

    //
    // Allocate a WDFDEVICE_INIT structure and set the properties
    // so that we can create a device object for the child.
    //
    DeviceInit = WdfPdoInitAllocate(_Device);
    if (DeviceInit == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Set DeviceType
    //
    WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_BUS_EXTENDER);

    //
    // Provide DeviceID, HardwareIDs, CompatibleIDs and InstanceId
    //
    RtlInitUnicodeString(&DeviceId, _HardwareIds);
    Status = WdfPdoInitAssignDeviceID(DeviceInit, &DeviceId);
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // Note: same string is used to initialize hardware id
    //
    Status = WdfPdoInitAddHardwareID(DeviceInit, &DeviceId);
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    RtlInitUnicodeString(&StaticString, BT_PDO_COMPATIBLE_IDS);
    Status = WdfPdoInitAddCompatibleID(DeviceInit, &StaticString);
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    Status =  RtlUnicodeStringPrintf(&Buffer, L"%02d", _SerialNo);
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }
    Status = WdfPdoInitAssignInstanceID(DeviceInit, &Buffer);
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // Assign the containerID for an internally connected device
    //
    Status = RtlStringFromGUID(&GUID_CONTAINERID_INTERNALLY_CONNECTED_DEVICE, &ContainerID);
    if (!NT_SUCCESS(Status)) {
        DoTrace(LEVEL_ERROR, TFLAG_PNP, ("Failed to generate the ContainerID, %!STATUS!", Status));;
        goto Cleanup;
    }

    Status = WdfPdoInitAssignContainerID(DeviceInit, &ContainerID);
    if (!NT_SUCCESS(Status)) {
        DoTrace(LEVEL_ERROR, TFLAG_PNP, ("Failed to assign the ContainerID, %!STATUS!", Status));
        goto Cleanup;
    }
    
    //
    // Provide a description about the device. This text is usually read from
    // the device. This text is displayed momentarily by the PnP manager while
    // it's looking for a matching INF. If it finds one, it uses the Device
    // Description from the INF file or the friendly name created by
    // coinstallers to display in the device manager. FriendlyName takes
    // precedence over the DeviceDesc from the INF file.
    //
    Status = RtlUnicodeStringPrintf(&Buffer, L"SerialHciBus_%02d", _SerialNo );
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // You can call WdfPdoInitAddDeviceText multiple times, adding device
    // text for multiple locales. When the system displays the text, it
    // chooses the text that matches the current locale, if available.
    // Otherwise it will use the string for the default locale.
    // The driver can specify the driver's default locale by calling
    // WdfPdoInitSetDefaultLocale.
    //
    RtlInitUnicodeString(&StaticString, BT_PDO_DEVICE_LOCATION);
    Status = WdfPdoInitAddDeviceText(DeviceInit,
                                     &Buffer,
                                     &StaticString,
                                     0x409);
    if (!NT_SUCCESS(Status)) {        
        goto Cleanup;
    }

    WdfPdoInitSetDefaultLocale(DeviceInit, 0x409);

    //
    // Initialize the attributes to specify the size of PDO device extension.
    // All the state information private to the PDO will be tracked here.
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attributes, PDO_EXTENSION);

    //
    // Set power callbacks to handle idle/active transition of the Bluetooth function
    //
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&PnpPowerCallbacks);  

    //
    // Register PnP callback
    //
    PnpPowerCallbacks.EvtDevicePrepareHardware = PdoDevPrepareHardware;
    PnpPowerCallbacks.EvtDeviceReleaseHardware = PdoDevReleaseHardware; 

    //
    // Register for Power callback
    //
    PnpPowerCallbacks.EvtDeviceD0Entry = PdoDevD0Entry;
    PnpPowerCallbacks.EvtDeviceD0Exit  = PdoDevD0Exit; 
    
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, 
                                           &PnpPowerCallbacks); 

    //
    // Allow to forward requests to its FDO of this bus driver by using 
    // WdfRequestForwardToParentDeviceIoQueue()in the DeviceIoControl callback.
    //    
    WdfPdoInitAllowForwardingRequestToParent(DeviceInit);

    //
    // Register to handle bus level power management (arm for wake?)
    //
    WDF_PDO_EVENT_CALLBACKS_INIT(&Callbacks);
        
    //
    // Arm the device for wake:    
    //
    // When the device is powered down, the framework calls the bus driver's 
    // EvtDeviceEnableWakeAtBus callback function at the beginning of the shutdown 
    // sequence, while the child device is still in the D0 state. In this callback 
    // function, the bus driver must do whatever is required at the bus level to 
    // enable the wake signal.
    //    
    Callbacks.EvtDeviceEnableWakeAtBus  = PdoDevEnableWakeAtBus; 

    //
    // Disarm the device for wake:        
    //
    // If the child device triggered a wake signal, the system and the framework 
    // return the device to D0. The framework calls the bus driver's 
    // EvtDeviceDisableWakeAtBus callback function during startup of the child 
    // device. In this callback function, the bus driver should do whatever is 
    // required at the bus level to disable the wake signal, so that the device 
    // can no longer trigger it. Thus, EvtDeviceDisableWakeAtBus reverses the 
    // actions of EvtDeviceEnableWakeAtBus.
    //
    Callbacks.EvtDeviceDisableWakeAtBus = PdoDevDisableWakeAtBus;

    WdfPdoInitSetEventCallbacks(DeviceInit, &Callbacks);
    
    //
    // Create a framework device object to represent PDO of this bus driver. In response 
    // to this call, framework creates a WDM deviceobject.
    //
    Status = WdfDeviceCreate(&DeviceInit, 
                             &Attributes, 
                             &ChildDevice);    
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // Note: Once the device is created successfully, framework frees the
    // DeviceInit memory and sets the DeviceInit to NULL. So don't
    // call any WdfDeviceInit functions after that.
    //

    //
    // Initalize the PDO extension
    //
    PdoExtension = PdoGetExtension(ChildDevice);

    RtlZeroMemory(PdoExtension, sizeof(PDO_EXTENSION));

    PdoExtension->FdoExtension = FdoGetExtension(_Device);

    PdoExtension->SerialNo = _SerialNo;

    PdoExtension->ResetRecoveryType = SUPPORTED_RESET_RECOVERY_TYPE;

    //
    // Set PnP and Power capabilities for this child device.
    //    
    WDF_DEVICE_PNP_CAPABILITIES_INIT(&PnpCaps);  // Zeros this structure (note: WdfFalse is 0)

    //
    // Bus driver sets this value to WdfFalse for this embedded device, which cannot 
    // be physically removed; its FDO must not set/override this.
    //
    PnpCaps.Removable         = WdfFalse;      

    //
    // Bus driver sets this value to WdfTrue.  FDO can override this value (when this irp is on its 
    // way up) if it determines that this device cannot be safely surprise (not orderly) removed 
    // without data loss.
    //
    PnpCaps.SurpriseRemovalOK = WdfTrue;    

    PnpCaps.Address  = _SerialNo;
    PnpCaps.UINumber = _SerialNo;

    WdfDeviceSetPnpCapabilities(ChildDevice, &PnpCaps);

    WDF_DEVICE_POWER_CAPABILITIES_INIT(&PowerCaps);

    PowerCaps.DeviceD1 = WdfFalse;
    PowerCaps.DeviceD2 = WdfTrue;    

    PowerCaps.WakeFromD0 = WdfFalse;    
    PowerCaps.WakeFromD1 = WdfFalse;    
    PowerCaps.WakeFromD2 = WdfTrue;
    PowerCaps.WakeFromD3 = WdfTrue;    
    
    PowerCaps.DeviceState[PowerSystemWorking]   = PowerDeviceD0;
    PowerCaps.DeviceState[PowerSystemSleeping1] = PowerDeviceD2;
    PowerCaps.DeviceState[PowerSystemSleeping2] = PowerDeviceD2;
    PowerCaps.DeviceState[PowerSystemSleeping3] = PowerDeviceD2;
    PowerCaps.DeviceState[PowerSystemHibernate] = PowerDeviceD3;
    PowerCaps.DeviceState[PowerSystemShutdown]  = PowerDeviceD3;

    PowerCaps.DeviceWake = PowerDeviceD2;   // Lowest-powered Dx state to send wake signal to system

    WdfDeviceSetPowerCapabilities(ChildDevice, &PowerCaps);       

    //
    // Configure a default queue so that requests that are not
    // configure-fowarded using WdfDeviceConfigureRequestDispatching to goto
    // other queues get dispatched here.
    //

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&QueueConfig, WdfIoQueueDispatchParallel);

    //
    // Cannot be power managed queue (dispatch only at D0) as
    // BthMini issues BthX DDI to get version and capabilities 
    // before enter D0.  A deadlock occurs if this is power managed.
    //
    QueueConfig.PowerManaged = WdfFalse;
    
    QueueConfig.EvtIoDeviceControl = PdoIoQuDeviceControl;

    Status = WdfIoQueueCreate(ChildDevice, 
                              &QueueConfig, 
                              WDF_NO_OBJECT_ATTRIBUTES,
                              &Queue);
    DoTrace(LEVEL_INFO, TFLAG_PNP, (" WdfIoQueueCreate (%!STATUS!)", Status));    
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    if (PdoExtension->ResetRecoveryType == ResetRecoveryTypeParentResetInterface) {

        //
        // Instruct WDF to simply forward a request for the reset interface to the parent stack, so
        // that it reaches the ACPI bus/filter driver. That way when the reset interface is invoked,
        // it is the parent stack that gets reset and re-enumerated.
        //

        WDF_QUERY_INTERFACE_CONFIG_INIT(&DeviceResetInterfaceConfig, NULL, &GUID_DEVICE_RESET_INTERFACE_STANDARD, NULL);
        DeviceResetInterfaceConfig.SendQueryToParentStack = TRUE;

        Status = WdfDeviceAddQueryInterface(ChildDevice, &DeviceResetInterfaceConfig);
        if (!NT_SUCCESS(Status)) {
            DoTrace(LEVEL_ERROR, TFLAG_PNP, ("WdfDeviceAddQueryInterface failed, %!STATUS!", Status));
            goto Cleanup;
        }

    } else if (PdoExtension->ResetRecoveryType == ResetRecoveryTypeDriverImplemented) {

        RtlZeroMemory(&ResetInterface, sizeof(ResetInterface));
        ResetInterface.Size = sizeof(ResetInterface);
        ResetInterface.Version = DEVICE_RESET_INTERFACE_VERSION;
        ResetInterface.Context = PdoExtension;

        //
        // Since this interface is expected to be used only by drivers in the same stack, reference
        // counting is not required. If there is an expectation that drivers may query for the
        // interface using a remote I/O target to this stack (unusual for this interface), reference
        // counting must be implemented.
        //
        ResetInterface.InterfaceReference = WdfDeviceInterfaceReferenceNoOp;
        ResetInterface.InterfaceDereference = WdfDeviceInterfaceDereferenceNoOp;

        ResetInterface.SupportedResetTypes = (1 << PlatformLevelDeviceReset);
        ResetInterface.DeviceReset = PdoResetHandler;

        WDF_QUERY_INTERFACE_CONFIG_INIT(&DeviceResetInterfaceConfig, (PINTERFACE) &ResetInterface, &GUID_DEVICE_RESET_INTERFACE_STANDARD, NULL);
        Status = WdfDeviceAddQueryInterface(ChildDevice, &DeviceResetInterfaceConfig);
        if (!NT_SUCCESS(Status))
        {
            DoTrace(LEVEL_ERROR, TFLAG_PNP, ("WdfDeviceAddQueryInterface failed, %!STATUS!", Status));
            goto Cleanup;
        }
    }

    //
    // Add this device to the FDO's collection of children.
    // After the child device is added to the static collection successfully,
    // driver must call WdfPdoMarkMissing to get the device deleted. It
    // shouldn't delete the child device directly by calling WdfObjectDelete.
    //
    Status = WdfFdoAddStaticChild(_Device, ChildDevice);
    DoTrace(LEVEL_INFO, TFLAG_PNP, (" WdfFdoAddStaticChild (%!STATUS!)", Status));
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

Cleanup:
    
    //
    // Call WdfDeviceInitFree if you encounter an error before the
    // device is created. Once the device is created, framework
    // NULLs the DeviceInit value.
    //
    if (!NT_SUCCESS(Status)) {
        
        DoTrace(LEVEL_ERROR, TFLAG_PNP, (" -PdoCreate: exit %!STATUS!", Status));
        
        if (DeviceInit != NULL) {
            WdfDeviceInitFree(DeviceInit);
        }

        if(ChildDevice) {
            WdfObjectDelete(ChildDevice);
        }
    }
    else
    {
        DoTrace(LEVEL_INFO, TFLAG_PNP, (" -PdoCreate: exit %!STATUS!", Status));          
    }

    if (NULL != ContainerID.Buffer) {
        RtlFreeUnicodeString(&ContainerID);
    }

    return Status;
}

NTSTATUS
PdoDevPrepareHardware(
    _In_  WDFDEVICE     _Device,
    _In_  WDFCMRESLIST  _ResourcesRaw,
    _In_  WDFCMRESLIST  _ResourcesTranslated     
    )
/*++
Routine Description:

    This PnP CB function take a refernce of its parent so it will not enter DxState while in S0Idle.

Arguments:

    _Device - WDF Device object

    _ResourcesRaw - (Not referenced)
    
    _ResourcesTranslated - (Not referenced)

Return Value:

    NTSTATUS

--*/     
{
    NTSTATUS Status = STATUS_SUCCESS;
    WDFDEVICE ParentDevice;    
    
    PAGED_CODE();

    UNREFERENCED_PARAMETER(_ResourcesRaw);   
    UNREFERENCED_PARAMETER(_ResourcesTranslated);     

    DoTrace(LEVEL_INFO, TFLAG_PNP,("+PdoDevPrepareHardware"));  

    ParentDevice = WdfPdoGetParent(_Device);

    //
    // Take a reference to avoid FDO to enter DxState in IdleS0
    //
    Status = WdfDeviceStopIdle(ParentDevice, FALSE);
    DoTrace(LEVEL_INFO, TFLAG_PNP, ("WdfDeviceStopIdle %!STATUS!", Status));  
    
    //
    // Any failure Status code should be invesigated to ensure that the reference count is balanced. 
    //
    NT_ASSERT(NT_SUCCESS(Status));

    DoTrace(LEVEL_INFO, TFLAG_PNP, ("-PdoDevPrepareHardware"));   

    return Status;
}


NTSTATUS
PdoDevReleaseHardware(
    _In_  WDFDEVICE     _Device,
    _In_  WDFCMRESLIST  _ResourcesTranslated
    )
/*++
Routine Description:

    This PnP CB function release a refcount of its parent so it can enter DxState in S0Idle.

Arguments:

    _Device - WDF Device object
    
    _ResourcesTranslated - (Not referenced)

Return Value:

    NTSTATUS

--*/    
{
    WDFDEVICE ParentDevice;       
    
    PAGED_CODE();
   
    UNREFERENCED_PARAMETER(_ResourcesTranslated);   

    DoTrace(LEVEL_INFO, TFLAG_PNP,("+PdoDevReleaseHardware")); 

    ParentDevice = WdfPdoGetParent(_Device);

    //
    // Release a reference to allow FDO to enter DxState in IdleS0
    //
    WdfDeviceResumeIdle(ParentDevice);    
        
    return STATUS_SUCCESS;
}



NTSTATUS
PdoDevD0Entry(
    _In_   WDFDEVICE              _Device,
    _In_   WDF_POWER_DEVICE_STATE _PreviousState
    )
/*++
Routine Description:

    This PnPPower CB function is invoked after device has entered D0 (working) state. 

Arguments:

    _Device - WDF Device object
    
    PreviousState - Previous device power state

Return Value:

    NTSTATUS

--*/    
{
    NTSTATUS       Status = STATUS_SUCCESS;    

    PAGED_CODE();   

    UNREFERENCED_PARAMETER(_Device);
    UNREFERENCED_PARAMETER(_PreviousState);    
    
    DoTrace(LEVEL_INFO, TFLAG_UART, ("+PdoDevD0Entry")); 

    // 
    // Can bring the Bluetooth function back to active state
    //


    DoTrace(LEVEL_INFO, TFLAG_UART, ("-PdoDevD0Entry %!STATUS!", Status));
    
    return Status;
}


NTSTATUS
PdoDevD0Exit(
    _In_   WDFDEVICE              _Device,
    _In_   WDF_POWER_DEVICE_STATE _TargetState
    )
/*++
Routine Description:

    This PnP CB function is invoked when device has exited D0 (working) state.

Arguments:

    _Device - WDF Device object
    
    _TargetState - Next device power state that it is about to enter

Return Value:

    NTSTATUS

--*/    
{
    PAGED_CODE();  

    UNREFERENCED_PARAMETER(_Device);
    UNREFERENCED_PARAMETER(_TargetState);
    
    DoTrace(LEVEL_INFO, TFLAG_UART, ("+PdoDevD0Exit: D0 -> D%d", _TargetState-WdfPowerDeviceD0));

    //
    // Can prepare the Bluetooth function to enter lower power device state
    //

   
    DoTrace(LEVEL_INFO, TFLAG_UART, ("-PdoDevD0Exit"));

    return STATUS_SUCCESS;
}


VOID
PdoDevDisableWakeAtBus(
    _In_  WDFDEVICE _Device
    )
/*++

Routine Description:

    This framework callback routine performs bus-level operations that disable 
    the ability of one of the bus's devices to trigger a wake-up signal.
    
Arguments:

    _Device - Framework device object

Return Value:

   VOID

--*/    
{   
    //
    // Do not mark this function pageable to potentially reduce power up time.
    //
    
    DoTrace(LEVEL_INFO, TFLAG_POWER,("<==(D)== PdoDevDisableWakeAtBus"));  

    //
    // Device specific implementation to disarm for wake
    //
    DeviceDisableWakeControl(_Device);    
}

NTSTATUS
PdoDevEnableWakeAtBus(
    _In_  WDFDEVICE          _Device,
    _In_  SYSTEM_POWER_STATE _PowerState
    ) 
/*++

Routine Description:

    This framework callback routine performs bus-level operations that enable 
    one of the bus's devices to trigger a wake-up signal.
    
Arguments:

    _Device - Framework device object

    _PowerState - identifies the system power state that the system or device will wake from. 

Return Value:

   NTSTATUS

--*/
{
    //
    // Do not mark this function pageable to potentially reduce power up time.
    //
    
    DoTrace(LEVEL_INFO, TFLAG_POWER,("==(E)==> PdoDevEnableWakeAtBus from %S", 
            _PowerState == PowerSystemWorking ? L"S0" : L"Sx"));

    //
    // Device specific implementation to arm for wake
    //
    return DeviceEnableWakeControl(_Device, _PowerState);  
}

VOID
PdoIoQuDeviceControl(
    _In_  WDFQUEUE     _Queue,
    _In_  WDFREQUEST   _Request,
    _In_  size_t       _OutputBufferLength,
    _In_  size_t       _InputBufferLength,
    _In_  ULONG        _IoControlCode
    )
/*++

Routine Description:

    This routine is the dispatch routine for device control requests.  This routine can be invoke
    at the DISPATCH level from BthPort/mini.
    
Arguments:

    _Queue - Handle to the framework queue object that is associated
            with the I/O request.
    _Request - Handle to a framework request object.

    _OutputBufferLength - length of the request's output buffer,
                        if an output buffer is available.
    _InputBufferLength - length of the request's input buffer,
                        if an input buffer is available.

    _IoControlCode - the driver-defined or system-defined I/O control code
                    (IOCTL) that is associated with the request.

Return Value:

   VOID

--*/
{
    WDFDEVICE Device = NULL;
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    WDF_REQUEST_FORWARD_OPTIONS ForwardOptions;
    WDFDEVICE ParentDevice;
    ULONG ControlCode = (_IoControlCode & 0x00003ffc) >> 2;    

    UNREFERENCED_PARAMETER(_OutputBufferLength);
    UNREFERENCED_PARAMETER(_InputBufferLength);

    DoTrace(LEVEL_INFO, TFLAG_IOCTL,("+IoDeviceControl - InBufLen:%d, OutBufLen:%d",
            (ULONG) _InputBufferLength, (ULONG) _OutputBufferLength));
    
    switch (_IoControlCode) {  
    case IOCTL_BTHX_GET_VERSION:
    case IOCTL_BTHX_SET_VERSION: 
    case IOCTL_BTHX_QUERY_CAPABILITIES:        
    case IOCTL_BTHX_WRITE_HCI:  
    case IOCTL_BTHX_READ_HCI:           
        Device = WdfIoQueueGetDevice(_Queue);
        WDF_REQUEST_FORWARD_OPTIONS_INIT(&ForwardOptions);
        ForwardOptions.Flags = WDF_REQUEST_FORWARD_OPTION_SEND_AND_FORGET;        
        ParentDevice = WdfPdoGetParent(Device);

        //
        // Forward known IOCTLs to FDO to process
        //
        Status = WdfRequestForwardToParentDeviceIoQueue(_Request,
                                                        WdfDeviceGetDefaultQueue(ParentDevice),
                                                        &ForwardOptions);
        break;  
      
    default:
        //
        // Complete this unexptected IOCTL with default STATUS_INVALID_PARAMETER.
        //        
        DoTrace(LEVEL_ERROR, TFLAG_IOCTL,("Unexpected IOCTL_(0x%x, Func %d)", _IoControlCode, ControlCode));
        break;         
    }
    
    if (!NT_SUCCESS(Status)){
        DoTrace(LEVEL_ERROR, TFLAG_IOCTL,(" IOCTL_(0x%x, Func %d) failed %!STATUS!", _IoControlCode, ControlCode, Status));
        WdfRequestComplete(_Request, Status);
        return;
    }        

    return;
}


