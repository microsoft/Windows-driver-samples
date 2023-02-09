//
//    Copyright (C) Microsoft.  All rights reserved.
//
#pragma once

#define INITIALIZE_PASSIVE_LOCK(__lock) KeInitializeGuardedMutex(__lock)

#define ACQUIRE_PASSIVE_LOCK(__lock) KeAcquireGuardedMutex(__lock)

#define RELEASE_PASSIVE_LOCK(__lock) KeReleaseGuardedMutex(__lock)

#define WMB_PUSH_LOCK EX_PUSH_LOCK

#define INIT_PUSHLOCK(PushLock) ExInitializePushLock(PushLock)

#define ACQUIRE_PUSHLOCK_EXCLUSIVE(PushLock) \
    do \
    { \
        KeEnterCriticalRegion(); \
        ExAcquirePushLockExclusive((PushLock)); \
    } while (0)

#define RELEASE_PUSHLOCK_EXCLUSIVE(PushLock) \
    do \
    { \
        ExReleasePushLockExclusive((PushLock)); \
        KeLeaveCriticalRegion(); \
    } while (0)

#define ACQUIRE_PUSHLOCK_SHARED(PushLock) \
    do \
    { \
        KeEnterCriticalRegion(); \
        ExAcquirePushLockShared((PushLock)); \
    } while (0)

#define RELEASE_PUSHLOCK_SHARED(PushLock) \
    do \
    { \
        ExReleasePushLockShared((PushLock)); \
        KeLeaveCriticalRegion(); \
    } while (0)

#define MAX_PREALLOCATED_WRITE_REQUESTS (20)

//#define DEFAULT_INTERRUPT_PIPE_READ_SIZE  (8)
#define DEFAULT_IO_TIMEOUT (10)

#define MAX_HOST_NTB_SIZE (0x10000)
#define MAX_HOST_NTB_SIZE_FOR_UDE_MBIM (0x20000)
#define MAX_OUT_DATAGRAMS (64)

#define INTERRUPT_REASSEMBLY_BUFFER_SIZE (64)

#define MIN_CONTROL_MESSAGE_SIZE (64)
#define MAX_CONTROL_MESSAGE_SIZE (4096)

#define ALT_DATA_SETTING_0_PIPES (0)
#define ALT_DATA_SETTING_1_PIPES (2)

#define INITIAL_OPEN_TIMEOUT (5)
#define MAX_OPEN_RETRY_ATTEMPTS (4)

#define PENDING_BULK_IN_READS (3)
#define PENDING_BULK_IN_READS_FOR_UDE_MBIM (10)

typedef enum _BUS_STATE
{

    BUS_STATE_CLOSED = 0,
    BUS_STATE_CLOSING,

    BUS_STATE_OPENING,
    BUS_STATE_OPENED

} BUS_STATE,
    *PBUS_STATE;

typedef struct _POWER_FILTER_LOOKUP
{

    ULONG PatternId;
    BOOLEAN InUse;

} POWER_FILTER_LOOKUP, *PPOWER_FILTER_LOOKUP;

