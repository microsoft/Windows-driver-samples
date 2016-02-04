



//
// IHV can start defining reason codes in the IHV range.
//
enum
{
   L2_REASON_CODE_IHV_BAD_USER_KEY = L2_REASON_CODE_IHV_BASE,
   L2_REASON_CODE_IHV_OUTOFMEMORY,
   L2_REASON_CODE_IHV_BAD_PROFILE,
   L2_REASON_CODE_IHV_HARDWARE_FAILURE,
   L2_REASON_CODE_IHV_ONEX_FAILURE,
   L2_REASON_CODE_IHV_INVALID_STATE
};






////////////////////////////////////
// IHV provided Handler functions //
////////////////////////////////////



VOID
WINAPI
IhvDeinitService
(
   VOID
);




DWORD
WINAPI
IhvInitAdapter
(
   IN    PDOT11_ADAPTER    pDot11Adapter,
   IN    HANDLE            hDot11SvcHandle,
   OUT   PHANDLE           phIhvExtAdapter
);




VOID
WINAPI
IhvDeinitAdapter
(
   IN    HANDLE   hIhvExtAdapter
);




DWORD
WINAPI
IhvProcessSessionChange
(
   IN    ULONG                         uEventType,
   IN    PWTSSESSION_NOTIFICATION      pSessionNotification
);




DWORD
WINAPI
IhvIsUIRequestPending
(
   IN    GUID     guidUIRequest,
   OUT   PBOOL    pbIsRequestPending
);




DWORD
WINAPI
IhvReceiveIndication
(
    IN  HANDLE                          hIhvExtAdapter,
    IN  DOT11EXT_IHV_INDICATION_TYPE    indicationType,
    IN  ULONG                           uBufferLength,
    IN  LPVOID                          pvBuffer
);




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
);




DWORD
WINAPI
IhvValidateProfile
(
    IN  HANDLE                              hIhvExtAdapter,
    IN  PDOT11EXT_IHV_PROFILE_PARAMS        pIhvProfileParams,
    IN  PDOT11EXT_IHV_CONNECTIVITY_PROFILE  pIhvConnProfile,
    IN  PDOT11EXT_IHV_SECURITY_PROFILE      pIhvSecProfile,
    OUT PDWORD                              pdwReasonCode
);




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
);




DWORD
WINAPI
IhvPerformPostAssociate
(
   IN    HANDLE                                       hIhvExtAdapter,
   IN    HANDLE                                       hSecuritySessionID,
   IN    PDOT11_PORT_STATE                            pPortState,
   IN    ULONG                                        uDot11AssocParamsBytes,
   IN    PDOT11_ASSOCIATION_COMPLETION_PARAMETERS     pDot11AssocParams
);




DWORD
WINAPI
IhvAdapterReset
(
   IN    HANDLE   hIhvExtAdapter
);




DWORD
WINAPI
IhvStopPostAssociate
(
   IN    HANDLE               hIhvExtAdapter,
   IN    PDOT11_MAC_ADDRESS   pPeer,
   IN    DOT11_ASSOC_STATUS   dot11AssocStatus
);





DWORD
WINAPI
IhvReceivePacket
(
   IN    HANDLE   hIhvExtAdapter,
   IN    DWORD    dwInBufferSize,
   IN    LPVOID   pvInBuffer
);




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
);




DWORD
WINAPI
IhvProcessUIResponse
(
   IN    GUID        guidUIRequest,
   IN    DWORD       dwByteCount,
   IN    LPVOID      pvResponseBuffer
);




DWORD
WINAPI
IhvSendPacketCompletion
(
   IN    HANDLE   hSendCompletion
);




DWORD
WINAPI
IhvQueryUIRequest
(
   IN    HANDLE                        hIhvExtAdapter,
   IN    DOT11EXT_IHV_CONNECTION_PHASE connectionPhase,
   OUT   PDOT11EXT_IHV_UI_REQUEST*     ppIhvUIRequest
);




DWORD
WINAPI
IhvOnexIndicateResult
(
   IN    HANDLE                           hIhvExtAdapter,
   IN    DOT11_MSONEX_RESULT              msOneXResult,
   IN    PDOT11_MSONEX_RESULT_PARAMS      pDot11MsOneXResultParams
);




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
);
