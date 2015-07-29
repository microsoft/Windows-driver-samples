/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    wdf.c

Abstract:
    This module implements WDF and WDM functionality required to register as a
    device driver, instantiate devices, and register those devices with the
    battery class driver.

    N.B. This code is provided "AS IS" without any expressed or implied warranty.

--*/

//--------------------------------------------------------------------- Includes

#include "simbatt.h"
#include "simbattdriverif.h"
#include "batclass_prepublish.h"

//------------------------------------------------------------------- Prototypes

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD SimBattDriverDeviceAdd;
EVT_WDF_DEVICE_SELF_MANAGED_IO_INIT  SimBattSelfManagedIoInit;
EVT_WDF_DEVICE_SELF_MANAGED_IO_CLEANUP  SimBattSelfManagedIoCleanup;
EVT_WDF_DEVICE_QUERY_STOP SimBattQueryStop;
EVT_WDF_DEVICE_PREPARE_HARDWARE SimBattDevicePrepareHardware;
EVT_WDFDEVICE_WDM_IRP_PREPROCESS SimBattWdmIrpPreprocessDeviceControl;
EVT_WDFDEVICE_WDM_IRP_PREPROCESS SimBattWdmIrpPreprocessSystemControl;
WMI_QUERY_REGINFO_CALLBACK SimBattQueryWmiRegInfo;
WMI_QUERY_DATABLOCK_CALLBACK SimBattQueryWmiDataBlock;

//---------------------------------------------------------------------- Pragmas

#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, SimBattSelfManagedIoInit)
#pragma alloc_text(PAGE, SimBattSelfManagedIoCleanup)
#pragma alloc_text(PAGE, SimBattQueryStop)
#pragma alloc_text(PAGE, SimBattDriverDeviceAdd)
#pragma alloc_text(PAGE, SimBattDevicePrepareHardware)
#pragma alloc_text(PAGE, SimBattWdmIrpPreprocessDeviceControl)
#pragma alloc_text(PAGE, SimBattWdmIrpPreprocessSystemControl)
#pragma alloc_text(PAGE, SimBattQueryWmiRegInfo)
#pragma alloc_text(PAGE, SimBattQueryWmiDataBlock)

//-------------------------------------------------------------------- Functions

_Use_decl_annotations_
NTSTATUS
DriverEntry (
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    DriverEntry initializes the driver and is the first routine called by the
    system after the driver is loaded. DriverEntry configures and creates a WDF
    driver object.

Parameters Description:

    DriverObject - Supplies a pointer to the driver object.

    RegistryPath - Supplies a pointer to a unicode string representing the path
        to the driver-specific key in the registry.

Return Value:

    NTSTATUS.

--*/

{

    WDF_OBJECT_ATTRIBUTES DriverAttributes;
    WDF_DRIVER_CONFIG DriverConfig;
    PSIMBATT_GLOBAL_DATA GlobalData;
    NTSTATUS Status;

    DebugEnter();

    WDF_DRIVER_CONFIG_INIT(&DriverConfig, SimBattDriverDeviceAdd);

    //
    // Initialize attributes and a context area for the driver object.
    //
    // N.B. ExecutionLevel is set to Passive because this driver expect callback
    //      functions to be called at PASSIVE_LEVEL.
    //
    // N.B. SynchronizationScope is not specified and therefore it is set to
    //      None. This means that the WDF framework does not synchronize the
    //      callbacks, you may want to set this to a different value based on
    //      how the callbacks are required to be synchronized in your driver.
    //

    WDF_OBJECT_ATTRIBUTES_INIT(&DriverAttributes);
    WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(&DriverAttributes,
                                           SIMBATT_GLOBAL_DATA);

    DriverAttributes.ExecutionLevel = WdfExecutionLevelPassive;

    //
    // Create the driver object
    //

    Status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             &DriverAttributes,
                             &DriverConfig,
                             WDF_NO_HANDLE);

    if (!NT_SUCCESS(Status)) {
        DebugPrint(SIMBATT_ERROR,
                   "WdfDriverCreate() Failed. Status 0x%x\n",
                   Status);

        goto DriverEntryEnd;
    }

    GlobalData = GetGlobalData(WdfGetDriver());
    GlobalData->RegistryPath.MaximumLength = RegistryPath->Length +
                                             sizeof(UNICODE_NULL);

    GlobalData->RegistryPath.Length = RegistryPath->Length;
    GlobalData->RegistryPath.Buffer = WdfDriverGetRegistryPath(WdfGetDriver());

DriverEntryEnd:
    DebugExitStatus(Status);
    return Status;
}

