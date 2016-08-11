/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2000-2003
*
*
*  PUBLISHED TITLE : WiaCI.h
*
*  VERSION:     1.0
*
*  AUTHOR:      KeisukeT
*
*  DATE:        11 Mar, 2003
*
*  DESCRIPTION:
*  Header file for class installer exports.
*
*******************************************************************************/




#ifndef _DEVMGR_H_
#define _DEVMGR_H_


//
// Include
//
#include <objbase.h>

//
// Define
//

#define WIA_DEVSEARCH_DRVKEY        0x00000001
#define WIA_DEVSEARCH_DEVICEDATA    0x00000002


#define MAX_FRIENDLYNAME        64
#define MAX_DEVICE_ID           64

//
// Struct
//

typedef struct _WIADEVICEINSTALL {
 IN     DWORD dwSize;                           // Size of the structure.
 IN     DWORD dwFlags;                          // Reserved, must be 0.
 IN     WCHAR szInfPath[MAX_PATH];              // Full path to the INF file.
 IN     WCHAR szPnPID[MAX_PATH];                // PnP ID string for INF install.
 IN     WCHAR szIhvID[MAX_PATH];                // IHV unique ID, will be in DeviceData.
 IN OUT WCHAR szFriendlyName[MAX_FRIENDLYNAME]; // Specify name, result will be stored too.
    OUT WCHAR szWiaDeviceID[MAX_DEVICE_ID];     // WIA Device ID upon successful install.
 } WIADEVICEINSTALL, *PWIADEVICEINSTALL;


//
// Prototype
//

DWORD
WINAPI
InstallWiaDevice(
    _In_ PWIADEVICEINSTALL pWiaDeviceInstall
    );

DWORD
WINAPI
UninstallWiaDevice(
    _In_ HANDLE  hWiaDeviceList,
         DWORD   dwIndex
    );

DWORD
WINAPI
CreateWiaDeviceList(
                                       DWORD   dwFlags,    
    _In_opt_                           LPCWSTR szQueryEntry,
    _In_reads_bytes_(dwQueryParameterSize)  PVOID   pvQueryParameter,
                                       DWORD   dwQueryParameterSize,
    _Out_opt_                          HANDLE  *phWiaDeviceList
    );

DWORD
WINAPI
DestroyWiaDeviceList(
    _In_ HANDLE  hWiaDeviceList
    );

DWORD
WINAPI
GetWiaDeviceProperty(
    _In_            HANDLE  hWiaDeviceList,
                    DWORD   dwIndex,
                    DWORD   dwFlags,
    _In_opt_        LPCWSTR szEntry,
    _Out_opt_       LPDWORD pdwType,
    _Out_opt_       PVOID   pvBuffer,
    _Inout_opt_     LPDWORD pdwBufferSize
    );

DWORD
WINAPI
SetWiaDeviceProperty(
    _In_ HANDLE  hWiaDeviceList,
    DWORD   dwIndex,
    DWORD   dwFlags,
    _In_opt_ LPCWSTR szEntry,
    DWORD   dwType,
    _In_reads_bytes_(dwBufferSize) PVOID   pvBuffer,
    DWORD   dwBufferSize
    );

DWORD
WINAPI
EnableWiaDevice(
    _In_ HANDLE  hWiaDeviceList,
         DWORD   dwIndex
    );


DWORD
WINAPI
DisableWiaDevice(
    _In_ HANDLE  hWiaDeviceList,
         DWORD   dwIndex
    );


#endif // _DEVMGR_H_





