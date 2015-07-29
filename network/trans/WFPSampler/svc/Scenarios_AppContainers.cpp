////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      Scenarios_AppContainers.cpp
//
//   Abstract:
//      This module contains functions which implements the APPLICATION_CONTAINER scenarios.
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
//                                      - Function is likely visible to other modules
//            Prv                       - Function is private to this module.
//          }
//       <Object>
//          {
//            ScenarioAppContainer      - Function pertains to all of the Application Container 
//                                           Scenarios
//            RPC                       - Function is and RPC entry point.
//          }
//       <Action>
//          {
//            Add                       - Function adds objects
//            Remove                    - Function removes objects
//            Invoke                    - Function implements the scenario based on parameters 
//                                           passed from the commandline interface (WFPSampler.exe).
//          }
//       <Modifier>
//          {
//            FwpmObjects               - Function acts on WFP objects.
//            ScenarioAppContainer      - Function pertains to all of the Application Container 
//                                           Scenarios
//          }
//
//   Public Functions:
//      RPCInvokeScenarioAppContainer().
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Enhance function declaration for IntelliSense
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Framework_WFPSamplerService.h" /// .

#if(NTDDI_VERSION >= NTDDI_WIN8)

#include <netfw.h>

static BOOLEAN scenarioConfigured = FALSE;
static HANDLE  registrationHandle = 0;

