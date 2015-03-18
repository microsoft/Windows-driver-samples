//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       B I N D V I E W . C P P
//
//  Contents:
//
//  Notes:
//
//  Author:     Alok Sinha    15-Amy-01
//
//----------------------------------------------------------------------------


#include "BindView.h"

//----------------------------------------------------------------------------
// Globals
//

//
// Image list for devices of various setup class.
//

SP_CLASSIMAGELIST_DATA ClassImageListData;

HINSTANCE              hInstance;
HMENU                  hMainMenu;
HMENU                  hComponentSubMenu;
HMENU                  hBindingPathSubMenu;

//
// Network components whose bindings are enumerated.
//

LPWSTR   lpszNetClass[] = {
                    L"All Clients",
                    L"All Services",
                    L"All Protocols"
         };

//
// GUIDs of network components.
//

const GUID     *pguidNetClass [] = {
                     &GUID_DEVCLASS_NETCLIENT,
                     &GUID_DEVCLASS_NETSERVICE,
                     &GUID_DEVCLASS_NETTRANS,
                     &GUID_DEVCLASS_NET
         };

//
// Program entry point.
//

int APIENTRY
WinMain (
    _In_ HINSTANCE hInst,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nCmdShow )
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    //
    // Make sure common control DLL is loaded.
    //

    hInstance = hInst;

    InitCommonControls();

    if ( DialogBoxW(hInst,
                    MAKEINTRESOURCEW(IDD_MAIN),
                    NULL,
                    MainDlgProc) == -1 ) {

        ErrMsg( HRESULT_FROM_WIN32(GetLastError()),
                L"Failed to create the main dialog box, exiting..." );
    }

    return 0;
}

//
// WndProc for the main dialog box.
//

INT_PTR CALLBACK MainDlgProc (HWND hwndDlg,
                              UINT uMsg,
                              WPARAM wParam,
                              LPARAM lParam)
{
    HWND   hwndBindingTree;
    HICON  hIcon;

    switch (uMsg) {

        case WM_INITDIALOG:

            hIcon = LoadIcon( hInstance,
                              MAKEINTRESOURCE(IDI_BINDVIEW) );

            if ( !hIcon ) {
                ErrMsg( HRESULT_FROM_WIN32(GetLastError()),
                        L"Couldn't load the program icon, exiting..." );

                return FALSE;
            }

            SetClassLongPtr( hwndDlg,
                              GCLP_HICON,
                              (LONG_PTR)hIcon );

            hMainMenu = LoadMenu( hInstance,
                                  MAKEINTRESOURCE(IDM_OPTIONS) );

            if ( !hMainMenu ) {

                ErrMsg( HRESULT_FROM_WIN32(GetLastError()),
                        L"Couldn't load the program menu, exiting..." );

                return FALSE;
            }

            hComponentSubMenu = GetSubMenu( hMainMenu,
                                            0 );

            hBindingPathSubMenu = GetSubMenu( hMainMenu,
                                              1 );

            if ( !hComponentSubMenu || !hBindingPathSubMenu ) {

                ErrMsg( HRESULT_FROM_WIN32(GetLastError()),
                        L"Couldn't load the program menu, exiting..." );

                DestroyMenu( hMainMenu );
                return FALSE;
            }

            //
            // Add the network components types whose bindings are shown.
            //

            UpdateComponentTypeList( GetDlgItem(hwndDlg,
                                                IDL_COMPONENT_TYPES) );

            //
            // Load and associate the image list of all device classes with
            // tree.
            //

            hwndBindingTree = GetDlgItem( hwndDlg,
                                          IDT_BINDINGS );

            ZeroMemory( &ClassImageListData, sizeof(SP_CLASSIMAGELIST_DATA) );
            ClassImageListData.cbSize = sizeof(SP_CLASSIMAGELIST_DATA);

            if ( SetupDiGetClassImageList(&ClassImageListData) == TRUE ) {

                TreeView_SetImageList( hwndBindingTree,
                                       ClassImageListData.ImageList,
                                       LVSIL_NORMAL );
            }
            else {

                //
                // In case, we failed to load the image list, abort.
                //

                ErrMsg( HRESULT_FROM_WIN32(GetLastError()),
                        L"Couldn't load the image list of "
                        L"device classes, exiting..." );

                DestroyMenu( hMainMenu );
                return FALSE;
            }

            //
            // Enumerate  the bindings of the network component selected by default.
            //

            EnumNetBindings( hwndBindingTree,
                             DEFAULT_COMPONENT_SELECTED );

            return TRUE; // Tell Windows to continue creating the dialog box.

        case WM_COMMAND:

            switch( LOWORD(wParam) ) {

                case IDL_COMPONENT_TYPES:

                    if ( HIWORD(wParam) == CBN_SELCHANGE ) {

                        //
                        // User has selected a new network component type.
                        //

                        RefreshAll( hwndDlg );
                    }

                    break;

                case IDB_EXPAND_ALL:
                case IDB_COLLAPSE_ALL:

                    if ( HIWORD(wParam) == BN_CLICKED ) {

                        HTREEITEM hItem;
                        //
                        // Expand/Collapse the entire tree.
                        //

                        hwndBindingTree = GetDlgItem( hwndDlg,
                                                      IDT_BINDINGS );

                        hItem = TreeView_GetSelection( hwndBindingTree );

                        ExpandCollapseAll( hwndBindingTree,
                                           TVI_ROOT,
                                           (LOWORD(wParam) == IDB_EXPAND_ALL) ?
                                           TVE_EXPAND : TVE_COLLAPSE );

                        TreeView_SelectSetFirstVisible( hwndBindingTree,
                                                        hItem );
                    }

                    break;

                case IDB_SAVE:

                    if ( HIWORD(wParam) == BN_CLICKED ) {

                        //
                        // Save the binding information to a file.
                        //

                        WCHAR lpszFile[MAX_PATH+1];

                        if ( GetFileName(hwndDlg,
                                         L"Text files (*.txt)\0*.txt\0",
                                         L"Select a file name",
                                         OFN_DONTADDTORECENT | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT,
                                         lpszFile,
                                         L"txt",
                                         TRUE) ) {

                            DumpBindings( lpszFile );
                        }
                    }

                    break;

                case IDB_INSTALL:

                    if ( HIWORD(wParam) == BN_CLICKED ) {

                        //
                        // Install a network component.
                        //

                        if ( (BOOL)DialogBoxW(hInstance,
                                    MAKEINTRESOURCEW(IDD_INSTALL),
                                    hwndDlg,
                                    InstallDlg) == TRUE ) {

                            RefreshAll( hwndDlg );
                        }
                    }

                    break;

                case IDB_UNINSTALL:

                    if ( HIWORD(wParam) == BN_CLICKED ) {

                        //
                        // Uninstall a network component.
                        //

                        if ( (BOOL)DialogBoxW(hInstance,
                                    MAKEINTRESOURCEW(IDD_UNINSTALL),
                                    hwndDlg,
                                    UninstallDlg) == TRUE ) {

                            RefreshAll( hwndDlg );
                        }
                    }
            }

            break;

        case WM_NOTIFY:
            {
                LPNMHDR       lpnm;

                lpnm = (LPNMHDR)lParam;

                if ( (lpnm->idFrom == IDT_BINDINGS) &&
                      (lpnm->code == NM_RCLICK) ) {

                    //
                    // A network component or a binding path is selected
                    // with a right-click.
                    //

                    ProcessRightClick( lpnm );

                    //
                    // Tell Windows that the right-click has been handled
                    // us.
                    //

                    return TRUE;
                }
            }
            break;

        case WM_SYSCOMMAND:

            if ( (0xFFF0 & wParam) == SC_CLOSE ) {

                //
                // Before exiting, make sure to delete the image list
                // and the buffers associated with each item in the tree.
                //

                SetupDiDestroyClassImageList( &ClassImageListData );

                ReleaseMemory( GetDlgItem(hwndDlg, IDT_BINDINGS),
                               TVI_ROOT );

                DestroyMenu( hMainMenu );
                EndDialog( hwndDlg, 0 );
            }
    }

    return FALSE;
}

