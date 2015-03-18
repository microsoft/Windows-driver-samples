/*++

Copyright (c) 2005 Microsoft Corporation

Abstract:

   Sample IHV Extensibility DLL to extend
   802.11 LWF driver for third party protocols.


--*/

#include "precomp.h"



//
// Get Version info.
//
DWORD
WINAPI
Dot11ExtIhvGetVersionInfo
(
   OUT   PDOT11_IHV_VERSION_INFO    pDot11IHVVersionInfo
)
{
   if ( pDot11IHVVersionInfo )
   {
      pDot11IHVVersionInfo->dwVerMin = 0;
      pDot11IHVVersionInfo->dwVerMax = 0;
   }
    return ERROR_SUCCESS;
}


//
// Initialize service.
//

DWORD
WINAPI
Dot11ExtIhvInitService
(
   IN    DWORD                      dwVerNumUsed,
   IN    PDOT11EXT_APIS             pDot11ExtAPI,
   IN    LPVOID                     pvReserved,
   OUT   PDOT11EXT_IHV_HANDLERS     pDot11IHVHandlers
)
{
    DWORD   dwResult    =   ERROR_SUCCESS;
    BOOL    bLocked     =   FALSE;

    UNREFERENCED_PARAMETER( pvReserved );

    if
    (
        ( 0 != dwVerNumUsed )      ||
        ( !pDot11ExtAPI )          ||
        ( !pDot11IHVHandlers )
    )
    {
        dwResult = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR( dwResult );
    }

    EnterCriticalSection( &g_csSynch );
    bLocked = TRUE;

    InitAdapterDetailsList( );

    g_pDot11ExtApi = (PDOT11EXT_APIS) PrivateMemoryAlloc( sizeof( DOT11EXT_APIS ) );
    if ( !g_pDot11ExtApi )
    {
        dwResult = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR( dwResult );
    }
    (*g_pDot11ExtApi)    =  (*pDot11ExtAPI);

    HandlerInit( pDot11IHVHandlers );


error:
    if ( bLocked )
    {
        LeaveCriticalSection( &g_csSynch );
    }
    return dwResult;
}





//
// Deinitialize service.
//
VOID
WINAPI
IhvDeinitService
(
   VOID
)
{

    EnterCriticalSection( &g_csSynch );

    // disable starting new threads or adding new adapters.
    StartShutdown( );

    LeaveCriticalSection( &g_csSynch );


    WaitOnZeroThreads( );
    DeinitAdapterDetailsList( );

    if ( g_pDot11ExtApi )
    {
        PrivateMemoryFree( g_pDot11ExtApi );
        g_pDot11ExtApi = NULL;
    }

    return;
}



//
// Initialize adapter
//
DWORD
WINAPI
IhvInitAdapter
(
   IN    PDOT11_ADAPTER    pDot11Adapter,
   IN    HANDLE            hDot11SvcHandle,
   OUT   PHANDLE           phIhvExtAdapter
)
{
    DWORD       dwResult            =   ERROR_SUCCESS;
    BOOL        bLocked             =   FALSE;
    HANDLE      hIhvExtAdapter      =   NULL;
    USHORT      usEtherTypeReg[]    =   { 0x888e, 0x8333 };

    if (( !pDot11Adapter ) || (!hDot11SvcHandle) || (!phIhvExtAdapter) )
    {
        dwResult = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR( dwResult );
    }

    EnterCriticalSection( &g_csSynch );
    bLocked = TRUE;

    dwResult =
    InitAdapterDetails
    (
        hDot11SvcHandle,
        &hIhvExtAdapter
    );
    BAIL_ON_WIN32_ERROR( dwResult );

    dwResult =
    (g_pDot11ExtApi->Dot11ExtSetEtherTypeHandling)
    (
        hDot11SvcHandle,
        3,
        0,
        NULL,
        2,
        usEtherTypeReg
    );
    BAIL_ON_WIN32_ERROR( dwResult );

    (*phIhvExtAdapter)  =   hIhvExtAdapter;
    hIhvExtAdapter      =   NULL;

error:

    if ( hIhvExtAdapter )
    {
        DerefenceAdapterDetails( hIhvExtAdapter );
    }

    if ( bLocked )
    {
        LeaveCriticalSection( &g_csSynch );
    }
    return dwResult;
}



//
// Deinit adapter.
//
VOID
WINAPI
IhvDeinitAdapter
(
   IN    HANDLE   hIhvExtAdapter
)
{
    DerefenceAdapterDetails( hIhvExtAdapter );

    return;
}





//
// Handle session change notification.
//
DWORD
WINAPI
IhvProcessSessionChange
(
   IN    ULONG                         uEventType,
   IN    PWTSSESSION_NOTIFICATION      pSessionNotification
)
{
   UNREFERENCED_PARAMETER( pSessionNotification );

   if ( WTS_CONSOLE_CONNECT == uEventType )
   {
      // The sample does not use this session ID
      // anywhere. In an actual application, IHV
      // developers may want to use this session ID to
      // get/set the user data.
      g_dwSessionID = WTSGetActiveConsoleSessionId( );
   }
   return ERROR_SUCCESS;
}





