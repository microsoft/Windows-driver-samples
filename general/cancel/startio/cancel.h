#ifndef __CANCEL_H
#define __CANCEL_H

#include <initguid.h>

//
// Since this driver is a legacy driver and gets installed as a service 
// (without an INF file),  we will define a class guid for use in 
// IoCreateDeviceSecure function. This would allow  the system to store
// Security, DeviceType, Characteristics and Exclusivity information of the
// deviceobject in the registery under 
// HKLM\SYSTEM\CurrentControlSet\Control\Class\ClassGUID\Properties. 
// This information can be overrided by an Administrators giving them the ability
// to control access to the device beyond what is initially allowed 
// by the driver developer.
//
 

// {5D006E1A-2631-466c-B8A0-32FD498E4424}  - generated using guidgen.exe
DEFINE_GUID (GUID_DEVCLASS_CANCEL_SAMPLE,
        0x5d006e1a, 0x2631, 0x466c, 0xb8, 0xa0, 0x32, 0xfd, 0x49, 0x8e, 0x44, 0x24);

//
// GUID definition are required to be outside of header inclusion pragma to 
// avoid error during precompiled headers.
//
#include <ntddk.h>
#include <wdmsec.h> // for IoCreateDeviceSecure
#include <dontuse.h>

//  Debugging macros

#if DBG
#define CSAMP_KDPRINT(_x_) \
                DbgPrint("CANCEL.SYS: ");\
                DbgPrint _x_;

#define TRAP()  DbgBreakPoint()

#else

#define CSAMP_KDPRINT(_x_)

#define TRAP()

#endif

#define CSAMP_DEVICE_NAME_U     L"\\Device\\CANCELSAMP"
#define CSAMP_DOS_DEVICE_NAME_U L"\\DosDevices\\CancelSamp"
#define CSAMP_RETRY_INTERVAL    500*1000 //500 ms
#define TAG (ULONG)'MASC'

typedef struct _INPUT_DATA{

    ULONG Data; //device data is stored here

} INPUT_DATA, *PINPUT_DATA;

typedef struct _DEVICE_EXTENSION{
  
    // Irps waiting to be processed are queued here
    LIST_ENTRY   PendingIrpQueue;

    //  SpinLock to protect access to the queue
    KSPIN_LOCK QueueLock;

    // SpinLock to provide exclusive access to the port
    KSPIN_LOCK DeviceLock;

    // Pointer to current device IRP. Exclusive access to this
    // field is also provided by the QueueLock.
    PIRP CurrentIrp;

    // Customtimer DPC object
    KDPC PollingDpc;

    // Time at which the device was last polled
    LARGE_INTEGER LastPollTime;  

    // Polling timer object
    KTIMER  PollingTimer;

    // Polling interval (retry interval)
    LARGE_INTEGER PollingInterval;

    IO_CSQ CancelSafeQueue;   
        
}  DEVICE_EXTENSION, *PDEVICE_EXTENSION;

typedef struct _FILE_CONTEXT{
    //
    // Lock to rundown threads that are dispatching I/Os on a file handle 
    // while the cleanup for that handle is in progress.
    //
    IO_REMOVE_LOCK  FileRundownLock;
} FILE_CONTEXT, *PFILE_CONTEXT;

DRIVER_INITIALIZE DriverEntry;

_Dispatch_type_(IRP_MJ_CREATE)
_Dispatch_type_(IRP_MJ_CLOSE)
DRIVER_DISPATCH CsampCreateClose;

_Dispatch_type_(IRP_MJ_CLEANUP)
DRIVER_DISPATCH CsampCleanup;

_Dispatch_type_(IRP_MJ_READ)
DRIVER_DISPATCH CsampRead;

DRIVER_DISPATCH CsampPollDevice;

DRIVER_UNLOAD CsampUnload;

KDEFERRED_ROUTINE CsampPollingTimerDpc;

VOID 
CsampInitiateIo(
   _In_ PDEVICE_OBJECT DeviceObject
);

NTSTATUS 
CsampInsertIrp (
    _In_ PIO_CSQ   Csq,
    _In_ PIRP              Irp,
    _In_ PVOID    InsertContext
   );

VOID 
CsampRemoveIrp(
    _In_  PIO_CSQ Csq,
    _In_  PIRP    Irp
   );

PIRP 
CsampPeekNextIrp(
    _In_  PIO_CSQ Csq,
    _In_  PIRP    Irp,
    _In_  PVOID  PeekContext
   );

_IRQL_raises_(DISPATCH_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_Acquires_lock_(CONTAINING_RECORD(Csq,DEVICE_EXTENSION, CancelSafeQueue)->QueueLock)
VOID
CsampAcquireLock(
    _In_                                   PIO_CSQ Csq,
    _Out_ _At_(*Irql, _Post_ _IRQL_saves_) PKIRQL  Irql
   );

_IRQL_requires_(DISPATCH_LEVEL)
_Releases_lock_(CONTAINING_RECORD(Csq,DEVICE_EXTENSION, CancelSafeQueue)->QueueLock)
VOID
CsampReleaseLock(
    _In_                    PIO_CSQ Csq,
    _In_ _IRQL_restores_    KIRQL   Irql
   );

VOID 
CsampCompleteCanceledIrp(
    _In_  PIO_CSQ             pCsq,
    _In_  PIRP                Irp
   );

#endif


