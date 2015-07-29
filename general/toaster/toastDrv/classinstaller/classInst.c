//+---------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  File:       ClassINST.C
//
//  Contents:   class-installer for toaster.
//
//  Notes:      This is a sample class installer. Class installer is not required 
//              for every class of device. I have included here to provide 
//              an icon for the toaster class and to demonstrate how one
//              can provide a customer property sheet in the device manager
//              in response to DIF_ADDPROPERTYPAGE_ADVANCED. 
//              The property sheet has a check box. If you enable the box
//              and press OK, it will restart the device. This is required
//              if the user changes the device properties, and it needs to
//              be restarted to apply the new attributes.
//              For a complete description of ClassInstallers, please see the 
//              Microsoft Windows 2000 DDK Documentation.
//
//  
//  Revision: Added property page - Nov 14, 2000
//  
//
//----------------------------------------------------------------------------

//
// Annotation to indicate to prefast that this is nondriver user-mode code.
//
#include <DriverSpecs.h>
__user_code  

#include <windows.h>
#include <strsafe.h>
#include <setupapi.h>
#include <stdio.h>
#include "resource.h"
#include <dontuse.h>
//+---------------------------------------------------------------------------
//
// WARNING! 
//
// Installer must not generate any popup to the user.
//    It should provide appropriate defaults.
//
//  OutputDebugString should be fine...
//
#if DBG
#define DbgOut(Text) OutputDebugString(TEXT("ClassInstaller: " Text "\n"))
#else
#define DbgOut(Text) 
#endif 

typedef struct _TOASTER_PROP_PARAMS
{

   HDEVINFO                     DeviceInfoSet;
   PSP_DEVINFO_DATA             DeviceInfoData;
   BOOL                         Restart;
   
} TOASTER_PROP_PARAMS, *PTOASTER_PROP_PARAMS;


INT_PTR
PropPageDlgProc(_In_ HWND   hDlg,
               _In_ UINT   uMessage,
               _In_ WPARAM wParam,
               _In_ LPARAM lParam);

UINT
CALLBACK
PropPageDlgCallback(HWND hwnd,
                    UINT uMsg,
                    LPPROPSHEETPAGE ppsp);
DWORD
PropPageProvider(
    _In_  HDEVINFO            DeviceInfoSet,
    _In_  PSP_DEVINFO_DATA    DeviceInfoData OPTIONAL
);

BOOL
OnNotify(
    HWND    ParentHwnd,
    LPNMHDR NmHdr,
    PTOASTER_PROP_PARAMS Params
    );
    
HMODULE ModuleInstance;

BOOL WINAPI 
DllMain(
    HINSTANCE DllInstance,
    DWORD Reason,
    PVOID Reserved
    )
{

    UNREFERENCED_PARAMETER( Reserved );

    switch(Reason) {

        case DLL_PROCESS_ATTACH: {

            ModuleInstance = DllInstance;
            DisableThreadLibraryCalls(DllInstance);
            InitCommonControls();
            break;
        }

        case DLL_PROCESS_DETACH: {
            ModuleInstance = NULL;
            break;
        }

        default: {
            break;
        }
    }

    return TRUE;
}

DWORD CALLBACK
ToasterClassInstaller(
    _In_  DI_FUNCTION         InstallFunction,
    _In_  HDEVINFO            DeviceInfoSet,
    _In_  PSP_DEVINFO_DATA    DeviceInfoData OPTIONAL
    )
