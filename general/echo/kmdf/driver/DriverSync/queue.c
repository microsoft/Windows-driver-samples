/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

    queue.c

Abstract:

    This is a C version of a very simple sample driver that illustrates
    how to use the driver framework and demonstrates best practices.

--*/

#include "driver.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, EchoQueueInitialize)
#pragma alloc_text (PAGE, EchoTimerCreate)
#endif

LONG
EchoInterlockedIncrementFloor(
    LONG volatile *Target,
    LONG Floor
    )
/*++

Routine Description:
    This routine will interlock increment a value only if the current value
    is greater then the floor value.

    The volatile keyword on the Target pointer is absolutely required, otherwise
    the compiler might rearrange pointer dereferences and that cannot happen.

Arguments:
    Target - the  value that will be pontetially incrmented

    Floor - the value in which the Target value must be greater then if it is
            to be incremented

Return Value:
    The current value of Target.  To detect failure, the return value will be
    <= Floor + 1.  It is +1 because we cannot increment from the Floor value
    itself, so Floor+1 cannot be a successful return value.

  --*/
{
    LONG oldValue, currentValue;

    currentValue = *Target;

    do {
        if (currentValue <= Floor) {
            return currentValue;
        }

        oldValue = currentValue;

        //
        // currentValue will be the value that used to be Target if the exchange
        // was made or its current value if the exchange was not made.
        //
        currentValue = InterlockedCompareExchange(Target, oldValue + 1, oldValue);

        //
        // If oldValue == currentValue, then no one updated Target in between
        // the deref at the top and the InterlockecCompareExchange afterward
        // and we have successfully incremented the value and can exit the loop.
        //
    } while (oldValue != currentValue);

    //
    // Since InterlockedIncrement returns the new incremented value of Target,
    // we should do the same here.
    //
    return oldValue + 1;
}

FORCEINLINE
LONG
EchoInterlockedIncrementGTZero(
    IN OUT LONG  volatile *Target
    )
/*++

Routine Description:
    Increment the value only if it is currently > 0.

Arguments:
    Target - the value to be incremented.  NOTE:  the volatile keyword is requreid

Return Value:
    Upon success, a value > 0.  Upon failure, a value <= 0.

  --*/
{
    return EchoInterlockedIncrementFloor(Target, 0);
}

NTSTATUS
EchoQueueInitialize(
    WDFDEVICE Device
    )
/*++

Routine Description:


     The I/O dispatch callbacks for the frameworks device object
     are configured in this function.

     A single default I/O Queue is configured for serial request
     processing, and a driver context memory allocation is created
     to hold our structure QUEUE_CONTEXT.

     This memory may be used by the driver automatically synchronized
     by the Queue's presentation lock.

     The lifetime of this memory is tied to the lifetime of the I/O
     Queue object, and we register an optional destructor callback
     to release any private allocations, and/or resources.


Arguments:

    Device - Handle to a framework device object.

Return Value:

    NTSTATUS

--*/
{
    WDFQUEUE queue;
    NTSTATUS status;
    PQUEUE_CONTEXT queueContext;
    WDF_IO_QUEUE_CONFIG queueConfig;
    WDF_OBJECT_ATTRIBUTES attributes;

    PAGED_CODE();

    //
    // Configure a default queue so that requests that are not
    // configure-fowarded using WdfDeviceConfigureRequestDispatching to goto
    // other queues get dispatched here.
    //
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
         &queueConfig,
        WdfIoQueueDispatchSequential
        );

    queueConfig.EvtIoRead   = EchoEvtIoRead;
    queueConfig.EvtIoWrite  = EchoEvtIoWrite;

    //
    // Fill in a callback for destroy, and our QUEUE_CONTEXT size
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, QUEUE_CONTEXT);
    attributes.EvtDestroyCallback = EchoEvtIoQueueContextDestroy;

    status = WdfIoQueueCreate(
        Device,
        &queueConfig,
        &attributes,
        &queue
        );

    if( !NT_SUCCESS(status) ) {
        KdPrint(("WdfIoQueueCreate failed 0x%x\n",status));
        return status;
    }

    // Get our Driver Context memory from the returned Queue handle
    queueContext = QueueGetContext(queue);

    queueContext->Buffer = NULL;
    queueContext->Timer = NULL;

    queueContext->CurrentRequest = NULL;
    queueContext->CurrentStatus = STATUS_INVALID_DEVICE_REQUEST;

    //
    // Create the SpinLock.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = queue;

    status = WdfSpinLockCreate(&attributes, &queueContext->SpinLock);
    if (!NT_SUCCESS(status)) {
        KdPrint(("WdfSpinLockCreate failed 0x%x\n",status));
        return status;
    }
    
    //
    // Create the Queue timer
    //
    status = EchoTimerCreate(&queueContext->Timer, TIMER_PERIOD, queue);
    if (!NT_SUCCESS(status)) {
        KdPrint(("Error creating timer 0x%x\n",status));
        return status;
    }

    return status;
}


