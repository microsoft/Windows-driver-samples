/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    CtrlPath.c

Abstract:

    This module implements the miniport's control path.  It contains the main
    miniport entrypoint for OID handling.

--*/


#include "netvmin6.h"
#include "ctrlpath.tmh"


static
NDIS_STATUS
MPMethodRequest(
    _In_  PMP_ADAPTER             Adapter,
    _In_  PNDIS_OID_REQUEST       NdisRequest);

static
NDIS_STATUS
MPSynchronousMethodRequest(
    _In_  PMP_ADAPTER             Adapter,
    _In_  PNDIS_OID_REQUEST       NdisRequest);

static
NDIS_STATUS
MPSetInformation(
    _In_  PMP_ADAPTER         Adapter,
    _In_  PNDIS_OID_REQUEST   NdisSetRequest);

static
NDIS_STATUS
MPQueryInformation(
    _In_  PMP_ADAPTER        Adapter,
    _Inout_ PNDIS_OID_REQUEST  NdisQueryRequest);

static
NDIS_STATUS
NICSetMulticastList(
    _In_  PMP_ADAPTER        Adapter,
    _In_  PNDIS_OID_REQUEST  NdisSetRequest);

static
NDIS_STATUS
NICSetPacketFilter(
    _In_ PMP_ADAPTER Adapter,
    _In_ ULONG PacketFilter);

#if (NDIS_SUPPORT_NDIS620)

static
NDIS_STATUS
NICAllocateRxQueue(
    _In_ PMP_ADAPTER        Adapter,
    _In_ PNDIS_OID_REQUEST  NdisMethodRequest);

static
NDIS_STATUS
NICCompleteAllocationRxQueue(
    _In_ PMP_ADAPTER        Adapter,
    _In_ PNDIS_OID_REQUEST  NdisMethodRequest);

static
NDIS_STATUS
NICFreeRxQueue(
    _In_ PMP_ADAPTER        Adapter,
    _In_ PNDIS_OID_REQUEST  NdisSetRequest);

static
NDIS_STATUS
NICSetRxFilter(
    _In_ PMP_ADAPTER        Adapter,
    _In_ PNDIS_OID_REQUEST  NdisMethodRequest);

static
NDIS_STATUS
NICClearRxFilter(
    _In_ PMP_ADAPTER        Adapter,
    _In_ PNDIS_OID_REQUEST  NdisSetRequest);

static
NDIS_STATUS
NICUpdateRxQueue(
    _In_ PMP_ADAPTER        Adapter,
    _In_ PNDIS_OID_REQUEST  NdisSetRequest);

_IRQL_requires_(PASSIVE_LEVEL)
static
NDIS_STATUS
NICSetQOSParameters(
    _In_ PMP_ADAPTER        Adapter,
    _In_ PNDIS_OID_REQUEST  NdisMethodRequest);

#pragma NDIS_PAGEABLE_FUNCTION(NICFreeRxQueue)
#pragma NDIS_PAGEABLE_FUNCTION(NICClearRxFilter)
#pragma NDIS_PAGEABLE_FUNCTION(NICUpdateRxQueue)
#pragma NDIS_PAGEABLE_FUNCTION(NICAllocateRxQueue)
#pragma NDIS_PAGEABLE_FUNCTION(NICCompleteAllocationRxQueue)
#pragma NDIS_PAGEABLE_FUNCTION(NICSetRxFilter)
#pragma NDIS_PAGEABLE_FUNCTION(NICSetQOSParameters)

#endif

#if (NDIS_SUPPORT_NDIS680)
static
NDIS_STATUS
MPSetRSSv2Parameters(
    _In_ PMP_ADAPTER        Adapter,
    _In_ PNDIS_OID_REQUEST  NdisSetRequest);

static
NDIS_STATUS
MPSetRSSv2IndirectionTableEntries(
    _In_ PMP_ADAPTER        Adapter,
    _In_ PNDIS_OID_REQUEST  NdisSetRequest);
#endif

static
NDIS_STATUS
MPSetPower(
    _In_ PMP_ADAPTER        Adapter,
    _In_ PNDIS_OID_REQUEST  NdisSetRequest);

static
NDIS_STATUS
MPSetPowerD0(
    _In_ PMP_ADAPTER        Adapter);

static
NDIS_STATUS
MPSetPowerLow(
    _In_ PMP_ADAPTER                Adapter,
    _In_ NDIS_DEVICE_POWER_STATE    PowerState);


#pragma NDIS_PAGEABLE_FUNCTION(MPOidRequest)
#pragma NDIS_PAGEABLE_FUNCTION(MPQueryInformation)
#pragma NDIS_PAGEABLE_FUNCTION(MPSetInformation)
#pragma NDIS_PAGEABLE_FUNCTION(MPMethodRequest)
#pragma NDIS_PAGEABLE_FUNCTION(MPSetPower)
#pragma NDIS_PAGEABLE_FUNCTION(MPSetPowerD0)
#pragma NDIS_PAGEABLE_FUNCTION(MPSetPowerLow)
#pragma NDIS_PAGEABLE_FUNCTION(NICSetMulticastList)
#pragma NDIS_PAGEABLE_FUNCTION(NICSetPacketFilter)


NDIS_STATUS
MPOidRequest(
    _In_  NDIS_HANDLE        MiniportAdapterContext,
    _In_  PNDIS_OID_REQUEST  NdisRequest)
/*++

Routine Description:

    Entry point called by NDIS to get or set the value of a specified OID.

Arguments:

    MiniportAdapterContext  - Our adapter handle
    NdisRequest             - The OID request to handle

Return Value:

    Return code from the NdisRequest below.

--*/
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PMP_ADAPTER Adapter = MP_ADAPTER_FROM_CONTEXT(MiniportAdapterContext);

    PAGED_CODE();

    DEBUGP(MP_LOUD, "[%p] ---> MPOidRequest\n", Adapter);


    switch (NdisRequest->RequestType)
    {
        case NdisRequestMethod:
            Status = MPMethodRequest(Adapter, NdisRequest);
            break;

        case NdisRequestSetInformation:
            Status = MPSetInformation(Adapter, NdisRequest);
            break;

        case NdisRequestQueryInformation:
        case NdisRequestQueryStatistics:
            Status = MPQueryInformation(Adapter, NdisRequest);
            break;

        default:
            //
            // The entry point may by used by other requests
            //
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;
    }


    DEBUGP(MP_LOUD, "[%p] <--- MPOidRequest Status = 0x%08x\n", Adapter, Status);
    return Status;
}

#if (NDIS_SUPPORT_NDIS680)
NDIS_STATUS
MPSynchronousOidRequest(
    _In_  NDIS_HANDLE        MiniportAdapterContext,
    _In_  PNDIS_OID_REQUEST  NdisRequest)
/*++

Routine Description:

    Entry point called by NDIS to get or set the value of a specified 
    synchronous OID.

Arguments:

    MiniportAdapterContext  - Our adapter handle
    NdisRequest             - The OID request to handle

Return Value:

    Return code from the NdisRequest below.

--*/
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PMP_ADAPTER Adapter = MP_ADAPTER_FROM_CONTEXT(MiniportAdapterContext);

    DEBUGP(MP_LOUD, "[%p] ---> MPSynchronousOidRequest\n", Adapter);

    switch (NdisRequest->RequestType)
    {
        case NdisRequestMethod:
            Status = MPSynchronousMethodRequest(Adapter, NdisRequest);
            break;
        default:
            //
            // The entry point may by used by other requests
            //
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;
    }


    DEBUGP(MP_LOUD, "[%p] <--- MPSynchronousOidRequest Status = 0x%08x\n", Adapter, Status);
    return Status;
}
#endif

