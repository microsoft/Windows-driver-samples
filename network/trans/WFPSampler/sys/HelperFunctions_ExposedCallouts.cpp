////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_ExposedCallouts.cpp
//
//   Abstract:
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   - Add support for ADVANCED_PACKET_INJECTION, 
//                                             FLOW_ASSOCIATION, and PEND_ENDPOINT_CLOSURE callouts
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Framework_WFPSamplerCalloutDriver.h" /// .
#include "HelperFunctions_ExposedCallouts.tmh" /// $(OBJ_PATH)\$(O)\

FWPS_CALLOUT* ppRegisteredCallouts[EXPOSED_CALLOUT_COUNT] = {0};

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID PrvExposedCalloutsUndefine()
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> PrvExposedCalloutsUndefine()\n");

#endif /// DBG
   
   for(UINT32 calloutIndex = 0;
       calloutIndex < EXPOSED_CALLOUT_COUNT;
       calloutIndex++)
   {
      HLPR_DELETE(ppRegisteredCallouts[calloutIndex],
                  WFPSAMPLER_CALLOUT_DRIVER_TAG);
   }

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- PrvExposedCalloutsUndefine()\n");

#endif /// DBG
   
   return;
}

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return == STATUS_SUCCESS)
NTSTATUS PrvExposedCalloutsDefine()
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> PrvExposedCalloutsDefine()\n");

#endif /// DBG
   
   NTSTATUS status = 0;
   UINT32   flags  = 0;

#if(NTDDI_VERSION >= NTDDI_WIN7)

   flags = FWP_CALLOUT_FLAG_ENABLE_COMMIT_ADD_NOTIFY;

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

   /// Register all ADVANCED_PACKET_INJECTION Callouts
   for(UINT32 apiIndex = 0;
       apiIndex < ADVANCED_PACKET_INJECTION_COUNT;
       apiIndex++)
   {
      HLPR_NEW(ppRegisteredCallouts[BASE_INDEX_API + apiIndex],
               FWPS_CALLOUT,
               WFPSAMPLER_CALLOUT_DRIVER_TAG);
      HLPR_BAIL_ON_ALLOC_FAILURE(ppRegisteredCallouts[BASE_INDEX_API + apiIndex],
                                 status);

      ppRegisteredCallouts[BASE_INDEX_API + apiIndex]->calloutKey   = *(ppAdvancedPacketInjection[apiIndex]);
      ppRegisteredCallouts[BASE_INDEX_API + apiIndex]->flags        = flags;
      ppRegisteredCallouts[BASE_INDEX_API + apiIndex]->classifyFn   = ClassifyAdvancedPacketInjection;
      ppRegisteredCallouts[BASE_INDEX_API + apiIndex]->notifyFn     = NotifyAdvancedNotification;
      ppRegisteredCallouts[BASE_INDEX_API + apiIndex]->flowDeleteFn = 0;
   }

   /// Register all BASIC_ACTION_BLOCK Callouts
   for(UINT32 babIndex = 0;
       babIndex < BASIC_ACTION_BLOCK_COUNT;
       babIndex++)
   {
      HLPR_NEW(ppRegisteredCallouts[BASE_INDEX_BAB + babIndex],
               FWPS_CALLOUT,
               WFPSAMPLER_CALLOUT_DRIVER_TAG);
      HLPR_BAIL_ON_ALLOC_FAILURE(ppRegisteredCallouts[BASE_INDEX_BAB + babIndex],
                                 status);

      ppRegisteredCallouts[BASE_INDEX_BAB + babIndex]->calloutKey   = *(ppBasicActionBlock[babIndex]);
      ppRegisteredCallouts[BASE_INDEX_BAB + babIndex]->flags        = flags;
      ppRegisteredCallouts[BASE_INDEX_BAB + babIndex]->classifyFn   = ClassifyBasicActionBlock;
      ppRegisteredCallouts[BASE_INDEX_BAB + babIndex]->notifyFn     = NotifyBasicNotification;
      ppRegisteredCallouts[BASE_INDEX_BAB + babIndex]->flowDeleteFn = 0;
   }

   /// Register all BASIC_ACTION_CONTINUE Callouts
   for(UINT32 bacIndex = 0;
       bacIndex < BASIC_ACTION_CONTINUE_COUNT;
       bacIndex++)
   {
      HLPR_NEW(ppRegisteredCallouts[BASE_INDEX_BAC + bacIndex],
               FWPS_CALLOUT,
               WFPSAMPLER_CALLOUT_DRIVER_TAG);
      HLPR_BAIL_ON_ALLOC_FAILURE(ppRegisteredCallouts[BASE_INDEX_BAC + bacIndex],
                                 status);

      ppRegisteredCallouts[BASE_INDEX_BAC + bacIndex]->calloutKey   = *(ppBasicActionContinue[bacIndex]);
      ppRegisteredCallouts[BASE_INDEX_BAC + bacIndex]->flags        = flags;
      ppRegisteredCallouts[BASE_INDEX_BAC + bacIndex]->classifyFn   = ClassifyBasicActionContinue;
      ppRegisteredCallouts[BASE_INDEX_BAC + bacIndex]->notifyFn     = NotifyBasicNotification;
      ppRegisteredCallouts[BASE_INDEX_BAC + bacIndex]->flowDeleteFn = 0;
   }

   /// Register all BASIC_ACTION_PERMIT Callouts
   for(UINT32 bapIndex = 0;
       bapIndex < BASIC_ACTION_PERMIT_COUNT;
       bapIndex++)
   {
      HLPR_NEW(ppRegisteredCallouts[BASE_INDEX_BAP + bapIndex],
               FWPS_CALLOUT,
               WFPSAMPLER_CALLOUT_DRIVER_TAG);
      HLPR_BAIL_ON_ALLOC_FAILURE(ppRegisteredCallouts[BASE_INDEX_BAP + bapIndex],
                                 status);

      ppRegisteredCallouts[BASE_INDEX_BAP + bapIndex]->calloutKey   = *(ppBasicActionPermit[bapIndex]);
      ppRegisteredCallouts[BASE_INDEX_BAP + bapIndex]->flags        = flags;
      ppRegisteredCallouts[BASE_INDEX_BAP + bapIndex]->classifyFn   = ClassifyBasicActionPermit;
      ppRegisteredCallouts[BASE_INDEX_BAP + bapIndex]->notifyFn     = NotifyBasicNotification;
      ppRegisteredCallouts[BASE_INDEX_BAP + bapIndex]->flowDeleteFn = 0;
   }

   /// Register all BASIC_ACTION_RANDOM Callouts
   for(UINT32 barIndex = 0;
       barIndex < BASIC_ACTION_RANDOM_COUNT;
       barIndex++)
   {
      HLPR_NEW(ppRegisteredCallouts[BASE_INDEX_BAR + barIndex],
               FWPS_CALLOUT,
               WFPSAMPLER_CALLOUT_DRIVER_TAG);
      HLPR_BAIL_ON_ALLOC_FAILURE(ppRegisteredCallouts[BASE_INDEX_BAR + barIndex],
                                 status);

      ppRegisteredCallouts[BASE_INDEX_BAR + barIndex]->calloutKey   = *(ppBasicActionRandom[barIndex]);
      ppRegisteredCallouts[BASE_INDEX_BAR + barIndex]->flags        = flags;
      ppRegisteredCallouts[BASE_INDEX_BAR + barIndex]->classifyFn   = ClassifyBasicActionRandom;
      ppRegisteredCallouts[BASE_INDEX_BAR + barIndex]->notifyFn     = NotifyBasicNotification;
      ppRegisteredCallouts[BASE_INDEX_BAR + barIndex]->flowDeleteFn = 0;
   }

   /// Register all BASIC_PACKET_EXAMINATION Callouts
   for(UINT32 bpeIndex = 0;
       bpeIndex < BASIC_PACKET_EXAMINATION_COUNT;
       bpeIndex++)
   {
      HLPR_NEW(ppRegisteredCallouts[BASE_INDEX_BPE + bpeIndex],
               FWPS_CALLOUT,
               WFPSAMPLER_CALLOUT_DRIVER_TAG);
      HLPR_BAIL_ON_ALLOC_FAILURE(ppRegisteredCallouts[BASE_INDEX_BPE +bpeIndex],
                                 status);

      ppRegisteredCallouts[BASE_INDEX_BPE + bpeIndex]->calloutKey   = *(ppBasicPacketExamination[bpeIndex]);
      ppRegisteredCallouts[BASE_INDEX_BPE + bpeIndex]->flags        = flags;
      ppRegisteredCallouts[BASE_INDEX_BPE + bpeIndex]->classifyFn   = ClassifyBasicPacketExamination;
      ppRegisteredCallouts[BASE_INDEX_BPE + bpeIndex]->notifyFn     = NotifyBasicNotification;
      ppRegisteredCallouts[BASE_INDEX_BPE + bpeIndex]->flowDeleteFn = 0;
   }

   /// Register all BASIC_PACKET_INJECTION Callouts
   for(UINT32 bpiIndex = 0;
       bpiIndex < BASIC_PACKET_INJECTION_COUNT;
       bpiIndex++)
   {
      HLPR_NEW(ppRegisteredCallouts[BASE_INDEX_BPI + bpiIndex],
               FWPS_CALLOUT,
               WFPSAMPLER_CALLOUT_DRIVER_TAG);
      HLPR_BAIL_ON_ALLOC_FAILURE(ppRegisteredCallouts[BASE_INDEX_BPI + bpiIndex],
                                 status);

      ppRegisteredCallouts[BASE_INDEX_BPI + bpiIndex]->calloutKey   = *(ppBasicPacketInjection[bpiIndex]);
      ppRegisteredCallouts[BASE_INDEX_BPI + bpiIndex]->flags        = flags;
      ppRegisteredCallouts[BASE_INDEX_BPI + bpiIndex]->classifyFn   = ClassifyBasicPacketInjection;
      ppRegisteredCallouts[BASE_INDEX_BPI + bpiIndex]->notifyFn     = NotifyBasicNotification;
      ppRegisteredCallouts[BASE_INDEX_BPI + bpiIndex]->flowDeleteFn = 0;
   }

   /// Register all BASIC_PACKET_MODIFICATION Callouts
   for(UINT32 bpmIndex = 0;
       bpmIndex < BASIC_PACKET_MODIFICATION_COUNT;
       bpmIndex++)
   {
      HLPR_NEW(ppRegisteredCallouts[BASE_INDEX_BPM + bpmIndex],
               FWPS_CALLOUT,
               WFPSAMPLER_CALLOUT_DRIVER_TAG);
      HLPR_BAIL_ON_ALLOC_FAILURE(ppRegisteredCallouts[BASE_INDEX_BPM + bpmIndex],
                                 status);

      ppRegisteredCallouts[BASE_INDEX_BPM + bpmIndex]->calloutKey   = *(ppBasicPacketModification[bpmIndex]);
      ppRegisteredCallouts[BASE_INDEX_BPM + bpmIndex]->flags        = flags;
      ppRegisteredCallouts[BASE_INDEX_BPM + bpmIndex]->classifyFn   = ClassifyBasicPacketModification;
      ppRegisteredCallouts[BASE_INDEX_BPM + bpmIndex]->notifyFn     = NotifyBasicNotification;
      ppRegisteredCallouts[BASE_INDEX_BPM + bpmIndex]->flowDeleteFn = 0;
   }

   /// Register all BASIC_STREAM_INJECTION Callouts
   for(UINT32 bsiIndex = 0;
       bsiIndex < BASIC_STREAM_INJECTION_COUNT;
       bsiIndex++)
   {
      UINT32 calloutFlags = flags;

      HLPR_NEW(ppRegisteredCallouts[BASE_INDEX_BSI + bsiIndex],
               FWPS_CALLOUT,
               WFPSAMPLER_CALLOUT_DRIVER_TAG);
      HLPR_BAIL_ON_ALLOC_FAILURE(ppRegisteredCallouts[BASE_INDEX_BSI + bsiIndex],
                                 status);

#if(NTDDI_VERSION >= NTDDI_WIN7)

      calloutFlags |= FWP_CALLOUT_FLAG_ALLOW_MID_STREAM_INSPECTION;

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

      ppRegisteredCallouts[BASE_INDEX_BSI + bsiIndex]->calloutKey   = *(ppBasicStreamInjection[bsiIndex]);
      ppRegisteredCallouts[BASE_INDEX_BSI + bsiIndex]->flags        = calloutFlags;
      ppRegisteredCallouts[BASE_INDEX_BSI + bsiIndex]->classifyFn   = ClassifyBasicStreamInjection;
      ppRegisteredCallouts[BASE_INDEX_BSI + bsiIndex]->notifyFn     = NotifyBasicNotification;
      ppRegisteredCallouts[BASE_INDEX_BSI + bsiIndex]->flowDeleteFn = NotifyFlowDeleteNotification;
   }

   /// Register all FAST_PACKET_INJECTION Callouts
   for(UINT32 fpiIndex = 0;
       fpiIndex < FAST_PACKET_INJECTION_COUNT;
       fpiIndex++)
   {
      HLPR_NEW(ppRegisteredCallouts[BASE_INDEX_FPI + fpiIndex],
               FWPS_CALLOUT,
               WFPSAMPLER_CALLOUT_DRIVER_TAG);
      HLPR_BAIL_ON_ALLOC_FAILURE(ppRegisteredCallouts[BASE_INDEX_FPI + fpiIndex],
                                 status);

      ppRegisteredCallouts[BASE_INDEX_FPI + fpiIndex]->calloutKey   = *(ppFastPacketInjection[fpiIndex]);
      ppRegisteredCallouts[BASE_INDEX_FPI + fpiIndex]->flags        = flags;
      ppRegisteredCallouts[BASE_INDEX_FPI + fpiIndex]->classifyFn   = ClassifyFastPacketInjection;
      ppRegisteredCallouts[BASE_INDEX_FPI + fpiIndex]->notifyFn     = NotifyFastNotification;
      ppRegisteredCallouts[BASE_INDEX_FPI + fpiIndex]->flowDeleteFn = 0;
   }

   /// Register all FAST_STREAM_INJECTION Callouts
   for(UINT32 fsiIndex = 0;
       fsiIndex < FAST_STREAM_INJECTION_COUNT;
       fsiIndex++)
   {
      UINT32 calloutFlags = flags;

      HLPR_NEW(ppRegisteredCallouts[BASE_INDEX_FSI + fsiIndex],
               FWPS_CALLOUT,
               WFPSAMPLER_CALLOUT_DRIVER_TAG);
      HLPR_BAIL_ON_ALLOC_FAILURE(ppRegisteredCallouts[BASE_INDEX_FSI + fsiIndex],
                                 status);

#if(NTDDI_VERSION >= NTDDI_WIN7)
      
            calloutFlags |= FWP_CALLOUT_FLAG_ALLOW_MID_STREAM_INSPECTION;
      
#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

      ppRegisteredCallouts[BASE_INDEX_FSI + fsiIndex]->calloutKey   = *(ppFastStreamInjection[fsiIndex]);
      ppRegisteredCallouts[BASE_INDEX_FSI + fsiIndex]->flags        = calloutFlags;
      ppRegisteredCallouts[BASE_INDEX_FSI + fsiIndex]->classifyFn   = ClassifyFastStreamInjection;
      ppRegisteredCallouts[BASE_INDEX_FSI + fsiIndex]->notifyFn     = NotifyFastNotification;
      ppRegisteredCallouts[BASE_INDEX_FSI + fsiIndex]->flowDeleteFn = NotifyFlowDeleteNotification;
   }

   /// Register all FLOW_ASSOCIATION Callouts
   for(UINT32 faIndex = 0;
       faIndex < FLOW_ASSOCIATION_COUNT;
       faIndex++)
   {
      HLPR_NEW(ppRegisteredCallouts[BASE_INDEX_FA + faIndex],
               FWPS_CALLOUT,
               WFPSAMPLER_CALLOUT_DRIVER_TAG);
      HLPR_BAIL_ON_ALLOC_FAILURE(ppRegisteredCallouts[BASE_INDEX_FA + faIndex],
                                 status);

      ppRegisteredCallouts[BASE_INDEX_FA + faIndex]->calloutKey   = *(ppFlowAssociation[faIndex]);
      ppRegisteredCallouts[BASE_INDEX_FA + faIndex]->flags        = flags;
      ppRegisteredCallouts[BASE_INDEX_FA + faIndex]->classifyFn   = ClassifyFlowAssociation;
      ppRegisteredCallouts[BASE_INDEX_FA + faIndex]->notifyFn     = NotifyBasicNotification;
      ppRegisteredCallouts[BASE_INDEX_FA + faIndex]->flowDeleteFn = 0;
   }

   /// Register all PEND_AUTHORIZATION Callouts
   for(UINT32 paIndex = 0;
       paIndex < PEND_AUTHORIZATION_COUNT;
       paIndex++)
   {
      HLPR_NEW(ppRegisteredCallouts[BASE_INDEX_PA + paIndex],
               FWPS_CALLOUT,
               WFPSAMPLER_CALLOUT_DRIVER_TAG);
      HLPR_BAIL_ON_ALLOC_FAILURE(ppRegisteredCallouts[BASE_INDEX_PA + paIndex],
                                 status);

      ppRegisteredCallouts[BASE_INDEX_PA + paIndex]->calloutKey   = *(ppPendAuthorization[paIndex]);
      ppRegisteredCallouts[BASE_INDEX_PA + paIndex]->flags        = flags;
      ppRegisteredCallouts[BASE_INDEX_PA + paIndex]->classifyFn   = ClassifyPendAuthorization;
      ppRegisteredCallouts[BASE_INDEX_PA + paIndex]->notifyFn     = NotifyPendNotification;
      ppRegisteredCallouts[BASE_INDEX_PA + paIndex]->flowDeleteFn = 0;
   }

