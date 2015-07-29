/*++
    Copyright (c) Microsoft Corporation. All Rights Reserved. 
    Sample code. Dealpoint ID #843729.

    Module Name:

        power.c

    Abstract:

        Contains Synaptics power-on and power-off functionality

    Environment:

        Kernel mode

    Revision History:

--*/

#include "controller.h"
#include "rmiinternal.h"
#include "spb.h"
#include "power.tmh"

NTSTATUS
RmiChangeSleepState(
   IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
   IN SPB_CONTEXT *SpbContext,
   IN UCHAR SleepState
   )
/*++

Routine Description:

   Changes the SleepMode bits on the controller as specified

Arguments:

   ControllerContext - Touch controller context
   
   SpbContext - A pointer to the current i2c context

   SleepState - Either RMI4_F11_DEVICE_CONTROL_SLEEP_MODE_OPERATING
                or RMI4_F11_DEVICE_CONTROL_SLEEP_MODE_SLEEPING

Return Value:

   NTSTATUS indicating success or failure

--*/
{
    RMI4_F01_CTRL_REGISTERS* controlF01;
    UCHAR deviceControl;
    int index;
    NTSTATUS status;

    controlF01 = (RMI4_F01_CTRL_REGISTERS*) &deviceControl;

    //
    // Find RMI device control function housing sleep settings
    // 
    index = RmiGetFunctionIndex(
        ControllerContext->Descriptors,
        ControllerContext->FunctionCount,
        RMI4_F01_RMI_DEVICE_CONTROL);

    if (index == ControllerContext->FunctionCount)
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_POWER,
            "Power change failure - RMI Function 01 missing");

        status = STATUS_INVALID_DEVICE_STATE;
        goto exit;
    }

    status = RmiChangePage(
        ControllerContext,
        SpbContext,
        ControllerContext->FunctionOnPage[index]);

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_POWER,
            "Could not change register page");

        goto exit;
    }

    //
    // Read Device Control register
    //
    status = SpbReadDataSynchronously(
        SpbContext,
        ControllerContext->Descriptors[index].ControlBase,
        &deviceControl,
        sizeof(deviceControl)
        );

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_POWER,
            "Could not read sleep register - %!STATUS!",
            status);

        goto exit;
    }

    //
    // Assign new sleep state
    //
    controlF01->DeviceControl.SleepMode = SleepState;

    //
    // Write setting back to the controller
    //
    status = SpbWriteDataSynchronously(
        SpbContext,
        ControllerContext->Descriptors[index].ControlBase,
        &deviceControl,
        sizeof(deviceControl)
        );

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_POWER,
            "Could not write sleep register - %X",
            status);

        goto exit;
    }

exit:

    return status;
}

NTSTATUS 
TchWakeDevice(
   IN VOID *ControllerContext,
   IN SPB_CONTEXT *SpbContext
   )
/*++

Routine Description:

   Enables multi-touch scanning

Arguments:

   ControllerContext - Touch controller context
   
   SpbContext - A pointer to the current i2c context

Return Value:

   NTSTATUS indicating success or failure

--*/
{    
    RMI4_CONTROLLER_CONTEXT* controller;
    NTSTATUS status;

    controller = (RMI4_CONTROLLER_CONTEXT*) ControllerContext;

    //
    // Check if we were already on
    //
    if (controller->DevicePowerState == PowerDeviceD0)
    {
        goto exit;
    }

    controller->DevicePowerState = PowerDeviceD0;

    //
    // Attempt to put the controller into operating mode 
    //
    status = RmiChangeSleepState(
        controller,
        SpbContext,
        RMI4_F11_DEVICE_CONTROL_SLEEP_MODE_OPERATING);

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_POWER,
            "Error waking touch controller - %!STATUS!",
            status);
    }

exit:

    return STATUS_SUCCESS;
}

NTSTATUS
TchStandbyDevice(
   IN VOID *ControllerContext,
   IN SPB_CONTEXT *SpbContext
   )
/*++

Routine Description:

   Disables multi-touch scanning to conserve power

Arguments:

   ControllerContext - Touch controller context
   
   SpbContext - A pointer to the current i2c context

Return Value:

   NTSTATUS indicating success or failure

--*/
{
    RMI4_CONTROLLER_CONTEXT* controller;
    NTSTATUS status;

    controller = (RMI4_CONTROLLER_CONTEXT*) ControllerContext;

    //
    // Interrupts are now disabled but the ISR may still be
    // executing, so grab the controller lock to ensure ISR
    // is finished touching HW and controller state.
    //
    WdfWaitLockAcquire(controller->ControllerLock, NULL);

    //
    // Put the chip in sleep mode
    //
    status = RmiChangeSleepState(
        ControllerContext,
        SpbContext,
        RMI4_F11_DEVICE_CONTROL_SLEEP_MODE_SLEEPING);

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_POWER,
            "Error sleeping touch controller - %!STATUS!",
            status);
    }

    controller->DevicePowerState = PowerDeviceD3;

    //
    // Invalidate state
    //
    controller->TouchesReported = 0;
    controller->TouchesTotal = 0;
    controller->Cache.FingerSlotValid = 0;
    controller->Cache.FingerSlotDirty = 0;
    controller->Cache.FingerDownCount = 0;

    WdfWaitLockRelease(controller->ControllerLock);

    return STATUS_SUCCESS;
}