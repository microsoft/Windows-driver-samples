//
//    Copyright (C) Microsoft.  All rights reserved.
//
#pragma once
NTSTATUS
SendSyncControlCommand(
    __in WDFUSBDEVICE WdfUsbDevice,
    __in WDF_USB_BMREQUEST_DIRECTION Direction,
    __in WDF_USB_BMREQUEST_RECIPIENT Recipient,
    __in BYTE Request,
    __in USHORT Value,
    __inout_bcount_opt(BufferLength) PUCHAR Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG BytesTransfered);

NTSTATUS
TransactControlChannel(
    __in PBUS_OBJECT BusObject,
    __in ULONG TimeOut,
    __in_bcount_opt(InBufferLength) PUCHAR InBuffer,
    __in ULONG InBufferLength,
    __out_bcount_opt(OutBufferLength) PUCHAR OutBuffer,
    __in ULONG OutBufferLength,
    __out PULONG BytesRead);

NTSTATUS
ReadInterrupt(_In_ PBUS_OBJECT BusObject, _Inout_updates_bytes_(BufferLength) PUCHAR Buffer, _In_ ULONG BufferLength, _In_ ULONG Timeout, _Out_ PULONG BytesRead);

NTSTATUS
GetDeviceString(__in WDFUSBDEVICE UsbDevice, __in UCHAR Index, __out PWSTR* String);

NTSTATUS
MbbBusSetPacketFilter(__in MBB_BUS_HANDLE BusHandle, __in ULONG PacketFilter);

NTSTATUS
MbbBusSetNtbInSize(__in MBB_BUS_HANDLE BusHandle, __in ULONG InSize);

NTSTATUS
MbbBusGetStat(__in MBB_BUS_HANDLE BusHandle, __in USHORT StatIndex, __out ULONGLONG* Value);

NTSTATUS
MbbBusSetNtbFormat(__in MBB_BUS_HANDLE BusHandle, __in USHORT Format);

NTSTATUS
MbbBusResetFunction(__in MBB_BUS_HANDLE BusHandle);

ULONG
ProcessInterruptPipeRead(
    PBUS_OBJECT BusObject,
    __in_ecount(NewFragmentLength) PCUCHAR NewFragment,
    ULONG NewFragmentLength,
    __out_ecount(MessageBufferSize) PUCHAR CompleteMessageBuffer,
    ULONG MessageBufferSize);

NTSTATUS
WaitForResponseAvailible(PBUS_OBJECT BusObject, ULONG TimeOut);

NTSTATUS
MbbBusResetDeviceAndSetParms(__in PBUS_OBJECT BusObject);

BOOLEAN IsUsbCapDeviceInfoValid(_In_ USB_CAP_DEVICE_INFO UsbCapDeviceInfo, _In_ ULONG CapResultLength);