//
// WndProc of the dialog box for binding/unbinding components.
//

INT_PTR CALLBACK BindComponentDlg (HWND hwndDlg,
                                   UINT uMsg,
                                   WPARAM wParam,
                                   LPARAM lParam)
{
    LPBIND_UNBIND_INFO lpBindUnbind;

    switch (uMsg) {

        case WM_INITDIALOG:
            {
                DWORD dwCount;

                //
                // Save the lParam which is an index to the selected network
                // component.
                //

                SetWindowLongPtr( hwndDlg,
                                  DWLP_USER,
                                  (LONG_PTR)lParam );

                lpBindUnbind = (LPBIND_UNBIND_INFO)lParam;

                //
                // fBindTo is TRUE when the user wants to bind the selected
                // component to other components. So, we list the components
                // that are not bound and can bind.
                //
                //
                // fBindTo is FALSE when the user wants to unbind the selected
                // component from other components. So, we list the components
                // that are bound to it.
                //
                //
                // ListCompToBindUnbind returns number of components added to
                // the list. Keep track of it. If it zero then, we don't want to
                // show this dialog box.
                //

                dwCount = ListCompToBindUnbind(
                                      lpBindUnbind->lpszInfId,
                                      ADAPTERS_SELECTED,
                                      GetDlgItem(hwndDlg, IDT_COMPONENT_LIST),
                                      lpBindUnbind->fBindTo == FALSE );

                dwCount += ListCompToBindUnbind(
                                      lpBindUnbind->lpszInfId,
                                      CLIENTS_SELECTED,
                                      GetDlgItem(hwndDlg, IDT_COMPONENT_LIST),
                                      lpBindUnbind->fBindTo == FALSE );

                dwCount += ListCompToBindUnbind(
                                      lpBindUnbind->lpszInfId,
                                      SERVICES_SELECTED,
                                      GetDlgItem(hwndDlg, IDT_COMPONENT_LIST),
                                      lpBindUnbind->fBindTo == FALSE );

                dwCount += ListCompToBindUnbind(
                                      lpBindUnbind->lpszInfId,
                                      PROTOCOLS_SELECTED,
                                      GetDlgItem(hwndDlg, IDT_COMPONENT_LIST),
                                      lpBindUnbind->fBindTo == FALSE );

                if ( dwCount > 0 ) {

                    //
                    // Since the same dialog box is used for unbind operation,
                    // we need to update the text on the button to reflect that
                    // it is a bind operation.
                    //

                    if ( lpBindUnbind->fBindTo == FALSE ) {

                        SetWindowTextW( hwndDlg,
                                       L"Unbind From Network Components" );

                        SetWindowTextW( GetDlgItem(hwndDlg, IDB_BIND_UNBIND),
                                        L"Unbind" );

                        SetWindowTextW( GetDlgItem(hwndDlg, IDG_COMPONENT_LIST),
                                        L"Select components to unbind from" );
                    }
                }
                else {
                    if ( lpBindUnbind->fBindTo == TRUE ) {
                      ErrMsg( 0,
                                L"There no network components that can "
                                L"bind to the selected component." );
                    }
                    else {
                      ErrMsg( 0,
                                L"There no network components that are "
                                L"bound to the selected component." );
                    }

                    PostMessage( hwndDlg, WM_NO_COMPONENTS, 0, 0 );
                }

                return TRUE;
            }

        case WM_NO_COMPONENTS:
            EndDialog( hwndDlg, 0 );
            break;

        case WM_COMMAND:

            if ( (LOWORD(wParam) == IDB_CLOSE) &&
                 (HIWORD(wParam) == BN_CLICKED) ) {

                //
                // Before deleting the list in the tree, free the buffer
                // associated with each item. The buffer holds the
                // INF Id of network components.
                //

                ReleaseMemory( GetDlgItem(hwndDlg, IDT_COMPONENT_LIST),
                               TVI_ROOT );

                EndDialog( hwndDlg, 0 );
            }
            else {

                //
                // User wants to bind/unbind.
                //

                if ( (LOWORD(wParam) == IDB_BIND_UNBIND) &&
                     (HIWORD(wParam) == BN_CLICKED) ) {



                    lpBindUnbind = (LPBIND_UNBIND_INFO)GetWindowLongPtr( hwndDlg,
                                                                         DWLP_USER );

                    if ( BindUnbind(lpBindUnbind->lpszInfId,
                                    GetDlgItem(hwndDlg, IDT_COMPONENT_LIST),
                                    lpBindUnbind->fBindTo) ) {

                        RefreshBindings( hwndDlg,
                                         lpBindUnbind->lpszInfId );
                    }

                    ReleaseMemory( GetDlgItem(hwndDlg, IDT_COMPONENT_LIST),
                                   TVI_ROOT );
                    EndDialog( hwndDlg, 0 );
                }
            }
            break;

        case WM_SYSCOMMAND:

            if ( (0xFFF0 & wParam) == SC_CLOSE ) {

                //
                // Before deleting the list in the tree, free the buffer
                // associated with each item. The buffer holds the
                // INF Id of network components.
                //

                ReleaseMemory( GetDlgItem(hwndDlg, IDT_COMPONENT_LIST),
                               TVI_ROOT );

                EndDialog( hwndDlg, 0 );
            }
    }

    return FALSE;
}

//
//WndProc of the dialog box for installing network components.
//

