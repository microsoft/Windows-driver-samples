/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    driver.h

Abstract:

    This module contains the common private declarations for
    for the Serial HCI bus driver.

Environment:

    kernel mode only

--*/

#ifndef DRIVER_H
#define DRIVER_H

#include <ntddk.h>
#include <wdf.h>

#define NTSTRSAFE_LIB
#include <ntstrsafe.h>

#define INITGUID
#include <guiddef.h>
#include <ntddser.h>    // Constants and types for access Serial device

#include <BthXDDI.h>    // BT Extensible Transport DDI

#include "device.h"     // Device specific
#include "io.h"         // Read pump
#include "debugdef.h"   // WPP trace
#include "public.h"     // Share between driver and application

#ifdef DEFINE_GUID

//
// Container ID for internally connected device
//
DEFINE_GUID(GUID_CONTAINERID_INTERNALLY_CONNECTED_DEVICE,
        0x00000000, 0x0000, 0x0000, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff);

//{00000000-0000-0000-ffff-ffffffffffff}

#endif // #ifdef DEFINE_GUID

//
// Define HCI event code
//
#ifndef CommandComplete
#define CommandComplete 0x0e
#endif
#ifndef CommandStatus
#define CommandStatus   0x0f
#endif

#define POOLTAG_UARTHCIBUSSAMPLE  'SBHS'  // 'S'erial 'H'ci 'B'us 'S'ample
#undef ExAllocatePool
#define ExAllocatePool(type, size) \
        ExAllocatePoolWithTag(type, size, POOLTAG_UARTHCIBUSSAMPLE)

//
// An ID used to uniquely identify Bluetooth function from other function
// of this multifunction device.
//
#define BLUETOOTH_FUNC_IDS  0x1001


//
// Device's idle state capability
//
typedef enum _IDLE_CAP_STATE {
    IdleCapActiveOnly   = 1,   // Support active only (cannot idle)
    IdleCapCanWake      = 2,   // Can enter D2 (idle) and remote wake to save power while in idle state.
    IdleCapCanTurnOff   = 3    // Can enter D3 (off) and not remote wake to save max power while device is off.
} IDLE_CAP_STATE;

#ifdef DYNAMIC_ENUM
//
// The goal of the identification and address description abstractions is that enough
// information is stored for a discovered device so that when it appears on the bus,
// the framework (with the help of the driver writer) can determine if it is a new or
// existing device.  The identification and address descriptions are opaque structures
// to the framework, they are private to the driver writer.  The only thing the framework
// knows about these descriptions is what their size is.
// The identification contains the bus specific information required to recognize
// an instance of a device on its the bus.  The identification information usually
// contains device IDs along with any serial or slot numbers.
// For some buses (like USB and PCI), the identification of the device is sufficient to
// address the device on the bus; in these instances there is no need for a separate
// address description.  Once reported, the identification description remains static
// for the lifetime of the device.  For example, the identification description that the
// PCI bus driver would use for a child would contain the vendor ID, device ID,
// subsystem ID, revision, and class for the device. This sample uses only identification
// description.
// On other busses (like 1394 and auto LUN SCSI), the device is assigned a dynamic
// address by the hardware (which may reassigned and updated periodically); in these
// instances the driver will use the address description to encapsulate this dynamic piece
// of data.    For example in a 1394 driver, the address description would contain the
// device's current generation count while the identification description would contain
// vendor name, model name, unit spec ID, and unit software version.
//
typedef struct _PDO_IDENTIFICATION_DESCRIPTION
{
    WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER Header; // should contain this header

    //
    // Unique serail number of the device on the bus
    //
    ULONG SerialNo;

    size_t CchHardwareIds;

    _Field_size_bytes_(CchHardwareIds) PWCHAR HardwareIds;

} PDO_IDENTIFICATION_DESCRIPTION, *PPDO_IDENTIFICATION_DESCRIPTION;
#endif  // #ifdef DYNAMIC_ENUM

typedef struct _UART_READ_CONTEXT *PUART_READ_CONTEXT;

//
// Bus driver's FDO (Function Device Object) extension structure used to maintain device
// properties and state.
//

