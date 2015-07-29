////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      Scenarios_PendAuthorization.cpp
//
//   Abstract:
//      This module contains functions which implements the PEND_AUTHORIZATION scenarios.
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
//                                      - Function is likely visible to other modules.
//            Prv                       - Function is private to this module.
//          }
//       <Object>
//          {
//            ScenarioPendAuthorization - Function pertains to all of the Pend Authorization 
//                                           Scenarios.
//            RPC                       - Function is and RPC entry point.
//          }
//       <Action>
//          {
//            Add                       - Function adds objects.
//            Remove                    - Function removes objects.
//            Invoke                    - Function implements the scenario based on parameters
//                                                 passed from the commandline interface 
//                                                 (WFPSampler.exe).
//          }
//       <Modifier>
//          {
//            FwpmObjects               - Function acts on WFP objects.
//            ScenarioPendAuthorization - Function pertains to all of the Pend Authorization
//                                           Scenarios.
//          }
//
//   Private Functions:
//      PrvScenarioPendAuthorizationAddFwpmObjects(),
//      PrvScenarioPendAuthorizationDeleteFwpmObjects(),
//      ScenarioPendAuthorizationAdd(),
//      ScenarioPendAuthorizationRemove(),
//
//   Public Functions:
//      RPCInvokeScenarioPendAuthorization(),
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
 @private_function="PrvScenarioPendAuthorizationDeleteFwpmObjects"
 
   Purpose:  Function that disables the SCENARIO_PEND_AUTHORIZATION scenarios.                  <br>
                                                                                                <br>
   Notes:    Scenario removes the filters using specified filtering conditions at the specified 
             layer.  The associated callout and provider contexts are removed as well.          <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 PrvScenarioPendAuthorizationDeleteFwpmObjects(_In_ const FWPM_FILTER* pFilter)
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

   calloutKey          = WFPSAMPLER_CALLOUT_PEND_AUTHORIZATION;
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
 @private_function="PrvScenarioPendAuthorizationAddFwpmObjects"
 
   Purpose:  Function that enables the SCENARIO_PEND_AUTHORIZATION scenarios.                   <br>
                                                                                                <br>
   Notes:    Scenario adds a filter using specified filtering conditions to the specified layer. 
             This filter is associated with WFPSampler's default sublayer and provider.  The 
             appropriate callout and provider context is added and associated with the filter.  <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 PrvScenarioPendAuthorizationAddFwpmObjects(_In_ const FWPM_FILTER* pFilter,
                                                  _In_ const PC_PEND_AUTHORIZATION_DATA* pPCPendAuthorizationData)
{
   ASSERT(pFilter);
   ASSERT(pPCPendAuthorizationData);

   UINT32 status = NO_ERROR;

   if(pFilter->layerKey == FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V4 ||
      pFilter->layerKey == FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V6 ||
      pFilter->layerKey == FWPM_LAYER_ALE_AUTH_LISTEN_V4 ||
      pFilter->layerKey == FWPM_LAYER_ALE_AUTH_LISTEN_V6 ||
      pFilter->layerKey == FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4 ||
      pFilter->layerKey == FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6 ||
      pFilter->layerKey == FWPM_LAYER_ALE_AUTH_CONNECT_V4 ||
      pFilter->layerKey == FWPM_LAYER_ALE_AUTH_CONNECT_V6)
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

      providerContext.displayData.name        = L"WFPSampler's Pend Authorization ProviderContext";
      providerContext.displayData.description = L"Instructs the driver what final action to take on the pended authorization";      
      providerContext.providerKey             = (GUID*)&WFPSAMPLER_PROVIDER;
      providerContext.type                    = FWPM_GENERAL_CONTEXT;
      providerContext.dataBuffer              = &byteBlob;
      providerContext.dataBuffer->size        = sizeof(PC_PEND_AUTHORIZATION_DATA);
      providerContext.dataBuffer->data        = (UINT8*)pPCPendAuthorizationData;

      callout.calloutKey              = WFPSAMPLER_CALLOUT_PEND_AUTHORIZATION;
      callout.calloutKey.Data4[7]     = HlprFwpmLayerGetIDByKey(&(filter.layerKey));                /// Uniquely identifies the callout used
      callout.displayData.name        = L"WFPSampler's Pend Authorization Callout";
      callout.displayData.description = L"Pends the authorization request and completes out of band";
      callout.flags                   = FWPM_CALLOUT_FLAG_USES_PROVIDER_CONTEXT;
      callout.providerKey             = (GUID*)&WFPSAMPLER_PROVIDER;
      callout.applicableLayer         = filter.layerKey;

      status = HlprGUIDPopulate(&(filter.filterKey));
      HLPR_BAIL_ON_FAILURE(status);

      filter.flags             |= FWPM_FILTER_FLAG_HAS_PROVIDER_CONTEXT;
      filter.providerKey        = (GUID*)&WFPSAMPLER_PROVIDER;
      filter.weight.type        = FWP_UINT8;
      filter.weight.uint8       = 0xF;
      filter.action.type        = FWP_ACTION_CALLOUT_UNKNOWN;
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
 @scenario_function="ScenarioPendAuthorizationRemove"
 
   Purpose:  Function that removes corresponding objects for a previously added 
             SCENARIO_PEND_AUTHORIZATION.                                                       <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 ScenarioPendAuthorizationRemove(_In_ const FWPM_FILTER* pFilter)
{
   ASSERT(pFilter);

   return PrvScenarioPendAuthorizationDeleteFwpmObjects(pFilter);
}

/**
 @scenario_function="ScenarioPendAuthorizationAdd"
 
   Purpose:  Scenario which will delay and either block or permit an authorization attempt.     <br>
                                                                                                <br>
   Notes:    Adds a filter which references one of the 
             WFPSAMPLER_CALLOUT_PEND_AUTHORIZATION callouts for the provided layer.             <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 ScenarioPendAuthorizationAdd(_In_ const FWPM_FILTER* pFilter,
                                    _In_ const PC_PEND_AUTHORIZATION_DATA* pPCPendAuthorizationData)
{
   ASSERT(pFilter);
   ASSERT(pPCPendAuthorizationData);

   return PrvScenarioPendAuthorizationAddFwpmObjects(pFilter,
                                                     pPCPendAuthorizationData);
}

/**
 @rpc_function="RPCInvokeScenarioPendAuthorization"
 
   Purpose:  RPC exposed function used to dipatch the scenario routines for 
             SCENARIO_PEND_AUTHORIZATION.                                                       <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
/* [fault_status][comm_status] */
error_status_t RPCInvokeScenarioPendAuthorization(/* [in] */ handle_t rpcBindingHandle,
                                                  /* [in] */ WFPSAMPLER_SCENARIO scenario,
                                                  /* [in] */ FWPM_CHANGE_TYPE changeType,
                                                  /* [ref][in] */ __RPC__in const FWPM_FILTER0* pFilter,
                                                  /* [unique][in] */ __RPC__in_opt const PC_PEND_AUTHORIZATION_DATA* pPCPendAuthorizationData)
{
   UNREFERENCED_PARAMETER(rpcBindingHandle);
   UNREFERENCED_PARAMETER(scenario);

   ASSERT(pFilter);
   ASSERT(scenario == SCENARIO_PEND_AUTHORIZATION);
   ASSERT(changeType < FWPM_CHANGE_TYPE_MAX);

   UINT32 status = NO_ERROR;

   if(changeType == FWPM_CHANGE_ADD)
   {
      if(pPCPendAuthorizationData)
         status = ScenarioPendAuthorizationAdd(pFilter,
                                               pPCPendAuthorizationData);
      else
      {
         status = ERROR_INVALID_PARAMETER;

         HlprLogError(L"RPCInvokeScenarioPendAuthorization() [status: %#x][pPCPendAuthorizationData: %#p]",
                      status,
                      pPCPendAuthorizationData);
      }
   }
   else
      status = ScenarioPendAuthorizationRemove(pFilter);

   return status;
}
