/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

   Hardware.H

Abstract:

    This module defines constants that describe physical characteristics and
    limits of the underlying hardware.  While this miniport is virtual -- it
    doesn't have any "real" hardware -- it emulates a fast Ethernet (802.3 at
    100 Mbit/s) link.  Constants (like the 1514 byte frame size) are based on
    typical Ethernet parameters.

    TODO:
       1.  Change the vendor name and ID from Microsoft.
       2.  Update the hardware limits and physical adapter properties to match
           those of your hardware.
       3.  If your hardware does not use a 100 Mbit/s Ethernet medium, also
           update the addressing, framing, and medium sections.

--*/


#ifndef _HARDWARE_H
#define _HARDWARE_H


//
// Link layer addressing
// -----------------------------------------------------------------------------
//

// Number of bytes in a hardware address.  Ethernet uses 6 byte addresses.
#define NIC_MACADDR_SIZE                   ETH_LENGTH_OF_ADDRESS

// True iff the given address is a multicast address
#define NIC_ADDR_IS_MULTICAST(_addr)       ETH_IS_MULTICAST(_addr)

// True iff the given address is a broadcast address
#define NIC_ADDR_IS_BROADCAST(_addr)       ETH_IS_BROADCAST(_addr)

// Copies a hardware address from _src to _dest
#define NIC_COPY_ADDRESS(_dest,_src)       ETH_COPY_NETWORK_ADDRESS(_dest,_src)

// True iff the given address was assigned by the local administrator
#define NIC_ADDR_IS_LOCALLY_ADMINISTERED(_addr) \
        (BOOLEAN)(((PUCHAR)(_addr))[0] & ((UCHAR)0x02))

// True iff two addresses are equal
#define NIC_ADDR_EQUAL(_a,_b) \
        ((*(ULONG UNALIGNED *)&(_a)[2] == *(ULONG UNALIGNED *)&(_b)[2]) \
        && (*(USHORT UNALIGNED *)(_a) == *(USHORT UNALIGNED *)(_b)))



//
// Frames
// -----------------------------------------------------------------------------
//

#define HW_FRAME_HEADER_SIZE               14
#define HW_FRAME_MAX_DATA_SIZE             1500
#define HW_MAX_FRAME_SIZE                  (HW_FRAME_HEADER_SIZE + HW_FRAME_MAX_DATA_SIZE)
#define HW_MIN_FRAME_SIZE                  60

typedef struct tagNIC_FRAME_HEADER
{
    UCHAR  DestAddress[NIC_MACADDR_SIZE];
    UCHAR  SrcAddress[NIC_MACADDR_SIZE];
    UCHAR  EtherType[2];
} NIC_FRAME_HEADER, *PNIC_FRAME_HEADER;

C_ASSERT(sizeof(NIC_FRAME_HEADER) == HW_FRAME_HEADER_SIZE);

#define GET_DESTINATION_OF_FRAME(_dest, _frame) NdisMoveMemory(_dest, ((PNIC_FRAME_HEADER)(_frame))->DestAddress, NIC_MACADDR_SIZE)

//
// Medium properties
// -----------------------------------------------------------------------------
//

#define NIC_MEDIUM_TYPE                    NdisMedium802_3

// If you have physical hardware on 802.3, use NdisPhysicalMedium802_3.
#define NIC_PHYSICAL_MEDIUM                NdisPhysicalMedium802_3


// Set this value to TRUE if there is a physical adapter.
#define NIC_HAS_PHYSICAL_CONNECTOR         FALSE
#define NIC_ACCESS_TYPE                    NET_IF_ACCESS_BROADCAST
#define NIC_DIRECTION_TYPE                 NET_IF_DIRECTION_SENDRECEIVE
#define NIC_CONNECTION_TYPE                NET_IF_CONNECTION_DEDICATED

// This value must match the *IfType in the driver .inf file
#define NIC_IFTYPE                         IF_TYPE_ETHERNET_CSMACD

// Claim to be 100mbps duplex
#define MEGABITS_PER_SECOND                1000000ULL
#define NIC_XMIT_SPEED                     (100ULL*MEGABITS_PER_SECOND)
#define NIC_RECV_SPEED                     (100ULL*MEGABITS_PER_SECOND)



//
// Hardware limits
// -----------------------------------------------------------------------------
//

// Max number of multicast addresses supported in hardware
#define NIC_MAX_MCAST_LIST                 32

// Maximum number of uncompleted sends that a single adapter will permit
#define NIC_MAX_BUSY_SENDS                 256

