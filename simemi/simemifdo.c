/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    simemifdo.c

Abstract:

    This module contains the implementation of the routines related to the
    simemi virtual bus device.

--*/

#include "simemi.h"
#include <limits.h>
#include <ntintsafe.h>

NTSTATUS
SimEmiFdoCreateChildDevice (
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request
    );

NTSTATUS
SimEmiFdoDeleteChildDevice (
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request
    );

PSIM_EMI_BUS_PDO_DATA
SimEmiFdoFindChildDevice (
    _In_ WDFDEVICE Device,
    _In_ ULONG ChildDeviceHandle
    );

EVT_WDF_CHILD_LIST_IDENTIFICATION_DESCRIPTION_CLEANUP SimEmiPdoIdentificationCleanup;
EVT_WDF_CHILD_LIST_IDENTIFICATION_DESCRIPTION_COMPARE SimEmiPdoIdentificationCompare;
EVT_WDF_CHILD_LIST_IDENTIFICATION_DESCRIPTION_DUPLICATE SimEmiPdoIdentificationDuplicate;

NTSTATUS
SimEmiFdoQueryChildDevice (
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request,
    _Out_ PULONG OutputBufferSizeOut
    );

NTSTATUS
SimEmiFdoUpdateChildDevice (
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request
    );

#pragma alloc_text(PAGE, SimEmiFdoControl)
#pragma alloc_text(PAGE, SimEmiFdoCreateChildDevice)
#pragma alloc_text(PAGE, SimEmiFdoDeleteChildDevice)
#pragma alloc_text(PAGE, SimEmiFdoCreateDevice)
#pragma alloc_text(PAGE, SimEmiFdoFindChildDevice)
#pragma alloc_text(PAGE, SimEmiFdoQueryChildDevice)
#pragma alloc_text(PAGE, SimEmiFdoUpdateChildDevice)

VOID
SimEmiFdoControl (
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
    )

/*++

Routine Description:

    This routine handles device control requests for the EMI Bus.

Arguments:

    Queue - Supplies a reference to the WDF queue that this request originates.

    Request - Supplies a reference to the WDF request that was sent.

    OutputBufferLength - Supplies the length of the output buffer in bytes.

    InputBufferLength - Supplies the length of the input buffer in bytes.

    IoControlCode - Supplies the IOCTL for the request.

Return Value:

    None.

--*/

