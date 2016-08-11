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

#include <initguid.h>
#include <ntddk.h>
#include <wdmguid.h>
#include <wdf.h>
#include <intsafe.h>
#include <ntstrsafe.h>
#include <acpiioct.h>
#include <acpitabl.h>

#include <UcmCx.h>
#include <UcsiInterface.h>
#include <Ucsi.h>

#include "Trace.h"
#include "Driver.h"
#include "Acpi.h"
#include "UcmCallbacks.h"
#include "Ppm.h"
#include "Fdo.h"
#include "UcsiUcmConvert.h"
#include "UcmNotifications.h"