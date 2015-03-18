/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    dump.cpp

Abstract:

    Device Console
    dump information out about a particular device

--*/

#include "devcon.h"

BOOL DumpDeviceWithInfo(_In_ HDEVINFO Devs, _In_ PSP_DEVINFO_DATA DevInfo, _In_opt_ LPCTSTR Info)
/*++

Routine Description:

    Write device instance & info to stdout

Arguments:

    Devs    )_ uniquely identify device
    DevInfo )

Return Value:

    none

--*/
{
    TCHAR devID[MAX_DEVICE_ID_LEN];
    BOOL b = TRUE;
    SP_DEVINFO_LIST_DETAIL_DATA devInfoListDetail;

    devInfoListDetail.cbSize = sizeof(devInfoListDetail);
    if((!SetupDiGetDeviceInfoListDetail(Devs,&devInfoListDetail)) ||
            (CM_Get_Device_ID_Ex(DevInfo->DevInst,devID,MAX_DEVICE_ID_LEN,0,devInfoListDetail.RemoteMachineHandle)!=CR_SUCCESS)) {
        StringCchCopy(devID, ARRAYSIZE(devID), TEXT("?"));
        b = FALSE;
    }

    if(Info) {
        _tprintf(TEXT("%-60s: %s\n"),devID,Info);
    } else {
        _tprintf(TEXT("%s\n"),devID);
    }
    return b;
}

BOOL DumpDevice(_In_ HDEVINFO Devs, _In_ PSP_DEVINFO_DATA DevInfo)
/*++

Routine Description:

    Write device instance & description to stdout

Arguments:

    Devs    )_ uniquely identify device
    DevInfo )

Return Value:

    TRUE if success

--*/
{
    LPTSTR desc;
    BOOL b;

    desc = GetDeviceDescription(Devs,DevInfo);
    b = DumpDeviceWithInfo(Devs,DevInfo,desc);
    if(desc) {
        delete [] desc;
    }
    return b;
}

BOOL DumpDeviceDescr(_In_ HDEVINFO Devs, _In_ PSP_DEVINFO_DATA DevInfo)
/*++

Routine Description:

    Write device description to stdout

Arguments:

    Devs    )_ uniquely identify device
    DevInfo )

Return Value:

    TRUE if success

--*/
{
    LPTSTR desc;

    desc = GetDeviceDescription(Devs,DevInfo);
    if(!desc) {
        return FALSE;
    }
    Padding(1);
    FormatToStream(stdout,MSG_DUMP_DESCRIPTION,desc);
    delete [] desc;
    return TRUE;
}

BOOL DumpDeviceClass(_In_ HDEVINFO Devs, _In_ PSP_DEVINFO_DATA DevInfo)
/*++

Routine Description:

    Write device class information to stdout

Arguments:

    Devs    )_ uniquely identify device
    DevInfo )

Return Value:

    TRUE if success

--*/
{
    LPTSTR cls;
    LPTSTR guid;

    Padding(1);
    cls = GetDeviceStringProperty(Devs,DevInfo,SPDRP_CLASS);
    guid = GetDeviceStringProperty(Devs,DevInfo,SPDRP_CLASSGUID);
    if(!cls && !guid) {
        FormatToStream(stdout,
                        MSG_DUMP_NOSETUPCLASS
                        );
    } else {
        FormatToStream(stdout,
                        MSG_DUMP_SETUPCLASS,
                        guid ? guid : TEXT("{}"),
                        cls ? cls : TEXT("(?)")
                        );
    }

    if(cls) {
        delete [] cls;
    }
    if(guid) {
        delete [] guid;
    }

    return TRUE;
}

BOOL DumpDeviceStatus(_In_ HDEVINFO Devs, _In_ PSP_DEVINFO_DATA DevInfo)
/*++

Routine Description:

    Write device status to stdout

Arguments:

    Devs    )_ uniquely identify device
    DevInfo )

Return Value:

    none

--*/
{
    SP_DEVINFO_LIST_DETAIL_DATA devInfoListDetail;
    ULONG status = 0;
    ULONG problem = 0;
    BOOL hasInfo = FALSE;
    BOOL isPhantom = FALSE;
    CONFIGRET cr = CR_SUCCESS;

    devInfoListDetail.cbSize = sizeof(devInfoListDetail);
    if((!SetupDiGetDeviceInfoListDetail(Devs,&devInfoListDetail)) ||
            ((cr = CM_Get_DevNode_Status_Ex(&status,&problem,DevInfo->DevInst,0,devInfoListDetail.RemoteMachineHandle))!=CR_SUCCESS)) {
        if ((cr == CR_NO_SUCH_DEVINST) || (cr == CR_NO_SUCH_VALUE)) {
            isPhantom = TRUE;
        } else {
            Padding(1);
            FormatToStream(stdout,MSG_DUMP_STATUS_ERROR);
            return FALSE;
        }
    }
    //
    // handle off the status/problem codes
    //
    if (isPhantom) {
        Padding(1);
        FormatToStream(stdout,MSG_DUMP_PHANTOM);
        return TRUE;
    }
    if((status & DN_HAS_PROBLEM) && problem == CM_PROB_DISABLED) {
        hasInfo = TRUE;
        Padding(1);
        FormatToStream(stdout,MSG_DUMP_DISABLED);
        return TRUE;
    }
    if(status & DN_HAS_PROBLEM) {
        hasInfo = TRUE;
        Padding(1);
        FormatToStream(stdout,MSG_DUMP_PROBLEM,problem);
    }
    if(status & DN_PRIVATE_PROBLEM) {
        hasInfo = TRUE;
        Padding(1);
        FormatToStream(stdout,MSG_DUMP_PRIVATE_PROBLEM);
    }
    if(status & DN_STARTED) {
        Padding(1);
        FormatToStream(stdout,MSG_DUMP_STARTED);
    } else if (!hasInfo) {
        Padding(1);
        FormatToStream(stdout,MSG_DUMP_NOTSTARTED);
    }
    return TRUE;
}

