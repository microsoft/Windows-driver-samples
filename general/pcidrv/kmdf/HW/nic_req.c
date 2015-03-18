/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:
    mp_req.c

Abstract:
    This module handle NDIS OID ioctls. This module is not required
    if the upper edge is not NDIS.

Environment:

    Kernel mode

--*/

#include "precomp.h"

#if defined(EVENT_TRACING)
#include "nic_req.tmh"
#endif

//
// Following status values are copied from NDIS.H
//
#define NDIS_STATUS_MEDIA_CONNECT       0x4001000BL
#define NDIS_STATUS_MEDIA_DISCONNECT    0x4001000CL


PCHAR DbgGetOidName(ULONG oid);


VOID
NICHandleQueryOidRequest(
    IN WDFQUEUE             Queue,
    IN WDFREQUEST           Request,
    WDF_REQUEST_PARAMETERS  *Params
    )
/*++

Routine Description:

    Query an arbitrary OID value from the miniport.

Arguments:

    Queue - Default queue handle
    Request - IOCTL request handle
    Params - pointer to params structure for the request. This is
             equivalent to the IRP stack location pointer.

Return Value:


--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PNDISPROT_QUERY_OID     pQuery = NULL;
    NDIS_OID                Oid = 0;
    ULONG                   ulInfo = 0;
    ULONG64                 ul64Info = 0;
    PVOID                   pInfo = (PVOID) &ulInfo;
    ULONG                   ulInfoLen = sizeof(ulInfo);
    PVOID                   InformationBuffer = NULL;
    ULONG                   InformationBufferLength = 0;
    MEDIA_STATE             CurrMediaState;
    PVOID                   DataBuffer;
    size_t                  BufferLength;
    NDIS_PNP_CAPABILITIES   Power_Management_Capabilities;
    PFDO_DATA               FdoData = NULL;


    UNREFERENCED_PARAMETER( Params );

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTLS,
                "--> HandleQueryOIDRequest \n");

    FdoData = FdoGetData(WdfIoQueueGetDevice(Queue));

    //
    // Since the IOCTL is buffered, WdfRequestRetrieveOutputBuffer &
    // WdfRequestRetrieveInputBuffer return the same buffer pointer.
    // So make sure you read all the information you need from
    // the buffer before you write to it.
    //
    status = WdfRequestRetrieveOutputBuffer(Request,
                                            sizeof(NDISPROT_QUERY_OID),
                                            &DataBuffer,
                                            &BufferLength);
    if( !NT_SUCCESS(status) ) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTLS,
                    "WdfRequestRetrieveInputBuffer failed 0x%x\n", status);
        WdfRequestComplete(Request, status);
        return;
    }

    do {

        pQuery = (PNDISPROT_QUERY_OID)DataBuffer;
        Oid = pQuery->Oid;
        if(OID_GEN_LINK_SPEED != Oid) { // To avoid flood of trace messages
            TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTLS, "\t%s\n",
                        DbgGetOidName(Oid));
        }
        InformationBuffer = &pQuery->Data[0];
        InformationBufferLength = (ULONG)BufferLength -
                            FIELD_OFFSET(NDISPROT_QUERY_OID, Data);

        switch(Oid)
        {

        case OID_GEN_LINK_SPEED:
        case OID_GEN_MEDIA_CONNECT_STATUS:

            if (InformationBufferLength < sizeof(ULONG))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }


            WdfSpinLockAcquire(FdoData->Lock);
            if (MP_TEST_FLAG(FdoData, fMP_ADAPTER_LINK_DETECTION))
            {
                status = WdfRequestForwardToIoQueue(Request,
                                            FdoData->PendingIoctlQueue);

                WdfSpinLockRelease(FdoData->Lock);
                if(NT_SUCCESS(status)) {
                    goto End;
                }
                break;
            }
            else
            {

                WdfSpinLockRelease(FdoData->Lock);
                if (Oid == OID_GEN_LINK_SPEED)
                {
                    ulInfo = FdoData->usLinkSpeed * 10000;
                } else {

                    CurrMediaState = NICIndicateMediaState(FdoData);
                    ulInfo = CurrMediaState;
                }
            }
            break;

        case OID_802_3_PERMANENT_ADDRESS:

            if (InformationBufferLength < ETH_LENGTH_OF_ADDRESS)
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            pInfo = FdoData->PermanentAddress;
            ulInfoLen = ETH_LENGTH_OF_ADDRESS;
            break;

        case OID_802_3_CURRENT_ADDRESS:

            if (InformationBufferLength < ETH_LENGTH_OF_ADDRESS)
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            pInfo = FdoData->CurrentAddress;
            ulInfoLen = ETH_LENGTH_OF_ADDRESS;
            break;

        case OID_802_3_MAXIMUM_LIST_SIZE:

            if (InformationBufferLength < sizeof(ULONG))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }
            ulInfo = NIC_MAX_MCAST_LIST;
            break;

        case OID_GEN_XMIT_OK:
        case OID_GEN_RCV_OK:
        case OID_GEN_XMIT_ERROR:
        case OID_GEN_RCV_ERROR:
        case OID_GEN_RCV_NO_BUFFER:
        case OID_GEN_RCV_CRC_ERROR:
        case OID_GEN_TRANSMIT_QUEUE_LENGTH:
        case OID_802_3_RCV_ERROR_ALIGNMENT:
        case OID_802_3_XMIT_ONE_COLLISION:
        case OID_802_3_XMIT_MORE_COLLISIONS:
        case OID_802_3_XMIT_DEFERRED:
        case OID_802_3_XMIT_MAX_COLLISIONS:
        case OID_802_3_RCV_OVERRUN:
        case OID_802_3_XMIT_UNDERRUN:
        case OID_802_3_XMIT_HEARTBEAT_FAILURE:
        case OID_802_3_XMIT_TIMES_CRS_LOST:
        case OID_802_3_XMIT_LATE_COLLISIONS:

            if (InformationBufferLength < sizeof(ULONG))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            ulInfoLen = sizeof(ul64Info);
            status = NICGetStatsCounters(FdoData, Oid, &ul64Info);
            if (status == STATUS_SUCCESS)
            {
                ulInfoLen = min(InformationBufferLength, ulInfoLen);
                pInfo = &ul64Info;
            }
            break;

        case OID_PNP_CAPABILITIES:
            //
            // This query is sent during init to get the PNP capabilities of the device.
            //
            NICFillPoMgmtCaps (FdoData,
                                &Power_Management_Capabilities,
                                (PNDIS_STATUS) &status,
                                &ulInfoLen);
            if (status == STATUS_SUCCESS &&
                    ulInfoLen <= InformationBufferLength)
            {
                pInfo = (PVOID) &Power_Management_Capabilities;
            }
            else
            {
                status = STATUS_BUFFER_TOO_SMALL;
                pInfo = NULL;
            }
            break;

        case OID_PNP_QUERY_POWER:
            //
            // NDIS sends the query when it receives Query-DIRP.
            // As a power policy owner, NDIS generates  query-DIRP when it
            // receives query-SIRP from the system.
            // NDIS will forward the D-IRP to lower stack only if we answer this query
            // successfully.
            //
            status = STATUS_SUCCESS;
            break;

        default:
            status = STATUS_NOT_SUPPORTED;
            break;
        }

    } WHILE (FALSE);

    if (status == STATUS_SUCCESS)
    {
        RtlMoveMemory(InformationBuffer, pInfo, ulInfoLen);
    }
    //
    // Adjust the size to include the structure.
    //
    ulInfoLen += FIELD_OFFSET(NDISPROT_QUERY_OID, Data);
    WdfRequestCompleteWithInformation(Request, status, ulInfoLen);

End:

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTLS,
                "<--HandleQueryOIDRequest: Status %x\n",
                status);

    return;

}

VOID
NICHandleSetOidRequest(
    IN WDFQUEUE              Queue,
    IN WDFREQUEST            Request,
    WDF_REQUEST_PARAMETERS  *Params
    )
/*++

Routine Description:

   This routine is called to handle set OID request sent in the ioctl buffer.
   If the device is busy, we will forward the request into a queue and complete
   it later in the DPC.

Arguments:

    Queue - Default queue handle
    Request - IOCTL request handle
    Params - pointer to params structure for the request. This is
             equivalent to the IRP stack location pointer.

Return Value:

    VOID

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PNDISPROT_SET_OID       pSet;
    NDIS_OID                Oid;
    ULONG                   PacketFilter;
    PVOID                   InformationBuffer = NULL;
    ULONG                   InformationBufferLength = 0;
    PVOID                   DataBuffer;
    size_t                  BufferLength;
    ULONG                   unUsed;
    WDF_POWER_DEVICE_STATE  newDeviceState;
    WDF_POWER_DEVICE_STATE  oldDeviceState;
    PFDO_DATA               FdoData = NULL;

    UNREFERENCED_PARAMETER( Params );

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTLS,
                "--> HandleSetOIDRequest\n");

    FdoData = FdoGetData(WdfIoQueueGetDevice(Queue));

    //
    // Since the IOCTL is buffered, WdfRequestRetrieveOutputBuffer &
    // WdfRequestRetrieveInputBuffer return the same buffer pointer.
    // So make sure you read all the information you need from
    // the buffer before you write to it.
    //
    status = WdfRequestRetrieveInputBuffer(Request,
                                           sizeof(NDISPROT_SET_OID),
                                           &DataBuffer,
                                           &BufferLength);
    if( !NT_SUCCESS(status) ) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTLS,
                    "WdfRequestRetrieveInputBuffer failed 0x%x\n",
                    status);
        WdfRequestComplete(Request, status);
        return;
    }

    Oid = 0;

    do {

        if (BufferLength < sizeof(NDISPROT_SET_OID))
        {
            status = STATUS_BUFFER_OVERFLOW;
            break;
        }

        pSet = (PNDISPROT_SET_OID)DataBuffer;
        Oid = pSet->Oid;
        InformationBuffer = &pSet->Data[0];
        InformationBufferLength =
            (ULONG)BufferLength - FIELD_OFFSET(NDISPROT_SET_OID, Data);

        TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTLS, "\t%s\n",
                    DbgGetOidName(Oid));

        switch(Oid)
        {

        case OID_802_3_MULTICAST_LIST:
            //
            // Verify the length
            //
            if (InformationBufferLength % ETH_LENGTH_OF_ADDRESS != 0)
            {
                status = STATUS_INVALID_BUFFER_SIZE;
                break;

            }

            //
            // Save the number of MC list size
            //
            FdoData->MCAddressCount = InformationBufferLength / ETH_LENGTH_OF_ADDRESS;
            ASSERT(FdoData->MCAddressCount <= NIC_MAX_MCAST_LIST);

            //
            // Save the MC list
            //
            RtlMoveMemory(
                FdoData->MCList,
                InformationBuffer,
                InformationBufferLength);


            WdfSpinLockAcquire(FdoData->Lock);

            WdfSpinLockAcquire(FdoData->RcvLock);

            status = NICSetMulticastList(FdoData);


            WdfSpinLockRelease(FdoData->RcvLock);

            WdfSpinLockRelease(FdoData->Lock);
            break;

        case OID_GEN_CURRENT_PACKET_FILTER:
            //
            // Verify the Length
            //
            if (InformationBufferLength != sizeof(ULONG))
            {
                status = STATUS_INVALID_BUFFER_SIZE;
                break;
            }

            RtlMoveMemory(&PacketFilter, InformationBuffer, sizeof(ULONG));

            //
            // any bits not supported?
            //
            if (PacketFilter & ~NIC_SUPPORTED_FILTERS)
            {
                status = STATUS_NOT_SUPPORTED;
                break;
            }

            //
            // any filtering changes?
            //
            if (PacketFilter == FdoData->PacketFilter)
            {
                break;
            }


            WdfSpinLockAcquire(FdoData->Lock);

            WdfSpinLockAcquire(FdoData->RcvLock);

            if (MP_TEST_FLAG(FdoData, fMP_ADAPTER_LINK_DETECTION))
            {

                status = WdfRequestForwardToIoQueue(Request,
                                            FdoData->PendingIoctlQueue);
                WdfSpinLockRelease(FdoData->RcvLock);
                WdfSpinLockRelease(FdoData->Lock);

                if(NT_SUCCESS(status)) {
                    goto End;
                }

                break;
            }

            status = NICSetPacketFilter(
                         FdoData,
                         PacketFilter);


            WdfSpinLockRelease(FdoData->RcvLock);

            WdfSpinLockRelease(FdoData->Lock);

            if (status == STATUS_SUCCESS)
            {
                FdoData->PacketFilter = PacketFilter;
            }

            break;

        case OID_PNP_SET_POWER:

            //
            // NDIS sends this query when it receives Set D-IRP. As a power policy
            // owner, it requests a set D-IRP when it recieves a set S-IRP from the system.
            //
            if (InformationBufferLength != sizeof(NDIS_DEVICE_POWER_STATE ))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            newDeviceState = *(PDEVICE_POWER_STATE UNALIGNED)InformationBuffer;
            oldDeviceState = FdoData->DevicePowerState;
            FdoData->DevicePowerState = newDeviceState;
            //
            // Set the power state - Cannot fail this request.
            //
            status = NICSetPower(FdoData, newDeviceState );

            if (status != STATUS_SUCCESS)
            {
                TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTLS, "SET Power: Hardware error !!!\n");
                break;
            }

            status = STATUS_SUCCESS;
            break;

        case OID_PNP_ADD_WAKE_UP_PATTERN:
            //
            // call a function that would program the adapter's wake
            // up pattern, return success
            //

            status = NICAddWakeUpPattern(FdoData,
                                        InformationBuffer,
                                        InformationBufferLength,
                                        &unUsed,
                                        &unUsed);
            break;


        case OID_PNP_REMOVE_WAKE_UP_PATTERN:

            //
            // call a function that would remove the adapter's wake
            // up pattern, return success
            //

            status = NICRemoveWakeUpPattern(FdoData,
                                               InformationBuffer,
                                               InformationBufferLength,
                                               &unUsed,
                                               &unUsed);

            break;

        case OID_PNP_ENABLE_WAKE_UP:
            //
            // call a function that would enable wake up on the adapter
            // return success
            //
            if (IsPoMgmtSupported(FdoData))
            {
                ULONG       WakeUpEnable;
                RtlMoveMemory(&WakeUpEnable, InformationBuffer,sizeof(ULONG));
                //
                // The WakeUpEable can only be 0, or NDIS_PNP_WAKE_UP_PATTERN_MATCH since the driver only
                // supports wake up pattern match
                //
                if ((WakeUpEnable != 0)
                       && ((WakeUpEnable & NDIS_PNP_WAKE_UP_PATTERN_MATCH) != NDIS_PNP_WAKE_UP_PATTERN_MATCH ))
                {
                    status = STATUS_NOT_SUPPORTED;
                    FdoData->AllowWakeArming = FALSE;
                    break;
                }
                //
                // When the driver goes to low power state, it would check WakeUpEnable to decide
                // which wake up methed it should use to wake up the machine. If WakeUpEnable is 0,
                // no wake up method is enabled.
                //
                FdoData->AllowWakeArming = TRUE;

                status = STATUS_SUCCESS;
            }
            else
            {
                status = STATUS_NOT_SUPPORTED;
            }

            break;
        default:
            status = STATUS_NOT_SUPPORTED;
            break;
        }
    } WHILE (FALSE);

    WdfRequestCompleteWithInformation(Request, status, 0);

End:
    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTLS,
                "<-- HandleSetOIDRequest %x\n", status);

    return;
}


VOID
NICServiceIndicateStatusIrp(
    IN PFDO_DATA        FdoData
    )
/*++

Routine Description:

   We process the IRP based on the input arguments and complete
   the IRP. If the IRP was cancelled for some reason we will let
   the cancel routine do the IRP completion.

Arguments:

    Cancel - Should the IRP be cancelled right away.

Return Value:

    None

--*/
{
    PNDISPROT_INDICATE_STATUS   pIndicateStatus = NULL;
    NTSTATUS                    status;
    ULONG                       bytes = 0;
    size_t                      bufLength;
    WDFREQUEST                  request;

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTLS, "-->ndisServiceIndicateStatusIrp\n");

    status = NICGetIoctlRequest(FdoData->PendingIoctlQueue,
                            IOCTL_NDISPROT_INDICATE_STATUS,
                             &request);

    if(!NT_SUCCESS(status)) {
        return;
    }

    //
    // Since the IOCTL is buffered, WdfRequestRetrieveOutputBuffer &
    // WdfRequestRetrieveInputBuffer return the same buffer pointer.
    // So make sure you read all the information you need from
    // the buffer before you write to it.
    //
    status = WdfRequestRetrieveOutputBuffer(request, sizeof(NDISPROT_INDICATE_STATUS), &pIndicateStatus, &bufLength);
    if( !NT_SUCCESS(status) ) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTLS, "WdfRequestRetrieveInputBuffer failed 0x%x\n", status);
        WdfRequestComplete(request, status);
        return;
    }

    //
    // Check to see whether the buffer is large enough.
    //


    if(MP_TEST_FLAG(FdoData, fMP_ADAPTER_NO_CABLE)){
        pIndicateStatus->IndicatedStatus = NDIS_STATUS_MEDIA_DISCONNECT;
    } else {
        pIndicateStatus->IndicatedStatus = NDIS_STATUS_MEDIA_CONNECT;
    }

    pIndicateStatus->StatusBufferLength = 0;
    pIndicateStatus->StatusBufferOffset = 0;
    status = STATUS_SUCCESS;
    bytes = sizeof(NDISPROT_INDICATE_STATUS);
    WdfRequestCompleteWithInformation(request, status, bytes);

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTLS, "<--ndisServiceIndicateStatusIrp\n");
    return;
}

