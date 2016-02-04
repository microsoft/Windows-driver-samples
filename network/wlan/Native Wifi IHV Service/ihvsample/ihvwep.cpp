/*++

Copyright (c) 2005 Microsoft Corporation

Abstract:

   Sample IHV Extensibility DLL to extend
   802.11 LWF driver for third party protocols.


--*/

#include "precomp.h"



//
// UI Request structure to
// be parsed by IHV UI DLL.
//
typedef struct _IHV_UI_REQUEST
{
   CHAR  szTitle  [80];
   CHAR  szHelp   [80];
}
IHV_UI_REQUEST, *PIHV_UI_REQUEST;


DWORD
ConvertHexCharToNibble
(
    CHAR    chData,
    BOOL    bUpper,
    PBYTE   pbtData
)
{
    DWORD   dwResult    =   ERROR_SUCCESS;
    BYTE    btNibble    =   0xF0;

    ASSERT( pbtData );

    if ( (chData >= '0') && (chData <= '9') )
    {
        btNibble = (BYTE) (chData - '0');
    }
    else if ( (chData >= 'a') && (chData <= 'f') )
    {
        btNibble = (BYTE) (chData - 'a') + 0xA;
    }
    else if ( (chData >= 'A') && (chData <= 'F') )
    {
        btNibble = (BYTE) (chData - 'A') + 0xA;
    }
    else
    {
        // Wrong type input
        dwResult = ERROR_BAD_FORMAT;
        BAIL_ON_WIN32_ERROR( dwResult );
    }

    ASSERT( btNibble < 0xF0 );

    if ( bUpper )
    {
        (*pbtData) = (*pbtData) & 0x0F;
        (*pbtData) = (*pbtData) | (btNibble << 4);
    }
    else
    {
        (*pbtData) = (*pbtData) & 0xF0;
        (*pbtData) = (*pbtData) | btNibble;
    }

error:
    return dwResult;
}
    

// For a 13 byte key, user can choose to input upto 26 hex digits

#define MAX_KEY_STRING_LENGTH       26

#define MAX_RESPONSE_SIZE           (( MAX_KEY_STRING_LENGTH + 1 ) * sizeof( WCHAR ) )

//
// The UI Response is a UNICODE string since
// the UI module has just passed a BSTR to this
// module. This UNICODE string needs to converted
// to a key. Any IHV specific algorithm can be
// used here.
//

DWORD
ConvertStringToKey
(
    BYTE*   pbKeyData,
    DWORD*  pdwKeyLen
)
{
    DWORD       dwResult                            =   ERROR_SUCCESS;
    HRESULT     hr                                  =   S_OK;
    CHAR        szKey[ MAX_KEY_STRING_LENGTH + 2 ]  =   {0};
    DWORD       dwKeyStringLen                      =   0;
    DWORD       dwIndex                             =   0;


    if
    (
        (!pbKeyData)                        ||
        (!pdwKeyLen)                        ||
        ( 0 == (*pdwKeyLen) )               ||
        ( (*pdwKeyLen) % sizeof( WCHAR ) )
    )
    {
        dwResult = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR( dwResult );
    }

    dwKeyStringLen = (DWORD) wcslen( (LPWSTR) pbKeyData );
    if ( MAX_KEY_STRING_LENGTH < dwKeyStringLen )
    {
        dwResult = ERROR_BAD_FORMAT;
        BAIL_ON_WIN32_ERROR( dwResult );
    }

    // Converting the UNICODE string to
    // ANSI string in scratch pad.
    hr =
    StringCchPrintfA
    (
        szKey,
        MAX_KEY_STRING_LENGTH + 1,
        "%S",
        (WCHAR*) pbKeyData
    );
    BAIL_ON_FAILURE( hr );

    ASSERT( dwKeyStringLen == (DWORD) strlen( szKey ));


    if ( ( 5 == dwKeyStringLen ) || ( 13 == dwKeyStringLen ) )
    {
        // Copying the ANSI string back to original buffer.
        hr =
        StringCchPrintfA
        (
            (CHAR*) pbKeyData,
            (*pdwKeyLen),
            "%s",
            szKey
        );
        BAIL_ON_FAILURE( hr );

        // The strings are direct representations
        // of the Wep Key and can be directly returned.
        (*pdwKeyLen) = dwKeyStringLen;

        TRACE_MESSAGE_VAL( "Received WEP key of length = ", (*pdwKeyLen) );

        BAIL( );
    }

    if (( 10 != dwKeyStringLen ) && ( 26 != dwKeyStringLen ))
    {
        // Wrong length input
        dwResult = ERROR_BAD_FORMAT;
        BAIL_ON_WIN32_ERROR( dwResult );
    }

    for( dwIndex = 0; dwIndex < (dwKeyStringLen / 2); dwIndex++ )
    {
        dwResult =
        ConvertHexCharToNibble
        (
            szKey[ 2 * dwIndex ],
            TRUE,
            &(pbKeyData[ dwIndex ])
        );
        BAIL_ON_WIN32_ERROR( dwResult );

        dwResult =
        ConvertHexCharToNibble
        (
            szKey[ 1 + (2 * dwIndex) ],
            FALSE,
            &(pbKeyData[ dwIndex ])
        );
        BAIL_ON_WIN32_ERROR( dwResult );

    }

    (*pdwKeyLen) = dwKeyStringLen / 2;

    TRACE_MESSAGE_VAL( "Received WEP key of length = ", (*pdwKeyLen) );

error:
    return WIN32_COMBINED_ERROR( dwResult, hr );
}





