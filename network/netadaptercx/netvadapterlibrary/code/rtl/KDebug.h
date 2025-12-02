
#pragma once

#include <KMacros.h>

template<ULONG SIGNATURE>
struct KRTL_CLASS NdisDebugBlock
{
#if DBG
    PAGED ~NdisDebugBlock()
    {
        ASSERT_VALID();
        Signature |= 0x80;
    }
#endif

    _IRQL_requires_max_(HIGH_LEVEL)
    NONPAGEDX bool ASSERT_VALID() const
    {
#if DBG
        return NT_VERIFY(Signature == SIGNATURE);
#else
        return true;
#endif
    }

private:
#if DBG
    ULONG Signature = SIGNATURE;
#endif
};
