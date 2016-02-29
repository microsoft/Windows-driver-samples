////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_Headers.cpp
//
//   Abstract:
//      This module contains kernel helper functions that assist with IP and Transport header 
//         operations.
//
//   Naming Convention:
//
//      <Module><Object><Action><Modifier>
//  
//      i.e.
//
//       KrnlHlprIPHeaderCalculateV4Checksum
//
//       <Module>
//          KrnlHlpr               -       Function is located in syslib\ and applies to kernel mode.
//       <Object>
//          {
//            IPHeader             -       Function pertains to the network's IP_HEADER.
//            ICMPv4Header         -       Function pertains to the transport's ICMPV4_HEADER.
//            ICMPv6Header         -       Function pertains to the transport's ICMPV6_HEADER.
//            TCPHeader            -       Function pertains to the transport's TCP_HEADER.
//            UDPHeader            -       Function pertains to the transport's UDP_HEADER.
//          }
//       <Action>
//          {
//             Calculate           -       Function performs a computation on the header.
//             Modify              -       Function changes the field to the provided value.
//          }
//       <Modifier>
//          {
//             Code                -       Function acts on the ICMP Code Field.
//             Destination Address -       Function acts on the IP Destination Address Field.
//             Destination Port    -       Function acts on the TCP / UDP Destination Port Field.
//             Source Address      -       Function acts on the IP Source Address Field.
//             Source Port         -       Function acts on the TCP / UDP Source Port Field.
//             Type                -       Function acts on the ICMP Type Field.
//             V4Checksum          -       Function pertains to IPv4 packets and its Checksum Field.
//          }
//
//   Private Functions:
//
//   Public Functions:
//      KrnlHlprICMPv4HeaderModifyCode(),
//      KrnlHlprICMPv4HeaderModifyType(),
//      KrnlHlprICMPv6HeaderModifyCode(),
//      KrnlHlprICMPv6HeaderModifyType(),
//      KrnlHlprIPHeaderCalculateV4Checksum(),
//      KrnlHlprIPHeaderDestroy(),
//      KrnlHlprIPHeaderGet(),
//      KrnlHlprIPHeaderGetProtocolField(),
//      KrnlHlprIPHeaderModifyDestinationAddress(),
//      KrnlHlprIPHeaderModifyLoopbackToLocal(),
//      KrnlHlprIPHeaderModifySourceAddress(),
//      KrnlHlprMACHeaderDestroy(),
//      KrnlHlprMACHeaderGet(),
//      KrnlHlprMACHeaderModifyDestinationAddress(),
//      KrnlHlprMACHeaderModifySourceAddress(),
//      KrnlHlprTCPHeaderModifyDestinationPort(),
//      KrnlHlprTCPHeaderModifySourcePort(),
//      KrnlHlprUDPHeaderModifyDestinationPort(),
//      KrnlHlprUDPHeaderModifySourcePort(),
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Enhance annotations, add
//                                              KrnlHlprIPHeaderGetDestinationAddressField, 
//                                              KrnlHlprIPHeaderGetSourceAddressField,
//                                              KrnlHlprIPHeaderGetVersionField,
//                                              KrnlHlprTransportHeaderGetSourcePortField,
//                                              KrnlHlprTransportHeaderGetDestinationPortField, add 
//                                              support for controlData, and fix various bugs.
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "HelperFunctions_Include.h"   /// .
#include "HelperFunctions_Headers.tmh" /// $(OBJ_PATH)\$(O)\ 

/**
 @private_kernel_helper_function="PrvKrnlHlprCopyBufferToMDL"
 
   Purpose:  Copy a flat buffer into an MDL chain.                                              <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
NTSTATUS PrvKrnlHlprCopyBufferToMDL(_In_reads_(bytesToCopy) const BYTE* pBuffer,
                                    _In_ PMDL pMDL,
                                    _In_ SIZE_T mdlOffset,
                                    _In_ SIZE_T bytesToCopy,
                                    _Out_ SIZE_T* pBytesCopied)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> PrvKrnlHlprCopyBufferToMDL()\n");

#endif /// DBG

   NTSTATUS status               = STATUS_SUCCESS;
   SIZE_T   mdlByteCount         = 0;
   SIZE_T   remainingBytesToCopy = bytesToCopy;
   SIZE_T   copySize             = 0;

   *pBytesCopied = 0;

   if(MmGetMdlByteCount(pMDL) >= mdlOffset + bytesToCopy)
   {
      BYTE*  pSystemAddress = 0;
      UINT32 noExecute      = 0;

#if(NTDDI_VERSION >= NTDDI_WIN8)

      noExecute = MdlMappingNoExecute;

#endif /// (NTDDI_VERSION >= NTDDI_WIN8)

      pSystemAddress = (BYTE*)MmGetSystemAddressForMdlSafe(pMDL,
                                                           LowPagePriority | noExecute);
      if(pSystemAddress)
      {
         RtlCopyMemory(pSystemAddress + mdlOffset,
                       pBuffer,
                       bytesToCopy);

         remainingBytesToCopy = 0;

         HLPR_BAIL;
      }
   }

   /// Skip over the offset in the MDL chain
   for(mdlByteCount = MmGetMdlByteCount(pMDL);
       pMDL &&
       mdlOffset >= mdlByteCount;
       mdlByteCount = MmGetMdlByteCount(pMDL))
   {
      mdlOffset -= mdlByteCount;

      pMDL = pMDL->Next;
   }

   /// Copy data while there are MDLs to walk and data to copy
   for(;
       pMDL &&
       remainingBytesToCopy > 0;
       pMDL = pMDL->Next)
   {
      BYTE* pSystemAddress = 0;
  
      mdlByteCount = MmGetMdlByteCount(pMDL);
      if(mdlByteCount)
      {
         ASSERT(mdlOffset < mdlByteCount);

         mdlByteCount -= mdlOffset;

         copySize = min(remainingBytesToCopy,
                        mdlByteCount);  

         pSystemAddress = (BYTE*)MmGetSystemAddressForMdlSafe(pMDL,
                                                              LowPagePriority);
         if(pSystemAddress)
         {
            RtlCopyMemory(pSystemAddress + mdlOffset,
                          pBuffer,
                          copySize);

            pBuffer += copySize;

            remainingBytesToCopy -= copySize;

            mdlOffset = 0;
         }
         else
         {  
            status = STATUS_INSUFFICIENT_RESOURCES;

            HLPR_BAIL;
         }
      }
   }  

   HLPR_BAIL_LABEL:
  
   *pBytesCopied = bytesToCopy - remainingBytesToCopy;

   ASSERT(*pBytesCopied <= bytesToCopy);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- PrvKrnlHlprCopyBufferToMDL() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

#ifndef MAC_HEADER____
#define MAC_HEADER____

/**
 @kernel_helper_function="KrnlHlprMACHeaderDestroy"
 
   Purpose:  Frees the allocated memory indicated in KrnlMACHeaderGet().                        <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_At_(*ppMACHeader, _Pre_ _Notnull_)
_At_(*ppMACHeader, _Post_ _Null_)
_Success_(*ppMACHeader == 0)
inline VOID KrnlHlprMACHeaderDestroy(_Inout_ VOID** ppMACHeader)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprMACHeaderDestroy()\n");

#endif /// DBG
   
   NT_ASSERT(ppMACHeader);

   HLPR_DELETE_ARRAY(*ppMACHeader,
                     WFPSAMPLER_SYSLIB_TAG);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprMACHeaderDestroy()\n");

#endif /// DBG

   return;
}

/**
 @kernel_helper_function="KrnlHlprMACHeaderGet"
 
   Purpose:  Retrieve a pointer to the MAC Header from the NET_BUFFER_LIST.                     <br>
                                                                                                <br>
   Notes:    Assumes the NBL is at the start of the MAC Header.                                 <br>
                                                                                                <br>
             Function is overloaded.                                                            <br>
                                                                                                <br>
             If needToFreeMemory is TRUE, caller should call KrnlHlprMACHeaderDestroy() when 
                finished  with the header.                                                      <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_When_(return != STATUS_SUCCESS, _At_(*ppMACHeader, _Post_ _Null_))
_When_(return == STATUS_SUCCESS, _At_(*ppMACHeader, _Post_ _Notnull_))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprMACHeaderGet(_In_ NET_BUFFER_LIST* pNetBufferList,
                              _Outptr_ VOID** ppMACHeader,
                              _Inout_ BOOLEAN* pNeedToFreeMemory,
                              _In_ UINT32 macHeaderSize)            /* 0 */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprMACHeaderGet()\n");

#endif /// DBG
   
   NT_ASSERT(pNetBufferList);
   NT_ASSERT(ppMACHeader);
   NT_ASSERT(pNeedToFreeMemory);

   NTSTATUS     status          = STATUS_SUCCESS;
   BYTE*        pBuffer         = 0;
   NET_BUFFER*  pNetBuffer      = NET_BUFFER_LIST_FIRST_NB(pNetBufferList);
   UINT32       bytesNeeded     = macHeaderSize ? macHeaderSize : NET_BUFFER_DATA_LENGTH(pNetBuffer);
   PVOID        pContiguousData = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pBuffer is expected to be cleaned up by caller using KrnlHlprMACHeaderDestroy if *pNeedToFreeMemory is TRUE

   HLPR_NEW_ARRAY(pBuffer,
                  BYTE,
                  bytesNeeded,
                  WFPSAMPLER_SYSLIB_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pBuffer,
                              status);

#pragma warning(pop)

   *pNeedToFreeMemory = TRUE;

   pContiguousData = NdisGetDataBuffer(pNetBuffer,
                                       bytesNeeded,
                                       pBuffer,
                                       1,
                                       0);
   if(!pContiguousData)
   {
      status = STATUS_UNSUCCESSFUL;

      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! KrnlHlprMACHeaderGet : NdisGetDataBuffer() [status: %#x]\n",
                 status);

      HLPR_BAIL;
   }

   if(pBuffer != pContiguousData)
   {
      HLPR_DELETE_ARRAY(pBuffer,
                        WFPSAMPLER_SYSLIB_TAG);

      *pNeedToFreeMemory = FALSE;
   }

   *ppMACHeader = pContiguousData;

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS &&
      *pNeedToFreeMemory &&
      pBuffer)
   {
      KrnlHlprMACHeaderDestroy((VOID**)&pBuffer);

      *pNeedToFreeMemory = FALSE;
   }

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprMACHeaderGet() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

/**
 @kernel_helper_function="KrnlHlprMACHeaderModifySourceAddress"
 
   Purpose:  Set the Source Address field in the MAC Header to the provided value.              <br>
                                                                                                <br>
   Notes:    The NetBufferList parameter is expected to be offset to the start of the MAC 
             Header.                                                                            <br>
                                                                                                <br>
             Assumes the Header is and Ethernet Header.                                         <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprMACHeaderModifySourceAddress(_In_ const FWP_VALUE* pValue,
                                              _Inout_ NET_BUFFER_LIST* pNetBufferList)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprMACHeaderModifySourceAddress()\n");

#endif /// DBG

   NT_ASSERT(pValue);
   NT_ASSERT(pNetBufferList);

   NTSTATUS            status          = STATUS_SUCCESS;
   VOID*               pMACHeader      = 0;
   BOOLEAN             needToFree      = FALSE;
   ETHERNET_II_HEADER* pEthernetHeader = 0;

   status = KrnlHlprMACHeaderGet(pNetBufferList,
                                 &pMACHeader,
                                 &needToFree);
   HLPR_BAIL_ON_FAILURE(status);

   pEthernetHeader = (ETHERNET_II_HEADER*)pMACHeader;

   RtlCopyMemory(pEthernetHeader->pSourceAddress,
                 pValue->byteArray6->byteArray6,
                 ETHERNET_ADDRESS_SIZE);

   HLPR_BAIL_LABEL:

   if(needToFree)
   {
      /// Copy the contents of the allocated buffer to the NBL's discontiguous buffer
      if(status == STATUS_SUCCESS)
      {
         NET_BUFFER* pFirstNetBuffer  = NET_BUFFER_LIST_FIRST_NB(pNetBufferList);
         PMDL        pCurrentMDL      = NET_BUFFER_CURRENT_MDL(pFirstNetBuffer);
         SIZE_T      currentMDLOffset = NET_BUFFER_CURRENT_MDL_OFFSET(pFirstNetBuffer);
         SIZE_T      headerSize       = sizeof(ETHERNET_II_HEADER);
         SIZE_T      bytesCopied      = 0;

         status = PrvKrnlHlprCopyBufferToMDL((BYTE*)pEthernetHeader,
                                             pCurrentMDL,
                                             currentMDLOffset,
                                             headerSize,
                                             &bytesCopied);
         if(status == STATUS_SUCCESS &&
            bytesCopied != headerSize)
            status = STATUS_INSUFFICIENT_RESOURCES;
      }

      KrnlHlprMACHeaderDestroy(&pMACHeader);
   }

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprMACHeaderModifySourceAddress() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

/**
 @kernel_helper_function="KrnlHlprMACHeaderModifyDestinationAddress"
 
   Purpose:  Set the Destination Address field in the MAC Header to the provided value.         <br>
                                                                                                <br>
   Notes:    The NetBufferList parameter is expected to be offset to the start of the MAC 
             Header.                                                                            <br>
                                                                                                <br>
             Assumes the Header is and Ethernet Header.                                         <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprMACHeaderModifyDestinationAddress(_In_ const FWP_VALUE* pValue,
                                                   _Inout_ NET_BUFFER_LIST* pNetBufferList)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprMACHeaderModifyDestinationAddress()\n");

#endif /// DBG

   NT_ASSERT(pValue);
   NT_ASSERT(pNetBufferList);

   NTSTATUS            status          = STATUS_SUCCESS;
   VOID*               pMACHeader      = 0;
   BOOLEAN             needToFree      = FALSE;
   ETHERNET_II_HEADER* pEthernetHeader = 0;

   status = KrnlHlprMACHeaderGet(pNetBufferList,
                                 &pMACHeader,
                                 &needToFree);
   HLPR_BAIL_ON_FAILURE(status);

   pEthernetHeader = (ETHERNET_II_HEADER*)pMACHeader;

   RtlCopyMemory(pEthernetHeader->pDestinationAddress,
                 pValue->byteArray6->byteArray6,
                 ETHERNET_ADDRESS_SIZE);

   HLPR_BAIL_LABEL:

   if(needToFree)
   {
      /// Copy the contents of the allocated buffer to the NBL's discontiguous buffer
      if(status == STATUS_SUCCESS)
      {
         NET_BUFFER* pFirstNetBuffer  = NET_BUFFER_LIST_FIRST_NB(pNetBufferList);
         PMDL        pCurrentMDL      = NET_BUFFER_CURRENT_MDL(pFirstNetBuffer);
         SIZE_T      currentMDLOffset = NET_BUFFER_CURRENT_MDL_OFFSET(pFirstNetBuffer);
         SIZE_T      headerSize       = sizeof(ETHERNET_II_HEADER);
         SIZE_T      bytesCopied      = 0;

         status = PrvKrnlHlprCopyBufferToMDL((BYTE*)pEthernetHeader,
                                             pCurrentMDL,
                                             currentMDLOffset,
                                             headerSize,
                                             &bytesCopied);
         if(status == STATUS_SUCCESS &&
            bytesCopied != headerSize)
            status = STATUS_INSUFFICIENT_RESOURCES;
      }

      KrnlHlprMACHeaderDestroy(&pMACHeader);
   }

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprMACHeaderModifyDestinationAddress() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

#endif /// MAC_HEADER____

#ifndef IP_HEADER____
#define IP_HEADER____

/**
 @kernel_helper_function="KrnlHlprIPHeaderDestroy"
 
   Purpose:  Frees the allocated memory indicated in KrnlIPHeaderGet().                         <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_At_(*ppIPHeader, _Pre_ _Notnull_)
_At_(*ppIPHeader, _Post_ _Null_)
_Success_(*ppIPHeader == 0)
inline VOID KrnlHlprIPHeaderDestroy(_Inout_ VOID** ppIPHeader)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprIPHeaderDestroy()\n");

#endif /// DBG
   
   NT_ASSERT(ppIPHeader);

   HLPR_DELETE_ARRAY(*ppIPHeader,
                     WFPSAMPLER_SYSLIB_TAG);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprIPHeaderDestroy()\n");

#endif /// DBG
   
   return;
}

/**
 @kernel_helper_function="KrnlHlprIPHeaderGet"
 
   Purpose:  Retrieve a pointer to the IP Header from the NET_BUFFER_LIST.                      <br>
                                                                                                <br>
   Notes:    Function is overloaded                            .                                <br>
                                                                                                <br>
             If needToFreeMemory is TRUE, caller should call KrnlHlprIPHeaderDestroy() when 
                finished  with the header.                                                      <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_When_(return != STATUS_SUCCESS, _At_(*ppIPHeader, _Post_ _Null_))
_When_(return == STATUS_SUCCESS, _At_(*ppIPHeader, _Post_ _Notnull_))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprIPHeaderGet(_In_ NET_BUFFER_LIST* pNetBufferList,
                             _In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                             _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                             _Outptr_result_buffer_(*pIPHeaderSize) VOID** ppIPHeader,
                             _Inout_ BOOLEAN* pNeedToFreeMemory,
                             _Inout_opt_ FWP_DIRECTION* pDirection,               /* 0 */
                             _Inout_opt_ UINT32* pIPHeaderSize)                   /* 0 */
{
   NT_ASSERT(pNetBufferList);
   NT_ASSERT(pClassifyValues);
   NT_ASSERT(pMetadata);
   NT_ASSERT(ppIPHeader);
   NT_ASSERT(pNeedToFreeMemory);

   NTSTATUS      status              = STATUS_SUCCESS;
   UINT32        bytesRetreated      = 0;
   UINT32        bytesAdvanced       = 0;
   UINT32        ipHeaderSize        = 0;
   UINT32        transportHeaderSize = 0;
   FWP_DIRECTION direction           = FWP_DIRECTION_MAX;
   BOOLEAN       ipHeaderAvailable   = TRUE;

#if(NTDDI_VERSION >= NTDDI_WIN8)

   UINT32        ethernetHeaderSize  = 0;

   if(FWPS_IS_L2_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_L2_METADATA_FIELD_ETHERNET_MAC_HEADER_SIZE))
      ethernetHeaderSize = pMetadata->ethernetMacHeaderSize;

