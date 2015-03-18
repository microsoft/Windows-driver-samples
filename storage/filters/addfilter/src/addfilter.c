/*++

Copyright (C) Microsoft Corporation, 1999 - 1999

Module Name:

    addfilter.c

Abstract:

    This command line utility adds and removes upper filter drivers
    for a given drive or volume

Author:

    Benjamin Strautin (t-bensta)

Environment:

    User mode only

Notes:

    - the filter is not checked for validity before it is added to the driver
      stack; if an invalid filter is added, the device may no longer be
      accessible.
    - all code works irrespective of character set (ANSI, Unicode, ...)

Revision History:

    05-24-99 : created

--*/

#include <driverspecs.h>
_Analysis_mode_(_Analysis_code_type_user_code_);

#include <windows.h>
#include <stdio.h>
#include <malloc.h>

// defines GUID
#include <initguid.h>

// the SetupDiXXX api (from the DDK)
#include <setupapi.h>

// defines guids for device classes (GUID_DEVINTERFACE_DISK, etc)
#include <devioctl.h>
#include <ntddstor.h>

// for all of the _t stuff (to allow compiling for both Unicode/Ansi)
#include <tchar.h>

#include <strsafe.h>

#if DBG
#include <assert.h>
#define ASSERT(condition) assert(condition)
#else
#define ASSERT(condition)
#endif

BOOLEAN
AddFilterDriver(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    _In_ IN LPTSTR Filter,
    IN BOOLEAN UpperFilter
    );

BOOLEAN
RemoveFilterDriver(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    _In_ IN LPTSTR Filter,
    IN BOOLEAN UpperFilter
    );

void
PrintFilters(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN BOOLEAN UpperFilters
    );

LPTSTR
GetFilters(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN BOOLEAN UpperFilters
    );

void PrintDeviceName(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData
    );

BOOLEAN
DeviceNameMatches(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    _In_ IN LPTSTR DeviceName
    );

PBYTE
GetDeviceRegistryProperty(
    IN  HDEVINFO DeviceInfoSet,
    IN  PSP_DEVINFO_DATA DeviceInfoData,
    IN  DWORD Property,
    OUT PDWORD PropertyRegDataType
    );

BOOLEAN
RestartDevice(
    IN HDEVINFO DeviceInfoSet,
    IN OUT PSP_DEVINFO_DATA DeviceInfoData
    );

BOOLEAN
PrependSzToMultiSz(
    _In_        LPTSTR  SzToPrepend,
    _Inout_ LPTSTR *MultiSz
    );

size_t
MultiSzLength(
    _In_ IN LPTSTR MultiSz
    );

size_t
MultiSzSearchAndDeleteCaseInsensitive(
    _In_ IN  LPTSTR  FindThis,
    _In_ IN  LPTSTR  FindWithin,
    OUT size_t  *NewStringLength
    );

void
PrintUsage();

// To add/remove filter drivers:
// - use SetupDiGetClassDevs to get a list of devices of the given interface
//   class
// - use SetupDiEnumDeviceInfo to enumerate the items in that list and
//   obtain a SP_DEVINFO_DATA
// - use SetupDiGetDeviceRegistryProperty to get the list of filter drivers
// - add/remove items in the filter list
// - use SetupDiSetDeviceRegistryProperty to put the list back in place
// To restart the device:
// - use SetupDiCallClassInstaller with DIF_PROPERTYCHANGE and DICS_STOP to
//   stop the device
// - use SetupDiCallClassInstaller with DIF_PROPERTYCHANGE and DICS_START to
//   restart the device

