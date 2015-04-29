/*++

Copyright (C) Microsoft Corporation, All Rights Reserved.

Module Name:

    util.cpp

Abstract:

    This module contains the implementation of the driver

Environment:

    Windows Driver Framework (WDF)

--*/

#include "vhidmini.h"

//
// First let's review Buffer Descriptions for I/O Control Codes
//
//   METHOD_BUFFERED
//    - Input buffer:  Irp->AssociatedIrp.SystemBuffer
//    - Output buffer: Irp->AssociatedIrp.SystemBuffer
//
//   METHOD_IN_DIRECT or METHOD_OUT_DIRECT
//    - Input buffer:  Irp->AssociatedIrp.SystemBuffer
//    - Second buffer: Irp->MdlAddress
//
//   METHOD_NEITHER
//    - Input buffer:  Parameters.DeviceIoControl.Type3InputBuffer;
//    - Output buffer: Irp->UserBuffer
//
// HID minidriver IOCTL stores a pointer to HID_XFER_PACKET in Irp->UserBuffer.
// For IOCTLs like IOCTL_HID_GET_FEATURE (which is METHOD_OUT_DIRECT) this is
// not the expected buffer location. So we cannot retrieve UserBuffer from the
// IRP using WdfRequestXxx functions. Instead, we have to escape to WDM.
//

NTSTATUS
RequestGetHidXferPacket_ToReadFromDevice(
    _In_  WDFREQUEST        Request,
    _Out_ HID_XFER_PACKET  *Packet
    )
{
    NTSTATUS                status;
    WDF_REQUEST_PARAMETERS  params;

    WDF_REQUEST_PARAMETERS_INIT(&params);
    WdfRequestGetParameters(Request, &params);

    if (params.Parameters.DeviceIoControl.OutputBufferLength < sizeof(HID_XFER_PACKET)) {
        status = STATUS_BUFFER_TOO_SMALL;
        KdPrint(("RequestGetHidXferPacket: invalid HID_XFER_PACKET\n"));
        return status;
    }

    RtlCopyMemory(Packet, WdfRequestWdmGetIrp(Request)->UserBuffer, sizeof(HID_XFER_PACKET));
    return STATUS_SUCCESS;
}

NTSTATUS
RequestGetHidXferPacket_ToWriteToDevice(
    _In_  WDFREQUEST        Request,
    _Out_ HID_XFER_PACKET  *Packet
    )
{
    NTSTATUS                status;
    WDF_REQUEST_PARAMETERS  params;

    WDF_REQUEST_PARAMETERS_INIT(&params);
    WdfRequestGetParameters(Request, &params);

    if (params.Parameters.DeviceIoControl.InputBufferLength < sizeof(HID_XFER_PACKET)) {
        status = STATUS_BUFFER_TOO_SMALL;
        KdPrint(("RequestGetHidXferPacket: invalid HID_XFER_PACKET\n"));
        return status;
    }

    RtlCopyMemory(Packet, WdfRequestWdmGetIrp(Request)->UserBuffer, sizeof(HID_XFER_PACKET));
    return STATUS_SUCCESS;
}
