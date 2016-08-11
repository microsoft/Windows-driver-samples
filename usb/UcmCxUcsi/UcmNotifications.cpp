/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    UcmNotifications.cpp

Abstract:

    Type-C Platform Policy Manager. Interface to notify UcmCx of events.

Environment:

Kernel-mode only.

--*/

#include "Pch.h"
#include "UcmNotifications.tmh"

#pragma alloc_text(PAGE, Ucm_CreateConnectors)
#pragma alloc_text(PAGE, Ucm_EvtGetPdosCompleted)
#pragma alloc_text(PAGE, Ucm_EvtSetDataRoleCompleted)
#pragma alloc_text(PAGE, Ucm_EvtSetPowerRoleCompleted)
#pragma alloc_text(PAGE, Ucm_ReportTypeCAttach)
#pragma alloc_text(PAGE, Ucm_ReportTypeCDetach)
#pragma alloc_text(PAGE, Ucm_ReportPowerOperationMode)
#pragma alloc_text(PAGE, Ucm_ReportPowerDirectionChanged)
#pragma alloc_text(PAGE, Ucm_ReportDataDirectionChanged)
#pragma alloc_text(PAGE, Ucm_ReportChargingStatusChanged)

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Ucm_CreateConnectors (
    _In_ PPPM_CONTEXT PpmCtx
)
/*++

Routine Description:

    Create connector objects and register them with UCM.

Arguments:

    PpmCtx - Platform policy manager context object.

--*/
{
    NTSTATUS status;
    WDFDEVICE device;
    UCSI_GET_CAPABILITY_IN caps;
    UCSI_GET_CONNECTOR_CAPABILITY_IN connCaps;
    UCM_CONNECTOR_CONFIG connConfig;
    UCM_CONNECTOR_TYPEC_CONFIG typeCConfig;
    UCM_CONNECTOR_PD_CONFIG pdConfig;
    ULONG supportedOperatingModes;
    ULONG supportedPowerSourcingCapabilities;
    ULONG supportedPdPowerRoles;
    PPPM_CONNECTOR connector;
    ULONG connectorCount;
    WDF_OBJECT_ATTRIBUTES attributes;
    PPPM_UCM_CONNECTOR_CONTEXT connectorCtx;
    CONNECTOR_INDEX i;

    UNREFERENCED_PARAMETER(PpmCtx);

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_UCMNOTIFICATIONS);

    device = Context_GetWdfDevice(PpmCtx);

    status = Ppm_GetCapability(PpmCtx, &caps);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    TRACE_INFO(TRACE_FLAG_UCMNOTIFICATIONS, "[Device: 0x%p] PPM optional features: 0x%x", device, caps.OptionalFeatures);

    connectorCount = WdfCollectionGetCount(PpmCtx->Connectors);
    if (connectorCount != caps.bNumConnectors)
    {
        status = STATUS_DEVICE_PROTOCOL_ERROR;
        TRACE_ERROR(TRACE_FLAG_UCMNOTIFICATIONS, "[Device: 0x%p] Connector count mismatch. ACPI reported %lu, UCSI reported %lu", device, connectorCount, caps.bNumConnectors);
        goto Exit;
    }

    for (i = 1; i <= connectorCount; ++i)
    {
        status = Ppm_GetConnectorCapability(PpmCtx, i, &connCaps);
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }

        connector = Ppm_GetConnector(PpmCtx, i);

        supportedOperatingModes = (connCaps.OperationMode.DfpOnly ? UcmTypeCOperatingModeDfp : 0) |
            (connCaps.OperationMode.UfpOnly ? UcmTypeCOperatingModeUfp : 0) |
            (connCaps.OperationMode.Drp ? UcmTypeCOperatingModeDrp : 0);

        if (connCaps.Provider)
        {
            supportedPowerSourcingCapabilities = UcmTypeCCurrentDefaultUsb |
                UcmTypeCCurrent1500mA |
                UcmTypeCCurrent3000mA;
        }
        else
        {
            supportedPowerSourcingCapabilities = 0;
        }

        //
        // Assemble the Type-C and PD configuration for UCM.
        //

        UCM_CONNECTOR_CONFIG_INIT(&connConfig, connector->Id);

        UCM_CONNECTOR_TYPEC_CONFIG_INIT(&typeCConfig,
            supportedOperatingModes,
            supportedPowerSourcingCapabilities);

        typeCConfig.EvtSetDataRole = Ucm_EvtConnectorSetDataRole;
        typeCConfig.AudioAccessoryCapable = connCaps.OperationMode.AudioAccessoryMode;

        supportedPdPowerRoles = (connCaps.Provider ? UcmPowerRoleSource : 0) |
            (connCaps.Consumer ? UcmPowerRoleSink : 0);

        UCM_CONNECTOR_PD_CONFIG_INIT(&pdConfig, supportedPdPowerRoles);

        pdConfig.EvtSetPowerRole = Ucm_EvtConnectorSetPowerRole;

        connConfig.TypeCConfig = &typeCConfig;
        connConfig.PdConfig = &pdConfig;

        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, PPM_UCM_CONNECTOR_CONTEXT);

        //
        // Create the UCM connector object.
        //

        status = UcmConnectorCreate(device, &connConfig, &attributes, &connector->Handle);
        if (!NT_SUCCESS(status))
        {
            TRACE_ERROR(TRACE_FLAG_UCMNOTIFICATIONS, "[Device: 0x%p] UcmConnectorCreate failed - %!STATUS!", device, status);
            goto Exit;
        }

        connectorCtx = PpmUcmConnector_GetContext(connector->Handle);
        connectorCtx->PpmConnector = connector;
    }

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_UCMNOTIFICATIONS);

    return status;
}


