/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    simemifdo.c

Abstract:

    This module contains the implementation of the routines related to the
    simemi virtual child device.

--*/

#include "simemi.h"
#include <devguid.h>
#include <ntintsafe.h>
#include <ntstrsafe.h>

NTSTATUS
SimEmiPdoCalculateDataSize (
    _In_ USHORT ChannelCount,
    _In_ PSIM_EMI_BUS_CHANNEL_INFO Info,
    _Out_ PULONG PdoDataSizeOut
    );

NTSTATUS
SimEmiPdoCalculateMetadataSize (
    _In_ PSIM_EMI_BUS_PDO_DATA PdoData,
    _Out_ PULONG MetadataSizeOut
    );

NTSTATUS
SimEmiPdoCopyChannelMeasurements (
    _In_ PSIM_EMI_BUS_PDO_DATA PdoData,
    _Out_ EMI_CHANNEL_MEASUREMENT_DATA *MeasurementData
    );

NTSTATUS
SimEmiPdoCopyDeviceMetadata (
    _In_ PSIM_EMI_BUS_PDO_DATA PdoData,
    _Out_ PVOID MetadataBuffer
    );

EVT_WDF_DEVICE_CONTEXT_DESTROY SimEmiPdoDestroyDevice;

VOID
SimEmiPdoInitializePdoData (
    _In_ PSIM_EMI_BUS_PDO_IDENTIFICATION_INFO Identification,
    _Out_ PSIM_EMI_BUS_PDO_DATA PdoData
    );

#pragma alloc_text(PAGE, SimEmiPdoCalculateDataSize)
#pragma alloc_text(PAGE, SimEmiPdoCalculateMetadataSize)
#pragma alloc_text(PAGE, SimEmiPdoControl)
#pragma alloc_text(PAGE, SimEmiPdoCopyChannelMeasurements)
#pragma alloc_text(PAGE, SimEmiPdoDestroyDevice)
#pragma alloc_text(PAGE, SimEmiPdoCopyDeviceMetadata)
#pragma alloc_text(PAGE, SimEmiPdoCreateDevice)
#pragma alloc_text(PAGE, SimEmiPdoInitializePdoData)

NTSTATUS
SimEmiPdoCalculateDataSize (
    _In_ USHORT ChannelCount,
    _In_ PSIM_EMI_BUS_CHANNEL_INFO Info,
    _Out_ PULONG PdoDataSizeOut
    )

/*++

Routine Description:

    This routine calculates the required size of the child device's PDO data.

Arguments:

    ChannelCount - Supplies the number of channels for this child device.

    Info - Supplies a pointer to an array of channel info structures for this
        child device.

    PdoDataSizeOut - Supplies a pointer in which to place the required size of
        the child device's PDO data.

Return Value:

    NTSTATUS.

--*/

{

    ULONG EntrySize;
    ULONG PdoDataSize;
    NTSTATUS Status;

    Status = STATUS_SUCCESS;
    PdoDataSize = FIELD_OFFSET(SIM_EMI_BUS_PDO_DATA, ChannelData);
    while (ChannelCount != 0) {
        EntrySize = SIM_EMI_CHANNEL_DATA_SIZE(Info->ChannelNameSize);
        Status = RtlULongAdd(PdoDataSize, EntrySize, &PdoDataSize);
        if (!NT_SUCCESS(Status)) {
            goto PdoCalculateDataSizeEnd;
        }

        Info = SIM_EMI_BUS_CHANNEL_INFO_NEXT_CHANNEL_INFO(Info);
        ChannelCount -= 1;
    }

    *PdoDataSizeOut = PdoDataSize;

PdoCalculateDataSizeEnd:
    return Status;
}

NTSTATUS
SimEmiPdoCalculateMetadataSize (
    _In_ PSIM_EMI_BUS_PDO_DATA PdoData,
    _Out_ PULONG MetadataSizeOut
    )

