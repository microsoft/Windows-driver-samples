////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_WorkItems.cpp
//
//   Abstract:
//      This module contains kernel helper functions that assist with IO_WORKITEM routines.
//
//   Naming Convention:
//
//      <Module><Object><Action>
//  
//      i.e.
//
//       KrnlHlprWorkItemQueue
//
//       <Module>
//          KrnlHlpr     -       Function is located in syslib\ and applies to kernel mode.
//       <Object>
//          WorkItem     -       Function pertains to work item routines.
//          WorkItemData -       Function pertains to WORKITEM_DATA objects.
//       <Action>
//          {
//            Create     -       Function allocates and fills memory.
//            Destroy    -       Function cleans up and frees memory.
//            Populate   -       Function fills memory with values
//            Purge      -       Function cleans up values
//            Queue      -       Function will take a reference on an object
//            Sleep      -       Function performs no operations for a set period.
//          }
//
//   Private Functions:
//
//   Public Functions:
//      KrnlHlprWorkItemDataCreate(),
//      KrnlHlprWorkItemDataDestroy(),
//      KrnlHlprWorkItemDataPopulate(),
//      KrnlHlprWorkItemDataPurge(),
//      KrnlHlprWorkItemQueue(),
//      KrnlHlprWorkItemSleep(),
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

#include "HelperFunctions_Include.h"     /// .
#include "HelperFunctions_WorkItems.tmh" /// $(OBJ_PATH)\$(O)\ 

/**
 @kernel_helper_function="KrnlHlprWorkItemDataPurge"
 
   Purpose:  Cleanup a WORKITEM_DATA object.                                                    <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/FF549133.aspx                              <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprWorkItemDataPurge(_Inout_ WORKITEM_DATA* pWorkItemData)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprWorkItemDataPurge()\n");

#endif /// DBG
   
   NT_ASSERT(pWorkItemData);

   if(pWorkItemData->pIOWorkItem)
      IoFreeWorkItem(pWorkItemData->pIOWorkItem);

   RtlZeroMemory(pWorkItemData,
                 sizeof(WORKITEM_DATA));

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprWorkItemDataPurge()\n");

#endif /// DBG
   
   return;
}

/**
 @kernel_helper_function="KrnlHlprWorkItemDataDestroy"
 
   Purpose:  Cleanup and free a WORKITEM_DATA object.                                           <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_At_(*ppWorkItemData, _Pre_ _Notnull_)
_At_(*ppWorkItemData, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppWorkItemData == 0)
inline VOID KrnlHlprWorkItemDataDestroy(_Inout_ WORKITEM_DATA** ppWorkItemData)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprWorkItemDataDestroy()\n");

#endif /// DBG
   
   NT_ASSERT(ppWorkItemData);

   if(*ppWorkItemData)
   {
      KrnlHlprWorkItemDataPurge(*ppWorkItemData);

      HLPR_DELETE(*ppWorkItemData,
                  WFPSAMPLER_SYSLIB_TAG);
   }

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprWorkItemDataDestroy()\n");

#endif /// DBG
   
   return;
}

/**
 @kernel_helper_function="KrnlHlprWorkItemDataPopulate"
 
   Purpose:  Populates a WORKITEM_DATA object with the classifyData, injectionData, 
             PIOWorkItem, and context supplied.                                                 <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprWorkItemDataPopulate(_Inout_ WORKITEM_DATA* pWorkItemData,
                                         _In_ CLASSIFY_DATA* pClassifyData,
                                         _In_opt_ INJECTION_DATA* pInjectionData, /* 0 */
                                         _In_opt_ PIO_WORKITEM pIOWorkItem,       /* 0 */
                                         _In_opt_ VOID* pContext)                 /* 0 */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprWorkItemDataPopulate()\n");

#endif /// DBG
   
   NT_ASSERT(pWorkItemData);
   NT_ASSERT(pClassifyData);

   pWorkItemData->pIOWorkItem    = pIOWorkItem;
   pWorkItemData->pClassifyData  = pClassifyData;
   pWorkItemData->pInjectionData = pInjectionData;
   pWorkItemData->pContext       = pContext;

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprWorkItemDataPopulate()\n");

#endif /// DBG
   
   return;
}