/**
 @private_function="PrvScenarioAppContainerDeleteFwpmObjects"
 
   Purpose:                                                                                     <br>
                                                                                                <br>
   Notes:    Function is overloaded.                                                            <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 PrvScenarioAppContainerDeleteFwpmObjects(_In_ const SID* pPackageID,
                                                _In_ const SID* pUserID)
{
   UNREFERENCED_PARAMETER(pUserID);

   ASSERT(pPackageID);
   ASSERT(pUserID);

   UINT32                status                            = NO_ERROR;
///   UINT32                sidSize                           = 0;
   HANDLE                engineHandle                      = 0;
   const GUID            pLayerKeys[]                      = {FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4,
                                                              FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6,
                                                              FWPM_LAYER_ALE_AUTH_CONNECT_V4,
                                                              FWPM_LAYER_ALE_AUTH_CONNECT_V6};
   const UINT8           NUM_CONDITIONS                    = 1; /// 2
   const UINT8           NUM_OBJECTS                       = RTL_NUMBER_OF(pLayerKeys);
   FWPM_FILTER_CONDITION pFilterConditions[NUM_CONDITIONS] = {0};

   pFilterConditions[0].fieldKey            = FWPM_CONDITION_ALE_PACKAGE_ID;
   pFilterConditions[0].matchType           = FWP_MATCH_EQUAL;
   pFilterConditions[0].conditionValue.type = FWP_SID;
   pFilterConditions[0].conditionValue.sid  = (SID*)pPackageID;

///   pFilterConditions[1].fieldKey            = FWPM_CONDITION_ALE_USER_ID;
///   pFilterConditions[1].matchType           = FWP_MATCH_EQUAL;
///   pFilterConditions[1].conditionValue.type = FWP_SECURITY_DESCRIPTOR_TYPE;
///   pFilterConditions[1].conditionValue.sd   = ;

   status = HlprFwpmEngineOpen(&engineHandle);
   HLPR_BAIL_ON_FAILURE(status);

   for(UINT32 objectIndex = 0;
       objectIndex < NUM_OBJECTS;
       objectIndex++)
   {
      UINT32                    entryCount         = 0;
      FWPM_FILTER**             ppFilters          = 0;
      HANDLE                    enumHandle         = 0;
      FWPM_FILTER_ENUM_TEMPLATE filterEnumTemplate = {0};

      filterEnumTemplate.providerKey         = (GUID*)&WFPSAMPLER_PROVIDER;
      filterEnumTemplate.layerKey            = pLayerKeys[objectIndex];
      filterEnumTemplate.enumType            = FWP_FILTER_ENUM_FULLY_CONTAINED;
      filterEnumTemplate.flags               = FWP_FILTER_ENUM_FLAG_INCLUDE_BOOTTIME |
                                               FWP_FILTER_ENUM_FLAG_INCLUDE_DISABLED;
      filterEnumTemplate.numFilterConditions = NUM_CONDITIONS;
      filterEnumTemplate.filterCondition     = pFilterConditions;
      filterEnumTemplate.actionMask          = 0xFFFFFFFF;

      status = HlprFwpmFilterCreateEnumHandle(engineHandle,
                                              &filterEnumTemplate,
                                              &enumHandle);
      HLPR_BAIL_ON_FAILURE_WITH_LABEL(status,
                                      HLPR_BAIL_LABEL_2);

      status = HlprFwpmFilterEnum(engineHandle,
                                  enumHandle,
                                  0xFFFFFFFF,
                                  &ppFilters,
                                  &entryCount);
      HLPR_BAIL_ON_FAILURE_WITH_LABEL(status,
                                      HLPR_BAIL_LABEL_2);

      if(ppFilters)
      {
         for(UINT32 filterIndex = 0;
             filterIndex < entryCount;
             filterIndex++)
         {
            HlprFwpmFilterDeleteByKey(engineHandle,
                                      &(ppFilters[filterIndex]->filterKey));
         }

         FwpmFreeMemory((VOID**)&ppFilters);
      }

      HLPR_BAIL_LABEL_2:

      if(enumHandle)
         HlprFwpmFilterDestroyEnumHandle(engineHandle,
                                         &enumHandle);
   }

   HLPR_BAIL_LABEL:

   HlprFwpmEngineClose(&engineHandle);

   return status;
}

/**
 @private_function="PrvScenarioAppContainerDeleteFwpmObjects"
 
   Purpose:                                                                                     <br>
                                                                                                <br>
   Notes:    Function is overloaded.                                                            <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 PrvScenarioAppContainerDeleteFwpmObjects()
{
   UINT32                status          = NO_ERROR;
   UINT32                sidSize         = 0;
   HANDLE                engineHandle    = 0;
   const GUID            pLayerKeys[]    = {FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4,
                                            FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6,
                                            FWPM_LAYER_ALE_AUTH_CONNECT_V4,
                                            FWPM_LAYER_ALE_AUTH_CONNECT_V6};
   const UINT32          NUM_OBJECTS     = RTL_NUMBER_OF(pLayerKeys);
   FWPM_FILTER_CONDITION filterCondition = {0};

   /// Only App Containers have a valid (non-NULL) SID for the ALE_PACKAGE_ID
   filterCondition.fieldKey            = FWPM_CONDITION_ALE_PACKAGE_ID;
   filterCondition.matchType           = FWP_MATCH_NOT_EQUAL;
   filterCondition.conditionValue.type = FWP_SID;

#pragma warning(push)
#pragma warning(disable: 6388) /// filterCondition.conditionValue.sid guaranteed to be 0 due to ZeroMemory call

   status = HlprSIDGetWellKnown(WinNullSid,
                                &(filterCondition.conditionValue.sid),
                                &sidSize);
   HLPR_BAIL_ON_FAILURE(status);

#pragma warning(pop)

   status = HlprFwpmEngineOpen(&engineHandle);
   HLPR_BAIL_ON_FAILURE(status);

   for(UINT32 objectIndex = 0;
       objectIndex < NUM_OBJECTS;
       objectIndex++)
   {
      HANDLE                    enumHandle         = 0;
      UINT32                    entryCount         = 0;
      FWPM_FILTER**             ppFilters          = 0;
      FWPM_FILTER_ENUM_TEMPLATE filterEnumTemplate = {0};

      filterEnumTemplate.providerKey         = (GUID*)&WFPSAMPLER_PROVIDER;
      filterEnumTemplate.layerKey            = pLayerKeys[objectIndex];
      filterEnumTemplate.enumType            = FWP_FILTER_ENUM_FULLY_CONTAINED;
      filterEnumTemplate.flags               = FWP_FILTER_ENUM_FLAG_INCLUDE_BOOTTIME |
                                               FWP_FILTER_ENUM_FLAG_INCLUDE_DISABLED;
      filterEnumTemplate.numFilterConditions = 1;
      filterEnumTemplate.filterCondition     = &filterCondition;
      filterEnumTemplate.actionMask          = 0xFFFFFFFF;

      status = HlprFwpmFilterCreateEnumHandle(engineHandle,
                                              &filterEnumTemplate,
                                              &enumHandle);
      HLPR_BAIL_ON_FAILURE_WITH_LABEL(status,
                                      HLPR_BAIL_LABEL_2);

      status = HlprFwpmFilterEnum(engineHandle,
                                  enumHandle,
                                  0xFFFFFFFF,
                                  &ppFilters,
                                  &entryCount);
      HLPR_BAIL_ON_FAILURE_WITH_LABEL(status,
                                      HLPR_BAIL_LABEL_2);

      if(ppFilters)
      {
         for(UINT32 filterIndex = 0;
             filterIndex < entryCount;
             filterIndex++)
         {
            HlprFwpmFilterDeleteByKey(engineHandle,
                                      &(ppFilters[filterIndex]->filterKey));
         }

         FwpmFreeMemory((VOID**)&ppFilters);
      }

      HLPR_BAIL_LABEL_2:

      if(enumHandle)
         HlprFwpmFilterDestroyEnumHandle(engineHandle,
                                         &enumHandle);
   }

   HLPR_BAIL_LABEL:

   HlprSIDDestroy(&(filterCondition.conditionValue.sid));

   HlprFwpmEngineClose(&engineHandle);

   return status;
}

/**
 @private_function="PrvScenarioAppContainerAddFwpmObjects"
 
   Purpose:  Add filters for specific containers.                                               <br>
                                                                                                <br>
   Notes:    Function is overloaded.                                                            <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 PrvScenarioAppContainerAddFwpmObjects(_In_ const SID* pPackageID,
                                             _In_ const SID* pUserID,
                                             _In_opt_ PCWSTR pDisplayName,
                                             _In_ BOOLEAN persistent = TRUE,
                                             _In_ BOOLEAN bootTime = FALSE)
{
   UNREFERENCED_PARAMETER(pUserID);

   ASSERT(pPackageID);
   ASSERT(pUserID);

   UINT32                status                            = NO_ERROR;
///   UINT32                sidSize                           = 0;
   HANDLE                engineHandle                      = 0;
   const GUID            pLayerKeys[]                      = {FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4,
                                                              FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6,
                                                              FWPM_LAYER_ALE_AUTH_CONNECT_V4,
                                                              FWPM_LAYER_ALE_AUTH_CONNECT_V6};
   const UINT8           NUM_CONDITIONS                    = 1; /// 2
   const UINT8           NUM_OBJECTS                       = RTL_NUMBER_OF(pLayerKeys);
   FWPM_FILTER_CONDITION pFilterConditions[NUM_CONDITIONS] = {0};
   FWPM_FILTER           pFilters[NUM_OBJECTS]             = {0};

///   status = HlprSecurityDescriptorGetSelfRelativeForUser();

   pFilterConditions[0].fieldKey            = FWPM_CONDITION_ALE_PACKAGE_ID;
   pFilterConditions[0].matchType           = FWP_MATCH_EQUAL;
   pFilterConditions[0].conditionValue.type = FWP_SID;
   pFilterConditions[0].conditionValue.sid  = (SID*)pPackageID;

///   pFilterConditions[1].fieldKey            = FWPM_CONDITION_ALE_USER_ID;
///   pFilterConditions[1].matchType           = FWP_MATCH_EQUAL;
///   pFilterConditions[1].conditionValue.type = FWP_SECURITY_DESCRIPTOR_TYPE;
///   pFilterConditions[1].conditionValue.sd   = ;

   for(UINT32 objectIndex = 0;
       objectIndex < NUM_OBJECTS;
       objectIndex++)
   {
      status = HlprGUIDPopulate(&(pFilters[objectIndex].filterKey));
      HLPR_BAIL_ON_FAILURE(status);

      pFilters[objectIndex].displayData.name         = L"WFPSampler's AppContainer Scenario Filter";
      pFilters[objectIndex].displayData.description  = (PWSTR)pDisplayName;
      pFilters[objectIndex].flags                   |= FWPM_FILTER_FLAG_PERSISTENT;
      pFilters[objectIndex].providerKey              = (GUID*)&WFPSAMPLER_PROVIDER;
      pFilters[objectIndex].layerKey                 = pLayerKeys[objectIndex];
      pFilters[objectIndex].subLayerKey              = WFPSAMPLER_SUBLAYER;
      pFilters[objectIndex].weight.type              = FWP_UINT8;
      pFilters[objectIndex].weight.uint8             = 0xF;
      pFilters[objectIndex].numFilterConditions      = NUM_CONDITIONS;
      pFilters[objectIndex].filterCondition          = pFilterConditions;
      pFilters[objectIndex].action.type              = FWP_ACTION_PERMIT;

      if(!persistent)
         pFilters[objectIndex].flags ^= FWPM_FILTER_FLAG_PERSISTENT;

      if(bootTime)
      {
         if(pFilters[objectIndex].flags & FWPM_FILTER_FLAG_PERSISTENT)
            pFilters[objectIndex].flags ^= FWPM_FILTER_FLAG_PERSISTENT;

         pFilters[objectIndex].flags |= FWPM_FILTER_FLAG_BOOTTIME;
      }
   }

   status = HlprFwpmEngineOpen(&engineHandle);
   HLPR_BAIL_ON_FAILURE(status);

   status = HlprFwpmTransactionBegin(engineHandle);
   HLPR_BAIL_ON_FAILURE(status);

   for(UINT32 objectIndex = 0;
       objectIndex < NUM_OBJECTS;
       objectIndex++)
   {
      status = HlprFwpmFilterAdd(engineHandle,
                                 &(pFilters[objectIndex]));
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

   return status;
}

/**
 @private_function="PrvScenarioAppContainerAddFwpmObjects"
 
   Purpose:  Function that enables the SCENARIO_APP_CONTAINER scenarios.                        <br>
                                                                                                <br>
   Notes:    Function is overloaded.                                                            <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 PrvScenarioAppContainerAddFwpmObjects(_In_ BOOLEAN persistent = TRUE,
                                             _In_ BOOLEAN bootTime = FALSE)
{
   UINT32                status                = NO_ERROR;
   UINT32                sidSize               = 0;
   HANDLE                engineHandle          = 0;
   const GUID            pLayerKeys[]          = {FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4,
                                                  FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6,
                                                  FWPM_LAYER_ALE_AUTH_CONNECT_V4,
                                                  FWPM_LAYER_ALE_AUTH_CONNECT_V6};
   const UINT32          NUM_OBJECTS           = RTL_NUMBER_OF(pLayerKeys);
   FWPM_FILTER_CONDITION filterCondition       = {0};
   FWPM_FILTER           pFilters[NUM_OBJECTS] = {0};

   /// Only App Containers have a valid (non-NULL) SID for the ALE_PACKAGE_ID
   filterCondition.fieldKey            = FWPM_CONDITION_ALE_PACKAGE_ID;
   filterCondition.matchType           = FWP_MATCH_NOT_EQUAL;
   filterCondition.conditionValue.type = FWP_SID;

#pragma warning(push)
#pragma warning(disable: 6388) /// filterCondition.conditionValue.sid guaranteed to be 0 due to ZeroMemory call

   status = HlprSIDGetWellKnown(WinNullSid,
                                &(filterCondition.conditionValue.sid),
                                &sidSize);
   HLPR_BAIL_ON_FAILURE(status);

#pragma warning(pop)

   for(UINT32 objectIndex = 0;
       objectIndex < NUM_OBJECTS;
       objectIndex++)
   {
      status = HlprGUIDPopulate(&(pFilters[objectIndex].filterKey));
      HLPR_BAIL_ON_FAILURE(status);

      pFilters[objectIndex].displayData.name         = L"WFPSampler's AppContainer Scenario Filter";
      pFilters[objectIndex].displayData.description  = L"Trust Windows Service Hardening to handle all App Containers";
      pFilters[objectIndex].flags                   |= FWPM_FILTER_FLAG_PERSISTENT;
      pFilters[objectIndex].providerKey              = (GUID*)&WFPSAMPLER_PROVIDER;
      pFilters[objectIndex].layerKey                 = pLayerKeys[objectIndex];
      pFilters[objectIndex].subLayerKey              = WFPSAMPLER_SUBLAYER;
      pFilters[objectIndex].weight.type              = FWP_UINT8;
      pFilters[objectIndex].weight.uint8             = 0xF;
      pFilters[objectIndex].numFilterConditions      = 1;
      pFilters[objectIndex].filterCondition          = &filterCondition;
      pFilters[objectIndex].action.type              = FWP_ACTION_PERMIT;

      if(!persistent)
         pFilters[objectIndex].flags ^= FWPM_FILTER_FLAG_PERSISTENT;

      if(bootTime)
      {
         if(pFilters[objectIndex].flags & FWPM_FILTER_FLAG_PERSISTENT)
            pFilters[objectIndex].flags ^= FWPM_FILTER_FLAG_PERSISTENT;

         pFilters[objectIndex].flags |= FWPM_FILTER_FLAG_BOOTTIME;
      }
   }

   status = HlprFwpmEngineOpen(&engineHandle);
   HLPR_BAIL_ON_FAILURE(status);

   status = HlprFwpmTransactionBegin(engineHandle);
   HLPR_BAIL_ON_FAILURE(status);

   for(UINT32 objectIndex = 0;
       objectIndex < NUM_OBJECTS;
       objectIndex++)
   {
      status = HlprFwpmFilterAdd(engineHandle,
                                 &(pFilters[objectIndex]));
      HLPR_BAIL_ON_FAILURE(status);
   }

   status = HlprFwpmTransactionCommit(engineHandle);
   HLPR_BAIL_ON_FAILURE(status);

   HLPR_BAIL_LABEL:

   HlprSIDDestroy(&(filterCondition.conditionValue.sid));

   if(engineHandle)
   {
      if(status != NO_ERROR)
         HlprFwpmTransactionAbort(engineHandle);

      HlprFwpmEngineClose(&engineHandle);
   }

   return status;
}

/**
 @callback_function="PrvScenarioAppContainerActOnChange"
 
   Purpose:  Callback function which is invoked when an application adds or removes it's 
             appContainer capabilities.                                                         <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
VOID CALLBACK PrvScenarioAppContainerActOnChange(_In_opt_ VOID* pContext,
                                                 _In_ const INET_FIREWALL_AC_CHANGE* pChange)
{
   UNREFERENCED_PARAMETER(pContext);

   ASSERT(pChange);

   if(pChange->changeType == INET_FIREWALL_AC_CHANGE_CREATE)
   {
      PrvScenarioAppContainerAddFwpmObjects(pChange->appContainerSid,
                                            pChange->userSid,
                                            pChange->displayName);
   }
   else if(pChange->changeType == INET_FIREWALL_AC_CHANGE_DELETE)
   {
      PrvScenarioAppContainerDeleteFwpmObjects(pChange->appContainerSid,
                                               pChange->userSid);
   }

   return;
}

/**
 @private_function="PrvScenarioAppContainerEnumExisting"
 
   Purpose:                                                                                     <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/HH447479.aspx              <br>
*/
_Success_(return == NO_ERROR)
UINT32 PrvScenarioAppContainerEnumExisting()
{
   UINT32                       status           = NO_ERROR;
   UINT32                       numAppContainers = 0;
   INET_FIREWALL_APP_CONTAINER* pAppContainers   = 0;

   status = NetworkIsolationEnumAppContainers(NETISO_FLAG_FORCE_COMPUTE_BINARIES,
                                              (DWORD*)&numAppContainers,
                                              &pAppContainers);

   HLPR_BAIL_ON_FAILURE(status);

   for(UINT32 containerIndex = 0;
       containerIndex < numAppContainers;
       containerIndex++)
   {
      status = PrvScenarioAppContainerAddFwpmObjects(pAppContainers[containerIndex].appContainerSid,
                                                     pAppContainers[containerIndex].userSid,
                                                     pAppContainers[containerIndex].displayName);
      HLPR_BAIL_ON_FAILURE(status);
   }

   HLPR_BAIL_LABEL:

   return status;
}

