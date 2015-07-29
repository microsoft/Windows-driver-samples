// 45678901234567890123456789012345678901234567890123456789012345678901234567890
//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       COINST.C
//
//  Contents:   co-installer hook.
//
//  Notes:      For a complete description of CoInstallers, please see the
//                 Microsoft Windows 2000 DDK Documentation
//
//
// Revision History:
//              Added FriendlyName interface
//
//              Modified to create a friendlyname in the first callback to
//              to install device rather than in post-processing.
//                  - July 28, 2000
//
//----------------------------------------------------------------------------

//
// Annotation to indicate to prefast that this is nondriver user-mode code.
//
#include <DriverSpecs.h>
__user_code  

#include <windows.h>
#include <setupapi.h>
#include <stdio.h>
#include <strsafe.h>

//+---------------------------------------------------------------------------
//
// WARNING!
//
// A Coinstaller must not generate any popup to the user.
//     it should provide appropriate defaults.
//
//  OutputDebugString should be fine...
//
#if DBG
#define DbgOut(Text) OutputDebugString(TEXT("CoInstaller: " Text "\n"))
#else
#define DbgOut(Text)
#endif

//+---------------------------------------------------------------------------
//
//  Function:   MyOpenInfFile
//
//  Purpose:    Will open the handle to the INF file being installed
//
//  Arguments:
//      DeviceInfoSet     [in]
//      DeviceInfoData    [in]
//      ErrorLine         [out] // Optional, See SetupOpenInfFile
//
//  Returns:    HINF Handle of INF file
//
HINF MyOpenInfFile (
                    _In_      HDEVINFO          DeviceInfoSet,
                    _In_      PSP_DEVINFO_DATA  DeviceInfoData,
                    _Out_opt_ PUINT             ErrorLine
                    )
{
    SP_DRVINFO_DATA DriverInfoData;
    SP_DRVINFO_DETAIL_DATA DriverInfoDetailData;
    DWORD Status;
    HINF FileHandle;

    if (NULL != ErrorLine)
    {
        *ErrorLine = 0;
    }

    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    if (!SetupDiGetSelectedDriver( DeviceInfoSet,
        DeviceInfoData,
        &DriverInfoData))
    {
        DbgOut("Fail: SetupDiGetSelectedDriver");
        return INVALID_HANDLE_VALUE;
    }

    DriverInfoDetailData.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
    if (!SetupDiGetDriverInfoDetail(DeviceInfoSet,
        DeviceInfoData,
        &DriverInfoData,
        &DriverInfoDetailData,
        sizeof(SP_DRVINFO_DETAIL_DATA),
        NULL))
    {
        if ((Status = GetLastError()) == ERROR_INSUFFICIENT_BUFFER)
        {
            // We don't need the extended information.  Ignore.
        }
        else
        {
            DbgOut("Fail: SetupDiGetDriverInfoDetail");
            return INVALID_HANDLE_VALUE;
        }
    }

    if (INVALID_HANDLE_VALUE == (FileHandle = SetupOpenInfFile(
        DriverInfoDetailData.InfFileName,
        NULL,
        INF_STYLE_WIN4,
        ErrorLine)))
    {
        DbgOut("Fail: SetupOpenInfFile");
    }
    return FileHandle;
}

