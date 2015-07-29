////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      Scenarios_BasicPacketExamination.cpp
//
//   Abstract:
//      This module contains functions which implements the BASIC_PACKET_EXAMINATION scenarios.
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
//                                           - Function is likely visible to other modules
//            Prv                            - Function is private to this module.
//          }
//       <Object>
//          {
//            ScenarioBasicPacketExamination - Function pertains to all of the Basic Packet 
//                                                Examination Scenarios
//            RPC                            - Function is and RPC entry point.
//          }
//       <Action>
//          {
//            Add                            - Function adds objects or scenarios
//            Delete                         - Function deletes objects
//            Invoke                         - Function implements the scenario.
//            Remove                         - Function removes scenarios
//          }
//       <Modifier>
//          {
//            FwpmObjects                    - Function acts on WFP objects.
//            ScenarioBasicPacketExamination - Function pertains to all of the Basic Packet
//                                                Examination Scenarios
//          }
//
//   Private Functions:
//      PrvScenarioBasicPacketExaminationAddFwpmObjects(),
//      PrvScenarioBasicPacketExaminationDeleteFwpmObjects(),
//      ScenarioBasicPacketExaminationAdd(),
//      ScenarioBasicPacketExaminationRemove(),
//
//   Public Functions:
//      RPCInvokeScenarioBasicPacketExamination().
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Prune filters for enumeration and limit scenario to
//                                              only the supported layers 
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Framework_WFPSamplerService.h" /// .

