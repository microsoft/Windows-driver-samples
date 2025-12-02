//
//    Copyright (C) Microsoft.  All rights reserved.
//

#include "precomp.h"

#include "device.h"
#include "rxqueue.h"

const ULONG MBB_DEFAULT_IDLE_TIMEOUT_HINT_MS = 2u * 1000u; // 2 seconds

_Use_decl_annotations_ VOID EvtMbbDeviceSendMbimFragment(WDFDEVICE Device, MBBREQUEST Fragment)
{
    PWMBCLASS_DEVICE_CONTEXT deviceContext = WmbClassGetDeviceContext(Device);

    size_t bufferSize = 0;
    PVOID buffer = MbbRequestGetBuffer(Fragment, &bufferSize);

    auto completionRoutine = [](MBB_PROTOCOL_HANDLE, MBB_REQUEST_HANDLE RequestHandle, NTSTATUS NtStatus) {
        MbbRequestComplete((MBBREQUEST)RequestHandle, NtStatus);
    };

    NTSTATUS sendStatus = MbbBusSendMessageFragment(
        deviceContext->BusHandle, Fragment, buffer, (ULONG)bufferSize, (LPGUID)MbbRequestGetCorrelationId(Fragment), completionRoutine);

    // Accoding to the documentation of MbbBusSendMessageFragment the completion routine
    // will be called only when ntStatus is STATUS_PENDING, so we need to call it ourselves
    // for the other cases
    if (sendStatus != STATUS_PENDING)
    {
        completionRoutine(deviceContext, Fragment, sendStatus);
    }
}

VOID MbbNdisResponseFragmentAvailable(__in MBB_PROTOCOL_HANDLE ProtocolHandle)
{
    PWMBCLASS_DEVICE_CONTEXT deviceContext = (PWMBCLASS_DEVICE_CONTEXT)ProtocolHandle;
    MbbDeviceResponseAvailable(deviceContext->WdfDevice);
}

_Use_decl_annotations_ VOID EvtMbbDeviceReceiveMbimFragment(WDFDEVICE Device, MBBREQUEST Fragment)
{
    PWMBCLASS_DEVICE_CONTEXT deviceContext = WmbClassGetDeviceContext(Device);

    size_t bufferSize = 0;
    PVOID buffer = MbbRequestGetBuffer(Fragment, &bufferSize);

    auto completionRoutine = [](MBB_PROTOCOL_HANDLE, MBB_REQUEST_HANDLE RequestHandle, NTSTATUS Status, ULONG_PTR ReceivedLength) {
        MbbRequestCompleteWithInformation((MBBREQUEST)RequestHandle, Status, ReceivedLength);
    };

    // Accoding to the documentation of MbbBusReceiveMessageFragment the completion routine
    // will be called only when ntStatus is STATUS_PENDING, so we need to call it ourselves
    // for the other cases
    NTSTATUS receiveStatus = MbbBusReceiveMessageFragment(
        deviceContext->BusHandle, Fragment, buffer, (ULONG)bufferSize, (LPGUID)MbbRequestGetCorrelationId(Fragment), completionRoutine);

    if (receiveStatus != STATUS_PENDING)
    {
        completionRoutine(deviceContext, Fragment, receiveStatus, 0);
    }
}

static NTSTATUS SetupAdapterSpecificPowerSettings(_In_ PWMBCLASS_DEVICE_CONTEXT DeviceContext)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (DeviceContext->BusParams.RemoteWakeCapable)
    {
        //
        // Configure USB selective suspend with default timeout
        //
        WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS idleSettings;
        WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(&idleSettings, IdleUsbSelectiveSuspend);

        idleSettings.IdleTimeout = MBB_DEFAULT_IDLE_TIMEOUT_HINT_MS;
        idleSettings.IdleTimeoutType = SystemManagedIdleTimeoutWithHint;

        status = WdfDeviceAssignS0IdleSettings(DeviceContext->WdfDevice, &idleSettings);
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }
    }
Exit:
    return status;
}

