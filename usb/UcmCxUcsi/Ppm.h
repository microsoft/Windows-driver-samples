/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    Ppm.h

Abstract:

    Type-C Platform Policy Manager. Main interface to talk to the hardware.

Environment:

    Kernel-mode only.

--*/

#pragma once

#define MAKEWORD(a, b) ((WORD)(((BYTE)(((DWORD_PTR)(a)) & 0xff)) | ((WORD)((BYTE)(((DWORD_PTR)(b)) & 0xff))) << 8))

#define UCSI_EXPECTED_NOTIFY_CODE 0x80

#define FILE_DEVICE_PPM 32769

#define IOCTL_INTERNAL_UCSI_SEND_COMMAND \
    (DWORD) CTL_CODE(FILE_DEVICE_PPM, \
                     0x900, \
                     METHOD_BUFFERED, \
                     FILE_ANY_ACCESS)

EXTERN_C_START

typedef struct _PPM_COMMAND_ACK_PARAMS
{
    BOOLEAN AckConnectorChange;
} PPM_COMMAND_ACK_PARAMS, *PPPM_COMMAND_ACK_PARAMS;

typedef
_Function_class_(EVT_PPM_COMMAND_COMPLETION_ROUTINE)
_IRQL_requires_max_(PASSIVE_LEVEL)
_IRQL_requires_same_
VOID
EVT_PPM_COMMAND_COMPLETION_ROUTINE (
    _In_ UCSI_CONTROL Command,
    _In_ PVOID Context,
    _Inout_ PPPM_COMMAND_ACK_PARAMS CommandAckParams
    );

typedef EVT_PPM_COMMAND_COMPLETION_ROUTINE *PFN_PPM_COMMAND_COMPLETION_ROUTINE;

typedef ULONG CONNECTOR_INDEX;

typedef struct _PPM_ACTIVE_COMMAND_CONTEXT
{
    WDFREQUEST Request;
    NTSTATUS Status;
    UCSI_CONTROL Command;
    PFN_PPM_COMMAND_COMPLETION_ROUTINE CompletionRoutine;
    PVOID CompletionContext;
    BOOLEAN CompletionRoutineInvoked;
    LONG CompletionAcked;
} PPM_ACTIVE_COMMAND_CONTEXT, *PPPM_ACTIVE_COMMAND_CONTEXT;

typedef struct _PPM_CONNECTOR_CHANGE_CONTEXT
{
    LONG InProgress;
    UCM_CHARGING_STATE ChargingState;
    UCM_PD_REQUEST_DATA_OBJECT Rdo;
    UCM_PD_POWER_DATA_OBJECT Pdos[UCSI_MAX_NUM_PDOS];
    UCHAR PdoCount;
} PPM_CONNECTOR_CHANGE_CONTEXT, *PPPM_CONNECTOR_CHANGE_CONTEXT;

