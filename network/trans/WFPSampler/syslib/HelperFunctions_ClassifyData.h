////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_ClassifyData.h
//
//   Abstract:
//      This module contains prototypes for kernel helper functions that assist with CLASSIFY_DATA.
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

#ifndef HELPERFUNCTIONS_CLASSIFY_DATA_H
#define HELPERFUNCTIONS_CLASSIFY_DATA_H

#if DBG

extern INT64 g_OutstandingNBLReferences;

#endif /// DBG

typedef struct CLASSIFY_DATA_
{
   const FWPS_INCOMING_VALUES*          pClassifyValues;
   const FWPS_INCOMING_METADATA_VALUES* pMetadataValues;
   VOID*                                pPacket;               /// NET_BUFFER_LIST | FWPS_STREAM_CALLOUT_IO_PACKET
   const VOID*                          pClassifyContext;
   const FWPS_FILTER*                   pFilter;
   UINT64                               flowContext;
   FWPS_CLASSIFY_OUT*                   pClassifyOut;
   UINT64                               classifyContextHandle;
   BOOLEAN                              chainedNBL;
   UINT32                               numChainedNBLs;
}CLASSIFY_DATA, *PCLASSIFY_DATA;

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprClassifyDataReleaseLocalCopy(_Inout_ CLASSIFY_DATA* pClassifyData);

_At_(*ppClassifyData, _Pre_ _Notnull_)
_At_(*ppClassifyData, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppClassifyData == 0)
inline VOID KrnlHlprClassifyDataDestroyLocalCopy(_Inout_ CLASSIFY_DATA** ppClassifyData);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprClassifyDataAcquireLocalCopy(_Inout_ CLASSIFY_DATA* pClassifyData,
                                              _In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                              _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                              _In_opt_ VOID* pPacket,
                                              _In_opt_ const VOID* pClassifyContext,
                                              _In_ const FWPS_FILTER* pFilter,
                                              _In_ const UINT64 flowContext,
                                              _In_ FWPS_CLASSIFY_OUT* pClassifyOut);

_At_(*ppClassifyData, _Pre_ _Null_)
_When_(return != STATUS_SUCCESS, _At_(*ppClassifyData, _Post_ _Null_))
_When_(return == STATUS_SUCCESS, _At_(*ppClassifyData, _Post_ _Notnull_ __drv_allocatesMem(Pool)))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprClassifyDataCreateLocalCopy(_Outptr_ CLASSIFY_DATA** ppClassifyData,
                                             _In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                             _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                             _In_opt_ VOID* pPacket,
                                             _In_opt_ const VOID* pClassifyContext,
                                             _In_ const FWPS_FILTER* pFilter,
                                             _In_ const UINT64 flowContext,
                                             _In_ FWPS_CLASSIFY_OUT* pClassifyOut);

#endif /// HELPERFUNCTIONS_CLASSIFY_DATA_H
