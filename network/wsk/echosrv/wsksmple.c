/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    wsksample.c

Abstract:

    This module implements a simple kernel-mode application by using the 
    Winsock Kernel (WSK) programming interface. The application accepts
    incoming connection requests and, on each connection, echoes all received
    data back to the peer until the connection is closed by the peer.
    The application is designed to use a single worker thread to perform all of
    its processing. For better performance on MP machines, the sample may be 
    enhanced to use more worker threads. Operations on a given connection 
    should always be processed by the same worker thread. This provides a
    simple form of synchronization ensuring proper socket closure in a setting
    where multiple operations may be outstanding and completed asynchronously
    on a given connection. For the sake of simplicty, this sample does not
    enforce any limit on the number of connections accepted (other than the
    natural limit imposed by the available system memory) or on the amount of
    time a connection stays around. A full-fledged server application should be
    designed with these points in mind from a security viewpoint. 

Author:

Environment:

    Kernel-Mode only

Revision History:

--*/

#pragma warning(push)
#pragma warning(disable:4201) // nameless struct/union
#pragma warning(disable:4214) // bit field types other than int

#define POOL_ZERO_DOWN_LEVEL_SUPPORT

#include <ntddk.h>
#include <wsk.h>

#pragma warning(pop)

// Software Tracing definitions
#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID(WskSampleCtlGuid, \
        (998bdf51, 0349, 4fbc, 870c, d6130a955a5f), \
        WPP_DEFINE_BIT(TRCERROR) \
        WPP_DEFINE_BIT(TRCINFO) )

#include "wsksmple.tmh"

// Pool tags used for memory allocations
#define WSKSAMPLE_SOCKET_POOL_TAG ((ULONG)'sksw')
#define WSKSAMPLE_BUFFER_POOL_TAG ((ULONG)'bksw')
#define WSKSAMPLE_GENERIC_POOL_TAG ((ULONG)'xksw')

// Default length for data buffers used in send and receive operations
#define WSKSAMPLE_DATA_BUFFER_LENGTH 2048

// Forward declaration for the socket context structure
typedef struct _WSKSAMPLE_SOCKET_CONTEXT *PWSKSAMPLE_SOCKET_CONTEXT;

// Forward declaration for the socket operation context structure
typedef struct _WSKSAMPLE_SOCKET_OP_CONTEXT *PWSKSAMPLE_SOCKET_OP_CONTEXT;

// Function prototype for socket operation handling routines
typedef
VOID
(* PWSKSAMPLE_OP_HANDLER_FN)(
    _In_ PWSKSAMPLE_SOCKET_OP_CONTEXT SocketOpContext);

// Structure that represents a work queue processed by a dedicated worker thread
typedef struct _WSKSAMPLE_WORK_QUEUE {

    // List head
    SLIST_HEADER Head;

    // Wake up event
    KEVENT Event;

    // Worker thread can exit safely only if queue is empty and Stop is TRUE
    BOOLEAN Stop;

    // Worker thread pointer
    PETHREAD Thread;
    
} WSKSAMPLE_WORK_QUEUE, *PWSKSAMPLE_WORK_QUEUE;

// Structure that represents the context for a WSK socket operation.
typedef struct _WSKSAMPLE_SOCKET_OP_CONTEXT {

    // Work queue linkage
    SLIST_ENTRY QueueEntry;
    
    // Pointer to the function that will handle the operation 
    PWSKSAMPLE_OP_HANDLER_FN OpHandler;
    
    // Pointer to the WSK socket context.
    PWSKSAMPLE_SOCKET_CONTEXT SocketContext;

    // IRP to use for the operation
    PIRP Irp;
    
    // Data buffer and MDL used by send and receive operations
    _Field_size_bytes_part_(BufferLength,DataLength) PVOID  DataBuffer;
    PMDL   DataMdl;
    SIZE_T BufferLength; // size of the buffer
    SIZE_T DataLength;   // length of actual data stored in the buffer
    
} WSKSAMPLE_SOCKET_OP_CONTEXT;

// Maximum number of operations that can be outstanding on a socket at any time 
#define WSKSAMPLE_OP_COUNT 2

// Structure that represents the context for a WSK socket.
typedef struct _WSKSAMPLE_SOCKET_CONTEXT {

    // Pointer to the WSK socket.
    PWSK_SOCKET Socket;

    // Work queue used for enqueueing operations on the socket
    PWSKSAMPLE_WORK_QUEUE WorkQueue;

    // Socket is being closed.
    BOOLEAN Closing;

    // Peer has gracefully disconnected its half of the connection and we are
    // about to disconnect (or have disconnected) our half.
    BOOLEAN Disconnecting;
    
    // Stop accepting incoming connections. Valid for listening sockets only.
    BOOLEAN StopListening;

    // Embedded array of contexts for outstanding operations on the socket.
    // Note that operation contexts could also be allocated separately. This
    // sample preallocates a fixed number of operation contexts along with
    // the socket context for a new socket.
    WSKSAMPLE_SOCKET_OP_CONTEXT OpContext[WSKSAMPLE_OP_COUNT];
    
} WSKSAMPLE_SOCKET_CONTEXT;

// Forward declaration for WskAcceptEvent in WSK_CLIENT_LISTEN_DISPATCH 
NTSTATUS
WSKAPI 
WskSampleAcceptEvent(
    _In_  PVOID         SocketContext,
    _In_  ULONG         Flags,
    _In_  PSOCKADDR     LocalAddress,
    _In_  PSOCKADDR     RemoteAddress,
    _In_opt_  PWSK_SOCKET AcceptSocket,
    _Outptr_result_maybenull_ PVOID *AcceptSocketContext,
    _Outptr_result_maybenull_ CONST WSK_CLIENT_CONNECTION_DISPATCH **AcceptSocketDispatch
    );

// Client-level callback table
const WSK_CLIENT_DISPATCH WskSampleClientDispatch = {
    MAKE_WSK_VERSION(1, 0), // This sample uses WSK version 1.0
    0, // Reserved
    NULL // WskClientEvent callback is not required in WSK version 1.0
};

// WSK Registration object
WSK_REGISTRATION WskSampleRegistration;

