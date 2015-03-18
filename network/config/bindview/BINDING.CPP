//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       B I N D I N G . C P P
//
//  Contents:   Functions to illustrate
//              o How to enumerate binding paths.
//              o How to enumerate binding interfaces.
//              o How to enable/disable bindings.
//
//  Notes:      
//
//  Author:     Alok Sinha    15-May-01
//
//----------------------------------------------------------------------------

#include "bindview.h"

//
// Function:  WriteBindings
//
// Purpose:   Write bindings to specified file.
//
// Arguments:
//    fp  [in]  File handle.
//
// Returns:   None.
//
// Notes:
//

VOID WriteBindings (FILE *fp)
{
    INetCfg              *pnc;
    IEnumNetCfgComponent *pencc;
    INetCfgComponent     *pncc;
    LPWSTR               lpszApp;
    HRESULT              hr;
    UINT                 i;


    hr = HrGetINetCfg( FALSE,
                       APP_NAME,
                       &pnc,
                       &lpszApp );

    if ( hr == S_OK ) {

        for (i=CLIENTS_SELECTED; i <= PROTOCOLS_SELECTED; ++i) {

            fwprintf( fp, L"--- Bindings of %s ---\n", lpszNetClass[i] );

            //
            // Get Component Enumerator Interface.
            //

            hr = HrGetComponentEnum( pnc,
                                     pguidNetClass[i],
                                     &pencc );
            if ( hr == S_OK ) {

                hr = HrGetFirstComponent( pencc, &pncc );

                while( hr == S_OK ) {

                    //
                    // Write bindings of the component.
                    //

                    WriteBindingPath( fp,
                                      pncc );
                    ReleaseRef( pncc );

                    fwprintf( fp, L"\n" );

                    hr = HrGetNextComponent( pencc, &pncc );
                }

                fwprintf( fp, L"\n" );

                //
                // S_FALSE merely indicates that there are no more components.
                //

                if ( hr == S_FALSE ) {
                    hr = S_OK;
                }

                ReleaseRef( pencc );
            }
            else {
                ErrMsg( hr,
                        L"Couldn't get the component enumerator interface." );
            }
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
// Function:  WriteBindingPath
//
// Purpose:   Write binding paths of a component.
//
// Arguments:
//    fp    [in]  File handle.
//    pncc  [in]  Network component.
//
// Returns:   None.
//
// Notes:
//

VOID WriteBindingPath (FILE *fp,
                       INetCfgComponent *pncc)
{
    IEnumNetCfgBindingPath  *pencbp;
    INetCfgBindingPath      *pncbp;
    LPWSTR                  lpszName;
    HRESULT                 hr;

    //
    // Write the first component's name.
    //

    hr = pncc->GetDisplayName( &lpszName );

    if ( hr == S_OK ) {
        fwprintf( fp, L"\n%s", lpszName );
    }
    else {
        ErrMsg( hr,
                L"Unable to get the display name of a component, "
                L" some binding paths will not be written." );

       return;
    }

    //
    // Get binding path enumerator.
    //

    hr = HrGetBindingPathEnum( pncc,
                               EBP_BELOW,
                               &pencbp );
    if ( hr == S_OK ) {

        hr = HrGetFirstBindingPath( pencbp,
                                    &pncbp );

        while( hr == S_OK ) {

            //
            // Write interfaces of the binding path.
            //

            WriteInterfaces( fp,
                             pncbp );

            ReleaseRef( pncbp );

            hr = HrGetNextBindingPath( pencbp,
                                       &pncbp );
            if ( hr == S_OK ) {
                fwprintf( fp, L"\n%s", lpszName );
            }
        }
  
        ReleaseRef( pencbp );
    }
    else {
        ErrMsg( hr,
                L"Couldn't get the binding path enumerator of %s. "
                L"Its binding paths will not be written.",
                lpszName );
    }

    CoTaskMemFree( lpszName );
    return;
}

//
// Function:  WriteInterfaces
//
// Purpose:   Write bindings to specified file.
//
// Arguments:
//    fp     [in]  File handle.
//    pncbp  [in]  Binding path.
//
// Returns:   None.
//
// Notes:
//

VOID WriteInterfaces (FILE *fp,
                      INetCfgBindingPath *pncbp)
{
    IEnumNetCfgBindingInterface *pencbi;
    INetCfgBindingInterface     *pncbi;
    INetCfgComponent            *pnccLower;
    LPWSTR                      lpszName;
    HRESULT                     hr;

    hr = HrGetBindingInterfaceEnum( pncbp,
                                    &pencbi );

    if ( hr == S_OK ) {

        hr = HrGetFirstBindingInterface( pencbi,
                                         &pncbi );

        //
        // Write lower component of each interface.
        //

        while( hr == S_OK ) {

            hr = pncbi->GetLowerComponent ( &pnccLower );

            if ( hr == S_OK ) {

                hr = pnccLower->GetDisplayName( &lpszName );
                if ( hr == S_OK ) {
                    fwprintf( fp, L"-->%s", lpszName );
                    CoTaskMemFree( lpszName );
                }
            }

            ReleaseRef( pnccLower );
            ReleaseRef( pncbi );

            hr = HrGetNextBindingInterface( pencbi,
                                            &pncbi );
        }

        ReleaseRef( pencbi );
    }
    else {
        ErrMsg( hr,
                L"Couldn't get the binding interface enumerator."
                L"The binding interfaces will not be shown." );
    }

    return;
}

//
// Function:  EnumNetBindings
//
// Purpose:   Enumerate components and their bindings.
//
// Arguments:
//    hwndTree        [in]  Tree handle.
//    uiTypeSelected  [in]  Type of network component selected.
//
// Returns:   TRUE on success.
//
// Notes:
//

BOOL EnumNetBindings (HWND hwndTree,
                      UINT uiTypeSelected)
{
    INetCfg              *pnc;
    IEnumNetCfgComponent *pencc;
    INetCfgComponent     *pncc;
    LPWSTR               lpszApp;
    HTREEITEM            hTreeItem;
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
                                 pguidNetClass[uiTypeSelected],
                                 &pencc );
        if ( hr == S_OK ) {

            hr = HrGetFirstComponent( pencc, &pncc );

            while( hr == S_OK ) {

                //
                // Add the component's name to the tree.
                //

                hTreeItem = AddToTree( hwndTree,
                                       TVI_ROOT,
                                       pncc );
                if ( hTreeItem ) {

                    //
                    // Enumerate bindings.
                    //

                    ListBindings( pncc,
                                  hwndTree,
                                  hTreeItem );
                }

                ReleaseRef( pncc );

                hr = HrGetNextComponent( pencc, &pncc );
            }

            //
            // S_FALSE merely indicates that there are no more components.
            //

            if ( hr == S_FALSE ) {
                hr = S_OK;
            }

            ReleaseRef( pencc );
        }
        else {
            ErrMsg( hr,
                    L"Couldn't get the component enumerator interface." );
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

    return hr == S_OK;
}

//
// Function:  ListBindings
//
// Purpose:   Enumerate bindings of network components.
//
// Arguments:
//    pncc          [in]  Network component.
//    hwndTree      [in]  Tree handle.
//    hTreeItemRoot [in]   Parent item.
//
// Returns:   None.
//
// Notes:
//

VOID ListBindings (INetCfgComponent *pncc,
                   HWND hwndTree,
                   HTREEITEM hTreeItemRoot)
{
    IEnumNetCfgBindingPath      *pencbp;
    INetCfgBindingPath          *pncbp;
    HTREEITEM                   hTreeItem;
    ULONG                       ulIndex;
    HRESULT                     hr;
  
    hr = HrGetBindingPathEnum( pncc,
                               EBP_BELOW,
                               &pencbp );
    if ( hr == S_OK ) {

        hr = HrGetFirstBindingPath( pencbp,
                                    &pncbp );

        ulIndex = 1;

        while( hr == S_OK ) {

            //
            // Add an item for the binding path.
            //

            hTreeItem = AddBindNameToTree( pncbp,
                                           hwndTree,
                                           hTreeItemRoot,
                                           ulIndex );

            if ( hTreeItem ) {

                //
                // Enumerate interfaces.
                //

                ListInterfaces( pncbp,
                                hwndTree,
                                hTreeItem );
            }

            ReleaseRef( pncbp );

            hr = HrGetNextBindingPath( pencbp,
                                       &pncbp );

            ulIndex++;
        }
  
        ReleaseRef( pencbp );
    }
    else {
       LPWSTR  lpszName;

       if ( pncc->GetDisplayName(&lpszName) == S_OK ) {

          ErrMsg( hr,
                  L"Couldn't get the binding path enumerator of %s. "
                  L"Its binding paths will not be shown.",
                  lpszName );

          CoTaskMemFree( lpszName );
       }
       else {
          ErrMsg( hr,
                  L"Couldn't get the binding path enumerator of a "
                  L"network component. The binding paths will not "
                  L"be shown." );
       }
    }

    return;
}

//
// Function:  ListInterfaces
//
// Purpose:   Enumerate interfaces of a binding path.
//
// Arguments:
//    pncbp         [in]  Binding path.
//    hwndTree      [in]  Tree handle.
//    hTreeItemRoot [in]  Parent item.
//
// Returns:   None.
//
// Notes:
//

VOID ListInterfaces (INetCfgBindingPath *pncbp,
                     HWND hwndTree,
                     HTREEITEM hTreeItemRoot)
{
    IEnumNetCfgBindingInterface *pencbi;
    INetCfgBindingInterface     *pncbi;
    INetCfgComponent            *pnccBound;
    HTREEITEM                   hTreeItem;
    HRESULT                     hr;

    hr = HrGetBindingInterfaceEnum( pncbp,
                                    &pencbi );

    if ( hr == S_OK ) {

        hr = HrGetFirstBindingInterface( pencbi,
                                         &pncbi );
        hTreeItem = hTreeItemRoot;

        while( (hr == S_OK) && hTreeItem ) {

            //
            // Add lower component of every interface to the tree.
            //

            pncbi->GetLowerComponent( &pnccBound );

            hTreeItem = AddToTree( hwndTree,
                                   hTreeItem,
                                   pnccBound );

            ReleaseRef( pnccBound );
            ReleaseRef( pncbi );

            hr = HrGetNextBindingInterface( pencbi,
                                            &pncbi );
        }

        //
        // If hr is S_OK then, the loop terminated due to error in adding
        // the binding path to the tree and pncbi has a reference to an
        // interface.
        //

        if ( hr == S_OK ) {

            ReleaseRef( pncbi );
        }

        ReleaseRef( pencbi );
    }
    else {
        ErrMsg( hr,
                L"Couldn't get the binding interface enumerator."
                L"The binding interfaces will not be shown." );
    }

    return;
}

//
// Function:  HandleBindingPathOperation
//
// Purpose:   
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

VOID HandleBindingPathOperation (HWND hwndOwner,
                                 ULONG ulSelection,
                                 HTREEITEM hItem,
                                 LPARAM lParam)
{
    switch( ulSelection ) {

        case IDI_ENABLE:
        case IDI_DISABLE:

            //
            // Enable/disable binding path.
            //

            EnableBindingPath( hwndOwner,
                               hItem,
                               (LPWSTR)lParam,
                               ulSelection == IDI_ENABLE );
    }

    return;
}

//
// Function:  EnableBindingPath
//
// Purpose:   Enable/disable binding path.
//
// Arguments:
//    hwndOwner      [in]  Owner window.
//    hItem          [in]  Item handle of the binding path.
//    lpszPathToken  [in]  Path token of the binding path.
//    fEnable        [in]  if TRUE, enable, otherwise disable.
//
// Returns:   None.
//
// Notes:
//

VOID
EnableBindingPath (
    HWND hwndOwner,
    HTREEITEM hItem,
    _In_ LPWSTR lpszPathToken,
    BOOL fEnable)
{
    INetCfg              *pnc;
    INetCfgBindingPath   *pncbp;
    LPWSTR               lpszInfId;
    LPWSTR               lpszApp;
    HRESULT              hr;

    //
    // Get PnpID of the owner component.
    //

    lpszInfId = GetComponentId( hwndOwner,
                                hItem );

    if ( lpszInfId ) {

        hr = HrGetINetCfg( TRUE,
                           APP_NAME,
                           &pnc,
                           &lpszApp );

        if ( hr == S_OK ) {

            //
            // Find the binding path reference.
            //

            pncbp = FindBindingPath( pnc,
                                     lpszInfId,
                                     lpszPathToken );

            if ( pncbp ) {

                //
                // Enable/disable.
                //

                hr = pncbp->Enable( fEnable );

                if ( hr == S_OK ) {
                    hr = pnc->Apply();

                    if ( hr == S_OK ) {

                        //
                        // Refreshe the state of the item representing the
                        // binding path.
                        //

                        RefreshItemState( hwndOwner,
                                          hItem,
                                          fEnable );
                    }
                    else {
                        ErrMsg( hr,
                                L"Failed to apply changes to the binding path." );
                    }
                }
                else {
                    if ( fEnable ) {
                        ErrMsg( hr,
                                L"Failed to enable the binding path." );
                    }
                    else {
                        ErrMsg( hr,
                                L"Failed to disable the binding path." );
                    }
                }

                ReleaseRef( pncbp );
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
    }
    else {
          ErrMsg( HRESULT_FROM_WIN32(GetLastError()),
                  L"Couldn't determine the owner of the binding path." );
    }

    return;
}

//
// Function:  GetComponentId
//
// Purpose:   Find the PnpID of a network component.
//
// Arguments:
//    hwndTree  [in]  Tree handle.
//    hItem     [in]  Item handle of the binding path.
//
// Returns:   PnpID of the network component.
//
// Notes:
//

LPWSTR GetComponentId (HWND hwndTree,
                       HTREEITEM hItem)
{
    LPWSTR       lpszInfId;
    HTREEITEM    hTreeItemParent;
    TVITEMW      tvItem;

    lpszInfId = NULL;

    //
    // Get the item handle of the owner component.
    //

    hTreeItemParent = TreeView_GetParent( hwndTree,
                                          hItem );
    if ( hTreeItemParent ) {

        //
        // Get lParam of the owner component. lParam is the PnpID.
        //

        ZeroMemory( &tvItem,
                    sizeof(TVITEMW) );

        tvItem.hItem = hTreeItemParent;
        tvItem.mask = TVIF_PARAM;

        if ( TreeView_GetItem(hwndTree,
                              &tvItem) ) {

            lpszInfId = (LPWSTR)tvItem.lParam;
        }
    }

    return lpszInfId;
}

//
// Function:  WriteBindings
//
// Purpose:   Find the binding path with a give path token.
//
// Arguments:
//    pnc                    [in]  INetCfg reference.
//    lpszInfId              [in]  PnpID of the network component.
//    lpszPathTokenSelected  [in]  Path token of the binding path to search.
//
// Returns:   Reference to the binding path on success, otherwise NULL.
//
// Notes:
//

INetCfgBindingPath *
FindBindingPath (
    INetCfg *pnc,
    _In_ LPWSTR lpszInfId,
    _In_ LPWSTR lpszPathTokenSelected)
{
    INetCfgComponent       *pncc = NULL;
    IEnumNetCfgBindingPath *pencbp = NULL;
    INetCfgBindingPath     *pncbp = NULL;
    LPWSTR                 lpszPathToken;
    HRESULT                hr;
    BOOL                   fFound;


    fFound = FALSE;

    //
    // Get the component reference.
    //

    hr = pnc->FindComponent( lpszInfId,
                             &pncc );

    if ( hr == S_OK ) {
     
        hr = HrGetBindingPathEnum( pncc,
                                   EBP_BELOW,
                                   &pencbp );
        if ( hr == S_OK ) {

            hr = HrGetFirstBindingPath( pencbp,
                                        &pncbp );

            // Enumerate each binding path and find the one
            // whose path token matches the specified one.
            //

            while ( !fFound && (hr == S_OK) ) {

                hr = pncbp->GetPathToken( &lpszPathToken );

                if ( hr == S_OK ) {
                    fFound = !wcscmp( lpszPathToken,
                                       lpszPathTokenSelected );

                    CoTaskMemFree( lpszPathToken );
                }

                if ( !fFound ) {
                    ReleaseRef( pncbp );

                    hr = HrGetNextBindingPath( pencbp,
                                               &pncbp );
                }
            }

            ReleaseRef( pencbp );
        }
        else {
            ErrMsg( hr,
                    L"Couldn't get the binding path enumerator interface." );
        }
    }
    else {
        ErrMsg( hr,
                L"Couldn't get an interface pointer to %s.",
                lpszInfId );
    }

    return (fFound) ? pncbp : NULL;
}
