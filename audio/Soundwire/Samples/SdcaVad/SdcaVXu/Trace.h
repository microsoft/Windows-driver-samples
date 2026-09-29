/*++

Copyright (c) Microsoft Corporation

Module Name:

Trace.h

--*/

#pragma once 

#include <WppRecorder.h>
#include <evntrace.h> // For TRACE_LEVEL definitions

#define WPP_TOTAL_BUFFER_SIZE (PAGE_SIZE)
#define WPP_ERROR_PARTITION_SIZE (WPP_TOTAL_BUFFER_SIZE/4)

// {47BE3522-EAE5-47F4-9EE0-BC4260C39937}
#define WPP_CONTROL_GUIDS \
WPP_DEFINE_CONTROL_GUID(DrvLogger,(47BE3522,EAE5,47F4,9EE0,BC4260C39937), \
        WPP_DEFINE_BIT(FLAG_DEVICE_ALL)       /* bit  0 = 0x00000001 */ \
        WPP_DEFINE_BIT(FLAG_FUNCTION)         /* bit  1 = 0x00000002 */ \
        WPP_DEFINE_BIT(FLAG_INFO)             /* bit  2 = 0x00000004 */ \
        WPP_DEFINE_BIT(FLAG_PNP)              /* bit  3 = 0x00000008 */ \
        WPP_DEFINE_BIT(FLAG_POWER)            /* bit  4 = 0x00000010 */ \
        WPP_DEFINE_BIT(FLAG_STREAM)           /* bit  5 = 0x00000020 */ \
        WPP_DEFINE_BIT(FLAG_INIT)             /* bit  6 = 0x00000040 */ \
        WPP_DEFINE_BIT(FLAG_DDI)              /* bit  7 = 0x00000080 */ \
        WPP_DEFINE_BIT(FLAG_GENERIC)          /* bit  8 = 0x00000100 */ \
        )

#include "trace_macros.h"

