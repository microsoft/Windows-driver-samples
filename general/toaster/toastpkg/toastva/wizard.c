/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    wizard.c

Abstract:

    This module implements the TOASTVA wizard that installs/updates toaster
    drivers and allows the user to select additional value-added software.

--*/

#include "precomp.h"
#pragma hdrstop

//
// Constants
//
#define WMX_UPDATE_DRIVER_DONE (WM_USER + 500)

//
// Structures
//
typedef struct _SHAREDWIZDATA {
    HFONT   hTitleFont;          // Title font for the Welcome and Completion pages
    BOOL    HwInsertedFirst;     // Is the hardware already present?
    LPCWSTR MediaRootDirectory;  // Fully-qualified path to root of install media
    BOOL    DoDriverUpdatePage;  // Should Update Driver page do anything?
    BOOL    RebootRequired;      // Did we do anything that requires a reboot?
    HWND    hwndDlg;             // Handle to dialog notified by drv update thread
} SHAREDWIZDATA, *LPSHAREDWIZDATA;

//
// Function prototypes
//
INT_PTR
CALLBACK
IntroDlgProc(
    _In_ HWND     hwndDlg,
    _In_ UINT     uMsg,
    _In_ WPARAM   wParam,
    _In_ LPARAM   lParam
    );

INT_PTR
CALLBACK
IntPage1DlgProc(
    _In_ HWND     hwndDlg,
    _In_ UINT     uMsg,
    _In_ WPARAM   wParam,
    _In_ LPARAM   lParam
    );

INT_PTR
CALLBACK
EndDlgProc(
    _In_ HWND     hwndDlg,
    _In_ UINT     uMsg,
    _In_ WPARAM   wParam,
    _In_ LPARAM   lParam
    );

DWORD
WINAPI
UpdateDriverThreadProc(
    _In_ LPVOID ThreadData
    );

INT
CALLBACK
WizardCallback(
    _In_ HWND   hwndDlg,
    _In_ UINT   uMsg,
    _In_ LPARAM lParam
    );

//
// Prototype for a routine exported from toastco.dll
//
HPROPSHEETPAGE
GetValueAddSoftwareWizPage(
    _In_ LPCWSTR MediaRootDirectory,
    _In_opt_ LPCWSTR MediaDiskName,
    _In_opt_ LPCWSTR MediaTagFile
    );

//
// Implementation
//

VOID
DoValueAddWizard(
    _In_ LPCWSTR MediaRootDirectory
    )

/*++

Routine Description:

    This routine displays a wizard that steps the user through the following
    actions:

    (a) Performs a "driver update" for any currently-present toasters
    (b) Installs the INF and CAT in case no toasters presently exist
    (c) Optionally, installs value-add software selected by the user (this
        wizard page is retrieved from the toaster co-installer, and is the same
        page the user gets if they do a "hardware-first" installation using our
        driver).

Arguments:

    MediaRootDirectory - Supplies the fully-qualified path to the root
        directory where the installation media is located.

Return Value:

    none

--*/