// Socket-level callback table for listening sockets
const WSK_CLIENT_LISTEN_DISPATCH WskSampleClientListenDispatch = {
    WskSampleAcceptEvent,
    NULL, // WskInspectEvent is required only if conditional-accept is used.
    NULL  // WskAbortEvent is required only if conditional-accept is used.
};

// Global reference to the socket context for the listening socket
PWSKSAMPLE_SOCKET_CONTEXT WskSampleListeningSocketContext;

// Global work queue used for enqueueing all socket operations
WSKSAMPLE_WORK_QUEUE WskSampleWorkQueue;

// IPv6 wildcard address and port number 40007 to listen on
SOCKADDR_IN6 IPv6ListeningAddress = {
                    AF_INET6, 
                    0x479c, // 40007 in hex in network byte order 
                    0, 
                    IN6ADDR_ANY_INIT,
                    0};

// Forward function declarations

DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD WskSampleUnload;
KSTART_ROUTINE WskSampleWorkerThread;
IO_COMPLETION_ROUTINE WskSampleSyncIrpCompletionRoutine;
IO_COMPLETION_ROUTINE WskSampleReceiveIrpCompletionRoutine;
IO_COMPLETION_ROUTINE WskSampleSendIrpCompletionRoutine;
IO_COMPLETION_ROUTINE WskSampleDisconnectIrpCompletionRoutine;
IO_COMPLETION_ROUTINE WskSampleCloseIrpCompletionRoutine;

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    );

_Must_inspect_result_
__drv_allocatesMem(Mem)
_Success_(return != NULL)
PWSKSAMPLE_SOCKET_CONTEXT
WskSampleAllocateSocketContext(
    _In_ PWSKSAMPLE_WORK_QUEUE WorkQueue,
    _In_ ULONG BufferLength
    );

_At_(SocketContext, __drv_freesMem(Mem))
VOID
WskSampleFreeSocketContext(
    _In_ PWSKSAMPLE_SOCKET_CONTEXT SocketContext
    );

_At_(SocketOpContext, __drv_aliasesMem)
VOID
WskSampleEnqueueOp(
    _In_ PWSKSAMPLE_SOCKET_OP_CONTEXT SocketOpContext,
    _In_ PWSKSAMPLE_OP_HANDLER_FN OpHandler
    );

VOID
WskSampleWorkerThread (
    _In_ PVOID Context
    );

VOID
WskSampleSetupListeningSocket(
    _In_ PWSK_PROVIDER_NPI WskProviderNpi,
    _In_ PWSKSAMPLE_SOCKET_OP_CONTEXT SocketOpContext
    );

VOID
WskSampleOpStartListen(
    _In_ PWSKSAMPLE_SOCKET_OP_CONTEXT SocketOpContext
    );

VOID
WskSampleOpStopListen(
    _In_ PWSKSAMPLE_SOCKET_OP_CONTEXT SocketOpContext
    );

VOID
WskSampleOpReceive(
    _In_ PWSKSAMPLE_SOCKET_OP_CONTEXT SocketOpContext
    );

VOID
WskSampleOpSend(
    _In_ PWSKSAMPLE_SOCKET_OP_CONTEXT SocketOpContext
    );

VOID
WskSampleOpDisconnect(
    _In_ PWSKSAMPLE_SOCKET_OP_CONTEXT SocketOpContext
    );

VOID
WskSampleOpClose(
    _In_ PWSKSAMPLE_SOCKET_OP_CONTEXT SocketOpContext
    );

VOID
WskSampleOpFree(
    _In_ PWSKSAMPLE_SOCKET_OP_CONTEXT SocketOpContext
    );

NTSTATUS
WskSampleStartWorkQueue(
    _Out_ PWSKSAMPLE_WORK_QUEUE WorkQueue
    );

VOID
WskSampleStopWorkQueue(
    _In_ PWSKSAMPLE_WORK_QUEUE WorkQueue
    );

VOID
WskSampleUnload(
    _In_ PDRIVER_OBJECT DriverObject
    );

#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(INIT, WskSampleStartWorkQueue)
#pragma alloc_text(PAGE, WskSampleUnload)
#pragma alloc_text(PAGE, WskSampleStopWorkQueue)
#pragma alloc_text(PAGE, WskSampleWorkerThread)
#pragma alloc_text(PAGE, WskSampleOpStartListen)
#pragma alloc_text(PAGE, WskSampleOpStopListen)
#pragma alloc_text(PAGE, WskSampleSetupListeningSocket)
#pragma alloc_text(PAGE, WskSampleOpReceive)
#pragma alloc_text(PAGE, WskSampleOpSend)
#pragma alloc_text(PAGE, WskSampleOpDisconnect)
#pragma alloc_text(PAGE, WskSampleOpClose)
#pragma alloc_text(PAGE, WskSampleOpFree)

#endif

// Driver entry routine
NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS status;
    WSK_CLIENT_NPI wskClientNpi;
    
    UNREFERENCED_PARAMETER(RegistryPath);

    PAGED_CODE();

    ExInitializeDriverRuntime(DrvRtPoolNxOptIn);
    
    // Allocate a socket context that will be used for queueing an operation
    // to setup a listening socket that will accept incoming connections
    WskSampleListeningSocketContext = WskSampleAllocateSocketContext(
                                            &WskSampleWorkQueue, 0);

    if(WskSampleListeningSocketContext == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Register with WSK.
    wskClientNpi.ClientContext = NULL;
    wskClientNpi.Dispatch = &WskSampleClientDispatch;
    status = WskRegister(&wskClientNpi, &WskSampleRegistration);

    if(!NT_SUCCESS(status)) {
        WskSampleFreeSocketContext(WskSampleListeningSocketContext);
        return status;
    }

    // Initialize and start the global work queue
    status = WskSampleStartWorkQueue(&WskSampleWorkQueue);

    if(!NT_SUCCESS(status)) {
        WskDeregister(&WskSampleRegistration);
        WskSampleFreeSocketContext(WskSampleListeningSocketContext);
        return status;
    }

    // Enqueue the first operation to setup the listening socket
    WskSampleEnqueueOp(&WskSampleListeningSocketContext->OpContext[0],
                       WskSampleOpStartListen);
    
    // Everything has been initiated successfully. Now we can enable the
    // unload routine and return.
    DriverObject->DriverUnload = WskSampleUnload;

    // Initialize software tracing
    WPP_INIT_TRACING(DriverObject, RegistryPath);
    
    DoTraceMessage(TRCINFO, "LOADED");
    
    return STATUS_SUCCESS;
}