VOID
Ucm_EvtGetPdosCompleted (
    _In_ UCSI_CONTROL Command,
    _In_ PVOID Context,
    _Inout_ PPPM_COMMAND_ACK_PARAMS CommandAckParams
)
/*++

Routine Description:

    GetPdos command completion routine. Notifies UCM of new source capabilities and PD contract.

Arguments:

    Command - The UCSI command that was completed. In this case, it should only be UcsiCommandGetPdos.

    Context - Platform policy manager context object.

    CommandAckParams - UCSI command acknowledge parameters.

--*/
{
    WDFDEVICE device;
    PPPM_CONTEXT ppmCtx;
    PUCSI_GET_PDOS_IN pdos;
    UCHAR pdoCount;
    UINT8 pdoOffset;
    UCSI_CONTROL newCmd;
    PPPM_CONNECTOR connector;
    UCM_CONNECTOR_PD_CONN_STATE_CHANGED_PARAMS connParams;
    bool forPartner;
    NTSTATUS status;

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_UCMNOTIFICATIONS);

    ppmCtx = (PPPM_CONTEXT)Context;
    device = Context_GetWdfDevice(ppmCtx);

    NT_ASSERT(Command.Command == UcsiCommandGetPdos);

    //
    // This GetPdos command was sent in response to a change in the PD contract. So we
    // need to acknowledge the connector change that we know is pending.
    //

    CommandAckParams->AckConnectorChange = TRUE;

    if (!UCSI_CMD_SUCCEEDED(ppmCtx->UcsiDataBlock->CCI))
    {
        //
        // The command failed. Acknowledge the connector change and move on.
        //

        goto Exit;
    }

    pdos = &ppmCtx->UcsiDataBlock->MessageIn.Pdos;
    pdoOffset = Command.GetPdos.PdoOffset;
    pdoCount = (UCHAR)(ppmCtx->UcsiDataBlock->CCI.DataLength / sizeof(pdos->Pdos[0]));
    connector = Ppm_GetConnector(ppmCtx, Command.GetPdos.ConnectorNumber);
    forPartner = (Command.GetPdos.PartnerPdo == 1);

    if (pdoCount == 0)
    {
        if (forPartner)
        {
            TRACE_ERROR(TRACE_FLAG_UCMNOTIFICATIONS, "[Device: 0x%p] Port partner of connector ID 0x%I64x returned zero source PDOs", device, connector->Id);
        }
        else
        {
            TRACE_ERROR(TRACE_FLAG_UCMNOTIFICATIONS, "[Device: 0x%p] Connector ID 0x%I64x returned zero source PDOs", device, connector->Id);
        }

        goto Exit;
    }

    NT_ASSERT_ASSUME(pdoCount <= MAX_MESSAGE_IN_PDOS);

    if (pdoOffset >= ARRAYSIZE(ppmCtx->ConnectorChangeCtx.Pdos))
    {
        TRACE_ERROR(TRACE_FLAG_UCMNOTIFICATIONS, "[Device: 0x%p] Invalid offset (%u) used in %!UCSI_COMMAND! command on connector ID 0x%I64x", device, pdoOffset, Command.Command, connector->Id);
        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    RtlCopyMemory(&ppmCtx->ConnectorChangeCtx.Pdos[pdoOffset], pdos->Pdos, pdoCount * sizeof(pdos->Pdos[0]));
    ppmCtx->ConnectorChangeCtx.PdoCount = pdoOffset + pdoCount;

    //
    // Check to see if there might be more PDOs to retrieve. If so, don't acknowledge the connector
    // change just yet, and send the command again to fetch the remaining PDOs.
    //
    // If we got 4 PDOs in this batch (the maximum we can retrieve at once), and the total number of
    // PDOs we have received so far is less than 7 (the maximum allowed by the PD spec), there may
    // be more PDOs to retrieve.
    //

    if ((pdoCount == MAX_MESSAGE_IN_PDOS) && (ppmCtx->ConnectorChangeCtx.PdoCount < MAX_PDO_COUNT))
    {
        newCmd = Command;
        newCmd.GetPdos.PdoOffset = ppmCtx->ConnectorChangeCtx.PdoCount;

        status = Ppm_SendCommand(ppmCtx, newCmd, Ucm_EvtGetPdosCompleted, ppmCtx);
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }

        CommandAckParams->AckConnectorChange = FALSE;
        goto Exit;
    }

    if (forPartner)
    {
        //
        // Notify UCM of the partner's source capabilities.
        //

        TRACE_INFO(TRACE_FLAG_UCMNOTIFICATIONS, "[Device: 0x%p] Reporting PD port partner source caps on connector ID 0x%I64x", device, connector->Id);
        status = UcmConnectorPdPartnerSourceCaps(connector->Handle,
            ppmCtx->ConnectorChangeCtx.Pdos,
            ppmCtx->ConnectorChangeCtx.PdoCount);
        if (!NT_SUCCESS(status))
        {
            TRACE_ERROR(TRACE_FLAG_UCMNOTIFICATIONS, "[Device: 0x%p] UcmConnectorPdPartnerSourceCaps for connector ID 0x%I64x failed - %!STATUS!", device, connector->Id, status);
            goto Exit;
        }
    }
    else
    {
        //
        // Notify UCM of our own source capabilities.
        //

        TRACE_INFO(TRACE_FLAG_UCMNOTIFICATIONS, "[Device: 0x%p] Reporting PD source caps on connector ID 0x%I64x", device, connector->Id);
        status = UcmConnectorPdSourceCaps(connector->Handle,
            ppmCtx->ConnectorChangeCtx.Pdos,
            ppmCtx->ConnectorChangeCtx.PdoCount);
        if (!NT_SUCCESS(status))
        {
            TRACE_ERROR(TRACE_FLAG_UCMNOTIFICATIONS, "[Device: 0x%p] UcmConnectorPdSourceCaps for connector ID 0x%I64x failed - %!STATUS!", device, connector->Id, status);
            goto Exit;
        }
    }

    UCM_CONNECTOR_PD_CONN_STATE_CHANGED_PARAMS_INIT(&connParams,
        UcmPdConnStateNegotiationSucceeded);

    if (forPartner)
    {
        connParams.ChargingState = ppmCtx->ConnectorChangeCtx.ChargingState;
    }
    connParams.Rdo = ppmCtx->ConnectorChangeCtx.Rdo;

    //
    // Notify UCM that the PD connection state has changed on a certain connector.
    //

    TRACE_INFO(TRACE_FLAG_UCMNOTIFICATIONS, "[Device: 0x%p] Reporting PD connection state change on connector ID 0x%I64x", device, connector->Id);

    status = UcmConnectorPdConnectionStateChanged(connector->Handle, &connParams);
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_UCMNOTIFICATIONS, "[Device: 0x%p] UcmConnectorPdConnectionStateChanged for connector ID 0x%I64x failed - %!STATUS!", device, connector->Id, status);
        goto Exit;
    }

    //
    // A PD contract has been established at this point. Perform role correction if necessary.
    //

    if (connector->PerformRoleCorrectionOnNextPdContract)
    {
        connector->PerformRoleCorrectionOnNextPdContract = FALSE;
        status = Ppm_PerformRoleCorrection(ppmCtx, connector);
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }
    }

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_UCMNOTIFICATIONS);
}