// Maximum number of unreturned receives that a single adapter will permit
#define NIC_MAX_BUSY_RECVS                 256



// Maximum number of send completes that will be processed per DPC.
#define NIC_MAX_SENDS_PER_DPC              64

//
// Maximum number of receives that will be processed per DPC.
// This constraints the amount of time spent for a single receive DPC. 
//
#define NIC_MAX_RECVS_PER_DPC              64

#define NIC_MAX_LOOKAHEAD                  HW_FRAME_MAX_DATA_SIZE
#define NIC_BUFFER_SIZE                    HW_MAX_FRAME_SIZE


// Simulated latency across the link.  If this is set to zero, the driver
// will saturate the link.  Unfortunately, when the "link" is simulated in CPU,
// that means that the CPU is saturated, and you hit DPC timeouts.
// (This throttling isn't needed with a physical NIC)
#define NIC_SIMULATED_LATENCY              0 // in 100ns units

//
// Physical adapter properties
// -----------------------------------------------------------------------------
//

// The bus that connects the adapter to the PC.
// (Example: PCI adapters should use NdisInterfacePci).
#define NIC_INTERFACE_TYPE                 NdisInterfaceInternal

// Change to your company name instead of using Microsoft
#define NIC_VENDOR_DESC                    "Microsoft"

// Highest byte is the NIC byte plus three vendor bytes. This is normally
// obtained from the NIC.
#define NIC_VENDOR_ID                      0x00FFFFFF


// Wakeup capabilities, as in OID_PNP_CAPABILITIES
#define NIC_MAGIC_PACKET_WAKEUP            NdisDeviceStateUnspecified
#define NIC_PATTERN_WAKEUP                 NdisDeviceStateUnspecified
#define NIC_LINK_CHANGE_WAKEUP             NdisDeviceStateUnspecified


#define NIC_SUPPORTED_FILTERS ( \
                NDIS_PACKET_TYPE_DIRECTED   | \
                NDIS_PACKET_TYPE_MULTICAST  | \
                NDIS_PACKET_TYPE_BROADCAST  | \
                NDIS_PACKET_TYPE_PROMISCUOUS | \
                NDIS_PACKET_TYPE_ALL_MULTICAST)

//
// This sample is a virtual device, so it can tolerate surprise removal
// and suspend.  Ensure the correct flags are set for your hardware.
//
// If your hardware supports busmaster DMA, you must specify
// NDIS_MINIPORT_ATTRIBUTES_BUS_MASTER. Our virtual miniport will 
// not be allocating hardware resources such as interrupts, so we set the 
// WDM attribute.  
//
#define NIC_ADAPTER_ATTRIBUTES_FLAGS (\
                NDIS_MINIPORT_ATTRIBUTES_SURPRISE_REMOVE_OK | NDIS_MINIPORT_ATTRIBUTES_NDIS_WDM)


//
// Specify a bitmask that defines optional properties of the NIC.
// This miniport indicates receive with NdisMIndicateReceiveNetBufferLists
// function.  Such a driver should set this NDIS_MAC_OPTION_TRANSFERS_NOT_PEND
// flag.
//
// NDIS_MAC_OPTION_NO_LOOPBACK tells NDIS that NIC has no internal
// loopback support so NDIS will manage loopbacks on behalf of
// this driver.
//
// NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA tells the protocol that
// our receive buffer is not on a device-specific card. If
// NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA is not set, multi-buffer
// indications are copied to a single flat buffer.
//
#define NIC_MAC_OPTIONS (\
                NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA | \
                NDIS_MAC_OPTION_TRANSFERS_NOT_PEND  | \
                NDIS_MAC_OPTION_NO_LOOPBACK         | \
                NDIS_MAC_OPTION_8021P_PRIORITY      | \
                NDIS_MAC_OPTION_8021Q_VLAN)

