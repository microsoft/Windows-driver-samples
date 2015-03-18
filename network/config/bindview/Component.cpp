//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       C O M P O N E N T . C P P
//
//  Contents:   Functions to illustrate
//              o How to enumerate network components.
//              o How to install protocols, clients and services.
//              o How to uninstall protocols, clients and services.
//              o How to bind/unbind network components.
//
//  Notes:
//
//  Author:     Alok Sinha    15-May-01
//
//----------------------------------------------------------------------------

#include "bindview.h"

//
// Function:  HandleComponentOperation
//
// Purpose:   Do component specific functions.
//
// Arguments:
//    hwndOwner    [in]  Owner window.
//    ulSelection  [in]  Option selected.
//    hItem        [in]  Item selected.
//    lParam       [in]  lParam of the item.
//
// Returns:   None.
//
// Notes:
//

VOID HandleComponentOperation (HWND hwndOwner,
                               ULONG ulSelection,
                               HTREEITEM hItem,
                               LPARAM lParam)
{
    switch( ulSelection ) {

        case IDI_BIND_TO:
        case IDI_UNBIND_FROM:

            //
            // Bind/unbind components.
            //

            BindUnbindComponents( hwndOwner,
                                  hItem,
                                  (LPWSTR)lParam,
                                  ulSelection == IDI_BIND_TO );
    }

    return;
}

//
// Function:  BindUnbindComponents
//
// Purpose:   Bind/unbind a network component.
//
// Arguments:
//    hwndOwner  [in]  Owner window.
//    hItem      [in]  Item handle of the network component.
//    lpszInfId  [in]  PnpID of the network component.
//    fBindTo    [in]  if TRUE, bind, otherwise unbind.
//
// Returns:   None.
//
// Notes:
//

VOID BindUnbindComponents( HWND hwndOwner,
                           HTREEITEM hItem,
                           _In_ LPWSTR lpszInfId,
                           BOOL fBindTo)
{
    UNREFERENCED_PARAMETER(hItem);

    BIND_UNBIND_INFO  BindUnbind;

    BindUnbind.lpszInfId = lpszInfId;
    BindUnbind.fBindTo = fBindTo;

    DialogBoxParam( hInstance,
                    MAKEINTRESOURCE(IDD_BIND_UNBIND),
                    hwndOwner,
                    BindComponentDlg,
                    (LPARAM)&BindUnbind );

    return;
}

//
// Function:  InstallComponent
//
// Purpose:   Install a network component.
//
// Arguments:
//    hwndDlg     [in]  Owner window.
//    pguidClass  [in]  Class GUID of type of network component to install.
//
// Returns:   S_OK on success, otherwise an error code.
//
// Notes:
//

HRESULT InstallComponent (HWND hwndDlg,
                          const GUID *pguidClass)
{
    INetCfg              *pnc;
    INetCfgClass         *pncClass;
    INetCfgClassSetup    *pncClassSetup;
    INetCfgComponent     *pnccItem;
    LPWSTR               lpszApp;
    OBO_TOKEN            obo;
    HRESULT              hr;

    //
    // Get INetCfg reference.
    //

    hr = HrGetINetCfg( TRUE,
                       APP_NAME,
                       &pnc,
                       &lpszApp );

    if ( hr == S_OK ) {

        //
        // Get network component's class reference.
        //

        hr = pnc->QueryNetCfgClass( pguidClass,
                                    IID_INetCfgClass,
                                    (PVOID *)&pncClass );

        if ( hr == S_OK ) {

            //
            // Get Setup class reference.
            //

            hr = pncClass->QueryInterface( IID_INetCfgClassSetup,
                                           (LPVOID *)&pncClassSetup );

            if ( hr == S_OK ) {

                ZeroMemory( &obo,
                            sizeof(OBO_TOKEN) );

                obo.Type = OBO_USER;

                //
                // Let the network class installer prompt the user to select
                // a network component to install.
                //

                hr = pncClassSetup->SelectAndInstall( hwndDlg,
                                                      &obo,
                                                      &pnccItem );

                if ( (hr == S_OK) || (hr == NETCFG_S_REBOOT) ) {

                    hr = pnc->Apply();

                    if ( (hr != S_OK) && (hr != NETCFG_S_REBOOT) ) {

                        ErrMsg( hr,
                                L"Couldn't apply the changes after"
                                L" installing the network component." );
                    }

                }
                else {
                    if ( hr != HRESULT_FROM_WIN32(ERROR_CANCELLED) ) {
                        ErrMsg( hr,
                                L"Couldn't install the network component." );
                    }
                }

                ReleaseRef( pncClassSetup );
            }
            else {
                ErrMsg( hr,
                        L"Couldn't get an interface to setup class." );
            }

            ReleaseRef( pncClass );
        }
        else {
            ErrMsg( hr,
                    L"Couldn't get a pointer to class interface." );
        }

        HrReleaseINetCfg( pnc,
                          TRUE );
    }
    else {
        if ( (hr == NETCFG_E_NO_WRITE_LOCK) && lpszApp ) {
            ErrMsg( hr,
                    L"%s currently holds the lock, try later.",
                    lpszApp );

            CoTaskMemFree( lpszApp );
        }
        else {
            ErrMsg( hr,
                    L"Couldn't the get notify object interface." );
        }
    }

    return hr;
}

