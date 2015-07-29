////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      ClassifyFunctions_BasicPacketModificationCallouts.cpp
//
//   Abstract:
//      This module contains WFP Classify functions for modifying and injecting packets back into 
//         the data path using the clone / block / inject method.
//
//   Naming Convention:
//
//      <Module><Scenario>
//  
//      i.e.
//       ClassifyBasicPacketModification
//
//       <Module>
//          Classify                - Function is an FWPS_CALLOUT_CLASSIFY_FN
//       <Scenario>
//          BasicPacketModification - Function demonstrates the clone / block / modify / inject 
//                                       model.
//
//      <Action><Scenario><Modifier>
//
//      i.e.
//       TriggerBasicPacketModificationOutOfBand
//
//       <Action>
//        {
//                                    -
//          Trigger                   - Initiates the desired scenario.
//          Perform                   - Executes the desired scenario.
//        }
//       <Scenario>
//          BasicPacketModification   - Function demonstrates the clone / block / modify / inject 
//                                         model.
//       <Modifier>
//          DeferredProcedureCall     - DPC routine for Out of Band injection which dispatches the 
//                                         proper Perform Function.
//          WorkItemRoutine           - WorkItem Routine for Out of Band Injection which dispatches
//                                         the proper Perform Function.
//          AtInboundMACFrame         - Function operates on:
//                                         FWPM_LAYER_INBOUND_MAC_FRAME_ETHERNET, and 
//                                         FWPM_LAYER_INBOUND_MAC_NATIVE.
//          AtOutboundMACFrame        - Function operates on:
//                                         FWPM_LAYER_OUTBOUND_MAC_FRAME_ETHERNET, and 
//                                         FWPM_LAYER_OUTBOUND_MAC_NATIVE.
//          AtEgressVSwitchEthernet   - Function operates on:
//                                         FWPM_LAYER_EGRESS_VSWITCH_ETHERNET.
//          AtIngressVSwitchEthernet  - Function operates on:
//                                         FWPM_LAYER_INGRESS_VSWITCH_ETHERNET.
//          AtInboundNetwork          - Function operates on:
//                                         FWPM_LAYER_INBOUND_IPPACKET_V{4/6}
//          AtOutboundNetwork         - Function operates on:
//                                         FWPM_LAYER_OUTBOUND_IPPACKET_V{4/6}
//          AtForward                 - Function operates on:
//                                         FWPM_LAYER_IPFORWARD_V{4/6}
//          AtInboundTransport        - Function operates on: 
//                                         FWPM_LAYER_INBOUND_TRANSPORT_V{4/6},
//                                         FWPM_LAYER_INBOUND_ICMP_ERROR_V{4/6},
//                                         FWPM_LAYER_DATAGRAM_DATA_V{4/6},
//                                         FWPM_LAYER_STREAM_PACKET_V{4/6}, and 
//                                         FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V{4/6}
//                                         FWPM_LAYER_ALE_FLOW_ESTABLISHED_V{4/6}
//          AtOutboundTransport       - Function operates on:
//                                         FWPM_LAYER_OUTBOUND_TRANSPORT_V{4/6},
//                                         FWPM_LAYER_OUTBOUND_ICMP_ERROR_V{4/6},
//                                         FWPM_LAYER_DATAGRAM_DATA_V{4/6},
//                                         FWPM_LAYER_STREAM_PACKET_V{4/6}, and 
//                                         FWPM_LAYER_ALE_AUTH_CONNECT_V{4/6}
//                                         FWPM_LAYER_ALE_FLOW_ESTABLISHED_V{4/6}
//
//   Private Functions:
//      BasicPacketModificationDeferredProcedureCall(),
//      BasicPacketModificationWorkItemRoutine(),
//      PerformBasicPacketModificationAtEgressVSwitchEthernet(),
//      PerformBasicPacketModificationAtForward(),
//      PerformBasicPacketModificationAtInboundMACFrame(),
//      PerformBasicPacketModificationAtInboundNetwork(),
//      PerformBasicPacketModificationAtInboundTransport(),
//      PerformBasicPacketModificationAtIngressVSwitchEthernet(),
//      PerformBasicPacketModificationAtOutboundMACFrame(),
//      PerformBasicPacketModificationAtOutboundNetwork(),
//      PerformBasicPacketModificationAtOutboundTransport(),
//      TriggerBasicPacketModificationInline(),
//      TriggerBasicPacketModificationOutOfBand(),
//
//   Public Functions:
//      ClassifyBasicPacketModification(),
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Enhance function declaration for IntelliSense, enhance 
//                                              traces, fix weakhost injection, fix expected 
//                                              offsets,fix copy / paste issues with modifying dst 
//                                              port, and add support for multiple injectors and 
//                                              controlData.
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Framework_WFPSamplerCalloutDriver.h"                   /// .
#include "ClassifyFunctions_BasicPacketModificationCallouts.tmh" /// $(OBJ_PATH)\$(O)\ 

#if(NTDDI_VERSION >= NTDDI_WIN8)

/**
 @private_function="PerformBasicPacketModificationAtInboundMACFrame"
 
   Purpose:  Clones the NET_BUFFER_LIST, modifies it with data from the associated context and 
             injects the clone back to the stack's inbound path from the incoming MAC Layers 
             using FwpsInjectMacReceiveAsync().                                                 <br>
                                                                                                <br>
   Notes:    Applies to the following inbound layers:                                           <br>
                FWPM_LAYER_INBOUND_MAC_FRAME_ETHERNET                                           <br>
                FWPM_LAYER_INBOUND_MAC_FRAME_NATIVE                                             <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/HH439588.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF546324.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
NTSTATUS PerformBasicPacketModificationAtInboundMACFrame(_In_ CLASSIFY_DATA** ppClassifyData,
                                                         _In_ INJECTION_DATA** ppInjectionData,
                                                         _In_ PC_BASIC_PACKET_MODIFICATION_DATA* pModificationData,
                                                         _In_ BOOLEAN isInline = FALSE)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> PerformBasicPacketModificationAtInboundMACFrame()\n");

#endif /// DBG

   NT_ASSERT(ppClassifyData);
   NT_ASSERT(ppInjectionData);
   NT_ASSERT(pModificationData);
   NT_ASSERT(*ppClassifyData);
   NT_ASSERT(*ppInjectionData);

   NTSTATUS                                   status          = STATUS_SUCCESS;
   FWPS_INCOMING_VALUES*                      pClassifyValues = (FWPS_INCOMING_VALUES*)(*ppClassifyData)->pClassifyValues;
   FWPS_INCOMING_METADATA_VALUES*             pMetadata       = (FWPS_INCOMING_METADATA_VALUES*)(*ppClassifyData)->pMetadataValues;
   IF_INDEX                                   interfaceIndex  = 0;
   NDIS_PORT_NUMBER                           ndisPort        = 0;
   NET_BUFFER_LIST*                           pNetBufferList  = 0;
   BASIC_PACKET_MODIFICATION_COMPLETION_DATA* pCompletionData = 0;
   UINT32                                     bytesRetreated  = 0;
   FWP_VALUE*                                 pInterfaceIndex = 0;
   FWP_VALUE*                                 pNDISPort       = 0;

#if DBG

   KIRQL                                      irql            = KeGetCurrentIrql();

#endif /// DBG

#pragma warning(push)
#pragma warning(disable: 6014) /// pCompletionData will be freed in completionFn using BasicPacketModificationCompletionDataDestroy

   HLPR_NEW(pCompletionData,
            BASIC_PACKET_MODIFICATION_COMPLETION_DATA,
            WFPSAMPLER_CALLOUT_DRIVER_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pCompletionData,
                              status);

#pragma warning(pop)

   KeInitializeSpinLock(&(pCompletionData->spinLock));

   pCompletionData->performedInline = isInline;
   pCompletionData->pClassifyData   = *ppClassifyData;
   pCompletionData->pInjectionData  = *ppInjectionData;

   /// Responsibility for freeing this memory has been transferred to the pCompletionData
   *ppClassifyData = 0;

   *ppInjectionData = 0;

   pInterfaceIndex = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                               &FWPM_CONDITION_INTERFACE_INDEX);
   if(pInterfaceIndex &&
      pInterfaceIndex->type == FWP_UINT32)
      interfaceIndex = (IF_INDEX)pInterfaceIndex->uint32;

   pNDISPort = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                         &FWPM_CONDITION_NDIS_PORT);
   if(pNDISPort &&
      pNDISPort->type == FWP_UINT32)
      ndisPort = (NDIS_PORT_NUMBER)pNDISPort->uint32;

   /// If NATIVE, initial offset is at the MAC Header ...
   if(pClassifyValues->layerId != FWPS_LAYER_INBOUND_MAC_FRAME_NATIVE &&
      FWPS_IS_L2_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_L2_METADATA_FIELD_ETHERNET_MAC_HEADER_SIZE))
      bytesRetreated = pMetadata->ethernetMacHeaderSize;

   if(bytesRetreated)
   {
      /// ... otherwise the offset is at the IP Header, so retreat the size of the MAC Header ...
      status = NdisRetreatNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)pCompletionData->pClassifyData->pPacket),
                                             bytesRetreated,
                                             0,
                                             0);
      if(status != STATUS_SUCCESS)
      {
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_ERROR_LEVEL,
                    " !!!! PerformBasicPacketModificationAtInboundMACFrame: NdisRetreatNetBufferDataStart() [status: %#x]\n",
                    status);

         HLPR_BAIL;
      }
   }

   /// ... clone the entire NET_BUFFER_LIST ...
   status = FwpsAllocateCloneNetBufferList((NET_BUFFER_LIST*)pCompletionData->pClassifyData->pPacket,
                                           g_pNDISPoolData->nblPoolHandle,
                                           g_pNDISPoolData->nbPoolHandle,
                                           0,
                                           &pNetBufferList);

   if(bytesRetreated)
   {
      /// ... and advance the offset back to the original position.
      NdisAdvanceNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)pCompletionData->pClassifyData->pPacket),
                                    bytesRetreated,
                                    FALSE,
                                    0);
   }

   if(status != STATUS_SUCCESS)
   {
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! PerformBasicPacketModificationAtInboundMACFrame: FwpsAllocateCloneNetBufferList() [status: %#x]\n",
                 status);

      HLPR_BAIL;
   }

   if(pModificationData->flags)
   {
      /// Various checks and balances must be performed to modify the IP and Transport headers at this modification point.
      /// Parsing of the headers will need to occur, as well as spot checking to verify everything is as it should be.
      /// Additionally, checksum routines will need to be written to recalculate checksums for some of the headers.
      /// The following block of code is to get you started with modifying the headers with info not readily available
      /// (i.e. header parsing has not occurred so there is no relevant classifiable data nor metadata present).
/*
      if(pModificationData->flags & PCPMDF_MODIFY_TRANSPORT_HEADER)
      {
         UINT32  tmpStatus = STATUS_SUCCESS;
         IPPROTO protocol  = IPPROTO_MAX;

         /// The clone is at the Ethernet Header, so advance by the size of the Ethernet Header...
         NdisAdvanceNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB(pNetBufferList),
                                       ethernetHeaderSize,
                                       FALSE,
                                       0);

         protocol = KrnlHlprIPHeaderGetProtocolField(pNetBufferList,
                                                     pCompletionData->pInjectionData->addressFamily);

         /// No Transport Modification if IPsec encrypted
         if(protocol != IPPROTO_ESP &&
            protocol != IPPROTO_AH)
         {
            /// ... advance by the size of the IP Header.
            NdisAdvanceNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB(pNetBufferList),
                                          ipHeaderSize,
                                          FALSE,
                                          0);

            switch(protocol)
            {
               case IPPROTO_ICMP:
               {
                  UINT32 icmpHeaderSize = ICMP_HEADER_MIN_SIZE;
            
                  if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                                    FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE))
                     icmpHeaderSize = pMetadata->transportHeaderSize;
            
                  if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_ICMP_TYPE)
                  {
                     FWP_VALUE0 icmpType;
            
                     icmpType.type  = FWP_UINT8;
                     icmpType.uint8 = (UINT8)ntohs(pModificationData->transportData.sourcePort);
            
                     status = KrnlHlprICMPv4HeaderModifyType(&icmpType,
                                                             pNetBufferList,
                                                             icmpHeaderSize);
                     HLPR_BAIL_ON_FAILURE_2(status);
                  }
            
                  if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_ICMP_CODE)
                  {
                     FWP_VALUE0 icmpCode;
                  
                     icmpCode.type  = FWP_UINT8;
                     icmpCode.uint8 = (UINT8)ntohs(pModificationData->transportData.destinationPort);
            
                     status = KrnlHlprICMPv4HeaderModifyCode(&icmpCode,
                                                             pNetBufferList,
                                                             icmpHeaderSize);
                     HLPR_BAIL_ON_FAILURE_2(status);
                  }
            
                  break;
               }
               case IPPROTO_TCP:
               {
                  UINT32 tcpHeaderSize = TCP_HEADER_MIN_SIZE;
            
                  if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                                    FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE))
                     tcpHeaderSize = pMetadata->transportHeaderSize;
            
                  if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_SOURCE_PORT)
                  {
                     FWP_VALUE0 srcPort;
            
                     srcPort.type   = FWP_UINT16;
                     srcPort.uint16 = pModificationData->transportData.sourcePort;
            
                     status = KrnlHlprTCPHeaderModifySourcePort(&srcPort,
                                                                pNetBufferList,
                                                                tcpHeaderSize);
                     HLPR_BAIL_ON_FAILURE_2(status);
                  }
            
                  if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_DESTINATION_PORT)
                  {
                     FWP_VALUE0 dstPort;
            
                     dstPort.type   = FWP_UINT16;
                     dstPort.uint16 = pModificationData->transportData.destinationPort;
            
                     status = KrnlHlprTCPHeaderModifyDestinationPort(&dstPort,
                                                                     pNetBufferList,
                                                                     tcpHeaderSize);
                     HLPR_BAIL_ON_FAILURE_2(status);
                  }
            
                  break;
               }
               case IPPROTO_UDP:
               {
                  UINT32 udpHeaderSize = UDP_HEADER_MIN_SIZE;
            
                  if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                                    FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE))
                     udpHeaderSize = pMetadata->transportHeaderSize;
            
                  if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_SOURCE_PORT)
                  {
                     FWP_VALUE0 srcPort;
            
                     srcPort.type   = FWP_UINT16;
                     srcPort.uint16 = pModificationData->transportData.sourcePort;
            
                     status = KrnlHlprUDPHeaderModifySourcePort(&srcPort,
                                                                pNetBufferList,
                                                                udpHeaderSize);
                     HLPR_BAIL_ON_FAILURE_2(status);
                  }
            
                  if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_DESTINATION_PORT)
                  {
                     FWP_VALUE0 dstPort;
            
                     dstPort.type   = FWP_UINT16;
                     dstPort.uint16 = pModificationData->transportData.destinationPort;
            
                     status = KrnlHlprUDPHeaderModifyDestinationPort(&dstPort,
                                                                     pNetBufferList,
                                                                     udpHeaderSize);
                     HLPR_BAIL_ON_FAILURE_2(status);
                  }
            
                  break;
               }
               case IPPROTO_ICMPV6:
               {
                  UINT32 icmpHeaderSize = ICMP_HEADER_MIN_SIZE;
            
                  if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                                    FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE))
                     icmpHeaderSize = pMetadata->transportHeaderSize;
            
                  if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_ICMP_TYPE)
                  {
                     FWP_VALUE0 icmpType;
            
                     icmpType.type  = FWP_UINT8;
                     icmpType.uint8 = (UINT8)ntohs(pModificationData->transportData.sourcePort);
            
                     status = KrnlHlprICMPv6HeaderModifyType(&icmpType,
                                                             pNetBufferList,
                                                             icmpHeaderSize);
                     HLPR_BAIL_ON_FAILURE_2(status);
                  }
            
                  if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_ICMP_CODE)
                  {
                     FWP_VALUE0 icmpCode;
            
                     icmpCode.type  = FWP_UINT8;
                     icmpCode.uint8 = (UINT8)ntohs(pModificationData->transportData.destinationPort);
            
                     status = KrnlHlprICMPv6HeaderModifyCode(&icmpCode,
                                                             pNetBufferList,
                                                             icmpHeaderSize);
                     HLPR_BAIL_ON_FAILURE_2(status);
                  }
            
                  break;
               }
            }

            /// ToDo: Recalculate the Transport Checksum Here

            HLPR_BAIL_LABEL_2:

            /// return the data offset to the beginning of the IP Header
            tmpStatus = NdisRetreatNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB(pNetBufferList),
                                                      ipHeaderSize,
                                                      0,
                                                      0);
            if(tmpStatus != STATUS_SUCCESS)
            {
               DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                          DPFLTR_ERROR_LEVEL,
                          " !!!! PerformBasicPacketModificationAtInboundMACFrame: NdisRetreatNetBufferDataStart() [status: %#x]\n",
                          status);
            
               HLPR_BAIL;
            }

            HLPR_BAIL_ON_FAILURE(status);
         }

         /// return the data offset to the beginning of the Ethernet Header
         tmpStatus = NdisRetreatNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB(pNetBufferList),
                                                   ipHeaderSize,
                                                   0,
                                                   0);
         if(tmpStatus != STATUS_SUCCESS)
         {
            DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                       DPFLTR_ERROR_LEVEL,
                       " !!!! PerformBasicPacketModificationAtInboundMACFrame: NdisRetreatNetBufferDataStart() [status: %#x]\n",
                       status);

            HLPR_BAIL;
         }
      }

      if(pModificationData->flags & PCPMDF_MODIFY_IP_HEADER)
      {
         /// The clone is at the Ethernet Header, so advance by the size of the Ethernet Header...
         NdisAdvanceNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB(pNetBufferList),
                                       ethernetHeaderSize,
                                       FALSE,
                                       0);

         if(pModificationData->ipData.flags & PCPMDF_MODIFY_IP_HEADER_SOURCE_ADDRESS)
         {
            FWP_VALUE value;

            RtlZeroMemory(&value,
                          sizeof(FWP_VALUE));

            if(pCompletionData->pInjectionData->addressFamily == AF_INET)
            {
               value.type = FWP_UINT32;

               RtlCopyMemory(&(value.uint32),
                             pModificationData->ipData.sourceAddress.pIPv4,
                             IPV4_ADDRESS_SIZE);
            }
            else
            {
               HLPR_NEW(value.byteArray16,
                        FWP_BYTE_ARRAY16,
                        WFPSAMPLER_CALLOUT_DRIVER_TAG);
               HLPR_BAIL_ON_ALLOC_FAILURE_WITH_LABEL(value.byteArray16,
                                                     status,
                                                     HLPR_BAIL_LABEL_3);

               value.type = FWP_BYTE_ARRAY16_TYPE;

               RtlCopyMemory(value.byteArray16->byteArray16,
                             pModificationData->ipData.sourceAddress.pIPv6,
                             IPV6_ADDRESS_SIZE);
            }

            status = KrnlHlprIPHeaderModifySourceAddress(&value,
                                                         pNetBufferList,
                                                         TRUE);

            KrnlHlprFwpValuePurgeLocalCopy(&value);

            HLPR_BAIL_ON_FAILURE_WITH_LABEL(status,
                                            HLPR_BAIL_LABEL_3);
         }

         if(pModificationData->ipData.flags & PCPMDF_MODIFY_IP_HEADER_DESTINATION_ADDRESS)
         {
            FWP_VALUE value;

            RtlZeroMemory(&value,
                          sizeof(FWP_VALUE));

            if(pCompletionData->pInjectionData->addressFamily == AF_INET)
            {
               value.type = FWP_UINT32;

               RtlCopyMemory(&(value.uint32),
                             pModificationData->ipData.destinationAddress.pIPv4,
                             IPV4_ADDRESS_SIZE);
            }
            else
            {
               HLPR_NEW(value.byteArray16,
                        FWP_BYTE_ARRAY16,
                        WFPSAMPLER_CALLOUT_DRIVER_TAG);
               HLPR_BAIL_ON_ALLOC_FAILURE_WITH_LABEL(value.byteArray16,
                                                     status,
                                                     HLPR_BAIL_LABEL_3);

               value.type = FWP_BYTE_ARRAY16_TYPE;

               RtlCopyMemory(value.byteArray16->byteArray16,
                             pModificationData->ipData.destinationAddress.pIPv6,
                             IPV6_ADDRESS_SIZE);
            }

            status = KrnlHlprIPHeaderModifyDestinationAddress(&value,
                                                              pNetBufferList,
                                                              TRUE);

            KrnlHlprFwpValuePurgeLocalCopy(&value);

            HLPR_BAIL_ON_FAILURE_WITH_LABEL(status,
                                            HLPR_BAIL_LABEL_3);
         }

         HLPR_BAIL_LABEL_3:

         /// return the data offset to the beginning of the Ethernet Header
         tmpStatus = NdisRetreatNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB(pNetBufferList),
                                                   ipHeaderSize,
                                                   0,
                                                   0);
         if(tmpStatus != STATUS_SUCCESS)
         {
            DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                       DPFLTR_ERROR_LEVEL,
                       " !!!! PerformBasicPacketModificationAtInboundMACFrame: NdisRetreatNetBufferDataStart() [status: %#x]\n",
                       status);
         
            HLPR_BAIL;
         }
      }
*/
      if(pModificationData->flags & PCPMDF_MODIFY_MAC_HEADER)
      {
         if(pModificationData->macData.flags & PCPMDF_MODIFY_MAC_HEADER_SOURCE_ADDRESS)
         {
            FWP_VALUE value;

            RtlZeroMemory(&value,
                          sizeof(FWP_VALUE));

            HLPR_NEW(value.byteArray6,
                     FWP_BYTE_ARRAY6,
                     WFPSAMPLER_CALLOUT_DRIVER_TAG);
            HLPR_BAIL_ON_ALLOC_FAILURE(value.byteArray6,
                                       status);
            
            value.type = FWP_BYTE_ARRAY6_TYPE;
            
            RtlCopyMemory(value.byteArray6->byteArray6,
                          pModificationData->macData.pSourceMACAddress,
                          ETHERNET_ADDRESS_SIZE);

            status = KrnlHlprMACHeaderModifySourceAddress(&value,
                                                          pNetBufferList);

            KrnlHlprFwpValuePurgeLocalCopy(&value);

            HLPR_BAIL_ON_FAILURE(status);
         }

         if(pModificationData->macData.flags & PCPMDF_MODIFY_MAC_HEADER_DESTINATION_ADDRESS)
         {
            FWP_VALUE value;

            RtlZeroMemory(&value,
                          sizeof(FWP_VALUE));

#pragma warning(push)
#pragma warning(disable: 6014) /// value.byteArray6 will be freed in with call to KrnlHlprFwpValuePurgeLocalCopy

            HLPR_NEW(value.byteArray6,
                     FWP_BYTE_ARRAY6,
                     WFPSAMPLER_CALLOUT_DRIVER_TAG);
            HLPR_BAIL_ON_ALLOC_FAILURE(value.byteArray6,
                                       status);

#pragma warning(pop)
            
            value.type = FWP_BYTE_ARRAY6_TYPE;
            
            RtlCopyMemory(value.byteArray6->byteArray6,
                          pModificationData->macData.pDestinationMACAddress,
                          ETHERNET_ADDRESS_SIZE);

            status = KrnlHlprMACHeaderModifyDestinationAddress(&value,
                                                               pNetBufferList);

            KrnlHlprFwpValuePurgeLocalCopy(&value);

            HLPR_BAIL_ON_FAILURE(status);
         }
      }
   }

   pCompletionData->refCount = KrnlHlprNBLGetRequiredRefCount(pNetBufferList,
                                                              TRUE);

   status = FwpsInjectMacReceiveAsync(pCompletionData->pInjectionData->injectionHandle,
                                      pCompletionData->pInjectionData->injectionContext,
                                      0,
                                      pClassifyValues->layerId,
                                      interfaceIndex,
                                      ndisPort,
                                      pNetBufferList,
                                      CompleteBasicPacketModification,
                                      pCompletionData);

   NT_ASSERT(irql == KeGetCurrentIrql());

   if(status != STATUS_SUCCESS)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! PerformBasicPacketModificationAtInboundMACFrame: FwpsInjectMacReceiveAsync() [status: %#x]\n",
                 status);

   HLPR_BAIL_LABEL:

   NT_ASSERT(status == STATUS_SUCCESS);

   if(status != STATUS_SUCCESS)
   {
      if(pNetBufferList)
      {
         FwpsFreeCloneNetBufferList(pNetBufferList,
                                    0);

         pNetBufferList = 0;
      }

      if(pCompletionData)
         BasicPacketModificationCompletionDataDestroy(&pCompletionData,
                                                      TRUE);
   }

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- PerformBasicPacketModificationAtInboundMACFrame() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