#if(NTDDI_VERSION >= NTDDI_WIN7)

   /// Register all PEND_ENDPOINT_CLOSURE Callouts
   for(UINT32 pecIndex = 0;
       pecIndex < PEND_ENDPOINT_CLOSURE_COUNT;
       pecIndex++)
   {
      HLPR_NEW(ppRegisteredCallouts[BASE_INDEX_PEC + pecIndex],
               FWPS_CALLOUT,
               WFPSAMPLER_CALLOUT_DRIVER_TAG);
      HLPR_BAIL_ON_ALLOC_FAILURE(ppRegisteredCallouts[BASE_INDEX_PEC + pecIndex],
                                 status);

      ppRegisteredCallouts[BASE_INDEX_PEC + pecIndex]->calloutKey   = *(ppPendEndpointClosure[pecIndex]);
      ppRegisteredCallouts[BASE_INDEX_PEC + pecIndex]->flags        = flags;
      ppRegisteredCallouts[BASE_INDEX_PEC + pecIndex]->classifyFn   = ClassifyPendEndpointClosure;
      ppRegisteredCallouts[BASE_INDEX_PEC + pecIndex]->notifyFn     = NotifyPendNotification;
      ppRegisteredCallouts[BASE_INDEX_PEC + pecIndex]->flowDeleteFn = NotifyFlowDeleteNotification;
   }

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

   /// Register all PROXY_BY_INJECTION Callouts
   for(UINT32 pbiIndex = 0;
       pbiIndex < PROXY_BY_INJECTION_COUNT;
       pbiIndex++)
   {
      HLPR_NEW(ppRegisteredCallouts[BASE_INDEX_PBI + pbiIndex],
               FWPS_CALLOUT,
               WFPSAMPLER_CALLOUT_DRIVER_TAG);
      HLPR_BAIL_ON_ALLOC_FAILURE(ppRegisteredCallouts[BASE_INDEX_PBI + pbiIndex],
                                 status);

      ppRegisteredCallouts[BASE_INDEX_PBI + pbiIndex]->calloutKey   = *(ppProxyByInjection[pbiIndex]);
      ppRegisteredCallouts[BASE_INDEX_PBI + pbiIndex]->flags        = flags;
      ppRegisteredCallouts[BASE_INDEX_PBI + pbiIndex]->classifyFn   = ClassifyProxyByInjection;
      ppRegisteredCallouts[BASE_INDEX_PBI + pbiIndex]->notifyFn     = NotifyBasicNotification;
      ppRegisteredCallouts[BASE_INDEX_PBI + pbiIndex]->flowDeleteFn = 0;
   }


