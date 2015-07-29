////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_DeferredProcedureCalls.cpp
//
//   Abstract:
//      This module contains kernel helper functions that assist with DPC and threaded DPC routines.
//
//   Naming Convention:
//
//      <Module><Object><Action>
//  
//      i.e.
//
//       KrnlHlprDPCQueue
//
//       <Module>
//          KrnlHlpr     -       Function is located in syslib\ and applies to kernel mode.
//       <Object>
//          DPC          -       Function pertains to deferred procedure calls.
//          DPCData      -       Function pertains to DPC_DATA objects.
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
//      KrnlHlprDPCDataCreate(),
//      KrnlHlprDPCDataDestroy(),
//      KrnlHlprDPCDataPopulate(),
//      KrnlHlprDPCDataPurge(),
//      KrnlHlprDPCQueue(),
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

#include "HelperFunctions_Include.h"                  /// .
#include "HelperFunctions_DeferredProcedureCalls.tmh" /// $(OBJ_PATH)\$(O)\

#ifndef DPC____
#define DPC____

/**
 @kernel_helper_function="KrnlHlprDPCDataPurge"
 
   Purpose:  Cleanup a DPC_DATA object.                                                         <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprDPCDataPurge(_Inout_ DPC_DATA* pDPCData)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprDPCDataPurge()\n");

#endif /// DBG

   NT_ASSERT(pDPCData);

   RtlZeroMemory(pDPCData,
                 sizeof(DPC_DATA));

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprDPCDataPurge()\n");

#endif /// DBG

   return;
}

/**
 @kernel_helper_function="KrnlHlprDPCDataDestroy"
 
   Purpose:  Cleanup and free a DPC_DATA object.                                                <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_At_(*ppDPCData, _Pre_ _Notnull_)
_At_(*ppDPCData, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppDPCData == 0)
inline VOID KrnlHlprDPCDataDestroy(_Inout_ DPC_DATA** ppDPCData)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprDPCDataDestroy()\n");

#endif /// DBG

   NT_ASSERT(ppDPCData);

   if(*ppDPCData)
   {
      KrnlHlprDPCDataPurge(*ppDPCData);

      HLPR_DELETE(*ppDPCData,
                  WFPSAMPLER_SYSLIB_TAG);
   }

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprDPCDataDestroy()\n");

#endif /// DBG

   return;
}

/**
 @kernel_helper_function="KrnlHlprDPCDataPopulate"
 
   Purpose:  Populates a DPC_DATA object with the classifyData, injectionData, and context 
             supplied.                                                                          <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprDPCDataPopulate(_Inout_ DPC_DATA* pDPCData,
                                    _In_ CLASSIFY_DATA* pClassifyData,
                                    _In_opt_ INJECTION_DATA* pInjectionData, /* 0 */
                                    _In_opt_ VOID* pContext)                 /* 0 */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprDPCDataPopulate()\n");

#endif /// DBG
   
   NT_ASSERT(pDPCData);
   NT_ASSERT(pClassifyData);

   pDPCData->pClassifyData  = pClassifyData;
   pDPCData->pInjectionData = pInjectionData;
   pDPCData->pContext       = pContext;

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprDPCDataPopulate()\n");

#endif /// DBG
   
   return;
}

/**
 @kernel_helper_function="KrnlHlprDPCDataPopulate"
 
   Purpose:  Populates a DPC_DATA object with the classifyData, pendData, and context supplied. <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprDPCDataPopulate(_Inout_ DPC_DATA* pDPCData,
                                    _In_ CLASSIFY_DATA* pClassifyData,
                                    _In_ PEND_DATA* pPendData,
                                    _In_opt_ VOID* pContext)           /* 0 */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprDPCDataPopulate()\n");

#endif /// DBG
   
   NT_ASSERT(pDPCData);
   NT_ASSERT(pClassifyData);
   NT_ASSERT(pPendData);

   pDPCData->pClassifyData = pClassifyData;
   pDPCData->pPendData     = pPendData;
   pDPCData->pContext      = pContext;

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprDPCDataPopulate()\n");

#endif /// DBG
   
   return;
}

