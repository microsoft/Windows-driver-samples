/*++

Copyright (c) 2005 Microsoft Corporation

Abstract:

   Sample IHV Extensibility DLL to extend
   802.11 LWF driver for third party protocols.


--*/

#include "precomp.h"

//
// Service specific global Variables
//
PDOT11EXT_APIS             g_pDot11ExtApi          =  NULL;

CRITICAL_SECTION           g_csSynch               =  {0};
DWORD                      g_dwThreadCount         =  0;
DWORD                      g_dwSessionID           =  0;
BOOL                       g_bAllowInit            =  TRUE;




//
// Structure to store data
// required to start a new
// thread. The thread count
// needs to be protected.
//
typedef
struct _THREAD_PROTECTOR
{
   LPTHREAD_START_ROUTINE   pStartRoutine;
   LPVOID                   pvParams;
   HANDLE                   hIhvExtAdapter;
}
THREAD_PROTECTOR, *PTHREAD_PROTECTOR;



//
// Logical copy function macro.
//
#define COPY_FUNCTION( _p, _FuncName, _Preface )   \
   (_p)->Dot11ExtIhv##_FuncName =                  \
   _Preface##_FuncName;                            \



//
// Trace utility function
//
VOID
SampleTraceFn
(
    LPCSTR      pszFormat,
    LPCSTR      pszVal1,
    DWORD       dwVal1
)
{
    CHAR    szMsgString[ 512 ]  =   {0};
    HRESULT hr                  =   S_OK;

    hr =
    StringCchPrintfA
    (
        szMsgString,
        sizeof( szMsgString ) - 1,
        pszFormat,
        pszVal1,
        dwVal1
    );

    if ( S_OK == hr )
    {
        OutputDebugStringA( szMsgString );
    }
    else
    {
        OutputDebugStringA( "ERROR: Trace message generation failed.\n" );
    }

    return;
}

// Private Memory alloc function
LPVOID
PrivateMemoryAlloc
(
    size_t  MemSize
)
{
    LPVOID  pvBuffer    =   NULL;

    if (!MemSize)
    {
        BAIL( );
    }

    pvBuffer = malloc( MemSize );
    if ( pvBuffer )
    {
        ZeroMemory( pvBuffer, MemSize );
    }

error:
    return pvBuffer;
}

// Private Memory free function
VOID
PrivateMemoryFree
(
    LPVOID  pvBuffer
)
{
    if ( pvBuffer )
    {
        free( pvBuffer );
    }
}



//
// Initialize the handler functions
//
VOID
HandlerInit
(
   OUT   PDOT11EXT_IHV_HANDLERS     pDot11IHVHandlers
)
{

   COPY_FUNCTION( pDot11IHVHandlers,  DeinitService           , Ihv  );
   COPY_FUNCTION( pDot11IHVHandlers,  InitAdapter             , Ihv  );
   COPY_FUNCTION( pDot11IHVHandlers,  DeinitAdapter           , Ihv  );
   COPY_FUNCTION( pDot11IHVHandlers,  ProcessSessionChange    , Ihv  );
   COPY_FUNCTION( pDot11IHVHandlers,  IsUIRequestPending      , Ihv  );
   COPY_FUNCTION( pDot11IHVHandlers,  ReceiveIndication       , Ihv  );
   COPY_FUNCTION( pDot11IHVHandlers,  PerformCapabilityMatch  , Ihv  );
   COPY_FUNCTION( pDot11IHVHandlers,  ValidateProfile         , Ihv  );
   COPY_FUNCTION( pDot11IHVHandlers,  PerformPreAssociate     , Ihv  );
   COPY_FUNCTION( pDot11IHVHandlers,  PerformPostAssociate    , Ihv  );
   COPY_FUNCTION( pDot11IHVHandlers,  AdapterReset            , Ihv  );
   COPY_FUNCTION( pDot11IHVHandlers,  StopPostAssociate       , Ihv  );
   COPY_FUNCTION( pDot11IHVHandlers,  ReceivePacket           , Ihv  );
   COPY_FUNCTION( pDot11IHVHandlers,  CreateDiscoveryProfiles , Ihv  );
   COPY_FUNCTION( pDot11IHVHandlers,  ProcessUIResponse       , Ihv  );
   COPY_FUNCTION( pDot11IHVHandlers,  SendPacketCompletion    , Ihv  );
   COPY_FUNCTION( pDot11IHVHandlers,  QueryUIRequest          , Ihv  );
   COPY_FUNCTION( pDot11IHVHandlers,  OnexIndicateResult      , Ihv  );
   COPY_FUNCTION( pDot11IHVHandlers,  Control                 , Ihv  );

}

