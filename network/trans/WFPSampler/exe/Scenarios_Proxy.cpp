////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      Scenarios_Proxy.cpp
//
//   Abstract:
//      This module contains functions which prepares and sends data for the PROXY scenario 
//         implementation.
//
//   Naming Convention:
//
//      <Scope><Object><Action><Modifier>
//  
//      i.e.
//
//       <Scope>
//          {
//                          - Function is likely visible to other modules.
//            Prv           - Function is private to this module.
//          }
//       <Object>
//          {
//            ProxyScenario - Function pertains to all of the Proxy Scenarios.
//          }
//       <Action>
//          {
//            Execute       - Function packages data and invokes RPC to the WFPSampler service.
//            Log           - Function writes to the console.
//            Parse         - Function pulls data into the required format from the provided data.
//          }
//       <Modifier>
//          {
//            ProxyData     - Function acts on the PC_PROXY_DATA.
//            Help          - Function provides usage information.
//          }
//
//   Private Functions:
//      PrvProxyScenarioParseProxyData(),
//
//   Public Functions:
//      ProxyScenarioExecute(),
//      ProxyScenarioLogHelp(),
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Add support for specifying a different sublayer, and
//                                              set the original traffic data in the providerContext
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Framework_WFPSampler.h" /// .

/// WFPSamplerProxyService.Exe's ports
#define PROXY_PORT_V4 17476
#define PROXY_PORT_V6 26214

