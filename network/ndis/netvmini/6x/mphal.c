/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

   MpHAL.C

Abstract:

    This module implements the adapter's hardware.

--*/


#include "netvmin6.h"
#include "mphal.tmh"

//
// This registry value saves the permanent MAC address of a netvmini NIC.  It is
// only needed because there's no hardware that keeps track of the permanent
// address across reboots.
//
// It is saved as a NdisParameterBinary configuration value (REG_BINARY) with
// length NIC_MACADDR_LEN (6 bytes).
//
#define NETVMINI_MAC_ADDRESS_KEY L"NetvminiMacAddress"


static
NDIS_STATUS
HWCopyBytesFromNetBuffer(
    _In_     PNET_BUFFER        NetBuffer,
    _Inout_  PULONG             cbDest,
    _Out_writes_bytes_to_(*cbDest, *cbDest) PVOID Dest);


#pragma NDIS_PAGEABLE_FUNCTION(HWInitialize)
#pragma NDIS_PAGEABLE_FUNCTION(HWReadPermanentMacAddress)




NDIS_STATUS
HWInitialize(
    _In_  PMP_ADAPTER                     Adapter,
    _In_  PNDIS_MINIPORT_INIT_PARAMETERS  InitParameters)
/*++
Routine Description:

    Query assigned resources and initialize the adapter.

Arguments:

    Adapter                     Pointer to our adapter
    InitParameters              Parameters to MiniportInitializeEx

Return Value:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_ADAPTER_NOT_FOUND

--*/
{
    NDIS_STATUS                     Status = NDIS_STATUS_ADAPTER_NOT_FOUND;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR pResDesc;
    ULONG                           index;

    UNREFERENCED_PARAMETER(Adapter);

    DEBUGP(MP_TRACE, "[%p] ---> HWInitialize\n", Adapter);
    PAGED_CODE();


    do
    {
        if (InitParameters->AllocatedResources)
        {
            for (index=0; index < InitParameters->AllocatedResources->Count; index++)
            {
                pResDesc = &InitParameters->AllocatedResources->PartialDescriptors[index];

                switch (pResDesc->Type)
                {
                    case CmResourceTypePort:
                        DEBUGP(MP_INFO, "[%p] IoBaseAddress = 0x%x\n", Adapter,
                                NdisGetPhysicalAddressLow(pResDesc->u.Port.Start));
                        DEBUGP(MP_INFO, "[%p] IoRange = x%x\n", Adapter, 
                                pResDesc->u.Port.Length);
                        break;

                    case CmResourceTypeInterrupt:
                        DEBUGP(MP_INFO, "[%p] InterruptLevel = x%x\n", Adapter,
                                pResDesc->u.Interrupt.Level);
                        break;

                    case CmResourceTypeMemory:
                        DEBUGP(MP_INFO, "[%p] MemPhysAddress(Low) = 0x%0x\n", Adapter,
                                NdisGetPhysicalAddressLow(pResDesc->u.Memory.Start));
                        DEBUGP(MP_INFO, "[%p] MemPhysAddress(High) = 0x%0x\n", Adapter,
                                NdisGetPhysicalAddressHigh(pResDesc->u.Memory.Start));
                        break;
                }
            }
        }

        Status = NDIS_STATUS_SUCCESS;

        //
        // Map bus-relative IO range to system IO space using
        // NdisMRegisterIoPortRange
        //

        //
        // Map bus-relative registers to virtual system-space
        // using NdisMMapIoSpace
        //


        //
        // Disable interrupts here as soon as possible
        //

        //
        // Register the interrupt using NdisMRegisterInterruptEx
        //

        //
        // Initialize the hardware with mapped resources
        //

        //
        // Enable the interrupt
        //

    } while (FALSE);


    DEBUGP(MP_TRACE, "[%p] <--- HWInitialize Status = 0x%x\n", Adapter, Status);
    return Status;
}

