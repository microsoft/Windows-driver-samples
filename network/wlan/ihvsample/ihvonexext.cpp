
/*++

Copyright (c) 2005 Microsoft Corporation

Abstract:

   Sample IHV Extensibility DLL to extend
   802.11 LWF driver for third party protocols.


--*/

#include "precomp.h"





#define MAX_BACKLOG         32
#define MAX_EXEMPTIONS      1
#define MAX_REGISTRATIONS   2
#define ETHTYPE_EAPOL       0x888e
#define EapolTypeEapolKey   0x03


DWORD
PlumbWEPKey
(
    HANDLE              hDot11SvcHandle,
    ULONG               uKeyIndex,
    DOT11_DIRECTION     direction,
    LPBYTE              pbKey,
    ULONG               uKeyLen
);


DWORD
ProcessRC4Key
(
    HANDLE                          hIhvExtAdapter,
    PDOT11_MSONEX_RESULT_PARAMS     pOneXResultParams,
    HANDLE                          hDot11SvcHandle,
    HANDLE                          hSecuritySessionID,
    ULONG                           uPktLen,
    PBYTE                           pbEapolPkt
);



// initialize onex data structure.
DWORD
GetNewOnexData
(
    PONEX_DATA* ppOnexData
)
{
    DWORD           dwResult    =   ERROR_SUCCESS;
    PONEX_DATA      pOnexData   =   NULL;

    ASSERT( ppOnexData );

    if (*ppOnexData)
    {
        dwResult = ERROR_INVALID_STATE;
        BAIL_ON_WIN32_ERROR( dwResult );
    }

    // allocate memory.
    pOnexData = (PONEX_DATA) PrivateMemoryAlloc( sizeof( ONEX_DATA ) );
    if (!pOnexData)
    {
        dwResult = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR( dwResult );
    }

    // init fields here.
    pOnexData->lpfnReceivePacket    =   Do1xReceivePacket;
    pOnexData->lpfnIndicateResult   =   Do1xIndicateResult;

    // transfer data to caller.
    (*ppOnexData)   =   pOnexData;
    pOnexData       =   NULL;

error:
    if ( pOnexData )
    {
        FreeOnexData( &pOnexData );
    }
    return dwResult;
}




// free onex data structure.
VOID
FreeOnexData
(
    PONEX_DATA* ppOnexData
)
{
    PONEX_DATA      pOnexData   =   NULL;

    if ( ppOnexData && (*ppOnexData) )
    {
        // Freeing caller's variable.
        pOnexData       =   (*ppOnexData);
        (*ppOnexData)   =   NULL;

        RC4UtilsFreeResultParams( &(pOnexData->pOnexResultParams) );
        ZeroMemory( pOnexData , sizeof( ONEX_DATA ) );
        PrivateMemoryFree( pOnexData );
    }
}


// verify if result params are available and valid.
BOOL
IsOneXResultParamsAvailable
(
    PDOT11_MSONEX_RESULT_PARAMS     pOneXResultParams
)
{
    BOOL    bResult =   FALSE;


    bResult =
    (
        pOneXResultParams                   &&
        pOneXResultParams->pbMPPERecvKey    &&
        pOneXResultParams->dwMPPERecvKeyLen &&
        pOneXResultParams->pbMPPESendKey    &&
        pOneXResultParams->dwMPPESendKeyLen
    );

    TRACE_MESSAGE_VAL( "Onex Result Params Available = ", bResult );

    return bResult;
}


// free rc4 packet.
VOID
FreeRC4Pkt
(
    PCACHED_PKT pPkt
)
{
    if ( pPkt )
    {
        PrivateMemoryFree( pPkt->pbPkt );
        pPkt->pbPkt     =   NULL;
        pPkt->uPktLen   =   0;
    }
}

// flush rc4 packet cache.
VOID
FlushRC4PktCache
(
    PONEX_DATA  pOnexData
)
{
    ULONG i = 0;

    ASSERT( pOnexData );

    for (i = 0; i< RC4_CACHE_SIZE; i++)
    {
        FreeRC4Pkt( &(pOnexData->RC4Cache[i]) );
    }
}