/**
 @private_function="PerformBasicPacketInjectionAtOutboundMACFrame"
 
   Purpose:  Clones the NET_BUFFER_LIST and injects the clone back to the stack from the 
             outgoing MAC Layers using FwpsInjectMacSendAsync().                                <br>
                                                                                                <br>
   Notes:    Applies to the following inbound layers:                                           <br>
                FWPM_LAYER_OUTBOUND_MAC_FRAME_ETHERNET                                          <br>
                FWPM_LAYER_OUTBOUND_MAC_FRAME_NATIVE                                            <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/HH439593.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF546324.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
NTSTATUS PerformBasicPacketModificationAtOutboundMACFrame(_In_ CLASSIFY_DATA** ppClassifyData,
                                                          _In_ INJECTION_DATA** ppInjectionData,
                                                          _In_ PC_BASIC_PACKET_MODIFICATION_DATA* pModificationData,
                                                          _In_ BOOLEAN isInline = FALSE)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> PerformBasicPacketModificationAtOutboundMACFrame()\n");

#endif ///  DBG

   NT_ASSERT(ppClassifyData);
   NT_ASSERT(ppInjectionData);
   NT_ASSERT(pModificationData);
   NT_ASSERT(*ppClassifyData);
   NT_ASSERT(*ppInjectionData);

   NTSTATUS                                   status          = STATUS_SUCCESS;
   FWPS_INCOMING_VALUES*                      pClassifyValues = (FWPS_INCOMING_VALUES*)(*ppClassifyData)->pClassifyValues;
   IF_INDEX                                   interfaceIndex  = 0;
   NDIS_PORT_NUMBER                           ndisPort        = 0;
   NET_BUFFER_LIST*                           pNetBufferList  = 0;
   BASIC_PACKET_MODIFICATION_COMPLETION_DATA* pCompletionData = 0;
   FWP_VALUE*                                 pInterfaceIndex = 0;
   FWP_VALUE*                                 pNDISPort       = 0;

#if DBG

   KIRQL                                      irql            = KeGetCurrentIrql();

#endif /// DBG

#pragma warning(push)
#pragma warning(disable: 6014) /// pCompletionData will be freed in completionFn using BasicPacketModificationCompletionDataDestroy

   HLPR_NEW(pCompletionData,
            BASIC_PACKET_MODIFICATION_COMPLETION_DATA,
            WFPSAMPLER_CALLOUT_DRIVER_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pCompletionData,
                              status);

#pragma warning(pop)

   KeInitializeSpinLock(&(pCompletionData->spinLock));

   pCompletionData->performedInline = isInline;
   pCompletionData->pClassifyData   = *ppClassifyData;
   pCompletionData->pInjectionData  = *ppInjectionData;

   /// Responsibility for freeing this memory has been transferred to the pCompletionData
   *ppClassifyData = 0;

   *ppInjectionData = 0;

   pInterfaceIndex = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                               &FWPM_CONDITION_INTERFACE_INDEX);
   if(pInterfaceIndex &&
      pInterfaceIndex->type == FWP_UINT32)
      interfaceIndex = (IF_INDEX)pInterfaceIndex->uint32;

   pNDISPort = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                         &FWPM_CONDITION_NDIS_PORT);
   if(pNDISPort &&
      pNDISPort->type == FWP_UINT32)
      ndisPort = (NDIS_PORT_NUMBER)pNDISPort->uint32;

   /// Initial offset is at the MAC Header, so just clone the entire NET_BUFFER_LIST.
   status = FwpsAllocateCloneNetBufferList((NET_BUFFER_LIST*)pCompletionData->pClassifyData->pPacket,
                                           g_pNDISPoolData->nblPoolHandle,
                                           g_pNDISPoolData->nbPoolHandle,
                                           0,
                                           &pNetBufferList);
   if(status != STATUS_SUCCESS)
   {
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! PerformBasicPacketModificationAtOutboundMACFrame: FwpsAllocateCloneNetBufferList() [status: %#x]\n",
                 status);

      HLPR_BAIL;
   }

   pCompletionData->refCount = KrnlHlprNBLGetRequiredRefCount(pNetBufferList,
                                                              TRUE);

   if(pModificationData->flags)
   {
      /// Various checks and balances must be performed to modify the IP and Transport headers at this modification point.
      /// Parsing of the headers will need to occur, as well as spot checking to verify everything is as it should be.
      /// Additionally, checksum routines will need to be written to recalculate checksums for some of the headers.
      /// The following block of code is to get you started with modifying the headers with info not readily available.
      /// (i.e. header parsing has not occurred so there is no relevant classifiable data nor metadata present).
/*
      if(pModificationData->flags & PCPMDF_MODIFY_TRANSPORT_HEADER)
      {
         UINT32  tmpStatus = STATUS_SUCCESS;
         IPPROTO protocol  = IPPROTO_MAX;

         /// The clone is at the Ethernet Header, so advance by the size of the Ethernet Header...
         NdisAdvanceNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB(pNetBufferList),
                                       ethernetHeaderSize,
                                       FALSE,
                                       0);

         protocol = KrnlHlprIPHeaderGetProtocolField(pNetBufferList,
                                                     pCompletionData->pInjectionData->addressFamily);

         /// No Transport Modification if IPsec encrypted
         if(protocol != IPPROTO_ESP &&
            protocol != IPPROTO_AH)
         {
            /// ... advance by the size of the IP Header.
            NdisAdvanceNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB(pNetBufferList),
                                          ipHeaderSize,
                                          FALSE,
                                          0);

            switch(protocol)
            {
               case IPPROTO_ICMP:
               {
                  UINT32 icmpHeaderSize = ICMP_HEADER_MIN_SIZE;
            
                  if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                                    FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE))
                     icmpHeaderSize = pMetadata->transportHeaderSize;
            
                  if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_ICMP_TYPE)
                  {
                     FWP_VALUE0 icmpType;
            
                     icmpType.type  = FWP_UINT8;
                     icmpType.uint8 = (UINT8)ntohs(pModificationData->transportData.sourcePort);
            
                     status = KrnlHlprICMPv4HeaderModifyType(&icmpType,
                                                             pNetBufferList,
                                                             icmpHeaderSize);
                     HLPR_BAIL_ON_FAILURE_2(status);
                  }
            
                  if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_ICMP_CODE)
                  {
                     FWP_VALUE0 icmpCode;
                  
                     icmpCode.type  = FWP_UINT8;
                     icmpCode.uint8 = (UINT8)ntohs(pModificationData->transportData.destinationPort);
            
                     status = KrnlHlprICMPv4HeaderModifyCode(&icmpCode,
                                                             pNetBufferList,
                                                             icmpHeaderSize);
                     HLPR_BAIL_ON_FAILURE_2(status);
                  }
            
                  break;
               }
               case IPPROTO_TCP:
               {
                  UINT32 tcpHeaderSize = TCP_HEADER_MIN_SIZE;
            
                  if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                                    FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE))
                     tcpHeaderSize = pMetadata->transportHeaderSize;
            
                  if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_SOURCE_PORT)
                  {
                     FWP_VALUE0 srcPort;
            
                     srcPort.type   = FWP_UINT16;
                     srcPort.uint16 = pModificationData->transportData.sourcePort;
            
                     status = KrnlHlprTCPHeaderModifySourcePort(&srcPort,
                                                                pNetBufferList,
                                                                tcpHeaderSize);
                     HLPR_BAIL_ON_FAILURE_2(status);
                  }
            
                  if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_DESTINATION_PORT)
                  {
                     FWP_VALUE0 dstPort;
            
                     dstPort.type   = FWP_UINT16;
                     dstPort.uint16 = pModificationData->transportData.destinationPort;
            
                     status = KrnlHlprTCPHeaderModifyDestinationPort(&dstPort,
                                                                     pNetBufferList,
                                                                     tcpHeaderSize);
                     HLPR_BAIL_ON_FAILURE_2(status);
                  }
            
                  break;
               }
               case IPPROTO_UDP:
               {
                  UINT32 udpHeaderSize = UDP_HEADER_MIN_SIZE;
            
                  if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                                    FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE))
                     udpHeaderSize = pMetadata->transportHeaderSize;
            
                  if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_SOURCE_PORT)
                  {
                     FWP_VALUE0 srcPort;
            
                     srcPort.type   = FWP_UINT16;
                     srcPort.uint16 = pModificationData->transportData.sourcePort;
            
                     status = KrnlHlprUDPHeaderModifySourcePort(&srcPort,
                                                                pNetBufferList,
                                                                udpHeaderSize);
                     HLPR_BAIL_ON_FAILURE_2(status);
                  }
            
                  if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_DESTINATION_PORT)
                  {
                     FWP_VALUE0 dstPort;
            
                     dstPort.type   = FWP_UINT16;
                     dstPort.uint16 = pModificationData->transportData.destinationPort;
            
                     status = KrnlHlprUDPHeaderModifyDestinationPort(&dstPort,
                                                                     pNetBufferList,
                                                                     udpHeaderSize);
                     HLPR_BAIL_ON_FAILURE_2(status);
                  }
            
                  break;
               }
               case IPPROTO_ICMPV6:
               {
                  UINT32 icmpHeaderSize = ICMP_HEADER_MIN_SIZE;
            
                  if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                                    FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE))
                     icmpHeaderSize = pMetadata->transportHeaderSize;
            
                  if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_ICMP_TYPE)
                  {
                     FWP_VALUE0 icmpType;
            
                     icmpType.type  = FWP_UINT8;
                     icmpType.uint8 = (UINT8)ntohs(pModificationData->transportData.sourcePort);
            
                     status = KrnlHlprICMPv6HeaderModifyType(&icmpType,
                                                             pNetBufferList,
                                                             icmpHeaderSize);
                     HLPR_BAIL_ON_FAILURE_2(status);
                  }
            
                  if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_ICMP_CODE)
                  {
                     FWP_VALUE0 icmpCode;
            
                     icmpCode.type  = FWP_UINT8;
                     icmpCode.uint8 = (UINT8)ntohs(pModificationData->transportData.destinationPort);
            
                     status = KrnlHlprICMPv6HeaderModifyCode(&icmpCode,
                                                             pNetBufferList,
                                                             icmpHeaderSize);
                     HLPR_BAIL_ON_FAILURE_2(status);
                  }
            
                  break;
               }
            }

            /// ToDo: Recalculate the Transport Checksum Here

            HLPR_BAIL_LABEL_2:

            /// return the data offset to the beginning of the IP Header
            tmpStatus = NdisRetreatNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB(pNetBufferList),
                                                      ipHeaderSize,
                                                      0,
                                                      0);
            if(tmpStatus != STATUS_SUCCESS)
            {
               DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                          DPFLTR_ERROR_LEVEL,
                          " !!!! PerformBasicPacketModificationAtOutboundMacFrame: NdisRetreatNetBufferDataStart() [status: %#x]\n",
                          status);
            
               HLPR_BAIL;
            }

            HLPR_BAIL_ON_FAILURE(status);
         }

         /// return the data offset to the beginning of the Ethernet Header
         tmpStatus = NdisRetreatNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB(pNetBufferList),
                                                   ipHeaderSize,
                                                   0,
                                                   0);
         if(tmpStatus != STATUS_SUCCESS)
         {
            DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                       DPFLTR_ERROR_LEVEL,
                       " !!!! PerformBasicPacketModificationAtOutboundMACFrame: NdisRetreatNetBufferDataStart() [status: %#x]\n",
                       status);

            HLPR_BAIL;
         }
      }

      if(pModificationData->flags & PCPMDF_MODIFY_IP_HEADER)
      {
         /// The clone is at the Ethernet Header, so advance by the size of the Ethernet Header...
         NdisAdvanceNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB(pNetBufferList),
                                       ethernetHeaderSize,
                                       FALSE,
                                       0);

         if(pModificationData->ipData.flags & PCPMDF_MODIFY_IP_HEADER_SOURCE_ADDRESS)
         {
            FWP_VALUE value;

            RtlZeroMemory(&value,
                          sizeof(FWP_VALUE));

            if(pCompletionData->pInjectionData->addressFamily == AF_INET)
            {
               value.type = FWP_UINT32;

               RtlCopyMemory(&(value.uint32),
                             pModificationData->ipData.sourceAddress.pIPv4,
                             IPV4_ADDRESS_SIZE);
            }
            else
            {
               HLPR_NEW(value.byteArray16,
                        FWP_BYTE_ARRAY16,
                        WFPSAMPLER_CALLOUT_DRIVER_TAG);
               HLPR_BAIL_ON_ALLOC_FAILURE_WITH_LABEL(value.byteArray16,
                                                     status,
                                                     HLPR_BAIL_LABEL_3);

               value.type = FWP_BYTE_ARRAY16_TYPE;

               RtlCopyMemory(value.byteArray16->byteArray16,
                             pModificationData->ipData.sourceAddress.pIPv6,
                             IPV6_ADDRESS_SIZE);
            }

            status = KrnlHlprIPHeaderModifySourceAddress(&value,
                                                         pNetBufferList,
                                                         TRUE);

            KrnlHlprFwpValuePurgeLocalCopy(&value);

            HLPR_BAIL_ON_FAILURE_WITH_LABEL(status,
                                            HLPR_BAIL_LABEL_3);
         }

         if(pModificationData->ipData.flags & PCPMDF_MODIFY_IP_HEADER_DESTINATION_ADDRESS)
         {
            FWP_VALUE value;

            RtlZeroMemory(&value,
                          sizeof(FWP_VALUE));

            if(pCompletionData->pInjectionData->addressFamily == AF_INET)
            {
               value.type = FWP_UINT32;

               RtlCopyMemory(&(value.uint32),
                             pModificationData->ipData.destinationAddress.pIPv4,
                             IPV4_ADDRESS_SIZE);
            }
            else
            {
               HLPR_NEW(value.byteArray16,
                        FWP_BYTE_ARRAY16,
                        WFPSAMPLER_CALLOUT_DRIVER_TAG);
               HLPR_BAIL_ON_ALLOC_FAILURE_WITH_LABEL(value.byteArray16,
                                                     status,
                                                     HLPR_BAIL_LABEL_3);

               value.type = FWP_BYTE_ARRAY16_TYPE;

               RtlCopyMemory(value.byteArray16->byteArray16,
                             pModificationData->ipData.destinationAddress.pIPv6,
                             IPV6_ADDRESS_SIZE);
            }

            status = KrnlHlprIPHeaderModifyDestinationAddress(&value,
                                                              pNetBufferList,
                                                              TRUE);

            KrnlHlprFwpValuePurgeLocalCopy(&value);

            HLPR_BAIL_ON_FAILURE_WITH_LABEL(status,
                                            HLPR_BAIL_LABEL_3);
         }

         HLPR_BAIL_LABEL_3:

         /// return the data offset to the beginning of the Ethernet Header
         tmpStatus = NdisRetreatNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB(pNetBufferList),
                                                   ipHeaderSize,
                                                   0,
                                                   0);
         if(tmpStatus != STATUS_SUCCESS)
         {
            DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                       DPFLTR_ERROR_LEVEL,
                       " !!!! PerformBasicPacketModificationAtOutboundMACFrame: NdisRetreatNetBufferDataStart() [status: %#x]\n",
                       status);
         
            HLPR_BAIL;
         }
      }
*/
         if(pModificationData->flags & PCPMDF_MODIFY_MAC_HEADER)
         {
            if(pModificationData->macData.flags & PCPMDF_MODIFY_MAC_HEADER_SOURCE_ADDRESS)
            {
               FWP_VALUE value;
   
               RtlZeroMemory(&value,
                             sizeof(FWP_VALUE));
   
               HLPR_NEW(value.byteArray6,
                        FWP_BYTE_ARRAY6,
                        WFPSAMPLER_CALLOUT_DRIVER_TAG);
               HLPR_BAIL_ON_ALLOC_FAILURE(value.byteArray6,
                                          status);
               
               value.type = FWP_BYTE_ARRAY6_TYPE;
               
               RtlCopyMemory(value.byteArray6->byteArray6,
                             pModificationData->macData.pSourceMACAddress,
                             ETHERNET_ADDRESS_SIZE);
   
               status = KrnlHlprMACHeaderModifySourceAddress(&value,
                                                             pNetBufferList);
   
               KrnlHlprFwpValuePurgeLocalCopy(&value);
   
               HLPR_BAIL_ON_FAILURE(status);
            }
   
            if(pModificationData->macData.flags & PCPMDF_MODIFY_MAC_HEADER_DESTINATION_ADDRESS)
            {
               FWP_VALUE value;
   
               RtlZeroMemory(&value,
                             sizeof(FWP_VALUE));
   
#pragma warning(push)
#pragma warning(disable: 6014) /// value.byteArray6 will be freed in with call to KrnlHlprFwpValuePurgeLocalCopy

               HLPR_NEW(value.byteArray6,
                        FWP_BYTE_ARRAY6,
                        WFPSAMPLER_CALLOUT_DRIVER_TAG);
               HLPR_BAIL_ON_ALLOC_FAILURE(value.byteArray6,
                                          status);

#pragma warning(pop)
               
               value.type = FWP_BYTE_ARRAY6_TYPE;
               
               RtlCopyMemory(value.byteArray6->byteArray6,
                             pModificationData->macData.pDestinationMACAddress,
                             ETHERNET_ADDRESS_SIZE);
   
               status = KrnlHlprMACHeaderModifyDestinationAddress(&value,
                                                                  pNetBufferList);
   
               KrnlHlprFwpValuePurgeLocalCopy(&value);
   
               HLPR_BAIL_ON_FAILURE(status);
            }
         }
      }

   status = FwpsInjectMacSendAsync(pCompletionData->pInjectionData->injectionHandle,
                                   pCompletionData->pInjectionData->injectionContext,
                                   0,
                                   pClassifyValues->layerId,
                                   interfaceIndex,
                                   ndisPort,
                                   pNetBufferList,
                                   CompleteBasicPacketModification,
                                   pCompletionData);

   NT_ASSERT(irql == KeGetCurrentIrql());

   if(status != STATUS_SUCCESS)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! PerformBasicPacketModificationAtOutboundMACFrame: FwpsInjectMacSendAsync() [status: %#x]\n",
                 status);

   HLPR_BAIL_LABEL:

   NT_ASSERT(status == STATUS_SUCCESS);

   if(status != STATUS_SUCCESS)
   {
      if(pNetBufferList)
      {
         FwpsFreeCloneNetBufferList(pNetBufferList,
                                    0);

         pNetBufferList = 0;
      }

      if(pCompletionData)
         BasicPacketModificationCompletionDataDestroy(&pCompletionData,
                                                      TRUE);
   }

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- PerformBasicPacketModificationAtOutboundMACFrame() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

/**
 @private_function="PerformBasicPacketModificationAtIngressVSwitchEthernet"
 
   Purpose:  Clones the NET_BUFFER_LIST and injects the clone back to the virtual switch's 
             ingress path from the ingress VSwitch Layers using 
             FwpsInjectvSwitchEthernetIngressAsync0().                                          <br>
                                                                                                <br>
   Notes:    Applies to the following ingress layers:                                           <br>
                FWPM_LAYER_INGRESS_VSWITCH_ETHERNET                                             <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/HH439669.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF546324.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
NTSTATUS PerformBasicPacketModificationAtIngressVSwitchEthernet(_In_ CLASSIFY_DATA** ppClassifyData,
                                                                _In_ INJECTION_DATA** ppInjectionData,
                                                                _In_ PC_BASIC_PACKET_MODIFICATION_DATA* pModificationData,
                                                                _In_ BOOLEAN isInline = FALSE)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> PerformBasicPacketModificationAtIngressVSwitchEthernet()\n");

#endif /// DBG

   NT_ASSERT(ppClassifyData);
   NT_ASSERT(ppInjectionData);
   NT_ASSERT(pModificationData);
   NT_ASSERT(*ppClassifyData);
   NT_ASSERT(*ppInjectionData);

   NTSTATUS                                   status          = STATUS_SUCCESS;
   FWPS_INCOMING_VALUES*                      pClassifyValues = (FWPS_INCOMING_VALUES*)(*ppClassifyData)->pClassifyValues;
   FWPS_INCOMING_METADATA_VALUES*             pMetadata       = (FWPS_INCOMING_METADATA_VALUES*)(*ppClassifyData)->pMetadataValues;
   FWP_VALUE*                                 pVSwitchIDValue = 0;
   FWP_BYTE_BLOB*                             pVSwitchID      = 0;
   NDIS_SWITCH_PORT_ID                        sourcePortID    = 0;
   NDIS_SWITCH_NIC_INDEX                      sourceNICIndex  = 0;
   NET_BUFFER_LIST*                           pNetBufferList  = 0;
   BASIC_PACKET_MODIFICATION_COMPLETION_DATA* pCompletionData = 0;

#if DBG

   KIRQL                                      irql            = KeGetCurrentIrql();

#endif /// DBG

#pragma warning(push)
#pragma warning(disable: 6014) /// pCompletionData will be freed in completionFn using BasicPacketModificationCompletionDataDestroy

   HLPR_NEW(pCompletionData,
            BASIC_PACKET_MODIFICATION_COMPLETION_DATA,
            WFPSAMPLER_CALLOUT_DRIVER_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pCompletionData,
                              status);

#pragma warning(pop)

   KeInitializeSpinLock(&(pCompletionData->spinLock));

   pCompletionData->performedInline = isInline;
   pCompletionData->pClassifyData   = *ppClassifyData;
   pCompletionData->pInjectionData  = *ppInjectionData;

   /// Responsibility for freeing this memory has been transferred to the pCompletionData
   *ppClassifyData = 0;

   *ppInjectionData = 0;

   if(FWPS_IS_L2_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_L2_METADATA_FIELD_VSWITCH_SOURCE_PORT_ID))
      sourcePortID = pMetadata->vSwitchSourcePortId;

   if(FWPS_IS_L2_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_L2_METADATA_FIELD_VSWITCH_SOURCE_NIC_INDEX))
      sourceNICIndex = (NDIS_SWITCH_NIC_INDEX)pMetadata->vSwitchSourceNicIndex;

   pVSwitchIDValue = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                               &FWPM_CONDITION_VSWITCH_ID);
   if(pVSwitchIDValue)
      pVSwitchID = pVSwitchIDValue->byteBlob;

   if(pVSwitchID == 0)
   {
      status = STATUS_INVALID_MEMBER;

       DbgPrintEx(DPFLTR_IHVNETWORK_ID,
           DPFLTR_ERROR_LEVEL,
           " !!!! PerformBasicPacketModificationAtIngressVSwitchEthernet() [status: %#x][pVSwitchID: %#p]\n",
           status,
           pVSwitchID);

      HLPR_BAIL;
   }

   /// Initial offset is at the MAC Header, so just clone the entire NET_BUFFER_LIST.
   status = FwpsAllocateCloneNetBufferList((NET_BUFFER_LIST*)pCompletionData->pClassifyData->pPacket,
                                           g_pNDISPoolData->nblPoolHandle,
                                           g_pNDISPoolData->nbPoolHandle,
                                           0,
                                           &pNetBufferList);
   if(status != STATUS_SUCCESS)
   {
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! PerformBasicPacketModificationAtIngressVSwitchEthernet: FwpsAllocateCloneNetBufferList() [status: %#x]\n",
                 status);

      HLPR_BAIL;
   }

   pCompletionData->refCount = KrnlHlprNBLGetRequiredRefCount(pNetBufferList,
                                                              TRUE);

   if(pModificationData->flags)
   {
      /// Various checks and balances must be performed to modify the IP and Transport headers at this modification point.
      /// Parsing of the headers will need to occur, as well as spot checking to verify everything is as it should be.
      /// Additionally, checksum routines will need to be written to recalculate checksums for some of the headers.
      /// The following block of code is to get you started with modifying the headers with info not readily available.
      /// (i.e. header parsing has not occurred so there is no relevant classifiable data nor metadata present).
/*
      if(pModificationData->flags & PCPMDF_MODIFY_TRANSPORT_HEADER)
      {
         UINT32  tmpStatus = STATUS_SUCCESS;
         IPPROTO protocol  = IPPROTO_MAX;

         /// The clone is at the Ethernet Header, so advance by the size of the Ethernet Header...
         NdisAdvanceNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB(pNetBufferList),
                                       ethernetHeaderSize,
                                       FALSE,
                                       0);

         protocol = KrnlHlprIPHeaderGetProtocolField(pNetBufferList,
                                                     pCompletionData->pInjectionData->addressFamily);

         /// No Transport Modification if IPsec encrypted
         if(protocol != IPPROTO_ESP &&
            protocol != IPPROTO_AH)
         {
            /// ... advance by the size of the IP Header.
            NdisAdvanceNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB(pNetBufferList),
                                          ipHeaderSize,
                                          FALSE,
                                          0);

            switch(protocol)
            {
               case IPPROTO_ICMP:
               {
                  UINT32 icmpHeaderSize = ICMP_HEADER_MIN_SIZE;
            
                  if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                                    FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE))
                     icmpHeaderSize = pMetadata->transportHeaderSize;
            
                  if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_ICMP_TYPE)
                  {
                     FWP_VALUE0 icmpType;
            
                     icmpType.type  = FWP_UINT8;
                     icmpType.uint8 = (UINT8)ntohs(pModificationData->transportData.sourcePort);
            
                     status = KrnlHlprICMPv4HeaderModifyType(&icmpType,
                                                             pNetBufferList,
                                                             icmpHeaderSize);
                     HLPR_BAIL_ON_FAILURE_2(status);
                  }
            
                  if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_ICMP_CODE)
                  {
                     FWP_VALUE0 icmpCode;
                  
                     icmpCode.type  = FWP_UINT8;
                     icmpCode.uint8 = (UINT8)ntohs(pModificationData->transportData.destinationPort);
            
                     status = KrnlHlprICMPv4HeaderModifyCode(&icmpCode,
                                                             pNetBufferList,
                                                             icmpHeaderSize);
                     HLPR_BAIL_ON_FAILURE_2(status);
                  }
            
                  break;
               }
               case IPPROTO_TCP:
               {
                  UINT32 tcpHeaderSize = TCP_HEADER_MIN_SIZE;
            
                  if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                                    FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE))
                     tcpHeaderSize = pMetadata->transportHeaderSize;
            
                  if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_SOURCE_PORT)
                  {
                     FWP_VALUE0 srcPort;
            
                     srcPort.type   = FWP_UINT16;
                     srcPort.uint16 = pModificationData->transportData.sourcePort;
            
                     status = KrnlHlprTCPHeaderModifySourcePort(&srcPort,
                                                                pNetBufferList,
                                                                tcpHeaderSize);
                     HLPR_BAIL_ON_FAILURE_2(status);
                  }
            
                  if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_DESTINATION_PORT)
                  {
                     FWP_VALUE0 dstPort;
            
                     dstPort.type   = FWP_UINT16;
                     dstPort.uint16 = pModificationData->transportData.destinationPort;
            
                     status = KrnlHlprTCPHeaderModifyDestinationPort(&dstPort,
                                                                     pNetBufferList,
                                                                     tcpHeaderSize);
                     HLPR_BAIL_ON_FAILURE_2(status);
                  }
            
                  break;
               }
               case IPPROTO_UDP:
               {
                  UINT32 udpHeaderSize = UDP_HEADER_MIN_SIZE;
            
                  if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                                    FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE))
                     udpHeaderSize = pMetadata->transportHeaderSize;
            
                  if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_SOURCE_PORT)
                  {
                     FWP_VALUE0 srcPort;
            
                     srcPort.type   = FWP_UINT16;
                     srcPort.uint16 = pModificationData->transportData.sourcePort;
            
                     status = KrnlHlprUDPHeaderModifySourcePort(&srcPort,
                                                                pNetBufferList,
                                                                udpHeaderSize);
                     HLPR_BAIL_ON_FAILURE_2(status);
                  }
            
                  if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_DESTINATION_PORT)
                  {
                     FWP_VALUE0 dstPort;
            
                     dstPort.type   = FWP_UINT16;
                     dstPort.uint16 = pModificationData->transportData.destinationPort;
            
                     status = KrnlHlprUDPHeaderModifyDestinationPort(&dstPort,
                                                                     pNetBufferList,
                                                                     udpHeaderSize);
                     HLPR_BAIL_ON_FAILURE_2(status);
                  }
            
                  break;
               }
               case IPPROTO_ICMPV6:
               {
                  UINT32 icmpHeaderSize = ICMP_HEADER_MIN_SIZE;
            
                  if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                                    FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE))
                     icmpHeaderSize = pMetadata->transportHeaderSize;
            
                  if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_ICMP_TYPE)
                  {
                     FWP_VALUE0 icmpType;
            
                     icmpType.type  = FWP_UINT8;
                     icmpType.uint8 = (UINT8)ntohs(pModificationData->transportData.sourcePort);
            
                     status = KrnlHlprICMPv6HeaderModifyType(&icmpType,
                                                             pNetBufferList,
                                                             icmpHeaderSize);
                     HLPR_BAIL_ON_FAILURE_2(status);
                  }
            
                  if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_ICMP_CODE)
                  {
                     FWP_VALUE0 icmpCode;
            
                     icmpCode.type  = FWP_UINT8;
                     icmpCode.uint8 = (UINT8)ntohs(pModificationData->transportData.destinationPort);
            
                     status = KrnlHlprICMPv6HeaderModifyCode(&icmpCode,
                                                             pNetBufferList,
                                                             icmpHeaderSize);
                     HLPR_BAIL_ON_FAILURE_2(status);
                  }
            
                  break;
               }
            }

            /// ToDo: Recalculate the Transport Checksum Here

            HLPR_BAIL_LABEL_2:

            /// return the data offset to the beginning of the IP Header
            tmpStatus = NdisRetreatNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB(pNetBufferList),
                                                      ipHeaderSize,
                                                      0,
                                                      0);
            if(tmpStatus != STATUS_SUCCESS)
            {
               DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                          DPFLTR_ERROR_LEVEL,
                          " !!!! PerformBasicPacketModificationAtIngressVSwitchEthernet: NdisRetreatNetBufferDataStart() [status: %#x]\n",
                          status);
            
               HLPR_BAIL;
            }

            HLPR_BAIL_ON_FAILURE(status);
         }

         /// return the data offset to the beginning of the Ethernet Header
         tmpStatus = NdisRetreatNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB(pNetBufferList),
                                                   ipHeaderSize,
                                                   0,
                                                   0);
         if(tmpStatus != STATUS_SUCCESS)
         {
            DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                       DPFLTR_ERROR_LEVEL,
                       " !!!! PerformBasicPacketModificationAtIngressVSwitchEthernet: NdisRetreatNetBufferDataStart() [status: %#x]\n",
                       status);

            HLPR_BAIL;
         }
      }

      if(pModificationData->flags & PCPMDF_MODIFY_IP_HEADER)
      {
         /// The clone is at the Ethernet Header, so advance by the size of the Ethernet Header...
         NdisAdvanceNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB(pNetBufferList),
                                       ethernetHeaderSize,
                                       FALSE,
                                       0);

         if(pModificationData->ipData.flags & PCPMDF_MODIFY_IP_HEADER_SOURCE_ADDRESS)
         {
            FWP_VALUE value;

            RtlZeroMemory(&value,
                          sizeof(FWP_VALUE));

            if(pCompletionData->pInjectionData->addressFamily == AF_INET)
            {
               value.type = FWP_UINT32;

               RtlCopyMemory(&(value.uint32),
                             pModificationData->ipData.sourceAddress.pIPv4,
                             IPV4_ADDRESS_SIZE);
            }
            else
            {
               HLPR_NEW(value.byteArray16,
                        FWP_BYTE_ARRAY16,
                        WFPSAMPLER_CALLOUT_DRIVER_TAG);
               HLPR_BAIL_ON_ALLOC_FAILURE_WITH_LABEL(value.byteArray16,
                                                     status,
                                                     HLPR_BAIL_LABEL_3);

               value.type = FWP_BYTE_ARRAY16_TYPE;

               RtlCopyMemory(value.byteArray16->byteArray16,
                             pModificationData->ipData.sourceAddress.pIPv6,
                             IPV6_ADDRESS_SIZE);
            }

            status = KrnlHlprIPHeaderModifySourceAddress(&value,
                                                         pNetBufferList,
                                                         TRUE);

            KrnlHlprFwpValuePurgeLocalCopy(&value);

            HLPR_BAIL_ON_FAILURE_WITH_LABEL(status,
                                            HLPR_BAIL_LABEL_3);
         }

         if(pModificationData->ipData.flags & PCPMDF_MODIFY_IP_HEADER_DESTINATION_ADDRESS)
         {
            FWP_VALUE value;

            RtlZeroMemory(&value,
                          sizeof(FWP_VALUE));

            if(pCompletionData->pInjectionData->addressFamily == AF_INET)
            {
               value.type = FWP_UINT32;

               RtlCopyMemory(&(value.uint32),
                             pModificationData->ipData.destinationAddress.pIPv4,
                             IPV4_ADDRESS_SIZE);
            }
            else
            {
               HLPR_NEW(value.byteArray16,
                        FWP_BYTE_ARRAY16,
                        WFPSAMPLER_CALLOUT_DRIVER_TAG);
               HLPR_BAIL_ON_ALLOC_FAILURE_WITH_LABEL(value.byteArray16,
                                                     status,
                                                     HLPR_BAIL_LABEL_3);

               value.type = FWP_BYTE_ARRAY16_TYPE;

               RtlCopyMemory(value.byteArray16->byteArray16,
                             pModificationData->ipData.destinationAddress.pIPv6,
                             IPV6_ADDRESS_SIZE);
            }

            status = KrnlHlprIPHeaderModifyDestinationAddress(&value,
                                                              pNetBufferList,
                                                              TRUE);

            KrnlHlprFwpValuePurgeLocalCopy(&value);

            HLPR_BAIL_ON_FAILURE_WITH_LABEL(status,
                                            HLPR_BAIL_LABEL_3);
         }

         HLPR_BAIL_LABEL_3:

         /// return the data offset to the beginning of the Ethernet Header
         tmpStatus = NdisRetreatNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB(pNetBufferList),
                                                   ipHeaderSize,
                                                   0,
                                                   0);
         if(tmpStatus != STATUS_SUCCESS)
         {
            DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                       DPFLTR_ERROR_LEVEL,
                       " !!!! PerformBasicPacketModificationAtIngressVSwitchEthernet: NdisRetreatNetBufferDataStart() [status: %#x]\n",
                       status);
         
            HLPR_BAIL;
         }
      }
*/
         if(pModificationData->flags & PCPMDF_MODIFY_MAC_HEADER)
         {
            if(pModificationData->macData.flags & PCPMDF_MODIFY_MAC_HEADER_SOURCE_ADDRESS)
            {
               FWP_VALUE value;
   
               RtlZeroMemory(&value,
                             sizeof(FWP_VALUE));
   
               HLPR_NEW(value.byteArray6,
                        FWP_BYTE_ARRAY6,
                        WFPSAMPLER_CALLOUT_DRIVER_TAG);
               HLPR_BAIL_ON_ALLOC_FAILURE(value.byteArray6,
                                          status);
               
               value.type = FWP_BYTE_ARRAY6_TYPE;
               
               RtlCopyMemory(value.byteArray6->byteArray6,
                             pModificationData->macData.pSourceMACAddress,
                             ETHERNET_ADDRESS_SIZE);
   
               status = KrnlHlprMACHeaderModifySourceAddress(&value,
                                                             pNetBufferList);
   
               KrnlHlprFwpValuePurgeLocalCopy(&value);
   
               HLPR_BAIL_ON_FAILURE(status);
            }
   
            if(pModificationData->macData.flags & PCPMDF_MODIFY_MAC_HEADER_DESTINATION_ADDRESS)
            {
               FWP_VALUE value;
   
               RtlZeroMemory(&value,
                             sizeof(FWP_VALUE));
   
#pragma warning(push)
#pragma warning(disable: 6014) /// value.byteArray6 will be freed in with call to KrnlHlprFwpValuePurgeLocalCopy

               HLPR_NEW(value.byteArray6,
                        FWP_BYTE_ARRAY6,
                        WFPSAMPLER_CALLOUT_DRIVER_TAG);
               HLPR_BAIL_ON_ALLOC_FAILURE(value.byteArray6,
                                          status);

#pragma warning(pop)

               value.type = FWP_BYTE_ARRAY6_TYPE;
               
               RtlCopyMemory(value.byteArray6->byteArray6,
                             pModificationData->macData.pDestinationMACAddress,
                             ETHERNET_ADDRESS_SIZE);
   
               status = KrnlHlprMACHeaderModifyDestinationAddress(&value,
                                                                  pNetBufferList);
   
               KrnlHlprFwpValuePurgeLocalCopy(&value);
   
               HLPR_BAIL_ON_FAILURE(status);
            }
         }
      }


   status = FwpsInjectvSwitchEthernetIngressAsync(pCompletionData->pInjectionData->injectionHandle,
                                                  pCompletionData->pInjectionData->injectionContext,
                                                  0,
                                                  0,
                                                  pVSwitchID,
                                                  sourcePortID,
                                                  sourceNICIndex,
                                                  pNetBufferList,
                                                  CompleteBasicPacketModification,
                                                  pCompletionData);

   NT_ASSERT(irql == KeGetCurrentIrql());

   if(status != STATUS_SUCCESS)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! PerformBasicPacketModificationAtIngressVSwitchEthernet: FwpsInjectvSwitchEthernetIngressAsync() [status: %#x]\n",
                 status);

   HLPR_BAIL_LABEL:

   NT_ASSERT(status == STATUS_SUCCESS);

   if(status != STATUS_SUCCESS)
   {
      if(pNetBufferList)
      {
         FwpsFreeCloneNetBufferList(pNetBufferList,
                                    0);

         pNetBufferList = 0;
      }

      if(pCompletionData)
         BasicPacketModificationCompletionDataDestroy(&pCompletionData,
                                                      TRUE);
   }

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- PerformBasicPacketModificationAtIngressVSwitchEthernet() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

