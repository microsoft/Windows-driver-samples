/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    toastco.c

Abstract:

    Device-specific co-installer for the Toaster Package sample.

--*/

#include "precomp.h"
#pragma hdrstop

//
// Constants
//
#define TOASTAPP_SETUP_SUBDIR  L"ToastApp"
#define TOASTAPP_SETUP_EXE     L"\\setup.exe"
#define TOASTAPP_SETUP_PATH    (TOASTAPP_SETUP_SUBDIR TOASTAPP_SETUP_EXE)

#define TOASTER_MEDIA_SOURCE_ID   1

//
// Globals
//
HMODULE g_hInstance;
WCHAR   SetupExeName[] = TOASTAPP_SETUP_EXE;

//
// Structures
//
typedef struct _VALUEADDWIZDATA {
    BOOL  AppInstallAttempted;          // Have we previously attempted to install app?
    WCHAR MediaRootDirectory[MAX_PATH]; // Fully-qualified path to root of install media
    WCHAR MediaDiskName[LINE_LEN];      // Name of media to prompt for (or empty string)
    WCHAR MediaTagFile[MAX_PATH];       // Tagfile identifying removable media (or empty string)
} VALUEADDWIZDATA, *LPVALUEADDWIZDATA;

//
// Function prototypes
//
INT_PTR
CALLBACK
ValueAddDlgProc(
    _In_ HWND     hwndDlg,
    _In_ UINT     uMsg,
    _In_ WPARAM   wParam,
    _In_ LPARAM   lParam
    );

BOOL
InstallToastApp(
    _In_ HWND    hwndDlg,
    _In_ LPCWSTR MediaRootDirectory
    );

UINT
ValueAddPropSheetPageProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ LPPROPSHEETPAGE ppsp
    );

_Success_(return == TRUE)
BOOL
GetMediaRootDirectory(
    _In_            HDEVINFO            DeviceInfoSet,
    _In_            PSP_DEVINFO_DATA    DeviceInfoData,
    _Out_           LPWSTR              *MediaRootDirectory,
    _Outptr_result_maybenull_ LPWSTR              *MediaDiskName,
    _Outptr_result_maybenull_ LPWSTR              *MediaTagFile
    );

HPROPSHEETPAGE
GetValueAddSoftwareWizPage(
    _In_ LPCWSTR MediaRootDirectory,
    _In_opt_ LPCWSTR MediaDiskName,
    _In_opt_ LPCWSTR MediaTagFile
    );

VOID
SetDeviceFriendlyName(
    _In_ HDEVINFO         DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData
    );

//
// Implementation
//

BOOL WINAPI
DllMain(
    _In_ HINSTANCE hInstDll,
    _In_ DWORD Reason,
    _In_ LPVOID Reserved
    )

/*++

Routine Description:

    Initialization/de-initialization entry point for toastco.dll

Arguments:

    hInstDll - Supplies handle to the DLL module

    Reason - Supplies the reason for calling the function

    pctx - Reserved

Return Value:

    This function always returns TRUE.

--*/

{
    UNREFERENCED_PARAMETER(Reserved);

    switch(Reason) {

        case DLL_PROCESS_ATTACH:
            g_hInstance = hInstDll;
            break;

        case DLL_PROCESS_DETACH:
            g_hInstance = NULL;
            break;

        default:
            break;
    }

    return TRUE;
}


DWORD CALLBACK
ToasterCoInstaller(
    _In_     DI_FUNCTION               InstallFunction,
    _In_     HDEVINFO                  DeviceInfoSet,
    _In_     PSP_DEVINFO_DATA          DeviceInfoData   OPTIONAL,
    _Inout_ PCOINSTALLER_CONTEXT_DATA Context
    )

/*++

Routine Description:

    This function acts as a device-specific co-installer for "toaster" devices.

Arguments:

    InstallFunction - Specifies the device installer function code indicating
        the action being performed.

    DeviceInfoSet - Supplies a handle to the device information set being
        acted upon by this install action.

    DeviceInfoData - Optionally, supplies the address of a device information
        element being acted upon by this install action.

    Context - Supplies the installation context that is per-install request/
        per-coinstaller.

Return Value:

    If this function successfully completed the requested action (or did
        nothing) and wishes for the installation to continue, the return value
        is NO_ERROR.

    If this function successfully completed the requested action (or did
        nothing) and would like to be called back once installation has
        completed, the return value is ERROR_DI_POSTPROCESSING_REQUIRED.

    If an error occurred while attempting to perform the requested action, a
        Win32 error code is returned.  The install action will be aborted.

--*/