// Driver unload routine
VOID
WskSampleUnload(
    _In_ PDRIVER_OBJECT DriverObject
    )
{  
    C_ASSERT(WSKSAMPLE_OP_COUNT >= 2);

    UNREFERENCED_PARAMETER(DriverObject);

    PAGED_CODE();

    DoTraceMessage(TRCINFO, "UNLOAD START");

    // Enqueue an operation to stop listening for any new connections.
    // This will close the listening socket, but there may still be 
    // connection-oriented sockets that are open. The WskDeregister call
    // below will block until all sockets including the connection-oriented
    // ones that were accepted over the listening socket are closed. Since
    // this sample currently closes the connection-oriented sockets
    // only when an error occurs or when the remote peer disconnects, the
    // unload of the driver will be blocked until the last connection is 
    // disconnected by the peer. If the requirements for a full-fledged
    // application are such that the unload of the driver (or any other
    // similar event) proactively closes all the sockets (that may 
    // otherwise stay open indefinitely), then the application must keep
    // track of all the sockets in a fashion that allows enumerating them and
    // closing each socket. In such a scheme, the application must still ensure
    // that a socket is closed only when other calls can NOT anymore be issued
    // on the socket.
    WskSampleEnqueueOp(&WskSampleListeningSocketContext->OpContext[1],
                       WskSampleOpStopListen);

    // Deregister with WSK. This call will wait until all the references to
    // the WSK provider NPI are released and all the sockets are closed. Note
    // that if the worker thread has not started yet, then when it eventually
    // starts, its WskCaptureProviderNPI call will fail and the work queue
    // will be flushed and cleaned up properly.
    WskDeregister(&WskSampleRegistration);

    // WskDeregister returns only if all the sockets are closed. Thus, at this
    // point, it's guaranteed that all socket are closed, which also means that
    // there can not be any further outstanding operations on any socket. So,
    // the worker thread can now safely stop processing the work queue if there
    // are no queued items. Signal the worker thread to stop and wait for it. 
    WskSampleStopWorkQueue(&WskSampleWorkQueue);
    
    DoTraceMessage(TRCINFO, "UNLOAD END");

    WPP_CLEANUP(DriverObject);
}

// Initialize a given work queue and start the worker thread for it
NTSTATUS
WskSampleStartWorkQueue(
    _Out_ PWSKSAMPLE_WORK_QUEUE WorkQueue
    )
{
    NTSTATUS status;
    HANDLE threadHandle;

    PAGED_CODE();
    
    InitializeSListHead(&WorkQueue->Head);
    KeInitializeEvent(&WorkQueue->Event, SynchronizationEvent, FALSE);
    WorkQueue->Stop = FALSE;

    status = PsCreateSystemThread(
                &threadHandle, THREAD_ALL_ACCESS, NULL, NULL, NULL,
                WskSampleWorkerThread, WorkQueue);

    if(!NT_SUCCESS(status)) {
        return status;
    }

    status = ObReferenceObjectByHandle(
                threadHandle, THREAD_ALL_ACCESS, NULL, KernelMode,
                &WorkQueue->Thread, NULL);
    
    ZwClose(threadHandle);

    if(!NT_SUCCESS(status)) {
        WorkQueue->Stop = TRUE;
        KeSetEvent(&WorkQueue->Event, 0, FALSE);
    }

    return status;
}

// Stop a given work queue and wait for its worker thread to exit
VOID
WskSampleStopWorkQueue(
    _In_ PWSKSAMPLE_WORK_QUEUE WorkQueue
    )
{
    PAGED_CODE();

    ASSERT(WorkQueue->Stop == FALSE);
    ASSERT(WorkQueue->Thread);
    WorkQueue->Stop = TRUE;
    KeSetEvent(&WorkQueue->Event, 0, FALSE);
    KeWaitForSingleObject(WorkQueue->Thread, Executive, KernelMode, FALSE,NULL);
    ObDereferenceObject(WorkQueue->Thread);
}

// Allocate and setup a socket context
_Must_inspect_result_
__drv_allocatesMem(Mem)
_Success_(return != NULL)
PWSKSAMPLE_SOCKET_CONTEXT
WskSampleAllocateSocketContext(
    _In_ PWSKSAMPLE_WORK_QUEUE WorkQueue,
    _In_ ULONG BufferLength
    )
{
    PWSKSAMPLE_SOCKET_CONTEXT socketContext;
    
    // Allocate and setup a socket context with optional data buffers, and
    // attach the socket to the given work queue. Even though this sample uses
    // only a single global work queue, the sample is designed to be easily
    // adaptable to using multiple work queues. If the sample is changed to
    // use multiple work queues (say a work queue per processor), a given
    // socket will/must always use the same work queue.

    socketContext = ExAllocatePoolZero(
        NonPagedPoolNx, sizeof(*socketContext), WSKSAMPLE_SOCKET_POOL_TAG);

    if(socketContext != NULL) {

        ULONG i;

        socketContext->WorkQueue = WorkQueue;

        for(i = 0; i < WSKSAMPLE_OP_COUNT; i++) {

            socketContext->OpContext[i].SocketContext = socketContext;

            socketContext->OpContext[i].Irp = IoAllocateIrp(1, FALSE);
            if(socketContext->OpContext[i].Irp == NULL) {
                goto failure;
            }

            if(BufferLength > 0) {
                socketContext->OpContext[i].DataBuffer = ExAllocatePoolUninitialized(
                    NonPagedPoolNx, BufferLength, WSKSAMPLE_BUFFER_POOL_TAG);
                if(socketContext->OpContext[i].DataBuffer == NULL) {
                    goto failure;
                }
                socketContext->OpContext[i].DataMdl = IoAllocateMdl(
                   socketContext->OpContext[i].DataBuffer,
                   BufferLength, FALSE, FALSE, NULL);
                if(socketContext->OpContext[i].DataMdl == NULL) {
                    goto failure;
                }
                MmBuildMdlForNonPagedPool(socketContext->OpContext[i].DataMdl);
                socketContext->OpContext[i].BufferLength = BufferLength;
            }
        }

        DoTraceMessage(TRCINFO, "AllocateSocketContext: %p", socketContext);

        return socketContext;
    }

failure:

    DoTraceMessage(TRCERROR, "AllocateSocketContext: FAIL");

    if(socketContext) {
        WskSampleFreeSocketContext(socketContext);
    }

    return NULL;
}

