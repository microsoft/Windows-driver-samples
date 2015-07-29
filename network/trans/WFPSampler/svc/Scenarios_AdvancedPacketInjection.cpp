////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      Scenarios_AdvancedPacketInjection.cpp
//
//   Abstract:
//      This module contains functions which implements the ADVANCED_PACKET_INJECTION scenarios.
//
//   Naming Convention:
//
//      <Scope><Object><Action><Modifier>
//      <Scope><Object><Action>
//  
//      i.e.
//
//       <Scope>
//          {
//                                            - Function is likely visible to other modules
//            Prv                             - Function is private to this module.
//          }
//       <Object>
//          {
//            ScenarioAdvancedPacketInjection - Function pertains to all of the Advanced Packet 
//                                                 Injection Scenarios
//            RPC                             - Function is and RPC entry point.
//          }
//       <Action>
//          {
//            Add                             - Function adds objects
//            Remove                          - Function removes objects
//            Invoke                          - Function implements the scenario based on parameters 
//                                                 passed from the commandline interface 
//                                                 (WFPSampler.exe).
//          }
//       <Modifier>
//          {
//            FwpmObjects                     - Function acts on WFP objects.
//            ScenarioAdvancedPacketInjection - Function pertains to all of the Advanced Packet
//                                                 Injection Scenarios
//          }
//
//   Private Functions:
//      PrvScenarioAdvancedPacketInjectionAddFwpmObjects(),
//      PrvScenarioAdvancedPacketInjectionDeleteFwpmObjects(),
//      ScenarioAdvancedStreamInjectionAdd(),
//      ScenarioAdvancedStreamInjectionRemove(),
//
//   Public Functions:
//      RPCInvokeScenarioAdvancedPacketInjection(),
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

#include "Framework_WFPSamplerService.h" /// .

