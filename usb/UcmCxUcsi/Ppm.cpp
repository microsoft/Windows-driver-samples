/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    Ppm.cpp

Abstract:

    Type-C Platform Policy Manager. Main interface to talk to the hardware.

Environment:

    Kernel-mode only.

--*/

#include "Pch.h"
#include "Ppm.tmh"

#pragma alloc_text(PAGE, Ppm_Initialize)
#pragma alloc_text(PAGE, Ppm_PrepareHardware)
#pragma alloc_text(PAGE, Ppm_ReleaseHardware)
#pragma alloc_text(PAGE, Ppm_SendCommandSynchronously)
#pragma alloc_text(PAGE, Ppm_PowerOn)
#pragma alloc_text(PAGE, Ppm_PowerOff)
#pragma alloc_text(PAGE, Ppm_WaitForResetComplete)
#pragma alloc_text(PAGE, Ppm_EvtIoInternalDeviceControl)
#pragma alloc_text(PAGE, Ppm_ExecuteCommand)
#pragma alloc_text(PAGE, Ppm_CommandCompletionWorkItem)
#pragma alloc_text(PAGE, Ppm_QueryConnectors)
#pragma alloc_text(PAGE, Ppm_CommandCompletionHandler)
#pragma alloc_text(PAGE, Ppm_GetCapability)
#pragma alloc_text(PAGE, Ppm_GetConnectorCapability)
#pragma alloc_text(PAGE, Ppm_AddConnector)
#pragma alloc_text(PAGE, Ppm_EvtGetConnectorStatusCompleted)
#pragma alloc_text(PAGE, Ppm_EnableNotifications)
#pragma alloc_text(PAGE, Ppm_ReportNegotiatedPowerLevelChanged)
#pragma alloc_text(PAGE, Ppm_ConnectorSetUor)
#pragma alloc_text(PAGE, Ppm_ConnectorSetPdr)
#pragma alloc_text(PAGE, Ppm_PerformRoleCorrection)


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Ppm_Initialize (
    _In_ PPPM_CONTEXT PpmCtx
)
{
    NTSTATUS status;
    WDFDEVICE device;
    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_IO_QUEUE_CONFIG queueConfig;
    WDF_IO_TARGET_OPEN_PARAMS openParams;
    WDF_WORKITEM_CONFIG workItemConfig;
    UCM_MANAGER_CONFIG ucmConfig;

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_PPM);

    device = Context_GetWdfDevice(PpmCtx);

    //
    // UCSI can process only one command at a time, hence this queue needs to be
    // sequential.
    //

    WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, WdfIoQueueDispatchSequential);

    queueConfig.EvtIoInternalDeviceControl = Ppm_EvtIoInternalDeviceControl;

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

    //
    // It is more convenient to do command processing at PASSIVE_LEVEL.
    //

    attributes.ExecutionLevel = WdfExecutionLevelPassive;

    status = WdfIoQueueCreate(device, &queueConfig, &attributes, &PpmCtx->CommandQueue);
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_PPM, "[Device: 0x%p] WdfIoQueueCreate failed - %!STATUS!", device, status);
        goto Exit;
    }

    //
    // Steal all IRP_MJ_INTERNAL_DEVICE_CONTROL because we expect the only code we receive to
    // be IOCTL_INTERNAL_UCSI_SEND_COMMAND. If we ever need to support more codes and some other
    // component needs to handle them, we need to configure an IRP preprocess routine, and redirect
    // the requests accordingly.
    //

    status = WdfDeviceConfigureRequestDispatching(device,
        PpmCtx->CommandQueue,
        WdfRequestTypeDeviceControlInternal);
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_PPM, "[Device: 0x%p] WdfDeviceConfigureRequestDispatching failed - %!STATUS!", device, status);
        goto Exit;
    }

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = device;

    status = WdfIoTargetCreate(device, &attributes, &PpmCtx->SelfIoTarget);
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_PPM, "[Device: 0x%p] WdfIoTargetCreate failed - %!STATUS!", device, status);
        goto Exit;
    }

    WDF_IO_TARGET_OPEN_PARAMS_INIT_EXISTING_DEVICE(&openParams, WdfDeviceWdmGetDeviceObject(device));

    status = WdfIoTargetOpen(PpmCtx->SelfIoTarget, &openParams);
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_PPM, "[Device: 0x%p] WdfIoTargetOpen failed - %!STATUS!", device, status);
        goto Exit;
    }

    WDF_WORKITEM_CONFIG_INIT(&workItemConfig, Ppm_CommandCompletionWorkItem);

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = device;

    status = WdfWorkItemCreate(&workItemConfig, &attributes, &PpmCtx->CommandCompletionWorkItem);
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_PPM, "[Device: 0x%p] WdfWorkItemCreate failed - %!STATUS!", device, status);
        goto Exit;
    }

    UCM_MANAGER_CONFIG_INIT(&ucmConfig);
    status = UcmInitializeDevice(device, &ucmConfig);
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_PPM, "[Device: 0x%p] UcmInitializeDevice failed - %!STATUS!", device, status);
        goto Exit;
    }

    TRACE_INFO(TRACE_FLAG_PPM, "[Device: 0x%p] PPM initialized", device);

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_PPM);

    return status;
}


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Ppm_PrepareHardware (
    _In_ PPPM_CONTEXT PpmCtx,
    _In_ PHYSICAL_ADDRESS MemoryAddress,
    _In_ ULONG MemoryLength
    )
{
    NTSTATUS status;
    WDFDEVICE device;
    PVOID mappedMemory;
    PACPI_CONTEXT acpiCtx;
    UCSI_VERSION version;

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_PPM);

    device = Context_GetWdfDevice(PpmCtx);

    mappedMemory = MmMapIoSpaceEx(MemoryAddress,
                                  MemoryLength,
                                  PAGE_NOCACHE | PAGE_READWRITE);

#pragma prefast(suppress: __WARNING_REDUNDANT_POINTER_TEST, "API indicates failure by returning NULL")
    if (mappedMemory == NULL)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        TRACE_ERROR(TRACE_FLAG_PPM, "[Device: 0x%p] MmMapIoSpaceEx failed", device);
        goto Exit;
    }

    PpmCtx->UcsiDataBlock = (PUCSI_DATA_BLOCK) mappedMemory;
    PpmCtx->MappedMemoryLength = MemoryLength;

    version = PpmCtx->UcsiDataBlock->UcsiVersion;
    TRACE_INFO(TRACE_FLAG_PPM, "[Device: 0x%p] UCSI version Major: %hx Minor: %hx SubMinor: %hx", device, version.MajorVersion, version.MinorVersion, version.SubMinorVersion);

    status = Ppm_QueryConnectors(PpmCtx);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    acpiCtx = &Context_GetFdoContext(PpmCtx)->AcpiCtx;
    status = Acpi_UcsiDsmIsUsbDeviceControllerEnabled(acpiCtx,
                                                      &PpmCtx->IsUsbDeviceControllerEnabled);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    if (PpmCtx->IsUsbDeviceControllerEnabled)
    {
        TRACE_INFO(TRACE_FLAG_PPM, "[Device: 0x%p] USB Device Controller is enabled", device);
    }
    else
    {
        TRACE_INFO(TRACE_FLAG_PPM, "[Device: 0x%p] USB Device Controller is disabled", device);
    }

    TRACE_INFO(TRACE_FLAG_PPM, "[Device: 0x%p] PPM prepare hardware completed", device);

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_PPM);

    return status;
}