#define ASSERT_MSG_LEN  768

//
// Calling DebugBreak indirectly
// to facilitate frame stepping
// in a debugger.
//
VOID
AssertFunc
(
    _In_    LPCSTR  pszFile,
            int     nLine
)
{
    HRESULT     hr                          =   S_OK;
    CHAR        Message[ ASSERT_MSG_LEN ]   =   {0};

    hr =
    StringCchPrintfA
    (
        Message,
        ASSERT_MSG_LEN-1,
        "\n\nAssertion failed in File %s, Line %d\n\n",
        pszFile,
        nLine
    );
    if ( S_OK == hr )
    {
        OutputDebugStringA( Message );
    }
    else
    {
        OutputDebugStringA( "Assertion Failed\n" );
    }

    DebugBreak( );
}

//
// Initialize global synchronization structure.
//
DWORD
InitCritSect
(
   CRITICAL_SECTION* pCritSect
)
{
   DWORD    dwResult =  ERROR_SUCCESS;

   __try
   {
      InitializeCriticalSection( pCritSect );
   }
   __except (EXCEPTION_EXECUTE_HANDLER)
   {
      dwResult = GetExceptionCode( );
      BAIL_ON_WIN32_ERROR( dwResult );
   }

error:
    return dwResult;
}





//
// Dll Main function.
//
BOOL
WINAPI
DllMain
(
   IN HINSTANCE   Dll,
   IN DWORD       Reason,
   IN PVOID       Reserved
)
{
    DWORD   dwResult =  ERROR_SUCCESS;

    UNREFERENCED_PARAMETER( Dll );
    UNREFERENCED_PARAMETER( Reserved );

    switch (Reason)
    {
        case DLL_PROCESS_ATTACH:

            dwResult = InitCritSect( &g_csSynch );
            BAIL_ON_WIN32_ERROR( dwResult );

            g_dwThreadCount =  0;
            g_dwSessionID   =  0;
            g_bAllowInit    =  TRUE;

            break;

        case DLL_PROCESS_DETACH:
            DeleteCriticalSection( &g_csSynch );
            break;

        default:
            break;
    }

error:
    return ( ERROR_SUCCESS == dwResult );
}




//
// Disable starting new threads or adding new adapters..
//
VOID
StartShutdown
(
   VOID
)
{
   g_bAllowInit = FALSE;
}



//
// Wait for spawned threadcount
// to come down to zero.
//
VOID
WaitOnZeroThreads
(
   VOID
)
{
   BOOL  bZeroThreads   =  FALSE;

   for ( ;; )
   {
      EnterCriticalSection( &g_csSynch );
      bZeroThreads = ( 0 == g_dwThreadCount );
      LeaveCriticalSection( &g_csSynch );

      if ( bZeroThreads )
      {
          break;
      }

      Sleep( 100 );
   }
}






//
// Function uses pThreadProtector structure
// to start a new thread. Decrements the
// global thread count after this thread has
// returned.
//
DWORD
WINAPI
ProtectedThreadEntry
(
   LPVOID pvThreadProtector
)
{
    DWORD                       dwResult            =   ERROR_SUCCESS;
    PTHREAD_PROTECTOR           pThreadProtector    =   NULL;
    LPTHREAD_START_ROUTINE      pStartRoutine       =   NULL;
    LPVOID                      pvParams            =   NULL;
    HANDLE                      hIhvExtAdapter      =   NULL;

    pThreadProtector  = (PTHREAD_PROTECTOR) pvThreadProtector;
    ASSERT ( pThreadProtector )

    pStartRoutine  =  pThreadProtector->pStartRoutine;
    pvParams       =  pThreadProtector->pvParams;
    hIhvExtAdapter =  pThreadProtector->hIhvExtAdapter;

    PrivateMemoryFree( pThreadProtector );
    pThreadProtector = NULL;


    if ( pStartRoutine )
    {
        dwResult = pStartRoutine( pvParams );
        BAIL_ON_WIN32_ERROR( dwResult );
    }

error:
    EnterCriticalSection( &g_csSynch );

    // decrement thread count.
    g_dwThreadCount--;

    // decrement adapter reference.
    DerefenceAdapterDetails( hIhvExtAdapter );

    LeaveCriticalSection( &g_csSynch );

    return dwResult;
}