typedef struct _FDO_EXTENSION
{
    WDFWAITLOCK ChildLock;

    //
    // Radio On/Off state
    //
    BOOLEAN IsRadioEnabled;

    //
    // WDF Device handle
    //
    WDFDEVICE WdfDevice;

    //
    // Serial port IO Target where we send IOCTL/READ/WRITE reuquest to
    //
    WDFIOTARGET IoTargetSerial;

    //
    // (optional) GPIO IO Target to enable serial bus device
    //
    WDFIOTARGET IoTargetGPIO;

    //
    // Bluetooth child dev node (PDO) capabilities
    //
    BTHX_CAPABILITIES  BthXCaps;

    //
    // Indicator if UART is properly initialize; may require re-inialization
    // when tranistion from exiting D0 to resume D0.
    //
    BOOLEAN DeviceInitialized;

    //
    // Cached UART controller connection IDs
    //
    LARGE_INTEGER UARTConnectionId;

    //
    // Cached I2C controller connection IDs
    //
    LARGE_INTEGER I2CConnectionId;

    //
    // Cached GPIO controller connection IDs
    //
    LARGE_INTEGER GPIOConnectionId;

    //
    // Preallocate WDF Requests for synchronous operation like serial port settings
    //
    WDFREQUEST  RequestIoctlSync;

    //
    // Preallocate WDF Requests to wait on serial error event
    //
    WDFREQUEST  RequestWaitOnError;

    //
    // Data return from serial event wait mask IOCTL
    //
    ULONG   SerErrorMask;

    //
    // WDM memory use for Wait Mask event
    //
    WDFMEMORY   WaitMaskMemory;

    //
    // Set if a hardware error (e.g. data overrun in UART FIFO) is detected
    //
    BOOLEAN    HardwareErrorDetected;

    //
    // Indication the state of the read pump (TRUE = active)
    //
    BOOLEAN     ReadPumpRunning;

    //
    // Track number of out-of-sync error that has been detected
    //
    ULONG   OutOfSyncErrorCount;

    //
    // Locks for synchronization for list and queue
    //
    WDFSPINLOCK  QueueAccessLock;

    //
    // Track next packet read (one and only one)
    //
    UART_READ_CONTEXT ReadContext;

    //
    // Preallocated local WDF requested and memory object that is reused to
    // implement read pump
    //
    WDFREQUEST  ReadRequest;
    WDFMEMORY   ReadMemory;
    UCHAR       ReadBuffer[MAX_H4_HCI_PACKET_SIZE];

#if DBG
    //
    // Track last completed HCI packet
    //
    UCHAR       LastPacket[MAX_H4_HCI_PACKET_SIZE];
    ULONG       LastPacketLength;
#endif
    //
    // WDF Queue for HCI event Request and total number of such request recevied
    //
    WDFQUEUE    ReadEventQueue;
    LONG        EventQueueCount;

    //
    // List to store (prefetched) incoming HCI events and number of entries
    //
    LIST_ENTRY  ReadEventList;
    LONG        EventListCount;

    //
    // WDF Queue for HCI read data Request and total number of such request recevied
    //
    WDFQUEUE    ReadDataQueue;
    LONG        DataQueueCount;

    //
    // List to store (prefetched) incoming HCI data and number of entries
    //
    LIST_ENTRY  ReadDataList;
    LONG        DataListCount;

    //
    // Counts used to track HCI requests received and completed for various packet types
    //
    LONG       CntCommandReq;          // Track total number of HCI command Requests
    LONG       CntCommandCompleted;    // Number of HCI Command completed

    LONG       CntEventReq;            // Track total number of HCI Event Requests
    LONG       CntEventCompleted;      // Number of HCI Command completed

    LONG       CntWriteDataReq;        // Track total number of HCI Write Data requests
    LONG       CntWriteDataCompleted;  // Number of HCI (write) Data completed

    LONG       CntReadDataReq;         // Track total number of HCI Read Data Requests
    LONG       CntReadDataCompleted;   // Number of HCI (Read) Data completed
} FDO_EXTENSION, *PFDO_EXTENSION;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FDO_EXTENSION, FdoGetExtension)

//
// Can send IO only if the device (UART) is in the initialized state.
//
#define IsDeviceInitialized(FdoExtension) (FdoExtension->DeviceInitialized)

#define ValidConnectionID(ConnectionId) (ConnectionId.QuadPart != 0)