//
// Checks if UI request is pending.
//
DWORD
WINAPI
IhvIsUIRequestPending
(
   IN    GUID     guidUIRequest,
   OUT   PBOOL    pbIsRequestPending
)
{
    DWORD                   dwResult        =   ERROR_SUCCESS;
    BOOL                    bLocked         =   FALSE;
    PADAPTER_DETAILS        pAdapterDetails =   NULL;
    HANDLE                  hIhvExtAdapter  =   NULL;


    ASSERT( pbIsRequestPending );

    EnterCriticalSection( &g_csSynch );
    bLocked = TRUE;

    // obtain reference to the adapter using
    // UI request GUID.
    dwResult =
    ReferenceAdapterDetailsByUIRequestGuid
    (
        &guidUIRequest,
        &pAdapterDetails,
        &hIhvExtAdapter
    );
    if ( ERROR_NOT_FOUND == dwResult )
    {
        dwResult = ERROR_SUCCESS;
        (*pbIsRequestPending) = FALSE;
        BAIL( );
    }
    BAIL_ON_WIN32_ERROR( dwResult );
    ASSERT( pAdapterDetails );


    (*pbIsRequestPending) = TRUE;

error:
    if ( pAdapterDetails )
    {
        DerefenceAdapterDetails( hIhvExtAdapter );
    }
    if ( bLocked )
    {
        LeaveCriticalSection( &g_csSynch );
    }
    return dwResult;
}




//
// Handle NIC specific notifications.
//
DWORD
WINAPI
IhvReceiveIndication
(
    IN  HANDLE                          hIhvExtAdapter,
    IN  DOT11EXT_IHV_INDICATION_TYPE    indicationType,
    IN  ULONG                           uBufferLength,
    IN  LPVOID                          pvBuffer
)
{
    DWORD                                   dwResult                    =   ERROR_SUCCESS;
    PDOT11_PMKID_CANDIDATE_LIST_PARAMETERS  pPMKCandidateListParams     =   NULL;
    PDOT11_TKIPMIC_FAILURE_PARAMETERS       pTkipMicFailureParams       =   NULL;
    PDOT11_PHY_STATE_PARAMETERS             pPHYStateChange             =   NULL;
    PDOT11_LINK_QUALITY_PARAMETERS          pDot11LinkQualityParams     =   NULL;

    ASSERT( pvBuffer );

    // Note that sample does not really use the buffer.
    UNREFERENCED_PARAMETER( hIhvExtAdapter );
    UNREFERENCED_PARAMETER( uBufferLength );

    switch ( indicationType )
    {
        case IndicationTypeNicSpecificNotification:
            // The pointer pvBuffer can be interpreted by IHV appropriately. Format
            // is a contract between the miniport driver and the IHV extensibility module.
            break;

        case IndicationTypePmkidCandidateList:
            pPMKCandidateListParams = (PDOT11_PMKID_CANDIDATE_LIST_PARAMETERS) pvBuffer;
            // Ihv extensibility module may choose to use this data appropriately.
            break;

        case IndicationTypeTkipMicFailure:
            pTkipMicFailureParams = (PDOT11_TKIPMIC_FAILURE_PARAMETERS) pvBuffer;
            // Ihv extensibility module may choose to use this data appropriately.
            break;

        case IndicationTypePhyStateChange:
            pPHYStateChange = (PDOT11_PHY_STATE_PARAMETERS) pvBuffer;
            // Ihv extensibility module may choose to use this data appropriately.
            break;

        case IndicationTypeLinkQuality:
            pDot11LinkQualityParams = (PDOT11_LINK_QUALITY_PARAMETERS) pvBuffer;
            // Ihv extensibility module may choose to use this data appropriately.
            break;

        default:
            ASSERTFAILURE();
            dwResult = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR( dwResult );
            break;
    }

error:
    return dwResult;
}






