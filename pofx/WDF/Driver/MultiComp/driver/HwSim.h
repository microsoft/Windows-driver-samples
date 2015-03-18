/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    HwSim.h

Abstract:
    Header file for the hardware simulation module of the KMDF sample driver for
    a multi-component device.

Environment:

    Kernel mode

--*/

#include "WdfMultiComp.h"

#if !defined(_HWSIM_H_)
#define _HWSIM_H_

NTSTATUS
HwSimInitialize(
    _In_ WDFDEVICE Device
    );
    
VOID
HwSimD0Entry(
    _In_ WDFDEVICE Device
    );
    
VOID
HwSimD0Exit(
    _In_ WDFDEVICE Device
    );
    
VOID
HwSimFStateChange(
    _In_ WDFDEVICE Device, 
    _In_ ULONG Component, 
    _In_ ULONG State
    );

ULONG
HwSimReadComponent(
    _In_ WDFDEVICE Device, 
    _In_ ULONG Component
    );

//
// This structure represents the hardware simulation module's device context
// space
//
typedef struct _HWSIM_CONTEXT {
    //
    // The following array tracks the F-state for each of the device's 
    // components. It is used for verification purposes only.
    //
    ULONG ComponentFState[COMPONENT_COUNT];

    //
    // The following member tracks whether or not the device is in D0. It is 
    // used for verification purposes only.
    //
    BOOLEAN DevicePoweredOn;

    //
    // The following member tracks whether or not we have previously entered the
    // D0 state for this device
    //
    BOOLEAN FirstD0Entry;
} HWSIM_CONTEXT, *PHWSIM_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(HWSIM_CONTEXT, HwSimGetDeviceContext)

#endif // _HWSIM_H_
