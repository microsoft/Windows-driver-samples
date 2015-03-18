/*++
Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    install.c

Abstract:

    Win32 routines to dynamically load and unload a Windows NT kernel-mode
    driver using the Service Control Manager APIs.

Environment:

    User mode only

--*/


#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strsafe.h>
#include "sioctl.h"

BOOLEAN
InstallDriver(
    _In_ SC_HANDLE  SchSCManager,
    _In_ LPCTSTR    DriverName,
    _In_ LPCTSTR    ServiceExe
    );


BOOLEAN
RemoveDriver(
    _In_ SC_HANDLE  SchSCManager,
    _In_ LPCTSTR    DriverName
    );

BOOLEAN
StartDriver(
    _In_ SC_HANDLE  SchSCManager,
    _In_ LPCTSTR    DriverName
    );

BOOLEAN
StopDriver(
    _In_ SC_HANDLE  SchSCManager,
    _In_ LPCTSTR    DriverName
    );

BOOLEAN
InstallDriver(
    _In_ SC_HANDLE  SchSCManager,
    _In_ LPCTSTR    DriverName,
    _In_ LPCTSTR    ServiceExe
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    SC_HANDLE   schService;
    DWORD       err;

    //
    // NOTE: This creates an entry for a standalone driver. If this
    //       is modified for use with a driver that requires a Tag,
    //       Group, and/or Dependencies, it may be necessary to
    //       query the registry for existing driver information
    //       (in order to determine a unique Tag, etc.).
    //

    //
    // Create a new a service object.
    //

    schService = CreateService(SchSCManager,           // handle of service control manager database
                               DriverName,             // address of name of service to start
                               DriverName,             // address of display name
                               SERVICE_ALL_ACCESS,     // type of access to service
                               SERVICE_KERNEL_DRIVER,  // type of service
                               SERVICE_DEMAND_START,   // when to start service
                               SERVICE_ERROR_NORMAL,   // severity if service fails to start
                               ServiceExe,             // address of name of binary file
                               NULL,                   // service does not belong to a group
                               NULL,                   // no tag requested
                               NULL,                   // no dependency names
                               NULL,                   // use LocalSystem account
                               NULL                    // no password for service account
                               );

    if (schService == NULL) {

        err = GetLastError();

        if (err == ERROR_SERVICE_EXISTS) {

            //
            // Ignore this error.
            //

            return TRUE;

        } else {

            printf("CreateService failed!  Error = %d \n", err );

            //
            // Indicate an error.
            //

            return  FALSE;
        }
    }

    //
    // Close the service object.
    //

    if (schService) {

        CloseServiceHandle(schService);
    }

    //
    // Indicate success.
    //

    return TRUE;

}   // InstallDriver

BOOLEAN
ManageDriver(
    _In_ LPCTSTR  DriverName,
    _In_ LPCTSTR  ServiceName,
    _In_ USHORT   Function
    )
{

    SC_HANDLE   schSCManager;

    BOOLEAN rCode = TRUE;

    //
    // Insure (somewhat) that the driver and service names are valid.
    //

    if (!DriverName || !ServiceName) {

        printf("Invalid Driver or Service provided to ManageDriver() \n");

        return FALSE;
    }

    //
    // Connect to the Service Control Manager and open the Services database.
    //

    schSCManager = OpenSCManager(NULL,                   // local machine
                                 NULL,                   // local database
                                 SC_MANAGER_ALL_ACCESS   // access required
                                 );

    if (!schSCManager) {

        printf("Open SC Manager failed! Error = %d \n", GetLastError());

        return FALSE;
    }

    //
    // Do the requested function.
    //

    switch( Function ) {

        case DRIVER_FUNC_INSTALL:

            //
            // Install the driver service.
            //

            if (InstallDriver(schSCManager,
                              DriverName,
                              ServiceName
                              )) {

                //
                // Start the driver service (i.e. start the driver).
                //

                rCode = StartDriver(schSCManager,
                                    DriverName
                                    );

            } else {

                //
                // Indicate an error.
                //

                rCode = FALSE;
            }

            break;

        case DRIVER_FUNC_REMOVE:

            //
            // Stop the driver.
            //

            StopDriver(schSCManager,
                       DriverName
                       );

            //
            // Remove the driver service.
            //

            RemoveDriver(schSCManager,
                         DriverName
                         );

            //
            // Ignore all errors.
            //

            rCode = TRUE;

            break;

        default:

            printf("Unknown ManageDriver() function. \n");

            rCode = FALSE;

            break;
    }

    //
    // Close handle to service control manager.
    //

    if (schSCManager) {

        CloseServiceHandle(schSCManager);
    }

    return rCode;

}   // ManageDriver