VOID
MPCancelOidRequest(
    _In_  NDIS_HANDLE  MiniportAdapterContext,
    _In_  PVOID        RequestId)
/*++

Routine Description:

    Entry point called by NDIS to abort an asynchronous OID request.

Arguments:

    MiniportAdapterContext  - Our adapter handle
    RequestId               - An identifier that corresponds to the RequestId
                              field of the NDIS_OID_REQUEST

Return Value:

    None.

--*/
{
    PMP_ADAPTER Adapter = MP_ADAPTER_FROM_CONTEXT(MiniportAdapterContext);

    DEBUGP(MP_LOUD, "[%p] ---> MPCancelOidRequest", Adapter);
    UNREFERENCED_PARAMETER(Adapter);
    UNREFERENCED_PARAMETER(RequestId);


    //
    // This miniport sample does not pend any OID requests, so we don't have
    // to worry about cancelling them.
    //

    DEBUGP(MP_LOUD, "[%p] <--- MPCancelOidRequest", Adapter);
}



NDIS_STATUS
MPQueryInformation(
    _In_  PMP_ADAPTER        Adapter,
    _Inout_ PNDIS_OID_REQUEST  NdisQueryRequest)
/*++

Routine Description:

    Helper function to perform a query OID request

Arguments:

    Adapter           -
    NdisQueryRequest  - The OID that is being queried

Return Value:

    NDIS_STATUS

--*/
{
    NDIS_STATUS             Status = NDIS_STATUS_SUCCESS;
    struct _QUERY          *Query = &NdisQueryRequest->DATA.QUERY_INFORMATION;

    NDIS_HARDWARE_STATUS    HardwareStatus = NdisHardwareStatusReady;
    UCHAR                   VendorDesc[] = NIC_VENDOR_DESC;
    NDIS_MEDIUM             Medium = NIC_MEDIUM_TYPE;
    ULONG                   ulInfo;
    USHORT                  usInfo;
    ULONG64                 ulInfo64;

    // Default to returning the ULONG value
    PVOID                   pInfo=NULL;
    ULONG                   ulInfoLen = sizeof(ulInfo);

    PAGED_CODE();

    DEBUGP(MP_LOUD, "[%p] ---> MPQueryInformation ", Adapter);
    DbgPrintOidName(Query->Oid);


    switch(Query->Oid)
    {
        case OID_GEN_HARDWARE_STATUS:
            //
            // Specify the current hardware status of the underlying NIC as
            // one of the following NDIS_HARDWARE_STATUS-type values.
            //
            pInfo = (PVOID) &HardwareStatus;
            ulInfoLen = sizeof(NDIS_HARDWARE_STATUS);
            break;

        case OID_GEN_MAXIMUM_TOTAL_SIZE:
            //
            // Specify the maximum total packet length, in bytes, the NIC
            // supports including the header. A protocol driver might use
            // this returned length as a gauge to determine the maximum
            // size packet that a NIC driver could forward to the
            // protocol driver. The miniport driver must never indicate
            // up to the bound protocol driver packets received over the
            // network that are longer than the packet size specified by
            // OID_GEN_MAXIMUM_TOTAL_SIZE.
            //

            __fallthrough;

        case OID_GEN_TRANSMIT_BLOCK_SIZE:
            //
            // The OID_GEN_TRANSMIT_BLOCK_SIZE OID specifies the minimum
            // number of bytes that a single net packet occupies in the
            // transmit buffer space of the NIC. For example, a NIC that
            // has a transmit space divided into 256-byte pieces would have
            // a transmit block size of 256 bytes. To calculate the total
            // transmit buffer space on such a NIC, its driver multiplies
            // the number of transmit buffers on the NIC by its transmit
            // block size. In our case, the transmit block size is
            // identical to its maximum packet size.

            __fallthrough;

        case OID_GEN_RECEIVE_BLOCK_SIZE:
            //
            // The OID_GEN_RECEIVE_BLOCK_SIZE OID specifies the amount of
            // storage, in bytes, that a single packet occupies in the receive
            // buffer space of the NIC.
            //

            ulInfo = (ULONG) HW_MAX_FRAME_SIZE;
            pInfo = &ulInfo;
            break;

        case OID_GEN_TRANSMIT_BUFFER_SPACE:
            //
            // Specify the amount of memory, in bytes, on the NIC that
            // is available for buffering transmit data. A protocol can
            // use this OID as a guide for sizing the amount of transmit
            // data per send.
            //

            ulInfo = HW_MAX_FRAME_SIZE * Adapter->ulMaxBusySends;
            pInfo = &ulInfo;
            break;

        case OID_GEN_RECEIVE_BUFFER_SPACE:
            //
            // Specify the amount of memory on the NIC that is available
            // for buffering receive data. A protocol driver can use this
            // OID as a guide for advertising its receive window after it
            // establishes sessions with remote nodes.
            //

            ulInfo = HW_MAX_FRAME_SIZE * Adapter->ulMaxBusyRecvs;
            pInfo = &ulInfo;
            break;

        case OID_GEN_MEDIA_SUPPORTED:
            //
            // Return an array of media that are supported by the miniport.
            // This miniport only supports one medium (Ethernet), so the OID
            // returns identical results to OID_GEN_MEDIA_IN_USE.
            //

            __fallthrough;

        case OID_GEN_MEDIA_IN_USE:
            //
            // Return an array of media that are currently in use by the
            // miniport.  This array should be a subset of the array returned
            // by OID_GEN_MEDIA_SUPPORTED.
            //
            pInfo = &Medium;
            ulInfoLen = sizeof(Medium);
            break;

        case OID_GEN_MAXIMUM_SEND_PACKETS:
            ulInfo = NIC_MAX_BUSY_SENDS;
            pInfo = &ulInfo;
            break;

        case OID_GEN_XMIT_ERROR:
            ulInfo = (ULONG)
                    (Adapter->TxAbortExcessCollisions +
                    Adapter->TxDmaUnderrun +
                    Adapter->TxLostCRS +
                    Adapter->TxLateCollisions+
                    Adapter->TransmitFailuresOther);
            pInfo = &ulInfo;
            break;

        case OID_GEN_RCV_ERROR:
            ulInfo = (ULONG)
                    (Adapter->RxCrcErrors +
                    Adapter->RxAlignmentErrors +
                    Adapter->RxDmaOverrunErrors +
                    Adapter->RxRuntErrors);
            pInfo = &ulInfo;
            break;

        case OID_GEN_RCV_DISCARDS:
            ulInfo = (ULONG)Adapter->RxResourceErrors;
            pInfo = &ulInfo;
            break;

        case OID_GEN_RCV_NO_BUFFER:
            ulInfo = (ULONG)
                    Adapter->RxResourceErrors;
            pInfo = &ulInfo;
            break;

        case OID_GEN_VENDOR_ID:
            //
            // Specify a three-byte IEEE-registered vendor code, followed
            // by a single byte that the vendor assigns to identify a
            // particular NIC. The IEEE code uniquely identifies the vendor
            // and is the same as the three bytes appearing at the beginning
            // of the NIC hardware address. Vendors without an IEEE-registered
            // code should use the value 0xFFFFFF.
            //

            ulInfo = NIC_VENDOR_ID;
            pInfo = &ulInfo;
            break;

        case OID_GEN_VENDOR_DESCRIPTION:
            //
            // Specify a zero-terminated string describing the NIC vendor.
            //
            pInfo = VendorDesc;
            ulInfoLen = sizeof(VendorDesc);
            break;

        case OID_GEN_VENDOR_DRIVER_VERSION:
            //
            // Specify the vendor-assigned version number of the NIC driver.
            // The low-order half of the return value specifies the minor
            // version; the high-order half specifies the major version.
            //

            ulInfo = NIC_VENDOR_DRIVER_VERSION;
            pInfo = &ulInfo;
            break;

        case OID_GEN_DRIVER_VERSION:
            //
            // Specify the NDIS version in use by the NIC driver. The high
            // byte is the major version number; the low byte is the minor
            // version number.
            //
            usInfo = (USHORT) (MP_NDIS_MAJOR_VERSION<<8) + MP_NDIS_MINOR_VERSION;
            pInfo = (PVOID) &usInfo;
            ulInfoLen = sizeof(USHORT);
            break;

#if (NDIS_SUPPORT_NDIS61 && !NDIS_SUPPORT_NDIS620)
        case OID_PNP_CAPABILITIES:
            //
            // This OID is obsolete for NDIS 6.20 drivers 
            //
            // Return the wake-up capabilities of its NIC. If you return
            // NDIS_STATUS_NOT_SUPPORTED, NDIS considers the miniport driver
            // to be not Power management aware and doesn't send any power
            // or wake-up related queries such as
            // OID_PNP_SET_POWER, OID_PNP_QUERY_POWER,
            // OID_PNP_ADD_WAKE_UP_PATTERN, OID_PNP_REMOVE_WAKE_UP_PATTERN,
            // OID_PNP_ENABLE_WAKE_UP.
            //
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;
#endif
            //
            // Following 4 OIDs are for querying Ethernet Operational
            // Characteristics.
            //
        case OID_802_3_PERMANENT_ADDRESS:
            //
            // Return the MAC address of the NIC burnt in the hardware.
            //
            pInfo = Adapter->PermanentAddress;
            ulInfoLen = NIC_MACADDR_SIZE;
            break;

        case OID_802_3_CURRENT_ADDRESS:
            //
            // Return the MAC address the NIC is currently programmed to
            // use. Note that this address could be different from the
            // permananent address as the user can override using
            // registry. Read NdisReadNetworkAddress doc for more info.
            //
            pInfo = Adapter->CurrentAddress;
            ulInfoLen = NIC_MACADDR_SIZE;
            break;

        case OID_802_3_MAXIMUM_LIST_SIZE:
            //
            // The maximum number of multicast addresses the NIC driver
            // can manage. This list is global for all protocols bound
            // to (or above) the NIC. Consequently, a protocol can receive
            // NDIS_STATUS_MULTICAST_FULL from the NIC driver when
            // attempting to set the multicast address list, even if
            // the number of elements in the given list is less than
            // the number originally returned for this query.
            //

            ulInfo = NIC_MAX_MCAST_LIST;
            pInfo = &ulInfo;
            break;

            //
            // Following list  consists of both general and Ethernet
            // specific statistical OIDs.
            //

        case OID_GEN_XMIT_OK:
            ulInfo64 = Adapter->FramesTxBroadcast
                    + Adapter->FramesTxMulticast
                    + Adapter->FramesTxDirected;
            pInfo = &ulInfo64;
            if (Query->InformationBufferLength >= sizeof(ULONG64) ||
                Query->InformationBufferLength == 0)
            {
                ulInfoLen = sizeof(ULONG64);
            }
            else
            {
                ulInfoLen = sizeof(ULONG);
            }
            // We should always report that only 8 bytes are required to keep ndistest happy
            Query->BytesNeeded =  sizeof(ULONG64);
            break;

        case OID_GEN_RCV_OK:
            ulInfo64 = Adapter->FramesRxBroadcast
                    + Adapter->FramesRxMulticast
                    + Adapter->FramesRxDirected;
            pInfo = &ulInfo64;
            if (Query->InformationBufferLength >= sizeof(ULONG64) ||
                Query->InformationBufferLength == 0)
            {
                ulInfoLen = sizeof(ULONG64);
            }
            else
            {
                ulInfoLen = sizeof(ULONG);
            }
            // We should always report that only 8 bytes are required to keep ndistest happy
            Query->BytesNeeded =  sizeof(ULONG64);
            break;

        case OID_GEN_STATISTICS:

            if (Query->InformationBufferLength < sizeof(NDIS_STATISTICS_INFO))
            {
                Status = NDIS_STATUS_INVALID_LENGTH;
                Query->BytesNeeded = sizeof(NDIS_STATISTICS_INFO);
                break;
            }
            else
            {
                PNDIS_STATISTICS_INFO Statistics = (PNDIS_STATISTICS_INFO)Query->InformationBuffer;

                {C_ASSERT(sizeof(NDIS_STATISTICS_INFO) >= NDIS_SIZEOF_STATISTICS_INFO_REVISION_1);}
                Statistics->Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
                Statistics->Header.Size = NDIS_SIZEOF_STATISTICS_INFO_REVISION_1;
                Statistics->Header.Revision = NDIS_STATISTICS_INFO_REVISION_1;

                Statistics->SupportedStatistics = NIC_SUPPORTED_STATISTICS;

                /* Bytes in */
                Statistics->ifHCInOctets =
                        Adapter->BytesRxDirected +
                        Adapter->BytesRxMulticast +
                        Adapter->BytesRxBroadcast;

                Statistics->ifHCInUcastOctets =
                        Adapter->BytesRxDirected;

                Statistics->ifHCInMulticastOctets =
                        Adapter->BytesRxMulticast;

                Statistics->ifHCInBroadcastOctets =
                        Adapter->BytesRxBroadcast;

                /* Packets in */
                Statistics->ifHCInUcastPkts =
                        Adapter->FramesRxDirected;

                Statistics->ifHCInMulticastPkts =
                        Adapter->FramesRxMulticast;

                Statistics->ifHCInBroadcastPkts =
                        Adapter->FramesRxBroadcast;

                /* Errors in */
                Statistics->ifInErrors =
                        Adapter->RxCrcErrors +
                        Adapter->RxAlignmentErrors +
                        Adapter->RxDmaOverrunErrors +
                        Adapter->RxRuntErrors;

                Statistics->ifInDiscards =
                        Adapter->RxResourceErrors;


                /* Bytes out */
                Statistics->ifHCOutOctets =
                        Adapter->BytesTxDirected +
                        Adapter->BytesTxMulticast +
                        Adapter->BytesTxBroadcast;

                Statistics->ifHCOutUcastOctets =
                        Adapter->BytesTxDirected;

                Statistics->ifHCOutMulticastOctets =
                        Adapter->BytesTxMulticast;

                Statistics->ifHCOutBroadcastOctets =
                        Adapter->BytesTxBroadcast;

                /* Packets out */
                Statistics->ifHCOutUcastPkts =
                        Adapter->FramesTxDirected;

                Statistics->ifHCOutMulticastPkts =
                        Adapter->FramesTxMulticast;

                Statistics->ifHCOutBroadcastPkts =
                        Adapter->FramesTxBroadcast;

                /* Errors out */
                Statistics->ifOutErrors =
                        Adapter->TxAbortExcessCollisions +
                        Adapter->TxDmaUnderrun +
                        Adapter->TxLostCRS +
                        Adapter->TxLateCollisions+
                        Adapter->TransmitFailuresOther;

                Statistics->ifOutDiscards =
                        0ULL;

                ulInfoLen = NDIS_SIZEOF_STATISTICS_INFO_REVISION_1;
            }

            break;

        case OID_GEN_TRANSMIT_QUEUE_LENGTH:

            ulInfo = Adapter->nBusySend;
            pInfo = &ulInfo;
            break;

        case OID_802_3_RCV_ERROR_ALIGNMENT:

            ulInfo = Adapter->RxAlignmentErrors;
            pInfo = &ulInfo;
            break;

        case OID_802_3_XMIT_ONE_COLLISION:

            ulInfo = Adapter->OneRetry;
            pInfo = &ulInfo;
            break;

        case OID_802_3_XMIT_MORE_COLLISIONS:

            ulInfo = Adapter->MoreThanOneRetry;
            pInfo = &ulInfo;
            break;

        case OID_802_3_XMIT_DEFERRED:

            ulInfo = Adapter->TxOKButDeferred;
            pInfo = &ulInfo;
            break;

        case OID_802_3_XMIT_MAX_COLLISIONS:

            ulInfo = Adapter->TxAbortExcessCollisions;
            pInfo = &ulInfo;
            break;

        case OID_802_3_RCV_OVERRUN:

            ulInfo = Adapter->RxDmaOverrunErrors;
            pInfo = &ulInfo;
            break;

        case OID_802_3_XMIT_UNDERRUN:

            ulInfo = Adapter->TxDmaUnderrun;
            pInfo = &ulInfo;
            break;

        case OID_802_3_XMIT_HEARTBEAT_FAILURE:

            ulInfo = Adapter->TxLostCRS;
            pInfo = &ulInfo;
            break;

        case OID_802_3_XMIT_TIMES_CRS_LOST:

            ulInfo = Adapter->TxLostCRS;
            pInfo = &ulInfo;
            break;

        case OID_802_3_XMIT_LATE_COLLISIONS:

            ulInfo = Adapter->TxLateCollisions;
            pInfo = &ulInfo;
            break;

        case OID_GEN_INTERRUPT_MODERATION:
        {
            PNDIS_INTERRUPT_MODERATION_PARAMETERS Moderation = (PNDIS_INTERRUPT_MODERATION_PARAMETERS)Query->InformationBuffer;
            Moderation->Header.Type = NDIS_OBJECT_TYPE_DEFAULT; 
            Moderation->Header.Revision = NDIS_INTERRUPT_MODERATION_PARAMETERS_REVISION_1;
            Moderation->Header.Size = NDIS_SIZEOF_INTERRUPT_MODERATION_PARAMETERS_REVISION_1;
            Moderation->Flags = 0;
            Moderation->InterruptModeration = NdisInterruptModerationNotSupported;
            ulInfoLen = NDIS_SIZEOF_INTERRUPT_MODERATION_PARAMETERS_REVISION_1;
        }
            break;

        case OID_PNP_QUERY_POWER:
            // simply succeed this.
            break;

        default:
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;
    }

    if (Status == NDIS_STATUS_SUCCESS)
    {
        ASSERT(ulInfoLen > 0);

        if (ulInfoLen <= Query->InformationBufferLength)
        {
            if(pInfo)
            {
                // Copy result into InformationBuffer
                NdisMoveMemory(Query->InformationBuffer, pInfo, ulInfoLen);
            }
            Query->BytesWritten = ulInfoLen;
        }
        else
        {
            // too short
            Query->BytesNeeded = ulInfoLen;
            Status = NDIS_STATUS_BUFFER_TOO_SHORT;
        }
    }


    DEBUGP(MP_LOUD, "[%p] <--- MPQueryInformation Status = 0x%08x\n", Adapter, Status);
    return Status;
}