{
    SP_NEWDEVICEWIZARD_DATA NewDeviceWizardData;
    HKEY hKey;
    DWORD Err;
    DWORD RegDataType;
    DWORD RequiredSize;
    DWORD UserPrompted;
    PWSTR MediaRootDirectory, MediaDiskName, MediaTagFile;

    UNREFERENCED_PARAMETER( Context );

    switch(InstallFunction) {

        case DIF_INSTALLDEVICE:
            //
            // In version 1 of our coinstaller, we had a hard-coded format
            // for the device's FriendlyName.  This wasn't a localizable
            // solution.  Now, we retrieve a (localized) format string from
            // the device INF.
            //
            // We should always get called with a valid DeviceInfoData, but
            // just to be sure...
            //
            if(DeviceInfoData != NULL) {
                SetDeviceFriendlyName(DeviceInfoSet, DeviceInfoData);
            }

            break;

        case DIF_NEWDEVICEWIZARD_FINISHINSTALL:
            //
            // We should always get called with a valid DeviceInfoData, but
            // just in case we don't we will bail out right away.
            //
            if(DeviceInfoData == NULL) {
                break;
            }

            //
            // Only supply a finish-install wizard page the first time...
            //
            if(ERROR_SUCCESS != RegOpenKeyEx(
                                    HKEY_LOCAL_MACHINE,
                                    TEXT("SOFTWARE\\Microsoft\\Toaster"),
                                    0,
                                    KEY_READ,
                                    &hKey)) {
                //
                // If we can't open this key, then we can't ascertain whether
                // or not the user was previously prompted to select value-add
                // software.  Assume they haven't been (i.e., this is the first
                // time, so the key doesn't exist yet).
                //
                UserPrompted = 0;

            } else {
                //
                // Look for non-zero "User Prompted" value entry to indicate
                // that the user has previously responded to question about
                // installation of value-add software.
                //
                RequiredSize = sizeof(UserPrompted);

                Err = RegQueryValueEx(hKey,
                                      TEXT("User Prompted"),
                                      NULL,
                                      &RegDataType,
                                      (PBYTE)&UserPrompted,
                                      &RequiredSize
                                     );

                if(Err != ERROR_SUCCESS) {
                    UserPrompted = 0;
                }

                RegCloseKey(hKey);
            }

            if(UserPrompted) {
                //
                // We asked the user this question before--don't bother them
                // again.
                //
                break;
            }

            //
            // It's possible that we could return the handle of our finish
            // install wizard page, yet it might never be used (e.g., if we're
            // in a server-side installation).  Thus, we won't set our
            // "User Prompted" registry flag just yet.  We'll wait until the
            // user actually sees this page before setting the flag.
            //

            ZeroMemory(&NewDeviceWizardData, sizeof(NewDeviceWizardData));
            NewDeviceWizardData.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);

            if(SetupDiGetClassInstallParams(DeviceInfoSet,
                                            DeviceInfoData,
                                            (PSP_CLASSINSTALL_HEADER)&NewDeviceWizardData,
                                            sizeof(SP_NEWDEVICEWIZARD_DATA),
                                            NULL)) {
                //
                // First, make sure there's room for us to add a page...
                //
                if(NewDeviceWizardData.NumDynamicPages >= MAX_INSTALLWIZARD_DYNAPAGES) {
                    break;
                }

                //
                // Retrieve the location of the source media based on the INF
                // we're installing from.
                //
                if(!GetMediaRootDirectory(DeviceInfoSet,
                                          DeviceInfoData,
                                          &MediaRootDirectory,
                                          &MediaDiskName,
                                          &MediaTagFile)) {
                    //
                    // We couldn't figure out where the source media is, so we
                    // can't offer any value-added software to the user.
                    //
                    break;
                }

                NewDeviceWizardData.DynamicPages[NewDeviceWizardData.NumDynamicPages] =
                    GetValueAddSoftwareWizPage(MediaRootDirectory,
                                               MediaDiskName,
                                               MediaTagFile
                                              );

                //
                // We don't need the media strings any more.
                //
                GlobalFree(MediaRootDirectory);

                if(MediaDiskName) {
                    GlobalFree(MediaDiskName);
                }

                if(MediaTagFile) {
                    GlobalFree(MediaTagFile);
                }

                if(NewDeviceWizardData.DynamicPages[NewDeviceWizardData.NumDynamicPages] != NULL) {
                    NewDeviceWizardData.NumDynamicPages++;
                }

                SetupDiSetClassInstallParams(DeviceInfoSet,
                                             DeviceInfoData,
                                             (PSP_CLASSINSTALL_HEADER)&NewDeviceWizardData,
                                             sizeof(SP_NEWDEVICEWIZARD_DATA)
                                            );
            }

            break;

        default:
            break;
    }

    return NO_ERROR;
}


