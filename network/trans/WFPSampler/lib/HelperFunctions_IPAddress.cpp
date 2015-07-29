////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_IPAddress.cpp
//
//   Abstract:
//      This module contains functions which assist in actions pertaining to IP Addresses.
//
//   Naming Convention:
//
//      <Scope><Module><Object><Action><Modifier>
//  
//      i.e.
//
//       <Scope>
//          {
//                              - Function is likely visible to other modules
//          }
//       <Module>
//          {
//            Hlpr              - Function is from HelperFunctions_* Modules.
//          }
//       <Object>
//          {
//            IPAddressV4String - Function pertains to null terminated wide character string 
//                                   representaions of IPv4 Addresses
//            IPAddressV6String - Function pertains to null terminated wide character string 
//                                   representaions of IPv6 Addresses
//          }
//       <Action>
//          {
//            To                -  Function converts from one type to another
//            Is                -  Function compares values.
//          }
//       <Modifier>
//          {
//            ValidFormat       - Function validates condition.
//            Value             - Function acts on a value.
//          }
//
//   Private Functions:
//
//   Public Functions:
//      HlprIPAddressV4StringIsValidFormat(),
//      HlprIPAddressV4StringToValue(),
//      HlprIPAddressV6StringIsValidFormat(),
//      HlprIPAddressV6StringToValue(),
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "HelperFunctions_Include.h" /// .

/**
 @helper_function="HlprIPAddressV4StringIsValidFormat"
 
   Purpose: Determine if a string may be an IPv4 address by verifying the string:               <br>
               is at least 7 characters                                                         <br>
               has at least 3 decimals('.')                                                     <br>
               is not more than 16 characters                                                   <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
BOOLEAN HlprIPAddressV4StringIsValidFormat(_In_ PCWSTR pIPAddress)
{
   ASSERT(pIPAddress);

   UINT32  status        = NO_ERROR;
   BOOLEAN isIPv4Address = FALSE;
   size_t  addressSize   = 0;

   status = StringCchLength(pIPAddress,
                            STRSAFE_MAX_CCH,
                            &addressSize);
   if(FAILED(status))
   {
      HlprLogError(L"HlprIPAddressV4StringIsValidFormat : StringCchLength() [status: %#x]",
                   status);

      HLPR_BAIL;
   }

   if(addressSize > 6 &&   /// minimum address size: 0.0.0.0
      addressSize < 16)    /// maximum address size: 255.255.255.255
   {
      UINT32 numPeriods = 0;

      for(UINT32 index = 0;
          index < addressSize;
          index++)
      {
         if(pIPAddress[index] == '.')
            numPeriods++;
      }

      if(numPeriods == 3)
         isIPv4Address = TRUE;
   }

   HLPR_BAIL_LABEL:

   return isIPv4Address;
}

/**
 @helper_function="HlprIPAddressV4StringToValue"
 
   Purpose:  Convert a string representing an IPv4 Address to it's 4 BYTE value.                <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA814459.aspx              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprIPAddressV4StringToValue(_In_ PCWSTR pIPv4AddressString,
                                    _Inout_ UINT32* pIPv4Address)
{
   ASSERT(pIPv4AddressString);
   ASSERT(pIPv4Address);

   UINT32 status = NO_ERROR;

#pragma warning(push)
#pragma warning(disable: 24002) /// function is IPv4 specific, however there is a v6 counter-part
#pragma warning(disable: 24007) /// function is IPv4 specific, however there is a v6 counter-part

   if(HlprIPAddressV4StringIsValidFormat(pIPv4AddressString))
   {
      UINT16  port   = 0;
      IN_ADDR v4Addr = {0};

      status = RtlIpv4StringToAddressEx(pIPv4AddressString,
                                        FALSE,
                                        &v4Addr,
                                        &port);
      if(status != NO_ERROR)
      {
         HlprLogError(L"HlprIPAddressV4StringToValue : RtlIpv4StringToAddressEx() [status: %#x][pIPv4AddressString: %s]",
                      status,
                      pIPv4AddressString);

         HLPR_BAIL;
      }

      CopyMemory(pIPv4Address,
                 &v4Addr,
                 IPV4_ADDRESS_LENGTH);

      *pIPv4Address = ntohl(*pIPv4Address);
   }
   else
   {
      status = ERROR_INVALID_PARAMETER;

      HlprLogError(L"HlprIPAddressV4StringToValue() [status: %#x][pIPv4AddressString: %#s]",
                   status,
                   pIPv4AddressString);
   }

#pragma warning(pop)

   HLPR_BAIL_LABEL:

   return status;
}

/**
 @helper_function="HlprIPAddressV6StringIsValidFormat"
 
   Purpose: Determine if a string may be an IPv6 address by verifying the string:               <br>
               is at least 3 characters                                                         <br>
               has at least 2 colons (':')                                                      <br>
               is not more than 40 characters                                                   <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
BOOLEAN HlprIPAddressV6StringIsValidFormat(_In_ PCWSTR pIPAddress)
{
   ASSERT(pIPAddress);

   UINT32  status        = NO_ERROR;
   BOOLEAN isIPv6Address = FALSE;
   size_t  addressSize   = 0;

   status = StringCchLength(pIPAddress,
                            STRSAFE_MAX_CCH,
                            &addressSize);
   if(FAILED(status))
   {
      HlprLogError(L"HlprIPAddressV6StringIsValidFormat : StringCchLength() [status: %#x]",
                   status);

      HLPR_BAIL;
   }

   if(addressSize > 2 &&   /// minimum address size: ::1
      addressSize < 40)    /// maximum address size: FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF
   {
      UINT32 numColons = 0;

      for(UINT32 index = 0;
          index < addressSize;
          index++)
      {
         if(pIPAddress[index] == ':')
            numColons++;
      }

      if(numColons > 1 &&
         numColons < 8)
         isIPv6Address = TRUE;
   }

   HLPR_BAIL_LABEL:

   return isIPv6Address;
}

/**
 @helper_function="HlprIPAddressV4StringToValue"
 
   Purpose:  Convert a string representing an IPv6 Address to it's 16 BYTE value.               <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA814463.aspx              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprIPAddressV6StringToValue(_In_ PCWSTR pIPv6AddressString,
                                    _Inout_updates_all_(IPV6_ADDRESS_LENGTH) BYTE* pIPv6Address)
{
   ASSERT(pIPv6AddressString);
   ASSERT(pIPv6Address);

   UINT32 status = NO_ERROR;

   if(HlprIPAddressV6StringIsValidFormat(pIPv6AddressString))
   {
      UINT32   scopeId = 0;
      UINT16   port    = 0;
      IN6_ADDR v6Addr  = {0};

      status = RtlIpv6StringToAddressEx(pIPv6AddressString,
                                        &v6Addr,
                                        (PULONG)&scopeId,
                                        &port);
      if(status != NO_ERROR)
      {
         HlprLogError(L"HlprIPAddressV6StringToValue : RtlIpv6StringToAddressEx() [status: %#x][pIPv6AddressString: %s]",
                      status,
                      pIPv6AddressString);

         HLPR_BAIL;
      }

      CopyMemory(pIPv6Address,
                 &v6Addr,
                 IPV6_ADDRESS_LENGTH);
   }
   else
   {
      status = ERROR_INVALID_PARAMETER;

      HlprLogError(L"HlprIPAddressV6StringToValue() [status: %#x][pIPv6AddressString: %#p][pIPv6Address: %#p]",
                   status,
                   pIPv6AddressString,
                   pIPv6Address);
   }

   HLPR_BAIL_LABEL:

   return status;
}
