//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992-2001.
//
//  File:       V I R T U A L . C P P
//
//  Contents:   Virtual miniport class definition.
//
//  Notes:
//
//  Author:     Alok Sinha
//----------------------------------------------------------------------------

#include "virtual.h"
#include "common.h"

//+---------------------------------------------------------------------------
//
// Function:  CMuxVirtualMiniport::CMuxVirtualMiniport
//
// Purpose:   Constructor for class CMuxVirtualMiniport
//
// Arguments: None
//
// Returns:
//
// Notes:
//

CMuxVirtualMiniport::CMuxVirtualMiniport(INetCfg *pnc,
                                         GUID    *pguidMiniport,
                                         GUID    *pguidAdapter)
{
    TraceMsg( L"-->CMuxVirtualMiniport::CMuxVirtualMiniport(Constructor).\n" );

    m_pnc = pnc;
    m_pnc->AddRef();

    CopyMemory( &m_guidAdapter,
                pguidAdapter,
                sizeof(GUID) );

    if ( pguidMiniport ) {

        CopyMemory( &m_guidMiniport,
                    pguidMiniport,
                    sizeof(GUID) );

    }
    else {

        ZeroMemory( &m_guidMiniport,
                    sizeof(GUID) );
    }

    TraceMsg( L"<--CMuxVirtualMiniport::CMuxVirtualMiniport(Constructor).\n" );
}

//+---------------------------------------------------------------------------
//
// Function:  CMuxVirtualMiniport::~CMuxVirtualMiniport
//
// Purpose:   Destructor for class CMuxVirtualMiniport
//
// Arguments: None
//
// Returns:
//
// Notes:
//

CMuxVirtualMiniport::~CMuxVirtualMiniport(VOID)
{
    TraceMsg( L"-->CMuxVirtualMiniport::~CMuxVirtualMiniport(Destructor).\n" );

    ReleaseObj( m_pnc );

    TraceMsg( L"<--CMuxVirtualMiniport::~CMuxVirtualMiniport(Destructor).\n" );

}

//+---------------------------------------------------------------------------
//
// Function:  CMuxVirtualMiniport::LoadConfiguration
//
// Purpose:   Load miniport configuration from the registry.
//
// Arguments: None
//
// Returns: S_OK on success, otherwise an error code.
//
// Notes:
//

