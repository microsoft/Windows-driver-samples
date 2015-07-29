////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      Scenarios_FastStreamInjection.cpp
//
//   Abstract:
//      This module contains functions which prepares and sends data for the FAST_STREAM_INJECTION
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
//            FastStreamInjectionScenario  - Function pertains to Fast Stream Injection Scenario.
//          }
//       <Action>
//          {
//            Execute                      - Function packages data and invokes RPC to the 
//                                              WFPSampler service
//            Log                          - Function writes to the console.
//            Parse                        - Function pulls data into the required format from the 
//                                              provided data.
//          }
//       <Modifier>
//          {
//            Help                         - Function provides context sensitive help for the 
//                                              scenario.
//          }
//
//   Private Functions:
//
//   Public Functions:
//      FastStreamInjectionScenarioExecute(),
//      FastStreamInjectionScenarioLogHelp(),
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
 @scenario_function="FastStreamInjectionScenarioExecute"

   Purpose:  Gather and package data neccessary to setup the FAST_STREAM_INJECTION scenario, 
             then invoke RPC to implement the scenario in the WFPSampler service.               <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 FastStreamInjectionScenarioExecute(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                          _In_ const UINT32 stringCount)
{
   ASSERT(ppCLPStrings);
   ASSERT(stringCount);

   UINT32       status         = NO_ERROR;
   BOOLEAN      removeScenario = FALSE;
   FWPM_FILTER* pFilter        = 0;

   status = HlprFwpmFilterCreate(&pFilter);
   HLPR_BAIL_ON_FAILURE(status);

   pFilter->displayData.name = L"WFPSampler's Fast Stream Injection Scenario Filter";

   HlprCommandLineParseForScenarioRemoval(ppCLPStrings,
                                          stringCount,
                                          &removeScenario);

   status = HlprCommandLineParseForFilterInfo(ppCLPStrings,
                                              stringCount,
                                              pFilter,
                                              removeScenario);
   HLPR_BAIL_ON_FAILURE(status);

   if(pFilter->layerKey == FWPM_LAYER_STREAM_V4 ||
      pFilter->layerKey == FWPM_LAYER_STREAM_V6)
   {
      status = RPCInvokeScenarioFastStreamInjection(wfpSamplerBindingHandle,
                                                    SCENARIO_FAST_STREAM_INJECTION,
                                                    removeScenario ? FWPM_CHANGE_DELETE : FWPM_CHANGE_ADD,
                                                    pFilter);
      if(status != NO_ERROR)
         HlprLogError(L"FastStreamInjectionScenarioExecute : RpcInvokeScenarioFastStreamInjection() [status: %#x]",
                      status);
      else
         HlprLogInfo(L"FastStreamInjectionScenarioExecute : RpcInvokeScenarioFastStreamInjection() [status: %#x]",
                     status);
   }
   else
   {
      status = (UINT32)FWP_E_INCOMPATIBLE_LAYER;

      HlprLogError(L"FastStreamInjectionScenarioExecute() [status: %#x]",
                   status);
   }

   HLPR_BAIL_LABEL:

   if(pFilter)
      HlprFwpmFilterDestroy(&pFilter);

   return status;
}

/**
 @public_function="FastStreamInjectionScenarioLogHelp"
 
   Purpose:  Log usage information for the FAST_STREAM_INJECTION scenario to the console.       <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
VOID FastStreamInjectionScenarioLogHelp()
{
   wprintf(L"\n\t\t -s     \t FAST_STREAM_INJECTION");
   wprintf(L"\n\t\t -?     \t Receive usage information.");
   wprintf(L"\n\t\t -l     \t Specify the layer to perform the filtering. [Required]");
   wprintf(L"\n\t\t -sl    \t Specify the sublayer to perform the filtering. [Optional]");
   wprintf(L"\n\t\t -r     \t Remove the scenario objects.");
   wprintf(L"\n\t\t -v     \t Make the filter volatile (non-persistent). [Optional]");
   wprintf(L"\n\t\t -b     \t Makes the objects available during boot time. [Optional]");
   wprintf(L"\n\t\t -ipla  \t Specify the IP_LOCAL_ADDRESS /");
   wprintf(L"\n\t\t        \t    IP_SOURCE_ADDRESS to filter. [Optional]");
   wprintf(L"\n\t\t -ipra  \t Specify the IP_REMOTE_ADDRESS /");
   wprintf(L"\n\t\t        \t    IP_DESTINATION_ADDRESS to filter. [Optional]");
   wprintf(L"\n\t\t -iplp  \t Specify the IP_LOCAL_PORT to filter. [Optional]");
   wprintf(L"\n\t\t -iprp  \t Specify the IP_REMOTE_PORT to filter. [Optional]");
   wprintf(L"\n");
   wprintf(L"\n\t i.e.");
   wprintf(L"\n\t\t  WFPSampler.Exe -s FAST_STREAM_INJECTION -l FWPM_LAYER_STREAM_V4 -ipla 1.0.0.1 -ipra 1.0.0.254 -iprp 80 -v");
   wprintf(L"\n\t\t  WFPSampler.Exe -s FAST_STREAM_INJECTION -l FWPM_LAYER_STREAM_V4 -ipla 1.0.0.1 -ipra 1.0.0.254 -iprp 80 -v -r");
   wprintf(L"\n");

   return;
}
