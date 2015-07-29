////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      ClassifyFunctions_FlowAssocationCallouts.cpp
//
//   Abstract:
//      This module contains WFP Classify functions for associating contexts with flows.
//
//   Naming Convention:
//
//      <Module><Scenario>
//  
//      i.e.
//
//       ClassifyFlowAssociation
//
//       <Module>
//          Classify        - Function is an FWPS_CALLOUT_CLASSIFY_FN.
//          Perform         - Executes the desired scenario.
//       <Scenario>
//          FlowAssociation - Function demonstates associating contexts with flows.
//
//   Private Functions:
//      PerformFlowAssociation(),
//
//   Public Functions:
//      ClassifyFlowAssociation(),
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      December  13,   2013  -     1.1   -  Creation
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Framework_WFPSamplerCalloutDriver.h"           /// .
#include "ClassifyFunctions_FlowAssociationCallouts.tmh" /// $(OBJ_PATH)\$(O)\ 

NTSTATUS PerformFlowAssociation(_In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                _In_ const PC_FLOW_ASSOCIATION_DATA* pFlowAssociationData)
{

#if DBG

      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 " ---> PerformFlowAssociation()\n");

#endif /// DBG

   NT_ASSERT(pMetadata);
   NT_ASSERT(pFlowAssociationData);

   NTSTATUS status = STATUS_SUCCESS;

   if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                     FWPS_METADATA_FIELD_FLOW_HANDLE))
   {
      for(UINT32 index = 0;
          index < pFlowAssociationData->itemCount;
          index++)
      {
         FLOW_CONTEXT* pFlowContext = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pNotifyData is expected to be cleaned up by NotifyFlowDeleteNotification

         status = KrnlHlprFlowContextCreate(&pFlowContext,
                                            pMetadata->flowHandle,
                                            pFlowAssociationData->pLayerIDs[index],
                                            pFlowAssociationData->pCalloutIDs[index],
                                            CONTEXT_TYPE_DEFAULT,
                                            pFlowAssociationData);
         HLPR_BAIL_ON_FAILURE(status);

#pragma warning(pop)

      }
   }

   HLPR_BAIL_LABEL:

#if DBG
   
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 " <--- PerformFlowAssociation() [status: %#x]\n",
                 status);

#endif /// DBG

   return status;
}

#if(NTDDI_VERSION >= NTDDI_WIN7)

