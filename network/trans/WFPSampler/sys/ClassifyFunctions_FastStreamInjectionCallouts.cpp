////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      ClassifyFunctions_FastStreamInjectionCallouts.cpp
//
//   Abstract:
//      This module contains WFP Classify functions that inject data back into the stream using
//         the clone / block / inject method.  This method is inline only, no modification,
//         and uses as little validation and error checking as possible. These functions are meant
//         for test performance purposes only.
//
//   Naming Convention:
//
//      <Module><Scenario>
//  
//      i.e.
//       ClassifyFastStreamInjection
//
//       <Module>
//          Classify             - Function is an FWPS_CALLOUT_CLASSIFY_FN
//       <Scenario>
//          FastStreamInjection  - Function demonstrates the clone / block / inject model in the 
//                                    fastest form available (inline, no validation, etc.).
//
//   Private Functions:
//
//   Public Functions:
//      ClassifyFastStreamInjection(),
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Enhance function declaration for IntelliSense, enhance 
//                                              traces, and add support for multiple injectors.
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Framework_WFPSamplerCalloutDriver.h"               /// .
#include "ClassifyFunctions_FastStreamInjectionCallouts.tmh" /// $(OBJ_PATH)\$(O)\ 

#if(NTDDI_VERSION >= NTDDI_WIN7)

