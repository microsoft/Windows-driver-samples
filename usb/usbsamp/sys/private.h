/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    private.h

Abstract:

    Contains structure definitions and function prototypes private to
    the driver.

Environment:

    Kernel mode

--*/

#pragma warning(disable:4200)  //
#pragma warning(disable:4201)  // nameless struct/union
#pragma warning(disable:4214)  // bit field types other than int

#include <initguid.h>
#include <ntddk.h>
#include <ntintsafe.h>
#include "usbdi.h"
#include "usbdlib.h"

#pragma warning(default:4200)
#pragma warning(default:4201)
#pragma warning(default:4214)

#include <wdf.h>
#include <wdfusb.h>
#include "public.h"


#ifndef _H
#define _H

#define POOL_TAG (ULONG) 'SBSU'

#define MAX_SUPPORTED_PACKETS_FOR_SUPER_SPEED 1024
#define MAX_SUPPORTED_PACKETS_FOR_HIGH_SPEED 1024
#define MAX_SUPPORTED_PACKETS_FOR_FULL_SPEED 255

#define DISPATCH_LATENCY_IN_MS 10


#undef ExAllocatePool
#define ExAllocatePool(type, size) \
    ExAllocatePoolWithTag(type, size, POOL_TAG);

#if DBG

#define UsbSamp_DbgPrint(level, _x_) \
            if((level) <= DebugLevel) { \
                DbgPrint("UsbSamp: "); DbgPrint _x_; \
            }

#else

#define UsbSamp_DbgPrint(level, _x_)

#endif

#define MAX_FULL_SPEED_TRANSFER_SIZE   64
#define MAX_HIGH_SPEED_TRANSFER_SIZE   512
#define MAX_SUPER_SPEED_TRANSFER_SIZE  1024
#define MAX_STREAM_VALID_PACKET_SIZE   1024
#define REMOTE_WAKEUP_MASK 0x20

#define DEFAULT_REGISTRY_TRANSFER_SIZE 65536

#define IDLE_CAPS_TYPE IdleUsbSelectiveSuspend


//
// A structure representing the instance information associated with
// this particular device.
//

typedef struct _DEVICE_CONTEXT {

    USB_DEVICE_DESCRIPTOR           UsbDeviceDescriptor;

    PUSB_CONFIGURATION_DESCRIPTOR   UsbConfigurationDescriptor;

    WDFUSBDEVICE                    WdfUsbTargetDevice;

    ULONG                           WaitWakeEnable;

    BOOLEAN                         IsDeviceHighSpeed;

    BOOLEAN                         IsDeviceSuperSpeed;

    WDFUSBINTERFACE                 UsbInterface;

    UCHAR                           SelectedAlternateSetting;

    UCHAR                           NumberConfiguredPipes;

    ULONG                           MaximumTransferSize;

    WDFQUEUE                        IsochReadQueue;
    
    WDFQUEUE                        IsochWriteQueue;

    BOOLEAN                         IsStaticStreamsSupported;

    USHORT                          NumberOfStreamsSupportedByController;

    USBD_HANDLE                     UsbdHandle;

 
} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, GetDeviceContext)
//
// This context is associated with every open handle.
//
typedef struct _FILE_CONTEXT {

    WDFUSBPIPE Pipe;

} FILE_CONTEXT, *PFILE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FILE_CONTEXT, GetFileContext)

#if (NTDDI_VERSION >= NTDDI_WIN8)

typedef struct _USBSAMP_STREAM_INFO {

    // Number of enabled streams on this pipe
    ULONG NumberOfStreams;

    // Array of stream information structures representing streams on this pipe
    PUSBD_STREAM_INFORMATION StreamList;

} USBSAMP_STREAM_INFO, *PUSBSAMP_STREAM_INFO;

#endif

//
// This context is associated with every pipe handle. In this sample,
// it used for isoch transfers.
//
typedef struct _PIPE_CONTEXT {

    ULONG NextFrameNumber;    
    
    ULONG   TransferSizePerMicroframe;

    ULONG   TransferSizePerFrame;

    BOOLEAN  StreamConfigured;

#if (NTDDI_VERSION >= NTDDI_WIN8)
    USBSAMP_STREAM_INFO    StreamInfo;
#endif

} PIPE_CONTEXT, *PPIPE_CONTEXT;


WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(PIPE_CONTEXT, GetPipeContext)


//
// This context is associated with every request recevied by the driver
// from the app.
//
typedef struct _REQUEST_CONTEXT {

    WDFMEMORY         UrbMemory;
    PMDL              Mdl;
    ULONG             Length;         // remaining to xfer
    ULONG             Numxfer;
    ULONG_PTR         VirtualAddress; // va for next segment of xfer.
    BOOLEAN           Read; // TRUE if Read
} REQUEST_CONTEXT, * PREQUEST_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(REQUEST_CONTEXT, GetRequestContext)

typedef struct _WORKITEM_CONTEXT {
    WDFDEVICE       Device;
    WDFUSBPIPE      Pipe;
} WORKITEM_CONTEXT, *PWORKITEM_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(WORKITEM_CONTEXT, GetWorkItemContext)
extern ULONG DebugLevel;

DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_DEVICE_ADD UsbSamp_EvtDeviceAdd;
EVT_WDF_DEVICE_PREPARE_HARDWARE UsbSamp_EvtDevicePrepareHardware;

EVT_WDF_OBJECT_CONTEXT_CLEANUP UsbSamp_EvtDeviceContextCleanup;