/**
 @private_function="PerformBasicPacketModificationAtEgressVSwitchEthernet"
 
   Purpose:  Clones the NET_BUFFER_LIST and injects the clone back to the virtual switch's 
             ingress path from the egress VSwitch Layers using 
             FwpsInjectvSwitchEthernetIngressAsync0().                                          <br>
                                                                                                <br>
   Notes:    Applies to the following egress layers:                                            <br>
                FWPM_LAYER_EGRESS_VSWITCH_ETHERNET                                              <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/HH439669.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF546324.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
NTSTATUS PerformBasicPacketModificationAtEgressVSwitchEthernet(_In_ CLASSIFY_DATA** ppClassifyData,
                                                               _In_ INJECTION_DATA** ppInjectionData,
                                                               _In_ PC_BASIC_PACKET_MODIFICATION_DATA* pModificationData,
                                                               _In_ BOOLEAN isInline = FALSE)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> PerformBasicPacketModificationAtEgressVSwitchEthernet()\n");

#endif /// DBG

   NT_ASSERT(ppClassifyData);
   NT_ASSERT(ppInjectionData);
   NT_ASSERT(pModificationData);
   NT_ASSERT(*ppClassifyData);
   NT_ASSERT(*ppInjectionData);

   NTSTATUS                                   status          = STATUS_SUCCESS;
   FWPS_INCOMING_VALUES*                      pClassifyValues = (FWPS_INCOMING_VALUES*)(*ppClassifyData)->pClassifyValues;
   FWPS_INCOMING_METADATA_VALUES*             pMetadata       = (FWPS_INCOMING_METADATA_VALUES*)(*ppClassifyData)->pMetadataValues;
   FWP_VALUE*                                 pVSwitchIDValue = 0;
   FWP_BYTE_BLOB*                             pVSwitchID      = 0;
   NDIS_SWITCH_PORT_ID                        sourcePortID    = 0;
   NDIS_SWITCH_NIC_INDEX                      sourceNICIndex  = 0;
   NET_BUFFER_LIST*                           pNetBufferList  = 0;
   BASIC_PACKET_MODIFICATION_COMPLETION_DATA* pCompletionData = 0;

#if DBG

   KIRQL                                      irql            = KeGetCurrentIrql();

#endif /// DBG

#pragma warning(push)
#pragma warning(disable: 6014) /// pCompletionData will be freed in completionFn using BasicPacketModificationCompletionDataDestroy

   HLPR_NEW(pCompletionData,
            BASIC_PACKET_MODIFICATION_COMPLETION_DATA,
            WFPSAMPLER_CALLOUT_DRIVER_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pCompletionData,
                              status);

#pragma warning(pop)

   KeInitializeSpinLock(&(pCompletionData->spinLock));

   pCompletionData->performedInline = isInline;
   pCompletionData->pClassifyData   = *ppClassifyData;
   pCompletionData->pInjectionData  = *ppInjectionData;

   /// Responsibility for freeing this memory has been transferred to the pCompletionData
   *ppClassifyData = 0;

   *ppInjectionData = 0;

   if(FWPS_IS_L2_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_L2_METADATA_FIELD_VSWITCH_SOURCE_PORT_ID))
      sourcePortID = pMetadata->vSwitchSourcePortId;

   if(FWPS_IS_L2_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_L2_METADATA_FIELD_VSWITCH_SOURCE_NIC_INDEX))
      sourceNICIndex = (NDIS_SWITCH_NIC_INDEX)pMetadata->vSwitchSourceNicIndex;

   pVSwitchIDValue = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                               &FWPM_CONDITION_VSWITCH_ID);
   if(pVSwitchIDValue)
      pVSwitchID = pVSwitchIDValue->byteBlob;

   if(pVSwitchID == 0)
   {
      status = STATUS_INVALID_MEMBER;

       DbgPrintEx(DPFLTR_IHVNETWORK_ID,
           DPFLTR_ERROR_LEVEL,
           " !!!! PerformBasicPacketModificationAtEgressVSwitchEthernet() [status: %#x][pVSwitchID: %#p]\n",
           status,
           pVSwitchID);

      HLPR_BAIL;
   }

   /// Initial offset is at the MAC Header, so just clone the entire NET_BUFFER_LIST.
   status = FwpsAllocateCloneNetBufferList((NET_BUFFER_LIST*)pCompletionData->pClassifyData->pPacket,
                                           g_pNDISPoolData->nblPoolHandle,
                                           g_pNDISPoolData->nbPoolHandle,
                                           0,
                                           &pNetBufferList);
   if(status != STATUS_SUCCESS)
   {
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! PerformBasicPacketModificationAtEgressVSwitchEthernet: FwpsAllocateCloneNetBufferList() [status: %#x]\n",
                 status);

      HLPR_BAIL;
   }

   pCompletionData->refCount = KrnlHlprNBLGetRequiredRefCount(pNetBufferList,
                                                              TRUE);

   if(pModificationData->flags)
   {
      /// Various checks and balances must be performed to modify the IP and Transport headers at this modification point.
      /// Parsing of the headers will need to occur, as well as spot checking to verify everything is as it should be.
      /// Additionally, checksum routines will need to be written to recalculate checksums for some of the headers.
      /// The following block of code is to get you started with modifying the headers with info not readily available.
      /// (i.e. header parsing has not occurred so there is no relevant classifiable data nor metadata present).
/*
      if(pModificationData->flags & PCPMDF_MODIFY_TRANSPORT_HEADER)
      {
         UINT32  tmpStatus = STATUS_SUCCESS;
         IPPROTO protocol  = IPPROTO_MAX;

         /// The clone is at the Ethernet Header, so advance by the size of the Ethernet Header...
         NdisAdvanceNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB(pNetBufferList),
                                       ethernetHeaderSize,
                                       FALSE,
                                       0);

         protocol = KrnlHlprIPHeaderGetProtocolField(pNetBufferList,
                                                     pCompletionData->pInjectionData->addressFamily);

         /// No Transport Modification if IPsec encrypted
         if(protocol != IPPROTO_ESP &&
            protocol != IPPROTO_AH)
         {
            /// ... advance by the size of the IP Header.
            NdisAdvanceNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB(pNetBufferList),
                                          ipHeaderSize,
                                          FALSE,
                                          0);

            switch(protocol)
            {
               case IPPROTO_ICMP:
               {
                  UINT32 icmpHeaderSize = ICMP_HEADER_MIN_SIZE;
            
                  if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                                    FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE))
                     icmpHeaderSize = pMetadata->transportHeaderSize;
            
                  if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_ICMP_TYPE)
                  {
                     FWP_VALUE0 icmpType;
            
                     icmpType.type  = FWP_UINT8;
                     icmpType.uint8 = (UINT8)ntohs(pModificationData->transportData.sourcePort);
            
                     status = KrnlHlprICMPv4HeaderModifyType(&icmpType,
                                                             pNetBufferList,
                                                             icmpHeaderSize);
                     HLPR_BAIL_ON_FAILURE_2(status);
                  }
            
                  if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_ICMP_CODE)
                  {
                     FWP_VALUE0 icmpCode;
                  
                     icmpCode.type  = FWP_UINT8;
                     icmpCode.uint8 = (UINT8)ntohs(pModificationData->transportData.destinationPort);
            
                     status = KrnlHlprICMPv4HeaderModifyCode(&icmpCode,
                                                             pNetBufferList,
                                                             icmpHeaderSize);
                     HLPR_BAIL_ON_FAILURE_2(status);
                  }
            
                  break;
               }
               case IPPROTO_TCP:
               {
                  UINT32 tcpHeaderSize = TCP_HEADER_MIN_SIZE;
            
                  if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                                    FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE))
                     tcpHeaderSize = pMetadata->transportHeaderSize;
            
                  if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_SOURCE_PORT)
                  {
                     FWP_VALUE0 srcPort;
            
                     srcPort.type   = FWP_UINT16;
                     srcPort.uint16 = pModificationData->transportData.sourcePort;
            
                     status = KrnlHlprTCPHeaderModifySourcePort(&srcPort,
                                                                pNetBufferList,
                                                                tcpHeaderSize);
                     HLPR_BAIL_ON_FAILURE_2(status);
                  }
            
                  if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_DESTINATION_PORT)
                  {
                     FWP_VALUE0 dstPort;
            
                     dstPort.type   = FWP_UINT16;
                     dstPort.uint16 = pModificationData->transportData.destinationPort;
            
                     status = KrnlHlprTCPHeaderModifyDestinationPort(&dstPort,
                                                                     pNetBufferList,
                                                                     tcpHeaderSize);
                     HLPR_BAIL_ON_FAILURE_2(status);
                  }
            
                  break;
               }
               case IPPROTO_UDP:
               {
                  UINT32 udpHeaderSize = UDP_HEADER_MIN_SIZE;
            
                  if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                                    FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE))
                     udpHeaderSize = pMetadata->transportHeaderSize;
            
                  if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_SOURCE_PORT)
                  {
                     FWP_VALUE0 srcPort;
            
                     srcPort.type   = FWP_UINT16;
                     srcPort.uint16 = pModificationData->transportData.sourcePort;
            
                     status = KrnlHlprUDPHeaderModifySourcePort(&srcPort,
                                                                pNetBufferList,
                                                                udpHeaderSize);
                     HLPR_BAIL_ON_FAILURE_2(status);
                  }
            
                  if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_DESTINATION_PORT)
                  {
                     FWP_VALUE0 dstPort;
            
                     dstPort.type   = FWP_UINT16;
                     dstPort.uint16 = pModificationData->transportData.destinationPort;
            
                     status = KrnlHlprUDPHeaderModifyDestinationPort(&dstPort,
                                                                     pNetBufferList,
                                                                     udpHeaderSize);
                     HLPR_BAIL_ON_FAILURE_2(status);
                  }
            
                  break;
               }
               case IPPROTO_ICMPV6:
               {
                  UINT32 icmpHeaderSize = ICMP_HEADER_MIN_SIZE;
            
                  if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                                    FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE))
                     icmpHeaderSize = pMetadata->transportHeaderSize;
            
                  if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_ICMP_TYPE)
                  {
                     FWP_VALUE0 icmpType;
            
                     icmpType.type  = FWP_UINT8;
                     icmpType.uint8 = (UINT8)ntohs(pModificationData->transportData.sourcePort);
            
                     status = KrnlHlprICMPv6HeaderModifyType(&icmpType,
                                                             pNetBufferList,
                                                             icmpHeaderSize);
                     HLPR_BAIL_ON_FAILURE_2(status);
                  }
            
                  if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_ICMP_CODE)
                  {
                     FWP_VALUE0 icmpCode;
            
                     icmpCode.type  = FWP_UINT8;
                     icmpCode.uint8 = (UINT8)ntohs(pModificationData->transportData.destinationPort);
            
                     status = KrnlHlprICMPv6HeaderModifyCode(&icmpCode,
                                                             pNetBufferList,
                                                             icmpHeaderSize);
                     HLPR_BAIL_ON_FAILURE_2(status);
                  }
            
                  break;
               }
            }

            /// ToDo: Recalculate the Transport Checksum Here

            HLPR_BAIL_LABEL_2:

            /// return the data offset to the beginning of the IP Header
            tmpStatus = NdisRetreatNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB(pNetBufferList),
                                                      ipHeaderSize,
                                                      0,
                                                      0);
            if(tmpStatus != STATUS_SUCCESS)
            {
               DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                          DPFLTR_ERROR_LEVEL,
                          " !!!! PerformBasicPacketModificationAtEgressVSwitchEthernet: NdisRetreatNetBufferDataStart() [status: %#x]\n",
                          status);
            
               HLPR_BAIL;
            }

            HLPR_BAIL_ON_FAILURE(status);
         }

         /// return the data offset to the beginning of the Ethernet Header
         tmpStatus = NdisRetreatNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB(pNetBufferList),
                                                   ipHeaderSize,
                                                   0,
                                                   0);
         if(tmpStatus != STATUS_SUCCESS)
         {
            DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                       DPFLTR_ERROR_LEVEL,
                       " !!!! PerformBasicPacketModificationAtEgressVSwitchEthernet: NdisRetreatNetBufferDataStart() [status: %#x]\n",
                       status);

            HLPR_BAIL;
         }
      }

      if(pModificationData->flags & PCPMDF_MODIFY_IP_HEADER)
      {
         /// The clone is at the Ethernet Header, so advance by the size of the Ethernet Header...
         NdisAdvanceNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB(pNetBufferList),
                                       ethernetHeaderSize,
                                       FALSE,
                                       0);

         if(pModificationData->ipData.flags & PCPMDF_MODIFY_IP_HEADER_SOURCE_ADDRESS)
         {
            FWP_VALUE value;

            RtlZeroMemory(&value,
                          sizeof(FWP_VALUE));

            if(pCompletionData->pInjectionData->addressFamily == AF_INET)
            {
               value.type = FWP_UINT32;

               RtlCopyMemory(&(value.uint32),
                             pModificationData->ipData.sourceAddress.pIPv4,
                             IPV4_ADDRESS_SIZE);
            }
            else
            {
               HLPR_NEW(value.byteArray16,
                        FWP_BYTE_ARRAY16,
                        WFPSAMPLER_CALLOUT_DRIVER_TAG);
               HLPR_BAIL_ON_ALLOC_FAILURE_WITH_LABEL(value.byteArray16,
                                                     status,
                                                     HLPR_BAIL_LABEL_3);

               value.type = FWP_BYTE_ARRAY16_TYPE;

               RtlCopyMemory(value.byteArray16->byteArray16,
                             pModificationData->ipData.sourceAddress.pIPv6,
                             IPV6_ADDRESS_SIZE);
            }

            status = KrnlHlprIPHeaderModifySourceAddress(&value,
                                                         pNetBufferList,
                                                         TRUE);

            KrnlHlprFwpValuePurgeLocalCopy(&value);

            HLPR_BAIL_ON_FAILURE_WITH_LABEL(status,
                                            HLPR_BAIL_LABEL_3);
         }

         if(pModificationData->ipData.flags & PCPMDF_MODIFY_IP_HEADER_DESTINATION_ADDRESS)
         {
            FWP_VALUE value;

            RtlZeroMemory(&value,
                          sizeof(FWP_VALUE));

            if(pCompletionData->pInjectionData->addressFamily == AF_INET)
            {
               value.type = FWP_UINT32;

               RtlCopyMemory(&(value.uint32),
                             pModificationData->ipData.destinationAddress.pIPv4,
                             IPV4_ADDRESS_SIZE);
            }
            else
            {
               HLPR_NEW(value.byteArray16,
                        FWP_BYTE_ARRAY16,
                        WFPSAMPLER_CALLOUT_DRIVER_TAG);
               HLPR_BAIL_ON_ALLOC_FAILURE_WITH_LABEL(value.byteArray16,
                                                     status,
                                                     HLPR_BAIL_LABEL_3);

               value.type = FWP_BYTE_ARRAY16_TYPE;

               RtlCopyMemory(value.byteArray16->byteArray16,
                             pModificationData->ipData.destinationAddress.pIPv6,
                             IPV6_ADDRESS_SIZE);
            }

            status = KrnlHlprIPHeaderModifyDestinationAddress(&value,
                                                              pNetBufferList,
                                                              TRUE);

            KrnlHlprFwpValuePurgeLocalCopy(&value);

            HLPR_BAIL_ON_FAILURE_WITH_LABEL(status,
                                            HLPR_BAIL_LABEL_3);
         }

         HLPR_BAIL_LABEL_3:

         /// return the data offset to the beginning of the Ethernet Header
         tmpStatus = NdisRetreatNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB(pNetBufferList),
                                                   ipHeaderSize,
                                                   0,
                                                   0);
         if(tmpStatus != STATUS_SUCCESS)
         {
            DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                       DPFLTR_ERROR_LEVEL,
                       " !!!! PerformBasicPacketModificationAtEgressVSwitchEthernet: NdisRetreatNetBufferDataStart() [status: %#x]\n",
                       status);
         
            HLPR_BAIL;
         }
      }
*/
      if(pModificationData->flags & PCPMDF_MODIFY_MAC_HEADER)
      {
         if(pModificationData->macData.flags & PCPMDF_MODIFY_MAC_HEADER_SOURCE_ADDRESS)
         {
            FWP_VALUE value;

            RtlZeroMemory(&value,
                          sizeof(FWP_VALUE));

            HLPR_NEW(value.byteArray6,
                     FWP_BYTE_ARRAY6,
                     WFPSAMPLER_CALLOUT_DRIVER_TAG);
            HLPR_BAIL_ON_ALLOC_FAILURE(value.byteArray6,
                                       status);
            
            value.type = FWP_BYTE_ARRAY6_TYPE;
            
            RtlCopyMemory(value.byteArray6->byteArray6,
                          pModificationData->macData.pSourceMACAddress,
                          ETHERNET_ADDRESS_SIZE);

            status = KrnlHlprMACHeaderModifySourceAddress(&value,
                                                          pNetBufferList);

            KrnlHlprFwpValuePurgeLocalCopy(&value);

            HLPR_BAIL_ON_FAILURE(status);
         }

         if(pModificationData->macData.flags & PCPMDF_MODIFY_MAC_HEADER_DESTINATION_ADDRESS)
         {
            FWP_VALUE value;

            RtlZeroMemory(&value,
                          sizeof(FWP_VALUE));

#pragma warning(push)
#pragma warning(disable: 6014) /// value.byteArray6 will be freed in with call to KrnlHlprFwpValuePurgeLocalCopy

            HLPR_NEW(value.byteArray6,
                     FWP_BYTE_ARRAY6,
                     WFPSAMPLER_CALLOUT_DRIVER_TAG);
            HLPR_BAIL_ON_ALLOC_FAILURE(value.byteArray6,
                                       status);

#pragma warning(pop)
            
            value.type = FWP_BYTE_ARRAY6_TYPE;
            
            RtlCopyMemory(value.byteArray6->byteArray6,
                          pModificationData->macData.pDestinationMACAddress,
                          ETHERNET_ADDRESS_SIZE);

            status = KrnlHlprMACHeaderModifyDestinationAddress(&value,
                                                               pNetBufferList);

            KrnlHlprFwpValuePurgeLocalCopy(&value);

            HLPR_BAIL_ON_FAILURE(status);
         }
      }
   }

   status = FwpsInjectvSwitchEthernetIngressAsync(pCompletionData->pInjectionData->injectionHandle,
                                                  pCompletionData->pInjectionData->injectionContext,
                                                  0,
                                                  0,
                                                  pVSwitchID,
                                                  sourcePortID,
                                                  sourceNICIndex,
                                                  pNetBufferList,
                                                  CompleteBasicPacketModification,
                                                  pCompletionData);

   NT_ASSERT(irql == KeGetCurrentIrql());

   if(status != STATUS_SUCCESS)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! PerformBasicPacketModificationAtEgressVSwitchEthernet: FwpsInjectvSwitchEthernetIngressAsync() [status: %#x]\n",
                 status);

   HLPR_BAIL_LABEL:

   NT_ASSERT(status == STATUS_SUCCESS);

   if(status != STATUS_SUCCESS)
   {
      if(pNetBufferList)
      {
         FwpsFreeCloneNetBufferList(pNetBufferList,
                                    0);

         pNetBufferList = 0;
      }

      if(pCompletionData)
         BasicPacketModificationCompletionDataDestroy(&pCompletionData,
                                                      TRUE);
   }

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- PerformBasicPacketModificationAtEgressVSwitchEthernet() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}


#endif // (NTDDI_VERSION >= NTDDI_WIN8)

