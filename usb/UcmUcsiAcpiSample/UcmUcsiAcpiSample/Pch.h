/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    Pch.h

Abstract:

    Precompiled header.

Environment:

    Kernel-mode only.

--*/

#pragma once

#define TAG_UCSI 'cAcU'

#include <initguid.h>
#include <ntddk.h>
#include <wdmguid.h>
#include <wdf.h>
#include <intsafe.h>
#include <ntstrsafe.h>
#include <acpiioct.h>
#include <acpitabl.h>

#include <UcmUcsi\1.0\UcmucsiCx.h>
#include <UCm\1.0\UcmCx.h>

#include "UCM.h"
#include "ProjectCommon.h"
#include "Trace.h"
#include "Driver.h"
#include "Acpi.h"
#include "Fdo.h"
#include "Ppm.h"