// Cleanup and free a socket context
_At_(SocketContext, __drv_freesMem(Mem))
VOID
WskSampleFreeSocketContext(
    _In_ PWSKSAMPLE_SOCKET_CONTEXT SocketContext
    )
{
    ULONG i;

    // Socket context is freed only after all the WSK calls on the
    // socket are completed and all enqueued operations for the socket
    // are dequeued. So, we can safely free the Irp, Mdl, and data buffer
    // pointed by the socket operation contexts.
    
    for(i = 0; i < WSKSAMPLE_OP_COUNT; i++) {
        
        if(SocketContext->OpContext[i].Irp != NULL) {
            IoFreeIrp(SocketContext->OpContext[i].Irp);
            SocketContext->OpContext[i].Irp = NULL;
        }
        if(SocketContext->OpContext[i].DataMdl != NULL) {
            IoFreeMdl(SocketContext->OpContext[i].DataMdl);
            SocketContext->OpContext[i].DataMdl = NULL;
        }
        if(SocketContext->OpContext[i].DataBuffer != NULL) {
            ExFreePool(SocketContext->OpContext[i].DataBuffer);
            SocketContext->OpContext[i].DataBuffer = NULL;
        }
    }

    DoTraceMessage(TRCINFO, "FreeSocketContext: %p", SocketContext);

    ExFreePool(SocketContext);
}

// Enqueue an operation on a socket
_At_(SocketOpContext, __drv_aliasesMem)
VOID
WskSampleEnqueueOp(
    _In_ PWSKSAMPLE_SOCKET_OP_CONTEXT SocketOpContext,
    _In_ PWSKSAMPLE_OP_HANDLER_FN OpHandler
    )
{
    PWSKSAMPLE_WORK_QUEUE workQueue = SocketOpContext->SocketContext->WorkQueue;

    SocketOpContext->OpHandler = OpHandler;

    #pragma warning(disable:4054) // turn off typecast warning
    
    if(NULL == InterlockedPushEntrySList(
                &workQueue->Head, 
                &SocketOpContext->QueueEntry)) {
        DoTraceMessage(TRCINFO, "EnqueueOp: %p %p (%p) SetEvent", 
            SocketOpContext->SocketContext, SocketOpContext, (PVOID)OpHandler);
        // Work queue was empty. So, signal the work queue event in case the
        // worker thread is waiting on the event for more operations.
        KeSetEvent(&workQueue->Event, 0, FALSE);
    }
    else
        DoTraceMessage(TRCINFO, "EnqueueOp: %p %p (%p)", 
            SocketOpContext->SocketContext, SocketOpContext, (PVOID)OpHandler);

    #pragma warning(default:4054) // restore typecast warning
}

// Worker thread which drains and processes a given work queue
VOID
WskSampleWorkerThread (
    _In_ PVOID Context
    )
{
    PWSKSAMPLE_WORK_QUEUE workQueue;
    PSLIST_ENTRY listEntryRev, listEntry, next;
    
    PAGED_CODE();

    workQueue = (PWSKSAMPLE_WORK_QUEUE)Context;

    for(;;) {
        
        // Flush all the queued operations into a local list
        listEntryRev = InterlockedFlushSList(&workQueue->Head);

        if(listEntryRev == NULL) {

            // There's no work to do. If we are allowed to stop, then stop.
            if(workQueue->Stop) {
                DoTraceMessage(TRCINFO, "WorkerThread: WQ %p exit", workQueue);
                break;
            }

            DoTraceMessage(TRCINFO, "WorkerThread: WQ %p wait", workQueue);

            // Otherwise, wait for more operations to be enqueued.
            KeWaitForSingleObject(&workQueue->Event, 
                Executive, KernelMode, FALSE, 0);
            continue;
        }

        DoTraceMessage(TRCINFO, "WorkerThread: WQ %p process", workQueue);

        // Need to reverse the flushed list in order to preserve the FIFO order
        listEntry = NULL;
        while (listEntryRev != NULL) {
            next = listEntryRev->Next;
            listEntryRev->Next = listEntry;
            listEntry = listEntryRev;
            listEntryRev = next;
        }

        // Now process the correctly ordered list of operations one by one
        while(listEntry) {

            PWSKSAMPLE_SOCKET_OP_CONTEXT socketOpContext =
                CONTAINING_RECORD(listEntry, 
                    WSKSAMPLE_SOCKET_OP_CONTEXT, QueueEntry);
            PWSKSAMPLE_OP_HANDLER_FN opHandler = socketOpContext->OpHandler;
            
            listEntry = listEntry->Next;

            opHandler(socketOpContext);
        }
    }
    
    PsTerminateSystemThread(STATUS_SUCCESS);
}

// Operation handler for creating and setting up a listening socket
VOID
WskSampleOpStartListen(
    _In_ PWSKSAMPLE_SOCKET_OP_CONTEXT SocketOpContext
    )
{
    NTSTATUS status;
    WSK_PROVIDER_NPI wskProviderNpi;

    PAGED_CODE();

    // This is the very first operation that runs in the worker thread.
    // This operation is made up of multiple WSK calls, and no other
    // operations will be processed until this compound operation is
    // finished synchronously in the context of the worker thread.
    
    // Capture the WSK Provider NPI
    status = WskCaptureProviderNPI(
                &WskSampleRegistration, 
                WSK_INFINITE_WAIT,
                &wskProviderNpi);

    if(NT_SUCCESS(status)) {

        // Create a listening socket
        WskSampleSetupListeningSocket(&wskProviderNpi, SocketOpContext);

        // Release the WSK provider NPI since we won't use it anymore
        WskReleaseProviderNPI(&WskSampleRegistration);
    }
    else {
        // WskCaptureProviderNPI will fail if WskDeregister is called
        // from the Driver Unload routine before the WSK subsystem
        // becomes ready.
        DoTraceMessage(TRCINFO, 
            "OpStartListen: WskCaptureProviderNPI failed 0x%lx", status);
    }
}

