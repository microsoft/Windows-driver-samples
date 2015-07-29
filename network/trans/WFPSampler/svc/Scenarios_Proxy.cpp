////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      Scenarios_Proxy.cpp
//
//   Abstract:
//      This module contains functions which implements the PROXY scenarios.
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
//                          - Function is likely visible to other modules.
//            Prv           - Function is private to this module.
//          }
//       <Object>
//          {
//            ScenarioProxy - Function pertains to all of the Proxy Scenarios.
//            RPC           - Function is and RPC entry point.
//          }
//       <Action>
//          {
//            Add           - Function adds objects.
//            Remove        - Function removes objects.
//            Invoke        - Function implements the scenario based on parameters passed from the 
//                               commandline interface (WFPSampler.exe).
//          }
//       <Modifier>
//          {
//            FwpmObjects   - Function acts on WFP objects.
//            ScenarioProxy - Function pertains to all of the Proxy Scenarios.
//          }
//
//   Private Functions:
//      PrvScenarioProxyAddFwpmObjects(),
//      PrvScenarioProxyDeleteFwpmObjects(),
//      ScenarioProxyAdd(),
//      ScenarioProxyRemove(),
//
//   Public Functions:
//      RPCInvokeScenarioProxy(),
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Prune filters for enumeration, fix legacy proxying by 
//                                              injection, and limit scenario to only the supported 
//                                              layers 
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Framework_WFPSamplerService.h" /// .

