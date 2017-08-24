//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1998 - 2005  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:    globals.cxx
//

#include "precomp.hxx"

// This indicates to Prefast that this is a usermode driver file.
_Analysis_mode_(_Analysis_code_type_user_driver_);

///////////////////////////////////////
//          Globals
///////////////////////////////////////

// Module's Instance handle from DLLEntry of process.
HINSTANCE   ghInstance = NULL;