// Create and prepare a listening socket for accepting incoming connections
VOID
WskSampleSetupListeningSocket(
    _In_ PWSK_PROVIDER_NPI WskProviderNpi,
    _In_ PWSKSAMPLE_SOCKET_OP_CONTEXT SocketOpContext
    )
{
    NTSTATUS status;
    WSK_EVENT_CALLBACK_CONTROL callbackControl;
    KEVENT compEvent;
    ULONG optionValue;
    PWSK_SOCKET listeningSocket = NULL;
    PWSK_PROVIDER_LISTEN_DISPATCH dispatch = NULL;
    PWSKSAMPLE_SOCKET_CONTEXT socketContext = SocketOpContext->SocketContext;
    PIRP irp = SocketOpContext->Irp;

    PAGED_CODE();
    
    KeInitializeEvent(&compEvent, SynchronizationEvent, FALSE);

    if(socketContext->Socket != NULL) {

        // If there's already a socket then we must be getting called because
        // the WskAcceptevent indicated a NULL AcceptSocket, i.e. the listening
        // socket is no longer functional. So, we will close the existing
        // listening socket and try to create a new one.

        if(socketContext->StopListening) {
            // Listening socket is already being closed because the driver
            // is being unloaded. So, there's no need to recreate it.
            return;
        }
        
        IoReuseIrp(irp, STATUS_UNSUCCESSFUL);

        IoSetCompletionRoutine(irp, 
            WskSampleSyncIrpCompletionRoutine,
            &compEvent, TRUE, TRUE, TRUE);

        ((PWSK_PROVIDER_LISTEN_DISPATCH)
            socketContext->Socket->Dispatch)->
                WskCloseSocket(socketContext->Socket, irp);

        KeWaitForSingleObject(&compEvent, Executive, KernelMode, FALSE, NULL);

        ASSERT(NT_SUCCESS(irp->IoStatus.Status));
        socketContext->Socket = NULL;

        IoReuseIrp(irp, STATUS_UNSUCCESSFUL);
    }
    else {
        // First, configure the WSK client such that WskAcceptEvent callback
        // will be automatically enabled on the listening socket upon creation.
        callbackControl.NpiId = (PNPIID)&NPI_WSK_INTERFACE_ID;
        callbackControl.EventMask = WSK_EVENT_ACCEPT;
        status = WskProviderNpi->Dispatch->WskControlClient(
                            WskProviderNpi->Client,
                            WSK_SET_STATIC_EVENT_CALLBACKS,
                            sizeof(callbackControl),
                            &callbackControl,
                            0,
                            NULL,
                            NULL,
                            NULL);
        if(!NT_SUCCESS(status)) {
            DoTraceMessage(TRCERROR, 
              "SetupListeningSocket: WSK_SET_STATIC_EVENT_CALLBACKS FAIL 0x%lx",
              status);
            goto failexit;        
        }
    }
    
    // Create a listening socket over AF_INET6 address family. Put the socket
    // into dual-family mode by setting IPV6_V6ONLY option to FALSE so that 
    // AF_INET traffic is also handled over the same socket.

    IoSetCompletionRoutine(irp,
        WskSampleSyncIrpCompletionRoutine,
        &compEvent, TRUE, TRUE, TRUE);

    // We do not need to check the return status since the actual completion
    // status will be captured from the IRP after the IRP is completed.
    WskProviderNpi->Dispatch->WskSocket(
                WskProviderNpi->Client,
                AF_INET6,
                SOCK_STREAM,
                IPPROTO_TCP,
                WSK_FLAG_LISTEN_SOCKET,
                socketContext,
                &WskSampleClientListenDispatch,
                NULL, // Process
                NULL, // Thread
                NULL, // SecurityDescriptor
                irp);
    
    KeWaitForSingleObject(&compEvent, Executive, KernelMode, FALSE, NULL);

    if(!NT_SUCCESS(irp->IoStatus.Status)) {
        DoTraceMessage(TRCERROR, "SetupListeningSocket: WskSocket FAIL 0x%lx",
            irp->IoStatus.Status);
        goto failexit;
    }

    listeningSocket = (PWSK_SOCKET)irp->IoStatus.Information;
    dispatch = (PWSK_PROVIDER_LISTEN_DISPATCH)listeningSocket->Dispatch;
    
    // Set IPV6_V6ONLY to FALSE before the bind operation

    optionValue = 0;

    IoReuseIrp(irp, STATUS_UNSUCCESSFUL);    
    IoSetCompletionRoutine(irp,
        WskSampleSyncIrpCompletionRoutine,
        &compEvent, TRUE, TRUE, TRUE);

    // We do not need to check the return status since the actual completion
    // status will be captured from the IRP after the IRP is completed.
    dispatch->WskControlSocket(
                listeningSocket,
                WskSetOption,
                IPV6_V6ONLY,
                IPPROTO_IPV6,
                sizeof(optionValue),
                &optionValue,
                0,
                NULL,
                NULL,
                irp);
    
    KeWaitForSingleObject(&compEvent, Executive, KernelMode, FALSE, NULL);

    if(!NT_SUCCESS(irp->IoStatus.Status)) {
        DoTraceMessage(TRCERROR, "SetupListeningSocket: IPV6_V6ONLY FAIL 0x%lx",
            irp->IoStatus.Status);
        goto failexit;
    }

    // Bind the socket to the wildcard address. Once bind is completed,
    // WSK provider will make WskAcceptEvent callbacks as connections arrive.
    IoReuseIrp(irp, STATUS_UNSUCCESSFUL);
    IoSetCompletionRoutine(irp,
        WskSampleSyncIrpCompletionRoutine,
        &compEvent, TRUE, TRUE, TRUE);

    // We do not need to check the return status since the actual completion
    // status will be captured from the IRP after the IRP is completed.
    dispatch->WskBind(
                listeningSocket,
                (PSOCKADDR)&IPv6ListeningAddress,
                0,
                irp);

    KeWaitForSingleObject(&compEvent, Executive, KernelMode, FALSE, NULL);

    if(!NT_SUCCESS(irp->IoStatus.Status)) {
        DoTraceMessage(TRCERROR, "SetupListeningSocket: WskBind FAIL 0x%lx",
            irp->IoStatus.Status);
        goto failexit;
    }

    // Store the WSK socket pointer in the socket context only if everthing
    // has succeeded. Otherwise, the listening WSK socket, if one was created
    // successfully, will be closed inline below.
    ASSERT(socketContext->Socket == NULL);
    socketContext->Socket = listeningSocket;

    DoTraceMessage(TRCINFO, "SetupListeningSocket: %p %p", 
        socketContext, SocketOpContext);
    
    return;
    
failexit:

    if(listeningSocket) {

        IoReuseIrp(irp, STATUS_UNSUCCESSFUL);
        IoSetCompletionRoutine(irp, 
            WskSampleSyncIrpCompletionRoutine,
            &compEvent, TRUE, TRUE, TRUE);

        dispatch->WskCloseSocket(listeningSocket, irp);
        KeWaitForSingleObject(&compEvent, Executive, KernelMode, FALSE, NULL);
    }
}

