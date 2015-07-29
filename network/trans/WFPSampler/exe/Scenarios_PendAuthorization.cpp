////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      Scenarios_PendAuthorization.cpp
//
//   Abstract:
//      This module contains functions which prepares and sends data for the PEND_AUTHORIZATION 
//         scenario implementation.
//
//   Naming Convention:
//
//      <Scope><Object><Action><Modifier>
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
//            PendAuthorizationScenario - Function pertains to all of the Pend Authorization
//                                           Scenarios.
//          }
//       <Action>
//          {
//            Execute                   - Function packages data and invokes RPC to the WFPSampler 
//                                           service.
//            Log                       - Function writes to the console.
//            Parse                     - Function pulls data into the required format from the 
//                                           provided data.
//          }
//       <Modifier>
//          {
//            AuthorizationData         - Function acts on the PC_PEND_AUTHORIZATION_DATA.
//            Help                      - Function provides usage information.
//          }
//
//   Private Functions:
//      PrvPendAuthorizationScenarioParseAuthorizationData(),
//
//   Public Functions:
//      PendAuthorizationScenarioExecute(),
//      PendAuthorizationScenarioLogHelp(),
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Add support for specifying a different sublayer, change 
//                                              parameter, and limit scenario to only supported 
//                                              layers
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Framework_WFPSampler.h" /// .