BOOL
InstallToastApp(
    _In_ HWND    hwndWizard,
    _In_ LPCWSTR FullSetupPath
    )

/*++

Routine Description:

    This routine hides the wizard, kicks off the ToastApp setup program, then
    unhides the wizard when the ToastApp setup process terminates.

Arguments:

    hwndWizard - Handle to the wizard window to be hidden.

    FullSetupPath - Supplies the path to the setup program to be launched.

Return Value:

    If the setup app was successfully launched, the return value is TRUE.
    Otherwise, it is FALSE.

--*/

{
    BOOL b;
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInformation;

    //
    // Hide our wizard for the duration of the Toaster app's installation...
    //
    ShowWindow(hwndWizard, SW_HIDE);
    GetStartupInfo(&StartupInfo);

    b = CreateProcess(FullSetupPath,
                      NULL,
                      NULL,
                      NULL,
                      FALSE,
                      DETACHED_PROCESS | NORMAL_PRIORITY_CLASS,
                      NULL,
                      NULL,
                      &StartupInfo,
                      &ProcessInformation
                     );

    if(b) {
        //
        // Don't need a handle to the thread...
        //
        CloseHandle(ProcessInformation.hThread);

        //
        // ...but we _do_ want to wait on the process handle.
        //
        WaitForMultipleObjects(1, &ProcessInformation.hProcess, FALSE, INFINITE);

        CloseHandle(ProcessInformation.hProcess);
    }

    //
    // Now show our wizard once again...
    //
    ShowWindow(hwndWizard, SW_SHOW);

    return b;
}


UINT
ValueAddPropSheetPageProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ LPPROPSHEETPAGE ppsp
    )

/*++

Routine Description:

    This function is the property sheet page procedure, used to free the
    context data associated with the page when it is released.

Arguments:

    hwnd - Supplies a handle to the property page window

    uMsg - Supplies the message identifying the action being taken

    ppsp - Supplies the PROPSHEETPAGE structure for our page

Return Value:

    This routine always return non-zero (1).

--*/

{
    UNREFERENCED_PARAMETER(hwnd);

    switch(uMsg) {

        case PSPCB_RELEASE :
            GlobalFree((LPVALUEADDWIZDATA)(ppsp->lParam));
            break;

        default :
            break;
    }

    return 1; // let the page be created (return ignored on page release)
}


INT_PTR
CALLBACK
ValueAddDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )

/*++

Routine Description:

    This function is the dialog procedure for the value-add software selection
    wizard page.  If the user selects any software on this page, the software
    will be automatically installed when the user presses "Next".

Arguments:

    hwndDlg - Supplies a handle to the dialog box window

    uMsg - Supplies the message

    wParam - Supplies the first message parameter

    lParam - Supplies the second message parameter

Return Value:

    This dialog procedure always returns zero.

--*/

