////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_InjectionData.cpp
//
//   Abstract:
//      This module contains kernel helper functions that assist with INJECTION_DATA and 
//         INJECTION_HANDLE_DATA.
//
//   Naming Convention:
//
//      <Module><Object><Action>
//  
//      i.e.
//
//       KrnlHlprInjectionDataCreate
//
//       <Module>
//          KrnlHlpr            -       Function is located in syslib\ and applies to kernel mode.
//       <Object>
//          InjectionData       -       Function pertains to INJECTION_DATA objects.
//          InjectionHandleData -       Function pertains to INJECTION_HANDLE_DATA objects.
//       <Action>
//          {
//            Create            -       Function allocates and fills memory.
//            Destroy           -       Function cleans up and frees memory.
//            Populate          -       Function fills memory with values.
//            Purge             -       Function cleans up values.
//          }
//
//   Private Functions:
//
//   Public Functions:
//      KrnlHlprInjectionDataCreate(),
//      KrnlHlprInjectionDataDestroy(),
//      KrnlHlprInjectionDataPopulate(),
//      KrnlHlprInjectionDataPurge(),
//      KrnlHlprInjectionHandleDataCreate(),
//      KrnlHlprInjectionHandleDataDestroy(),
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Enhance annotations, add multiple injector support, add 
//                                              missing layers for injection handles, and add 
//                                              support for controlData.
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "HelperFunctions_Include.h"         /// .
#include "HelperFunctions_InjectionData.tmh" /// $(OBJ_PATH)\$(O)\

#ifndef INJECTION_DATA____
#define INJECTION_DATA____