#endif /// (NTDDI_VERSION >= NTDDI_WIN8)

   if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                     FWPS_METADATA_FIELD_IP_HEADER_SIZE))
      ipHeaderSize = pMetadata->ipHeaderSize;

   if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                     FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE))
      transportHeaderSize = pMetadata->transportHeaderSize;

   if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                     FWPS_METADATA_FIELD_PACKET_DIRECTION))
      direction = pMetadata->packetDirection;

   switch(pClassifyValues->layerId)
   {
      case FWPS_LAYER_INBOUND_IPPACKET_V4:
      case FWPS_LAYER_INBOUND_IPPACKET_V6:
      {
         direction = FWP_DIRECTION_INBOUND;

         bytesRetreated = ipHeaderSize;

         break;
      }
      case FWPS_LAYER_INBOUND_IPPACKET_V4_DISCARD:
      case FWPS_LAYER_INBOUND_IPPACKET_V6_DISCARD:
      {
         direction = FWP_DIRECTION_INBOUND;

         if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                           FWPS_METADATA_FIELD_DISCARD_REASON))
         {
            if(pMetadata->discardMetadata.discardModule == FWPS_DISCARD_MODULE_GENERAL &&
               pMetadata->discardMetadata.discardReason == FWPS_DISCARD_FIREWALL_POLICY)
               bytesRetreated = ipHeaderSize;
         }

         break;
      }
      case FWPS_LAYER_OUTBOUND_IPPACKET_V4:
      case FWPS_LAYER_OUTBOUND_IPPACKET_V4_DISCARD:
      case FWPS_LAYER_OUTBOUND_IPPACKET_V6:
      case FWPS_LAYER_OUTBOUND_IPPACKET_V6_DISCARD:
      {
         direction = FWP_DIRECTION_OUTBOUND;

         /// At the IP Header
   
         break;
      }
      case FWPS_LAYER_IPFORWARD_V4:
      case FWPS_LAYER_IPFORWARD_V4_DISCARD:
      case FWPS_LAYER_IPFORWARD_V6:
      case FWPS_LAYER_IPFORWARD_V6_DISCARD:
      {
         /// At the IP Header
   
         break;
      }
      case FWPS_LAYER_INBOUND_TRANSPORT_V4:
      case FWPS_LAYER_INBOUND_TRANSPORT_V4_DISCARD:
      case FWPS_LAYER_INBOUND_TRANSPORT_V6:
      case FWPS_LAYER_INBOUND_TRANSPORT_V6_DISCARD:
      {
         direction = FWP_DIRECTION_INBOUND;

         if(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_TRANSPORT_V4_IP_PROTOCOL].value.uint8 == IPPROTO_ICMP ||
            pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_TRANSPORT_V4_IP_PROTOCOL].value.uint8 == IPPROTO_ICMPV6)
            bytesRetreated = ipHeaderSize;
         else
            bytesRetreated = ipHeaderSize + transportHeaderSize;

         break;
      }
      case FWPS_LAYER_OUTBOUND_TRANSPORT_V4:
      case FWPS_LAYER_OUTBOUND_TRANSPORT_V4_DISCARD:
      case FWPS_LAYER_OUTBOUND_TRANSPORT_V6:
      case FWPS_LAYER_OUTBOUND_TRANSPORT_V6_DISCARD:
      {
         direction = FWP_DIRECTION_OUTBOUND;

         ipHeaderAvailable = FALSE;
   
         break;
      }
      case FWPS_LAYER_STREAM_V4:
      case FWPS_LAYER_STREAM_V4_DISCARD:
      case FWPS_LAYER_STREAM_V6:
      case FWPS_LAYER_STREAM_V6_DISCARD:
      {
         ipHeaderAvailable = FALSE;

         break;
      }
      case FWPS_LAYER_DATAGRAM_DATA_V4:
      case FWPS_LAYER_DATAGRAM_DATA_V4_DISCARD:
      case FWPS_LAYER_DATAGRAM_DATA_V6:
      case FWPS_LAYER_DATAGRAM_DATA_V6_DISCARD:
      {
         direction = (FWP_DIRECTION)pClassifyValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V4_DIRECTION].value.uint32;

         if(direction == FWP_DIRECTION_OUTBOUND)
            bytesRetreated = ipHeaderSize;
         else
         {
            if(pClassifyValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V4_IP_PROTOCOL].value.uint8 == IPPROTO_ICMP ||
               pClassifyValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V4_IP_PROTOCOL].value.uint8 == IPPROTO_ICMPV6)
               bytesRetreated = ipHeaderSize;
            else
               bytesRetreated = ipHeaderSize + transportHeaderSize;
         }
   
         break;
      }
      case FWPS_LAYER_INBOUND_ICMP_ERROR_V4:
      case FWPS_LAYER_INBOUND_ICMP_ERROR_V4_DISCARD:
      case FWPS_LAYER_INBOUND_ICMP_ERROR_V6:
      case FWPS_LAYER_INBOUND_ICMP_ERROR_V6_DISCARD:
      {
         direction = FWP_DIRECTION_INBOUND;

         bytesRetreated = ipHeaderSize + transportHeaderSize;

         break;
      }
      case FWPS_LAYER_OUTBOUND_ICMP_ERROR_V4:
      case FWPS_LAYER_OUTBOUND_ICMP_ERROR_V4_DISCARD:
      case FWPS_LAYER_OUTBOUND_ICMP_ERROR_V6:
      case FWPS_LAYER_OUTBOUND_ICMP_ERROR_V6_DISCARD:
      {
         direction = FWP_DIRECTION_OUTBOUND;

         bytesRetreated = ipHeaderSize;

         break;
      }
      case FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V4:
      case FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V4_DISCARD:
      case FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V6:
      case FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V6_DISCARD:
      {
         ipHeaderAvailable = FALSE;

         break;
      }
      case FWPS_LAYER_ALE_AUTH_LISTEN_V4:
      case FWPS_LAYER_ALE_AUTH_LISTEN_V4_DISCARD:
      case FWPS_LAYER_ALE_AUTH_LISTEN_V6:
      case FWPS_LAYER_ALE_AUTH_LISTEN_V6_DISCARD:
      {
         ipHeaderAvailable = FALSE;

         break;
      }
      case FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4:
      case FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4_DISCARD:
      case FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6:
      case FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6_DISCARD:
      {
         if(direction == FWP_DIRECTION_OUTBOUND)
         {
            ipHeaderAvailable = FALSE;
         }
         else
         {
            if(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_PROTOCOL].value.uint8 == IPPROTO_ICMP ||
               pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_PROTOCOL].value.uint8 == IPPROTO_ICMPV6)
               bytesRetreated = ipHeaderSize;
            else
               bytesRetreated = ipHeaderSize + transportHeaderSize;
         }

         break;
      }
      case FWPS_LAYER_ALE_AUTH_CONNECT_V4:
      case FWPS_LAYER_ALE_AUTH_CONNECT_V4_DISCARD:
      case FWPS_LAYER_ALE_AUTH_CONNECT_V6:
      case FWPS_LAYER_ALE_AUTH_CONNECT_V6_DISCARD:
      {
         if(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_PROTOCOL].value.uint8 == IPPROTO_TCP)
            ipHeaderAvailable = FALSE;
         else if(direction == FWP_DIRECTION_INBOUND)
            ipHeaderAvailable = FALSE;
         else
            ipHeaderAvailable = FALSE;

         break;
      }
      case FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4:
      case FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4_DISCARD:
      case FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6:
      case FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6_DISCARD:
      {
         direction = (FWP_DIRECTION)pClassifyValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_DIRECTION].value.uint32;

         if(direction == FWP_DIRECTION_OUTBOUND)
            bytesRetreated = ipHeaderSize;
         else
         {
            if(pClassifyValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_IP_PROTOCOL].value.uint8 == IPPROTO_ICMP ||
               pClassifyValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V6_IP_PROTOCOL].value.uint8 == IPPROTO_ICMPV6)
               bytesRetreated = ipHeaderSize;
            else
               bytesRetreated = ipHeaderSize + transportHeaderSize;
         }
   
         break;
      }

#if(NTDDI_VERSION >= NTDDI_WIN7)

      case FWPS_LAYER_NAME_RESOLUTION_CACHE_V4:
      case FWPS_LAYER_NAME_RESOLUTION_CACHE_V6:
      {
         ipHeaderAvailable = FALSE;

         break;
      }
      case FWPS_LAYER_ALE_RESOURCE_RELEASE_V4:
      case FWPS_LAYER_ALE_RESOURCE_RELEASE_V6:
      {
         ipHeaderAvailable = FALSE;

         break;
      }
      case FWPS_LAYER_ALE_ENDPOINT_CLOSURE_V4:
      case FWPS_LAYER_ALE_ENDPOINT_CLOSURE_V6:
      {
         ipHeaderAvailable = FALSE;

         break;
      }
      case FWPS_LAYER_ALE_CONNECT_REDIRECT_V4:
      case FWPS_LAYER_ALE_CONNECT_REDIRECT_V6:
      {
         ipHeaderAvailable = FALSE;

         break;
      }
      case FWPS_LAYER_ALE_BIND_REDIRECT_V4:
      case FWPS_LAYER_ALE_BIND_REDIRECT_V6:
      {
         ipHeaderAvailable = FALSE;

         break;
      }
      case FWPS_LAYER_STREAM_PACKET_V4:
      case FWPS_LAYER_STREAM_PACKET_V6:
      {
         direction = (FWP_DIRECTION)pClassifyValues->incomingValue[FWPS_FIELD_STREAM_PACKET_V4_DIRECTION].value.uint32;

         if(direction == FWP_DIRECTION_OUTBOUND)
            bytesRetreated = ipHeaderSize;
         else
            bytesRetreated = ipHeaderSize + transportHeaderSize;

         break;
      }
   
#if(NTDDI_VERSION >= NTDDI_WIN8)
   
      case FWPS_LAYER_INBOUND_MAC_FRAME_ETHERNET:
      {
         UINT16 etherType = pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_MAC_FRAME_ETHERNET_ETHER_TYPE].value.uint16;

         if(etherType != 0x86DD &&
            etherType != 0x0800)
            ipHeaderAvailable = FALSE;

         break;
      }
      case FWPS_LAYER_OUTBOUND_MAC_FRAME_ETHERNET:
      {
         UINT16 etherType = pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_MAC_FRAME_ETHERNET_ETHER_TYPE].value.uint16;

         if(etherType == 0x86DD ||
            etherType == 0x0800)
            bytesAdvanced = ethernetHeaderSize;
         else
            ipHeaderAvailable = FALSE;

         break;
      }
      case FWPS_LAYER_INBOUND_MAC_FRAME_NATIVE:
      case FWPS_LAYER_OUTBOUND_MAC_FRAME_NATIVE:
      {
         ipHeaderAvailable = FALSE;

         break;
      }
      case FWPS_LAYER_INGRESS_VSWITCH_ETHERNET:
      {
         UINT16 etherType = pClassifyValues->incomingValue[FWPS_FIELD_INGRESS_VSWITCH_ETHERNET_ETHER_TYPE].value.uint16;

         if(etherType == 0x86DD ||
            etherType == 0x0800)
            bytesAdvanced = ethernetHeaderSize;
         else
            ipHeaderAvailable = FALSE;

         break;
      }
      case FWPS_LAYER_EGRESS_VSWITCH_ETHERNET:
      {
         UINT16 etherType = pClassifyValues->incomingValue[FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_ETHER_TYPE].value.uint16;

         if(etherType == 0x86DD ||
            etherType == 0x0800)
            bytesAdvanced = ethernetHeaderSize;
         else
            ipHeaderAvailable = FALSE;

         break;
      }
      case FWPS_LAYER_INGRESS_VSWITCH_TRANSPORT_V4:
      case FWPS_LAYER_INGRESS_VSWITCH_TRANSPORT_V6:
      {
         /// At the IP Header

         break;
      }
      case FWPS_LAYER_EGRESS_VSWITCH_TRANSPORT_V4:
      case FWPS_LAYER_EGRESS_VSWITCH_TRANSPORT_V6:
      {
         /// At the IP Header

         break;
      }