{
    LPVALUEADDWIZDATA pdata;
    LPNMHDR lpnm;
    HKEY hKey;

    UNREFERENCED_PARAMETER( wParam );

    //
    // Retrieve the shared user data from GWL_USERDATA
    //
    pdata = (LPVALUEADDWIZDATA) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

    switch(uMsg) {

        case WM_INITDIALOG :
            //
            // Get the PROPSHEETPAGE lParam value and load it into GWL_USERDATA
            //
            pdata = (LPVALUEADDWIZDATA) ((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)pdata);
            break;

        case WM_NOTIFY :

            lpnm = (LPNMHDR)lParam;

            switch(lpnm->code) {

                case PSN_SETACTIVE :
                    //
                    // Enable the Next and Back buttons
                    //
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
                    break;

                case PSN_WIZNEXT :
                    //
                    // Install any applications the user selected.
                    //
                    if(IsDlgButtonChecked(hwndDlg, IDC_CHECK1) && !pdata->AppInstallAttempted) {

                        BOOL MediaPresent = FALSE;
                        WCHAR PathToSetupExe[MAX_PATH];
                        PWSTR LastChar;

                        //
                        // If we need to prompt the user for media, do so now.
                        //
                        if(*(pdata->MediaDiskName)) {

                            WCHAR TempString[64];
                            size_t PathLength;

                            if(!LoadString(g_hInstance,
                                           IDS_MEDIA_PROMPT_TITLE,
                                           TempString,
                                           sizeof(TempString) / sizeof(WCHAR))) {

                                *TempString = TEXT('\0');
                            }

                            //
                            // Append subdirectory where toastapp's setup.exe
                            // is located, so we can prompt user for media.
                            //
                            if (FAILED(StringCchLength(pdata->MediaRootDirectory,
                                                        MAX_PATH,
                                                        &PathLength))) {
                                break;
                            }

                            LastChar = pdata->MediaRootDirectory + PathLength;

                            if(FAILED(StringCchCopy(LastChar,
                                                    MAX_PATH - PathLength,
                                                    TOASTAPP_SETUP_SUBDIR))) {
                                break;
                            }

                            //
                            // (Note, we skip the first character in
                            // SetupExeName for our "FileSought" argument
                            // below, because we don't want to include the
                            // first character, which is a path separator.)
                            //
                            if(DPROMPT_SUCCESS == SetupPromptForDisk(
                                                      GetParent(hwndDlg),
                                                      TempString,
                                                      pdata->MediaDiskName,
                                                      pdata->MediaRootDirectory,
                                                      SetupExeName+1,
                                                      pdata->MediaTagFile,
                                                      IDF_CHECKFIRST | IDF_NOBEEP,
                                                      PathToSetupExe,
                                                      MAX_PATH,
                                                      (PDWORD)&PathLength))
                            {
                                MediaPresent = TRUE;
                            }

                            //
                            // Strip the ToastApp subdir off media root path
                            //
                            *LastChar = L'\0';

                            if(MediaPresent) {
                                //
                                // SetupPromptForDisk gives us the directory
                                // where our setup program is located--now we
                                // need to append the setup program onto the
                                // end of that path.
                                //
                                PathLength--; // Don't include terminating null

                                if((PathToSetupExe[PathLength-1] != L'\\') &&
                                   (PathToSetupExe[PathLength-1] != L'/')) {
                                    //
                                    // We need the path separator char...
                                    //
                                    if(FAILED(StringCchCopy(PathToSetupExe+PathLength,
                                                            MAX_PATH - PathLength,
                                                            SetupExeName))) {
                                        break;
                                    }

                                 } else {
                                    //
                                    // We don't need the path separator char...
                                    //
                                    if(FAILED(StringCchCopy(PathToSetupExe+PathLength,
                                                            MAX_PATH - PathLength,
                                                            SetupExeName+1))) {
                                        break;
                                    }

                                }
                            }

                        } else {

                            //
                            // Assume media is already present (i.e., because
                            // we're in our auto-launch setup program running
                            // off the media.
                            //
                            MediaPresent = TRUE;

                            //
                            // Construct the fully-qualified path to the setup
                            // executable.
                            //
                            if(FAILED(StringCchCopy(PathToSetupExe,
                                                    MAX_PATH,
                                                    pdata->MediaRootDirectory))) {
                                break;
                            }

                            if(FAILED(StringCchCat(PathToSetupExe,
                                                    MAX_PATH,
                                                    TOASTAPP_SETUP_PATH))) {
                                break;
                            }

                        }

                        if(MediaPresent) {
                            //
                            // We're attempting app install.  Success or
                            // failure, we don't want to try again.
                            //
                            pdata->AppInstallAttempted = TRUE;

                            if(!InstallToastApp(GetParent(hwndDlg), PathToSetupExe)) {
                                //
                                // We failed to install the toast app. Un-check
                                // the checkbox before we disable it.
                                //
                                CheckDlgButton(hwndDlg, IDC_CHECK1, BST_UNCHECKED);
                            }

                            EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK1), FALSE);
                        }
                    }

                    break;

                case PSN_KILLACTIVE :
                    //
                    // If we get to this point, we know that our wizard page
                    // has been displayed.  We can now set the "User Prompted"
                    // registry flag, certain that the user has been given the
                    // opportunity to select the value-added software they wish
                    // to install.
                    //
                    if(ERROR_SUCCESS == RegCreateKeyEx(
                                            HKEY_LOCAL_MACHINE,
                                            TEXT("SOFTWARE\\Microsoft\\Toaster"),
                                            0,
                                            NULL,
                                            REG_OPTION_NON_VOLATILE,
                                            KEY_READ | KEY_WRITE,
                                            NULL,
                                            &hKey,
                                            NULL)) {

                        DWORD UserPrompted = 1;

                        RegSetValueEx(hKey,
                                      TEXT("User Prompted"),
                                      0,
                                      REG_DWORD,
                                      (PBYTE)&UserPrompted,
                                      sizeof(UserPrompted)
                                     );

                        RegCloseKey(hKey);
                    }

                    break;

                case PSN_WIZBACK :
                    //Handle a Back button click, if necessary
                    break;

                case PSN_RESET :
                    //Handle a Cancel button click, if necessary
                    break;

                default :
                    break;
            }

            break;

        default:
            break;
    }

    return 0;
}