/**
 @kernel_helper_function="KrnlHlprWorkItemDataPopulate"
 
   Purpose:  Populates a WORKITEM_DATA object with the classifyData, pendData, PIOWorkItem, and 
             context supplied.                                                                  <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprWorkItemDataPopulate(_Inout_ WORKITEM_DATA* pWorkItemData,
                                         _In_ CLASSIFY_DATA* pClassifyData,
                                         _In_ PEND_DATA* pPendData,
                                         _In_opt_ PIO_WORKITEM pIOWorkItem,    /* 0 */
                                         _In_opt_ VOID* pContext)              /* 0 */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprWorkItemDataPopulate()\n");

#endif /// DBG
   
   NT_ASSERT(pWorkItemData);
   NT_ASSERT(pClassifyData);
   NT_ASSERT(pPendData);

   pWorkItemData->pIOWorkItem   = pIOWorkItem;
   pWorkItemData->pClassifyData = pClassifyData;
   pWorkItemData->pPendData     = pPendData;
   pWorkItemData->pContext      = pContext;

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprWorkItemDataPopulate()\n");

#endif /// DBG
   
   return;
}

/**
 @kernel_helper_function="KrnlHlprWorkItemDataPopulate"
 
   Purpose:  Populates a WORKITEM_DATA object with the classifyData, redirectData, PIOWorkItem, 
             and context supplied.                                                              <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprWorkItemDataPopulate(_Inout_ WORKITEM_DATA* pWorkItemData,
                                         _In_ CLASSIFY_DATA* pClassifyData,
                                         _In_ REDIRECT_DATA* pRedirectData,
                                         _In_opt_ PIO_WORKITEM pIOWorkItem,    /* 0 */
                                         _In_opt_ VOID* pContext)              /* 0 */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprWorkItemDataPopulate()\n");

#endif /// DBG
   
   NT_ASSERT(pWorkItemData);
   NT_ASSERT(pClassifyData);
   NT_ASSERT(pRedirectData);

   pWorkItemData->pIOWorkItem   = pIOWorkItem;
   pWorkItemData->pClassifyData = pClassifyData;
   pWorkItemData->pRedirectData = pRedirectData;
   pWorkItemData->pContext      = pContext;

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprWorkItemDataPopulate()\n");

#endif /// DBG
   
   return;
}

/**
 @kernel_helper_function="KrnlHlprWorkItemDataPopulate"
 
   Purpose:  Populates a WORKITEM_DATA object with the notifyData, PIOWorkItem, and context 
             supplied.                                                                          <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprWorkItemDataPopulate(_Inout_ WORKITEM_DATA* pWorkItemData,
                                         _In_ NOTIFY_DATA* pNotifyData,
                                         _In_opt_ PIO_WORKITEM pIOWorkItem,   /* 0 */
                                         _In_opt_ VOID* pContext)             /* 0 */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprWorkItemDataPopulate()\n");

#endif /// DBG
   
   NT_ASSERT(pWorkItemData);
   NT_ASSERT(pNotifyData);

   pWorkItemData->pIOWorkItem = pIOWorkItem;
   pWorkItemData->pNotifyData = pNotifyData;
   pWorkItemData->pContext    = pContext;

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprWorkItemDataPopulate()\n");

#endif /// DBG
   
   return;
}

/**
 @kernel_helper_function="KrnlHlprWorkItemDataPopulate"
 
   Purpose:  Populates a WORKITEM_DATA object with the pendData, PIOWorkItem, and context 
             supplied.                                                                          <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprWorkItemDataPopulate(_Inout_ WORKITEM_DATA* pWorkItemData,
                                         _In_ PEND_DATA* pPendData,
                                         _In_opt_ PIO_WORKITEM pIOWorkItem,    /* 0 */
                                         _In_opt_ VOID* pContext)              /* 0 */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprWorkItemDataPopulate()\n");

#endif /// DBG
   
   NT_ASSERT(pWorkItemData);
   NT_ASSERT(pPendData);

   pWorkItemData->pIOWorkItem   = pIOWorkItem;
   pWorkItemData->pPendData     = pPendData;
   pWorkItemData->pContext      = pContext;

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprWorkItemDataPopulate()\n");

#endif /// DBG
   
   return;
}

/**
 @kernel_helper_function="KrnlHlprWorkItemDataCreate"
 
   Purpose:  Allocates and populates a WORKITEM_DATA object with the classifyData, 
             injectionData, PIOWorkItem, and context supplied.                                  <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
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
                                    _In_opt_ INJECTION_DATA* pInjectionData, /* 0 */
                                    _In_opt_ PIO_WORKITEM pIOWorkItem,       /* 0 */
                                    _In_opt_ VOID* pContext)                 /* 0 */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprWorkItemDataCreate()\n");