VOID
Ucm_EvtSetPowerRoleCompleted (
    _In_ UCSI_CONTROL Command,
    _In_ PVOID Context,
    _Inout_ PPPM_COMMAND_ACK_PARAMS CommandAckParams
)
/*++

Routine Description:

    Set power role command completion routine. Notifies UCM that the power direction has changed.

Arguments:

    Command - The UCSI command that was completed. In this case, it should be only SetPdr.PowerDirectionRole.

    Context - Platform policy manager context object.

    CommandAckParams - UCSI command acknowledge parameters.

--*/
{
    UCM_POWER_ROLE powerRole;
    PPPM_CONTEXT ppmCtx;
    WDFDEVICE device;
    PPPM_CONNECTOR connector;
    BOOLEAN success;

    UNREFERENCED_PARAMETER(CommandAckParams);

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_UCMNOTIFICATIONS);

    ppmCtx = (PPPM_CONTEXT)Context;
    device = Context_GetWdfDevice(ppmCtx);
    connector = Ppm_GetConnector(ppmCtx, Command.SetPdr.ConnectorNumber);

    NT_VERIFY(Convert((UCSI_POWER_DIRECTION_ROLE)Command.SetPdr.PowerDirectionRole, powerRole));

    success = UCSI_CMD_SUCCEEDED(ppmCtx->UcsiDataBlock->CCI);

    if (success)
    {
        TRACE_INFO(TRACE_FLAG_UCMNOTIFICATIONS, "[Device: 0x%p] Power role successfully changed to %!UCM_POWER_ROLE!", device, powerRole);
    }
    else
    {
        TRACE_ERROR(TRACE_FLAG_UCMNOTIFICATIONS, "[Device: 0x%p] Power role change to %!UCM_POWER_ROLE! failed", device, powerRole);
    }

    //
    // Notify UCM that the power direction has changed.
    //

    UcmConnectorPowerDirectionChanged(connector->Handle, success, powerRole);

    TRACE_FUNC_EXIT(TRACE_FLAG_UCMNOTIFICATIONS);
}