/**
 @private_function="PerformBasicPacketModificationAtInboundNetwork"
 
   Purpose:  Clones the NET_BUFFER_LIST and injects the clone back to the stack from the 
             incoming Network Layers using FwpsInjectNetworkReceiveAsync().                     <br>
                                                                                                <br>
   Notes:    Applies to the following inbound layers:                                           <br>
                FWPM_LAYER_INBOUND_IPPACKET_V{4/6}                                              <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF551183.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF546324.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
NTSTATUS PerformBasicPacketModificationAtInboundNetwork(_In_ CLASSIFY_DATA** ppClassifyData,
                                                        _In_ INJECTION_DATA** ppInjectionData,
                                                        _In_ PC_BASIC_PACKET_MODIFICATION_DATA* pModificationData,
                                                        _In_ BOOLEAN isInline = FALSE)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> PerformBasicPacketModificationAtInboundNetwork()\n");

#endif /// DBG

   NT_ASSERT(ppClassifyData);
   NT_ASSERT(ppInjectionData);
   NT_ASSERT(pModificationData);
   NT_ASSERT(*ppClassifyData);
   NT_ASSERT(*ppInjectionData);

   NTSTATUS                                   status              = STATUS_SUCCESS;
   FWPS_INCOMING_VALUES*                      pClassifyValues     = (FWPS_INCOMING_VALUES*)(*ppClassifyData)->pClassifyValues;
   FWPS_INCOMING_METADATA_VALUES*             pMetadata           = (FWPS_INCOMING_METADATA_VALUES*)(*ppClassifyData)->pMetadataValues;
   COMPARTMENT_ID                             compartmentID       = DEFAULT_COMPARTMENT_ID;
   IF_INDEX                                   interfaceIndex      = 0;
   IF_INDEX                                   subInterfaceIndex   = 0;
   UINT32                                     flags               = 0;
   NET_BUFFER_LIST*                           pNetBufferList      = 0;
   BASIC_PACKET_MODIFICATION_COMPLETION_DATA* pCompletionData     = 0;
   UINT32                                     ipHeaderSize        = 0;
   UINT32                                     bytesRetreated      = 0;
   UINT64                                     endpointHandle      = 0;
   IPPROTO                                    protocol            = IPPROTO_MAX;
   FWP_VALUE*                                 pInterfaceIndex     = 0;
   FWP_VALUE*                                 pSubInterfaceIndex  = 0;
   FWP_VALUE*                                 pFlags              = 0;
   BYTE*                                      pSourceAddress      = 0;
   BYTE*                                      pDestinationAddress = 0;
   NDIS_TCP_IP_CHECKSUM_PACKET_INFO           checksumInfo        = {0};

#if DBG

   KIRQL                                      irql               = KeGetCurrentIrql();

#endif /// DBG

#pragma warning(push)
#pragma warning(disable: 6014) /// pCompletionData will be freed in completionFn using BasicPacketModificationCompletionDataDestroy

   HLPR_NEW(pCompletionData,
            BASIC_PACKET_MODIFICATION_COMPLETION_DATA,
            WFPSAMPLER_CALLOUT_DRIVER_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pCompletionData,
                              status);

#pragma warning(pop)

   KeInitializeSpinLock(&(pCompletionData->spinLock));

   pCompletionData->performedInline = isInline;
   pCompletionData->pClassifyData   = *ppClassifyData;
   pCompletionData->pInjectionData  = *ppInjectionData;

   /// Responsibility for freeing this memory has been transferred to the pCompletionData
   *ppClassifyData = 0;

   *ppInjectionData = 0;

   if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                     FWPS_METADATA_FIELD_COMPARTMENT_ID))
      compartmentID = (COMPARTMENT_ID)pMetadata->compartmentId;

   if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                     FWPS_METADATA_FIELD_TRANSPORT_ENDPOINT_HANDLE))
      endpointHandle = pMetadata->transportEndpointHandle;

   pInterfaceIndex = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                               &FWPM_CONDITION_INTERFACE_INDEX);
   if(pInterfaceIndex &&
      pInterfaceIndex->type == FWP_UINT32)
      interfaceIndex = (IF_INDEX)pInterfaceIndex->uint32;

   pSubInterfaceIndex = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                  &FWPM_CONDITION_SUB_INTERFACE_INDEX);
   if(pSubInterfaceIndex &&
      pSubInterfaceIndex->type == FWP_UINT32)
      subInterfaceIndex = (IF_INDEX)pSubInterfaceIndex->uint32;

   pFlags = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                      &FWPM_CONDITION_FLAGS);
   if(pFlags &&
      pFlags->type == FWP_UINT32)
      flags = pFlags->uint32;

   if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                     FWPS_METADATA_FIELD_IP_HEADER_SIZE))
      bytesRetreated = ipHeaderSize = pMetadata->ipHeaderSize;

   checksumInfo.Value = (ULONG)(ULONG_PTR)NET_BUFFER_LIST_INFO((NET_BUFFER_LIST*)pCompletionData->pClassifyData->pPacket,
                                                               TcpIpChecksumNetBufferListInfo);

   /// Initial offset is at the Transport Header, so retreat the size of the IP Header ...
   status = NdisRetreatNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)pCompletionData->pClassifyData->pPacket),
                                          bytesRetreated,
                                          0,
                                          0);
   if(status != STATUS_SUCCESS)
   {
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! PerformBasicPacketModificationAtInboundNetwork: NdisRetreatNetBufferDataStart() [status: %#x]\n",
                 status);

      HLPR_BAIL;
   }

   /// ... clone the entire NET_BUFFER_LIST ...
   status = FwpsAllocateCloneNetBufferList((NET_BUFFER_LIST*)pCompletionData->pClassifyData->pPacket,
                                           g_pNDISPoolData->nblPoolHandle,
                                           g_pNDISPoolData->nbPoolHandle,
                                           0,
                                           &pNetBufferList);

   /// ... and advance the offset back to the original position.
   NdisAdvanceNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)pCompletionData->pClassifyData->pPacket),
                                 bytesRetreated,
                                 FALSE,
                                 0);

   if(status != STATUS_SUCCESS ||
      !pNetBufferList)
   {
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! PerformBasicPacketModificationAtInboundNetwork: FwpsAllocateCloneNetBufferList() [status: %#x]\n",
                 status);

      HLPR_BAIL;
   }

   /// Handle if this packet had the IP checksum offloaded or if it's loopback
   if(checksumInfo.Receive.NdisPacketIpChecksumSucceeded ||
      flags & FWP_CONDITION_FLAG_IS_LOOPBACK)
   {
      /// Prevent TCP/IP Zone crossing and recalculate the checksums
      if(flags & FWP_CONDITION_FLAG_IS_LOOPBACK)
      {
         FWP_VALUE* pLocalAddress    = 0;
         FWP_VALUE* pRemoteAddress   = 0;
         FWP_VALUE* pLoopbackAddress = 0;

         pLocalAddress = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                    &FWPM_CONDITION_IP_REMOTE_ADDRESS);
         if(pLocalAddress &&
            ((pLocalAddress->type == FWP_UINT32 &&
            RtlCompareMemory(&(pLocalAddress->uint32),
                             IPV4_LOOPBACK_ADDRESS,
                             IPV4_ADDRESS_SIZE)) ||
            (pLocalAddress->type == FWP_BYTE_ARRAY16_TYPE &&
            RtlCompareMemory(pLocalAddress->byteArray16->byteArray16,
                             IPV6_LOOPBACK_ADDRESS,
                             IPV6_ADDRESS_SIZE))))
            pLoopbackAddress = pLocalAddress;

         if(!pLoopbackAddress)
         {
            pRemoteAddress = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                       &FWPM_CONDITION_IP_REMOTE_ADDRESS);
            if(pRemoteAddress &&
               ((pRemoteAddress->type == FWP_UINT32 &&
               RtlCompareMemory(&(pRemoteAddress->uint32),
                                IPV4_LOOPBACK_ADDRESS,
                                IPV4_ADDRESS_SIZE)) ||
               (pRemoteAddress->type == FWP_BYTE_ARRAY16_TYPE &&
               RtlCompareMemory(pRemoteAddress->byteArray16->byteArray16,
                                IPV6_LOOPBACK_ADDRESS,
                                IPV6_ADDRESS_SIZE))))
               pLoopbackAddress = pRemoteAddress;
         }

         if(pLoopbackAddress)
         {
            status = KrnlHlprIPHeaderModifyLoopbackToLocal(pMetadata,
                                                           pLoopbackAddress,
                                                           ipHeaderSize,
                                                           pNetBufferList,
                                                           (const WSACMSGHDR*)pCompletionData->pInjectionData->pControlData,
                                                           pCompletionData->pInjectionData->controlDataLength);
            if(status != STATUS_SUCCESS)
            {
               DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                          DPFLTR_ERROR_LEVEL,
                          " !!!! PerformBasicPacketModificationAtInboundNetwork: KrnlHlprIPHeaderModifyLoopbackToLocal() [status: %#x]\n",
                          status);

               HLPR_BAIL;
            }
         }
      }
      else
      {
         /// Recalculate the checksum
         if(pCompletionData->pInjectionData->addressFamily == AF_INET)
            KrnlHlprIPHeaderCalculateV4Checksum(pNetBufferList,
                                                ipHeaderSize);
      }
   }

   pCompletionData->refCount = KrnlHlprNBLGetRequiredRefCount(pNetBufferList);

   protocol = KrnlHlprIPHeaderGetProtocolField(pNetBufferList,
                                               pCompletionData->pInjectionData->addressFamily);

   if(pModificationData->flags)
   {
      if(pModificationData->flags & PCPMDF_MODIFY_TRANSPORT_HEADER)
      {
         NTSTATUS tmpStatus = STATUS_SUCCESS;

         /// The clone is at the IP Header, so advance by the size of the IP Header.
         NdisAdvanceNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB(pNetBufferList),
                                       ipHeaderSize,
                                       FALSE,
                                       0);

         switch(protocol)
         {
            case IPPROTO_ICMP:
            {
               UINT32 icmpHeaderSize = ICMP_HEADER_MIN_SIZE;
         
               if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                                 FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE))
                  icmpHeaderSize = pMetadata->transportHeaderSize;
         
               if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_ICMP_TYPE)
               {
                  FWP_VALUE0 icmpType;
         
                  icmpType.type  = FWP_UINT8;
                  icmpType.uint8 = (UINT8)ntohs(pModificationData->transportData.sourcePort);
         
                  status = KrnlHlprICMPv4HeaderModifyType(&icmpType,
                                                          pNetBufferList,
                                                          icmpHeaderSize);
                  HLPR_BAIL_ON_FAILURE_2(status);
               }
         
               if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_ICMP_CODE)
               {
                  FWP_VALUE0 icmpCode;
               
                  icmpCode.type  = FWP_UINT8;
                  icmpCode.uint8 = (UINT8)ntohs(pModificationData->transportData.destinationPort);
         
                  status = KrnlHlprICMPv4HeaderModifyCode(&icmpCode,
                                                          pNetBufferList,
                                                          icmpHeaderSize);
                  HLPR_BAIL_ON_FAILURE_2(status);
               }
         
               break;
            }
            case IPPROTO_TCP:
            {
               UINT32 tcpHeaderSize = TCP_HEADER_MIN_SIZE;
         
               if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                                 FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE))
                  tcpHeaderSize = pMetadata->transportHeaderSize;
         
               if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_SOURCE_PORT)
               {
                  FWP_VALUE0 srcPort;
         
                  srcPort.type   = FWP_UINT16;
                  srcPort.uint16 = pModificationData->transportData.sourcePort;
         
                  status = KrnlHlprTCPHeaderModifySourcePort(&srcPort,
                                                             pNetBufferList,
                                                             tcpHeaderSize);
                  HLPR_BAIL_ON_FAILURE_2(status);
               }
         
               if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_DESTINATION_PORT)
               {
                  FWP_VALUE0 dstPort;
         
                  dstPort.type   = FWP_UINT16;
                  dstPort.uint16 = pModificationData->transportData.destinationPort;
         
                  status = KrnlHlprTCPHeaderModifyDestinationPort(&dstPort,
                                                                  pNetBufferList,
                                                                  tcpHeaderSize);
                  HLPR_BAIL_ON_FAILURE_2(status);
               }
         
               break;
            }
            case IPPROTO_UDP:
            {
               UINT32 udpHeaderSize = UDP_HEADER_MIN_SIZE;
         
               if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                                 FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE))
                  udpHeaderSize = pMetadata->transportHeaderSize;
         
               if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_SOURCE_PORT)
               {
                  FWP_VALUE0 srcPort;
         
                  srcPort.type   = FWP_UINT16;
                  srcPort.uint16 = pModificationData->transportData.sourcePort;
         
                  status = KrnlHlprUDPHeaderModifySourcePort(&srcPort,
                                                             pNetBufferList,
                                                             udpHeaderSize);
                  HLPR_BAIL_ON_FAILURE_2(status);
               }
         
               if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_DESTINATION_PORT)
               {
                  FWP_VALUE0 dstPort;
         
                  dstPort.type   = FWP_UINT16;
                  dstPort.uint16 = pModificationData->transportData.destinationPort;
         
                  status = KrnlHlprUDPHeaderModifyDestinationPort(&dstPort,
                                                                  pNetBufferList,
                                                                  udpHeaderSize);
                  HLPR_BAIL_ON_FAILURE_2(status);
               }
         
               break;
            }
            case IPPROTO_ICMPV6:
            {
               UINT32 icmpHeaderSize = ICMP_HEADER_MIN_SIZE;
         
               if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                                 FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE))
                  icmpHeaderSize = pMetadata->transportHeaderSize;
         
               if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_ICMP_TYPE)
               {
                  FWP_VALUE0 icmpType;
         
                  icmpType.type  = FWP_UINT8;
                  icmpType.uint8 = (UINT8)ntohs(pModificationData->transportData.sourcePort);
         
                  status = KrnlHlprICMPv6HeaderModifyType(&icmpType,
                                                          pNetBufferList,
                                                          icmpHeaderSize);
                  HLPR_BAIL_ON_FAILURE_2(status);
               }
         
               if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_ICMP_CODE)
               {
                  FWP_VALUE0 icmpCode;
         
                  icmpCode.type  = FWP_UINT8;
                  icmpCode.uint8 = (UINT8)ntohs(pModificationData->transportData.destinationPort);
         
                  status = KrnlHlprICMPv6HeaderModifyCode(&icmpCode,
                                                          pNetBufferList,
                                                          icmpHeaderSize);
                  HLPR_BAIL_ON_FAILURE_2(status);
               }
         
               break;
            }
         }

         HLPR_BAIL_LABEL_2:

         /// return the data offset to the beginning of the IP Header
         tmpStatus = NdisRetreatNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB(pNetBufferList),
                                                   ipHeaderSize,
                                                   0,
                                                   0);
         if(tmpStatus != STATUS_SUCCESS)
         {
            DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                       DPFLTR_ERROR_LEVEL,
                       " !!!! PerformBasicPacketModificationAtInboundNetwork: NdisRetreatNetBufferDataStart() [status: %#x]\n",
                       status);
         
            HLPR_BAIL;
         }

         HLPR_BAIL_ON_FAILURE(status);
      }

      if(pModificationData->flags & PCPMDF_MODIFY_IP_HEADER)
      {
         if(pModificationData->ipData.flags & PCPMDF_MODIFY_IP_HEADER_INTERFACE_INDEX)
            interfaceIndex = pModificationData->ipData.interfaceIndex;

         if(pModificationData->ipData.flags & PCPMDF_MODIFY_IP_HEADER_SOURCE_ADDRESS)
         {
            FWP_VALUE value;

            RtlZeroMemory(&value,
                          sizeof(FWP_VALUE));

            if(pCompletionData->pInjectionData->addressFamily == AF_INET)
            {
               value.type = FWP_UINT32;

               RtlCopyMemory(&(value.uint32),
                             pModificationData->ipData.sourceAddress.pIPv4,
                             IPV4_ADDRESS_SIZE);
            }
            else
            {
               HLPR_NEW(value.byteArray16,
                        FWP_BYTE_ARRAY16,
                        WFPSAMPLER_CALLOUT_DRIVER_TAG);
               HLPR_BAIL_ON_ALLOC_FAILURE(value.byteArray16,
                                          status);

               value.type = FWP_BYTE_ARRAY16_TYPE;

               RtlCopyMemory(value.byteArray16->byteArray16,
                             pModificationData->ipData.sourceAddress.pIPv6,
                             IPV6_ADDRESS_SIZE);
            }

            status = KrnlHlprIPHeaderModifySourceAddress(&value,
                                                         pNetBufferList,
                                                         TRUE);

            KrnlHlprFwpValuePurgeLocalCopy(&value);

            HLPR_BAIL_ON_FAILURE(status);
         }

         if(pModificationData->ipData.flags & PCPMDF_MODIFY_IP_HEADER_DESTINATION_ADDRESS)
         {
            FWP_VALUE value;

            RtlZeroMemory(&value,
                          sizeof(FWP_VALUE));

            if(pCompletionData->pInjectionData->addressFamily == AF_INET)
            {
               value.type = FWP_UINT32;

               RtlCopyMemory(&(value.uint32),
                             pModificationData->ipData.destinationAddress.pIPv4,
                             IPV4_ADDRESS_SIZE);
            }
            else
            {
#pragma warning(push)
#pragma warning(disable: 6014) /// value.byteArray16 will be freed in with call to KrnlHlprFwpValuePurgeLocalCopy

               HLPR_NEW(value.byteArray16,
                        FWP_BYTE_ARRAY16,
                        WFPSAMPLER_CALLOUT_DRIVER_TAG);
               HLPR_BAIL_ON_ALLOC_FAILURE(value.byteArray16,
                                          status);

#pragma warning(pop)

               value.type = FWP_BYTE_ARRAY16_TYPE;

               RtlCopyMemory(value.byteArray16->byteArray16,
                             pModificationData->ipData.destinationAddress.pIPv6,
                             IPV6_ADDRESS_SIZE);
            }

            status = KrnlHlprIPHeaderModifyDestinationAddress(&value,
                                                              pNetBufferList,
                                                              TRUE);

            KrnlHlprFwpValuePurgeLocalCopy(&value);

            HLPR_BAIL_ON_FAILURE(status);
         }
      }
   }

   /// Handle if this packet is destined for the software loopback
   if(flags & FWP_CONDITION_FLAG_IS_LOOPBACK)
   {
      FWP_VALUE* pLocalAddress    = 0;
      FWP_VALUE* pRemoteAddress   = 0;
      FWP_VALUE* pLoopbackAddress = 0;

      pLocalAddress = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                 &FWPM_CONDITION_IP_REMOTE_ADDRESS);
      if(pLocalAddress &&
         ((pLocalAddress->type == FWP_UINT32 &&
         RtlCompareMemory(&(pLocalAddress->uint32),
                          IPV4_LOOPBACK_ADDRESS,
                          IPV4_ADDRESS_SIZE)) ||
         (pLocalAddress->type == FWP_BYTE_ARRAY16_TYPE &&
         RtlCompareMemory(pLocalAddress->byteArray16->byteArray16,
                          IPV6_LOOPBACK_ADDRESS,
                          IPV6_ADDRESS_SIZE))))
         pLoopbackAddress = pLocalAddress;

      if(!pLoopbackAddress)
      {
         pRemoteAddress = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                    &FWPM_CONDITION_IP_REMOTE_ADDRESS);
         if(pRemoteAddress &&
            ((pRemoteAddress->type == FWP_UINT32 &&
            RtlCompareMemory(&(pRemoteAddress->uint32),
                             IPV4_LOOPBACK_ADDRESS,
                             IPV4_ADDRESS_SIZE)) ||
            (pRemoteAddress->type == FWP_BYTE_ARRAY16_TYPE &&
            RtlCompareMemory(pRemoteAddress->byteArray16->byteArray16,
                             IPV6_LOOPBACK_ADDRESS,
                             IPV6_ADDRESS_SIZE))))
            pLoopbackAddress = pRemoteAddress;
      }

      if(pLoopbackAddress)
      {
         status = KrnlHlprIPHeaderModifyLoopbackToLocal(pMetadata,
                                                        pLoopbackAddress,
                                                        ipHeaderSize,
                                                        pNetBufferList,
                                                        (const WSACMSGHDR*)pCompletionData->pInjectionData->pControlData,
                                                        pCompletionData->pInjectionData->controlDataLength);
         if(status != STATUS_SUCCESS)
         {
            DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                       DPFLTR_ERROR_LEVEL,
                       " !!!! PerformBasicPacketModificationAtInboundNetwork: KrnlHlprIPHeaderModifyLoopbackToLocal() [status: %#x]\n",
                       status);

            HLPR_BAIL;
         }
      }
   }

   pSourceAddress = KrnlHlprIPHeaderGetSourceAddressField(pNetBufferList,
                                                          pCompletionData->pInjectionData->addressFamily);

   pDestinationAddress = KrnlHlprIPHeaderGetDestinationAddressField(pNetBufferList,
                                                                    pCompletionData->pInjectionData->addressFamily);

   status = FwpsConstructIpHeaderForTransportPacket(pNetBufferList,
                                                    ipHeaderSize,
                                                    pCompletionData->pInjectionData->addressFamily,
                                                    (UCHAR*)pSourceAddress,
                                                    (UCHAR*)pDestinationAddress,
                                                    protocol,
                                                    endpointHandle,
                                                    (const WSACMSGHDR*)pCompletionData->pInjectionData->pControlData,
                                                    pCompletionData->pInjectionData->controlDataLength,
                                                    0,
                                                    0,
                                                    interfaceIndex,
                                                    subInterfaceIndex);
   if(status != STATUS_SUCCESS)
   {
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! PerformBasicPacketModificationAtInboundNetwork: NdisRetreatNetBufferDataStart() [status: %#x]\n",
                 status);
   
      HLPR_BAIL;
   }

   status = FwpsInjectNetworkReceiveAsync(pCompletionData->pInjectionData->injectionHandle,
                                          pCompletionData->pInjectionData->injectionContext,
                                          0,
                                          compartmentID,
                                          interfaceIndex,
                                          subInterfaceIndex,
                                          pNetBufferList,
                                          CompleteBasicPacketModification,
                                          pCompletionData);

   NT_ASSERT(irql == KeGetCurrentIrql());

   if(status != STATUS_SUCCESS)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! PerformBasicPacketModificationAtInboundNetwork: FwpsInjectNetworkReceiveAsync() [status: %#x]\n",
                 status);

   HLPR_BAIL_LABEL:

   NT_ASSERT(status == STATUS_SUCCESS);

   if(status != STATUS_SUCCESS)
   {
      if(pNetBufferList)
      {
         FwpsFreeCloneNetBufferList(pNetBufferList,
                                    0);

         pNetBufferList = 0;
      }

      if(pCompletionData)
         BasicPacketModificationCompletionDataDestroy(&pCompletionData,
                                                      TRUE);
   }

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- PerformBasicPacketModificationAtInboundNetwork() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

/**
 @private_function="PerformBasicPacketModificationAtOutboundNetwork"
 
   Purpose:  Clones the NET_BUFFER_LIST and injects the clone back to the stack from the 
             outgoing Network Layers using FwpsInjectNetworkSendAsync().                        <br>
                                                                                                <br>
   Notes:    Applies to the following inbound layers:                                           <br>
                FWPM_LAYER_OUTBOUND_IPPACKET_V{4/6}                                             <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF551185.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF546324.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
NTSTATUS PerformBasicPacketModificationAtOutboundNetwork(_In_ CLASSIFY_DATA** ppClassifyData,
                                                         _In_ INJECTION_DATA** ppInjectionData,
                                                         _In_ PC_BASIC_PACKET_MODIFICATION_DATA* pModificationData,
                                                         _In_ BOOLEAN isInline = FALSE)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> PerformBasicPacketModificationAtOutboundNetwork()\n");

#endif /// DBG

   NT_ASSERT(ppClassifyData);
   NT_ASSERT(ppInjectionData);
   NT_ASSERT(pModificationData);
   NT_ASSERT(*ppClassifyData);
   NT_ASSERT(*ppInjectionData);

   NTSTATUS                                   status          = STATUS_SUCCESS;
   FWPS_INCOMING_METADATA_VALUES*             pMetadata       = (FWPS_INCOMING_METADATA_VALUES*)(*ppClassifyData)->pMetadataValues;
   COMPARTMENT_ID                             compartmentID   = DEFAULT_COMPARTMENT_ID;
   NET_BUFFER_LIST*                           pNetBufferList  = 0;
   BASIC_PACKET_MODIFICATION_COMPLETION_DATA* pCompletionData = 0;

#if DBG

   KIRQL                                      irql            = KeGetCurrentIrql();

#endif /// DBG

#pragma warning(push)
#pragma warning(disable: 6014) /// pCompletionData will be freed in completionFn using BasicPacketModificationCompletionDataDestroy

   HLPR_NEW(pCompletionData,
            BASIC_PACKET_MODIFICATION_COMPLETION_DATA,
            WFPSAMPLER_CALLOUT_DRIVER_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pCompletionData,
                              status);

#pragma warning(pop)

   KeInitializeSpinLock(&(pCompletionData->spinLock));

   pCompletionData->performedInline = isInline;
   pCompletionData->pClassifyData   = *ppClassifyData;
   pCompletionData->pInjectionData  = *ppInjectionData;

   /// Responsibility for freeing this memory has been transferred to the pCompletionData
   *ppClassifyData = 0;

   *ppInjectionData = 0;

   if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                     FWPS_METADATA_FIELD_COMPARTMENT_ID))
      compartmentID = (COMPARTMENT_ID)pMetadata->compartmentId;

   /// Initial offset is at the IP Header, so just clone the entire NET_BUFFER_LIST.
   status = FwpsAllocateCloneNetBufferList((NET_BUFFER_LIST*)pCompletionData->pClassifyData->pPacket,
                                           g_pNDISPoolData->nblPoolHandle,
                                           g_pNDISPoolData->nbPoolHandle,
                                           0,
                                           &pNetBufferList);
   if(status != STATUS_SUCCESS)
   {
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! PerformBasicPacketModificationAtOutboundNetwork: FwpsAllocateCloneNetBufferList() [status: %#x]\n",
                 status);

      HLPR_BAIL;
   }

   pCompletionData->refCount = KrnlHlprNBLGetRequiredRefCount(pNetBufferList);

   if(pModificationData->flags)
   {
      /// Various checks and balances must be performed to modify the Transport header at this modification point.
      /// Parsing of the headers will need to occur, as well as spot checking to verify everything is as it should be.
      /// Additionally, checksum routines will need to be written to recalculate checksums for some of the headers.
      /// The following block of code is to get you started with modifying the headers with info not readily available.
      /// (i.e. header parsing has not occurred so there is no relevant classifiable data nor metadata present).
/*
      if(pModificationData->flags & PCPMDF_MODIFY_TRANSPORT_HEADER)
      {
         UINT32  tmpStatus = STATUS_SUCCESS;
         IPPROTO protocol  = IPPROTO_MAX;

         protocol = KrnlHlprIPHeaderGetProtocolField(pNetBufferList,
                                                     pCompletionData->pInjectionData->addressFamily);

         /// The clone is at the IP Header, so advance by the size of the IP Header.
         NdisAdvanceNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB(pNetBufferList),
                                       ipHeaderSize,
                                       FALSE,
                                       0);

         switch(protocol)
         {
            case IPPROTO_ICMP:
            {
               UINT32 icmpHeaderSize = ICMP_HEADER_MIN_SIZE;
         
               if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                                 FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE))
                  icmpHeaderSize = pMetadata->transportHeaderSize;
         
               if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_ICMP_TYPE)
               {
                  FWP_VALUE0 icmpType;
         
                  icmpType.type  = FWP_UINT8;
                  icmpType.uint8 = (UINT8)ntohs(pModificationData->transportData.sourcePort);
         
                  status = KrnlHlprICMPv4HeaderModifyType(&icmpType,
                                                          pNetBufferList,
                                                          icmpHeaderSize);
                  HLPR_BAIL_ON_FAILURE_2(status);
               }
         
               if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_ICMP_CODE)
               {
                  FWP_VALUE0 icmpCode;
               
                  icmpCode.type  = FWP_UINT8;
                  icmpCode.uint8 = (UINT8)ntohs(pModificationData->transportData.destinationPort);
         
                  status = KrnlHlprICMPv4HeaderModifyCode(&icmpCode,
                                                          pNetBufferList,
                                                          icmpHeaderSize);
                  HLPR_BAIL_ON_FAILURE_2(status);
               }
         
               break;
            }
            case IPPROTO_TCP:
            {
               UINT32 tcpHeaderSize = TCP_HEADER_MIN_SIZE;
         
               if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                                 FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE))
                  tcpHeaderSize = pMetadata->transportHeaderSize;
         
               if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_SOURCE_PORT)
               {
                  FWP_VALUE0 srcPort;
         
                  srcPort.type   = FWP_UINT16;
                  srcPort.uint16 = pModificationData->transportData.sourcePort;
         
                  status = KrnlHlprTCPHeaderModifySourcePort(&srcPort,
                                                             pNetBufferList,
                                                             tcpHeaderSize);
                  HLPR_BAIL_ON_FAILURE_2(status);
               }
         
               if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_DESTINATION_PORT)
               {
                  FWP_VALUE0 dstPort;
         
                  dstPort.type   = FWP_UINT16;
                  dstPort.uint16 = pModificationData->transportData.destinationPort;
         
                  status = KrnlHlprTCPHeaderModifyDestinationPort(&dstPort,
                                                                  pNetBufferList,
                                                                  tcpHeaderSize);
                  HLPR_BAIL_ON_FAILURE_2(status);
               }
         
               break;
            }
            case IPPROTO_UDP:
            {
               UINT32 udpHeaderSize = UDP_HEADER_MIN_SIZE;
         
               if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                                 FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE))
                  udpHeaderSize = pMetadata->transportHeaderSize;
         
               if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_SOURCE_PORT)
               {
                  FWP_VALUE0 srcPort;
         
                  srcPort.type   = FWP_UINT16;
                  srcPort.uint16 = pModificationData->transportData.sourcePort;
         
                  status = KrnlHlprUDPHeaderModifySourcePort(&srcPort,
                                                             pNetBufferList,
                                                             udpHeaderSize);
                  HLPR_BAIL_ON_FAILURE_2(status);
               }
         
               if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_DESTINATION_PORT)
               {
                  FWP_VALUE0 dstPort;
         
                  dstPort.type   = FWP_UINT16;
                  dstPort.uint16 = pModificationData->transportData.destinationPort;
         
                  status = KrnlHlprUDPHeaderModifyDestinationPort(&dstPort,
                                                                  pNetBufferList,
                                                                  udpHeaderSize);
                  HLPR_BAIL_ON_FAILURE_2(status);
               }
         
               break;
            }
            case IPPROTO_ICMPV6:
            {
               UINT32 icmpHeaderSize = ICMP_HEADER_MIN_SIZE;
         
               if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                                 FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE))
                  icmpHeaderSize = pMetadata->transportHeaderSize;
         
               if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_ICMP_TYPE)
               {
                  FWP_VALUE0 icmpType;
         
                  icmpType.type  = FWP_UINT8;
                  icmpType.uint8 = (UINT8)ntohs(pModificationData->transportData.sourcePort);
         
                  status = KrnlHlprICMPv6HeaderModifyType(&icmpType,
                                                          pNetBufferList,
                                                          icmpHeaderSize);
                  HLPR_BAIL_ON_FAILURE_2(status);
               }
         
               if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_ICMP_CODE)
               {
                  FWP_VALUE0 icmpCode;
         
                  icmpCode.type  = FWP_UINT8;
                  icmpCode.uint8 = (UINT8)ntohs(pModificationData->transportData.destinationPort);
         
                  status = KrnlHlprICMPv6HeaderModifyCode(&icmpCode,
                                                          pNetBufferList,
                                                          icmpHeaderSize);
                  HLPR_BAIL_ON_FAILURE_2(status);
               }
         
               break;
            }
         }

         /// ToDo: Recalculate the Transport Checksum Here

         HLPR_BAIL_LABEL_2:

         /// return the data offset to the beginning of the IP Header
         tmpStatus = NdisRetreatNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB(pNetBufferList),
                                                   ipHeaderSize,
                                                   0,
                                                   0);
         if(tmpStatus != STATUS_SUCCESS)
         {
            DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                       DPFLTR_ERROR_LEVEL,
                       " !!!! PerformBasicPacketModificationAtOutboundNetwork: NdisRetreatNetBufferDataStart() [status: %#x]\n",
                       status);
         
            HLPR_BAIL;
         }

         HLPR_BAIL_ON_FAILURE(status);
      }
*/
      if(pModificationData->flags & PCPMDF_MODIFY_IP_HEADER)
      {
         if(pModificationData->ipData.flags & PCPMDF_MODIFY_IP_HEADER_SOURCE_ADDRESS)
         {
            FWP_VALUE value;

            RtlZeroMemory(&value,
                          sizeof(FWP_VALUE));

            if(pCompletionData->pInjectionData->addressFamily == AF_INET)
            {
               value.type = FWP_UINT32;

               RtlCopyMemory(&(value.uint32),
                             pModificationData->ipData.sourceAddress.pIPv4,
                             IPV4_ADDRESS_SIZE);
            }
            else
            {
               HLPR_NEW(value.byteArray16,
                        FWP_BYTE_ARRAY16,
                        WFPSAMPLER_CALLOUT_DRIVER_TAG);
               HLPR_BAIL_ON_ALLOC_FAILURE(value.byteArray16,
                                          status);

               value.type = FWP_BYTE_ARRAY16_TYPE;

               RtlCopyMemory(value.byteArray16->byteArray16,
                             pModificationData->ipData.sourceAddress.pIPv6,
                             IPV6_ADDRESS_SIZE);
            }

            status = KrnlHlprIPHeaderModifySourceAddress(&value,
                                                         pNetBufferList,
                                                         TRUE);

            KrnlHlprFwpValuePurgeLocalCopy(&value);

            HLPR_BAIL_ON_FAILURE(status);
         }

         if(pModificationData->ipData.flags & PCPMDF_MODIFY_IP_HEADER_DESTINATION_ADDRESS)
         {
            FWP_VALUE value;

            RtlZeroMemory(&value,
                          sizeof(FWP_VALUE));

            if(pCompletionData->pInjectionData->addressFamily == AF_INET)
            {
               value.type = FWP_UINT32;

               RtlCopyMemory(&(value.uint32),
                             pModificationData->ipData.destinationAddress.pIPv4,
                             IPV4_ADDRESS_SIZE);
            }
            else
            {
#pragma warning(push)
#pragma warning(disable: 6014) /// value.byteArray16 will be freed in with call to KrnlHlprFwpValuePurgeLocalCopy

               HLPR_NEW(value.byteArray16,
                        FWP_BYTE_ARRAY16,
                        WFPSAMPLER_CALLOUT_DRIVER_TAG);
               HLPR_BAIL_ON_ALLOC_FAILURE(value.byteArray16,
                                          status);

#pragma warning(pop)

               value.type = FWP_BYTE_ARRAY16_TYPE;

               RtlCopyMemory(value.byteArray16->byteArray16,
                             pModificationData->ipData.destinationAddress.pIPv6,
                             IPV6_ADDRESS_SIZE);
            }

            status = KrnlHlprIPHeaderModifyDestinationAddress(&value,
                                                              pNetBufferList,
                                                              TRUE);

            KrnlHlprFwpValuePurgeLocalCopy(&value);

            HLPR_BAIL_ON_FAILURE(status);
         }
      }
   }

   status = FwpsInjectNetworkSendAsync(pCompletionData->pInjectionData->injectionHandle,
                                       pCompletionData->pInjectionData->injectionContext,
                                       0,
                                       compartmentID,
                                       pNetBufferList,
                                       CompleteBasicPacketModification,
                                       pCompletionData);

   NT_ASSERT(irql == KeGetCurrentIrql());

   if(status != STATUS_SUCCESS)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! PerformBasicPacketModificationAtOutboundNetwork: FwpsInjectNetworkSendAsync() [status: %#x]\n",
                 status);

   HLPR_BAIL_LABEL:

   NT_ASSERT(status == STATUS_SUCCESS);

   if(status != STATUS_SUCCESS)
   {
      if(pNetBufferList)
      {
         FwpsFreeCloneNetBufferList(pNetBufferList,
                                    0);

         pNetBufferList = 0;
      }

      if(pCompletionData)
         BasicPacketModificationCompletionDataDestroy(&pCompletionData,
                                                      TRUE);
   }

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- PerformBasicPacketModificationAtOutboundNetwork() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

