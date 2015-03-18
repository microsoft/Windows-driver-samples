/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    util.c

Abstract:

    Utility routines used by the TOASTVA sample.

--*/

#include "precomp.h"
#pragma hdrstop

/*++

    Here's the description for the CMP_WaitNoPendingInstallEvents API used to
    suppress execution of this app (e.g., via autorun upon CD insertion) while
    device installation is underway...
    
    DWORD
    CMP_WaitNoPendingInstallEvents(
        _In_ DWORD dwTimeout
        );
    
    Routine Description:

        This routine waits until there are no pending device install events.
        If a timeout value is specified then it will return either when no
        install events are pending or when the timeout period has expired,
        whichever comes first. This routine is intended to be called after
        user-logon, only.

        NOTE: New install events can occur at anytime, this routine just
        indicates that there are no install events at this moment.

    Parameters:

        dwTimeout - Specifies the time-out interval, in milliseconds. The 
            function returns if the interval elapses, even if there are still 
            pending install events. If dwTimeout is zero, the function just 
            tests whether there are pending install events and returns 
            immediately. If dwTimeout is INFINITE, the function's time-out
            interval never elapses.

    Return Value:

        If the function succeeds, the return value indicates the event that 
        caused the function to return. If the function fails, the return value 
        is WAIT_FAILED. To get extended error information, call GetLastError.
        The return value on success is one of the following values:

        WAIT_ABANDONED  The specified object is a mutex object that was not
                        released by the thread that owned the mutex object 
                        before the owning thread terminated. Ownership of the 
                        mutex object is granted to the calling thread, and the 
                        mutex is set to nonsignaled.
        WAIT_OBJECT_0   The state of the specified object is signaled.
        WAIT_TIMEOUT    The time-out interval elapsed, and the object's state is
                        nonsignaled.

--*/

typedef DWORD (WINAPI *CMP_WAITNOPENDINGINSTALLEVENTS_PROC)(
    _In_ DWORD dwTimeout
    );


BOOL
IsDeviceInstallInProgress(VOID)

/*++

Routine Description:

    This routine dynamically retrieves the entrypoint to a Windows 2000 (and
    later) Configuration Manager (CM) API that can be used to detect whether 
    there are presently any device installations in-progress.  If this API is 
    not available (it may be obsoleted on future releases of the OS), then the 
    API assumes there are no device installations in progress.  This is OK, 
    because future versions of the OS will suppress auto-run when "Found New 
    Hardware" wizard is up, thus eliminating the need for apps to do their own 
    checking.

Arguments:

    none

Return Value:

    If there is presently a device installation in progress, the return value
    is TRUE.
    
    If there is not presently a device installation in progress, or we were
    unsuccessful in retrieving the necessary Configuration Manager API, then
    the return value is FALSE.

--*/

{
    HMODULE hModule;
    CMP_WAITNOPENDINGINSTALLEVENTS_PROC pCMP_WaitNoPendingInstallEvents;
    
    hModule = GetModuleHandle(L"setupapi.dll");
    
    if(!hModule) {
        //
        // Should never happen since we're linked to setupapi, but...
        //
        return FALSE;
    }
    
    pCMP_WaitNoPendingInstallEvents = 
        (CMP_WAITNOPENDINGINSTALLEVENTS_PROC)GetProcAddress(
                                               hModule,
                                               "CMP_WaitNoPendingInstallEvents"
                                               );
    if(!pCMP_WaitNoPendingInstallEvents) {
        //
        // We're running on a release of the OS that doesn't supply this API.
        // Trust the OS to suppress autorun when appropriate.
        //
        return FALSE;
    }

    return (pCMP_WaitNoPendingInstallEvents(0) == WAIT_TIMEOUT);
}


VOID
MarkDevicesAsNeedReinstall(
    _In_ HDEVINFO DeviceInfoSet
    )

/*++

Routine Description:

    This routine enumerates every device information element in the specified
    list and sets the CONFIGFLAG_REINSTALL registry flag for each one.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set whose 
        members are to be marked as need-reinstall.

Return Value:

    none

--*/

{
    SP_DEVINFO_DATA DeviceInfoData;
    DWORD i, ConfigFlags;
    
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    
    for(i = 0;
        SetupDiEnumDeviceInfo(DeviceInfoSet, i, &DeviceInfoData);
        i++)
    {
        ConfigFlags = GetDeviceConfigFlags(DeviceInfoSet, &DeviceInfoData);
        ConfigFlags |= CONFIGFLAG_REINSTALL;
        SetDeviceConfigFlags(DeviceInfoSet, &DeviceInfoData, ConfigFlags);
    }
}