_Success_(return == TRUE)
BOOL
GetMediaRootDirectory(
    _In_            HDEVINFO            DeviceInfoSet,
    _In_            PSP_DEVINFO_DATA    DeviceInfoData,
    _Out_           LPWSTR              *MediaRootDirectory,
    _Outptr_result_maybenull_ LPWSTR              *MediaDiskName,
    _Outptr_result_maybenull_ LPWSTR              *MediaTagFile
    )

/*++

Routine Description:

    This function retrieves the root of the installation media for the INF
    selected in the specified device information element.

    There are two possibilities here:

        1.  INF is on source media.  If so, then just use that path.

        2.  INF is already in %windir%\Inf.  This is less likely, because we
            should've previously prompted the user to install software at the
            time the INF was installed.  However, perhaps someone called
            SetupCopyOEMInf to install the INF without prompting the user for
            value-add software selection.  Another way you could get into this
            state is if the user previously elected to install the application,
            then subsequently uninstalled it via "Add/Remove Programs". The MSI
            package is configured to delete the UserPrompted value from the
            registry during uninstall, so if a new toaster is subsequently
            inserted, the user will be prompted once again.

            When the INF is in %windir%\Inf, we need to retrieve the original
            source location from which the INF was installed.  Plug&Play stores
            this information for 3rd-party INF files, but it is not directly
            accessible.  Fortunately, there is an INF DIRID that corresponds to
            this path, so we retrieve the value of that DIRID from the INF in
            order to ascertain the media root directory.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        the element for which an INF driver node is currently selected.

    DeviceInfoData - Supplies the address of a device information element for
        which an INF driver node is currently selected.

    MediaRootDirectory - Supplies the address of a string pointer that, upon
        successful return, will be set to point to a newly-allocated string
        containing the root directory of the setup media.  The caller must free
        this buffer via GlobalFree.

        This pointer will be set to NULL upon error.

    MediaDiskName - Supplies the address of a string pointer that, upon
        successful return will be set to either:

            1. A newly-allocated string containing the disk name to be used
               when prompting for the setup media (when INF is in %windir%\Inf)
            2. NULL (when INF isn't in %windir%\Inf it is presumed to be on
               source media, hence no prompting is necessary)

        This pointer will be set to NULL upon error.

    MediaTagFile - Supplies the address of a string pointer that, upon
        successful return will be set to either:

            1. A newly-allocated string containing the disk tagfile to be used
               when prompting for the setup media (when INF is in %windir%\Inf)
            2. NULL (when INF isn't in %windir%\Inf it is presumed to be on
               source media, hence no prompting is necessary)

        This pointer will be set to NULL upon error.

Return Value:

    If this function succeeds, the return value is non-zero (TRUE).

    If this function fails, the return value is FALSE.

--*/

