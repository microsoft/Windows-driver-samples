/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    ISRDPC.C

Abstract:

    Contains routine to handle interrupts, interrupt DPCs and WatchDogTimer DPC

Environment:

    Kernel mode

--*/

#include "precomp.h"

#if defined(EVENT_TRACING)
#include "ISRDPC.tmh"
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, NICEvtDeviceD0ExitPreInterruptsDisabled)
#endif


BOOLEAN
NICEvtInterruptIsr(
    IN WDFINTERRUPT Interrupt,
    IN ULONG        MessageID
    )
/*++
Routine Description:

    Interrupt handler for the device.

Arguments:

    Interupt - Address of the framework interrupt object
    MessageID -

Return Value:

     TRUE if our device is interrupting, FALSE otherwise.

--*/
{
    BOOLEAN    InterruptRecognized = FALSE;
    PFDO_DATA  FdoData = NULL;
    USHORT     IntStatus;

    UNREFERENCED_PARAMETER( MessageID );

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INTERRUPT, "--> NICEvtInterruptIsr\n");

    FdoData = FdoGetData(WdfInterruptGetDevice(Interrupt));

    //
    // We process the interrupt if it's not disabled and it's active
    //
    if (!NIC_INTERRUPT_DISABLED(FdoData) && NIC_INTERRUPT_ACTIVE(FdoData))
    {
        InterruptRecognized = TRUE;

        //
        // Disable the interrupt (will be re-enabled in NICEvtInterruptDpc
        //
        NICDisableInterrupt(FdoData);

        //
        // Acknowledge the interrupt(s) and get the interrupt status
        //

        NIC_ACK_INTERRUPT(FdoData, IntStatus);

        WdfInterruptQueueDpcForIsr( Interrupt );

    }

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INTERRUPT, "<-- NICEvtInterruptIsr\n");

    return InterruptRecognized;
}

VOID
NICEvtInterruptDpc(
    IN WDFINTERRUPT WdfInterrupt,
    IN WDFOBJECT    WdfDevice
    )

/*++

Routine Description:

    DPC callback for ISR.

Arguments:

    WdfInterrupt - Handle to the framework interrupt object

    WdfDevice - Associated device object.

Return Value:

--*/
{
    PFDO_DATA fdoData = NULL;

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_DPC, "--> NICEvtInterruptDpc\n");

    fdoData = FdoGetData(WdfDevice);


    WdfSpinLockAcquire(fdoData->RcvLock);

    NICHandleRecvInterrupt(fdoData);


    WdfSpinLockRelease(fdoData->RcvLock);

    //
    // Handle send interrupt
    //

    WdfSpinLockAcquire(fdoData->SendLock);

    NICHandleSendInterrupt(fdoData);


    WdfSpinLockRelease(fdoData->SendLock);

    //
    // Check if any queued Sends need to be reprocessed.
    //
    NICCheckForQueuedSends(fdoData);

    //
    // Start the receive unit if it had stopped
    //

    WdfSpinLockAcquire(fdoData->RcvLock);

    NICStartRecv(fdoData);


    WdfSpinLockRelease(fdoData->RcvLock);

    //
    // Re-enable the interrupt (disabled in MPIsr)
    //
    WdfInterruptSynchronize(
        WdfInterrupt,
        NICEnableInterrupt,
        fdoData);

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_DPC, "<-- NICEvtInterruptDpc\n");

}

NTSTATUS
NICEvtInterruptEnable(
    IN WDFINTERRUPT  Interrupt,
    IN WDFDEVICE     AssociatedDevice
    )
/*++

Routine Description:

    This event is called when the Framework moves the device to D0, and after
    EvtDeviceD0Entry.  The driver should enable its interrupt here.

    This function will be called at the device's assigned interrupt
    IRQL (DIRQL.)

Arguments:

    Interrupt - Handle to a Framework interrupt object.

    AssociatedDevice - Handle to a Framework device object.

Return Value:

    BOOLEAN - TRUE indicates that the interrupt was successfully enabled.

--*/
{
    PFDO_DATA           fdoData;

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_PNP, "--> NICEvtInterruptEnable\n");

    fdoData = FdoGetData(AssociatedDevice);
    NICEnableInterrupt(Interrupt, fdoData);

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_PNP, "<-- NICEvtInterruptEnable\n");

    return STATUS_SUCCESS;
}

NTSTATUS
NICEvtInterruptDisable(
    IN WDFINTERRUPT  Interrupt,
    IN WDFDEVICE     AssociatedDevice
    )