/**
 @private_function="PrvProxyScenarioParseProxyData"
 
   Purpose:  Parse the command line parameters for proxying data such as:                       <br>
                Proxy to local address                                     (-pla IP_ADDRESS)    <br>
                Proxy to local port                                        (-plp PORT)          <br>
                Proxy to remote address                                    (-pra IP_ADDRESS)    <br>
                Proxy to remote port                                       (-prp PORT)          <br>
                Proxy to local service                                     (-plspid PID)        <br>
                Perform the injection inline (from within the classify)    (-in)                <br>
                Use threaded DPCs for out of band (asynchronous)           (-tdpc)              <br>
                Use work items for out of band (asynchronous)              (-wi)                <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 PrvProxyScenarioParseProxyData(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                      _In_ const UINT32 stringCount,
                                      _Inout_ PC_PROXY_DATA* pPCProxyData)
{
   ASSERT(ppCLPStrings);
   ASSERT(stringCount);
   ASSERT(pPCProxyData);

   UINT32       status         = NO_ERROR;
   const UINT32 MAX_PARAMETERS = 9;
   UINT32       found          = 0;

   for(UINT32 stringIndex = 0;
       stringIndex < stringCount &&
       found != MAX_PARAMETERS;
       stringIndex++)
   {
      if((stringIndex + 1) < stringCount)
      {
         /// Original local address
         if(HlprStringsAreEqual(L"-ipla",
                                ppCLPStrings[stringIndex]))
         {
            PCWSTR pString = ppCLPStrings[stringIndex + 1];

            if(HlprIPAddressV6StringIsValidFormat(pString))
            {
               status = HlprIPAddressV6StringToValue(pString,
                                                     pPCProxyData->originalLocalAddress.pIPv6);
               HLPR_BAIL_ON_FAILURE(status);
            }
            else
            {
               UINT32 ipv4Address = 0;

               status = HlprIPAddressV4StringToValue(pString,
                                                     &ipv4Address);
               HLPR_BAIL_ON_FAILURE(status);

               ipv4Address = htonl(ipv4Address);

               RtlCopyMemory(pPCProxyData->originalLocalAddress.pIPv4,
                             &ipv4Address,
                             sizeof(UINT32));
            }

            continue;
         }

         /// Original remote address
         if(HlprStringsAreEqual(L"-ipra",
                                ppCLPStrings[stringIndex]))
         {
            PCWSTR pString = ppCLPStrings[stringIndex + 1];

            if(HlprIPAddressV6StringIsValidFormat(pString))
            {
               status = HlprIPAddressV6StringToValue(pString,
                                                     pPCProxyData->originalRemoteAddress.pIPv6);
               HLPR_BAIL_ON_FAILURE(status);
            }
            else
            {
               UINT32 ipv4Address = 0;

               status = HlprIPAddressV4StringToValue(pString,
                                                     &ipv4Address);

               ipv4Address = htonl(ipv4Address);

               RtlCopyMemory(pPCProxyData->originalRemoteAddress.pIPv4,
                             &ipv4Address,
                             sizeof(UINT32));
            }

            continue;
         }

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
                  pPCProxyData->ipProtocol = (UINT8)(protocol & 0xFF);
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

               pPCProxyData->ipProtocol = (UINT8)(protocol & 0xFF);
            }

            continue;
         }

         /// Original local port
         if(HlprStringsAreEqual(L"-iplp",
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
                  pPCProxyData->originalLocalPort = (UINT16)(port & 0xFFFF);
                  pPCProxyData->originalLocalPort = htons(pPCProxyData->originalLocalPort);
               }
            }

            continue;
         }

         /// Original remote port
         if(HlprStringsAreEqual(L"-iprp",
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
                  pPCProxyData->originalRemotePort = (UINT16)(port & 0xFFFF);
                  pPCProxyData->originalRemotePort = htons(pPCProxyData->originalRemotePort);
               }
            }

            continue;
         }

         /// Proxy to Local Address
         if(HlprStringsAreEqual(L"-pla",
                                ppCLPStrings[stringIndex]))
         {
            PCWSTR pString = ppCLPStrings[stringIndex + 1];

            if(pPCProxyData->ipVersion == IPV6 &&
               HlprIPAddressV6StringIsValidFormat(pString))
            {
               status = HlprIPAddressV6StringToValue(pString,
                                                     (BYTE*)pPCProxyData->proxyLocalAddress.pIPv6);
               HLPR_BAIL_ON_FAILURE(status);
            }
            else if(pPCProxyData->ipVersion == IPV4)
            {
               UINT32 localAddress = 0;

               status = HlprIPAddressV4StringToValue(pString,
                                                     &localAddress);
               HLPR_BAIL_ON_FAILURE(status);

               localAddress = htonl(localAddress);

               RtlCopyMemory(pPCProxyData->proxyLocalAddress.pIPv4,
                             &localAddress,
                             sizeof(UINT32));
            }
            else
            {
               status = ERROR_INVALID_DATA;

               HlprLogError(L"PrvProxyScenarioParseRandomizedData() [status: %#x][ipVersion: %d][local ipAddress: %s]",
                            status,
                            pPCProxyData->ipVersion,
                            pString);

               HLPR_BAIL;
            }

            pPCProxyData->flags |= PCPDF_PROXY_LOCAL_ADDRESS;

            found++;

            continue;
         }

         /// Proxy to Local Port
         if(HlprStringsAreEqual(L"-plp",
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

                  pPCProxyData->proxyLocalPort = htons(port);
               }

               pPCProxyData->flags |= PCPDF_PROXY_LOCAL_PORT;

               found++;
            }

            continue;
         }

         /// Proxy to Remote Address
         if(HlprStringsAreEqual(L"-pra",
                                ppCLPStrings[stringIndex]))
         {
            PCWSTR pString = ppCLPStrings[stringIndex + 1];

            if(pPCProxyData->ipVersion == IPV6 &&
               HlprIPAddressV6StringIsValidFormat(pString))
            {
               status = HlprIPAddressV6StringToValue(pString,
                                                     (BYTE*)pPCProxyData->proxyRemoteAddress.pIPv6);
               HLPR_BAIL_ON_FAILURE(status);
            }
            else if(pPCProxyData->ipVersion == IPV4)
            {
               UINT32 remoteAddress = 0;

               status = HlprIPAddressV4StringToValue(pString,
                                                     &remoteAddress);
               HLPR_BAIL_ON_FAILURE(status);

               remoteAddress = htonl(remoteAddress);

               RtlCopyMemory(pPCProxyData->proxyRemoteAddress.pIPv4,
                             &remoteAddress,
                             sizeof(UINT32));
            }
            else
            {
               status = ERROR_INVALID_DATA;

               HlprLogError(L"PrvProxyScenarioParseRandomizedData() [status: %#x][ipVersion: %d][Remote ipAddress: %s]",
                            status,
                            pPCProxyData->ipVersion,
                            pString);

               HLPR_BAIL;
            }

            pPCProxyData->flags |= PCPDF_PROXY_REMOTE_ADDRESS;

            found++;

            continue;
         }

         /// Proxy to Remote Port
         if(HlprStringsAreEqual(L"-prp",
                                ppCLPStrings[stringIndex]))
         {
            PCWSTR pString = ppCLPStrings[stringIndex + 1];

            if(iswdigit((wint_t)pString[0]))
            {
               UINT32 remotePort = wcstol(pString,
                                          0,
                                          0);
               if(remotePort <= 0xFFFF)
               {
                  UINT16 port = remotePort & 0xFFFF;

                  pPCProxyData->proxyRemotePort = htons(port);
               }

               pPCProxyData->flags |= PCPDF_PROXY_REMOTE_PORT;

               found++;
            }

            continue;
         }

         /// Proxy to local service (process ID)
         if(HlprStringsAreEqual(L"-plspid",
                                ppCLPStrings[stringIndex]))
         {
            PCWSTR pString = ppCLPStrings[stringIndex + 1];

            if(iswdigit((wint_t)pString[0]))
            {
               UINT32 pid = wcstol(pString,
                                   0,
                                   0);
               if(pid)
                  pPCProxyData->targetProcessID = pid;

               found++;
            }

            continue;
         }
      }

      /// Proxy Inline
      if(HlprStringsAreEqual(L"-in",
                             ppCLPStrings[stringIndex]))
      {
         pPCProxyData->performInline = TRUE;

         found++;

         continue;
      }

      /// Proxy to a remote service
      if(HlprStringsAreEqual(L"-prs",
                             ppCLPStrings[stringIndex]))
      {
         pPCProxyData->proxyToRemoteService = TRUE;

         found++;

         continue;
      }

      /// Threaded DPC
      if(HlprStringsAreEqual(L"-tdpc",
                             ppCLPStrings[stringIndex]))
      {
         pPCProxyData->useThreadedDPC = TRUE;
      
         found++;
      
         continue;
      }
      
      /// Work Items
      if(HlprStringsAreEqual(L"-wi",
                             ppCLPStrings[stringIndex]))
      {
         pPCProxyData->useWorkItems = TRUE;
      
         found++;
      
         continue;
      }
   }

   if(!(pPCProxyData->proxyToRemoteService))
   {
/*
      INET_PORT_RANGE portRange = {0};

      portRange.StartPort     = pPCProxyData->ipVersion == IPV6 ? PROXY_PORT_V6 : PROXY_PORT_V4;
      portRange.NumberOfPorts = 1;

      HlprWinSockQueryPortReservation(IPPROTO_TCP,
                                      &portRange,
                                      &pPCProxyData->tcpPortReservationToken);

      HlprWinSockQueryPortReservation(IPPROTO_UDP,
                                      &portRange,
                                      &pPCProxyData->udpPortReservationToken);
*/
      if(pPCProxyData->targetProcessID == 0)
         HlprProcessGetID(L"WFPSamplerProxyService.Exe",
                          &(pPCProxyData->targetProcessID));
   }

   HLPR_BAIL_LABEL:

   return status;
}

