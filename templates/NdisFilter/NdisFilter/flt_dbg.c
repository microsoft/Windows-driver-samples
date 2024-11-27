/*++

Module Name:

    debug.c

Abstract:

    This module contains all debug-related code.

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------

Notes:

--*/

#include  "precomp.h"

#define __FILENUMBER 'GBED'

#if DBG

INT                 filterDebugLevel = DL_WARN;

NDIS_SPIN_LOCK        filterDbgLogLock;

PFILTERD_ALLOCATION    filterdMemoryHead = (PFILTERD_ALLOCATION)NULL;
PFILTERD_ALLOCATION    filterdMemoryTail = (PFILTERD_ALLOCATION)NULL;
ULONG                filterdAllocCount = 0;    // how many allocated so far (unfreed)

NDIS_SPIN_LOCK        filterdMemoryLock;
BOOLEAN                filterdInitDone = FALSE;


PVOID
filterAuditAllocMem(
    NDIS_HANDLE  NdisHandle,
    ULONG         Size,
    ULONG         FileNumber,
    ULONG         LineNumber
)
{
    PVOID                pBuffer;
    PFILTERD_ALLOCATION    pAllocInfo;

    if (!filterdInitDone)
    {
        NdisAllocateSpinLock(&(filterdMemoryLock));
        filterdInitDone = TRUE;
    }

    //
    // Integer overflow check
    //
    if ((ULONG)(Size + sizeof(FILTERD_ALLOCATION)) < Size)
    {
        DEBUGP(DL_VERY_LOUD+50,
               "filterAuditAllocMem: Integer overflow error file %d, line %d, Size %d \n",
               FileNumber, LineNumber, Size);

        pBuffer = NULL;
    }
    else
    {

        pAllocInfo = 
            (PFILTERD_ALLOCATION)
            NdisAllocateMemoryWithTagPriority(
                NdisHandle,
                Size+sizeof(FILTERD_ALLOCATION),
                (ULONG)'gdTF',
                LowPoolPriority
            );

        if (pAllocInfo == (PFILTERD_ALLOCATION)NULL)
        {
            DEBUGP(DL_VERY_LOUD+50,
                "filterAuditAllocMem: file %d, line %d, Size %d failed!\n",
                    FileNumber, LineNumber, Size);
            pBuffer = NULL;
        }
        else
        {
            pBuffer = (PVOID)&(pAllocInfo->UserData);
            NdisFillMemory(pBuffer, Size, 0xaf);
            pAllocInfo->Signature = FILTERD_MEMORY_SIGNATURE;
            pAllocInfo->FileNumber = FileNumber;
            pAllocInfo->LineNumber = LineNumber;
            pAllocInfo->Size = Size;
            pAllocInfo->OwnerHandle = NdisHandle;
            pAllocInfo->Next = (PFILTERD_ALLOCATION)NULL;

            NdisAcquireSpinLock(&(filterdMemoryLock));

            pAllocInfo->Prev = filterdMemoryTail;
            if (filterdMemoryTail == (PFILTERD_ALLOCATION)NULL)
            {
                //
                // empty list
                //
                filterdMemoryHead = filterdMemoryTail = pAllocInfo;
            }
            else
            {
                filterdMemoryTail->Next = pAllocInfo;
            }
            filterdMemoryTail = pAllocInfo;

            filterdAllocCount++;
            NdisReleaseSpinLock(&(filterdMemoryLock));
        }
    }

    DEBUGP(DL_VERY_LOUD+100,
     "filterAuditAllocMem: file %c%c%c%c, line %d, %d bytes, OwnerHandle %p, Memory 0x%p\n",
                 (CHAR)(FileNumber & 0xff),
                 (CHAR)((FileNumber >> 8) & 0xff),
                 (CHAR)((FileNumber >> 16) & 0xff),
                 (CHAR)((FileNumber >> 24) & 0xff),
                LineNumber, Size, NdisHandle, pBuffer);

    return (pBuffer);

}


