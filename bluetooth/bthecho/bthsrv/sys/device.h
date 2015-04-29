#include "clisrv.h" 

typedef struct _BTHECHOSAMPLE_SERVER_CONTEXT
{
    //
    // Context common to client and server
    //
    BTHECHOSAMPLE_DEVICE_CONTEXT_HEADER Header;

    //
    // Our Data PSM
    //    
    USHORT Psm;

    //
    // Handle to published SDP record
    //
    HANDLE_SDP SdpRecordHandle;

    //
    // Handle obtained by registering L2CAP server
    //
    L2CAP_SERVER_HANDLE L2CAPServerHandle;

    //
    // BRB used for server and PSM register and unregister
    //
    // Server and PSM register and unregister must be done
    // sequentially since access to this brb is not
    // synchronized.
    //
    struct _BRB RegisterUnregisterBrb;
    
    //
    // Connection List lock
    //
    WDFSPINLOCK ConnectionListLock;

    //
    // Outstanding open connections
    //
    LIST_ENTRY ConnectionList;
    
} BTHECHOSAMPLE_SERVER_CONTEXT, *PBTHECHOSAMPLE_SERVER_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(BTHECHOSAMPLE_SERVER_CONTEXT, GetServerDeviceContext)

NTSTATUS
FORCEINLINE
BthEchoSampleServerContextInit(
    PBTHECHOSAMPLE_SERVER_CONTEXT Context,
    WDFDEVICE Device
    )
{
    NTSTATUS status;
    WDF_OBJECT_ATTRIBUTES attributes;
    
    status = BthEchoSharedDeviceContextHeaderInit(&Context->Header, Device);
    if (!NT_SUCCESS(status))
    {
        goto exit;                
    }

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = Device;

    status = WdfSpinLockCreate(
                               &attributes,
                               &Context->ConnectionListLock
                               );
    if (!NT_SUCCESS(status))
    {
        goto exit;                
    }

    InitializeListHead(&Context->ConnectionList);

exit:
    return status;
}

EVT_WDF_DRIVER_DEVICE_ADD BthEchoSrvEvtDriverDeviceAdd;

EVT_WDF_DEVICE_SELF_MANAGED_IO_INIT BthEchoSrvEvtDeviceSelfManagedIoInit;

EVT_WDF_DEVICE_SELF_MANAGED_IO_CLEANUP BthEchoSrvEvtDeviceSelfManagedIoCleanup;

//////////////////////////////////////////////////////
// Device specific functionality invoked by server.c
//////////////////////////////////////////////////////

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
BthEchoSrvConnectionStateConnected(
    WDFOBJECT ConnectionObject
    );

////////////////////////////////////////////////////////////////////
// Continuous reader callbacks (invoked by common/lib/connection.c)
////////////////////////////////////////////////////////////////////

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
BthEchoSrvConnectionObjectContReaderReadCompletedCallback(
    _In_ PBTHECHOSAMPLE_DEVICE_CONTEXT_HEADER DevCtxHdr,
    _In_ PBTHECHO_CONNECTION Connection,
    _In_ PVOID Buffer,
    _In_ size_t BufferLength
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
BthEchoSrvConnectionObjectContReaderFailedCallback(
    _In_ PBTHECHOSAMPLE_DEVICE_CONTEXT_HEADER DevCtxHdr,
    _In_ PBTHECHO_CONNECTION Connection
    );