BOOL DumpDeviceResourcesOfType(_In_ DEVINST DevInst, _In_ HMACHINE MachineHandle, _In_ LOG_CONF Config, _In_ RESOURCEID ReqResId)
{
    RES_DES prevResDes = (RES_DES)Config;
    RES_DES resDes = 0;
    RESOURCEID resId = ReqResId;
    ULONG dataSize;
    PBYTE resDesData;
    BOOL  retval = FALSE;

    UNREFERENCED_PARAMETER(DevInst);

    while(CM_Get_Next_Res_Des_Ex(&resDes,prevResDes,ReqResId,&resId,0,MachineHandle)==CR_SUCCESS) {
        if(prevResDes != Config) {
            CM_Free_Res_Des_Handle(prevResDes);
        }
        prevResDes = resDes;
        if(CM_Get_Res_Des_Data_Size_Ex(&dataSize,resDes,0,MachineHandle)!=CR_SUCCESS) {
            continue;
        }
        resDesData = new BYTE[dataSize];
        if(!resDesData) {
            continue;
        }
        if(CM_Get_Res_Des_Data_Ex(resDes,resDesData,dataSize,0,MachineHandle)!=CR_SUCCESS) {
            delete [] resDesData;
            continue;
        }
        switch(resId) {
            case ResType_Mem: {

                PMEM_RESOURCE  pMemData = (PMEM_RESOURCE)resDesData;
                if(pMemData->MEM_Header.MD_Alloc_End-pMemData->MEM_Header.MD_Alloc_Base+1) {
                    Padding(2);
                    _tprintf(TEXT("MEM : %08I64x-%08I64x\n"),pMemData->MEM_Header.MD_Alloc_Base,pMemData->MEM_Header.MD_Alloc_End);
                    retval = TRUE;
                }
                break;
            }

            case ResType_IO: {

                PIO_RESOURCE   pIoData = (PIO_RESOURCE)resDesData;
                if(pIoData->IO_Header.IOD_Alloc_End-pIoData->IO_Header.IOD_Alloc_Base+1) {
                    Padding(2);
                    _tprintf(TEXT("IO  : %04I64x-%04I64x\n"),pIoData->IO_Header.IOD_Alloc_Base,pIoData->IO_Header.IOD_Alloc_End);
                    retval = TRUE;
                }
                break;
            }

            case ResType_DMA: {

                PDMA_RESOURCE  pDmaData = (PDMA_RESOURCE)resDesData;
                Padding(2);
                _tprintf(TEXT("DMA : %u\n"),pDmaData->DMA_Header.DD_Alloc_Chan);
                retval = TRUE;
                break;
            }

            case ResType_IRQ: {

                PIRQ_RESOURCE  pIrqData = (PIRQ_RESOURCE)resDesData;

                Padding(2);
                _tprintf(TEXT("IRQ : %u\n"),pIrqData->IRQ_Header.IRQD_Alloc_Num);
                retval = TRUE;
                break;
            }
        }
        delete [] resDesData;
    }
    if(prevResDes != Config) {
        CM_Free_Res_Des_Handle(prevResDes);
    }
    return retval;
}

