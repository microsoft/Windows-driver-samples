/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    change.h

Abstract:

    Header file which contains the structures, type definitions,
    constants, global variables and function prototypes that are
    only visible within the kernel. Mainly used by change module.

Environment:

    Kernel mode

--*/
#ifndef __CHANGE_H__
#define __CHANGE_H__

#define CG_VISTA    (NTDDI_VERSION >= NTDDI_VISTA)

#include <fltKernel.h>
#include <suppress.h>
#include "context.h"
#include "utility.h"

#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")

//
//  The global variable
//

PFLT_FILTER gFilterInstance;

#define CGDBG_TRACE_ROUTINES            0x00000001
#define CGDBG_TRACE_OPERATION_STATUS    0x00000002
#define CGDBG_TRACE_DEBUG               0x00000004
#define CGDBG_TRACE_ERROR               0x00000008

static ULONG gTraceFlags = CGDBG_TRACE_DEBUG | CGDBG_TRACE_ERROR;

#define CG_DBG_PRINT( _dbgLevel, _string )          \
    (FlagOn(gTraceFlags,(_dbgLevel)) ?              \
        DbgPrint _string :                          \
        ((int)0))

#endif
