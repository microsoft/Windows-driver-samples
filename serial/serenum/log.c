/*++

Copyright (c) 1995,1996 Microsoft Corporation
:ts=4

Module Name:

    log.c

Abstract:

    Debug log Code for serial.

Environment:

    kernel mode only

Notes:

Revision History:

    10-08-95 : created

--*/

#include "pch.h"
#include <stdio.h>

#if DBG

KSPIN_LOCK LogSpinLock;

struct SERENUM_LOG_ENTRY {
    ULONG        le_sig;          // Identifying string
    ULONG_PTR    le_info1;        // entry specific info
    ULONG_PTR    le_info2;        // entry specific info
    ULONG_PTR    le_info3;        // entry specific info
}; // SERENUM_LOG_ENTRY


struct SERENUM_LOG_ENTRY *SerenumLStart = 0;    // No log yet
struct SERENUM_LOG_ENTRY *SerenumLPtr;
struct SERENUM_LOG_ENTRY *SerenumLEnd;

ULONG LogMask = 0xffffffff;

VOID
SerenumDebugLogEntry(IN ULONG Mask, IN ULONG Sig, IN ULONG_PTR Info1,
                      IN ULONG_PTR Info2, IN ULONG_PTR Info3)
/*++

Routine Description:

    Adds an Entry to serial log.

Arguments:

Return Value:

    None.

--*/
{
    KIRQL irql;

typedef union _SIG {
    struct {
        UCHAR Byte0;
        UCHAR Byte1;
        UCHAR Byte2;
        UCHAR Byte3;
    } b;
    ULONG l;
} SIG, *PSIG;

    SIG sig, rsig;


    if (SerenumLStart == 0) {
        return;
    }

    if ((Mask & LogMask) == 0) {
        return;
    }

    irql = KeGetCurrentIrql();

    if (irql < DISPATCH_LEVEL) {
        KeAcquireSpinLock(&LogSpinLock, &irql);
    } else {
        KeAcquireSpinLockAtDpcLevel(&LogSpinLock);
    }

    if (SerenumLPtr > SerenumLStart) {
        SerenumLPtr -= 1;    // Decrement to next entry
    } else {
        SerenumLPtr = SerenumLEnd;
    }

    sig.l = Sig;
    rsig.b.Byte0 = sig.b.Byte3;
    rsig.b.Byte1 = sig.b.Byte2;
    rsig.b.Byte2 = sig.b.Byte1;
    rsig.b.Byte3 = sig.b.Byte0;

    SerenumLPtr->le_sig = rsig.l;
    SerenumLPtr->le_info1 = Info1;
    SerenumLPtr->le_info2 = Info2;
    SerenumLPtr->le_info3 = Info3;

    ASSERT(SerenumLPtr >= SerenumLStart);

    if (irql < DISPATCH_LEVEL) {
        KeReleaseSpinLock(&LogSpinLock, irql);
    } else {
        KeReleaseSpinLockFromDpcLevel(&LogSpinLock);
    }

    return;
}


VOID
SerenumLogInit()
/*++

Routine Description:

    Init the debug log - remember interesting information in a circular buffer

Arguments:

Return Value:

    None.

--*/
{
#ifdef MAX_DEBUG
    ULONG logSize = 4096*6;
#else
    ULONG logSize = 4096*3;
#endif


    KeInitializeSpinLock(&LogSpinLock);

    SerenumLStart = ExAllocatePoolZero(NonPagedPoolNx, logSize, 'mneS');

    if (SerenumLStart) {
        SerenumLPtr = SerenumLStart;

        // Point the end (and first entry) 1 entry from the end of the segment
        SerenumLEnd = SerenumLStart + (logSize
                                       / sizeof(struct SERENUM_LOG_ENTRY))
            - 1;
    }

    return;
}

VOID
SerenumLogFree(
    )
/*++

Routine Description:

Arguments:

Return Value:

    None.

--*/
{
    if (SerenumLStart) {
        ExFreePoolWithTag(SerenumLStart,SERENUM_POOL_TAG);
    }

    return;
}

#endif // DBG
