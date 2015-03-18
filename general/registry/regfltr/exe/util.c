/*++
Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    util.c

Abstract: 

    Utility routines to dynamically load and unload a Windows NT kernel-mode
    driver using the Service Control Manager APIs.

Environment:

    User mode only

--*/



#include "regctrl.h"


BOOL 
UtilCreateService(
    _In_ SC_HANDLE hSCM, 
    _In_ LPTSTR szDriverName, 
    _In_ LPTSTR szDriverPath
);

BOOL 
UtilStartService(
    _In_ SC_HANDLE hSCM, 
    _In_ LPTSTR szDriverName
);

BOOL 
UtilStopService(
    _In_ SC_HANDLE hSCM, 
    _In_ LPTSTR szDriverName
);

BOOL 
UtilDeleteService(
    _In_ SC_HANDLE hSCM, 
    _In_ LPTSTR szDriverName
);

BOOL 
UtilOpenDevice(
    _In_ LPTSTR szWin32DeviceName, 
    _Out_ HANDLE * phDevice);

BOOL 
UtilGetServiceState(
    _In_ SC_HANDLE hService, 
    _Out_ DWORD * State);

BOOL 
UtilWaitForServiceState(
    _In_ SC_HANDLE hService, 
    _In_ DWORD State);


BOOL 
UtilLoadDriver(
    _In_ LPTSTR szDriverName,
    _In_ LPTSTR szDriverFileName,
    _In_ LPTSTR szWin32DeviceName,
    _Out_ HANDLE *pDriver
    )
/*++

Routine Description:

    This routine uses the Service Control Manager APIs to create an entry 
    for a standalone driver. It then opens a handle to the driver. 
    The driver is assumed to be in the current directory.

    NOTE: This routine creates an entry for a standalone driver. If this
    is modified for use with a driver that requires a Tag, Group, and/or 
    Dependencies, it may be necessary to query the registry for existing 
    driver information (in order to determine a unique Tag, etc.).
    
Arguments:

    szDriverName - name of the driver (no extension)

    szDriverFileName - name of driver (with extension)

    szWin32DeviceName - Name of driver (no extension) prefixed with \\.\

    pDriver - pointer to a variable that receives the handle to the driver

Return Value:

    TRUE if driver is loaded successfully.

--*/
{
    BOOL ReturnValue = FALSE;
    TCHAR* pPathSeparator;
    TCHAR szDriverPath[MAX_PATH] = _T("");
    DWORD dwSize;
    SC_HANDLE hSCM = NULL;
    HANDLE hDriver = NULL;

    *pDriver = NULL;

    //
    // Construct driver path.
    //

    dwSize = GetModuleFileName(NULL, szDriverPath, ARRAY_LENGTH(szDriverPath));
    
    if (dwSize == 0) {
        ErrorPrint("GetModuleFileName failed, last error 0x%u", GetLastError());
        goto Exit;
    }

    pPathSeparator = _tcsrchr(szDriverPath, _T('\\'));

    if (pPathSeparator != NULL) {
        pPathSeparator[1] = _T('\0');
        _tcscat_s(szDriverPath, MAX_PATH, szDriverFileName);
    } else {
        ErrorPrint("_tcsrchr failed to file \\ in driver path.");
        goto Exit;
    }

    //
    // Get a handle to SCM
    //

    hSCM = OpenSCManager ( NULL, NULL, SC_MANAGER_ALL_ACCESS );

    if (hSCM == NULL) {
        ErrorPrint("OpenSCManager failed, last error 0x%x", GetLastError());
        goto Exit;
    }

    //
    // First, uninstall and unload the driver. 
    //

    ReturnValue = UtilUnloadDriver( INVALID_HANDLE_VALUE, hSCM, szDriverName);

    if (ReturnValue != TRUE) {
        ErrorPrint("UnloadDriver failed");
        goto Exit;
    }

    //
    // Install the driver.
    //

    ReturnValue = UtilCreateService(hSCM, szDriverName, szDriverPath);

    if (ReturnValue == FALSE) {
        ErrorPrint("UtilCreateService failed");
        goto Exit;
    }

    //
    // Load the driver.
    //

    ReturnValue = UtilStartService(hSCM, szDriverName);

    if (ReturnValue == FALSE) {
        ErrorPrint("UtilStartService failed");
        goto Exit;
    }

    //
    // Open a handle to the device.
    //

    ReturnValue = UtilOpenDevice(szWin32DeviceName, &hDriver);

    if (ReturnValue == FALSE) {
        ErrorPrint("UtilOpenDevice failed");
        goto Exit;
    }

    *pDriver = hDriver;
    ReturnValue = TRUE;

Exit:

    if (hSCM != NULL) {
        CloseServiceHandle(hSCM);
    }
    
    return ReturnValue;
}


BOOL 
UtilUnloadDriver(
    _In_ HANDLE hDriver,
    _In_opt_ SC_HANDLE hPassedSCM,
    _In_ LPTSTR szDriverName
    )
