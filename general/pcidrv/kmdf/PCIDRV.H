/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    PciDrv.h

Abstract:

    Header file for the PCIDRV driver modules.

Environment:

    Kernel mode

--*/


#if !defined(_PCIDRV_H_)
#define _PCIDRV_H_

//
// Let us use newly introduced (.NET DDK) safe string function to avoid
// security issues related buffer overrun.
// The advantages of the RtlStrsafe functions include:
// 1) The size of the destination buffer is always provided to the
// function to ensure that the function does not write past the end of
// the buffer.
// 2) Buffers are guaranteed to be null-terminated, even if the
// operation truncates the intended result.
//

//
// In this driver we are using a safe version vsnprintf, which is
// RtlStringCbVPrintfA. To use strsafe function on 9x, ME, and Win2K Oses, we
// have to define NTSTRSAFE_LIB before including this header file and explicitly
// link to ntstrsafe.lib. If your driver is just target for XP and above, there is
// no define NTSTRSAFE_LIB and link to the lib.
//
#define NTSTRSAFE_LIB
#include <ntstrsafe.h>

//-----------------------------------------------------------------------------
// 4127 -- Conditional Expression is Constant warning
//-----------------------------------------------------------------------------
#define WHILE(constant) \
__pragma(warning(suppress: 4127)) while(constant)

#define _DRIVER_NAME_ "PCIDRV"

#define PCIDRV_POOL_TAG (ULONG) 'DICP'
#define PCIDRV_FDO_INSTANCE_SIGNATURE (ULONG) 'odFT'

#define MILLISECONDS_TO_100NS   (10000)
#define SECOND_TO_MILLISEC      (1000)
#define SECOND_TO_100NS         (SECOND_TO_MILLISEC * MILLISECONDS_TO_100NS)

//
// Bit Flag Macros
//

#define SET_FLAG(Flags, Bit)    ((Flags) |= (Bit))
#define CLEAR_FLAG(Flags, Bit)  ((Flags) &= ~(Bit))
#define TEST_FLAG(Flags, Bit)   (((Flags) & (Bit)) != 0)

//
// The driver context contains global data to the whole driver.
//
typedef struct _DRIVER_CONTEXT {
    //
    // The assumption here is that there is nothing device specific in the lookaside list
    // and hence the same list can be used to do allocations for multiple devices.
    //
    WDFLOOKASIDE            RecvLookaside;

} DRIVER_CONTEXT, * PDRIVER_CONTEXT;
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DRIVER_CONTEXT, GetDriverContext)



//
// Connector Types
//

typedef struct _PCIDRV_WMI_STD_DATA {

    //
    // Current Mac Address of the NIC
    //

    UINT64  MacAddress;

} PCIDRV_WMI_STD_DATA, * PPCIDRV_WMI_STD_DATA;


//
// General purpose workitem context used in dispatching work to
// system worker thread to be executed at PASSIVE_LEVEL.
//
typedef struct _WORKER_ITEM_CONTEXT {
    PFDO_DATA      FdoData;
    PVOID          Argument1;
    PVOID          Argument2;
} WORKER_ITEM_CONTEXT, *PWORKER_ITEM_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(WORKER_ITEM_CONTEXT, GetWorkItemContext)