NTSTATUS
NICGetIoctlRequest(
    IN WDFQUEUE Queue,
    IN ULONG FunctionCode,
    OUT WDFREQUEST*  Request
    )
{
    NTSTATUS            status = STATUS_UNSUCCESSFUL;
    WDF_REQUEST_PARAMETERS params;
    WDFREQUEST          tagRequest;
    WDFREQUEST          prevTagRequest;

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTLS, "--> NICGetIoctlRequest\n");

    WDF_REQUEST_PARAMETERS_INIT(&params);

    *Request = NULL;
    prevTagRequest = tagRequest = NULL;

    do {

        WDF_REQUEST_PARAMETERS_INIT(&params);
        status = WdfIoQueueFindRequest(Queue,
                                    prevTagRequest,
                                    NULL,
                                    &params,
                                    &tagRequest);

        //
        // WdfIoQueueFindRequest takes an extra reference on the returned tagRequest to
        // prevent the memory from being freed. However, the tagRequest still
        // in the queue and can be cancelled or removed by another thread and
        // completed.
        //
        if(prevTagRequest) {
            WdfObjectDereference(prevTagRequest);
        }

        if(status == STATUS_NO_MORE_ENTRIES) {
            status = STATUS_UNSUCCESSFUL;
            break;
        }

        if(status == STATUS_NOT_FOUND) {
            //
            // It seems like prevTagRequest disappeared from the
            // queue for some reason - either it got cancelled, got
            // dispatched to the driver. There might be other requests
            // that match our criteria so let us restart the search.
            //
            prevTagRequest = tagRequest = NULL;
            continue;
        }

        if( !NT_SUCCESS(status)) {
            //
            // Something bad happened.
            //
            TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTLS,
                                        "WdfIoQueueFindRequest failed %!STATUS!\n", status);
            status = STATUS_UNSUCCESSFUL;
            break;
        }

        if(FunctionCode == params.Parameters.DeviceIoControl.IoControlCode){

            status = WdfIoQueueRetrieveFoundRequest(
                     Queue,
                     tagRequest,   // TagRequest
                     Request
                     );

            WdfObjectDereference(tagRequest);

            if(status == STATUS_NOT_FOUND) {
                //
                // It seems like the tagrequest disappeared from the
                // queue for some reason - either it got cancelled, got
                // dispatched to the driver.  There might be other requests
                // that match our criteria so let us restart the search.
                //
                prevTagRequest = tagRequest = NULL;
                continue;
            }

            if( !NT_SUCCESS(status)) {
                TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTLS,
                                            "WdfIoQueueRetrieveNextRequest failed %!STATUS!\n", status);
                status = STATUS_UNSUCCESSFUL;
                break;
            }

            //
            //  We got a request. Drop the extra reference taken by peek request before
            // returning the call.
            //
            ASSERT(*Request == tagRequest);
            status =  STATUS_SUCCESS;
            break;

        }else {
            //
            // This is not the request we need. We will drop the reference
            // on the tagrequest after we looking for the next request.
            //
            prevTagRequest = tagRequest;
            continue;
        }

    } WHILE (TRUE);

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTLS, "<-- NICGetIoctlRequest\n");

    return status;

}