/**
 @private_function="PerformBasicPacketModificationAtForward"
 
   Purpose:  Clones the NET_BUFFER_LIST, modifies it with data from the associated context and 
             injects the clone back to the stack's forward path using FwpsInjectForwardAsync(). <br>
                                                                                                <br>
   Notes:    Applies to the following forwarding layers:                                        <br>
                FWPM_LAYER_IPFORWARD_V{4/6}                                                     <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF551186.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF546324.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
NTSTATUS PerformBasicPacketModificationAtForward(_In_ CLASSIFY_DATA** ppClassifyData,
                                                 _In_ INJECTION_DATA** ppInjectionData,
                                                 _In_ PC_BASIC_PACKET_MODIFICATION_DATA* pModificationData,
                                                 _In_ BOOLEAN isInline = FALSE)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> PerformBasicPacketModificationAtForward()\n");

#endif /// DBG

   NT_ASSERT(ppClassifyData);
   NT_ASSERT(ppInjectionData);
   NT_ASSERT(pModificationData);
   NT_ASSERT(*ppClassifyData);
   NT_ASSERT(*ppInjectionData);

   NTSTATUS                                   status            = STATUS_SUCCESS;
   FWPS_INCOMING_VALUES*                      pClassifyValues   = (FWPS_INCOMING_VALUES*)(*ppClassifyData)->pClassifyValues;
   FWPS_INCOMING_METADATA_VALUES*             pMetadata         = (FWPS_INCOMING_METADATA_VALUES*)(*ppClassifyData)->pMetadataValues;
   COMPARTMENT_ID                             compartmentID     = DEFAULT_COMPARTMENT_ID;
   IF_INDEX                                   interfaceIndex    = 0;
   UINT32                                     flags             = 0;
   NET_BUFFER_LIST*                           pNetBufferList    = 0;
   BASIC_PACKET_MODIFICATION_COMPLETION_DATA* pCompletionData   = 0;
   UINT32                                     ipHeaderSize      = 0;
   FWP_VALUE*                                 pInterfaceIndex   = 0;
   FWP_VALUE*                                 pFlags            = 0;
   BOOLEAN                                    isWeakHostReceive = FALSE;
   BOOLEAN                                    isWeakHostSend    = FALSE;
   PSTR                                       pInjectionFn      = "FwpsInjectForwardAsync";
   NDIS_TCP_IP_CHECKSUM_PACKET_INFO           checksumInfo      = {0};

#if DBG

   KIRQL                                      irql              = KeGetCurrentIrql();

#endif /// DBG

#pragma warning(push)
#pragma warning(disable: 6014) /// pCompletionData will be freed in completionFn using BasicPacketModificationCompletionDataDestroy

   HLPR_NEW(pCompletionData,
            BASIC_PACKET_MODIFICATION_COMPLETION_DATA,
            WFPSAMPLER_CALLOUT_DRIVER_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pCompletionData,
                              status);

#pragma warning(pop)

   KeInitializeSpinLock(&(pCompletionData->spinLock));

   pCompletionData->performedInline = isInline;
   pCompletionData->pClassifyData   = *ppClassifyData;
   pCompletionData->pInjectionData  = *ppInjectionData;

   /// Responsibility for freeing this memory has been transferred to the pCompletionData
   *ppClassifyData = 0;

   *ppInjectionData = 0;

   if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                     FWPS_METADATA_FIELD_COMPARTMENT_ID))
      compartmentID = (COMPARTMENT_ID)pMetadata->compartmentId;

   if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                     FWPS_METADATA_FIELD_IP_HEADER_SIZE))
      ipHeaderSize = pMetadata->ipHeaderSize;

   pInterfaceIndex = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                               &FWPM_CONDITION_DESTINATION_INTERFACE_INDEX);
   if(pInterfaceIndex &&
      pInterfaceIndex->type == FWP_UINT32)
      interfaceIndex = (IF_INDEX)pInterfaceIndex->uint32;

   pFlags = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                      &FWPM_CONDITION_FLAGS);
   if(pFlags &&
      pFlags->type == FWP_UINT32)
      flags = pFlags->uint32;

#if(NTDDI_VERSION >= NTDDI_WIN7)
   
      /// Determine if this is a weakhost forward
      if(flags & FWP_CONDITION_FLAG_IS_INBOUND_PASS_THRU)
         isWeakHostReceive = TRUE;
   
      if(flags & FWP_CONDITION_FLAG_IS_OUTBOUND_PASS_THRU)
         isWeakHostSend = TRUE;
   
#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

   /// Initial offset is at the IP Header, so just clone the entire NET_BUFFER_LIST.
   status = FwpsAllocateCloneNetBufferList((NET_BUFFER_LIST*)pCompletionData->pClassifyData->pPacket,
                                           g_pNDISPoolData->nblPoolHandle,
                                           g_pNDISPoolData->nbPoolHandle,
                                           0,
                                           &pNetBufferList);
   if(status != STATUS_SUCCESS ||
      !pNetBufferList)
   {
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! PerformBasicPacketModificationAtForward: FwpsAllocateCloneNetBufferList() [status: %#x]\n",
                 status);

      HLPR_BAIL;
   }

   checksumInfo.Value = (ULONG)(ULONG_PTR)NET_BUFFER_LIST_INFO((NET_BUFFER_LIST*)pCompletionData->pClassifyData->pPacket,
                                                               TcpIpChecksumNetBufferListInfo);

   /// Handle if this packet had the IP checksum offloaded or if it's loopback
   if(checksumInfo.Receive.NdisPacketIpChecksumSucceeded ||
      flags & FWP_CONDITION_FLAG_IS_LOOPBACK)
   {
      /// Prevent TCP/IP Zone crossing and recalculate the checksums
      if(flags & FWP_CONDITION_FLAG_IS_LOOPBACK)
      {
         FWP_VALUE* pLocalAddress    = 0;
         FWP_VALUE* pRemoteAddress   = 0;
         FWP_VALUE* pLoopbackAddress = 0;

         pLocalAddress = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                    &FWPM_CONDITION_IP_REMOTE_ADDRESS);
         if(pLocalAddress &&
            ((pLocalAddress->type == FWP_UINT32 &&
            RtlCompareMemory(&(pLocalAddress->uint32),
                             IPV4_LOOPBACK_ADDRESS,
                             IPV4_ADDRESS_SIZE)) ||
            (pLocalAddress->type == FWP_BYTE_ARRAY16_TYPE &&
            RtlCompareMemory(pLocalAddress->byteArray16->byteArray16,
                             IPV6_LOOPBACK_ADDRESS,
                             IPV6_ADDRESS_SIZE))))
            pLoopbackAddress = pLocalAddress;

         if(!pLoopbackAddress)
         {
            pRemoteAddress = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                       &FWPM_CONDITION_IP_REMOTE_ADDRESS);
            if(pRemoteAddress &&
               ((pRemoteAddress->type == FWP_UINT32 &&
               RtlCompareMemory(&(pRemoteAddress->uint32),
                                IPV4_LOOPBACK_ADDRESS,
                                IPV4_ADDRESS_SIZE)) ||
               (pRemoteAddress->type == FWP_BYTE_ARRAY16_TYPE &&
               RtlCompareMemory(pRemoteAddress->byteArray16->byteArray16,
                                IPV6_LOOPBACK_ADDRESS,
                                IPV6_ADDRESS_SIZE))))
               pLoopbackAddress = pRemoteAddress;
         }

         if(pLoopbackAddress)
         {
            status = KrnlHlprIPHeaderModifyLoopbackToLocal(pMetadata,
                                                           pLoopbackAddress,
                                                           ipHeaderSize,
                                                           pNetBufferList,
                                                           (const WSACMSGHDR*)pCompletionData->pInjectionData->pControlData,
                                                           pCompletionData->pInjectionData->controlDataLength);
            if(status != STATUS_SUCCESS)
            {
               DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                          DPFLTR_ERROR_LEVEL,
                          " !!!! PerformBasicPacketModificationAtForward: KrnlHlprIPHeaderModifyLoopbackToLocal() [status: %#x]\n",
                          status);

               HLPR_BAIL;
            }
         }
      }
      else
      {
         /// Recalculate the checksum
         if(pCompletionData->pInjectionData->addressFamily == AF_INET)
            KrnlHlprIPHeaderCalculateV4Checksum(pNetBufferList,
                                                ipHeaderSize);
      }
   }

   pCompletionData->refCount = KrnlHlprNBLGetRequiredRefCount(pNetBufferList);

   if(pModificationData->flags)
   {
      /// Various checks and balances must be performed to modify the Transport header at this modification point.
      /// Parsing of the headers will need to occur, as well as spot checking to verify everything is as it should be.
      /// Additionally, checksum routines will need to be written to recalculate checksums for some of the headers.
      /// The following block of code is to get you started with modifying the headers with info not readily available.
      /// (i.e. header parsing has not occurred so there is no relevant classifiable data nor metadata present).
/*
      if(pModificationData->flags & PCPMDF_MODIFY_TRANSPORT_HEADER)
      {
         UINT32  tmpStatus = STATUS_SUCCESS;
         IPPROTO protocol  = IPPROTO_MAX;

         protocol = KrnlHlprIPHeaderGetProtocolField(pNetBufferList,
                                                     pCompletionData->pInjectionData->addressFamily);

         /// The clone is at the IP Header, so advance by the size of the IP Header.
         NdisAdvanceNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB(pNetBufferList),
                                       ipHeaderSize,
                                       FALSE,
                                       0);

         switch(protocol)
         {
            case IPPROTO_ICMP:
            {
               UINT32 icmpHeaderSize = ICMP_HEADER_MIN_SIZE;
         
               if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                                 FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE))
                  icmpHeaderSize = pMetadata->transportHeaderSize;
         
               if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_ICMP_TYPE)
               {
                  FWP_VALUE0 icmpType;
         
                  icmpType.type  = FWP_UINT8;
                  icmpType.uint8 = (UINT8)ntohs(pModificationData->transportData.sourcePort);
         
                  status = KrnlHlprICMPv4HeaderModifyType(&icmpType,
                                                          pNetBufferList,
                                                          icmpHeaderSize);
                  HLPR_BAIL_ON_FAILURE_2(status);
               }
         
               if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_ICMP_CODE)
               {
                  FWP_VALUE0 icmpCode;
               
                  icmpCode.type  = FWP_UINT8;
                  icmpCode.uint8 = (UINT8)ntohs(pModificationData->transportData.destinationPort);
         
                  status = KrnlHlprICMPv4HeaderModifyCode(&icmpCode,
                                                          pNetBufferList,
                                                          icmpHeaderSize);
                  HLPR_BAIL_ON_FAILURE_2(status);
               }
         
               break;
            }
            case IPPROTO_TCP:
            {
               UINT32 tcpHeaderSize = TCP_HEADER_MIN_SIZE;
         
               if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                                 FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE))
                  tcpHeaderSize = pMetadata->transportHeaderSize;
         
               if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_SOURCE_PORT)
               {
                  FWP_VALUE0 srcPort;
         
                  srcPort.type   = FWP_UINT16;
                  srcPort.uint16 = pModificationData->transportData.sourcePort;
         
                  status = KrnlHlprTCPHeaderModifySourcePort(&srcPort,
                                                             pNetBufferList,
                                                             tcpHeaderSize);
                  HLPR_BAIL_ON_FAILURE_2(status);
               }
         
               if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_DESTINATION_PORT)
               {
                  FWP_VALUE0 dstPort;
         
                  dstPort.type   = FWP_UINT16;
                  dstPort.uint16 = pModificationData->transportData.destinationPort;
         
                  status = KrnlHlprTCPHeaderModifyDestinationPort(&dstPort,
                                                                  pNetBufferList,
                                                                  tcpHeaderSize);
                  HLPR_BAIL_ON_FAILURE_2(status);
               }
         
               break;
            }
            case IPPROTO_UDP:
            {
               UINT32 udpHeaderSize = UDP_HEADER_MIN_SIZE;
         
               if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                                 FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE))
                  udpHeaderSize = pMetadata->transportHeaderSize;
         
               if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_SOURCE_PORT)
               {
                  FWP_VALUE0 srcPort;
         
                  srcPort.type   = FWP_UINT16;
                  srcPort.uint16 = pModificationData->transportData.sourcePort;
         
                  status = KrnlHlprUDPHeaderModifySourcePort(&srcPort,
                                                             pNetBufferList,
                                                             udpHeaderSize);
                  HLPR_BAIL_ON_FAILURE_2(status);
               }
         
               if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_DESTINATION_PORT)
               {
                  FWP_VALUE0 dstPort;
         
                  dstPort.type   = FWP_UINT16;
                  dstPort.uint16 = pModificationData->transportData.destinationPort;
         
                  status = KrnlHlprUDPHeaderModifyDestinationPort(&dstPort,
                                                                  pNetBufferList,
                                                                  udpHeaderSize);
                  HLPR_BAIL_ON_FAILURE_2(status);
               }
         
               break;
            }
            case IPPROTO_ICMPV6:
            {
               UINT32 icmpHeaderSize = ICMP_HEADER_MIN_SIZE;
         
               if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                                 FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE))
                  icmpHeaderSize = pMetadata->transportHeaderSize;
         
               if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_ICMP_TYPE)
               {
                  FWP_VALUE0 icmpType;
         
                  icmpType.type  = FWP_UINT8;
                  icmpType.uint8 = (UINT8)ntohs(pModificationData->transportData.sourcePort);
         
                  status = KrnlHlprICMPv6HeaderModifyType(&icmpType,
                                                          pNetBufferList,
                                                          icmpHeaderSize);
                  HLPR_BAIL_ON_FAILURE_2(status);
               }
         
               if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_ICMP_CODE)
               {
                  FWP_VALUE0 icmpCode;
         
                  icmpCode.type  = FWP_UINT8;
                  icmpCode.uint8 = (UINT8)ntohs(pModificationData->transportData.destinationPort);
         
                  status = KrnlHlprICMPv6HeaderModifyCode(&icmpCode,
                                                          pNetBufferList,
                                                          icmpHeaderSize);
                  HLPR_BAIL_ON_FAILURE_2(status);
               }
         
               break;
            }
         }

         /// ToDo: Recalculate the Transport Checksum Here

         HLPR_BAIL_LABEL_2:

         /// return the data offset to the beginning of the IP Header
         tmpStatus = NdisRetreatNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB(pNetBufferList),
                                                   ipHeaderSize,
                                                   0,
                                                   0);
         if(tmpStatus != STATUS_SUCCESS)
         {
            DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                       DPFLTR_ERROR_LEVEL,
                       " !!!! PerformBasicPacketModificationAtForward: NdisRetreatNetBufferDataStart() [status: %#x]\n",
                       status);
         
            HLPR_BAIL;
         }

         HLPR_BAIL_ON_FAILURE(status);
      }
*/
      if(pModificationData->flags & PCPMDF_MODIFY_IP_HEADER)
      {
         if(pModificationData->ipData.flags & PCPMDF_MODIFY_IP_HEADER_INTERFACE_INDEX)
            interfaceIndex = pModificationData->ipData.interfaceIndex;

         if(pModificationData->ipData.flags & PCPMDF_MODIFY_IP_HEADER_SOURCE_ADDRESS)
         {
            FWP_VALUE value;

            RtlZeroMemory(&value,
                          sizeof(FWP_VALUE));

            if(pCompletionData->pInjectionData->addressFamily == AF_INET)
            {
               value.type = FWP_UINT32;

               RtlCopyMemory(&(value.uint32),
                             pModificationData->ipData.sourceAddress.pIPv4,
                             IPV4_ADDRESS_SIZE);
            }
            else
            {
               HLPR_NEW(value.byteArray16,
                        FWP_BYTE_ARRAY16,
                        WFPSAMPLER_CALLOUT_DRIVER_TAG);
               HLPR_BAIL_ON_ALLOC_FAILURE(value.byteArray16,
                                          status);

               value.type = FWP_BYTE_ARRAY16_TYPE;

               RtlCopyMemory(value.byteArray16->byteArray16,
                             pModificationData->ipData.sourceAddress.pIPv6,
                             IPV6_ADDRESS_SIZE);
            }

            status = KrnlHlprIPHeaderModifySourceAddress(&value,
                                                         pNetBufferList,
                                                         TRUE);

            KrnlHlprFwpValuePurgeLocalCopy(&value);

            HLPR_BAIL_ON_FAILURE(status);
         }

         if(pModificationData->ipData.flags & PCPMDF_MODIFY_IP_HEADER_DESTINATION_ADDRESS)
         {
            FWP_VALUE value;

            RtlZeroMemory(&value,
                          sizeof(FWP_VALUE));

            if(pCompletionData->pInjectionData->addressFamily == AF_INET)
            {
               value.type = FWP_UINT32;

               RtlCopyMemory(&(value.uint32),
                             pModificationData->ipData.destinationAddress.pIPv4,
                             IPV4_ADDRESS_SIZE);
            }
            else
            {
#pragma warning(push)
#pragma warning(disable: 6014) /// value.byteArray16 will be freed in with call to KrnlHlprFwpValuePurgeLocalCopy

               HLPR_NEW(value.byteArray16,
                        FWP_BYTE_ARRAY16,
                        WFPSAMPLER_CALLOUT_DRIVER_TAG);
               HLPR_BAIL_ON_ALLOC_FAILURE(value.byteArray16,
                                          status);

#pragma warning(pop)

               value.type = FWP_BYTE_ARRAY16_TYPE;

               RtlCopyMemory(value.byteArray16->byteArray16,
                             pModificationData->ipData.destinationAddress.pIPv6,
                             IPV6_ADDRESS_SIZE);
            }

            status = KrnlHlprIPHeaderModifyDestinationAddress(&value,
                                                              pNetBufferList,
                                                              TRUE);

            KrnlHlprFwpValuePurgeLocalCopy(&value);

            HLPR_BAIL_ON_FAILURE(status);
         }
      }
   }

   /// If the Forwarded NBL is destined locally, inject using FwpsInjectNetworkReceiveAsync rather 
   /// than the traditional FwpsInjectForwardAsync otherwise STATUS_INVALID_PARAMETER will be 
   /// returned in the NBL.status and the injection fails.
   if(isWeakHostReceive)
   {
      UINT32     index              = WFPSAMPLER_INDEX;
      IF_INDEX   subInterfaceIndex  = 0;
      FWP_VALUE* pSubInterfaceIndex = 0;

      if(pCompletionData->pClassifyData->pFilter->subLayerWeight == FWPM_SUBLAYER_UNIVERSAL_WEIGHT)
         index = UNIVERSAL_INDEX;

      if(pCompletionData->pInjectionData->addressFamily == AF_INET)
         pCompletionData->pInjectionData->injectionHandle = g_pIPv4InboundNetworkInjectionHandles[index];
      else
         pCompletionData->pInjectionData->injectionHandle = g_pIPv6InboundNetworkInjectionHandles[index];

      pSubInterfaceIndex = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                     &FWPM_CONDITION_DESTINATION_SUB_INTERFACE_INDEX);
      if(pSubInterfaceIndex &&
         pSubInterfaceIndex->type == FWP_UINT32)
         subInterfaceIndex = (IF_INDEX)pSubInterfaceIndex->uint32;

      status = FwpsInjectNetworkReceiveAsync(pCompletionData->pInjectionData->injectionHandle,
                                             pCompletionData->pInjectionData->injectionContext,
                                             0,
                                             compartmentID,
                                             interfaceIndex,
                                             subInterfaceIndex,
                                             pNetBufferList,
                                             CompleteBasicPacketInjection,
                                             pCompletionData);
   }
   /// If the Forwarded NBL is sourced locally, but another interface, inject using 
   /// FwpsInjectNetworkSendAsync rather than the traditional FwpsInjectForwardAsync otherwise 
   /// STATUS_INVALID_PARAMETER will be returned in the NBL.status and the injection fails
   else if(isWeakHostSend)
   {
      UINT32 index = WFPSAMPLER_INDEX;

      if(pCompletionData->pClassifyData->pFilter->subLayerWeight == FWPM_SUBLAYER_UNIVERSAL_WEIGHT)
         index = UNIVERSAL_INDEX;

      if(pCompletionData->pInjectionData->addressFamily == AF_INET)
         pCompletionData->pInjectionData->injectionHandle = g_pIPv4OutboundNetworkInjectionHandles[index];
      else
         pCompletionData->pInjectionData->injectionHandle = g_pIPv6OutboundNetworkInjectionHandles[index];

      status = FwpsInjectNetworkSendAsync(pCompletionData->pInjectionData->injectionHandle,
                                          pCompletionData->pInjectionData->injectionContext,
                                          0,
                                          compartmentID,
                                          pNetBufferList,
                                          CompleteBasicPacketInjection,
                                          pCompletionData);
   }
   else
      status = FwpsInjectForwardAsync(pCompletionData->pInjectionData->injectionHandle,
                                      pCompletionData->pInjectionData->injectionContext,
                                      0,
                                      pCompletionData->pInjectionData->addressFamily,
                                      compartmentID,
                                      interfaceIndex,
                                      pNetBufferList,
                                      CompleteBasicPacketModification,
                                      pCompletionData);

   NT_ASSERT(irql == KeGetCurrentIrql());

   if(status != STATUS_SUCCESS)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! PerformBasicPacketModificationAtForward: %s() [status: %#x]\n",
                 pInjectionFn,
                 status);

   HLPR_BAIL_LABEL:

   NT_ASSERT(status == STATUS_SUCCESS);

   if(status != STATUS_SUCCESS)
   {
      if(pNetBufferList)
      {
         FwpsFreeCloneNetBufferList(pNetBufferList,
                                    0);

         pNetBufferList = 0;
      }

      if(pCompletionData)
         BasicPacketModificationCompletionDataDestroy(&pCompletionData,
                                                      TRUE);
   }

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- PerformBasicPacketModificationAtForward() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