//
// Perform the capabiity match.
//
DWORD
WINAPI
IhvPerformCapabilityMatch
(
    IN  HANDLE                              hIhvExtAdapter,
    IN  PDOT11EXT_IHV_PROFILE_PARAMS        pIhvProfileParams,
    IN  PDOT11EXT_IHV_CONNECTIVITY_PROFILE  pIhvConnProfile,
    IN  PDOT11EXT_IHV_SECURITY_PROFILE      pIhvSecProfile,
    IN  PDOT11_BSS_LIST                     pConnectableBssid,
    OUT PDWORD                              pdwReasonCode
)
{
    DWORD                       dwResult                =   ERROR_SUCCESS;
    BOOL                        bLocked                 =   FALSE;
    PBYTE                       pbCurrPos               =   NULL;
    ULONG                       uRemainBytes            =   0;
    ULONG                       uBssEntryBytes          =   0;
    PULDOT11_BSS_ENTRY          pBssEntry               =   NULL;
    PADAPTER_DETAILS            pAdapterDetails         =   NULL;
    PIHV_CONNECTIVITY_PROFILE   pConnectivityProfile    =   NULL;
    PIHV_SECURITY_PROFILE       pSecurityProfile        =   NULL;

    ASSERT( hIhvExtAdapter );
    ASSERT( pdwReasonCode );

    (*pdwReasonCode) = L2_REASON_CODE_UNKNOWN;

    if ( !pConnectableBssid )
    {
        dwResult =
        IhvValidateProfile
        (
            hIhvExtAdapter,
            pIhvProfileParams,
            pIhvConnProfile,
            pIhvSecProfile,
            pdwReasonCode
        );
        BAIL_ON_WIN32_ERROR( dwResult );

        // Only validate profile if
        // pConnectableBssid is NULL.
        BAIL( );
    }

    // Validating data in pConnectableBssid
    if (( 0 == pConnectableBssid->uNumOfBytes ) || ( NULL == pConnectableBssid->pucBuffer ))
    {
        dwResult = ERROR_NO_MATCH;
        BAIL_ON_WIN32_ERROR( dwResult );
    }

    (*pdwReasonCode) = L2_REASON_CODE_IHV_BAD_PROFILE;

    // parse the connectivity profile.
    dwResult =
    GetIhvConnectivityProfile
    (
        pIhvConnProfile,
        &pConnectivityProfile
    );
    BAIL_ON_WIN32_ERROR( dwResult );
    ASSERT( pConnectivityProfile );

    // parse the security profile.
    dwResult =
    GetIhvSecurityProfile
    (
        pIhvSecProfile,
        &pSecurityProfile
    );
    BAIL_ON_WIN32_ERROR( dwResult );
    ASSERT( pSecurityProfile );

    (*pdwReasonCode) = L2_REASON_CODE_UNKNOWN;

    EnterCriticalSection( &g_csSynch );
    bLocked = TRUE;

    // reference the adapter.
    dwResult =
    ReferenceAdapterDetails
    (
        hIhvExtAdapter,
        &pAdapterDetails
    );
    BAIL_ON_WIN32_ERROR( dwResult );
    ASSERT( pAdapterDetails );


    // try matching each BSS description field with the profile
    // and see if there is a match.
    uRemainBytes   =  pConnectableBssid->uNumOfBytes;
    pbCurrPos      =  pConnectableBssid->pucBuffer;

    // BSS Description might not contain any buffer!
    while (uRemainBytes >= FIELD_OFFSET(DOT11_BSS_ENTRY, ucBuffer))
    {
        pBssEntry = (PULDOT11_BSS_ENTRY)pbCurrPos;
        uBssEntryBytes = pBssEntry->uBufferLength + FIELD_OFFSET(DOT11_BSS_ENTRY, ucBuffer);

        ASSERT (uRemainBytes >= uBssEntryBytes);
        uRemainBytes -= uBssEntryBytes;
        pbCurrPos += uBssEntryBytes;

        if
        (
            MatchBssDescription
            (
                pIhvProfileParams,
                pConnectivityProfile,
                pSecurityProfile,
                pBssEntry
            )
        )
        {
            (*pdwReasonCode) = L2_REASON_CODE_SUCCESS;
            dwResult = ERROR_SUCCESS;
            BAIL( );
        }
    }

    // Since none of the BSS entries matched.
    dwResult = ERROR_NO_MATCH;

error:
    if ( pAdapterDetails )
    {
        DerefenceAdapterDetails( hIhvExtAdapter );
    }
    if ( bLocked )
    {
        LeaveCriticalSection( &g_csSynch );
    }
    FreeIhvConnectivityProfile( &pConnectivityProfile );
    FreeIhvSecurityProfile( &pSecurityProfile );
    return dwResult;
}





//
// Function to validate profile.
//
DWORD
WINAPI
IhvValidateProfile
(
    IN  HANDLE                              hIhvExtAdapter,
    IN  PDOT11EXT_IHV_PROFILE_PARAMS        pIhvProfileParams,
    IN  PDOT11EXT_IHV_CONNECTIVITY_PROFILE  pIhvConnProfile,
    IN  PDOT11EXT_IHV_SECURITY_PROFILE      pIhvSecProfile,
    OUT PDWORD                              pdwReasonCode
)
{
    DWORD                       dwResult                =   ERROR_SUCCESS;
    PIHV_CONNECTIVITY_PROFILE   pConnectivityProfile    =   NULL;
    PIHV_SECURITY_PROFILE       pSecurityProfile        =   NULL;


    ASSERT( hIhvExtAdapter );
    ASSERT( pdwReasonCode );

    UNREFERENCED_PARAMETER( hIhvExtAdapter );
    UNREFERENCED_PARAMETER( pIhvProfileParams );

    (*pdwReasonCode) = L2_REASON_CODE_IHV_BAD_PROFILE;

    // parse ihv connectivity profile.
    dwResult =
    GetIhvConnectivityProfile
    (
        pIhvConnProfile,
        &pConnectivityProfile
    );
    BAIL_ON_WIN32_ERROR( dwResult );
    ASSERT( pConnectivityProfile );

    // parse ihv security profile.
    dwResult =
    GetIhvSecurityProfile
    (
        pIhvSecProfile,
        &pSecurityProfile
    );
    BAIL_ON_WIN32_ERROR( dwResult );
    ASSERT( pSecurityProfile );

    (*pdwReasonCode) = L2_REASON_CODE_SUCCESS;

error:
    FreeIhvConnectivityProfile( &pConnectivityProfile );
    FreeIhvSecurityProfile( &pSecurityProfile );
    return dwResult;
}