/**
 @private_function="PrvScenarioAppContainerUnregister"
 
   Purpose:                                                                                     <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/HH447485.aspx              <br>
*/
_Success_(return == NO_ERROR)
UINT32 PrvScenarioAppContainerUnregister()
{
   UINT32 status = NO_ERROR;

   if(registrationHandle)
   {
      status = NetworkIsolationUnregisterForAppContainerChanges(registrationHandle);
      if(status != NO_ERROR)
         HlprLogError(L"PrvScenarioAppContainerUnregister : NetworkIsolationUnregisterForAppContainerChanges() [status: %#x]",
                      status);
      else
         registrationHandle = 0;
   }

   return status;
}

/**
 @private_function="PrvScenarioAppContainerRegister"
 
   Purpose:                                                                                     <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/HH447482.aspx              <br>
*/
_Success_(return == NO_ERROR)
UINT32 PrvScenarioAppContainerRegister()
{
   UINT32 status = NO_ERROR;

   status = NetworkIsolationRegisterForAppContainerChanges(0,
                                                           PrvScenarioAppContainerActOnChange,
                                                           0,
                                                           &registrationHandle);
   if(status != NO_ERROR)
      HlprLogError(L"ScenarioAppContainerRegister : NetworkIsolationRegisterForAppContainerChanges() [status: %#x]",
                   status);

   return status;
}