{

    WDFDEVICE Device;
    PSIM_EMI_BUS_FDO_DATA FdoData;
    ULONG OutputLength;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    Device = WdfIoQueueGetDevice(Queue);
    FdoData = SimEmiGetFdoData(Device);
    ExAcquireFastMutex(&FdoData->BusMutex);
    OutputLength = 0;
    switch (IoControlCode) {
    case IOCTL_EMI_BUS_CREATE_DEVICE:
        Status = SimEmiFdoCreateChildDevice(Device, Request);
        break;

    case IOCTL_EMI_BUS_DELETE_DEVICE:
        Status = SimEmiFdoDeleteChildDevice(Device, Request);
        break;

    case IOCTL_EMI_BUS_QUERY_DEVICE_INFO:
        Status = SimEmiFdoQueryChildDevice(Device, Request, &OutputLength);
        break;

    case IOCTL_EMI_BUS_SET_DEVICE_RATE:
        Status = SimEmiFdoUpdateChildDevice(Device, Request);
        break;

    default:
        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    WdfRequestCompleteWithInformation(Request, Status, OutputLength);
    ExReleaseFastMutex(&FdoData->BusMutex);
    return;
}

NTSTATUS
SimEmiFdoCreateChildDevice (
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request
    )

/*++

Routine Description:

    This routine handles an IOCTL_EMI_BUS_CREATE_DEVICE request.

Arguments:

    Device - Supplies a reference to the BUS FDO device.

    Request - Supplies a reference to the WDF Request.

Return Value:

    NTSTATUS.

--*/

{

    USHORT ChannelCount;
    USHORT ChannelIndex;
    PSIM_EMI_BUS_CHANNEL_INFO ChannelInfo;
    ULONG ChannelInfoSize;
    ULONG EntrySize;
    SIM_EMI_BUS_PDO_IDENTIFICATION_INFO Identification;
    PSIM_EMI_BUS_CREATE_DEVICE_INPUT_BUFFER InputBuffer;
    size_t InputBufferLength;
    ULONG RemainingBufferSize;
    NTSTATUS Status;

    RtlZeroMemory(&Identification,
                  sizeof(SIM_EMI_BUS_PDO_IDENTIFICATION_INFO));

    Status = WdfRequestRetrieveInputBuffer(
                Request,
                sizeof(SIM_EMI_BUS_CREATE_DEVICE_INPUT_BUFFER),
                (PVOID*)&InputBuffer,
                &InputBufferLength);

    if (!NT_SUCCESS(Status)) {
        goto FdoCreateChildDeviceEnd;
    }

    ChannelCount = InputBuffer->ChannelCount;
    if (InputBuffer->EmiVersion == EMI_VERSION_V1) {
        if (ChannelCount != 1) {
            Status = STATUS_INVALID_PARAMETER;
            goto FdoCreateChildDeviceEnd;
        }

    } else if (InputBuffer->EmiVersion == EMI_VERSION_V2) {
        if (ChannelCount == 0) {
            Status = STATUS_INVALID_PARAMETER;
            goto FdoCreateChildDeviceEnd;
        }

    } else {
        Status = STATUS_INVALID_PARAMETER;
        goto FdoCreateChildDeviceEnd;
    }

    if (InputBufferLength > ULONG_MAX) {
        Status = STATUS_BUFFER_OVERFLOW;
        goto FdoCreateChildDeviceEnd;
    }

    ChannelInfo = &InputBuffer->ChannelInfo[0];
    ChannelInfoSize = 0;
    RemainingBufferSize = (ULONG)InputBufferLength;
    RemainingBufferSize -= FIELD_OFFSET(SIM_EMI_BUS_CREATE_DEVICE_INPUT_BUFFER,
                                        ChannelInfo);

    for (ChannelIndex = 0; ChannelIndex < ChannelCount; ++ChannelIndex) {
        EntrySize = SIM_EMI_BUS_CHANNEL_INFO_SIZE(ChannelInfo->ChannelNameSize);
        if (EntrySize > RemainingBufferSize) {
            Status = STATUS_INVALID_PARAMETER;
            goto FdoCreateChildDeviceEnd;
        }

        RemainingBufferSize -= EntrySize;
        ChannelInfoSize += EntrySize;
        ChannelInfo = SIM_EMI_BUS_CHANNEL_INFO_NEXT_CHANNEL_INFO(ChannelInfo);
    }

    WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_INIT(
        &Identification.Header,
        sizeof(SIM_EMI_BUS_PDO_IDENTIFICATION_INFO));

    //#pragma warning( suppress : 4996 )
    Identification.ChannelInfo = ExAllocatePool2(POOL_FLAG_NON_PAGED,
                                                 ChannelInfoSize,
                                                 SIM_EMI_TAG);

    if (Identification.ChannelInfo == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto FdoCreateChildDeviceEnd;
    }

    //
    // Initialize description and add the device.
    //

    Identification.EmiVersion = InputBuffer->EmiVersion;
    Identification.ChildDeviceHandle = InputBuffer->ChildDeviceHandle;
    Identification.ChannelCount = InputBuffer->ChannelCount;
    Identification.ChannelInfoSize = ChannelInfoSize;
    RtlCopyMemory(&Identification.ChannelInfo[0],
                  &InputBuffer->ChannelInfo[0],
                  ChannelInfoSize);

    Status = WdfChildListAddOrUpdateChildDescriptionAsPresent(
                WdfFdoGetDefaultChildList(Device),
                &Identification.Header,
                NULL);

    if (!NT_SUCCESS(Status)) {
        goto FdoCreateChildDeviceEnd;
    }

    Status = STATUS_SUCCESS;

FdoCreateChildDeviceEnd:
    if (Identification.ChannelInfo != NULL) {
        ExFreePoolWithTag(Identification.ChannelInfo, SIM_EMI_TAG);
    }

    return Status;
}

NTSTATUS
SimEmiFdoDeleteChildDevice (
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request
    )

/*++

Routine Description:

    This routine handles an IOCTL_EMI_BUS_DELETE_DEVICE request.

Arguments:

    Device - Supplies a reference to the BUS FDO device.

    Request - Supplies a reference to the WDF Request.

Return Value:

    NTSTATUS.

--*/

{

    WDFCHILDLIST ChildList;
    SIM_EMI_BUS_PDO_IDENTIFICATION_INFO Identification;
    PSIM_EMI_BUS_DELETE_DEVICE_INPUT_BUFFER InputBuffer;
    NTSTATUS Status;

    ChildList = WdfFdoGetDefaultChildList(Device);
    Status = WdfRequestRetrieveInputBuffer(
                Request,
                sizeof(SIM_EMI_BUS_DELETE_DEVICE_INPUT_BUFFER),
                (PVOID*)&InputBuffer,
                NULL);

    if (!NT_SUCCESS(Status)) {
        goto FdoDeleteChildDeviceEnd;
    }

    WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_INIT(
        &Identification.Header,
        sizeof(SIM_EMI_BUS_PDO_IDENTIFICATION_INFO));

    Identification.ChildDeviceHandle = InputBuffer->ChildDeviceHandle;
    Status = WdfChildListUpdateChildDescriptionAsMissing(
                ChildList,
                &Identification.Header);

    if (Status == STATUS_NO_SUCH_DEVICE) {
        Status = STATUS_INVALID_PARAMETER;
    }

FdoDeleteChildDeviceEnd:
    return Status;
}

NTSTATUS
SimEmiFdoCreateDevice (
    _In_ WDFDRIVER Driver,
    _In_ PWDFDEVICE_INIT DeviceInit
    )

/*++

Routine Description:

    This routine creates and initializes the bus FDO device.

Arguments:

    Driver - Supplies a reference to the WDFDRIVER for this device.

    DeviceInit - Supplies a pointer to the WDFDEVICE_INIT instance for this
        device.

Return Value:

    NTSTATUS.

--*/

{

    WDF_CHILD_LIST_CONFIG ChildListConfig;
    WDFDEVICE DeviceHandle;
    PSIM_EMI_BUS_FDO_DATA FdoData;
    WDF_OBJECT_ATTRIBUTES FdoDeviceAttributes;
    WDFQUEUE Queue;
    WDF_IO_QUEUE_CONFIG QueueConfig;
    NTSTATUS Status;

    DECLARE_CONST_UNICODE_STRING(DeviceName, SIM_EMI_DEVICE_PATH);
    DECLARE_CONST_UNICODE_STRING(DeviceAlias, SIM_EMI_DEVICE_ALIAS_PATH);

    PAGED_CODE();

    UNREFERENCED_PARAMETER(Driver);

    KdPrint(("Initializing SimEmi PDO.\n"));
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&FdoDeviceAttributes,
                                            SIM_EMI_BUS_FDO_DATA);

    WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_BUS_EXTENDER);
    WdfDeviceInitSetExclusive(DeviceInit, TRUE);
    WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoBuffered);

    //
    // Initialize child list.
    //

    WDF_CHILD_LIST_CONFIG_INIT(&ChildListConfig,
                               sizeof(SIM_EMI_BUS_PDO_IDENTIFICATION_INFO),
                               &SimEmiPdoCreateDevice);

    ChildListConfig.EvtChildListIdentificationDescriptionCleanup =
        &SimEmiPdoIdentificationCleanup;

    ChildListConfig.EvtChildListIdentificationDescriptionCompare =
        &SimEmiPdoIdentificationCompare;

    ChildListConfig.EvtChildListIdentificationDescriptionDuplicate =
        &SimEmiPdoIdentificationDuplicate;

    WdfFdoInitSetDefaultChildListConfig(DeviceInit,
                                        &ChildListConfig,
                                        WDF_NO_OBJECT_ATTRIBUTES);

    Status = WdfDeviceInitAssignName(DeviceInit, &DeviceName);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("WdfDeviceInitAssignName failed with status code 0x%08X.\n",
                 Status));

        goto FdoCreateDeviceEnd;
    }

    Status = WdfDeviceCreate(&DeviceInit, &FdoDeviceAttributes, &DeviceHandle);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("WdfDeviceCreate failed with status code 0x%08X.\n", Status));
        goto FdoCreateDeviceEnd;
    }

    FdoData = SimEmiGetFdoData(DeviceHandle);
    RtlZeroMemory(FdoData, sizeof(SIM_EMI_BUS_FDO_DATA));
    InitializeListHead(&FdoData->ChildDevices);
    ExInitializeFastMutex(&FdoData->BusMutex);

    //
    // Setup symbolic links for UM.
    //

    Status = WdfDeviceCreateSymbolicLink(DeviceHandle, &DeviceAlias);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("WdfDeviceCreateSymbolicLink failed with status code :0x%08X.\n",
                 Status));

        goto FdoCreateDeviceEnd;
    }



    //
    // Initialize IO Queue.
    //

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&QueueConfig,
                                           WdfIoQueueDispatchSequential);

    QueueConfig.EvtIoDeviceControl = &SimEmiFdoControl;
    Status = WdfIoQueueCreate(DeviceHandle,
                              &QueueConfig,
                              WDF_NO_OBJECT_ATTRIBUTES,
                              &Queue);

    if (!NT_SUCCESS(Status)) {
        KdPrint(("WdfIoQueueCreate failed with status code 0x%08X.\n", Status));
        goto FdoCreateDeviceEnd;
    }

    Status = WdfDeviceCreateDeviceInterface(DeviceHandle,
                                            &GUID_DEVICE_SIM_EMI_BUS,
                                            NULL);

    if (!NT_SUCCESS(Status)) {
        KdPrint(("WdfDeviceCreateDeviceInterface failed with status code 0x%08X.\n",
                 Status));

        goto FdoCreateDeviceEnd;
    }

    Status = STATUS_SUCCESS;