//
// Bus driver's child PDO (Physical Device Object) extension structure used to maintain this
// PDO's device properties and state.
//

typedef struct _PDO_EXTENSION
{
    //
    // Back pointer to FDO_EXTENSION
    //
    PFDO_EXTENSION FdoExtension;

    //
    // Unique serial number of the device on the bus
    //
    ULONG SerialNo;

} PDO_EXTENSION, *PPDO_EXTENSION;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(PDO_EXTENSION, PdoGetExtension)

//
// Prototypes of functions
//

//
// Driver.c
//

EVT_WDF_OBJECT_CONTEXT_CLEANUP DriverCleanup;

VOID
DriverSetDeviceCallbackEvents(
    _In_  PWDFDEVICE_INIT  _DeviceInit
    );

EVT_WDF_DRIVER_DEVICE_ADD DriverDeviceAdd;

DRIVER_INITIALIZE DriverEntry;

//
// FDO.c
//

NTSTATUS
HlpInitializeFdoExtension(WDFDEVICE   _Device);

NTSTATUS
FdoWriteDeviceIO(_In_ WDFREQUEST        _RequestFromBthport,
                 _In_ WDFDEVICE         _Device,
                 _In_ PFDO_EXTENSION    _FdoExtension,
                 _In_ PBTHX_HCI_READ_WRITE_CONTEXT _HCIContext);

NTSTATUS
FdoWriteToDeviceSync(_In_ WDFIOTARGET  _IoTargetSerial,
                     _In_ WDFREQUEST   _RequestWriteSync,
                     _In_ ULONG        _IoControlCode,
                     _In_opt_ ULONG    _InBufferSize,
                     _In_opt_ PVOID    _InBuffer,
                     _Out_ PULONG_PTR  _BytesWritten);

NTSTATUS
DeviceConfigWaitOnError(_In_ WDFIOTARGET    _IoTargetSerial,
                        _In_ WDFREQUEST     _RequestWaitOnError,
                        _In_ WDFMEMORY      _WaitMaskMemory,
                        _In_ PULONG         _ErrorResult,
                        _In_ PFDO_EXTENSION _FdoExtension);

NTSTATUS
HCIContextValidate(ULONG Index,
                   PBTHX_HCI_READ_WRITE_CONTEXT _HCIContext);

// Power policy events
EVT_WDF_DEVICE_ARM_WAKE_FROM_S0     FdoEvtDeviceArmWake;
EVT_WDF_DEVICE_DISARM_WAKE_FROM_S0  FdoEvtDeviceDisarmWake;

EVT_WDF_DEVICE_ARM_WAKE_FROM_SX     FdoEvtDeviceArmWake;
EVT_WDF_DEVICE_DISARM_WAKE_FROM_SX  FdoEvtDeviceDisarmWake;

// PnP events
EVT_WDF_DEVICE_PREPARE_HARDWARE  FdoDevPrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE  FdoDevReleaseHardware;

// Power events
EVT_WDF_DEVICE_D0_ENTRY          FdoDevD0Entry;
EVT_WDF_DEVICE_D0_EXIT           FdoDevD0Exit;

EVT_WDF_DEVICE_SELF_MANAGED_IO_INIT  FdoDevSelfManagedIoInit;
EVT_WDF_DEVICE_SELF_MANAGED_IO_CLEANUP  FdoDevSelfManagedIoCleanup;

// Queue
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL FdoIoQuDeviceControl;

// PDO creation

#ifdef DYNAMIC_ENUM
EVT_WDF_CHILD_LIST_CREATE_DEVICE FdoEvtDeviceListCreatePdo;

NTSTATUS
PdoCreateDynamic(_In_ WDFDEVICE       Device,
                 _In_ PWDFDEVICE_INIT DeviceInit,
                 _In_ PWCHAR          HardwareIds,
                 _In_ ULONG           SerialNo);

NTSTATUS
FdoCreateOneChildDeviceDynamic(_In_ WDFDEVICE  _Device,
                               _In_ PWCHAR     _HardwareIds,
                               _In_ size_t     _CchHardwareIds,
                               _In_ ULONG      _SerialNo);
#endif

EVT_WDF_DEVICE_DISABLE_WAKE_AT_BUS PdoDevDisableWakeAtBus;
EVT_WDF_DEVICE_ENABLE_WAKE_AT_BUS  PdoDevEnableWakeAtBus;