/*++

Routine Description:

    Unloads the driver using SCManager.

Arguments:

    hDriver - handle to the driver

    hPassedSCM - handle to the SCManager (optional)

    szDriverName - name of driver (no extension)
    
Return Value:

    TRUE if driver is successfully unloaded

--*/
{

    BOOL ReturnValue = FALSE;
    SC_HANDLE hSCM = hPassedSCM;

    //
    // Get a handle to SCM if not passed in
    //

    if (hSCM == NULL) {

        hSCM = OpenSCManager ( NULL, NULL, SC_MANAGER_ALL_ACCESS );

        if (hSCM == NULL) {
            ErrorPrint("OpenSCManager failed, last error 0x%x", GetLastError());
            goto Exit;
        }
    }


    //
    // Close our handle to the device.
    //

    if ((hDriver != NULL) && (hDriver != INVALID_HANDLE_VALUE)) {
        CloseHandle (hDriver);
        hDriver = INVALID_HANDLE_VALUE;
    }

    //
    // Unload the driver.
    //

    ReturnValue = UtilStopService(hSCM, szDriverName);

    if (ReturnValue == FALSE) {
        ErrorPrint("UtilStopService failed");
        goto Exit;
    }

    //
    // Delete the service.
    //

    ReturnValue = UtilDeleteService(hSCM, szDriverName);

    if (ReturnValue == FALSE) {
        ErrorPrint("UtilDeleteService failed");
        goto Exit;
    }

    ReturnValue = TRUE;

Exit:

    if ((hPassedSCM == NULL) && (hSCM != NULL)) {
        CloseServiceHandle(hSCM);
    }
    
    return ReturnValue;
}



BOOL 
UtilGetServiceState (
    _In_ SC_HANDLE hService,
    _Out_ DWORD* State
    )
/*++

Routine Description:

    Gets the state of the service using QueryServiceStatusEx

Arguments:

    hService - handle to the service to query

    State - pointer to a variable that receives the state

Return Value:

    TRUE if service is queried successfully.

--*/
{
    SERVICE_STATUS_PROCESS ServiceStatus;
    DWORD BytesNeeded;
    BOOL Result;
    
    *State = 0;

    Result = QueryServiceStatusEx ( hService,
                                         SC_STATUS_PROCESS_INFO,
                                         (LPBYTE)&ServiceStatus,
                                         sizeof(ServiceStatus),
                                         &BytesNeeded);

    if (Result == FALSE) {
        ErrorPrint("QueryServiceStatusEx failed, last error 0x%x", GetLastError());
        return FALSE;
    }

    *State = ServiceStatus.dwCurrentState;

    return TRUE;
}


BOOL 
UtilWaitForServiceState (
    _In_ SC_HANDLE hService,
    _In_ DWORD State
    )
/*++

Routine Description:

    This routine waits for the service to reach a certain state

Arguments:

    hService - handle to the service

    State - the desired state

Return Value:

    TRUE if service reaches the desired state or FALSE if querying the
    service returns an error.

--*/
{

    DWORD ServiceState;
    BOOL Result; 
    
    for (;;) {

        Result = UtilGetServiceState (hService, &ServiceState);

        if (Result == FALSE) {
            return FALSE;
        }

        if (ServiceState == State) {
            break;
        }

        Sleep (1000);
    }

    return TRUE;
}

//
// UtilCreateService
//

BOOL 
UtilCreateService(
    _In_ SC_HANDLE hSCM,
    _In_ LPTSTR szDriverName,
    _In_ LPTSTR szDriverPath
    )
/*++

Routine Description:

    Uses SCManager to create a service

Arguments:

    hSCM - handle to the SCManager

    szDriverName - name of driver (no extension) which will serve as the 
        created service's name

    szDriverPath - path to driver

Return Value:

    TRUE if service is created successfully, FALSE otherwise.

--*/
{
    BOOL ReturnValue = FALSE;

    //
    // Create the service
    //

    SC_HANDLE hService = CreateService (
        hSCM,                 // handle to SC manager
        szDriverName,         // name of service
        szDriverName,         // display name
        SERVICE_ALL_ACCESS,     // access mask
        SERVICE_KERNEL_DRIVER,  // service type
        SERVICE_DEMAND_START,   // start type
        SERVICE_ERROR_NORMAL,   // error control
        szDriverPath,           // full path to driver
        NULL,                   // load ordering
        NULL,                   // tag id
        NULL,                   // dependency
        NULL,                   // account name
        NULL                    // password
    );

    if ((hService == NULL) && (GetLastError() != ERROR_SERVICE_EXISTS)) {
        ErrorPrint("CreateService failed, last error 0x%x", GetLastError());
        goto Exit;
    }

    ReturnValue = TRUE;

Exit:

    if (hService) {
        CloseServiceHandle(hService);
    }

    return ReturnValue;
}


BOOL 
UtilStartService(
    _In_ SC_HANDLE hSCM,
    _In_ LPTSTR szDriverName
    )
