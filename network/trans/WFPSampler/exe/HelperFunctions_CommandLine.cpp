////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_CommandLine.cpp
//
//   Abstract:
//      This module contains functions which assist in parsing informatin from the command prompt.
//
//   Naming Convention:
//
//      <Scope><Module><Object><Action><Modifier>
//  
//      i.e.
//
//       <Scope>
//          {
//                                          - Function is likely visible to other modules.
//            Prv                           - Function is private to this module.
//          }
//       <Module>
//          {
//            Hlpr                          - Function is from HelperFunctions_* Modules.
//          }
//       <Object>
//          {
//            CommandLine                   - Function acts on the arguments passed to the command 
//                                               line.
//            DataLinkAddressType           - Function pertains to DL_ADDRESS_TYPE values.
//            EtherType                     - Function pertains to the Ethernet types.
//            FwpConditionFlag              - Function pertains to FWP_CONDITION_FLAG_* values.
//            FwpConditionL2Flag            - Function pertains to FWP_CONDITION_L2* values.
//            FwpConditionReauthorizeReason - Function pertains to FWP_CONDITION_L2* values.
//            FwpDirection                  - Function pertains to FWP_DIRECTION values.
//            InterfaceType                 - Function pertains to interface types.
//            IPAddressType                 - Function pertains to NL_ADDRESS_TYPE values.
//            NDISMediumType                - Function pertains to NDIS_MEDIUM values.
//            NDISPhysicalMediumType        - Function pertains to NDIS_PHYSICAL_MEDIUM values.
//            ProfileID                     - Function pertains to NLM_NETWORK_CATEGORY values.
//            Protocol                      = Function pertains to IP protocol values.
//            TunnelType                    - Function pertains to TUNNEL_TYPE values.
//            VSwitchNetworkType            - Function pertains to FWP_VSWITCH_NETWORK_TYPE values.
//            VSwitchNICType                - Function pertains to NDIS_SWITCH_NIC_TYPE values.
//          }
//       <Action>
//          {
//            Parse              - Function pulls data into the required format from the provided 
//                                    data.
//          }
//       <Modifier>
//          {
//
//          }
//
//   Private Functions:
//      PrvHlprCommandLineStringToFwpMatchType(),
//      PrvHlprDataLinkAddressTypeParse(),
//      PrvHlprEtherTypeParse(),
//      PrvHlprFwpConditionFlagParse(),
//      PrvHlprFwpConditionL2FlagParse(),
//      PrvHlprFwpConditionReauthorizeReasonParse(),
//      PrvHlprFwpDirectionParse(),
//      PrvHlprInterfaceTypeParse(),
//      PrvHlprIPAddressTypeParse(),
//      PrvHlprNDISMediumTypeParse(),
//      PrvHlprNDISPhysicalMediumTypeParse(),
//      PrvHlprProfileIDParse(),
//      PrvHlprProtocolParse(),
//      PrvHlprTunnelTypeParse(),
//      PrvHlprVSwitchNetworkTypeParse(),
//      PrvHlprVSwitchNICTypeParse(),
//
//   Public Functions:
//      HlprCommandLineParseForBootTime(),
//      HlprCommandLineParseForCalloutUse(),
//      HlprCommandLineParseForLayerKey(),
//      HlprCommandLineParseForVolatility(),
//      HlprCommandLineParseForFilterConditions(),
//      HlprCommandLineParseForFilterInfo()
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Add support for specifying a different sublayer
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "HelperFunctions_Include.h" /// .
#include <AccCtrl.h>
#include <ACLAPI.h>
#include <SDDL.h>

#if(NTDDI_VERSION >= NTDDI_WIN8)

/**
 @private_function="PrvHlprCreateAppContainerSecurityDescriptor"
 
   Purpose:  Create a SECURITY_DESCRIPTOR for use with AppContainer filtering.                  <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return != 0)
PSECURITY_DESCRIPTOR PrvHlprCreateAppContainerSecurityDescriptor()
{
   PSECURITY_DESCRIPTOR     pSelfRelativeSecurityDescriptor = 0;
   UINT32                   status                          = NO_ERROR;
   SID_IDENTIFIER_AUTHORITY appPackageAuthority             = SECURITY_APP_PACKAGE_AUTHORITY;
   PSID                     pAppContainerSID                = 0;
   PSECURITY_DESCRIPTOR     pAbsoluteSecurityDescriptor     = 0;
   PACL                     pACL                            = 0;
   UINT32                   aclLength                       = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) + (GetLengthSid(pAppContainerSID) - sizeof(UINT32));
   UINT32                   securityDescriptorLength        = sizeof(SECURITY_DESCRIPTOR);

   HLPR_NEW_CASTED_ARRAY(pAbsoluteSecurityDescriptor,
                         SECURITY_DESCRIPTOR,
                         BYTE,
                         securityDescriptorLength);
   HLPR_BAIL_ON_ALLOC_FAILURE(pAbsoluteSecurityDescriptor,
                              status);

   HLPR_NEW_CASTED_ARRAY(pACL,
                         ACL,
                         BYTE,
                         aclLength);
   HLPR_BAIL_ON_ALLOC_FAILURE(pACL,
                              status);

   if(AllocateAndInitializeSid(&appPackageAuthority,
                               SECURITY_BUILTIN_APP_PACKAGE_RID_COUNT,
                               SECURITY_APP_PACKAGE_BASE_RID,
                               SECURITY_BUILTIN_PACKAGE_ANY_PACKAGE,
                               0,
                               0,
                               0,
                               0,
                               0,
                               0,
                               &pAppContainerSID) == 0 ||
      pAppContainerSID == 0)
   {
      status = GetLastError();

      HlprLogInfo(L"PrvHlprCreateAppContainerSecurityDescriptor : AllocateAndInitializeSid() [status: %#x][pAppContainerSID: %#p]",
                  status,
                  pAppContainerSID);

      HLPR_BAIL;
   }

   if(InitializeAcl(pACL,
                    aclLength,
                    ACL_REVISION) == 0)
   {
      status = GetLastError();

      HlprLogInfo(L"PrvHlprCreateAppContainerSecurityDescriptor : InitializeACL() [status: %#x]",
                  status);

      HLPR_BAIL;
   }

   if(AddAccessAllowedAce(pACL,
                          ACL_REVISION,
                          READ_CONTROL | FWP_ACTRL_MATCH_FILTER,
                          pAppContainerSID) == 0)
   {
      status = GetLastError();

      HlprLogInfo(L"PrvHlprCreateAppContainerSecurityDescriptor : AddAccessAllowedAce() [status: %#x]",
                  status);

      HLPR_BAIL;
   }

   if(InitializeSecurityDescriptor(pAbsoluteSecurityDescriptor,
                                   SECURITY_DESCRIPTOR_REVISION) == 0)
   {
      status = GetLastError();

      HlprLogInfo(L"PrvHlprCreateAppContainerSecurityDescriptor : InitializeSecurityDescriptor() [status: %#x]",
                  status);

      HLPR_BAIL;
   }

   if(SetSecurityDescriptorDacl(pAbsoluteSecurityDescriptor,
                                TRUE,
                                pACL,
                                FALSE) == 0)
   {
      status = GetLastError();

      HlprLogInfo(L"PrvHlprCreateAppContainerSecurityDescriptor : SetSecurityDescriptorGroup() [status: %#x]",
                  status);

      HLPR_BAIL;
   }

   if(MakeSelfRelativeSD(pAbsoluteSecurityDescriptor,
                         pSelfRelativeSecurityDescriptor,
                         (LPDWORD)&securityDescriptorLength) == 0)
   {
      status = GetLastError();

      if(status != ERROR_INSUFFICIENT_BUFFER ||
         securityDescriptorLength == 0)
      {
         HlprLogInfo(L"PrvHlprCreateAppContainerSecurityDescriptor : MakeSelfRelativeSD() [status: %#x]",
                     status);

         HLPR_BAIL;
      }
      else
         status = NO_ERROR;
   }

   HLPR_NEW_CASTED_ARRAY(pSelfRelativeSecurityDescriptor,
                         SECURITY_DESCRIPTOR,
                         BYTE,
                         securityDescriptorLength);
   HLPR_BAIL_ON_ALLOC_FAILURE(pSelfRelativeSecurityDescriptor,
                              status);

   if(MakeSelfRelativeSD(pAbsoluteSecurityDescriptor,
                         pSelfRelativeSecurityDescriptor,
                         (LPDWORD)&securityDescriptorLength) == 0)
   {
      status = GetLastError();

      HlprLogInfo(L"PrvHlprCreateAppContainerSecurityDescriptor : MakeSelfRelativeSD() [status: %#x]",
                  status);

      HLPR_BAIL;
   }

   HLPR_BAIL_LABEL:

   HLPR_DELETE_ARRAY(pACL);

   HLPR_DELETE_ARRAY(pAbsoluteSecurityDescriptor);

   if(status != NO_ERROR)
   {
      HLPR_DELETE_ARRAY(pSelfRelativeSecurityDescriptor);
   }

   if(pAppContainerSID)
   {
      FreeSid(pAppContainerSID);

      pAppContainerSID = 0;
   }

   return pSelfRelativeSecurityDescriptor;
}

/**
 @private_function="AppliesToAppContainers"
 
   Purpose:  Determine if the codition should apply to app containers.                          <br>
                                                                                                <br>
   Notes:    Applies additional logic to:                                                       <br>
                FWPM_CONDITION_ALE_USER_ID                                                      <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
BOOLEAN AppliesToAppContainers(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                               _In_ UINT32 stringCount)
{
   BOOLEAN appliesToAppContainers = FALSE;

   for(UINT32 stringIndex = 0;
       (stringIndex + 1) < stringCount;
       stringIndex++)
   {
      if(HlprStringsAreEqual(L"-a2ac",
                             ppCLPStrings[stringIndex]))
      {
         appliesToAppContainers = TRUE;

         break;
      }
   }

   return appliesToAppContainers;
}

#endif /// (NTDDI_VERSION >= NTDDI_WIN8)

/**
 @private_function="PrvHlprSIDGet"
 
   Purpose:  Lookup the SID for the provided user.                                              <br>
                                                                                                <br>
   Notes:    The caller is responsible for freeing any allocated memory using HLPR_DELETE_ARRAY.<br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA379159.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA379151.aspx              <br>
*/
UINT32 PrvHlprSIDGet(_In_ PCWSTR pName,
                     _Inout_ PSID* ppSID)
{
   UINT32       status      = NO_ERROR;
   PSID         pSID        = 0;
   UINT32       sidSize     = 0;
   size_t       domainSize  = 0;
   PWSTR        pDomainName = 0;
   SID_NAME_USE sidType     = SidTypeUser;

   /// First call will give us the necessary size
   if(LookupAccountName(0,
                        pName,
                        0,
                        (DWORD*)&sidSize,
                        0,
                        (DWORD*)&domainSize,
                        &sidType) == 0)
   {
      status = GetLastError();

      if(status != ERROR_INSUFFICIENT_BUFFER ||
         sidSize == 0)
      {
         HlprLogError(L"PrvHlprSIDGet: LookupAccountName [status: %#x]",
                      status);

         HLPR_BAIL;
      }
   }

   HLPR_NEW_CASTED_ARRAY(pSID,
                         SID,
                         BYTE,
                         sidSize);
   HLPR_BAIL_ON_ALLOC_FAILURE(pSID,
                              status);

   if(domainSize)
   {
      HLPR_NEW_ARRAY(pDomainName,
                     WCHAR,
                     domainSize);
      HLPR_BAIL_ON_ALLOC_FAILURE(pDomainName,
                                 status);
   }

   if(LookupAccountName(0,
                        pName,
                        pSID,
                        (DWORD*)&sidSize,
                        pDomainName,
                        (DWORD*)&domainSize,
                        &sidType) == 0)
   {
      status = GetLastError();

      HlprLogError(L"PrvHlprSIDGet: LookupAccountName [status: %#x]",
                   status);

      HLPR_BAIL;
   }

   if(!IsValidSid(pSID))
   {
      status = ERROR_INVALID_SID;
   
      HlprLogError(L"PrvHlprSIDGet: IsValidSid() [status: %#x]",
                   status);

      HLPR_BAIL;
   }

   *ppSID = pSID;
   
   status = NO_ERROR;

   HLPR_BAIL_LABEL:

   if(status != NO_ERROR)
   {
      HLPR_DELETE_ARRAY(pSID);
   }

   HLPR_DELETE_ARRAY(pDomainName);

   return status;
}

/**
 @private_function="PrvHlprCommandLineStringToFwpMatchType"
 
   Purpose:  Parse a string for a FWP_MATCH_TYPE value.                                         <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA364942.aspx              <br>
*/
FWP_MATCH_TYPE PrvHlprCommandLineStringToFwpMatchType(_In_ PCWSTR pMatchType)
{
   ASSERT(pMatchType);

   FWP_MATCH_TYPE matchType = FWP_MATCH_TYPE_MAX;

   if(HlprStringsAreEqual(pMatchType,
                          L"==") ||
      HlprStringsAreEqual(pMatchType,
                          L"="))
      matchType = FWP_MATCH_EQUAL;
   else if(HlprStringsAreEqual(pMatchType,
                               L"!="))
      matchType = FWP_MATCH_NOT_EQUAL;
   else if(HlprStringsAreEqual(pMatchType,
                               L">"))
      matchType = FWP_MATCH_GREATER;
   else if(HlprStringsAreEqual(pMatchType,
                               L"<"))
      matchType = FWP_MATCH_LESS;
   else if(HlprStringsAreEqual(pMatchType,
                               L">="))
      matchType = FWP_MATCH_GREATER_OR_EQUAL;
   else if(HlprStringsAreEqual(pMatchType,
                               L"<="))
      matchType = FWP_MATCH_LESS_OR_EQUAL;

   return matchType;
}

/**
 @private_function="PrvHlprEtherTypeParse"
 
   Purpose:  Parse a string for a well known frame type value.                                  <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
UINT32 PrvHlprEtherTypeParse(_In_ PCWSTR pEtherType)
{
   ASSERT(pEtherType);

   UINT32 etherType = 0;

   if(HlprStringsAreEqual(pEtherType,
                          L"IPv4") ||
      HlprStringsAreEqual(pEtherType,
                          L"NDIS_ETH_TYPE_IPV4"))
      etherType = NDIS_ETH_TYPE_IPV4;                    /// 0x0800
   else if(HlprStringsAreEqual(pEtherType,
                               L"ARP") ||
           HlprStringsAreEqual(pEtherType,
                               L"NDIS_ETH_TYPE_ARP"))
      etherType = NDIS_ETH_TYPE_ARP;                     /// 0x0806
   else if(HlprStringsAreEqual(pEtherType,
                               L"IPv6") ||
           HlprStringsAreEqual(pEtherType,
                               L"NDIS_ETH_TYPE_IPV6"))
      etherType = NDIS_ETH_TYPE_IPV6;                    /// 0x86DD
   else if(HlprStringsAreEqual(pEtherType,
                               L"NDIS_ETH_TYPE_802_1X"))
      etherType = NDIS_ETH_TYPE_802_1X;                  /// 0x888E
   else if(HlprStringsAreEqual(pEtherType,
                               L"NDIS_ETH_TYPE_802_1Q"))
      etherType = NDIS_ETH_TYPE_802_1Q;                  /// 0x8100
   else if(HlprStringsAreEqual(pEtherType,
                               L"NDIS_ETH_TYPE_SLOW_PROTOCOL"))
      etherType = NDIS_ETH_TYPE_SLOW_PROTOCOL;           /// 0x8809

   return etherType;
}

/**
 @private_function="PrvHlprNDISMediumTypeParse"
 
   Purpose:  Parse a string for a NDIS_MEDIUM value.                                            <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF565910.aspx              <br>
*/
UINT32 PrvHlprNDISMediumTypeParse(_In_ PCWSTR pMediumType)
{
   ASSERT(pMediumType);

   UINT32 mediumType = NdisMediumMax;

   if(HlprStringsAreEqual(pMediumType,
                          L"Ethernet") ||
      HlprStringsAreEqual(pMediumType,
                          L"NdisMedium802_3"))
      mediumType = NdisMedium802_3;                       /// 0x0000
   else if(HlprStringsAreEqual(pMediumType,
                               L"TokenRing") ||
           HlprStringsAreEqual(pMediumType,
                               L"NdisMedium802_5"))
      mediumType = NdisMedium802_5;                       /// 0x0001
   else if(HlprStringsAreEqual(pMediumType,
                               L"FDDI") ||
           HlprStringsAreEqual(pMediumType,
                               L"NdisMediumFddi"))
      mediumType = NdisMediumFddi;                        /// 0x0002
   else if(HlprStringsAreEqual(pMediumType,
                               L"WAN") ||
           HlprStringsAreEqual(pMediumType,
                               L"NdisMediumWan"))
      mediumType = NdisMediumWan;                         /// 0x0003
   else if(HlprStringsAreEqual(pMediumType,
                               L"LocalTalk") ||
           HlprStringsAreEqual(pMediumType,
                               L"NdisMediumLocalTalk"))
      mediumType = NdisMediumLocalTalk;                   /// 0x0004
   else if(HlprStringsAreEqual(pMediumType,
                               L"DIX") ||
           HlprStringsAreEqual(pMediumType,
                               L"NdisMediumDix"))
      mediumType = NdisMediumDix;                         /// 0x0005
   else if(HlprStringsAreEqual(pMediumType,
                               L"ARCNet") ||
           HlprStringsAreEqual(pMediumType,
                               L"NdisMediumArcnetRaw"))
      mediumType = NdisMediumArcnetRaw;                   /// 0x0006
   else if(HlprStringsAreEqual(pMediumType,
                               L"NdisMediumArcnet878_2"))
      mediumType = NdisMediumArcnet878_2;                 /// 0x0007
   else if(HlprStringsAreEqual(pMediumType,
                               L"ATM") ||
           HlprStringsAreEqual(pMediumType,
                               L"NdisMediumAtm"))
      mediumType = NdisMediumAtm;                        /// 0x0008
   else if(HlprStringsAreEqual(pMediumType,
                               L"WirelessWAN") ||
           HlprStringsAreEqual(pMediumType,
                               L"NdisMediumWirelessWan"))
      mediumType = NdisMediumWirelessWan;                /// 0x0009
   else if(HlprStringsAreEqual(pMediumType,
                               L"IRDA") ||
           HlprStringsAreEqual(pMediumType,
                               L"NdisMediumIrda"))
      mediumType = NdisMediumIrda;                       /// 0x000A
   else if(HlprStringsAreEqual(pMediumType,
                               L"BPC") ||
           HlprStringsAreEqual(pMediumType,
                               L"NdisMediumBpc"))
      mediumType = NdisMediumBpc;                        /// 0x000B
   else if(HlprStringsAreEqual(pMediumType,
                               L"NdisMediumCoWan"))
      mediumType = NdisMediumCoWan;                      /// 0x000C
   else if(HlprStringsAreEqual(pMediumType,
                               L"Firewire") ||
           HlprStringsAreEqual(pMediumType,
                               L"NdisMedium1394"))
      mediumType = NdisMedium1394;                       /// 0x000D
   else if(HlprStringsAreEqual(pMediumType,
                               L"InfiniBand") ||
           HlprStringsAreEqual(pMediumType,
                               L"NdisMediumInfiniBand"))
      mediumType = NdisMediumInfiniBand;                 /// 0x000E
   else if(HlprStringsAreEqual(pMediumType,
                               L"Tunnel") ||
           HlprStringsAreEqual(pMediumType,
                               L"NdisMediumTunnel"))
      mediumType = NdisMediumTunnel;                     /// 0x000F
   else if(HlprStringsAreEqual(pMediumType,
                               L"NdisMediumNative802_11"))
      mediumType = NdisMediumNative802_11;               /// 0x0010
   else if(HlprStringsAreEqual(pMediumType,
                          L"Loopback") ||
           HlprStringsAreEqual(pMediumType,
                               L"NdisMediumLoopback"))
      mediumType = NdisMediumLoopback;                   /// 0x0011

#if(NTDDI_VERSION >= NTDDI_WIN7)

   else if(HlprStringsAreEqual(pMediumType,
                          L"WiMax") ||
           HlprStringsAreEqual(pMediumType,
                               L"NdisMediumWiMAX"))
      mediumType = NdisMediumWiMAX;                      /// 0x0012
   else if(HlprStringsAreEqual(pMediumType,
                          L"IP") ||
           HlprStringsAreEqual(pMediumType,
                               L"NdisMediumIP"))
      mediumType = NdisMediumIP;                         /// 0x0013

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

   return mediumType;
}