BOOL DumpDeviceResources(_In_ HDEVINFO Devs, _In_ PSP_DEVINFO_DATA DevInfo)
/*++

Routine Description:

    Dump Resources to stdout

Arguments:

    Devs    )_ uniquely identify device
    DevInfo )

Return Value:

    none

--*/
{
    SP_DEVINFO_LIST_DETAIL_DATA devInfoListDetail;
    ULONG status = 0;
    ULONG problem = 0;
    LOG_CONF config = 0;
    BOOL haveConfig = FALSE;

    //
    // see what state the device is in
    //
    devInfoListDetail.cbSize = sizeof(devInfoListDetail);
    if((!SetupDiGetDeviceInfoListDetail(Devs,&devInfoListDetail)) ||
            (CM_Get_DevNode_Status_Ex(&status,&problem,DevInfo->DevInst,0,devInfoListDetail.RemoteMachineHandle)!=CR_SUCCESS)) {
        return FALSE;
    }

    //
    // see if the device is running and what resources it might be using
    //
    if(!(status & DN_HAS_PROBLEM)) {
        //
        // If this device is running, does this devinst have a ALLOC log config?
        //
        if (CM_Get_First_Log_Conf_Ex(&config,
                                     DevInfo->DevInst,
                                     ALLOC_LOG_CONF,
                                     devInfoListDetail.RemoteMachineHandle) == CR_SUCCESS) {
            haveConfig = TRUE;
        }
    }
    if(!haveConfig) {
        //
        // If no config so far, does it have a FORCED log config?
        // (note that technically these resources might be used by another device
        // but is useful info to show)
        //
        if (CM_Get_First_Log_Conf_Ex(&config,
                                     DevInfo->DevInst,
                                     FORCED_LOG_CONF,
                                     devInfoListDetail.RemoteMachineHandle) == CR_SUCCESS) {
            haveConfig = TRUE;
        }
    }

    if(!haveConfig) {
        //
        // if there's a hardware-disabled problem, boot-config isn't valid
        // otherwise use this if we don't have anything else
        //
        if(!(status & DN_HAS_PROBLEM) || (problem != CM_PROB_HARDWARE_DISABLED)) {
            //
            // Does it have a BOOT log config?
            //
            if (CM_Get_First_Log_Conf_Ex(&config,
                                         DevInfo->DevInst,
                                         BOOT_LOG_CONF,
                                         devInfoListDetail.RemoteMachineHandle) == CR_SUCCESS) {
                haveConfig = TRUE;
            }
        }
    }

    if(!haveConfig) {
        //
        // if we don't have any configuration, display an apropriate message
        //
        Padding(1);
        FormatToStream(stdout,(status & DN_STARTED) ? MSG_DUMP_NO_RESOURCES : MSG_DUMP_NO_RESERVED_RESOURCES );
        return TRUE;
    }
    Padding(1);
    FormatToStream(stdout,(status & DN_STARTED) ? MSG_DUMP_RESOURCES : MSG_DUMP_RESERVED_RESOURCES );

    //
    // dump resources
    //
    DumpDeviceResourcesOfType(DevInfo->DevInst,devInfoListDetail.RemoteMachineHandle,config,ResType_All);

    //
    // release handle
    //
    CM_Free_Log_Conf_Handle(config);

    return TRUE;
}


UINT DumpDeviceDriversCallback(_In_ PVOID Context, _In_ UINT Notification, _In_ UINT_PTR Param1, _In_ UINT_PTR Param2)
/*++

Routine Description:

    if Context provided, Simply count
    otherwise dump files indented 2

Arguments:

    Context      - DWORD Count
    Notification - SPFILENOTIFY_QUEUESCAN
    Param1       - scan

Return Value:

    none

--*/
{
    LPDWORD count = (LPDWORD)Context;
    LPTSTR file = (LPTSTR)Param1;

    UNREFERENCED_PARAMETER(Notification);
    UNREFERENCED_PARAMETER(Param2);

    if(count) {
        count[0]++;
    } else {
        Padding(2);
        _tprintf(TEXT("%s\n"),file);
    }

    return NO_ERROR;
}

