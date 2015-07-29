////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      Scenarios_FlowAssociation.cpp
//
//   Abstract:
//      This module contains functions which prepares and sends data for the FLOW_ASSOCIATION
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
//            FlowAssociationScenario      - Function pertains to Flow Association Scenario.
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
//      FlowAssociationScenarioExecute(),
//      FlowAssociationScenarioLogHelp(),
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

/**
 @private_function="PrvFlowAssociationScenarioParseFlowAsociationData"
 
   Purpose:  Parse the command line parameters for implementing flow association such as:       <br>
                Associate context with the scenario callout for the layer (-aws)                <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 PrvFlowAssociationScenarioParseFlowAsociationData(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                                         _In_ const UINT32 stringCount,
                                                         _Inout_ PC_FLOW_ASSOCIATION_DATA* pPCFlowAssociationData)
{
   ASSERT(ppCLPStrings);
   ASSERT(stringCount);
   ASSERT(pPCFlowAssociationData);

   UINT32        status       = NO_ERROR;
   HANDLE        engineHandle = 0;
   FWPM_CALLOUT* pCallout     = 0;
   GUID          calloutKey   = {0};

   for(UINT32 stringIndex = 0;
       stringIndex < stringCount;
       stringIndex++)
   {
#pragma warning(push)
#pragma warning(disable: 6385) /// careful validation of stringIndex (and advancement) against stringCount prevents read overrun

      if((stringIndex + 1) < stringCount)
      {
         /// Associate with callout
         if(HlprStringsAreEqual(L"-aws",
                                 ppCLPStrings[stringIndex]))
         {
            PCWSTR pString = ppCLPStrings[++stringIndex];

            if(HlprStringsAreEqual(L"BASIC_PACKET_INJECTION",
                                   pString))
               calloutKey = WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION;
            else if(HlprStringsAreEqual(L"BASIC_PACKET_MODIFICATION",
                                        pString))
               calloutKey = WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION;
            else if(HlprStringsAreEqual(L"BASIC_STREAM_INJECTION",
                                        pString))
               calloutKey = WFPSAMPLER_CALLOUT_BASIC_STREAM_INJECTION;
            else if(HlprStringsAreEqual(L"PEND_ENDPOINT_CLOSURE",
                                        pString))
               calloutKey = WFPSAMPLER_CALLOUT_PEND_ENDPOINT_CLOSURE;

            break;
         }
      }

#pragma warning(pop)

   }

   if(HlprGUIDIsNull(&calloutKey))
   {
      status = (UINT32)FWP_E_INCOMPATIBLE_LAYER;

      HlprLogError(L"PrvFlowAssociationScenarioParseFlowAsociationData() [status: %#x]",
                   status);

      HLPR_BAIL;
   }

   status = HlprFwpmEngineOpen(&engineHandle);
   HLPR_BAIL_ON_FAILURE(status);

   for(UINT32 stringIndex = 0;
       stringIndex < stringCount;
       stringIndex++)
   {
      if((stringIndex + 1) < stringCount)
      {
         /// Associate with layer
         if(HlprStringsAreEqual(L"-awl",
                                 ppCLPStrings[stringIndex]))
         {
            for(UINT32 index = 0;
                index < PCFA_MAX_COUNT;
                index++)
            {
               stringIndex++;
                
               if(stringIndex < stringCount)
               {
                  PCWSTR pString = ppCLPStrings[stringIndex];
                  UINT8  layerID = HlprFwpmLayerGetIDByString(pString);

                  if(layerID == FWPS_BUILTIN_LAYER_MAX)
                     break;
                  else
                  {
                     GUID queryCalloutKey = calloutKey;

#if(NTDDI_VERSION >= NTDDI_WIN7)

                     if(layerID == FWPS_LAYER_ALE_ENDPOINT_CLOSURE_V4 ||
                        layerID == FWPS_LAYER_ALE_ENDPOINT_CLOSURE_V6)
                        queryCalloutKey = WFPSAMPLER_CALLOUT_PEND_ENDPOINT_CLOSURE;

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

                     queryCalloutKey.Data4[7] = layerID;

                     status = FwpmCalloutGetByKey(engineHandle,
                                                  &queryCalloutKey,
                                                  &pCallout);
                     if(status != NO_ERROR)
                     {
                        if(status == FWP_E_CALLOUT_NOT_FOUND)
                           HlprLogError(L"Must add scenario filter and callout first");

                        HlprLogError(L"PrvFlowAssociationScenarioParseFlowAsociationData() [status: %#x]",
                                     status);

                        HLPR_BAIL;
                     }

                     if(pCallout)
                     {
                        pPCFlowAssociationData->itemCount++;
                        pPCFlowAssociationData->pLayerIDs[index]   = layerID;
                        pPCFlowAssociationData->pCalloutIDs[index] = pCallout->calloutId;

                        FwpmFreeMemory((VOID**)&pCallout);
                     }
                  }
               }
            }
         }
      }
   }

   if(pPCFlowAssociationData->itemCount == 0)
   {
      status = ERROR_INVALID_DATA;

      HlprLogError(L"PrvFlowAssociationScenarioParseFlowAsociationData() [status: %#x]",
                   status);
   }

   HLPR_BAIL_LABEL:

   if(engineHandle)
      HlprFwpmEngineClose(&engineHandle);

   return status;
}

/**
 @scenario_function="FlowAssociationScenarioExecute"

   Purpose:  Gather and package data neccessary to setup the FLOW_ASSOCIATION scenario, 
             then invoke RPC to implement the scenario in the WFPSampler service.               <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 FlowAssociationScenarioExecute(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                      _In_ const UINT32 stringCount)
{
   ASSERT(ppCLPStrings);
   ASSERT(stringCount);

   UINT32                    status                 = NO_ERROR;
   BOOLEAN                   removeScenario         = FALSE;
   FWPM_FILTER*              pFilter                = 0;
   PC_FLOW_ASSOCIATION_DATA* pPCFlowAssociationData = 0;

   status = HlprFwpmFilterCreate(&pFilter);
   HLPR_BAIL_ON_FAILURE(status);

   pFilter->displayData.name = L"WFPSampler's Flow Association Scenario Filter";

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
      HLPR_NEW(pPCFlowAssociationData,
               PC_FLOW_ASSOCIATION_DATA);
      HLPR_BAIL_ON_ALLOC_FAILURE(pPCFlowAssociationData,
                                 status);

      status = PrvFlowAssociationScenarioParseFlowAsociationData(ppCLPStrings,
                                                                 stringCount,
                                                                 pPCFlowAssociationData);
      HLPR_BAIL_ON_FAILURE(status);
   }

   status = RPCInvokeScenarioFlowAssociation(wfpSamplerBindingHandle,
                                             SCENARIO_FLOW_ASSOCIATION,
                                             removeScenario ? FWPM_CHANGE_DELETE : FWPM_CHANGE_ADD,
                                             pFilter,
                                             pPCFlowAssociationData);
   if(status != NO_ERROR)
      HlprLogError(L"FlowAssociationScenarioExecute : RPCInvokeScenarioFlowAssociation() [status: %#x]",
                   status);
   else
      HlprLogInfo(L"FlowAssociationScenarioExecute : RPCInvokeScenarioFlowAssociation() [status: %#x]",
                  status);

   HLPR_BAIL_LABEL:

   if(pFilter)
      HlprFwpmFilterDestroy(&pFilter);

   HLPR_DELETE(pPCFlowAssociationData);

   return status;
}

/**
 @public_function="FlowAssociationScenarioLogHelp"
 
   Purpose:  Log usage information for the FLOW_ASSOCIATION scenario to the console.            <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
VOID FlowAssociationScenarioLogHelp()
{
   wprintf(L"\n\t\t -s     \t FLOW_ASSOCIATION");
   wprintf(L"\n\t\t -?     \t Receive usage information.");
   wprintf(L"\n\t\t -l     \t Specify the flow established layer from which the context will be associated. [Required]");
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
   wprintf(L"\n\t\t -aws   \t Specify the SCENARIO to associate the context with. [Required]");
   wprintf(L"\n\t\t -awl   \t Specify the layers to associate context with. [Required]");
   wprintf(L"\n");
   wprintf(L"\n\t i.e.");
   wprintf(L"\n\t\t  WFPSampler.Exe -s FLOW_ASSOCIATION -l FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4 -ipla 1.0.0.1 -ipra 1.0.0.254 -iprp 80 -aws BASIC_STREAM_INJECTION -awl FWPM_LAYER_STREAM_V4 FWPM_LAYER_ALE_ENDPOINT_CLOSURE_V4 -v");
   wprintf(L"\n\t\t  WFPSampler.Exe -s FLOW_ASSOCIATION -l FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4 -ipla 1.0.0.1 -ipra 1.0.0.254 -iprp 80 -aws BASIC_STREAM_INJECTION -awl FWPM_LAYER_STREAM_V4 FWPM_LAYER_ALE_ENDPOINT_CLOSURE_V4 -v -r");
   wprintf(L"\n");

   return;
}
