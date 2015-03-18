/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Mars.h

Abstract:

    Mars.h defines the data types used in the different stages of the function
    driver.

Environment:

    Kernel mode

--*/


#if !defined(_MARS_H_)
#define _MARS_H_

#include <ntddk.h>
#include <wdf.h>
#include <initguid.h> // required for GUID definitions
//
// Disables few warnings so that we can build our driver with MSC W4 level.
// Disable warning C4057; X differs in indirection to slightly different base types from Y
// Disable warning C4100: unreferenced formal parameter
//
#pragma warning(disable:4100 4057)

#include <ntddsd.h>
#include <ntddmars.h>

#define MARS_POOL_TAG (ULONG) 'sraM'


//
// The FDO_DATA structure describes the Mars sample function driver's device
// extension. The device extension is where all per-device-instance information
// is kept.
//
typedef struct _FDO_DATA {

    WDFDEVICE               WdfDevice;

    WDFQUEUE                IoctlQueue;
    //
    // Context for SD BUS api
    //
    SDBUS_INTERFACE_STANDARD BusInterface;
    //
    // Driver version
    //
    USHORT DriverVersion;
    //
    // Function number on SD card
    //
    UCHAR FunctionNumber;

    //
    // Target function for I/O transactions (not necessarily our function#)
    //
    UCHAR FunctionFocus;

    //
    // Send data transactions in byte mode (0) or block mode (1)
    //
    UCHAR BlockMode;
}  FDO_DATA, *PFDO_DATA;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FDO_DATA, MarsFdoGetData)


//
// Declare the function prototypes for all of the function driver's routines in all
// the stages of the function driver.
//

DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_DEVICE_ADD MarsEvtDeviceAdd;
/*
NTSTATUS
MarsEvtDeviceAdd(
    IN WDFDRIVER Driver,
    IN PWDFDEVICE_INIT DeviceInit
    );
*/

EVT_WDF_DEVICE_PREPARE_HARDWARE MarsEvtDevicePrepareHardware;
/*
NTSTATUS
MarsEvtDevicePrepareHardware(
    WDFDEVICE Device,
    WDFCMRESLIST Resources,
    WDFCMRESLIST ResourcesTranslated
    );
*/

EVT_WDF_DEVICE_RELEASE_HARDWARE MarsEvtDeviceReleaseHardware;
/*
NTSTATUS
MarsEvtDeviceReleaseHardware(
    IN  WDFDEVICE Device,
    IN  WDFCMRESLIST ResourcesTranslated
    );
*/


//
// Io events callbacks.
//

EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL MarsEvtIoDeviceControl;
/*
VOID
MarsEvtIoDeviceControl(
    IN WDFQUEUE Queue,
    IN WDFREQUEST Request,
    IN size_t OutputBufferLength,
    IN size_t InputBufferLength,
    IN ULONG IoControlCode
    );
*/

EVT_WDF_IO_QUEUE_IO_READ MarsEvtIoRead;
/*
VOID
MarsEvtIoRead (
    WDFQUEUE      Queue,
    WDFREQUEST    Request,
    size_t         Length
    );
*/

EVT_WDF_IO_QUEUE_IO_WRITE MarsEvtIoWrite;
/*
VOID
MarsEvtIoWrite (
    WDFQUEUE      Queue,
    WDFREQUEST    Request,
    size_t         Length
    );
*/

SDBUS_CALLBACK_ROUTINE MarsEventCallback;
/*
VOID
MarsEventCallback(
    IN PVOID Context,
    IN ULONG InterruptType
    );
*/

NTSTATUS
SdioReadWriteBuffer(
    IN WDFDEVICE Device,
    IN ULONG Function,
    IN PMDL Mdl,
    IN ULONG Address,
    IN ULONG Length,
    IN BOOLEAN WriteToDevice,
    OUT PULONG BytesRead
    );

NTSTATUS
SdioReadWriteByte(
    IN WDFDEVICE Device,
    IN ULONG Function,
    IN PUCHAR Data,
    IN ULONG Address,
    IN BOOLEAN WriteToDevice
    );

NTSTATUS
SdioGetProperty(
    IN WDFDEVICE Device,
    IN SDBUS_PROPERTY Property,
    IN PVOID Buffer,
    IN ULONG Length
    );

NTSTATUS
SdioSetProperty(
    IN WDFDEVICE Device,
    IN SDBUS_PROPERTY Property,
    IN PVOID Buffer,
    IN ULONG Length
    );
#endif  // _MARS_H_