//
// Start a new thread.
//
DWORD
StartNewProtectedThread
(
    HANDLE                  hIhvExtAdapter,
    LPTHREAD_START_ROUTINE  pStartRoutine,
    LPVOID                  pvParams
)
{
    DWORD               dwResult            =   ERROR_SUCCESS;
    HANDLE              hThread             =   NULL;
    PTHREAD_PROTECTOR   pThreadProtector    =   NULL;
    BOOL                bLocked             =   FALSE;
    PADAPTER_DETAILS    pAdapterDetails     =   NULL;
    BOOL                bCloseHandle        =   FALSE;
    BOOL                bOk                 =   TRUE;


    EnterCriticalSection( &g_csSynch );
    bLocked = TRUE;

    if (( !g_bAllowInit ) || (g_dwThreadCount >= MAX_THREAD_COUNT) )
    {
        dwResult = ERROR_INVALID_STATE;
        BAIL_ON_WIN32_ERROR( dwResult );
    }

    dwResult =
    ReferenceAdapterDetails
    (
        hIhvExtAdapter,
        &pAdapterDetails
    );
    BAIL_ON_WIN32_ERROR( dwResult );
    ASSERT( pAdapterDetails );


    // This memory is initialized, used and freed entirely by the Extensibility
    // DLL. Hence this memory can be allocated and freed using any method.
    pThreadProtector = (PTHREAD_PROTECTOR) PrivateMemoryAlloc( sizeof( THREAD_PROTECTOR ) );
    if ( !pThreadProtector )
    {
        dwResult = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR( dwResult );
    }

    pThreadProtector->pStartRoutine  =  pStartRoutine;
    pThreadProtector->pvParams       =  pvParams;
    pThreadProtector->hIhvExtAdapter =  hIhvExtAdapter;

    hThread =
    CreateThread
    (
        NULL,                   // security attributes
        0,                      // default stack size
        ProtectedThreadEntry,   // pointer to function to run
        pThreadProtector,       // parameter
        0,                      // run thread immediately
        NULL                    // Thread ID receiver
    );
    if ( !hThread )
    {
        dwResult = GetLastError( );
        BAIL_ON_WIN32_ERROR( dwResult );
    }
    bCloseHandle = TRUE;

    // Thread successfully started, memory
    // would be freed by ProtectedThreadEntry
    pThreadProtector = NULL;

    // Transfering dereference duty to ProtectedThreadEntry function.
    pAdapterDetails = NULL;

    g_dwThreadCount++;

error:
    if ( bCloseHandle )
    {
        bOk = CloseHandle( hThread );
        ASSERT( bOk );
    }

    PrivateMemoryFree( pThreadProtector );

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
)
{
    UNREFERENCED_PARAMETER( pIhvProfileParams );
    UNREFERENCED_PARAMETER( pConnectivityProfile );
    UNREFERENCED_PARAMETER( pSecurityProfile );
    UNREFERENCED_PARAMETER( pBssEntry );

    // Try to match the current profile with the beacon.
    return TRUE;
}



DWORD
CopyConnectivityProfile
(
    IN      PDOT11EXT_IHV_CONNECTIVITY_PROFILE  pSrc,
    OUT     PDOT11EXT_IHV_CONNECTIVITY_PROFILE  pDst
)
{
    DWORD   dwResult    =   ERROR_SUCCESS;
    DWORD   dwLen       =   0;

    ASSERT( pSrc );
    ASSERT( pDst );

    ZeroMemory( pDst, sizeof( DOT11EXT_IHV_CONNECTIVITY_PROFILE ) );

    if ( pSrc->pszXmlFragmentIhvConnectivity )
    {
        dwLen =  (DWORD) wcslen( pSrc->pszXmlFragmentIhvConnectivity );

        dwResult =  
        (g_pDot11ExtApi->Dot11ExtAllocateBuffer)
        ( 
            (dwLen+1) * sizeof( WCHAR ),
            (LPVOID*) &(pDst->pszXmlFragmentIhvConnectivity)
        );
        BAIL_ON_WIN32_ERROR( dwResult );
        ASSERT( pDst->pszXmlFragmentIhvConnectivity );

        CopyMemory
        (
            (LPVOID) pDst->pszXmlFragmentIhvConnectivity,
            (LPVOID) pSrc->pszXmlFragmentIhvConnectivity,
            (dwLen+1) * sizeof( WCHAR )
        );
    }

error:
    return dwResult;
}