/**
 @classify_function="ClassifyFlowAssociation"
 
   Purpose:  Associates a context with a flow which is available for each of the layers 
             specified in the provider context                                                  <br>
                                                                                                <br>
   Notes:    Applies to the following layers:                                                   <br>
                FWPS_LAYER_ALE_FLOW_ESTABLISHED_V{4/6}                                          <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI ClassifyFlowAssociation(_In_ const FWPS_INCOMING_VALUES0* pClassifyValues,
                                   _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                   _Inout_opt_ VOID* pNetBufferList,
                                   _In_opt_ const VOID* pClassifyContext,
                                   _In_ const FWPS_FILTER* pFilter,
                                   _In_ UINT64 flowContext,
                                   _Inout_ FWPS_CLASSIFY_OUT0* pClassifyOut)
{
   NT_ASSERT(pClassifyValues);
   NT_ASSERT(pClassifyValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6);
   NT_ASSERT(pMetadata);
   NT_ASSERT(pFilter);
   NT_ASSERT(pFilter->providerContext);
   NT_ASSERT(pFilter->providerContext->type == FWPM_GENERAL_CONTEXT);
   NT_ASSERT(pFilter->providerContext->dataBuffer);
   NT_ASSERT(pFilter->providerContext->dataBuffer);
   NT_ASSERT(pFilter->providerContext->dataBuffer->size == sizeof(PC_FLOW_ASSOCIATION_DATA));
   NT_ASSERT(pFilter->providerContext->dataBuffer->data);
   NT_ASSERT(pClassifyOut);

   UNREFERENCED_PARAMETER(pNetBufferList);
   UNREFERENCED_PARAMETER(pClassifyContext);

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> ClassifyFlowAssociation() [Layer: %s][FilterID: %#I64x][Rights: %#x]",
              KrnlHlprFwpsLayerIDToString(pClassifyValues->layerId),
              pFilter->filterId,
              pClassifyOut->rights);

   if(pClassifyOut->rights & FWPS_RIGHT_ACTION_WRITE)
   {
      pClassifyOut->actionType = FWP_ACTION_CONTINUE;

      /// ensure we only associate the context once
      if(flowContext == 0 &&
         !(pClassifyValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_FLAGS].value.uint32 & FWP_CONDITION_FLAG_IS_REAUTHORIZE))
         PerformFlowAssociation(pMetadata,
                                (PC_FLOW_ASSOCIATION_DATA*)pFilter->providerContext->dataBuffer->data);
   }

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- ClassifyFlowAssociation() [Layer: %s][FilterID: %#I64x][Action: %#x][Rights: %#x][Absorb: %s]\n",
              KrnlHlprFwpsLayerIDToString(pClassifyValues->layerId),
              pFilter->filterId,
              pClassifyOut->actionType,
              pClassifyOut->rights,
              (pClassifyOut->flags & FWPS_CLASSIFY_OUT_FLAG_ABSORB) ? "TRUE" : "FALSE");

   return;
}

#else

/**
 @classify_function="ClassifyFlowAssociation"
 
   Purpose:  Associates a context with a flow which is available for each of the layers 
             specified in the provider context                                                  <br>
                                                                                                <br>
   Notes:    Applies to the following layers:                                                   <br>
                FWPS_LAYER_ALE_FLOW_ESTABLISHED_V{4/6}                                          <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI ClassifyFlowAssociation(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                   _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                   _Inout_opt_ VOID* pNetBufferList,
                                   _In_ const FWPS_FILTER* pFilter,
                                   _In_ UINT64 flowContext,
                                   _Inout_ FWPS_CLASSIFY_OUT* pClassifyOut)
{
   NT_ASSERT(pClassifyValues);
   NT_ASSERT(pClassifyValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6);
   NT_ASSERT(pMetadata);
   NT_ASSERT(pFilter);
   NT_ASSERT(pFilter->providerContext);
   NT_ASSERT(pFilter->providerContext->type == FWPM_GENERAL_CONTEXT);
   NT_ASSERT(pFilter->providerContext->dataBuffer);
   NT_ASSERT(pFilter->providerContext->dataBuffer);
   NT_ASSERT(pFilter->providerContext->dataBuffer->size == sizeof(PC_FLOW_ASSOCIATION_DATA));
   NT_ASSERT(pFilter->providerContext->dataBuffer->data);
   NT_ASSERT(pClassifyOut);

   UNREFERENCED_PARAMETER(pNetBufferList);

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> ClassifyFlowAssociation() [Layer: %s][FilterID: %#I64x][Rights: %#x]",
              KrnlHlprFwpsLayerIDToString(pClassifyValues->layerId),
              pFilter->filterId,
              pClassifyOut->rights);

   if(pClassifyOut->rights & FWPS_RIGHT_ACTION_WRITE)
   {
      pClassifyOut->actionType = FWP_ACTION_CONTINUE;

      /// ensure we only associate the context once
      if(flowContext == 0 &&
         !(pClassifyValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_FLAGS].value.uint32 & FWP_CONDITION_FLAG_IS_REAUTHORIZE))
         PerformFlowAssociation(pMetadata,
                                (PC_FLOW_ASSOCIATION_DATA*)pFilter->providerContext->dataBuffer->data);
   }

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- ClassifyFlowAssociation() [Layer: %s][FilterID: %#I64x][Action: %#x][Rights: %#x][Absorb: %s]\n",
              KrnlHlprFwpsLayerIDToString(pClassifyValues->layerId),
              pFilter->filterId,
              pClassifyOut->actionType,
              pClassifyOut->rights,
              (pClassifyOut->flags & FWPS_CLASSIFY_OUT_FLAG_ABSORB) ? "TRUE" : "FALSE");

   return;
}

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)