int __cdecl _tmain(int argc, _In_reads_(argc) LPTSTR argv[])
{
    // these two constants are used to help enumerate through the list of all
    // disks and volumes on the system. Adding another GUID should "just work"
    static const GUID * deviceGuids[] = {
        &GUID_DEVINTERFACE_DISK,
        &GUID_DEVINTERFACE_VOLUME,
        &GUID_DEVINTERFACE_CDROM
    };
    static const int numdeviceGuids = sizeof(deviceGuids) / sizeof(LPGUID);

    // structs needed to contain information about devices
    HDEVINFO                 devInfo = INVALID_HANDLE_VALUE;
    SP_DEVINFO_DATA          devInfoData;

    // indices for stepping through devices, and device interface guids
    int argIndex;
    int devGuidIndex;
    int deviceIndex;

    // variables used to deal with the command-line options of this program
    BOOLEAN listDevices   = FALSE;
    BOOLEAN upperFilter   = TRUE;

    LPTSTR deviceName     = NULL;
    LPTSTR filterToAdd    = NULL;
    LPTSTR filterToRemove = NULL;

    BOOLEAN keepGoing   = TRUE;
    BOOLEAN needReboot  = FALSE;
    BOOLEAN deviceMatch = FALSE;

    ////////////////////////////////////////////////
    // parse arguments; nothing too exciting here //
    ////////////////////////////////////////////////

    if( argc < 2 || _tcscmp(argv[1], _T("/?")) == 0 )
    {
        PrintUsage();
        return (0);
    }
 
    for (argIndex = 1; argIndex < argc; argIndex++) {

#pragma prefast(suppress:6385, "Previously checked argIndex being less than argc. No buffer overflow.")   
        if( _tcscmp(argv[argIndex], _T("/listdevices")) == 0 ) {

            listDevices = TRUE;

        } else if( _tcscmp(argv[argIndex], _T("/lower")) == 0 ) {

            upperFilter = FALSE;
            printf("Using Lower Filters\n");

        } else if( _tcscmp(argv[argIndex], _T("/device")) == 0 ) {

            argIndex++;

            if( argIndex < argc ) {
                deviceName = argv[argIndex];
            } else {
                PrintUsage();
                return (0);
            }

        } else if( _tcscmp(argv[argIndex], _T("/add")) == 0 ) {

            argIndex++;

            if( argIndex<argc ) {
                filterToAdd = argv[argIndex];
            } else {
                PrintUsage();
                return (0);
            }

        } else if( _tcscmp(argv[argIndex], _T("/remove")) == 0 ) {

            argIndex++;
            if( argIndex<argc ) {
                filterToRemove = argv[argIndex];
            } else {
                PrintUsage();
                return (0);
            }

        } else {
            PrintUsage();
            return (0);
        }

    }

    //////////////////////////////////////////////////////
    // done parsing arguments, move onto the good stuff //
    //////////////////////////////////////////////////////

    // This outer loop steps through the array of device guid pointers that is
    // defined above main(). It was just the easiest way to deal with both
    // Disks and Volumes (and it is easy to add other types of devices)

    for(devGuidIndex = 0; devGuidIndex<numdeviceGuids; devGuidIndex++) {

        // get a list of devices which support the given interface
        devInfo = SetupDiGetClassDevs( deviceGuids[devGuidIndex],
                                       NULL,
                                       NULL,
                                       DIGCF_PROFILE |
                                       DIGCF_DEVICEINTERFACE |
                                       DIGCF_PRESENT );

        if( devInfo == INVALID_HANDLE_VALUE ) {
            printf("got INVALID_HANDLE_VALUE!\n");
            return (1);
        }

        // as per DDK docs on SetupDiEnumDeviceInfo
        devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        // step through the list of devices for this handle
        // get device info at index deviceIndex, the function returns FALSE
        // when there is no device at the given index.
        for( deviceIndex=0;
             SetupDiEnumDeviceInfo( devInfo, deviceIndex, &devInfoData );
             deviceIndex++ ) {

            // setting this variable to FALSE will cause all of the if
            // statements to fall through, cutting off processing for this
            // device.
            keepGoing = TRUE;

            // if a device name was specified, and it doesn't match this one,
            // stop. If there is a match (or no name was specified), mark that
            // there was a match.
            if( deviceName != NULL &&
                !DeviceNameMatches( devInfo, &devInfoData, deviceName )
                ) {

                keepGoing = FALSE;

            } else {

                deviceMatch = TRUE;

            }

            // print the device name
            if( keepGoing && listDevices ) {

                PrintDeviceName( devInfo, &devInfoData );

            }

            // print the drivers, if we are not adding or removing one
            if( keepGoing && filterToAdd == NULL && filterToRemove == NULL ) {

                PrintFilters( devInfo, &devInfoData, upperFilter );

            }

            // add the filter, then try to restart the device
            if( keepGoing && filterToAdd != NULL ) {

                if( !AddFilterDriver(devInfo,
                                     &devInfoData,
                                     filterToAdd,
                                     upperFilter)) {

                    printf("Unable to add filter!\n");

                } else {

                    if( !RestartDevice( devInfo, &devInfoData) ) {
                        needReboot = TRUE;
                    }

                }
            }

            // remove the filter, then try to restart the device
            if( keepGoing && filterToRemove != NULL ) {

                if( !RemoveFilterDriver(devInfo,
                                        &devInfoData,
                                        filterToRemove,
                                        upperFilter)) {

                    printf("Unable to remove filter!\n");

                } else {

                    if( !RestartDevice( devInfo, &devInfoData) ) {
                        needReboot = TRUE;
                    }

                }

            }

            if( listDevices )
            {
                printf("\n");
            }

            // end of main processing loop
        }

        // clean up the device list
        if( devInfo != INVALID_HANDLE_VALUE ) {

            if( !SetupDiDestroyDeviceInfoList( devInfo ) ) {
                printf("unable to delete device info list! error: %u\n",
                       GetLastError());
            }

        }

    } // loop for each GUID index

    if( !deviceMatch ) {

        printf("No devices matched that name\n");

    } else {

        if( needReboot ) {

            printf("One or more devices could not be restarted. The machine "
                   "must be restarted\n"
                   "in order for settings to take effect\n");

        } else {

            printf("Everything has completed normally.\n");
            return (2);

        }

    }

    return (0);
}