MEDIA_STATE
NICIndicateMediaState(
    IN PFDO_DATA FdoData
    )
{
    MEDIA_STATE CurrMediaState;


    WdfSpinLockAcquire(FdoData->Lock);

    CurrMediaState = GetMediaState(FdoData);

    if (CurrMediaState != FdoData->MediaState)
    {
        TraceEvents(TRACE_LEVEL_WARNING, DBG_IOCTLS, "Media state changed to %s\n",
            ((CurrMediaState == Connected)?
            "Connected": "Disconnected"));

        FdoData->MediaState = CurrMediaState;

        if (CurrMediaState == Connected)
        {
            MP_CLEAR_FLAG(FdoData, fMP_ADAPTER_NO_CABLE);
        }
        else
        {
            MP_SET_FLAG(FdoData, fMP_ADAPTER_NO_CABLE);
        }


        WdfSpinLockRelease(FdoData->Lock);

        // Indicate the media event
        NICServiceIndicateStatusIrp(FdoData);
    }
    else
    {

        WdfSpinLockRelease(FdoData->Lock);
    }

    return CurrMediaState;
}


NTSTATUS
NICGetStatsCounters(
    IN  PFDO_DATA  FdoData,
    IN  NDIS_OID     Oid,
    OUT PULONG64     pCounter
    )