// cache rc4 packet
DWORD
CacheRC4Pkt
(
    PONEX_DATA  pOnexData,
    ULONG       uPktLen,
    PBYTE       pbEapolPkt
)
{
    DWORD   dwResult    =   ERROR_SUCCESS;
    PBYTE   pbPktCopy   =   NULL;

    if ( !(pOnexData && uPktLen && pbEapolPkt) )
    {
        dwResult = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR( dwResult );
    }

    // copy the packet.
    pbPktCopy = (PBYTE) PrivateMemoryAlloc(uPktLen);
    if (!pbPktCopy)
    {
        dwResult = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR( dwResult );
    }

    CopyMemory
    (
        pbPktCopy,
        pbEapolPkt,
        uPktLen
    );

    // free packet destination.
    FreeRC4Pkt( &(pOnexData->RC4Cache[pOnexData->uCacheFreeIdx]) );

    // move packet copy to cache.
    pOnexData->RC4Cache[ pOnexData->uCacheFreeIdx ].pbPkt   =   pbPktCopy;
    pOnexData->RC4Cache[ pOnexData->uCacheFreeIdx ].uPktLen =   uPktLen;


    TRACE_MESSAGE_VAL( "RC4 Packet cached, Index = ", pOnexData->uCacheFreeIdx );

    // increment index.
    pOnexData->uCacheFreeIdx = (pOnexData->uCacheFreeIdx + 1 ) % RC4_CACHE_SIZE;

error:
    return dwResult;
}

// process cached rc4 packets.
DWORD
ProcessCachedRC4Packets
(
    PADAPTER_DETAILS    pAdapterDetails
)
{
    DWORD       dwResult        =   ERROR_SUCCESS;
    ULONG       ulIndex         =   0;
    ULONG       uProcessIndex   =   0;
    PONEX_DATA  pOnexData       =   NULL;

    ASSERT( pAdapterDetails );
    ASSERT( pAdapterDetails->pOnexData );

    pOnexData = pAdapterDetails->pOnexData;

    // If no 1x result available, bail
    if (!IsOneXResultParamsAvailable( pOnexData->pOnexResultParams ))
    {
        dwResult = ERROR_SUCCESS;
        BAIL( );
    }

    TRACE_MESSAGE( "Processing cached RC4 packets." );

    // We process last 2 received frames

    for ( ulIndex = 1; ulIndex <= RC4_CACHE_SIZE; ulIndex++ )
    {
        uProcessIndex = ( pOnexData->uCacheFreeIdx - ulIndex) % RC4_CACHE_SIZE;

        if
        (
            ( pOnexData->RC4Cache[uProcessIndex].pbPkt )     &&
            ( pOnexData->RC4Cache[uProcessIndex].uPktLen )
        )
        {
            dwResult =
            ProcessRC4Key
            (
                (HANDLE) &(pAdapterDetails->Link),
                pOnexData->pOnexResultParams,
                pAdapterDetails->hDot11SvcHandle,
                pOnexData->hSecuritySessionID,
                pOnexData->RC4Cache[uProcessIndex].uPktLen,
                pOnexData->RC4Cache[uProcessIndex].pbPkt
            );
            BAIL_ON_WIN32_ERROR(dwResult);

            FreeRC4Pkt( &(pOnexData->RC4Cache[uProcessIndex]) );

        }
    }

error:
    return dwResult;
}