/**
 @private_function="PrvPendAuthorizationScenarioParseAuthorizationData"
 
   Purpose:  Parse the command line parameters for pending authorization data such as:          <br>
                delay before callout returns (-d DELAY_IN_MS)                                   <br>
                final action - BLOCK         (-fab)                                             <br>
                final action - PERMIT        (-fap)                                             <br>
                                                                                                <br>
   Notes:    If no final action is specified, the action will default to BLOCK.                 <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 PrvPendAuthorizationScenarioParseAuthorizationData(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                                          _In_ const UINT32 stringCount,
                                                          _Inout_ PC_PEND_AUTHORIZATION_DATA* pPCPendAuthorizationData)
{
   ASSERT(ppCLPStrings);
   ASSERT(stringCount);
   ASSERT(pPCPendAuthorizationData);

   UINT32       status         = NO_ERROR;
   const UINT32 MAX_PARAMETERS = 5;
   UINT32       found          = 0;

   for(UINT32 stringIndex = 0;
       stringIndex < stringCount &&
         found != MAX_PARAMETERS;
       stringIndex++)
   {
      /// Final Action BLOCK
      if(HlprStringsAreEqual(L"-fab",
                             ppCLPStrings[stringIndex]))
      {
         pPCPendAuthorizationData->finalAction = FWP_ACTION_BLOCK;

         found++;

         continue;
      }

      /// Final Action PERMIT
      if(HlprStringsAreEqual(L"-fap",
                             ppCLPStrings[stringIndex]))
      {
         pPCPendAuthorizationData->finalAction = FWP_ACTION_PERMIT;

         found++;

         continue;
      }

      if((stringIndex + 1) < stringCount)
      {
         /// Delay (in MS)
         if(HlprStringsAreEqual(L"-pcd",
                                ppCLPStrings[stringIndex]))
         {
            PCWSTR pString = ppCLPStrings[stringIndex + 1];

            if(iswdigit((wint_t)pString[0]))
            {
               UINT32 delay = wcstol(pString,
                                     0,
                                     0);

               pPCPendAuthorizationData->delay = delay;

               found++;
            }

            continue;
         }
      }

      /// Threaded DPC
      if(HlprStringsAreEqual(L"-tdpc",
                             ppCLPStrings[stringIndex]))
      {
         pPCPendAuthorizationData->useThreadedDPC = TRUE;
      
         found++;
      
         continue;
      }

      /// Work Items
      if(HlprStringsAreEqual(L"-wi",
                             ppCLPStrings[stringIndex]))
      {
         pPCPendAuthorizationData->useWorkItems = TRUE;

         found++;

         continue;
      }
   }

   /// Default Final Action BLOCK
   if(pPCPendAuthorizationData->finalAction == 0)
      pPCPendAuthorizationData->finalAction = FWP_ACTION_BLOCK;

   return status;
}

/**
 @scenario_function="PendAuthorizationScenarioExecute"

   Purpose:  Gather and package data neccessary to setup the PEND_AUTHORIZATION scenario, then 
             invoke RPC to implement the scenario in the WFPSampler service.                    <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 PendAuthorizationScenarioExecute(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                        _In_ UINT32 stringCount)
{
   ASSERT(ppCLPStrings);
   ASSERT(stringCount);

   UINT32                      status                   = NO_ERROR;
   BOOLEAN                     removeScenario           = FALSE;
   PC_PEND_AUTHORIZATION_DATA* pPCPendAuthorizationData = 0;
   FWPM_FILTER*                pFilter                  = 0;

   status = HlprFwpmFilterCreate(&pFilter);
   HLPR_BAIL_ON_FAILURE(status);

   pFilter->displayData.name = L"WFPSampler's Pend Authorization Scenario Filter";

   HlprCommandLineParseForScenarioRemoval(ppCLPStrings,
                                          stringCount,
                                          &removeScenario);

   status = HlprCommandLineParseForFilterInfo(ppCLPStrings,
                                              stringCount,
                                              pFilter,
                                              removeScenario);
   HLPR_BAIL_ON_FAILURE(status);

   if(!removeScenario)
   {
      HLPR_NEW(pPCPendAuthorizationData,
               PC_PEND_AUTHORIZATION_DATA);
      HLPR_BAIL_ON_ALLOC_FAILURE(pPCPendAuthorizationData,
                                 status);

      status = PrvPendAuthorizationScenarioParseAuthorizationData(ppCLPStrings,
                                                                  stringCount,
                                                                  pPCPendAuthorizationData);
      HLPR_BAIL_ON_FAILURE(status);
   }

   status = RPCInvokeScenarioPendAuthorization(wfpSamplerBindingHandle,
                                               SCENARIO_PEND_AUTHORIZATION,
                                               removeScenario ? FWPM_CHANGE_DELETE : FWPM_CHANGE_ADD,
                                               pFilter,
                                               pPCPendAuthorizationData);
   if(status != NO_ERROR)
      HlprLogError(L"PendAuthorizationScenarioExecute : RpcInvokeScenarioPendAuthorization() [status: %#x]",
                   status);
   else
      HlprLogInfo(L"PendAuthorizationScenarioExecute : RpcInvokeScenarioPendAuthorization() [status: %#x]",
                  status);

   HLPR_BAIL_LABEL:

   if(pFilter)
      HlprFwpmFilterDestroy(&pFilter);

   HLPR_DELETE(pPCPendAuthorizationData);

   return status;
}

/**
 @public_function="PendAuthorizationScenarioLogHelp"
 
   Purpose:  Log usage information for the PEND_AUTHORIZATION scenario to the console.          <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
VOID PendAuthorizationScenarioLogHelp()
{
   wprintf(L"\n\t\t -s     \t PEND_AUTHORIZATION");
   wprintf(L"\n\t\t -?     \t Receive usage information.");
   wprintf(L"\n\t\t -l     \t Specify the layer to perform the filtering. [Required]");
   wprintf(L"\n\t\t -sl    \t Specify the sublayer to perform the filtering. [Optional]");
   wprintf(L"\n\t\t -r     \t Remove the scenario objects.");
   wprintf(L"\n\t\t -v     \t Make the filter volatile (non-persistent). [Optional]");
   wprintf(L"\n\t\t -b     \t Makes the objects available during boot time. [Optional]");
   wprintf(L"\n\t\t -tdpc  \t Use threaded DPCs for out of band. [Optional]");
   wprintf(L"\n\t\t -wi    \t Use work items for out of band. [Optional]");
   wprintf(L"\n\t\t -pcd   \t Introduce a delay (in ms) before the pend is completed. [Optional]");
   wprintf(L"\n\t\t -fab   \t Specify the final action to be block. [Optional][Default]");
   wprintf(L"\n\t\t -fap   \t Specify the final action to be permit. [Optional]");
   wprintf(L"\n\t\t -ipla  \t Specify the IP_LOCAL_ADDRESS /");
   wprintf(L"\n\t\t        \t    IP_SOURCE_ADDRESS to filter. [Optional]");
   wprintf(L"\n\t\t -ipra  \t Specify the IP_REMOTE_ADDRESS /");
   wprintf(L"\n\t\t        \t    IP_DESTINATION_ADDRESS to filter. [Optional]");
   wprintf(L"\n\t\t -ipp   \t Specify the IP_PROTOCOL to filter. [Optional]");
   wprintf(L"\n\t\t -iplp  \t Specify the IP_LOCAL_PORT to filter. [Optional]");
   wprintf(L"\n\t\t -icmpt \t Specify the ICMP_TYPE to filter. [Optional]");
   wprintf(L"\n\t\t -iprp  \t Specify the IP_REMOTE_PORT to filter. [Optional]");
   wprintf(L"\n\t\t -icmpc \t Specify the ICMP_CODE to filter. [Optional]");
   wprintf(L"\n");
   wprintf(L"\n\t i.e.");
   wprintf(L"\n\t\t  WFPSampler.Exe -s PEND_AUTHORIZATION -l FWPM_LAYER_ALE_AUTH_CONNECT_V4 -ipla 1.0.0.1 -ipra 1.0.0.254 -ipp TCP -d 10000 -fap -v");
   wprintf(L"\n\t\t  WFPSampler.Exe -s PEND_AUTHORIZATION -l FWPM_LAYER_ALE_AUTH_CONNECT_V4 -ipla 1.0.0.1 -ipra 1.0.0.254 -ipp TCP -d 10000 -fap -v -r");
   wprintf(L"\n");

   return;
}