/*++

Routine Description:

    This routine calculates the size of a child device's EMI metadata
    structure.

Arguments:

    PdoData - Supplies a pointer to the child device's PDO data.

    MetadataSizeOut - Supplies a pointer in which to place the required size of
        this child device's EMI metadata structure.

Return Value:

    NTSTATUS.

--*/

{

    USHORT ChannelCount;
    PSIM_EMI_CHANNEL_DATA ChannelData;
    ULONG EntrySize;
    ULONG MetadataSize;
    NTSTATUS Status;

    NT_ASSERT((PdoData->EmiVersion == EMI_VERSION_V1) ||
              (PdoData->EmiVersion == EMI_VERSION_V2));

    if (PdoData->EmiVersion == EMI_VERSION_V1) {

        NT_ASSERT(PdoData->ChannelCount == 1);

        MetadataSize = FIELD_OFFSET(EMI_METADATA_V1, MeteredHardwareName);
        Status = RtlULongAdd(MetadataSize,
                             PdoData->ChannelData[0].Info.ChannelNameSize,
                             &MetadataSize);

        if (!NT_SUCCESS(Status)) {
            goto PdoCalculateMetadataSizeEnd;
        }

    } else if (PdoData->EmiVersion == EMI_VERSION_V2) {

        NT_ASSERT(PdoData->ChannelCount > 0);

        MetadataSize = FIELD_OFFSET(EMI_METADATA_V2, Channels);
        ChannelCount = PdoData->ChannelCount;
        ChannelData = &PdoData->ChannelData[0];
        while (ChannelCount != 0) {
            EntrySize =
                EMI_CHANNEL_V2_LENGTH(ChannelData->Info.ChannelNameSize);

            Status = RtlULongAdd(MetadataSize, EntrySize, &MetadataSize);
            if (!NT_SUCCESS(Status)) {
                goto PdoCalculateMetadataSizeEnd;
            }

            ChannelData = SIM_EMI_CHANNEL_DATA_NEXT_CHANNEL_DATA(ChannelData);
            ChannelCount -= 1;
        }

    } else {
        Status = STATUS_INVALID_DEVICE_STATE;
        goto PdoCalculateMetadataSizeEnd;
    }

    *MetadataSizeOut = MetadataSize;
    Status = STATUS_SUCCESS;

PdoCalculateMetadataSizeEnd:
    return Status;
}

VOID
SimEmiPdoControl (
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
    )

