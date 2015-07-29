/*++

Module Name:

    device.h

Abstract:

    This file contains the device definitions.

Environment:

    Kernel-mode Driver Framework

--*/
#pragma once

#include "driver.h"
#include "transfer.h"
#include "registers.h"
#include "ufxdevice.h"
#include <ntstrsafe.h>

#define UFXCLIENT_TAG ('CXFU') // UFXC

#define TEST_BIT(value, bitNumber) ((value) & (1<<(bitNumber))) ? TRUE : FALSE

#define MAX_DMA_LENGTH 0x1000

#define UFX_CLIENT_ALIGNMENT 128

//
// Time interval to wait after being suspended and before requesting remote
// wakeup request 
//
#define REMOTE_WAKEUP_TIMEOUT_INTERVAL_MS 1

//
// Context space for WDFDEVICE object representing the controller
//
typedef struct _CONTROLLER_CONTEXT {

    UFXDEVICE UfxDevice;

    WDFINTERRUPT DeviceInterrupt;

    WDFINTERRUPT WakeInterrupt;

    WDFINTERRUPT AttachDetachInterrupt;

    KEVENT DetachEvent;

    WDFDMAENABLER DmaEnabler;

    //
    // This variable tells us if we have indicated to UFX, via UfxDeviceNotifyAttach, that we are
    // attached.
    //
    BOOLEAN WasAttached;

    //
    // This variable tracks what the hardware thinks about the current
    // attachment status. It is updated right away based on incoming notifications.
    //
    volatile BOOLEAN Attached;

    //
    // This variable tracks whether there was some change, possibly transient, in the attachment
    // status. Simply inspecting "Attached", one may conclude that there was no change at all, but
    // this variable will tell us if there was a rapid attach-detach or detach-attach event.
    BOOLEAN GotAttachOrDetach;

    WDFSPINLOCK DpcLock;

    BOOLEAN RemoteWakeupRequested;

    BOOLEAN StopPending;

    BOOLEAN Connect;

    BOOLEAN Suspended;

    USB_DEVICE_SPEED Speed;

    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS IdleSettings;

    BOOLEAN InitializeDefaultEndpoint;

    WDFWAITLOCK InitializeDefaultEndpointLock;

} CONTROLLER_CONTEXT, *PCONTROLLER_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(CONTROLLER_CONTEXT, DeviceGetControllerContext)


typedef enum _UFX_CLIENT_EXCEPTION_CODE {
    ExceptionDisconnectTimeout = 0,
    ExceptionMaximum
} UFX_CLIENT_EXCEPTION_CODE, *PUFX_CLIENT_EXCEPTION_CODE;

typedef struct _HARDWARE_FAILURE_CONTEXT {
    ULONG Size;
    UFX_CLIENT_EXCEPTION_CODE ExceptionCode;
    CONTROLLER_CONTEXT ControllerContext;
    UFXDEVICE_CONTEXT DeviceContext;
} HARDWARE_FAILURE_CONTEXT, *PHARDWARE_FAILURE_CONTEXT;

//
// Function to initialize the device and its callbacks
//
_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
UfxClientDeviceCreate (
    _In_ WDFDRIVER Driver,
    _In_ PWDFDEVICE_INIT DeviceInit
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
DevicePerformSoftReset (
    _In_ WDFDEVICE Device
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
DeviceHardwareFailure (
    _In_ WDFDEVICE Device,
    _In_ ULONG ExceptionCode
    );

VOID
DeviceVendorReset (
    _In_ WDFDEVICE Device
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
DeviceInitializeDefaultEndpoint (
    _In_ WDFDEVICE Device
    );
