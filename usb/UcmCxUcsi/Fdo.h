/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    Fdo.h

Abstract:

    FDO callbacks, functions, and types.

Environment:

    Kernel-mode only.

--*/

#pragma once

EXTERN_C_START

typedef struct _FDO_CONTEXT
{
    WDFDEVICE WdfDevice;

    ACPI_CONTEXT AcpiCtx;
    PPM_CONTEXT PpmCtx;
    WDFWORKITEM ConnectorAndNotificationWorkItem;

} FDO_CONTEXT, *PFDO_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FDO_CONTEXT, Fdo_GetContext)

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Fdo_Create (
    _Inout_ PWDFDEVICE_INIT DeviceInit
    );

EXTERN_C_END

PFDO_CONTEXT
FORCEINLINE
Context_GetFdoContext (
    _In_ PACPI_CONTEXT AcpiCtx
    )
{
    return CONTAINING_RECORD(AcpiCtx, FDO_CONTEXT, AcpiCtx);
}

PFDO_CONTEXT
FORCEINLINE
Context_GetFdoContext (
    _In_ PPPM_CONTEXT PpmCtx
    )
{
    return CONTAINING_RECORD(PpmCtx, FDO_CONTEXT, PpmCtx);
}

template<typename ContextType>
WDFDEVICE
FORCEINLINE
Context_GetWdfDevice (
    _In_ ContextType Ctx
    )
{
    return Context_GetFdoContext(Ctx)->WdfDevice;
}