#endif /// DBG
   
   NT_ASSERT(ppWorkItemData);
   NT_ASSERT(pClassifyData);
   NT_ASSERT(pInjectionData);

   NTSTATUS status = STATUS_SUCCESS;

   HLPR_NEW(*ppWorkItemData,
            WORKITEM_DATA,
            WFPSAMPLER_SYSLIB_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(*ppWorkItemData,
                              status);

   KrnlHlprWorkItemDataPopulate(*ppWorkItemData,
                                pClassifyData,
                                pInjectionData,
                                pIOWorkItem,
                                pContext);

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
      KrnlHlprWorkItemDataDestroy(ppWorkItemData);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprWorkItemDataCreate() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}

/**
 @kernel_helper_function="KrnlHlprWorkItemDataCreate"
 
   Purpose:  Allocates and populates a WORKITEM_DATA object with the classifyData, pendData, 
             PIOWorkItem, and context supplied.                                                 <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
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
                                    _In_opt_ PIO_WORKITEM pIOWorkItem,       /* 0 */
                                    _In_opt_ VOID* pContext)                 /* 0 */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprWorkItemDataCreate()\n");

#endif /// DBG
   
   NT_ASSERT(ppWorkItemData);
   NT_ASSERT(pClassifyData);
   NT_ASSERT(pPendData);

   NTSTATUS status = STATUS_SUCCESS;

   HLPR_NEW(*ppWorkItemData,
            WORKITEM_DATA,
            WFPSAMPLER_SYSLIB_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(*ppWorkItemData,
                              status);

   KrnlHlprWorkItemDataPopulate(*ppWorkItemData,
                                pClassifyData,
                                pPendData,
                                pIOWorkItem,
                                pContext);

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
      KrnlHlprWorkItemDataDestroy(ppWorkItemData);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprWorkItemDataCreate() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}

/**
 @kernel_helper_function="KrnlHlprWorkItemDataCreate"
 
   Purpose:  Allocates and populates a WORKITEM_DATA object with the classifyData, redirectData, 
             PIOWorkItem, and context supplied.                                                 <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
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
                                    _In_opt_ PIO_WORKITEM pIOWorkItem,       /* 0 */
                                    _In_opt_ VOID* pContext)                 /* 0 */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprWorkItemDataCreate()\n");

#endif /// DBG
   
   NT_ASSERT(ppWorkItemData);
   NT_ASSERT(pClassifyData);
   NT_ASSERT(pRedirectData);

   NTSTATUS status = STATUS_SUCCESS;

   HLPR_NEW(*ppWorkItemData,
            WORKITEM_DATA,
            WFPSAMPLER_SYSLIB_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(*ppWorkItemData,
                              status);

   KrnlHlprWorkItemDataPopulate(*ppWorkItemData,
                                pClassifyData,
                                pRedirectData,
                                pIOWorkItem,
                                pContext);

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
      KrnlHlprWorkItemDataDestroy(ppWorkItemData);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprWorkItemDataCreate() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}

/**
 @kernel_helper_function="KrnlHlprWorkItemDataCreate"
 
   Purpose:  Allocates and populates a WORKITEM_DATA object with the notifyData, PIOWorkItem, 
             and context supplied.                                                              <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
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
                                    _In_opt_ PIO_WORKITEM pIOWorkItem,       /* 0 */
                                    _In_opt_ VOID* pContext)                 /* 0 */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprWorkItemDataCreate()\n");

#endif /// DBG
   
   NT_ASSERT(ppWorkItemData);
   NT_ASSERT(pNotifyData);

   NTSTATUS status = STATUS_SUCCESS;

   HLPR_NEW(*ppWorkItemData,
            WORKITEM_DATA,
            WFPSAMPLER_SYSLIB_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(*ppWorkItemData,
                              status);

   KrnlHlprWorkItemDataPopulate(*ppWorkItemData,
                                pNotifyData,
                                pIOWorkItem,
                                pContext);

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
      KrnlHlprWorkItemDataDestroy(ppWorkItemData);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprWorkItemDataCreate() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}

/**
 @kernel_helper_function="KrnlHlprWorkItemDataCreate"
 
   Purpose:  Allocates and populates a WORKITEM_DATA object with the pendData, PIOWorkItem, and 
             context supplied.                                                                  <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
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
                                    _In_opt_ PIO_WORKITEM pIOWorkItem,       /* 0 */
                                    _In_opt_ VOID* pContext)                 /* 0 */
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprWorkItemDataCreate()\n");

