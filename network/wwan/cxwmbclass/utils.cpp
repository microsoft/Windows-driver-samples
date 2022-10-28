//
//    Copyright (C) Microsoft.  All rights reserved.
//
#include <precomp.h>
#include "util.h"
NTSTATUS
SendSyncControlCommand(
    __in WDFUSBDEVICE WdfUsbDevice,
    __in WDF_USB_BMREQUEST_DIRECTION Direction,
    __in WDF_USB_BMREQUEST_RECIPIENT Recipient,
    __in BYTE Request,
    __in USHORT Value,
    __inout_bcount_opt(BufferLength) PUCHAR Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG BytesTransfered)

{

    NTSTATUS Status;
    WDF_USB_CONTROL_SETUP_PACKET controlSetupPacket;
    WDF_MEMORY_DESCRIPTOR memoryDescriptor;
    WDF_REQUEST_SEND_OPTIONS SendOptions;
    PUSB_DEVICE_CONTEXT usbDeviceContext = NULL;

    usbDeviceContext = GetUsbDeviceContext(WdfUsbDevice);

    if (Buffer != NULL)
    {

        WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&memoryDescriptor, (PVOID)Buffer, BufferLength);
    }

    WDF_USB_CONTROL_SETUP_PACKET_INIT_CLASS(&controlSetupPacket, Direction, Recipient, Request, Value, usbDeviceContext->UsbCommunicationInterfaceIndex);

    WDF_REQUEST_SEND_OPTIONS_INIT(&SendOptions, 0);

    WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&SendOptions, WDF_REL_TIMEOUT_IN_SEC(10));

    Status = WdfUsbTargetDeviceSendControlTransferSynchronously(
        WdfUsbDevice, WDF_NO_HANDLE, &SendOptions, &controlSetupPacket, (Buffer != NULL) ? &memoryDescriptor : NULL, BytesTransfered);

    if (!NT_SUCCESS(Status))
    {
        BOOLEAN ToHost = Direction == BmRequestDeviceToHost;
    }

    return Status;
}

NTSTATUS
TransactControlChannel(
    __in PBUS_OBJECT BusObject,
    __in ULONG TimeOut,
    __in_bcount_opt(InBufferLength) PUCHAR InBuffer,
    __in ULONG InBufferLength,
    __out_bcount_opt(OutBufferLength) PUCHAR OutBuffer,
    __in ULONG OutBufferLength,
    __out PULONG BytesRead)

{
    ULONG BytesTransfered = 0;
    NTSTATUS Status;

    *BytesRead = 0;

    Status = SendSyncControlCommand(
        BusObject->WdfUsbDevice, BmRequestHostToDevice, BmRequestToInterface, SEND_ENCAPSULATE_COMMAND, 0, InBuffer, InBufferLength, &BytesTransfered);

    if (NT_SUCCESS(Status))
    {
        if (BytesTransfered < InBufferLength)
        {
            Status = STATUS_INFO_LENGTH_MISMATCH;
        }
    }

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = WaitForResponseAvailible(BusObject, TimeOut);

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = SendSyncControlCommand(
        BusObject->WdfUsbDevice, BmRequestDeviceToHost, BmRequestToInterface, GET_ENCAPSULATE_RESPONSE, 0, OutBuffer, OutBufferLength, &BytesTransfered);

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    *BytesRead = BytesTransfered;

Cleanup:

    return Status;
}

NTSTATUS
ReadInterrupt(_In_ PBUS_OBJECT BusObject, _Inout_updates_bytes_(BufferLength) PUCHAR Buffer, _In_ ULONG BufferLength, _In_ ULONG Timeout, _Out_ PULONG BytesRead)

{

    NTSTATUS status;
    WDFMEMORY memHandle = NULL;

    WDFREQUEST request = NULL;
    BOOLEAN SentToDevice = FALSE;
    WDF_REQUEST_SEND_OPTIONS SendOptions;
    PUSB_DEVICE_CONTEXT usbDeviceContext = NULL;
    WDF_MEMORY_DESCRIPTOR MemoryDescriptor;

    usbDeviceContext = GetUsbDeviceContext(BusObject->WdfUsbDevice);

    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&MemoryDescriptor, (PVOID)Buffer, BufferLength);

    WDF_REQUEST_SEND_OPTIONS_INIT(&SendOptions, WDF_REQUEST_SEND_OPTION_IGNORE_TARGET_STATE);

    WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&SendOptions, WDF_REL_TIMEOUT_IN_SEC(Timeout));

    status = WdfUsbTargetPipeReadSynchronously(usbDeviceContext->InterruptPipe, NULL, &SendOptions, &MemoryDescriptor, BytesRead);

    return status;
}

