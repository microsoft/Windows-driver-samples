//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1997 - 2005  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:	precomp.hxx
//
//  PURPOSE:	Header files that should be in the precompiled header.
//

#pragma once

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
#include <windows.h>
#include <WINSPOOL.H>
#include <TCHAR.H>
#include <INITGUID.H>
#include <msxml6.h>
#include <string.h>
#include <EXCPT.H>
#include <ASSERT.H>
#include <PRSHT.H>
#include <COMPSTUI.H>
#include <WINDDI.H>
#include <WINDDIUI.H>

#include <PRINTOEM.H>
#include <prdrvcom.h>
#include <prcomoem.h>

// StrSafe.h needs to be included last
// to disallow unsafe string functions.
#include <intsafe.h>
#include <strsafe.h>

#include "common.ver"