NTSTATUS
EchoTimerCreate(
    IN WDFTIMER*       Timer,
    IN ULONG           Period,
    IN WDFQUEUE        Queue
    )
/*++

Routine Description:

    Subroutine to create periodic timer.

Arguments:


Return Value:

    NTSTATUS

--*/
{
    NTSTATUS Status;
    WDF_TIMER_CONFIG       timerConfig;
    WDF_OBJECT_ATTRIBUTES  timerAttributes;

    PAGED_CODE();

    //
    // Create a WDFTIMER object
    //
    WDF_TIMER_CONFIG_INIT_PERIODIC(&timerConfig, EchoEvtTimerFunc, Period);

    WDF_OBJECT_ATTRIBUTES_INIT(&timerAttributes);

    //
    // We are explicitly *not* serializing against the queue's lock, we will do
    // that on our own.
    //
    timerAttributes.ParentObject = Queue;

    Status = WdfTimerCreate(
        &timerConfig,
        &timerAttributes,
        Timer     // Output handle
        );

    return Status;
}



VOID
EchoEvtIoQueueContextDestroy(
    WDFOBJECT Object
)
/*++

Routine Description:

    This is called when the Queue that our driver context memory
    is associated with is destroyed.

Arguments:

    Context - Context that's being freed.

Return Value:

    VOID

--*/
{
    PQUEUE_CONTEXT queueContext = QueueGetContext(Object);

    //
    // Release any resources pointed to in the queue context.
    //
    // The body of the queue context will be released after
    // this callback handler returns
    //

    //
    // If Queue context has an I/O buffer, release it
    //
    if( queueContext->Buffer != NULL ) {
        ExFreePool(queueContext->Buffer);
        queueContext->Buffer = NULL;
    }

    return;
}

BOOLEAN
EchoDecrementRequestCancelOwnershipCount(
    PREQUEST_CONTEXT RequestContext
    )
/*++

Routine Description:
    Decrements the cancel ownership count for the request.  When the count
    reaches zero ownership has been acquired.

Arguments:
    RequestContext - the context which holds the count

Return Value:
    TRUE if the caller can complete the request, FALSE otherwise

  --*/
{
    LONG result;

    result = InterlockedDecrement(
        &RequestContext->CancelCompletionOwnershipCount
        );

    ASSERT(result >= 0);

    if (result == 0) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}

BOOLEAN
EchoIncrementRequestCancelOwnershipCount(
    PREQUEST_CONTEXT RequestContext
    )
/*++

Routine Description:
    Attempts to increment the request ownership count so that it cannot be
    completed until the count has been decremented

Arguments:
    RequestContext - context which holds the count

Return Value:
    TRUE if the count was incremented, FALSE otherwise

  --*/
{
    //
    // See comments in EchoInterlockedIncrementFloor as to why <= 1 is failure
    //
    if (EchoInterlockedIncrementGTZero(
            &RequestContext->CancelCompletionOwnershipCount
            ) <= 1) {
        return FALSE;
    }
    else {
        return TRUE;
    }
}

VOID
EchoEvtRequestCancel(
    IN WDFREQUEST Request
    )
