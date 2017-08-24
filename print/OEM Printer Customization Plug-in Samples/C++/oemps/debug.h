//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1996 - 2003  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:   Debug.H
//
//  PURPOSE:    Define common data types, and external function prototypes
//          for debugging functions. Also defines the various debug macros.
//
//

#pragma once

/////////////////////////////////////////////////////////
//      Macros
/////////////////////////////////////////////////////////
//
// These macros are used for debugging purposes. They expand
// to white spaces on a free build. Here is a brief description
// of what they do and how they are used:
//
// giDebugLevel
// Global variable which set the current debug level to control
// the amount of debug messages emitted.
//
// WARNING(msg)
// Display a message if the current debug level is <= DBG_WARNING.
// The message format is: WRN filename (linenumber): message
//
// ERR(msg)
// Similiar to WARNING macro above - displays a message
// if the current debug level is <= DBG_ERROR.
//
// ASSERT(cond)
// Verify a condition is true. If not, force a breakpoint.
//
// RIP(msg)
// Display a message and force a breakpoint.
//
// Usage:
//      WARNING("App passed NULL pointer, ignoring...\n");
//

#define DBG_VERBOSE     0
#define DBG_WARNING     1
#define DBG_ERROR       2
#define DBG_NONE        3

// VC and Build use different debug defines.
#if defined (DBG) || defined (_DEBUG)

    // __declspec(selectany) ensures that even if this is defined in multiple compilation 
    // modules, they will all resolve to the same symbol in the linked DLL
    __declspec(selectany) INT giDebugLevel = 2;

    #define STRINGIZE(x) #x
    #define QUOTE(x) STRINGIZE(x)
    #define FILE_AND_LINE __FILE__ "@" QUOTE(__LINE__)
        
    #define VERBOSE(msg)     { if(giDebugLevel <= DBG_VERBOSE) OutputDebugStringA( FILE_AND_LINE ": " msg); }
    #define WARNING(msg)     { if(giDebugLevel <= DBG_WARNING) OutputDebugStringA( FILE_AND_LINE ": Warning: " msg); }
    #define ERR(msg)         { if(giDebugLevel <= DBG_ERROR) OutputDebugStringA(FILE_AND_LINE ": Error:" msg); }
    #define ASSERT(cond)     { if (!(cond)) RIP("\n"); }
    #define RIP(msg)         { OutputDebugStringA(FILE_AND_LINE ": Error (not recoverable): " msg); DebugBreak(); }

#else // !DBG

    #define VERBOSE(msg)
    #define WARNING(msg) 
    #define ERR(msg) 
    #define ASSERT(cond)
    #define RIP(msg)

#endif





