


// cache size.
#define RC4_CACHE_SIZE 2


// cached packet.
typedef
struct _CACHED_PKT
{
    ULONG   uPktLen;
    PBYTE   pbPkt;
}
CACHED_PKT, *PCACHED_PKT;


// connection specific data for onex profiles.
typedef
struct _ONEX_DATA
{
    HANDLE                          hSecuritySessionID;
    BOOL                            fMSOneXStarted;
    CACHED_PKT                      RC4Cache[RC4_CACHE_SIZE];
    ULONG                           uCacheFreeIdx;
    PDOT11_MSONEX_RESULT_PARAMS     pOnexResultParams;
    IHV_RECEIVE_PACKET_HANDLER      lpfnReceivePacket;
    IHV_INDICATE_RESULT_HANDLER     lpfnIndicateResult;
}
ONEX_DATA, *PONEX_DATA;


// initialize onex data structure.
DWORD
GetNewOnexData
(
    PONEX_DATA* ppOnexData
);

// free onex data structure.
VOID
FreeOnexData
(
    PONEX_DATA* ppOnexData
);




// pre-associate function for onex profile.
DWORD
WINAPI
Do1xPreAssociate
(
    PADAPTER_DETAILS    pAdapterDetails,
    DWORD*              pdwReasonCode
);



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
);


// stop-post-associate function for onex profile.
DWORD
WINAPI
Do1xStopPostAssociate
(
    PADAPTER_DETAILS    pAdapterDetails,
    PDOT11_MAC_ADDRESS  pPeer,
    DOT11_ASSOC_STATUS  dot11AssocStatus
);



// receive packet function for onex profile.
DWORD
WINAPI
Do1xReceivePacket
(
    PADAPTER_DETAILS    pAdapterDetails,
    DWORD               dwInBufferSize,
    LPVOID              pvInBuffer
);



// indicate result function for onex profile.
DWORD
WINAPI
Do1xIndicateResult
(
    PADAPTER_DETAILS                pAdapterDetails,
    DOT11_MSONEX_RESULT             msOneXResult,
    PDOT11_MSONEX_RESULT_PARAMS     pDot11MsOneXResultParams
);