/**
 @scenario_function="ScenarioAppContainerRemove"
 
   Purpose:  Function that removes corresponding objects for a previously added 
             SCENARIO_APP_CONTAINER.                                                            <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 ScenarioAppContainerRemove(_In_ BOOLEAN trustWSH)
{
   UINT32 status = NO_ERROR;

   if(trustWSH)
      status = PrvScenarioAppContainerDeleteFwpmObjects();
   else
   {
   }

   return status;
}

/**
 @scenario_function="ScenarioAppContainerAdd"
 
   Purpose:                                                                                     <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 ScenarioAppContainerAdd(_In_ BOOLEAN trustWSH,
                               _In_ BOOLEAN persistent,
                               _In_ BOOLEAN bootTime)
{
   UINT32 status = NO_ERROR;

   if(trustWSH)
      status = PrvScenarioAppContainerAddFwpmObjects(persistent,
                                                     bootTime);
   else
   {
      status = PrvScenarioAppContainerRegister();
      HLPR_BAIL_ON_FAILURE(status);

      status = PrvScenarioAppContainerEnumExisting();
      HLPR_BAIL_ON_FAILURE(status);
   }

   HLPR_BAIL_LABEL:

   if(status == NO_ERROR)
      scenarioConfigured = TRUE;

   return status;
}

/**
 @rpc_function="RPCInvokeScenarioBasicAction"
 
   Purpose:  RPC exposed function used to dipatch the scenario routines for 
             SCENARIO_APP_CONTAINER.                                                            <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
/* [fault_status][comm_status] */
error_status_t RPCInvokeScenarioAppContainer(/* [in] */ handle_t rpcBindingHandle,
                                             /* [in] */ WFPSAMPLER_SCENARIO scenario,
                                             /* [in] */ FWPM_CHANGE_TYPE changeType,
                                             /* [in] */ BOOLEAN trustWSH,
                                             /* [in] */ BOOLEAN persistent,
                                             /* [in] */ BOOLEAN bootTime)
{
   UINT32 status = NO_ERROR;

   UNREFERENCED_PARAMETER(rpcBindingHandle);
   UNREFERENCED_PARAMETER(scenario);

   if(scenario < SCENARIO_MAX &&
      changeType < FWPM_CHANGE_TYPE_MAX)
   {
      switch(scenario)
      {
         case SCENARIO_APP_CONTAINER:
         {
            if(changeType == FWPM_CHANGE_ADD)
            {
               if(!scenarioConfigured)
                  status = ScenarioAppContainerAdd(trustWSH,
                                                   persistent,
                                                   bootTime);
               else
               {
                  status = ERROR_ALREADY_EXISTS;

                  HlprLogError(L"RPCInvokeScenarioAppContainer() [status: %#x]",
                               status);
               }
            }
            else
               status = ScenarioAppContainerRemove(trustWSH);

            break;
         }
         default:
         {
            status = ERROR_INVALID_PARAMETER;

            HlprLogError(L"RPCInvokeScenarioAppContainer() [status: %#x][scenario: %d]",
                         status,
                         scenario);

            break;
         }
      }
   }
   else
   {
      status = ERROR_INVALID_PARAMETER;

      HlprLogError(L"RPCInvokeScenarioAppContainer() [status: %#x][scenario: %#x][changeType: %#x]",
                   status,
                   scenario,
                   changeType);
   }

   return status;
}