_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
Ppm_ReleaseHardware (
    _In_ PPPM_CONTEXT PpmCtx
    )
{
    WDFDEVICE device;

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_PPM);

    if (PpmCtx->UcsiDataBlock == nullptr)
    {
        goto Exit;
    }

    device = Context_GetWdfDevice(PpmCtx);

    MmUnmapIoSpace(PpmCtx->UcsiDataBlock, PpmCtx->MappedMemoryLength);
    PpmCtx->UcsiDataBlock = nullptr;
    PpmCtx->MappedMemoryLength = 0;

    if (PpmCtx->Connectors != WDF_NO_HANDLE)
    {
        WdfObjectDelete(PpmCtx->Connectors);
        PpmCtx->Connectors = WDF_NO_HANDLE;
    }

    TRACE_INFO(TRACE_FLAG_PPM, "[Device: 0x%p] PPM release hardware completed", device);

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_PPM);
}


VOID
Ppm_CommandRequestCompletionRoutine (
    _In_ WDFREQUEST Request,
    _In_ WDFIOTARGET Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS Params,
    _In_ WDFCONTEXT Context
    )
{
    PPPM_CONTEXT ppmCtx;
    WDFDEVICE device;
    NTSTATUS status;
    PPPM_REQUEST_CONTEXT reqCtx;
    UCSI_CONTROL command;

    UNREFERENCED_PARAMETER(Target);

    TRACE_FUNC_ENTRY(TRACE_FLAG_PPM);

    ppmCtx = (PPPM_CONTEXT) Context;
    device = Context_GetWdfDevice(ppmCtx);
    reqCtx = PpmRequest_GetContext(Request);
    command = reqCtx->Command;

    status = Params->IoStatus.Status;

    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_PPM, "[Device: 0x%p] PPM asynchronous command 0x%016I64x (%!UCSI_COMMAND!) failed - %!STATUS!", device, command.AsUInt64, command.Command, status);
    }
    else
    {
        TRACE_INFO(TRACE_FLAG_PPM, "[Device: 0x%p] PPM asynchronous command 0x%016I64x (%!UCSI_COMMAND!) completed", device, command.AsUInt64, command.Command);
    }

    WdfObjectDelete(Request);

    TRACE_FUNC_EXIT(TRACE_FLAG_PPM);
}


_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
Ppm_SendCommand (
    _In_ PPPM_CONTEXT PpmCtx,
    _In_ UCSI_CONTROL Command,
    _In_opt_ PFN_PPM_COMMAND_COMPLETION_ROUTINE CompletionRoutine,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    WDFDEVICE device;
    WDFREQUEST request;
    WDF_OBJECT_ATTRIBUTES attributes;
    WDFMEMORY inputMemory;
    PPPM_SEND_COMMAND_PARAMS sendCommandParams;
    WDFMEMORY outputMemory;
    PPPM_REQUEST_CONTEXT reqCtx;
    BOOLEAN sent;

    TRACE_FUNC_ENTRY(TRACE_FLAG_PPM);

    device = Context_GetWdfDevice(PpmCtx);
    request = WDF_NO_HANDLE;

    TRACE_INFO(TRACE_FLAG_PPM, "[Device: 0x%p] Sending command asynchronously - 0x%016I64x (%!UCSI_COMMAND!)", device, Command.AsUInt64, Command.Command);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, PPM_REQUEST_CONTEXT);
    attributes.ParentObject = device;

    status = WdfRequestCreate(&attributes, PpmCtx->SelfIoTarget, &request);
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_PPM, "[Device: 0x%p] WdfRequestCreate failed - %!STATUS!", device, status);
        goto Exit;
    }

    reqCtx = PpmRequest_GetContext(request);
    reqCtx->Command = Command;

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = request;

    status = WdfMemoryCreate(&attributes,
                             NonPagedPoolNx,
                             0,
                             sizeof(*sendCommandParams),
                             &inputMemory,
                             (PVOID*) &sendCommandParams);
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_PPM, "[Device: 0x%p] WdfMemoryCreate for input memory failed - %!STATUS!", device, status);
        goto Exit;
    }

    RtlZeroMemory(sendCommandParams, sizeof(*sendCommandParams));
    sendCommandParams->Command = Command;
    sendCommandParams->CompletionRoutine = CompletionRoutine;
    sendCommandParams->Context = Context;

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = request;

    status = WdfMemoryCreate(&attributes, NonPagedPoolNx, 0, sizeof(UCSI_MESSAGE_IN), &outputMemory, nullptr);
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_PPM, "[Device: 0x%p] WdfMemoryCreate for output memory failed - %!STATUS!", device, status);
        goto Exit;
    }

    status = WdfIoTargetFormatRequestForInternalIoctl(PpmCtx->SelfIoTarget,
                                                      request,
                                                      IOCTL_INTERNAL_UCSI_SEND_COMMAND,
                                                      inputMemory,
                                                      nullptr,
                                                      outputMemory,
                                                      nullptr);
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_PPM, "[Device: 0x%p] WdfIoTargetFormatRequestForInternalIoctl failed - %!STATUS!", device, status);
        goto Exit;
    }

    WdfRequestSetCompletionRoutine(request, Ppm_CommandRequestCompletionRoutine, PpmCtx);

    sent = WdfRequestSend(request, PpmCtx->SelfIoTarget, nullptr);
    if (sent == FALSE)
    {
        status = WdfRequestGetStatus(request);
        TRACE_ERROR(TRACE_FLAG_PPM, "[Device: 0x%p] PPM command 0x%016I64x dispatch failed - %!STATUS!", device, Command.AsUInt64, status);
        goto Exit;
    }

Exit:

    if (!NT_SUCCESS(status) && (request != WDF_NO_HANDLE))
    {
        WdfObjectDelete(request);
    }

    TRACE_FUNC_EXIT(TRACE_FLAG_PPM);

    return status;
}


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Ppm_SendCommandSynchronously (
    _In_ PPPM_CONTEXT PpmCtx,
    _In_ UCSI_CONTROL Command,
    _Out_opt_ PUCSI_MESSAGE_IN MessageIn,
    _Out_opt_ PULONG_PTR BytesReturned
    )
{
    NTSTATUS status;
    WDFDEVICE device;
    PPM_SEND_COMMAND_PARAMS sendCommandParams;
    WDF_MEMORY_DESCRIPTOR inputMem;
    WDF_MEMORY_DESCRIPTOR outputMem;
    PWDF_MEMORY_DESCRIPTOR outputMemPtr;

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_PPM);

    device = Context_GetWdfDevice(PpmCtx);

    TRACE_INFO(TRACE_FLAG_PPM, "[Device: 0x%p] Sending command synchronously - 0x%016I64x (%!UCSI_COMMAND!)", device, Command.AsUInt64, Command.Command);

    RtlZeroMemory(&sendCommandParams, sizeof(sendCommandParams));
    sendCommandParams.Command = Command;

    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&inputMem, &sendCommandParams, sizeof(sendCommandParams));

    if (MessageIn == nullptr)
    {
        outputMemPtr = nullptr;
    }
    else
    {
#pragma prefast(suppress:__WARNING_USING_UNINIT_VAR, "This is the out buffer")
        WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&outputMem, MessageIn, sizeof(*MessageIn));
        outputMemPtr = &outputMem;
    }

    status = WdfIoTargetSendInternalIoctlSynchronously(PpmCtx->SelfIoTarget,
                                                       NULL,
                                                       IOCTL_INTERNAL_UCSI_SEND_COMMAND,
                                                       &inputMem,
                                                       outputMemPtr,
                                                       nullptr,
                                                       BytesReturned);

    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_PPM, "[Device: 0x%p] PPM synchronous command 0x%016I64x (%!UCSI_COMMAND!) failed - %!STATUS!", device, Command.AsUInt64, Command.Command, status);
        goto Exit;
    }

    TRACE_INFO(TRACE_FLAG_PPM, "[Device: 0x%p] PPM synchronous command 0x%016I64x (%!UCSI_COMMAND!) completed", device, Command.AsUInt64, Command.Command);

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_PPM);

    return status;
}