//
// This function does the actual preassociation work.
//
DWORD
WINAPI
DoPreAssociate
(
    LPVOID  pvPreAssociate
)
{
    DWORD                       dwResult            =   ERROR_SUCCESS;
    DWORD                       dwStatus            =   ERROR_SUCCESS;
    PADAPTER_DETAILS            pAdapterDetails     =   NULL;
    PRE_ASSOCIATE_FUNCTION      lpfnPreAssociate    =   NULL;
    BOOL                        bLocked             =   FALSE;
    DWORD                       dwReasonCode        =   L2_REASON_CODE_UNKNOWN;


    EnterCriticalSection( &g_csSynch );
    bLocked = TRUE;

    dwResult =
    ReferenceAdapterDetails
    (
        (HANDLE) pvPreAssociate,
        &pAdapterDetails
    );
    BAIL_ON_WIN32_ERROR( dwResult );
    ASSERT( pAdapterDetails );


    dwReasonCode = L2_REASON_CODE_IHV_BAD_PROFILE;

    if ( !(pAdapterDetails->pConnectivityProfile && pAdapterDetails->pSecurityProfile) )
    {
        dwResult = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR( dwResult );
    }

    // choose pre-association function based on profile content.
    if
    (
        ( pAdapterDetails->pSecurityProfile->bUseFullSecurity ) ||
        ( pAdapterDetails->pSecurityProfile->bUseIhvConnectivityOnly )
    )
    {
        if ( pAdapterDetails->pSecurityProfile->bUseIhvConnectivityOnly )
        {
            lpfnPreAssociate = DoIhvConnPreAssociate;
        }
        else if ( NULL == pAdapterDetails->pConnectivityProfile->pszParam2 )
        {
            lpfnPreAssociate = DoMissingKeyWepPreAssociate;
        }
        else if ( 0 == pAdapterDetails->pConnectivityProfile->pszParam2[0] )
        {
            lpfnPreAssociate = DoMissingKeyWepPreAssociate;
        }
        else
        {
            lpfnPreAssociate = DoWepPreAssociate;
        }
    }
    else if
    (
        ( !(pAdapterDetails->pSecurityProfile->bUseFullSecurity) )     &&
        ( IHVAuthV1 == pAdapterDetails->pSecurityProfile->AuthType )
    )
    {
            lpfnPreAssociate = Do1xPreAssociate;
    }
    else
    {
        dwResult = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR( dwResult );
    }

    dwReasonCode = L2_REASON_CODE_UNKNOWN;

    LeaveCriticalSection( &g_csSynch );
    bLocked = FALSE;

    ASSERT( lpfnPreAssociate );
    ASSERT( !bLocked );

    dwResult =
    (lpfnPreAssociate)
    (
        pAdapterDetails,
        &dwReasonCode
    );
    BAIL_ON_WIN32_ERROR( dwResult );

error:
    if ( pAdapterDetails )
    {
        DerefenceAdapterDetails( (HANDLE) &(pAdapterDetails->Link) );
    }
    if ( bLocked )
    {
        LeaveCriticalSection( &g_csSynch );
    }
    if ( pAdapterDetails )
    {
        // call the completion function directly in this thread.
        dwStatus =
        (g_pDot11ExtApi->Dot11ExtPreAssociateCompletion)
        (
            pAdapterDetails->hDot11SvcHandle,
            pAdapterDetails->hConnectSession,
            dwReasonCode,
            dwResult
        );
    }
    if ( ERROR_SUCCESS != dwStatus )
    {
        // IHV specific logging can happen here.
    }
    return dwResult;
}



//
// Function to start the preassociation thread.
//
DWORD
WINAPI
IhvPerformPreAssociate
(
   IN    HANDLE                                 hIhvExtAdapter,
   IN    HANDLE                                 hConnectSession,
   IN    PDOT11EXT_IHV_PROFILE_PARAMS           pIhvProfileParams,
   IN    PDOT11EXT_IHV_CONNECTIVITY_PROFILE     pIhvConnProfile,
   IN    PDOT11EXT_IHV_SECURITY_PROFILE         pIhvSecProfile,
   IN    PDOT11_BSS_LIST                        pConnectableBssid,
   OUT   PDWORD                                 pdwReasonCode
)
{
    DWORD               dwResult        =   ERROR_SUCCESS;
    DWORD               dwStatus        =   ERROR_SUCCESS;
    BOOL                bLocked         =   FALSE;
    PADAPTER_DETAILS    pAdapterDetails =   NULL;


    ASSERT ( pIhvProfileParams );
    ASSERT ( pdwReasonCode );

    UNREFERENCED_PARAMETER( pIhvProfileParams );
    UNREFERENCED_PARAMETER( pConnectableBssid );

    (*pdwReasonCode) = L2_REASON_CODE_UNKNOWN;

    EnterCriticalSection( &g_csSynch );
    bLocked = TRUE;

    dwResult =
    ReferenceAdapterDetails
    (
        hIhvExtAdapter,
        &pAdapterDetails
    );
    BAIL_ON_WIN32_ERROR( dwResult );
    ASSERT( pAdapterDetails );

    if ( nic_state_initialized != pAdapterDetails->NicState )
    {
        dwResult = ERROR_INVALID_STATE;
        BAIL_ON_WIN32_ERROR( dwResult );
    }

    // Connection specific parameters should be properly initialized.
    if
    (
        ( pAdapterDetails->pOnexData )                              ||
        ( pAdapterDetails->hConnectSession )                        ||
        ( pAdapterDetails->pPerformPostAssociateCompletionRoutine ) ||
        ( pAdapterDetails->pPerformPostAssociateRoutine )           ||
        ( pAdapterDetails->pStopPostAssociateRoutine )
    )
    {
        dwResult = ERROR_INVALID_STATE;
        BAIL_ON_WIN32_ERROR( dwResult );
    }



    (*pdwReasonCode) = L2_REASON_CODE_IHV_BAD_PROFILE;

    // parse connectivity profile
    dwResult =
    GetIhvConnectivityProfile
    (
        pIhvConnProfile,
        &(pAdapterDetails->pConnectivityProfile)
    );
    BAIL_ON_WIN32_ERROR( dwResult );
    ASSERT( pAdapterDetails->pConnectivityProfile );

    // parse security profile.
    dwResult =
    GetIhvSecurityProfile
    (
        pIhvSecProfile,
        &(pAdapterDetails->pSecurityProfile)
    );
    BAIL_ON_WIN32_ERROR( dwResult );
    ASSERT( pAdapterDetails->pSecurityProfile );

    (*pdwReasonCode) = L2_REASON_CODE_UNKNOWN;

    pAdapterDetails->NicState = nic_state_pre_assoc_started;

    pAdapterDetails->hConnectSession        =   hConnectSession;
    pAdapterDetails->bModifyCurrentProfile  =   FALSE;


    // post a new thread to do the pre-association work.
    dwResult =
    StartNewProtectedThread
    (
        hIhvExtAdapter,
        DoPreAssociate,
        (LPVOID) hIhvExtAdapter
    );
    BAIL_ON_WIN32_ERROR( dwResult );


    (*pdwReasonCode) = L2_REASON_CODE_SUCCESS;


error:
    if ( ERROR_SUCCESS != dwResult )
    {
        dwStatus =
        IhvAdapterReset
        (
            hIhvExtAdapter
        );
        ASSERT( ERROR_SUCCESS == dwStatus );
    }
    if ( pAdapterDetails )
    {
        DerefenceAdapterDetails( hIhvExtAdapter );
    }
    if ( bLocked )
    {
        LeaveCriticalSection( &g_csSynch );
    }
    return dwResult;
}