HRESULT CMuxVirtualMiniport::LoadConfiguration(VOID)
{
    TraceMsg( L"-->CMuxVirtualMiniport::LoadConfiguration.\n" );

    TraceMsg( L"<--CMuxVirtualMiniport::LoadConfiguration(HRESULT = %x).\n",
            S_OK );

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// Function:  CMuxVirtualMiniport::GetAdapterGUID
//
// Purpose:   Returns the adapter GUID.
//
// Arguments:
//          OUT pguidAdapter: GUID of the adapter returned.
//
// Returns: None.
//
// Notes:
//

VOID CMuxVirtualMiniport::GetAdapterGUID (GUID *pguidAdapter)
{
    TraceMsg( L"-->CMuxVirtualMiniport::GetAdapterGUID.\n" );

    CopyMemory( pguidAdapter,
                &m_guidAdapter,
                sizeof(GUID) );

    TraceMsg( L"<--CMuxVirtualMiniport::GetAdapterGUID.\n" );
}

//+---------------------------------------------------------------------------
//
// Function:  CMuxVirtualMiniport::GetMiniportGUID
//
// Purpose:   Returns the miniport GUID.
//
// Arguments:
//          OUT pguidMiniport: GUID of the miniport returned.
//
// Returns: None.
//
// Notes:
//

VOID CMuxVirtualMiniport::GetMiniportGUID (GUID *pguidMiniport)
{
    TraceMsg( L"-->CMuxVirtualMiniport::GetMiniportGUID.\n" );

    CopyMemory( pguidMiniport,
              &m_guidMiniport,
              sizeof(GUID) );

    TraceMsg( L"<--CMuxVirtualMiniport::GetMiniportGUID.\n" );
}

//+---------------------------------------------------------------------------
//
// Function:  CMuxVirtualMiniport::Install
//
// Purpose:   Installs a virtual miniport.
//
// Arguments: None
//
// Returns: S_OK on success, otherwise an error code.
//
// Notes:
//

HRESULT CMuxVirtualMiniport::Install (VOID)
{
    INetCfgClass       *pncClass;
    INetCfgClassSetup  *pncClassSetup;
    INetCfgComponent   *pnccMiniport;
    HRESULT            hr;
    LPWSTR  *pmszwRefs=NULL;
    OBO_TOKEN  *pOboToken=NULL;    
    DWORD  dwSetupFlags=0;
    LPCWSTR  pszwAnswerFile=NULL;
    LPCWSTR  pszwAnswerSections=NULL; 


    TraceMsg( L"-->CMuxVirtualMiniport::Install.\n" );

    hr = m_pnc->QueryNetCfgClass( &GUID_DEVCLASS_NET,
                                  IID_INetCfgClass,
                                  (void **)&pncClass );
    if ( hr == S_OK ) {

        hr = pncClass->QueryInterface( IID_INetCfgClassSetup,
                                       (void **)&pncClassSetup );
        if ( hr == S_OK ) {
            
            hr = pncClassSetup->Install( c_szMuxMiniport,
                                         pOboToken,
                                         dwSetupFlags,
                                         0,
                                         pszwAnswerFile,
                                         pszwAnswerSections,
                                         &pnccMiniport );
            if ( hr == S_OK ) {

                hr = pnccMiniport->GetInstanceGuid( &m_guidMiniport );

                if ( hr != S_OK ) {

                    TraceMsg( L"   Failed to get the instance guid, uninstalling "
                              L" the miniport.\n" );
                    
                    pncClassSetup->DeInstall( pnccMiniport,
                                              pOboToken,
                                              pmszwRefs );
                }

                ReleaseObj( pnccMiniport );
            }
            else {

                TraceMsg( L"   Failed to install the miniport.\n" );
            }

            ReleaseObj( pncClassSetup );
        }
        else {

            TraceMsg( L"   QueryInterface failed.\n" );
        }

        ReleaseObj( pncClass );
    }
    else {

     TraceMsg( L"   QueryNetCfgClass failed.\n" );
    }

    TraceMsg( L"<--CMuxVirtualMiniport::Install(HRESULT = %x).\n",
            hr );

    return hr;
}

//+---------------------------------------------------------------------------
//
// Function:  CMuxVirtualMiniport::DeInstall
//
// Purpose:   Uninstalls the virtual miniport.
//
// Arguments: None
//
// Returns: S_OK on success, otherwise an error code.
//
// Notes:
//

HRESULT CMuxVirtualMiniport::DeInstall (VOID)
{
    INetCfgClass       *pncClass;
    INetCfgClassSetup  *pncClassSetup;
    INetCfgComponent   *pnccMiniport;
    HRESULT            hr;
    LPWSTR  *pmszwRefs=NULL;
    OBO_TOKEN  *pOboToken=NULL;

    TraceMsg( L"-->CMuxVirtualMiniport::DeInstall.\n" );

    hr = m_pnc->QueryNetCfgClass( &GUID_DEVCLASS_NET,
                                  IID_INetCfgClass,
                                  (void **)&pncClass );
    if ( hr == S_OK ) {

        hr = pncClass->QueryInterface( IID_INetCfgClassSetup,
                                       (void **)&pncClassSetup );
        if ( hr == S_OK ) {

            hr = HrFindInstance( m_pnc,
                                 m_guidMiniport,
                                 &pnccMiniport );

            if ( hr == S_OK ) {

                TraceMsg( L"   Found the miniport instance to uninstall.\n" );
                
                hr = pncClassSetup->DeInstall( pnccMiniport,
                                               pOboToken,
                                               pmszwRefs );
                ReleaseObj( pnccMiniport );
            }
            else {
                TraceMsg( L"   Didn't find the miniport instance to uninstall.\n" );
            }

            ReleaseObj( pncClassSetup );
        }
        else {

            TraceMsg( L"   QueryInterface failed.\n" );
        }

        ReleaseObj( pncClass );
    }
    else {

        TraceMsg( L"   QueryNetCfgClass failed.\n" );
    }

    TraceMsg( L"<--CMuxVirtualMiniport::DeInstall(HRESULT = %x).\n",
              hr );

    return hr;
}

//+---------------------------------------------------------------------------
//
// Function:  CMuxVirtualMiniport::ApplyRegistryChanges
//
// Purpose:   Store the changes in the registry.
//
// Arguments:
//            IN eApplyAction: Action performed.
//
// Returns: S_OK on success, otherwise an error code.
//
// Notes:
//

HRESULT CMuxVirtualMiniport::ApplyRegistryChanges(ConfigAction eApplyAction)
{
    HKEY                    hkeyAdapterGuid;
    WCHAR                   szAdapterGuid[MAX_PATH+1];
    WCHAR                   szAdapterGuidKey[MAX_PATH+1];
    WCHAR                   szMiniportGuid[MAX_PATH+1];
    LPWSTR                  lpDevice;
    LONG                    lResult = 0;
    int                     numChars;
    TraceMsg( L"-->CMuxVirtualMiniport::ApplyRegistryChanges.\n" );

    switch( eApplyAction ) {

        case eActAdd:         // Virtual miniport added.

            numChars = StringFromGUID2( m_guidAdapter,
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
                                       NULL);


            if ( lResult == ERROR_SUCCESS ) {

                numChars = StringFromGUID2( m_guidMiniport,
                                 szMiniportGuid,
                                 MAX_PATH+1 );

                if (numChars == 0) {

                    RegCloseKey(hkeyAdapterGuid);

                    return HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
                }

                lpDevice = AddDevicePrefix( szMiniportGuid );

                if ( lpDevice ) {

#ifndef PASSTHRU_NOTIFY

                    lResult = AddToMultiSzValue( hkeyAdapterGuid,
                                                 lpDevice );
#else

                    lResult = RegSetValueExW( hkeyAdapterGuid,
                                              c_szUpperBindings,
                                              0,
                                              REG_SZ,
                                              (LPBYTE)lpDevice,
                                              (wcslen(lpDevice) + 1) *
                                              sizeof(WCHAR) );


#endif

                    if ( lResult != ERROR_SUCCESS ) {

                        TraceMsg( L"   Failed to save %s at %s\\%s.\n",
                                  lpDevice,
                                  szAdapterGuidKey,
                                  c_szUpperBindings );

                    }

                    free( lpDevice );
                }
                else {
                    lResult = ERROR_NOT_ENOUGH_MEMORY;
                }

                RegCloseKey( hkeyAdapterGuid );
            }
            else {
                TraceMsg( L"   Failed to open the registry key: %s.\n",
                          szAdapterGuidKey );
            }
            break;

        case eActRemove:                  // Virtual miniport removed.

            numChars = StringFromGUID2( m_guidAdapter,
                             szAdapterGuid,
                             MAX_PATH+1 );

            if (numChars == 0) {

                return HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
            }

            StringCchPrintfW( szAdapterGuidKey,
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
                                        NULL);


            if ( lResult == ERROR_SUCCESS ) {

                numChars = StringFromGUID2( m_guidMiniport,
                                 szMiniportGuid,
                                 MAX_PATH+1 );

                if (numChars == 0) {

                    RegCloseKey(hkeyAdapterGuid);

                    return HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
                }

                lpDevice = AddDevicePrefix( szMiniportGuid );
                TraceMsg( L"   Deleting %s at %s.\n",
                          lpDevice,
                          szAdapterGuidKey );

                if ( lpDevice ) {

#ifndef PASSTHRU_NOTIFY

                    lResult = DeleteFromMultiSzValue( hkeyAdapterGuid,
                                                      lpDevice );
#else

                    lResult = RegDeleteValueW( hkeyAdapterGuid,
                                               c_szUpperBindings );
#endif

                    if ( lResult != ERROR_SUCCESS ) {

                        TraceMsg( L"   Failed to delete %s at %s\\%s.\n",
                                  lpDevice,
                                  szAdapterGuidKey,
                                  c_szUpperBindings );

                    }

                    free( lpDevice );
                }

                RegCloseKey( hkeyAdapterGuid );
            }
            else {
                TraceMsg( L"   Failed to open the registry key: %s.\n",
                          szAdapterGuidKey );
            }
    }

    TraceMsg( L"<--CMuxVirtualMiniport::ApplyRegistryChanges(HRESULT = %x).\n",
              HRESULT_FROM_WIN32(lResult) );

    return HRESULT_FROM_WIN32(lResult);
}

//+---------------------------------------------------------------------------
//
// Function:  CMuxVirtualMiniport::ApplyPnpChanges
//
// Purpose:   
//
// Arguments:
//            IN eApplyAction: Action performed.
//
// Returns: S_OK on success, otherwise an error code.
//
// Notes:
//

HRESULT CMuxVirtualMiniport::ApplyPnpChanges
                                 (INetCfgPnpReconfigCallback *pfCallback,
                                  ConfigAction eApplyAction)
{
    UNREFERENCED_PARAMETER(pfCallback);
    UNREFERENCED_PARAMETER(eApplyAction);

    TraceMsg( L"-->CMuxVirtualMiniport::ApplyPnpChanges.\n" );

    TraceMsg( L"<--CMuxVirtualMiniport::ApplyPnpChanges(HRESULT = %x).\n",
            S_OK );

    return S_OK;
}