#endif /// DBG

   NT_ASSERT(ppWorkItemData);
   NT_ASSERT(pPendData);

   NTSTATUS status = STATUS_SUCCESS;

   HLPR_NEW(*ppWorkItemData,
            WORKITEM_DATA,
            WFPSAMPLER_SYSLIB_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(*ppWorkItemData,
                              status);

   KrnlHlprWorkItemDataPopulate(*ppWorkItemData,
                                pPendData,
                                pIOWorkItem,
                                pContext);

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
      KrnlHlprWorkItemDataDestroy(ppWorkItemData);

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprWorkItemDataCreate() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

/**
 @kernel_helper_function="KrnlHlprWorkItemQueue"
 
   Purpose:  Queue a workitem for later execution at PASSIVE_LEVEL.                             <br>
                                                                                                <br>
   Notes:    Data is expected to be obtained by other means (i.e. LIST_ENTRY)                   <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/FF548276.aspx                              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/FF549466.aspx                              <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprWorkItemQueue(_In_ PDEVICE_OBJECT pWDMDevice,
                               _In_ IO_WORKITEM_ROUTINE* pWorkItemFn)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprWorkItemQueue()\n");

#endif /// DBG
   
   NT_ASSERT(pWDMDevice);
   NT_ASSERT(pWorkItemFn);

   NTSTATUS     status      = STATUS_SUCCESS;
   PIO_WORKITEM pIOWorkItem = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pIOWorkItem is cleaned up in pWorkItemFn

   pIOWorkItem = IoAllocateWorkItem(pWDMDevice);
   if(pIOWorkItem == 0)
   {
      status = STATUS_UNSUCCESSFUL;

      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! KrnlHlprWorkItemQueue : IoAllocateWorkItem() [status: %#x]\n",
                 status);

      HLPR_BAIL;
   }

#pragma warning(pop)

   IoQueueWorkItem(pIOWorkItem,
                   pWorkItemFn,
                   DelayedWorkQueue,
                   (PVOID)pIOWorkItem);

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS &&
      pIOWorkItem)
      IoFreeWorkItem(pIOWorkItem);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprWorkItemQueue() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}


/**
 @kernel_helper_function="KrnlHlprWorkItemQueue"
 
   Purpose:  Queue a workitem for later execution at PASSIVE_LEVEL.                             <br>
                                                                                                <br>
   Notes:    INJECTION_DATA specific.                                                           <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/FF548276.aspx                              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/FF549466.aspx                              <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprWorkItemQueue(_In_ PDEVICE_OBJECT pWDMDevice,
                               _In_ IO_WORKITEM_ROUTINE* pWorkItemFn,
                               _In_ CLASSIFY_DATA* pClassifyData,
                               _In_opt_ INJECTION_DATA* pInjectionData, /* 0 */
                               _In_opt_ VOID* pContext)                 /* 0 */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprWorkItemQueue()\n");

#endif /// DBG
   
   NT_ASSERT(pWDMDevice);
   NT_ASSERT(pWorkItemFn);
   NT_ASSERT(pClassifyData);

   NTSTATUS       status        = STATUS_SUCCESS;
   PIO_WORKITEM   pIOWorkItem   = 0;
   WORKITEM_DATA* pWorkItemData = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pIOWorkItem is cleaned up in KrnlHlprWorkItemDataDestroy

   pIOWorkItem = IoAllocateWorkItem(pWDMDevice);
   if(pIOWorkItem == 0)
   {
      status = STATUS_UNSUCCESSFUL;

      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! KrnlHlprWorkItemQueue : IoAllocateWorkItem() [status: %#x]\n",
                 status);

      HLPR_BAIL;
   }

#pragma warning(pop)

   status = KrnlHlprWorkItemDataCreate(&pWorkItemData,
                                       pClassifyData,
                                       pInjectionData,
                                       pIOWorkItem,
                                       pContext);
   HLPR_BAIL_ON_FAILURE(status);

   IoQueueWorkItem(pWorkItemData->pIOWorkItem,
                   pWorkItemFn,
                   DelayedWorkQueue,
                   (PVOID)pWorkItemData);

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS &&
      pWorkItemData)
      KrnlHlprWorkItemDataDestroy(&pWorkItemData);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprWorkItemQueue() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}