FdoCreateDeviceEnd:
    return Status;
}

PSIM_EMI_BUS_PDO_DATA
SimEmiFdoFindChildDevice (
    _In_ WDFDEVICE Device,
    _In_ ULONG ChildDeviceHandle
    )

/*++

Routine Description:

    This routine locates the PDO data for the child device with the provided
    device handle.

Arguments:

    Device - Supplies a reference to the WDFDEVICE of the bus FDO.

    ChildDeviceHandle - Supplies the device handle of the child device to
        locate.

Return Value:

    Returns a pointer to the PDO data for the child device with the provided
    device handle or NULL if no child exists with the provided device handle.

--*/

{

    PSIM_EMI_BUS_PDO_DATA ChildData;
    PLIST_ENTRY ListEntry;
    PSIM_EMI_BUS_FDO_DATA FdoData;

    FdoData = SimEmiGetFdoData(Device);
    ChildData = NULL;
    for (ListEntry = FdoData->ChildDevices.Flink;
         ListEntry != &FdoData->ChildDevices;
         ListEntry = ListEntry->Flink) {

        ChildData = CONTAINING_RECORD(ListEntry,
                                      SIM_EMI_BUS_PDO_DATA,
                                      Link);

        if (ChildData->ChildDeviceHandle == ChildDeviceHandle) {
            break;
        }

        ChildData = NULL;
    }

    return ChildData;
}