/*++

Routine Description:

    This routine handles device control requests for an EMI child device.

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
    EMI_CHANNEL_MEASUREMENT_DATA *EmiMeasurementDataBuffer;
    ULONG EmiMeasurementDataBufferSize;
    PVOID EmiMetadataBuffer;
    EMI_METADATA_SIZE *EmiMetadataSizeBuffer;
    EMI_VERSION *EmiVersionBuffer;
    ULONG MetadataSize;
    PSIM_EMI_BUS_PDO_DATA PdoData;
    ULONG OutputSize;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    Device = WdfIoQueueGetDevice(Queue);
    PdoData = SimEmiGetPdoData(Device);
    ExAcquireFastMutex(PdoData->BusMutex);
    OutputSize = 0;
    switch (IoControlCode) {
    case IOCTL_EMI_GET_VERSION:
        Status = WdfRequestRetrieveOutputBuffer(Request,
                                                sizeof(EMI_VERSION),
                                                (PVOID*)&EmiVersionBuffer,
                                                NULL);

        if (!NT_SUCCESS(Status)) {
            goto PdoControlEnd;
        }

        EmiVersionBuffer->EmiVersion = PdoData->EmiVersion;
        OutputSize = sizeof(EMI_VERSION);
        Status = STATUS_SUCCESS;
        break;

    case IOCTL_EMI_GET_METADATA_SIZE:
        Status = WdfRequestRetrieveOutputBuffer(Request,
                                                sizeof(EMI_METADATA_SIZE),
                                                (PVOID*)&EmiMetadataSizeBuffer,
                                                NULL);

        if (!NT_SUCCESS(Status)) {
            goto PdoControlEnd;
        }

        Status = SimEmiPdoCalculateMetadataSize(PdoData, &MetadataSize);
        if (!NT_SUCCESS(Status)) {
            goto PdoControlEnd;
        }

        EmiMetadataSizeBuffer->MetadataSize = MetadataSize;
        OutputSize = sizeof(EMI_METADATA_SIZE);
        Status = STATUS_SUCCESS;
        break;

    case IOCTL_EMI_GET_METADATA:
        Status = SimEmiPdoCalculateMetadataSize(PdoData, &MetadataSize);
        if (!NT_SUCCESS(Status)) {
            goto PdoControlEnd;
        }

        Status = WdfRequestRetrieveOutputBuffer(Request,
                                                MetadataSize,
                                                &EmiMetadataBuffer,
                                                NULL);

        if (!NT_SUCCESS(Status)) {
            goto PdoControlEnd;
        }

        Status = SimEmiPdoCopyDeviceMetadata(PdoData, EmiMetadataBuffer);
        if (!NT_SUCCESS(Status)) {
            goto PdoControlEnd;
        }

        OutputSize = MetadataSize;
        Status = STATUS_SUCCESS;
        break;

    case IOCTL_EMI_GET_MEASUREMENT:
        Status = RtlULongMult(sizeof(EMI_CHANNEL_MEASUREMENT_DATA),
                              PdoData->ChannelCount,
                              &EmiMeasurementDataBufferSize);

        if (!NT_SUCCESS(Status)) {
            goto PdoControlEnd;
        }

        Status = WdfRequestRetrieveOutputBuffer(
                    Request,
                    EmiMeasurementDataBufferSize,
                    (PVOID*)&EmiMeasurementDataBuffer,
                    NULL);

        if (!NT_SUCCESS(Status)) {
            goto PdoControlEnd;
        }

        Status = SimEmiPdoCopyChannelMeasurements(PdoData,
                                                  EmiMeasurementDataBuffer);

        if (!NT_SUCCESS(Status)) {
            goto PdoControlEnd;
        }

        OutputSize = EmiMeasurementDataBufferSize;
        Status = STATUS_SUCCESS;
        break;

    default:
        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

PdoControlEnd:
    WdfRequestCompleteWithInformation(Request, Status, OutputSize);
    ExReleaseFastMutex(PdoData->BusMutex);
    return;
}

NTSTATUS
SimEmiPdoCopyChannelMeasurements (
    _In_ PSIM_EMI_BUS_PDO_DATA PdoData,
    _Out_ EMI_CHANNEL_MEASUREMENT_DATA *MeasurementData
    )

/*++

Routine Description:

    This routine copies the channel measurement data for each channel in this
    device to the provided output buffer.

Arguments:

    PdoData - Supplies a pointer to the child device's PDO data.

    MeasurementData - Supplies a pointer to an array of EMI measurement data
        instances in which to place each channels measurement data.

Return Value:

    NTSTATUS.

--*/
{

    USHORT ChannelCount;
    PSIM_EMI_CHANNEL_DATA ChannelData;
    PSIM_EMI_BUS_CHANNEL_INFO ChannelInfo;
    ULONG64 CurrentTime;
    ULONG64 EnergyGain;
    ULONG64 PollTimeDelta;
    NTSTATUS Status;

    NT_ASSERT((PdoData->EmiVersion == EMI_VERSION_V1) ||
              (PdoData->EmiVersion == EMI_VERSION_V2));

    if ((PdoData->EmiVersion == EMI_VERSION_V1) ||
        (PdoData->EmiVersion == EMI_VERSION_V2)) {

        ChannelCount = PdoData->ChannelCount;
        ChannelData = &PdoData->ChannelData[0];
        CurrentTime = KeQueryInterruptTime();
        while (ChannelCount != 0) {
            ChannelInfo = &ChannelData->Info;

            //
            // Update channel measurement.
            //

            NT_ASSERT(CurrentTime >= ChannelData->LastPollTime);

            PollTimeDelta = CurrentTime - ChannelData->LastPollTime;
            EnergyGain = PollTimeDelta * ChannelInfo->AbsoluteEnergyRate;
            ChannelData->LastAbsoluteEnergy += EnergyGain;
            ChannelData->LastPollTime = CurrentTime;

            //
            // Copy channel measurement.
            //

            MeasurementData->AbsoluteEnergy = ChannelData->LastAbsoluteEnergy;
            MeasurementData->AbsoluteTime = ChannelData->LastPollTime;

            //
            // Move to next channel.
            //

            ChannelData = SIM_EMI_CHANNEL_DATA_NEXT_CHANNEL_DATA(ChannelData);
            MeasurementData += 1;
            ChannelCount -= 1;
        }

        Status = STATUS_SUCCESS;

    } else {
        Status = STATUS_INVALID_DEVICE_STATE;
        goto PdoCopyChannelMeasurementsEnd;
    }

PdoCopyChannelMeasurementsEnd:
    return Status;
}