INT_PTR CALLBACK InstallDlg (HWND hwndDlg,
                             UINT uMsg,
                             WPARAM wParam,
                             LPARAM lParam)
{
    switch (uMsg) {

        case WM_INITDIALOG:
            {
                HWND     hwndTree;

                //
                // List types of network components e.g. client,
                // protocol and service.
                //

                hwndTree = GetDlgItem( hwndDlg,
                                       IDT_COMPONENT_LIST );

                TreeView_SetImageList( hwndTree,
                                       ClassImageListData.ImageList,
                                       LVSIL_NORMAL );

                //
                // Insert and select client by default.
                //

                TreeView_Select( hwndTree,
                                 InsertItem(hwndTree,
                                            CLIENTS_SELECTED),
                                 TVGN_CARET );

                InsertItem( hwndTree,
                            SERVICES_SELECTED );

                InsertItem( hwndTree,
                            PROTOCOLS_SELECTED );

                //
                // Initialize it to FALSE. It will be set to TRUE when
                // at least one component is installed.
                //

                SetWindowLongPtr( hwndDlg,
                                  DWLP_USER,
                                  (LONG_PTR)FALSE );
                return TRUE;
            }

        case WM_COMMAND:

            switch( LOWORD(wParam) ) {

                case IDB_INSTALL:

                    //
                    // Install from Windows system directory.
                    //

                    if ( HIWORD(wParam) == BN_CLICKED ) {

                        InstallSelectedComponentType( hwndDlg, NULL );
                    }
                    break;

                case IDB_BROWSE:

                    //
                    // User wants to specify an INF file for the network
                    // to install.
                    //
                    if ( HIWORD(wParam) == BN_CLICKED ) {

                        WCHAR lpszInfFile[MAX_PATH+1];

                        if ( GetFileName(hwndDlg,
                                         L"INF files (*.inf)\0*.inf\0",
                                         L"Select the INF file of the network component to install",
                                         OFN_DONTADDTORECENT | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST,
                                         lpszInfFile,
                                         NULL,
                                         FALSE) ) {

                            InstallSelectedComponentType( hwndDlg,
                                                          lpszInfFile );
                        }
                    }
                    break;

                case IDB_CLOSE:

                    if ( HIWORD(wParam) == BN_CLICKED ) {

                        //
                        // Return the value of DWLP_USER to indicate whether one or
                        // more components have been installed. Accordingly, the
                        // the list will be refreshed.
                        //

                        EndDialog( hwndDlg,
                                   GetWindowLongPtr(hwndDlg, DWLP_USER) );
                    }
            }
            break;

        case WM_NOTIFY:
            {
                LPNMHDR       lpnm;

                lpnm = (LPNMHDR)lParam;

                if ( (lpnm->idFrom == IDT_COMPONENT_LIST) &&
                    (lpnm->code == NM_DBLCLK) ) {

                    //
                    // On double-click, install from Windows system directory.
                    //

                    InstallSelectedComponentType( hwndDlg, NULL );
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);
                    return TRUE;
                }
             }
             break;

        case WM_SYSCOMMAND:

            if ( (0xFFF0 & wParam) == SC_CLOSE ) {

                //
                // Return the value of DWLP_USER to indicate whether one or
                // more components have been installed. Accordingly, the
                // the list will be refreshed.
                //

                EndDialog( hwndDlg,
                           GetWindowLongPtr(hwndDlg, DWLP_USER) );
            }
    }

    return FALSE;
}

//
// WndProc of the dialog box for uninstalling a network component.
//

