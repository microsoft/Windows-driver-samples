/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Enum.c

Abstract:
        This application simulates the plugin, unplug or ejection
        of devices.

Environment:

    usermode console application

Revision History:

  Eliyas Yakub  Oct 14, 1998


--*/

#include <basetyps.h>
#include <stdlib.h>
#include <wtypes.h>
#include <strsafe.h>
#include <cfgmgr32.h>
#include <initguid.h>
#include <stdio.h>
#include <string.h>
#include <winioctl.h>
#include "public.h"
#include <dontuse.h>

//
// Prototypes
//
BOOLEAN
GetDevicePath(
    _In_ LPCGUID InterfaceGuid,
    _Out_writes_(BufLen) PWCHAR DevicePath,
    _In_ size_t BufLen
    );

BOOLEAN
OpenBusInterface(
    _In_z_ LPCWSTR DevicePath
    );

#define USAGE  \
"Usage: Enum [-p SerialNo] Plugs in a device. SerialNo must be greater than zero.\n\
             [-u SerialNo or 0] Unplugs device(s) - specify 0 to unplug all \
                                the devices enumerated so far.\n\
             [-e SerialNo or 0] Ejects device(s) - specify 0 to eject all \
                                the devices enumerated so far.\n"

#define MAX_DEVPATH_LENGTH                       256

BOOLEAN     bPlugIn, bUnplug, bEject;
ULONG       SerialNo;

INT __cdecl
main(
    _In_ ULONG argc,
    _In_reads_(argc) PCHAR argv[]
    )
{
    WCHAR devicePath[MAX_DEVPATH_LENGTH] = { 0 };

    bPlugIn = bUnplug = bEject = FALSE;

    if(argc <3) {
        goto usage;
    }

    if(argv[1][0] == '-') {
        if(tolower(argv[1][1]) == 'p') {
            if(argv[2])
                SerialNo = (USHORT)atol(argv[2]);
        bPlugIn = TRUE;
        }
        else if(tolower(argv[1][1]) == 'u') {
            if(argv[2])
                SerialNo = (ULONG)atol(argv[2]);
            bUnplug = TRUE;
        }
        else if(tolower(argv[1][1]) == 'e') {
            if(argv[2])
                SerialNo = (ULONG)atol(argv[2]);
            bEject = TRUE;
        }
        else {
            goto usage;
        }
    }
    else
        goto usage;

    if(bPlugIn && 0 == SerialNo)
        goto usage;

    if (GetDevicePath(&GUID_DEVINTERFACE_BUSENUM_TOASTER,
                devicePath,
                sizeof(devicePath) / sizeof(devicePath[0]))) {
        OpenBusInterface(devicePath);
    }

    return 0;
usage:
    printf(USAGE);
    exit(0);
}