/**
 @kernel_helper_function="KrnlHlprDPCDataPopulate"
 
   Purpose:  Populates a DPC_DATA object with the classifyData, redirectData, and context 
             supplied.                                                                          <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprDPCDataPopulate(_Inout_ DPC_DATA* pDPCData,
                                    _In_ CLASSIFY_DATA* pClassifyData,
                                    _In_ REDIRECT_DATA* pRedirectData,
                                    _In_opt_ VOID* pContext)           /* 0 */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprDPCDataPopulate()\n");

#endif /// DBG
   
   NT_ASSERT(pDPCData);
   NT_ASSERT(pClassifyData);
   NT_ASSERT(pRedirectData);

   pDPCData->pClassifyData = pClassifyData;
   pDPCData->pRedirectData = pRedirectData;
   pDPCData->pContext      = pContext;

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprDPCDataPopulate()\n");

#endif /// DBG
   
   return;
}

/**
 @kernel_helper_function="KrnlHlprDPCDataPopulate"
 
   Purpose:  Populates a DPC_DATA object with the notifyData and context supplied.              <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprDPCDataPopulate(_Inout_ DPC_DATA* pDPCData,
                                    _In_ NOTIFY_DATA* pNotifyData,
                                    _In_opt_ VOID* pContext)       /* 0 */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprDPCDataPopulate()\n");

#endif /// DBG
   
   NT_ASSERT(pDPCData);
   NT_ASSERT(pNotifyData);

   pDPCData->pNotifyData = pNotifyData;
   pDPCData->pContext    = pContext;

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprDPCDataPopulate()\n");

#endif /// DBG
   
   return;
}

/**
 @kernel_helper_function="KrnlHlprDPCDataPopulate"
 
   Purpose:  Populates a DPC_DATA object with the pendData, and context supplied.               <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprDPCDataPopulate(_Inout_ DPC_DATA* pDPCData,
                                    _In_ PEND_DATA* pPendData,
                                    _In_opt_ VOID* pContext)    /* 0 */
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprDPCDataPopulate()\n");

#endif /// DBG

   NT_ASSERT(pDPCData);
   NT_ASSERT(pPendData);

   pDPCData->pPendData     = pPendData;
   pDPCData->pContext      = pContext;

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprDPCDataPopulate()\n");

#endif /// DBG

   return;
}

/**
 @kernel_helper_function="KrnlHlprDPCDataCreate"
 
   Purpose:  Allocates and populates a DPC_DATA object with the KDPC, classifyData, 
             injectionData, and context supplied.                                               <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
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
                               _In_opt_ INJECTION_DATA* pInjectionData, /* 0 */
                               _In_opt_ VOID* pContext)                 /* 0 */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprDPCDataCreate()\n");

#endif /// DBG
   
   NT_ASSERT(ppDPCData);
   NT_ASSERT(pClassifyData);
   NT_ASSERT(pInjectionData);

   NTSTATUS status = STATUS_SUCCESS;

   HLPR_NEW(*ppDPCData,
            DPC_DATA,
            WFPSAMPLER_SYSLIB_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(*ppDPCData,
                              status);

   KrnlHlprDPCDataPopulate(*ppDPCData,
                           pClassifyData,
                           pInjectionData,
                           pContext);

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
      KrnlHlprDPCDataDestroy(ppDPCData);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprDPCDataCreate() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}

/**
 @kernel_helper_function="KrnlHlprDPCDataCreate"
 
   Purpose:  Allocates and populates a DPC_DATA object with the KDPC, classifyData, pendData, 
             and context supplied.                                                              <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
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
                               _In_opt_ VOID* pContext)                 /* 0 */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprDPCDataCreate()\n");

#endif /// DBG
   
   NT_ASSERT(ppDPCData);
   NT_ASSERT(pClassifyData);
   NT_ASSERT(pPendData);

   NTSTATUS status = STATUS_SUCCESS;

   HLPR_NEW(*ppDPCData,
            DPC_DATA,
            WFPSAMPLER_SYSLIB_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(*ppDPCData,
                              status);

   KrnlHlprDPCDataPopulate(*ppDPCData,
                           pClassifyData,
                           pPendData,
                           pContext);

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
      KrnlHlprDPCDataDestroy(ppDPCData);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprDPCDataCreate() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}