/*
 * add the given filter driver to the list of upper filter drivers for the
 * device.
 *
 * After the call, the device must be restarted in order for the new setting to
 * take effect. This can be accomplished with a call to RestartDevice(), or by
 * rebooting the machine.
 *
 * returns TRUE if successful, FALSE otherwise
 *
 * note: The filter is prepended to the list of drivers, which will put it at
 * the bottom of the filter driver stack
 *
 * parameters:
 *   DeviceInfoSet  - The device information set which contains DeviceInfoData
 *   DeviceInfoData - Information needed to deal with the given device
 *   Filter         - the filter to add
 */
BOOLEAN
AddFilterDriver(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    _In_ IN LPTSTR Filter,
    IN BOOLEAN UpperFilter
    )
{
    size_t length = 0; // character length
    size_t size   = 0; // buffer size
    LPTSTR buffer = GetFilters( DeviceInfoSet, DeviceInfoData, UpperFilter );

    ASSERT(DeviceInfoData != NULL);
    ASSERT(Filter != NULL);

    if( buffer == NULL )
    {
        // if there is no such value in the registry, then there are no upper
        // filter drivers loaded, and we can just put one there

        // make room for the string, string null terminator, and multisz null
        // terminator
        length = _tcslen(Filter)+1;
        size   = (length+1)*sizeof(_TCHAR);
        buffer = malloc( size );
        if( buffer == NULL )
        {
            printf("in AddUpperFilterDriver(): unable to allocate memory!\n");
            return (FALSE);
        }
        memset(buffer, 0, size);

        // copy the string into the new buffer

        memcpy(buffer, Filter, length*sizeof(_TCHAR));

    }
    else
    {
        LPTSTR buffer2;
        // remove all instances of filter from driver list
        MultiSzSearchAndDeleteCaseInsensitive( Filter, buffer, &length );

        // allocate a buffer large enough to add the new filter
        // MultiSzLength already includes length of terminating NULL

        // determing the new length of the string
        length = MultiSzLength(buffer) + _tcslen(Filter) + 1;
        size   = length*sizeof(_TCHAR);

        buffer2 = malloc( size );
        if (buffer2 == NULL) {
            printf("Out of memory adding filter\n");
            return (0);
        }
        memset(buffer2, 0, size);

        // swap the buffers out
        memcpy(buffer2, buffer, MultiSzLength(buffer)*sizeof(_TCHAR));
        free(buffer);
        buffer = buffer2;

        // add the driver to the driver list
        PrependSzToMultiSz(Filter, &buffer);

    }

    // set the new list of filters in place
    if( !SetupDiSetDeviceRegistryProperty( DeviceInfoSet,
                                           DeviceInfoData,
                                           (UpperFilter ? SPDRP_UPPERFILTERS : SPDRP_LOWERFILTERS),
                                           (PBYTE)buffer,
                                           (DWORD)(MultiSzLength(buffer)*sizeof(_TCHAR)) )
        )
    {
        printf("in AddUpperFilterDriver(): "
               "couldn't set registry value! error: %u\n", GetLastError());
        free( buffer );
        return (FALSE);
    }

    // no need for buffer anymore
    free( buffer );

    return (TRUE);
}


