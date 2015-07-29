////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      CompletionFunctions_BasicPacketInjectionCallouts.cpp
//
//   Abstract:
//      This module contains WFP Completion functions for injecting packets back into the data path 
//         using the clone / block / inject method.
//
//   Naming Convention:
//
//      <Module><Scenario>
//  
//      i.e.
//       CompleteBasicPacketInjection
//
//       <Module>
//          Complete                           - Function is an FWPS_INJECT_COMPLETE function.
//       <Scenario>
//          BasicPacketInjection               - Function demonstrates the clone / block / inject 
//                                               model.
//
//      <Object><Action>
//
//      i.e.
//       BasicPacketInjectionCompletionDataDestroy
//
//       <Object>
//        {
//          BasicPacketInjectionCompletionData - pertains to BASIC_PACKET_INJECTION_COMPLETION_DATA.
//        }
//       <Action>
//          Destroy                            - Cleans up and frees all memory in the object.
//
//   Private Functions:
//      BasicPacketInjectionCompletionDataDestroy(),
//
//   Public Functions:
//      CompleteBasicPacketInjection(),
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Enhance function declaration for IntelliSense, add 
//                                              support for multiple injectors, and enhance traces.
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Framework_WFPSamplerCalloutDriver.h"                  /// .
#include "CompletionFunctions_BasicPacketInjectionCallouts.tmh" /// $(OBJ_PATH)\$(O)\

#if DBG

INJECTION_COUNTERS g_bpiTotalCompletions = {0};

