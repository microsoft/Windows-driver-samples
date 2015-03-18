/*++

Copyright (c) 1990-98  Microsoft Corporation All Rights Reserved

Module Name:

    testapp.c

Abstract:

Environment:

    Win32 console multi-threaded application

--*/
#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strsafe.h>
#include "..\sys\sdma.h"


BOOLEAN
ManageDriver(
    _In_ LPCTSTR  DriverName,
    _In_ LPCTSTR  ServiceName,
    _In_ USHORT   Function
    );

BOOLEAN
SetupDriverName(
    _Inout_updates_bytes_all_(BufferLength) PCHAR DriverLocation,
    _In_ ULONG BufferLength
    );

char OutputBuffer[100];
char InputBuffer[100];

VOID __cdecl
main(
    _In_ ULONG argc,
    _In_reads_(argc) PCHAR argv[]
    )
{
    HANDLE hDevice;
    BOOL bRc;
    ULONG bytesReturned;
    DWORD errNum = 0;
    TCHAR driverLocation[MAX_PATH];

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    //
    // open the device
    //

    if ((hDevice = CreateFile( "\\\\.\\DmaTest",
                            GENERIC_READ | GENERIC_WRITE,
                            0,
                            NULL,
                            CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL)) == INVALID_HANDLE_VALUE) {

        errNum = GetLastError();

        if (errNum != ERROR_FILE_NOT_FOUND) {

            printf("CreateFile failed!  ERROR_FILE_NOT_FOUND = %d\n", (int)errNum);

            return ;
        }

        //
        // The driver is not started yet so let us the install the driver.
        // First setup full path to driver name.
        //

        if (!SetupDriverName(driverLocation, sizeof(driverLocation))) {

            return ;
        }

        if (!ManageDriver(DRIVER_NAME,
                          driverLocation,
                          DRIVER_FUNC_INSTALL
                          )) {

            printf("Unable to install driver. \n");

            //
            // Error - remove driver.
            //

            ManageDriver(DRIVER_NAME,
                         driverLocation,
                         DRIVER_FUNC_REMOVE
                         );

            return;
        }

        hDevice = CreateFile( "\\\\.\\DmaTest",
                            GENERIC_READ | GENERIC_WRITE,
                            0,
                            NULL,
                            CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);

        if ( hDevice == INVALID_HANDLE_VALUE ){
            printf ( "Error: CreatFile Failed : %d\n", (int)GetLastError());
            return;
        }

    }

#if 0
    //
    // Printing Input & Output buffer pointers and size
    //

    printf("InputBuffer Pointer = %p, BufLength = %d\n", (ULONG *)InputBuffer,
                        sizeof(InputBuffer));
    printf("OutputBuffer Pointer = %p BufLength = %d\n", (ULONG *)OutputBuffer,
                                sizeof(OutputBuffer));
#endif

    //
    // Performing METHOD_BUFFERED
    //

    StringCbCopy(InputBuffer, sizeof(InputBuffer),
        "This String is from User Application; using METHOD_BUFFERED");

    printf("\nCalling DeviceIoControl METHOD_BUFFERED:\n");

    memset(OutputBuffer, 0, sizeof(OutputBuffer));

    bRc = DeviceIoControl ( hDevice,
                            (DWORD) IOCTL_SDMA_WRITE,
                            &InputBuffer,
                            (DWORD) strlen ( InputBuffer )+1,
                            &OutputBuffer,
                            sizeof( OutputBuffer),
                            &bytesReturned,
                            NULL
                            );

    if ( !bRc )
    {
        printf ( "Error in DeviceIoControl : %d", (int)GetLastError());
        return;

    }
    printf("    OutBuffer (%d): %s\n", bytesReturned, OutputBuffer);

    CloseHandle ( hDevice );

    //
    // Unload the driver.  Ignore any errors.
    //

    driverLocation[ 259 ] = '\0';
    ManageDriver(DRIVER_NAME,
                 driverLocation,
                 DRIVER_FUNC_REMOVE
                 );


    //
    // close the handle to the device.
    //

}


