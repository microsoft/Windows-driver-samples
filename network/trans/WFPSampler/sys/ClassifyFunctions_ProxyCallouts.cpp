////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      ClassifyFunctions_ProxyCallouts.cpp
//
//   Abstract:
//      This module contains WFP Classify functions for proxying connections and sockets.
//
//   Naming Convention:
//
//      <Module><Scenario>
//  
//      i.e.
//
//       ClassifyProxyByALERedirect
//
//       <Module>
//          Classify               -       Function is an FWPS_CALLOUT_CLASSIFY_FN
//          Perform                -       Function executes the desired scenario.
//          Prv                    -       Function is a private helper to this module.
//          Trigger                -
//       <Scenario>
//          ProxyByALERedirect     -       Function demonstates use of 
//                                            FWPM_LAYER_ALE_CONNECT_REDIRECT_V{4/6} and
//                                            FWPM_LAYER_ALE_BIND_REDIRECT_V{4/6} for proxying.
//                                            (For use in Win7+)
//
//   Private Functions:
//      PerformProxyInjectionAtInboundNetwork(),
//      PerformProxyInjectionAtOutboundTransport(),
//      PerformProxySocketRedirection(),
//      PerformProxyConnectRedirection(),
//      ProxyByALERedirectWorkItemRoutine(),
//      ProxyUsingInjectionMethodWorkItemRoutine(),
//      TriggerProxyByALERedirectInline(),
//      TriggerProxyByALERedirectOutOfBand(),
//      TriggerProxyInjectionInline(),
//      TriggerProxyInjectionOutOfBand(),
//
//   Public Functions:
//      ClassifyProxyByALERedirect(),
//      ClassifyProxyByInjection(),
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Enhance function declaration for IntelliSense, fix 
//                                              proxying by injection to use INBOUND_IPPACKET, fix 
//                                              proxying by ALE to a local service, and add support 
//                                              for multiple redirectors.
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Framework_WFPSamplerCalloutDriver.h" /// .
#include "ClassifyFunctions_ProxyCallouts.tmh" /// $(OBJ_PATH)\$(O)\ 

