//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992-2001.
//
//  File:       N O T I F Y . C P P
//
//  Contents:   Sample notify object code
//  
//  Notes:
//
//  Author:     Alok Sinha

//----------------------------------------------------------------------------

#include "notify.h"

//----------------------------------------------------------------------------
//
// Function:  CMuxNotify::CMuxNotify
//
// Purpose:   Constructor for CMuxNotify
//
// Arguments: None
//
// Returns:   None
//
// Notes:
//

CMuxNotify::CMuxNotify (VOID) : m_pncc (NULL),
                                m_pnc(NULL),
                                m_eApplyAction(eActUnknown),
                                m_pUnkContext(NULL)
{
    TraceMsg( L"-->CMuxNotify::CMuxNotify(Constructor).\n" );

    TraceMsg( L"<--CMuxNotify::CMuxNotify(Constructor).\n" );
}


// ----------------------------------------------------------------------
//
// Function:  CMuxNotify::~CMuxNotify
//
// Purpose:   Destructor for class CMuxNotify
//
// Arguments: None
//
// Returns:   None
//
// Notes:
//
CMuxNotify::~CMuxNotify (VOID)
{
    CMuxPhysicalAdapter *pAdapter;
    DWORD dwAdapterCount;
    DWORD i;

    TraceMsg( L"-->CMuxNotify::~CMuxNotify(Destructor).\n" );

    // release interfaces if acquired

    ReleaseObj( m_pncc );
    ReleaseObj( m_pnc );
    ReleaseObj( m_pUnkContext );

    dwAdapterCount = m_AdaptersList.ListCount();

    for (i=0; i < dwAdapterCount; ++i) {

        pAdapter = NULL;
        m_AdaptersList.Remove( &pAdapter );

        delete pAdapter;
    }

    dwAdapterCount = m_AdaptersToRemove.ListCount();

    for (i=0; i < dwAdapterCount; ++i) {

        pAdapter = NULL;
        m_AdaptersToRemove.Remove( &pAdapter );

        delete pAdapter;
    }

    dwAdapterCount = m_AdaptersToAdd.ListCount();

    for (i=0; i < dwAdapterCount; ++i) {

        pAdapter = NULL;
        m_AdaptersToAdd.Remove( &pAdapter );

        delete pAdapter;
    }

    TraceMsg( L"<--CMuxNotify::~CMuxNotify(Destructor).\n" );
}

//
//---------------------- NOTIFY OBJECT FUNCTIONS -----------------------------
//

//----------------------------------------------------------------------------
// INetCfgComponentControl                                           
//                                                                       
// The following functions provide the INetCfgComponentControl interface.
//                                                                       
//----------------------------------------------------------------------------

//
// Function:  CMuxNotify::Initialize
//
// Purpose:   Initialize the notify object
//
// Arguments:
//           IN pnccItem   :  Pointer to INetCfgComponent object
//           IN pnc        :  Pointer to INetCfg object
//           IN fInstalling:  TRUE if we are being installed
//
// Returns:
//
// Notes:
//

