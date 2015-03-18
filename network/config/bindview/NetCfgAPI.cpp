//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       N E T C F G A P I . C P P
//
//  Contents:   Functions to illustrate INetCfg API
//
//  Notes:
//
//  Author:     Alok Sinha    15-May-01
//
//----------------------------------------------------------------------------

#include "NetCfgAPI.h"

//
// Function:  HrGetINetCfg
//
// Purpose:   Get a reference to INetCfg.
//
// Arguments:
//    fGetWriteLock  [in]  If TRUE, Write lock.requested.
//    lpszAppName    [in]  Application name requesting the reference.
//    ppnc           [out] Reference to INetCfg.
//    lpszLockedBy   [in]  Optional. Application who holds the write lock.
//
// Returns:   S_OK on success, otherwise an error code.
//
// Notes:
//

HRESULT HrGetINetCfg (IN BOOL fGetWriteLock,
                      IN LPCWSTR lpszAppName,
                      OUT INetCfg** ppnc,
                      _Outptr_opt_result_maybenull_ LPWSTR *lpszLockedBy)
{
    INetCfg      *pnc = NULL;
    INetCfgLock  *pncLock = NULL;
    HRESULT      hr = S_OK;

    //
    // Initialize the output parameters.
    //

    *ppnc = NULL;

    if ( lpszLockedBy )
    {
        *lpszLockedBy = NULL;
    }
    //
    // Initialize COM
    //

    hr = CoInitialize( NULL );

    if ( hr == S_OK ) {

        //
        // Create the object implementing INetCfg.
        //

        hr = CoCreateInstance( CLSID_CNetCfg,
                               NULL, CLSCTX_INPROC_SERVER,
                               IID_INetCfg,
                               (void**)&pnc );
        if ( hr == S_OK ) {

            if ( fGetWriteLock ) {

                //
                // Get the locking reference
                //

                hr = pnc->QueryInterface( IID_INetCfgLock,
                                          (LPVOID *)&pncLock );
                if ( hr == S_OK ) {

                    //
                    // Attempt to lock the INetCfg for read/write
                    //

                    hr = pncLock->AcquireWriteLock( LOCK_TIME_OUT,
                                                    lpszAppName,
                                                    lpszLockedBy);
                    if (hr == S_FALSE ) {
                        hr = NETCFG_E_NO_WRITE_LOCK;
                    }
                }
            }

            if ( hr == S_OK ) {

                //
                // Initialize the INetCfg object.
                //

                hr = pnc->Initialize( NULL );

                if ( hr == S_OK ) {
                    *ppnc = pnc;
                    pnc->AddRef();
                }
                else {

                    //
                    // Initialize failed, if obtained lock, release it
                    //

                    if ( pncLock ) {
                        pncLock->ReleaseWriteLock();
                    }
                }
            }

            ReleaseRef( pncLock );
            ReleaseRef( pnc );
        }

        //
        // In case of error, uninitialize COM.
        //

        if ( hr != S_OK ) {
            CoUninitialize();
        }
    }

    return hr;
}

//
// Function:  HrReleaseINetCfg
//
// Purpose:   Get a reference to INetCfg.
//
// Arguments:
//    pnc           [in] Reference to INetCfg to release.
//    fHasWriteLock [in] If TRUE, reference was held with write lock.
//
// Returns:   S_OK on success, otherwise an error code.
//
// Notes:
//

HRESULT HrReleaseINetCfg (IN INetCfg* pnc,
                          IN BOOL fHasWriteLock)
{
    INetCfgLock    *pncLock = NULL;
    HRESULT        hr = S_OK;

    //
    // Uninitialize INetCfg
    //

    hr = pnc->Uninitialize();

    //
    // If write lock is present, unlock it
    //

    if ( hr == S_OK && fHasWriteLock ) {

        //
        // Get the locking reference
        //

        hr = pnc->QueryInterface( IID_INetCfgLock,
                                  (LPVOID *)&pncLock);
        if ( hr == S_OK ) {
           hr = pncLock->ReleaseWriteLock();
           ReleaseRef( pncLock );
        }
    }

    ReleaseRef( pnc );

    //
    // Uninitialize COM.
    //

    CoUninitialize();

    return hr;
}

