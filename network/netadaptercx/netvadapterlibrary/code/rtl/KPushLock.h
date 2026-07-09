// Copyright (C) Microsoft Corporation. All rights reserved.

#pragma once

#include <KMacros.h>
#include <KCriticalRegion.h>

typedef struct _KTHREAD *PKTHREAD;
// Copyright (C) Microsoft Corporation. All rights reserved.

class KPushLockBase
{
public:

    KPushLockBase() = default;
    KPushLockBase(KPushLockBase &) = delete;
    KPushLockBase & operator=(KPushLockBase &) = delete;

    PAGED
        void
        KPushLockBase::AcquireShared()
    {
#ifdef _KERNEL_MODE
        ExAcquirePushLockShared(&m_Lock);
#else
        AcquireSRWLockShared(&m_Lock);
#endif
    }

    PAGED
        void
        KPushLockBase::ReleaseShared()
    {
#ifdef _KERNEL_MODE
        ExReleasePushLockShared(&m_Lock);
#else
        ReleaseSRWLockShared(&m_Lock);
#endif
    }

    PAGED
        void
        KPushLockBase::AcquireExclusive()
    {
#ifdef _KERNEL_MODE
        ExAcquirePushLockExclusive(&m_Lock);
#if DBG
        m_ExclusiveOwner = KeGetCurrentThread();
#endif
#else
        AcquireSRWLockExclusive(&m_Lock);
#endif

    }

    PAGED
        void
        KPushLockBase::ReleaseExclusive()
    {
#if DBG
        m_ExclusiveOwner = nullptr;
#endif
#ifdef _KERNEL_MODE
        ExReleasePushLockExclusive(&m_Lock);
#else
        ReleaseSRWLockExclusive(&m_Lock);
#endif
    }

    PAGED
        void
        KPushLockBase::AssertLockHeld()
    {
#ifdef _KERNEL_MODE
        WIN_ASSERT(m_ExclusiveOwner == KeGetCurrentThread());
#endif
    }

    PAGED
        void
        KPushLockBase::AssertLockNotHeld()
    {
#if DBG && defined(_KERNEL_MODE)
        WIN_ASSERT(m_ExclusiveOwner != KeGetCurrentThread());
#endif
    }

protected:

    PAGED
        void
        KPushLockBase::InitializeInner()
    {
#ifdef _KERNEL_MODE
        ExInitializePushLock(&m_Lock);
#else
        InitializeSRWLock(&m_Lock);
#endif
#if DBG
        m_ExclusiveOwner = nullptr;
#endif
    }

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

    PAGED
        KPushLock::KPushLock() noexcept
    {
        InitializeInner();
    }



    PAGED
        KPushLock::~KPushLock()
    {
        AssertLockNotHeld();
    }
};

class KPushLockManualConstruct : public KPushLockBase
{
public:

    PAGED
        void
        KPushLockManualConstruct::Initialize()
    {
        InitializeInner();
    }
};