/*++

Routine Description: 

    Responds to Class-installer messages
    .  
Arguments:

     InstallFunction   [in] 
     DeviceInfoSet     [in]
     DeviceInfoData    [in]

Return Value:

Returns:    NO_ERROR, ERROR_DI_POSTPROCESSING_REQUIRED, or an error code.

--*/
{
    switch (InstallFunction)
    {
        case DIF_INSTALLDEVICE: 
            //
            // Sent twice: once before installing the device and once
            // after installing device, if you have returned 
            // ERROR_DI_POSTPROCESSING_REQUIRED during the first pass.
            //
            DbgOut("DIF_INSTALLDEVICE");
            break;
            
        case DIF_ADDPROPERTYPAGE_ADVANCED:
            //
            // Sent when you check the properties of the device in the
            // device manager.
            //
            DbgOut("DIF_ADDPROPERTYPAGE_ADVANCED");
            return PropPageProvider(DeviceInfoSet, DeviceInfoData);           
            
        case DIF_POWERMESSAGEWAKE:
            //
            // Sent when you check the power management tab 
            //
            DbgOut("DIF_POWERMESSAGEWAKE");
            break;

        case DIF_PROPERTYCHANGE:
            //
            // Sent when you change the property of the device using
            // SetupDiSetDeviceInstallParams. (Enable/Disable/Restart)
            //
            DbgOut("DIF_PROPERTYCHANGE");
            break;
        case DIF_REMOVE: 
             //
             // Sent when you uninstall the device.
             //
             DbgOut("DIF_REMOVE");
             break;
             
        case DIF_NEWDEVICEWIZARD_FINISHINSTALL:
            //
            // Sent near the end of installation to allow 
            // an installer to supply wizard page(s) to the user.
            // These wizard pages are different from the device manager
            // property sheet.There are popped only once during install.
            //
            DbgOut("DIF_NEWDEVICEWIZARD_FINISHINSTALL");
            break;
            
        case DIF_SELECTDEVICE:
            DbgOut("DIF_SELECTDEVICE");
            break;
        case DIF_DESTROYPRIVATEDATA:
            //
            // Sent when Setup destroys a device information set 
            // or an SP_DEVINFO_DATA element, or when Setup discards 
            // its list of co-installers and class installer for a device
            //
            DbgOut("DIF_DESTROYPRIVATEDATA");
            break;
        case DIF_INSTALLDEVICEFILES:
            DbgOut("DIF_INSTALLDEVICEFILES");
            break;
        case DIF_ALLOW_INSTALL:
            //
            // Sent to confirm whether the installer wants to allow
            // the installation of device.
            //
            DbgOut("DIF_ALLOW_INSTALL");
            break;
        case DIF_SELECTBESTCOMPATDRV:
            DbgOut("DIF_SELECTBESTCOMPATDRV");
            break;

        case DIF_INSTALLINTERFACES:
            DbgOut("DIF_INSTALLINTERFACES");
            break;
        case DIF_REGISTER_COINSTALLERS:
            DbgOut("DIF_REGISTER_COINSTALLERS");
            break;
        default:
            DbgOut("DIF_???");
            break;
    }   
    return ERROR_DI_DO_DEFAULT;    
}

DWORD
PropPageProvider(
    _In_  HDEVINFO            DeviceInfoSet,
    _In_  PSP_DEVINFO_DATA    DeviceInfoData OPTIONAL
)

/*++

Routine Description: 

    Entry-point for adding additional device manager property
    sheet pages.  
Arguments:

Return Value:

Returns:    NO_ERROR, ERROR_DI_DO_DEFAULT, or an error code.

--*/
{
    HPROPSHEETPAGE  pageHandle;
    PROPSHEETPAGE   page;
    PTOASTER_PROP_PARAMS      params = NULL;
    SP_ADDPROPERTYPAGE_DATA AddPropertyPageData = {0};

    //
    // DeviceInfoSet is NULL if setup is requesting property pages for
    // the device setup class. We don't want to do anything in this 
    // case.
    //
    if (DeviceInfoData==NULL) {
        return ERROR_DI_DO_DEFAULT;
    }

    AddPropertyPageData.ClassInstallHeader.cbSize = 
         sizeof(SP_CLASSINSTALL_HEADER);

    //
    // Get the current class install parameters for the device
    //

    if (SetupDiGetClassInstallParams(DeviceInfoSet, DeviceInfoData,
         (PSP_CLASSINSTALL_HEADER)&AddPropertyPageData,
         sizeof(SP_ADDPROPERTYPAGE_DATA), NULL )) 
    {

        //
        // Ensure that the maximum number of dynamic pages for the 
        // device has not yet been met
        //
        if(AddPropertyPageData.NumDynamicPages >= MAX_INSTALLWIZARD_DYNAPAGES){
            return NO_ERROR;
        }
        params = HeapAlloc(GetProcessHeap(), 0, sizeof(TOASTER_PROP_PARAMS));
        if (params)
        {
            //
            // Save DeviceInfoSet and DeviceInfoData
            //
            params->DeviceInfoSet     = DeviceInfoSet;
            params->DeviceInfoData    = DeviceInfoData;
            params->Restart           = FALSE;
            
            //
            // Create custom property sheet page
            //
            memset(&page, 0, sizeof(PROPSHEETPAGE));

            page.dwSize = sizeof(PROPSHEETPAGE);
            page.dwFlags = PSP_USECALLBACK;
            page.hInstance = ModuleInstance;
            page.pszTemplate = MAKEINTRESOURCE(DLG_TOASTER_PORTSETTINGS);
            page.pfnDlgProc = (DLGPROC) PropPageDlgProc;
            page.pfnCallback = PropPageDlgCallback;

            page.lParam = (LPARAM) params;

            pageHandle = CreatePropertySheetPage(&page);
            if(!pageHandle)
            {
                HeapFree(GetProcessHeap(), 0, params);
                return NO_ERROR;
            }

            //
            // Add the new page to the list of dynamic property 
            // sheets
            //
            AddPropertyPageData.DynamicPages[
                AddPropertyPageData.NumDynamicPages++]=pageHandle;

            SetupDiSetClassInstallParams(DeviceInfoSet,
                        DeviceInfoData,
                        (PSP_CLASSINSTALL_HEADER)&AddPropertyPageData,
                        sizeof(SP_ADDPROPERTYPAGE_DATA));
        }
    }
    return NO_ERROR;
} 

