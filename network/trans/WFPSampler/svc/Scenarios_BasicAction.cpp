////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      Scenarios_BasicAction.cpp
//
//   Abstract:
//      This module contains functions which implements the BASIC_ACTION_* scenarios.
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
//                                        - Function is likely visible to other modules.
//            Prv                         - Function is private to this module.
//          }
//       <Object>
//          {
//            ScenarioBasicActionBlock    - Function pertains to the Basic Action Block Scenario.
//            ScenarioBasicActionContinue - Function pertains to the Basic Action Continue Scenario.
//            ScenarioBasicActionPermit   - Function pertains to the Basic Action Permit Scenario.
//            ScenarioBasicActionRandom   - Function pertains to the Basic Action Random Scenario.
//            RPC                         - Function is and RPC entry point.
//          }
//       <Action>
//          {
//            Add                         - Function adds objects
//            Remove                      - Function removes objects
//            Invoke                      - Function implements the scenario based on parameters 
//                                             passed from the commandline interface 
//                                             (WFPSampler.exe).
//          }
//       <Modifier>
//          {
//            FwpmObjects               - Function acts on WFP objects.
//            ScenarioBasicAction       - Function pertains to all of the Basic Action Scenarios.
//          }
//
//   Private Functions:
//      PrvScenarioBasicActionAddFwpmObjects(),
//      PrvScenarioBasicActionDeleteFwpmObjects(),
//      ScenarioBasicActionBlockRemove(),
//      ScenarioBasicActionBlockAdd(),
//      ScenarioBasicActionContinueRemove(),
//      ScenarioBasicActionContinueAdd(),
//      ScenarioBasicActionPermitRemove(),
//      ScenarioBasicActionPermitAdd(),
//      ScenarioBasicActionRandomRemove(),
//      ScenarioBasicActionRandomAdd(),
//
//   Public Functions:
//      RPCInvokeScenarioBasicAction().
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Prune filters for enumeration and handle WinBlue Fast 
//                                              layers.
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Framework_WFPSamplerService.h" /// .