//
// Function:  HrInstallNetComponent
//
// Purpose:   Install a network component(protocols, clients and services)
//            given its INF file.
//
// Arguments:
//    pnc              [in] Reference to INetCfg.
//    lpszComponentId  [in] PnpID of the network component.
//    pguidClass       [in] Class GUID of the network component.
//    lpszInfFullPath  [in] INF file to install from.
//
// Returns:   S_OK on success, otherwise an error code.
//
// Notes:
//

HRESULT HrInstallNetComponent (IN INetCfg *pnc,
                               IN LPCWSTR lpszComponentId,
                               IN const GUID    *pguidClass,
                               IN LPCWSTR lpszInfFullPath)
{
    DWORD     dwError;
    HRESULT   hr = S_OK;
    WCHAR*     Drive = NULL;
    WCHAR*     Dir = NULL;
    WCHAR*     DirWithDrive = NULL;

    do
    {
        //
        // If full path to INF has been specified, the INF
        // needs to be copied using Setup API to ensure that any other files
        // that the primary INF copies will be correctly found by Setup API
        //

        if ( lpszInfFullPath ) {

            //
            // Allocate memory to hold the strings
            //
            Drive = (WCHAR*)CoTaskMemAlloc(_MAX_DRIVE * sizeof(WCHAR));
            if (NULL == Drive)
            {
                hr = E_OUTOFMEMORY;
                break;
            }
            ZeroMemory(Drive, _MAX_DRIVE * sizeof(WCHAR));

            Dir = (WCHAR*)CoTaskMemAlloc(_MAX_DIR * sizeof(WCHAR));
            if (NULL == Dir)
            {
                hr = E_OUTOFMEMORY;
                break;
            }
            ZeroMemory(Dir, _MAX_DRIVE * sizeof(WCHAR));

            DirWithDrive = (WCHAR*)CoTaskMemAlloc((_MAX_DRIVE + _MAX_DIR) * sizeof(WCHAR));
            if (NULL == DirWithDrive)
            {
                hr = E_OUTOFMEMORY;
                break;
            }               
            ZeroMemory(DirWithDrive, (_MAX_DRIVE + _MAX_DIR) * sizeof(WCHAR));

            //
            // Get the path where the INF file is.
            //

            _wsplitpath_s ( lpszInfFullPath,
                Drive,
                _MAX_DRIVE,
                Dir,
                _MAX_DIR,
                NULL,
                0,
                NULL,
                0);

            StringCchCopyW ( DirWithDrive,
                _MAX_DRIVE + _MAX_DIR,
                Drive );
            StringCchCatW ( DirWithDrive,
                _MAX_DRIVE + _MAX_DIR,
                Dir );

            //
            // Copy the INF file and other files referenced in the INF file.
            //

            if ( !SetupCopyOEMInfW(lpszInfFullPath,
                                   DirWithDrive,  // Other files are in the
                                                  // same dir. as primary INF
                                   SPOST_PATH,    // First param is path to INF
                                   0,             // Default copy style
                                   NULL,          // Name of the INF after
                                                  // it's copied to %windir%\inf
                                   0,             // Max buf. size for the above
                                   NULL,          // Required size if non-null
                                   NULL) ) {      // Optionally get the filename
                                                  // part of Inf name after it is copied.
                dwError = GetLastError();

                hr = HRESULT_FROM_WIN32( dwError );
            }
        }

        if ( S_OK == hr ) {

            //
            // Install the network component.
            //

            hr = HrInstallComponent( pnc,
                                     lpszComponentId,
                                     pguidClass );
            if ( hr == S_OK ) {

                //
                // On success, apply the changes
                //

                hr = pnc->Apply();
            }
        }

    #pragma warning(disable:4127) /* Conditional expression is constant */
    } while (false);

    if (Drive != NULL)
    {
        CoTaskMemFree(Drive);
        Drive = NULL;
    }
    if (Dir != NULL)
    {
        CoTaskMemFree(Dir);
        Dir = NULL;
    }
    if (DirWithDrive != NULL)
    {
        CoTaskMemFree(DirWithDrive);
        DirWithDrive = NULL;
    }    

    return hr;
}

//
// Function:  HrInstallComponent
//
// Purpose:   Install a network component(protocols, clients and services)
//            given its INF file.
// Arguments:
//    pnc              [in] Reference to INetCfg.
//    lpszComponentId  [in] PnpID of the network component.
//    pguidClass       [in] Class GUID of the network component.
//
// Returns:   S_OK on success, otherwise an error code.
//
// Notes:
//