/**
 @kernel_helper_function="KrnlHlprWorkItemQueue"
 
   Purpose:  Queue a workitem for later execution at PASSIVE_LEVEL.                             <br>
                                                                                                <br>
   Notes:    PEND_DATA specific.                                                                <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/FF548276.aspx                              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/FF549466.aspx                              <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprWorkItemQueue(_In_ PDEVICE_OBJECT pWDMDevice,
                               _In_ IO_WORKITEM_ROUTINE* pWorkItemFn,
                               _In_ CLASSIFY_DATA* pClassifyData,
                               _In_ PEND_DATA* pPendData,
                               _In_opt_ VOID* pContext)               /* 0 */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprWorkItemQueue()\n");

#endif /// DBG
   
   NT_ASSERT(pWDMDevice);
   NT_ASSERT(pWorkItemFn);
   NT_ASSERT(pClassifyData);
   NT_ASSERT(pPendData);

   NTSTATUS       status        = STATUS_SUCCESS;
   PIO_WORKITEM   pIOWorkItem   = 0;
   WORKITEM_DATA* pWorkItemData = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pIOWorkItem is cleaned up in KrnlHlprWorkItemDataDestroy

   pIOWorkItem = IoAllocateWorkItem(pWDMDevice);
   if(pIOWorkItem == 0)
   {
      status = STATUS_UNSUCCESSFUL;

      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! KrnlHlprWorkItemQueue : IoAllocateWorkItem() [status: %#x]\n",
                 status);

      HLPR_BAIL;
   }

#pragma warning(pop)

   status = KrnlHlprWorkItemDataCreate(&pWorkItemData,
                                       pClassifyData,
                                       pPendData,
                                       pIOWorkItem,
                                       pContext);
   HLPR_BAIL_ON_FAILURE(status);

   IoQueueWorkItem(pWorkItemData->pIOWorkItem,
                   pWorkItemFn,
                   DelayedWorkQueue,
                   (PVOID)pWorkItemData);

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS &&
      pWorkItemData)
      KrnlHlprWorkItemDataDestroy(&pWorkItemData);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprWorkItemQueue() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}

/**
 @kernel_helper_function="KrnlHlprWorkItemQueue"
 
   Purpose:  Queue a workitem for later execution at PASSIVE_LEVEL.                             <br>
                                                                                                <br>
   Notes:    REDIRECT_DATA specific.                                                            <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/FF548276.aspx                              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/FF549466.aspx                              <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprWorkItemQueue(_In_ PDEVICE_OBJECT pWDMDevice,
                               _In_ IO_WORKITEM_ROUTINE* pWorkItemFn,
                               _In_ CLASSIFY_DATA* pClassifyData,
                               _In_ REDIRECT_DATA* pRedirectData,
                               _In_opt_ VOID* pContext)               /* 0 */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprWorkItemQueue()\n");

#endif /// DBG
   
   NT_ASSERT(pWDMDevice);
   NT_ASSERT(pWorkItemFn);
   NT_ASSERT(pClassifyData);
   NT_ASSERT(pRedirectData);

   NTSTATUS       status        = STATUS_SUCCESS;
   PIO_WORKITEM   pIOWorkItem   = 0;
   WORKITEM_DATA* pWorkItemData = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pIOWorkItem is cleaned up in KrnlHlprWorkItemDataDestroy

   pIOWorkItem = IoAllocateWorkItem(pWDMDevice);
   if(pIOWorkItem == 0)
   {
      status = STATUS_UNSUCCESSFUL;

      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! KrnlHlprWorkItemQueue : IoAllocateWorkItem() [status: %#x]\n",
                 status);

      HLPR_BAIL;
   }

#pragma warning(pop)

   status = KrnlHlprWorkItemDataCreate(&pWorkItemData,
                                       pClassifyData,
                                       pRedirectData,
                                       pIOWorkItem,
                                       pContext);
   HLPR_BAIL_ON_FAILURE(status);

   IoQueueWorkItem(pWorkItemData->pIOWorkItem,
                   pWorkItemFn,
                   DelayedWorkQueue,
                   (PVOID)pWorkItemData);

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS &&
      pWorkItemData)
      KrnlHlprWorkItemDataDestroy(&pWorkItemData);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprWorkItemQueue() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}

