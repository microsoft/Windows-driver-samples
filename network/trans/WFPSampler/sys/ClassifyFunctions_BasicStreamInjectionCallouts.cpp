////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      ClassifyFunctions_BasicStreamInjectionCallouts.cpp
//
//   Abstract:
//      This module contains WFP Classify functions for injecting data back into TCP's stream using 
//         the clone / block / inject method.
//
//   Naming Convention:
//
//      <Module><Scenario>
//  
//      i.e.
//
//       ClassifyBasicStreamInjection
//
//       <Module>
//          Classify             -       Function is an FWPS_CALLOUT_CLASSIFY_FN
//       <Scenario>
//          BasicStreamInjection -       Function demonstates use clone / block / inject model for 
//                                          Stream
//
//      <Action><Scenario><Modifier>
//
//      i.e.
//
//       TriggerBasicStreamInjectionOutOfBand
//
//       <Action>
//        {
//                               -
//          Trigger              -       Initiates the desired scenario
//          Perform              -       Executes the desired scenario
//        }
//       <Scenario>
//          BasicStreamInjection -       Function demonstates use clone / block / inject model
//       <Modifier>
//          WorkItemRoutine      -       WorkItem Routine for Out of Band Injection.
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Enhance function declaration for IntelliSense, enhance 
//                                              traces, fix serialization, and add support for 
//                                              multiple injectors and flowContext
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Framework_WFPSamplerCalloutDriver.h"                /// .
#include "ClassifyFunctions_BasicStreamInjectionCallouts.tmh" /// $(OBJ_PATH)\$(O)\ 

