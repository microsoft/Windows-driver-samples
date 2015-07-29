////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      Scenarios_BasicPacketExamination.cpp
//
//   Abstract:
//      This module contains functions which prepares and sends data for the 
//         BASIC_PACKET_EXAMINATION scenario implementation.
//
//   Naming Convention:
//
//      <Scope><Object><Action><Modifier>
//  
//      i.e.
//
//       <Scope>
//          {
//                                           - Function is likely visible to other modules.
//            Prv                            - Function is private to this module.
//          }
//       <Object>
//          {
//            BasicPacketExaminationScenario - Function pertains to the Basic Packet Examination.
//          }
//       <Action>
//          {
//            Execute                        - Function packages data and invokes RPC to the 
//                                                WFPSampler service.
//            Log                            - Function writes to the console.
//          }
//       <Modifier>
//          {
//            Help                           - Function provides context sensitive help for the 
//                                                scenario.
//            ExaminationFlags               - Function acts on the .
//          }
//

//   Private Functions:
//      PrvScenarioBasicPacketExaminationParseExaminationFlags(),
//
//   Public Functions:
//      BasicPacketExaminationScenarioExecute(),
//      BasicPacketExaminationScenarioLogHelp(),
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Add support for specifying a different sublayer and
//                                              for examination options
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Framework_WFPSampler.h" /// .

