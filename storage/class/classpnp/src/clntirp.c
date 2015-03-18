/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    clntirp.c

Abstract:

    Client IRP queuing routines for CLASSPNP

Environment:

    kernel mode only

Notes:


Revision History:

--*/

#include "classp.h"
#include "debug.h"


#ifdef DEBUG_USE_WPP
#include "clntirp.tmh"
#endif

VOID
ClasspStartIdleTimer(
    IN PCLASS_PRIVATE_FDO_DATA FdoData,
    IN ULONGLONG IdleInterval
    );

VOID
ClasspStopIdleTimer(
    PCLASS_PRIVATE_FDO_DATA FdoData
    );

KDEFERRED_ROUTINE ClasspIdleTimerDpc;

VOID
ClasspServiceIdleRequest(
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    BOOLEAN PostToDpc
    );

PIRP
ClasspDequeueIdleRequest(
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
    );


/*++

EnqueueDeferredClientIrp

Routine Description:

    Insert the deferred irp into the list.

    Note: we currently do not support Cancel for storage irps.

Arguments:

    Fdo - Pointer to the device object
    Irp     - Pointer to the I/O request packet

Return Value:

    None

--*/
VOID
EnqueueDeferredClientIrp(
    PDEVICE_OBJECT Fdo,
    PIRP Irp
    )
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PCLASS_PRIVATE_FDO_DATA fdoData = fdoExtension->PrivateFdoData;
    KIRQL oldIrql;


    KeAcquireSpinLock(&fdoData->SpinLock, &oldIrql);
    InsertTailList(&fdoData->DeferredClientIrpList, &Irp->Tail.Overlay.ListEntry);


    KeReleaseSpinLock(&fdoData->SpinLock, oldIrql);
}

/*++

DequeueDeferredClientIrp

Routine Description:

    Remove the deferred irp from the list.

Arguments:

    Fdo - Pointer to the device object

Return Value:

    Pointer to removed IRP

--*/
PIRP
DequeueDeferredClientIrp(
    PDEVICE_OBJECT Fdo
    )
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PCLASS_PRIVATE_FDO_DATA fdoData = fdoExtension->PrivateFdoData;
    PIRP irp;

    //
    // The DeferredClientIrpList is almost always empty.
    // We don't want to grab the spinlock every time we check it (which is on every xfer completion)
    // so check once first before we grab the spinlock.
    //
    if (IsListEmpty(&fdoData->DeferredClientIrpList)){
        irp = NULL;
    }
    else {
        PLIST_ENTRY listEntry;
        KIRQL oldIrql;

        KeAcquireSpinLock(&fdoData->SpinLock, &oldIrql);
        if (IsListEmpty(&fdoData->DeferredClientIrpList)){
            listEntry = NULL;
        }
        else {
            listEntry = RemoveHeadList(&fdoData->DeferredClientIrpList);
        }
        KeReleaseSpinLock(&fdoData->SpinLock, oldIrql);

        if (listEntry == NULL) {
            irp = NULL;
        }
        else {
            irp = CONTAINING_RECORD(listEntry, IRP, Tail.Overlay.ListEntry);
            NT_ASSERT(irp->Type == IO_TYPE_IRP);


            InitializeListHead(&irp->Tail.Overlay.ListEntry);
        }
    }

    return irp;
}