HRESULT HrInstallComponent(IN INetCfg* pnc,
                           IN LPCWSTR szComponentId,
                           IN const GUID* pguidClass)
{
    INetCfgClassSetup   *pncClassSetup = NULL;
    INetCfgComponent    *pncc = NULL;
    OBO_TOKEN           OboToken;
    HRESULT             hr = S_OK;

    //
    // OBO_TOKEN specifies on whose behalf this
    // component is being installed.
    // Set it to OBO_USER so that szComponentId will be installed
    // on behalf of the user.
    //

    ZeroMemory( &OboToken,
                sizeof(OboToken) );
    OboToken.Type = OBO_USER;

    //
    // Get component's setup class reference.
    //

    hr = pnc->QueryNetCfgClass ( pguidClass,
                                 IID_INetCfgClassSetup,
                                 (void**)&pncClassSetup );
    if ( hr == S_OK ) {

        hr = pncClassSetup->Install( szComponentId,
                                     &OboToken,
                                     0,
                                     0,       // Upgrade from build number.
                                     NULL,    // Answerfile name
                                     NULL,    // Answerfile section name
                                     &pncc ); // Reference after the component
        if ( S_OK == hr ) {                   // is installed.

            //
            // we don't need to use pncc (INetCfgComponent), release it
            //

            ReleaseRef( pncc );
        }

        ReleaseRef( pncClassSetup );
    }

    return hr;
}

//
// Function:  HrUninstallNetComponent
//
// Purpose:   Uninstall a network component(protocols, clients and services).
//
// Arguments:
//    pnc           [in] Reference to INetCfg.
//    szComponentId [in] PnpID of the network component to uninstall.
//
// Returns:   S_OK on success, otherwise an error code.
//
// Notes:
//

HRESULT HrUninstallNetComponent(IN INetCfg* pnc,
                                IN LPCWSTR szComponentId)
{
    INetCfgComponent    *pncc = NULL;
    INetCfgClass        *pncClass = NULL;
    INetCfgClassSetup   *pncClassSetup = NULL;
    OBO_TOKEN           OboToken;
    GUID                guidClass;
    HRESULT             hr = S_OK;

    //
    // OBO_TOKEN specifies on whose behalf this
    // component is being installed.
    // Set it to OBO_USER so that szComponentId will be installed
    // on behalf of the user.
    //

    ZeroMemory( &OboToken,
                sizeof(OboToken) );
    OboToken.Type = OBO_USER;

    //
    // Get the component's reference.
    //

    hr = pnc->FindComponent( szComponentId,
                             &pncc );

    if (S_OK == hr) {

        //
        // Get the component's class GUID.
        //

        hr = pncc->GetClassGuid( &guidClass );

        if ( hr == S_OK ) {

            //
            // Get component's class reference.
            //

            hr = pnc->QueryNetCfgClass( &guidClass,
                                        IID_INetCfgClass,
                                        (void**)&pncClass );
            if ( hr == S_OK ) {

                //
                // Get Setup reference.
                //

                hr = pncClass->QueryInterface( IID_INetCfgClassSetup,
                                               (void**)&pncClassSetup );
                    if ( hr == S_OK ) {

                         hr = pncClassSetup->DeInstall( pncc,
                                                        &OboToken,
                                                        NULL);
                         if ( hr == S_OK ) {

                             //
                             // Apply the changes
                             //

                             hr = pnc->Apply();
                         }

                         ReleaseRef( pncClassSetup );
                    }

                ReleaseRef( pncClass );
            }
        }

        ReleaseRef( pncc );
    }

    return hr;
}

//
// Function:  HrGetComponentEnum
//
// Purpose:   Get network component enumerator reference.
//
// Arguments:
//    pnc         [in]  Reference to INetCfg.
//    pguidClass  [in]  Class GUID of the network component.
//    ppencc      [out] Enumerator reference.
//
// Returns:   S_OK on success, otherwise an error code.
//
// Notes:
//