VOID
filterAuditFreeMem(
    PVOID    Pointer
)
{
    PFILTERD_ALLOCATION    pAllocInfo;

    NdisAcquireSpinLock(&(filterdMemoryLock));

    pAllocInfo = CONTAINING_RECORD(Pointer, FILTERD_ALLOCATION, UserData);

    if (pAllocInfo->Signature != FILTERD_MEMORY_SIGNATURE)
    {
        DEBUGP(DL_ERROR,
         "filterAuditFreeMem: unknown buffer 0x%p!\n", Pointer);
        NdisReleaseSpinLock(&(filterdMemoryLock));
#if DBG
        DbgBreakPoint();
#endif
        return;
    }

    pAllocInfo->Signature = (ULONG)'DEAD';
    if (pAllocInfo->Prev != (PFILTERD_ALLOCATION)NULL)
    {
        pAllocInfo->Prev->Next = pAllocInfo->Next;
    }
    else
    {
        filterdMemoryHead = pAllocInfo->Next;
    }
    if (pAllocInfo->Next != (PFILTERD_ALLOCATION)NULL)
    {
        pAllocInfo->Next->Prev = pAllocInfo->Prev;
    }
    else
    {
        filterdMemoryTail = pAllocInfo->Prev;
    }
    filterdAllocCount--;
    NdisReleaseSpinLock(&(filterdMemoryLock));

    NdisFreeMemory(pAllocInfo, 0, 0);
}


VOID
filterAuditShutdown(
    VOID
)
{
    if (filterdInitDone)
    {
        if (filterdAllocCount != 0)
        {
            DEBUGP(DL_ERROR, "AuditShutdown: unfreed memory, %d blocks!\n",
                    filterdAllocCount);
            DEBUGP(DL_ERROR, "MemoryHead: 0x%p, MemoryTail: 0x%p\n",
                    filterdMemoryHead, filterdMemoryTail);
            DbgBreakPoint();
            {
                PFILTERD_ALLOCATION        pAllocInfo;

                while (filterdMemoryHead != (PFILTERD_ALLOCATION)NULL)
                {
                    pAllocInfo = filterdMemoryHead;
                    DEBUGP(DL_INFO, "AuditShutdown: will free 0x%p\n", pAllocInfo);
                    filterAuditFreeMem(&(pAllocInfo->UserData));
                }
            }
        }
        filterdInitDone = FALSE;
    }
}

#define MAX_HD_LENGTH        128

VOID
DbgPrintHexDump(
    IN    PUCHAR            pBuffer,
    IN    ULONG            Length
)
/*++

Routine Description:

    Print a hex dump of the given contiguous buffer. If the length
    is too long, we truncate it.

Arguments:

    pBuffer            - Points to start of data to be dumped
    Length            - Length of above.

Return Value:

    None

--*/
{
    ULONG        i;

    if (Length > MAX_HD_LENGTH)
    {
        Length = MAX_HD_LENGTH;
    }

    for (i = 0; i < Length; i++)
    {
        //
        //  Check if we are at the end of a line
        //
        if ((i > 0) && ((i & 0xf) == 0))
        {
            DbgPrint("\n");
        }

        //
        //  Print addr if we are at start of a new line
        //
        if ((i & 0xf) == 0)
        {
            DbgPrint("%08p ", pBuffer);
        }

        DbgPrint(" %02x", *pBuffer++);
    }

    //
    //  Terminate the last line.
    //
    if (Length > 0)
    {
        DbgPrint("\n");
    }
}
#endif // DBG


#if DBG_SPIN_LOCK
ULONG    filterdSpinLockInitDone = 0;
NDIS_SPIN_LOCK    filterdLockLock;