/*++

ClasspInitializeIdleTimer

Routine Description:

    Initialize the idle timer for the given device.

Arguments:

    FdoExtension    - Pointer to the device extension
    IdleInterval    - Timer interval

Return Value:

    None

--*/
VOID
ClasspInitializeIdleTimer(
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
    )
{
    PCLASS_PRIVATE_FDO_DATA fdoData = FdoExtension->PrivateFdoData;
    ULONG idleInterval = CLASS_IDLE_INTERVAL;
    ULONG idlePrioritySupported = TRUE;
    ULONG activeIdleIoMax = 1;

    ClassGetDeviceParameter(FdoExtension,
                            CLASSP_REG_SUBKEY_NAME,
                            CLASSP_REG_IDLE_PRIORITY_SUPPORTED,
                            &idlePrioritySupported);


    if (idlePrioritySupported == FALSE) {
        //
        // User has set the registry to disable idle priority for this disk.
        // No need to initialize any further.
        // Always ensure that none of the other fields used for idle priority
        // io are ever used without checking if it is supported.
        //
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_TIMER, "ClasspInitializeIdleTimer: Idle priority not supported for disk %p\n", FdoExtension));
        fdoData->IdlePrioritySupported = FALSE;
        fdoData->IdleIoCount = 0;
        fdoData->ActiveIoCount = 0;
        return;
    }

    ClassGetDeviceParameter(FdoExtension,
                            CLASSP_REG_SUBKEY_NAME,
                            CLASSP_REG_IDLE_INTERVAL_NAME,
                            &idleInterval);

    if ((idleInterval < CLASS_IDLE_TIMER_TICKS) || (idleInterval > USHORT_MAX)) {
        //
        // If the interval is too low or too high, reset it to the default value.
        //
        idleInterval = CLASS_IDLE_INTERVAL;
    }

    fdoData->IdlePrioritySupported = TRUE;
    KeInitializeSpinLock(&fdoData->IdleListLock);
    KeInitializeTimer(&fdoData->IdleTimer);
    KeInitializeDpc(&fdoData->IdleDpc, ClasspIdleTimerDpc, FdoExtension);
    InitializeListHead(&fdoData->IdleIrpList);
    fdoData->IdleTimerStarted = FALSE;
    fdoData->IdleTimerInterval = (USHORT) (idleInterval / CLASS_IDLE_TIMER_TICKS);
    fdoData->StarvationCount = CLASS_STARVATION_INTERVAL / fdoData->IdleTimerInterval;

    //
    // Due to the coarseness of the idle timer frequency, some variability in
    // the idle interval will be tolerated such that it is the desired idle
    // interval on average.
    fdoData->IdleInterval =
        (USHORT)(idleInterval - (fdoData->IdleTimerInterval / 2));

    fdoData->IdleTimerTicks = 0;
    fdoData->IdleTicks = 0;
    fdoData->IdleIoCount = 0;
    fdoData->ActiveIoCount = 0;
    fdoData->ActiveIdleIoCount = 0;

    ClassGetDeviceParameter(FdoExtension,
                            CLASSP_REG_SUBKEY_NAME,
                            CLASSP_REG_IDLE_ACTIVE_MAX,
                            &activeIdleIoMax);

    activeIdleIoMax = max(activeIdleIoMax, 1);
    activeIdleIoMax = min(activeIdleIoMax, USHORT_MAX);

    fdoData->IdleActiveIoMax = (USHORT)activeIdleIoMax;

    return;
}

/*++

ClasspStartIdleTimer

Routine Description:

    Start the idle timer if not already running. Reset the
    timer counters before starting the timer. Use the IdleInterval
    in the private fdo data to setup the timer.

Arguments:

    FdoData - Pointer to the private fdo data

    IdleInterval - Amount of time since the completion of the last non-idle request

Return Value:

    None

--*/
VOID
ClasspStartIdleTimer(
    IN PCLASS_PRIVATE_FDO_DATA FdoData,
    IN ULONGLONG IdleInterval
    )
{
    LARGE_INTEGER dueTime;
    LONG mstotimer;
    LONG timerStarted;

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_TIMER, "ClasspStartIdleTimer: Start idle timer\n"));

    timerStarted = InterlockedCompareExchange(&FdoData->IdleTimerStarted, 1, 0);

    if (!timerStarted) {

        //
        // Reset the anti-starvation timer tick counter and set the idle tick
        // counter according to the actual amount of idle time. The latter is
        // important to do to ensure that if the idle queue drains and the timer
        // has to be stopped and started on the arrival of the next idle request,
        // those requests don't get delayed unnecessarily due to IdleTicks not
        // reflecting actual idle time.
        //
        FdoData->IdleTimerTicks = 0;
        FdoData->IdleTicks = (ULONG)(IdleInterval / FdoData->IdleTimerInterval);

        //
        // convert milliseconds to a relative 100ns
        //
        mstotimer = (-10) * 1000;

        //
        // multiply the period
        //
        dueTime.QuadPart = Int32x32To64(FdoData->IdleTimerInterval, mstotimer);

        KeSetTimerEx(&FdoData->IdleTimer,
                     dueTime,
                     FdoData->IdleTimerInterval,
                     &FdoData->IdleDpc);
    }
    return;
}