//
// The device extension for the device object
//
typedef struct _FDO_DATA
{
    ULONG                   Signature;       // must be PCIDRV_FDO_INSTANCE_SIGNATURE
                                             // beneath this device object.
    WDFDEVICE               WdfDevice;

    // Power Management
    MP_POWER_MGMT           PoMgmt;
    WDF_POWER_DEVICE_STATE      DevicePowerState;   // Current power state of the device(D0 - D3)

    // Wait-Wake
    BOOLEAN                 AllowWakeArming;

    // Idle Detection
    //BOOLEAN                 IdleDetectionEnabled;
    PCIDRV_WMI_STD_DATA     StdDeviceData;

    // Following fields are specific to the hardware
    // Configuration
    ULONG                   Flags;
    UCHAR                   PermanentAddress[ETH_LENGTH_OF_ADDRESS];
    UCHAR                   CurrentAddress[ETH_LENGTH_OF_ADDRESS];
    BOOLEAN                 bOverrideAddress;
    USHORT                  AiTxFifo;           // TX FIFO Threshold
    USHORT                  AiRxFifo;           // RX FIFO Threshold
    UCHAR                   AiTxDmaCount;       // Tx dma count
    UCHAR                   AiRxDmaCount;       // Rx dma count
    UCHAR                   AiUnderrunRetry;    // The underrun retry mechanism
    UCHAR                   AiForceDpx;         // duplex setting
    USHORT                  AiTempSpeed;        // 'Speed', user over-ride of line speed
    USHORT                  AiThreshold;        // 'Threshold', Transmit Threshold
    BOOLEAN                 MWIEnable;          // Memory Write Invalidate bit in the PCI command word
    UCHAR                   Congest;            // Enables congestion control
    ULONG                   SpeedDuplex;        // New reg value for speed/duplex


    // IDs
    UCHAR                   RevsionID;
    USHORT                  SubVendorID;
    USHORT                  SubSystemID;

    // HW Resources
    ULONG                   CacheFillSize;
    PULONG                  IoBaseAddress;
    ULONG                   IoRange;
    PHYSICAL_ADDRESS        MemPhysAddress;

    WDFINTERRUPT            WdfInterrupt;

    BOOLEAN                 MappedPorts;
    PHW_CSR                 CSRAddress;
    BUS_INTERFACE_STANDARD  BusInterface;
    PREAD_PORT              ReadPort;
    PWRITE_PORT             WritePort;
    WDFDMAENABLER           WdfDmaEnabler;

    // Media Link State
    UCHAR                   CurrentScanPhyIndex;
    UCHAR                   LinkDetectionWaitCount;
    UCHAR                   FoundPhyAt;
    USHORT                  EepromAddressSize;
    MEDIA_STATE             MediaState;

    // SEND
    PMP_TCB                 CurrSendHead;
    PMP_TCB                 CurrSendTail;
    ULONG                   nBusySend;
    LONG                    nWaitSend;
    LONG                    nCancelSend;
    WDFQUEUE                WriteQueue;
    WDFQUEUE                PendingWriteQueue;
    SINGLE_LIST_ENTRY       SendBufList;
    WDFSPINLOCK             SendLock;

    ULONG                   NumTcb;             // Total number of TCBs
    LONG                    RegNumTcb;          // 'NumTcb'
    ULONG                   NumBuffers;


    _Field_size_(MpTcbMemSize) PUCHAR MpTcbMem;
    ULONG                   MpTcbMemSize;

    WDFCOMMONBUFFER         WdfSendCommonBuffer;

    _Field_size_(HwSendMemAllocSize) PUCHAR HwSendMemAllocVa;
    ULONG                   HwSendMemAllocSize;
    PHYSICAL_ADDRESS        HwSendMemAllocLa;     // Logical Address

    // command unit status flags
    BOOLEAN                 TransmitIdle;
    BOOLEAN                 ResumeWait;

    // RECV
    LIST_ENTRY              RecvList;
    ULONG                   nReadyRecv;
    LONG                    RefCount;

    ULONG                   NumRfd;
    ULONG                   CurrNumRfd;
    ULONG                   MaxNumRfd;
    ULONG                   HwRfdSize;
    LONG                    RfdShrinkCount;

    WDFQUEUE                PendingReadQueue;
    WDFSPINLOCK             RcvLock;

    BOOLEAN                 AllocNewRfd;

    // spin locks for protecting misc variables
    WDFSPINLOCK         Lock;

    // Packet Filter and look ahead size.
    ULONG                   PacketFilter;
    ULONG                   OldPacketFilter;
    ULONG                   ulLookAhead;
    USHORT                  usLinkSpeed;
    USHORT                  usDuplexMode;

    // multicast list
    UINT                    MCAddressCount;
    UCHAR                   MCList[NIC_MAX_MCAST_LIST][ETH_LENGTH_OF_ADDRESS];

    WDFCOMMONBUFFER         WdfMiscCommonBuffer;
    _Field_size_(HwMiscMemAllocSize) PUCHAR HwMiscMemAllocVa;
    ULONG                   HwMiscMemAllocSize;
    PHYSICAL_ADDRESS        HwMiscMemAllocLa;   // Logical Address

    PSELF_TEST_STRUC        SelfTest;           // 82558 SelfTest
    ULONG                   SelfTestPhys;
    BOOLEAN                 SelfTested;

    PNON_TRANSMIT_CB        NonTxCmdBlock;      // 82558 (non transmit) Command Block
    ULONG                   NonTxCmdBlockPhys;

    PDUMP_AREA_STRUC        DumpSpace;          // 82558 dump buffer area
    ULONG                   DumpSpacePhys;

    PERR_COUNT_STRUC        StatsCounters;
    ULONG                   StatsCounterPhys;

    UINT                    PhyAddress;         // Address of the phy component
    UCHAR                   Connector;          // 0=Auto, 1=TPE, 2=MII

    UCHAR                   OldParameterField;

    ULONG                   HwErrCount;

    // WatchDog timer related fields
    WDFTIMER                WatchDogTimer;
    BOOLEAN                 bLinkDetectionWait;
    BOOLEAN                 bLookForLink;
    BOOLEAN                 CheckForHang;

    // For handling IOCTLs - required if the upper edge is NDIS
    WDFQUEUE                IoctlQueue;
    WDFQUEUE                PendingIoctlQueue;

    // Packet counts
    ULONG64                 GoodTransmits;
    ULONG64                 GoodReceives;
    ULONG                   NumTxSinceLastAdjust;

    // Count of transmit errors
    ULONG                   TxAbortExcessCollisions;
    ULONG                   TxLateCollisions;
    ULONG                   TxDmaUnderrun;
    ULONG                   TxLostCRS;
    ULONG                   TxOKButDeferred;
    ULONG                   OneRetry;
    ULONG                   MoreThanOneRetry;
    ULONG                   TotalRetries;

    // Count of receive errors
    ULONG                   RcvCrcErrors;
    ULONG                   RcvAlignmentErrors;
    ULONG                   RcvResourceErrors;
    ULONG                   RcvDmaOverrunErrors;
    ULONG                   RcvCdtFrames;
    ULONG                   RcvRuntErrors;

    // Count of bytes received & transmitted
    ULONG64                 BytesReceived;
    ULONG64                 BytesTransmitted;
}  FDO_DATA, *PFDO_DATA;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FDO_DATA, FdoGetData)

