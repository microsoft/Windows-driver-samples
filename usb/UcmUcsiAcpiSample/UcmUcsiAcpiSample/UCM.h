/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    UCM.h

Abstract:

    Routines for interfacing with the system USB Connector Manager.

Environment:

    Kernel-mode and user-mode.

--*/

#pragma once

#include <acpitabl.h>


_IRQL_requires_max_(HIGH_LEVEL)
ULONG64
FORCEINLINE
UCM_CONNECTOR_ID_FROM_ACPI_PLD (
    _In_ PACPI_PLD_BUFFER PldBuffer
    )
{
    ULONG64 connectorId;

    connectorId = 0;
    connectorId |= (PldBuffer->GroupToken & 0xFF);
    connectorId <<= 8;
    connectorId |= (PldBuffer->GroupPosition & 0xFF);

    return connectorId;
}