NTSTATUS
SimEmiFdoQueryChildDevice (
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request,
    _Out_ PULONG OutputBufferSizeOut
    )

/*++

Routine Description:

    This routine handles an IOCTL_EMI_BUS_QUERY_DEVICE_INFO request.

Arguments:

    Device - Supplies a reference to the BUS FDO device.

    Request - Supplies a reference to the WDF Request.

    OutputBufferSizeOut - Supplies a pointer in which to place the size of the
        output data written.

Return Value:

    NTSTATUS.

--*/
{

    USHORT ChannelCount;
    PSIM_EMI_CHANNEL_DATA ChannelData;
    PSIM_EMI_BUS_CHANNEL_INFO ChannelInfo;
    PSIM_EMI_BUS_PDO_DATA ChildData;
    ULONG EntrySize;
    PSIM_EMI_BUS_QUERY_DEVICE_INFO_INPUT_BUFFER InputBuffer;
    PSIM_EMI_BUS_QUERY_DEVICE_INFO_OUTPUT_BUFFER OutputBuffer;
    ULONG OutputBufferSize;
    NTSTATUS Status;

    Status = WdfRequestRetrieveInputBuffer(
                Request,
                sizeof(SIM_EMI_BUS_QUERY_DEVICE_INFO_INPUT_BUFFER),
                (PVOID*)&InputBuffer,
                NULL);

    if (!NT_SUCCESS(Status)) {
        goto FdoQueryChildDeviceEnd;
    }

    ChildData = SimEmiFdoFindChildDevice(Device,
                                         InputBuffer->ChildDeviceHandle);

    if (ChildData == NULL) {
        Status = STATUS_NOT_FOUND;
        goto FdoQueryChildDeviceEnd;
    }

    OutputBufferSize = FIELD_OFFSET(SIM_EMI_BUS_QUERY_DEVICE_INFO_OUTPUT_BUFFER,
                                    ChannelInfo);

    ChannelCount = ChildData->ChannelCount;
    ChannelData = &ChildData->ChannelData[0];
    while (ChannelCount != 0) {
        EntrySize =
            SIM_EMI_BUS_CHANNEL_INFO_SIZE(ChannelData->Info.ChannelNameSize);

        Status = RtlULongAdd(OutputBufferSize, EntrySize, &OutputBufferSize);
        if (!NT_SUCCESS(Status)) {
            goto FdoQueryChildDeviceEnd;
        }

        ChannelData = SIM_EMI_CHANNEL_DATA_NEXT_CHANNEL_DATA(ChannelData);
        ChannelCount -= 1;
    }

    *OutputBufferSizeOut = OutputBufferSize;
    Status = WdfRequestRetrieveOutputBuffer(Request,
                                            OutputBufferSize,
                                            (PVOID*)&OutputBuffer,
                                            NULL);

    if (!NT_SUCCESS(Status)) {
        goto FdoQueryChildDeviceEnd;
    }

    OutputBuffer->EmiVersion = ChildData->EmiVersion;
    OutputBuffer->ChannelCount = ChildData->ChannelCount;
    ChannelCount = ChildData->ChannelCount;
    ChannelData = &ChildData->ChannelData[0];
    ChannelInfo = &OutputBuffer->ChannelInfo[0];
    while (ChannelCount != 0) {
        EntrySize =
            SIM_EMI_BUS_CHANNEL_INFO_SIZE(ChannelData->Info.ChannelNameSize);

        RtlCopyMemory(ChannelInfo, &ChannelData->Info, EntrySize);
        ChannelData = SIM_EMI_CHANNEL_DATA_NEXT_CHANNEL_DATA(ChannelData);
        ChannelInfo = SIM_EMI_BUS_CHANNEL_INFO_NEXT_CHANNEL_INFO(ChannelInfo);
        ChannelCount -= 1;
    }

    Status = STATUS_SUCCESS;

FdoQueryChildDeviceEnd:
    return Status;
}