/*++

Routine Description:

    This event is called before the Framework moves the device to D1, D2 or D3
    and before EvtDeviceD0Exit.  The driver should disable its interrupt here.

    This function will be called at the device's assigned interrupt
    IRQL (DIRQL.)

Arguments:

    Interrupt - Handle to a Framework interrupt object.

    AssociatedDevice - Handle to a Framework device object.

Return Value:

    STATUS_SUCCESS -  indicates success.

--*/
{
    PFDO_DATA           fdoData;

    UNREFERENCED_PARAMETER(Interrupt);
    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_PNP, "--> NICEvtInterruptDisable\n");

    fdoData = FdoGetData(AssociatedDevice);
    NICDisableInterrupt(fdoData);

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_PNP, "<-- NICEvtInterruptDisable\n");

    return STATUS_SUCCESS;
}

NTSTATUS
NICEvtDeviceD0EntryPostInterruptsEnabled(
    IN WDFDEVICE     Device,
    IN WDF_POWER_DEVICE_STATE PreviousState
    )
/*++

Routine Description:

    This event is called so that driver can do PASSIVE_LEVEL work after
    the interrupt is connected and enabled. Here we start the watchdog timer.
    Watch dog timer is used to do the initial link detection during
    start and then used to make sure the device is not stuck for any reason.

    This function is not marked pageable because this function is in the
    device power up path. When a function is marked pagable and the code
    section is paged out, it will generate a page fault which could impact
    the fast resume behavior because the client driver will have to wait
    until the system drivers can service this page fault.

Arguments:

    Interrupt - Handle to a Framework interrupt object.

    AssociatedDevice - Handle to a Framework device object.

Return Value:

   STATUS_SUCCESS -  indicates success.

--*/
{
    PFDO_DATA  fdoData;

    UNREFERENCED_PARAMETER( PreviousState );

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_PNP, "--> NICEvtDeviceD0EntryPostInterruptsEnabled\n");

    fdoData = FdoGetData(Device);

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_PNP, "<-- NICEvtDeviceD0EntryPostInterruptsEnabled\n");

    return STATUS_SUCCESS;

}

NTSTATUS
NICEvtDeviceD0ExitPreInterruptsDisabled(
    IN WDFDEVICE     Device,
    IN WDF_POWER_DEVICE_STATE TargetState
    )
/*++

Routine Description:

    This event is called so that driver can do PASSIVE_LEVEL work before
    the interrupt is disconnected and disabled.

Arguments:

    Interrupt - Handle to a Framework interrupt object.

    AssociatedDevice - Handle to a Framework device object.

Return Value:

    STATUS_SUCCESS -  indicates success.

--*/
{
    PFDO_DATA           fdoData;

    UNREFERENCED_PARAMETER(TargetState);

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_PNP, "--> NICEvtDeviceD0ExitPreInterruptsDisabled\n");

    fdoData = FdoGetData(Device);

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_PNP, "<-- NICEvtDeviceD0ExitPreInterruptsDisabled\n");

    return STATUS_SUCCESS;

}

VOID
NICStartWatchDogTimer(
    IN  PFDO_DATA     FdoData
    )
{
    LARGE_INTEGER           dueTime;

    if(!FdoData->CheckForHang){

        //
        // Set the link detection flag to indicate that NICWatchDogEvtTimerFunc
        // is first doing link-detection.
        //
        MP_SET_FLAG(FdoData, fMP_ADAPTER_LINK_DETECTION);
        FdoData->CheckForHang = FALSE;
        FdoData->bLinkDetectionWait = FALSE;
        FdoData->bLookForLink = FALSE;
        dueTime.QuadPart = NIC_LINK_DETECTION_DELAY;

    } else {
        dueTime.QuadPart = NIC_CHECK_FOR_HANG_DELAY;
    }

    WdfTimerStart(FdoData->WatchDogTimer,
                    dueTime.QuadPart
                    );

}

VOID
NICWatchDogEvtTimerFunc(
    IN WDFTIMER Timer
    )