NTSTATUS
SimEmiPdoCopyDeviceMetadata (
    _In_ PSIM_EMI_BUS_PDO_DATA PdoData,
    _Out_ PVOID MetadataBuffer
    )

/*++

Routine Description:

    This routine copies the child device's EMI metadata to the provided output
    buffer.

Arguments:

    PdoData - Supplies a pointer to the child device's PDO data.

    MetadataBuffer - Supplies a pointer in which to place the child device's
        EMI metadata.

Return Value:

    NTSTATUS.

--*/

{

    USHORT ChannelCount;
    PSIM_EMI_CHANNEL_DATA ChannelData;
    PSIM_EMI_BUS_CHANNEL_INFO ChannelInfo;
    EMI_CHANNEL_V2 *EmiV2Channel;
    NTSTATUS Status;
    EMI_METADATA_V1 *V1Buffer;
    EMI_METADATA_V2 *V2Buffer;

    NT_ASSERT((PdoData->EmiVersion == EMI_VERSION_V1) ||
              (PdoData->EmiVersion == EMI_VERSION_V2));

    if (PdoData->EmiVersion == EMI_VERSION_V1) {

        NT_ASSERT(PdoData->ChannelCount == 1);

        V1Buffer = (EMI_METADATA_V1*)MetadataBuffer;
        ChannelData = &PdoData->ChannelData[0];
        ChannelInfo = &ChannelData->Info;
        V1Buffer->MeasurementUnit = ChannelInfo->MeasurementUnit;
        RtlStringCchCopyW(&V1Buffer->HardwareOEM[0],
                          EMI_NAME_MAX,
                          &SimEmiHardwareOEM[0]);

        RtlStringCchCopyW(&V1Buffer->HardwareModel[0],
                          EMI_NAME_MAX,
                          &SimEmiHardwareModelV1[0]);

        V1Buffer->HardwareRevision = SimEmiHardwareRevisionV1;
        V1Buffer->MeteredHardwareNameSize = ChannelInfo->ChannelNameSize;
        RtlCopyMemory(&V1Buffer->MeteredHardwareName[0],
                      &ChannelInfo->ChannelName[0],
                      ChannelInfo->ChannelNameSize);

    } else if (PdoData->EmiVersion == EMI_VERSION_V2) {

        NT_ASSERT(PdoData->ChannelCount > 0);

        V2Buffer = (EMI_METADATA_V2*)MetadataBuffer;
        RtlStringCchCopyW(&V2Buffer->HardwareOEM[0],
                          EMI_NAME_MAX,
                          &SimEmiHardwareOEM[0]);

        RtlStringCchCopyW(&V2Buffer->HardwareModel[0],
                          EMI_NAME_MAX,
                          &SimEmiHardwareModelV1[0]);

        V2Buffer->HardwareRevision = SimEmiHardwareRevisionV1;
        V2Buffer->ChannelCount = PdoData->ChannelCount;

        //
        // Copy channel information.
        //

        ChannelCount = PdoData->ChannelCount;
        ChannelData = &PdoData->ChannelData[0];
        EmiV2Channel = &V2Buffer->Channels[0];
        while (ChannelCount != 0) {
            ChannelInfo = &ChannelData->Info;
            EmiV2Channel->MeasurementUnit = ChannelInfo->MeasurementUnit;
            EmiV2Channel->ChannelNameSize = ChannelInfo->ChannelNameSize;
            RtlCopyMemory(&EmiV2Channel->ChannelName[0],
                          &ChannelInfo->ChannelName[0],
                          ChannelInfo->ChannelNameSize);

            ChannelData = SIM_EMI_CHANNEL_DATA_NEXT_CHANNEL_DATA(ChannelData);
            EmiV2Channel = EMI_CHANNEL_V2_NEXT_CHANNEL(EmiV2Channel);
            ChannelCount -= 1;
        }

    } else {
        Status = STATUS_INVALID_DEVICE_STATE;
        goto PdoCopyDeviceMetadataEnd;
    }

    Status = STATUS_SUCCESS;

PdoCopyDeviceMetadataEnd:
    return Status;
}

