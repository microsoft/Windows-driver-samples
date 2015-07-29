////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_IPAddress.h
//
//   Abstract:
//      This module contains prototypes for functions which assist in actions pertaining to IP 
//      Addresses.
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

#ifndef HELPERFUNCTIONS_IP_ADDRESS_H
#define HELPERFUNCTIONS_IP_ADDRESS_H

#define IPV4 4
#define IPV6 6

#define IPV4_ADDRESS_LENGTH  4
#define IPV6_ADDRESS_LENGTH 16

BOOLEAN HlprIPAddressV4StringIsValidFormat(_In_ PCWSTR pIPAddress);

_Success_(return == NO_ERROR)
UINT32 HlprIPAddressV4StringToValue(_In_ PCWSTR pIPv4AddressString,
                                    _Inout_ UINT32* pIPv4Address);

BOOLEAN HlprIPAddressV6StringIsValidFormat(_In_ PCWSTR pIPAddress);

_Success_(return == NO_ERROR)
UINT32 HlprIPAddressV6StringToValue(_In_ PCWSTR pIPv6AddressString,
                                    _Inout_updates_all_(IPV6_ADDRESS_LENGTH) BYTE* pIPv6Address);

#endif /// HELPERFUNCTIONS_IP_ADDRESS_H