//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1998 - 2003  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:	Globals.h
//
//  PURPOSE:   Lists of globals declared in Globals.cpp.
//
#pragma once 

///////////////////////////////////////
//          Globals
///////////////////////////////////////


#define OEM_SIGNATURE   'MSFT'
#define OEM_VERSION     0x00000001L

// OEM Signature and version.
#define PROP_TITLE      L"OEM PS UI Replacement Page"
#define DLLTEXT(s)      TEXT("PSUIREP:  ") TEXT(s)

// OEM UI Misc defines.
#define ERRORTEXT(s)    TEXT("ERROR ") DLLTEXT(s)

// Module's Instance handle from DLLEntry of process.
extern HINSTANCE   ghInstance;

