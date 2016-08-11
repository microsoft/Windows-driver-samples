/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    UcmCallbacks.cpp

Abstract:

    Interface for callbacks from UcmCx.

Environment:

    Kernel-mode only.

--*/

#include "Pch.h"
#include "UcmCallbacks.tmh"

#pragma alloc_text(PAGE, Ucm_EvtConnectorSetDataRole)
#pragma alloc_text(PAGE, Ucm_EvtConnectorSetPowerRole)

NTSTATUS
Ucm_EvtConnectorSetDataRole(
    _In_ UCMCONNECTOR Connector,
    _In_ UCM_DATA_ROLE DataRole
)
/*++

Routine Description:

    Set the power role on a given connector. This may require performing
    a data role swap.

Arguments:

    Connector - The connector on which to set the power role.

    DataRole - The data role to set on the connector.

Returns:

    NTSTATUS

--*/
{
    NTSTATUS status;
    WDFDEVICE device;
    PPPM_CONNECTOR connector;
    PPPM_CONTEXT ppmCtx;
    UCSI_USB_OPERATION_ROLE role;
    UCSI_CONTROL cmd;

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_UCMCALLBACKS);

    connector = PpmUcmConnector_GetContext(Connector)->PpmConnector;
    device = connector->WdfDevice;
    ppmCtx = &Fdo_GetContext(device)->PpmCtx;

    TRACE_INFO(TRACE_FLAG_UCMCALLBACKS, "[Device: 0x%p] Data role change to %!UCM_DATA_ROLE! requested", device, DataRole);

    if (!Convert(DataRole, role))
    {
        status = STATUS_INVALID_PARAMETER;
        TRACE_ERROR(TRACE_FLAG_UCMCALLBACKS, "[Device: 0x%p] Invalid data role requested %!UCM_DATA_ROLE!", device, DataRole);
        goto Exit;
    }

    cmd.AsUInt64 = 0;
    cmd.Command = UcsiCommandSetUor;
    cmd.SetUor.ConnectorNumber = connector->Index;
    cmd.SetUor.UsbOperationRole = role;

    status = Ppm_SendCommand(ppmCtx, cmd, Ucm_EvtSetDataRoleCompleted, ppmCtx);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_UCMCALLBACKS);

    return status;
}


NTSTATUS
Ucm_EvtConnectorSetPowerRole(
    _In_ UCMCONNECTOR Connector,
    _In_ UCM_POWER_ROLE PowerRole
)
/*++

Routine Description:

    Set the power role on a given connector. This may require performing
    a power role swap.

Arguments:

    Connector - The connector on which to set the power role.

    PowerRole - The power role to set on the connector.

Returns:

    NTSTATUS

--*/
{
    NTSTATUS status;
    WDFDEVICE device;
    PPPM_CONNECTOR connector;
    PPPM_CONTEXT ppmCtx;
    UCSI_POWER_DIRECTION_ROLE role;
    UCSI_CONTROL cmd;

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_UCMCALLBACKS);

    connector = PpmUcmConnector_GetContext(Connector)->PpmConnector;
    device = connector->WdfDevice;
    ppmCtx = &Fdo_GetContext(device)->PpmCtx;

    TRACE_INFO(TRACE_FLAG_UCMCALLBACKS, "[Device: 0x%p] Power role change to %!UCM_POWER_ROLE! requested", device, PowerRole);

    if (!Convert(PowerRole, role))
    {
        status = STATUS_INVALID_PARAMETER;
        TRACE_ERROR(TRACE_FLAG_UCMCALLBACKS, "[Device: 0x%p] Invalid power role requested %!UCM_POWER_ROLE!", device, PowerRole);
        goto Exit;
    }

    cmd.AsUInt64 = 0;
    cmd.Command = UcsiCommandSetPdr;
    cmd.SetPdr.ConnectorNumber = connector->Index;
    cmd.SetPdr.PowerDirectionRole = role;

    status = Ppm_SendCommand(ppmCtx, cmd, Ucm_EvtSetPowerRoleCompleted, ppmCtx);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_UCMCALLBACKS);

    return status;
}