NDIS_STATUS
MPSetInformation(
    _In_  PMP_ADAPTER         Adapter,
    _In_  PNDIS_OID_REQUEST   NdisSetRequest)
/*++

Routine Description:

    Helper function to perform a set OID request

Arguments:

    Adapter         -
    NdisSetRequest  - The OID to set

Return Value:

    NDIS_STATUS

--*/
{
    NDIS_STATUS             Status = NDIS_STATUS_SUCCESS;
    struct _SET            *Set = &NdisSetRequest->DATA.SET_INFORMATION;

    PAGED_CODE();

    DEBUGP(MP_LOUD, "[%p] ---> MPSetInformation ", Adapter);
    DbgPrintOidName(Set->Oid);


    switch(Set->Oid)
    {
        case OID_802_3_MULTICAST_LIST:
            //
            // Set the multicast address list on the NIC for packet reception.
            // The NIC driver can set a limit on the number of multicast
            // addresses bound protocol drivers can enable simultaneously.
            // NDIS returns NDIS_STATUS_MULTICAST_FULL if a protocol driver
            // exceeds this limit or if it specifies an invalid multicast
            // address.
            //
            Status = NICSetMulticastList(Adapter, NdisSetRequest);

            break;

        case OID_GEN_CURRENT_PACKET_FILTER:
            //
            // Program the hardware to indicate the packets
            // of certain filter types.
            //
            if(Set->InformationBufferLength != sizeof(ULONG))
            {
                Set->BytesNeeded = sizeof(ULONG);
                Status = NDIS_STATUS_INVALID_LENGTH;
                break;
            }

            Set->BytesRead = Set->InformationBufferLength;

            Status = NICSetPacketFilter(
                            Adapter,
                            *((PULONG)Set->InformationBuffer));

            break;

        case OID_GEN_CURRENT_LOOKAHEAD:
            //
            // A protocol driver can set a suggested value for the number
            // of bytes to be used in its binding; however, the underlying
            // NIC driver is never required to limit its indications to
            // the value set.
            //
            if (Set->InformationBufferLength != sizeof(ULONG))
            {
                Set->BytesNeeded = sizeof(ULONG);
                Status = NDIS_STATUS_INVALID_LENGTH;
                break;
            }
            Adapter->ulLookahead = *(PULONG)Set->InformationBuffer;

            Set->BytesRead = sizeof(ULONG);
            Status = NDIS_STATUS_SUCCESS;
            break;

#if (NDIS_SUPPORT_NDIS620)

        case OID_RECEIVE_FILTER_FREE_QUEUE:
            //
            // Free the requested receive queue. 
            //
            Status = NICFreeRxQueue(
                            Adapter, 
                            NdisSetRequest);
            break;

        case OID_RECEIVE_FILTER_CLEAR_FILTER:
            //
            // Remove the requested filter on the requested receive queue.
            //
            Status = NICClearRxFilter(
                            Adapter,
                            NdisSetRequest);
            break;

        case OID_RECEIVE_FILTER_QUEUE_PARAMETERS:
            //
            // Update the queue information.
            //
            Status = NICUpdateRxQueue(
                            Adapter,
                            NdisSetRequest);             
             break;
#endif

        case OID_PNP_SET_POWER:
            //
            // Update power state 
            //
            Status = MPSetPower(
                            Adapter,
                            NdisSetRequest);             
            break;

#if (NDIS_SUPPORT_NDIS680)
        case OID_GEN_RECEIVE_SCALE_PARAMETERS_V2:
            //
            // Update RSSv2 parameters
            //
            Status = MPSetRSSv2Parameters(Adapter, NdisSetRequest);
            break;
#endif

#if (NDIS_SUPPORT_NDIS620)
        case OID_PM_ADD_WOL_PATTERN:
        case OID_PM_REMOVE_WOL_PATTERN:
        case OID_PM_ADD_PROTOCOL_OFFLOAD:
        case OID_PM_REMOVE_PROTOCOL_OFFLOAD:
        case OID_PM_PARAMETERS:

#else if (NDIS_SUPPORT_NDIS61)
        case OID_PNP_ADD_WAKE_UP_PATTERN:
        case OID_PNP_REMOVE_WAKE_UP_PATTERN:
        case OID_PNP_ENABLE_WAKE_UP:
#endif
            ASSERT(!"NIC does not support wake on LAN OIDs"); 
        default:
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;
    }

    if(Status == NDIS_STATUS_SUCCESS)
    {
        Set->BytesRead = Set->InformationBufferLength;
    }


    DEBUGP(MP_LOUD, "[%p] <--- MPSetInformation Status = 0x%08x\n", Adapter, Status);

    return Status;
}


