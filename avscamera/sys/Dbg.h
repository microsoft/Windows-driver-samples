/**************************************************************************

    AVStream Camera Sample

    Copyright (c) 2014, Microsoft Corporation.

    File:

        Dbg.h

    Abstract:

        Debug definitions.

    History:

        created 2/18/2015

**************************************************************************/

#pragma once

#define DEBUGLVL_VERBOSE 2
#define DEBUGLVL_TERSE 1
#define DEBUGLVL_ERROR 0

const int DebugLevel = DEBUGLVL_TERSE;

#if (DBG)
#define _DbgPrintF(lvl, strings) \
{ \
    if (lvl <= DebugLevel) {\
        DbgPrint(STR_MODULENAME);\
        DbgPrint##strings;\
        DbgPrint("\n");\
        if ((lvl) == DEBUGLVL_ERROR) {\
            NT_ASSERT(0);\
        } \
    }\
}
#else // !DBG
#define _DbgPrintF(lvl, strings)
#endif // !DBG

//  This is for debug output; but we're going to leave it in the
//  driver for now.  We can always turn off the Windbg output at
//  the debugger.
//  Enable via debugger: ed Kd_IHVSTREAMING_Mask 0xFFFFFFFF
#define ENABLE_TRACING  1
#if ENABLE_TRACING

#define DBG_OUT( FMT, ... ) \
    { \
        LARGE_INTEGER   SysTime; \
        LARGE_INTEGER   LocalTime; \
        TIME_FIELDS     TimeFields; \
        \
        KeQuerySystemTimePrecise( &SysTime ); \
        ExSystemTimeToLocalTime( &SysTime, &LocalTime ); \
        RtlTimeToTimeFields( &LocalTime, &TimeFields ); \
        DbgPrintEx( DPFLTR_IHVSTREAMING_ID, DPFLTR_MASK|DPFLTR_TRACE_LEVEL, \
        "%02d:%02d:%02d.%03d [%p] " ## FMT ## "\n", \
        TimeFields.Hour, TimeFields.Minute, TimeFields.Second, TimeFields.Milliseconds, \
        KeGetCurrentThread(), __VA_ARGS__ ); \
    }

#define DBG_ENTER( FMT, ... ) \
    DBG_OUT( "Entering: " __FUNCTION__ ## FMT, __VA_ARGS__ );

#define DBG_LEAVE( FMT, ... ) \
    DBG_OUT( " Leaving: " __FUNCTION__ ## FMT, __VA_ARGS__ );

#define DBG_TRACE( FMT, ... ) \
    DBG_OUT( "      In: " __FUNCTION__ ## " " ## FMT, __VA_ARGS__ );

#else // !ENABLE_TRACING  
#define DBG_ENTER( ... )
#define DBG_LEAVE( ... )
#define DBG_TRACE( ... )
#endif // !ENABLE_TRACING  