_IRQL_requires_max_(HIGH_LEVEL)
NTSTATUS
Ppm_GetCci (
    _In_ PPPM_CONTEXT PpmCtx,
    _Out_ PUCSI_CCI UcsiCci
    )
{
    PUCSI_DATA_BLOCK ucsiDataBlock;

    TRACE_FUNC_ENTRY(TRACE_FLAG_PPM);

    ucsiDataBlock = PpmCtx->UcsiDataBlock;

    *UcsiCci = ucsiDataBlock->CCI;

    TRACE_FUNC_EXIT(TRACE_FLAG_PPM);

    return STATUS_SUCCESS;
}


_IRQL_requires_max_(HIGH_LEVEL)
NTSTATUS
Ppm_GetMessage (
    _In_ PPPM_CONTEXT PpmCtx,
    _Out_ UINT8 (&Message)[UCSI_MAX_DATA_LENGTH]
    )
{
    PUCSI_DATA_BLOCK ucsiDataBlock;

    TRACE_FUNC_ENTRY(TRACE_FLAG_PPM);

    ucsiDataBlock = PpmCtx->UcsiDataBlock;

    RtlCopyMemory(Message, ucsiDataBlock->MessageIn.AsBuffer, sizeof(Message));

    TRACE_FUNC_EXIT(TRACE_FLAG_PPM);

    return STATUS_SUCCESS;
}


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Ppm_PowerOn (
    _In_ PPPM_CONTEXT PpmCtx
    )
{
    NTSTATUS status;
    PACPI_CONTEXT acpiCtx;
    WDFDEVICE device;
    UCSI_CONTROL command;

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_PPM);

    device = Context_GetWdfDevice(PpmCtx);
    acpiCtx = &Context_GetFdoContext(PpmCtx)->AcpiCtx;
    status = Acpi_RegisterNotificationCallback(acpiCtx, Ppm_NotificationHandler, PpmCtx);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    //
    // Execute a PPM_RESET, bypassing the command queue. We can do that because completion of
    // PPM_RESET is not indicated using a notification (we need to poll for completion), so
    // we don't need to use all the command completion handling logic.
    //
    // Ppm_ExecuteCommand will poll for completion of the PPM_RESET.
    //

    command.AsUInt64 = 0;
    command.Command = UcsiCommandPpmReset;
    status = Ppm_ExecuteCommand(PpmCtx, command);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    //
    // Queue a request to enable command completion notifications.
    //
    // N.B. We only enable command completion notifications here. All other notifications should
    //      be enabled separately. This is because we need to query some information and create
    //      the connector objects before we can enable connector change notifications.
    //

    command.AsUInt64 = 0;
    command.Command = UcsiCommandSetNotificationEnable;
    command.SetNotificationEnable.CommandCompleteNotificationEnable = 1;
    status = Ppm_SendCommand(PpmCtx, command, nullptr, nullptr);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    TRACE_INFO(TRACE_FLAG_PPM, "[Device: 0x%p] PPM power-on complete", device);

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_PPM);

    return status;
}


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Ppm_EnableNotifications (
    _In_ PPPM_CONTEXT PpmCtx
    )
{
    NTSTATUS status;
    WDFDEVICE device;
    UCSI_CONTROL command;

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_PPM);

    device = Context_GetWdfDevice(PpmCtx);

    command.AsUInt64 = 0;
    command.Command = UcsiCommandSetNotificationEnable;
    command.SetNotificationEnable.NotificationEnable = 0xffff;
    status = Ppm_SendCommand(PpmCtx, command, nullptr, nullptr);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    TRACE_INFO(TRACE_FLAG_PPM, "[Device: 0x%p] PPM notifications enabled", device);

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_PPM);

    return status;
}


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Ppm_PowerOff (
    _In_ PPPM_CONTEXT PpmCtx
    )
{
    NTSTATUS status;
    PACPI_CONTEXT acpiCtx;
    WDFDEVICE device;
    UCSI_CONTROL command;

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_PPM);

    device = Context_GetWdfDevice(PpmCtx);

    command.AsUInt64 = 0;
    command.Command = UcsiCommandSetNotificationEnable;
    command.SetNotificationEnable.NotificationEnable = 0;
    status = Ppm_ExecuteCommand(PpmCtx, command);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    acpiCtx = &Context_GetFdoContext(PpmCtx)->AcpiCtx;
    Acpi_UnregisterNotificationCallback(acpiCtx);

    TRACE_INFO(TRACE_FLAG_PPM, "[Device: 0x%p] PPM power-off complete", device);

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_PPM);

    return status;
}


_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
Ppm_NotificationHandler (
    _In_ PVOID Context,
    _In_ ULONG NotifyValue
    )
{
    PPPM_CONTEXT ppmCtx;
    WDFDEVICE device;

    TRACE_FUNC_ENTRY(TRACE_FLAG_PPM);

    ppmCtx = (PPPM_CONTEXT) Context;
    device = Context_GetWdfDevice(ppmCtx);

    if (NotifyValue != UCSI_EXPECTED_NOTIFY_CODE)
    {
        TRACE_WARN(TRACE_FLAG_PPM, "[Device: 0x%p] Unexpected notify code %lu", device, NotifyValue);
        goto Exit;
    }

    Ppm_ProcessNotifications(ppmCtx);

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_PPM);
}


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Ppm_WaitForResetComplete (
    _In_ PPPM_CONTEXT PpmCtx
    )
{
    WDFDEVICE device;
    PACPI_CONTEXT acpiCtx;
    NTSTATUS status;
    ULONG retries;
    LARGE_INTEGER delay;
    const ULONG MAX_RETRIES = 20;
    const ULONG WAIT_IN_MS = 20;

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_PPM);

    device = Context_GetWdfDevice(PpmCtx);
    acpiCtx = &Context_GetFdoContext(PpmCtx)->AcpiCtx;

    delay.QuadPart = WDF_REL_TIMEOUT_IN_MS(WAIT_IN_MS);

    for (retries = 0; retries <= MAX_RETRIES; ++retries)
    {
        KeDelayExecutionThread(KernelMode, FALSE, &delay);

        status = Acpi_UcsiDsmReceiveData(acpiCtx);
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }

        if (PpmCtx->UcsiDataBlock->CCI.ResetCompletedIndicator)
        {
            TRACE_INFO(TRACE_FLAG_PPM, "[Device: 0x%p] Reset completed after %u retries", device, retries);
            break;
        }
    }

    if (retries > MAX_RETRIES)
    {
        status = STATUS_DEVICE_BUSY;
        TRACE_ERROR(TRACE_FLAG_PPM, "[Device: 0x%p] Reset timed-out", device);
        goto Exit;
    }

    status = STATUS_SUCCESS;

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_PPM);

    return status;
}


