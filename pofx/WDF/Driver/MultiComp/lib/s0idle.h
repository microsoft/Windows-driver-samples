/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    s0idle.h

Abstract:
    Private header file that enables the power framework library to be 
    conditionally compiled either with or without S0-idle power management 
    support, based on the value of the PFH_S0IDLE_SUPPORTED compile-time
    switch.

Environment:

    Kernel mode

--*/

#if !defined(_S0IDLE_H_)
#define _S0IDLE_H_

EVT_WDF_WORKITEM _PowerRequiredPassiveHandler;

#if PFH_S0IDLE_SUPPORTED

//
// The power framework library is compiled with S0-idle power management support
//
NTSTATUS
InitializeDeviceSettingsForS0Idle(
    _In_ WDFDEVICE Device,
    _Inout_ PPOFX_DEVICE_CONTEXT DevCtx
    );
    
NTSTATUS
StopDeviceIdleOnDeviceStart(
    _In_ WDFDEVICE Device,
    _In_ PPOFX_DEVICE_CONTEXT DevCtx
    );

VOID
DevicePowerRequiredForS0Idle(
    _In_ WDFDEVICE Device,
    _In_ PPOFX_DEVICE_CONTEXT DevCtx
    );

VOID
DevicePowerNotRequiredForS0Idle(
    _In_ WDFDEVICE Device,
    _In_ PPOFX_DEVICE_CONTEXT DevCtx
    );

#define INITIALIZE_DEVICE_SETTINGS_FOR_S0_IDLE(_Device, _DevCtx)               \
                    InitializeDeviceSettingsForS0Idle(_Device, _DevCtx)

#define STOP_DEVICE_IDLE_ON_DEVICE_START(_Status, _Device, _DevCtx, _ExitLabel)\
{                                                                              \
    _Status = StopDeviceIdleOnDeviceStart(_Device, _DevCtx);                   \
    if (FALSE == NT_SUCCESS(_Status)) {                                        \
        goto _ExitLabel;                                                       \
    }                                                                          \
}

//
// For PDOs, if the device is restarted after being disabled, we need to   
// prevent device idle until the power framework permits it. Therefore, we 
// reset the StopIdleInvokedOnDeviceStart value here, so that a subsequent
// restart causes us to invoke WdfDeviceStopIdle to prevent device idling.
//
#define SELF_MANAGED_IO_FLUSH_FOR_S0_IDLE(_DevCtx)                             \
{                                                                              \
    _DevCtx->StopIdleInvokedOnDeviceStart = FALSE;                             \
}

#define DEVICE_POWER_REQUIRED_FOR_S0_IDLE(_Device, _DevCtx)                    \
                        DevicePowerRequiredForS0Idle(_Device,_DevCtx)

#define DEVICE_POWER_NOT_REQUIRED_FOR_S0_IDLE(_Device, _DevCtx)                \
                        DevicePowerNotRequiredForS0Idle(_Device, _DevCtx)

#else // PFH_S0IDLE_SUPPORTED


//
// The power framework library is compiled without S0-idle power management 
// support
//
#define INITIALIZE_DEVICE_SETTINGS_FOR_S0_IDLE(_Device, _DevCtx)    (0)
#define STOP_DEVICE_IDLE_ON_DEVICE_START(_Status, _Device, _DevCtx, _ExitLabel)\
                                                                             (0)     
#define SELF_MANAGED_IO_FLUSH_FOR_S0_IDLE(_DevCtx)  (0)

//
// The current implementation does not support S0-idle. It assumes that the 
// device always remains in D0 unless there is a system sleep transition, 
// resource rebalance or device removal. Therefore, we call 
// PoFxReportDevicePoweredOn immediately.
//
#define DEVICE_POWER_REQUIRED_FOR_S0_IDLE(_Device, _DevCtx)                    \
                        PoFxReportDevicePoweredOn(_DevCtx->PoHandle)

#define DEVICE_POWER_NOT_REQUIRED_FOR_S0_IDLE(_Device, _DevCtx) (0)

#endif // PFH_S0IDLE_SUPPORTED

#endif // _S0IDLE_H_
