////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      ClassifyFunctions_PendAuthorizationCallouts.cpp
//
//   Abstract:
//      This module contains WFP Classify functions for pending and completing authorizations.
//
//   Naming Convention:
//
//      <Module><Scenario>
//  
//      i.e.
//
//       ClassifyPendAuthorization
//
//       <Module>
//          Classify               -       Function is an FWPS_CALLOUT_CLASSIFY_FN.
//          Perform                -       Function executes the desired scenario.
//          Prv                    -       Function is a private helper to this module.
//          Trigger                -
//       <Scenario>
//          PendAuthorization      -       Function demonstates pending authorization requests.
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Private Functions:
//      PendAuthorizationWorkItemRoutine(),
//      PerformPendAuthorization(),
//      PrvCloneAuthorizedNBLAndInject()
//      TriggerPendAuthorizationOutOfBand(),
//
//   Public Functions:
//      PendAuthorizationDeferredProcedureCall(),
//      ClassifyPendAuthorization(),
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Enhance function declaration for IntelliSense, enhance 
//                                              traces, and add support for controlData.
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Framework_WFPSamplerCalloutDriver.h"             /// .
#include "ClassifyFunctions_PendAuthorizationCallouts.tmh" /// $(OBJ_PATH)\$(O)\ 

/**
 @private_function="PrvCloneAuthorizedNBLAndInject"
 
   Purpose:                                                                                     <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS PrvCloneAuthorizedNBLAndInject(_Inout_ CLASSIFY_DATA** ppClassifyData,
                                        _Inout_ INJECTION_DATA** ppInjectionData)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> PrvCloneAuthorizedNBLAndInject()\n");

#endif /// DBG
   
   NT_ASSERT(ppClassifyData);
   NT_ASSERT(ppInjectionData);
   NT_ASSERT(*ppClassifyData);
   NT_ASSERT(*ppInjectionData);

   NTSTATUS                            status          = STATUS_SUCCESS;
   FWPS_INCOMING_VALUES*               pClassifyValues = (FWPS_INCOMING_VALUES*)(*ppClassifyData)->pClassifyValues;
   FWPS_INCOMING_METADATA_VALUES*      pMetadata       = (FWPS_INCOMING_METADATA_VALUES*)(*ppClassifyData)->pMetadataValues;
   UINT32                              bytesRetreated  = 0;
   COMPARTMENT_ID                      compartmentID   = DEFAULT_COMPARTMENT_ID;
   BOOLEAN                             isInbound       = FALSE;
   NET_BUFFER_LIST*                    pNetBufferList  = 0;
   PEND_AUTHORIZATION_COMPLETION_DATA* pCompletionData = 0;
   FWPS_TRANSPORT_SEND_PARAMS*         pSendParams     = 0;
   BYTE*                               pRemoteAddress  = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pCompletionData will be freed in completionFn using PendAuthorizationCompletionDataDestroy

   HLPR_NEW(pCompletionData,
            PEND_AUTHORIZATION_COMPLETION_DATA,
            WFPSAMPLER_CALLOUT_DRIVER_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pCompletionData,
                              status);

   HLPR_NEW(pSendParams,
            FWPS_TRANSPORT_SEND_PARAMS,
            WFPSAMPLER_CALLOUT_DRIVER_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pSendParams,
                              status);

#pragma warning(pop)

   KeInitializeSpinLock(&(pCompletionData->spinLock));

   pCompletionData->performedInline = FALSE;
   pCompletionData->pClassifyData   = *ppClassifyData;
   pCompletionData->pInjectionData  = *ppInjectionData;
   pCompletionData->pSendParams     = pSendParams;

   /// Responsibility for freeing this memory has been transferred to the pCompletionData
   *ppClassifyData = 0;

   *ppInjectionData = 0;

   pSendParams = 0;

   if(pCompletionData->pInjectionData->direction == FWP_DIRECTION_INBOUND)
      isInbound = TRUE;

   if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                     FWPS_METADATA_FIELD_COMPARTMENT_ID))
      compartmentID = (COMPARTMENT_ID)pMetadata->compartmentId;

   if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                     FWPS_METADATA_FIELD_IP_HEADER_SIZE))
      bytesRetreated = pMetadata->ipHeaderSize;

   if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                     FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE))
      bytesRetreated += pMetadata->transportHeaderSize;

   if(pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4 ||
      pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6)
   {
      if(isInbound) /// NBL offset is at the start of the transport header ...
      {
         if(bytesRetreated)
         {
            /// so retreat (size of IP Header) to clone the whole NBL
            status = NdisRetreatNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)pCompletionData->pClassifyData->pPacket),
                                                   bytesRetreated,
                                                   0,
                                                   0);
            if(status != STATUS_SUCCESS)
            {
               DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                          DPFLTR_ERROR_LEVEL,
                          " !!!! PrvCloneAuthorizedNBLAndInject: NdisRetreatNetBufferDataStart() [status: %#x]\n",
                          status);

               HLPR_BAIL;
            }
         }
      }
   }
   else if(pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V4 ||
           pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V6)
   {
      /// NBL offset is at the Transport Header, and no IP Header is present yet
      bytesRetreated = 0;

      isInbound = FALSE;
   }
   else
   {
      status = STATUS_FWP_INCOMPATIBLE_LAYER;

      HLPR_BAIL;
   }

   status = FwpsAllocateCloneNetBufferList((NET_BUFFER_LIST*)(pCompletionData->pClassifyData->pPacket),
                                           g_pNDISPoolData->nblPoolHandle,
                                           g_pNDISPoolData->nbPoolHandle,
                                           0,
                                           &pNetBufferList);

   if(bytesRetreated)
   {
      /// Advance the NBL offset so we are back at the expected position in the NET_BUFFER_LIST
      NdisAdvanceNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)pCompletionData->pClassifyData->pPacket),
                                    bytesRetreated,
                                    FALSE,
                                    0);
   }

   if(status != STATUS_SUCCESS)
   {
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! PrvCloneAuthorizedNBLAndInject : FwpsAllocateCloneNetBufferList() [status: %#x]\n",
                 status);

      HLPR_BAIL;
   }

   if(isInbound)
   {
      FWP_VALUE* pInterfaceIndex    = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                                &FWPM_CONDITION_INTERFACE_INDEX);
      FWP_VALUE* pSubInterfaceIndex = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                                &FWPM_CONDITION_SUB_INTERFACE_INDEX);
      IF_INDEX   interfaceIndex     = 0;
      IF_INDEX   subInterfaceIndex  = 0;

      if(pInterfaceIndex &&
         pInterfaceIndex->type == FWP_UINT32)
         interfaceIndex = (IF_INDEX)pInterfaceIndex->uint32;

      if(pSubInterfaceIndex &&
         pSubInterfaceIndex->type == FWP_UINT32)
         subInterfaceIndex = (IF_INDEX)pSubInterfaceIndex->uint32;

      status = FwpsInjectTransportReceiveAsync(pCompletionData->pInjectionData->injectionHandle,
                                               pCompletionData->pInjectionData->injectionContext,
                                               0,
                                               0,
                                               pCompletionData->pInjectionData->addressFamily,
                                               compartmentID,
                                               interfaceIndex,
                                               subInterfaceIndex,
                                               pNetBufferList,
                                               CompletePendAuthorization,
                                               pCompletionData);
   }
   else
   {
      UINT64     endpointHandle = 0;
      FWP_VALUE* pAddressValue  = 0;

      if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_METADATA_FIELD_TRANSPORT_ENDPOINT_HANDLE))
         endpointHandle = pMetadata->transportEndpointHandle;

      pAddressValue = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                &FWPM_CONDITION_IP_REMOTE_ADDRESS);
      if(pAddressValue)
      {
         if(pCompletionData->pInjectionData->addressFamily == AF_INET)
         {
            UINT32 tempAddress = htonl(pAddressValue->uint32);

#pragma warning(push)
#pragma warning(disable: 6014) /// pRemoteAddress will be freed in completionFn using PendAuthorizationCompletionDataDestroy

            HLPR_NEW_ARRAY(pRemoteAddress,
                           BYTE,
                           IPV4_ADDRESS_SIZE,
                           WFPSAMPLER_CALLOUT_DRIVER_TAG);
            HLPR_BAIL_ON_ALLOC_FAILURE(pRemoteAddress,
                                       status);

#pragma warning(pop)

            RtlCopyMemory(pRemoteAddress,
                          &tempAddress,
                          IPV4_ADDRESS_SIZE);
         }
         else
         {

#pragma warning(push)
#pragma warning(disable: 6014) /// pRemoteAddress will be freed in completionFn using PendAuthorizationCompletionDataDestroy

            HLPR_NEW_ARRAY(pRemoteAddress,
                           BYTE,
                           IPV6_ADDRESS_SIZE,
                           WFPSAMPLER_CALLOUT_DRIVER_TAG);
            HLPR_BAIL_ON_ALLOC_FAILURE(pRemoteAddress,
                                       status);

#pragma warning(pop)

            RtlCopyMemory(pRemoteAddress,
                          pAddressValue->byteArray16->byteArray16,
                          IPV6_ADDRESS_SIZE);

            if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                              FWPS_METADATA_FIELD_REMOTE_SCOPE_ID))
               pCompletionData->pSendParams->remoteScopeId = pMetadata->remoteScopeId;
         }

         pCompletionData->pSendParams->remoteAddress = pRemoteAddress;
      }

      pCompletionData->pSendParams->controlData       = (WSACMSGHDR*)pCompletionData->pInjectionData->pControlData;
      pCompletionData->pSendParams->controlDataLength = pCompletionData->pInjectionData->controlDataLength;

      pCompletionData->refCount = KrnlHlprNBLGetRequiredRefCount(pNetBufferList);

      status = FwpsInjectTransportSendAsync(pCompletionData->pInjectionData->injectionHandle,
                                            pCompletionData->pInjectionData->injectionContext,
                                            endpointHandle,
                                            0,
                                            pCompletionData->pSendParams,
                                            pCompletionData->pInjectionData->addressFamily,
                                            compartmentID,
                                            pNetBufferList,
                                            CompletePendAuthorization,
                                            pCompletionData);
   }

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
   {
      if(pNetBufferList)
      {
         FwpsFreeCloneNetBufferList(pNetBufferList,
                                    0);

         pNetBufferList = 0;
      }

      if(pCompletionData)
         PendAuthorizationCompletionDataDestroy(&pCompletionData,
                                                TRUE);
   }

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- PrvCloneAuthorizedNBLAndInject() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}

#pragma warning(push)
#pragma warning(disable: 6101) /// *ppClassifyData and *ppPendData are freed and set to 0 on successful completion

/**
 @private_function="PerformPendAuthorization"
 
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
NTSTATUS PerformPendAuthorization(_Inout_ CLASSIFY_DATA** ppClassifyData,
                                  _Inout_ PEND_DATA** ppPendData,
                                  _Inout_ INJECTION_DATA** ppInjectionData)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> PerformPendAuthorization()\n");

#endif /// DBG
   
   NT_ASSERT(ppClassifyData);
   NT_ASSERT(ppPendData);
   NT_ASSERT(ppInjectionData);
   NT_ASSERT(*ppClassifyData);
   NT_ASSERT(*ppPendData);

   NTSTATUS         status      = STATUS_SUCCESS;
   NET_BUFFER_LIST* pNBL        = (*ppPendData)->pNBL;
   UINT32           finalAction = (*ppPendData)->pPendAuthorizationData->finalAction;
   UINT32           delay       = (*ppPendData)->pPendAuthorizationData->delay;
   BOOLEAN          reInjected  = FALSE;

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

   if(pNBL &&
      finalAction == FWP_ACTION_PERMIT)
   {
      status = PrvCloneAuthorizedNBLAndInject(ppClassifyData,
                                              ppInjectionData);
      if(status != STATUS_SUCCESS)
      {
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_ERROR_LEVEL,
                    " !!!! PerformPendAuthorization : PrvCloneAuthorizedNBLAndInject() [status: %#x]\n",
                    status);
      }
      else
         reInjected = TRUE;
   }

#pragma warning(push)
#pragma warning(disable: 6001) /// *ppClassifyData initialized prior to call to this function

   if(!reInjected)
      KrnlHlprClassifyDataDestroyLocalCopy(ppClassifyData);

#pragma warning(pop)

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- PerformPendAuthorization() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
};

#pragma warning(pop)

/**
 @private_function="PendAuthorizationDeferredProcedureCall"
 
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
VOID PendAuthorizationDeferredProcedureCall(_In_ KDPC* pDPC,
                                            _In_opt_ PVOID pContext,
                                            _In_opt_ PVOID pArg1,
                                            _In_opt_ PVOID pArg2)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> PendAuthorizationDeferredProcedureCall()\n");

#endif /// DBG
   
   UNREFERENCED_PARAMETER(pDPC);
   UNREFERENCED_PARAMETER(pContext);
   UNREFERENCED_PARAMETER(pArg2);

   NT_ASSERT(pDPC);
   NT_ASSERT(pArg1);
   NT_ASSERT(((DPC_DATA*)pArg1)->pClassifyData);
   NT_ASSERT(((DPC_DATA*)pArg1)->pInjectionData);

   DPC_DATA* pDPCData = (DPC_DATA*)pArg1;

   if(pDPCData)
   {
      NTSTATUS status = STATUS_SUCCESS;

      status = PerformPendAuthorization(&(pDPCData->pClassifyData),
                                        &(pDPCData->pPendData),
                                        (INJECTION_DATA**)&(pDPCData->pContext));
      if(status != STATUS_SUCCESS)
      {
         if(pDPCData->pClassifyData)
            KrnlHlprClassifyDataDestroyLocalCopy(&(pDPCData->pClassifyData));

         if(pDPCData->pInjectionData)
            KrnlHlprInjectionDataDestroy(&(pDPCData->pInjectionData));

         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_ERROR_LEVEL,
                    " !!!! PendAuthorizationDeferredProcedureCall() [status: %#x]\n",
                    status);
      }

      KrnlHlprDPCDataDestroy(&pDPCData);
   }

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- PendAuthorizationDeferredProcedureCall()\n");

#endif /// DBG
   
   return;
}

/**
 @private_function="PendAuthorizationWorkItemRoutine"
 
   Purpose:                                                                                     <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Function_class_(IO_WORKITEM_ROUTINE)
VOID PendAuthorizationWorkItemRoutine(_In_ PDEVICE_OBJECT pDeviceObject,
                                      _In_opt_ PVOID pContext)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> PendAuthorizationWorkItemRoutine()\n");

#endif /// DBG
   
   UNREFERENCED_PARAMETER(pDeviceObject);

   NT_ASSERT(pContext);
   NT_ASSERT(((WORKITEM_DATA*)pContext)->pClassifyData);
   NT_ASSERT(((WORKITEM_DATA*)pContext)->pInjectionData);

   WORKITEM_DATA* pWorkItemData = (WORKITEM_DATA*)pContext;

   if(pWorkItemData)
   {
      NTSTATUS status = STATUS_SUCCESS;

      status = PerformPendAuthorization(&(pWorkItemData->pClassifyData),
                                        &(pWorkItemData->pPendData),
                                        (INJECTION_DATA**)&(pWorkItemData->pContext));
      if(status != STATUS_SUCCESS)
      {
         if(pWorkItemData->pClassifyData)
            KrnlHlprClassifyDataDestroyLocalCopy(&(pWorkItemData->pClassifyData));

         if(pWorkItemData->pInjectionData)
            KrnlHlprInjectionDataDestroy(&(pWorkItemData->pInjectionData));

         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_ERROR_LEVEL,
                    " !!!! PendAuthorizationWorkItemRoutine() [status: %#x]\n",
                    status);
      }

      KrnlHlprWorkItemDataDestroy(&pWorkItemData);
   }

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- PendAuthorizationWorkItemRoutine()\n");

#endif /// DBG
   
   return;
}

/**
 @private_function="TriggerPendAuthorizationOutOfBand"
 
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
NTSTATUS TriggerPendAuthorizationOutOfBand(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                           _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                           _Inout_opt_ VOID* pLayerData,
                                           _In_opt_ const VOID* pClassifyContext,
                                           _In_ const FWPS_FILTER* pFilter,
                                           _In_ UINT64 flowContext,
                                           _In_ FWPS_CLASSIFY_OUT* pClassifyOut,
                                           _Inout_opt_ INJECTION_DATA* pInjectionData,
                                           _Inout_ PEND_DATA* pPendData,
                                           _In_ PC_PEND_AUTHORIZATION_DATA* pPCData)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> TriggerPendAuthorizationOutOfBand()\n");

#endif /// DBG
   
   NT_ASSERT(pClassifyValues);
   NT_ASSERT(pMetadata);
   NT_ASSERT(pFilter);
   NT_ASSERT(pClassifyOut);
   NT_ASSERT(pPendData);
   NT_ASSERT(pPCData);
 
   NTSTATUS       status        = STATUS_SUCCESS;
   CLASSIFY_DATA* pClassifyData = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pInjectionData will be freed in completionFn using PendAuthorizationCompletionDataDestroy

   status = KrnlHlprClassifyDataCreateLocalCopy(&pClassifyData,
                                                pClassifyValues,
                                                pMetadata,
                                                pLayerData,
                                                pClassifyContext,
                                                pFilter,
                                                flowContext,
                                                pClassifyOut);
   HLPR_BAIL_ON_FAILURE(status);

#pragma warning(pop)

   if(pPCData->useWorkItems ||
      pPCData->delay)
   {
      /// introducing the delay requires PASSIVE_LEVEL, so force use of a Work Item
      status = KrnlHlprWorkItemQueue(g_pWDMDevice,
                                     PendAuthorizationWorkItemRoutine,
                                     pClassifyData,
                                     pPendData,
                                     (VOID*)pInjectionData);
   }
   else
   {
      if(pPCData->useThreadedDPC)
         status = KrnlHlprThreadedDPCQueue(PendAuthorizationDeferredProcedureCall,
                                           pClassifyData,
                                           pPendData,
                                           pInjectionData);
      else
         status = KrnlHlprDPCQueue(PendAuthorizationDeferredProcedureCall,
                                   pClassifyData,
                                   pPendData,
                                   pInjectionData);
   }

   HLPR_BAIL_ON_FAILURE(status);

   pClassifyOut->actionType = FWP_ACTION_BLOCK;

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
   {
      if(pClassifyData)
         KrnlHlprClassifyDataDestroyLocalCopy(&pClassifyData);
   }

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- TriggerPendAuthorizationOutOfBand() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}

#if(NTDDI_VERSION >= NTDDI_WIN7)

/**
 @classify_function="ClassifyPendAuthorization"
 
   Purpose:  Classify Function which will pend an authorization request, and send the request 
             for further processing to a worker thread.  On a reauthorization the final decided 
             action is returned as the actionType.                                              <br>
                                                                                                <br>
   Notes:    Applies to the following layers:                                                   <br>
                FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V{4/6}                                       <br>
                FWPM_LAYER_ALE_AUTH_LISTEN_V{4/6}                                               <br>
                FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V{4/6}                                          <br>
                FWPM_LAYER_ALE_AUTH_CONNECT_V{4/6}                                              <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF551229.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF544893.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI ClassifyPendAuthorization(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                     _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                     _Inout_opt_ VOID* pNetBufferList,
                                     _In_opt_ const VOID* pClassifyContext,
                                     _In_ const FWPS_FILTER* pFilter,
                                     _In_ UINT64 flowContext,
                                     _Inout_ FWPS_CLASSIFY_OUT* pClassifyOut)
{
   NT_ASSERT(pClassifyValues);
   NT_ASSERT(pMetadata);
   NT_ASSERT(pFilter);
   NT_ASSERT(pClassifyOut);
   NT_ASSERT(pClassifyValues->layerId == FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_LISTEN_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_LISTEN_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V6);
   NT_ASSERT(pFilter->providerContext);
   NT_ASSERT(pFilter->providerContext->type == FWPM_GENERAL_CONTEXT);
   NT_ASSERT(pFilter->providerContext->dataBuffer);
   NT_ASSERT(pFilter->providerContext->dataBuffer->size == sizeof(PC_PEND_AUTHORIZATION_DATA));
   NT_ASSERT(pFilter->providerContext->dataBuffer->data);

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> ClassifyPendAuthorization() [Layer: %s][FilterID: %#I64x][Rights: %#x][Reauth: %s]",
              KrnlHlprFwpsLayerIDToString(pClassifyValues->layerId),
              pFilter->filterId,
              pClassifyOut->rights,
              KrnlHlprFwpsIncomingValueConditionFlagsAreSet(pClassifyValues,
                                                            FWP_CONDITION_FLAG_IS_REAUTHORIZE) ? "TRUE" : "FALSE");

   PC_PEND_AUTHORIZATION_DATA* pPendAuthorizationData = (PC_PEND_AUTHORIZATION_DATA*)pFilter->providerContext->dataBuffer->data;
   BOOLEAN                     isReauth               = KrnlHlprFwpsIncomingValueConditionFlagsAreSet(pClassifyValues,
                                                                                                      FWP_CONDITION_FLAG_IS_REAUTHORIZE);

   /// RESOURCE_ASSIGNTMENT & AUTH_LISTEN will Reauthorize after the completion of the pend.
   /// AUTH_CONNECT's completeOperation will trigger a Reauthorization
   if(isReauth)
   {
      if(pClassifyOut->rights & FWPS_RIGHT_ACTION_WRITE)
         pClassifyOut->actionType = pPendAuthorizationData->finalAction;
   }
   else
   {
      BOOLEAN actionSet = FALSE;

      if(pClassifyOut->rights & FWPS_RIGHT_ACTION_WRITE)
      {
         NTSTATUS        status         = STATUS_SUCCESS;
         INJECTION_DATA* pInjectionData = 0;
         PEND_DATA*      pPendData      = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pInjectionData will be freed in completionFn using PendAuthorizationCompletionDataDestroy

         if(pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4 ||
            pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6 ||
            pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V4 ||
            pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V6)
         {
            status = KrnlHlprInjectionDataCreate(&pInjectionData,
                                                 pClassifyValues,
                                                 pMetadata,
                                                 (NET_BUFFER_LIST*)pNetBufferList,
                                                 pFilter);
            HLPR_BAIL_ON_FAILURE(status);
         }

#pragma warning(pop)

         /// AUTH_RECV_ACCEPT's injection will be indicative of PERMIT
         if(pInjectionData &&
            pInjectionData->injectionState == FWPS_PACKET_INJECTED_BY_SELF)
         {
            isReauth = TRUE;

            pClassifyOut->actionType = pPendAuthorizationData->finalAction;

            actionSet = TRUE;
         }
         else
         {
#pragma warning(push)
#pragma warning(disable: 6014) /// pPendData will be freed in completionFn using PendAuthorizationCompletionDataDestroy

            status = KrnlHlprPendDataCreate(&pPendData,
                                            pClassifyValues,
                                            pMetadata,
                                            (NET_BUFFER_LIST*)pNetBufferList,
                                            pFilter);
            HLPR_BAIL_ON_FAILURE(status);

#pragma warning(pop)

            status = TriggerPendAuthorizationOutOfBand(pClassifyValues,
                                                       pMetadata,
                                                       pNetBufferList,
                                                       pClassifyContext,
                                                       pFilter,
                                                       flowContext,
                                                       pClassifyOut,
                                                       pInjectionData,
                                                       pPendData,
                                                       pPendAuthorizationData);
         }

         HLPR_BAIL_LABEL:

         if(status != STATUS_SUCCESS)
         {
            DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                       DPFLTR_ERROR_LEVEL,
                       " !!!! ClassifyPendAuthorization() [status: %#x]\n",
                       status);

            if(pInjectionData)
               KrnlHlprInjectionDataDestroy(&pInjectionData);

            if(pPendData)
               KrnlHlprPendDataDestroy(&pPendData);
         }

         if(!actionSet)
         {
            pClassifyOut->actionType  = FWP_ACTION_BLOCK;
            pClassifyOut->flags      |= FWPS_CLASSIFY_OUT_FLAG_ABSORB;
         }
      }
   }

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- ClassifyPendAuthorization() [Layer: %s][FilterID: %#I64x][Action: %#x][Rights: %#x][Absorb: %s][ReAuth: %s]\n",
              KrnlHlprFwpsLayerIDToString(pClassifyValues->layerId),
              pFilter->filterId,
              pClassifyOut->actionType,
              pClassifyOut->rights,
              (pClassifyOut->flags & FWPS_CLASSIFY_OUT_FLAG_ABSORB) ? "TRUE" : "FALSE",
              isReauth ? "TRUE" : "FALSE");

   return;
}

#else

/**
 @classify_function="ClassifyPendAuthorization"
 
   Purpose:  Classify Function which will pend an authorization request, and send the request 
             for further processing to a worker thread.  On a reauthorization the final decided 
             action is returned as the actionType.                                              <br>
                                                                                                <br>
   Notes:    Applies to the following layers:                                                   <br>
                FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V{4/6}                                       <br>
                FWPM_LAYER_ALE_AUTH_LISTEN_V{4/6}                                               <br>
                FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V{4/6}                                          <br>
                FWPM_LAYER_ALE_AUTH_CONNECT_V{4/6}                                              <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF551229.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF544890.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI ClassifyPendAuthorization(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                     _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                     _Inout_opt_ VOID* pNetBufferList,
                                     _In_ const FWPS_FILTER* pFilter,
                                     _In_ UINT64 flowContext,
                                     _Inout_ FWPS_CLASSIFY_OUT* pClassifyOut)
{
   NT_ASSERT(pClassifyValues);
   NT_ASSERT(pMetadata);
   NT_ASSERT(pFilter);
   NT_ASSERT(pClassifyOut);
   NT_ASSERT(pClassifyValues->layerId == FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_LISTEN_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_LISTEN_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V6);
   NT_ASSERT(pFilter->providerContext);
   NT_ASSERT(pFilter->providerContext->type == FWPM_GENERAL_CONTEXT);
   NT_ASSERT(pFilter->providerContext->dataBuffer);
   NT_ASSERT(pFilter->providerContext->dataBuffer->size == sizeof(PC_PEND_AUTHORIZATION_DATA));
   NT_ASSERT(pFilter->providerContext->dataBuffer->data);

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> ClassifyPendAuthorization() [Layer: %s][FilterID: %#I64x][Rights: %#x][ReAuth: %s]",
              KrnlHlprFwpsLayerIDToString(pClassifyValues->layerId),
              pFilter->filterId,
              pClassifyOut->rights,
              KrnlHlprFwpsIncomingValueConditionFlagsAreSet(pClassifyValues,
                                                            FWP_CONDITION_FLAG_IS_REAUTHORIZE) ? "TRUE" : "FALSE");

   PC_PEND_AUTHORIZATION_DATA* pPendAuthorizationData = (PC_PEND_AUTHORIZATION_DATA*)pFilter->providerContext->dataBuffer->data;
   BOOLEAN                     isReauth               = KrnlHlprFwpsIncomingValueConditionFlagsAreSet(pClassifyValues,
                                                                                                      FWP_CONDITION_FLAG_IS_REAUTHORIZE);

   /// RESOURCE_ASSIGNTMENT & AUTH_LISTEN will Reauthorize after the completion of the pend.
   /// AUTH_CONNECT's completeOperation will trigger a Reauthorization
   if(isReauth)
   {
      if(pClassifyOut->rights & FWPS_RIGHT_ACTION_WRITE)
         pClassifyOut->actionType = pPendAuthorizationData->finalAction;
   }
   else
   {
      BOOLEAN actionSet = FALSE;

      if(pClassifyOut->rights & FWPS_RIGHT_ACTION_WRITE)
      {
         NTSTATUS        status         = STATUS_SUCCESS;
         INJECTION_DATA* pInjectionData = 0;
         PEND_DATA*      pPendData      = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pInjectionData will be freed in completionFn using PendAuthorizationCompletionDataDestroy

         if(pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4 ||
            pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6 ||
            pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V4 ||
            pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V6)
         {
            status = KrnlHlprInjectionDataCreate(&pInjectionData,
                                                 pClassifyValues,
                                                 pMetadata,
                                                 (NET_BUFFER_LIST*)pNetBufferList,
                                                 pFilter);
            HLPR_BAIL_ON_FAILURE(status);
         }

#pragma warning(pop)

         /// AUTH_RECV_ACCEPT's injection will be indicative of ReAuth
         if(pInjectionData &&
            pInjectionData->injectionState == FWPS_PACKET_INJECTED_BY_SELF)
         {
            isReauth = TRUE;

            pClassifyOut->actionType = pPendAuthorizationData->finalAction;

            actionSet = TRUE;
         }
         else
         {
#pragma warning(push)
#pragma warning(disable: 6014) /// pInjectionData will be freed in completionFn using PendAuthorizationCompletionDataDestroy

            status = KrnlHlprPendDataCreate(&pPendData,
                                            pClassifyValues,
                                            pMetadata,
                                            (NET_BUFFER_LIST*)pNetBufferList,
                                            pFilter);
            HLPR_BAIL_ON_FAILURE(status);

#pragma warning(pop)

            status = TriggerPendAuthorizationOutOfBand(pClassifyValues,
                                                       pMetadata,
                                                       pNetBufferList,
                                                       0,
                                                       pFilter,
                                                       flowContext,
                                                       pClassifyOut,
                                                       pInjectionData,
                                                       pPendData,
                                                       pPendAuthorizationData);
         }

         HLPR_BAIL_LABEL:

         if(status != STATUS_SUCCESS)
         {
            DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                       DPFLTR_ERROR_LEVEL,
                       " !!!! ClassifyPendAuthorization() [status: %#x]\n",
                       status);

            if(pInjectionData)
               KrnlHlprInjectionDataDestroy(&pInjectionData);

            if(pPendData)
               KrnlHlprPendDataDestroy(&pPendData);
         }

         if(!actionSet)
         {
            pClassifyOut->actionType  = FWP_ACTION_BLOCK;
            pClassifyOut->flags      |= FWPS_CLASSIFY_OUT_FLAG_ABSORB;
         }
      }
   }

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- ClassifyPendAuthorization() [Layer: %s][FilterID: %#I64x][Action: %#x][Rights: %#x][Absorb: %s][ReAuth: %s]\n",
              KrnlHlprFwpsLayerIDToString(pClassifyValues->layerId),
              pFilter->filterId,
              pClassifyOut->actionType,
              pClassifyOut->rights,
              (pClassifyOut->flags & FWPS_CLASSIFY_OUT_FLAG_ABSORB) ? "TRUE" : "FALSE",
              isReauth ? "TRUE" : "FALSE");

   return;
}

#endif // (NTDDI_VERSION >= NTDDI_WIN7)