BOOL FindCurrentDriver(_In_ HDEVINFO Devs, _In_ PSP_DEVINFO_DATA DevInfo, _In_ PSP_DRVINFO_DATA DriverInfoData)
/*++

Routine Description:

    Find the driver that is associated with the current device
    We can do this either the quick way (available in WinXP)
    or the long way that works in Win2k.

Arguments:

    Devs    )_ uniquely identify device
    DevInfo )

Return Value:

    TRUE if we managed to determine and select current driver

--*/
{
    SP_DEVINSTALL_PARAMS deviceInstallParams;
    WCHAR SectionName[LINE_LEN];
    WCHAR DrvDescription[LINE_LEN];
    WCHAR MfgName[LINE_LEN];
    WCHAR ProviderName[LINE_LEN];
    HKEY hKey = NULL;
    DWORD RegDataLength;
    DWORD RegDataType;
    DWORD c;
    BOOL match = FALSE;
    long regerr;

    ZeroMemory(&deviceInstallParams, sizeof(deviceInstallParams));
    deviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

    if(!SetupDiGetDeviceInstallParams(Devs, DevInfo, &deviceInstallParams)) {
        return FALSE;
    }

#ifdef DI_FLAGSEX_INSTALLEDDRIVER
    //
    // Set the flags that tell SetupDiBuildDriverInfoList to just put the
    // currently installed driver node in the list, and that it should allow
    // excluded drivers. This flag introduced in WinXP.
    //
    deviceInstallParams.FlagsEx |= (DI_FLAGSEX_INSTALLEDDRIVER | DI_FLAGSEX_ALLOWEXCLUDEDDRVS);

    if(SetupDiSetDeviceInstallParams(Devs, DevInfo, &deviceInstallParams)) {
        //
        // we were able to specify this flag, so proceed the easy way
        // we should get a list of no more than 1 driver
        //
        if(!SetupDiBuildDriverInfoList(Devs, DevInfo, SPDIT_CLASSDRIVER)) {
            return FALSE;
        }
        if (!SetupDiEnumDriverInfo(Devs, DevInfo, SPDIT_CLASSDRIVER,
                                   0, DriverInfoData)) {
            return FALSE;
        }
        //
        // we've selected the current driver
        //
        return TRUE;
    }
    deviceInstallParams.FlagsEx &= ~(DI_FLAGSEX_INSTALLEDDRIVER | DI_FLAGSEX_ALLOWEXCLUDEDDRVS);
#endif
    //
    // The following method works in Win2k, but it's slow and painful.
    //
    // First, get driver key - if it doesn't exist, no driver
    //
    hKey = SetupDiOpenDevRegKey(Devs,
                                DevInfo,
                                DICS_FLAG_GLOBAL,
                                0,
                                DIREG_DRV,
                                KEY_READ
                               );

    if(hKey == INVALID_HANDLE_VALUE) {
        //
        // no such value exists, so there can't be an associated driver
        //
        RegCloseKey(hKey);
        return FALSE;
    }

    //
    // obtain path of INF - we'll do a search on this specific INF
    //
    RegDataLength = sizeof(deviceInstallParams.DriverPath); // bytes!!!
    regerr = RegQueryValueEx(hKey,
                             REGSTR_VAL_INFPATH,
                             NULL,
                             &RegDataType,
                             (PBYTE)deviceInstallParams.DriverPath,
                             &RegDataLength
                             );

    if((regerr != ERROR_SUCCESS) || (RegDataType != REG_SZ)) {
        //
        // no such value exists, so no associated driver
        //
        RegCloseKey(hKey);
        return FALSE;
    }

    //
    // obtain name of Provider to fill into DriverInfoData
    //
    RegDataLength = sizeof(ProviderName); // bytes!!!
    regerr = RegQueryValueEx(hKey,
                             REGSTR_VAL_PROVIDER_NAME,
                             NULL,
                             &RegDataType,
                             (PBYTE)ProviderName,
                             &RegDataLength
                             );

    if((regerr != ERROR_SUCCESS) || (RegDataType != REG_SZ)) {
        //
        // no such value exists, so we don't have a valid associated driver
        //
        RegCloseKey(hKey);
        return FALSE;
    }

    //
    // obtain name of section - for final verification
    //
    RegDataLength = sizeof(SectionName); // bytes!!!
    regerr = RegQueryValueEx(hKey,
                             REGSTR_VAL_INFSECTION,
                             NULL,
                             &RegDataType,
                             (PBYTE)SectionName,
                             &RegDataLength
                             );

    if((regerr != ERROR_SUCCESS) || (RegDataType != REG_SZ)) {
        //
        // no such value exists, so we don't have a valid associated driver
        //
        RegCloseKey(hKey);
        return FALSE;
    }

    //
    // driver description (need not be same as device description)
    // - for final verification
    //
    RegDataLength = sizeof(DrvDescription); // bytes!!!
    regerr = RegQueryValueEx(hKey,
                             REGSTR_VAL_DRVDESC,
                             NULL,
                             &RegDataType,
                             (PBYTE)DrvDescription,
                             &RegDataLength
                             );

    RegCloseKey(hKey);

    if((regerr != ERROR_SUCCESS) || (RegDataType != REG_SZ)) {
        //
        // no such value exists, so we don't have a valid associated driver
        //
        return FALSE;
    }

    //
    // Manufacturer (via SPDRP_MFG, don't access registry directly!)
    //

    if(!SetupDiGetDeviceRegistryProperty(Devs,
                                        DevInfo,
                                        SPDRP_MFG,
                                        NULL,      // datatype is guaranteed to always be REG_SZ.
                                        (PBYTE)MfgName,
                                        sizeof(MfgName), // bytes!!!
                                        NULL)) {
        //
        // no such value exists, so we don't have a valid associated driver
        //
        return FALSE;
    }

    //
    // now search for drivers listed in the INF
    //
    //
    deviceInstallParams.Flags |= DI_ENUMSINGLEINF;
    deviceInstallParams.FlagsEx |= DI_FLAGSEX_ALLOWEXCLUDEDDRVS;

    if(!SetupDiSetDeviceInstallParams(Devs, DevInfo, &deviceInstallParams)) {
        return FALSE;
    }
    if(!SetupDiBuildDriverInfoList(Devs, DevInfo, SPDIT_CLASSDRIVER)) {
        return FALSE;
    }

    //
    // find the entry in the INF that was used to install the driver for
    // this device
    //
    for(c=0;SetupDiEnumDriverInfo(Devs,DevInfo,SPDIT_CLASSDRIVER,c,DriverInfoData);c++) {
        if((_tcscmp(DriverInfoData->MfgName,MfgName)==0)
            &&(_tcscmp(DriverInfoData->ProviderName,ProviderName)==0)) {
            //
            // these two fields match, try more detailed info
            // to ensure we have the exact driver entry used
            //
            SP_DRVINFO_DETAIL_DATA detail;
            detail.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
            if(!SetupDiGetDriverInfoDetail(Devs,DevInfo,DriverInfoData,&detail,sizeof(detail),NULL)
                    && (GetLastError() != ERROR_INSUFFICIENT_BUFFER)) {
                continue;
            }
            if((_tcscmp(detail.SectionName,SectionName)==0) &&
                (_tcscmp(detail.DrvDescription,DrvDescription)==0)) {
                match = TRUE;
                break;
            }
        }
    }
    if(!match) {
        SetupDiDestroyDriverInfoList(Devs,DevInfo,SPDIT_CLASSDRIVER);
    }
    return match;
}