/**
 @kernel_helper_function="KrnlHlprDPCDataCreate"
 
   Purpose:  Allocates and populates a DPC_DATA object with the KDPC, classifyData, 
             redirectData, and context supplied.                                                <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
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
                               _In_opt_ VOID* pContext)                 /* 0 */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprDPCDataCreate()\n");

#endif /// DBG
   
   NT_ASSERT(ppDPCData);
   NT_ASSERT(pClassifyData);
   NT_ASSERT(pRedirectData);

   NTSTATUS status = STATUS_SUCCESS;

   HLPR_NEW(*ppDPCData,
            DPC_DATA,
            WFPSAMPLER_SYSLIB_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(*ppDPCData,
                              status);

   KrnlHlprDPCDataPopulate(*ppDPCData,
                           pClassifyData,
                           pRedirectData,
                           pContext);

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
      KrnlHlprDPCDataDestroy(ppDPCData);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprDPCDataCreate() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}

/**
 @kernel_helper_function="KrnlHlprDPCDataCreate"
 
   Purpose:  Allocates and populates a DPC_DATA object with the notifyData, KDPC, and context 
             supplied.                                                                          <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
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
                               _In_opt_ VOID* pContext)                 /* 0 */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprDPCDataCreate()\n");

#endif /// DBG
   
   NT_ASSERT(ppDPCData);
   NT_ASSERT(pNotifyData);

   NTSTATUS status = STATUS_SUCCESS;

   HLPR_NEW(*ppDPCData,
            DPC_DATA,
            WFPSAMPLER_SYSLIB_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(*ppDPCData,
                              status);

   KrnlHlprDPCDataPopulate(*ppDPCData,
                           pNotifyData,
                           pContext);

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
      KrnlHlprDPCDataDestroy(ppDPCData);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprDPCDataCreate() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}

/**
 @kernel_helper_function="KrnlHlprDPCDataCreate"
 
   Purpose:  Allocates and populates a DPC_DATA object with the KDPC, pendData, and context 
             supplied.                                                                          <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
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
                               _In_opt_ VOID* pContext)       /* 0 */
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprDPCDataCreate()\n");

#endif /// DBG

   NT_ASSERT(ppDPCData);
   NT_ASSERT(pPendData);

   NTSTATUS status = STATUS_SUCCESS;

   HLPR_NEW(*ppDPCData,
            DPC_DATA,
            WFPSAMPLER_SYSLIB_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(*ppDPCData,
                              status);

   KrnlHlprDPCDataPopulate(*ppDPCData,
                           pPendData,
                           pContext);

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
      KrnlHlprDPCDataDestroy(ppDPCData);

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprDPCDataCreate() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

/**
 @kernel_helper_function="KrnlHlprDPCQueue"
 
   Purpose:  Queue a DPC for later execution at DISPATCH_LEVEL.                                 <br>
                                                                                                <br>
   Notes:    Data is expected to be obtained by other means (i.e. LIST_ENTRY)                   <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552130.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552185.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprDPCQueue(_In_ KDEFERRED_ROUTINE* pDPCFn,
                          _In_ UINT32 processorNumber,    /* 0 */
                          _In_ BOOLEAN forceProcessor)    /* FALSE */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprDPCQueue()\n");

#endif /// DBG
   
   NT_ASSERT(pDPCFn);

   NTSTATUS  status   = STATUS_SUCCESS;
   DPC_DATA* pDPCData = 0;

   HLPR_NEW(pDPCData,
            DPC_DATA,
            WFPSAMPLER_SYSLIB_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pDPCData,
                              status);

   KeInitializeDpc(&(pDPCData->kdpc),
                   pDPCFn,
                   0);

   if(forceProcessor)
      KeSetTargetProcessorDpc(&(pDPCData->kdpc),
                              (CCHAR)processorNumber);

   KeInsertQueueDpc(&(pDPCData->kdpc),
                    pDPCData,
                    0);

   HLPR_BAIL_LABEL:

#pragma warning(push)
#pragma warning(disable: 6001) /// pDPCData initialized with call to HLPR_NEW 

   if(status != STATUS_SUCCESS &&
      pDPCData)
      KrnlHlprDPCDataDestroy(&pDPCData);

#pragma warning(pop)

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprDPCQueue() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}


