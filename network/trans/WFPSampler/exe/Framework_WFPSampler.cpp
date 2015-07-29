////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      Framework_WFPSampler.cpp
//
//   Abstract:
//      This module contains functions which form the entry point to our client program 
//         WFPSampler.Exe.
//
//   Naming Convention:
//
//      <Scope><Object><Action><Modifier>
//  
//      i.e.
//
//       <Scope>
//          {
//                        - Function is likely visible to other modules.
//            Prv         - Function is private to this module.
//          }
//       <Object>
//          {
//            FlowControl - Function pertains to how the program execution should behave.
//            Scenario    - Function pertains to scenarios.
//       <Action>
//          {
//            Dispatch    - Function selects and invokes necessary code paths for given data.
//            Get         - Function parses strings for requested value.
//            Log         - Function writes to the console.
//          }
//       <Modifier>
//          {
//            Usage       - Function provides generic application usage information.
//          }
//
//   Private Functions:
//      PrvFlowControlGet(),
//      PrvLogUsage(),
//      PrvScenarioDispatch(),
//      PrvScenarioGet(),
//      wmain(),

//   Public Functions:
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Add ADVANCED_PACKET_INJECTION, FLOW_ASSOCIATION, and
//                                              PEND_ENDPOINT_CLOSURE scenarios
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Framework_WFPSampler.h"         /// .
#include "Framework_RPCClientInterface.h" /// .

/// Module's Local Enumerations

typedef enum WFPSAMPLER_FLOW_CONTROL_
{
   FLOW_CONTROL_NORMAL = 0,
   FLOW_CONTROL_HELP   = 1,
   FLOW_CONTROL_CLEAN  = 2,
}WFPSAMPLER_FLOW_CONTROL;

///