/**
 @function="BasicPacketInjectionCountersDecrement"
 
   Purpose:  Deccrement the appropriate counters based on the injection handle.                 <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Desktop/MS683581.aspx                      <br>
*/
VOID BasicPacketInjectionCountersDecrement(_In_ HANDLE injectionHandle,
                                           _Inout_ INJECTION_COUNTERS* pCounters)
{
   if(injectionHandle == g_pIPv4InboundNetworkInjectionHandles[0] ||
      injectionHandle == g_pIPv4InboundNetworkInjectionHandles[1])
      InterlockedDecrement64((LONG64*)&(pCounters->inboundNetwork_IPv4));
   else if(injectionHandle == g_pIPv6InboundNetworkInjectionHandles[0] ||
           injectionHandle == g_pIPv6InboundNetworkInjectionHandles[1])
      InterlockedDecrement64((LONG64*)&(pCounters->inboundNetwork_IPv6));
   else if(injectionHandle == g_pIPv4OutboundNetworkInjectionHandles[0] ||
           injectionHandle == g_pIPv4OutboundNetworkInjectionHandles[1])
      InterlockedDecrement64((LONG64*)&(pCounters->outboundNetwork_IPv4));
   else if(injectionHandle == g_pIPv6OutboundNetworkInjectionHandles[0] ||
           injectionHandle == g_pIPv6OutboundNetworkInjectionHandles[1])
      InterlockedDecrement64((LONG64*)&(pCounters->outboundNetwork_IPv6));
   else if(injectionHandle == g_pIPv4InboundForwardInjectionHandles[0] ||
           injectionHandle == g_pIPv4InboundForwardInjectionHandles[1])
      InterlockedDecrement64((LONG64*)&(pCounters->inboundForward_IPv4));
   else if(injectionHandle == g_pIPv6InboundForwardInjectionHandles[0] ||
           injectionHandle == g_pIPv6InboundForwardInjectionHandles[1])
      InterlockedDecrement64((LONG64*)&(pCounters->inboundForward_IPv6));
   else if(injectionHandle == g_pIPv4OutboundForwardInjectionHandles[0] ||
           injectionHandle == g_pIPv4OutboundForwardInjectionHandles[1])
      InterlockedDecrement64((LONG64*)&(pCounters->outboundForward_IPv4));
   else if(injectionHandle == g_pIPv6OutboundForwardInjectionHandles[0] ||
           injectionHandle == g_pIPv6OutboundForwardInjectionHandles[1])
      InterlockedDecrement64((LONG64*)&(pCounters->outboundForward_IPv6));
   else if(injectionHandle == g_pIPv4InboundTransportInjectionHandles[0] ||
           injectionHandle == g_pIPv4InboundTransportInjectionHandles[1])
      InterlockedDecrement64((LONG64*)&(pCounters->inboundTransport_IPv4));
   else if(injectionHandle == g_pIPv6InboundTransportInjectionHandles[0] ||
           injectionHandle == g_pIPv6InboundTransportInjectionHandles[1])
      InterlockedDecrement64((LONG64*)&(pCounters->inboundTransport_IPv6));
   else if(injectionHandle == g_pIPv4OutboundTransportInjectionHandles[0] ||
           injectionHandle == g_pIPv4OutboundTransportInjectionHandles[1])
      InterlockedDecrement64((LONG64*)&(pCounters->outboundTransport_IPv4));
   else if(injectionHandle == g_pIPv6OutboundTransportInjectionHandles[0] ||
           injectionHandle == g_pIPv6OutboundTransportInjectionHandles[1])
      InterlockedDecrement64((LONG64*)&(pCounters->outboundTransport_IPv6));

#if(NTDDI_VERSION >= NTDDI_WIN8)

   else if(injectionHandle == g_pIPv4InboundMACInjectionHandles[0] ||
           injectionHandle == g_pIPv4InboundMACInjectionHandles[1])
      InterlockedDecrement64((LONG64*)&(pCounters->inboundMAC_IPv4));
   else if(injectionHandle == g_pIPv6InboundMACInjectionHandles[0] ||
           injectionHandle == g_pIPv6InboundMACInjectionHandles[1])
      InterlockedDecrement64((LONG64*)&(pCounters->inboundMAC_IPv6));
   else if(injectionHandle == g_pInboundMACInjectionHandles[0] ||
           injectionHandle == g_pInboundMACInjectionHandles[1])
      InterlockedDecrement64((LONG64*)&(pCounters->inboundMAC_Unknown));
   else if(injectionHandle == g_pIPv4OutboundMACInjectionHandles[0] ||
           injectionHandle == g_pIPv4OutboundMACInjectionHandles[1])
      InterlockedDecrement64((LONG64*)&(pCounters->outboundMAC_IPv4));
   else if(injectionHandle == g_pIPv6OutboundMACInjectionHandles[0] ||
           injectionHandle == g_pIPv6OutboundMACInjectionHandles[1])
      InterlockedDecrement64((LONG64*)&(pCounters->outboundMAC_IPv6));
   else if(injectionHandle == g_pOutboundMACInjectionHandles[0] ||
           injectionHandle == g_pOutboundMACInjectionHandles[1])
      InterlockedDecrement64((LONG64*)&(pCounters->outboundMAC_Unknown));
   else if(injectionHandle == g_pIPv4IngressVSwitchEthernetInjectionHandles[0] ||
           injectionHandle == g_pIPv4IngressVSwitchEthernetInjectionHandles[1])
      InterlockedDecrement64((LONG64*)&(pCounters->ingressVSwitch_IPv4));
   else if(injectionHandle == g_pIPv6IngressVSwitchEthernetInjectionHandles[0] ||
           injectionHandle == g_pIPv6IngressVSwitchEthernetInjectionHandles[1])
      InterlockedDecrement64((LONG64*)&(pCounters->ingressVSwitch_IPv6));
   else if(injectionHandle == g_pIngressVSwitchEthernetInjectionHandles[0] ||
           injectionHandle == g_pIngressVSwitchEthernetInjectionHandles[1])
      InterlockedDecrement64((LONG64*)&(pCounters->ingressVSwitch_Unknown));
   else if(injectionHandle == g_pIPv4EgressVSwitchEthernetInjectionHandles[0] ||
           injectionHandle == g_pIPv4EgressVSwitchEthernetInjectionHandles[1])
      InterlockedDecrement64((LONG64*)&(pCounters->egressVSwitch_IPv4));
   else if(injectionHandle == g_pIPv6EgressVSwitchEthernetInjectionHandles[0] ||
           injectionHandle == g_pIPv6EgressVSwitchEthernetInjectionHandles[1])
      InterlockedDecrement64((LONG64*)&(pCounters->egressVSwitch_IPv6));
   else if(injectionHandle == g_pEgressVSwitchEthernetInjectionHandles[0] ||
           injectionHandle == g_pEgressVSwitchEthernetInjectionHandles[1])
      InterlockedDecrement64((LONG64*)&(pCounters->egressVSwitch_Unknown));

#endif /// (NTDDI_VERSION >= NTDDI_WIN8)

   return;
}

#endif /// DBG

/**
 @private_function="BasicPacketInjectionCompletionDataDestroy"
 
   Purpose:                                                                                     <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_At_(*ppCompletionData, _Pre_ _Notnull_)
_At_(*ppCompletionData, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppCompletionData == 0)
VOID BasicPacketInjectionCompletionDataDestroy(_Inout_ BASIC_PACKET_INJECTION_COMPLETION_DATA** ppCompletionData,
                                               _In_ BOOLEAN override)                                             /* FALSE */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> BasicPacketInjectionCompletionDataDestroy()\n");