#endif /// (NTDDI_VERSION >= NTDDI_WIN8)
#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

   }


   if(ipHeaderAvailable)
   {
      BYTE*        pBuffer         = 0;
      NET_BUFFER*  pNetBuffer      = NET_BUFFER_LIST_FIRST_NB(pNetBufferList);
      UINT32       bytesNeeded     = ipHeaderSize ? ipHeaderSize : NET_BUFFER_DATA_LENGTH(pNetBuffer);
      PVOID        pContiguousData = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pBuffer is expected to be cleaned up by caller using KrnlHlprIPHeaderDestroy if *pNeedToFreeMemory is TRUE

      HLPR_NEW_ARRAY(pBuffer,
                     BYTE,
                     bytesNeeded,
                     WFPSAMPLER_SYSLIB_TAG);
      HLPR_BAIL_ON_ALLOC_FAILURE(pBuffer,
                                 status);

#pragma warning(pop)

      *pNeedToFreeMemory = TRUE;

      if(bytesAdvanced)
         NdisAdvanceNetBufferDataStart(pNetBuffer,
                                       bytesAdvanced,
                                       0,
                                       0);
      else if(bytesRetreated)
      {
         status = NdisRetreatNetBufferDataStart(pNetBuffer,
                                                bytesRetreated,
                                                0,
                                                0);
         if(status != STATUS_SUCCESS)
         {
            DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                       DPFLTR_ERROR_LEVEL,
                       " !!!! KrnlHlprIPHeaderGet : NdisRetreatNetBufferDataStart() [status: %#x]\n",
                       status);

            HLPR_BAIL;
         }
      }

      pContiguousData = NdisGetDataBuffer(pNetBuffer,
                                          bytesNeeded,
                                          pBuffer,
                                          1,
                                          0);

      /// Return to the original offset
      if(bytesRetreated)
         NdisAdvanceNetBufferDataStart(pNetBuffer,
                                       bytesRetreated,
                                       0,
                                       0);
      else if(bytesAdvanced)
      {
         status = NdisRetreatNetBufferDataStart(pNetBuffer,
                                                bytesAdvanced,
                                                0,
                                                0);
         if(status != STATUS_SUCCESS)
         {
            DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                       DPFLTR_ERROR_LEVEL,
                       " !!!! KrnlHlprIPHeaderGet : NdisRetreatNetBufferDataStart() [status: %#x]\n",
                       status);

            HLPR_BAIL;
         }
      }

      if(!pContiguousData)
      {
         status = STATUS_UNSUCCESSFUL;
      
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_ERROR_LEVEL,
                    " !!!! KrnlHlprIPHeaderGet : NdisGetDataBuffer() [status: %#x]\n",
                    status);
      
         HLPR_BAIL;
      }

      if(pBuffer != pContiguousData)
      {
         HLPR_DELETE_ARRAY(pBuffer,
                           WFPSAMPLER_SYSLIB_TAG);

         *pNeedToFreeMemory = FALSE;
      }

      *ppIPHeader = pContiguousData;

      if(pDirection)
         *pDirection = direction;

      if(pIPHeaderSize)
         *pIPHeaderSize = ipHeaderSize;

      HLPR_BAIL_LABEL:

      if(status != STATUS_SUCCESS &&
         *pNeedToFreeMemory &&
         pBuffer)
      {
         KrnlHlprIPHeaderDestroy((VOID**)&pBuffer);

         *pNeedToFreeMemory = FALSE;
      }
   }
   else
      status = STATUS_NO_MATCH;

   return status;
}


/**
 @kernel_helper_function="KrnlHlprIPHeaderGet"
 
   Purpose:  Retrieve a pointer to the IP Header from the NET_BUFFER_LIST.                      <br>
                                                                                                <br>
   Notes:    Assumes the NBL is at the start of the IP Header.                                  <br>
                                                                                                <br>
             Function is overloaded.                                                            <br>
                                                                                                <br>
             If needToFreeMemory is TRUE, caller should call KrnlHlprIPHeaderDestroy() when 
                finished  with the header.                                                      <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_When_(return != STATUS_SUCCESS, _At_(*ppIPHeader, _Post_ _Null_))
_When_(return == STATUS_SUCCESS, _At_(*ppIPHeader, _Post_ _Notnull_))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprIPHeaderGet(_In_ NET_BUFFER_LIST* pNetBufferList,
                             _Outptr_result_buffer_(ipHeaderSize) VOID** ppIPHeader,
                             _Inout_ BOOLEAN* pNeedToFreeMemory,
                             _In_ UINT32 ipHeaderSize)                               /* 0 */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprIPHeaderGet()\n");

#endif /// DBG
   
   NT_ASSERT(pNetBufferList);
   NT_ASSERT(ppIPHeader);
   NT_ASSERT(pNeedToFreeMemory);

   NTSTATUS     status          = STATUS_SUCCESS;
   BYTE*        pBuffer         = 0;
   NET_BUFFER*  pNetBuffer      = NET_BUFFER_LIST_FIRST_NB(pNetBufferList);
   UINT32       bytesNeeded     = ipHeaderSize ? ipHeaderSize : NET_BUFFER_DATA_LENGTH(pNetBuffer);
   PVOID        pContiguousData = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pBuffer is expected to be cleaned up by caller using KrnlHlprIPHeaderDestroy if *pNeedToFreeMemory is TRUE

   HLPR_NEW_ARRAY(pBuffer,
                  BYTE,
                  bytesNeeded,
                  WFPSAMPLER_SYSLIB_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pBuffer,
                              status);

#pragma warning(pop)

   *pNeedToFreeMemory = TRUE;

   pContiguousData = NdisGetDataBuffer(pNetBuffer,
                                       bytesNeeded,
                                       pBuffer,
                                       1,
                                       0);
   if(!pContiguousData)
   {
      status = STATUS_UNSUCCESSFUL;

      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! KrnlHlprIPHeaderGet : NdisGetDataBuffer() [status: %#x]\n",
                 status);

      HLPR_BAIL;
   }

   if(pBuffer != pContiguousData)
   {
      HLPR_DELETE_ARRAY(pBuffer,
                        WFPSAMPLER_SYSLIB_TAG);

      *pNeedToFreeMemory = FALSE;
   }

   *ppIPHeader = pContiguousData;

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS &&
      *pNeedToFreeMemory &&
      pBuffer)
   {
      KrnlHlprIPHeaderDestroy((VOID**)&pBuffer);

      *pNeedToFreeMemory = FALSE;
   }

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprIPHeaderGet() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}

/**
 @kernel_helper_function="KrnlHlprIPHeaderGetDestinationAddressField"
 
   Purpose:  Retrieve the source address from the IP header.                                    <br>
                                                                                                <br>
   Notes:    Assumes the NBL is at the start of the IP Header.                                  <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return != 0)
BYTE* KrnlHlprIPHeaderGetDestinationAddressField(_In_ NET_BUFFER_LIST* pNetBufferList,
                                                 _In_ ADDRESS_FAMILY addressFamily)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprIPHeaderGetDestinationAddressField()\n");

#endif /// DBG

   NT_ASSERT(pNetBufferList);

   NTSTATUS status              = STATUS_SUCCESS;
   VOID*    pIPHeader           = 0;
   BOOLEAN  needToFree          = FALSE;
   BYTE*    pDestinationAddress = 0;

   status = KrnlHlprIPHeaderGet(pNetBufferList,
                                &pIPHeader,
                                &needToFree);
   HLPR_BAIL_ON_FAILURE(status);

   if(addressFamily == AF_INET6)
   {
      IP_HEADER_V6* pIPv6Header = (IP_HEADER_V6*)pIPHeader;

      pDestinationAddress = pIPv6Header->pDestinationAddress;
   }
   else
   {
      IP_HEADER_V4* pIPv4Header = (IP_HEADER_V4*)pIPHeader;
   
      pDestinationAddress = pIPv4Header->pDestinationAddress;

   }

   if(needToFree)
   {
      /// Copy the contents of the allocated buffer to the NBL's discontiguous buffer
      if(status == STATUS_SUCCESS)
      {
         NET_BUFFER* pFirstNetBuffer  = NET_BUFFER_LIST_FIRST_NB(pNetBufferList);
         PMDL        pCurrentMDL      = NET_BUFFER_CURRENT_MDL(pFirstNetBuffer);
         SIZE_T      currentMDLOffset = NET_BUFFER_CURRENT_MDL_OFFSET(pFirstNetBuffer);
         SIZE_T      headerSize       = (addressFamily == AF_INET) ? IPV4_HEADER_MIN_SIZE : IPV6_HEADER_MIN_SIZE;
         SIZE_T      bytesCopied      = 0;

         status = PrvKrnlHlprCopyBufferToMDL((BYTE*)pIPHeader,
                                             pCurrentMDL,
                                             currentMDLOffset,
                                             headerSize,
                                             &bytesCopied);
         if(status == STATUS_SUCCESS &&
            bytesCopied != headerSize)
            status = STATUS_INSUFFICIENT_RESOURCES;
      }

      KrnlHlprIPHeaderDestroy(&pIPHeader);
   }

   HLPR_BAIL_LABEL:

#if DBG

      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 " <--- KrnlHlprIPHeaderGetDestinationAddressField()\n");

#endif /// DBG

   return pDestinationAddress;
}

/**
 @kernel_helper_function="KrnlHlprIPHeaderGetSourceAddressField"
 
   Purpose:  Retrieve the source address from the IP header.                                    <br>
                                                                                                <br>
   Notes:    Assumes the NBL is at the start of the IP Header.                                  <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return != 0)
BYTE* KrnlHlprIPHeaderGetSourceAddressField(_In_ NET_BUFFER_LIST* pNetBufferList,
                                            _In_ ADDRESS_FAMILY addressFamily)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprIPHeaderGetSourceAddressField()\n");

#endif /// DBG

   NT_ASSERT(pNetBufferList);

   NTSTATUS status         = STATUS_SUCCESS;
   VOID*    pIPHeader      = 0;
   BOOLEAN  needToFree     = FALSE;
   BYTE*    pSourceAddress = 0;

   status = KrnlHlprIPHeaderGet(pNetBufferList,
                                &pIPHeader,
                                &needToFree);
   HLPR_BAIL_ON_FAILURE(status);

   if(addressFamily == AF_INET6)
   {
      IP_HEADER_V6* pIPv6Header = (IP_HEADER_V6*)pIPHeader;

      pSourceAddress = pIPv6Header->pSourceAddress;
   }
   else
   {
      IP_HEADER_V4* pIPv4Header = (IP_HEADER_V4*)pIPHeader;
   
      pSourceAddress = pIPv4Header->pSourceAddress;

   }

   if(needToFree)
   {
      /// Copy the contents of the allocated buffer to the NBL's discontiguous buffer
      if(status == STATUS_SUCCESS)
      {
         NET_BUFFER* pFirstNetBuffer  = NET_BUFFER_LIST_FIRST_NB(pNetBufferList);
         PMDL        pCurrentMDL      = NET_BUFFER_CURRENT_MDL(pFirstNetBuffer);
         SIZE_T      currentMDLOffset = NET_BUFFER_CURRENT_MDL_OFFSET(pFirstNetBuffer);
         SIZE_T      headerSize       = (addressFamily == AF_INET) ? IPV4_HEADER_MIN_SIZE : IPV6_HEADER_MIN_SIZE;
         SIZE_T      bytesCopied      = 0;

         status = PrvKrnlHlprCopyBufferToMDL((BYTE*)pIPHeader,
                                             pCurrentMDL,
                                             currentMDLOffset,
                                             headerSize,
                                             &bytesCopied);
         if(status == STATUS_SUCCESS &&
            bytesCopied != headerSize)
            status = STATUS_INSUFFICIENT_RESOURCES;
      }

      KrnlHlprIPHeaderDestroy(&pIPHeader);
   }

   HLPR_BAIL_LABEL:

#if DBG

      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 " <--- KrnlHlprIPHeaderGetSourceAddressField()\n");

#endif /// DBG

   return pSourceAddress;
}

/**
 @kernel_helper_function="KrnlHlprIPHeaderGetProtocolField"
 
   Purpose:  Retrieve the protocol from the IP header.                                          <br>
                                                                                                <br>
   Notes:    Assumes the NBL is at the start of the IP Header.                                  <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
IPPROTO KrnlHlprIPHeaderGetProtocolField(_In_ NET_BUFFER_LIST* pNetBufferList,
                                         _In_ ADDRESS_FAMILY addressFamily)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprIPHeaderGetProtocolField()\n");

#endif /// DBG

   NT_ASSERT(pNetBufferList);

   NTSTATUS status     = STATUS_SUCCESS;
   VOID*    pIPHeader  = 0;
   BOOLEAN  needToFree = FALSE;
   IPPROTO  protocol   = IPPROTO_MAX;

   status = KrnlHlprIPHeaderGet(pNetBufferList,
                                &pIPHeader,
                                &needToFree);
   HLPR_BAIL_ON_FAILURE(status);

   if(addressFamily == AF_INET6)
   {
      IP_HEADER_V6* pIPv6Header = (IP_HEADER_V6*)pIPHeader;

      protocol = (IPPROTO)pIPv6Header->nextHeader;
   }
   else
   {
      IP_HEADER_V4* pIPv4Header = (IP_HEADER_V4*)pIPHeader;
   
      protocol = (IPPROTO)pIPv4Header->protocol;
   }

   if(needToFree)
   {
      /// Copy the contents of the allocated buffer to the NBL's discontiguous buffer
      if(status == STATUS_SUCCESS)
      {
         NET_BUFFER* pFirstNetBuffer  = NET_BUFFER_LIST_FIRST_NB(pNetBufferList);
         PMDL        pCurrentMDL      = NET_BUFFER_CURRENT_MDL(pFirstNetBuffer);
         SIZE_T      currentMDLOffset = NET_BUFFER_CURRENT_MDL_OFFSET(pFirstNetBuffer);
         SIZE_T      headerSize       = (addressFamily == AF_INET) ? IPV4_HEADER_MIN_SIZE : IPV6_HEADER_MIN_SIZE;
         SIZE_T      bytesCopied      = 0;

         status = PrvKrnlHlprCopyBufferToMDL((BYTE*)pIPHeader,
                                             pCurrentMDL,
                                             currentMDLOffset,
                                             headerSize,
                                             &bytesCopied);
         if(status == STATUS_SUCCESS &&
            bytesCopied != headerSize)
            status = STATUS_INSUFFICIENT_RESOURCES;
      }

      KrnlHlprIPHeaderDestroy(&pIPHeader);
   }

   HLPR_BAIL_LABEL:

#if DBG

      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 " <--- KrnlHlprIPHeaderGetProtocolField()\n");

#endif /// DBG

   return protocol;
}

/**
 @kernel_helper_function="KrnlHlprIPHeaderGetVersionField"
 
   Purpose:  Retrieve the version from the IP header.                                           <br>
                                                                                                <br>
   Notes:    Assumes the NBL is at the start of the IP Header.                                  <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
UINT8 KrnlHlprIPHeaderGetVersionField(_In_ NET_BUFFER_LIST* pNetBufferList)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprIPHeaderGetVersionField()\n");

#endif /// DBG

   NT_ASSERT(pNetBufferList);

   NTSTATUS status     = STATUS_SUCCESS;
   BYTE*    pIPHeader  = 0;
   BOOLEAN  needToFree = FALSE;
   UINT8    version    = 0;

   status = KrnlHlprIPHeaderGet(pNetBufferList,
                                (VOID**)&pIPHeader,
                                &needToFree);
   HLPR_BAIL_ON_FAILURE(status);

   version = pIPHeader[0] >> 4;

   if(needToFree)
   {
      /// Copy the contents of the allocated buffer to the NBL's discontiguous buffer
      if(status == STATUS_SUCCESS)
      {
         NET_BUFFER* pFirstNetBuffer  = NET_BUFFER_LIST_FIRST_NB(pNetBufferList);
         PMDL        pCurrentMDL      = NET_BUFFER_CURRENT_MDL(pFirstNetBuffer);
         SIZE_T      currentMDLOffset = NET_BUFFER_CURRENT_MDL_OFFSET(pFirstNetBuffer);
         SIZE_T      headerSize       = (version == IPV4) ? IPV4_HEADER_MIN_SIZE : IPV6_HEADER_MIN_SIZE;
         SIZE_T      bytesCopied      = 0;

         status = PrvKrnlHlprCopyBufferToMDL(pIPHeader,
                                             pCurrentMDL,
                                             currentMDLOffset,
                                             headerSize,
                                             &bytesCopied);
         if(status == STATUS_SUCCESS &&
            bytesCopied != headerSize)
            status = STATUS_INSUFFICIENT_RESOURCES;
      }

      KrnlHlprIPHeaderDestroy((VOID**)&pIPHeader);
   }

   HLPR_BAIL_LABEL:

#if DBG

      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 " <--- KrnlHlprIPHeaderGetVersionField()\n");

#endif /// DBG

   return version;
}

/**
 @kernel_helper_function="KrnlHlprIPHeaderCalculateV4Checksum"
 
   Purpose:  Calculate the Checksum for the IPv4 Header.                                        <br>
                                                                                                <br>
   Notes:    Assumes the NBL is at the start of the IPv4 Header.                                <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID KrnlHlprIPHeaderCalculateV4Checksum(_Inout_ NET_BUFFER_LIST* pNetBufferList,
                                         _In_ UINT32 ipHeaderSize)                 /* IPV4_HEADER_MIN_SIZE */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprIPHeaderCalculateV4Checksum()\n");