DWORD
GetDeviceConfigFlags(
    _In_ HDEVINFO         DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData
    )
    
/*++

Routine Description:

    This routine retrieves the ConfigFlags registry property for the specified
    device info element, or zero if the property cannot be retrieved (e.g.,
    because ConfigFlags haven't yet been set by Found New Hardware process).

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        the device of interest.
        
    DeviceInfoData - Supplies context of a device info element for which
        ConfigFlags is to be retrieved.

Return Value:

    If device's REG_DWORD ConfigFlags property can be retrieved, it is returned.
    Otherwise, zero is returned.

--*/

{
    DWORD ConfigFlags, RegDataType;
    
    if(!SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                         DeviceInfoData,
                                         SPDRP_CONFIGFLAGS,
                                         &RegDataType,
                                         (PBYTE)&ConfigFlags,
                                         sizeof(ConfigFlags),
                                         NULL)
       || (RegDataType != REG_DWORD))
    {
        //
        // It's possible that this property isn't there, although we should
        // never enounter other problems like wrong datatype or data length
        // longer than sizeof(DWORD).  In any event, just return zero.
        //
        ConfigFlags = 0;
    }
    
    return ConfigFlags;
}


VOID
SetDeviceConfigFlags(
    _In_ HDEVINFO         DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData, 
    _In_ DWORD            ConfigFlags
    )

/*++

Routine Description:

    This routine sets a device's ConfigFlags property to the specified value.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        the device of interest.
        
    DeviceInfoData - Supplies context of a device info element for which
        ConfigFlags is to be set.
        
    ConfigFlags - Specifies the value to be stored to the device's ConfigFlags
        property.

Return Value:

    none

--*/

{
    SetupDiSetDeviceRegistryProperty(DeviceInfoSet,
                                     DeviceInfoData,
                                     SPDRP_CONFIGFLAGS,
                                     (PBYTE)&ConfigFlags,
                                     sizeof(ConfigFlags)
                                    );
}


HDEVINFO
GetNonPresentDevices(
    _In_  LPCWSTR   Enumerator OPTIONAL,
    _In_  LPCWSTR   HardwareID
    )
    
/*++

Routine Description:

    This routine retrieves any non-present devices matching the specified 
    criteria, and returns them in a device information set.

Arguments:

    Enumerator - Optionally, supplies the name of the Enumerator under which 
        this device may be found.  If the device may show up under more than 
        one enumerator, the routine can be called with Enumerator specified as
        NULL, in which case all device instances in the registry are examined.
        
    HardwareID - Supplies the hardware ID to be searched for.  This will be
        compared against each of the hardware IDs for all device instances in
        the system (potentially filtered based on Enumerator), present or not.

Return Value:

    If any non-present devices are discovered, this routine returns a device
    information set containing those devices.  This set must be freed via
    SetupDiDestroyDeviceInfoList by the caller.
    
    If no such devices are encountered (or if an error occurs), the return
    value is INVALID_HANDLE_VALUE.  GetLastError will indicate the cause of
    failure.

--*/

