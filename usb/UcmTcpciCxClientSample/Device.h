/*++

Module Name:

    Device.h

Abstract:

    This file contains the device declarations.

Environment:

    Kernel-mode Driver Framework

--*/

//
// The I2C_REQUEST_SOURCE enum defines the type of requests that we sent to I2C controller.
// At any time there will be at most one request per source type. For example:
//
//   1 - We receive an incoming request from EvtIoDeviceControl. The queue is sequential and
//       thus we'll get at most one incoming request at any time.
//   2 - On response, we can:
//       * Either send an async request to I2C controller (SourceAsync). Its completion routine
//         will complete the parent incoming request automatically;
//       * Or send one or multiple sync requests (SourceClient). The caller needs to complete the
//         parent incoming request manually.
//   3 - Also when I2C controller triggers an interrupt, the ISR handler can also:
//       * Send a sync request (SourceAlertIsr). The caller also needs to complete the parent
//         incoming request manually later.
//
enum I2C_REQUEST_SOURCE
{
    I2CRequestSourceAsync = 0,
    I2CRequestSourceClient,
    I2CRequestSourceAlertIsr,
    I2CRequestSourceMax
};

typedef struct _DEVICE_CONTEXT
{
    WDFDEVICE       Device;

    UCMTCPCIPORTCONTROLLER  PortController;

    // Request that we recieved from I/O queue.
    WDFREQUEST      IncomingRequest;

    // Requests that we created and sent out.
    WDFREQUEST      OutgoingRequests[I2CRequestSourceMax];

    // An alias since we use this async request frequently.
    #define         I2CAsyncRequest \
                    OutgoingRequests[I2CRequestSourceAsync]

    // Alert processing.
    WDFINTERRUPT    AlertInterrupt;

    WDFIOTARGET     I2CIoTarget;

    UINT8           I2CRegisterAddress;

    LARGE_INTEGER   I2CConnectionId;

    WDFMEMORY       I2CMemory;

    UINT8           I2CAsyncBuffer[I2C_BUFFER_SIZE];

    // Used to perform a device reset.
    DEVICE_RESET_INTERFACE_STANDARD ResetInterface;

    UINT8           ResetAttempts;

    WDFWORKITEM     I2CWorkItemGetStatus;

    WDFWORKITEM     I2CWorkItemGetControl;

} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, DeviceGetContext)

typedef struct _WORKITEM_CONTEXT
{
    WDFDEVICE  Device;
    WDFREQUEST Request;
} WORKITEM_CONTEXT, *PWORKITEM_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(WORKITEM_CONTEXT, WorkitemGetContext);

EXTERN_C_START

NTSTATUS
EvtCreateDevice(
    _Inout_ PWDFDEVICE_INIT DeviceInit
);

EVT_WDF_DEVICE_PREPARE_HARDWARE
EvtPrepareHardware;

EVT_WDF_DEVICE_D0_ENTRY
EvtDeviceD0Entry;

EVT_WDF_DEVICE_RELEASE_HARDWARE
EvtReleaseHardware;

EXTERN_C_END