_Use_decl_annotations_
NTSTATUS
SimBattDriverDeviceAdd (
    WDFDRIVER Driver,
    PWDFDEVICE_INIT DeviceInit
    )

/*++
Routine Description:

    EvtDriverDeviceAdd is called by the framework in response to AddDevice
    call from the PnP manager. A WDF device object is created and initialized to
    represent a new instance of the battery device.

Arguments:

    Driver - Supplies a handle to the WDF Driver object.

    DeviceInit - Supplies a pointer to a framework-allocated WDFDEVICE_INIT
        structure.

Return Value:

    NTSTATUS

--*/

{

    WDF_OBJECT_ATTRIBUTES DeviceAttributes;
    PSIMBATT_FDO_DATA DevExt;
    WDFDEVICE DeviceHandle;
    WDF_FILEOBJECT_CONFIG FileObjectConfig;
    WDF_OBJECT_ATTRIBUTES LockAttributes;
    WDF_PNPPOWER_EVENT_CALLBACKS PnpPowerCallbacks;
    WDFQUEUE Queue;
    WDF_IO_QUEUE_CONFIG QueueConfig;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(Driver);

    DebugEnter();
    PAGED_CODE();

    //
    // Initialize the PnpPowerCallbacks structure.  Callback events for PNP
    // and Power are specified here.
    //

    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&PnpPowerCallbacks);
    PnpPowerCallbacks.EvtDevicePrepareHardware = SimBattDevicePrepareHardware;
    PnpPowerCallbacks.EvtDeviceSelfManagedIoInit = SimBattSelfManagedIoInit;
    PnpPowerCallbacks.EvtDeviceSelfManagedIoCleanup = SimBattSelfManagedIoCleanup;
    PnpPowerCallbacks.EvtDeviceQueryStop = SimBattQueryStop;
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &PnpPowerCallbacks);

    //
    // Configure file handlers to forward all create, close, and cleanup
    // requests to the PDO.
    //

    WDF_FILEOBJECT_CONFIG_INIT(&FileObjectConfig,
                               WDF_NO_EVENT_CALLBACK,
                               WDF_NO_EVENT_CALLBACK,
                               WDF_NO_EVENT_CALLBACK);

    FileObjectConfig.AutoForwardCleanupClose = WdfTrue;
    WdfDeviceInitSetFileObjectConfig(DeviceInit,
                                     &FileObjectConfig,
                                     WDF_NO_OBJECT_ATTRIBUTES);

    //
    // Register WDM preprocess callbacks for IRP_MJ_DEVICE_CONTROL and
    // IRP_MJ_SYSTEM_CONTROL. The battery class driver needs to handle these IO
    // requests directly.
    //

    Status = WdfDeviceInitAssignWdmIrpPreprocessCallback(
                 DeviceInit,
                 SimBattWdmIrpPreprocessDeviceControl,
                 IRP_MJ_DEVICE_CONTROL,
                 NULL,
                 0);

    if (!NT_SUCCESS(Status)) {
         DebugPrint(SIMBATT_ERROR,
                    "WdfDeviceInitAssignWdmIrpPreprocessCallback"
                    "(IRP_MJ_DEVICE_CONTROL) Failed. 0x%x\n",
                    Status);

         goto DriverDeviceAddEnd;
    }

    Status = WdfDeviceInitAssignWdmIrpPreprocessCallback(
                 DeviceInit,
                 SimBattWdmIrpPreprocessSystemControl,
                 IRP_MJ_SYSTEM_CONTROL,
                 NULL,
                 0);

    if (!NT_SUCCESS(Status)) {
         DebugPrint(SIMBATT_ERROR,
                    "WdfDeviceInitAssignWdmIrpPreprocessCallback"
                    "(IRP_MJ_SYSTEM_CONTROL) Failed. 0x%x\n",
                    Status);

         goto DriverDeviceAddEnd;
    }

    //
    // Initialize attributes and a context area for the device object.
    //

    WDF_OBJECT_ATTRIBUTES_INIT(&DeviceAttributes);
    WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(&DeviceAttributes, SIMBATT_FDO_DATA);

    //
    // Create a framework device object.  This call will in turn create
    // a WDM device object, attach to the lower stack, and set the
    // appropriate flags and attributes.
    //

    Status = WdfDeviceCreate(&DeviceInit, &DeviceAttributes, &DeviceHandle);
    if (!NT_SUCCESS(Status)) {
        DebugPrint(SIMBATT_ERROR, "WdfDeviceCreate() Failed. 0x%x\n", Status);
        goto DriverDeviceAddEnd;
    }

    //
    // Configure a default queue for IO requests that are not handled by the
    // class driver. For the simulated battery, this queue processes requests
    // to set the simulated status.
    //

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&QueueConfig,
                                           WdfIoQueueDispatchSequential);

    QueueConfig.EvtIoDeviceControl = SimBattIoDeviceControl;
    Status = WdfIoQueueCreate(DeviceHandle,
                              &QueueConfig,
                              WDF_NO_OBJECT_ATTRIBUTES,
                              &Queue);

    if (!NT_SUCCESS(Status)) {
        DebugPrint(SIMBATT_ERROR, "WdfIoQueueCreate() Failed. 0x%x\n", Status);
        goto DriverDeviceAddEnd;
    }

    //
    // Create a device interface for this device to advertise the simulated
    // battery IO interface.
    //

    Status = WdfDeviceCreateDeviceInterface(DeviceHandle,
                                            &SIMBATT_DEVINTERFACE_GUID,
                                            NULL);

    if (!NT_SUCCESS(Status)) {
        goto DriverDeviceAddEnd;
    }

    //
    // Finish initializing the device context area.
    //

    DevExt = GetDeviceExtension(DeviceHandle);
    DevExt->BatteryTag = BATTERY_TAG_INVALID;
    DevExt->ClassHandle = NULL;
    WDF_OBJECT_ATTRIBUTES_INIT(&LockAttributes);
    LockAttributes.ParentObject = DeviceHandle;
    Status = WdfWaitLockCreate(&LockAttributes,
                               &DevExt->ClassInitLock);

    if (!NT_SUCCESS(Status)) {
        DebugPrint(SIMBATT_ERROR,
                   "WdfWaitLockCreate(ClassInitLock) Failed. Status 0x%x\n",
                   Status);

        goto DriverDeviceAddEnd;
    }

    WDF_OBJECT_ATTRIBUTES_INIT(&LockAttributes);
    LockAttributes.ParentObject = DeviceHandle;
    Status = WdfWaitLockCreate(&LockAttributes,
                               &DevExt->StateLock);

    if (!NT_SUCCESS(Status)) {
        DebugPrint(SIMBATT_ERROR,
                   "WdfWaitLockCreate(StateLock) Failed. Status 0x%x\n",
                   Status);

        goto DriverDeviceAddEnd;
    }