/**
 @private_function="PrvScenarioBasicPacketExaminationParseExaminationFlags"
 
   Purpose:  Parse the command line parameters for implementing packet examination such as:     <br>
                Log while under lock                            (-lul)                          <br>
                Log the FWPS_INCOMING_VALUES                    (-liv)                          <br>
                Log the FWPS_INCOMING_METADATA_VALUES           (-limv)                         <br>
                Log the layerData pointer                       (-lld)                          <br>
                Log the classifyContext                         (-lcc)                          <br>
                Log the FWPS_FILTER                             (-lf)                           <br>
                Log the flowContext                             (-lfc)                          <br>
                Log the FWPS_CLASSIFY_OUT                       (-lco)                          <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 PrvScenarioBasicPacketExaminationParseExaminationFlags(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                                              _In_ const UINT32 stringCount,
                                                              _Inout_ UINT64* pExaminationFlags)
{
   ASSERT(ppCLPStrings);
   ASSERT(stringCount);
   ASSERT(pExaminationFlags);

   UINT32       status         = NO_ERROR;
   const UINT32 MAX_PARAMETERS = 7;
   UINT32       found          = 0;

   *pExaminationFlags = 0;

   for(UINT32 stringIndex = 0;
       stringIndex < stringCount &&
       found != MAX_PARAMETERS;
       stringIndex++)
   {
      /// Log while under lock
      if(HlprStringsAreEqual(L"-lul",
                             ppCLPStrings[stringIndex]))
      {
         (*pExaminationFlags) |= PCPEF_EXAMINE_UNDER_LOCK;

         found++;

         continue;
      }

      /// Log FWPS_INCOMING_VALUES
      if(HlprStringsAreEqual(L"-liv",
                             ppCLPStrings[stringIndex]))
      {
         (*pExaminationFlags) |= PCPEF_EXAMINE_INCOMING_VALUES;

         found++;

         continue;
      }

      /// Log FWPS_INCOMING_METADATA_VALUES
      if(HlprStringsAreEqual(L"-limv",
                             ppCLPStrings[stringIndex]))
      {
         (*pExaminationFlags) |= PCPEF_EXAMINE_INCOMING_METADATA_VALUES;

         found++;

         continue;
      }

      /// Log layerData pointer
      if(HlprStringsAreEqual(L"-lld",
                             ppCLPStrings[stringIndex]))
      {
         (*pExaminationFlags) |= PCPEF_EXAMINE_LAYER_DATA;

         found++;

         continue;
      }

      /// Log classifyContext
      if(HlprStringsAreEqual(L"-lcc",
                             ppCLPStrings[stringIndex]))
      {
         (*pExaminationFlags) |= PCPEF_EXAMINE_CLASSIFY_CONTEXT;

         found++;

         continue;
      }

      /// Log FWPS_FILTER
      if(HlprStringsAreEqual(L"-lf",
                             ppCLPStrings[stringIndex]))
      {
         (*pExaminationFlags) |= PCPEF_EXAMINE_FILTER;

         found++;

         continue;
      }


      /// Log flowContext
      if(HlprStringsAreEqual(L"-lfc",
                             ppCLPStrings[stringIndex]))
      {
         (*pExaminationFlags) |= PCPEF_EXAMINE_FLOW_CONTEXT;

         found++;

         continue;
      }

      /// Log FWPS_CLASSIFY_OUT
      if(HlprStringsAreEqual(L"-lco",
                             ppCLPStrings[stringIndex]))
      {
         (*pExaminationFlags) |= PCPEF_EXAMINE_CLASSIFY_OUT;

         found++;

         continue;
      }
   }

   return status;
}

/**
 @scenario_function="BasicPacketExaminationScenarioExecute"

   Purpose:  Gather and package data neccessary to setup the BASIC_PACKET_EXAMINATION scenario, 
             then invoke RPC to implement the scenario in the WFPSampler service.               <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 BasicPacketExaminationScenarioExecute(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                             _In_ UINT32 stringCount)
{
   ASSERT(ppCLPStrings);
   ASSERT(stringCount);

   UINT32       status           = NO_ERROR;
   BOOLEAN      removeScenario   = FALSE;
   FWPM_FILTER* pFilter          = 0;

   status = HlprFwpmFilterCreate(&pFilter);
   HLPR_BAIL_ON_FAILURE(status);

   pFilter->displayData.name = L"WFPSampler's Basic Packet Examination Scenario Filter";

   HlprCommandLineParseForScenarioRemoval(ppCLPStrings,
                                          stringCount,
                                          &removeScenario);

   status = HlprCommandLineParseForFilterInfo(ppCLPStrings,
                                              stringCount,
                                              pFilter,
                                              removeScenario);
   HLPR_BAIL_ON_FAILURE(status);

   PrvScenarioBasicPacketExaminationParseExaminationFlags(ppCLPStrings,
                                                          stringCount,
                                                          &(pFilter->rawContext));

   status = RPCInvokeScenarioBasicPacketExamination(wfpSamplerBindingHandle,
                                                    SCENARIO_BASIC_PACKET_EXAMINATION,
                                                    removeScenario ? FWPM_CHANGE_DELETE : FWPM_CHANGE_ADD,
                                                    pFilter);
   if(status != NO_ERROR)
      HlprLogError(L"BasicPacketExaminationScenarioExecute : RpcInvokeScenarioBasicPacketExamination() [status: %#x]",
                   status);
   else
      HlprLogInfo(L"BasicPacketExaminationScenarioExecute : RpcInvokeScenarioBasicPacketExamination() [status: %#x]",
                  status);

   HLPR_BAIL_LABEL:

   if(pFilter)
      HlprFwpmFilterDestroy(&pFilter);

   return status;
}

/**
 @public_function="BasicPacketExaminationScenarioLogHelp"
 
   Purpose:  Log usage information for the BASIC_PACKET_EXAMINATION scenario to the console.    <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
VOID BasicPacketExaminationScenarioLogHelp()
{
   wprintf(L"\n\t\t -s     \t BASIC_PACKET_EXAMINATION");
   wprintf(L"\n\t\t -?     \t Receive usage information.");
   wprintf(L"\n\t\t -l     \t Specify the layer to perform the filtering. [Required]");
   wprintf(L"\n\t\t -r     \t Remove the scenario objects.");
   wprintf(L"\n\t\t -v     \t Make the filter volatile (non-persistent). [Optional]");
   wprintf(L"\n\t\t -b     \t Makes the objects available during boot time. [Optional]");
   wprintf(L"\n\t\t -lul   \t log while under lock");
   wprintf(L"\n\t\t -liv   \t log the FWPS_INCOMING_VALUES");
   wprintf(L"\n\t\t -limv  \t log the FWPS_INCOMING_METADATA_VALUES");
   wprintf(L"\n\t\t -lld   \t log the layerData");
   wprintf(L"\n\t\t -lcc   \t log the classifyContext");
   wprintf(L"\n\t\t -lf    \t log the FWPS_FILTER");
   wprintf(L"\n\t\t -lfc   \t log the flowContext");
   wprintf(L"\n\t\t -lco   \t log the FWPS_CLASSIFY_OUT");
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
   wprintf(L"\n\t\t  WFPSampler.Exe -s BASIC_PACKET_EXAMINATION -l FWPM_LAYER_INBOUND_TRANSPORT_V4 -ipla 1.0.0.1 -ipra 1.0.0.254 -ipp TCP -v");
   wprintf(L"\n\t\t  WFPSampler.Exe -s BASIC_PACKET_EXAMINATION -l FWPM_LAYER_INBOUND_TRANSPORT_V4 -ipla 1.0.0.1 -ipra 1.0.0.254 -ipp TCP -v -r");
   wprintf(L"\n");

   return;
}