VOID
Ucm_EvtSetDataRoleCompleted (
    _In_ UCSI_CONTROL Command,
    _In_ PVOID Context,
    _Inout_ PPPM_COMMAND_ACK_PARAMS CommandAckParams
)
/*++

Routine Description:

    Set data role command completion routine. Notifies UCM that the power direction has changed.

Arguments:

    Command - The UCSI command that was completed. In this case, it should be only SetUor.UsbOperationRole.

    Context - Platform policy manager context object.

    CommandAckParams - UCSI command acknowledge parameters.

--*/
{
    UCM_DATA_ROLE dataRole;
    PPPM_CONTEXT ppmCtx;
    WDFDEVICE device;
    PPPM_CONNECTOR connector;
    BOOLEAN success;

    UNREFERENCED_PARAMETER(CommandAckParams);

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_UCMNOTIFICATIONS);

    ppmCtx = (PPPM_CONTEXT)Context;
    device = Context_GetWdfDevice(ppmCtx);
    connector = Ppm_GetConnector(ppmCtx, Command.SetUor.ConnectorNumber);

    NT_VERIFY(Convert((UCSI_USB_OPERATION_ROLE)Command.SetUor.UsbOperationRole, dataRole));

    success = UCSI_CMD_SUCCEEDED(ppmCtx->UcsiDataBlock->CCI);

    if (success)
    {
        TRACE_INFO(TRACE_FLAG_UCMNOTIFICATIONS, "[Device: 0x%p] Data role successfully changed to %!UCM_DATA_ROLE!", device, dataRole);
    }
    else
    {
        TRACE_ERROR(TRACE_FLAG_UCMNOTIFICATIONS, "[Device: 0x%p] Data role change to %!UCM_DATA_ROLE! failed", device, dataRole);
    }

    //
    // Notify UCM that the data direction has changed.
    //

    UcmConnectorDataDirectionChanged(connector->Handle, success, dataRole);

    TRACE_FUNC_EXIT(TRACE_FLAG_UCMNOTIFICATIONS);
}