VOID
filterAllocateSpinLock(
    _In_    PFILTER_LOCK         pLock,
    _In_    ULONG                FileNumber,
    _In_    ULONG                LineNumber
)
{
    if (filterdSpinLockInitDone == 0)
    {
        filterdSpinLockInitDone = 1;
        NdisAllocateSpinLock(&(filterdLockLock));
    }

    NdisAcquireSpinLock(&(filterdLockLock));
    pLock->Signature = FILT_LOCK_SIG;
    pLock->TouchedByFileNumber = FileNumber;
    pLock->TouchedInLineNumber = LineNumber;
    pLock->IsAcquired = 0;
    pLock->OwnerThread = 0;
    NdisAllocateSpinLock(&(pLock->NdisLock));
    NdisReleaseSpinLock(&(filterdLockLock));
}

VOID
filterFreeSpinLock(
    _In_    PFILTER_LOCK        pLock
    )
{
    ASSERT(filterdSpinLockInitDone == 1);


    NdisFreeSpinLock(&(filterdLockLock));
    filterdSpinLockInitDone = 0;
    NdisFreeSpinLock(&(pLock->NdisLock));

}



VOID
filterAcquireSpinLock(
    _In_    PFILTER_LOCK         pLock,
    _In_    ULONG                FileNumber,
    _In_    ULONG                LineNumber,
    _In_  BOOLEAN                DispatchLevel
)
{
    if (DispatchLevel)
    {
        NdisDprAcquireSpinLock(&(filterdLockLock));
    }
    else
    {
        NdisAcquireSpinLock(&(filterdLockLock));
    }
    if (pLock->Signature != FILT_LOCK_SIG)
    {
        DbgPrint("Trying to acquire uninited lock 0x%x, File %c%c%c%c, Line %d\n",
                pLock,
                (CHAR)(FileNumber & 0xff),
                (CHAR)((FileNumber >> 8) & 0xff),
                (CHAR)((FileNumber >> 16) & 0xff),
                (CHAR)((FileNumber >> 24) & 0xff),
                LineNumber);
        DbgBreakPoint();
    }


    pLock->IsAcquired++;

    if (DispatchLevel)
    {
        NdisDprReleaseSpinLock(&(filterdLockLock));
        NdisDprAcquireSpinLock(&(pLock->NdisLock));
    }
    else
    {
        NdisReleaseSpinLock(&(filterdLockLock));
        NdisAcquireSpinLock(&(pLock->NdisLock));
    }

    //
    //  Mark this lock.
    //
    pLock->TouchedByFileNumber = FileNumber;
    pLock->TouchedInLineNumber = LineNumber;
}


VOID
filterReleaseSpinLock(
    _In_    PFILTER_LOCK         pLock,
    _In_    ULONG                FileNumber,
    _In_    ULONG                LineNumber,
    _In_  BOOLEAN                DispatchLevel
)
{
    NdisDprAcquireSpinLock(&(filterdLockLock));
    if (pLock->Signature != FILT_LOCK_SIG)
    {
        DbgPrint("Trying to release uninited lock 0x%x, File %c%c%c%c, Line %d\n",
                pLock,
                (CHAR)(FileNumber & 0xff),
                (CHAR)((FileNumber >> 8) & 0xff),
                (CHAR)((FileNumber >> 16) & 0xff),
                (CHAR)((FileNumber >> 24) & 0xff),
                LineNumber);
        DbgBreakPoint();
    }

    if (pLock->IsAcquired == 0)
    {
        DbgPrint("Detected release of unacquired lock 0x%x, File %c%c%c%c, Line %d\n",
                pLock,
                (CHAR)(FileNumber & 0xff),
                (CHAR)((FileNumber >> 8) & 0xff),
                (CHAR)((FileNumber >> 16) & 0xff),
                (CHAR)((FileNumber >> 24) & 0xff),
                LineNumber);
        DbgBreakPoint();
    }
    pLock->TouchedByFileNumber = FileNumber;
    pLock->TouchedInLineNumber = LineNumber;
    pLock->IsAcquired--;
    NdisDprReleaseSpinLock(&(filterdLockLock));

    if (DispatchLevel)
    {
        NdisDprReleaseSpinLock(&(pLock->NdisLock));
    }
    else
    {
        NdisReleaseSpinLock(&(pLock->NdisLock));
    }
}
#endif // DBG_SPIN_LOCK