/**
 @kernel_helper_function="KrnlHlprDPCQueue"
 
   Purpose:  Queue a DPC for later execution at DISPATCH_LEVEL.                                 <br>
                                                                                                <br>
   Notes:    INJECTION_DATA specific.                                                           <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552130.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552185.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprDPCQueue(_In_ KDEFERRED_ROUTINE* pDPCFn,
                          _In_ CLASSIFY_DATA* pClassifyData,
                          _In_opt_ INJECTION_DATA* pInjectionData, /* 0 */
                          _In_opt_ VOID* pContext,                 /* 0 */
                          _In_ UINT32 processorNumber,             /* 0 */
                          _In_ BOOLEAN forceProcessor)             /* FALSE */
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprDPCQueue()\n");

#endif /// DBG

   NT_ASSERT(pDPCFn);
   NT_ASSERT(pClassifyData);

   NTSTATUS  status   = STATUS_SUCCESS;
   DPC_DATA* pDPCData = 0;

   status = KrnlHlprDPCDataCreate(&pDPCData,
                                  pClassifyData,
                                  pInjectionData,
                                  pContext);
   HLPR_BAIL_ON_FAILURE(status);

   KeInitializeDpc(&(pDPCData->kdpc),
                   pDPCFn,
                   0);

   if(forceProcessor)
      KeSetTargetProcessorDpc(&(pDPCData->kdpc),
                              (CCHAR)processorNumber);

   KeInsertQueueDpc(&(pDPCData->kdpc),
                    pDPCData,
                    0);

   HLPR_BAIL_LABEL:

#pragma warning(push)
#pragma warning(disable: 6001) /// pDPCData initialized with call to KrnlHlprDPCDataCreate 

   if(status != STATUS_SUCCESS &&
      pDPCData)
      KrnlHlprDPCDataDestroy(&pDPCData);

#pragma warning(pop)

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprDPCQueue() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

/**
 @kernel_helper_function="KrnlHlprDPCQueue"
 
   Purpose:  Queue a DPC for later execution at DISPATCH_LEVEL.                                 <br>
                                                                                                <br>
   Notes:    PEND_DATA specific.                                                                <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552130.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552185.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprDPCQueue(_In_ KDEFERRED_ROUTINE* pDPCFn,
                          _In_ CLASSIFY_DATA* pClassifyData,
                          _In_ PEND_DATA* pPendData,
                          _In_opt_ VOID* pContext,           /* 0 */
                          _In_ UINT32 processorNumber,       /* 0 */
                          _In_ BOOLEAN forceProcessor)       /* FALSE */
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprDPCQueue()\n");

#endif /// DBG

   NT_ASSERT(pDPCFn);
   NT_ASSERT(pClassifyData);
   NT_ASSERT(pPendData);

   NTSTATUS  status   = STATUS_SUCCESS;
   DPC_DATA* pDPCData = 0;

   status = KrnlHlprDPCDataCreate(&pDPCData,
                                  pClassifyData,
                                  pPendData,
                                  pContext);
   HLPR_BAIL_ON_FAILURE(status);

   KeInitializeDpc(&(pDPCData->kdpc),
                   pDPCFn,
                   0);

   if(forceProcessor)
      KeSetTargetProcessorDpc(&(pDPCData->kdpc),
                              (CCHAR)processorNumber);

   KeInsertQueueDpc(&(pDPCData->kdpc),
                    pDPCData,
                    0);

   HLPR_BAIL_LABEL:

#pragma warning(push)
#pragma warning(disable: 6001) /// pDPCData initialized with call to KrnlHlprDPCDataCreate 

   if(status != STATUS_SUCCESS &&
      pDPCData)
      KrnlHlprDPCDataDestroy(&pDPCData);

#pragma warning(pop)

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprDPCQueue() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

/**
 @kernel_helper_function="KrnlHlprDPCQueue"
 
   Purpose:  Queue a DPC for later execution at DISPATCH_LEVEL.                                 <br>
                                                                                                <br>
   Notes:    REDIRECT_DATA specific.                                                            <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552130.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552185.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprDPCQueue(_In_ KDEFERRED_ROUTINE* pDPCFn,
                          _In_ CLASSIFY_DATA* pClassifyData,
                          _In_ REDIRECT_DATA* pRedirectData,
                          _In_opt_ VOID* pContext,           /* 0 */
                          _In_ UINT32 processorNumber,       /* 0 */
                          _In_ BOOLEAN forceProcessor)       /* FALSE */
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprDPCQueue()\n");

