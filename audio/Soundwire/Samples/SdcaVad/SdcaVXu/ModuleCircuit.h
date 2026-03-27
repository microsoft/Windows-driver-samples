/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    ModuleCircuit.h

Abstract:

    Raw PDO for ACX circuits. This file contains routines to create Device
    and handle pnp requests

Environment:

    Kernel mode

--*/

#pragma once

PAGED_CODE_SEG
NTSTATUS
SdcaXu_CreateModuleCircuit(
    _In_ WDFDEVICE Device
);