#endif /// DBG
   
   NT_ASSERT(pNetBufferList);

   NTSTATUS      status      = STATUS_SUCCESS;
   IP_HEADER_V4* pIPv4Header = 0;
   BOOLEAN       needToFree  = FALSE;

   status = KrnlHlprIPHeaderGet(pNetBufferList,
                                (VOID**)&pIPv4Header,
                                &needToFree,
                                ipHeaderSize);
   if(status == STATUS_SUCCESS &&
      ipHeaderSize >= IPV4_HEADER_MIN_SIZE)
   {
      UINT32            sum    = 0;
      UINT32            words  = ipHeaderSize / 2;
      UINT16 UNALIGNED* pStart = (UINT16*)pIPv4Header;

      pIPv4Header->checksum = 0;

      for(UINT8 i = 0;
          i < words;
          i++)
      {
         sum += pStart[i];
      }

      sum = (sum & 0x0000ffff) + (sum >> 16);
      sum += (sum >> 16);

      pIPv4Header->checksum = (UINT16)~sum;

      if(needToFree)
      {
         /// Copy the contents of the allocated buffer to the NBL's discontiguous buffer
         if(status == STATUS_SUCCESS)
         {
            NET_BUFFER* pFirstNetBuffer  = NET_BUFFER_LIST_FIRST_NB(pNetBufferList);
            PMDL        pCurrentMDL      = NET_BUFFER_CURRENT_MDL(pFirstNetBuffer);
            SIZE_T      currentMDLOffset = NET_BUFFER_CURRENT_MDL_OFFSET(pFirstNetBuffer);
            SIZE_T      headerSize       = ipHeaderSize ? ipHeaderSize : IPV4_HEADER_MIN_SIZE;
            SIZE_T      bytesCopied      = 0;

            status = PrvKrnlHlprCopyBufferToMDL((BYTE*)pIPv4Header,
                                                pCurrentMDL,
                                                currentMDLOffset,
                                                headerSize,
                                                &bytesCopied);
            if(status == STATUS_SUCCESS &&
               bytesCopied != headerSize)
               status = STATUS_INSUFFICIENT_RESOURCES;
         }

         KrnlHlprIPHeaderDestroy((VOID**)&pIPv4Header);
      }
   }

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprIPHeaderCalculateV4Checksum()\n");

#endif /// DBG
   
   return;
}

/**
 @kernel_helper_function="KrnlHlprIPHeaderModifySourceAddress"
 
   Purpose:  Set the Source Address field in the IP Header to the provided value.               <br>
                                                                                                <br>
   Notes:    The NetBufferList parameter is expected to be offset to the start of the IP Header.<br>
                                                                                                <br>
             Values should be in Network Byte Order.                                            <br>
                                                                                                <br>
             Function is IP version agnostic.                                                   <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprIPHeaderModifySourceAddress(_In_ const FWP_VALUE* pValue,
                                             _Inout_ NET_BUFFER_LIST* pNetBufferList,
                                             _In_ BOOLEAN recalculateChecksum,        /* TRUE */
                                             _In_ BOOLEAN convertByteOrder)           /* FALSE */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprIPHeaderModifySourceAddress()\n");

#endif /// DBG
   
   NT_ASSERT(pValue);
   NT_ASSERT(pNetBufferList);

   NTSTATUS status     = STATUS_SUCCESS;
   VOID*    pIPHeader  = 0;
   BOOLEAN  needToFree = FALSE;

   status = KrnlHlprIPHeaderGet(pNetBufferList,
                                &pIPHeader,
                                &needToFree);
   HLPR_BAIL_ON_FAILURE(status);

   switch(pValue->type)
   {
      case FWP_UINT32:
      {
         IP_HEADER_V4* pIPv4Header   = (IP_HEADER_V4*)pIPHeader;
         UINT32        sourceAddress = convertByteOrder ? htonl(pValue->uint32) : pValue->uint32;

         RtlCopyMemory(pIPv4Header->pSourceAddress,
                       &sourceAddress,
                       IPV4_ADDRESS_SIZE);

         break;
      }
      case FWP_BYTE_ARRAY16_TYPE:
      {
         IP_HEADER_V6* pIPv6Header = (IP_HEADER_V6*)pIPHeader;

         RtlCopyMemory(pIPv6Header->pSourceAddress,
                       &(pValue->byteArray16->byteArray16),
                       IPV6_ADDRESS_SIZE);

         break;
      }
   }

   HLPR_BAIL_LABEL:

   if(needToFree)
   {
      /// Copy the contents of the allocated buffer to the NBL's discontiguous buffer
      if(status == STATUS_SUCCESS)
      {
         NET_BUFFER* pFirstNetBuffer  = NET_BUFFER_LIST_FIRST_NB(pNetBufferList);
         PMDL        pCurrentMDL      = NET_BUFFER_CURRENT_MDL(pFirstNetBuffer);
         SIZE_T      currentMDLOffset = NET_BUFFER_CURRENT_MDL_OFFSET(pFirstNetBuffer);
         SIZE_T      headerSize       = (pValue->type == FWP_UINT32) ? IPV4_HEADER_MIN_SIZE : IPV6_HEADER_MIN_SIZE;
         SIZE_T      bytesCopied      = 0;

         status = PrvKrnlHlprCopyBufferToMDL((BYTE*)pIPHeader,
                                             pCurrentMDL,
                                             currentMDLOffset,
                                             headerSize,
                                             &bytesCopied);
         if(status == STATUS_SUCCESS &&
            bytesCopied != headerSize)
            status = STATUS_INSUFFICIENT_RESOURCES;
      }

      KrnlHlprIPHeaderDestroy(&pIPHeader);
   }

   if(recalculateChecksum &&
      pValue->type == FWP_UINT32)
      KrnlHlprIPHeaderCalculateV4Checksum(pNetBufferList);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprIPHeaderModifySourceAddress() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}

/**
 @kernel_helper_function="KrnlHlprIPHeaderModifyDestinationAddress"
 
   Purpose:  Set the Destination Address field in the IP Header to the provided value.          <br>
                                                                                                <br>
   Notes:    The NetBufferList parameter is expected to be offset to the start of the IP Header.<br>
                                                                                                <br>
             Values should be in Network Byte Order.                                            <br>
                                                                                                <br>
             Function is IP version agnostic.                                                   <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprIPHeaderModifyDestinationAddress(_In_ const FWP_VALUE* pValue,
                                                  _Inout_ NET_BUFFER_LIST* pNetBufferList,
                                                  _In_ const BOOLEAN recalculateChecksum,  /* TRUE */
                                                  _In_ BOOLEAN convertByteOrder)           /* FALSE */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprIPHeaderModifyDestinationAddress()\n");

#endif /// DBG
   
   NT_ASSERT(pValue);
   NT_ASSERT(pNetBufferList);

   NTSTATUS status     = STATUS_SUCCESS;
   VOID*    pIPHeader  = 0;
   BOOLEAN  needToFree = FALSE;

   status = KrnlHlprIPHeaderGet(pNetBufferList,
                                &pIPHeader,
                                &needToFree);
   HLPR_BAIL_ON_FAILURE(status);

   switch(pValue->type)
   {
      case FWP_UINT32:
      {
         IP_HEADER_V4* pIPv4Header        = (IP_HEADER_V4*)pIPHeader;
         UINT32        destinationAddress = convertByteOrder ? htonl(pValue->uint32) : pValue->uint32;

         RtlCopyMemory(pIPv4Header->pDestinationAddress,
                       &destinationAddress,
                       IPV4_ADDRESS_SIZE);

         break;
      }
      case FWP_BYTE_ARRAY16_TYPE:
      {
         IP_HEADER_V6* pIPv6Header = (IP_HEADER_V6*)pIPHeader;

         RtlCopyMemory(pIPv6Header->pDestinationAddress,
                       &(pValue->byteArray16->byteArray16),
                       IPV6_ADDRESS_SIZE);

         break;
      }
   }

   HLPR_BAIL_LABEL:

   if(needToFree)
   {
      /// Copy the contents of the allocated buffer to the NBL's discontiguous buffer
      if(status == STATUS_SUCCESS)
      {
         NET_BUFFER* pFirstNetBuffer  = NET_BUFFER_LIST_FIRST_NB(pNetBufferList);
         PMDL        pCurrentMDL      = NET_BUFFER_CURRENT_MDL(pFirstNetBuffer);
         SIZE_T      currentMDLOffset = NET_BUFFER_CURRENT_MDL_OFFSET(pFirstNetBuffer);
         SIZE_T      headerSize       = (pValue->type == FWP_UINT32) ? IPV4_HEADER_MIN_SIZE : IPV6_HEADER_MIN_SIZE;
         SIZE_T      bytesCopied      = 0;

         status = PrvKrnlHlprCopyBufferToMDL((BYTE*)pIPHeader,
                                             pCurrentMDL,
                                             currentMDLOffset,
                                             headerSize,
                                             &bytesCopied);
         if(status == STATUS_SUCCESS &&
            bytesCopied != headerSize)
            status = STATUS_INSUFFICIENT_RESOURCES;
      }

      KrnlHlprIPHeaderDestroy(&pIPHeader);
   }

   if(recalculateChecksum &&
      pValue->type == FWP_UINT32)
      KrnlHlprIPHeaderCalculateV4Checksum(pNetBufferList);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprIPHeaderModifyDestinationAddress() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}

/**
 @kernel_helper_function="KrnlHlprIPHeaderModifyLoopbackToLocal"
 
   Purpose:  Modifies the source address and destination address from software loopback to an 
             actual local IP address (i.e. 127.0.0.1 to 157.59.10.233).                         <br>
                                                                                                <br>
   Notes:    The NetBufferList parameter is expected to be offset to the start of the IP Header.<br>
                                                                                                <br>
             The source address is modified to pass TCP/IP's source IP address validation, and 
             the destination address is modified to pass TCP/IP's zone crossing restrictions.   <br>
                                                                                                <br>
             For some protocols, the need to capture and modify a response packet's addresses 
             back to the loopback addresses will exist (i.e. ICMP Echo Requests)                <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprIPHeaderModifyLoopbackToLocal(_In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                               _In_ const FWP_VALUE* pLoopbackAddress,
                                               _In_ const UINT32 ipHeaderSize,
                                               _Inout_ NET_BUFFER_LIST* pNetBufferList,
                                               _In_reads_opt_(controlDataSize) const WSACMSGHDR* pControlData, /* 0 */
                                               _In_ UINT32 controlDataSize)                                    /* 0 */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprIPHeaderModifyLoopbackToLocal()\n");

