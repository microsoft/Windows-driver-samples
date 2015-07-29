////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      Scenarios_FastPacketInjection.cpp
//
//   Abstract:
//      This module contains functions which prepares and sends data for the FAST_PACKET_INJECTION
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
//            FastPacketInjectionScenario  - Function pertains to Fast Packet Injection Scenario.
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
//      FastPacketInjectionScenarioExecute(),
//      FastPacketInjectionScenarioLogHelp(),
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
 @scenario_function="FastPacketInjectionScenarioExecute"

   Purpose:  Gather and package data neccessary to setup the FAST_PACKET_INJECTION scenario, 
             then invoke RPC to implement the scenario in the WFPSampler service.               <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 FastPacketInjectionScenarioExecute(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                          _In_ const UINT32 stringCount)
{
   ASSERT(ppCLPStrings);
   ASSERT(stringCount);

   UINT32       status         = NO_ERROR;
   BOOLEAN      removeScenario = FALSE;
   FWPM_FILTER* pFilter        = 0;

   status = HlprFwpmFilterCreate(&pFilter);
   HLPR_BAIL_ON_FAILURE(status);

   pFilter->displayData.name = L"WFPSampler's Fast Packet Injection Scenario Filter";

   HlprCommandLineParseForScenarioRemoval(ppCLPStrings,
                                          stringCount,
                                          &removeScenario);

   status = HlprCommandLineParseForFilterInfo(ppCLPStrings,
                                              stringCount,
                                              pFilter,
                                              removeScenario);
   HLPR_BAIL_ON_FAILURE(status);

   status = RPCInvokeScenarioFastPacketInjection(wfpSamplerBindingHandle,
                                                 SCENARIO_FAST_PACKET_INJECTION,
                                                 removeScenario ? FWPM_CHANGE_DELETE : FWPM_CHANGE_ADD,
                                                 pFilter);
   if(status != NO_ERROR)
      HlprLogError(L"FastPacketInjectionScenarioExecute : RpcInvokeScenarioFastPacketInjection() [status: %#x]",
                   status);
   else
      HlprLogInfo(L"FastPacketInjectionScenarioExecute : RpcInvokeScenarioFastPacketInjection() [status: %#x]",
                  status);

   HLPR_BAIL_LABEL:

   if(pFilter)
      HlprFwpmFilterDestroy(&pFilter);

   return status;
}

/**
 @public_function="FastPacketInjectionScenarioLogHelp"
 
   Purpose:  Log usage information for the FAST_PACKET_INJECTION scenario to the console.       <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
VOID FastPacketInjectionScenarioLogHelp()
{
   wprintf(L"\n\t\t -s     \t FAST_PACKET_INJECTION");
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
   wprintf(L"\n\t\t -ipp   \t Specify the IP_PROTOCOL to filter. [Optional]");
   wprintf(L"\n\t\t -iplp  \t Specify the IP_LOCAL_PORT to filter. [Optional]");
   wprintf(L"\n\t\t -icmpt \t Specify the ICMP_TYPE to filter. [Optional]");
   wprintf(L"\n\t\t -iprp  \t Specify the IP_REMOTE_PORT to filter. [Optional]");
   wprintf(L"\n\t\t -icmpc \t Specify the ICMP_CODE to filter. [Optional]");
   wprintf(L"\n");
   wprintf(L"\n\t i.e.");
   wprintf(L"\n\t\t  WFPSampler.Exe -s FAST_PACKET_INJECTION -l FWPM_LAYER_INBOUND_TRANSPORT_V4 -ipla 1.0.0.1 -ipra 1.0.0.254 -ipp TCP -v");
   wprintf(L"\n\t\t  WFPSampler.Exe -s FAST_PACKET_INJECTION -l FWPM_LAYER_INBOUND_TRANSPORT_V4 -ipla 1.0.0.1 -ipra 1.0.0.254 -ipp TCP -v -r");
   wprintf(L"\n");

   return;
}