VOID
HWReadPermanentMacAddress(
    _In_  PMP_ADAPTER  Adapter,
    _In_  NDIS_HANDLE  ConfigurationHandle,
    _Out_writes_bytes_(NIC_MACADDR_SIZE)  PUCHAR  PermanentMacAddress)
/*++

Routine Description:

    Loads the permanent MAC address that is burnt into the NIC.

    IRQL = PASSIVE_LEVEL

Arguments:

    Adapter                     Pointer to our adapter
    ConfigurationHandle         NIC configuration from NdisOpenConfigurationEx
    PermanentMacAddress         On return, receives the NIC's MAC address

Return Value:

    None.

--*/
{
    NDIS_STATUS                   Status;
    PNDIS_CONFIGURATION_PARAMETER Parameter = NULL;
    NDIS_STRING                   PermanentAddressKey = RTL_CONSTANT_STRING(NETVMINI_MAC_ADDRESS_KEY);
 
    UNREFERENCED_PARAMETER(Adapter);
    PAGED_CODE();

    //
    // We want to figure out what the NIC's physical address is.
    // If we had a hardware NIC, we would query the physical address from it.
    // Instead, for the purposes of this sample, we'll read it from the
    // registry.  This will help us keep the permanent address constant, even
    // if the adapter is disabled/enabled.
    //
    // Note that the registry value that saves our permanent MAC address isn't
    // the one that end-users can configure through the NIC management GUI,
    // nor is it expected that other miniports would need to use a parameter
    // like this.  We only have it to work around the lack of physical hardware.
    //
    NdisReadConfiguration(
            &Status,
            &Parameter,
            ConfigurationHandle,
            &PermanentAddressKey,
            NdisParameterBinary);
    if (Status == NDIS_STATUS_SUCCESS
            && Parameter->ParameterType == NdisParameterBinary
            && Parameter->ParameterData.BinaryData.Length == NIC_MACADDR_SIZE)
    {
        //
        // There is a permanent address stashed in the special netvmini
        // parameter.
        //
        NIC_COPY_ADDRESS(PermanentMacAddress, Parameter->ParameterData.BinaryData.Buffer);
    }
    else
    {
        NDIS_CONFIGURATION_PARAMETER NewPhysicalAddress;
        LARGE_INTEGER TickCountValue;
        UCHAR CurrentMacIndex = 3;

        //
        // There is no (valid) address stashed in the netvmini parameter, so
        // this is probably the first time we've loaded this adapter before.
        //
        // Just for testing purposes, let us make up a dummy mac address.
        // In order to avoid conflicts with MAC addresses, it is usually a good
        // idea to check the IEEE OUI list (e.g. at
        // http://standards.ieee.org/regauth/oui/oui.txt). According to that
        // list 00-50-F2 is owned by Microsoft.
        //
        // An important rule to "generating" MAC addresses is to have the
        // "locally administered bit" set in the address, which is bit 0x02 for
        // LSB-type networks like Ethernet. Also make sure to never set the
        // multicast bit in any MAC address: bit 0x01 in LSB networks.
        //

        {C_ASSERT(NIC_MACADDR_SIZE > 3);}
        NdisZeroMemory(PermanentMacAddress, NIC_MACADDR_SIZE);
        PermanentMacAddress[0] = 0x02;
        PermanentMacAddress[1] = 0x50;
        PermanentMacAddress[2] = 0xF2;

        //
        // Generated value based on the current tick count value. 
        //
        KeQueryTickCount(&TickCountValue);
        do
        {
            //
            // Pick up the value in groups of 8 bits to populate the rest of the MAC address.
            //
            PermanentMacAddress[CurrentMacIndex] = (UCHAR)(TickCountValue.LowPart>>((CurrentMacIndex-3)*8));
        } while(++CurrentMacIndex < NIC_MACADDR_SIZE);

        //
        // Finally, we should make a best-effort attempt to save this address
        // to our configuration, so the NIC will always come up with this
        // permanent address.
        //

        NewPhysicalAddress.ParameterType = NdisParameterBinary;
        NewPhysicalAddress.ParameterData.BinaryData.Length = NIC_MACADDR_SIZE;
        NewPhysicalAddress.ParameterData.BinaryData.Buffer = PermanentMacAddress;

        NdisWriteConfiguration(
                &Status,
                ConfigurationHandle,
                &PermanentAddressKey,
                &NewPhysicalAddress);
        if (NDIS_STATUS_SUCCESS != Status)
        {
            DEBUGP(MP_WARNING, "[%p] NdisWriteConfiguration failed to save the permanent MAC address", Adapter);
            // No other handling -- this isn't a fatal error
        }
    }
}