/**
 @private_function="PerformBasicPacketInjectionAtOutboundTransport"
 
   Purpose:  Clones the stream data and injects the clone back to the stream using 
             FwpsStreamInjectAsync().                                                           <br>
                                                                                                <br>
   Notes:    Applies to the following layers:                                                   <br>
                FWPM_LAYER_STREAM_V{4/6}                                                        <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/FF551188.aspx                              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/FF546324.aspx                              <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
NTSTATUS PerformBasicStreamInjection(_In_ CLASSIFY_DATA** ppClassifyData,
                                     _In_ INJECTION_DATA** ppInjectionData,
                                     _In_ BOOLEAN isInline = FALSE)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> PerformBasicStreamInjection()\n");

#endif /// DBG

   NT_ASSERT(ppClassifyData);
   NT_ASSERT(ppInjectionData);
   NT_ASSERT(*ppClassifyData);
   NT_ASSERT(*ppInjectionData);

   NTSTATUS                                status                 = STATUS_SUCCESS;
   FWPS_INCOMING_VALUES*                   pClassifyValues        = (FWPS_INCOMING_VALUES*)(*ppClassifyData)->pClassifyValues;
   FWPS_INCOMING_METADATA_VALUES*          pMetadata              = (FWPS_INCOMING_METADATA_VALUES*)(*ppClassifyData)->pMetadataValues;
   FWPS_FILTER*                            pFilter                = (FWPS_FILTER*)(*ppClassifyData)->pFilter;
   FWPS_STREAM_CALLOUT_IO_PACKET*          pStreamCalloutIOPacket = (FWPS_STREAM_CALLOUT_IO_PACKET*)(*ppClassifyData)->pPacket;
   UINT64                                  flowID                 = 0;
   UINT32                                  streamFlags            = pStreamCalloutIOPacket->streamData->flags;
   NET_BUFFER_LIST*                        pNetBufferListChain    = 0;
   SIZE_T                                  dataLength             = pStreamCalloutIOPacket->streamData->dataLength;
   BASIC_STREAM_INJECTION_COMPLETION_DATA* pCompletionData        = 0;
   KIRQL                                   irql                   = KeGetCurrentIrql();

#if(NTDDI_VERSION >= NTDDI_WIN7)

   FLOW_CONTEXT*                           pFlowContext           = (FLOW_CONTEXT*)(*ppClassifyData)->flowContext;

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

#pragma warning(push)
#pragma warning(disable: 6014) /// pCompletionData will be freed in completionFn using BasicStreamInjectionCompletionDataDestroy
   
      HLPR_NEW(pCompletionData,
               BASIC_STREAM_INJECTION_COMPLETION_DATA,
               WFPSAMPLER_CALLOUT_DRIVER_TAG);
      HLPR_BAIL_ON_ALLOC_FAILURE(pCompletionData,
                                 status);
   
#pragma warning(pop)

   KeInitializeSpinLock(&(pCompletionData->spinLock));

   pCompletionData->performedInline = isInline;
   pCompletionData->pClassifyData   = *ppClassifyData;
   pCompletionData->pInjectionData  = *ppInjectionData;

   /// Responsibility for freeing this memory has been transferred to the pCompletionData
   *ppClassifyData = 0;

   *ppInjectionData = 0;

   if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                     FWPS_METADATA_FIELD_FLOW_HANDLE))
      flowID = pMetadata->flowHandle;

   if(!(streamFlags & FWPS_STREAM_FLAG_RECEIVE) &&
      !(streamFlags & FWPS_STREAM_FLAG_SEND))
      streamFlags |= pCompletionData->pInjectionData->direction ? FWPS_STREAM_FLAG_RECEIVE : FWPS_STREAM_FLAG_SEND;

   pStreamCalloutIOPacket->countBytesRequired = 0;
   pStreamCalloutIOPacket->countBytesEnforced = pStreamCalloutIOPacket->streamData->dataLength;
   pStreamCalloutIOPacket->streamAction       = FWPS_STREAM_ACTION_NONE;

   status = FwpsCloneStreamData(pStreamCalloutIOPacket->streamData,
                                g_pNDISPoolData->nblPoolHandle,
                                g_pNDISPoolData->nbPoolHandle,
                                0,
                                &pNetBufferListChain);
   if(status != STATUS_SUCCESS)
   {
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! PerformBasicStreamInjection : FwpsCloneStreamData() [status: %#x]\n",
                 status);

      HLPR_BAIL;
   }

   pCompletionData->refCount = KrnlHlprNBLGetRequiredRefCount(pNetBufferListChain,
                                                              TRUE);

   status = FwpsStreamInjectAsync(pCompletionData->pInjectionData->injectionHandle,
                                  pCompletionData->pInjectionData->injectionContext,
                                  0,
                                  flowID,
                                  pFilter->action.calloutId,
                                  pClassifyValues->layerId,
                                  streamFlags,
                                  pNetBufferListChain,
                                  dataLength,
                                  CompleteBasicStreamInjection,
                                  pCompletionData);

   NT_ASSERT(irql == KeGetCurrentIrql());

   if(status != STATUS_SUCCESS)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! PerformBasicStreamInjection : FwpsStreamInjectAsync() [status: %#x]\n",
                 status);

#if(NTDDI_VERSION >= NTDDI_WIN7)

   /// If the enpoint has been pended, this will cause the FlowDeleteFn to get invoked on the
   /// PEND_ENDPOINT_CLOSURE callout, which will unpend the closure, and free its flow context.
   if(streamFlags & FWPS_STREAM_FLAG_RECEIVE_DISCONNECT || /// FIN
      streamFlags & FWPS_STREAM_FLAG_RECEIVE_ABORT ||      /// RST
      streamFlags & FWPS_STREAM_FLAG_SEND_ABORT)           /// RST
   {
      if(pFlowContext &&
         pFlowContext->aecLayerID)
      {
         NTSTATUS tempStatus = STATUS_SUCCESS;

         tempStatus = FwpsFlowRemoveContext(pFlowContext->flowID,
                                            pFlowContext->aecLayerID,
                                            pFlowContext->aecCalloutID);
         if(status != STATUS_SUCCESS)
            DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                       DPFLTR_ERROR_LEVEL,
                       " !!!! PerformBasicStreamInjection : FwpsFlowRemoveContext() [status: %#x]\n",
                       tempStatus);
      }
   }

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)


   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
   {
      if(pNetBufferListChain)
      {
         FwpsDiscardClonedStreamData(pNetBufferListChain,
                                     0,
                                     irql == DISPATCH_LEVEL ? TRUE : FALSE);

         pNetBufferListChain = 0;
      }

      if(pCompletionData)
         BasicStreamInjectionCompletionDataDestroy(&pCompletionData,
                                                   TRUE);
   }

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- PerformBasicStreamInjection() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

/**
 @private_function="BasicStreamInjectionDeferredProcedureCall"
 
   Purpose:  Invokes the appropriate private injection routine to perform the injection at 
             DISPATCH_LEVEL.                                                                    <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF542972.aspx             <br>
*/
_IRQL_requires_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Function_class_(KDEFERRED_ROUTINE)
VOID BasicStreamInjectionDeferredProcedureCall(_In_ KDPC* pDPC,
                                               _In_opt_ PVOID pContext,
                                               _In_opt_ PVOID pArg1,
                                               _In_opt_ PVOID pArg2)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> BasicStreamInjectionDeferredProcedureCall()\n");

