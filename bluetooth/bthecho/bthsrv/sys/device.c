/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Device.c

Abstract:

    Device object related functionality for bthecho server device

Environment:

    Kernel mode only


--*/

#include "clisrv.h"
#include "device.h"
#include "server.h"
#include "sdp.h"
#include "echo.h"

#define INITGUID
#include "public.h"

#if defined(EVENT_TRACING)
#include "device.tmh"
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, BthEchoSrvEvtDriverDeviceAdd)
#pragma alloc_text (PAGE, BthEchoSrvEvtDeviceSelfManagedIoCleanup)
#endif

NTSTATUS
BthEchoSrvEvtDriverDeviceAdd(
    _In_ WDFDRIVER  Driver,
    _Inout_ PWDFDEVICE_INIT  DeviceInit
)
/*++
Routine Description:

    BthEchoSrvEvtDeviceAdd is called by the framework in response to AddDevice
    call from the PnP manager. We create and initialize a device object to
    represent a new instance of the device. All the software resources
    should be allocated in this callback.

Arguments:

    Driver - Handle to a framework driver object created in DriverEntry

    DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                        status;
    WDFDEVICE                       device;    
    WDF_OBJECT_ATTRIBUTES           attributes;
    WDF_PNPPOWER_EVENT_CALLBACKS    pnpPowerCallbacks;
    
    UNREFERENCED_PARAMETER(Driver);

    PAGED_CODE();

    //
    // Configure Pnp/power callbacks
    //
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);
    pnpPowerCallbacks.EvtDeviceSelfManagedIoInit = BthEchoSrvEvtDeviceSelfManagedIoInit;
    pnpPowerCallbacks.EvtDeviceSelfManagedIoCleanup = BthEchoSrvEvtDeviceSelfManagedIoCleanup;

    WdfDeviceInitSetPnpPowerEventCallbacks(
        DeviceInit,
        &pnpPowerCallbacks
        );

    //
    // Create device object with context
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, BTHECHOSAMPLE_SERVER_CONTEXT);
    
    status = WdfDeviceCreate(
        &DeviceInit,
        &attributes,
        &device
        );

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, 
            "WdfDeviceCreate failed with Status code %!STATUS!\n", status);

        goto exit;
    }

    status = BthEchoSampleServerContextInit(GetServerDeviceContext(device), device);

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, 
            "Initialization of context failed with Status code %!STATUS!\n", status);

        goto exit;       
    }

    //
    // Query for interfaces and pre-allocate BRBs
    //
    
    status = BthEchoSrvInitialize(GetServerDeviceContext(device));
    if (!NT_SUCCESS(status))
    {
        goto exit;
    }

    //
    // Not creating device interface because we don't expect apps to talk to the
    // server.
    //

exit:    
    return status;
}

NTSTATUS
BthEchoSrvEvtDeviceSelfManagedIoInit(
    _In_ WDFDEVICE  Device
    )
/*++

Description:

    This routine is called by the framework only once
    and hence we use it for our one time initialization.

    In this routine we retrieve local bth address and local stack supported
    features. We also register the server and publish SDP record.

Arguments:

    Device - Framework device object

Return Value:

    NTSTATUS Status code.

--*/
{
    NTSTATUS status;
    PBTHECHOSAMPLE_SERVER_CONTEXT devCtx = GetServerDeviceContext(Device);
    PUCHAR sdpRecordStream = NULL;
    ULONG sdpRecordLength = 0;
    BTHDDI_SDP_NODE_INTERFACE sdpNodeInterface;
    BTHDDI_SDP_PARSE_INTERFACE sdpParseInterface;

    status = BthEchoSharedRetrieveLocalInfo(&devCtx->Header);
    if (!NT_SUCCESS(status))
    {
        goto exit;
    }

    status = BthEchoSrvRegisterPSM(devCtx);
    if (!NT_SUCCESS(status))
    {
        goto exit;
    }

    status = BthEchoSrvRegisterL2CAPServer(devCtx);
    if (!NT_SUCCESS(status))
    {
        goto exit;
    }

    status = WdfFdoQueryForInterface(
        Device,
        &GUID_BTHDDI_SDP_PARSE_INTERFACE, 
        (PINTERFACE) (&sdpParseInterface),
        sizeof(sdpParseInterface), 
        BTHDDI_SDP_PARSE_INTERFACE_VERSION_FOR_QI, 
        NULL
        );
                
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, 
            "QueryInterface failed for Interface sdp parse interface, version %d, Status code %!STATUS!\n", 
            BTHDDI_SDP_PARSE_INTERFACE_VERSION_FOR_QI,
            status);

        goto exit;
    }

    status = WdfFdoQueryForInterface(
        Device,
        &GUID_BTHDDI_SDP_NODE_INTERFACE, 
        (PINTERFACE) (&sdpNodeInterface),
        sizeof(sdpNodeInterface), 
        BTHDDI_SDP_NODE_INTERFACE_VERSION_FOR_QI, 
        NULL
        );
                
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, 
            "QueryInterface failed for Interface sdp node interface, version %d, Status code %!STATUS!\n", 
            BTHDDI_SDP_NODE_INTERFACE_VERSION_FOR_QI,
            status);

        goto exit;
    }

    status = CreateSdpRecord(
        &sdpNodeInterface,
        &sdpParseInterface,
        &BTHECHOSAMPLE_SVC_GUID,
        BthEchoSampleSvcName,
        devCtx->Psm,
        &sdpRecordStream,
        &sdpRecordLength
        );                                                
    if (!NT_SUCCESS(status))
    {
        goto exit;
    }
    
    status = BthEchoSrvPublishSdpRecord(devCtx, sdpRecordStream, sdpRecordLength);
    if (!NT_SUCCESS(status))
    {
        goto exit;
    }
    