/**
 @private_function="PerformBasicPacketModificationAtInboundTransport"
 
   Purpose:  Clones the NET_BUFFER_LIST, modifies it with data from the associated context, and 
             injects the clone back to the stack's inbound path from the incoming Transport 
             Layers using FwpsInjectTransportRecveiveAsync().                                   <br>
                                                                                                <br>
   Notes:    Applies to the following inbound layers:                                           <br>
                FWPM_LAYER_INBOUND_TRANSPORT_V{4/6}                                             <br>
                FWPM_LAYER_INBOUND_ICMP_ERROR_V{4/6}                                            <br>
                FWPM_LAYER_DATAGRAM_DATA_V{4/6}        (Inbound only)                           <br>
                FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V{4/6} (Inbound only)                           <br>
                FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V{4/6} (Inbound only)                           <br>
                FWPM_LAYER_ALE_AUTH_CONNECT_V{4/6}     (Inbound, reauthorization only)          <br>
                FWPM_LAYER_ALE_FLOW_ESTABLISHED_V{4/6} (Inbound, non-TCP only)                  <br>
                FWPM_LAYER_STREAM_PACKET_V{4/6}        (Inbound only)                           <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF551186.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF546324.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
NTSTATUS PerformBasicPacketModificationAtInboundTransport(_In_ CLASSIFY_DATA** ppClassifyData,
                                                          _In_ INJECTION_DATA** ppInjectionData,
                                                          _In_ PC_BASIC_PACKET_MODIFICATION_DATA* pModificationData,
                                                          _In_ BOOLEAN isInline = FALSE)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> PerformBasicPacketModificationAtInboundTransport()\n");

#endif /// DBG

   NT_ASSERT(ppClassifyData);
   NT_ASSERT(ppInjectionData);
   NT_ASSERT(pModificationData);
   NT_ASSERT(*ppClassifyData);
   NT_ASSERT(*ppInjectionData);

   NTSTATUS                                   status              = STATUS_SUCCESS;
   FWPS_INCOMING_VALUES*                      pClassifyValues     = (FWPS_INCOMING_VALUES*)(*ppClassifyData)->pClassifyValues;
   FWPS_INCOMING_METADATA_VALUES*             pMetadata           = (FWPS_INCOMING_METADATA_VALUES*)(*ppClassifyData)->pMetadataValues;
   COMPARTMENT_ID                             compartmentID       = DEFAULT_COMPARTMENT_ID;
   IF_INDEX                                   interfaceIndex      = 0;
   IF_INDEX                                   subInterfaceIndex   = 0;
   UINT32                                     flags               = 0;
   NET_BUFFER_LIST*                           pNetBufferList      = 0;
   BASIC_PACKET_MODIFICATION_COMPLETION_DATA* pCompletionData     = 0;
   UINT32                                     ipHeaderSize        = 0;
   UINT32                                     transportHeaderSize = 0;
   UINT32                                     bytesRetreated      = 0;
   IPPROTO                                    protocol            = IPPROTO_MAX;
   FWP_VALUE*                                 pProtocol           = 0;
   FWP_VALUE*                                 pInterfaceIndex     = 0;
   FWP_VALUE*                                 pSubInterfaceIndex  = 0;
   FWP_VALUE*                                 pFlags              = 0;
   FWPS_PACKET_LIST_INFORMATION*              pPacketInformation  = 0;
   BOOLEAN                                    bypassInjection     = FALSE;
   UINT64                                     endpointHandle      = 0;
   BYTE*                                      pSourceAddress      = 0;
   BYTE*                                      pDestinationAddress = 0;
   NDIS_TCP_IP_CHECKSUM_PACKET_INFO           checksumInfo        = {0};

#if DBG

   KIRQL                                      irql                = KeGetCurrentIrql();

#endif /// DBG

#pragma warning(push)
#pragma warning(disable: 6014) /// pCompletionData will be freed in completionFn using BasicPacketModificationCompletionDataDestroy

   HLPR_NEW(pCompletionData,
            BASIC_PACKET_MODIFICATION_COMPLETION_DATA,
            WFPSAMPLER_CALLOUT_DRIVER_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pCompletionData,
                              status);

#pragma warning(pop)

   KeInitializeSpinLock(&(pCompletionData->spinLock));

   pCompletionData->performedInline = isInline;
   pCompletionData->pClassifyData   = *ppClassifyData;
   pCompletionData->pInjectionData  = *ppInjectionData;

   /// Responsibility for freeing this memory has been transferred to the pCompletionData
   *ppClassifyData = 0;

   *ppInjectionData = 0;

   HLPR_NEW(pPacketInformation,
            FWPS_PACKET_LIST_INFORMATION,
            WFPSAMPLER_CALLOUT_DRIVER_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pPacketInformation,
                              status);
   pInterfaceIndex = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                               &FWPM_CONDITION_INTERFACE_INDEX);
   if(pInterfaceIndex &&
      pInterfaceIndex->type == FWP_UINT32)
      interfaceIndex = (IF_INDEX)pInterfaceIndex->uint32;

   pSubInterfaceIndex = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                  &FWPM_CONDITION_SUB_INTERFACE_INDEX);
   if(pSubInterfaceIndex &&
      pSubInterfaceIndex->type == FWP_UINT32)
      subInterfaceIndex = (IF_INDEX)pSubInterfaceIndex->uint32;

   pFlags = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                      &FWPM_CONDITION_FLAGS);
   if(pFlags &&
      pFlags->type == FWP_UINT32)
      flags = pFlags->uint32;

   if(pClassifyValues->layerId == FWPS_LAYER_INBOUND_ICMP_ERROR_V4 ||
      pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V4)
      protocol = IPPROTO_ICMP;
   else if(pClassifyValues->layerId == FWPS_LAYER_INBOUND_ICMP_ERROR_V6 ||
           pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V6)
      protocol = IPPROTO_ICMPV6;

#if(NTDDI_VERSION >= NTDDI_WIN7)

   else if(pClassifyValues->layerId == FWPS_LAYER_STREAM_PACKET_V4 ||
           pClassifyValues->layerId == FWPS_LAYER_STREAM_PACKET_V6)
      protocol = IPPROTO_TCP;

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

   else
   {
      pProtocol = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                            &FWPM_CONDITION_IP_PROTOCOL);
      HLPR_BAIL_ON_NULL_POINTER(pProtocol);

      protocol = (IPPROTO)pProtocol->uint8;
   }

   if(pClassifyValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4)
   {
      ipHeaderSize = IPV4_HEADER_MIN_SIZE;
   
      if(protocol == IPPROTO_ICMP)
         transportHeaderSize = ICMP_HEADER_MIN_SIZE;
      else if(protocol == IPPROTO_TCP)
         transportHeaderSize = TCP_HEADER_MIN_SIZE;
      else if(protocol == IPPROTO_UDP)
         transportHeaderSize = UDP_HEADER_MIN_SIZE;
   }
   else if(pClassifyValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6)
   {
      ipHeaderSize = IPV6_HEADER_MIN_SIZE;

      if(protocol == IPPROTO_ICMPV6)
         transportHeaderSize = ICMP_HEADER_MIN_SIZE;
      else if(protocol == IPPROTO_TCP)
         transportHeaderSize = TCP_HEADER_MIN_SIZE;
      else if(protocol == IPPROTO_UDP)
         transportHeaderSize = UDP_HEADER_MIN_SIZE;
   }

   if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                     FWPS_METADATA_FIELD_COMPARTMENT_ID))
      compartmentID = (COMPARTMENT_ID)pMetadata->compartmentId;

   if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                     FWPS_METADATA_FIELD_IP_HEADER_SIZE) &&
      pMetadata->ipHeaderSize)
      ipHeaderSize = pMetadata->ipHeaderSize;

   if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                     FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE) &&
      pMetadata->transportHeaderSize)
      transportHeaderSize = pMetadata->transportHeaderSize;

   bytesRetreated = ipHeaderSize;

   if(protocol != IPPROTO_ICMP &&
      protocol != IPPROTO_ICMPV6)
   {
      if(!isInline &&
         protocol != IPPROTO_TCP &&
         !(protocol == IPPROTO_UDP &&
         flags & FWP_CONDITION_FLAG_IS_RAW_ENDPOINT) &&
         (pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4 ||
         pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6))
      {
         /// For asynchronous execution, the drop will cause the stack to continue processing on the 
         /// NBL for auditing purposes.  This processing retreats the NBL Offset to the Transport header.
         /// We need to take this into account because we only took a reference on the NBL.
      }
      else
      {
         if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                           FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE))
            bytesRetreated += transportHeaderSize;
      }
   }
   else
   {
      if(pClassifyValues->layerId == FWPS_LAYER_DATAGRAM_DATA_V4 ||
         pClassifyValues->layerId == FWPS_LAYER_DATAGRAM_DATA_V6 ||
         pClassifyValues->layerId == FWPS_LAYER_INBOUND_ICMP_ERROR_V4 ||
         pClassifyValues->layerId == FWPS_LAYER_INBOUND_ICMP_ERROR_V6)
      {
         if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                           FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE))
            bytesRetreated += transportHeaderSize;
      }
   }

   if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                     FWPS_METADATA_FIELD_TRANSPORT_ENDPOINT_HANDLE))
      endpointHandle = pMetadata->transportEndpointHandle;

   /// Query to see if IPsec has applied tunnel mode SA's to this NET_BUFFER_LIST ...
   status = FwpsGetPacketListSecurityInformation((NET_BUFFER_LIST*)pCompletionData->pClassifyData->pPacket,
                                                 FWPS_PACKET_LIST_INFORMATION_QUERY_ALL_INBOUND,
                                                 pPacketInformation);
   if(status != STATUS_SUCCESS)
   {
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! PerformBasicPacketModificationAtInboundTransport: FwpsGetPacketListSecurityInformation() [status: %#x]\n",
                 status);

      HLPR_BAIL;
   }

   /// ... if it has, then bypass the injection until the NET_BUFFER_LIST has come out of the tunnel
   if((pPacketInformation->ipsecInformation.inbound.isTunnelMode &&
      !(pPacketInformation->ipsecInformation.inbound.isDeTunneled)) ||
      pPacketInformation->ipsecInformation.inbound.isSecure)
   {
      bypassInjection = TRUE;

      HLPR_BAIL;
   }

   /// Initial offset is at the data, so retreat the size of the IP Header and Transport Header ...
   /// except for ICMP, offset is at the ICMP Header, so retreat the size of the IP Header ...
   status = NdisRetreatNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)pCompletionData->pClassifyData->pPacket),
                                          bytesRetreated,
                                          0,
                                          0);
   if(status != STATUS_SUCCESS)
   {
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! PerformBasicPacketModificationAtInboundTransport: NdisRetreatNetBufferDataStart() [status: %#x]\n",
                 status);

      HLPR_BAIL;
   }

   /// ... clone the entire NET_BUFFER_LIST ...
   status = FwpsAllocateCloneNetBufferList((NET_BUFFER_LIST*)pCompletionData->pClassifyData->pPacket,
                                           g_pNDISPoolData->nblPoolHandle,
                                           g_pNDISPoolData->nbPoolHandle,
                                           0,
                                           &pNetBufferList);

   /// ... and advance the offset back to the original position.
   NdisAdvanceNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)pCompletionData->pClassifyData->pPacket),
                                 bytesRetreated,
                                 FALSE,
                                 0);
   if(status != STATUS_SUCCESS ||
      !pNetBufferList)
   {
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! PerformBasicPacketModificationAtInboundTransport: FwpsAllocateCloneNetBufferList() [status: %#x]\n",
                 status);

      HLPR_BAIL;
   }

   checksumInfo.Value = (ULONG)(ULONG_PTR)NET_BUFFER_LIST_INFO((NET_BUFFER_LIST*)pCompletionData->pClassifyData->pPacket,
                                                               TcpIpChecksumNetBufferListInfo);

   /// Handle if the packet was IPsec secured
   if(pCompletionData->pInjectionData->isIPsecSecured)
   {
      /// For performance reasons, IPsec leaves the original ESP / AH information in the IP Header ...
      UINT32     headerIncludeSize   = 0;
      UINT32     ipv4Address         = 0;
      UINT32     addressSize         = 0;
      FWP_VALUE* pRemoteAddressValue = 0;
      FWP_VALUE* pLocalAddressValue  = 0;
      FWP_VALUE* pProtocolValue      = 0;

      pRemoteAddressValue = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                      &FWPM_CONDITION_IP_REMOTE_ADDRESS);
      if(pRemoteAddressValue)
      {
         if(pRemoteAddressValue->type == FWP_BYTE_ARRAY16_TYPE)
            addressSize = IPV6_ADDRESS_SIZE;
         else
            addressSize = IPV4_ADDRESS_SIZE;               

         HLPR_NEW_ARRAY(pSourceAddress,
                        BYTE,
                        addressSize,
                        WFPSAMPLER_CALLOUT_DRIVER_TAG);
         HLPR_BAIL_ON_ALLOC_FAILURE(pSourceAddress,
                                    status);

         if(pRemoteAddressValue->type == FWP_BYTE_ARRAY16_TYPE)
            RtlCopyMemory(pSourceAddress,
                          pRemoteAddressValue->byteArray16->byteArray16,
                          addressSize);
         else
         {
            ipv4Address = htonl(pRemoteAddressValue->uint32);

            RtlCopyMemory(pSourceAddress,
                          &ipv4Address,
                          addressSize);
         }
      }

      pLocalAddressValue = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                     &FWPM_CONDITION_IP_LOCAL_ADDRESS);
      if(pLocalAddressValue)
      {
         if(pLocalAddressValue->type == FWP_BYTE_ARRAY16_TYPE)
            addressSize = IPV6_ADDRESS_SIZE;
         else
            addressSize = IPV4_ADDRESS_SIZE;

         HLPR_NEW_ARRAY(pDestinationAddress,
                        BYTE,
                        addressSize,
                        WFPSAMPLER_CALLOUT_DRIVER_TAG);
         HLPR_BAIL_ON_ALLOC_FAILURE(pDestinationAddress,
                                    status);

         if(pLocalAddressValue->type == FWP_BYTE_ARRAY16_TYPE)
            RtlCopyMemory(pDestinationAddress,
                          pLocalAddressValue->byteArray16->byteArray16,
                          addressSize);
         else
         {
            ipv4Address = htonl(pLocalAddressValue->uint32);

            RtlCopyMemory(pDestinationAddress,
                          &ipv4Address,
                          addressSize);
         }            
      }

      pProtocolValue = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                 &FWPM_CONDITION_IP_PROTOCOL);
      if(pProtocolValue &&
         pProtocolValue->type == FWP_UINT8)
         protocol = (IPPROTO)pProtocolValue->uint8;
      else
        protocol = IPPROTO_MAX;

      NT_ASSERT(protocol != IPPROTO_MAX);

#if (NTDDI_VERSION >= NTDDI_WIN6SP1)

      if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_METADATA_FIELD_TRANSPORT_HEADER_INCLUDE_HEADER))
         headerIncludeSize = pMetadata->headerIncludeHeaderLength;

#endif // (NTDDI_VERSION >= NTDDI_WIN6SP1)

      if(pSourceAddress == 0 ||
         pDestinationAddress == 0)
      {
         status = STATUS_INVALID_MEMBER;

         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_ERROR_LEVEL,
                    " !!!! PerformBasicPacketModificationAtInboundTransport() [status: %#x][pSourceAddress: %#p][pDestinationAddress: %#p]\n",
                    status,
                    pSourceAddress,
                    pDestinationAddress);

         HLPR_BAIL;
      }

      /// ... so we must re-construct the IPHeader with the appropriate information
      /// for checksum offload, this will recalculate the checksums
      status = FwpsConstructIpHeaderForTransportPacket(pNetBufferList,
                                                       headerIncludeSize,
                                                       pCompletionData->pInjectionData->addressFamily,
                                                       pSourceAddress,
                                                       pDestinationAddress,
                                                       protocol,
                                                       endpointHandle,
                                                       (const WSACMSGHDR*)pCompletionData->pInjectionData->pControlData,
                                                       pCompletionData->pInjectionData->controlDataLength,
                                                       0,
                                                       0,
                                                       interfaceIndex,
                                                       subInterfaceIndex);
      if(status != STATUS_SUCCESS)
      {
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_ERROR_LEVEL,
                    " !!!! PerformBasicPacketInjectionAtInboundTransport: FwpsConstructIpHeaderForTransportPacket() [status: %#x]\n",
                    status);

         HLPR_BAIL;
      }
   }
   /// Handle if this packet had the IP or Transport checksums offloaded or if it's loopback
   else if(checksumInfo.Receive.NdisPacketIpChecksumSucceeded ||
           checksumInfo.Receive.NdisPacketTcpChecksumSucceeded ||
           checksumInfo.Receive.NdisPacketUdpChecksumSucceeded ||
           flags & FWP_CONDITION_FLAG_IS_LOOPBACK)
   {
      /// Prevent TCP/IP Zone crossing and recalculate the checksums
      if(flags & FWP_CONDITION_FLAG_IS_LOOPBACK)
      {
         FWP_VALUE* pLocalAddress    = 0;
         FWP_VALUE* pRemoteAddress   = 0;
         FWP_VALUE* pLoopbackAddress = 0;

         pLocalAddress = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                    &FWPM_CONDITION_IP_REMOTE_ADDRESS);
         if(pLocalAddress &&
            ((pLocalAddress->type == FWP_UINT32 &&
            RtlCompareMemory(&(pLocalAddress->uint32),
                             IPV4_LOOPBACK_ADDRESS,
                             IPV4_ADDRESS_SIZE)) ||
            (pLocalAddress->type == FWP_BYTE_ARRAY16_TYPE &&
            RtlCompareMemory(pLocalAddress->byteArray16->byteArray16,
                             IPV6_LOOPBACK_ADDRESS,
                             IPV6_ADDRESS_SIZE))))
            pLoopbackAddress = pLocalAddress;

         if(!pLoopbackAddress)
         {
            pRemoteAddress = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                       &FWPM_CONDITION_IP_REMOTE_ADDRESS);
            if(pRemoteAddress &&
               ((pRemoteAddress->type == FWP_UINT32 &&
               RtlCompareMemory(&(pRemoteAddress->uint32),
                                IPV4_LOOPBACK_ADDRESS,
                                IPV4_ADDRESS_SIZE)) ||
               (pRemoteAddress->type == FWP_BYTE_ARRAY16_TYPE &&
               RtlCompareMemory(pRemoteAddress->byteArray16->byteArray16,
                                IPV6_LOOPBACK_ADDRESS,
                                IPV6_ADDRESS_SIZE))))
               pLoopbackAddress = pRemoteAddress;
         }

         if(pLoopbackAddress)
         {
            status = KrnlHlprIPHeaderModifyLoopbackToLocal(pMetadata,
                                                           pLoopbackAddress,
                                                           ipHeaderSize,
                                                           pNetBufferList,
                                                           (const WSACMSGHDR*)pCompletionData->pInjectionData->pControlData,
                                                           pCompletionData->pInjectionData->controlDataLength);
            if(status != STATUS_SUCCESS)
            {
               DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                          DPFLTR_ERROR_LEVEL,
                          " !!!! PerformBasicPacketModificationAtInboundTransport: KrnlHlprIPHeaderModifyLoopbackToLocal() [status: %#x]\n",
                          status);

               HLPR_BAIL;
            }
         }
      }
      else
      {
         /// Recalculate the checksum
         if(pCompletionData->pInjectionData->addressFamily == AF_INET)
            KrnlHlprIPHeaderCalculateV4Checksum(pNetBufferList,
                                                ipHeaderSize);
      }
   }

   pCompletionData->refCount = KrnlHlprNBLGetRequiredRefCount(pNetBufferList);

   if(pModificationData->flags)
   {
      FWP_VALUE* pLocalAddressValue        = 0;
      FWP_VALUE* pRemoteAddressValue       = 0;
      BYTE       pIPDestinationAddress[16] = {0};
      BYTE       pIPSourceAddress[16]      = {0};

      pRemoteAddressValue = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                      &FWPM_CONDITION_IP_REMOTE_ADDRESS);
      HLPR_BAIL_ON_NULL_POINTER_WITH_STATUS(pRemoteAddressValue,
                                            status);

      pLocalAddressValue = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                     &FWPM_CONDITION_IP_LOCAL_ADDRESS);
      HLPR_BAIL_ON_NULL_POINTER_WITH_STATUS(pLocalAddressValue,
                                            status);

      if(pCompletionData->pInjectionData->addressFamily == AF_INET6)
      {
         RtlCopyMemory(pIPSourceAddress,
                       pRemoteAddressValue->byteArray16->byteArray16,
                       IPV6_ADDRESS_SIZE);

         RtlCopyMemory(pIPDestinationAddress,
                       pLocalAddressValue->byteArray16->byteArray16,
                       IPV6_ADDRESS_SIZE);
      }
      else
      {
         UINT32 sourceAddress      = htonl(pRemoteAddressValue->uint32);
         UINT32 destinationAddress = htonl(pLocalAddressValue->uint32);
          
         RtlCopyMemory(pIPSourceAddress,
                        &sourceAddress,
                        IPV4_ADDRESS_SIZE);
         
         RtlCopyMemory(pIPDestinationAddress,
                       &destinationAddress,
                       IPV4_ADDRESS_SIZE);
      }

      if(pModificationData->flags & PCPMDF_MODIFY_TRANSPORT_HEADER)
      {
         UINT32     tmpStatus      = STATUS_SUCCESS;
         FWP_VALUE* pProtocolValue = 0;

         /// The clone is at the IP Header, so advance by the size of the IP Header.
         NdisAdvanceNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB(pNetBufferList),
                                       ipHeaderSize,
                                       FALSE,
                                       0);

         pProtocolValue = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                    &FWPM_CONDITION_IP_PROTOCOL);
         if(pProtocolValue &&
            pProtocolValue->type == FWP_UINT8)
            protocol = (IPPROTO)pProtocolValue->uint8;
         else
            protocol = IPPROTO_MAX;

         NT_ASSERT(protocol != IPPROTO_MAX);
         
         if(pClassifyValues->layerId == FWPS_LAYER_INBOUND_ICMP_ERROR_V4)
            protocol = IPPROTO_ICMP;
         else if(pClassifyValues->layerId == FWPS_LAYER_INBOUND_ICMP_ERROR_V6)
            protocol = IPPROTO_ICMPV6;
         
         switch(protocol)
         {
            case IPPROTO_ICMP:
            {
               UINT32 icmpHeaderSize = transportHeaderSize ? transportHeaderSize : ICMP_HEADER_MIN_SIZE;

               if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_ICMP_TYPE)
               {
                  FWP_VALUE0 icmpType;
         
                  icmpType.type  = FWP_UINT8;
                  icmpType.uint8 = (UINT8)ntohs(pModificationData->transportData.sourcePort);
         
                  status = KrnlHlprICMPv4HeaderModifyType(&icmpType,
                                                          pNetBufferList,
                                                          icmpHeaderSize);
                  HLPR_BAIL_ON_FAILURE_2(status);
               }
         
               if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_ICMP_CODE)
               {
                  FWP_VALUE0 icmpCode;
               
                  icmpCode.type  = FWP_UINT8;
                  icmpCode.uint8 = (UINT8)ntohs(pModificationData->transportData.destinationPort);
         
                  status = KrnlHlprICMPv4HeaderModifyCode(&icmpCode,
                                                          pNetBufferList,
                                                          icmpHeaderSize);
                  HLPR_BAIL_ON_FAILURE_2(status);
               }
         
               break;
            }
            case IPPROTO_TCP:
            {
               UINT32 tcpHeaderSize = transportHeaderSize ? transportHeaderSize : TCP_HEADER_MIN_SIZE;
         
               if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_SOURCE_PORT)
               {
                  FWP_VALUE0 srcPort;
         
                  srcPort.type   = FWP_UINT16;
                  srcPort.uint16 = pModificationData->transportData.sourcePort;
         
                  status = KrnlHlprTCPHeaderModifySourcePort(&srcPort,
                                                             pNetBufferList,
                                                             tcpHeaderSize);
                  HLPR_BAIL_ON_FAILURE_2(status);
               }
         
               if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_DESTINATION_PORT)
               {
                  FWP_VALUE0 dstPort;
         
                  dstPort.type   = FWP_UINT16;
                  dstPort.uint16 = pModificationData->transportData.destinationPort;
         
                  status = KrnlHlprTCPHeaderModifyDestinationPort(&dstPort,
                                                                  pNetBufferList,
                                                                  tcpHeaderSize);
                  HLPR_BAIL_ON_FAILURE_2(status);
               }
         
               break;
            }
            case IPPROTO_UDP:
            {
               UINT32 udpHeaderSize = transportHeaderSize ? transportHeaderSize : UDP_HEADER_MIN_SIZE;
         
               if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_SOURCE_PORT)
               {
                  FWP_VALUE0 srcPort;
         
                  srcPort.type   = FWP_UINT16;
                  srcPort.uint16 = pModificationData->transportData.sourcePort;
         
                  status = KrnlHlprUDPHeaderModifySourcePort(&srcPort,
                                                             pNetBufferList,
                                                             udpHeaderSize);
                  HLPR_BAIL_ON_FAILURE_2(status);
               }
         
               if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_DESTINATION_PORT)
               {
                  FWP_VALUE0 dstPort;
         
                  dstPort.type   = FWP_UINT16;
                  dstPort.uint16 = pModificationData->transportData.destinationPort;
         
                  status = KrnlHlprUDPHeaderModifyDestinationPort(&dstPort,
                                                                  pNetBufferList,
                                                                  udpHeaderSize);
                  HLPR_BAIL_ON_FAILURE_2(status);
               }
         
               break;
            }
            case IPPROTO_ICMPV6:
            {
               UINT32 icmpHeaderSize = transportHeaderSize ? transportHeaderSize : ICMP_HEADER_MIN_SIZE;
         
               if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_ICMP_TYPE)
               {
                  FWP_VALUE0 icmpType;
         
                  icmpType.type  = FWP_UINT8;
                  icmpType.uint8 = (UINT8)ntohs(pModificationData->transportData.sourcePort);
         
                  status = KrnlHlprICMPv6HeaderModifyType(&icmpType,
                                                          pNetBufferList,
                                                          icmpHeaderSize);
                  HLPR_BAIL_ON_FAILURE_2(status);
               }
         
               if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_ICMP_CODE)
               {
                  FWP_VALUE0 icmpCode;
         
                  icmpCode.type  = FWP_UINT8;
                  icmpCode.uint8 = (UINT8)ntohs(pModificationData->transportData.destinationPort);
         
                  status = KrnlHlprICMPv6HeaderModifyCode(&icmpCode,
                                                          pNetBufferList,
                                                          icmpHeaderSize);
                  HLPR_BAIL_ON_FAILURE_2(status);
               }
         
               break;
            }
         }

         HLPR_BAIL_LABEL_2:

         /// return the data offset to the beginning of the IP Header
         tmpStatus = NdisRetreatNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB(pNetBufferList),
                                                   ipHeaderSize,
                                                   0,
                                                   0);
         if(tmpStatus != STATUS_SUCCESS)
         {
            DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                       DPFLTR_ERROR_LEVEL,
                       " !!!! PerformBasicPacketModificationAtInboundTransport: NdisRetreatNetBufferDataStart() [status: %#x]\n",
                       status);
         
            HLPR_BAIL;
         }

         HLPR_BAIL_ON_FAILURE(status);
      }

      if(pModificationData->flags & PCPMDF_MODIFY_IP_HEADER)
      {
         if(pModificationData->ipData.flags & PCPMDF_MODIFY_IP_HEADER_INTERFACE_INDEX)
            interfaceIndex = pModificationData->ipData.interfaceIndex;

         if(pModificationData->ipData.flags & PCPMDF_MODIFY_IP_HEADER_SOURCE_ADDRESS)
         {
            FWP_VALUE value;

            RtlZeroMemory(&value,
                          sizeof(FWP_VALUE));

            if(pCompletionData->pInjectionData->addressFamily == AF_INET)
            {
               value.type = FWP_UINT32;

               RtlCopyMemory(&(value.uint32),
                             pModificationData->ipData.sourceAddress.pIPv4,
                             IPV4_ADDRESS_SIZE);

               RtlCopyMemory(pIPSourceAddress,
                             pModificationData->ipData.sourceAddress.pIPv4,
                             IPV4_ADDRESS_SIZE);                             
            }
            else
            {
               HLPR_NEW(value.byteArray16,
                        FWP_BYTE_ARRAY16,
                        WFPSAMPLER_CALLOUT_DRIVER_TAG);
               HLPR_BAIL_ON_ALLOC_FAILURE(value.byteArray16,
                                          status);

               value.type = FWP_BYTE_ARRAY16_TYPE;

               RtlCopyMemory(value.byteArray16->byteArray16,
                             pModificationData->ipData.sourceAddress.pIPv6,
                             IPV6_ADDRESS_SIZE);

               RtlCopyMemory(pIPSourceAddress,
                             pModificationData->ipData.sourceAddress.pIPv6,
                             IPV6_ADDRESS_SIZE);
            }

            status = KrnlHlprIPHeaderModifySourceAddress(&value,
                                                         pNetBufferList,
                                                         FALSE);

            KrnlHlprFwpValuePurgeLocalCopy(&value);

            HLPR_BAIL_ON_FAILURE(status);
         }

         if(pModificationData->ipData.flags & PCPMDF_MODIFY_IP_HEADER_DESTINATION_ADDRESS)
         {
            FWP_VALUE value;

            RtlZeroMemory(&value,
                          sizeof(FWP_VALUE));

            if(pCompletionData->pInjectionData->addressFamily == AF_INET)
            {
               value.type = FWP_UINT32;

               RtlCopyMemory(&(value.uint32),
                             pModificationData->ipData.destinationAddress.pIPv4,
                             IPV4_ADDRESS_SIZE);

               RtlCopyMemory(pIPDestinationAddress,
                             pModificationData->ipData.destinationAddress.pIPv4,
                             IPV4_ADDRESS_SIZE);
            }
            else
            {
#pragma warning(push)
#pragma warning(disable: 6014) /// value.byteArray16 will be freed in with call to KrnlHlprFwpValuePurgeLocalCopy

               HLPR_NEW(value.byteArray16,
                        FWP_BYTE_ARRAY16,
                        WFPSAMPLER_CALLOUT_DRIVER_TAG);
               HLPR_BAIL_ON_ALLOC_FAILURE(value.byteArray16,
                                          status);

#pragma warning(pop)

               value.type = FWP_BYTE_ARRAY16_TYPE;

               RtlCopyMemory(value.byteArray16->byteArray16,
                             pModificationData->ipData.destinationAddress.pIPv6,
                             IPV6_ADDRESS_SIZE);

               RtlCopyMemory(pIPDestinationAddress,
                             pModificationData->ipData.destinationAddress.pIPv6,
                             IPV6_ADDRESS_SIZE);
            }

            status = KrnlHlprIPHeaderModifyDestinationAddress(&value,
                                                              pNetBufferList,
                                                              FALSE);

            KrnlHlprFwpValuePurgeLocalCopy(&value);

            HLPR_BAIL_ON_FAILURE(status);

         }
      }

      status = FwpsConstructIpHeaderForTransportPacket(pNetBufferList,
                                                       ipHeaderSize,
                                                       pCompletionData->pInjectionData->addressFamily,
                                                       (UCHAR*)pIPSourceAddress,
                                                       (UCHAR*)pIPDestinationAddress,
                                                       protocol,
                                                       endpointHandle,
                                                       (const WSACMSGHDR*)pCompletionData->pInjectionData->pControlData,
                                                       pCompletionData->pInjectionData->controlDataLength,
                                                       0,
                                                       0,
                                                       interfaceIndex,
                                                       subInterfaceIndex);
      HLPR_BAIL_ON_FAILURE(status);
   }

   status = FwpsInjectTransportReceiveAsync(pCompletionData->pInjectionData->injectionHandle,
                                            pCompletionData->pInjectionData->injectionContext,
                                            0,
                                            0,
                                            pCompletionData->pInjectionData->addressFamily,
                                            compartmentID,
                                            interfaceIndex,
                                            subInterfaceIndex,
                                            pNetBufferList,
                                            CompleteBasicPacketModification,
                                            pCompletionData);

   NT_ASSERT(irql == KeGetCurrentIrql());

   if(status != STATUS_SUCCESS)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! PerformBasicPacketInjectionAtInboundTransport: FwpsInjectTransportReceiveAsync() [status: %#x]\n",
                 status);

   HLPR_BAIL_LABEL:

   NT_ASSERT(status == STATUS_SUCCESS);

   if(status != STATUS_SUCCESS ||
      bypassInjection)
   {
      if(pNetBufferList)
      {
         FwpsFreeCloneNetBufferList(pNetBufferList,
                                    0);

         pNetBufferList = 0;
      }

      if(pCompletionData)
         BasicPacketModificationCompletionDataDestroy(&pCompletionData,
                                                      TRUE);
   }

   HLPR_DELETE_ARRAY(pSourceAddress,
                     WFPSAMPLER_CALLOUT_DRIVER_TAG);

   HLPR_DELETE_ARRAY(pDestinationAddress,
                     WFPSAMPLER_CALLOUT_DRIVER_TAG);

   HLPR_DELETE(pPacketInformation,
               WFPSAMPLER_CALLOUT_DRIVER_TAG);

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- PerformBasicPacketModificationAtInboundTransport() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

/**
 @private_function="PerformBasicPacketModificationAtOutboundTransport"
 
   Purpose:  Clones the NET_BUFFER_LIST, modifies it with data from the associated context, and 
             injects the clone back to the stack's outbound path from the outgoing Transport 
             Layers using FwpsInjectTransportSendAsync().                                       <br>
                                                                                                <br>
   Notes:    Applies to the following outbound layers:                                          <br>
                FWPM_LAYER_OUTBOUND_TRANSPORT_V{4/6}                                            <br>
                FWPM_LAYER_OUTBOUND_ICMP_ERROR_V{4/6}                                           <br>
                FWPM_LAYER_DATAGRAM_DATA_V{4/6}        (Outbound only)                          <br>
                FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V{4/6} (Outbound only)                          <br>
                FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V{4/6} (Outbound only)                          <br>
                FWPM_LAYER_ALE_AUTH_CONNECT_V{4/6}     (Outbound, reauthorization only)         <br>
                FWPM_LAYER_ALE_FLOW_ESTABLISHED_V{4/6} (Outbound, non-TCP only)                 <br>
                FWPM_LAYER_STREAM_PACKET_V{4/6}        (Outbound only)                          <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF551188.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF546324.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
NTSTATUS PerformBasicPacketModificationAtOutboundTransport(_In_ CLASSIFY_DATA** ppClassifyData,
                                                           _In_ INJECTION_DATA** ppInjectionData,
                                                           _In_ PC_BASIC_PACKET_MODIFICATION_DATA* pModificationData,
                                                           _In_ BOOLEAN isInline = FALSE)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> PerformBasicPacketModificationAtOutboundTransport()\n");

#endif /// DBG

   NT_ASSERT(ppClassifyData);
   NT_ASSERT(ppInjectionData);
   NT_ASSERT(pModificationData);
   NT_ASSERT(*ppClassifyData);
   NT_ASSERT(*ppInjectionData);

   NTSTATUS                                   status          = STATUS_SUCCESS;
   FWPS_INCOMING_VALUES*                      pClassifyValues = (FWPS_INCOMING_VALUES*)(*ppClassifyData)->pClassifyValues;
   FWPS_INCOMING_METADATA_VALUES*             pMetadata       = (FWPS_INCOMING_METADATA_VALUES*)(*ppClassifyData)->pMetadataValues;
   UINT64                                     endpointHandle  = 0;
   FWPS_TRANSPORT_SEND_PARAMS*                pSendParams     = 0;
   COMPARTMENT_ID                             compartmentID   = DEFAULT_COMPARTMENT_ID;
   NET_BUFFER_LIST*                           pNetBufferList  = 0;
   BASIC_PACKET_MODIFICATION_COMPLETION_DATA* pCompletionData = 0;
   BYTE*                                      pRemoteAddress  = 0;
   FWP_VALUE*                                 pAddressValue   = 0;

#if DBG

   KIRQL                                      irql            = KeGetCurrentIrql();

#endif /// DBG

#pragma warning(push)
#pragma warning(disable: 6014) /// pCompletionData & pSendParams will be freed in completionFn using BasicPacketModificationCompletionDataDestroy

   HLPR_NEW(pCompletionData,
            BASIC_PACKET_MODIFICATION_COMPLETION_DATA,
            WFPSAMPLER_CALLOUT_DRIVER_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pCompletionData,
                              status);

   HLPR_NEW(pSendParams,
            FWPS_TRANSPORT_SEND_PARAMS,
            WFPSAMPLER_CALLOUT_DRIVER_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pSendParams,
                              status);

#pragma warning(pop)

   KeInitializeSpinLock(&(pCompletionData->spinLock));

   pCompletionData->performedInline = isInline;
   pCompletionData->pClassifyData   = *ppClassifyData;
   pCompletionData->pInjectionData  = *ppInjectionData;
   pCompletionData->pSendParams     = pSendParams;

   /// Responsibility for freeing this memory has been transferred to the pCompletionData
   *ppClassifyData = 0;

   *ppInjectionData = 0;

   pSendParams = 0;

   if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                     FWPS_METADATA_FIELD_TRANSPORT_ENDPOINT_HANDLE))
      endpointHandle = pMetadata->transportEndpointHandle;

   if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                     FWPS_METADATA_FIELD_COMPARTMENT_ID))
      compartmentID = (COMPARTMENT_ID)pMetadata->compartmentId;

   if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                     FWPS_METADATA_FIELD_TRANSPORT_CONTROL_DATA))
   {
      pSendParams->controlData       = pMetadata->controlData;
      pSendParams->controlDataLength = pMetadata->controlDataLength;
   }

   pAddressValue = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                             &FWPM_CONDITION_IP_REMOTE_ADDRESS);
   if(pAddressValue)
   {
      if(pCompletionData->pInjectionData->addressFamily == AF_INET)
      {
         UINT32 tempAddress = htonl(pAddressValue->uint32);

         HLPR_NEW_ARRAY(pRemoteAddress,
                        BYTE,
                        IPV4_ADDRESS_SIZE,
                        WFPSAMPLER_CALLOUT_DRIVER_TAG);
         HLPR_BAIL_ON_ALLOC_FAILURE(pRemoteAddress,
                                    status);

         RtlCopyMemory(pRemoteAddress,
                       &tempAddress,
                       IPV4_ADDRESS_SIZE);
      }
      else
      {
#pragma warning(push)
#pragma warning(disable: 6014) /// pRemoteAddress will be freed in completionFn using BasicPacketModificationCompletionDataDestroy

         HLPR_NEW_ARRAY(pRemoteAddress,
                        BYTE,
                        IPV6_ADDRESS_SIZE,
                        WFPSAMPLER_CALLOUT_DRIVER_TAG);
         HLPR_BAIL_ON_ALLOC_FAILURE(pRemoteAddress,
                                    status);

#pragma warning(pop)

         RtlCopyMemory(pRemoteAddress,
                       pAddressValue->byteArray16->byteArray16,
                       IPV6_ADDRESS_SIZE);

         if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                           FWPS_METADATA_FIELD_REMOTE_SCOPE_ID))
            pCompletionData->pSendParams->remoteScopeId = pMetadata->remoteScopeId;
      }

      pCompletionData->pSendParams->remoteAddress = pRemoteAddress;
   }

   pCompletionData->pSendParams->controlData       = (WSACMSGHDR*)pCompletionData->pInjectionData->pControlData;
   pCompletionData->pSendParams->controlDataLength = pCompletionData->pInjectionData->controlDataLength;

   /// Initial offset is at Transport Header (no IP Header yet), so just clone entire NET_BUFFER_LIST.
   status = FwpsAllocateCloneNetBufferList((NET_BUFFER_LIST*)pCompletionData->pClassifyData->pPacket,
                                           g_pNDISPoolData->nblPoolHandle,
                                           g_pNDISPoolData->nbPoolHandle,
                                           0,
                                           &pNetBufferList);
   if(status != STATUS_SUCCESS)
   {
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! PerformBasicPacketModificationAtOutboundTransport: FwpsAllocateCloneNetBufferList() [status: %#x]\n",
                 status);

      HLPR_BAIL;
   }

   pCompletionData->refCount = KrnlHlprNBLGetRequiredRefCount(pNetBufferList);

   if(pModificationData->flags)
   {
      if(pModificationData->flags & PCPMDF_MODIFY_TRANSPORT_HEADER)
      {
         FWP_VALUE* pProtocolValue = 0;
         IPPROTO    protocol       = IPPROTO_MAX;

         pProtocolValue = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                    &FWPM_CONDITION_IP_PROTOCOL);
         if(pProtocolValue &&
            pProtocolValue->type == FWP_UINT8)
            protocol = (IPPROTO)pProtocolValue->uint8;

         if(pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V4)
            protocol = IPPROTO_ICMP;
         else if(pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V6)
            protocol = IPPROTO_ICMPV6;

#if(NTDDI_VERSION >= NTDDI_WIN7)

         else if(pClassifyValues->layerId == FWPS_LAYER_STREAM_PACKET_V4 ||
                 pClassifyValues->layerId == FWPS_LAYER_STREAM_PACKET_V6)
            protocol = IPPROTO_TCP;

#endif

         switch(protocol)
         {
            case IPPROTO_ICMP:
            {
               UINT32 icmpHeaderSize = ICMP_HEADER_MIN_SIZE;

               if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                                 FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE))
                  icmpHeaderSize = pMetadata->transportHeaderSize;

               if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_ICMP_TYPE)
               {
                  FWP_VALUE0 icmpType;

                  icmpType.type  = FWP_UINT8;
                  icmpType.uint8 = (UINT8)ntohs(pModificationData->transportData.sourcePort);

                  status = KrnlHlprICMPv4HeaderModifyType(&icmpType,
                                                          pNetBufferList,
                                                          icmpHeaderSize);
                  HLPR_BAIL_ON_FAILURE(status);
               }

               if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_ICMP_CODE)
               {
                  FWP_VALUE0 icmpCode;
               
                  icmpCode.type  = FWP_UINT8;
                  icmpCode.uint8 = (UINT8)ntohs(pModificationData->transportData.destinationPort);

                  status = KrnlHlprICMPv4HeaderModifyCode(&icmpCode,
                                                          pNetBufferList,
                                                          icmpHeaderSize);
                  HLPR_BAIL_ON_FAILURE(status);
               }

               break;
            }
            case IPPROTO_TCP:
            {
               UINT32 tcpHeaderSize = TCP_HEADER_MIN_SIZE;

               if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                                 FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE))
                  tcpHeaderSize = pMetadata->transportHeaderSize;

               if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_SOURCE_PORT)
               {
                  FWP_VALUE0 srcPort;

                  srcPort.type   = FWP_UINT16;
                  srcPort.uint16 = pModificationData->transportData.sourcePort;

                  status = KrnlHlprTCPHeaderModifySourcePort(&srcPort,
                                                             pNetBufferList,
                                                             tcpHeaderSize);
                  HLPR_BAIL_ON_FAILURE(status);
               }

               if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_DESTINATION_PORT)
               {
                  FWP_VALUE0 dstPort;

                  dstPort.type   = FWP_UINT16;
                  dstPort.uint16 = pModificationData->transportData.destinationPort;

                  status = KrnlHlprTCPHeaderModifyDestinationPort(&dstPort,
                                                                  pNetBufferList,
                                                                  tcpHeaderSize);
                  HLPR_BAIL_ON_FAILURE(status);
               }

               break;
            }
            case IPPROTO_UDP:
            {
               UINT32 udpHeaderSize = UDP_HEADER_MIN_SIZE;

               if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                                 FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE))
                  udpHeaderSize = pMetadata->transportHeaderSize;

               if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_SOURCE_PORT)
               {
                  FWP_VALUE0 srcPort;

                  srcPort.type   = FWP_UINT16;
                  srcPort.uint16 = pModificationData->transportData.sourcePort;

                  status = KrnlHlprUDPHeaderModifySourcePort(&srcPort,
                                                             pNetBufferList,
                                                             udpHeaderSize);
                  HLPR_BAIL_ON_FAILURE(status);
               }

               if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_DESTINATION_PORT)
               {
                  FWP_VALUE0 dstPort;

                  dstPort.type   = FWP_UINT16;
                  dstPort.uint16 = pModificationData->transportData.destinationPort;

                  status = KrnlHlprUDPHeaderModifyDestinationPort(&dstPort,
                                                                  pNetBufferList,
                                                                  udpHeaderSize);
                  HLPR_BAIL_ON_FAILURE(status);
               }

               break;
            }
            case IPPROTO_ICMPV6:
            {
               UINT32 icmpHeaderSize = ICMP_HEADER_MIN_SIZE;

               if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                                 FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE))
                  icmpHeaderSize = pMetadata->transportHeaderSize;

               if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_ICMP_TYPE)
               {
                  FWP_VALUE0 icmpType;

                  icmpType.type  = FWP_UINT8;
                  icmpType.uint8 = (UINT8)ntohs(pModificationData->transportData.sourcePort);

                  status = KrnlHlprICMPv6HeaderModifyType(&icmpType,
                                                          pNetBufferList,
                                                          icmpHeaderSize);
                  HLPR_BAIL_ON_FAILURE(status);
               }

               if(pModificationData->transportData.flags & PCPMDF_MODIFY_TRANSPORT_HEADER_ICMP_CODE)
               {
                  FWP_VALUE0 icmpCode;

                  icmpCode.type  = FWP_UINT8;
                  icmpCode.uint8 = (UINT8)ntohs(pModificationData->transportData.destinationPort);

                  status = KrnlHlprICMPv6HeaderModifyCode(&icmpCode,
                                                          pNetBufferList,
                                                          icmpHeaderSize);
                  HLPR_BAIL_ON_FAILURE(status);
               }

               break;
            }
         }
      }

      /// As there is no IP Header yet, we can only modify the destination IP address via the FWPS_TRANSPORT_SEND_PARAMS
      if(pModificationData->flags & PCPMDF_MODIFY_IP_HEADER)
      {
         if(pModificationData->ipData.flags & PCPMDF_MODIFY_IP_HEADER_DESTINATION_ADDRESS)
         {
            if(pCompletionData->pInjectionData->addressFamily == AF_INET)
               RtlCopyMemory(pCompletionData->pSendParams->remoteAddress,
                             pModificationData->ipData.destinationAddress.pIPv4,
                             IPV4_ADDRESS_SIZE);
            else
               RtlCopyMemory(pCompletionData->pSendParams->remoteAddress,
                             pModificationData->ipData.destinationAddress.pIPv6,
                             IPV6_ADDRESS_SIZE);
         }
      }
   }

   status = FwpsInjectTransportSendAsync(pCompletionData->pInjectionData->injectionHandle,
                                         pCompletionData->pInjectionData->injectionContext,
                                         endpointHandle,
                                         0,
                                         pCompletionData->pSendParams,
                                         pCompletionData->pInjectionData->addressFamily,
                                         compartmentID,
                                         pNetBufferList,
                                         CompleteBasicPacketModification,
                                         pCompletionData);

   NT_ASSERT(irql == KeGetCurrentIrql());

   if(status != STATUS_SUCCESS)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! PerformBasicPacketModificationAtOutboundTransport: FwpsInjectTransportSendAsync() [status: %#x]\n",
                 status);

   HLPR_BAIL_LABEL:

   NT_ASSERT(status == STATUS_SUCCESS);

   if(status != STATUS_SUCCESS)
   {
      if(pNetBufferList)
      {
         FwpsFreeCloneNetBufferList(pNetBufferList,
                                    0);

         pNetBufferList = 0;
      }

      if(pCompletionData)
         BasicPacketModificationCompletionDataDestroy(&pCompletionData,
                                                      TRUE);
   }

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- PerformBasicPacketModificationAtOutboundTransport() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

/**
 @private_function="BasicPacketModificationDeferredProcedureCall"
 
   Purpose:  Invokes the appropriate private injection routine to perform the injection at 
             DISPATCH_LEVEL.                                                                    <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF542972.aspx             <br>
*/
_IRQL_requires_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Function_class_(KDEFERRED_ROUTINE)
VOID BasicPacketModificationDeferredProcedureCall(_In_ KDPC* pDPC,
                                                  _In_opt_ PVOID pContext,
                                                  _In_opt_ PVOID pArg1,
                                                  _In_opt_ PVOID pArg2)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> BasicPacketModificationDeferredProcedureCall()\n");