/*++

Routine Description:

    This DPC is used to do both link detection during hardware init and
    after that for hardware hang detection.

Arguments:


Return Value:

    None

--*/
{
    PFDO_DATA           FdoData = NULL;
    LARGE_INTEGER       DueTime;
    NTSTATUS            status = STATUS_SUCCESS;

    FdoData = FdoGetData(WdfTimerGetParentObject(Timer));

    DueTime.QuadPart = NIC_CHECK_FOR_HANG_DELAY;


    if(!FdoData->CheckForHang){
        //
        // We are still doing link detection
        //
        status = NICLinkDetection(FdoData);
        if(status == STATUS_PENDING) {
            // Wait for 100 ms
            FdoData->bLinkDetectionWait = TRUE;
            DueTime.QuadPart = NIC_LINK_DETECTION_DELAY;
        }else {
            FdoData->CheckForHang = TRUE;
        }
    }else {
        //
        // Link detection is over, let us check to see
        // if the hardware is stuck.
        //
        if(NICCheckForHang(FdoData)){

            status = NICReset(FdoData);
            if(!NT_SUCCESS(status)){
                goto Exit;
            }
        }
    }

    WdfTimerStart(FdoData->WatchDogTimer,   // Timer
                DueTime.QuadPart                     // DueTime
                );

    return;

Exit:
    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_DPC, "WatchDogTimer is exiting %x\n", status);
    return;

}

BOOLEAN
NICCheckForHang(
    IN  PFDO_DATA     FdoData
    )
/*++

Routine Description:

    CheckForHang handler is called in the context of a timer DPC.
    take advantage of this fact when acquiring/releasing spinlocks

Arguments:

    FdoData  Pointer to our adapter

Return Value:

    TRUE    This NIC needs a reset
    FALSE   Everything is fine

--*/
{
     PMP_TCB             pMpTcb;

    //
    // Just skip this part if the adapter is doing link detection
    //
    if (MP_TEST_FLAG(FdoData, fMP_ADAPTER_LINK_DETECTION))
    {
        return(FALSE);
    }

    //
    // any nonrecoverable hardware error?
    //
    if (MP_TEST_FLAG(FdoData, fMP_ADAPTER_NON_RECOVER_ERROR))
    {
        TraceEvents(TRACE_LEVEL_WARNING, DBG_DPC, "Non recoverable error - remove\n");
        return (TRUE);
    }

    //
    // hardware failure?
    //
    if (MP_TEST_FLAG(FdoData, fMP_ADAPTER_HARDWARE_ERROR))
    {
        TraceEvents(TRACE_LEVEL_WARNING, DBG_DPC, "hardware error - reset\n");
        return(TRUE);
    }

    //
    // Is send stuck?
    //


    WdfSpinLockAcquire(FdoData->SendLock);

    if (FdoData->nBusySend > 0)
    {
        pMpTcb = FdoData->CurrSendHead;
        pMpTcb->Count++;
        if (pMpTcb->Count > NIC_SEND_HANG_THRESHOLD)
        {

            WdfSpinLockRelease(FdoData->SendLock);
            TraceEvents(TRACE_LEVEL_WARNING, DBG_DPC, "Send is stuck - reset\n");
            return(TRUE);
        }
    }


    WdfSpinLockRelease(FdoData->SendLock);


    WdfSpinLockAcquire(FdoData->RcvLock);

    //
    // Update the RFD shrink count
    //
    if (FdoData->CurrNumRfd > FdoData->NumRfd)
    {
        FdoData->RfdShrinkCount++;
    }


    WdfSpinLockRelease(FdoData->RcvLock);

    NICIndicateMediaState(FdoData);

    return(FALSE);
}

NTSTATUS
NICReset(
    IN PFDO_DATA FdoData
    )