// pre-associate function for onex profile.
DWORD
WINAPI
Do1xPreAssociate
(
    PADAPTER_DETAILS    pAdapterDetails,
    DWORD*              pdwReasonCode
)
{
    DWORD                       dwResult                            =   ERROR_SUCCESS;
    BOOL                        bLocked                             =   FALSE;
    ULONG                       uNumExemptions                      =   0;
    ULONG                       uNumRegistrations                   =   0;
    DOT11_PRIVACY_EXEMPTION     PrivacyExemption[MAX_EXEMPTIONS]    =   {0};
    USHORT                      usRegistration[MAX_REGISTRATIONS]   =   {0};

    ASSERT( pAdapterDetails );
    ASSERT( pdwReasonCode );

    EnterCriticalSection( &g_csSynch );
    bLocked = TRUE;

    // Reason code is set before making calls that could fail.
    (*pdwReasonCode) = L2_REASON_CODE_IHV_INVALID_STATE;

    if ( nic_state_pre_assoc_started != pAdapterDetails->NicState )
    {
        dwResult = ERROR_INVALID_STATE;
        BAIL_ON_WIN32_ERROR( dwResult );
    }

    // Reason code is set before making calls that could fail.
    (*pdwReasonCode) = L2_REASON_CODE_IHV_OUTOFMEMORY;

    dwResult =
    GetNewOnexData
    (
        &(pAdapterDetails->pOnexData)
    );
    BAIL_ON_WIN32_ERROR(dwResult);


    // Reason code is set before making calls that could fail.
    (*pdwReasonCode) = L2_REASON_CODE_IHV_HARDWARE_FAILURE;


    TRACE_MESSAGE( "Setting Auth Algorithm." );
    dwResult =
    (g_pDot11ExtApi->Dot11ExtSetAuthAlgorithm)
    (
        pAdapterDetails->hDot11SvcHandle,
        DOT11_AUTH_ALGO_80211_OPEN
    );
    BAIL_ON_WIN32_ERROR(dwResult);

    TRACE_MESSAGE( "Setting Unicast cipher algorithm." );
    dwResult =
    (g_pDot11ExtApi->Dot11ExtSetUnicastCipherAlgorithm)
    (
        pAdapterDetails->hDot11SvcHandle,
        DOT11_CIPHER_ALGO_WEP
    );
    BAIL_ON_WIN32_ERROR(dwResult);

    TRACE_MESSAGE( "Setting Multicast cipher algorithm." );
    dwResult =
    (g_pDot11ExtApi->Dot11ExtSetMulticastCipherAlgorithm)
    (
        pAdapterDetails->hDot11SvcHandle,
        DOT11_CIPHER_ALGO_WEP
    );
    BAIL_ON_WIN32_ERROR(dwResult);

    TRACE_MESSAGE( "Setting exclude unencrypted flag." );
    dwResult =
    (g_pDot11ExtApi->Dot11ExtSetExcludeUnencrypted)
    (
        pAdapterDetails->hDot11SvcHandle,
        TRUE
    );
    BAIL_ON_WIN32_ERROR(dwResult);

    // set the exemption handler

    // In vanilla 1x, 802.1x packets are never encrypted
    PrivacyExemption[uNumExemptions].usExemptionActionType  =   DOT11_EXEMPT_ALWAYS;
    PrivacyExemption[uNumExemptions].usEtherType            =   htons(ETHTYPE_EAPOL);
    PrivacyExemption[uNumExemptions].usExemptionPacketType  =   DOT11_EXEMPT_UNICAST;
    uNumExemptions++;
    ASSERT(uNumExemptions <= MAX_EXEMPTIONS);

    usRegistration[uNumRegistrations] = htons(ETHTYPE_EAPOL);
    uNumRegistrations++;
    ASSERT(uNumRegistrations <= MAX_REGISTRATIONS);

    TRACE_MESSAGE( "Setting ethertype handling." );
    dwResult =
    g_pDot11ExtApi->Dot11ExtSetEtherTypeHandling
    (
        pAdapterDetails->hDot11SvcHandle,
        MAX_BACKLOG,
        uNumExemptions,
        PrivacyExemption,
        uNumRegistrations,
        usRegistration
    );
    BAIL_ON_WIN32_ERROR(dwResult);


    // Verified before, just after acquiring lock.
    ASSERT( nic_state_pre_assoc_started == pAdapterDetails->NicState  );

    pAdapterDetails->NicState = nic_state_pre_assoc_ended;

    // Reason code is set to SUCCESS.
    (*pdwReasonCode) = L2_REASON_CODE_SUCCESS;

    // populate post associate handler functions.
    pAdapterDetails->pPerformPostAssociateCompletionRoutine =   NULL;
    pAdapterDetails->pPerformPostAssociateRoutine           =   Do1xPostAssociate;
    pAdapterDetails->pStopPostAssociateRoutine              =   Do1xStopPostAssociate;

error:
    if ( bLocked )
    {
        LeaveCriticalSection( &g_csSynch );
    }
    return dwResult;
}