#endif /// DBG
   
   NT_ASSERT(pMetadata);
   NT_ASSERT(pLoopbackAddress);
   NT_ASSERT(pNetBufferList);

   NTSTATUS       status           = STATUS_SUCCESS;
   ADDRESS_FAMILY addressFamily    = pLoopbackAddress->type == FWP_BYTE_ARRAY16_TYPE ? AF_INET6 : AF_INET;
   VOID*          pIPHeader        = 0;
   BOOLEAN        needToFreeMemory = FALSE;
   UINT8*         pLocalAddress    = 0;
   IPPROTO        nextProtocol     = IPPROTO_MAX;

   status = KrnlHlprIPHeaderGet(pNetBufferList,
                                &pIPHeader,
                                &needToFreeMemory,
                                ipHeaderSize);
   HLPR_BAIL_ON_FAILURE(status);

   if(addressFamily == AF_INET)
   {
      IP_HEADER_V4* pIPv4Header = (IP_HEADER_V4*)pIPHeader;

      nextProtocol = (IPPROTO)pIPv4Header->protocol;

      /// Only modify if the addresses are different
      if(RtlCompareMemory(pIPv4Header->pSourceAddress,
                          pIPv4Header->pDestinationAddress,
                          IPV4_ADDRESS_SIZE) != IPV4_ADDRESS_SIZE)
      {
         HLPR_NEW_ARRAY(pLocalAddress,
                        UINT8,
                        IPV4_ADDRESS_SIZE,
                        WFPSAMPLER_SYSLIB_TAG);
         HLPR_BAIL_ON_ALLOC_FAILURE(pLocalAddress,
                                    status);

         if(RtlCompareMemory(pIPv4Header->pSourceAddress,
                             IPV4_LOOPBACK_ADDRESS,
                             IPV4_ADDRESS_SIZE) == IPV4_ADDRESS_SIZE)
            RtlCopyMemory(pLocalAddress,
                          pIPv4Header->pDestinationAddress,
                          IPV4_ADDRESS_SIZE);
         else
            RtlCopyMemory(pLocalAddress,
                          pIPv4Header->pSourceAddress,
                          IPV4_ADDRESS_SIZE);
      }
   }
   else
   {
      IP_HEADER_V6* pIPv6Header = (IP_HEADER_V6*)pIPHeader;

      nextProtocol = (IPPROTO)pIPv6Header->nextHeader;

      /// Only modify if the addresses are different
      if(RtlCompareMemory(pIPv6Header->pSourceAddress,
                          pIPv6Header->pDestinationAddress,
                          IPV6_ADDRESS_SIZE) != IPV6_ADDRESS_SIZE)
      {
         HLPR_NEW_ARRAY(pLocalAddress,
                        UINT8,
                        IPV6_ADDRESS_SIZE,
                        WFPSAMPLER_SYSLIB_TAG);
         HLPR_BAIL_ON_ALLOC_FAILURE(pLocalAddress,
                                    status);

         if(RtlCompareMemory(pIPv6Header->pSourceAddress,
                             IPV6_LOOPBACK_ADDRESS,
                             IPV6_ADDRESS_SIZE) == IPV6_ADDRESS_SIZE)
            RtlCopyMemory(pLocalAddress,
                          pIPv6Header->pDestinationAddress,
                          IPV6_ADDRESS_SIZE);
         else
            RtlCopyMemory(pLocalAddress,
                          pIPv6Header->pSourceAddress,
                          IPV6_ADDRESS_SIZE);
      }
   }

   if(pLocalAddress)
   {
      UINT64 endpointHandle = 0;

      if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_METADATA_FIELD_TRANSPORT_ENDPOINT_HANDLE))
         endpointHandle = pMetadata->transportEndpointHandle;

      /// Rebuild the IP Header (recalculating the IP and transport checksums)
      status = FwpsConstructIpHeaderForTransportPacket(pNetBufferList,
                                                       ipHeaderSize,
                                                       addressFamily,
                                                       pLocalAddress,
                                                       pLocalAddress,
                                                       nextProtocol,
                                                       endpointHandle,
                                                       pControlData,
                                                       controlDataSize,
                                                       0,
                                                       0,
                                                       0,
                                                       0);
      if(status != STATUS_SUCCESS)
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_ERROR_LEVEL,
                    " !!!! KrnlHlprIPHeaderModifyLoopbackToLocal : FwpsConstructIpHeaderForTransportPacket() [status: %#x]\n",
                    status);
   }

   HLPR_BAIL_LABEL:

   HLPR_DELETE_ARRAY(pLocalAddress,
                     WFPSAMPLER_SYSLIB_TAG);

   if(needToFreeMemory)
      KrnlHlprIPHeaderDestroy(&pIPHeader);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprIPHeaderModifyLoopbackToLocal() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}

#endif /// IP_HEADER____

#ifndef TRANSPORT_HEADERS____
#define TRANSPORT_HEADERS____

/**
 @kernel_helper_function="KrnlHlprTransportHeaderDestroy"
 
   Purpose:  Frees the allocated memory indicated in KrnlTransportHeaderGet().                  <br>
                                                                                                <br>
   Notes:    For use with generic and specific transport header functions.                      <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_At_(*ppTransportHeader, _Pre_ _Notnull_)
_At_(*ppTransportHeader, _Post_ _Null_)
_Success_(*ppTransportHeader == 0)
inline VOID KrnlHlprTransportHeaderDestroy(_Inout_ VOID** ppTransportHeader)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprTransportHeaderDestroy()\n");

#endif /// DBG
   
   NT_ASSERT(ppTransportHeader);

   HLPR_DELETE_ARRAY(*ppTransportHeader,
                     WFPSAMPLER_SYSLIB_TAG);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprTransportHeaderDestroy()\n");

#endif /// DBG
   
   return;
}

/**
 @kernel_helper_function="KrnlHlprTransportHeaderGet"
 
   Purpose:  Retrieve a pointer to the Transport Header from the NET_BUFFER_LIST.               <br>
                                                                                                <br>
   Notes:    Function is overloaded.                            .                               <br>
                                                                                                <br>
             If needToFreeMemory is TRUE, caller should call KrnlHlprTransportHeaderDestroy() 
                when finished with the header.                                                  <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_When_(return != STATUS_SUCCESS, _At_(*ppTransportHeader, _Post_ _Null_))
_When_(return == STATUS_SUCCESS, _At_(*ppTransportHeader, _Post_ _Notnull_))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprTransportHeaderGet(_In_ NET_BUFFER_LIST* pNetBufferList,
                                    _In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                    _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                    _Outptr_result_buffer_(*pTransportHeaderSize) VOID** ppTransportHeader,
                                    _Inout_ BOOLEAN* pNeedToFreeMemory,
                                    _Inout_opt_ IPPROTO* pProtocol,                                         /* 0 */
                                    _Inout_opt_ FWP_DIRECTION* pDirection,                                  /* 0 */
                                    _Inout_opt_ UINT32* pTransportHeaderSize)                               /* 0 */
{
   NT_ASSERT(pNetBufferList);
   NT_ASSERT(pClassifyValues);
   NT_ASSERT(pMetadata);
   NT_ASSERT(ppTransportHeader);
   NT_ASSERT(pNeedToFreeMemory);

   NTSTATUS      status                   = STATUS_SUCCESS;
   UINT32        bytesRetreated           = 0;
   UINT32        bytesAdvanced            = 0;
   UINT32        ipHeaderSize             = 0;
   UINT32        transportHeaderSize      = 0;
   FWP_DIRECTION direction                = FWP_DIRECTION_MAX;
   IPPROTO       protocol                 = IPPROTO_MAX;
   BOOLEAN       transportHeaderAvailable = TRUE;

   if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                     FWPS_METADATA_FIELD_IP_HEADER_SIZE))
      ipHeaderSize = pMetadata->ipHeaderSize;

   if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                     FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE))
      transportHeaderSize = pMetadata->transportHeaderSize;

   if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                     FWPS_METADATA_FIELD_PACKET_DIRECTION))
      direction = pMetadata->packetDirection;

   switch(pClassifyValues->layerId)
   {
      case FWPS_LAYER_INBOUND_IPPACKET_V4:
      case FWPS_LAYER_INBOUND_IPPACKET_V6:
      {
         /// At the Transport Header

         break;
      }
      case FWPS_LAYER_INBOUND_IPPACKET_V4_DISCARD:
      case FWPS_LAYER_INBOUND_IPPACKET_V6_DISCARD:
      {
         if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                           FWPS_METADATA_FIELD_DISCARD_REASON))
         {
            if(pMetadata->discardMetadata.discardModule == FWPS_DISCARD_MODULE_GENERAL &&
               pMetadata->discardMetadata.discardReason == FWPS_DISCARD_FIREWALL_POLICY)
            {
               /// At the Transport Header
            }
         }

         break;
      }
      case FWPS_LAYER_OUTBOUND_IPPACKET_V4:
      case FWPS_LAYER_OUTBOUND_IPPACKET_V4_DISCARD:
      case FWPS_LAYER_OUTBOUND_IPPACKET_V6:
      case FWPS_LAYER_OUTBOUND_IPPACKET_V6_DISCARD:
      {
         bytesAdvanced = ipHeaderSize;
   
         break;
      }
      case FWPS_LAYER_IPFORWARD_V4:
      case FWPS_LAYER_IPFORWARD_V4_DISCARD:
      case FWPS_LAYER_IPFORWARD_V6:
      case FWPS_LAYER_IPFORWARD_V6_DISCARD:
      {
         bytesAdvanced = ipHeaderSize;
   
         break;
      }
      case FWPS_LAYER_INBOUND_TRANSPORT_V4:
      case FWPS_LAYER_INBOUND_TRANSPORT_V4_DISCARD:
      case FWPS_LAYER_INBOUND_TRANSPORT_V6:
      case FWPS_LAYER_INBOUND_TRANSPORT_V6_DISCARD:
      {
         protocol = (IPPROTO)(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_TRANSPORT_V4_IP_PROTOCOL].value.uint8);

         if(protocol == IPPROTO_ICMP ||
            protocol == IPPROTO_ICMPV6)
         {
            /// At the Transport Header
         }
         else
            bytesRetreated = transportHeaderSize;

         break;
      }
      case FWPS_LAYER_OUTBOUND_TRANSPORT_V4:
      case FWPS_LAYER_OUTBOUND_TRANSPORT_V4_DISCARD:
      case FWPS_LAYER_OUTBOUND_TRANSPORT_V6:
      case FWPS_LAYER_OUTBOUND_TRANSPORT_V6_DISCARD:
      {
         protocol = (IPPROTO)(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_TRANSPORT_V4_IP_PROTOCOL].value.uint8);

         /// At the Transport Header
   
         break;
      }
      case FWPS_LAYER_STREAM_V4:
      case FWPS_LAYER_STREAM_V4_DISCARD:
      case FWPS_LAYER_STREAM_V6:
      case FWPS_LAYER_STREAM_V6_DISCARD:
      {
         transportHeaderAvailable = FALSE;

         break;
      }
      case FWPS_LAYER_DATAGRAM_DATA_V4:
      case FWPS_LAYER_DATAGRAM_DATA_V4_DISCARD:
      case FWPS_LAYER_DATAGRAM_DATA_V6:
      case FWPS_LAYER_DATAGRAM_DATA_V6_DISCARD:
      {
         protocol = (IPPROTO)(pClassifyValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V4_IP_PROTOCOL].value.uint8);

         direction = (FWP_DIRECTION)pClassifyValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V4_DIRECTION].value.uint32;

         if(direction == FWP_DIRECTION_OUTBOUND)
         {
            /// At the Transport Header
         }
         else
         {
            if(protocol == IPPROTO_ICMP ||
               protocol == IPPROTO_ICMPV6)
            {
               /// At the Transport Header
            }
            else
               bytesRetreated = transportHeaderSize;
         }
   
         break;
      }
      case FWPS_LAYER_INBOUND_ICMP_ERROR_V4:
      case FWPS_LAYER_INBOUND_ICMP_ERROR_V4_DISCARD:
      case FWPS_LAYER_INBOUND_ICMP_ERROR_V6:
      case FWPS_LAYER_INBOUND_ICMP_ERROR_V6_DISCARD:
      {
         direction = FWP_DIRECTION_INBOUND;

         bytesRetreated = transportHeaderSize;

         break;
      }
      case FWPS_LAYER_OUTBOUND_ICMP_ERROR_V4:
      case FWPS_LAYER_OUTBOUND_ICMP_ERROR_V4_DISCARD:
      case FWPS_LAYER_OUTBOUND_ICMP_ERROR_V6:
      case FWPS_LAYER_OUTBOUND_ICMP_ERROR_V6_DISCARD:
      {
         direction = FWP_DIRECTION_OUTBOUND;

         /// At the Transport Header

         break;
      }
      case FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V4:
      case FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V4_DISCARD:
      case FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V6:
      case FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V6_DISCARD:
      {
         transportHeaderAvailable = FALSE;

         break;
      }
      case FWPS_LAYER_ALE_AUTH_LISTEN_V4:
      case FWPS_LAYER_ALE_AUTH_LISTEN_V4_DISCARD:
      case FWPS_LAYER_ALE_AUTH_LISTEN_V6:
      case FWPS_LAYER_ALE_AUTH_LISTEN_V6_DISCARD:
      {
         transportHeaderAvailable = FALSE;

         break;
      }
      case FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4:
      case FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4_DISCARD:
      case FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6:
      case FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6_DISCARD:
      {
         protocol = (IPPROTO)(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_PROTOCOL].value.uint8);

         if(direction == FWP_DIRECTION_OUTBOUND)
         {
            /// At the Transport Header
         }
         else
         {
            if(protocol == IPPROTO_ICMP ||
               protocol == IPPROTO_ICMPV6)
            {
               /// At the Transport Header
            }
            else
               bytesRetreated = transportHeaderSize;
         }

         break;
      }
      case FWPS_LAYER_ALE_AUTH_CONNECT_V4:
      case FWPS_LAYER_ALE_AUTH_CONNECT_V4_DISCARD:
      case FWPS_LAYER_ALE_AUTH_CONNECT_V6:
      case FWPS_LAYER_ALE_AUTH_CONNECT_V6_DISCARD:
      {
         protocol = (IPPROTO)(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_PROTOCOL].value.uint8);

         if(protocol == IPPROTO_TCP)
            transportHeaderAvailable = FALSE;
         if(direction == FWP_DIRECTION_INBOUND)
         {
            /// At the Transport Header
         }
         else
         {
            /// At the Transport Header
         }
   
         break;
      }
      case FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4:
      case FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4_DISCARD:
      case FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6:
      case FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6_DISCARD:
      {
         direction = (FWP_DIRECTION)pClassifyValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_DIRECTION].value.uint32;

         protocol = (IPPROTO)(pClassifyValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_IP_PROTOCOL].value.uint8);

         if(direction == FWP_DIRECTION_OUTBOUND)
         {
            /// At the Transport Header
         }
         else
         {
            if(protocol == IPPROTO_ICMP ||
               protocol == IPPROTO_ICMPV6)
            {
               /// At the Transport Header
            }
            else
               bytesRetreated =  transportHeaderSize;
         }
   
         break;
      }

#if(NTDDI_VERSION >= NTDDI_WIN7)

      case FWPS_LAYER_NAME_RESOLUTION_CACHE_V4:
      case FWPS_LAYER_NAME_RESOLUTION_CACHE_V6:
      {
         transportHeaderAvailable = FALSE;

         break;
      }
      case FWPS_LAYER_ALE_RESOURCE_RELEASE_V4:
      case FWPS_LAYER_ALE_RESOURCE_RELEASE_V6:
      {
         transportHeaderAvailable = FALSE;

         break;
      }
      case FWPS_LAYER_ALE_ENDPOINT_CLOSURE_V4:
      case FWPS_LAYER_ALE_ENDPOINT_CLOSURE_V6:
      {
         transportHeaderAvailable = FALSE;

         break;
      }
      case FWPS_LAYER_ALE_CONNECT_REDIRECT_V4:
      case FWPS_LAYER_ALE_CONNECT_REDIRECT_V6:
      {
         transportHeaderAvailable = FALSE;

         break;
      }
      case FWPS_LAYER_ALE_BIND_REDIRECT_V4:
      case FWPS_LAYER_ALE_BIND_REDIRECT_V6:
      {
         transportHeaderAvailable = FALSE;

         break;
      }
      case FWPS_LAYER_STREAM_PACKET_V4:
      case FWPS_LAYER_STREAM_PACKET_V6:
      {
         direction = (FWP_DIRECTION)pClassifyValues->incomingValue[FWPS_FIELD_STREAM_PACKET_V4_DIRECTION].value.uint32;

         protocol = IPPROTO_TCP;

         if(direction == FWP_DIRECTION_OUTBOUND)
         {
            /// At the Transport Header
         }
         else
            bytesRetreated =  transportHeaderSize;

         break;
      }
   
