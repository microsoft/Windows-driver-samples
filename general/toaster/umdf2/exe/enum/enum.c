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
#include <setupapi.h>
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
OpenBusInterface (
    _In_       HDEVINFO                    HardwareDeviceInfo,
    _In_       PSP_DEVICE_INTERFACE_DATA   DeviceInterfaceData
    );



#define USAGE  \
"Usage: Enum [-p SerialNo] Plugs in a device. SerialNo must be greater than zero.\n\
             [-u SerialNo or 0] Unplugs device(s) - specify 0 to unplug all \
                                the devices enumerated so far.\n\
             [-e SerialNo or 0] Ejects device(s) - specify 0 to eject all \
                                the devices enumerated so far.\n"

BOOLEAN     bPlugIn, bUnplug, bEject;
ULONG       SerialNo;

INT __cdecl
main(
    _In_ ULONG argc,
    _In_reads_(argc) PCHAR argv[]
    )
{
    HDEVINFO                    hardwareDeviceInfo;
    SP_DEVICE_INTERFACE_DATA    deviceInterfaceData;

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
    //
    // Open a handle to the device interface information set of all
    // present toaster bus enumerator interfaces.
    //

    hardwareDeviceInfo = SetupDiGetClassDevs (
                       (LPGUID)&GUID_DEVINTERFACE_BUSENUM_TOASTER,
                       NULL, // Define no enumerator (global)
                       NULL, // Define no
                       (DIGCF_PRESENT | // Only Devices present
                       DIGCF_DEVICEINTERFACE)); // Function class devices.

    if(INVALID_HANDLE_VALUE == hardwareDeviceInfo)
    {
        printf("SetupDiGetClassDevs failed: %x\n", GetLastError());
        return 0;
    }

    deviceInterfaceData.cbSize = sizeof (SP_DEVICE_INTERFACE_DATA);

    if (SetupDiEnumDeviceInterfaces (hardwareDeviceInfo,
                                 0, // No care about specific PDOs
                                 (LPGUID)&GUID_DEVINTERFACE_BUSENUM_TOASTER,
                                 0, //
                                 &deviceInterfaceData)) {

        OpenBusInterface(hardwareDeviceInfo, &deviceInterfaceData);
    } else if (ERROR_NO_MORE_ITEMS == GetLastError()) {

        printf(
        "Error:Interface GUID_DEVINTERFACE_BUSENUM_TOASTER is not registered\n");
    }

    SetupDiDestroyDeviceInfoList (hardwareDeviceInfo);
    return 0;
usage:
    printf(USAGE);
    exit(0);
}

BOOLEAN
OpenBusInterface (
    _In_       HDEVINFO                    HardwareDeviceInfo,
    _In_       PSP_DEVICE_INTERFACE_DATA   DeviceInterfaceData
    )
{
    HANDLE                              file;
    PSP_DEVICE_INTERFACE_DETAIL_DATA    deviceInterfaceDetailData = NULL;
    ULONG                               predictedLength = 0;
    ULONG                               requiredLength = 0;
    ULONG                               bytes;
    BUSENUM_UNPLUG_HARDWARE             unplug;
    BUSENUM_EJECT_HARDWARE              eject;
    PBUSENUM_PLUGIN_HARDWARE            hardware;
    BOOLEAN                             bSuccess;

    //
    // Allocate a function class device data structure to receive the
    // information about this particular device.
    //

    SetupDiGetDeviceInterfaceDetail (
            HardwareDeviceInfo,
            DeviceInterfaceData,
            NULL, // probing so no output buffer yet
            0, // probing so output buffer length of zero
            &requiredLength,
            NULL); // not interested in the specific dev-node

    if(ERROR_INSUFFICIENT_BUFFER != GetLastError()) {
        printf("Error in SetupDiGetDeviceInterfaceDetail%d\n",
                                                            GetLastError());
        return FALSE;
    }

    predictedLength = requiredLength;

    deviceInterfaceDetailData = malloc (predictedLength);

    if(deviceInterfaceDetailData) {
        deviceInterfaceDetailData->cbSize =
                      sizeof (SP_DEVICE_INTERFACE_DETAIL_DATA);
    } else {
        printf("Couldn't allocate %d bytes for device interface details.\n", predictedLength);
        return FALSE;
    }


    if (! SetupDiGetDeviceInterfaceDetail (
               HardwareDeviceInfo,
               DeviceInterfaceData,
               deviceInterfaceDetailData,
               predictedLength,
               &requiredLength,
               NULL)) {
        printf("Error in SetupDiGetDeviceInterfaceDetail\n");
        free (deviceInterfaceDetailData);
        return FALSE;
    }

    printf("Opening %s\n", deviceInterfaceDetailData->DevicePath);

    file = CreateFile ( deviceInterfaceDetailData->DevicePath,
                        GENERIC_READ, // Only read access
                        0, // FILE_SHARE_READ | FILE_SHARE_WRITE
                        NULL, // no SECURITY_ATTRIBUTES structure
                        OPEN_EXISTING, // No special create flags
                        0, // No special attributes
                        NULL); // No template file

    if (INVALID_HANDLE_VALUE == file) {
        printf("CreateFile failed: 0x%x", GetLastError());
        free (deviceInterfaceDetailData);
        return FALSE;
    }

    printf("Bus interface opened!!!\n");

    //
    // From this point on, we need to jump to the end of the routine for
    // common clean-up.  Keep track of whether we succeeded or failed, so
    // we'll know what to return to the caller.
    //
    bSuccess = FALSE;

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

    if(bEject)
    {
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
    CloseHandle(file);
    free (deviceInterfaceDetailData);
    return bSuccess;
}