/*
 * remove all instances of the given filter driver from the list of upper
 * filter drivers for the device.
 *
 * After the call, the device must be restarted in order for the new setting to
 * take effect. This can be accomplished with a call to RestartDevice(), or by
 * rebooting the machine.
 *
 * returns TRUE if successful, FALSE otherwise
 *
 * parameters:
 *   DeviceInfoSet  - The device information set which contains DeviceInfoData
 *   DeviceInfoData - Information needed to deal with the given device
 *   Filter - the filter to remove
 */
BOOLEAN
RemoveFilterDriver(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    _In_ IN LPTSTR Filter,
    IN BOOLEAN UpperFilter
    )
{
    size_t length  = 0;
    size_t size    = 0;
    LPTSTR buffer  = GetFilters( DeviceInfoSet, DeviceInfoData, UpperFilter );
    BOOL   success = FALSE;

    ASSERT(DeviceInfoData != NULL);
    ASSERT(Filter != NULL);

    if( buffer == NULL )
    {
        // if there is no such value in the registry, then there are no upper
        // filter drivers loaded, and we are done
        return (TRUE);
    }
    else
    {
        // remove all instances of filter from driver list
        MultiSzSearchAndDeleteCaseInsensitive( Filter, buffer, &length );
    }

    length = MultiSzLength(buffer);

    ASSERT ( length > 0 );

    if( length == 1 )
    {
        // if the length of the list is 1, the return value from
        // MultiSzLength() was just accounting for the trailing '\0', so we can
        // delete the registry key, by setting it to NULL.
        success = SetupDiSetDeviceRegistryProperty( DeviceInfoSet,
                                                    DeviceInfoData,
                                                    (UpperFilter ? SPDRP_UPPERFILTERS : SPDRP_LOWERFILTERS),
                                                    NULL,
                                                    0 );
    }
    else
    {
        // set the new list of drivers into the registry
        size = length*sizeof(_TCHAR);
        success = SetupDiSetDeviceRegistryProperty( DeviceInfoSet,
                                                    DeviceInfoData,
                                                    (UpperFilter ? SPDRP_UPPERFILTERS : SPDRP_LOWERFILTERS),
                                                    (PBYTE)buffer,
                                                    (DWORD)size );
    }

    // no need for buffer anymore
    free( buffer );

    if( !success )
    {
        printf("in RemoveUpperFilterDriver(): "
               "couldn't set registry value! error: %u\n", GetLastError());
        return (FALSE);
    }

    return (TRUE);
}