//+---------------------------------------------------------------------------
//
//  Function:   ToasterCoInstaller
//
//  Purpose:    Responds to co-installer messages
//
//  Arguments:
//      InstallFunction   [in]
//      DeviceInfoSet     [in]
//      DeviceInfoData    [in]
//      Context           [inout]
//
//  Returns:    NO_ERROR, ERROR_DI_POSTPROCESSING_REQUIRED, or an error code.
//
DWORD CALLBACK
ToasterCoInstaller (
               _In_     DI_FUNCTION               InstallFunction,
               _In_     HDEVINFO                  DeviceInfoSet,
               _In_     PSP_DEVINFO_DATA          DeviceInfoData,  OPTIONAL
               _Inout_ PCOINSTALLER_CONTEXT_DATA Context
               )
{
    switch (InstallFunction)
    {
    case DIF_INSTALLDEVICE:
        if(!Context->PostProcessing)
        {
            TCHAR       FriendlyName[MAX_PATH];
            BOOL        fSuccess=FALSE;
            DWORD       dwRegType, UINumber;
            size_t      len;

            DbgOut("DIF_INSTALLDEVICE");

            //
            // We wil create here a friendly name for this device
            // based on it's serial number.
            // The bus driver returns the serial No. in the UINumber.
            // field of the device capabiliities structure.
            // So let us get that first .
            //
            fSuccess =
            SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                             DeviceInfoData,
                                             SPDRP_UI_NUMBER,
                                             &dwRegType,
                                             (BYTE*) &UINumber,
                                             sizeof(UINumber),
                                             NULL);
            if (fSuccess)
            {
                //
                // Cook a FriendlyName and add it to the registry
                //
                if (SUCCEEDED(StringCbPrintf(FriendlyName,
                               sizeof(FriendlyName),
                               TEXT("ToasterDevice%02u"),
                               UINumber))) {

                    if (SUCCEEDED(StringCbLength(FriendlyName,
                                    sizeof(FriendlyName),
                                    &len))) {
                        fSuccess = SetupDiSetDeviceRegistryProperty(DeviceInfoSet,
                                     DeviceInfoData,
                                     SPDRP_FRIENDLYNAME,
                                     (BYTE *)FriendlyName,
                                     (DWORD)len
                                     );
                        if(!fSuccess) {
                            DbgOut("SetupDiSetDeviceRegistryProperty failed!");
                        }
                    }
                    
                }

            }

            // You can use PrivateData to pass Data needed for PostProcessing
            // Context->PrivateData = Something;

            return ERROR_DI_POSTPROCESSING_REQUIRED; //Set for PostProcessing
        }
        else // post processing
        {
            INFCONTEXT  InfContext;
            HINF        InfFile;

            //
            // Sample code to show how you can process a custom section
            // in your INF file.
            //
            DbgOut("DIF_INSTALLDEVICE PostProcessing");

            if (INVALID_HANDLE_VALUE == (InfFile =
                MyOpenInfFile(DeviceInfoSet,DeviceInfoData,NULL)))
            {
                return GetLastError();
            }

            if (SetupFindFirstLine(InfFile, // InfHandle
                TEXT("MySection"),
                TEXT("MySpecialFlag"),
                &InfContext))
            {
                DbgOut("DIF_INSTALLDEVICE MySpecicalFlag, Do something here!");

            }

        }
        break;

    case DIF_REMOVE:
        DbgOut("DIF_REMOVE");
        break;

    case DIF_SELECTDEVICE:
        DbgOut("DIF_SELECTDEVICE");
        break;
    case DIF_ASSIGNRESOURCES:
        DbgOut("DIF_ASSIGNRESOURCES");
        break;
    case DIF_PROPERTIES:
        DbgOut("DIF_PROPERTIES");
        break;
    case DIF_FIRSTTIMESETUP:
        DbgOut("DIF_FIRSTTIMESETUP");
        break;
    case DIF_FOUNDDEVICE:
        DbgOut("DIF_FOUNDDEVICE");
        break;
    case DIF_SELECTCLASSDRIVERS:
        DbgOut("DIF_SELECTCLASSDRIVERS");
        break;
    case DIF_VALIDATECLASSDRIVERS:
        DbgOut("DIF_VALIDATECLASSDRIVERS");
        break;
    case DIF_INSTALLCLASSDRIVERS:
        DbgOut("DIF_INSTALLCLASSDRIVERS");
        break;
    case DIF_CALCDISKSPACE:
        DbgOut("DIF_CALCDISKSPACE");
        break;
    case DIF_DESTROYPRIVATEDATA:
        DbgOut("DIF_DESTROYPRIVATEDATA");
        break;
    case DIF_VALIDATEDRIVER:
        DbgOut("DIF_VALIDATEDRIVER");
        break;
    case DIF_MOVEDEVICE:
        DbgOut("DIF_MOVEDEVICE");
        break;
    case DIF_DETECT:
        DbgOut("DIF_DETECT");
        break;
    case DIF_INSTALLWIZARD:
        DbgOut("DIF_INSTALLWIZARD");
        break;
    case DIF_DESTROYWIZARDDATA:
        DbgOut("DIF_DESTROYWIZARDDATA");
        break;
    case DIF_PROPERTYCHANGE:
        DbgOut("DIF_PROPERTYCHANGE");
        break;
    case DIF_ENABLECLASS:
        DbgOut("DIF_ENABLECLASS");
        break;
    case DIF_DETECTVERIFY:
        DbgOut("DIF_DETECTVERIFY");
        break;
    case DIF_INSTALLDEVICEFILES:
        DbgOut("DIF_INSTALLDEVICEFILES");
        break;
    case DIF_ALLOW_INSTALL:
        DbgOut("DIF_ALLOW_INSTALL");
        break;
    case DIF_SELECTBESTCOMPATDRV:
        DbgOut("DIF_SELECTBESTCOMPATDRV");
        break;
    case DIF_REGISTERDEVICE:
        DbgOut("DIF_REGISTERDEVICE");
        break;
    case DIF_NEWDEVICEWIZARD_PRESELECT:
        DbgOut("DIF_NEWDEVICEWIZARD_PRESELECT");
        break;
    case DIF_NEWDEVICEWIZARD_SELECT:
        DbgOut("DIF_NEWDEVICEWIZARD_SELECT");
        break;
    case DIF_NEWDEVICEWIZARD_PREANALYZE:
        DbgOut("DIF_NEWDEVICEWIZARD_PREANALYZE");
        break;
    case DIF_NEWDEVICEWIZARD_POSTANALYZE:
        DbgOut("DIF_NEWDEVICEWIZARD_POSTANALYZE");
        break;
    case DIF_NEWDEVICEWIZARD_FINISHINSTALL:
        DbgOut("DIF_NEWDEVICEWIZARD_FINISHINSTALL");
        break;
    case DIF_INSTALLINTERFACES:
        DbgOut("DIF_INSTALLINTERFACES");
        break;
    case DIF_DETECTCANCEL:
        DbgOut("DIF_DETECTCANCEL");
        break;
    case DIF_REGISTER_COINSTALLERS:
        DbgOut("DIF_REGISTER_COINSTALLERS");
        break;
    case DIF_ADDPROPERTYPAGE_ADVANCED:
        DbgOut("DIF_ADDPROPERTYPAGE_ADVANCED");
        break;
    case DIF_ADDPROPERTYPAGE_BASIC:
        DbgOut("DIF_ADDPROPERTYPAGE_BASIC");
        break;
    case DIF_TROUBLESHOOTER:
        DbgOut("DIF_TROUBLESHOOTER");
        break;
    case DIF_POWERMESSAGEWAKE:
        DbgOut("DIF_POWERMESSAGEWAKE");
        break;
    default:
        DbgOut("?????");
        break;
    }

    return NO_ERROR;
}