INT_PTR
PropPageDlgProc(_In_ HWND   hDlg,
                   _In_ UINT   uMessage,
                   _In_ WPARAM wParam,
                   _In_ LPARAM lParam)
/*++

Routine Description: PropPageDlgProc

    The windows control function for the custom property page window

Arguments:

    hDlg, uMessage, wParam, lParam: standard windows DlgProc parameters

Return Value:

    BOOL: FALSE if function fails, TRUE if function passes

--*/
{
    PTOASTER_PROP_PARAMS params;

    UNREFERENCED_PARAMETER( wParam );

    params = (PTOASTER_PROP_PARAMS) GetWindowLongPtr(hDlg, DWLP_USER);

    switch(uMessage) {
    case WM_COMMAND:
        break;

    case WM_CONTEXTMENU:
        break;

    case WM_HELP:
        break;

    case WM_INITDIALOG:

        //
        // on WM_INITDIALOG call, lParam points to the property
        // sheet page.
        //
        // The lParam field in the property sheet page struct is set by the
        // caller. This was set when we created the property sheet.
        // Save this in the user window long so that we can access it on later 
        // on later messages.
        //

        params = (PTOASTER_PROP_PARAMS) ((LPPROPSHEETPAGE)lParam)->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR) params);
        break;


    case WM_NOTIFY:
        OnNotify(hDlg, (NMHDR *)lParam, params);
        break;

    default: 
        return FALSE;
    }

    return TRUE;
} 


UINT
CALLBACK
PropPageDlgCallback(HWND hwnd,
                   UINT uMsg,
                   LPPROPSHEETPAGE ppsp)
{
    PTOASTER_PROP_PARAMS params;

    UNREFERENCED_PARAMETER( hwnd );

    switch (uMsg) {

    case PSPCB_CREATE:
        //
        // Called when the property sheet is first displayed
        //
        return TRUE;    // return TRUE to continue with creation of page

    case PSPCB_RELEASE:
        //
        // Called when property page is destroyed, even if the page 
        // was never displayed. This is the correct way to release data.
        //
        params = (PTOASTER_PROP_PARAMS) ppsp->lParam;
        LocalFree(params);
        return 0;       // return value ignored
    default:
        break;
    }

    return TRUE;
}


BOOL
OnNotify(
    HWND    ParentHwnd,
    LPNMHDR NmHdr,
    PTOASTER_PROP_PARAMS Params
    )
{
    SP_DEVINSTALL_PARAMS spDevInstall = {0};
    TCHAR                friendlyName[LINE_LEN] ={0};
    size_t  charCount;
    BOOL    fSuccess;

    switch (NmHdr->code) {
    case PSN_APPLY:
        //
        // Sent when the user clicks on Apply OR OK !!
        //

        GetDlgItemText(ParentHwnd, IDC_FRIENDLYNAME, friendlyName,
                                        LINE_LEN-1 );
        friendlyName[LINE_LEN-1] = UNICODE_NULL;
        if(friendlyName[0]) {

            if (FAILED(StringCchLength(friendlyName,
                         STRSAFE_MAX_CCH,
                         &charCount))) {
                DbgOut("StringCchLength failed!");                   
                break;
            }
            fSuccess = SetupDiSetDeviceRegistryProperty(Params->DeviceInfoSet, 
                         Params->DeviceInfoData,
                         SPDRP_FRIENDLYNAME,
                         (BYTE *)friendlyName,
                         (DWORD)((charCount + 1) * sizeof(TCHAR))
                         );
            if(!fSuccess) {
                DbgOut("SetupDiSetDeviceRegistryProperty failed!");                   
                break;
            }

            //
            // Inform setup about property change so that it can
            // restart the device.
            //

            spDevInstall.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
     
            if (Params && SetupDiGetDeviceInstallParams(Params->DeviceInfoSet,
                                              Params->DeviceInfoData,
                                              &spDevInstall)) {
                //
                // If your device requires a reboot to restart, you can
                // specify that by setting DI_NEEDREBOOT as shown below
                //
                // if(NeedReboot) {
                //    spDevInstall.Flags |= DI_PROPERTIES_CHANGE | DI_NEEDREBOOT;
                // }
                //
                spDevInstall.FlagsEx |= DI_FLAGSEX_PROPCHANGE_PENDING;
                
                SetupDiSetDeviceInstallParams(Params->DeviceInfoSet,
                                              Params->DeviceInfoData,
                                              &spDevInstall);
            }
        
        }
        return TRUE;

    default:
        return FALSE;
    }
    return FALSE;   
} 