// post-associate function for onex profile.
DWORD
WINAPI
Do1xPostAssociate
(
    IN  PADAPTER_DETAILS                            pAdapterDetails,
    IN  HANDLE                                      hSecuritySessionID,
    IN  PDOT11_PORT_STATE                           pPortState,
    IN  ULONG                                       uDot11AssocParamsBytes,
    IN  PDOT11_ASSOCIATION_COMPLETION_PARAMETERS    pDot11AssocParams
)
{
    DWORD               dwResult        =   ERROR_SUCCESS;
    BOOL                bLocked         =   FALSE;

    ASSERT( pAdapterDetails );

    UNREFERENCED_PARAMETER( pPortState );
    UNREFERENCED_PARAMETER( uDot11AssocParamsBytes );
    UNREFERENCED_PARAMETER( pDot11AssocParams );


    EnterCriticalSection( &g_csSynch );
    bLocked = TRUE;

    // verify and change state.
    if
    (
        ( nic_state_post_assoc_started != pAdapterDetails->NicState && 
          nic_state_post_assoc_ended != pAdapterDetails->NicState
        )  ||
        ( !(pAdapterDetails->pOnexData) )
    )
    {
        dwResult = ERROR_INVALID_STATE;
        BAIL_ON_WIN32_ERROR( dwResult );
    }
    pAdapterDetails->NicState = nic_state_onex_in_progress;

    // no need to free old id handle.
    pAdapterDetails->pOnexData->hSecuritySessionID = hSecuritySessionID;

    TRACE_MESSAGE( "Starting OneX." );
    dwResult =
    (g_pDot11ExtApi->Dot11ExtStartOneX)
    (
        pAdapterDetails->hDot11SvcHandle,
        NULL // EAP attributes
    );
    BAIL_ON_WIN32_ERROR( dwResult );
    pAdapterDetails->pOnexData->fMSOneXStarted = TRUE;


error:
    if ( bLocked )
    {
        LeaveCriticalSection( &g_csSynch );
    }
    return dwResult;
}





// receive rc4 key.
DWORD
ReceiveRC4Key
(
    PADAPTER_DETAILS    pAdapterDetails,
    ULONG               uPktLen,
    PEAPOL_PACKET       pEapolPkt
)
{
    DWORD   dwResult    =   ERROR_SUCCESS;
    BOOL    bLocked     =   FALSE;

    ASSERT( pAdapterDetails );
    ASSERT( uPktLen );
    ASSERT( pEapolPkt );

    EnterCriticalSection(&g_csSynch);
    bLocked = TRUE;

    ASSERT( pAdapterDetails->pOnexData );

    TRACE_MESSAGE( "Received RC4 Key packet." );

    if ( IsOneXResultParamsAvailable( pAdapterDetails->pOnexData->pOnexResultParams ) )
    {
        // 1x params are available
        dwResult =
        ProcessRC4Key
        (
            (HANDLE) &(pAdapterDetails->Link),
            pAdapterDetails->pOnexData->pOnexResultParams,
            pAdapterDetails->hDot11SvcHandle,
            pAdapterDetails->pOnexData->hSecuritySessionID,
            uPktLen,
            (PBYTE)pEapolPkt
        );
        BAIL_ON_WIN32_ERROR(dwResult);
    }
    else
    {
        dwResult =
        CacheRC4Pkt
        (
            pAdapterDetails->pOnexData,
            uPktLen,
            (PBYTE) pEapolPkt
        );
        BAIL_ON_WIN32_ERROR(dwResult);
    }

error:
    if (bLocked)
    {
        LeaveCriticalSection(&g_csSynch);
    }
    return dwResult;
}