NTSTATUS
MbbInitializeHardware(_In_ PWMBCLASS_DEVICE_CONTEXT DeviceContext)
{
    NTSTATUS status = STATUS_SUCCESS;
    WDF_OBJECT_ATTRIBUTES attributes;
    UNICODE_STRING manufacturer;
    UNICODE_STRING model;

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = DeviceContext->WdfDevice;
    status = WdfLookasideListCreate(
        &attributes,
        sizeof(MBB_NDIS_RECEIVE_CONTEXT),
        NonPagedPoolNx,
        WDF_NO_OBJECT_ATTRIBUTES,
        MbbPoolTagMdlReceive,
        &DeviceContext->ReceiveLookasideList);
    if (!NT_SUCCESS(status))
    {
        goto Cleanup;
    }

    for (int i = 0; i < MBB_MAX_NUMBER_OF_SESSIONS; i++)
    {
        WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
        attributes.ParentObject = DeviceContext->WdfDevice;

        status = WdfSpinLockCreate(&attributes, &DeviceContext->Sessions[i].WdfRecvSpinLock);
        if (!NT_SUCCESS(status))
        {
            goto Cleanup;
        }
    }

    status = MbbBusInitializeByWdf(
        DeviceContext->WdfDevice,
        MbbNdisResponseFragmentAvailable,
        MbbNdisReceiveCallback,
        nullptr,
        nullptr,
        DeviceContext,
        &DeviceContext->BusHandle,
        nullptr);

    if (status != STATUS_SUCCESS)
    {
        goto Cleanup;
    }

    //
    // Initialize the OID/CID handlers before the device is OPENED.
    // Once the device is opened and the interrupt pipe is connected.
    // This driver will start receiving unsolicited interrupt message
    // from the device.
    //
    status = MbbBusQueryBusParameters(DeviceContext->BusHandle, &DeviceContext->BusParams);

    if (status != STATUS_SUCCESS)
    {
        goto Cleanup;
    }

    WDFMEMORY sharedPaddingMemory;
    ULONG sharedPaddingLength = max(DeviceContext->BusParams.NdpOutAlignment, DeviceContext->BusParams.NdpOutDivisor);
    status = CreateNonPagedWdfMemory(
        sharedPaddingLength, &sharedPaddingMemory, (PVOID*)&DeviceContext->sharedPaddingBuffer, DeviceContext->WdfDevice, MbbPoolTagNtbSend);
    if (!NT_SUCCESS(status))
    {
        goto Cleanup;
    }

    RtlInitUnicodeString(&manufacturer, DeviceContext->BusParams.Manufacturer);
    RtlInitUnicodeString(&model, DeviceContext->BusParams.Model);
    MBB_DEVICE_MBIM_PARAMETERS mbimParams;

    // 2.0 by default
    MBB_MBIM_EXTENDED_VERSION mbimExVer = MbbMbimExtendedVersion2Dot0;
    switch(DeviceContext->BusParams.MbimExtendedVersion)
    {
        case 0x0100:
            mbimExVer = MbbMbimExtendedVersion1Dot0;
            break;
        case 0x0200:
            mbimExVer = MbbMbimExtendedVersion2Dot0;
            break;
        case 0x0300:
            mbimExVer = MbbMbimExtendedVersion3Dot0;
            break;
        case 0x0400:
            mbimExVer = MbbMbimExtendedVersion4Dot0;
            break;
        default:
            status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
    }

    MBB_DEVICE_MBIM_PARAMETERS_INIT(
        &mbimParams,
        DeviceContext->BusParams.IsErrataDevice ? MbbMbimVersion1Dot0Errata : MbbMbimVersion1Dot0,
        DeviceContext->BusParams.FragmentSize,
        mbimExVer);

    MbbDeviceSetMbimParameters(DeviceContext->WdfDevice, &mbimParams);

    MBB_DEVICE_WAKE_CAPABILITIES mbbWakeCapabilities;
    MBB_DEVICE_WAKE_CAPABILITIES_INIT(&mbbWakeCapabilities);

    mbbWakeCapabilities.PacketState = TRUE;
    mbbWakeCapabilities.RegisterState = TRUE;
    mbbWakeCapabilities.SmsReceive = TRUE;
    mbbWakeCapabilities.UiccChange = TRUE;
    mbbWakeCapabilities.UssdReceive = TRUE;

    MbbDeviceSetWakeCapabilities(DeviceContext->WdfDevice, &mbbWakeCapabilities);

    status = SetupAdapterSpecificPowerSettings(DeviceContext);
    if (NT_ERROR(status))
    {
        goto Cleanup;
    }

Cleanup:

    if (status != STATUS_SUCCESS)
    {
        if (DeviceContext != NULL)
        {
            if (DeviceContext->BusHandle != NULL)
            {
                MbbBusStop(DeviceContext->BusHandle);
                MbbBusCleanup(DeviceContext->BusHandle);
                DeviceContext->BusHandle = NULL;
            }
        }

    }

    return status;
}