/*
 * print the list of upper filters for the given device
 *
 * parameters:
 *   DeviceInfoSet  - The device information set which contains DeviceInfoData
 *   DeviceInfoData - Information needed to deal with the given device
 */
void
PrintFilters(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN BOOLEAN UpperFilters
    )
{
    // get the list of filters
    LPTSTR buffer = GetFilters( DeviceInfoSet, DeviceInfoData, UpperFilters );
    LPTSTR filterName;
    size_t filterPosition;

    if( buffer == NULL )
    {
        // if there is no such value in the registry, then there are no upper
        // filter drivers loaded
        printf("There are no upper filter drivers loaded for this device.\n");
    }
    else
    {
        // go through the multisz and print out each driver
        filterPosition=0;
        filterName = buffer;
        while( *filterName != _T('\0') )
        {
            _tprintf(_T("%u: %s\n"), filterPosition, filterName);
            filterName += _tcslen(filterName)+1;
            filterPosition++;
        }

        // no need for buffer anymore
        free( buffer );
    }

    return;
}

/*
 * print the device name
 *
 * parameters:
 *   DeviceInfoSet  - The device information set which contains DeviceInfoData
 *   DeviceInfoData - Information needed to deal with the given device
 */
void PrintDeviceName(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData
    )
{
    DWORD  regDataType;
    LPTSTR deviceName =
        (LPTSTR) GetDeviceRegistryProperty( DeviceInfoSet,
                                            DeviceInfoData,
                                            SPDRP_PHYSICAL_DEVICE_OBJECT_NAME,
                                            &regDataType );

    if( deviceName != NULL )
    {
        // just to make sure we are getting the expected type of buffer
        if( regDataType != REG_SZ )
        {
            printf("in PrintDeviceName(): registry key is not an SZ!\n");
        }
        else
        {
            // if the device name starts with \Device, cut that off (all
            // devices will start with it, so it is redundant)

            if( _tcsncmp(deviceName, _T("\\Device"), 7) == 0 )
            {
                memmove(deviceName,
                        deviceName+7,
                        (_tcslen(deviceName)-6)*sizeof(_TCHAR) );
            }

            _tprintf(_T("%s\n"), deviceName);
        }
        free( deviceName );
    }
    else
    {
        printf("in PrintDeviceName(): registry key is NULL! error: %u\n",
               GetLastError());
    }

    return;
}

/*
 * Returns a buffer containing the list of upper filters for the device. (NULL
 * is returned if there is no buffer, or an error occurs)
 * The buffer must be freed by the caller.
 *
 * parameters:
 *   DeviceInfoSet  - The device information set which contains DeviceInfoData
 *   DeviceInfoData - Information needed to deal with the given device
 */
LPTSTR
GetFilters(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN BOOLEAN UpperFilters
    )
{
    DWORD  regDataType;
    LPTSTR buffer = (LPTSTR) GetDeviceRegistryProperty( DeviceInfoSet,
                                                        DeviceInfoData,
                                                        (UpperFilters ? SPDRP_UPPERFILTERS : SPDRP_LOWERFILTERS),
                                                        &regDataType );

    // just to make sure we are getting the expected type of buffer
    if( buffer != NULL && regDataType != REG_MULTI_SZ )
    {
        printf("in GetUpperFilters(): "
               "registry key is not a MULTI_SZ!\n");
        free( buffer );
        return (NULL);
    }

    return (buffer);
}

/*
 * return true if DeviceName matches the name of the device specified by
 * DeviceInfoData
 *
 * parameters:
 *   DeviceInfoSet  - The device information set which contains DeviceInfoData
 *   DeviceInfoData - Information needed to deal with the given device
 *   DeviceName     - the name to try to match
 */