DriverDeviceAddEnd:
    DebugExitStatus(Status);
    return Status;
}

_Use_decl_annotations_
NTSTATUS
SimBattSelfManagedIoInit (
    WDFDEVICE Device
    )

/*++

Routine Description:

    The framework calls this function once per device after EvtDeviceD0Entry
    callback has been called for the first time. This function is not called
    again unless device is remove and reconnected or the drivers are reloaded.

Arguments:

    Device - Supplies a handle to a framework device object.

Return Value:

    NTSTATUS

--*/

{

    BATTERY_MINIPORT_INFO_V1_1 BattInit;
    PSIMBATT_FDO_DATA DevExt;
    PDEVICE_OBJECT DeviceObject;
    NTSTATUS Status;

    DebugEnter();
    PAGED_CODE();

    DevExt = GetDeviceExtension(Device);

    //
    // Attach to the battery class driver.
    //

    RtlZeroMemory(&BattInit, sizeof(BattInit));
    BattInit.MajorVersion = BATTERY_CLASS_MAJOR_VERSION;
    BattInit.MinorVersion = BATTERY_CLASS_MINOR_VERSION_1;
    BattInit.Context = DevExt;
    BattInit.QueryTag = SimBattQueryTag;
    BattInit.QueryInformation = SimBattQueryInformation;
    BattInit.SetInformation = SimBattSetInformation;
    BattInit.QueryStatus = SimBattQueryStatus;
    BattInit.SetStatusNotify = SimBattSetStatusNotify;
    BattInit.DisableStatusNotify = SimBattDisableStatusNotify;
    BattInit.Pdo = WdfDeviceWdmGetPhysicalDevice(Device);
    BattInit.DeviceName = NULL;
    BattInit.Fdo = WdfDeviceWdmGetDeviceObject(Device);
    WdfWaitLockAcquire(DevExt->ClassInitLock, NULL);
    Status = BatteryClassInitializeDevice((PBATTERY_MINIPORT_INFO)&BattInit,
                                          &DevExt->ClassHandle);

    WdfWaitLockRelease(DevExt->ClassInitLock);
    if (!NT_SUCCESS(Status)) {
        goto DevicePrepareHardwareEnd;
    }

    //
    // Register the device as a WMI data provider. This is done using WDM
    // methods because the battery class driver uses WDM methods to complete
    // WMI requests.
    //

    DevExt->WmiLibContext.GuidCount = 0;
    DevExt->WmiLibContext.GuidList = NULL;
    DevExt->WmiLibContext.QueryWmiRegInfo = SimBattQueryWmiRegInfo;
    DevExt->WmiLibContext.QueryWmiDataBlock = SimBattQueryWmiDataBlock;
    DevExt->WmiLibContext.SetWmiDataBlock = NULL;
    DevExt->WmiLibContext.SetWmiDataItem = NULL;
    DevExt->WmiLibContext.ExecuteWmiMethod = NULL;
    DevExt->WmiLibContext.WmiFunctionControl = NULL;
    DeviceObject = WdfDeviceWdmGetDeviceObject(Device);
    Status = IoWMIRegistrationControl(DeviceObject, WMIREG_ACTION_REGISTER);

    //
    // Failure to register with WMI is nonfatal.
    //

    if (!NT_SUCCESS(Status)) {
        DebugPrint(SIMBATT_WARN,
                   "IoWMIRegistrationControl() Failed. Status 0x%x\n",
                   Status);

        Status = STATUS_SUCCESS;
    }

DevicePrepareHardwareEnd:
    DebugExitStatus(Status);
    return Status;
}

