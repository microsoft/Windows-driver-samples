//
// Module:  utils.cpp
//
// Helper functions for Ob sample code tests.
//
// Notice:
//
//    Use this sample code at your own risk; there is no support from Microsoft for the sample code.
//    In addition, this sample code is licensed to you under the terms of the Microsoft Public License
//    (http://www.microsoft.com/opensource/licenses.mspx)
//
//

#include "pch.h"
#include "common.h"

//
// Globals
//

SC_HANDLE TcScmHandle = NULL;
HANDLE TcDeviceHandle = INVALID_HANDLE_VALUE;

WCHAR TcDriverPath[MAX_PATH];


//
// TcUnprotectCallback
//
// Sends unprotect callback ioctl to the driver.
//

BOOL TcUnprotectCallback ()
{
    TD_UNPROTECT_CALLBACK_INPUT UnprotectCallbackInput = {0};

    DWORD BytesReturned = 0;

    LOG_INFO (L"TcUnprotectCallback: entering");

    BOOL Result = DeviceIoControl (
        TcDeviceHandle,
        TD_IOCTL_UNPROTECT_CALLBACK,
        &UnprotectCallbackInput,
        sizeof(UnprotectCallbackInput),
        NULL,
        0,
        &BytesReturned,
        NULL
    );

    if (Result == TRUE)
    {
        LOG_INFO (L"TcUnprotectCallback: succeeded");
    }
    else
    {
        LOG_INFO_FAILURE (L"TcUnprotectCallback: DeviceIoControl failed, last error 0x%x", GetLastError());
    }


    LOG_INFO (L"TcUnprotectCallback: exiting");
    return Result;
}


//
// TcUnprotectCallback
//
// Sends unprotect callback ioctl to the driver.
//

BOOL TcProcessNameCallback (
    _In_reads_(NAME_SIZE+1) PCWSTR  pnametoprotect,
    _In_ ULONG ulOperation
)
{
    TD_PROTECTNAME_INPUT ProtectNameCallbackInput = {0};
    BOOL Result = FALSE;
    DWORD BytesReturned = 0;

    LOG_INFO (L"TcProtectNameCallback: entering - nametoprotect %ls", pnametoprotect);

    // Copy the name of the exececutible to protect into IOCTL structure
    if (!pnametoprotect) {
        LOG_INFO_FAILURE (L"TcProcessNameCallback: NULL Protect Name");
        Result = FALSE;
        goto Exit;
    }
    wcsncpy_s(ProtectNameCallbackInput.Name, pnametoprotect, NAME_SIZE);
    ProtectNameCallbackInput.Operation = ulOperation;


    LOG_INFO (L"TcProtectNameCallback: IOCTL sending nametoprotect %ls", ProtectNameCallbackInput.Name);

    Result = DeviceIoControl (
        TcDeviceHandle,
        TD_IOCTL_PROTECT_NAME_CALLBACK,
        &ProtectNameCallbackInput,
        sizeof(ProtectNameCallbackInput),
        NULL,
        0,
        &BytesReturned,
        NULL
    );

    if (Result == TRUE)
    {
        LOG_INFO (L"TcProcessNameCallback: succeeded");
    }
    else
    {
        LOG_INFO_FAILURE (L"TcProcessNameCallback: DeviceIoControl failed, last error 0x%x", GetLastError());
    }

Exit:

    LOG_INFO (L"TcProcessNameCallback: exiting");
    return Result;
}



//
// TcInitializeGlobals
//

