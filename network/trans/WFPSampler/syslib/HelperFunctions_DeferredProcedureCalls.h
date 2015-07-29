////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_DeferredProcedureCalls.h
//
//   Abstract:
//      This module contains prototypes for kernel helper functions that assist with DPC and 
//         threaded DPC routines.
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Add support for serializing asynchronous 
//                                              FWPM_LAYER_STREAM injections
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef HELPERFUNCTIONS_DEFERRED_PROCEDURE_CALLS_H
#define HELPERFUNCTIONS_DEFERRED_PROCEDURE_CALLS_H

typedef struct DPC_DATA_
{
   KDPC              kdpc;
   union
   {
      CLASSIFY_DATA*  pClassifyData;
      NOTIFY_DATA*    pNotifyData;
   };
   union
   {
      INJECTION_DATA* pInjectionData;
      REDIRECT_DATA*  pRedirectData;
      PEND_DATA*      pPendData;
   };
   VOID*              pContext;
}DPC_DATA, *PDPC_DATA;

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprDPCDataPurge(_Inout_ DPC_DATA* pDPCData);

_At_(*ppDPCData, _Pre_ _Notnull_)
_At_(*ppDPCData, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppDPCData == 0)
inline VOID KrnlHlprDPCDataDestroy(_Inout_ DPC_DATA** ppDPCData);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprDPCDataPopulate(_Inout_ DPC_DATA* pDPCData,
                                    _In_ CLASSIFY_DATA* pClassifyData,
                                    _In_opt_ INJECTION_DATA* pInjectionData = 0,
                                    _In_opt_ VOID* pContext = 0);
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprDPCDataPopulate(_Inout_ DPC_DATA* pDPCData,
                                    _In_ CLASSIFY_DATA* pClassifyData,
                                    _In_ PEND_DATA* pPendData,
                                    _In_opt_ VOID* pContext = 0);
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprDPCDataPopulate(_Inout_ DPC_DATA* pDPCData,
                                    _In_ CLASSIFY_DATA* pClassifyData,
                                    _In_ REDIRECT_DATA* pRedirectData,
                                    _In_opt_ VOID** pContext = 0);
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprDPCDataPopulate(_Inout_ DPC_DATA* pDPCData,
                                    _In_ NOTIFY_DATA* pNotifyData,
                                    _In_opt_ VOID* pContext = 0);
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprDPCDataPopulate(_Inout_ DPC_DATA* pDPCData,
                                    _In_ PEND_DATA* pPendData,
                                    _In_opt_ VOID* pContext = 0);

_At_(*ppDPCData, _Pre_ _Null_)
_When_(return != STATUS_SUCCESS, _At_(*ppDPCData, _Post_ _Null_))
_When_(return == STATUS_SUCCESS, _At_(*ppDPCData, _Post_ _Notnull_ __drv_allocatesMem(Pool)))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprDPCDataCreate(_Outptr_ DPC_DATA** ppDPCData,
                               _In_ CLASSIFY_DATA* pClassifyData,
                               _In_opt_ INJECTION_DATA* pInjectionData = 0,
                               _In_opt_ VOID* pContext = 0);
_At_(*ppDPCData, _Pre_ _Null_)
_When_(return != STATUS_SUCCESS, _At_(*ppDPCData, _Post_ _Null_))
_When_(return == STATUS_SUCCESS, _At_(*ppDPCData, _Post_ _Notnull_ __drv_allocatesMem(Pool)))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprDPCDataCreate(_Outptr_ DPC_DATA** ppDPCData,
                               _In_ CLASSIFY_DATA* pClassifyData,
                               _In_ PEND_DATA* pPendData,
                               _In_opt_ VOID* pContext = 0);
_At_(*ppDPCData, _Pre_ _Null_)
_When_(return != STATUS_SUCCESS, _At_(*ppDPCData, _Post_ _Null_))
_When_(return == STATUS_SUCCESS, _At_(*ppDPCData, _Post_ _Notnull_ __drv_allocatesMem(Pool)))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprDPCDataCreate(_Outptr_ DPC_DATA** ppDPCData,
                               _In_ CLASSIFY_DATA* pClassifyData,
                               _In_ REDIRECT_DATA* pRedirectData,
                               _In_opt_ VOID* pContext = 0);
_At_(*ppDPCData, _Pre_ _Null_)
_When_(return != STATUS_SUCCESS, _At_(*ppDPCData, _Post_ _Null_))
_When_(return == STATUS_SUCCESS, _At_(*ppDPCData, _Post_ _Notnull_ __drv_allocatesMem(Pool)))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprDPCDataCreate(_Outptr_ DPC_DATA** ppDPCData,
                               _In_ NOTIFY_DATA* pNotifyData,
                               _In_opt_ VOID* pContext = 0);

