/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    nuiouser.h

Abstract:

    Constants and types to access the NDISPROT driver.
    Users must also include ntddndis.h

Environment:

    User/Kernel mode.

--*/

#ifndef __NUIOUSER__H
#define __NUIOUSER__H


#define FSCTL_NDISPROT_BASE      FILE_DEVICE_NETWORK

#define _NDISPROT_CTL_CODE(_Function, _Method, _Access)  \
            CTL_CODE(FSCTL_NDISPROT_BASE, _Function, _Method, _Access)

#define IOCTL_NDISPROT_OPEN_DEVICE   \
            _NDISPROT_CTL_CODE(0x200, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define IOCTL_NDISPROT_QUERY_OID_VALUE   \
            _NDISPROT_CTL_CODE(0x201, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define IOCTL_NDISPROT_SET_OID_VALUE   \
            _NDISPROT_CTL_CODE(0x205, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define IOCTL_NDISPROT_QUERY_BINDING   \
            _NDISPROT_CTL_CODE(0x203, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define IOCTL_NDISPROT_BIND_WAIT   \
            _NDISPROT_CTL_CODE(0x204, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define IOCTL_NDISPROT_INDICATE_STATUS   \
            _NDISPROT_CTL_CODE(0x206, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)



//
//  Structure to go with IOCTL_NDISPROT_QUERY_OID_VALUE.
//  The Data part is of variable length, determined by
//  the input buffer length passed to DeviceIoControl.
//
typedef struct _NDISPROT_QUERY_OID
{
    NDIS_OID        Oid;
    UCHAR           Data[sizeof(ULONG)];

} NDISPROT_QUERY_OID, *PNDISPROT_QUERY_OID;

//
//  Structure to go with IOCTL_NDISPROT_SET_OID_VALUE.
//  The Data part is of variable length, determined
//  by the input buffer length passed to DeviceIoControl.
//
typedef struct _NDISPROT_SET_OID
{
    NDIS_OID        Oid;
    UCHAR           Data[sizeof(ULONG)];

} NDISPROT_SET_OID, *PNDISPROT_SET_OID;


//
//  Structure to go with IOCTL_NDISPROT_QUERY_BINDING.
//  The input parameter is BindingIndex, which is the
//  index into the list of bindings active at the driver.
//  On successful completion, we get back a device name
//  and a device descriptor (friendly name).
//
typedef struct _NDISPROT_QUERY_BINDING
{
    ULONG            BindingIndex;        // 0-based binding number
    ULONG            DeviceNameOffset;    // from start of this struct
    ULONG            DeviceNameLength;    // in bytes
    ULONG            DeviceDescrOffset;    // from start of this struct
    ULONG            DeviceDescrLength;    // in bytes

} NDISPROT_QUERY_BINDING, *PNDISPROT_QUERY_BINDING;

//
//  Structure to go with IOCTL_NDISPROT_INDICATE_STATUS.
//  NDISPROT copies the status indicated by the NIC and
//  also the data indicated in the StatusBuffer.
//
typedef struct _NDISPROT_INDICATE_STATUS
{
    ULONG            IndicatedStatus;        // NDIS_STATUS
    ULONG            StatusBufferOffset;    // from start of this struct
    ULONG            StatusBufferLength;    // in bytes
} NDISPROT_INDICATE_STATUS, *PNDISPROT_INDICATE_STATUS;

#endif // __NUIOUSER__H


