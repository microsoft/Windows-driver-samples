#include <ntddk.h>
#include <wdf.h>
#include <initguid.h>
#include <wdmguid.h>
#include <TrustedRuntimeClx.h>
#include "SampleMiniport.h"
#include "..\inc\SampleSecureService.h"
#include "..\inc\SampleOSService.h"

EVT_TR_CREATE_SECURE_SERVICE_CONTEXT TestServiceCreateSecureServiceContext;
EVT_TR_DESTROY_SECURE_SERVICE_CONTEXT TestServiceDestroySecureServiceContext;
EVT_TR_CONNECT_SECURE_SERVICE TestServiceConnectSecureService;
EVT_TR_DISCONNECT_SECURE_SERVICE TestServiceDisconnectSecureService;
EVT_TR_CREATE_SECURE_SERVICE_SESSION_CONTEXT TestServiceCreateSessionContext;
EVT_TR_DESTROY_SECURE_SERVICE_SESSION_CONTEXT TestServiceDestroySessionContext;
EVT_TR_PROCESS_SECURE_SERVICE_REQUEST TestServiceProcessSecureServiceRequest;
EVT_TR_CANCEL_SECURE_SERVICE_REQUEST TestServiceCancelSecureServiceRequest;
EVT_TR_PROCESS_OTHER_SECURE_SERVICE_IO TestServiceProcessOtherSecureServiceIo;

EVT_TR_PROCESS_SECURE_SERVICE_REQUEST Test2ServiceProcessSecureServiceRequest;

#pragma data_seg("PAGED")

TR_SECURE_SERVICE_CALLBACKS TestServiceCallbacks = {
    0,

    &TestServiceCreateSecureServiceContext,
    &TestServiceDestroySecureServiceContext,

    &TestServiceConnectSecureService,
    &TestServiceDisconnectSecureService,

    &TestServiceCreateSessionContext,
    &TestServiceDestroySessionContext,

    &TestServiceProcessSecureServiceRequest,
    &TestServiceCancelSecureServiceRequest,
    &TestServiceProcessOtherSecureServiceIo
};

TR_SECURE_SERVICE_CALLBACKS Test2ServiceCallbacks = {
    0,

    &TestServiceCreateSecureServiceContext,
    &TestServiceDestroySecureServiceContext,

    &TestServiceConnectSecureService,
    &TestServiceDisconnectSecureService,

    &TestServiceCreateSessionContext,
    &TestServiceDestroySessionContext,

    &Test2ServiceProcessSecureServiceRequest,
    &TestServiceCancelSecureServiceRequest,
    &TestServiceProcessOtherSecureServiceIo
};

#pragma data_seg()

typedef struct _TEST_SERVICE_CONTEXT {
    GUID ServiceGuid;
} TEST_SERVICE_CONTEXT, *PTEST_SERVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE(TEST_SERVICE_CONTEXT);

//
// Macro to convert between forced 64-bit pointers to native
//
#define TruncatePointer64(_Pointer) ((PVOID)(ULONG_PTR)(ULONG64)(_Pointer))

_Use_decl_annotations_
NTSTATUS
TestServiceCreateSecureServiceContext(
    WDFDEVICE MasterDevice,
    LPCGUID ServiceGuid,
    WDFDEVICE ServiceDevice
    )

/*++

    Routine Description:

        This routine is called when a secure service is being created.

    Arguments:

        MasterDevice - Supplies a handle to the master device object.

        ServiceGuid - Supplies GUID of the secure service.

        ServiceDevice - Supplies a handle to the new service device object
                        being created.

    Return Value:

        NTSTATUS code.

--*/

{

    WDF_OBJECT_ATTRIBUTES ContextAttributes;
    PTEST_SERVICE_CONTEXT ServiceContext;
    NTSTATUS Status;

    PAGED_CODE();
    
    UNREFERENCED_PARAMETER(MasterDevice);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&ContextAttributes,
                                            TEST_SERVICE_CONTEXT);

    Status = WdfObjectAllocateContext(ServiceDevice,
                                      &ContextAttributes,
                                      &ServiceContext);

    if (!NT_SUCCESS(Status)) {
        goto TestServiceCreateSecureServiceContextEnd;
    }

    ServiceContext->ServiceGuid = *ServiceGuid;

TestServiceCreateSecureServiceContextEnd:
    return Status;
}

_Use_decl_annotations_
NTSTATUS
TestServiceDestroySecureServiceContext(
    WDFDEVICE ServiceDevice
    )

/*++

    Routine Description:

        This routine is called when a secure service is being destroyed.

    Arguments:

        ServiceDevice - Supplies a handle to the service device object.

    Return Value:

        NTSTATUS code.

--*/