_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
Ucm_ReportPowerOperationMode (
    _In_ PPPM_CONTEXT PpmCtx,
    _In_ PPPM_CONNECTOR Connector,
    _Inout_ PUCSI_GET_CONNECTOR_STATUS_IN ConnStatus,
    _Inout_ PPPM_COMMAND_ACK_PARAMS CommandAckParams
)
/*++

Routine Description:

    Report power information to UCM.

Arguments:

    PpmCtx - Platform policy manager context object.

    Connector - The connector for which to report the mode.

    ConnStatus - The connector status for the given connector.

    CommandAckParams - UCSI command acknowledge parameters.

--*/
{
    WDFDEVICE device;
    NTSTATUS status;
    UCM_TYPEC_CURRENT currentAd;

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_UCMNOTIFICATIONS);

    device = Context_GetWdfDevice(PpmCtx);

    if (ConnStatus->PowerOperationMode == UcsiPowerOperationModePd)
    {
        Ppm_ReportNegotiatedPowerLevelChanged(PpmCtx, Connector, ConnStatus, CommandAckParams);

        goto Exit;
    }

    //
    // The other cases indicate a change in Type-C current advertisement.
    //
    if (!Convert((UCSI_POWER_OPERATION_MODE)ConnStatus->PowerOperationMode, currentAd))
    {
        TRACE_ERROR(TRACE_FLAG_UCMNOTIFICATIONS, "[Device: 0x%p] Invalid power operation mode %u", device, ConnStatus->PowerOperationMode);
        goto Exit;
    }

    TRACE_INFO(TRACE_FLAG_UCMNOTIFICATIONS, "[Device: 0x%p] Reporting Type-C current advertisement change on connector ID 0x%I64x", device, Connector->Id);

    //
    // Notify UCM that the Type-C current advertisement has changed.
    //

    status = UcmConnectorTypeCCurrentAdChanged(Connector->Handle, currentAd);
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_UCMNOTIFICATIONS, "[Device: 0x%p] UcmConnectorTypeCCurrentAdChanged for connector 0x%I64x failed - %!STATUS!", device, Connector->Id, status);
        goto Exit;
    }

    ConnStatus->ConnectorStatusChange.PowerOperationModeChange = 0;

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_UCMNOTIFICATIONS);
}


