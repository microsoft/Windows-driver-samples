/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Power.C

Abstract:

    Implements callbacks to manager power transition, wait-wake and selective
    suspend.

Environment:

    Kernel mode

--*/

#include "toaster.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, ToasterEvtDeviceD0Exit)
#pragma alloc_text(PAGE, ToasterEvtDeviceArmWakeFromS0)
#pragma alloc_text(PAGE, ToasterEvtDeviceArmWakeFromSx)
#pragma alloc_text(PAGE, ToasterEvtDeviceWakeFromS0Triggered)
#pragma alloc_text(PAGE, DbgDevicePowerString)
#endif // ALLOC_PRAGMA


NTSTATUS
ToasterEvtDeviceD0Entry(
    IN WDFDEVICE                Device,
    IN WDF_POWER_DEVICE_STATE   RecentPowerState
    )
/*++
Routine Description:

    EvtDeviceD0Entry event is called to program the device to goto
    D0, which is the working state. The framework calls the driver's
    EvtDeviceD0Entry callback when the Power manager sends an
    IRP_MN_SET_POWER-DevicePower request to the driver stack. The Power manager
    sends this request when the power policy manager of this device stack
    (probaby the FDO) requests a change in D-state by calling PoRequestPowerIrp.

    This function is not marked pageable because this function is in the
    device power up path. When a function is marked pagable and the code
    section is paged out, it will generate a page fault which could impact
    the fast resume behavior because the client driver will have to wait
    until the system drivers can service this page fault.

Arguments:

    Device - handle to a framework device object.

    RecentPowerState - WDF_POWER_DEVICE_STATE-typed enumerator that identifies the
                device power state that the device was in before this transition
                to D0.

Return Value:

    NTSTATUS    - A failure here will indicate a fatal error in the driver.
                  The Framework will attempt to tear down the stack.

--*/
{
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(RecentPowerState);

    KdPrint(("ToasterEvtDeviceD0Entry - coming from %s\n",
              DbgDevicePowerString(RecentPowerState)));

    return STATUS_SUCCESS;
}

NTSTATUS
ToasterEvtDeviceD0Exit(
    IN WDFDEVICE                Device,
    IN WDF_POWER_DEVICE_STATE   PowerState
    )
/*++
Routine Description:

    EvtDeviceD0Entry event is called to program the device to goto
    D1, D2 or D3, which are the low-power states. The framework calls the
    driver's EvtDeviceD0Exit callback when the Power manager sends an
    IRP_MN_SET_POWER-DevicePower request to the driver stack. The Power manager
    sends this request when the power policy manager of this device stack
    (probaby the FDO) requests a change in D-state by calling PoRequestPowerIrp.

Arguments:

    Device - handle to a framework device object.

    DeviceState - WDF_POWER_DEVICE_STATE-typed enumerator that identifies the
                device power state that the power policy owner (probably the
                FDO) has decided is appropriate.

Return Value:

    NTSTATUS    - A failure here will indicate a fatal error in the driver.
                  The Framework will attempt to tear down the stack.
--*/
{
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(PowerState);

    PAGED_CODE();

    KdPrint(("ToasterEvtDeviceD0Exit %s\n",
              DbgDevicePowerString(PowerState)));

    return STATUS_SUCCESS;
}

NTSTATUS
ToasterEvtDeviceArmWakeFromS0(
    IN WDFDEVICE Device
    )
/*++

Routine Description:

    EvtDeviceArmWakeFromS0 is called when the Framework arms the device for
    wake from S0.  If there is any device-specific initialization
    that needs to be done to arm internal wake signals, or to route internal
    interrupt signals to the wake logic, it should be done here.  The device
    will be moved out of the D0 state soon after this callback is invoked.

    This function is pageable and it will run at PASSIVE_LEVEL.

Arguments:

    Device - Handle to a Framework device object.

Return Value:

    NTSTATUS - Failure will result in the device remaining in the D0 state.

--*/
{
    UNREFERENCED_PARAMETER(Device);

    PAGED_CODE();

    KdPrint(( "--> ToasterEvtDeviceArmWakeFromS0\n"));

    KdPrint(( "<-- ToasterEvtDeviceArmWakeFromS0\n"));

    return STATUS_SUCCESS;
}

NTSTATUS
ToasterEvtDeviceArmWakeFromSx(
    IN WDFDEVICE Device
    )
/*++

Routine Description:

    EvtDeviceArmWakeFromSx is called when the Framework arms the device for
    wake from Sx.  If there is any device-specific initialization
    that needs to be done to arm internal wake signals, or to route internal
    interrupt signals to the wake logic, it should be done here.  The device
    will be moved out of the D0 state soon after this callback is invoked.

    This function is pageable and it will run at PASSIVE_LEVEL.

Arguments:

    Device - Handle to a Framework device object.

Return Value:

    NTSTATUS - Failure will result in the device remaining in the D0 state.

--*/
{
    UNREFERENCED_PARAMETER(Device);

    PAGED_CODE();

    KdPrint(( "--> ToasterEvtDeviceArmWakeFromSx\n"));

    KdPrint(( "<-- ToasterEvtDeviceArmWakeFromSx\n"));

    return STATUS_SUCCESS;
}