{
    SP_DRVINFO_DATA DriverInfoData;
    PSP_DRVINFO_DETAIL_DATA DriverInfoDetailData = NULL;
    LPWSTR FileNamePart;
    WCHAR InfDirPath[MAX_PATH];
    BOOL b = FALSE;
    HINF hInf = INVALID_HANDLE_VALUE;
    INFCONTEXT InfContext;
    DWORD PathLength;

    *MediaRootDirectory = NULL;
    *MediaDiskName = NULL;
    *MediaTagFile = NULL;

    //
    // First, retrieve the full path of the INF being used to install this
    // device.
    //
    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);

    if(!SetupDiGetSelectedDriver(DeviceInfoSet, DeviceInfoData, &DriverInfoData)) {
        //
        // This shouldn't fail, but if it does, just bail.
        //
        goto clean0;
    }

    //
    // Retrieve the driver info details.  We don't care about the id list at
    // the end, so we can just allocate a buffer for the fixed-size part...
    //
    DriverInfoDetailData = GlobalAlloc(0, sizeof(SP_DRVINFO_DETAIL_DATA));
    if(!DriverInfoDetailData) {
        goto clean0;
    }
    DriverInfoDetailData->cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);

    if(!SetupDiGetDriverInfoDetail(DeviceInfoSet,
                                   DeviceInfoData,
                                   &DriverInfoData,
                                   DriverInfoDetailData,
                                   sizeof(SP_DRVINFO_DETAIL_DATA),
                                   NULL)
       && (GetLastError() != ERROR_INSUFFICIENT_BUFFER))
    {
        //
        // Again, this should never fail, but if it does we're outta here.
        //
        goto clean0;
    }

    *MediaRootDirectory = GlobalAlloc(0, MAX_PATH * sizeof(WCHAR));
    if(!*MediaRootDirectory) {
        goto clean0;
    }

    //
    // Strip the INF name off of the path.  (Resultant path will always end
    // with a path separator char ('\\').)
    //
    PathLength = GetFullPathName(DriverInfoDetailData->InfFileName,
                                 MAX_PATH,
                                 *MediaRootDirectory,
                                 &FileNamePart
                                );

    if(!PathLength || (PathLength >= MAX_PATH)) {
        goto clean0;
    }

    *FileNamePart = L'\0';

    //
    // Check to see this INF is already in %windir%\Inf.
    //
    PathLength = GetSystemWindowsDirectory(InfDirPath, MAX_PATH);

    if(!PathLength || (PathLength >= MAX_PATH)) {
        goto clean0;
    }

    //
    // Append INF directory to path (making sure we don't end up with two path
    // separator chars).
    //
    if((InfDirPath[PathLength-1] != L'\\') && (InfDirPath[PathLength-1] != L'/')) {
        if(FAILED(StringCchCopy(&(InfDirPath[PathLength]),
                                MAX_PATH - PathLength,
                                L"\\Inf\\"))) {
            goto clean0;
        }
    } else {

        if(FAILED(StringCchCopy(&(InfDirPath[PathLength]),
                                MAX_PATH - PathLength,
                                L"Inf\\"))) {
            goto clean0;
        }
    }

    if(lstrcmpi(*MediaRootDirectory, InfDirPath)) {
        //
        // The INF isn't in %windir%\Inf, so assume its location is the root of
        // the installation media.  (We don't bother to retrieve the disk name
        // or tagfile name in this case.)
        //
        b = TRUE;
        goto clean0;
    }

    //
    // Since the INF is already in %windir%\Inf, we need to find out where it
    // originally came from.  There is no direct way to ascertain an INF's
    // path of origin, but we can indirectly determine it by retrieving a field
    // from our INF that uses a string substitution of %1% (DIRID_SRCPATH).
    //
    hInf = SetupOpenInfFile(DriverInfoDetailData->InfFileName,
                            NULL,
                            INF_STYLE_WIN4,
                            NULL
                           );

    if(hInf == INVALID_HANDLE_VALUE) {
        goto clean0;
    }

    //
    // Contained within our INF should be a [ToastCoInfo] section with the
    // following entry:
    //
    //     OriginalInfSourcePath = %1%
    //
    // If we retrieve the value (i.e., field 1) of this line, we'll get the
    // full path where the INF originally came from.
    //
    if(!SetupFindFirstLine(hInf, L"ToastCoInfo", L"OriginalInfSourcePath", &InfContext)) {
        goto clean0;
    }

    if(!SetupGetStringField(&InfContext, 1, *MediaRootDirectory, MAX_PATH, &PathLength) ||
       (PathLength <= 1)) {
        goto clean0;
    }

    //
    // PathLength we get back includes the terminating null character. Subtract
    // one to get actual length of string.
    //
    PathLength--;

    //
    // Ensure the path we retrieved has a path separator character at the end.
    //
    if(((*MediaRootDirectory)[PathLength-1] != L'\\') &&
       ((*MediaRootDirectory)[PathLength-1] != L'/'))
    {
        if(FAILED(StringCchCopy(*MediaRootDirectory+PathLength,
                                MAX_PATH - PathLength,
                                L"\\"))) {
            goto clean0;
        }
    }

    //
    // Now retrieve the disk name and tagfile for our setup media.
    //
    *MediaDiskName = GlobalAlloc(0, LINE_LEN * sizeof(WCHAR));
    *MediaTagFile = GlobalAlloc(0, MAX_PATH * sizeof(WCHAR));

    if(!(*MediaDiskName && *MediaTagFile)) {
        goto clean0;
    }

    if(!SetupGetSourceInfo(hInf,
                           TOASTER_MEDIA_SOURCE_ID,
                           SRCINFO_DESCRIPTION,
                           *MediaDiskName,
                           LINE_LEN,
                           NULL)) {
        goto clean0;
    }

    if(!SetupGetSourceInfo(hInf,
                           TOASTER_MEDIA_SOURCE_ID,
                           SRCINFO_TAGFILE,
                           *MediaTagFile,
                           MAX_PATH,
                           NULL)) {
        goto clean0;
    }

    b = TRUE;