void MbbReleaseHardware(_In_ PWMBCLASS_DEVICE_CONTEXT DeviceContext)
{

    if (DeviceContext->BusHandle != NULL)
    {
        MbbBusCleanup(DeviceContext->BusHandle);
        DeviceContext->BusHandle = NULL;
    }

    return;
}

NTSTATUS
EvtDevicePrepareHardware(_In_ WDFDEVICE device, _In_ WDFCMRESLIST resourcesRaw, _In_ WDFCMRESLIST resourcesTranslated)
{
    UNREFERENCED_PARAMETER(resourcesRaw);
    UNREFERENCED_PARAMETER(resourcesTranslated);

    PWMBCLASS_DEVICE_CONTEXT deviceContext = WmbClassGetDeviceContext(device);

    NTSTATUS status = MbbInitializeHardware(deviceContext);

    return status;
}

NTSTATUS
EvtDeviceReleaseHardware(_In_ WDFDEVICE device, _In_ WDFCMRESLIST resourcesTranslated)
{
    UNREFERENCED_PARAMETER(resourcesTranslated);
    PWMBCLASS_DEVICE_CONTEXT deviceContext = WmbClassGetDeviceContext(device);

    MbbReleaseHardware(deviceContext);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_ void EvtDeviceSurpriseRemoval(WDFDEVICE device)
{
    UNREFERENCED_PARAMETER(device);
}

NTSTATUS MbbInitAdapterContext(_In_ WDFDEVICE Device, _In_ NETADAPTER NetAdapter)
{
    WDF_OBJECT_ATTRIBUTES adapterAttributes;
    PWMBCLASS_DEVICE_CONTEXT deviceContext = WmbClassGetDeviceContext(Device);
    PWMBCLASS_NETADAPTER_CONTEXT netAdapterContext = WmbClassGetNetAdapterContext(NetAdapter);

    ULONG ntbSize;
    ntbSize = FIELD_OFFSET(MBB_NTB_BUILD_CONTEXT, NdpDatagramEntries);
    ntbSize += (deviceContext->BusParams.MaxOutDatagrams * sizeof(MBB_NDP_HEADER_ENTRY));

    WDF_OBJECT_ATTRIBUTES_INIT(&adapterAttributes);
    adapterAttributes.ParentObject = NetAdapter;

    NTSTATUS ntStatus = WdfLookasideListCreate(
        &adapterAttributes, ntbSize, NonPagedPoolNx, WDF_NO_OBJECT_ATTRIBUTES, MbbPoolTagNtbSend, &netAdapterContext->NtbLookasideList);
    if (!NT_SUCCESS(ntStatus))
    {
        return ntStatus;
    }

    WDF_OBJECT_ATTRIBUTES_INIT(&adapterAttributes);
    adapterAttributes.ParentObject = NetAdapter;
    ntStatus = WdfLookasideListCreate(
        &adapterAttributes,
        sizeof(MBB_RECEIVE_NDP_CONTEXT),
        NonPagedPoolNx,
        WDF_NO_OBJECT_ATTRIBUTES,
        MbbPoolTagMdlReceive,
        &netAdapterContext->ReceiveNdpLookasideList);
    if (!NT_SUCCESS(ntStatus))
    {
        return ntStatus;
    }
    netAdapterContext->SessionId = MbbAdapterGetSessionId(NetAdapter);
    WdfSpinLockAcquire(deviceContext->Sessions[netAdapterContext->SessionId].WdfRecvSpinLock);
    deviceContext->Sessions[netAdapterContext->SessionId].NetAdapterContext = netAdapterContext;
    // Allow to receive data from this point since connection may happen before RxQueueCreate, if it happens, we allow to cache the recieved NDP
    netAdapterContext->AllowRxTraffic = TRUE;
    InitializeListHead(&netAdapterContext->ReceiveNdpList);
    netAdapterContext->WmbDeviceContext = deviceContext;
    netAdapterContext->NetAdapter = NetAdapter;
    WdfSpinLockRelease(deviceContext->Sessions[netAdapterContext->SessionId].WdfRecvSpinLock);
    return ntStatus;
}

_Use_decl_annotations_ NTSTATUS EvtMbbDeviceCreateAdapter(WDFDEVICE Device, NETADAPTER_INIT* AdapterInit)
{

    NET_ADAPTER_DATAPATH_CALLBACKS datapathCallbacks;
    NET_ADAPTER_DATAPATH_CALLBACKS_INIT(&datapathCallbacks, EvtAdapterCreateTxQueue, EvtAdapterCreateRxQueue);

    NetAdapterInitSetDatapathCallbacks(AdapterInit, &datapathCallbacks);

    WDF_OBJECT_ATTRIBUTES adapterAttributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&adapterAttributes);
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&adapterAttributes, WMBCLASS_NETADAPTER_CONTEXT);
    adapterAttributes.EvtCleanupCallback = EvtAdapterCleanup;

    NETADAPTER netAdapter;
    NTSTATUS ntStatus = NetAdapterCreate(AdapterInit, &adapterAttributes, &netAdapter);
    if (!NT_SUCCESS(ntStatus))
    {
        return ntStatus;
    }

    ntStatus = MbbAdapterInitialize(netAdapter);

    if (!NT_SUCCESS(ntStatus))
    {
        return ntStatus;
    }

    ntStatus = MbbInitAdapterContext(Device, netAdapter);
    if (!NT_SUCCESS(ntStatus))
    {
        return ntStatus;
    }

    ntStatus = WmbClassAdapterStart(netAdapter);

    if (!NT_SUCCESS(ntStatus))
    {
        return ntStatus;
    }

    return ntStatus;
}

