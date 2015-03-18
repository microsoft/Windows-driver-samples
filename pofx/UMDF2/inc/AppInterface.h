/*++

Copyright (c) Microsoft Corporation, All Rights Reserved

Module Name:

    AppInterface.h

Abstract:

    This module contains the common declarations shared by the driver and the 
    user application for the PowerFx sample.

Environment:

    user and kernel

--*/

#if !defined(_APPINTERFACE_H_)
#define _APPINTERFACE_H_

#include <initguid.h>

//
// Component count for multi-component device
//
#define COMPONENT_COUNT         4

//
// Interface GUID for the PowerFx driver
// {D21FD4DB-5FB6-4df2-B73B-5E7F8632B390}
//
DEFINE_GUID(
    GUID_DEVINTERFACE_POWERFX,
    0xd21fd4db, 0x5fb6, 0x4df2, 0xb7, 0x3b, 0x5e, 0x7f, 0x86, 0x32, 0xb3, 0x90
    );

//
// IOCTLs that the application sends to the PowerFx driver
//
#define IOCTL_INDEX_POWERFX     0x900
#define FILE_DEVICE_POWERFX     0x9000

//
// This IOCTL is used to read a data from a component. The app initializes the 
// input buffer with the component number to read. The driver writes the data
// from that component to the output buffer.
// In this sample, the "data" for a given component is just the bit-wise 
// complement of the component number. In other words, the data for component 
// number 'X' is simply '~X'.
//
#define IOCTL_POWERFX_READ_COMPONENT    CTL_CODE(FILE_DEVICE_POWERFX,  \
                                                 IOCTL_INDEX_POWERFX,  \
                                                 METHOD_BUFFERED,      \
                                                 FILE_READ_ACCESS)

//
// Input and output buffers for IOCTL_POWERFX_READ_COMPONENT
//
typedef struct _POWERFX_READ_COMPONENT_INPUT {
    ULONG ComponentNumber;
} POWERFX_READ_COMPONENT_INPUT, *PPOWERFX_READ_COMPONENT_INPUT;

typedef struct _POWERFX_READ_COMPONENT_OUTPUT {
    ULONG ComponentData;
} POWERFX_READ_COMPONENT_OUTPUT, *PPOWERFX_READ_COMPONENT_OUTPUT;

#endif // _APPINTERFACE_H_