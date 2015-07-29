////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      Scenarios_BasicPacketModification.cpp
//
//   Abstract:
//      This module contains functions which prepares and sends data for the 
//         BASIC_PACKET_MODIFICATION scenario implementation.
//
//   Naming Convention:
//
//      <Scope><Object><Action><Modifier>
//  
//      i.e.
//
//       <Scope>
//          {
//                                            - Function is likely visible to other modules.
//            Prv                             - Function is private to this module.
//          }
//       <Object>
//          {
//            BasicPacketModificationScenario - Function pertains to Basic Packet Modification 
//                                                 Scenario.
//          }
//       <Action>
//          {
//            Execute                         - Function packages data and invokes RPC to the 
//                                                 WFPSampler service.
//            Log                             - Function writes to the console.
//            Parse                           - Function pulls data into the required format from 
//                                                 the provided data.
//          }
//       <Modifier>
//          {
//            ModificationData                - Function acts on modification data.
//            Help                            - Function provides context sensitive help for the 
//                                                 scenario.
//          }
//
//   Private Functions:
//      PrvBasicPacketModificationScenarioParseModificationData(),
//
//
//   Public Functions:
//      BasicPacketModificationScenarioExecute(),
//      BasicPacketModificationScenarioLogHelp(),
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Add support for specifying a different sublayer, 
//                                              modifying the interfaceIndex, and set the original
//                                              traffic data in the providerContext.
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Framework_WFPSampler.h" /// .