// NDIS 6.x miniports must support all counters in OID_GEN_STATISTICS.
#define NIC_SUPPORTED_STATISTICS (\
                NDIS_STATISTICS_FLAGS_VALID_DIRECTED_FRAMES_RCV    | \
                NDIS_STATISTICS_FLAGS_VALID_MULTICAST_FRAMES_RCV   | \
                NDIS_STATISTICS_FLAGS_VALID_BROADCAST_FRAMES_RCV   | \
                NDIS_STATISTICS_FLAGS_VALID_BYTES_RCV              | \
                NDIS_STATISTICS_FLAGS_VALID_RCV_DISCARDS           | \
                NDIS_STATISTICS_FLAGS_VALID_RCV_ERROR              | \
                NDIS_STATISTICS_FLAGS_VALID_DIRECTED_FRAMES_XMIT   | \
                NDIS_STATISTICS_FLAGS_VALID_MULTICAST_FRAMES_XMIT  | \
                NDIS_STATISTICS_FLAGS_VALID_BROADCAST_FRAMES_XMIT  | \
                NDIS_STATISTICS_FLAGS_VALID_BYTES_XMIT             | \
                NDIS_STATISTICS_FLAGS_VALID_XMIT_ERROR             | \
                NDIS_STATISTICS_FLAGS_VALID_XMIT_DISCARDS          | \
                NDIS_STATISTICS_FLAGS_VALID_DIRECTED_BYTES_RCV     | \
                NDIS_STATISTICS_FLAGS_VALID_MULTICAST_BYTES_RCV    | \
                NDIS_STATISTICS_FLAGS_VALID_BROADCAST_BYTES_RCV    | \
                NDIS_STATISTICS_FLAGS_VALID_DIRECTED_BYTES_XMIT    | \
                NDIS_STATISTICS_FLAGS_VALID_MULTICAST_BYTES_XMIT   | \
                NDIS_STATISTICS_FLAGS_VALID_BROADCAST_BYTES_XMIT)

#if (NDIS_SUPPORT_NDIS620)

//
// VMQ hardware information
//

//
// The NIC must reserve at least one filter available HW queue. More filters allows
// the VMQ queues to be assigned asymmetrically. For this sample we chose to allow
// twice as many filters as queues. 
//
#define NIC_SUPPORTED_NUM_QUEUES 8
#define NIC_MAX_HEADER_FILTERS (NIC_SUPPORTED_NUM_QUEUES*2)

//
// Determines the minimum and maximum amount of lookahead split that we can do. Real hardware
// might have tighter constraints on the range depending on the HW design. 
//
#define NIC_MIN_LOOKAHEAD_SPLIT 64
#define NIC_MAX_LOOKAHEAD_SPLIT 128

//
// Determines the minimum amount of receive blocks we will attempt to allocate if the full allocations fail and
// we retry with a reduced count. If we fail with this amount we fail the allocation. 
//
#define NIC_MIN_BUSY_RECVS 64

#else

//
// No VMQ, define the queue count to 1 so that we only allocate the global receive block
//
#define NIC_SUPPORTED_NUM_QUEUES 1

#endif

#if (NDIS_SUPPORT_NDIS630)

//
// NDIS QoS hardware information
//

//
// Total number of general Traffic Classes (TCs) that the hardware supports, each of which is capable of but
// not just limited to ETS functionality.
//
#define NIC_SUPPORTED_NUM_TCS               8

//
// Total number of TCs that the hardware supports that is capable of only ETS functionality.
//
#define NIC_SUPPORTED_NUM_ETS_CAPABLE_TCS   NIC_SUPPORTED_NUM_TCS
C_ASSERT(NIC_SUPPORTED_NUM_ETS_CAPABLE_TCS <= NIC_SUPPORTED_NUM_TCS);

//
// Number of simultaneous PFC-enabled TCs that the hardware can support.
//
#define NIC_SUPPORTED_NUM_PFC_ENABLED_TCS   NIC_SUPPORTED_NUM_TCS
C_ASSERT(NIC_SUPPORTED_NUM_PFC_ENABLED_TCS <= NIC_SUPPORTED_NUM_TCS);

//
// The Ethertype value for FCoE traffic and the default 802.1p priority assignment.
//
#define NIC_FCOE_ETHERTYPE                  0x8906
#define NIC_FCOE_PRIORITY                   1

//
// The TCP port number for iSCSI traffic and the default 802.1p priority assignment.
//
#define NIC_ISCSI_TCP_PORT                  3260
#define NIC_ISCSI_PRIORITY                  2

//
// Maximum number of classification entries that the miniport can support.
//
// In most cases, a standard NDIS QoS-compliant miniport will look for one or two very specific classification entries
// and not implement a general classification mechanism as is done here.
//
// For example, a DCB-capable NIC with its own FCoE implementation would, in its NDIS QoS-compliant miniport, look for
// a QoS classification entry specifying Ethertype 0x8906. The associated priority value would then be used in
// all FCoE packets sent by that NIC.
//
#define NIC_SUPPORTED_NUM_CLASSIFICATIONS   2


#endif

#endif // _HARDWARE_H