clean0:

    if(hInf != INVALID_HANDLE_VALUE) {
        SetupCloseInfFile(hInf);
    }

    if(DriverInfoDetailData) {
        GlobalFree(DriverInfoDetailData);
    }

    if(!b) {
        if(*MediaRootDirectory) {
            GlobalFree(*MediaRootDirectory);
            *MediaRootDirectory = NULL;
        }
        if(*MediaDiskName) {
            GlobalFree(*MediaDiskName);
            *MediaDiskName = NULL;
        }
        if(*MediaTagFile) {
            GlobalFree(*MediaTagFile);
            *MediaTagFile = NULL;
        }
    }

    return b;
}


HPROPSHEETPAGE
GetValueAddSoftwareWizPage(
    _In_ LPCWSTR MediaRootDirectory,
    _In_opt_ LPCWSTR MediaDiskName,
    _In_opt_ LPCWSTR MediaTagFile
    )

/*++

Routine Description:

    This function returns a newly-created property sheet page handle that may
    be used in a wizard to allow user-selection of value-added software.

    This wizard page is used by both the toaster co-installer (as a finish-
    install wizard page), as well as by the toastva installation application.

Arguments:

    MediaRootDirectory - Supplies the fully-qualified path to the root of the
        installation media.

    MediaDiskName - Optionally, supplies the name of the disk to be used when
        prompting the user for source media.  If this parameter is not
        supplied, the media is assumed to already be present, and no prompting
        occurs

    MediaTagFile - Optionally, supplies the tagfile for the disk to be used
        when prompting the user for source media.  If MediaDiskName is not
        specified, this parameter is ignored.

Return Value:

    If this function succeeds, the return value is a newly-created property
    sheet page handle.

    If this function fails, the return value is NULL.

--*/

{
    HPROPSHEETPAGE hpsp;
    LPVALUEADDWIZDATA ValueAddWizData; //data for the value-add sw chooser page
    PROPSHEETPAGE page;

    ValueAddWizData = GlobalAlloc(0, sizeof(VALUEADDWIZDATA));

    if(ValueAddWizData) {
        ZeroMemory(ValueAddWizData, sizeof(VALUEADDWIZDATA));
    } else {
        return NULL;
    }

    if(FAILED(StringCchCopy(ValueAddWizData->MediaRootDirectory,
                            MAX_PATH,
                            MediaRootDirectory))) {
        return NULL;
    }

    if(MediaDiskName && MediaTagFile) {
        //
        // The caller wants us to prompt user for media (e.g., installation
        // occurring from %windir%\Inf\OEM<n>.INF, and we want to ensure that
        // our CD is in the drive before launching setup.exe from it).
        //
        if(FAILED(StringCchCopy(ValueAddWizData->MediaDiskName,
                                LINE_LEN,
                                MediaDiskName))) {
            return NULL;
        }

        if(FAILED(StringCchCopy(ValueAddWizData->MediaTagFile,
                                MAX_PATH,
                                MediaTagFile))) {
            return NULL;
        }
    }

    ZeroMemory(&page, sizeof(PROPSHEETPAGE));

    //
    // Create the sample Wizard Page
    //
    page.dwSize =            sizeof(PROPSHEETPAGE);
    page.dwFlags =           PSP_DEFAULT|PSP_USEHEADERTITLE|PSP_USEHEADERSUBTITLE|PSP_USETITLE|PSP_USECALLBACK;
    page.hInstance =         g_hInstance;
    page.pszHeaderTitle =    MAKEINTRESOURCE(IDS_TITLE);
    page.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_SUBTITLE);
    page.pszTemplate =       MAKEINTRESOURCE(IDD_SAMPLE_INSTALLAPP);
    page.pfnDlgProc =        ValueAddDlgProc;
    page.lParam =            (LPARAM)ValueAddWizData;
    page.pfnCallback =       (LPFNPSPCALLBACKW) ValueAddPropSheetPageProc;

    hpsp = CreatePropertySheetPage(&page);

    if(!hpsp) {
        GlobalFree(ValueAddWizData);
    }

    return hpsp;
}