NTSTATUS
GetDeviceString(__in WDFUSBDEVICE UsbDevice, __in UCHAR Index, __out PWSTR* String)

{
    NTSTATUS Status;
    WDF_REQUEST_SEND_OPTIONS SendOptions;
    USHORT CharacterCount = 0;
    ULONG AllocationSize = 0;
    PWSTR AllocatedString = NULL;

    WDF_REQUEST_SEND_OPTIONS_INIT(&SendOptions, 0);

    WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&SendOptions, WDF_REL_TIMEOUT_IN_SEC(DEFAULT_IO_TIMEOUT));

    *String = NULL;

    Status = WdfUsbTargetDeviceQueryString(UsbDevice, NULL, &SendOptions, NULL, &CharacterCount, Index, 0);

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    //  allocate one more char to make sure sting is null terminated
    //
    AllocationSize = (CharacterCount + 1) * sizeof(WCHAR);

    AllocatedString = (PWSTR)ALLOCATE_PAGED_POOL(AllocationSize);

    if (AllocatedString == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    RtlZeroMemory(AllocatedString, AllocationSize);

    Status = WdfUsbTargetDeviceQueryString(UsbDevice, NULL, &SendOptions, AllocatedString, &CharacterCount, Index, 0);

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    *String = AllocatedString;
    AllocatedString = NULL;

Cleanup:

    if (AllocatedString != NULL)
    {
        FREE_POOL(AllocatedString);
        AllocatedString = NULL;
    }

    return Status;
}

NTSTATUS
MbbBusSetPacketFilter(__in MBB_BUS_HANDLE BusHandle, __in ULONG PacketFilter)

{

    PBUS_OBJECT BusObject = (PBUS_OBJECT)BusHandle;
    NTSTATUS Status;
    ULONG BytesTransfered = 0;
    UCHAR ReadBuffer[256];

    Status = SendSyncControlCommand(
        BusObject->WdfUsbDevice, BmRequestHostToDevice, BmRequestToInterface, SET_PACKET_FILTER, 0, (PUCHAR)&PacketFilter, sizeof(PacketFilter), &BytesTransfered);

    if (NT_SUCCESS(Status))
    {
        if (BytesTransfered < sizeof(PacketFilter))
        {
            Status = STATUS_INFO_LENGTH_MISMATCH;
        }
    }

    return Status;
}

NTSTATUS
MbbBusSetNtbInSize(__in MBB_BUS_HANDLE BusHandle, __in ULONG InSize)

{

    PBUS_OBJECT BusObject = (PBUS_OBJECT)BusHandle;
    NTSTATUS Status;
    ULONG BytesTransfered = 0;

    Status = SendSyncControlCommand(
        BusObject->WdfUsbDevice, BmRequestHostToDevice, BmRequestToInterface, SET_NTB_INPUT_SIZE, 0, (PUCHAR)&InSize, sizeof(InSize), &BytesTransfered);

    if (NT_SUCCESS(Status))
    {
        if (BytesTransfered < sizeof(InSize))
        {
            Status = STATUS_INFO_LENGTH_MISMATCH;
        }
    }

    return Status;
}

NTSTATUS
MbbBusGetStat(__in MBB_BUS_HANDLE BusHandle, __in USHORT StatIndex, __out ULONGLONG* Value)

{
    PBUS_OBJECT BusObject = (PBUS_OBJECT)BusHandle;
    NTSTATUS Status;
    ULONG BytesTransfered = 0;

#pragma warning(suppress : 6001)
    Status = SendSyncControlCommand(
        BusObject->WdfUsbDevice, BmRequestDeviceToHost, BmRequestToInterface, GET_STATISTIC, StatIndex, (PUCHAR)Value, sizeof(*Value), &BytesTransfered);

    if (NT_SUCCESS(Status))
    {
        if (BytesTransfered < sizeof(Value))
        {
            Status = STATUS_INFO_LENGTH_MISMATCH;
        }
    }

    return Status;
}

NTSTATUS
MbbBusSetNtbFormat(__in MBB_BUS_HANDLE BusHandle, __in USHORT Format)