//
// Function:  InstallSpecifiedComponent
//
// Purpose:   Install a network component from an INF file.
//
// Arguments:
//    lpszInfFile [in]  INF file.
//    lpszPnpID   [in]  PnpID of the network component to install.
//    pguidClass  [in]  Class GUID of the network component.
//
// Returns:   None.
//
// Notes:
//

HRESULT InstallSpecifiedComponent ( _In_ LPWSTR lpszInfFile,
                                   _In_ LPWSTR lpszPnpID,
                                   const GUID *pguidClass)
{
    INetCfg    *pnc;
    LPWSTR     lpszApp;
    HRESULT    hr;

    hr = HrGetINetCfg( TRUE,
                       APP_NAME,
                       &pnc,
                       &lpszApp );

    if ( hr == S_OK ) {

        //
        // Install the network component.
        //

        hr = HrInstallNetComponent( pnc,
                                    lpszPnpID,
                                    pguidClass,
                                    lpszInfFile );
        if ( (hr == S_OK) || (hr == NETCFG_S_REBOOT) ) {

            hr = pnc->Apply();
        }
        else {
            if ( hr != HRESULT_FROM_WIN32(ERROR_CANCELLED) ) {
                ErrMsg( hr,
                        L"Couldn't install the network component." );
            }
        }

        HrReleaseINetCfg( pnc,
                          TRUE );
    }
    else {
        if ( (hr == NETCFG_E_NO_WRITE_LOCK) && lpszApp ) {
            ErrMsg( hr,
                    L"%s currently holds the lock, try later.",
                    lpszApp );

            CoTaskMemFree( lpszApp );
        }
        else {
            ErrMsg( hr,
                    L"Couldn't the get notify object interface." );
        }
    }

    return hr;
}

//
// Function:  ListCompToBindUnbind
//
// Purpose:   List all the components that are bound or bindable.
//
// Arguments:
//    lpszInfId [in]  PnpID of the network component.
//    uiType    [in]  Type of network component.
//    hwndTree  [in]  Tree handle in which to list.
//    fBound    [in]  if TRUE, list components that are bound.
//
// Returns:   Number of components listed.
//
// Notes:
//