#endif /// DBG
   
   NT_ASSERT(ppCompletionData);
   NT_ASSERT(*ppCompletionData);

   BASIC_PACKET_INJECTION_COMPLETION_DATA* pCompletionData = *ppCompletionData;
   KIRQL                                   originalIRQL    = PASSIVE_LEVEL;
   INT32                                   refCount        = -1;

   refCount = InterlockedDecrement((LONG*)&(pCompletionData->refCount));
   if(refCount == 0 ||
      override)
   {
      KeAcquireSpinLock(&(pCompletionData->spinLock),
                        &originalIRQL);

      pCompletionData->refCount = -1;

      if(pCompletionData->pClassifyData)
      {
         if(!(pCompletionData->performedInline))
            KrnlHlprClassifyDataDestroyLocalCopy(&(pCompletionData->pClassifyData));
         else
         {
            HLPR_DELETE(pCompletionData->pClassifyData,
                        WFPSAMPLER_CALLOUT_DRIVER_TAG);
         }
      }

      if(pCompletionData->pInjectionData)
         KrnlHlprInjectionDataDestroy(&(pCompletionData->pInjectionData));

      if(pCompletionData->pSendParams)
      {
         HLPR_DELETE_ARRAY(pCompletionData->pSendParams->remoteAddress,
                           WFPSAMPLER_CALLOUT_DRIVER_TAG);

         HLPR_DELETE(pCompletionData->pSendParams,
                     WFPSAMPLER_CALLOUT_DRIVER_TAG);
      }

      KeReleaseSpinLock(&(pCompletionData->spinLock),
                        originalIRQL);

      HLPR_DELETE(*ppCompletionData,
                  WFPSAMPLER_CALLOUT_DRIVER_TAG);
   }

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- BasicPacketInjectionCompletionDataDestroy()\n");

#endif /// DBG

   return;
}

/**
 @completion_function="CompleteBasicPacketInjection"
 
   Purpose: Cleanup injection objects and memory.                                               <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-Us/Library/Windows/Hardware/FF545018.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI CompleteBasicPacketInjection(_In_ VOID* pContext,
                                        _Inout_ NET_BUFFER_LIST* pNetBufferList,
                                        _In_ BOOLEAN dispatchLevel)
{
   NT_ASSERT(pContext);
   NT_ASSERT(((BASIC_PACKET_INJECTION_COMPLETION_DATA*)pContext)->pInjectionData);
   NT_ASSERT(((BASIC_PACKET_INJECTION_COMPLETION_DATA*)pContext)->pClassifyData);
   NT_ASSERT(((BASIC_PACKET_INJECTION_COMPLETION_DATA*)pContext)->pClassifyData->pClassifyValues);
   NT_ASSERT(((BASIC_PACKET_INJECTION_COMPLETION_DATA*)pContext)->pClassifyData->pClassifyOut);
   NT_ASSERT(((BASIC_PACKET_INJECTION_COMPLETION_DATA*)pContext)->pClassifyData->pFilter);
   NT_ASSERT(pNetBufferList);
   NT_ASSERT(NT_SUCCESS(pNetBufferList->Status));

   UNREFERENCED_PARAMETER(dispatchLevel);

   NTSTATUS                                status          = pNetBufferList->Status;
   BASIC_PACKET_INJECTION_COMPLETION_DATA* pCompletionData = (BASIC_PACKET_INJECTION_COMPLETION_DATA*)pContext;
   UINT32                                  layerID         = pCompletionData->pClassifyData->pClassifyValues->layerId;
   UINT64                                  filterID        = pCompletionData->pClassifyData->pFilter->filterId;

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> CompleteBasicPacketInjection() [Layer: %s][FilterID: %#I64x][NBL->Status: %#x]",
              KrnlHlprFwpsLayerIDToString(layerID),
              filterID,
              status);

   if(status != STATUS_SUCCESS)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! CompleteBasicPacketInjection() [status: %#x]\n",
                 status);

   FwpsFreeCloneNetBufferList(pNetBufferList,
                              0);

#if DBG

   BasicPacketInjectionCountersIncrement(pCompletionData->pInjectionData->injectionHandle,
                                      &g_bpiTotalCompletions);

   BasicPacketInjectionCountersDecrement(pCompletionData->pInjectionData->injectionHandle,
                                         &g_bpiOutstandingNBLClones);

#endif // DBG

   BasicPacketInjectionCompletionDataDestroy(&pCompletionData);

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- CompleteBasicPacketInjection() [Layer: %s][FilterID: %#I64x][NBL->Status: %#x]",
              KrnlHlprFwpsLayerIDToString(layerID),
              filterID,
              status);

   return;
}
