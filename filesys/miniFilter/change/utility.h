/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    utility.h

Abstract:

    Header file which contains the structures, type definitions,
    constants, global variables and function prototypes that are
    only visible within the kernel. The functions include
	generic table routines.

Environment:

    Kernel mode

--*/
#ifndef __UTILITY_H__
#define __UTILITY_H__

#define CG_MUTEX_TAG                         'tMgC'

FORCEINLINE
PFAST_MUTEX
CgAllocateMutex (
    VOID
    )
{
    //
    //  Fast mutex by its rule has to be in the non-paged pool
    //

    return ExAllocatePoolZero( NonPagedPoolNx,
                               sizeof( FAST_MUTEX ),
                               CG_MUTEX_TAG );
}

FORCEINLINE
VOID
CgFreeMutex (
    _In_ PFAST_MUTEX Mutex
    )
{

    ExFreePoolWithTag( Mutex,
                       CG_MUTEX_TAG );
}

#define LIST_FOR_EACH_SAFE(curr, n, head) \
        for (curr = (head)->Flink , n = curr->Flink ; curr != (head); \
             curr = n, n = curr->Flink )


#endif

