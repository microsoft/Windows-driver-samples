// Copyright (C) Microsoft Corporation. All rights reserved.

#pragma once

#if UMDF_DRIVER == 0
#include "pooltypes.h"
#endif

#ifndef _KERNEL_MODE

#if (NTDDI_VERSION >= NTDDI_WIN10_VB) && !defined(KRTL_USE_LEGACY_POOL_API)

typedef _Enum_is_bitflag_ enum _EX_POOL_PRIORITY {
    LowPoolPriority,
    LowPoolPrioritySpecialPoolOverrun = 8,
    LowPoolPrioritySpecialPoolUnderrun = 9,
    NormalPoolPriority = 16,
    NormalPoolPrioritySpecialPoolOverrun = 24,
    NormalPoolPrioritySpecialPoolUnderrun = 25,
    HighPoolPriority = 32,
    HighPoolPrioritySpecialPoolOverrun = 40,
    HighPoolPrioritySpecialPoolUnderrun = 41
} EX_POOL_PRIORITY;

typedef enum POOL_EXTENDED_PARAMETER_TYPE {
    PoolExtendedParameterInvalidType = 0,
    PoolExtendedParameterPriority,
    PoolExtendedParameterMax
} POOL_EXTENDED_PARAMETER_TYPE, *PPOOL_EXTENDED_PARAMETER_TYPE;

#define POOL_EXTENDED_PARAMETER_TYPE_BITS    8
#define POOL_EXTENDED_PARAMETER_REQUIRED_FIELD_BITS    1
#define POOL_EXTENDED_PARAMETER_RESERVED_BITS    (64 - POOL_EXTENDED_PARAMETER_TYPE_BITS - POOL_EXTENDED_PARAMETER_REQUIRED_FIELD_BITS)

#pragma warning(push)
#pragma warning(disable: 4201) // nameless struct/union
typedef struct DECLSPEC_ALIGN(8) POOL_EXTENDED_PARAMETER {
    struct {
        ULONG64 Type : POOL_EXTENDED_PARAMETER_TYPE_BITS;
        ULONG64 Optional : POOL_EXTENDED_PARAMETER_REQUIRED_FIELD_BITS;
        ULONG64 Reserved : POOL_EXTENDED_PARAMETER_RESERVED_BITS;
    } DUMMYSTRUCTNAME;

    union {
        ULONG64 Reserved2;
        PVOID Reserved3;
        EX_POOL_PRIORITY Priority;
    } DUMMYUNIONNAME;
} POOL_EXTENDED_PARAMETER, *PPOOL_EXTENDED_PARAMETER;
#pragma warning(pop)

typedef ULONG64 POOL_FLAGS;

_Check_return_
_Ret_maybenull_
_Post_writable_byte_size_(NumberOfBytes)
PVOID
ExAllocatePool2 (
    _In_ POOL_FLAGS Flags,
    _In_ SIZE_T NumberOfBytes,
    _In_ ULONG Tag
    );

_Check_return_
_Ret_maybenull_
_Post_writable_byte_size_(NumberOfBytes)
PVOID
ExAllocatePool3 (
    _In_ POOL_FLAGS Flags,
    _In_ SIZE_T NumberOfBytes,
    _In_ ULONG Tag,
    _In_reads_opt_(ExtendedParameterCount) PPOOL_EXTENDED_PARAMETER ExtendedParameters,
    _In_ ULONG ExtendedParametersCount
    );

#endif // (NTDDI_VERSION >= NTDDI_WIN10_VB) && !defined(KRTL_USE_LEGACY_POOL_API)

PVOID
ExAllocatePoolWithTag(
    POOL_TYPE PoolType,
    SIZE_T NumberOfBytes,
    ULONG Tag
    );

VOID
ExFreePoolWithTag(
    _Pre_notnull_ __drv_freesMem(Mem) PVOID P,
    _In_ ULONG Tag
    );

VOID
ExFreePool(
    _Pre_notnull_ __drv_freesMem(Mem) PVOID P
    );

#endif // _KERNEL_MODE

