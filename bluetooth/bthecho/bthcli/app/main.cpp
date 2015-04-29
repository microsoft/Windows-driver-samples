/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Main.cpp

Abstract:

    A simple test for bthecho sample.

Environment:

    user mode only

--*/

#define INITGUID

#include <windows.h>
#include <strsafe.h>
#include <stdio.h>
#include <stdlib.h>
#include <cfgmgr32.h>

#include "public.h"

char testData[] = "WDF Bluetooth Sample Echo";
char replyData[sizeof(testData)];

DWORD
GetDevicePath(
    _In_ LPGUID InterfaceGuid,
    _Out_ LPWSTR * DevicePath
    );

BOOL
DoEcho(
    HANDLE hDevice
    );

VOID 
__cdecl 
wmain()
{
    HANDLE hDevice = INVALID_HANDLE_VALUE;
    BOOL echo = TRUE;
    LPWSTR devicePath = NULL;
    DWORD err = GetDevicePath((LPGUID)&BTHECHOSAMPLE_DEVICE_INTERFACE, &devicePath);

    if (ERROR_SUCCESS != err) {
        printf("Failed to find the BTHECHO device\n");
        exit(1);
    }

    hDevice = CreateFile(devicePath,
                         GENERIC_READ|GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL,
                         OPEN_EXISTING,
                         0,
                         NULL );

    if (hDevice == INVALID_HANDLE_VALUE) {
        printf("Failed to open device. Error %d\n",GetLastError());
        LocalFree(devicePath);
        exit(1);
    } 

    printf("Opened device successfully\n");

    while (echo)
    {
        echo = DoEcho(hDevice);
    }
                
    if (INVALID_HANDLE_VALUE != hDevice) {
        CloseHandle(hDevice);
        printf("Closed device\n");
    }

    if (NULL != devicePath) {
        LocalFree(devicePath);
    }
}

BOOL
DoEcho(
    HANDLE hDevice
    )
{
    BOOL retval = FALSE;
    DWORD cbWritten = 0;
    DWORD cbRead = 0;
    
    BOOL bRet = WriteFile(
        hDevice,
        testData,
        sizeof(testData),
        &cbWritten,
        NULL
        );

    if (!bRet)
    {
        printf("Write failed. Error: %d\n", GetLastError());
        goto exit;
    }
    else
    {
        printf("Written \t\t%d bytes: %s\n", cbWritten, testData);
    }

    bRet = ReadFile(
        hDevice,
        replyData,
        cbWritten,
        &cbRead,
        NULL
        );

    if (!bRet)
    {
        printf("Read failed. Error: %d\n", GetLastError());
        goto exit;
    }
    else
    {
        printf("Reply from server \t%d bytes: %s\n", cbRead, replyData);
    }

    if (cbRead == cbWritten)
    {
        retval = memcmp(replyData, testData, cbRead) ? FALSE : TRUE;
    }

exit:
    return retval;
}

DWORD
GetDevicePath(
    _In_ LPGUID InterfaceGuid,
    _Out_ LPWSTR * DevicePath
    )
{
    CONFIGRET cmRet = CR_SUCCESS;
    ULONG interfaceListSize = 0;

    *DevicePath = NULL;

    cmRet = CM_Get_Device_Interface_List_Size_Ex(&interfaceListSize, InterfaceGuid, 0, CM_GET_DEVICE_INTERFACE_LIST_PRESENT, NULL);
    if (cmRet != CR_SUCCESS) {
        goto exit;
    }

    *DevicePath = (LPWSTR)LocalAlloc(LMEM_ZEROINIT, interfaceListSize * sizeof((*DevicePath)[0]));
    if (NULL == *DevicePath) {
        cmRet = CR_OUT_OF_MEMORY;
        goto exit;
    }

    cmRet = CM_Get_Device_Interface_List_Ex(InterfaceGuid, 0, *DevicePath, interfaceListSize, CM_GET_DEVICE_INTERFACE_LIST_PRESENT, NULL);

exit:

    if (cmRet != CR_SUCCESS) {
        if (NULL != *DevicePath) {
            LocalFree(*DevicePath);
            *DevicePath = NULL;
        }
    }
    
    return CM_MapCrToWin32Err(cmRet, ERROR_UNIDENTIFIED_ERROR);
}