NTSTATUS
FdoCreateOneChildDevice(_In_ WDFDEVICE  _Device,
                        _In_ PWCHAR     _HardwareIds,
                        _In_ ULONG      _SerialNo);

NTSTATUS
FdoCreateAllChildren(_In_ WDFDEVICE _Device);

NTSTATUS
FdoRemoveOneChildDevice(WDFDEVICE   _Device,
                        ULONG       _SerialNo);

NTSTATUS
FdoFindConnectResources(_In_ WDFDEVICE    _Device,
                        _In_ WDFCMRESLIST _ResourcesRaw,
                        _In_ WDFCMRESLIST _ResourcesTranslated);

//
// Pdo.c
//

EVT_WDF_DEVICE_PREPARE_HARDWARE  PdoDevPrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE  PdoDevReleaseHardware;

EVT_WDF_DEVICE_D0_ENTRY         PdoDevD0Entry;
EVT_WDF_DEVICE_D0_EXIT          PdoDevD0Exit;

EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL PdoIoQuDeviceControl;

NTSTATUS
PdoCreate(_In_ WDFDEVICE  _Device,
          _In_ PWSTR      _HardwareIds,
          _In_ ULONG      _SerialNo);

VOID
PdoDevDisableWakeAtBus(_In_ WDFDEVICE _Device);

NTSTATUS
PdoDevEnableWakeAtBus(_In_ WDFDEVICE          _Device,
                      _In_ SYSTEM_POWER_STATE _PowerState);

//
// Define in io.c
//
NTSTATUS ReadResourcesAllocate(_In_ WDFDEVICE _Device);
VOID ReadResourcesFree(_In_ WDFDEVICE _Device);

NTSTATUS
HLP_AllocateResourceForWrite(_In_ WDFDEVICE   _Device,
                             _In_ WDFIOTARGET _IoTargetSerial,
                             _Out_ WDFREQUEST *_pRequest);

VOID
HLP_FreeResourceForWrite(PUART_WRITE_CONTEXT _TransferContext);

EVT_WDF_REQUEST_CANCEL CB_RequestFromBthportCancel;

EVT_WDF_REQUEST_COMPLETION_ROUTINE CR_WriteDeviceIO;

NTSTATUS
ReadRequestComplete(_In_ PFDO_EXTENSION    _FdoExtension,
                    _In_ UCHAR             _Type,
                    _In_ ULONG             _PacketLength,
                    _In_reads_bytes_opt_(_PacketLength) PUCHAR _Packet,
                    _Inout_  WDFQUEUE      _Queue,
                    _Inout_  PLONG         _QueueCount,
                    _Inout_  PLIST_ENTRY   _ListHead,
                    _Inout_  PLONG         _ListCount);

EVT_WDF_REQUEST_COMPLETION_ROUTINE ReadH4PacketCompletionRoutine;

NTSTATUS
ReadH4Packet(_In_ PUART_READ_CONTEXT _ReadContext,
             _In_  WDFREQUEST        _WdfRequest,
             _In_  WDFMEMORY         _WdfMemory,
             _Pre_notnull_ _Pre_writable_byte_size_(_BufferLen) PVOID _Buffer,
             _In_  ULONG             _BufferLen);

//
// Device.c
//

VOID
DeviceQueryDeviceParameters(_In_ WDFDRIVER  _Driver);

BOOLEAN
DeviceInitialize(_In_ PFDO_EXTENSION _FdoExtension,
                 _In_ WDFIOTARGET    _IoTargetSerial,
                 _In_ WDFREQUEST     _RequestSync,
                 _In_ BOOLEAN        _ResetUart);

NTSTATUS
DeviceEnableWakeControl(_In_ WDFDEVICE          _Device,
                        _In_ SYSTEM_POWER_STATE _PowerState);
void
DeviceDisableWakeControl(WDFDEVICE _Device);

NTSTATUS
DeviceEnable(_In_ WDFDEVICE _Device,
             _In_ BOOLEAN   _Enabled);

NTSTATUS
DevicePowerOn(_In_ WDFDEVICE _Device);

NTSTATUS
DevicePowerOff(_In_ WDFDEVICE _Device);

#endif