/**
 @private_function="PrvScenarioBasicPacketExaminationDeleteFwpmObjects"
 
   Purpose:  Function that disables the SCENARIO_BASIC_PACKET_EXAMINATION scenarios.            <br>
                                                                                                <br>
   Notes:    Scenario removes the filters using specified filtering conditions at the 
             specified layer. Associated callouts are removed as well.                          <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 PrvScenarioBasicPacketExaminationDeleteFwpmObjects(_In_ const FWPM_FILTER* pFilter)
{
   ASSERT(pFilter);

   UINT32                    status              = NO_ERROR;
   HANDLE                    engineHandle        = 0;
   HANDLE                    enumHandle          = 0;
   UINT32                    entryCount          = 0;
   FWPM_FILTER**             ppFilters           = 0;
   FWPM_FILTER_CONDITION*    pFilterConditions   = 0;
   UINT32                    numFilterConditions = 0;
   FWPM_FILTER_ENUM_TEMPLATE filterEnumTemplate  = {0};
   GUID                      calloutKey          = {0};

   HlprFwpmFilterConditionPrune(pFilter->filterCondition,
                                pFilter->numFilterConditions,
                                &pFilterConditions,
                                &numFilterConditions);

   calloutKey          = WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION;
   calloutKey.Data4[7] = HlprFwpmLayerGetIDByKey(&(pFilter->layerKey));                          /// Uniquely identifies the callout used

   filterEnumTemplate.providerKey         = (GUID*)&WFPSAMPLER_PROVIDER;
   filterEnumTemplate.layerKey            = pFilter->layerKey;
   filterEnumTemplate.enumType            = FWP_FILTER_ENUM_FULLY_CONTAINED;
   filterEnumTemplate.flags               = FWP_FILTER_ENUM_FLAG_INCLUDE_BOOTTIME |
                                            FWP_FILTER_ENUM_FLAG_INCLUDE_DISABLED;
   filterEnumTemplate.numFilterConditions = pFilterConditions ? numFilterConditions : pFilter->numFilterConditions;
   filterEnumTemplate.filterCondition     = pFilterConditions ? pFilterConditions : pFilter->filterCondition;
   filterEnumTemplate.actionMask          = FWP_ACTION_FLAG_CALLOUT;
   filterEnumTemplate.calloutKey          = &calloutKey;

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
 @private_function="PrvScenarioBasicPacketExaminationAddFwpmObjects"
 
   Purpose:  Function that enables the SCENARIO_BASIC_PACKET_EXAMINATION scenarios.             <br>
                                                                                                <br>
   Notes:    Scenario adds a filter using specified filtering conditions to the specified 
             layer.  This filter is associated with WFPSampler's default provider and the 
             built-in inspection sublayer.  The appropriate callout is then added and 
             associated with the filter.                                                        <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 PrvScenarioBasicPacketExaminationAddFwpmObjects(_In_ const FWPM_FILTER* pFilter)
{
   ASSERT(pFilter);

   UINT32 status = NO_ERROR;

   if(pFilter->layerKey == FWPM_LAYER_INBOUND_IPPACKET_V4 ||
      pFilter->layerKey == FWPM_LAYER_INBOUND_IPPACKET_V4_DISCARD ||
      pFilter->layerKey == FWPM_LAYER_INBOUND_IPPACKET_V6 ||
      pFilter->layerKey == FWPM_LAYER_INBOUND_IPPACKET_V6_DISCARD ||
      pFilter->layerKey == FWPM_LAYER_OUTBOUND_IPPACKET_V4 ||
      pFilter->layerKey == FWPM_LAYER_OUTBOUND_IPPACKET_V4_DISCARD ||
      pFilter->layerKey == FWPM_LAYER_OUTBOUND_IPPACKET_V6 ||
      pFilter->layerKey == FWPM_LAYER_OUTBOUND_IPPACKET_V6_DISCARD ||
      pFilter->layerKey == FWPM_LAYER_IPFORWARD_V4 ||
      pFilter->layerKey == FWPM_LAYER_IPFORWARD_V4_DISCARD ||
      pFilter->layerKey == FWPM_LAYER_IPFORWARD_V6 ||
      pFilter->layerKey == FWPM_LAYER_IPFORWARD_V6_DISCARD ||
      pFilter->layerKey == FWPM_LAYER_INBOUND_TRANSPORT_V4 ||
      pFilter->layerKey == FWPM_LAYER_INBOUND_TRANSPORT_V4_DISCARD ||
      pFilter->layerKey == FWPM_LAYER_INBOUND_TRANSPORT_V6 ||
      pFilter->layerKey == FWPM_LAYER_INBOUND_TRANSPORT_V6_DISCARD ||
      pFilter->layerKey == FWPM_LAYER_OUTBOUND_TRANSPORT_V4 ||
      pFilter->layerKey == FWPM_LAYER_OUTBOUND_TRANSPORT_V4_DISCARD ||
      pFilter->layerKey == FWPM_LAYER_OUTBOUND_TRANSPORT_V6 ||
      pFilter->layerKey == FWPM_LAYER_OUTBOUND_TRANSPORT_V6_DISCARD ||
      pFilter->layerKey == FWPM_LAYER_STREAM_V4 ||
      pFilter->layerKey == FWPM_LAYER_STREAM_V4_DISCARD ||
      pFilter->layerKey == FWPM_LAYER_STREAM_V6 ||
      pFilter->layerKey == FWPM_LAYER_STREAM_V6_DISCARD ||
      pFilter->layerKey == FWPM_LAYER_DATAGRAM_DATA_V4 ||
      pFilter->layerKey == FWPM_LAYER_DATAGRAM_DATA_V4_DISCARD ||
      pFilter->layerKey == FWPM_LAYER_DATAGRAM_DATA_V6 ||
      pFilter->layerKey == FWPM_LAYER_DATAGRAM_DATA_V6_DISCARD ||
      pFilter->layerKey == FWPM_LAYER_INBOUND_ICMP_ERROR_V4 ||
      pFilter->layerKey == FWPM_LAYER_INBOUND_ICMP_ERROR_V4_DISCARD ||
      pFilter->layerKey == FWPM_LAYER_INBOUND_ICMP_ERROR_V6 ||
      pFilter->layerKey == FWPM_LAYER_INBOUND_ICMP_ERROR_V6_DISCARD ||
      pFilter->layerKey == FWPM_LAYER_OUTBOUND_ICMP_ERROR_V4 ||
      pFilter->layerKey == FWPM_LAYER_OUTBOUND_ICMP_ERROR_V4_DISCARD ||
      pFilter->layerKey == FWPM_LAYER_OUTBOUND_ICMP_ERROR_V6 ||
      pFilter->layerKey == FWPM_LAYER_OUTBOUND_ICMP_ERROR_V6_DISCARD ||
      pFilter->layerKey == FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V4 ||
      pFilter->layerKey == FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V4_DISCARD ||
      pFilter->layerKey == FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V6 ||
      pFilter->layerKey == FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V6_DISCARD ||
      pFilter->layerKey == FWPM_LAYER_ALE_AUTH_LISTEN_V4 ||
      pFilter->layerKey == FWPM_LAYER_ALE_AUTH_LISTEN_V4_DISCARD ||
      pFilter->layerKey == FWPM_LAYER_ALE_AUTH_LISTEN_V6 ||
      pFilter->layerKey == FWPM_LAYER_ALE_AUTH_LISTEN_V6_DISCARD ||
      pFilter->layerKey == FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4 ||
      pFilter->layerKey == FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4_DISCARD ||
      pFilter->layerKey == FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6 ||
      pFilter->layerKey == FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6_DISCARD ||
      pFilter->layerKey == FWPM_LAYER_ALE_AUTH_CONNECT_V4 ||
      pFilter->layerKey == FWPM_LAYER_ALE_AUTH_CONNECT_V4_DISCARD ||
      pFilter->layerKey == FWPM_LAYER_ALE_AUTH_CONNECT_V6 ||
      pFilter->layerKey == FWPM_LAYER_ALE_AUTH_CONNECT_V6_DISCARD ||
      pFilter->layerKey == FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4 ||
      pFilter->layerKey == FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4_DISCARD ||
      pFilter->layerKey == FWPM_LAYER_ALE_FLOW_ESTABLISHED_V6 ||
      pFilter->layerKey == FWPM_LAYER_ALE_FLOW_ESTABLISHED_V6_DISCARD

#if(NTDDI_VERSION >= NTDDI_WIN7)

      ||
      pFilter->layerKey == FWPM_LAYER_ALE_RESOURCE_RELEASE_V4 ||
      pFilter->layerKey == FWPM_LAYER_ALE_RESOURCE_RELEASE_V6 ||
      pFilter->layerKey == FWPM_LAYER_ALE_ENDPOINT_CLOSURE_V4 ||
      pFilter->layerKey == FWPM_LAYER_ALE_ENDPOINT_CLOSURE_V6 ||
      pFilter->layerKey == FWPM_LAYER_ALE_CONNECT_REDIRECT_V4 ||
      pFilter->layerKey == FWPM_LAYER_ALE_CONNECT_REDIRECT_V6 ||
      pFilter->layerKey == FWPM_LAYER_ALE_BIND_REDIRECT_V4 ||
      pFilter->layerKey == FWPM_LAYER_ALE_BIND_REDIRECT_V6 ||
      pFilter->layerKey == FWPM_LAYER_STREAM_PACKET_V4 ||
      pFilter->layerKey == FWPM_LAYER_STREAM_PACKET_V6
   
#if(NTDDI_VERSION >= NTDDI_WIN8)
   
      ||
      pFilter->layerKey == FWPM_LAYER_INBOUND_MAC_FRAME_ETHERNET ||
      pFilter->layerKey == FWPM_LAYER_OUTBOUND_MAC_FRAME_ETHERNET ||
      pFilter->layerKey == FWPM_LAYER_INBOUND_MAC_FRAME_NATIVE ||
      pFilter->layerKey == FWPM_LAYER_OUTBOUND_MAC_FRAME_NATIVE ||
      pFilter->layerKey == FWPM_LAYER_INGRESS_VSWITCH_ETHERNET ||
      pFilter->layerKey == FWPM_LAYER_EGRESS_VSWITCH_ETHERNET ||
      pFilter->layerKey == FWPM_LAYER_INGRESS_VSWITCH_TRANSPORT_V4 ||
      pFilter->layerKey == FWPM_LAYER_INGRESS_VSWITCH_TRANSPORT_V6 ||
      pFilter->layerKey == FWPM_LAYER_EGRESS_VSWITCH_TRANSPORT_V4 ||
      pFilter->layerKey == FWPM_LAYER_EGRESS_VSWITCH_TRANSPORT_V6

#endif /// (NTDDI_VERSION >= NTDDI_WIN8)
#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

      )
   {
      HANDLE       engineHandle = 0;
      FWPM_CALLOUT callout      = {0};
      FWPM_FILTER  filter       = {0};

      RtlCopyMemory(&filter,
                    pFilter,
                    sizeof(FWPM_FILTER));

      callout.calloutKey              = WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION;
      callout.calloutKey.Data4[7]     = HlprFwpmLayerGetIDByKey(&(filter.layerKey));                /// Uniquely identifies the layer the callout will use 
      callout.displayData.name        = L"WFPSampler's Basic Packet Examination Callout";
      callout.displayData.description = L"Causes callout invocation which examines traffic";
      callout.providerKey             = (GUID*)&WFPSAMPLER_PROVIDER;
      callout.applicableLayer         = filter.layerKey;

      if(filter.flags & FWPM_FILTER_FLAG_BOOTTIME ||
         filter.flags & FWPM_FILTER_FLAG_PERSISTENT)
         callout.flags |= FWPM_CALLOUT_FLAG_PERSISTENT;

      status = HlprGUIDPopulate(&(filter.filterKey));
      HLPR_BAIL_ON_FAILURE(status);

      filter.providerKey       = (GUID*)&WFPSAMPLER_PROVIDER;
      filter.subLayerKey       = FWPM_SUBLAYER_INSPECTION;
      filter.weight.type       = FWP_UINT8;
      filter.weight.uint8      = 0xF;
      filter.action.type       = FWP_ACTION_CALLOUT_INSPECTION;
      filter.action.calloutKey = callout.calloutKey;

      status = HlprFwpmEngineOpen(&engineHandle);
      HLPR_BAIL_ON_FAILURE(status);

      status = HlprFwpmTransactionBegin(engineHandle);
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
 @scenario_function="ScenarioBasicPacketExaminationRemove"
 
   Purpose:  Function that removes corresponding objects for a previously added 
             SCENARIO_BASIC_PACKET_EXAMINATION.                                                 <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
UINT32 ScenarioBasicPacketExaminationRemove(_In_ const FWPM_FILTER* pFilter)
{
   ASSERT(pFilter);

   return PrvScenarioBasicPacketExaminationDeleteFwpmObjects(pFilter);
}

/**
 @scenario_function="ScenarioBasicPacketExaminationAdd"
 
   Purpose:  Scenario which will blindly reinject the classified traffic.                       <br>
                                                                                                <br>
   Notes:    Adds a filter which references one of the 
             WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION callouts for the provided layer.       <br>
                                                                                                <br>
             No data modification is made to the traffic.  No injection occurs.                 <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
UINT32 ScenarioBasicPacketExaminationAdd(_In_ const FWPM_FILTER* pFilter)
{
   ASSERT(pFilter);

   return PrvScenarioBasicPacketExaminationAddFwpmObjects(pFilter);
}

/**
 @rpc_function="RPCInvokeScenarioBasicPacketExamination"
 
   Purpose:  RPC exposed function used to dipatch the scenario routines for 
             SCENARIO_BASIC_PACKET_EXAMINATION.                                                 <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
/* [fault_status][comm_status] */
error_status_t RPCInvokeScenarioBasicPacketExamination(/* [in] */ handle_t rpcBindingHandle,
                                                       /* [in] */ WFPSAMPLER_SCENARIO scenario,
                                                       /* [in] */ FWPM_CHANGE_TYPE changeType,
                                                       /* [ref][in] */ __RPC__in const FWPM_FILTER0* pFilter)
{
   UNREFERENCED_PARAMETER(rpcBindingHandle);
   UNREFERENCED_PARAMETER(scenario);

   ASSERT(pFilter);
   ASSERT(scenario == SCENARIO_BASIC_PACKET_EXAMINATION);
   ASSERT(changeType < FWPM_CHANGE_TYPE_MAX);

   UINT32 status = NO_ERROR;

   if(changeType == FWPM_CHANGE_ADD)
      status = ScenarioBasicPacketExaminationAdd(pFilter);
   else
      status = ScenarioBasicPacketExaminationRemove(pFilter);

   return status;
}