BOOLEAN
OpenBusInterface (
    _In_z_ LPCWSTR DevicePath
    )
{
    HANDLE                              file;
    ULONG                               bytes;
    BUSENUM_UNPLUG_HARDWARE             unplug;
    BUSENUM_EJECT_HARDWARE              eject;
    PBUSENUM_PLUGIN_HARDWARE            hardware;
    BOOLEAN                             bSuccess = FALSE;

    printf("Opening %ws\n", DevicePath);

    file = CreateFile(DevicePath,
                    GENERIC_READ, // Only read access
                    0, // FILE_SHARE_READ | FILE_SHARE_WRITE
                    NULL, // no SECURITY_ATTRIBUTES structure
                    OPEN_EXISTING, // No special create flags
                    0, // No special attributes
                    NULL); // No template file
    if (INVALID_HANDLE_VALUE == file) {
        printf("CreateFile failed: 0x%x", GetLastError());
        goto End;
    }

    printf("Bus interface opened!!!\n");

    //
    // Enumerate Devices
    //

    if(bPlugIn) {

        printf("SerialNo. of the device to be enumerated: %d\n", SerialNo);

        hardware = malloc (bytes = (sizeof (BUSENUM_PLUGIN_HARDWARE) +
                                              BUS_HARDWARE_IDS_LENGTH));

        if(hardware) {
            hardware->Size = sizeof (BUSENUM_PLUGIN_HARDWARE);
            hardware->SerialNo = SerialNo;
        } else {
            printf("Couldn't allocate %d bytes for busenum plugin hardware structure.\n", bytes);
            goto End;
        }

        //
        // Allocate storage for the Device ID
        //
        memcpy (hardware->HardwareIDs,
                BUS_HARDWARE_IDS,
                BUS_HARDWARE_IDS_LENGTH);

        if (!DeviceIoControl (file,
                              IOCTL_BUSENUM_PLUGIN_HARDWARE ,
                              hardware, bytes,
                              NULL, 0,
                              &bytes, NULL)) {
              free (hardware);
              printf("PlugIn failed:0x%x\n", GetLastError());
              goto End;
        }

        free (hardware);
    }

    //
    // Removes a device if given the specific Id of the device. Otherwise this
    // ioctls removes all the devices that are enumerated so far.
    //

    if(bUnplug) {
        printf("Unplugging device(s)....\n");

        unplug.Size = bytes = sizeof (unplug);
        unplug.SerialNo = SerialNo;
        if (!DeviceIoControl (file,
                              IOCTL_BUSENUM_UNPLUG_HARDWARE,
                              &unplug, bytes,
                              NULL, 0,
                              &bytes, NULL)) {
            printf("Unplug failed: 0x%x\n", GetLastError());
            goto End;
        }
    }

    //
    // Ejects a device if given the specific Id of the device. Otherwise this
    // ioctls ejects all the devices that are enumerated so far.
    //
    if(bEject) {
        printf("Ejecting Device(s)\n");

        eject.Size = bytes = sizeof (eject);
        eject.SerialNo = SerialNo;
        if (!DeviceIoControl (file,
                              IOCTL_BUSENUM_EJECT_HARDWARE,
                              &eject, bytes,
                              NULL, 0,
                              &bytes, NULL)) {
            printf("Eject failed: 0x%x\n", GetLastError());
            goto End;
        }
    }

    printf("Success!!!\n");
    bSuccess = TRUE;

End:
    if (INVALID_HANDLE_VALUE != file) {
        CloseHandle(file);
    }
    return bSuccess;
}

BOOLEAN
GetDevicePath(
    _In_ LPCGUID InterfaceGuid,
    _Out_writes_(BufLen) PWCHAR DevicePath,
    _In_ size_t BufLen
)
{
    CONFIGRET cr = CR_SUCCESS;
    PWSTR deviceInterfaceList = NULL;
    ULONG deviceInterfaceListLength = 0;
    PWSTR nextInterface;
    HRESULT hr = E_FAIL;
    BOOLEAN bRet = TRUE;

    cr = CM_Get_Device_Interface_List_Size(
                &deviceInterfaceListLength,
                (LPGUID)InterfaceGuid,
                NULL,
                CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    if (cr != CR_SUCCESS) {
        printf("Error 0x%x retrieving device interface list size.\n", cr);
        goto clean0;
    }

    if (deviceInterfaceListLength <= 1) {
        bRet = FALSE;
        printf("Error: No active device interfaces found.\n"
            " Is the sample driver loaded?");
        goto clean0;
    }

    deviceInterfaceList = (PWSTR)malloc(deviceInterfaceListLength * sizeof(WCHAR));
    if (deviceInterfaceList == NULL) {
        printf("Error allocating memory for device interface list.\n");
        goto clean0;
    }
    ZeroMemory(deviceInterfaceList, deviceInterfaceListLength * sizeof(WCHAR));

    cr = CM_Get_Device_Interface_List(
                (LPGUID)InterfaceGuid,
                NULL,
                deviceInterfaceList,
                deviceInterfaceListLength,
                CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    if (cr != CR_SUCCESS) {
        printf("Error 0x%x retrieving device interface list.\n", cr);
        goto clean0;
    }

    nextInterface = deviceInterfaceList + wcslen(deviceInterfaceList) + 1;
    if (*nextInterface != UNICODE_NULL) {
        printf("Warning: More than one device interface instance found. \n"
            "Selecting first matching device.\n\n");
    }

    hr = StringCchCopy(DevicePath, BufLen, deviceInterfaceList);
    if (FAILED(hr)) {
        bRet = FALSE;
        printf("Error: StringCchCopy failed with HRESULT 0x%x", hr);
        goto clean0;
    }

clean0:
    if (deviceInterfaceList != NULL) {
        free(deviceInterfaceList);
    }
    if (CR_SUCCESS != cr) {
        bRet = FALSE;
    }

    return bRet;
}