{

    PBUS_OBJECT BusObject = (PBUS_OBJECT)BusHandle;
    NTSTATUS Status;
    ULONG BytesTransfered = 0;

    Status = SendSyncControlCommand(
        BusObject->WdfUsbDevice, BmRequestHostToDevice, BmRequestToInterface, SET_NBT_FORMAT, Format, (PUCHAR)NULL, 0, &BytesTransfered);

    if (NT_SUCCESS(Status))
    {
    }

    return Status;
}

NTSTATUS
MbbBusResetFunction(__in MBB_BUS_HANDLE BusHandle)

{

    PBUS_OBJECT BusObject = (PBUS_OBJECT)BusHandle;
    NTSTATUS Status;
    ULONG BytesTransfered = 0;

    Status = SendSyncControlCommand(
        BusObject->WdfUsbDevice, BmRequestHostToDevice, BmRequestToInterface, RESET_FUNCTION, 0, (PUCHAR)NULL, 0, &BytesTransfered);

    if (NT_SUCCESS(Status))
    {
    }

    return Status;
}

NTSTATUS
MbbBusResetDeviceAndSetParms(__in PBUS_OBJECT BusObject)

{
    NTSTATUS Status;
    USHORT NtbFormat = NCM_SET_NTB_FORMAT_16_BIT;

    //
    //  put the device back into a known state
    //
    Status = MbbBusResetFunction(BusObject);

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = MbbBusSetNtbInSize(BusObject, BusObject->MaxBulkInTransfer);

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    if ((BusObject->NtbParam.bmNtbFormatSupported & NCM_NTB_FORMAT_32_BIT) == NCM_NTB_FORMAT_32_BIT)
    {
        //
        //  device supports 32 bit mode
        //
        if (BusObject->NtbFormat32Bit)
        {
            //
            //  the device can actually handle a transfer larger than 16bit, change to 32 bit
            //
            NtbFormat = NCM_SET_NTB_FORMAT_32_BIT;
        }

        //
        //  set the format
        //
        Status = MbbBusSetNtbFormat(BusObject, NtbFormat);

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

Cleanup:

    return Status;
}

ULONG
ProcessInterruptPipeRead(
    PBUS_OBJECT BusObject,
    __in_ecount(NewFragmentLength) PCUCHAR NewFragment,
    ULONG NewFragmentLength,
    __out_ecount(MessageBufferSize) PUCHAR CompleteMessageBuffer,
    ULONG MessageBufferSize)

{

    ULONG CompleteMessageLength = 0;
    PUSB_CDC_NOTIFICATION CdcNotification = NULL;

    ASSERT(MessageBufferSize == sizeof(BusObject->InterruptReassemnblyBuffer));

    if (BusObject->CurrentOffset == 0)
    {
        //
        //  beggining of interrupt transfer
        //
        if ((NewFragmentLength >= sizeof(*CdcNotification)) && (NewFragmentLength <= sizeof(BusObject->InterruptReassemnblyBuffer)))
        {
            //
            //  big enough for a notification
            //
            CdcNotification = (PUSB_CDC_NOTIFICATION)NewFragment;

            if ((CdcNotification->bmRequestType == 0xa1))
            {
                BusObject->ExpectLength = sizeof(*CdcNotification) + CdcNotification->wLength;

                RtlCopyMemory(&BusObject->InterruptReassemnblyBuffer[BusObject->CurrentOffset], NewFragment, NewFragmentLength);

                BusObject->CurrentOffset += NewFragmentLength;
            }
            else
            {
                //
                //  wrong request type, drop
                //
            }
        }
        else
        {
            //
            //  not big enough to be a cdc notification, drop
            //
        }
    }
    else
    {
        //
        //  we already have some of the data
        //
        if ((NewFragmentLength + BusObject->CurrentOffset) <= sizeof(BusObject->InterruptReassemnblyBuffer))
        {
            //
            //  still room in the NewFragment
            //
            RtlCopyMemory(&BusObject->InterruptReassemnblyBuffer[BusObject->CurrentOffset], NewFragment, NewFragmentLength);
        }
        else
        {
            //
            //  over flow, Keep procssing until we get the whole thing so we don't get out of sync
            //
        }
        BusObject->CurrentOffset += NewFragmentLength;
    }

    if (BusObject->CurrentOffset < BusObject->ExpectLength)
    {
        //
        //  we don't have the whole thing yet, keep going
        //
        if (NewFragmentLength == 0)
        {
            //
            //  got a zero length transfer, reset the re-assembly state
            BusObject->ExpectLength = 0;
            BusObject->CurrentOffset = 0;
        }
    }
    else
    {
        if (BusObject->CurrentOffset == BusObject->ExpectLength)
        {
            //
            //  got it all
            //
            if ((BusObject->CurrentOffset <= sizeof(BusObject->InterruptReassemnblyBuffer)) && (BusObject->CurrentOffset <= MessageBufferSize))
            {
                //
                //  the whole thing fit in the NewFragment
                //
                RtlCopyMemory(CompleteMessageBuffer, BusObject->InterruptReassemnblyBuffer, BusObject->CurrentOffset);

                CompleteMessageLength = BusObject->CurrentOffset;
                BusObject->CurrentOffset = 0;
            }
            else
            {
                //
                //  it is bigger than the re-assembly NewFragment,
                //  drop it
                //
            }

            //
            //  done with the current re-assembled NewFragment
            //
            BusObject->ExpectLength = 0;
            BusObject->CurrentOffset = 0;
        }
        else
        {
            //
            //  current offset is beyond the what we expected
            //
            ASSERT(0);
        }
    }

    return CompleteMessageLength;
}

NTSTATUS
WaitForResponseAvailible(PBUS_OBJECT BusObject, ULONG TimeOut)

{
    NTSTATUS Status;
    UCHAR IndicateBuffer[INTERRUPT_REASSEMBLY_BUFFER_SIZE];
    ULONG BufferLength = 0;
    ULONG BytesTransfered = 0;
    BOOLEAN ExitLoop = FALSE;
    PUSB_DEVICE_CONTEXT usbDeviceContext = NULL;
    PUSB_CDC_NOTIFICATION Notification = NULL;

    usbDeviceContext = GetUsbDeviceContext(BusObject->WdfUsbDevice);

    BusObject->ExpectLength = 0;
    BusObject->CurrentOffset = 0;

    while (!ExitLoop)
    {

        Status = ReadInterrupt(BusObject, BusObject->SyncInterruptReadBuffer, usbDeviceContext->InterruptPipeMaxPacket, TimeOut, &BytesTransfered);

        if (NT_SUCCESS(Status))
        {

            BufferLength = ProcessInterruptPipeRead(
                BusObject, BusObject->SyncInterruptReadBuffer, BytesTransfered, IndicateBuffer, sizeof(IndicateBuffer));

            if (BufferLength == 0)
            {
                //
                //
                //   don't have a complete message yet
                //
                continue;
            }
            else
            {
                if (BufferLength < sizeof(*Notification))
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    goto Cleanup;
                }

                Notification = (PUSB_CDC_NOTIFICATION)IndicateBuffer;

                if (Notification->bNotificationCode != USB_CDC_NOTIFICATION_RESPONSE_AVAILABLE)
                {
                    continue;
                }
            }
        }
        else
        {
            goto Cleanup;
        }

        ExitLoop = TRUE;
    }

    Status = STATUS_SUCCESS;

Cleanup:

    return Status;
}