_IRQL_requires_same_
_Function_class_(ALLOCATE_FUNCTION)
PVOID
HWFrameAllocate (
    _In_ POOL_TYPE PoolType,
    _In_ SIZE_T NumberOfBytes,
    _In_ ULONG Tag)
/*++

Routine Description:

    This routine allocates memory for a FRAME.  It is an ALLOCATE_FUNCTION, so
    its parameters and usage are the same as ExAllocatePoolWithTag.

    This allocator is meant to be used with the send NPAGED_LOOKASIDE_LIST.
    While you do not normally need to provide your own allocator (and in fact
    there is a performance penalty for doing so), we use the allocator to
    initialize the FRAME's MDL.  This saves us the effort of (re)initializing
    the same MDL every time we reuse the FRAME.

    Runs at IRQL <= DISPATCH_LEVEL

Arguments:

    PoolType                    Must be NonPagedPool, or NonPagedPoolNx for Win8
                                    and later
    NumberOfBytes               Must be sizeof(FRAME)
    Tag                         The pool allocation tag

Return Value:

    NULL if there are insufficient resources to allocate a FRAME.
    Else, a pointer to a newly-allocated FRAME.  Free it with HWFrameFree.

--*/
{
    PFRAME Frame = NULL;

    DEBUGP(MP_TRACE, "---> HWFrameAllocate\n");

    UNREFERENCED_PARAMETER(PoolType);
    ASSERT(NumberOfBytes == sizeof(FRAME));

    Frame = (PFRAME) NdisAllocateMemoryWithTagPriority(
            NdisDriverHandle,
            (UINT)NumberOfBytes,
            Tag,
            NormalPoolPriority);
    if (!Frame)
    {
        DEBUGP(MP_ERROR, "NdisAllocateMemoryWithTagPriority failed");
        return NULL;
    }
    NdisZeroMemory(Frame, NumberOfBytes);

    Frame->Mdl = NdisAllocateMdl(
            NdisDriverHandle,
            (PVOID)&Frame->Data[0],
            sizeof(Frame->Data));
    if (Frame->Mdl == NULL)
    {
        DEBUGP(MP_ERROR, "NdisAllocateMdl failed\n");
        NdisFreeMemory(Frame, (UINT)NumberOfBytes, 0);
        return NULL;
    }

    DEBUGP(MP_TRACE, "<--- HWFrameAllocate. Frame: %p\n", Frame);

    return Frame;
}


_Use_decl_annotations_
VOID
HWFrameFree (
    PVOID  Memory)
/*++

Routine Description:

    This routine frees memory for a FRAME.  It is a FREE_FUNCTION, the
    reciprocal of HWFrameAllocate.

    Runs at IRQL <= DISPATCH_LEVEL

Arguments:

    Memory                      A buffer allocated with HWFrameAllocate

Return Value:

    None.

--*/
{
    PFRAME Frame = (PFRAME) Memory;

    DEBUGP(MP_TRACE, "---> HWFrameFree. Frame: %p\n", Frame);

    ASSERT(Frame && Frame->Mdl);

    NdisFreeMdl(Frame->Mdl);
    NdisFreeMemory(Frame, sizeof(FRAME), 0);

    DEBUGP(MP_TRACE, "<--- HWFrameFree\n");

}