NTSTATUS
SimEmiFdoUpdateChildDevice (
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request
    )

/*++

Routine Description:

    This routine handles an IOCTL_EMI_BUS_SET_DEVICE_RATE request.

Arguments:

    Device - Supplies a reference to the BUS FDO device.

    Request - Supplies a reference to the WDF Request.

Return Value:

    NTSTATUS.

--*/
{

    PULONG64 AbsoluteEnergyRates;
    USHORT ChannelCount;
    PSIM_EMI_CHANNEL_DATA ChannelData;
    PSIM_EMI_BUS_PDO_DATA ChildData;
    PSIM_EMI_BUS_SET_DEVICE_RATE_INPUT_BUFFER InputBuffer;
    ULONG InputBufferSize;
    NTSTATUS Status;

    Status = WdfRequestRetrieveInputBuffer(
                Request,
                sizeof(SIM_EMI_BUS_SET_DEVICE_RATE_INPUT_BUFFER),
                (PVOID*)&InputBuffer,
                NULL);

    if (!NT_SUCCESS(Status)) {
        goto FdoUpdateChildDeviceEnd;
    }

    ChildData = SimEmiFdoFindChildDevice(Device,
                                         InputBuffer->ChildDeviceHandle);

    if (ChildData == NULL) {
        Status = STATUS_INVALID_PARAMETER;
        goto FdoUpdateChildDeviceEnd;
    }

    if ((ChildData->EmiVersion != InputBuffer->EmiVersion) ||
        (ChildData->ChannelCount != InputBuffer->ChannelCount)) {

        Status = STATUS_INVALID_PARAMETER;
        goto FdoUpdateChildDeviceEnd;
    }

    ChannelCount = ChildData->ChannelCount;
    Status = RtlULongMult(sizeof(ULONG64), ChannelCount, &InputBufferSize);
    if (!NT_SUCCESS(Status)) {
        goto FdoUpdateChildDeviceEnd;
    }

    InputBufferSize += FIELD_OFFSET(SIM_EMI_BUS_SET_DEVICE_RATE_INPUT_BUFFER,
                                    AbsoluteEnergyRates);

    Status = WdfRequestRetrieveInputBuffer(Request,
                                           InputBufferSize,
                                           (PVOID*)&InputBuffer,
                                           NULL);

    if (!NT_SUCCESS(Status)) {
        goto FdoUpdateChildDeviceEnd;
    }

    AbsoluteEnergyRates = &InputBuffer->AbsoluteEnergyRates[0];
    ChannelData = &ChildData->ChannelData[0];
    while (ChannelCount != 0) {
        ChannelData->Info.AbsoluteEnergyRate = *AbsoluteEnergyRates;
        AbsoluteEnergyRates += 1;
        ChannelData = SIM_EMI_CHANNEL_DATA_NEXT_CHANNEL_DATA(ChannelData);
        ChannelCount -= 1;
    }

    Status = STATUS_SUCCESS;

FdoUpdateChildDeviceEnd:
    return Status;
}

