/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

   Adapter.C

Abstract:

    The purpose of this sample is to illustrate functionality of a deserialized
    NDIS miniport driver without requiring a physical network adapter. This
    sample is based on E100BEX sample present in the DDK. It is basically a
    simplified version of E100bex driver. The driver can be installed either
    manually using Add Hardware wizard as a root enumerated virtual miniport
    driver or on a virtual bus (like toaster bus). Since the driver does not
    interact with any hardware, it makes it very easy to understand the miniport
    interface and the usage of various NDIS functions without the clutter of
    hardware specific code normally found in a fully functional driver.

    This sample provides an example of minimal driver intended for education
    purposes. Neither the driver or its sample test programs are intended
    for use in a production environment.

Revision History:

Notes:

--*/

#include "netvmin6.h"
#include "adapter.tmh"

static
NDIS_STATUS
NICAllocAdapter(
    _In_ NDIS_HANDLE MiniportAdapterHandle,
    _Outptr_ PMP_ADAPTER *Adapter);

void
NICFreeAdapter(
    _In_  PMP_ADAPTER Adapter);

VOID
NICFreeReceiveDpc(
    _In_ PMP_ADAPTER_RECEIVE_DPC AdapterDpc);

static
NDIS_STATUS
NICReadRegParameters(
    _In_  PMP_ADAPTER Adapter);

static
VOID
NICSetMacAddress(
    _In_  PMP_ADAPTER  Adapter,
    _In_  NDIS_HANDLE  ConfigurationHandle);


static
VOID
NICScheduleTheResetOrPauseDpc(
    _In_  PMP_ADAPTER  Adapter,
    _In_  BOOLEAN      fReschedule);

NDIS_TIMER_FUNCTION NICAsyncResetOrPauseDpc;

#pragma NDIS_PAGEABLE_FUNCTION(MPInitializeEx)
#pragma NDIS_PAGEABLE_FUNCTION(MPPause)
#pragma NDIS_PAGEABLE_FUNCTION(MPRestart)
#pragma NDIS_PAGEABLE_FUNCTION(MPHaltEx)
#pragma NDIS_PAGEABLE_FUNCTION(MPDevicePnpEventNotify)
#pragma NDIS_PAGEABLE_FUNCTION(NICAllocAdapter)
#pragma NDIS_PAGEABLE_FUNCTION(NICReadRegParameters)
#pragma NDIS_PAGEABLE_FUNCTION(NICSetMacAddress)



NDIS_OID NICSupportedOids[] =
{
        OID_GEN_HARDWARE_STATUS,
        OID_GEN_TRANSMIT_BUFFER_SPACE,
        OID_GEN_RECEIVE_BUFFER_SPACE,
        OID_GEN_TRANSMIT_BLOCK_SIZE,
        OID_GEN_RECEIVE_BLOCK_SIZE,
        OID_GEN_VENDOR_ID,
        OID_GEN_VENDOR_DESCRIPTION,
        OID_GEN_VENDOR_DRIVER_VERSION,
        OID_GEN_CURRENT_PACKET_FILTER,
        OID_GEN_CURRENT_LOOKAHEAD,
        OID_GEN_DRIVER_VERSION,
        OID_GEN_MAXIMUM_TOTAL_SIZE,
        OID_GEN_XMIT_OK,
        OID_GEN_RCV_OK,
        OID_GEN_STATISTICS,
        OID_GEN_TRANSMIT_QUEUE_LENGTH,       // Optional
        OID_GEN_LINK_PARAMETERS,
        OID_GEN_INTERRUPT_MODERATION,
        OID_GEN_MEDIA_SUPPORTED,
        OID_GEN_MEDIA_IN_USE,
        OID_GEN_MAXIMUM_SEND_PACKETS,
        OID_GEN_XMIT_ERROR,
        OID_GEN_RCV_ERROR,
        OID_GEN_RCV_NO_BUFFER,
        OID_802_3_PERMANENT_ADDRESS,
        OID_802_3_CURRENT_ADDRESS,
        OID_802_3_MULTICAST_LIST,
        OID_802_3_MAXIMUM_LIST_SIZE,
        OID_802_3_RCV_ERROR_ALIGNMENT,
        OID_802_3_XMIT_ONE_COLLISION,
        OID_802_3_XMIT_MORE_COLLISIONS,
        OID_802_3_XMIT_DEFERRED,             // Optional
        OID_802_3_XMIT_MAX_COLLISIONS,       // Optional
        OID_802_3_RCV_OVERRUN,               // Optional
        OID_802_3_XMIT_UNDERRUN,             // Optional
        OID_802_3_XMIT_HEARTBEAT_FAILURE,    // Optional
        OID_802_3_XMIT_TIMES_CRS_LOST,       // Optional
        OID_802_3_XMIT_LATE_COLLISIONS,      // Optional
        OID_PNP_CAPABILITIES,                // Optional
 #if (NDIS_SUPPORT_NDIS620)
        OID_RECEIVE_FILTER_ALLOCATE_QUEUE,
        OID_RECEIVE_FILTER_QUEUE_ALLOCATION_COMPLETE,
        OID_RECEIVE_FILTER_FREE_QUEUE,
        OID_RECEIVE_FILTER_CLEAR_FILTER,
        OID_RECEIVE_FILTER_SET_FILTER,
#endif
};


NDIS_STATUS
MPInitializeEx(
    _In_  NDIS_HANDLE MiniportAdapterHandle,
    _In_  NDIS_HANDLE MiniportDriverContext,
    _In_  PNDIS_MINIPORT_INIT_PARAMETERS MiniportInitParameters)