HRESULT HrGetComponentEnum (INetCfg* pnc,
                            IN const GUID* pguidClass,
                            OUT IEnumNetCfgComponent **ppencc)
{
    INetCfgClass  *pncclass;
    HRESULT       hr;

    *ppencc = NULL;

    //
    // Get the class reference.
    //

    hr = pnc->QueryNetCfgClass( pguidClass,
                                IID_INetCfgClass,
                                (PVOID *)&pncclass );

    if ( hr == S_OK ) {

        //
        // Get the enumerator reference.
        //

        hr = pncclass->EnumComponents( ppencc );

        //
        // We don't need the class reference any more.
        //

        ReleaseRef( pncclass );
    }

    return hr;
}

//
// Function:  HrGetFirstComponent
//
// Purpose:   Enumerates the first network component.
//
// Arguments:
//    pencc      [in]  Component enumerator reference.
//    ppncc      [out] Network component reference.
//
// Returns:   S_OK on success, otherwise an error code.
//
// Notes:
//

HRESULT HrGetFirstComponent (IN IEnumNetCfgComponent* pencc,
                             OUT INetCfgComponent **ppncc)
{
    HRESULT  hr;
    ULONG    ulCount;

    *ppncc = NULL;

    pencc->Reset();

    hr = pencc->Next( 1,
                      ppncc,
                      &ulCount );
    return hr;
}

//
// Function:  HrGetNextComponent
//
// Purpose:   Enumerate the next network component.
//
// Arguments:
//    pencc      [in]  Component enumerator reference.
//    ppncc      [out] Network component reference.
//
// Returns:   S_OK on success, otherwise an error code.
//
// Notes:     The function behaves just like HrGetFirstComponent if
//            it is called right after HrGetComponentEnum.
//
//

HRESULT HrGetNextComponent (IN IEnumNetCfgComponent* pencc,
                            OUT INetCfgComponent **ppncc)
{
    HRESULT  hr;
    ULONG    ulCount;

    *ppncc = NULL;

    hr = pencc->Next( 1,
                      ppncc,
                      &ulCount );
    return hr;
}

//
// Function:  HrFindNetComponentByPnpId
//
// Purpose:   Get network adapter identified by a particular pnp device instance id.
//
// Arguments:
//    pncc              [in]  Network component reference.
//    lpszPnpDevNodeId  [in]  pnp device instance id.
//    ppncc             [out] pointer to network adapter reference.
//
// Returns:   S_OK on success, otherwise an error code.
//
// Notes:
//

HRESULT HrFindNetComponentByPnpId (IN INetCfg *pnc,
                                   IN _In_ LPWSTR lpszPnpDevNodeId,
                                   OUT INetCfgComponent **ppncc)
{
    IEnumNetCfgComponent    *pencc;
    LPWSTR                  pszPnpId;
    HRESULT                 hr;
    BOOL                    fFound;

    hr = HrGetComponentEnum( pnc,
                             &GUID_DEVCLASS_NET,
                             &pencc );
    if ( hr == S_OK ) {

        hr = HrGetFirstComponent( pencc, ppncc );
        fFound = FALSE;
        while( hr == S_OK ) {
            hr = (*ppncc)->GetPnpDevNodeId( &pszPnpId );
            if ( hr == S_OK ) {
                fFound = wcscmp( pszPnpId, lpszPnpDevNodeId ) == 0;
                CoTaskMemFree( pszPnpId );
                if ( fFound ) {
                    break;
                }
            }
            else {
                hr = S_OK;
            }

            ReleaseRef( *ppncc );
            hr = HrGetNextComponent( pencc, ppncc );
        }

        ReleaseRef( pencc );
    }

    return hr;
}

//
// Function:  HrGetBindingPathEnum
//
// Purpose:   Get network component's binding path enumerator reference.
//
// Arguments:
//    pncc           [in]  Network component reference.
//    dwBindingType  [in]  EBP_ABOVE or EBP_BELOW.
//    ppencbp        [out] Enumerator reference.
//
// Returns:   S_OK on success, otherwise an error code.
//
// Notes:
//

HRESULT HrGetBindingPathEnum (IN INetCfgComponent *pncc,
                              IN DWORD dwBindingType,
                              OUT IEnumNetCfgBindingPath **ppencbp)
{
    INetCfgComponentBindings *pnccb = NULL;
    HRESULT                  hr;

    *ppencbp = NULL;

    //
    // Get component's binding.
    //

    hr = pncc->QueryInterface( IID_INetCfgComponentBindings,
                               (PVOID *)&pnccb );

    if ( hr == S_OK ) {

        //
        // Get binding path enumerator reference.
        //

        hr = pnccb->EnumBindingPaths( dwBindingType,
                                      ppencbp );

        ReleaseRef( pnccb );
    }

    return hr;
}