/**
 @private_function="PrvScenarioAdvancedPacketInjectionDeleteFwpmObjects"
 
   Purpose:  Function that disables the SCENARIO_ADVANCED_PACKET_INJECTION scenarios.           <br>
                                                                                                <br>
   Notes:    Scenario removes the filters using specified filtering conditions at the specified 
             layer. Associated callouts are removed as well.                                    <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 PrvScenarioAdvancedPacketInjectionDeleteFwpmObjects(_In_ const FWPM_FILTER* pFilter)
{
   ASSERT(pFilter);

   UINT32                              status                      = NO_ERROR;
   HANDLE                              engineHandle                = 0;
   HANDLE                              enumHandle                  = 0;
   UINT32                              entryCount                  = 0;
   FWPM_FILTER**                       ppFilters                   = 0;
   FWPM_FILTER_CONDITION*              pFilterConditions           = 0;
   UINT32                              numFilterConditions         = 0;
   FWPM_FILTER_ENUM_TEMPLATE           filterEnumTemplate          = {0};
   FWPM_PROVIDER_CONTEXT_ENUM_TEMPLATE providerContextEnumTemplate = {0};
   GUID                                calloutKey                  = {0};

   HlprFwpmFilterConditionPrune(pFilter->filterCondition,
                                pFilter->numFilterConditions,
                                &pFilterConditions,
                                &numFilterConditions);

   calloutKey          = WFPSAMPLER_CALLOUT_ADVANCED_PACKET_INJECTION;
   calloutKey.Data4[7] = HlprFwpmLayerGetIDByKey(&(pFilter->layerKey));                          /// Uniquely identifies the callout used

   providerContextEnumTemplate.providerContextType = FWPM_GENERAL_CONTEXT;

   filterEnumTemplate.providerKey             = (GUID*)&WFPSAMPLER_PROVIDER;
   filterEnumTemplate.layerKey                = pFilter->layerKey;
   filterEnumTemplate.enumType                = FWP_FILTER_ENUM_FULLY_CONTAINED;
   filterEnumTemplate.flags                   = FWP_FILTER_ENUM_FLAG_INCLUDE_BOOTTIME |
                                                FWP_FILTER_ENUM_FLAG_INCLUDE_DISABLED;
   filterEnumTemplate.numFilterConditions     = pFilterConditions ? numFilterConditions : pFilter->numFilterConditions;
   filterEnumTemplate.filterCondition         = pFilterConditions ? pFilterConditions : pFilter->filterCondition;
   filterEnumTemplate.providerContextTemplate = &providerContextEnumTemplate;
   filterEnumTemplate.actionMask              = FWP_ACTION_FLAG_CALLOUT;
   filterEnumTemplate.calloutKey              = &calloutKey;

   status = HlprFwpmEngineOpen(&engineHandle);
   HLPR_BAIL_ON_FAILURE(status);

   status = HlprFwpmFilterCreateEnumHandle(engineHandle,
                                           &filterEnumTemplate,
                                           &enumHandle);
   HLPR_BAIL_ON_FAILURE(status);

   status = HlprFwpmFilterEnum(engineHandle,
                               enumHandle,
                               0xFFFFFFFF,
                               &ppFilters,
                               &entryCount);
   HLPR_BAIL_ON_FAILURE(status);

   if(ppFilters)
   {
      for(UINT32 filterIndex = 0;
          filterIndex < entryCount;
          filterIndex++)
      {
         HlprFwpmFilterDeleteByKey(engineHandle,
                                   &(ppFilters[filterIndex]->filterKey));

         HlprFwpmCalloutDeleteByKey(engineHandle,
                                    &(ppFilters[filterIndex]->action.calloutKey));

         if(ppFilters[filterIndex]->flags & FWPM_FILTER_FLAG_HAS_PROVIDER_CONTEXT)
            HlprFwpmProviderContextDeleteByKey(engineHandle,
                                               &(ppFilters[filterIndex]->providerContextKey));
      }

      FwpmFreeMemory((VOID**)&ppFilters);
   }

   HLPR_BAIL_LABEL:

   if(engineHandle)
   {
      if(enumHandle)
         HlprFwpmFilterDestroyEnumHandle(engineHandle,
                                         &enumHandle);

      HlprFwpmEngineClose(&engineHandle);
   }

   HLPR_DELETE_ARRAY(pFilterConditions);

   return status;
}

/**
 @private_function="PrvScenarioAdvancedPacketInjectionAddFwpmObjects"
 
   Purpose:  Function that enables the SCENARIO_ADVANCED_PACKET_INJECTION scenarios.            <br>
                                                                                                <br>
   Notes:    Scenario adds a filter using specified filtering conditions to the specified layer. 
             This filter is associated with WFPSampler's default sublayer and provider.  The 
             appropriate callout is then added and associated with the filter.                  <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 PrvScenarioAdvancedPacketInjectionAddFwpmObjects(_In_ const FWPM_FILTER* pFilter,
                                                        _In_ const PC_ADVANCED_PACKET_INJECTION_DATA* pPCAdvancedPacketInjectionData)
{
   ASSERT(pFilter);
   ASSERT(pPCAdvancedPacketInjectionData);

   UINT32 status = NO_ERROR;

   if(pFilter->layerKey == FWPM_LAYER_INBOUND_IPPACKET_V4 ||
      pFilter->layerKey == FWPM_LAYER_INBOUND_IPPACKET_V6 ||
      pFilter->layerKey == FWPM_LAYER_OUTBOUND_IPPACKET_V4 ||
      pFilter->layerKey == FWPM_LAYER_OUTBOUND_IPPACKET_V6 ||
      pFilter->layerKey == FWPM_LAYER_IPFORWARD_V4 ||
      pFilter->layerKey == FWPM_LAYER_IPFORWARD_V6 ||
      pFilter->layerKey == FWPM_LAYER_INBOUND_TRANSPORT_V4 ||
      pFilter->layerKey == FWPM_LAYER_INBOUND_TRANSPORT_V6 ||
      pFilter->layerKey == FWPM_LAYER_OUTBOUND_TRANSPORT_V4 ||
      pFilter->layerKey == FWPM_LAYER_OUTBOUND_TRANSPORT_V6 ||
      pFilter->layerKey == FWPM_LAYER_DATAGRAM_DATA_V4 ||
      pFilter->layerKey == FWPM_LAYER_DATAGRAM_DATA_V6 ||
      pFilter->layerKey == FWPM_LAYER_INBOUND_ICMP_ERROR_V4 ||
      pFilter->layerKey == FWPM_LAYER_INBOUND_ICMP_ERROR_V6 ||
      pFilter->layerKey == FWPM_LAYER_OUTBOUND_ICMP_ERROR_V4 ||
      pFilter->layerKey == FWPM_LAYER_OUTBOUND_ICMP_ERROR_V6 ||
      pFilter->layerKey == FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4 ||
      pFilter->layerKey == FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6 ||
      pFilter->layerKey == FWPM_LAYER_ALE_AUTH_CONNECT_V4 ||
      pFilter->layerKey == FWPM_LAYER_ALE_AUTH_CONNECT_V6 ||
      pFilter->layerKey == FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4 ||
      pFilter->layerKey == FWPM_LAYER_ALE_FLOW_ESTABLISHED_V6

#if(NTDDI_VERSION >= NTDDI_WIN7)

      ||
      pFilter->layerKey == FWPM_LAYER_STREAM_PACKET_V4 ||
      pFilter->layerKey == FWPM_LAYER_STREAM_PACKET_V6
   
#if(NTDDI_VERSION >= NTDDI_WIN8)
   
      ||
      pFilter->layerKey == FWPM_LAYER_INBOUND_MAC_FRAME_ETHERNET ||
      pFilter->layerKey == FWPM_LAYER_OUTBOUND_MAC_FRAME_ETHERNET ||
      pFilter->layerKey == FWPM_LAYER_INBOUND_MAC_FRAME_NATIVE ||
      pFilter->layerKey == FWPM_LAYER_OUTBOUND_MAC_FRAME_NATIVE ||
      pFilter->layerKey == FWPM_LAYER_INGRESS_VSWITCH_ETHERNET ||
      pFilter->layerKey == FWPM_LAYER_EGRESS_VSWITCH_ETHERNET
   
#endif /// (NTDDI_VERSION >= NTDDI_WIN8)
#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

      )
   {
      HANDLE                engineHandle    = 0;
      FWP_BYTE_BLOB         byteBlob        = {0};
      FWPM_PROVIDER_CONTEXT providerContext = {0};
      FWPM_CALLOUT          callout         = {0};
      FWPM_FILTER           filter          = {0};

      RtlCopyMemory(&filter,
                    pFilter,
                    sizeof(FWPM_FILTER));

      status = HlprGUIDPopulate(&(providerContext.providerContextKey));
      HLPR_BAIL_ON_FAILURE(status);

      providerContext.displayData.name = L"WFPSampler's Advanced Packet Injection ProviderContext";
      providerContext.providerKey      = (GUID*)&WFPSAMPLER_PROVIDER;
      providerContext.type             = FWPM_GENERAL_CONTEXT;
      providerContext.dataBuffer       = &byteBlob;
      providerContext.dataBuffer->size = sizeof(PC_ADVANCED_PACKET_INJECTION_DATA);
      providerContext.dataBuffer->data = (UINT8*)pPCAdvancedPacketInjectionData;

      callout.calloutKey              = WFPSAMPLER_CALLOUT_ADVANCED_PACKET_INJECTION;
      callout.calloutKey.Data4[7]     = HlprFwpmLayerGetIDByKey(&(filter.layerKey));                /// Uniquely identifies the callout used
      callout.displayData.name        = L"WFPSampler's Advanced Packet Injection Callout";
      callout.displayData.description = L"Causes callout invocation which blindly injects traffic back";
      callout.flags                   = FWPM_CALLOUT_FLAG_USES_PROVIDER_CONTEXT;
      callout.providerKey             = (GUID*)&WFPSAMPLER_PROVIDER;
      callout.applicableLayer         = filter.layerKey;

      status = HlprGUIDPopulate(&(filter.filterKey));
      HLPR_BAIL_ON_FAILURE(status);

      filter.flags             |= FWPM_FILTER_FLAG_HAS_PROVIDER_CONTEXT;
      filter.providerKey        = (GUID*)&WFPSAMPLER_PROVIDER;
      filter.weight.type        = FWP_UINT8;
      filter.weight.uint8       = 0xF;
      filter.action.type        = FWP_ACTION_CALLOUT_TERMINATING;
      filter.action.calloutKey  = callout.calloutKey;
      filter.providerContextKey = providerContext.providerContextKey;

      if(filter.flags & FWPM_FILTER_FLAG_BOOTTIME ||
         filter.flags & FWPM_FILTER_FLAG_PERSISTENT)
      {
         providerContext.flags |= FWPM_PROVIDER_CONTEXT_FLAG_PERSISTENT;

         callout.flags |= FWPM_CALLOUT_FLAG_PERSISTENT;
      }

      status = HlprFwpmEngineOpen(&engineHandle);
      HLPR_BAIL_ON_FAILURE(status);

      status = HlprFwpmTransactionBegin(engineHandle);
      HLPR_BAIL_ON_FAILURE(status);

      status = HlprFwpmProviderContextAdd(engineHandle,
                                          &providerContext);
      HLPR_BAIL_ON_FAILURE(status);

      status = HlprFwpmCalloutAdd(engineHandle,
                                  &callout);
      HLPR_BAIL_ON_FAILURE(status);

      status = HlprFwpmFilterAdd(engineHandle,
                                 &filter);
      HLPR_BAIL_ON_FAILURE(status);

      status = HlprFwpmTransactionCommit(engineHandle);
      HLPR_BAIL_ON_FAILURE(status);

      HLPR_BAIL_LABEL:

      if(engineHandle)
      {
         if(status != NO_ERROR)
            HlprFwpmTransactionAbort(engineHandle);

         HlprFwpmEngineClose(&engineHandle);
      }
   }
   else
      status = (UINT32)FWP_E_INCOMPATIBLE_LAYER;

   return status;
}

/**
 @scenario_function="ScenarioAdvancedPacketInjectionRemove"
 
   Purpose:  Function that removes corresponding objects for a previously added 
             SCENARIO_ADVANCED_PACKET_INJECTION.                                                <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 ScenarioAdvancedPacketInjectionRemove(_In_ const FWPM_FILTER* pFilter)
{
   ASSERT(pFilter);

   return PrvScenarioAdvancedPacketInjectionDeleteFwpmObjects(pFilter);
}

/**
 @scenario_function="ScenarioAdvancedPacketInjectionAdd"

   Purpose:  Scenario which will blindly reinject the classified traffic.                       <br>
                                                                                                <br>
   Notes:    Adds a filter which references one of the 
             WFPSAMPLER_CALLOUT_ADVANCED_PACKET_INJECTION callouts for the provided layer.      <br>
                                                                                                <br>
             No data modification is made to the traffic, and checks are in place to prevent 
             infinite reinjection of the traffic.                                               <br>
                                                                                                <br>
             Ideal usage is to implement in the presence of a 3rd party firewall to see how they 
             coexist with another provider performing injection.                                <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 ScenarioAdvancedPacketInjectionAdd(_In_ const FWPM_FILTER* pFilter,
                                       _In_ const PC_ADVANCED_PACKET_INJECTION_DATA* pPCAdvancedPacketInjectionData)
{

   ASSERT(pFilter);
   ASSERT(pPCAdvancedPacketInjectionData);

   return PrvScenarioAdvancedPacketInjectionAddFwpmObjects(pFilter,
                                                           pPCAdvancedPacketInjectionData);
}

/**
 @rpc_function="RPCInvokeScenarioAdvancedPacketInjection"
 
   Purpose:  RPC exposed function used to dipatch the scenario routines for 
             SCENARIO_ADVANCED_PACKET_INJECTION.                                                <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
/* [fault_status][comm_status] */
error_status_t RPCInvokeScenarioAdvancedPacketInjection(/* [in] */ handle_t rpcBindingHandle,
                                                        /* [in] */ WFPSAMPLER_SCENARIO scenario,
                                                        /* [in] */ FWPM_CHANGE_TYPE changeType,
                                                        /* [ref][in] */ __RPC__in const FWPM_FILTER0* pFilter,
                                                        /* [unique][in] */ __RPC__in_opt const PC_ADVANCED_PACKET_INJECTION_DATA* pPCAdvancedPacketInjectionData)
{
   UNREFERENCED_PARAMETER(rpcBindingHandle);
   UNREFERENCED_PARAMETER(scenario);

   ASSERT(pFilter);
   ASSERT(scenario == SCENARIO_ADVANCED_PACKET_INJECTION);
   ASSERT(changeType < FWPM_CHANGE_TYPE_MAX);

   UINT32 status = NO_ERROR;

   if(changeType == FWPM_CHANGE_ADD)
   {
      if(pPCAdvancedPacketInjectionData)
         status = ScenarioAdvancedPacketInjectionAdd(pFilter,
                                                     pPCAdvancedPacketInjectionData);
      else
      {
         status = ERROR_INVALID_PARAMETER;

         HlprLogError(L"RpcInvokeScenarioAdvancedPacketInjection() [status: %#x][pPCAdvancedPacketInjectionData: %#x]",
                      status,
                      pPCAdvancedPacketInjectionData);
      }
   }
   else
      status = ScenarioAdvancedPacketInjectionRemove(pFilter);

   return status;
}
