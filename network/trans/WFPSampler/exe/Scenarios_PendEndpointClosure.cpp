////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      Scenarios_PendEndpointClosure.cpp
//
//   Abstract:
//      This module contains functions which prepares and sends data for the PEND_ENDPOINT_CLOSURE
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
//                                        - Function is likely visible to other modules.
//            Prv                         - Function is private to this module.
//          }
//       <Object>
//          {
//            PendEndpointClosureScenario - Function pertains to all of the Pend Endpoint Closure
//                                           Scenarios.
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
//            ClosureData                 - Function acts on the PC_ENDPOINT_CLOSURE_DATA.
//            Help                        - Function provides usage information.
//          }
//
//   Private Functions:
//      PrvPendEndpointClosureScenarioParseClosureData(),
//
//   Public Functions:
//      PendEndpointClosureScenarioExecute(),
//      PendEndpointClosureScenarioLogHelp(),
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      December  13,   2013  -     1.1   -  Creation
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Framework_WFPSampler.h" /// .

#if(NTDDI_VERSION >= NTDDI_WIN7)

/**
 @private_function="PrvPendEndpointClosureScenarioParseClosureData"
 
   Purpose:  Parse the command line parameters for closure data such as:                        <br>
                delay before callout returns (-p DELAY_IN_MS)                                   <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 PrvPendEndpointClosureScenarioParseClosureData(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                                      _In_ const UINT32 stringCount,
                                                      _Inout_ PC_PEND_ENDPOINT_CLOSURE_DATA* pPCPendEndpointClosureData)
{
   ASSERT(ppCLPStrings);
   ASSERT(stringCount);
   ASSERT(pPCPendEndpointClosureData);

   UINT32       status         = NO_ERROR;
   const UINT32 MAX_PARAMETERS = 5;
   UINT32       found          = 0;

   for(UINT32 stringIndex = 0;
       stringIndex < stringCount &&
         found != MAX_PARAMETERS;
       stringIndex++)
   {
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

               pPCPendEndpointClosureData->delay = delay;

               found++;
            }

            continue;
         }
      }

      /// Threaded DPC
      if(HlprStringsAreEqual(L"-tdpc",
                             ppCLPStrings[stringIndex]))
      {
         pPCPendEndpointClosureData->useThreadedDPC = TRUE;
      
         found++;
      
         continue;
      }

      /// Work Items
      if(HlprStringsAreEqual(L"-wi",
                             ppCLPStrings[stringIndex]))
      {
         pPCPendEndpointClosureData->useWorkItems = TRUE;

         found++;

         continue;
      }
   }

   return status;
}

/**
 @scenario_function="PendEndpointClosureScenarioExecute"

   Purpose:  Gather and package data neccessary to setup the PEND_ENDPOINT_CLOSURE scenario, 
             then invoke RPC to implement the scenario in the WFPSampler service.               <br>
                                                                                                <br>
   Notes:    This scenario is only supported on Win7+.                                          <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 PendEndpointClosureScenarioExecute(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                          _In_ UINT32 stringCount)
{
   ASSERT(ppCLPStrings);
   ASSERT(stringCount);

   UINT32                         status                     = NO_ERROR;
   BOOLEAN                        removeScenario             = FALSE;
   PC_PEND_ENDPOINT_CLOSURE_DATA* pPCPendEndpointClosureData = 0;
   FWPM_FILTER*                   pFilter                    = 0;

#if(NTDDI_VERSION >= NTDDI_WIN7)

   status = HlprFwpmFilterCreate(&pFilter);
   HLPR_BAIL_ON_FAILURE(status);

   pFilter->displayData.name = L"WFPSampler's Pend Endpoint Closure Scenario Filter";

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
      HLPR_NEW(pPCPendEndpointClosureData,
               PC_PEND_ENDPOINT_CLOSURE_DATA);
      HLPR_BAIL_ON_ALLOC_FAILURE(pPCPendEndpointClosureData,
                                 status);

      status = PrvPendEndpointClosureScenarioParseClosureData(ppCLPStrings,
                                                              stringCount,
                                                              pPCPendEndpointClosureData);
      HLPR_BAIL_ON_FAILURE(status);
   }

   status = RPCInvokeScenarioPendEndpointClosure(wfpSamplerBindingHandle,
                                                 SCENARIO_PEND_ENDPOINT_CLOSURE,
                                                 removeScenario ? FWPM_CHANGE_DELETE : FWPM_CHANGE_ADD,
                                                 pFilter,
                                                 pPCPendEndpointClosureData);
   if(status != NO_ERROR)
      HlprLogError(L"PendEndpointClosureScenarioExecute : RpcInvokeScenarioPendEndpointClosure() [status: %#x]",
                   status);
   else
      HlprLogInfo(L"PendEndpointClosureScenarioExecute : RpcInvokeScenarioPendEndpointClosure() [status: %#x]",
                  status);

   HLPR_BAIL_LABEL:

   if(pFilter)
      HlprFwpmFilterDestroy(&pFilter);

   HLPR_DELETE(pPCPendEndpointClosureData);

#else

   UNREFERENCED_PARAMETER(ppCLPStrings);
   UNREFERENCED_PARAMETER(stringCount);

   status = ERROR_NOT_SUPPORTED;

   HlprLogError(L"PendEndpointClosureScenarioExecute() [status: %#x]",
                status);

#endif // (NTDDI_VERSION >= NTDDI_WIN7)

   return status;
}

/**
 @public_function="PendEndpointClosureScenarioLogHelp"
 
   Purpose:  Log usage information for the PEND_ENDPOINT_CLOSURE scenario to the console.       <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
VOID PendEndpointClosureScenarioLogHelp()
{
   wprintf(L"\n\t\t -s     \t PEND_ENDPOINT_CLOSURE");
   wprintf(L"\n\t\t -?     \t Receive usage information.");
   wprintf(L"\n\t\t -l     \t Specify the layer to perform the filtering. [Required]");
   wprintf(L"\n\t\t -sl    \t Specify the sublayer to perform the filtering. [Optional]");
   wprintf(L"\n\t\t -r     \t Remove the scenario objects.");
   wprintf(L"\n\t\t -v     \t Make the filter volatile (non-persistent). [Optional]");
   wprintf(L"\n\t\t -b     \t Makes the objects available during boot time. [Optional]");
   wprintf(L"\n\t\t -tdpc  \t Use threaded DPCs for out of band. [Optional]");
   wprintf(L"\n\t\t -wi    \t Use work items for out of band. [Optional]");
   wprintf(L"\n\t\t -pcd   \t Introduce a delay (in ms) before the pend is completed. [Optional]");
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
   wprintf(L"\n\t\t  WFPSampler.Exe -s PEND_ENDPOINT_CLOSURE -l FWPM_LAYER_ALE_ENDPOINT_CLOSURE_V4 -ipla 1.0.0.1 -ipra 1.0.0.254 -ipp TCP -d 10000 -v");
   wprintf(L"\n\t\t  WFPSampler.Exe -s PEND_ENDPOINT_CLOSURE -l FWPM_LAYER_ALE_ENDPOINT_CLOSURE_V4 -ipla 1.0.0.1 -ipra 1.0.0.254 -ipp TCP -d 10000 -v -r");
   wprintf(L"\n");

   return;
}

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)