_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
Ppm_ProcessNotifications (
    _In_ PPPM_CONTEXT PpmCtx
    )
{
    WDFDEVICE device;
    UCSI_CCI cci;
    LONG oldValue;

    TRACE_FUNC_ENTRY(TRACE_FLAG_PPM);

    device = Context_GetWdfDevice(PpmCtx);
    cci = PpmCtx->UcsiDataBlock->CCI;

    TRACE_INFO(TRACE_FLAG_PPM, "[Device: 0x%p] PPM notification received. CCI: 0x%08x", device, cci.AsUInt32);

    if (cci.AcknowledgeCommandIndicator)
    {
        //
        // If we did send an acknowledgment for command completion earlier, this bit
        // indicates that we can now complete the request from the command queue. Otherwise
        // this is simply a stale bit from the last command completion.
        //

        oldValue = InterlockedExchange(&PpmCtx->ActiveCommandCtx.CompletionAcked, 0);

        if (oldValue)
        {
            TRACE_INFO(TRACE_FLAG_PPM, "[Device: 0x%p] Acknowledge command indicator", device);
            Ppm_CompleteActiveRequest(PpmCtx);
        }
    }

    //
    // Now check mutually exclusive cases.
    //

    if (cci.ConnectorChangeIndicator &&
        (InterlockedExchange(&PpmCtx->ConnectorChangeCtx.InProgress, 1) == 0))
    {
        //
        // A new connector change. Process it.
        //

        Ppm_HandleConnectorChangeNotification(PpmCtx, cci);
    }
    else if (cci.CommandCompletedIndicator)
    {
        Ppm_HandleCommandCompletionNotification(PpmCtx, cci);
    }
    else if (cci.BusyIndicator)
    {
        TRACE_INFO(TRACE_FLAG_PPM, "[Device: 0x%p] Busy indicator for command %!UCSI_COMMAND!", device, PpmCtx->ActiveCommandCtx.Command.Command);
    }

    TRACE_FUNC_EXIT(TRACE_FLAG_PPM);
}


_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
Ppm_HandleConnectorChangeNotification (
    _In_ PPPM_CONTEXT PpmCtx,
    _In_ UCSI_CCI Cci
    )
{
    WDFDEVICE device;
    UCSI_CONTROL command;

    TRACE_FUNC_ENTRY(TRACE_FLAG_PPM);

    device = Context_GetWdfDevice(PpmCtx);

    TRACE_INFO(TRACE_FLAG_PPM, "[Device: 0x%p] Connector change indicated for connector %u. Getting connector status", device, Cci.ConnectorChangeIndicator);

    command.AsUInt64 = 0;
    command.Command = UcsiCommandGetConnectorStatus;
    command.GetConnectorStatus.ConnectorNumber = Cci.ConnectorChangeIndicator;

    //
    // It may be that we had already sent a command down to the PPM (or are about to), and we
    // got this notification just before we did, or the PPM decided to send us a notification
    // first. Queue this command into the command queue so that everything is synchronized
    // nicely.

    (void) Ppm_SendCommand(PpmCtx, command, Ppm_EvtGetConnectorStatusCompleted, PpmCtx);

    TRACE_FUNC_EXIT(TRACE_FLAG_PPM);
}


_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
Ppm_HandleCommandCompletionNotification (
    _In_ PPPM_CONTEXT PpmCtx,
    _In_ UCSI_CCI Cci
    )
{
    UNREFERENCED_PARAMETER(Cci);

    TRACE_FUNC_ENTRY(TRACE_FLAG_PPM);

    //
    // Send a command to acknowledge the completion. Obviously we can't send the command to
    // the command queue because there is already an outstanding request (the one we got the
    // completion for), and that request will be completed only when we get the Acknowledge
    // Command Indicator. However, we can safely execute the command right now, because we
    // know the PPM is ready to accept an ACK_CC_CI command.
    //

    if (KeGetCurrentIrql() == PASSIVE_LEVEL)
    {
        (void) Ppm_CommandCompletionHandler(PpmCtx);
    }
    else
    {
        WdfWorkItemEnqueue(PpmCtx->CommandCompletionWorkItem);
    }

    TRACE_FUNC_EXIT(TRACE_FLAG_PPM);
}


VOID
Ppm_EvtIoInternalDeviceControl (
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
    )
{
    WDFDEVICE device;
    PPPM_CONTEXT ppmCtx;
    PPPM_ACTIVE_COMMAND_CONTEXT cmdCtx;
    NTSTATUS status;
    size_t expectedInputSize;
    PPPM_SEND_COMMAND_PARAMS sendCommandParams;
    BOOLEAN tookOwnership;

    UNREFERENCED_PARAMETER(OutputBufferLength);

    _IRQL_limited_to_(PASSIVE_LEVEL);
    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_PPM);

    device = WdfIoQueueGetDevice(Queue);
    ppmCtx = &Fdo_GetContext(device)->PpmCtx;
    cmdCtx = &ppmCtx->ActiveCommandCtx;
    tookOwnership = FALSE;

    if (IoControlCode != IOCTL_INTERNAL_UCSI_SEND_COMMAND)
    {
        status = STATUS_NOT_SUPPORTED;
        TRACE_ERROR(TRACE_FLAG_PPM, "[Device: 0x%p] PPM command processor received unknown IOCTL 0x%lu", device, IoControlCode);
        goto Exit;
    }

    expectedInputSize = sizeof(*sendCommandParams);

    if (InputBufferLength != expectedInputSize)
    {
        status = STATUS_INVALID_BUFFER_SIZE;
        TRACE_ERROR(TRACE_FLAG_PPM, "[Device: 0x%p] Invalid buffer size. Expected %Iu, got %Iu", device, expectedInputSize, InputBufferLength);
        goto Exit;
    }

    status = WdfRequestRetrieveInputBuffer(Request,
                                           expectedInputSize,
                                           (PVOID*) &sendCommandParams,
                                           nullptr);
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_PPM, "[Device: 0x%p] WdfRequestRetrieveInputBuffer failed - %!STATUS!", device, status);
        goto Exit;
    }

    NT_ASSERT(cmdCtx->Request == WDF_NO_HANDLE);
    cmdCtx->Request = Request;

    NT_ASSERT(cmdCtx->Command.AsUInt64 == 0);
    cmdCtx->Command = sendCommandParams->Command;

    NT_ASSERT(cmdCtx->CompletionRoutine == nullptr);
    cmdCtx->CompletionRoutine = sendCommandParams->CompletionRoutine;

    NT_ASSERT(cmdCtx->CompletionContext == nullptr);
    cmdCtx->CompletionContext = sendCommandParams->Context;

    NT_ASSERT(cmdCtx->CompletionAcked == 0);
    cmdCtx->CompletionAcked = 0;

    cmdCtx->Status = STATUS_PENDING;
    tookOwnership = TRUE;

    status = Ppm_ExecuteCommand(ppmCtx, cmdCtx->Command);
    if (!NT_SUCCESS(status))
    {
        cmdCtx->Status = status;
        Ppm_CompleteActiveRequest(ppmCtx);
        goto Exit;
    }

    //
    // If command completion notifications are enabled, we let the notification handler
    // complete the request. Otherwise, complete it here. Note that in the latter case, we
    // cannot return the MESSAGE_IN contents because we have no idea when it will be valid (or if
    // at all it will be).
    //

    if (ppmCtx->CommandCompleteNotificationEnabled == FALSE)
    {
        TRACE_INFO(TRACE_FLAG_PPM, "[Device: 0x%p] Command completion notification not enabled", device);
        cmdCtx->Status = STATUS_SUCCESS;
        Ppm_CompleteActiveRequest(ppmCtx);
    }

