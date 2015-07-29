////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      Scenarios_BasicPacketInjection.cpp
//
//   Abstract:
//      This module contains functions which prepares and sends data for the BASIC_PACKET_INJECTION
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
//            BasicPacketInjectionScenario - Function pertains to Basic Packet Injection Scenario.
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
//            InjectionData                - Function acts on the PC_BASIC_PACKET_INJECTION_DATA.
//          }
//
//   Private Functions:
//      PrvScenarioBasicPacketInjectionParseInjectionData(),
//
//   Public Functions:
//      BasicPacketInjectionScenarioExecute(),
//      BasicPacketInjectionScenarioLogHelp(),
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
 @private_function="PrvBasicPacketInjectionScenarioParseInjectionData"
 
   Purpose:  Parse the command line parameters for implementing packet injection such as:       <br>
                Perform the injection inline (from within the classify)    (-in)                <br>
                Use threaded DPCs for out of band (asynchronous)           (-tdpc)              <br>
                Use work items for out of band (asynchronous)              (-wi)                <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 PrvBasicPacketInjectionScenarioParseInjectionData(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                                         _In_ const UINT32 stringCount,
                                                         _Inout_ PC_BASIC_PACKET_INJECTION_DATA* pPCBasicPacketInjectionData)
{
   ASSERT(ppCLPStrings);
   ASSERT(stringCount);
   ASSERT(pPCBasicPacketInjectionData);

   UINT32       status         = NO_ERROR;
   const UINT32 MAX_PARAMETERS = 4;
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
         pPCBasicPacketInjectionData->performInline = TRUE;

         found++;

         continue;
      }

      /// Threaded DPC
      if(HlprStringsAreEqual(L"-tdpc",
                             ppCLPStrings[stringIndex]))
      {
         pPCBasicPacketInjectionData->useThreadedDPC = TRUE;
      
         found++;
      
         continue;
      }

      /// Work Items
      if(HlprStringsAreEqual(L"-wi",
                             ppCLPStrings[stringIndex]))
      {
         pPCBasicPacketInjectionData->useWorkItems = TRUE;

         found++;

         continue;
      }
   }

   return status;
}

/**
 @scenario_function="BasicPacketInjectionScenarioExecute"

   Purpose:  Gather and package data neccessary to setup the BASIC_PACKET_INJECTION scenario, 
             then invoke RPC to implement the scenario in the WFPSampler service.               <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 BasicPacketInjectionScenarioExecute(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                           _In_ const UINT32 stringCount)
{
   ASSERT(ppCLPStrings);
   ASSERT(stringCount);

   UINT32                          status                      = NO_ERROR;
   BOOLEAN                         removeScenario              = FALSE;
   PC_BASIC_PACKET_INJECTION_DATA* pPCBasicPacketInjectionData = 0;
   FWPM_FILTER*                    pFilter                     = 0;

   status = HlprFwpmFilterCreate(&pFilter);
   HLPR_BAIL_ON_FAILURE(status);

   pFilter->displayData.name = L"WFPSampler's Basic Packet Injection Scenario Filter";

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
      HLPR_NEW(pPCBasicPacketInjectionData,
               PC_BASIC_PACKET_INJECTION_DATA);
      HLPR_BAIL_ON_ALLOC_FAILURE(pPCBasicPacketInjectionData,
                                 status);
      
      status = PrvBasicPacketInjectionScenarioParseInjectionData(ppCLPStrings,
                                                                 stringCount,
                                                                 pPCBasicPacketInjectionData);
      HLPR_BAIL_ON_FAILURE(status);
   }

   status = RPCInvokeScenarioBasicPacketInjection(wfpSamplerBindingHandle,
                                                  SCENARIO_BASIC_PACKET_INJECTION,
                                                  removeScenario ? FWPM_CHANGE_DELETE : FWPM_CHANGE_ADD,
                                                  pFilter,
                                                  pPCBasicPacketInjectionData);
   if(status != NO_ERROR)
      HlprLogError(L"BasicPacketInjectionScenarioExecute : RpcInvokeScenarioBasicPacketInjection() [status: %#x]",
                   status);
   else
      HlprLogInfo(L"BasicPacketInjectionScenarioExecute : RpcInvokeScenarioBasicPacketInjection() [status: %#x]",
                  status);

   HLPR_BAIL_LABEL:

   if(pFilter)
      HlprFwpmFilterDestroy(&pFilter);

   HLPR_DELETE(pPCBasicPacketInjectionData);

   return status;
}

/**
 @public_function="BasicPacketInjectionScenarioLogHelp"
 
   Purpose:  Log usage information for the BASIC_PACKET_INJECTION scenario to the console.      <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
VOID BasicPacketInjectionScenarioLogHelp()
{
   wprintf(L"\n\t\t -s     \t BASIC_PACKET_INJECTION");
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
   wprintf(L"\n\t\t -ipp   \t Specify the IP_PROTOCOL to filter. [Optional]");
   wprintf(L"\n\t\t -iplp  \t Specify the IP_LOCAL_PORT to filter. [Optional]");
   wprintf(L"\n\t\t -icmpt \t Specify the ICMP_TYPE to filter. [Optional]");
   wprintf(L"\n\t\t -iprp  \t Specify the IP_REMOTE_PORT to filter. [Optional]");
   wprintf(L"\n\t\t -icmpc \t Specify the ICMP_CODE to filter. [Optional]");
   wprintf(L"\n");
   wprintf(L"\n\t i.e.");
   wprintf(L"\n\t\t  WFPSampler.Exe -s BASIC_PACKET_INJECTION -l FWPM_LAYER_INBOUND_TRANSPORT_V4 -ipla 1.0.0.1 -ipra 1.0.0.254 -ipp TCP -v");
   wprintf(L"\n\t\t  WFPSampler.Exe -s BASIC_PACKET_INJECTION -l FWPM_LAYER_INBOUND_TRANSPORT_V4 -ipla 1.0.0.1 -ipra 1.0.0.254 -ipp TCP -v -r");
   wprintf(L"\n");

   return;
}
