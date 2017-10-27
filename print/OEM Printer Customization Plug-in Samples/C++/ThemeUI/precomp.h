//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1997 - 2003  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:	precomp.H
//    
//
//  PURPOSE:	Common headers that don't change often
//

#pragma once

// Necessary for compiling under VC.
#if(!defined(WINVER) || (WINVER < 0x0500))
	#undef WINVER
	#define WINVER          0x0500
#endif
#if(!defined(_WIN32_WINNT) || (_WIN32_WINNT < 0x0500))
	#undef _WIN32_WINNT
	#define _WIN32_WINNT    0x0500
#endif

// Isolation define for using ComCtrl v6.
#ifndef ISOLATION_AWARE_ENABLED
    #define ISOLATION_AWARE_ENABLED
#endif


// Required header files that shouldn't change often.

#include <STDDEF.H>
#include <STDLIB.H>
#include <OBJBASE.H>
#include <STDARG.H>
#include <STDIO.H>
#include <WINDEF.H>
#include <WINERROR.H>
#include <WINBASE.H>
#include <WINGDI.H>
#include <WINDDI.H>
#include <WINSPOOL.H>
#include <TCHAR.H>
#include <EXCPT.H>
#include <ASSERT.H>
#include <PRSHT.H>
#include "COMPSTUI.H"
#include <WINDDIUI.H>
#include <PRINTOEM.H>
#include <COMMCTRL.H>
#include <INITGUID.H>
#include <PRCOMOEM.H>

// Safe character string functions to detect buffer overrun conditions:
#include <strsafe.h>

// Safe integer functions to detect integer overflow/underflow conditions:
#include <intsafe.h>