#if(NTDDI_VERSION >= NTDDI_WIN8)
   
      case FWPS_LAYER_INBOUND_MAC_FRAME_ETHERNET:
      case FWPS_LAYER_OUTBOUND_MAC_FRAME_ETHERNET:
      case FWPS_LAYER_INBOUND_MAC_FRAME_NATIVE:
      case FWPS_LAYER_OUTBOUND_MAC_FRAME_NATIVE:
      case FWPS_LAYER_INGRESS_VSWITCH_ETHERNET:
      case FWPS_LAYER_EGRESS_VSWITCH_ETHERNET:
      {
         transportHeaderAvailable = FALSE;

         break;
      }
      case FWPS_LAYER_INGRESS_VSWITCH_TRANSPORT_V4:
      case FWPS_LAYER_INGRESS_VSWITCH_TRANSPORT_V6:
      {
         bytesAdvanced = ipHeaderSize;

         break;
      }
      case FWPS_LAYER_EGRESS_VSWITCH_TRANSPORT_V4:
      case FWPS_LAYER_EGRESS_VSWITCH_TRANSPORT_V6:
      {
         bytesAdvanced = ipHeaderSize;

         break;
      }

#endif /// (NTDDI_VERSION >= NTDDI_WIN8)
#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

   }

   if(transportHeaderAvailable)
   {
      BYTE*       pBuffer         = 0;
      NET_BUFFER* pNetBuffer      = NET_BUFFER_LIST_FIRST_NB(pNetBufferList);
      UINT32      bytesNeeded     = transportHeaderSize ? transportHeaderSize : NET_BUFFER_DATA_LENGTH(pNetBuffer);
      PVOID       pContiguousData = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pBuffer is expected to be cleaned up by caller using KrnlHlprTransportHeaderDestroy if *pNeedToFreeMemory is TRUE

      HLPR_NEW_ARRAY(pBuffer,
                     BYTE,
                     bytesNeeded,
                     WFPSAMPLER_SYSLIB_TAG);
      HLPR_BAIL_ON_ALLOC_FAILURE(pBuffer,
                                 status);

#pragma warning(pop)

      *pNeedToFreeMemory = TRUE;

      if(bytesAdvanced)
         NdisAdvanceNetBufferDataStart(pNetBuffer,
                                       bytesAdvanced,
                                       0,
                                       0);
      else if(bytesRetreated)
      {
         status = NdisRetreatNetBufferDataStart(pNetBuffer,
                                                bytesRetreated,
                                                0,
                                                0);
         if(status != STATUS_SUCCESS)
         {
            DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                       DPFLTR_ERROR_LEVEL,
                       " !!!! KrnlHlpTransportHeaderGet : NdisRetreatNetBufferDataStart() [status: %#x]\n",
                       status);

            HLPR_BAIL;
         }
      }

      pContiguousData = NdisGetDataBuffer(pNetBuffer,
                                          bytesNeeded,
                                          pBuffer,
                                          1,
                                          0);

      /// Return to the original offset
      if(bytesRetreated)
         NdisAdvanceNetBufferDataStart(pNetBuffer,
                                       bytesRetreated,
                                       0,
                                       0);
      else if(bytesAdvanced)
      {
         status = NdisRetreatNetBufferDataStart(pNetBuffer,
                                                bytesAdvanced,
                                                0,
                                                0);
         if(status != STATUS_SUCCESS)
         {
            DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                       DPFLTR_ERROR_LEVEL,
                       " !!!! KrnlHlprTransportHeaderGet : NdisRetreatNetBufferDataStart() [status: %#x]\n",
                       status);

            HLPR_BAIL;
         }
      }

      if(!pContiguousData)
      {
         status = STATUS_UNSUCCESSFUL;
      
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_ERROR_LEVEL,
                    " !!!! KrnlHlprTransportHeaderGet : NdisGetDataBuffer() [status: %#x]\n",
                    status);
      
         HLPR_BAIL;
      }

      if(pBuffer != pContiguousData)
      {
         HLPR_DELETE_ARRAY(pBuffer,
                           WFPSAMPLER_SYSLIB_TAG);

         *pNeedToFreeMemory = FALSE;
      }

      *ppTransportHeader = pContiguousData;

      if(pProtocol)
         *pProtocol = protocol;

      if(pDirection)
         *pDirection = direction;

      if(pTransportHeaderSize)
         *pTransportHeaderSize = transportHeaderSize;

      HLPR_BAIL_LABEL:

      if(status != STATUS_SUCCESS &&
         *pNeedToFreeMemory &&
         pBuffer)
      {
         KrnlHlprTransportHeaderDestroy((VOID**)&pBuffer);

         *pNeedToFreeMemory = FALSE;
      }
   }
   else
      status = STATUS_NO_MATCH;

   return status;
}


/**
 @kernel_helper_function="KrnlHlprTransportHeaderGet"
 
   Purpose:  Retrieve a pointer to the Transport Header from the NET_BUFFER_LIST.               <br>
                                                                                                <br>
   Notes:    Assumes the NBL is at the start of the Transport Header.                           <br>
                                                                                                <br>
             Function is overloaded.                                                            <br>
                                                                                                <br>
             If needToFreeMemory is TRUE, caller should call KrnlHlprTransportHeaderDestroy() 
                when finished  with the header.                                                 <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_When_(return != STATUS_SUCCESS, _At_(*ppTransportHeader, _Post_ _Null_))
_When_(return == STATUS_SUCCESS, _At_(*ppTransportHeader, _Post_ _Notnull_))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprTransportHeaderGet(_In_ NET_BUFFER_LIST* pNetBufferList,
                                    _Outptr_result_buffer_(transportHeaderSize) VOID** ppTransportHeader,
                                    _Inout_ BOOLEAN* pNeedToFreeMemory,
                                    _In_ UINT32 transportHeaderSize)                                      /* 0 */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprTransportHeaderGet()\n");

#endif /// DBG
   
   NT_ASSERT(pNetBufferList);
   NT_ASSERT(ppTransportHeader);
   NT_ASSERT(pNeedToFreeMemory);

   NTSTATUS     status          = STATUS_SUCCESS;
   BYTE*        pBuffer         = 0;
   NET_BUFFER*  pNetBuffer      = NET_BUFFER_LIST_FIRST_NB(pNetBufferList);
   const UINT32 BUFFER_SIZE     = transportHeaderSize ? transportHeaderSize : NET_BUFFER_DATA_LENGTH(pNetBuffer);
   UINT32       bytesNeeded     = transportHeaderSize ? transportHeaderSize : NET_BUFFER_DATA_LENGTH(pNetBuffer);
   PVOID        pContiguousData = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pBuffer is expected to be cleaned up by caller using KrnlHlprTransportHeaderDestroy if *pNeedToFreeMemory is TRUE

   HLPR_NEW_ARRAY(pBuffer,
                  BYTE,
                  BUFFER_SIZE,
                  WFPSAMPLER_SYSLIB_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pBuffer,
                              status);

#pragma warning(pop)

   *pNeedToFreeMemory = TRUE;

   pContiguousData = NdisGetDataBuffer(pNetBuffer,
                                       bytesNeeded,
                                       pBuffer,
                                       1,
                                       0);
   if(!pContiguousData)
   {
      status = STATUS_UNSUCCESSFUL;

      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! KrnlHlprTransportHeaderGet : NdisGetDataBuffer() [status: %#x]\n",
                 status);

      HLPR_BAIL;
   }

   if(pBuffer != pContiguousData)
   {
      HLPR_DELETE_ARRAY(pBuffer,
                        WFPSAMPLER_SYSLIB_TAG);

      *pNeedToFreeMemory = FALSE;
   }

   *ppTransportHeader = pContiguousData;

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS &&
      *pNeedToFreeMemory &&
      pBuffer)
   {
      KrnlHlprTransportHeaderDestroy((VOID**)&pBuffer);

      *pNeedToFreeMemory = FALSE;
   }

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprTransportHeaderGet() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}

/**
 @kernel_helper_function="KrnlHlprTransportHeaderGetSourcePortField"
 
   Purpose:  Retrieve the source port from the Transport header.                                <br>
                                                                                                <br>
   Notes:    Assumes the NBL is at the start of the Transport Header.                           <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
UINT16 KrnlHlprTransportHeaderGetSourcePortField(_In_ NET_BUFFER_LIST* pNetBufferList,
                                                 _In_ IPPROTO protocol)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprTransportHeaderGetSourcePortField()\n");

#endif /// DBG

   NT_ASSERT(pNetBufferList);

   NTSTATUS status           = STATUS_SUCCESS;
   BYTE*    pTransportHeader = 0;
   BOOLEAN  needToFree       = FALSE;
   UINT16   port             = 0;

   status = KrnlHlprTransportHeaderGet(pNetBufferList,
                                       (VOID**)&pTransportHeader,
                                       &needToFree);
   HLPR_BAIL_ON_FAILURE(status);

   switch(protocol)
   {
      case TCP:
      {
         TCP_HEADER* pTCPHeader = (TCP_HEADER*)pTransportHeader;

         port = pTCPHeader->sourcePort;

         break;
      }
      case UDP:
      {
         UDP_HEADER* pUDPHeader = (UDP_HEADER*)pTransportHeader;

         port = pUDPHeader->sourcePort;

         break;
      }
   }

   if(needToFree)
   {
      /// Copy the contents of the allocated buffer to the NBL's discontiguous buffer
      if(status == STATUS_SUCCESS)
      {
         NET_BUFFER* pFirstNetBuffer  = NET_BUFFER_LIST_FIRST_NB(pNetBufferList);
         PMDL        pCurrentMDL      = NET_BUFFER_CURRENT_MDL(pFirstNetBuffer);
         SIZE_T      currentMDLOffset = NET_BUFFER_CURRENT_MDL_OFFSET(pFirstNetBuffer);
         SIZE_T      headerSize       = 0;
         SIZE_T      bytesCopied      = 0;

         switch(protocol)
         {
            case TCP:
            {
               headerSize = TCP_HEADER_MIN_SIZE;

               break;
            }
            case UDP:
            {
               headerSize = UDP_HEADER_MIN_SIZE;

               break;
            }
         }

         status = PrvKrnlHlprCopyBufferToMDL(pTransportHeader,
                                             pCurrentMDL,
                                             currentMDLOffset,
                                             headerSize,
                                             &bytesCopied);
         if(status == STATUS_SUCCESS &&
            bytesCopied != headerSize)
            status = STATUS_INSUFFICIENT_RESOURCES;
      }

      KrnlHlprTransportHeaderDestroy((VOID**)&pTransportHeader);
   }

   HLPR_BAIL_LABEL:

#if DBG

      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 " <--- KrnlHlprTransportHeaderGetSourcePortField()\n");

#endif /// DBG

   return port;
}

/**
 @kernel_helper_function="KrnlHlprTransportHeaderGetDestinationPortField"
 
   Purpose:  Retrieve the destination port from the Transport header.                           <br>
                                                                                                <br>
   Notes:    Assumes the NBL is at the start of the Transport Header.                           <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
UINT16 KrnlHlprTransportHeaderGetDestinationPortField(_In_ NET_BUFFER_LIST* pNetBufferList,
                                                      _In_ IPPROTO protocol)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprTransportHeaderGetDestinationPortField()\n");

#endif /// DBG

   NT_ASSERT(pNetBufferList);

   NTSTATUS status           = STATUS_SUCCESS;
   BYTE*    pTransportHeader = 0;
   BOOLEAN  needToFree       = FALSE;
   UINT16   port             = 0;

   status = KrnlHlprTransportHeaderGet(pNetBufferList,
                                       (VOID**)&pTransportHeader,
                                       &needToFree);
   HLPR_BAIL_ON_FAILURE(status);

   switch(protocol)
   {
      case TCP:
      {
         TCP_HEADER* pTCPHeader = (TCP_HEADER*)pTransportHeader;

         port = pTCPHeader->destinationPort;

         break;
      }
      case UDP:
      {
         UDP_HEADER* pUDPHeader = (UDP_HEADER*)pTransportHeader;

         port = pUDPHeader->destinationPort;

         break;
      }
   }

   if(needToFree)
   {
      /// Copy the contents of the allocated buffer to the NBL's discontiguous buffer
      if(status == STATUS_SUCCESS)
      {
         NET_BUFFER* pFirstNetBuffer  = NET_BUFFER_LIST_FIRST_NB(pNetBufferList);
         PMDL        pCurrentMDL      = NET_BUFFER_CURRENT_MDL(pFirstNetBuffer);
         SIZE_T      currentMDLOffset = NET_BUFFER_CURRENT_MDL_OFFSET(pFirstNetBuffer);
         SIZE_T      headerSize       = 0;
         SIZE_T      bytesCopied      = 0;

         switch(protocol)
         {
            case TCP:
            {
               headerSize = TCP_HEADER_MIN_SIZE;

               break;
            }
            case UDP:
            {
               headerSize = UDP_HEADER_MIN_SIZE;

               break;
            }
         }

         status = PrvKrnlHlprCopyBufferToMDL(pTransportHeader,
                                             pCurrentMDL,
                                             currentMDLOffset,
                                             headerSize,
                                             &bytesCopied);
         if(status == STATUS_SUCCESS &&
            bytesCopied != headerSize)
            status = STATUS_INSUFFICIENT_RESOURCES;
      }

      KrnlHlprTransportHeaderDestroy((VOID**)&pTransportHeader);
   }

   HLPR_BAIL_LABEL:

#if DBG

      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 " <--- KrnlHlprTransportHeaderGetDestinationPortField()\n");

#endif /// DBG

   return port;
}

#endif /// TRANSPORT_HEADERS____

#ifndef ICMPV4_HEADER____
#define ICMPV4_HEADER____

/**
 @kernel_helper_function="KrnlHlprICMPv4HeaderGet"
 
   Purpose:  Retrieve a pointer to the ICMPv4 Header from the NET_BUFFER_LIST.                  <br>
                                                                                                <br>
   Notes:    Assumes the NBL is at the start of the ICMPv4 Header.                              <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_When_(return != STATUS_SUCCESS, _At_(*ppICMPv4Header, _Post_ _Null_))
_When_(return == STATUS_SUCCESS, _At_(*ppICMPv4Header, _Post_ _Notnull_))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprICMPv4HeaderGet(_In_ NET_BUFFER_LIST* pNetBufferList,
                                 _Outptr_result_buffer_(icmpHeaderSize) VOID** ppICMPv4Header,
                                 _Inout_ BOOLEAN* pNeedToFreeMemory,
                                 _In_ UINT32 icmpHeaderSize)                                   /* 0 */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprICMPv4HeaderGet()\n");

#endif /// DBG

   NTSTATUS     status          = STATUS_SUCCESS;
   BYTE*        pBuffer         = 0;
   NET_BUFFER*  pNetBuffer      = NET_BUFFER_LIST_FIRST_NB(pNetBufferList);
   const UINT32 BUFFER_SIZE     = icmpHeaderSize ? icmpHeaderSize : NET_BUFFER_DATA_LENGTH(pNetBuffer);
   UINT32       bytesNeeded     = icmpHeaderSize ? icmpHeaderSize : NET_BUFFER_DATA_LENGTH(pNetBuffer);
   PVOID        pContiguousData = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pBuffer is expected to be cleaned up by caller using KrnlHlprTransportHeaderDestroy if *pNeedToFreeMemory is TRUE

   HLPR_NEW_ARRAY(pBuffer,
                  BYTE,
                  BUFFER_SIZE,
                  WFPSAMPLER_SYSLIB_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pBuffer,
                              status);

