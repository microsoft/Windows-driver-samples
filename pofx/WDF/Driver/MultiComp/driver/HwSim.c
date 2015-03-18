/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    HwSim.c

Abstract:
    This module implements a simple hardware simulator that simulates reading of
    data from the device's components. In this sample, the "data" that is read 
    is simply the bitwise complement of the component number. In other words, 
    the data for component number 'X' is simply '~X'.

    The hardware simulator also verifies that when a component is read, the 
    device is in D0 and the component is in F0. If not, it breaks into the 
    debugger.

Environment:

    Kernel mode

--*/

#include "HwSim.h"
#include "HwSim.tmh"

NTSTATUS
HwSimInitialize(
    _In_ WDFDEVICE Device
    )
/*++
Routine Description:

    This routine initializes the hardware simulator

Arguments:

    Device - Handle to the framework device object

Return Value:

    An NTSTATUS value representing success or failure of the function.

--*/
{
    NTSTATUS status;
    WDF_OBJECT_ATTRIBUTES objectAttributes;
    PHWSIM_CONTEXT devCtx;

    //
    // Allocate our context for this device
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&objectAttributes, 
                                            HWSIM_CONTEXT);
    status = WdfObjectAllocateContext((WDFOBJECT) Device,
                                      &objectAttributes,
                                      (PVOID*) &devCtx);
    if (FALSE == NT_SUCCESS(status)) {
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - WdfObjectAllocateContext failed with %!status!", 
              status);
        goto exit;
    }

    devCtx->FirstD0Entry = TRUE;

    status = STATUS_SUCCESS;
    
exit:
    return status;
}

VOID
HwSimD0Entry(
    _In_ WDFDEVICE Device
    )
/*++
Routine Description:

    This routine simulates the device entering D0

Arguments:

    Device - Handle to the framework device object

Return Value:

    None

--*/
{
    PHWSIM_CONTEXT devCtx;
    ULONG i;

    devCtx = HwSimGetDeviceContext(Device);

    if (devCtx->FirstD0Entry) {
        devCtx->FirstD0Entry = FALSE;

        //
        // Put all components in F0
        //
        for (i=0; i < COMPONENT_COUNT; i++) {
            devCtx->ComponentFState[i] = 0;
        }
    }

    devCtx->DevicePoweredOn = TRUE;
    
    return;
}
    
VOID
HwSimD0Exit(
    _In_ WDFDEVICE Device
    )
/*++
Routine Description:

    This routine simulates the device exiting D0

Arguments:

    Device - Handle to the framework device object

Return Value:

    None

--*/
{
    PHWSIM_CONTEXT devCtx;

    devCtx = HwSimGetDeviceContext(Device);

    devCtx->DevicePoweredOn = FALSE;
    
    return;
}
    
VOID
HwSimFStateChange(
    _In_ WDFDEVICE Device, 
    _In_ ULONG Component, 
    _In_ ULONG State
    )
/*++
Routine Description:

    This routine simulates a component changing its F-state

Arguments:

    Device - Handle to the framework device object

    Component - Index of the component whose F-state is changing

    State - New F-state for the component

Return Value:

    None

--*/
{
    PHWSIM_CONTEXT devCtx;

    devCtx = HwSimGetDeviceContext(Device);

    //
    // The caller should have ensured that this routine is called only when the
    // device is powered on. Verify this. If false, we'll generate a verifier 
    // breakpoint.
    //
    if (FALSE == devCtx->DevicePoweredOn) {
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - Expected device to be powered on, but it was not.");
        WdfVerifierDbgBreakPoint();
    }

    //
    // Put the component in the requested F-state
    //
    devCtx->ComponentFState[Component] = State;
    return;
}

ULONG
HwSimReadComponent(
    _In_ WDFDEVICE Device, 
    _In_ ULONG Component
    )
/*++
Routine Description:

    This routine simulates the reading of data from a component

Arguments:

    Device - Handle to the framework device object

    Component - Component from which data is being read

Return Value:

    A ULONG value representing the data that was read from the component

--*/
{
    ULONG componentData;
    PHWSIM_CONTEXT devCtx;

    devCtx = HwSimGetDeviceContext(Device);

    //
    // The caller should have ensured that this routine is called only when the
    // device is powered on. Verify this. If false, we'll generate a verifier 
    // breakpoint.
    //
    if (FALSE == devCtx->DevicePoweredOn) {
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - Expected device to be powered on, but it was not.");
        WdfVerifierDbgBreakPoint();
    }

    //
    // The caller should have ensured that this routine is called only when the
    // component is in F0. Verify this. If false, we'll generate a verifier 
    // breakpoint.
    //
    if (0 != devCtx->ComponentFState[Component]) {
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - Expected component %d to be in F0, but it was in F%d.", 
              Component,
              devCtx->ComponentFState[Component]);
        WdfVerifierDbgBreakPoint();
    }

    //
    // In this sample, component data is just a bit-wise complement of the 
    // component number.
    //
    componentData = ~Component;
    return componentData;
}
