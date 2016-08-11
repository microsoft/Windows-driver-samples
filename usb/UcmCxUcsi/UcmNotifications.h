/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    UcmNotifications.h

Abstract:

    Interface to notify UcmCx of events.

Environment:

    Kernel-mode only.

--*/
#pragma once

//
// There is only enough space for 4 PDOs in the message-in.
//
#define MAX_MESSAGE_IN_PDOS 4

//
// The PD specification defines an upper limit of 7 PDOs in a message.
//
#define MAX_PDO_COUNT 7

EXTERN_C_START

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Ucm_CreateConnectors(
    _In_ PPPM_CONTEXT PpmCtx
);

EVT_PPM_COMMAND_COMPLETION_ROUTINE Ucm_EvtGetPdosCompleted;
EVT_PPM_COMMAND_COMPLETION_ROUTINE Ucm_EvtSetDataRoleCompleted;
EVT_PPM_COMMAND_COMPLETION_ROUTINE Ucm_EvtSetPowerRoleCompleted;


_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
Ucm_ReportPowerOperationMode(
    _In_ PPPM_CONTEXT PpmCtx,
    _In_ PPPM_CONNECTOR Connector,
    _Inout_ PUCSI_GET_CONNECTOR_STATUS_IN ConnStatus,
    _Inout_ PPPM_COMMAND_ACK_PARAMS CommandAckParams
);

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
Ucm_ReportTypeCAttach(
    _In_ PPPM_CONTEXT PpmCtx,
    _In_ PPPM_CONNECTOR Connector,
    _Inout_ PUCSI_GET_CONNECTOR_STATUS_IN ConnStatus,
    _Inout_ PPPM_COMMAND_ACK_PARAMS CommandAckParams
);

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
Ucm_ReportTypeCDetach(
    _In_ PPPM_CONTEXT PpmCtx,
    _In_ PPPM_CONNECTOR Connector,
    _Inout_ PUCSI_GET_CONNECTOR_STATUS_IN ConnStatus,
    _Inout_ PPPM_COMMAND_ACK_PARAMS CommandAckParams
);

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
Ucm_ReportPowerDirectionChanged(
    _In_ PPPM_CONTEXT PpmCtx,
    _In_ PPPM_CONNECTOR Connector,
    _Inout_ PUCSI_GET_CONNECTOR_STATUS_IN ConnStatus,
    _Inout_ PPPM_COMMAND_ACK_PARAMS CommandAckParams
);

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
Ucm_ReportDataDirectionChanged(
    _In_ PPPM_CONTEXT PpmCtx,
    _In_ PPPM_CONNECTOR Connector,
    _Inout_ PUCSI_GET_CONNECTOR_STATUS_IN ConnStatus,
    _Inout_ PPPM_COMMAND_ACK_PARAMS CommandAckParams
);

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
Ucm_ReportChargingStatusChanged(
    _In_ PPPM_CONTEXT PpmCtx,
    _In_ PPPM_CONNECTOR Connector,
    _Inout_ PUCSI_GET_CONNECTOR_STATUS_IN ConnStatus,
    _Inout_ PPPM_COMMAND_ACK_PARAMS CommandAckParams
);

EXTERN_C_END