// IRP completion routine used for synchronously waiting for completion
_Use_decl_annotations_
NTSTATUS
WskSampleSyncIrpCompletionRoutine(
    PDEVICE_OBJECT Reserved,
    PIRP Irp,
    PVOID Context
    )
{    
    PKEVENT compEvent = (PKEVENT)Context;
    _Analysis_assume_(Context != NULL);
    UNREFERENCED_PARAMETER(Reserved);
    UNREFERENCED_PARAMETER(Irp);
    KeSetEvent(compEvent, 2, FALSE);    
    return STATUS_MORE_PROCESSING_REQUIRED;
}

// Operation handler for stopping listening
VOID
WskSampleOpStopListen(
    _In_ PWSKSAMPLE_SOCKET_OP_CONTEXT SocketOpContext
    )
{
    PWSKSAMPLE_SOCKET_CONTEXT socketContext;

    PAGED_CODE();

    socketContext = SocketOpContext->SocketContext;

    DoTraceMessage(TRCINFO, "OpStopListen: %p %p", 
        socketContext, SocketOpContext);

    socketContext->StopListening = TRUE;
    
    if(socketContext->Socket == NULL) {
        // Listening socket was NOT created due to some failure.
        // All we need to do is to free up the socket context.
        WskSampleEnqueueOp(SocketOpContext, WskSampleOpFree);
    }
    else {
        // Enqueue an operation to close the listening socket
        WskSampleEnqueueOp(SocketOpContext, WskSampleOpClose);
    }
}

// Listening socket callback which is invoked whenever a new connection arrives.
NTSTATUS
WSKAPI 
WskSampleAcceptEvent(
    _In_  PVOID         SocketContext,
    _In_  ULONG         Flags,
    _In_  PSOCKADDR     LocalAddress,
    _In_  PSOCKADDR     RemoteAddress,
    _In_opt_  PWSK_SOCKET AcceptSocket,
    _Outptr_result_maybenull_ PVOID *AcceptSocketContext,
    _Outptr_result_maybenull_ CONST WSK_CLIENT_CONNECTION_DISPATCH **AcceptSocketDispatch
    )
{
    PWSKSAMPLE_SOCKET_CONTEXT socketContext = NULL;
    PWSKSAMPLE_SOCKET_CONTEXT listeningSocketContext;
    ULONG i;

    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(LocalAddress);
    UNREFERENCED_PARAMETER(RemoteAddress);
    
    listeningSocketContext = (PWSKSAMPLE_SOCKET_CONTEXT)SocketContext;

    if(AcceptSocket == NULL) {
        // If WSK provider makes a WskAcceptEvent callback with NULL 
        // AcceptSocket, this means that the listening socket is no longer
        // functional. The WSK client may handle this situation by trying
        // to create a new listening socket or by restarting the driver, etc.
        // In this sample, we will attempt to close the existing listening
        // socket and create a new one. Note that the WskSampleAcceptEvent
        // callback is guaranteed to be invoked with a NULL AcceptSocket
        // by the WSK subsystem only *once*. So, we can safely use the same
        // operation context that was originally used for enqueueing the first
        // WskSampleStartListen operation on the listening socket. The
        // WskSampleStartListen operation will close the existing listening
        // socket and create a new one.
        WskSampleEnqueueOp(&listeningSocketContext->OpContext[0], 
                           WskSampleOpStartListen);
        return STATUS_REQUEST_NOT_ACCEPTED;
    }

    // Allocate socket context for the newly accepted socket.
    socketContext = WskSampleAllocateSocketContext(
                        &WskSampleWorkQueue, WSKSAMPLE_DATA_BUFFER_LENGTH);
    
    if(socketContext == NULL) {
        return STATUS_REQUEST_NOT_ACCEPTED;
    }

    socketContext->Socket = AcceptSocket;

    DoTraceMessage(TRCINFO, "AcceptEvent: %p", socketContext);
    
    // Enqueue receive operations on the accepted socket. Whenever a receive
    // operation is completed successfully, the received data will be echoed
    // back to the peer via a send operation. Whenever a send operation is 
    // completed, a new receive request will be issued over the connection.
    // This will continue until the connection is closed by the peer.
    for(i = 0; i < WSKSAMPLE_OP_COUNT; i++) {
        _Analysis_assume_(socketContext == socketContext->OpContext[i].SocketContext);
        WskSampleEnqueueOp(&socketContext->OpContext[i], WskSampleOpReceive);
    }
    
    // Since we will not use any callbacks on the accepted socket, we specify no
    // socketContext or callback dispatch table pointer for the accepted socket.
    *AcceptSocketContext = NULL;
    *AcceptSocketDispatch = NULL;

    return STATUS_SUCCESS;
}