VOID
HWFrameReference(
    _In_  PFRAME  Frame)
/*++

Routine Description:

    This routine increments the reference count of a FRAME.

    Runs at IRQL <= DISPATCH_LEVEL

Arguments:

    Frame                       Frame to reference

Return Value:

    None.

--*/
{
    DEBUGP(MP_TRACE, "---> HWFrameReference. Frame: %p\n", Frame);

    NdisInterlockedIncrement(&Frame->Ref);

    DEBUGP(MP_TRACE, "<--- HWFrameReference. Frame: %p\n", Frame);

}


VOID
HWFrameRelease(
    _In_  PFRAME  Frame)
/*++

Routine Description:

    This routine decrements the reference count of a FRAME.  If the last
    refernce was released, the FRAME is freed back to the unused pool.

    Runs at IRQL <= DISPATCH_LEVEL

Arguments:

    Frame                       Frame to release

Return Value:

    None.

--*/
{
    DEBUGP(MP_TRACE, "---> HWFrameRelease. Frame: %p\n", Frame);

    if (0 == NdisInterlockedDecrement(&Frame->Ref))
    {
        DEBUGP(MP_TRACE, "---> Freeing Frame: %p\n", Frame);

        NdisFreeToNPagedLookasideList(&GlobalData.FrameDataLookaside, Frame);
        Frame = NULL;
    }

    DEBUGP(MP_TRACE, "<--- HWFrameRelease. Frame: %p\n", Frame);

}


NDIS_STATUS
HWCopyBytesFromNetBuffer(
    _In_     PNET_BUFFER        NetBuffer,
    _Inout_  PULONG             cbDest,
    _Out_writes_bytes_to_(*cbDest, *cbDest) PVOID Dest)
/*++

Routine Description:

    Copies the first cbDest bytes from a NET_BUFFER. In order to show how the various data structures fit together, this 
    implementation copies the data by iterating through the MDLs for the NET_BUFFER. The NdisGetDataBuffer API also allows you
    to copy a contiguous block of data from a NET_BUFFER. 

    Runs at IRQL <= DISPATCH_LEVEL.

Arguments:

    NetBuffer                   The NB to read
    cbDest                      On input, the number of bytes in the buffer Dest
                                On return, the number of bytes actually copied
    Dest                        On return, receives the first cbDest bytes of
                                the network frame in NetBuffer

Return Value:

    None.

Notes:

    If the output buffer is larger than the NB's frame size, *cbDest will
    contain the number of bytes in the frame size.

    If the output buffer is smaller than the NB's frame size, only the first
    *cbDest bytes will be copied (the buffer will receive a truncated copy of
    the frame).

--*/
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    //
    // Start copy from current MDL
    //
    PMDL CurrentMdl = NET_BUFFER_CURRENT_MDL(NetBuffer);
    //
    // Data on current MDL may be offset from start of MDL
    //
    ULONG DestOffset = 0;
    while (DestOffset < *cbDest && CurrentMdl)
    {
        //
        // Map MDL memory to System Address Space. LowPagePriority means mapping may fail if 
        // system is low on memory resources. 
        //
        PUCHAR SrcMemory = MmGetSystemAddressForMdlSafe(CurrentMdl, LowPagePriority | MdlMappingNoExecute);
        ULONG Length = MmGetMdlByteCount(CurrentMdl);
        if (!SrcMemory)
        {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        if(DestOffset==0)
        {
            //
            // The first MDL segment should be accessed from the current MDL offset
            //
            ULONG MdlOffset = NET_BUFFER_CURRENT_MDL_OFFSET(NetBuffer);
            SrcMemory += MdlOffset;
            Length -= MdlOffset;
        }

        Length = min(Length, *cbDest-DestOffset);

        //
        // Copy Memory
        //
        NdisMoveMemory((PUCHAR)Dest+DestOffset, SrcMemory, Length);
        DestOffset += Length;

        //
        // Get next MDL (if any available) 
        //
        CurrentMdl = NDIS_MDL_LINKAGE(CurrentMdl);
    }

    if(Status == NDIS_STATUS_SUCCESS)
    {
        *cbDest = DestOffset;
    }

    return Status;
}