STDMETHODIMP CMuxNotify::Initialize (INetCfgComponent* pncc,
                                     INetCfg* pnc, 
                                     BOOL fInstalling)
{
    HRESULT hr = S_OK;

    TraceMsg( L"-->CMuxNotify INetCfgControl::Initialize.\n" );


    // Save INetCfg & INetCfgComponent and add a refcount

    m_pncc = pncc;
    m_pnc = pnc;

    if (m_pncc) {

        m_pncc->AddRef();
    }

    if (m_pnc) {

        m_pnc->AddRef();
    }


    //
    // If this not an installation, then we need to 
    // initialize all of our data and classes
    //

    if ( !fInstalling ) {

        hr = HrLoadAdapterConfiguration();
    }

    TraceMsg( L"<--CMuxNotify INetCfgControl::Initialize(HRESULT = %x).\n",
           hr );

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  CMuxNotify::CancelChanges
//
// Purpose:   Cancel any changes made to internal data
//
// Arguments: None
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//

STDMETHODIMP CMuxNotify::CancelChanges (VOID)
{
    TraceMsg( L"-->CMuxNotify INetCfgControl::CancelChanges.\n" );


    TraceMsg( L"<--CMuxNotify INetCfgControl::CancelChanges(HRESULT = %x).\n",
              S_OK );

    return S_OK;
}

// ----------------------------------------------------------------------
//
// Function:  CMuxNotify::ApplyRegistryChanges
//
// Purpose:   Apply changes.
//
// Arguments: None
//
// Returns:   S_OK.
//
// Notes:     We can make changes to registry etc. here.

STDMETHODIMP CMuxNotify::ApplyRegistryChanges(VOID)
{
    CMuxPhysicalAdapter *pAdapter = NULL;
    DWORD                  dwAdapterCount;
    DWORD                  i;

    TraceMsg( L"-->CMuxNotify INetCfgControl::ApplyRegistryChanges.\n" );

    //
    // Make registry changes for the adapters added.
    //

    dwAdapterCount = m_AdaptersToAdd.ListCount();

    TraceMsg( L"   Adding %d new adapters.\n",
              dwAdapterCount );

    for (i=0; i < dwAdapterCount; ++i) {

        m_AdaptersToAdd.Find( i,
                              &pAdapter );
   
        pAdapter->ApplyRegistryChanges( eActAdd );

    }

    //
    // Make registry changes for the adapters uninstalled.
    //

    dwAdapterCount = m_AdaptersToRemove.ListCount();

    TraceMsg( L"   Removing %d adapters.\n",
              dwAdapterCount );

    for (i=0; i < dwAdapterCount; ++i) {

        m_AdaptersToRemove.Find( i,
                                 &pAdapter );
           
        pAdapter->ApplyRegistryChanges( eActRemove );
    }

    //
    // Make registry changes for the miniports added/removed
    // through the property pages.
    //

    dwAdapterCount = m_AdaptersList.ListCount();

    for (i=0; i < dwAdapterCount; ++i) {

        m_AdaptersList.Find( i,
                             &pAdapter );

        pAdapter->ApplyRegistryChanges( eActUpdate );
    }

    TraceMsg( L"<--CMuxNotify INetCfgControl::ApplyRegistryChanges(HRESULT = %x).\n",
              S_OK );

    return S_OK;
}

// ----------------------------------------------------------------------
//
// Function:  CMuxNotify::ApplyPnpChanges
//
// Purpose:   Apply changes.
//
// Arguments:
//            IN pfCallback: PnPConfigCallback interface.
//
// Returns:   S_OK.
//
// Notes:     

STDMETHODIMP CMuxNotify::ApplyPnpChanges (
                                       INetCfgPnpReconfigCallback* pfCallback)
{
    CMuxPhysicalAdapter *pAdapter = NULL;
    GUID                   guidAdapter;
    DWORD                  dwAdapterCount;
    DWORD                  i;

    TraceMsg( L"-->CMuxNotify INetCfgControl::ApplyPnpChanges.\n" );

    //
    // Apply PnP changes for the adapters added.
    //

    dwAdapterCount = m_AdaptersToAdd.ListCount();

    TraceMsg( L"   Applying PnP changes when %d adapters added.\n",
            dwAdapterCount );

    for (i=0; i < dwAdapterCount; ++i) {

        m_AdaptersToAdd.Remove( &pAdapter );

        pAdapter->ApplyPnpChanges( pfCallback,
                                eActAdd );

        pAdapter->GetAdapterGUID( &guidAdapter );

        m_AdaptersList.Insert( pAdapter,
                            guidAdapter );
    }

    //
    // Apply PnP changes for the adapters uninstalled.
    //

    dwAdapterCount = m_AdaptersToRemove.ListCount();

    TraceMsg( L"   Applying PnP changes when %d adapters removed.\n",
            dwAdapterCount );

    for (i=0; i < dwAdapterCount; ++i) {

        m_AdaptersToRemove.Remove( &pAdapter );

        pAdapter->ApplyPnpChanges( pfCallback,
                            eActRemove );

        delete pAdapter;
    }

    //
    // Apply PnP changes for the miniports added/removed through
    // the property pages.
    //

    dwAdapterCount = m_AdaptersList.ListCount();

    for (i=0; i < dwAdapterCount; ++i) {

        m_AdaptersList.Find( i,
                          &pAdapter );

        pAdapter->ApplyPnpChanges( pfCallback,
                                eActUpdate );
    }

    TraceMsg( L"<--CMuxNotify INetCfgControl::ApplyPnpChanges(HRESULT = %x).\n",
            S_OK );

    return S_OK;
}


//----------------------------------------------------------------------------
// INetCfgComponentSetup                                           
//                                                                       
// The following functions provide the INetCfgComponentSetup interface.
//                                                                       
//----------------------------------------------------------------------------

// ----------------------------------------------------------------------
//
// Function:  CMuxNotify::Install
//
// Purpose:   Do operations necessary during the installation.
//
// Arguments:
//            IN dwSetupFlags:  Setup flags
//
// Returns:   S_OK
//
// Notes:     Don't do anything irreversible (like modifying registry) yet
//            since the config. actually completes only when Apply is called!
//

STDMETHODIMP CMuxNotify::Install (DWORD dwSetupFlags)
{

    UNREFERENCED_PARAMETER(dwSetupFlags);
    TraceMsg( L"-->CMuxNotify INetCfgSetup::Install.\n" );

    // Start up the install process

    m_eApplyAction = eActInstall;

    TraceMsg( L"<--CMuxNotify INetCfgSetup::Install(HRESULT = %x).\n",
            S_OK );

    return S_OK;
}

// ----------------------------------------------------------------------
//
// Function:  CMuxNotify::Upgrade
//
// Purpose:   Do operations necessary during the upgrade.
//
// Arguments:
//            IN dwSetupFlags: Setup flags
//
// Returns:   S_OK
//
// Notes:     Don't do anything irreversible (like modifying registry) yet
//            since the config. actually completes only when Apply is called!
//

STDMETHODIMP CMuxNotify::Upgrade (IN DWORD dwSetupFlags,
                                  IN DWORD dwUpgradeFromBuildNo)
{

    TraceMsg( L"-->CMuxNotify INetCfgSetup::Upgrade.\n" );

    TraceMsg( L"   DwSetupFlags = %x, dwUpgradeFromBuildNo = %x\n",
              dwSetupFlags,
              dwUpgradeFromBuildNo );

    TraceMsg( L"<--CMuxNotify INetCfgSetup::Upgrade(HRESULT = %x).\n",
              S_OK );

    return S_OK;
}


// ----------------------------------------------------------------------
//
// Function:  CMuxNotify::ReadAnswerFile
//
// Purpose:   Read settings from answerfile and configure CMuxNotify
//
// Arguments:
//            IN pszAnswerFile   : Name of AnswerFile
//            IN pszAnswerSection: Name of parameters section
//
// Returns:
//
// Notes:     Don't do anything irreversible (like modifying registry) yet
//            since the config. actually completes only when Apply is called!
//

STDMETHODIMP CMuxNotify::ReadAnswerFile (PCWSTR pszAnswerFile,
                                         PCWSTR pszAnswerSection)
{
//    PCWSTR pszParamReadFromAnswerFile = L"ParamFromAnswerFile";

    UNREFERENCED_PARAMETER(pszAnswerFile);
    UNREFERENCED_PARAMETER(pszAnswerSection);
    
    TraceMsg( L"-->CMuxNotify INetCfgSetup::ReadAnswerFile.\n" );

    // We will pretend here that szParamReadFromAnswerFile was actually
    // read from the AnswerFile using the following steps
    //
    //   - Open file pszAnswerFile using SetupAPI
    //   - locate section pszAnswerSection
    //   - locate the required key and get its value
    //   - store its value in pszParamReadFromAnswerFile
    //   - close HINF for pszAnswerFile

    // Now that we have read pszParamReadFromAnswerFile from the
    // AnswerFile, store it in our memory structure.
    // Remember we should not be writing it to the registry till
    // our Apply is called!!
    //

    TraceMsg( L"<--CMuxNotify INetCfgSetup::ReadAnswerFile(HRESULT = %x).\n",
              S_OK );

    return S_OK;
}


// ----------------------------------------------------------------------
//
// Function:  CMuxNotify::Removing
//
// Purpose:   Do necessary cleanup when being removed
//
// Arguments: None
//
// Returns:   S_OK
//
// Notes:     Don't do anything irreversible (like modifying registry) yet
//            since the removal is actually complete only when Apply is called!
//

STDMETHODIMP CMuxNotify::Removing (VOID)
{

    TraceMsg( L"-->CMuxNotify INetCfgSetup::Removing.\n" );

    TraceMsg( L"<--CMuxNotify INetCfgSetup::Removing(HRESULT = %x).\n",
            S_OK );

    return S_OK;
}



//----------------------------------------------------------------------------
// INetCfgComponentNotifyBinding                                          
//                                                                       
// The following functions provide the INetCfgComponentNotifyBinding interface.
//                                                                       
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//
// Function:  CMuxNotify::QueryBindingPath
//
// Purpose:  This is specific to the component being installed. This will 
//           ask us if we want to bind to the Item being passed into
//           this routine. We can disable the binding by returning
//           NETCFG_S_DISABLE_QUERY 
//
//
// Arguments:
//           IN dwChangeFlag: Type of binding change
//           IN pncbpItem   : Pointer to INetCfgBindingPath object
//
// Returns:   S_OK on success, otherwise an error code.
//
// Notes:
//
STDMETHODIMP CMuxNotify::QueryBindingPath (IN DWORD dwChangeFlag,  
                                           IN INetCfgBindingPath *pncbp)
{
    UNREFERENCED_PARAMETER(pncbp);
    UNREFERENCED_PARAMETER(dwChangeFlag);

    TraceMsg( L"-->CMuxNotify INetCfgNotifyBinding::QueryBindingPath.\n" );

    DumpChangeFlag( dwChangeFlag );
    DumpBindingPath( pncbp );

    TraceMsg( L"<--CMuxNotify INetCfgNotifyBinding::QueryBindingPath(HRESULT = %x).\n",
            S_OK );

    return S_OK;
}

// ----------------------------------------------------------------------
//
// Function:  CMuxNotify::NotifyBindingPath
//
// Purpose:  We are now being told to bind to the component passed to us. 
//
//
// Arguments:
//           IN dwChangeFlag: Type of system change
//           IN pncc        : Pointer to INetCfgComponent object
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//



STDMETHODIMP CMuxNotify::NotifyBindingPath (IN DWORD dwChangeFlag,  
                                            IN INetCfgBindingPath *pncbp)
{
    INetCfgComponent     *pnccLower;
    INetCfgComponent     *pnccUpper;
    LPWSTR               pszwInfIdLower;
    LPWSTR               pszwInfIdUpper;
    DWORD                dwCharcteristics;
    HRESULT              hr = S_OK;

    TraceMsg( L"-->CMuxNotify INetCfgNotifyBinding::NotifyBindingPath.\n" );

    DumpChangeFlag( dwChangeFlag );
    DumpBindingPath( pncbp );

     //
     // We are only interested to know 1) when a component is installed
     // and we are binding to it i.e. dwChangeFlag = NCN_ADD | NCN_ENABLE
     // and 2) when a component is removed to which we are bound i.e.
     // dwChangeFlag = NCN_REMOVE | NCN_ENABLE. dwChangeFlag is never
     // set to NCN_ADD or NCN_REMOVE only. So, checking for NCN_ENABLE
     // covers the case of NCN_ADD | NCN_ENABLE and checking for NCN_REMOVE
     // covers the case of NCN_REMOVE | NCN_ENABLE. We don't care about
     // NCN_ADD | NCN_DISABLE (case 1) and NCN_REMOVE | NCN_DISABLE (case 2).
     //

     if ( dwChangeFlag & (NCN_ENABLE | NCN_REMOVE) ) {

        //
        // Get the upper and lower components.
        //

        hr = HrGetUpperAndLower( pncbp,
                                 &pnccUpper,
                                 &pnccLower );

        if ( hr == S_OK ) {

            hr = pnccLower->GetCharacteristics( &dwCharcteristics );

            if ( hr == S_OK ) {
                hr = pnccLower->GetId( &pszwInfIdLower );

                if ( hr == S_OK ) {
                    hr = pnccUpper->GetId( &pszwInfIdUpper );

                    if ( hr == S_OK ) {

                        //
                        // We are interested only in binding to a
                        // physical ethernet adapters.
                        // 

                        if ( dwCharcteristics & NCF_PHYSICAL ) {

                            if ( !_wcsicmp( pszwInfIdUpper, c_szMuxProtocol ) ) {

                                if ( dwChangeFlag & NCN_ADD ) {

                                    hr = HrAddAdapter( pnccLower );
                                    if( hr != S_OK ){
                                        // you may do something
                                    }
                                    m_eApplyAction = eActAdd;

                                } else if ( dwChangeFlag & NCN_REMOVE ) {

                                    hr = HrRemoveAdapter( pnccLower );
                                    if( hr != S_OK ){
                                        // you may do something
                                    }
                                    m_eApplyAction = eActRemove;
                                }
                            }
                        } // Physical Adapters. 
                        else if (dwCharcteristics & NCF_VIRTUAL) {

                        }

                        CoTaskMemFree( pszwInfIdUpper );

                    } // Got the upper component id.

                    CoTaskMemFree( pszwInfIdLower );

                } // Got the lower component id.

            } // Got NIC's characteristics

            ReleaseObj(pnccLower);
            ReleaseObj(pnccUpper);

        } // Got the upper and lower components.

    } 

    TraceMsg( L"<--CMuxNotify INetCfgNotifyBinding::NotifyBindingPath(HRESULT = %x).\n",
            S_OK );

    return S_OK;
}




//----------------------------------------------------------------------------
// INetCfgComponentNotifyGlobal
//                                                                       
// The following functions provide the INetCfgComponentNotifyGlobal interface.
//                                                                       
//----------------------------------------------------------------------------

// ----------------------------------------------------------------------
//
// Function:  CMuxNotify::GetSupportedNotifications
//
// Purpose:   Tell the system which notifications we are interested in
//
// Arguments:
//            OUT pdwNotificationFlag: Pointer to NotificationFlag
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//
STDMETHODIMP CMuxNotify::GetSupportedNotifications (
                                             OUT DWORD* pdwNotificationFlag)
{
    TraceMsg( L"-->CMuxNotify INetCfgNotifyGlobal::GetSupportedNotifications.\n" );

    *pdwNotificationFlag = NCN_NET | NCN_NETTRANS | NCN_ADD | NCN_REMOVE |
                           NCN_BINDING_PATH | NCN_ENABLE | NCN_DISABLE;

    TraceMsg( L"<--CMuxNotify INetCfgNotifyGlobal::GetSupportedNotifications(HRESULT = %x).\n",
            S_OK );

    return S_OK;
}

// ----------------------------------------------------------------------
//
// Function:  CMuxNotify::SysQueryBindingPath
//
// Purpose:   Enable or Disable a binding path.
//
// Arguments:
//            IN dwChangeFlag: Type of binding change
//            IN pncbp       : Pointer to INetCfgBindingPath object
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//

STDMETHODIMP CMuxNotify::SysQueryBindingPath (DWORD dwChangeFlag,
                                              INetCfgBindingPath* pncbp)
{
    INetCfgComponent     *pnccLower;
    INetCfgComponent     *pnccUpper;
    LPWSTR               pszwInfIdLower;
    LPWSTR               pszwInfIdUpper;
    DWORD                dwCharcteristics;
    HRESULT              hr = S_OK;


    TraceMsg( L"-->CMuxNotify INetCfgNotifyGlobal::SysQueryBindingPath.\n" );

    DumpChangeFlag( dwChangeFlag );
    DumpBindingPath( pncbp );

    if ( dwChangeFlag & NCN_ENABLE ) {

        //
        // Get the upper and lower components.
        //

        hr = HrGetUpperAndLower( pncbp,
                                 &pnccUpper,
                                 &pnccLower );

        if ( hr == S_OK ) {
            hr = pnccLower->GetCharacteristics( &dwCharcteristics );

            if ( hr == S_OK ) {
                hr = pnccLower->GetId( &pszwInfIdLower );

                if ( hr == S_OK ) {
                    hr = pnccUpper->GetId( &pszwInfIdUpper );

                    if ( hr == S_OK ) {

                        //
                        // We are interested only in bindings to physical 
                        // ethernet adapters.
                        // 

                        if ( dwCharcteristics & NCF_PHYSICAL ) {

#ifdef DISABLE_PROTOCOLS_TO_PHYSICAL

                            //
                            // If it not our protocol binding to the
                            // physical adapter then, disable the
                            // binding.
                            //

                            if (_wcsicmp( pszwInfIdUpper, c_szMuxProtocol ) ) {

                                TraceMsg( L"   Disabling the binding between %s "
                                          L"and %s.\n",
                                          pszwInfIdUpper,
                                          pszwInfIdLower );

                                hr = NETCFG_S_DISABLE_QUERY;
                            }
#endif

                        } // Physical Adapters. 
                        else {
                            if (dwCharcteristics & NCF_VIRTUAL) {

                                // If the lower component is our miniport
                                // and the upper component is our protocol
                                // then also, disable the binding.

                                if ( !_wcsicmp(pszwInfIdLower, c_szMuxMiniport) &&
                                     !_wcsicmp(pszwInfIdUpper, c_szMuxProtocol) ) {
                                  
                                    TraceMsg( L"   Disabling the binding between %s "
                                              L"and %s.\n",
                                              pszwInfIdUpper,
                                              pszwInfIdLower );

                                    hr = NETCFG_S_DISABLE_QUERY;
                                }

                            } // Virtual Adapters

                        }

                        CoTaskMemFree( pszwInfIdUpper );

                    } // Got the upper component id.

                    CoTaskMemFree( pszwInfIdLower );

                } // Got the lower component id.

            } // Got NIC's characteristics

            ReleaseObj(pnccLower);
            ReleaseObj(pnccUpper);

        }

    }

    TraceMsg( L"<--CMuxNotify INetCfgNotifyGlobal::SysQueryBindingPath(HRESULT = %x).\n",
            hr );

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  CMuxNotify::SysNotifyBindingPath
//
// Purpose:   System tells us by calling this function which
//            binding path has just been formed.
//
// Arguments:
//            IN dwChangeFlag: Type of binding change
//            IN pncbpItem   : Pointer to INetCfgBindingPath object
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//
STDMETHODIMP CMuxNotify::SysNotifyBindingPath (DWORD dwChangeFlag,
                                               INetCfgBindingPath* pncbp)
{
    UNREFERENCED_PARAMETER(dwChangeFlag);
    UNREFERENCED_PARAMETER(pncbp);

    TraceMsg( L"-->CMuxNotify INetCfgNotifyGlobal::SysNotifyBindingPath.\n" );

    DumpChangeFlag( dwChangeFlag );
    DumpBindingPath( pncbp );

    TraceMsg( L"<--CMuxNotify INetCfgNotifyGlobal::SysNotifyBindingPath(HRESULT = %x).\n",
            S_OK );

    return S_OK;
}

// ----------------------------------------------------------------------
//
// Function:  CMuxNotify::SysNotifyComponent
//
// Purpose:   System tells us by calling this function which
//            component has undergone a change (installed/removed)
//
// Arguments:
//            IN dwChangeFlag: Type of system change
//            IN pncc        : Pointer to INetCfgComponent object
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//
STDMETHODIMP CMuxNotify::SysNotifyComponent (DWORD dwChangeFlag,
                                                INetCfgComponent* pncc)
{
    UNREFERENCED_PARAMETER(dwChangeFlag);
    UNREFERENCED_PARAMETER(pncc);

    TraceMsg( L"-->CMuxNotify INetCfgNotifyGlobal::SysNotifyComponent.\n" );

    DumpChangeFlag( dwChangeFlag );
    DumpComponent( pncc );

    TraceMsg( L"<--CMuxNotify INetCfgNotifyGlobal::SysNotifyComponent(HRESULT = %x).\n",
            S_OK );

    return S_OK;
}


//----------------------------------------------------------------------------
// INetCfgComponentPropertyUi                                          
//                                                                       
// The following functions provide the INetCfgComponentPropertyUi interface.
//                                                                       
//----------------------------------------------------------------------------

// ----------------------------------------------------------------------
//
// Function:  CMuxNotify::MergePropPages
//
// Purpose:   Supply our property page to system.
//
// Arguments:
//            OUT pdwDefPages  : Pointer to num default pages
//            OUT pahpspPrivate: Pointer to array of pages
//            OUT pcPages      : Pointer to num pages
//            IN  hwndParent   : Handle of parent window
//            IN  szStartPage  : Pointer to
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//
STDMETHODIMP CMuxNotify::MergePropPages (IN OUT DWORD* pdwDefPages,
                                         OUT LPBYTE* pahpspPrivate,
                                         OUT UINT* pcPages,
                                         IN HWND hwndParent,
                                         OUT PCWSTR* szStartPage)
{
    HRESULT                 hr = S_OK;
    HPROPSHEETPAGE          *ahpsp;;
    INetLanConnectionUiInfo *pLanConnUiInfo;

    UNREFERENCED_PARAMETER(szStartPage);
    UNREFERENCED_PARAMETER(hwndParent);

    TraceMsg(L"-->CMuxNotify INetCfgPropertyUi::MergePropPages\n");

    //
    // We don't want any default pages to be shown
    //

    *pdwDefPages = 0;
    *pcPages = 0;
    *pahpspPrivate = NULL;

    if ( !m_pUnkContext ) {
        return E_UNEXPECTED;
    }

    hr = m_pUnkContext->QueryInterface(
          IID_INetLanConnectionUiInfo,
          reinterpret_cast<PVOID *>(&pLanConnUiInfo));

    if ( hr == S_OK ) {

        ReleaseObj( pLanConnUiInfo );

        ahpsp = (HPROPSHEETPAGE*)CoTaskMemAlloc( sizeof(HPROPSHEETPAGE) );

        if (ahpsp) {

            PROPSHEETPAGE   psp = {0};

            psp.dwSize            = sizeof(PROPSHEETPAGE);
            psp.dwFlags           = PSP_DEFAULT;
            psp.hInstance         = _Module.GetModuleInstance();
            psp.pszTemplate       = MAKEINTRESOURCE(IDD_NOTIFY_GENERAL);
            psp.pfnDlgProc        = NotifyDialogProc;
            psp.pfnCallback       = NULL; (LPFNPSPCALLBACK)NotifyPropSheetPageProc;
            psp.lParam            = (LPARAM) this;
            psp.pszHeaderTitle    = NULL;
            psp.pszHeaderSubTitle = NULL;

            ahpsp[0] = ::CreatePropertySheetPage(&psp);
            *pcPages = 1;
            *pahpspPrivate = (LPBYTE)ahpsp;
        }
        else {
            hr = E_OUTOFMEMORY;
        }
    }
    TraceMsg(L"<--CMuxNotify INetCfgPropertyUi::MergePropPages(HRESULT = %x).\n",
           hr );

    return hr;
}


// ----------------------------------------------------------------------
//
// Function:  CMuxNotify::ValidateProperties
//
// Purpose:   Validate changes to property page.
//
// Arguments:
//            IN hwndSheet: Window handle of property sheet
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//

STDMETHODIMP CMuxNotify::ValidateProperties (HWND hwndSheet)
{

    UNREFERENCED_PARAMETER(hwndSheet);

    TraceMsg( L"-->CMuxNotify INetCfgPropertyUi::ValidateProperties\n" );

    TraceMsg(L"<--CMuxNotify INetCfgPropertyUi::ValidateProperties(HRESULT = %x).\n",
           S_OK );
    return S_OK;
}

// ----------------------------------------------------------------------
//
// Function:  CMuxNotify::CancelProperties
//
// Purpose:   Cancel changes to property page
//
// Arguments: None
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//
STDMETHODIMP CMuxNotify::CancelProperties (VOID)
{
    TraceMsg(L"-->CMuxNotify INetCfgPropertyUi::CancelProperties\n");

    TraceMsg(L"<--CMuxNotify INetCfgPropertyUi::CancelProperties(HRESULT = %x).\n",
           S_OK );

    return S_OK;
}

// ----------------------------------------------------------------------
//
// Function:  CMuxNotify::ApplyProperties
//
// Purpose:   Apply value of controls on property page
//            to internal memory structure
//
// Arguments: None
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:     
//
STDMETHODIMP CMuxNotify::ApplyProperties (VOID)
{
    INetLanConnectionUiInfo *pLanConnUiInfo;
    CMuxPhysicalAdapter     *pAdapter;
    GUID                    guidAdapter;
    HRESULT                 hr = S_OK;

    TraceMsg(L"-->CMuxNotify INetCfgPropertyUi::ApplyProperties\n");

    if ( m_pUnkContext ) {

        hr = m_pUnkContext->QueryInterface(
                                  IID_INetLanConnectionUiInfo,
                                  reinterpret_cast<PVOID *>(&pLanConnUiInfo));

        if ( hr == S_OK ) {

            hr = pLanConnUiInfo->GetDeviceGuid( &guidAdapter );

            if ( hr == S_OK ) {

                hr = m_AdaptersList.FindByKey( guidAdapter,
                                               &pAdapter );
                if ( hr == S_OK ) {

                    switch( m_eApplyAction ) {

                        case eActPropertyUIAdd:

                              hr = HrAddMiniport( pAdapter,
                                                  &guidAdapter );
                        break;

                        case eActPropertyUIRemove:

                              hr = HrRemoveMiniport( pAdapter,
                                                     &guidAdapter );
                        break;
                    }
                }
            }

            ReleaseObj( pLanConnUiInfo );
        }
    }

    TraceMsg(L"<--CMuxNotify INetCfgPropertyUi::ApplyProperties(HRESULT = %x).\n",
           hr );
    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  CMuxNotify::QueryPropertyUi
//
// Purpose:   System is asking if we support property pages.
//
// Arguments:
//            IN pUnk: Pointer to IUnknown.
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:     We display property pages only in the context of
//            a LAN connection.
//

STDMETHODIMP CMuxNotify::QueryPropertyUi (IUnknown * pUnk)
{
    INetLanConnectionUiInfo *pLanConnUiInfo;
    HRESULT                 hr=S_FALSE;

    TraceMsg(L"-->CMuxNotify INetCfgPropertyUi::QueryPropertyUi\n");

#ifndef PASSTHRU_NOTIFY

    if ( pUnk ) {

        hr = pUnk->QueryInterface(
                              IID_INetLanConnectionUiInfo,
                              reinterpret_cast<PVOID *>(&pLanConnUiInfo));

        ReleaseObj( pLanConnUiInfo );
    } 
#endif

    TraceMsg(L"<--CMuxNotify INetCfgPropertyUi::QueryPropertyUi(HRESULT = %x).\n",
           hr );

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  CMuxNotify::SetContext
//
// Purpose:   Save the LAN connection context.
//
// Arguments: 
//            IN pUnk: Pointer to IUnknown.
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:     It is also called to release the current LAN connection context.
//

STDMETHODIMP CMuxNotify::SetContext (IUnknown * pUnk)
{
    TraceMsg(L"-->CMuxNotify INetCfgPropertyUi::SetContext\n");

    //
    // Release previous context, if any
    //

    ReleaseObj( m_pUnkContext );

    m_pUnkContext = NULL;

    if ( pUnk ) {

        m_pUnkContext = pUnk;
        m_pUnkContext->AddRef();
    }

    TraceMsg(L"<--CMuxNotify INetCfgPropertyUi::SetContext(HRESULT = %x).\n",
           S_OK );

    return S_OK;
}

//----------------------------------------------------------------------------
//
//  Function:   CMuxNotify::HrLoadAdapterConfiguration
//
//  Purpose:    This loads the Miniport and adapters that have already been 
//              installed into our own data structures
//
//  Arguments:  None.
//
//  Returns:    S_OK, or an error.
//
//
//  Notes:
//


HRESULT CMuxNotify::HrLoadAdapterConfiguration (VOID)
{
    HKEY                 hkeyAdapterList;
    WCHAR                szAdapterGuid[MAX_PATH+1];
    DWORD                dwDisp;
    CMuxPhysicalAdapter  *pAdapter;
    GUID                 guidAdapter;
    DWORD                dwIndex;
    LONG                 lResult;

    TraceMsg( L"-->CMuxNotify::HrLoadAdapterConfiguration.\n" );

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

        //
        // If dwDisp indicates that a new key is created then, we know there
        // is no adapter currently listed underneath and we simply
        // return, otherwise, we enumerate the subkeys, each one representing an 
        // adapter.
        //

        if ( dwDisp != REG_CREATED_NEW_KEY ) {

            lResult = RegEnumKeyW( hkeyAdapterList,
                                   0,
                                   szAdapterGuid,
                                   MAX_PATH+1 );

            for (dwIndex=1; lResult == ERROR_SUCCESS; ++dwIndex) {

                TraceMsg( L"   Loading configuration for adapter %s...\n",
                         szAdapterGuid );

                //
                // Subkeys are actually a guid/bindname of the adapters.
                //
                szAdapterGuid[MAX_PATH]='\0';
                HRESULT hrResult = CLSIDFromString( szAdapterGuid,
                                 &guidAdapter );

                if (hrResult != S_OK) {

                    lResult = ERROR_INVALID_PARAMETER;
                }

                //
                // Create an instance representing the adapter.
                //

                #pragma prefast(suppress:8197, "The instance is freed in the destructor")
                pAdapter = new CMuxPhysicalAdapter( m_pnc,
                                                    &guidAdapter );

                if ( pAdapter ) {

                  //
                  // Load any adapter specific configuration.
                  //

                  pAdapter->LoadConfiguration();

                  //
                  // Save the adapter instance in a list.
                  //

                  m_AdaptersList.Insert( pAdapter,
                                         guidAdapter );

                  //
                  // Get next subkey.
                  //

                  lResult = RegEnumKeyW( hkeyAdapterList,
                                         dwIndex,
                                         szAdapterGuid,
                                         MAX_PATH+1 );
                }
                else {

                 lResult = ERROR_NOT_ENOUGH_MEMORY;
                }
            }

            //
            // RegEnumKeyW may have returned error when there are no more
            // subkeys to read.
            //

            lResult = ERROR_SUCCESS;
        }

        RegCloseKey( hkeyAdapterList );
    }

    TraceMsg( L"<--CMuxNotify::HrLoadAdapterConfiguration(HRESULT = %x).\n",
              HRESULT_FROM_WIN32(lResult) );

    return HRESULT_FROM_WIN32(lResult);
}

//----------------------------------------------------------------------------
//
//  Function:   CMuxNotify::HrGetUpperAndLower
//
//  Purpose:    Get the upper and lower component of the first interface
//              of a binding path.
//
//  Arguments:  
//              IN  pncbp     : Binding path.
//              OUT ppnccUpper: Upper component.
//              OUT ppnccLower: Lower component.
//
//  Returns:    S_OK, or an error.
//
//
//  Notes:
//

HRESULT CMuxNotify::HrGetUpperAndLower (INetCfgBindingPath* pncbp,
                                        INetCfgComponent **ppnccUpper,
                                        INetCfgComponent **ppnccLower)
{
    IEnumNetCfgBindingInterface*    pencbi;
    INetCfgBindingInterface*        pncbi;
    ULONG                           ulCount;
    HRESULT                         hr;

    TraceMsg( L"-->CMuxNotify::HrGetUpperAndLowerComponent.\n" );

    *ppnccUpper = NULL;
    *ppnccLower = NULL;

    hr = pncbp->EnumBindingInterfaces(&pencbi);

    if (S_OK == hr) {
     
        //
        // get the first binding interface
        //

        hr = pencbi->Next(1, &pncbi, &ulCount);

        if ( hr == S_OK ) {

            hr = pncbi->GetUpperComponent( ppnccUpper );

            if ( hr == S_OK ) {

                hr = pncbi->GetLowerComponent ( ppnccLower );
            }
            else {
                if( ppnccUpper != NULL )
                {
                    ReleaseObj( *ppnccUpper );
                }
            }

            ReleaseObj( pncbi );
        }

        ReleaseObj( pencbi );
    }

    TraceMsg( L"<--CMuxNotify::HrGetUpperAndLowerComponent(HRESULT = %x).\n",
            hr );

    return hr;
}

//----------------------------------------------------------------------------
//
//  Function:   CMuxNotify::HrAddAdapter
//
//  Purpose:    Create an instance representing the physical adapter and install
//              a virtual miniport.
//
//  Arguments:  
//              IN pnccAdapter: Pointer to the physical adapter.
//
//  Returns:    S_OK, or an error.
//
//
//  Notes:
//

HRESULT CMuxNotify::HrAddAdapter (INetCfgComponent *pnccAdapter)
{
    GUID                     guidAdapter;
    CMuxPhysicalAdapter      *pAdapter;
    HRESULT                  hr;

    TraceMsg( L"-->CMuxNotify::HrAddAdapter.\n" );

    hr = pnccAdapter->GetInstanceGuid( &guidAdapter );

    if ( hr == S_OK ) {

        #pragma prefast(suppress:8197, "The instance is freed in the destructor")
        pAdapter = new CMuxPhysicalAdapter( m_pnc,
                                            &guidAdapter );

        if ( pAdapter ) {

            hr = HrAddMiniport( pAdapter,
                                &guidAdapter );

            if ( hr == S_OK ) {

               m_AdaptersToAdd.Insert( pAdapter,
                                       guidAdapter );
            }
            else {

               delete pAdapter;
            }
        }
        else {
            hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        }
    } 

    TraceMsg( L"<--CMuxNotify::HrAddAdapter(HRESULT = %x).\n",
            hr );

    return hr;
}

//----------------------------------------------------------------------------
//
//  Function:   CMuxNotify::HrRemoveAdapter
//
//  Purpose:    Deletes the instance representing the physical adapter
//              and uninstalls all the virtual miniports.
//
//  Arguments:  
//              IN pnccAdapter: Pointer to the physical adapter.
//
//  Returns:    S_OK, or an error.
//
//
//  Notes:      This function is called when the adapter or the protocol
//              is being uninstalled.
//

HRESULT CMuxNotify::HrRemoveAdapter (INetCfgComponent *pnccAdapter)
{
    GUID                  guidAdapter;
    CMuxPhysicalAdapter   *pAdapter;
    HRESULT               hr;

    TraceMsg( L"-->CMuxNotify::HrRemoveAdapter.\n" );

    hr = pnccAdapter->GetInstanceGuid( &guidAdapter );

    if ( hr == S_OK ) {

        hr = m_AdaptersList.RemoveByKey( guidAdapter,
                                      &pAdapter );

         if ( hr == S_OK ) {

            m_AdaptersToRemove.Insert( pAdapter,  
                                       guidAdapter );
            hr = pAdapter->Remove();

#ifdef DISABLE_PROTOCOLS_TO_PHYSICAL

            //
            // Restore the bindings of other protocols to the physical
            // adapter.
            //
 
            EnableBindings( pnccAdapter,
                            TRUE );
#endif
         }
    }

    TraceMsg( L"<--CMuxNotify::HrRemoveAdapter(HRESULT = %x).\n",
            hr );

    return hr;
}

//----------------------------------------------------------------------------
//
//  Function:   CMuxNotify::HrAddMiniport
//
//  Purpose:    Installs a virtual miniport.
//
//  Arguments:  
//              IN pAdapter    : Pointer to the physical adapter class instance.
//              IN pguidAdapter: Pointer to the GUID of the adapter.
//
//  Returns:    S_OK, or an error.
//
//
//  Notes:      
//

HRESULT CMuxNotify::HrAddMiniport (CMuxPhysicalAdapter *pAdapter,
                                   GUID *pguidAdapter)
{
    CMuxVirtualMiniport   *pMiniport;
    INetCfgComponent      *pnccAdapter;
    HRESULT               hr = S_OK;

    TraceMsg( L"-->CMuxNotify::HrAddMiniport.\n" );

    //
    // Limit the number of virtual miniports 
    //
    if ((pAdapter->MiniportCount() + pAdapter->MiniportAddCount()) >= MAX_VIRTUAL_MP_PER_ADAPTER)
    {
        hr = HRESULT_FROM_WIN32( ERROR_NO_SYSTEM_RESOURCES );   
        TraceMsg( L"   Virtual miniport limit reached\n" );
    }

    if ( hr == S_OK ) 
    {
        #pragma prefast(suppress:8197, "The instance is freed in the destructor")
        pMiniport = new CMuxVirtualMiniport( m_pnc,
                                             NULL,
                                             pguidAdapter );
        if ( pMiniport ) {

            hr = pMiniport->Install();

            if ( hr == S_OK ) {

                hr = pAdapter->AddMiniport( pMiniport );

                if ( hr != S_OK ) {

                    pMiniport->DeInstall();

                    delete pMiniport;
                }
            }
            else
            {
                delete pMiniport;
            }
        }
        else {

            hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        }
    }

#ifdef DISABLE_PROTOCOLS_TO_PHYSICAL

    if ( hr == S_OK ) {

        //
        // If this is the first virtual miniport then, disable the bindings
        // of other protocols to the physical adapter.
        //

        if ( pAdapter->MiniportCount() == 0 ) {

            hr = HrFindInstance( m_pnc,
                                 *pguidAdapter,
                                 &pnccAdapter );

            if ( hr == S_OK ) {
                EnableBindings( pnccAdapter,
                                FALSE );

                ReleaseObj( pnccAdapter );
            }
        }
    }
#endif

    TraceMsg( L"<--CMuxNotify::HrAddMiniport(HRESULT = %x).\n",
            hr );
    return hr;
}

//----------------------------------------------------------------------------
//
//  Function:   CMuxNotify::HrRemoveMiniport
//
//  Purpose:    Uninstalls a virtual miniport.
//
//  Arguments:  
//              IN pAdapter    : Pointer to the physical adapter class instance.
//              IN pguidAdapter: Pointer to the GUID of the adapter.
//
//  Returns:    S_OK, or an error.
//
//
//  Notes:      
//

HRESULT CMuxNotify::HrRemoveMiniport (CMuxPhysicalAdapter *pAdapter,
                                      GUID *pguidAdapter)
{
    INetCfgComponent      *pnccAdapter;
    HRESULT                hr;

    TraceMsg( L"-->CMuxNotify::HrRemoveMiniport.\n" );

    hr = pAdapter->RemoveMiniport( NULL );

#ifdef DISABLE_PROTOCOLS_TO_PHYSICAL

    if ( hr == S_OK ) {

        //
        // If this was the last miniport that was removed then, restore the
        // bindings of other protocols to the physical adapter.
        //

        if ( pAdapter->AllMiniportsRemoved() ) {

            hr = HrFindInstance( m_pnc,
                                 *pguidAdapter,
                                 &pnccAdapter );

            if ( hr == S_OK ) {
                EnableBindings( pnccAdapter,
                                TRUE );

                ReleaseObj( pnccAdapter );
            }
        }
    }
#endif

    TraceMsg( L"<--CMuxNotify::HrRemoveMiniport(HRESULT = %x).\n",
            hr );

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  CMuxNotify::OnInitDialog
//
// Purpose:   Initialize controls
//
// Arguments:
//            IN hWnd: Window handle to the property page.
//
// Returns: TRUE.
//
// Notes:
//

LRESULT CMuxNotify::OnInitDialog (IN HWND hWndPage)
{
    m_eApplyAction = eActUnknown;

    ::SendMessage(GetDlgItem(hWndPage, IDC_ADD), BM_SETCHECK, BST_CHECKED, 0);
    ::SendMessage(GetDlgItem(hWndPage, IDC_REMOVE), BM_SETCHECK, BST_UNCHECKED, 0);

    return TRUE;
}

// ----------------------------------------------------------------------
//
// Function:  CMuxNotify::OnOk
//
// Purpose:   Do actions when OK is pressed
//
// Arguments:
//            IN hWnd: Window handle to the property page.
//
// Returns:   PSNRET_NOERROR
//
// Notes:
//

LRESULT CMuxNotify::OnOk (IN HWND hWndPage)
{
    TraceMsg(L"-->CMuxNotify::OnOk\n");

    if ( ::SendMessage(GetDlgItem(hWndPage, IDC_ADD),
                       BM_GETCHECK, 0, 0) == BST_CHECKED ) {
     
        m_eApplyAction = eActPropertyUIAdd;
    }
    else {
        m_eApplyAction = eActPropertyUIRemove;
    }

    //
    // Set the property sheet changed flag if any of our controls
    // get changed.  This is important so that we get called to
    // apply our property changes.
    //

    PropSheet_Changed( GetParent(hWndPage), hWndPage);

    TraceMsg(L"<--CMuxNotify::OnOk(Action = %s).\n",
           (m_eApplyAction == eActPropertyUIAdd) ? L"Add" : L"Remove" );

    return PSNRET_NOERROR;
}

// ----------------------------------------------------------------------
//
// Function:  CMuxNotify::OnCancel
//
// Purpose:   Do actions when CANCEL is pressed
//
// Arguments:
//            IN hWnd: Window handle to the property page.
//
// Returns:   FALSE
//
// Notes:
//
LRESULT CMuxNotify::OnCancel (IN HWND hWndPage)
{
    UNREFERENCED_PARAMETER(hWndPage);

    TraceMsg(L"-->CMuxNotify::OnCancel\n");

    m_eApplyAction = eActUnknown;

    TraceMsg(L"<--CMuxNotify::OnCancel\n");

    return FALSE;
}

// ----------------------------------------------------------------------
//
// Function:  CMuxNotifyDialogProc
//
// Purpose:   Dialog proc
//
// Arguments:
//            IN hWnd  : See win32 documentation.
//            IN uMsg  : See win32 documentation.
//            IN wParam: See win32 documentation.
//            IN lParam: See win32 documentation.
//
// Returns:   See win32 documentation.
//
// Notes:
//
INT_PTR CALLBACK NotifyDialogProc (HWND hWnd,
                                   UINT uMsg,
                                   WPARAM wParam,
                                   LPARAM lParam)
{
    CMuxNotify     *psf;
    LRESULT        lRes=FALSE;

    UNREFERENCED_PARAMETER(wParam);

    if ( uMsg != WM_INITDIALOG ) {

        psf = (CMuxNotify *)::GetWindowLongPtr( hWnd,
                                                DWLP_USER );

        // Until we get WM_INITDIALOG, just return FALSE

        if ( !psf ) {

            return lRes;
        }
    }

    switch( uMsg ) {

        case WM_INITDIALOG:
        {
            PROPSHEETPAGE* ppsp;
            ppsp = (PROPSHEETPAGE *)lParam;

            psf = (CMuxNotify *)ppsp->lParam;

            SetWindowLongPtr( hWnd,
                          DWLP_USER,
                          (LONG_PTR)psf);

            lRes = psf->OnInitDialog( hWnd );
        }
        break;

        case WM_COMMAND:

        break;

        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;

            switch (pnmh->code) {
        
            case PSN_KILLACTIVE:

                //
                // ok to loose focus.
                //

                SetWindowLongPtr( hWnd, DWLP_MSGRESULT, FALSE);

                lRes = TRUE;
                break;

            case PSN_APPLY:

                psf = (CMuxNotify *)::GetWindowLongPtr( hWnd, DWLP_USER);
                lRes = psf->OnOk( hWnd );

                SetWindowLongPtr( hWnd, DWLP_MSGRESULT, lRes);
                lRes = TRUE;
                break;

            case PSN_RESET:

                psf = (CMuxNotify *)::GetWindowLongPtr( hWnd, DWLP_USER);
                psf->OnCancel( hWnd );
        }
     }
  }

  return lRes;
}

// ----------------------------------------------------------------------
//
// Function:  CMuxNotifyPropSheetPageProc
//
// Purpose:   Prop sheet proc
//
// Arguments:
//            IN hWnd: See win32 documentation
//            IN uMsg: See win32 documentation
//            IN ppsp: See win32 documentation
//
// Returns:   See win32 documentation
//
// Notes:
//

UINT CALLBACK NotifyPropSheetPageProc(HWND hWnd,
                                      UINT uMsg,
                                      LPPROPSHEETPAGE ppsp)
{
    UNREFERENCED_PARAMETER(hWnd);
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(ppsp);

    return TRUE;
}


#ifdef DISABLE_PROTOCOLS_TO_PHYSICAL

// ----------------------------------------------------------------------
//
// Function:  CMuxNotify::EnableBindings
//
// Purpose:   Enable/Disable the bindings of other protocols to 
//            the physical adapter.
//
// Arguments:
//            IN pnccAdapter: Pointer to the physical adapter.
//            IN bEnable: TRUE/FALSE to enable/disable respectively.
//
// Returns:   None.
//
// Notes:
//

VOID CMuxNotify::EnableBindings (INetCfgComponent *pnccAdapter,
                                 BOOL bEnable)
{
    IEnumNetCfgBindingPath      *pencbp;
    INetCfgBindingPath          *pncbp;
    HRESULT                     hr;
  
    TraceMsg( L"-->CMuxNotify::EnableBindings.\n" );


    //
    // Get the binding path enumerator.
    //

    hr = HrGetBindingPathEnum( pnccAdapter,
                               EBP_ABOVE,
                               &pencbp );
    if ( hr == S_OK ) {

        hr = HrGetBindingPath( pencbp,
                               &pncbp );

        //
        // Traverse each binding path.
        //

        while( hr == S_OK ) {

            //
            // If our protocol does exist in the binding path then,
            // disable it.
            //

            if ( !IfExistMux(pncbp) ) {

                pncbp->Enable( bEnable );
            }

            ReleaseObj( pncbp );

            hr = HrGetBindingPath( pencbp,
                                   &pncbp );
        }

        ReleaseObj( pencbp );
    }
    else {
        TraceMsg( L"   Couldn't get the binding path enumerator, "
                  L"bindings will not be %s.\n",
                  bEnable ? L"enabled" : L"disabled" );
    }

    TraceMsg( L"<--CMuxNotify::EnableBindings.\n" );

    return;
}

// ----------------------------------------------------------------------
//
// Function:  CMuxNotify::IfExistMux
//
// Purpose:   Determine if a given binding path contains our protocol.
//
// Arguments:
//            IN pncbp: Pointer to the binding path.
//
// Returns:   TRUE if our protocol exists, otherwise FALSE.
//
// Notes:
//

BOOL CMuxNotify::IfExistMux (INetCfgBindingPath *pncbp)
{
    IEnumNetCfgBindingInterface *pencbi;
    INetCfgBindingInterface     *pncbi;
    INetCfgComponent            *pnccUpper;
    LPWSTR                      lpszIdUpper;
    HRESULT                     hr;
    BOOL                        bExist = FALSE;

    TraceMsg( L"-->CMuxNotify::IfExistMux.\n" );

    //
    // Get the binding interface enumerator.
    //

    hr = HrGetBindingInterfaceEnum( pncbp,
                                  &pencbi );

    if ( hr == S_OK ) {

        //
        // Traverse each binding interface.
        //

        hr = HrGetBindingInterface( pencbi,
                                    &pncbi );

        while( !bExist && (hr == S_OK) ) {

            //
            // Is the upper component our protocol?
            //

            hr = pncbi->GetUpperComponent( &pnccUpper );

            if ( hr == S_OK ) {

                hr = pnccUpper->GetId( &lpszIdUpper );

                if ( hr == S_OK ) {

                    bExist = !_wcsicmp( lpszIdUpper, c_szMuxProtocol );

                    CoTaskMemFree( lpszIdUpper );
                }
                else {
                    TraceMsg( L"   Failed to get the upper component of the interface.\n" );
                }

                ReleaseObj( pnccUpper );
            }
            else {
                TraceMsg( L"   Failed to get the upper component of the interface.\n" );
            }

            ReleaseObj( pncbi );

            if ( !bExist ) {
                hr = HrGetBindingInterface( pencbi,
                                            &pncbi );
            }
        }

        ReleaseObj( pencbi );
    }
    else {
        TraceMsg( L"   Couldn't get the binding interface enumerator.\n" );
    }

    TraceMsg( L"<--CMuxNotify::IfExistMux(BOOL = %x).\n",
            bExist );

    return bExist;
}

// ----------------------------------------------------------------------
//
// Function:  CMuxNotify::HrGetBindingPathEnum
//
// Purpose:   Returns the binding path enumerator.
//
// Arguments:
//            IN  pnccAdapter  : Pointer to the physical adapter.
//            IN  dwBindingType: Type of binding path enumerator.
//            OUT ppencbp      : Pointer to the binding path enumerator.
//
// Returns:   S_OK on success, otherwise an error code.
//
// Notes:
//

HRESULT CMuxNotify::HrGetBindingPathEnum (
                                     INetCfgComponent *pnccAdapter,
                                     DWORD dwBindingType,
                                     IEnumNetCfgBindingPath **ppencbp)
{
    INetCfgComponentBindings *pnccb = NULL;
    HRESULT                  hr;

    *ppencbp = NULL;

    hr = pnccAdapter->QueryInterface( IID_INetCfgComponentBindings,
                               (PVOID *)&pnccb );

    if ( hr == S_OK ) {
        hr = pnccb->EnumBindingPaths( dwBindingType,
                                      ppencbp );

        ReleaseObj( pnccb );
    }

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  CMuxNotify::HrGetBindingPath
//
// Purpose:   Returns a binding path.
//
// Arguments:
//            IN  pencbp  : Pointer to the binding path enumerator.
//            OUT ppncbp  : Pointer to the binding path.
//
// Returns:   S_OK on success, otherwise an error code.
//
// Notes:
//

HRESULT CMuxNotify::HrGetBindingPath (IEnumNetCfgBindingPath *pencbp,
                                      INetCfgBindingPath **ppncbp)
{
    ULONG   ulCount;
    HRESULT hr;

    *ppncbp = NULL;

    hr = pencbp->Next( 1,
                       ppncbp,
                       &ulCount );

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  CMuxNotify::HrGetBindingInterfaceEnum
//
// Purpose:   Returns the binding interface enumerator.
//
// Arguments:
//            IN  pncbp  : Pointer to the binding path.
//            OUT ppencbi: Pointer to the binding path enumerator.
//
// Returns:   S_OK on success, otherwise an error code.
//
// Notes:
//

HRESULT CMuxNotify::HrGetBindingInterfaceEnum (
                                     INetCfgBindingPath *pncbp,
                                     IEnumNetCfgBindingInterface **ppencbi)
{
    HRESULT hr;

    *ppencbi = NULL;

    hr = pncbp->EnumBindingInterfaces( ppencbi );

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  CMuxNotify::HrGetBindingInterface
//
// Purpose:   Returns a binding interface.
//
// Arguments:
//            IN  pencbi  : Pointer to the binding interface enumerator.
//            OUT ppncbi  : Pointer to the binding interface.
//
// Returns:   S_OK on success, otherwise an error code.
//
// Notes:
//

HRESULT CMuxNotify::HrGetBindingInterface (
                                     IEnumNetCfgBindingInterface *pencbi,
                                     INetCfgBindingInterface **ppncbi)
{
    ULONG   ulCount;
    HRESULT hr;

    *ppncbi = NULL;

    hr = pencbi->Next( 1,
                       ppncbi,
                       &ulCount );

    return hr;
}

#endif

