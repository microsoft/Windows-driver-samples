
#pragma once

#include <KMacros.h>

#if _KERNEL_MODE
typedef wistd::integral_constant<EVENT_TYPE, SynchronizationEvent> auto_reset_event_t;
#else
typedef wistd::integral_constant<bool, false> auto_reset_event_t;
#endif

#if _KERNEL_MODE
typedef wistd::integral_constant<EVENT_TYPE, NotificationEvent> manual_reset_event_t;
#else
typedef wistd::integral_constant<bool, true> manual_reset_event_t;
#endif

#define WIN_VERIFY      NT_VERIFY
#define WIN_ASSERT      NT_ASSERT

template<typename TEventType>
class KWaitEventBase
{
public:

    KWaitEventBase() = default;

#ifdef _KERNEL_MODE
    NONPAGED ~KWaitEventBase() = default;
#else
    NONPAGED ~KWaitEventBase()
    {
        WIN_ASSERT(m_event == nullptr);
    }
#endif

    KWaitEventBase(KWaitEventBase &) = delete;
    KWaitEventBase(KWaitEventBase &&) = delete;
    KWaitEventBase & operator=(KWaitEventBase &) = delete;
    KWaitEventBase & operator=(KWaitEventBase &&) = delete;

    _IRQL_requires_max_(DISPATCH_LEVEL) void Set()
    {
#if _KERNEL_MODE
        KeSetEvent(&m_event, 0, false);
#else
        WIN_VERIFY(SetEvent(m_event));
#endif
    }

    _IRQL_requires_max_(DISPATCH_LEVEL) void Clear()
    {
#if _KERNEL_MODE
        KeClearEvent(&m_event);
#else
        WIN_VERIFY(ResetEvent(m_event));
#endif
    }

    PAGED void Wait()
    {
#if _KERNEL_MODE
        // Not used at runtime, but might be useful during debugging
        volatile LARGE_INTEGER SystemTime;
        KeQuerySystemTime(const_cast<LARGE_INTEGER*>(&SystemTime));

        NTSTATUS NtStatus = KeWaitForSingleObject(
                &m_event, Executive, KernelMode, FALSE, nullptr);
        NT_VERIFY(NtStatus == STATUS_SUCCESS);
#else
        ULONG r = WaitForSingleObject(m_event, INFINITE);
        WIN_VERIFY(r == NO_ERROR);
#endif
    }

    PAGED bool Test()
    {
#if _KERNEL_MODE
        return !!KeReadStateEvent(&m_event);
#else
        ULONG r = WaitForSingleObject(m_event, 0);
        WIN_VERIFY(r == WAIT_TIMEOUT || r == WAIT_OBJECT_0);
        return (r == WAIT_OBJECT_0);
#endif
    }

    NONPAGED bool TestNP()
    {
#if _KERNEL_MODE
        return !!KeReadStateEvent(&m_event);
#else
        ULONG r = WaitForSingleObject(m_event, 0);
        WIN_VERIFY(r == WAIT_TIMEOUT || r == WAIT_OBJECT_0);
        return (r == WAIT_OBJECT_0);
#endif
    }

protected:

    PAGED void InitializeBase()
    {
#if _KERNEL_MODE
        KeInitializeEvent(&m_event, TEventType(), FALSE);
#else
        m_event = CreateEventW(nullptr, TEventType(), false, nullptr);
        WIN_VERIFY(m_event);
#endif
    }

    NONPAGED void CleanupBase()
    {
#ifndef _KERNEL_MODE
        CloseHandle(m_event);
        m_event = nullptr;
#endif
    }

private:

#if _KERNEL_MODE
    KEVENT m_event;
#else
    HANDLE m_event;
#endif

};

class KWaitEvent : public KWaitEventBase<manual_reset_event_t>
{
public:

    PAGED KWaitEvent() noexcept
    {
        InitializeBase();
    }

    NONPAGED ~KWaitEvent()
    {
        CleanupBase();
    }
};

class KWaitEventManualConstruct : public KWaitEventBase<manual_reset_event_t>
{
public:

    PAGED void Initialize()
    {
        InitializeBase();
    }

    PAGED void Cleanup()
    {
        CleanupBase();
    }
};


class KAutoEvent : public KWaitEventBase<auto_reset_event_t>
{
public:

    PAGED KAutoEvent() noexcept
    {
        InitializeBase();
    }

    NONPAGED ~KAutoEvent()
    {
        CleanupBase();
    }
};