#else

/**
 @rpc_function="RPCInvokeScenarioBasicAction"
 
   Purpose:  RPC exposed function used to dipatch the scenario routines for 
             SCENARIO_APP_CONTAINER.                                                            <br>
                                                                                                <br>
   Notes:    This particular function is only a stub for RPC on downlevel SKUs (Windows 7 and 
             below).                                                                            <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
   /* [fault_status][comm_status] */
   error_status_t RPCInvokeScenarioAppContainer(/* [in] */ handle_t rpcBindingHandle,
                                                /* [in] */ WFPSAMPLER_SCENARIO scenario,
                                                /* [in] */ FWPM_CHANGE_TYPE changeType,
                                                /* [in] */ BOOLEAN trustWSH,
                                                /* [in] */ BOOLEAN persistent,
                                                /* [in] */ BOOLEAN bootTime)
{
   UNREFERENCED_PARAMETER(rpcBindingHandle);
   UNREFERENCED_PARAMETER(scenario);
   UNREFERENCED_PARAMETER(changeType);
   UNREFERENCED_PARAMETER(trustWSH);
   UNREFERENCED_PARAMETER(persistent);
   UNREFERENCED_PARAMETER(bootTime);

   return ERROR_NOT_SUPPORTED;
}

#endif /// (NTDDI_VERSION >= NTDDI_WIN8)