NDIS_STATUS
HWGetDestinationAddress(
    _In_  PNET_BUFFER  NetBuffer,
    _Out_writes_bytes_(NIC_MACADDR_SIZE) PUCHAR DestAddress)
/*++

Routine Description:

    Returns the destination address of a NET_BUFFER that is to be sent.

    Runs at IRQL <= DISPATCH_LEVEL.

Arguments:

    NetBuffer                   The NB containing the frame that is being sent
    DestAddress                 On return, receives the frame's destination

Return Value:

    NDIS_STATUS_FAILURE         The frame is too short
    NDIS_STATUS_SUCCESS         Else

--*/
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    NIC_FRAME_HEADER Header;
    ULONG cbHeader = sizeof(Header);

    Status = HWCopyBytesFromNetBuffer(NetBuffer, &cbHeader, &Header);
    if(Status == NDIS_STATUS_SUCCESS)
    {
        if (cbHeader < sizeof(Header))
        {
            NdisZeroMemory(DestAddress, NIC_MACADDR_SIZE);
            Status = NDIS_STATUS_FAILURE;
        }
        else
        {
            GET_DESTINATION_OF_FRAME(DestAddress, &Header);
        }
    }
	else
	{
		NdisZeroMemory(DestAddress, NIC_MACADDR_SIZE);
	}

    return Status;
}

BOOLEAN
HWIsFrameAcceptedByPacketFilter(
    _In_  PMP_ADAPTER  Adapter,
    _In_reads_bytes_(NIC_MACADDR_SIZE) PUCHAR  DestAddress,
    _In_  ULONG        FrameType)
/*++

Routine Description:

    This routines checks to see whether the packet can be accepted
    for transmission based on the currently programmed filter type
    of the NIC and the mac address of the packet.

    With real adapter, this routine would be implemented in hardware.  However,
    since we don't have any hardware to do the matching for us, we'll do it in
    the driver.

Arguments:

    Adapter                     Our adapter that is receiving a frame
    FrameData                   The raw frame, starting at the frame header
    cbFrameData                 Number of bytes in the FrameData buffer

Return Value:

    TRUE if the frame is accepted by the packet filter, and should be indicated
    up to the higher levels of the stack.

    FALSE if the frame doesn't match the filter, and should just be dropped.

--*/
{
    BOOLEAN     result = FALSE;

    DEBUGP(MP_LOUD, "[%p] ---> HWIsFrameAcceptedByPacketFilter PacketFilter = 0x%08x, FrameType = 0x%08x\n",
            Adapter,
            Adapter->PacketFilter,
            FrameType);

    do
    {
        //
        // If the NIC is in promiscuous mode, we will accept anything
        // and everything.
        //
        if (Adapter->PacketFilter & NDIS_PACKET_TYPE_PROMISCUOUS)
        {
            result = TRUE;
            break;
        }


        switch (FrameType)
        {
            case NDIS_PACKET_TYPE_BROADCAST:
                if (Adapter->PacketFilter & NDIS_PACKET_TYPE_BROADCAST)
                {
                    //
                    // If it's a broadcast packet and broadcast is enabled,
                    // we can accept that.
                    //
                    result = TRUE;
                }
                break;


            case NDIS_PACKET_TYPE_MULTICAST:
                //
                // If it's a multicast packet and multicast is enabled,
                // we can accept that.
                //
                if (Adapter->PacketFilter & NDIS_PACKET_TYPE_ALL_MULTICAST)
                {
                    result = TRUE;
                    break;
                }
                else if (Adapter->PacketFilter & NDIS_PACKET_TYPE_MULTICAST)
                {
                    ULONG index;

                    //
                    // Check to see if the multicast address is in our list
                    //
                    ASSERT(Adapter->ulMCListSize <= NIC_MAX_MCAST_LIST);
                    for (index=0; index < Adapter->ulMCListSize && index < NIC_MAX_MCAST_LIST; index++)
                    {
                        if (NIC_ADDR_EQUAL(DestAddress, Adapter->MCList[index]))
                        {
                            result = TRUE;
                            break;
                        }
                    }
                }
                break;


            case NDIS_PACKET_TYPE_DIRECTED:

                if (Adapter->PacketFilter & NDIS_PACKET_TYPE_DIRECTED)
                {
                    //
                    // This has to be a directed packet. If so, does packet dest
                    // address match with the mac address of the NIC.
                    //
                    if (NIC_ADDR_EQUAL(DestAddress, Adapter->CurrentAddress))
                    {
                        result = TRUE;
                        break;
                    }
                }

                break;
        }

    } while(FALSE);


    DEBUGP(MP_LOUD, "[%p] <--- HWIsFrameAcceptedByPacketFilter Result = %u\n", Adapter, result);
    return result;
}


