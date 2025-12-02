// Copyright (C) Microsoft Corporation. All rights reserved.
#include "pch.hpp"
//#include <ntassert.h>
#include "enlthreads.h"

#ifdef _KERNEL_MODE
unique_thread::operator bool(
    void
) const
{
    return !!NtHandle;
}

void unique_thread::reset(
)
{
    NtHandle.reset();
    ObHandle.reset();
}
#endif

ENL_THREAD
EnlGetCurrentThread(
    void
)
{
#ifdef _KERNEL_MODE
    return KeGetCurrentThread();
#else
    return GetCurrentThreadId();
#endif
}

_Use_decl_annotations_
NTSTATUS
EnlThreadCreate(
    ENL_START_ROUTINE StartRoutine,
    void * Context,
    unique_thread & Thread
)
{
#ifdef _KERNEL_MODE

    unique_zw_handle ntHandle;
    unique_pkthread obHandle;

    auto const ntStatus = PsCreateSystemThread(
        &ntHandle,
        THREAD_ALL_ACCESS,
        nullptr,
        nullptr,
        nullptr,
        StartRoutine,
        Context);

    if (ntStatus != STATUS_SUCCESS)
    {
        return ntStatus;
    }

    NT_FRE_ASSERT(
        NT_SUCCESS(
            ObReferenceObjectByHandle(
                ntHandle.get(),
                THREAD_ALL_ACCESS,
                nullptr,
                KernelMode,
                reinterpret_cast<void **>(&obHandle),
                nullptr)));

    Thread.NtHandle = wistd::move(ntHandle);
    Thread.ObHandle = wistd::move(obHandle);

#else
    wil::unique_handle thread{ CreateThread(nullptr, 0, StartRoutine, Context, 0, nullptr) };

    if (!thread)
    {
        return NTSTATUS_FROM_WIN32(GetLastError());
    }

    Thread = wistd::move(thread);
#endif

    return STATUS_SUCCESS;
}

void
EnlThreadSetPriority(
    unique_thread & Thread,
    ENL_THREAD_PRIORITY Priority
)
{
#ifdef _KERNEL_MODE
    // KeSetBasePriorityThread does not take the actual priority, but an increment
    // to be added to the current base priority. Calculate this value.
    auto const increment = Priority - (LOW_REALTIME_PRIORITY + LOW_PRIORITY) / 2;
    KeSetBasePriorityThread(Thread.ObHandle.get(), increment);
#else
    SetThreadPriority(Thread.get(), Priority);
#endif
}

_Use_decl_annotations_
void
EnlThreadSetAffinity(
    PGROUP_AFFINITY GroupAffinity,
    PGROUP_AFFINITY PreviousAffinity
)
{
#ifdef _KERNEL_MODE
    KeSetSystemGroupAffinityThread(GroupAffinity, PreviousAffinity);
#else
    SetThreadGroupAffinity(GetCurrentThread(), GroupAffinity, PreviousAffinity);
#endif
}

_Use_decl_annotations_
void
EnlThreadWaitForTermination(
    unique_thread & Thread
)
{
#ifdef _KERNEL_MODE
    KeWaitForSingleObject(
        Thread.ObHandle.get(),
        KWAIT_REASON::Executive,
        KernelMode,
        FALSE,
        nullptr);
#else
    WaitForSingleObject(
        Thread.get(),
        INFINITE);
#endif
}