#if (NTDDI_VERSION >= NTDDI_WIN8)
EVT_WDF_OBJECT_CONTEXT_CLEANUP  UsbSamp_EvtPipeContextCleanup;
#endif

EVT_WDF_DEVICE_FILE_CREATE UsbSamp_EvtDeviceFileCreate;

EVT_WDF_IO_QUEUE_IO_READ UsbSamp_EvtIoRead;
EVT_WDF_IO_QUEUE_IO_WRITE UsbSamp_EvtIoWrite;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL UsbSamp_EvtIoDeviceControl;

EVT_WDF_REQUEST_COMPLETION_ROUTINE UsbSamp_EvtReadWriteCompletion;
EVT_WDF_REQUEST_COMPLETION_ROUTINE UsbSamp_EvtIsoRequestCompletionRoutine;

EVT_WDF_IO_QUEUE_IO_STOP UsbSamp_EvtIoStop;

EVT_WDF_IO_QUEUE_STATE   UsbSamp_EvtIoQueueReadyNotification;

EVT_WDF_WORKITEM UsbSamp_EvtReadWriteWorkItem;

WDFUSBPIPE
GetPipeFromName(
    _In_ PDEVICE_CONTEXT DeviceContext,
    _In_ PUNICODE_STRING FileName
    );

VOID
ReadWriteIsochEndPoints(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ ULONG Length,
    _In_ WDF_REQUEST_TYPE RequestType
    );

VOID
ReadWriteBulkEndPoints(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ ULONG Length,
    _In_ WDF_REQUEST_TYPE RequestType
    );

NTSTATUS
ResetPipe(
    _In_ WDFUSBPIPE             Pipe
    );

NTSTATUS
ResetDevice(
    _In_ WDFDEVICE Device
    );

NTSTATUS
ReadAndSelectDescriptors(
    _In_ WDFDEVICE Device
    );

NTSTATUS
ConfigureDevice(
    _In_ WDFDEVICE Device
    );

NTSTATUS
SelectInterfaces(
    _In_ WDFDEVICE Device
    );

NTSTATUS
SetPowerPolicy(
        _In_ WDFDEVICE Device
    );

NTSTATUS
AbortPipes(
    _In_ WDFDEVICE Device
    );


NTSTATUS
QueuePassiveLevelCallback(
    _In_ WDFDEVICE    Device,
    _In_ WDFUSBPIPE   Pipe
    );

VOID
PerformIsochTransfer(
    _In_ PDEVICE_CONTEXT  DeviceContext,
    _In_ WDFREQUEST       Request,
    _In_ ULONG            TotalLength
    );

VOID
DbgPrintRWContext(
    PREQUEST_CONTEXT                 rwContext
    );

NTSTATUS
ReadFdoRegistryKeyValue(
    _In_  WDFDRIVER   Driver,
    _In_ LPWSTR       Name,
    _Out_ PULONG      Value
    );

NTSTATUS
InitializePipeContextForHighSpeedDevice(
    _In_ WDFUSBPIPE Pipe
    );

NTSTATUS
InitializePipeContextForFullSpeedDevice(
    _In_ WDFUSBPIPE Pipe
    );

NTSTATUS
RetrieveDeviceInformation(
    _In_ WDFDEVICE Device
    );


USBD_STATUS
UsbSamp_ValidateConfigurationDescriptor(  
    _In_reads_bytes_(BufferLength) PUSB_CONFIGURATION_DESCRIPTOR ConfigDesc,
    _In_ ULONG BufferLength,
    _Inout_ PUCHAR *Offset
    );


NTSTATUS
GetStackCapability(
    _In_  WDFDEVICE   Device,
    _In_  const GUID* CapabilityType,
    _In_ ULONG       OutputBufferLength,
    _When_(OutputBufferLength == 0, _Pre_null_)
    _When_(OutputBufferLength != 0, _Out_writes_bytes_(OutputBufferLength))
        PUCHAR      OutputBuffer
    );

#if (NTDDI_VERSION >= NTDDI_WIN8)
NTSTATUS
InitializePipeContextForSuperSpeedDevice(
    _In_ PDEVICE_CONTEXT    DeviceContext,
    _In_ WDFUSBPIPE         Pipe
    );

NTSTATUS
InitializePipeContextForSuperSpeedIsochPipe(
    _In_ PDEVICE_CONTEXT    DeviceContext,
    _In_ UCHAR              InterfaceNumber,
    _In_ WDFUSBPIPE         Pipe
    );

PUSB_ENDPOINT_DESCRIPTOR
GetEndpointDescriptorForEndpointAddress(
    _In_ PDEVICE_CONTEXT  DeviceContext,
    _In_ UCHAR            InterfaceNumber,
    _In_ UCHAR            EndpointAddress,
    _Out_ PUSB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR *ppEndpointCompanionDescriptor
    );

NTSTATUS
InitializePipeContextForSuperSpeedBulkPipe(
    _In_ PDEVICE_CONTEXT    DeviceContext,
    _In_ UCHAR              InterfaceNumber,
    _In_ WDFUSBPIPE         Pipe
    );

USBD_PIPE_HANDLE
GetStreamPipeHandleFromBulkPipe(
    _In_ WDFUSBPIPE Pipe
    );

VOID
ConfigureStreamPipeHandleForRequest(
    _In_ WDFREQUEST       Request,
    _In_ WDFUSBPIPE       Pipe
    );

#endif

ULONG
GetMaxTransferSize(
    _In_ WDFUSBPIPE        Pipe, 
    _In_ PDEVICE_CONTEXT   DeviceContext
);


#endif