/**
 @private_function="PrvFlowControlGet"
 
   Purpose:  Parse the command line parameters for any flow control commands such as:           <br>
                help (-?) (?) (-help)                                                           <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
WFPSAMPLER_FLOW_CONTROL PrvFlowControlGet(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                          _In_ UINT32 stringCount)
{
   ASSERT(ppCLPStrings);
   ASSERT(stringCount);

   WFPSAMPLER_FLOW_CONTROL flowControl = FLOW_CONTROL_NORMAL;

   for(UINT32 stringIndex = 0;
       stringIndex < stringCount;
       stringIndex++)
   {
      if(HlprStringsAreEqual(ppCLPStrings[stringIndex],
                             L"-?") ||
         HlprStringsAreEqual(ppCLPStrings[stringIndex],
                             L"/?") ||
         HlprStringsAreEqual(ppCLPStrings[stringIndex],
                             L"?") ||
         HlprStringsAreEqual(ppCLPStrings[stringIndex],
                             L"-help") ||
         HlprStringsAreEqual(ppCLPStrings[stringIndex],
                             L"/help"))
      {
         flowControl = FLOW_CONTROL_HELP;

         break;
      }
      /// This is used for testing purposes only.  Generally we should not be touching any other 
      /// provider's WFP objects, and is grounds for failure in the Hardware Certification Kit.
      else if(HlprStringsAreEqual(ppCLPStrings[stringIndex],
                                  L"-clean") ||
              HlprStringsAreEqual(ppCLPStrings[stringIndex],
                                  L"/clean"))
      {
         flowControl = FLOW_CONTROL_CLEAN;

         break;
      }
   }

   return flowControl;
}

/**
 @private_function="PrvScenarioDispatcher"
 
   Purpose:  Route the provider scenario to the appropriate Scenario implementation.            <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
UINT32 PrvScenarioDispatcher(_In_ WFPSAMPLER_SCENARIO scenario,
                             _In_reads_(stringCount) PCWSTR* ppCLPStrings,
                             _In_ UINT32 stringCount)
{
   ASSERT(scenario < SCENARIO_MAX);
   ASSERT(ppCLPStrings);
   ASSERT(stringCount);

   UINT32 status = NO_ERROR;

   switch(scenario)
   {

      case SCENARIO_ADVANCED_PACKET_INJECTION:
      {
         status = AdvancedPacketInjectionScenarioExecute(ppCLPStrings,
                                                         stringCount);

         break;
      }

#if(NTDDI_VERSION >= NTDDI_WIN8)

      case SCENARIO_APP_CONTAINER:
      {
         status = AppContainerScenarioExecute(ppCLPStrings,
                                              stringCount);

         break;
      }

#endif /// (NTDDI_VERSION >= NTDDI_WIN8)

      case SCENARIO_BASIC_ACTION_BLOCK:
      {
         status = BasicActionBlockScenarioExecute(ppCLPStrings,
                                                  stringCount);

         break;
      }
      case SCENARIO_BASIC_ACTION_CONTINUE:
      {
         status = BasicActionContinueScenarioExecute(ppCLPStrings,
                                                     stringCount);

         break;
      }
      case SCENARIO_BASIC_ACTION_PERMIT:
      {
         status = BasicActionPermitScenarioExecute(ppCLPStrings,
                                                   stringCount);

         break;
      }
      case SCENARIO_BASIC_ACTION_RANDOM:
      {
         status = BasicActionRandomScenarioExecute(ppCLPStrings,
                                                   stringCount);

         break;
      }
      case SCENARIO_BASIC_PACKET_EXAMINATION:
      {
         status = BasicPacketExaminationScenarioExecute(ppCLPStrings,
                                                        stringCount);

         break;
      }
      case SCENARIO_BASIC_PACKET_INJECTION:
      {
         status = BasicPacketInjectionScenarioExecute(ppCLPStrings,
                                                      stringCount);

         break;
      }
      case SCENARIO_BASIC_PACKET_MODIFICATION:
      {
         status = BasicPacketModificationScenarioExecute(ppCLPStrings,
                                                         stringCount);

         break;
      }
      case SCENARIO_BASIC_STREAM_INJECTION:
      {
         status = BasicStreamInjectionScenarioExecute(ppCLPStrings,
                                                      stringCount);

         break;
      }
      case SCENARIO_FAST_PACKET_INJECTION:
      {
         status = FastPacketInjectionScenarioExecute(ppCLPStrings,
                                                     stringCount);

         break;
      }
      case SCENARIO_FAST_STREAM_INJECTION:
      {
         status = FastStreamInjectionScenarioExecute(ppCLPStrings,
                                                     stringCount);

         break;
      }
      case SCENARIO_FLOW_ASSOCIATION:
      {
         status = FlowAssociationScenarioExecute(ppCLPStrings,
                                                 stringCount);

         break;
      }
      case SCENARIO_PEND_AUTHORIZATION:
      {
         status = PendAuthorizationScenarioExecute(ppCLPStrings,
                                                   stringCount);

         break;
      }

#if(NTDDI_VERSION >= NTDDI_WIN7)

      case SCENARIO_PEND_ENDPOINT_CLOSURE:
      {
         status = PendEndpointClosureScenarioExecute(ppCLPStrings,
                                                     stringCount);

         break;
      }

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

      case SCENARIO_PROXY:
      {
         status = ProxyScenarioExecute(ppCLPStrings,
                                       stringCount);

         break;
      }
      default:
      {
         status = ERROR_NOT_SUPPORTED;

         break;
      }
   }

   return status;
}

/**
 @private_function="PrvScenarioGet"
 
   Purpose:  Parse the command line parameters for the scenario (-s ).                          <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: N/A                                                                                <br>
*/
WFPSAMPLER_SCENARIO PrvScenarioGet(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                   _In_ UINT32 stringCount,
                                   _In_ WFPSAMPLER_FLOW_CONTROL flowControl)
{
   ASSERT(ppCLPStrings);
   ASSERT(stringCount);

   WFPSAMPLER_SCENARIO scenario = SCENARIO_UNDEFINED;

   for(UINT32 stringIndex = 0;
       (stringIndex + 1) < stringCount;
       stringIndex++)
   {
      if(HlprStringsAreEqual(ppCLPStrings[stringIndex],
                             L"-s") ||
         HlprStringsAreEqual(ppCLPStrings[stringIndex],
                             L"/s"))
      {
         PCWSTR pScenarioString = ppCLPStrings[stringIndex + 1];

         if(pScenarioString)
         {
            if(HlprStringsAreEqual(pScenarioString,
                                   L"ADVANCED_PACKET_INJECTION"))
               scenario = SCENARIO_ADVANCED_PACKET_INJECTION;
            else if(HlprStringsAreEqual(pScenarioString,
                                        L"APP_CONTAINER"))
               scenario = SCENARIO_APP_CONTAINER;
            else if(HlprStringsAreEqual(pScenarioString,
                                        L"BASIC_ACTION_BLOCK"))
               scenario = SCENARIO_BASIC_ACTION_BLOCK;
            else if(HlprStringsAreEqual(pScenarioString,
                                        L"BASIC_ACTION_CONTINUE"))
               scenario = SCENARIO_BASIC_ACTION_CONTINUE;
            else if(HlprStringsAreEqual(pScenarioString,
                                        L"BASIC_ACTION_PERMIT"))
               scenario = SCENARIO_BASIC_ACTION_PERMIT;
            else if(HlprStringsAreEqual(pScenarioString,
                                        L"BASIC_ACTION_RANDOM"))
               scenario = SCENARIO_BASIC_ACTION_RANDOM;
            else if(HlprStringsAreEqual(pScenarioString,
                                        L"BASIC_PACKET_EXAMINATION"))
               scenario = SCENARIO_BASIC_PACKET_EXAMINATION;
            else if(HlprStringsAreEqual(pScenarioString,
                                        L"BASIC_PACKET_INJECTION"))
               scenario = SCENARIO_BASIC_PACKET_INJECTION;
            else if(HlprStringsAreEqual(pScenarioString,
                                        L"BASIC_PACKET_MODIFICATION"))
               scenario = SCENARIO_BASIC_PACKET_MODIFICATION;
            else if(HlprStringsAreEqual(pScenarioString,
                                        L"BASIC_STREAM_INJECTION"))
               scenario = SCENARIO_BASIC_STREAM_INJECTION;
            else if(HlprStringsAreEqual(pScenarioString,
                                        L"FAST_PACKET_INJECTION"))
               scenario = SCENARIO_FAST_PACKET_INJECTION;
            else if(HlprStringsAreEqual(pScenarioString,
                                        L"FAST_STREAM_INJECTION"))
               scenario = SCENARIO_FAST_STREAM_INJECTION;
            else if(HlprStringsAreEqual(pScenarioString,
                                        L"FLOW_ASSOCIATION"))
               scenario = SCENARIO_FLOW_ASSOCIATION;
            else if(HlprStringsAreEqual(pScenarioString,
                                        L"PEND_AUTHORIZATION"))
               scenario = SCENARIO_PEND_AUTHORIZATION;
            else if(HlprStringsAreEqual(pScenarioString,
                                        L"PEND_ENDPOINT_CLOSURE"))
               scenario = SCENARIO_PEND_ENDPOINT_CLOSURE;
            else if(HlprStringsAreEqual(pScenarioString,
                                        L"PROXY"))
               scenario = SCENARIO_PROXY;
         }

         if(scenario != SCENARIO_UNDEFINED)
            break;
      }
   }

   if(scenario == SCENARIO_UNDEFINED &&
      flowControl == FLOW_CONTROL_NORMAL)
      HlprLogError(L"Invalid Scenario defined");

   return scenario;
}