/*++
Routine Description:

    Get the value for a statistics OID

Arguments:

    FdoData     Pointer to our FdoData
    Oid         Self-explanatory
    pCounter    Pointer to receive the value

Return Value:

    NT Status code

--*/
{
    NTSTATUS     status = STATUS_SUCCESS;

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTLS, "--> NICGetStatsCounters\n");

    *pCounter = 0;

    DumpStatsCounters(FdoData);

    switch(Oid)
    {
        case OID_GEN_XMIT_OK:
            *pCounter = FdoData->GoodTransmits;
            break;

        case OID_GEN_RCV_OK:
            *pCounter = FdoData->GoodReceives;
            break;

        case OID_GEN_XMIT_ERROR:
            *pCounter = FdoData->TxAbortExcessCollisions +
                        FdoData->TxDmaUnderrun +
                        FdoData->TxLostCRS +
                        FdoData->TxLateCollisions;
            break;

        case OID_GEN_RCV_ERROR:
            *pCounter = FdoData->RcvCrcErrors +
                        FdoData->RcvAlignmentErrors +
                        FdoData->RcvResourceErrors +
                        FdoData->RcvDmaOverrunErrors +
                        FdoData->RcvRuntErrors;
            break;

        case OID_GEN_RCV_NO_BUFFER:
            *pCounter = FdoData->RcvResourceErrors;
            break;

        case OID_GEN_RCV_CRC_ERROR:
            *pCounter = FdoData->RcvCrcErrors;
            break;

        case OID_GEN_TRANSMIT_QUEUE_LENGTH:
            *pCounter = FdoData->nWaitSend;
            break;

        case OID_802_3_RCV_ERROR_ALIGNMENT:
            *pCounter = FdoData->RcvAlignmentErrors;
            break;

        case OID_802_3_XMIT_ONE_COLLISION:
            *pCounter = FdoData->OneRetry;
            break;

        case OID_802_3_XMIT_MORE_COLLISIONS:
            *pCounter = FdoData->MoreThanOneRetry;
            break;

        case OID_802_3_XMIT_DEFERRED:
            *pCounter = FdoData->TxOKButDeferred;
            break;

        case OID_802_3_XMIT_MAX_COLLISIONS:
            *pCounter = FdoData->TxAbortExcessCollisions;
            break;

        case OID_802_3_RCV_OVERRUN:
            *pCounter = FdoData->RcvDmaOverrunErrors;
            break;

        case OID_802_3_XMIT_UNDERRUN:
            *pCounter = FdoData->TxDmaUnderrun;
            break;

        case OID_802_3_XMIT_HEARTBEAT_FAILURE:
            *pCounter = FdoData->TxLostCRS;
            break;

        case OID_802_3_XMIT_TIMES_CRS_LOST:
            *pCounter = FdoData->TxLostCRS;
            break;

        case OID_802_3_XMIT_LATE_COLLISIONS:
            *pCounter = FdoData->TxLateCollisions;
            break;

        default:
            status = STATUS_NOT_SUPPORTED;
            break;
    }

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTLS, "<-- NICGetStatsCounters\n");

    return(status);
}

