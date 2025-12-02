
#pragma once

#include <KCriticalRegion.h>
#include <KPushLock.h>

class KRTL_CLASS KLockHolder
{
private:

    enum { Unlocked, Shared, Exclusive } m_State;

public:

    PAGED KLockHolder(KPushLockBase &lock) : m_Lock(lock), m_State(Unlocked) { }
    PAGED ~KLockHolder()
    {
        switch (m_State)
        {
            case Shared:
                ReleaseShared();
                break;
            case Exclusive:
                ReleaseExclusive();
                break;
        }
    }

    KLockHolder(KLockHolder &) = delete;
    KLockHolder &operator=(KLockHolder &) = delete;

    _IRQL_requires_(PASSIVE_LEVEL)
    PAGED void AcquireShared()
    {
        m_Region.Enter();
        ASSERT(m_State == Unlocked);
        m_Lock.AcquireShared();
        m_State = Shared;
    }

    _IRQL_requires_(PASSIVE_LEVEL)
    PAGED void ReleaseShared()
    {
        ASSERT(m_State == Shared);
        m_Lock.ReleaseShared();
        m_State = Unlocked;
        m_Region.Leave();
    }

    _IRQL_requires_(PASSIVE_LEVEL)
    PAGED void AcquireExclusive()
    {
        m_Region.Enter();
        ASSERT(m_State == Unlocked);
        m_Lock.AcquireExclusive();
        m_State = Exclusive;
    }

    _IRQL_requires_(PASSIVE_LEVEL)
    PAGED void ReleaseExclusive()
    {
        ASSERT(m_State == Exclusive);
        m_Lock.ReleaseExclusive();
        m_State = Unlocked;
        m_Region.Leave();
    }

private:

    KPushLockBase &m_Lock;
    KCriticalRegion m_Region;
};

class KRTL_CLASS KLockThisShared : protected KLockHolder
{
public:

    PAGED KLockThisShared(KPushLockBase &lock) : KLockHolder(lock)
    {
        AcquireShared();
    }

    PAGED void Acquire()
    {
        AcquireShared();
    }

    PAGED void Release()
    {
        ReleaseShared();
    }
};

class KRTL_CLASS KLockThisExclusive : protected KLockHolder
{
public:

    PAGED KLockThisExclusive(KPushLockBase &lock) : KLockHolder(lock)
    {
        AcquireExclusive();
    }

    PAGED void Acquire()
    {
        AcquireExclusive();
    }

    PAGED void Release()
    {
        ReleaseExclusive();
    }
};


