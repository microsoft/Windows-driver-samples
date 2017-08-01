/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Main.cpp

Abstract:

    Implements the functions to control the OSR USB FX2 device.

Environment:

    User mode

--*/

#include "Main.h"
#include "Utils.h"

INT             CurrentBar;
BAR_GRAPH_STATE BarGraphState;

/*++

Routine Description:

    Sets the variables in this service to their default values.

Arguments:

    VOID

Return Value:

    VOID

--*/
VOID
SetVariables()
{
    CurrentBar = 0;
}


/*++

Routine Description:

    Retrieves the device path of a given interface.

Arguments:

    InterfaceGuid    - The GUID of the interface to search for

    DevicePath       - The resulting device path

    DevicePathLength - The length of DevicePath

Return Value:

    TRUE if the function succeeded and FALSE otherwise.  Errors
    are logged in the Application event log.

--*/
_Success_(return)
BOOL
GetDevicePath(
    _In_ LPGUID InterfaceGuid,
    _Out_writes_z_(DevicePathLength) PWCHAR DevicePath,
    _In_ size_t DevicePathLength
    )
{
    HRESULT hr = E_FAIL;
    BOOL Return = FALSE;
    CONFIGRET Status = CR_SUCCESS;
    PWSTR DeviceInterfaceList = NULL;
    ULONG DeviceInterfaceListLength = 0;
    PWSTR NextInterface;

    Status = CM_Get_Device_Interface_List_Size(&DeviceInterfaceListLength,
                                               InterfaceGuid,
                                               NULL,
                                               CM_GET_DEVICE_INTERFACE_LIST_PRESENT);

    if (Status != CR_SUCCESS)
    {
        WriteToErrorLog(L"CM_Get_DeviceInterface_List_Size",
                        CM_MapCrToWin32Err(Status, ERROR_FILE_NOT_FOUND));
        goto clean0;
    }

    if (DeviceInterfaceListLength < 1)
    {
        WriteToErrorLog(L"CM_Get_DeviceInterface_List_Size",
                        CM_MapCrToWin32Err(Status, ERROR_EMPTY));
        goto clean0;
    }

    DeviceInterfaceList = (PWSTR)malloc(DeviceInterfaceListLength * sizeof(WCHAR));

    if (DeviceInterfaceList == NULL)
    {
        WriteToEventLog(L"Failed to allocate memory for the device interface list",
                        TRACE_LEVEL_ERROR);
        goto clean0;
    }

    ZeroMemory(DeviceInterfaceList, DeviceInterfaceListLength * sizeof(WCHAR));

    Status = CM_Get_Device_Interface_List(InterfaceGuid,
                                          NULL,
                                          DeviceInterfaceList,
                                          DeviceInterfaceListLength,
                                          CM_GET_DEVICE_INTERFACE_LIST_PRESENT);

    if (Status != CR_SUCCESS)
    {
        WriteToErrorLog(L"CM_Get_Device_Interface_List",
                        CM_MapCrToWin32Err(Status, ERROR_FILE_NOT_FOUND));
        goto clean0;
    }

    if (*DeviceInterfaceList == UNICODE_NULL)
    {
        WriteToEventLog(L"CM_Get_Device_Interface_List returned an empty list",
                        TRACE_LEVEL_ERROR);
    }

    NextInterface = DeviceInterfaceList + wcslen(DeviceInterfaceList) + 1;

    if (*NextInterface != UNICODE_NULL)
    {
        WriteToEventLog(L"More than one device interface instance found.  "
                        "Selecting first matching device.",
                        TRACE_LEVEL_WARNING);
    }

    hr = StringCchCopy(DevicePath, DevicePathLength, DeviceInterfaceList);

    if (FAILED(hr))
    {
        WriteToErrorLog(L"StringCchCopy", HRESULT_CODE(hr));
        goto clean0;
    }

clean0:

    if (DeviceInterfaceList != NULL)
    {
        free(DeviceInterfaceList);
    }

    if (Status == CR_SUCCESS)
    {
        Return = TRUE;
    }

    return Return;
}