/**
 @private_function="PrvHlprNDISMediumTypeParse"
 
   Purpose:  Parse a string for a NDIS_PHYSICAL_MEDIUM value.                                   <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
UINT32 PrvHlprNDISPhysicalMediumTypeParse(_In_ PCWSTR pPhysicalMediumType)
{
   ASSERT(pPhysicalMediumType);

   UINT32 mediumType = NdisPhysicalMediumMax;

   if(HlprStringsAreEqual(pPhysicalMediumType,
                          L"Unspecified") ||
      HlprStringsAreEqual(pPhysicalMediumType,
                          L"NdisPhysicalMediumUnspecified"))
      mediumType = NdisPhysicalMediumUnspecified;         /// 0x0000
   else if(HlprStringsAreEqual(pPhysicalMediumType,
                               L"WirelessLan") ||
           HlprStringsAreEqual(pPhysicalMediumType,
                               L"NdisPhysicalMediumWirelessLan"))
      mediumType = NdisPhysicalMediumWirelessLan;         /// 0x0001
   else if(HlprStringsAreEqual(pPhysicalMediumType,
                               L"CableModem") ||
           HlprStringsAreEqual(pPhysicalMediumType,
                               L"NdisPhysicalMediumCableModem"))
      mediumType = NdisPhysicalMediumCableModem;          /// 0x0002
   else if(HlprStringsAreEqual(pPhysicalMediumType,
                               L"PhoneLine") ||
           HlprStringsAreEqual(pPhysicalMediumType,
                               L"NdisPhysicalMediumPhoneLine"))
      mediumType = NdisPhysicalMediumPhoneLine;           /// 0x0003
   else if(HlprStringsAreEqual(pPhysicalMediumType,
                               L"PowerLine") ||
           HlprStringsAreEqual(pPhysicalMediumType,
                               L"NdisPhysicalMediumPowerLine"))
      mediumType = NdisPhysicalMediumPowerLine;           /// 0x0004
   else if(HlprStringsAreEqual(pPhysicalMediumType,
                               L"DSL") ||
           HlprStringsAreEqual(pPhysicalMediumType,
                               L"NdisPhysicalMediumDSL"))
      mediumType = NdisPhysicalMediumDSL;                 /// 0x0005
   else if(HlprStringsAreEqual(pPhysicalMediumType,
                               L"FibreChannel") ||
           HlprStringsAreEqual(pPhysicalMediumType,
                               L"NdisPhysicalMediumFibreChannel"))
      mediumType = NdisPhysicalMediumFibreChannel;        /// 0x0006
   else if(HlprStringsAreEqual(pPhysicalMediumType,
                               L"Firewire") ||
           HlprStringsAreEqual(pPhysicalMediumType,
                               L"NdisPhysicalMedium1394"))
      mediumType = NdisPhysicalMedium1394;                /// 0x0007
   else if(HlprStringsAreEqual(pPhysicalMediumType,
                               L"WirelessWan") ||
           HlprStringsAreEqual(pPhysicalMediumType,
                               L"NdisPhysicalMediumWirelessWan"))
      mediumType = NdisPhysicalMediumWirelessWan;         /// 0x0008
   else if(HlprStringsAreEqual(pPhysicalMediumType,
                               L"Native802_11") ||
           HlprStringsAreEqual(pPhysicalMediumType,
                               L"NdisPhysicalMediumNative802_11"))
      mediumType = NdisPhysicalMediumNative802_11;        /// 0x0009
   else if(HlprStringsAreEqual(pPhysicalMediumType,
                               L"Bluetooth") ||
           HlprStringsAreEqual(pPhysicalMediumType,
                               L"NdisPhysicalMediumBluetooth"))
      mediumType = NdisPhysicalMediumBluetooth;           /// 0x000A
   else if(HlprStringsAreEqual(pPhysicalMediumType,
                               L"Infiniband") ||
           HlprStringsAreEqual(pPhysicalMediumType,
                               L"NdisPhysicalMediumInfiniband"))
      mediumType = NdisPhysicalMediumInfiniband;          /// 0x000B
   else if(HlprStringsAreEqual(pPhysicalMediumType,
                               L"WiMax") ||
           HlprStringsAreEqual(pPhysicalMediumType,
                               L"NdisPhysicalMediumWiMax"))
      mediumType = NdisPhysicalMediumWiMax;               /// 0x000C
   else if(HlprStringsAreEqual(pPhysicalMediumType,
                               L"UWB") ||
           HlprStringsAreEqual(pPhysicalMediumType,
                               L"NdisPhysicalMediumUWB"))
      mediumType = NdisPhysicalMediumUWB;                 /// 0x000D
   else if(HlprStringsAreEqual(pPhysicalMediumType,
                               L"Ethernet") ||
           HlprStringsAreEqual(pPhysicalMediumType,
                               L"NdisPhysicalMedium802_3"))
      mediumType = NdisPhysicalMedium802_3;               /// 0x000E
   else if(HlprStringsAreEqual(pPhysicalMediumType,
                               L"TokenRing") ||
           HlprStringsAreEqual(pPhysicalMediumType,
                               L"NdisPhysicalMedium802_5"))
      mediumType = NdisPhysicalMedium802_5;               /// 0x000F
   else if(HlprStringsAreEqual(pPhysicalMediumType,
                               L"Irda") ||
           HlprStringsAreEqual(pPhysicalMediumType,
                               L"NdisPhysicalMediumIrda"))
      mediumType = NdisPhysicalMediumIrda;                /// 0x0010
   else if(HlprStringsAreEqual(pPhysicalMediumType,
                               L"WiredWan") ||
           HlprStringsAreEqual(pPhysicalMediumType,
                               L"NdisPhysicalMediumWiredWan"))
      mediumType = NdisPhysicalMediumWiredWAN;            /// 0x0011
   else if(HlprStringsAreEqual(pPhysicalMediumType,
                               L"WiredCoWan") ||
           HlprStringsAreEqual(pPhysicalMediumType,
                               L"NdisPhysicalMediumWiredCoWan"))
      mediumType = NdisPhysicalMediumWiredCoWan;          /// 0x0012
   else if(HlprStringsAreEqual(pPhysicalMediumType,
                               L"NdisPhysicalMediumOther"))
      mediumType = NdisPhysicalMediumOther;               /// 0x0013

   return mediumType;
}

/**
 @private_function="PrvHlprDataLinkAddressTypeParse"
 
   Purpose:  Parse a string for an DL_ADDRESS_TYPE value.                                       <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/DD744934.aspx              <br>
*/
UINT8 PrvHlprDataLinkAddressTypeParse(_In_ PCWSTR pDLAddressType)
{
   ASSERT(pDLAddressType);

   UINT8 type = DlUnicast;

   if(HlprStringsAreEqual(pDLAddressType,
                          L"Unicast") ||
      HlprStringsAreEqual(pDLAddressType,
                          L"DlUnicast"))
      type = DlUnicast;
   else if(HlprStringsAreEqual(pDLAddressType,
                               L"Multicast") ||
           HlprStringsAreEqual(pDLAddressType,
                               L"DlMulticast"))
      type = DlMulticast;
   else if(HlprStringsAreEqual(pDLAddressType,
                               L"Broadcast") ||
           HlprStringsAreEqual(pDLAddressType,
                               L"DlBroadcast"))
      type = DlBroadcast;

   return type;
}

/**
 @private_function="PrvHlprIPAddressTypeParse"
 
   Purpose:  Parse a string for an NL_ADDRESS_TYPE value.                                       <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/FF568757.aspx              <br>
*/
UINT8 PrvHlprIPAddressTypeParse(_In_ PCWSTR pNLAddressType)
{
   ASSERT(pNLAddressType);

   UINT8 type = NlatUnspecified;

   if(HlprStringsAreEqual(pNLAddressType,
                          L"Unicast") ||
      HlprStringsAreEqual(pNLAddressType,
                          L"NlatUnicast"))
      type = NlatUnicast;
   else if(HlprStringsAreEqual(pNLAddressType,
                               L"Anycast") ||
           HlprStringsAreEqual(pNLAddressType,
                               L"NlatAnycast"))
      type = NlatAnycast;
   else if(HlprStringsAreEqual(pNLAddressType,
                          L"Multicast") ||
           HlprStringsAreEqual(pNLAddressType,
                               L"NlatMulticast"))
      type = NlatMulticast;
   else if(HlprStringsAreEqual(pNLAddressType,
                          L"Broadcast") ||
           HlprStringsAreEqual(pNLAddressType,
                               L"NlatBroadcast"))
      type = NlatBroadcast;

   return type;
}

/**
 @private_function="PrvHlprProtocolParse"
 
   Purpose:  Parse a string for a well known protocol value.                                    <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
UINT32 PrvHlprProtocolParse(_In_ PCWSTR pProtocol)
{
   ASSERT(pProtocol);

   UINT32 protocol = IPPROTO_MAX;

   if(HlprStringsAreEqual(L"TCP",
                          pProtocol))
      protocol = IPPROTO_TCP;
   else if(HlprStringsAreEqual(L"UDP",
                               pProtocol))
      protocol = IPPROTO_UDP;
   else if(HlprStringsAreEqual(L"ICMPV4",
                               pProtocol))
      protocol = IPPROTO_ICMP;
   else if(HlprStringsAreEqual(L"ICMPV6",
                               pProtocol))
      protocol = IPPROTO_ICMPV6;

   return protocol;
}

/**
 @private_function="PrvHlprFwpDirectionParse"
 
   Purpose:  Parse a string for a FWP_DIRECTION value.                                          <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/FF552433.aspx              <br>
*/
UINT32 PrvHlprFwpDirectionParse(_In_ PCWSTR pDirection)
{
   ASSERT(pDirection);

   UINT32 direction = FWP_DIRECTION_MAX;

   if(HlprStringsAreEqual(pDirection,
                          L"OUTBOUND") ||
      HlprStringsAreEqual(pDirection,
                          L"FWP_DIRECTION_OUTBOUND"))
      direction = FWP_DIRECTION_OUTBOUND;
   else if(HlprStringsAreEqual(pDirection,
                               L"INBOUND") ||
           HlprStringsAreEqual(pDirection,
                               L"FWP_DIRECTION_INBOUND"))
      direction = FWP_DIRECTION_INBOUND;

   return direction;
}

/**
 @helper_function="PrvHlprFwpConditionFlagParse"
 
   Purpose:  Parse a string for a FWP_CONDITION_FLAGS value.                                    <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA364002.aspx              <br>
*/
UINT32 PrvHlprFwpConditionFlagParse(_In_ PCWSTR pFlag)
{
   ASSERT(pFlag);

   UINT32 flag = 0;

   if(HlprStringsAreEqual(pFlag,
                          L"FWP_CONDITION_FLAG_IS_LOOPBACK"))
      flag = FWP_CONDITION_FLAG_IS_LOOPBACK;
   else if(HlprStringsAreEqual(pFlag,
                               L"FWP_CONDITION_FLAG_IS_IPSEC_SECURED"))
      flag = FWP_CONDITION_FLAG_IS_IPSEC_SECURED;
   else if(HlprStringsAreEqual(pFlag,
                               L"FWP_CONDITION_FLAG_IS_REAUTHORIZE"))
      flag = FWP_CONDITION_FLAG_IS_REAUTHORIZE;
   else if(HlprStringsAreEqual(pFlag,
                               L"FWP_CONDITION_FLAG_IS_WILDCARD_BIND"))
      flag = FWP_CONDITION_FLAG_IS_WILDCARD_BIND;
   else if(HlprStringsAreEqual(pFlag,
                               L"FWP_CONDITION_FLAG_IS_RAW_ENDPOINT"))
      flag = FWP_CONDITION_FLAG_IS_RAW_ENDPOINT;
   else if(HlprStringsAreEqual(pFlag,
                               L"FWP_CONDITION_FLAG_IS_FRAGMENT"))
      flag = FWP_CONDITION_FLAG_IS_FRAGMENT;
   else if(HlprStringsAreEqual(pFlag,
                               L"FWP_CONDITION_FLAG_IS_FRAGMENT_GROUP"))
      flag = FWP_CONDITION_FLAG_IS_FRAGMENT_GROUP;
   else if(HlprStringsAreEqual(pFlag,
                               L"FWP_CONDITION_FLAG_IS_IPSEC_NATT_RECLASSIFY"))
      flag = FWP_CONDITION_FLAG_IS_IPSEC_NATT_RECLASSIFY;
   else if(HlprStringsAreEqual(pFlag,
                               L"FWP_CONDITION_FLAG_REQUIRES_ALE_CLASSIFY"))
      flag = FWP_CONDITION_FLAG_REQUIRES_ALE_CLASSIFY;
   else if(HlprStringsAreEqual(pFlag,
                               L"FWP_CONDITION_FLAG_IS_IMPLICIT_BIND"))
      flag = FWP_CONDITION_FLAG_IS_IMPLICIT_BIND;

#if(NTDDI_VERSION >= NTDDI_VISTASP1)

   else if(HlprStringsAreEqual(pFlag,
                               L"FWP_CONDITION_FLAG_IS_REASSEMBLED"))
      flag = FWP_CONDITION_FLAG_IS_REASSEMBLED;

#if(NTDDI_VERSION >= NTDDI_WIN7)

   else if(HlprStringsAreEqual(pFlag,
                               L"FWP_CONDITION_FLAG_IS_NAME_APP_SPECIFIED"))
      flag = FWP_CONDITION_FLAG_IS_NAME_APP_SPECIFIED;
   else if(HlprStringsAreEqual(pFlag,
                               L"FWP_CONDITION_FLAG_IS_PROMISCUOUS"))
      flag = FWP_CONDITION_FLAG_IS_PROMISCUOUS;
   else if(HlprStringsAreEqual(pFlag,
                               L"FWP_CONDITION_FLAG_IS_AUTH_FW"))
      flag = FWP_CONDITION_FLAG_IS_AUTH_FW;
   else if(HlprStringsAreEqual(pFlag,
                               L"FWP_CONDITION_FLAG_IS_RECLASSIFY"))
      flag = FWP_CONDITION_FLAG_IS_RECLASSIFY;
   else if(HlprStringsAreEqual(pFlag,
                               L"FWP_CONDITION_FLAG_IS_OUTBOUND_PASS_THRU"))
      flag = FWP_CONDITION_FLAG_IS_OUTBOUND_PASS_THRU;
   else if(HlprStringsAreEqual(pFlag,
                               L"FWP_CONDITION_FLAG_IS_INBOUND_PASS_THRU"))
      flag = FWP_CONDITION_FLAG_IS_INBOUND_PASS_THRU;
   else if(HlprStringsAreEqual(pFlag,
                               L"FWP_CONDITION_FLAG_IS_CONNECTION_REDIRECTED"))
      flag = FWP_CONDITION_FLAG_IS_CONNECTION_REDIRECTED;

#if(NTDDI_VERSION >= NTDDI_WIN8)

   else if(HlprStringsAreEqual(pFlag,
                               L"FWP_CONDITION_FLAG_IS_PROXY_CONNECTION"))
      flag = FWP_CONDITION_FLAG_IS_PROXY_CONNECTION;
   else if(HlprStringsAreEqual(pFlag,
                               L"FWP_CONDITION_FLAG_IS_APPCONTAINER_LOOPBACK"))
      flag = FWP_CONDITION_FLAG_IS_APPCONTAINER_LOOPBACK;
   else if(HlprStringsAreEqual(pFlag,
                               L"FWP_CONDITION_FLAG_IS_NON_APPCONTAINER_LOOPBACK"))
      flag = FWP_CONDITION_FLAG_IS_NON_APPCONTAINER_LOOPBACK;

#endif /// (NTDDI_VERSION >= NTDDI_WIN8)
#endif /// (NTDDI_VERSION >= NTDDI_WIN7)
#endif /// (NTDDI_VERSION >= NTDDI_VISTASP1)

   return flag;
}

/**
 @private_function="PrvHlprInterfaceTypeParse"
 
   Purpose:  Parse a string for an interface type value.                                        <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/FF565767.aspx              <br>
*/
UINT32 PrvHlprInterfaceTypeParse(_In_ PCWSTR pInterfaceType)
{
   ASSERT(pInterfaceType);

   UINT32 type = MAX_IF_TYPE;

   if(HlprStringsAreEqual(pInterfaceType,
                          L"ETHERNET") ||
      HlprStringsAreEqual(pInterfaceType,
                          L"IF_TYPE_ETHERNET_CSMACD"))
      type = IF_TYPE_ETHERNET_CSMACD;
   else if(HlprStringsAreEqual(pInterfaceType,
                               L"TOKENRING") ||
           HlprStringsAreEqual(pInterfaceType,
                               L"IF_TYPE_ISO88025_TOKENRING"))
      type = IF_TYPE_ISO88025_TOKENRING;
   else if(HlprStringsAreEqual(pInterfaceType,
                               L"PPP") ||
           HlprStringsAreEqual(pInterfaceType,
                               L"IF_TYPE_PPP"))
      type = IF_TYPE_PPP;
   else if(HlprStringsAreEqual(pInterfaceType,
                               L"LOOPBACK") ||
           HlprStringsAreEqual(pInterfaceType,
                               L"IF_TYPE_SOFTWARE_LOOPBACK"))
      type = IF_TYPE_SOFTWARE_LOOPBACK;
   else if(HlprStringsAreEqual(pInterfaceType,
                               L"SLIP") ||
           HlprStringsAreEqual(pInterfaceType,
                               L"IF_TYPE_SLIP"))
      type = IF_TYPE_SLIP;
   else if(HlprStringsAreEqual(pInterfaceType,
                               L"WIRELESS") ||
           HlprStringsAreEqual(pInterfaceType,
                               L"IF_TYPE_IEEE80211"))
      type = IF_TYPE_IEEE80211;
   else if(HlprStringsAreEqual(pInterfaceType,
                               L"TUNNEL") ||
           HlprStringsAreEqual(pInterfaceType,
                               L"IF_TYPE_TUNNEL"))
      type = IF_TYPE_TUNNEL;

   return type;
}

/**
 @private_function="PrvHlprTunnelTypeParse"
 
   Purpose:  Parse a string for a TUNNEL_TYPE value.                                            <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/FF570962.aspx              <br>
*/
UINT32 PrvHlprTunnelTypeParse(_In_ PCWSTR pTunnelType)
{
   ASSERT(pTunnelType);

   UINT32 type = TUNNEL_TYPE_NONE;

   if(HlprStringsAreEqual(pTunnelType,
                          L"OTHER") ||
      HlprStringsAreEqual(pTunnelType,
                          L"TUNNEL_TYPE_OTHER"))
      type = TUNNEL_TYPE_OTHER;
   else if(HlprStringsAreEqual(pTunnelType,
                               L"6TO4") ||
      HlprStringsAreEqual(pTunnelType,
                          L"TUNNEL_TYPE_6TO4"))
      type = TUNNEL_TYPE_6TO4;
   else if(HlprStringsAreEqual(pTunnelType,
                               L"ISATAP") ||
      HlprStringsAreEqual(pTunnelType,
                          L"TUNNEL_TYPE_ISATAP"))
      type = TUNNEL_TYPE_ISATAP;
   else if(HlprStringsAreEqual(pTunnelType,
                               L"TEREDO") ||
      HlprStringsAreEqual(pTunnelType,
                          L"TUNNEL_TYPE_TEREDO"))
      type = TUNNEL_TYPE_TEREDO;
   else if(HlprStringsAreEqual(pTunnelType,
                               L"IPHTTPS") ||
      HlprStringsAreEqual(pTunnelType,
                          L"TUNNEL_TYPE_IPHTTPS"))
      type = TUNNEL_TYPE_IPHTTPS;

   return type;
}