/**
 @private_function="PerformProxyInjectionAtInboundNetwork"
 
   Purpose:                                                                                     <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS PerformProxyInjectionAtInboundNetwork(_In_ CLASSIFY_DATA** ppClassifyData,
                                               _In_ INJECTION_DATA** ppInjectionData,
                                               _In_ PC_PROXY_DATA* pProxyData,
                                               _In_ BOOLEAN isInline = FALSE)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> PerformProxyInjectionAtInboundNetwork()\n");

#endif /// DBG

   NT_ASSERT(ppClassifyData);
   NT_ASSERT(ppInjectionData);
   NT_ASSERT(pProxyData);
   NT_ASSERT(*ppClassifyData);
   NT_ASSERT(*ppInjectionData);

   NTSTATUS                         status              = STATUS_SUCCESS;
   FWPS_INCOMING_VALUES*            pClassifyValues     = (FWPS_INCOMING_VALUES*)(*ppClassifyData)->pClassifyValues;
   FWPS_INCOMING_METADATA_VALUES*   pMetadata           = (FWPS_INCOMING_METADATA_VALUES*)(*ppClassifyData)->pMetadataValues;
   COMPARTMENT_ID                   compartmentID       = DEFAULT_COMPARTMENT_ID;
   IF_INDEX                         interfaceIndex      = 0;
   IF_INDEX                         subInterfaceIndex   = 0;
   UINT32                           flags               = 0;
   NET_BUFFER_LIST*                 pNetBufferList      = 0;
   PROXY_COMPLETION_DATA*           pCompletionData     = 0;
   UINT32                           ipHeaderSize        = 0;
   UINT32                           bytesRetreated      = 0;
   UINT64                           endpointHandle      = 0;
   IPPROTO                          protocol            = IPPROTO_MAX;
   FWP_VALUE*                       pInterfaceIndex     = 0;
   FWP_VALUE*                       pSubInterfaceIndex  = 0;
   FWP_VALUE*                       pFlags              = 0;
   BYTE*                            pSourceAddress      = 0;
   BYTE*                            pDestinationAddress = 0;
   NDIS_TCP_IP_CHECKSUM_PACKET_INFO checksumInfo        = {0};

#if DBG

      KIRQL                          irql               = KeGetCurrentIrql();

#endif /// DBG

#pragma warning(push)
#pragma warning(disable: 6014) /// pCompletionData will be freed in completionFn using BasicPacketModificationCompletionDataDestroy

   HLPR_NEW(pCompletionData,
            PROXY_COMPLETION_DATA,
            WFPSAMPLER_CALLOUT_DRIVER_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pCompletionData,
                              status);

#pragma warning(pop)

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
                 " !!!! PerformProxyInjectionAtInboundNetwork : NdisRetreatNetBufferDataStart() [status: %#x]\n",
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
                 " !!!! PerformProxyInjectionAtInboundNetwork : FwpsAllocateCloneNetBufferList() [status: %#x]\n",
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
                          " !!!! PerformProxyInjectionAtInboundNetwork: KrnlHlprIPHeaderModifyLoopbackToLocal() [status: %#x]\n",
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

   protocol = KrnlHlprIPHeaderGetProtocolField(pNetBufferList,
                                               pCompletionData->pInjectionData->addressFamily);

   if(pProxyData->flags & PCPDF_PROXY_LOCAL_PORT ||
      pProxyData->flags & PCPDF_PROXY_REMOTE_PORT)
   {
      NTSTATUS tmpStatus = STATUS_SUCCESS;

      protocol = KrnlHlprIPHeaderGetProtocolField(pNetBufferList,
                                                  pCompletionData->pInjectionData->addressFamily);

      /// The clone is at the IP Header, so advance by the size of the IP Header.
      NdisAdvanceNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB(pNetBufferList),
                                    ipHeaderSize,
                                    FALSE,
                                    0);

      if(pProxyData->flags & PCPDF_PROXY_LOCAL_PORT)
      {
         FWP_VALUE localPort;

         RtlZeroMemory(&localPort,
                       sizeof(FWP_VALUE));

         localPort.type   = FWP_UINT16;
         localPort.uint16 = pProxyData->originalLocalPort;

         switch(protocol)
         {
            case TCP:
            {
               UINT32 tcpHeaderSize = TCP_HEADER_MIN_SIZE;

               if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                                 FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE))
                  tcpHeaderSize = pMetadata->transportHeaderSize;

               status = KrnlHlprTCPHeaderModifyDestinationPort(&localPort,
                                                               pNetBufferList,
                                                               tcpHeaderSize);
               HLPR_BAIL_ON_FAILURE_2(status);

               break;
            }
            case UDP:
            {
               UINT32 udpHeaderSize = UDP_HEADER_MIN_SIZE;

               if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                                 FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE))
                  udpHeaderSize = pMetadata->transportHeaderSize;

               status = KrnlHlprUDPHeaderModifyDestinationPort(&localPort,
                                                               pNetBufferList,
                                                               udpHeaderSize);
               HLPR_BAIL_ON_FAILURE_2(status);

               break;
            }
         }
      }

      if(pProxyData->flags & PCPDF_PROXY_REMOTE_PORT)
      {
         FWP_VALUE remotePort;

         RtlZeroMemory(&remotePort,
                       sizeof(FWP_VALUE));

         remotePort.type   = FWP_UINT16;
         remotePort.uint16 = pProxyData->originalRemotePort;

         switch(protocol)
         {
            case TCP:
            {
               UINT32 tcpHeaderSize = TCP_HEADER_MIN_SIZE;

               if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                                 FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE))
                  tcpHeaderSize = pMetadata->transportHeaderSize;

               status = KrnlHlprTCPHeaderModifySourcePort(&remotePort,
                                                          pNetBufferList,
                                                          tcpHeaderSize);
               HLPR_BAIL_ON_FAILURE_2(status);

               break;
            }
            case UDP:
            {
               UINT32 udpHeaderSize = UDP_HEADER_MIN_SIZE;

               if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                                 FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE))
                  udpHeaderSize = pMetadata->transportHeaderSize;

               status = KrnlHlprUDPHeaderModifySourcePort(&remotePort,
                                                          pNetBufferList,
                                                          udpHeaderSize);
               HLPR_BAIL_ON_FAILURE_2(status);

               break;
            }
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
                    " !!!! PerformProxyInjectionAtInboundNetwork : NdisRetreatNetBufferDataStart() [status: %#x]\n",
                    status);

         HLPR_BAIL;
      }

      HLPR_BAIL_ON_FAILURE(status);
   }

   if(pProxyData->flags & PCPDF_PROXY_LOCAL_ADDRESS)
   {
      FWP_VALUE localAddress;

      RtlZeroMemory(&localAddress,
                    sizeof(FWP_VALUE));

      if(pCompletionData->pInjectionData->addressFamily == AF_INET)
      {
         localAddress.type = FWP_UINT32;

         RtlCopyMemory(&(localAddress.uint32),
                       pProxyData->originalLocalAddress.pIPv4,
                       IPV4_ADDRESS_SIZE);
      }
      else
      {
         HLPR_NEW(localAddress.byteArray16,
                  FWP_BYTE_ARRAY16,
                  WFPSAMPLER_CALLOUT_DRIVER_TAG);
         HLPR_BAIL_ON_ALLOC_FAILURE(localAddress.byteArray16,
                                    status);

         localAddress.type = FWP_BYTE_ARRAY16_TYPE;

         RtlCopyMemory(localAddress.byteArray16->byteArray16,
                       pProxyData->originalLocalAddress.pIPv6,
                       IPV6_ADDRESS_SIZE);
      }

      status = KrnlHlprIPHeaderModifyDestinationAddress(&localAddress,
                                                        pNetBufferList,
                                                        TRUE);

      KrnlHlprFwpValuePurgeLocalCopy(&localAddress);

      HLPR_BAIL_ON_FAILURE(status);
   }

   if(pProxyData->flags & PCPDF_PROXY_REMOTE_ADDRESS)
   {
      FWP_VALUE remoteAddress;

      RtlZeroMemory(&remoteAddress,
                    sizeof(FWP_VALUE));

      if(pCompletionData->pInjectionData->addressFamily == AF_INET)
      {
         remoteAddress.type = FWP_UINT32;

         RtlCopyMemory(&(remoteAddress.uint32),
                       pProxyData->originalRemoteAddress.pIPv4,
                       IPV4_ADDRESS_SIZE);
      }
      else
      {
         HLPR_NEW(remoteAddress.byteArray16,
                  FWP_BYTE_ARRAY16,
                  WFPSAMPLER_CALLOUT_DRIVER_TAG);
         HLPR_BAIL_ON_ALLOC_FAILURE(remoteAddress.byteArray16,
                                    status);

         remoteAddress.type = FWP_BYTE_ARRAY16_TYPE;

         RtlCopyMemory(remoteAddress.byteArray16->byteArray16,
                       pProxyData->originalRemoteAddress.pIPv6,
                       IPV6_ADDRESS_SIZE);
      }

      status = KrnlHlprIPHeaderModifySourceAddress(&remoteAddress,
                                                   pNetBufferList,
                                                   TRUE);

      KrnlHlprFwpValuePurgeLocalCopy(&remoteAddress);

      HLPR_BAIL_ON_FAILURE(status);
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
                 " !!!! PerformProxyInjectionAtInboundNetwork : NdisRetreatNetBufferDataStart() [status: %#x]\n",
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
                 " !!!! PerformProxyInjectionAtInboundNetwork : FwpsInjectNetworkReceiveAsync() [status: %#x]\n",
                 status);

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
   {
      if(pNetBufferList)
      {
         FwpsFreeCloneNetBufferList(pNetBufferList,
                                    0);

         pNetBufferList = 0;
      }

      if(pCompletionData)
         ProxyCompletionDataDestroy(&pCompletionData);
   }

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- PerformProxyInjectionAtInboundNetwork() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

/**
 @private_function="PerformProxyInjectionAtOutboundTransport"
 
   Purpose:                                                                                     <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS PerformProxyInjectionAtOutboundTransport(_In_ CLASSIFY_DATA** ppClassifyData,
                                                  _In_ INJECTION_DATA** ppInjectionData,
                                                  _In_ PC_PROXY_DATA* pProxyData,
                                                  _In_ BOOLEAN isInline = FALSE)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> PerformProxyInjectionAtOutboundTransport()\n");

#endif /// DBG
   
   NT_ASSERT(ppClassifyData);
   NT_ASSERT(ppInjectionData);
   NT_ASSERT(pProxyData);
   NT_ASSERT(*ppClassifyData);
   NT_ASSERT(*ppInjectionData);

   NTSTATUS                       status           = STATUS_SUCCESS;
   FWPS_INCOMING_VALUES*          pClassifyValues  = (FWPS_INCOMING_VALUES*)(*ppClassifyData)->pClassifyValues;
   FWPS_INCOMING_METADATA_VALUES* pMetadata        = (FWPS_INCOMING_METADATA_VALUES*)(*ppClassifyData)->pMetadataValues;
   UINT64                         endpointHandle   = 0;
   FWPS_TRANSPORT_SEND_PARAMS*    pSendParams      = 0;
   COMPARTMENT_ID                 compartmentID    = DEFAULT_COMPARTMENT_ID;
   NET_BUFFER_LIST*               pNetBufferList   = 0;
   PROXY_COMPLETION_DATA*         pCompletionData  = 0;
   IPPROTO                        protocol         = IPPROTO_MAX;
   BYTE*                          pRemoteAddress   = 0;
   FWP_VALUE*                     pAddressValue    = 0;
   FWP_VALUE*                     pProtocolValue   = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                                               &FWPM_CONDITION_IP_PROTOCOL);

   HLPR_BAIL_ON_NULL_POINTER_WITH_STATUS(pProtocolValue,
                                         status);

#pragma warning(push)
#pragma warning(disable: 6014) /// pCompletionData & pSendParams will be freed in completionFn using BasicPacketInjectionCompletionDataDestroy

   HLPR_NEW(pCompletionData,
            PROXY_COMPLETION_DATA,
            WFPSAMPLER_CALLOUT_DRIVER_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pCompletionData,
                              status);

   HLPR_NEW(pSendParams,
            FWPS_TRANSPORT_SEND_PARAMS,
            WFPSAMPLER_CALLOUT_DRIVER_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pSendParams,
                              status);

#pragma warning(pop)

   pCompletionData->performedInline = isInline;
   pCompletionData->pClassifyData   = *ppClassifyData;
   pCompletionData->pInjectionData  = *ppInjectionData;
   pCompletionData->pProxyData      = pProxyData;
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

   protocol = (IPPROTO)pProtocolValue->uint8;

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
                 " !!!! PerformProxyInjectionAtOutboundTransport : FwpsAllocateCloneNetBufferList() [status: %#x]\n",
                 status);

      HLPR_BAIL;
   }

   if(pProxyData->flags & PCPDF_PROXY_LOCAL_PORT)
   {
      FWP_VALUE localPort;

      RtlZeroMemory(&localPort,
                    sizeof(FWP_VALUE));

      localPort.type   = FWP_UINT16;
      localPort.uint16 = pProxyData->proxyLocalPort;

      switch(protocol)
      {
         case IPPROTO_TCP:
         {
            UINT32 tcpHeaderSize = TCP_HEADER_MIN_SIZE;

            if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                              FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE))
               tcpHeaderSize = pMetadata->transportHeaderSize;

            status = KrnlHlprTCPHeaderModifySourcePort(&localPort,
                                                       pNetBufferList,
                                                       tcpHeaderSize);

            break;
         }
         case IPPROTO_UDP:
         {
            UINT32 udpHeaderSize = UDP_HEADER_MIN_SIZE;

            if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                              FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE))
               udpHeaderSize = pMetadata->transportHeaderSize;

            status = KrnlHlprUDPHeaderModifySourcePort(&localPort,
                                                       pNetBufferList,
                                                       udpHeaderSize);

            break;
         }
      }

      HLPR_BAIL_ON_FAILURE(status);
   }

   if(pProxyData->flags & PCPDF_PROXY_REMOTE_PORT)
   {
      FWP_VALUE remotePort;
   
      RtlZeroMemory(&remotePort,
                    sizeof(FWP_VALUE));
   
      remotePort.type   = FWP_UINT16;
      remotePort.uint16 = pProxyData->proxyRemotePort;
   
      switch(protocol)
      {
         case IPPROTO_TCP:
         {
            UINT32 tcpHeaderSize = TCP_HEADER_MIN_SIZE;

            if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                              FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE))
               tcpHeaderSize = pMetadata->transportHeaderSize;

            status = KrnlHlprTCPHeaderModifyDestinationPort(&remotePort,
                                                            pNetBufferList,
                                                            tcpHeaderSize);
   
            break;
         }
         case IPPROTO_UDP:
         {
            UINT32 udpHeaderSize = UDP_HEADER_MIN_SIZE;

            if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                              FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE))
               udpHeaderSize = pMetadata->transportHeaderSize;

            status = KrnlHlprUDPHeaderModifyDestinationPort(&remotePort,
                                                            pNetBufferList,
                                                            udpHeaderSize);
   
            break;
         }
      }

      HLPR_BAIL_ON_FAILURE(status);
   }

   /// As there is no IP Header yet, we can only modify the destination IP address via the FWPS_TRANSPORT_SEND_PARAMS
   if(pProxyData->flags & PCPDF_PROXY_REMOTE_ADDRESS)
   {
      if(pCompletionData->pInjectionData->addressFamily == AF_INET)
         RtlCopyMemory(pCompletionData->pSendParams->remoteAddress,
                       pProxyData->proxyRemoteAddress.pIPv4,
                       IPV4_ADDRESS_SIZE);
      else
      {
         RtlCopyMemory(pCompletionData->pSendParams->remoteAddress,
                       pProxyData->proxyRemoteAddress.pIPv6,
                       IPV6_ADDRESS_SIZE);

         pCompletionData->pSendParams->remoteScopeId.Value = pProxyData->remoteScopeId;
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
                                         CompleteProxyInjection,
                                         pCompletionData);
   if(status != STATUS_SUCCESS)
   {
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! PerformProxyInjectionAtOutboundTransport : FwpsInjectTransportSendAsync() [status: %#x]\n",
                 status);

      HLPR_BAIL;
   }

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
   {
      if(pNetBufferList)
      {
         FwpsFreeCloneNetBufferList(pNetBufferList,
                                    0);

         pNetBufferList = 0;
      }

      if(pCompletionData)
         ProxyCompletionDataDestroy(&pCompletionData);
   }

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- PerformProxyInjectionAtOutboundTransport() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}

/**
 @private_function="ProxyUsingInjectionMethodDeferredProcedureCall"
 
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
VOID ProxyUsingInjectionMethodDeferredProcedureCall(_In_ KDPC* pDPC,
                                                    _In_opt_ PVOID pContext,
                                                    _In_opt_ PVOID pArg1,
                                                    _In_opt_ PVOID pArg2)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> BasicPacketInjectionDeferredProcedureCall()\n");

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
      NTSTATUS              status          = STATUS_SUCCESS;
      FWPS_INCOMING_VALUES* pClassifyValues = 0;

      pClassifyValues = (FWPS_INCOMING_VALUES*)pDPCData->pClassifyData->pClassifyValues;

      if(pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_TRANSPORT_V4 ||
         pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_TRANSPORT_V6)
         status = PerformProxyInjectionAtOutboundTransport(&(pDPCData->pClassifyData),
                                                           &(pDPCData->pInjectionData),
                                                           (PC_PROXY_DATA*)pDPCData->pContext);
      else if(pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V6)
         status = PerformProxyInjectionAtInboundNetwork(&(pDPCData->pClassifyData),
                                                        &(pDPCData->pInjectionData),
                                                        (PC_PROXY_DATA*)pDPCData->pContext);
      else
         status = STATUS_FWP_INCOMPATIBLE_LAYER;


      if(status != STATUS_SUCCESS)
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_ERROR_LEVEL,
                    " !!!! ProxyUsingInjectionMethodDeferredProcedureCall() [status: %#x]\n",
                    status);

      KrnlHlprDPCDataDestroy(&pDPCData);
   }

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- ProxyUsingInjectionMethodDeferredProcedureCall()\n");

#endif /// DBG
   
   return;
}

/**
 @private_function="ProxyUsingInjectionMethodWorkItemRoutine"
 
   Purpose:                                                                                     <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Function_class_(IO_WORKITEM_ROUTINE)
VOID ProxyUsingInjectionMethodWorkItemRoutine(_In_ PDEVICE_OBJECT pDeviceObject,
                                              _In_opt_ PVOID pContext)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> ProxyUsingInjectionMethodWorkItemRoutine()\n");

#endif /// DBG
   
   UNREFERENCED_PARAMETER(pDeviceObject);

   NT_ASSERT(pContext);
   NT_ASSERT(((WORKITEM_DATA*)pContext)->pClassifyData);
   NT_ASSERT(((WORKITEM_DATA*)pContext)->pInjectionData);

   WORKITEM_DATA* pWorkItemData = (WORKITEM_DATA*)pContext;

   if(pWorkItemData)
   {
      NTSTATUS              status          = STATUS_SUCCESS;
      FWPS_INCOMING_VALUES* pClassifyValues = 0;

      pClassifyValues = (FWPS_INCOMING_VALUES*)pWorkItemData->pClassifyData->pClassifyValues;

      if(pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_TRANSPORT_V4 ||
         pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_TRANSPORT_V6)
         status = PerformProxyInjectionAtOutboundTransport(&(pWorkItemData->pClassifyData),
                                                           &(pWorkItemData->pInjectionData),
                                                           (PC_PROXY_DATA*)pWorkItemData->pInjectionData->pContext);
      else if(pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V6)
         status = PerformProxyInjectionAtInboundNetwork(&(pWorkItemData->pClassifyData),
                                                        &(pWorkItemData->pInjectionData),
                                                        (PC_PROXY_DATA*)pWorkItemData->pContext);
      else
         status = STATUS_FWP_INCOMPATIBLE_LAYER;


      if(status != STATUS_SUCCESS)
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_ERROR_LEVEL,
                    " !!!! ProxyUsingInjectionMethodWorkItemRoutine : PerformProxyInjectionAt*() [status: %#x]\n",
                    status);

      KrnlHlprWorkItemDataDestroy(&pWorkItemData);
   }

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- ProxyUsingInjectionMethodWorkItemRoutine()\n");

#endif /// DBG
   
   return;
}

/**
 @private_function="TriggerProxyInjectionInline"
 
   Purpose:                                                                                     <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return == STATUS_SUCCESS)
NTSTATUS TriggerProxyInjectionInline(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                     _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                     _Inout_ VOID* pNetBufferList,
                                     _In_opt_ const VOID* pClassifyContext,
                                     _In_ const FWPS_FILTER* pFilter,
                                     _In_ UINT64 flowContext,
                                     _Inout_ FWPS_CLASSIFY_OUT* pClassifyOut,
                                     _In_ INJECTION_DATA** ppInjectionData,
                                     _In_ PC_PROXY_DATA* pProxyData)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> TriggerProxyInjectionInline()\n");

#endif /// DBG
   
   NT_ASSERT(pClassifyValues);
   NT_ASSERT(pMetadata);
   NT_ASSERT(pNetBufferList);
   NT_ASSERT(pFilter);
   NT_ASSERT(pClassifyOut);
   NT_ASSERT(ppInjectionData);
   NT_ASSERT(*ppInjectionData);
   NT_ASSERT(pProxyData);
   NT_ASSERT(pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_TRANSPORT_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_TRANSPORT_V6);

   NTSTATUS       status        = STATUS_SUCCESS;
   CLASSIFY_DATA* pClassifyData = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pClassifyData will be freed in completionFn using ProxyCompletionDataDestroy

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

   if(pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_TRANSPORT_V4 ||
      pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_TRANSPORT_V6)
      status = PerformProxyInjectionAtOutboundTransport(&pClassifyData,
                                                        ppInjectionData,
                                                        pProxyData,
                                                        TRUE);
   else if(pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V4 ||
           pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V6)
      status = PerformProxyInjectionAtInboundNetwork(&pClassifyData,
                                                     ppInjectionData,
                                                     pProxyData,
                                                     TRUE);
   else
      status = STATUS_FWP_INCOMPATIBLE_LAYER;

   HLPR_BAIL_LABEL:

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- TriggerProxyInjectionInline() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}

/**
 @private_function="TriggerProxyInjectionOutOfBand"
 
   Purpose:                                                                                     <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return == STATUS_SUCCESS)
NTSTATUS TriggerProxyInjectionOutOfBand(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                        _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                        _Inout_ VOID* pNetBufferList,
                                        _In_opt_ const VOID* pClassifyContext,
                                        _In_ const FWPS_FILTER* pFilter,
                                        _In_ UINT64 flowContext,
                                        _In_ FWPS_CLASSIFY_OUT* pClassifyOut,
                                        _In_ INJECTION_DATA* pInjectionData,
                                        _In_ PC_PROXY_DATA* pPCData)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> TriggerProxyInjectionOutOfBand()\n");

#endif /// DBG
   
   NT_ASSERT(pClassifyValues);
   NT_ASSERT(pMetadata);
   NT_ASSERT(pNetBufferList);
   NT_ASSERT(pFilter);
   NT_ASSERT(pClassifyOut);
   NT_ASSERT(pInjectionData);
   NT_ASSERT(pPCData);
   NT_ASSERT(pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_TRANSPORT_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_TRANSPORT_V6);

   NTSTATUS       status        = STATUS_SUCCESS;
   CLASSIFY_DATA* pClassifyData = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pClassifyData will be freed in completionFn using ProxyInjectionCompletionDataDestroy

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
                                     ProxyUsingInjectionMethodWorkItemRoutine,
                                     pClassifyData,
                                     pInjectionData,
                                     (VOID*)pPCData);
   else if(pPCData->useThreadedDPC)
      status = KrnlHlprThreadedDPCQueue(ProxyUsingInjectionMethodDeferredProcedureCall,
                                        pClassifyData,
                                        pInjectionData,
                                        (VOID*)pPCData);
   else
      status = KrnlHlprDPCQueue(ProxyUsingInjectionMethodDeferredProcedureCall,
                                pClassifyData,
                                pInjectionData,
                                (VOID*)pPCData);

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
   {
      if(pClassifyData)
         KrnlHlprClassifyDataDestroyLocalCopy(&pClassifyData);
   }

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- TriggerProxyInjectionOutOfBand() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}

#if(NTDDI_VERSION >= NTDDI_WIN7)

#pragma warning(push)
#pragma warning(disable: 6101) /// *ppClassifyData and *ppRedirectData are freed and set to 0 on successful completion

/**
 @private_function="PerformProxySocketRedirection"
 
   Purpose:                                                                                     <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_At_(*ppRedirectData, _Pre_ _Notnull_)
///_At_(*ppRedirectData, _Post_ _Null_)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS PerformProxySocketRedirection(_In_ CLASSIFY_DATA** ppClassifyData,
                                       _Inout_ REDIRECT_DATA** ppRedirectData)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> PerformProxySocketRedirection()\n");

#endif /// DBG
   
   NT_ASSERT(ppClassifyData);
   NT_ASSERT(ppRedirectData);
   NT_ASSERT((*ppRedirectData)->pWritableLayerData);
   NT_ASSERT((*ppRedirectData)->pProxyData);

   NTSTATUS              status          = STATUS_SUCCESS;
   FWPS_BIND_REQUEST*    pBindRequest    = (FWPS_BIND_REQUEST*)(*ppRedirectData)->pWritableLayerData;
   FWPS_INCOMING_VALUES* pClassifyValues = (FWPS_INCOMING_VALUES*)(*ppClassifyData)->pClassifyValues;
   FWP_VALUE*            pProtocolValue  = 0;
   UINT8                 ipProtocol      = 0;
   
   pProtocolValue = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                              &FWPM_CONDITION_IP_PROTOCOL);
   if(pProtocolValue)
      ipProtocol = pProtocolValue->uint8;

   if((*ppRedirectData)->pProxyData->flags & PCPDF_PROXY_LOCAL_ADDRESS)
      INETADDR_SET_ADDRESS((PSOCKADDR)&(pBindRequest->localAddressAndPort),
                           (*ppRedirectData)->pProxyData->proxyLocalAddress.pBytes);

   if((*ppRedirectData)->pProxyData->flags & PCPDF_PROXY_LOCAL_PORT)
      INETADDR_SET_PORT((PSOCKADDR)&(pBindRequest->localAddressAndPort),
                        (*ppRedirectData)->pProxyData->proxyLocalPort);

   if(ipProtocol == IPPROTO_TCP)
      pBindRequest->portReservationToken = (*ppRedirectData)->pProxyData->tcpPortReservationToken;
   else if(ipProtocol == IPPROTO_UDP)
      pBindRequest->portReservationToken = (*ppRedirectData)->pProxyData->udpPortReservationToken;

   (*ppRedirectData)->pClassifyOut->actionType  = FWP_ACTION_PERMIT;
   (*ppRedirectData)->pClassifyOut->rights     ^= FWPS_RIGHT_ACTION_WRITE;

#pragma warning(push)
#pragma warning(disable: 6001) /// *ppRedirectData has already been initialized in previous call to KrnlHlprRedirectDataCreate

   /// This will apply the modified data and cleanup the classify handle
   KrnlHlprRedirectDataDestroy(ppRedirectData);

#pragma warning(pop)

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- PerformProxySocketRedirection() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}

/**
 @private_function="PerformProxyConnectRedirection"
 
   Purpose:                                                                                     <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_At_(*ppRedirectData, _Pre_ _Notnull_)
///_At_(*ppRedirectData, _Post_ _Null_)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS PerformProxyConnectRedirection(_In_ CLASSIFY_DATA** ppClassifyData,
                                        _Inout_ REDIRECT_DATA** ppRedirectData)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> PerformProxyConnectRedirection()\n");

#endif /// DBG
   
   NT_ASSERT(ppClassifyData);
   NT_ASSERT(ppRedirectData);
   NT_ASSERT(*ppClassifyData);
   NT_ASSERT(*ppRedirectData);

   NTSTATUS              status           = STATUS_SUCCESS;
   FWPS_CONNECT_REQUEST* pConnectRequest  = (FWPS_CONNECT_REQUEST*)(*ppRedirectData)->pWritableLayerData;
   UINT32                actionType       = FWP_ACTION_PERMIT;
   FWPS_INCOMING_VALUES* pClassifyValues  = (FWPS_INCOMING_VALUES*)(*ppClassifyData)->pClassifyValues;
   FWP_VALUE*            pProtocolValue   = 0;
   UINT8                 ipProtocol       = 0;

#if(NTDDI_VERSION >= NTDDI_WIN8)

   SOCKADDR_STORAGE*     pSockAddrStorage = 0;

   /// Set redirectHandle only if proxying locally
   if((*ppRedirectData)->redirectHandle &&
      !((*ppRedirectData)->pProxyData->proxyToRemoteService))
      pConnectRequest->localRedirectHandle = (*ppRedirectData)->redirectHandle;

   HLPR_NEW_ARRAY(pSockAddrStorage,
                  SOCKADDR_STORAGE,
                  2,
                  WFPSAMPLER_CALLOUT_DRIVER_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pSockAddrStorage,
                              status);

   /// Pass original remote destination values to query them in user mode
   RtlCopyMemory(&(pSockAddrStorage[0]),
                 &(pConnectRequest->remoteAddressAndPort),
                 sizeof(SOCKADDR_STORAGE));

   RtlCopyMemory(&(pSockAddrStorage[1]),
                 &(pConnectRequest->localAddressAndPort),
                 sizeof(SOCKADDR_STORAGE));

   /// WFP will take ownership of this memory and free it when the flow / redirection terminates
   pConnectRequest->localRedirectContext     = pSockAddrStorage;
   pConnectRequest->localRedirectContextSize = sizeof(SOCKADDR_STORAGE) * 2;

#endif /// (NTDDI_VERSION >= NTDDI_WIN8)

   pProtocolValue = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                              &FWPM_CONDITION_IP_PROTOCOL);
   if(pProtocolValue)
      ipProtocol = pProtocolValue->uint8;

   /// For non-TCP, this setting will not be enforced being that local redirection of this tuple is only 
   /// available during bind time. and ideally redirection should be performed using ALE_BIND_REDIRECT instead.
   if((*ppRedirectData)->pProxyData->flags & PCPDF_PROXY_LOCAL_ADDRESS)
      INETADDR_SET_ADDRESS((PSOCKADDR)&(pConnectRequest->localAddressAndPort),
                           (*ppRedirectData)->pProxyData->proxyLocalAddress.pBytes);

   /// For non-TCP, this setting will not be enforced being that local redirection of this tuple is only 
   /// available during bind time. and ideally redirection should be performed using ALE_BIND_REDIRECT instead.
   if((*ppRedirectData)->pProxyData->flags & PCPDF_PROXY_LOCAL_PORT)
      INETADDR_SET_PORT((PSOCKADDR)&(pConnectRequest->localAddressAndPort),
                        (*ppRedirectData)->pProxyData->proxyLocalPort);

   if((*ppRedirectData)->pProxyData->flags & PCPDF_PROXY_REMOTE_ADDRESS)
   {
      if((*ppRedirectData)->pProxyData->proxyToRemoteService)
         INETADDR_SET_ADDRESS((PSOCKADDR)&(pConnectRequest->remoteAddressAndPort),
                              (*ppRedirectData)->pProxyData->proxyRemoteAddress.pBytes);
      else
      {
         /// Ensure we don't need to worry about crossing any of the TCP/IP stack's zones
         if(INETADDR_ISANY((PSOCKADDR)&(pConnectRequest->localAddressAndPort)))
            INETADDR_SETLOOPBACK((PSOCKADDR)&(pConnectRequest->remoteAddressAndPort));
         else
            INETADDR_SET_ADDRESS((PSOCKADDR)&(pConnectRequest->remoteAddressAndPort),
                                 INETADDR_ADDRESS((PSOCKADDR)&(pConnectRequest->localAddressAndPort)));
      }
   }

   if((*ppRedirectData)->pProxyData->flags & PCPDF_PROXY_REMOTE_PORT)
      INETADDR_SET_PORT((PSOCKADDR)&(pConnectRequest->remoteAddressAndPort),
                        (*ppRedirectData)->pProxyData->proxyRemotePort);

   if(ipProtocol == IPPROTO_TCP)
      pConnectRequest->portReservationToken = (*ppRedirectData)->pProxyData->tcpPortReservationToken;
   else if(ipProtocol == IPPROTO_UDP)
      pConnectRequest->portReservationToken = (*ppRedirectData)->pProxyData->udpPortReservationToken;

   if((*ppRedirectData)->pProxyData->targetProcessID)
      pConnectRequest->localRedirectTargetPID = (*ppRedirectData)->pProxyData->targetProcessID;

#if(NTDDI_VERSION >= NTDDI_WIN8)

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
   {
      actionType = FWP_ACTION_BLOCK;

      HLPR_DELETE_ARRAY(pSockAddrStorage,
                        WFPSAMPLER_CALLOUT_DRIVER_TAG);
   }

#endif /// (NTDDI_VERSION >= NTDDI_WIN8)

#pragma warning(push)
#pragma warning(disable: 6001) /// *ppRedirectData has already been initialized in previous call to KrnlHlprRedirectDataCreate

   (*ppRedirectData)->pClassifyOut->actionType  = actionType;
   (*ppRedirectData)->pClassifyOut->rights     ^= FWPS_RIGHT_ACTION_WRITE;


   /// This will apply the modified data and cleanup the classify handle
   KrnlHlprRedirectDataDestroy(ppRedirectData);

#pragma warning(pop)

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- PerformProxyConnectRedirection() [status:%#x]\n",
              status);

#endif /// DBG
   
   return status;
}

#pragma warning(pop)

/**
 @private_function="ProxyByALERedirectDeferredProcedureCall"
 
   Purpose:  Invokes the appropriate private redirection routine to at DISPATCH_LEVEL.          <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF542972.aspx             <br>
*/
_IRQL_requires_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Function_class_(KDEFERRED_ROUTINE)
VOID ProxyByALERedirectDeferredProcedureCall(_In_ KDPC* pDPC,
                                             _In_opt_ PVOID pContext,
                                             _In_opt_ PVOID pArg1,
                                             _In_opt_ PVOID pArg2)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> ProxyByALERedirectDeferredProcedureCall()\n");

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
      NTSTATUS              status          = STATUS_SUCCESS;
      FWPS_INCOMING_VALUES* pClassifyValues = (FWPS_INCOMING_VALUES*)pDPCData->pClassifyData->pClassifyValues;
   
      if(pClassifyValues->layerId == FWPS_LAYER_ALE_CONNECT_REDIRECT_V4 ||
         pClassifyValues->layerId == FWPS_LAYER_ALE_CONNECT_REDIRECT_V6)
         status = PerformProxyConnectRedirection(&(pDPCData->pClassifyData),
                                                 &(pDPCData->pRedirectData));
      else if(pClassifyValues->layerId == FWPS_LAYER_ALE_BIND_REDIRECT_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_ALE_BIND_REDIRECT_V6)
         status = PerformProxySocketRedirection(&(pDPCData->pClassifyData),
                                                &(pDPCData->pRedirectData));
   
      if(status != STATUS_SUCCESS)
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_ERROR_LEVEL,
                    " !!!! ProxyByALERedirectDeferredProcedureCall() [status: %#x]\n",
                    status);

      KrnlHlprDPCDataDestroy(&pDPCData);
   }

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- ProxyByALERedirectDeferredProcedureCall()\n");