// receive packet function for onex profile.
DWORD
WINAPI
Do1xReceivePacket
(
    PADAPTER_DETAILS    pAdapterDetails,
    DWORD               dwInBufferSize,
    LPVOID              pvInBuffer
)
{
    DWORD                           dwResult        =   ERROR_SUCCESS;
    HANDLE                          hDot11SvcHandle =   NULL;
    BOOL                            bLocked         =   FALSE;
    PDOT11_SECURITY_PACKET_HEADER   pSecurityPkt    =   NULL;
    PEAPOL_PACKET                   pEapolPkt       =   NULL;
    DWORD                           uReqdPktLen     =   0;

    ASSERT( pAdapterDetails );

    // Must include at least one byte of data
    uReqdPktLen = FIELD_OFFSET(DOT11_SECURITY_PACKET_HEADER, Data) + 1;

    if ( dwInBufferSize < uReqdPktLen )
    {
        dwResult = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwResult);
    }

    if ( !pvInBuffer )
    {
        dwResult = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR( dwResult );
    }

    pSecurityPkt    =   (PDOT11_SECURITY_PACKET_HEADER) pvInBuffer;
    pEapolPkt       =   (PEAPOL_PACKET) pSecurityPkt->Data;

    EnterCriticalSection( &g_csSynch );
    bLocked = TRUE;

    // validate state.
    if
    (
        (!(pAdapterDetails->pOnexData))                                 ||
        (
            ( nic_state_onex_in_progress != pAdapterDetails->NicState  )    &&
            ( nic_state_post_assoc_ended != pAdapterDetails->NicState  )
        )
    )
    {
        dwResult = ERROR_INVALID_STATE;
        BAIL_ON_WIN32_ERROR( dwResult );
    }

    hDot11SvcHandle = pAdapterDetails->hDot11SvcHandle;

    TRACE_MESSAGE( "Received security packet." );

    // Ignoring version
    if ( EapolTypeEapolKey == pEapolPkt->PacketType )
    {
        dwResult =
        ReceiveRC4Key
        (
            pAdapterDetails,
            dwInBufferSize - FIELD_OFFSET(DOT11_SECURITY_PACKET_HEADER, Data),
            pEapolPkt
        );
        BAIL_ON_WIN32_ERROR(dwResult);
    }
    else
    {
        // leave lock before sending security packet to AC.
        LeaveCriticalSection( &g_csSynch );
        bLocked = FALSE;

        dwResult =
        (g_pDot11ExtApi->Dot11ExtProcessSecurityPacket)
        (
            hDot11SvcHandle,
            dwInBufferSize - FIELD_OFFSET(DOT11_SECURITY_PACKET_HEADER, Data),
            pSecurityPkt->Data
        );
        BAIL_ON_WIN32_ERROR(dwResult);
    }


error:
    if ( bLocked )
    {
        LeaveCriticalSection( &g_csSynch );
    }
    return dwResult;
}


// parameters for calling AC with post assoc completion.
typedef
struct _POST_ASSOC_COMPL_DATA
{
    HANDLE              hIhvExtAdapter;
    HANDLE              hDot11SvcHandle;
    HANDLE              hSecuritySessionID;
    DOT11_MAC_ADDRESS   PeerMacAddress;
    DWORD               dwSecurityReasonCode;
    DWORD               dwSecurityWin32Error;
}
POST_ASSOC_COMPL_DATA, *PPOST_ASSOC_COMPL_DATA;


// function to make the actual post assoc completion call.
// this function is started in a separate thread.
DWORD
WINAPI
PostAssocComplWorker
(
    LPVOID  pvCtxt
)
{
    DWORD                   dwResult        =   ERROR_SUCCESS;
    BOOL                    bLocked         =   FALSE;
    PPOST_ASSOC_COMPL_DATA  pCtxt           =   (PPOST_ASSOC_COMPL_DATA) pvCtxt;
    PADAPTER_DETAILS        pAdapterDetails =   NULL;

    ASSERT( pCtxt );

    EnterCriticalSection( &g_csSynch );
    bLocked = TRUE;

    dwResult =
    ReferenceAdapterDetails
    (
        pCtxt->hIhvExtAdapter,
        &pAdapterDetails
    );
    BAIL_ON_WIN32_ERROR( dwResult );
    ASSERT( pAdapterDetails );

    if ( nic_state_onex_in_progress != pAdapterDetails->NicState &&
         nic_state_post_assoc_ended != pAdapterDetails->NicState )
    {
        dwResult = ERROR_INVALID_STATE;
        BAIL_ON_WIN32_ERROR( dwResult );
    }

    pAdapterDetails->NicState = nic_state_post_assoc_ended;

    LeaveCriticalSection( &g_csSynch );
    bLocked = FALSE;

    TRACE_MESSAGE_VAL( "Calling PostAssocCompletion, Status = ", pCtxt->dwSecurityWin32Error );

    dwResult =
    (g_pDot11ExtApi->Dot11ExtPostAssociateCompletion)
    (
        pCtxt->hDot11SvcHandle,
        pCtxt->hSecuritySessionID,
        &(pCtxt->PeerMacAddress),
        pCtxt->dwSecurityReasonCode,
        pCtxt->dwSecurityWin32Error
    );
    BAIL_ON_WIN32_ERROR( dwResult );

error:
    if ( pAdapterDetails )
    {
        DerefenceAdapterDetails( pCtxt->hIhvExtAdapter );
    }
    if ( bLocked )
    {
        LeaveCriticalSection( &g_csSynch );
    }
    PrivateMemoryFree( pvCtxt );
    return dwResult;
}


