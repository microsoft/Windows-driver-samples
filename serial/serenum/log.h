/*++

Copyright (c) 1993  Microsoft Corporation
:ts=4

Module Name:

    log.h

Abstract:

    debug macros

Environment:

    Kernel & user mode

Revision History:

    10-27-95 : created

--*/

#ifndef   __LOG_H__
#define   __LOG_H__


#define LOG_MISC          0x00000001        //debug log entries
#define LOG_ENUM          0x00000002
#define LOG_PASSTHROUGH   0x00000004

//
// Assert Macros
//

#if DBG
#define LOGENTRY(mask, sig, info1, info2, info3)      \
    SerenumDebugLogEntry(mask, sig, (ULONG_PTR)info1, \
                         (ULONG_PTR)info2,            \
                         (ULONG_PTR)info3)

VOID
SerenumDebugLogEntry(IN ULONG Mask, IN ULONG Sig, IN ULONG_PTR Info1,
                     IN ULONG_PTR Info2, IN ULONG_PTR Info3);

VOID
SerenumLogInit();

VOID
SerenumLogFree();

#else
#define LOGENTRY(mask, sig, info1, info2, info3)
#endif


#endif // __LOG_H__
