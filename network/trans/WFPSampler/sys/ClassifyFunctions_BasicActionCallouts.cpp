////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      ClassifyFunctions_BasicActionCallouts.cpp
//
//   Abstract:
//      This module contains WFP Classify functions for returning the simple actions 
//         FWP_ACTION_BLOCK and FWP_ACTION_PERMIT.
//
//   Naming Convention:
//
//      <Module><Scenario><Action>
//  
//      i.e.
//
//       ClassifyBasicActionBlock
//
//       <Module>
//          Classify    - Function is an FWPS_CALLOUT_CLASSIFY_FN.
//       <Scenario>
//          BasicAction - Function demonstates use of the simple action types.
//       <Action>
//          {
//            Block     - Function returns FWP_ACTION_BLOCK, thus blocking the traffic.
//            Continue  - Function returns FWP_ACTION_CONTINUE thus allowing the traffic for another 
//                           filter (if any) to make a final decision.
//            Permit    - Function returns FWP_ACTION_PERMIT, thus allowing the traffic.
//            Random    - Function randomly returns either FWP_ACTION_BLOCK or FWP_ACTION_PERMIT.
//          }
//
//   Private Functions:
//      PerformBasicAction(),
//
//   Public Functions:
//      ClassifyBasicActionBlock(),
//      ClassifyBasicActionContinue(),
//      ClassifyBasicActionPermit(),
//      ClassifyBasicActionRandom(),
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Enhance function declaration for IntelliSense and
//                                              enhance traces
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Framework_WFPSamplerCalloutDriver.h"       /// .
#include "ClassifyFunctions_BasicActionCallouts.tmh" /// $(OBJ_PATH)\$(O)\ 

