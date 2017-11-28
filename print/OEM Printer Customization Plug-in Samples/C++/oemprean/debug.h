//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1996 - 2003  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:   Debug.H
//
//
//  PURPOSE:    Define common data types, and external function prototypes
//          for debugging functions. Also defines the various debug macros.
//
//
//  History:
//          06/28/03    xxx created.
//
//

#pragma once 

#include "devmode.h"

// VC and Build use different debug defines.
// The following makes it so either will
// cause the inclusion of debugging code.
#if !defined(_DEBUG) && defined(DBG)
    #define _DEBUG      DBG
#elif defined(_DEBUG) && !defined(DBG)
    #define DBG         _DEBUG
#endif



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
// VERBOSE(msg)
// Display a message if the current debug level is <= DBG_VERBOSE.
//
// TERSE(msg)
// Display a message if the current debug level is <= DBG_TERSE.
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
// ASSERTMSG(cond, msg)
// Verify a condition is true. If not, display a message and
// force a breakpoint.
//
// RIP(msg)
// Display a message and force a breakpoint.
//
// Usage:
// These macros require extra parantheses for the msg argument
// example, ASSERTMSG(x > 0, ("x is less than 0\n"));
//           WARNING(("App passed NULL pointer, ignoring...\n"));
//

#define DBG_VERBOSE     1
#define DBG_TERSE       2
#define DBG_WARNING 3
#define DBG_ERROR       4
#define DBG_RIP         5
#define DBG_NONE        6

//
// Determine what level of debugging messages to emit.
// This can be set in the sources file, or omitted 
// completely
//
#ifdef VERBOSE_MSG
    #define DEBUG_LEVEL     DBG_VERBOSE
#elif TERSE_MSG
    #define DEBUG_LEVEL     DBG_TERSE
#elif WARNING_MSG
    #define DEBUG_LEVEL     DBG_WARNING
#elif ERROR_MSG
    #define DEBUG_LEVEL     DBG_ERROR
#elif RIP_MSG
    #define DEBUG_LEVEL     DBG_RIP
#elif NO_DBG_MSG
    #define DEBUG_LEVEL     DBG_NONE
#else
    #define DEBUG_LEVEL     DBG_WARNING
#endif