_Use_decl_annotations_
VOID
SimBattSelfManagedIoCleanup (
    WDFDEVICE Device
    )

/*++

Routine Description:

    This function is called after EvtDeviceSelfManagedIoSuspend callback. This
    function must release any sel-managed I/O operation data.

Arguments:

    Device - Supplies a handle to a framework device object.

Return Value:

    NTSTATUS - Failures will be logged, but not acted on.

--*/

{

    PSIMBATT_FDO_DATA DevExt;
    PDEVICE_OBJECT DeviceObject;
    NTSTATUS Status;

    DebugEnter();
    PAGED_CODE();

    DeviceObject = WdfDeviceWdmGetDeviceObject(Device);
    Status = IoWMIRegistrationControl(DeviceObject, WMIREG_ACTION_DEREGISTER);
    if (!NT_SUCCESS(Status)) {
        DebugPrint(SIMBATT_WARN,
                   "IoWMIRegistrationControl() Failed. Status 0x%x\n",
                   Status);

        Status = STATUS_SUCCESS;
    }

    DevExt = GetDeviceExtension(Device);
    WdfWaitLockAcquire(DevExt->ClassInitLock, NULL);
    if (DevExt->ClassHandle != NULL) {
        Status = BatteryClassUnload(DevExt->ClassHandle);
        DevExt->ClassHandle = NULL;
    }

    WdfWaitLockRelease(DevExt->ClassInitLock);
    DebugExitStatus(Status);
    return;
}

_Use_decl_annotations_
NTSTATUS
SimBattQueryStop (
    _In_ WDFDEVICE Device
    )