NDIS_STATUS
MPMethodRequest(
    _In_  PMP_ADAPTER             Adapter,
    _In_  PNDIS_OID_REQUEST       NdisRequest)
/*++
Routine Description:

    Helper function to perform a WMI OID request

Arguments:

    Adapter      -
    NdisRequest  - THe WMI OID request

Return Value:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_NOT_SUPPORTED

--*/
{
    NDIS_STATUS      Status = NDIS_STATUS_SUCCESS;
    NDIS_OID         Oid = NdisRequest->DATA.METHOD_INFORMATION.Oid;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(Adapter);
    UNREFERENCED_PARAMETER(NdisRequest);

    DEBUGP(MP_LOUD, "[%p] ---> MPMethodRequest ", Adapter);
    DbgPrintOidName(Oid);


    switch (Oid)
    {
    
#if (NDIS_SUPPORT_NDIS620)
        case OID_RECEIVE_FILTER_ALLOCATE_QUEUE:
            //
            // Allocate the requested receive queue.
            //
            Status = NICAllocateRxQueue(
                            Adapter, 
                            NdisRequest);

            break;

        case OID_RECEIVE_FILTER_QUEUE_ALLOCATION_COMPLETE:
            //
            // Complete any remaining allocation for receive queues. 
            //
            Status = NICCompleteAllocationRxQueue(
                            Adapter, 
                            NdisRequest);

            break;

        case OID_RECEIVE_FILTER_SET_FILTER:
            //
            // Add a filter to the requested queue.
            //
            Status = NICSetRxFilter(
                            Adapter,
                            NdisRequest);
            break;

#endif

#if (NDIS_SUPPORT_NDIS630)
        case OID_QOS_PARAMETERS:
            //
            // Set NDIS QoS configuration parameters.
            //
            Status = NICSetQOSParameters(
                            Adapter,
                            NdisRequest);
            break;

#endif

        default:
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;
    }


    DEBUGP(MP_LOUD, "[%p] <--- MPMethodRequest Status = 0x%08x\n", Adapter, Status);
    return Status;
}