BOOLEAN
RemoveDriver(
    _In_ SC_HANDLE    SchSCManager,
    _In_ LPCTSTR      DriverName
    )
{
    SC_HANDLE   schService;
    BOOLEAN     rCode;

    //
    // Open the handle to the existing service.
    //

    schService = OpenService(SchSCManager,
                             DriverName,
                             SERVICE_ALL_ACCESS
                             );

    if (schService == NULL) {

        printf("OpenService failed!  Error = %d \n", GetLastError());

        //
        // Indicate error.
        //

        return FALSE;
    }

    //
    // Mark the service for deletion from the service control manager database.
    //

    if (DeleteService(schService)) {

        //
        // Indicate success.
        //

        rCode = TRUE;

    } else {

        printf("DeleteService failed!  Error = %d \n", GetLastError());

        //
        // Indicate failure.  Fall through to properly close the service handle.
        //

        rCode = FALSE;
    }

    //
    // Close the service object.
    //

    if (schService) {

        CloseServiceHandle(schService);
    }

    return rCode;

}   // RemoveDriver



BOOLEAN
StartDriver(
    _In_ SC_HANDLE    SchSCManager,
    _In_ LPCTSTR      DriverName
    )
{
    SC_HANDLE   schService;
    DWORD       err;

    //
    // Open the handle to the existing service.
    //

    schService = OpenService(SchSCManager,
                             DriverName,
                             SERVICE_ALL_ACCESS
                             );

    if (schService == NULL) {

        printf("OpenService failed!  Error = %d \n", GetLastError());

        //
        // Indicate failure.
        //

        return FALSE;
    }

    //
    // Start the execution of the service (i.e. start the driver).
    //

    if (!StartService(schService,     // service identifier
                      0,              // number of arguments
                      NULL            // pointer to arguments
                      )) {

        err = GetLastError();

        if (err == ERROR_SERVICE_ALREADY_RUNNING) {

            //
            // Ignore this error.
            //

            return TRUE;

        } else {

            printf("StartService failure! Error = %d \n", err );

            //
            // Indicate failure.  Fall through to properly close the service handle.
            //

            return FALSE;
        }

    }

    //
    // Close the service object.
    //

    if (schService) {

        CloseServiceHandle(schService);
    }

    return TRUE;

}   // StartDriver



BOOLEAN
StopDriver(
    _In_ SC_HANDLE    SchSCManager,
    _In_ LPCTSTR      DriverName
    )
{
    BOOLEAN         rCode = TRUE;
    SC_HANDLE       schService;
    SERVICE_STATUS  serviceStatus;

    //
    // Open the handle to the existing service.
    //

    schService = OpenService(SchSCManager,
                             DriverName,
                             SERVICE_ALL_ACCESS
                             );

    if (schService == NULL) {

        printf("OpenService failed!  Error = %d \n", GetLastError());

        return FALSE;
    }

    //
    // Request that the service stop.
    //

    if (ControlService(schService,
                       SERVICE_CONTROL_STOP,
                       &serviceStatus
                       )) {

        //
        // Indicate success.
        //

        rCode = TRUE;

    } else {

        printf("ControlService failed!  Error = %d \n", GetLastError() );

        //
        // Indicate failure.  Fall through to properly close the service handle.
        //

        rCode = FALSE;
    }

    //
    // Close the service object.
    //

    if (schService) {

        CloseServiceHandle (schService);
    }

    return rCode;

}   //  StopDriver

BOOLEAN
SetupDriverName(
    _Inout_updates_bytes_all_(BufferLength) PCHAR DriverLocation,
    _In_ ULONG BufferLength
    )
{
    HANDLE fileHandle;
    DWORD driverLocLen = 0;

    //
    // Get the current directory.
    //

    driverLocLen = GetCurrentDirectory(BufferLength,
                                       DriverLocation
                                       );

    if (driverLocLen == 0) {

        printf("GetCurrentDirectory failed!  Error = %d \n", GetLastError());

        return FALSE;
    }

    //
    // Setup path name to driver file.
    //
    if (FAILED( StringCbCat(DriverLocation, BufferLength, "\\"DRIVER_NAME".sys") )) {
        return FALSE;
    }

    //
    // Insure driver file is in the specified directory.
    //

    if ((fileHandle = CreateFile(DriverLocation,
                                 GENERIC_READ,
                                 0,
                                 NULL,
                                 OPEN_EXISTING,
                                 FILE_ATTRIBUTE_NORMAL,
                                 NULL
                                 )) == INVALID_HANDLE_VALUE) {


        printf("%s.sys is not loaded.\n", DRIVER_NAME);

        //
        // Indicate failure.
        //

        return FALSE;
    }

    //
    // Close open file handle.
    //

    if (fileHandle) {

        CloseHandle(fileHandle);
    }

    //
    // Indicate success.
    //

    return TRUE;


}   // SetupDriverName