VOID
SimEmiPdoIdentificationCleanup (
    _In_ WDFCHILDLIST ChildList,
    _Inout_ PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER IdentificationDescription
    )

/*++

Routine Description:

    This routine cleans up the resources owned by a bus PDO description
    instance.

Arguments:

    ChildList - Supplies a reference to the WDFCHILDLIST this description is
        being added to.

    IdentificationDescription - Supplies a pointer to the identification info
        instance to clean up.

Return Value:

    None.

--*/

{

    PSIM_EMI_BUS_PDO_IDENTIFICATION_INFO Description;

    UNREFERENCED_PARAMETER(ChildList);

    Description =
        (PSIM_EMI_BUS_PDO_IDENTIFICATION_INFO)IdentificationDescription;

    if (Description->ChannelInfo != NULL) {
        ExFreePoolWithTag(Description->ChannelInfo, SIM_EMI_TAG);
    }

    return;
}

BOOLEAN
SimEmiPdoIdentificationCompare (
    _In_ WDFCHILDLIST ChildList,
    _In_ PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER FirstIdentificationDescription,
    _In_ PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER SecondIdentificationDescription
    )

/*++

Routine Description:

    This routine compares two SIM_EMI_BUS_PDO_IDENTIFICATION_INFOs for
    equality.

Arguments:

    ChildList - Supplies a reference to the WDFCHILDLIST this description is
        being added to.

    FirstIdentificationDescription - Supplies a pointer to a PDO identification
        info instance.

    SecondIdentificationDescription - Supplies a pointer to a PDO
        identification instance.

Return Value:

    Returns TRUE if FirstIdentificationInfo == SecondIdentificationInfo, FALSE
    otherwise.

--*/