#if (NDIS_SUPPORT_NDIS680)
NDIS_STATUS
MPSynchronousMethodRequest(
    _In_  PMP_ADAPTER             Adapter,
    _In_  PNDIS_OID_REQUEST       NdisRequest)
/*++
Routine Description:

    Helper function to perform a synchronous method OID request

Arguments:

    Adapter      -
    NdisRequest  - THe synchronous method OID request

Return Value:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_NOT_SUPPORTED

--*/
{
    NDIS_STATUS      Status;
    NDIS_OID         Oid;

    DEBUGP(MP_LOUD, "[%p] ---> MPSynchronousMethodRequest ", Adapter);

    Oid = NdisRequest->DATA.METHOD_INFORMATION.Oid;
    DbgPrintOidName(Oid);

    switch (Oid)
    {
        case OID_GEN_RSS_SET_INDIRECTION_TABLE_ENTRIES:
            //
            // Handle RSSv2 traffic steering OID.
            //
            Status = MPSetRSSv2IndirectionTableEntries(Adapter, NdisRequest);
            break;

        default:
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;
    }


    DEBUGP(MP_LOUD, "[%p] <--- MPSynchronousMethodRequest Status = 0x%08x\n", Adapter, Status);
    return Status;
}
#endif


NDIS_STATUS
NICSetPacketFilter(
    _In_  PMP_ADAPTER Adapter,
    _In_  ULONG PacketFilter)
/*++
Routine Description:

    This routine will set up the adapter so that it accepts packets
    that match the specified packet filter.

Arguments:

    Adapter      - pointer to adapter block
    PacketFilter - the new packet filter

Return Value:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_NOT_SUPPORTED

--*/

{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

    PAGED_CODE();

    DEBUGP(MP_TRACE, "[%p] ---> NICSetPacketFilter\n", Adapter);


    // any bits not supported?
    if (PacketFilter & ~(NIC_SUPPORTED_FILTERS))
    {
        DEBUGP(MP_WARNING, "[%p] Unsupported packet filter: 0x%08x\n", Adapter, PacketFilter);
        return NDIS_STATUS_NOT_SUPPORTED;
    }

    // any filtering changes?
    if (PacketFilter != Adapter->PacketFilter)
    {
        //
        // Change the filtering modes on hardware
        //


        // Save the new packet filter value
        Adapter->PacketFilter = PacketFilter;
    }


    DEBUGP(MP_TRACE, "[%p] <--- NICSetPacketFilter Status = 0x%08x\n", Adapter, Status);

    return Status;
}


NDIS_STATUS
NICSetMulticastList(
    _In_  PMP_ADAPTER        Adapter,
    _In_  PNDIS_OID_REQUEST  NdisSetRequest)