typedef struct _BUS_OBJECT
{

    KGUARDED_MUTEX Lock;
    BUS_STATE State;

    PDEVICE_OBJECT Pdo;
    PDEVICE_OBJECT Fdo;
    PDEVICE_OBJECT NextDeviceObject;

    MBB_PROTOCOL_HANDLE ProtocolHandle;
    MBB_BUS_RESPONSE_AVAILABLE_CALLBACK ResponseAvailableCallback;

    MBB_BUS_DATA_RECEIVE_CALLBACK ReceiveDataCallback;

    MBB_BUS_SS_IDLE_CONFIRM_CALLBACK IdleConfirmCallback;
    MBB_BUS_SS_IDLE_NOTIFICATION_COMPLETE_CALLBACK IdleNotificationComplete;

    WDFDEVICE WdfDevice;
    WDFUSBDEVICE WdfUsbDevice;

    USHORT MaxControlChannelSize;

    USHORT MaxSegmentSize;
    BYTE NcmParams;
    BOOLEAN NtbFormat32Bit;
    BYTE PowerFiltersSupported;
    BYTE MaxPowerFilterSize;

    BYTE MaxOutstandingCommandMessages;
    USHORT MTU;

    NCM_NTB_PARAMETER NtbParam;

    PWSTR Manufacturer;
    PWSTR Model;

    ULONG MaxBulkInTransfer;
    ULONG BulkInHeaderSize;

    BOOLEAN ChainedMdlsSupported;

    PIRP UsbSsIrp;
    USB_IDLE_CALLBACK_INFO UsbSsCallback;

    KEVENT UsbSsIrpComplete;

    PPOWER_FILTER_LOOKUP PowerFilterTable;

    PUCHAR SyncInterruptReadBuffer;

    UCHAR InterruptReassemnblyBuffer[INTERRUPT_REASSEMBLY_BUFFER_SIZE];
    ULONG CurrentOffset;
    ULONG ExpectLength;

    USHORT MbimVersion;
    USHORT MbimExtendedVersion;
    BOOLEAN RemoteWakeCapable;

    USB_CAP_DEVICE_INFO UsbCapDeviceInfo;
    PVOID ModemContext;

} BUS_OBJECT, *PBUS_OBJECT;

typedef struct _WDF_DEVICE_INFO
{

    PBUS_OBJECT BusObject;

} WDF_DEVICE_INFO, *PWDF_DEVICE_INFO;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(WDF_DEVICE_INFO, GetWdfDeviceInfo)

//
// put this in the NIC data structure
//
typedef struct _USB_DEVICE_CONTEXT
{
    size_t NumberOfPipes;

    WDFWAITLOCK PipeStateLock;

    UCHAR UsbCommunicationInterfaceIndex;
    UCHAR UsbDataInterfaceIndex;

    UCHAR UsbCommunicationInterfaceSetting;
    UCHAR UsbDataInterfaceSetting;

    UCHAR WdfCommunicationInterfaceIndex;
    UCHAR WdfDataInterfaceIndex;

    BOOLEAN BulkInputPipeConfigured;
    BOOLEAN BulkInputPipeStarted;
    ULONG BulkInputPipeMaxPacket;
    WDFUSBPIPE BulkInputPipe;

    BOOLEAN BulkOutputPipeConfigured;
    BOOLEAN BulkOutputPipeStarted;
    ULONG BulkOutputPipeMaxPacket;
    WDFUSBPIPE BulkOutputPipe;

    WDFUSBPIPE InterruptPipe;

    WDFIOTARGET InterruptPipeIoTarget;
    ULONG InterruptPipeMaxPacket;

    WDFLOOKASIDE LookasideList;

    WDFSPINLOCK WriteCollectionLock;
    WDFCOLLECTION WriteRequestCollection;

    WDFWORKITEM BulkPipeResetWorkitem;
    LONG BulkPipeResetFlag;
    EX_RUNDOWN_REF BulkPipeResetRundown;
    PBUS_OBJECT BusObject;
} USB_DEVICE_CONTEXT, *PUSB_DEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(USB_DEVICE_CONTEXT, GetUsbDeviceContext)

typedef struct _REQUEST_CONTEXT
{

    PVOID CallbackContext;
    union
    {

        MBB_BUS_SEND_COMPLETION_CALLBACK Send;
        MBB_BUS_RECEIVE_COMPLETION_CALLBACK Receive;

    } Callback;

    MBB_PROTOCOL_HANDLE ProtocolHandle;

    PUSB_DEVICE_CONTEXT UsbDeviceContext;

    PVOID Buffer;
    ULONG BufferLength;

} REQUEST_CONTEXT, *PREQUEST_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(REQUEST_CONTEXT, GetRequestContext)

VOID MbbBusMiniportUsbIdle(PVOID Context);

NTSTATUS
PreAllocateWriteRequests(WDFUSBDEVICE UsbDevice);

#define IS_USB_DEVICE_REMOTE_WAKE_CAPABLE(deviceInformation) \
    ((deviceInformation.Traits & WDF_USB_DEVICE_TRAIT_REMOTE_WAKE_CAPABLE) > 0 ? TRUE : FALSE);