//
// Defining UI help strings. In a realistic
// implementation these values would be numbers
// to be interpreted the IHV UI DLL that could
// be converted to strings using resource files.
//
#define  UI_TITLE_STRING      "Title"
#define  UI_HELP_STRING       "Help"




// Send the UI request and wait for the UI response.
DWORD
SendUIRequestToReceiveKey
(
    PADAPTER_DETAILS    pAdapterDetails,
    DWORD*              pdwKeyLen,
    BYTE**              ppbKeyData
)
{
    DWORD                           dwResult        =   ERROR_SUCCESS;
    DOT11EXT_IHV_UI_REQUEST         uiRequest       =   {0};
    PIHV_UI_REQUEST                 pIHVRequest     =   NULL;
    CHAR                            szTitle[]       =   UI_TITLE_STRING;
    CHAR                            szHelp[]        =   UI_HELP_STRING;
    BOOL                            bLocked         =   FALSE;
    HANDLE                          hUIResponse     =   NULL;



    // CLSID of COM class that implements the UI page. In a real
    // implementation this GUID could be dynamically obtained.
    CLSID                            uiPageClsid    =
    {
        /* 4A01F9F9-6012-4343-A8C4-10B5DF32672A */
        0x4A01F9F9,
        0x6012,
        0x4343,
        {0xA8, 0xC4, 0x10, 0xB5, 0xDF, 0x32, 0x67, 0x2A}
    };

    ASSERT( pAdapterDetails );

    // prepare the IHV request.
    uiRequest.dwByteCount = sizeof(IHV_UI_REQUEST);
    uiRequest.pvUIRequest = (BYTE*) PrivateMemoryAlloc( sizeof(IHV_UI_REQUEST) );
    if ( !(uiRequest.pvUIRequest) )
    {
        dwResult = ERROR_OUTOFMEMORY;
        BAIL( );
    }

    pIHVRequest = (IHV_UI_REQUEST*)uiRequest.pvUIRequest;


    memcpy(  pIHVRequest->szTitle ,  szTitle  , sizeof(szTitle) );
    memcpy(  pIHVRequest->szHelp  ,  szHelp   , sizeof(szHelp)  );

    uiRequest.dwSessionId = WTSGetActiveConsoleSessionId( );
    uiRequest.UIPageClsid = uiPageClsid;

    // acquire the lock to register the request.
    EnterCriticalSection( &g_csSynch );
    bLocked = TRUE;

    // create new request guid.
    dwResult = UuidCreate( &(uiRequest.guidUIRequest) );
    BAIL_ON_WIN32_ERROR(dwResult);

    // free the existing response.
    PrivateMemoryFree( pAdapterDetails->pbResponse );
    pAdapterDetails->pbResponse = NULL;

    // register the guid.
    pAdapterDetails->currentGuidUIRequest = uiRequest.guidUIRequest;

    // Initializing the event this thread
    // would be waiting on later.
    ResetEvent( pAdapterDetails->hUIResponse );

    hUIResponse = pAdapterDetails->hUIResponse;

    // leave the lock since this thread needs
    // to wait for the response.
    LeaveCriticalSection( &g_csSynch );
    bLocked = FALSE;


    // send the request.
    dwResult =
    (g_pDot11ExtApi->Dot11ExtSendUIRequest)
    (
        pAdapterDetails->hDot11SvcHandle,
        &uiRequest
    );
    BAIL_ON_WIN32_ERROR(dwResult);

    TRACE_MESSAGE( "Sent UI request to receive key." );

    // Waiting for UI response.
    // This would be triggered
    // off if no UI response
    // is received.
    dwResult =
    WaitForSingleObject
    (
        hUIResponse,
        1000 * 60 * 5                    // 5 minutes
    );

    // acquire the lock - required for both success and failure.
    EnterCriticalSection( &g_csSynch );
    bLocked = TRUE;

    ZeroMemory( &(pAdapterDetails->currentGuidUIRequest), sizeof( GUID ) );

    if ( WAIT_OBJECT_0 == dwResult )
    {
        dwResult = ERROR_SUCCESS;
    }
    BAIL_ON_WIN32_ERROR(dwResult);

    if ( NULL == pAdapterDetails->pbResponse )
    {
        dwResult = ERROR_INVALID_STATE;
        BAIL_ON_WIN32_ERROR(dwResult);
    }


    // At this point in the code a response
    // has been received, and the thread
    // has not been aborted.

    (*ppbKeyData) = pAdapterDetails->pbResponse;
    pAdapterDetails->pbResponse = NULL;

    (*pdwKeyLen) = pAdapterDetails->dwResponseLen;


    // Convert the Unicode string to ASCII.
    dwResult =
    ConvertStringToKey
    (
        *ppbKeyData,
        pdwKeyLen
    );
    BAIL_ON_WIN32_ERROR(dwResult);

    pAdapterDetails->bModifyCurrentProfile  =   TRUE;

error:
    if ( bLocked )
    {
        LeaveCriticalSection( &g_csSynch );
    }
    PrivateMemoryFree( uiRequest.pvUIRequest );
    return dwResult;
}


