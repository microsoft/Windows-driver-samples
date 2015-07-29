////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_MACAddress.cpp
//
//   Abstract:
//      This module contains functions which assist in actions pertaining to MAC Addresses.
//
//   Naming Convention:
//
//      <Scope><Module><Object><Action><Modifier>
//  
//      i.e.
//
//       <Scope>
//          {
//                                     - Function is likely visible to other modules
//          }
//       <Module>
//          {
//            Hlpr                     - Function is from HelperFunctions_* Modules.
//          }
//       <Object>
//          {
//            EthernetMACAddressString - Function pertains to null terminated wide character string
//                                          representaions of Ethernet MAC Addresses
//          }
//       <Action>
//          {
//            To                       -  Function converts from one type to another
//            Is                       -  Function compares values.
//          }
//       <Modifier>
//          {
//            ValidFormat              - Function validates condition.
//            Value                    - Function acts on a value.
//          }
//
//   Private Functions:
//
//   Public Functions:
//      HlprEthernetMACAddressStringIsValidFormat(),
//      HlprEthernetMACAddressStringToValue(),
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Add offset to check in 
//                                              HlprEthernetMACAddressStringToValue
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "HelperFunctions_Include.h"

/**
 @helper_function="HlprEthernetMACAddressStringIsValidFormat"
 
   Purpose: Determine if a string may be an Ethernet MAC address by verifying the string:       <br>
               is at least 17 characters                                                        <br>
               has at least 5 colons(':') or hyphens('-')                                       <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
BOOLEAN HlprEthernetMACAddressStringIsValidFormat(_In_ PCWSTR pEthernetMACAddressString)
{
   BOOLEAN isEthernetMACAddress = FALSE;

   if(pEthernetMACAddressString)
   {
      UINT32 status       = NO_ERROR;
      size_t stringLength = 0;

      status = StringCchLength(pEthernetMACAddressString,
                               STRSAFE_MAX_CCH,
                               &stringLength);
      if(status != ERROR_SUCCESS)
      {
         HlprLogError(L"HlprEthernetMACAddressStringIsValidFormat : StringCchLength() [status: %#x]",
                      status);

         HLPR_BAIL;
      }
      else if(stringLength <= IEEE_802_ADDRESS_STRING_BUFFER)
      {
         if(wcschr(pEthernetMACAddressString,
                   L':'))
         {
            UINT32 numColons = 0;

            for(UINT32 index = 0;
                index < stringLength;
                index++)
            {
               if(pEthernetMACAddressString[index] == L':')
                 numColons++;
            }

            if(numColons == 5)
               isEthernetMACAddress = TRUE;
         }
         else
         {
            if(wcschr(pEthernetMACAddressString,
                      L'-'))
            {
               UINT32 numHyphens = 0;

               for(UINT32 index = 0;
                   index < stringLength;
                   index++)
               {
                  if(pEthernetMACAddressString[index] == L'-')
                     numHyphens++;
               }

               if(numHyphens == 5)
                  isEthernetMACAddress = TRUE;
            }
         }
      }
   }

   HLPR_BAIL_LABEL:

   if(!isEthernetMACAddress)
      HlprLogError(L"HlprEthernetMACAddressStringIsValidFormat() [status: %#x][pEthernetMACAddressString: %s]",
                   ERROR_INVALID_DATA,
                   pEthernetMACAddressString);

   return isEthernetMACAddress;
}

/**
 @helper_function="HlprEthernetMACAddressStringToValue"
 
   Purpose:  Convert a string representing an Ethernet MAC Address to it's 6 BYTE value.        <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprEthernetMACAddressStringToValue(_In_ PCWSTR pEthernetMACAddressString,
                                           _Inout_updates_all_(IEEE_802_ADDRESS_LENGTH) BYTE* pEthernetMACAddress)
{
   UINT32 status = NO_ERROR;

   if(pEthernetMACAddressString &&
      HlprEthernetMACAddressStringIsValidFormat(pEthernetMACAddressString))
   {
      UINT32 offset       = 0;
      UINT32 bytePosition = 0x10;

      ZeroMemory(pEthernetMACAddress,
                 IEEE_802_ADDRESS_LENGTH);

#pragma warning(push)
#pragma warning(disable: 26018) /// length validation occurs in HlprEthernetMACAddressStringIsValidFormat

      for(UINT32 i = 0;
          i < IEEE_802_ADDRESS_STRING_BUFFER && /// does not include '\0'
          offset < IEEE_802_ADDRESS_LENGTH;
          i++)
      {
         WCHAR character = pEthernetMACAddressString[i];

         /// Validate every 3 character is either a ':' or a '-'
         if((i + 1) % 3 == 0)
         {
            if(character != L':' &&
               character != L'-')
            {
               status = ERROR_INVALID_DATA;

                HlprLogError(L"HlprEthernetMACAddressStringToValue() [status: %#x][pEthernetMACAddressString: %s]",
                             status,
                             pEthernetMACAddressString);

               HLPR_BAIL;
            }

            continue;
         }

         if(character >= L'0' &&
            character <= L'9')
            pEthernetMACAddress[offset] = (BYTE)(pEthernetMACAddress[offset] + ((character - L'0') * bytePosition));
         else if(character >= L'a' &&
                 character <= L'f')
            pEthernetMACAddress[offset] = (BYTE)(pEthernetMACAddress[offset] + ((character + 10 - L'a') * bytePosition));
         else if(character >= L'A' &&
                 character <= L'F')
            pEthernetMACAddress[offset] = (BYTE)(pEthernetMACAddress[offset] + ((character + 10 - L'A') * bytePosition));
         else
         {
            status = ERROR_INVALID_DATA;

            HlprLogError(L"HlprEthernetMACAddressStringToValue() [status: %#x][pEthernetMACAddressString: %s]",
                         status,
                         pEthernetMACAddressString);

            HLPR_BAIL;
         }

         if(bytePosition == 1)
         {
            offset++;

            bytePosition = 0x10;
         }
         else
            bytePosition = 1;
      }

#pragma warning(pop)

      HLPR_BAIL_LABEL:

      if(status != ERROR_SUCCESS)
         ZeroMemory(pEthernetMACAddress,
                    IEEE_802_ADDRESS_LENGTH);
   }
   else
      HlprLogError(L"HlprEthernetMACAddressStringToValue() [status: %#x][pEthernetMACAddressString: %s]",
                   status,
                   pEthernetMACAddressString);

   return status;
}