//
// Function:  HrGetFirstBindingPath
//
// Purpose:   Enumerates the first binding path.
//
// Arguments:
//    pencc      [in]  Binding path enumerator reference.
//    ppncc      [out] Binding path reference.
//
// Returns:   S_OK on success, otherwise an error code.
//
// Notes:
//

HRESULT HrGetFirstBindingPath (IN IEnumNetCfgBindingPath *pencbp,
                               OUT INetCfgBindingPath **ppncbp)
{
    ULONG   ulCount;
    HRESULT hr;

    *ppncbp = NULL;

    pencbp->Reset();

    hr = pencbp->Next( 1,
                       ppncbp,
                       &ulCount );

    return hr;
}

//
// Function:  HrGetNextBindingPath
//
// Purpose:   Enumerate the next binding path.
//
// Arguments:
//    pencbp      [in]  Binding path enumerator reference.
//    ppncbp      [out] Binding path reference.
//
// Returns:   S_OK on success, otherwise an error code.
//
// Notes:     The function behaves just like HrGetFirstBindingPath if
//            it is called right after HrGetBindingPathEnum.
//
//

HRESULT HrGetNextBindingPath (IN IEnumNetCfgBindingPath *pencbp,
                              OUT INetCfgBindingPath **ppncbp)
{
    ULONG   ulCount;
    HRESULT hr;

    *ppncbp = NULL;

    hr = pencbp->Next( 1,
                       ppncbp,
                       &ulCount );

    return hr;
}

//
// Function:  HrGetBindingInterfaceEnum
//
// Purpose:   Get binding interface enumerator reference.
//
// Arguments:
//    pncbp          [in]  Binding path reference.
//    ppencbp        [out] Enumerator reference.
//
// Returns:   S_OK on success, otherwise an error code.
//
// Notes:
//

HRESULT HrGetBindingInterfaceEnum (IN INetCfgBindingPath *pncbp,
                                   OUT IEnumNetCfgBindingInterface **ppencbi)
{
    HRESULT hr;

    *ppencbi = NULL;

    hr = pncbp->EnumBindingInterfaces( ppencbi );

    return hr;
}

//
// Function:  HrGetFirstBindingInterface
//
// Purpose:   Enumerates the first binding interface.
//
// Arguments:
//    pencbi      [in]  Binding interface enumerator reference.
//    ppncbi      [out] Binding interface reference.
//
// Returns:   S_OK on success, otherwise an error code.
//
// Notes:
//

HRESULT HrGetFirstBindingInterface (IN IEnumNetCfgBindingInterface *pencbi,
                                    OUT INetCfgBindingInterface **ppncbi)
{
    ULONG   ulCount;
    HRESULT hr;

    *ppncbi = NULL;

    pencbi->Reset();

    hr = pencbi->Next( 1,
                       ppncbi,
                       &ulCount );

    return hr;
}

//
// Function:  HrGetNextBindingInterface
//
// Purpose:   Enumerate the next binding interface.
//
// Arguments:
//    pencbi      [in]  Binding interface enumerator reference.
//    ppncbi      [out] Binding interface reference.
//
// Returns:   S_OK on success, otherwise an error code.
//
// Notes:     The function behaves just like HrGetFirstBindingInterface if
//            it is called right after HrGetBindingInterfaceEnum.
//
//

HRESULT HrGetNextBindingInterface (IN IEnumNetCfgBindingInterface *pencbi,
                                   OUT INetCfgBindingInterface **ppncbi)
{
    ULONG   ulCount;
    HRESULT hr;

    *ppncbi = NULL;

    hr = pencbi->Next( 1,
                       ppncbi,
                       &ulCount );

    return hr;
}

//
// Function:  ReleaseRef
//
// Purpose:   Release reference.
//
// Arguments:
//    punk     [in]  IUnknown reference to release.
//
// Returns:   Reference count.
//
// Notes:
//

VOID ReleaseRef (IN IUnknown* punk)
{
    if ( punk ) {
        punk->Release();
    }

    return;
}