{
    HDEVINFO AllDevs, ExistingNonPresentDevices;
    DWORD i, Err;
    SP_DEVINFO_DATA DeviceInfoData;
    LPWSTR HwIdBuffer, CurId;
    DWORD HwIdBufferLen, RegDataType, RequiredSize;
    BOOL bRet;
    ULONG Status, Problem;
    TCHAR DeviceInstanceId[MAX_DEVNODE_ID_LEN];

    ExistingNonPresentDevices = INVALID_HANDLE_VALUE;
    
    AllDevs = SetupDiGetClassDevs(NULL,
                                  Enumerator,
                                  NULL,
                                  DIGCF_ALLCLASSES
                                 );
                                 
    if(AllDevs == INVALID_HANDLE_VALUE) {
        //
        // last error has already been set during the above call.
        //
        return INVALID_HANDLE_VALUE;
    }
                                  
    //
    // Iterate through each device we found, comparing its hardware ID(s)
    // against the one we were passed in.
    //
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    
    HwIdBuffer = NULL;
    HwIdBufferLen = 0;
    Err = NO_ERROR;
    bRet = FALSE;
    
    i = 0;
        
    while(SetupDiEnumDeviceInfo(AllDevs, i, &DeviceInfoData)) {
        //
        // Retrieve the HardwareID property for this device info element
        //
        if(!SetupDiGetDeviceRegistryProperty(AllDevs,
                                             &DeviceInfoData,
                                             SPDRP_HARDWAREID,
                                             &RegDataType,
                                             (PBYTE)HwIdBuffer,
                                             HwIdBufferLen,
                                             &RequiredSize)) {
            //
            // If the failure was due to buffer-too-small, we can resize and
            // try again.
            //
            if(GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            
                if(HwIdBuffer) {
                    GlobalFree(HwIdBuffer);
                }
                
                HwIdBuffer = GlobalAlloc(0, RequiredSize);
                if(HwIdBuffer) {
                    HwIdBufferLen = RequiredSize;
                    //
                    // try again
                    //
                    continue;
                } else {
                    //
                    // We failed to allocate the buffer we needed.  This is
                    // considered a critical failure that should cause us to
                    // bail.
                    //
                    Err = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
                
            } else {
                //
                // We failed to retrieve the property for some other reason.
                // Skip this device and move on to the next.
                //
                i++;
                continue;
            }
        }

        if((RegDataType != REG_MULTI_SZ) || (RequiredSize < sizeof(TCHAR))) {
            //
            // Data is invalid--this should never happen, but we'll skip the
            // device in this case...
            //
            i++;
            continue;
        }
        
        //
        // If we get to here, then we successfully retrieved the multi-sz
        // hardware id list for this device.  Compare each of those IDs with
        // the caller-supplied one.
        //
        for(CurId = HwIdBuffer; CurId && *CurId; CurId += (lstrlen(CurId) + 1)) {
        
            if(!lstrcmpi(CurId, HardwareID)) {
                //
                // We found a match!
                //
                bRet = TRUE;
                
                //
                // If the device isn't currently present (as indicated by
                // failure to retrieve its status), then add it to the list of
                // such devices to be returned to the caller.
                //
                if(CR_SUCCESS != CM_Get_DevNode_Status(&Status,
                                                       &Problem,
                                                       (DEVNODE)DeviceInfoData.DevInst,
                                                       0))
                {
                    if(ExistingNonPresentDevices == INVALID_HANDLE_VALUE) {
                        //
                        // This is the first non-present device we've 
                        // encountered--we need to create the HDEVINFO set.
                        //
                        ExistingNonPresentDevices = 
                            SetupDiCreateDeviceInfoList(NULL, NULL);
                        
                        if(ExistingNonPresentDevices == INVALID_HANDLE_VALUE) {
                            //
                            // Failure to create this set is a critical error!
                            //
                            Err = GetLastError();
                            bRet = FALSE;
                            break;
                        }
                    }
                        
                    //
                    // We need to get the device instance's name so we can
                    // open it up into our "non-present devices" list
                    //
                    if(!SetupDiGetDeviceInstanceId(AllDevs,
                                                   &DeviceInfoData,
                                                   DeviceInstanceId,
                                                   sizeof(DeviceInstanceId) / sizeof(TCHAR),
                                                   NULL)) {
                        //
                        // Should never fail, but considered critical if it
                        // does...
                        //
                        Err = GetLastError();
                        bRet = FALSE;
                        break;
                    }

                    //
                    // Now open up the non-present device into our list.
                    //
                    if(!SetupDiOpenDeviceInfo(ExistingNonPresentDevices,
                                              DeviceInstanceId,
                                              NULL,
                                              0,
                                              NULL)) {
                        //
                        // This failure is also considered critical!
                        //                          
                        Err = GetLastError();
                        bRet = FALSE;
                    }

                    break;
                }
            }
        }

        if(Err != NO_ERROR) {
            //
            // Critical error encountered--bail!
            //
            break;
        }

        //
        // Move onto the next device instance
        //
        i++;
    }
    
    if(HwIdBuffer) {
        GlobalFree(HwIdBuffer);
    }

    //
    // We can now destroy our temporary list of all devices under consideration
    //
    SetupDiDestroyDeviceInfoList(AllDevs);

    if((Err != NO_ERROR) && 
       (ExistingNonPresentDevices != INVALID_HANDLE_VALUE)) {
        //
        // We encountered a critical error, so we need to destroy the (partial)
        // list of non-present devices we'd built.
        //
        SetupDiDestroyDeviceInfoList(ExistingNonPresentDevices);
        ExistingNonPresentDevices = INVALID_HANDLE_VALUE;
    }

    SetLastError(Err);

    return ExistingNonPresentDevices;
}