BOOL TcInitializeGlobals()
{
    WCHAR SysDir[MAX_PATH];
    BOOL ReturnValue = FALSE;

#if !defined (_WIN64)

    BOOL Result = FALSE;
    BOOL Wow64Process = FALSE;
    PVOID OldWowRedirectionValue = NULL;

    Result = IsWow64Process (
        GetCurrentProcess(),
        &Wow64Process
    );

    if (Result == FALSE) 
    {
        LOG_INFO_FAILURE (L"IsWow64Process failed, last error 0x%x", GetLastError());
        goto Exit;
    }

    if (Wow64Process == TRUE)
    {
        //
        // Disable FS redirection to make sure a 32 bit test process will
        // copy our (64 bit) driver to system32\drivers rather than syswow64\drivers.
        //

        Result = Wow64DisableWow64FsRedirection (&OldWowRedirectionValue);

        if (Result == FALSE) 
        {
            LOG_INFO_FAILURE (L"Wow64DisableWow64FsRedirection failed, last error 0x%x", GetLastError());
            goto Exit;
        }
    }

#endif

    //
    // Open the service control manager if not already open
    //

    if (TcScmHandle == NULL) {
        TcScmHandle = OpenSCManager (
            NULL,
            NULL,
            SC_MANAGER_ALL_ACCESS
        );
    
        if (TcScmHandle == NULL)
        {
            LOG_INFO_FAILURE (L"OpenSCManager failed, last error 0x%x", GetLastError());
            goto Exit;
        }
    }
    //
    // Construct driver path.
    //

    UINT Size = GetSystemDirectory (SysDir, ARRAYSIZE(SysDir));

    if (Size == 0)
    {
        LOG_INFO_FAILURE (L"GetSystemDirectory failed, last error 0x%x", GetLastError());
        goto Exit;
    }

    HRESULT hr = StringCchPrintf (
        TcDriverPath,
        ARRAYSIZE(TcDriverPath),
        L"%ls\\drivers\\%ls.sys",
        SysDir,
        TD_DRIVER_NAME
    );

    if (FAILED (hr))
    {
        LOG_INFO_FAILURE (L"StringCchPrintf failed, hr 0x%08x", hr);
        goto Exit;
    }

    ReturnValue = TRUE;

Exit:
    return ReturnValue;
}


//
// TcUnInitialize
//

BOOL TcCleanupSCM()
{
    if (TcScmHandle != NULL) {
        CloseServiceHandle(TcScmHandle);
        TcScmHandle = NULL;
    }

    return TRUE;
}

//
// TcLoadDriver
//

BOOL TcLoadDriver()
{
    BOOL ReturnValue = FALSE;

    LOG_INFO(L"TcLoadDriver: Entering");

    //
    // First, uninstall and unload the driver. 
    //

    ReturnValue = TcUnloadDriver();

    if (ReturnValue != TRUE)
    {
        LOG_INFO_FAILURE (L"TcUnloadDriver failed");
        goto Exit;
    }

    //
    // Copy the driver to system32\drivers
    //

    ReturnValue = CopyFile (TD_DRIVER_NAME_WITH_EXT, TcDriverPath, FALSE);

    if (ReturnValue == FALSE)
    {
        LOG_INFO_FAILURE (
            L"CopyFile(%ls, %ls) failed, last error 0x%x",
            TD_DRIVER_NAME_WITH_EXT, TcDriverPath, GetLastError()
        );

        goto Exit;
    }

    //
    // Install the driver.
    //

    ReturnValue = TcCreateService();

    if (ReturnValue == FALSE)
    {
        LOG_INFO_FAILURE (L"TcCreateService failed");
        goto Exit;
    }

    //
    // Load the driver.
    //

    ReturnValue = TcStartService();

    if (ReturnValue == FALSE)
    {
        LOG_INFO_FAILURE (L"TcStartService failed");
        goto Exit;
    }


    ReturnValue = TRUE;

Exit:

    LOG_INFO(L"TcLoadDriver: Exiting");
    return ReturnValue;
}



//
// TcUnloadDriver
//

BOOL TcUnloadDriver()
{
    BOOL ReturnValue = FALSE;

    LOG_INFO(L"TcUnloadDriver: Entering");


    //
    // Unload the driver.
    //

    ReturnValue = TcStopService();

    if (ReturnValue == FALSE)
    {
        LOG_INFO_FAILURE (L"TcStopService failed");
        goto Exit;
    }

    //
    // Delete the service.
    //

    ReturnValue = TcDeleteService();

    if (ReturnValue == FALSE)
    {
        LOG_INFO_FAILURE (L"TcDeleteService failed");
        goto Exit;
    }

    ReturnValue = TRUE;

Exit:

    LOG_INFO(L"TcUnloadDriver: Exiting");

    return ReturnValue;
}

//
// TcGetServiceState
//