VOID
ToasterEvtDeviceDisarmWakeFromS0(
    IN WDFDEVICE Device
    )
/*++

Routine Description:

    EvtDeviceDisarmWakeFromS0 reverses anything done in EvtDeviceArmWakeFromS0.

    This function is not marked pageable because this function is in the
    device power up path. When a function is marked pagable and the code
    section is paged out, it will generate a page fault which could impact
    the fast resume behavior because the client driver will have to wait
    until the system drivers can service this page fault.

Arguments:

    Device - Handle to a Framework device object.

Return Value:

    VOID.

--*/
{
    UNREFERENCED_PARAMETER(Device);

    KdPrint(( "--> ToasterEvtDeviceDisarmWakeFromS0\n"));

    KdPrint(( "<-- ToasterEvtDeviceDisarmWakeFromS0\n"));

    return ;
}

VOID
ToasterEvtDeviceDisarmWakeFromSx(
    IN WDFDEVICE Device
    )
/*++

Routine Description:

    EvtDeviceDisarmWakeFromSx reverses anything done in EvtDeviceArmWakeFromSx.

    This function will run at PASSIVE_LEVEL.

    This function is not marked pageable because this function is in the
    device power up path. When a function is marked pagable and the code
    section is paged out, it will generate a page fault which could impact
    the fast resume behavior because the client driver will have to wait
    until the system drivers can service this page fault.

Arguments:

    Device - Handle to a Framework device object.

Return Value:

    VOID.

--*/
{
    UNREFERENCED_PARAMETER(Device);

    KdPrint(( "--> ToasterEvtDeviceDisarmWakeFromSx\n"));

    KdPrint(( "<-- ToasterEvtDeviceDisarmWakeFromSx\n"));

    return ;
}

VOID
ToasterEvtDeviceWakeFromS0Triggered(
    IN WDFDEVICE Device
    )
/*++

Routine Description:

    EvtDeviceWakeFromS0Triggered will be called whenever the device triggers its
    wake signal after being armed for wake.

    This function is pageable and runs at PASSIVE_LEVEL.

Arguments:

    Device - Handle to a Framework device object.

Return Value:

    VOID

--*/
{
    UNREFERENCED_PARAMETER(Device);

    PAGED_CODE();

    KdPrint(( "--> ToasterEvtDeviceWakeFromS0Triggered\n"));


    KdPrint(( "<-- ToasterEvtDeviceWakeFromS0Triggered\n"));

}

VOID
ToasterEvtDeviceWakeFromSxTriggered(
    IN WDFDEVICE Device
    )
/*++

Routine Description:

    EvtDeviceWakeFromSxTriggered will be called whenever the device triggers its
    wake signal after being armed for wake.

    This function runs at PASSIVE_LEVEL.

    This function is not marked pageable because this function is in the
    device power up path. When a function is marked pagable and the code
    section is paged out, it will generate a page fault which could impact
    the fast resume behavior because the client driver will have to wait
    until the system drivers can service this page fault.

Arguments:

    Device - Handle to a Framework device object.

Return Value:

    VOID

--*/
{
    UNREFERENCED_PARAMETER(Device);

    KdPrint(( "--> ToasterEvtDeviceWakeFromSxTriggered\n"));

    KdPrint(( "<-- ToasterEvtDeviceWakeFromSxTriggered\n"));

}

PCHAR
DbgDevicePowerString(
    IN WDF_POWER_DEVICE_STATE Type
    )
/*++

New Routine Description:
    DbgDevicePowerString converts the device power state code of a power IRP to a
    text string that is helpful when tracing the execution of power IRPs.

Parameters Description:
    Type
    Type specifies the device power state code of a power IRP.

Return Value Description:
    DbgDevicePowerString returns a pointer to a string that represents the
    text description of the incoming device power state code.

--*/
{
    PAGED_CODE();

    switch (Type)
    {
    case WdfPowerDeviceInvalid:
        return "WdfPowerDeviceInvalid";
    case WdfPowerDeviceD0:
        return "WdfPowerDeviceD0";
    case WdfPowerDeviceD1:
        return "WdfPowerDeviceD1";
    case WdfPowerDeviceD2:
        return "WdfPowerDeviceD2";
    case WdfPowerDeviceD3:
        return "WdfPowerDeviceD3";
    case WdfPowerDeviceD3Final:
        return "WdfPowerDeviceD3Final";
    case WdfPowerDevicePrepareForHibernation:
        return "WdfPowerDevicePrepareForHibernation";
    case WdfPowerDeviceMaximum:
        return "WdfPowerDeviceMaximum";
    default:
        return "UnKnown Device Power State";
    }
}




