
#pragma once

#include <KMacros.h>

class KCriticalRegion
{
public:

    PAGED KCriticalRegion() : m_Entered(false) { }

    PAGED ~KCriticalRegion() { if (m_Entered) Leave(); }

    KCriticalRegion(KCriticalRegion &) = delete;
    KCriticalRegion &operator=(KCriticalRegion &) = delete;

    PAGED void Enter()
    {
        ASSERT(m_Entered == false);
        UnbalancedEnter();
        m_Entered = true;
    }

    PAGED void Leave()
    {
        ASSERT(m_Entered == true);
        m_Entered = false;
        UnbalancedLeave();
    }

    static PAGED void UnbalancedEnter()
    {
#if _KERNEL_MODE
        KeEnterCriticalRegion();
#endif
    }

    static PAGED void UnbalancedLeave()
    {
#if _KERNEL_MODE
        KeLeaveCriticalRegion();
#endif
    }

private:

    bool m_Entered;
};

struct KDefaultRegion
{
    void Enter() { }
    void Leave() { }
};

struct KIrqlRegion
{
    KIrqlRegion() { }
    ~KIrqlRegion() { }

    void Enter() { }
    void Leave() { }

    KIRQL m_OldIrql;
};