_At_(*ppDPCData, _Pre_ _Null_)
_When_(return != STATUS_SUCCESS, _At_(*ppDPCData, _Post_ _Null_))
_When_(return == STATUS_SUCCESS, _At_(*ppDPCData, _Post_ _Notnull_ __drv_allocatesMem(Pool)))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprDPCDataCreate(_Outptr_ DPC_DATA** ppDPCData,
                               _In_ PEND_DATA* pPendData,
                               _In_opt_ VOID* pContext = 0);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprDPCQueue(_In_ KDEFERRED_ROUTINE* pDPCFn,
                          _In_ UINT32 processorNumber = 0,
                          _In_ BOOLEAN forceProcessor = FALSE);
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprDPCQueue(_In_ KDEFERRED_ROUTINE* pDPCFn,
                          _In_ CLASSIFY_DATA* pClassifyData,
                          _In_opt_ INJECTION_DATA* pInjectionData = 0,
                          _In_opt_ VOID* pContext = 0,
                          _In_ UINT32 processorNumber = 0,
                          _In_ BOOLEAN forceProcessor = FALSE);
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprDPCQueue(_In_ KDEFERRED_ROUTINE* pDPCFn,
                          _In_ CLASSIFY_DATA* pClassifyData,
                          _In_ PEND_DATA* pPendData,
                          _In_opt_ VOID* pContext = 0,
                          _In_ UINT32 processorNumber = 0,
                          _In_ BOOLEAN forceProcessor = FALSE);
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprDPCQueue(_In_ KDEFERRED_ROUTINE* pDPCFn,
                          _In_ CLASSIFY_DATA* pClassifyData,
                          _In_ REDIRECT_DATA* pRedirectData,
                          _In_opt_ VOID* pContext = 0,
                          _In_ UINT32 processorNumber = 0,
                          _In_ BOOLEAN forceProcessor = FALSE);
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprDPCQueue(_In_ KDEFERRED_ROUTINE* pDPCFn,
                          _In_ NOTIFY_DATA* pNotifyData,
                          _In_opt_ VOID* pContext = 0,
                          _In_ UINT32 processorNumber = 0,
                          _In_ BOOLEAN forceProcessor = FALSE);
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprDPCQueue(_In_ KDEFERRED_ROUTINE* pDPCFn,
                          _In_ PEND_DATA* pPendData,
                          _In_opt_ VOID* pContext = 0,
                          _In_ UINT32 processorNumber = 0,
                          _In_ BOOLEAN forceProcessor = FALSE);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprThreadedDPCQueue(_In_ KDEFERRED_ROUTINE* pDPCFn,
                                  _In_ UINT32 processorNumber = 0,
                                  _In_ BOOLEAN forceProcessor = FALSE);
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprThreadedDPCQueue(_In_ KDEFERRED_ROUTINE* pDPCFn,
                                  _In_ CLASSIFY_DATA* pClassifyData,
                                  _In_opt_ INJECTION_DATA* pInjectionData = 0,
                                  _In_opt_ VOID* pContext = 0,
                                  _In_ UINT32 processorNumber = 0,
                                  _In_ BOOLEAN forceProcessor = FALSE);
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprThreadedDPCQueue(_In_ KDEFERRED_ROUTINE* pDPCFn,
                                  _In_ CLASSIFY_DATA* pClassifyData,
                                  _In_ PEND_DATA* pPendData,
                                  _In_opt_ VOID* pContext = 0,
                                  _In_ UINT32 processorNumber = 0,
                                  _In_ BOOLEAN forceProcessor = FALSE);
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprThreadedDPCQueue(_In_ KDEFERRED_ROUTINE* pDPCFn,
                                  _In_ CLASSIFY_DATA* pClassifyData,
                                  _In_ REDIRECT_DATA* pRedirectData,
                                  _In_opt_ VOID* pContext = 0,
                                  _In_ UINT32 processorNumber = 0,
                                  _In_ BOOLEAN forceProcessor = FALSE);
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprThreadedDPCQueue(_In_ KDEFERRED_ROUTINE* pDPCFn,
                                  _In_ NOTIFY_DATA* pNotifyData,
                                  _In_opt_ VOID* pContext = 0,
                                  _In_ UINT32 processorNumber = 0,
                                  _In_ BOOLEAN forceProcessor = FALSE);
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprThreadedDPCQueue(_In_ KDEFERRED_ROUTINE* pDPCFn,
                                  _In_ PEND_DATA* pPendData,
                                  _In_opt_ VOID* pContext = 0,
                                  _In_ UINT32 processorNumber = 0,
                                  _In_ BOOLEAN forceProcessor = FALSE);

#endif /// HELPERFUNCTIONS_DEFERRED_PROCEDURE_CALLS_H