//
// Key index limits.
//
#define MIN_KEY_INDEX      0
#define MAX_KEY_INDEX      3


//
// Perform Wep based pre-association once the key is known.
//
DWORD
WINAPI
DoWepPreAssociateCommon
(
    PADAPTER_DETAILS                    pAdapterDetails,
    DWORD                               dwKeyLen,
    BYTE*                               pbKeyData,
    DWORD*                              pdwReasonCode
)
{
    DWORD                               dwResult        =   ERROR_SUCCESS;
    BOOL                                bLocked         =   FALSE;
    LONG                                lIndex          =   0;
    PDOT11_CIPHER_DEFAULT_KEY_VALUE     pKey            =   0;
    ULONG                               uLen            =   0;

    ASSERT( pAdapterDetails );
    ASSERT( dwKeyLen );
    ASSERT( pbKeyData );
    ASSERT( pdwReasonCode );

    // Reason code is set before making calls that could fail.
    (*pdwReasonCode) = L2_REASON_CODE_IHV_OUTOFMEMORY;

    uLen = FIELD_OFFSET(DOT11_CIPHER_DEFAULT_KEY_VALUE, ucKey) + dwKeyLen * sizeof(UCHAR);
    pKey = (PDOT11_CIPHER_DEFAULT_KEY_VALUE) PrivateMemoryAlloc(uLen);
    if (!pKey)
    {
        dwResult = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwResult);
    }
    CopyMemory( &(pKey->ucKey), pbKeyData, dwKeyLen );

    // Prepare the key.
    pKey->AlgorithmId = DOT11_CIPHER_ALGO_WEP;
    pKey->usKeyLength = (USHORT) dwKeyLen;
    pKey->bStatic = TRUE;
    pKey->Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
    pKey->Header.Revision = DOT11_CIPHER_DEFAULT_KEY_VALUE_REVISION_1;
    pKey->Header.Size = sizeof(DOT11_CIPHER_DEFAULT_KEY_VALUE);

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
    (*pdwReasonCode) = L2_REASON_CODE_IHV_HARDWARE_FAILURE;

    // plumb the settings and keys down.

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

    TRACE_MESSAGE( "Setting exclude unencrypted flag." );
    dwResult =
    (g_pDot11ExtApi->Dot11ExtSetExcludeUnencrypted)
    (
        pAdapterDetails->hDot11SvcHandle,
        TRUE
    );
    BAIL_ON_WIN32_ERROR(dwResult);

    for ( lIndex = MAX_KEY_INDEX; lIndex >= MIN_KEY_INDEX; lIndex-- )
    {
        pKey->uKeyIndex = lIndex;

        TRACE_MESSAGE( "Setting default key." );

        dwResult =
        (g_pDot11ExtApi->Dot11ExtSetDefaultKey)
        (
            pAdapterDetails->hDot11SvcHandle,
            pKey,
            DOT11_DIR_BOTH
        );
        BAIL_ON_WIN32_ERROR(dwResult);
    }

    TRACE_MESSAGE( "Setting default key ID." );
    dwResult =
    (g_pDot11ExtApi->Dot11ExtSetDefaultKeyId)
    (
        pAdapterDetails->hDot11SvcHandle,
        0
    );
    BAIL_ON_WIN32_ERROR(dwResult);


    // Verified before, just after acquiring lock.
    ASSERT( nic_state_pre_assoc_started == pAdapterDetails->NicState  );

    pAdapterDetails->NicState = nic_state_pre_assoc_ended;

    // Reason code is set to SUCCESS.
    (*pdwReasonCode) = L2_REASON_CODE_SUCCESS;

    // register the post-association handlers with the adapter.

    pAdapterDetails->pPerformPostAssociateCompletionRoutine =   DoWepPostAssociate;
    pAdapterDetails->pPerformPostAssociateRoutine           =   NULL;
    pAdapterDetails->pStopPostAssociateRoutine              =   NULL;