Exit:

    if (!NT_SUCCESS(status) && !tookOwnership)
    {
        RtlZeroMemory(cmdCtx, sizeof(*cmdCtx));
        WdfRequestComplete(Request, status);
    }

    TRACE_FUNC_EXIT(TRACE_FLAG_PPM);
}


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Ppm_ExecuteCommand (
    _In_ PPPM_CONTEXT PpmCtx,
    _In_ UCSI_CONTROL Command
    )
{
    WDFDEVICE device;
    NTSTATUS status;
    PUCSI_DATA_BLOCK ucsiDataBlock;
    PACPI_CONTEXT acpiCtx;

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_PPM);

    device = Context_GetWdfDevice(PpmCtx);

    TRACE_INFO(TRACE_FLAG_PPM, "[Device: 0x%p] Executing command - 0x%016I64x (%!UCSI_COMMAND!)", device, Command.AsUInt64, Command.Command);

    ucsiDataBlock = PpmCtx->UcsiDataBlock;
    ucsiDataBlock->Control = Command;

    acpiCtx = &Context_GetFdoContext(PpmCtx)->AcpiCtx;

    status = Acpi_UcsiDsmSendData(acpiCtx);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    //
    // See if this command would have updated the status of the command completion notification.
    // This will be used to decide when to complete this request and any future requests in the
    // command queue. Note that if this command enabled the notification, the notification handler
    // may have already run and completed the request.
    //

    if (Command.Command == UcsiCommandSetNotificationEnable)
    {
        PpmCtx->CommandCompleteNotificationEnabled =
            !!Command.SetNotificationEnable.CommandCompleteNotificationEnable;
    }
    else if (Command.Command == UcsiCommandPpmReset)
    {
        //
        // All notifications will be disabled on PPM_RESET.
        //

        PpmCtx->CommandCompleteNotificationEnabled = FALSE;

        status = Ppm_WaitForResetComplete(PpmCtx);
        if (!NT_SUCCESS(status))
        {
            TRACE_ERROR(TRACE_FLAG_PPM, "[Device: 0x%p] Wait for reset completion failed - %!STATUS!", device, status);
            goto Exit;
        }
    }

    TRACE_INFO(TRACE_FLAG_PPM, "[Device: 0x%p] PPM command execution completed", device);

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_PPM);

    return status;
}


VOID
Ppm_CommandCompletionWorkItem (
    _In_ WDFWORKITEM WorkItem
    )
{
    WDFDEVICE device;
    PPPM_CONTEXT ppmCtx;

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_PPM);

    device = (WDFDEVICE) WdfWorkItemGetParentObject(WorkItem);
    ppmCtx = &Fdo_GetContext(device)->PpmCtx;

    TRACE_INFO(TRACE_FLAG_PPM, "[Device: 0x%p] Handling command completion in a workitem", device);

    (void) Ppm_CommandCompletionHandler(ppmCtx);

    TRACE_FUNC_EXIT(TRACE_FLAG_PPM);
}


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Ppm_CommandCompletionHandler (
    _In_ PPPM_CONTEXT PpmCtx
    )
{
    NTSTATUS status;
    PPPM_ACTIVE_COMMAND_CONTEXT cmdCtx;
    UCSI_CONTROL ackCommand;
    PPM_COMMAND_ACK_PARAMS ackParams;
    WDFDEVICE device;
    UCSI_CCI cci;

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_PPM);

    device = Context_GetWdfDevice(PpmCtx);
    cmdCtx = &PpmCtx->ActiveCommandCtx;
    cci = PpmCtx->UcsiDataBlock->CCI;

    if (cci.ErrorIndicator)
    {
        TRACE_ERROR(TRACE_FLAG_PPM, "[Device: 0x%p] Error indicator. Active command: 0x%016I64x (%!UCSI_COMMAND!)", device, cmdCtx->Command.AsUInt64, cmdCtx->Command.Command);
        cmdCtx->Status = STATUS_UNSUCCESSFUL;
    }
    else if (cci.NotSupportedIndicator)
    {
        TRACE_ERROR(TRACE_FLAG_PPM, "[Device: 0x%p] Not supported indicator. Active command: 0x%016I64x (%!UCSI_COMMAND!)", device, cmdCtx->Command.AsUInt64, cmdCtx->Command.Command);
        cmdCtx->Status = STATUS_NOT_SUPPORTED;
    }
    else
    {
        TRACE_INFO(TRACE_FLAG_PPM, "[Device: 0x%p] Command completed indicator. Active command: 0x%016I64x (%!UCSI_COMMAND!)", device, cmdCtx->Command.AsUInt64, cmdCtx->Command.Command);
        Ppm_SaveMessageInContentsInRequest(PpmCtx);
        cmdCtx->Status = STATUS_SUCCESS;
    }

    ackCommand.AsUInt64 = 0;
    ackCommand.Command = UcsiCommandAckCcCi;
    ackCommand.AckCcCi.CommandCompletedAcknowledge = 1;

    if (cmdCtx->CompletionRoutine != nullptr)
    {
        RtlZeroMemory(&ackParams, sizeof(ackParams));
        cmdCtx->CompletionRoutine(cmdCtx->Command,
                                  cmdCtx->CompletionContext,
                                  &ackParams);

        if (ackParams.AckConnectorChange)
        {
            TRACE_INFO(TRACE_FLAG_PPM, "[Device: 0x%p] Acknowledging connector change", device);
            ackCommand.AckCcCi.ConnectorChangeAcknowledge = 1;
            PpmCtx->ConnectorChangeCtx.InProgress = 0;
        }
    }

    cmdCtx->CompletionAcked = 1;
    status = Ppm_ExecuteCommand(PpmCtx, ackCommand);

    TRACE_FUNC_EXIT(TRACE_FLAG_PPM);

    return status;
}


_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
Ppm_CompleteActiveRequest (
    _In_ PPPM_CONTEXT PpmCtx
    )
{
    WDFREQUEST request;
    WDFDEVICE device;
    UCSI_CONTROL command;
    NTSTATUS status;
    PPPM_ACTIVE_COMMAND_CONTEXT cmdCtx;

    TRACE_FUNC_ENTRY(TRACE_FLAG_PPM);

    cmdCtx = &PpmCtx->ActiveCommandCtx;
    device = Context_GetWdfDevice(PpmCtx);
    request = cmdCtx->Request;
    command = cmdCtx->Command;
    status = cmdCtx->Status;

    RtlZeroMemory(cmdCtx, sizeof(*cmdCtx));

    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_PPM, "[Device: 0x%p] Completing command 0x%016I64x (%!UCSI_COMMAND!) with %!STATUS!", device, command.AsUInt64, command.Command, status);
    }
    else
    {
        TRACE_INFO(TRACE_FLAG_PPM, "[Device: 0x%p] Completing command 0x%016I64x (%!UCSI_COMMAND!) with %!STATUS!", device, command.AsUInt64, command.Command, status);
    }

    WdfRequestComplete(request, status);

    TRACE_FUNC_EXIT(TRACE_FLAG_PPM);
}


