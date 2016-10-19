/*++

Module Name:

    queue.h

Abstract:

    This file contains the queue declarations.

Environment:

    Kernel-mode Driver Framework

--*/

EXTERN_C_START

NTSTATUS
HardwareRequestQueueInitialize(
    _In_ WDFDEVICE Device
);

EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL
EvtIoDeviceControl;

EVT_WDF_IO_QUEUE_IO_STOP
EvtIoStop;

EXTERN_C_END