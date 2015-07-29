////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_WorkItems.h
//
//   Abstract:
//      This module contains prototypes for kernel helper functions that assist with IO_WORKITEM
//         routines.
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Add support for pending at 
//                                              FWPM_LAYER_ALE_ENDPOINT_CLOSURE.
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef HELPERFUNCTIONS_WORKITEMS_H
#define HELPERFUNCTIONS_WORKITEMS_H

typedef struct WORKITEM_DATA_
{
   PIO_WORKITEM       pIOWorkItem;
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
}WORKITEM_DATA, *PWORKITEM_DATA;

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprWorkItemDataPurge(_Inout_ WORKITEM_DATA* pWorkItemData);

_At_(*ppWorkItemData, _Pre_ _Notnull_)
_At_(*ppWorkItemData, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppWorkItemData == 0)
inline VOID KrnlHlprWorkItemDataDestroy(_Inout_ WORKITEM_DATA** ppWorkItemData);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprWorkItemDataPopulate(_Inout_ WORKITEM_DATA* pWorkItemData,
                                         _In_ CLASSIFY_DATA* pClassifyData,
                                         _In_opt_ INJECTION_DATA* pInjectionData = 0,
                                         _In_opt_ PIO_WORKITEM pIOWorkItem = 0,
                                         _In_opt_ VOID* pContext = 0);
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprWorkItemDataPopulate(_Inout_ WORKITEM_DATA* pWorkItemData,
                                         _In_ CLASSIFY_DATA* pClassifyData,
                                         _In_ PEND_DATA* pPendData,
                                         _In_opt_ PIO_WORKITEM pIOWorkItem = 0,
                                         _In_opt_ VOID* pContext = 0);
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprWorkItemDataPopulate(_Inout_ WORKITEM_DATA* pWorkItemData,
                                         _In_ CLASSIFY_DATA* pClassifyData,
                                         _In_ REDIRECT_DATA* pRedirectData,
                                         _In_opt_ PIO_WORKITEM pIOWorkItem = 0,
                                         _In_opt_ VOID** pContext = 0);
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprWorkItemDataPopulate(_Inout_ WORKITEM_DATA* pWorkItemData,
                                         _In_ NOTIFY_DATA* pNotifyData,
                                         _In_opt_ PIO_WORKITEM pIOWorkItem = 0,
                                         _In_opt_ VOID* pContext = 0);
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprWorkItemDataPopulate(_Inout_ WORKITEM_DATA* pWorkItemData,
                                         _In_ PEND_DATA* pPendData,
                                         _In_opt_ PIO_WORKITEM pIOWorkItem = 0,
                                         _In_opt_ VOID* pContext = 0);

_At_(*ppWorkItemData, _Pre_ _Null_)
_When_(return != STATUS_SUCCESS, _At_(*ppWorkItemData, _Post_ _Null_))
_When_(return == STATUS_SUCCESS, _At_(*ppWorkItemData, _Post_ _Notnull_ __drv_allocatesMem(Pool)))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprWorkItemDataCreate(_Outptr_ WORKITEM_DATA** ppWorkItemData,
                                    _In_ CLASSIFY_DATA* pClassifyData,
                                    _In_opt_ INJECTION_DATA* pInjectionData = 0,
                                    _In_opt_ PIO_WORKITEM pIOWorkItem = 0,
                                    _In_opt_ VOID* pContext = 0);
_At_(*ppWorkItemData, _Pre_ _Null_)
_When_(return != STATUS_SUCCESS, _At_(*ppWorkItemData, _Post_ _Null_))
_When_(return == STATUS_SUCCESS, _At_(*ppWorkItemData, _Post_ _Notnull_ __drv_allocatesMem(Pool)))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprWorkItemDataCreate(_Outptr_ WORKITEM_DATA** ppWorkItemData,
                                    _In_ CLASSIFY_DATA* pClassifyData,
                                    _In_ PEND_DATA* pPendData,
                                    _In_opt_ PIO_WORKITEM pIOWorkItem = 0,
                                    _In_opt_ VOID* pContext = 0);
