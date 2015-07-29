////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      Scenarios_AppContainers.cpp
//
//   Abstract:
//      This module contains functions which prepares and sends data for the APPLICATION_CONTAINER 
//      scenario implementation.
//
//   Naming Convention:
//
//      <Scope><Object><Action><Modifier>
//  
//      i.e.
//
//       <Scope>
//          {
//                                 - Function is likely visible to other modules.
//            Prv                  - Function is private to this module.
//          }
//       <Object>
//          {
//            AppContainerScenario - Function pertains to all of the Application Container Scenarios.
//          }
//       <Action>
//          {
//            Execute              - Function packages data and invokes RPC to the WFPSampler 
//                                      service.
//            Log                  - Function writes to the console.
//            Parse                - Function pulls data into the required format from the provided 
//                                      data.
//          }
//       <Modifier>
//          {
//            DataForModifiers     - Function acts on scenario modifiers such as volatility and
//                                      removal.
//            Help                 - Function provides context sensitive help for the scenario.
//          }
//
//   Public Functions:
//      AppContainerScenarioExecute(),
//      AppContainerScenarioLogHelp
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Framework_WFPSampler.h" /// .

#if(NTDDI_VERSION >= NTDDI_WIN8)

/**
 @private_function="PrvAppContainerScenarioParseDataForModifiers"
 
   Purpose:  Parse the command line parameters for any scenario modifiers such as:              <br>
                trust Windows Service Hardening        (-trustWSH)                              <br>
                add own filters for containers         (-distrustWSH)                           <br>
                use a callout                          (-c)                                     <br>
                make volatile / non persistent         (-v)                                     <br>
                mark objects available during boottime (-b)                                     <br>
                remove scenario                        (-r)                                     <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 PrvAppContainerScenarioParseDataForModifiers(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                                    _In_ UINT32 stringCount,
                                                    _Inout_ BOOLEAN* pTrustWSH,
                                                    _Inout_ BOOLEAN* pRemoveScenario,
                                                    _Inout_ BOOLEAN* pIsPersistent,
                                                    _Inout_ BOOLEAN* pIsBootTime)
{
   ASSERT(ppCLPStrings);
   ASSERT(stringCount);
   ASSERT(pTrustWSH);
   ASSERT(pRemoveScenario);
   ASSERT(pIsPersistent);
   ASSERT(pIsBootTime);

   UINT32 status = NO_ERROR;

   for(UINT32 stringIndex = 0;
       stringIndex < stringCount;
       stringIndex++)
   {
      /// Trust that Windows Service Hardening rules are enforcing AppContainer constraints
      if(HlprStringsAreEqual(L"-trustWSH",
                             ppCLPStrings[stringIndex]))
      {
         *pTrustWSH = TRUE;

         continue;
      }

      /// Distrust that Windows Service Hardening rules are enforcing AppContainer constraints
      if(HlprStringsAreEqual(L"-distrustWSH",
                             ppCLPStrings[stringIndex]))
      {
         *pTrustWSH = FALSE;

         continue;
      }

      /// Remove any filter with this setup
      if(HlprStringsAreEqual(L"-r",
                             ppCLPStrings[stringIndex]))
      {
         *pRemoveScenario = TRUE;

         continue;
      }

      /// Make the objects volatile
      if(HlprStringsAreEqual(L"-v",
                             ppCLPStrings[stringIndex]) &&
         *pIsBootTime == FALSE)
      {
         *pIsPersistent = FALSE;

         continue;
      }

      /// Mark the objects available during BootTime
      if(HlprStringsAreEqual(L"-b",
                             ppCLPStrings[stringIndex]))
      {
         *pIsPersistent = TRUE;

         *pIsBootTime = TRUE;

         continue;
      }
   }

   return status;
}

/**
 @scenario_function="AppContainerScenarioExecute"

   Purpose:  Gather and package data neccessary to setup the APPLICATION_CONTAINER scenario, 
             then invoke RPC to implement the scenario in the WFPSampler service.               <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 AppContainerScenarioExecute(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                   _In_ UINT32 stringCount)
{
   ASSERT(ppCLPStrings);
   ASSERT(stringCount);

   UINT32  status         = NO_ERROR;
   BOOLEAN trustWSH       = TRUE;
   BOOLEAN removeScenario = FALSE;
   BOOLEAN persistent     = TRUE;
   BOOLEAN bootTime       = FALSE;

   status = PrvAppContainerScenarioParseDataForModifiers(ppCLPStrings,
                                                        stringCount,
                                                        &trustWSH,
                                                        &removeScenario,
                                                        &persistent,
                                                        &bootTime);
   HLPR_BAIL_ON_FAILURE(status);

   status = RPCInvokeScenarioAppContainer(wfpSamplerBindingHandle,
                                          SCENARIO_APP_CONTAINER,
                                          removeScenario ? FWPM_CHANGE_DELETE : FWPM_CHANGE_ADD,
                                          trustWSH,
                                          persistent,
                                          bootTime);
   if(status != NO_ERROR)
      HlprLogError(L"AppContainerScenarioExecute : RPCInvokeScenarioAppContainer() [status: %#x]",
                   status);
   else
      HlprLogInfo(L"AppContainerScenarioExecute : RPCInvokeScenarioAppContainer() [status: %#x]",
                  status);

   HLPR_BAIL_LABEL:

   return status;
}

/**
 @public_function="AppContainerScenarioLogHelp"
 
   Purpose:  Log usage information to the console for the APP_CONTAINER scenario.               <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
VOID AppContainerScenarioLogHelp()
{
   PWSTR pScenario = L"APP_CONTAINER";

   wprintf(L"\n\t\t -s           \t %s ",
           pScenario);
   wprintf(L"\n\t\t -?           \t Receive usage information.");
   wprintf(L"\n\t\t -r           \t Remove the scenario objects.");
   wprintf(L"\n\t\t -v           \t Make the objects volatile (non-persistent). [Optional]");
   wprintf(L"\n\t\t -b           \t Makes the objects available during boot time. [Optional]");
   wprintf(L"\n\t\t -trustWSH    \t Adds filters that inherently allows Windows Service Hardening");
   wprintf(L"\n\t\t              \t    to handle all App Container decisions. (Default / Recommended)");
   wprintf(L"\n\t\t -distrustWSH \t Enumerates current WSH filters and subscribes to receive future");
   wprintf(L"\n\t\t              \t    filters.  Adds granular filters to allow contained apps.");
   wprintf(L"\n");
   wprintf(L"\n\t i.e.");
   wprintf(L"\n\t\t WFPSampler.Exe -s %s -trustWSH -v",
           pScenario);
   wprintf(L"\n\t\t WFPSampler.Exe -s %s -trustWSH -v -r",
           pScenario);
   wprintf(L"\n");

   return;
}

#endif /// (NTDDI_VERSION >= NTDDI_WIN8)
