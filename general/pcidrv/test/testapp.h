/*++
Copyright (c) 1990-2000    Microsoft Corporation All Rights Reserved

Module Name:

    notify.h

Abstract:

--*/

#ifndef __TESTAPP_H
#define __TESTAPP_H

#pragma warning(disable:4214 4201 4115 4100)

#define UNICODE 1
#define INITGUID

#include <windows.h>  // for using Windows data types and functions
#include <winsock2.h> // for using winsock utility inet_addr/ntohs functions.
#include <setupapi.h> // for using SetupDi functions
#include <dbt.h>       // for PNP device notification interfaces
#include <winioctl.h>  // for defining ioctls
#include <ntddndis.h>  // for NDIS OIDs
#include <strsafe.h>  // for safe string functions
#include <devguid.h> // for GUID_DEVCLASS_NET
#include <cfgmgr32.h>  // for MAX_GUID_STRING_LEN
#include <OBJBASE.H> // for CLSIDFromString. Link to ole32.lib

#include "public.h"
#include "resource.h"
#include <dontuse.h>

//
// Registry path where the IP addresses are saved
//
#define REG_PATH L"Software\\Microsoft\\PCIDRV\\MyPing"
#define DEF_SOURCE_IP L"192.168.0.2"
#define DEF_DEST_IP L"192.168.0.1"

#define PING_SLEEP_TIME        100
#define MAC_ADDR_LEN            6

#define MAX_LEN                 64
#define MAX_PAYLOAD_SIZE        1428
#define MIN_PAYLOAD_SIZE        32
#define MAX_PING_RETRY          10

extern BOOLEAN     Verbose;

typedef struct _DEVICE_INFO
{
    LIST_ENTRY      ListEntry;
    HANDLE          hDevice; // file handle
    HDEVNOTIFY      hHandleNotification; // notification handle
    TCHAR           DeviceName[MAX_PATH];// friendly name of device description
    TCHAR           DevicePath[MAX_PATH];//
    ULONG           DeviceIndex; // Serial number of the device.
    CHAR            SourceIp[MAX_LEN];
    CHAR            DestIp[MAX_LEN];
    WCHAR           UnicodeSourceIp[MAX_LEN];
    WCHAR           UnicodeDestIp[MAX_LEN];
    ULONG           PacketSize;
    UCHAR           SrcMacAddr[MAC_ADDR_LEN];
    UCHAR           TargetMacAddr[MAC_ADDR_LEN];
    HANDLE          PingEvent;
    ULONG           NumberOfRequestSent;
    BOOL            Sleep;
    ULONG           TimeOut;
    BOOL            IsANetworkMiniport;
    BOOLEAN         ExitThread;
    HANDLE          ThreadHandle;

} DEVICE_INFO, *PDEVICE_INFO;


typedef struct _DIALOG_RESULT
{
    ULONG   DeviceIndex;
    WCHAR   SourceIp[MAX_LEN];
    WCHAR   DestIp[MAX_LEN];
    ULONG   PacketSize;
} DIALOG_RESULT, *PDIALOG_RESULT;


//
// Copied Macros from ntddk.h. Used to using the kernel-mode
// style of linked list.
//

#define CONTAINING_RECORD(address, type, field) ((type *)( \
                          (PCHAR)(address) - \
                          (ULONG_PTR)(&((type *)0)->field)))


#define InitializeListHead(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = (ListHead))

#define RemoveHeadList(ListHead) \
    (ListHead)->Flink;\
    {RemoveEntryList((ListHead)->Flink)}

#define IsListEmpty(ListHead) \
    ((ListHead)->Flink == (ListHead))


#define RemoveEntryList(Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_Flink;\
    _EX_Flink = (Entry)->Flink;\
    _EX_Blink = (Entry)->Blink;\
    _EX_Blink->Flink = _EX_Flink;\
    _EX_Flink->Blink = _EX_Blink;\
    }

#define InsertTailList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Blink = _EX_ListHead->Blink;\
    (Entry)->Flink = _EX_ListHead;\
    (Entry)->Blink = _EX_Blink;\
    _EX_Blink->Flink = (Entry);\
    _EX_ListHead->Blink = (Entry);\
    }


#ifndef min
#define min(_a, _b)     (((_a) < (_b)) ? (_a) : (_b))
#endif

#ifndef max
#define max(_a, _b)     (((_a) > (_b)) ? (_a) : (_b))
#endif

#define DisplayV(pstrFormat, ...) if (Verbose) {Display(pstrFormat, __VA_ARGS__);}

LRESULT FAR PASCAL
WndProc (
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

BOOLEAN EnumExistingDevices(
    HWND   hWnd
    );

BOOL HandleDeviceInterfaceChange(
    HWND hwnd,
    DWORD evtype,
    PDEV_BROADCAST_DEVICEINTERFACE dip
    );

BOOL HandleDeviceChange(
    HWND hwnd,
    DWORD evtype,
    PDEV_BROADCAST_HANDLE dhp
    );

LRESULT
HandleCommands(
    HWND     hWnd,
    UINT     uMsg,
    WPARAM   wParam,
    LPARAM   lParam
    );

BOOL
HandlePowerBroadcast(
    HWND hWnd,
    WPARAM wParam,
    LPARAM lParam);

BOOLEAN Cleanup(
    HWND hWnd
    );

_Success_(return != FALSE)
BOOL
GetDeviceDescription(
    _In_ LPTSTR DevPath,
    _Out_writes_bytes_all_(OutBufferLen) LPTSTR OutBuffer,
    _In_ ULONG OutBufferLen,
    BOOL   *NetClassDevice
);

BOOLEAN
OpenDevice(
    _In_ HWND HWnd,
    _In_ PDEVICE_INFO DeviceInfo
    );


INT_PTR CALLBACK
DlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam);

DWORD
PingThread (
    PDEVICE_INFO DeviceInfo
    );

VOID Display(
    _In_ LPWSTR pstrFormat,
    ...
    ) ;


BOOL
CreatePingThread(
    PDEVICE_INFO DeviceInfo
    );

PDEVICE_INFO
FindDeviceInfo(
    PDIALOG_RESULT InputInfo
    );

VOID
FreeDeviceInfo(
    _In_ PDEVICE_INFO DeviceInfo
    );

PDEVICE_INFO
CreateDeviceInfo(
    _In_ LPWSTR DevicePath
    );

VOID
TerminatePingThread(
    PDEVICE_INFO DeviceInfo
    );

_Success_(return)
BOOL
GetRegistryInfo(
    _Out_writes_bytes_(* SourceIPLen) PWSTR SourceIP,
    _Inout_ LPDWORD SourceIPLen,
    _Out_writes_bytes_(* DestinationIPLen) PWSTR  DestinationIP,
    _Inout_ LPDWORD DestinationIPLen
    );

BOOL
SetRegistryInfo(
    _In_reads_bytes_(SourceIPLen) LPWSTR SourceIP,
    _In_ DWORD SourceIPLen,
    _In_reads_bytes_(DestinationIPLen) LPWSTR  DestinationIP,
    _In_ DWORD DestinationIPLen
    );


#endif