NTSTATUS NICSetPacketFilter(
    IN PFDO_DATA FdoData,
    IN ULONG PacketFilter
    )
/*++
Routine Description:

    This routine will set up the FdoData so that it accepts packets
    that match the specified packet filter.  The only filter bits
    that can truly be toggled are for broadcast and promiscuous

Arguments:

    FdoData         Pointer to our FdoData
    PacketFilter    The new packet filter

Return Value:


--*/
{
    NTSTATUS        status = STATUS_SUCCESS;
    UCHAR           NewParameterField;
    UINT            i;
    BOOLEAN         bResult;

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTLS, "--> NICSetPacketFilter, PacketFilter=%08x\n", PacketFilter);

    //
    // Need to enable or disable broadcast and promiscuous support depending
    // on the new filter
    //
    NewParameterField = CB_557_CFIG_DEFAULT_PARM15;

    if (PacketFilter & NDIS_PACKET_TYPE_BROADCAST)
    {
        NewParameterField &= ~CB_CFIG_BROADCAST_DIS;
    }
    else
    {
        NewParameterField |= CB_CFIG_BROADCAST_DIS;
    }

    if (PacketFilter & NDIS_PACKET_TYPE_PROMISCUOUS)
    {
        NewParameterField |= CB_CFIG_PROMISCUOUS;
    }
    else
    {
        NewParameterField &= ~CB_CFIG_PROMISCUOUS;
    }

    do
    {
        if ((FdoData->OldParameterField == NewParameterField ) &&
            !(PacketFilter & NDIS_PACKET_TYPE_ALL_MULTICAST))
        {
            break;
        }

        //
        // Only need to do something to the HW if the filter bits have changed.
        //
        FdoData->OldParameterField = NewParameterField;
        ((PCB_HEADER_STRUC)FdoData->NonTxCmdBlock)->CbCommand = CB_CONFIGURE;
        ((PCB_HEADER_STRUC)FdoData->NonTxCmdBlock)->CbStatus = 0;
        ((PCB_HEADER_STRUC)FdoData->NonTxCmdBlock)->CbLinkPointer = DRIVER_NULL;

        //
        // First fill in the static (end user can't change) config bytes
        //
        FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[0] = CB_557_CFIG_DEFAULT_PARM0;
        FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[2] = CB_557_CFIG_DEFAULT_PARM2;
        FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[3] = CB_557_CFIG_DEFAULT_PARM3;
        FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[6] = CB_557_CFIG_DEFAULT_PARM6;
        FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[9] = CB_557_CFIG_DEFAULT_PARM9;
        FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[10] = CB_557_CFIG_DEFAULT_PARM10;
        FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[11] = CB_557_CFIG_DEFAULT_PARM11;
        FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[12] = CB_557_CFIG_DEFAULT_PARM12;
        FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[13] = CB_557_CFIG_DEFAULT_PARM13;
        FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[14] = CB_557_CFIG_DEFAULT_PARM14;
        FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[16] = CB_557_CFIG_DEFAULT_PARM16;
        FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[17] = CB_557_CFIG_DEFAULT_PARM17;
        FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[18] = CB_557_CFIG_DEFAULT_PARM18;
        FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[20] = CB_557_CFIG_DEFAULT_PARM20;

        //
        // Set the Tx underrun retries
        //
        FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[7] =
            (UCHAR) (CB_557_CFIG_DEFAULT_PARM7 | (FdoData->AiUnderrunRetry << 1));

        //
        // Set the Tx and Rx Fifo limits
        //
        FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[1] =
            (UCHAR) ((FdoData->AiTxFifo << 4) | FdoData->AiRxFifo);

        //
        // set the MWI enable bit if needed
        //
        if (FdoData->MWIEnable)
            FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[3] |= CB_CFIG_B3_MWI_ENABLE;

        //
        // Set the Tx and Rx DMA maximum byte count fields.
        //
        if ((FdoData->AiRxDmaCount) || (FdoData->AiTxDmaCount))
        {
            FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[4] =
                FdoData->AiRxDmaCount;
            FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[5] =
                (UCHAR) (FdoData->AiTxDmaCount | CB_CFIG_DMBC_EN);
        }
        else
        {
            FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[4] =
                CB_557_CFIG_DEFAULT_PARM4;
            FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[5] =
                CB_557_CFIG_DEFAULT_PARM5;
        }

        //
        // Setup for MII or 503 operation.  The CRS+CDT bit should only be
        // set when operating in 503 mode.
        //
        if (FdoData->PhyAddress == 32)
        {
            FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[8] =
                (CB_557_CFIG_DEFAULT_PARM8 & (~CB_CFIG_503_MII));
            FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[15] =
                (UCHAR) (NewParameterField | CB_CFIG_CRS_OR_CDT);
        }
        else
        {
            FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[8] =
                (CB_557_CFIG_DEFAULT_PARM8 | CB_CFIG_503_MII);
            FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[15] =
                (UCHAR) (NewParameterField & (~CB_CFIG_CRS_OR_CDT));
        }

        //
        // Setup Full duplex stuff
        //

        //
        // If forced to half duplex
        //
        if (FdoData->AiForceDpx == 1)
            {
            FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[19] =
                (CB_557_CFIG_DEFAULT_PARM19 &
                (~(CB_CFIG_FORCE_FDX| CB_CFIG_FDX_ENABLE)));
        }
        //
        // If forced to full duplex
        //
        else if (FdoData->AiForceDpx == 2)
            {
            FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[19] =
                (CB_557_CFIG_DEFAULT_PARM19 | CB_CFIG_FORCE_FDX);
        }
        //
        // If auto-duplex
        //
        else
            {
            FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[19] =
                                                CB_557_CFIG_DEFAULT_PARM19;
        }

        //
        // if multicast all is being turned on, set the bit
        //
        if (PacketFilter & NDIS_PACKET_TYPE_ALL_MULTICAST)
            {
            FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[21] =
                                 (CB_557_CFIG_DEFAULT_PARM21 | CB_CFIG_MULTICAST_ALL);
        }
        else
            {
            FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[21] =
                                                CB_557_CFIG_DEFAULT_PARM21;
        }


        //
        // Wait for the SCB to clear before we check the CU status.
        //
        if (!WaitScb(FdoData))
        {
            status = STATUS_DEVICE_DATA_ERROR;
            break;
        }

        //
        // If we have issued any transmits, then the CU will either be active,
        // or in the suspended state.  If the CU is active, then we wait for
        // it to be suspended.
        //
        if (FdoData->TransmitIdle == FALSE)
        {
            //
            // Wait for suspended state
            //
            MP_STALL_AND_WAIT((FdoData->CSRAddress->ScbStatus & SCB_CUS_MASK) != SCB_CUS_ACTIVE, 5000, bResult);
            if (!bResult)
            {
                MP_SET_HARDWARE_ERROR(FdoData);
                status = STATUS_DEVICE_DATA_ERROR;
                break;
            }

            //
            // Check the current status of the receive unit
            //
            if ((FdoData->CSRAddress->ScbStatus & SCB_RUS_MASK) != SCB_RUS_IDLE)
            {
                // Issue an RU abort.  Since an interrupt will be issued, the
                // RU will be started by the DPC.
                status = D100IssueScbCommand(FdoData, SCB_RUC_ABORT, TRUE);
                if (status != STATUS_SUCCESS)
                {
                    break;
                }
            }

            if (!WaitScb(FdoData))
            {
                status = STATUS_DEVICE_DATA_ERROR;
                break;
            }

            //
            // Restore the transmit software flags.  After the multicast
            // command is issued, the command unit will be idle, because the
            // EL bit will be set in the multicast commmand block.
            //
            FdoData->TransmitIdle = TRUE;
            FdoData->ResumeWait = TRUE;
        }

        //
        // Display config information
        //
        TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTLS, "Re-Issuing Configure command for filter change\n");
        TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTLS, "Config Block at virt addr %p, phys address %x\n",
            &((PCB_HEADER_STRUC)FdoData->NonTxCmdBlock)->CbStatus, FdoData->NonTxCmdBlockPhys);

        for (i = 0; i < CB_CFIG_BYTE_COUNT; i++)
            TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTLS, "  Config byte %x = %.2x\n",
                        i, FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[i]);

        //
        // Submit the configure command to the chip, and wait for it to complete.
        //
        FdoData->CSRAddress->ScbGeneralPointer = FdoData->NonTxCmdBlockPhys;
        status = D100SubmitCommandBlockAndWait(FdoData);
        if (status != STATUS_SUCCESS)
        {
            status = STATUS_DEVICE_NOT_READY;
        }

    } WHILE (FALSE);

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTLS, "<-- NICSetPacketFilter, Status=%x\n", status);

    return(status);
}