_Use_decl_annotations_ VOID EvtAdapterCleanup(_In_ WDFOBJECT NetAdapter)
{
    PWMBCLASS_NETADAPTER_CONTEXT netAdapterContext = WmbClassGetNetAdapterContext(NetAdapter);
    PWMBCLASS_DEVICE_CONTEXT deviceContext = netAdapterContext->WmbDeviceContext;
    if (deviceContext == NULL || deviceContext->Sessions[netAdapterContext->SessionId].NetAdapterContext == NULL)
    {
        return;
    }

    // Cancel these Ndps which are cached before RxQueueCreate but not cleaned in case RxQueueCreate failed.
    // NetAdapter should be destoryed immediately if RxQueueCreate failed, or these cached Ndps will stay in memory until release device or delete additional PDP
    MbbRecvCancelNdps(deviceContext, netAdapterContext->SessionId);

    WdfSpinLockAcquire(deviceContext->Sessions[netAdapterContext->SessionId].WdfRecvSpinLock);
    deviceContext->Sessions[netAdapterContext->SessionId].NetAdapterContext = NULL;
    WdfSpinLockRelease(deviceContext->Sessions[netAdapterContext->SessionId].WdfRecvSpinLock);

}

_Use_decl_annotations_ VOID EvtMbbDeviceReset(WDFDEVICE WdfDevice)
{
    PWMBCLASS_DEVICE_CONTEXT deviceContext = WmbClassGetDeviceContext(WdfDevice);
    MbbBusResetDataPipes(deviceContext->BusHandle);
}