{

    PSIM_EMI_BUS_PDO_IDENTIFICATION_INFO Description1;
    PSIM_EMI_BUS_PDO_IDENTIFICATION_INFO Description2;
    BOOLEAN Result;

    UNREFERENCED_PARAMETER(ChildList);

    Description1 =
        (PSIM_EMI_BUS_PDO_IDENTIFICATION_INFO)FirstIdentificationDescription;

    Description2 =
        (PSIM_EMI_BUS_PDO_IDENTIFICATION_INFO)SecondIdentificationDescription;

    Result = FALSE;
    if (Description1->ChildDeviceHandle == Description2->ChildDeviceHandle) {
        Result = TRUE;
    }

    return Result;
}

NTSTATUS
SimEmiPdoIdentificationDuplicate (
    _In_ WDFCHILDLIST ChildList,
    _In_ PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER SourceIdentificationDescription,
    _Out_ PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER DestinationIdentificationDescription
    )

/*++

Routine Description:

    This routine duplicates a SIM_EMI_BUS_PDO_IDENTIFICATION_INFO instance.

Arguments:

    ChildList - Supplies a reference to the WDFCHILDLIST this description is
        being added to.

    SourceIdentificationDescription - Supplies a pointer to the identification
        info instance to duplicate.

    DestinationIdentificationDescription - Supplies a pointer to the location
        to place the copied identification info.

Return Value:

    NTSTATUS.

--*/

{

    ULONG ChannelInfoSize;
    PSIM_EMI_BUS_PDO_IDENTIFICATION_INFO DestinationDescription;
    PSIM_EMI_BUS_PDO_IDENTIFICATION_INFO SourceDescription;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(ChildList);

    DestinationDescription =
        (PSIM_EMI_BUS_PDO_IDENTIFICATION_INFO)DestinationIdentificationDescription;

    SourceDescription =
        (PSIM_EMI_BUS_PDO_IDENTIFICATION_INFO)SourceIdentificationDescription;

    ChannelInfoSize = SourceDescription->ChannelInfoSize;
    //#pragma warning( suppress : 4996 )
    DestinationDescription->ChannelInfo = ExAllocatePool2(POOL_FLAG_NON_PAGED,
                                                          ChannelInfoSize,
                                                          SIM_EMI_TAG);

    if (DestinationDescription->ChannelInfo == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto PdoIdentificationDuplicateEnd;
    }

    DestinationDescription->EmiVersion = SourceDescription->EmiVersion;
    DestinationDescription->ChildDeviceHandle =
        SourceDescription->ChildDeviceHandle;

    DestinationDescription->ChannelCount = SourceDescription->ChannelCount;
    DestinationDescription->ChannelInfoSize = ChannelInfoSize;
    RtlCopyMemory(&DestinationDescription->ChannelInfo[0],
                  &SourceDescription->ChannelInfo[0],
                  ChannelInfoSize);

    Status = STATUS_SUCCESS;

PdoIdentificationDuplicateEnd:
    return Status;
}