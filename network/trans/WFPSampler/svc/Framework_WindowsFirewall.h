////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      Framework_WindowsFirewall.h
//
//   Abstract:
//      This module contains prototypes for functions for interoperating with the Microsoft Windows
//         Firewall
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

#ifndef FRAMEWORK_WINDOWS_FIREWALL_H
#define FRAMEWORK_WINDOWS_FIREWALL_H

VOID WindowsFirewallReleaseFirewallCategory();

_Success_(return == NO_ERROR)
UINT32 WindowsFirewallAcquireFirewallCategory();

#endif /// FRAMEWORK_WINDOWS_FIREWALL_H