/*++

Routine Description:


    Called when an I/O request is cancelled after the driver has marked
    the request cancellable. This callback is not automatically synchronized
    with the I/O callbacks since we have chosen not to use frameworks Device
    or Queue level locking.

Arguments:

    Request - Request being cancelled.

Return Value:

    VOID

--*/
{
    PQUEUE_CONTEXT queueContext;
    PREQUEST_CONTEXT requestContext;
    WDFQUEUE queue;
    BOOLEAN completeRequest;

    KdPrint(("EchoEvtRequestCancel called on Request 0x%p\n",  Request));

    queue = WdfRequestGetIoQueue(Request);

    requestContext = RequestGetContext(Request);
    queueContext = QueueGetContext(queue);

    //
    // This book keeping is synchronized by the common
    // Queue presentation lock which we are now acquiring
    //
    WdfSpinLockAcquire(queueContext->SpinLock);

    completeRequest = EchoDecrementRequestCancelOwnershipCount(requestContext);

    if (completeRequest) {
        ASSERT(queueContext->CurrentRequest == Request);
        queueContext->CurrentRequest = NULL;
    }
    else {
        queueContext->CurrentStatus = STATUS_CANCELLED;
    }

    WdfSpinLockRelease(queueContext->SpinLock);

    //
    // Complete the request outside of holding any locks
    //
    if (completeRequest) {
        WdfRequestCompleteWithInformation(Request, STATUS_CANCELLED, 0L);
    }

    return;
}

VOID
EchoSetCurrentRequest(
    WDFREQUEST Request,
    WDFQUEUE Queue
    )
{
    NTSTATUS status;
    PQUEUE_CONTEXT queueContext;
    PREQUEST_CONTEXT requestContext;

    requestContext = RequestGetContext(Request);
    queueContext = QueueGetContext(Queue);

    //
    // Set the ownership count to one.  When a caller wants to claim ownership,
    // they will interlock decrement the count.  When the count reaches zero,
    // ownership has been acquired and the caller may complete the request.
    //
    requestContext->CancelCompletionOwnershipCount = 1;

    //
    // Defer the completion to another thread from the timer dpc
    //
    WdfSpinLockAcquire(queueContext->SpinLock);

    queueContext->CurrentRequest = Request;
    queueContext->CurrentStatus  = STATUS_SUCCESS;

    //
    // Set the cancel routine under the lock, otherwise if we set it outside 
    // of the lock, the timer could run and attempt to mark the request 
    // uncancelable before we can mark it cancelable on this thread. Use 
    // WdfRequestMarkCancelableEx here to prevent to deadlock with ourselves
    // (cancel routine tries to acquire the queue object lock).
    //
    status = WdfRequestMarkCancelableEx(Request, EchoEvtRequestCancel);
    if (!NT_SUCCESS(status)) {
        queueContext->CurrentRequest = NULL;
    }

    WdfSpinLockRelease(queueContext->SpinLock);

    //
    // Complete the request with an error when unable to mark it cancelable.
    //
    if (!NT_SUCCESS(status)) {
        WdfRequestCompleteWithInformation(Request, status, 0L);
    }
}

VOID
EchoEvtIoRead(
    IN WDFQUEUE   Queue,
    IN WDFREQUEST Request,
    IN size_t      Length
    )
/*++

Routine Description:

    This event is called when the framework receives IRP_MJ_READ request.
    It will copy the content from the queue-context buffer to the request buffer.
    If the driver hasn't received any write request earlier, the read returns zero.

Arguments:

    Queue -  Handle to the framework queue object that is associated with the
            I/O request.
    Request - Handle to a framework request object.

    Length  - number of bytes to be read.
                 The default property of the queue is to not dispatch
                 zero lenght read & write requests to the driver and
                 complete is with status success. So we will never get
                 a zero length request.


Return Value:

    VOID

--*/
{
    NTSTATUS Status;
    PQUEUE_CONTEXT queueContext = QueueGetContext(Queue);
    WDFMEMORY memory;

    _Analysis_assume_(Length > 0);

    KdPrint(("EchoEvtIoRead Called! Queue 0x%p, Request 0x%p Length %Iu\n",
                            Queue,Request,Length));
    //
    // No data to read
    //
    if( (queueContext->Buffer == NULL)  ) {
        WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, (ULONG_PTR)0L);
        return;
    }

    _Analysis_assume_(queueContext->Length > 0);

    //
    // Read what we have
    //
    if( queueContext->Length < Length ) {
        Length = queueContext->Length;
    }

    //
    // Get the request memory
    //
    Status = WdfRequestRetrieveOutputMemory(Request, &memory);
    if( !NT_SUCCESS(Status) ) {
        KdPrint(("EchoEvtIoRead Could not get request memory buffer 0x%x\n",Status));
        WdfVerifierDbgBreakPoint();
        WdfRequestCompleteWithInformation(Request, Status, 0L);
        return;
    }

    // Copy the memory out
    Status = WdfMemoryCopyFromBuffer( memory, // destination
                             0,      // offset into the destination memory
                             queueContext->Buffer,
                             Length );
    if( !NT_SUCCESS(Status) ) {
        KdPrint(("EchoEvtIoRead: WdfMemoryCopyFromBuffer failed 0x%x\n", Status));
        WdfRequestComplete(Request, Status);
        return;
    }

    // Set transfer information
    WdfRequestSetInformation(Request, (ULONG_PTR)Length);

    //
    // Mark the request is cancelable.  This must be the last thing we do because
    // the cancel routine can run immediately after we set it.  This means that
    // CurrentRequest and CurrentStatus must be initialized before we mark the
    // request cancelable.
    //
    EchoSetCurrentRequest(Request, Queue);

    return;
}