BOOL TcGetServiceState (
    _In_ SC_HANDLE ServiceHandle,
    _Out_ DWORD* State
)
{
    SERVICE_STATUS_PROCESS ServiceStatus;
    DWORD BytesNeeded;

    *State = 0;
	
    BOOL Result = QueryServiceStatusEx ( 
        ServiceHandle,
        SC_STATUS_PROCESS_INFO,
        (LPBYTE)&ServiceStatus,
        sizeof(ServiceStatus),
        &BytesNeeded
        );

    if (Result == FALSE)
    {
        LOG_INFO_FAILURE (L"TcGetServiceState: QueryServiceStatusEx failed, last error 0x%x", GetLastError());
        return FALSE;
    }

    *State = ServiceStatus.dwCurrentState;

    return TRUE;
}

//
// Wait for service to enter specified state.
//

BOOL TcWaitForServiceState (
    _In_ SC_HANDLE ServiceHandle,
    _In_ DWORD State
)
{
    for (;;)
    {
        LOG_INFO (L"TcWaitForServiceState: Waiting for service %p to enter state %u...", (DWORD_PTR)ServiceHandle, State);

        DWORD ServiceState;
        BOOL Result = TcGetServiceState (ServiceHandle, &ServiceState);

        if (Result == FALSE)
        {
            return FALSE;
        }

        if (ServiceState == State)
        {
            break;
        }

        Sleep (1000);
    }

    return TRUE;
}

//
// TcCreateService
//

BOOL TcCreateService()
{
    BOOL ReturnValue = FALSE;

    LOG_INFO(L"TcCreateService: Entering");

    //
    // Create the service
    //

    SC_HANDLE ServiceHandle = CreateService (
        TcScmHandle,            // handle to SC manager
        TD_DRIVER_NAME,         // name of service
        TD_DRIVER_NAME,         // display name
        SERVICE_ALL_ACCESS,     // access mask
        SERVICE_KERNEL_DRIVER,  // service type
        SERVICE_DEMAND_START,   // start type
        SERVICE_ERROR_NORMAL,   // error control
        TcDriverPath,           // full path to driver
        NULL,                   // load ordering
        NULL,                   // tag id
        NULL,                   // dependency
        NULL,                   // account name
        NULL                    // password
    );

    DWORD LastError = GetLastError();

    if (ServiceHandle == NULL && LastError != ERROR_SERVICE_EXISTS)
    {
        LOG_INFO_FAILURE (L"CreateService failed, last error 0x%x", LastError);
        goto Exit;
    }

    ReturnValue = TRUE;

Exit:

    if (ServiceHandle)
    {
        CloseServiceHandle (ServiceHandle);
    }

    LOG_INFO(L"TcCreateService: Exiting");

    return ReturnValue;
}

//
// TcStartService
//

BOOL TcStartService()
{
    BOOL ReturnValue = FALSE;

    //
    // Open the service. The function assumes that
    // TdCreateService has been called before this one
    // and the service is already installed.
    //

    SC_HANDLE ServiceHandle = OpenService (
        TcScmHandle,
        TD_DRIVER_NAME,
        SERVICE_ALL_ACCESS
    );

    if (ServiceHandle == NULL)
    {
        LOG_INFO_FAILURE (L"TcStartService: OpenService failed, last error 0x%x", GetLastError());
        goto Exit;
    }

    //
    // Start the service
    //

    if (! StartService (ServiceHandle, 0, NULL))
    {
        if (GetLastError() != ERROR_SERVICE_ALREADY_RUNNING)
        {
            LOG_INFO_FAILURE (L"TcStartService: StartService failed, last error 0x%x", GetLastError());
            goto Exit;
        }
    }

    if (FALSE == TcWaitForServiceState (ServiceHandle, SERVICE_RUNNING))
    {
        goto Exit;
    }
    
    ReturnValue = TRUE;

Exit:

    if (ServiceHandle)
    {
        CloseServiceHandle (ServiceHandle);
    }

    return ReturnValue;
}


//
// TcStopService
//

