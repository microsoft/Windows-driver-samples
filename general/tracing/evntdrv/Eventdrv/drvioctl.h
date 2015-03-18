/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.


Module Name:

    drvioctl.h

Abstract:

    Definitions of IOCTL codes and data structures exported by TRACEDRV.


--*/

#ifndef __EVENTKMP_IOCTL__
#define __EVENTKMP_IOCTL__

//
// IOCTL control codes
//
#define IOCTL_EVNTKMP_TRACE_EVENT_A         \
    CTL_CODE( FILE_DEVICE_UNKNOWN, 0x801,   \
        METHOD_BUFFERED, FILE_ANY_ACCESS )

#endif // __EVENTKMP_IOCTL__