/*++

ClasspStopIdleTimer

Routine Description:

    Stop the idle timer if running. Also reset the timer counters.

Arguments:

    FdoData - Pointer to the private fdo data

Return Value:

    None

--*/
VOID
ClasspStopIdleTimer(
    PCLASS_PRIVATE_FDO_DATA FdoData
    )
{
    LONG timerStarted;

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_TIMER, "ClasspStopIdleTimer: Stop idle timer\n"));

    timerStarted = InterlockedCompareExchange(&FdoData->IdleTimerStarted, 0, 1);

    if (timerStarted) {
        (VOID)KeCancelTimer(&FdoData->IdleTimer);
    }
    return;
}

/*++

ClasspGetIdleTime

Routine Description:

    This routine returns how long it has been since the last non-idle request
    completed by checking the actual time.

Arguments:

    FdoData - Pointer to the private fdo data

Return Value:

    The idle interval in ms.

--*/
ULONGLONG
ClasspGetIdleTime (
    IN PCLASS_PRIVATE_FDO_DATA FdoData
    )
{
    ULONGLONG idleTime;
    LARGE_INTEGER currentTime;
    NTSTATUS status;

    //
    // If there are any outstanding non-idle requests, then there has been no
    // idle time.
    //
    if (FdoData->ActiveIoCount > 0) {
        return 0;
    }

    //
    // Get the time difference between current time and last I/O
    // complete time.
    //
    currentTime = ClasspGetCurrentTime(NULL);

    status = RtlULongLongSub((ULONGLONG)currentTime.QuadPart,
                             (ULONGLONG)FdoData->LastIoTime.QuadPart,
                             &idleTime);

    if (NT_SUCCESS(status)) {
        //
        // Convert the time to milliseconds.
        //
        idleTime = ClasspTimeDiffToMs(FdoData, idleTime);
    } else {
        //
        // Failed to get time difference, assume enough time passed.
        //
        idleTime = FdoData->IdleInterval;
    }

    return idleTime;
}

/*++

ClasspIdleTicksSufficient

Routine Description:

    This routine whether enough idle ticks have occurred since the completion of
    the last non-idle request.

Arguments:

    FdoData - Pointer to the private fdo data

Return Value:

    TRUE if sufficient idle ticks have expired to issue the next idle request.

--*/
LOGICAL
ClasspIdleTicksSufficient (
    IN PCLASS_PRIVATE_FDO_DATA FdoData
    )
{
    ULONGLONG idleInterval;

    //
    // If it has been more than enough idle timer ticks since the completion of
    // the last non-idle request, enough idle time has passed.
    //

    if (FdoData->IdleTicks > CLASS_IDLE_TIMER_TICKS) {
        return TRUE;
    }

    //
    // If there have not been enough timer ticks, then there has not been
    // enough idle time.
    //
    if (FdoData->IdleTicks < CLASS_IDLE_TIMER_TICKS) {
        return FALSE;
    }

    //
    // IdleTicks can reach CLASS_IDLE_TIMER_TICKS before FdoData->IdleInterval
    // worth of time elapses from the completion of the last non-idle request.
    // This can happen because when the idle timer is running, the last non-idle
    // request can complete at any time in the middle of the timer period (half
    // on average) so on the next timer expiration, IdleTicks will transition
    // 0->1 without its full time having passed since the completion of the last
    // non-idle request. So when IdleTicks is exactly CLASS_IDLE_TIMER_TICKS,
    // explicitly check whether an idle request should be issued now or on the
    // next timer expiration.
    //
    idleInterval = ClasspGetIdleTime(FdoData);

    if (idleInterval >= FdoData->IdleInterval) {
        return TRUE;
    }

    return FALSE;
}

