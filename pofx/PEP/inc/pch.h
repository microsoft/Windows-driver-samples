/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    pch.h

Abstract:

    This file contains pre-compiled header files.


Environment:

    Kernel mode

--*/

#pragma once

#define NT_PROCESSOR_GROUPS 1

#include "common.h" // DDK, WDF headers

#pragma warning(push)
#pragma warning(disable:4214) /* nonstandard extension used: bit field types other then int */
#pragma warning(disable:4201) /* nonstandard extension used: nameless struct/union */
#include <acpitabl.h>
#include <acpiioct.h>
#include "ntpoapi.h" // public PO interfaces

//
// Opt-into V3 PEP interfaces
//
#define PEP_KERNEL_INFORMATION_VERSION PEP_KERNEL_INFORMATION_V3
#include "pepfx.h"

#if defined(EVENT_TRACING)

#include <wpprecorder.h>

#else

//
// Mock version of the handle type used everywhere.
// We set all instances of this type to NULL.
//

typedef PVOID RECORDER_LOG;

#endif

#include "trace.h"
#include "pep.h"

#pragma warning(pop)

#include <ntintsafe.h>