#endif /// DBG

   UNREFERENCED_PARAMETER(pDPC);
   UNREFERENCED_PARAMETER(pContext);
   UNREFERENCED_PARAMETER(pArg2);

   NT_ASSERT(pDPC);
   NT_ASSERT(pArg1);

   DPC_DATA* pDPCData = (DPC_DATA*)pArg1;

   if(pDPCData)
   {
      NTSTATUS     status      = STATUS_SUCCESS;
      KSPIN_LOCK*  pSpinLock   = &(g_bsiSerializationList.spinLock);
      LIST_ENTRY*  pListHead   = &(g_bsiSerializationList.listHead);
      LIST_ENTRY*  pListItem   = 0;
      INT64*       pNumEntries = &(g_bsiSerializationList.numEntries);

      /// Seriailze the Stream injection to prevent data corruption
      if(!IsListEmpty(pListHead))
      {
         pListItem = ExInterlockedRemoveHeadList(pListHead,
                                                 pSpinLock);
      
         InterlockedDecrement64(pNumEntries);
      }

      if(pListItem)
      {
         BASIC_STREAM_INJECTION_LIST_ITEM* pBSIListItem = (BASIC_STREAM_INJECTION_LIST_ITEM*)pListItem;

         status = PerformBasicStreamInjection(&(pBSIListItem->pClassifyData),
                                              &(pBSIListItem->pInjectionData),
                                              FALSE);
         if(status != STATUS_SUCCESS)
         {
            if(pBSIListItem->pClassifyData)
               KrnlHlprClassifyDataDestroyLocalCopy(&(pBSIListItem->pClassifyData));

            if(pBSIListItem->pInjectionData)
               KrnlHlprInjectionDataDestroy(&(pBSIListItem->pInjectionData));

            DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                       DPFLTR_ERROR_LEVEL,
                       "  !!!! BasicStreamInjectionDeferredProcedureCall() [status: %#x]\n",
                       status);
         }

         HLPR_DELETE(pBSIListItem,
                     WFPSAMPLER_SYSLIB_TAG);
      }

      KrnlHlprDPCDataDestroy(&pDPCData);
   }

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- BasicStreamInjectionDeferredProcedureCall()\n");

#endif /// DBG

   return;
}