/**
 @private_function="PrvScenarioProxyDeleteFwpmObjects"
 
   Purpose:  Function that disables the SCENARIO_PROXY scenarios.                               <br>
                                                                                                <br>
   Notes:    Scenario removes the filters using specified filtering conditions at the specified 
             layer.  The associated callout and provider contexts are removed as well.          <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 PrvScenarioProxyDeleteFwpmObjects(_In_ const FWPM_FILTER* pFilter,
                                         _In_ const PC_PROXY_DATA* pPCProxyData)
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
   FWP_BYTE_ARRAY16                    pIPv6Addresses[2]           = {0};

   HlprFwpmFilterConditionPrune(pFilter->filterCondition,
                                pFilter->numFilterConditions,
                                &pFilterConditions,
                                &numFilterConditions);

#if(NTDDI_VERSION >= NTDDI_WIN7)

   if(pFilter->layerKey == FWPM_LAYER_ALE_CONNECT_REDIRECT_V4 ||
      pFilter->layerKey == FWPM_LAYER_ALE_CONNECT_REDIRECT_V6 ||
      pFilter->layerKey == FWPM_LAYER_ALE_BIND_REDIRECT_V4 ||
      pFilter->layerKey == FWPM_LAYER_ALE_BIND_REDIRECT_V6)
      calloutKey = WFPSAMPLER_CALLOUT_PROXY_BY_ALE_REDIRECT;
   else

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

   calloutKey          = WFPSAMPLER_CALLOUT_PROXY_BY_INJECTION;
   calloutKey.Data4[7] = HlprFwpmLayerGetIDByKey(&(pFilter->layerKey));                          /// Uniquely identifies the callout used

   providerContextEnumTemplate.providerContextType = FWPM_GENERAL_CONTEXT;

   filterEnumTemplate.providerKey             = (GUID*)&WFPSAMPLER_PROVIDER;
   filterEnumTemplate.layerKey                = pFilter->layerKey;
   filterEnumTemplate.enumType                = FWP_FILTER_ENUM_FULLY_CONTAINED;
   filterEnumTemplate.flags                   = FWP_FILTER_ENUM_FLAG_INCLUDE_BOOTTIME |
                                                FWP_FILTER_ENUM_FLAG_INCLUDE_DISABLED;
   filterEnumTemplate.numFilterConditions     = pFilter->numFilterConditions;
   filterEnumTemplate.numFilterConditions     = pFilterConditions ? numFilterConditions : pFilter->numFilterConditions;
   filterEnumTemplate.filterCondition         = pFilterConditions ? pFilterConditions : pFilter->filterCondition;
   filterEnumTemplate.actionMask              = FWP_ACTION_FLAG_CALLOUT;
   filterEnumTemplate.calloutKey              = &calloutKey;

   status = HlprFwpmEngineOpen(&engineHandle);
   HLPR_BAIL_ON_FAILURE(status);

   status = HlprFwpmFilterCreateEnumHandle(engineHandle,
                                           &(filterEnumTemplate),
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

         if(pFilter->layerKey != FWPM_LAYER_OUTBOUND_TRANSPORT_V4 &&
            pFilter->layerKey != FWPM_LAYER_OUTBOUND_TRANSPORT_V6 &&
            ppFilters[filterIndex]->flags & FWPM_FILTER_FLAG_HAS_PROVIDER_CONTEXT)
            HlprFwpmProviderContextDeleteByKey(engineHandle,
                                               &(ppFilters[filterIndex]->providerContextKey));
      }

      FwpmFreeMemory((VOID**)&ppFilters);

      ppFilters = 0;
   }

   HLPR_DELETE_ARRAY(pFilterConditions);

   numFilterConditions = 0;

   if(pFilter->layerKey == FWPM_LAYER_OUTBOUND_TRANSPORT_V4 ||
      pFilter->layerKey == FWPM_LAYER_OUTBOUND_TRANSPORT_V6)
   {
      GUID layerKey = FWPM_LAYER_INBOUND_IPPACKET_V4;

      if(HlprFwpmLayerIsIPv6(&(pFilter->layerKey)))
         layerKey = FWPM_LAYER_INBOUND_IPPACKET_V6;

      if(enumHandle)
         HlprFwpmFilterDestroyEnumHandle(engineHandle,
                                         &enumHandle);

      calloutKey.Data4[7] = HlprFwpmLayerGetIDByKey(&layerKey);                          /// Uniquely identifies the callout used

      filterEnumTemplate.layerKey                = layerKey;
      filterEnumTemplate.numFilterConditions     = 0;
      filterEnumTemplate.filterCondition         = 0;
      filterEnumTemplate.calloutKey              = &calloutKey;

      for(UINT32 index = 0;
          index < pFilter->numFilterConditions;
          index++)
      {
         if(HlprFwpmFilterConditionIsValidForLayer(&layerKey,
                                                   &(pFilter->filterCondition[index].fieldKey)))
            filterEnumTemplate.numFilterConditions++;
      }

      if(filterEnumTemplate.numFilterConditions)
      {
         UINT32 conditionIndex = 0;

         HLPR_NEW_ARRAY(filterEnumTemplate.filterCondition,
                        FWPM_FILTER_CONDITION,
                        pFilter->numFilterConditions);
         HLPR_BAIL_ON_ALLOC_FAILURE(filterEnumTemplate.filterCondition,
                                    status);

         for(UINT32 index = 0;
             index < pFilter->numFilterConditions;
             index++)
         {
            if(conditionIndex < filterEnumTemplate.numFilterConditions &&
               HlprFwpmFilterConditionIsValidForLayer(&layerKey,
                                                      &(pFilter->filterCondition[index].fieldKey)))
            {
               RtlCopyMemory(&(filterEnumTemplate.filterCondition[conditionIndex]),
                             &(pFilter->filterCondition[index]),
                             sizeof(FWPM_FILTER_CONDITION));

               if(pPCProxyData->flags & PCPDF_PROXY_LOCAL_ADDRESS &&
                  filterEnumTemplate.filterCondition[conditionIndex].fieldKey == FWPM_CONDITION_IP_LOCAL_ADDRESS)
               {
                  if(filterEnumTemplate.filterCondition[conditionIndex].conditionValue.type == FWP_UINT32)
                  {
                     UINT32 ipv4Address = 0;

                     RtlCopyMemory(&ipv4Address,
                                   pPCProxyData->proxyLocalAddress.pIPv4,
                                   IPV4_ADDRESS_LENGTH);

                     ipv4Address = ntohl(ipv4Address);

                     filterEnumTemplate.filterCondition[conditionIndex].conditionValue.uint32 = ipv4Address;
                  }
                  else
                  {
                     filterEnumTemplate.filterCondition[conditionIndex].conditionValue.byteArray16 = &(pIPv6Addresses[0]);

                     RtlCopyMemory(filterEnumTemplate.filterCondition[conditionIndex].conditionValue.byteArray16->byteArray16,
                                   pPCProxyData->proxyLocalAddress.pIPv6,
                                   IPV6_ADDRESS_LENGTH);
                  }
               }
               else if(pPCProxyData->flags & PCPDF_PROXY_REMOTE_ADDRESS &&
                       filterEnumTemplate.filterCondition[conditionIndex].fieldKey == FWPM_CONDITION_IP_REMOTE_ADDRESS)
               {
                  if(filterEnumTemplate.filterCondition[conditionIndex].conditionValue.type == FWP_UINT32)
                  {
                     UINT32 ipv4Address = 0;

                     RtlCopyMemory(&ipv4Address,
                                   pPCProxyData->proxyRemoteAddress.pIPv4,
                                   IPV4_ADDRESS_LENGTH);

                     ipv4Address = ntohl(ipv4Address);

                     filterEnumTemplate.filterCondition[conditionIndex].conditionValue.uint32 = ipv4Address;
                  }
                  else
                  {
                     filterEnumTemplate.filterCondition[conditionIndex].conditionValue.byteArray16 = &(pIPv6Addresses[0]);

                     RtlCopyMemory(filterEnumTemplate.filterCondition[conditionIndex].conditionValue.byteArray16->byteArray16,
                                   pPCProxyData->proxyRemoteAddress.pIPv6,
                                   IPV6_ADDRESS_LENGTH);
                  }
               }

               conditionIndex++;
            }
         }
      }

      HlprFwpmFilterConditionPrune(pFilter->filterCondition,
                                   pFilter->numFilterConditions,
                                   &pFilterConditions,
                                   &numFilterConditions);
      if(pFilterConditions)
      {
         HLPR_DELETE_ARRAY(filterEnumTemplate.filterCondition);

         filterEnumTemplate.numFilterConditions = numFilterConditions;
         filterEnumTemplate.filterCondition     = pFilterConditions;
      }

      status = HlprFwpmFilterCreateEnumHandle(engineHandle,
                                              &filterEnumTemplate,
                                              &enumHandle);
      HLPR_BAIL_ON_FAILURE_2(status);

      status = HlprFwpmFilterEnum(engineHandle,
                                  enumHandle,
                                  0xFFFFFFFF,
                                  &ppFilters,
                                  &entryCount);
      HLPR_BAIL_ON_FAILURE_2(status);

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

            if((ppFilters[filterIndex]->layerKey == FWPM_LAYER_INBOUND_IPPACKET_V4 ||
               ppFilters[filterIndex]->layerKey == FWPM_LAYER_INBOUND_IPPACKET_V6) &&
               ppFilters[filterIndex]->flags & FWPM_FILTER_FLAG_HAS_PROVIDER_CONTEXT)
               HlprFwpmProviderContextDeleteByKey(engineHandle,
                                                  &(ppFilters[filterIndex]->providerContextKey));
         }

         FwpmFreeMemory((VOID**)&ppFilters);

         ppFilters = 0;
      }

      HLPR_BAIL_LABEL_2:

      HLPR_DELETE_ARRAY(filterEnumTemplate.filterCondition);
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
 @private_function="PrvScenarioProxyAddFwpmObjects"
 
   Purpose:  Function that enables the SCENARIO_PROXY scenarios.                                <br>
                                                                                                <br>
   Notes:    Scenario adds a filter using specified filtering conditions to the specified layer. 
             This filter is associated with WFPSampler's default sublayer and provider.  The 
             appropriate callout and provider context are added and associated with the filter. <br>
                                                                                                <br>
             If proxying by injection, then both the OUTBOUND_TRANSPORT and INBOUND_IPPACKET 
             objects are added.  This has the limitation of not working with IPsec, however 
             proxing using INBOUND_TRANSPORT has the limitation of not working with TCP and 
             connected UDP.                                                                     <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 PrvScenarioProxyAddFwpmObjects(_In_ const FWPM_FILTER* pFilter,
                                      _In_ const PC_PROXY_DATA* pPCProxyData)
{
   ASSERT(pFilter);
   ASSERT(pPCProxyData);

   UINT32 status = NO_ERROR;

   if(pFilter->layerKey == FWPM_LAYER_OUTBOUND_TRANSPORT_V4 ||
      pFilter->layerKey == FWPM_LAYER_OUTBOUND_TRANSPORT_V6

#if(NTDDI_VERSION >= NTDDI_WIN7)

      ||
      pFilter->layerKey == FWPM_LAYER_ALE_CONNECT_REDIRECT_V4 ||
      pFilter->layerKey == FWPM_LAYER_ALE_CONNECT_REDIRECT_V6 ||
      pFilter->layerKey == FWPM_LAYER_ALE_BIND_REDIRECT_V4 ||
      pFilter->layerKey == FWPM_LAYER_ALE_BIND_REDIRECT_V6

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)
     )
   {
      HANDLE                engineHandle      = 0;
      BOOLEAN               hasNetworkFilter  = FALSE;
      FWP_BYTE_BLOB         byteBlob          = {0};
      FWPM_PROVIDER_CONTEXT providerContext   = {0};
      FWPM_CALLOUT          outboundCallout   = {0};
      FWPM_CALLOUT          inboundCallout    = {0};
      FWPM_FILTER           outboundFilter    = {0};
      FWPM_FILTER           inboundFilter     = {0};
      FWP_BYTE_ARRAY16      pIPv6Addresses[2] = {0};

      RtlCopyMemory(&outboundFilter,
                    pFilter,
                    sizeof(FWPM_FILTER));

      status = HlprGUIDPopulate(&(providerContext.providerContextKey));
      HLPR_BAIL_ON_FAILURE(status);

      providerContext.displayData.name        = L"WFPSampler's Proxy ProviderContext";
      providerContext.displayData.description = L"Instructs the driver where to proxy the socket or connection";      
      providerContext.providerKey             = (GUID*)&WFPSAMPLER_PROVIDER;
      providerContext.type                    = FWPM_GENERAL_CONTEXT;
      providerContext.dataBuffer              = &byteBlob;
      providerContext.dataBuffer->size        = sizeof(PC_PROXY_DATA);
      providerContext.dataBuffer->data        = (UINT8*)pPCProxyData;

#if(NTDDI_VERSION >= NTDDI_WIN7)

      if(pFilter->layerKey == FWPM_LAYER_ALE_CONNECT_REDIRECT_V4 ||
         pFilter->layerKey == FWPM_LAYER_ALE_CONNECT_REDIRECT_V6 ||
         pFilter->layerKey == FWPM_LAYER_ALE_BIND_REDIRECT_V4 ||
         pFilter->layerKey == FWPM_LAYER_ALE_BIND_REDIRECT_V6)
         outboundCallout.calloutKey = WFPSAMPLER_CALLOUT_PROXY_BY_ALE_REDIRECT;
      else

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

      outboundCallout.calloutKey              = WFPSAMPLER_CALLOUT_PROXY_BY_INJECTION;
      outboundCallout.calloutKey.Data4[7]     = HlprFwpmLayerGetIDByKey(&(outboundFilter.layerKey));                /// Uniquely identifies the callout used
      outboundCallout.displayData.name        = L"WFPSampler's Proxy Callout";
      outboundCallout.displayData.description = L"Proxies the socket or connection to the designated destination";
      outboundCallout.flags                   = FWPM_CALLOUT_FLAG_USES_PROVIDER_CONTEXT;
      outboundCallout.providerKey             = (GUID*)&WFPSAMPLER_PROVIDER;
      outboundCallout.applicableLayer         = outboundFilter.layerKey;

      status = HlprGUIDPopulate(&(outboundFilter.filterKey));
      HLPR_BAIL_ON_FAILURE(status);

      outboundFilter.flags             |= FWPM_FILTER_FLAG_HAS_PROVIDER_CONTEXT;
      outboundFilter.providerKey        = (GUID*)&WFPSAMPLER_PROVIDER;
      outboundFilter.weight.type        = FWP_UINT8;
      outboundFilter.weight.uint8       = 0xF;
      outboundFilter.action.type        = FWP_ACTION_CALLOUT_UNKNOWN;
      outboundFilter.action.calloutKey  = outboundCallout.calloutKey;
      outboundFilter.providerContextKey = providerContext.providerContextKey;

      if(outboundFilter.flags & FWPM_FILTER_FLAG_BOOTTIME ||
         outboundFilter.flags & FWPM_FILTER_FLAG_PERSISTENT)
      {
         providerContext.flags |= FWPM_PROVIDER_CONTEXT_FLAG_PERSISTENT;

         outboundCallout.flags |= FWPM_CALLOUT_FLAG_PERSISTENT;
      }

      if(pFilter->layerKey == FWPM_LAYER_OUTBOUND_TRANSPORT_V4 ||
         pFilter->layerKey == FWPM_LAYER_OUTBOUND_TRANSPORT_V6)
      {
         GUID layerKey = FWPM_LAYER_INBOUND_IPPACKET_V4;

         if(HlprFwpmLayerIsIPv6(&(pFilter->layerKey)))
            layerKey = FWPM_LAYER_INBOUND_IPPACKET_V6;

         RtlCopyMemory(&inboundCallout,
                       &outboundCallout,
                       sizeof(FWPM_CALLOUT));

         inboundCallout.calloutKey.Data4[7] = HlprFwpmLayerGetIDByKey(&layerKey);                /// Uniquely identifies the callout used
         inboundCallout.applicableLayer     = layerKey;

         RtlCopyMemory(&inboundFilter,
                       &outboundFilter,
                       sizeof(FWPM_FILTER));

         inboundFilter.layerKey            = layerKey;
         inboundFilter.action.calloutKey   = inboundCallout.calloutKey;
         inboundFilter.numFilterConditions = 0;
         inboundFilter.filterCondition     = 0;

         hasNetworkFilter = TRUE;

         status = HlprGUIDPopulate(&(inboundFilter.filterKey));
         HLPR_BAIL_ON_FAILURE(status);

         for(UINT32 index = 0;
             index < outboundFilter.numFilterConditions;
             index++)
         {
            if(HlprFwpmFilterConditionIsValidForLayer(&layerKey,
                                                      &(outboundFilter.filterCondition[index].fieldKey)))
               inboundFilter.numFilterConditions++;
         }

         if(inboundFilter.numFilterConditions)
         {
            UINT32 conditionIndex = 0;

            HLPR_NEW_ARRAY(inboundFilter.filterCondition,
                           FWPM_FILTER_CONDITION,
                           inboundFilter.numFilterConditions);
            HLPR_BAIL_ON_ALLOC_FAILURE(inboundFilter.filterCondition,
                                       status);

            for(UINT32 index = 0;
                index < outboundFilter.numFilterConditions;
                index++)
            {
               if(conditionIndex < inboundFilter.numFilterConditions &&
                  HlprFwpmFilterConditionIsValidForLayer(&layerKey,
                                                         &(outboundFilter.filterCondition[index].fieldKey)))
               {
                  RtlCopyMemory(&(inboundFilter.filterCondition[conditionIndex]),
                                &(outboundFilter.filterCondition[index]),
                                sizeof(FWPM_FILTER_CONDITION));

                  if(pPCProxyData->flags & PCPDF_PROXY_LOCAL_ADDRESS &&
                     inboundFilter.filterCondition[conditionIndex].fieldKey == FWPM_CONDITION_IP_LOCAL_ADDRESS)
                  {
                     if(inboundFilter.filterCondition[conditionIndex].conditionValue.type == FWP_UINT32)
                     {
                        UINT32 ipv4Address = 0;

                        RtlCopyMemory(&ipv4Address,
                                      pPCProxyData->proxyLocalAddress.pIPv4,
                                      IPV4_ADDRESS_LENGTH);

                        ipv4Address = ntohl(ipv4Address);

                        inboundFilter.filterCondition[conditionIndex].conditionValue.uint32 = ipv4Address;
                     }
                     else
                     {
                        inboundFilter.filterCondition[conditionIndex].conditionValue.byteArray16 = &(pIPv6Addresses[0]);

                        RtlCopyMemory(inboundFilter.filterCondition[conditionIndex].conditionValue.byteArray16->byteArray16,
                                      pPCProxyData->proxyLocalAddress.pIPv6,
                                      IPV6_ADDRESS_LENGTH);
                     }
                  }
                  else if(pPCProxyData->flags & PCPDF_PROXY_REMOTE_ADDRESS &&
                          inboundFilter.filterCondition[conditionIndex].fieldKey == FWPM_CONDITION_IP_REMOTE_ADDRESS)
                  {
                     if(inboundFilter.filterCondition[conditionIndex].conditionValue.type == FWP_UINT32)
                     {
                        UINT32 ipv4Address = 0;

                        RtlCopyMemory(&ipv4Address,
                                      pPCProxyData->proxyRemoteAddress.pIPv4,
                                      IPV4_ADDRESS_LENGTH);

                        ipv4Address = ntohl(ipv4Address);

                        inboundFilter.filterCondition[conditionIndex].conditionValue.uint32 = ipv4Address;
                     }
                     else
                     {
                        inboundFilter.filterCondition[conditionIndex].conditionValue.byteArray16 = &(pIPv6Addresses[0]);

                        RtlCopyMemory(inboundFilter.filterCondition[conditionIndex].conditionValue.byteArray16->byteArray16,
                                      pPCProxyData->proxyRemoteAddress.pIPv6,
                                      IPV6_ADDRESS_LENGTH);
                     }
                  }

                  conditionIndex++;
               }
            }
         }
      }

      status = HlprFwpmEngineOpen(&engineHandle);
      HLPR_BAIL_ON_FAILURE(status);

      status = HlprFwpmTransactionBegin(engineHandle);
      HLPR_BAIL_ON_FAILURE(status);

      status = HlprFwpmProviderContextAdd(engineHandle,
                                          &providerContext);
      HLPR_BAIL_ON_FAILURE(status);

      status = HlprFwpmCalloutAdd(engineHandle,
                                  &outboundCallout);
      HLPR_BAIL_ON_FAILURE(status);

      status = HlprFwpmFilterAdd(engineHandle,
                                 &outboundFilter);
      HLPR_BAIL_ON_FAILURE(status);

      if(pFilter->layerKey == FWPM_LAYER_OUTBOUND_TRANSPORT_V4 ||
         pFilter->layerKey == FWPM_LAYER_OUTBOUND_TRANSPORT_V6)
      {
         status = HlprFwpmCalloutAdd(engineHandle,
                                     &inboundCallout);
         HLPR_BAIL_ON_FAILURE(status);

         status = HlprFwpmFilterAdd(engineHandle,
                                    &inboundFilter);
         HLPR_BAIL_ON_FAILURE(status);
      }

      status = HlprFwpmTransactionCommit(engineHandle);
      HLPR_BAIL_ON_FAILURE(status);

      HLPR_BAIL_LABEL:

      if(engineHandle)
      {
         if(status != NO_ERROR)
            HlprFwpmTransactionAbort(engineHandle);

         HlprFwpmEngineClose(&engineHandle);
      }

      if(hasNetworkFilter)
      {
         HLPR_DELETE_ARRAY(inboundFilter.filterCondition);
      }
   }
   else
      status = (UINT32)FWP_E_INCOMPATIBLE_LAYER;

   return status;
}

/**
 @scenario_function="ScenarioProxyRemove"
 
   Purpose:  Function that removes corresponding objects for a previously added SCENARIO_PROXY. <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 ScenarioProxyRemove(_In_ const FWPM_FILTER* pFilter,
                           _In_ const PC_PROXY_DATA* pPCProxyData)
{
   ASSERT(pFilter);
   ASSERT(pPCProxyData);

   return PrvScenarioProxyDeleteFwpmObjects(pFilter,
                                            pPCProxyData);
}

/**
 @scenario_function="ScenarioProxyAdd"
 
   Purpose:  Scenario which will proxy either a connection or a socket to a new endpoint.       <br>
                                                                                                <br>
   Notes:    Adds a filter which references one of the WFPSAMPLER_CALLOUT_PROXY callouts for 
             the provided layer.                                                                <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 ScenarioProxyAdd(_In_ const FWPM_FILTER* pFilter,
                        _In_ const PC_PROXY_DATA* pPCProxyData)
{
   ASSERT(pFilter);
   ASSERT(pPCProxyData);

   return PrvScenarioProxyAddFwpmObjects(pFilter,
                                         pPCProxyData);
}

/**
 @rpc_function="RPCInvokeScenarioProxy"
 
   Purpose:  RPC exposed function used to dipatch the scenario routines for SCENARIO_PROXY.     <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
/* [fault_status][comm_status] */
error_status_t RPCInvokeScenarioProxy(/* [in] */ handle_t rpcBindingHandle,
                                      /* [in] */ WFPSAMPLER_SCENARIO scenario,
                                      /* [in] */ FWPM_CHANGE_TYPE changeType,
                                      /* [ref][in] */ __RPC__in const FWPM_FILTER0* pFilter,
                                      /* [ref][in] */ __RPC__in const PC_PROXY_DATA* pPCProxyData)
{
   UNREFERENCED_PARAMETER(rpcBindingHandle);
   UNREFERENCED_PARAMETER(scenario);

   ASSERT(pFilter);
   ASSERT(pPCProxyData);
   ASSERT(scenario == SCENARIO_PROXY);
   ASSERT(changeType < FWPM_CHANGE_TYPE_MAX);

   UINT32 status = NO_ERROR;

   if(changeType == FWPM_CHANGE_ADD)
   {
      if(pPCProxyData)
         status = ScenarioProxyAdd(pFilter,
                                   pPCProxyData);
      else
         status = ERROR_INVALID_PARAMETER;
   }
   else
   {
      if(pPCProxyData)
         status = ScenarioProxyRemove(pFilter,
                                      pPCProxyData);
      else
         status = ERROR_INVALID_PARAMETER;
   }

   if(status != NO_ERROR)
      HlprLogError(L"RPCInvokeScenarioProxy() [status: %#x][pPCProxyData: %#x]",
                   status,
                   pPCProxyData);

   return status;
}