/*++
Routine Description:

    This routine will set up the adapter for a specified multicast
    address list.

Arguments:

    Adapter         - Pointer to adapter block
    NdisSetRequest  - The OID request with the new multicast list

Return Value:

    NDIS_STATUS

--*/
{
    NDIS_STATUS   Status = NDIS_STATUS_SUCCESS;
    struct _SET  *Set = &NdisSetRequest->DATA.SET_INFORMATION;

#if DBG
    ULONG                  index;
#endif

    PAGED_CODE();

    DEBUGP(MP_TRACE, "[%p] ---> NICSetMulticastList\n", Adapter);


    //
    // Initialize.
    //
    Set->BytesNeeded = NIC_MACADDR_SIZE;
    Set->BytesRead = Set->InformationBufferLength;

    do
    {
        if (Set->InformationBufferLength % NIC_MACADDR_SIZE)
        {
            Status = NDIS_STATUS_INVALID_LENGTH;
            break;
        }

        if (Set->InformationBufferLength > (NIC_MAX_MCAST_LIST * NIC_MACADDR_SIZE))
        {
            Status = NDIS_STATUS_MULTICAST_FULL;
            Set->BytesNeeded = NIC_MAX_MCAST_LIST * NIC_MACADDR_SIZE;
            break;
        }

        //
        // Protect the list update with a lock if it can be updated by
        // another thread simultaneously.
        //

        NdisZeroMemory(Adapter->MCList,
                       NIC_MAX_MCAST_LIST * NIC_MACADDR_SIZE);

        NdisMoveMemory(Adapter->MCList,
                       Set->InformationBuffer,
                       Set->InformationBufferLength);

        Adapter->ulMCListSize = Set->InformationBufferLength / NIC_MACADDR_SIZE;

#if DBG
        // display the multicast list
        for(index = 0; index < Adapter->ulMCListSize; index++)
        {
            DEBUGP(MP_LOUD, "[%p] MC(%d) = ", Adapter, index);
            DbgPrintAddress(Adapter->MCList[index]);
        }
#endif
    }
    while (FALSE);


    //
    // Program the hardware to add suport for these muticast addresses
    //


    DEBUGP(MP_TRACE, "[%p] <--- NICSetMulticastList Status 0x%08x\n", Adapter, Status);

    return Status;
}

#if (NDIS_SUPPORT_NDIS620)

#define VERIFY_OID_SET(_Request, _MinRevision, _MinLength)\
    _Request->DATA.SET_INFORMATION.BytesNeeded = _MinLength;\
    if(_Request->Header.Revision < _MinRevision)\
    { \
        Status = NDIS_STATUS_NOT_SUPPORTED;\
        break;\
    }\
    if(_Request->DATA.SET_INFORMATION.InformationBufferLength < _MinLength)\
    {\
        Status = NDIS_STATUS_INVALID_LENGTH;\
        break;\
    }\

#define VERIFY_OID_METHOD(_Request, _MinRevision, _MinLength)\
    _Request->DATA.METHOD_INFORMATION.BytesNeeded = _MinLength;\
    if(_Request->Header.Revision < _MinRevision)\
    { \
        Status = NDIS_STATUS_NOT_SUPPORTED;\
        break;\
    }\
    if(_Request->DATA.METHOD_INFORMATION.InputBufferLength < _MinLength)\
    {\
        Status = NDIS_STATUS_INVALID_LENGTH;\
        break;\
    }\


NDIS_STATUS
NICAllocateRxQueue(
    _In_ PMP_ADAPTER        Adapter,
    _In_ PNDIS_OID_REQUEST   NdisMethodRequest)
/*++
Routine Description:

    This routine will allocate a receive queue according to the passed in allocation request. It verifies that the request
    is well formed, then passes the request to underlying queue management code. 

Arguments:

    Adapter         - Pointer to adapter block
    NdisSetRequest  - The OID data for the request

Return Value:

    NDIS_STATUS

--*/
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    struct _METHOD *Method = &NdisMethodRequest->DATA.METHOD_INFORMATION;
    PNDIS_RECEIVE_QUEUE_PARAMETERS QueueParams = (PNDIS_RECEIVE_QUEUE_PARAMETERS)Method->InformationBuffer;
        
    PAGED_CODE();

    DEBUGP(MP_TRACE, "[%p] ---> NICAllocateRxQueue\n", Adapter);
    
    do
    {
        //
        // Verify that the request matches our requirements
        //
        VERIFY_OID_METHOD(NdisMethodRequest, 
                          NDIS_RECEIVE_QUEUE_PARAMETERS_REVISION_1, 
                          NDIS_SIZEOF_RECEIVE_QUEUE_PARAMETERS_REVISION_1);

        //
        // Request is well formed, set bytes read 
        //
        Method->BytesRead = NDIS_SIZEOF_RECEIVE_QUEUE_PARAMETERS_REVISION_1;

        //
        // Should not ask to allocate default queue, or non VMQ type
        //
        if(QueueParams->QueueId==NDIS_DEFAULT_RECEIVE_QUEUE_ID
            ||
            QueueParams->QueueType != NdisReceiveQueueTypeVMQueue)
        {
            DEBUGP(MP_ERROR, "[%p] Unsupported QueueId (%i) or QueueType (%i) for allocation.\n", Adapter, QueueParams->QueueId, QueueParams->QueueType);
            Status = NDIS_STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Ready to allocate
        //
        Status = AllocateRxQueue(Adapter, QueueParams);

    } while(FALSE);

    DEBUGP(MP_TRACE, "[%p] <--- NICAllocateRxQueuee Status 0x%08x\n", Adapter, Status);

    return Status;
}


NDIS_STATUS
NICCompleteAllocationRxQueue(
    _In_ PMP_ADAPTER        Adapter,
    _In_ PNDIS_OID_REQUEST   NdisMethodRequest)
/*++
Routine Description:

    This routine will complete any remaining queue allocation, including shared memory. It verifies that the request
    is well formed, then passes the request to underlying queue management code. 

Arguments:

    Adapter         - Pointer to adapter block
    NdisSetRequest  - The OID data for the request

Return Value:

    NDIS_STATUS

--*/    
{

    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    struct _METHOD *Method = &NdisMethodRequest->DATA.METHOD_INFORMATION;
    PNDIS_RECEIVE_QUEUE_ALLOCATION_COMPLETE_ARRAY  CompleteArray = (PNDIS_RECEIVE_QUEUE_ALLOCATION_COMPLETE_ARRAY)Method->InformationBuffer;

    PAGED_CODE();

    DEBUGP(MP_TRACE, "[%p] ---> NICCompleteAllocationRxQueue\n", Adapter);
    
    do
    {

        //
        // Verify that the request matches our requirements
        //
        VERIFY_OID_METHOD(NdisMethodRequest, 
                          NDIS_RECEIVE_QUEUE_ALLOCATION_COMPLETE_ARRAY_REVISION_1, 
                          NDIS_SIZEOF_RECEIVE_QUEUE_ALLOCATION_COMPLETE_ARRAY_REVISION_1);

        //
        // Request is well formed, set bytes read 
        //
        Method->BytesRead = NDIS_SIZEOF_RECEIVE_QUEUE_ALLOCATION_COMPLETE_ARRAY_REVISION_1+ 
                             (CompleteArray->NumElements * CompleteArray->ElementSize);

        //
        // Ready to complete allocation
        //
        Status = CompleteAllocationRxQueue(Adapter, CompleteArray);

    }while(FALSE);
    
    DEBUGP(MP_TRACE, "[%p] <--- NICCompleteAllocationRxQueue Status 0x%08x\n", Adapter, Status);
    
    return Status;

}

NDIS_STATUS
NICFreeRxQueue(
    _In_ PMP_ADAPTER        Adapter,
    _In_ PNDIS_OID_REQUEST   NdisSetRequest)