typedef struct _PPM_CONTEXT
{
    PUCSI_DATA_BLOCK UcsiDataBlock;
    ULONG MappedMemoryLength;
    BOOLEAN IsUsbDeviceControllerEnabled;
    WDFIOTARGET SelfIoTarget;
    WDFQUEUE CommandQueue;
    WDFWORKITEM CommandCompletionWorkItem;
    BOOLEAN CommandCompleteNotificationEnabled;
    WDFCOLLECTION Connectors;

    //
    // State associated with the current command.
    //
    PPM_ACTIVE_COMMAND_CONTEXT ActiveCommandCtx;

    //
    // Temporary state associated with a reported connector
    // change. Since multiple commands may need to be sent,
    // all the data is temporarily staged in here, before reporting
    // it all to the Cx.
    //
    PPM_CONNECTOR_CHANGE_CONTEXT ConnectorChangeCtx;
} PPM_CONTEXT, *PPPM_CONTEXT;

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Ppm_Initialize(
    _In_ PPPM_CONTEXT PpmCtx
);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Ppm_PrepareHardware (
    _In_ PPPM_CONTEXT PpmCtx,
    _In_ PHYSICAL_ADDRESS MemoryAddress,
    _In_ ULONG MemoryLength
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
Ppm_ReleaseHardware (
    _In_ PPPM_CONTEXT PpmCtx
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
Ppm_SendCommand (
    _In_ PPPM_CONTEXT PpmCtx,
    _In_ UCSI_CONTROL Command,
    _In_opt_ PFN_PPM_COMMAND_COMPLETION_ROUTINE CompletionRoutine,
    _In_opt_ PVOID Context
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Ppm_SendCommandSynchronously (
    _In_ PPPM_CONTEXT PpmCtx,
    _In_ UCSI_CONTROL Command,
    _Out_opt_ PUCSI_MESSAGE_IN MessageIn,
    _Out_opt_ PULONG_PTR BytesReturned
    );

_IRQL_requires_max_(HIGH_LEVEL)
NTSTATUS
Ppm_GetCci (
    _In_ PPPM_CONTEXT PpmCtx,
    _Out_ PUCSI_CCI UcsiCci
    );

_IRQL_requires_max_(HIGH_LEVEL)
NTSTATUS
Ppm_GetMessage (
    _In_ PPPM_CONTEXT PpmCtx,
    _Out_ UINT8 (&Message)[UCSI_MAX_DATA_LENGTH]
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Ppm_PowerOn (
    _In_ PPPM_CONTEXT PpmCtx
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Ppm_EnableNotifications (
    _In_ PPPM_CONTEXT PpmCtx
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Ppm_PowerOff (
    _In_ PPPM_CONTEXT PpmCtx
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Ppm_ConnectorSetUor (
    _In_ PPPM_CONTEXT PpmCtx,
    _In_ CONNECTOR_INDEX ConnectorIndex,
    _In_ UCSI_USB_OPERATION_ROLE Uor
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Ppm_ConnectorSetPdr (
    _In_ PPPM_CONTEXT PpmCtx,
    _In_ CONNECTOR_INDEX ConnectorIndex,
    _In_ UCSI_POWER_DIRECTION_ROLE Pdr
    );

EVT_ACPI_NOTIFY_CALLBACK Ppm_NotificationHandler;

_IRQL_requires_max_(HIGH_LEVEL)
ULONG64
FORCEINLINE
UCM_CONNECTOR_ID_FROM_ACPI_PLD(
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


typedef struct _PPM_SEND_COMMAND_PARAMS
{
    UCSI_CONTROL Command;
    PFN_PPM_COMMAND_COMPLETION_ROUTINE CompletionRoutine;
    PVOID Context;
} PPM_SEND_COMMAND_PARAMS, *PPPM_SEND_COMMAND_PARAMS;

typedef struct _PPM_REQUEST_CONTEXT
{
    UCSI_CONTROL Command;
} PPM_REQUEST_CONTEXT, *PPPM_REQUEST_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(PPM_REQUEST_CONTEXT, PpmRequest_GetContext);

typedef struct _PPM_CONNECTOR
{
    WDFDEVICE WdfDevice;
    ULONGLONG Id;
    CONNECTOR_INDEX Index;
    UCMCONNECTOR Handle;
    BOOLEAN PerformRoleCorrectionOnNextPdContract;
} PPM_CONNECTOR, *PPPM_CONNECTOR;

typedef struct _PPM_UCM_CONNECTOR_CONTEXT
{
    PPPM_CONNECTOR PpmConnector;
} PPM_UCM_CONNECTOR_CONTEXT, *PPPM_UCM_CONNECTOR_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(PPM_UCM_CONNECTOR_CONTEXT, PpmUcmConnector_GetContext);

EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL Ppm_EvtIoInternalDeviceControl;
EVT_WDF_REQUEST_COMPLETION_ROUTINE Ppm_CommandRequestCompletionRoutine;
EVT_WDF_WORKITEM Ppm_CommandCompletionWorkItem;
EVT_PPM_COMMAND_COMPLETION_ROUTINE Ppm_EvtGetConnectorStatusCompleted;

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Ppm_WaitForResetComplete(
    _In_ PPPM_CONTEXT PpmCtx
);

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
Ppm_ProcessNotifications(
    _In_ PPPM_CONTEXT PpmCtx
);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Ppm_ExecuteCommand(
    _In_ PPPM_CONTEXT PpmCtx,
    _In_ UCSI_CONTROL Command
);

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
Ppm_CompleteActiveRequest(
    _In_ PPPM_CONTEXT PpmCtx
);

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
Ppm_SaveMessageInContentsInRequest(
    _In_ PPPM_CONTEXT PpmCtx
);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Ppm_QueryConnectors(
    _In_ PPPM_CONTEXT PpmCtx
);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Ppm_CommandCompletionHandler(
    _In_ PPPM_CONTEXT PpmCtx
);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Ppm_GetCapability(
    _In_ PPPM_CONTEXT PpmCtx,
    _Out_ PUCSI_GET_CAPABILITY_IN Caps
);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Ppm_GetConnectorCapability(
    _In_ PPPM_CONTEXT PpmCtx,
    _In_ CONNECTOR_INDEX ConnectorIndex,
    _Out_ PUCSI_GET_CONNECTOR_CAPABILITY_IN ConnCaps
);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
_Success_(return != 0)
PPPM_CONNECTOR
Ppm_AddConnector(
    _In_ PPPM_CONTEXT PpmCtx
);

_IRQL_requires_max_(DISPATCH_LEVEL)
PPPM_CONNECTOR
Ppm_GetConnector(
    _In_ PPPM_CONTEXT PpmCtx,
    _In_ CONNECTOR_INDEX Index
);

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
Ppm_HandleCommandCompletionNotification(
    _In_ PPPM_CONTEXT PpmCtx,
    _In_ UCSI_CCI Cci
);

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
Ppm_HandleConnectorChangeNotification(
    _In_ PPPM_CONTEXT PpmCtx,
    _In_ UCSI_CCI Cci
);

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
Ppm_ReportNegotiatedPowerLevelChanged(
    _In_ PPPM_CONTEXT PpmCtx,
    _In_ PPPM_CONNECTOR Connector,
    _Inout_ PUCSI_GET_CONNECTOR_STATUS_IN ConnStatus,
    _Inout_ PPPM_COMMAND_ACK_PARAMS CommandAckParams
);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Ppm_PerformRoleCorrection(
    _In_ PPPM_CONTEXT PpmCtx,
    _In_ PPPM_CONNECTOR Connector
);

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
Ppm_PrettyDebugPrintMessageIn(
    _In_ PPPM_CONTEXT PpmCtx
);


EXTERN_C_END