exit:

    if (sdpRecordStream)
    {
        FreeSdpRecord(sdpRecordStream);
    }
    
    return status;    
}    

VOID
BthEchoSrvEvtDeviceSelfManagedIoCleanup(
    _In_ WDFDEVICE  Device
    )
/*++

Description:

    This routine is called by the framework only once
    and hence we use it for our one time de-initialization.

    In this routine we remove sdp record and unregister server.

    We also disconnect all the outstanding connections.

Arguments:

    Device - Framework device object

Return Value:

    NTSTATUS Status code.

--*/
{
    PBTHECHOSAMPLE_SERVER_CONTEXT devCtx = GetServerDeviceContext(Device);
        
    PAGED_CODE();

    if (HANDLE_SDP_NULL != devCtx->SdpRecordHandle)
    {
        BthEchoSrvRemoveSdpRecord(devCtx);        
    }

    if (NULL != devCtx->L2CAPServerHandle)
    {
        BthEchoSrvUnregisterL2CAPServer(devCtx);
    }

    if (0 != devCtx->Psm)
    {
        BthEchoSrvUnregisterPSM(devCtx);
    }

    //
    // Disconnect any open connections, after this point no more
    // connections can come beacuse we have unregistered server
    //
    // BthEchoSrvDisconnectConnectionsOnRemove does not wait for disconnect
    // to complete. Connection object's cleanup callback waits on that. Since 
    // connection objects are children of device, they will be cleaned up and
    // disconnect would complete before device object is cleaned up.
    //
    
    BthEchoSrvDisconnectConnectionsOnRemove(devCtx);

    return;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
BthEchoSrvConnectionObjectContReaderReadCompletedCallback(
    _In_ PBTHECHOSAMPLE_DEVICE_CONTEXT_HEADER DevCtxHdr,
    _In_ PBTHECHO_CONNECTION Connection,
    _In_ PVOID Buffer,
    _In_ size_t BufferLength
    )
/*++
Routine Description:

    This routine is invoked by the continous reader when read completes.

    We in turn call BthEchoSrvSendEcho to perform echo.

Arguments:

    DevCtxHdr - Device context
    Connection - Connection whose continous reader had read completion
    Bufer - Buffer which received read
    SrcBufferLength - Length of read

--*/
{
    BthEchoSrvSendEcho(
        DevCtxHdr,
        Connection,
        Buffer,
        BufferLength
        );
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
BthEchoSrvConnectionObjectContReaderFailedCallback(
    _In_ PBTHECHOSAMPLE_DEVICE_CONTEXT_HEADER DevCtxHdr,
    _In_ PBTHECHO_CONNECTION Connection
    )
/*++
Routine Description:

    This routine is invoked by the continous reader when there is a failure
    in read submission on completion

    In response we disconnect the remote connection

Arguments:

    DevCtxHdr - Device context
    Connection - Connection whose continous reader had read completion
    Bufer - Buffer which received read
    SrcBufferLength - Length of read

--*/
{
    UNREFERENCED_PARAMETER (DevCtxHdr);
    
    BthEchoSrvDisconnectConnection(Connection);
}


_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
BthEchoSrvConnectionStateConnected(
    WDFOBJECT ConnectionObject
    )
/*++
Routine Description:

    This routine is invoked by BthEchoSrvRemoteConnectResponseCompletion
    when connect response is completed.

    We initialize and submit continous readers in this routine.

Arguments:

    ConnectionObject - Connection object for which connect response completed

--*/
{
    PBTHECHO_CONNECTION connection;
    NTSTATUS status;

    connection = GetConnectionObjectContext(ConnectionObject);

    status = BthEchoConnectionObjectInitializeContinuousReader(
        connection,
        BthEchoSrvConnectionObjectContReaderReadCompletedCallback,
        BthEchoSrvConnectionObjectContReaderFailedCallback,
        BthEchoSampleMaxDataLength
        );

    if (!NT_SUCCESS(status))
    {
        goto exit;
    }

    status = BthEchoConnectionObjectContinuousReaderSubmitReaders(
        connection
        );

    if (!NT_SUCCESS(status))
    {
        goto exit;
    }
    
exit:    
    return status;
}