VOID
FreeConnectivityProfile
(
    IN  PDOT11EXT_IHV_CONNECTIVITY_PROFILE  pSrc
)
{
    if ( pSrc && pSrc->pszXmlFragmentIhvConnectivity )
    {
        (g_pDot11ExtApi->Dot11ExtFreeBuffer)( (LPVOID) pSrc->pszXmlFragmentIhvConnectivity );
        pSrc->pszXmlFragmentIhvConnectivity = NULL;
    }
}



DWORD
CopySecurityProfile
(
    IN  PDOT11EXT_IHV_SECURITY_PROFILE  pSrc,
    OUT PDOT11EXT_IHV_SECURITY_PROFILE  pDst
)
{
    DWORD   dwResult    =   ERROR_SUCCESS;
    DWORD   dwLen       =   0;

    ASSERT( pSrc );
    ASSERT( pDst );

    ZeroMemory( pDst, sizeof( DOT11EXT_IHV_SECURITY_PROFILE ) );

    pDst->bUseMSOnex = pSrc->bUseMSOnex;

    if ( pSrc->pszXmlFragmentIhvSecurity )
    {
        dwLen =  (DWORD) wcslen( pSrc->pszXmlFragmentIhvSecurity );

        dwResult =  
        (g_pDot11ExtApi->Dot11ExtAllocateBuffer)
        ( 
            (dwLen+1) * sizeof( WCHAR ),
            (LPVOID*) &(pDst->pszXmlFragmentIhvSecurity)
        );
        BAIL_ON_WIN32_ERROR( dwResult );
        ASSERT( pDst->pszXmlFragmentIhvSecurity );

        CopyMemory
        (
            (LPVOID) pDst->pszXmlFragmentIhvSecurity,
            (LPVOID) pSrc->pszXmlFragmentIhvSecurity,
            (dwLen+1) * sizeof( WCHAR )
        );
    }


error:
    return dwResult;
}


VOID
FreeSecurityProfile
(
    IN  PDOT11EXT_IHV_SECURITY_PROFILE  pSrc
)
{
    if ( pSrc && pSrc->pszXmlFragmentIhvSecurity )
    {
        (g_pDot11ExtApi->Dot11ExtFreeBuffer)( (LPVOID) pSrc->pszXmlFragmentIhvSecurity );
        pSrc->pszXmlFragmentIhvSecurity = NULL;
    }
}




DWORD
CopyDiscoveryProfile
(
    IN  PDOT11EXT_IHV_DISCOVERY_PROFILE     pSrc,
    OUT PDOT11EXT_IHV_DISCOVERY_PROFILE     pDst
)
{
    DWORD   dwResult    =   ERROR_SUCCESS;

    ASSERT( pSrc );
    ASSERT( pDst );

    dwResult =
    CopyConnectivityProfile
    (
        &(pSrc->IhvConnectivityProfile),
        &(pDst->IhvConnectivityProfile)
    );
    BAIL_ON_WIN32_ERROR( dwResult );


    dwResult =
    CopySecurityProfile
    (
        &(pSrc->IhvSecurityProfile),
        &(pDst->IhvSecurityProfile)
    );
    BAIL_ON_WIN32_ERROR( dwResult );

error:
    if ( ERROR_SUCCESS != dwResult )
    {
        FreeDiscoveryProfile( pDst );
    }
    return dwResult;
}


VOID
FreeDiscoveryProfile
(
    IN  PDOT11EXT_IHV_DISCOVERY_PROFILE     pSrc
)
{
    if ( pSrc )
    {
        FreeConnectivityProfile( &(pSrc->IhvConnectivityProfile) );
        FreeSecurityProfile( &(pSrc->IhvSecurityProfile) );
    }
}



