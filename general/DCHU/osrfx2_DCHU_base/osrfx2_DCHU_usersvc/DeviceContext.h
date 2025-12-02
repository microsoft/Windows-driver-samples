/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

--*/

#pragma once

//
// Device list
//
typedef struct _DEVICE_LIST_ENTRY {
    struct _DEVICE_LIST_ENTRY *Flink;
    struct _DEVICE_LIST_ENTRY *Blink;
} DEVICE_LIST_ENTRY, *PDEVICE_LIST_ENTRY;

FORCEINLINE
VOID
InitializeDeviceListHead(
    _Out_ PDEVICE_LIST_ENTRY Head
    )
{
    Head->Blink = Head->Flink = Head;
}

FORCEINLINE
BOOL
IsDeviceListEmpty(
    _In_ const PDEVICE_LIST_ENTRY ListHead
    )

{
    return (ListHead->Flink == ListHead);
}

FORCEINLINE
VOID
RemoveDeviceListEntry(
    _In_ PDEVICE_LIST_ENTRY Entry
    )
{
    PDEVICE_LIST_ENTRY Prev = Entry->Blink;
    PDEVICE_LIST_ENTRY Next = Entry->Flink;

    Prev->Flink = Next;
    Next->Blink = Prev;
}

FORCEINLINE
VOID
InsertTailDeviceListEntry(
    _Inout_ PDEVICE_LIST_ENTRY Head,
    _Inout_ PDEVICE_LIST_ENTRY Entry
    )
{
    PDEVICE_LIST_ENTRY Tail = Head->Blink;

    Tail->Flink = Entry;
    Entry->Blink = Tail;
    Entry->Flink = Head;
    Head->Blink = Entry;
}

//
// Context for device handle
//
typedef struct _DEVICE_CONTEXT {
    PWSTR             SymbolicLink;
    HANDLE            DeviceHandle;
    HCMNOTIFICATION   DeviceNotificationHandle;
    BOOL              Unregistered;
    CRITICAL_SECTION  ContextLock;
    DEVICE_LIST_ENTRY ListEntry;
} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

#define CONTAINING_DEVICE_RECORD(address) ((PDEVICE_CONTEXT)( \
                                            (PCHAR)(address) - \
                                            (ULONG_PTR)(&((PDEVICE_CONTEXT)0)->ListEntry)))

//
// Device interface context
//
DWORD
SetupDeviceInterfaceContext(
    VOID
    );

VOID
CleanupDeviceInterfaceContext(
    VOID
    );

//
// Device notifications related
//
VOID
DeviceQueryRemoveAction(
    _In_ PDEVICE_CONTEXT Context
    );

VOID
DeviceQueryRemoveFailedAction(
    _In_ PDEVICE_CONTEXT Context
    );

VOID
DeviceRemoveCompleteAction(
    _In_ PDEVICE_CONTEXT Context
    );

DWORD
WINAPI
DeviceCallback(
    _In_ HCMNOTIFICATION       hNotify,
    _In_ PVOID                 hContext,
    _In_ CM_NOTIFY_ACTION      Action,
    _In_ PCM_NOTIFY_EVENT_DATA EventData,
    _In_ DWORD                 EventDataSize
    );

DWORD
RegisterDeviceNotifications(
    _In_ PCWSTR DeviceInterfacePath
    );

VOID
UnregisterDeviceNotifications(
    _Inout_ PDEVICE_CONTEXT Context
    );

DWORD
WINAPI
InterfaceCallback(
    _In_ HCMNOTIFICATION       hNotify,
    _In_ PVOID                 hContext,
    _In_ CM_NOTIFY_ACTION      Action,
    _In_ PCM_NOTIFY_EVENT_DATA EventData,
    _In_ DWORD                 EventDataSize
    );

DWORD
RegisterInterfaceNotifications(
    _Out_ PHCMNOTIFICATION pInterfaceNotificationHandle
    );

DWORD
WINAPI
UnregisterDeviceNotificationsWorkerThread(
    _In_ PVOID lpThreadParameter
    );
