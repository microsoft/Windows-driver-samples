/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.


Module Name:

   Event.h

--*/

#ifndef __EVENT__
#define __EVENT__

//
// DEFINES
//

#define NTDEVICE_NAME_STRING      L"\\Device\\Event_Sample"
#define SYMBOLIC_NAME_STRING      L"\\DosDevices\\Event_Sample"
#define TAG (ULONG)'TEVE'

#if DBG
#define DebugPrint(_x_) \
                DbgPrint("EVENT.SYS: ");\
                DbgPrint _x_;

#else

#define DebugPrint(_x_)

#endif

//
// DATA
//
typedef struct _DEVICE_EXTENSION {
    PDEVICE_OBJECT  Self;
    LIST_ENTRY      EventQueueHead; // where all the user notification requests are queued
    KSPIN_LOCK      QueueLock;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


typedef struct _NOTIFY_RECORD{
    NOTIFY_TYPE     Type;
    LIST_ENTRY      ListEntry;
    union {
        PKEVENT     Event;
        PIRP        PendingIrp;
    } Message;
    KDPC            Dpc;
    KTIMER          Timer;
    PFILE_OBJECT    FileObject;
    PDEVICE_EXTENSION   DeviceExtension;
    BOOLEAN         CancelRoutineFreeMemory;
} NOTIFY_RECORD, *PNOTIFY_RECORD;

typedef struct _FILE_CONTEXT{
    //
    // Lock to rundown threads that are dispatching I/Os on a file handle 
    // while the cleanup for that handle is in progress.
    //
    IO_REMOVE_LOCK  FileRundownLock;
} FILE_CONTEXT, *PFILE_CONTEXT;

//
// Function prototypes
//


DRIVER_INITIALIZE DriverEntry;

_Dispatch_type_(IRP_MJ_CREATE)
_Dispatch_type_(IRP_MJ_CLOSE)
DRIVER_DISPATCH EventCreateClose;

_Dispatch_type_(IRP_MJ_CLEANUP)
DRIVER_DISPATCH EventCleanup;

_Dispatch_type_(IRP_MJ_DEVICE_CONTROL)
DRIVER_DISPATCH EventDispatchIoControl;

DRIVER_UNLOAD EventUnload;

DRIVER_CANCEL EventCancelRoutine;

KDEFERRED_ROUTINE CustomTimerDPC;

DRIVER_DISPATCH RegisterEventBasedNotification;

DRIVER_DISPATCH RegisterIrpBasedNotification;


#endif // __EVENT__