/*++
Routine Description:

    This routine will handle the passed in queue free request. It verifies that the request
    is well formed, then passes the request to underlying queue management code. 

Arguments:

    Adapter         - Pointer to adapter block
    NdisSetRequest  - The OID data for the request

Return Value:

    NDIS_STATUS

--*/
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    struct _SET  *Set = &NdisSetRequest->DATA.SET_INFORMATION;
    PNDIS_RECEIVE_QUEUE_FREE_PARAMETERS QueueFreeParams = (PNDIS_RECEIVE_QUEUE_FREE_PARAMETERS)Set->InformationBuffer;

    PAGED_CODE();

    DEBUGP(MP_TRACE, "[%p] ---> NICFreeRxQueue\n", Adapter);
    
    do
    {
        //
        // Verify that the request matches our requirements
        //
        VERIFY_OID_SET(NdisSetRequest, 
                       NDIS_RECEIVE_QUEUE_FREE_PARAMETERS_REVISION_1, 
                       NDIS_SIZEOF_RECEIVE_QUEUE_FREE_PARAMETERS_REVISION_1);

        //
        // Request is well formed, set bytes read 
        //
        Set->BytesRead = NDIS_SIZEOF_RECEIVE_QUEUE_FREE_PARAMETERS_REVISION_1;
    
        //
        // Default queue cannot be freed
        //
        if(QueueFreeParams->QueueId==NDIS_DEFAULT_RECEIVE_QUEUE_ID)
        {
            DEBUGP(MP_ERROR, "[%p] Received request to free default queue.\n", Adapter);
            Status = NDIS_STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Ready to attempt a free. 
        //
        Status = FreeRxQueue(Adapter, QueueFreeParams, NdisSetRequest);

    }while(FALSE);
    
    DEBUGP(MP_TRACE, "[%p] <--- NICFreeRxQueuee Status 0x%08x\n", Adapter, Status);
    
    return Status;
}

static
NDIS_STATUS
NICSetRxFilter(
    _In_ PMP_ADAPTER        Adapter,
    _In_ PNDIS_OID_REQUEST  NdisMethodRequest)
/*++
Routine Description:

    This routine will handle the passed filter set request. It verifies that the request
    is well formed, then passes the request to underlying filter management code. 

Arguments:

    Adapter         - Pointer to adapter block
    NdisSetRequest  - The OID data for the request

Return Value:

    NDIS_STATUS

--*/    
{

    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    struct _METHOD *Method = &NdisMethodRequest->DATA.METHOD_INFORMATION;
    PNDIS_RECEIVE_FILTER_PARAMETERS FilterParams = (PNDIS_RECEIVE_FILTER_PARAMETERS)Method->InformationBuffer;

    PAGED_CODE();

    do
    {
        //
        // Verify that the request matches our requirements
        //
        VERIFY_OID_METHOD(NdisMethodRequest, 
                          NDIS_RECEIVE_FILTER_PARAMETERS_REVISION_1, 
                          NDIS_SIZEOF_RECEIVE_FILTER_PARAMETERS_REVISION_1);

        //
        // Request is well formed, set bytes read 
        //
        Method->BytesRead = NDIS_SIZEOF_RECEIVE_FILTER_PARAMETERS_REVISION_1;

        //
        // Ready to set Filter
        //
        Status = SetRxFilter(Adapter, FilterParams);
    
    }while(FALSE);

    return Status;
}

static
NDIS_STATUS
NICClearRxFilter(
    _In_ PMP_ADAPTER        Adapter,
    _In_ PNDIS_OID_REQUEST   NdisSetRequest)
/*++
Routine Description:

    This routine will handle the passed filter clear request. It verifies that the request
    is well formed, then passes the request to underlying filter management code. 

Arguments:

    Adapter         - Pointer to adapter block
    NdisSetRequest  - The OID data for the request

Return Value:

    NDIS_STATUS

--*/      
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    struct _SET  *Set = &NdisSetRequest->DATA.SET_INFORMATION;
    PNDIS_RECEIVE_FILTER_CLEAR_PARAMETERS FilterParams = (PNDIS_RECEIVE_FILTER_CLEAR_PARAMETERS)Set->InformationBuffer;

    PAGED_CODE();

    do
    {

        //
        // Verify that the request matches our requirements
        //
        VERIFY_OID_SET(NdisSetRequest, 
                       NDIS_RECEIVE_FILTER_CLEAR_PARAMETERS_REVISION_1, 
                       NDIS_SIZEOF_RECEIVE_FILTER_CLEAR_PARAMETERS_REVISION_1);

        //
        // Request is well formed, set bytes read 
        //
        Set->BytesRead = NDIS_SIZEOF_RECEIVE_FILTER_CLEAR_PARAMETERS_REVISION_1;
    
        //
        // Ready to clear the filter
        //
        Status = ClearRxFilter(Adapter, FilterParams);

    }while(FALSE);

    return Status;
}

static
NDIS_STATUS
NICUpdateRxQueue(
    _In_ PMP_ADAPTER        Adapter,
    _In_ PNDIS_OID_REQUEST  NdisSetRequest)
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    struct _SET  *Set = &NdisSetRequest->DATA.SET_INFORMATION;
    PNDIS_RECEIVE_QUEUE_PARAMETERS FilterParams = (PNDIS_RECEIVE_QUEUE_PARAMETERS)Set->InformationBuffer;

    PAGED_CODE();

    do
    {

        //
        // Verify that the request matches our requirements
        //
        VERIFY_OID_SET(NdisSetRequest, 
                       NDIS_RECEIVE_QUEUE_PARAMETERS_REVISION_1, 
                       NDIS_SIZEOF_RECEIVE_QUEUE_PARAMETERS_REVISION_1);

        //
        // Request is well formed, set bytes read 
        //
        Set->BytesRead = NDIS_SIZEOF_RECEIVE_FILTER_CLEAR_PARAMETERS_REVISION_1;
    
        //
        // Ready to clear the filter
        //
        Status = UpdateRxQueue(Adapter, FilterParams);

    } while(FALSE);

    return Status;

}

#endif

#if (NDIS_SUPPORT_NDIS630)

_IRQL_requires_(PASSIVE_LEVEL)
NDIS_STATUS
NICSetQOSParameters(
    _In_ PMP_ADAPTER        Adapter,
    _In_ PNDIS_OID_REQUEST  NdisMethodRequest)
/*++
Routine Description:

    This routine will build a classification table according to the request. It verifies that
    the request is well formed, then passes the request to the underlying classification
    management code. On the other hand, hardware-oriented parameters such as PFC and ETS are
    validated but not enforceable in a software implementation.

Arguments:

    Adapter         - Pointer to adapter block
    NdisSetRequest  - The OID data for the request

Return Value:

    NDIS_STATUS

--*/
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    struct _METHOD *Method = &NdisMethodRequest->DATA.METHOD_INFORMATION;
    PNDIS_QOS_PARAMETERS Params = (PNDIS_QOS_PARAMETERS)Method->InformationBuffer;

    PAGED_CODE();

    DEBUGP(MP_TRACE, "[%p] ---> NICSetQOSParameters\n", Adapter);

    do
    {
        //
        // Verify that the request matches our requirements.
        //
        VERIFY_OID_METHOD(NdisMethodRequest,
                          NDIS_QOS_PARAMETERS_REVISION_1,
                          NDIS_SIZEOF_QOS_PARAMETERS_REVISION_1);

        //
        // Request is well formed, set bytes read.
        //
        Method->BytesRead = NDIS_SIZEOF_QOS_PARAMETERS_REVISION_1 +
                            Params->NumClassificationElements * Params->ClassificationElementSize;

        Status = SetQOSParameters(Adapter, Params);
        if (Status != NDIS_STATUS_SUCCESS)
        {
            break;
        }

        //
        // Set bytes written for in-place write, basically same as bytes read.
        //
        Method->BytesWritten = Method->BytesRead;
    } while(FALSE);

    DEBUGP(MP_TRACE, "[%p] <--- NICSetQOSParameters Status 0x%08x\n", Adapter, Status);

    return Status;
}