#if(NTDDI_VERSION >= NTDDI_WIN7)

      /// Register all PROXY_BY_ALE Callouts
      for(UINT32 pbaIndex = 0;
          pbaIndex < PROXY_BY_ALE_REDIRECT_COUNT;
          pbaIndex++)
      {
         HLPR_NEW(ppRegisteredCallouts[BASE_INDEX_PBA + pbaIndex],
                  FWPS_CALLOUT,
                  WFPSAMPLER_CALLOUT_DRIVER_TAG);
         HLPR_BAIL_ON_ALLOC_FAILURE(ppRegisteredCallouts[BASE_INDEX_PBA + pbaIndex],
                                    status);
   
         ppRegisteredCallouts[BASE_INDEX_PBA + pbaIndex]->calloutKey   = *(ppProxyByALERedirect[pbaIndex]);
         ppRegisteredCallouts[BASE_INDEX_PBA + pbaIndex]->flags        = flags;
         ppRegisteredCallouts[BASE_INDEX_PBA + pbaIndex]->classifyFn   = ClassifyProxyByALERedirect;
         ppRegisteredCallouts[BASE_INDEX_PBA + pbaIndex]->notifyFn     = NotifyProxyByALERedirectNotification;
         ppRegisteredCallouts[BASE_INDEX_PBA + pbaIndex]->flowDeleteFn = 0;
      }
   
#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
      PrvExposedCalloutsUndefine();

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- PrvExposedCalloutsDefine() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}

_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprExposedCalloutsUnregister()
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprExposedCalloutsUnregister()\n");

#endif /// DBG
   
   NTSTATUS status = STATUS_SUCCESS;

   for(UINT32 calloutIndex = 0;
       calloutIndex < EXPOSED_CALLOUT_COUNT;
       calloutIndex++)
   {
      if(ppRegisteredCallouts[calloutIndex] &&
         ppRegisteredCallouts[calloutIndex]->classifyFn)
      {
         NTSTATUS errCode = STATUS_SUCCESS;

         errCode = FwpsCalloutUnregisterByKey(&(ppRegisteredCallouts[calloutIndex]->calloutKey));
         if(errCode != STATUS_SUCCESS)
         {
            if(status == STATUS_SUCCESS)
               status = STATUS_UNSUCCESSFUL;

            DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                       DPFLTR_ERROR_LEVEL,
                       " !!!! KrnlHlprExposedCalloutsUnregister() [status: %#x]\n",
                       errCode);
         }
      }
   }

   PrvExposedCalloutsUndefine();

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprExposedCalloutsUnregister() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}

_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprExposedCalloutsRegister()
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprExposedCalloutsRegister()\n");

#endif /// DBG
   
   NTSTATUS status = STATUS_SUCCESS;

   status = PrvExposedCalloutsDefine();
   HLPR_BAIL_ON_FAILURE(status);

   for(UINT32 calloutIndex = 0;
       calloutIndex < EXPOSED_CALLOUT_COUNT;
       calloutIndex++)
   {
      if(ppRegisteredCallouts[calloutIndex] &&
         ppRegisteredCallouts[calloutIndex]->classifyFn)
      {
         status = FwpsCalloutRegister(g_pWDMDevice,
                                      ppRegisteredCallouts[calloutIndex],
                                      0);
         if(status != STATUS_SUCCESS)
         {
            DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                       DPFLTR_ERROR_LEVEL,
                       " !!!! KrnlHlprExposedCalloutsRegister : FwpsCalloutRegister() [status: %#x][Index: %#x]\n",
                       status,
                       calloutIndex);

            HLPR_BAIL;
         }
      }
   }

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
      KrnlHlprExposedCalloutsUnregister();

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprExposedCalloutsRegister() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return != 0)
PSTR KrnlHlprExposedCalloutToString(_In_ const GUID* pCalloutKey)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprExposedCalloutToString()\n");