error:
    if ( bLocked )
    {
        LeaveCriticalSection( &g_csSynch );
    }
    PrivateMemoryFree( pKey );
    return dwResult;
}


// Pre-association when the profile does not have the key.
DWORD
WINAPI
DoMissingKeyWepPreAssociate
(
    PADAPTER_DETAILS    pAdapterDetails,
    DWORD*              pdwReasonCode
)
{
    DWORD   dwResult        =   ERROR_SUCCESS;
    DWORD   dwKeyLen        =   ERROR_SUCCESS;
    BYTE*   pbKeyData       =   NULL;

    ASSERT( pAdapterDetails );
    ASSERT( pdwReasonCode );

    // Reason code is set before making calls that could fail.
    (*pdwReasonCode) = L2_REASON_CODE_IHV_BAD_USER_KEY;

    // Possible enhancement - try to call Dot11ExtGetProfileCustomUserData
    // function in IHV Framework to see if the key is
    // available there. Else, send UI Request.


    // try to obtain the key through an UI request.
    dwResult =
    SendUIRequestToReceiveKey
    (
        pAdapterDetails,
        &dwKeyLen,
        &pbKeyData
    );
    BAIL_ON_WIN32_ERROR(dwResult);

    ASSERT ( pbKeyData );
    ASSERT ( dwKeyLen );

    // use the key for connection.
    dwResult =
    DoWepPreAssociateCommon
    (
        pAdapterDetails,
        dwKeyLen,
        pbKeyData,
        pdwReasonCode
    );
    BAIL_ON_WIN32_ERROR(dwResult);


    // Possible enhancement - try to call Dot11ExtSetProfileCustomUserData
    // to store the key if the key was obtained by a UI request.

error:
    PrivateMemoryFree( pbKeyData );
    return dwResult;
}







