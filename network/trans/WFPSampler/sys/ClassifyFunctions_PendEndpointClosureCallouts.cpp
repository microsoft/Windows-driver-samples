////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      ClassifyFunctions_PendEndpointClosureCallouts.cpp
//
//   Abstract:
//      This module contains WFP Classify functions for pending and completing endpoint closures.
//
//   Naming Convention:
//
//      <Module><Scenario>
//  
//      i.e.
//
//       ClassifyPendEndpointClosure
//
//       <Module>
//          Classify               -       Function is an FWPS_CALLOUT_CLASSIFY_FN.
//          Perform                -       Function executes the desired scenario.
//          Trigger                -       Function queues a worker thread for later execution of 
//                                            the the scenario.
//       <Scenario>
//          PendEndpointClosure    -       Function demonstates pending endpoint closure requests.
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Private Functions:
//      PendEndpointClosureWorkItemRoutine(),
//      PerformPendEndpointClosure(),
//      TriggerPendEndpointClosureOutOfBand(),
//
//   Public Functions:
//      PendEndpointClosureDeferredProcedureCall(),
//      ClassifyPendEndpointClosure(),
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      December  13,   2013  -     1.1   -  Creation
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Framework_WFPSamplerCalloutDriver.h"               /// .
#include "ClassifyFunctions_PendEndpointClosureCallouts.tmh" /// $(OBJ_PATH)\$(O)\ 

#if(NTDDI_VERSION >= NTDDI_WIN7)

#pragma warning(push)
#pragma warning(disable: 6101) /// *ppPendData are freed and set to 0 on successful completion

/**
 @private_function="PerformPendEndpointClosure"
 
   Purpose:  Waits for the specified delay, then completes the pended classify, and frees the 
             memory.                                                                            <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return == STATUS_SUCCESS)
NTSTATUS PerformPendEndpointClosure(_Inout_ PEND_DATA** ppPendData)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> PerformPendEndpointClosure()\n");

#endif /// DBG
   
   NT_ASSERT(*ppPendData);
   NT_ASSERT((*ppPendData)->pPendEndpointClosureData);

   NTSTATUS status = STATUS_SUCCESS;
   UINT32   delay  = (*ppPendData)->pPendEndpointClosureData->delay;

   if(delay &&
      KeGetCurrentIrql() < DISPATCH_LEVEL)
   {

#pragma warning(push)
#pragma warning(disable: 28118) /// IRQL check has already been performed

      KrnlHlprWorkItemSleep(delay);

#pragma warning(pop)

      }

#pragma warning(push)
#pragma warning(disable: 6001) /// *ppPendData initialized prior to call to this function

   /// Completes the Pend
   KrnlHlprPendDataDestroy(ppPendData);

#pragma warning(pop)

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- PerformPendEndpointClosure() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
};

#pragma warning(pop)

/**
 @private_function="PendEndpointClosureDeferredProcedureCall"
 
   Purpose:  Invokes the appropriate private routine to complete the pended classify.           <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF542972.aspx             <br>
*/
_IRQL_requires_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Function_class_(KDEFERRED_ROUTINE)
VOID PendEndpointClosureDeferredProcedureCall(_In_ KDPC* pDPC,
                                              _In_opt_ PVOID pContext,
                                              _In_opt_ PVOID pArg1,
                                              _In_opt_ PVOID pArg2)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> PendEndpointClosureDeferredProcedureCall()\n");

#endif /// DBG
   
   UNREFERENCED_PARAMETER(pDPC);
   UNREFERENCED_PARAMETER(pContext);
   UNREFERENCED_PARAMETER(pArg2);

   NT_ASSERT(pDPC);
   NT_ASSERT(pArg1);
   NT_ASSERT(((DPC_DATA*)pArg1)->pPendData);

   DPC_DATA* pDPCData = (DPC_DATA*)pArg1;

   if(pDPCData)
   {
      NTSTATUS status = STATUS_SUCCESS;

      status = PerformPendEndpointClosure(&(pDPCData->pPendData));
      if(status != STATUS_SUCCESS)
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_ERROR_LEVEL,
                    " !!!! PendEndpointClosureDeferredProcedureCall() [status: %#x]\n",
                    status);

      KrnlHlprDPCDataDestroy(&pDPCData);
   }

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- PendEndpointClosureDeferredProcedureCall()\n");