INT_PTR CALLBACK UninstallDlg (HWND hwndDlg,
                               UINT uMsg,
                               WPARAM wParam,
                               LPARAM lParam)
{
    HWND     hwndTree;

    switch (uMsg) {

        case WM_INITDIALOG:

            hwndTree = GetDlgItem( hwndDlg,
                                   IDT_COMPONENT_LIST );
            TreeView_SetImageList( hwndTree,
                                   ClassImageListData.ImageList,
                                   LVSIL_NORMAL );

            //
            // List all the components currently installed.
            //

            ListInstalledComponents( hwndTree,
                                     &GUID_DEVCLASS_NETCLIENT);
            ListInstalledComponents( hwndTree,
                                     &GUID_DEVCLASS_NETSERVICE );
            ListInstalledComponents( hwndTree,
                                     &GUID_DEVCLASS_NETTRANS );

            //
            // Initialize it to FALSE. It will be set to TRUE when
            // at least one component is installed.
            //

            SetWindowLongPtr( hwndDlg,
                              DWLP_USER,
                              (LONG_PTR)FALSE );
            return TRUE;

        case WM_COMMAND:

            switch( LOWORD(wParam) ) {

                case IDB_REMOVE:

                    if ( HIWORD(wParam) == BN_CLICKED ) {

                        //
                        // Uninstall the selected component.
                        //

                        UninstallSelectedComponent( hwndDlg );

                    }
                    break;

                case IDB_CLOSE:

                    if ( HIWORD(wParam) == BN_CLICKED ) {

                        hwndTree = GetDlgItem( hwndDlg,
                                               IDT_COMPONENT_LIST );
                        ReleaseMemory( hwndTree,
                                       TVI_ROOT );

                        //
                        // Return the value of DWLP_USER to indicate whether one or
                        // more components have been installed. Accordingly, the
                        // the list will be refreshed.
                        //

                        EndDialog( hwndDlg,
                                   GetWindowLongPtr(hwndDlg, DWLP_USER) );
                    }
            }

            break;

        case WM_NOTIFY:
            {
                LPNMHDR       lpnm;

                lpnm = (LPNMHDR)lParam;

                if ( (lpnm->idFrom == IDT_COMPONENT_LIST) &&
                    (lpnm->code == NM_DBLCLK) ) {

                    UninstallSelectedComponent( hwndDlg );
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);
                    return TRUE;
                }
             }
             break;

        case WM_SYSCOMMAND:

            if ( (0xFFF0 & wParam) == SC_CLOSE ) {

                hwndTree = GetDlgItem( hwndDlg,
                                       IDT_COMPONENT_LIST );
                ReleaseMemory( hwndTree,
                               TVI_ROOT );

                //
                // Return the value of DWLP_USER to indicate whether one or
                // more components have been installed. Accordingly, the
                // the list will be refreshed.
                //

                EndDialog( hwndDlg,
                           GetWindowLongPtr(hwndDlg, DWLP_USER) );
            }
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
// Function:  DumpBindings
//
// Purpose:   Write the binding information.
//
// Arguments:
//    lpszFile [in]  Name of the file in which to write.
//
// Returns:   None
//
// Notes:
//

VOID
DumpBindings (
    _In_ LPWSTR lpszFile)
{
    FILE *fp;
    errno_t err;

    err = _wfopen_s( &fp,
                     lpszFile,
                     L"w" );

    if ( err != 0 || fp == NULL ) {

        ErrMsg( 0,
                L"Unable to open %s.",
                lpszFile );
    }
    else {
        WriteBindings( fp );

        fclose( fp );
    }

    return;
}

//
// Function:  InstallSelectedComponentType
//
// Purpose:   Install a network component.
//
// Arguments:
//    hwndDlg     [in]  Handle to Install dialog box.
//    lpszInfFile [in]  Inf file of the network component.
//
// Returns:   None
//
// Notes:
//       If lpszInfFile is NULL, network components are installed from the
//       system directory.
//

VOID
InstallSelectedComponentType (
    HWND hwndDlg,
    _In_opt_ LPWSTR lpszInfFile)
{
    HWND      hwndTree = NULL;
    HTREEITEM hItem = NULL;
    LPARAM    lParam;
    HCURSOR   hPrevCursor = NULL;
    HCURSOR   hWaitCursor = NULL;
    HWND      hwndFocus = NULL;
    DWORD     dwType;
    BOOL      fEnable;
    HRESULT   hr;

    hwndTree = GetDlgItem( hwndDlg,
                           IDT_COMPONENT_LIST );

    //
    // Find out the type of component selected.
    //

    hItem = TreeView_GetSelection( hwndTree );

    if ( hItem ) {
        if ( GetItemInfo( hwndTree,
                          hItem,
                          &lParam,
                          &dwType,
                          &fEnable) ) {

            //
            // Disable the install dialog controls.
            //

            hwndFocus = GetFocus();

            hWaitCursor = LoadCursor( NULL,
                                      IDC_WAIT );
            if ( hWaitCursor ) {
                hPrevCursor = SetCursor( hWaitCursor );
            }

            EnableWindow( hwndTree, FALSE );
            EnableWindow( GetDlgItem(hwndDlg,IDB_INSTALL),
                          FALSE );
            EnableWindow( GetDlgItem(hwndDlg,IDB_BROWSE),
                          FALSE );
            EnableWindow( GetDlgItem(hwndDlg,IDB_CLOSE),
                          FALSE );

            if ( lpszInfFile ) {

                LPWSTR  lpszPnpID;

                //
                // Inf file name specified, install the network component
                // from this file.
                //

                hr = GetPnpID( lpszInfFile, &lpszPnpID );

                if ( hr == S_OK ) {

                    hr = InstallSpecifiedComponent( lpszInfFile,
                                                    lpszPnpID,
                                                    pguidNetClass[(UINT)lParam] );

                    CoTaskMemFree( lpszPnpID );
                }
                else {
                    ErrMsg( hr,
                            L"Error reading the INF file %s.",
                            lpszInfFile );
                }
            }
            else {

                //
                // Install from system directory.
                //

                hr = InstallComponent( hwndTree,
                                       pguidNetClass[(UINT)lParam] );
            }

            if ( hWaitCursor ) {
                SetCursor( hPrevCursor );
            }

            switch( hr ) {

                case S_OK:
                    MessageBoxW(
                           hwndDlg,
                           L"Component installed successfully.",
                           L"Network Component Installation",
                           MB_OK | MB_ICONINFORMATION );
                           SetWindowLongPtr( hwndDlg,
                                             DWLP_USER,
                                             (LONG_PTR)TRUE );
                           break;

                case NETCFG_S_REBOOT:
                    MessageBoxW(
                          hwndDlg,
                          L"Component installed successfully: "
                          L"Reboot required.",
                          L"Network Component Installation",
                          MB_OK | MB_ICONINFORMATION );
                          SetWindowLongPtr( hwndDlg,
                                            DWLP_USER,
                                            (LONG_PTR)TRUE );

            }

            //
            // Enable the install dialog controls.
            //

            EnableWindow( hwndTree, TRUE );
            EnableWindow( GetDlgItem(hwndDlg,IDB_INSTALL),
                          TRUE );
            EnableWindow( GetDlgItem(hwndDlg,IDB_BROWSE),
                          TRUE );
            EnableWindow( GetDlgItem(hwndDlg,IDB_CLOSE),
                          TRUE );

            SetFocus( hwndFocus );
        }
    }

    return;
}

//
// Function:  GetPnpID
//
// Purpose:   Retrieve PnpID from an inf file.
//
// Arguments:
//    lpszInfFile [in]  Inf file to search.
//    lppszPnpID  [out] PnpID found.
//
// Returns:   TRUE on success.
//
// Notes:
//

HRESULT
GetPnpID (
    _In_ LPWSTR lpszInfFile,
    _Outptr_ LPWSTR *lppszPnpID)
{
    HINF    hInf;
    LPWSTR  lpszModelSection;
    HRESULT hr;

    *lppszPnpID = NULL;

    hInf = SetupOpenInfFileW( lpszInfFile,
                              NULL,
                              INF_STYLE_WIN4,
                              NULL );

    if ( hInf == INVALID_HANDLE_VALUE )
    {

        return HRESULT_FROM_WIN32(GetLastError());
    }

    //
    // Read the Model section name from Manufacturer section.
    //

    hr = GetKeyValue( hInf,
                      L"Manufacturer",
                      NULL,
                      1,
                      &lpszModelSection );

    if ( SUCCEEDED(hr) )
    {

        //
        // Read PnpID from the Model section.
        //

        hr = GetKeyValue( hInf,
                          lpszModelSection,
                          NULL,
                          2,
                          lppszPnpID );

        CoTaskMemFree( lpszModelSection );
    }

    SetupCloseInfFile( hInf );

    return hr;
}

//
// Function:  GetKeyValue
//
// Purpose:   Retrieve the value of a key from the inf file.
//
// Arguments:
//    hInf        [in]  Inf file handle.
//    lpszSection [in]  Section name.
//    lpszKey     [in]  Key name.
//    dwIndex     [in]  Key index.
//    lppszValue  [out] Key value.
//
// Returns:   S_OK on success, otherwise and error code.
//
// Notes:
//

HRESULT
GetKeyValue (
    HINF hInf,
    _In_ LPCWSTR lpszSection,
    _In_opt_ LPCWSTR lpszKey,
    DWORD  dwIndex,
    _Outptr_ LPWSTR *lppszValue)
{
    INFCONTEXT  infCtx;
    __range(0, 512) DWORD       dwSizeNeeded;
    HRESULT     hr;

    *lppszValue = NULL;

    if ( SetupFindFirstLineW(hInf,
                             lpszSection,
                             lpszKey,
                             &infCtx) == FALSE )
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if ( SetupGetStringFieldW(&infCtx,
                              dwIndex,
                              NULL,
                              0,
                              &dwSizeNeeded) )
    {
        *lppszValue = (LPWSTR)CoTaskMemAlloc( sizeof(WCHAR) * dwSizeNeeded );

        if ( !*lppszValue  )
        {
        return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        }

        if ( SetupGetStringFieldW(&infCtx,
                                dwIndex,
                                *lppszValue,
                                dwSizeNeeded,
                                NULL) == FALSE )
        {

            hr = HRESULT_FROM_WIN32(GetLastError());

            CoTaskMemFree( *lppszValue );
            *lppszValue = NULL;
        }
        else
        {
            hr = S_OK;
        }
    }
    else
    {
        DWORD dwErr = GetLastError();
        hr = HRESULT_FROM_WIN32(dwErr);
    }

    return hr;
}

//
// Function:  UninstallSelectedComponent
//
// Purpose:   Uninstall the selected network component.
//
// Arguments:
//    hwndDlg     [in]  Window handle of the uninstall dialog box.
//
// Returns:   TRUE on success.
//
// Notes:
//

VOID UninstallSelectedComponent (HWND hwndDlg)
{
    HWND      hwndTree = NULL;
    HTREEITEM hItem = NULL;
    LPARAM    lParam;
    HCURSOR   hPrevCursor = NULL;
    HCURSOR   hWaitCursor = NULL;
    DWORD     dwType;
    BOOL      fEnable;
    HRESULT   hr;

    hwndTree = GetDlgItem( hwndDlg,
                           IDT_COMPONENT_LIST );

    //
    // Get the selected item to get its lParam which is the
    // PnpID of the network component.
    //

    hItem = TreeView_GetSelection( hwndTree );

    if ( hItem ) {
        if ( GetItemInfo( hwndTree,
                          hItem,
                          &lParam,
                          &dwType,
                          &fEnable) ) {

            hWaitCursor = LoadCursor( NULL,
                                      IDC_WAIT );
            if ( hWaitCursor ) {
                hPrevCursor = SetCursor( hWaitCursor );
            }

            EnableWindow( hwndTree, FALSE );
            EnableWindow( GetDlgItem(hwndDlg,IDB_REMOVE),
                          FALSE );
            EnableWindow( GetDlgItem(hwndDlg,IDB_CLOSE),
                          FALSE );

            //
            // Uninstall the selected component.
            //

            hr = UninstallComponent( (LPWSTR)lParam );


            if ( hWaitCursor ) {
                SetCursor( hPrevCursor );
            }

            switch( hr ) {

                case S_OK:
                    MessageBoxW(
                           hwndDlg,
                           L"Uninstallation successful.",
                           L"Network Component Uninstallation",
                           MB_OK | MB_ICONINFORMATION );

                    CoTaskMemFree( (LPVOID)lParam );
                    TreeView_DeleteItem( hwndTree,
                                         hItem );

                    SetWindowLongPtr( hwndDlg,
                                      DWLP_USER,
                                      (LONG_PTR)TRUE );
                    break;

                case NETCFG_S_REBOOT:
                    MessageBoxW(
                          hwndDlg,
                          L"Uninstallation successful: "
                          L"Reboot required.",
                          L"Network Component Uninstallation",
                          MB_OK | MB_ICONINFORMATION );

                    CoTaskMemFree( (LPVOID)lParam );
                    TreeView_DeleteItem( hwndTree,
                                         hItem );

                    SetWindowLongPtr( hwndDlg,
                                      DWLP_USER,
                                      (LONG_PTR)TRUE );
            }

            EnableWindow( hwndTree, TRUE );
            EnableWindow( GetDlgItem(hwndDlg,IDB_REMOVE),
                          TRUE );
            EnableWindow( GetDlgItem(hwndDlg,IDB_CLOSE),
                          TRUE );
        }
    }

    return;
}

//
// Function:  ExpandCollapseAll
//
// Purpose:   Expand or collapse a tree.
//
// Arguments:
//    hwndTree   [in]  Window handle of the tree.
//    hTreeItem  [in]  Handle of root item.
//    uiFlag     [in]  Flag indicating whether to expand or collapse.
//
// Returns:   None.
//
// Notes:
//

VOID ExpandCollapseAll (HWND hwndTree,
                        HTREEITEM hTreeItem,
                        UINT uiFlag)
{
    HTREEITEM  hItemChild;

    hItemChild = TreeView_GetChild( hwndTree,
                                    hTreeItem );

    if ( hItemChild ) {

        //
        // If the root has one or more children, expand/collapse the root.
        //

        TreeView_Expand( hwndTree,
                         hTreeItem,
                         uiFlag );
    }

    while ( hItemChild ) {

        //
        // Expand/collapse all the children.
        //

        ExpandCollapseAll( hwndTree,
                           hItemChild,
                           uiFlag );

        //
        // Expand/collapse all the siblings.
        //

        hItemChild = TreeView_GetNextSibling( hwndTree,
                                              hItemChild );
    }

    return;
}

//
// Function:  GetFileName
//
// Purpose:   Prompt for a filename.
//
// Arguments:
//    hwndDlg    [in]  Window handle of the parent.
//    lpszFilter [in]  See documentation for GetOpenFileName.
//    lpszTitle  [in]  See documentation for GetOpenFileName.
//    dwFlags    [in]  See documentation for GetOpenFileName.
//    lpszFile   [out]  See documentation for GetOpenFileName.
//                      Supplied buffer must be at least MAX_PATH+1 WCHARS
//
// Returns:   See documentation for GetOpenFileName.
//
// Notes:
//

BOOL
GetFileName (
    HWND hwndDlg,
    _In_opt_ LPWSTR lpszFilter,
    _In_ LPWSTR lpszTitle,
    DWORD dwFlags,
    _Out_writes_(MAX_PATH+1) LPWSTR lpszFile,
    _In_opt_ LPWSTR lpszDefExt,
    BOOL   fSave)
{
    OPENFILENAMEW ofn;

    lpszFile[0] = NULL;

    ZeroMemory( &ofn, sizeof(OPENFILENAMEW) );
    ofn.lStructSize = sizeof(OPENFILENAMEW);
    ofn.hwndOwner = hwndDlg;
    ofn.lpstrFilter = lpszFilter;
    ofn.lpstrFile = lpszFile;
    ofn.lpstrDefExt  = lpszDefExt;
    ofn.nMaxFile = MAX_PATH+1;
    ofn.lpstrTitle = lpszTitle;
    ofn.Flags = dwFlags;

    if ( fSave )
    {
        return GetSaveFileName( &ofn );
    }
    else
    {
        return GetOpenFileName( &ofn );
    }
}

//
// Function:  ProcessRightClick
//
// Purpose:   Handle righ mouse button click.
//
// Arguments:
//    lpnm    [in]  LPNMHDR info
//
// Returns:   None.
//
// Notes:
//

VOID ProcessRightClick (LPNMHDR lpnm)
{
    HTREEITEM hItemSelected;
    LPARAM    lParam;
    DWORD     dwItemType;
    BOOL      fEnabled;

    //
    // Determine the item on which user clicked the right mouse button.
    //

    hItemSelected = TreeView_GetDropHilight( lpnm->hwndFrom );

    if ( !hItemSelected ) {
        hItemSelected = TreeView_GetSelection( lpnm->hwndFrom );
    }
    else {

        //
        // User has right-clicked an unselected item, make that a selected
        // item.
        //

        TreeView_Select( lpnm->hwndFrom,
                         hItemSelected,
                         TVGN_CARET );
    }

    if ( hItemSelected ) {

        //
        // Get the lParam of the selected node in the tree which points to inf id or
        // pathtoken name depending on if the node represents a network component or
        // a binding path.
        //

        if ( GetItemInfo(lpnm->hwndFrom,
                         hItemSelected,
                         &lParam,
                         &dwItemType,
                         &fEnabled) ) {

            if ( dwItemType & ITEM_NET_COMPONENTS ) {

                //
                // Show the shortcut menu of operations for a network component.
                //

                ShowComponentMenu( lpnm->hwndFrom,
                                   hItemSelected,
                                   lParam);
            }
            else {
                if ( dwItemType & ITEM_NET_BINDINGS ) {

                    //
                    // Show the shortcut menu of operations for a binding path.
                    //

                    ShowBindingPathMenu( lpnm->hwndFrom,
                                         hItemSelected,
                                         lParam,
                                         fEnabled );
                }
            }
        }
    }

    return;
}

//
// Function:  ShowComponentMenu
//
// Purpose:   Show shortcut menu of options for a network component.
//
// Arguments:
//    hwndOwner  [in]  Owner window.
//    hItem      [in]  Selected item representing a network component.
//    lParam     [in]  PnpID of the network component.
//
// Returns:   None.
//
// Notes:
//

VOID ShowComponentMenu (HWND hwndOwner,
                        HTREEITEM hItem,
                        LPARAM lParam)
{
    ULONG   ulSelection;
    POINT   pt;

    GetCursorPos( &pt );
    ulSelection = (ULONG)TrackPopupMenu( hComponentSubMenu,
                                         TPM_RIGHTALIGN | TPM_BOTTOMALIGN |
                                         TPM_NONOTIFY | TPM_RETURNCMD |
                                         TPM_RIGHTBUTTON,
                                         pt.x,
                                         pt.y,
                                         0,
                                         hwndOwner,
                                         NULL );

    if ( ulSelection ) {

        //
        // Do the selected action.
        //

        HandleComponentOperation( hwndOwner,
                                  ulSelection,
                                  hItem,
                                  lParam );
    }

    return;
}

//
// Function:  ShowBindingPathMenu
//
// Purpose:   Show shortcut menu of options for a network component.
//
// Arguments:
//    hwndOwner  [in]  Owner window.
//    hItem      [in]  Selected item representing a binding path.
//    lParam     [in]  PnpID of the network component.
//    fEnabled   [in]  TRUE when the path is enabled.
//
// Returns:   None.
//
// Notes:
//


VOID ShowBindingPathMenu (HWND hwndOwner,
                          HTREEITEM hItem,
                          LPARAM lParam,
                          BOOL fEnabled)
{
    MENUITEMINFOW  menuItemInfo;
    ULONG   ulSelection;
    POINT   pt;

    //
    // Build the shortcut menu depending on whether path is
    // disabled or enabled.
    //

    ZeroMemory( &menuItemInfo,
                sizeof(MENUITEMINFOW) );

    menuItemInfo.cbSize = sizeof( MENUITEMINFOW );
    menuItemInfo.fMask = MIIM_TYPE | MIIM_ID;
    menuItemInfo.fType = MFT_STRING;
    menuItemInfo.fState = MFS_ENABLED;

    if ( fEnabled ) {
        menuItemInfo.dwTypeData = MENUITEM_DISABLE;
        menuItemInfo.wID = IDI_DISABLE;
    }
    else {
        menuItemInfo.dwTypeData = MENUITEM_ENABLE;
        menuItemInfo.wID = IDI_ENABLE;
    }

    SetMenuItemInfoW( hBindingPathSubMenu,
                     0,
                     TRUE,
                     &menuItemInfo );

    GetCursorPos( &pt );
    ulSelection = (ULONG)TrackPopupMenu( hBindingPathSubMenu,
                                         TPM_RIGHTALIGN | TPM_BOTTOMALIGN |
                                         TPM_NONOTIFY | TPM_RETURNCMD |
                                         TPM_RIGHTBUTTON,
                                         pt.x,
                                         pt.y,
                                         0,
                                         hwndOwner,
                                         NULL );

    if ( ulSelection ) {

        //
        // Do the selected action.
        //

        HandleBindingPathOperation( hwndOwner,
                                    ulSelection,
                                    hItem,
                                    lParam );
    }

    return;
}

//
// Function:  GetItemInfo
//
// Purpose:   Returns information about an item.
//
// Arguments:
//    hwndTree     [in]  Window handle of the tree.
//    hItem        [in]  Item handle.
//    lParam       [out] lParam
//    lpdwItemType [out] Type, binding path or network component.
//    fEnabled     [out] TRUE if the binding path or component is enabled.
//
// Returns:   TRUE on sucess.
//
// Notes:
//

BOOL GetItemInfo (HWND hwndTree,
                  HTREEITEM hItem,
                  LPARAM *lParam,
                  LPDWORD lpdwItemType,
                  BOOL *fEnabled)
{
    TVITEMW   tvItem;
    int       iImage;
    BOOL      fSuccess;


    fSuccess = FALSE;

    //
    // Get item's information.
    //

    ZeroMemory( &tvItem,
                sizeof(TVITEMW) );
    tvItem.hItem = hItem;
    tvItem.mask = TVIF_PARAM | TVIF_IMAGE | TVIF_STATE;
    tvItem.stateMask = TVIS_OVERLAYMASK ;

    if ( TreeView_GetItem(hwndTree,
                          &tvItem) ) {

        *lParam = tvItem.lParam;

        if ( SetupDiGetClassImageIndex(&ClassImageListData,
                                       &GUID_DEVCLASS_SYSTEM,
                                       &iImage) ) {

            //
            // Is it a binding path?
            //

            if ( tvItem.iImage == iImage ) {
                *lpdwItemType = ITEM_NET_BINDINGS;

                *fEnabled = !(TVIS_OVERLAYMASK & tvItem.state);

                fSuccess = TRUE;
            }
            else {

                //
                // Item is a network component.
                //

                if ( SetupDiGetClassImageIndex(&ClassImageListData,
                                               &GUID_DEVCLASS_NET,
                                               &iImage) ) {

                    if ( tvItem.iImage == iImage ) {
                        *lpdwItemType = ITEM_NET_ADAPTERS;
                    }
                    else {
                        *lpdwItemType = ITEM_NET_COMPONENTS;
                    }

                    *fEnabled = !(TVIS_OVERLAYMASK & tvItem.state);

                    fSuccess = TRUE;
                }
                else {
                    ErrMsg( HRESULT_FROM_WIN32(GetLastError()),
                            L"Couldn't load the images of network adapters." );
                }
            }
        }
        else {
            ErrMsg( HRESULT_FROM_WIN32(GetLastError()),
                    L"Couldn't load the images of system devices." );
        }
    }

    return fSuccess;
}

//
// Function:  AddBindNameToTree
//
// Purpose:   Adds an item representing the binding path.
//
// Arguments:
//    pncbp     [in]  Binding path to add.
//    hwndTree  [in]  Tree handle.
//    hParent   [in]  Parent item.
//    ulIndex   [in]  Index of the binding path.
//
// Returns:   Handle of the item added on success, otherwise NULL.
//
// Notes:
//

HTREEITEM AddBindNameToTree (INetCfgBindingPath *pncbp,
                             HWND hwndTree,
                             HTREEITEM hParent,
                             ULONG  ulIndex)
{
    WCHAR            lpszBindName[40];
    LPWSTR           lpszPathToken;
    HTREEITEM        hTreeItem;
    TV_INSERTSTRUCTW tvInsertStruc;
    HRESULT          hr;

    hTreeItem = NULL;

    //
    // Store the path token as lParam.
    //

    hr = pncbp->GetPathToken( &lpszPathToken );

    if ( hr == S_OK ) {

        StringCchPrintfW (lpszBindName,
            celems(lpszBindName),
            L"Binding Path %d",
            ulIndex );

        ZeroMemory(
              &tvInsertStruc,
              sizeof(TV_INSERTSTRUCTW) );

        tvInsertStruc.hParent = hParent;

        tvInsertStruc.hInsertAfter = TVI_LAST;

        tvInsertStruc.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE |
                                  TVIF_SELECTEDIMAGE | TVIF_STATE;

        tvInsertStruc.item.pszText = lpszBindName;

        SetupDiGetClassImageIndex( &ClassImageListData,
                                   &GUID_DEVCLASS_SYSTEM,
                                   &tvInsertStruc.item.iImage );

        tvInsertStruc.item.iSelectedImage = tvInsertStruc.item.iImage;

        tvInsertStruc.item.stateMask = TVIS_OVERLAYMASK;

        if (  pncbp->IsEnabled() == S_FALSE ) {
            tvInsertStruc.item.state = INDEXTOOVERLAYMASK(
                                   IDI_DISABLED_OVL - IDI_CLASSICON_OVERLAYFIRST + 1);
        }

        tvInsertStruc.item.lParam = (LPARAM)lpszPathToken;

        hTreeItem = TreeView_InsertItem( hwndTree,
                                         &tvInsertStruc );

        if ( !hTreeItem ) {
            ErrMsg( hr,
                    L"Couldn't add the binding path %d to the list."
                    L" The binding path will not be shown.", ulIndex );

            CoTaskMemFree( lpszPathToken );
        }
    }
    else {
        ErrMsg( hr,
                L"Couldn't get the PathToken of the binding path %d."
                L" The binding path will not be shown.", ulIndex );
    }

    return hTreeItem;
}

//
// Function:  AddToTree
//
// Purpose:   Adds an item representing the network component.
//
// Arguments:
//    hwndTree  [in]  Tree handle.
//    hParent   [in]  Parent item.
//    pncc      [in]  Network component.
//
// Returns:   Handle of the item added on success, otherwise NULL.
//
// Notes:
//

HTREEITEM AddToTree (HWND hwndTree,
                     HTREEITEM hParent,
                     INetCfgComponent *pncc)
{
    return AddToTreeEx( hwndTree,
                        hParent,
                        pncc,
                        FALSE );
}

//
// Function:  AddToTree
//
// Purpose:   Adds an item representing the network component.
//
// Arguments:
//    hwndTree  [in]  Tree handle.
//    hParent   [in]  Parent item.
//    pncc      [in]  Network component.
//    fUsePnpId [in]  if TRUE, pnp device instance id is used to indentify the component, otherwise inf id.
//
// Returns:   Handle of the item added on success, otherwise NULL.
//
// Notes:
//

HTREEITEM AddToTreeEx (HWND hwndTree,
                       HTREEITEM hParent,
                       INetCfgComponent *pncc,
                       BOOL fUsePnpId)
{
    LPWSTR           lpszItemName;
    LPWSTR           lpszId;
    GUID             guidClass;
    BOOL             fEnabled;
    ULONG            ulStatus;
    HTREEITEM        hTreeItem;
    TV_INSERTSTRUCTW tvInsertStruc;
    HRESULT          hr;

    hTreeItem = NULL;

    hr = pncc->GetDisplayName( &lpszItemName );

    if ( hr == S_OK ) {

        //
        // Get the inf id or pnp instance id of the network component. We store it at lParam
        // and use it later to retrieve its interface pointer.
        //

        if ( fUsePnpId )
        {
            hr = pncc->GetPnpDevNodeId( &lpszId );
        }
        else
        {
            hr = pncc->GetId( &lpszId );
        }

        if ( hr == S_OK ) {

            //
            // If it is a network adapter then, find out if it enabled/disabled.
            //

            hr = pncc->GetClassGuid( &guidClass );

            if ( hr == S_OK ) {
                if ( IsEqualGUID(guidClass, GUID_DEVCLASS_NET) ) {
                    hr = pncc->GetDeviceStatus( &ulStatus );
                    fEnabled = ulStatus == 0;
                }
                else {
                    fEnabled = TRUE;
                }
            }
            else {

                //
                // We can't get the status, so assume that it is disabled.
                //

                fEnabled = FALSE;
            }

            ZeroMemory(
                  &tvInsertStruc,
                  sizeof(TV_INSERTSTRUCTW) );

            tvInsertStruc.hParent = hParent;

            tvInsertStruc.hInsertAfter = TVI_LAST;

            tvInsertStruc.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE |
                                      TVIF_SELECTEDIMAGE | TVIF_STATE;

            tvInsertStruc.item.pszText = lpszItemName;

            SetupDiGetClassImageIndex( &ClassImageListData,
                                       &guidClass,
                                       &tvInsertStruc.item.iImage );

            tvInsertStruc.item.iSelectedImage = tvInsertStruc.item.iImage;

            tvInsertStruc.item.stateMask = TVIS_OVERLAYMASK;

            if ( fEnabled == FALSE ) {
                tvInsertStruc.item.state = INDEXTOOVERLAYMASK(
                                       IDI_DISABLED_OVL - IDI_CLASSICON_OVERLAYFIRST + 1);
            }

            tvInsertStruc.item.lParam = (LPARAM)lpszId;

            hTreeItem = TreeView_InsertItem( hwndTree,
                                             &tvInsertStruc );
            if ( !hTreeItem ) {
                ErrMsg( hr,
                        L"Failed to add %s to the list.",
                        lpszItemName );

                CoTaskMemFree( lpszId );
            }
        }
        else {
            ErrMsg( hr,
                    L"Couldn't get the inf id of %s."
                    L" It will not be added to the list.",
                    lpszItemName );
        }

        CoTaskMemFree( lpszItemName );
    }
    else {
        ErrMsg( hr,
                L"Couldn't get the display name of a network component."
                L" It will not be added to the list." );
    }

    return hTreeItem;
}

//
// Function:  RefreshAll
//
// Purpose:   Refreshes the main dialog box.
//
// Arguments:
//    hwndDlg  [in]  Dialog box handle.
//
// Returns:   None.
//
// Notes:
//

VOID RefreshAll (HWND hwndDlg)
{
    HWND hwndTypeList;
    INT  iSelected;

    //
    // Find the selected network component type.
    //

    hwndTypeList = GetDlgItem( hwndDlg,
                              IDL_COMPONENT_TYPES );

    iSelected = (int)SendMessage( hwndTypeList,
                                  CB_GETCURSEL,
                                  0,
                                  0 );

    if ( iSelected != CB_ERR ) {

        //
        // Before deleting the list in the tree, free the buffer
        // associated with each item. The buffer holds either the
        // INF Id or the pathtoken depending on whether it is a
        // network component or a binding path.
        //

        ReleaseMemory( GetDlgItem(hwndDlg, IDT_BINDINGS),
                       TVI_ROOT );

        TreeView_DeleteItem (
                    GetDlgItem(hwndDlg, IDT_BINDINGS),
                    TVI_ROOT );

        //
        // Repopulate the tree with the selected network compnent
        // type.
        //

        EnumNetBindings( GetDlgItem(hwndDlg, IDT_BINDINGS),
                         (UINT)iSelected );

    }

    return;
}

//
// Function:  RefreshItemState
//
// Purpose:   Refreshes the specified item.
//
// Arguments:
//    hwndTree  [in]  Dialog box handle.
//    hItem     [in]  Item to refresh.
//    fEnable   [in]  TRUE if component is enabled.
//
// Returns:   None.
//
// Notes:
//

VOID RefreshItemState (HWND hwndTree,
                       HTREEITEM hItem,
                       BOOL fEnable)
{
    TVITEMW       tvItem;

    ZeroMemory( &tvItem,
                sizeof(TVITEMW) );

    tvItem.hItem = hItem;
    tvItem.mask = TVIF_STATE;
    tvItem.stateMask = TVIS_OVERLAYMASK;

    if ( fEnable )
        tvItem.state = INDEXTOOVERLAYMASK( 0 );
    else
        tvItem.state = INDEXTOOVERLAYMASK(
                             IDI_DISABLED_OVL - IDI_CLASSICON_OVERLAYFIRST + 1);
    TreeView_SetItem( hwndTree,
                      &tvItem );
    return;
}

//
// Function:  RefreshBindings
//
// Purpose:   Refreshes bindings of a specific component.
//
// Arguments:
//    hwndBindUnBindDlg  [in]  Dialog box handle.
//    lpszInfId          [in]  PnpID of the component whose bindings changed.
//
// Returns:   None.
//
// Notes:
//

VOID
RefreshBindings (
    HWND hwndBindUnBindDlg,
    _In_ LPWSTR lpszInfId)
{
    INetCfg              *pnc;
    INetCfgComponent     *pncc;
    HWND                 hwndParent;
    HWND                 hwndTree;
    HTREEITEM            hItem;
    HRESULT              hr;


    hwndParent = GetParent( hwndBindUnBindDlg );
    hwndTree = GetDlgItem( hwndParent,
                           IDT_BINDINGS );

    hItem = TreeView_GetSelection( hwndTree );

    hr = HrGetINetCfg( FALSE,
                       APP_NAME,
                       &pnc,
                       NULL );

    if ( hr == S_OK ) {

        hr = pnc->FindComponent( lpszInfId,
                                 &pncc );

        if ( hr == S_OK ) {

            //
            // Delete all the children.
            //

            ReleaseMemory( hwndTree,
                           hItem );

            DeleteChildren( hwndTree,
                            hItem );

            ListBindings( pncc,
                          hwndTree,
                          hItem );

            ReleaseRef( pncc );
        }

        HrReleaseINetCfg( pnc,
                          FALSE );
    }

    return;
}

//
// Function:  ReleaseMemory
//
// Purpose:   Free memory associated with each item in the tree.
//
// Arguments:
//    hwndTree  [in]  Tree handle.
//    hTreeItem [in]  Root item.
//
// Returns:   None.
//
// Notes:
//
// Each node of the tree represents a network component or a binding path.
// At each node, lParam points to an allocated buffer wherein we store the
// inf id if it is a network component or pathtoken name if it is a binding
// path.
//
//

VOID ReleaseMemory (HWND hwndTree,
                    HTREEITEM hTreeItem)
{
    HTREEITEM  hItemChild;
    TVITEMW    tvItem;

    hItemChild = TreeView_GetChild( hwndTree,
                                    hTreeItem );

    while ( hItemChild ) {

        ZeroMemory(
              &tvItem,
              sizeof(TVITEMW) );

        tvItem.hItem = hItemChild;
        tvItem.mask = TVIF_PARAM;

        TreeView_GetItem( hwndTree,
                          &tvItem );

        //
        // It should never be NULL but just in case...
        //

        if ( tvItem.lParam ) {
            CoTaskMemFree( (LPVOID)tvItem.lParam );

        }

        ReleaseMemory( hwndTree, hItemChild );

        hItemChild = TreeView_GetNextSibling( hwndTree,
                                              hItemChild );
    }

    return;
}

//
// Function:  DeleteChildren
//
// Purpose:   Delete childen of a specific item.
//
// Arguments:
//    hwndTree  [in]  Tree handle.
//    hTreeItem [in]  Parent item.
//
// Returns:   None.
//
// Notes:
//

VOID DeleteChildren (HWND hwndTree,
                     HTREEITEM hTreeItem)
{
    HTREEITEM  hItemChild;
    HTREEITEM  hItemSibling;

    hItemChild = TreeView_GetChild( hwndTree,
                                    hTreeItem );

    while ( hItemChild ) {

        DeleteChildren( hwndTree,
                        hItemChild );

        hItemSibling = TreeView_GetNextSibling( hwndTree,
                                                hItemChild );
        TreeView_DeleteItem( hwndTree,
                             hItemChild );

        hItemChild = hItemSibling;
    }

    return;
}

//
// Function:  InsertItem
//
// Purpose:   Insert text for each network component type.
//
// Arguments:
//    hwndTree  [in]  Tree handle.
//    uiType    [in]  Item type, protocol, client, service.
//
// Returns:   Item handle on success, otherwise NULL.
//
// Notes:
//

HTREEITEM InsertItem (HWND hwndTree,
                      UINT uiType)
{
    TV_INSERTSTRUCTW tvInsertStruc;

    ZeroMemory(
          &tvInsertStruc,
          sizeof(TV_INSERTSTRUCTW) );

    tvInsertStruc.hParent = TVI_ROOT;

    tvInsertStruc.hInsertAfter = TVI_LAST;

    tvInsertStruc.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE |
                              TVIF_SELECTEDIMAGE;


    switch( uiType ) {

        case CLIENTS_SELECTED:
            tvInsertStruc.item.pszText = L"Client";
            break;

        case SERVICES_SELECTED:
            tvInsertStruc.item.pszText = L"Service";
            break;

        default:
            tvInsertStruc.item.pszText = L"Protocol";
            break;
    }

    SetupDiGetClassImageIndex( &ClassImageListData,
                               pguidNetClass[uiType],
                               &tvInsertStruc.item.iImage );

    tvInsertStruc.item.iSelectedImage = tvInsertStruc.item.iImage;

    tvInsertStruc.item.lParam = (LPARAM)uiType;

    return TreeView_InsertItem( hwndTree,
                                &tvInsertStruc );

}

//
// Function:  UpdateComponentTypeList
//
// Purpose:   Insert text for each network component type.
//
// Arguments:
//    hwndTypeList  [in]  ListView handle.
//
// Returns:   TRUE on success.
//
// Notes:
//

BOOL UpdateComponentTypeList (HWND hwndTypeList)
{
    UINT i;

    for (i=0; i < 3; ++i) {
        SendMessage( hwndTypeList,
                     CB_ADDSTRING,
                     (WPARAM)0,
                     (LPARAM)lpszNetClass[i] );
    }

    SendMessage( hwndTypeList,
                 CB_SETCURSEL,
                 (WPARAM)DEFAULT_COMPONENT_SELECTED,
                 (LPARAM)0 );
    return TRUE;
}

//
// Function:  ErrMsg
//
// Purpose:   Insert text for each network component type.
//
// Arguments:
//    hr  [in]  Error code.
//
// Returns:   None.
//
// Notes:
//

VOID ErrMsg (HRESULT hr,
             LPCWSTR  lpFmt,
             ...)
{

    LPWSTR   lpSysMsg = NULL;
    WCHAR    buf[400];
    size_t   offset;
    va_list  vArgList;


    if ( hr != 0 ) {
        StringCchPrintfW ( buf,
            celems(buf),
            L"Error %#lx: ",
            hr );
    }
    else {
        buf[0] = 0;
    }

    offset = wcslen( buf );

    va_start( vArgList,
              lpFmt );
    StringCchVPrintfW ( buf+offset,
        celems(buf) - offset,
        lpFmt,
        vArgList );

    va_end( vArgList );

    if ( hr != 0 ) {
        FormatMessageW( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                       FORMAT_MESSAGE_FROM_SYSTEM |
                       FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL,
                       hr,
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       (LPWSTR)&lpSysMsg,
                       0,
                       NULL );
        if ( lpSysMsg ) {

            offset = wcslen( buf );

            StringCchPrintfW ( buf+offset,
                celems(buf) - offset,
                L"\n\nPossible cause:\n\n" );

            offset = wcslen( buf );

            StringCchCatW ( buf+offset,
                celems(buf) - offset,
                lpSysMsg );

            LocalFree( (HLOCAL)lpSysMsg );
        }

        MessageBoxW( NULL,
                    buf,
                    L"Error",
                    MB_ICONERROR | MB_OK );
    }
    else {
        MessageBoxW( NULL,
                    buf,
                    L"BindView",
                    MB_ICONINFORMATION | MB_OK );
    }

    return;
}