#endif /// DBG
   
   NT_ASSERT(pCalloutKey);

   PSTR         pCalloutString   = 0;
   const UINT32 NUM_MASKED_BYTES = 15;

   if(RtlCompareMemory(&WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK,
                       pCalloutKey,
                       NUM_MASKED_BYTES))
   {
      if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_INBOUND_IPPACKET_V4)
         pCalloutString = "BASIC_ACTION_BLOCK_AT_INBOUND_IPPACKET_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_INBOUND_IPPACKET_V6)
         pCalloutString = "BASIC_ACTION_BLOCK_AT_INBOUND_IPPACKET_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_OUTBOUND_IPPACKET_V4)
         pCalloutString = "BASIC_ACTION_BLOCK_AT_OUTBOUND_IPPACKET_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_OUTBOUND_IPPACKET_V6)
         pCalloutString = "BASIC_ACTION_BLOCK_AT_OUTBOUND_IPPACKET_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_IPFORWARD_V4)
         pCalloutString = "BASIC_ACTION_BLOCK_AT_IPFORWARD_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_IPFORWARD_V6)
         pCalloutString = "BASIC_ACTION_BLOCK_AT_IPFORWARD_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_INBOUND_TRANSPORT_V4)
         pCalloutString = "BASIC_ACTION_BLOCK_AT_INBOUND_TRANSPORT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_INBOUND_TRANSPORT_V6)
         pCalloutString = "BASIC_ACTION_BLOCK_AT_INBOUND_TRANSPORT_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_OUTBOUND_TRANSPORT_V4)
         pCalloutString = "BASIC_ACTION_BLOCK_AT_OUTBOUND_TRANSPORT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_OUTBOUND_TRANSPORT_V6)
         pCalloutString = "BASIC_ACTION_BLOCK_AT_OUTBOUND_TRANSPORT_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_STREAM_V4)
         pCalloutString = "BASIC_ACTION_BLOCK_AT_STREAM_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_STREAM_V6)
         pCalloutString = "BASIC_ACTION_BLOCK_AT_STREAM_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_DATAGRAM_DATA_V4)
         pCalloutString = "BASIC_ACTION_BLOCK_AT_DATAGRAM_DATA_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_DATAGRAM_DATA_V6)
         pCalloutString = "BASIC_ACTION_BLOCK_AT_DATAGRAM_DATA_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_INBOUND_ICMP_ERROR_V4)
         pCalloutString = "BASIC_ACTION_BLOCK_AT_INBOUND_ICMP_ERROR_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_INBOUND_ICMP_ERROR_V6)
         pCalloutString = "BASIC_ACTION_BLOCK_AT_INBOUND_ICMP_ERROR_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_OUTBOUND_ICMP_ERROR_V4)
         pCalloutString = "BASIC_ACTION_BLOCK_AT_OUTBOUND_ICMP_ERROR_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_OUTBOUND_ICMP_ERROR_V6)
         pCalloutString = "BASIC_ACTION_BLOCK_AT_OUTBOUND_ICMP_ERROR_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_ALE_RESOURCE_ASSIGNMENT_V4)
         pCalloutString = "BASIC_ACTION_BLOCK_AT_ALE_RESOURCE_ASSIGNMENT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_ALE_RESOURCE_ASSIGNMENT_V6)
         pCalloutString = "BASIC_ACTION_BLOCK_AT_ALE_RESOURCE_ASSIGNMENT_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_ALE_AUTH_LISTEN_V4)
         pCalloutString = "BASIC_ACTION_BLOCK_AT_ALE_AUTH_LISTEN_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_ALE_AUTH_LISTEN_V6)
         pCalloutString = "BASIC_ACTION_BLOCK_AT_ALE_AUTH_LISTEN_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_ALE_AUTH_RECV_ACCEPT_V4)
         pCalloutString = "BASIC_ACTION_BLOCK_AT_ALE_AUTH_RECV_ACCEPT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_ALE_AUTH_RECV_ACCEPT_V6)
         pCalloutString = "BASIC_ACTION_BLOCK_AT_ALE_AUTH_RECV_ACCEPT_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_ALE_AUTH_CONNECT_V4)
         pCalloutString = "BASIC_ACTION_BLOCK_AT_ALE_AUTH_CONNECT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_ALE_AUTH_CONNECT_V6)
         pCalloutString = "BASIC_ACTION_BLOCK_AT_ALE_AUTH_CONNECT_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_ALE_FLOW_ESTABLISHED_V4)
         pCalloutString = "BASIC_ACTION_BLOCK_AT_ALE_FLOW_ESTABLISHED_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_ALE_FLOW_ESTABLISHED_V6)
         pCalloutString = "BASIC_ACTION_BLOCK_AT_ALE_FLOW_ESTABLISHED_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_INBOUND_MAC_FRAME_ETHERNET)
         pCalloutString = "BASIC_ACTION_BLOCK_AT_INBOUND_MAC_FRAME_ETHERNET";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_OUTBOUND_MAC_FRAME_ETHERNET)
         pCalloutString = "BASIC_ACTION_BLOCK_AT_OUTBOUND_MAC_FRAME_ETHERNET";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_ALE_RESOURCE_RELEASE_V4)
         pCalloutString = "BASIC_ACTION_BLOCK_AT_ALE_RESOURCE_RELEASE_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_ALE_RESOURCE_RELEASE_V6)
         pCalloutString = "BASIC_ACTION_BLOCK_AT_ALE_RESOURCE_RELEASE_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_ALE_ENDPOINT_CLOSURE_V4)
         pCalloutString = "BASIC_ACTION_BLOCK_AT_ALE_ENDPOINT_CLOSURE_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_ALE_ENDPOINT_CLOSURE_V6)
         pCalloutString = "BASIC_ACTION_BLOCK_AT_ALE_ENDPOINT_CLOSURE_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_STREAM_PACKET_V4)
         pCalloutString = "BASIC_ACTION_BLOCK_AT_STREAM_PACKET_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_STREAM_PACKET_V6)
         pCalloutString = "BASIC_ACTION_BLOCK_AT_STREAM_PACKET_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_INBOUND_MAC_FRAME_NATIVE)
         pCalloutString = "BASIC_ACTION_BLOCK_AT_INBOUND_MAC_FRAME_NATIVE";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_OUTBOUND_MAC_FRAME_NATIVE)
         pCalloutString = "BASIC_ACTION_BLOCK_AT_OUTBOUND_MAC_FRAME_NATIVE";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_INGRESS_VSWITCH_ETHERNET)
         pCalloutString = "BASIC_ACTION_BLOCK_AT_INGRESS_VSWITCH_ETHERNET";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_EGRESS_VSWITCH_ETHERNET)
         pCalloutString = "BASIC_ACTION_BLOCK_AT_EGRESS_VSWITCH_ETHERNET";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_INGRESS_VSWITCH_TRANSPORT_V4)
         pCalloutString = "BASIC_ACTION_BLOCK_AT_INGRESS_VSWITCH_TRANSPORT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_INGRESS_VSWITCH_TRANSPORT_V6)
         pCalloutString = "BASIC_ACTION_BLOCK_AT_INGRESS_VSWITCH_TRANSPORT_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_EGRESS_VSWITCH_TRANSPORT_V4)
         pCalloutString = "BASIC_ACTION_BLOCK_AT_EGRESS_VSWITCH_TRANSPORT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_EGRESS_VSWITCH_TRANSPORT_V6)
         pCalloutString = "BASIC_ACTION_BLOCK_AT_EGRESS_VSWITCH_TRANSPORT_V6";
   }
   else if(RtlCompareMemory(&WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE,
                            pCalloutKey,
                            NUM_MASKED_BYTES))
   {
      if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_INBOUND_IPPACKET_V4)
         pCalloutString = "BASIC_ACTION_CONTINUE_AT_INBOUND_IPPACKET_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_INBOUND_IPPACKET_V6)
         pCalloutString = "BASIC_ACTION_CONTINUE_AT_INBOUND_IPPACKET_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_OUTBOUND_IPPACKET_V4)
         pCalloutString = "BASIC_ACTION_CONTINUE_AT_OUTBOUND_IPPACKET_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_OUTBOUND_IPPACKET_V6)
         pCalloutString = "BASIC_ACTION_CONTINUE_AT_OUTBOUND_IPPACKET_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_IPFORWARD_V4)
         pCalloutString = "BASIC_ACTION_CONTINUE_AT_IPFORWARD_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_IPFORWARD_V6)
         pCalloutString = "BASIC_ACTION_CONTINUE_AT_IPFORWARD_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_INBOUND_TRANSPORT_V4)
         pCalloutString = "BASIC_ACTION_CONTINUE_AT_INBOUND_TRANSPORT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_INBOUND_TRANSPORT_V6)
         pCalloutString = "BASIC_ACTION_CONTINUE_AT_INBOUND_TRANSPORT_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_OUTBOUND_TRANSPORT_V4)
         pCalloutString = "BASIC_ACTION_CONTINUE_AT_OUTBOUND_TRANSPORT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_OUTBOUND_TRANSPORT_V6)
         pCalloutString = "BASIC_ACTION_CONTINUE_AT_OUTBOUND_TRANSPORT_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_STREAM_V4)
         pCalloutString = "BASIC_ACTION_CONTINUE_AT_STREAM_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_STREAM_V6)
         pCalloutString = "BASIC_ACTION_CONTINUE_AT_STREAM_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_DATAGRAM_DATA_V4)
         pCalloutString = "BASIC_ACTION_CONTINUE_AT_DATAGRAM_DATA_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_DATAGRAM_DATA_V6)
         pCalloutString = "BASIC_ACTION_CONTINUE_AT_DATAGRAM_DATA_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_INBOUND_ICMP_ERROR_V4)
         pCalloutString = "BASIC_ACTION_CONTINUE_AT_INBOUND_ICMP_ERROR_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_INBOUND_ICMP_ERROR_V6)
         pCalloutString = "BASIC_ACTION_CONTINUE_AT_INBOUND_ICMP_ERROR_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_OUTBOUND_ICMP_ERROR_V4)
         pCalloutString = "BASIC_ACTION_CONTINUE_AT_OUTBOUND_ICMP_ERROR_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_OUTBOUND_ICMP_ERROR_V6)
         pCalloutString = "BASIC_ACTION_CONTINUE_AT_OUTBOUND_ICMP_ERROR_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_ALE_RESOURCE_ASSIGNMENT_V4)
         pCalloutString = "BASIC_ACTION_CONTINUE_AT_ALE_RESOURCE_ASSIGNMENT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_ALE_RESOURCE_ASSIGNMENT_V6)
         pCalloutString = "BASIC_ACTION_CONTINUE_AT_ALE_RESOURCE_ASSIGNMENT_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_ALE_AUTH_LISTEN_V4)
         pCalloutString = "BASIC_ACTION_CONTINUE_AT_ALE_AUTH_LISTEN_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_ALE_AUTH_LISTEN_V6)
         pCalloutString = "BASIC_ACTION_CONTINUE_AT_ALE_AUTH_LISTEN_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_ALE_AUTH_RECV_ACCEPT_V4)
         pCalloutString = "BASIC_ACTION_CONTINUE_AT_ALE_AUTH_RECV_ACCEPT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_ALE_AUTH_RECV_ACCEPT_V6)
         pCalloutString = "BASIC_ACTION_CONTINUE_AT_ALE_AUTH_RECV_ACCEPT_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_ALE_AUTH_CONNECT_V4)
         pCalloutString = "BASIC_ACTION_CONTINUE_AT_ALE_AUTH_CONNECT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_ALE_AUTH_CONNECT_V6)
         pCalloutString = "BASIC_ACTION_CONTINUE_AT_ALE_AUTH_CONNECT_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_ALE_FLOW_ESTABLISHED_V4)
         pCalloutString = "BASIC_ACTION_CONTINUE_AT_ALE_FLOW_ESTABLISHED_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_ALE_FLOW_ESTABLISHED_V6)
         pCalloutString = "BASIC_ACTION_CONTINUE_AT_ALE_FLOW_ESTABLISHED_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_INBOUND_MAC_FRAME_ETHERNET)
         pCalloutString = "BASIC_ACTION_CONTINUE_AT_INBOUND_MAC_FRAME_ETHERNET";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_OUTBOUND_MAC_FRAME_ETHERNET)
         pCalloutString = "BASIC_ACTION_CONTINUE_AT_OUTBOUND_MAC_FRAME_ETHERNET";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_ALE_RESOURCE_RELEASE_V4)
         pCalloutString = "BASIC_ACTION_CONTINUE_AT_ALE_RESOURCE_RELEASE_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_ALE_RESOURCE_RELEASE_V6)
         pCalloutString = "BASIC_ACTION_CONTINUE_AT_ALE_RESOURCE_RELEASE_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_ALE_ENDPOINT_CLOSURE_V4)
         pCalloutString = "BASIC_ACTION_CONTINUE_AT_ALE_ENDPOINT_CLOSURE_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_ALE_ENDPOINT_CLOSURE_V6)
         pCalloutString = "BASIC_ACTION_CONTINUE_AT_ALE_ENDPOINT_CLOSURE_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_STREAM_PACKET_V4)
         pCalloutString = "BASIC_ACTION_CONTINUE_AT_STREAM_PACKET_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_STREAM_PACKET_V6)
         pCalloutString = "BASIC_ACTION_CONTINUE_AT_STREAM_PACKET_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_INBOUND_MAC_FRAME_NATIVE)
         pCalloutString = "BASIC_ACTION_CONTINUE_AT_INBOUND_MAC_FRAME_NATIVE";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_OUTBOUND_MAC_FRAME_NATIVE)
         pCalloutString = "BASIC_ACTION_CONTINUE_AT_OUTBOUND_MAC_FRAME_NATIVE";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_INGRESS_VSWITCH_ETHERNET)
         pCalloutString = "BASIC_ACTION_CONTINUE_AT_INGRESS_VSWITCH_ETHERNET";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_EGRESS_VSWITCH_ETHERNET)
         pCalloutString = "BASIC_ACTION_CONTINUE_AT_EGRESS_VSWITCH_ETHERNET";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_INGRESS_VSWITCH_TRANSPORT_V4)
         pCalloutString = "BASIC_ACTION_CONTINUE_AT_INGRESS_VSWITCH_TRANSPORT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_INGRESS_VSWITCH_TRANSPORT_V6)
         pCalloutString = "BASIC_ACTION_CONTINUE_AT_INGRESS_VSWITCH_TRANSPORT_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_EGRESS_VSWITCH_TRANSPORT_V4)
         pCalloutString = "BASIC_ACTION_CONTINUE_AT_EGRESS_VSWITCH_TRANSPORT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_EGRESS_VSWITCH_TRANSPORT_V6)
         pCalloutString = "BASIC_ACTION_CONTINUE_AT_EGRESS_VSWITCH_TRANSPORT_V6";
   }
   else if(RtlCompareMemory(&WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT,
                            pCalloutKey,
                            NUM_MASKED_BYTES))
   {
      if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_INBOUND_IPPACKET_V4)
         pCalloutString = "BASIC_ACTION_PERMIT_AT_INBOUND_IPPACKET_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_INBOUND_IPPACKET_V6)
         pCalloutString = "BASIC_ACTION_PERMIT_AT_INBOUND_IPPACKET_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_OUTBOUND_IPPACKET_V4)
         pCalloutString = "BASIC_ACTION_PERMIT_AT_OUTBOUND_IPPACKET_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_OUTBOUND_IPPACKET_V6)
         pCalloutString = "BASIC_ACTION_PERMIT_AT_OUTBOUND_IPPACKET_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_IPFORWARD_V4)
         pCalloutString = "BASIC_ACTION_PERMIT_AT_IPFORWARD_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_IPFORWARD_V6)
         pCalloutString = "BASIC_ACTION_PERMIT_AT_IPFORWARD_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_INBOUND_TRANSPORT_V4)
         pCalloutString = "BASIC_ACTION_PERMIT_AT_INBOUND_TRANSPORT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_INBOUND_TRANSPORT_V6)
         pCalloutString = "BASIC_ACTION_PERMIT_AT_INBOUND_TRANSPORT_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_OUTBOUND_TRANSPORT_V4)
         pCalloutString = "BASIC_ACTION_PERMIT_AT_OUTBOUND_TRANSPORT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_OUTBOUND_TRANSPORT_V6)
         pCalloutString = "BASIC_ACTION_PERMIT_AT_OUTBOUND_TRANSPORT_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_STREAM_V4)
         pCalloutString = "BASIC_ACTION_PERMIT_AT_STREAM_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_STREAM_V6)
         pCalloutString = "BASIC_ACTION_PERMIT_AT_STREAM_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_DATAGRAM_DATA_V4)
         pCalloutString = "BASIC_ACTION_PERMIT_AT_DATAGRAM_DATA_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_DATAGRAM_DATA_V6)
         pCalloutString = "BASIC_ACTION_PERMIT_AT_DATAGRAM_DATA_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_INBOUND_ICMP_ERROR_V4)
         pCalloutString = "BASIC_ACTION_PERMIT_AT_INBOUND_ICMP_ERROR_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_INBOUND_ICMP_ERROR_V6)
         pCalloutString = "BASIC_ACTION_PERMIT_AT_INBOUND_ICMP_ERROR_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_OUTBOUND_ICMP_ERROR_V4)
         pCalloutString = "BASIC_ACTION_PERMIT_AT_OUTBOUND_ICMP_ERROR_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_OUTBOUND_ICMP_ERROR_V6)
         pCalloutString = "BASIC_ACTION_PERMIT_AT_OUTBOUND_ICMP_ERROR_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_ALE_RESOURCE_ASSIGNMENT_V4)
         pCalloutString = "BASIC_ACTION_PERMIT_AT_ALE_RESOURCE_ASSIGNMENT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_ALE_RESOURCE_ASSIGNMENT_V6)
         pCalloutString = "BASIC_ACTION_PERMIT_AT_ALE_RESOURCE_ASSIGNMENT_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_ALE_AUTH_LISTEN_V4)
         pCalloutString = "BASIC_ACTION_PERMIT_AT_ALE_AUTH_LISTEN_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_ALE_AUTH_LISTEN_V6)
         pCalloutString = "BASIC_ACTION_PERMIT_AT_ALE_AUTH_LISTEN_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_ALE_AUTH_RECV_ACCEPT_V4)
         pCalloutString = "BASIC_ACTION_PERMIT_AT_ALE_AUTH_RECV_ACCEPT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_ALE_AUTH_RECV_ACCEPT_V6)
         pCalloutString = "BASIC_ACTION_PERMIT_AT_ALE_AUTH_RECV_ACCEPT_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_ALE_AUTH_CONNECT_V4)
         pCalloutString = "BASIC_ACTION_PERMIT_AT_ALE_AUTH_CONNECT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_ALE_AUTH_CONNECT_V6)
         pCalloutString = "BASIC_ACTION_PERMIT_AT_ALE_AUTH_CONNECT_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_ALE_FLOW_ESTABLISHED_V4)
         pCalloutString = "BASIC_ACTION_PERMIT_AT_ALE_FLOW_ESTABLISHED_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_ALE_FLOW_ESTABLISHED_V6)
         pCalloutString = "BASIC_ACTION_PERMIT_AT_ALE_FLOW_ESTABLISHED_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_INBOUND_MAC_FRAME_ETHERNET)
         pCalloutString = "BASIC_ACTION_PERMIT_AT_INBOUND_MAC_FRAME_ETHERNET";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_OUTBOUND_MAC_FRAME_ETHERNET)
         pCalloutString = "BASIC_ACTION_PERMIT_AT_OUTBOUND_MAC_FRAME_ETHERNET";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_ALE_RESOURCE_RELEASE_V4)
         pCalloutString = "BASIC_ACTION_PERMIT_AT_ALE_RESOURCE_RELEASE_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_ALE_RESOURCE_RELEASE_V6)
         pCalloutString = "BASIC_ACTION_PERMIT_AT_ALE_RESOURCE_RELEASE_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_ALE_ENDPOINT_CLOSURE_V4)
         pCalloutString = "BASIC_ACTION_PERMIT_AT_ALE_ENDPOINT_CLOSURE_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_ALE_ENDPOINT_CLOSURE_V6)
         pCalloutString = "BASIC_ACTION_PERMIT_AT_ALE_ENDPOINT_CLOSURE_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_STREAM_PACKET_V4)
         pCalloutString = "BASIC_ACTION_PERMIT_AT_STREAM_PACKET_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_STREAM_PACKET_V6)
         pCalloutString = "BASIC_ACTION_PERMIT_AT_STREAM_PACKET_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_INBOUND_MAC_FRAME_NATIVE)
         pCalloutString = "BASIC_ACTION_PERMIT_AT_INBOUND_MAC_FRAME_NATIVE";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_OUTBOUND_MAC_FRAME_NATIVE)
         pCalloutString = "BASIC_ACTION_PERMIT_AT_OUTBOUND_MAC_FRAME_NATIVE";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_INGRESS_VSWITCH_ETHERNET)
         pCalloutString = "BASIC_ACTION_PERMIT_AT_INGRESS_VSWITCH_ETHERNET";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_EGRESS_VSWITCH_ETHERNET)
         pCalloutString = "BASIC_ACTION_PERMIT_AT_EGRESS_VSWITCH_ETHERNET";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_INGRESS_VSWITCH_TRANSPORT_V4)
         pCalloutString = "BASIC_ACTION_PERMIT_AT_INGRESS_VSWITCH_TRANSPORT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_INGRESS_VSWITCH_TRANSPORT_V6)
         pCalloutString = "BASIC_ACTION_PERMIT_AT_INGRESS_VSWITCH_TRANSPORT_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_EGRESS_VSWITCH_TRANSPORT_V4)
         pCalloutString = "BASIC_ACTION_PERMIT_AT_EGRESS_VSWITCH_TRANSPORT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_EGRESS_VSWITCH_TRANSPORT_V6)
         pCalloutString = "BASIC_ACTION_PERMIT_AT_EGRESS_VSWITCH_TRANSPORT_V6";
   }
   else if(RtlCompareMemory(&WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM,
                            pCalloutKey,
                            NUM_MASKED_BYTES))
   {
      if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_INBOUND_IPPACKET_V4)
         pCalloutString = "BASIC_ACTION_RANDOM_AT_INBOUND_IPPACKET_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_INBOUND_IPPACKET_V6)
         pCalloutString = "BASIC_ACTION_RANDOM_AT_INBOUND_IPPACKET_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_OUTBOUND_IPPACKET_V4)
         pCalloutString = "BASIC_ACTION_RANDOM_AT_OUTBOUND_IPPACKET_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_OUTBOUND_IPPACKET_V6)
         pCalloutString = "BASIC_ACTION_RANDOM_AT_OUTBOUND_IPPACKET_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_IPFORWARD_V4)
         pCalloutString = "BASIC_ACTION_RANDOM_AT_IPFORWARD_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_IPFORWARD_V6)
         pCalloutString = "BASIC_ACTION_RANDOM_AT_IPFORWARD_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_INBOUND_TRANSPORT_V4)
         pCalloutString = "BASIC_ACTION_RANDOM_AT_INBOUND_TRANSPORT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_INBOUND_TRANSPORT_V6)
         pCalloutString = "BASIC_ACTION_RANDOM_AT_INBOUND_TRANSPORT_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_OUTBOUND_TRANSPORT_V4)
         pCalloutString = "BASIC_ACTION_RANDOM_AT_OUTBOUND_TRANSPORT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_OUTBOUND_TRANSPORT_V6)
         pCalloutString = "BASIC_ACTION_RANDOM_AT_OUTBOUND_TRANSPORT_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_STREAM_V4)
         pCalloutString = "BASIC_ACTION_RANDOM_AT_STREAM_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_STREAM_V6)
         pCalloutString = "BASIC_ACTION_RANDOM_AT_STREAM_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_DATAGRAM_DATA_V4)
         pCalloutString = "BASIC_ACTION_RANDOM_AT_DATAGRAM_DATA_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_DATAGRAM_DATA_V6)
         pCalloutString = "BASIC_ACTION_RANDOM_AT_DATAGRAM_DATA_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_INBOUND_ICMP_ERROR_V4)
         pCalloutString = "BASIC_ACTION_RANDOM_AT_INBOUND_ICMP_ERROR_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_INBOUND_ICMP_ERROR_V6)
         pCalloutString = "BASIC_ACTION_RANDOM_AT_INBOUND_ICMP_ERROR_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_OUTBOUND_ICMP_ERROR_V4)
         pCalloutString = "BASIC_ACTION_RANDOM_AT_OUTBOUND_ICMP_ERROR_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_OUTBOUND_ICMP_ERROR_V6)
         pCalloutString = "BASIC_ACTION_RANDOM_AT_OUTBOUND_ICMP_ERROR_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_ALE_RESOURCE_ASSIGNMENT_V4)
         pCalloutString = "BASIC_ACTION_RANDOM_AT_ALE_RESOURCE_ASSIGNMENT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_ALE_RESOURCE_ASSIGNMENT_V6)
         pCalloutString = "BASIC_ACTION_RANDOM_AT_ALE_RESOURCE_ASSIGNMENT_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_ALE_AUTH_LISTEN_V4)
         pCalloutString = "BASIC_ACTION_RANDOM_AT_ALE_AUTH_LISTEN_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_ALE_AUTH_LISTEN_V6)
         pCalloutString = "BASIC_ACTION_RANDOM_AT_ALE_AUTH_LISTEN_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_ALE_AUTH_RECV_ACCEPT_V4)
         pCalloutString = "BASIC_ACTION_RANDOM_AT_ALE_AUTH_RECV_ACCEPT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_ALE_AUTH_RECV_ACCEPT_V6)
         pCalloutString = "BASIC_ACTION_RANDOM_AT_ALE_AUTH_RECV_ACCEPT_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_ALE_AUTH_CONNECT_V4)
         pCalloutString = "BASIC_ACTION_RANDOM_AT_ALE_AUTH_CONNECT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_ALE_AUTH_CONNECT_V6)
         pCalloutString = "BASIC_ACTION_RANDOM_AT_ALE_AUTH_CONNECT_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_ALE_FLOW_ESTABLISHED_V4)
         pCalloutString = "BASIC_ACTION_RANDOM_AT_ALE_FLOW_ESTABLISHED_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_ALE_FLOW_ESTABLISHED_V6)
         pCalloutString = "BASIC_ACTION_RANDOM_AT_ALE_FLOW_ESTABLISHED_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_INBOUND_MAC_FRAME_ETHERNET)
         pCalloutString = "BASIC_ACTION_RANDOM_AT_INBOUND_MAC_FRAME_ETHERNET";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_OUTBOUND_MAC_FRAME_ETHERNET)
         pCalloutString = "BASIC_ACTION_RANDOM_AT_OUTBOUND_MAC_FRAME_ETHERNET";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_ALE_RESOURCE_RELEASE_V4)
         pCalloutString = "BASIC_ACTION_RANDOM_AT_ALE_RESOURCE_RELEASE_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_ALE_RESOURCE_RELEASE_V6)
         pCalloutString = "BASIC_ACTION_RANDOM_AT_ALE_RESOURCE_RELEASE_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_ALE_ENDPOINT_CLOSURE_V4)
         pCalloutString = "BASIC_ACTION_RANDOM_AT_ALE_ENDPOINT_CLOSURE_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_ALE_ENDPOINT_CLOSURE_V6)
         pCalloutString = "BASIC_ACTION_RANDOM_AT_ALE_ENDPOINT_CLOSURE_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_STREAM_PACKET_V4)
         pCalloutString = "BASIC_ACTION_RANDOM_AT_STREAM_PACKET_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_STREAM_PACKET_V6)
         pCalloutString = "BASIC_ACTION_RANDOM_AT_STREAM_PACKET_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_INBOUND_MAC_FRAME_NATIVE)
         pCalloutString = "BASIC_ACTION_RANDOM_AT_INBOUND_MAC_FRAME_NATIVE";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_OUTBOUND_MAC_FRAME_NATIVE)
         pCalloutString = "BASIC_ACTION_RANDOM_AT_OUTBOUND_MAC_FRAME_NATIVE";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_INGRESS_VSWITCH_ETHERNET)
         pCalloutString = "BASIC_ACTION_RANDOM_AT_INGRESS_VSWITCH_ETHERNET";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_EGRESS_VSWITCH_ETHERNET)
         pCalloutString = "BASIC_ACTION_RANDOM_AT_EGRESS_VSWITCH_ETHERNET";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_INGRESS_VSWITCH_TRANSPORT_V4)
         pCalloutString = "BASIC_ACTION_RANDOM_AT_INGRESS_VSWITCH_TRANSPORT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_INGRESS_VSWITCH_TRANSPORT_V6)
         pCalloutString = "BASIC_ACTION_RANDOM_AT_INGRESS_VSWITCH_TRANSPORT_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_EGRESS_VSWITCH_TRANSPORT_V4)
         pCalloutString = "BASIC_ACTION_RANDOM_AT_EGRESS_VSWITCH_TRANSPORT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_EGRESS_VSWITCH_TRANSPORT_V6)
         pCalloutString = "BASIC_ACTION_RANDOM_AT_EGRESS_VSWITCH_TRANSPORT_V6";
   }
   else if(RtlCompareMemory(&WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION,
                            pCalloutKey,
                            NUM_MASKED_BYTES))
   {
      if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_INBOUND_IPPACKET_V4)
         pCalloutString = "BASIC_PACKET_EXAMINATION_AT_INBOUND_IPPACKET_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_INBOUND_IPPACKET_V6)
         pCalloutString = "BASIC_PACKET_EXAMINATION_AT_INBOUND_IPPACKET_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_OUTBOUND_IPPACKET_V4)
         pCalloutString = "BASIC_PACKET_EXAMINATION_AT_OUTBOUND_IPPACKET_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_OUTBOUND_IPPACKET_V6)
         pCalloutString = "BASIC_PACKET_EXAMINATION_AT_OUTBOUND_IPPACKET_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_IPFORWARD_V4)
         pCalloutString = "BASIC_PACKET_EXAMINATION_AT_IPFORWARD_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_IPFORWARD_V6)
         pCalloutString = "BASIC_PACKET_EXAMINATION_AT_IPFORWARD_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_INBOUND_TRANSPORT_V4)
         pCalloutString = "BASIC_PACKET_EXAMINATION_AT_INBOUND_TRANSPORT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_INBOUND_TRANSPORT_V6)
         pCalloutString = "BASIC_PACKET_EXAMINATION_AT_INBOUND_TRANSPORT_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_OUTBOUND_TRANSPORT_V4)
         pCalloutString = "BASIC_PACKET_EXAMINATION_AT_OUTBOUND_TRANSPORT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_OUTBOUND_TRANSPORT_V6)
         pCalloutString = "BASIC_PACKET_EXAMINATION_AT_OUTBOUND_TRANSPORT_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_DATAGRAM_DATA_V4)
         pCalloutString = "BASIC_PACKET_EXAMINATION_AT_DATAGRAM_DATA_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_DATAGRAM_DATA_V6)
         pCalloutString = "BASIC_PACKET_EXAMINATION_AT_DATAGRAM_DATA_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_INBOUND_ICMP_ERROR_V4)
         pCalloutString = "BASIC_PACKET_EXAMINATION_AT_INBOUND_ICMP_ERROR_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_INBOUND_ICMP_ERROR_V6)
         pCalloutString = "BASIC_PACKET_EXAMINATION_AT_INBOUND_ICMP_ERROR_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_OUTBOUND_ICMP_ERROR_V4)
         pCalloutString = "BASIC_PACKET_EXAMINATION_AT_OUTBOUND_ICMP_ERROR_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_OUTBOUND_ICMP_ERROR_V6)
         pCalloutString = "BASIC_PACKET_EXAMINATION_AT_OUTBOUND_ICMP_ERROR_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_ALE_AUTH_RECV_ACCEPT_V4)
         pCalloutString = "BASIC_PACKET_EXAMINATION_AT_ALE_AUTH_RECV_ACCEPT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_ALE_AUTH_RECV_ACCEPT_V6)
         pCalloutString = "BASIC_PACKET_EXAMINATION_AT_ALE_AUTH_RECV_ACCEPT_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_ALE_AUTH_CONNECT_V4)
         pCalloutString = "BASIC_PACKET_EXAMINATION_AT_ALE_AUTH_CONNECT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_ALE_AUTH_CONNECT_V6)
         pCalloutString = "BASIC_PACKET_EXAMINATION_AT_ALE_AUTH_CONNECT_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_ALE_FLOW_ESTABLISHED_V4)
         pCalloutString = "BASIC_PACKET_EXAMINATION_AT_ALE_FLOW_ESTABLISHED_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_ALE_FLOW_ESTABLISHED_V6)
         pCalloutString = "BASIC_PACKET_EXAMINATION_AT_ALE_FLOW_ESTABLISHED_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_STREAM_PACKET_V4)
         pCalloutString = "BASIC_PACKET_EXAMINATION_AT_STREAM_PACKET_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_STREAM_PACKET_V6)
         pCalloutString = "BASIC_PACKET_EXAMINATION_AT_STREAM_PACKET_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_INBOUND_MAC_FRAME_ETHERNET)
         pCalloutString = "BASIC_PACKET_EXAMINATION_AT_INBOUND_MAC_FRAME_ETHERNET";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_OUTBOUND_MAC_FRAME_ETHERNET)
         pCalloutString = "BASIC_PACKET_EXAMINATION_AT_OUTBOUND_MAC_FRAME_ETHERNET";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_INBOUND_MAC_FRAME_NATIVE)
         pCalloutString = "BASIC_PACKET_EXAMINATION_AT_INBOUND_MAC_FRAME_NATIVE";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_OUTBOUND_MAC_FRAME_NATIVE)
         pCalloutString = "BASIC_PACKET_EXAMINATION_AT_OUTBOUND_MAC_FRAME_NATIVE";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_INGRESS_VSWITCH_ETHERNET)
         pCalloutString = "BASIC_PACKET_EXAMINATION_AT_INGRESS_VSWITCH_ETHERNET";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_EGRESS_VSWITCH_ETHERNET)
         pCalloutString = "BASIC_PACKET_EXAMINATION_AT_EGRESS_VSWITCH_ETHERNET";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_INGRESS_VSWITCH_TRANSPORT_V4)
         pCalloutString = "BASIC_PACKET_EXAMINATION_AT_INGRESS_VSWITCH_TRANSPORT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_INGRESS_VSWITCH_TRANSPORT_V6)
         pCalloutString = "BASIC_PACKET_EXAMINATION_AT_INGRESS_VSWITCH_TRANSPORT_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_EGRESS_VSWITCH_TRANSPORT_V4)
         pCalloutString = "BASIC_PACKET_EXAMINATION_AT_EGRESS_VSWITCH_TRANSPORT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_EGRESS_VSWITCH_TRANSPORT_V6)
         pCalloutString = "BASIC_PACKET_EXAMINATION_AT_EGRESS_VSWITCH_TRANSPORT_V6";
   }
   else if(RtlCompareMemory(&WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION,
                            pCalloutKey,
                            NUM_MASKED_BYTES))
   {
      if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_INBOUND_IPPACKET_V4)
         pCalloutString = "BASIC_PACKET_INJECTION_AT_INBOUND_IPPACKET_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_INBOUND_IPPACKET_V6)
         pCalloutString = "BASIC_PACKET_INJECTION_AT_INBOUND_IPPACKET_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_OUTBOUND_IPPACKET_V4)
         pCalloutString = "BASIC_PACKET_INJECTION_AT_OUTBOUND_IPPACKET_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_OUTBOUND_IPPACKET_V6)
         pCalloutString = "BASIC_PACKET_INJECTION_AT_OUTBOUND_IPPACKET_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_IPFORWARD_V4)
         pCalloutString = "BASIC_PACKET_INJECTION_AT_IPFORWARD_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_IPFORWARD_V6)
         pCalloutString = "BASIC_PACKET_INJECTION_AT_IPFORWARD_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_INBOUND_TRANSPORT_V4)
         pCalloutString = "BASIC_PACKET_INJECTION_AT_INBOUND_TRANSPORT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_INBOUND_TRANSPORT_V6)
         pCalloutString = "BASIC_PACKET_INJECTION_AT_INBOUND_TRANSPORT_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_OUTBOUND_TRANSPORT_V4)
         pCalloutString = "BASIC_PACKET_INJECTION_AT_OUTBOUND_TRANSPORT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_OUTBOUND_TRANSPORT_V6)
         pCalloutString = "BASIC_PACKET_INJECTION_AT_OUTBOUND_TRANSPORT_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_DATAGRAM_DATA_V4)
         pCalloutString = "BASIC_PACKET_INJECTION_AT_DATAGRAM_DATA_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_DATAGRAM_DATA_V6)
         pCalloutString = "BASIC_PACKET_INJECTION_AT_DATAGRAM_DATA_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_INBOUND_ICMP_ERROR_V4)
         pCalloutString = "BASIC_PACKET_INJECTION_AT_INBOUND_ICMP_ERROR_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_INBOUND_ICMP_ERROR_V6)
         pCalloutString = "BASIC_PACKET_INJECTION_AT_INBOUND_ICMP_ERROR_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_OUTBOUND_ICMP_ERROR_V4)
         pCalloutString = "BASIC_PACKET_INJECTION_AT_OUTBOUND_ICMP_ERROR_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_OUTBOUND_ICMP_ERROR_V6)
         pCalloutString = "BASIC_PACKET_INJECTION_AT_OUTBOUND_ICMP_ERROR_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_ALE_AUTH_RECV_ACCEPT_V4)
         pCalloutString = "BASIC_PACKET_INJECTION_AT_ALE_AUTH_RECV_ACCEPT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_ALE_AUTH_RECV_ACCEPT_V6)
         pCalloutString = "BASIC_PACKET_INJECTION_AT_ALE_AUTH_RECV_ACCEPT_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_ALE_AUTH_CONNECT_V4)
         pCalloutString = "BASIC_PACKET_INJECTION_AT_ALE_AUTH_CONNECT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_ALE_AUTH_CONNECT_V6)
         pCalloutString = "BASIC_PACKET_INJECTION_AT_ALE_AUTH_CONNECT_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_ALE_FLOW_ESTABLISHED_V4)
         pCalloutString = "BASIC_PACKET_INJECTION_AT_ALE_FLOW_ESTABLISHED_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_ALE_FLOW_ESTABLISHED_V6)
         pCalloutString = "BASIC_PACKET_INJECTION_AT_ALE_FLOW_ESTABLISHED_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_STREAM_PACKET_V4)
         pCalloutString = "BASIC_PACKET_INJECTION_AT_STREAM_PACKET_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_STREAM_PACKET_V6)
         pCalloutString = "BASIC_PACKET_INJECTION_AT_STREAM_PACKET_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_INBOUND_MAC_FRAME_ETHERNET)
         pCalloutString = "BASIC_PACKET_INJECTION_AT_INBOUND_MAC_FRAME_ETHERNET";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_OUTBOUND_MAC_FRAME_ETHERNET)
         pCalloutString = "BASIC_PACKET_INJECTION_AT_OUTBOUND_MAC_FRAME_ETHERNET";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_INBOUND_MAC_FRAME_NATIVE)
         pCalloutString = "BASIC_PACKET_INJECTION_AT_INBOUND_MAC_FRAME_NATIVE";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_OUTBOUND_MAC_FRAME_NATIVE)
         pCalloutString = "BASIC_PACKET_INJECTION_AT_OUTBOUND_MAC_FRAME_NATIVE";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_INGRESS_VSWITCH_ETHERNET)
         pCalloutString = "BASIC_PACKET_INJECTION_AT_INGRESS_VSWITCH_ETHERNET";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_EGRESS_VSWITCH_ETHERNET)
         pCalloutString = "BASIC_PACKET_INJECTION_AT_EGRESS_VSWITCH_ETHERNET";
   }
   else if(RtlCompareMemory(&WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION,
                            pCalloutKey,
                            NUM_MASKED_BYTES))
   {
      if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_INBOUND_IPPACKET_V4)
         pCalloutString = "BASIC_PACKET_MODIFICATION_AT_INBOUND_IPPACKET_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_INBOUND_IPPACKET_V6)
         pCalloutString = "BASIC_PACKET_MODIFICATION_AT_INBOUND_IPPACKET_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_OUTBOUND_IPPACKET_V4)
         pCalloutString = "BASIC_PACKET_MODIFICATION_AT_OUTBOUND_IPPACKET_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_OUTBOUND_IPPACKET_V6)
         pCalloutString = "BASIC_PACKET_MODIFICATION_AT_OUTBOUND_IPPACKET_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_IPFORWARD_V4)
         pCalloutString = "BASIC_PACKET_MODIFICATION_AT_IPFORWARD_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_IPFORWARD_V6)
         pCalloutString = "BASIC_PACKET_MODIFICATION_AT_IPFORWARD_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_INBOUND_TRANSPORT_V4)
         pCalloutString = "BASIC_PACKET_MODIFICATION_AT_INBOUND_TRANSPORT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_INBOUND_TRANSPORT_V6)
         pCalloutString = "BASIC_PACKET_MODIFICATION_AT_INBOUND_TRANSPORT_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_OUTBOUND_TRANSPORT_V4)
         pCalloutString = "BASIC_PACKET_MODIFICATION_AT_OUTBOUND_TRANSPORT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_OUTBOUND_TRANSPORT_V6)
         pCalloutString = "BASIC_PACKET_MODIFICATION_AT_OUTBOUND_TRANSPORT_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_DATAGRAM_DATA_V4)
         pCalloutString = "BASIC_PACKET_MODIFICATION_AT_DATAGRAM_DATA_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_DATAGRAM_DATA_V6)
         pCalloutString = "BASIC_PACKET_MODIFICATION_AT_DATAGRAM_DATA_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_INBOUND_ICMP_ERROR_V4)
         pCalloutString = "BASIC_PACKET_MODIFICATION_AT_INBOUND_ICMP_ERROR_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_INBOUND_ICMP_ERROR_V6)
         pCalloutString = "BASIC_PACKET_MODIFICATION_AT_INBOUND_ICMP_ERROR_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_OUTBOUND_ICMP_ERROR_V4)
         pCalloutString = "BASIC_PACKET_MODIFICATION_AT_OUTBOUND_ICMP_ERROR_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_OUTBOUND_ICMP_ERROR_V6)
         pCalloutString = "BASIC_PACKET_MODIFICATION_AT_OUTBOUND_ICMP_ERROR_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_ALE_AUTH_RECV_ACCEPT_V4)
         pCalloutString = "BASIC_PACKET_MODIFICATION_AT_ALE_AUTH_RECV_ACCEPT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_ALE_AUTH_RECV_ACCEPT_V6)
         pCalloutString = "BASIC_PACKET_MODIFICATION_AT_ALE_AUTH_RECV_ACCEPT_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_ALE_AUTH_CONNECT_V4)
         pCalloutString = "BASIC_PACKET_MODIFICATION_AT_ALE_AUTH_CONNECT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_ALE_AUTH_CONNECT_V6)
         pCalloutString = "BASIC_PACKET_MODIFICATION_AT_ALE_AUTH_CONNECT_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_ALE_FLOW_ESTABLISHED_V4)
         pCalloutString = "BASIC_PACKET_MODIFICATION_AT_ALE_FLOW_ESTABLISHED_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_ALE_FLOW_ESTABLISHED_V6)
         pCalloutString = "BASIC_PACKET_MODIFICATION_AT_ALE_FLOW_ESTABLISHED_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_STREAM_PACKET_V4)
         pCalloutString = "BASIC_PACKET_MODIFICATION_AT_STREAM_PACKET_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_STREAM_PACKET_V6)
         pCalloutString = "BASIC_PACKET_MODIFICATION_AT_STREAM_PACKET_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_INBOUND_MAC_FRAME_ETHERNET)
         pCalloutString = "BASIC_PACKET_MODIFICATION_AT_INBOUND_MAC_FRAME_ETHERNET";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_OUTBOUND_MAC_FRAME_ETHERNET)
         pCalloutString = "BASIC_PACKET_MODIFICATION_AT_OUTBOUND_MAC_FRAME_ETHERNET";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_INBOUND_MAC_FRAME_NATIVE)
         pCalloutString = "BASIC_PACKET_MODIFICATION_AT_INBOUND_MAC_FRAME_NATIVE";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_INBOUND_MAC_FRAME_NATIVE)
         pCalloutString = "BASIC_PACKET_MODIFICATION_AT_OUTBOUND_MAC_FRAME_NATIVE";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_INGRESS_VSWITCH_ETHERNET)
         pCalloutString = "BASIC_PACKET_MODIFICATION_AT_INGRESS_VSWITCH_ETHERNET";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_EGRESS_VSWITCH_ETHERNET)
         pCalloutString = "BASIC_PACKET_MODIFICATION_AT_EGRESS_VSWITCH_ETHERNET";
   }
   else if(RtlCompareMemory(&WFPSAMPLER_CALLOUT_BASIC_STREAM_INJECTION,
                            pCalloutKey,
                            NUM_MASKED_BYTES))
   {
      if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_STREAM_INJECTION_AT_STREAM_V4)
         pCalloutString = "BASIC_STREAM_INJECTION_AT_STREAM_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_BASIC_STREAM_INJECTION_AT_STREAM_V6)
         pCalloutString = "BASIC_STREAM_INJECTION_AT_STREAM_V6";
   }
   else if(RtlCompareMemory(&WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION,
                            pCalloutKey,
                            NUM_MASKED_BYTES))
   {
      if(pCalloutKey == &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_INBOUND_IPPACKET_V4)
         pCalloutString = "FAST_PACKET_INJECTION_AT_INBOUND_IPPACKET_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_INBOUND_IPPACKET_V6)
         pCalloutString = "FAST_PACKET_INJECTION_AT_INBOUND_IPPACKET_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_OUTBOUND_IPPACKET_V4)
         pCalloutString = "FAST_PACKET_INJECTION_AT_OUTBOUND_IPPACKET_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_OUTBOUND_IPPACKET_V6)
         pCalloutString = "FAST_PACKET_INJECTION_AT_OUTBOUND_IPPACKET_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_IPFORWARD_V4)
         pCalloutString = "FAST_PACKET_INJECTION_AT_IPFORWARD_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_IPFORWARD_V6)
         pCalloutString = "FAST_PACKET_INJECTION_AT_IPFORWARD_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_INBOUND_TRANSPORT_V4)
         pCalloutString = "FAST_PACKET_INJECTION_AT_INBOUND_TRANSPORT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_INBOUND_TRANSPORT_V6)
         pCalloutString = "FAST_PACKET_INJECTION_AT_INBOUND_TRANSPORT_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_OUTBOUND_TRANSPORT_V4)
         pCalloutString = "FAST_PACKET_INJECTION_AT_OUTBOUND_TRANSPORT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_OUTBOUND_TRANSPORT_V6)
         pCalloutString = "FAST_PACKET_INJECTION_AT_OUTBOUND_TRANSPORT_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_DATAGRAM_DATA_V4)
         pCalloutString = "FAST_PACKET_INJECTION_AT_DATAGRAM_DATA_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_DATAGRAM_DATA_V6)
         pCalloutString = "FAST_PACKET_INJECTION_AT_DATAGRAM_DATA_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_INBOUND_ICMP_ERROR_V4)
         pCalloutString = "FAST_PACKET_INJECTION_AT_INBOUND_ICMP_ERROR_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_INBOUND_ICMP_ERROR_V6)
         pCalloutString = "FAST_PACKET_INJECTION_AT_INBOUND_ICMP_ERROR_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_OUTBOUND_ICMP_ERROR_V4)
         pCalloutString = "FAST_PACKET_INJECTION_AT_OUTBOUND_ICMP_ERROR_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_OUTBOUND_ICMP_ERROR_V6)
         pCalloutString = "FAST_PACKET_INJECTION_AT_OUTBOUND_ICMP_ERROR_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_ALE_AUTH_RECV_ACCEPT_V4)
         pCalloutString = "FAST_PACKET_INJECTION_AT_ALE_AUTH_RECV_ACCEPT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_ALE_AUTH_RECV_ACCEPT_V6)
         pCalloutString = "FAST_PACKET_INJECTION_AT_ALE_AUTH_RECV_ACCEPT_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_ALE_AUTH_CONNECT_V4)
         pCalloutString = "FAST_PACKET_INJECTION_AT_ALE_AUTH_CONNECT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_ALE_AUTH_CONNECT_V6)
         pCalloutString = "FAST_PACKET_INJECTION_AT_ALE_AUTH_CONNECT_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_ALE_FLOW_ESTABLISHED_V4)
         pCalloutString = "FAST_PACKET_INJECTION_AT_ALE_FLOW_ESTABLISHED_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_ALE_FLOW_ESTABLISHED_V6)
         pCalloutString = "FAST_PACKET_INJECTION_AT_ALE_FLOW_ESTABLISHED_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_STREAM_PACKET_V4)
         pCalloutString = "FAST_PACKET_INJECTION_AT_STREAM_PACKET_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_STREAM_PACKET_V6)
         pCalloutString = "FAST_PACKET_INJECTION_AT_STREAM_PACKET_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_INBOUND_MAC_FRAME_ETHERNET)
         pCalloutString = "FAST_PACKET_INJECTION_AT_INBOUND_MAC_FRAME_ETHERNET";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_OUTBOUND_MAC_FRAME_ETHERNET)
         pCalloutString = "FAST_PACKET_INJECTION_AT_OUTBOUND_MAC_FRAME_ETHERNET";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_INBOUND_MAC_FRAME_NATIVE)
         pCalloutString = "FAST_PACKET_INJECTION_AT_INBOUND_MAC_FRAME_NATIVE";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_OUTBOUND_MAC_FRAME_NATIVE)
         pCalloutString = "FAST_PACKET_INJECTION_AT_OUTBOUND_MAC_FRAME_NATIVE";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_INGRESS_VSWITCH_ETHERNET)
         pCalloutString = "FAST_PACKET_INJECTION_AT_INGRESS_VSWITCH_ETHERNET";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_EGRESS_VSWITCH_ETHERNET)
         pCalloutString = "FAST_PACKET_INJECTION_AT_EGRESS_VSWITCH_ETHERNET";
   }
   else if(RtlCompareMemory(&WFPSAMPLER_CALLOUT_FAST_STREAM_INJECTION,
                            pCalloutKey,
                            NUM_MASKED_BYTES))
   {
      if(pCalloutKey == &WFPSAMPLER_CALLOUT_FAST_STREAM_INJECTION_AT_STREAM_V4)
         pCalloutString = "FAST_STREAM_INJECTION_AT_STREAM_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_FAST_STREAM_INJECTION_AT_STREAM_V6)
         pCalloutString = "FAST_STREAM_INJECTION_AT_STREAM_V6";
   }
   else if(RtlCompareMemory(&WFPSAMPLER_CALLOUT_PEND_AUTHORIZATION,
                            pCalloutKey,
                            NUM_MASKED_BYTES))
   {
      if(pCalloutKey == &WFPSAMPLER_CALLOUT_PEND_AUTHORIZATION_AT_ALE_RESOURCE_ASSIGNMENT_V4)
         pCalloutString = "PEND_AUTHORIZATION_AT_ALE_RESOURCE_ASSIGNMENT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_PEND_AUTHORIZATION_AT_ALE_RESOURCE_ASSIGNMENT_V6)
         pCalloutString = "PEND_AUTHORIZATION_AT_ALE_RESOURCE_ASSIGNMENT_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_PEND_AUTHORIZATION_AT_ALE_AUTH_LISTEN_V4)
         pCalloutString = "PEND_AUTHORIZATION_AT_ALE_AUTH_LISTEN_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_PEND_AUTHORIZATION_AT_ALE_AUTH_LISTEN_V6)
         pCalloutString = "PEND_AUTHORIZATION_AT_ALE_AUTH_LISTEN_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_PEND_AUTHORIZATION_AT_ALE_AUTH_RECV_ACCEPT_V4)
         pCalloutString = "PEND_AUTHORIZATION_AT_ALE_AUTH_RECV_ACCEPT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_PEND_AUTHORIZATION_AT_ALE_AUTH_RECV_ACCEPT_V6)
         pCalloutString = "PEND_AUTHORIZATION_AT_ALE_AUTH_RECV_ACCEPT_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_PEND_AUTHORIZATION_AT_ALE_AUTH_CONNECT_V4)
         pCalloutString = "PEND_AUTHORIZATION_AT_ALE_AUTH_CONNECT_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_PEND_AUTHORIZATION_AT_ALE_AUTH_CONNECT_V6)
         pCalloutString = "PEND_AUTHORIZATION_AT_ALE_AUTH_CONNECT_V6";
   }
   else if(RtlCompareMemory(&WFPSAMPLER_CALLOUT_PEND_ENDPOINT_CLOSURE,
                            pCalloutKey,
                            NUM_MASKED_BYTES))
   {
      if(pCalloutKey == &WFPSAMPLER_CALLOUT_PEND_ENDPOINT_CLOSURE_AT_ALE_ENDPOINT_CLOSURE_V4)
         pCalloutString = "PEND_ENDPOINT_CLOSURE_AT_ALE_RESOURCE_ASSIGNMENT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_PEND_ENDPOINT_CLOSURE_AT_ALE_ENDPOINT_CLOSURE_V6)
         pCalloutString = "PEND_ENDPOINT_CLOSURE_AT_ALE_ENDPOINT_CLOSURE_V6";
   }
   else if(RtlCompareMemory(&WFPSAMPLER_CALLOUT_PROXY_BY_ALE_REDIRECT,
                            pCalloutKey,
                            NUM_MASKED_BYTES))
   {
      if(pCalloutKey == &WFPSAMPLER_CALLOUT_PROXY_BY_ALE_AT_CONNECT_REDIRECT_V4)
         pCalloutString = "PROXY_BY_ALE_AT_CONNECT_REDIRECT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_PROXY_BY_ALE_AT_CONNECT_REDIRECT_V6)
         pCalloutString = "PROXY_BY_ALE_AT_CONNECT_REDIRECT_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_PROXY_BY_ALE_AT_BIND_REDIRECT_V4)
         pCalloutString = "PROXY_BY_ALE_AT_BIND_REDIRECT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_PROXY_BY_ALE_AT_BIND_REDIRECT_V6)
         pCalloutString = "PROXY_BY_ALE_AT_BIND_REDIRECT_V6";
   }
   else if(RtlCompareMemory(&WFPSAMPLER_CALLOUT_PROXY_BY_INJECTION,
                            pCalloutKey,
                            NUM_MASKED_BYTES))
   {
      if(pCalloutKey == &WFPSAMPLER_CALLOUT_PROXY_BY_INJECTION_AT_INBOUND_IPPACKET_V4)
         pCalloutString = "PROXY_BY_INJECTION_AT_INBOUND_IPPACKET_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_PROXY_BY_INJECTION_AT_INBOUND_IPPACKET_V6)
         pCalloutString = "PROXY_BY_INJECTION_AT_INBOUND_IPPACKET_V6";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_PROXY_BY_INJECTION_AT_OUTBOUND_TRANSPORT_V4)
         pCalloutString = "PROXY_BY_INJECTION_AT_OUTBOUND_TRANSPORT_V4";
      else if(pCalloutKey == &WFPSAMPLER_CALLOUT_PROXY_BY_INJECTION_AT_OUTBOUND_TRANSPORT_V6)
         pCalloutString = "PROXY_BY_INJECTION_AT_INBOUND_TRANSPORT_V6";
   }

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprExposedCalloutToString()\n");

#endif /// DBG
   
   return pCalloutString;
}
