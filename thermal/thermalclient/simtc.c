/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    simtc.c

Abstract:

    The module implements a simulated thermal client.

@@BEGIN_DDKSPLIT
Author:

    Nicholas Brekhus (NiBrekhu) 26-Jul-2011

Revision History:

@@END_DDKSPLIT
--*/

//-------------------------------------------------------------------- Includes

#include "simtc.h"

//--------------------------------------------------------------------- Globals

ULONG SimThermalClientDebug = SIMTC_PRINT_ALWAYS;

//------------------------------------------------------------------ Prototypes

DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_DEVICE_ADD           SimTcDriverDeviceAdd;

DEVICE_ACTIVE_COOLING               SimTcEngageActiveCooling;
DEVICE_PASSIVE_COOLING              SimTcEngagePassiveCooling;

//--------------------------------------------------------------------- Pragmas

#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, SimTcDriverDeviceAdd)
#pragma alloc_text(PAGE, SimTcEngageActiveCooling)
#pragma alloc_text(PAGE, SimTcEngagePassiveCooling)

//------------------------------------------------------------------- Functions

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
    NTSTATUS Status;

    DebugEnter();

    WDF_DRIVER_CONFIG_INIT(&DriverConfig, SimTcDriverDeviceAdd);

    //
    // Initialize attributes and a context area for the driver object.
    //

    WDF_OBJECT_ATTRIBUTES_INIT(&DriverAttributes);
    DriverAttributes.SynchronizationScope = WdfSynchronizationScopeNone;

    //
    // Create the driver object
    //

    Status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             &DriverAttributes,
                             &DriverConfig,
                             WDF_NO_HANDLE);

    if (!NT_SUCCESS(Status)) {
        DebugPrint(SIMTC_ERROR,
                   "WdfDriverCreate() Failed. Status 0x%x\n",
                   Status);

        goto DriverEntryEnd;
    }

DriverEntryEnd:
    DebugExitStatus(Status);
    return Status;
}

NTSTATUS
SimTcDriverDeviceAdd (
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

    PFDO_DATA DevExt;
    WDF_OBJECT_ATTRIBUTES DeviceAttributes;
    WDFDEVICE DeviceHandle;
    WDF_QUERY_INTERFACE_CONFIG QueryInterfaceConfig;
    NTSTATUS Status;
    THERMAL_DEVICE_INTERFACE ThermalDeviceInterface;

    UNREFERENCED_PARAMETER(Driver);

    DebugEnter();
    PAGED_CODE();

    //
    // Initialize attributes and a context area for the device object.
    //

    WDF_OBJECT_ATTRIBUTES_INIT(&DeviceAttributes);
    WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(&DeviceAttributes, FDO_DATA);

    //
    // Create a framework device object.  This call will in turn create
    // a WDM device object, attach to the lower stack, and set the
    // appropriate flags and attributes.
    //

    Status = WdfDeviceCreate(&DeviceInit, &DeviceAttributes, &DeviceHandle);
    if (!NT_SUCCESS(Status)) {
        DebugPrint(SIMTC_ERROR, "WdfDeviceCreate() Failed. 0x%x\n", Status);
        goto DriverDeviceAddEnd;
    }

    //
    // Create a driver interface for this device to advertise the thermal
    // cooling interface.
    //

    RtlZeroMemory(&ThermalDeviceInterface, sizeof(ThermalDeviceInterface));

    ThermalDeviceInterface.Size =
        sizeof(ThermalDeviceInterface);

    ThermalDeviceInterface.Version = 1;

    ThermalDeviceInterface.Context = DeviceHandle;
    ThermalDeviceInterface.InterfaceReference =
        WdfDeviceInterfaceReferenceNoOp;

    ThermalDeviceInterface.InterfaceDereference =
        WdfDeviceInterfaceDereferenceNoOp;

    ThermalDeviceInterface.Flags = ThermalDeviceFlagPassiveCooling
                                 | ThermalDeviceFlagActiveCooling;

    ThermalDeviceInterface.ActiveCooling = SimTcEngageActiveCooling;
    ThermalDeviceInterface.PassiveCooling = SimTcEngagePassiveCooling;

    WDF_QUERY_INTERFACE_CONFIG_INIT(&QueryInterfaceConfig,
                                    (PINTERFACE) &ThermalDeviceInterface,
                                    &GUID_THERMAL_COOLING_INTERFACE,
                                    NULL);

    Status = WdfDeviceAddQueryInterface(DeviceHandle, &QueryInterfaceConfig);
    if (!NT_SUCCESS(Status)) {
        DebugPrint(SIMTC_ERROR,
                   "WdfDeviceAddQueryInterface() Failed. 0x%x\n",
                   Status);

        goto DriverDeviceAddEnd;
    }

    Status = WdfDeviceCreateDeviceInterface(DeviceHandle,
                                            &GUID_DEVINTERFACE_THERMAL_COOLING,
                                            NULL);

    if (!NT_SUCCESS(Status)) {
        DebugPrint(SIMTC_ERROR,
                   "WdfDeviceCreateDeviceInterface() Failed. 0x%x\n",
                   Status);

        goto DriverDeviceAddEnd;
    }

    //
    // Finish initializing the device context area.
    //

    DevExt = GetDeviceExtension(DeviceHandle);
    DevExt->ThermalLevel = 100UL;
    DevExt->ActiveCoolingEngaged = FALSE;

DriverDeviceAddEnd:
    DebugExitStatus(Status);
    return Status;
}

VOID
SimTcEngageActiveCooling(
    _Inout_opt_ PVOID Context,
    _In_        BOOLEAN Engaged
    )

/*++

Routine Description:

    SimTcEngageActiveCooling is called by the device's clients to set the
    device's active cooling state.

Arguments:

    Context - Supplies a handle to the target device.

    Engaged - Supplies the new cooling state.

Return Value:

    None

--*/

{
    PFDO_DATA DevExt;
    BOOLEAN PreviousActiveCoolingState;

    DebugEnter();
    PAGED_CODE();

    _Analysis_assume_(Context != NULL);
    NT_ASSERT(Context != NULL);
    DevExt = GetDeviceExtension((WDFDEVICE)Context);

    PreviousActiveCoolingState = DevExt->ActiveCoolingEngaged;
    DevExt->ActiveCoolingEngaged = Engaged;

    DebugPrint(SIMTC_TRACE,
               "Active cooling state was %s, now %s.\n",
               (PreviousActiveCoolingState != FALSE) ? "on" : "off",
               (Engaged != FALSE) ? "on" : "off");

    DebugExit();
}

VOID
SimTcEngagePassiveCooling (
    _Inout_opt_ PVOID Context,
    _In_        ULONG Percentage
    )

/*++

Routine Description:

    SimTcEngagePassiveCooling is called by the device's clients to set the
    device's passive cooling state.

Arguments:

    Context - Supplies a handle to the target device.

    Percentage - Supplies the new thermal level in percent.

Return Value:

    None

--*/

{
    PFDO_DATA DevExt;
    ULONG PreviousThermalLevel;

    DebugEnter();
    PAGED_CODE();

    _Analysis_assume_(Context != NULL);
    NT_ASSERT(Context != NULL);
    DevExt = GetDeviceExtension((WDFDEVICE)Context);

    PreviousThermalLevel = DevExt->ThermalLevel;
    DevExt->ThermalLevel = Percentage;

    DebugPrint(SIMTC_TRACE,
               "Thermal level was %lu, now %lu.\n",
               PreviousThermalLevel,
               Percentage);

    DebugExit();
}