/**
 @kernel_helper_function="KrnlHlprInjectionDataPurge"
 
   Purpose:  Cleanup a INJECTION_DATA object.                                                   <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprInjectionDataPurge(_Inout_ INJECTION_DATA* pInjectionData)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprInjectionDataPurge()\n");

#endif /// DBG
   
   NT_ASSERT(pInjectionData);

   if(pInjectionData->pControlData)
   {
      HLPR_DELETE_ARRAY(pInjectionData->pControlData,
                        WFPSAMPLER_CALLOUT_DRIVER_TAG);
   }

   RtlZeroMemory(pInjectionData,
                 sizeof(INJECTION_DATA));

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprInjectionDataPurge()\n");

#endif /// DBG
   
   return;
}

/**
 @kernel_helper_function="KrnlHlprInjectionDataDestroy"
 
   Purpose:  Cleanup and free a INJECTION_DATA object.                                          <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_At_(*ppInjectionData, _Pre_ _Notnull_)
_At_(*ppInjectionData, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppInjectionData == 0)
inline VOID KrnlHlprInjectionDataDestroy(_Inout_ INJECTION_DATA** ppInjectionData)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprInjectionDataDestroy()\n");

#endif /// DBG
   
   NT_ASSERT(ppInjectionData);

   if(*ppInjectionData)
   {
      KrnlHlprInjectionDataPurge(*ppInjectionData);

      HLPR_DELETE(*ppInjectionData,
                  WFPSAMPLER_SYSLIB_TAG);
   }

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprInjectionDataDestroy()\n");

#endif /// DBG
   
   return;
}

/**
 @kernel_helper_function="KrnlHlprInjectionDataPopulate"
 
   Purpose:  Populates a INJECTION_DATA object with the data based off values obtained in the 
             classifyFn.                                                                        <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/FF551202.aspx                              <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprInjectionDataPopulate(_Inout_ INJECTION_DATA* pInjectionData,
                                       _In_ const FWPS_INCOMING_VALUES0* pClassifyValues,
                                       _In_ const FWPS_INCOMING_METADATA_VALUES0* pMetadataValues,
                                       _In_opt_ const NET_BUFFER_LIST* pNetBufferList,
                                       _In_ const FWPS_FILTER* pFilter)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprInjectionDataPopulate()\n");

#endif /// DBG
   
   NT_ASSERT(pInjectionData);
   NT_ASSERT(pClassifyValues);
   NT_ASSERT(pMetadataValues);
   NT_ASSERT(pFilter);

   NTSTATUS   status          = STATUS_SUCCESS;
   UINT32     index           = WFPSAMPLER_INDEX;
   FWP_VALUE* pDirectionValue = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                          &FWPM_CONDITION_DIRECTION);

   pInjectionData->direction = KrnlHlprFwpsLayerGetDirection(pClassifyValues->layerId);

   if(pFilter->subLayerWeight == FWPM_SUBLAYER_UNIVERSAL_WEIGHT)
      index = UNIVERSAL_INDEX;

   if(pDirectionValue &&
      pDirectionValue->type == FWP_UINT32)
      pInjectionData->direction = (FWP_DIRECTION)pDirectionValue->uint32;
   else
   {
      if(pClassifyValues->layerId == FWPS_LAYER_IPFORWARD_V4 ||
         pClassifyValues->layerId == FWPS_LAYER_IPFORWARD_V4_DISCARD ||
         pClassifyValues->layerId == FWPS_LAYER_IPFORWARD_V6 ||
         pClassifyValues->layerId == FWPS_LAYER_IPFORWARD_V6_DISCARD)
      {
         if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadataValues,
                                           FWPS_METADATA_FIELD_FORWARD_LAYER_INBOUND_PASS_THRU))
            pInjectionData->direction = FWP_DIRECTION_INBOUND;
         else if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadataValues,
                                                FWPS_METADATA_FIELD_FORWARD_LAYER_OUTBOUND_PASS_THRU))
            pInjectionData->direction = FWP_DIRECTION_OUTBOUND;
      }
      else
      {
         if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadataValues,
                                           FWPS_METADATA_FIELD_PACKET_DIRECTION))
            pInjectionData->direction = pMetadataValues->packetDirection;
      }
   }

   if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadataValues,
                                     FWPS_METADATA_FIELD_TRANSPORT_CONTROL_DATA))
   {
      if(pMetadataValues->controlData &&
         pMetadataValues->controlDataLength)
      {
         HLPR_NEW_ARRAY(pInjectionData->pControlData,
                        BYTE,
                        pMetadataValues->controlDataLength,
                        WFPSAMPLER_CALLOUT_DRIVER_TAG);
         HLPR_BAIL_ON_ALLOC_FAILURE(pInjectionData->pControlData,
                                    status);
   
         RtlCopyMemory(pInjectionData->pControlData,
                       pMetadataValues->controlData,
                       pMetadataValues->controlDataLength);

         pInjectionData->controlDataLength = pMetadataValues->controlDataLength;
      }
   }

   switch(pClassifyValues->layerId)
   {
      case FWPS_LAYER_INBOUND_IPPACKET_V4:
      {
         pInjectionData->addressFamily   = AF_INET;
         pInjectionData->injectionHandle = g_pIPv4InboundNetworkInjectionHandles[index];

         break;
      }
      case FWPS_LAYER_INBOUND_IPPACKET_V6:
      {
         pInjectionData->addressFamily   = AF_INET6;
         pInjectionData->injectionHandle = g_pIPv6InboundNetworkInjectionHandles[index];

         break;
      }
      case FWPS_LAYER_OUTBOUND_IPPACKET_V4:
      {
         pInjectionData->addressFamily   = AF_INET;
         pInjectionData->injectionHandle = g_pIPv4OutboundNetworkInjectionHandles[index];

         break;
      }
      case FWPS_LAYER_OUTBOUND_IPPACKET_V6:
      {
         pInjectionData->addressFamily   = AF_INET6;
         pInjectionData->injectionHandle = g_pIPv6OutboundNetworkInjectionHandles[index];

         break;
      }
      case FWPS_LAYER_IPFORWARD_V4:
      {
         pInjectionData->addressFamily = AF_INET;

         if(pInjectionData->direction == FWP_DIRECTION_OUTBOUND)
            pInjectionData->injectionHandle = g_pIPv4OutboundForwardInjectionHandles[index];
         else
            pInjectionData->injectionHandle = g_pIPv4InboundForwardInjectionHandles[index];

         break;
      }
      case FWPS_LAYER_IPFORWARD_V6:
      {
         pInjectionData->addressFamily = AF_INET6;

         if(pInjectionData->direction == FWP_DIRECTION_OUTBOUND)
            pInjectionData->injectionHandle = g_pIPv6OutboundForwardInjectionHandles[index];
         else
            pInjectionData->injectionHandle = g_pIPv6InboundForwardInjectionHandles[index];

         break;
      }
      case FWPS_LAYER_INBOUND_TRANSPORT_V4:
      {
         pInjectionData->addressFamily   = AF_INET;
         pInjectionData->injectionHandle = g_pIPv4InboundTransportInjectionHandles[index];

         break;
      }
      case FWPS_LAYER_INBOUND_TRANSPORT_V6:
      {
         pInjectionData->addressFamily   = AF_INET6;
         pInjectionData->injectionHandle = g_pIPv6InboundTransportInjectionHandles[index];

         break;
      }
      case FWPS_LAYER_OUTBOUND_TRANSPORT_V4:
      {
         pInjectionData->addressFamily   = AF_INET;
         pInjectionData->injectionHandle = g_pIPv4OutboundTransportInjectionHandles[index];

         break;
      }
      case FWPS_LAYER_OUTBOUND_TRANSPORT_V6:
      {
         pInjectionData->addressFamily   = AF_INET6;
         pInjectionData->injectionHandle = g_pIPv6OutboundTransportInjectionHandles[index];

         break;
      }
      case FWPS_LAYER_STREAM_V4:
      {
         pInjectionData->addressFamily = AF_INET;

         if(pInjectionData->direction == FWP_DIRECTION_OUTBOUND)
            pInjectionData->injectionHandle = g_pIPv4OutboundStreamInjectionHandles[index];
         else
            pInjectionData->injectionHandle = g_pIPv4InboundStreamInjectionHandles[index];

         break;
      }
      case FWPS_LAYER_STREAM_V6:
      {
         pInjectionData->addressFamily = AF_INET6;

         if(pInjectionData->direction == FWP_DIRECTION_OUTBOUND)
            pInjectionData->injectionHandle = g_pIPv6OutboundStreamInjectionHandles[index];
         else
            pInjectionData->injectionHandle = g_pIPv6InboundStreamInjectionHandles[index];

         break;
      }
      case FWPS_LAYER_DATAGRAM_DATA_V4:
      {
         pInjectionData->addressFamily = AF_INET;

         if(pInjectionData->direction == FWP_DIRECTION_OUTBOUND)
            pInjectionData->injectionHandle = g_pIPv4OutboundTransportInjectionHandles[index];
         else
            pInjectionData->injectionHandle = g_pIPv4InboundTransportInjectionHandles[index];

         break;
      }
      case FWPS_LAYER_DATAGRAM_DATA_V6:
      {
         pInjectionData->addressFamily = AF_INET6;

         if(pInjectionData->direction == FWP_DIRECTION_OUTBOUND)
            pInjectionData->injectionHandle = g_pIPv6OutboundTransportInjectionHandles[index];
         else
            pInjectionData->injectionHandle = g_pIPv6InboundTransportInjectionHandles[index];

         break;
      }
      case FWPS_LAYER_INBOUND_ICMP_ERROR_V4:
      {
         pInjectionData->addressFamily   = AF_INET;
         pInjectionData->injectionHandle = g_pIPv4InboundTransportInjectionHandles[index];

         break;
      }
      case FWPS_LAYER_INBOUND_ICMP_ERROR_V6:
      {
         pInjectionData->addressFamily   = AF_INET6;
         pInjectionData->injectionHandle = g_pIPv6InboundTransportInjectionHandles[index];

         break;
      }
      case FWPS_LAYER_OUTBOUND_ICMP_ERROR_V4:
      {
         pInjectionData->addressFamily   = AF_INET;
         pInjectionData->injectionHandle = g_pIPv4OutboundTransportInjectionHandles[index];

         break;
      }
      case FWPS_LAYER_OUTBOUND_ICMP_ERROR_V6:
      {
         pInjectionData->addressFamily   = AF_INET6;
         pInjectionData->injectionHandle = g_pIPv6OutboundTransportInjectionHandles[index];

         break;
      }
      case FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4:
      {
         pInjectionData->addressFamily = AF_INET;

         if(pInjectionData->direction == FWP_DIRECTION_OUTBOUND)
            pInjectionData->injectionHandle = g_pIPv4OutboundTransportInjectionHandles[index];
         else
            pInjectionData->injectionHandle = g_pIPv4InboundTransportInjectionHandles[index];

         break;
      }
      case FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6:
      {
         pInjectionData->addressFamily = AF_INET6;

         if(pInjectionData->direction == FWP_DIRECTION_OUTBOUND)
            pInjectionData->injectionHandle = g_pIPv6OutboundTransportInjectionHandles[index];
         else
            pInjectionData->injectionHandle = g_pIPv6InboundTransportInjectionHandles[index];

         break;
      }
      case FWPS_LAYER_ALE_AUTH_CONNECT_V4:
      {
         pInjectionData->addressFamily = AF_INET;

         if(pInjectionData->direction == FWP_DIRECTION_INBOUND)
            pInjectionData->injectionHandle = g_pIPv4InboundTransportInjectionHandles[index];
         else
            pInjectionData->injectionHandle = g_pIPv4OutboundTransportInjectionHandles[index];

         break;
      }
      case FWPS_LAYER_ALE_AUTH_CONNECT_V6:
      {
         pInjectionData->addressFamily = AF_INET6;

         if(pInjectionData->direction == FWP_DIRECTION_INBOUND)
            pInjectionData->injectionHandle = g_pIPv6InboundTransportInjectionHandles[index];
         else
            pInjectionData->injectionHandle = g_pIPv6OutboundTransportInjectionHandles[index];

         break;
      }
      case FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4:
      {
         pInjectionData->addressFamily = AF_INET;

         if(pInjectionData->direction == FWP_DIRECTION_OUTBOUND)
            pInjectionData->injectionHandle = g_pIPv4OutboundTransportInjectionHandles[index];
         else
            pInjectionData->injectionHandle = g_pIPv4InboundTransportInjectionHandles[index];

         break;
      }
      case FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6:
      {
         pInjectionData->addressFamily = AF_INET6;

         if(pInjectionData->direction == FWP_DIRECTION_OUTBOUND)
            pInjectionData->injectionHandle = g_pIPv6OutboundTransportInjectionHandles[index];
         else
            pInjectionData->injectionHandle = g_pIPv6InboundTransportInjectionHandles[index];

         break;
      }

#if(NTDDI_VERSION >= NTDDI_WIN7)

      case FWPS_LAYER_STREAM_PACKET_V4:
      {
         pInjectionData->addressFamily = AF_INET;

         if(pInjectionData->direction == FWP_DIRECTION_OUTBOUND)
            pInjectionData->injectionHandle = g_pIPv4OutboundTransportInjectionHandles[index];
         else
            pInjectionData->injectionHandle = g_pIPv4InboundTransportInjectionHandles[index];

         break;
      }
      case FWPS_LAYER_STREAM_PACKET_V6:
      {
         pInjectionData->addressFamily = AF_INET6;

         if(pInjectionData->direction == FWP_DIRECTION_OUTBOUND)
            pInjectionData->injectionHandle = g_pIPv6OutboundTransportInjectionHandles[index];
         else
            pInjectionData->injectionHandle = g_pIPv6InboundTransportInjectionHandles[index];

         break;
      }

#if(NTDDI_VERSION >= NTDDI_WIN8)

      case FWPS_LAYER_INBOUND_MAC_FRAME_ETHERNET:
      {
         UINT16 etherType = pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_MAC_FRAME_ETHERNET_ETHER_TYPE].value.uint16;

         if(etherType == 0x86DD)
         {
            pInjectionData->addressFamily   = AF_INET6;
            pInjectionData->injectionHandle = g_pIPv6InboundMACInjectionHandles[index];
         }
         else if(etherType == 0x800)
         {
            pInjectionData->addressFamily   = AF_INET;
            pInjectionData->injectionHandle = g_pIPv4InboundMACInjectionHandles[index];
         }
         else
         {
            pInjectionData->addressFamily   = AF_UNSPEC;
            pInjectionData->injectionHandle = g_pInboundMACInjectionHandles[index];
         }

         break;
      }
      case FWPS_LAYER_OUTBOUND_MAC_FRAME_ETHERNET:
      {
         UINT16 etherType = pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_MAC_FRAME_ETHERNET_ETHER_TYPE].value.uint16;

         if(etherType == 0x86DD)
         {
            pInjectionData->addressFamily   = AF_INET6;
            pInjectionData->injectionHandle = g_pIPv6OutboundMACInjectionHandles[index];
         }
         else if(etherType == 0x800)
         {
            pInjectionData->addressFamily   = AF_INET;
            pInjectionData->injectionHandle = g_pIPv4OutboundMACInjectionHandles[index];
         }
         else
         {
            pInjectionData->addressFamily   = AF_UNSPEC;
            pInjectionData->injectionHandle = g_pOutboundMACInjectionHandles[index];
         }

         break;
      }
      case FWPS_LAYER_INBOUND_MAC_FRAME_NATIVE:
      {
         pInjectionData->addressFamily   = AF_UNSPEC;
         pInjectionData->injectionHandle = g_pInboundMACInjectionHandles[index];

         break;
      }
      case FWPS_LAYER_OUTBOUND_MAC_FRAME_NATIVE:
      {
         pInjectionData->addressFamily   = AF_UNSPEC;
         pInjectionData->injectionHandle = g_pOutboundMACInjectionHandles[index];

         break;
      }
      case FWPS_LAYER_INGRESS_VSWITCH_ETHERNET:
      {
         UINT16 etherType = pClassifyValues->incomingValue[FWPS_FIELD_INGRESS_VSWITCH_ETHERNET_ETHER_TYPE].value.uint16;

         if(etherType == 0x86DD)
         {
            pInjectionData->addressFamily   = AF_INET6;
            pInjectionData->injectionHandle = g_pIPv6IngressVSwitchEthernetInjectionHandles[index];
         }
         else if(etherType == 0x800)
         {
            pInjectionData->addressFamily   = AF_INET;
            pInjectionData->injectionHandle = g_pIPv4IngressVSwitchEthernetInjectionHandles[index];
         }
         else
         {
            pInjectionData->addressFamily   = AF_UNSPEC;
            pInjectionData->injectionHandle = g_pIngressVSwitchEthernetInjectionHandles[index];
         }

         break;
      }
      case FWPS_LAYER_EGRESS_VSWITCH_ETHERNET:
      {
         UINT16 etherType = pClassifyValues->incomingValue[FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_ETHER_TYPE].value.uint16;

         if(etherType == 0x86DD)
         {
            pInjectionData->addressFamily   = AF_INET6;
            pInjectionData->injectionHandle = g_pIPv6EgressVSwitchEthernetInjectionHandles[index];
         }
         else if(etherType == 0x800)
         {
            pInjectionData->addressFamily   = AF_INET;
            pInjectionData->injectionHandle = g_pIPv4EgressVSwitchEthernetInjectionHandles[index];
         }
         else
         {
            pInjectionData->addressFamily   = AF_UNSPEC;
            pInjectionData->injectionHandle = g_pEgressVSwitchEthernetInjectionHandles[index];
         }

         break;
      }

#endif // (NTDDI_VERSION >= NTDDI_WIN8)
#endif // (NTDDI_VERSION >= NTDDI_WIN7)

      default:
         status = STATUS_NOT_SUPPORTED;
   }

   if(pInjectionData->injectionHandle &&
      pNetBufferList &&
      pClassifyValues->layerId != FWPS_LAYER_STREAM_V4 &&
      pClassifyValues->layerId != FWPS_LAYER_STREAM_V6)
      pInjectionData->injectionState = FwpsQueryPacketInjectionState(pInjectionData->injectionHandle,
                                                                     pNetBufferList,
                                                                     &(pInjectionData->injectionContext));

   HLPR_BAIL_LABEL:

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprInjectionDataPopulate() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}

/**
 @kernel_helper_function="KrnlHlprInjectionDataCreate"
 
   Purpose:  Allocates and populates a INJECTION_DATA object with data based on values obtained 
             in the classifyFn.                                                                 <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_At_(*ppInjectionData, _Pre_ _Null_)
_When_(return != STATUS_SUCCESS, _At_(*ppInjectionData, _Post_ _Null_))
_When_(return == STATUS_SUCCESS, _At_(*ppInjectionData, _Post_ _Notnull_ __drv_allocatesMem(Pool)))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprInjectionDataCreate(_Outptr_ INJECTION_DATA** ppInjectionData,
                                     _In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                     _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadataValues,
                                     _In_opt_ const NET_BUFFER_LIST* pNetBufferList,
                                     _In_ const FWPS_FILTER* pFilter)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprInjectionDataCreate()\n");

#endif /// DBG
   
   NT_ASSERT(ppInjectionData);
   NT_ASSERT(pClassifyValues);
   NT_ASSERT(pMetadataValues);
   NT_ASSERT(pFilter);

   NTSTATUS status = STATUS_SUCCESS;

   HLPR_NEW(*ppInjectionData,
            INJECTION_DATA,
            WFPSAMPLER_SYSLIB_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(*ppInjectionData,
                              status);

   status = KrnlHlprInjectionDataPopulate(*ppInjectionData,
                                          pClassifyValues,
                                          pMetadataValues,
                                          pNetBufferList,
                                          pFilter);

   HLPR_BAIL_LABEL:

#pragma warning(push)
#pragma warning(disable: 6001) /// *ppInjectionData initialized with call to HLPR_NEW & KrnlHlprInjectionDataPopulate 

   if(status != STATUS_SUCCESS &&
      *ppInjectionData)
      KrnlHlprInjectionDataDestroy(ppInjectionData);

#pragma warning(pop)

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprInjectionDataCreate() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}

#endif /// INJECTION_DATA____

#ifndef INJECTION_HANDLE_DATA____
#define INJECTION_HANDLE_DATA____

/**
 @kernel_helper_function="KrnlHlprInjectionHandleDataDestroy"
 
   Purpose:  Cleanup and free a INJECTION_HANDLE_DATA object.                                   <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_At_(*ppInjectionHandleData, _Pre_ _Notnull_)
_At_(*ppInjectionHandleData, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprInjectionHandleDataDestroy(_Inout_ INJECTION_HANDLE_DATA** ppInjectionHandleData)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprInjectionHandleDataDestroy()\n");

#endif /// DBG
   
   NT_ASSERT(ppInjectionHandleData);

   NTSTATUS status = STATUS_SUCCESS;

   if(*ppInjectionHandleData)
   {
      if((*ppInjectionHandleData)->pMACHandle)
      {
         status = KrnlHlprFwpsInjectionReleaseHandle((*ppInjectionHandleData)->pMACHandle);
         HLPR_BAIL_ON_FAILURE(status);
      }

      if((*ppInjectionHandleData)->pVSwitchEthernetHandle)
      {
         status = KrnlHlprFwpsInjectionReleaseHandle((*ppInjectionHandleData)->pVSwitchEthernetHandle);
         HLPR_BAIL_ON_FAILURE(status);
      }

      if((*ppInjectionHandleData)->pForwardHandle)
      {
         status = KrnlHlprFwpsInjectionReleaseHandle((*ppInjectionHandleData)->pForwardHandle);
         HLPR_BAIL_ON_FAILURE(status);
      }

      if((*ppInjectionHandleData)->pNetworkHandle)
      {
         status = KrnlHlprFwpsInjectionReleaseHandle((*ppInjectionHandleData)->pNetworkHandle);
         HLPR_BAIL_ON_FAILURE(status);
      }

      if((*ppInjectionHandleData)->pTransportHandle)
      {
         status = KrnlHlprFwpsInjectionReleaseHandle((*ppInjectionHandleData)->pTransportHandle);
         HLPR_BAIL_ON_FAILURE(status);
      }

      if((*ppInjectionHandleData)->pStreamHandle)
      {
         status = KrnlHlprFwpsInjectionReleaseHandle((*ppInjectionHandleData)->pStreamHandle);
         HLPR_BAIL_ON_FAILURE(status);
      }

      HLPR_DELETE(*ppInjectionHandleData,
                  WFPSAMPLER_SYSLIB_TAG);
   }

   HLPR_BAIL_LABEL:

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprInjectionHandleDataDestroy() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}

/**
 @kernel_helper_function="KrnlHlprInjectionHandleDataCreate"
 
   Purpose:  Allocates and populates a INJECTION_HANDLE_DATA object with the various injection 
             handles created when the driver loads.                                             <br>
                                                                                                <br>
   Notes:    The index is related to which sublayer the handle operates on.  For 
             FWPM_SUBLAYER_UNIVERSAL, the index is 1, otherwise it's 0.                         <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_At_(*ppInjectionHandleData, _Pre_ _Null_)
_When_(return != STATUS_SUCCESS, _At_(*ppInjectionHandleData, _Post_ _Null_))
_When_(return == STATUS_SUCCESS, _At_(*ppInjectionHandleData, _Post_ _Notnull_ __drv_allocatesMem(Pool)))
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprInjectionHandleDataCreate(_Outptr_ INJECTION_HANDLE_DATA** ppInjectionHandleData,
                                           _In_ ADDRESS_FAMILY addressFamily,                      /* AF_INET */
                                           _In_ BOOLEAN isInbound,                                 /* TRUE */
                                           _In_ UINT32 index)                                      /* WFPSAMPLER_INDEX */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprInjectionHandleDataCreate()\n");

#endif /// DBG
   
   NT_ASSERT(ppInjectionHandleData);

   NTSTATUS status = STATUS_SUCCESS;

   HLPR_NEW(*ppInjectionHandleData,
            INJECTION_HANDLE_DATA,
            WFPSAMPLER_SYSLIB_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(*ppInjectionHandleData,
                              status);

   if(addressFamily == AF_INET)
   {
      if(isInbound)
      {
         (*ppInjectionHandleData)->pForwardHandle          = &(g_pIPv4InboundForwardInjectionHandles[index]);
         (*ppInjectionHandleData)->pNetworkHandle          = &(g_pIPv4InboundNetworkInjectionHandles[index]);
         (*ppInjectionHandleData)->pTransportHandle        = &(g_pIPv4InboundTransportInjectionHandles[index]);
         (*ppInjectionHandleData)->pStreamHandle           = &(g_pIPv4InboundStreamInjectionHandles[index]);
         (*ppInjectionHandleData)->pMACHandle              = &(g_pIPv4InboundMACInjectionHandles[index]);
         (*ppInjectionHandleData)->pVSwitchEthernetHandle  = &(g_pIPv4IngressVSwitchEthernetInjectionHandles[index]);
      }
      else
      {
         (*ppInjectionHandleData)->pForwardHandle          = &(g_pIPv4OutboundForwardInjectionHandles[index]);
         (*ppInjectionHandleData)->pNetworkHandle          = &(g_pIPv4OutboundNetworkInjectionHandles[index]);
         (*ppInjectionHandleData)->pTransportHandle        = &(g_pIPv4OutboundTransportInjectionHandles[index]);
         (*ppInjectionHandleData)->pStreamHandle           = &(g_pIPv4OutboundStreamInjectionHandles[index]);
         (*ppInjectionHandleData)->pMACHandle              = &(g_pIPv4OutboundMACInjectionHandles[index]);
         (*ppInjectionHandleData)->pVSwitchEthernetHandle  = &(g_pIPv4EgressVSwitchEthernetInjectionHandles[index]);
      }
   }
   else if(addressFamily == AF_INET6)
   {
      if(isInbound)
      {
         (*ppInjectionHandleData)->pForwardHandle          = &(g_pIPv6InboundForwardInjectionHandles[index]);
         (*ppInjectionHandleData)->pNetworkHandle          = &(g_pIPv6InboundNetworkInjectionHandles[index]);
         (*ppInjectionHandleData)->pTransportHandle        = &(g_pIPv6InboundTransportInjectionHandles[index]);
         (*ppInjectionHandleData)->pStreamHandle           = &(g_pIPv6InboundStreamInjectionHandles[index]);
         (*ppInjectionHandleData)->pMACHandle              = &(g_pIPv6InboundMACInjectionHandles[index]);
         (*ppInjectionHandleData)->pVSwitchEthernetHandle  = &(g_pIPv6IngressVSwitchEthernetInjectionHandles[index]);
      }
      else
      {
         (*ppInjectionHandleData)->pForwardHandle          = &(g_pIPv6OutboundForwardInjectionHandles[index]);
         (*ppInjectionHandleData)->pNetworkHandle          = &(g_pIPv6OutboundNetworkInjectionHandles[index]);
         (*ppInjectionHandleData)->pTransportHandle        = &(g_pIPv6OutboundTransportInjectionHandles[index]);
         (*ppInjectionHandleData)->pStreamHandle           = &(g_pIPv6OutboundStreamInjectionHandles[index]);
         (*ppInjectionHandleData)->pMACHandle              = &(g_pIPv6OutboundMACInjectionHandles[index]);
         (*ppInjectionHandleData)->pVSwitchEthernetHandle  = &(g_pIPv6EgressVSwitchEthernetInjectionHandles[index]);
      }
   }
   else
   {
      if(isInbound)
      {
         (*ppInjectionHandleData)->pForwardHandle          = &(g_pInboundForwardInjectionHandles[index]);
         (*ppInjectionHandleData)->pNetworkHandle          = &(g_pInboundNetworkInjectionHandles[index]);
         (*ppInjectionHandleData)->pTransportHandle        = &(g_pInboundTransportInjectionHandles[index]);
         (*ppInjectionHandleData)->pStreamHandle           = &(g_pInboundStreamInjectionHandles[index]);
         (*ppInjectionHandleData)->pMACHandle              = &(g_pInboundMACInjectionHandles[index]);
         (*ppInjectionHandleData)->pVSwitchEthernetHandle  = &(g_pIngressVSwitchEthernetInjectionHandles[index]);
      }
      else
      {
         (*ppInjectionHandleData)->pForwardHandle          = &(g_pOutboundForwardInjectionHandles[index]);
         (*ppInjectionHandleData)->pNetworkHandle          = &(g_pOutboundNetworkInjectionHandles[index]);
         (*ppInjectionHandleData)->pTransportHandle        = &(g_pOutboundTransportInjectionHandles[index]);
         (*ppInjectionHandleData)->pStreamHandle           = &(g_pOutboundStreamInjectionHandles[index]);
         (*ppInjectionHandleData)->pMACHandle              = &(g_pOutboundMACInjectionHandles[index]);
         (*ppInjectionHandleData)->pVSwitchEthernetHandle  = &(g_pEgressVSwitchEthernetInjectionHandles[index]);
      }
   }

#if(NTDDI_VERSION >= NTDDI_WIN8)

   status = KrnlHlprFwpsInjectionAcquireHandle((*ppInjectionHandleData)->pMACHandle,
                                               addressFamily,
                                               FWPS_INJECTION_TYPE_L2);
   if(status != STATUS_SUCCESS)
   {
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! KrnlHlprInjectionHandleDataCreate : KrnlHlprFwpsInjectionAcquireHandle() [type: FWPS_INJECTION_TYPE_L2][status: %#x]\n",
                 status);

      HLPR_BAIL;
   }

   status = KrnlHlprFwpsInjectionAcquireHandle((*ppInjectionHandleData)->pVSwitchEthernetHandle,
                                               addressFamily,
                                               FWPS_INJECTION_TYPE_L2);
   if(status != STATUS_SUCCESS)
   {
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! KrnlHlprInjectionHandleDataCreate : KrnlHlprFwpsInjectionAcquireHandle() [type: FWPS_INJECTION_TYPE_L2][status: %#x]\n",
                 status);

      HLPR_BAIL;
   }

#endif // (NTDDI_VERSION >= NTDDI_WIN8)

   status = KrnlHlprFwpsInjectionAcquireHandle((*ppInjectionHandleData)->pForwardHandle,
                                               addressFamily,
                                               FWPS_INJECTION_TYPE_FORWARD);
   if(status != STATUS_SUCCESS)
   {
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! KrnlHlprInjectionHandleDataCreate : KrnlHlprFwpsInjectionAcquireHandle() [type: FWPS_INJECTION_TYPE_FORWARD][status: %#x]\n",
                 status);

      HLPR_BAIL;
   }

   status = KrnlHlprFwpsInjectionAcquireHandle((*ppInjectionHandleData)->pNetworkHandle,
                                               addressFamily ? addressFamily : AF_INET,
                                               FWPS_INJECTION_TYPE_NETWORK);
   if(status != STATUS_SUCCESS)
   {
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! KrnlHlprInjectionHandleDataCreate : KrnlHlprFwpsInjectionAcquireHandle() [type: FWPS_INJECTION_TYPE_NETWORK][status: %#x]\n",
                 status);

      HLPR_BAIL;
   }

   status = KrnlHlprFwpsInjectionAcquireHandle((*ppInjectionHandleData)->pTransportHandle,
                                               addressFamily,
                                               FWPS_INJECTION_TYPE_TRANSPORT);
   if(status != STATUS_SUCCESS)
   {
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! KrnlHlprInjectionHandleDataCreate : KrnlHlprFwpsInjectionAcquireHandle() [type: FWPS_INJECTION_TYPE_TRANSPORT][status: %#x]\n",
                 status);

      HLPR_BAIL;
   }

   status = KrnlHlprFwpsInjectionAcquireHandle((*ppInjectionHandleData)->pStreamHandle,
                                               addressFamily,
                                               FWPS_INJECTION_TYPE_STREAM);
   if(status != STATUS_SUCCESS)
   {
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! KrnlHlprInjectionHandleDataCreate : KrnlHlprFwpsInjectionAcquireHandle() [type: FWPS_INJECTION_TYPE_STREAM][status: %#x]\n",
                 status);

      HLPR_BAIL;
   }

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS &&
      *ppInjectionHandleData)
      KrnlHlprInjectionHandleDataDestroy(ppInjectionHandleData);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprInjectionHandleDataCreate() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}

#endif /// INJECTION_HANDLE_DATA____