BOOLEAN
DeviceNameMatches(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    _In_ IN LPTSTR DeviceName
    )
{
    BOOLEAN matching = FALSE;
    DWORD   regDataType;

    // get the device name
    LPTSTR  deviceName =
        (LPTSTR) GetDeviceRegistryProperty( DeviceInfoSet,
                                            DeviceInfoData,
                                            SPDRP_PHYSICAL_DEVICE_OBJECT_NAME,
                                            &regDataType );

    if( deviceName != NULL )
    {
        // just to make sure we are getting the expected type of buffer
        if( regDataType != REG_SZ )
        {
            printf("in DeviceNameMatches(): registry key is not an SZ!\n");
            matching = FALSE;
        }
        else
        {
            // if the device name starts with \Device, cut that off (all
            // devices will start with it, so it is redundant)

            if( _tcsncmp(deviceName, _T("\\Device"), 7) == 0 )
            {
                memmove(deviceName,
                        deviceName+7,
                        (_tcslen(deviceName)-6)*sizeof(_TCHAR) );
            }

            // do the strings match?
            matching = (_tcscmp(deviceName, DeviceName) == 0) ? TRUE : FALSE;
        }
        free( deviceName );
    }
    else
    {
        printf("in DeviceNameMatches(): registry key is NULL!\n");
        matching = FALSE;
    }

    return (matching);
}

/*
 * A wrapper around SetupDiGetDeviceRegistryProperty, so that I don't have to
 * deal with memory allocation anywhere else
 *
 * parameters:
 *   DeviceInfoSet  - The device information set which contains DeviceInfoData
 *   DeviceInfoData - Information needed to deal with the given device
 *   Property       - which property to get (SPDRP_XXX)
 *   PropertyRegDataType - the type of registry property
 */