_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
Ppm_SaveMessageInContentsInRequest (
    _In_ PPPM_CONTEXT PpmCtx
    )
{
    WDFREQUEST request;
    PUCSI_MESSAGE_IN messageIn;
    NTSTATUS status;
    ULONG_PTR information;
    size_t outputSize;

    TRACE_FUNC_ENTRY(TRACE_FLAG_PPM);

    request = PpmCtx->ActiveCommandCtx.Request;

    Ppm_PrettyDebugPrintMessageIn(PpmCtx);

    outputSize = PpmCtx->UcsiDataBlock->CCI.DataLength;
    status = WdfRequestRetrieveOutputBuffer(request, outputSize, (PVOID*) &messageIn, nullptr);
    if (NT_SUCCESS(status))
    {
        RtlCopyMemory(messageIn, PpmCtx->UcsiDataBlock->MessageIn.AsBuffer, outputSize);
        information = outputSize;
    }
    else
    {
        information = 0;
    }

    WdfRequestSetInformation(request, information);

    TRACE_FUNC_EXIT(TRACE_FLAG_PPM);
}


_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
Ppm_PrettyDebugPrintMessageIn (
    _In_ PPPM_CONTEXT PpmCtx
    )
{
    WDFDEVICE device;
    ULONG i;
    PUINT8 messageBuf;
    UCSI_CONTROL activeCommand;
    PUCSI_GET_CONNECTOR_STATUS_IN connStatus;
    size_t messageInSize;

    TRACE_FUNC_ENTRY(TRACE_FLAG_PPM);

    device = Context_GetWdfDevice(PpmCtx);
    activeCommand = PpmCtx->ActiveCommandCtx.Command;
    messageInSize = PpmCtx->UcsiDataBlock->CCI.DataLength;

    static_assert(sizeof(UCSI_MESSAGE_IN) == 16, "MESSAGE_IN size assumption out-of-sync");

    if (activeCommand.Command == UcsiCommandGetConnectorStatus)
    {
        connStatus = &PpmCtx->UcsiDataBlock->MessageIn.ConnectorStatus;

        TRACE_INFO(TRACE_FLAG_PPM, "[Device: 0x%p] Connector Status", device);

        TRACE_INFO(
            TRACE_FLAG_PPM,
            "[Device: 0x%p] "
            "%s %s %s %s %s %s %s %s %s %s %s",
            device,
            connStatus->ConnectorStatusChange.ExternalSupplyChange ? "ExternalSupplyChange" : " ",
            connStatus->ConnectorStatusChange.PowerOperationModeChange ? "PowerOperationModeChange" : " ",
            connStatus->ConnectorStatusChange.SupportedProviderCapabilitiesChange ? "SupportedProviderCapabilitiesChange" : " ",
            connStatus->ConnectorStatusChange.NegotiatedPowerLevelChange ? "NegotiatedPowerLevelChange" : " ",
            connStatus->ConnectorStatusChange.PdResetComplete ? "PdResetComplete" : " ",
            connStatus->ConnectorStatusChange.SupportedCamChange ? "SupportedCamChange" : " ",
            connStatus->ConnectorStatusChange.BatteryChargingStatusChange ? "BatteryChargingStatusChange" : " ",
            connStatus->ConnectorStatusChange.ConnectorPartnerChange ? "ConnectorPartnerChange" : " ",
            connStatus->ConnectorStatusChange.PowerDirectionChange ? "PowerDirectionChange" : " ",
            connStatus->ConnectorStatusChange.ConnectChange ? "ConnectChange" : " ",
            connStatus->ConnectorStatusChange.Error ? "Error" : " "
            );

        TRACE_INFO(
            TRACE_FLAG_PPM,
            "[Device: 0x%p] "
            "Connect: %d PartnerFlags: %d RDO: 0x%x",
            device,
            connStatus->ConnectStatus,
            connStatus->ConnectorPartnerFlags,
            connStatus->RequestDataObject
            );

        TRACE_INFO(
            TRACE_FLAG_PPM,
            "[Device: 0x%p] "
            "%!UCSI_POWER_OPERATION_MODE! "
            "%!UCSI_POWER_DIRECTION! "
            "%!UCSI_CONNECTOR_PARTNER_TYPE! "
            "%!UCSI_BATTERY_CHARGING_STATUS!",
            device,
            connStatus->PowerOperationMode,
            connStatus->PowerDirection,
            connStatus->ConnectorPartnerType,
            connStatus->BatteryChargingStatus
            );
    }
    else
    {
        for (i = 0; (i < 4) && messageInSize; ++i)
        {
            messageBuf = &PpmCtx->UcsiDataBlock->MessageIn.AsBuffer[i * 4];
            TRACE_INFO(TRACE_FLAG_PPM, "[Device: 0x%p] MESSAGE_IN[%d]: %02x %02x %02x %02x", device, i, messageBuf[0], messageBuf[1], messageBuf[2], messageBuf[3]);

            messageInSize -= min(messageInSize, 4);
        }
    }

    TRACE_FUNC_EXIT(TRACE_FLAG_PPM);
}


_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
Ppm_ReportNegotiatedPowerLevelChanged (
    _In_ PPPM_CONTEXT PpmCtx,
    _In_ PPPM_CONNECTOR Connector,
    _Inout_ PUCSI_GET_CONNECTOR_STATUS_IN ConnStatus,
    _Inout_ PPPM_COMMAND_ACK_PARAMS CommandAckParams
    )
{
    WDFDEVICE device;
    NTSTATUS status;
    UCSI_CONTROL cmd;

    UNREFERENCED_PARAMETER(CommandAckParams);

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_PPM);

    NT_ASSERT(ConnStatus->PowerOperationMode == UcsiPowerOperationModePd);

    device = Context_GetWdfDevice(PpmCtx);

    //
    // Save off stuff we will need for later.
    //

    PpmCtx->ConnectorChangeCtx.Rdo.Ul = ConnStatus->RequestDataObject;

    if (!Convert((UCSI_BATTERY_CHARGING_STATUS) ConnStatus->BatteryChargingStatus,
                 PpmCtx->ConnectorChangeCtx.ChargingState))
    {
        TRACE_ERROR(TRACE_FLAG_PPM, "[Device: 0x%p] Invalid battery charging state %u", device, ConnStatus->BatteryChargingStatus);
        goto Exit;
    }

    TRACE_INFO(TRACE_FLAG_PPM, "[Device: 0x%p] Retrieving PD contract details", device);

    cmd.AsUInt64 = 0;
    cmd.Command = UcsiCommandGetPdos;
    cmd.GetPdos.ConnectorNumber = Connector->Index;
    cmd.GetPdos.NumberOfPdos = 3;

    if (ConnStatus->PowerDirection == UcsiPowerDirectionConsumer)
    {
        cmd.GetPdos.PartnerPdo = 1;
    }
    else
    {
        cmd.GetPdos.PartnerPdo = 0;
    }

    cmd.GetPdos.SourceOrSinkPdos = UcsiGetPdosTypeSource;
    cmd.GetPdos.PdoOffset = 0;

    status = Ppm_SendCommand(PpmCtx, cmd, Ucm_EvtGetPdosCompleted, PpmCtx);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    //
    // Don't acknowledge the change just yet. Wait till we get the PD contract information
    // as well.
    //

    CommandAckParams->AckConnectorChange = FALSE;

    //
    // We will report the charging state when we report the PD contract details.
    //

    ConnStatus->ConnectorStatusChange.BatteryChargingStatusChange = 0;
    ConnStatus->ConnectorStatusChange.NegotiatedPowerLevelChange = 0;

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_PPM);
}