//
// This function starts post associate routine
// in a separate thread and returns.
//
DWORD
WINAPI
IhvPerformPostAssociate
(
   IN    HANDLE                                       hIhvExtAdapter,
   IN    HANDLE                                       hSecuritySessionID,
   IN    PDOT11_PORT_STATE                            pPortState,
   IN    ULONG                                        uDot11AssocParamsBytes,
   IN    PDOT11_ASSOCIATION_COMPLETION_PARAMETERS     pDot11AssocParams
)
{
    DWORD               dwResult        =   ERROR_SUCCESS;
    BOOL                bLocked         =   FALSE;
    PPOST_ASSOC_DATA    ppostAssocData  =   NULL;
    PADAPTER_DETAILS    pAdapterDetails =   NULL;


    ASSERT ( pDot11AssocParams );

    EnterCriticalSection( &g_csSynch );
    bLocked = TRUE;

    dwResult =
    ReferenceAdapterDetails
    (
        hIhvExtAdapter,
        &pAdapterDetails
    );
    BAIL_ON_WIN32_ERROR( dwResult );
    ASSERT( pAdapterDetails );


    if ( nic_state_pre_assoc_ended != pAdapterDetails->NicState )
    {
        dwResult = ERROR_INVALID_STATE;
        BAIL_ON_WIN32_ERROR( dwResult );
    }
    pAdapterDetails->NicState = nic_state_post_assoc_started;

    // is any post-association work required ??
    if ( pAdapterDetails->pPerformPostAssociateRoutine )
    {
        dwResult =
        ( pAdapterDetails->pPerformPostAssociateRoutine )
        (
            pAdapterDetails,
            hSecuritySessionID,
            pPortState,
            uDot11AssocParamsBytes,
            pDot11AssocParams
        );
        BAIL_ON_WIN32_ERROR( dwResult );
    }

    // The post association completion thread may have been
    // started by the perform post association routine for
    // the current profile. Check if the completion thread
    // needs to be started and start the routine.
    if ( pAdapterDetails->pPerformPostAssociateCompletionRoutine )
    {
        ppostAssocData = (PPOST_ASSOC_DATA) PrivateMemoryAlloc( sizeof( POST_ASSOC_DATA ) );
        if ( !ppostAssocData )
        {
            dwResult = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR( dwResult );
        }

        ppostAssocData->hIhvExtAdapter      =  hIhvExtAdapter;
        ppostAssocData->hDot11SvcHandle     =  pAdapterDetails->hDot11SvcHandle;
        ppostAssocData->hSecuritySessionId  =  hSecuritySessionID;


        dwResult =
        StartNewProtectedThread
        (
            hIhvExtAdapter,
            pAdapterDetails->pPerformPostAssociateCompletionRoutine,
            ppostAssocData
        );
        BAIL_ON_WIN32_ERROR( dwResult );


        ppostAssocData = NULL;
    }



error:
    if ( pAdapterDetails )
    {
        DerefenceAdapterDetails( hIhvExtAdapter );
    }
    PrivateMemoryFree( ppostAssocData );
    if ( bLocked )
    {
        LeaveCriticalSection( &g_csSynch );
    }
    return dwResult;
}



//
// Reset the adapter to initialized state.
//
DWORD
WINAPI
IhvAdapterReset
(
   IN    HANDLE   hIhvExtAdapter
)
{
    DWORD               dwResult        =   ERROR_SUCCESS;
    PADAPTER_DETAILS    pAdapterDetails =   NULL;
    DWORD               dwRefCount      =   0;
    BOOL                bOk             =   FALSE;
    BOOL                bLocked         =   FALSE;


    EnterCriticalSection( &g_csSynch );
    bLocked = TRUE;

    dwResult =
    ReferenceAdapterDetails
    (
        hIhvExtAdapter,
        &pAdapterDetails
    );
    BAIL_ON_WIN32_ERROR( dwResult );
    ASSERT( pAdapterDetails );


    for ( ;; )
    {
        ASSERT( bLocked );

        dwRefCount = pAdapterDetails->dwRefCount;

        if ( 2 == dwRefCount ) // For adapter init and self call.
        {
            break;
        }

        if ( nic_state_pre_assoc_started == pAdapterDetails->NicState )
        {
            pAdapterDetails->NicState = nic_state_pre_assoc_ended;
            bOk = SetEvent( pAdapterDetails->hUIResponse );
            ASSERT( bOk );
        }

        LeaveCriticalSection( &g_csSynch );
        bLocked = FALSE;

        Sleep( 100 );

        EnterCriticalSection( &g_csSynch );
        bLocked = TRUE;
    }

    ASSERT( bLocked );

    // Resetting adapter.

    pAdapterDetails->NicState                                   =   nic_state_initialized;
    pAdapterDetails->hConnectSession                            =   NULL;
    pAdapterDetails->bModifyCurrentProfile                      =   FALSE;
    pAdapterDetails->pPerformPostAssociateCompletionRoutine     =   NULL;
    pAdapterDetails->pPerformPostAssociateRoutine               =   NULL;
    pAdapterDetails->pStopPostAssociateRoutine                  =   NULL;

    // freeing UI response data.
    pAdapterDetails->dwResponseLen                              =   0;

    PrivateMemoryFree( pAdapterDetails->pbResponse );
    pAdapterDetails->pbResponse                                 =   NULL;

    ZeroMemory( &(pAdapterDetails->currentGuidUIRequest), sizeof( GUID ) );

    // freeing profile and onex data
    FreeOnexData( &(pAdapterDetails->pOnexData) );
    FreeIhvConnectivityProfile( &(pAdapterDetails->pConnectivityProfile) );
    FreeIhvSecurityProfile( &(pAdapterDetails->pSecurityProfile) );

    // unblocking UI thread.
    bOk = ResetEvent( pAdapterDetails->hUIResponse );
    if ( !bOk )
    {
        dwResult = GetLastError( );
        BAIL_ON_WIN32_ERROR( dwResult );
    }

error:
    if ( bLocked )
    {
        LeaveCriticalSection( &g_csSynch );
    }
    if ( pAdapterDetails )
    {
        DerefenceAdapterDetails( hIhvExtAdapter );
    }
    return dwResult;
}