{
    PROPSHEETPAGE   psp =       {0}; //defines the property sheet pages
    HPROPSHEETPAGE  ahpsp[4] =  {0}; //an array to hold the page's HPROPSHEETPAGE handles
    PROPSHEETHEADER psh =       {0}; //defines the property sheet
    SHAREDWIZDATA wizdata =     {0}; //the shared data structure

    NONCLIENTMETRICS ncm = {0};
    LOGFONT TitleLogFont;
    HDC hdc;
    INT FontSize;
    INT index = 0;
    HRESULT hr;

    //
    //Create the Wizard pages
    //
    // Intro page...
    //
    psp.dwSize =        sizeof(psp);
    psp.dwFlags =       PSP_DEFAULT|PSP_HIDEHEADER;
    psp.hInstance =     g_hInstance;
    psp.lParam =        (LPARAM) &wizdata; //The shared data structure
    psp.pfnDlgProc =    IntroDlgProc;
    psp.pszTemplate =   MAKEINTRESOURCE(IDD_INTRO);

    ahpsp[index++] =          CreatePropertySheetPage(&psp);

    //
    // Updating drivers page...
    //
    psp.dwFlags =           PSP_DEFAULT|PSP_USEHEADERTITLE|PSP_USEHEADERSUBTITLE|PSP_USETITLE;
    psp.pszHeaderTitle =    MAKEINTRESOURCE(IDS_TITLE1);
    psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_SUBTITLE1);
    psp.pszTemplate =       MAKEINTRESOURCE(IDD_INTERIOR1);
    psp.pfnDlgProc =        IntPage1DlgProc;

    ahpsp[index++] =              CreatePropertySheetPage(&psp);

    //
    // Retrieve the value-add software chooser page from the toaster
    // co-installer (toastco.dll).
    //
    ahpsp[index] = GetValueAddSoftwareWizPage(MediaRootDirectory, NULL, NULL);

    if(ahpsp[index]) {
        index++;
    }

    //
    // Finish page...
    //
    psp.dwFlags =       PSP_DEFAULT|PSP_HIDEHEADER;
    psp.pszTemplate =   MAKEINTRESOURCE(IDD_END);
    psp.pfnDlgProc =    EndDlgProc;

    ahpsp[index] =          CreatePropertySheetPage(&psp);

    //
    // Create the property sheet...
    //
    psh.dwSize =            sizeof(psh);
    psh.hInstance =         g_hInstance;
    psh.hwndParent =        NULL;
    psh.phpage =            ahpsp;
    psh.dwFlags =           PSH_WIZARD97|PSH_WATERMARK|PSH_HEADER|PSH_STRETCHWATERMARK|PSH_WIZARD|PSH_USECALLBACK;
    psh.pszbmWatermark =    MAKEINTRESOURCE(IDB_WATERMARK);
    psh.pszbmHeader =       MAKEINTRESOURCE(IDB_BANNER);
    psh.nStartPage =        0;
    psh.nPages =            4;
    psh.pfnCallback =       WizardCallback;

    //
    // Set up the font for the titles on the intro and ending pages
    //
    ncm.cbSize = sizeof(ncm);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

    //
    // Create the intro/end title font
    //
    TitleLogFont = ncm.lfMessageFont;
    TitleLogFont.lfWeight = FW_BOLD;
    hr = StringCchCopy(TitleLogFont.lfFaceName, LF_FACESIZE, L"Verdana Bold");
    if(SUCCEEDED(hr) == FALSE) {
        return; // assert
    }

    hdc = GetDC(NULL); //gets the screen DC
    FontSize = 12;
    TitleLogFont.lfHeight = 0 - GetDeviceCaps(hdc, LOGPIXELSY) * FontSize / 72;
    wizdata.hTitleFont = CreateFontIndirect(&TitleLogFont);
    ReleaseDC(NULL, hdc);
    wizdata.MediaRootDirectory = MediaRootDirectory;

    //
    // Display the wizard
    //
    PropertySheet(&psh);

    //
    // Destroy the fonts
    //
    DeleteObject(wizdata.hTitleFont);

    //
    // If we did anything that requires a reboot, prompt the user
    // now.  Note that we need to do this regardle
    //
    if(wizdata.RebootRequired) {
        SetupPromptReboot(NULL, NULL, FALSE);
    }
}


INT_PTR
CALLBACK
IntroDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )

/*++

Routine Description:

    This function is the dialog procedure for the Welcome page of the wizard.

Arguments:

    hwndDlg - Supplies a handle to the dialog box window

    uMsg - Supplies the message

    wParam - Supplies the first message parameter

    lParam - Supplies the second message parameter

Return Value:

    This dialog procedure always returns zero.

--*/

{
    LPSHAREDWIZDATA pdata;
    LPNMHDR lpnm;

    UNREFERENCED_PARAMETER( wParam );

    //
    // Retrieve the shared user data from GWL_USERDATA
    //
    pdata = (LPSHAREDWIZDATA) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

    switch(uMsg) {

        case WM_INITDIALOG :
            {
                HWND hwndControl;

                //
                // Get the shared data from PROPSHEETPAGE lParam valueand load
                // it into GWL_USERDATA
                //
                pdata = (LPSHAREDWIZDATA) ((LPPROPSHEETPAGE) lParam) -> lParam;

                SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pdata);

                //
                // It's an intro/end page, so get the title font from the
                // shared data and use it for the title control
                //
                hwndControl = GetDlgItem(hwndDlg, IDC_TITLE);
                SetWindowFont(hwndControl,pdata->hTitleFont, TRUE);
                break;
            }

        case WM_NOTIFY :

            lpnm = (LPNMHDR)lParam;

            switch(lpnm->code) {

                case PSN_SETACTIVE :
                    //
                    // Enable the Next button
                    //
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT);

                    //
                    // When we're moving forward through the wizard, we want
                    // the driver update page to do its work.
                    //
                    pdata->DoDriverUpdatePage = TRUE;
                    break;

                case PSN_WIZNEXT :
                    //Handle a Next button click here
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


INT_PTR
CALLBACK
IntPage1DlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )

/*++

Routine Description:

    This function is the dialog procedure for the first interior wizard page.
    This page updates the drivers for any existing (present) devices, or
    installs the INF if there aren't any present devices.

Arguments:

    hwndDlg - Supplies a handle to the dialog box window

    uMsg - Supplies the message

    wParam - Supplies the first message parameter

    lParam - Supplies the second message parameter

Return Value:

    This dialog procedure always returns zero.

--*/

{
    LPSHAREDWIZDATA pdata;
    LPNMHDR lpnm;
    HANDLE hThread;
    HKEY hKey;
    DWORD UserPrompted;

    UNREFERENCED_PARAMETER( wParam );

    //
    // Retrieve the shared user data from GWL_USERDATA
    //
    pdata = (LPSHAREDWIZDATA) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

    switch(uMsg) {

        case WM_INITDIALOG :
            //
            // Get the PROPSHEETPAGE lParam value and load it into GWL_USERDATA
            //
            pdata = (LPSHAREDWIZDATA) ((LPPROPSHEETPAGE) lParam) -> lParam;
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pdata);
            break;

        case WM_NOTIFY :

            lpnm = (LPNMHDR)lParam;

            switch(lpnm->code) {

                case PSN_SETACTIVE :
                    //
                    // If we're coming here from the intro page, then disable
                    // the Back and Next buttons (we're going to be busy for a
                    // little bit updating drivers).
                    //
                    // If we're coming to this page from anywhere else,
                    // immediately jump to the intro page.
                    //
                    if(pdata->DoDriverUpdatePage) {
                        //
                        // Reset our flag so that we won't try this again if we
                        // go to later pages, then come back to this one.  (We
                        // only do anything when the wizard page is accessed in
                        // the forward direction, from the Intro page.)
                        //
                        pdata->DoDriverUpdatePage = FALSE;

                        //
                        // Set our "UserPrompted" registry flag so the
                        // co-installer won't popup its own value-add software
                        // chooser page during driver update.
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

                            UserPrompted = 1;
                            RegSetValueEx(hKey,
                                          TEXT("User Prompted"),
                                          0,
                                          REG_DWORD,
                                          (PBYTE)&UserPrompted,
                                          sizeof(UserPrompted)
                                         );

                            RegCloseKey(hKey);
                        }

                        //
                        // Disable Next, Back, and Cancel
                        //
                        PropSheet_SetWizButtons(GetParent(hwndDlg), 0);
                        EnableWindow(GetDlgItem(GetParent(hwndDlg), IDCANCEL), FALSE);

                        //
                        // Show "searching" animation...
                        //
                        ShowWindow(GetDlgItem(hwndDlg, IDC_ANIMATE1), SW_SHOW);
                        Animate_Open(GetDlgItem(hwndDlg, IDC_ANIMATE1), MAKEINTRESOURCE(IDA_SEARCHING));
                        Animate_Play(GetDlgItem(hwndDlg, IDC_ANIMATE1), 0, -1, -1);

                        //
                        // Create a thread to do the work of updating the
                        // driver, etc.
                        //
                        pdata->hwndDlg = hwndDlg;

                        hThread = CreateThread(NULL,
                                               0,
                                               UpdateDriverThreadProc,
                                               pdata,
                                               0,
                                               NULL
                                              );

                        if(hThread) {
                            //
                            // Thread launched successfully--close the handle,
                            // then just wait to be notified of thread's
                            // completion.
                            //
                            CloseHandle(hThread);

                        } else {
                            //
                            // Couldn't launch the thread--just move on to the
                            // value-add software page.
                            //
                            PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT);
                            PropSheet_PressButton(GetParent(hwndDlg), PSBTN_NEXT);
                        }

                    } else {
                        //
                        // We're coming "back" to this page.  Skip it, and go
                        // to the intro page.
                        //
                        PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK);
                        PropSheet_PressButton(GetParent(hwndDlg), PSBTN_BACK);
                    }
                    break;

                case PSN_WIZNEXT :
                    //Handle a Next button click, if necessary
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

        case WMX_UPDATE_DRIVER_DONE :
            //
            // Stop "searching" animation...
            //
            Animate_Stop(GetDlgItem(hwndDlg, IDC_ANIMATE1));
            ShowWindow(GetDlgItem(hwndDlg, IDC_ANIMATE1), SW_HIDE);

            //
            // Regardless of whether we succeeded in upgrading any drivers, we'll
            // go ahead and proceed to the value-add software page.
            //
            PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT);
            EnableWindow(GetDlgItem(GetParent(hwndDlg), IDCANCEL), TRUE);

            PropSheet_PressButton(GetParent(hwndDlg), PSBTN_NEXT);

            break;

        default:
            break;
    }

    return 0;
}