#endif

static
NDIS_STATUS
MPSetPower(
    _In_ PMP_ADAPTER        Adapter,
    _In_ PNDIS_OID_REQUEST  NdisSetRequest)
/*++
Routine Description:

    This routine handles OID_PNP_SET_POWER request. 

Arguments:

    Adapter         - Pointer to adapter block
    NdisSetRequest  - The OID data for the request

Return Value:

    NDIS_STATUS   

--*/      
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    struct _SET *Set = &NdisSetRequest->DATA.SET_INFORMATION;
    NDIS_DEVICE_POWER_STATE     PowerState;

    PAGED_CODE();

    if (Set->InformationBufferLength < sizeof(NDIS_DEVICE_POWER_STATE))
    {
        return NDIS_STATUS_INVALID_LENGTH;
    }

    PowerState = *(PNDIS_DEVICE_POWER_STATE UNALIGNED)Set->InformationBuffer;
    Set->BytesRead = sizeof(NDIS_DEVICE_POWER_STATE);

    if(PowerState < NdisDeviceStateD0  ||
       PowerState > NdisDeviceStateD3)
    {
        return NDIS_STATUS_INVALID_DATA;
    }


    if (PowerState == NdisDeviceStateD0)
    {
        Status = MPSetPowerD0(Adapter);
    }
    else
    {
        Status = MPSetPowerLow(Adapter, PowerState);
    }

    return Status;
}

static
NDIS_STATUS
MPSetPowerD0(
    _In_ PMP_ADAPTER        Adapter)
/*++
Routine Description:

    NIC power has been restored to the working power state (D0).
    Prepare the NIC for normal operation:
        - Restore hardware context (packet filters, multicast addresses, MAC address, etc.)
        - Enable interrupts and the NIC's DMA engine.

Arguments:

    Adapter     - Pointer to adapter block

Return Value:

    NDIS_STATUS   

--*/      
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

    PAGED_CODE();

    Adapter->CurrentPowerState = NdisDeviceStateD0;
    MP_CLEAR_FLAG(Adapter, fMP_ADAPTER_LOW_POWER);

    NICStartTheDatapath(Adapter);

    return Status;
}

static
NDIS_STATUS
MPSetPowerLow(
    _In_ PMP_ADAPTER                Adapter,
    _In_ NDIS_DEVICE_POWER_STATE    PowerState)
/*++
Routine Description:

    The NIC is about to be transitioned to a low power state. 
    Prepare the NIC for the sleeping state:
        - Disable interrupts and the NIC's DMA engine, cancel timers.  
        - Save any hardware context that the NIC cannot preserve in 
          a sleeping state (packet filters, multicast addresses, 
          the current MAC address, etc.)
    A miniport driver cannot access the NIC hardware after 
    the NIC has been set to the D3 state by the bus driver.

    Miniport drivers NDIS v6.30 and above 
        Do NOT wait for NDIS to return the ownership of all 
        NBLs from outstanding receive indications
        Retain ownership of all the receive descriptors and 
        packet buffers previously owned by the hardware.

Arguments:

    Adapter         - Pointer to adapter block
    PowerState      - New power state

Return Value:

    NDIS_STATUS   

--*/      
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    LONG        nSendWaitCount = 0;

    PAGED_CODE();

    MP_SET_FLAG(Adapter, fMP_ADAPTER_LOW_POWER);
    Adapter->CurrentPowerState = PowerState;


#if (NDIS_SUPPORT_NDIS630)

    //
    // Miniport drivers NDIS v6.30 and above are not 
    // necessarily paused prior the low power transition 
    //

    //
    // Prevent future sends and receives on the data path
    //
    NICStopTheDatapath(Adapter);

    //
    // Wait for outstanding sends
    // Do NOT wait for outstanding receives 
    //
    while(Adapter->nBusySend)
    {
        if (++nSendWaitCount % 100)
        {
            DEBUGP(MP_ERROR, "[%p] MPSetPowerLow timed out!\n", Adapter);
            ASSERT(FALSE);
        }

        DEBUGP(MP_INFO, "[%p] MPSetPowerLow - waiting ...\n", Adapter);
        NdisMSleep(1000);
    }

#else

    UNREFERENCED_PARAMETER(nSendWaitCount);

    //
    // Miniport drivers NDIS v6.20 and below are 
    // paused prior the low power transition 
    //
    ASSERT(MP_TEST_FLAG(Adapter, fMP_ADAPTER_PAUSED));
    ASSERT(!NICIsBusy(Adapter));

#endif

    return Status;
}


#if (NDIS_SUPPORT_NDIS680)
static
NDIS_STATUS
MPSetRSSv2Parameters(
    _In_ PMP_ADAPTER        Adapter,
    _In_ PNDIS_OID_REQUEST  NdisSetRequest)
/*++
Routine Description:

    This routine handles OID_GEN_RECEIVE_SCALE_PARAMETERS_V2 set request. 

Arguments:

    Adapter         - Pointer to adapter block
    NdisSetRequest  - The OID data for the request

Return Value:

    NDIS_STATUS   

--*/      
{
    struct _SET *Set;

    Set = &NdisSetRequest->DATA.SET_INFORMATION;

    //
    // Validate the request
    //
    if (Set->InformationBufferLength < 
        NDIS_SIZEOF_RECEIVE_SCALE_PARAMETERS_V2_REVISION_1)
    {
        DEBUGP(MP_ERROR, "OID_GEN_RECEIVE_SCALE_PARAMETERS_V2: Invalid InformationBufferLength\n");

        return NDIS_STATUS_INVALID_LENGTH;
    }

    return NICSetRSSv2Parameters(Adapter, NdisSetRequest);
}


static
NDIS_STATUS
MPSetRSSv2IndirectionTableEntries(
    _In_ PMP_ADAPTER        Adapter,
    _In_ PNDIS_OID_REQUEST  NdisMethodRequest)
/*++
Routine Description:

    This routine handles OID_GEN_RSS_SET_INDIRECTION_TABLE_ENTRIES method request. 

Arguments:

    Adapter         - Pointer to adapter block
    NdisSetRequest  - The OID data for the request

Return Value:

    NDIS_STATUS   

--*/      
{
    struct _METHOD *Method;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    Method = &NdisMethodRequest->DATA.METHOD_INFORMATION;

    //
    // Validate the request
    //
    if (Method->InputBufferLength < 
        NDIS_SIZEOF_RSS_SET_INDIRECTION_ENTRIES_REVISION_1)
    {
        DEBUGP(MP_ERROR, "OID_GEN_RSS_SET_INDIRECTION_TABLE_ENTRIES: Invalid InformationBufferLength\n");

        return NDIS_STATUS_INVALID_LENGTH;
    }

    return NICSetRSSv2IndirectionTableEntries(Adapter, NdisMethodRequest);
}
#endif