NTSTATUS
NICSetMulticastList(
    IN  PFDO_DATA  FdoData
    )
/*++
Routine Description:

    This routine will set up the FdoData for a specified multicast address list

Arguments:

    FdoData     Pointer to our FdoData

Return Value:


--*/
{
    NTSTATUS        status;
    PUCHAR          McAddress;
    UINT            i, j;
    BOOLEAN         bResult;

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTLS, "--> NICSetMulticastList\n");

    //
    // Setup the command block for the multicast command.
    //
    for (i = 0; i < FdoData->MCAddressCount; i++)
    {
        TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTLS, "MC(%d) = %02x-%02x-%02x-%02x-%02x-%02x\n",
            i,
            FdoData->MCList[i][0],
            FdoData->MCList[i][1],
            FdoData->MCList[i][2],
            FdoData->MCList[i][3],
            FdoData->MCList[i][4],
            FdoData->MCList[i][5]);

        McAddress = &FdoData->NonTxCmdBlock->NonTxCb.Multicast.McAddress[i*ETHERNET_ADDRESS_LENGTH];

        for (j = 0; j < ETH_LENGTH_OF_ADDRESS; j++)
            *(McAddress++) = FdoData->MCList[i][j];
    }

    FdoData->NonTxCmdBlock->NonTxCb.Multicast.McCount =
        (USHORT)(FdoData->MCAddressCount * ETH_LENGTH_OF_ADDRESS);
    ((PCB_HEADER_STRUC)FdoData->NonTxCmdBlock)->CbStatus = 0;
    ((PCB_HEADER_STRUC)FdoData->NonTxCmdBlock)->CbCommand = CB_MULTICAST;

    //
    // Wait for the SCB to clear before we check the CU status.
    //
    if (!WaitScb(FdoData))
    {
        status = STATUS_DEVICE_DATA_ERROR;
        goto exit;
    }

    //
    // If we have issued any transmits, then the CU will either be active, or
    // in the suspended state.  If the CU is active, then we wait for it to be
    // suspended.
    //
    if (FdoData->TransmitIdle == FALSE)
    {
        //
        // Wait for suspended state
        //
        MP_STALL_AND_WAIT((FdoData->CSRAddress->ScbStatus & SCB_CUS_MASK) != SCB_CUS_ACTIVE, 5000, bResult);
        if (!bResult)
        {
            MP_SET_HARDWARE_ERROR(FdoData);
            status = STATUS_DEVICE_DATA_ERROR;
        }

        //
        // Restore the transmit software flags.  After the multicast command is
        // issued, the command unit will be idle, because the EL bit will be
        // set in the multicast commmand block.
        //
        FdoData->TransmitIdle = TRUE;
        FdoData->ResumeWait = TRUE;
    }

    //
    // Update the command list pointer.
    //
    FdoData->CSRAddress->ScbGeneralPointer = FdoData->NonTxCmdBlockPhys;

    //
    // Submit the multicast command to the FdoData and wait for it to complete.
    //
    status = D100SubmitCommandBlockAndWait(FdoData);
    if (status != STATUS_SUCCESS)
    {
        status = STATUS_DEVICE_NOT_READY;
    }

    exit:

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTLS, "<-- NICSetMulticastList, Status=%x\n", status);

    return(status);

}


