//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992-2001.
//
//  File:       A D A P T E R . C P P
//
//  Contents:   Physical adapter class definition.
//
//  Notes:
//
//  Author:     Alok Sinha
//
//----------------------------------------------------------------------------

#include "adapter.h"
#include "common.h"

#ifdef  CUSTOM_EVENTS
#include "public.h"
#endif

//+---------------------------------------------------------------------------
//
// Function:  CMuxPhysicalAdapter::CMuxPhysicalAdapter
//
// Purpose:   Constructor for class CMuxPhysicalAdapter
//
// Arguments: None
//
// Returns:
//
// Notes:
//

CMuxPhysicalAdapter::CMuxPhysicalAdapter (INetCfg *pnc,
                                          GUID *pguidAdapter)
{

    TraceMsg( L"-->CMuxPhysicalAdapter::CMuxPhysicalAdapter(Constructor).\n" );

    m_pnc = pnc;
    m_pnc->AddRef();

    CopyMemory( &m_guidAdapter,
                pguidAdapter,
                sizeof(GUID) );

    TraceMsg( L"<--CMuxPhysicalAdapter::CMuxPhysicalAdapter(Constructor).\n" );
}

//+---------------------------------------------------------------------------
//
// Function:  CMuxPhysicalAdapter::~CMuxPhysicalAdapter
//
// Purpose:   Destructor for class CMuxPhysicalAdapter
//
// Arguments: None
//
// Returns:
//
// Notes:
//

CMuxPhysicalAdapter::~CMuxPhysicalAdapter (VOID)
{
    CMuxVirtualMiniport  *pMiniport;
    DWORD                dwMiniportCount;
    DWORD                i;


    TraceMsg( L"-->CMuxPhysicalAdapter::~CMuxPhysicalAdapter(Destructor).\n" );

    //
    // Delete all the instances representing the virtual miniports.
    // We are only deleting the class instances, not uninstalling the
    // the virtual miniports.
    //

    dwMiniportCount = m_MiniportList.ListCount();

    for (i=0; i < dwMiniportCount; ++i) {

        pMiniport = NULL;
        m_MiniportList.Remove( &pMiniport );
        delete pMiniport;
    }

    dwMiniportCount = m_MiniportsToAdd.ListCount();

    for (i=0; i < dwMiniportCount; ++i) {

        pMiniport = NULL;
        m_MiniportsToAdd.Remove( &pMiniport );
        delete pMiniport;
    }

    dwMiniportCount = m_MiniportsToRemove.ListCount();

    for (i=0; i < dwMiniportCount; ++i) {

        pMiniport = NULL;
        m_MiniportsToRemove.Remove( &pMiniport );
        delete pMiniport;
    }

    ReleaseObj( m_pnc );

    TraceMsg( L"<--CMuxPhysicalAdapter::~CMuxPhysicalAdapter(Destructor).\n" );

}

//+---------------------------------------------------------------------------
//
// Function:  CMuxPhysicalAdapter::LoadConfiguration
//
// Purpose:   Read the registry to get the device IDs of the 
//            virtual miniports installed on the adapter and
//            crate an instance to represent each virtual miniport.
//
// Arguments: None
//
// Returns: S_OK on success, otherwise an error code.
//
// Notes:
//