#endif /// DBG
   
   return;
}

/**
 @private_function="PendEndpointClosureWorkItemRoutine"
 
   Purpose:  Invokes the appropriate private routine to complete the pended classify at 
             PASSIVE_LEVEL.                                                                     <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF566380.aspx             <br>
*/
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Function_class_(IO_WORKITEM_ROUTINE)
VOID PendEndpointClosureWorkItemRoutine(_In_ PDEVICE_OBJECT pDeviceObject,
                                        _In_opt_ PVOID pContext)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> PendEndpointClosureWorkItemRoutine()\n");

#endif /// DBG
   
   UNREFERENCED_PARAMETER(pDeviceObject);

   NT_ASSERT(pContext);
   NT_ASSERT(((WORKITEM_DATA*)pContext)->pPendData);

   WORKITEM_DATA* pWorkItemData = (WORKITEM_DATA*)pContext;

   if(pWorkItemData)
   {
      NTSTATUS status = STATUS_SUCCESS;

      status = PerformPendEndpointClosure(&(pWorkItemData->pPendData));
      if(status != STATUS_SUCCESS)
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_ERROR_LEVEL,
                    " !!!! PendEndpointClosureWorkItemRoutine() [status: %#x]\n",
                    status);

      KrnlHlprWorkItemDataDestroy(&pWorkItemData);
   }

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- PendEndpointClosureWorkItemRoutine()\n");

#endif /// DBG
   
   return;
}