DWORD ListCompToBindUnbind ( _In_ LPWSTR lpszInfId,
                            UINT uiType,
                            HWND hwndTree,
                            BOOL fBound)
{
    INetCfg                   *pnc;
    INetCfgComponent          *pncc;
    IEnumNetCfgComponent      *pencc;
    INetCfgComponentBindings  *pnccb;
    INetCfgComponent          *pnccToBindUnbind;
    LPWSTR                    lpszApp;
    DWORD                     dwCount;
    HRESULT                   hr;


    dwCount = 0;
    hr = HrGetINetCfg( TRUE,
                       APP_NAME,
                       &pnc,
                       &lpszApp );

    if ( hr == S_OK ) {

        //
        // Get a reference to the network component selected.
        //

        hr = pnc->FindComponent( lpszInfId,
                                 &pncc );

        if ( hr == S_OK ) {

            //
            // Get Component Enumerator Interface.
            //

            hr = HrGetComponentEnum( pnc,
                                     pguidNetClass[uiType],
                                     &pencc );
            if ( hr == S_OK ) {

                hr = pncc->QueryInterface( IID_INetCfgComponentBindings,
                                          (PVOID *)&pnccb );
                if ( hr == S_OK ) {

                    hr = HrGetFirstComponent( pencc, &pnccToBindUnbind );

                    while( hr == S_OK ) {

                        hr = pnccb->IsBoundTo( pnccToBindUnbind );

                        //
                        // fBound = TRUE ==> Want to list components that are
                        // bound.
                        //

                        if ( fBound ) {

                            if ( hr == S_OK ) {
                                if ( IsEqualIID( *pguidNetClass[uiType], GUID_DEVCLASS_NET) )
                                {
                                    AddToTreeEx( hwndTree,
                                                 TVI_ROOT,
                                                 pnccToBindUnbind,
                                                 TRUE );
                                }
                                else
                                {
                                    AddToTreeEx( hwndTree,
                                                 TVI_ROOT,
                                                 pnccToBindUnbind,
                                                 FALSE );
                                }

                                dwCount++;
                            }
                        }
                        else {

                            //
                            // fBound = FALSE ==> Want to list components that
                            // are not bound but are bindable.
                            //

                            if ( hr == S_FALSE ) {

                                hr = pnccb->IsBindableTo( pnccToBindUnbind );

                                if ( hr == S_OK ) {

                                    if ( IsEqualIID( *pguidNetClass[uiType], GUID_DEVCLASS_NET) )
                                    {
                                        AddToTreeEx( hwndTree,
                                                    TVI_ROOT,
                                                    pnccToBindUnbind,
                                                    TRUE );
                                    }
                                    else
                                    {
                                        AddToTreeEx( hwndTree,
                                                    TVI_ROOT,
                                                    pnccToBindUnbind,
                                                    FALSE );
                                    }

                                    dwCount++;
                                }
                            }
                        }

                        ReleaseRef( pnccToBindUnbind );

                        hr = HrGetNextComponent( pencc, &pnccToBindUnbind );
                    }

                    ReleaseRef( pnccb );
                }
                else {
                    ErrMsg( hr,
                            L"Couldn't get the component binding interface "
                            L"of %s.",
                            lpszInfId );
                }

                ReleaseRef( pencc );
            }
            else {
                ErrMsg( hr,
                        L"Couldn't get the network component enumerator "
                        L"interface." );
            }

            ReleaseRef( pncc );

        }
        else {
            ErrMsg( hr,
                    L"Couldn't get an interface pointer to %s.",
                    lpszInfId );
        }

        HrReleaseINetCfg( pnc,
                          TRUE );
    }
    else {
        if ( (hr == NETCFG_E_NO_WRITE_LOCK) && lpszApp ) {
            ErrMsg( hr,
                    L"%s currently holds the lock, try later.",
                    lpszApp );

            CoTaskMemFree( lpszApp );
        }
        else {
            ErrMsg( hr,
                    L"Couldn't get the notify object interface." );
        }
    }

    return dwCount;
}

//
// Function:  BindUnbind
//
// Purpose:   Bind/unbind a network component.
//
// Arguments:
//    lpszInfId  [in]  PnpID of the network component to bind/unbind.
//    hwndTree   [in]  Tree handle.
//    fBind      [in]  if TRUE, bind, otherwise unbind.
//
// Returns:   TRUE on success.
//
// Notes:
//