/**
 @kernel_helper_function="KrnlHlprWorkItemQueue"
 
   Purpose:  Queue a workitem for later execution at PASSIVE_LEVEL.                             <br>
                                                                                                <br>
   Notes:    NOTIFY_DATA specific.                                                              <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/FF548276.aspx                              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/FF549466.aspx                              <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprWorkItemQueue(_In_ PDEVICE_OBJECT pWDMDevice,
                               _In_ IO_WORKITEM_ROUTINE* pWorkItemFn,
                               _In_ NOTIFY_DATA* pNotifyData,
                               _In_opt_ VOID* pContext)               /* 0 */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprWorkItemQueue()\n");

#endif /// DBG
   
   NT_ASSERT(pWDMDevice);
   NT_ASSERT(pWorkItemFn);
   NT_ASSERT(pNotifyData);

   NTSTATUS       status        = STATUS_SUCCESS;
   PIO_WORKITEM   pIOWorkItem   = 0;
   WORKITEM_DATA* pWorkItemData = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pIOWorkItem is cleaned up in KrnlHlprWorkItemDataDestroy

   pIOWorkItem = IoAllocateWorkItem(pWDMDevice);
   if(pIOWorkItem == 0)
   {
      status = STATUS_UNSUCCESSFUL;

      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! KrnlHlprWorkItemQueue : IoAllocateWorkItem() [status: %#x]\n",
                 status);

      HLPR_BAIL;
   }

#pragma warning(pop)

   status = KrnlHlprWorkItemDataCreate(&pWorkItemData,
                                       pNotifyData,
                                       pIOWorkItem,
                                       pContext);
   HLPR_BAIL_ON_FAILURE(status);

   IoQueueWorkItem(pWorkItemData->pIOWorkItem,
                   pWorkItemFn,
                   DelayedWorkQueue,
                   (PVOID)pWorkItemData);

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS &&
      pWorkItemData)
      KrnlHlprWorkItemDataDestroy(&pWorkItemData);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprWorkItemQueue() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}

/**
 @kernel_helper_function="KrnlHlprWorkItemQueue"
 
   Purpose:  Queue a workitem for later execution at PASSIVE_LEVEL.                             <br>
                                                                                                <br>
   Notes:    PEND_DATA specific.                                                                <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/FF548276.aspx                              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/FF549466.aspx                              <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprWorkItemQueue(_In_ PDEVICE_OBJECT pWDMDevice,
                               _In_ IO_WORKITEM_ROUTINE* pWorkItemFn,
                               _In_ PEND_DATA* pPendData,
                               _In_opt_ VOID* pContext)               /* 0 */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprWorkItemQueue()\n");

#endif /// DBG
   
   NT_ASSERT(pWDMDevice);
   NT_ASSERT(pWorkItemFn);
   NT_ASSERT(pPendData);

   NTSTATUS       status        = STATUS_SUCCESS;
   PIO_WORKITEM   pIOWorkItem   = 0;
   WORKITEM_DATA* pWorkItemData = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pIOWorkItem is cleaned up in KrnlHlprWorkItemDataDestroy

   pIOWorkItem = IoAllocateWorkItem(pWDMDevice);
   if(pIOWorkItem == 0)
   {
      status = STATUS_UNSUCCESSFUL;

      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! KrnlHlprWorkItemQueue : IoAllocateWorkItem() [status: %#x]\n",
                 status);

      HLPR_BAIL;
   }

#pragma warning(pop)

   status = KrnlHlprWorkItemDataCreate(&pWorkItemData,
                                       pPendData,
                                       pIOWorkItem,
                                       pContext);
   HLPR_BAIL_ON_FAILURE(status);

   IoQueueWorkItem(pWorkItemData->pIOWorkItem,
                   pWorkItemFn,
                   DelayedWorkQueue,
                   (PVOID)pWorkItemData);

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS &&
      pWorkItemData)
      KrnlHlprWorkItemDataDestroy(&pWorkItemData);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprWorkItemQueue() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}

/**
 @kernel_helper_function="KrnlHlprWorkItemSleep"
 
   Purpose:  Delay the thread for the specified time.                                           <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/FF551986.aspx                              <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_same_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprWorkItemSleep(_In_ UINT32 numMS)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprWorkItemSleep()\n");

#endif /// DBG
   
   NT_ASSERT(numMS);

   NTSTATUS status   = STATUS_SUCCESS;
   INT64    interval = numMS * -10000i64; /// (numMS[milli] * (-1[relative] * 1000[milli to micro] * 1000[micro to nano]) / 100[ns]

   status = KeDelayExecutionThread(KernelMode,
                                   FALSE,
                                   (PLARGE_INTEGER)&interval);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprWorkItemSleep() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}
