/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Device.h

Abstract:

    Conext and function declarations for bthecho client device

Environment:

    Kernel mode only


--*/

#include "clisrv.h"

typedef struct _BTHECHOSAMPLE_CLIENT_CONTEXT
{
    //
    // Context common to client and server
    //
    BTHECHOSAMPLE_DEVICE_CONTEXT_HEADER  Header;

    //
    // Server address
    //
    BTH_ADDR                                ServerBthAddress; 

} BTHECHOSAMPLE_CLIENT_CONTEXT, *PBTHECHOSAMPLE_CLIENT_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(BTHECHOSAMPLE_CLIENT_CONTEXT, GetClientDeviceContext)

typedef struct _BTHECHOSAMPLE_CLIENT_FILE_CONTEXT
{
    //
    // Connection to server opened for this file
    //
    PBTHECHO_CONNECTION Connection;

    //
    // Server PSM
    //
    USHORT                  ServerPsm;    
} BTHECHOSAMPLE_CLIENT_FILE_CONTEXT, *PBTHECHOSAMPLE_CLIENT_FILE_CONTEXT;

//
// Context for requests
//
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(BRB, GetRequestContext);    

//
// Context for file objects
//
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(BTHECHOSAMPLE_CLIENT_FILE_CONTEXT, GetFileContext);

NTSTATUS
FORCEINLINE
BthEchoCliContextInit(
    PBTHECHOSAMPLE_CLIENT_CONTEXT context,
    WDFDEVICE Device
    )
{
    return BthEchoSharedDeviceContextHeaderInit(&context->Header, Device);
}

EVT_WDF_DRIVER_DEVICE_ADD BthEchoCliEvtDriverDeviceAdd;

EVT_WDF_DEVICE_SELF_MANAGED_IO_INIT BthEchoCliEvtDeviceSelfManagedIoInit;

EVT_WDF_DEVICE_FILE_CREATE BthEchoCliEvtDeviceFileCreate;

EVT_WDF_FILE_CLOSE BthEchoCliEvtFileClose;

NTSTATUS
BthEchoCliConnectionStateConnected(
    _In_ WDFFILEOBJECT  FileObject,
    _In_ PBTHECHO_CONNECTION Connection
    );