_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
Ucm_ReportTypeCAttach (
    _In_ PPPM_CONTEXT PpmCtx,
    _In_ PPPM_CONNECTOR Connector,
    _Inout_ PUCSI_GET_CONNECTOR_STATUS_IN ConnStatus,
    _Inout_ PPPM_COMMAND_ACK_PARAMS CommandAckParams
)
/*++

Routine Description:

    Report a Type-C attach event to UCM.

Arguments:

    PpmCtx - Platform policy manager context object.

    Connector - The connector for which to report the attach.

    ConnStatus - The connector status for the given connector.

    CommandAckParams - UCSI command acknowledge parameters.

--*/
{
    WDFDEVICE device;
    NTSTATUS status;
    UCM_CONNECTOR_TYPEC_ATTACH_PARAMS typeCParams;
    UCM_TYPEC_PARTNER partner;

    UNREFERENCED_PARAMETER(CommandAckParams);

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_UCMNOTIFICATIONS);

    device = Context_GetWdfDevice(PpmCtx);

    TRACE_INFO(TRACE_FLAG_UCMNOTIFICATIONS, "[Device: 0x%p] Reporting Type-C attach on connector ID 0x%I64x", device, Connector->Id);

    if (!Convert((UCSI_CONNECTOR_PARTNER_TYPE)ConnStatus->ConnectorPartnerType, partner))
    {
        TRACE_ERROR(TRACE_FLAG_UCMNOTIFICATIONS, "[Device: 0x%p] Invalid connector partner type %u", device, ConnStatus->ConnectorPartnerType);
        goto Exit;
    }

    UCM_CONNECTOR_TYPEC_ATTACH_PARAMS_INIT(&typeCParams, partner);

    if (ConnStatus->PowerDirection == UcsiPowerDirectionConsumer)
    {
        if (!Convert((UCSI_BATTERY_CHARGING_STATUS)ConnStatus->BatteryChargingStatus,
            typeCParams.ChargingState))
        {
            TRACE_ERROR(TRACE_FLAG_UCMNOTIFICATIONS, "[Device: 0x%p] Invalid battery charging state %u", device, ConnStatus->BatteryChargingStatus);
            goto Exit;
        }
    }

    //
    // N.B. Even in the case of PD, convert the power operation mode to the some Type-C
    //      current advertisement. UCSI has no way of telling us the Type-C advertisement when
    //      there is a PD connection, and the PD spec has certain rules around what that
    //      advertisement should be. Since we don't have that information from UCSI,
    //      we simply assume the current level is 3.0A in the PD case.
    //

    if (!Convert((UCSI_POWER_OPERATION_MODE)ConnStatus->PowerOperationMode,
        typeCParams.CurrentAdvertisement))
    {
        TRACE_ERROR(TRACE_FLAG_UCMNOTIFICATIONS, "[Device: 0x%p] Invalid connector power operation mode %u", device, ConnStatus->PowerOperationMode);
        goto Exit;
    }

    //
    // Notify UCM that we have detected an attach event on the connector.
    //

    status = UcmConnectorTypeCAttach(Connector->Handle, &typeCParams);
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_UCMNOTIFICATIONS, "[Device: 0x%p] UcmConnectorTypeCAttach failed - %!STATUS!", device, status);
        goto Exit;
    }

    //
    // Clear all the bits for information we may have processed here.
    //

    ConnStatus->ConnectorStatusChange.ConnectChange = 0;
    ConnStatus->ConnectorStatusChange.ConnectorPartnerChange = 0;
    ConnStatus->ConnectorStatusChange.BatteryChargingStatusChange = 0;
    ConnStatus->ConnectorStatusChange.PowerOperationModeChange = 0;

    if (!PpmCtx->IsUsbDeviceControllerEnabled && (partner == UcmTypeCPartnerDfp))
    {
        Connector->PerformRoleCorrectionOnNextPdContract = TRUE;
    }
    else
    {
        Connector->PerformRoleCorrectionOnNextPdContract = FALSE;
    }

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_UCMNOTIFICATIONS);
}