//
// Stop post association. Get back
// to the state before post association started.
//
DWORD
WINAPI
IhvStopPostAssociate
(
   IN    HANDLE               hIhvExtAdapter,
   IN    PDOT11_MAC_ADDRESS   pPeer,
   IN    DOT11_ASSOC_STATUS   dot11AssocStatus
)
{
    DWORD               dwResult        =   ERROR_SUCCESS;
    PADAPTER_DETAILS    pAdapterDetails =   NULL;
    DWORD               dwRefCount      =   0;
    BOOL                bLocked         =   FALSE;

    // The dot11AssocStatus parameter can be compared with
    // values DOT11_ASSOC_STATUS_* defined in the public
    // headers.

    EnterCriticalSection( &g_csSynch );
    bLocked = TRUE;

    dwResult =
    ReferenceAdapterDetails
    (
        hIhvExtAdapter,
        &pAdapterDetails
    );
    BAIL_ON_WIN32_ERROR( dwResult );
    ASSERT( pAdapterDetails );

    for ( ;; )
    {
        ASSERT( bLocked );

        dwRefCount = pAdapterDetails->dwRefCount;

        if ( 2 == dwRefCount ) // For adapter init and self call.
        {
            break;
        }

        LeaveCriticalSection( &g_csSynch );
        bLocked = FALSE;

        Sleep( 100 );

        EnterCriticalSection( &g_csSynch );
        bLocked = TRUE;
    }

    ASSERT( bLocked );

    if ( nic_state_initialized == pAdapterDetails->NicState )
    {
        // NO OP
        // To handle the case when stop post associate
        // call comes after adapter reset.
        BAIL( );
    }

    // Call the post association routine - if there is any.
    if ( pAdapterDetails->pStopPostAssociateRoutine )
    {
        dwResult =
        (pAdapterDetails->pStopPostAssociateRoutine)
        (
            pAdapterDetails,
            pPeer,
            dot11AssocStatus
        );
        BAIL_ON_WIN32_ERROR( dwResult );
    }

    pAdapterDetails->NicState = nic_state_pre_assoc_ended;

error:
    if ( bLocked )
    {
        LeaveCriticalSection( &g_csSynch );
    }
    if ( pAdapterDetails )
    {
        DerefenceAdapterDetails( hIhvExtAdapter );
    }
    return dwResult;
}


//
// Process received packets.
//
DWORD
WINAPI
IhvReceivePacket
(
   IN    HANDLE   hIhvExtAdapter,
   IN    DWORD    dwInBufferSize,
   IN    LPVOID   pvInBuffer
)
{
    DWORD                       dwResult            =   ERROR_SUCCESS;
    BOOL                        bLocked             =   FALSE;
    PADAPTER_DETAILS            pAdapterDetails     =   NULL;
    IHV_RECEIVE_PACKET_HANDLER  lpfnReceivePacket   =   NULL;


    EnterCriticalSection( &g_csSynch );
    bLocked = TRUE;


    dwResult =
    ReferenceAdapterDetails
    (
        hIhvExtAdapter,
        &pAdapterDetails
    );
    BAIL_ON_WIN32_ERROR( dwResult );
    ASSERT( pAdapterDetails );


    if ( pAdapterDetails->pOnexData && pAdapterDetails->pOnexData->lpfnReceivePacket )
    {
        lpfnReceivePacket = pAdapterDetails->pOnexData->lpfnReceivePacket;
    }
    else
    {
        dwResult = ERROR_INVALID_STATE;
        BAIL_ON_WIN32_ERROR( dwResult );
    }

    LeaveCriticalSection( &g_csSynch );
    bLocked = FALSE;

    ASSERT( lpfnReceivePacket );

    // Call the receive packet routine.
    dwResult =
    (lpfnReceivePacket)
    (
        pAdapterDetails,
        dwInBufferSize,
        pvInBuffer
    );
    BAIL_ON_WIN32_ERROR( dwResult );


error:
    if ( pAdapterDetails )
    {
        DerefenceAdapterDetails( hIhvExtAdapter );
    }
    if ( bLocked )
    {
        LeaveCriticalSection( &g_csSynch );
    }
    return dwResult;
}