// starts a worker thread for the post association completion.
DWORD
Do1xPostAssocCompletion
(
    HANDLE              hIhvExtAdapter,
    HANDLE              hDot11SvcHandle,
    HANDLE              hSecuritySessionID,
    PDOT11_MAC_ADDRESS  pPeer,
    DWORD               dwSecurityReasonCode,
    DWORD               dwSecurityWin32Error
)
{
    DWORD                   dwResult    =   ERROR_SUCCESS;
    PPOST_ASSOC_COMPL_DATA  pCtxt       =   NULL;

    // allocate memory and copy parameters.
    pCtxt = (PPOST_ASSOC_COMPL_DATA) PrivateMemoryAlloc(sizeof(POST_ASSOC_COMPL_DATA));
    if (!pCtxt)
    {
        dwResult = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwResult);
    }

    pCtxt->hIhvExtAdapter       =   hIhvExtAdapter;
    pCtxt->hDot11SvcHandle      =   hDot11SvcHandle;
    pCtxt->hSecuritySessionID   =   hSecuritySessionID;

    if (pPeer)
    {
        CopyMemory
        (
            &(pCtxt->PeerMacAddress),
            pPeer,
            sizeof(DOT11_MAC_ADDRESS)
        );
    }

    pCtxt->dwSecurityReasonCode =   dwSecurityReasonCode;
    pCtxt->dwSecurityWin32Error =   dwSecurityWin32Error;

    // start the new thread.
    dwResult =
    StartNewProtectedThread
    (
        hIhvExtAdapter,
        PostAssocComplWorker,
        pCtxt
    );
    BAIL_ON_WIN32_ERROR(dwResult);

    pCtxt = NULL;

error:
    PrivateMemoryFree( pCtxt );
    return dwResult;
}


// indicate result function for onex profile.
DWORD
WINAPI
Do1xIndicateResult
(
    PADAPTER_DETAILS                pAdapterDetails,
    DOT11_MSONEX_RESULT             msOneXResult,
    PDOT11_MSONEX_RESULT_PARAMS     pDot11MsOneXResultParams
)
{
    DWORD   dwResult    =   ERROR_SUCCESS;
    BOOL    bLocked     =   FALSE;


    EnterCriticalSection( &g_csSynch );
    bLocked = TRUE;

    ASSERT( pAdapterDetails );
    ASSERT( pAdapterDetails->pOnexData );

    // validate state.
    if ( nic_state_onex_in_progress != pAdapterDetails->NicState &&
         nic_state_post_assoc_ended != pAdapterDetails->NicState
       )
    {
        dwResult = ERROR_INVALID_STATE;
        BAIL_ON_WIN32_ERROR( dwResult );
    }

    if ( msOneXResult == DOT11_MSONEX_IN_PROGRESS )
    {
        // free the onex params if they exist
        RC4UtilsFreeResultParams( &( pAdapterDetails->pOnexData->pOnexResultParams) );
        FlushRC4PktCache( pAdapterDetails->pOnexData );
        TRACE_MESSAGE( "Received DOT11_MSONEX_IN_PROGRESS." );
    }
    else if ( msOneXResult == DOT11_MSONEX_FAILURE )
    {
        TRACE_MESSAGE( "Received DOT11_MSONEX_FAILURE." );

        // If failure, indicate right away
        dwResult =
        Do1xPostAssocCompletion
        (
            (HANDLE) &(pAdapterDetails->Link),
            pAdapterDetails->hDot11SvcHandle,
            pAdapterDetails->pOnexData->hSecuritySessionID,
            NULL,
            (pDot11MsOneXResultParams && 
             pDot11MsOneXResultParams->Dot11OneXReasonCode != ONEX_REASON_CODE_SUCCESS)?
                 pDot11MsOneXResultParams->Dot11OneXReasonCode:
                 L2_REASON_CODE_IHV_ONEX_FAILURE,
            msOneXResult
        );
        BAIL_ON_WIN32_ERROR(dwResult);
    }
    else if ( msOneXResult == DOT11_MSONEX_SUCCESS )
    {
        // success case.
        TRACE_MESSAGE( "Received DOT11_MSONEX_SUCCESS." );

        // free result params and get new result params.
        RC4UtilsFreeResultParams( &(pAdapterDetails->pOnexData->pOnexResultParams) );

        dwResult =
        RC4UtilsDecryptResultParams
        (
            pDot11MsOneXResultParams,
            &(pAdapterDetails->pOnexData->pOnexResultParams)
        );
        BAIL_ON_WIN32_ERROR(dwResult);
    }

    // Process cached RC4 packets if any
    dwResult =
    ProcessCachedRC4Packets
    (
        pAdapterDetails
    );
    BAIL_ON_WIN32_ERROR(dwResult);

error:
    if ( bLocked )
    {
        LeaveCriticalSection( &g_csSynch );
    }
    return dwResult;
}