_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
Ucm_ReportTypeCDetach (
    _In_ PPPM_CONTEXT PpmCtx,
    _In_ PPPM_CONNECTOR Connector,
    _Inout_ PUCSI_GET_CONNECTOR_STATUS_IN ConnStatus,
    _Inout_ PPPM_COMMAND_ACK_PARAMS CommandAckParams
)
/*++

Routine Description:

    Report a Type-C detach event to UCM.

Arguments:

    PpmCtx - Platform policy manager context object.

    Connector - The connector for which to report the detach.

    ConnStatus - The connector status for the given connector.

    CommandAckParams - UCSI command acknowledge parameters.

--*/
{
    WDFDEVICE device;
    NTSTATUS status;

    UNREFERENCED_PARAMETER(ConnStatus);
    UNREFERENCED_PARAMETER(CommandAckParams);

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_UCMNOTIFICATIONS);

    device = Context_GetWdfDevice(PpmCtx);

    TRACE_INFO(TRACE_FLAG_UCMNOTIFICATIONS, "[Device: 0x%p] Reporting Type-C detach on connector ID 0x%I64x", device, Connector->Id);

    //
    // Notify UCM that we have detected a detach event on the connector.
    //

    status = UcmConnectorTypeCDetach(Connector->Handle);
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_UCMNOTIFICATIONS, "[Device: 0x%p] UcmConnectorTypeCDetach failed - %!STATUS!", device, status);
        goto Exit;
    }

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_UCMNOTIFICATIONS);
}


_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
Ucm_ReportPowerDirectionChanged (
    _In_ PPPM_CONTEXT PpmCtx,
    _In_ PPPM_CONNECTOR Connector,
    _Inout_ PUCSI_GET_CONNECTOR_STATUS_IN ConnStatus,
    _Inout_ PPPM_COMMAND_ACK_PARAMS CommandAckParams
)
/*++

Routine Description:

    Report to UCM that the power direction has changed. This may be called
    after a role swap completes.

Arguments:

    PpmCtx - Platform policy manager context object.

    Connector - The connector for which to report the power direction change.

    ConnStatus - The connector status for the given connector.

    CommandAckParams - UCSI command acknowledge parameters.

--*/
{
    WDFDEVICE device;
    UCM_POWER_ROLE powerRole;

    UNREFERENCED_PARAMETER(CommandAckParams);

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_UCMNOTIFICATIONS);

    device = Context_GetWdfDevice(PpmCtx);

    if (!Convert((UCSI_POWER_DIRECTION)ConnStatus->PowerDirection, powerRole))
    {
        TRACE_ERROR(TRACE_FLAG_UCMNOTIFICATIONS, "[Device: 0x%p] Invalid power direction %u", device, ConnStatus->PowerDirection);
        goto Exit;
    }

    TRACE_INFO(TRACE_FLAG_UCMNOTIFICATIONS, "[Device: 0x%p] Reporting power role change to %!UCM_POWER_ROLE! on connector ID 0x%I64x", device, powerRole, Connector->Id);

    //
    // Notify UCM that the power direction has changed.
    //

    UcmConnectorPowerDirectionChanged(Connector->Handle, TRUE, powerRole);

    ConnStatus->ConnectorStatusChange.PowerDirectionChange = 0;

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_UCMNOTIFICATIONS);
}