#pragma warning(pop)

   *pNeedToFreeMemory = TRUE;

   pContiguousData = NdisGetDataBuffer(pNetBuffer,
                                       bytesNeeded,
                                       pBuffer,
                                       1,
                                       0);
   if(!pContiguousData)
   {
      status = STATUS_UNSUCCESSFUL;

      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! KrnlHlprICMPv4HeaderGet : NdisGetDataBuffer() [status: %#x]\n",
                 status);

      HLPR_BAIL;
   }

   if(pBuffer != pContiguousData)
   {
      HLPR_DELETE_ARRAY(pBuffer,
                        WFPSAMPLER_SYSLIB_TAG);

      *pNeedToFreeMemory = FALSE;
   }

   *ppICMPv4Header = pContiguousData;

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS &&
      *pNeedToFreeMemory &&
      pBuffer)
   {
      KrnlHlprTransportHeaderDestroy((VOID**)&pBuffer);

      *pNeedToFreeMemory = FALSE;
   }

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprICMPv4HeaderGet() [status: %#x]\n",
              status);
   
#endif /// DBG

   return status;
}

/**
 @kernel_helper_function="KrnlHlprICMPv4HeaderModifyType"
 
   Purpose:  Set the ICMP Type field in the ICMPv4 Header to the provided value.                <br>
                                                                                                <br>
   Notes:    The NetBufferList parameter is expected to be offset to the start of the ICMPv4 
             Header.                                                                            <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprICMPv4HeaderModifyType(_In_ const FWP_VALUE* pValue,
                                        _Inout_ NET_BUFFER_LIST* pNetBufferList,
                                        _In_ UINT32 icmpHeaderSize)              /* 0 */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprICMPv4HeaderModifyType()\n");

#endif /// DBG
   
   NT_ASSERT(pValue);
   NT_ASSERT(pNetBufferList);

   NTSTATUS        status      = STATUS_SUCCESS;
   ICMP_HEADER_V4* pICMPHeader = 0;
   BOOLEAN         needToFree  = FALSE;

   status = KrnlHlprICMPv4HeaderGet(pNetBufferList,
                                    (VOID**)&pICMPHeader,
                                    &needToFree,
                                    icmpHeaderSize);
   HLPR_BAIL_ON_FAILURE(status);

   pICMPHeader->type = pValue->uint8;

   HLPR_BAIL_LABEL:

   if(needToFree)
   {
      /// Copy the contents of the allocated buffer to the NBL's discontiguous buffer
      if(status == STATUS_SUCCESS)
      {
         NET_BUFFER* pFirstNetBuffer  = NET_BUFFER_LIST_FIRST_NB(pNetBufferList);
         PMDL        pCurrentMDL      = NET_BUFFER_CURRENT_MDL(pFirstNetBuffer);
         SIZE_T      currentMDLOffset = NET_BUFFER_CURRENT_MDL_OFFSET(pFirstNetBuffer);
         SIZE_T      headerSize       = icmpHeaderSize ? icmpHeaderSize : ICMP_HEADER_MIN_SIZE;
         SIZE_T      bytesCopied      = 0;

         status = PrvKrnlHlprCopyBufferToMDL((BYTE*)pICMPHeader,
                                             pCurrentMDL,
                                             currentMDLOffset,
                                             headerSize,
                                             &bytesCopied);
         if(status == STATUS_SUCCESS &&
            bytesCopied != headerSize)
            status = STATUS_INSUFFICIENT_RESOURCES;
      }

      KrnlHlprTransportHeaderDestroy((VOID**)&pICMPHeader);
   }

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprICMPv4HeaderModifyType() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}

/**
 @kernel_helper_function="KrnlHlprICMPv4HeaderModifyCode"
 
   Purpose:  Set the ICMP Code field in the ICMPv4 Header to the provided value.                <br>
                                                                                                <br>
   Notes:    The NetBufferList parameter is expected to be offset to the start of the ICMPv4 
             Header.                                                                            <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprICMPv4HeaderModifyCode(_In_ const FWP_VALUE* pValue,
                                        _Inout_ NET_BUFFER_LIST* pNetBufferList,
                                        _In_ UINT32 icmpHeaderSize)              /* 0 */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprICMPv4HeaderModifyCode()\n");

#endif /// DBG
   
   NT_ASSERT(pValue);
   NT_ASSERT(pNetBufferList);

   NTSTATUS        status      = STATUS_SUCCESS;
   ICMP_HEADER_V4* pICMPHeader = 0;
   BOOLEAN         needToFree  = FALSE;

   status = KrnlHlprICMPv4HeaderGet(pNetBufferList,
                                    (VOID**)&pICMPHeader,
                                    &needToFree,
                                    icmpHeaderSize);
   HLPR_BAIL_ON_FAILURE(status);

   pICMPHeader->code = pValue->uint8;

   HLPR_BAIL_LABEL:

   if(needToFree)
   {
      /// Copy the contents of the allocated buffer to the NBL's discontiguous buffer
      if(status == STATUS_SUCCESS)
      {
         NET_BUFFER* pFirstNetBuffer  = NET_BUFFER_LIST_FIRST_NB(pNetBufferList);
         PMDL        pCurrentMDL      = NET_BUFFER_CURRENT_MDL(pFirstNetBuffer);
         SIZE_T      currentMDLOffset = NET_BUFFER_CURRENT_MDL_OFFSET(pFirstNetBuffer);
         SIZE_T      headerSize       = icmpHeaderSize ? icmpHeaderSize : ICMP_HEADER_MIN_SIZE;
         SIZE_T      bytesCopied      = 0;

         status = PrvKrnlHlprCopyBufferToMDL((BYTE*)pICMPHeader,
                                             pCurrentMDL,
                                             currentMDLOffset,
                                             headerSize,
                                             &bytesCopied);
         if(status == STATUS_SUCCESS &&
            bytesCopied != headerSize)
            status = STATUS_INSUFFICIENT_RESOURCES;
      }

      KrnlHlprTransportHeaderDestroy((VOID**)&pICMPHeader);
   }

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprICMPv4HeaderModifyCode() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}

#endif /// ICMPV4_HEADER____

#ifndef ICMPV6_HEADER____
#define ICMPV6_HEADER____

/**
 @kernel_helper_function="KrnlHlprICMPv6HeaderGet"
 
   Purpose:  Retrieve a pointer to the ICMPv6 Header from the NET_BUFFER_LIST.                  <br>
                                                                                                <br>
   Notes:    Assumes the NBL is at the start of the ICMPv6 Header.                              <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_When_(return != STATUS_SUCCESS, _At_(*ppICMPv6Header, _Post_ _Null_))
_When_(return == STATUS_SUCCESS, _At_(*ppICMPv6Header, _Post_ _Notnull_))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprICMPv6HeaderGet(_In_ NET_BUFFER_LIST* pNetBufferList,
                                 _Outptr_result_buffer_(icmpHeaderSize) VOID** ppICMPv6Header,
                                 _Inout_ BOOLEAN* pNeedToFreeMemory,
                                 _In_ UINT32 icmpHeaderSize)                                   /* 0 */
{
#if DBG
      
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprICMPv6HeaderGet()\n");
   
#endif /// DBG

   NTSTATUS     status          = STATUS_SUCCESS;
   BYTE*        pBuffer         = 0;
   NET_BUFFER*  pNetBuffer      = NET_BUFFER_LIST_FIRST_NB(pNetBufferList);
   const UINT32 BUFFER_SIZE     = icmpHeaderSize ? icmpHeaderSize : NET_BUFFER_DATA_LENGTH(pNetBuffer);
   UINT32       bytesNeeded     = icmpHeaderSize ? icmpHeaderSize : NET_BUFFER_DATA_LENGTH(pNetBuffer);
   PVOID        pContiguousData = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pBuffer is expected to be cleaned up by caller using KrnlHlprTransportHeaderDestroy if *pNeedToFreeMemory is TRUE

   HLPR_NEW_ARRAY(pBuffer,
                  BYTE,
                  BUFFER_SIZE,
                  WFPSAMPLER_SYSLIB_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pBuffer,
                              status);

#pragma warning(pop)

   *pNeedToFreeMemory = TRUE;

   pContiguousData = NdisGetDataBuffer(pNetBuffer,
                                       bytesNeeded,
                                       pBuffer,
                                       1,
                                       0);
   if(!pContiguousData)
   {
      status = STATUS_UNSUCCESSFUL;

      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! KrnlHlprICMPv6HeaderGet : NdisGetDataBuffer() [status: %#x]\n",
                 status);

      HLPR_BAIL;
   }

   if(pBuffer != pContiguousData)
   {
      HLPR_DELETE_ARRAY(pBuffer,
                        WFPSAMPLER_SYSLIB_TAG);

      *pNeedToFreeMemory = FALSE;
   }

   *ppICMPv6Header = pContiguousData;

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS &&
      *pNeedToFreeMemory &&
      pBuffer)
   {
      KrnlHlprTransportHeaderDestroy((VOID**)&pBuffer);

      *pNeedToFreeMemory = FALSE;
   }

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprICMPv4HeaderGet() [status: %#x]\n",
              status);
      
#endif /// DBG

   return status;
}

/**
 @kernel_helper_function="KrnlHlprICMPv6HeaderModifyType"
 
   Purpose:  Set the ICMP Type field in the ICMPv6 Header to the provided value.                <br>
                                                                                                <br>
   Notes:    The NetBufferList parameter is expected to be offset to the start of the ICMPv6 
             Header.                                                                            <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprICMPv6HeaderModifyType(_In_ const FWP_VALUE* pValue,
                                        _Inout_ NET_BUFFER_LIST* pNetBufferList,
                                        _In_ UINT32 icmpHeaderSize)              /* 0 */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprICMPv6HeaderModifyType()\n");

#endif /// DBG
   
   NT_ASSERT(pValue);
   NT_ASSERT(pNetBufferList);

   NTSTATUS        status      = STATUS_SUCCESS;
   ICMP_HEADER_V6* pICMPHeader = 0;
   BOOLEAN         needToFree  = FALSE;

   status = KrnlHlprICMPv4HeaderGet(pNetBufferList,
                                    (VOID**)&pICMPHeader,
                                    &needToFree,
                                    icmpHeaderSize);
   HLPR_BAIL_ON_FAILURE(status);

   pICMPHeader->type = pValue->uint8;

   HLPR_BAIL_LABEL:

   if(needToFree)
   {
      /// Copy the contents of the allocated buffer to the NBL's discontiguous buffer
      if(status == STATUS_SUCCESS)
      {
         NET_BUFFER* pFirstNetBuffer  = NET_BUFFER_LIST_FIRST_NB(pNetBufferList);
         PMDL        pCurrentMDL      = NET_BUFFER_CURRENT_MDL(pFirstNetBuffer);
         SIZE_T      currentMDLOffset = NET_BUFFER_CURRENT_MDL_OFFSET(pFirstNetBuffer);
         SIZE_T      headerSize       = icmpHeaderSize ? icmpHeaderSize : ICMP_HEADER_MIN_SIZE;
         SIZE_T      bytesCopied      = 0;

         status = PrvKrnlHlprCopyBufferToMDL((BYTE*)pICMPHeader,
                                             pCurrentMDL,
                                             currentMDLOffset,
                                             headerSize,
                                             &bytesCopied);
         if(status == STATUS_SUCCESS &&
            bytesCopied != headerSize)
            status = STATUS_INSUFFICIENT_RESOURCES;
      }

      KrnlHlprTransportHeaderDestroy((VOID**)&pICMPHeader);
   }

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprICMPv6HeaderModifyType() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}

/**
 @kernel_helper_function="KrnlHlprICMPv6HeaderModifyCode"
 
   Purpose:  Set the ICMP Code field in the ICMPv6 Header to the provided value.                <br>
                                                                                                <br>
   Notes:    The NetBufferList parameter is expected to be offset to the start of the ICMPv6 
             Header.                                                                            <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprICMPv6HeaderModifyCode(_In_ const FWP_VALUE* pValue,
                                        _Inout_ NET_BUFFER_LIST* pNetBufferList,
                                        _In_ UINT32 icmpHeaderSize)              /* 0 */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprICMPv6HeaderModifyCode()\n");

#endif /// DBG
   
   NT_ASSERT(pValue);
   NT_ASSERT(pNetBufferList);

   NTSTATUS        status      = STATUS_SUCCESS;
   ICMP_HEADER_V6* pICMPHeader = 0;
   BOOLEAN         needToFree  = FALSE;

   status = KrnlHlprICMPv4HeaderGet(pNetBufferList,
                                    (VOID**)&pICMPHeader,
                                    &needToFree,
                                    icmpHeaderSize);
   HLPR_BAIL_ON_FAILURE(status);

   pICMPHeader->code = pValue->uint8;

   HLPR_BAIL_LABEL:

   if(needToFree)
   {
      /// Copy the contents of the allocated buffer to the NBL's discontiguous buffer
      if(status == STATUS_SUCCESS)
      {
         NET_BUFFER* pFirstNetBuffer  = NET_BUFFER_LIST_FIRST_NB(pNetBufferList);
         PMDL        pCurrentMDL      = NET_BUFFER_CURRENT_MDL(pFirstNetBuffer);
         SIZE_T      currentMDLOffset = NET_BUFFER_CURRENT_MDL_OFFSET(pFirstNetBuffer);
         SIZE_T      headerSize       = icmpHeaderSize ? icmpHeaderSize : ICMP_HEADER_MIN_SIZE;
         SIZE_T      bytesCopied      = 0;

         status = PrvKrnlHlprCopyBufferToMDL((BYTE*)pICMPHeader,
                                             pCurrentMDL,
                                             currentMDLOffset,
                                             headerSize,
                                             &bytesCopied);
         if(status == STATUS_SUCCESS &&
            bytesCopied != headerSize)
            status = STATUS_INSUFFICIENT_RESOURCES;
      }

      KrnlHlprTransportHeaderDestroy((VOID**)&pICMPHeader);
   }

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprICMPv6HeaderModifyCode() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}

#endif /// ICMPV6_HEADER____

#ifndef TCP_HEADER____
#define TCP_HEADER____

/**
 @kernel_helper_function="KrnlHlprTCPHeaderGet"
 
   Purpose:  Retrieve a pointer to the TCP Header from the NET_BUFFER_LIST.                     <br>
                                                                                                <br>
   Notes:    Assumes the NBL is at the start of the TCP Header.                                 <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_When_(return != STATUS_SUCCESS, _At_(*ppTCPHeader, _Post_ _Null_))
_When_(return == STATUS_SUCCESS, _At_(*ppTCPHeader, _Post_ _Notnull_))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprTCPHeaderGet(_In_ NET_BUFFER_LIST* pNetBufferList,
                              _Outptr_result_buffer_(tcpHeaderSize) VOID** ppTCPHeader,
                              _Inout_ BOOLEAN* pNeedToFreeMemory,
                              _In_ UINT32 tcpHeaderSize)                                /* 0 */
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprTCPHeaderGet()\n");
      