/**
 @private_function="PrvScenarioBasicActionDeleteFwpmObjects"
 
   Purpose:  Function that disables the SCENARIO_BASIC_ACTION_* scenarios.                      <br>
                                                                                                <br>
   Notes:    Scenario removes the filters using specified filtering conditions at the specified 
             layer.  If no layer is provided, then a default layer is used. If associated, the 
             callout and provider contexts are removed as well.                                 <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 PrvScenarioBasicActionDeleteFwpmObjects(_In_ const FWPM_FILTER* pFilter,
                                               _In_ FWP_ACTION_TYPE actionType)
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

   HlprFwpmFilterConditionPrune(pFilter->filterCondition,
                                pFilter->numFilterConditions,
                                &pFilterConditions,
                                &numFilterConditions);

   filterEnumTemplate.providerKey         = (GUID*)&WFPSAMPLER_PROVIDER;
   filterEnumTemplate.layerKey            = pFilter->layerKey;
   filterEnumTemplate.enumType            = FWP_FILTER_ENUM_FULLY_CONTAINED;
   filterEnumTemplate.flags               = FWP_FILTER_ENUM_FLAG_INCLUDE_BOOTTIME |
                                            FWP_FILTER_ENUM_FLAG_INCLUDE_DISABLED;
   filterEnumTemplate.numFilterConditions = pFilterConditions ? numFilterConditions : pFilter->numFilterConditions;
   filterEnumTemplate.filterCondition     = pFilterConditions ? pFilterConditions : pFilter->filterCondition;
   filterEnumTemplate.actionMask          = 0xFFFFFFFF;

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
         if(ppFilters[filterIndex]->action.type == actionType ||
            ppFilters[filterIndex]->action.type & FWP_ACTION_FLAG_CALLOUT)
         {
            HlprFwpmFilterDeleteByKey(engineHandle,
                                      &(ppFilters[filterIndex]->filterKey));

            if(ppFilters[filterIndex]->flags & FWPM_FILTER_FLAG_HAS_PROVIDER_CONTEXT)
               HlprFwpmProviderContextDeleteByKey(engineHandle,
                                                  &(ppFilters[filterIndex]->providerContextKey));

            if(ppFilters[filterIndex]->action.type & FWP_ACTION_FLAG_CALLOUT)
               HlprFwpmCalloutDeleteByKey(engineHandle,
                                          &(ppFilters[filterIndex]->action.calloutKey));
         }
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
 @private_function="PrvScenarioBasicActionAddFwpmObjects"
 
   Purpose:  Function that enables the SCENARIO_BASIC_ACTION_* scenarios.                       <br>
                                                                                                <br>
   Notes:    Scenario adds a filter using specified filtering conditions to the specified layer. 
             If no layer is provided, then a default layer is used.  This filter is associated 
             with WFPSampler's default sublayer and provider.  If required, the appropriate 
             callout and provider context is added and associated with the filter.              <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 PrvScenarioBasicActionAddFwpmObjects(_In_ const FWPM_FILTER* pFilter,
                                            _In_ FWP_ACTION_TYPE actionType = 0,
                                            _In_opt_ const PC_BASIC_ACTION_DATA* pPCBasicActionData = 0)
{
   ASSERT(pFilter);

   UINT32                 status            = NO_ERROR;
   HANDLE                 engineHandle      = 0;
   FWPM_CALLOUT*          pCallout          = 0;
   FWPM_PROVIDER_CONTEXT* pProviderContext  = 0;
   FWP_BYTE_BLOB*         pByteBlob         = 0;
   FWPM_FILTER            filter            = {0};

   RtlCopyMemory(&filter,
                 pFilter,
                 sizeof(FWPM_FILTER));

#if(NTDDI_VERSION >= NTDDI_WINBLUE)

   /// It is not advised to use these layers as they are meant for Microsoft's internal use.
   /// Trying to use static filtering will yield FWP_E_INCOMPATIBLE_LAYER when adding the filter.
   if(pFilter->layerKey == FWPM_LAYER_INBOUND_TRANSPORT_FAST ||
      pFilter->layerKey == FWPM_LAYER_OUTBOUND_TRANSPORT_FAST ||
      pFilter->layerKey == FWPM_LAYER_INBOUND_MAC_FRAME_NATIVE_FAST ||
      pFilter->layerKey == FWPM_LAYER_OUTBOUND_MAC_FRAME_NATIVE_FAST)
   {
      /// Callouts are not publicly supported for these layers.
      if(filter.action.type & FWP_ACTION_FLAG_CALLOUT)
      {
         status = (UINT32)FWP_E_INCOMPATIBLE_LAYER;

         HLPR_BAIL;
      }

      filter.numFilterConditions = 0;
      filter.filterCondition     = 0;
   }

#endif /// (NTDDI_VERSION >= NTDDI_WINBLUE)

   status = HlprGUIDPopulate(&(filter.filterKey));
   HLPR_BAIL_ON_FAILURE(status);

   filter.providerKey  = (GUID*)&WFPSAMPLER_PROVIDER;
   filter.weight.type  = FWP_UINT8;
   filter.weight.uint8 = filter.numFilterConditions ? 0xF : 0x0;

   if(filter.action.type & FWP_ACTION_FLAG_CALLOUT)
   {
      FWP_ACTION_TYPE newActionType = FWP_ACTION_CALLOUT_TERMINATING; 

      HLPR_NEW(pCallout,
               FWPM_CALLOUT);
      HLPR_BAIL_ON_ALLOC_FAILURE(pCallout,
                                 status);

      if(actionType == FWP_ACTION_BLOCK)
      {
         pCallout->calloutKey              = WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK;
         pCallout->displayData.name        = L"WFPSampler's Basic Block Callout";
         pCallout->displayData.description = L"Causes callout invocation which returns FWP_ACTION_BLOCK";

         filter.flags |=  FWPM_FILTER_FLAG_CLEAR_ACTION_RIGHT;
      }
      else if(actionType == FWP_ACTION_CONTINUE)
      {
         pCallout->calloutKey              = WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE;
         pCallout->displayData.name        = L"WFPSampler's Basic Continue Callout";
         pCallout->displayData.description = L"Causes callout invocation which returns FWP_ACTION_CONTINUE";

         newActionType = FWP_ACTION_CALLOUT_UNKNOWN;
      }
      else if(actionType == FWP_ACTION_PERMIT)
      {
         pCallout->calloutKey              = WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT;
         pCallout->displayData.name        = L"WFPSampler's Basic Permit Callout";
         pCallout->displayData.description = L"Causes callout invocation which returns FWP_ACTION_PERMIT";

         filter.flags |=  FWPM_FILTER_FLAG_CLEAR_ACTION_RIGHT;
      }
      else
      {
         HLPR_NEW(pProviderContext,
                  FWPM_PROVIDER_CONTEXT);
         HLPR_BAIL_ON_ALLOC_FAILURE(pProviderContext,
                                    status);

         HLPR_NEW(pByteBlob,
                  FWP_BYTE_BLOB);
         HLPR_BAIL_ON_ALLOC_FAILURE(pProviderContext,
                                    status);

         status = HlprGUIDPopulate(&(pProviderContext->providerContextKey));
         HLPR_BAIL_ON_FAILURE(status);

         pProviderContext->displayData.name        = L"WFPSampler's Basic Random Action Provider Context";
         pProviderContext->displayData.description = L"Testing Purposes Only!!!";
         pProviderContext->providerKey             = (GUID*)&WFPSAMPLER_PROVIDER;
         pProviderContext->type                    = FWPM_GENERAL_CONTEXT;
         pProviderContext->dataBuffer              = pByteBlob;
         pProviderContext->dataBuffer->size        = sizeof(PC_BASIC_ACTION_DATA);
         pProviderContext->dataBuffer->data        = (BYTE*)pPCBasicActionData;

         pCallout->calloutKey              = WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM;
         pCallout->displayData.name        = L"WFPSampler's Basic Random Action Callout";
         pCallout->displayData.description = L"Testing Purposes Only!!! Causes callout invocation which randomly chooses an action to return.";
         pCallout->flags                  |= FWPM_CALLOUT_FLAG_USES_PROVIDER_CONTEXT;

         filter.flags             |= FWPM_FILTER_FLAG_HAS_PROVIDER_CONTEXT;
         filter.providerContextKey = pProviderContext->providerContextKey;

         newActionType = FWP_ACTION_CALLOUT_UNKNOWN;
      }

      pCallout->calloutKey.Data4[7] = HlprFwpmLayerGetIDByKey(&(filter.layerKey));               /// Uniquely identifies the callout used
      pCallout->providerKey         = (GUID*)&WFPSAMPLER_PROVIDER;
      pCallout->applicableLayer     = filter.layerKey;

      if(filter.flags & FWPM_FILTER_FLAG_BOOTTIME ||
         filter.flags & FWPM_FILTER_FLAG_PERSISTENT)
      {
         if(pProviderContext)
            pProviderContext->flags = FWPM_PROVIDER_CONTEXT_FLAG_PERSISTENT;

         pCallout->flags = FWPM_CALLOUT_FLAG_PERSISTENT;
      }

      filter.action.type       = newActionType;
      filter.action.calloutKey = pCallout->calloutKey;
   }
   else
      filter.action.type = actionType;

   status = HlprFwpmEngineOpen(&engineHandle);
   HLPR_BAIL_ON_FAILURE(status);

   status = HlprFwpmTransactionBegin(engineHandle);
   HLPR_BAIL_ON_FAILURE(status);

   if(pCallout)
   {
      status = HlprFwpmCalloutAdd(engineHandle,
                                  pCallout);
      HLPR_BAIL_ON_FAILURE(status);
   }

   if(pProviderContext)
   {
      status = HlprFwpmProviderContextAdd(engineHandle,
                                          pProviderContext);
      HLPR_BAIL_ON_FAILURE(status);
   }

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

   HLPR_DELETE(pByteBlob);

   HLPR_DELETE(pProviderContext);

   HLPR_DELETE(pCallout);

   return status;

}

/**
 @scenario_function="ScenarioBasicActionBlockRemove"
 
   Purpose:  Function that removes corresponding objects for a previously added 
             SCENARIO_BASIC_ACTION_BLOCK.                                                       <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 ScenarioBasicActionBlockRemove(_In_ const FWPM_FILTER* pFilter)
{
   ASSERT(pFilter);

   return PrvScenarioBasicActionDeleteFwpmObjects(pFilter,
                                                  FWP_ACTION_BLOCK);
}

/**
 @scenario_function="ScenarioBasicActionBlockAdd"
 
   Purpose:  Scenario which will return FWP_ACTION_BLOCK.                                       <br>
                                                                                                <br>
   Notes:    Adds a filter. By default this will be a static terminating filter. If specified, 
             this filter can be made to reference one of the 
             WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK callouts.                                    <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 ScenarioBasicActionBlockAdd(_In_ const FWPM_FILTER* pFilter)
{
   ASSERT(pFilter);

   return PrvScenarioBasicActionAddFwpmObjects(pFilter,
                                               FWP_ACTION_BLOCK,
                                               0);
}

/**
 @scenario_function="ScenarioBasicActionContinueRemove"
 
   Purpose:  Function that removes corresponding objects for a previously added 
             SCENARIO_BASIC_ACTION_CONTINUE.                                                    <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 ScenarioBasicActionContinueRemove(_In_ const FWPM_FILTER* pFilter)
{
   ASSERT(pFilter);

   return PrvScenarioBasicActionDeleteFwpmObjects(pFilter,
                                                  FWP_ACTION_CONTINUE);
}

/**
 @scenario_function="ScenarioBasicActionContinueAdd"
 
   Purpose:  Scenario which will return FWP_ACTION_CONTINUE.                                    <br>
                                                                                                <br>
   Notes:    Adds a filter. By default this will be a static terminating filter. If specified, 
             this filter can be made to reference one of the 
             WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE callouts.                                 <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 ScenarioBasicActionContinueAdd(_In_ const FWPM_FILTER* pFilter)
{
   ASSERT(pFilter);

   return PrvScenarioBasicActionAddFwpmObjects(pFilter,
                                               FWP_ACTION_CONTINUE,
                                               0);
}

/**
 @scenario_function="ScenarioBasicActionPermitRemove"
 
   Purpose:  Function that removes corresponding objects for a previously added 
             SCENARIO_BASIC_ACTION_PERMIT.                                                      <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 ScenarioBasicActionPermitRemove(_In_ const FWPM_FILTER* pFilter)
{
   ASSERT(pFilter);
   
   return PrvScenarioBasicActionDeleteFwpmObjects(pFilter,
                                                  FWP_ACTION_PERMIT);
}

/**
 @scenario_function="ScenarioBasicActionPermitAdd"
 
   Purpose:  Scenario which will return FWP_ACTION_PERMIT.                                      <br>
                                                                                                <br>
   Notes:    Adds a filter. By default this will be a static terminating filter. If specified, 
             this filter can be made to reference one of the 
             WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT callouts.                                   <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 ScenarioBasicActionPermitAdd(_In_ const FWPM_FILTER* pFilter)
{
   ASSERT(pFilter);

   return PrvScenarioBasicActionAddFwpmObjects(pFilter,
                                               FWP_ACTION_PERMIT,
                                               0);
}

/**
 @scenario_function="ScenarioBasicActionRandomRemove"
 
   Purpose:  Function that removes corresponding objects for a previously added 
             SCENARIO_BASIC_ACTION_RANDOM.                                                      <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 ScenarioBasicActionRandomRemove(_In_ const FWPM_FILTER* pFilter)
{
   ASSERT(pFilter);
   
   return PrvScenarioBasicActionDeleteFwpmObjects(pFilter,
                                                  FWP_ACTION_CALLOUT_UNKNOWN);
}

/**
 @scenario_function="ScenarioBasicActionRandomAdd"
 
   Purpose:  Scenario which will randomly return FWP_ACTION_BLOCK, FWP_ACTION_CONTINUE, or 
             FWP_ACTION_PERMIT.                                                                 <br>
                                                                                                <br>
   Notes:    Adds a filter which references one of the WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM 
             callouts for the provided layer.                                                   <br>
                                                                                                <br>
             The randomness of the action is configurable via the percentage values for this 
             scenario.                                                                          <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 ScenarioBasicActionRandomAdd(_In_ const FWPM_FILTER* pFilter,
                                    _In_ const PC_BASIC_ACTION_DATA* pPCBasicActionData)
{
   ASSERT(pFilter);
   ASSERT(pPCBasicActionData);

   return PrvScenarioBasicActionAddFwpmObjects(pFilter,
                                               0,
                                               pPCBasicActionData);
}

/**
 @rpc_function="RPCInvokeScenarioBasicAction"
 
   Purpose:  RPC exposed function used to dipatch the scenario routines for 
             SCENARIO_BASIC_ACTION_*.                                                           <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
/* [fault_status][comm_status] */
error_status_t RPCInvokeScenarioBasicAction(/* [in] */ handle_t rpcBindingHandle,
                                            /* [in] */ WFPSAMPLER_SCENARIO scenario,
                                            /* [in] */ FWPM_CHANGE_TYPE changeType,
                                            /* [ref][in] */ __RPC__in const FWPM_FILTER0* pFilter,
                                            /* [unique][in] */ __RPC__in_opt const PC_BASIC_ACTION_DATA* pPCBasicActionData)
{
   UINT32 status = NO_ERROR;

   UNREFERENCED_PARAMETER(rpcBindingHandle);

   ASSERT(scenario == SCENARIO_BASIC_ACTION_BLOCK ||
          scenario == SCENARIO_BASIC_ACTION_CONTINUE ||
          scenario == SCENARIO_BASIC_ACTION_PERMIT ||
          scenario == SCENARIO_BASIC_ACTION_RANDOM);
   ASSERT(changeType < FWPM_CHANGE_TYPE_MAX);
   ASSERT(pFilter);

   switch(scenario)
   {
      case SCENARIO_BASIC_ACTION_BLOCK:
      {
         if(changeType == FWPM_CHANGE_ADD)
            status = ScenarioBasicActionBlockAdd(pFilter);
         else
            status = ScenarioBasicActionBlockRemove(pFilter);

         break;
      }
      case SCENARIO_BASIC_ACTION_CONTINUE:
      {
         if(changeType == FWPM_CHANGE_ADD)
            status = ScenarioBasicActionContinueAdd(pFilter);
         else
            status = ScenarioBasicActionContinueRemove(pFilter);

         break;
      }
      case SCENARIO_BASIC_ACTION_PERMIT:
      {
         if(changeType == FWPM_CHANGE_ADD)
            status = ScenarioBasicActionPermitAdd(pFilter);
         else
            status = ScenarioBasicActionPermitRemove(pFilter);

         break;
      }
      case SCENARIO_BASIC_ACTION_RANDOM:
      {
         if(changeType == FWPM_CHANGE_ADD)
         {
            if(pPCBasicActionData)
               status = ScenarioBasicActionRandomAdd(pFilter,
                                                     pPCBasicActionData);
            else
            {
               status = ERROR_INVALID_PARAMETER;

               HlprLogError(L"RpcInvokeScenarioBasicAction() [status: %#x][pPCBasicActionData: %#p]",
                            status,
                            pPCBasicActionData);
            }
         }
         else
            status = ScenarioBasicActionRandomRemove(pFilter);
         
         break;
      }
   }

   return status;
}