/*++

Routine Description:

    Starts a service

Arguments:

    hSCM - handle to the SCManager

    szDriverName - name of driver (without extension), services as name of
        the service to start

Return Value:

    TRUE if service is successfully started, FALSE otherwise.

--*/
{
    BOOL ReturnValue = FALSE;

    //
    // Open the service. The function assumes that
    // UtilCreateService has been called before this one
    // and the service is already installed.
    //

    SC_HANDLE hService = OpenService ( hSCM, szDriverName, SERVICE_ALL_ACCESS );

    if (hService == NULL) {
        ErrorPrint("OpenService failed, last error 0x%x", GetLastError());
        goto Exit;
    }

    //
    // Start the service
    //

    if (! StartService (hService, 0, NULL)) {

        if (GetLastError() != ERROR_SERVICE_ALREADY_RUNNING) {
            ErrorPrint("StartService failed, last error 0x%x", GetLastError());
            goto Exit;
        }
    }

    if (FALSE == UtilWaitForServiceState (hService, SERVICE_RUNNING)) {
        goto Exit;
    }
    
    ReturnValue = TRUE;

Exit:

    if (hService) {
        CloseServiceHandle(hService);
    }

    return ReturnValue;
}


BOOL 
UtilStopService(
    _In_ SC_HANDLE hSCM,
    _In_ LPTSTR szDriverName
    )
/*++

Routine Description:

    Stops a service

Arguments:

    hSCM - handle to the SCManager

    szDriverName - name of driver (without extension), services as name of
        the service

Return Value:

    TRUE if service is successfully stopped, FALSE otherwise.

--*/
{
    BOOL ReturnValue = FALSE;
    SERVICE_STATUS ServiceStatus;
    
    //
    // Open the service so we can stop it
    //

    SC_HANDLE hService = OpenService ( hSCM, szDriverName, SERVICE_ALL_ACCESS );

    if (hService == NULL) {
        if (GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST) {
            ReturnValue = TRUE;
        } else {
            ErrorPrint("OpenService failed, last error 0x%x", GetLastError());
        }
        goto Exit;
    }

    //
    // Stop the service
    //

    if (FALSE == ControlService (hService, SERVICE_CONTROL_STOP, &ServiceStatus)) {
        if (GetLastError() != ERROR_SERVICE_NOT_ACTIVE) {
            ErrorPrint("ControlService failed, last error 0x%x", GetLastError());
            goto Exit;
        }
    }

    if (FALSE == UtilWaitForServiceState (hService, SERVICE_STOPPED)) {
        goto Exit;
    }
    
    ReturnValue = TRUE;

Exit:

    if (hService) {
        CloseServiceHandle (hService);
    }

    return ReturnValue;
}


BOOL 
UtilDeleteService(
    _In_ SC_HANDLE hSCM,
    _In_ LPTSTR szDriverName
    )
/*++

Routine Description:

    Deletes a service

Arguments:

    hSCM - handle to the SCManager

    szDriverName - name of driver (without extension), services as name of
        the service

Return Value:

    TRUE if service is successfully deleted, FALSE otherwise.

--*/
{
    BOOL ReturnValue = FALSE;

    //
    // Open the service so we can delete it
    //

    SC_HANDLE hService = OpenService ( hSCM, szDriverName, SERVICE_ALL_ACCESS );

    if (hService == NULL) {
        if (GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST) {
            ReturnValue = TRUE;
        } else {
            ErrorPrint("OpenService failed, last error 0x%x", GetLastError());
        }
       goto Exit;
    }

    //
    // Delete the service
    //

    if (! DeleteService (hService)) {
        if (GetLastError() != ERROR_SERVICE_MARKED_FOR_DELETE) {
            ErrorPrint("DeleteService failed, last error 0x%x", GetLastError());
            goto Exit;
        }
    }

    ReturnValue = TRUE;

Exit:

    if (hService) {
        CloseServiceHandle (hService);
    }

    return ReturnValue;
}


BOOL 

UtilOpenDevice(
    _In_ LPTSTR szWin32DeviceName,
    _Out_ HANDLE *phDevice
    )
/*++

Routine Description:

    Opens a device

Arguments:

    szWin32DeviceName - name of the device

    phDevice - pointer to a variable that receives the handle to the device

Return Value:

    TRUE if the device is successfully opened, FALSE otherwise.

--*/
{
    BOOL ReturnValue = FALSE;
    HANDLE hDevice;

    //
    // Open the device
    //

    hDevice = CreateFile ( szWin32DeviceName,
                           GENERIC_READ | GENERIC_WRITE,
                           0,
                           NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);

    if (hDevice == INVALID_HANDLE_VALUE) {
        ErrorPrint("CreateFile(%ls) failed, last error 0x%x", 
                    szWin32DeviceName, 
                    GetLastError() );
        goto Exit;
    }

    ReturnValue = TRUE;

Exit:

    *phDevice = hDevice;
    return ReturnValue;
}