#endif /// DBG
   
   NT_ASSERT(pDPCFn);
   NT_ASSERT(pClassifyData);
   NT_ASSERT(pRedirectData);

   NTSTATUS  status   = STATUS_SUCCESS;
   DPC_DATA* pDPCData = 0;

   status = KrnlHlprDPCDataCreate(&pDPCData,
                                  pClassifyData,
                                  pRedirectData,
                                  pContext);
   HLPR_BAIL_ON_FAILURE(status);

   KeInitializeDpc(&(pDPCData->kdpc),
                   pDPCFn,
                   0);

   if(forceProcessor)
      KeSetTargetProcessorDpc(&(pDPCData->kdpc),
                              (CCHAR)processorNumber);

   KeInsertQueueDpc(&(pDPCData->kdpc),
                    pDPCData,
                    0);

   HLPR_BAIL_LABEL:

#pragma warning(push)
#pragma warning(disable: 6001) /// pDPCData initialized with call to KrnlHlprDPCDataCreate 
   
      if(status != STATUS_SUCCESS &&
         pDPCData)
         KrnlHlprDPCDataDestroy(&pDPCData);

#pragma warning(pop)

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprDPCQueue() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

/**
 @kernel_helper_function="KrnlHlprDPCQueue"
 
   Purpose:  Queue a DPC for later execution at DISPATCH_LEVEL.                                 <br>
                                                                                                <br>
   Notes:    NOTIFY_DATA specific.                                                              <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552130.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552185.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprDPCQueue(_In_ KDEFERRED_ROUTINE* pDPCFn,
                          _In_ NOTIFY_DATA* pNotifyData,
                          _In_opt_ VOID* pContext,        /* 0 */
                          _In_ UINT32 processorNumber,    /* 0 */
                          _In_ BOOLEAN forceProcessor)    /* FALSE */
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprDPCQueue()\n");

#endif /// DBG

   NT_ASSERT(pDPCFn);
   NT_ASSERT(pNotifyData);

   NTSTATUS  status   = STATUS_SUCCESS;
   DPC_DATA* pDPCData = 0;

   status = KrnlHlprDPCDataCreate(&pDPCData,
                                  pNotifyData,
                                  pContext);
   HLPR_BAIL_ON_FAILURE(status);

   KeInitializeDpc(&(pDPCData->kdpc),
                   pDPCFn,
                   0);

   if(forceProcessor)
      KeSetTargetProcessorDpc(&(pDPCData->kdpc),
                              (CCHAR)processorNumber);

   KeInsertQueueDpc(&(pDPCData->kdpc),
                    pDPCData,
                    0);

   HLPR_BAIL_LABEL:

#pragma warning(push)
#pragma warning(disable: 6001) /// pDPCData initialized with call to KrnlHlprDPCDataCreate

      if(status != STATUS_SUCCESS &&
         pDPCData)
         KrnlHlprDPCDataDestroy(&pDPCData);

#pragma warning(pop)

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprDPCQueue() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

/**
 @kernel_helper_function="KrnlHlprDPCQueue"
 
   Purpose:  Queue a DPC for later execution at DISPATCH_LEVEL.                                 <br>
                                                                                                <br>
   Notes:    PEND_DATA specific.                                                                <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552130.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552185.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprDPCQueue(_In_ KDEFERRED_ROUTINE* pDPCFn,
                          _In_ PEND_DATA* pPendData,
                          _In_opt_ VOID* pContext,        /* 0 */
                          _In_ UINT32 processorNumber,    /* 0 */
                          _In_ BOOLEAN forceProcessor)    /* FALSE */
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprDPCQueue()\n");