BOOL TcStopService()
{
    BOOL ReturnValue = FALSE;

    LOG_INFO(L"TcStopService: Entering");

    //
    // Open the service so we can stop it
    //

    SC_HANDLE ServiceHandle = OpenService (
        TcScmHandle,
        TD_DRIVER_NAME,
        SERVICE_ALL_ACCESS
    );

    DWORD LastError = GetLastError();

    if (ServiceHandle == NULL)
    {
        if (LastError == ERROR_SERVICE_DOES_NOT_EXIST)
        {
            ReturnValue = TRUE;
        }
        else
        {
            LOG_INFO_FAILURE (L"TcStopService: OpenService failed, last error 0x%x", LastError);
        }

        goto Exit;
    }

    //
    // Stop the service
    //

    SERVICE_STATUS ServiceStatus;

    if (FALSE == ControlService (ServiceHandle, SERVICE_CONTROL_STOP, &ServiceStatus))
    {
        LastError = GetLastError();

        if (LastError != ERROR_SERVICE_NOT_ACTIVE)
        {
            LOG_INFO_FAILURE (L"TcStopService: ControlService failed, last error 0x%x", LastError);
            goto Exit;
        }
    }

    if (FALSE == TcWaitForServiceState (ServiceHandle, SERVICE_STOPPED))
    {
        goto Exit;
    }
    
    ReturnValue = TRUE;

Exit:

    if (ServiceHandle)
    {
        CloseServiceHandle (ServiceHandle);
    }

    LOG_INFO(L"TcStopService: Exiting");

    return ReturnValue;
}

//
// TcDeleteService
//

BOOL TcDeleteService()
{
    BOOL ReturnValue = FALSE;


    LOG_INFO(L"TcDeleteService: Entering");

    //
    // Open the service so we can delete it
    //

    SC_HANDLE ServiceHandle = OpenService (
        TcScmHandle,
        TD_DRIVER_NAME,
        SERVICE_ALL_ACCESS
    );

    DWORD LastError = GetLastError();

    if (ServiceHandle == NULL)
    {
        if (LastError == ERROR_SERVICE_DOES_NOT_EXIST)
        {
            ReturnValue = TRUE;
        }
        else
        {
            LOG_INFO_FAILURE (L"TcDeleteService: OpenService failed, last error 0x%x", LastError);
        }

        goto Exit;
    }

    //
    // Delete the service
    //

    if (! DeleteService (ServiceHandle))
    {
        LastError = GetLastError();

        if (LastError != ERROR_SERVICE_MARKED_FOR_DELETE)
        {
            LOG_INFO_FAILURE (L"TcDeleteService: DeleteService failed, last error 0x%x", LastError);
            goto Exit;
        }
    }

    ReturnValue = TRUE;

Exit:

    if (ServiceHandle)
    {
        CloseServiceHandle (ServiceHandle);
    }

    LOG_INFO(L"TcDeleteService: Exiting");

    return ReturnValue;
}

//
// TcOpenDevice
//

BOOL TcOpenDevice()
{
    BOOL ReturnValue = FALSE;

    LOG_INFO(L"TcOpenDevice: Entering");


    //
    // Open the device if not already opened
    //
    if (TcDeviceHandle == INVALID_HANDLE_VALUE) {
        TcDeviceHandle = CreateFile (
            TD_WIN32_DEVICE_NAME,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );

        if (TcDeviceHandle == INVALID_HANDLE_VALUE)
        {
            LOG_INFO_FAILURE (L"TcOpenDevice: CreateFile(%ls) failed, last error 0x%x", TD_WIN32_DEVICE_NAME, GetLastError());
            goto Exit;
        }
    }


    ReturnValue = TRUE;

Exit:

    LOG_INFO(L"TcOpenDevice: Exiting");
    return ReturnValue;
}

//
// TcOpenDevice
//

BOOL TcCloseDevice()
{
    BOOL ReturnValue = FALSE;

    LOG_INFO(L"TcCloseDevice: Entering");

    //
    // Close our handle to the device.
    //

    if (TcDeviceHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle (TcDeviceHandle);
        TcDeviceHandle = INVALID_HANDLE_VALUE;
    }

    ReturnValue = TRUE;

    LOG_INFO(L"TcCloseDevice: Exiting");
    return ReturnValue;
}