NTSTATUS
SimEmiPdoCreateDevice (
    _In_ WDFCHILDLIST ChildList,
    _In_ PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER IdentificationDescription,
    _In_ PWDFDEVICE_INIT ChildInit
    )

/*++

Routine Description:

    This routine creates the PDO for an EMI child device.

Arguments:

    ChildList - Supplies a reference to a WDFCHILDLIST in which this device is
        being added.

    IdentificationDescription - Supplies a pointer to the identification info
        instance for this newly created child device.

    ChildInit - Supplies a pointer to a WDFDEVICE_INIT instance for this child
        device's PDO.

Return Value:

    NTSTATUS.

--*/

{

    UNICODE_STRING Buffer;
    WCHAR BufferStore[256];
    WDFDEVICE FdoDevice;
    PSIM_EMI_BUS_FDO_DATA FdoData;
    PSIM_EMI_BUS_PDO_IDENTIFICATION_INFO Identification;
    WDF_OBJECT_ATTRIBUTES PdoAttributes;
    PSIM_EMI_BUS_PDO_DATA PdoData;
    ULONG PdoDataSize;
    WDFDEVICE PdoDevice;
    WDFQUEUE PdoQueue;
    WDF_IO_QUEUE_CONFIG PdoQueueConfig;
    NTSTATUS Status;

    DECLARE_CONST_UNICODE_STRING(DeviceLocation, L"SimEmi Bus 0");
    DECLARE_CONST_UNICODE_STRING(DeviceSddl, L"D:P(A;;GA;;;AU)(A;;GA;;;S-1-15-2-1)");

    FdoDevice = WdfChildListGetDevice(ChildList);
    FdoData = SimEmiGetFdoData(FdoDevice);
    Identification =
        (PSIM_EMI_BUS_PDO_IDENTIFICATION_INFO)IdentificationDescription;

    KdPrint(("Creating Sim Emi PDO."));

    //
    // Calculate the size of the required PDO data structure.
    //

    Status = SimEmiPdoCalculateDataSize(Identification->ChannelCount,
                                        &Identification->ChannelInfo[0],
                                        &PdoDataSize);

    if (!NT_SUCCESS(Status)) {
        KdPrint(("SimEmiPdoCalculateDataSize failed with status code 0x%08X.\n",
                 Status));

        goto PdoCreateDeviceEnd;
    }

    //
    // Initialize the pdo device attributes and create the device.
    //

    WdfDeviceInitSetIoType(ChildInit, WdfDeviceIoBuffered);
    WdfDeviceInitSetDeviceType(ChildInit, FILE_DEVICE_BUS_EXTENDER);

    //
    // Setup ids for device.
    //

    Buffer.Buffer = &BufferStore[0];
    Buffer.Length = 0;
    Buffer.MaximumLength = sizeof(BufferStore);
    Status = RtlUnicodeStringPrintf(&Buffer,
                                    L"simemibus\\%08X",
                                    Identification->ChildDeviceHandle);

    if (!NT_SUCCESS(Status)) {
        KdPrint(("RtlUnicodeStringPrintf failed with status code 0x%08X.\n",
                 Status));

        goto PdoCreateDeviceEnd;
    }

    Status = WdfPdoInitAddHardwareID(ChildInit, &Buffer);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("WdfPdoInitAddHardwareID failed with status code 0x%08X.\n",
                 Status));

        goto PdoCreateDeviceEnd;
    }

    Status = WdfPdoInitAssignDeviceID(ChildInit, &Buffer);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("WdfPdoInitAssignDeviceID failed with status code 0x%08X.\n",
                 Status));

        goto PdoCreateDeviceEnd;
    }

    Status = RtlUnicodeStringPrintf(&Buffer,
                                    L"%08X",
                                    Identification->ChildDeviceHandle);

    if (!NT_SUCCESS(Status)) {
        KdPrint(("RtlUnicodeStringPrintf failed with status code 0x%08X.\n",
                 Status));

        goto PdoCreateDeviceEnd;
    }

    Status = WdfPdoInitAssignInstanceID(ChildInit, &Buffer);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("WdfPdoInitAssignInstanceID failed with status code 0x%08X.\n",
                 Status));

        goto PdoCreateDeviceEnd;
    }

    Status = RtlUnicodeStringPrintf(&Buffer,
                                    L"SimEmi V%d Child Device %08X: %d Channels",
                                    Identification->EmiVersion,
                                    Identification->ChildDeviceHandle,
                                    Identification->ChannelCount);

    if (!NT_SUCCESS(Status)) {
        KdPrint(("RtlUnicodeStringPrintf failed with status code 0x%08X.\n",
                 Status));

        goto PdoCreateDeviceEnd;
    }

    Status = WdfPdoInitAddDeviceText(ChildInit,
                                     &Buffer,
                                     &DeviceLocation,
                                     0x409);

    if (!NT_SUCCESS(Status)) {
        KdPrint(("WdfPdoInitAddDeviceText failed with status code 0x%08X.\n",
                 Status));

        goto PdoCreateDeviceEnd;
    }

    WdfPdoInitSetDefaultLocale(ChildInit, 0x409);
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&PdoAttributes,
                                            SIM_EMI_BUS_PDO_DATA);

    PdoAttributes.ContextSizeOverride = PdoDataSize;
    PdoAttributes.EvtDestroyCallback = &SimEmiPdoDestroyDevice;
    Status = WdfPdoInitAssignRawDevice(ChildInit, &GUID_DEVCLASS_SENSOR);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("WdfPdoInitAssignRawDevice failed with status code 0x%08X.\n",
                 Status));

        goto PdoCreateDeviceEnd;
    }

    Status = WdfDeviceInitAssignSDDLString(ChildInit, &DeviceSddl);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("WdfDeviceInitAssignSDDLString failed with status code 0x%08X.\n",
            Status));

        goto PdoCreateDeviceEnd;
    }

    Status = WdfDeviceCreate(&ChildInit, &PdoAttributes, &PdoDevice);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("WdfDeviceCreate failed with status code 0x%08X.\n", Status));
        goto PdoCreateDeviceEnd;
    }

    //
    // Initialize the PDO device data.
    //

    PdoData = SimEmiGetPdoData(PdoDevice);
    PdoData->BusMutex = &FdoData->BusMutex;
    SimEmiPdoInitializePdoData(Identification, PdoData);

    //
    // Initialize IO Queue.
    //

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&PdoQueueConfig,
                                           WdfIoQueueDispatchSequential);

    PdoQueueConfig.EvtIoDeviceControl = &SimEmiPdoControl;
    Status = WdfIoQueueCreate(PdoDevice,
                              &PdoQueueConfig,
                              WDF_NO_OBJECT_ATTRIBUTES,
                              &PdoQueue);

    if (!NT_SUCCESS(Status)) {
        KdPrint(("WdfIoQueueCreate failed with status code 0x%08X.\n", Status));
        goto PdoCreateDeviceEnd;
    }

    Status = WdfDeviceCreateDeviceInterface(PdoDevice,
                                            &GUID_DEVICE_ENERGY_METER,
                                            NULL);

    if (!NT_SUCCESS(Status)) {
        KdPrint(("WdfDeviceCreateDeviceInterface failed with status code 0x%08X.\n",
                 Status));

        goto PdoCreateDeviceEnd;
    }

    ExAcquireFastMutex(&FdoData->BusMutex);
    InsertTailList(&FdoData->ChildDevices, &PdoData->Link);
    ExReleaseFastMutex(&FdoData->BusMutex);
    Status = STATUS_SUCCESS;