VOID
SetDeviceFriendlyName(
    _In_ HDEVINFO         DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData
    )

/*++

Routine Description:

    This function retrieves the (localized) string format (suitable for use
    with FormatMessage) to be used in generating the device's friendly name,
    then constructs the name using that format in concert with the device's UI
    number.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        the element for which an INF driver node is currently selected.

    DeviceInfoData - Supplies the address of a device information element for
        which an INF driver node is currently selected.

Return Value:

    none

--*/

{
    SP_DRVINFO_DATA DriverInfoData;
    SP_DRVINFO_DETAIL_DATA DriverInfoDetailData;
    HINF hInf;
    WCHAR InfSectionWithExt[255];
    WCHAR FormatString[LINE_LEN];
    INFCONTEXT InfContext;
    DWORD UINumber;
    PVOID FriendlyNameBuffer;
    DWORD FriendlyNameBufferSize;

    //
    // First, retrieve the format string from the driver's [DDInstall] section.
    //
    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    if(!SetupDiGetSelectedDriver(DeviceInfoSet,
                                 DeviceInfoData,
                                 &DriverInfoData)) {
        //
        // NULL driver install
        //
        goto clean0;
    }

    DriverInfoDetailData.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
    if(!SetupDiGetDriverInfoDetail(DeviceInfoSet,
                                   DeviceInfoData,
                                   &DriverInfoData,
                                   &DriverInfoDetailData,
                                   sizeof(DriverInfoDetailData),
                                   NULL) &&
       (GetLastError() != ERROR_INSUFFICIENT_BUFFER))
    {
        //
        // Unable to retrieve detail info about selected driver node
        //
        goto clean0;
    }

    hInf = SetupOpenInfFile(DriverInfoDetailData.InfFileName,
                            NULL,
                            INF_STYLE_WIN4,
                            NULL
                           );

    if(hInf == INVALID_HANDLE_VALUE) {
        //
        // Couldn't open the INF
        //
        goto clean0;
    }

    //
    // Figure out actual (potentially decorated) DDInstall section being used
    // for this install.
    //
    *FormatString = L'\0';  // default to empty string in case error occurs.

    if(SetupDiGetActualSectionToInstall(hInf,
                                        DriverInfoDetailData.SectionName,
                                        InfSectionWithExt,
                                        sizeof(InfSectionWithExt) / sizeof(WCHAR),
                                        NULL,
                                        NULL)) {

        if(SetupFindFirstLine(hInf,
                              InfSectionWithExt,
                              L"FriendlyNameFormat",
                              &InfContext)) {

            if(!SetupGetStringField(&InfContext,
                                    1,
                                    FormatString,
                                    sizeof(FormatString) / sizeof(WCHAR),
                                    NULL)) {
                //
                // Failed to retrieve format string into our buffer.  Make sure
                // our buffer still contains an empty string.
                //
                *FormatString = L'\0';
            }
        }
    }

    SetupCloseInfFile(hInf);

    if(!(*FormatString)) {
        goto clean0;
    }

    //
    // Now retrieve the device's UI number, which is actually the serial number
    // used by the bus driver.
    //
    if(SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                        DeviceInfoData,
                                        SPDRP_UI_NUMBER,
                                        NULL,
                                        (PBYTE)&UINumber,
                                        sizeof(UINumber),
                                        NULL)) {

        FriendlyNameBufferSize = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER
                                               | FORMAT_MESSAGE_FROM_STRING
                                               | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                               FormatString,
                                               0,
                                               0,
                                               (LPWSTR)&FriendlyNameBuffer,
                                               0,
                                               (va_list *)&UINumber
                                              );

        if(FriendlyNameBufferSize) {

            SetupDiSetDeviceRegistryProperty(DeviceInfoSet,
                                             DeviceInfoData,
                                             SPDRP_FRIENDLYNAME,
                                             (PBYTE)FriendlyNameBuffer,
                                             (FriendlyNameBufferSize + 1) * sizeof(WCHAR)
                                            );

            LocalFree(FriendlyNameBuffer);
        }
    }

clean0:
    ;   // nothing to do
}

