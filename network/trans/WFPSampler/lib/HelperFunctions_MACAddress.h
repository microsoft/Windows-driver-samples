////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_MACAddress.h
//
//   Abstract:
//      This module contains functions which assist in actions pertaining to MAC Addresses.
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

#ifndef HELPERFUNCTIONS_MAC_ADDRESS_H
#define HELPERFUNCTIONS_MAC_ADDRESS_H

BOOLEAN HlprEthernetMACAddressStringIsValidFormat(_In_ PCWSTR pEthernetMACAddress);

_Success_(return == NO_ERROR)
UINT32 HlprEthernetMACAddressStringToValue(_In_ PCWSTR pEthernetMACAddressString,
                                           _Inout_updates_all_(IEEE_802_ADDRESS_LENGTH) BYTE* pEthernetMACAddress);

#endif /// HELPERFUNCTIONS_MAC_ADDRESS_H