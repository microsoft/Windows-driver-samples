/*++

Module Name:

    I2C.h

Abstract:

    This file contains the declarations for I2C functions and callbacks.

Environment:

    Kernel-mode Driver Framework

--*/

#pragma once

// Number of platform-level device resets to attempt.
#define MAX_DEVICE_RESET_ATTEMPTS 3

// Transfers required for an I2C read or write operation.
// The first transfer in the sequence writes a one-byte register address to the device.
// The second transfer reads from or writes to the selected register.
#define I2C_TRANSFER_COUNT 2

#define REGISTER_ADDR_SIZE 1

// Timeout in milliseconds for synchronous I2C reads/writes.
// The I2C specification does not specify a timeout. 300 ms was chosen arbitrarily.
#define I2C_SYNCHRONOUS_TIMEOUT 300

// Size used to initialize the I2C read and write buffers.
#define I2C_BUFFER_SIZE 50

typedef struct _DEVICE_CONTEXT DEVICE_CONTEXT, *PDEVICE_CONTEXT;

enum I2C_REQUEST_SOURCE;

typedef struct _REGISTER_ITEM
{
    UINT8 RegisterAddress;
    _Out_writes_bytes_(Length) PVOID Data;
    ULONG Length;
} REGISTER_ITEM, *PREGISTER_ITEM;

// Helper macro to generate an array item.
#define GEN_REGISTER_ITEM(RegisterAddress, Variable) \
    { (RegisterAddress), &(Variable), sizeof((Variable)) }

#ifndef _countof
#define _countof(_Array) (sizeof(_Array) / sizeof(_Array[0]))
#endif

EXTERN_C_START

NTSTATUS
I2CInitialize(
    _In_ PDEVICE_CONTEXT DeviceContext,
    _In_ WDFCMRESLIST ResourcesRaw,
    _In_ WDFCMRESLIST ResourcesTranslated
);

NTSTATUS
I2COpen(
    _In_ PDEVICE_CONTEXT DeviceContext
);

void
I2CClose(
    _In_ PDEVICE_CONTEXT DeviceContext
);

NTSTATUS
I2CWriteAsynchronously(
    _In_ PDEVICE_CONTEXT DeviceContext,
    _In_ UINT8 RegisterAddress,
    _In_reads_bytes_(Length) PVOID Data,
    _In_ ULONG Length
);

NTSTATUS
I2CReadAsynchronously(
    _In_ PDEVICE_CONTEXT DeviceContext,
    _In_ UINT8 RegisterAddress,
    _In_ ULONG Length
);

NTSTATUS
I2CReadSynchronously(
    _In_ PDEVICE_CONTEXT DeviceContext,
    _In_ I2C_REQUEST_SOURCE requestSource,
    _In_ UINT8 RegisterAddress,
    _Out_writes_bytes_(Length) PVOID Data,
    _In_ ULONG Length
);

NTSTATUS
I2CWriteSynchronously(
    _In_ PDEVICE_CONTEXT DeviceContext,
    _In_ I2C_REQUEST_SOURCE requestSource,
    _In_ UINT8 RegisterAddress,
    _In_reads_(Length) PVOID Data,
    _In_ ULONG Length
);

NTSTATUS
I2CReadSynchronouslyMultiple(
    _In_ PDEVICE_CONTEXT DeviceContext,
    _In_ I2C_REQUEST_SOURCE requestSource,
    _Inout_updates_(Count) REGISTER_ITEM* Items,
    _In_ ULONG Count
);

EVT_WDF_REQUEST_COMPLETION_ROUTINE
I2COnReadCompletion;

EVT_WDF_REQUEST_COMPLETION_ROUTINE
I2COnWriteCompletion;

void
I2CPerformDeviceReset(
    _In_ PDEVICE_CONTEXT DeviceContext
);

EXTERN_C_END