/**
 @private_function="TriggerPendEndpointClosure"
 
   Purpose:                                                                                     <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return == STATUS_SUCCESS)
NTSTATUS TriggerPendEndpointClosure(_Inout_ PEND_DATA* pPendData,
                                    _In_ PC_PEND_ENDPOINT_CLOSURE_DATA* pPCData)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> TriggerPendEndpointClosure()\n");

#endif /// DBG
   
   NT_ASSERT(pPendData);
   NT_ASSERT(pPCData);
 
   NTSTATUS status        = STATUS_SUCCESS;

   if(pPCData->useWorkItems ||
      pPCData->delay)
   {
      /// introducing the delay requires PASSIVE_LEVEL, so force use of a Work Item
      status = KrnlHlprWorkItemQueue(g_pWDMDevice,
                                     PendEndpointClosureWorkItemRoutine,
                                     pPendData);
   }
   else
   {
      if(pPCData->useThreadedDPC)
         status = KrnlHlprThreadedDPCQueue(PendEndpointClosureDeferredProcedureCall,
                                           pPendData);
      else
         status = KrnlHlprDPCQueue(PendEndpointClosureDeferredProcedureCall,
                                   pPendData);
   }

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- TriggerPendEndpointClosure() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

/**
 @classify_function="ClassifyPendEndpointClosure"
 
   Purpose:  Classify Function which will pend an endpoint closure request.  If there is 
             appropriate flowContext, te request will be pended, and then exit, requiring a call
             to the FlowDeleteFn to perform the pend completion.  Otherwise a worker thread is 
             queued, and a will wait for the specified milliseconds before the pend is 
             completed.                                                                         <br>
                                                                                                <br>
   Notes:    Applies to the following layers:                                                   <br>
                FWPM_LAYER_ALE_ENDPOINT_CLOSURE_V{4/6}                                          <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF551229.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF544893.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI ClassifyPendEndpointClosure(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                       _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                       _Inout_opt_ VOID* pLayerData,
                                       _In_opt_ const VOID* pClassifyContext,
                                       _In_ const FWPS_FILTER* pFilter,
                                       _In_ UINT64 flowContext,
                                       _Inout_ FWPS_CLASSIFY_OUT* pClassifyOut)
{
   NT_ASSERT(pClassifyValues);
   NT_ASSERT(pMetadata);
   NT_ASSERT(pFilter);
   NT_ASSERT(pClassifyOut);
   NT_ASSERT(pClassifyValues->layerId == FWPS_LAYER_ALE_ENDPOINT_CLOSURE_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_ENDPOINT_CLOSURE_V6);
   NT_ASSERT(pFilter->providerContext);
   NT_ASSERT(pFilter->providerContext->type == FWPM_GENERAL_CONTEXT);
   NT_ASSERT(pFilter->providerContext->dataBuffer);
   NT_ASSERT(pFilter->providerContext->dataBuffer->size == sizeof(PC_PEND_ENDPOINT_CLOSURE_DATA));
   NT_ASSERT(pFilter->providerContext->dataBuffer->data);

   UNREFERENCED_PARAMETER(pLayerData);

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> ClassifyPendEndpointClosure() [Layer: %s][FilterID: %#I64x][FlowContext: %#I64x][Rights: %#x]",
              KrnlHlprFwpsLayerIDToString(pClassifyValues->layerId),
              pFilter->filterId,
              flowContext,
              pClassifyOut->rights);

   NTSTATUS   status    = STATUS_SUCCESS;
   PEND_DATA* pPendData = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pPendData will be freed in PerformPendEndpointClosure

   status = KrnlHlprPendDataCreate(&pPendData,
                                   pClassifyValues,
                                   pMetadata,
                                   0,
                                   pFilter,
                                   (VOID*)pClassifyContext,
                                   pClassifyOut);
   HLPR_BAIL_ON_FAILURE(status);

#pragma warning(pop)

   if(flowContext)
   {
      NT_ASSERT(flowContext);
      NT_ASSERT(((FLOW_CONTEXT*)flowContext)->contextType == CONTEXT_TYPE_ALE_ENDPOINT_CLOSURE);
      NT_ASSERT(((FLOW_CONTEXT*)flowContext)->pALEEndpointClosureContext);

      FLOW_CONTEXT* pFlowContext = (FLOW_CONTEXT*)flowContext;
      KIRQL         irql         = PASSIVE_LEVEL;

      KeAcquireSpinLock(&(pFlowContext->pALEEndpointClosureContext->spinLock),
                        &irql);

      /// NotifyFlowDeleteNotification will free this memory
      pFlowContext->pALEEndpointClosureContext->pPendData = pPendData;
      pFlowContext->pALEEndpointClosureContext->filterID  = pFilter->filterId;

      KeReleaseSpinLock(&(pFlowContext->pALEEndpointClosureContext->spinLock),
                        irql);
   }
   else
   {
      PC_PEND_ENDPOINT_CLOSURE_DATA* pPendEndpointClosureData = (PC_PEND_ENDPOINT_CLOSURE_DATA*)pFilter->providerContext->dataBuffer->data;

      status = TriggerPendEndpointClosure(pPendData,
                                          pPendEndpointClosureData);
   }

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
   {
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! ClassifyPendEndpointClosure() [status: %#x]\n",
                 status);

      KrnlHlprPendDataDestroy(&pPendData);
   }

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- ClassifyPendEndpointClosure() [Layer: %s][FilterID: %#I64x][Action: %#x][Rights: %#x][Absorb: %s]\n",
              KrnlHlprFwpsLayerIDToString(pClassifyValues->layerId),
              pFilter->filterId,
              pClassifyOut->actionType,
              pClassifyOut->rights,
              (pClassifyOut->flags & FWPS_CLASSIFY_OUT_FLAG_ABSORB) ? "TRUE" : "FALSE");

   return;
}

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)