/*++

Routine Description:

    EvtDeviceQueryStop event callback function determines whether a specified 
    device can be stopped so that the PnP manager can redistribute system 
    hardware resources.

    SimBatt is designed to fail a rebalance operation, for reasons described 
    below. Note however that this approach must *not* be adopted by an actual
    battery driver. 

    SimBatt unregisters itself as a Battery driver by calling 
    BatteryClassUnload() when IRP_MN_STOP_DEVICE arrives at the driver. It
    re-establishes itself as a Battery driver on arrival of IRP_MN_START_DEVICE.
    This results in any IOCTLs normally handeled by the Battery Class driver to 
    be delivered to SimBatt. The IO Queue employed by SimBatt is power managed,
    it causes these IOCTLs to be pended when SimBatt is not in D0. Now if the 
    device attempts to do a shutdown while an IOCTL is pended in SimBatt, it
    would result in a 0x9F bugcheck. By opting out of PNP rebalance operation 
    SimBatt circumvents this issue.

Arguments:

    Device - Supplies a handle to a framework device object.

Return Value:

    NTSTATUS

--*/

{

    UNREFERENCED_PARAMETER(Device);

    return STATUS_UNSUCCESSFUL;
}

_Use_decl_annotations_
NTSTATUS
SimBattDevicePrepareHardware (
    WDFDEVICE Device,
    WDFCMRESLIST ResourcesRaw,
    WDFCMRESLIST ResourcesTranslated
    )

/*++

Routine Description:

    EvtDevicePrepareHardware event callback performs operations that are
    necessary to make the driver's device operational. The framework calls the
    driver's EvtDevicePrepareHardware callback when the PnP manager sends an
    IRP_MN_START_DEVICE request to the driver stack.

Arguments:

    Device - Supplies a handle to a framework device object.

    ResourcesRaw - Supplies a handle to a collection of framework resource
        objects. This collection identifies the raw (bus-relative) hardware
        resources that have been assigned to the device.

    ResourcesTranslated - Supplies a handle to a collection of framework
        resource objects. This collection identifies the translated
        (system-physical) hardware resources that have been assigned to the
        device. The resources appear from the CPU's point of view. Use this list
        of resources to map I/O space and device-accessible memory into virtual
        address space

Return Value:

    NTSTATUS

--*/

{

    NTSTATUS Status;

    UNREFERENCED_PARAMETER(ResourcesRaw);
    UNREFERENCED_PARAMETER(ResourcesTranslated);

    DebugEnter();
    PAGED_CODE();

    SimBattPrepareHardware(Device);
    Status = STATUS_SUCCESS;
    DebugExitStatus(Status);
    return Status;
}

_Use_decl_annotations_
NTSTATUS
SimBattWdmIrpPreprocessDeviceControl (
    WDFDEVICE Device,
    PIRP Irp
    )

/*++

Routine Description:

    This event is called when the framework receives IRP_MJ_DEVICE_CONTROL
    requests from the system.

    N.B. Battery stack requires the device IOCTLs be sent to it at
         PASSIVE_LEVEL only, any IOCTL comming from user mode is therefore
         fine, kernel components, such as filter drivers sitting on top of
         the battery drivers should be careful to not voilate this
         requirement.

Arguments:

    Device - Supplies a handle to a framework device object.

    Irp - Supplies the IO request being processed.

Return Value:

    NTSTATUS

--*/

{

    PSIMBATT_FDO_DATA DevExt;
    NTSTATUS Status;

    PAGED_CODE();
    DebugEnter();

    ASSERTMSG("Must be called at IRQL = PASSIVE_LEVEL",
              (KeGetCurrentIrql() == PASSIVE_LEVEL));

    DevExt = GetDeviceExtension(Device);
    Status = STATUS_NOT_SUPPORTED;

    //
    // Suppress 28118:Irq Exceeds Caller, see Routine Description for
    // explaination.
    //

    #pragma warning(suppress: 28118)
    WdfWaitLockAcquire(DevExt->ClassInitLock, NULL);

    //
    // N.B. An attempt to queue the IRP with the port driver should happen
    //      before WDF assumes ownership of this IRP, i.e. before
    //      WdfDeviceWdmDispatchPreprocessedIrp is called, this is so that the
    //      Battery port driver, which is a WDM driver, may complete the IRP if
    //      it does endup procesing it.
    //

    if (DevExt->ClassHandle != NULL) {

        //
        // Suppress 28118:Irq Exceeds Caller, see above N.B.
        //

        #pragma warning(suppress: 28118)
        Status = BatteryClassIoctl(DevExt->ClassHandle, Irp);
    }

    WdfWaitLockRelease(DevExt->ClassInitLock);
    if (Status == STATUS_NOT_SUPPORTED) {
        IoSkipCurrentIrpStackLocation(Irp);
        Status = WdfDeviceWdmDispatchPreprocessedIrp(Device, Irp);
    }

    DebugExitStatus(Status);
    return Status;
}