/**
 @private_function="PrvHlprProfileIDParse"
 
   Purpose:  Parse a string for a NLM_NETWORK_CATEGORY value.                                   <br>
                                                                                                <br>
   Notes:    Function converts from NLM_NETWORK_CATEGORY to NL_INTERFACE_NETWORK_CATEGORY_STATE.<br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA370800.aspx              <br>
*/
UINT32 PrvHlprProfileIDParse(_In_ PCWSTR pProfileID)
{
   ASSERT(pProfileID);

   UINT32 id = NlincCategoryStateMax;

   if(HlprStringsAreEqual(pProfileID,
                          L"UNKNOWN") ||
      HlprStringsAreEqual(pProfileID,
                          L"NlincCategoryUnknown"))
      id = NlincCategoryUnknown;
   else if(HlprStringsAreEqual(pProfileID,
                               L"PUBLIC") ||
           HlprStringsAreEqual(pProfileID,
                               L"NLM_NETWORK_CATEGORY_PUBLIC") ||
           HlprStringsAreEqual(pProfileID,
                               L"NlincPublic"))
      id = NlincPublic;
   else if(HlprStringsAreEqual(pProfileID,
                               L"PRIVATE") ||
           HlprStringsAreEqual(pProfileID,
                               L"NLM_NETWORK_CATEGORY_PRIVATE") ||
           HlprStringsAreEqual(pProfileID,
                               L"NlincPrivate"))
      id = NlincPrivate;
   else if(HlprStringsAreEqual(pProfileID,
                          L"DOMAIN") ||
           HlprStringsAreEqual(pProfileID,
                               L"NLM_NETWORK_CATEGORY_DOMAIN_AUTHENTICATED") ||
           HlprStringsAreEqual(pProfileID,
                               L"NlincDomainAuthenticated"))
      id = NlincDomainAuthenticated;

   return id;
}

/**
 @private_function="PrvHlprRPCIFFlagParse"
 
   Purpose:  Parse a string for a RPC_FW_IF_FLAG value.                                         <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
UINT32 PrvHlprRPCIFFlagParse(_In_ PCWSTR pFlag)
{
   ASSERT(pFlag);

   UINT32 flag = 0;

   if(HlprStringsAreEqual(pFlag,
                          L"RPC_FW_IF_FLAG_DCOM"))
      flag = RPC_FW_IF_FLAG_DCOM;

   return flag;
}

/**
 @private_function="PrvHlprRPCProtocolParse"
 
   Purpose:  Parse a string for a RPC protocol value.                                           <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.microsoft.com/En-US/Library/Windows/Desktop/AA374395.aspx              <br>
*/
UINT8 PrvHlprRPCProtocolParse(_In_ PCWSTR pProtocol)
{
   ASSERT(pProtocol);

   UINT8 protocol = 0;

   if(HlprStringsAreEqual(pProtocol,
                          L"NCACN_IP_TCP") ||
      HlprStringsAreEqual(pProtocol,
                          L"TCP"))
      protocol = 0x07; /// NCACN_IP_TCP
   else if(HlprStringsAreEqual(pProtocol,
                               L"NCACN_NP") ||
           HlprStringsAreEqual(pProtocol,
                          L"NamedPipes"))
      protocol = 0x0F; /// NCACN_NP
   else if(HlprStringsAreEqual(pProtocol,
                               L"NCALRPC") ||
           HlprStringsAreEqual(pProtocol,
                          L"LocalRPC"))
      protocol = 0x10; /// NCALRPC

   return protocol;
}

/**
 @private_function="PrvHlprRPCAuthTypeParse"
 
   Purpose:  Parse a string for a RPC Auth Type value.                                          <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.microsoft.com/En-US/Library/Windows/Desktop/AA373556.aspx              <br>
*/
UINT32 PrvHlprRPCAuthTypeParse(_In_ PCWSTR pType)
{
   ASSERT(pType);

   UINT32 type = RPC_C_AUTHN_DEFAULT;

   if(HlprStringsAreEqual(pType,
                          L"RPC_C_AUTHN_NONE") ||
      HlprStringsAreEqual(pType,
                          L"None"))
      type = RPC_C_AUTHN_NONE;
   else if(HlprStringsAreEqual(pType,
                               L"RPC_C_AUTHN_WINNT") ||
           HlprStringsAreEqual(pType,
                          L"WinNT"))
      type = RPC_C_AUTHN_WINNT;
   else if(HlprStringsAreEqual(pType,
                               L"RPC_C_AUTHN_DEFAULT") ||
           HlprStringsAreEqual(pType,
                          L"Default"))
      type = RPC_C_AUTHN_DEFAULT;

   return type;
}

/**
 @private_function="PrvHlprRPCAuthLevelParse"
 
   Purpose:  Parse a string for a RPC Auth Level value.                                         <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.microsoft.com/En-US/Library/Windows/Desktop/AA373553.aspx              <br>
*/
UINT8 PrvHlprRPCAuthLevelParse(_In_ PCWSTR pLevel)
{
   ASSERT(pLevel);

   UINT8 level = RPC_C_AUTHN_LEVEL_DEFAULT;

   if(HlprStringsAreEqual(pLevel,
                          L"RPC_C_AUTHN_LEVEL_DEFAULT") ||
      HlprStringsAreEqual(pLevel,
                          L"Default"))
      level = RPC_C_AUTHN_LEVEL_DEFAULT;
   else if(HlprStringsAreEqual(pLevel,
                               L"RPC_C_AUTHN_LEVEL_NONE") ||
           HlprStringsAreEqual(pLevel,
                               L"None"))
      level = RPC_C_AUTHN_LEVEL_NONE;
   else if(HlprStringsAreEqual(pLevel,
                               L"RPC_C_AUTHN_LEVEL_CONNECT") ||
           HlprStringsAreEqual(pLevel,
                               L"Connect"))
      level = RPC_C_AUTHN_LEVEL_CONNECT;
   else if(HlprStringsAreEqual(pLevel,
                               L"RPC_C_AUTHN_LEVEL_NONE") ||
           HlprStringsAreEqual(pLevel,
                               L"None"))
      level = RPC_C_AUTHN_LEVEL_NONE;
   else if(HlprStringsAreEqual(pLevel,
                               L"RPC_C_AUTHN_LEVEL_CALL") ||
           HlprStringsAreEqual(pLevel,
                               L"Call"))
      level = RPC_C_AUTHN_LEVEL_CALL;
   else if(HlprStringsAreEqual(pLevel,
                               L"RPC_C_AUTHN_LEVEL_PKT_INTEGRITY") ||
           HlprStringsAreEqual(pLevel,
                               L"Integrity"))
      level = RPC_C_AUTHN_LEVEL_PKT_INTEGRITY;
   else if(HlprStringsAreEqual(pLevel,
                               L"RPC_C_AUTHN_LEVEL_PKT_PRIVACY") ||
           HlprStringsAreEqual(pLevel,
                               L"Privacy"))
      level = RPC_C_AUTHN_LEVEL_PKT_PRIVACY;

   return level;
}

/**
 @private_function="PrvHlprFwpmFilterConditionsSort"
 
   Purpose:  Pare down the filterConditions to only those applicable to the intended layer, and 
             sort them so identical condition fields are contiguous.                            <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF549939.aspx             <br>
*/
UINT32 PrvHlprFwpmFilterConditionsSort(_In_reads_(numFilterConditions) FWPM_FILTER_CONDITION* pFilterConditions,
                                       _In_ UINT32 numFilterConditions,
                                       _Inout_ FWPM_FILTER* pFilter)
{
   ASSERT(pFilterConditions);
   ASSERT(numFilterConditions);
   ASSERT(pFilter);

   UINT32 status                  = NO_ERROR;
   GUID** ppValidFilterConditions = 0;
   UINT16 validConditionCount     = 0;

   status = HlprFwpmLayerGetFilterConditionArrayByKey(&(pFilter->layerKey),
                                                      &ppValidFilterConditions,
                                                      &validConditionCount);
   HLPR_BAIL_ON_FAILURE(status);

   status = HlprFwpmFilterConditionCreate(&(pFilter->filterCondition),
                                          numFilterConditions);
   HLPR_BAIL_ON_FAILURE(status);

   pFilter->numFilterConditions = 0;

   if(ppValidFilterConditions &&
      validConditionCount)
   {
      for(UINT32 validConditionIndex = 0;
          validConditionIndex < validConditionCount;
          validConditionIndex++)
      {
         for(UINT32 conditionIndex = 0;
             conditionIndex < numFilterConditions;
             conditionIndex++)
         {
            if(HlprGUIDsAreEqual(ppValidFilterConditions[validConditionIndex],
                                 &(pFilterConditions[conditionIndex].fieldKey)))
            {
               RtlCopyMemory(&(pFilter->filterCondition[pFilter->numFilterConditions]),
                             &(pFilterConditions[conditionIndex]),
                             sizeof(FWPM_FILTER_CONDITION));

               pFilter->numFilterConditions++;
            }
         }
      }
   }

   HLPR_BAIL_LABEL:

   if(status != NO_ERROR)
   {
      HlprFwpmFilterConditionDestroy(&(pFilter->filterCondition),
                                     pFilter->numFilterConditions);

      pFilter->numFilterConditions = 0;
   }

   return status;
}

#if(NTDDI_VERSION >= NTDDI_WIN7)

/**
 @private_function="PrvHlprFwpConditionReauthorizeReasonParse"
 
   Purpose:  Parse a string for a FWP_CONDITION_REAUTHORIZE_REASON value.                       <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA364002.aspx              <br>
*/
UINT32 PrvHlprFwpConditionReauthorizeReasonParse(_In_ PCWSTR pFlag)
{
   ASSERT(pFlag);

   UINT32 flag = 0;

   if(HlprStringsAreEqual(pFlag,
                          L"POLICY_CHANGE") ||
      HlprStringsAreEqual(pFlag,
                          L"FWP_CONDITION_REAUTHORIZE_REASON_POLICY_CHANGE"))
      flag = FWP_CONDITION_REAUTHORIZE_REASON_POLICY_CHANGE;
   else if(HlprStringsAreEqual(pFlag,
                               L"NEW_ARRIVAL_INTERFACE") ||
           HlprStringsAreEqual(pFlag,
                               L"FWP_CONDITION_REAUTHORIZE_REASON_NEW_ARRIVAL_INTERFACE"))
      flag = FWP_CONDITION_REAUTHORIZE_REASON_NEW_ARRIVAL_INTERFACE;
   else if(HlprStringsAreEqual(pFlag,
                               L"NEW_NEXTHOP_INTERFACE") ||
           HlprStringsAreEqual(pFlag,
                               L"FWP_CONDITION_REAUTHORIZE_REASON_NEW_NEXTHOP_INTERFACE"))
      flag = FWP_CONDITION_REAUTHORIZE_REASON_NEW_NEXTHOP_INTERFACE;
   else if(HlprStringsAreEqual(pFlag,
                               L"PROFILE_CROSSING") ||
           HlprStringsAreEqual(pFlag,
                               L"FWP_CONDITION_REAUTHORIZE_REASON_PROFILE_CROSSING"))
      flag = FWP_CONDITION_REAUTHORIZE_REASON_PROFILE_CROSSING;
   else if(HlprStringsAreEqual(pFlag,
                               L"CLASSIFY_COMPLETION") ||
           HlprStringsAreEqual(pFlag,
                               L"FWP_CONDITION_REAUTHORIZE_REASON_CLASSIFY_COMPLETION"))
      flag = FWP_CONDITION_REAUTHORIZE_REASON_CLASSIFY_COMPLETION;
   else if(HlprStringsAreEqual(pFlag,
                               L"IPSEC_PROPERTIES_CHANGED") ||
           HlprStringsAreEqual(pFlag,
                               L"FWP_CONDITION_REAUTHORIZE_REASON_IPSEC_PROPERTIES_CHANGED"))
      flag = FWP_CONDITION_REAUTHORIZE_REASON_IPSEC_PROPERTIES_CHANGED;
   else if(HlprStringsAreEqual(pFlag,
                               L"MID_STREAM_INSPECTION") ||
           HlprStringsAreEqual(pFlag,
                               L"FWP_CONDITION_REAUTHORIZE_REASON_MID_STREAM_INSPECTION"))
      flag = FWP_CONDITION_REAUTHORIZE_REASON_MID_STREAM_INSPECTION;
   else if(HlprStringsAreEqual(pFlag,
                               L"SOCKET_PROPERTY_CHANGED") ||
           HlprStringsAreEqual(pFlag,
                               L"FWP_CONDITION_REAUTHORIZE_REASON_SOCKET_PROPERTY_CHANGED"))
      flag = FWP_CONDITION_REAUTHORIZE_REASON_SOCKET_PROPERTY_CHANGED;
   else if(HlprStringsAreEqual(pFlag,
                               L"NEW_INBOUND_MCAST_BCAST_PACKET") ||
           HlprStringsAreEqual(pFlag,
                               L"FWP_CONDITION_REAUTHORIZE_REASON_NEW_INBOUND_MCAST_BCAST_PACKET"))
      flag = FWP_CONDITION_REAUTHORIZE_REASON_NEW_INBOUND_MCAST_BCAST_PACKET;

   return flag;
}

#if(NTDDI_VERSION >= NTDDI_WIN8)

/**
 @private_function="PrvHlprFwpConditionL2FlagParse"
 
   Purpose:  Parse a string for a FWP_CONDITION_L2_FLAGS value.                                 <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
UINT32 PrvHlprFwpConditionL2FlagParse(_In_ PCWSTR pL2Flag)
{
   ASSERT(pL2Flag);

   UINT32 l2Flag = 0;

   if(HlprStringsAreEqual(pL2Flag,
                          L"FWP_CONDITION_L2_IS_NATIVE_ETHERNET"))
      l2Flag = FWP_CONDITION_L2_IS_NATIVE_ETHERNET;
   else if(HlprStringsAreEqual(pL2Flag,
                               L"FWP_CONDITION_L2_IS_WIFI"))
      l2Flag = FWP_CONDITION_L2_IS_WIFI;
   else if(HlprStringsAreEqual(pL2Flag,
                               L"FWP_CONDITION_L2_IS_MOBILE_BROADBAND"))
      l2Flag = FWP_CONDITION_L2_IS_MOBILE_BROADBAND;
   else if(HlprStringsAreEqual(pL2Flag,
                               L"FWP_CONDITION_L2_IS_WIFI_DIRECT_DATA"))
      l2Flag = FWP_CONDITION_L2_IS_WIFI_DIRECT_DATA;

   return l2Flag;
}

/**
 @private_function="PrvHlprVSwitchNetworkTypeParse"
 
   Purpose:  Parse a string for an FWP_VSWITCH_NETWORK_TYPE value.                              <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/HH447394.aspx              <br>
*/
UINT8 PrvHlprVSwitchNetworkTypeParse(_In_ PCWSTR pVSwitchNetworkType)
{
   ASSERT(pVSwitchNetworkType);

   UINT8 type = FWP_VSWITCH_NETWORK_TYPE_UNKNOWN;

   if(HlprStringsAreEqual(pVSwitchNetworkType,
                          L"Unknown") ||
      HlprStringsAreEqual(pVSwitchNetworkType,
                          L"FWP_VSWITCH_NETWORK_TYPE_UNKNOWN"))
      type = FWP_VSWITCH_NETWORK_TYPE_UNKNOWN;
   else if(HlprStringsAreEqual(pVSwitchNetworkType,
                          L"Private") ||
           HlprStringsAreEqual(pVSwitchNetworkType,
                               L"FWP_VSWITCH_NETWORK_TYPE_PRIVATE"))
      type = FWP_VSWITCH_NETWORK_TYPE_PRIVATE;
   else if(HlprStringsAreEqual(pVSwitchNetworkType,
                          L"Internal") ||
           HlprStringsAreEqual(pVSwitchNetworkType,
                               L"FWP_VSWITCH_NETWORK_TYPE_INTERNAL"))
      type = FWP_VSWITCH_NETWORK_TYPE_INTERNAL;
   else if(HlprStringsAreEqual(pVSwitchNetworkType,
                          L"External") ||
           HlprStringsAreEqual(pVSwitchNetworkType,
                               L"FWP_VSWITCH_NETWORK_TYPE_EXTERNAL"))
      type = FWP_VSWITCH_NETWORK_TYPE_EXTERNAL;

   return type;
}

/**
 @private_function="PrvHlprVSwitchNICTypeParse"
 
   Purpose:  Parse a string for an NDIS_SWITCH_NIC_TYPE value.                                  <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/HH598218.aspx             <br>
*/
UINT32 PrvHlprVSwitchNICTypeParse(_In_ PCWSTR pNICType)
{
   ASSERT(pNICType);

   UINT32 type = NdisSwitchNicTypeInternal + 1;

   if(HlprStringsAreEqual(pNICType,
                          L"External") ||
      HlprStringsAreEqual(pNICType,
                          L"NdisSwitchNicTypeExternal"))
      type = NdisSwitchNicTypeExternal;
   else if(HlprStringsAreEqual(pNICType,
                               L"Synthetic") ||
           HlprStringsAreEqual(pNICType,
                               L"NdisSwitchNicTypeSynthetic"))
      type = NdisSwitchNicTypeSynthetic;
   else if(HlprStringsAreEqual(pNICType,
                               L"Emulated") ||
           HlprStringsAreEqual(pNICType,
                               L"NdisSwitchNicTypeEmulated"))
      type = NdisSwitchNicTypeEmulated;
   else if(HlprStringsAreEqual(pNICType,
                               L"Internal") ||
           HlprStringsAreEqual(pNICType,
                               L"NdisSwitchNicTypeInternal"))
      type = NdisSwitchNicTypeInternal;

   return type;
}

#endif /// (NTDDI_VERSION >= NTDDI_WIN8)
#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