VOID
Ppm_EvtGetConnectorStatusCompleted (
    _In_ UCSI_CONTROL Command,
    _In_ PVOID Context,
    _Inout_ PPPM_COMMAND_ACK_PARAMS CommandAckParams
    )
{
    PPPM_CONTEXT ppmCtx;
    UCSI_GET_CONNECTOR_STATUS_IN connStatus;
    PPPM_CONNECTOR connector;

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_PPM);

    ppmCtx = (PPPM_CONTEXT) Context;

    NT_ASSERT(Command.Command == UcsiCommandGetConnectorStatus);

    //
    // Default disposition is to acknowledge the connector change. If we need to send more
    // commands to get more details on the change, then don't acknowledge; individual handler
    // routines will update the CommandAckParams accordingly.
    //

    CommandAckParams->AckConnectorChange = TRUE;

    if (!UCSI_CMD_SUCCEEDED(ppmCtx->UcsiDataBlock->CCI))
    {
        //
        // The command failed. Acknowledge the connector change and move on.
        //

        goto Exit;
    }

    //
    // Send all the appropriate notifications to UCM. In the process of handling each type of
    // change, other related bits in the connector status change may be cleared by the handler
    // because the associated information has already been reported.
    //
    // N.B. Even if we encounter errors, make sure to acknowledge the connector change. Else
    //      we will not get any more notifications.
    //

    connStatus = ppmCtx->UcsiDataBlock->MessageIn.ConnectorStatus;
    connector = Ppm_GetConnector(ppmCtx, Command.GetConnectorStatus.ConnectorNumber);

    if (connStatus.ConnectorStatusChange.ConnectChange)
    {
        if (connStatus.ConnectStatus)
        {
            Ucm_ReportTypeCAttach(ppmCtx, connector, &connStatus, CommandAckParams);

            //
            // If PD has already been negotiated, report that information as well.
            //

            if (connStatus.PowerOperationMode == UcsiPowerOperationModePd)
            {
                Ppm_ReportNegotiatedPowerLevelChanged(ppmCtx,
                                                      connector,
                                                      &connStatus,
                                                      CommandAckParams);
            }
        }
        else
        {
            Ucm_ReportTypeCDetach(ppmCtx, connector, &connStatus, CommandAckParams);

            //
            // If ConnectStatus is not set, nothing else is valid.
            //
            goto Exit;
        }
    }

    if (connStatus.ConnectorStatusChange.PowerOperationModeChange)
    {
        Ucm_ReportPowerOperationMode(ppmCtx, connector, &connStatus, CommandAckParams);
    }

    if(connStatus.ConnectorStatusChange.PowerDirectionChange)
    {
        Ucm_ReportPowerDirectionChanged(ppmCtx, connector, &connStatus, CommandAckParams);
    }

    if (connStatus.ConnectorStatusChange.ConnectorPartnerChange)
    {
        Ucm_ReportDataDirectionChanged(ppmCtx, connector, &connStatus, CommandAckParams);
    }

    if (connStatus.ConnectorStatusChange.BatteryChargingStatusChange)
    {
        Ucm_ReportChargingStatusChanged(ppmCtx, connector, &connStatus, CommandAckParams);
    }

    if (connStatus.ConnectorStatusChange.NegotiatedPowerLevelChange)
    {
        Ppm_ReportNegotiatedPowerLevelChanged(ppmCtx, connector, &connStatus, CommandAckParams);
    }

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_PPM);
}


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Ppm_QueryConnectors (
    _In_ PPPM_CONTEXT PpmCtx
    )
{
    NTSTATUS status;
    WDFDEVICE device;
    WDFMEMORY enumChildrenMem;
    PPPM_CONNECTOR connector;
    PACPI_ENUM_CHILDREN_OUTPUT_BUFFER enumChildrenBuf;
    PACPI_ENUM_CHILD enumChild;
    ACPI_PLD_BUFFER pldBuffer;
    PACPI_CONTEXT acpiCtx;
    ULONG64 connectorId;
    WDF_OBJECT_ATTRIBUTES attributes;
    ULONG i;

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_PPM);

    enumChildrenMem = WDF_NO_HANDLE;
    device = Context_GetWdfDevice(PpmCtx);
    acpiCtx = &Context_GetFdoContext(PpmCtx)->AcpiCtx;

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = device;

    status = WdfCollectionCreate(&attributes, &PpmCtx->Connectors);
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_PPM, "[Device: 0x%p] WdfCollectionCreate failed - %!STATUS!", device, status);
        goto Exit;
    }

    status = Acpi_EnumChildren(acpiCtx, &enumChildrenMem);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    enumChildrenBuf =
        (PACPI_ENUM_CHILDREN_OUTPUT_BUFFER) WdfMemoryGetBuffer(enumChildrenMem, nullptr);

    //
    // If there is only one child, which is this device itself, then there is no connector
    // information available. Assume there is only one connector.
    //

    if (enumChildrenBuf->NumberOfChildren == 1)
    {
        TRACE_INFO(TRACE_FLAG_PPM, "[Device: 0x%p] Connector information not found. Assuming single-connector system", device);

        connector = Ppm_AddConnector(PpmCtx);
        if (connector == nullptr)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }

        connector->Id = 0;
        connector->Index = 1;

        goto Exit;
    }

    enumChild = enumChildrenBuf->Children;

    for (i = 1; i < enumChildrenBuf->NumberOfChildren; ++i)
    {
        enumChild = ACPI_ENUM_CHILD_NEXT(enumChild);

        //
        // Connectors are child devices that have a _PLD method. Skip any children
        // that don't have any children, and attempt to evaluate _PLD on the ones that do.
        //

        if ((enumChild->Flags & ACPI_OBJECT_HAS_CHILDREN) == 0)
        {
            continue;
        }

        status = Acpi_EvaluatePld(acpiCtx, enumChild->Name, &pldBuffer);
        if (!NT_SUCCESS(status))
        {
            continue;
        }

        connectorId = UCM_CONNECTOR_ID_FROM_ACPI_PLD(&pldBuffer);

        TRACE_INFO(TRACE_FLAG_PPM, "[Device: 0x%p] Found connector %s with ID 0x%I64x", device, enumChild->Name, connectorId);

        connector = Ppm_AddConnector(PpmCtx);
        if (connector == nullptr)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }

        connector->Id = connectorId;
    }

    TRACE_INFO(TRACE_FLAG_PPM, "[Device: 0x%p] Found %lu connectors", device, WdfCollectionGetCount(PpmCtx->Connectors));