// Operation handler for issuing a receive request on a connected socket
VOID
WskSampleOpReceive(
    _In_ PWSKSAMPLE_SOCKET_OP_CONTEXT SocketOpContext
    )
{
    PWSKSAMPLE_SOCKET_CONTEXT socketContext;

    PAGED_CODE();

    socketContext = SocketOpContext->SocketContext;

    if(socketContext->Closing || socketContext->Disconnecting) {
        // Do not call WskReceive if socket is being disconnected
        // or closed. The operation context will not be used any more.
        DoTraceMessage(TRCINFO, "OpReceive: %p %p SKIP", 
            socketContext, SocketOpContext);
    }
    else {
        WSK_BUF wskbuf;
        CONST WSK_PROVIDER_CONNECTION_DISPATCH *dispatch;

        dispatch = socketContext->Socket->Dispatch;

        wskbuf.Offset = 0;
        wskbuf.Length = SocketOpContext->BufferLength;
        wskbuf.Mdl = SocketOpContext->DataMdl;

        IoReuseIrp(SocketOpContext->Irp, STATUS_UNSUCCESSFUL);
        IoSetCompletionRoutine(SocketOpContext->Irp,
            WskSampleReceiveIrpCompletionRoutine,
            SocketOpContext, TRUE, TRUE, TRUE);

        DoTraceMessage(TRCINFO, "OpReceive: %p %p %Iu", 
            socketContext, SocketOpContext, wskbuf.Length);

        // No need to check the return status here. The IRP completion
        // routine will take action based on the completion status.
        dispatch->WskReceive(
            socketContext->Socket,
            &wskbuf,
            0,
            SocketOpContext->Irp);
    }
}

// IRP completion routine for WskReceive requests
_Use_decl_annotations_
NTSTATUS
WskSampleReceiveIrpCompletionRoutine(
    PDEVICE_OBJECT Reserved,
    PIRP Irp,
    PVOID Context
    )
{
    PWSKSAMPLE_SOCKET_OP_CONTEXT socketOpContext;
    PWSKSAMPLE_SOCKET_CONTEXT socketContext;

    UNREFERENCED_PARAMETER(Reserved);

    _Analysis_assume_(Context != NULL);

    socketOpContext = (PWSKSAMPLE_SOCKET_OP_CONTEXT)Context;
    socketContext = socketOpContext->SocketContext;

    DoTraceMessage(TRCINFO, 
        "ReceiveIrpCompletionRoutine: %p %p 0x%lx %Iu", 
        socketContext, socketOpContext, Irp->IoStatus.Status, 
        Irp->IoStatus.Information);

    if(!NT_SUCCESS(Irp->IoStatus.Status)) {
        // Receive failed. Enqueue an operation to close the socket
        WskSampleEnqueueOp(socketOpContext, WskSampleOpClose);
    }
    else {
        if(Irp->IoStatus.Information == 0) {
            // Successful receive completion with 0 bytes means the peer
            // has gracefully disconnected its half of the connection.
            // So, we enqueue an operation to disconnect our half.
            WskSampleEnqueueOp(socketOpContext, WskSampleOpDisconnect);
        }
        else {
            // Receive has completed with some data. So, we enqueue an
            // operation to send the data back. Note that the data
            // buffer is attached to the operation context that is being
            // queued. We just need to remember the actual length of
            // data received into the buffer.
            socketOpContext->DataLength = Irp->IoStatus.Information;
            WskSampleEnqueueOp(socketOpContext, WskSampleOpSend);                    
        }
    }
    
    return STATUS_MORE_PROCESSING_REQUIRED;
}

// Operation handler for issuing a send request on a connected socket
VOID
WskSampleOpSend(
    _In_ PWSKSAMPLE_SOCKET_OP_CONTEXT SocketOpContext
    )
{
    PWSKSAMPLE_SOCKET_CONTEXT socketContext;

    PAGED_CODE();

    socketContext = SocketOpContext->SocketContext;

    if(socketContext->Closing || socketContext->Disconnecting) {
        // Do not call WskSend if socket is being disconnected
        // or closed. The operation context will not be used any more.
        DoTraceMessage(TRCINFO, "OpSend: %p %p SKIP", 
            socketContext, SocketOpContext);
    }
    else {
        WSK_BUF wskbuf;
        CONST WSK_PROVIDER_CONNECTION_DISPATCH *dispatch;

        dispatch = socketContext->Socket->Dispatch;

        wskbuf.Offset = 0;
        wskbuf.Length = SocketOpContext->DataLength;
        wskbuf.Mdl = SocketOpContext->DataMdl;

        IoReuseIrp(SocketOpContext->Irp, STATUS_UNSUCCESSFUL);
        IoSetCompletionRoutine(SocketOpContext->Irp,
            WskSampleSendIrpCompletionRoutine,
            SocketOpContext, TRUE, TRUE, TRUE);

        DoTraceMessage(TRCINFO, "OpSend: %p %p %Iu", 
            socketContext, SocketOpContext, wskbuf.Length);

        // No need to check the return status here. The IRP completion
        // routine will take action based on the completion status.
        dispatch->WskSend(
            socketContext->Socket,
            &wskbuf,
            0,
            SocketOpContext->Irp);
    }
}

// IRP completion routine for WskSend requests
_Use_decl_annotations_
NTSTATUS
WskSampleSendIrpCompletionRoutine(
    PDEVICE_OBJECT Reserved,
    PIRP Irp,
    PVOID Context
    )
{
    PWSKSAMPLE_SOCKET_OP_CONTEXT socketOpContext;
    PWSKSAMPLE_SOCKET_CONTEXT socketContext;

    UNREFERENCED_PARAMETER(Reserved);

    _Analysis_assume_(Context != NULL);

    socketOpContext = (PWSKSAMPLE_SOCKET_OP_CONTEXT)Context;
    socketContext = socketOpContext->SocketContext;

    DoTraceMessage(TRCINFO, 
        "SendIrpCompletionRoutine: %p %p 0x%lx %Iu", socketContext,
        socketOpContext, Irp->IoStatus.Status, Irp->IoStatus.Information);

    if(!NT_SUCCESS(Irp->IoStatus.Status)) {
        // Send failed. Enqueue an operation to close the socket.
        WskSampleEnqueueOp(socketOpContext, WskSampleOpClose);
    }
    else {
        // Send succeeded. Enqueue an operation to receive more data.
        WskSampleEnqueueOp(socketOpContext, WskSampleOpReceive);                    
    }

    return STATUS_MORE_PROCESSING_REQUIRED;
}