HRESULT CMuxPhysicalAdapter::LoadConfiguration (VOID)
{
    HKEY                    hkeyAdapterGuid;
    WCHAR                   szAdapterGuidKey[MAX_PATH+1];
    WCHAR                   szAdapterGuid[MAX_PATH+1];
    LPWSTR                  lpMiniportList;
    LPWSTR                  lpMiniport;
    LPWSTR                  lpMiniportGuid;
    DWORD                   dwDisp;
    CMuxVirtualMiniport     *pMiniport;
    GUID                    guidMiniport;
    DWORD                   dwBytes;
    LONG                    lResult;

    TraceMsg( L"-->CMuxPhysicalAdapter::LoadConfiguration.\n" );

    //
    // Build the registry key using the adapter guid under which 
    // device IDs of the virtual miniports are stored.
    //

    int numChars = StringFromGUID2( m_guidAdapter,
                    szAdapterGuid,
                    MAX_PATH+1 );
    if (numChars == 0) {

        return HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
    }

    StringCchPrintfW ( szAdapterGuidKey,
        celems(szAdapterGuidKey),
        L"%s\\%s",
        c_szAdapterList,
        szAdapterGuid );
    
    szAdapterGuidKey[MAX_PATH]='\0';
    lResult = RegCreateKeyExW( HKEY_LOCAL_MACHINE,
                                szAdapterGuidKey,
                                0,
                                NULL,
                                REG_OPTION_NON_VOLATILE,
                                KEY_ALL_ACCESS,
                                NULL,
                                &hkeyAdapterGuid,
                                &dwDisp);

    if ( lResult == ERROR_SUCCESS ) {

        //
        // If dwDisp indicates that a new key is created then, we know there
        // is no virtual miniport currently listed underneath and we simply
        // return.
        //

        if ( dwDisp != REG_CREATED_NEW_KEY ) {

            dwBytes = 0;
            lResult =  RegQueryValueExW(
                                        hkeyAdapterGuid,
                                        c_szUpperBindings,
                                        NULL,
                                        NULL,
                                        NULL,
                                        &dwBytes );
            if(lResult != ERROR_SUCCESS){
                // you may do something
            }



            //alloc lpMiniportList with dwBytes elements and initialzes each to be 0
            lpMiniportList = (LPWSTR)calloc( dwBytes, 1 );

            if ( lpMiniportList != NULL ) {

                lResult =  RegQueryValueExW(
                                    hkeyAdapterGuid,
                                    c_szUpperBindings,
                                    NULL,
                                    NULL,
                                    (LPBYTE)lpMiniportList,
                                    &dwBytes );

                if ( lResult == ERROR_SUCCESS ) {

                    lpMiniport = lpMiniportList;
                    

#ifndef PASSTHRU_NOTIFY

                    //
                    // In case of mux, c_szUpperBindings is a REG_MULTI_SZ string.
                    //
                  
                    lpMiniport[dwBytes-1] = '\0';
                    while ( wcslen(lpMiniport) ) {

                        lpMiniportGuid = RemoveDevicePrefix( lpMiniport );

                        TraceMsg( L"   Loading configuration for miniport %s...\n",
                        lpMiniportGuid );

                        if ( lpMiniportGuid != NULL ) {

                            HRESULT hrResult = CLSIDFromString( lpMiniportGuid,
                                             &guidMiniport );

                            if (hrResult != S_OK) {

                                lResult = ERROR_INVALID_PARAMETER;
                            }

                            //
                            // Create an instance representing the virtual miniport.
                            //

                            #pragma prefast(suppress:8197, "The instance is freed in the destructor")
                            pMiniport = new CMuxVirtualMiniport( m_pnc,
                                                                 &guidMiniport,
                                                                 &m_guidAdapter );

                            if ( pMiniport ) {

                                //
                                // Load any miniport specific configuration.
                                //

                                pMiniport->LoadConfiguration();

                                //
                                // Save the miniport instance in a list.
                                //

                                m_MiniportList.Insert( pMiniport,
                                                       guidMiniport );

                            }

                            free( lpMiniportGuid );
                        }

                        //
                        // Get next miniport guid.
                        //

                        lpMiniport += wcslen(lpMiniport) + 1;
                    }

#else

                    //
                    // In case of the passthru driver, c_szUpperBindings is
                    // a reg_sz string.
                    //

                    lpMiniportGuid = RemoveDevicePrefix( lpMiniport );

                    TraceMsg( L"   Loading configuration for miniport %s...\n",
                              lpMiniportGuid );

                    if ( lpMiniportGuid ) {

                        CLSIDFromString( lpMiniportGuid,
                                         &guidMiniport );

                        //
                        // Create an instance representing the virtual miniport.
                        //

                        pMiniport = new CMuxVirtualMiniport( m_pnc,
                                                             &guidMiniport,
                                                             &m_guidAdapter );

                        if ( pMiniport ) {

                            //
                            // Load any miniport specific configuration.
                            //

                            pMiniport->LoadConfiguration();

                            //
                            // Save the miniport instance in a list.
                            //

                            m_MiniportList.Insert( pMiniport,
                                                   guidMiniport );
                        }

                        free( lpMiniportGuid );
                    }
#endif
                }
                else {
                    TraceMsg( L"   Failed to read the registry value: %s.\n",
                              c_szUpperBindings );
                }

                free( lpMiniportList );
            }
            else {
                lResult = ERROR_NOT_ENOUGH_MEMORY;
            }
        }

        RegCloseKey( hkeyAdapterGuid );
    }
    else {

        TraceMsg( L"   Failed to open the registry key: %s.\n",
                  szAdapterGuidKey );
    }

    TraceMsg( L"<--CMuxPhysicalAdapter::LoadConfiguration(HRESULT = %x).\n",
              HRESULT_FROM_WIN32(lResult) );

    return HRESULT_FROM_WIN32(lResult);
}