#endif /// DBG

   NT_ASSERT(pDPCFn);
   NT_ASSERT(pPendData);

   NTSTATUS  status   = STATUS_SUCCESS;
   DPC_DATA* pDPCData = 0;

   status = KrnlHlprDPCDataCreate(&pDPCData,
                                  pPendData,
                                  pContext);
   HLPR_BAIL_ON_FAILURE(status);

   KeInitializeDpc(&(pDPCData->kdpc),
                   pDPCFn,
                   0);

   if(forceProcessor)
      KeSetTargetProcessorDpc(&(pDPCData->kdpc),
                              (CCHAR)processorNumber);

   KeInsertQueueDpc(&(pDPCData->kdpc),
                    pDPCData,
                    0);

   HLPR_BAIL_LABEL:

#pragma warning(push)
#pragma warning(disable: 6001) /// pDPCData initialized with call to KrnlHlprDPCDataCreate 

   if(status != STATUS_SUCCESS &&
      pDPCData)
      KrnlHlprDPCDataDestroy(&pDPCData);

#pragma warning(pop)

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprDPCQueue() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

#endif /// DPC____

#ifndef THREADED_DPC____
#define THREADED_DPC____

/**
 @kernel_helper_function="KrnlHlprThreadedDPCQueue"
 
   Purpose:  Queue a threaded DPC for later execution at either PASSIVE_LEVEL or DISPATCH_LEVEL.<br>
                                                                                                <br>
   Notes:    Data is expected to be obtained by other means (i.e. LIST_ENTRY)                   <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552166.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552185.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprThreadedDPCQueue(_In_ KDEFERRED_ROUTINE* pDPCFn,
                                  _In_ UINT32 processorNumber,    /* 0 */
                                  _In_ BOOLEAN forceProcessor)    /* FALSE */
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprThreadedDPCQueue()\n");

#endif /// DBG

   NT_ASSERT(pDPCFn);

   NTSTATUS  status   = STATUS_SUCCESS;
   DPC_DATA* pDPCData = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pDPCData will be freed by caller

   HLPR_NEW(pDPCData,
            DPC_DATA,
            WFPSAMPLER_SYSLIB_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pDPCData,
                              status);

#pragma warning(pop)

   KeInitializeThreadedDpc(&(pDPCData->kdpc),
                           pDPCFn,
                           0);

   if(forceProcessor)
      KeSetTargetProcessorDpc(&(pDPCData->kdpc),
                              (CCHAR)processorNumber);

   KeInsertQueueDpc(&(pDPCData->kdpc),
                    pDPCData,
                    0);

   HLPR_BAIL_LABEL:

#pragma warning(push)
#pragma warning(disable: 6001) /// pDPCData initialized with call to HLPR_NEW 

   if(status != STATUS_SUCCESS &&
      pDPCData)
      KrnlHlprDPCDataDestroy(&pDPCData);

#pragma warning(pop)

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprThreadedDPCQueue() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}


/**
 @kernel_helper_function="KrnlHlprThreadedDPCQueue"
 
   Purpose:  Queue a threaded DPC for later execution at either PASSIVE_LEVEL or DISPATCH_LEVEL.<br>
                                                                                                <br>
   Notes:    INJECTION_DATA specific.                                                           <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552166.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552185.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprThreadedDPCQueue(_In_ KDEFERRED_ROUTINE* pDPCFn,
                                  _In_ CLASSIFY_DATA* pClassifyData,
                                  _In_opt_ INJECTION_DATA* pInjectionData, /* 0 */
                                  _In_opt_ VOID* pContext,                 /* 0 */
                                  _In_ UINT32 processorNumber,             /* 0 */
                                  _In_ BOOLEAN forceProcessor)             /* FALSE */
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprThreadedDPCQueue()\n");

#endif /// DBG

   NT_ASSERT(pDPCFn);
   NT_ASSERT(pClassifyData);

   NTSTATUS  status   = STATUS_SUCCESS;
   DPC_DATA* pDPCData = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pDPCData will be freed by caller

   status = KrnlHlprDPCDataCreate(&pDPCData,
                                  pClassifyData,
                                  pInjectionData,
                                  pContext);
   HLPR_BAIL_ON_FAILURE(status);

#pragma warning(pop)

   KeInitializeThreadedDpc(&(pDPCData->kdpc),
                           pDPCFn,
                           0);

   if(forceProcessor)
      KeSetTargetProcessorDpc(&(pDPCData->kdpc),
                              (CCHAR)processorNumber);

   KeInsertQueueDpc(&(pDPCData->kdpc),
                    pDPCData,
                    0);

   HLPR_BAIL_LABEL:

#pragma warning(push)
#pragma warning(disable: 6001) /// pDPCData initialized with call to KrnlHlprDPCDataCreate 

   if(status != STATUS_SUCCESS &&
      pDPCData)
      KrnlHlprDPCDataDestroy(&pDPCData);

#pragma warning(pop)

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprThreadedDPCQueue() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

/**
 @kernel_helper_function="KrnlHlprThreadedDPCQueue"
 
   Purpose:  Queue a threaded DPC for later execution at either PASSIVE_LEVEL or DISPATCH_LEVEL.<br>
                                                                                                <br>
   Notes:    PEND_DATA specific.                                                                <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552166.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552185.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprThreadedDPCQueue(_In_ KDEFERRED_ROUTINE* pDPCFn,
                                  _In_ CLASSIFY_DATA* pClassifyData,
                                  _In_ PEND_DATA* pPendData,
                                  _In_opt_ VOID* pContext,           /* 0 */
                                  _In_ UINT32 processorNumber,       /* 0 */
                                  _In_ BOOLEAN forceProcessor)       /* FALSE */
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprThreadedDPCQueue()\n");

#endif /// DBG

   NT_ASSERT(pDPCFn);
   NT_ASSERT(pClassifyData);
   NT_ASSERT(pPendData);

   NTSTATUS  status   = STATUS_SUCCESS;
   DPC_DATA* pDPCData = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pDPCData will be freed by caller

   status = KrnlHlprDPCDataCreate(&pDPCData,
                                  pClassifyData,
                                  pPendData,
                                  pContext);
   HLPR_BAIL_ON_FAILURE(status);

#pragma warning(pop)

   KeInitializeThreadedDpc(&(pDPCData->kdpc),
                           pDPCFn,
                           0);

   if(forceProcessor)
      KeSetTargetProcessorDpc(&(pDPCData->kdpc),
                              (CCHAR)processorNumber);

   KeInsertQueueDpc(&(pDPCData->kdpc),
                    pDPCData,
                    0);

   HLPR_BAIL_LABEL:

#pragma warning(push)
#pragma warning(disable: 6001) /// pDPCData initialized with call to KrnlHlprDPCDataCreate 

   if(status != STATUS_SUCCESS &&
      pDPCData)
      KrnlHlprDPCDataDestroy(&pDPCData);

#pragma warning(pop)

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprThreadedDPCQueue() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

/**
 @kernel_helper_function="KrnlHlprThreadedDPCQueue"
 
   Purpose:  Queue a threaded DPC for later execution at either PASSIVE_LEVEL or DISPATCH_LEVEL.<br>
                                                                                                <br>
   Notes:    REDIRECT_DATA specific.                                                            <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552166.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552185.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprThreadedDPCQueue(_In_ KDEFERRED_ROUTINE* pDPCFn,
                                  _In_ CLASSIFY_DATA* pClassifyData,
                                  _In_ REDIRECT_DATA* pRedirectData,
                                  _In_opt_ VOID* pContext,           /* 0 */
                                  _In_ UINT32 processorNumber,       /* 0 */
                                  _In_ BOOLEAN forceProcessor)       /* FALSE */