//
// Local copy of possible discovery profiles.
//
DOT11EXT_IHV_DISCOVERY_PROFILE
g_IhvDiscoveryProfiles[] =
{
    // discovery profile 1.
    {
        {
            L"<IhvConnectivity xmlns=\"http://www.someihv.com/nwifi/profile\">"
            L"<IHVConnectivityParam1>0</IHVConnectivityParam1>"
            L"<IHVConnectivityParam2></IHVConnectivityParam2>"
            L"</IhvConnectivity>"
        },

        // Use MS Onex.
        {
            L"<IhvSecurity xmlns=\"http://www.someihv.com/nwifi/profile\">"
            L"<IHVUsesFullSecurity>FALSE</IHVUsesFullSecurity>"
            L"<IHVAuthentication>IHVAuthV1</IHVAuthentication>"
            L"<IHVEncryption>IHVCipher1</IHVEncryption>"
            L"<IHVSecurityParam1>0</IHVSecurityParam1>"
            L"<IHVSecurityParam2></IHVSecurityParam2>"
            L"</IhvSecurity>",

            TRUE
        }
    },

    // discovery profile 2
    {
        // Open Wep
        {
            L"<IhvConnectivity xmlns=\"http://www.someihv.com/nwifi/profile\">"
            L"<IHVConnectivityParam1>0</IHVConnectivityParam1>"
            L"<IHVConnectivityParam2></IHVConnectivityParam2>"
            L"</IhvConnectivity>"
        },

        // Full IHV Security
        {
            L"<IhvSecurity xmlns=\"http://www.someihv.com/nwifi/profile\">"
            L"<IHVUsesFullSecurity>TRUE</IHVUsesFullSecurity>"
            L"<IHVAuthentication>IHVAuthV1</IHVAuthentication>"
            L"<IHVEncryption>IHVCipher1</IHVEncryption>"
            L"<IHVSecurityParam1>0</IHVSecurityParam1>"
            L"<IHVSecurityParam2></IHVSecurityParam2>"
            L"</IhvSecurity>",

            FALSE
        }
    }
};







//
// Create discovery profiles.
//
DWORD
WINAPI
IhvCreateDiscoveryProfiles
(
    IN  HANDLE                                      hIhvExtAdapter,
    IN  BOOL                                        bInsecure,
    IN  PDOT11EXT_IHV_PROFILE_PARAMS                pIhvProfileParams,
    IN  PDOT11_BSS_LIST                             pConnectableBssid,
    OUT PDOT11EXT_IHV_DISCOVERY_PROFILE_LIST        pIhvDiscoveryProfileList,
    OUT PDWORD                                      pdwReasonCode
)
{
    DWORD   dwResult            =   ERROR_SUCCESS;
    DWORD   dwIndex             =   0;
    DWORD   dwIHVNumProfiles    =   ARRAY_LENGTH( g_IhvDiscoveryProfiles );

    ASSERT( pIhvDiscoveryProfileList );
    ASSERT( pdwReasonCode );

    UNREFERENCED_PARAMETER( hIhvExtAdapter );
    UNREFERENCED_PARAMETER( bInsecure );
    UNREFERENCED_PARAMETER( pIhvProfileParams );
    UNREFERENCED_PARAMETER( pConnectableBssid );

    (*pdwReasonCode) = L2_REASON_CODE_IHV_OUTOFMEMORY;

    // allocate buffer for the array.
    dwResult =
    (g_pDot11ExtApi->Dot11ExtAllocateBuffer)
    (
        dwIHVNumProfiles * sizeof( DOT11EXT_IHV_DISCOVERY_PROFILE ),
        (LPVOID*) &(pIhvDiscoveryProfileList->pIhvDiscoveryProfiles)
    );
    BAIL_ON_WIN32_ERROR( dwResult );

    ASSERT( pIhvDiscoveryProfileList->pIhvDiscoveryProfiles );
    ZeroMemory
    (
        pIhvDiscoveryProfileList->pIhvDiscoveryProfiles,
        dwIHVNumProfiles * sizeof( DOT11EXT_IHV_DISCOVERY_PROFILE )
    );



    // prepare each discovery profile.
    for ( dwIndex = 0; dwIndex < dwIHVNumProfiles; dwIndex++ )
    {
        dwResult =
        CopyDiscoveryProfile
        (
            g_IhvDiscoveryProfiles + dwIndex,
            pIhvDiscoveryProfileList->pIhvDiscoveryProfiles + dwIndex
        );
        BAIL_ON_WIN32_ERROR( dwResult );

    }

    pIhvDiscoveryProfileList->dwCount = dwIHVNumProfiles;
    (*pdwReasonCode) = L2_REASON_CODE_SUCCESS;

error:
    if ( ERROR_SUCCESS != dwResult )
    {
        if ( pIhvDiscoveryProfileList->pIhvDiscoveryProfiles )
        {
            for ( dwIndex = 0; dwIndex < dwIHVNumProfiles; dwIndex++ )
            {
                FreeDiscoveryProfile( pIhvDiscoveryProfileList->pIhvDiscoveryProfiles + dwIndex );
            }
            (g_pDot11ExtApi->Dot11ExtFreeBuffer)( pIhvDiscoveryProfileList->pIhvDiscoveryProfiles );
        }
        pIhvDiscoveryProfileList->dwCount = 0;
    }
    return dwResult;
}