PdoCreateDeviceEnd:
    return Status;
}

VOID
SimEmiPdoDestroyDevice (
    _In_ WDFOBJECT Object
    )

/*++

Routine Description:

    This routine cleans up the resources owned by a child device's PDO.

Arguments:

    Object - Supplies a reference to the child device's PDO device.

Return Value:

    None.

--*/

{

    PSIM_EMI_BUS_PDO_DATA PdoData;

    PdoData = SimEmiGetPdoData(Object);
    ExAcquireFastMutex(PdoData->BusMutex);
    RemoveEntryList(&PdoData->Link);
    ExReleaseFastMutex(PdoData->BusMutex);
    return;
}

VOID
SimEmiPdoInitializePdoData (
    _In_ PSIM_EMI_BUS_PDO_IDENTIFICATION_INFO Identification,
    _Out_ PSIM_EMI_BUS_PDO_DATA PdoData
    )

/*++

Routine Description:

    This routine initializes a child device's PDO data instance.

Arguments:

    Identification - Supplies a pointer to this child device's identification
        info.

    PdoData - Supplies a pointer to this child device's PDO data instance.

Return Value:

    None.

--*/

{

    USHORT ChannelCount;
    PSIM_EMI_CHANNEL_DATA ChannelData;
    PSIM_EMI_BUS_CHANNEL_INFO ChannelInfo;
    ULONG ChannelInfoSize;
    ULONG64 LastPollTime;

    PdoData->EmiVersion = Identification->EmiVersion;
    PdoData->ChildDeviceHandle = Identification->ChildDeviceHandle;
    PdoData->ChannelCount = Identification->ChannelCount;

    //
    // Initialize the per channel data.
    //

    LastPollTime = KeQueryInterruptTime();
    ChannelCount = Identification->ChannelCount;
    ChannelData = &PdoData->ChannelData[0];
    ChannelInfo = &Identification->ChannelInfo[0];
    while (ChannelCount != 0) {
        ChannelData->LastPollTime = LastPollTime;
        ChannelData->LastAbsoluteEnergy = 0;
        ChannelInfoSize =
            SIM_EMI_BUS_CHANNEL_INFO_SIZE(ChannelInfo->ChannelNameSize);

        RtlCopyMemory(&ChannelData->Info, ChannelInfo, ChannelInfoSize);
        ChannelInfo = SIM_EMI_BUS_CHANNEL_INFO_NEXT_CHANNEL_INFO(ChannelInfo);
        ChannelData = SIM_EMI_CHANNEL_DATA_NEXT_CHANNEL_DATA(ChannelData);
        ChannelCount -= 1;
    }

    return;
}
