/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    debug.h

Abstract:

    This module contains all debug related prototypes for MS FILTER

Revision History:


Notes:

--*/

// disable warnings


#ifndef _FILTDEBUG__H
#define _FILTDEBUG__H

//
// Message verbosity: lower values indicate higher urgency
//
#define DL_EXTRA_LOUD       20
#define DL_VERY_LOUD        10
#define DL_LOUD             8
#define DL_INFO             6
#define DL_TRACE            5
#define DL_WARN             4
#define DL_ERROR            2
#define DL_FATAL            0

#if DBG_SPIN_LOCK

typedef struct _FILTER_LOCK
{
    ULONG                   Signature;
    ULONG                   IsAcquired;
    ULONG                   TouchedByFileNumber;
    ULONG                   TouchedInLineNumber;
    NDIS_SPIN_LOCK          NdisLock;
} FILTER_LOCK, *PFILTER_LOCK;

#define FILT_LOCK_SIG    'kcoL'

extern NDIS_SPIN_LOCK       filterDbgLogLock;

extern
VOID
filterAllocateSpinLock(
    IN  PFILTER_LOCK        pLock,
    IN  ULONG               FileNumber,
    IN  ULONG               LineNumber
);

extern
VOID
filterFreeSpinLock(
    IN  PFILTER_LOCK        pLock

);


extern
VOID
filterAcquireSpinLock(
    IN  PFILTER_LOCK        pLock,
    IN  ULONG               FileNumber,
    IN  ULONG               LineNumber,
    IN  BOOLEAN             DispatchLevel
);

extern
VOID
filterReleaseSpinLock(
    IN  PFILTER_LOCK        pLock,
    IN  ULONG               FileNumber,
    IN  ULONG               LineNumber,
    IN  BOOLEAN             DispatchLevel
);


#else

typedef NDIS_SPIN_LOCK      FILTER_LOCK;
typedef PNDIS_SPIN_LOCK     PFILTER_LOCK;

#endif    // DBG_SPIN_LOCK

#if DBG

extern INT                filterDebugLevel;


#define DEBUGP(lev, ...)                                                \
        {                                                               \
            if ((lev) <= filterDebugLevel)                              \
            {                                                           \
                DbgPrint("NDISLWF: "); DbgPrint(__VA_ARGS__);           \
            }                                                           \
        }

#define DEBUGPDUMP(lev, pBuf, Len)                                      \
        {                                                               \
            if ((lev) <= filterDebugLevel)                              \
            {                                                           \
                DbgPrintHexDump((PUCHAR)(pBuf), (ULONG)(Len));          \
            }                                                           \
        }

#define FILTER_ASSERT(exp)                                              \
        {                                                               \
            if (!(exp))                                                 \
            {                                                           \
                DbgPrint("Filter: assert " #exp " failed in"            \
                    " file %s, line %d\n", __FILE__, __LINE__);         \
                DbgBreakPoint();                                        \
            }                                                           \
        }



//
// Memory Allocation/Freeing Audit:
//

//
// The FILTER_ALLOCATION structure stores all info about one allocation
//
typedef struct _FILTERD_ALLOCATION {

        ULONG                       Signature;
        struct _FILTERD_ALLOCATION   *Next;
        struct _FILTERD_ALLOCATION   *Prev;
        ULONG                       FileNumber;
        ULONG                       LineNumber;
        ULONG                       Size;
        NDIS_HANDLE                 OwnerHandle;
        union
        {
            ULONGLONG               Alignment;
            UCHAR                   UserData;
        };

} FILTERD_ALLOCATION, *PFILTERD_ALLOCATION;

#define FILTERD_MEMORY_SIGNATURE    (ULONG)'TFSM'

extern
PVOID
filterAuditAllocMem (
    NDIS_HANDLE  NdisHandle,
    ULONG        Size,
    ULONG        FileNumber,
    ULONG        LineNumber
);

extern
VOID
filterAuditFreeMem(
    PVOID        Pointer
);

extern
VOID
filterAuditShutdown(
    VOID
);

extern
VOID
DbgPrintHexDump(
    PUCHAR        pBuffer,
    ULONG        Length
);

#else

//
// No debug
//
#define DEBUGP(lev, ...)
#define DEBUGPDUMP(lev, pBuf, Len)

#define FILTER_ASSERT(exp)

#endif    // DBG


#endif // _FILTDEBUG__H
