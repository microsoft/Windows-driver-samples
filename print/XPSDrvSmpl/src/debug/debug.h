/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   xdsdbg.h

Abstract:

   Debug definitions.

--*/

#pragma once

//
// These macros are used for debugging purposes. They expand
// to white spaces on a free build. Here is a brief description
// of what they do and how they are used:
//
// giDebugLevel
//  Global variable which set the current debug level to control
//  the amount of debug messages emitted.
//
// VERBOSE(msg)
//  Display a message if the current debug level is <= DBG_VERBOSE.
//
// TERSE(msg)
//  Display a message if the current debug level is <= DBG_TERSE.
//
// WARNING(msg)
//  Display a message if the current debug level is <= DBG_WARNING.
//  The message format is: WRN filename (linenumber): message
//
// ERR(msg)
//  Similiar to WARNING macro above - displays a message
//  if the current debug level is <= DBG_ERROR.
//
// ASSERT(cond)
//  Verify a condition is true. If not, force a breakpoint.
//
// ASSERTMSG(cond, msg)
//  Verify a condition is true. If not, display a message and
//  force a breakpoint.
//
// RIP(msg)
//  Display a message and force a breakpoint.
//
// Usage:
//  These macros require extra parantheses for the msg argument
//  example, ASSERTMSG(x > 0, ("x is less than 0\n"));
//           WARNING(("App passed NULL pointer, ignoring...\n"));
//

#pragma once

#define DBG_VERBOSE 1
#define DBG_TERSE   2
#define DBG_WARNING 3
#define DBG_ERROR   4
#define DBG_RIP     5

BOOL
RealDebugMessage(
    _In_ DWORD   dwSize,
    _In_ PCSTR   pszMessage,
         va_list arglist
    );

BOOL
DbgPrint(
    _In_ PCSTR pszFormatString,
    ...
    );

VOID
DbgDOMDoc(
    _In_ PCSTR             pszMessage,
    _In_ IXMLDOMDocument2* pDomDoc
    );

#if DBG

#ifndef MAX_DEBUG_LEVEL
#define MAX_DEBUG_LEVEL DBG_VERBOSE
#endif

#define DBGMSG(level, prefix, msg) { \
            INT dbgLevel = level; \
            if (MAX_DEBUG_LEVEL <= (dbgLevel)) { \
                DbgPrint("%s %s (%d): ", prefix, __FILE__, __LINE__); \
                DbgPrint(msg); \
            } \
        }

#define DBGMSG_ON_HR(level, prefix, hr) { \
            INT dbgLevel = level; \
            HRESULT hres = hr; \
            if (MAX_DEBUG_LEVEL <= (dbgLevel) && FAILED(hres)) { \
                DbgPrint("%s %s (%d): Call failed with HRESULT = 0x%x\n", prefix, __FILE__, __LINE__, hr); \
            } \
        }

#define DBGMSG_ON_HR_EXC(level, prefix, hr, hr_exc) { \
            INT dbgLevel = level; \
            HRESULT hres = hr; \
            if (MAX_DEBUG_LEVEL <= (dbgLevel) && FAILED(hres) && hres != hr_exc) { \
            DbgPrint("%s %s (%d): Call failed with HRESULT = 0x%x\n", prefix, __FILE__, __LINE__, hr); \
            } \
        }

#define DBGPRINT(level, msg) { \
            INT dbgLevel = level; \
            if (MAX_DEBUG_LEVEL <= (dbgLevel)) { \
                DbgPrint(msg); \
            } \
        }

#define DBGXML(msg, pDomDoc) { \
            INT dbgLevel = DBG_VERBOSE; \
            if (MAX_DEBUG_LEVEL <= dbgLevel) { \
                DbgDOMDoc(msg, pDomDoc); \
            } \
        }

#define VERBOSE(msg) DBGPRINT(DBG_VERBOSE, msg)
#define TERSE(msg) DBGPRINT(DBG_TERSE, msg)
#define WARNING(msg) DBGMSG(DBG_WARNING, "WRN", msg)
#define ERR(msg) DBGMSG(DBG_ERROR, "ERR", msg)
#define ERR_ON_HR(hr) DBGMSG_ON_HR(DBG_ERROR, "ERR", hr)
#define ERR_ON_HR_EXC(hr, hr_exc) DBGMSG_ON_HR_EXC(DBG_ERROR, "ERR", hr, hr_exc)

#define ASSERT(cond) { \
            if (! (cond)) { \
                RIP(("\n")); \
            } \
        }

#define ASSERTMSG(cond, msg) { \
            if (!(cond)) { \
                RIP(msg); \
            } \
        }

#define RIP(msg) { \
            DBGMSG(DBG_RIP, "RIP", msg); \
            DebugBreak(); \
        }

#define DBG_ONLY(p) p

#else // !DBG

#define VERBOSE(msg)
#define TERSE(msg)
#define WARNING(msg)
#define ERR(msg)
#define ERR_ON_HR(hr)
#define ERR_ON_HR_EXC(hr, hr_exc)

#define ASSERT(cond)

#define ASSERTMSG(cond, msg)

#define RIP(msg)

#define DBG_ONLY(p)

#define DBGMSG(level, prefix, msg)

#define DBGMSG_ON_HR(level, prefix, hr)

#define DBGPRINT(level, msg)

#define DBGXML(msg, pDomDoc)

#endif

