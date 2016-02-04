

//////////////////
// Macros       //
//////////////////


// Trace Macros


VOID
SampleTraceFn
(
    LPCSTR      pszFormat,
    LPCSTR      pszVal1,
    DWORD       dwVal1
);

#define TRACE_MESSAGE( _x )             SampleTraceFn( "INFO: %s\n",_x, 0 );
#define TRACE_MESSAGE_VAL( _x, _y )     SampleTraceFn( "INFO: %s %lu\n", _x, _y );


//
// Error Handling
//
#define BAIL_ON_WIN32_ERROR( __x )     \
   if ( ERROR_SUCCESS != (__x) )       \
   {                                   \
      goto error;                      \
   }                                   \


//
// COM failure
//
#define BAIL_ON_FAILURE( __hr )     \
   if ( FAILED( __hr ) )            \
   {                                \
      goto error;                   \
   }                                \

//
// combine win32 and com error codes.
//
#define WIN32_FROM_HRESULT(hr)           \
    (SUCCEEDED(hr) ? ERROR_SUCCESS :     \
        (HRESULT_FACILITY(hr) == FACILITY_WIN32 ? HRESULT_CODE(hr) : (hr)))


// Combined Error Macro
#define WIN32_COMBINED_ERROR( _dwError, _hr ) ( (_dwError)?(_dwError):WIN32_FROM_HRESULT((_hr)))

//
// Unconditional bail
//
#define BAIL( ) goto error;


//
// Maximum number of new threads
// to spawn at any given time.
//
#define MAX_THREAD_COUNT 100



//
// Debug Macro.
//
#ifdef DBG
#define ASSERT(exp)                             \
   if (!(exp))                                  \
   {                                            \
      AssertFunc( __FILE__, __LINE__ );         \
   }

#define ASSERTFAILURE()                         \
   AssertFunc( __FILE__, __LINE__ ); 
#else
#define ASSERT(exp)
#define ASSERTFAILURE()
#endif // DBG



// Private Memory alloc function
LPVOID
PrivateMemoryAlloc
(
    size_t  MemSize
);

// Private Memory free function
VOID
PrivateMemoryFree
(
    LPVOID  pvBuffer
);


//
// Array length.
//
#define ARRAY_LENGTH( _x ) (sizeof( (_x) ) / sizeof( (_x)[0] ))

//
// Declarations for service specific global Variables
//
extern  PDOT11EXT_APIS             g_pDot11ExtApi;
extern  CRITICAL_SECTION           g_csSynch;
extern  DWORD                      g_dwThreadCount;
extern  DWORD                      g_dwSessionID;
extern  BOOL                       g_bAllowInit;








// Pre-declaration for the ADAPTER_DETAILS structure.
typedef
struct _ADAPTER_DETAILS
ADAPTER_DETAILS, *PADAPTER_DETAILS;




//
// Handler type for pre-association.
//
typedef
DWORD
(WINAPI *PRE_ASSOCIATE_FUNCTION)
(
    PADAPTER_DETAILS    pAdapterDetails,
    DWORD*              pdwReasonCode
);


//
// Handler type for post-association.
//
typedef
DWORD
(WINAPI *POST_ASSOCIATE_FUNCTION)
(
    PADAPTER_DETAILS                            pAdapterDetails,
    HANDLE                                      hSecuritySessionID,
    PDOT11_PORT_STATE                           pPortState,
    ULONG                                       uDot11AssocParamsBytes,
    PDOT11_ASSOCIATION_COMPLETION_PARAMETERS    pDot11AssocParams
);



//
// Handler type for stop-post-association.
//
typedef
DWORD
(WINAPI *STOP_POST_ASSOCIATE_FUNCTION)
(
    PADAPTER_DETAILS    pAdapterDetails,
    PDOT11_MAC_ADDRESS  pPeer,
    DOT11_ASSOC_STATUS  dot11AssocStatus
);



//
// Handler type for receive packet.
//
typedef
DWORD
(WINAPI *IHV_RECEIVE_PACKET_HANDLER)
(
    PADAPTER_DETAILS    pAdapterDetails,
    DWORD               dwInBufferSize,
    LPVOID              pvInBuffer
);



//
// Handler type for IHV result indication.
//
typedef
DWORD
(WINAPI *IHV_INDICATE_RESULT_HANDLER)
(
    PADAPTER_DETAILS                pAdapterDetails,
    DOT11_MSONEX_RESULT             msOneXResult,
    PDOT11_MSONEX_RESULT_PARAMS     pDot11MsOneXResultParams
);



// Assert function in debug builds.
VOID
AssertFunc
(
    _In_    LPCSTR  pszFile,
            int     nLine
);


//
// Register intention to start shut down.
//
VOID
StartShutdown
(
   VOID
);


//
// Wait for posted thread count to go down to zero.
//
VOID
WaitOnZeroThreads
(
   VOID
);


//
// Populate IHV handler function pointers to IHV Framework.
//
VOID
HandlerInit
(
   OUT   PDOT11EXT_IHV_HANDLERS     pDot11IHVHandlers
);


//
// Call createthread to start a new thread and ensure
// the adapter stays active till the thread finishes
// by putting a refcount increment/decrement around
// the lifetime of the thread.
//
DWORD
StartNewProtectedThread
(
    HANDLE                  hIhvExtAdapter,
    LPTHREAD_START_ROUTINE  pStartRoutine,
    LPVOID                  pvParams
);


// Typedef for unaligned BSS Entry to process beacons.
typedef UNALIGNED DOT11_BSS_ENTRY* PULDOT11_BSS_ENTRY;

//
// Function to match beacon and profile.
//
BOOL
WINAPI
MatchBssDescription
(
    PDOT11EXT_IHV_PROFILE_PARAMS    pIhvProfileParams,
    PIHV_CONNECTIVITY_PROFILE       pConnectivityProfile,
    PIHV_SECURITY_PROFILE           pSecurityProfile,
    PULDOT11_BSS_ENTRY              pBssEntry
);


//
// Functions to copy and free discovery profiles.
//

DWORD
CopyConnectivityProfile
(
    IN      PDOT11EXT_IHV_CONNECTIVITY_PROFILE  pSrc,
    OUT     PDOT11EXT_IHV_CONNECTIVITY_PROFILE  pDst
);


VOID
FreeConnectivityProfile
(
    IN  PDOT11EXT_IHV_CONNECTIVITY_PROFILE  pSrc
);


DWORD
CopySecurityProfile
(
    IN  PDOT11EXT_IHV_SECURITY_PROFILE  pSrc,
    OUT PDOT11EXT_IHV_SECURITY_PROFILE  pDst
);

VOID
FreeSecurityProfile
(
    IN  PDOT11EXT_IHV_SECURITY_PROFILE  pSrc
);



VOID
FreeDiscoveryProfile
(
    IN  PDOT11EXT_IHV_DISCOVERY_PROFILE     pSrc
);



DWORD
CopyDiscoveryProfile
(
    IN  PDOT11EXT_IHV_DISCOVERY_PROFILE     pSrc,
    OUT PDOT11EXT_IHV_DISCOVERY_PROFILE     pDst
);