#endif /// DBG

   NTSTATUS     status          = STATUS_SUCCESS;
   BYTE*        pBuffer         = 0;
   NET_BUFFER*  pNetBuffer      = NET_BUFFER_LIST_FIRST_NB(pNetBufferList);
   const UINT32 BUFFER_SIZE     = tcpHeaderSize ? tcpHeaderSize : NET_BUFFER_DATA_LENGTH(pNetBuffer);
   UINT32       bytesNeeded     = tcpHeaderSize ? tcpHeaderSize : NET_BUFFER_DATA_LENGTH(pNetBuffer);
   PVOID        pContiguousData = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pBuffer is expected to be cleaned up by caller using KrnlHlprTransportHeaderDestroy if *pNeedToFreeMemory is TRUE

   HLPR_NEW_ARRAY(pBuffer,
                  BYTE,
                  BUFFER_SIZE,
                  WFPSAMPLER_SYSLIB_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pBuffer,
                              status);

#pragma warning(pop)

   *pNeedToFreeMemory = TRUE;

   pContiguousData = NdisGetDataBuffer(pNetBuffer,
                                       bytesNeeded,
                                       pBuffer,
                                       1,
                                       0);
   if(!pContiguousData)
   {
      status = STATUS_UNSUCCESSFUL;

      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! KrnlHlprTCPHeaderGet : NdisGetDataBuffer() [status: %#x]\n",
                 status);

      HLPR_BAIL;
   }

   if(pBuffer != pContiguousData)
   {
      HLPR_DELETE_ARRAY(pBuffer,
                        WFPSAMPLER_SYSLIB_TAG);

      *pNeedToFreeMemory = FALSE;
   }

   *ppTCPHeader = pContiguousData;

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS &&
      *pNeedToFreeMemory &&
      pBuffer)
   {
      KrnlHlprTransportHeaderDestroy((VOID**)&pBuffer);

      *pNeedToFreeMemory = FALSE;
   }

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprTCPHeaderGet() [status: %#x]\n",
              status);
         
#endif /// DBG

   return status;
}

/**
 @kernel_helper_function="KrnlHlprTCPHeaderModifySourcePort"
 
   Purpose:  Set the Source Address field in the TCP Header to the provided value.              <br>
                                                                                                <br>
   Notes:    The NetBufferList parameter is expected to be offset to the start of the TCP 
             Header.                                                                            <br>
                                                                                                <br>
             Values should be in Network Byte Order.                                            <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprTCPHeaderModifySourcePort(_In_ const FWP_VALUE* pValue,
                                           _Inout_ NET_BUFFER_LIST* pNetBufferList,
                                           _In_ UINT32 tcpHeaderSize,               /* 0 */
                                           _In_ BOOLEAN convertByteOrder)           /* FALSE */
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprTCPHeaderModifySourcePort()\n");

#endif /// DBG
   
   NT_ASSERT(pValue);
   NT_ASSERT(pNetBufferList);

   NTSTATUS    status     = STATUS_SUCCESS;
   TCP_HEADER* pTCPHeader = 0;
   BOOLEAN     needToFree = FALSE;
   UINT16      port       = convertByteOrder ? htons(pValue->uint16) : pValue->uint16;

   status = KrnlHlprTCPHeaderGet(pNetBufferList,
                                 (VOID**)&pTCPHeader,
                                 &needToFree,
                                 tcpHeaderSize);
   HLPR_BAIL_ON_FAILURE(status);

   pTCPHeader->sourcePort = port;

   HLPR_BAIL_LABEL:

   if(needToFree)
   {
      /// Copy the contents of the allocated buffer to the NBL's discontiguous buffer
      if(status == STATUS_SUCCESS)
      {
         NET_BUFFER* pFirstNetBuffer  = NET_BUFFER_LIST_FIRST_NB(pNetBufferList);
         PMDL        pCurrentMDL      = NET_BUFFER_CURRENT_MDL(pFirstNetBuffer);
         SIZE_T      currentMDLOffset = NET_BUFFER_CURRENT_MDL_OFFSET(pFirstNetBuffer);
         SIZE_T      headerSize       = tcpHeaderSize ? tcpHeaderSize : TCP_HEADER_MIN_SIZE;
         SIZE_T      bytesCopied      = 0;

         status = PrvKrnlHlprCopyBufferToMDL((BYTE*)pTCPHeader,
                                             pCurrentMDL,
                                             currentMDLOffset,
                                             headerSize,
                                             &bytesCopied);
         if(status == STATUS_SUCCESS &&
            bytesCopied != headerSize)
            status = STATUS_INSUFFICIENT_RESOURCES;
      }

      KrnlHlprTransportHeaderDestroy((VOID**)&pTCPHeader);
   }

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprTCPHeaderModifySourcePort() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}

/**
 @kernel_helper_function="KrnlHlprTCPHeaderModifyDestinationPort"
 
   Purpose:  Set the Destination Address field in the TCP Header to the provided value.         <br>
                                                                                                <br>
   Notes:    The NetBufferList parameter is expected to be offset to the start of the TCP 
             Header.                                                                            <br>
                                                                                                <br>
             Values should be in Network Byte Order.                                            <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprTCPHeaderModifyDestinationPort(_In_ const FWP_VALUE* pValue,
                                                _Inout_ NET_BUFFER_LIST* pNetBufferList,
                                                _In_ UINT32 tcpHeaderSize,               /* 0 */
                                                _In_ BOOLEAN convertByteOrder)           /* FALSE */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprTCPHeaderModifyDestinationPort()\n");

#endif /// DBG
   
   NT_ASSERT(pValue);
   NT_ASSERT(pNetBufferList);

   NTSTATUS    status     = STATUS_SUCCESS;
   TCP_HEADER* pTCPHeader = 0;
   BOOLEAN     needToFree = FALSE;
   UINT16      port       = convertByteOrder ? htons(pValue->uint16) : pValue->uint16;

   status = KrnlHlprTCPHeaderGet(pNetBufferList,
                                 (VOID**)&pTCPHeader,
                                 &needToFree,
                                 tcpHeaderSize);
   HLPR_BAIL_ON_FAILURE(status);

   pTCPHeader->destinationPort = port;

   HLPR_BAIL_LABEL:

   if(needToFree)
   {
      /// Copy the contents of the allocated buffer to the NBL's discontiguous buffer
      if(status == STATUS_SUCCESS)
      {
         NET_BUFFER* pFirstNetBuffer  = NET_BUFFER_LIST_FIRST_NB(pNetBufferList);
         PMDL        pCurrentMDL      = NET_BUFFER_CURRENT_MDL(pFirstNetBuffer);
         SIZE_T      currentMDLOffset = NET_BUFFER_CURRENT_MDL_OFFSET(pFirstNetBuffer);
         SIZE_T      headerSize       = tcpHeaderSize ? tcpHeaderSize : TCP_HEADER_MIN_SIZE;
         SIZE_T      bytesCopied      = 0;

         status = PrvKrnlHlprCopyBufferToMDL((BYTE*)pTCPHeader,
                                             pCurrentMDL,
                                             currentMDLOffset,
                                             headerSize,
                                             &bytesCopied);
         if(status == STATUS_SUCCESS &&
            bytesCopied != headerSize)
            status = STATUS_INSUFFICIENT_RESOURCES;
      }

      KrnlHlprTransportHeaderDestroy((VOID**)&pTCPHeader);
   }

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprTCPHeaderModifyDestinationPort() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}

#endif /// TCP_HEADER____

#ifndef UDP_HEADER____
#define UDP_HEADER____

/**
 @kernel_helper_function="KrnlHlprUDPHeaderGet"
 
   Purpose:  Retrieve a pointer to the UDP Header from the NET_BUFFER_LIST.                     <br>
                                                                                                <br>
   Notes:    Assumes the NBL is at the start of the UDP Header.                                 <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_When_(return != STATUS_SUCCESS, _At_(*ppUDPHeader, _Post_ _Null_))
_When_(return == STATUS_SUCCESS, _At_(*ppUDPHeader, _Post_ _Notnull_))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprUDPHeaderGet(_In_ NET_BUFFER_LIST* pNetBufferList,
                              _Outptr_result_buffer_(udpHeaderSize) VOID** ppUDPHeader,
                              _Inout_ BOOLEAN* pNeedToFreeMemory,
                              _In_ UINT32 udpHeaderSize)                                /* 0 */
{
#if DBG
            
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprUDPHeaderGet()\n");
         
#endif /// DBG

   NTSTATUS     status          = STATUS_SUCCESS;
   BYTE*        pBuffer         = 0;
   NET_BUFFER*  pNetBuffer      = NET_BUFFER_LIST_FIRST_NB(pNetBufferList);
   const UINT32 BUFFER_SIZE     = udpHeaderSize ? udpHeaderSize : NET_BUFFER_DATA_LENGTH(pNetBuffer);
   UINT32       bytesNeeded     = udpHeaderSize ? udpHeaderSize : NET_BUFFER_DATA_LENGTH(pNetBuffer);
   PVOID        pContiguousData = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pBuffer is expected to be cleaned up by caller using KrnlHlprTransportHeaderDestroy if *pNeedToFreeMemory is TRUE

   HLPR_NEW_ARRAY(pBuffer,
                  BYTE,
                  BUFFER_SIZE,
                  WFPSAMPLER_SYSLIB_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pBuffer,
                              status);

#pragma warning(pop)

   *pNeedToFreeMemory = TRUE;

   pContiguousData = NdisGetDataBuffer(pNetBuffer,
                                       bytesNeeded,
                                       pBuffer,
                                       1,
                                       0);
   if(!pContiguousData)
   {
      status = STATUS_UNSUCCESSFUL;

      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! KrnlHlprUDPHeaderGet : NdisGetDataBuffer() [status: %#x]\n",
                 status);

      HLPR_BAIL;
   }

   if(pBuffer != pContiguousData)
   {
      HLPR_DELETE_ARRAY(pBuffer,
                        WFPSAMPLER_SYSLIB_TAG);

      *pNeedToFreeMemory = FALSE;
   }

   *ppUDPHeader = pContiguousData;

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS &&
      *pNeedToFreeMemory &&
      pBuffer)
   {
      KrnlHlprTransportHeaderDestroy((VOID**)&pBuffer);

      *pNeedToFreeMemory = FALSE;
   }

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprUDPHeaderGet() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

/**
 @kernel_helper_function="KrnlHlprUDPHeaderModifySourcePort"
 
   Purpose:  Set the Source Address field in the UDP Header to the provided value.              <br>
                                                                                                <br>
   Notes:    The NetBufferList parameter is expected to be offset to the start of the UDP 
             Header.                                                                            <br>
                                                                                                <br>
             Values should be in Network Byte Order.                                            <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprUDPHeaderModifySourcePort(_In_ const FWP_VALUE* pValue,
                                           _Inout_ NET_BUFFER_LIST* pNetBufferList,
                                           _In_ UINT32 udpHeaderSize,               /* 0 */
                                           _In_ BOOLEAN convertByteOrder)           /* FALSE */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprUDPHeaderModifySourcePort()\n");

#endif /// DBG
   
   NT_ASSERT(pValue);
   NT_ASSERT(pNetBufferList);

   NTSTATUS    status     = STATUS_SUCCESS;
   UDP_HEADER* pUDPHeader = 0;
   BOOLEAN     needToFree = FALSE;
   UINT16      port       = convertByteOrder ? htons(pValue->uint16) : pValue->uint16;

   status = KrnlHlprUDPHeaderGet(pNetBufferList,
                                 (VOID**)&pUDPHeader,
                                 &needToFree,
                                 udpHeaderSize);
   HLPR_BAIL_ON_FAILURE(status);

   pUDPHeader->sourcePort = port;

   HLPR_BAIL_LABEL:

   if(needToFree)
   {
      /// Copy the contents of the allocated buffer to the NBL's discontiguous buffer
      if(status == STATUS_SUCCESS)
      {
         NET_BUFFER* pFirstNetBuffer  = NET_BUFFER_LIST_FIRST_NB(pNetBufferList);
         PMDL        pCurrentMDL      = NET_BUFFER_CURRENT_MDL(pFirstNetBuffer);
         SIZE_T      currentMDLOffset = NET_BUFFER_CURRENT_MDL_OFFSET(pFirstNetBuffer);
         SIZE_T      headerSize       = udpHeaderSize ? udpHeaderSize : UDP_HEADER_MIN_SIZE;
         SIZE_T      bytesCopied      = 0;

         status = PrvKrnlHlprCopyBufferToMDL((BYTE*)pUDPHeader,
                                             pCurrentMDL,
                                             currentMDLOffset,
                                             headerSize,
                                             &bytesCopied);
         if(status == STATUS_SUCCESS &&
            bytesCopied != headerSize)
            status = STATUS_INSUFFICIENT_RESOURCES;
      }

      KrnlHlprTransportHeaderDestroy((VOID**)&pUDPHeader);
   }

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprUDPHeaderModifySourcePort() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}

/**
 @kernel_helper_function="KrnlHlprUDPHeaderModifyDestinationPort"
 
   Purpose:  Set the Destination Address field in the UDP Header to the provided value.         <br>
                                                                                                <br>
   Notes:    The NetBufferList parameter is expected to be offset to the start of the UDP 
             Header.                                                                            <br>
                                                                                                <br>
             Values should be in Network Byte Order.                                            <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprUDPHeaderModifyDestinationPort(_In_ const FWP_VALUE* pValue,
                                                _Inout_ NET_BUFFER_LIST* pNetBufferList,
                                                _In_ UINT32 udpHeaderSize,               /* 0 */
                                                _In_ BOOLEAN convertByteOrder)           /* FALSE */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprUDPHeaderModifyDestinationPort()\n");

#endif /// DBG
   
   NT_ASSERT(pValue);
   NT_ASSERT(pNetBufferList);

   NTSTATUS    status     = STATUS_SUCCESS;
   UDP_HEADER* pUDPHeader = 0;
   BOOLEAN     needToFree = FALSE;
   UINT16      port       = convertByteOrder ? htons(pValue->uint16) : pValue->uint16;

   status = KrnlHlprUDPHeaderGet(pNetBufferList,
                                 (VOID**)&pUDPHeader,
                                 &needToFree,
                                 udpHeaderSize);
   HLPR_BAIL_ON_FAILURE(status);

   pUDPHeader->destinationPort = port;

   HLPR_BAIL_LABEL:

   if(needToFree)
   {
      /// Copy the contents of the allocated buffer to the NBL's discontiguous buffer
      if(status == STATUS_SUCCESS)
      {
         NET_BUFFER* pFirstNetBuffer  = NET_BUFFER_LIST_FIRST_NB(pNetBufferList);
         PMDL        pCurrentMDL      = NET_BUFFER_CURRENT_MDL(pFirstNetBuffer);
         SIZE_T      currentMDLOffset = NET_BUFFER_CURRENT_MDL_OFFSET(pFirstNetBuffer);
         SIZE_T      headerSize       = udpHeaderSize ? udpHeaderSize : UDP_HEADER_MIN_SIZE;
         SIZE_T      bytesCopied      = 0;

         status = PrvKrnlHlprCopyBufferToMDL((BYTE*)pUDPHeader,
                                             pCurrentMDL,
                                             currentMDLOffset,
                                             headerSize,
                                             &bytesCopied);
         if(status == STATUS_SUCCESS &&
            bytesCopied != headerSize)
            status = STATUS_INSUFFICIENT_RESOURCES;
      }

      KrnlHlprTransportHeaderDestroy((VOID**)&pUDPHeader);
   }

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprUDPHeaderModifyDestinationPort() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}

#endif /// UDP_HEADER____