// stop-post-associate function for onex profile.
DWORD
WINAPI
Do1xStopPostAssociate
(
    PADAPTER_DETAILS    pAdapterDetails,
    PDOT11_MAC_ADDRESS  pPeer,
    DOT11_ASSOC_STATUS  dot11AssocStatus
)
{
    DWORD   dwResult    =   ERROR_SUCCESS;
    BOOL    bLocked     =   FALSE;

    UNREFERENCED_PARAMETER( pPeer );
    UNREFERENCED_PARAMETER( dot11AssocStatus );

    EnterCriticalSection( &g_csSynch );
    bLocked = TRUE;

    TRACE_MESSAGE( "Performing StopPostAssociate." );

    if
    (
        ( nic_state_post_assoc_ended    !=  pAdapterDetails->NicState )     &&
        ( nic_state_onex_in_progress    !=  pAdapterDetails->NicState )     &&
        ( nic_state_post_assoc_started  !=  pAdapterDetails->NicState )
    )
    {
        dwResult = ERROR_INVALID_STATE;
        BAIL_ON_WIN32_ERROR( dwResult );
    }

    // Reset the port specific variables.

    if ( pAdapterDetails->pOnexData )
    {
        if(pAdapterDetails->pOnexData->fMSOneXStarted)
        {
            // NOTE THAT THE OS WILL STOP THE AUTH EVEN IF THIS CALL IS NOT MADE. 
            // The sample should keep track of port downs and call start auth 
            // once a port up is received.
            dwResult = 
            (g_pDot11ExtApi->Dot11ExtStopOneX)
            (
                pAdapterDetails->hDot11SvcHandle
            );
            if(dwResult != ERROR_SUCCESS)
            {
                // log error
                dwResult = ERROR_SUCCESS;
            }
            else
            {
                pAdapterDetails->pOnexData->fMSOneXStarted = FALSE;
            }
        }

        // flush the rc4 packet cache.
        FlushRC4PktCache( pAdapterDetails->pOnexData );

        // free the result params.
        RC4UtilsFreeResultParams( &(pAdapterDetails->pOnexData->pOnexResultParams) );
    }


error:
    if ( bLocked )
    {
        LeaveCriticalSection( &g_csSynch );
    }
    return dwResult;
}