/*++

ClasspIdleTimerDpc

Routine Description:

    Timer dpc function. This function will be called once every
    IdleInterval. This will increment the IdleTicks and
    if it goes above 1 (i.e., disk is in idle state) then
    it will service an idle request.

    This function will increment IdleTimerTicks if the IdleTicks
    does not go above 1 (i.e., disk is not in idle state). When it
    reaches the starvation idle count (1 second) it will process
    one idle request.

Arguments:

    Dpc             - Pointer to DPC object
    Context         - Pointer to the fdo device extension
    SystemArgument1 - Not used
    SystemArgument2 - Not used

Return Value:

    None

--*/
VOID
ClasspIdleTimerDpc(
    IN PKDPC Dpc,
    IN PVOID Context,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Context;
    PCLASS_PRIVATE_FDO_DATA fdoData;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    if (fdoExtension == NULL) {
        NT_ASSERT(fdoExtension != NULL);
        return;
    }

    fdoData = fdoExtension->PrivateFdoData;

    if ((fdoData->ActiveIoCount <= 0) &&
        (++fdoData->IdleTicks >= CLASS_IDLE_TIMER_TICKS)) {

        //
        // If there are max active idle request, do not issue another one here.
        //
        if (fdoData->ActiveIdleIoCount >= fdoData->IdleActiveIoMax) {
            return;
        }

        //
        // Check whether enough idle time has passed since the last non-idle
        // request has completed.
        //

        if (ClasspIdleTicksSufficient(fdoData)) {
            //
            // We are going to issue an idle request so reset the anti-starvation
            // timer counter.
            //
            fdoData->IdleTimerTicks = 0;
            ClasspServiceIdleRequest(fdoExtension, FALSE);
        }
        return;
    }

    //
    // If the timer is running then there must be at least one idle priority I/O pending
    //
    if (++fdoData->IdleTimerTicks >= fdoData->StarvationCount) {
        fdoData->IdleTimerTicks = 0;
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_TIMER, "ClasspIdleTimerDpc: Starvation timer. Send one idle request\n"));
        ClasspServiceIdleRequest(fdoExtension, FALSE);
    }
    return;
}

/*++

ClasspEnqueueIdleRequest

Routine Description:

    This function will insert the idle request into the list.
    If the inserted reqeust is the first request then it will
    start the timer.

Arguments:

    DeviceObject    - Pointer to device object
    Irp             - Pointer to the idle I/O request packet

Return Value:

    NT status code.

--*/
NTSTATUS
ClasspEnqueueIdleRequest(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PCLASS_PRIVATE_FDO_DATA fdoData = fdoExtension->PrivateFdoData;
    KIRQL oldIrql;
    BOOLEAN issueRequest = TRUE;
    ULONGLONG idleInterval;

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_TIMER, "ClasspEnqueueIdleRequest: Queue idle request %p\n", Irp));

    IoMarkIrpPending(Irp);

    //
    // Get the time difference between current time and last non-idle request
    // complete time. If the there has been enough idle time, then issue the
    // request (unless other factors prevent us from doing so below) and set the
    // idle time such that we starting the timer below, it would start off with
    // enough idle ticks.
    //
    idleInterval = ClasspGetIdleTime(fdoData);

    if (idleInterval >= fdoData->IdleInterval) {
        idleInterval = fdoData->IdleTimerInterval * CLASS_IDLE_TIMER_TICKS;
    } else {
        issueRequest = FALSE;
    }

    //
    // If there are already max active idle requests in the port driver, then
    // queue this idle request.
    //
    if (fdoData->ActiveIdleIoCount >= fdoData->IdleActiveIoMax) {
        issueRequest = FALSE;
    }

    TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_TIMER, "ClasspEnqueueIdleRequest: Diff time %I64d\n", idleInterval));

    KeAcquireSpinLock(&fdoData->IdleListLock, &oldIrql);
    if (IsListEmpty(&fdoData->IdleIrpList)) {
        NT_ASSERT(fdoData->IdleIoCount == 0);
    }
    InsertTailList(&fdoData->IdleIrpList, &Irp->Tail.Overlay.ListEntry);


    fdoData->IdleIoCount++;
    if (!fdoData->IdleTimerStarted) {
        ClasspStartIdleTimer(fdoData, idleInterval);
    }

    if (fdoData->IdleIoCount != 1) {
        issueRequest = FALSE;
    }


    KeReleaseSpinLock(&fdoData->IdleListLock, oldIrql);

    if (issueRequest) {
        ClasspServiceIdleRequest(fdoExtension, FALSE);
    }

    return STATUS_PENDING;
}