// Operation handler for issuing a disconnect request on a connected socket
VOID
WskSampleOpDisconnect(
    _In_ PWSKSAMPLE_SOCKET_OP_CONTEXT SocketOpContext
    )
{
    PWSKSAMPLE_SOCKET_CONTEXT socketContext;

    PAGED_CODE();

    socketContext = SocketOpContext->SocketContext;

    if(socketContext->Closing || socketContext->Disconnecting) {
        // Do not call WskDisconnect if socket is already being
        // disconnected or closed. A disconnect operation may get
        // enqueued multiple times as a result of multiple outstanding
        // receive requests getting completed with success and 0 bytes.
        DoTraceMessage(TRCINFO, "OpDisconnect: %p %p SKIP", 
            socketContext, SocketOpContext);
    }
    else {
        CONST WSK_PROVIDER_CONNECTION_DISPATCH *dispatch;

        dispatch = socketContext->Socket->Dispatch;

        socketContext->Disconnecting = TRUE;
        
        IoReuseIrp(SocketOpContext->Irp, STATUS_UNSUCCESSFUL);
        IoSetCompletionRoutine(SocketOpContext->Irp,
            WskSampleDisconnectIrpCompletionRoutine,
            SocketOpContext, TRUE, TRUE, TRUE);

        DoTraceMessage(TRCINFO, "OpDisconnect: %p %p", 
            socketContext, SocketOpContext);

        // No need to check the return status here. The IRP completion
        // routine will take action based on the completion status.
        dispatch->WskDisconnect(
            socketContext->Socket,
            NULL,
            0,
            SocketOpContext->Irp);
    }
}

// IRP completion routine for WskDisconnect requests
_Use_decl_annotations_
NTSTATUS
WskSampleDisconnectIrpCompletionRoutine(
    PDEVICE_OBJECT Reserved,
    PIRP Irp,
    PVOID Context
    )
{
    PWSKSAMPLE_SOCKET_OP_CONTEXT socketOpContext;
    PWSKSAMPLE_SOCKET_CONTEXT socketContext;

    UNREFERENCED_PARAMETER(Reserved);
    UNREFERENCED_PARAMETER(Irp);

    _Analysis_assume_(Context != NULL);

    socketOpContext = (PWSKSAMPLE_SOCKET_OP_CONTEXT)Context;
    socketContext = socketOpContext->SocketContext;

    DoTraceMessage(TRCINFO, "DisconnectIrpCompletionRoutine: %p %p 0x%lx",
        socketContext, socketOpContext, Irp->IoStatus.Status);

    // Disconnect completed. Enqueue an operation to close the socket.
    WskSampleEnqueueOp(socketOpContext, WskSampleOpClose);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

// Operation handler for closing a socket
VOID
WskSampleOpClose(
    _In_ PWSKSAMPLE_SOCKET_OP_CONTEXT SocketOpContext
    )
{
    PWSKSAMPLE_SOCKET_CONTEXT socketContext;

    PAGED_CODE();

    socketContext = SocketOpContext->SocketContext;

    if(socketContext->Closing) {
        // Do not call WskClose if socket is already being closed.
        // A close operation may get enqueued multiple times as a result
        // of multiple outstanding send/receive/disconnect operations
        // getting completed with failure.
        DoTraceMessage(TRCINFO, "OpClose: %p %p SKIP", 
            socketContext, SocketOpContext);
    }
    else {
        CONST WSK_PROVIDER_BASIC_DISPATCH *dispatch;

        socketContext->Closing = TRUE;

        dispatch = socketContext->Socket->Dispatch;

        IoReuseIrp(SocketOpContext->Irp, STATUS_UNSUCCESSFUL);
        IoSetCompletionRoutine(SocketOpContext->Irp,
            WskSampleCloseIrpCompletionRoutine,
            SocketOpContext, TRUE, TRUE, TRUE);
        
        DoTraceMessage(TRCINFO,"OpClose: %p %p", socketContext,SocketOpContext);

        // No need to check the return status here. The IRP completion
        // routine will take action based on the completion status.
        dispatch->WskCloseSocket(
            socketContext->Socket,
            SocketOpContext->Irp);
    }
}

// IRP completion routine for WskCloseSocket requests
_Use_decl_annotations_
NTSTATUS
WskSampleCloseIrpCompletionRoutine(
    PDEVICE_OBJECT Reserved,
    PIRP Irp,
    PVOID Context
    )
{
    PWSKSAMPLE_SOCKET_OP_CONTEXT socketOpContext;
    PWSKSAMPLE_SOCKET_CONTEXT socketContext;

    UNREFERENCED_PARAMETER(Reserved);
    UNREFERENCED_PARAMETER(Irp);

    _Analysis_assume_(Context != NULL);

    socketOpContext = (PWSKSAMPLE_SOCKET_OP_CONTEXT)Context;
    socketContext = socketOpContext->SocketContext;

    // WskCloseSocket can never fail.
    ASSERT(NT_SUCCESS(Irp->IoStatus.Status));

    DoTraceMessage(TRCINFO, "CloseIrpCompletionRoutine: %p %p 0x%lx",
        socketContext, socketOpContext, Irp->IoStatus.Status);
    
    // Enqueue an operation to free the socket context. Since the 
    // completion of WskCloseSocket guarantees that there are no
    // outstanding requests or callbacks on the socket, we can
    // safely free the socket context. However, we still can NOT
    // free the socket context directly here since there may still be
    // queued operations not yet issued over the socket. Thus, we need
    // to queue this "free" operation behind any other queued 
    // operations (which will not be issued over the socket since the
    // socket context is marked as "closing"). Note that NO new 
    // operations can be queued behind the "free" operation for a
    // given socket since we enqueue new operations only from the
    // context of IRP completions on the socket.
    WskSampleEnqueueOp(socketOpContext, WskSampleOpFree);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

// Operation handler for freeing the context block for a closed socket
VOID
WskSampleOpFree(
    _In_ PWSKSAMPLE_SOCKET_OP_CONTEXT SocketOpContext
    )
{
    PWSKSAMPLE_SOCKET_CONTEXT socketContext;

    PAGED_CODE();

    socketContext = SocketOpContext->SocketContext;

    DoTraceMessage(TRCINFO, "OpFree: %p %p", socketContext, SocketOpContext);

    ASSERT(socketContext->Closing || socketContext->StopListening);
    WskSampleFreeSocketContext(socketContext);
}