VOID
NICFillPoMgmtCaps (
    IN PFDO_DATA                 FdoData,
    IN OUT PNDIS_PNP_CAPABILITIES  pPower_Management_Capabilities,
    IN OUT PNDIS_STATUS            pStatus,
    IN OUT PULONG                  pulInfoLen
    )
/*++
Routine Description:

    Fills in the Power  Managment structure depending the capabilities of
    the software driver and the card.

    Currently this is only supported on 82559 Version of the driver

Arguments:

    FdoData                 Pointer to the FdoData structure
    pPower_Management_Capabilities - Power management struct as defined in the DDK,
    pStatus                 Status to be returned by the request,
    pulInfoLen              Length of the pPowerManagmentCapabilites

Return Value:

    Success or failure depending on the type of card
--*/

{

    BOOLEAN bIsPoMgmtSupported;

    bIsPoMgmtSupported = IsPoMgmtSupported(FdoData);

    if (bIsPoMgmtSupported == TRUE)
    {
        pPower_Management_Capabilities->Flags = NDIS_DEVICE_WAKE_UP_ENABLE;
        pPower_Management_Capabilities->WakeUpCapabilities.MinMagicPacketWakeUp = NdisDeviceStateUnspecified;
        pPower_Management_Capabilities->WakeUpCapabilities.MinPatternWakeUp = NdisDeviceStateD3;
        pPower_Management_Capabilities->WakeUpCapabilities.MinLinkChangeWakeUp  = NdisDeviceStateUnspecified;
        *pulInfoLen = sizeof (*pPower_Management_Capabilities);
        *pStatus =  STATUS_SUCCESS;
    }
    else
    {
        RtlZeroMemory (pPower_Management_Capabilities, sizeof(*pPower_Management_Capabilities));
        *pStatus =  STATUS_NOT_SUPPORTED;
        *pulInfoLen = 0;

    }
}