{

    PAGED_CODE();

    UNREFERENCED_PARAMETER(ServiceDevice);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
TestServiceConnectSecureService(
    WDFDEVICE ServiceDevice
    )

/*++

    Routine Description:

        This routine is called when a secure service is being used for the
        first time or has returned from a power-state change.

    Arguments:

        ServiceDevice - Supplies a handle to the service device object.

    Return Value:

        NTSTATUS code.

--*/

{

    PAGED_CODE();

    UNREFERENCED_PARAMETER(ServiceDevice);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
TestServiceDisconnectSecureService(
    WDFDEVICE ServiceDevice
    )

/*++

    Routine Description:

        This routine is called to disconnect a secure service in preparation
        for a possible power-state change.

    Arguments:

        ServiceDevice - Supplies a handle to the service device object.

    Return Value:

        NTSTATUS code.

--*/

{

    UNREFERENCED_PARAMETER(ServiceDevice);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
TestServiceCreateSessionContext(
    WDFDEVICE ServiceDevice,
    WDFOBJECT *SessionContextObject
    )

/*++

    Routine Description:

        This routine is called on creation of a new session to a secure
        service. This can be used to track any state through multiple requests
        from the same client.

    Arguments:

        ServiceDevice - Supplies a handle to the service device object.

        SessionContext - Supplies a pointer to hold any session state that may
                         be required for future calls.

    Return Value:

        NTSTATUS code.

--*/

{

    PAGED_CODE();

    UNREFERENCED_PARAMETER(ServiceDevice);
    UNREFERENCED_PARAMETER(SessionContextObject);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
TestServiceDestroySessionContext(
    WDFDEVICE ServiceDevice,
    WDFOBJECT *SessionContextObject
    )

/*++

    Routine Description:

        This routine is called on destruction a session.

    Arguments:

        ServiceDevice - Supplies a handle to the service device object.

        SessionContext - Supplies a pointer to the session context.

    Return Value:

        NTSTATUS code.

--*/

{

    PAGED_CODE();

    UNREFERENCED_PARAMETER(ServiceDevice);
    UNREFERENCED_PARAMETER(SessionContextObject);

    return STATUS_SUCCESS;
}

typedef
NTSTATUS
TREE_TEST_SERVICE_REQUEST_HANDLER(
    _In_ WDFDEVICE ServiceDevice,
    _In_ PTEST_SERVICE_CONTEXT ServiceContext,
    _In_ PVOID RequestHandle,
    _In_ KPRIORITY Priority,
    _In_ PTR_SERVICE_REQUEST Request,
    _In_ ULONG Flags,
    _Out_ PULONG_PTR BytesWritten,
    _Inout_opt_ PVOID* RequestContext
    );

TREE_TEST_SERVICE_REQUEST_HANDLER TestServiceHelloWorld;
TREE_TEST_SERVICE_REQUEST_HANDLER TestServiceInterruptTime;
TREE_TEST_SERVICE_REQUEST_HANDLER TestServiceKernelOnly;
TREE_TEST_SERVICE_REQUEST_HANDLER TestServiceDelayedCompletion;

TREE_TEST_SERVICE_REQUEST_HANDLER *TestServiceDispatch[] = {
    NULL,
    TestServiceHelloWorld,
    TestServiceInterruptTime,
    TestServiceKernelOnly,
    TestServiceDelayedCompletion,
};

TREE_TEST_SERVICE_REQUEST_HANDLER Test2ServiceEchoTest;
TREE_TEST_SERVICE_REQUEST_HANDLER Test2ServiceTwiceReversed;

TREE_TEST_SERVICE_REQUEST_HANDLER *Test2ServiceDispatch[] = {
    NULL,
    Test2ServiceEchoTest,
    Test2ServiceTwiceReversed,
};

_Use_decl_annotations_
NTSTATUS
TestServiceProcessSecureServiceRequest(
    WDFDEVICE ServiceDevice,
    WDFOBJECT SessionContextObject,
    PVOID RequestHandle,
    KPRIORITY Priority,
    PTR_SERVICE_REQUEST Request,
    ULONG Flags,
    PULONG_PTR BytesWritten,
    PVOID* RequestContext
    )

/*++

    Routine Description:

        This routine is called to process a request to the secure service.
        This is typically the only way communication would be done to a secure
        service.

    Arguments:

        ServiceDevice - Supplies a handle to the service device object.

        SessionContext - Supplies a pointer to the context.

        RequestHandle - Supplies a pointer to a request handle that will be
                        used if the operation completes asynchronously.

        Priority - Supplies the priority of the request.

        Request - Supplies a pointer to the data for the request.

        RequestorMode - Supplies where the request originated from.

        BytesWritten - Supplies a pointer to be filled out with the number of
                       bytes written.

        RequestContext - Supplies a pointer to a PVOID variable where
                         additional information needed to cancel the request.
                         It will be provided in TestServiceCancelSecureServiceRequest.

    Return Value:

        NTSTATUS code.

--*/

{

    PTEST_SERVICE_CONTEXT ServiceContext;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(SessionContextObject);
    UNREFERENCED_PARAMETER(RequestHandle);
    UNREFERENCED_PARAMETER(Priority);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(RequestContext);

    *BytesWritten = 0;
    if ((Request->FunctionCode == 0) || 
        (Request->FunctionCode >= sizeof(TestServiceDispatch)/sizeof(TestServiceDispatch[0]))) {

        return STATUS_INVALID_PARAMETER;
    }

    ServiceContext = WdfObjectGet_TEST_SERVICE_CONTEXT(ServiceDevice);
    return (*TestServiceDispatch[Request->FunctionCode])(ServiceDevice,
                                                         ServiceContext,
                                                         RequestHandle,
                                                         Priority,
                                                         Request,
                                                         Flags,
                                                         BytesWritten,
                                                         RequestContext);
}

#pragma region Test service request handlers

#define STR_HELLO_WORLD L"Hello, world!"

_Use_decl_annotations_
NTSTATUS
TestServiceHelloWorld(
    WDFDEVICE ServiceDevice,
    PTEST_SERVICE_CONTEXT ServiceContext,
    PVOID RequestHandle,
    KPRIORITY Priority,
    PTR_SERVICE_REQUEST Request,
    ULONG Flags,
    PULONG_PTR BytesWritten,
    PVOID* RequestContext
    )

/*++

    Routine Description:

        This routine will write classic L"Hello, world!" message to the output
        buffer. If output buffer is too small, BytesWritten field in the
        response struct will contain the required size.

    Arguments:

        ServiceContext - Supplies a pointer to service device context object.

        Refer to TestServiceProcessSecureServiceRequest for the rest.

    Return Value:

        NTSTATUS code.

--*/

{

    PAGED_CODE();

    UNREFERENCED_PARAMETER(ServiceDevice);
    UNREFERENCED_PARAMETER(ServiceContext);
    UNREFERENCED_PARAMETER(RequestHandle);
    UNREFERENCED_PARAMETER(Priority);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(RequestContext);

    *BytesWritten = sizeof(STR_HELLO_WORLD);
    if (Request->OutputBufferSize < sizeof(STR_HELLO_WORLD)) {
        //
        // STATUS_BUFFER_OVERFLOW must be used to make WDF forward the value of
        // *BytesWritten to let Win32 caller know how large the output buffer
        // should be.
        //
        return STATUS_BUFFER_OVERFLOW;
    }

    RtlCopyMemory(Request->OutputBuffer, STR_HELLO_WORLD, sizeof(STR_HELLO_WORLD));

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
TestServiceInterruptTime(
    WDFDEVICE ServiceDevice,
    PTEST_SERVICE_CONTEXT ServiceContext,
    PVOID RequestHandle,
    KPRIORITY Priority,
    PTR_SERVICE_REQUEST Request,
    ULONG Flags,
    PULONG_PTR BytesWritten,
    PVOID* RequestContext
    )

/*++

    Routine Description:

        This routine will write the value returned from
        KeQueryInterruptTimePrecise in the output buffer.

    Arguments:

        ServiceContext - Supplies a pointer to service device context object.

        Refer to TestServiceProcessSecureServiceRequest for the rest.

    Return Value:

        NTSTATUS code.

--*/

{
    ULONG64 QpcTimestamp;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(ServiceDevice);
    UNREFERENCED_PARAMETER(ServiceContext);
    UNREFERENCED_PARAMETER(RequestHandle);
    UNREFERENCED_PARAMETER(Priority);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(RequestContext);

    *BytesWritten = sizeof(ULONG64);
    if (Request->OutputBufferSize < sizeof(ULONG64)) {
        //
        // STATUS_BUFFER_OVERFLOW must be used to make WDF forward the value of
        // *BytesWritten to let Win32 caller know how large the output buffer
        // should be.
        //
        return STATUS_BUFFER_OVERFLOW;
    }

    *(PULONG64)TruncatePointer64(Request->OutputBuffer) = KeQueryInterruptTimePrecise(&QpcTimestamp);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
TestServiceKernelOnly(
    WDFDEVICE ServiceDevice,
    PTEST_SERVICE_CONTEXT ServiceContext,
    PVOID RequestHandle,
    KPRIORITY Priority,
    PTR_SERVICE_REQUEST Request,
    ULONG Flags,
    PULONG_PTR BytesWritten,
    PVOID* RequestContext
    )

/*++

    Routine Description:

        This routine does nothing. However, if this request was sent from
        usermode, it will complete with failure status code.

    Arguments:

        ServiceContext - Supplies a pointer to service device context object.

        Refer to TestServiceProcessSecureServiceRequest for the rest.

    Return Value:

        NTSTATUS code.

--*/

{

    PAGED_CODE();

    UNREFERENCED_PARAMETER(ServiceDevice);
    UNREFERENCED_PARAMETER(ServiceContext);
    UNREFERENCED_PARAMETER(RequestHandle);
    UNREFERENCED_PARAMETER(Priority);
    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(RequestContext);

    *BytesWritten = 0;
    if (Flags & TR_SERVICE_REQUEST_FROM_USERMODE) {
        return STATUS_ACCESS_DENIED;

    } else {

        return STATUS_SUCCESS;
    }
}

typedef struct _DELAYED_COMPLETION_TIMER_CONTEXT {
    union {
        PVOID RequestHandle;
        WDFREQUEST Request;
    } Request;
    WDFWORKITEM WorkItem;
    PVOID OutputBuffer;
} DELAYED_COMPLETION_TIMER_CONTEXT, *PDELAYED_COMPLETION_TIMER_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE(DELAYED_COMPLETION_TIMER_CONTEXT);

EVT_WDF_TIMER DelayedCompletionTimerCallback;
EVT_WDF_WORKITEM DelayedCompletionWorkItemCallback;

_Use_decl_annotations_
VOID
DelayedCompletionTimerCallback(
    WDFTIMER Timer
    )

/*++

    Routine Description:

        This routine is the callback for the timer used in delayed completion
        request handler. It'll enqueue a work item to complete the request and
        clean up resources.

    Arguments:

        Timer - Supplies a handle to the timer.

    Return Value:

        None.

--*/

{
    PDELAYED_COMPLETION_TIMER_CONTEXT Context;

    Context = WdfObjectGet_DELAYED_COMPLETION_TIMER_CONTEXT(Timer);
    WdfWorkItemEnqueue(Context->WorkItem);
}

_Use_decl_annotations_
VOID
DelayedCompletionWorkItemCallback(
    WDFWORKITEM WorkItem
    )

/*++

    Routine Description:

        This routine is enqueued from delayed completion timer callback. Since
        a timer cannot be deleted inside its handler, it is deleted here
        instead. Also the request is completed here.

    Arguments:

        WorkItem - Supplies a handle to this work item.

    Return Value:

        None.

--*/

{
    PDELAYED_COMPLETION_TIMER_CONTEXT Context;
    WDFTIMER Timer;

    PAGED_CODE();

    Timer = (WDFTIMER)WdfWorkItemGetParentObject(WorkItem);
    Context = WdfObjectGet_DELAYED_COMPLETION_TIMER_CONTEXT(Timer);
    *(PULONG)Context->OutputBuffer = 0x12345678;

    //
    // The request couldn't have been canceled if we reach here. The
    // cancellation routine tries to stop the timer and prevent callback from
    // being called. If it succeeds, this routine never runs. If it fails, the
    // request is not canceled.
    //
    TrSecureDeviceCompleteAsyncRequest(Context->Request.RequestHandle,
                                       STATUS_SUCCESS,
                                       sizeof(ULONG));

    WdfObjectDelete(Timer);
}

_Use_decl_annotations_
NTSTATUS                       
TestServiceDelayedCompletion(
    WDFDEVICE ServiceDevice,
    PTEST_SERVICE_CONTEXT ServiceContext,
    PVOID RequestHandle,
    KPRIORITY Priority,
    PTR_SERVICE_REQUEST Request,
    ULONG Flags,
    PULONG_PTR BytesWritten,
    PVOID* RequestContext
    )

/*++

    Routine Description:

        This routine completes the request after specified delay. The delay is
        given in input buffer as ULONG in units of milliseconds. When
        completed, output buffer will contain 0x12345678 ULONG value.

    Arguments:

        ServiceContext - Supplies a pointer to service device context object.

        Refer to TestServiceProcessSecureServiceRequest for the rest.

    Return Value:

        NTSTATUS code.

--*/

{
    PDELAYED_COMPLETION_TIMER_CONTEXT Context;
    NTSTATUS Status;
    WDFTIMER Timer;
    WDF_OBJECT_ATTRIBUTES TimerAttributes;
    WDF_TIMER_CONFIG TimerConfig;
    WDF_OBJECT_ATTRIBUTES WorkItemAttributes;
    WDF_WORKITEM_CONFIG WorkItemConfig;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(ServiceDevice);
    UNREFERENCED_PARAMETER(ServiceContext);
    UNREFERENCED_PARAMETER(RequestHandle);
    UNREFERENCED_PARAMETER(Priority);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(BytesWritten);
    UNREFERENCED_PARAMETER(RequestContext);

    if (Request->InputBufferSize < sizeof(ULONG)) {
        return STATUS_INVALID_PARAMETER;
    }

    if (Request->OutputBufferSize < sizeof(ULONG)) {
        //
        // STATUS_BUFFER_OVERFLOW must be used to make WDF forward the value of
        // *BytesWritten to let Win32 caller know how large the output buffer
        // should be.
        //
        *BytesWritten = sizeof(ULONG);
        return STATUS_BUFFER_OVERFLOW;
    }

    *BytesWritten = 0;
    Timer = NULL;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&TimerAttributes, DELAYED_COMPLETION_TIMER_CONTEXT);
    WDF_TIMER_CONFIG_INIT(&TimerConfig, DelayedCompletionTimerCallback);
    TimerAttributes.ExecutionLevel = WdfExecutionLevelDispatch;
    TimerAttributes.ParentObject = ServiceDevice;
    Status = WdfTimerCreate(&TimerConfig, &TimerAttributes, &Timer);
    if (!NT_SUCCESS(Status)) {
        goto TestServiceDelayedCompletionEnd;
    }

    Context = WdfObjectGet_DELAYED_COMPLETION_TIMER_CONTEXT(Timer);
    Context->Request.RequestHandle = RequestHandle;
    Context->OutputBuffer = TruncatePointer64(Request->OutputBuffer);

    WDF_OBJECT_ATTRIBUTES_INIT(&WorkItemAttributes);
    WDF_WORKITEM_CONFIG_INIT(&WorkItemConfig, DelayedCompletionWorkItemCallback);
    WorkItemAttributes.ParentObject = Timer;
    Status = WdfWorkItemCreate(&WorkItemConfig, &WorkItemAttributes, &Context->WorkItem);
    if (!NT_SUCCESS(Status)) {
        goto TestServiceDelayedCompletionEnd;
    }

    //
    // Set timer due after given delay
    //
    WdfTimerStart(Timer, WDF_REL_TIMEOUT_IN_MS(*(PULONG)TruncatePointer64(Request->InputBuffer)));
    Status = STATUS_PENDING;

    //
    // RequestContext stores the data that is needed when canceling the request
    //
    *RequestContext = (PVOID)Timer;

TestServiceDelayedCompletionEnd:
    if (!NT_SUCCESS(Status)) {
        if (Timer != NULL) {
            WdfObjectDelete(Timer);
        }

        //
        // WorkItem is automatically deleted when parent object (Timer) is
        // destroyed.
        //
    }

    return Status;
}

#pragma endregion

_Use_decl_annotations_
NTSTATUS
Test2ServiceProcessSecureServiceRequest(
    WDFDEVICE ServiceDevice,
    WDFOBJECT SessionContextObject,
    PVOID RequestHandle,
    KPRIORITY Priority,
    PTR_SERVICE_REQUEST Request,
    ULONG Flags,
    PULONG_PTR BytesWritten,
    PVOID* RequestContext
    )

/*++

    Routine Description:

        This routine is called to process a request to the secure service.
        This is typically the only way communication would be done to a secure
        service.

    Arguments:

        ServiceDevice - Supplies a handle to the service device object.

        SessionContext - Supplies a pointer to the context.

        RequestHandle - Supplies a pointer to a request handle that will be
                        used if the operation completes asynchronously.

        Priority - Supplies the priority of the request.

        Request - Supplies a pointer to the data for the request.

        RequestorMode - Supplies where the request originated from.

        BytesWritten - Supplies a pointer to be filled out with the number of
                       bytes written.

        RequestContext - Supplies a pointer to a PVOID variable where
                         additional information needed to cancel the request.
                         It will be provided in TestServiceCancelSecureServiceRequest.
                         This address is valid until the request is completed
                         or canceled.

    Return Value:

        NTSTATUS code.

--*/

{

    PTEST_SERVICE_CONTEXT ServiceContext;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(SessionContextObject);
    UNREFERENCED_PARAMETER(RequestHandle);
    UNREFERENCED_PARAMETER(Priority);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(RequestContext);

    *BytesWritten = 0;
    if ((Request->FunctionCode == 0) || 
        (Request->FunctionCode >= sizeof(Test2ServiceDispatch)/sizeof(Test2ServiceDispatch[0]))) {

        return STATUS_INVALID_PARAMETER;
    }

    ServiceContext = WdfObjectGet_TEST_SERVICE_CONTEXT(ServiceDevice);
    return (*Test2ServiceDispatch[Request->FunctionCode])(ServiceDevice,
                                                          ServiceContext,
                                                          RequestHandle,
                                                          Priority,
                                                          Request,
                                                          Flags,
                                                          BytesWritten,
                                                          RequestContext);
}

#pragma region Test2 service request handlers

_Use_decl_annotations_
NTSTATUS
Test2ServiceEchoTest(
    WDFDEVICE ServiceDevice,
    PTEST_SERVICE_CONTEXT ServiceContext,
    PVOID RequestHandle,
    KPRIORITY Priority,
    PTR_SERVICE_REQUEST Request,
    ULONG Flags,
    PULONG_PTR BytesWritten,
    PVOID* RequestContext
    )
{
    TR_SERVICE_REQUEST OSServiceRequest = {0};
    NTSTATUS Status;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(ServiceDevice);
    UNREFERENCED_PARAMETER(ServiceContext);
    UNREFERENCED_PARAMETER(RequestHandle);
    UNREFERENCED_PARAMETER(Priority);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(RequestContext);

    OSServiceRequest.FunctionCode = ECHO_SERVICE_ECHO;
    OSServiceRequest.InputBuffer = Request->InputBuffer;
    OSServiceRequest.InputBufferSize = Request->InputBufferSize;
    OSServiceRequest.OutputBuffer = Request->OutputBuffer;
    OSServiceRequest.OutputBufferSize = Request->OutputBufferSize;
    Status = TrSecureDeviceCallOSService(ServiceDevice,
                                         &GUID_ECHO_SERVICE,
                                         &OSServiceRequest,
                                         BytesWritten);

    //
    // OS service calls never returns STATUS_PENDING. No need to take care of
    // asynchronous processing.
    //

    NT_ASSERT(Status != STATUS_PENDING);

    return Status;
}

_Use_decl_annotations_
NTSTATUS
Test2ServiceTwiceReversed(
    WDFDEVICE ServiceDevice,
    PTEST_SERVICE_CONTEXT ServiceContext,
    PVOID RequestHandle,
    KPRIORITY Priority,
    PTR_SERVICE_REQUEST Request,
    ULONG Flags,
    PULONG_PTR BytesWritten,
    PVOID* RequestContext
    )
{
    PVOID TemporaryBuffer;
    ULONG_PTR OSServiceBytesWritten;
    TR_SERVICE_REQUEST OSServiceRequest = {0};
    NTSTATUS Status;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(ServiceContext);
    UNREFERENCED_PARAMETER(Priority);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(RequestHandle);
    UNREFERENCED_PARAMETER(RequestContext);

    TemporaryBuffer = NULL;

    *BytesWritten = (ULONG_PTR)(Request->InputBufferSize * 2);
    if (Request->OutputBufferSize < Request->InputBufferSize * 2) {
        //
        // STATUS_BUFFER_OVERFLOW must be used to make WDF forward the value of
        // *BytesWritten to let Win32 caller know how large the output buffer
        // should be.
        //
        Status = STATUS_BUFFER_OVERFLOW;
        goto TestServiceTwiceReversedEnd;
    }

    TemporaryBuffer = ExAllocatePool2(PagedPool,
                                            (SIZE_T)Request->InputBufferSize,
                                            'PMET');

    if (TemporaryBuffer == NULL) {
        *BytesWritten = 0;
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto TestServiceTwiceReversedEnd;
    }

    OSServiceRequest.FunctionCode = ECHO_SERVICE_REVERSE;
    OSServiceRequest.InputBuffer = Request->InputBuffer;
    OSServiceRequest.InputBufferSize = Request->InputBufferSize;
    OSServiceRequest.OutputBuffer = TemporaryBuffer;
    OSServiceRequest.OutputBufferSize = Request->InputBufferSize;
    Status = TrSecureDeviceCallOSService(ServiceDevice,
                                         &GUID_ECHO_SERVICE,
                                         &OSServiceRequest,
                                         &OSServiceBytesWritten);

    NT_ASSERT(Status != STATUS_PENDING);
    NT_ASSERT(OSServiceBytesWritten == Request->InputBufferSize);

    if (!NT_SUCCESS(Status)) {
        goto TestServiceTwiceReversedEnd;
    }

    OSServiceRequest.FunctionCode = ECHO_SERVICE_REPEAT;
    OSServiceRequest.InputBuffer = TemporaryBuffer;
    OSServiceRequest.InputBufferSize = Request->InputBufferSize;
    OSServiceRequest.OutputBuffer = Request->OutputBuffer;
    OSServiceRequest.OutputBufferSize = Request->InputBufferSize * 2;
    Status = TrSecureDeviceCallOSService(ServiceDevice,
                                         &GUID_ECHO_SERVICE,
                                         &OSServiceRequest,
                                         &OSServiceBytesWritten);

    NT_ASSERT(Status != STATUS_PENDING);
    NT_ASSERT(OSServiceBytesWritten == Request->InputBufferSize * 2);

TestServiceTwiceReversedEnd:
    if (TemporaryBuffer != NULL) {
        ExFreePool(TemporaryBuffer);
    }

    return Status;
}

#pragma endregion

_Use_decl_annotations_
VOID
TestServiceCancelSecureServiceRequest(
    WDFDEVICE ServiceDevice,
    WDFOBJECT SessionContextObject,
    PVOID RequestHandle,
    PVOID* RequestContext
    )

/*++

    Routine Description:

        This routine is called to cancel a request made via a previous call to
        TestServiceProcessSecureServiceRequest. Note that cancellation is
        best-effort, and on success would result in STATUS_CANCELLED being
        returned from the original request.

    Arguments:

        ServiceDevice - Supplies a handle to the service device object.

        SessionContext - Supplies a pointer to the context.

        RequestHandle - Supplies the request handle for which a cancellation
                        is being requested.

        RequestContext - Supplies a pointer to a PVOID variable where
                         additional information needed to cancel the request
                         set in TestServiceProcessSecureServiceRequest.

    Return Value:

        NTSTATUS code.

--*/

{

    BOOLEAN Stopped;
    WDFTIMER Timer;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(ServiceDevice);
    UNREFERENCED_PARAMETER(SessionContextObject);

    if (RequestContext != NULL) {
        //
        // This request is the delayed completion request. Try to cancel the
        // request by stopping the timer.
        //
        Timer = (WDFTIMER)*RequestContext;
        Stopped = WdfTimerStop(Timer, FALSE);
        if (Stopped) {
            //
            // Complete the request with cancelled status.
            //
            TrSecureDeviceCompleteAsyncRequest(RequestHandle, STATUS_CANCELLED, 0);
            WdfObjectDelete(Timer);

        } else {
            //
            // The timer routine is already running or completed. The request
            // is going to be completed by timer and work item callback.
            //
        }
    } else {
        //
        // There's nothing we can do for other types of request.
        //
    }

    return;
}

typedef struct _OTHERIO_DELAYED_COMPLETION_CONTEXT {
    WDFTIMER Timer;
} OTHERIO_DELAYED_COMPLETION_CONTEXT, *POTHERIO_DELAYED_COMPLETION_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE(OTHERIO_DELAYED_COMPLETION_CONTEXT);

EVT_WDF_REQUEST_CANCEL OtherIoDelayedCompletionCancel;
EVT_WDF_WORKITEM OtherIoDelayedCompletionWorkItemCallback;

_Use_decl_annotations_
VOID
OtherIoDelayedCompletionCancel(
    WDFREQUEST Request
    )

/*++

    Routine Description:

        This routine is called to cancel delayed completion request in other
        service I/O. Other I/O path uses WDF request objects as it is, so we
        cannot reuse TestServiceCancelSecureServiceRequest.

    Arguments:

        Request - Supplies a handle to the request to be canceled.

    Return Value:

        None.

--*/

{

    POTHERIO_DELAYED_COMPLETION_CONTEXT RequestContext;
    BOOLEAN Stopped;
    WDFTIMER Timer;

    PAGED_CODE();

    RequestContext = WdfObjectGet_OTHERIO_DELAYED_COMPLETION_CONTEXT(Request);
    Timer = RequestContext->Timer;
    Stopped = WdfTimerStop(Timer, FALSE);
    if (Stopped) {
        WdfRequestComplete(Request, STATUS_CANCELLED);

        //
        // RequestContext is no more valid from here.
        //
        WdfObjectDelete(Timer);
    }
}

_Use_decl_annotations_
VOID
OtherIoDelayedCompletionWorkItemCallback(
    WDFWORKITEM WorkItem
    )

/*++

    Routine Description:

        This routine is enqueued from delayed completion timer callback. Since
        a timer cannot be deleted inside its handler, it is deleted here
        instead. Also the request is completed here.

    Arguments:

        WorkItem - Supplies a handle to this work item.

    Return Value:

        None.

--*/

{
    PDELAYED_COMPLETION_TIMER_CONTEXT Context;
    NTSTATUS Status;
    WDFTIMER Timer;

    PAGED_CODE();

    Timer = (WDFTIMER)WdfWorkItemGetParentObject(WorkItem);
    Context = WdfObjectGet_DELAYED_COMPLETION_TIMER_CONTEXT(Timer);
    *(PULONG)Context->OutputBuffer = 0x12345678;

    Status = WdfRequestUnmarkCancelable(Context->Request.Request);
    if (Status != STATUS_CANCELLED) {
        WdfRequestCompleteWithInformation(Context->Request.Request,
                                          STATUS_SUCCESS,
                                          sizeof(ULONG));
    }

    WdfObjectDelete(Timer);
}

_Use_decl_annotations_
VOID
TestServiceProcessOtherSecureServiceIo(
    WDFDEVICE ServiceDevice,
    WDFOBJECT SessionContextObject,
    WDFREQUEST Request
    )

/*++

    Routine Description:

        This routine is called when an unrecognized IO request is made to a
        secure service. This can be used to process private calls.

    Arguments:

        ServiceDevice - Supplies a handle to the service device object.

        SessionContext - Supplies a pointer to the context.

        Request - Supplies a pointer to the WDF request object.

    Return Value:

        NTSTATUS code.

--*/

{

    WDF_REQUEST_PARAMETERS Parameters;
    NTSTATUS Status;
    PVOID InputBuffer;
    PVOID OutputBuffer;
    ULONG_PTR BytesWritten;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(ServiceDevice);
    UNREFERENCED_PARAMETER(SessionContextObject);

    BytesWritten = 0;

    WDF_REQUEST_PARAMETERS_INIT(&Parameters);
    WdfRequestGetParameters(Request, &Parameters);
    switch (Parameters.Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_TEST_DELAYED_COMPLETION:
        {
            POTHERIO_DELAYED_COMPLETION_CONTEXT RequestContext;
            WDF_OBJECT_ATTRIBUTES RequestAttributes;
            WDFTIMER Timer;
            WDF_OBJECT_ATTRIBUTES TimerAttributes;
            WDF_TIMER_CONFIG TimerConfig;
            PDELAYED_COMPLETION_TIMER_CONTEXT TimerContext;
            WDF_OBJECT_ATTRIBUTES WorkItemAttributes;
            WDF_WORKITEM_CONFIG WorkItemConfig;

            Status = WdfRequestRetrieveInputBuffer(Request,
                                                   sizeof(ULONG),
                                                   &InputBuffer,
                                                   NULL);

            if (!NT_SUCCESS(Status)) {
                goto TestServiceProcessOtherSecureServiceIoEnd;
            }

            Status = WdfRequestRetrieveOutputBuffer(Request,
                                                    sizeof(ULONG),
                                                    &OutputBuffer,
                                                    NULL);

            if (!NT_SUCCESS(Status)) {
                BytesWritten = sizeof(ULONG);
                if (Status == STATUS_BUFFER_TOO_SMALL) {
                    //
                    // STATUS_BUFFER_OVERFLOW must be used to make WDF forward the value of
                    // *BytesWritten to let Win32 caller know how large the output buffer
                    // should be.
                    //
                    Status = STATUS_BUFFER_OVERFLOW;
                }

                goto TestServiceProcessOtherSecureServiceIoEnd;
            }

            WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(
                &RequestAttributes,
                OTHERIO_DELAYED_COMPLETION_CONTEXT);

            Status = WdfObjectAllocateContext(Request,
                                              &RequestAttributes,
                                              &RequestContext);

            if (!NT_SUCCESS(Status)) {
                goto TestServiceProcessOtherSecureServiceIoEnd;
            }

            WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(
                &TimerAttributes,
                DELAYED_COMPLETION_TIMER_CONTEXT);

            WDF_TIMER_CONFIG_INIT(&TimerConfig, DelayedCompletionTimerCallback);
            TimerAttributes.ExecutionLevel = WdfExecutionLevelDispatch;
            TimerAttributes.ParentObject = ServiceDevice;
            Status = WdfTimerCreate(&TimerConfig, &TimerAttributes, &Timer);
            if (!NT_SUCCESS(Status)) {
                goto TestServiceProcessOtherSecureServiceIoEnd;
            }

            RequestContext->Timer =Timer;

            TimerContext = WdfObjectGet_DELAYED_COMPLETION_TIMER_CONTEXT(Timer);
            TimerContext->Request.Request = Request;
            TimerContext->OutputBuffer = TruncatePointer64(OutputBuffer);

            WDF_OBJECT_ATTRIBUTES_INIT(&WorkItemAttributes);
            WDF_WORKITEM_CONFIG_INIT(&WorkItemConfig,
                                     OtherIoDelayedCompletionWorkItemCallback);

            WorkItemAttributes.ParentObject = Timer;
            Status = WdfWorkItemCreate(&WorkItemConfig,
                                       &WorkItemAttributes,
                                       &TimerContext->WorkItem);

            if (!NT_SUCCESS(Status)) {
                WdfObjectDelete(Timer);
                goto TestServiceProcessOtherSecureServiceIoEnd;
            }

            //
            // Set timer due after given delay
            //
            WdfTimerStart(Timer,
                          WDF_REL_TIMEOUT_IN_MS(
                            *(PULONG)TruncatePointer64(InputBuffer)));

            WdfRequestMarkCancelable(Request, OtherIoDelayedCompletionCancel);
            return;
        }

    default:
        Status = STATUS_INVALID_PARAMETER;
        break;
    }

TestServiceProcessOtherSecureServiceIoEnd:
    WdfRequestCompleteWithInformation(Request, Status, BytesWritten);
}
