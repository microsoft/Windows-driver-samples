//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1998 - 2003  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:	fsutils.h
//    
//
//  PURPOSE:   Fusion utilities
//
#pragma once 

HANDLE GetMyActivationContext();
BOOL CreateMyActivationContext();
HANDLE CreateActivationContextFromResource(HMODULE hModule, LPCTSTR pszResourceName);