BOOL DumpDeviceDriverFiles(_In_ HDEVINFO Devs, _In_ PSP_DEVINFO_DATA DevInfo)
/*++

Routine Description:

    Dump information about what files were installed for driver package
    <tab>Installed using OEM123.INF section [abc.NT]
    <tab><tab>file...

Arguments:

    Devs    )_ uniquely identify device
    DevInfo )

Return Value:

    none

--*/
{
    //
    // do this by 'searching' for the current driver
    // mimmicing a copy-only install to our own file queue
    // and then parsing that file queue
    //
    SP_DEVINSTALL_PARAMS deviceInstallParams;
    SP_DRVINFO_DATA driverInfoData;
    SP_DRVINFO_DETAIL_DATA driverInfoDetail;
    HSPFILEQ queueHandle = INVALID_HANDLE_VALUE;
    DWORD count;
    DWORD scanResult;
    BOOL success = FALSE;

    ZeroMemory(&driverInfoData,sizeof(driverInfoData));
    driverInfoData.cbSize = sizeof(driverInfoData);

    if(!FindCurrentDriver(Devs,DevInfo,&driverInfoData)) {
        Padding(1);
        FormatToStream(stdout, MSG_DUMP_NO_DRIVER);
        return FALSE;
    }

    //
    // get useful driver information
    //
    driverInfoDetail.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
    if(!SetupDiGetDriverInfoDetail(Devs,DevInfo,&driverInfoData,&driverInfoDetail,sizeof(SP_DRVINFO_DETAIL_DATA),NULL) &&
       GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        //
        // no information about driver or section
        //
        goto final;
    }
    if(!driverInfoDetail.InfFileName[0] || !driverInfoDetail.SectionName[0]) {
        goto final;
    }

    //
    // pretend to do the file-copy part of a driver install
    // to determine what files are used
    // the specified driver must be selected as the active driver
    //
    if(!SetupDiSetSelectedDriver(Devs, DevInfo, &driverInfoData)) {
        goto final;
    }

    //
    // create a file queue so we can look at this queue later
    //
    queueHandle = SetupOpenFileQueue();

    if ( queueHandle == (HSPFILEQ)INVALID_HANDLE_VALUE ) {
        goto final;
    }

    //
    // modify flags to indicate we're providing our own queue
    //
    ZeroMemory(&deviceInstallParams, sizeof(deviceInstallParams));
    deviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    if ( !SetupDiGetDeviceInstallParams(Devs, DevInfo, &deviceInstallParams) ) {
        goto final;
    }

    //
    // we want to add the files to the file queue, not install them!
    //
    deviceInstallParams.FileQueue = queueHandle;
    deviceInstallParams.Flags |= DI_NOVCP;

    if ( !SetupDiSetDeviceInstallParams(Devs, DevInfo, &deviceInstallParams) ) {
        goto final;
    }

    //
    // now fill queue with files that are to be installed
    // this involves all class/co-installers
    //
    if ( !SetupDiCallClassInstaller(DIF_INSTALLDEVICEFILES, Devs, DevInfo) ) {
        goto final;
    }

    //
    // we now have a list of delete/rename/copy files
    // iterate the copy queue twice - 1st time to get # of files
    // 2nd time to get files
    // (WinXP has API to get # of files, but we want this to work
    // on Win2k too)
    //

    count = 0;
    scanResult = 0;
    //
    // call once to count
    //
    SetupScanFileQueue(queueHandle,SPQ_SCAN_USE_CALLBACK,NULL,DumpDeviceDriversCallback,&count,&scanResult);
    Padding(1);
    FormatToStream(stdout, count ? MSG_DUMP_DRIVER_FILES : MSG_DUMP_NO_DRIVER_FILES, count, driverInfoDetail.InfFileName, driverInfoDetail.SectionName);
    //
    // call again to dump the files
    //
    SetupScanFileQueue(queueHandle,SPQ_SCAN_USE_CALLBACK,NULL,DumpDeviceDriversCallback,NULL,&scanResult);

    success = TRUE;

final:

    SetupDiDestroyDriverInfoList(Devs,DevInfo,SPDIT_CLASSDRIVER);

    if ( queueHandle != (HSPFILEQ)INVALID_HANDLE_VALUE ) {
        SetupCloseFileQueue(queueHandle);
    }

    if(!success) {
        Padding(1);
        FormatToStream(stdout, MSG_DUMP_NO_DRIVER);
    }

    return success;

}

BOOL DumpArray(_In_ int pad, _In_ PZPWSTR Array)
/*++

Routine Description:

    Iterate array and dump entries to screen

Arguments:

    pad   - padding
    Array - array to dump

Return Value:

    none

--*/
{
    if(!Array || !Array[0]) {
        return FALSE;
    }
    while(Array[0]) {
        Padding(pad);
        _tprintf(TEXT("%s\n"),Array[0]);
        Array++;
    }
    return TRUE;
}

BOOL DumpDeviceHwIds(_In_ HDEVINFO Devs, _In_ PSP_DEVINFO_DATA DevInfo)
/*++

Routine Description:

    Write device instance & description to stdout
    <tab>Hardware ID's
    <tab><tab>ID
    ...
    <tab>Compatible ID's
    <tab><tab>ID
    ...
    or
    <tab>No Hardware ID's for device

Arguments:

    Devs    )_ uniquely identify device
    DevInfo )

Return Value:

    none

--*/
{
    LPTSTR * hwIdArray = GetDevMultiSz(Devs,DevInfo,SPDRP_HARDWAREID);
    LPTSTR * compatIdArray = GetDevMultiSz(Devs,DevInfo,SPDRP_COMPATIBLEIDS);
    BOOL displayed = FALSE;

    if(hwIdArray && hwIdArray[0]) {
        displayed = TRUE;
        Padding(1);
        FormatToStream(stdout, MSG_DUMP_HWIDS);
        DumpArray(2,hwIdArray);
    }
    if(compatIdArray && compatIdArray[0]) {
        displayed = TRUE;
        Padding(1);
        FormatToStream(stdout, MSG_DUMP_COMPATIDS);
        DumpArray(2,compatIdArray);
    }
    if(!displayed) {
        Padding(1);
        FormatToStream(stdout, MSG_DUMP_NO_HWIDS);
    }

    DelMultiSz(hwIdArray);
    DelMultiSz(compatIdArray);

    return TRUE;
}