BOOL BindUnbind ( _In_ LPWSTR lpszInfId,
                 HWND hwndTree,
                 BOOL fBind)
{
    INetCfg                   *pnc;
    INetCfgComponent          *pncc;
    INetCfgComponentBindings  *pnccb;
    INetCfgComponent          *pnccToBindUnbind;
    LPWSTR                    lpszApp;
    HTREEITEM                 hTreeItem;
    TVITEMW                   tvItem;
    HRESULT                   hr;
    BOOL                      fChange;


    hr = HrGetINetCfg( TRUE,
                       APP_NAME,
                       &pnc,
                       &lpszApp );

    fChange = FALSE;

    if ( hr == S_OK ) {

        //
        // Get a reference to the network component.
        //

        hr = pnc->FindComponent( lpszInfId,
                                 &pncc );
        if ( hr == S_OK ) {

            //
            // Get a reference to the component's binding.
            //

            hr = pncc->QueryInterface( IID_INetCfgComponentBindings,
                                         (PVOID *)&pnccb );
            if ( hr == S_OK ) {

                //
                // Start with the root item.
                //

                hTreeItem = TreeView_GetRoot( hwndTree );

                //
                // Bind/unbind the network component with every component
                // that is checked.
                //

                while ( hTreeItem ) {

                    ZeroMemory( &tvItem,
                                sizeof(TVITEMW) );

                    tvItem.hItem = hTreeItem;
                    tvItem.mask = TVIF_PARAM | TVIF_STATE;
                    tvItem.stateMask = TVIS_STATEIMAGEMASK;

                    if ( TreeView_GetItem(hwndTree,
                                          &tvItem) ) {

                        //
                        // Is the network component selected?
                        //

                        if ( (tvItem.state >> 12) == 2 ) {

                            //
                            // Get a reference to the selected component.
                            //
                            // For adapters, lParam is pnp instance id. So, FindComponent will fail. In that case, search
                            // for a network adapter matching the pnp instance id. This will ensure that we get to the right
                            // network adapter in case there are multiple identical adapters in the system.

                            hr = pnc->FindComponent( (LPWSTR)tvItem.lParam,
                                                     &pnccToBindUnbind );
                            if ( hr != S_OK )
                            {
                                hr = HrFindNetComponentByPnpId( pnc,
                                                                (LPWSTR)tvItem.lParam,
                                                                &pnccToBindUnbind );
                            }

                            if ( hr == S_OK ) {

                                if ( fBind ) {

                                    //
                                    // Bind the component to the selected component.
                                    //

                                    hr = pnccb->BindTo( pnccToBindUnbind );

                                    if ( !fChange ) {
                                        fChange = hr == S_OK;
                                    }

                                    if ( hr != S_OK ) {
                                        ErrMsg( hr,
                                                L"%s couldn't be bound to %s.",
                                                     lpszInfId, (LPWSTR)tvItem.lParam );
                                    }
                                }
                                else {
                                    //
                                    // Unbind the component from the selected component.
                                    //

                                    hr = pnccb->UnbindFrom( pnccToBindUnbind );

                                    if ( !fChange ) {
                                        fChange = hr == S_OK;
                                    }

                                    if ( hr != S_OK ) {
                                        ErrMsg( hr,
                                                L"%s couldn't be unbound from %s.",
                                                     lpszInfId, (LPWSTR)tvItem.lParam );
                                    }
                                }

                                ReleaseRef( pnccToBindUnbind );
                            }
                            else {
                                ErrMsg( hr,
                                        L"Couldn't get an interface pointer to %s. "
                                        L"%s will not be bound to it.",
                                        (LPWSTR)tvItem.lParam,
                                        lpszInfId );
                            }
                        }
                    }

                    //
                    // Get the next item.
                    //

                    hTreeItem = TreeView_GetNextSibling( hwndTree,
                                                         hTreeItem );
                }

                ReleaseRef( pnccb );
            }
            else {
                ErrMsg( hr,
                        L"Couldn't get a binding interface of %s.",
                        lpszInfId );
            }

            ReleaseRef( pncc );
        }
        else {
            ErrMsg( hr,
                    L"Couldn't get an interface pointer to %s.",
                    lpszInfId );
        }

        //
        // If one or more network components have been bound/unbound,
        // apply the changes.
        //

        if ( fChange ) {
            hr = pnc->Apply();

            fChange = hr == S_OK;
        }

        HrReleaseINetCfg( pnc,
                          TRUE );
    }
    else {
        if ( (hr == NETCFG_E_NO_WRITE_LOCK) && lpszApp ) {
            ErrMsg( hr,
                    L"%s currently holds the lock, try later.",
                    lpszApp );

            CoTaskMemFree( lpszApp );
        }
        else {
            ErrMsg( hr,
                    L"Couldn't get the notify object interface." );
        }
    }

    return fChange;
}

//
// Function:  ListInstalledComponents
//
// Purpose:   List installed network components of specific class.
//
// Arguments:
//    hwndTree      [in]  Tree handle in which to list.
//    pguidClass    [in]  Class GUID of the network component class.
//
// Returns:   None.
//
// Notes:
//

