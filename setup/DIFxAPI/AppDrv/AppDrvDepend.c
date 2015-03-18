/*++

Copyright (c) Microsoft Corporation


Description:

This sample demonstrates how to use the application-to-driver dependency
feature of the Driver Install Frameworks (DIFx) API. The DriverPackageInstall
and the DriverPackageUninstall APIs allow the calling application to register
and remove dependencies on a driver package. In order to register or remove
a dependency on a driver package, the calling application needs to populate
the fields of an INSTALLERINFO structure with information about itself. It
then needs to pass in this structure as a parameter to the DIFx APIs, as
shown in this sample.

--*/
#include <windows.h>
#include <stdio.h>
#include <difxapi.h>

int __cdecl
wmain( int argc, WCHAR* argv[] )
{
    WCHAR *InfPath = NULL;
    INSTALLERINFO AppInfo;
    BOOL Result;
    BOOL NeedReboot;

    if (argc < 7) {
        wprintf( TEXT("USAGE: AppDrv.exe <option> <full path to the inf file> <Application ID> <Product display name> <Product name> <Manufacturer name>\n") );
        wprintf( TEXT("<option> can be one of the following -\n") );
        wprintf( TEXT("/i - Install the driver\n") );
        wprintf( TEXT("/u - Uninstall the driver\n") );
        return 0;
    }

    //
    // Initialization
    //
    NeedReboot = FALSE;

    //
    // Get the path to the INF file
    //
    InfPath = argv[2];

    //
    // Fill in the fields of the INSTALLERINFO structure with information about
    // the application performing the install.
    //

    //
    // The application ID should be some random string unique to this
    // application.
    //
    AppInfo.pApplicationId = argv[3];
    AppInfo.pDisplayName = argv[4];
    AppInfo.pProductName = argv[5];
    AppInfo.pMfgName = argv[6];

    //
    // The option parameter should be present and be exactly two characters.
    //
    if ((argv[1] == NULL) ||
        (argv[1][0] == TEXT('\0')) ||
        (argv[1][1] == TEXT('\0')) ||
        (argv[1][2] != TEXT('\0'))) {
        wprintf( TEXT("ERROR: An invalid command line option %ws was passed in!\n"), argv[1] );
        goto clean0;
    }

    if ((argv[1][0] == TEXT('/')) &&
        ((argv[1][1] == TEXT('i')) ||
         (argv[1][1] == TEXT('I')))) {
        //
        // Install the driver package, passing in the INSTALLERINFO structure with
        // information about the application performing the install. By passing in
        // this structure, the "MySoftware" application is expressing a dependency
        // on the driver being installed.
        //
        wprintf( TEXT("INFO: Attempting to install the driver package %ws.\n"), InfPath );

        Result = DriverPackageInstall(InfPath,
                                      0,        // flags
                                      &AppInfo, // dependency information
                                      &NeedReboot);

        if (ERROR_SUCCESS == Result) {
            wprintf( TEXT("SUCCESS: installed driver package %ws.\n"), InfPath );
        } else if (ERROR_NO_MORE_ITEMS == Result) {
            wprintf( TEXT("INFO: the driver package %ws was successfully staged on the machine.\n"), InfPath );
            wprintf( TEXT("INFO: All devices found already have a better driver than what is contained in the specified inf file.\n"));
        } else if (ERROR_NO_SUCH_DEVINST == Result) {
            wprintf( TEXT("INFO: the driver package %ws was successfully staged on the machine.\n"), InfPath );
            wprintf( TEXT("INFO: There aren't any live devnodes with the device ID contained in the INF.\n"));
        } else if (ERROR_NO_DEVICE_ID == Result) {
            wprintf( TEXT("INFO: the driver package %ws was successfully staged on the machine.\n"), InfPath );
            wprintf( TEXT("INFO: No device IDs found in INF '%ws' for current platform.\n"), InfPath);
        } else {
            wprintf( TEXT("ERROR: An error occurred while installing driver package %ws. Error code - 0x%X.\n"), InfPath, Result);
            goto clean0;
        }

        if (NeedReboot){
            wprintf( TEXT("INFO: Machine will have to be rebooted to complete install.\n"));
        }

    } else if ((argv[1][0] == TEXT('/')) &&
               ((argv[1][1] == TEXT('u')) ||
                (argv[1][1] == TEXT('U')))) {
        //
        // Uninstall the driver package, passing in the INSTALLERINFO structure
        // with information about the application performing the uninstall. By
        // passing in this structure, the "MySoftware" application is removing
        // its dependency on the driver. The driver will actually be
        // uninstalled only if there are no other applications dependent on it.
        // If there are other applications depending on the driver, then the call
        // to DriverPackageUninstall will simply remove the dependency of the
        // "MySoftware" application on the driver, and return
        // ERROR_DEPENDENT_APPLICATIONS_EXIST.
        //
        wprintf( TEXT("INFO: Attempting to uninstall the driver package %ws.\n"), InfPath );

        Result = DriverPackageUninstall(InfPath,
                                        0,        // flags
                                        &AppInfo, // dependency information
                                        &NeedReboot);
        if (ERROR_SUCCESS != Result) {
            if (ERROR_DEPENDENT_APPLICATIONS_EXIST == Result) {
                wprintf( TEXT("INFO: The driver package %ws was not uninstalled because there were some applications still depending on it.\n"), InfPath);
            } else {
                wprintf( TEXT("ERROR: An error occurred while uninstalling driver package %ws\n."), InfPath);
                goto clean0;
            }
        } else {
            wprintf( TEXT("SUCCESS: Uninstalled driver package %ws.\n"), InfPath );
        }
    } else {
        wprintf( TEXT("ERROR: An invalid command line option %ws was passed in!\n"), argv[1] );
    }

    clean0:
        return 0;
}