BOOL DumpDeviceDriverNodes(_In_ HDEVINFO Devs, _In_ PSP_DEVINFO_DATA DevInfo)
/*++

Routine Description:

    Write device instance & description to stdout
    <tab>Installed using OEM123.INF section [abc.NT]
    <tab><tab>file...

Arguments:

    Devs    )_ uniquely identify device
    DevInfo )

Return Value:

    none

--*/
{
    BOOL success = FALSE;
    SP_DEVINSTALL_PARAMS deviceInstallParams;
    SP_DRVINFO_DATA driverInfoData;
    SP_DRVINFO_DETAIL_DATA driverInfoDetail;
    SP_DRVINSTALL_PARAMS driverInstallParams;
    DWORD index;
    SYSTEMTIME SystemTime;
    ULARGE_INTEGER Version;
    TCHAR Buffer[MAX_PATH];

    ZeroMemory(&deviceInstallParams, sizeof(deviceInstallParams));
    ZeroMemory(&driverInfoData, sizeof(driverInfoData));

    driverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    deviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

    if(!SetupDiGetDeviceInstallParams(Devs, DevInfo, &deviceInstallParams)) {
        return FALSE;
    }

    //
    // Set the flags that tell SetupDiBuildDriverInfoList to allow excluded drivers.
    //
    deviceInstallParams.FlagsEx |= DI_FLAGSEX_ALLOWEXCLUDEDDRVS;

    if(!SetupDiSetDeviceInstallParams(Devs, DevInfo, &deviceInstallParams)) {
        return FALSE;
    }

    //
    // Now build a class driver list.
    //
    if(!SetupDiBuildDriverInfoList(Devs, DevInfo, SPDIT_COMPATDRIVER)) {
        goto final2;
    }

    //
    // Enumerate all of the drivernodes.
    //
    index = 0;
    while(SetupDiEnumDriverInfo(Devs, DevInfo, SPDIT_COMPATDRIVER,
                                index, &driverInfoData)) {

        success = TRUE;

        FormatToStream(stdout,MSG_DUMP_DRIVERNODE_HEADER,index);

        //
        // get useful driver information
        //
        driverInfoDetail.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
        if(SetupDiGetDriverInfoDetail(Devs,DevInfo,&driverInfoData,&driverInfoDetail,sizeof(SP_DRVINFO_DETAIL_DATA),NULL) ||
           GetLastError() == ERROR_INSUFFICIENT_BUFFER) {

            Padding(1);
            FormatToStream(stdout,MSG_DUMP_DRIVERNODE_INF,driverInfoDetail.InfFileName);
            Padding(1);
            FormatToStream(stdout,MSG_DUMP_DRIVERNODE_SECTION,driverInfoDetail.SectionName);
        }

        Padding(1);
        FormatToStream(stdout,MSG_DUMP_DRIVERNODE_DESCRIPTION,driverInfoData.Description);
        Padding(1);
        FormatToStream(stdout,MSG_DUMP_DRIVERNODE_MFGNAME,driverInfoData.MfgName);
        Padding(1);
        FormatToStream(stdout,MSG_DUMP_DRIVERNODE_PROVIDERNAME,driverInfoData.ProviderName);

        if (FileTimeToSystemTime(&driverInfoData.DriverDate, &SystemTime)) {
            if (GetDateFormat(LOCALE_USER_DEFAULT,
                              DATE_SHORTDATE,
                              &SystemTime,
                              NULL,
                              Buffer,
                              sizeof(Buffer)/sizeof(TCHAR)
                              ) != 0) {
                Padding(1);
                FormatToStream(stdout,MSG_DUMP_DRIVERNODE_DRIVERDATE,Buffer);
            }
        }

        Version.QuadPart = driverInfoData.DriverVersion;
        Padding(1);
        FormatToStream(stdout,MSG_DUMP_DRIVERNODE_DRIVERVERSION,
                       HIWORD(Version.HighPart),
                       LOWORD(Version.HighPart),
                       HIWORD(Version.LowPart),
                       LOWORD(Version.LowPart)
                       );

        driverInstallParams.cbSize = sizeof(SP_DRVINSTALL_PARAMS);
        if(SetupDiGetDriverInstallParams(Devs,DevInfo,&driverInfoData,&driverInstallParams)) {
            Padding(1);
            FormatToStream(stdout,MSG_DUMP_DRIVERNODE_RANK,driverInstallParams.Rank);
            Padding(1);
            FormatToStream(stdout,MSG_DUMP_DRIVERNODE_FLAGS,driverInstallParams.Flags);

            //
            // Interesting flags to dump
            //
            if (driverInstallParams.Flags & DNF_OLD_INET_DRIVER) {
                Padding(2);
                FormatToStream(stdout,MSG_DUMP_DRIVERNODE_FLAGS_OLD_INET_DRIVER);
            }
            if (driverInstallParams.Flags & DNF_BAD_DRIVER) {
                Padding(2);
                FormatToStream(stdout,MSG_DUMP_DRIVERNODE_FLAGS_BAD_DRIVER);
            }
#if defined(DNF_INF_IS_SIGNED)
            //
            // DNF_INF_IS_SIGNED is only available since WinXP
            //
            if (driverInstallParams.Flags & DNF_INF_IS_SIGNED) {
                Padding(2);
                FormatToStream(stdout,MSG_DUMP_DRIVERNODE_FLAGS_INF_IS_SIGNED);
            }
#endif
#if defined(DNF_OEM_F6_INF)
            //
            // DNF_OEM_F6_INF is only available since WinXP
            //
            if (driverInstallParams.Flags & DNF_OEM_F6_INF) {
                Padding(2);
                FormatToStream(stdout,MSG_DUMP_DRIVERNODE_FLAGS_OEM_F6_INF);
            }
#endif
#if defined(DNF_BASIC_DRIVER)
            //
            // DNF_BASIC_DRIVER is only available since WinXP
            //
            if (driverInstallParams.Flags & DNF_BASIC_DRIVER) {
                Padding(2);
                FormatToStream(stdout,MSG_DUMP_DRIVERNODE_FLAGS_BASIC_DRIVER);
            }
#endif
        }

        index++;
    }


    SetupDiDestroyDriverInfoList(Devs,DevInfo,SPDIT_COMPATDRIVER);


final2:

    if(!success) {
        Padding(1);
        FormatToStream(stdout, MSG_DUMP_NO_DRIVERNODES);
    }

    return success;

}