_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
Ucm_ReportDataDirectionChanged (
    _In_ PPPM_CONTEXT PpmCtx,
    _In_ PPPM_CONNECTOR Connector,
    _Inout_ PUCSI_GET_CONNECTOR_STATUS_IN ConnStatus,
    _Inout_ PPPM_COMMAND_ACK_PARAMS CommandAckParams
)
/*++

Routine Description:

    Report to UCM that the data direction has changed. This may be called
    after a role swap completes.

Arguments:

    PpmCtx - Platform policy manager context object.

    Connector - The connector for which to report the data direction change.

    ConnStatus - The connector status for the given connector.

    CommandAckParams - UCSI command acknowledge parameters.

--*/
{
    WDFDEVICE device;
    UCM_TYPEC_PARTNER partnerType;
    UCM_DATA_ROLE newDataRole;

    UNREFERENCED_PARAMETER(CommandAckParams);

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_UCMNOTIFICATIONS);

    device = Context_GetWdfDevice(PpmCtx);

    if (!Convert((UCSI_CONNECTOR_PARTNER_TYPE)ConnStatus->ConnectorPartnerType, partnerType))
    {
        TRACE_ERROR(TRACE_FLAG_UCMNOTIFICATIONS, "[Device: 0x%p] Invalid connector partner type %u", device, ConnStatus->ConnectorPartnerType);
        goto Exit;
    }

    if ((partnerType != UcmTypeCPartnerDfp) && (partnerType != UcmTypeCPartnerUfp))
    {
        //
        // This is one of those other cases where the partner type changed, for instance,
        // PoweredCableNoUfp changed to PoweredCableWithUfp.
        //

        goto Exit;
    }

    newDataRole = (partnerType == UcmTypeCPartnerDfp) ? UcmDataRoleUfp : UcmDataRoleDfp;

    TRACE_INFO(TRACE_FLAG_UCMNOTIFICATIONS, "[Device: 0x%p] Reporting date role change to %!UCM_DATA_ROLE! on connector ID 0x%I64x", device, newDataRole, Connector->Id);

    //
    // Notify UCM that the data direction has changed.
    //

    UcmConnectorDataDirectionChanged(Connector->Handle, TRUE, newDataRole);

    ConnStatus->ConnectorStatusChange.ConnectorPartnerChange = 0;

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_UCMNOTIFICATIONS);
}


_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
Ucm_ReportChargingStatusChanged (
    _In_ PPPM_CONTEXT PpmCtx,
    _In_ PPPM_CONNECTOR Connector,
    _Inout_ PUCSI_GET_CONNECTOR_STATUS_IN ConnStatus,
    _Inout_ PPPM_COMMAND_ACK_PARAMS CommandAckParams
)
/*++

Routine Description:

    Report to UCM that the charging status has changed. This may be called
    after a role swap completes.

Arguments:

    PpmCtx - Platform policy manager context object.

    Connector - The connector for which to report the charging status change.

    ConnStatus - The connector status for the given connector.

    CommandAckParams - UCSI command acknowledge parameters.

--*/
{
    WDFDEVICE device;
    UCM_CHARGING_STATE chargingState;
    NTSTATUS status;

    UNREFERENCED_PARAMETER(CommandAckParams);

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_UCMNOTIFICATIONS);

    device = Context_GetWdfDevice(PpmCtx);

    if (!Convert((UCSI_BATTERY_CHARGING_STATUS)ConnStatus->BatteryChargingStatus, chargingState))
    {
        TRACE_ERROR(TRACE_FLAG_UCMNOTIFICATIONS, "[Device: 0x%p] Invalid charging state %u", device, ConnStatus->BatteryChargingStatus);
        goto Exit;
    }

    TRACE_INFO(TRACE_FLAG_UCMNOTIFICATIONS, "[Device: 0x%p] Reporting charging status change to %!UCM_CHARGING_STATE! on connector ID 0x%I64x", device, chargingState, Connector->Id);

    //
    // Notify UCM that the charging state has changed.
    //

    status = UcmConnectorChargingStateChanged(Connector->Handle, chargingState);
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_UCMNOTIFICATIONS, "[Device: 0x%p] UcmConnectorChargingStateChanged failed - %!STATUS!", device, status);
        goto Exit;
    }

    ConnStatus->ConnectorStatusChange.BatteryChargingStatusChange = 0;

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_UCMNOTIFICATIONS);
}