PCHAR
DbgGetOidName(ULONG oid)
{
    PCHAR oidName;

    switch (oid){

        #undef MAKECASE
        #define MAKECASE(oidx) case oidx: oidName = #oidx; break;

        MAKECASE(OID_GEN_SUPPORTED_LIST)
        MAKECASE(OID_GEN_HARDWARE_STATUS)
        MAKECASE(OID_GEN_MEDIA_SUPPORTED)
        MAKECASE(OID_GEN_MEDIA_IN_USE)
        MAKECASE(OID_GEN_MAXIMUM_LOOKAHEAD)
        MAKECASE(OID_GEN_MAXIMUM_FRAME_SIZE)
        MAKECASE(OID_GEN_LINK_SPEED)
        MAKECASE(OID_GEN_TRANSMIT_BUFFER_SPACE)
        MAKECASE(OID_GEN_RECEIVE_BUFFER_SPACE)
        MAKECASE(OID_GEN_TRANSMIT_BLOCK_SIZE)
        MAKECASE(OID_GEN_RECEIVE_BLOCK_SIZE)
        MAKECASE(OID_GEN_VENDOR_ID)
        MAKECASE(OID_GEN_VENDOR_DESCRIPTION)
        MAKECASE(OID_GEN_CURRENT_PACKET_FILTER)
        MAKECASE(OID_GEN_CURRENT_LOOKAHEAD)
        MAKECASE(OID_GEN_DRIVER_VERSION)
        MAKECASE(OID_GEN_MAXIMUM_TOTAL_SIZE)
        MAKECASE(OID_GEN_PROTOCOL_OPTIONS)
        MAKECASE(OID_GEN_MAC_OPTIONS)
        MAKECASE(OID_GEN_MEDIA_CONNECT_STATUS)
        MAKECASE(OID_GEN_MAXIMUM_SEND_PACKETS)
        MAKECASE(OID_GEN_VENDOR_DRIVER_VERSION)
        MAKECASE(OID_GEN_SUPPORTED_GUIDS)
        MAKECASE(OID_GEN_NETWORK_LAYER_ADDRESSES)
        MAKECASE(OID_GEN_TRANSPORT_HEADER_OFFSET)
        MAKECASE(OID_GEN_MEDIA_CAPABILITIES)
        MAKECASE(OID_GEN_PHYSICAL_MEDIUM)
        MAKECASE(OID_GEN_XMIT_OK)
        MAKECASE(OID_GEN_RCV_OK)
        MAKECASE(OID_GEN_XMIT_ERROR)
        MAKECASE(OID_GEN_RCV_ERROR)
        MAKECASE(OID_GEN_RCV_NO_BUFFER)
        MAKECASE(OID_GEN_DIRECTED_BYTES_XMIT)
        MAKECASE(OID_GEN_DIRECTED_FRAMES_XMIT)
        MAKECASE(OID_GEN_MULTICAST_BYTES_XMIT)
        MAKECASE(OID_GEN_MULTICAST_FRAMES_XMIT)
        MAKECASE(OID_GEN_BROADCAST_BYTES_XMIT)
        MAKECASE(OID_GEN_BROADCAST_FRAMES_XMIT)
        MAKECASE(OID_GEN_DIRECTED_BYTES_RCV)
        MAKECASE(OID_GEN_DIRECTED_FRAMES_RCV)
        MAKECASE(OID_GEN_MULTICAST_BYTES_RCV)
        MAKECASE(OID_GEN_MULTICAST_FRAMES_RCV)
        MAKECASE(OID_GEN_BROADCAST_BYTES_RCV)
        MAKECASE(OID_GEN_BROADCAST_FRAMES_RCV)
        MAKECASE(OID_GEN_RCV_CRC_ERROR)
        MAKECASE(OID_GEN_TRANSMIT_QUEUE_LENGTH)
        MAKECASE(OID_GEN_GET_TIME_CAPS)
        MAKECASE(OID_GEN_GET_NETCARD_TIME)
        MAKECASE(OID_GEN_NETCARD_LOAD)
        MAKECASE(OID_GEN_DEVICE_PROFILE)
        MAKECASE(OID_GEN_INIT_TIME_MS)
        MAKECASE(OID_GEN_RESET_COUNTS)
        MAKECASE(OID_GEN_MEDIA_SENSE_COUNTS)
        MAKECASE(OID_PNP_CAPABILITIES)
        MAKECASE(OID_PNP_SET_POWER)
        MAKECASE(OID_PNP_QUERY_POWER)
        MAKECASE(OID_PNP_ADD_WAKE_UP_PATTERN)
        MAKECASE(OID_PNP_REMOVE_WAKE_UP_PATTERN)
        MAKECASE(OID_PNP_ENABLE_WAKE_UP)
        MAKECASE(OID_802_3_PERMANENT_ADDRESS)
        MAKECASE(OID_802_3_CURRENT_ADDRESS)
        MAKECASE(OID_802_3_MULTICAST_LIST)
        MAKECASE(OID_802_3_MAXIMUM_LIST_SIZE)
        MAKECASE(OID_802_3_MAC_OPTIONS)
        MAKECASE(OID_802_3_RCV_ERROR_ALIGNMENT)
        MAKECASE(OID_802_3_XMIT_ONE_COLLISION)
        MAKECASE(OID_802_3_XMIT_MORE_COLLISIONS)
        MAKECASE(OID_802_3_XMIT_DEFERRED)
        MAKECASE(OID_802_3_XMIT_MAX_COLLISIONS)
        MAKECASE(OID_802_3_RCV_OVERRUN)
        MAKECASE(OID_802_3_XMIT_UNDERRUN)
        MAKECASE(OID_802_3_XMIT_HEARTBEAT_FAILURE)
        MAKECASE(OID_802_3_XMIT_TIMES_CRS_LOST)
        MAKECASE(OID_802_3_XMIT_LATE_COLLISIONS)

        default:
            oidName = "<** UNKNOWN OID **>";
            break;
    }

    return oidName;
}