Exit:

    if (enumChildrenMem != WDF_NO_HANDLE)
    {
        WdfObjectDelete(enumChildrenMem);
    }

    TRACE_FUNC_EXIT(TRACE_FLAG_PPM);

    return status;
}


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Ppm_GetCapability (
    _In_ PPPM_CONTEXT PpmCtx,
    _Out_ PUCSI_GET_CAPABILITY_IN Caps
    )
{
    NTSTATUS status;
    WDFDEVICE device;
    UCSI_CONTROL cmd;
    UCSI_MESSAGE_IN msg;
    ULONG_PTR bytesReturned;

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_PPM);

    device = Context_GetWdfDevice(PpmCtx);

    cmd.AsUInt64 = 0;
    cmd.Command = UcsiCommandGetCapability;

    status = Ppm_SendCommandSynchronously(PpmCtx, cmd, &msg, &bytesReturned);
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_PPM, "[Device: 0x%p] Failed to retrieve capabilities - %!STATUS!", device, status);
        goto Exit;
    }

    if (bytesReturned != sizeof(*Caps))
    {
        status = STATUS_INVALID_BUFFER_SIZE;
        TRACE_ERROR(TRACE_FLAG_PPM, "[Device: 0x%p] Invalid capabilities buffer size", device);
        goto Exit;
    }

    RtlCopyMemory(Caps, &msg.Capability, sizeof(*Caps));

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_PPM);

    return status;
}


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Ppm_GetConnectorCapability (
    _In_ PPPM_CONTEXT PpmCtx,
    _In_ CONNECTOR_INDEX ConnectorIndex,
    _Out_ PUCSI_GET_CONNECTOR_CAPABILITY_IN ConnCaps
    )
{
    NTSTATUS status;
    WDFDEVICE device;
    UCSI_CONTROL cmd;
    UCSI_MESSAGE_IN msg;
    ULONG_PTR bytesReturned;

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_PPM);

    device = Context_GetWdfDevice(PpmCtx);

    cmd.AsUInt64 = 0;
    cmd.Command = UcsiCommandGetConnectorCapability;
    cmd.GetConnectorCapability.ConnectorNumber = ConnectorIndex;

    status = Ppm_SendCommandSynchronously(PpmCtx, cmd, &msg, &bytesReturned);
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_PPM, "[Device: 0x%p] Failed to retrieve connector capabilities for connector index %u - %!STATUS!", device, ConnectorIndex, status);
        goto Exit;
    }

    if (bytesReturned != sizeof(*ConnCaps))
    {
        status = STATUS_INVALID_BUFFER_SIZE;
        TRACE_ERROR(TRACE_FLAG_PPM, "[Device: 0x%p] Invalid connector capabilities buffer size", device);
        goto Exit;
    }

    RtlCopyMemory(ConnCaps, &msg.Capability, sizeof(*ConnCaps));

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_PPM);

    return status;
}


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Ppm_ConnectorSetUor (
    _In_ PPPM_CONTEXT PpmCtx,
    _In_ CONNECTOR_INDEX ConnectorIndex,
    _In_ UCSI_USB_OPERATION_ROLE Uor
    )
{
    WDFDEVICE device;
    PPPM_CONNECTOR connector;
    UCM_DATA_ROLE dataRole;
    NTSTATUS status;

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_PPM);

    device = Context_GetWdfDevice(PpmCtx);

    if (!Convert(Uor, dataRole))
    {
        status = STATUS_INVALID_PARAMETER;
        TRACE_ERROR(TRACE_FLAG_PPM, "[Device: 0x%p] Invalid USB operation role 0x%x", device, Uor);
        goto Exit;
    }

    TRACE_INFO(TRACE_FLAG_PPM, "[Device: 0x%p] Test interface invoking SetDataRole for %!UCM_DATA_ROLE!", device, dataRole);

    connector = Ppm_GetConnector(PpmCtx, ConnectorIndex);
    status = Ucm_EvtConnectorSetDataRole(connector->Handle, dataRole);

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_PPM);

    return status;
}


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Ppm_ConnectorSetPdr (
    _In_ PPPM_CONTEXT PpmCtx,
    _In_ CONNECTOR_INDEX ConnectorIndex,
    _In_ UCSI_POWER_DIRECTION_ROLE Pdr
    )
{
    WDFDEVICE device;
    PPPM_CONNECTOR connector;
    UCM_POWER_ROLE powerRole;
    NTSTATUS status;

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_PPM);

    device = Context_GetWdfDevice(PpmCtx);

    if (!Convert(Pdr, powerRole))
    {
        status = STATUS_INVALID_PARAMETER;
        TRACE_ERROR(TRACE_FLAG_PPM, "[Device: 0x%p] Invalid power direction role 0x%x", device, Pdr);
        goto Exit;
    }

    TRACE_INFO(TRACE_FLAG_PPM, "[Device: 0x%p] Test interface invoking SetPowerRole for %!UCM_POWER_ROLE!", device, powerRole);

    connector = Ppm_GetConnector(PpmCtx, ConnectorIndex);
    status = Ucm_EvtConnectorSetPowerRole(connector->Handle, powerRole);

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_PPM);

    return status;
}


_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
_Success_(return != 0)
PPPM_CONNECTOR
Ppm_AddConnector (
    _In_ PPPM_CONTEXT PpmCtx
    )
{
    NTSTATUS status;
    WDFDEVICE device;
    WDFMEMORY connectorMem;
    PPPM_CONNECTOR connector;
    WDF_OBJECT_ATTRIBUTES attributes;

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_PPM);

    device = Context_GetWdfDevice(PpmCtx);
    connector = nullptr;

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = PpmCtx->Connectors;
    status = WdfMemoryCreate(&attributes,
                             NonPagedPoolNx,
                             0,
                             sizeof(PPM_CONNECTOR),
                             &connectorMem,
                             (PVOID*) &connector);
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_PPM, "[Device: 0x%p] WdfMemoryCreate for PPM_CONNECTOR failed - %!STATUS!", device, status);
        goto Exit;
    }

    status = WdfCollectionAdd(PpmCtx->Connectors, connectorMem);
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_PPM, "[Device: 0x%p] WdfCollectionAdd for connector failed - %!STATUS!", device, status);
        goto Exit;
    }

    //
    // N.B. Should not fail beyond this point, else the connector object must be removed
    //      from the collection.
    //

    RtlZeroMemory(connector, sizeof(*connector));

    connector->WdfDevice = device;
    connector->Index = WdfCollectionGetCount(PpmCtx->Connectors);

Exit:

    if (!NT_SUCCESS(status) && (connector != nullptr))
    {
        WdfObjectDelete(connectorMem);
        connector = nullptr;
    }

    TRACE_FUNC_EXIT(TRACE_FLAG_PPM);

    return connector;
}


_IRQL_requires_max_(DISPATCH_LEVEL)
PPPM_CONNECTOR
Ppm_GetConnector (
    _In_ PPPM_CONTEXT PpmCtx,
    _In_ CONNECTOR_INDEX Index
    )
{
    WDFMEMORY connectorMem;
    PPPM_CONNECTOR connector;

    TRACE_FUNC_ENTRY(TRACE_FLAG_PPM);

    //
    // UCSI connector index is one-based.
    //

    connectorMem = (WDFMEMORY) WdfCollectionGetItem(PpmCtx->Connectors, Index - 1);

    //
    // Assuming caller knows what they are doing.
    //

    NT_ASSERT_ASSUME(connectorMem != NULL);

    connector = (PPPM_CONNECTOR) WdfMemoryGetBuffer(connectorMem, nullptr);

    NT_ASSERTMSG("Connectors were added in incorrect order", connector->Index == Index);

    TRACE_FUNC_EXIT(TRACE_FLAG_PPM);

    return connector;
}


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Ppm_PerformRoleCorrection (
    _In_ PPPM_CONTEXT PpmCtx,
    _In_ PPPM_CONNECTOR Connector
    )
{
    WDFDEVICE device;
    UCSI_CONTROL cmd;
    NTSTATUS status;

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_PPM);

    device = Context_GetWdfDevice(PpmCtx);

    TRACE_INFO(TRACE_FLAG_PPM, "[Device: 0x%p] Performing role-correction on connector ID 0x%I64x", device, Connector->Id);

    cmd.AsUInt64 = 0;
    cmd.Command = UcsiCommandSetUor;
    cmd.SetUor.ConnectorNumber = Connector->Index;
    cmd.SetUor.UsbOperationRole = UcsiUsbOperationRoleDfp;

    status = Ppm_SendCommand(PpmCtx, cmd, Ucm_EvtSetDataRoleCompleted, PpmCtx);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_PPM);

    return status;
}