//+---------------------------------------------------------------------------
//
// Function:  CMuxPhysicalAdapter::GetAdapterGUID
//
// Purpose:   Returns the adapter GUID.
//
// Arguments:
//          OUT pguidAdapter:  GUID of the adapter returned.
//
// Returns: None.
//
// Notes:
//

VOID CMuxPhysicalAdapter::GetAdapterGUID (GUID *pguidAdapter)
{
    TraceMsg( L"-->CMuxPhysicalAdapter::GetAdapterGUID.\n" );

    CopyMemory( pguidAdapter,
                &m_guidAdapter,
                sizeof(GUID) );

    TraceMsg( L"<--CMuxPhysicalAdapter::GetAdapterGUID.\n" );
}

//+---------------------------------------------------------------------------
//
// Function:  CMuxPhysicalAdapter::AddMiniport
//
// Purpose:   Puts the miniport instance into the list of newly added miniports.
//
// Arguments:
//          IN pMiniport:  A newly create miniport instance.
//
// Returns: S_OK on success, otherwize and error code.
//
// Notes:
//

HRESULT CMuxPhysicalAdapter::AddMiniport (CMuxVirtualMiniport *pMiniport)
{
    GUID    guidMiniport;
    HRESULT hr;

    TraceMsg( L"-->CMuxPhysicalAdapter::AddMiniport.\n" );

    pMiniport->GetMiniportGUID( &guidMiniport );

    hr = m_MiniportsToAdd.Insert( pMiniport,
                                  guidMiniport );

    TraceMsg( L"<--CMuxPhysicalAdapter::AddMiniport(HRESULT = %x).\n",
              hr );

    return hr;
}

//+---------------------------------------------------------------------------
//
// Function:  CMuxPhysicalAdapter::RemoveMiniport
//
// Purpose:   Remove a specified miniport instance from the list and
//            uninstalls the corresponding virtual miniport.
//
// Arguments:
//          IN pguidMiniportToRemove: GUID of the miniport to be removed
//                                    and uninstalled. If it is NULL then,
//                                    the first miniport instance is removed.
//
// Returns: S_OK on success, otherwize and error code.
//
// Notes:
//

HRESULT CMuxPhysicalAdapter::RemoveMiniport (GUID *pguidMiniportToRemove)
{
    CMuxVirtualMiniport  *pMiniport;
    GUID                 guidMiniport;
    HRESULT              hr;

    TraceMsg( L"-->CMuxPhysicalAdapter::RemoveMiniport.\n" );

    //
    // If miniport GUID specified then, delete that one.
    //

    if ( pguidMiniportToRemove ) {

        hr = m_MiniportList.RemoveByKey( *pguidMiniportToRemove,
                                      &pMiniport );
    }
    else {

        //
        // No GUID specified, so we just delete the first one.
        //

        hr = m_MiniportList.Remove( &pMiniport );
    }

    if ( hr == S_OK ) {

        pMiniport->GetMiniportGUID( &guidMiniport );

        m_MiniportsToRemove.Insert( pMiniport,
                                    guidMiniport );
        pMiniport->DeInstall();
    }

    TraceMsg( L"<--CMuxPhysicalAdapter::RemoveMiniport(HRESULT = %x).\n",
              hr );

    return hr;
}

//+---------------------------------------------------------------------------
//
// Function:  CMuxPhysicalAdapter::Remove
//
// Purpose:   Uninstall all the instances of virtual miniports.
//
// Arguments: None
//
// Returns: S_OK.
//
// Notes:
//