/*++

Routine Description:

    Function to reset the device.

Arguments:

    FdoData  Pointer to our adapter


Return Value:

    NT Status code.

Note:
    NICReset is called at DPC. Take advantage of this fact
    when acquiring or releasing spinlocks

--*/
{
    NTSTATUS                status;

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_DPC, "---> MPReset\n");


    WdfSpinLockAcquire(FdoData->Lock);

    WdfSpinLockAcquire(FdoData->SendLock);

    WdfSpinLockAcquire(FdoData->RcvLock);

    do
    {
        //
        // Is this adapter already doing a reset?
        //
        if (MP_TEST_FLAG(FdoData, fMP_ADAPTER_RESET_IN_PROGRESS))
        {
            status = STATUS_SUCCESS;
            goto exit;
        }

        MP_SET_FLAG(FdoData, fMP_ADAPTER_RESET_IN_PROGRESS);

        //
        // Is this adapter doing link detection?
        //
        if (MP_TEST_FLAG(FdoData, fMP_ADAPTER_LINK_DETECTION))
        {
            TraceEvents(TRACE_LEVEL_WARNING, DBG_DPC, "Reset is pended...\n");
            status = STATUS_SUCCESS;
            goto exit;
        }
        //
        // Is this adapter going to be removed
        //
        if (MP_TEST_FLAG(FdoData, fMP_ADAPTER_NON_RECOVER_ERROR))
        {
           status = STATUS_DEVICE_DATA_ERROR;
           if (MP_TEST_FLAG(FdoData, fMP_ADAPTER_REMOVE_IN_PROGRESS))
           {
               goto exit;
           }

           // This is an unrecoverable hardware failure.
           // We need to tell NDIS to remove this miniport
           MP_SET_FLAG(FdoData, fMP_ADAPTER_REMOVE_IN_PROGRESS);
           MP_CLEAR_FLAG(FdoData, fMP_ADAPTER_RESET_IN_PROGRESS);


           WdfSpinLockRelease(FdoData->RcvLock);

            WdfSpinLockRelease(FdoData->SendLock);

            WdfSpinLockRelease(FdoData->Lock);

           // TODO: Log an entry into the eventlog
           WdfDeviceSetFailed(FdoData->WdfDevice, WdfDeviceFailedAttemptRestart);

           TraceEvents(TRACE_LEVEL_FATAL, DBG_DPC, "<--- MPReset, status=%x\n", status);

           return status;
        }


        //
        // Disable the interrupt and issue a reset to the NIC
        //
        NICDisableInterrupt(FdoData);
        NICIssueSelectiveReset(FdoData);


        //
        // Release all the locks and then acquire back the send lock
        // we are going to clean up the send queues
        // which may involve calling Ndis APIs
        // release all the locks before grabbing the send lock to
        // avoid deadlocks
        //


         WdfSpinLockRelease(FdoData->RcvLock);

        WdfSpinLockRelease(FdoData->SendLock);

        WdfSpinLockRelease(FdoData->Lock);


        WdfSpinLockAcquire(FdoData->SendLock);

        //
        // Free the packets on SendQueueList
        //
        NICFreeQueuedSendPackets(FdoData);

        //
        // Free the packets being actively sent & stopped
        //
        NICFreeBusySendPackets(FdoData);


        RtlZeroMemory(FdoData->MpTcbMem, FdoData->MpTcbMemSize);

        //
        // Re-initialize the send structures
        //
        NICInitSendBuffers(FdoData);


        WdfSpinLockRelease(FdoData->SendLock);

        //
        // get all the locks again in the right order
        //


        WdfSpinLockAcquire(FdoData->Lock);

        WdfSpinLockAcquire(FdoData->SendLock);

        WdfSpinLockAcquire(FdoData->RcvLock);

        //
        // Reset the RFD list and re-start RU
        //
        NICResetRecv(FdoData);
        status = NICStartRecv(FdoData);
        if (status != STATUS_SUCCESS)
        {
            // Are we having failures in a few consecutive resets?
            if (FdoData->HwErrCount < NIC_HARDWARE_ERROR_THRESHOLD)
            {
                // It's not over the threshold yet, let it to continue
                FdoData->HwErrCount++;
            }
            else
            {
                // This is an unrecoverable hardware failure.
                // We need to tell NDIS to remove this miniport
                MP_SET_FLAG(FdoData, fMP_ADAPTER_REMOVE_IN_PROGRESS);
                MP_CLEAR_FLAG(FdoData, fMP_ADAPTER_RESET_IN_PROGRESS);



                WdfSpinLockRelease(FdoData->RcvLock);

                WdfSpinLockRelease(FdoData->SendLock);

                WdfSpinLockRelease(FdoData->Lock);

                // TODO: Log an entry into the eventlog
                //
                // Tell the system that the device has failed.
                //
                WdfDeviceSetFailed(FdoData->WdfDevice, WdfDeviceFailedAttemptRestart);

                TraceEvents(TRACE_LEVEL_ERROR, DBG_DPC, "<--- MPReset, status=%x\n", status);
                return(status);
            }

            break;
        }

        FdoData->HwErrCount = 0;
        MP_CLEAR_FLAG(FdoData, fMP_ADAPTER_HARDWARE_ERROR);

        NICEnableInterrupt(FdoData->WdfInterrupt, FdoData);

    } WHILE (FALSE);

    MP_CLEAR_FLAG(FdoData, fMP_ADAPTER_RESET_IN_PROGRESS);

    exit:


        WdfSpinLockRelease(FdoData->RcvLock);

        WdfSpinLockRelease(FdoData->SendLock);

        WdfSpinLockRelease(FdoData->Lock);



    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_DPC, "<--- MPReset, status=%x\n", status);
    return(status);
}


NTSTATUS
NICLinkDetection(
    PFDO_DATA         FdoData
    )
