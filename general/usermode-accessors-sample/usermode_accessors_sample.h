/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    usermode_accessors_sample.h

Abstract:

    Header for the usermode_accessors sample driver.
    Demonstrates safe kernel-to-user and user-to-kernel memory access
    using the usermode_accessors.h API family.

Environment:

    Kernel mode

--*/

#pragma once

#include <ntddk.h>
#include <wdf.h>
#include <usermode_accessors.h>

//
// Pool tag for allocations: 'UmAs'
//
#define UMA_POOL_TAG 'sAmU'

//
// Device context structure
//
typedef struct _DEVICE_CONTEXT {
    WDFDEVICE Device;
    ULONG     OperationCount;   // Tracks how many IOCTL operations performed
} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, DeviceGetContext)

//
// IOCTL definitions for exercising usermode_accessors functions.
//
// FILE_DEVICE_UNKNOWN is used for sample/test drivers.
//
#define FILE_DEVICE_UMA_SAMPLE  FILE_DEVICE_UNKNOWN

//
// IOCTL_UMA_READ_VALUES:
//   Input:  User-mode buffer containing typed values
//   Output: Kernel-read copies of those values (demonstrates ReadXxxFromUser)
//
#define IOCTL_UMA_READ_VALUES \
    CTL_CODE(FILE_DEVICE_UMA_SAMPLE, 0x800, METHOD_NEITHER, FILE_ANY_ACCESS)

//
// IOCTL_UMA_WRITE_VALUES:
//   Input:  Kernel-side values to write
//   Output: User-mode buffer filled with values (demonstrates WriteXxxToUser)
//
#define IOCTL_UMA_WRITE_VALUES \
    CTL_CODE(FILE_DEVICE_UMA_SAMPLE, 0x801, METHOD_NEITHER, FILE_ANY_ACCESS)

//
// IOCTL_UMA_COPY_BUFFER:
//   Input:  User buffer with source data
//   Output: User buffer for destination (demonstrates CopyFromUser/CopyToUser)
//
#define IOCTL_UMA_COPY_BUFFER \
    CTL_CODE(FILE_DEVICE_UMA_SAMPLE, 0x802, METHOD_NEITHER, FILE_ANY_ACCESS)

//
// IOCTL_UMA_FILL_BUFFER:
//   Input:  UCHAR fill value + SIZE_T length
//   Output: User buffer filled with the value (demonstrates FillUserMemory)
//
#define IOCTL_UMA_FILL_BUFFER \
    CTL_CODE(FILE_DEVICE_UMA_SAMPLE, 0x803, METHOD_NEITHER, FILE_ANY_ACCESS)

//
// IOCTL_UMA_INTERLOCKED_OPS:
//   Input:  User-mode LONG and LONG64 values
//   Output: Results of interlocked operations (demonstrates InterlockedXxxToUser)
//
#define IOCTL_UMA_INTERLOCKED_OPS \
    CTL_CODE(FILE_DEVICE_UMA_SAMPLE, 0x804, METHOD_NEITHER, FILE_ANY_ACCESS)

//
// IOCTL_UMA_STRING_LENGTH:
//   Input:  User-mode null-terminated string
//   Output: Length of the string (demonstrates StringLengthFromUser)
//
#define IOCTL_UMA_STRING_LENGTH \
    CTL_CODE(FILE_DEVICE_UMA_SAMPLE, 0x805, METHOD_NEITHER, FILE_ANY_ACCESS)

//
// IOCTL_UMA_STRUCT_ACCESS:
//   Input:  User-mode struct
//   Output: Modified struct (demonstrates ReadStructFromUser/WriteStructToUser)
//
#define IOCTL_UMA_STRUCT_ACCESS \
    CTL_CODE(FILE_DEVICE_UMA_SAMPLE, 0x806, METHOD_NEITHER, FILE_ANY_ACCESS)

//
// IOCTL_UMA_MODE_OPERATIONS:
//   Input:  User-mode buffer + processor mode flag
//   Output: Results (demonstrates XxxFromMode/XxxToMode variants)
//
#define IOCTL_UMA_MODE_OPERATIONS \
    CTL_CODE(FILE_DEVICE_UMA_SAMPLE, 0x807, METHOD_NEITHER, FILE_ANY_ACCESS)

//
// Shared structures for IOCTLs
//
#pragma pack(push, 1)

typedef struct _UMA_READ_VALUES_INPUT {
    UCHAR   UCharValue;
    USHORT  UShortValue;
    ULONG   ULongValue;
    ULONG64 ULong64Value;
    BOOLEAN BoolValue;
    HANDLE  HandleValue;
} UMA_READ_VALUES_INPUT, *PUMA_READ_VALUES_INPUT;

typedef struct _UMA_READ_VALUES_OUTPUT {
    UCHAR   UCharValue;
    USHORT  UShortValue;
    ULONG   ULongValue;
    ULONG64 ULong64Value;
    BOOLEAN BoolValue;
    NTSTATUS StatusResult;
} UMA_READ_VALUES_OUTPUT, *PUMA_READ_VALUES_OUTPUT;

typedef struct _UMA_WRITE_VALUES_INPUT {
    UCHAR   UCharValue;
    USHORT  UShortValue;
    ULONG   ULongValue;
    ULONG64 ULong64Value;
    BOOLEAN BoolValue;
} UMA_WRITE_VALUES_INPUT, *PUMA_WRITE_VALUES_INPUT;

typedef struct _UMA_FILL_INPUT {
    UCHAR  FillValue;
    SIZE_T Length;
} UMA_FILL_INPUT, *PUMA_FILL_INPUT;

typedef struct _UMA_INTERLOCKED_INPUT {
    LONG   Value32;
    LONG   Operand32;
    LONG64 Value64;
    LONG64 Operand64;
} UMA_INTERLOCKED_INPUT, *PUMA_INTERLOCKED_INPUT;

typedef struct _UMA_INTERLOCKED_OUTPUT {
    LONG   AndResult32;
    LONG   OrResult32;
    LONG   CmpXchgResult32;
    LONG64 AndResult64;
    LONG64 OrResult64;
    LONG64 CmpXchgResult64;
} UMA_INTERLOCKED_OUTPUT, *PUMA_INTERLOCKED_OUTPUT;

typedef struct _UMA_STRING_LENGTH_OUTPUT {
    SIZE_T AnsiLength;
    SIZE_T WideLength;
} UMA_STRING_LENGTH_OUTPUT, *PUMA_STRING_LENGTH_OUTPUT;

typedef struct _UMA_SAMPLE_STRUCT {
    ULONG   Id;
    ULONG64 Timestamp;
    WCHAR   Name[32];
    BOOLEAN Active;
} UMA_SAMPLE_STRUCT, *PUMA_SAMPLE_STRUCT;

typedef struct _UMA_MODE_INPUT {
    KPROCESSOR_MODE Mode;
    ULONG           Value;
} UMA_MODE_INPUT, *PUMA_MODE_INPUT;

#pragma pack(pop)

//
// Function prototypes
//
DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD EvtDeviceAdd;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL EvtIoDeviceControl;