BOOL DumpDeviceStack(_In_ HDEVINFO Devs, _In_ PSP_DEVINFO_DATA DevInfo)
/*++

Routine Description:

    Write expected stack information to stdout

Arguments:

    Devs    )_ uniquely identify device
    DevInfo )

Return Value:

    TRUE if success

--*/
{
    LPTSTR * filters;
    LPTSTR service;
    HKEY hClassKey = (HKEY)INVALID_HANDLE_VALUE;
    SP_DEVINFO_LIST_DETAIL_DATA devInfoListDetail;

    //
    // we need machine information
    //
    devInfoListDetail.cbSize = sizeof(devInfoListDetail);
    if(!SetupDiGetDeviceInfoListDetail(Devs,&devInfoListDetail)) {
        return FALSE;
    }

    //
    // we need device setup class, we can use the GUID in DevInfo
    // note that this GUID is a snapshot, but works fine
    // if DevInfo isn't old
    //

    //
    // class upper/lower filters are in class registry
    //
    hClassKey = SetupDiOpenClassRegKeyEx(&DevInfo->ClassGuid,
                                         KEY_READ,
                                         DIOCR_INSTALLER,
                                         devInfoListDetail.RemoteMachineName[0] ? devInfoListDetail.RemoteMachineName : NULL,
                                         NULL);

    if(hClassKey != INVALID_HANDLE_VALUE) {
        //
        // dump upper class filters if available
        //
        filters = GetRegMultiSz(hClassKey,REGSTR_VAL_UPPERFILTERS);
        if(filters) {
            if(filters[0]) {
                Padding(1);
                FormatToStream(stdout,MSG_DUMP_DEVICESTACK_UPPERCLASSFILTERS);
                DumpArray(2,filters);
            }
            DelMultiSz(filters);
        }
    }
    filters = GetDevMultiSz(Devs,DevInfo,SPDRP_UPPERFILTERS);
    if(filters) {
        if(filters[0]) {
            //
            // dump upper device filters
            //
            Padding(1);
            FormatToStream(stdout,MSG_DUMP_DEVICESTACK_UPPERFILTERS);
            DumpArray(2,filters);
        }
        DelMultiSz(filters);
    }
    service = GetDeviceStringProperty(Devs,DevInfo,SPDRP_SERVICE);
    Padding(1);
    FormatToStream(stdout,MSG_DUMP_DEVICESTACK_SERVICE);
    if(service && service[0]) {
        //
        // dump service
        //
        Padding(2);
        _tprintf(TEXT("%s\n"),service);
    } else {
        //
        // dump the fact that there's no service
        //
        Padding(2);
        FormatToStream(stdout,MSG_DUMP_DEVICESTACK_NOSERVICE);
    }
    if(service) {
        delete [] service;
    }
    if(hClassKey != INVALID_HANDLE_VALUE) {
        filters = GetRegMultiSz(hClassKey,REGSTR_VAL_LOWERFILTERS);
        if(filters) {
            if(filters[0]) {
                //
                // lower class filters
                //
                Padding(1);
                FormatToStream(stdout,MSG_DUMP_DEVICESTACK_LOWERCLASSFILTERS);
                DumpArray(2,filters);
            }
            DelMultiSz(filters);
        }
        RegCloseKey(hClassKey);
    }
    filters = GetDevMultiSz(Devs,DevInfo,SPDRP_LOWERFILTERS);
    if(filters) {
        if(filters[0]) {
            //
            // lower device filters
            //
            Padding(1);
            FormatToStream(stdout,MSG_DUMP_DEVICESTACK_LOWERFILTERS);
            DumpArray(2,filters);
        }
        DelMultiSz(filters);
    }

    return TRUE;
}

