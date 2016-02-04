/*++

Copyright (c) 2005 Microsoft Corporation

Abstract:

   Sample IHV Extensibility DLL to extend
   802.11 LWF driver for third party protocols.


--*/

#include "precomp.h"

LIST_ENTRY g_AdaptersList = {0};



//
// Copied Macros from wdm.h
//

#define CONTAINING_RECORD(address, type, field) ((type *)( \
                          (PCHAR)(address) - \
                          (ULONG_PTR)(&((type *)0)->field)))


#define InitializeListHead(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = (ListHead))



#define RemoveEntryList(Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_Flink;\
    _EX_Flink = (Entry)->Flink;\
    _EX_Blink = (Entry)->Blink;\
    _EX_Blink->Flink = _EX_Flink;\
    _EX_Flink->Blink = _EX_Blink;\
    }


#define InsertTailList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Blink = _EX_ListHead->Blink;\
    (Entry)->Flink = _EX_ListHead;\
    (Entry)->Blink = _EX_Blink;\
    _EX_Blink->Flink = (Entry);\
    _EX_ListHead->Blink = (Entry);\
    }




//
// Initialize the AdapterList data structure.
//
VOID
InitAdapterDetailsList
(
    VOID
)
{
    EnterCriticalSection( &g_csSynch );

    // Assuming that memory in g_AdaptersList
    // could be garbage - just initialize the
    // fields appropriately.

    InitializeListHead( &g_AdaptersList );

    LeaveCriticalSection( &g_csSynch );

    return;
}




//
// Free the AdapterList data structure - contention is not supported.
//
VOID
DeinitAdapterDetailsList
(
    VOID
)
{
    EnterCriticalSection( &g_csSynch );

    // ASSERT that the list is empty. The call
    // to InitializeListHead is not really required
    // if the following conditions are true.
    ASSERT(g_AdaptersList.Flink == &g_AdaptersList );
    ASSERT(g_AdaptersList.Blink == &g_AdaptersList );

    // clear the memory any way.
    InitializeListHead( &g_AdaptersList );

    LeaveCriticalSection( &g_csSynch );

    return;
}





//
// Add a single adapter to the list. Starting refcount is one.
//
DWORD
InitAdapterDetails
(
   HANDLE            hDot11SvcHandle,
   PHANDLE           phIhvExtAdapter
)
{
    DWORD               dwResult        =   ERROR_SUCCESS;
    BOOL                bLocked         =   FALSE;
    PADAPTER_DETAILS    pAdapterDetails =   NULL;
    HANDLE              hIhvExtAdapter  =   NULL;


    // acquire global lock.
    EnterCriticalSection( &g_csSynch );
    bLocked = TRUE;

    // verify state. this check prevents new adapters from
    // being added when service is being deinited.
    if (!g_bAllowInit)
    {
        dwResult = ERROR_INVALID_STATE;
        BAIL_ON_WIN32_ERROR( dwResult );
    }

    // allocate memory.
    pAdapterDetails = (PADAPTER_DETAILS) PrivateMemoryAlloc( sizeof( ADAPTER_DETAILS ) );
    if ( !pAdapterDetails )
    {
        dwResult = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR( dwResult );
    }

    // add the new object to the list. if anything fails
    // in this function the object would be automatically
    // dereferenced and removed from the list.
    InsertTailList( &g_AdaptersList, &(pAdapterDetails->Link) );

    // initialize fields that are used by object lifetime management.
    hIhvExtAdapter                          =   (HANDLE) &(pAdapterDetails->Link);
    pAdapterDetails->dwRefCount             =   1;
    pAdapterDetails->hDot11SvcHandle        =   hDot11SvcHandle;
    pAdapterDetails->NicState               =   nic_state_initialized;


    // Event that gets triggered
    // once UI response is received.
    pAdapterDetails->hUIResponse =
    CreateEvent
    (
        NULL,
        FALSE, // Auto Reset
        FALSE, // Start in non-signaled state.
        NULL
    );
    if ( !(pAdapterDetails->hUIResponse) )
    {
        dwResult = GetLastError( );
        BAIL_ON_WIN32_ERROR( dwResult );
    }


    // fill out-params. release responsibility to
    // for deiniting object to caller.
    (*phIhvExtAdapter)  =   hIhvExtAdapter;
    hIhvExtAdapter      =   NULL;

error:
    if ( hIhvExtAdapter )
    {
        // something failed after adding the object to
        // the global list. so it needs to be removed.
        DerefenceAdapterDetails( hIhvExtAdapter );
    }
    if ( bLocked )
    {
        // leave global lock.
        LeaveCriticalSection( &g_csSynch );
    }
    return dwResult;
}






//
// Add a reference to a particular adapter and find the pointer to the context.
//
DWORD
ReferenceAdapterDetails
(
    HANDLE              hIhvExtAdapter,
    PADAPTER_DETAILS*   ppAdapterDetails
)
{
    DWORD               dwResult        =   ERROR_NOT_FOUND;
    BOOL                bLocked         =   FALSE;
    PLIST_ENTRY         pEntry          =   NULL;
    PADAPTER_DETAILS    pAdapterDetails =   NULL;

    ASSERT( ppAdapterDetails );

    // acquire global lock.
    EnterCriticalSection( &g_csSynch );
    bLocked = TRUE;


    // search the global list of adapters for a match.
    for
    (
        pEntry  =   g_AdaptersList.Flink;
        pEntry  !=  &g_AdaptersList;
        pEntry  =   pEntry->Flink
    )
    {
        // Get the adapter data structure for current entry.
        pAdapterDetails = CONTAINING_RECORD( pEntry, ADAPTER_DETAILS, Link );
        ASSERT( pAdapterDetails );

        // conditions could be converted to an ASSERT.
        if
        (
            ( pAdapterDetails->NicState == nic_state_uninitialized ) ||
            ( pAdapterDetails->NicState >= nic_state_max )
        )
        {
            ASSERTFAILURE();
            dwResult = ERROR_INVALID_STATE;
            BAIL_ON_WIN32_ERROR( dwResult );
        }

        // if match found - bail with success.
        if ( hIhvExtAdapter == ((HANDLE) pEntry) )
        {
            (pAdapterDetails->dwRefCount)++;
            (*ppAdapterDetails) = pAdapterDetails;

            dwResult = ERROR_SUCCESS;
            BAIL( );
        }
    }

error:
    if ( bLocked )
    {
        // release global lock.
        LeaveCriticalSection( &g_csSynch );
    }
    return dwResult;
}





