////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_PendData.h
//
//   Abstract:
//      This module contains prototypes for kernel helper functions that assist with PEND_DATA.
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Add support for pending at 
//                                              FWPM_LAYER_ALE_ENDPOINT_CLOSURE
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef HELPERFUNCTIONS_PEND_DATA_H
#define HELPERFUNCTIONS_PEND_DATA_H

typedef struct PEND_DATA_
{
   HANDLE                            completionContext;
   UINT64                            classifyHandle;
   NET_BUFFER_LIST*                  pNBL;
   UINT16                            layerID;
   BOOLEAN                           isPended;
   BYTE                              pReserved[1];
   union
   {
      PC_PEND_AUTHORIZATION_DATA*    pPendAuthorizationData;
      PC_PEND_ENDPOINT_CLOSURE_DATA* pPendEndpointClosureData;
      VOID*                          pPCPendData;
   };
   FWPS_CLASSIFY_OUT                  classifyOut;
}PEND_DATA, *PPEND_DATA;

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprPendDataPurge(_Inout_ PEND_DATA* pPendData);

_At_(ppPendData, _Pre_ _Notnull_)
_At_(*ppPendData, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppPendData == 0)
inline VOID KrnlHlprPendDataDestroy(_Inout_ PEND_DATA** ppPendData);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprPendDataPopulate(_Inout_ PEND_DATA* pPendData,
                                  _In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                  _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                  _In_opt_ NET_BUFFER_LIST* pNBL,
                                  _In_ const FWPS_FILTER* pFilter,
                                  _In_opt_ VOID* pClassifyContext = 0,
                                  _In_opt_ FWPS_CLASSIFY_OUT* pClassifyOut = 0);

_At_(*ppPendData, _Pre_ _Null_)
_When_(return != STATUS_SUCCESS, _At_(*ppPendData, _Post_ _Null_))
_When_(return == STATUS_SUCCESS, _At_(*ppPendData, _Post_ _Notnull_ __drv_allocatesMem(Pool)))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprPendDataCreate(_Outptr_ PEND_DATA** ppPendData,
                                _In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                _In_opt_ NET_BUFFER_LIST* pNBL,
                                _In_ const FWPS_FILTER* pFilter,
                                _In_opt_ VOID* pClassifyContext = 0,
                                _In_opt_ FWPS_CLASSIFY_OUT* pClassifyOut = 0);

#endif /// HELPERFUNCTIONS_PEND_DATA_H