NDIS_MEDIA_CONNECT_STATE
HWGetMediaConnectStatus(
    _In_  PMP_ADAPTER Adapter)
/*++

Routine Description:

    This routine will query the hardware and return
    the media status.

Arguments:

    Adapter                     Our Adapter

Return Value:

    NdisMediaStateDisconnected or
    NdisMediaStateConnected

--*/
{
    if (MP_TEST_FLAG(Adapter, fMP_DISCONNECTED))
    {
        return MediaConnectStateDisconnected;
    }
    else
    {
        return MediaConnectStateConnected;
    }
}

VOID
HWProgramDmaForSend(
    _In_  PMP_ADAPTER   Adapter,
    _In_  PTCB          Tcb,
    _In_  PNET_BUFFER   NetBuffer,
    _In_  BOOLEAN       fAtDispatch)
/*++

Routine Description:

    Program the hardware to read the data payload from the NET_BUFFER's MDL
    and queue it for transmission.  When the hardware has finished reading the
    MDL, it will fire an interrupt to indicate that it no longer needs the MDL
    anymore.

    Our hardware, of course, doesn't have any DMA, so it just copies the data
    to a FRAME structure and transmits that.


    Runs at IRQL <= DISPATCH_LEVEL

Arguments:

    Adapter                     Our adapter that will send a frame
    Tcb                         The TCB that tracks the transmit status
    NetBuffer                   Contains the data to send
    fAtDispatch                 TRUE if the current IRQL is DISPATCH_LEVEL

Return Value:

    None.

--*/
{
    PFRAME Frame;
    NDIS_NET_BUFFER_LIST_8021Q_INFO Nbl1QInfo = {0};
    PNET_BUFFER_LIST Nbl = NULL;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

    DEBUGP(MP_TRACE, "[%p] ---> HWProgramDmaForSend. NB: 0x%p\n", Adapter, NetBuffer);

    do
    {
        //
        // Program the hardware to begin reading the data from NetBuffer's MDL and
        // queue it for transmission.
        //

        Tcb->NetBuffer = NetBuffer;
        Tcb->BytesActuallySent = 0;

        Frame = (PFRAME)NdisAllocateFromNPagedLookasideList(&GlobalData.FrameDataLookaside);

        if (!Frame)
        {
            DEBUGP(MP_TRACE, "[%p] ---> No frames available for send.\n", Adapter);
            //
            // Oops, we couldn't get any more FRAMEs to send.
            //
            // When the SendComplete fires, we'll tell the driver that zero bytes
            // were sent successfully.  It will update the bookkeeping and inform
            // the protocol that the NBL wasn't completely sent.
            //
            break;
        }

        DEBUGP(MP_TRACE, "[%p] Send Frame: 0x%p\n", Adapter, Frame);
        ASSERT(NET_BUFFER_DATA_LENGTH(NetBuffer) <= NIC_BUFFER_SIZE);

        Frame->Ref = 1;

        //
        // Copy the data from the NB to the FRAME's data region.  This step roughly
        // corresponds to a hardware DMA.
        //
        Frame->ulSize = min(NET_BUFFER_DATA_LENGTH(NetBuffer), NIC_BUFFER_SIZE);
        Status = HWCopyBytesFromNetBuffer(NetBuffer, &Frame->ulSize, Frame->Data);
        if(Status != NDIS_STATUS_SUCCESS)
        {
            DEBUGP(MP_TRACE, "[%p] ---> Failed to copy frame buffer. Result = %u\n", Adapter, Status);
            break;
        }

        if (Frame->ulSize < HW_MIN_FRAME_SIZE)
        {
            // Don't leak the contents of kernel memory!  Zero out padding bytes.
            ULONG cbPaddingNeeded = HW_MIN_FRAME_SIZE - Frame->ulSize;
            NdisZeroMemory(Frame->Data + Frame->ulSize, cbPaddingNeeded);
            Frame->ulSize += cbPaddingNeeded;
        }

        ASSERT(Frame->ulSize >= HW_MIN_FRAME_SIZE && Frame->ulSize <= NIC_BUFFER_SIZE);


        //
        // For simplicity in the sample in order to support VLAN we extract the information from the NBL or frame and pass the 
        // NDIS_NET_BUFFER_LIST_8021Q_INFO structure to the code that simulates the send/receive. The code does nothing to convert
        // modify the frame format. 
        // In real HW, on send the code should extract the information from the NBL and covert it to 802.1Q format for transmission, and
        // on receive the adapter should detect if the packet is in 802.1Q format and if so convert it back to 802.3 before indicating it up to NDIS
        // (populating the 8021Q info in the NBL being indicated). 
        //
        Nbl = NBL_FROM_SEND_NB(NetBuffer);
        Nbl1QInfo.Value = NET_BUFFER_LIST_INFO(Nbl, Ieee8021QNetBufferListInfo);

        if(Nbl1QInfo.Value)
        {
            DEBUGP(MP_TRACE, "[%p] Send NBL (%p) OOB Vlan ID: %i\n", Adapter, Nbl, Nbl1QInfo.TagHeader.VlanId);
        }
        else
        {
            DEBUGP(MP_TRACE, "[%p] Send NBL (%p) has no OOB VLAN tag, checking frame header.\n", Adapter, Nbl);
            if(IS_FRAME_8021Q(Frame)) 
            {
                //
                // The frame has type of 802.1Q. Retrieve the VLAN information
                //
                COPY_TAG_INFO_FROM_HEADER_TO_PACKET_INFO(Nbl1QInfo, GET_FRAME_VLAN_TAG_HEADER(Frame));
                DEBUGP(MP_TRACE, "[%p] Send NBL (%p) frame Vlan ID: %i\n", Adapter, Nbl, Nbl1QInfo.TagHeader.VlanId);
            }
            else
            {
                DEBUGP(MP_TRACE, "[%p] Send NBL (%p) has no VLAN information in its frame header.\n", Adapter, Nbl); 
            }
        } 

        RXDeliverFrameToEveryAdapter(Adapter, &Nbl1QInfo, Frame, fAtDispatch);

        Tcb->BytesActuallySent = Frame->ulSize;

    } while(FALSE);

    if(Frame)
    {
        HWFrameRelease(Frame);
    }

    DEBUGP(MP_TRACE, "[%p] <--- HWProgramDmaForSend\n", Adapter);

}