/*++

Routine Description:

    Opens up the OSR USB FX2 device handle.

Arguments:

    Synchronous - Whether or not this device should be
                  opened for synchronous access

Return Value:

    The handle to the OSR USB FX2 device.

--*/
_Check_return_
_Ret_notnull_
_Success_(return != INVALID_HANDLE_VALUE)
HANDLE
OpenDevice(
    _In_ BOOL Synchronous
    )
{
    HANDLE DeviceHandle;
    WCHAR DeviceName[MAX_DEVPATH_LENGTH];

    if (!GetDevicePath((LPGUID)&GUID_DEVINTERFACE_OSRUSBFX2,
                       DeviceName,
                       sizeof(DeviceName) / sizeof(DeviceName[0])))
    {
        return INVALID_HANDLE_VALUE;
    }

    if (Synchronous)
    {
        DeviceHandle = CreateFile(DeviceName,
                                  GENERIC_WRITE | GENERIC_READ,
                                  FILE_SHARE_WRITE | FILE_SHARE_READ,
                                  NULL, // default security
                                  OPEN_EXISTING,
                                  FILE_ATTRIBUTE_NORMAL,
                                  NULL);
    }
    else
    {
        DeviceHandle = CreateFile(DeviceName,
                                  GENERIC_WRITE | GENERIC_READ,
                                  FILE_SHARE_WRITE | FILE_SHARE_READ,
                                  NULL, // default security
                                  OPEN_EXISTING,
                                  FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                                  NULL);
    }

    if (DeviceHandle == INVALID_HANDLE_VALUE)
    {
        WriteToErrorLog(L"CreateFile", GetLastError());
    }
    else
    {
        WriteToEventLog(L"Opened Device Successfully", TRACE_LEVEL_INFORMATION);
    }

    return DeviceHandle;
}


/*++

Routine Description:

    Turns off all of the bar graph lights on the OSR USB FX2 device.

Arguments:

    DeviceHandle - The handle to the OSR USB FX2 device

Return Value:

    VOID

--*/
VOID
ClearAllBars(
    _In_ HANDLE DeviceHandle
    )
{
    ULONG BytesReturned;

    BarGraphState.BarsAsUChar = 0;

    if (!DeviceIoControl(DeviceHandle,
                         IOCTL_OSRUSBFX2_SET_BAR_GRAPH_DISPLAY,
                         &BarGraphState,          // Pointer to InBuffer
                         sizeof(BAR_GRAPH_STATE), // Length of InBuffer
                         NULL,                    // Pointer to OutBuffer
                         0,                       // Length of OutBuffer
                         &BytesReturned,          // BytesReturned
                         0))                      // Pointer to Overlapped structure
    {
        WriteToErrorLog(L"DeviceIOControl", GetLastError());
    }
}


/*++

Routine Description:

    Lights the next bar on the OSR USB FX2 device.

Arguments:

    DeviceHandle - The handle to the OSR USB FX2 device

Return Value:

    VOID

--*/
VOID
LightNextBar(
    _In_ HANDLE DeviceHandle
    )
{
    ULONG BytesReturned;

    //
    // Normalize to 0-7
    //
    CurrentBar += 1;

    if (CurrentBar > 7)
    {
        CurrentBar = 0;
    }

    BarGraphState.BarsAsUChar = 1 << (UCHAR)CurrentBar;

    if (!DeviceIoControl(DeviceHandle,
                         IOCTL_OSRUSBFX2_SET_BAR_GRAPH_DISPLAY,
                         &BarGraphState,          // Pointer to InBuffer
                         sizeof(BAR_GRAPH_STATE), // Length of InBuffer
                         NULL,                    // Pointer to OutBuffer
                         0,                       // Length of OutBuffer
                         &BytesReturned,          // BytesReturned
                         0))                      // Pointer to Overlapped structure
    {
        WriteToErrorLog(L"DeviceIOControl", GetLastError());
    }
}