/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    simtc.h

Abstract:

    This is the header file for the simulated thermal client driver.

@@BEGIN_DDKSPLIT
Author:

    Nicholas Brekhus (NiBrekhu) 25-Jul-2011

Revision History:

@@END_DDKSPLIT
--*/

//--------------------------------------------------------------------- Pragmas

#pragma once

//-------------------------------------------------------------------- Includes

#include <ntddk.h>
#include <wdf.h>
#include <ntstrsafe.h>
#include <initguid.h>
#include <wdmguid.h>
#include <poclass.h>

//----------------------------------------------------------------------- Debug

#if DBG
    extern ULONG SimThermalClientDebug;
    #define DebugPrint(l, m, ...) \
        if(l & SimThermalClientDebug)  DbgPrint(m, __VA_ARGS__)
    #define DebugEnter() \
        DebugPrint(SIMTC_TRACE, "Entering " __FUNCTION__ "\n")
    #define DebugExit() \
        DebugPrint(SIMTC_TRACE, "Leaving " __FUNCTION__ "\n")
    #define DebugExitStatus(_status_) \
        DebugPrint(SIMTC_TRACE, "Leaving " __FUNCTION__ ": Status=0x%x\n", _status_)
#else
    #define DebugPrint(l, m, ...)
    #define DebugEnter()
    #define DebugExit()
    #define DebugExitStatus(_status_)
#endif

#define SIMTC_LOW          0x00000001
#define SIMTC_NOTE         0x00000002
#define SIMTC_WARN         0x00000004
#define SIMTC_ERROR_ONLY   0x00000008
#define SIMTC_ERROR        (SIMTC_ERROR_ONLY | SIMTC_WARN)
#define SIMTC_POWER        0x00000010
#define SIMTC_PNP          0x00000020
#define SIMTC_TRACE        0x00000040
#define SIMTC_DEBUG        0x80000000
#define SIMTC_PRINT_ALWAYS 0xffffffff
#define SIMTC_PRINT_NEVER  0x0

#define SimTcTag 'SChT'

//----------------------------------------------------------------- Definitions

typedef struct {
    BOOLEAN ActiveCoolingEngaged;
    ULONG ThermalLevel;
} FDO_DATA, *PFDO_DATA;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FDO_DATA, GetDeviceExtension);