#endif /// DBG

   UNREFERENCED_PARAMETER(pDPC);
   UNREFERENCED_PARAMETER(pContext);
   UNREFERENCED_PARAMETER(pArg2);

   NT_ASSERT(pDPC);
   NT_ASSERT(pArg1);
   NT_ASSERT(((DPC_DATA*)pArg1)->pClassifyData);
   NT_ASSERT(((DPC_DATA*)pArg1)->pInjectionData);

   DPC_DATA* pDPCData = (DPC_DATA*)pArg1;

   if(pDPCData)
   {
      NTSTATUS                           status            = STATUS_SUCCESS;
      FWPS_INCOMING_VALUES*              pClassifyValues   = (FWPS_INCOMING_VALUES*)pDPCData->pClassifyData->pClassifyValues;
      FWPS_FILTER*                       pFilter           = (FWPS_FILTER*)pDPCData->pClassifyData->pFilter;
      PC_BASIC_PACKET_MODIFICATION_DATA* pModificationData = (PC_BASIC_PACKET_MODIFICATION_DATA*)pFilter->providerContext->dataBuffer->data;

      if(pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V4 ||
         pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V6)
         status = PerformBasicPacketModificationAtInboundNetwork(&(pDPCData->pClassifyData),
                                                                 &(pDPCData->pInjectionData),
                                                                 pModificationData,
                                                                 FALSE);
      else if(pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_IPPACKET_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_IPPACKET_V6)
         status = PerformBasicPacketModificationAtOutboundNetwork(&(pDPCData->pClassifyData),
                                                                  &(pDPCData->pInjectionData),
                                                                  pModificationData,
                                                                  FALSE);
      else if(pClassifyValues->layerId == FWPS_LAYER_IPFORWARD_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_IPFORWARD_V6)
         status = PerformBasicPacketModificationAtForward(&(pDPCData->pClassifyData),
                                                          &(pDPCData->pInjectionData),
                                                          pModificationData,
                                                          FALSE);
      else if(pClassifyValues->layerId == FWPS_LAYER_INBOUND_TRANSPORT_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_INBOUND_TRANSPORT_V6 ||
              pClassifyValues->layerId == FWPS_LAYER_INBOUND_ICMP_ERROR_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_INBOUND_ICMP_ERROR_V6 ||
              (pDPCData->pInjectionData->direction == FWP_DIRECTION_INBOUND &&
              (pClassifyValues->layerId == FWPS_LAYER_DATAGRAM_DATA_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_DATAGRAM_DATA_V6 ||
              pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6 ||
              pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V4 ||     /// Policy Change Reauthorization
              pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V6 ||     /// Policy Change Reauthorization
              pClassifyValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6)))
         status = PerformBasicPacketModificationAtInboundTransport(&(pDPCData->pClassifyData),
                                                                   &(pDPCData->pInjectionData),
                                                                   pModificationData,
                                                                   FALSE);
      else if(pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_TRANSPORT_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_TRANSPORT_V6 ||
              pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V6 ||
              (pDPCData->pInjectionData->direction == FWP_DIRECTION_OUTBOUND &&
              (pClassifyValues->layerId == FWPS_LAYER_DATAGRAM_DATA_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_DATAGRAM_DATA_V6 ||
              pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4 || /// Policy Change Reauthorization
              pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6 || /// Policy Change Reauthorization
              pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V6 ||
              pClassifyValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6)))
         status = PerformBasicPacketModificationAtOutboundTransport(&(pDPCData->pClassifyData),
                                                                    &(pDPCData->pInjectionData),
                                                                    pModificationData,
                                                                    FALSE);

#if(NTDDI_VERSION >= NTDDI_WIN7)

      else if(pDPCData->pInjectionData->direction == FWP_DIRECTION_INBOUND &&
              (pClassifyValues->layerId == FWPS_LAYER_STREAM_PACKET_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_STREAM_PACKET_V6))
         status = PerformBasicPacketModificationAtInboundTransport(&(pDPCData->pClassifyData),
                                                                   &(pDPCData->pInjectionData),
                                                                   pModificationData,
                                                                   FALSE);
      else if(pDPCData->pInjectionData->direction == FWP_DIRECTION_OUTBOUND &&
              (pClassifyValues->layerId == FWPS_LAYER_STREAM_PACKET_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_STREAM_PACKET_V6))
         status = PerformBasicPacketModificationAtOutboundTransport(&(pDPCData->pClassifyData),
                                                                    &(pDPCData->pInjectionData),
                                                                    pModificationData,
                                                                    FALSE);

#if(NTDDI_VERSION >= NTDDI_WIN8)

      else if(pClassifyValues->layerId == FWPS_LAYER_INBOUND_MAC_FRAME_ETHERNET ||
              pClassifyValues->layerId == FWPS_LAYER_INBOUND_MAC_FRAME_NATIVE)
         status = PerformBasicPacketModificationAtInboundMACFrame(&(pDPCData->pClassifyData),
                                                                  &(pDPCData->pInjectionData),
                                                                  pModificationData,
                                                                  FALSE);
      else if(pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_MAC_FRAME_ETHERNET ||
              pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_MAC_FRAME_NATIVE)
         status = PerformBasicPacketModificationAtOutboundMACFrame(&(pDPCData->pClassifyData),
                                                                   &(pDPCData->pInjectionData),
                                                                   pModificationData,
                                                                   FALSE);
      else if(pClassifyValues->layerId == FWPS_LAYER_INGRESS_VSWITCH_ETHERNET)
         status = PerformBasicPacketModificationAtIngressVSwitchEthernet(&(pDPCData->pClassifyData),
                                                                         &(pDPCData->pInjectionData),
                                                                         pModificationData,
                                                                         FALSE);
      else if(pClassifyValues->layerId == FWPS_LAYER_EGRESS_VSWITCH_ETHERNET)
         status = PerformBasicPacketModificationAtEgressVSwitchEthernet(&(pDPCData->pClassifyData),
                                                                        &(pDPCData->pInjectionData),
                                                                        pModificationData,
                                                                        FALSE);

#endif // (NTDDI_VERSION >= NTDDI_WIN8)
#endif // (NTDDI_VERSION >= NTDDI_WIN7)

      else
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_ERROR_LEVEL,
                    " !!!! BasicPacketModificationDeferredProcedureCall() [status: %#x]\n",
                    (UINT32)STATUS_NOT_SUPPORTED);

      if(status != STATUS_SUCCESS)
      {
         if(pDPCData->pClassifyData)
            KrnlHlprClassifyDataDestroyLocalCopy(&(pDPCData->pClassifyData));

         if(pDPCData->pInjectionData)
            KrnlHlprInjectionDataDestroy(&(pDPCData->pInjectionData));

         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_ERROR_LEVEL,
                    "  !!!! BasicPacketModificationDeferredProcedureCall: PerformBasicPacketModification() [status: %#x]\n",
                    status);
      }

      KrnlHlprDPCDataDestroy(&pDPCData);
   }

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- BasicPacketModificationDeferredProcedureCall()\n");

#endif /// DBG

   return;
}

/**
 @private_function="BasicPacketModificationWorkItemRoutine"
 
   Purpose:  Invokes the appropriate private routine to perform the modification and injection 
             at PASSIVE_LEVEL.                                                                  <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF566380.aspx             <br>
*/
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Function_class_(IO_WORKITEM_ROUTINE)
VOID BasicPacketModificationWorkItemRoutine(_In_ PDEVICE_OBJECT pDeviceObject,
                                            _Inout_opt_ PVOID pContext)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> BasicPacketModificationWorkItemRoutine()\n");

#endif /// DBG

   UNREFERENCED_PARAMETER(pDeviceObject);

   NT_ASSERT(pContext);
   NT_ASSERT(((WORKITEM_DATA*)pContext)->pClassifyData);
   NT_ASSERT(((WORKITEM_DATA*)pContext)->pInjectionData);

   WORKITEM_DATA* pWorkItemData = (WORKITEM_DATA*)pContext;

   if(pWorkItemData)
   {
      NTSTATUS                           status            = STATUS_SUCCESS;
      FWPS_INCOMING_VALUES*              pClassifyValues   = (FWPS_INCOMING_VALUES*)pWorkItemData->pClassifyData->pClassifyValues;
      FWPS_FILTER*                       pFilter           = (FWPS_FILTER*)pWorkItemData->pClassifyData->pFilter;
      PC_BASIC_PACKET_MODIFICATION_DATA* pModificationData = (PC_BASIC_PACKET_MODIFICATION_DATA*)pFilter->providerContext->dataBuffer->data;

      if(pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V4 ||
         pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V6)
         status = PerformBasicPacketModificationAtInboundNetwork(&(pWorkItemData->pClassifyData),
                                                                 &(pWorkItemData->pInjectionData),
                                                                 pModificationData,
                                                                 FALSE);
      else if(pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_IPPACKET_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_IPPACKET_V6)
         status = PerformBasicPacketModificationAtOutboundNetwork(&(pWorkItemData->pClassifyData),
                                                                  &(pWorkItemData->pInjectionData),
                                                                  pModificationData,
                                                                  FALSE);
      else if(pClassifyValues->layerId == FWPS_LAYER_IPFORWARD_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_IPFORWARD_V6)
         status = PerformBasicPacketModificationAtForward(&(pWorkItemData->pClassifyData),
                                                          &(pWorkItemData->pInjectionData),
                                                          pModificationData,
                                                          FALSE);
      else if(pClassifyValues->layerId == FWPS_LAYER_INBOUND_TRANSPORT_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_INBOUND_TRANSPORT_V6 ||
              pClassifyValues->layerId == FWPS_LAYER_INBOUND_ICMP_ERROR_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_INBOUND_ICMP_ERROR_V6 ||
              (pWorkItemData->pInjectionData->direction == FWP_DIRECTION_INBOUND &&
              (pClassifyValues->layerId == FWPS_LAYER_DATAGRAM_DATA_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_DATAGRAM_DATA_V6 ||
              pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6 ||
              pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V4 ||     /// Policy Change Reauthorization
              pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V6 ||     /// Policy Change Reauthorization
              pClassifyValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6)))
         status = PerformBasicPacketModificationAtInboundTransport(&(pWorkItemData->pClassifyData),
                                                                   &(pWorkItemData->pInjectionData),
                                                                   pModificationData,
                                                                   FALSE);
      else if(pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_TRANSPORT_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_TRANSPORT_V6 ||
              pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V6 ||
              (pWorkItemData->pInjectionData->direction == FWP_DIRECTION_OUTBOUND &&
              (pClassifyValues->layerId == FWPS_LAYER_DATAGRAM_DATA_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_DATAGRAM_DATA_V6 ||
              pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4 || /// Policy Change Reauthorization
              pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6 || /// Policy Change Reauthorization
              pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V6 ||
              pClassifyValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6)))
         status = PerformBasicPacketModificationAtOutboundTransport(&(pWorkItemData->pClassifyData),
                                                                    &(pWorkItemData->pInjectionData),
                                                                    pModificationData,
                                                                    FALSE);

#if(NTDDI_VERSION >= NTDDI_WIN7)

      else if(pWorkItemData->pInjectionData->direction == FWP_DIRECTION_INBOUND &&
              (pClassifyValues->layerId == FWPS_LAYER_STREAM_PACKET_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_STREAM_PACKET_V6))
         status = PerformBasicPacketModificationAtInboundTransport(&(pWorkItemData->pClassifyData),
                                                                   &(pWorkItemData->pInjectionData),
                                                                   pModificationData,
                                                                   FALSE);
      else if(pWorkItemData->pInjectionData->direction == FWP_DIRECTION_OUTBOUND &&
              (pClassifyValues->layerId == FWPS_LAYER_STREAM_PACKET_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_STREAM_PACKET_V6))
         status = PerformBasicPacketModificationAtOutboundTransport(&(pWorkItemData->pClassifyData),
                                                                    &(pWorkItemData->pInjectionData),
                                                                    pModificationData,
                                                                    FALSE);

#if(NTDDI_VERSION >= NTDDI_WIN8)

      else if(pClassifyValues->layerId == FWPS_LAYER_INBOUND_MAC_FRAME_ETHERNET ||
              pClassifyValues->layerId == FWPS_LAYER_INBOUND_MAC_FRAME_NATIVE)
         status = PerformBasicPacketModificationAtInboundMACFrame(&(pWorkItemData->pClassifyData),
                                                                  &(pWorkItemData->pInjectionData),
                                                                  pModificationData,
                                                                  FALSE);
      else if(pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_MAC_FRAME_ETHERNET ||
              pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_MAC_FRAME_NATIVE)
         status = PerformBasicPacketModificationAtOutboundMACFrame(&(pWorkItemData->pClassifyData),
                                                                   &(pWorkItemData->pInjectionData),
                                                                   pModificationData,
                                                                   FALSE);
      else if(pClassifyValues->layerId == FWPS_LAYER_INGRESS_VSWITCH_ETHERNET)
         status = PerformBasicPacketModificationAtIngressVSwitchEthernet(&(pWorkItemData->pClassifyData),
                                                                         &(pWorkItemData->pInjectionData),
                                                                         pModificationData,
                                                                         FALSE);
      else if(pClassifyValues->layerId == FWPS_LAYER_EGRESS_VSWITCH_ETHERNET)
         status = PerformBasicPacketModificationAtEgressVSwitchEthernet(&(pWorkItemData->pClassifyData),
                                                                        &(pWorkItemData->pInjectionData),
                                                                        pModificationData,
                                                                        FALSE);

#endif // (NTDDI_VERSION >= NTDDI_WIN8)
#endif // (NTDDI_VERSION >= NTDDI_WIN7)

      else
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_ERROR_LEVEL,
                    " !!!! BasicPacketModificationWorkItemRoutine() [status: %#x]\n",
                    (UINT32)STATUS_NOT_SUPPORTED);

      if(status != STATUS_SUCCESS)
      {
         if(pWorkItemData->pClassifyData)
            KrnlHlprClassifyDataDestroyLocalCopy(&(pWorkItemData->pClassifyData));

         if(pWorkItemData->pInjectionData)
            KrnlHlprInjectionDataDestroy(&(pWorkItemData->pInjectionData));

         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_ERROR_LEVEL,
                    "  !!!! BasicPacketModificationWorkItemRoutine: PerformBasicPacketModification() [status: %#x]\n",
                    status);
      }

      KrnlHlprWorkItemDataDestroy(&pWorkItemData);
   }

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- BasicPacketModificationWorkItemRoutine()\n");

#endif /// DBG

   return;
}