VOID
EchoEvtIoWrite(
    IN WDFQUEUE   Queue,
    IN WDFREQUEST Request,
    IN size_t     Length
    )
/*++

Routine Description:

    This event is invoked when the framework receives IRP_MJ_WRITE request.
    This routine allocates memory buffer, copies the data from the request to it,
    and stores the buffer pointer in the queue-context with the length variable
    representing the buffers length. The actual completion of the request
    is defered to the periodic timer dpc.

Arguments:

    Queue -  Handle to the framework queue object that is associated with the
            I/O request.
    Request - Handle to a framework request object.

    Length  - number of bytes to be read.
                 The default property of the queue is to not dispatch
                 zero lenght read & write requests to the driver and
                 complete is with status success. So we will never get
                 a zero length request.

Return Value:

    VOID

--*/
{
    NTSTATUS Status;
    WDFMEMORY memory;
    PQUEUE_CONTEXT queueContext = QueueGetContext(Queue);

    _Analysis_assume_(Length > 0);

    KdPrint(("EchoEvtIoWrite Called! Queue 0x%p, Request 0x%p Length %Iu\n",
                            Queue,Request,Length));

    if( Length > MAX_WRITE_LENGTH ) {
        KdPrint(("EchoEvtIoWrite Buffer Length to big %Iu, Max is %d\n",
                 Length,MAX_WRITE_LENGTH));
        WdfRequestCompleteWithInformation(Request, STATUS_BUFFER_OVERFLOW, 0L);
        return;
    }

    // Get the memory buffer
    Status = WdfRequestRetrieveInputMemory(Request, &memory);
    if( !NT_SUCCESS(Status) ) {
        KdPrint(("EchoEvtIoWrite Could not get request memory buffer 0x%x\n",
                 Status));
        WdfVerifierDbgBreakPoint();
        WdfRequestComplete(Request, Status);
        return;
    }

    // Release previous buffer if set
    if( queueContext->Buffer != NULL ) {
        ExFreePool(queueContext->Buffer);
        queueContext->Buffer = NULL;
        queueContext->Length = 0L;
    }

    queueContext->Buffer = ExAllocatePoolWithTag(NonPagedPool, Length, 'sam1');
    if( queueContext->Buffer == NULL ) {
        KdPrint(("EchoEvtIoWrite: Could not allocate %Iu byte buffer\n",Length));
        WdfRequestComplete(Request, STATUS_INSUFFICIENT_RESOURCES);
        return;
    }


    // Copy the memory in
    Status = WdfMemoryCopyToBuffer( memory,
                           0,  // offset into the source memory
                           queueContext->Buffer,
                           Length );
    if( !NT_SUCCESS(Status) ) {
        KdPrint(("EchoEvtIoWrite WdfMemoryCopyToBuffer failed 0x%x\n", Status));
        WdfVerifierDbgBreakPoint();
        ExFreePool(queueContext->Buffer);
        queueContext->Buffer = NULL;
        queueContext->Length = 0L;
        WdfRequestComplete(Request, Status);
        return;
    }

    queueContext->Length = (ULONG) Length;

    // Set transfer information
    WdfRequestSetInformation(Request, (ULONG_PTR)Length);


    //
    // Mark the request is cancelable.  This must be the last thing we do because
    // the cancel routine can run immediately after we set it.  This means that
    // CurrentRequest and CurrentStatus must be initialized before we mark the
    // request cancelable.
    //
    EchoSetCurrentRequest(Request, Queue);

    return;
}