#if DBG

    extern INT giDebugLevel;

    #define DBGMSG(level, prefix, msg) { \
        if (giDebugLevel <= (level)) { \
            OEMDebugMessage(L"%s %s (%d): ", prefix, StripDirPrefixA(__FILE__), __LINE__); \
            OEMDebugMessage(msg); \
        } \
    }

    #define VERBOSE     if(giDebugLevel <= DBG_VERBOSE) OEMDebugMessage
    #define TERSE       if(giDebugLevel <= DBG_TERSE) OEMDebugMessage
    #define WARNING     if(giDebugLevel <= DBG_WARNING) OEMDebugMessage
    #define ERR         if(giDebugLevel <= DBG_ERROR) OEMDebugMessage

    #define DBG_OEMDMPARAM(iDbgLvl, szLabel, pobj)              {vDumpOemDMParam(iDbgLvl, szLabel, pobj);}
    #define DBG_SURFOBJ(iDbgLvl, szLabel, pobj)                 {vDumpSURFOBJ(iDbgLvl, szLabel, pobj);}
    #define DBG_STROBJ(iDbgLvl, szLabel, pobj)                  {vDumpSTROBJ(iDbgLvl, szLabel, pobj);}
    #define DBG_FONTOBJ(iDbgLvl, szLabel, pobj)                 {vDumpFONTOBJ(iDbgLvl, szLabel, pobj);}
    #define DBG_CLIPOBJ(iDbgLvl, szLabel, pobj)                 {vDumpCLIPOBJ(iDbgLvl, szLabel, pobj);}
    #define DBG_BRUSHOBJ(iDbgLvl, szLabel, pobj)                {vDumpBRUSHOBJ(iDbgLvl, szLabel, pobj);}
    #define DBG_GDIINFO(iDbgLvl, szLabel, pobj)                 {vDumpGDIINFO(iDbgLvl, szLabel, pobj);}
    #define DBG_DEVINFO(iDbgLvl, szLabel, pobj)                 {vDumpDEVINFO(iDbgLvl, szLabel, pobj);}
    #define DBG_BMPINFO(iDbgLvl, szLabel, pobj)                 {vDumpBitmapInfoHeader(iDbgLvl, szLabel, pobj);}
    #define DBG_POINTL(iDbgLvl, szLabel, pobj)                  {vDumpPOINTL(iDbgLvl, szLabel, pobj);}
    #define DBG_RECTL(iDbgLvl, szLabel, pobj)                   {vDumpRECTL(iDbgLvl, szLabel, pobj);}
    #define DBG_XLATEOBJ(iDbgLvl, szLabel, pobj)                {vDumpXLATEOBJ(iDbgLvl, szLabel, pobj);}
    #define DBG_COLORADJUSTMENT(iDbgLvl, szLabel, pobj)         {vDumpCOLORADJUSTMENT(iDbgLvl, szLabel, pobj);}

    #define ASSERT(cond) \
    { \
        if (! (cond)) \
        { \
            RIP((L"\n")); \
        } \
    }

    #define ASSERTMSG(cond, msg) \
    { \
        if (! (cond)) { \
            RIP(msg); \
        } \
    }

    #define RIP(msg) \
    { \
        DBGMSG(DBG_RIP, L"RIP", msg); \
        DebugBreak(); \
    }

    typedef struct DBG_FLAGS {
        LPWSTR pszFlag;
        DWORD dwFlag;
    } *PDBG_FLAGS;

    BOOL OEMDebugMessage(LPCWSTR, ...);
    void vDumpOemDMParam(INT iDebugLevel, _In_ PWSTR pszInLabel, POEMDMPARAM pOemDMParam);
    void vDumpSURFOBJ(INT iDebugLevel, _In_ PWSTR pszInLabel, SURFOBJ *pso);
    void vDumpSTROBJ(INT iDebugLevel, _In_ PWSTR pszInLabel, STROBJ *pstro);
    void vDumpFONTOBJ(INT iDebugLevel, _In_ PWSTR pszInLabel, FONTOBJ *pfo);
    void vDumpCLIPOBJ(INT iDebugLevel, _In_ PWSTR pszInLabel, CLIPOBJ *pco);
    void vDumpBRUSHOBJ(INT iDebugLevel, _In_ PWSTR pszInLabel, BRUSHOBJ *pbo);
    void vDumpGDIINFO(INT iDebugLevel, _In_ PWSTR pszInLabel, GDIINFO *pGdiInfo);
    void vDumpDEVINFO(INT iDebugLevel, _In_ PWSTR pszInLabel, DEVINFO *pDevInfo);
    void vDumpBitmapInfoHeader(INT iDebugLevel, _In_ PWSTR pszInLabel, PBITMAPINFOHEADER pBitmapInfoHeader);
    void vDumpPOINTL(INT iDebugLevel, _In_ PWSTR pszInLabel, POINTL *pptl);
    void vDumpRECTL(INT iDebugLevel, _In_ PWSTR pszInLabel, RECTL *prcl);
    void vDumpXLATEOBJ(INT iDebugLevel, _In_ PWSTR pszInLabel, XLATEOBJ *pxlo);
    void vDumpCOLORADJUSTMENT(INT iDebugLevel, _In_ PWSTR pszInLabel, COLORADJUSTMENT *pca);

    BOOL OEMRealDebugMessage(DWORD dwSize, LPCWSTR lpszMessage, va_list arglist);
    PCSTR StripDirPrefixA(IN PCSTR    pstrFilename);
    void vDumpFlags(DWORD dwFlags, PDBG_FLAGS pDebugFlags);

#else // !DBG

    #define DebugMsg    NOP_FUNCTION

    #define VERBOSE     NOP_FUNCTION
    #define TERSE       NOP_FUNCTION
    #define WARNING     NOP_FUNCTION
    #define ERR         NOP_FUNCTION

    #define DBG_OEMDMPARAM(iDbgLvl, szLabel, pobj)          NOP_FUNCTION
    #define DBG_OEMDEVMODE(iDbgLvl, szLabel, pobj)          NOP_FUNCTION
    #define DBG_SURFOBJ(iDbgLvl, szLabel, pobj)             NOP_FUNCTION
    #define DBG_STROBJ(iDbgLvl, szLabel, pobj)              NOP_FUNCTION
    #define DBG_FONTOBJ(iDbgLvl, szLabel, pobj)             NOP_FUNCTION
    #define DBG_CLIPOBJ(iDbgLvl, szLabel, pobj)             NOP_FUNCTION
    #define DBG_BRUSHOBJ(iDbgLvl, szLabel, pobj)                NOP_FUNCTION
    #define DBG_GDIINFO(iDbgLvl, szLabel, pobj)             NOP_FUNCTION
    #define DBG_DEVINFO(iDbgLvl, szLabel, pobj)             NOP_FUNCTION
    #define DBG_BMPINFO(iDbgLvl, szLabel, pobj)             NOP_FUNCTION
    #define DBG_POINTL(iDbgLvl, szLabel, pobj)              NOP_FUNCTION
    #define DBG_RECTL(iDbgLvl, szLabel, pobj)                   NOP_FUNCTION
    #define DBG_XLATEOBJ(iDbgLvl, szLabel, pobj)                NOP_FUNCTION
    #define DBG_COLORADJUSTMENT(iDbgLvl, szLabel, pobj)     NOP_FUNCTION

    #define ASSERT(cond)

    #define ASSERTMSG(cond, msg)
    #define RIP(msg)

#endif





