/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    osrfx2_DCHU_componentsoftware.cpp

Abstract:

    An example .exe to run using the AddSoftware directive within an INF.
    Contains an example on how to obtain a handle to the primary device using
    SoftwareArguments within an AddSoftware directive's section.

Environment:

    User mode

--*/

#include "stdafx.h"

#define BUFFER_SIZE (128 * sizeof(WCHAR))
#define DEVICE_REG_SUBKEY L"OSR"
#define DEVICE_VALUE_OP_MODE L"OperatingMode"
#define DEVICE_VALUE_OP_PARAMS L"OperatingParams"
#define DEVICE_VALUE_OP_EXCEPTIONS L"OperatingExceptions"
#define CONTOSO_FILEPATH L"C:\\Program Files\\Contoso"
#define OSRFX2_DCHU_FILEPATH L"C:\\Program Files\\Contoso\\OsrFx2_DCHU"
#define FILEPATH L"C:\\Program Files\\Contoso\\OsrFx2_DCHU\\AddSoftwareInstallationFile.txt"
#define FILE_CONTENT L"Operating Mode: %ws\r\nOperating Params: %ws\r\nOperating Exceptions: %ws"


/*++

Routine Description:

Arguments:

Return Value:


--*/
DWORD
WriteToFile(
    _In_ PWSTR MessageText,
    _In_ DWORD MessageTextLength
    )
{
    DWORD Err = ERROR_SUCCESS;
    BOOL ErrorFlag = FALSE;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD BytesToWrite;
    DWORD BytesWritten;

    if (!CreateDirectory(CONTOSO_FILEPATH, NULL))
    {
        Err = GetLastError();

        if (Err == ERROR_ALREADY_EXISTS)
        {
            Err = ERROR_SUCCESS;
        }
        else
        {
            goto cleanup;
        }
    }

    if (!CreateDirectory(OSRFX2_DCHU_FILEPATH, NULL))
    {
        Err = GetLastError();

        if (Err == ERROR_ALREADY_EXISTS)
        {
            Err = ERROR_SUCCESS;
        }
        else
        {
            goto cleanup;
        }
    }

    hFile = CreateFile(FILEPATH,
                       GENERIC_WRITE,
                       0,
                       NULL,
                       OPEN_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        Err = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    BytesToWrite = MessageTextLength * sizeof(WCHAR);

    ErrorFlag = WriteFile(hFile,
                          MessageText,
                          BytesToWrite,
                          &BytesWritten,
                          NULL);

    if ((!ErrorFlag) ||
        (BytesToWrite != BytesWritten))
    {
        Err = GetLastError();
        goto cleanup;
    }

cleanup:

    if (hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
    }

    return Err;
}


/*++

Routine Description:

    Obtains the handle to the device that used a separate Software Component
    INF to install software.

Arguments:

    Argc - The number of command line arguments

    Argv - The array of command line arguments

Return Value:

    VOID

--*/
VOID
__cdecl
_tmain(
    INT Argc,
    TCHAR *Argv[])
{
    DWORD Err = ERROR_SUCCESS;
    HRESULT hr = S_OK;
    BOOL ErrorFlag = FALSE;
    DWORD BytesToWrite = 0;
    DWORD BytesWritten = 0;
    PTSTR DeviceInstanceId;
    HDEVINFO DevInfoList = INVALID_HANDLE_VALUE;
    SP_DEVINFO_DATA DevInfoData;
    WCHAR ParentDeviceInstanceId[MAX_DEVICE_ID_LEN];
    DWORD RequiredSize;
    DEVPROPTYPE DevPropType;
    SP_DEVINFO_DATA ParentDevInfoData;
    HKEY DevRegKey = NULL;
    PWSTR OperatingMode = NULL;
    PWSTR OperatingParams = NULL;
    PWSTR OperatingExceptions = NULL;
    DWORD ValueType;
    DWORD ValueSize = BUFFER_SIZE;
    HKEY DevOsrRegKey = NULL;
    PWSTR FileText = NULL;
    DWORD FileTextLength;

    //
    // Validate arguments.
    //
    if (Argc != 2)
    {
        Err = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    OperatingMode = (PWSTR)malloc(BUFFER_SIZE * sizeof(WCHAR));

    if (OperatingMode == NULL)
    {
        Err = ERROR_OUTOFMEMORY;
        goto cleanup;
    }

    OperatingParams = (PWSTR)malloc(BUFFER_SIZE * sizeof(WCHAR));

    if (OperatingParams == NULL)
    {
        Err = ERROR_OUTOFMEMORY;
        goto cleanup;
    }

    OperatingExceptions = (PWSTR)malloc(BUFFER_SIZE * sizeof(WCHAR));

    if (OperatingExceptions == NULL)
    {
        Err = ERROR_OUTOFMEMORY;
        goto cleanup;
    }

    //
    // Argv[1] should be the device instance ID.
    //
    DeviceInstanceId = Argv[1];

    //
    // Create a device info list to store queried device handles.
    //
    DevInfoList = SetupDiCreateDeviceInfoList(NULL, NULL);

    if (DevInfoList == INVALID_HANDLE_VALUE)
    {
        Err = ERROR_OUTOFMEMORY;
        goto cleanup;
    }

    DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    //
    // Add the Software Component device that called AddSoftware to the
    // device info list, which will be used to find the primary device that
    // called AddComponent in the first place.
    //
    if (!SetupDiOpenDeviceInfo(DevInfoList,
                               DeviceInstanceId,
                               NULL,
                               0,
                               &DevInfoData))
    {
        Err = GetLastError();
        goto cleanup;
    }

    //
    // Get the device instance id of the opened device's parent.
    //
    if (!SetupDiGetDeviceProperty(DevInfoList,
                                  &DevInfoData,
                                  &DEVPKEY_Device_Parent,
                                  &DevPropType,
                                  (PBYTE)ParentDeviceInstanceId,
                                  sizeof(ParentDeviceInstanceId),
                                  &RequiredSize,
                                  0))
    {
        Err = GetLastError();
        goto cleanup;
    }

    if ((DevPropType != DEVPROP_TYPE_STRING) ||
        (RequiredSize < sizeof(WCHAR)))
    {
        Err = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    ParentDevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    //
    // Get the parent of the device we retrieved first, which is the device
    // that called AddComponent in its INF.
    //
    if (!SetupDiOpenDeviceInfoW(DevInfoList,
                                ParentDeviceInstanceId,
                                NULL,
                                0,
                                &ParentDevInfoData))
    {
        Err = GetLastError();
        goto cleanup;
    }

    //
    // Open up the HW registry keys of the primary device.
    //
    DevRegKey = SetupDiOpenDevRegKey(DevInfoList,
                                     &ParentDevInfoData,
                                     DICS_FLAG_GLOBAL,
                                     0,
                                     DIREG_DEV,
                                     KEY_READ);

    if (DevRegKey == INVALID_HANDLE_VALUE)
    {
        Err = GetLastError();
        DevRegKey = NULL;
        goto cleanup;
    }

    Err = RegOpenKeyEx(DevRegKey,
                       DEVICE_REG_SUBKEY,
                       0,
                       KEY_QUERY_VALUE,
                       &DevOsrRegKey);

    if (Err != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    //
    // Retrieve the registry values defined in the primary device's INF.
    // Note that if the extension INF was applied, the extension INF would
    // overwrite OperatingParams and create OperatingExceptions.
    //
    Err = RegQueryValueEx(DevOsrRegKey,
                          DEVICE_VALUE_OP_MODE,
                          NULL,
                          &ValueType,
                          (LPBYTE)OperatingMode,
                          &ValueSize);

    while (Err == ERROR_MORE_DATA)
    {
        //
        // Create a larger buffer.
        //
        OperatingMode = (PTSTR)realloc(OperatingMode, ValueSize);

        Err = RegQueryValueEx(DevOsrRegKey,
                              DEVICE_VALUE_OP_MODE,
                              NULL,
                              &ValueType,
                              (LPBYTE)OperatingMode,
                              &ValueSize);
    }

    if (Err != ERROR_SUCCESS)
    {
        OperatingMode[0] = TEXT('\0');
    }
    else if ((ValueType != REG_SZ) ||
             (ValueSize < sizeof(TCHAR)))
    {
        Err = ERROR_REGISTRY_CORRUPT;
        OperatingMode[0] = TEXT('\0');
    }

    ValueSize = BUFFER_SIZE;

    Err = RegQueryValueEx(DevOsrRegKey,
                          DEVICE_VALUE_OP_PARAMS,
                          NULL,
                          &ValueType,
                          (LPBYTE)OperatingParams,
                          &ValueSize);

    while (Err == ERROR_MORE_DATA)
    {
        //
        // Create a larger buffer.
        //
        OperatingParams = (PTSTR)realloc(OperatingParams, ValueSize);

        Err = RegQueryValueEx(DevOsrRegKey,
                              DEVICE_VALUE_OP_MODE,
                              NULL,
                              &ValueType,
                              (LPBYTE)OperatingParams,
                              &ValueSize);
    }

    if (Err != ERROR_SUCCESS)
    {
        OperatingParams[0] = TEXT('\0');
    }
    else if ((ValueType != REG_SZ) ||
             (ValueSize < sizeof(TCHAR)))
    {
        Err = ERROR_REGISTRY_CORRUPT;
        OperatingParams[0] = TEXT('\0');
    }

    ValueSize = BUFFER_SIZE;

    Err = RegQueryValueEx(DevOsrRegKey,
                          DEVICE_VALUE_OP_EXCEPTIONS,
                          NULL,
                          &ValueType,
                          (LPBYTE)OperatingExceptions,
                          &ValueSize);

    while (Err == ERROR_MORE_DATA)
    {
        //
        // Create a larger buffer.
        //
        OperatingExceptions = (PTSTR)realloc(OperatingExceptions, ValueSize);

        Err = RegQueryValueEx(DevOsrRegKey,
                              DEVICE_VALUE_OP_EXCEPTIONS,
                              NULL,
                              &ValueType,
                              (LPBYTE)OperatingExceptions,
                              &ValueSize);
    }

    if (Err != ERROR_SUCCESS)
    {
        OperatingExceptions[0] = TEXT('\0');
    }
    else if ((ValueType != REG_SZ) ||
             (ValueSize < sizeof(TCHAR)))
    {
        Err = ERROR_REGISTRY_CORRUPT;
        OperatingExceptions[0] = TEXT('\0');
    }

    FileTextLength = (DWORD)wcslen(OperatingMode) +
                     (DWORD)wcslen(OperatingParams) +
                     (DWORD)wcslen(OperatingExceptions) +
                     (DWORD)wcslen(FILE_CONTENT) -
                     9; // Subtract the 3 %ws

    FileText = (PWSTR)malloc((FileTextLength + 1) * sizeof(WCHAR));

    if (FileText == NULL)
    {
        Err = ERROR_OUTOFMEMORY;
        goto cleanup;
    }

    hr = StringCchPrintf(FileText,
                         FileTextLength + 1,
                         FILE_CONTENT,
                         OperatingMode,
                         OperatingParams,
                         OperatingExceptions);

    if (FAILED(hr))
    {
        Err = HRESULT_CODE(hr);
        goto cleanup;
    }

    Err = WriteToFile(FileText, FileTextLength);

    if (Err != ERROR_SUCCESS)
    {
        goto cleanup;
    }

cleanup:

    if (FileText != NULL)
    {
        free(FileText);
    }

    if (OperatingExceptions != NULL)
    {
        free(OperatingExceptions);
    }

    if (OperatingParams != NULL)
    {
        free(OperatingParams);
    }

    if (OperatingMode != NULL)
    {
        free(OperatingMode);
    }

    if (DevOsrRegKey != NULL)
    {
        RegCloseKey(DevOsrRegKey);
    }

    if (DevRegKey != NULL)
    {
        RegCloseKey(DevRegKey);
    }

    if (DevInfoList != INVALID_HANDLE_VALUE)
    {
        SetupDiDestroyDeviceInfoList(DevInfoList);
    }

    return;
}