BOOL DumpDriverPackageData(_In_ LPCTSTR InfName)
{
    DWORD Err = NO_ERROR;
    HINF hInf = INVALID_HANDLE_VALUE;
    UINT ErrorLine;
    INFCONTEXT Context;
    TCHAR InfData[MAX_INF_STRING_LENGTH];
    GUID ClassGuid;
#if _SETUPAPI_VER >= _WIN32_WINNT_WINXP
    SetupVerifyInfFileProto SVIFFn;
#endif // _SETUPAPI_VER >= _WIN32_WINNT_WINXP
    HMODULE setupapiMod = NULL;
    
#if _SETUPAPI_VER >= _WIN32_WINNT_WINXP
    SP_INF_SIGNER_INFO InfSignerInfo;
#endif // _SETUPAPI_VER >= _WIN32_WINNT_WINXP

    hInf = SetupOpenInfFile(InfName,
                            NULL,
                            INF_STYLE_WIN4,
                            &ErrorLine);
    if (hInf == INVALID_HANDLE_VALUE) {
        Err = GetLastError();
        goto failed;
    }

    //
    // Dump out the provider.
    //
    if (SetupFindFirstLine(hInf,
                           INFSTR_SECT_VERSION,
                           INFSTR_KEY_PROVIDER,
                           &Context) &&
        (SetupGetStringField(&Context,
                             1,
                             InfData,
                             ARRAYSIZE(InfData),
                             NULL))) {
        FormatToStream(stdout,MSG_DPENUM_DUMP_PROVIDER,InfData);
    } else {
        FormatToStream(stdout,MSG_DPENUM_DUMP_PROVIDER_UNKNOWN);
    }

    //
    // Dump out the class
    //
    if (SetupFindFirstLine(hInf,
                           INFSTR_SECT_VERSION,
                           INFSTR_KEY_HARDWARE_CLASSGUID,
                           &Context) &&
        (SetupGetStringField(&Context,
                             1,
                             InfData,
                             ARRAYSIZE(InfData),
                             NULL)) &&
        (SUCCEEDED(CLSIDFromString(InfData, &ClassGuid))) &&
        SetupDiGetClassDescriptionEx(&ClassGuid,InfData,ARRAYSIZE(InfData),NULL,NULL,NULL)) {
        FormatToStream(stdout,MSG_DPENUM_DUMP_CLASS,InfData);
    } else {
        FormatToStream(stdout,MSG_DPENUM_DUMP_CLASS_UNKNOWN);
    }

#if _SETUPAPI_VER >= _WIN32_WINNT_WINXP
    //
    // Dump out the digital signer
    //
    setupapiMod = LoadLibrary(TEXT("setupapi.dll"));
    if(!setupapiMod){
        goto failed;
    }
    SVIFFn = (SetupVerifyInfFileProto)GetProcAddress(setupapiMod, SETUPVERIFYINFFILE);
    if(!SVIFFn){
        goto failed;
    }
    
    ZeroMemory(&InfSignerInfo, sizeof(InfSignerInfo));
    InfSignerInfo.cbSize = sizeof(InfSignerInfo);
    if (SVIFFn(InfName, NULL, &InfSignerInfo) ||
        (GetLastError() == ERROR_AUTHENTICODE_TRUSTED_PUBLISHER) ||
        (GetLastError() == ERROR_AUTHENTICODE_TRUST_NOT_ESTABLISHED)) {
        FormatToStream(stdout,MSG_DPENUM_DUMP_SIGNER,InfSignerInfo.DigitalSigner);
    } else {
        FormatToStream(stdout,MSG_DPENUM_DUMP_SIGNER_UNKNOWN);
    }
#endif // _SETUPAPI_VER >= _WIN32_WINNT_WINXP

    //
    // Dump out the version and date
    //
    if (SetupFindFirstLine(hInf,
                           INFSTR_SECT_VERSION,
                           INFSTR_DRIVERVERSION_SECTION,
                           &Context)) {
        if (SetupGetStringField(&Context,
                                1,
                                InfData,
                                ARRAYSIZE(InfData),
                                NULL)) {
            FormatToStream(stdout,MSG_DPENUM_DUMP_DATE,InfData);
        } else {
            FormatToStream(stdout,MSG_DPENUM_DUMP_DATE_UNKNOWN);
        }
    
        if (SetupGetStringField(&Context,
                                2,
                                InfData,
                                ARRAYSIZE(InfData),
                                NULL)) {
            FormatToStream(stdout,MSG_DPENUM_DUMP_VERSION,InfData);
        } else {
            FormatToStream(stdout,MSG_DPENUM_DUMP_VERSION_UNKNOWN);
        }

    } else {
        FormatToStream(stdout,MSG_DPENUM_DUMP_DATE_UNKNOWN);
        FormatToStream(stdout,MSG_DPENUM_DUMP_VERSION_UNKNOWN);
    }


failed:
    if (hInf != INVALID_HANDLE_VALUE) {
        SetupCloseInfFile(hInf);
    }

    if(setupapiMod){
        FreeLibrary(setupapiMod);
    }

    return (Err == NO_ERROR);
}