_At_(*ppWorkItemData, _Pre_ _Null_)
_When_(return != STATUS_SUCCESS, _At_(*ppWorkItemData, _Post_ _Null_))
_When_(return == STATUS_SUCCESS, _At_(*ppWorkItemData, _Post_ _Notnull_ __drv_allocatesMem(Pool)))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprWorkItemDataCreate(_Outptr_ WORKITEM_DATA** ppWorkItemData,
                                    _In_ CLASSIFY_DATA* pClassifyData,
                                    _In_ REDIRECT_DATA* pRedirectData,
                                    _In_opt_ PIO_WORKITEM pIOWorkItem = 0,
                                    _In_opt_ VOID* pContext = 0);
_At_(*ppWorkItemData, _Pre_ _Null_)
_When_(return != STATUS_SUCCESS, _At_(*ppWorkItemData, _Post_ _Null_))
_When_(return == STATUS_SUCCESS, _At_(*ppWorkItemData, _Post_ _Notnull_ __drv_allocatesMem(Pool)))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprWorkItemDataCreate(_Outptr_ WORKITEM_DATA** ppWorkItemData,
                                    _In_ NOTIFY_DATA* pNotifyData,
                                    _In_opt_ PIO_WORKITEM pIOWorkItem = 0,
                                    _In_opt_ VOID* pContext = 0);

_At_(*ppWorkItemData, _Pre_ _Null_)
_When_(return != STATUS_SUCCESS, _At_(*ppWorkItemData, _Post_ _Null_))
_When_(return == STATUS_SUCCESS, _At_(*ppWorkItemData, _Post_ _Notnull_ __drv_allocatesMem(Pool)))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprWorkItemDataCreate(_Outptr_ WORKITEM_DATA** ppWorkItemData,
                                    _In_ PEND_DATA* pPendData,
                                    _In_opt_ PIO_WORKITEM pIOWorkItem = 0,
                                    _In_opt_ VOID* pContext = 0);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprWorkItemQueue(_In_ PDEVICE_OBJECT pWDMDevice,
                               _In_ IO_WORKITEM_ROUTINE* pWorkItemFn);
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprWorkItemQueue(_In_ PDEVICE_OBJECT pWDMDevice,
                               _In_ IO_WORKITEM_ROUTINE* pWorkItemFn,
                               _In_ CLASSIFY_DATA* pClassifyData,
                               _In_opt_ INJECTION_DATA* pInjectionData = 0,
                               _In_opt_ VOID* pContext = 0);
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprWorkItemQueue(_In_ PDEVICE_OBJECT pWDMDevice,
                               _In_ IO_WORKITEM_ROUTINE* pWorkItemFn,
                               _In_ CLASSIFY_DATA* pClassifyData,
                               _In_ PEND_DATA* pPendData,
                               _In_opt_ VOID* pContext = 0);
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprWorkItemQueue(_In_ PDEVICE_OBJECT pWDMDevice,
                               _In_ IO_WORKITEM_ROUTINE* pWorkItemFn,
                               _In_ CLASSIFY_DATA* pClassifyData,
                               _In_ REDIRECT_DATA* pRedirectData,
                               _In_opt_ VOID* pContext = 0);
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprWorkItemQueue(_In_ PDEVICE_OBJECT pWDMDevice,
                               _In_ IO_WORKITEM_ROUTINE* pWorkItemFn,
                               _In_ NOTIFY_DATA* pNotifyData,
                               _In_opt_ VOID* pContext = 0);
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprWorkItemQueue(_In_ PDEVICE_OBJECT pWDMDevice,
                               _In_ IO_WORKITEM_ROUTINE* pWorkItemFn,
                               _In_ PEND_DATA* pPendData,
                               _In_opt_ VOID* pContext = 0);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_same_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprWorkItemSleep(_In_ UINT32 numMS);

#endif /// HELPERFUNCTIONS_WORKITEMS_H
