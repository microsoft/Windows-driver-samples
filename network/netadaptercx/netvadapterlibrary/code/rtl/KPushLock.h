
#pragma once

#include <KMacros.h>
#include <KCriticalRegion.h>

typedef struct _KTHREAD *PKTHREAD;

class KPushLockBase
{
public:

    KPushLockBase() = default;
    KPushLockBase(KPushLockBase &) = delete;
    KPushLockBase & operator=(KPushLockBase &) = delete;

    PAGED void AcquireShared();

    PAGED void ReleaseShared();

    PAGED void AcquireExclusive();

    PAGED void ReleaseExclusive();

    PAGED void AssertLockHeld();

    PAGED void AssertLockNotHeld();

protected:

    PAGED void InitializeInner();

private:

#ifdef _KERNEL_MODE
    EX_PUSH_LOCK m_Lock;
#else
    SRWLOCK m_Lock;
#endif

#if DBG
    PKTHREAD m_ExclusiveOwner;
#endif
};

class KPushLock : public KPushLockBase
{
public:

    PAGED KPushLock() noexcept;

    PAGED ~KPushLock();
};

class KPushLockManualConstruct : public KPushLockBase
{
public:

    PAGED void Initialize();
};

