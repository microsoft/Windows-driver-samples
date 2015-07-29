/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    batclass_prepublish.h

Abstract:

    This module defines pre-publish definations to be made available in DDK/SDK.

    N.B. This file must not be included when batclass.h defines 
         BATTERY_MINIPORT_INFO_V1_1 type and BATTERY_CLASS_MINOR_VERSION_1.

    N.B. This code is provided "AS IS" without any expressed or implied warranty.

--*/

//---------------------------------------------------------------------- Pragmas

#pragma once

//--------------------------------------------------------------------- Includes

#include <batclass.h>

//-------------------------------------------------- Would be Public Definitions

#ifndef BATTERY_CLASS_MINOR_VERSION_1

typedef struct {
    USHORT                          MajorVersion;
    USHORT                          MinorVersion;

    PVOID                           Context;        // Miniport context

    BCLASS_QUERY_TAG                QueryTag;
    BCLASS_QUERY_INFORMATION        QueryInformation;
    BCLASS_SET_INFORMATION          SetInformation;
    BCLASS_QUERY_STATUS             QueryStatus;
    BCLASS_SET_STATUS_NOTIFY        SetStatusNotify;
    BCLASS_DISABLE_STATUS_NOTIFY    DisableStatusNotify;
    PDEVICE_OBJECT                  Pdo;
    PUNICODE_STRING                 DeviceName;
    PDEVICE_OBJECT                  Fdo;
} BATTERY_MINIPORT_INFO_V1_1, *PBATTERY_MINIPORT_INFO_V1_1;

#define BATTERY_CLASS_MINOR_VERSION_1   0x0001

#endif // BATTERY_CLASS_MINOR_VERSION_1