//
// Find adapter context pointer using UI request GUID.
//
DWORD
ReferenceAdapterDetailsByUIRequestGuid
(
    GUID*               pguidUIRequest,
    PADAPTER_DETAILS*   ppAdapterDetails,
    HANDLE*             phIhvExtAdapter
)
{
    DWORD               dwResult        =   ERROR_NOT_FOUND;
    BOOL                bLocked         =   FALSE;
    PLIST_ENTRY         pEntry          =   NULL;
    PADAPTER_DETAILS    pAdapterDetails =   NULL;

    ASSERT( ppAdapterDetails );
    ASSERT( phIhvExtAdapter );

    // acquire global lock.
    EnterCriticalSection( &g_csSynch );
    bLocked = TRUE;

    // look for adapter with matching ui request guid in the global list.
    for
    (
        pEntry  =   g_AdaptersList.Flink;
        pEntry  !=  &g_AdaptersList;
        pEntry  =   pEntry->Flink
    )
    {
        // get the current adapter's context info.
        pAdapterDetails = CONTAINING_RECORD( pEntry, ADAPTER_DETAILS, Link );
        ASSERT( pAdapterDetails );

        // could be an ASSERT.
        if
        (
            ( pAdapterDetails->NicState == nic_state_uninitialized ) ||
            ( pAdapterDetails->NicState >= nic_state_max )
        )
        {
            ASSERTFAILURE();
            dwResult = ERROR_INVALID_STATE;
            BAIL_ON_WIN32_ERROR( dwResult );
        }

        // if match found, bail with success.
        if ( pAdapterDetails->currentGuidUIRequest == (*pguidUIRequest) )
        {
            (pAdapterDetails->dwRefCount)++;
            (*ppAdapterDetails) = pAdapterDetails;
            (*phIhvExtAdapter) = (HANDLE) pEntry;

            dwResult = ERROR_SUCCESS;
            BAIL( );
        }
    }

error:
    if ( bLocked )
    {
        // leave global lock.
        LeaveCriticalSection( &g_csSynch );
    }
    return dwResult;
}






//
// Dereference an adapter - resources will be freed when the refcount
// goes to zero.
//
VOID
DerefenceAdapterDetails
(
    HANDLE      hIhvExtAdapter
)
{
    DWORD               dwResult        =   ERROR_SUCCESS;
    BOOL                bLocked         =   FALSE;
    PADAPTER_DETAILS    pAdapterDetails =   NULL;
    BOOL                bOk             =   TRUE;


    // acquire global lock.
    EnterCriticalSection( &g_csSynch );
    bLocked = TRUE;

    // trying to reference the adapter. the part of the
    // code that increments the refcount is not useful
    // here, we are only using the algorithm to find
    // the context pointer. the refcount would be decremented
    // after the function call if the call succeeds.
    dwResult =
    ReferenceAdapterDetails
    (
        hIhvExtAdapter,
        &pAdapterDetails
    );
    BAIL_ON_WIN32_ERROR( dwResult );

    ASSERT( pAdapterDetails );

    // Undoing the refcount increment in the previous call.
    (pAdapterDetails->dwRefCount)--;

    // could be an ASSERT - the adapter
    // context should not be in this state.
    if ( 0 == pAdapterDetails->dwRefCount )
    {
        dwResult = ERROR_INVALID_STATE;
        BAIL_ON_WIN32_ERROR( dwResult );
    }

    // performing the intended dereferencing.
    (pAdapterDetails->dwRefCount)--;

    // checking if the adapter entry can go.
    if ( pAdapterDetails->dwRefCount )
    {
        BAIL( );
    }

    // Since this was the last reference,
    // it is time to deinitialize the memory.

    // remove the adapter from the global list.
    RemoveEntryList( &(pAdapterDetails->Link) );

    // Close handle to the UI response event.
    if ( pAdapterDetails->hUIResponse )
    {
        bOk = CloseHandle( pAdapterDetails->hUIResponse );
        ASSERT( bOk );
    }

    // free the UI response memory.
    if ( pAdapterDetails->pbResponse )
    {
        PrivateMemoryFree( pAdapterDetails->pbResponse );
        pAdapterDetails->pbResponse = NULL;
    }

    // frees the connection specific data in the adapter data structure.
    FreeOnexData( &(pAdapterDetails->pOnexData) );

    // frees the profiles.
    FreeIhvConnectivityProfile( &(pAdapterDetails->pConnectivityProfile) );
    FreeIhvSecurityProfile( &(pAdapterDetails->pSecurityProfile) );

    // frees the context.
    SecureZeroMemory( pAdapterDetails, sizeof( ADAPTER_DETAILS ) );
    PrivateMemoryFree( pAdapterDetails );

error:
    if ( bLocked )
    {
        // release global lock.
        LeaveCriticalSection( &g_csSynch );
    }
    ASSERT( ERROR_SUCCESS == dwResult );
    return;
}