_IRQL_requires_(DISPATCH_LEVEL)
ULONG
HWGetBytesSent(
    _In_  PMP_ADAPTER  Adapter,
    _In_  PTCB         Tcb)
/*++

Routine Description:

    When the adapter indicates it has completed the send operation, call this
    routine to determine how many bytes were successfully sent (if any).

Arguments:

    Adapter                     Our adapter that will send a frame
    Tcb                         The TCB that tracks the transmit status

Return Value:

    0                           There was an error sending this frame
    >0                          Number of bytes actually sent

--*/
{
    UNREFERENCED_PARAMETER(Adapter);

    return Tcb->BytesActuallySent;
}

NDIS_STATUS
HWBeginReceiveDma(
    _In_  PMP_ADAPTER  Adapter,
    _In_  PNDIS_NET_BUFFER_LIST_8021Q_INFO Nbl1QInfo,
    _In_  PRCB         Rcb,
    _In_  PFRAME       Frame)
/*++

Routine Description:

    Simulate the hardware deciding to receive a FRAME into one of its RCBs. In VMQ enabled scenarios, it will 
    find the matching queue and if matched retrieve the shared memory for the queue for the NBL. Otherwise, it 
    uses the existing Frame for the NBL.

Arguments:

    Adapter                     Pointer to our adapter
    Nbl1QInfo                   8021Q Tag information for the FRAME being received
    Rcb                         The RCB that tracks this receive operation
    Frame                       The FRAME that to receive

Return Value:

    NDIS_STATUS

--*/
{

    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PNET_BUFFER NetBuffer = NET_BUFFER_LIST_FIRST_NB(Rcb->Nbl);

    do
    {
        DEBUGP(MP_TRACE, "[%p] ---> HWBeginReceiveDma. Frame: 0x%p\n", Adapter, Frame);

        //
        // Preserve 802.1Q information, if specified. 
        //
        if(Nbl1QInfo->Value)
        {
            DEBUGP(MP_TRACE, "[%p] Preserved VLAN Id=%i\n", Adapter, Nbl1QInfo->TagHeader.VlanId);
            NET_BUFFER_LIST_INFO(Rcb->Nbl, Ieee8021QNetBufferListInfo) = Nbl1QInfo->Value;
        }
        else
        {
            DEBUGP(MP_TRACE, "[%p] No VLAN tag to preserve.\n", Adapter);
            NET_BUFFER_LIST_INFO(Rcb->Nbl, Ieee8021QNetBufferListInfo) = 0;
        }

        //
        // If VMQ is enabled, and we're not using the default queue, 
        // we need to copy the FRAME to the NBL's shared memory area
        //
        if(VMQ_ENABLED(Adapter))
        {
            BOOLEAN Copied;
            Status = CopyFrameToRxQueueRcb(Adapter, Frame, Nbl1QInfo, Rcb, &Copied);
            if(Copied || Status != NDIS_STATUS_SUCCESS)
            {
                break;
            }
        }
        else
        {
            UNREFERENCED_PARAMETER(Adapter);
        }

        //
        // Either 6.20 is not supported, VMQ is disabled, or matched default queue, so use existing frame memory for indication
        //
        HWFrameReference(Frame);
        Rcb->Data = Frame;

        NET_BUFFER_FIRST_MDL(NetBuffer) = Frame->Mdl;
        NET_BUFFER_DATA_LENGTH(NetBuffer) = Frame->ulSize;
        NET_BUFFER_DATA_OFFSET(NetBuffer) = 0;
        NET_BUFFER_CURRENT_MDL(NetBuffer) = NET_BUFFER_FIRST_MDL(NetBuffer);
        NET_BUFFER_CURRENT_MDL_OFFSET(NetBuffer) = 0;
    
    }
    while(FALSE);
     
    DEBUGP(MP_TRACE, "[%p] <--- HWBeginReceiveDma Status 0x%08x\n", Adapter, Status);
    return Status;

}
 