VOID
EchoEvtTimerFunc(
    IN WDFTIMER     Timer
    )
/*++

Routine Description:

    This is the TimerDPC the driver sets up to complete requests.
    This function is registered when the WDFTIMER object is created.

    This function does *NOT* automatically synchronize with the I/O Queue
    callbacks and cancel routine, we must do it ourself in the routine.

Arguments:

    Timer - Handle to a framework Timer object.

Return Value:

    VOID

--*/
{
    NTSTATUS status;
    WDFREQUEST request;
    WDFQUEUE queue;
    PQUEUE_CONTEXT queueContext;
    PREQUEST_CONTEXT requestContext;
    BOOLEAN cancel, completeRequest;

    //
    // Default to failure.  status is initialized so that the compiler does not
    // think we are using an uninitialized value when completing the request.
    //
    status = STATUS_UNSUCCESSFUL;
    cancel = FALSE;
    completeRequest = FALSE;

    queue = (WDFQUEUE) WdfTimerGetParentObject(Timer);
    queueContext = QueueGetContext(queue);
    requestContext = NULL;

    //
    // We must synchronize with the cancel routine which will be taking the
    // request out of the context under this lock.
    //
    WdfSpinLockAcquire(queueContext->SpinLock);

    request = queueContext->CurrentRequest;

    if (request != NULL) {
        requestContext = RequestGetContext(request);

        if (EchoIncrementRequestCancelOwnershipCount(requestContext)) {
            cancel = TRUE;
        }
        else {
            //
            // What has happened is that the cancel routine has executed and
            // has already claimed cancel ownership of the request, but has not
            // yet acquired the object lock and cleared the CurrentRequest field
            // in queueContext.  In this case, do nothing and let the cancel
            // routine run to completion and complete the request.
            //
        }
    }
    
    WdfSpinLockRelease(queueContext->SpinLock);

    //
    // If we could not claim cancel ownership, we are done.
    //
    if (cancel == FALSE) {
        return;
    }

    //
    // The request handle and requestContext are valid until we release
    // the cancel ownership count we already acquired.
    //
    status = WdfRequestUnmarkCancelable(request);
    if (status != STATUS_CANCELLED) {
        KdPrint(("CustomTimerDPC successfully cleared cancel routine on "
                 "request 0x%p, Status 0x%x \n", request,status));

        //
        // Since we successfully removed the cancel routine (and we are not
        // currently racing with it), there is no need to use an interlocked
        // decrement to lower the cancel ownership count.
        //

        //
        // 2 is the initial count we set when we initialized CancelCompletionOwnershipCount
        // plus the call to EchoIncrementRequestCancelOwnershipCount()
        //
        ASSERT(requestContext->CancelCompletionOwnershipCount == 2);
        requestContext->CancelCompletionOwnershipCount -=2;

        completeRequest = TRUE;
    }
    else {
        completeRequest = EchoDecrementRequestCancelOwnershipCount(
            requestContext
            );

        if (completeRequest) {
            KdPrint(
                ("CustomTimerDPC Request 0x%p is STATUS_CANCELLED, but "
                 "claimed completion ownership\n", request));
        }
        else {
            KdPrint(
                ("CustomTimerDPC Request 0x%p is STATUS_CANCELLED, not "
                 "completing", request));
        }
    }

    if (completeRequest) {
        KdPrint(("CustomTimerDPC Completing request 0x%p, Status 0x%x \n",
                 request,status));

        //
        // Clear the current request out of the queue context and complete
        // the request.
        //
        WdfSpinLockAcquire(queueContext->SpinLock);
        queueContext->CurrentRequest = NULL;
        status = queueContext->CurrentStatus;
        WdfSpinLockRelease(queueContext->SpinLock);

        WdfRequestComplete(request, status);
    }
}

