////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      Scenarios_BasicAction.cpp
//
//   Abstract:
//      This module contains functions which prepares and sends data for the BASIC_ACTION_* scenario
//         implementation.
//
//   Naming Convention:
//
//      <Scope><Object><Action><Modifier>
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
//            BasicActionBlockScenario    - Function pertains to the Basic Action Block Scenario.
//            BasicActionContinueScenario - Function pertains to the Basic Action Continue Scenario.
//            BasicActionPermitScenario   - Function pertains to the Basic Action Permit Scenario.
//            BasicActionRandomScenario   - Function pertains to the Basic Action Random Scenario.
//            BasicActionScenario         - Function pertains to all of the Basic Action Scenarios.
//          }
//       <Action>
//          {
//            Execute                     - Function packages data and invokes RPC to the WFPSampler
//                                             service.
//            Log                         - Function writes to the console.
//            Parse                       - Function pulls data into the required format from the
//                                             provided data.
//          }
//       <Modifier>
//          {
//            Help                        - Function provides context sensitive help for the 
//                                             scenario.
//            RandomizedData              - Function acts on the PC_BASIC_ACTION_DATA.
//          }
//
//   Private Functions:
//      PrvBasicActionScenarioParseRandomizedData(),
//
//   Public Functions:
//      BasicActionBlockScenarioExecute(),
//      BasicActionContinueScenarioExecute(),
//      BasicActionPermitScenarioExecute(),
//      BasicActionRandomScenarioExecute(),
//      BasicActionScenarioLogHelp(),
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Add support for specifying a different sublayer
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Framework_WFPSampler.h" /// .