/*++

ClasspDequeueIdleRequest

Routine Description:

    This function will remove the next idle request from the list.
    If there are no requests in the queue, then it will return NULL.

Arguments:

    FdoExtension         - Pointer to the functional device extension

Return Value:

    Pointer to removed IRP

--*/
PIRP
ClasspDequeueIdleRequest(
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
    )
{
    PCLASS_PRIVATE_FDO_DATA fdoData = FdoExtension->PrivateFdoData;
    PLIST_ENTRY listEntry = NULL;
    PIRP irp = NULL;
    KIRQL oldIrql;

    KeAcquireSpinLock(&fdoData->IdleListLock, &oldIrql);

    if (fdoData->IdleIoCount > 0) {
        listEntry = RemoveHeadList(&fdoData->IdleIrpList);
        //
        // Make sure we actaully removed a request from the list
        //
        NT_ASSERT(listEntry != &fdoData->IdleIrpList);
        //
        // Decrement the idle I/O count.
        //
        fdoData->IdleIoCount--;
        //
        // Stop the timer on last request
        //
        if (fdoData->IdleIoCount == 0) {
            ClasspStopIdleTimer(fdoData);
        }
        irp = CONTAINING_RECORD(listEntry, IRP, Tail.Overlay.ListEntry);
        NT_ASSERT(irp->Type == IO_TYPE_IRP);


        InitializeListHead(&irp->Tail.Overlay.ListEntry);
    }

    KeReleaseSpinLock(&fdoData->IdleListLock, oldIrql);
    return irp;
}

/*++

ClasspCompleteIdleRequest

Routine Description:

    This function will be called every time an idle request is completed.
    This will call ClasspServiceIdleRequest to process any other pending idle requests.

Arguments:

    FdoExtension    - Pointer to the device extension

Return Value:

    None

--*/
VOID
ClasspCompleteIdleRequest(
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
    )
{
    PCLASS_PRIVATE_FDO_DATA fdoData = FdoExtension->PrivateFdoData;

    //
    // Issue the next idle request if there are any left in the queue, there are
    // no non-idle requests outstanding, there are less than max idle requests
    // outstanding, and it has been long enough since the completion of the last
    // non-idle request.
    //
    if ((fdoData->IdleIoCount > 0) &&
        (fdoData->ActiveIdleIoCount < fdoData->IdleActiveIoMax) &&
        (fdoData->ActiveIoCount <= 0) &&
        (ClasspIdleTicksSufficient(fdoData))) {
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_TIMER, "ClasspCompleteIdleRequest: Service next idle reqeusts\n"));
        ClasspServiceIdleRequest(FdoExtension, TRUE);
    }

    return;
}

/*++

ClasspServiceIdleRequest

Routine Description:

    Remove the next pending idle request from the queue and process it.
    If a request was removed then it will be processed otherwise it will
    just return.

Arguments:

    FdoExtension    - Pointer to the device extension
    PostToDpc       - Flag to pass to ServiceTransferRequest to indicate if request must be posted to a DPC

Return Value:

    None

--*/
VOID
ClasspServiceIdleRequest(
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    BOOLEAN PostToDpc
    )
{
    PIRP irp;

    irp = ClasspDequeueIdleRequest(FdoExtension);
    if (irp != NULL) {
        ServiceTransferRequest(FdoExtension->DeviceObject, irp, PostToDpc);
    }
    return;
}