/*++
Routine Description:

    The MiniportInitialize function is a required function that sets up a
    NIC (or virtual NIC) for network I/O operations, claims all hardware
    resources necessary to the NIC in the registry, and allocates resources
    the driver needs to carry out network I/O operations.

    MiniportInitialize runs at IRQL = PASSIVE_LEVEL.

Arguments:

    Return Value:

    NDIS_STATUS_xxx code

--*/
{
    NDIS_STATUS          Status = NDIS_STATUS_SUCCESS;
    PMP_ADAPTER Adapter = NULL;


    DEBUGP(MP_TRACE, "---> MPInitializeEx\n");
    UNREFERENCED_PARAMETER(MiniportDriverContext);
    PAGED_CODE();


    do
    {
        NDIS_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES AdapterRegistration = {0};
        NDIS_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES AdapterGeneral = {0};

#if (NDIS_SUPPORT_NDIS620)
        NDIS_PM_CAPABILITIES PmCapabilities;
#elif (NDIS_SUPPORT_NDIS6)
        NDIS_PNP_CAPABILITIES PnpCapabilities;
#endif // NDIS MINIPORT VERSION

        //
        // Allocate adapter context structure and initialize all the
        // memory resources for sending and receiving packets.
        //
        Status = NICAllocAdapter(MiniportAdapterHandle, &Adapter);
        if(Status != NDIS_STATUS_SUCCESS)
        {
            break;
        }

        DEBUGP(MP_TRACE, "[%p] MPInitializeEx Adapter allocated.\n", Adapter);

        //
        // First, set the registration attributes.
        //
#if (NDIS_SUPPORT_NDIS630)
        {C_ASSERT(sizeof(AdapterRegistration) >= NDIS_SIZEOF_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES_REVISION_2);}
        AdapterRegistration.Header.Type = NDIS_OBJECT_TYPE_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES;
        AdapterRegistration.Header.Size = sizeof(AdapterRegistration);
        AdapterRegistration.Header.Revision = NDIS_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES_REVISION_2;
#else
        {C_ASSERT(sizeof(AdapterRegistration) >= NDIS_SIZEOF_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES_REVISION_1);}
        AdapterRegistration.Header.Type = NDIS_OBJECT_TYPE_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES;
        AdapterRegistration.Header.Size = sizeof(AdapterRegistration);
        AdapterRegistration.Header.Revision = NDIS_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES_REVISION_1;
#endif // NDIS MINIPORT VERSION

        AdapterRegistration.MiniportAdapterContext = Adapter;
        AdapterRegistration.AttributeFlags = NIC_ADAPTER_ATTRIBUTES_FLAGS;

#if (NDIS_SUPPORT_NDIS630)
        AdapterRegistration.AttributeFlags |= NDIS_MINIPORT_ATTRIBUTES_NO_PAUSE_ON_SUSPEND;
#endif

        AdapterRegistration.CheckForHangTimeInSeconds = NIC_ADAPTER_CHECK_FOR_HANG_TIME_IN_SECONDS;
        AdapterRegistration.InterfaceType = NIC_INTERFACE_TYPE;

        NDIS_DECLARE_MINIPORT_ADAPTER_CONTEXT(MP_ADAPTER);
        Status = NdisMSetMiniportAttributes(
                MiniportAdapterHandle,
                (PNDIS_MINIPORT_ADAPTER_ATTRIBUTES)&AdapterRegistration);
        if (NDIS_STATUS_SUCCESS != Status)
        {
            DEBUGP(MP_ERROR, "[%p] NdisSetOptionalHandlers Status 0x%08x\n", Adapter, Status);
            break;
        }

        //
        // Read Advanced configuration information from the registry
        //
        Status = NICReadRegParameters(Adapter);
        if(Status != NDIS_STATUS_SUCCESS)
        {
            break;
        }

        //
        // NdisMGetDeviceProperty function enables us to get the:
        // PDO - created by the bus driver to represent our device.
        // FDO - created by NDIS to represent our miniport as a function driver.
        // NextDeviceObject - deviceobject of another driver (filter)
        //                      attached to us at the bottom.
        // In a pure NDIS miniport driver, there is no use for this
        // information, but a NDISWDM driver would need to know this so that it
        // can transfer packets to the lower WDM stack using IRPs.
        //
        NdisMGetDeviceProperty(
                MiniportAdapterHandle,
                &Adapter->Pdo,
                &Adapter->Fdo,
                &Adapter->NextDeviceObject,
                NULL,
                NULL);

        //
        // Allocate default DPC, using current processor information (this will be
        // added to the DPC list by the allocation function). The default DPC
        // is used for all receives when VMQ is not enabled, and for the default queue
        // when VMQ is enabled.
        //
        Adapter->DefaultRecvDpc = NICAllocReceiveDpc(Adapter, 0, 0, 0);
        if(!Adapter->DefaultRecvDpc)
        {
            DEBUGP(MP_ERROR, "[%p] Failed while allocating the default DPC\n", Adapter);
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        //
        // If VMQ is enabled, allocate the default receive queue. Otherwise, initialize the default receive
        // block for use in non-VMQ receives
        //
        if(VMQ_ENABLED(Adapter))
        {
            Status = AllocateDefaultRxQueue(Adapter);
            if(Status != NDIS_STATUS_SUCCESS)
            {
                DEBUGP(MP_ERROR, "[%p] AllocateDefaultRxQueue Status 0x%08x\n", Adapter, Status);
                break;
            }
        }
        else
        {
            //
            // Initialize the default receive block
            //
            Status = NICInitializeReceiveBlock(Adapter, 0);
            if(Status != NDIS_STATUS_SUCCESS)
            {
                DEBUGP(MP_ERROR, "[%p] NICInitializeReceiveBlock Status 0x%08x\n", Adapter, Status);
                break;
            }

            //
            // Initialize list and lock of the free Rcb list
            //
            NdisInitializeListHead(&Adapter->FreeRcbList);
            NdisAllocateSpinLock(&Adapter->FreeRcbListLock);

            //
            // Allocate the adapter's non-VMQ RCB & receive NBL data
            //
            Status = NICAllocRCBData(
                Adapter,
                NIC_MAX_BUSY_RECVS,
                &Adapter->RcbMemoryBlock,
                &Adapter->FreeRcbList,
                &Adapter->FreeRcbListLock,
                &Adapter->RecvNblPoolHandle
                );
            if(Status != NDIS_STATUS_SUCCESS)
            {
                break;
            }
        }

        //
        // Next, set the general attributes.
        //

#if (NDIS_SUPPORT_NDIS620)
        {C_ASSERT(sizeof(AdapterGeneral) >= NDIS_SIZEOF_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES_REVISION_2);}
        AdapterGeneral.Header.Type = NDIS_OBJECT_TYPE_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES;
        AdapterGeneral.Header.Size = NDIS_SIZEOF_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES_REVISION_2;
        AdapterGeneral.Header.Revision = NDIS_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES_REVISION_2;
#elif (NDIS_SUPPORT_NDIS6)
        {C_ASSERT(sizeof(AdapterGeneral) >= NDIS_SIZEOF_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES_REVISION_1);}
        AdapterGeneral.Header.Type = NDIS_OBJECT_TYPE_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES;
        AdapterGeneral.Header.Size = NDIS_SIZEOF_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES_REVISION_1;
        AdapterGeneral.Header.Revision = NDIS_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES_REVISION_1;
#endif // NDIS MINIPORT VERSION

        //
        // Specify the medium type that the NIC can support but not
        // necessarily the medium type that the NIC currently uses.
        //
        AdapterGeneral.MediaType = NIC_MEDIUM_TYPE;

        //
        // Specifiy medium type that the NIC currently uses.
        //
        AdapterGeneral.PhysicalMediumType = NIC_PHYSICAL_MEDIUM;

        //
        // Specifiy the maximum network frame size, in bytes, that the NIC
        // supports excluding the header. A NIC driver that emulates another
        // medium type for binding to a transport must ensure that the maximum
        // frame size for a protocol-supplied net buffer does not exceed the
        // size limitations for the true network medium.
        //
        AdapterGeneral.MtuSize = HW_FRAME_MAX_DATA_SIZE;
        AdapterGeneral.MaxXmitLinkSpeed = Adapter->ulLinkSendSpeed;
        AdapterGeneral.XmitLinkSpeed = Adapter->ulLinkSendSpeed;
        AdapterGeneral.MaxRcvLinkSpeed = Adapter->ulLinkRecvSpeed;
        AdapterGeneral.RcvLinkSpeed = Adapter->ulLinkRecvSpeed;
        AdapterGeneral.MediaConnectState = HWGetMediaConnectStatus(Adapter);
        AdapterGeneral.MediaDuplexState = MediaDuplexStateFull;

        //
        // The maximum number of bytes the NIC can provide as lookahead data.
        // If that value is different from the size of the lookahead buffer
        // supported by bound protocols, NDIS will call MiniportOidRequest to
        // set the size of the lookahead buffer provided by the miniport driver
        // to the minimum of the miniport driver and protocol(s) values. If the
        // driver always indicates up full packets with
        // NdisMIndicateReceiveNetBufferLists, it should set this value to the
        // maximum total frame size, which excludes the header.
        //
        // Upper-layer drivers examine lookahead data to determine whether a
        // packet that is associated with the lookahead data is intended for
        // one or more of their clients. If the underlying driver supports
        // multipacket receive indications, bound protocols are given full net
        // packets on every indication. Consequently, this value is identical
        // to that returned for OID_GEN_RECEIVE_BLOCK_SIZE.
        //
        AdapterGeneral.LookaheadSize = Adapter->ulLookahead;
        AdapterGeneral.MacOptions = NIC_MAC_OPTIONS;
        AdapterGeneral.SupportedPacketFilters = NIC_SUPPORTED_FILTERS;


        //
        // The maximum number of multicast addresses the NIC driver can manage.
        // This list is global for all protocols bound to (or above) the NIC.
        // Consequently, a protocol can receive NDIS_STATUS_MULTICAST_FULL from
        // the NIC driver when attempting to set the multicast address list,
        // even if the number of elements in the given list is less than the
        // number originally returned for this query.
        //
        AdapterGeneral.MaxMulticastListSize = NIC_MAX_MCAST_LIST;
        AdapterGeneral.MacAddressLength = NIC_MACADDR_SIZE;


        //
        // Return the MAC address of the NIC burnt in the hardware.
        //
        NIC_COPY_ADDRESS(AdapterGeneral.PermanentMacAddress, Adapter->PermanentAddress);

        //
        // Return the MAC address the NIC is currently programmed to use. Note
        // that this address could be different from the permananent address as
        // the user can override using registry. Read NdisReadNetworkAddress
        // doc for more info.
        //
        NIC_COPY_ADDRESS(AdapterGeneral.CurrentMacAddress, Adapter->CurrentAddress);
        AdapterGeneral.RecvScaleCapabilities = NULL;
        AdapterGeneral.AccessType = NIC_ACCESS_TYPE;
        AdapterGeneral.DirectionType = NIC_DIRECTION_TYPE;
        AdapterGeneral.ConnectionType = NIC_CONNECTION_TYPE;
        AdapterGeneral.IfType = NIC_IFTYPE;
        AdapterGeneral.IfConnectorPresent = NIC_HAS_PHYSICAL_CONNECTOR;
        AdapterGeneral.SupportedStatistics = NIC_SUPPORTED_STATISTICS;
        AdapterGeneral.SupportedPauseFunctions = NdisPauseFunctionsUnsupported;
        AdapterGeneral.DataBackFillSize = 0;
        AdapterGeneral.ContextBackFillSize = 0;


        //
        // The SupportedOidList is an array of OIDs for objects that the
        // underlying driver or its NIC supports.  Objects include general,
        // media-specific, and implementation-specific objects. NDIS forwards a
        // subset of the returned list to protocols that make this query. That
        // is, NDIS filters any supported statistics OIDs out of the list
        // because protocols never make statistics queries.
        //
        AdapterGeneral.SupportedOidList = NICSupportedOids;
        AdapterGeneral.SupportedOidListLength = sizeof(NICSupportedOids);
        AdapterGeneral.AutoNegotiationFlags = NDIS_LINK_STATE_DUPLEX_AUTO_NEGOTIATED;

        //
        // Set the power management capabilities.  The format used is NDIS
        // version-specific.
        //
#if (NDIS_SUPPORT_NDIS680)

        NdisZeroMemory(&PmCapabilities, sizeof(PmCapabilities));

        {C_ASSERT(sizeof(PmCapabilities) >= NDIS_SIZEOF_NDIS_PM_CAPABILITIES_REVISION_2); }
        PmCapabilities.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
        PmCapabilities.Header.Size = NDIS_SIZEOF_NDIS_PM_CAPABILITIES_REVISION_2;
        PmCapabilities.Header.Revision = NDIS_PM_CAPABILITIES_REVISION_2;

        PmCapabilities.MinMagicPacketWakeUp = NIC_MAGIC_PACKET_WAKEUP;
        PmCapabilities.MinPatternWakeUp = NIC_PATTERN_WAKEUP;
        PmCapabilities.MinLinkChangeWakeUp = NIC_LINK_CHANGE_WAKEUP;

        AdapterGeneral.PowerManagementCapabilitiesEx = &PmCapabilities;

#elif (NDIS_SUPPORT_NDIS620)

        NdisZeroMemory(&PmCapabilities, sizeof(PmCapabilities));

        {C_ASSERT(sizeof(PmCapabilities) >= NDIS_SIZEOF_NDIS_PM_CAPABILITIES_REVISION_1); }
        PmCapabilities.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
        PmCapabilities.Header.Size = NDIS_SIZEOF_NDIS_PM_CAPABILITIES_REVISION_1;
        PmCapabilities.Header.Revision = NDIS_PM_CAPABILITIES_REVISION_1;

        PmCapabilities.MinMagicPacketWakeUp = NIC_MAGIC_PACKET_WAKEUP;
        PmCapabilities.MinPatternWakeUp = NIC_PATTERN_WAKEUP;
        PmCapabilities.MinLinkChangeWakeUp = NIC_LINK_CHANGE_WAKEUP;

        AdapterGeneral.PowerManagementCapabilitiesEx = &PmCapabilities;

#elif (NDIS_SUPPORT_NDIS6)

        NdisZeroMemory(&PnpCapabilities, sizeof(PnpCapabilities));
        PnpCapabilities.WakeUpCapabilities.MinMagicPacketWakeUp = NIC_MAGIC_PACKET_WAKEUP;
        PnpCapabilities.WakeUpCapabilities.MinPatternWakeUp = NIC_PATTERN_WAKEUP;
        AdapterGeneral.PowerManagementCapabilities = &PnpCapabilities; // Optional

#endif // NDIS MINIPORT VERSION

        Status = NdisMSetMiniportAttributes(
                MiniportAdapterHandle,
                (PNDIS_MINIPORT_ADAPTER_ATTRIBUTES)&AdapterGeneral);
        if (NDIS_STATUS_SUCCESS != Status)
        {
            DEBUGP(MP_ERROR, "[%p] NdisSetOptionalHandlers Status 0x%08x\n", Adapter, Status);
            break;
        }

        //
        // Set Miniport attributes for supported and enabled VMQ features.
        //
        Status = InitializeRxQueueMPConfig(Adapter);
        if (NDIS_STATUS_SUCCESS != Status)
        {
            DEBUGP(MP_ERROR, "[%p] InitializeRxQueueMPConfig Status 0x%08x\n", Adapter, Status);
            break;
        }

        //
        // Set miniport attributes for supported and enabled NDIS QOS features.
        //
        Status = InitializeQOSConfig(Adapter);
        if (NDIS_STATUS_SUCCESS != Status)
        {
            DEBUGP(MP_ERROR, "[%p] InitializeQOSConfig Status 0x%08x\n", Adapter, Status);
            break;
        }

#if (NDIS_SUPPORT_NDIS680)
        //
        // Set miniport attributes for supported and enabled NDIS RSS features.
        //
        Status = InitializeRSSConfig(Adapter);
        if (NDIS_STATUS_SUCCESS != Status)
        {
            DEBUGP(MP_ERROR, "[%p] InitializeRSSConfig Status 0x%08x\n", Adapter, Status);
            break;
        }
#endif
        //
        // For hardware devices, you should register your interrupt handlers
        // here, using NdisMRegisterInterruptEx.
        //

        //
        // Get the Adapter Resources & Initialize the hardware.
        //

        Status = HWInitialize(Adapter, MiniportInitParameters);
        if(Status != NDIS_STATUS_SUCCESS)
        {
            Status = NDIS_STATUS_FAILURE;
            break;
        }
    } while(FALSE);


    if (Status != NDIS_STATUS_SUCCESS)
    {
        if (Adapter)
        {
            NICFreeAdapter(Adapter);
        }
        Adapter = NULL;
    }


    DEBUGP(MP_TRACE, "[%p] <--- MPInitializeEx Status = 0x%08x%\n", Adapter, Status);
    return Status;
}


NDIS_STATUS
MPPause(
    _In_  NDIS_HANDLE  MiniportAdapterContext,
    _In_  PNDIS_MINIPORT_PAUSE_PARAMETERS MiniportPauseParameters)
/*++

Routine Description:

    When a miniport receives a pause request, it enters into a Pausing state.
    The miniport should not indicate up any more network data.  Any pending
    send requests must be completed, and new requests must be rejected with
    NDIS_STATUS_PAUSED.

    Once all sends have been completed and all recieve NBLs have returned to
    the miniport, the miniport enters the Paused state.

    While paused, the miniport can still service interrupts from the hardware
    (to, for example, continue to indicate NDIS_STATUS_MEDIA_CONNECT
    notifications).

    The miniport must continue to be able to handle status indications and OID
    requests.  MiniportPause is different from MiniportHalt because, in
    general, the MiniportPause operation won't release any resources.
    MiniportPause must not attempt to acquire any resources where allocation
    can fail, since MiniportPause itself must not fail.


    MiniportPause runs at IRQL = PASSIVE_LEVEL.

Arguments:

    MiniportAdapterContext  Pointer to the Adapter
    MiniportPauseParameters  Additional information about the pause operation

Return Value:

    If the miniport is able to immediately enter the Paused state, it should
    return NDIS_STATUS_SUCCESS.

    If the miniport must wait for send completions or pending receive NBLs, it
    should return NDIS_STATUS_PENDING now, and call NDISMPauseComplete when the
    miniport has entered the Paused state.

    No other return value is permitted.  The pause operation must not fail.

--*/
{
    NDIS_STATUS Status;

    PMP_ADAPTER Adapter = MP_ADAPTER_FROM_CONTEXT(MiniportAdapterContext);

    DEBUGP(MP_TRACE, "[%p] ---> MPPause\n", Adapter);
    UNREFERENCED_PARAMETER(MiniportPauseParameters);
    PAGED_CODE();

    MP_SET_FLAG(Adapter, fMP_ADAPTER_PAUSE_IN_PROGRESS);

    NICStopTheDatapath(Adapter);


    if (NICIsBusy(Adapter))
    {
        //
        // The adapter is busy sending or receiving data, so the pause must
        // be completed asynchronously later.
        //
        NICScheduleTheResetOrPauseDpc(Adapter, FALSE);
        Status = NDIS_STATUS_PENDING;
    }
    else // !NICIsBusy
    {
        //
        // The pause operation has completed synchronously.
        //
        MP_SET_FLAG(Adapter, fMP_ADAPTER_PAUSED);
        MP_CLEAR_FLAG(Adapter, fMP_ADAPTER_PAUSE_IN_PROGRESS);
        Status = NDIS_STATUS_SUCCESS;
    }

    DEBUGP(MP_TRACE, "[%p] <--- MPPause\n", Adapter);
    return Status;
}


NDIS_STATUS
MPRestart(
    _In_  NDIS_HANDLE                             MiniportAdapterContext,
    _In_  PNDIS_MINIPORT_RESTART_PARAMETERS       RestartParameters)
/*++

Routine Description:

    When a miniport receives a restart request, it enters into a Restarting
    state.  The miniport may begin indicating received data (e.g., using
    NdisMIndicateReceiveNetBufferLists), handling status indications, and
    processing OID requests in the Restarting state.  However, no sends will be
    requested while the miniport is in the Restarting state.

    Once the miniport is ready to send data, it has entered the Running state.
    The miniport informs NDIS that it is in the Running state by returning
    NDIS_STATUS_SUCCESS from this MiniportRestart function; or if this function
    has already returned NDIS_STATUS_PENDING, by calling NdisMRestartComplete.


    MiniportRestart runs at IRQL = PASSIVE_LEVEL.

Arguments:

    MiniportAdapterContext  Pointer to the Adapter
    RestartParameters  Additional information about the restart operation

Return Value:

    If the miniport is able to immediately enter the Running state, it should
    return NDIS_STATUS_SUCCESS.

    If the miniport is still in the Restarting state, it should return
    NDIS_STATUS_PENDING now, and call NdisMRestartComplete when the miniport
    has entered the Running state.

    Other NDIS_STATUS codes indicate errors.  If an error is encountered, the
    miniport must return to the Paused state (i.e., stop indicating receives).

--*/

{
    NDIS_STATUS Status = NDIS_STATUS_PENDING;

    PMP_ADAPTER Adapter = MP_ADAPTER_FROM_CONTEXT(MiniportAdapterContext);

    DEBUGP(MP_TRACE, "[%p] ---> MPRestart\n", Adapter);
    UNREFERENCED_PARAMETER(Adapter);
    UNREFERENCED_PARAMETER(RestartParameters);
    PAGED_CODE();


    MP_CLEAR_FLAG(Adapter, (fMP_ADAPTER_PAUSE_IN_PROGRESS|fMP_ADAPTER_PAUSED));

    NICStartTheDatapath(Adapter);


    //
    // The simulated hardware is immediately ready to send data again, so in
    // this sample code, MiniportRestart always returns success.  If we had to
    // wait for hardware to reinitialize, we'd return NDIS_STATUS_PENDING now,
    // and call NdisMRestartComplete later.
    //
    Status = NDIS_STATUS_SUCCESS;


    DEBUGP(MP_TRACE, "[%p] <--- MPRestart\n", Adapter);
    return Status;
}


VOID
MPHaltEx(
    IN  NDIS_HANDLE MiniportAdapterContext,
    IN  NDIS_HALT_ACTION HaltAction
    )
/*++

Routine Description:

    Halt handler is called when NDIS receives IRP_MN_STOP_DEVICE,
    IRP_MN_SUPRISE_REMOVE or IRP_MN_REMOVE_DEVICE requests from the PNP
    manager. Here, the driver should free all the resources acquired in
    MiniportInitialize and stop access to the hardware. NDIS will not submit
    any further request once this handler is invoked.

    1) Free and unmap all I/O resources.
    2) Disable interrupt and deregister interrupt handler.
    3) Deregister shutdown handler regsitered by
        NdisMRegisterAdapterShutdownHandler .
    4) Cancel all queued up timer callbacks.
    5) Finally wait indefinitely for all the outstanding receive
        packets indicated to the protocol to return.

    MiniportHalt runs at IRQL = PASSIVE_LEVEL.


Arguments:

    MiniportAdapterContext  Pointer to the Adapter
    HaltAction  The reason for halting the adapter

Return Value:

    None.

--*/
{
    PMP_ADAPTER       Adapter = MP_ADAPTER_FROM_CONTEXT(MiniportAdapterContext);
    LONG              nHaltCount = 0;


    PAGED_CODE();
    DEBUGP(MP_TRACE, "[%p] ---> MPHaltEx\n", Adapter);
    UNREFERENCED_PARAMETER(HaltAction);

    MP_SET_FLAG(Adapter, fMP_ADAPTER_HALT_IN_PROGRESS);

    //
    // Call Shutdown handler to disable interrupt and turn the hardware off by
    // issuing a full reset
    //
    // On XP and later, NDIS notifies our PNP event handler the reason for
    // calling Halt. So before accessing the device, check to see if the device
    // is surprise removed, if so don't bother calling the shutdown handler to
    // stop the hardware because it doesn't exist.
    //
    if(!MP_TEST_FLAG(Adapter, fMP_ADAPTER_SURPRISE_REMOVED)) {
        MPShutdownEx(MiniportAdapterContext, NdisShutdownPowerOff);
    }


    NICStopTheDatapath(Adapter);


    while (NICIsBusy(Adapter) ||
           Adapter->SendCompleteWorkItemQueued ||
           Adapter->SendCompleteWorkItemRunning)
    {
        if (++nHaltCount % 100 == 0)
        {
            DEBUGP(MP_ERROR, "[%p] Halt timed out!\n", Adapter);
            ASSERT(FALSE);
        }

        DEBUGP(MP_INFO, "[%p] MPHaltEx - waiting ...\n", Adapter);
        NdisMSleep(1000);
    }


    NICFreeAdapter(Adapter);

    DEBUGP(MP_TRACE, "[%p] <--- MPHaltEx\n", Adapter);
}


NDIS_STATUS
MPResetEx(
    _In_  NDIS_HANDLE MiniportAdapterContext,
    _Out_ PBOOLEAN AddressingReset)
/*++

Routine Description:

    MiniportResetEx is a required to issue a hardware reset to the NIC
    and/or to reset the driver's software state.

    1) The miniport driver can optionally complete any pending
        OID requests. NDIS will submit no further OID requests
        to the miniport driver for the NIC being reset until
        the reset operation has finished. After the reset,
        NDIS will resubmit to the miniport driver any OID requests
        that were pending but not completed by the miniport driver
        before the reset.

    2) A deserialized miniport driver must complete any pending send
        operations. NDIS will not requeue pending send packets for
        a deserialized driver since NDIS does not maintain the send
        queue for such a driver.

    3) If MiniportReset returns NDIS_STATUS_PENDING, the driver must
        complete the original request subsequently with a call to
        NdisMResetComplete.

    MiniportReset runs at IRQL <= DISPATCH_LEVEL.

Arguments:

AddressingReset - If multicast or functional addressing information
                  or the lookahead size, is changed by a reset,
                  MiniportReset must set the variable at AddressingReset
                  to TRUE before it returns control. This causes NDIS to
                  call the MiniportSetInformation function to restore
                  the information.

MiniportAdapterContext - Pointer to our adapter

Return Value:

    NDIS_STATUS

--*/
{
    NDIS_STATUS       Status;
    PMP_ADAPTER       Adapter = MP_ADAPTER_FROM_CONTEXT(MiniportAdapterContext);

    DEBUGP(MP_TRACE, "[%p] ---> MPResetEx\n", Adapter);

    *AddressingReset = FALSE;

    do
    {
        ASSERT(!MP_TEST_FLAG(Adapter, fMP_ADAPTER_HALT_IN_PROGRESS));

        if (MP_TEST_FLAG(Adapter, fMP_RESET_IN_PROGRESS))
        {
            Status = NDIS_STATUS_RESET_IN_PROGRESS;
            break;
        }

        MP_SET_FLAG(Adapter, fMP_RESET_IN_PROGRESS);

        //
        // Complete all the queued up send packets
        //
        TXFlushSendQueue(Adapter, NDIS_STATUS_RESET_IN_PROGRESS);

        if (NICIsBusy(Adapter))
        {
            NICScheduleTheResetOrPauseDpc(Adapter, FALSE);

            //
            // By returning NDIS_STATUS_PENDING, we are promising NDIS that
            // we will complete the reset request by calling NdisMResetComplete.
            //
            Status = NDIS_STATUS_PENDING;
            break;
        }

        MP_CLEAR_FLAG(Adapter, fMP_RESET_IN_PROGRESS);

        Status = NDIS_STATUS_SUCCESS;

    } while(FALSE);


    DEBUGP(MP_TRACE, "[%p] <--- MPResetEx Status = 0x%08x\n", Adapter, Status);
    return Status;
}


VOID
NICScheduleTheResetOrPauseDpc(
    _In_  PMP_ADAPTER  Adapter,
    _In_  BOOLEAN      fReschedule)
/*++

Routine Description:

    Schedules the timer callback function for asynchronous pause/reset.

Arguments:

    Adapter      -
    fReschedule  - TRUE if this is rescheduling an existing async pause/reset
                   operation, or FALSE if this is a new pause/reset operation.

Return Value:

    None.

--*/
{
    LARGE_INTEGER liRetryTime;

    DEBUGP(MP_TRACE, "[%p] ---> NICScheduleTheResetOrPauseDpc\n", Adapter);

    if (!fReschedule)
    {
        Adapter->AsyncBusyCheckCount = 0;
    }


    liRetryTime.QuadPart = -1000000LL; // 100ms in 100ns increments
    NdisSetTimerObject(Adapter->AsyncBusyCheckTimer, liRetryTime, 0, NULL);

    DEBUGP(MP_TRACE, "[%p] <--- NICScheduleTheResetOrPauseDpc\n", Adapter);
}


_Use_decl_annotations_
VOID
NICAsyncResetOrPauseDpc(
    PVOID                    SystemSpecific1,
    PVOID                    FunctionContext,
    PVOID                    SystemSpecific2,
    PVOID                    SystemSpecific3)
/*++

Routine Description:

    Timer callback function for Reset operation.

Arguments:

FunctionContext - Pointer to our adapter

Return Value:

    None.

--*/
{
    PMP_ADAPTER Adapter = MP_ADAPTER_FROM_CONTEXT(FunctionContext);
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(SystemSpecific1);
    UNREFERENCED_PARAMETER(SystemSpecific2);
    UNREFERENCED_PARAMETER(SystemSpecific3);


    DEBUGP(MP_TRACE, "[%p] ---> NICAsyncResetOrPauseDpc\n", Adapter);

    if (MP_TEST_FLAG(Adapter, fMP_ADAPTER_PAUSE_IN_PROGRESS)
            || MP_TEST_FLAG(Adapter, fMP_RESET_IN_PROGRESS))
    {
        BOOLEAN fBusy = NICIsBusy(Adapter);

        if (fBusy && ++Adapter->AsyncBusyCheckCount <= 20)
        {
            //
            // Still busy -- let's try another time.
            //
            NICScheduleTheResetOrPauseDpc(Adapter, TRUE);
        }
        else
        {
            if (fBusy)
            {

                //
                // We have tried enough. Something is wrong. Let us
                // just complete the reset request with failure.
                //
                DEBUGP(MP_ERROR, "[%p] Reset timed out!!!\n", Adapter);

                ASSERT(FALSE);

                Status = NDIS_STATUS_FAILURE;
            }


            if (MP_TEST_FLAG(Adapter, fMP_ADAPTER_PAUSE_IN_PROGRESS))
            {
                DEBUGP(MP_INFO, "[%p] Done - NdisMPauseComplete\n", Adapter);

                MP_SET_FLAG(Adapter, fMP_ADAPTER_PAUSED);
                MP_CLEAR_FLAG(Adapter, fMP_ADAPTER_PAUSE_IN_PROGRESS);

                NdisMPauseComplete(Adapter->AdapterHandle);
            }
            else if (MP_TEST_FLAG(Adapter, fMP_RESET_IN_PROGRESS))
            {
                DEBUGP(MP_INFO, "[%p] Done - NdisMResetComplete\n", Adapter);

                MP_CLEAR_FLAG(Adapter, fMP_RESET_IN_PROGRESS);

                NdisMResetComplete(Adapter->AdapterHandle, Status, FALSE);
            }
        }
    }


    DEBUGP(MP_TRACE, "[%p] <--- NICAsyncResetOrPauseDpc Status = 0x%08x\n", Adapter, Status);
}

VOID
MPShutdownEx(
    _In_  NDIS_HANDLE           MiniportAdapterContext,
    _In_  NDIS_SHUTDOWN_ACTION  ShutdownAction)
/*++

Routine Description:

    The MiniportShutdownEx handler restores hardware to its initial state when
    the system is shut down, whether by the user or because an unrecoverable
    system error occurred. This is to ensure that the NIC is in a known
    state and ready to be reinitialized when the machine is rebooted after
    a system shutdown occurs for any reason, including a crash dump.

    Here just disable the interrupt and stop the DMA engine.  Do not free
    memory resources or wait for any packet transfers to complete.  Do not call
    into NDIS at this time.

    This can be called at aribitrary IRQL, including in the context of a
    bugcheck.

Arguments:

    MiniportAdapterContext  Pointer to our adapter
    ShutdownAction  The reason why NDIS called the shutdown function

Return Value:

    None.

--*/
{
    PMP_ADAPTER Adapter = MP_ADAPTER_FROM_CONTEXT(MiniportAdapterContext);
    UNREFERENCED_PARAMETER(ShutdownAction);
    UNREFERENCED_PARAMETER(Adapter);

    DEBUGP(MP_TRACE, "[%p] ---> MPShutdownEx\n", Adapter);

    //
    // We don't have any hardware to reset.
    //

    DEBUGP(MP_TRACE, "[%p] <--- MPShutdownEx\n", Adapter);
}


BOOLEAN
MPCheckForHangEx(
    _In_  NDIS_HANDLE MiniportAdapterContext)
/*++

Routine Description:

    The MiniportCheckForHangEx handler is called to report the state of the
    NIC, or to monitor the responsiveness of an underlying device driver.
    This is an optional function. If this handler is not specified, NDIS
    judges the driver unresponsive when the driver holds
    MiniportQueryInformation or MiniportSetInformation requests for a
    time-out interval (deafult 4 sec), and then calls the driver's
    MiniportReset function. A NIC driver's MiniportInitialize function can
    extend NDIS's time-out interval by calling NdisMSetAttributesEx to
    avoid unnecessary resets.

    MiniportCheckForHangEx runs at IRQL <= DISPATCH_LEVEL.

Arguments:

    MiniportAdapterContext  Pointer to our adapter

Return Value:

    TRUE    NDIS calls the driver's MiniportReset function.
    FALSE   Everything is fine

--*/
{
    PMP_ADAPTER Adapter = MP_ADAPTER_FROM_CONTEXT(MiniportAdapterContext);

    DEBUGP(MP_TRACE, "[%p] ---> MPCheckForHangEx\n", Adapter);
    DEBUGP(MP_TRACE, "[%p] <--- MPCheckForHangEx. FALSE\n", Adapter);
    return FALSE;
}


VOID
MPDevicePnpEventNotify(
    _In_  NDIS_HANDLE             MiniportAdapterContext,
    _In_  PNET_DEVICE_PNP_EVENT   NetDevicePnPEvent)
/*++

Routine Description:



    Runs at IRQL = PASSIVE_LEVEL in the context of system thread.

Arguments:

    MiniportAdapterContext      Pointer to our adapter
    NetDevicePnPEvent           Self-explanatory

Return Value:

    None.

--*/
{
    PMP_ADAPTER Adapter = MP_ADAPTER_FROM_CONTEXT(MiniportAdapterContext);

    DEBUGP(MP_TRACE, "[%p] ---> MPDevicePnpEventNotify\n", Adapter);

    PAGED_CODE();


    switch (NetDevicePnPEvent->DevicePnPEvent)
    {
        case NdisDevicePnPEventSurpriseRemoved:
            //
            // Called when NDIS receives IRP_MN_SUPRISE_REMOVAL.
            // NDIS calls MiniportHalt function after this call returns.
            //
            MP_SET_FLAG(Adapter, fMP_ADAPTER_SURPRISE_REMOVED);
            DEBUGP(MP_INFO, "[%p] MPDevicePnpEventNotify: NdisDevicePnPEventSurpriseRemoved\n", Adapter);
            break;

        case NdisDevicePnPEventPowerProfileChanged:
            //
            // After initializing a miniport driver and after miniport driver
            // receives an OID_PNP_SET_POWER notification that specifies
            // a device power state of NdisDeviceStateD0 (the powered-on state),
            // NDIS calls the miniport's MiniportPnPEventNotify function with
            // PnPEvent set to NdisDevicePnPEventPowerProfileChanged.
            //
            DEBUGP(MP_INFO, "[%p] MPDevicePnpEventNotify: NdisDevicePnPEventPowerProfileChanged\n", Adapter);

            if (NetDevicePnPEvent->InformationBufferLength == sizeof(ULONG))
            {
                ULONG NdisPowerProfile = *((PULONG)NetDevicePnPEvent->InformationBuffer);

                if (NdisPowerProfile == NdisPowerProfileBattery)
                {
                    DEBUGP(MP_INFO, "[%p] The host system is running on battery power\n", Adapter);
                }
                if (NdisPowerProfile == NdisPowerProfileAcOnLine)
                {
                    DEBUGP(MP_INFO, "[%p] The host system is running on AC power\n", Adapter);
                }
            }
            break;

        default:
            DEBUGP(MP_ERROR, "[%p] MPDevicePnpEventNotify: unknown PnP event 0x%x\n", Adapter, NetDevicePnPEvent->DevicePnPEvent);
    }


    DEBUGP(MP_TRACE, "[%p] <--- MPDevicePnpEventNotify\n", Adapter);
}

NDIS_STATUS
NICAllocAdapter(
    _In_ NDIS_HANDLE MiniportAdapterHandle,
    _Outptr_ PMP_ADAPTER *pAdapter)
/*++
Routine Description:

    The NICAllocAdapter function allocates and initializes the memory used to track a miniport instance.

    NICAllocAdapter runs at IRQL = PASSIVE_LEVEL.

Arguments:

    MiniportAdapterHandle       NDIS handle for the adapter.
    pAdapter                    Receives the allocated and initialized adapter memory.

    Return Value:

    NDIS_STATUS_xxx code

--*/
{
    PMP_ADAPTER Adapter = NULL;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    LONG index;

    DEBUGP(MP_TRACE, "---> NICAllocAdapter\n");

    PAGED_CODE();

    *pAdapter = NULL;

    do
    {
        //
        // Allocate extra space for the MP_ADAPTER memory (one cache line's worth) so that we can
        // reference the memory from a cache-aligned starting point. This way we guarantee that
        // members with cache-aligned directives are actually cache aligned.
        //
        PVOID UnalignedAdapterBuffer = NULL;
        ULONG UnalignedAdapterBufferSize = sizeof(MP_ADAPTER)+ NdisGetSharedDataAlignment();
        NDIS_TIMER_CHARACTERISTICS Timer;

        //
        // Allocate memory for adapter context (unaligned)
        //
        UnalignedAdapterBuffer = NdisAllocateMemoryWithTagPriority(
                NdisDriverHandle,
                UnalignedAdapterBufferSize,
                NIC_TAG,
                NormalPoolPriority);
        if (!UnalignedAdapterBuffer)
        {
            Status = NDIS_STATUS_RESOURCES;
            DEBUGP(MP_ERROR, "Failed to allocate memory for adapter context\n");
            break;
        }

        //
        // Zero the memory block
        //
        NdisZeroMemory(UnalignedAdapterBuffer, UnalignedAdapterBufferSize);

        //
        // Start the Adapter pointer at a cache-aligned boundary
        //
        Adapter = ALIGN_UP_POINTER_BY(UnalignedAdapterBuffer, NdisGetSharedDataAlignment());

        //
        // Store the unaligned information so that we can free it later
        //
        Adapter->UnalignedAdapterBuffer = UnalignedAdapterBuffer;
        Adapter->UnalignedAdapterBufferSize = UnalignedAdapterBufferSize;

        //
        // Set the adapter handle
        //
        Adapter->AdapterHandle = MiniportAdapterHandle;

        NdisInitializeListHead(&Adapter->List);

        //
        // Initialize Send & Recv listheads and corresponding
        // spinlocks.
        //
        NdisInitializeListHead(&Adapter->FreeTcbList);
        NdisAllocateSpinLock(&Adapter->FreeTcbListLock);

        NdisInitializeListHead(&Adapter->SendWaitList);
        NdisAllocateSpinLock(&Adapter->SendWaitListLock);

        NdisInitializeListHead(&Adapter->BusyTcbList);
        NdisAllocateSpinLock(&Adapter->BusyTcbListLock);

        KeInitializeSpinLock(&Adapter->SendPathSpinLock);

        //
        // Set the default lookahead buffer size.
        //
        Adapter->ulLookahead = NIC_MAX_LOOKAHEAD;

        //
        // Allocate data for Send and Receive Control blocks.
        //

        Adapter->TcbMemoryBlock = NdisAllocateMemoryWithTagPriority(
                MiniportAdapterHandle,
                sizeof(TCB) * NIC_MAX_BUSY_SENDS,
                NIC_TAG_TCB,
                NormalPoolPriority);

        if (!Adapter->TcbMemoryBlock)
        {
            Status = NDIS_STATUS_RESOURCES;
            DEBUGP(MP_ERROR, "[%p] NdisAllocateMemoryWithTagPriority failed\n", Adapter);
            break;
        }

        for (index = 0; index < NIC_MAX_BUSY_SENDS; index++)
        {
            NdisInterlockedInsertTailList(
                    &Adapter->FreeTcbList,
                    &((PTCB)Adapter->TcbMemoryBlock)[index].TcbLink,
                    &Adapter->FreeTcbListLock);
        }

        //
        // Set up timers to simulate hardware interrupts (SendComplete and Recv)
        //

        NdisZeroMemory(&Timer, sizeof(Timer));

        {C_ASSERT(NDIS_SIZEOF_TIMER_CHARACTERISTICS_REVISION_1 <= sizeof(Timer));}
        Timer.Header.Type = NDIS_OBJECT_TYPE_TIMER_CHARACTERISTICS;
        Timer.Header.Size = NDIS_SIZEOF_TIMER_CHARACTERISTICS_REVISION_1;
        Timer.Header.Revision = NDIS_TIMER_CHARACTERISTICS_REVISION_1;

        Timer.TimerFunction = TXSendCompleteDpc;
        Timer.FunctionContext = Adapter;
        Timer.AllocationTag = NIC_TAG_TIMER;

        Status = NdisAllocateTimerObject(
                NdisDriverHandle,
                &Timer,
                &Adapter->SendCompleteTimer);
        if (Status != NDIS_STATUS_SUCCESS)
        {
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        Adapter->SendCompleteWorkItem = NdisAllocateIoWorkItem(MiniportAdapterHandle);
        if (Adapter->SendCompleteWorkItem == NULL)
        {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        //
        // Initialize DPC list data
        //
        NdisInitializeListHead(&Adapter->RecvDpcList);
        NdisAllocateSpinLock(&Adapter->RecvDpcListLock);

        //
        // Set up a timer function for use with our MPReset routine.
        //
        Timer.TimerFunction = NICAsyncResetOrPauseDpc;

        Status = NdisAllocateTimerObject(
                NdisDriverHandle,
                &Timer,
                &Adapter->AsyncBusyCheckTimer);
        if (Status != NDIS_STATUS_SUCCESS)
        {
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        //
        // The miniport adapter is powered up
        //
        Adapter->CurrentPowerState = NdisDeviceStateD0;

        //
        // Initialize and allocate the basic VMQ data for this adapter if supported. Queues, filters, shared memory,
        // and other data is allocated at a later time, when the relavant OID's are called to eable/configure them.
        //
        Status = AllocateVMQData(Adapter);
        if (Status != NDIS_STATUS_SUCCESS)
        {
            DEBUGP(MP_ERROR, "[%p] AllocateVMQData Status 0x%08x\n", Adapter, Status);
            Status = NDIS_STATUS_FAILURE;
            break;
        }

    } while(FALSE);


    *pAdapter = Adapter;

    //
    // In the failure case, the caller of this routine will end up
    // calling NICFreeAdapter to free all the successfully allocated
    // resources.
    //
    DEBUGP(MP_TRACE, "[%p] <--- NICAllocAdapter\n", Adapter);
    return Status;
}


void NICFreeAdapter(
    _In_  PMP_ADAPTER Adapter)
/*++
Routine Description:

    The NICFreeAdapter function frees memory used to track a miniport instance. Should only be called from MPHaltEx.

    NICFreeAdapter runs at IRQL = PASSIVE_LEVEL.

Arguments:

    Adapter                    Adapter memory to free.

    Return Value:

    NDIS_STATUS_xxx code

--*/
{
    PLIST_ENTRY pEntry;

    DEBUGP(MP_TRACE, "[%p] ---> NICFreeAdapter\n", Adapter);

    ASSERT(Adapter);

    //
    // Free all the resources we allocated in NICAllocAdapter.
    //

    if (Adapter->AsyncBusyCheckTimer)
    {
        NdisFreeTimerObject(Adapter->AsyncBusyCheckTimer);
        Adapter->AsyncBusyCheckTimer = NULL;
    }

    if(Adapter->SendCompleteTimer)
    {
        NdisFreeTimerObject(Adapter->SendCompleteTimer);
        Adapter->SendCompleteTimer = NULL;
    }

    if (Adapter->SendCompleteWorkItem)
    {
        ASSERT(!Adapter->SendCompleteWorkItemQueued);
        NdisFreeIoWorkItem(Adapter->SendCompleteWorkItem);
        Adapter->SendCompleteWorkItem = NULL;
    }

    //
    // If VMQ is enabled, then the global adapter RCB list will not have been allocated
    //
    if(!VMQ_ENABLED(Adapter))
    {
        if(Adapter->FreeRcbList.Flink)
        {
            while (NULL != (pEntry = NdisInterlockedRemoveHeadList(
                    &Adapter->FreeRcbList,
                    &Adapter->FreeRcbListLock)))
            {
                PRCB Rcb = CONTAINING_RECORD(pEntry, RCB, RcbLink);
                NdisFreeNetBufferList(Rcb->Nbl);
            }
        }

        if (Adapter->RecvNblPoolHandle)
        {
            NdisFreeNetBufferListPool(Adapter->RecvNblPoolHandle);
            Adapter->RecvNblPoolHandle = NULL;
        }

        if (Adapter->RcbMemoryBlock)
        {
            NdisFreeMemory(
                    Adapter->RcbMemoryBlock,
                    sizeof(RCB)*NIC_MAX_BUSY_RECVS,
                    0);
            Adapter->RcbMemoryBlock = NULL;
        }

        if(Adapter->DefaultRecvDpc)
        {
            //
            // For non VMQ receives, we've referenced the default queue DPC so we should reduce that count
            //
            NICReceiveDpcRemoveOwnership(Adapter->DefaultRecvDpc, 0);
            Adapter->DefaultRecvDpc = NULL;
        }

    }

    if (Adapter->TcbMemoryBlock)
    {
        NdisFreeMemory(
                Adapter->TcbMemoryBlock,
                sizeof(TCB)*NIC_MAX_BUSY_SENDS,
                0);
        Adapter->TcbMemoryBlock = NULL;
    }

    ASSERT(Adapter->SendWaitList.Flink && IsListEmpty(&Adapter->SendWaitList));
    ASSERT(Adapter->BusyTcbList.Flink && IsListEmpty(&Adapter->BusyTcbList));

    NdisFreeSpinLock(&Adapter->FreeTcbListLock);
    NdisFreeSpinLock(&Adapter->SendWaitListLock);
    NdisFreeSpinLock(&Adapter->BusyTcbListLock);
    NdisFreeSpinLock(&Adapter->FreeRcbListLock);

    //
    // Free any remaining VMQ related data
    //
    FreeVMQData(Adapter);

    //
    // Free receive DPCs
    //
    if(Adapter->RecvDpcList.Flink)
    {
        while (!IsListEmpty(&Adapter->RecvDpcList))
        {
            pEntry = RemoveHeadList(&Adapter->RecvDpcList);
            NICFreeReceiveDpc(CONTAINING_RECORD(pEntry, MP_ADAPTER_RECEIVE_DPC, Entry));
        }
    }
    NdisFreeSpinLock(&Adapter->RecvDpcListLock);

    //
    // Finally free the memory for adapter context.
    //
    NdisFreeMemory(Adapter->UnalignedAdapterBuffer, Adapter->UnalignedAdapterBufferSize, 0);

    DEBUGP(MP_TRACE, "[%p] <--- NICFreeAdapter\n", Adapter);
}

VOID
NICUpdateDPCMaxIndicateCount(
        _In_ PMP_ADAPTER_RECEIVE_DPC ReceiveDpc)
/*++
Routine Description:

    The NICUpdateDPCMaxIndicateCount function updates the maximum amount of NBLs to be indicated per block, based
    on the number of owned receive blocks.

Arguments:

    ReceiveDpc                  The receive DPC being modified
    BlockId                     ID of the receive block to add ownership

    Return Value:

        None
--*/
{
    if(ReceiveDpc->RecvBlockCount)
    {
        //
        // Update MaxNblCountPerIndicate. Scale back the amount of NBL indications we're allowed to do per
        // consumed receive block so that we don't sepnd too much time in the DPC as the number of queues grows large.
        //
        ReceiveDpc->MaxNblCountPerIndicate = NIC_MAX_RECVS_PER_DPC/ReceiveDpc->RecvBlockCount;
    }

}

VOID
NICReceiveDpcSetOwnership(
    _In_ PMP_ADAPTER_RECEIVE_DPC ReceiveDpc,
    _In_ _In_range_(0, NIC_SUPPORTED_NUM_QUEUES-1) ULONG BlockId)
/*++
Routine Description:

    The NICReceiveDpcSetOwnership function updates the Receive DPC passed in so that it consumes the receive queue of the
    passed in receive block ID. This involves updating the boolean array, block count, and updating the maximum
    amount of NBLs to be indicated per block.

Arguments:

    ReceiveDpc                  The receive DPC being modified
    BlockId                     ID of the receive block to add ownership

    Return Value:

    None
--*/
{
    ASSERT(BlockId <  NIC_SUPPORTED_NUM_QUEUES);

    //
    // If the DPC is not already set to consume receives for this QueueId, increase
    // the reference count and set it to be consumed.
    //
    if(!ReceiveDpc->RecvBlock[BlockId])
    {
        InterlockedIncrement(&ReceiveDpc->RecvBlockCount);
        ReceiveDpc->RecvBlock[BlockId]=TRUE;
        NICUpdateDPCMaxIndicateCount(ReceiveDpc);
    }
}

VOID
NICReceiveDpcRemoveOwnership(
    _In_ PMP_ADAPTER_RECEIVE_DPC ReceiveDpc,
    _In_ _In_range_(0, NIC_SUPPORTED_NUM_QUEUES-1) ULONG BlockId)
/*++
Routine Description:

    The NICReceiveDpcRemoveOwnership function updates the Receive DPC passed in so that it stops consuming the receive
    queue of the passed in receive block ID. This involves updating the boolean array, block count, and updating the maximum
    amount of NBLs to be indicated per block.

Arguments:

    ReceiveDpc                  The receive DPC being modified
    ReceiveBlockId              ID of the receive block to remove ownership

    Return Value:

    None
--*/
{
    ASSERT(BlockId <  NIC_SUPPORTED_NUM_QUEUES);

    //
    // If the DPC is set to consume receives for this QueueId, decrease
    // the reference count and set it to not be consumed.
    //
    if(ReceiveDpc->RecvBlock[BlockId])
    {
        InterlockedDecrement(&ReceiveDpc->RecvBlockCount);
        ASSERT(ReceiveDpc->RecvBlockCount>=0);
        ReceiveDpc->RecvBlock[BlockId]=FALSE;
        NICUpdateDPCMaxIndicateCount(ReceiveDpc);
    }
}

PMP_ADAPTER_RECEIVE_DPC
NICAllocReceiveDpc(
    _In_ PMP_ADAPTER      Adapter,
    ULONG ProcessorNumber,
    USHORT ProcessorGroup,
    _In_ _In_range_(0, NIC_SUPPORTED_NUM_QUEUES-1) ULONG BlockId)
/*++
Routine Description:

    The NICAllocReceiveDpc function allocates a receive DPC with the specified processor characteristics
    for the target QueueId. If the adapter has already allocated a DPC that matches the processor requirements,
    the existing DPC is reused and given ownership of consuming the QueueId's receives.

    NICAllocReceiveDpc runs at IRQL = PASSIVE_LEVEL.

Arguments:

    Adapter                     Pointer to the adapter that will own the DPC.
    ProcessorNumber             Target processor for DPC
    ProcessorGroup              Target processor group for the DPC (if not Win7, set to 0)
    QueueId                     Queue whose receives the DPC should consume (0 for non-VMQ scenarios)

    Return Value:

    PMP_ADAPTER_RECEIVE_DPC structure on success.
    NULL on failure.

--*/
{
    PMP_ADAPTER_RECEIVE_DPC ReceiveDpc=NULL, ExistingDpc=NULL;
    PLIST_ENTRY ReceiveListEntry;
    NTSTATUS Status = STATUS_SUCCESS;

    ASSERT(BlockId <  NIC_SUPPORTED_NUM_QUEUES);

    do
    {
        ReceiveDpc = NdisAllocateMemoryWithTagPriority(
                                Adapter->AdapterHandle,
                                sizeof(MP_ADAPTER_RECEIVE_DPC),
                                NIC_TAG_DPC,
                                NormalPoolPriority);
        if(!ReceiveDpc)
        {
            DEBUGP(MP_ERROR, "[%p] Could not allocate memory for receive DPC.\n", Adapter);
            break;
        }
        NdisZeroMemory(ReceiveDpc, sizeof(MP_ADAPTER_RECEIVE_DPC));

        ReceiveDpc->Adapter = Adapter;

        //
        // Initialize DPC data (entry, DPC function, target processor)
        //
        NdisInitializeListHead(&ReceiveDpc->Entry);
        KeInitializeDpc(&ReceiveDpc->Dpc, RXReceiveIndicateDpc, Adapter);

        //
        // Allocate the work item that's used if we're close to the DPC watchdog timer limit
        //
        ReceiveDpc->WorkItem = NdisAllocateIoWorkItem(Adapter->AdapterHandle);
        if(!ReceiveDpc->WorkItem)
        {
            DEBUGP(MP_ERROR, "[%p] Could not allocate work item for receive DPC.\n", Adapter);
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        //
        // Make sure the target DPC list starts getting processed as soon as it's queued even if was queued from
        // another processor.
        //
        KeSetImportanceDpc(&ReceiveDpc->Dpc, MediumHighImportance);

#if (NDIS_SUPPORT_NDIS620)
        {
            //
            // Use Win7 APIs to set the target processor
            //
            PROCESSOR_NUMBER DpcProcessor = {0};
            DpcProcessor.Number = (UCHAR)ProcessorNumber;
            DpcProcessor.Group = ProcessorGroup;
            Status = KeSetTargetProcessorDpcEx(&ReceiveDpc->Dpc, &DpcProcessor);
        }
#else
        //
        // Use Vista APIs to set target processor
        //
        KeSetTargetProcessorDpc(&ReceiveDpc->Dpc, (CCHAR)ProcessorNumber);
#endif
        if(!NT_SUCCESS(Status))
        {
            DEBUGP(MP_ERROR, "[%p] Failed setting KeSetTargetProcessorDpcEx. Status: 0x%08x\n", Adapter, Status);
            break;
        }
        ReceiveDpc->ProcessorGroup = ProcessorGroup;
        ReceiveDpc->ProcessorNumber = ProcessorNumber;

        //
        // Check if it's already on the list, if it is, return existing Dpc and free this one
        // otherwise, add to the adapter's DPC list.
        //
        NdisAcquireSpinLock(&Adapter->RecvDpcListLock);

        for(ReceiveListEntry = Adapter->RecvDpcList.Flink;
             ReceiveListEntry != &Adapter->RecvDpcList;
             ExistingDpc = NULL, ReceiveListEntry = ReceiveListEntry->Flink)
        {
            ExistingDpc = CONTAINING_RECORD(ReceiveListEntry, MP_ADAPTER_RECEIVE_DPC, Entry);
            if(ExistingDpc->ProcessorNumber == ProcessorNumber && ExistingDpc->ProcessorGroup == ProcessorGroup)
            {
                //
                // A DPC exists with matching affinity information. Break so that we don't NULL ExistingDpc
                //
                break;
            }
        }

        if(ExistingDpc)
        {
            //
            // Use existing DPC
            //
            DEBUGP(MP_TRACE, "[%p] Reusing existing DPC which matches affinity.\n", Adapter);
            NICFreeReceiveDpc(ReceiveDpc);
            ReceiveDpc = ExistingDpc;

        }
        else
        {
            //
            // Add new DPC to adapter's list
            //
            InsertTailList(&Adapter->RecvDpcList, &ReceiveDpc->Entry);
        }

        NdisReleaseSpinLock(&Adapter->RecvDpcListLock);

        NICReceiveDpcSetOwnership(ReceiveDpc,BlockId);

    }while(FALSE);

    if(!NT_SUCCESS(Status) && ReceiveDpc)
    {
        NICFreeReceiveDpc(ReceiveDpc);
        ReceiveDpc = NULL;
    }

    return ReceiveDpc;
}

VOID
NICFreeReceiveDpc(
    _In_ PMP_ADAPTER_RECEIVE_DPC AdapterDpc)
/*++
Routine Description:

    The NICFreeReceiveDpc function frees the passed in DPC. Should only be called when the DPC is not active (not queued, flushed).

Arguments:

    AdapterDpc                  DPC to free

    Return Value:

    None

--*/
{
    ASSERT(AdapterDpc->RecvBlockCount==0);
    //
    // Free DPC dymainc fields and memory
    //
    if(AdapterDpc->WorkItem)
    {
         NdisFreeIoWorkItem(AdapterDpc->WorkItem);
    }
    NdisFreeMemory(AdapterDpc, sizeof(MP_ADAPTER_RECEIVE_DPC), 0);
}

PMP_ADAPTER_RECEIVE_DPC
NICGetDefaultReceiveDpc(
    _In_ PMP_ADAPTER      Adapter,
    _In_ _In_range_(0, NIC_SUPPORTED_NUM_QUEUES-1) ULONG BlockId)
/*++
Routine Description:

    The NICGetDefaultReceiveDpc function returns the default receive DPC for the adapter, and sets it to consume
    receives from the passed in QueueId.


Arguments:

    Adapter                     Pointer to the adapter that will own the DPC.
    QueueId                     Queue whose receives the DPC should consume (0 for non-VMQ scenarios)

    Return Value:

    PMP_ADAPTER_RECEIVE_DPC structure.

--*/
{
    ASSERT(BlockId <  NIC_SUPPORTED_NUM_QUEUES);

    if(VMQ_ENABLED(Adapter))
    {
        NdisAcquireSpinLock(&Adapter->RecvDpcListLock);
        NICReceiveDpcSetOwnership(Adapter->DefaultRecvDpc,BlockId);
        NdisReleaseSpinLock(&Adapter->RecvDpcListLock);
    }
    else
    {
        UNREFERENCED_PARAMETER(BlockId);
    }

    return Adapter->DefaultRecvDpc;
}

NDIS_STATUS
    NICAllocRCBData(
    _In_ PMP_ADAPTER Adapter,
    ULONG NumberOfRcbs,
    _Outptr_result_bytebuffer_(NumberOfRcbs * sizeof(RCB)) PVOID *RcbMemoryBlock,
    _Inout_ PLIST_ENTRY FreeRcbList,
    _Inout_ PNDIS_SPIN_LOCK FreeRcbListLock,
    _Inout_ PNDIS_HANDLE RecvNblPoolHandle
    )
/*++
Routine Description:

    The NICAllocRCBData function allocated NumberOfRcbs worth of RCB and NBL memory for use in receive indication,
    and populates the passed in FreeRcbList with this data.

    IRQL = PASSIVE_LEVEL

Arguments:

    Adapter                     Pointer to our adapter
    NumberOfRcbs                Number of RCB structures to allocate (and NBLs as a result)
    RcbMemoryBlock              Receives the allocated memory block that is split up into each individual RCB
    FreeRcbList                 Initialized blank list to be populated with each individual RCB.
    FreeRcbListLock             Initialized lock used when updating the RCB list (and subsequent consumers should use lock).
    RcbNblPoolHandle            Receives the Ndis NBL pool handle for the allocated NBLs (to be used on free).

    Return Value:

    NDIS_STATUS_xxx code

--*/
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    NET_BUFFER_LIST_POOL_PARAMETERS NblParameters;
    ULONG index;

    do
    {
        //
        // Allocate an NBL pool for receive indications.
        //

        {C_ASSERT(sizeof(NblParameters) >= NDIS_SIZEOF_NET_BUFFER_LIST_POOL_PARAMETERS_REVISION_1);}
        NblParameters.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
        NblParameters.Header.Size = NDIS_SIZEOF_NET_BUFFER_LIST_POOL_PARAMETERS_REVISION_1;
        NblParameters.Header.Revision = NET_BUFFER_LIST_POOL_PARAMETERS_REVISION_1;

        NblParameters.ProtocolId = NDIS_PROTOCOL_ID_DEFAULT; // always use DEFAULT for miniport drivers
        NblParameters.fAllocateNetBuffer = TRUE;
        NblParameters.ContextSize = 0;
        NblParameters.PoolTag = NIC_TAG_RECV_NBL;
        NblParameters.DataSize = 0;

        *RecvNblPoolHandle = NdisAllocateNetBufferListPool(
                NdisDriverHandle,
                &NblParameters);
        if (!*RecvNblPoolHandle)
        {
            Status = NDIS_STATUS_RESOURCES;
            DEBUGP(MP_ERROR, "[%p] NdisAllocateNetBufferListPool failed\n", Adapter);
            break;
        }

        //
        // Allocate receive memory block
        //
        *RcbMemoryBlock = NdisAllocateMemoryWithTagPriority(
                Adapter->AdapterHandle,
                sizeof(RCB) * NumberOfRcbs,
                NIC_TAG_RCB,
                NormalPoolPriority);

        if (!*RcbMemoryBlock)
        {
            Status = NDIS_STATUS_RESOURCES;
            DEBUGP(MP_ERROR, "[%p] NdisAllocateMemoryWithTagPriority failed\n", Adapter);
            break;
        }
        NdisZeroMemory(*RcbMemoryBlock, sizeof(RCB) * NumberOfRcbs);

        //
        // Split into individual RCBs, allocate NBL, and add to free list
        //
        for (index = 0; index < NumberOfRcbs; index++)
        {
            PRCB Rcb = &((PRCB)*RcbMemoryBlock)[index];

            //
            // Allocate an NBL with its single NET_BUFFER from the preallocated
            // pool.
            //
            Rcb->Nbl = NdisAllocateNetBufferAndNetBufferList(
                    *RecvNblPoolHandle,
                    0,     // ContextSize
                    0,     // ContextBackfill
                    NULL,  // MdlChain
                    0,     // DataOffset
                    0);    // DataLength
            if (Rcb->Nbl == NULL)
            {
                Status = NDIS_STATUS_RESOURCES;
                DEBUGP(MP_ERROR, "[%p] NdisAllocateNetBufferAndNetBufferList failed\n", Adapter);
                break;
            }

            //
            // Add RCB pointer to miniport reserved portion of NBL
            //
            RCB_FROM_NBL(Rcb->Nbl) = Rcb;

            NdisInterlockedInsertTailList(
                    FreeRcbList,
                    &Rcb->RcbLink,
                    FreeRcbListLock);
        }

    }while(FALSE);

    return Status;
}

NDIS_STATUS
NICReadRegParameters(
    _In_  PMP_ADAPTER Adapter)
/*++
Routine Description:

    Read device configuration parameters from the registry

Arguments:

    Adapter                         Pointer to our adapter
    WrapperConfigurationContext     For use by NdisOpenConfiguration

    Should be called at IRQL = PASSIVE_LEVEL.

Return Value:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_FAILURE
    NDIS_STATUS_RESOURCES

--*/
{
    NDIS_STATUS                   Status = NDIS_STATUS_SUCCESS;

    NDIS_CONFIGURATION_OBJECT     ConfigurationParameters;
    NDIS_HANDLE                   ConfigurationHandle;

    DEBUGP(MP_TRACE, "[%p] ---> NICReadRegParameters\n", Adapter);

    PAGED_CODE();

    //
    // Open the registry for this adapter to read advanced
    // configuration parameters stored by the INF file.
    //

    NdisZeroMemory(&ConfigurationParameters, sizeof(ConfigurationParameters));

    {C_ASSERT(sizeof(ConfigurationParameters) >= NDIS_SIZEOF_CONFIGURATION_OBJECT_REVISION_1);}
    ConfigurationParameters.Header.Type = NDIS_OBJECT_TYPE_CONFIGURATION_OBJECT;
    ConfigurationParameters.Header.Size = NDIS_SIZEOF_CONFIGURATION_OBJECT_REVISION_1;
    ConfigurationParameters.Header.Revision = NDIS_CONFIGURATION_OBJECT_REVISION_1;

    ConfigurationParameters.NdisHandle = Adapter->AdapterHandle;
    ConfigurationParameters.Flags = 0;

    Status = NdisOpenConfigurationEx(
            &ConfigurationParameters,
            &ConfigurationHandle);
    if(Status != NDIS_STATUS_SUCCESS)
    {
        DEBUGP(MP_ERROR, "[%p] NdisOpenConfigurationEx Status = 0x%08x\n", Adapter, Status);
        return NDIS_STATUS_FAILURE;
    }


    //
    // Read all of our configuration parameters using NdisReadConfiguration
    // and parse the value.
    //
    NICSetMacAddress(Adapter, ConfigurationHandle);

    Adapter->ulLinkSendSpeed = NIC_XMIT_SPEED;
    Adapter->ulLinkRecvSpeed = NIC_RECV_SPEED;

    //
    // Read VMQ related configuration parameters
    //
    Status = ReadRxQueueConfig(ConfigurationHandle, Adapter);
    if(Status != NDIS_STATUS_SUCCESS)
    {
        DEBUGP(MP_ERROR, "[%p] ReadRxQueueConfig Status = 0x%08x\n", Adapter, Status);
        Status = NDIS_STATUS_FAILURE;
        goto Exit;
    }

    //
    // Read NDIS QOS related configuration parameters
    //
    Status = ReadQOSConfig(ConfigurationHandle, Adapter);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        DEBUGP(MP_ERROR, "[%p] ReadQOSConfig Status = 0x%08x\n", Adapter, Status);
        Status = NDIS_STATUS_FAILURE;
        goto Exit;
    }

Exit:
    //
    // Close the configuration registry
    //
    NdisCloseConfiguration(ConfigurationHandle);

    DEBUGP(MP_TRACE, "[%p] <--- NICReadRegParameters Status = 0x%08x\n", Adapter, Status);
    return Status;
}


VOID
NICSetMacAddress(
    _In_  PMP_ADAPTER  Adapter,
    _In_  NDIS_HANDLE  ConfigurationHandle)
/*++
Routine Description:

    Configures the NIC with the correct permanent and current MAC addresses.
    If no permanent address is saved, generate a new one.

    IRQL = PASSIVE_LEVEL

Arguments:

    Adapter                     Pointer to our adapter
    ConfigurationHandle         NIC configuration from NdisOpenConfigurationEx

Return Value:

    None.

--*/
{
    NDIS_STATUS                   Status;
    PUCHAR                        NetworkAddress;
    UINT                          Length = 0;


    PAGED_CODE();


    HWReadPermanentMacAddress(
            Adapter,
            ConfigurationHandle,

            Adapter->PermanentAddress);


    //
    // Now seed the current MAC address with the permanent address.
    //
    NIC_COPY_ADDRESS(Adapter->CurrentAddress, Adapter->PermanentAddress);


    //
    // Read NetworkAddress registry value and use it as the current address
    // if there is a software configurable NetworkAddress specified in
    // the registry.
    //
    NdisReadNetworkAddress(
            &Status,
            &NetworkAddress,
            &Length,
            ConfigurationHandle);

    if ((Status == NDIS_STATUS_SUCCESS) && (Length == NIC_MACADDR_SIZE))
    {
        if ((NIC_ADDR_IS_MULTICAST(NetworkAddress)
                || NIC_ADDR_IS_BROADCAST(NetworkAddress))
                || !NIC_ADDR_IS_LOCALLY_ADMINISTERED(NetworkAddress))
        {
            DEBUGP(MP_ERROR, "[%p] Overriding NetworkAddress is invalid: ", Adapter);
            DbgPrintAddress(NetworkAddress);
        }
        else
        {
            NIC_COPY_ADDRESS(Adapter->CurrentAddress, NetworkAddress);
        }
    }

    DEBUGP(MP_LOUD, "[%p] Permanent Address = ", Adapter);
    DbgPrintAddress(Adapter->PermanentAddress);

    DEBUGP(MP_LOUD, "[%p] Current Address = ", Adapter);
    DbgPrintAddress(Adapter->CurrentAddress);
}


BOOLEAN
NICIsBusy(
    _In_  PMP_ADAPTER  Adapter)
/*++
Routine Description:

    The NICIsBusy function returns whether the NIC has pending receives or sends. Before calling this
    function, the NIC should be in a state where it will not receive new sends/receives (either datapath
    is stopped, or !MP_IS_READY). Otherwise, the NIC may create new pending receives and sends after the
    NICIsBusy call.

Arguments:

    Adapter                     Pointer to our adapter

Return Value:

    TRUE: Send or Receive path is not idle.
    FALSE: Send & Receive paths are idle.

--*/
{
    BOOLEAN fBusy = FALSE;
    DEBUGP(MP_TRACE, "[%p] ---> NICIsBusy\n", Adapter);

#if DBG
    //
    // Check whether we might get new sends/receives
    //
    if(MPIsAdapterAttached(Adapter))
    {
        //
        // Adapter is still attached to the datapath, so it should not be ready (otherwise receives could get queued after we've read the counters)
        //
        ASSERT(!MP_IS_READY(Adapter));
    }
#endif

    //
    // Check if all the NBLs that the protocol sent down for transmission have
    // been completed yet.
    //
    if (Adapter->nBusySend)
    {
        DEBUGP(MP_INFO, "[%p] Send path is not idle, nBusySend = %d", Adapter, Adapter->nBusySend);
        fBusy = TRUE;
    }
    else
    {
        //
        // Check if all of our NBLs that we indicated up to the protocol have
        // returned to us yet.
        //
        USHORT ReceiveBlockId = 0;
        for(;ReceiveBlockId<NIC_SUPPORTED_NUM_QUEUES; ++ReceiveBlockId)
        {
            if(RECEIVE_BLOCK_IS_BUSY(Adapter, ReceiveBlockId))
            {
                DEBUGP(MP_INFO, "[%p] Recv block %i not idle, nBusyRecv = %u\n", Adapter, ReceiveBlockId, Adapter->ReceiveBlock[ReceiveBlockId].PendingReceives);
                fBusy = TRUE;
                break;
            }
        }
    }

    DEBUGP(MP_TRACE, "[%p] <--- NICIsBusy fBusy = %u\n", Adapter, (UINT)fBusy);
    return fBusy;
}


NDIS_STATUS
NICInitializeReceiveBlock(
    _In_ PMP_ADAPTER Adapter,
    _In_ _In_range_(0, NIC_SUPPORTED_NUM_QUEUES-1) ULONG BlockId)
/*++
Routine Description:

    The NICInitializeReceiveBlock function initializes the fields of an adapter receive block. This block
    is consumed by the owning receive DPC.


Arguments:

    Adapter      - Pointer to our Adapter
    BlockIndex   - Receive block to initialize

Return Value:

     NDIS_STATUS_xxx code.

--*/
{
    DEBUGP(MP_TRACE, "[%p] ---> NICInitializeReceiveBlock. Index: %i\n", Adapter, BlockId);
    ASSERT(BlockId <  NIC_SUPPORTED_NUM_QUEUES);

    NdisZeroMemory(&Adapter->ReceiveBlock[BlockId], sizeof(MP_ADAPTER_RECEIVE_BLOCK));
    NdisInitializeListHead(&Adapter->ReceiveBlock[BlockId].ReceiveList);
    NdisAllocateSpinLock(&Adapter->ReceiveBlock[BlockId].ReceiveListLock);

    DEBUGP(MP_TRACE, "[%p] <--- NICInitializeReceiveBlock\n", Adapter);

    return NDIS_STATUS_SUCCESS;

}

VOID
NICFlushReceiveBlock(
    _In_ PMP_ADAPTER Adapter,
    _In_ _In_range_(0, NIC_SUPPORTED_NUM_QUEUES-1) ULONG BlockId)
/*++
Routine Description:

    This routine flushes all pending receives for a specific adapter receive block.

Arguments:

    Adapter              - Pointer to our adapter
    BlockIndex           - Receive block to flush
Return Value:

    None

--*/
{
    PLIST_ENTRY Entry;
    PMP_ADAPTER_RECEIVE_BLOCK ReceiveBlock;

    DEBUGP(MP_TRACE, "[%p] ---> NICFlushReceiveBlock. QueueId: %i\n", Adapter, BlockId);
    ASSERT(BlockId <  NIC_SUPPORTED_NUM_QUEUES);
    ReceiveBlock = &Adapter->ReceiveBlock[BlockId];

    //
    // Return any pending receives on the block
    //
    for(Entry = NdisInterlockedRemoveHeadList(&ReceiveBlock->ReceiveList, &ReceiveBlock->ReceiveListLock);
        Entry;
        Entry = NdisInterlockedRemoveHeadList(&ReceiveBlock->ReceiveList, &ReceiveBlock->ReceiveListLock))
    {
        ReturnRCB(Adapter, CONTAINING_RECORD(Entry, RCB, RcbLink));
    }

    DEBUGP(MP_TRACE, "[%p] <-- NICFlushReceiveBlock\n", Adapter);

}

NDIS_STATUS
NICReferenceReceiveBlock(
    _In_ PMP_ADAPTER Adapter,
    _In_ _In_range_(0, NIC_SUPPORTED_NUM_QUEUES-1) ULONG BlockId)
/*++
Routine Description:

    This routine increments the pending receive count on a adapter receive block. This count is used
    throughout the code to determine whether there are pending operations that will use the adapter's resources such as
    receive buffers (which would pend operations such as adapter reset, VMQ queue free, etc...).

Arguments:

    Adapter              - Pointer to our adapter
    BlockIndex           - Receive block to flush

Return Value:

    NDIS_STATUS_SUCCESS if reference was acquired succesfully.
    NDIS_STATUS_ADAPTER_NOT_READY if the adapter state is such that we should not acquire new references to resources

--*/
{
    //
    // Increment the reference count before checking NIC state avoid race conditions with code that checks reference count. The
    // reference will be undone if the adapter is not ready.
    //
    ULONG RefCount = InterlockedIncrement(&Adapter->ReceiveBlock[BlockId].PendingReceives);

    //
    // Make sure the increment happens before ready state check
    //
    KeMemoryBarrier();

    //
    // If the adapter is not ready, undo the reference and fail the call
    //
    if(!MP_IS_READY(Adapter))
    {
        InterlockedDecrement(&Adapter->ReceiveBlock[BlockId].PendingReceives);
        DEBUGP(MP_LOUD, "[%p] Could not acquire reference for ReceiveBlock %i. The adapter is not ready.\n", Adapter, BlockId);
        return NDIS_STATUS_ADAPTER_NOT_READY;
    }

    DEBUGP(MP_LOUD, "[%p] Acquired reference for ReceiveBlock %i. RefCount: %i.\n", Adapter, BlockId, RefCount);

    return NDIS_STATUS_SUCCESS;
}

VOID
NICDereferenceReceiveBlock(
    _In_ PMP_ADAPTER Adapter,
    _In_ _In_range_(0, NIC_SUPPORTED_NUM_QUEUES-1) ULONG BlockId,
    _Out_opt_ ULONG *RefCount)
/*++
Routine Description:

    This routine decrements the pending receive count on a adapter receive block. This count is used
    throughout the code to determine whether there are pending operations that will use the adapter's resources such as
    receive buffers (which would pend operations such as adapter reset, VMQ queue free, etc...).

Arguments:

    Adapter              - Pointer to our adapter
    BlockIndex           - Receive block to flush

Return Value:

    None

--*/
{
    ULONG Count = InterlockedDecrement(&Adapter->ReceiveBlock[BlockId].PendingReceives);
    DEBUGP(MP_LOUD, "[%p] Released reference for ReceiveBlock %i. RefCount: %i.\n", Adapter, BlockId, Count);
    if(RefCount)
    {
        *RefCount = Count;
    }
}