/**
 @helper_function="HlprCommandLineParseForScenarioRemoval"
 
   Purpose:  Parse the command line parameters for strings which detail:                        <br>
                -r - remove the objects associated with this scenario.                          <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
VOID HlprCommandLineParseForScenarioRemoval(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                            _In_ UINT32 stringCount,
                                            _Inout_ BOOLEAN* pRemoveScenario)
{
   ASSERT(ppCLPStrings);
   ASSERT(stringCount);
   ASSERT(pRemoveScenario);

   *pRemoveScenario = FALSE;

   for(UINT32 stringIndex = 0;
       stringIndex < stringCount;
       stringIndex++)
   {
      if(HlprStringsAreEqual(L"-r",
                             ppCLPStrings[stringIndex]))
      {
         *pRemoveScenario = TRUE;

         break;
      }
   }

   return;
}

/**
 @helper_function="HlprCommandLineParseForScenarioRemoval"
 
   Purpose:  Parse the command line parameters for strings which detail:                        <br>
                -b - mark the filter for boottime use and make the associated objects 
                     persistent.                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
VOID HlprCommandLineParseForBootTime(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                     _In_ UINT32 stringCount,
                                     _Inout_ FWPM_FILTER* pFilter)
{
   ASSERT(ppCLPStrings);
   ASSERT(stringCount);
   ASSERT(pFilter);

   for(UINT32 stringIndex = 0;
       stringIndex < stringCount;
       stringIndex++)
   {
      if(HlprStringsAreEqual(L"-b",
                             ppCLPStrings[stringIndex]))
      {
         if(pFilter->flags & FWPM_FILTER_FLAG_PERSISTENT)
            pFilter->flags ^= FWPM_FILTER_FLAG_PERSISTENT;

         pFilter->flags |= FWPM_FILTER_FLAG_BOOTTIME;

         break;
      }
   }

   return;
}

/**
 @helper_function="HlprCommandLineParseForCalloutUse"
 
   Purpose:  Parse the command line parameters for strings which detail:                        <br>
                -c - mark the callout to use the appropriate callout.                           <br>
                                                                                                <br>
   Notes:    Applies only to the following scenarios:                                           <br>
                BASIC_ACTION_BLOCK                                                              <br>
                BASIC_ACTION_CONTINUE                                                           <br>
                BASIC_ACTION_PERMIT                                                             <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
VOID HlprCommandLineParseForCalloutUse(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                       _In_ UINT32 stringCount,
                                       _Inout_ FWPM_FILTER* pFilter)
{
   ASSERT(ppCLPStrings);
   ASSERT(stringCount);
   ASSERT(pFilter);

   for(UINT32 stringIndex = 0;
       stringIndex < stringCount;
       stringIndex++)
   {
      if(HlprStringsAreEqual(L"-c",
                             ppCLPStrings[stringIndex]))
      {
         pFilter->action.type = FWP_ACTION_CALLOUT_UNKNOWN;

         break;
      }
   }

   return;
}

/**
 @helper_function="HlprCommandLineParseForVolatility"
 
   Purpose:  Parse the command line parameters for strings which detail:                        <br>
                -v - unmark the filter and associated objects as persistent.                    <br>
                                                                                                <br>
   Notes:    Should not be combined with -b (boottime)                                          <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
VOID HlprCommandLineParseForVolatility(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                       _In_ UINT32 stringCount,
                                       _Inout_ FWPM_FILTER* pFilter)
{
   ASSERT(ppCLPStrings);
   ASSERT(stringCount);
   ASSERT(pFilter);

   for(UINT32 stringIndex = 0;
       stringIndex < stringCount;
       stringIndex++)
   {
      if(HlprStringsAreEqual(L"-v",
                             ppCLPStrings[stringIndex]))
      {
         if(pFilter->flags & FWPM_FILTER_FLAG_PERSISTENT)
            pFilter->flags ^= FWPM_FILTER_FLAG_PERSISTENT;

         break;
      }
   }

   return;
}

/**
 @helper_function="HlprCommandLineParseForLayerKey"
 
   Purpose: Parse the command line parameters for strings which detail:                         <br>
               -l <FWPM_LAYER> - specify at which layer the filter should be added.             <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprCommandLineParseForLayerKey(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                       _In_ UINT32 stringCount,
                                       _Inout_ FWPM_FILTER* pFilter)
{
   ASSERT(ppCLPStrings);
   ASSERT(stringCount);
   ASSERT(pFilter);

   UINT32 status = NO_ERROR;

   for(UINT32 stringIndex = 0;
       (stringIndex + 1) < stringCount;
       stringIndex++)
   {
      if(HlprStringsAreEqual(L"-l",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pLayerString = ppCLPStrings[++stringIndex];

         if(iswdigit((wint_t)pLayerString[0]))
         {
            UINT32 layerID = wcstol(pLayerString,
                                    0,
                                    0);

            RtlCopyMemory(&(pFilter->layerKey),
                          HlprFwpmLayerGetByID(layerID),
                          sizeof(GUID));
         }
         else
         {
            const GUID* pLayerKey = HlprFwpmLayerGetByString(pLayerString);

            if(pLayerKey)
               RtlCopyMemory(&(pFilter->layerKey),
                             pLayerKey,
                             sizeof(GUID));
            else
            {
               status = ERROR_INVALID_PARAMETER;

               HlprLogError(L"HlprCommandLineParseForLayerKey() [status: %#x][%s]",
                            status,
                            pLayerString);
            }
         }

         break;
      }
   }

   return status;
}

/**
 @helper_function="HlprCommandLineParseForSubLayerKey"
 
   Purpose: Parse the command line parameters for strings which detail:                         <br>
               -s <FWPM_SUBLAYER> - specify at which sublayer the filter should be added.       <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprCommandLineParseForSubLayerKey(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                          _In_ UINT32 stringCount,
                                          _Inout_ FWPM_FILTER* pFilter)
{
   ASSERT(ppCLPStrings);
   ASSERT(stringCount);
   ASSERT(pFilter);

   UINT32 status = NO_ERROR;

   pFilter->subLayerKey = WFPSAMPLER_SUBLAYER;

   for(UINT32 stringIndex = 0;
       (stringIndex + 1) < stringCount;
       stringIndex++)
   {
      if(HlprStringsAreEqual(L"-sl",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pSubLayerString = ppCLPStrings[++stringIndex];

         if(HlprStringsAreEqual(L"FWPM_SUBLAYER_UNIVERSAL",
                                pSubLayerString) ||
            HlprStringsAreEqual(L"UNIVERSAL",
                                pSubLayerString))
            pFilter->subLayerKey = FWPM_SUBLAYER_UNIVERSAL;
         else if(HlprStringsAreEqual(L"FWPM_SUBLAYER_INSPECTION",
                                     pSubLayerString) ||
                 HlprStringsAreEqual(L"INSPECTION",
                                     pSubLayerString))
            pFilter->subLayerKey = FWPM_SUBLAYER_INSPECTION;
      }
   }

   return status;
}


/**
 @helper_function="HlprCommandLineParseForFilterConditions"
 
   Purpose:  Parse the command line parameters for strings which the conditions, match type, and 
             value to use.                                                                      <br>
                                                                                                <br>
   Notes:    If no match type is specified, then MATCH_TYPE_EQUAL is used.                      <br>
                                                                                                <br>
             For a full list of parameters and expected values, refer to ..\docs\               <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF549939.aspx             <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprCommandLineParseForFilterConditions(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                               _In_ UINT32 stringCount,
                                               _Inout_ FWPM_FILTER* pFilter,
                                               _In_ BOOLEAN forEnum)                         /* FALSE */
{
   ASSERT(ppCLPStrings);
   ASSERT(stringCount);
   ASSERT(pFilter);

   UINT32                 status                = NO_ERROR;
   FWPM_FILTER_CONDITION* pTempFilterConditions = 0;
   const UINT32           MAX_CONDITIONS        = 50;
   UINT32                 tempConditionIndex    = 0;

   HLPR_NEW_ARRAY(pTempFilterConditions,
                  FWPM_FILTER_CONDITION,
                  MAX_CONDITIONS);
   HLPR_BAIL_ON_ALLOC_FAILURE_WITH_LABEL(pTempFilterConditions,
                                         status,
                                         HLPR_BAIL_LABEL_2);

#pragma warning(push)
#pragma warning(disable: 6385) /// careful validation of stringIndex (and advancement) against stringCount prevents read overrun

   for(UINT32 stringIndex = 0;
       (stringIndex + 1) < stringCount;
       stringIndex++)
   {

#if(NTDDI_VERSION >= NTDDI_WIN8)

      /// FWPM_CONDITION_INTERFACE_MAC_ADDRESS
      /// -ima <MATCH_TYPE> <INTERFACE_MAC_ADDRESS>
      /// -ima == 01:02:03:04:05:06
      if(HlprStringsAreEqual(L"-ima",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
            if((stringIndex + 1) < stringCount)
               pString = ppCLPStrings[++stringIndex];
            else
               HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_INTERFACE_MAC_ADDRESS;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_BYTE_ARRAY6_TYPE;

         HLPR_NEW(pTempFilterConditions[tempConditionIndex].conditionValue.byteArray6,
                  FWP_BYTE_ARRAY6);
         HLPR_BAIL_ON_ALLOC_FAILURE(pTempFilterConditions[tempConditionIndex].conditionValue.byteArray6,
                                    status);

         status = HlprEthernetMACAddressStringToValue(pString,
                                                      pTempFilterConditions[tempConditionIndex].conditionValue.byteArray6->byteArray6);
         HLPR_BAIL_ON_FAILURE(status);

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_MAC_LOCAL_ADDRESS
      /// -mla <MATCH_TYPE> <MAC_LOCAL_ADDRESS>
      /// -mla == 01:02:03:04:05:06
      if(HlprStringsAreEqual(L"-mla",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_MAC_LOCAL_ADDRESS;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_BYTE_ARRAY6_TYPE;

         HLPR_NEW(pTempFilterConditions[tempConditionIndex].conditionValue.byteArray6,
                  FWP_BYTE_ARRAY6);
         HLPR_BAIL_ON_ALLOC_FAILURE(pTempFilterConditions[tempConditionIndex].conditionValue.byteArray6,
                                    status);

         status = HlprEthernetMACAddressStringToValue(pString,
                                                      pTempFilterConditions[tempConditionIndex].conditionValue.byteArray6->byteArray6);
         HLPR_BAIL_ON_FAILURE(status);

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_MAC_REMOTE_ADDRESS
      /// -mra <MATCH_TYPE> <MAC_REMOTE_ADDRESS>
      /// -mra == 01:02:03:04:05:06
      if(HlprStringsAreEqual(L"-mra",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_MAC_REMOTE_ADDRESS;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_BYTE_ARRAY6_TYPE;

         HLPR_NEW(pTempFilterConditions[tempConditionIndex].conditionValue.byteArray6,
                  FWP_BYTE_ARRAY6);
         HLPR_BAIL_ON_ALLOC_FAILURE(pTempFilterConditions[tempConditionIndex].conditionValue.byteArray6,
                                    status);

         status = HlprEthernetMACAddressStringToValue(pString,
                                                      pTempFilterConditions[tempConditionIndex].conditionValue.byteArray6->byteArray6);
         HLPR_BAIL_ON_FAILURE(status);

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_ETHER_TYPE
      /// -et <MATCH_TYPE> <ETHER_TYPE>
      /// -et == ARP
      if(HlprStringsAreEqual(L"-et",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString   = ppCLPStrings[++stringIndex];
         UINT32 etherType = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_ETHER_TYPE;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT16;

         if(iswdigit((wint_t)pString[0]))
            etherType = wcstol(pString,
                               0,
                               0);
         else
            etherType = PrvHlprEtherTypeParse(pString);

         if(etherType <= 0xFFFF)
            pTempFilterConditions[tempConditionIndex].conditionValue.uint16 = (UINT16)etherType;
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_VLAN_ID
      /// -vlid <MATCH_TYPE> <VLAN_ID>
      /// -vlid == 100
      if(HlprStringsAreEqual(L"-vlid",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];
         UINT32 vlanID  = 0xFFFFFFFF;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_VLAN_ID;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT16;

         if(iswdigit((wint_t)pString[0]))
            vlanID = wcstol(pString,
                            0,
                            0);

         if(vlanID <= 0xFFFF)
            pTempFilterConditions[tempConditionIndex].conditionValue.uint16 = (UINT16)vlanID;
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_VSWITCH_TENANT_NETWORK_ID
      /// -vstnid <MATCH_TYPE> <NETWORK_ID>
      /// -vstnid == 100
      if(HlprStringsAreEqual(L"-vstnid",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString   = ppCLPStrings[++stringIndex];
         UINT32 networkID = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_VSWITCH_TENANT_NETWORK_ID;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

         if(iswdigit((wint_t)pString[0]))
         {
            networkID = wcstol(pString,
                               0,
                               0);

            pTempFilterConditions[tempConditionIndex].conditionValue.uint32 = networkID;
         }
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_NDIS_PORT
      /// -np <MATCH_TYPE> <NDIS_PORT>
      /// -np == 0
      if(HlprStringsAreEqual(L"-np",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString  = ppCLPStrings[++stringIndex];
         UINT32 ndisPort = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_NDIS_PORT;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

         if(iswdigit((wint_t)pString[0]))
         {
            ndisPort = wcstol(pString,
                              0,
                              0);

            pTempFilterConditions[tempConditionIndex].conditionValue.uint32 = ndisPort;
         }
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_NDIS_MEDIA_TYPE
      /// -nmt <MATCH_TYPE> <NDIS_MEDIA_TYPE>
      /// -nmt == NdisMedium802_3
      if(HlprStringsAreEqual(L"-nmt",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString        = ppCLPStrings[++stringIndex];
         UINT32 ndisMediumType = NdisMediumMax;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_NDIS_MEDIA_TYPE;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

         if(iswdigit((wint_t)pString[0]))
            ndisMediumType = wcstol(pString,
                                   0,
                                   0);
         else
            ndisMediumType = PrvHlprNDISMediumTypeParse(pString);

         if(ndisMediumType < NdisMediumMax)
            pTempFilterConditions[tempConditionIndex].conditionValue.uint32 = ndisMediumType;
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_NDIS_PHYSICAL_MEDIA_TYPE
      /// -npmt <MATCH_TYPE> <NDIS_PHYSICAL_MEDIA_TYPE>
      if(HlprStringsAreEqual(L"-npmt",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString                = ppCLPStrings[++stringIndex];
         UINT32 ndisPhysicalMediumType = NdisPhysicalMediumMax;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_NDIS_PHYSICAL_MEDIA_TYPE;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

         if(iswdigit((wint_t)pString[0]))
            ndisPhysicalMediumType = wcstol(pString,
                                           0,
                                           0);
         else
            ndisPhysicalMediumType = PrvHlprNDISPhysicalMediumTypeParse(pString);

         if(ndisPhysicalMediumType < NdisPhysicalMediumMax)
            pTempFilterConditions[tempConditionIndex].conditionValue.uint32 = ndisPhysicalMediumType;
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_L2_FLAGS
      /// -l2f <MATCH_TYPE> <L2_FLAGS>
      /// -l2f == FWP_CONDITION_L2_IS_NATIVE_ETHERNET
      if(HlprStringsAreEqual(L"-l2f",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];
         UINT32 l2Flags = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_L2_FLAGS;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

         if(iswdigit((wint_t)pString[0]))
            l2Flags = wcstol(pString,
                             0,
                             0);
         else
            l2Flags = PrvHlprFwpConditionL2FlagParse(pString);

         pTempFilterConditions[tempConditionIndex].conditionValue.uint32 = l2Flags;

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_EQUAL)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_FLAGS_ALL_SET;
         else if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_NOT_EQUAL)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_FLAGS_NONE_SET;
         else
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_FLAGS_ANY_SET;

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_MAC_LOCAL_ADDRESS_TYPE
      /// -mlat <MATCH_TYPE> <MAC_LOCAL_ADDRESS_TYPE>
      /// -mlat == DlUnicast
      if(HlprStringsAreEqual(L"-mlat",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString        = ppCLPStrings[++stringIndex];
         UINT32 macAddressType = 0xFFFFFFFF;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_MAC_LOCAL_ADDRESS_TYPE;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT8;

         if(iswdigit((wint_t)pString[0]))
            macAddressType = wcstol(pString,
                                    0,
                                    0);
         else
            macAddressType = PrvHlprDataLinkAddressTypeParse(pString);

         if(macAddressType <= DlBroadcast)
            pTempFilterConditions[tempConditionIndex].conditionValue.uint8 = (UINT8)macAddressType;
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_MAC_REMOTE_ADDRESS_TYPE
      /// -mrat <MATCH_TYPE> <MAC_REMOTE_ADDRESS_TYPE>
      /// -mrat == DlUnicast
      if(HlprStringsAreEqual(L"-mrat",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString        = ppCLPStrings[++stringIndex];
         UINT32 macAddressType = 0xFFFFFFFF;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_MAC_REMOTE_ADDRESS_TYPE;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT8;

         if(iswdigit((wint_t)pString[0]))
            macAddressType = wcstol(pString,
                                    0,
                                    0);
         else
            macAddressType = PrvHlprDataLinkAddressTypeParse(pString);

         if(macAddressType <= DlBroadcast)
            pTempFilterConditions[tempConditionIndex].conditionValue.uint8 = (UINT8)macAddressType;
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_ALE_PACKAGE_ID
      /// -apid <MATCH_TYPE> <PACKAGE_ID>
      /// -apid == WinNullSid
      if(HlprStringsAreEqual(L"-apid",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];
         UINT32 sidSize = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey = FWPM_CONDITION_ALE_PACKAGE_ID;

         if(HlprStringsAreEqual(L"WinNullSid",
                                 pString) ||
            HlprStringsAreEqual(L"NullSid",
                                 pString) ||
            HlprStringsAreEqual(L"Default",
                                 pString))
         {
            status = HlprSIDGetWellKnown(WinNullSid,
                                         &(pTempFilterConditions[tempConditionIndex].conditionValue.sid),
                                         &sidSize);
            HLPR_BAIL_ON_FAILURE(status);
         }
         else
         {
            PSID pSID = 0;

            if(ConvertStringSidToSid(pString,
                                     &pSID) == 0)
            {
               status = GetLastError();

               HlprLogInfo(L"FWPM_CONDITION_ALE_PACKAGE_ID: Invalid SID [%s]",
                           pString);

               HLPR_BAIL;
            }

            if(IsValidSid(pSID))
            {
               PSID pData = 0;

               sidSize = GetLengthSid(pSID);

               HLPR_NEW_CASTED_ARRAY(pData,
                                     SID,
                                     BYTE,
                                     sidSize);
               if(pData == 0)
               {
                  LocalFree(pSID);
               
                  status = (UINT32)STATUS_NO_MEMORY;

                  HLPR_BAIL;
               }

               pTempFilterConditions[tempConditionIndex].conditionValue.sid = (SID*)pData;

               RtlCopyMemory(pTempFilterConditions[tempConditionIndex].conditionValue.sid,
                             pSID,
                             sidSize);
            }

            LocalFree(pSID);
         }

         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_SID;

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_MAC_SOURCE_ADDRESS
      /// -msa <MATCH_TYPE> <MAC_SOURCE_ADDRESS>
      /// -msa == 01:02:03:04:05:06
      if(HlprStringsAreEqual(L"-msa",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_MAC_SOURCE_ADDRESS;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_BYTE_ARRAY6_TYPE;

         HLPR_NEW(pTempFilterConditions[tempConditionIndex].conditionValue.byteArray6,
                  FWP_BYTE_ARRAY6);
         HLPR_BAIL_ON_ALLOC_FAILURE(pTempFilterConditions[tempConditionIndex].conditionValue.byteArray6,
                                    status);

         status = HlprEthernetMACAddressStringToValue(pString,
                                                      pTempFilterConditions[tempConditionIndex].conditionValue.byteArray6->byteArray6);
         HLPR_BAIL_ON_FAILURE(status);

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_MAC_DESTINATION_ADDRESS
      /// -mda <MATCH_TYPE> <DESTINATION_MAC_ADDRESS>
      /// -mda == 01:02:03:04:05:06
      if(HlprStringsAreEqual(L"-mda",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_MAC_DESTINATION_ADDRESS;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_BYTE_ARRAY6_TYPE;

         HLPR_NEW(pTempFilterConditions[tempConditionIndex].conditionValue.byteArray6,
                  FWP_BYTE_ARRAY6);
         HLPR_BAIL_ON_ALLOC_FAILURE(pTempFilterConditions[tempConditionIndex].conditionValue.byteArray6,
                                    status);

         status = HlprEthernetMACAddressStringToValue(pString,
                                                      pTempFilterConditions[tempConditionIndex].conditionValue.byteArray6->byteArray6);
         HLPR_BAIL_ON_FAILURE(status);

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_MAC_SOURCE_ADDRESS_TYPE
      /// -msat <MATCH_TYPE> <MAC_SOURCE_ADDRESS_TYPE>
      /// -msat == DlUnicast
      if(HlprStringsAreEqual(L"-msat",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString        = ppCLPStrings[++stringIndex];
         UINT32 macAddressType = 0xFFFFFFFF;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_MAC_SOURCE_ADDRESS_TYPE;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT8;

         if(iswdigit((wint_t)pString[0]))
            macAddressType = wcstol(pString,
                                    0,
                                    0);
         else
            macAddressType = PrvHlprDataLinkAddressTypeParse(pString);

         if(macAddressType <= DlBroadcast)
            pTempFilterConditions[tempConditionIndex].conditionValue.uint8 = (UINT8)macAddressType;
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_MAC_DESTINATION_ADDRESS_TYPE
      /// -mdat <MATCH_TYPE> <MAC_DESTINATION_ADDRESS_TYPE>
      /// -mdat == DlUnicast
      if(HlprStringsAreEqual(L"-mdat",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString        = ppCLPStrings[++stringIndex];
         UINT32 macAddressType = 0xFFFFFFFF;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_MAC_DESTINATION_ADDRESS_TYPE;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT8;

         if(iswdigit((wint_t)pString[0]))
            macAddressType = wcstol(pString,
                                    0,
                                    0);
         else
            macAddressType = PrvHlprDataLinkAddressTypeParse(pString);

         if(macAddressType <= DlBroadcast)
            pTempFilterConditions[tempConditionIndex].conditionValue.uint8 = (UINT8)macAddressType;
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_IP_SOURCE_PORT \ FWPM_CONDITION_VSWITCH_ICMP_TYPE
      /// -ipsp <MATCH_TYPE> <IP_SOURCE_PORT> \ -vsicmpt <MATCH_TYPE> <ICMP_TYPE>
      /// -ipsp == 80
      if(HlprStringsAreEqual(L"-ipsp",
                             ppCLPStrings[stringIndex]) ||
         HlprStringsAreEqual(L"-vsicmpt",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];
         UINT32 port    = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_IP_SOURCE_PORT;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT16;

         if(iswdigit((wint_t)pString[0]))
            port = wcstol(pString,
                          0,
                          0);

         if(port <= 0xFFFF)
            pTempFilterConditions[tempConditionIndex].conditionValue.uint16 = (UINT16)port;
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_IP_DESTINATION_PORT \ FWPM_CONDITION_VSWITCH_ICMP_CODE
      /// -ipdp <MATCH_TYPE> <DESTINATION_PORT> \ -vsicmpc <MATCH_TYPE> <ICMP_CODE>
      /// -ipdp == 80
      if(HlprStringsAreEqual(L"-ipdp",
                             ppCLPStrings[stringIndex]) ||
         HlprStringsAreEqual(L"-vsicmpc",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];
         UINT32 port    = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_IP_DESTINATION_PORT;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT16;

         if(iswdigit((wint_t)pString[0]))
            port = wcstol(pString,
                          0,
                          0);

         if(port <= 0xFFFF)
            pTempFilterConditions[tempConditionIndex].conditionValue.uint16 = (UINT16)port;
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_VSWITCH_ID
      /// -vsid  <MATCH_TYPE> <ID>
      /// -vsid == 12345678-1234-1234-1234-123456789012
      if(HlprStringsAreEqual(L"-vsid",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR         pString   = ppCLPStrings[++stringIndex];
         FWP_BYTE_BLOB* pByteBlob = 0;
         size_t         size      = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey = FWPM_CONDITION_VSWITCH_ID;

         HLPR_NEW(pByteBlob,
                  FWP_BYTE_BLOB);
         HLPR_BAIL_ON_ALLOC_FAILURE(pByteBlob,
                                    status);

         status = StringCbLength(pString,
                                 STRSAFE_MAX_CCH * sizeof(WCHAR),
                                 &size);
         if(FAILED(status) ||
            size != (36 * sizeof(WCHAR))) /// size of a GUID without braces
         {
            HLPR_DELETE(pByteBlob);

            if(size != (36 * sizeof(WCHAR)))
               status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         HLPR_NEW_ARRAY(pByteBlob->data,
                        BYTE,
                        size);
         if(pByteBlob->data == 0)
         {
            HLPR_DELETE(pByteBlob);

            status = ERROR_OUTOFMEMORY;

            HLPR_BAIL;
         }

         pByteBlob->size = (UINT32)size;

         RtlCopyMemory(pByteBlob->data,
                       pString,
                       pByteBlob->size);

         pTempFilterConditions[tempConditionIndex].conditionValue.type     = FWP_BYTE_BLOB_TYPE;
         pTempFilterConditions[tempConditionIndex].conditionValue.byteBlob = pByteBlob;

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_VSWITCH_NETWORK_TYPE
      /// -vsnt <MATCH_TYPE> <NETWORK_TYPE>
      /// -vsnt == 1
      if(HlprStringsAreEqual(L"-vsnt",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString     = ppCLPStrings[++stringIndex];
         UINT32 networkType = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_VSWITCH_NETWORK_TYPE;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT8;

         if(iswdigit((wint_t)pString[0]))
            networkType = wcstol(pString,
                                 0,
                                 0);
         else
            networkType = PrvHlprVSwitchNetworkTypeParse(pString);

         if(networkType <= FWP_VSWITCH_NETWORK_TYPE_EXTERNAL)
            pTempFilterConditions[tempConditionIndex].conditionValue.uint8 = (UINT8)networkType;
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }


      /// FWPM_CONDITION_VSWITCH_SOURCE_INTERFACE_ID
      /// -vssiid <MATCH_TYPE> <INTERFACE_ID>
      /// -vssiid == 12345678-9ABC-DEF0-1234-56784ABCDEF0--12345678-9ABC-DEF0-1234-56784ABCDEF0
      if(HlprStringsAreEqual(L"-vssiid",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR         pString   = ppCLPStrings[++stringIndex];
         FWP_BYTE_BLOB* pByteBlob = 0;
         size_t         size      = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey = FWPM_CONDITION_VSWITCH_SOURCE_INTERFACE_ID;

         HLPR_NEW(pByteBlob,
                  FWP_BYTE_BLOB);
         HLPR_BAIL_ON_ALLOC_FAILURE(pByteBlob,
                                    status);

         status = StringCbLength(pString,
                                 STRSAFE_MAX_CCH * sizeof(WCHAR),
                                 &size);
         if(FAILED(status) ||
            size < (36 * sizeof(WCHAR)) || /// size of a GUID with braces
            size > (74 * sizeof(WCHAR)))   /// size of 2 GUIDs + 2 hyphen separators
         {
            HLPR_DELETE(pByteBlob);

            if(SUCCEEDED(status))
               status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         HLPR_NEW_ARRAY(pByteBlob->data,
                        BYTE,
                        size);
         if(pByteBlob->data == 0)
         {
            HLPR_DELETE(pByteBlob);

            status = ERROR_OUTOFMEMORY;

            HLPR_BAIL;
         }

         pByteBlob->size = (UINT32)size;

         RtlCopyMemory(pByteBlob->data,
                       pString,
                       pByteBlob->size);

         pTempFilterConditions[tempConditionIndex].conditionValue.type     = FWP_BYTE_BLOB_TYPE;
         pTempFilterConditions[tempConditionIndex].conditionValue.byteBlob = pByteBlob;

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_VSWITCH_DESTINATION_INTERFACE_ID
      /// -vsdiid <MATCH_TYPE> <INTERFACE_ID>
      /// -vsdiid == 12345678-9ABC-DEF0-1234-56784ABCDEF0--12345678-9ABC-DEF0-1234-56784ABCDEF0
      if(HlprStringsAreEqual(L"-vsdiid",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR         pString   = ppCLPStrings[++stringIndex];
         FWP_BYTE_BLOB* pByteBlob = 0;
         size_t         size      = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey = FWPM_CONDITION_VSWITCH_DESTINATION_INTERFACE_ID;

         HLPR_NEW(pByteBlob,
                  FWP_BYTE_BLOB);
         HLPR_BAIL_ON_ALLOC_FAILURE(pByteBlob,
                                    status);

         status = StringCbLength(pString,
                                 STRSAFE_MAX_CCH * sizeof(WCHAR),
                                 &size);
         if(FAILED(status) ||
            size < (36 * sizeof(WCHAR)) ||   /// size of a GUID with braces
            size > (74 * sizeof(WCHAR)))
         {
            HLPR_DELETE(pByteBlob);

            if(SUCCEEDED(status))
               status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         HLPR_NEW_ARRAY(pByteBlob->data,
                        BYTE,
                        size);
         if(pByteBlob->data == 0)
         {
            HLPR_DELETE(pByteBlob);

            status = ERROR_OUTOFMEMORY;

            HLPR_BAIL;
         }

         pByteBlob->size = (UINT32)size;

         RtlCopyMemory(pByteBlob->data,
                       pString,
                       pByteBlob->size);

         pTempFilterConditions[tempConditionIndex].conditionValue.type     = FWP_BYTE_BLOB_TYPE;
         pTempFilterConditions[tempConditionIndex].conditionValue.byteBlob = pByteBlob;

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_VSWITCH_SOURCE_VM_ID
      /// -vssvmid <MATCH_TYPE> <VM_ID>
      /// -vssvmid == 12345678-1234-1234-1234-123456789012
      if(HlprStringsAreEqual(L"-vssvmid",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR         pString   = ppCLPStrings[++stringIndex];
         FWP_BYTE_BLOB* pByteBlob = 0;
         size_t         size      = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey = FWPM_CONDITION_VSWITCH_SOURCE_VM_ID;

         HLPR_NEW(pByteBlob,
                  FWP_BYTE_BLOB);
         HLPR_BAIL_ON_ALLOC_FAILURE(pByteBlob,
                                    status);

         status = StringCbLength(pString,
                                 STRSAFE_MAX_CCH * sizeof(WCHAR),
                                 &size);
         if(FAILED(status) ||
            size != (36 * sizeof(WCHAR))) /// size of a GUID without braces
         {
            HLPR_DELETE(pByteBlob);

            if(size != (36 * sizeof(WCHAR)))
               status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         HLPR_NEW_ARRAY(pByteBlob->data,
                        BYTE,
                        size);
         if(pByteBlob->data == 0)
         {
            HLPR_DELETE(pByteBlob);

            status = ERROR_OUTOFMEMORY;

            HLPR_BAIL;
         }

         pByteBlob->size = (UINT32)size;

         RtlCopyMemory(pByteBlob->data,
                       pString,
                       pByteBlob->size);

         pTempFilterConditions[tempConditionIndex].conditionValue.type     = FWP_BYTE_BLOB_TYPE;
         pTempFilterConditions[tempConditionIndex].conditionValue.byteBlob = pByteBlob;

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_VSWITCH_DESTINATION_VM_ID
      /// -vsdvmid <MATCH_TYPE> <VM_ID>
      /// -vsdvmid == 12345678-1234-1234-1234-123456789012
      if(HlprStringsAreEqual(L"-vsdvmid",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR         pString   = ppCLPStrings[++stringIndex];
         FWP_BYTE_BLOB* pByteBlob = 0;
         size_t         size      = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey = FWPM_CONDITION_VSWITCH_DESTINATION_VM_ID;

         HLPR_NEW(pByteBlob,
                  FWP_BYTE_BLOB);
         HLPR_BAIL_ON_ALLOC_FAILURE(pByteBlob,
                                    status);

         status = StringCbLength(pString,
                                 STRSAFE_MAX_CCH * sizeof(WCHAR),
                                 &size);
         if(FAILED(status) ||
            size != (36 * sizeof(WCHAR))) /// size of a GUID without braces
         {
            HLPR_DELETE(pByteBlob);

            if(size != (36 * sizeof(WCHAR)))
               status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         HLPR_NEW_ARRAY(pByteBlob->data,
                        BYTE,
                        size);
         if(pByteBlob->data == 0)
         {
            HLPR_DELETE(pByteBlob);

            status = ERROR_OUTOFMEMORY;

            HLPR_BAIL;
         }

         pByteBlob->size = (UINT32)size;

         RtlCopyMemory(pByteBlob->data,
                       pString,
                       pByteBlob->size);

         pTempFilterConditions[tempConditionIndex].conditionValue.type     = FWP_BYTE_BLOB_TYPE;
         pTempFilterConditions[tempConditionIndex].conditionValue.byteBlob = pByteBlob;

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_VSWITCH_SOURCE_INTERFACE_TYPE
      /// -vssit <MATCH_TYPE> <INTERFACE_TYPE>
      /// -vssit == 0
      if(HlprStringsAreEqual(L"-vssit",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];
         UINT32 type    = NdisSwitchNicTypeInternal + 1;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_VSWITCH_SOURCE_INTERFACE_TYPE;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

         if(iswdigit((wint_t)pString[0]))
            type = wcstol(pString,
                           0,
                           0);
         else
            type = PrvHlprVSwitchNICTypeParse(pString);

         if(type <= NdisSwitchNicTypeInternal)
            pTempFilterConditions[tempConditionIndex].conditionValue.uint32 = type;
         else
         {
            status = ERROR_INVALID_PARAMETER;
         
            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_VSWITCH_DESTINATION_INTERFACE_TYPE
      /// -vsdit <MATCH_TYPE> <INTERFACE_TYPE>
      /// -vsdit == 0
      if(HlprStringsAreEqual(L"-vsdit",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];
         UINT32 type    = NdisSwitchNicTypeInternal + 1;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_VSWITCH_DESTINATION_INTERFACE_TYPE;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

         if(iswdigit((wint_t)pString[0]))
            type = wcstol(pString,
                           0,
                           0);
         else
            type = PrvHlprVSwitchNICTypeParse(pString);

         if(type <= NdisSwitchNicTypeInternal)
            pTempFilterConditions[tempConditionIndex].conditionValue.uint32 = type;
         else
         {
            status = ERROR_INVALID_PARAMETER;
         
            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_ALE_ORIGINAL_APP_ID
      /// -aoaid <MATCH_TYPE> <APPLICATION_NAME>
      /// -aoaid == IExplore.exe
      if(HlprStringsAreEqual(L"-aoaid",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR         pString   = ppCLPStrings[++stringIndex];
         FWP_BYTE_BLOB* pAppID    = 0;
         FWP_BYTE_BLOB* pByteBlob = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey = FWPM_CONDITION_ALE_ORIGINAL_APP_ID;

         status = FwpmGetAppIdFromFileName(pString,
                                           &pAppID);
         if(status != NO_ERROR)
         {
            HLPR_DELETE(pByteBlob);

            HLPR_BAIL;
         }
         else
         {
            HLPR_NEW(pByteBlob,
                     FWP_BYTE_BLOB);
            HLPR_BAIL_ON_ALLOC_FAILURE(pByteBlob,
                                       status);

            HLPR_NEW_ARRAY(pByteBlob->data,
                           BYTE,
                           pAppID->size);
            HLPR_BAIL_ON_ALLOC_FAILURE(pByteBlob->data,
                                       status);

            RtlCopyMemory(pByteBlob->data,
                          pAppID->data,
                          pAppID->size);

            pByteBlob->size = pAppID->size;

            pTempFilterConditions[tempConditionIndex].conditionValue.type     = FWP_BYTE_BLOB_TYPE;
            pTempFilterConditions[tempConditionIndex].conditionValue.byteBlob = pByteBlob;

            FwpmFreeMemory((VOID**)&pAppID);

            pAppID = 0;
         }

         tempConditionIndex++;

         continue;
      }

#endif /// (NTDDI_VERSION >= NTDDI_WIN8)

#if(NTDDI_VERSION >= NTDDI_WIN7)

      /// FWPM_CONDITION_IP_NEXTHOP_ADDRESS
      /// -ipnha <MATCH_TYPE> <IP_NEXTHOP_ADRESS>
      /// -ipnha == 1.2.3.4
      if(HlprStringsAreEqual(L"-ipnha",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey = FWPM_CONDITION_IP_NEXTHOP_ADDRESS;

         if(HlprIPAddressV6StringIsValidFormat(pString))
         {
            pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_BYTE_ARRAY16_TYPE;

            HLPR_NEW(pTempFilterConditions[tempConditionIndex].conditionValue.byteArray16,
                     FWP_BYTE_ARRAY16);
            HLPR_BAIL_ON_ALLOC_FAILURE(pTempFilterConditions[tempConditionIndex].conditionValue.byteArray16,
                                       status);

            status = HlprIPAddressV6StringToValue(pString,
                                                  pTempFilterConditions[tempConditionIndex].conditionValue.byteArray16->byteArray16);
            HLPR_BAIL_ON_FAILURE(status);
         }
         else
         {
            pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

            status = HlprIPAddressV4StringToValue(pString,
                                                  &(pTempFilterConditions[tempConditionIndex].conditionValue.uint32));
            HLPR_BAIL_ON_FAILURE(status);
         }

         tempConditionIndex++;
         
         continue;
      }

      
      /// FWPM_CONDITION_NEXTHOP_SUB_INTERFACE_INDEX
      /// -nhsii <MATCH_TYPE> <INDEX>
      /// -nhsii == 0
      if(HlprStringsAreEqual(L"-nhsii",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];
         UINT32 index   = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_NEXTHOP_SUB_INTERFACE_INDEX;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

         if(iswdigit((wint_t)pString[0]))
         {
            index = wcstol(pString,
                           0,
                           0);

            pTempFilterConditions[tempConditionIndex].conditionValue.uint32 = index;
         }
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_IP_NEXTHOP_INTERFACE
      /// -ipnhi <MATCH_TYPE> <INTERFACE>
      /// -ipnhi == 
      if(HlprStringsAreEqual(L"-ipnhi",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString          = ppCLPStrings[++stringIndex];
         UINT64 nexthopInterface = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_IP_NEXTHOP_INTERFACE;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT64;

         if(iswdigit((wint_t)pString[0]))
            nexthopInterface = _wcstoui64(pString,
                                          0,
                                          0);

         HLPR_NEW(pTempFilterConditions[tempConditionIndex].conditionValue.uint64,
                  UINT64);
         HLPR_BAIL_ON_ALLOC_FAILURE(pTempFilterConditions[tempConditionIndex].conditionValue.uint64,
                                    status);

         *(pTempFilterConditions[tempConditionIndex].conditionValue.uint64) = nexthopInterface;

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_NEXTHOP_INTERFACE_TYPE
      /// -nhit <MATCH_TYPE> <TYPE>
      /// -nhit == 0
      if(HlprStringsAreEqual(L"-nhit",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];
         UINT32 type    = MAX_IF_TYPE;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_NEXTHOP_INTERFACE_TYPE;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

         if(iswdigit((wint_t)pString[0]))
            type = wcstol(pString,
                           0,
                           0);
         else
            type = PrvHlprInterfaceTypeParse(pString);

         if(type < MAX_IF_TYPE)
            pTempFilterConditions[tempConditionIndex].conditionValue.uint32 = type;
         else
         {
            status = ERROR_INVALID_PARAMETER;
         
            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_NEXTHOP_TUNNEL_TYPE
      /// -nhtt <MATCH_TYPE> <TYPE>
      /// -nhtt == 14
      if(HlprStringsAreEqual(L"-nhtt",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];
         UINT32 type    = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_NEXTHOP_TUNNEL_TYPE;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

         if(iswdigit((wint_t)pString[0]))
            type = wcstol(pString,
                           0,
                           0);
         else
            type = PrvHlprTunnelTypeParse(pString);

         if(type <= TUNNEL_TYPE_IPHTTPS)
            pTempFilterConditions[tempConditionIndex].conditionValue.uint32 = type;
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_NEXTHOP_INTERFACE_INDEX
      /// -nhii <MATCH_TYPE> <INDEX>
      /// -nhii == 0
      if(HlprStringsAreEqual(L"-nhii",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];
         UINT32 index   = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_NEXTHOP_INTERFACE_INDEX;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

         if(iswdigit((wint_t)pString[0]))
         {
            index = wcstol(pString,
                           0,
                           0);

            pTempFilterConditions[tempConditionIndex].conditionValue.uint32 = index;
         }
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_ORIGINAL_PROFILE_ID
      /// -opid <MATCH_TYPE> <PROFILE_ID>
      /// -opid == 1
      if(HlprStringsAreEqual(L"-opid",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString   = ppCLPStrings[++stringIndex];
         UINT32 profileID = NlincCategoryStateMax;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_ORIGINAL_PROFILE_ID;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

         if(iswdigit((wint_t)pString[0]))
            profileID = wcstol(pString,
                             0,
                             0);
         else
            profileID = PrvHlprProfileIDParse(pString);

         if(profileID < NlincCategoryStateMax)
            pTempFilterConditions[tempConditionIndex].conditionValue.uint32 = profileID;
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_CURRENT_PROFILE_ID
      /// -cpid <MATCH_TYPE> <PROFILE_ID>
      /// -cpid == 1
      if(HlprStringsAreEqual(L"-cpid",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString   = ppCLPStrings[++stringIndex];
         UINT32 profileID = NlincCategoryStateMax;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_CURRENT_PROFILE_ID;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

         if(iswdigit((wint_t)pString[0]))
            profileID = wcstol(pString,
                             0,
                             0);
         else
            profileID = PrvHlprProfileIDParse(pString);

         if(profileID < NlincCategoryStateMax)
            pTempFilterConditions[tempConditionIndex].conditionValue.uint32 = profileID;
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_LOCAL_INTERFACE_PROFILE_ID
      /// -lipid <MATCH_TYPE> <PROFILE_ID>
      /// -lipid == 1
      if(HlprStringsAreEqual(L"-lipid",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString   = ppCLPStrings[++stringIndex];
         UINT32 profileID = NlincCategoryStateMax;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_LOCAL_INTERFACE_PROFILE_ID;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

         if(iswdigit((wint_t)pString[0]))
            profileID = wcstol(pString,
                             0,
                             0);
         else
            profileID = PrvHlprProfileIDParse(pString);

         if(profileID < NlincCategoryStateMax)
            pTempFilterConditions[tempConditionIndex].conditionValue.uint32 = profileID;
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_ARRIVAL_INTERFACE_PROFILE_ID
      /// -aipid <MATCH_TYPE> <PROFILE_ID>
      /// -aipid == 1
      if(HlprStringsAreEqual(L"-aipid",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString   = ppCLPStrings[++stringIndex];
         UINT32 profileID = NlincCategoryStateMax;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_ARRIVAL_INTERFACE_PROFILE_ID;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

         if(iswdigit((wint_t)pString[0]))
            profileID = wcstol(pString,
                             0,
                             0);
         else
            profileID = PrvHlprProfileIDParse(pString);

         if(profileID < NlincCategoryStateMax)
            pTempFilterConditions[tempConditionIndex].conditionValue.uint32 = profileID;
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_NEXTHOP_INTERFACE_PROFILE_ID
      /// -nhipid <MATCH_TYPE> <PROFILE_ID>
      /// -nhipid == 1
      if(HlprStringsAreEqual(L"-nhipid",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString   = ppCLPStrings[++stringIndex];
         UINT32 profileID = NlincCategoryStateMax;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_NEXTHOP_INTERFACE_PROFILE_ID;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

         if(iswdigit((wint_t)pString[0]))
            profileID = wcstol(pString,
                             0,
                             0);
         else
            profileID = PrvHlprProfileIDParse(pString);

         if(profileID < NlincCategoryStateMax)
            pTempFilterConditions[tempConditionIndex].conditionValue.uint32 = profileID;
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_REAUTHORIZE_REASON
      /// -rr <MATCH_TYPE> <FLAGS>
      /// -rr == FWP_CONDITION_REAUTHORIZE_REASON_CLASSIFY_COMPLETION
      if(HlprStringsAreEqual(L"-rr",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];
         UINT32 flags   = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_REAUTHORIZE_REASON;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

         if(iswdigit((wint_t)pString[0]))
            flags = wcstol(pString,
                           0,
                           0);
         else
            flags = PrvHlprFwpConditionReauthorizeReasonParse(pString);

         pTempFilterConditions[tempConditionIndex].conditionValue.uint32 = flags;

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_EQUAL)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_FLAGS_ALL_SET;
         else if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_NOT_EQUAL)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_FLAGS_NONE_SET;
         else
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_FLAGS_ANY_SET;

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_ORIGINAL_ICMP_TYPE
      /// -oicmpt <MATCH_TYPE> <ICMP_TYPE>
      /// -oicmpt == 0
      if(HlprStringsAreEqual(L"-oicmpt",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];
         UINT32 port    = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_ORIGINAL_ICMP_TYPE;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT16;

         if(iswdigit((wint_t)pString[0]))
            port = wcstol(pString,
                          0,
                          0);

         if(port <= 0xFFFF)
            pTempFilterConditions[tempConditionIndex].conditionValue.uint16 = (UINT16)port;
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_IP_PHYSICAL_ARRIVAL_INTERFACE
      /// -ippai <MATCH_TYPE> <INTERFACE>
      /// -ippai == 
      if(HlprStringsAreEqual(L"-ippai",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString          = ppCLPStrings[++stringIndex];
         UINT64 arrivalInterface = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_IP_PHYSICAL_ARRIVAL_INTERFACE;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT64;

         if(iswdigit((wint_t)pString[0]))
            arrivalInterface = _wcstoui64(pString,
                                          0,
                                          0);

         HLPR_NEW(pTempFilterConditions[tempConditionIndex].conditionValue.uint64,
                  UINT64);
         HLPR_BAIL_ON_ALLOC_FAILURE(pTempFilterConditions[tempConditionIndex].conditionValue.uint64,
                                    status);

         *(pTempFilterConditions[tempConditionIndex].conditionValue.uint64) = arrivalInterface;

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_IP_PHYSICAL_NEXTHOP_INTERFACE
      /// -ippnhi <MATCH_TYPE> <INTERFACE>
      /// -ippnhi == 
      if(HlprStringsAreEqual(L"-ippnhi",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString          = ppCLPStrings[++stringIndex];
         UINT64 nexthopInterface = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_IP_PHYSICAL_NEXTHOP_INTERFACE;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT64;

         if(iswdigit((wint_t)pString[0]))
            nexthopInterface = _wcstoui64(pString,
                                          0,
                                          0);

         HLPR_NEW(pTempFilterConditions[tempConditionIndex].conditionValue.uint64,
                  UINT64);
         HLPR_BAIL_ON_ALLOC_FAILURE(pTempFilterConditions[tempConditionIndex].conditionValue.uint64,
                                    status);

         *(pTempFilterConditions[tempConditionIndex].conditionValue.uint64) = nexthopInterface;

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_INTERFACE_QUARANTINE_EPOCH
      /// -iqe <MATCH_TYPE> <EPOCH>
      /// -iqe == 68719476721
      if(HlprStringsAreEqual(L"-iqe",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];
         UINT64 epoch   = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_INTERFACE_QUARANTINE_EPOCH;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT64;

         if(iswdigit((wint_t)pString[0]))
            epoch = _wcstoui64(pString,
                               0,
                               0);

         HLPR_NEW(pTempFilterConditions[tempConditionIndex].conditionValue.uint64,
                  UINT64);
         HLPR_BAIL_ON_ALLOC_FAILURE(pTempFilterConditions[tempConditionIndex].conditionValue.uint64,
                                    status);

         *(pTempFilterConditions[tempConditionIndex].conditionValue.uint64) = epoch;

         tempConditionIndex++;

         continue;
      }

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)


      /// FWPM_CONDITION_IP_LOCAL_ADDRESS
      /// -ipla <MATCH_TYPE> <IP_LOCAL_ADRESS>
      /// -ipla == 1.2.3.4
      if(HlprStringsAreEqual(L"-ipla",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey = FWPM_CONDITION_IP_LOCAL_ADDRESS;

         if(HlprIPAddressV6StringIsValidFormat(pString))
         {
            pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_BYTE_ARRAY16_TYPE;

            HLPR_NEW(pTempFilterConditions[tempConditionIndex].conditionValue.byteArray16,
                     FWP_BYTE_ARRAY16);
            HLPR_BAIL_ON_ALLOC_FAILURE(pTempFilterConditions[tempConditionIndex].conditionValue.byteArray16,
                                       status);

            status = HlprIPAddressV6StringToValue(pString,
                                                  pTempFilterConditions[tempConditionIndex].conditionValue.byteArray16->byteArray16);
            HLPR_BAIL_ON_FAILURE(status);
         }
         else
         {
            pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

            status = HlprIPAddressV4StringToValue(pString,
                                                  &(pTempFilterConditions[tempConditionIndex].conditionValue.uint32));
            HLPR_BAIL_ON_FAILURE(status);
         }

         tempConditionIndex++;
         
         continue;
      }

      /// FWPM_CONDITION_IP_REMOTE_ADDRESS
      /// -ipra <MATCH_TYPE> <IP_REMOTE_ADRESS>
      /// -ipra == 1.2.3.4
      if(HlprStringsAreEqual(L"-ipra",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey = FWPM_CONDITION_IP_REMOTE_ADDRESS;

         if(HlprIPAddressV6StringIsValidFormat(pString))
         {
            pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_BYTE_ARRAY16_TYPE;

            HLPR_NEW(pTempFilterConditions[tempConditionIndex].conditionValue.byteArray16,
                     FWP_BYTE_ARRAY16);
            HLPR_BAIL_ON_ALLOC_FAILURE(pTempFilterConditions[tempConditionIndex].conditionValue.byteArray16,
                                       status);

            status = HlprIPAddressV6StringToValue(pString,
                                                  pTempFilterConditions[tempConditionIndex].conditionValue.byteArray16->byteArray16);
            HLPR_BAIL_ON_FAILURE(status);
         }
         else
         {
            pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

            status = HlprIPAddressV4StringToValue(pString,
                                                  &(pTempFilterConditions[tempConditionIndex].conditionValue.uint32));
            HLPR_BAIL_ON_FAILURE(status);
         }

         tempConditionIndex++;
         
         continue;
      }

      /// FWPM_CONDITION_IP_SOURCE_ADDRESS
      /// -ipsa <MATCH_TYPE> <IP_SOURCE_ADRESS>
      /// -ipsa == 1.2.3.4
      if(HlprStringsAreEqual(L"-ipsa",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey = FWPM_CONDITION_IP_SOURCE_ADDRESS;

         if(HlprIPAddressV6StringIsValidFormat(pString))
         {
            pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_BYTE_ARRAY16_TYPE;

            HLPR_NEW(pTempFilterConditions[tempConditionIndex].conditionValue.byteArray16,
                     FWP_BYTE_ARRAY16);
            HLPR_BAIL_ON_ALLOC_FAILURE(pTempFilterConditions[tempConditionIndex].conditionValue.byteArray16,
                                       status);

            status = HlprIPAddressV6StringToValue(pString,
                                                  pTempFilterConditions[tempConditionIndex].conditionValue.byteArray16->byteArray16);
            HLPR_BAIL_ON_FAILURE(status);
         }
         else
         {
            pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

            status = HlprIPAddressV4StringToValue(pString,
                                                  &(pTempFilterConditions[tempConditionIndex].conditionValue.uint32));
            HLPR_BAIL_ON_FAILURE(status);
         }

         tempConditionIndex++;
         
         continue;
      }

      /// FWPM_CONDITION_IP_DESTINATION_ADDRESS
      /// -ipda <MATCH_TYPE> <IP_DESTINATION_ADRESS>
      /// -ipda == 1.2.3.4
      if(HlprStringsAreEqual(L"-ipda",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey = FWPM_CONDITION_IP_DESTINATION_ADDRESS;

         if(HlprIPAddressV6StringIsValidFormat(pString))
         {
            pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_BYTE_ARRAY16_TYPE;

            HLPR_NEW(pTempFilterConditions[tempConditionIndex].conditionValue.byteArray16,
                     FWP_BYTE_ARRAY16);
            HLPR_BAIL_ON_ALLOC_FAILURE(pTempFilterConditions[tempConditionIndex].conditionValue.byteArray16,
                                       status);

            status = HlprIPAddressV6StringToValue(pString,
                                                  pTempFilterConditions[tempConditionIndex].conditionValue.byteArray16->byteArray16);
            HLPR_BAIL_ON_FAILURE(status);
         }
         else
         {
            pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

            status = HlprIPAddressV4StringToValue(pString,
                                                  &(pTempFilterConditions[tempConditionIndex].conditionValue.uint32));
            HLPR_BAIL_ON_FAILURE(status);
         }

         tempConditionIndex++;
         
         continue;
      }

      /// FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE
      /// -iplat <MATCH_TYPE> <IP_LOCAL_ADDRESS_TYPE>
      /// -iplat == NlatUnicast
      if(HlprStringsAreEqual(L"-iplat",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString       = ppCLPStrings[++stringIndex];
         UINT32 ipAddressType = IPPROTO_MAX;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT8;

         if(iswdigit((wint_t)pString[0]))
            ipAddressType = wcstol(pString,
                                    0,
                                    0);
         else
            ipAddressType = PrvHlprIPAddressTypeParse(pString);

         if(ipAddressType < IPPROTO_MAX)
            pTempFilterConditions[tempConditionIndex].conditionValue.uint8 = (UINT8)ipAddressType;
         else
         {
            status = ERROR_INVALID_PARAMETER;
         
            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_IP_DESTINATION_ADDRESS_TYPE
      /// -ipdat <MATCH_TYPE> <IP_DESTINATION_ADDRESS_TYPE>
      /// -ipdat == NlatUnicast
      if(HlprStringsAreEqual(L"-ipdat",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString       = ppCLPStrings[++stringIndex];
         UINT32 ipAddressType = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_IP_DESTINATION_ADDRESS_TYPE;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT8;

         if(iswdigit((wint_t)pString[0]))
            ipAddressType = wcstol(pString,
                                    0,
                                    0);
         else
            ipAddressType = PrvHlprIPAddressTypeParse(pString);

         if(ipAddressType < IPPROTO_MAX)
            pTempFilterConditions[tempConditionIndex].conditionValue.uint8 = (UINT8)ipAddressType;
         else
         {
            status = ERROR_INVALID_PARAMETER;
         
            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_IP_LOCAL_INTERFACE \ FWPM_CONDITION_INTERFACE
      /// -ipli <MATCH_TYPE> <INTERFACE> \ -ipi <MATCH_TYPE> <INTERFACE>
      /// -ipli == 
      if(HlprStringsAreEqual(L"-ipli",
                             ppCLPStrings[stringIndex]) ||
         HlprStringsAreEqual(L"-ipi",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString        = ppCLPStrings[++stringIndex];
         UINT64 localInterface = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_IP_LOCAL_INTERFACE;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT64;

         if(iswdigit((wint_t)pString[0]))
            localInterface = _wcstoui64(pString,
                                        0,
                                        0);

         HLPR_NEW(pTempFilterConditions[tempConditionIndex].conditionValue.uint64,
                  UINT64);
         HLPR_BAIL_ON_ALLOC_FAILURE(pTempFilterConditions[tempConditionIndex].conditionValue.uint64,
                                    status);

         *(pTempFilterConditions[tempConditionIndex].conditionValue.uint64) = localInterface;

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_IP_ARRIVAL_INTERFACE
      /// -ipai <MATCH_TYPE> <INTERFACE>
      /// -ipai == 
      if(HlprStringsAreEqual(L"-ipai",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString          = ppCLPStrings[++stringIndex];
         UINT64 arrivalInterface = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_IP_ARRIVAL_INTERFACE;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT64;

         if(iswdigit((wint_t)pString[0]))
            arrivalInterface = _wcstoui64(pString,
                                          0,
                                          0);

         HLPR_NEW(pTempFilterConditions[tempConditionIndex].conditionValue.uint64,
                  UINT64);
         HLPR_BAIL_ON_ALLOC_FAILURE(pTempFilterConditions[tempConditionIndex].conditionValue.uint64,
                                    status);

         *(pTempFilterConditions[tempConditionIndex].conditionValue.uint64) = arrivalInterface;

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_ARRIVAL_INTERFACE_TYPE
      /// -ait <MATCH_TYPE> <TYPE>
      /// -ait == 0
      if(HlprStringsAreEqual(L"-ait",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];
         UINT32 type    = MAX_IF_TYPE;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_ARRIVAL_INTERFACE_TYPE;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

         if(iswdigit((wint_t)pString[0]))
            type = wcstol(pString,
                           0,
                           0);
         else
            type = PrvHlprInterfaceTypeParse(pString);

         if(type < MAX_IF_TYPE)
            pTempFilterConditions[tempConditionIndex].conditionValue.uint32 = type;
         else
         {
            status = ERROR_INVALID_PARAMETER;
         
            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_ARRIVAL_TUNNEL_TYPE
      /// -att <MATCH_TYPE> <TYPE>
      /// -att == 14
      if(HlprStringsAreEqual(L"-att",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];
         UINT32 type    = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_ARRIVAL_TUNNEL_TYPE;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

         if(iswdigit((wint_t)pString[0]))
            type = wcstol(pString,
                           0,
                           0);
         else
            type = PrvHlprTunnelTypeParse(pString);

         if(type <= TUNNEL_TYPE_IPHTTPS)
            pTempFilterConditions[tempConditionIndex].conditionValue.uint32 = type;
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_ARRIVAL_INTERFACE_INDEX
      /// -aii <MATCH_TYPE> <INDEX>
      /// -aii == 0
      if(HlprStringsAreEqual(L"-aii",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];
         UINT32 index   = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_ARRIVAL_INTERFACE_INDEX;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

         if(iswdigit((wint_t)pString[0]))
         {
            index = wcstol(pString,
                           0,
                           0);

            pTempFilterConditions[tempConditionIndex].conditionValue.uint32 = index;
         }
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

     
      /// FWPM_CONDITION_INTERFACE_TYPE \ FWPM_CONDITION_LOCAL_INTERFACE_TYPE
      /// -it <MATCH_TYPE> <TYPE> \ -lit <MATCH_TYPE> <TYPE>
      /// -it == 0 \ -lit == 0
      if(HlprStringsAreEqual(L"-it",
                             ppCLPStrings[stringIndex]) ||
         HlprStringsAreEqual(L"-lit",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];
         UINT32 type    = MAX_IF_TYPE;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_INTERFACE_TYPE;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

         if(iswdigit((wint_t)pString[0]))
            type = wcstol(pString,
                           0,
                           0);
         else
            type = PrvHlprInterfaceTypeParse(pString);

         if(type < MAX_IF_TYPE)
            pTempFilterConditions[tempConditionIndex].conditionValue.uint32 = type;
         else
         {
            status = ERROR_INVALID_PARAMETER;
         
            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_TUNNEL_TYPE \ FWPM_CONDITION_LOCAL_TUNNEL_TYPE
      /// -tt <MATCH_TYPE> <TYPE> \ -ltt <MATCH_TYPE> <TYPE>
      /// -tt == 14 \ -ltt == 14
      if(HlprStringsAreEqual(L"-tt",
                             ppCLPStrings[stringIndex]) ||
         HlprStringsAreEqual(L"-ltt",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];
         UINT32 type    = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_TUNNEL_TYPE;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

         if(iswdigit((wint_t)pString[0]))
            type = wcstol(pString,
                           0,
                           0);
         else
            type = PrvHlprTunnelTypeParse(pString);

         if(type <= TUNNEL_TYPE_IPHTTPS)
            pTempFilterConditions[tempConditionIndex].conditionValue.uint32 = type;
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_IP_FORWARD_INTERFACE
      /// -ipfi <MATCH_TYPE> <INTERFACE>
      /// -ipfi == 
      if(HlprStringsAreEqual(L"-ipfi",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString          = ppCLPStrings[++stringIndex];
         UINT64 forwardInterface = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_IP_FORWARD_INTERFACE;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT64;

         if(iswdigit((wint_t)pString[0]))
            forwardInterface = _wcstoui64(pString,
                                          0,
                                          0);

         HLPR_NEW(pTempFilterConditions[tempConditionIndex].conditionValue.uint64,
                  UINT64);
         HLPR_BAIL_ON_ALLOC_FAILURE(pTempFilterConditions[tempConditionIndex].conditionValue.uint64,
                                    status);

         *(pTempFilterConditions[tempConditionIndex].conditionValue.uint64) = forwardInterface;

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_IP_PROTOCOL
      /// -ipp <MATCH_TYPE> <PROTOCOL>
      /// -ipp == TCP
      if(HlprStringsAreEqual(L"-ipp",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString  = ppCLPStrings[++stringIndex];
         UINT32 protocol = IPPROTO_MAX;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_IP_PROTOCOL;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT8;

         if(iswdigit((wint_t)pString[0]))
            protocol = wcstol(pString,
                              0,
                              0);
         else
            protocol = PrvHlprProtocolParse(pString);

         if(protocol <= 0xFF)
            pTempFilterConditions[tempConditionIndex].conditionValue.uint8 = (UINT8)protocol;
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_IP_LOCAL_PORT \ FWPM_CONDITION_ICMP_TYPE
      /// -iplp <MATCH_TYPE> <IP_LOCAL_PORT> \ -icmpt <MATCH_TYPE> <ICMP_TYPE>
      /// -iplp == 80
      if(HlprStringsAreEqual(L"-iplp",
                             ppCLPStrings[stringIndex]) ||
         HlprStringsAreEqual(L"-icmpt",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];
         UINT32 port    = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_IP_LOCAL_PORT;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT16;

         if(iswdigit((wint_t)pString[0]))
            port = wcstol(pString,
                          0,
                          0);

         if(port <= 0xFFFF)
            pTempFilterConditions[tempConditionIndex].conditionValue.uint16 = (UINT16)(port & 0xFFFF);

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_IP_REMOTE_PORT \ FWPM_CONDITION_ICMP_CODE
      /// -iprp <MATCH_TYPE> <IP_REMOTE_PORT> \ -icmpc <MATCH_TYPE> <ICMP_CODE>
      /// -iprp == 80
      if(HlprStringsAreEqual(L"-iprp",
                             ppCLPStrings[stringIndex]) ||
         HlprStringsAreEqual(L"-icmpc",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];
         UINT32 port    = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_IP_REMOTE_PORT;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT16;

         if(iswdigit((wint_t)pString[0]))
            port = wcstol(pString,
                          0,
                          0);

         if(port <= 0xFFFF)
            pTempFilterConditions[tempConditionIndex].conditionValue.uint16 = (UINT16)port;
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_EMBEDDED_LOCAL_ADDRESS_TYPE
      /// -elat <MATCH_TYPE> <EMBEDDED_LOCAL_ADDRESS_TYPE>
      /// -elat == NlatUnicast
      if(HlprStringsAreEqual(L"-elat",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString       = ppCLPStrings[++stringIndex];
         UINT32 ipAddressType = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_EMBEDDED_LOCAL_ADDRESS_TYPE;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT8;

         if(iswdigit((wint_t)pString[0]))
            ipAddressType = wcstol(pString,
                                    0,
                                    0);
         else
            ipAddressType = PrvHlprIPAddressTypeParse(pString);

         pTempFilterConditions[tempConditionIndex].conditionValue.uint8 = (UINT8)ipAddressType;

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_EMBEDDED_REMOTE_ADDRESS
      /// -era <MATCH_TYPE> <EMBEDDED_REMOTE_ADRESS>
      /// -era == 1.2.3.4
      if(HlprStringsAreEqual(L"-era",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey = FWPM_CONDITION_EMBEDDED_REMOTE_ADDRESS;

         if(HlprIPAddressV6StringIsValidFormat(pString))
         {
            pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_BYTE_ARRAY16_TYPE;

            HLPR_NEW(pTempFilterConditions[tempConditionIndex].conditionValue.byteArray16,
                     FWP_BYTE_ARRAY16);
            HLPR_BAIL_ON_ALLOC_FAILURE(pTempFilterConditions[tempConditionIndex].conditionValue.byteArray16,
                                       status);

            status = HlprIPAddressV6StringToValue(pString,
                                                  pTempFilterConditions[tempConditionIndex].conditionValue.byteArray16->byteArray16);
            HLPR_BAIL_ON_FAILURE(status);
         }
         else
         {
            pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

            status = HlprIPAddressV4StringToValue(pString,
                                                  &(pTempFilterConditions[tempConditionIndex].conditionValue.uint32));
            HLPR_BAIL_ON_FAILURE(status);
         }

         tempConditionIndex++;
         
         continue;
      }

      /// FWPM_CONDITION_EMBEDDED_PROTOCOL
      /// -ep <MATCH_TYPE> <PROTOCOL>
      /// -ep == TCP
      if(HlprStringsAreEqual(L"-ep",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString  = ppCLPStrings[++stringIndex];
         UINT32 protocol = IPPROTO_MAX;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_EMBEDDED_PROTOCOL;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT8;

         if(iswdigit((wint_t)pString[0]))
            protocol = wcstol(pString,
                              0,
                              0);
         else
            protocol = PrvHlprProtocolParse(pString);

         if(protocol <= 0xFF)
            pTempFilterConditions[tempConditionIndex].conditionValue.uint8 = (UINT8)(protocol & 0xFF);
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_EMBEDDED_LOCAL_PORT
      /// -elp <MATCH_TYPE> <EMBEDDED_LOCAL_PORT>
      /// -elp == 80
      if(HlprStringsAreEqual(L"-elp",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];
         UINT32 port    = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_EMBEDDED_LOCAL_PORT;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT16;

         if(iswdigit((wint_t)pString[0]))
            port = wcstol(pString,
                          0,
                          0);

         if(port <= 0xFFFF)
            pTempFilterConditions[tempConditionIndex].conditionValue.uint16 = (UINT16)port;
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_EMBEDDED_REMOTE_PORT
      /// -erp <MATCH_TYPE> <EMBEDDED_REMOTE_PORT>
      /// -erp == 80
      if(HlprStringsAreEqual(L"-erp",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];
         UINT32 port    = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_EMBEDDED_REMOTE_PORT;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT16;

         if(iswdigit((wint_t)pString[0]))
            port = wcstol(pString,
                          0,
                          0);

         if(port <= 0xFFFF)
            pTempFilterConditions[tempConditionIndex].conditionValue.uint16 = (UINT16)port;
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_FLAGS
      /// -f <MATCH_TYPE> <FLAGS>
      /// -f == FWP_CONDITION_FLAG_IS_LOOPBACK
      if(HlprStringsAreEqual(L"-f",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];
         UINT32 flags   = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_FLAGS;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

         if(iswdigit((wint_t)pString[0]))
            flags = wcstol(pString,
                           0,
                           0);
         else
            flags = PrvHlprFwpConditionFlagParse(pString);

         pTempFilterConditions[tempConditionIndex].conditionValue.uint32 = flags;

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_EQUAL)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_FLAGS_ALL_SET;
         else if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_NOT_EQUAL)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_FLAGS_NONE_SET;
         else
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_FLAGS_ANY_SET;

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_DIRECTION
      /// -d <MATCH_TYPE> <DIRECTION>
      /// -d == 0
      if(HlprStringsAreEqual(L"-d",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString   = ppCLPStrings[++stringIndex];
         UINT32 direction = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_DIRECTION;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

         if(iswdigit((wint_t)pString[0]))
            direction = wcstol(pString,
                               0,
                               0);
         else
            direction = PrvHlprFwpDirectionParse(pString);

         if(direction < FWP_DIRECTION_MAX)
            pTempFilterConditions[tempConditionIndex].conditionValue.uint32 = direction;
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }


      /// FWPM_CONDITION_INTERFACE_INDEX \ FWPM_CONDITION_LOCAL_INTERFACE_INDEX
      /// -ii <MATCH_TYPE> <INDEX> \ -lii <MATCH_TYPE> <INDEX>
      /// -ii == 0 \ -lii == 0
      if(HlprStringsAreEqual(L"-ii",
                             ppCLPStrings[stringIndex]) ||
         HlprStringsAreEqual(L"-lii",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];
         UINT32 index   = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_INTERFACE_INDEX;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

         if(iswdigit((wint_t)pString[0]))
         {
            index = wcstol(pString,
                           0,
                           0);

            pTempFilterConditions[tempConditionIndex].conditionValue.uint32 = index;
         }
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_SUB_INTERFACE_INDEX \ FWPM_CONDITION_ARRIVAL_SUB_INTERFACE_INDEX
      /// -sii <MATCH_TYPE> <INDEX> \ -asii <MATCH_TYPE> <INDEX>
      /// -sii == 0 \ -asii == 0
      if(HlprStringsAreEqual(L"-sii",
                             ppCLPStrings[stringIndex]) ||
         HlprStringsAreEqual(L"-asii",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];
         UINT32 index   = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_SUB_INTERFACE_INDEX;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

         if(iswdigit((wint_t)pString[0]))
         {
            index = wcstol(pString,
                           0,
                           0);

            pTempFilterConditions[tempConditionIndex].conditionValue.uint32 = index;
         }
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_SOURCE_INTERFACE_INDEX
      /// -sii <MATCH_TYPE> <INDEX>
      /// -sii == 0
      if(HlprStringsAreEqual(L"-sii",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];
         UINT32 index   = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_SOURCE_INTERFACE_INDEX;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

         if(iswdigit((wint_t)pString[0]))
         {
            index = wcstol(pString,
                           0,
                           0);

            pTempFilterConditions[tempConditionIndex].conditionValue.uint32 = index;
         }
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_SOURCE_SUB_INTERFACE_INDEX
      /// -ssii <MATCH_TYPE> <INDEX>
      /// -ssii == 0
      if(HlprStringsAreEqual(L"-ssii",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];
         UINT32 index   = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_SOURCE_SUB_INTERFACE_INDEX;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

         if(iswdigit((wint_t)pString[0]))
         {
            index = wcstol(pString,
                           0,
                           0);

            pTempFilterConditions[tempConditionIndex].conditionValue.uint32 = index;
         }
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_DESTINATION_INTERFACE_INDEX
      /// -dii <MATCH_TYPE> <INDEX>
      /// -dii == 0
      if(HlprStringsAreEqual(L"-dii",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];
         UINT32 index   = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_DESTINATION_INTERFACE_INDEX;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

         if(iswdigit((wint_t)pString[0]))
         {
            index = wcstol(pString,
                           0,
                           0);

            pTempFilterConditions[tempConditionIndex].conditionValue.uint32 = index;
         }
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_DESTINATION_SUB_INTERFACE_INDEX
      /// -dsii <MATCH_TYPE> <INDEX>
      /// -dsii == 0
      if(HlprStringsAreEqual(L"-dsii",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];
         UINT32 index   = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_DESTINATION_SUB_INTERFACE_INDEX;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

         if(iswdigit((wint_t)pString[0]))
         {
            index = wcstol(pString,
                           0,
                           0);

            pTempFilterConditions[tempConditionIndex].conditionValue.uint32 = index;
         }
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_ALE_APP_ID
      /// -aaid <MATCH_TYPE> <APPLICATION_NAME>
      /// -aaid == IExplore.exe
      if(HlprStringsAreEqual(L"-aaid",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR         pString   = ppCLPStrings[++stringIndex];
         FWP_BYTE_BLOB* pAppID    = 0;
         FWP_BYTE_BLOB* pByteBlob = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey = FWPM_CONDITION_ALE_APP_ID;

         status = FwpmGetAppIdFromFileName(pString,
                                           &pAppID);
         if(status != NO_ERROR)
         {
            HLPR_DELETE(pByteBlob);

            HLPR_BAIL;
         }
         else
         {
            HLPR_NEW(pByteBlob,
                     FWP_BYTE_BLOB);
            HLPR_BAIL_ON_ALLOC_FAILURE(pByteBlob,
                                       status);

            HLPR_NEW_ARRAY(pByteBlob->data,
                           BYTE,
                           pAppID->size);
            HLPR_BAIL_ON_ALLOC_FAILURE(pByteBlob->data,
                                       status);

            RtlCopyMemory(pByteBlob->data,
                          pAppID->data,
                          pAppID->size);

            pByteBlob->size = pAppID->size;

            pTempFilterConditions[tempConditionIndex].conditionValue.type     = FWP_BYTE_BLOB_TYPE;
            pTempFilterConditions[tempConditionIndex].conditionValue.byteBlob = pByteBlob;

            FwpmFreeMemory((VOID**)&pAppID);

            pAppID = 0;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_ALE_USER_ID
      /// -auid <MATCH_TYPE> <USER>
      /// -auid == Domain\UserName
      if(HlprStringsAreEqual(L"-auid",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey = FWPM_CONDITION_ALE_USER_ID;

         if(forEnum)
         {
            PSID pSID = 0;

            status = PrvHlprSIDGet(pString,
                                   &pSID);
            HLPR_BAIL_ON_FAILURE(status);

            pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_SID;
            pTempFilterConditions[tempConditionIndex].conditionValue.sid  = (SID*)pSID;
         }
         else
         {
            UINT32               length                          = 0;
            PSECURITY_DESCRIPTOR pAppContainerSecurityDescriptor = 0;
            PSECURITY_DESCRIPTOR pSecurityDescriptor             = 0;
            EXPLICIT_ACCESS      explicitAccess                  = {0};

            BuildExplicitAccessWithName(&explicitAccess,
                                        (LPWSTR)pString,
                                        FWP_ACTRL_MATCH_FILTER,
                                        GRANT_ACCESS,
                                        0);

#if(NTDDI_VERSION >= NTDDI_WIN8)

            if(AppliesToAppContainers(ppCLPStrings,
                                      stringCount))
               pAppContainerSecurityDescriptor = PrvHlprCreateAppContainerSecurityDescriptor();

#endif /// (NTDDI_VERSION >= NTDDI_WIN8)

            /// Returns the self-relative security descriptor
            status = BuildSecurityDescriptor(0,
                                             0,
                                             1,
                                             &explicitAccess,
                                             0,
                                             0,
                                             pAppContainerSecurityDescriptor,
                                             (PULONG)&length,
                                             &pSecurityDescriptor);

            HLPR_DELETE_ARRAY(pAppContainerSecurityDescriptor)

            if(status != NO_ERROR)
            {
               if(status == ERROR_NONE_MAPPED)
                  HlprLogInfo(L"FWPM_CONDITION_ALE_USER_ID: UserName %s Not Found",
                              pString);

               status = ERROR_INVALID_PARAMETER;

               HLPR_BAIL;
            }

            HLPR_NEW(pTempFilterConditions[tempConditionIndex].conditionValue.sd,
                     FWP_BYTE_BLOB);
            if(pTempFilterConditions[tempConditionIndex].conditionValue.sd == 0)
            {
               LocalFree(pSecurityDescriptor);

               HLPR_BAIL;
            }

            HLPR_NEW_ARRAY(pTempFilterConditions[tempConditionIndex].conditionValue.sd->data,
                           BYTE,
                           length);
            if(pTempFilterConditions[tempConditionIndex].conditionValue.sd->data == 0)
            {
               LocalFree(pSecurityDescriptor);

               HLPR_DELETE(pTempFilterConditions[tempConditionIndex].conditionValue.sd);

               HLPR_BAIL;
            }

            RtlCopyMemory(pTempFilterConditions[tempConditionIndex].conditionValue.sd->data,
                          pSecurityDescriptor,
                          length);

            LocalFree(pSecurityDescriptor);

            pTempFilterConditions[tempConditionIndex].conditionValue.type     = FWP_SECURITY_DESCRIPTOR_TYPE;
            pTempFilterConditions[tempConditionIndex].conditionValue.sd->size = length;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_ALE_REMOTE_USER_ID
      /// -aruid <MATCH_ID> <USER>
      /// -aruid == Domain\UserName
      if(HlprStringsAreEqual(L"-aruid",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR               pString             = ppCLPStrings[++stringIndex];
         UINT32               length              = 0;
         PSECURITY_DESCRIPTOR pSecurityDescriptor = 0;
         EXPLICIT_ACCESS      explicitAccess      = {0};

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey = FWPM_CONDITION_ALE_REMOTE_USER_ID;

         if(forEnum)
         {
            PSID pSID = 0;

            status = PrvHlprSIDGet(pString,
                                   &pSID);
            HLPR_BAIL_ON_FAILURE(status);

            pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_SID;
            pTempFilterConditions[tempConditionIndex].conditionValue.sid  = (SID*)pSID;
         }
         else
         {
            BuildExplicitAccessWithName(&explicitAccess,
                                        (LPWSTR)pString,
                                        FWP_ACTRL_MATCH_FILTER,
                                        GRANT_ACCESS,
                                        0);

            /// Returns the self-relative security descriptor
            status = BuildSecurityDescriptor(0,
                                             0,
                                             1,
                                             &explicitAccess,
                                             0,
                                             0,
                                             0,
                                             (PULONG)&length,
                                             &pSecurityDescriptor);
            if(status != NO_ERROR)
            {
               if(status == ERROR_NONE_MAPPED)
                  HlprLogInfo(L"FWPM_CONDITION_ALE_REMOTE_USER_ID: UserName %s Not Found",
                              pString);

               status = ERROR_INVALID_PARAMETER;

               HLPR_BAIL;
            }

            HLPR_NEW(pTempFilterConditions[tempConditionIndex].conditionValue.sd,
                     FWP_BYTE_BLOB);
            if(pTempFilterConditions[tempConditionIndex].conditionValue.sd == 0)
            {
               LocalFree(pSecurityDescriptor);

               HLPR_BAIL;
            }

            HLPR_NEW_ARRAY(pTempFilterConditions[tempConditionIndex].conditionValue.sd->data,
                           BYTE,
                           length);
            if(pTempFilterConditions[tempConditionIndex].conditionValue.sd->data == 0)
            {
               LocalFree(pSecurityDescriptor);

               HLPR_DELETE(pTempFilterConditions[tempConditionIndex].conditionValue.sd);

               HLPR_BAIL;
            }

            RtlCopyMemory(pTempFilterConditions[tempConditionIndex].conditionValue.sd->data,
                          pSecurityDescriptor,
                          length);

            LocalFree(pSecurityDescriptor);

            pTempFilterConditions[tempConditionIndex].conditionValue.type     = forEnum ? FWP_SID : FWP_SECURITY_DESCRIPTOR_TYPE;
            pTempFilterConditions[tempConditionIndex].conditionValue.sd->size = length;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_ALE_REMOTE_MACHINE_ID
      /// -armid <MATCH_TYPE> <MACHINE>
      /// -armid == MachineName
      if(HlprStringsAreEqual(L"-armid",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR               pString             = ppCLPStrings[++stringIndex];
         UINT32               length              = 0;
         PSECURITY_DESCRIPTOR pSecurityDescriptor = 0;
         EXPLICIT_ACCESS      explicitAccess      = {0};

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey = FWPM_CONDITION_ALE_REMOTE_MACHINE_ID;

         if(forEnum)
         {
            PSID pSID = 0;

            status = PrvHlprSIDGet(pString,
                                   &pSID);
            HLPR_BAIL_ON_FAILURE(status);

            pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_SID;
            pTempFilterConditions[tempConditionIndex].conditionValue.sid  = (SID*)pSID;
         }
         else
         {
            BuildExplicitAccessWithName(&explicitAccess,
                                        (LPWSTR)pString,
                                        FWP_ACTRL_MATCH_FILTER,
                                        GRANT_ACCESS,
                                        0);

            /// Returns the self-relative security descriptor
            status = BuildSecurityDescriptor(0,
                                             0,
                                             1,
                                             &explicitAccess,
                                             0,
                                             0,
                                             0,
                                             (PULONG)&length,
                                             &pSecurityDescriptor);
            if(status != NO_ERROR)
            {
               if(status == ERROR_NONE_MAPPED)
                  HlprLogInfo(L"FWPM_CONDITION_ALE_REMOTE_MACHINE_ID: MachineName %s Not Found",
                              pString);

               status = ERROR_INVALID_PARAMETER;

               HLPR_BAIL;
            }

            HLPR_NEW(pTempFilterConditions[tempConditionIndex].conditionValue.sd,
                     FWP_BYTE_BLOB);
            if(pTempFilterConditions[tempConditionIndex].conditionValue.sd == 0)
            {
               LocalFree(pSecurityDescriptor);

               HLPR_BAIL;
            }

            HLPR_NEW_ARRAY(pTempFilterConditions[tempConditionIndex].conditionValue.sd->data,
                           BYTE,
                           length);
            if(pTempFilterConditions[tempConditionIndex].conditionValue.sd->data == 0)
            {
               LocalFree(pSecurityDescriptor);

               HLPR_DELETE(pTempFilterConditions[tempConditionIndex].conditionValue.sd);

               HLPR_BAIL;
            }

            RtlCopyMemory(pTempFilterConditions[tempConditionIndex].conditionValue.sd->data,
                          pSecurityDescriptor,
                          length);

            LocalFree(pSecurityDescriptor);

            pTempFilterConditions[tempConditionIndex].conditionValue.type     = forEnum ? FWP_SID : FWP_SECURITY_DESCRIPTOR_TYPE;
            pTempFilterConditions[tempConditionIndex].conditionValue.sd->size = length;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_ALE_PROMISCUOUS_MODE
      /// -apm <MATCH_TYPE> <CONTEXT>
      /// -apm == 0
      if(HlprStringsAreEqual(L"-apm",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];
         UINT32 context = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_ALE_PROMISCUOUS_MODE;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

         if(iswdigit((wint_t)pString[0]))
         {
            context = wcstol(pString,
                             0,
                             0);

            pTempFilterConditions[tempConditionIndex].conditionValue.uint32 = context;
         }
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_ALE_SIO_FIREWALL_SYSTEM_PORT \ FWPM_CONDITION_ALE_SIO_FIREWALL_SOCKET_PROPERTY
      /// -asfsp <MATCH_TYPE> <PORT>
      /// -asfsp == 0
      if(HlprStringsAreEqual(L"-asfsp",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];
         UINT32 context = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_ALE_SIO_FIREWALL_SYSTEM_PORT;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

         if(iswdigit((wint_t)pString[0]))
         {
            context = wcstol(pString,
                             0,
                             0);

            pTempFilterConditions[tempConditionIndex].conditionValue.uint32 = context;
         }
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_ALE_NAP_CONTEXT
      /// -anc <MATCH_TYPE> <CONTEXT>
      /// -anc == 0
      if(HlprStringsAreEqual(L"-anc",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];
         UINT32 context = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_ALE_NAP_CONTEXT;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

         if(iswdigit((wint_t)pString[0]))
         {
            context = wcstol(pString,
                             0,
                             0);

            pTempFilterConditions[tempConditionIndex].conditionValue.uint32 = context;
         }
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

/// These can be implemented later as they apply only to UM layers and none of these are demo'd yet

      /// FWPM_CONDITION_REMOTE_USER_TOKEN
      /// FWPM_CONDITION_RPC_IF_UUID

      /// FWPM_CONDITION_RPC_IF_VERSION
      /// -riv <MATCH_TYPE> <VERSION>
      /// -riv == 1
      if(HlprStringsAreEqual(L"-riv",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];
         UINT32 version = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_RPC_IF_VERSION;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT16;

         if(iswdigit((wint_t)pString[0]))
            version = wcstol(pString,
                             0,
                             0);
         else
            HLPR_BAIL;

         if(version <= 0xFFFF)
            pTempFilterConditions[tempConditionIndex].conditionValue.uint16 = (UINT16)version;
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_RPC_IF_FLAG
      /// -rif <MATCH_TYPE> <FLAGS>
      /// -rif == 1
      if(HlprStringsAreEqual(L"-rif",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];
         UINT32 flag    = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_RPC_IF_FLAG;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

         if(iswdigit((wint_t)pString[0]))
            flag = wcstol(pString,
                          0,
                          0);
         else
            flag = PrvHlprRPCIFFlagParse(pString);

         pTempFilterConditions[tempConditionIndex].conditionValue.uint32 = flag;

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_DCOM_APP_ID
      /// FWPM_CONDITION_IMAGE_NAME

      /// FWPM_CONDITION_RPC_PROTOCOL
      /// -rp <MATCH_TYPE> <RPC_PROTOCOL>
      /// -rp == NCACN_IP_TCP
      if(HlprStringsAreEqual(L"-rp",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString  = ppCLPStrings[++stringIndex];
         UINT32 protocol = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_RPC_PROTOCOL;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT8;

         if(iswdigit((wint_t)pString[0]))
            protocol = wcstol(pString,
                              0,
                              0);
         else
            protocol = PrvHlprRPCProtocolParse(pString);

         if(protocol <= 0xFF)
            pTempFilterConditions[tempConditionIndex].conditionValue.uint8 = (UINT8)protocol;
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_RPC_AUTH_TYPE
      /// -rat <MATCH_TYPE> <RPC_AUTH_TYPE>
      /// -rat == WinNT
      if(HlprStringsAreEqual(L"-rat",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString  = ppCLPStrings[++stringIndex];
         UINT32 authType = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_RPC_AUTH_TYPE;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT8;

         if(iswdigit((wint_t)pString[0]))
            authType = wcstol(pString,
                              0,
                              0);
         else
            authType = PrvHlprRPCAuthTypeParse(pString);

         if(authType <= 0xFF)
            pTempFilterConditions[tempConditionIndex].conditionValue.uint8 = (UINT8)authType;
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_RPC_AUTH_LEVEL
      /// -ral <MATCH_TYPE> <RPC_AUTH_LEVEL>
      /// -ral == Default
      if(HlprStringsAreEqual(L"-ral",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString   = ppCLPStrings[++stringIndex];
         UINT32 authLevel = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_RPC_AUTH_LEVEL;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT8;

         if(iswdigit((wint_t)pString[0]))
            authLevel = wcstol(pString,
                               0,
                               0);
         else
            authLevel = PrvHlprRPCAuthLevelParse(pString);

         if(authLevel <= 0xFF)
            pTempFilterConditions[tempConditionIndex].conditionValue.uint8 = (UINT8)authLevel;
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_SEC_ENCRYPT_ALGORITHM
      /// -sea <MATCH_TYPE> <ALGORITHM>
      /// -sea == 1
      if(HlprStringsAreEqual(L"-sea",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString   = ppCLPStrings[++stringIndex];
         UINT32 algorithm = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_SEC_ENCRYPT_ALGORITHM;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

         if(iswdigit((wint_t)pString[0]))
         {
            algorithm = wcstol(pString,
                               0,
                               0);

            pTempFilterConditions[tempConditionIndex].conditionValue.uint32 = algorithm;
         }
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_SEC_KEY_SIZE
      /// -sks <MATCH_TYPE> <SIZE>
      /// -sks == 128
      if(HlprStringsAreEqual(L"-sks",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];
         UINT32 keySize = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_SEC_KEY_SIZE;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

         if(iswdigit((wint_t)pString[0]))
         {
            keySize = wcstol(pString,
                             0,
                             0);

            pTempFilterConditions[tempConditionIndex].conditionValue.uint32 = keySize;
         }
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_IP_LOCAL_ADDRESS_V4
      /// -iplav4 <MATCH_TYPE> <IP_LOCAL_ADRESS>
      /// -iplav4 == 1.2.3.4
      if(HlprStringsAreEqual(L"-iplav4",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey = FWPM_CONDITION_IP_LOCAL_ADDRESS_V4;

         if(HlprIPAddressV4StringIsValidFormat(pString))
         {
            pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

            status = HlprIPAddressV4StringToValue(pString,
                                                  &(pTempFilterConditions[tempConditionIndex].conditionValue.uint32));
            HLPR_BAIL_ON_FAILURE(status);
         }

         tempConditionIndex++;
         
         continue;
      }

      /// FWPM_CONDITION_IP_LOCAL_ADDRESS_V6
      /// -iplav6 <MATCH_TYPE> <IP_LOCAL_ADRESS_V6>
      /// -iplav6 == 1:2:3:4:5:6:7:8
      if(HlprStringsAreEqual(L"-iplav6",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey = FWPM_CONDITION_IP_LOCAL_ADDRESS_V6;

         if(HlprIPAddressV6StringIsValidFormat(pString))
         {
            pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_BYTE_ARRAY16_TYPE;

            HLPR_NEW(pTempFilterConditions[tempConditionIndex].conditionValue.byteArray16,
                     FWP_BYTE_ARRAY16);
            HLPR_BAIL_ON_ALLOC_FAILURE(pTempFilterConditions[tempConditionIndex].conditionValue.byteArray16,
                                       status);

            status = HlprIPAddressV6StringToValue(pString,
                                                  pTempFilterConditions[tempConditionIndex].conditionValue.byteArray16->byteArray16);
            HLPR_BAIL_ON_FAILURE(status);
         }

         tempConditionIndex++;
         
         continue;
      }

      /// FWPM_CONDITION_PIPE

      /// FWPM_CONDITION_IP_REMOTE_ADDRESS_V4
      /// -iprav4 <MATCH_TYPE> <IP_REMOTE_ADRESS_V4>
      /// -iprav4 == 127.0.0.1
      if(HlprStringsAreEqual(L"-iprav4",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey = FWPM_CONDITION_IP_REMOTE_ADDRESS_V4;

         if(HlprIPAddressV4StringIsValidFormat(pString))
         {
            pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

            status = HlprIPAddressV4StringToValue(pString,
                                                  &(pTempFilterConditions[tempConditionIndex].conditionValue.uint32));
            HLPR_BAIL_ON_FAILURE(status);
         }

         tempConditionIndex++;
         
         continue;
      }

      /// FWPM_CONDITION_IP_REMOTE_ADDRESS_V6
      /// -iprav6 <MATCH_TYPE> <IP_REMOTE_ADRESS_V6>
      /// -iprav6 == 1:2:3:4:5:6:7:8
      if(HlprStringsAreEqual(L"-iprav6",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey = FWPM_CONDITION_IP_REMOTE_ADDRESS_V6;

         if(HlprIPAddressV6StringIsValidFormat(pString))
         {
            pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_BYTE_ARRAY16_TYPE;

            HLPR_NEW(pTempFilterConditions[tempConditionIndex].conditionValue.byteArray16,
                     FWP_BYTE_ARRAY16);
            HLPR_BAIL_ON_ALLOC_FAILURE(pTempFilterConditions[tempConditionIndex].conditionValue.byteArray16,
                                       status);

            status = HlprIPAddressV6StringToValue(pString,
                                                  pTempFilterConditions[tempConditionIndex].conditionValue.byteArray16->byteArray16);
            HLPR_BAIL_ON_FAILURE(status);
         }

         tempConditionIndex++;
         
         continue;
      }

      /// FWPM_CONDITION_PROCESS_WITH_RPC_IF_UUID
      /// FWPM_CONDITION_RPC_EP_VALUE

      /// FWPM_CONDITION_RPC_EP_FLAGS
      /// -ref <MATCH_TYPE> <FLAGS>
      /// -ref == 1
      if(HlprStringsAreEqual(L"-ref",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];
         UINT32 flag    = 0;
      
         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);
      
         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }
      
         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_RPC_EP_FLAGS;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;
      
         if(iswdigit((wint_t)pString[0]))
            flag = wcstol(pString,
                          0,
                          0);
         else
            flag = PrvHlprRPCIFFlagParse(pString);
      
         pTempFilterConditions[tempConditionIndex].conditionValue.uint32 = flag;
      
         tempConditionIndex++;
      
         continue;
      }

      /// FWPM_CONDITION_CLIENT_TOKEN
      /// FWPM_CONDITION_RPC_SERVER_NAME

      /// FWPM_CONDITION_RPC_SERVER_PORT
      /// -rsp <MATCH_TYPE> <PORT>
      /// -rsp == 1
      if(HlprStringsAreEqual(L"-rsp",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];
         UINT32 port    = 0xFFFFFFFF;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_RPC_SERVER_PORT;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT16;

         if(iswdigit((wint_t)pString[0]))
            port = wcstol(pString,
                          0,
                          0);

         if(port <= 0xFFFF)
            pTempFilterConditions[tempConditionIndex].conditionValue.uint16 = (UINT16)port;
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_RPC_PROXY_AUTH_TYPE

      /// FWPM_CONDITION_CLIENT_CERT_KEY_LENGTH
      /// -cckl <MATCH_TYPE> <LENGTH>
      /// -cckl == 1
      if(HlprStringsAreEqual(L"-cckl",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString   = ppCLPStrings[++stringIndex];
         UINT32 keyLength = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_CLIENT_CERT_KEY_LENGTH;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

         if(iswdigit((wint_t)pString[0]))
         {
            keyLength = wcstol(pString,
                               0,
                               0);

            pTempFilterConditions[tempConditionIndex].conditionValue.uint32 = keyLength;
         }
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_CLIENT_CERT_OID
      /// FWPM_CONDITION_NET_EVENT_TYPE

#if(NTDDI_VERSION >= NTDDI_WIN7)

      /// FWPM_CONDITION_KM_AUTH_NAP_CONTEXT
      /// -kanc <MATCH_TYPE> <CONTEXT>
      /// -kanc == 1
      if(HlprStringsAreEqual(L"-kanc",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];
         UINT32 context = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_KM_AUTH_NAP_CONTEXT;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

         if(iswdigit((wint_t)pString[0]))
         {
            context = wcstol(pString,
                             0,
                             0);

            pTempFilterConditions[tempConditionIndex].conditionValue.uint32 = context;
         }
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_PEER_NAME
      /// FWPM_CONDITION_REMOTE_ID

      /// FWPM_CONDITION_AUTHENTICATION_TYPE
      /// -at <MATCH_TYPE> <TYPE>
      /// -at == 1
      if(HlprStringsAreEqual(L"-at",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];
         UINT32 type    = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_AUTHENTICATION_TYPE;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

         if(iswdigit((wint_t)pString[0]))
         {
            type = wcstol(pString,
                          0,
                          0);

            pTempFilterConditions[tempConditionIndex].conditionValue.uint32 = type;
         }
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_KM_TYPE
      /// -kt <MATCH_TYPE> <TYPE>
      /// -kt == 1
      if(HlprStringsAreEqual(L"-kt",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];
         UINT32 type    = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_KM_TYPE;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

         if(iswdigit((wint_t)pString[0]))
         {
            type = wcstol(pString,
                          0,
                          0);

            pTempFilterConditions[tempConditionIndex].conditionValue.uint32 = type;
         }
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_KM_MODE
      /// -km <MATCH_TYPE> <MODE>
      /// -km == 1
      if(HlprStringsAreEqual(L"-km",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];
         UINT32 mode    = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_KM_MODE;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

         if(iswdigit((wint_t)pString[0]))
         {
            mode = wcstol(pString,
                          0,
                          0);

            pTempFilterConditions[tempConditionIndex].conditionValue.uint32 = mode;
         }
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_IPSEC_POLICY_KEY

#if(NTDDI_VERSION >= NTDDI_WIN8)

      /// FWPM_CONDITION_QM_MODE
      /// -qm <MATCH_TYPE> <MODE>
      /// -qm == 1
      if(HlprStringsAreEqual(L"-qm",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR pString = ppCLPStrings[++stringIndex];
         UINT32 mode    = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey            = FWPM_CONDITION_QM_MODE;
         pTempFilterConditions[tempConditionIndex].conditionValue.type = FWP_UINT32;

         if(iswdigit((wint_t)pString[0]))
         {
            mode = wcstol(pString,
                          0,
                          0);

            pTempFilterConditions[tempConditionIndex].conditionValue.uint32 = mode;
         }
         else
         {
            status = ERROR_INVALID_PARAMETER;

            HLPR_BAIL;
         }

         tempConditionIndex++;

         continue;
      }

#if(NTDDI_VERSION >= NTDDI_WINTHRESHOLD)

      /// FWPM_CONDITION_ALE_SECURITY_ATTRIBUTE_FQBN_VALUE
      /// -asfv <MATCH_TYPE> <APPLICATION_NAME>
      /// -asfv == "CN="
      if(HlprStringsAreEqual(L"-asafv",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR         pString    = ppCLPStrings[++stringIndex];
         FWP_BYTE_BLOB* pByteBlob  = 0;
         size_t         stringSize = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey = FWPM_CONDITION_IPSEC_SECURITY_REALM_ID;

         status = StringCbLength(pString,
                                 STRSAFE_MAX_CCH * sizeof(WCHAR),
                                 &stringSize);
         if(FAILED(status))
            HLPR_BAIL;
         else
         {
            HLPR_NEW(pByteBlob,
                     FWP_BYTE_BLOB);
            HLPR_BAIL_ON_ALLOC_FAILURE(pByteBlob,
                                       status);

            HLPR_NEW_ARRAY(pByteBlob->data,
                           BYTE,
                           stringSize);
            HLPR_BAIL_ON_ALLOC_FAILURE(pByteBlob->data,
                                      status);

            RtlCopyMemory(pByteBlob->data,
                          pString,
                          stringSize);
   
            pByteBlob->size = (UINT32)stringSize;
   
            pTempFilterConditions[tempConditionIndex].conditionValue.type     = FWP_BYTE_BLOB_TYPE;
            pTempFilterConditions[tempConditionIndex].conditionValue.byteBlob = pByteBlob;
         }

         tempConditionIndex++;

         continue;
      }

      /// FWPM_CONDITION_IPSEC_SECURITY_REALM_ID
      /// -isrid <MATCH_TYPE> <APPLICATION_NAME>
      /// -isrid == MyRealm
      if(HlprStringsAreEqual(L"-isrid",
                             ppCLPStrings[stringIndex]))
      {
         PCWSTR         pString    = ppCLPStrings[++stringIndex];
         FWP_BYTE_BLOB* pByteBlob  = 0;
         size_t         stringSize = 0;

         pTempFilterConditions[tempConditionIndex].matchType = PrvHlprCommandLineStringToFwpMatchType(pString);

         if(pTempFilterConditions[tempConditionIndex].matchType == FWP_MATCH_TYPE_MAX)
            pTempFilterConditions[tempConditionIndex].matchType = FWP_MATCH_EQUAL;
         else
         {
           if((stringIndex + 1) < stringCount)
              pString = ppCLPStrings[++stringIndex];
           else
              HLPR_BAIL;
         }

         pTempFilterConditions[tempConditionIndex].fieldKey = FWPM_CONDITION_IPSEC_SECURITY_REALM_ID;

         status = StringCbLength(pString,
                                 STRSAFE_MAX_CCH * sizeof(WCHAR),
                                 &stringSize);
         if(FAILED(status))
            HLPR_BAIL;
         else
         {
            HLPR_NEW(pByteBlob,
                     FWP_BYTE_BLOB);
            HLPR_BAIL_ON_ALLOC_FAILURE(pByteBlob,
                                       status);

            HLPR_NEW_ARRAY(pByteBlob->data,
                           BYTE,
                           stringSize);
            HLPR_BAIL_ON_ALLOC_FAILURE(pByteBlob->data,
                                       status);

            RtlCopyMemory(pByteBlob->data,
                          pString,
                          stringSize);
   
            pByteBlob->size = (UINT32)stringSize;
   
            pTempFilterConditions[tempConditionIndex].conditionValue.type     = FWP_BYTE_BLOB_TYPE;
            pTempFilterConditions[tempConditionIndex].conditionValue.byteBlob = pByteBlob;
         }

         tempConditionIndex++;

         continue;
      }

#endif /// (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
#endif /// (NTDDI_VERSION >= NTDDI_WIN8)
#endif /// (NTDDI_VERSION >= NTDDI_WIN7)


      HLPR_BAIL_LABEL:

      if(status != NO_ERROR)
         HlprFwpmFilterConditionPurge(&(pTempFilterConditions[tempConditionIndex]));
   }

#pragma warning(pop)

   if(tempConditionIndex)
      status = PrvHlprFwpmFilterConditionsSort(pTempFilterConditions,
                                               tempConditionIndex,
                                               pFilter);

   HLPR_BAIL_LABEL_2:

   HLPR_DELETE_ARRAY(pTempFilterConditions);

   return status;
}

/**
 @helper_function="HlprCommandLineParseForFilterInfo"
 
   Purpose: Parse the command line parameters for strings which detail:                         <br>
               -l <FWPM_LAYER> - which layer to have the filter reside.                         <br>
               -b              - mark the filter and corresponding objects for boot time.       <br>
               -v              - mark the filter and corresponding objects as non-persistent.   <br>
               -c              - mark the filter for using a callout.                           <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/DD744934.aspx                              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprCommandLineParseForFilterInfo(_In_reads_(stringCount) PCWSTR* ppCLPStrings,
                                         _In_ UINT32 stringCount,
                                         _Inout_ FWPM_FILTER* pFilter,
                                         _In_ BOOLEAN forEnum)                         /* FALSE */
{
   ASSERT(ppCLPStrings);
   ASSERT(stringCount);
   ASSERT(pFilter);

   UINT32 status = NO_ERROR;

   pFilter->flags |= FWPM_FILTER_FLAG_PERSISTENT;

   /// -l <FWPM_LAYER_>
   status = HlprCommandLineParseForLayerKey(ppCLPStrings,
                                            stringCount,
                                            pFilter);
   HLPR_BAIL_ON_FAILURE(status);

   /// -s <FWPM_SUBLAYER_>
   status = HlprCommandLineParseForSubLayerKey(ppCLPStrings,
                                               stringCount,
                                               pFilter);
   HLPR_BAIL_ON_FAILURE(status);

   /// <FilterCondition> <value>
   /// -ipla 1.0.0.1
   status = HlprCommandLineParseForFilterConditions(ppCLPStrings,
                                                    stringCount,
                                                    pFilter,
                                                    forEnum);
   HLPR_BAIL_ON_FAILURE(status);

   /// -b
   HlprCommandLineParseForBootTime(ppCLPStrings,
                                   stringCount,
                                   pFilter);

   if(!(pFilter->flags & FWPM_FILTER_FLAG_BOOTTIME))
   {
      /// -v
      HlprCommandLineParseForVolatility(ppCLPStrings,
                                        stringCount,
                                        pFilter);
   }

   /// -c
   HlprCommandLineParseForCalloutUse(ppCLPStrings,
                                     stringCount,
                                     pFilter);

   HLPR_BAIL_LABEL:

   return status;
}