// Pre-association when the profile does have the key.
DWORD
WINAPI
DoWepPreAssociate
(
    PADAPTER_DETAILS    pAdapterDetails,
    DWORD*              pdwReasonCode
)
{
    DWORD   dwResult                    =   ERROR_SUCCESS;
    BOOL    bLocked                     =   FALSE;
    DWORD   dwKeyLen                    =   ERROR_SUCCESS;
    CHAR    szKey[MAX_RESPONSE_SIZE+1]  =   {0};

    ASSERT( pAdapterDetails );
    ASSERT( pdwReasonCode );

    EnterCriticalSection( &g_csSynch );
    bLocked = TRUE;

    // Reason code is set before making calls that could fail.
    (*pdwReasonCode) = L2_REASON_CODE_IHV_BAD_USER_KEY;

    if
    (
        (!( pAdapterDetails->pConnectivityProfile ))                        ||
        (!( pAdapterDetails->pConnectivityProfile->pszParam2 ))             ||
        ( 0 == pAdapterDetails->pConnectivityProfile->pszParam2[0] )
    )
    {
        dwResult = ERROR_INVALID_STATE;
        BAIL_ON_WIN32_ERROR(dwResult);
    }

    // Convert the profile string to a usable key.
    dwKeyLen = (DWORD)wcslen( pAdapterDetails->pConnectivityProfile->pszParam2 );
    dwKeyLen = sizeof(WCHAR) * ( 1 + dwKeyLen );

    if ( dwKeyLen > sizeof( szKey ) )
    {
        dwResult = ERROR_BAD_PROFILE;
        BAIL_ON_WIN32_ERROR(dwResult);
    }


    // Copy string.
    CopyMemory
    (
        (BYTE*) szKey,
        pAdapterDetails->pConnectivityProfile->pszParam2,
        dwKeyLen
    );

    // Convert UNICODE to ASCII.
    dwResult =
    ConvertStringToKey
    (
        (BYTE*) szKey,
        &dwKeyLen
    );
    BAIL_ON_WIN32_ERROR(dwResult);
    ASSERT ( dwKeyLen );


    // Do pre-association with the key in the profile.
    dwResult =
    DoWepPreAssociateCommon
    (
        pAdapterDetails,
        dwKeyLen,
        (BYTE*) szKey,
        pdwReasonCode
    );
    BAIL_ON_WIN32_ERROR(dwResult);

error:
    if ( bLocked )
    {
        LeaveCriticalSection( &g_csSynch );
    }
    return dwResult;
}




extern
DOT11EXT_IHV_DISCOVERY_PROFILE
g_IhvDiscoveryProfiles[];