INT_PTR
CALLBACK
EndDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )

/*++

Routine Description:

    This function is the dialog procedure for the Finish page of the wizard.

Arguments:

    hwndDlg - Supplies a handle to the dialog box window

    uMsg - Supplies the message

    wParam - Supplies the first message parameter

    lParam - Supplies the second message parameter

Return Value:

    This dialog procedure always returns zero.

--*/

{
    LPSHAREDWIZDATA pdata;
    LPNMHDR lpnm;

    UNREFERENCED_PARAMETER( wParam );

    //
    // Retrieve the shared user data from GWL_USERDATA
    //
    pdata = (LPSHAREDWIZDATA) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

    switch(uMsg) {

        case WM_INITDIALOG :
            {
                HWND hwndControl;

                //
                // Get the shared data from PROPSHEETPAGE lParam value and load
                // it into GWL_USERDATA
                //
                pdata = (LPSHAREDWIZDATA) ((LPPROPSHEETPAGE) lParam) -> lParam;
                SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pdata);

                //
                // It's an intro/end page, so get the title font from userdata
                // and use it on the title control
                //
                hwndControl = GetDlgItem(hwndDlg, IDC_TITLE);
                SetWindowFont(hwndControl,pdata->hTitleFont, TRUE);
                break;
            }

        case WM_NOTIFY :

            lpnm = (LPNMHDR)lParam;

            switch(lpnm->code) {

                case PSN_SETACTIVE :
                    //
                    // Enable the correct buttons for the active page
                    //
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_FINISH);

                    //
                    // Doesn't make sense to have Cancel button enabled here
                    //
                    EnableWindow(GetDlgItem(GetParent(hwndDlg),  IDCANCEL), FALSE);

                    //
                    // If we didn't find any currently-present devices, then prompt
                    // the user to insert their device now.
                    //
                    if(!pdata->HwInsertedFirst) {

                        WCHAR TempString[LINE_LEN];

                        if(LoadString(g_hInstance,
                                      IDS_PROMPT_FOR_HW,
                                      TempString,
                                      sizeof(TempString) / sizeof(WCHAR))) {

                            SetDlgItemText(hwndDlg, IDC_FINISH_TEXT, TempString);
                        }
                    }

                    break;

                case PSN_WIZBACK :
                    //
                    // Jumping back from this page, so turn Cancel button back on.
                    //
                    EnableWindow(GetDlgItem(GetParent(hwndDlg),  IDCANCEL), TRUE);
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


DWORD
WINAPI
UpdateDriverThreadProc(
    _In_ LPVOID ThreadData
    )

/*++

Routine Description:

    This function updates the drivers for any existing toasters.  If there are
    no toasters currently connected to the computer, it installs the INF/CAT so
    that the system will be ready to automatically install toasters that are
    plugged in later.  This routine will also mark any non-present (aka,
    "phantom") toasters as needs-reinstall, so that they'll be updated to the
    new driver if they're ever plugged in again.

Arguments:

    ThreadData - Supplies a pointer to a SHAREDWIZDATA structure that's used
        both by this thread, and by the wizard in the main thread.

Return Value:

    If successful, the function returns NO_ERROR.

    Otherwise, the function returns a Win32 error code indicating the cause of
    failure.

--*/

{
    DWORD Err;
    LPSHAREDWIZDATA pdata;
    WCHAR FullInfPath[MAX_PATH];
    HDEVINFO ExistingNonPresentDevices;


    pdata = (LPSHAREDWIZDATA)ThreadData;
    Err = NO_ERROR;

    //
    // First, attempt to update any present devices to our driver...
    //
    if (FAILED(StringCchCopy(FullInfPath, MAX_PATH, pdata->MediaRootDirectory))) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    if (FAILED(StringCchCat(FullInfPath, MAX_PATH, DEVICE_INF_NAME))) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    if(UpdateDriverForPlugAndPlayDevices(GetParent(pdata->hwndDlg),
                                         HW_ID_TO_UPDATE,
                                         FullInfPath,
                                         0,
                                         &pdata->RebootRequired)) {
        //
        // We know that at least one device existed, and was upgraded.
        //
        pdata->HwInsertedFirst = TRUE;

    } else {

        Err = GetLastError();

        //
        // We failed to update the driver.  If we failed simply because
        // there were no toasters currently attached to the computer, then
        // we still want to install the INF.
        //
        if(Err == ERROR_NO_SUCH_DEVINST) {

            pdata->HwInsertedFirst = FALSE;

            //
            // Since we didn't do any device installs, the INF (and CAT)
            // didn't get automatically installed.  We'll install them now,
            // so that they'll be present when the user subsequently plugs
            // their hardware in.
            //
            if(!SetupCopyOEMInf(FullInfPath,
                                NULL,
                                SPOST_PATH,
                                0,
                                NULL,
                                0,
                                NULL,
                                NULL)) {
                //
                // Failure to install the INF is more important (worse) than
                // the absence of any devices!
                //
                Err = GetLastError();
            }

        } else {
            //
            // Apparently there _were_ existing devices--we just failed to
            // upgrade their drivers.  This might be due to an installation
            // problem, or perhaps because the devices already have drivers
            // newer than the one we offered.
            //
            pdata->HwInsertedFirst = TRUE;
        }
    }

    if((Err == NO_ERROR) || (Err == ERROR_NO_SUCH_DEVINST)) {
        //
        // Either we successfully upgraded one or more toasters, or there were
        // no present toasters but we successfully installed our INF and CAT.
        //
        // There may exist, however, devices that were once connected to the
        // computer, but presently are not.  If such devices are connected
        // again in the future, we want to ensure they go through device
        // installation.  We will retrieve the list of non-present devices, and
        // mark each as "needs re-install" to kick them back through the "New
        // Hardware Found" process if they ever show up again.  (Note that this
        // doesn't destroy any device-specific settings they may have, so this
        // is just forcing an upgrade, not an uninstall/re-install.)
        //
        // (The HardwareID used is the one defined for the toaster sample,
        // BUS_HARDWARE_IDS in src\general\toaster\bus\common.h.  We also take
        // advantage of the fact that we know these devices will always be
        // enumerated under the "{b85b7c50-6a01-11d2-b841-00c04fad5171}"
        // enum namespace.)
        //
        ExistingNonPresentDevices = GetNonPresentDevices(ENUMERATOR_NAME,
                                                         HW_ID_TO_UPDATE
                                                        );

        if(ExistingNonPresentDevices != INVALID_HANDLE_VALUE) {

            MarkDevicesAsNeedReinstall(ExistingNonPresentDevices);

            SetupDiDestroyDeviceInfoList(ExistingNonPresentDevices);
        }
    }

    PostMessage(pdata->hwndDlg, WMX_UPDATE_DRIVER_DONE, 0, 0);

    return Err;
}


INT
CALLBACK
WizardCallback(
    _In_ HWND   hwndDlg,
    _In_ UINT   uMsg,
    _In_ LPARAM lParam
    )

/*++

Routine Description:

    Call back used to remove the "X" and  "?" from the wizard page.

Arguments:

    hwndDlg - Handle to the property sheet dialog box.

    uMsg - Identifies the message being received. This parameter is one of the
        following values:

        PSCB_INITIALIZED - Indicates that the property sheet is being
                           initialized. The lParam value is zero for this
                           message.

        PSCB_PRECREATE   - Indicates that the property sheet is about to be
                           created. The hwndDlg parameter is NULL and the
                           lParam parameter is a pointer to a dialog template
                           in memory. This template is in the form of a
                           DLGTEMPLATE structure followed by one or more
                           DLGITEMTEMPLATE structures.

    lParam - Specifies additional information about the message. The
        meaning of this value depends on the uMsg parameter.

Return Value:

    The function returns zero.

--*/

{
    DLGTEMPLATE *pDlgTemplate;

    UNREFERENCED_PARAMETER( hwndDlg );

    switch(uMsg) {

        case PSCB_PRECREATE:
            if(lParam){
                //
                // This is done to hide the X and ? at the top of the wizard
                //
                pDlgTemplate = (DLGTEMPLATE *)lParam;
                pDlgTemplate->style &= ~(DS_CONTEXTHELP | WS_SYSMENU);
            }
            break;

        default:
            break;
    }

    return 0;
}

