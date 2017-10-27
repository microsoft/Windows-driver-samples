//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1998 - 2005  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:	globals.hxx
//

#pragma once

//
// Maximum string length limit for STRSAFE string length function StringCchLength
// Must be less than or equal to maximum upper bound on string length defined by STRSAFE
// library ie STRSAFE_MAX_CCH
//
#define STR_MAX_CCH 10000

///////////////////////////////////////
//          Globals
///////////////////////////////////////

// Module's Instance handle from DLLEntry of process.
extern HINSTANCE   ghInstance;
