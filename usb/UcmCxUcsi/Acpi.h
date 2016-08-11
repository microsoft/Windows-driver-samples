/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    Acpi.h

Abstract:

    ACPI method evaluation helper routines.

Environment:

    Kernel-mode only.

--*/

#pragma once

EXTERN_C_START

typedef struct _ACPI_CONTEXT
{
    BOOLEAN Initialized;
    ACPI_INTERFACE_STANDARD2 AcpiInterface;
    BOOLEAN RegisteredForNotifications;
} ACPI_CONTEXT, *PACPI_CONTEXT;


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Acpi_PrepareHardware (
    _In_ PACPI_CONTEXT AcpiCtx
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
Acpi_ReleaseHardware (
    _In_ PACPI_CONTEXT AcpiCtx
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Acpi_UcsiDsmSendData (
    _In_ PACPI_CONTEXT AcpiCtx
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Acpi_UcsiDsmReceiveData (
    _In_ PACPI_CONTEXT AcpiCtx
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Acpi_UcsiDsmIsUsbDeviceControllerEnabled (
    _In_ PACPI_CONTEXT AcpiCtx,
    _Out_ PBOOLEAN IsUsbDeviceControllerEnabled
    );

typedef
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
EVT_ACPI_NOTIFY_CALLBACK (
    _In_ PVOID NotificationContext,
    _In_ ULONG NotifyCode
    );

typedef EVT_ACPI_NOTIFY_CALLBACK *PFN_ACPI_NOTIFY_CALLBACK;

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
Acpi_RegisterNotificationCallback (
    _In_ PACPI_CONTEXT AcpiCtx,
    _In_ PFN_ACPI_NOTIFY_CALLBACK Callback,
    _In_ PVOID Context
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
Acpi_UnregisterNotificationCallback (
    _In_ PACPI_CONTEXT AcpiCtx
    );

EVT_ACPI_NOTIFY_CALLBACK OpmAcpiNotifyCallback;

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Acpi_EnumChildren (
    _In_ PACPI_CONTEXT AcpiCtx,
    _Out_ WDFMEMORY* EnumChildrenOutput
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Acpi_EvaluatePld (
    _In_ PACPI_CONTEXT AcpiCtx,
    _In_ LPCSTR DeviceName,
    _Out_ PACPI_PLD_BUFFER PldBuffer
    );

EXTERN_C_END
