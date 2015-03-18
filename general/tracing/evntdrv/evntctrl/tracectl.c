/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    tracectl.c

Environment:

    User mode Win32 console application

Revision History:


--*/
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <winioctl.h>
#include <tchar.h>
#include <stdio.h>
#include "drvioctl.h"
#include "install.h"
#include <conio.h>
#include <strsafe.h>

#define WAIT_TIME       10

BOOLEAN
SetupDriverName(
    _Out_writes_(MAX_PATH)LPTSTR DriverLocation
    );

int _cdecl main(int argc, LPCTSTR argv[])
{
    HANDLE hDevice;              // handle to a device, file, or directory 
    DWORD  dwError = ERROR_SUCCESS;
    LPVOID lpFileName = _T("\\\\.\\EventEtw") ;
    TCHAR  driverLocation[MAX_PATH];
    DWORD     dwOutBuffer[2048];
    DWORD  dwOutBufferCount ;
    int ch;
  
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    if ((hDevice = CreateFile(
                        lpFileName,             // pointer to name of the file
                        0,                      // access (read-write) mode
                        0,                      // share mode
                        NULL,                   // pointer to security attributes
                        OPEN_EXISTING,          // how to create
                        FILE_ATTRIBUTE_NORMAL,  // file attributes
                        NULL                    // handle to file with attributes to 
                                                // copy
                                    )) == INVALID_HANDLE_VALUE) {
        dwError = GetLastError();

        if ( dwError != ERROR_FILE_NOT_FOUND ) {
            _tprintf(_T("CreateFile failed ! error: %d\n"), dwError);
            return 1;
        }

        //
        //  Setup full path to driver name
        //

        if (!SetupDriverName(driverLocation)) {

            return 2;

        }

        //
        // Install driver
        //

        if (!ManageDriver(DRIVER_NAME,
                          driverLocation,
                          DRIVER_FUNC_INSTALL
                          )) {

            _tprintf(_T("Unable to install driver. \n"));

            //
            // Error - remove driver.
            //

            ManageDriver(DRIVER_NAME,
                         driverLocation,
                         DRIVER_FUNC_REMOVE
                         );
            
            return 3;
        }

        if ((hDevice = CreateFile(
                                lpFileName,             // pointer to name of the file
                                0,                      // access (read-write) mode
                                0,                      // share mode
                                NULL,                   // pointer to security attributes
                                OPEN_EXISTING,          // how to create
                                FILE_ATTRIBUTE_NORMAL,  // file attributes
                                NULL                    // handle to file with attributes to 
                                                        // copy
                                            )) == INVALID_HANDLE_VALUE) {        

            _tprintf(_T("Error: CreateFile failed\n"));
            return 4;
        }
    }



    _tprintf(_T("\nPress 'q' to exit, any other key to send ioctl...\n"));
    fflush(stdin);
    ch = _getche();

    while(tolower(ch) != 'q' )
    {
    
         _tprintf(_T("Making IOCTL_EVNTKMP_TRACE_EVENT_A ioctl to log events\n"));
        if (DeviceIoControl(
            hDevice,                    // handle to a device, file, or directory 
            IOCTL_EVNTKMP_TRACE_EVENT_A, // control code of operation to perform
            NULL,                        // pointer to buffer to supply input data
            0,                            // size, in bytes, of input buffer
            dwOutBuffer,                // pointer to buffer to receive output data
            2048,                        // size, in bytes, of output buffer
            &dwOutBufferCount,            // pointer to variable to receive byte count
            NULL                        // pointer to structure for asynchronous operation
            ) == 0) {

            _tprintf(_T("DeviceIOControl Failed %d\n"),GetLastError());
            return 5;

        }
        ch = _getche();
    }

    if (CloseHandle(hDevice) == 0) {

        _tprintf(_T("CloseHandle Failed %d\n"),GetLastError());
        return 6;

    }

    //
    //  stop the driver
    //
    
    ManageDriver(DRIVER_NAME,
                 driverLocation,
                 DRIVER_FUNC_REMOVE
                 );

    _tprintf(_T("Driver '%s' is removed\n"), DRIVER_NAME);

    return 0;
}

BOOLEAN
SetupDriverName(
    _Out_writes_(MAX_PATH) LPTSTR DriverLocation
    )
{
    HANDLE fileHandle;

    DWORD driverLocLen = 0;

    //
    // Get the current directory.
    //

    driverLocLen = GetCurrentDirectory(MAX_PATH,
                                       DriverLocation
                                       );

    if (!driverLocLen) {

        _tprintf(_T("GetCurrentDirectory failed!  Error = %d \n"), GetLastError());

        return FALSE;
    }

    //
    // Setup path name to driver file.
    //

    if (StringCchPrintf(&DriverLocation[_tcslen(DriverLocation)], 
                        (MAX_PATH - _tcslen(DriverLocation)),
                        _T("\\%s.sys"),
                        DRIVER_NAME) != S_OK){
        _tprintf(_T("Failed to generate DriverLocation!,  StringCchPrintf Error = %d \n"), GetLastError());
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


        _tprintf(_T("Driver: '%s' is not in the current directory. \n"), DRIVER_NAME);

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
