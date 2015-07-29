////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      Scenarios_BasicStreamInjection.cpp
//
//   Abstract:
//      This module contains functions which prepares and sends data for the BASIC_STREAM_INJECTION 
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
//                                         - Function is likely visible to other modules.
//            Prv                          - Function is private to this module.
//          }
//       <Object>
//          {
//            BasicStreamInjectionScenario - Function pertains to the Basic Stream Injection 
//                                              Scenario.
//          }
//       <Action>
//          {
//            Execute                      - Function packages data and invokes RPC to the 
//                                              WFPSampler service.
//            Log                          - Function writes to the console.
//            Parse                        - Function pulls data into the required format from the 
//                                              provided data.
//          }
//       <Modifier>
//          {
//            Help                         - Function provides context sensitive help for the 
//                                              scenario.
//            InjectionData                - Function acts on the PC_BASIC_STREAM_INJECTION_DATA.
//          }
//
//   Private Functions:
//      PrvScenarioBasicStreamInjectionParseInjectionData(),
//
//   Public Functions:
//      BasicStreamInjectionScenarioExecute(),
//      BasicStreamInjectionScenarioLogHelp(),
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      June      15,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Add support for specifying a different sublayer
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Framework_WFPSampler.h" /// .

/**
 @private_function="PrvBasicStreamInjectionScenarioParseInjectionData"
 
   Purpose:  Parse the command line parameters for implementing stream injection such as:       <br>
                Perform the injection inline (from within the classify)    (-in)                <br>
                Use threaded DPCs for out of band (asynchronous)           (-tdpc)              <br>
                Use work items for out of band (asynchronous)              (-wi)                <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 PrvBasicStreamInjectionScenarioParseInjectionData(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                                         _In_ const UINT32 stringCount,
                                                         _Inout_ PC_BASIC_STREAM_INJECTION_DATA* pPCBasicStreamInjectionData)
{
   ASSERT(ppCLPStrings);
   ASSERT(stringCount);
   ASSERT(pPCBasicStreamInjectionData);

   UINT32       status         = NO_ERROR;
   const UINT32 MAX_PARAMETERS = 3;
   UINT32       found          = 0;

   for(UINT32 stringIndex = 0;
       stringIndex < stringCount &&
       found != MAX_PARAMETERS;
       stringIndex++)
   {
      /// Inline Injection
      if(HlprStringsAreEqual(L"-in",
                             ppCLPStrings[stringIndex]))
      {
         pPCBasicStreamInjectionData->performInline = TRUE;

         found++;

         continue;
      }

      /// Threaded DPC
      if(HlprStringsAreEqual(L"-tdpc",
                             ppCLPStrings[stringIndex]))
      {
         pPCBasicStreamInjectionData->useThreadedDPC = TRUE;
      
         found++;
      
         continue;
      }
      
      /// Work Items
      if(HlprStringsAreEqual(L"-wi",
                             ppCLPStrings[stringIndex]))
      {
         pPCBasicStreamInjectionData->useWorkItems = TRUE;
      
         found++;
      
         continue;
      }
   }

   return status;
}

/**
 @scenario_function="BasicStreamInjectionScenarioExecute"

   Purpose:  Gather and package data neccessary to setup the BASIC_STREAM_INJECTION scenario, 
             then invoke RPC to implement the scenario in the WFPSampler service.               <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 BasicStreamInjectionScenarioExecute(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                           _In_ const UINT32 stringCount)
{
   ASSERT(ppCLPStrings);
   ASSERT(stringCount);

   UINT32                          status                      = NO_ERROR;
   BOOLEAN                         removeScenario              = FALSE;
   PC_BASIC_STREAM_INJECTION_DATA* pPCBasicStreamInjectionData = 0;
   FWPM_FILTER*                    pFilter                     = 0;

   status = HlprFwpmFilterCreate(&pFilter);
   HLPR_BAIL_ON_FAILURE(status);

   pFilter->displayData.name = L"WFPSampler's Basic Stream Injection Scenario Filter";

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
      HLPR_NEW(pPCBasicStreamInjectionData,
               PC_BASIC_STREAM_INJECTION_DATA);
      HLPR_BAIL_ON_ALLOC_FAILURE(pPCBasicStreamInjectionData,
                                 status);

      status = PrvBasicStreamInjectionScenarioParseInjectionData(ppCLPStrings,
                                                                 stringCount,
                                                                 pPCBasicStreamInjectionData);
      HLPR_BAIL_ON_FAILURE(status);
   }

   status = RPCInvokeScenarioBasicStreamInjection(wfpSamplerBindingHandle,
                                                  SCENARIO_BASIC_STREAM_INJECTION,
                                                  removeScenario ? FWPM_CHANGE_DELETE : FWPM_CHANGE_ADD,
                                                  pFilter,
                                                  pPCBasicStreamInjectionData);
   if(status != NO_ERROR)
      HlprLogError(L"BasicStreamInjectionScenarioExecute : RpcInvokeScenarioBasicStreamInjection() [status: %#x]",
                   status);
   else
      HlprLogInfo(L"BasicStreamInjectionScenarioExecute : RpcInvokeScenarioBasicStreamInjection() [status: %#x]",
                  status);

   HLPR_BAIL_LABEL:

   if(pFilter)
      HlprFwpmFilterDestroy(&pFilter);

   HLPR_DELETE(pPCBasicStreamInjectionData);

   return status;
}

/**
 @public_function="BasicStreamInjectionScenarioLogHelp"
 
   Purpose:  Log usage information for the BASIC_STREAM_INJECTION scenario to the console.      <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
VOID BasicStreamInjectionScenarioLogHelp()
{
   wprintf(L"\n\t\t -s     \t BASIC_STREAM_INJECTION");
   wprintf(L"\n\t\t -?     \t Receive usage information.");
   wprintf(L"\n\t\t -l     \t Specify the layer to perform the filtering. [Required]");
   wprintf(L"\n\t\t -sl    \t Specify the sublayer to perform the filtering. [Optional]");
   wprintf(L"\n\t\t -r     \t Remove the scenario objects.");
   wprintf(L"\n\t\t -v     \t Make the filter volatile (non-persistent). [Optional]");
   wprintf(L"\n\t\t -b     \t Makes the objects available during boot time. [Optional]");
   wprintf(L"\n\t\t -in    \t  Perform the injection inline if possible. [Optional]");
   wprintf(L"\n\t\t -tdpc  \t Use threaded DPCs for out of band. [Optional]");
   wprintf(L"\n\t\t -wi    \t Use work items for out of band. [Optional]");
   wprintf(L"\n\t\t -ipla  \t Specify the IP_LOCAL_ADDRESS /");
   wprintf(L"\n\t\t        \t    IP_SOURCE_ADDRESS to filter. [Optional]");
   wprintf(L"\n\t\t -ipra  \t Specify the IP_REMOTE_ADDRESS /");
   wprintf(L"\n\t\t        \t    IP_DESTINATION_ADDRESS to filter. [Optional]");
   wprintf(L"\n\t\t -iplp  \t Specify the IP_LOCAL_PORT to filter. [Optional]");
   wprintf(L"\n\t\t -iprp  \t Specify the IP_REMOTE_PORT to filter. [Optional]");
   wprintf(L"\n");
   wprintf(L"\n\t i.e.");
   wprintf(L"\n\t\t  WFPSampler.Exe -s BASIC_STREAM_INJECTION -l FWPM_LAYER_STREAM_V4 -ipla 1.0.0.1 -ipra 1.0.0.254 -v");
   wprintf(L"\n\t\t  WFPSampler.Exe -s BASIC_STREAM_INJECTION -l FWPM_LAYER_STREAM_V4 -ipla 1.0.0.1 -ipra 1.0.0.254 -v -r");
   wprintf(L"\n");

   return;
}