HRESULT CMuxPhysicalAdapter::Remove (VOID)
{
    CMuxVirtualMiniport  *pMiniport = NULL;
    GUID                    guidMiniport;
    DWORD                   dwMiniportCount;
    DWORD                   i;

    TraceMsg( L"-->CMuxPhysicalAdapter::Remove.\n" );

    dwMiniportCount = m_MiniportList.ListCount();

    TraceMsg ( L"   Removing %d miniports.\n",
               dwMiniportCount );

    for (i=0; i < dwMiniportCount; ++i) {

        pMiniport = NULL;
        m_MiniportList.Remove( &pMiniport );
        pMiniport->GetMiniportGUID( &guidMiniport );
        m_MiniportsToRemove.Insert( pMiniport,
                                    guidMiniport );
        pMiniport->DeInstall();
    }

    TraceMsg( L"<--CMuxPhysicalAdapter::Remove(HRESULT = %x).\n",
              S_OK );

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// Function:  CMuxPhysicalAdapter::ApplyRegistryChanges
//
// Purpose:   Update the registry depending on the actions performed.
//
// Arguments:
//          IN eApplyAction:  Action that was last performed.
//                            
//
// Returns: S_OK.
//
// Notes:
//        More than one action could have been performed by the user
//        but this function is called only once at the end. So, the argument
//        only denotes the very last action performed. For example, if the 
//        user deletes one miniport and adds two miniports then, the argument
//        will denote an add action.
//

HRESULT CMuxPhysicalAdapter::ApplyRegistryChanges (ConfigAction eApplyAction)
{
    HKEY                    hkeyAdapterList;
    HKEY                    hkeyAdapterGuid;
    WCHAR                   szAdapterGuid[MAX_PATH+1];
    CMuxVirtualMiniport     *pMiniport = NULL;
    DWORD                   dwMiniportCount;
    DWORD                   dwDisp;
    DWORD                   i;
    LONG                    lResult;
    HRESULT                 hr;


    TraceMsg( L"-->CMuxPhysicalAdapter::ApplyRegistryChanges.\n" );

    //
    // Open/create and then close the registry key to ensure that it does exist.
    //

    int numChars = StringFromGUID2( m_guidAdapter,
                     szAdapterGuid,
                     MAX_PATH+1 );

    if (numChars == 0) {

        return HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
    }


    lResult = RegCreateKeyExW( HKEY_LOCAL_MACHINE,
                               c_szAdapterList,
                               0,
                               NULL,
                               REG_OPTION_NON_VOLATILE,
                               KEY_ALL_ACCESS,
                               NULL,
                               &hkeyAdapterList,
                               &dwDisp);


    if ( lResult == ERROR_SUCCESS ) {

        lResult = RegCreateKeyExW( hkeyAdapterList,
                                   szAdapterGuid,
                                   0,
                                   NULL,
                                   REG_OPTION_NON_VOLATILE,
                                   KEY_ALL_ACCESS,
                                   NULL,
                                   &hkeyAdapterGuid,
                                   &dwDisp);

        if ( lResult == ERROR_SUCCESS ) {

            RegCloseKey( hkeyAdapterGuid );
        }
        else {
            TraceMsg( L"   Failed to create/open the registry key: %s\\%s.\n",
                      c_szAdapterList, szAdapterGuid );
        }

        RegCloseKey( hkeyAdapterList );
    }
    else {

        TraceMsg( L"   Failed to open the registry key: %s.\n",
                  c_szAdapterList );
    }

    //
    // Update the registry in case there were new miniports installed.
    //

    hr = HRESULT_FROM_WIN32( lResult );
    if( hr != S_OK){
        // you may do something
    }

    dwMiniportCount = m_MiniportsToAdd.ListCount();

    TraceMsg( L"   Applying registry changes when %d miniports added.\n",
              dwMiniportCount );

    for (i=0; i < dwMiniportCount; ++i) {

        m_MiniportsToAdd.Find( i,
                               &pMiniport );

        //
        // Do virtual miniport specific registry changes.
        //
        // We need to tell the miniport instance explicitly what the action
        // is.
        //

        hr = pMiniport->ApplyRegistryChanges( eActAdd );

        if ( hr != S_OK ) {

            TraceMsg( L"   Failed to apply registry changes to miniport(%d).\n",
                      i );

        }
    }



    //
    // Update the registry in case one or more miniports were uninstalled.
    //

    dwMiniportCount = m_MiniportsToRemove.ListCount();

    TraceMsg( L"   Applying registry changes when %d miniports removed.\n",
              dwMiniportCount );

    for (i=0; i < dwMiniportCount; ++i) {

        m_MiniportsToRemove.Find( i,
                                  &pMiniport );

        //
        // Do virtual miniport specific registry changes.
        //
        // We need to tell the miniport instance explicitly what the action
        // is.
        //

        hr = pMiniport->ApplyRegistryChanges( eActRemove );

        if ( hr != S_OK ) {

            TraceMsg( L"   Failed to apply registry changes to miniport(%d).\n",
                      i );

        }
    }

    //
    // If the adapter is being removed or the protocol is being uninstalled,
    // delete the adatper registry key.
    //

    if ( eApplyAction == eActRemove ) {

        //
        // Delete the adapter key.
        //

        lResult = RegCreateKeyExW( HKEY_LOCAL_MACHINE,
                                   c_szAdapterList,
                                   0,
                                   NULL,
                                   REG_OPTION_NON_VOLATILE,
                                   KEY_ALL_ACCESS,
                                   NULL,
                                   &hkeyAdapterList,
                                   &dwDisp);

        if ( lResult == ERROR_SUCCESS ) {

            TraceMsg( L"   Deleting the registry key: %s.\n", szAdapterGuid );

            RegDeleteKeyW( hkeyAdapterList, szAdapterGuid );
            RegCloseKey ( hkeyAdapterList );
        }
    }

    TraceMsg( L"<--CMuxPhysicalAdapter::ApplyRegistryChanges(HRESULT = %x).\n",
            S_OK );

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// Function:  CMuxPhysicalAdapter::ApplyPnpChanges
//
// Purpose:   Apply the PnP changes depending on the actions performed.
//
// Arguments:
//          IN pfCallback  :  SendPnpConfig Callback interface.
//          IN eApplyAction:  Action that was last performed.
//                            
//
// Returns: S_OK.
//
// Notes:
//        More than one action could have been performed by the user
//        but this function is called only once at the end. So, the argument
//        only denotes the very last action performed. For example, if the 
//        user deletes one miniport and adds two miniports then, the argument
//        will denote an add action.
//

HRESULT CMuxPhysicalAdapter::ApplyPnpChanges(
                    INetCfgPnpReconfigCallback *pfCallback,
                    ConfigAction eApplyAction)
{
    CMuxVirtualMiniport     *pMiniport = NULL;
    GUID                    guidMiniport;
    DWORD                   dwMiniportCount;
    DWORD                   i;
    HRESULT                 hr;

#ifdef CUSTOM_EVENTS    
    LPWSTR                  lpDevice;
    WCHAR                   szMiniportGuid[MAX_PATH+1];
    DWORD                   dwBytes;
    INetCfgComponent        *pncc;
    LPWSTR                  lpszBindName;
    PNOTIFY_CUSTOM_EVENT       lppnpEvent;
#endif

    UNREFERENCED_PARAMETER(eApplyAction);
    TraceMsg( L"-->CMuxPhysicalAdapter::ApplyPnpChanges.\n" );

#ifdef CUSTOM_EVENTS    

    //
    // Find the instance of the adapter to get its bindname.
    //

    hr = HrFindInstance( m_pnc,
                         m_guidAdapter,
                         &pncc );

    if ( hr == S_OK ) {

        hr = pncc->GetBindName( &lpszBindName );

        if ( hr != S_OK ) {
            TraceMsg( L"  GetBindName failed.(HRESULT = %x). PnP changes will not "
                      L"be applied and the driver will not be notified.\n",
                      hr );
        }

        ReleaseObj( pncc );
    }
    else {
        TraceMsg( L"  PnP changes will not "
               L"be applied and the driver will not be notified.\n",
               hr );
    }

#endif    

    dwMiniportCount = m_MiniportsToAdd.ListCount();

    TraceMsg( L"   Applying PnP changes to %d new miniports.\n",
            dwMiniportCount );

    for (i=0; i < dwMiniportCount; ++i) {

        pMiniport = NULL;
        m_MiniportsToAdd.Remove( &pMiniport );

        pMiniport->GetMiniportGUID( &guidMiniport );

        m_MiniportList.Insert( pMiniport,
                               guidMiniport );

        //
        // Do miniport specific Pnp Changes when they are added.
        //

        hr = pMiniport->ApplyPnpChanges( pfCallback,
                                         eActAdd );
        if( hr != S_OK){
            // you may do something
        }

#ifdef CUSTOM_EVENTS


        //
        // Notify the driver that one or more virtual miniports have been added.
        //

        int numChars = StringFromGUID2( guidMiniport,
                         szMiniportGuid,
                         MAX_PATH+1 );
        if (numChars == 0) {

            CoTaskMemFree(lpszBindName);

            return HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
        }

        lpDevice = AddDevicePrefix( szMiniportGuid );

        if ( lpDevice ) {

            dwBytes = sizeof(NOTIFY_CUSTOM_EVENT) +
                      ((wcslen(lpDevice) + 1) * sizeof(WCHAR));

            lppnpEvent = (PNOTIFY_CUSTOM_EVENT)malloc( dwBytes );

            if ( lppnpEvent ) {

                lppnpEvent->uSignature = NOTIFY_SIGNATURE;
                lppnpEvent->uEvent = MUX_CUSTOM_EVENT;
                wcscpy( lppnpEvent->szMiniport,
                       lpDevice );

                hr = pfCallback->SendPnpReconfig( NCRL_NDIS,
                                                 c_szMuxService,
                                                 lpszBindName,
                                                 (PVOID)lppnpEvent,
                                                 dwBytes );

                TraceMsg( L"   INetCfgPnpReconfigCallback->SendPnpReconfig returned "
                         L"%#x.\n",
                         hr );

                if ( hr != S_OK ) {

                  TraceMsg( L"   Failed to apply Pnp changes, miniport(%d).\n",
                            i );

                }

                free( lppnpEvent );
            }
            free( lpDevice );
        }
#endif        
    }

    dwMiniportCount = m_MiniportsToRemove.ListCount();

    TraceMsg( L"   Applying PnP changes to %d removed miniports.\n",
            dwMiniportCount );

    for (i=0; i < dwMiniportCount; ++i) {

        pMiniport = NULL;
        m_MiniportsToRemove.Remove( &pMiniport );

        pMiniport->GetMiniportGUID( &guidMiniport );

        //
        // Do miniport specific Pnp Changes when they are uninstalled.
        //

        hr = pMiniport->ApplyPnpChanges( pfCallback,
                                         eActRemove );
        if( hr != S_OK){
            // you may do something
        }

        delete pMiniport;

#ifdef CUSTOM_EVENTS

        //
        // Notify the driver that one or more virtual miniports have been
        // uninstalled.
        //
        // We can't notify the driver in case the adapter or the protocol is
        // being uninstalled because the binding handle doesn't exist.
        //

        if ( eApplyAction != eActRemove ) {

            int numChars = StringFromGUID2( guidMiniport,
                             szMiniportGuid,
                             MAX_PATH+1 );
            if(numChars == 0) {

                CoTaskMemFree(lpszBindName);

                return HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
            }

            lpDevice = AddDevicePrefix( szMiniportGuid );

            if ( lpDevice ) {

                dwBytes = sizeof(NOTIFY_CUSTOM_EVENT) +
                         ((wcslen(lpDevice) + 1) * sizeof(WCHAR));

                lppnpEvent = (PNOTIFY_CUSTOM_EVENT)malloc( dwBytes );

                if ( lppnpEvent ) {

                    lppnpEvent->uSignature = NOTIFY_SIGNATURE;
                    lppnpEvent->uEvent = MUX_CUSTOM_EVENT;
                    wcscpy( lppnpEvent->szMiniport,
                          lpDevice );

                    hr = pfCallback->SendPnpReconfig( NCRL_NDIS,
                                                    c_szMuxService,
                                                    lpszBindName,
                                                    (PVOID)lppnpEvent,
                                                    dwBytes );
                    TraceMsg( L"   INetCfgPnpReconfigCallback->SendPnpReconfig returned "
                            L"%#x.\n",
                            hr );

                    if ( hr != S_OK ) {

                        TraceMsg( L"   Failed to apply Pnp changes, miniport(%d).\n",
                               i );

                    }

                    free( lppnpEvent );
                }

                free( lpDevice );
            }
        }
#endif         

    }

#ifdef CUSTOM_EVENTS    
    CoTaskMemFree( lpszBindName );
#endif

    TraceMsg( L"<--CMuxPhysicalAdapter::ApplyPnpChanges(HRESULT = %x).\n",
            S_OK );

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// Function:  CMuxPhysicalAdapter::CancelChanges
//
// Purpose:   Cancel any changes made.
//
// Arguments: None
//                            
//
// Returns: S_OK.
//
// Notes:
//

HRESULT CMuxPhysicalAdapter::CancelChanges (VOID)
{
    TraceMsg( L"-->CMuxPhysicalAdapter::CancelChanges.\n" );

    TraceMsg( L"<--CMuxPhysicalAdapter::CancelChanges(HRESULT = %x).\n",
            S_OK );

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// Function:  CMuxPhysicalAdapter::AllMiniportsRemoved
//
// Purpose:   Find out if there is no miniport installed on the adapter.
//
// Arguments: None
//                            
//
// Returns: TRUE if all the miniports associated with this adapter have been
//          uninstalled and there is none pending to be added, otherwise FALSE.
//
// Notes:
//

BOOL  CMuxPhysicalAdapter::AllMiniportsRemoved (VOID)
{
  return (m_MiniportList.ListCount() + m_MiniportsToAdd.ListCount()) == 0;
}
