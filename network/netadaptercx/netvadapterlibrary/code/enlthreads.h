// Copyright (C) Microsoft Corporation. All rights reserved.
#pragma once

#include <wil/resource.h>
// #include <KMacros.h>

#ifdef _KERNEL_MODE
    using ENL_START_ROUTINE = KSTART_ROUTINE;
    using ENL_THREAD_ROUTINE_RETURN = VOID;
    using ENL_THREAD = PKTHREAD;
    // static const EC_THREAD EC_THREAD_INVALID = nullptr;
    using ENL_THREAD_PRIORITY = LONG;
    // #define EC_INVALID_THREAD_PRIORITY MAXLONG

    using unique_zw_handle = wil::unique_any<HANDLE, decltype(&::ZwClose), &::ZwClose>;
    using unique_pkthread = wil::unique_any<PKTHREAD, decltype(&ObfDereferenceObject), &ObfDereferenceObject>;
    using unique_pkevent = wil::unique_any<PKEVENT, decltype(&ObfDereferenceObject), &ObfDereferenceObject>;
    using unique_completionport = wil::unique_any<void *, decltype(&ObfDereferenceObject), &ObfDereferenceObject>;

    struct unique_thread
    {
        unique_zw_handle
            NtHandle;

        unique_pkthread
            ObHandle;

        operator bool(
            void
        ) const;

        void reset(
        );
    };
#else

#ifndef NOTHING
#define NOTHING
#endif

#ifndef AFFINITY_MASK
#define AFFINITY_MASK(n) ((KAFFINITY)1 << (n))
#endif

typedef struct _NDTBUS_POWER_INTERFACE_STANDARD
{
} NDTBUS_POWER_INTERFACE_STANDARD;

_IRQL_requires_same_
typedef DWORD (WINAPI ENL_START_ROUTINE)(
    LPVOID lpThreadParameter
);
using ENL_THREAD_ROUTINE_RETURN = DWORD;
using ENL_THREAD = DWORD;
// static const EC_THREAD EC_THREAD_INVALID = 0;
using ENL_THREAD_PRIORITY = int;
// #define EC_INVALID_THREAD_PRIORITY THREAD_PRIORITY_ERROR_RETURN
using unique_thread = wil::unique_handle;

#define KeGetProcessorIndexFromNumber(_processor) (_processor)->Number
#define KeMemoryBarrier() MemoryBarrier()

inline
NTSTATUS
KeGetProcessorNumberFromIndex (
    _In_ ULONG ProcIndex,
    _Out_ PPROCESSOR_NUMBER ProcNumber
    )
{
    ProcNumber->Number = static_cast<UCHAR>(ProcIndex);
    ProcNumber->Group = 0;
    ProcNumber->Reserved = 0; 

    return STATUS_SUCCESS;
}

inline
VOID
KeQueryNodeActiveAffinity (
    __in USHORT NodeNumber,
    __out_opt PGROUP_AFFINITY Affinity,
    __out_opt PUSHORT Count
)
{
    UNREFERENCED_PARAMETER(NodeNumber);
    UNREFERENCED_PARAMETER(Count);
    
    GetThreadGroupAffinity(GetCurrentThread(), Affinity);
}
#endif

ENL_THREAD
EnlGetCurrentThread(
    void
);

NTSTATUS
EnlThreadCreate(
    _In_ ENL_START_ROUTINE StartRoutine,
    _In_opt_ void * Context,
    _Out_ unique_thread & Thread
);

void
EnlThreadSetPriority(
    _In_ unique_thread & Thread,
    _In_ ENL_THREAD_PRIORITY Priority
);

void
EnlThreadSetAffinity(
    _In_ PGROUP_AFFINITY GroupAffinity,
    _Out_opt_ PGROUP_AFFINITY PreviousAffinity
);

void
EnlThreadWaitForTermination(
    _In_ unique_thread & Thread
);
