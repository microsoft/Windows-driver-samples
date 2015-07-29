////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_NDIS.h
//
//   Abstract:
//      This module contains prototypes for kernel helper functions that assist with NDIS operations.
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef HELPERFUNCTIONS_NDIS_H
#define HELPERFUNCTIONS_NDIS_H

typedef struct NDIS_POOL_DATA_
{
   HANDLE ndisHandle;    /// NDIS_HANDLE
   HANDLE nblPoolHandle; /// NDIS_HANDLE
   HANDLE nbPoolHandle;  /// NDIS_HANDLE
}NDIS_POOL_DATA, *PNDIS_POOL_DATA;

extern NDIS_POOL_DATA* g_pNDISPoolData;

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID KrnlHlprNDISPoolDataPurge(_Inout_ NDIS_POOL_DATA* pNDISPoolData);

_At_(*ppNDISPoolData, _Pre_ _Notnull_)
_At_(*ppNDISPoolData, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppNDISPoolData == 0)
VOID KrnlHlprNDISPoolDataDestroy(_Inout_ NDIS_POOL_DATA** ppNDISPoolData);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprNDISPoolDataPopulate(_Inout_ NDIS_POOL_DATA* pNDISPoolData,
                                      _In_opt_ UINT32 memoryTag = WFPSAMPLER_NDIS_POOL_TAG);

_At_(*ppNDISPoolData, _Pre_ _Null_)
_When_(return != STATUS_SUCCESS, _At_(*ppNDISPoolData, _Post_ _Null_))
_When_(return == STATUS_SUCCESS, _At_(*ppNDISPoolData, _Post_ _Notnull_ __drv_allocatesMem(Pool)))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprNDISPoolDataCreate(_Outptr_ NDIS_POOL_DATA** ppNDISPoolData,
                                    _In_opt_ UINT32 memoryTag = WFPSAMPLER_NDIS_POOL_TAG);

#endif /// HELPERFUNCTIONS_NDIS_H