{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprThreadedDPCQueue()\n");

#endif /// DBG

   NT_ASSERT(pDPCFn);
   NT_ASSERT(pClassifyData);
   NT_ASSERT(pRedirectData);

   NTSTATUS  status   = STATUS_SUCCESS;
   DPC_DATA* pDPCData = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pDPCData will be freed by caller

   status = KrnlHlprDPCDataCreate(&pDPCData,
                                  pClassifyData,
                                  pRedirectData,
                                  pContext);
   HLPR_BAIL_ON_FAILURE(status);

#pragma warning(pop)

   KeInitializeThreadedDpc(&(pDPCData->kdpc),
                           pDPCFn,
                           0);

   if(forceProcessor)
      KeSetTargetProcessorDpc(&(pDPCData->kdpc),
                              (CCHAR)processorNumber);

   KeInsertQueueDpc(&(pDPCData->kdpc),
                    pDPCData,
                    0);

   HLPR_BAIL_LABEL:

#pragma warning(push)
#pragma warning(disable: 6001) /// pDPCData initialized with call to KrnlHlprDPCDataCreate

   if(status != STATUS_SUCCESS &&
      pDPCData)
      KrnlHlprDPCDataDestroy(&pDPCData);

#pragma warning(pop)

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprThreadedDPCQueue() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

/**
 @kernel_helper_function="KrnlHlprThreadedDPCQueue"
 
   Purpose:  Queue a threaded DPC for later execution at either PASSIVE_LEVEL or DISPATCH_LEVEL.<br>
                                                                                                <br>
   Notes:    NOTIFY_DATA specific.                                                              <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552166.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552185.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprThreadedDPCQueue(_In_ KDEFERRED_ROUTINE* pDPCFn,
                                  _In_ NOTIFY_DATA* pNotifyData,
                                  _In_opt_ VOID* pContext,        /* 0 */
                                  _In_ UINT32 processorNumber,    /* 0 */
                                  _In_ BOOLEAN forceProcessor)    /* FALSE */

{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprThreadedDPCQueue()\n");

#endif /// DBG

   NT_ASSERT(pDPCFn);
   NT_ASSERT(pNotifyData);

   NTSTATUS  status   = STATUS_SUCCESS;
   DPC_DATA* pDPCData = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pDPCData will be freed by caller

   status = KrnlHlprDPCDataCreate(&pDPCData,
                                  pNotifyData,
                                  pContext);
   HLPR_BAIL_ON_FAILURE(status);

#pragma warning(pop)

   KeInitializeThreadedDpc(&(pDPCData->kdpc),
                           pDPCFn,
                           0);

   if(forceProcessor)
      KeSetTargetProcessorDpc(&(pDPCData->kdpc),
                              (CCHAR)processorNumber);

   KeInsertQueueDpc(&(pDPCData->kdpc),
                    pDPCData,
                    0);

   HLPR_BAIL_LABEL:

#pragma warning(push)
#pragma warning(disable: 6001) /// pDPCData initialized with call to KrnlHlprDPCDataCreate

   if(status != STATUS_SUCCESS &&
      pDPCData)
      KrnlHlprDPCDataDestroy(&pDPCData);

#pragma warning(pop)

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprThreadedDPCQueue() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

/**
 @kernel_helper_function="KrnlHlprThreadedDPCQueue"
 
   Purpose:  Queue a threaded DPC for later execution at either PASSIVE_LEVEL or DISPATCH_LEVEL.<br>
                                                                                                <br>
   Notes:    PEND_DATA specific.                                                                <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552166.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552185.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprThreadedDPCQueue(_In_ KDEFERRED_ROUTINE* pDPCFn,
                                  _In_ PEND_DATA* pPendData,
                                  _In_opt_ VOID* pContext,        /* 0 */
                                  _In_ UINT32 processorNumber,    /* 0 */
                                  _In_ BOOLEAN forceProcessor)    /* FALSE */
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprThreadedDPCQueue()\n");

#endif /// DBG

   NT_ASSERT(pDPCFn);
   NT_ASSERT(pPendData);

   NTSTATUS  status   = STATUS_SUCCESS;
   DPC_DATA* pDPCData = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pDPCData will be freed by caller

   status = KrnlHlprDPCDataCreate(&pDPCData,
                                  pPendData,
                                  pContext);
   HLPR_BAIL_ON_FAILURE(status);

#pragma warning(pop)

   KeInitializeThreadedDpc(&(pDPCData->kdpc),
                           pDPCFn,
                           0);

   if(forceProcessor)
      KeSetTargetProcessorDpc(&(pDPCData->kdpc),
                              (CCHAR)processorNumber);

   KeInsertQueueDpc(&(pDPCData->kdpc),
                    pDPCData,
                    0);

   HLPR_BAIL_LABEL:

#pragma warning(push)
#pragma warning(disable: 6001) /// pDPCData initialized with call to KrnlHlprDPCDataCreate 

   if(status != STATUS_SUCCESS &&
      pDPCData)
      KrnlHlprDPCDataDestroy(&pDPCData);

#pragma warning(pop)

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprThreadedDPCQueue() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

#endif /// THREADED_DPC____