DWORD
PlumbWEPKey
(
    HANDLE              hDot11SvcHandle,
    ULONG               uKeyIndex,
    DOT11_DIRECTION     direction,
    LPBYTE              pbKey,
    ULONG               uKeyLen
)
{
    DWORD                           dwResult    =   ERROR_SUCCESS;
    PDOT11_CIPHER_DEFAULT_KEY_VALUE pDefaultKey =   NULL;
    ULONG                           uAllocLen   =   0;
    BOOL                            bLocked     =   FALSE;

    uAllocLen = FIELD_OFFSET(DOT11_CIPHER_DEFAULT_KEY_VALUE, ucKey) + uKeyLen;

    if (uAllocLen < uKeyLen)
    {
        dwResult = ERROR_ARITHMETIC_OVERFLOW;
        BAIL_ON_WIN32_ERROR(dwResult);
    }

    pDefaultKey = (PDOT11_CIPHER_DEFAULT_KEY_VALUE) PrivateMemoryAlloc(uAllocLen);
    if (!pDefaultKey)
    {
        dwResult = ERROR_NOT_ENOUGH_MEMORY;
        BAIL_ON_WIN32_ERROR(dwResult);
    }

    pDefaultKey->AlgorithmId    =   DOT11_CIPHER_ALGO_WEP;
    pDefaultKey->uKeyIndex      =   uKeyIndex;
    pDefaultKey->bDelete        =   FALSE;
    pDefaultKey->bStatic        =   FALSE;
    pDefaultKey->usKeyLength    =   (USHORT) uKeyLen;

    ZeroMemory(&(pDefaultKey->MacAddr), sizeof(DOT11_MAC_ADDRESS));

    CopyMemory
    (
        pDefaultKey->ucKey,
        pbKey,
        uKeyLen
    );

    EnterCriticalSection( &g_csSynch );
    bLocked = TRUE;

    TRACE_MESSAGE( "Setting default key." );
    TRACE_MESSAGE_VAL( "    Key Index  = ", uKeyIndex );
    TRACE_MESSAGE_VAL( "    Key Length = ", uKeyLen );
    TRACE_MESSAGE_VAL( "    Direction  = ", direction );

    dwResult =
    (g_pDot11ExtApi->Dot11ExtSetDefaultKey)
    (
        hDot11SvcHandle,
        pDefaultKey,
        direction
    );
    BAIL_ON_WIN32_ERROR(dwResult);


error:
    if (pDefaultKey)
    {
        SecureZeroMemory( pDefaultKey, uAllocLen );
        PrivateMemoryFree( pDefaultKey );
    }
    if ( bLocked )
    {
        LeaveCriticalSection( &g_csSynch );
    }
    return dwResult;
}


DWORD
ProcessRC4Key
(
    HANDLE                          hIhvExtAdapter,
    PDOT11_MSONEX_RESULT_PARAMS     pOneXResultParams,
    HANDLE                          hDot11SvcHandle,
    HANDLE                          hSecuritySessionID,
    ULONG                           uPktLen,
    PBYTE                           pbEapolPkt
)
{
    DWORD           dwResult        =   ERROR_SUCCESS;
    PEAPOL_PACKET   pEapolPkt       =   NULL;
    DWORD           dwKeyLen        =   0;
    DWORD           dwKeyIndex      =   0;
    LPBYTE          pbDecryptedKey  =   NULL;
    BOOL            bUCast          =   FALSE;
    BOOL            bLocked         =   FALSE;


    EnterCriticalSection( &g_csSynch );
    bLocked = TRUE;

    ASSERT( IsOneXResultParamsAvailable ( pOneXResultParams ) );

    TRACE_MESSAGE( "Processing RC4 key." );

    pEapolPkt   =   (PEAPOL_PACKET) pbEapolPkt;

    dwResult =
    RC4UtilsParseKeyPacket
    (
        pEapolPkt,
        uPktLen,
        pOneXResultParams,
        &bUCast,
        &pbDecryptedKey,
        &dwKeyLen,
        &dwKeyIndex
    );
    BAIL_ON_WIN32_ERROR( dwResult );

    dwResult =
    PlumbWEPKey
    (
        hDot11SvcHandle,
        dwKeyIndex,
        bUCast ? DOT11_DIR_BOTH : DOT11_DIR_INBOUND,
        pbDecryptedKey,
        dwKeyLen
    );
    BAIL_ON_WIN32_ERROR(dwResult);

    if ( bUCast )
    {
        TRACE_MESSAGE_VAL( "Setting default key ID, Key Index = ", dwKeyIndex );

        dwResult =
        (g_pDot11ExtApi->Dot11ExtSetDefaultKeyId)
        (
            hDot11SvcHandle,
            dwKeyIndex
        );
        BAIL_ON_WIN32_ERROR(dwResult);

        dwResult =
        Do1xPostAssocCompletion
        (
            hIhvExtAdapter,
            hDot11SvcHandle,
            hSecuritySessionID,
            NULL,
            L2_REASON_CODE_SUCCESS,
            ERROR_SUCCESS
        );
        BAIL_ON_WIN32_ERROR(dwResult);
    }

error:
    RC4UtilsFreeKeyMaterial( pbDecryptedKey, dwKeyLen );
    if ( bLocked )
    {
        LeaveCriticalSection( &g_csSynch );
    }
    return dwResult;
}