/*++

Routine Description:

    Timer function for postponed link negotiation. Called from
    the NICWatchDogEvtTimerFunc. After the link detection is over
    we will complete any pending ioctl or send IRPs.

Arguments:

    FdoData     Pointer to our FdoData

Return Value:

    NT status

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    MEDIA_STATE             CurrMediaState;
    PNDISPROT_QUERY_OID     pQuery = NULL;
    PNDISPROT_SET_OID       pSet = NULL;
    PVOID                   DataBuffer;
    ULONG                   BytesWritten;
    NDIS_OID                Oid;
    PVOID                   InformationBuffer;
    size_t               bufSize;
    WDFREQUEST              request;

    //
    // Handle the link negotiation.
    //
    if (FdoData->bLinkDetectionWait)
    {
        status = ScanAndSetupPhy(FdoData);
    }
    else
    {
        status = PhyDetect(FdoData);
    }

    if (status == STATUS_PENDING)
    {
        return status;
    }

    //
    // Reset some variables for link detection
    //
    FdoData->bLinkDetectionWait = FALSE;

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_DPC, "NICLinkDetection - negotiation done\n");


    WdfSpinLockAcquire(FdoData->Lock);
    MP_CLEAR_FLAG(FdoData, fMP_ADAPTER_LINK_DETECTION);

    WdfSpinLockRelease(FdoData->Lock);

    //
    // Any OID query request pending?
    //

    status = NICGetIoctlRequest(FdoData->PendingIoctlQueue,
                                IOCTL_NDISPROT_QUERY_OID_VALUE,
                                &request);

    if(NT_SUCCESS(status)) {
        status = WdfRequestRetrieveOutputBuffer(request, sizeof(NDISPROT_QUERY_OID), &DataBuffer, &bufSize);
        if(NT_SUCCESS(status)) {

            pQuery = (PNDISPROT_QUERY_OID)DataBuffer;
            Oid = pQuery->Oid;
            InformationBuffer = &pQuery->Data[0];
            switch(Oid)
            {
                case OID_GEN_LINK_SPEED:
                    *((PULONG)InformationBuffer) = FdoData->usLinkSpeed * 10000;
                    BytesWritten = sizeof(ULONG);

                    break;

                case OID_GEN_MEDIA_CONNECT_STATUS:
                default:
                    ASSERT(Oid == OID_GEN_MEDIA_CONNECT_STATUS);

                    CurrMediaState = NICIndicateMediaState(FdoData);

                    RtlMoveMemory(InformationBuffer,
                                  &CurrMediaState,
                                  sizeof(NDIS_MEDIA_STATE));

                    BytesWritten = sizeof(NDIS_MEDIA_STATE);
            }

            WdfRequestCompleteWithInformation(request, status, BytesWritten);
        }
    }

    //
    // Any OID set request pending?
    //
    status = NICGetIoctlRequest(FdoData->PendingIoctlQueue,
                            IOCTL_NDISPROT_SET_OID_VALUE,
                             &request);

    if(NT_SUCCESS(status)) {
        ULONG    PacketFilter;

        status = WdfRequestRetrieveOutputBuffer(request, sizeof(NDISPROT_SET_OID), &DataBuffer, &bufSize);
        if(NT_SUCCESS(status)) {

            pSet = (PNDISPROT_SET_OID)DataBuffer;
            Oid = pSet->Oid;
            InformationBuffer = &pSet->Data[0];
            if (Oid == OID_GEN_CURRENT_PACKET_FILTER)
            {

                RtlMoveMemory(&PacketFilter, InformationBuffer, sizeof(ULONG));


                WdfSpinLockAcquire(FdoData->Lock);

                status = NICSetPacketFilter(
                             FdoData,
                             PacketFilter);


                WdfSpinLockRelease(FdoData->Lock);

                if (status == STATUS_SUCCESS)
                {
                    FdoData->PacketFilter = PacketFilter;
                }

                WdfRequestCompleteWithInformation(request, status, 0);
            }
        }
    }

    //
    // Any read pending?
    //

    WdfSpinLockAcquire(FdoData->RcvLock);

    //
    // Start the NIC receive unit
    //
    status = NICStartRecv(FdoData);
    if (status != STATUS_SUCCESS)
    {
        MP_SET_HARDWARE_ERROR(FdoData);
    }


    WdfSpinLockRelease(FdoData->RcvLock);

    //
    // Send packets which have been queued while link detection was going on.
    //
    NICCheckForQueuedSends(FdoData);

    return status;
}