/**
 @private_function="PrvCleanPolicy"
 
   Purpose:  Remove all WFP objects based on the passed in scope.                               <br>
               default   -   cleanup all WFPSampler objects except WFPSAMPLER_SUBLAYER and 
                                WFPSAMPLER_PROVIDER.                                            <br>
               firewall  -   cleanup all WFP objects from the non-usermode layers except those 
                                that are built-in, the WFPSAMPLER_SUBLAYER, and the 
                                WFPSAMPLER_PROVIDER.                                            <br>
               all       -   cleanup all WFP objects except those that are built-in, the 
                                WFPSAMPLER_SUBLAYER, and the WFPSAMPLER_PROVIDER.               <br>
                                                                                                <br>
   Notes:    All and firewall are meant for testing purposes only.  Doing this in a commercial 
             product is grounds for failure in the Microsoft Hardware Certification Kit.        <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
UINT32 PrvCleanPolicy(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                      _In_ UINT32 stringCount)
{
   ASSERT(ppCLPStrings);
   ASSERT(stringCount);

   UINT32                               status                        = NO_ERROR;
   BOOLEAN                              cleanFirewallOnly             = FALSE;
   BOOLEAN                              cleanWFPSamplersOnly          = FALSE;
   HANDLE                               engineHandle                  = 0;
   HANDLE                               enumHandle                    = 0;
   UINT32                               numEntries                    = 0;
   FWPM_CALLOUT**                       ppCallouts                    = 0;
   FWPM_FILTER**                        ppFilters                     = 0;
   FWPM_PROVIDER**                      ppProviders                   = 0;
   FWPM_PROVIDER_CONTEXT**              ppProviderContexts            = 0;
   FWPM_SUBLAYER**                      ppSubLayers                   = 0;
   FWPM_FILTER_ENUM_TEMPLATE*           pFilterEnumTemplate           = 0;
   FWPM_CALLOUT_ENUM_TEMPLATE*          pCalloutEnumTemplate          = 0;
   FWPM_PROVIDER_CONTEXT_ENUM_TEMPLATE* pProviderContextEnumTemplate  = 0;

   for(UINT32 stringIndex = 0;
       stringIndex < stringCount;
       stringIndex++)
   {
      if(HlprStringsAreEqual(ppCLPStrings[stringIndex],
                             L"-clean") ||
         HlprStringsAreEqual(ppCLPStrings[stringIndex],
                             L"/clean"))
      {
         PCWSTR pString = 0;

         if((stringIndex + 1) < stringCount)
            pString = ppCLPStrings[++stringIndex];
         {
            if(HlprStringsAreEqual(pString,
                                   L"all"))
              break;
            else if(HlprStringsAreEqual(pString,
                                        L"firewall"))
               cleanFirewallOnly = TRUE;
            else
               cleanWFPSamplersOnly = TRUE;
         }

         break;
      }
   }

   status = HlprFwpmEngineOpen(&engineHandle);
   HLPR_BAIL_ON_FAILURE(status);

   if(cleanWFPSamplersOnly)
   {
      HLPR_NEW(pFilterEnumTemplate,
               FWPM_FILTER_ENUM_TEMPLATE);
      HLPR_BAIL_ON_ALLOC_FAILURE(pFilterEnumTemplate,
                                 status);

      HLPR_NEW(pCalloutEnumTemplate,
               FWPM_CALLOUT_ENUM_TEMPLATE);
      HLPR_BAIL_ON_ALLOC_FAILURE(pCalloutEnumTemplate,
                                 status);

      HLPR_NEW(pProviderContextEnumTemplate,
               FWPM_PROVIDER_CONTEXT_ENUM_TEMPLATE);
      HLPR_BAIL_ON_ALLOC_FAILURE(pProviderContextEnumTemplate,
                                 status);

      pFilterEnumTemplate->providerKey = (GUID*)&WFPSAMPLER_PROVIDER;
      pFilterEnumTemplate->enumType    = FWP_FILTER_ENUM_FULLY_CONTAINED;
      pFilterEnumTemplate->flags       = FWP_FILTER_ENUM_FLAG_INCLUDE_BOOTTIME |
                                         FWP_FILTER_ENUM_FLAG_INCLUDE_DISABLED;
      pFilterEnumTemplate->actionMask  = 0xFFFFFFFF;

      pCalloutEnumTemplate->providerKey = (GUID*)&WFPSAMPLER_PROVIDER;

      pProviderContextEnumTemplate->providerKey         = (GUID*)&WFPSAMPLER_PROVIDER;
      pProviderContextEnumTemplate->providerContextType = FWPM_GENERAL_CONTEXT;

      /// enumerate and flush filters
      for(UINT32 layerIndex = 0;
          layerIndex < RTL_NUMBER_OF(ppLayerKeyArray);
          layerIndex++)
      {
         pFilterEnumTemplate->layerKey = *(ppLayerKeyArray[layerIndex]);

         status = HlprFwpmFilterCreateEnumHandle(engineHandle,
                                                 pFilterEnumTemplate,
                                                 &enumHandle);
         HLPR_BAIL_ON_FAILURE_2(status);

         status = HlprFwpmFilterEnum(engineHandle,
                                     enumHandle,
                                     0xFFFFFFFF,
                                     &ppFilters,
                                     &numEntries);
         if(status == NO_ERROR &&
            ppFilters &&
            numEntries)
         {
            for(UINT32 filterIndex = 0;
                filterIndex < numEntries;
                filterIndex++)
            {
               if(cleanFirewallOnly &&
                  HlprFwpmLayerIsUserMode(&(ppFilters[filterIndex]->layerKey)))
                  continue;

               HlprFwpmFilterDeleteByKey(engineHandle,
                                         &(ppFilters[filterIndex]->filterKey));
            }

            numEntries = 0;
         }

         HLPR_BAIL_LABEL_2:

         HlprFwpmFilterDestroyEnumHandle(engineHandle,
                                         &enumHandle);
      }
   }
   else
   {
      status = HlprFwpmFilterCreateEnumHandle(engineHandle,
                                              pFilterEnumTemplate,
                                              &enumHandle);
      HLPR_BAIL_ON_FAILURE(status);

      status = HlprFwpmFilterEnum(engineHandle,
                                  enumHandle,
                                  0xFFFFFFFF,
                                  &ppFilters,
                                  &numEntries);
      if(status == NO_ERROR &&
         ppFilters &&
         numEntries)
      {
         for(UINT32 filterIndex = 0;
             filterIndex < numEntries;
             filterIndex++)
         {
            if(cleanFirewallOnly &&
               HlprFwpmLayerIsUserMode(&(ppFilters[filterIndex]->layerKey)))
               continue;

            HlprFwpmFilterDeleteByKey(engineHandle,
                                      &(ppFilters[filterIndex]->filterKey));
         }

         numEntries = 0;
      }

      HlprFwpmFilterDestroyEnumHandle(engineHandle,
                                      &enumHandle);
   }

   /// Enumerate and flush all Callouts
   status = HlprFwpmCalloutCreateEnumHandle(engineHandle,
                                            pCalloutEnumTemplate,
                                            &enumHandle);
   HLPR_BAIL_ON_FAILURE(status);

   status = HlprFwpmCalloutEnum(engineHandle,
                                enumHandle,
                                0xFFFFFFFF,
                                &ppCallouts,
                                &numEntries);
   if(status == NO_ERROR &&
      ppCallouts &&
      numEntries)
   {
      for(UINT32 calloutIndex = 0;
          calloutIndex < numEntries;
          calloutIndex++)
      {
         if(cleanFirewallOnly &&
            HlprFwpmLayerIsUserMode(&(ppCallouts[calloutIndex]->applicableLayer)))
            continue;

         HlprFwpmCalloutDeleteByKey(engineHandle,
                                    &(ppCallouts[calloutIndex]->calloutKey));
      }

      numEntries = 0;
   }

   HlprFwpmCalloutDestroyEnumHandle(engineHandle,
                                    &enumHandle);

   if(!cleanFirewallOnly)
   {
      /// Enumerate and flush all provider contexts
      status = HlprFwpmProviderContextCreateEnumHandle(engineHandle,
                                                       pProviderContextEnumTemplate,
                                                       &enumHandle);
      HLPR_BAIL_ON_FAILURE(status);

      status = HlprFwpmProviderContextEnum(engineHandle,
                                           enumHandle,
                                           0xFFFFFFFF,
                                           &ppProviderContexts,
                                           &numEntries);
      if(status == NO_ERROR &&
         ppProviderContexts &&
         numEntries)
      {
         for(UINT32 providerContextIndex = 0;
             providerContextIndex < numEntries;
             providerContextIndex++)
         {
            HlprFwpmProviderContextDeleteByKey(engineHandle,
                                               &(ppProviderContexts[providerContextIndex]->providerContextKey));
         }

         numEntries = 0;
      }

      HlprFwpmProviderContextDestroyEnumHandle(engineHandle,
                                               &enumHandle);
   }

   if(!cleanWFPSamplersOnly)
   {
      /// Enumerate and flush all sublayers not owned by WFPSampler
      status = HlprFwpmSubLayerCreateEnumHandle(engineHandle,
                                                0,
                                                &enumHandle);
      HLPR_BAIL_ON_FAILURE(status);

      status = HlprFwpmSubLayerEnum(engineHandle,
                                    enumHandle,
                                    0xFFFFFFFF,
                                    &ppSubLayers,
                                    &numEntries);
      if(status == NO_ERROR &&
         ppSubLayers &&
         numEntries)
      {
         for(UINT32 subLayerIndex = 0;
             subLayerIndex < numEntries;
             subLayerIndex++)
         {
            if(!HlprGUIDsAreEqual(ppSubLayers[subLayerIndex]->providerKey,
                                  &WFPSAMPLER_PROVIDER))
               HlprFwpmSubLayerDeleteByKey(engineHandle,
                                           &(ppSubLayers[subLayerIndex]->subLayerKey));
         }

         numEntries = 0;
      }

      HlprFwpmSubLayerDestroyEnumHandle(engineHandle,
                                        &enumHandle);

      /// Enumerate and flush all providers not owned by WFPSampler
      status = HlprFwpmProviderCreateEnumHandle(engineHandle,
                                                0,
                                                &enumHandle);
      HLPR_BAIL_ON_FAILURE(status);

      status = HlprFwpmProviderEnum(engineHandle,
                                    enumHandle,
                                    0xFFFFFFFF,
                                    &ppProviders,
                                    &numEntries);
      if(status == NO_ERROR &&
         ppProviders &&
         numEntries)
      {
         for(UINT32 providerIndex = 0;
             providerIndex < numEntries;
             providerIndex++)
         {
            if(!HlprGUIDsAreEqual(&(ppProviders[providerIndex]->providerKey),
                                  &WFPSAMPLER_PROVIDER))
               HlprFwpmProviderDeleteByKey(engineHandle,
                                           &(ppProviders[providerIndex]->providerKey));
         }

         numEntries = 0;
      }

      HlprFwpmProviderDestroyEnumHandle(engineHandle,
                                        &enumHandle);
   }

   HLPR_BAIL_LABEL:

   if(engineHandle)
      HlprFwpmEngineClose(&engineHandle);

   HLPR_DELETE(pProviderContextEnumTemplate);

   HLPR_DELETE(pCalloutEnumTemplate);

   HLPR_DELETE(pFilterEnumTemplate);

   return status;
}

/**
 @private_function="PrvLogUsage"
 
   Purpose:  Log usage information to the console.                                              <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
VOID PrvLogUsage(_In_ UINT32 scenario)
{
   wprintf(L"\n\t WFPSampler.Exe \n");
   wprintf(L"\n\t The syntax of this command is: \n\n");

   if(scenario != SCENARIO_UNDEFINED &&
      scenario < SCENARIO_MAX)
   {
      switch(scenario)
      {
         case SCENARIO_ADVANCED_PACKET_INJECTION:
         {
            AdvancedPacketInjectionScenarioLogHelp();

            break;
         }
         case SCENARIO_APP_CONTAINER:
         {

#if(NTDDI_VERSION >= NTDDI_WIN8)

            AppContainerScenarioLogHelp();

#else

            wprintf(L"\n\t\t\t APP_CONTAINER scenario is not supported on this version of Windows \n");

#endif /// (NTDDI_VERSION >= NTDDI_WIN8)

            break;
         }
         case SCENARIO_BASIC_ACTION_BLOCK:
         case SCENARIO_BASIC_ACTION_CONTINUE:
         case SCENARIO_BASIC_ACTION_PERMIT:
         case SCENARIO_BASIC_ACTION_RANDOM:
         {
            BasicActionScenarioLogHelp(scenario);

            break;
         }
         case SCENARIO_BASIC_PACKET_EXAMINATION:
         {
            BasicPacketExaminationScenarioLogHelp();

            break;
         }
         case SCENARIO_BASIC_PACKET_INJECTION:
         {
            BasicPacketInjectionScenarioLogHelp();

            break;
         }
         case SCENARIO_BASIC_PACKET_MODIFICATION:
         {
            BasicPacketModificationScenarioLogHelp();

            break;
         }
         case SCENARIO_BASIC_STREAM_INJECTION:
         {
            BasicStreamInjectionScenarioLogHelp();

            break;
         }
         case SCENARIO_FAST_PACKET_INJECTION:
         {
            FastPacketInjectionScenarioLogHelp();

            break;
         }
         case SCENARIO_FAST_STREAM_INJECTION:
         {
            FastStreamInjectionScenarioLogHelp();

            break;
         }
         case SCENARIO_FLOW_ASSOCIATION:
         {
            FlowAssociationScenarioLogHelp();

            break;
         }
         case SCENARIO_PEND_AUTHORIZATION:
         {
            PendAuthorizationScenarioLogHelp();

            break;
         }
         case SCENARIO_PEND_ENDPOINT_CLOSURE:
         {

#if(NTDDI_VERSION >= NTDDI_WIN7)
            
            PendEndpointClosureScenarioLogHelp();
            
#else
            
            wprintf(L"\n\t\t\t PEND_ENDPOINT_CLOSURE scenario is not supported on this version of Windows \n");

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

            break;
         }
         case SCENARIO_PROXY:
         {
            ProxyScenarioLogHelp();

            break;
         }
      }
   }
   else
   {
      wprintf(L"\n\t\t -?     \t Receive usage information.");
      wprintf(L"\n\t\t -clean \t Remove groups of WFP objects.");
      wprintf(L"\n\t\t        \t    default  \t Removes all of WFPSampler's objects except its Provider and SubLayer. 3rd party policy is preserved. [Optional]");
      wprintf(L"\n\t\t        \t    firewall \t Removes all of WFP's kernel-mode objects except built-in and WFPSampler's Provider and SubLayer. IPsec Policy will be preserved. [Optional]");
      wprintf(L"\n\t\t        \t    all      \t Removes all WFP objects except built-in and WFPSampler's Provider and SubLayer. No policy is preserved. [Optional]");
      wprintf(L"\n\t\t -s     \t Specify one of the following scenarios.");
      wprintf(L"\n\t\t\t\t ADVANCED_PACKET_INJECTION");

#if(NTDDI_VERSION >= NTDDI_WIN8)

      wprintf(L"\n\t\t\t\t APP_CONTAINER");

#endif /// (NTDDI_VERSION >= NTDDI_WIN8)

      wprintf(L"\n\t\t\t\t BASIC_ACTION_BLOCK");
      wprintf(L"\n\t\t\t\t BASIC_ACTION_CONTINUE");
      wprintf(L"\n\t\t\t\t BASIC_ACTION_PERMIT");
      wprintf(L"\n\t\t\t\t BASIC_ACTION_RANDOM");
      wprintf(L"\n\t\t\t\t BASIC_PACKET_EXAMINATION");
      wprintf(L"\n\t\t\t\t BASIC_PACKET_INJECTION");
      wprintf(L"\n\t\t\t\t BASIC_PACKET_MODIFICATION");
      wprintf(L"\n\t\t\t\t BASIC_STREAM_INJECTION");
      wprintf(L"\n\t\t\t\t FAST_PACKET_INJECTION");
      wprintf(L"\n\t\t\t\t FAST_STREAM_INJECTION");
      wprintf(L"\n\t\t\t\t FLOW_ASSOCIATION");
      wprintf(L"\n\t\t\t\t PEND_AUTHORIZATION");
      wprintf(L"\n\t\t\t\t PEND_ENDPOINT_CLOSURE");
      wprintf(L"\n\t\t\t\t PROXY");
      wprintf(L"\n");
      wprintf(L"\n\t Add -? for a scenario's context sensitive help");
      wprintf(L"\n");
      wprintf(L"\n\t\t  i.e.");
      wprintf(L"\n\t\t\t WFPSampler.exe -s BASIC_ACTION_BLOCK -?");
      wprintf(L"\n");
   }

   return;
}

/**
 @framework_function="wmain"

   Purpose:  Entry point for WFPSampler.Exe.  Accepts multiple parameters which are parsed and 
             dispatched to the rest of the program.                                             <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA299386.aspx              <br>
*/
int __cdecl wmain(_In_ const int argumentCount,
                  _In_reads_(argumentCount) PCWSTR pArguments[])
{
   ASSERT(argumentCount);
   ASSERT(pArguments);

   UINT32 status = NO_ERROR;

   /// First argument is the executable's name, WFPSampler.exe,
   /// so start with next argument
   if(argumentCount > 1)
   {
      PCWSTR*                 ppCommandLineParameterStrings = (PCWSTR*)&(pArguments[1]);
      UINT32                  stringCount                   = argumentCount - 1;
      WFPSAMPLER_FLOW_CONTROL flowControl                   = PrvFlowControlGet(ppCommandLineParameterStrings,
                                                                                stringCount);
      WFPSAMPLER_SCENARIO     scenario                      = PrvScenarioGet(ppCommandLineParameterStrings,
                                                                             stringCount,
                                                                             flowControl);

      if(flowControl == FLOW_CONTROL_CLEAN)
      {
         PrvCleanPolicy(ppCommandLineParameterStrings,
                        stringCount);

         if(scenario == SCENARIO_UNDEFINED)
            HLPR_BAIL;
         else
            flowControl = FLOW_CONTROL_NORMAL;
      }

      if(flowControl == FLOW_CONTROL_NORMAL)
      {
         if(scenario == SCENARIO_UNDEFINED ||
            scenario > SCENARIO_MAX)
         {
            status = ERROR_NOT_FOUND;

            HlprLogError(L"wmain : PrvScenarioGet() [status: %#x][scenario: %d]",
                         status,
                         scenario);

            HLPR_BAIL;
         }
      }
      else if(flowControl == FLOW_CONTROL_HELP)
      {
         PrvLogUsage(scenario);

         HLPR_BAIL;
      }

      status = HlprServiceStart(g_pServiceName);
      HLPR_BAIL_ON_FAILURE(status);

      status = RPCClientInterfaceInitialize();
      HLPR_BAIL_ON_FAILURE(status);

      status = PrvScenarioDispatcher(scenario,
                                     ppCommandLineParameterStrings,
                                     stringCount);
      HLPR_BAIL_ON_FAILURE(status);
   }
   else
   {
      status = ERROR_INVALID_PARAMETER;

      HlprLogError(L"wmain() [status: %#x]",
                   status);
   }

   HLPR_BAIL_LABEL:

   RPCClientInterfaceTerminate();

   return (int)status;
}