//
// Process UI Response function.
//
DWORD
WINAPI
IhvProcessUIResponse
(
   IN    GUID        guidUIRequest,
   IN    DWORD       dwByteCount,
   IN    LPVOID      pvResponseBuffer
)
{
    DWORD                   dwResult        =   ERROR_SUCCESS;
    BOOL                    bLocked         =   FALSE;
    PADAPTER_DETAILS        pAdapterDetails =   NULL;
    HANDLE                  hIhvExtAdapter  =   NULL;

    if ( !( dwByteCount && pvResponseBuffer ) )
    {
        dwResult = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR( dwResult );
    }

    EnterCriticalSection( &g_csSynch );
    bLocked = TRUE;

    // find the adapter from the UI response guid.
    dwResult =
    ReferenceAdapterDetailsByUIRequestGuid
    (
        &guidUIRequest,
        &pAdapterDetails,
        &hIhvExtAdapter
    );
    BAIL_ON_WIN32_ERROR( dwResult );
    ASSERT( pAdapterDetails );

    ZeroMemory( &(pAdapterDetails->currentGuidUIRequest), sizeof( GUID ) );

    // Copy the UI response to the adapter data structure.
    PrivateMemoryFree( pAdapterDetails->pbResponse );

    pAdapterDetails->pbResponse = (BYTE*) PrivateMemoryAlloc( dwByteCount );
    if ( !(pAdapterDetails->pbResponse) )
    {
        dwResult = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwResult);
    }
    CopyMemory( pAdapterDetails->pbResponse, pvResponseBuffer, dwByteCount );

    (pAdapterDetails->dwResponseLen) = dwByteCount;

    // Waiting thread can pick up the response now.
    SetEvent( pAdapterDetails->hUIResponse );

error:
    if ( pAdapterDetails )
    {
        DerefenceAdapterDetails( hIhvExtAdapter );
    }
    if ( bLocked )
    {
        LeaveCriticalSection( &g_csSynch );
    }
    return dwResult;
}




//
// Handler to clear memory after sending packet.
//
DWORD
WINAPI
IhvSendPacketCompletion
(
   IN    HANDLE   hSendCompletion
)
{
    // The sample is not sending any packets.
    // So send packet completion should
    // not be called.
    ASSERTFAILURE();

    // The freeing function used here
    // should be the reverse of the function
    // used to allocate memory before calling
    // Send Packet.
    PrivateMemoryFree( (LPVOID) hSendCompletion );
    return ERROR_SUCCESS;
}




//
// Function to return UI Request on OS query.
//
DWORD
WINAPI
IhvQueryUIRequest
(
   IN    HANDLE                        hIhvExtAdapter,
   IN    DOT11EXT_IHV_CONNECTION_PHASE connectionPhase,
   OUT   PDOT11EXT_IHV_UI_REQUEST*     ppIhvUIRequest
)
{
    // This IHV handler is a post Vista extensibility point.
    // It is currently unused.
    ASSERTFAILURE();

    UNREFERENCED_PARAMETER( hIhvExtAdapter );
    UNREFERENCED_PARAMETER( connectionPhase );
    UNREFERENCED_PARAMETER( ppIhvUIRequest );

    return ERROR_CALL_NOT_IMPLEMENTED;
}




//
// Handler to receive the Onex Result.
//
DWORD
WINAPI
IhvOnexIndicateResult
(
   IN    HANDLE                           hIhvExtAdapter,
   IN    DOT11_MSONEX_RESULT              msOneXResult,
   IN    PDOT11_MSONEX_RESULT_PARAMS      pDot11MsOneXResultParams
)
{
    DWORD               dwResult        =   ERROR_SUCCESS;
    BOOL                bLocked         =   FALSE;
    PADAPTER_DETAILS    pAdapterDetails =   NULL;


    EnterCriticalSection( &g_csSynch );
    bLocked = TRUE;


    dwResult =
    ReferenceAdapterDetails
    (
        hIhvExtAdapter,
        &pAdapterDetails
    );
    BAIL_ON_WIN32_ERROR( dwResult );
    ASSERT( pAdapterDetails );


    // If indication of result is required.
    if ( pAdapterDetails->pOnexData && pAdapterDetails->pOnexData->lpfnIndicateResult )
    {
        dwResult =
        (pAdapterDetails->pOnexData->lpfnIndicateResult)
        (
            pAdapterDetails,
            msOneXResult,
            pDot11MsOneXResultParams
        );
        BAIL_ON_WIN32_ERROR( dwResult );

    }
    else
    {
        dwResult = ERROR_INVALID_STATE;
        BAIL_ON_WIN32_ERROR( dwResult );
    }

error:
    if ( pAdapterDetails )
    {
        DerefenceAdapterDetails( hIhvExtAdapter );
    }
    if ( bLocked )
    {
        LeaveCriticalSection( &g_csSynch );
    }
    return dwResult;
}





//
// Handler to receive Control.
//
DWORD
WINAPI
IhvControl
(
   IN    HANDLE    hIhvExtAdapter,
   IN    DWORD     dwInBufferSize,
   IN    PBYTE     pInBuffer,
   IN    DWORD     dwOutBufferSize,
   OUT   PBYTE     pOutBuffer,
   OUT   PDWORD    pdwBytesReturned
)
{
    // Sample does not demonstrate use of IHV control.

    UNREFERENCED_PARAMETER( hIhvExtAdapter );
    UNREFERENCED_PARAMETER( dwInBufferSize );
    UNREFERENCED_PARAMETER( pInBuffer );
    UNREFERENCED_PARAMETER( dwOutBufferSize );
    UNREFERENCED_PARAMETER( pOutBuffer );
    UNREFERENCED_PARAMETER( pdwBytesReturned );

    return ERROR_SUCCESS;
}