PBYTE
GetDeviceRegistryProperty(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN DWORD Property,
    OUT PDWORD PropertyRegDataType
    )
{
    DWORD length = 0;
    PBYTE buffer = NULL;

    // get the required length of the buffer
    if( SetupDiGetDeviceRegistryProperty( DeviceInfoSet,
                                          DeviceInfoData,
                                          Property,
                                          NULL,   // registry data type
                                          NULL,   // buffer
                                          0,      // buffer size
                                          &length // required size
        ) )
    {
        // we should not be successful at this point, so this call succeeding
        // is an error condition
        printf("in GetDeviceRegistryProperty(): "
               "call SetupDiGetDeviceRegistryProperty did not fail? (%x)\n",
               GetLastError());
        return (NULL);
    }

    if( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
    {
        // this means there are no upper filter drivers loaded, so we can just
        // return.
        return (NULL);
    }

    // since we don't have a buffer yet, it is "insufficient"; we allocate
    // one and try again.
    buffer = malloc( length );
    if( buffer == NULL )
    {
        printf("in GetDeviceRegistryProperty(): "
               "unable to allocate memory!\n");
        return (NULL);
    }
    if( !SetupDiGetDeviceRegistryProperty( DeviceInfoSet,
                                           DeviceInfoData,
                                           Property,
                                           PropertyRegDataType,
                                           buffer,
                                           length,
                                           NULL // required size
        ) )
    {
        printf("in GetDeviceRegistryProperty(): "
               "couldn't get registry property! error: %u\n",
               GetLastError());
        free( buffer );
        return (NULL);
    }

    // ok, we are finally done, and can return the buffer
    return (buffer);
}


/*
 * restarts the given device
 *
 * call CM_Query_And_Remove_Subtree (to unload the driver)
 * call CM_Reenumerate_DevNode on the _parent_ (to reload the driver)
 *
 * parameters:
 *   DeviceInfoSet  - The device information set which contains DeviceInfoData
 *   DeviceInfoData - Information needed to deal with the given device
 */
BOOLEAN
RestartDevice(
    IN HDEVINFO DeviceInfoSet,
    IN OUT PSP_DEVINFO_DATA DeviceInfoData
    )
{
    SP_PROPCHANGE_PARAMS params;
    SP_DEVINSTALL_PARAMS installParams;

    // for future compatibility; this will zero out the entire struct, rather
    // than just the fields which exist now
    memset(&params, 0, sizeof(SP_PROPCHANGE_PARAMS));

    // initialize the SP_CLASSINSTALL_HEADER struct at the beginning of the
    // SP_PROPCHANGE_PARAMS struct, so that SetupDiSetClassInstallParams will
    // work
    params.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    params.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;

    // initialize SP_PROPCHANGE_PARAMS such that the device will be stopped.
    params.StateChange = DICS_STOP;
    params.Scope       = DICS_FLAG_CONFIGSPECIFIC;
    params.HwProfile   = 0; // current profile

    // prepare for the call to SetupDiCallClassInstaller (to stop the device)
    if( !SetupDiSetClassInstallParams( DeviceInfoSet,
                                       DeviceInfoData,
                                       (PSP_CLASSINSTALL_HEADER) &params,
                                       sizeof(SP_PROPCHANGE_PARAMS)
        ) )
    {
        printf("in RestartDevice(): couldn't set the install parameters!");
        printf(" error: %u\n", GetLastError());
        return (FALSE);
    }

    // stop the device
    if( !SetupDiCallClassInstaller( DIF_PROPERTYCHANGE,
                                    DeviceInfoSet,
                                    DeviceInfoData )
        )
    {
        printf("in RestartDevice(): call to class installer (STOP) failed!");
        printf(" error: %u\n", GetLastError() );
        return (FALSE);
    }

    // restarting the device
    params.StateChange = DICS_START;

    // prepare for the call to SetupDiCallClassInstaller (to stop the device)
    if( !SetupDiSetClassInstallParams( DeviceInfoSet,
                                       DeviceInfoData,
                                       (PSP_CLASSINSTALL_HEADER) &params,
                                       sizeof(SP_PROPCHANGE_PARAMS)
        ) )
    {
        printf("in RestartDevice(): couldn't set the install parameters!");
        printf(" error: %u\n", GetLastError());
        return (FALSE);
    }

    // restart the device
    if( !SetupDiCallClassInstaller( DIF_PROPERTYCHANGE,
                                    DeviceInfoSet,
                                    DeviceInfoData )
        )
    {
        printf("in RestartDevice(): call to class installer (START) failed!");
        printf(" error: %u\n", GetLastError());
        return (FALSE);
    }

    installParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

    // same as above, the call will succeed, but we still need to check status
    if( !SetupDiGetDeviceInstallParams( DeviceInfoSet,
                                        DeviceInfoData,
                                        &installParams )
        )
    {
        printf("in RestartDevice(): couldn't get the device install params!");
        printf(" error: %u\n", GetLastError() );
        return (FALSE);
    }

    // to see if the machine needs to be rebooted
    if( installParams.Flags & DI_NEEDREBOOT )
    {
        return (FALSE);
    }

    // if we get this far, then the device has been stopped and restarted
    return (TRUE);
}


/*
 * prepend the given string to a MultiSz
 *
 * returns true if successful, false if not (will only fail in memory
 * allocation)
 *
 * note: This WILL allocate and free memory, so don't keep pointers to the
 * MultiSz passed in.
 *
 * parameters:
 *   SzToPrepend - string to prepend
 *   MultiSz     - pointer to a MultiSz which will be prepended-to
 */
BOOLEAN
PrependSzToMultiSz(
    _In_        LPTSTR  SzToPrepend,
    _Inout_ LPTSTR *MultiSz
    )
{
    size_t szLen;
    size_t multiSzLen;
    LPTSTR newMultiSz = NULL;

    ASSERT(SzToPrepend != NULL);
    ASSERT(MultiSz != NULL);

    if (SzToPrepend == NULL || MultiSz == NULL) {
        return (FALSE);
    }

    // get the size, in bytes, of the two buffers
    szLen = (_tcslen(SzToPrepend)+1)*sizeof(_TCHAR);
    multiSzLen = MultiSzLength(*MultiSz)*sizeof(_TCHAR);
    newMultiSz = (LPTSTR)malloc( szLen+multiSzLen );

    if( newMultiSz == NULL )
    {
        return (FALSE);
    }

    // recopy the old MultiSz into proper position into the new buffer.
    // the (char*) cast is necessary, because newMultiSz may be a wchar*, and
    // szLen is in bytes.

    memcpy( ((char*)newMultiSz) + szLen, *MultiSz, multiSzLen );

    // copy in the new string
    StringCbCopy( newMultiSz, szLen, SzToPrepend );

    free( *MultiSz );
    *MultiSz = newMultiSz;

    return (TRUE);
}


/*
 * returns the length (in characters) of the buffer required to hold this
 * MultiSz, INCLUDING the trailing null.
 *
 * example: MultiSzLength("foo\0bar\0") returns 9
 *
 * note: since MultiSz cannot be null, a number >= 1 will always be returned
 *
 * parameters:
 *   MultiSz - the MultiSz to get the length of
 */
size_t
MultiSzLength(
    _In_ IN LPTSTR MultiSz
    )
{
    size_t len = 0;
    size_t totalLen = 0;

    ASSERT( MultiSz != NULL );

    // search for trailing null character
    while( *MultiSz != _T('\0') )
    {
        len = _tcslen(MultiSz)+1;
        MultiSz += len;
        totalLen += len;
    }

    // add one for the trailing null character
    return (totalLen+1);
}


/*
 * Deletes all instances of a string from within a multi-sz.
 *
 * parameters:
 *   FindThis        - the string to find and remove
 *   FindWithin      - the string having the instances removed
 *   NewStringLength - the new string length
 */
size_t
MultiSzSearchAndDeleteCaseInsensitive(
    _In_ IN  LPTSTR FindThis,
    _In_ IN  LPTSTR FindWithin,
    OUT size_t *NewLength
    )
{
    LPTSTR search;
    size_t currentOffset;
    DWORD  instancesDeleted;
    size_t searchLen;

    ASSERT(FindThis != NULL);
    ASSERT(FindWithin != NULL);
    ASSERT(NewLength != NULL);

    currentOffset = 0;
    instancesDeleted = 0;
    search = FindWithin;

    *NewLength = MultiSzLength(FindWithin);

    // loop while the multisz null terminator is not found
    while ( *search != _T('\0') )
    {
        // length of string + null char; used in more than a couple places
        searchLen = _tcslen(search) + 1;

        // if this string matches the current one in the multisz...
        if( _tcsicmp(search, FindThis) == 0 )
        {
            // they match, shift the contents of the multisz, to overwrite the
            // string (and terminating null), and update the length
            instancesDeleted++;
            *NewLength -= searchLen;
            memmove( search,
                     search + searchLen,
                     (*NewLength - currentOffset) * sizeof(TCHAR) );
        }
        else
        {
            // they don't mactch, so move pointers, increment counters
            currentOffset += searchLen;
            search        += searchLen;
        }
    }

    return (instancesDeleted);
}


/*
 * print usage
 */
void PrintUsage()
{
    printf("usage:\n\n"
           "addfilter"
           " [/listdevices]"
           " [/device device_name]"
           " [/add filter]"
           " [/remove filter]"
           " [/lower]"
           "\n\n");
    printf("If device_name is not supplied, settings will apply "
           "to all devices.\n");
    printf("If there is no /add or /remove argument, a list of currently"
           " installed drivers\n"
           "will be printed.\n");
    printf("The default is to process upper filters.  Use the /lower switch"
           " to process lower filters instead.\n");
    return;
}