_Use_decl_annotations_
NTSTATUS
SimBattWdmIrpPreprocessSystemControl (
    WDFDEVICE Device,
    PIRP Irp
    )

/*++

Routine Description:

    This event is called when the framework receives IRP_MJ_SYSTEM_CONTROL
    requests from the system.

    N.B. Battery stack requires the device IOCTLs be sent to it at
         PASSIVE_LEVEL only, any IOCTL comming from user mode is therefore
         fine, kernel components, such as filter drivers sitting on top of
         the battery drivers should be careful to not voilate this
         requirement.

Arguments:

    Device - Supplies a handle to a framework device object.

    Irp - Supplies the IO request being processed.

Return Value:

    NTSTATUS

--*/

{

    PSIMBATT_FDO_DATA DevExt;
    PDEVICE_OBJECT DeviceObject;
    SYSCTL_IRP_DISPOSITION Disposition;
    NTSTATUS Status;

    DebugEnter();
    PAGED_CODE();

    ASSERTMSG("Must be called at IRQL = PASSIVE_LEVEL",(KeGetCurrentIrql() == PASSIVE_LEVEL));

    Status = STATUS_NOT_IMPLEMENTED;
    DevExt = GetDeviceExtension(Device);
    Disposition = IrpForward;

    //
    // Acquire the class initialization lock and attempt to queue the IRP with
    // the class driver.
    //
    // Suppress 28118:Irq Exceeds Caller, see Routine Description for
    // explaination.
    //

    #pragma warning(suppress: 28118)
    WdfWaitLockAcquire(DevExt->ClassInitLock, NULL);
    if (DevExt->ClassHandle != NULL) {
        DeviceObject = WdfDeviceWdmGetDeviceObject(Device);
        Status = BatteryClassSystemControl(DevExt->ClassHandle,
                                           &DevExt->WmiLibContext,
                                           DeviceObject,
                                           Irp,
                                           &Disposition);
    }

    WdfWaitLockRelease(DevExt->ClassInitLock);
    switch (Disposition) {
    case IrpProcessed:
        break;

    case IrpNotCompleted:
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        break;

    case IrpForward:
    case IrpNotWmi:
    default:
        IoSkipCurrentIrpStackLocation(Irp);
        Status = WdfDeviceWdmDispatchPreprocessedIrp(Device, Irp);
        break;
    }

    DebugExitStatus(Status);
    return Status;
}

_Use_decl_annotations_
NTSTATUS
SimBattQueryWmiRegInfo (
    PDEVICE_OBJECT DeviceObject,
    PULONG RegFlags,
    PUNICODE_STRING InstanceName,
    PUNICODE_STRING *RegistryPath,
    PUNICODE_STRING MofResourceName,
    PDEVICE_OBJECT *Pdo
    )

/*++

Routine Description:

    This routine is a callback into the driver to retrieve the list of
    guids or data blocks that the driver wants to register with WMI. This
    routine may not pend or block. Driver should NOT call
    WmiCompleteRequest.

Arguments:

    DeviceObject - Supplies the device whose data block is being queried.

    RegFlags - Supplies a pointer to return a set of flags that describe the
        guids being registered for this device. If the device wants enable and
        disable collection callbacks before receiving queries for the registered
        guids then it should return the WMIREG_FLAG_EXPENSIVE flag. Also the
        returned flags may specify WMIREG_FLAG_INSTANCE_PDO in which case
        the instance name is determined from the PDO associated with the
        device object. Note that the PDO must have an associated devnode. If
        WMIREG_FLAG_INSTANCE_PDO is not set then Name must return a unique
        name for the device.

    InstanceName - Supplies a pointer to return the instance name for the guids
        if WMIREG_FLAG_INSTANCE_PDO is not set in the returned *RegFlags. The
        caller will call ExFreePool with the buffer returned.

    RegistryPath - Supplies a pointer to return the registry path of the driver.

    MofResourceName - Supplies a pointer to return the name of the MOF resource
        attached to the binary file. If the driver does not have a mof resource
        attached then this can be returned as NULL.

    Pdo - Supplies a pointer to return the device object for the PDO associated
        with this device if the WMIREG_FLAG_INSTANCE_PDO flag is returned in
        *RegFlags.

Return Value:

    NTSTATUS

--*/