//
// This function is responsible for the post association
// operations and completing the post association call
// for WEP scenario.
//
DWORD
WINAPI
DoWepPostAssociate
(
   LPVOID pvPostAssociate
)
{
    DWORD               dwResult        =   ERROR_SUCCESS;
    DWORD               dwStatus        =   ERROR_SUCCESS;
    DWORD               dwReasonCode    =   L2_REASON_CODE_IHV_INVALID_STATE;
    BOOL                bLocked         =   FALSE;
    PPOST_ASSOC_DATA    ppostAssocData  =   (PPOST_ASSOC_DATA) pvPostAssociate;
    PADAPTER_DETAILS    pAdapterDetails =   NULL;


    ASSERT( ppostAssocData );

    EnterCriticalSection( &g_csSynch );
    bLocked = TRUE;

    dwResult =
    ReferenceAdapterDetails
    (
        ppostAssocData->hIhvExtAdapter,
        &pAdapterDetails
    );
    BAIL_ON_WIN32_ERROR( dwResult );
    ASSERT( pAdapterDetails );


    if ( nic_state_post_assoc_started != pAdapterDetails->NicState )
    {
        dwResult = ERROR_INVALID_STATE;
        BAIL_ON_WIN32_ERROR( dwResult );
    }

    // This could be an appropriate place to modify the current profile

    if ( pAdapterDetails->bModifyCurrentProfile )
    {
        pAdapterDetails->bModifyCurrentProfile = FALSE;

        dwResult =
        (g_pDot11ExtApi->Dot11ExtSetCurrentProfile)
        (
            pAdapterDetails->hDot11SvcHandle,
            pAdapterDetails->hConnectSession,
            &(g_IhvDiscoveryProfiles[1].IhvConnectivityProfile),
            &(g_IhvDiscoveryProfiles[1].IhvSecurityProfile)
        );
        BAIL_ON_WIN32_ERROR( dwResult );
    }


    // In wep connection case, function only changes the adapter state.
    pAdapterDetails->NicState = nic_state_post_assoc_ended;


    // Reason Code is set to success.
    dwReasonCode = L2_REASON_CODE_SUCCESS;

error:
    if ( pAdapterDetails )
    {
        DerefenceAdapterDetails( ppostAssocData->hIhvExtAdapter );
    }
    if ( bLocked )
    {
        LeaveCriticalSection( &g_csSynch );
    }

    // call completion function.
    dwStatus =
    (g_pDot11ExtApi->Dot11ExtPostAssociateCompletion)
    (
        ppostAssocData->hDot11SvcHandle,
        ppostAssocData->hSecuritySessionId,
        NULL,
        dwReasonCode,
        dwResult
    );
    if ( ERROR_SUCCESS != dwStatus )
    {
        // IHV specific logging can happen here.
    }
    PrivateMemoryFree( ppostAssocData );
    return dwResult;
}


// no op function for preassociation when
// ihv is used only for connectivity. in a
// realistic implementation there would probably
// be calls to Dot11ExtNicSpecificExtension
// in this function to prepare the driver for
// additional connectivity settings.
DWORD
WINAPI
DoIhvConnPreAssociate
(
    PADAPTER_DETAILS    pAdapterDetails,
    DWORD*              pdwReasonCode
)
{
    DWORD   dwResult    =   ERROR_SUCCESS;
    DWORD   dwKeyLen    =   0;
    PBYTE   pbKeyData   =   NULL;
    BOOL    bLocked     =   FALSE;

    ASSERT( pAdapterDetails );
    ASSERT( pdwReasonCode );

    // try to send UI request to obtain some data that could be useful here.
    // the sample does not really use the data.
    dwResult =
    SendUIRequestToReceiveKey
    (
        pAdapterDetails,
        &dwKeyLen,
        &pbKeyData
    );
    BAIL_ON_WIN32_ERROR(dwResult);
    ASSERT ( pbKeyData );

    PrivateMemoryFree( pbKeyData );
    pbKeyData = NULL;

    // try to send UI request to obtain some data that could be useful here.
    // the sample does not really use the data.
    dwResult =
    SendUIRequestToReceiveKey
    (
        pAdapterDetails,
        &dwKeyLen,
        &pbKeyData
    );
    BAIL_ON_WIN32_ERROR(dwResult);
    ASSERT ( pbKeyData );


    EnterCriticalSection( &g_csSynch );
    bLocked = TRUE;

    // Reason code is set before making calls that could fail.
    (*pdwReasonCode) = L2_REASON_CODE_IHV_INVALID_STATE;

    if ( nic_state_pre_assoc_started != pAdapterDetails->NicState )
    {
        dwResult = ERROR_INVALID_STATE;
        BAIL_ON_WIN32_ERROR( dwResult );
    }

    pAdapterDetails->NicState = nic_state_pre_assoc_ended;

    // Reason code is set to SUCCESS.
    (*pdwReasonCode) = L2_REASON_CODE_SUCCESS;

    pAdapterDetails->pPerformPostAssociateCompletionRoutine =   NULL;
    pAdapterDetails->pPerformPostAssociateRoutine           =   NULL;
    pAdapterDetails->pStopPostAssociateRoutine              =   NULL;

error:
    if ( bLocked )
    {
        LeaveCriticalSection( &g_csSynch );
    }
    PrivateMemoryFree( pbKeyData );
    return dwResult;
}