VOID ListInstalledComponents (HWND hwndTree,
                              const GUID *pguidClass)
{
    INetCfg              *pnc;
    IEnumNetCfgComponent *pencc;
    INetCfgComponent     *pncc;
    LPWSTR               lpszApp;
    HRESULT              hr;


    hr = HrGetINetCfg( FALSE,
                       APP_NAME,
                       &pnc,
                       &lpszApp );

    if ( hr == S_OK ) {

        //
        // Get Component Enumerator Interface.
        //

        hr = HrGetComponentEnum( pnc,
                                 pguidClass,
                                 &pencc );
        if ( hr == S_OK ) {

            hr = HrGetFirstComponent( pencc, &pncc );

            while( hr == S_OK ) {

                //
                // Add an item to the tree for the network component.
                //

                AddToTree( hwndTree,
                           TVI_ROOT,
                           pncc );

                ReleaseRef( pncc );

                hr = HrGetNextComponent( pencc, &pncc );
            }

            ReleaseRef( pencc );
        }
        else {
            ErrMsg( hr,
                    L"Failed to get the network component enumerator." );
        }

        HrReleaseINetCfg( pnc, FALSE );
    }
    else {
        if ( (hr == NETCFG_E_NO_WRITE_LOCK) && lpszApp ) {
            ErrMsg( hr,
                    L"%s currently holds the lock, try later.",
                    lpszApp );

            CoTaskMemFree( lpszApp );
        }
        else {
            ErrMsg( hr,
                    L"Couldn't get the notify object interface." );
        }
    }

    return;
}

//
// Function:  UninstallComponent
//
// Purpose:   Uninstall a network component.
//
// Arguments:
//    lpszInfId  [in]  PnpID of the network component to uninstall.
//
// Returns:   S_OK on success, otherwise an error code.
//
// Notes:
//

HRESULT UninstallComponent ( _In_ LPWSTR lpszInfId)
{
    INetCfg              *pnc;
    INetCfgComponent     *pncc;
    INetCfgClass         *pncClass;
    INetCfgClassSetup    *pncClassSetup;
    LPWSTR               lpszApp;
    GUID                 guidClass;
    OBO_TOKEN            obo;
    HRESULT              hr;

    hr = HrGetINetCfg( TRUE,
                       APP_NAME,
                       &pnc,
                       &lpszApp );

    if ( hr == S_OK ) {

        //
        // Get a reference to the network component to uninstall.
        //

        hr = pnc->FindComponent( lpszInfId,
                                 &pncc );

        if ( hr == S_OK ) {

            //
            // Get the class GUID.
            //

            hr = pncc->GetClassGuid( &guidClass );

            if ( hr == S_OK ) {

                //
                // Get a reference to component's class.
                //

                hr = pnc->QueryNetCfgClass( &guidClass,
                                            IID_INetCfgClass,
                                            (PVOID *)&pncClass );
                if ( hr == S_OK ) {

                    //
                    // Get the setup interface.
                    //

                    hr = pncClass->QueryInterface( IID_INetCfgClassSetup,
                                                   (LPVOID *)&pncClassSetup );

                    if ( hr == S_OK ) {

                        //
                        // Uninstall the component.
                        //

                        ZeroMemory( &obo,
                                    sizeof(OBO_TOKEN) );

                        obo.Type = OBO_USER;

                        hr = pncClassSetup->DeInstall( pncc,
                                                       &obo,
                                                       NULL );
                        if ( (hr == S_OK) || (hr == NETCFG_S_REBOOT) ) {

                            hr = pnc->Apply();

                            if ( (hr != S_OK) && (hr != NETCFG_S_REBOOT) ) {
                                ErrMsg( hr,
                                        L"Couldn't apply the changes after"
                                        L" uninstalling %s.",
                                        lpszInfId );
                            }
                        }
                        else {
                            ErrMsg( hr,
                                    L"Failed to uninstall %s.",
                                    lpszInfId );
                        }

                        ReleaseRef( pncClassSetup );
                    }
                    else {
                        ErrMsg( hr,
                                L"Couldn't get an interface to setup class." );
                    }

                    ReleaseRef( pncClass );
                }
                else {
                    ErrMsg( hr,
                            L"Couldn't get a pointer to class interface "
                            L"of %s.",
                            lpszInfId );
                }
            }
            else {
                ErrMsg( hr,
                        L"Couldn't get the class guid of %s.",
                        lpszInfId );
            }

            ReleaseRef( pncc );
        }
        else {
            ErrMsg( hr,
                    L"Couldn't get an interface pointer to %s.",
                    lpszInfId );
        }

        HrReleaseINetCfg( pnc,
                          TRUE );
    }
    else {
        if ( (hr == NETCFG_E_NO_WRITE_LOCK) && lpszApp ) {
            ErrMsg( hr,
                    L"%s currently holds the lock, try later.",
                    lpszApp );

            CoTaskMemFree( lpszApp );
        }
        else {
            ErrMsg( hr,
                    L"Couldn't get the notify object interface." );
        }
    }

    return hr;
}