/**
 @private_function="PrvBasicPacketModificationScenarioParseModificationData"
 
   Purpose:  Parse the command line parameters for modifying packets such as:                   <br>
                Perform the injection inline (from within the classify)    (-in)                <br>
                Use threaded DPCs for out of band (asynchronous)           (-tdpc)              <br>
                Use work items for out of band (asynchronous)              (-wi)                <br>
                Modify MAC Source Address                                  (-mmsa MAC_ADDRESS)  <br>
                Modify MAC Destination Address                             (-mmda MAC_ADDRESS)  <br>
                Modify IP Source Address                                   (-misa IP_ADDRESS)   <br>
                Modify IP's Destination Address                            (-mida IP_ADDRESS)   <br>
                Modify Transport's Source Port                             (-mtsp PORT)         <br>
                Modify Transport's Destination Port                        (-mtdp PORT)         <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 PrvBasicPacketModificationScenarioParseModificationData(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                                               _In_ const UINT32 stringCount,
                                                               _In_ const GUID* pLayerKey,
                                                               _Inout_ PC_BASIC_PACKET_MODIFICATION_DATA* pPCBasicPacketModificationData)
{
   ASSERT(ppCLPStrings);
   ASSERT(stringCount);
   ASSERT(pPCBasicPacketModificationData);

   UINT32  status = NO_ERROR;

   if(HlprFwpmLayerIsIPv6(pLayerKey))
   {
      pPCBasicPacketModificationData->ipData.ipVersion         = IPV6;
      pPCBasicPacketModificationData->originalIPData.ipVersion = IPV6;
   }
   else
   {
      pPCBasicPacketModificationData->ipData.ipVersion         = IPV4;
      pPCBasicPacketModificationData->originalIPData.ipVersion = IPV4;
   }


   pPCBasicPacketModificationData->transportData.protocol         = IPPROTO_RAW;
   pPCBasicPacketModificationData->originalTransportData.protocol = IPPROTO_RAW;

   for(UINT32 stringIndex = 0;
       stringIndex < stringCount;
       stringIndex++)
   {
      /// Inline Injection
      if(HlprStringsAreEqual(L"-in",
                             ppCLPStrings[stringIndex]))
      {
         pPCBasicPacketModificationData->performInline = TRUE;

         continue;
      }

      /// Threaded DPC
      if(HlprStringsAreEqual(L"-tdpc",
                             ppCLPStrings[stringIndex]))
      {
         pPCBasicPacketModificationData->useThreadedDPC = TRUE;
      
     
         continue;
      }

      /// Work Items
      if(HlprStringsAreEqual(L"-wi",
                             ppCLPStrings[stringIndex]))
      {
         pPCBasicPacketModificationData->useWorkItems = TRUE;

         continue;
      }

      if((stringIndex + 1) < stringCount)
      {
         /// Original protocol
         if(HlprStringsAreEqual(L"-ipp",
                                ppCLPStrings[stringIndex]))
         {
            PCWSTR pString = ppCLPStrings[stringIndex + 1];

            if(iswdigit((wint_t)pString[0]))
            {
               UINT32 protocol = wcstol(pString,
                                        0,
                                        0);

               if(protocol <= 0xFF)
               {
                  pPCBasicPacketModificationData->transportData.protocol         = (UINT8)(protocol & 0xFF);
                  pPCBasicPacketModificationData->originalTransportData.protocol = (UINT8)(protocol & 0xFF);
               }
            }
            else
            {
               UINT32 protocol = IPPROTO_MAX;

               if(HlprStringsAreEqual(L"TCP",
                                      pString))
                  protocol = IPPROTO_TCP;
               else if(HlprStringsAreEqual(L"UDP",
                                           pString))
                  protocol = IPPROTO_UDP;
               else if(HlprStringsAreEqual(L"ICMPV4",
                                           pString))
                  protocol = IPPROTO_ICMP;
               else if(HlprStringsAreEqual(L"ICMPV6",
                                           pString))
                  protocol = IPPROTO_ICMPV6;

               pPCBasicPacketModificationData->transportData.protocol         = (UINT8)(protocol & 0xFF);
               pPCBasicPacketModificationData->originalTransportData.protocol = (UINT8)(protocol & 0xFF);
            }

            continue;
         }

         /// Original source port
         if(HlprStringsAreEqual(L"-ipsp",
                                ppCLPStrings[stringIndex]))
         {
            PCWSTR pString = ppCLPStrings[stringIndex + 1];

            if(iswdigit((wint_t)pString[0]))
            {
               UINT32 port = wcstol(pString,
                                    0,
                                    0);

               if(port <= 0xFFFF)
               {
                  pPCBasicPacketModificationData->originalTransportData.source.port = (UINT16)(port & 0xFFFF);
                  pPCBasicPacketModificationData->originalTransportData.source.port = htons(pPCBasicPacketModificationData->originalTransportData.source.port);
               }
            }

            continue;
         }

         /// Original destination port
         if(HlprStringsAreEqual(L"-ipdp",
                                ppCLPStrings[stringIndex]))
         {
            PCWSTR pString = ppCLPStrings[stringIndex + 1];

            if(iswdigit((wint_t)pString[0]))
            {
               UINT32 port = wcstol(pString,
                                    0,
                                    0);

               if(port <= 0xFFFF)
               {
                  pPCBasicPacketModificationData->originalTransportData.destination.port = (UINT16)(port & 0xFFFF);
                  pPCBasicPacketModificationData->originalTransportData.destination.port = htons(pPCBasicPacketModificationData->originalTransportData.destination.port);
               }
            }

            continue;
         }

         /// Modify MAC Source Address
         if(HlprStringsAreEqual(L"-mmsa",
                                ppCLPStrings[stringIndex]))
         {
            PCWSTR pString = ppCLPStrings[stringIndex + 1];

            if(HlprEthernetMACAddressStringIsValidFormat(pString))
            {
               status = HlprEthernetMACAddressStringToValue(pString,
                                                            (BYTE*)pPCBasicPacketModificationData->macData.pSourceMACAddress);
               HLPR_BAIL_ON_FAILURE(status);
            }
            else
            {
               status = ERROR_INVALID_DATA;

               HlprLogError(L"PrvBasicPacketModificationScenarioParseData() [status: %#x][source MAC Address: %s]",
                            status,
                            pString);

               HLPR_BAIL;
            }

            pPCBasicPacketModificationData->flags |= PCPMDF_MODIFY_MAC_HEADER;

            pPCBasicPacketModificationData->macData.flags |= PCPMDF_MODIFY_MAC_HEADER_SOURCE_ADDRESS;

            continue;
         }

         /// Modify MAC Source Address
         if(HlprStringsAreEqual(L"-mmda",
                                ppCLPStrings[stringIndex]))
         {
            PCWSTR pString = ppCLPStrings[stringIndex + 1];

            if(HlprEthernetMACAddressStringIsValidFormat(pString))
            {
               status = HlprEthernetMACAddressStringToValue(pString,
                                                            (BYTE*)pPCBasicPacketModificationData->macData.pDestinationMACAddress);
               HLPR_BAIL_ON_FAILURE(status);
            }
            else
            {
               status = ERROR_INVALID_DATA;

               HlprLogError(L"PrvBasicPacketModificationScenarioParseData() [status: %#x][destination MAC Address: %s]",
                            status,
                            pString);

               HLPR_BAIL;
            }

            pPCBasicPacketModificationData->flags |= PCPMDF_MODIFY_MAC_HEADER;

            pPCBasicPacketModificationData->macData.flags |= PCPMDF_MODIFY_MAC_HEADER_DESTINATION_ADDRESS;

            continue;
         }

         /// Modify IP Source Address
         if(HlprStringsAreEqual(L"-misa",
                                ppCLPStrings[stringIndex]))
         {
            PCWSTR pString = ppCLPStrings[stringIndex + 1];

            if(pPCBasicPacketModificationData->ipData.ipVersion == IPV6 &&
               HlprIPAddressV6StringIsValidFormat(pString))
            {
               status = HlprIPAddressV6StringToValue(pString,
                                                     (BYTE*)pPCBasicPacketModificationData->ipData.sourceAddress.pIPv6);
               HLPR_BAIL_ON_FAILURE(status);
            }
            else if(pPCBasicPacketModificationData->ipData.ipVersion == IPV4)
            {
               UINT32 sourceAddress = 0;

               status = HlprIPAddressV4StringToValue(pString,
                                                     &sourceAddress);
               HLPR_BAIL_ON_FAILURE(status);

               sourceAddress = htonl(sourceAddress);

               RtlCopyMemory(pPCBasicPacketModificationData->ipData.sourceAddress.pIPv4,
                             &sourceAddress,
                             sizeof(UINT32));
            }
            else
            {
               status = ERROR_INVALID_DATA;

               HlprLogError(L"PrvBasicPacketModificationScenarioParseModificationData() [status: %#x][ipVersion: %d][source ipAddress: %s]",
                            status,
                            pPCBasicPacketModificationData->ipData.ipVersion,
                            pString);

               HLPR_BAIL;
            }

            pPCBasicPacketModificationData->flags |= PCPMDF_MODIFY_IP_HEADER;

            pPCBasicPacketModificationData->ipData.flags |= PCPMDF_MODIFY_IP_HEADER_SOURCE_ADDRESS;

            continue;
         }

         /// Modify IP Destination Address
         if(HlprStringsAreEqual(L"-mida",
                                ppCLPStrings[stringIndex]))
         {
            PCWSTR pString = ppCLPStrings[stringIndex + 1];

            if(pPCBasicPacketModificationData->ipData.ipVersion == IPV6 &&
               HlprIPAddressV6StringIsValidFormat(pString))
            {
               status = HlprIPAddressV6StringToValue(pString,
                                                     (BYTE*)pPCBasicPacketModificationData->ipData.destinationAddress.pIPv6);
               HLPR_BAIL_ON_FAILURE(status);
            }
            else if(pPCBasicPacketModificationData->ipData.ipVersion == IPV4)
            {
               UINT32 destinationAddress = 0;

               status = HlprIPAddressV4StringToValue(pString,
                                                     &destinationAddress);
               HLPR_BAIL_ON_FAILURE(status);

               destinationAddress = htonl(destinationAddress);

               RtlCopyMemory(pPCBasicPacketModificationData->ipData.destinationAddress.pIPv4,
                             &destinationAddress,
                             sizeof(UINT32));
            }
            else
            {
               status = ERROR_INVALID_DATA;

               HlprLogError(L"PrvBasicPacketModificationScenarioParseModificationData() [status: %#x][ipVersion: %d][destination ipAddress: %s]",
                            status,
                            pPCBasicPacketModificationData->ipData.ipVersion,
                            pString);

               HLPR_BAIL;
            }

            pPCBasicPacketModificationData->flags |= PCPMDF_MODIFY_IP_HEADER;

            pPCBasicPacketModificationData->ipData.flags |= PCPMDF_MODIFY_IP_HEADER_DESTINATION_ADDRESS;

            continue;
         }

         /// Modify the Interface Index to inject to
         if(HlprStringsAreEqual(L"-mii",
                                ppCLPStrings[stringIndex]))
         {
            PCWSTR pString = ppCLPStrings[stringIndex + 1];

            if(iswdigit((wint_t)pString[0]))
            {
               UINT32 interfaceIndex = wcstol(pString,
                                              0,
                                              0);

               pPCBasicPacketModificationData->ipData.interfaceIndex  = interfaceIndex;

               pPCBasicPacketModificationData->flags |= PCPMDF_MODIFY_IP_HEADER;

               pPCBasicPacketModificationData->ipData.flags |= PCPMDF_MODIFY_IP_HEADER_INTERFACE_INDEX;
            }

            continue;
         }

         /// Modify Transport Source Port
         if(HlprStringsAreEqual(L"-mtsp",
                                ppCLPStrings[stringIndex]))
         {
            PCWSTR pString = ppCLPStrings[stringIndex + 1];

            if(iswdigit((wint_t)pString[0]))
            {
               UINT32 localPort = wcstol(pString,
                                         0,
                                         0);
               if(localPort <= 0xFFFF)
               {
                  UINT16 port = localPort & 0xFFFF;

                  pPCBasicPacketModificationData->transportData.source.port = htons(port);
               }

               pPCBasicPacketModificationData->flags |= PCPMDF_MODIFY_TRANSPORT_HEADER;

               pPCBasicPacketModificationData->transportData.flags |= PCPMDF_MODIFY_TRANSPORT_HEADER_SOURCE_PORT;
            }

            continue;
         }

         /// Modify Transport Destination Port
         if(HlprStringsAreEqual(L"-mtdp",
                                ppCLPStrings[stringIndex]))
         {
            PCWSTR pString = ppCLPStrings[stringIndex + 1];

            if(iswdigit((wint_t)pString[0]))
            {
               UINT32 localPort = wcstol(pString,
                                         0,
                                         0);
               if(localPort <= 0xFFFF)
               {
                  UINT16 port = localPort & 0xFFFF;

                  pPCBasicPacketModificationData->transportData.destination.port = htons(port);
               }

               pPCBasicPacketModificationData->flags |= PCPMDF_MODIFY_TRANSPORT_HEADER;

               pPCBasicPacketModificationData->transportData.flags |= PCPMDF_MODIFY_TRANSPORT_HEADER_DESTINATION_PORT;
            }

            continue;
         }
      }
   }

   HLPR_BAIL_LABEL:

   return status;
}

/**
 @scenario_function="BasicPacketModificationScenarioExecute"

   Purpose:  Gather and package data neccessary to setup the BASIC_PACKET_MODIFICATION scenario, 
             then invoke RPC to implement the scenario in the WFPSampler service.               <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 BasicPacketModificationScenarioExecute(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                              _In_ const UINT32 stringCount)
{
   ASSERT(ppCLPStrings);
   ASSERT(stringCount);

   UINT32                             status                         = NO_ERROR;
   BOOLEAN                            removeScenario                 = FALSE;
   PC_BASIC_PACKET_MODIFICATION_DATA* pPCBasicPacketModificationData = 0;
   FWPM_FILTER*                       pFilter                        = 0;

   status = HlprFwpmFilterCreate(&pFilter);
   HLPR_BAIL_ON_FAILURE(status);

   pFilter->displayData.name = L"WFPSampler's Basic Packet Modification Scenario Filter";

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
      HLPR_NEW(pPCBasicPacketModificationData,
               PC_BASIC_PACKET_MODIFICATION_DATA);
      HLPR_BAIL_ON_ALLOC_FAILURE(pPCBasicPacketModificationData,
                                 status);

      status = PrvBasicPacketModificationScenarioParseModificationData(ppCLPStrings,
                                                                       stringCount,
                                                                       &(pFilter->layerKey),
                                                                       pPCBasicPacketModificationData);
      HLPR_BAIL_ON_FAILURE(status);
   }

   status = RPCInvokeScenarioBasicPacketModification(wfpSamplerBindingHandle,
                                                     SCENARIO_BASIC_PACKET_MODIFICATION,
                                                     removeScenario ? FWPM_CHANGE_DELETE : FWPM_CHANGE_ADD,
                                                     pFilter,
                                                     pPCBasicPacketModificationData);
   if(status != NO_ERROR)
      HlprLogError(L"BasicPacketModificationScenarioExecute : RpcInvokeScenarioBasicPacketModification() [status: %#x]",
                   status);
   else
      HlprLogInfo(L"BasicPacketModificationScenarioExecute : RpcInvokeScenarioBasicPacketModification() [status: %#x]",
                  status);

   HLPR_BAIL_LABEL:

   if(pFilter)
      HlprFwpmFilterDestroy(&pFilter);

   HLPR_DELETE(pPCBasicPacketModificationData);

   return status;
}

/**
 @public_function="BasicPacketModificationScenarioLogHelp"
 
   Purpose:  Log usage information for the BASIC_PACKET_MODIFICATION scenario to the console.   <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
VOID BasicPacketModificationScenarioLogHelp()
{
   wprintf(L"\n\t\t -s     \t BASIC_PACKET_MODIFICATION");
   wprintf(L"\n\t\t -?     \t Receive usage information.");
   wprintf(L"\n\t\t -l     \t Specify the layer to perform the filtering. [Required]");
   wprintf(L"\n\t\t -sl    \t Specify the sublayer to perform the filtering. [Optional]");
   wprintf(L"\n\t\t -r     \t Remove the scenario objects.");
   wprintf(L"\n\t\t -v     \t Make the filter volatile (non-persistent). [Optional]");
   wprintf(L"\n\t\t -b     \t Makes the objects available during boot time. [Optional]");
   wprintf(L"\n\t\t -in    \t Perform the injection inline if possible. [Optional]");
   wprintf(L"\n\t\t -tdpc  \t Use threaded DPCs for out of band. [Optional]");
   wprintf(L"\n\t\t -wi    \t Use work items for out of band. [Optional]");
   wprintf(L"\n\t\t -ipla  \t Specify the IP_LOCAL_ADDRESS /");
   wprintf(L"\n\t\t        \t    IP_SOURCE_ADDRESS to filter. [Optional]");
   wprintf(L"\n\t\t -ipra  \t Specify the IP_REMOTE_ADDRESS /");
   wprintf(L"\n\t\t        \t    IP_DESTINATION_ADDRESS to filter. [Optional]");
   wprintf(L"\n\t\t -ipp   \t Specify the IP_PROTOCOL to filter. [Optional]");
   wprintf(L"\n\t\t -iplp  \t Specify the IP_LOCAL_PORT to filter. [Optional]");
   wprintf(L"\n\t\t -icmpt \t Specify the ICMP_TYPE to filter. [Optional]");
   wprintf(L"\n\t\t -iprp  \t Specify the IP_REMOTE_PORT to filter. [Optional]");
   wprintf(L"\n\t\t -icmpc \t Specify the ICMP_CODE to filter. [Optional]");
   wprintf(L"\n\t\t -mmsa  \t Specify the new source MAC address. [Optional]");
   wprintf(L"\n\t\t -mmda  \t Specify the new destination MAC address. [Optional]");
   wprintf(L"\n\t\t -mii   \t Specify the new interface index to inject to. [Optional]");
   wprintf(L"\n\t\t -misa  \t Specify the new source IP address. [Optional]");
   wprintf(L"\n\t\t -mida  \t Specify the new destination IP address. [Optional]");
   wprintf(L"\n\t\t -mtsp  \t Specify the new source transport port / icmp code. [Optional]");
   wprintf(L"\n\t\t -mtdp  \t Specify the new destination transport port / icmp type. [Optional]");
   wprintf(L"\n");
   wprintf(L"\n\t i.e.");
   wprintf(L"\n\t\t  WFPSampler.Exe -s BASIC_PACKET_MODIFICATION -l FWPM_LAYER_OUTBOUND_TRANSPORT_V4 -ipla 1.0.0.1 -ipra 1.0.0.254 -ipp UDP -mtdp 4000 -v");
   wprintf(L"\n\t\t  WFPSampler.Exe -s BASIC_PACKET_MODIFICATION -l FWPM_LAYER_OUTBOUND_TRANSPORT_V4 -ipla 1.0.0.1 -ipra 1.0.0.254 -ipp UDP -mtdp 4000 -v -r");
   wprintf(L"\n");

   return;
}