/**
 @private_function="BasicStreamInjectionWorkItemRoutine"
 
   Purpose:  Invokes the private stream injection routine to perform the injection at 
             PASSIVE_LEVEL.                                                                     <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/FF566380.aspx                              <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Function_class_(IO_WORKITEM_ROUTINE)
VOID BasicStreamInjectionWorkItemRoutine(_In_ PDEVICE_OBJECT pDeviceObject,
                                         _Inout_opt_ PVOID pContext)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> BasicStreamInjectionWorkItemRoutine()\n");

#endif /// DBG

   UNREFERENCED_PARAMETER(pDeviceObject);

   NT_ASSERT(pContext);
   NT_ASSERT(((WORKITEM_DATA*)pContext)->pClassifyData);
   NT_ASSERT(((WORKITEM_DATA*)pContext)->pInjectionData);

   NTSTATUS       status            = STATUS_SUCCESS;
   WORKITEM_DATA* pWorkItemData     = (WORKITEM_DATA*)pContext;
   KSPIN_LOCK*    pSpinLock         = &(g_bsiSerializationList.spinLock);
   WDFWAITLOCK*   pWaitLock         = &(g_bsiSerializationList.waitLock);
   LIST_ENTRY*    pOriginalListHead = &(g_bsiSerializationList.listHead);
   INT64*         pNumEntries       = &(g_bsiSerializationList.numEntries);
   KIRQL          irql              = PASSIVE_LEVEL;
   LIST_ENTRY*    pListItem         = 0;
   LIST_ENTRY     listHead          = {0};

   if(pWorkItemData->pClassifyData->flowContext)
   {
      FLOW_CONTEXT* pFlowContext = (FLOW_CONTEXT*)pWorkItemData->pClassifyData->flowContext;

      if(pFlowContext->contextType == CONTEXT_TYPE_STREAM &&
         pFlowContext->pStreamContext)
      {
         pSpinLock = &(pFlowContext->pStreamContext->serializationList.spinLock);

         pOriginalListHead = &(pFlowContext->pStreamContext->serializationList.listHead);

         pNumEntries = &(pFlowContext->pStreamContext->serializationList.numEntries);
      }
   }

   /// Seriailze the Stream injection to prevent data corruption
   WdfWaitLockAcquire(*pWaitLock,
                      0);

   /// Flush the queue
   KeAcquireSpinLock(pSpinLock,
                     &irql);

   if(!IsListEmpty(pOriginalListHead))
   {
      LIST_ENTRY* pListEntry = 0;

      pListEntry = pOriginalListHead->Flink;

      RemoveEntryList(pOriginalListHead);

      InitializeListHead(pOriginalListHead);

      InitializeListHead(&listHead);

      AppendTailList(&listHead,
                     pListEntry);
   }

   *pNumEntries = 0;

   KeReleaseSpinLock(pSpinLock,
                     irql);

   /// Inject all items from the queue until empty
   for(;
       !IsListEmpty(&listHead);
       )
   {
      pListItem = ExInterlockedRemoveHeadList(&listHead,
                                              pSpinLock);

      if(pListItem)
      {
         BASIC_STREAM_INJECTION_LIST_ITEM* pBSIListItem = (BASIC_STREAM_INJECTION_LIST_ITEM*)pListItem;

         status = PerformBasicStreamInjection(&(pBSIListItem->pClassifyData),
                                              &(pBSIListItem->pInjectionData),
                                              FALSE);
         if(status != STATUS_SUCCESS)
            DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                       DPFLTR_ERROR_LEVEL,
                       "  !!!! BasicStreamInjectionWorkItemRoutine() [status: %#x]\n",
                       status);

         HLPR_DELETE(pBSIListItem,
                     WFPSAMPLER_SYSLIB_TAG);
      }
   }

   WdfWaitLockRelease(*pWaitLock);

   KrnlHlprWorkItemDataDestroy(&pWorkItemData);

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- BasicStreamInjectionWorkItemRoutine()\n");

#endif /// DBG

   return;
}

/**
 @private_function="TriggerBasicPacketInjectionInline"
 
   Purpose:  Makes a reference to all the classification data structures and invokes the 
             private stream injection routine to perform the injection.                         <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
NTSTATUS TriggerBasicStreamInjectionInline(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                           _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                           _Inout_opt_ VOID* pStreamCalloutIOPacket,
                                           _In_opt_ const VOID* pClassifyContext,
                                           _In_ const FWPS_FILTER* pFilter,
                                           _In_ UINT64 flowContext,
                                           _Inout_ FWPS_CLASSIFY_OUT* pClassifyOut,
                                           _In_ INJECTION_DATA** ppInjectionData)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> TriggerBasicStreamInjectionInline()\n");

#endif /// DBG

   NT_ASSERT(pClassifyValues);
   NT_ASSERT(pMetadata);
   NT_ASSERT(pStreamCalloutIOPacket);
   NT_ASSERT(pFilter);
   NT_ASSERT(pClassifyOut);
   NT_ASSERT(ppInjectionData);
   NT_ASSERT(*ppInjectionData);

   NTSTATUS       status        = STATUS_SUCCESS;
   CLASSIFY_DATA* pClassifyData = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pClassifyData will be freed in completionFn using BasicStreamInjectionCompletionDataDestroy

   HLPR_NEW(pClassifyData,
            CLASSIFY_DATA,
            WFPSAMPLER_CALLOUT_DRIVER_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pClassifyData,
                              status);

#pragma warning(pop)

   pClassifyData->pClassifyValues  = pClassifyValues;
   pClassifyData->pMetadataValues  = pMetadata;
   pClassifyData->pPacket          = pStreamCalloutIOPacket;
   pClassifyData->pClassifyContext = pClassifyContext;
   pClassifyData->pFilter          = pFilter;
   pClassifyData->flowContext      = flowContext;
   pClassifyData->pClassifyOut     = pClassifyOut;

   /// Do not touch the rights in case other callouts use them (which they shouldn't).
   pClassifyOut->actionType  = FWP_ACTION_BLOCK;

   status = PerformBasicStreamInjection(&pClassifyData,
                                        ppInjectionData,
                                        TRUE);

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
   {
      if(pClassifyData)
         KrnlHlprClassifyDataDestroyLocalCopy(&pClassifyData);
   }

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- TriggerBasicStreamInjectionInline() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

/**
 @private_function="TriggerBasicStreamInjectionOutOfBand"
 
   Purpose:  Creates a local copy of the classification data structures and queues a WorkItem 
             to perform the injection at PASSIVE_LEVEL.                                         <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/FF550679.aspx                              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/FF566380.aspx                              <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
NTSTATUS TriggerBasicStreamInjectionOutOfBand(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                              _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                              _Inout_opt_ VOID* pStreamCalloutIOPacket,
                                              _In_opt_ const VOID* pClassifyContext,
                                              _In_ const FWPS_FILTER* pFilter,
                                              _In_ UINT64 flowContext,
                                              _In_ FWPS_CLASSIFY_OUT* pClassifyOut,
                                              _Inout_ INJECTION_DATA* pInjectionData,
                                              _In_ PC_BASIC_STREAM_INJECTION_DATA* pPCData)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> TriggerBasicStreamInjectionOutOfBand()\n");

#endif /// DBG

   NT_ASSERT(pClassifyValues);
   NT_ASSERT(pMetadata);
   NT_ASSERT(pStreamCalloutIOPacket);
   NT_ASSERT(pFilter);
   NT_ASSERT(pClassifyOut);
   NT_ASSERT(pInjectionData);
   NT_ASSERT(pPCData);

   NTSTATUS                          status          = STATUS_SUCCESS;
   CLASSIFY_DATA*                    pClassifyData   = 0;
   BASIC_STREAM_INJECTION_LIST_ITEM* pBSIListItem    = 0;
   UINT32                            processorNumber = KeGetCurrentProcessorNumber();
   KSPIN_LOCK*                       pSpinLock       = &(g_bsiSerializationList.spinLock);
   KIRQL                             irql            = PASSIVE_LEVEL;
   LIST_ENTRY*                       pListHead       = &(g_bsiSerializationList.listHead);
   INT64*                            pNumEntries     = &(g_bsiSerializationList.numEntries);
   LIST_ENTRY*                       pLastEntry      = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pClassifyData will be freed in completionFn using BasicStreamInjectionCompletionDataDestroy

   status = KrnlHlprClassifyDataCreateLocalCopy(&pClassifyData,
                                                pClassifyValues,
                                                pMetadata,
                                                pStreamCalloutIOPacket,
                                                pClassifyContext,
                                                pFilter,
                                                flowContext,
                                                pClassifyOut);
   HLPR_BAIL_ON_FAILURE(status);

#pragma warning(pop)

   if(pClassifyData->flowContext)
   {
      FLOW_CONTEXT* pFlowContext = (FLOW_CONTEXT*)pClassifyData->flowContext;

      if(pFlowContext->contextType == CONTEXT_TYPE_STREAM &&
         pFlowContext->pStreamContext)
      {
         pFlowContext->pStreamContext->filterID = pFilter->filterId;

         processorNumber = pFlowContext->pStreamContext->processorID;

         if(pPCData->useWorkItems)
         {
            pSpinLock = &(pFlowContext->pStreamContext->serializationList.spinLock);

            pListHead = &(pFlowContext->pStreamContext->serializationList.listHead);

            pNumEntries = &(pFlowContext->pStreamContext->serializationList.numEntries);
         }
      }
   }

   /// Seriailze the Stream injection to prevent data corruption
   HLPR_NEW(pBSIListItem,
            BASIC_STREAM_INJECTION_LIST_ITEM,
            WFPSAMPLER_SYSLIB_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pBSIListItem,
                              status);

   pBSIListItem->pClassifyData  = pClassifyData;
   pBSIListItem->pInjectionData = pInjectionData;

   pLastEntry = ExInterlockedInsertTailList(pListHead,
                                            &(pBSIListItem->entry),
                                            pSpinLock);

   InterlockedIncrement64(pNumEntries);

   if(pPCData->useWorkItems)
   {
      /// List was empty, so start a new thread
      if(!pLastEntry)
         status = KrnlHlprWorkItemQueue(g_pWDMDevice,
                                        BasicStreamInjectionWorkItemRoutine,
                                        pClassifyData,
                                        pInjectionData,
                                        (VOID*)pClassifyData->flowContext);
   }
   else if(pPCData->useThreadedDPC)
      status = KrnlHlprThreadedDPCQueue(BasicStreamInjectionDeferredProcedureCall,
                                        pClassifyData,
                                        pInjectionData,
                                        0,
                                        processorNumber,
                                        TRUE);
   else
      status = KrnlHlprDPCQueue(BasicStreamInjectionDeferredProcedureCall,
                                pClassifyData,
                                pInjectionData,
                                0,
                                processorNumber,
                                TRUE);

   HLPR_BAIL_ON_FAILURE(status);

   /// Do not touch the rights in case other callouts use them (which they shouldn't).
   pClassifyOut->actionType  = FWP_ACTION_BLOCK;

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
   {
      if(pBSIListItem)
      {
         KeAcquireSpinLock(pSpinLock,
                           &irql);

         RemoveEntryList(&(pBSIListItem->entry));

         KeReleaseSpinLock(pSpinLock,
                           irql);


         InterlockedDecrement64(pNumEntries);

         HLPR_DELETE(pBSIListItem,
                     WFPSAMPLER_SYSLIB_TAG);
      }

      if(pClassifyData)
         KrnlHlprClassifyDataDestroyLocalCopy(&pClassifyData);
   }

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- ClassifyBasicStreamInjection() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}


#if(NTDDI_VERSION >= NTDDI_WIN7)

/**
 @classify_function="ClassifyBasicStreamInjection"
 
   Purpose:  Blocks the current data and blindly injects a clone of the data back to the stream 
             without modification.                                                              <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/FF544893.aspx                              <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI ClassifyBasicStreamInjection(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                        _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                        _Inout_opt_ VOID* pStreamCalloutIOPacket,
                                        _In_opt_ const VOID* pClassifyContext,
                                        _In_ const FWPS_FILTER* pFilter,
                                        _In_ UINT64 flowContext,
                                        _Inout_ FWPS_CLASSIFY_OUT* pClassifyOut)
{
   NT_ASSERT(pClassifyValues);
   NT_ASSERT(pMetadata);
   NT_ASSERT(pStreamCalloutIOPacket);
   NT_ASSERT(pFilter);
   NT_ASSERT(pClassifyOut);
   NT_ASSERT(pFilter->providerContext);
   NT_ASSERT(pFilter->providerContext->type == FWPM_GENERAL_CONTEXT);
   NT_ASSERT(pFilter->providerContext->dataBuffer);
   NT_ASSERT(pFilter->providerContext->dataBuffer->size == sizeof(PC_BASIC_STREAM_INJECTION_DATA));
   NT_ASSERT(pClassifyValues->layerId == FWPS_LAYER_STREAM_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_STREAM_V6);

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> ClassifyBasicStreamInjection() [Layer: %s][FilterID: %#I64x][FlowContext: %#I64x][Rights: %#x][BytesIndicated: %I64d]",
              KrnlHlprFwpsLayerIDToString(pClassifyValues->layerId),
              pFilter->filterId,
              flowContext,
              pClassifyOut->rights,
              ((FWPS_STREAM_CALLOUT_IO_PACKET*)pStreamCalloutIOPacket)->streamData->dataLength);

   /// Stream has no concept of Veto. Due to its "waterfall" nature where a block will 
   /// remove the data and others will not be able to see it.  
   /// This means that if we got classified, regardless of the rights, we can do whatever we need to.

   NTSTATUS                        status           = STATUS_SUCCESS;
   INJECTION_DATA*                 pInjectionData   = 0;
   BOOLEAN                         performOutOfBand = TRUE;
   PC_BASIC_STREAM_INJECTION_DATA* pData            = (PC_BASIC_STREAM_INJECTION_DATA*)pFilter->providerContext->dataBuffer->data;

   pClassifyOut->actionType = FWP_ACTION_CONTINUE;

#pragma warning(push)
#pragma warning(disable: 6014) /// pInjectionData will be freed in completionFn using BasicStreamInjectionCompletionDataDestroy

   status = KrnlHlprInjectionDataCreate(&pInjectionData,
                                       pClassifyValues,
                                       pMetadata,
                                       0,
                                       pFilter);
   HLPR_BAIL_ON_FAILURE(status);

#pragma warning(pop)

   /// Override the default of performing Out of Band with the user's specified setting ...
   performOutOfBand = !(pData->performInline);

   if(performOutOfBand)
       status = TriggerBasicStreamInjectionOutOfBand(pClassifyValues,
                                                   pMetadata,
                                                   pStreamCalloutIOPacket,
                                                   pClassifyContext,
                                                   pFilter,
                                                   flowContext,
                                                   pClassifyOut,
                                                   pInjectionData,
                                                   pData);
   else
       status = TriggerBasicStreamInjectionInline(pClassifyValues,
                                               pMetadata,
                                               pStreamCalloutIOPacket,
                                               pClassifyContext,
                                               pFilter,
                                               flowContext,
                                               pClassifyOut,
                                               &pInjectionData);

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
   {
       DbgPrintEx(DPFLTR_IHVNETWORK_ID,
               DPFLTR_ERROR_LEVEL,
               " !!!! ClassifyBasicStreamInjection() [status: %#x]\n",
               status);

       if(pInjectionData)
       KrnlHlprInjectionDataDestroy(&pInjectionData);
   }

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- ClassifyBasicStreamInjection() [Layer: %s][FilterID: %#I64x][StreamAction: %#x][BytesEnforced: %I64d/%I64d][Action: %#x][Rights: %#x][Absorb: %s]\n",
              KrnlHlprFwpsLayerIDToString(pClassifyValues->layerId),
              pFilter->filterId,
              ((FWPS_STREAM_CALLOUT_IO_PACKET*)pStreamCalloutIOPacket)->streamAction,
              ((FWPS_STREAM_CALLOUT_IO_PACKET*)pStreamCalloutIOPacket)->countBytesEnforced,
              ((FWPS_STREAM_CALLOUT_IO_PACKET*)pStreamCalloutIOPacket)->streamData->dataLength,
              pClassifyOut->actionType,
              pClassifyOut->rights,
              (pClassifyOut->flags & FWPS_CLASSIFY_OUT_FLAG_ABSORB) ? "TRUE" : "FALSE");

   return;
}

#else

/**
 @classify_function="ClassifyBasicStreamInjection"
 
   Purpose:  Blocks the current data and blindly injects a clone of the data back to the stream 
             without modification.                                                              <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/FF544890.aspx                              <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI ClassifyBasicStreamInjection(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                        _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                        _Inout_opt_ VOID* pStreamCalloutIOPacket,
                                        _In_ const FWPS_FILTER* pFilter,
                                        _In_ UINT64 flowContext,
                                        _Inout_ FWPS_CLASSIFY_OUT* pClassifyOut)
{
   NT_ASSERT(pClassifyValues);
   NT_ASSERT(pMetadata);
   NT_ASSERT(pStreamCalloutIOPacket);
   NT_ASSERT(pFilter);
   NT_ASSERT(pClassifyOut);
   NT_ASSERT(pFilter->providerContext);
   NT_ASSERT(pFilter->providerContext->type == FWPM_GENERAL_CONTEXT);
   NT_ASSERT(pFilter->providerContext->dataBuffer);
   NT_ASSERT(pFilter->providerContext->dataBuffer->size == sizeof(PC_BASIC_STREAM_INJECTION_DATA));
   NT_ASSERT(pClassifyValues->layerId == FWPS_LAYER_STREAM_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_STREAM_V6);

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> ClassifyBasicStreamInjection() [Layer: %s][FilterID: %#I64x][FlowContext: %#I64x][Rights: %#x][BytesIndicated: %I64d]",
              KrnlHlprFwpsLayerIDToString(pClassifyValues->layerId),
              pFilter->filterId,
              flowContext,
              pClassifyOut->rights,
              ((FWPS_STREAM_CALLOUT_IO_PACKET*)pStreamCalloutIOPacket)->streamData->dataLength);

   /// Stream has no concept of Veto. Due to its "waterfall" nature where a block will 
   /// remove the data and others will not be able to see it.  
   /// This means that if we got classified, regardless of the rights, we can do whatever we need to.

   NTSTATUS                        status           = STATUS_SUCCESS;
   INJECTION_DATA*                 pInjectionData   = 0;
   BOOLEAN                         performOutOfBand = TRUE;
   PC_BASIC_STREAM_INJECTION_DATA* pData            = (PC_BASIC_STREAM_INJECTION_DATA*)pFilter->providerContext->dataBuffer->data;

   pClassifyOut->actionType = FWP_ACTION_CONTINUE;

   status = KrnlHlprInjectionDataCreate(&pInjectionData,
                                       pClassifyValues,
                                       pMetadata,
                                       0,
                                       pFilter);
   HLPR_BAIL_ON_FAILURE(status);

   /// Override the default of performing Out of Band with the user's specified setting ...
   performOutOfBand = !(pData->performInline);

   if(performOutOfBand)
       status = TriggerBasicStreamInjectionOutOfBand(pClassifyValues,
                                                   pMetadata,
                                                   pStreamCalloutIOPacket,
                                                   0,
                                                   pFilter,
                                                   flowContext,
                                                   pClassifyOut,
                                                   pInjectionData,
                                                   pData);
   else
       status = TriggerBasicStreamInjectionInline(pClassifyValues,
                                               pMetadata,
                                               pStreamCalloutIOPacket,
                                               0,
                                               pFilter,
                                               flowContext,
                                               pClassifyOut,
                                               &pInjectionData);

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
   {
       DbgPrintEx(DPFLTR_IHVNETWORK_ID,
               DPFLTR_ERROR_LEVEL,
               " !!!! ClassifyBasicStreamInjection() [status: %#x]\n",
               status);

       if(pInjectionData)
       KrnlHlprInjectionDataDestroy(&pInjectionData);
   }


   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- ClassifyBasicStreamInjection() [Layer: %s][FilterID: %#I64x][StreamAction: %#x][BytesEnforced: %I64d/%I64d][Action: %#x][Rights: %#x][Absorb: %s]\n",
              KrnlHlprFwpsLayerIDToString(pClassifyValues->layerId),
              pFilter->filterId,
              ((FWPS_STREAM_CALLOUT_IO_PACKET*)pStreamCalloutIOPacket)->streamAction,
              ((FWPS_STREAM_CALLOUT_IO_PACKET*)pStreamCalloutIOPacket)->countBytesEnforced,
              ((FWPS_STREAM_CALLOUT_IO_PACKET*)pStreamCalloutIOPacket)->streamData->dataLength,
              pClassifyOut->actionType,
              pClassifyOut->rights,
              (pClassifyOut->flags & FWPS_CLASSIFY_OUT_FLAG_ABSORB) ? "TRUE" : "FALSE");
   
   return;
}

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)