/**
 @private_function="PrvBasicActionScenarioParseRandomizedData"
 
   Purpose:  Parse the command line parameters for implementing randomization such as:          <br>
                Random chance of returning block        (-rab PERCENTAGE)                       <br>
                Random chance of returning continue     (-rac PERCENTAGE)                       <br>
                Random chance of returning permit       (-rap PERCENTAGE)                       <br>
                                                                                                <br>
   Notes:    This is only used if using SCENARIO_BASIC_ACTION_RANDOM.                           <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 PrvBasicActionScenarioParseRandomizedData(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                                 _In_ const UINT32 stringCount,
                                                 _Inout_ PC_BASIC_ACTION_DATA* pPCBasicActionData)
{
   ASSERT(ppCLPStrings);
   ASSERT(stringCount);
   ASSERT(pPCBasicActionData);

   UINT32 status = NO_ERROR;

   for(UINT32 stringIndex = 0;
       (stringIndex + 1) < stringCount;
       stringIndex++)
   {
      const UINT32 WHOLE_PERCENTAGE = 100;

      /// Random FWP_ACTION_BLOCK
      if(HlprStringsAreEqual(L"-rab",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[stringIndex + 1];

         if(iswdigit((wint_t)pString[0]))
         {
            UINT32 percentBlock = wcstol(pString,
                                         0,
                                         0);
            if(percentBlock <= WHOLE_PERCENTAGE)
               pPCBasicActionData->percentBlock = percentBlock & 0xFF;
         }

         continue;
      }

      /// Random FWP_ACTION_CONTINUE
      if(HlprStringsAreEqual(L"-rac",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[stringIndex + 1];

         if(iswdigit((wint_t)pString[0]))
         {
            UINT32 percentContinue = wcstol(pString,
                                            0,
                                            0);
            if(percentContinue <= WHOLE_PERCENTAGE)
               pPCBasicActionData->percentContinue = percentContinue & 0xFF;
         }

         continue;
      }

      /// FWP_ACTION_PERMIT
      if(HlprStringsAreEqual(L"-rap",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[stringIndex + 1];

         if(iswdigit((wint_t)pString[0]))
         {
            UINT32 percentPermit = wcstol(pString,
                                          0,
                                          0);
            if(percentPermit <= WHOLE_PERCENTAGE)
               pPCBasicActionData->percentPermit = percentPermit & 0xFF;
         }

         continue;
      }

      if(pPCBasicActionData->percentBlock &&
         pPCBasicActionData->percentContinue &&
         pPCBasicActionData->percentPermit)
         break;
   }

   /// Set the chance of each action for Randomization
   if(pPCBasicActionData->percentBlock == 0 &&
      pPCBasicActionData->percentContinue == 0 &&
      pPCBasicActionData->percentPermit == 0)
   {
      pPCBasicActionData->percentBlock    = 50;
      pPCBasicActionData->percentContinue = 25;
      pPCBasicActionData->percentPermit   = 25;
   }
   else
   {
      if(pPCBasicActionData->percentBlock == 0)
         pPCBasicActionData->percentBlock = 100 - (pPCBasicActionData->percentContinue + pPCBasicActionData->percentPermit);

      if(pPCBasicActionData->percentContinue == 0)
         pPCBasicActionData->percentContinue = 100 - (pPCBasicActionData->percentBlock + pPCBasicActionData->percentPermit);

      if(pPCBasicActionData->percentPermit == 0)
         pPCBasicActionData->percentPermit = 100 - (pPCBasicActionData->percentBlock + pPCBasicActionData->percentContinue);
   }

   if((pPCBasicActionData->percentBlock + 
      pPCBasicActionData->percentContinue +
      pPCBasicActionData->percentPermit) > 100)
   {
      pPCBasicActionData->percentBlock    = 50;
      pPCBasicActionData->percentContinue = 25;
      pPCBasicActionData->percentPermit   = 25;
   }

   return status;
}

/**
 @scenario_function="BasicActionBlockScenarioExecute"

   Purpose:  Gather and package data neccessary to setup the BASIC_ACTION_BLOCK scenario, then 
             invoke RPC to implement the scenario in the WFPSampler service.                    <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 BasicActionBlockScenarioExecute(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                       _In_ UINT32 stringCount)
{
   ASSERT(ppCLPStrings);
   ASSERT(stringCount);

   UINT32       status         = NO_ERROR;
   BOOLEAN      removeScenario = FALSE;
   FWPM_FILTER* pFilter        = 0;

   status = HlprFwpmFilterCreate(&pFilter);
   HLPR_BAIL_ON_FAILURE(status);

   pFilter->displayData.name = L"WFPSampler's Basic Action Block Scenario Filter";

   HlprCommandLineParseForScenarioRemoval(ppCLPStrings,
                                          stringCount,
                                          &removeScenario);

   status = HlprCommandLineParseForFilterInfo(ppCLPStrings,
                                              stringCount,
                                              pFilter,
                                              removeScenario);
   HLPR_BAIL_ON_FAILURE(status);

   status = RPCInvokeScenarioBasicAction(wfpSamplerBindingHandle,
                                         SCENARIO_BASIC_ACTION_BLOCK,
                                         removeScenario ? FWPM_CHANGE_DELETE : FWPM_CHANGE_ADD,
                                         pFilter,
                                         0);
   if(status != NO_ERROR)
      HlprLogError(L"BasicActionBlockScenarioExecute : RpcInvokeScenarioBasicAction() [status: %#x]",
                   status);
   else
      HlprLogInfo(L"BasicActionBlockScenarioExecute : RpcInvokeScenarioBasicAction() [status: %#x]",
                  status);

   HLPR_BAIL_LABEL:

   return status;
}

/**
 @scenario_function="BasicActionContinueScenarioExecute"

   Purpose:  Gather and package data neccessary to setup the BASIC_ACTION_CONTINUE scenario, 
             then invoke RPC to implement the scenario in the WFPSampler service.               <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 BasicActionContinueScenarioExecute(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                          _In_ UINT32 stringCount)
{
   ASSERT(ppCLPStrings);
   ASSERT(stringCount);

   UINT32       status         = NO_ERROR;
   BOOLEAN      removeScenario = FALSE;
   FWPM_FILTER* pFilter        = 0;

   status = HlprFwpmFilterCreate(&pFilter);
   HLPR_BAIL_ON_FAILURE(status);

   pFilter->displayData.name = L"WFPSampler's Basic Action Continue Scenario Filter";

   HlprCommandLineParseForScenarioRemoval(ppCLPStrings,
                                          stringCount,
                                          &removeScenario);

   status = HlprCommandLineParseForFilterInfo(ppCLPStrings,
                                              stringCount,
                                              pFilter,
                                              removeScenario);
   HLPR_BAIL_ON_FAILURE(status);

   status = RPCInvokeScenarioBasicAction(wfpSamplerBindingHandle,
                                         SCENARIO_BASIC_ACTION_CONTINUE,
                                         removeScenario ? FWPM_CHANGE_DELETE : FWPM_CHANGE_ADD,
                                         pFilter,
                                         0);
   if(status != NO_ERROR)
      HlprLogError(L"BasicActionContinueScenarioExecute : RpcInvokeScenarioBasicAction() [status: %#x]",
                   status);
   else
      HlprLogInfo(L"BasicActionContinueScenarioExecute : RpcInvokeScenarioBasicAction() [status: %#x]",
                  status);

   HLPR_BAIL_LABEL:

   if(pFilter)
      HlprFwpmFilterDestroy(&pFilter);

   return status;
}

/**
 @scenario_function="BasicActionPermitScenarioExecute"

   Purpose:  Gather and package data neccessary to setup the BASIC_ACTION_PERMIT scenario, then 
             invoke RPC to implement the scenario in the WFPSampler service.                    <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 BasicActionPermitScenarioExecute(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                        _In_ UINT32 stringCount)
{
   ASSERT(ppCLPStrings);
   ASSERT(stringCount);

   UINT32       status         = NO_ERROR;
   BOOLEAN      removeScenario = FALSE;
   FWPM_FILTER* pFilter        = 0;

   status = HlprFwpmFilterCreate(&pFilter);
   HLPR_BAIL_ON_FAILURE(status);

   pFilter->displayData.name = L"WFPSampler's Basic Action Permit Scenario Filter";

   HlprCommandLineParseForScenarioRemoval(ppCLPStrings,
                                          stringCount,
                                          &removeScenario);

   status = HlprCommandLineParseForFilterInfo(ppCLPStrings,
                                              stringCount,
                                              pFilter,
                                              removeScenario);
   HLPR_BAIL_ON_FAILURE(status);

   status = RPCInvokeScenarioBasicAction(wfpSamplerBindingHandle,
                                         SCENARIO_BASIC_ACTION_PERMIT,
                                         removeScenario ? FWPM_CHANGE_DELETE : FWPM_CHANGE_ADD,
                                         pFilter,
                                         0);
   if(status != NO_ERROR)
      HlprLogError(L"BasicActionPermitScenarioExecute : RpcInvokeScenarioBasicAction() [status: %#x]",
                   status);
   else
      HlprLogInfo(L"BasicActionPermitScenarioExecute : RpcInvokeScenarioBasicAction() [status: %#x]",
                  status);

   HLPR_BAIL_LABEL:

   if(pFilter)
      HlprFwpmFilterDestroy(&pFilter);

   return status;
}

/**
 @scenario_function="BasicActionRandomScenarioExecute"

   Purpose:  Gather and package data neccessary to setup the BASIC_ACTION_RANDOM scenario, then 
             invoke RPC to implement the scenario in the WFPSampler service.                    <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 BasicActionRandomScenarioExecute(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                        _In_ UINT32 stringCount)
{
   UINT32                status             = NO_ERROR;
   BOOLEAN               removeScenario     = FALSE;
   PC_BASIC_ACTION_DATA* pPCBasicActionData = 0;
   FWPM_FILTER*          pFilter            = 0;

   status = HlprFwpmFilterCreate(&pFilter);
   HLPR_BAIL_ON_FAILURE(status);

   pFilter->displayData.name = L"WFPSampler's Basic Action Random Scenario Filter";

   HlprCommandLineParseForScenarioRemoval(ppCLPStrings,
                                          stringCount,
                                          &removeScenario);

   status = HlprCommandLineParseForFilterInfo(ppCLPStrings,
                                              stringCount,
                                              pFilter,
                                              removeScenario);
   HLPR_BAIL_ON_FAILURE(status);

   pFilter->action.type = FWP_ACTION_CALLOUT_UNKNOWN;

   if(!removeScenario)
   {
      HLPR_NEW(pPCBasicActionData,
               PC_BASIC_ACTION_DATA);
      HLPR_BAIL_ON_ALLOC_FAILURE(pPCBasicActionData,
                                 status);

      status = PrvBasicActionScenarioParseRandomizedData(ppCLPStrings,
                                                         stringCount,
                                                         pPCBasicActionData);
      HLPR_BAIL_ON_FAILURE(status);
   }

   status = RPCInvokeScenarioBasicAction(wfpSamplerBindingHandle,
                                         SCENARIO_BASIC_ACTION_RANDOM,
                                         removeScenario ? FWPM_CHANGE_DELETE : FWPM_CHANGE_ADD,
                                         pFilter,
                                         pPCBasicActionData);
   if(status != NO_ERROR)
      HlprLogError(L"BasicActionRandomScenarioExecute : RpcInvokeScenarioBasicAction() [status: %#x]",
                   status);
   else
      HlprLogInfo(L"BasicActionRandomScenarioExecute : RpcInvokeScenarioBasicAction() [status: %#x]",
                  status);

   HLPR_BAIL_LABEL:

   if(pFilter)
      HlprFwpmFilterDestroy(&pFilter);

   HLPR_DELETE(pPCBasicActionData);

   return status;
}

/**
 @public_function="BasicActionScenarioLogHelp"
 
   Purpose:  Log usage information for the BASIC_ACTION* scenarios to the console.              <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
VOID BasicActionScenarioLogHelp(_In_ const UINT32 scenario)
{
   PWSTR pScenario   = L"BASIC_ACTION_BLOCK";
   PWSTR pRandomInfo = L" ";

   if(scenario == SCENARIO_BASIC_ACTION_CONTINUE)
      pScenario = L"BASIC_ACTION_CONTINUE";
   else if(scenario == SCENARIO_BASIC_ACTION_PERMIT)
      pScenario = L"BASIC_ACTION_PERMIT";
   else if(scenario == SCENARIO_BASIC_ACTION_RANDOM)
      pScenario = L"BASIC_ACTION_RANDOM";

   wprintf(L"\n\t\t -s     \t %s",
           pScenario);
   wprintf(L"\n\t\t -?     \t Receive usage information.");
   wprintf(L"\n\t\t -l     \t Specify the layer to perform the filtering. [Required]");
   wprintf(L"\n\t\t -sl    \t Specify the sublayer to perform the filtering. [Optional]");
   wprintf(L"\n\t\t -r     \t Remove the scenario objects.");
   wprintf(L"\n\t\t -c     \t Use a callout to perform the action. [Optional]");
   wprintf(L"\n\t\t -v     \t Make the filter volatile (non-persistent). [Optional]");
   wprintf(L"\n\t\t -b     \t Makes the objects available during boot time. [Optional]");
   wprintf(L"\n\t\t -ipla  \t Specify the IP_LOCAL_ADDRESS /");
   wprintf(L"\n\t\t        \t    IP_SOURCE_ADDRESS to filter. [Optional]");
   wprintf(L"\n\t\t -ipra  \t Specify the IP_REMOTE_ADDRESS /");
   wprintf(L"\n\t\t        \t    IP_DESTINATION_ADDRESS to filter. [Optional]");
   wprintf(L"\n\t\t -ipp   \t Specify the IP_PROTOCOL to filter. [Optional]");
   wprintf(L"\n\t\t -iplp  \t Specify the IP_LOCAL_PORT to filter. [Optional]");
   wprintf(L"\n\t\t -icmpt \t Specify the ICMP_TYPE to filter. [Optional]");
   wprintf(L"\n\t\t -iprp  \t Specify the IP_REMOTE_PORT to filter. [Optional]");
   wprintf(L"\n\t\t -icmpc \t Specify the ICMP_CODE to filter. [Optional]");

   if(scenario == SCENARIO_BASIC_ACTION_RANDOM)
   {
      pRandomInfo = L"-rab 50 -rac 25 -rap 25 ";

      wprintf(L"\n\t\t -rab   \t Percentage chance of returning FWP_ACTION_BLOCK. [Optional]");
      wprintf(L"\n\t\t -rac   \t Percentage chance of returning FWP_ACTION_CONTINUE. [Optional]");
      wprintf(L"\n\t\t -rap   \t Percentage chance of returning FWP_ACTION_PERMIT. [Optional]");
   }

   wprintf(L"\n");
   wprintf(L"\n\t i.e.");
   wprintf(L"\n\t\t WFPSampler.Exe -s %s -l FWPM_LAYER_INBOUND_TRANSPORT_V4 -ipla 1.0.0.1 -ipra 1.0.0.254 -ipp TCP -c %s",
           pScenario,
           pRandomInfo);
   wprintf(L"\n\t\t WFPSampler.Exe -s %s -l FWPM_LAYER_INBOUND_TRANSPORT_V4 -ipla 1.0.0.1 -ipra 1.0.0.254 -ipp TCP -c %s-r",
           pScenario,
           pRandomInfo);
   wprintf(L"\n");

   return;
}