#endif /// DBG

   return;
}

/**
 @private_function="ProxyByALERedirectWorkItemRoutine"
 
   Purpose:                                                                                     <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Function_class_(IO_WORKITEM_ROUTINE)
VOID ProxyByALERedirectWorkItemRoutine(_In_ PDEVICE_OBJECT pDeviceObject,
                                       _In_opt_ PVOID pContext)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> ProxyByALERedirectWorkItemRoutine()\n");

#endif /// DBG
   
   UNREFERENCED_PARAMETER(pDeviceObject);

   NT_ASSERT(pContext);
   NT_ASSERT((WORKITEM_DATA*)pContext);
   NT_ASSERT(((WORKITEM_DATA*)pContext)->pClassifyData);
   NT_ASSERT(((WORKITEM_DATA*)pContext)->pRedirectData);

   WORKITEM_DATA* pWorkItemData = (WORKITEM_DATA*)pContext;

   if(pWorkItemData)
   {
      NTSTATUS              status          = STATUS_SUCCESS;
      FWPS_INCOMING_VALUES* pClassifyValues = (FWPS_INCOMING_VALUES*)pWorkItemData->pClassifyData->pClassifyValues;

      if(pClassifyValues->layerId == FWPS_LAYER_ALE_CONNECT_REDIRECT_V4 ||
         pClassifyValues->layerId == FWPS_LAYER_ALE_CONNECT_REDIRECT_V6)
         status = PerformProxyConnectRedirection(&(pWorkItemData->pClassifyData),
                                                 &(pWorkItemData->pRedirectData));
      else if(pClassifyValues->layerId == FWPS_LAYER_ALE_BIND_REDIRECT_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_ALE_BIND_REDIRECT_V6)
         status = PerformProxySocketRedirection(&(pWorkItemData->pClassifyData),
                                                &(pWorkItemData->pRedirectData));

      if(status != STATUS_SUCCESS)
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_ERROR_LEVEL,
                    " !!!! ProxyByALERedirectWorkItemRoutine() [status: %#x]\n",
                    status);

      KrnlHlprWorkItemDataDestroy(&pWorkItemData);
   }

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- ProxyUsingInjectionMethodWorkItemRoutine()\n");

#endif /// DBG
   
   return;
}

/**
 @private_function="TriggerProxyByALERedirectInline"
 
   Purpose:                                                                                     <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return == STATUS_SUCCESS)
NTSTATUS TriggerProxyByALERedirectInline(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                         _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                         _Inout_ VOID* pLayerData,
                                         _In_opt_ const VOID* pClassifyContext,
                                         _In_ const FWPS_FILTER* pFilter,
                                         _In_ UINT64 flowContext,
                                         _Inout_ FWPS_CLASSIFY_OUT* pClassifyOut,
                                         _Inout_ REDIRECT_DATA** ppRedirectData)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> TriggerProxyByALERedirectInline()\n");

#endif /// DBG
   
   NT_ASSERT(pClassifyValues);
   NT_ASSERT(pMetadata);
   NT_ASSERT(pLayerData);
   NT_ASSERT(pFilter);
   NT_ASSERT(pClassifyOut);
   NT_ASSERT(ppRedirectData);
   NT_ASSERT(*ppRedirectData);

   NTSTATUS       status        = STATUS_SUCCESS;
   CLASSIFY_DATA* pClassifyData = 0;

   HLPR_NEW(pClassifyData,
            CLASSIFY_DATA,
            WFPSAMPLER_CALLOUT_DRIVER_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pClassifyData,
                              status);

   pClassifyData->pClassifyValues  = pClassifyValues;
   pClassifyData->pMetadataValues  = pMetadata;
   pClassifyData->pPacket          = pLayerData;
   pClassifyData->pClassifyContext = pClassifyContext;
   pClassifyData->pFilter          = pFilter;
   pClassifyData->flowContext      = flowContext;
   pClassifyData->pClassifyOut     = pClassifyOut;

   (*ppRedirectData)->pClassifyOut = pClassifyOut;

   if(pClassifyValues->layerId == FWPS_LAYER_ALE_CONNECT_REDIRECT_V4 ||
      pClassifyValues->layerId == FWPS_LAYER_ALE_CONNECT_REDIRECT_V6)
      status = PerformProxyConnectRedirection(&pClassifyData,
                                              ppRedirectData);
   else if(pClassifyValues->layerId == FWPS_LAYER_ALE_BIND_REDIRECT_V4 ||
           pClassifyValues->layerId == FWPS_LAYER_ALE_BIND_REDIRECT_V6)
      status = PerformProxySocketRedirection(&pClassifyData,
                                             ppRedirectData);

   HLPR_BAIL_LABEL:

   HLPR_DELETE(pClassifyData,
               WFPSAMPLER_CALLOUT_DRIVER_TAG);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- TriggerProxyByALERedirectInline() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}

/**
 @private_function="TriggerProxyByALERedirectOutOfBand"
 
   Purpose:                                                                                     <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS TriggerProxyByALERedirectOutOfBand(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                            _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                            _Inout_ VOID* pLayerData,
                                            _In_opt_ const VOID* pClassifyContext,
                                            _In_ const FWPS_FILTER* pFilter,
                                            _In_ UINT64 flowContext,
                                            _In_ FWPS_CLASSIFY_OUT* pClassifyOut,
                                            _Inout_ REDIRECT_DATA* pRedirectData)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> TriggerProxyByALERedirectOutOfBand()\n");

#endif /// DBG
   
   NT_ASSERT(pClassifyValues);
   NT_ASSERT(pMetadata);
   NT_ASSERT(pLayerData);
   NT_ASSERT(pFilter);
   NT_ASSERT(pClassifyOut);
   NT_ASSERT(pRedirectData);

   NTSTATUS       status        = STATUS_SUCCESS;
   CLASSIFY_DATA* pClassifyData = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pClassifyData will be freed in completionFn using ProxyInjectionCompletionDataDestroy

   status = KrnlHlprClassifyDataCreateLocalCopy(&pClassifyData,
                                                pClassifyValues,
                                                pMetadata,
                                                pLayerData,
                                                pClassifyContext,
                                                pFilter,
                                                flowContext,
                                                pClassifyOut);
   HLPR_BAIL_ON_FAILURE(status);

#pragma warning(pop)

   status = FwpsPendClassify(pRedirectData->classifyHandle,
                             pFilter->filterId,
                             0,
                             pClassifyOut);
   if(status != STATUS_SUCCESS)
   {
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! TriggerProxyByALERedirectOutOfBand : FwpsPendClassify() [status: %#x]\n",
                 status);

      HLPR_BAIL;
   }

   pClassifyOut->actionType  = FWP_ACTION_BLOCK;
   pClassifyOut->rights     ^= FWPS_RIGHT_ACTION_WRITE;

   pRedirectData->isPended     = TRUE;
   pRedirectData->pClassifyOut = pClassifyData->pClassifyOut;

   if(pRedirectData->pProxyData->useWorkItems)
      status = KrnlHlprWorkItemQueue(g_pWDMDevice,
                                     ProxyByALERedirectWorkItemRoutine,
                                     pClassifyData,
                                     pRedirectData,
                                     0);
   else if(pRedirectData->pProxyData->useThreadedDPC)
      status = KrnlHlprThreadedDPCQueue(ProxyByALERedirectDeferredProcedureCall,
                                        pClassifyData,
                                        pRedirectData,
                                        0);      
   else
      status = KrnlHlprDPCQueue(ProxyByALERedirectDeferredProcedureCall,
                                pClassifyData,
                                pRedirectData,
                                0);

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
   {
      if(pClassifyData)
         KrnlHlprClassifyDataDestroyLocalCopy(&pClassifyData);
   }

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- TriggerProxyByALERedirectOutOfBand() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}

/**
 @classify_function="ClassifyProxyByALERedirect"
 
   Purpose:                                                                                     <br>
                                                                                                <br>
   Notes:    Applies to the following layers:                                                   <br>
                FWPM_LAYER_ALE_REDIRECT_BIND_V4                                                 <br>
                FWPM_LAYER_ALE_REDIRECT_BIND_V6                                                 <br>
                FWPM_LAYER_ALE_REDIRECT_CONNECT_V4                                              <br>
                FWPM_LAYER_ALE_REDIRECT_CONNECT_V6                                              <br>
                                                                                                <br>
             Microsoft recommends using FWPM_LAYER_STREAM_V{4/6} rather than proxying network 
             data to a local service.  Doing so will make for a better ecosystem, however if you 
             feel you must proxy, then it is advised to use 
             FWPM_LAYER_ALE_REDIRECT_CONNECT_V{4/6}, and have the proxy service call the 
             REDIRECT_RECORD IOCTLs so multiple proxies can coexst without losing data on the 
             origin of the data.                                                                <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF551229.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF544893.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF571005.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/HH439677.aspx                              <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI ClassifyProxyByALERedirect(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                      _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                      _Inout_opt_ VOID* pLayerData,
                                      _In_opt_ const VOID* pClassifyContext,
                                      _In_ const FWPS_FILTER* pFilter,
                                      _In_ UINT64 flowContext,
                                      _Inout_ FWPS_CLASSIFY_OUT* pClassifyOut)
{
   NT_ASSERT(pClassifyValues);
   NT_ASSERT(pMetadata);
   NT_ASSERT(pLayerData);
   NT_ASSERT(pClassifyContext);
   NT_ASSERT(pFilter);
   NT_ASSERT(pClassifyOut);
   NT_ASSERT(pClassifyValues->layerId == FWPS_LAYER_ALE_CONNECT_REDIRECT_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_CONNECT_REDIRECT_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_BIND_REDIRECT_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_BIND_REDIRECT_V6);
   NT_ASSERT(pFilter->providerContext);
   NT_ASSERT(pFilter->providerContext->type == FWPM_GENERAL_CONTEXT);
   NT_ASSERT(pFilter->providerContext->dataBuffer);
   NT_ASSERT(pFilter->providerContext->dataBuffer->size == sizeof(PC_PROXY_DATA));
   NT_ASSERT(pFilter->providerContext->dataBuffer->data);

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> ClassifyProxyByALERedirect() [Layer: %s][FilterID: %#I64x][Rights: %#x]",
              KrnlHlprFwpsLayerIDToString(pClassifyValues->layerId),
              pFilter->filterId,
              pClassifyOut->rights);

   if(pClassifyOut->rights & FWPS_RIGHT_ACTION_WRITE)
   {
      NTSTATUS       status         = STATUS_SUCCESS;
      UINT32         conditionIndex = FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_FLAGS;
      BOOLEAN        redirectSocket = FALSE;
      REDIRECT_DATA* pRedirectData  = 0;
      UINT32         index          = WFPSAMPLER_INDEX;

      if(pFilter->subLayerWeight == FWPM_SUBLAYER_UNIVERSAL_WEIGHT)
         index = UNIVERSAL_INDEX;

      if(pClassifyValues->layerId == FWPS_LAYER_ALE_BIND_REDIRECT_V4 ||
         pClassifyValues->layerId == FWPS_LAYER_ALE_BIND_REDIRECT_V6)
      {
         conditionIndex = FWPS_FIELD_ALE_BIND_REDIRECT_V4_FLAGS;

         redirectSocket = TRUE;
      }

      /// Prevent infinite redirection

#if(NTDDI_VERSION >= NTDDI_WIN8)

      if(!redirectSocket)
      {
         FWPS_CONNECTION_REDIRECT_STATE redirectionState = FWPS_CONNECTION_NOT_REDIRECTED;

         if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                           FWPS_METADATA_FIELD_REDIRECT_RECORD_HANDLE))
         {
            VOID* pRedirectContext = 0;

            redirectionState = FwpsQueryConnectionRedirectState(pMetadata->redirectRecords,
                                                                *(g_WFPSamplerDeviceData.ppRedirectionHandles[index]),
                                                                &pRedirectContext);
         }

         switch(redirectionState)
         {
            /// Go ahead and continue with our redirection
            case FWPS_CONNECTION_NOT_REDIRECTED:
            case FWPS_CONNECTION_REDIRECTED_BY_OTHER:
            {
               break;
            }
            /// We've already seen this, so let it through
            case FWPS_CONNECTION_REDIRECTED_BY_SELF:
            {
               pClassifyOut->actionType = FWP_ACTION_PERMIT;

               HLPR_BAIL;
            }
            /// Must not perform redirection. In this case we are letting the last redirection action win.
            case FWPS_CONNECTION_PREVIOUSLY_REDIRECTED_BY_SELF:
            {
               HLPR_BAIL;
            }
         }
      }

#endif /// (NTDDI_VERSION >= NTDDI_WIN8)

#pragma warning(push)
#pragma warning(disable: 6014) /// pRedirectData will be freed in completionFn using ProxyInjectionCompletionDataDestroy
      
            status = KrnlHlprRedirectDataCreate(&pRedirectData,
                                                pClassifyContext,
                                                pFilter,
                                                pClassifyOut,
                                                *(g_WFPSamplerDeviceData.ppRedirectionHandles[index]));
            HLPR_BAIL_ON_FAILURE(status);
      
#pragma warning(pop)

      /// FWP_CONDITION_FLAG_IS_REAUTHORIZE will be set if:
      ///   1) a callout with a higher priority completed its pended classification and the current 
      ///      callout is seeing the connection for the first time
      ///   2) FWPS_CLASSIFY_FLAG_REAUTHORIZE_IF_MODIFIED_BY_OTHERS was set when calling FwpsApplyModifiedLayerData
      ///      and another callout has further redirected the initial connection.
      if(pClassifyValues->incomingValue[conditionIndex].value.uint32 & FWP_CONDITION_FLAG_IS_REAUTHORIZE)
      {
         if(redirectSocket)
         {
            FWPS_BIND_REQUEST* pBindRequest = ((FWPS_BIND_REQUEST*)(pRedirectData->pWritableLayerData))->previousVersion;

            /// ReAuth for our proxying, so let it go,
            /// and set the rights to write back as it was removed by acquiring the layer data
            if(pBindRequest->modifierFilterId == pFilter->filterId)
            {
               pClassifyOut->actionType  = FWP_ACTION_PERMIT;
               pClassifyOut->rights     |= FWPS_RIGHT_ACTION_WRITE;

               KrnlHlprRedirectDataDestroy(&pRedirectData);

               HLPR_BAIL;
            }
         }
         else
         {
            FWPS_CONNECT_REQUEST* pConnectRequest = ((FWPS_CONNECT_REQUEST*)(pRedirectData->pWritableLayerData))->previousVersion;

            /// ReAuth for our proxying, so let it go,
            /// and set the rights to write back as it was removed by acquiring the layer data
            if(pConnectRequest->modifierFilterId == pFilter->filterId)
            {
               pClassifyOut->actionType = FWP_ACTION_PERMIT;
               pClassifyOut->rights     |= FWPS_RIGHT_ACTION_WRITE;

               KrnlHlprRedirectDataDestroy(&pRedirectData);

               HLPR_BAIL;
            }

#if(NTDDI_VERSION >= NTDDI_WIN8)

            else
            {
               /// No more redirection if someone else has done it locally.  We could catch the new 
               /// connection from their proxy if we really must act on it.
               /// Set the rights to write back as it was removed when acquiring the layer data
               if(pConnectRequest->localRedirectHandle)
               {
                  pClassifyOut->actionType  = FWP_ACTION_PERMIT;
                  pClassifyOut->rights     |= FWPS_RIGHT_ACTION_WRITE;

                  KrnlHlprRedirectDataDestroy(&pRedirectData);

                  HLPR_BAIL;
               }
            }

#endif /// (NTDDI_VERSION >= NTDDI_WIN8)
            
         }
      }

      if(redirectSocket)
      {
         UINT32 timesRedirected = 0;

         for(FWPS_BIND_REQUEST* pBindRequest = ((FWPS_BIND_REQUEST*)(pRedirectData->pWritableLayerData))->previousVersion;
             pBindRequest;
             pBindRequest = pBindRequest->previousVersion)
         {
            if(pBindRequest->modifierFilterId == pFilter->filterId)
               timesRedirected++;

            /// Don't redirect the same socket more than 3 times
            if(timesRedirected > 3)
            {
               status = STATUS_TOO_MANY_COMMANDS;

               HLPR_BAIL;
            }
         }
      }
      else
      {
         UINT32 timesRedirected = 0;

         for(FWPS_CONNECT_REQUEST* pConnectRequest = ((FWPS_CONNECT_REQUEST*)(pRedirectData->pWritableLayerData))->previousVersion;
             pConnectRequest;
             pConnectRequest = pConnectRequest->previousVersion)
         {
            if(pConnectRequest->modifierFilterId == pFilter->filterId)
               timesRedirected++;

            /// Don't redirect the same connection more than 3 times
            if(timesRedirected > 3)
            {
               status = STATUS_TOO_MANY_COMMANDS;

               HLPR_BAIL;
            }
         }
      }

      if(pRedirectData->pProxyData->performInline)
         status = TriggerProxyByALERedirectInline(pClassifyValues,
                                                  pMetadata,
                                                  pLayerData,
                                                  pClassifyContext,
                                                  pFilter,
                                                  flowContext,
                                                  pClassifyOut,
                                                  &pRedirectData);
      else
         status = TriggerProxyByALERedirectOutOfBand(pClassifyValues,
                                                     pMetadata,
                                                     pLayerData,
                                                     pClassifyContext,
                                                     pFilter,
                                                     flowContext,
                                                     pClassifyOut,
                                                     pRedirectData);

      HLPR_BAIL_LABEL:

      if(status != STATUS_SUCCESS)
      {
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_ERROR_LEVEL,
                    " !!!! ClassifyProxyByALERedirect() [status: %#x]\n",
                    status);

         if(pRedirectData)
            KrnlHlprRedirectDataDestroy(&pRedirectData);

         pClassifyOut->actionType = FWP_ACTION_BLOCK;
         pClassifyOut->rights     |= FWPS_RIGHT_ACTION_WRITE;
      }
   }

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- ClassifyProxyByALERedirect() [Layer: %s][FilterID: %#I64x][Action: %#x][Rights: %#x][Absorb: %s]\n",
              KrnlHlprFwpsLayerIDToString(pClassifyValues->layerId),
              pFilter->filterId,
              pClassifyOut->actionType,
              pClassifyOut->rights,
              (pClassifyOut->flags & FWPS_CLASSIFY_OUT_FLAG_ABSORB) ? "TRUE" : "FALSE");

   return;
}

/**
 @classify_function="ClassifyProxyByInjection"
 
   Purpose:                                                                                     <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI ClassifyProxyByInjection(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                    _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                    _Inout_opt_ VOID* pNetBufferList,
                                    _In_opt_ const VOID* pClassifyContext,
                                    _In_ const FWPS_FILTER* pFilter,
                                    _In_ UINT64 flowContext,
                                    _Inout_ FWPS_CLASSIFY_OUT* pClassifyOut)
{
   NT_ASSERT(pClassifyValues);
   NT_ASSERT(pMetadata);
   NT_ASSERT(pNetBufferList);
   NT_ASSERT(pFilter);
   NT_ASSERT(pClassifyOut);
   NT_ASSERT(pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_TRANSPORT_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_TRANSPORT_V6);
   NT_ASSERT(pFilter->providerContext);
   NT_ASSERT(pFilter->providerContext->type == FWPM_GENERAL_CONTEXT);
   NT_ASSERT(pFilter->providerContext->dataBuffer);
   NT_ASSERT(pFilter->providerContext->dataBuffer->data);
   NT_ASSERT(pFilter->providerContext->dataBuffer->size == sizeof(PC_PROXY_DATA));

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> ClassifyProxyByInjection() [Layer: %s][FilterID: %#I64x][Rights: %#x]",
              KrnlHlprFwpsLayerIDToString(pClassifyValues->layerId),
              pFilter->filterId,
              pClassifyOut->rights);

   if(pClassifyOut->rights & FWPS_RIGHT_ACTION_WRITE)
   {
      NTSTATUS        status         = STATUS_SUCCESS;
      INJECTION_DATA* pInjectionData = 0;

      pClassifyOut->actionType = FWP_ACTION_CONTINUE;

      if(pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V4 ||
         pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V6)
      {
         /// Validate this is the correct traffic
         PC_PROXY_DATA* pProxyData   = (PPC_PROXY_DATA)pFilter->providerContext->dataBuffer->data;
         UINT32         ipHeaderSize = 0;
         UINT8          version      = IPV4;
         IPPROTO        protocol     = IPPROTO_MAX;

         if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                           FWPS_METADATA_FIELD_IP_HEADER_SIZE))
            ipHeaderSize = pMetadata->ipHeaderSize;

         /// Initial offset is at the Transport Header, so retreat the size of the IP Header ...
         status = NdisRetreatNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)pNetBufferList),
                                                ipHeaderSize,
                                                0,
                                                0);
         if(status != STATUS_SUCCESS)
         {
            DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                       DPFLTR_ERROR_LEVEL,
                       " !!!! ClassifyProxyByInjection: NdisRetreatNetBufferDataStart() [status: %#x]\n",
                       status);
         
            HLPR_BAIL;
         }

         version = KrnlHlprIPHeaderGetVersionField((NET_BUFFER_LIST*)pNetBufferList);
         
         protocol = KrnlHlprIPHeaderGetProtocolField((NET_BUFFER_LIST*)pNetBufferList,
                                                     version == IPV4 ? AF_INET : AF_INET6);

         /// ... and advance the offset back to the original position.
         NdisAdvanceNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)pNetBufferList),
                                       ipHeaderSize,
                                       FALSE,
                                       0);

         if(protocol == pProxyData->ipProtocol)
         {
            UINT16 destinationPort = KrnlHlprTransportHeaderGetDestinationPortField((NET_BUFFER_LIST*)pNetBufferList,
                                                                                    protocol);
            UINT16 sourcePort      = KrnlHlprTransportHeaderGetSourcePortField((NET_BUFFER_LIST*)pNetBufferList,
                                                                               protocol);

            if(pProxyData->proxyLocalPort &&
               pProxyData->proxyLocalPort != destinationPort)
               HLPR_BAIL;

            if(pProxyData->proxyRemotePort &&
               pProxyData->proxyRemotePort != sourcePort)
               HLPR_BAIL;
         }
         else
            HLPR_BAIL;
      }

#pragma warning(push)
#pragma warning(disable: 6014) /// pInjectionData will be freed in completionFn using ProxyInjectionCompletionDataDestroy

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
         PC_PROXY_DATA* pProxyData = (PPC_PROXY_DATA)pFilter->providerContext->dataBuffer->data;

         pClassifyOut->actionType  = FWP_ACTION_BLOCK;
         pClassifyOut->flags      |= FWPS_CLASSIFY_OUT_FLAG_ABSORB;

         if(pProxyData->performInline)
            status = TriggerProxyInjectionInline(pClassifyValues,
                                                 pMetadata,
                                                 pNetBufferList,
                                                 pClassifyContext,
                                                 pFilter,
                                                 flowContext,
                                                 pClassifyOut,
                                                 &pInjectionData,
                                                 pProxyData);
         else
            status = TriggerProxyInjectionOutOfBand(pClassifyValues,
                                                    pMetadata,
                                                    pNetBufferList,
                                                    pClassifyContext,
                                                    pFilter,
                                                    flowContext,
                                                    pClassifyOut,
                                                    pInjectionData,
                                                    pProxyData);
      }
      else
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "   -- ClassifyProxyByInjection() Injection previously performed.\n");

      HLPR_BAIL_LABEL:

      if(status != STATUS_SUCCESS)
      {
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_ERROR_LEVEL,
                    " !!!! ClassifyProxyByInjection() [status: %#x]\n",
                    status);

         if(pInjectionData)
            KrnlHlprInjectionDataDestroy(&pInjectionData);
      }
   }

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- ClassifyProxyByInjection() [Layer: %s][FilterID: %#I64x][Action: %#x][Rights: %#x][Absorb: %s]\n",
              KrnlHlprFwpsLayerIDToString(pClassifyValues->layerId),
              pFilter->filterId,
              pClassifyOut->actionType,
              pClassifyOut->rights,
              (pClassifyOut->flags & FWPS_CLASSIFY_OUT_FLAG_ABSORB) ? "TRUE" : "FALSE");

   return;
}

#else

/**
 @classify_function="ClassifyProxyByALE"
 
   Purpose:  Stub function for downlevel OS's trying to use uplevel functionality.              <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF551229.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF544890.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI ClassifyProxyByALERedirect(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                      _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                      _Inout_opt_ VOID* pLayerData,
                                      _In_ const FWPS_FILTER* pFilter,
                                      _In_ UINT64 flowContext,
                                      _Inout_ FWPS_CLASSIFY_OUT* pClassifyOut)
{
   NT_ASSERT(pClassifyValues);
   NT_ASSERT(pMetadata);
   NT_ASSERT(pLayerData);
   NT_ASSERT(pFilter);
   NT_ASSERT(pClassifyOut);

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> ClassifyProxyByALERedirect() [Layer: %s][FilterID: %#I64x][Rights: %#x]",
              KrnlHlprFwpsLayerIDToString(pClassifyValues->layerId),
              pFilter->filterId,
              pClassifyOut->rights);

   UNREFERENCED_PARAMETER(pMetadata);
   UNREFERENCED_PARAMETER(pLayerData);
   UNREFERENCED_PARAMETER(pFilter);
   UNREFERENCED_PARAMETER(flowContext);

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_ERROR_LEVEL,
              " !!!! ClassifyProxyByALERedirect() : Method not supported prior to Windows 7 [status: %#x]\n",
              (UINT32)STATUS_UNSUCCESSFUL);

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- ClassifyProxyByALERedirect() [Layer: %s][FilterID: %#I64x][Action: %#x][Rights: %#x][Absorb: %s]\n",
              KrnlHlprFwpsLayerIDToString(pClassifyValues->layerId),
              pFilter->filterId,
              pClassifyOut->actionType,
              pClassifyOut->rights,
              (pClassifyOut->flags & FWPS_CLASSIFY_OUT_FLAG_ABSORB) ? "TRUE" : "FALSE");

   return;
};

/**
 @classify_function="ClassifyProxyByInjection"
 
   Purpose:                                                                                     <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI ClassifyProxyByInjection(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                    _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                    _Inout_opt_ VOID* pNetBufferList,
                                    _In_ const FWPS_FILTER* pFilter,
                                    _In_ UINT64 flowContext,
                                    _Inout_ FWPS_CLASSIFY_OUT* pClassifyOut)
{
   NT_ASSERT(pClassifyValues);
   NT_ASSERT(pMetadata);
   NT_ASSERT(pNetBufferList);
   NT_ASSERT(pFilter);
   NT_ASSERT(pClassifyOut);
   NT_ASSERT(pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_TRANSPORT_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_TRANSPORT_V6);
   NT_ASSERT(pFilter->providerContext);
   NT_ASSERT(pFilter->providerContext->type == FWPM_GENERAL_CONTEXT);
   NT_ASSERT(pFilter->providerContext->dataBuffer);
   NT_ASSERT(pFilter->providerContext->dataBuffer->data);
   NT_ASSERT(pFilter->providerContext->dataBuffer->size == sizeof(PC_PROXY_DATA));
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> ClassifyProxyByInjection() [Layer: %s][FilterID: %#I64x][Rights: %#x]",
              KrnlHlprFwpsLayerIDToString(pClassifyValues->layerId),
              pFilter->filterId,
              pClassifyOut->rights);

   if(pNetBufferList)
   {
      NTSTATUS        status         = STATUS_SUCCESS;
      INJECTION_DATA* pInjectionData = 0;

      pClassifyOut->actionType = FWP_ACTION_CONTINUE;

#pragma warning(push)
#pragma warning(disable: 6014) /// pInjectionData will be freed in completionFn using ProxyInjectionCompletionDataDestroy

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
         PC_PROXY_DATA* pProxyData = (PPC_PROXY_DATA)pFilter->providerContext->dataBuffer->data;

         pClassifyOut->actionType  = FWP_ACTION_BLOCK;
         pClassifyOut->flags      |= FWPS_CLASSIFY_OUT_FLAG_ABSORB;

         if(pProxyData->performInline)
            status = TriggerProxyInjectionInline(pClassifyValues,
                                                 pMetadata,
                                                 pNetBufferList,
                                                 0,
                                                 pFilter,
                                                 flowContext,
                                                 pClassifyOut,
                                                 &pInjectionData,
                                                 pProxyData);
         else
            status = TriggerProxyInjectionOutOfBand(pClassifyValues,
                                                    pMetadata,
                                                    pNetBufferList,
                                                    0,
                                                    pFilter,
                                                    flowContext,
                                                    pClassifyOut,
                                                    pInjectionData,
                                                    pProxyData);
      }
      else
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "   -- ClassifyProxyByInjection() Injection previously performed.\n");

      HLPR_BAIL_LABEL:

      if(status != STATUS_SUCCESS)
      {
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_ERROR_LEVEL,
                    " !!!! ClassifyProxyByInjection() [status: %#x]\n",
                    status);

         if(pInjectionData)
            KrnlHlprInjectionDataDestroy(&pInjectionData);
      }
   }

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- ClassifyProxyByInjection() [Layer: %s][FilterID: %#I64x][Action: %#x][Rights: %#x][Absorb: %s]\n",
              KrnlHlprFwpsLayerIDToString(pClassifyValues->layerId),
              pFilter->filterId,
              pClassifyOut->actionType,
              pClassifyOut->rights,
              (pClassifyOut->flags & FWPS_CLASSIFY_OUT_FLAG_ABSORB) ? "TRUE" : "FALSE");

   return;
}

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)
