/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    Microsoft Confidential

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    new.cpp

Abstract:

    Test only stub WDI driver (C++ portions)

Environment:

    Kernel mode

Author:


--*/
#include "MP_Precomp.h"

#define POOL_TAG ((ULONG) '++CS')

void* __cdecl operator new(
    size_t Size,
    ULONG_PTR AllocationContext
    )
/*++

  Override C++ allocation operator. (for use by TLV Parser/Generator)

--*/
{
    PVOID pData = ExAllocatePoolWithTag(  NonPagedPoolNx, Size, POOL_TAG );

    UNREFERENCED_PARAMETER( AllocationContext );
    NT_ASSERT( AllocationContext == 0 );

    if ( pData != NULL )
    {
        RtlZeroMemory( pData, Size );
    }

    return pData;
} 


void __cdecl operator delete(
    void* pData )
/*++

  Override C++ delete operator. (for use by TLV Parser/Generator)

--*/
{
    if ( pData != NULL )
    {
        ExFreePoolWithTag( pData, POOL_TAG );
    }
}