#define CLRMASK(x, mask)     ((x) &= ~(mask));
#define SETMASK(x, mask)     ((x) |=  (mask));


//
// Function prototypes
//
DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_DEVICE_ADD PciDrvEvtDeviceAdd;

EVT_WDF_OBJECT_CONTEXT_CLEANUP PciDrvEvtDriverContextCleanup;
EVT_WDF_DEVICE_CONTEXT_CLEANUP PciDrvEvtDeviceContextCleanup;

EVT_WDF_DEVICE_D0_ENTRY PciDrvEvtDeviceD0Entry;
EVT_WDF_DEVICE_D0_EXIT PciDrvEvtDeviceD0Exit;
EVT_WDF_DEVICE_PREPARE_HARDWARE PciDrvEvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE PciDrvEvtDeviceReleaseHardware;

EVT_WDF_DEVICE_SELF_MANAGED_IO_CLEANUP PciDrvEvtDeviceSelfManagedIoCleanup;
EVT_WDF_DEVICE_SELF_MANAGED_IO_INIT PciDrvEvtDeviceSelfManagedIoInit;
EVT_WDF_DEVICE_SELF_MANAGED_IO_SUSPEND PciDrvEvtDeviceSelfManagedIoSuspend;
EVT_WDF_DEVICE_SELF_MANAGED_IO_RESTART PciDrvEvtDeviceSelfManagedIoRestart;

EVT_WDF_DEVICE_ARM_WAKE_FROM_S0 PciDrvEvtDeviceWakeArmS0;
EVT_WDF_DEVICE_ARM_WAKE_FROM_SX PciDrvEvtDeviceWakeArmSx;
EVT_WDF_DEVICE_DISARM_WAKE_FROM_S0 PciDrvEvtDeviceWakeDisarmS0;
EVT_WDF_DEVICE_DISARM_WAKE_FROM_SX PciDrvEvtDeviceWakeDisarmSx;
EVT_WDF_DEVICE_WAKE_FROM_S0_TRIGGERED PciDrvEvtDeviceWakeTriggeredS0;
EVT_WDF_DEVICE_WAKE_FROM_SX_TRIGGERED PciDrvEvtDeviceWakeTriggeredSx;

EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL PciDrvEvtIoDeviceControl;

NTSTATUS
PciDrvReturnResources (
    IN OUT PFDO_DATA FdoData
    );

NTSTATUS
PciDrvSetPowerPolicy(
    IN PFDO_DATA FdoData
    );

NTSTATUS
PciDrvQueuePassiveLevelCallback(
    IN PFDO_DATA        FdoData,
    IN PFN_WDF_WORKITEM CallbackFunction,
    IN PVOID            Context1,
    IN PVOID            Context2
    );

BOOLEAN
PciDrvReadRegistryValue(
    _In_  PFDO_DATA  FdoData,
    _In_  PWSTR      Name,
    _Out_ PULONG     Value
    );

BOOLEAN
PciDrvWriteRegistryValue(
    _In_ PFDO_DATA  FdoData,
    _In_ PWSTR      Name,
    _In_ ULONG      Value
    );


NTSTATUS
PciDrvWmiRegistration(
    WDFDEVICE      hDevice
);

PCHAR
DbgDevicePowerString(
    IN WDF_POWER_DEVICE_STATE Type
    );


BOOLEAN
PciDrvReadFdoRegistryKeyValue(
    _In_  PWDFDEVICE_INIT  DeviceInit,
    _In_  PWSTR            Name,
    _Out_ PULONG           Value
    );

#if defined(WIN2K)

NTKERNELAPI
VOID
ExFreePoolWithTag(
    _In_ PVOID P,
    _In_ ULONG Tag
    );

#endif

#endif  // _PCIDRV_H_