{

    WDFDEVICE Device;
    PSIMBATT_GLOBAL_DATA GlobalData;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(MofResourceName);
    UNREFERENCED_PARAMETER(InstanceName);

    DebugEnter();
    PAGED_CODE();

    Device = WdfWdmDeviceGetWdfDeviceHandle(DeviceObject);
    GlobalData = GetGlobalData(WdfGetDriver());
    *RegFlags = WMIREG_FLAG_INSTANCE_PDO;
    *RegistryPath = &GlobalData->RegistryPath;
    *Pdo = WdfDeviceWdmGetPhysicalDevice(Device);
    Status = STATUS_SUCCESS;
    DebugExitStatus(Status);
    return Status;
}

_Use_decl_annotations_
NTSTATUS
SimBattQueryWmiDataBlock (
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    ULONG GuidIndex,
    ULONG InstanceIndex,
    ULONG InstanceCount,
    PULONG InstanceLengthArray,
    ULONG BufferAvail,
    PUCHAR Buffer
    )

/*++

Routine Description:

    This routine is a callback into the driver to query for the contents of
    a data block. When the driver has finished filling the data block it
    must call WmiCompleteRequest to complete the irp. The driver can
    return STATUS_PENDING if the irp cannot be completed immediately.

Arguments:

    DeviceObject - Supplies the device whose data block is being queried.

    Irp - Supplies the Irp that makes this request.

    GuidIndex - Supplies the index into the list of guids provided when the
        device registered.

    InstanceIndex - Supplies the index that denotes which instance of the data
        block is being queried.

    InstanceCount - Supplies the number of instances expected to be returned for
        the data block.

    InstanceLengthArray - Supplies a pointer to an array of ULONG that returns
        the lengths of each instance of the data block. If this is NULL then
        there was not enough space in the output buffer to fulfill the request
        so the irp should be completed with the buffer needed.

    BufferAvail - Supplies the maximum size available to write the data
        block.

    Buffer - Supplies a pointer to a buffer to return the data block.


Return Value:

    NTSTATUS

--*/

{

    PSIMBATT_FDO_DATA DevExt;
    WDFDEVICE Device;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(InstanceIndex);
    UNREFERENCED_PARAMETER(InstanceCount);

    DebugEnter();
    PAGED_CODE();

    ASSERT((InstanceIndex == 0) && (InstanceCount == 1));

    if (InstanceLengthArray == NULL) {
        Status = STATUS_BUFFER_TOO_SMALL;
        goto SimBattQueryWmiDataBlockEnd;
    }

    Device = WdfWdmDeviceGetWdfDeviceHandle(DeviceObject);
    DevExt = GetDeviceExtension(Device);

    //
    // The class driver guarantees that all outstanding IO requests will be
    // completed before it finishes unregistering. As a result, the class
    // initialization lock does not need to be acquired in this callback, since
    // it is called during class driver processing of a WMI IRP.
    //

    Status = BatteryClassQueryWmiDataBlock(DevExt->ClassHandle,
                                           DeviceObject,
                                           Irp,
                                           GuidIndex,
                                           InstanceLengthArray,
                                           BufferAvail,
                                           Buffer);

    if (Status == STATUS_WMI_GUID_NOT_FOUND) {
        Status = WmiCompleteRequest(DeviceObject,
                                    Irp,
                                    STATUS_WMI_GUID_NOT_FOUND,
                                    0,
                                    IO_NO_INCREMENT);
    }

SimBattQueryWmiDataBlockEnd:
    DebugExitStatus(Status);
    return Status;
}