/**
 @private_function="PerformBasicAction"
 
   Purpose:  Sets the pClassifyOut->actionType to the basic action specified.                   <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF551229.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID PerformBasicAction(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                        _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                        _Inout_opt_ VOID* pLayerData,
                        _In_opt_ const VOID* pClassifyContext,
                        _In_ const FWPS_FILTER* pFilter,
                        _In_ UINT64 flowContext,
                        _Inout_ FWPS_CLASSIFY_OUT* pClassifyOut,
                        _In_ FWP_ACTION_TYPE basicAction)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> PerformBasicAction()\n");
#endif /// DBG


   NT_ASSERT(pClassifyValues);
   NT_ASSERT(pMetadata);
   NT_ASSERT(pFilter);
   NT_ASSERT(pClassifyOut);

   UNREFERENCED_PARAMETER(pMetadata);
   UNREFERENCED_PARAMETER(pClassifyContext);
   UNREFERENCED_PARAMETER(pFilter);
   UNREFERENCED_PARAMETER(flowContext);

   if(pClassifyValues->layerId == FWPS_LAYER_STREAM_V4 ||
      pClassifyValues->layerId == FWPS_LAYER_STREAM_V6)
   {
      FWPS_STREAM_CALLOUT_IO_PACKET0* pIOPacket = (FWPS_STREAM_CALLOUT_IO_PACKET*)pLayerData;

      if(pIOPacket)
      {
         pIOPacket->streamAction       = FWPS_STREAM_ACTION_NONE;
         pIOPacket->countBytesEnforced = pIOPacket->streamData->dataLength;
      }
   }

   if(pClassifyOut)
   {
      pClassifyOut->actionType  = basicAction;

      /// Clear the right to mark as the definitive answer.
      if(basicAction == FWP_ACTION_BLOCK ||
         (basicAction == FWP_ACTION_PERMIT &&
         pFilter->flags & FWPS_FILTER_FLAG_CLEAR_ACTION_RIGHT))
         pClassifyOut->rights ^= FWPS_RIGHT_ACTION_WRITE;
   }

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- PerformBasicAction()\n");

#endif /// DBG

   return;
}

#if(NTDDI_VERSION >= NTDDI_WIN7)

/**
 @classify_function="ClassifyBasicActionBlock"
 
   Purpose:  Sets the pClassifyOut->actionType to FWP_ACTION_BLOCK.                             <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF551229.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF544893.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI ClassifyBasicActionBlock(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                    _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                    _Inout_opt_ VOID* pLayerData,
                                    _In_opt_ const VOID* pClassifyContext,
                                    _In_ const FWPS_FILTER* pFilter,
                                    _In_ UINT64 flowContext,
                                    _Inout_ FWPS_CLASSIFY_OUT* pClassifyOut)
{
   NT_ASSERT(pClassifyValues);
   NT_ASSERT(pMetadata);
   NT_ASSERT(pFilter);
   NT_ASSERT(pClassifyOut);

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> ClassifyBasicActionBlock() [Layer: %s][FilterID: %#I64x][Rights: %#x]",
              KrnlHlprFwpsLayerIDToString(pClassifyValues->layerId),
              pFilter->filterId,
              pClassifyOut->rights);

   if(pClassifyOut->rights & FWPS_RIGHT_ACTION_WRITE)
   {
      PerformBasicAction(pClassifyValues,
                         pMetadata,
                         pLayerData,
                         pClassifyContext,
                         pFilter,
                         flowContext,
                         pClassifyOut,
                         FWP_ACTION_BLOCK);
   }

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- ClassifyBasicActionBlock() [Layer: %s][FilterID: %#I64x][Action: %#x][Rights: %#x][Absorb: %s]\n",
              KrnlHlprFwpsLayerIDToString(pClassifyValues->layerId),
              pFilter->filterId,
              pClassifyOut->actionType,
              pClassifyOut->rights,
              (pClassifyOut->flags & FWPS_CLASSIFY_OUT_FLAG_ABSORB) ? "TRUE" : "FALSE");

   return;
}

/**
 @classify_function="ClassifyBasicActionContinue"
 
   Purpose:  Sets the pClassifyOut->actionType to FWP_ACTION_CONTINUE.                          <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF551229.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF544893.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI ClassifyBasicActionContinue(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                       _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                       _Inout_opt_ VOID* pLayerData,
                                       _In_opt_ const VOID* pClassifyContext,
                                       _In_ const FWPS_FILTER* pFilter,
                                       _In_ UINT64 flowContext,
                                       _Inout_ FWPS_CLASSIFY_OUT* pClassifyOut)
{
   NT_ASSERT(pClassifyValues);
   NT_ASSERT(pMetadata);
   NT_ASSERT(pFilter);
   NT_ASSERT(pClassifyOut);

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> ClassifyBasicActionContinue() [Layer: %s][FilterID: %#I64x][Rights: %#x]",
              KrnlHlprFwpsLayerIDToString(pClassifyValues->layerId),
              pFilter->filterId,
              pClassifyOut->rights);

   if(pClassifyOut->rights & FWPS_RIGHT_ACTION_WRITE)
   {
      PerformBasicAction(pClassifyValues,
                         pMetadata,
                         pLayerData,
                         pClassifyContext,
                         pFilter,
                         flowContext,
                         pClassifyOut,
                         FWP_ACTION_CONTINUE);
   }

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- ClassifyBasicActionContinue() [Layer: %s][FilterID: %#I64x][Action: %#x][Rights: %#x][Absorb: %s]\n",
              KrnlHlprFwpsLayerIDToString(pClassifyValues->layerId),
              pFilter->filterId,
              pClassifyOut->actionType,
              pClassifyOut->rights,
              (pClassifyOut->flags & FWPS_CLASSIFY_OUT_FLAG_ABSORB) ? "TRUE" : "FALSE");

   return;
}

/**
 @classify_function="ClassifyBasicActionPermit"
 
   Purpose:  Sets the pClassifyOut->actionType to FWP_ACTION_PERMIT.                            <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF551229.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF544893.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI ClassifyBasicActionPermit(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                     _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                     _Inout_opt_ VOID* pLayerData,
                                     _In_opt_ const VOID* pClassifyContext,
                                     _In_ const FWPS_FILTER* pFilter,
                                     _In_ UINT64 flowContext,
                                     _Inout_ FWPS_CLASSIFY_OUT* pClassifyOut)
{
   NT_ASSERT(pClassifyValues);
   NT_ASSERT(pMetadata);
   NT_ASSERT(pFilter);
   NT_ASSERT(pClassifyOut);

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> ClassifyBasicActionPermit() [Layer: %s][FilterID: %#I64x][Rights: %#x]",
              KrnlHlprFwpsLayerIDToString(pClassifyValues->layerId),
              pFilter->filterId,
              pClassifyOut->rights);

   if(pClassifyOut->rights & FWPS_RIGHT_ACTION_WRITE)
   {
      PerformBasicAction(pClassifyValues,
                         pMetadata,
                         pLayerData,
                         pClassifyContext,
                         pFilter,
                         flowContext,
                         pClassifyOut,
                         FWP_ACTION_PERMIT);
   }

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- ClassifyBasicActionPermit() [Layer: %s][FilterID: %#I64x][Action: %#x][Rights: %#x][Absorb: %s]\n",
              KrnlHlprFwpsLayerIDToString(pClassifyValues->layerId),
              pFilter->filterId,
              pClassifyOut->actionType,
              pClassifyOut->rights,
              (pClassifyOut->flags & FWPS_CLASSIFY_OUT_FLAG_ABSORB) ? "TRUE" : "FALSE");

   return;
}

/**
 @classify_function="ClassifyBasicActionRandom"
 
   Purpose:  Sets the pClassifyOut->actionType to either FWP_ACTION_BLOCK or FWP_ACTION_PERMIT. <br>
                                                                                                <br>
   Notes:    Specifying -rab at the command line will allow one to determine how randomly 
                FWP_ACTION_BLOCK is returned.                                                   <br>
             Specifying -rac at the command line will allow one to determine how randomly 
                FWP_ACTION_CONTINUE is returned.                                                <br>
             Specifying -rap at the command line will allow one to determine how randomly 
                FWP_ACTION_PERMIT is returned.                                                  <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF551229.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF544893.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI ClassifyBasicActionRandom(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                     _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                     _Inout_opt_ VOID* pLayerData,
                                     _In_opt_ const VOID* pClassifyContext,
                                     _In_ const FWPS_FILTER* pFilter,
                                     _In_ UINT64 flowContext,
                                     _Inout_ FWPS_CLASSIFY_OUT* pClassifyOut)
{
   NT_ASSERT(pClassifyValues);
   NT_ASSERT(pMetadata);
   NT_ASSERT(pFilter);
   NT_ASSERT(pClassifyOut);

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> ClassifyBasicActionRandom() [Layer: %s][FilterID: %#I64x][Rights: %#x]",
              KrnlHlprFwpsLayerIDToString(pClassifyValues->layerId),
              pFilter->filterId,
              pClassifyOut->rights);

   if(pClassifyOut->rights & FWPS_RIGHT_ACTION_WRITE)
   {
      FWPM_PROVIDER_CONTEXT* pProviderContext = pFilter->providerContext;

      if(pProviderContext &&
         pProviderContext->type == FWPM_GENERAL_CONTEXT &&
         pProviderContext->dataBuffer &&
         pProviderContext->dataBuffer->data &&
         pProviderContext->dataBuffer->size == sizeof(PC_BASIC_ACTION_DATA))
      {
         PC_BASIC_ACTION_DATA* pPCBasicActionData = (PC_BASIC_ACTION_DATA*)(pProviderContext->dataBuffer->data);
         UINT32                randomizer         = 1;
         FWP_ACTION_TYPE       actionType         = FWP_ACTION_CONTINUE;
         UINT32                blockCap           = pPCBasicActionData->percentBlock;
         UINT32                permitCap          = pPCBasicActionData->percentPermit + blockCap;
         LARGE_INTEGER         largeInteger;

         RtlZeroMemory(&largeInteger,
                       sizeof(LARGE_INTEGER));

         KeQueryTickCount(&largeInteger);

         srand(largeInteger.u.LowPart);

         randomizer += (rand() % 100);

         if(randomizer <= blockCap)
            actionType = FWP_ACTION_BLOCK;
         else if(randomizer <= permitCap)
            actionType = FWP_ACTION_PERMIT;

         PerformBasicAction(pClassifyValues,
                            pMetadata,
                            pLayerData,
                            pClassifyContext,
                            pFilter,
                            flowContext,
                            pClassifyOut,
                            actionType);
      }
   }

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- ClassifyBasicActionRandom() [Layer: %s][FilterID: %#I64x][Action: %#x][Rights: %#x][Absorb: %s]\n",
              KrnlHlprFwpsLayerIDToString(pClassifyValues->layerId),
              pFilter->filterId,
              pClassifyOut->actionType,
              pClassifyOut->rights,
              (pClassifyOut->flags & FWPS_CLASSIFY_OUT_FLAG_ABSORB) ? "TRUE" : "FALSE");

   return;
}

#else

/**
 @classify_function="ClassifyBasicActionBlock"
 
   Purpose:  Sets the pClassifyOut->actionType to FWP_ACTION_BLOCK.                             <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF551229.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF544890.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI ClassifyBasicActionBlock(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                    _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                    _Inout_opt_ VOID* pLayerData,
                                    _In_ const FWPS_FILTER* pFilter,
                                    _In_ UINT64 flowContext,
                                    _Inout_ FWPS_CLASSIFY_OUT* pClassifyOut)
{
   NT_ASSERT(pClassifyValues);
   NT_ASSERT(pMetadata);
   NT_ASSERT(pFilter);
   NT_ASSERT(pClassifyOut);

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> ClassifyBasicActionBlock() [Layer: %s][FilterID: %#I64x][Rights: %#x]",
              KrnlHlprFwpsLayerIDToString(pClassifyValues->layerId),
              pFilter->filterId,
              pClassifyOut->rights);

   if(pClassifyOut->rights & FWPS_RIGHT_ACTION_WRITE)
   {
      PerformBasicAction(pClassifyValues,
                         pMetadata,
                         pLayerData,
                         0,
                         pFilter,
                         flowContext,
                         pClassifyOut,
                         FWP_ACTION_BLOCK);
   }

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- ClassifyBasicActionBlock() [Layer: %s][FilterID: %#I64x][Action: %#x][Rights: %#x][Absorb: %s]\n",
              KrnlHlprFwpsLayerIDToString(pClassifyValues->layerId),
              pFilter->filterId,
              pClassifyOut->actionType,
              pClassifyOut->rights,
              (pClassifyOut->flags & FWPS_CLASSIFY_OUT_FLAG_ABSORB) ? "TRUE" : "FALSE");

   return;
}

/**
 @classify_function="ClassifyBasicActionContinue"
 
   Purpose:  Sets the pClassifyOut->actionType to FWP_ACTION_CONTINUE.                          <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF551229.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF544890.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI ClassifyBasicActionContinue(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                       _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                       _Inout_opt_ VOID* pLayerData,
                                       _In_ const FWPS_FILTER* pFilter,
                                       _In_ UINT64 flowContext,
                                       _Inout_ FWPS_CLASSIFY_OUT* pClassifyOut)
{
   NT_ASSERT(pClassifyValues);
   NT_ASSERT(pMetadata);
   NT_ASSERT(pFilter);
   NT_ASSERT(pClassifyOut);

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> ClassifyBasicActionContinue() [Layer: %s][FilterID: %#I64x][Rights: %#x]",
              KrnlHlprFwpsLayerIDToString(pClassifyValues->layerId),
              pFilter->filterId,
              pClassifyOut->rights);

   if(pClassifyOut->rights & FWPS_RIGHT_ACTION_WRITE)
   {
      PerformBasicAction(pClassifyValues,
                         pMetadata,
                         pLayerData,
                         0,
                         pFilter,
                         flowContext,
                         pClassifyOut,
                         FWP_ACTION_CONTINUE);
   }

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- ClassifyBasicActionContinue() [Layer: %s][FilterID: %#I64x][Action: %#x][Rights: %#x][Absorb: %s]\n",
              KrnlHlprFwpsLayerIDToString(pClassifyValues->layerId),
              pFilter->filterId,
              pClassifyOut->actionType,
              pClassifyOut->rights,
              (pClassifyOut->flags & FWPS_CLASSIFY_OUT_FLAG_ABSORB) ? "TRUE" : "FALSE");

   return;
}

/**
 @classify_function="ClassifyBasicActionPermit"
 
   Purpose:  Sets the pClassifyOut->actionType to FWP_ACTION_PERMIT.                            <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF551229.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF544890.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI ClassifyBasicActionPermit(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                     _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                     _Inout_opt_ VOID* pLayerData,
                                     _In_ const FWPS_FILTER* pFilter,
                                     _In_ UINT64 flowContext,
                                     _Inout_ FWPS_CLASSIFY_OUT* pClassifyOut)
{
   NT_ASSERT(pClassifyValues);
   NT_ASSERT(pMetadata);
   NT_ASSERT(pFilter);
   NT_ASSERT(pClassifyOut);

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> ClassifyBasicActionPermit() [Layer: %s][FilterID: %#I64x][Rights: %#x]",
              KrnlHlprFwpsLayerIDToString(pClassifyValues->layerId),
              pFilter->filterId,
              pClassifyOut->rights);

   if(pClassifyOut->rights & FWPS_RIGHT_ACTION_WRITE)
   {
      PerformBasicAction(pClassifyValues,
                         pMetadata,
                         pLayerData,
                         0,
                         pFilter,
                         flowContext,
                         pClassifyOut,
                         FWP_ACTION_PERMIT);
   }

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- ClassifyBasicActionPermit() [Layer: %s][FilterID: %#I64x][Action: %#x][Rights: %#x][Absorb: %s]\n",
              KrnlHlprFwpsLayerIDToString(pClassifyValues->layerId),
              pFilter->filterId,
              pClassifyOut->actionType,
              pClassifyOut->rights,
              (pClassifyOut->flags & FWPS_CLASSIFY_OUT_FLAG_ABSORB) ? "TRUE" : "FALSE");

#endif /// DBG

   return;
}

/**
 @classify_function="ClassifyBasicActionRandom"
 
   Purpose:  Sets the pClassifyOut->actionType to either FWP_ACTION_BLOCK or FWP_ACTION_PERMIT. <br>
                                                                                                <br>
   Notes:    Specifying -rab at the command line will allow one to determine how randomly 
                FWP_ACTION_BLOCK is returned.                                                   <br>
             Specifying -rac at the command line will allow one to determine how randomly 
                FWP_ACTION_CONTINUE is returned.                                                <br>
             Specifying -rap at the command line will allow one to determine how randomly 
                FWP_ACTION_PERMIT is returned.                                                  <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF551229.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF544890.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI ClassifyBasicActionRandom(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                     _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                     _Inout_opt_ VOID* pLayerData,
                                     _In_ const FWPS_FILTER* pFilter,
                                     _In_ UINT64 flowContext,
                                     _Inout_ FWPS_CLASSIFY_OUT* pClassifyOut)
{
   NT_ASSERT(pClassifyValues);
   NT_ASSERT(pMetadata);
   NT_ASSERT(pFilter);
   NT_ASSERT(pClassifyOut);

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> ClassifyBasicActionRandom() [Layer: %s][FilterID: %#I64x][Rights: %#x]",
              KrnlHlprFwpsLayerIDToString(pClassifyValues->layerId),
              pFilter->filterId,
              pClassifyOut->rights);

   if(pClassifyOut->rights & FWPS_RIGHT_ACTION_WRITE)
   {
      NT_ASSERT(pFilter->providerContext);
      NT_ASSERT(pFilter->providerContext->type == FWPM_GENERAL_CONTEXT);
      NT_ASSERT(pFilter->providerContext->dataBuffer);
      NT_ASSERT(pFilter->providerContext->dataBuffer->data);
      NT_ASSERT(pFilter->providerContext->dataBuffer->size == sizeof(PC_BASIC_ACTION_DATA));

      FWPM_PROVIDER_CONTEXT* pProviderContext   = pFilter->providerContext;
      PC_BASIC_ACTION_DATA*  pPCBasicActionData = (PC_BASIC_ACTION_DATA*)(pProviderContext->dataBuffer->data);
      UINT32                 randomizer         = 1;
      FWP_ACTION_TYPE        actionType         = FWP_ACTION_CONTINUE;
      UINT32                 blockCap           = pPCBasicActionData->percentBlock;
      UINT32                 permitCap          = pPCBasicActionData->percentPermit + blockCap;
      LARGE_INTEGER          largeInteger       = {0};

      KeQueryTickCount(&largeInteger);

      srand(largeInteger.u.LowPart);

      randomizer += (rand() % 100);

      if(randomizer <= blockCap)
         actionType = FWP_ACTION_BLOCK;
      else if(randomizer <= permitCap)
         actionType = FWP_ACTION_PERMIT;

      PerformBasicAction(pClassifyValues,
                         pMetadata,
                         pLayerData,
                         0,
                         pFilter,
                         flowContext,
                         pClassifyOut,
                         actionType);
   }

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- ClassifyBasicActionRandom() [Layer: %s][FilterID: %#I64x][Action: %#x][Rights: %#x][Absorb: %s]\n",
              KrnlHlprFwpsLayerIDToString(pClassifyValues->layerId),
              pFilter->filterId,
              pClassifyOut->actionType,
              pClassifyOut->rights,
              (pClassifyOut->flags & FWPS_CLASSIFY_OUT_FLAG_ABSORB) ? "TRUE" : "FALSE");

   return;
}

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)