/**
 @scenario_function="ProxyScenarioExecute"

   Purpose:  Gather and package data neccessary to setup the PROXY scenario, then invoke RPC to 
             implement the scenario in the WFPSampler service.                                  <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 ProxyScenarioExecute(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                            _In_ UINT32 stringCount)
{
   ASSERT(ppCLPStrings);
   ASSERT(stringCount);

   UINT32         status         = NO_ERROR;
   BOOLEAN        removeScenario = FALSE;
   PC_PROXY_DATA* pPCProxyData   = 0;
   FWPM_FILTER*   pFilter        = 0;

   status = HlprFwpmFilterCreate(&pFilter);
   HLPR_BAIL_ON_FAILURE(status);

   pFilter->displayData.name = L"WFPSampler's Proxy Filter";

   HlprCommandLineParseForScenarioRemoval(ppCLPStrings,
                                          stringCount,
                                          &removeScenario);

   status = HlprCommandLineParseForFilterInfo(ppCLPStrings,
                                              stringCount,
                                              pFilter,
                                              removeScenario);
   HLPR_BAIL_ON_FAILURE(status);

   if(pFilter->layerKey != FWPM_LAYER_OUTBOUND_TRANSPORT_V4 &&
      pFilter->layerKey !=  FWPM_LAYER_OUTBOUND_TRANSPORT_V6

#if(NTDDI_VERSION >= NTDDI_WIN7)

      &&
      pFilter->layerKey != FWPM_LAYER_ALE_BIND_REDIRECT_V4 &&
      pFilter->layerKey != FWPM_LAYER_ALE_BIND_REDIRECT_V6 &&
      pFilter->layerKey != FWPM_LAYER_ALE_CONNECT_REDIRECT_V4 &&
      pFilter->layerKey != FWPM_LAYER_ALE_CONNECT_REDIRECT_V6

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

     )
   {
      status = (UINT32)FWP_E_INCOMPATIBLE_LAYER;

      HLPR_BAIL;
   }

   HLPR_NEW(pPCProxyData,
            PC_PROXY_DATA);
   HLPR_BAIL_ON_ALLOC_FAILURE(pPCProxyData,
                              status);

   if(HlprFwpmLayerIsIPv4(&(pFilter->layerKey)))
      pPCProxyData->ipVersion = IPV4;
   else
      pPCProxyData->ipVersion = IPV6;

   status = PrvProxyScenarioParseProxyData(ppCLPStrings,
                                           stringCount,
                                           pPCProxyData);
   HLPR_BAIL_ON_FAILURE(status);

   /// Proxying at this layer means we need to reverse the proxying on the inbound traffic.
   /// This will be done at INBOUND_IPPACKET so as to support as many scenarios as possible (TCP, UDP, connected UDP etc.)
   /// This will not work for IPsec protected traffic.
   if(pFilter->layerKey == FWPM_LAYER_OUTBOUND_TRANSPORT_V4 &&
      pFilter->layerKey ==  FWPM_LAYER_OUTBOUND_TRANSPORT_V6)
   {
      BYTE pEmptyAddress[IPV6_ADDRESS_LENGTH] = {0};

      if((pPCProxyData->flags & PCPDF_PROXY_LOCAL_ADDRESS &&
         RtlCompareMemory(pPCProxyData->originalLocalAddress.pIPv6,
                          pEmptyAddress,
                          IPV6_ADDRESS_LENGTH) == IPV6_ADDRESS_LENGTH) ||
         (pPCProxyData->flags & PCPDF_PROXY_REMOTE_ADDRESS &&
         RtlCompareMemory(pPCProxyData->originalLocalAddress.pIPv6,
                          pEmptyAddress,
                          IPV6_ADDRESS_LENGTH) == IPV6_ADDRESS_LENGTH) ||
         (pPCProxyData->flags & PCPDF_PROXY_LOCAL_PORT &&
         (!pPCProxyData->ipProtocol ||
         !pPCProxyData->originalLocalPort)) ||
         (pPCProxyData->flags & PCPDF_PROXY_REMOTE_PORT &&
         (!pPCProxyData->ipProtocol ||
         !pPCProxyData->originalRemotePort)))
      {
         status = ERROR_INVALID_DATA;

         HlprLogError(L"ProxyScenarioExecute : Must specify the original address and / or port being proxied [status: %#x]",
                      status);

         HLPR_BAIL;
      }
   }

   status = RPCInvokeScenarioProxy(wfpSamplerBindingHandle,
                                   SCENARIO_PROXY,
                                   removeScenario ? FWPM_CHANGE_DELETE : FWPM_CHANGE_ADD,
                                   pFilter,
                                   pPCProxyData);
   if(status != NO_ERROR)
      HlprLogError(L"ProxyScenarioExecute : RpcInvokeScenarioProxy() [status: %#x]",
                   status);
   else
      HlprLogInfo(L"ProxyScenarioExecute : RpcInvokeScenarioProxy() [status: %#x]",
                  status);

   HLPR_BAIL_LABEL:

   if(pFilter)
      HlprFwpmFilterDestroy(&pFilter);

   HLPR_DELETE(pPCProxyData);;

   return status;
}

/**
 @public_function="ProxyScenarioLogHelp"
 
   Purpose:  Log usage information for the PROXY scenario to the console.                       <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
VOID ProxyScenarioLogHelp()
{
#if(NTDDI_VERSION >= NTDDI_WIN7)

   PWSTR pLayer = L"FWPM_LAYER_ALE_CONNECT_REDIRECT_V4";

#else

   PWSTR pLayer = L"FWPM_LAYER_OUTBOUND_TRANSPORT_V4";

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

   wprintf(L"\n\t\t -s     \t PROXY");
   wprintf(L"\n\t\t -?     \t Receive usage information.");
   wprintf(L"\n\t\t -l     \t Specify the layer to perform the filtering. [Required]");
   wprintf(L"\n\t\t -sl    \t Specify the sublayer to perform the filtering. [Optional]");
   wprintf(L"\n\t\t -r     \t Remove the scenario objects.");
   wprintf(L"\n\t\t -v     \t Make the filter volatile (non-persistent). [Optional]");
   wprintf(L"\n\t\t -b     \t Makes the objects available during boot time. [Optional]");
   wprintf(L"\n\t\t -in    \t  Perform the injection inline if possible. [Optional]");
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
   wprintf(L"\n\t\t -pla   \t Specify the local address to proxy to. [Optional]");
   wprintf(L"\n\t\t -plp   \t Specify the local port to proxy to. [Optional]");
   wprintf(L"\n\t\t -pra   \t Specify the remote address to proxy to. [Optional]");
   wprintf(L"\n\t\t -prp   \t Specify the remote port to proxy to. [Optional]");
   wprintf(L"\n\t\t -plspid\t Specify the local service PID to proxy to. [Optional] [Defaults to WFPSamplerProxyService.Exe's PID]");
   wprintf(L"\n\t\t -prs   \t Specify the proxying will be to a remote service. [Optional]");
   wprintf(L"\n");
   wprintf(L"\n\t i.e.");
   wprintf(L"\n\t\t WFPSampler.Exe -s PROXY -l %s -rp 80 -p TCP -pla 127.0.0.1 -plp 4080 -v",
           pLayer);
   wprintf(L"\n\t\t WFPSampler.Exe -s PROXY -l %s -rp 80 -p TCP -pla 127.0.0.1 -plp 4080 -v -r",
           pLayer);
   wprintf(L"\n");
   wprintf(L"\n\t\t Note: if the layer specified is FWPM_LAYER_OUTBOUND_TRANSPORT_V{4/6}:");
   wprintf(L"\n\t\t\tIf proxying the ports, then you must specify the protocol (-ipp) and the original port being modified (-iprp and / or -iplp)");
   wprintf(L"\n\t\t\tIf proxying the addresses, then you must specify the the original address (-ipra and / or -ipla)");
   wprintf(L"\n");      

   return;
}