/**
 @classify_function="ClassifyFastStreamInjection"
 
   Purpose:  Blocks the current stream data and injects a clone back to the stack without 
             modification.                                                                      <br>
                                                                                                <br>
   Notes:    Applies to the following layers:                                                   <br>
                FWPS_LAYER_STREAM_V{4/6}                                                        <br>
                                                                                                <br>
             Intended for test performance purposes only.                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF544893.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF551149.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF551213.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF551161.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI ClassifyFastStreamInjection(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                       _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                       _Inout_opt_ VOID* pStreamCalloutIOPacket,
                                       _In_opt_ const VOID* pClassifyContext,
                                       _In_ const FWPS_FILTER* pFilter,
                                       _In_ UINT64 flowContext,
                                       _Inout_ FWPS_CLASSIFY_OUT* pClassifyOut)
{
   UNREFERENCED_PARAMETER(pClassifyContext);
   UNREFERENCED_PARAMETER(flowContext);

   /// Stream has no concept of Veto. Due to its "waterfall" nature where a block will 
   /// remove the data and others will not be able to see it.  
   /// This means that if we got classified, regardless of the rights, we can do whatever we need to.

   if(pStreamCalloutIOPacket)
   {
      NTSTATUS                       status              = STATUS_SUCCESS;
      FWPS_STREAM_CALLOUT_IO_PACKET* pStreamPacket       = (FWPS_STREAM_CALLOUT_IO_PACKET*)pStreamCalloutIOPacket;
      FWP_DIRECTION                  direction           = (FWP_DIRECTION)pClassifyValues->incomingValue[FWPS_FIELD_STREAM_V4_DIRECTION].value.uint32;
      HANDLE                         injectionHandle     = 0;
      UINT64                         flowID              = 0;
      UINT32                         streamFlags         = pStreamPacket->streamData->flags;
      NET_BUFFER_LIST*               pNetBufferListChain = 0;
      UINT32                         index               = WFPSAMPLER_INDEX;

      if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_METADATA_FIELD_FLOW_HANDLE))
         flowID = pMetadata->flowHandle;

      if(pFilter->subLayerWeight == FWPM_SUBLAYER_UNIVERSAL_WEIGHT)
         index = UNIVERSAL_INDEX;

      if(pClassifyValues->layerId == FWPS_LAYER_STREAM_V4)
      {
         if(direction == FWP_DIRECTION_OUTBOUND)
            injectionHandle = g_pIPv4OutboundStreamInjectionHandles[index];
         else
            injectionHandle = g_pIPv4InboundStreamInjectionHandles[index];
      }
      else if(pClassifyValues->layerId == FWPS_LAYER_STREAM_V6)
      {
         if(direction == FWP_DIRECTION_OUTBOUND)
            injectionHandle = g_pIPv6OutboundStreamInjectionHandles[index];
         else
            injectionHandle = g_pIPv6InboundStreamInjectionHandles[index];
      }
      else
         HLPR_BAIL;

      if(!(streamFlags & FWPS_STREAM_FLAG_RECEIVE) &&
         !(streamFlags & FWPS_STREAM_FLAG_SEND))
         streamFlags |= direction ? FWPS_STREAM_FLAG_RECEIVE : FWPS_STREAM_FLAG_SEND;

      pStreamPacket->countBytesRequired = 0;
      pStreamPacket->countBytesEnforced = pStreamPacket->streamData->dataLength;
      pStreamPacket->streamAction       = FWPS_STREAM_ACTION_NONE;

      status = FwpsCloneStreamData(pStreamPacket->streamData,
                                   g_pNDISPoolData->nblPoolHandle,
                                   g_pNDISPoolData->nbPoolHandle,
                                   0,
                                   &pNetBufferListChain);
      if(status != STATUS_SUCCESS)
      {
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_ERROR_LEVEL,
                    " !!!! PerformBasicStreamInjection : FwpsCloneStreamData() [status: %#x]\n",
                    status);

         HLPR_BAIL;
      }

      /// Do not touch the rights in case other callouts use them (which they shouldn't).
      pClassifyOut->actionType = FWP_ACTION_BLOCK;
      
      status = FwpsStreamInjectAsync(injectionHandle,
                                     0,
                                     0,
                                     flowID,
                                     pFilter->action.calloutId,
                                     pClassifyValues->layerId,
                                     streamFlags,
                                     pNetBufferListChain,
                                     pStreamPacket->streamData->dataLength,
                                     CompleteFastStreamInjection,
                                     0);

      HLPR_BAIL_LABEL:

      if(status != STATUS_SUCCESS &&
         pNetBufferListChain)
      {
         KIRQL irql = KeGetCurrentIrql();

         FwpsDiscardClonedStreamData(pNetBufferListChain,
                                     0,
                                     irql == DISPATCH_LEVEL ? TRUE : FALSE);
      }
   }

   return;
}

#else

/**
 @classify_function="ClassifyFastStreamInjection"
 
   Purpose:  Blocks the current stream data and injects a clone back to the stack without 
             modification.                                                                      <br>
                                                                                                <br>
   Notes:    Applies to the following layers:                                                   <br>
                FWPS_LAYER_STREAM_V{4/6}                                                        <br>
                                                                                                <br>
             Intended for test performance purposes only.                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF544890.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF551149.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF551213.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF551161.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI ClassifyFastStreamInjection(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                       _In_ const FWPS_INCOMING_METADATA_VALUES0* pMetadata,
                                       _Inout_opt_ VOID* pStreamCalloutIOPacket,
                                       _In_ const FWPS_FILTER* pFilter,
                                       _In_ UINT64 flowContext,
                                       _Inout_ FWPS_CLASSIFY_OUT* pClassifyOut)
{
   UNREFERENCED_PARAMETER(flowContext);

   /// Stream has no concept of Veto. Due to its "waterfall" nature where a block will 
   /// remove the data and others will not be able to see it.  
   /// This means that if we got classified, regardless of the rights, we can do whatever we need to.
   
   if(pStreamCalloutIOPacket)
   {
      NTSTATUS                       status              = STATUS_SUCCESS;
      FWPS_STREAM_CALLOUT_IO_PACKET* pStreamPacket       = (FWPS_STREAM_CALLOUT_IO_PACKET*)pStreamCalloutIOPacket;
      FWP_DIRECTION                  direction           = (FWP_DIRECTION)pClassifyValues->incomingValue[FWPS_FIELD_STREAM_V4_DIRECTION].value.uint32;
      HANDLE                         injectionHandle     = 0;
      UINT64                         flowID              = 0;
      UINT32                         streamFlags         = pStreamPacket->streamData->flags;
      NET_BUFFER_LIST*               pNetBufferListChain = 0;
      UINT32                         index               = WFPSAMPLER_INDEX;

      if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_METADATA_FIELD_FLOW_HANDLE))
         flowID = pMetadata->flowHandle;

      if(pFilter->subLayerWeight == FWPM_SUBLAYER_UNIVERSAL_WEIGHT)
         index = UNIVERSAL_INDEX;

      if(pClassifyValues->layerId == FWPS_LAYER_STREAM_V4)
      {
         if(direction == FWP_DIRECTION_OUTBOUND)
            injectionHandle = g_pIPv4OutboundStreamInjectionHandles[index];
         else
            injectionHandle = g_pIPv4InboundStreamInjectionHandles[index];
      }
      else if(pClassifyValues->layerId == FWPS_LAYER_STREAM_V6)
      {
         if(direction == FWP_DIRECTION_OUTBOUND)
            injectionHandle = g_pIPv6OutboundStreamInjectionHandles[index];
         else
            injectionHandle = g_pIPv6InboundStreamInjectionHandles[index];
      }
      else
         HLPR_BAIL;
   
      if(!(streamFlags & FWPS_STREAM_FLAG_RECEIVE) &&
         !(streamFlags & FWPS_STREAM_FLAG_SEND))
         streamFlags |= direction ? FWPS_STREAM_FLAG_RECEIVE : FWPS_STREAM_FLAG_SEND;
   
      pStreamPacket->countBytesRequired = 0;
      pStreamPacket->countBytesEnforced = pStreamPacket->streamData->dataLength;
      pStreamPacket->streamAction       = FWPS_STREAM_ACTION_NONE;
   
      status = FwpsCloneStreamData(pStreamPacket->streamData,
                                   g_pNDISPoolData->nblPoolHandle,
                                   g_pNDISPoolData->nbPoolHandle,
                                   0,
                                   &pNetBufferListChain);
      if(status != STATUS_SUCCESS)
      {
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_ERROR_LEVEL,
                    " !!!! PerformBasicStreamInjection : FwpsCloneStreamData() [status: %#x]\n",
                    status);
   
         HLPR_BAIL;
      }

      /// Do not touch the rights in case other callouts use them (which they shouldn't).
      pClassifyOut->actionType  = FWP_ACTION_BLOCK;

      status = FwpsStreamInjectAsync(injectionHandle,
                                     0,
                                     0,
                                     flowID,
                                     pFilter->action.calloutId,
                                     pClassifyValues->layerId,
                                     streamFlags,
                                     pNetBufferListChain,
                                     pStreamPacket->streamData->dataLength,
                                     CompleteFastStreamInjection,
                                     0);
   
      HLPR_BAIL_LABEL:
   
      if(status != STATUS_SUCCESS &&
         pNetBufferListChain)
      {
         KIRQL irql = KeGetCurrentIrql();
   
         FwpsDiscardClonedStreamData(pNetBufferListChain,
                                     0,
                                     irql == DISPATCH_LEVEL ? TRUE : FALSE);
      }
   }
   
   return;
}

#endif // (NTDDI_VERSION >= NTDDI_WIN7)
