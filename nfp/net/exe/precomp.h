////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// 
// Module Name: precomp.h
// Abstract:
// 
//    Precompiled header file for the NetNfpControl console app
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

// Windows Headers
#include <windows.h>
#include <Winsock2.h>
#include <Mswsock.h>
#include <Ws2bth.h>
#include <Ws2tcpip.h>
#include <conio.h>
#include <setupapi.h>
#include <tchar.h>
#include <strsafe.h>
#include <winioctl.h>

// ATL stuff
#include <atlbase.h>

// Common NetNfp header
#include "NetNfp.h"