BOOLEAN IsUsbCapDeviceInfoValid(_In_ USB_CAP_DEVICE_INFO UsbCapDeviceInfo, _In_ ULONG CapResultLength)
{
    if (CapResultLength < sizeof(USB_CAP_DEVICE_INFO_HEADER))
    {
        return FALSE;
    }
    switch (UsbCapDeviceInfo.DeviceInfoHeader.DeviceType)
    {
    case USB_CAP_DEVICE_TYPE_UDE_MBIM: __fallthrough;
    case USB_CAP_DEVICE_TYPE_UDE_MBIM_FASTIO:
        // Only support version 1.0 now
        if (UsbCapDeviceInfo.DeviceInfoHeader.DeviceMajorVersion == 0x1 && UsbCapDeviceInfo.DeviceInfoHeader.DeviceMinorVersion == 0x0)
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    default: return FALSE;
    }
}

/*++

Routine Description:

    Check if the bus type is Ude

Arguments:

    BusHandle - identifies the instance of the bus layer.

Return Value:

    TRUE - The bus type is Ude
    FALSE - The bus type isn't Ude

--*/
BOOLEAN
MbbBusIsUde(_In_ MBB_BUS_HANDLE BusHandle)
{
    USB_CAP_DEVICE_TYPE DeviceType = ((PBUS_OBJECT)BusHandle)->UsbCapDeviceInfo.DeviceInfoHeader.DeviceType;
    return (DeviceType == USB_CAP_DEVICE_TYPE_UDE_MBIM || DeviceType == USB_CAP_DEVICE_TYPE_UDE_MBIM_FASTIO);
}