/**
 @private_function="TriggerBasicPacketModificationInline"
 
   Purpose:  Makes a reference to all the classification data structures and invokes the 
             appropriate private routine to perform the modification and injection.             <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
NTSTATUS TriggerBasicPacketModificationInline(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                              _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                              _Inout_opt_ VOID* pNetBufferList,
                                              _In_opt_ const VOID* pClassifyContext,
                                              _In_ const FWPS_FILTER* pFilter,
                                              _In_ UINT64 flowContext,
                                              _Inout_ FWPS_CLASSIFY_OUT* pClassifyOut,
                                              _In_ INJECTION_DATA** ppInjectionData)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> TriggerBasicPacketModificationInline()\n");

#endif /// DBG

   NT_ASSERT(pClassifyValues);
   NT_ASSERT(pMetadata);
   NT_ASSERT(pNetBufferList);
   NT_ASSERT(pFilter);
   NT_ASSERT(pClassifyOut);
   NT_ASSERT(ppInjectionData);
   NT_ASSERT(*ppInjectionData);

   NTSTATUS                           status            = STATUS_SUCCESS;
   CLASSIFY_DATA*                     pClassifyData     = 0;
   PC_BASIC_PACKET_MODIFICATION_DATA* pModificationData = (PC_BASIC_PACKET_MODIFICATION_DATA*)pFilter->providerContext->dataBuffer->data;

#pragma warning(push)
#pragma warning(disable: 6014) /// pClassifyData will be freed in completionFn using BasicPacketModificationCompletionDataDestroy

   HLPR_NEW(pClassifyData,
            CLASSIFY_DATA,
            WFPSAMPLER_CALLOUT_DRIVER_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pClassifyData,
                              status);

#pragma warning(pop)

   pClassifyData->pClassifyValues  = pClassifyValues;
   pClassifyData->pMetadataValues  = pMetadata;
   pClassifyData->pPacket          = pNetBufferList;
   pClassifyData->pClassifyContext = pClassifyContext;
   pClassifyData->pFilter          = pFilter;
   pClassifyData->flowContext      = flowContext;
   pClassifyData->pClassifyOut     = pClassifyOut;

   if(pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V4 ||
      pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V6)
      status = PerformBasicPacketModificationAtInboundNetwork(&pClassifyData,
                                                              ppInjectionData,
                                                              pModificationData,
                                                              TRUE);
   else if(pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_IPPACKET_V4 ||
           pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_IPPACKET_V6)
      status = PerformBasicPacketModificationAtOutboundNetwork(&pClassifyData,
                                                               ppInjectionData,
                                                               pModificationData,
                                                               TRUE);
   else if(pClassifyValues->layerId == FWPS_LAYER_IPFORWARD_V4 ||
           pClassifyValues->layerId == FWPS_LAYER_IPFORWARD_V6)
      status = PerformBasicPacketModificationAtForward(&pClassifyData,
                                                       ppInjectionData,
                                                       pModificationData,
                                                       TRUE);
   else if(pClassifyValues->layerId == FWPS_LAYER_INBOUND_TRANSPORT_V4 ||
           pClassifyValues->layerId == FWPS_LAYER_INBOUND_TRANSPORT_V6 ||
           pClassifyValues->layerId == FWPS_LAYER_INBOUND_ICMP_ERROR_V4 ||
           pClassifyValues->layerId == FWPS_LAYER_INBOUND_ICMP_ERROR_V6 ||
           ((*ppInjectionData)->direction == FWP_DIRECTION_INBOUND &&
           (pClassifyValues->layerId == FWPS_LAYER_DATAGRAM_DATA_V4 ||
           pClassifyValues->layerId == FWPS_LAYER_DATAGRAM_DATA_V6 ||
           pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4 ||
           pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6 ||
           pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V4 ||     /// Policy Change Reauthorization
           pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V6 ||     /// Policy Change Reauthorization
           pClassifyValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4 ||
           pClassifyValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6)))
      status = PerformBasicPacketModificationAtInboundTransport(&pClassifyData,
                                                                ppInjectionData,
                                                                pModificationData,
                                                                TRUE);
   else if(pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_TRANSPORT_V4 ||
           pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_TRANSPORT_V6 ||
           pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V4 ||
           pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V6 ||
           ((*ppInjectionData)->direction == FWP_DIRECTION_OUTBOUND &&
           (pClassifyValues->layerId == FWPS_LAYER_DATAGRAM_DATA_V4 ||
           pClassifyValues->layerId == FWPS_LAYER_DATAGRAM_DATA_V6 ||
           pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4 || /// Policy Change Reauthorization
           pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6 || /// Policy Change Reauthorization
           pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V4 ||
           pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V6 ||
           pClassifyValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4 ||
           pClassifyValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6)))
      status = PerformBasicPacketModificationAtOutboundTransport(&pClassifyData,
                                                                 ppInjectionData,
                                                                 pModificationData,
                                                                 TRUE);

#if(NTDDI_VERSION >= NTDDI_WIN7)

      else if((*ppInjectionData)->direction == FWP_DIRECTION_INBOUND &&
              (pClassifyValues->layerId == FWPS_LAYER_STREAM_PACKET_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_STREAM_PACKET_V6))
         status = PerformBasicPacketModificationAtInboundTransport(&pClassifyData,
                                                                   ppInjectionData,
                                                                   pModificationData,
                                                                   TRUE);
      else if((*ppInjectionData)->direction == FWP_DIRECTION_OUTBOUND &&
              (pClassifyValues->layerId == FWPS_LAYER_STREAM_PACKET_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_STREAM_PACKET_V6))
         status = PerformBasicPacketModificationAtOutboundTransport(&pClassifyData,
                                                                    ppInjectionData,
                                                                    pModificationData,
                                                                    TRUE);

#if(NTDDI_VERSION >= NTDDI_WIN8)

   else if(pClassifyValues->layerId == FWPS_LAYER_INBOUND_MAC_FRAME_ETHERNET ||
           pClassifyValues->layerId == FWPS_LAYER_INBOUND_MAC_FRAME_NATIVE)
      status = PerformBasicPacketModificationAtInboundMACFrame(&pClassifyData,
                                                               ppInjectionData,
                                                               pModificationData,
                                                               TRUE);
   else if(pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_MAC_FRAME_ETHERNET ||
           pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_MAC_FRAME_NATIVE)
      status = PerformBasicPacketModificationAtOutboundMACFrame(&pClassifyData,
                                                                ppInjectionData,
                                                                pModificationData,
                                                                TRUE);
   else if(pClassifyValues->layerId == FWPS_LAYER_INGRESS_VSWITCH_ETHERNET)
      status = PerformBasicPacketModificationAtIngressVSwitchEthernet(&pClassifyData,
                                                                      ppInjectionData,
                                                                      pModificationData,
                                                                      TRUE);
   else if(pClassifyValues->layerId == FWPS_LAYER_EGRESS_VSWITCH_ETHERNET)
      status = PerformBasicPacketModificationAtEgressVSwitchEthernet(&pClassifyData,
                                                                     ppInjectionData,
                                                                     pModificationData,
                                                                     TRUE);

#endif // (NTDDI_VERSION >= NTDDI_WIN8)
#endif // (NTDDI_VERSION >= NTDDI_WIN7)

   else
   {
      status = STATUS_NOT_SUPPORTED;

      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! TriggerBasicPacketModificationInline() [status: %#x]\n",
                 status);
   }

   HLPR_BAIL_LABEL:

   NT_ASSERT(status == STATUS_SUCCESS);

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- TriggerBasicPacketModificationInline() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

/**
 @private_function="TriggerBasicPacketModificationOutOfBand"
 
   Purpose:  Creates a local copy of the classification data structures and queues a WorkItem 
             to perform the modification and injection at PASSIVE_LEVEL.                        <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF550679.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF566380.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
NTSTATUS TriggerBasicPacketModificationOutOfBand(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                                 _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                                 _Inout_opt_ VOID* pNetBufferList,
                                                 _In_opt_ const VOID* pClassifyContext,
                                                 _In_ const FWPS_FILTER* pFilter,
                                                 _In_ UINT64 flowContext,
                                                 _In_ FWPS_CLASSIFY_OUT* pClassifyOut,
                                                 _In_ INJECTION_DATA* pInjectionData,
                                                 _In_ PC_BASIC_PACKET_MODIFICATION_DATA* pPCData)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> TriggerBasicPacketModificationOutOfBand()\n");

#endif /// DBG

   NT_ASSERT(pClassifyValues);
   NT_ASSERT(pMetadata);
   NT_ASSERT(pNetBufferList);
   NT_ASSERT(pFilter);
   NT_ASSERT(pClassifyOut);
   NT_ASSERT(pInjectionData);
   NT_ASSERT(pPCData);

   NTSTATUS       status        = STATUS_SUCCESS;
   CLASSIFY_DATA* pClassifyData = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pClassifyData will be freed in completionFn using BasicPacketModificationCompletionDataDestroy

   status = KrnlHlprClassifyDataCreateLocalCopy(&pClassifyData,
                                                pClassifyValues,
                                                pMetadata,
                                                pNetBufferList,
                                                pClassifyContext,
                                                pFilter,
                                                flowContext,
                                                pClassifyOut);
   HLPR_BAIL_ON_FAILURE(status);

#pragma warning(pop)

   if(pPCData->useWorkItems)
      status = KrnlHlprWorkItemQueue(g_pWDMDevice,
                                     BasicPacketModificationWorkItemRoutine,
                                     pClassifyData,
                                     pInjectionData,
                                     0);
   else if(pPCData->useThreadedDPC)
      status = KrnlHlprThreadedDPCQueue(BasicPacketModificationDeferredProcedureCall,
                                        pClassifyData,
                                        pInjectionData,
                                        0);
   else
      status = KrnlHlprDPCQueue(BasicPacketModificationDeferredProcedureCall,
                                pClassifyData,
                                pInjectionData,
                                0);

   HLPR_BAIL_LABEL:

   NT_ASSERT(status == STATUS_SUCCESS);

   if(status != STATUS_SUCCESS)
   {
      if(pClassifyData)
         KrnlHlprClassifyDataDestroyLocalCopy(&pClassifyData);
   }

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- TriggerBasicPacketModificationOutOfBand() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

#if(NTDDI_VERSION >= NTDDI_WIN7)

/**
 @classify_function="ClassifyBasicPacketModification"
 
   Purpose:  Blocks the current NET_BUFFER_LIST, modifies a clone of the NBL with the specified 
             data and injects the clone back to the stack's data path.                          <br>
                                                                                                <br>
   Notes:    Applies to the following layers:                                                   <br>
                FWPS_LAYER_INBOUND_IPPACKET_V{4/6}                                              <br>
                FWPS_LAYER_OUTBOUND_IPPACKET_V{4/6}                                             <br>
                FWPS_LAYER_IPFORWARD_V{4/6}                                                     <br>
                FWPS_LAYER_INBOUND_TRANSPORT_V{4/6}                                             <br>
                FWPS_LAYER_OUTBOUND_TRANSPORT_V{4/6}                                            <br>
                FWPS_LAYER_DATAGRAM_DATA_V{4/6}                                                 <br>
                FWPS_LAYER_INBOUND_ICMP_ERROR_V{4/6}                                            <br>
                FWPS_LAYER_OUTBOUND_ICMP_ERROR_V{4/6}                                           <br>
                FWPS_LAYER_ALE_AUTH_CONNECT_V{4/6}                                              <br>
                FWPS_LAYER_ALE_FLOW_ESTABLISHED_V{4/6}                                          <br>
                FWPS_LAYER_STREAM_PACKET_V{4/6}                                                 <br>
                FWPS_LAYER_INBOUND_MAC_FRAME_ETHERNET                                           <br>
                FWPS_LAYER_OUTBOUND_MAC_FRAME_ETHERNET                                          <br>
                FWPS_LAYER_INBOUND_MAC_FRAME_NATIVE                                             <br>
                FWPS_LAYER_OUTBOUND_MAC_FRAME_NATIVE                                            <br>
                FWPS_LAYER_INGRESS_VSWITCH_ETHERNET                                             <br>
                FWPS_LAYER_EGRESS_VSWITCH_ETHERNET                                              <br>
                                                                                                <br>
             TCP @ FWPM_LAYER_ALE_AUTH_CONNECT_V{4/6} has no NBL                                <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF544893.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI ClassifyBasicPacketModification(_In_ const FWPS_INCOMING_VALUES0* pClassifyValues,
                                           _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                           _Inout_opt_ VOID* pNetBufferList,
                                           _In_opt_ const VOID* pClassifyContext,
                                           _In_ const FWPS_FILTER* pFilter,
                                           _In_ UINT64 flowContext,
                                           _Inout_ FWPS_CLASSIFY_OUT* pClassifyOut)
{
      NT_ASSERT(pClassifyValues);
      NT_ASSERT(pMetadata);
      NT_ASSERT(pFilter);
      NT_ASSERT(pClassifyOut);
      NT_ASSERT(pFilter->providerContext);
      NT_ASSERT(pFilter->providerContext->type == FWPM_GENERAL_CONTEXT);
      NT_ASSERT(pFilter->providerContext->dataBuffer);
      NT_ASSERT(pFilter->providerContext->dataBuffer->size == sizeof(PC_BASIC_PACKET_MODIFICATION_DATA));
   
#if(NTDDI_VERSION >= NTDDI_WIN8)
   
      NT_ASSERT(pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V4 ||
                pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V6 ||
                pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_IPPACKET_V4 ||
                pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_IPPACKET_V6 ||
                pClassifyValues->layerId == FWPS_LAYER_IPFORWARD_V4 ||
                pClassifyValues->layerId == FWPS_LAYER_IPFORWARD_V6 ||
                pClassifyValues->layerId == FWPS_LAYER_INBOUND_TRANSPORT_V4 ||
                pClassifyValues->layerId == FWPS_LAYER_INBOUND_TRANSPORT_V6 ||
                pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_TRANSPORT_V4 ||
                pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_TRANSPORT_V6 ||
                pClassifyValues->layerId == FWPS_LAYER_DATAGRAM_DATA_V4 ||
                pClassifyValues->layerId == FWPS_LAYER_DATAGRAM_DATA_V6 ||
                pClassifyValues->layerId == FWPS_LAYER_INBOUND_ICMP_ERROR_V4 ||
                pClassifyValues->layerId == FWPS_LAYER_INBOUND_ICMP_ERROR_V6 ||
                pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V4 ||
                pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V6 ||
                pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4 ||
                pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6 ||
                pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V4 ||
                pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V6 ||
                pClassifyValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4 ||
                pClassifyValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6 ||
                pClassifyValues->layerId == FWPS_LAYER_STREAM_PACKET_V4 ||
                pClassifyValues->layerId == FWPS_LAYER_STREAM_PACKET_V6 ||
                pClassifyValues->layerId == FWPS_LAYER_INBOUND_MAC_FRAME_ETHERNET ||
                pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_MAC_FRAME_ETHERNET ||
                pClassifyValues->layerId == FWPS_LAYER_INBOUND_MAC_FRAME_NATIVE ||
                pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_MAC_FRAME_NATIVE ||
                pClassifyValues->layerId == FWPS_LAYER_INGRESS_VSWITCH_ETHERNET ||
                pClassifyValues->layerId == FWPS_LAYER_EGRESS_VSWITCH_ETHERNET);
   
#else
   
      NT_ASSERT(pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V4 ||
                pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V6 ||
                pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_IPPACKET_V4 ||
                pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_IPPACKET_V6 ||
                pClassifyValues->layerId == FWPS_LAYER_IPFORWARD_V4 ||
                pClassifyValues->layerId == FWPS_LAYER_IPFORWARD_V6 ||
                pClassifyValues->layerId == FWPS_LAYER_INBOUND_TRANSPORT_V4 ||
                pClassifyValues->layerId == FWPS_LAYER_INBOUND_TRANSPORT_V6 ||
                pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_TRANSPORT_V4 ||
                pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_TRANSPORT_V6 ||
                pClassifyValues->layerId == FWPS_LAYER_DATAGRAM_DATA_V4 ||
                pClassifyValues->layerId == FWPS_LAYER_DATAGRAM_DATA_V6 ||
                pClassifyValues->layerId == FWPS_LAYER_INBOUND_ICMP_ERROR_V4 ||
                pClassifyValues->layerId == FWPS_LAYER_INBOUND_ICMP_ERROR_V6 ||
                pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V4 ||
                pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V6 ||
                pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4 ||
                pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6 ||
                pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V4 ||
                pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V6 ||
                pClassifyValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4 ||
                pClassifyValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6 ||
                pClassifyValues->layerId == FWPS_LAYER_STREAM_PACKET_V4 ||
                pClassifyValues->layerId == FWPS_LAYER_STREAM_PACKET_V6);
      
#endif /// (NTDDI_VERSION >= NTDDI_WIN8)
   
      NT_ASSERT(pFilter->providerContext);
      NT_ASSERT(pFilter->providerContext->type == FWPM_GENERAL_CONTEXT);
      NT_ASSERT(pFilter->providerContext->dataBuffer);
      NT_ASSERT(pFilter->providerContext->dataBuffer->size == sizeof(PC_BASIC_PACKET_MODIFICATION_DATA));

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> ClassifyBasicPacketModification() [Layer: %s][FilterID: %#I64x][Rights: %#x]",
              KrnlHlprFwpsLayerIDToString(pClassifyValues->layerId),
              pFilter->filterId,
              pClassifyOut->rights);

   if(pClassifyOut->rights & FWPS_RIGHT_ACTION_WRITE)
   {
      /// Packets are not available for TCP @ ALE_AUTH_CONNECT, so skip over as there is nothing to inject
      if(pNetBufferList)
      {
         PC_BASIC_PACKET_MODIFICATION_DATA* pData = (PC_BASIC_PACKET_MODIFICATION_DATA*)pFilter->providerContext->dataBuffer->data;

         if(pData)
         {
            NTSTATUS        status         = STATUS_SUCCESS;
            FWP_VALUE*      pFlags         = 0;
            INJECTION_DATA* pInjectionData = 0;

            pClassifyOut->actionType = FWP_ACTION_CONTINUE;

            pFlags = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                               &FWPM_CONDITION_FLAGS);
            if(pFlags &&
               pFlags->type == FWP_UINT32)
            {
               /// For IPsec interop, if  ALE classification is required, bypass the injection
               if(pFlags->uint32 & FWP_CONDITION_FLAG_IS_IPSEC_SECURED &&
                  FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                                 FWPS_METADATA_FIELD_ALE_CLASSIFY_REQUIRED))
                  HLPR_BAIL;

               /// Inject the individual fragments, but not the fragment grouping of those fragments
               if(pFlags->uint32 & FWP_CONDITION_FLAG_IS_FRAGMENT_GROUP)
                  HLPR_BAIL;
            }

            if(pData->flags & PCPMDF_MODIFY_TRANSPORT_HEADER &&
               (pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V4 ||
               pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V6 ||
               pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_IPPACKET_V4 ||
               pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_IPPACKET_V6 ||
               pClassifyValues->layerId == FWPS_LAYER_IPFORWARD_V4 ||
               pClassifyValues->layerId == FWPS_LAYER_IPFORWARD_V6))
            {
               UINT32  bytesRetreated = 0;
               UINT8   version        = 0;
               IPPROTO protocol       = IPPROTO_MAX;

               if(pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V4 ||
                  pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V6)
               {
                  if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                                    FWPS_METADATA_FIELD_IP_HEADER_SIZE))
                     bytesRetreated = pMetadata->ipHeaderSize;
               }

               if(bytesRetreated)
               {
                  /// Initial offset is at the Transport Header for INBOUND_IPPACKET, so retreat the size of the IP Header ...
                  status = NdisRetreatNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)pNetBufferList),
                                                         bytesRetreated,
                                                         0,
                                                         0);
                  if(status != STATUS_SUCCESS)
                  {
                     DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                                DPFLTR_ERROR_LEVEL,
                                " !!!! ClassifyBasicPacketModification: NdisRetreatNetBufferDataStart() [status: %#x]\n",
                                status);
                  
                     HLPR_BAIL;
                  }
               }

               version = KrnlHlprIPHeaderGetVersionField((NET_BUFFER_LIST*)pNetBufferList);

               protocol = KrnlHlprIPHeaderGetProtocolField((NET_BUFFER_LIST*)pNetBufferList,
                                                           version == IPV4 ? AF_INET : AF_INET6);

               if(bytesRetreated)
               {
                  /// ... and advance the offset back to the original position.
                  NdisAdvanceNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)pNetBufferList),
                                                bytesRetreated,
                                                FALSE,
                                                0);
               }

               /// Exit if this isn't the protocol we are looking for
               if(protocol != pData->originalTransportData.protocol)
                  HLPR_BAIL;
               else
               {
                  UINT16 sourcePort      = 0;
                  UINT16 destinationPort = 0;
                  UINT32 bytesAdvanced   = 0;

                  if(pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_IPPACKET_V4 ||
                     pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_IPPACKET_V6 ||
                     pClassifyValues->layerId == FWPS_LAYER_IPFORWARD_V4 ||
                     pClassifyValues->layerId == FWPS_LAYER_IPFORWARD_V6)
                  {
                     if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                                       FWPS_METADATA_FIELD_IP_HEADER_SIZE))
                        bytesAdvanced = pMetadata->ipHeaderSize;
                  }

                  if(bytesAdvanced)
                  {
                     /// Initial offset is at the IP Header for OUTBOUND_IPPACKET and IPFORWARD, so 
                     /// advance the size of the IP Header ...
                     NdisAdvanceNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)pNetBufferList),
                                                   bytesAdvanced,
                                                   FALSE,
                                                   0);
                  }

                  sourcePort      = KrnlHlprTransportHeaderGetSourcePortField((NET_BUFFER_LIST*)pNetBufferList,
                                                                              protocol);
                  destinationPort = KrnlHlprTransportHeaderGetDestinationPortField((NET_BUFFER_LIST*)pNetBufferList,
                                                                                   protocol);
                  /// Exit if the ports are not what we are looking for
                  if(pData->originalTransportData.sourcePort &&
                     sourcePort != pData->originalTransportData.sourcePort)
                      HLPR_BAIL;

                   if(pData->originalTransportData.destinationPort &&
                      destinationPort != pData->originalTransportData.destinationPort)
                      HLPR_BAIL;

                  if(bytesAdvanced)
                  {
                     /// ... and retreat the offset back to the original position.
                     status = NdisRetreatNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)pNetBufferList),
                                                            bytesAdvanced,
                                                            0,
                                                            0);
                     if(status != STATUS_SUCCESS)
                     {
                        DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                                   DPFLTR_ERROR_LEVEL,
                                   " !!!! ClassifyBasicPacketModification: NdisRetreatNetBufferDataStart() [status: %#x]\n",
                                   status);
                     
                        HLPR_BAIL;
                     }
                  }
               }
            }

#pragma warning(push)
#pragma warning(disable: 6014) /// pInjectionData will be freed in completionFn using BasicPacketModificationCompletionDataDestroy

            status = KrnlHlprInjectionDataCreate(&pInjectionData,
                                                 pClassifyValues,
                                                 pMetadata,
                                                 (NET_BUFFER_LIST*)pNetBufferList,
                                                 pFilter);
            HLPR_BAIL_ON_FAILURE(status);

#pragma warning(pop)

            if(pInjectionData->injectionState != FWPS_PACKET_INJECTED_BY_SELF &&
               pInjectionData->injectionState != FWPS_PACKET_PREVIOUSLY_INJECTED_BY_SELF)
            {
               BOOLEAN    performOutOfBand = TRUE;
               FWP_VALUE* pProtocolValue   = 0;

               pClassifyOut->actionType  = FWP_ACTION_BLOCK;
               pClassifyOut->flags      |= FWPS_CLASSIFY_OUT_FLAG_ABSORB;
               pClassifyOut->rights     ^= FWPS_RIGHT_ACTION_WRITE;

               if(pFlags &&
                  pFlags->type == FWP_UINT32 &&
                  pFlags->uint32 & FWP_CONDITION_FLAG_IS_IPSEC_SECURED)
                  pInjectionData->isIPsecSecured = TRUE;

               /// Override the default of performing Out of Band with the user's specified setting ...
               if(pData->performInline)
                  performOutOfBand = FALSE;

               /// ... however, due to TCP's locking semantics, TCP can only be injected Out of Band at any transport layer or equivalent, ...
               pProtocolValue = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                          &FWPM_CONDITION_IP_PROTOCOL);
               if((pProtocolValue &&
                  pProtocolValue->uint8 == IPPROTO_TCP &&
                  pClassifyValues->layerId > FWPS_LAYER_IPFORWARD_V6_DISCARD) ||
                  pClassifyValues->layerId == FWPS_LAYER_STREAM_PACKET_V4 ||
                  pClassifyValues->layerId == FWPS_LAYER_STREAM_PACKET_V6)
                  performOutOfBand = TRUE;

               /// ... and inbound injection of loopback traffic requires us to use Out of Band modification as well due to address lookups.
               if(!performOutOfBand &&
                  pInjectionData->direction == FWP_DIRECTION_INBOUND)
               {
                  FWP_VALUE* pRemoteAddress = 0;

                  pRemoteAddress = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                             &FWPM_CONDITION_IP_REMOTE_ADDRESS);
                  if(pRemoteAddress &&
                     ((pRemoteAddress->type == FWP_UINT32 &&
                     RtlCompareMemory(&(pRemoteAddress->uint32),
                                      IPV4_LOOPBACK_ADDRESS,
                                      IPV4_ADDRESS_SIZE)) ||
                     (pRemoteAddress->type == FWP_BYTE_ARRAY16_TYPE &&
                      RtlCompareMemory(&(pRemoteAddress->byteArray16->byteArray16),
                                    IPV6_LOOPBACK_ADDRESS,
                                    IPV6_ADDRESS_SIZE))))
                     performOutOfBand = TRUE;
               }

               if(performOutOfBand)
                  status = TriggerBasicPacketModificationOutOfBand(pClassifyValues,
                                                                   pMetadata,
                                                                   pNetBufferList,
                                                                   pClassifyContext,
                                                                   pFilter,
                                                                   flowContext,
                                                                   pClassifyOut,
                                                                   pInjectionData,
                                                                   pData);
               else
                  status = TriggerBasicPacketModificationInline(pClassifyValues,
                                                                pMetadata,
                                                                pNetBufferList,
                                                                pClassifyContext,
                                                                pFilter,
                                                                flowContext,
                                                                pClassifyOut,
                                                                &pInjectionData);
            }
            else
            {
               pClassifyOut->actionType = FWP_ACTION_PERMIT;

               KrnlHlprInjectionDataDestroy(&pInjectionData);

               DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                          DPFLTR_INFO_LEVEL,
                          "   -- Injection previously performed.\n");
            }

            HLPR_BAIL_LABEL:

            NT_ASSERT(status == STATUS_SUCCESS);

            if(status != STATUS_SUCCESS)
            {
               KrnlHlprInjectionDataDestroy(&pInjectionData);

               DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                          DPFLTR_ERROR_LEVEL,
                          " !!!! ClassifyBasicPacketModification() [status: %#x]\n",
                          status);
            }
         }
      }
      else
         pClassifyOut->actionType = FWP_ACTION_PERMIT;
   }

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- ClassifyBasicPacketModification() [Layer: %s][FilterID: %#I64x][Action: %#x][Rights: %#x][Absorb: %s]\n",
              KrnlHlprFwpsLayerIDToString(pClassifyValues->layerId),
              pFilter->filterId,
              pClassifyOut->actionType,
              pClassifyOut->rights,
              (pClassifyOut->flags & FWPS_CLASSIFY_OUT_FLAG_ABSORB) ? "TRUE" : "FALSE");

   return;
}

#else

/**
 @classify_function="ClassifyBasicPacketModification"
 
   Purpose:  Blocks the current NET_BUFFER_LIST, modifies a clone of the NBL with the specified 
             data and injects the clone back to the stack's data path.                          <br>
                                                                                                <br>
   Notes:    Applies to the following layers:                                                   <br>
                FWPS_LAYER_INBOUND_IPPACKET_V{4/6}                                              <br>
                FWPS_LAYER_OUTBOUND_IPPACKET_V{4/6}                                             <br>
                FWPS_LAYER_IPFORWARD_V{4/6}                                                     <br>
                FWPS_LAYER_INBOUND_TRANSPORT_V{4/6}                                             <br>
                FWPS_LAYER_OUTBOUND_TRANSPORT_V{4/6}                                            <br>
                FWPS_LAYER_DATAGRAM_DATA_V{4/6}                                                 <br>
                FWPS_LAYER_INBOUND_ICMP_ERROR_V{4/6}                                            <br>
                FWPS_LAYER_OUTBOUND_ICMP_ERROR_V{4/6}                                           <br>
                FWPS_LAYER_ALE_AUTH_CONNECT_V{4/6}                                              <br>
                FWPS_LAYER_ALE_FLOW_ESTABLISHED_V{4/6}                                          <br>
                                                                                                <br>
             TCP @ FWPM_LAYER_ALE_AUTH_CONNECT_V{4/6} has no NBL                                <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF544890.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI ClassifyBasicPacketModification(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                           _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                           _Inout_opt_ VOID* pNetBufferList,
                                           _In_ const FWPS_FILTER* pFilter,
                                           _In_ UINT64 flowContext,
                                           _Inout_ FWPS_CLASSIFY_OUT* pClassifyOut)
{
   NT_ASSERT(pClassifyValues);
   NT_ASSERT(pMetadata);
   NT_ASSERT(pFilter);
   NT_ASSERT(pClassifyOut);
   NT_ASSERT(pFilter->providerContext);
   NT_ASSERT(pFilter->providerContext->type == FWPM_GENERAL_CONTEXT);
   NT_ASSERT(pFilter->providerContext->dataBuffer);
   NT_ASSERT(pFilter->providerContext->dataBuffer->size == sizeof(PC_BASIC_PACKET_MODIFICATION_DATA));
   NT_ASSERT(pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_IPPACKET_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_IPPACKET_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_IPFORWARD_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_IPFORWARD_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_INBOUND_TRANSPORT_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_INBOUND_TRANSPORT_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_TRANSPORT_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_TRANSPORT_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_DATAGRAM_DATA_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_DATAGRAM_DATA_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_INBOUND_ICMP_ERROR_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_INBOUND_ICMP_ERROR_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6);
   NT_ASSERT(pFilter->providerContext);
   NT_ASSERT(pFilter->providerContext->type == FWPM_GENERAL_CONTEXT);
   NT_ASSERT(pFilter->providerContext->dataBuffer);
   NT_ASSERT(pFilter->providerContext->dataBuffer->size == sizeof(PC_BASIC_PACKET_MODIFICATION_DATA));

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> ClassifyBasicPacketModification() [Layer: %s][FilterID: %#I64x][Rights: %#x]",
              KrnlHlprFwpsLayerIDToString(pClassifyValues->layerId),
              pFilter->filterId,
              pClassifyOut->rights);


   if(pClassifyOut->rights & FWPS_RIGHT_ACTION_WRITE)
   {
      /// Packets are not available for TCP @ ALE_AUTH_CONNECT, so skip over as there is nothing to inject
      if(pNetBufferList)
      {
         NTSTATUS                           status         = STATUS_SUCCESS;
         FWP_VALUE*                         pFlags         = 0;
         PC_BASIC_PACKET_MODIFICATION_DATA* pData          = (PC_BASIC_PACKET_MODIFICATION_DATA*)pFilter->providerContext->dataBuffer->data;
         INJECTION_DATA*                    pInjectionData = 0;

         pClassifyOut->actionType = FWP_ACTION_CONTINUE;

         pFlags = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                            &FWPM_CONDITION_FLAGS);
         if(pFlags &&
            pFlags->type == FWP_UINT32)
         {
            /// For IPsec interop, if  ALE classification is required, bypass the injection
            if(pFlags->uint32 & FWP_CONDITION_FLAG_IS_IPSEC_SECURED &&

#if(NTDDI_VERSION >= NTDDI_WIN6SP1)

               FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                              FWPS_METADATA_FIELD_ALE_CLASSIFY_REQUIRED))
#else

               pFlags->uint32 & FWP_CONDITION_FLAG_REQUIRES_ALE_CLASSIFY)

#endif // (NTDDI_VERSION >= NTDDI_WIN6SP1)

            HLPR_BAIL;

            /// Inject the individual fragments, but not the fragment grouping of those fragments
            if(pFlags->uint32 & FWP_CONDITION_FLAG_IS_FRAGMENT_GROUP)
               HLPR_BAIL;
         }

         if(pData &&
            pData->flags & PCPMDF_MODIFY_TRANSPORT_HEADER &&
            (pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V4 ||
            pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V6 ||
            pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_IPPACKET_V4 ||
            pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V6 ||
            pClassifyValues->layerId == FWPS_LAYER_IPFORWARD_V4 ||
            pClassifyValues->layerId == FWPS_LAYER_IPFORWARD_V6))
         {
            UINT32  bytesRetreated = 0;
            UINT8   version        = 0;
            IPPROTO protocol       = IPPROTO_MAX;

            if(pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V4 ||
               pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V6)
            {
               if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                                 FWPS_METADATA_FIELD_IP_HEADER_SIZE))
                  bytesRetreated = pMetadata->ipHeaderSize;
            }

            if(bytesRetreated)
            {
               /// Initial offset is at the Transport Header for INBOUND_IPPACKET, so retreat the size of the IP Header ...
               status = NdisRetreatNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)pNetBufferList),
                                                      bytesRetreated,
                                                      0,
                                                      0);
               if(status != STATUS_SUCCESS)
               {
                  DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                             DPFLTR_ERROR_LEVEL,
                             " !!!! ClassifyBasicPacketModification: NdisRetreatNetBufferDataStart() [status: %#x]\n",
                             status);
               
                  HLPR_BAIL;
               }
            }

            version = KrnlHlprIPHeaderGetVersionField((NET_BUFFER_LIST*)pNetBufferList);

            protocol = KrnlHlprIPHeaderGetProtocolField((NET_BUFFER_LIST*)pNetBufferList,
                                                        version == IPV4 ? AF_INET : AF_INET6);

            if(bytesRetreated)
            {
               /// ... and advance the offset back to the original position.
               NdisAdvanceNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)pNetBufferList),
                                             bytesRetreated,
                                             FALSE,
                                             0);
            }

            /// Exit if this isn't the protocol we are looking for
            if(protocol != pData->originalTransportData.protocol)
               HLPR_BAIL;
            else
            {
               UINT16 sourcePort      = KrnlHlprTransportHeaderGetSourcePortField((NET_BUFFER_LIST*)pNetBufferList,
                                                                                  protocol);
               UINT16 destinationPort = KrnlHlprTransportHeaderGetDestinationPortField((NET_BUFFER_LIST*)pNetBufferList,
                                                                                       protocol);
               /// Exit if the ports are not what we are looking for
               if((pData->originalTransportData.sourcePort &&
                   sourcePort != pData->originalTransportData.sourcePort) ||
                   (pData->originalTransportData.destinationPort &&
                   destinationPort != pData->originalTransportData.destinationPort))
                   HLPR_BAIL;
            }
         }

#pragma warning(push)
#pragma warning(disable: 6014) /// pInjectionData will be freed in completionFn using BasicPacketModificationCompletionDataDestroy

         status = KrnlHlprInjectionDataCreate(&pInjectionData,
                                              pClassifyValues,
                                              pMetadata,
                                              (NET_BUFFER_LIST*)pNetBufferList,
                                              pFilter);
         HLPR_BAIL_ON_FAILURE(status);

#pragma warning(pop)
                                           
         if(pInjectionData->injectionState != FWPS_PACKET_INJECTED_BY_SELF &&
            pInjectionData->injectionState != FWPS_PACKET_PREVIOUSLY_INJECTED_BY_SELF)
         {
            BOOLEAN                            performOutOfBand = TRUE;
            FWP_VALUE*                         pProtocolValue   = 0;
            PC_BASIC_PACKET_MODIFICATION_DATA* pData            = (PC_BASIC_PACKET_MODIFICATION_DATA*)pFilter->providerContext->dataBuffer->data;

            pClassifyOut->actionType  = FWP_ACTION_BLOCK;
            pClassifyOut->flags      |= FWPS_CLASSIFY_OUT_FLAG_ABSORB;
            pClassifyOut->rights     ^= FWPS_RIGHT_ACTION_WRITE;

            if(pFlags &&
               pFlags->type == FWP_UINT32 &&
               pFlags->uint32 & FWP_CONDITION_FLAG_IS_IPSEC_SECURED)
               pInjectionData->isIPsecSecured = TRUE;

            /// Override the default of performing Out of Band with the user's specified setting ...
            if(pData->performInline)
               performOutOfBand = FALSE;

            /// Due to TCP's locking semantics, TCP can only be injected Out of Band at any transport layer or equivalent
            pProtocolValue = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                       &FWPM_CONDITION_IP_PROTOCOL);
            if(pProtocolValue &&
               pProtocolValue->uint8 == IPPROTO_TCP &&
               pClassifyValues->layerId > FWPS_LAYER_IPFORWARD_V6_DISCARD)
               performOutOfBand = TRUE;

            if(performOutOfBand)
               status = TriggerBasicPacketModificationOutOfBand(pClassifyValues,
                                                                pMetadata,
                                                                pNetBufferList,
                                                                0,
                                                                pFilter,
                                                                flowContext,
                                                                pClassifyOut,
                                                                pInjectionData,
                                                                pData);
            else
               status = TriggerBasicPacketModificationInline(pClassifyValues,
                                                             pMetadata,
                                                             pNetBufferList,
                                                             0,
                                                             pFilter,
                                                             flowContext,
                                                             pClassifyOut,
                                                             &pInjectionData);
         }
         else
         {
            pClassifyOut->actionType = FWP_ACTION_PERMIT;

            KrnlHlprInjectionDataDestroy(&pInjectionData);

            DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                       DPFLTR_INFO_LEVEL,
                       "   -- Injection previously performed.\n");
         }

         HLPR_BAIL_LABEL:

         NT_ASSERT(status == STATUS_SUCCESS);

         if(status != STATUS_SUCCESS)
         {
            DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                       DPFLTR_ERROR_LEVEL,
                       " !!!! ClassifyBasicPacketModification() [status: %#x]\n",
                       status);

            KrnlHlprInjectionDataDestroy(&pInjectionData);
         }
      }
      else
         pClassifyOut->actionType = FWP_ACTION_PERMIT;
   }

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- ClassifyBasicPacketModification() [Layer: %s][FilterID: %#I64x][Action: %#x][Rights: %#x][Absorb: %s]\n",
              KrnlHlprFwpsLayerIDToString(pClassifyValues->layerId),
              pFilter->filterId,
              pClassifyOut->actionType,
              pClassifyOut->rights,
              (pClassifyOut->flags & FWPS_CLASSIFY_OUT_FLAG_ABSORB) ? "TRUE" : "FALSE");

   return;
}

#endif // (NTDDI_VERSION >= NTDDI_WIN7)
